#include "OgreNewt_Stdafx.h"
#include "OgreNewt_BuoyancyBody.h"
#include "OgreNewt_World.h"
#include "OgreNewt_BodyNotify.h"

#include "ndWorld.h"
#include "ndBodyKinematic.h"
#include "ndBodyDynamic.h"
#include "ndShapeInstance.h"
#include "ndSharedPtr.h"
#include "ndContact.h"
#include "ndBodyTriggerVolume.h"

#include <cmath>

namespace OgreNewt
{
    // ============================================================
    // Internal ND4 trigger implementing Archimedes buoyancy
    // ============================================================
    class OgreNewtBuoyancyTriggerVolume : public ndBodyTriggerVolume
    {
    public:
        explicit OgreNewtBuoyancyTriggerVolume(BuoyancyForceTriggerCallback* cb)
            : ndBodyTriggerVolume()
            , m_cb(cb)
            , m_gravity(0.0f, -9.81f, 0.0f, 0.0f)
            , m_viscosity(0.99f)
            , m_plane(ndVector::m_zero)
            , m_hasPlane(false)
            , m_time(0.0f)
            , m_waveAmplitude(0.0f)
            , m_waveFrequency(0.0f)
        {
            // Make sure this kinematic body has a non-zero mass matrix to keep ND happy.
            SetMassMatrix(1.0f, 1.0f, 1.0f, 1.0f);
        }

        void SetGravity(const ndVector& g)
        {
            m_gravity = g;
        }

        void SetViscosity(ndFloat32 v)
        {
            m_viscosity = v;
        }

        void SetPlane(const ndPlane& plane)
        {
            m_plane = plane;
            m_hasPlane = true;
        }

        void ClearPlane()
        {
            m_hasPlane = false;
            m_plane = ndVector::m_zero;
        }

        void SetWaveAmplitude(ndFloat32 amplitude)
        {
            m_waveAmplitude = amplitude;
        }

        void SetWaveFrequency(ndFloat32 frequency)
        {
            m_waveFrequency = frequency;
        }

        void OnTriggerEnter(ndBodyKinematic* const body, ndFloat32 timestep) override
        {
            if (!m_hasPlane)
                calculatePlane(body);

            if (m_cb)
            {
                if (Body* visitor = bodyToOgre(body))
                    m_cb->OnEnter(visitor);
            }
        }

        void OnTrigger(const ndContact* const contact, ndFloat32 timestep) override
        {
            // 1) Apply Archimedes buoyancy (pure ND4 style + optional waves)
            applyBuoyancy(contact, timestep);

            // 2) Forward "inside" event to OgreNewt callback (Lua etc.)
            if (!m_cb || !contact)
                return;

            ndBodyKinematic* const self = static_cast<ndBodyKinematic*>(this);
            ndBodyKinematic* const otherKinematic =
                (contact->GetBody0() != self)
                ? contact->GetBody0()->GetAsBodyKinematic()
                : contact->GetBody1()->GetAsBodyKinematic();

            if (!otherKinematic)
                return;

            if (Body* visitor = bodyToOgre(otherKinematic))
                m_cb->OnInside(visitor);
        }

        void OnTriggerExit(ndBodyKinematic* const body, ndFloat32 timestep) override
        {
            if (m_cb)
            {
                if (Body* visitor = bodyToOgre(body))
                    m_cb->OnExit(visitor);
            }
        }

    private:
        static Body* bodyToOgre(ndBodyKinematic* const body)
        {
            if (!body)
                return nullptr;

            if (ndBodyNotify* notify = body->GetNotifyCallback())
            {
                if (auto* ogreNotify = dynamic_cast<BodyNotify*>(notify))
                    return ogreNotify->GetOgreNewtBody();
            }
            return nullptr;
        }

        /// Optional: if no plane explicitly set, compute a plane by raycasting.
        void calculatePlane(ndBodyKinematic* const body)
        {
            class ndCastTriggerPlane : public ndRayCastClosestHitCallback
            {
            public:
                ndUnsigned32 OnRayPrecastAction(const ndBody* const body, const ndShapeInstance*) override
                {
                    // Ignore other trigger volumes
                    return ((ndBody*)body)->GetAsBodyTriggerVolume() ? 1u : 0u;
                }
            };

            ndMatrix matrix(body->GetMatrix());
            ndVector p0(matrix.m_posit);
            ndVector p1(matrix.m_posit);

            // TODO: Make configureable?
            p0.m_y += 30.0f;
            p1.m_y -= 30.0f;

            ndCastTriggerPlane rayCaster;
            m_hasPlane = body->GetScene()->RayCast(rayCaster, p0, p1);
            if (m_hasPlane)
            {
                ndFloat32 dist = -rayCaster.m_contact.m_normal.DotProduct(rayCaster.m_contact.m_point).GetScalar();
                m_plane = ndPlane(rayCaster.m_contact.m_normal, dist);
            }
        }

        void applyBuoyancy(const ndContact* const contact, ndFloat32 timestep)
        {
            if (!contact)
                return;

            ndBodyKinematic* const self = static_cast<ndBodyKinematic*>(this);
            ndBodyKinematic* const kinBody =
                (contact->GetBody0() != self)
                ? contact->GetBody0()->GetAsBodyKinematic()
                : contact->GetBody1()->GetAsBodyKinematic();

            if (!kinBody)
                return;

            ndBodyDynamic* const body = kinBody->GetAsBodyDynamic();
            if (!body)
                return;

            if (!m_hasPlane)
                calculatePlane(kinBody);

            if (!m_hasPlane || body->GetInvMass() == 0.0f)
                return;

            ndVector mass(body->GetMassMatrix());
            const ndFloat32 bodyMass = mass.m_w;
            if (bodyMass <= 0.0f)
                return;

            ndMatrix matrix(body->GetMatrix());
            ndShapeInstance& collision = body->GetCollisionShape();

            ndVector centerOfPressure(ndVector::m_zero);
            ndFloat32 volume =
                collision.CalculateBuoyancyCenterOfPresure(centerOfPressure, matrix, m_plane);

            if (volume <= 0.0f)
                return;

            ndVector cog(body->GetCentreOfMass());
            centerOfPressure -= matrix.TransformVector(cog);

            // Use per-body density from material userParam[0], like ND4 demo.
            ndShapeMaterial material(collision.GetMaterial());
            body->GetCollisionShape().SetMaterial(material);

            ndFloat32 density = material.m_userParam[0].m_floatData;
            if (density <= 0.0f)
                density = 0.7f; // "light-ish" default

            const ndFloat32 shapeVolume = collision.GetVolume();
            const ndFloat32 displacedVolume = density * shapeVolume;

            const ndFloat32 displacedMass = bodyMass * volume / displacedVolume;

            // Buoyant force is opposite to gravity.
            ndVector gravity = m_gravity;
            if (gravity.DotProduct(gravity).GetScalar() == 0.0f)
                return;

            ndVector force = gravity.Scale(-displacedMass);
            ndVector torque = centerOfPressure.CrossProduct(force);

            body->SetForce(body->GetForce() + force);
            body->SetTorque(body->GetTorque() + torque);

            // Damping: only vertical velocity & angular velocity
            const ndFloat32 drag = m_viscosity; // 0..1

            ndVector vel = body->GetVelocity();
            ndVector omega = body->GetOmega();

            vel.m_y *= drag;
            omega = omega * drag;

            body->SetVelocity(vel);
            body->SetOmega(omega);

            // Optional small wave force to keep gentle bobbing.
            if (m_waveAmplitude > ndFloat32(0.0f) && m_waveFrequency > ndFloat32(0.0f))
            {
                m_time += timestep;

                ndVector up = m_gravity.Scale(-1.0f);
                ndFloat32 len2 = up.DotProduct(up).GetScalar();
                if (len2 > ndFloat32(1.0e-6f))
                {
                    ndFloat32 invLen = ndFloat32(1.0f / std::sqrt(len2));
                    up = up.Scale(invLen);
                }

                const ndFloat32 twoPi = ndFloat32(6.28318530717958647692f);
                ndFloat32 omegaWave = twoPi * m_waveFrequency;
                ndFloat32 waveScalar = m_waveAmplitude * ndFloat32(std::sin(omegaWave * m_time));

                ndVector waveForce = up.Scale(waveScalar * bodyMass);
                body->SetForce(body->GetForce() + waveForce);
            }
        }

        BuoyancyForceTriggerCallback* m_cb;  // not owned

        ndVector  m_gravity;
        ndFloat32 m_viscosity;
        ndPlane   m_plane;
        bool      m_hasPlane;

        ndFloat32 m_time;
        ndFloat32 m_waveAmplitude;
        ndFloat32 m_waveFrequency;
    };

    // ============================================================
    // BuoyancyForceTriggerCallback (event layer only)
    // ============================================================
    BuoyancyForceTriggerCallback::BuoyancyForceTriggerCallback(
        Ogre::Real waterToSolidVolumeRatio,
        Ogre::Real viscosity)
        : m_waterToSolidVolumeRatio(waterToSolidVolumeRatio)
        , m_viscosity(viscosity)
    {
    }

    void BuoyancyForceTriggerCallback::OnEnter(const Body* visitor)
    {
        if (!visitor)
            return;

        ndShapeInstance* shape = visitor->getNewtonCollision();
        if (!shape)
            return;

        ndShapeMaterial mat(shape->GetMaterial());

        // Use waterToSolidVolumeRatio as "density factor" for ND4.
        mat.m_userParam[0].m_floatData = static_cast<ndFloat32>(m_waterToSolidVolumeRatio);
        shape->SetMaterial(mat);
    }

    void BuoyancyForceTriggerCallback::OnInside(const Body* /*visitor*/)
    {
        // No physics here – ND4 trigger already handles buoyancy.
        // Derived classes (PhysicsBuoyancyTriggerCallback) use this for Lua callbacks only.
    }

    void BuoyancyForceTriggerCallback::OnExit(const Body* /*visitor*/)
    {
        // No physics here either; pure event.
    }

    void BuoyancyForceTriggerCallback::update(Ogre::Real)
    {
        // No-op; kept for possible future use.
    }

    void BuoyancyForceTriggerCallback::setWaterToSolidVolumeRatio(Ogre::Real waterToSolidVolumeRatio)
    {
        m_waterToSolidVolumeRatio = waterToSolidVolumeRatio;
    }

    Ogre::Real BuoyancyForceTriggerCallback::getWaterToSolidVolumeRatio() const
    {
        return m_waterToSolidVolumeRatio;
    }

    void BuoyancyForceTriggerCallback::setViscosity(Ogre::Real viscosity)
    {
        m_viscosity = viscosity;
    }

    Ogre::Real BuoyancyForceTriggerCallback::getViscosity() const
    {
        return m_viscosity;
    }

    // ============================================================
    // BuoyancyBody implementation
    // ============================================================
    BuoyancyBody::BuoyancyBody(
        World* world,
        Ogre::SceneManager* sceneManager,
        const CollisionPtr& col,
        BuoyancyForceTriggerCallback* buoyancyForceTriggerCallback)
        : Body(world, sceneManager, Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC)
        , m_fluidPlane(Ogre::Plane(Ogre::Vector3::UNIT_Y, 0.0f))
        , m_waterToSolidVolumeRatio(1.0f)
        , m_viscosity(0.99f)
        , m_gravity(Ogre::Vector3(0.0f, -9.81f, 0.0f))
        , m_useRaycastPlane(false)
        , m_waveAmplitude(0.0f)
        , m_waveFrequency(0.0f)
        , m_triggerBody(nullptr)
        , m_buoyancyForceTriggerCallback(buoyancyForceTriggerCallback)
    {
        reCreateTrigger(col);
    }

    BuoyancyBody::~BuoyancyBody()
    {
        if (m_buoyancyForceTriggerCallback)
        {
            delete m_buoyancyForceTriggerCallback;
            m_buoyancyForceTriggerCallback = nullptr;
        }
        m_triggerBody = nullptr;
    }

    void BuoyancyBody::reCreateTrigger(const CollisionPtr& col)
    {
        if (!col || !m_world)
            return;

        ndShapeInstance* srcInst = col->getShapeInstance();
        if (!srcInst && !col->getNewtonCollision())
            return;

        // Build shape instance copy on caller thread (safe)
        ndShapeInstance shapeCopy = srcInst
            ? ndShapeInstance(*srcInst)
            : ndShapeInstance(col->getNewtonCollision());

        // Allocate trigger on caller thread
        auto* newTrigger = new OgreNewtBuoyancyTriggerVolume(m_buoyancyForceTriggerCallback);

        // Cache current config values on caller thread (avoid reading members inside closure if you want)
        const Ogre::Vector3 gravity = m_gravity;
        const Ogre::Real viscosity = m_viscosity;
        const Ogre::Real waveAmplitude = m_waveAmplitude;
        const Ogre::Real waveFrequency = m_waveFrequency;
        const bool useRaycastPlane = m_useRaycastPlane;
        const Ogre::Plane fluidPlane = m_fluidPlane;

        if (!m_bodyNotify)
            m_bodyNotify = new BodyNotify(this);

        // Cache old body (if recreating)
        ndBodyKinematic* oldBody = m_body;
        auto* oldTrigger = static_cast<OgreNewtBuoyancyTriggerVolume*>(m_triggerBody);

        // World mutations must be on world thread / safe point
        m_world->enqueuePhysicsAndWait(
            [this,
            newTrigger,
            oldBody,
            oldTrigger,
            shapeCopy,
            gravity,
            viscosity,
            waveAmplitude,
            waveFrequency,
            useRaycastPlane,
            fluidPlane](World& w) mutable
            {
                // Remove old trigger/body if present
                if (oldBody)
                {
                    oldBody->SetNotifyCallback(nullptr);
                    w.destroyBody(oldBody);

                    // If you own the old trigger, delete it
                    delete oldBody;
                }

                // Configure new trigger (ND mutations)
                newTrigger->SetGravity(ndVector(
                    (ndFloat32)gravity.x,
                    (ndFloat32)gravity.y,
                    (ndFloat32)gravity.z,
                    0.0f));

                newTrigger->SetViscosity((ndFloat32)viscosity);
                newTrigger->SetWaveAmplitude((ndFloat32)waveAmplitude);
                newTrigger->SetWaveFrequency((ndFloat32)waveFrequency);

                if (!useRaycastPlane)
                {
                    ndVector n((ndFloat32)fluidPlane.normal.x,
                        (ndFloat32)fluidPlane.normal.y,
                        (ndFloat32)fluidPlane.normal.z,
                        0.0f);

                    ndFloat32 d = (ndFloat32)fluidPlane.d;
                    ndPlane plane(n, d);
                    newTrigger->SetPlane(plane);
                }
                else
                {
                    newTrigger->ClearPlane();
                }

                newTrigger->SetMatrix(ndGetIdentityMatrix());
                newTrigger->SetCollisionShape(shapeCopy);

                // Install pointers
                m_triggerBody = newTrigger;
                m_body = newTrigger;

                // Attach notify + add to world
                newTrigger->SetNotifyCallback(m_bodyNotify);
                w.addBody(newTrigger);
            });
    }

    void BuoyancyBody::setUseRaycastPlane(bool useRaycast)
    {
        m_useRaycastPlane = useRaycast;

        if (auto* trigger = static_cast<OgreNewtBuoyancyTriggerVolume*>(m_triggerBody))
        {
            if (m_useRaycastPlane)
            {
                // Switch to auto-plane mode
                trigger->ClearPlane();
            }
            else
            {
                // Reapply current m_fluidPlane as fixed plane
                const Ogre::Plane& fp = m_fluidPlane;
                ndVector n((ndFloat32)fp.normal.x, (ndFloat32)fp.normal.y, (ndFloat32)fp.normal.z, 0.0f);
                ndFloat32 d = (ndFloat32)fp.d;
                ndPlane plane(n, d);
                trigger->SetPlane(plane);
            }
        }
    }

    bool BuoyancyBody::getUseRaycastPlane() const
    {
        return m_useRaycastPlane;
    }

    void BuoyancyBody::setFluidPlane(const Ogre::Plane& fluidPlane)
    {
        m_fluidPlane = fluidPlane;
        m_useRaycastPlane = false;   // explicit plane: turn off raycast mode

        if (auto* trigger = static_cast<OgreNewtBuoyancyTriggerVolume*>(m_triggerBody))
        {
            ndVector n((ndFloat32)fluidPlane.normal.x, (ndFloat32)fluidPlane.normal.y, (ndFloat32)fluidPlane.normal.z, 0.0f);
            ndFloat32 d = (ndFloat32)fluidPlane.d;
            ndPlane plane(n, d);
            trigger->SetPlane(plane);
        }
    }

    Ogre::Plane BuoyancyBody::getFluidPlane() const
    {
        return m_fluidPlane;
    }

    void BuoyancyBody::setWaterToSolidVolumeRatio(Ogre::Real waterToSolidVolumeRatio)
    {
        m_waterToSolidVolumeRatio = waterToSolidVolumeRatio;

        if (m_buoyancyForceTriggerCallback)
            m_buoyancyForceTriggerCallback->setWaterToSolidVolumeRatio(waterToSolidVolumeRatio);
    }

    Ogre::Real BuoyancyBody::getWaterToSolidVolumeRatio() const
    {
        return m_waterToSolidVolumeRatio;
    }

    void BuoyancyBody::setViscosity(Ogre::Real viscosity)
    {
        m_viscosity = viscosity;

        if (m_buoyancyForceTriggerCallback)
            m_buoyancyForceTriggerCallback->setViscosity(viscosity);

        if (auto* trigger = static_cast<OgreNewtBuoyancyTriggerVolume*>(m_triggerBody))
            trigger->SetViscosity((ndFloat32)viscosity);
    }

    Ogre::Real BuoyancyBody::getViscosity() const
    {
        return m_viscosity;
    }

    void BuoyancyBody::setGravity(const Ogre::Vector3& gravity)
    {
        m_gravity = gravity;

        if (auto* trigger = static_cast<OgreNewtBuoyancyTriggerVolume*>(m_triggerBody))
        {
            trigger->SetGravity(ndVector(
                (ndFloat32)gravity.x,
                (ndFloat32)gravity.y,
                (ndFloat32)gravity.z,
                0.0f));
        }
    }

    Ogre::Vector3 BuoyancyBody::getGravity() const
    {
        return m_gravity;
    }

    void BuoyancyBody::setWaveAmplitude(Ogre::Real waveAmplitude)
    {
        m_waveAmplitude = waveAmplitude;

        if (auto* trigger = static_cast<OgreNewtBuoyancyTriggerVolume*>(m_triggerBody))
        {
            trigger->SetWaveAmplitude((ndFloat32)waveAmplitude);
        }
    }

    Ogre::Real BuoyancyBody::getWaveAmplitude() const
    {
        return m_waveAmplitude;
    }

    void BuoyancyBody::setWaveFrequency(Ogre::Real waveFrequency)
    {
        m_waveFrequency = waveFrequency;

        if (auto* trigger = static_cast<OgreNewtBuoyancyTriggerVolume*>(m_triggerBody))
        {
            trigger->SetWaveFrequency((ndFloat32)waveFrequency);
        }
    }

    Ogre::Real BuoyancyBody::getWaveFrequency() const
    {
        return m_waveFrequency;
    }

    BuoyancyForceTriggerCallback* BuoyancyBody::getTriggerCallback() const
    {
        return m_buoyancyForceTriggerCallback;
    }

    void BuoyancyBody::update(Ogre::Real dt)
    {
        if (m_buoyancyForceTriggerCallback)
            m_buoyancyForceTriggerCallback->update(dt);
    }

} // namespace OgreNewt

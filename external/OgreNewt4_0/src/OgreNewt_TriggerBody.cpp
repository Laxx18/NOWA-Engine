#include "OgreNewt_Stdafx.h"
#include "OgreNewt_TriggerBody.h"
#include "OgreNewt_Tools.h"
#include "OgreNewt_BodyNotify.h"   // to recover OgreNewt::Body from ndBodyNotify
#include "OgreNewt_Collision.h"

#include "ndWorld.h"
#include "ndBodyKinematic.h"
#include "ndBodyTriggerVolume.h"
#include "ndShapeInstance.h"
#include "ndSharedPtr.h"

namespace OgreNewt
{
    // Small adapter that forwards ND4 trigger callbacks to your TriggerCallback
    class OgreNewtTriggerVolume : public ndBodyTriggerVolume
    {
    public:
        OgreNewtTriggerVolume(TriggerCallback* cb)
            : ndBodyTriggerVolume(), m_cb(cb)
        {
        }

        void OnTriggerEnter(ndBodyKinematic* const body, ndFloat32 timestep) override
        {
            (void)timestep;
            if (!m_cb) return;
            if (OgreNewt::Body* visitor = bodyToOgre(body))
            {
                m_cb->OnEnter(visitor);
            }
        }

        void OnTrigger(const ndContact* const contact, ndFloat32 timestep) override
        {
            (void)timestep;
            if (!m_cb || !contact)
                return;

            // The trigger itself is this ndBodyTriggerVolume (which is an ndBodyKinematic)
            ndBodyKinematic* const self = static_cast<ndBodyKinematic*>(this);

            // Figure out which body is the "other" one in the contact
            ndBodyKinematic* otherKinematic = nullptr;

            if (contact->GetBody0() != self)
            {
                otherKinematic = contact->GetBody0()->GetAsBodyKinematic();
            }
            else
            {
                otherKinematic = contact->GetBody1()->GetAsBodyKinematic();
            }

            if (!otherKinematic)
                return;

            if (OgreNewt::Body* visitor = bodyToOgre(otherKinematic))
            {
                // This is your "stay / inside" callback
                m_cb->OnInside(visitor);
            }
        }

        void OnTriggerExit(ndBodyKinematic* const body, ndFloat32 timestep) override
        {
            (void)timestep;
            if (!m_cb) return;
            if (OgreNewt::Body* visitor = bodyToOgre(body))
            {
                m_cb->OnExit(visitor);
            }
        }

    private:
        static OgreNewt::Body* bodyToOgre(ndBodyKinematic* const body)
        {
            if (!body) return nullptr;
            if (ndBodyNotify* notify = body->GetNotifyCallback())
            {
                // Your BodyNotify stores a back-pointer to OgreNewt::Body
                if (auto* ogreNotify = dynamic_cast<BodyNotify*>(notify)) {
                    return ogreNotify->GetOgreNewtBody();
                }
            }
            return nullptr;
        }

        TriggerCallback* m_cb; // not owned; TriggerBody owns it (same as ND3 pattern)
    };

    // -------- TriggerBody --------

    TriggerBody::TriggerBody(World* world,
        Ogre::SceneManager* sceneManager,
        const OgreNewt::CollisionPtr& col,
        TriggerCallback* triggerCallback,
        Ogre::SceneMemoryMgrTypes memoryType)
        : Body(world, sceneManager, memoryType)
        , m_triggerVolume(nullptr)
        , m_triggerCallback(triggerCallback)
    {
        reCreateTrigger(col);
    }

    TriggerBody::~TriggerBody()
    {
        // We do NOT delete m_triggerVolume here because it is owned by the ND4 world (shared_ptr).
        // Just null our raw pointer.
        m_triggerVolume = nullptr;

        if (m_triggerCallback)
        {
            delete m_triggerCallback;
            m_triggerCallback = nullptr;
        }
    }

    void TriggerBody::reCreateTrigger(const OgreNewt::CollisionPtr& col)
    {
        if (!m_world || !col)
            return;

        ndWorld* const world = m_world->getNewtonWorld();
        if (!world)
            return;

        // Create a fresh trigger volume that forwards to our user callback
        OgreNewtTriggerVolume* trigger = new OgreNewtTriggerVolume(m_triggerCallback);

        // Identity matrix by default; you can move it later via setPositionOrientation or velocities.
        ndMatrix matrix(ndGetIdentityMatrix());
        trigger->SetMatrix(matrix);

        // Prefer the collision's ndShapeInstance if it has one (keeps local transform)
        ndShapeInstance* srcInst = col->getShapeInstance();
        if (!srcInst && !col->getNewtonCollision())
            return;

        ndShapeInstance shapeInstance = srcInst
            ? ndShapeInstance(*srcInst)                          // copy instance incl. local matrix
            : ndShapeInstance(col->getNewtonCollision());        // fallback: build from raw shape

        trigger->SetCollisionShape(shapeInstance);

        // Add to world (ND4 takes ownership via shared_ptr)
        ndSharedPtr<ndBody> bodyPtr(trigger);
        world->AddBody(bodyPtr);

        // Keep a raw convenience pointer and also set our base-class m_body to this
        m_triggerVolume = trigger;
        m_body = trigger;

        // Attach our BodyNotify so scene nodes update normally (same as Body ctor does)
        if (!m_bodyNotify)
        {
            m_bodyNotify = new BodyNotify(this);
        }
        m_body->SetNotifyCallback(m_bodyNotify);

        // For triggers, no damping/mass required; leave as purely kinematic/trigger.
    }

    TriggerCallback* TriggerBody::getTriggerCallback(void) const
    {
        return m_triggerCallback;
    }

    void TriggerBody::integrateVelocity(Ogre::Real dt)
    {
        // ND3: NewtonBodyIntegrateVelocity(m_body, dt);
        // ND4: integrate on the kinematic body
        if (m_body)
        {
            m_body->IntegrateVelocity(ndFloat32(dt));
        }
    }

} // namespace OgreNewt

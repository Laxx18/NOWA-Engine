#include "OgreNewt_Stdafx.h"
#include "OgreNewt_BuoyancyBody.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_World.h"
#include "OgreNewt_BodyNotify.h"
#include "ndWorld.h"
#include "ndBodyKinematic.h"
#include "ndShapeInstance.h"
#include "ndSharedPtr.h"

namespace OgreNewt
{
	// ================================================================
	// Adapter: forwards ND4 trigger events to BuoyancyForceTriggerCallback
	// ================================================================
	class OgreNewtBuoyancyTriggerVolume : public ndBodyTriggerVolume
	{
	public:
		OgreNewtBuoyancyTriggerVolume(BuoyancyForceTriggerCallback* cb)
			: ndBodyTriggerVolume(), m_cb(cb)
		{
		}

		void OnTriggerEnter(ndBodyKinematic* const body, ndFloat32 timestep) override
		{
			(void)timestep;
			if (!m_cb) return;
			if (Body* visitor = bodyToOgre(body))
				m_cb->OnEnter(visitor);
		}

		void OnTrigger(const ndContact* const contact, ndFloat32 timestep) override
		{
			(void)timestep;
			if (!m_cb || !contact)
				return;

			// Select the body that is NOT the trigger volume itself
			ndBodyKinematic* const self = static_cast<ndBodyKinematic*>(this);
			ndBodyKinematic* const otherKinematic =
				(contact->GetBody0() != self)
				? contact->GetBody0()->GetAsBodyKinematic()
				: contact->GetBody1()->GetAsBodyKinematic();

			if (!otherKinematic)
				return;

			if (Body* visitor = bodyToOgre(otherKinematic))
			{
				// This is your high-level "inside" generic callback
				m_cb->OnInside(visitor);
			}
		}

		void OnTriggerExit(ndBodyKinematic* const body, ndFloat32 timestep) override
		{
			(void)timestep;
			if (!m_cb) return;
			if (Body* visitor = bodyToOgre(body))
				m_cb->OnExit(visitor);
		}

	private:
		static Body* bodyToOgre(ndBodyKinematic* const body)
		{
			if (!body) return nullptr;
			if (ndBodyNotify* notify = body->GetNotifyCallback())
			{
				if (auto* ogreNotify = dynamic_cast<BodyNotify*>(notify))
					return ogreNotify->GetOgreNewtBody();
			}
			return nullptr;
		}

		BuoyancyForceTriggerCallback* m_cb; // not owned
	};

	// ================================================================
	// ndArchimedesBuoyancyVolume (for optional real buoyancy physics)
	// ================================================================
	ndArchimedesBuoyancyVolume::ndArchimedesBuoyancyVolume()
		: ndBodyTriggerVolume()
		, m_gravity(ndVector(0.0f, 0.0f, 0.0f, 0.0f))
		, m_density(1.0f)
		, m_viscosity(0.99f)
		, m_plane(ndVector(0.0f, 0.0f, 0.0f, 0.0f))
		, m_hasPlane(false)
	{
	}

	void ndArchimedesBuoyancyVolume::CalculatePlane(ndBodyKinematic* const body)
	{
		class ndCastTriggerPlane : public ndRayCastClosestHitCallback
		{
		public:
			ndUnsigned32 OnRayPrecastAction(const ndBody* const body, const ndShapeInstance*) override
			{
				return ((ndBody*)body)->GetAsBodyTriggerVolume() ? 1 : 0;
			}
		};

		ndMatrix matrix(body->GetMatrix());
		ndVector p0(matrix.m_posit);
		ndVector p1(matrix.m_posit);

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

	void ndArchimedesBuoyancyVolume::OnTriggerEnter(ndBodyKinematic* const body, ndFloat32)
	{
		CalculatePlane(body);
	}

	void ndArchimedesBuoyancyVolume::OnTrigger(const ndContact* const contact, ndFloat32)
	{
		if (!contact)
			return;

		// Pick the body that is NOT the trigger
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

		// Make sure we have a plane (in case OnTriggerEnter isn't called for some reason)
		if (!m_hasPlane)
		{
			CalculatePlane(kinBody);
		}

		if (!m_hasPlane || body->GetInvMass() == 0.0f)
			return;

		ndVector mass(body->GetMassMatrix());
		ndVector centerOfPressure(ndVector(0.0f, 0.0f, 0.0f, 0.0f));
		ndMatrix matrix(body->GetMatrix());
		ndShapeInstance& collision = body->GetCollisionShape();

		ndFloat32 volume = collision.CalculateBuoyancyCenterOfPresure(centerOfPressure, matrix, m_plane);
		if (volume <= 0.0f)
			return;

		// Apply buoyancy force and drag (same logic as before)
		ndVector cog(body->GetCentreOfMass());
		centerOfPressure -= matrix.TransformVector(cog);

		ndFloat32 displacedMass = mass.m_w * volume * m_density;
		ndVector force = m_gravity.Scale(-displacedMass);
		ndVector torque = centerOfPressure.CrossProduct(force);

		body->SetForce(body->GetForce() + force);
		body->SetTorque(body->GetTorque() + torque);

		// Viscous damping
		ndVector omega = body->GetOmega() * m_viscosity;
		ndVector veloc = body->GetVelocity() * m_viscosity;
		body->SetOmega(omega);
		body->SetVelocity(veloc);
	}

	void ndArchimedesBuoyancyVolume::OnTriggerExit(ndBodyKinematic* const, ndFloat32)
	{
	}

	// ================================================================
	// BuoyancyForceTriggerCallback
	// ================================================================
	BuoyancyForceTriggerCallback::BuoyancyForceTriggerCallback(
		const Ogre::Plane& fluidPlane,
		Ogre::Real waterToSolidVolumeRatio,
		Ogre::Real viscosity,
		const Ogre::Vector3& gravity,
		Ogre::Real waterSurfaceRestHeight)
		: m_fluidPlane(fluidPlane)
		, m_waterToSolidVolumeRatio(waterToSolidVolumeRatio)
		, m_viscosity(viscosity)
		, m_gravity(gravity)
		, m_waterSurfaceRestHeight(waterSurfaceRestHeight)
	{
	}

	void BuoyancyForceTriggerCallback::OnEnter(const Body* visitor)
	{
		ndShapeInstance* shape = visitor->getNewtonCollision();
		if (!shape)
			return;

		ndShapeMaterial mat = shape->GetMaterial();
		mat.m_userParam[0].m_floatData = (ndFloat32)m_waterToSolidVolumeRatio;
		shape->SetMaterial(mat);
	}

	void BuoyancyForceTriggerCallback::OnInside(const Body* visitor)
	{
		if (!visitor)
			return;

		Ogre::Vector3 vel = visitor->getVelocity();
		Ogre::Vector3 omega = visitor->getOmega();

		Ogre::Vector3 newVel = vel * m_viscosity;
		Ogre::Vector3 newOmega = omega * m_viscosity;

		const_cast<Body*>(visitor)->addForce((newVel - vel) * visitor->getMass());
		const_cast<Body*>(visitor)->setBodyAngularVelocity(newOmega, 1.0f / 60.0f);
	}

	void BuoyancyForceTriggerCallback::OnExit(const Body* visitor)
	{
		// Optional: gentle damping when leaving water
		if (!visitor)
			return;
		const Ogre::Vector3 vel = visitor->getVelocity() * 0.8f;
		const_cast<Body*>(visitor)->setVelocity(vel);
	}

	void BuoyancyForceTriggerCallback::update(Ogre::Real) {}

	void BuoyancyForceTriggerCallback::setFluidPlane(const Ogre::Plane& fluidPlane)
	{
		m_fluidPlane = fluidPlane;
	}

	Ogre::Plane BuoyancyForceTriggerCallback::getFluidPlane() const
	{
		return m_fluidPlane;
	}

	void BuoyancyForceTriggerCallback::setWaterToSolidVolumeRatio(Ogre::Real v)
	{
		m_waterToSolidVolumeRatio = v;
	}

	Ogre::Real BuoyancyForceTriggerCallback::getWaterToSolidVolumeRatio() const
	{
		return m_waterToSolidVolumeRatio;
	}

	void BuoyancyForceTriggerCallback::setViscosity(Ogre::Real v)
	{
		m_viscosity = v;
	}

	Ogre::Real BuoyancyForceTriggerCallback::getViscosity() const
	{
		return m_viscosity;
	}

	void BuoyancyForceTriggerCallback::setGravity(const Ogre::Vector3& g)
	{
		m_gravity = g;
	}

	Ogre::Vector3 BuoyancyForceTriggerCallback::getGravity() const
	{
		return m_gravity;
	}

	void BuoyancyForceTriggerCallback::setWaterSurfaceRestHeight(Ogre::Real h)
	{
		m_waterSurfaceRestHeight = h;
	}

	Ogre::Real BuoyancyForceTriggerCallback::getWaterSurfaceRestHeight() const
	{
		return m_waterSurfaceRestHeight;
	}

	// ================================================================
	// BuoyancyBody
	// ================================================================
	BuoyancyBody::BuoyancyBody(World* world, Ogre::SceneManager* sceneManager, const CollisionPtr& col, BuoyancyForceTriggerCallback* buoyancyForceTriggerCallback)
		: Body(world, sceneManager, Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC)
		, m_fluidPlane(Ogre::Plane(Ogre::Vector3::UNIT_Y, 0.0f))
		, m_waterToSolidVolumeRatio(1.0f)
		, m_viscosity(0.99f)
		, m_gravity(Ogre::Vector3(0.0f, -9.81f, 0.0f))
		, m_triggerBody(nullptr)
		, m_buoyancyForceTriggerCallback(buoyancyForceTriggerCallback)
	{
		// Keep behaviour consistent with legacy OgreNewt3 version
		reCreateTrigger(col);
	}

	BuoyancyBody::~BuoyancyBody()
	{
		m_triggerBody = nullptr;
		if (m_buoyancyForceTriggerCallback)
			delete m_buoyancyForceTriggerCallback;
	}

	void BuoyancyBody::reCreateTrigger(const CollisionPtr& col)
	{
		if (!col || !m_world)
			return;

		ndWorld* world = m_world->getNewtonWorld();
		if (!world)
			return;

		// Prefer the collision's ndShapeInstance if it has one (keeps local transform)
		ndShapeInstance* srcInst = col->getShapeInstance();
		if (!srcInst && !col->getNewtonCollision())
			return;

		ndShapeInstance shapeCopy = srcInst
			? ndShapeInstance(*srcInst)                          // copy instance incl. local matrix
			: ndShapeInstance(col->getNewtonCollision());        // fallback: build from raw shape

		// create trigger volume adapter
		auto* trigger = new OgreNewtBuoyancyTriggerVolume(m_buoyancyForceTriggerCallback);
		trigger->SetMatrix(ndGetIdentityMatrix());
		trigger->SetCollisionShape(shapeCopy);

		world->AddBody(ndSharedPtr<ndBody>(trigger));
		m_triggerBody = reinterpret_cast<ndArchimedesBuoyancyVolume*>(trigger);

		// assign BodyNotify to sync scene nodes
		if (!m_bodyNotify)
			m_bodyNotify = new BodyNotify(this);

		trigger->SetNotifyCallback(m_bodyNotify);
		m_body = trigger;
	}

	void BuoyancyBody::setFluidPlane(const Ogre::Plane& fluidPlane)
	{
		m_fluidPlane = fluidPlane;
		if (m_buoyancyForceTriggerCallback)
			m_buoyancyForceTriggerCallback->setFluidPlane(fluidPlane);
	}

	Ogre::Plane BuoyancyBody::getFluidPlane() const { return m_fluidPlane; }
	void BuoyancyBody::setWaterToSolidVolumeRatio(Ogre::Real v)
	{
		m_waterToSolidVolumeRatio = v;
		if (m_buoyancyForceTriggerCallback)
			m_buoyancyForceTriggerCallback->setWaterToSolidVolumeRatio(v);
	}

	Ogre::Real BuoyancyBody::getWaterToSolidVolumeRatio() const
	{
		return m_waterToSolidVolumeRatio;
	}

	void BuoyancyBody::setViscosity(Ogre::Real v)
	{
		m_viscosity = v;
		if (m_buoyancyForceTriggerCallback)
			m_buoyancyForceTriggerCallback->setViscosity(v);
	}

	Ogre::Real BuoyancyBody::getViscosity() const
	{
		return m_viscosity;
	}

	void BuoyancyBody::setGravity(const Ogre::Vector3& g)
	{
		m_gravity = g;
		if (m_buoyancyForceTriggerCallback)
			m_buoyancyForceTriggerCallback->setGravity(g);
	}

	Ogre::Vector3 BuoyancyBody::getGravity() const
	{
		return m_gravity;
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

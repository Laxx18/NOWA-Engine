#include "OgreNewt_Stdafx.h"
#include "OgreNewt_BuoyancyBody.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_Tools.h"
#include "OGreNewt_World.h"

namespace
{
	static unsigned ___dRandSeed___ = 0;

#define dRAND_MAX		0x0fffffff

	void dSetRandSeed(unsigned seed)
	{
		___dRandSeed___ = seed;
	}

	unsigned dRand()
	{
#define RAND_MUL 31415821u
		___dRandSeed___ = RAND_MUL * ___dRandSeed___ + 1;
		return ___dRandSeed___ & dRAND_MAX;
	}

	// a pseudo Gaussian random with mean 0 and variance 0.5f
	dFloat dGaussianRandom(dFloat amp)
	{
		unsigned val;
		val = dRand() + dRand();
		return amp * (dFloat(val) / dFloat(dRAND_MAX) - 1.0f) * 0.5f;
	}
}

namespace OgreNewt
{
	BuoyancyForceTriggerCallback::BuoyancyForceTriggerCallback(Ogre::Plane& fluidPlane, Ogre::Real waterToSolidVolumeRatio, Ogre::Real viscosity, const Ogre::Vector3& gravity, Ogre::Real waterSurfaceRestHeight)
		: TriggerCallback(),
		m_fluidPlane(fluidPlane),
		m_waterToSolidVolumeRatio(waterToSolidVolumeRatio),
		m_viscosity(viscosity),
		m_gravity(gravity),
		m_waterSurfaceRestHeight(waterSurfaceRestHeight),
		m_triggerController(nullptr)
	{

	}

	void BuoyancyForceTriggerCallback::OnEnter(const OgreNewt::Body* visitor)
	{
		// make some random density, and store on the collision shape for more interesting effect. 
		// dFloat density = 1.1f + dGaussianRandom(0.4f);
		const dFloat density = 1.35f;

		NewtonCollision* collision = NewtonBodyGetCollision(visitor->getNewtonBody());

		NewtonCollisionMaterial collisionMaterial;
		NewtonCollisionGetMaterial(collision, &collisionMaterial);
		collisionMaterial.m_userParam[0].m_float = density;
		collisionMaterial.m_userParam[1].m_float = 0.0f;
		NewtonCollisionSetMaterial(collision, &collisionMaterial);
	}

	void BuoyancyForceTriggerCallback::OnInside(const OgreNewt::Body* visitor)
	{
		const dFloat mass = visitor->getMass();
		if (mass > 0.0f)
		{
			dMatrix matrix;
			OgreNewt::Converters::QuatPosToMatrix(visitor->getOrientation(), visitor->getPosition(), &matrix[0][0]);

			dVector centerOfPreasure(0.0f);
			dVector plane;

			// calculate the volume and center of mass of the shape under the water surface 
			plane = this->calculateWaterPlane(matrix.m_posit);

			const NewtonCollision* const collision = visitor->getNewtonCollision();
			const Ogre::Real volume = NewtonConvexCollisionCalculateBuoyancyVolume(collision, &matrix[0][0], &plane[0], &centerOfPreasure[0]);
			if (volume > 0.0f)
			{
				// if some part of the shape si under water, calculate the buoyancy force base on 
					// Archimedes's buoyancy principle, which is the buoyancy force is equal to the 
					// weight of the fluid displaced by the volume under water. 

				const NewtonBody* body = visitor->getNewtonBody();

				dVector cog(0.0f);
				const dFloat viscousDrag = 0.99f;
				//const dFloat solidDentityFactor = 1.35f;

				// Get the body density form the collision material.
				NewtonCollisionMaterial collisionMaterial;
				NewtonCollisionGetMaterial(collision, &collisionMaterial);
				const dFloat solidDentityFactor = collisionMaterial.m_userParam[0].m_float;

				// calculate the ratio of volumes an use it calculate a density equivalent
				const dFloat shapeVolume = NewtonConvexCollisionCalculateVolume(collision);
				const dFloat density = mass * solidDentityFactor / shapeVolume;

				const dFloat displacedMass = density * volume;
				NewtonBodyGetCentreOfMass(body, &cog[0]);
				centerOfPreasure -= matrix.TransformVector(cog);

				// now with the mass and center of mass of the volume under water, calculate buoyancy force and torque
				dVector force(0.0f, -m_gravity.y * displacedMass, 0.0f, 0.0f);
				dVector torque(centerOfPreasure.CrossProduct(force));

				NewtonBodyAddForce(body, &force[0]);
				NewtonBodyAddTorque(body, &torque[0]);

				// apply a fake viscous drag to damp the under water motion 
				dVector omega(0.0f);
				dVector veloc(0.0f);
				NewtonBodyGetOmega(body, &omega[0]);
				NewtonBodyGetVelocity(body, &veloc[0]);
				omega = omega.Scale(viscousDrag);
				veloc = veloc.Scale(viscousDrag);
				NewtonBodySetOmega(body, &omega[0]);
				NewtonBodySetVelocity(body, &veloc[0]);
			}
		}
	}

	void BuoyancyForceTriggerCallback::update(Ogre::Real dt)
	{
		CustomTriggerManager* const manager = (CustomTriggerManager*)m_triggerController->GetManager();
		manager->update(dt);
	}
		
	void BuoyancyForceTriggerCallback::setFluidPlane(const Ogre::Plane& fluidPlane)
	{
		m_fluidPlane = fluidPlane;
	}

	void BuoyancyForceTriggerCallback::setWaterToSolidVolumeRatio(Ogre::Real waterToSolidVolumeRatio)
	{
		m_waterToSolidVolumeRatio = waterToSolidVolumeRatio;
	}

	void BuoyancyForceTriggerCallback::setViscosity(Ogre::Real viscosity)
	{
		m_viscosity = viscosity;
	}

	void BuoyancyForceTriggerCallback::setGravity(const Ogre::Vector3& gravity)
	{
		m_gravity = gravity;
	}

	void BuoyancyForceTriggerCallback::setWaterSurfaceRestHeight(Ogre::Real waterSufaceRestHeight)
	{
		m_waterSurfaceRestHeight = waterSufaceRestHeight;
	}

	void BuoyancyForceTriggerCallback::setTriggerController(dCustomTriggerController* triggerController)
	{
		m_triggerController = triggerController;
	}

	dVector BuoyancyForceTriggerCallback::calculateSurfacePosition(const dVector& position) const
	{
		CustomTriggerManager* const manager = (CustomTriggerManager*)m_triggerController->GetManager();
		// CustomTriggerManager* const triggerManager = m_world->getTriggerManager();

		dFloat xAngle = 2.0f * (position.m_x / manager->getWavePeriod()) * dPi;
		dFloat yAngle = 2.0f * (position.m_z / manager->getWavePeriod()) * dPi;

		dFloat height = m_waterSurfaceRestHeight;
		dFloat amplitude = manager->getWaveAmplitude();
		for (int i = 0; i < 3; i++)
		{
			height += amplitude * dSin(xAngle + manager->getFaceAngle()) * dCos(yAngle + manager->getFaceAngle());
			amplitude *= 0.5f;
			xAngle *= 0.5f;
			yAngle *= 0.5f;
		}
		return dVector(position.m_x, height, position.m_z, 1.0f);
	}

	dVector BuoyancyForceTriggerCallback::calculateWaterPlane(const dVector& position) const
	{
		dVector origin(this->calculateSurfacePosition(position));

		dVector px0(position.m_x - 0.1f, position.m_y, position.m_z, position.m_w);
		dVector px1(position.m_x + 0.1f, position.m_y, position.m_z, position.m_w);
		dVector pz0(position.m_x, position.m_y, position.m_z - 0.1f, position.m_w);
		dVector pz1(position.m_x, position.m_y, position.m_z + 0.1f, position.m_w);

		dVector ex(calculateSurfacePosition(px1) - calculateSurfacePosition(px0));
		dVector ez(calculateSurfacePosition(pz1) - calculateSurfacePosition(pz0));

		dVector surfacePlane(ez.CrossProduct(ex));
		surfacePlane.m_w = 0.0f;
		surfacePlane = surfacePlane.Normalize();
		surfacePlane.m_w = -surfacePlane.DotProduct3(origin);
		return surfacePlane;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	BuoyancyBody::BuoyancyBody(World* world, Ogre::SceneManager* sceneManager, const OgreNewt::CollisionPtr& col, BuoyancyForceTriggerCallback* buoyancyForceTriggerCallback, 
		Ogre::SceneMemoryMgrTypes memoryType, Ogre::Plane& fluidPlane, Ogre::Real waterToSolidVolumeRatio, Ogre::Real viscosity, const Ogre::Vector3& gravity)
		: Body(world, sceneManager, memoryType),
		m_fluidPlane(fluidPlane),
		m_waterToSolidVolumeRatio(waterToSolidVolumeRatio),
		m_viscosity(viscosity),
		m_gravity(gravity),
		m_buoyancyForceTriggerCallback(buoyancyForceTriggerCallback),
		m_triggerController(nullptr)
	{
		this->reCreateTrigger(col);
	}

	BuoyancyBody::~BuoyancyBody()
	{
		if (nullptr == m_body)
			return;

		if (nullptr != m_buoyancyForceTriggerCallback)
		{
			delete m_buoyancyForceTriggerCallback;
			m_buoyancyForceTriggerCallback = nullptr;
		}
		if (nullptr != m_triggerController)
		{
			// This internally calls the OgreNewt destructor callback when specified (NewtonBodySetDestructorCallback)
			m_triggerController->GetManager()->DestroyTrigger(m_triggerController);
			m_triggerController = nullptr;
		}
	}

	void BuoyancyBody::reCreateTrigger(const OgreNewt::CollisionPtr& col)
	{
		if (nullptr != m_triggerController)
		{
			// This internally calls the OgreNewt destructor callback when specified (NewtonBodySetDestructorCallback)
			m_triggerController->GetManager()->DestroyTrigger(m_triggerController);
			m_triggerController = nullptr;
		}

		// Add a trigger Manager to the world
		CustomTriggerManager* const triggerManager = m_world->getTriggerManager();

		dFloat matrix[16];
		OgreNewt::Converters::QuatPosToMatrix(Ogre::Quaternion::IDENTITY, Ogre::Vector3::ZERO, matrix);

		m_triggerController = triggerManager->CreateTrigger(&matrix[0], col->getNewtonCollision(), m_buoyancyForceTriggerCallback);
		// m_triggerController->SetUserData(m_triggerCallback);

		if (nullptr != m_buoyancyForceTriggerCallback)
		{
			m_buoyancyForceTriggerCallback->setTriggerController(m_triggerController);
		}

		m_body = m_triggerController->GetBody();

		// NewtonBodySetSimulationState(m_body, 0);
		// NewtonBodySetCollidable(m_body, 0);
		NewtonBodySetUserData(m_body, this);
		// Check this
		// NewtonBodySetDestructorCallback(m_body, newtonDestructor);
// Attention: Is this required?
		// NewtonBodySetTransformCallback(m_body, newtonTransformCallback);

		NewtonDestroyCollision(col->getNewtonCollision());
	}

	void BuoyancyBody::setFluidPlane(const Ogre::Plane& fluidPlane)
	{
		m_fluidPlane = fluidPlane;
		m_buoyancyForceTriggerCallback->setFluidPlane(fluidPlane);
	}

	Ogre::Plane BuoyancyBody::getFluidPlane(void) const
	{
		return m_fluidPlane;
	}

	void BuoyancyBody::setWaterToSolidVolumeRatio(Ogre::Real waterToSolidVolumeRatio)
	{
		m_waterToSolidVolumeRatio = waterToSolidVolumeRatio;
		m_buoyancyForceTriggerCallback->setWaterToSolidVolumeRatio(waterToSolidVolumeRatio);
	}

	Ogre::Real BuoyancyBody::getWaterToSolidVolumeRatio(void) const
	{
		return m_waterToSolidVolumeRatio;
	}

	void BuoyancyBody::setViscosity(Ogre::Real viscosity)
	{
		m_viscosity = viscosity;
		m_buoyancyForceTriggerCallback->setViscosity(viscosity);
	}

	Ogre::Real BuoyancyBody::getViscosity(void) const
	{
		return m_viscosity;
	}

	void BuoyancyBody::setGravity(const Ogre::Vector3& gravity)
	{
		m_gravity = gravity;
		m_buoyancyForceTriggerCallback->setGravity(gravity);
	}

	Ogre::Vector3 BuoyancyBody::getGravity(void) const
	{
		return m_gravity;
	}

	BuoyancyForceTriggerCallback* BuoyancyBody::getTriggerCallback(void) const
	{
		return m_buoyancyForceTriggerCallback;
	}

	void BuoyancyBody::update(Ogre::Real dt)
	{
		m_buoyancyForceTriggerCallback->update(dt);
	}
}
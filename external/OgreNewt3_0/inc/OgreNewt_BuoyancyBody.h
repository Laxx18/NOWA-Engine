/*
OgreNewt Library

Ogre implementation of Newton Game Dynamics SDK

OgreNewt basically has no license, you may use any or all of the library however you desire... I hope it can help you in any way.


by Lax

*/
#ifndef OGRENEWT_BUOYANCY
#define OGRENEWT_BUOYANCY

#include "OgreNewt_Prerequisites.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_World.h"

// OgreNewt namespace.  all functions and classes use this namespace.
namespace OgreNewt
{
	class _OgreNewtExport BuoyancyForceTriggerCallback : public TriggerCallback
	{
	public:
		BuoyancyForceTriggerCallback(Ogre::Plane& fluidPlane, Ogre::Real waterToSolidVolumeRatio, Ogre::Real viscosity, const Ogre::Vector3& gravity, Ogre::Real waterSurfaceRestHeight);

		virtual void OnEnter(const OgreNewt::Body* visitor) override;

		virtual void OnInside(const OgreNewt::Body* visitor) override;

		void update(Ogre::Real dt);
			
		void setFluidPlane(const Ogre::Plane& fluidPlane);

		void setWaterToSolidVolumeRatio(Ogre::Real waterToSolidVolumeRatio);

		void setViscosity(Ogre::Real viscosity);

		void setGravity(const Ogre::Vector3& gravity);

		void setWaterSurfaceRestHeight(Ogre::Real waterSufaceRestHeight);

		void setTriggerController(dCustomTriggerController* triggerController);
	private:
		dVector calculateSurfacePosition(const dVector& position) const;

		dVector calculateWaterPlane(const dVector& position) const;

	private:
		Ogre::Plane m_fluidPlane;
		Ogre::Real m_waterToSolidVolumeRatio;
		Ogre::Real m_viscosity;
		Ogre::Vector3 m_gravity;
		Ogre::Real m_waterSurfaceRestHeight;
		dCustomTriggerController* m_triggerController;
	};

	class _OgreNewtExport BuoyancyBody : public Body
	{
	public:
		///!  Get the fluid plane for the upper face of the trigger volume
		BuoyancyBody(World* world, Ogre::SceneManager* sceneManager, const OgreNewt::CollisionPtr& col, BuoyancyForceTriggerCallback* buoyancyForceTriggerCallback, Ogre::SceneMemoryMgrTypes memoryType = Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC, 
			Ogre::Plane& fluidPlane = Ogre::Plane(Ogre::Vector3(0.0f, 1.0f, 0.0f), 1.5f), Ogre::Real waterToSolidVolumeRatio = 0.9f, Ogre::Real viscosity = 0.995f, 
			const Ogre::Vector3& gravity = Ogre::Vector3(0.0f, -10.0f, 0.0f));

		virtual ~BuoyancyBody();

		void setFluidPlane(const Ogre::Plane& fluidPlane);

		Ogre::Plane getFluidPlane(void) const;

		void setWaterToSolidVolumeRatio(Ogre::Real waterToSolidVolumeRatio);

		Ogre::Real getWaterToSolidVolumeRatio(void) const;

		void setViscosity(Ogre::Real viscosity);

		Ogre::Real getViscosity(void) const;

		void setGravity(const Ogre::Vector3& gravity);

		Ogre::Vector3 getGravity(void) const;

		void reCreateTrigger(const OgreNewt::CollisionPtr& col);

		BuoyancyForceTriggerCallback* getTriggerCallback(void) const;

		void update(Ogre::Real dt);

	private:
		Ogre::Plane m_fluidPlane;
		Ogre::Real m_waterToSolidVolumeRatio;
		Ogre::Real m_viscosity;
		Ogre::Vector3 m_gravity;
		dCustomTriggerController* m_triggerController;
		BuoyancyForceTriggerCallback* m_buoyancyForceTriggerCallback;
	};

};

#endif


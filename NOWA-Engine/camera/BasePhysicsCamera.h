#ifndef BASE_PHYSICS_CAMERA_H
#define BASE_PHYSICS_CAMERA_H

#include "BaseCamera.h"

namespace NOWA
{
	class Ogre::SceneManager;
	class Ogre::Camera;
	class Ogre::SceneNode;

	class EXPORTED BasePhysicsCamera : public BaseCamera
	{
	public:
		BasePhysicsCamera(Ogre::SceneManager* sceneManager, OgreNewt::World* ogreNewt, const Ogre::Vector3& cameraSize = Ogre::Vector3(1.0f, 1.0f, 1.0f),
			Ogre::Real moveSpeed = 20.0f, Ogre::Real rotateSpeed = 20.0f, Ogre::Real smoothValue = 0.1f);

		virtual ~BasePhysicsCamera();

		virtual void moveCamera(Ogre::Real dt);

		virtual void rotateCamera(Ogre::Real dt, bool forJoyStick = false);

		virtual Ogre::Vector3 getPosition(void);

		virtual Ogre::Quaternion getOrientation(void);

		virtual Ogre::String getBehaviorType(void) 
		{
			return "BASE_PHYSICS_CAMERA";
		}

		static Ogre::String BehaviorType(void)
		{
			return "BASE_PHYSICS_CAMERA";
		}

		friend class CameraManager;
	protected:

		virtual void onSetData(void);

		virtual void onClearData(void);
		
		virtual void moveCallback(OgreNewt::Body* body, Ogre::Real timeStep, int threadIndex);
	protected:
		Ogre::SceneManager* sceneManager;
		OgreNewt::World* ogreNewt;
		OgreNewt::Body* cameraBody;
		Ogre::Vector3 cameraSize;
		OgreNewt::UpVector* upVector;
	};

}; //namespace end

#endif
#ifndef BASE_CAMERA_H
#define BASE_CAMERA_H

#include "defines.h"

namespace NOWA
{
	class Ogre::SceneManager;
	class Ogre::Camera;
	class Ogre::SceneNode;

	class EXPORTED BaseCamera
	{
	public:
		friend class CameraManager;

		BaseCamera(Ogre::Real moveSpeed = 0.05f, Ogre::Real rotateSpeed = 0.05f, Ogre::Real smoothValue = 0.1f, const Ogre::Vector3& defaultDirection = Ogre::Vector3::NEGATIVE_UNIT_Z);

		virtual ~BaseCamera();

		virtual void setDefaultDirection(const Ogre::Vector3& defaultDirection);

		virtual void moveCamera(Ogre::Real dt);

		virtual void rotateCamera(Ogre::Real dt, bool forJoyStick = false);

		virtual Ogre::Vector3 getPosition(void);

		virtual Ogre::Quaternion getOrientation(void);

		virtual void setMoveSpeed(Ogre::Real moveSpeed);

		virtual void setRotationSpeed(Ogre::Real rotationSpeed);

		virtual void reset(void);

		virtual Ogre::String getBehaviorType(void) 
		{
			return "BASE_MOVE_CAMERA";
		}

		static Ogre::String BehaviorType(void)
		{
			return "BASE_MOVE_CAMERA";
		}

		Ogre::Camera* getCamera(void) const;
		
		void setSmoothValue(Ogre::Real smoothValue);
		
		Ogre::Real getSmoothValue(void) const;

		void setCameraControlLocked(bool cameraControlLocked);

		bool getCameraControlLocked(void) const;

	protected:

		virtual void onSetData(void);
		
		virtual void onClearData(void);
	private:
		void postInitialize(Ogre::Camera* camera);
	protected:
		Ogre::Camera* camera;

		Ogre::SceneNode* cameraNode;
		Ogre::Vector3 defaultDirection;
		Ogre::Real moveSpeed;
		Ogre::Real rotateSpeed;
		Ogre::Vector2 lastValue;
		bool firstTimeValueSet;
		Ogre::Vector3 lastMoveValue;
		bool firstTimeMoveValueSet;
		Ogre::Real smoothValue;
		bool cameraControlLocked;
		Ogre::Real moveCameraWeight;
		Ogre::Real rotateCameraWeight;
	};

}; //namespace end

#endif
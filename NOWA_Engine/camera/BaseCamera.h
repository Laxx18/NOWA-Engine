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

		BaseCamera(unsigned int id, Ogre::Real moveSpeed = 20.0f, Ogre::Real rotateSpeed = 1.0f, Ogre::Real smoothValue = 0.3f, const Ogre::Vector3& defaultDirection = Ogre::Vector3::NEGATIVE_UNIT_Z);

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
			return "BASE_MOVE_CAMERA_" + Ogre::StringConverter::toString(this->id);
		}

		static Ogre::String BehaviorType(void)
		{
			return "BASE_MOVE_CAMERA";
		}

		unsigned int getId(void) const;

		Ogre::Camera* getCamera(void) const;
		
		void setSmoothValue(Ogre::Real smoothValue);
		
		Ogre::Real getSmoothValue(void) const;

		void setCameraControlLocked(bool cameraControlLocked);

		bool getCameraControlLocked(void) const;

		void applyGravityDirection(const Ogre::Vector3& gravityDirection);

	protected:

		virtual void onSetData(void);
		
		virtual void onClearData(void);
	private:
		void postInitialize(Ogre::Camera* camera);
	protected:
		unsigned int id;
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
		Ogre::Vector3 gravityDirection;
	};

}; //namespace end

#endif
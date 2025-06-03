#ifndef THIRD_PERSON_CAMERA_H
#define THIRD_PERSON_CAMERA_H

#include "BaseCamera.h"

namespace NOWA
{
	class EXPORTED ThirdPersonCamera : public BaseCamera
	{
	public:

		ThirdPersonCamera(unsigned int id, Ogre::SceneNode* sceneNode, const Ogre::Vector3& defaultDirection = Ogre::Vector3::NEGATIVE_UNIT_Z, const Ogre::Vector3& offsetPosition = Ogre::Vector3(0.0f, 2.0f, 0.0f),
			const Ogre::Vector3& lookAtOffset = Ogre::Vector3::ZERO, Ogre::Real cameraSpring = 0.1f,
			Ogre::Real cameraFriction = 0.5f, Ogre::Real cameraSpringLength = 6.0f);

		virtual ~ThirdPersonCamera();

		virtual void moveCamera(Ogre::Real dt);

		virtual void rotateCamera(Ogre::Real dt, bool forJoyStick = false);

		virtual Ogre::Vector3 getPosition(void);

		virtual Ogre::Quaternion getOrientation(void);

		virtual Ogre::String getBehaviorType(void)
		{
			return "THIRD_PERSON_CAMERA_" + Ogre::StringConverter::toString(this->id);
		}

		static Ogre::String BehaviorType(void)
		{
			return "THIRD_PERSON_CAMERA";
		}

		void setOffsetPosition(const Ogre::Vector3& offsetPosition);

		void setCameraSpring(Ogre::Real cameraSpring);

		void setCameraFriction(Ogre::Real cameraFriction);

		void setCameraSpringLength(Ogre::Real cameraSpringLength);

		void setLookAtOffset(const Ogre::Vector3& lookAtOffset);

		void setSceneNode(Ogre::SceneNode* sceneNode);
	protected:

		virtual void onSetData(void);

	private:
		Ogre::Vector3 offsetPosition;
		Ogre::Real cameraSpring;
		Ogre::Real cameraFriction;
		Ogre::Real cameraSpringLength;
		Ogre::Vector3 lookAtOffset;
		Ogre::Vector3 lastSmoothedCameraPos;
		Ogre::SceneNode* sceneNode;
	};

}; //namespace end

#endif
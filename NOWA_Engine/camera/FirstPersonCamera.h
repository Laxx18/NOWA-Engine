#ifndef FIRST_PERSON_CAMERA_H
#define FIRST_PERSON_CAMERA_H

#include "BaseCamera.h"

namespace NOWA
{
	class EXPORTED FirstPersonCamera : public BaseCamera
	{
	public:
		
		FirstPersonCamera(unsigned int id, Ogre::SceneNode* sceneNode, const Ogre::Vector3& defaultDirection = Ogre::Vector3::NEGATIVE_UNIT_Z,
			Ogre::Real smoothValue = 0.3f, Ogre::Real rotateSpeed = 0.5f, const Ogre::Vector3& offsetPosition = Ogre::Vector3(0.4f, 0.8f, -0.9f));

		virtual ~FirstPersonCamera();

		virtual void moveCamera(Ogre::Real dt);

		virtual void rotateCamera(Ogre::Real dt, bool forJoyStick = false);

		virtual Ogre::Vector3 getPosition(void);

		virtual Ogre::Quaternion getOrientation(void);

		virtual Ogre::String getBehaviorType(void)
		{
			return "FIRST_PERSON_CAMERA_" + Ogre::StringConverter::toString(this->id);
		}

		static Ogre::String BehaviorType(void)
		{
			return "FIRST_PERSON_CAMERA";
		}

		void setRotateSpeed(Ogre::Real rotateSpeed = 0.5f);

		void setSceneNode(Ogre::SceneNode* sceneNode);
	protected:

		virtual void onSetData(void);
	private:
		Ogre::SceneNode* sceneNode;
		Ogre::Vector3 offsetPosition;
	};

}; //namespace end

#endif
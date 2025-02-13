#ifndef ATTACH_CAMERA_H
#define ATTACH_CAMERA_H

#include "BaseCamera.h"

namespace NOWA
{
	class EXPORTED AttachCamera : public BaseCamera
	{
	public:
		
		AttachCamera(unsigned int id, Ogre::SceneNode* sceneNode,
			const Ogre::Vector3& offsetPosition, const Ogre::Quaternion& offsetOrientation, Ogre::Real smoothValue = 0.3f, const Ogre::Vector3& defaultDirection = Ogre::Vector3::NEGATIVE_UNIT_Z);

		virtual ~AttachCamera();

		virtual void moveCamera(Ogre::Real dt);

		virtual Ogre::Vector3 getPosition(void);

		virtual Ogre::Quaternion getOrientation(void);

		virtual Ogre::String getBehaviorType(void)
		{
			return "ATTACH_CAMERA_" + Ogre::StringConverter::toString(this->id);
		}

		static Ogre::String BehaviorType(void)
		{
			return "ATTACH_CAMERA";
		}

		void setSceneNode(Ogre::SceneNode* sceneNode);
	protected:

		virtual void onSetData(void);
	private:
		Ogre::SceneNode* sceneNode;
		Ogre::Vector3 offsetPosition;
		Ogre::Quaternion offsetOrientation;
	};

}; //namespace end

#endif
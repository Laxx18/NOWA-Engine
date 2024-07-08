#ifndef FOLLOW_CAMERA_2D_H
#define FOLLOW_CAMERA_2D_H

#include "BaseCamera.h"
#include "main/Events.h"

namespace NOWA
{
	
	class EXPORTED FollowCamera2D : public BaseCamera
	{
	public:
		FollowCamera2D(unsigned int id, Ogre::SceneNode* sceneNode, const Ogre::Vector3& offsetPosition, Ogre::Real smoothValue = 0.0f);

		virtual ~FollowCamera2D();

		virtual void moveCamera(Ogre::Real dt);

		virtual void rotateCamera(Ogre::Real dt, bool forJoyStick = false);

		virtual Ogre::Vector3 getPosition(void);

		virtual Ogre::Quaternion getOrientation(void);

		virtual Ogre::String getBehaviorType(void) 
		{
			return "FOLLOW_CAMERA_" + Ogre::StringConverter::toString(this->id);
		}

		static Ogre::String BehaviorType(void)
		{
			return "FOLLOW_CAMERA";
		}

		/**
		* @brief		Sets the offset for the camera to the player
		* @param[in]	offset	The offset vector to set
		* @Note			Alone this offset is sufficient to set the camera always correctly when a level gets loaded.
		*				That is, the camera position depends on the position of the target scene node.
		*/
		void setOffset(const Ogre::Vector3& offsetPosition);

		/**
		* @brief		Sets the border offset, at which the camera will no more move
		* @param[in]	borderOffset	The border offset vector to set
		*/
		void setBorderOffset(const Ogre::Vector3& borderOffset);

		void setBounds(Ogre::Vector3& minimumBounds, Ogre::Vector3& maximumBounds);

		void alwaysShowGameObject(bool show, const Ogre::String& category, Ogre::SceneManager* sceneManager);

		void setSceneNode(Ogre::SceneNode* sceneNode);
	protected:
		
		virtual void onSetData(void);
	private:
		void handleUpdateBounds(NOWA::EventDataPtr eventData);
	protected:
		Ogre::SceneManager* sceneManager;
		bool firstTimeValueSet;
		Ogre::Vector3 lastMoveValue;
		bool firstTimeMoveValueSet;
		Ogre::Real smoothValue;
		Ogre::Vector3 offset;
		Ogre::Vector3 borderOffset;
		Ogre::Vector3 minimumBounds;
		Ogre::Vector3 maximumBounds;

		bool showGameObject;
		Ogre::RaySceneQuery* raySceneQuery;
		Ogre::String category;
		Ogre::SceneNode* hiddenSceneNode;
		Ogre::Real fadeValue;
		bool fadingFinished;
		Ogre::Vector3 mostRightUp;

		Ogre::ManualObject* pDebugLine;
		Ogre::SceneNode* sceneNode;
	};

}; //namespace end

#endif
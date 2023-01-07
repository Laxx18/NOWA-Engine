#ifndef ZOOM_CAMERA_H
#define ZOOM_CAMERA_H

#include "BaseCamera.h"

namespace NOWA
{
	class EXPORTED ZoomCamera : public BaseCamera
	{
	public:
		ZoomCamera(const Ogre::Vector3& offsetPosition, Ogre::Real smoothValue = 0.0f);

		virtual ~ZoomCamera();

		virtual void moveCamera(Ogre::Real dt) override;

		virtual void rotateCamera(Ogre::Real dt, bool forJoyStick = false) override;

		virtual Ogre::Vector3 getPosition(void) override;

		virtual Ogre::Quaternion getOrientation(void) override;

		virtual Ogre::String getBehaviorType(void) 
		{
			return "ZOOM_CAMERA";
		}
		
		/**
		* @brief		Sets the direction at which the camera will zoom for GameObjects of the specified category
		* @param[in]	direction	The direction vector to set
		*/
		virtual void setDefaultDirection(const Ogre::Vector3& direction) override;

		static Ogre::String BehaviorType(void)
		{
			return "ZOOM_CAMERA";
		}
		
		/**
		* @brief		Sets the category, for which the bounds are calculated and the zoom adapted, so that all GameObjects of this category are visible.
		* @param[in]	category	The category to set
		*/
		void setCategory(const Ogre::String& category);

		/**
		* @brief		Sets a grow multiplicator how fast the orthogonal camera window size will increase, so that the game objects remain in view.
		* @param[in]	growMultiplicator	The grow multiplicator to set
		*/
		void setGrowMultiplicator(Ogre::Real growMultiplicator);
	private:
		void calcAveragePosition(void);

		Ogre::Real calcRequiredSize(void);

		void zoomCamera(void);

	protected:
		
		virtual void onSetData(void);

		virtual void onClearData(void);
	protected:
		Ogre::SceneManager* sceneManager;
		Ogre::Real smoothValue;

		Ogre::Vector3 desiredPosition;
		Ogre::Vector3 moveVelocity;
		Ogre::Real zoomSpeed;
		Ogre::Real growMultiplicator;

		Ogre::String category;
		unsigned int categoryId;
		Ogre::ProjectionType oldProjectionType;
		Ogre::Vector2 oldOrthogonalSize;
	};

}; //namespace end

#endif
#ifndef CAMERA_MANAGER_H
#define CAMERA_MANAGER_H

#include "defines.h"
#include "BaseCamera.h"

namespace NOWA
{
	class Ogre::SceneManager;
	class Ogre::Camera;
	class Ogre::SceneNode;

	class EXPORTED CameraManager
	{
	public:
		friend class AppState; // Only AppState may create this class

		/**
		 * @brief		Creates the camera manager
		 * @param[in]	name			The name for the camera manager
		 * @param[in]	camera			The camera to use
		 * @param[in]	moveSpeed		The move speed of the camera
		 * @param[in]	rotateSpeed		The rotate speed of the camera
		 * @note						Do not use the camera itself for position and orientation, after it has been added to this camera manager, but use CameraManager::getPosition()
		 *								and CameraManager::getOrientation()
		 */
		void init(const Ogre::String& name, Ogre::Camera* camera, Ogre::Real moveSpeed = 20.0f, Ogre::Real rotateSpeed = 30.0f);

		void destroyContent(void);

		void setMoveSpeed(Ogre::Real moveSpeed);

		void setRotationSpeed(Ogre::Real rotateSpeed);

		void setSmoothValue(Ogre::Real smoothValue);

		void moveCamera(Ogre::Real dt);

		void rotateCamera(Ogre::Real dt, bool forJoyStick = false);

		Ogre::Vector3 getPosition(void);

		Ogre::Quaternion getOrientation(void);

		unsigned int getCountCameras(void) const;

		/**
		* @brief		Adds the given camera behavior to the Ogre camera.
		* @param[in]	camera			The camera to set.
		* @param[in]	cameraBehavior	The camera behavior to set for the camera.					
		* @note			This function should be called before @addCamera(...).
		*				The cameraBehavior shall be created on the outside on heap via new and will be deleted if @removeCamera is called.
		*/
		void addCameraBehavior(Ogre::Camera* camera, BaseCamera* cameraBehavior);

		/**
		* @brief		Removes the camera behavior from the given key string
		* @param[in]	cameraBehaviorKey	The camera behavior key to remove
		*/
		void removeCameraBehavior(const Ogre::String& cameraBehaviorKey);

		void setActiveCameraBehavior(Ogre::Camera* camera, const Ogre::String& behaviorType);

		BaseCamera* getActiveCameraBehavior(Ogre::Camera* camera) const;

		void addCamera(Ogre::Camera* camera, bool activate, bool forSplitScreen = false);

		void removeCamera(Ogre::Camera* camera);

		void activateCamera(Ogre::Camera* camera);

		Ogre::Camera* getActiveCamera(void) const;

		Ogre::String getName(void) const;

		void setMoveCameraWeight(Ogre::Real moveCameraWeight);

		void setRotateCameraWeight(Ogre::Real rotateCameraWeight);

		unsigned int getCameraBehaviorId(void);

	private:
		CameraManager(const Ogre::String& appStateName);
		~CameraManager();
	private:
		Ogre::String appStateName;
		Ogre::String name;

		Ogre::Real moveSpeed;
		Ogre::Real rotateSpeed;

		struct BehaviorData
		{
			BehaviorData()
				: cameraBehavior(nullptr)
			{

			}

			Ogre::String cameraBehaviorKey;
			BaseCamera* cameraBehavior;
		};

		struct CameraData
		{
			std::vector<BehaviorData> behaviorData;
			bool isActive;
			bool forSplitScreen;

			CameraData()
				: isActive(false),
				forSplitScreen(false)
			{

			}
		};

		std::map<Ogre::Camera*, CameraData> cameraDataMap;
		unsigned int cameraBehaviorId;
	};

}; //namespace end

#endif
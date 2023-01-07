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

		void moveCamera(Ogre::Real dt);

		void rotateCamera(Ogre::Real dt, bool forJoyStick = false);

		Ogre::Vector3 getPosition(void);

		Ogre::Quaternion getOrientation(void);

		// bool attachToSceneNode(Ogre::SceneNode* sceneNode);

		void addCameraBehavior(BaseCamera* baseCamera);

		/**
		* @brief		Removes the camera behavior from the given type string
		* @param[in]	cameraBehaviorType	The camera behavior type to remove
		* @param[in]	destroy				If set to true the behavior will also be destroyed
		* @note			Internally the behavior will be deleted!
		*/

		void removeCameraBehavior(const Ogre::String& cameraBehaviorType, bool destroy = true);

		void setActiveCameraBehavior(const Ogre::String& cameraBehaviorType);

		BaseCamera* getActiveCameraBehavior(void) const;

		void addCamera(Ogre::Camera* camera, bool activate);

		void removeCamera(Ogre::Camera* camera);

		void activateCamera(Ogre::Camera* camera);

		Ogre::Camera* getActiveCamera(void) const;

		Ogre::String getName(void) const { return this->name; }

		void setMoveCameraWeight(Ogre::Real moveCameraWeight);

		void setRotateCameraWeight(Ogre::Real rotateCameraWeight);

	private:
		CameraManager(const Ogre::String& appStateName);
		~CameraManager();
	private:
		Ogre::String appStateName;
		Ogre::Camera* camera;
		Ogre::String name;

		Ogre::Real moveSpeed;
		Ogre::Real rotateSpeed;

		std::map<Ogre::String, BaseCamera*> cameraStrategies;
		Ogre::String cameraBehaviorType;
		Ogre::String oldBehaviorType;
		// camera, active
		std::map<Ogre::Camera*, bool> cameras;
	};

}; //namespace end

#endif
#include "NOWAPrecompiled.h"
#include "CameraManager.h"
#include "gameobject/GameObjectComponent.h"
#include "gameobject/GameObjectController.h"
#include "main/Core.h"
#include "modules/WorkspaceModule.h"
#include "modules/GameProgressModule.h"

#include "NullCamera.h"

// FPS camera with ogrenewt
// http://www.ogre3d.org/addonforums/viewtopic.php?t=2110
// http://www.ogre3d.org/addonforums/viewtopic.php?f=4&t=9838&p=57530&hilit=omega#p57530
// http://www.ogre3d.org/addonforums/viewtopic.php?f=4&t=2110&p=67742&hilit=collision+of+animation#p67742

namespace NOWA
{
	CameraManager::CameraManager(const Ogre::String& appStateName)
		: appStateName(appStateName),
		camera(nullptr),
		moveSpeed(0.0f),
		rotateSpeed(0.0f)
	{

	}

	void CameraManager::init(const Ogre::String& name, Ogre::Camera* camera, Ogre::Real moveSpeed, Ogre::Real rotateSpeed)
	{
		this->name = name;
		this->camera = camera;
		this->camera->setQueryFlags(0);
		moveSpeed = moveSpeed;
		rotateSpeed = rotateSpeed;
		bool activate = true;
		this->cameras.emplace(camera, activate);
		this->addCameraBehavior(new NullCamera());
	}

	CameraManager::~CameraManager()
	{
		this->destroyContent();
	}

	void CameraManager::destroyContent(void)
	{
		this->cameraBehaviorType.clear();

		for (auto& it : this->cameraStrategies)
		{
			BaseCamera* baseCamera = it.second;
			delete baseCamera;
			baseCamera = nullptr;
		}
		this->cameraStrategies.erase(this->cameraStrategies.begin(), this->cameraStrategies.end());
		this->cameraStrategies.clear();
		this->cameras.clear();
		this->camera = nullptr;
		/*while (this->cameraStrategies.size() > 0) {
		BaseCamera* baseCamera = this->cameraStrategies.
		delete baseCamera;
		this->cameraStrategies.pop_back();
		}*/
	}

	//bool CameraManager::attachToSceneNode(Ogre::SceneNode* sceneNode)
	//{
	//	this->sceneNode = sceneNode;
	//	if (nullptr == this->sceneNode)
	//	{
	//		// throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[CameraManager] The to be attached game object component is null.\n", "NOWA");
	//		return false;
	//	}
	//	std::map<Ogre::String, BaseCamera*>::iterator it;
	//	for (it = this->cameraStrategies.begin(); it != this->cameraStrategies.end(); it++) {
	//		it->second->setSceneNode(this->sceneNode);
	//	}
	//	return true;
	//}

	void CameraManager::addCameraBehavior(BaseCamera* baseCamera)
	{
		if (this->cameraBehaviorType != baseCamera->getBehaviorType())
		{
			this->oldBehaviorType = this->cameraBehaviorType;
		}
		this->cameraBehaviorType = baseCamera->getBehaviorType();
		baseCamera->postInitialize(this->camera);
		this->cameraStrategies.insert(std::make_pair(this->cameraBehaviorType, baseCamera));
	}

	void CameraManager::removeCameraBehavior(const Ogre::String& cameraBehaviorType, bool destroy)
	{
		if (cameraBehaviorType == "NO_ACTIVE_CAMERA")
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CameraManager] Default camera type can not be removed.");
			return;
		}
		std::map<Ogre::String, BaseCamera*>::iterator it = this->cameraStrategies.find(cameraBehaviorType);
		if (it != this->cameraStrategies.end())
		{
			it->second->onClearData();

			BaseCamera* baseCamera = it->second;
			if (true == destroy)
			{
				delete baseCamera;
				baseCamera = nullptr;
			}
			this->cameraStrategies.erase(it);

			// Sets the old behavior
			this->setActiveCameraBehavior(this->oldBehaviorType);

			this->cameraBehaviorType = this->oldBehaviorType;
		}
	}

	void CameraManager::setActiveCameraBehavior(const Ogre::String& cameraBehaviorType)
	{
		if (this->cameraBehaviorType != cameraBehaviorType)
		{
			this->oldBehaviorType = cameraBehaviorType;
		}
		this->cameraBehaviorType = cameraBehaviorType;

		std::map<Ogre::String, BaseCamera*>::iterator it;

		it = this->cameraStrategies.find(this->oldBehaviorType);
		if (it != this->cameraStrategies.end())
		{
			it->second->onClearData();
		}

		it = this->cameraStrategies.find(cameraBehaviorType);
		if (it != this->cameraStrategies.end())
		{
			it->second->onSetData();
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CameraManager] Camera with moving behavior type: "
				+ cameraBehaviorType + " does not exist. Add the camera behavior");
		}
	}

	BaseCamera* CameraManager::getActiveCameraBehavior(void) const
	{
		auto& it = this->cameraStrategies.find(this->cameraBehaviorType);
		if (it != this->cameraStrategies.end())
		{
			return it->second;
		}
		return nullptr;
	}

	void CameraManager::addCamera(Ogre::Camera* camera, bool activate)
	{
		bool foundActiveOne = false;
		if (true == activate)
		{
			// First deactivate all, besides the new one
			for (auto it = this->cameras.begin(); it != this->cameras.end(); ++it)
			{
				if (camera == it->first)
				{
					it->second = activate;
					it->first->setVisible(activate);
				}
				else
				{
					it->second = false;
					it->first->setVisible(false);
				}
				// When active, choose another for setting active
				if (true == it->second)
				{
					foundActiveOne = true;
				}
			}
			// Set active camera
			this->camera = camera;
			auto activeBehavior = this->getActiveCameraBehavior();
			if (nullptr != activeBehavior)
			{
				activeBehavior->postInitialize(this->camera);
			}
		}
		this->cameras.emplace(camera, activate);

		if (true == foundActiveOne && false == activate)
		{
			for (auto it = this->cameras.begin(); it != this->cameras.end(); ++it)
			{
				// Set the first one to come active
				it->second = true;
				it->first->setVisible(true);
				auto activeBehavior = this->getActiveCameraBehavior();
				if (nullptr != activeBehavior)
				{
					activeBehavior->postInitialize(this->camera);
				}
				break;
			}
		}
	}

	void CameraManager::removeCamera(Ogre::Camera* camera)
	{
		bool foundActiveOne = false;
		auto it = this->cameras.find(camera);
		if (this->cameras.end() != it)
		{
			// When active, choose another for setting active
			if (true == it->second)
			{
				foundActiveOne = true;
			}
			this->cameras.erase(camera);
			camera->setVisible(false);
		}

		if (true == foundActiveOne)
		{
			for (auto it = this->cameras.begin(); it != this->cameras.end(); ++it)
			{
				// Set the first one to come active
				it->second = true;
				it->first->setVisible(true);
				auto activeBehavior = this->getActiveCameraBehavior();
				if (nullptr != activeBehavior)
				{
					activeBehavior->postInitialize(this->camera);
				}
				break;
			}
		}
	}

	void CameraManager::activateCamera(Ogre::Camera* camera)
	{
		auto it = this->cameras.find(camera);
		if (this->cameras.end() != it)
		{
			// First deactivate all
			for (auto subIt = this->cameras.begin(); subIt != this->cameras.end(); ++subIt)
			{
				subIt->second = false;
				subIt->first->setVisible(false);
			}
	
			// Set active
			it->second = true;
			this->camera = it->first;
			this->camera->setVisible(true);
			auto activeBehavior = this->getActiveCameraBehavior();
			if (nullptr != activeBehavior)
			{
				activeBehavior->postInitialize(this->camera);
			}
		}
	}

	Ogre::Camera* CameraManager::getActiveCamera(void) const
	{
		for (auto it = this->cameras.begin(); it != this->cameras.end(); ++it)
		{
			if (true == it->second)
			{
				return it->first;
			}
		}
		return nullptr;
	}

	void CameraManager::setMoveCameraWeight(Ogre::Real moveCameraWeight)
	{
		this->cameraStrategies[this->cameraBehaviorType]->moveCameraWeight = moveCameraWeight;
	}

	void CameraManager::setRotateCameraWeight(Ogre::Real rotateCameraWeight)
	{
		this->cameraStrategies[this->cameraBehaviorType]->rotateCameraWeight = rotateCameraWeight;
	}

	void CameraManager::moveCamera(Ogre::Real dt)
	{
		if (nullptr != this->camera)
		{
			Ogre::Vector3 cameraPosition = this->camera->getPosition();
			this->cameraStrategies[this->cameraBehaviorType]->moveCamera(dt);
		}
	}

	void CameraManager::rotateCamera(Ogre::Real dt, bool forJoyStick)
	{
		if (nullptr != this->camera)
		{
			Ogre::Vector3 cameraPosition = this->camera->getPosition();
			Ogre::Quaternion cameraOrientation = this->camera->getOrientation();
			this->cameraStrategies[this->cameraBehaviorType]->rotateCamera(dt, forJoyStick);
		}
	}

	Ogre::Vector3 CameraManager::getPosition(void)
	{
		return this->cameraStrategies[this->cameraBehaviorType]->getPosition();
	}

	Ogre::Quaternion CameraManager::getOrientation(void)
	{
		return this->cameraStrategies[this->cameraBehaviorType]->getOrientation();
	}

}; //namespace end

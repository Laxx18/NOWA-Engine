#include "NOWAPrecompiled.h"
#include "CameraManager.h"
#include "main/AppStateManager.h"

#include "NullCamera.h"

// FPS camera with ogrenewt
// http://www.ogre3d.org/addonforums/viewtopic.php?t=2110
// http://www.ogre3d.org/addonforums/viewtopic.php?f=4&t=9838&p=57530&hilit=omega#p57530
// http://www.ogre3d.org/addonforums/viewtopic.php?f=4&t=2110&p=67742&hilit=collision+of+animation#p67742

namespace NOWA
{
	CameraManager::CameraManager(const Ogre::String& appStateName)
		: appStateName(appStateName),
		moveSpeed(0.0f),
		rotateSpeed(0.0f),
		cameraBehaviorId(0)
	{

	}

	void CameraManager::init(const Ogre::String& name, Ogre::Camera* camera, Ogre::Real moveSpeed, Ogre::Real rotateSpeed)
	{
		this->name = name;
		this->moveSpeed = moveSpeed;
		this->rotateSpeed = rotateSpeed;
		this->addCameraBehavior(camera, new NullCamera(this->cameraBehaviorId++));
	}

	CameraManager::~CameraManager()
	{
		this->destroyContent();
	}

	void CameraManager::destroyContent(void)
	{
		for (auto& entry : this->cameraDataMap)
		{
			for (auto& behavior : entry.second.behaviorData)
			{
				if (nullptr != behavior.cameraBehavior)
				{
					delete behavior.cameraBehavior;
					behavior.cameraBehavior = nullptr;
				}
			}
		}
		this->cameraDataMap.clear();
	}

	void CameraManager::setMoveSpeed(Ogre::Real moveSpeed)
	{
		for (auto& cameraPair : this->cameraDataMap)
		{
			cameraPair.second.behaviorData[0].cameraBehavior->setMoveSpeed(moveSpeed);
		}
	}

	void CameraManager::setRotationSpeed(Ogre::Real rotateSpeed)
	{
		for (auto& cameraPair : this->cameraDataMap)
		{
			cameraPair.second.behaviorData[0].cameraBehavior->setRotationSpeed(rotateSpeed);
		}
	}

	void CameraManager::setSmoothValue(Ogre::Real smoothValue)
	{
		for (auto& cameraPair : this->cameraDataMap)
		{
			cameraPair.second.behaviorData[0].cameraBehavior->setSmoothValue(smoothValue);
		}
	}

	unsigned int CameraManager::getCountCameras(void) const
	{
		return this->cameraDataMap.size();
	}

	void CameraManager::removeCameraBehavior(const Ogre::String& cameraBehaviorType)
	{
		size_t found = cameraBehaviorType.find(NullCamera::BehaviorType());
		if (found != Ogre::String::npos)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CameraManager] Default camera type cannot be removed.");
			return;
		}

		// Iterate through the cameraDataMap to find and remove the camera behavior
		for (auto mainIt = this->cameraDataMap.begin(); mainIt != this->cameraDataMap.end();)
		{
			CameraData& cameraData = mainIt->second;
			bool noBehaviorLeft = false;

			for (auto it = mainIt->second.behaviorData.begin(); it != mainIt->second.behaviorData.end(); ++it)
			{
				if (it->cameraBehaviorKey == cameraBehaviorType)
				{
					auto cameraBehavior = it->cameraBehavior;
					// Clear the behavior data
					cameraBehavior->onClearData();

					boost::shared_ptr<EventDataRemoveCameraBehavior> eventDataRemoveCamera(new EventDataRemoveCameraBehavior(mainIt->first));
					AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataRemoveCamera);

					delete cameraBehavior;

					// Clear the camera behavior key
					it->cameraBehaviorKey.clear();
					mainIt->second.behaviorData.erase(it);

					// If the removed behavior was the current active one, attempt to restore another behavior
					if (true == cameraData.isActive)
					{
						if (false == mainIt->second.behaviorData.empty())
						{
							auto otherBehavior = mainIt->second.behaviorData.begin();
							// Use setActiveCameraBehavior to set the new behavior
							this->setActiveCameraBehavior(mainIt->first, mainIt->second.behaviorData.begin()->cameraBehaviorKey);
						}
						else
						{
							noBehaviorLeft = true;
						}
					}
					break;
				}
			}

			if (false == noBehaviorLeft)
			{
				++mainIt;
			}
			else
			{
				mainIt = this->cameraDataMap.erase(mainIt);
			}
		}
	}

	void CameraManager::setActiveCameraBehavior(Ogre::Camera* camera, const Ogre::String& cameraBehaviorType)
	{
		Ogre::String cameraName = camera->getName();
		auto& cameraData = this->cameraDataMap[camera];

		// Clear data for the old behavior
		for (auto it = cameraData.behaviorData.begin(); it != cameraData.behaviorData.end(); ++it)
		{
			if (it->cameraBehaviorKey != cameraBehaviorType)
			{
				it->cameraBehavior->onClearData();
				break;
			}
		}

		// Find the new behavior in the list and move it to the front
		for (auto it = cameraData.behaviorData.begin(); it != cameraData.behaviorData.end(); ++it)
		{
			if (it->cameraBehaviorKey == cameraBehaviorType)
			{
				if (it != cameraData.behaviorData.begin())
				{
					std::swap(*it, cameraData.behaviorData.front());
				}
				cameraData.behaviorData.begin()->cameraBehavior = cameraData.behaviorData.front().cameraBehavior;
				cameraData.behaviorData.begin()->cameraBehavior->onSetData();
				cameraData.isActive = true;
				break;
			}
		}

		// If the new behavior was not found in the list, log an error
		if (cameraData.behaviorData.begin()->cameraBehavior->getBehaviorType() != cameraBehaviorType)
		{
			cameraData.behaviorData.begin()->cameraBehaviorKey = cameraBehaviorType;
			cameraData.behaviorData.begin()->cameraBehavior->onSetData();
			cameraData.isActive = true;
		}
	}

	BaseCamera* CameraManager::getActiveCameraBehavior(Ogre::Camera* camera) const
	{
		Ogre::String cameraName = camera->getName();
		auto it = this->cameraDataMap.find(camera);
		if (it != this->cameraDataMap.end())
		{
			if (false == it->second.behaviorData.empty())
			{
				return it->second.behaviorData.begin()->cameraBehavior;
			}
		}
		return nullptr;
	}

	void CameraManager::addCamera(Ogre::Camera* camera, bool activate, bool forSplitScreen)
	{
		bool foundActiveOne = false;

		Ogre::String cameraName = camera->getName();
		// Retrieve the camera data for the given camera from the map
		CameraData& cameraData = this->cameraDataMap[camera];
		cameraData.isActive = activate;
		cameraData.forSplitScreen = forSplitScreen;
		
		// If activating, we need to deactivate all other cameras first
		if (true == activate)
		{
			// Deactivate all cameras except for the one being activated
			for (auto& entry : this->cameraDataMap)
			{
				if (entry.first != camera && false == entry.second.forSplitScreen)
				{
					entry.second.isActive = false;
					entry.first->setVisible(false);

					for (auto it = entry.second.behaviorData.begin(); it != entry.second.behaviorData.end(); ++it)
					{
						it->cameraBehavior->onClearData();
					}
				}
			}

			auto activeBehavior = this->getActiveCameraBehavior(camera);
			if (nullptr != activeBehavior)
			{
				// Ensure the active behavior is at the front of the list
				for (auto it = cameraData.behaviorData.begin(); it != cameraData.behaviorData.end(); ++it)
				{
					if (it != cameraData.behaviorData.begin())
					{
						std::swap(*it, cameraData.behaviorData.front());
					}
					break;
				}

				// Now set this camera as active
				camera->setVisible(true);
				cameraData.behaviorData.begin()->cameraBehavior->postInitialize(camera);
				cameraData.behaviorData.begin()->cameraBehavior->onSetData();
			}
		}
		else
		{
			// If deactivating, just hide the camera and clear its data
			camera->setVisible(false);
			cameraData.behaviorData.begin()->cameraBehavior->onClearData();
		}

		// Add the camera to the map (this ensures the camera is part of the map, even if inactive)
		this->cameraDataMap[camera] = cameraData;

		// If we're deactivating and there was another active camera, find the next one to activate
		if (!activate)
		{
			for (auto& entry : this->cameraDataMap)
			{
				if (true == entry.second.isActive)
				{
					foundActiveOne = true;
					break;
				}
			}

			// If no active camera was found, activate the first camera in the map
			if (false == foundActiveOne && false == this->cameraDataMap.empty())
			{
				auto firstCamera = this->cameraDataMap.begin()->first;
				CameraData& firstCameraData = this->cameraDataMap[firstCamera];
				firstCameraData.isActive = true;
				firstCamera->setVisible(true);
				firstCameraData.behaviorData.begin()->cameraBehavior->postInitialize(firstCamera);
				firstCameraData.behaviorData.begin()->cameraBehavior->onSetData();
			}
		}
	}

	void CameraManager::addCameraBehavior(Ogre::Camera* camera, BaseCamera* baseCamera)
	{
		Ogre::String cameraName = camera->getName();
		auto& cameraData = this->cameraDataMap[camera];
		bool behaviorExists = false;

		// Check if the behavior already exists and move it to the beginning if found
		for (auto it = cameraData.behaviorData.begin(); it != cameraData.behaviorData.end(); ++it)
		{
			if (it->cameraBehaviorKey == baseCamera->getBehaviorType())
			{
				behaviorExists = true;
				if (it != cameraData.behaviorData.begin())
				{
					std::swap(*it, cameraData.behaviorData.front());
				}
				break;
			}
		}

		// If the behavior does not exist, add it to the front
		if (false == behaviorExists)
		{
			BehaviorData newBehavior;
			newBehavior.cameraBehaviorKey = baseCamera->getBehaviorType();
			newBehavior.cameraBehavior = baseCamera;
			cameraData.behaviorData.insert(cameraData.behaviorData.begin(), newBehavior);
		}

		// Set the camera behavior to the first item in the list and initialize it
		auto& firstBehavior = cameraData.behaviorData.front();
		cameraData.behaviorData.begin()->cameraBehaviorKey = firstBehavior.cameraBehaviorKey;
		cameraData.behaviorData.begin()->cameraBehavior = firstBehavior.cameraBehavior;
		firstBehavior.cameraBehavior->postInitialize(camera);
	}

	void CameraManager::removeCamera(Ogre::Camera* camera)
	{
		Ogre::String cameraName = camera->getName();
		auto it = this->cameraDataMap.find(camera);
		if (it != this->cameraDataMap.end())
		{
			// Get the camera's data
			CameraData& cameraData = it->second;

			// If the camera to be removed is currently active, find another one to activate
			if (true == cameraData.isActive)
			{
				bool foundActiveOne = false;

				// Deactivate the current camera and remove it from the map
				for (auto& behavior : cameraData.behaviorData)
				{
					Ogre::String cameraName = it->first->getName();
					boost::shared_ptr<EventDataRemoveCameraBehavior> eventDataRemoveCamera(new EventDataRemoveCameraBehavior(it->first));
					AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataRemoveCamera);

					behavior.cameraBehavior->onClearData();
					delete behavior.cameraBehavior;
				}
				cameraData.behaviorData.clear();

				this->cameraDataMap.erase(it);
				camera->setVisible(false);

				// Check if there is another active camera in the map
				for (auto& entry : this->cameraDataMap)
				{
					if (true == entry.second.isActive)
					{
						foundActiveOne = true;
						break;
					}
				}

				// If no active camera exists, activate the first camera in the map
				if (!foundActiveOne && !this->cameraDataMap.empty())
				{
					auto firstCamera = this->cameraDataMap.begin()->first;
					CameraData& firstCameraData = this->cameraDataMap[firstCamera];
					firstCameraData.isActive = true;
					firstCamera->setVisible(true);
					firstCameraData.behaviorData.begin()->cameraBehavior->postInitialize(firstCamera);
					firstCameraData.behaviorData.begin()->cameraBehavior->onSetData();
				}
			}
			else
			{
				// If the camera to be removed is not active, just remove it
				for (auto& behavior : cameraData.behaviorData)
				{
					Ogre::String cameraName = it->first->getName();
					boost::shared_ptr<EventDataRemoveCameraBehavior> eventDataRemoveCamera(new EventDataRemoveCameraBehavior(it->first));
					AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataRemoveCamera);

					behavior.cameraBehavior->onClearData();
					delete behavior.cameraBehavior;
				}
				cameraData.behaviorData.clear();

				this->cameraDataMap.erase(it);
				camera->setVisible(false);
			}
		}
	}

	void CameraManager::activateCamera(Ogre::Camera* camera)
	{
		auto it = this->cameraDataMap.find(camera);
		if (it != this->cameraDataMap.end())
		{
			// Deactivate all cameras first
			for (auto& entry : this->cameraDataMap)
			{
				if (false == entry.second.forSplitScreen)
				{
					// Set all cameras as inactive
					entry.second.isActive = false;
					entry.first->setVisible(false);

					entry.second.behaviorData.begin()->cameraBehavior->onClearData();
				}
			}

			// Now activate the specified camera
			CameraData& cameraData = it->second;
			cameraData.isActive = true;
			camera->setVisible(true);

			// Ensure the first behavior is set and initialized
			if (!cameraData.behaviorData.empty())
			{
				auto& firstBehavior = cameraData.behaviorData.front();
				cameraData.behaviorData.begin()->cameraBehaviorKey = firstBehavior.cameraBehaviorKey;
				cameraData.behaviorData.begin()->cameraBehavior = firstBehavior.cameraBehavior;
				firstBehavior.cameraBehavior->postInitialize(camera);
				firstBehavior.cameraBehavior->onSetData();
			}
		}
	}

	Ogre::Camera* CameraManager::getActiveCamera(void) const
	{
		// Iterates through all cameras in the cameraDataMap
		for (const auto& entry : this->cameraDataMap)
		{
			// Check if the camera is active
			if (true == entry.second.isActive && false == entry.second.forSplitScreen)
			{
				Ogre::String cameraName = entry.first->getName();
				return entry.first;
			}
		}
		return nullptr;
	}

	Ogre::String CameraManager::getName(void) const
	{
		return this->name;
	}

	void CameraManager::setMoveCameraWeight(Ogre::Real moveCameraWeight)
	{
		// Sets moveCameraWeight for all active cameras
		for (auto& cameraPair : this->cameraDataMap)
		{
			if (true == cameraPair.second.isActive) // Check if the camera is active
			{
				cameraPair.second.behaviorData.begin()->cameraBehavior->moveCameraWeight = moveCameraWeight;
			}
		}
	}

	void CameraManager::setRotateCameraWeight(Ogre::Real rotateCameraWeight)
	{
		// Set rotateCameraWeight for all active cameras
		for (auto& cameraPair : this->cameraDataMap)
		{
			// Checks if the camera is active
			if (true == cameraPair.second.isActive)
			{
				cameraPair.second.behaviorData.begin()->cameraBehavior->rotateCameraWeight = rotateCameraWeight;
			}
		}
	}

	unsigned int CameraManager::getCameraBehaviorId(void)
	{
		return this->cameraBehaviorId++;
	}

	void CameraManager::moveCamera(Ogre::Real dt)
	{
		// Moves all active cameras using their camera behaviors
		for (auto& cameraPair : this->cameraDataMap)
		{
			// Checks if the camera is active
			if (true == cameraPair.second.isActive)
			{
				// Moves the camera using its behavior
				cameraPair.second.behaviorData.begin()->cameraBehavior->moveCamera(dt);
			}
		}
	}

	void CameraManager::rotateCamera(Ogre::Real dt, bool forJoyStick)
	{
		// Rotates all active cameras using their camera behaviors
		for (auto& cameraPair : this->cameraDataMap)
		{
			if (true == cameraPair.second.isActive) // Check if the camera is active
			{
				// Rotates the camera using its behavior
				cameraPair.second.behaviorData.begin()->cameraBehavior->rotateCamera(dt, forJoyStick);
			}
		}
	}

	Ogre::Vector3 CameraManager::getPosition(void)
	{
		// Returns the position of the first active camera (if any)
		for (auto& cameraPair : this->cameraDataMap)
		{
			if (true == cameraPair.second.isActive) // Check if the camera is active
			{
				return cameraPair.second.behaviorData.begin()->cameraBehavior->getPosition();
			}
		}
		return Ogre::Vector3::ZERO;
	}

	Ogre::Quaternion CameraManager::getOrientation(void)
	{
		// Returns the orientation of the first active camera (if any)
		for (auto& cameraPair : this->cameraDataMap)
		{
			if (true == cameraPair.second.isActive) // Check if the camera is active
			{
				return cameraPair.second.behaviorData.begin()->cameraBehavior->getOrientation();
			}
		}
		return Ogre::Quaternion::IDENTITY;
	}

}; //namespace end

#include "NOWAPrecompiled.h"
#include "CameraManager.h"
#include "main/AppStateManager.h"
#include "utilities/MathHelper.h"

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
		return static_cast<unsigned int>(this->cameraDataMap.size());
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
        if (cameraData.behaviorData.empty())
        {
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[CameraManager] setActiveCameraBehavior: no behaviors registered for camera: " + cameraName);
            return;
        }

		// If the new behavior was not found in the list, log an error
        if (cameraData.behaviorData.front().cameraBehavior->getBehaviorType() != cameraBehaviorType)
        {
            cameraData.behaviorData.front().cameraBehaviorKey = cameraBehaviorType;
            cameraData.behaviorData.front().cameraBehavior->onSetData();
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
                // Really bad case: Camera added, but no behavior is assigned. That is: No CameraBehaviorComponent points via camera gameobject id to this camera. So the camera will be illegal.
                //
                // ATTENTION: This check must NEVER fire for (a) the camera THIS call is currently activating
                // (entry.first == camera) - it may legitimately not have its behavior wired up yet at this exact
                // point in the call chain, this very call (or the code that called it) is what is setting it up
                // right now - and (b) any camera flagged forSplitScreen. Split screen cameras registered via
                // CameraManager::registerSplitScreenCamera are legitimately allowed to have permanently empty
                // behaviorData (a purely statically placed split screen camera with no CameraBehaviorComponent
                // at all is an explicitly supported case). Without these two exclusions, this check erroneously
                // erased a split screen camera's own freshly registered map entry and aborted this entire
                // addCamera() call before it could set forSplitScreen/isActive correctly - which is exactly what
                // caused split screen mouse-to-camera lookups to keep resolving to the wrong camera.
                if (entry.first != camera && false == entry.second.forSplitScreen && true == entry.second.behaviorData.empty())
                {
                    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL,
                        "[CameraManager] Error in addCamera: no behaviors registered for camera: " + entry.first->getName() + "That is: No CameraBehaviorComponent points via camera gameobject id to this camera. So the camera will be illegal.");
                    this->removeCamera(entry.first);
                    return;
                }

                if (entry.first != camera && false == entry.second.forSplitScreen)
                {
                    entry.second.isActive = false;
                    NOWA::GraphicsModule::RenderCommand renderCommand = [this, entry]()
                    {
                        entry.first->setVisible(false);
                    };
                    NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "CameraManager::addCamera1");

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

                NOWA::GraphicsModule::RenderCommand renderCommand = [this, camera]()
                {
                    // Now set this camera as active
                    camera->setVisible(true);
                };
                NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "CameraManager::addCamera2");

                cameraData.behaviorData.begin()->cameraBehavior->postInitialize(camera);
                cameraData.behaviorData.begin()->cameraBehavior->onSetData();
            }
        }
        else
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this, camera]()
            {
                // If deactivating, just hide the camera and clear its data
                camera->setVisible(false);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "CameraManager::addCamera3");

            // ATTENTION: guarded against empty behaviorData - a split screen camera with no
            // CameraBehaviorComponent (purely statically placed) has an empty behaviorData vector, and calling
            // .begin() on that would be undefined behavior / a crash. Previously unguarded, this was never hit
            // because every addCamera(camera, false, ...) call in the codebase so far happened to target a
            // camera that already had a behavior - but it is not a safe assumption to keep relying on.
            if (false == cameraData.behaviorData.empty())
            {
                cameraData.behaviorData.begin()->cameraBehavior->onClearData();
            }
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

                NOWA::GraphicsModule::RenderCommand renderCommand = [this, firstCamera]()
                {
                    firstCamera->setVisible(true);
                };
                NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "CameraManager::addCamera4");

                // ATTENTION: same guard as above - firstCameraData may be a split screen camera without any
                // registered behavior.
                if (false == firstCameraData.behaviorData.empty())
                {
                    firstCameraData.behaviorData.begin()->cameraBehavior->postInitialize(firstCamera);
                    firstCameraData.behaviorData.begin()->cameraBehavior->onSetData();
                }
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
		auto firstBehavior = cameraData.behaviorData.front();
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
					AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRemoveCamera);

					behavior.cameraBehavior->onClearData();
					delete behavior.cameraBehavior;
				}
				cameraData.behaviorData.clear();

				this->cameraDataMap.erase(it);
				
				NOWA::GraphicsModule::RenderCommand renderCommand = [this, camera]()
                {
                    camera->setVisible(false);
                };
                NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "CameraManager::removeCamera");

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

					NOWA::GraphicsModule::RenderCommand renderCommand = [this, firstCamera]()
                    {
                        firstCamera->setVisible(true);
                    };
                    NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "CameraManager::removeCamera2");

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
					AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRemoveCamera);

					behavior.cameraBehavior->onClearData();
					delete behavior.cameraBehavior;
				}
				cameraData.behaviorData.clear();

				this->cameraDataMap.erase(it);

				NOWA::GraphicsModule::RenderCommand renderCommand = [this, camera]()
                {
                    camera->setVisible(false);
                };
                NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "CameraManager::removeCamera3");
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

					NOWA::GraphicsModule::RenderCommand renderCommand = [this, camera]()
                    {
                        camera->setVisible(false);
                    };
                    NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "CameraManager::activateCamera");

					entry.second.behaviorData.begin()->cameraBehavior->onClearData();
				}
			}

			// Now activate the specified camera
			CameraData& cameraData = it->second;
			cameraData.isActive = true;

			NOWA::GraphicsModule::RenderCommand renderCommand = [this, camera]()
            {
                camera->setVisible(true);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "CameraManager::activateCamera2");

			// Ensure the first behavior is set and initialized
			if (!cameraData.behaviorData.empty())
			{
				auto firstBehavior = cameraData.behaviorData.front();
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
        for (const auto entry : this->cameraDataMap)
        {
            // Check if the camera is active
            if (true == entry.second.isActive && false == entry.second.forSplitScreen)
            {
                return entry.first;
            }
        }

        // ATTENTION: getActiveCamera() must NEVER return nullptr as long as at least one non-split-screen
        // camera is registered - callers throughout the engine assume a valid camera and do not null-check the
        // result. If no camera is currently flagged active (should not normally happen, but can occur during
        // scene teardown/reload ordering), fall back to the first non-split-screen camera found in the map,
        // mark it active, and bring it up exactly like the equivalent fallback paths in addCamera()/
        // removeCamera() already do (visibility via render thread, behavior postInitialize/onSetData).
        //
        // const_cast is used deliberately here: this method stays const from the caller's point of view (it
        // only ever "discovers and repairs" missing internal state, never changes what the caller asked for),
        // but cameraDataMap itself must be mutated to record the new active camera.
        for (auto& entry : const_cast<CameraManager*>(this)->cameraDataMap)
        {
            if (false == entry.second.forSplitScreen)
            {
                entry.second.isActive = true;

                Ogre::Camera* fallbackCamera = entry.first;

                NOWA::GraphicsModule::RenderCommand renderCommand = [fallbackCamera]()
                {
                    fallbackCamera->setVisible(true);
                };
                NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "CameraManager::getActiveCamera_Fallback");

                if (false == entry.second.behaviorData.empty())
                {
                    entry.second.behaviorData.begin()->cameraBehavior->postInitialize(fallbackCamera);
                    entry.second.behaviorData.begin()->cameraBehavior->onSetData();
                }

                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[CameraManager] getActiveCamera: no active camera was flagged, activating fallback camera: " + fallbackCamera->getName());

                return fallbackCamera;
            }
        }

        // Truly no camera at all is registered (map empty, or only split-screen cameras exist) - there is
        // nothing left to fabricate a valid Ogre::Camera* from, so nullptr is unavoidable in this one case.
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

    void CameraManager::registerSplitScreenCamera(Ogre::Camera* camera, const Ogre::Vector4& geometry)
    {
        // Deliberately independent of addCamera(): a split screen camera must be findable for screen-space
        // lookups (mouse position -> camera) regardless of whether it has an optional CameraBehaviorComponent
        // attached. addCamera() only registers a camera when a behavior is set (SplitScreenComponent calls it
        // exclusively inside the "0 != cameraBehaviorGameObjectId" branch), so a purely statically placed split
        // screen camera would otherwise never appear in cameraDataMap at all.
        //
        // This function only touches forSplitScreen and splitScreenGeometry. It never touches isActive or
        // behaviorData, so calling it does not interfere with addCamera()/removeCamera() bookkeeping for
        // cameras that DO have a behavior - both paths can run for the same camera without conflict.
        CameraData& cameraData = this->cameraDataMap[camera];
        cameraData.forSplitScreen = true;
        cameraData.splitScreenGeometry = geometry;
    }

    Ogre::Camera* CameraManager::getCameraForScreenPosition(int mouseX, int mouseY, Ogre::Window* renderWindow) const
    {
        bool anySplitScreenCamera = false;

        Ogre::Real normalizedX = 0.0f;
        Ogre::Real normalizedY = 0.0f;

        // Same normalization MathHelper uses to feed Camera::getCameraToViewportRay, so this lookup uses the
        // exact same screen-space convention as everything else that turns a mouse position into a world ray.
        MathHelper::getInstance()->mouseToViewPort(mouseX, mouseY, normalizedX, normalizedY, renderWindow);

        for (const auto& entry : this->cameraDataMap)
        {
            if (true == entry.second.forSplitScreen)
            {
                anySplitScreenCamera = true;

                const Ogre::Vector4& geometry = entry.second.splitScreenGeometry;

                Ogre::Real left = geometry.x;
                Ogre::Real top = geometry.y;
                Ogre::Real right = geometry.x + geometry.z;
                Ogre::Real bottom = geometry.y + geometry.w;

                if (normalizedX >= left && normalizedX < right && normalizedY >= top && normalizedY < bottom)
                {
                    return entry.first;
                }
            }
        }

        if (true == anySplitScreenCamera)
        {
            // Split screen cameras exist, but the position matched none of their tiles (e.g. exact border
            // rounding, or the split screen scenario just ended this very frame) - fall back to the active
            // camera instead of returning nullptr, so every existing single-camera raycast callsite keeps
            // working unchanged.
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CameraManager] getCameraForScreenPosition: position did not match any split screen tile, falling back to the active camera.");
        }

        return this->getActiveCamera();
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

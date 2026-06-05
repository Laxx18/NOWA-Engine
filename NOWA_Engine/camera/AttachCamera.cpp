#include "NOWAPrecompiled.h"
#include "AttachCamera.h"
#include "gameobject/GameObjectComponent.h"
#include "utilities/MathHelper.h"
#include "main/Core.h"
#include "modules/GraphicsModule.h"

namespace NOWA
{
	AttachCamera::AttachCamera(unsigned int id, Ogre::SceneNode* sceneNode,
		const Ogre::Vector3& offsetPosition, const Ogre::Quaternion& offsetOrientation, Ogre::Real smoothValue, const Ogre::Vector3& defaultDirection)
		: BaseCamera(id, 0.0f, 0.0f, smoothValue, defaultDirection),
		sceneNode(sceneNode),
		offsetPosition(offsetPosition),
		offsetOrientation(offsetOrientation)
	{
		// Smoothing creates jitter if simulation updates are 30 ticks per second or below, hence disable smoothing
		if (Core::getSingletonPtr()->getOptionDesiredSimulationUpdates() <= 30)
		{
			this->smoothValue = 0.0f;
		}
	}

	AttachCamera::~AttachCamera()
	{
		this->sceneNode = nullptr;
	}

	void AttachCamera::onSetData(void)
	{
		BaseCamera::onSetData();

		this->firstTimeMoveValueSet = true;
		if (this->sceneNode)
		{
			ENQUEUE_RENDER_COMMAND("AttachCamera::onSetData",
			{
				this->camera->setPosition(this->sceneNode->_getDerivedPositionUpdated());
				this->camera->setOrientation(this->sceneNode->_getDerivedOrientationUpdated());
			});
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ThirdPersonCamera]: Warning cannot use the game object component for init positioning of the camera, since it is NULL.");
		}
	}

	void AttachCamera::setSceneNode(Ogre::SceneNode * sceneNode)
	{
		this->sceneNode = sceneNode;
	}

	//void AttachCamera::moveCamera(Ogre::Real dt)
 //   {
 //       if (nullptr == this->sceneNode)
 //       {
 //           return;
 //       }

 //       // Read position/orientation from the physics body directly — NOT from
 //       // the SceneNode. The SceneNode is updated by the render thread one frame
 //       // later. At high speed this lag makes the camera visibly chase the ship.
 //       Ogre::Vector3 targetPosition;
 //       Ogre::Quaternion targetOrientation;

 //       if (nullptr != this->physicsBody)
 //       {
 //           // m_curPosit/m_curRotation are written by OnTransform on the Newton
 //           // worker thread, but Newton has already Sync()'d before we get here,
 //           // so these values are safe to read on the logic thread.
 //           targetPosition = this->physicsBody->getPosition();
 //           targetOrientation = this->physicsBody->getOrientation();
 //       }
 //       else
 //       {
 //           targetPosition = this->sceneNode->_getDerivedPositionUpdated();
 //           targetOrientation = this->sceneNode->_getDerivedOrientationUpdated();
 //       }

 //       Ogre::Quaternion camOrientation = MathHelper::getInstance()->lookAt((targetOrientation * this->offsetOrientation) * this->defaultDirection, Ogre::Vector3::UNIT_Y);

 //       Ogre::Vector3 targetVector = targetPosition + (camOrientation * this->offsetPosition * this->moveCameraWeight);

 //       targetVector = (targetVector * this->smoothValue) + (this->camera->getPosition() * (1.0f - this->smoothValue));

 //       NOWA::GraphicsModule::getInstance()->updateCameraOrientation(this->camera, camOrientation);
 //       NOWA::GraphicsModule::getInstance()->updateCameraPosition(this->camera, targetVector);
 //   }

	void AttachCamera::moveCamera(Ogre::Real dt)
    {
        if (nullptr == this->sceneNode)
        {
            return;
        }

        Ogre::Vector3 targetPosition = this->sceneNode->_getDerivedPositionUpdated();
        Ogre::Quaternion targetOrientation = this->sceneNode->_getDerivedOrientationUpdated();

        Ogre::Quaternion camOrientation = MathHelper::getInstance()->lookAt((targetOrientation * this->offsetOrientation) * this->defaultDirection, Ogre::Vector3::UNIT_Y);

        Ogre::Vector3 desiredPos = targetPosition + (camOrientation * this->offsetPosition * this->moveCameraWeight);

        // Do NOT blend with camera->getPosition() here.
        // camera->getPosition() is written by updateAllTransforms() on the render
        // thread at ~519 FPS and oscillates with the interpolation alpha — using
        // it as a smoothing base produces the ghost-snap jumps you see.
        //
        // The interpolation buffer already provides sub-physics-step smoothness
        // at render frequency by lerping between the previous and current logic
        // snapshots. No additional position smoothing is needed or correct here.
        //
        // If you want lag/spring feel, it must be done against a logic-thread-owned
        // value (lastMoveValue is available in BaseCamera for exactly this).

        if (true == this->firstTimeMoveValueSet)
        {
            this->lastMoveValue = desiredPos;
            this->firstTimeMoveValueSet = false;
        }

        Ogre::Vector3 smoothedPos;
        if (this->smoothValue > 0.0f)
        {
            // Exponential smooth against lastMoveValue — a logic-thread-owned
            // value that advances exactly once per physics step. No render noise.
            const float ks = 1.0f - std::exp(-this->smoothValue/* * dt * 144.0f*/);
            smoothedPos = this->lastMoveValue + (desiredPos - this->lastMoveValue) * ks;
        }
        else
        {
            smoothedPos = desiredPos;
        }

        this->lastMoveValue = smoothedPos;



        NOWA::GraphicsModule::getInstance()->updateCameraOrientation(this->camera, camOrientation);
        NOWA::GraphicsModule::getInstance()->updateCameraPosition(this->camera, smoothedPos);

        // Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
         //    "[AttachCamera] dt=" + Ogre::StringConverter::toString(dt) + " smoothedPos=" + Ogre::StringConverter::toString(smoothedPos) + " camOrientation=" + Ogre::StringConverter::toString(camOrientation));
    }

	Ogre::Vector3 AttachCamera::getPosition(void)
	{
		return this->camera->getPosition();
	}

	Ogre::Quaternion AttachCamera::getOrientation(void)
	{
		return this->camera->getOrientation();
	}

}; //namespace end

#include "NOWAPrecompiled.h"
#include "AttachCamera.h"
#include "gameobject/GameObjectComponent.h"
#include "utilities/MathHelper.h"
#include "main/Core.h"

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
			this->camera->setPosition(this->sceneNode->_getDerivedPositionUpdated());
			this->camera->setOrientation(this->sceneNode->_getDerivedOrientationUpdated());
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

	void AttachCamera::moveCamera(Ogre::Real dt)
	{
		if (nullptr != this->sceneNode)
		{
			Ogre::Vector3 targetPosition = this->sceneNode->_getDerivedPositionUpdated();

			Ogre::Quaternion targetOrientation = MathHelper::getInstance()->lookAt((this->sceneNode->_getDerivedOrientationUpdated() * this->offsetOrientation) * this->defaultDirection, Ogre::Vector3::UNIT_Y);

			this->camera->setOrientation(targetOrientation);

			Ogre::Vector3 targetVector = targetPosition + (targetOrientation * this->offsetPosition * this->moveCameraWeight);
			targetVector = (targetVector * this->smoothValue) + (this->camera->getPosition() * (1.0f - this->smoothValue));
			this->camera->setPosition(targetVector);

			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AttachCamera]:Pos: " + Ogre::StringConverter::toString(this->camera->getPosition()));
		}
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

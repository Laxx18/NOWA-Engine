#include "NOWAPrecompiled.h"
#include "FirstPersonCamera.h"
#include "gameobject/GameObjectComponent.h"
#include "utilities/MathHelper.h"
#include "main/Core.h"

namespace NOWA
{
	FirstPersonCamera::FirstPersonCamera(unsigned int id, Ogre::SceneNode* sceneNode, const Ogre::Vector3& defaultDirection, Ogre::Real smoothValue,
		Ogre::Real rotateSpeed, const Ogre::Vector3& offsetPosition)
		: BaseCamera(id, 0.0f, rotateSpeed, smoothValue, defaultDirection),
		sceneNode(sceneNode),
		offsetPosition(offsetPosition)
	{
		// Smoothing creates jitter if simulation updates are 30 ticks per second or below, hence disable smoothing
		if (Core::getSingletonPtr()->getOptionDesiredSimulationUpdates() <= 30)
		{
			this->smoothValue = 0.0f;
		}
	}

	FirstPersonCamera::~FirstPersonCamera()
	{
		this->sceneNode = nullptr;
	}

	void FirstPersonCamera::onSetData(void)
	{
		BaseCamera::onSetData();
	}

	void FirstPersonCamera::setRotateSpeed(Ogre::Real rotateSpeed)
	{
		this->rotateSpeed = rotateSpeed;
	}

	void FirstPersonCamera::setSceneNode(Ogre::SceneNode * sceneNode)
	{
		this->sceneNode = sceneNode;
	}

	void FirstPersonCamera::moveCamera(Ogre::Real dt)
	{
		if (nullptr != this->sceneNode)
		{
			Ogre::Vector3 targetPosition = this->sceneNode->_getDerivedPositionUpdated();

			Ogre::Quaternion targetOrientation = MathHelper::getInstance()->lookAt(this->sceneNode->_getDerivedOrientationUpdated() * this->defaultDirection, Ogre::Vector3::UNIT_Y);
			//Drehungsanimation der Kamera
			Ogre::Quaternion delta = Ogre::Quaternion::Slerp(dt * this->rotateSpeed * this->rotateCameraWeight, this->camera->getOrientation(), targetOrientation, true);
			//Ogre::Quaternion delta = targetOrientation;
			this->camera->setOrientation(delta);
			//Die Kanone soll in der Mitte des Bildschirms sein, sie ist jedoch mit einem kleinen Offset (0.4f, 0.8f, -0.2f) am Flubber befestigt, daher X: 0.4
			//Die Kamera soll immer mit einem Offset zum Spiel unabhängig von der Richtung des Spielers platziert sein
			Ogre::Vector3 targetVector = targetPosition + (delta * this->offsetPosition * this->moveCameraWeight); // Attention: Is this correct here with move camera weight?

			//geschmeidige Bewegung der Kamera an die Zielposition
			targetVector = (targetVector * this->smoothValue) + (this->camera->getPosition() * (1.0f - this->smoothValue));
			this->camera->setPosition(targetVector);
		}
	}

	void FirstPersonCamera::rotateCamera(Ogre::Real dt, bool forJoyStick)
	{

	}

	Ogre::Vector3 FirstPersonCamera::getPosition(void)
	{
		return this->camera->getPosition();
	}

	Ogre::Quaternion FirstPersonCamera::getOrientation(void)
	{
		return this->camera->getOrientation();
	}

}; //namespace end

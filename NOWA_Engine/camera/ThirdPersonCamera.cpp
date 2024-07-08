#include "NOWAPrecompiled.h"
#include "ThirdPersonCamera.h"
#include "gameobject/GameObjectComponent.h"
#include "utilities/MathHelper.h"
#include "main/Core.h"

namespace NOWA
{
	ThirdPersonCamera::ThirdPersonCamera(unsigned int id, Ogre::SceneNode* sceneNode, const Ogre::Vector3& defaultDirection, Ogre::Real yOffset, const Ogre::Vector3& lookAtOffset,
		Ogre::Real cameraSpring, Ogre::Real cameraFriction, Ogre::Real cameraSpringLength)
		: BaseCamera(id, 0.0f, 0.0f, 0.0f, defaultDirection),
		sceneNode(sceneNode),
		yOffset(yOffset),
		lookAtOffset(lookAtOffset),
		cameraSpring(cameraSpring),
		cameraFriction(cameraFriction),
		cameraSpringLength(cameraSpringLength)
	{
		// Smoothing creates jitter if simulation updates are 30 ticks per second or below, hence disable smoothing
		if (Core::getSingletonPtr()->getOptionDesiredSimulationUpdates() <= 30)
		{
			this->smoothValue = 0.0f;
		}
	}

	ThirdPersonCamera::~ThirdPersonCamera()
	{
		this->sceneNode = nullptr;
	}

	void ThirdPersonCamera::onSetData(void)
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

	void ThirdPersonCamera::setYOffset(Ogre::Real yOffset)
	{
		this->yOffset = yOffset;
	}

	void ThirdPersonCamera::setCameraSpring(Ogre::Real cameraSpring)
	{
		this->cameraSpring = cameraSpring;
	}

	void ThirdPersonCamera::setCameraFriction(Ogre::Real cameraFriction)
	{
		this->cameraFriction = cameraFriction;
	}

	void ThirdPersonCamera::setCameraSpringLength(Ogre::Real cameraSpringLength)
	{
		this->cameraSpringLength = cameraSpringLength;
	}

	void ThirdPersonCamera::setLookAtOffset(const Ogre::Vector3& lookAtOffset)
	{
		this->lookAtOffset = lookAtOffset;
	}

	void ThirdPersonCamera::setSceneNode(Ogre::SceneNode * sceneNode)
	{
		this->sceneNode = sceneNode;
	}

	void ThirdPersonCamera::moveCamera(Ogre::Real dt)
	{
		if (nullptr != this->sceneNode)
		{
			/* ------------------HooksLaw------------------- */
			Ogre::Vector3 cameraPosition = this->camera->getPosition();

			Ogre::Vector3 playerPosition = Ogre::Vector3(this->sceneNode->_getDerivedPositionUpdated().x, this->sceneNode->_getDerivedPositionUpdated().y + this->yOffset, this->sceneNode->_getDerivedPositionUpdated().z);

			Ogre::Vector3 direction = direction = Ogre::Vector3(cameraPosition - playerPosition);

			//Winkel errechnen
			Ogre::Radian angle = Ogre::Math::ATan2(direction.z, direction.x);
			//Zielrichtung.xz aus der Spielerposition + Federlaenge(offset) * Richtung der Kamera erhalten
			Ogre::Vector3 targetDirection = Ogre::Vector3((playerPosition.x + (Ogre::Math::Cos(angle) * this->cameraSpringLength)), 0.0f, (playerPosition.z + (Ogre::Math::Sin(angle) * this->cameraSpringLength)));
			//Kamerabewegungsgeschwindigkeit.xz aus der Kameraposition zur neuen Zielposition * Federstaerke erhalten
			Ogre::Vector3 velocityVector = Ogre::Vector3((targetDirection.x - cameraPosition.x) * this->cameraSpring, 0.0f, (targetDirection.z - cameraPosition.z) * this->cameraSpring);

			//Zur Kamerabewebungsgeschwindigkeit eine Reibung hinzufuegen
			velocityVector *= this->cameraFriction;

			//Richtung des Spielers erhalten
			Ogre::Vector3 tVector = Ogre::Vector3(this->sceneNode->_getDerivedOrientationUpdated() * this->defaultDirection * -1.0f);
			//tVector.normalise();
			//Richtung.xz mit Kamerafederstaerke skalieren
			tVector *= this->cameraSpringLength * Ogre::Vector3(1.0f, 0.0f, 1.0f);
			//Spielerposition hinzuaddieren
			tVector += playerPosition * Ogre::Vector3(1.0f, 0.0f, 1.0f);
			//Zur Spielerhoehe einen Offset hinzuaddieren
			tVector.y += this->yOffset;

			//neuen Richtungsvektor aus der Kameraposition zum tVektor erhalten und mit der Federlaenge verlaengern
			Ogre::Vector3 vVector = Ogre::Vector3(tVector - cameraPosition) * this->cameraSpring * this->moveCameraWeight;

			//Reibung hinzufuegen
			vVector *= this->cameraFriction;
			//Bewegung zusaetzlich verstaerken
			vVector *= 0.4f * Ogre::Vector3(1.0f, 0.0f, 1.0f);

			//Gewichtung je nach winkel !!!

			//Vektoren addieren
			Ogre::Vector3 positionVector = (cameraPosition + velocityVector + vVector) * Ogre::Vector3(1.0f, 0.0f, 1.0f);
			positionVector.y = (playerPosition.y + this->yOffset);

			this->camera->setPosition(positionVector);
			//Auf die Spielerposition schauen
			this->camera->lookAt((playerPosition + this->lookAtOffset));
		}
	}

	void ThirdPersonCamera::rotateCamera(Ogre::Real dt, bool forJoyStick)
	{

	}

	Ogre::Vector3 ThirdPersonCamera::getPosition(void)
	{
		return this->camera->getPosition();
	}

	Ogre::Quaternion ThirdPersonCamera::getOrientation(void)
	{
		return this->camera->getOrientation();
	}

}; //namespace end

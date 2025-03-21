#include "NOWAPrecompiled.h"
#include "ThirdPersonCamera.h"
#include "gameobject/GameObjectComponent.h"
#include "utilities/MathHelper.h"
#include "main/Core.h"

namespace NOWA
{
	ThirdPersonCamera::ThirdPersonCamera(unsigned int id, Ogre::SceneNode* sceneNode, const Ogre::Vector3& defaultDirection, const Ogre::Vector3& offsetPosition, const Ogre::Vector3& lookAtOffset,
		Ogre::Real cameraSpring, Ogre::Real cameraFriction, Ogre::Real cameraSpringLength)
		: BaseCamera(id, 0.0f, 0.0f, 0.0f, defaultDirection),
		sceneNode(sceneNode),
		offsetPosition(offsetPosition),
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

	void ThirdPersonCamera::setOffsetPosition(const Ogre::Vector3& offsetPosition)
	{
		this->offsetPosition = offsetPosition;
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
		if (nullptr == this->sceneNode) return;

		// Get gravity direction (which points toward the planet center)
		Ogre::Vector3 gravityDir = Ogre::Vector3::UNIT_Y;
		if (false == this->gravityDirection.isZeroLength())
		{
			gravityDir = -this->gravityDirection;
		}

		// Calculate surface normal (opposite of gravity direction)
		Ogre::Vector3 surfaceNormal = gravityDir;

		// Get current camera and player positions
		Ogre::Vector3 cameraPosition = this->camera->getPosition();
		Ogre::Vector3 playerPosition = this->sceneNode->_getDerivedPositionUpdated();

		// Get player's orientation
		Ogre::Quaternion playerOrientation = this->sceneNode->_getDerivedOrientationUpdated();

		// Calculate player's forward direction
		Ogre::Vector3 playerForward = playerOrientation * this->defaultDirection;

		// Create a local coordinate system aligned with the planet surface
		Ogre::Vector3 localUp = surfaceNormal;

		// Project player's forward direction onto the local plane
		Ogre::Vector3 playerForwardProjected = playerForward - (playerForward.dotProduct(localUp) * localUp);
		if (playerForwardProjected.isZeroLength())
		{
			// Fallback if the projected direction is zero
			Ogre::Vector3 fallbackDirection = playerOrientation * Ogre::Vector3::UNIT_Z;
			playerForwardProjected = fallbackDirection - (fallbackDirection.dotProduct(localUp) * localUp);
			if (playerForwardProjected.isZeroLength())
			{
				// Second fallback
				playerForwardProjected = localUp.perpendicular();
			}
		}
		playerForwardProjected.normalise();

		// Calculate local right and forward vectors
		Ogre::Vector3 localRight = localUp.crossProduct(playerForwardProjected);
		localRight.normalise();
		Ogre::Vector3 localForward = localRight.crossProduct(localUp);
		localForward.normalise();

		// Calculate the desired camera position in local coordinates
		// This is the player position plus an offset in the direction opposite to the player's forward
		Ogre::Vector3 playerViewPosition = playerPosition + (localUp * this->offsetPosition);

		// Calculate the camera's desired position
		// It should be behind the player (in the direction opposite to the player's forward)
		// and offset upward along the local up vector
		Ogre::Vector3 targetPosition = playerViewPosition - (playerForwardProjected * this->cameraSpringLength);

		// Apply spring physics for smooth camera movement
		Ogre::Vector3 displacement = targetPosition - cameraPosition;
		Ogre::Vector3 velocityVector = displacement * this->cameraSpring * this->cameraFriction * dt * 60.0f;

		// Calculate new camera position
		Ogre::Vector3 newCameraPosition = cameraPosition + velocityVector;

		// Enforce minimum distance from player to avoid clipping
		Ogre::Vector3 cameraToPlayer = playerViewPosition - newCameraPosition;
		Ogre::Real currentDistance = cameraToPlayer.length();
		if (currentDistance < this->cameraSpringLength * 0.5f)
		{
			newCameraPosition = playerViewPosition - (cameraToPlayer.normalisedCopy() * this->cameraSpringLength * 0.5f);
		}

		// Set camera position
		this->camera->setPosition(newCameraPosition);

		// Make camera look at player with offset
		this->camera->lookAt(playerViewPosition + this->lookAtOffset);

		// Ensure camera's up vector is aligned with the local up vector
		// This prevents the camera from rolling when moving on curved surfaces
		this->camera->setFixedYawAxis(true, localUp);
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

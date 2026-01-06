#include "NOWAPrecompiled.h"
#include "ThirdPersonCamera.h"
#include "gameobject/GameObjectComponent.h"
#include "utilities/MathHelper.h"
#include "main/Core.h"
#include "modules/GraphicsModule.h"

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
		cameraSpringLength(cameraSpringLength),
		lastSmoothedCameraPos(Ogre::Vector3::ZERO)
	{

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
			ENQUEUE_RENDER_COMMAND_WAIT("ThirdPersonCamera::onSetData",
			{
				this->camera->setPosition(this->sceneNode->_getDerivedPositionUpdated());
				this->camera->setOrientation(this->sceneNode->_getDerivedOrientationUpdated());
			});
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ThirdPersonCamera]: Warning cannot use the game object component for init positioning of the camera, since it is NULL.");
		}
		NOWA::GraphicsModule::getInstance()->removeTrackedCamera(this->camera);
	}

	void ThirdPersonCamera::onClearData(void)
	{
		NOWA::GraphicsModule::getInstance()->removeTrackedCamera(this->camera);
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

		// Determine local up (planetary support)
		Ogre::Vector3 gravityDir = this->gravityDirection;
		if (gravityDir.isZeroLength())
			gravityDir = Ogre::Vector3::NEGATIVE_UNIT_Y;
		Ogre::Vector3 localUp = -gravityDir.normalisedCopy();

		// Player basis and camera
		Ogre::Vector3 playerPosition = this->sceneNode->_getDerivedPositionUpdated();
		Ogre::Quaternion playerOrientation = this->sceneNode->_getDerivedOrientationUpdated();
		Ogre::Vector3 cameraPosition = this->camera->getPosition();

		// Apply lookAt offset to get visual center of the player
		playerPosition += this->lookAtOffset;

		// Define local right/forward based on orientation and gravity
		Ogre::Vector3 playerForward = playerOrientation * this->defaultDirection;
		Ogre::Vector3 playerForwardProjected = playerForward - (playerForward.dotProduct(localUp) * localUp);
		playerForwardProjected.normalise();

		Ogre::Vector3 localRight = localUp.crossProduct(playerForwardProjected).normalisedCopy();
		Ogre::Vector3 localForward = localRight.crossProduct(localUp).normalisedCopy();

		// ----------------------
		// Hooks-style Camera Spring

		// Angle (for compatibility)
		Ogre::Vector3 direction = cameraPosition - playerPosition;
		Ogre::Real angle = Ogre::Math::ATan2(direction.dotProduct(localForward), direction.dotProduct(localRight)).valueRadians();

		// Target camera direction from player + cameraSpringLength
		Ogre::Vector3 offsetXZ = Ogre::Math::Cos(angle) * localRight + Ogre::Math::Sin(angle) * localForward;
		Ogre::Vector3 targetDirection = playerPosition + offsetXZ * this->cameraSpringLength;

		// Velocity vector (primary spring movement)
		Ogre::Vector3 velocityVector = (targetDirection - cameraPosition) * this->cameraSpring;
		velocityVector *= this->cameraFriction;

		// Support force (vVector from old code)
		Ogre::Vector3 tVector = playerOrientation * this->defaultDirection * -1.0f;
		tVector = tVector - (tVector.dotProduct(localUp) * localUp);
		tVector.normalise();
		tVector *= this->cameraSpringLength;

		Ogre::Vector3 supportTarget = playerPosition + tVector;
		supportTarget += localUp * this->offsetPosition.y;

		Ogre::Vector3 vVector = (supportTarget - cameraPosition) * this->cameraSpring * this->moveCameraWeight;
		vVector *= this->cameraFriction * 0.4f;

		// Final camera position (projected onto local plane)
		Ogre::Vector3 positionVector = cameraPosition + velocityVector + vVector;
		Ogre::Real height = playerPosition.dotProduct(localUp) + this->offsetPosition.y;
		Ogre::Real currentHeight = positionVector.dotProduct(localUp);
		Ogre::Vector3 verticalOffset = (height - currentHeight) * localUp;
		positionVector += verticalOffset;

		// Final look-at orientation
		Ogre::Quaternion resultOrientation = MathHelper::getInstance()->computeLookAtQuaternion(positionVector, playerPosition, localUp);

		// Apply to camera
		this->camera->setFixedYawAxis(true, localUp);
		NOWA::GraphicsModule::getInstance()->updateCameraOrientation(this->camera, resultOrientation);
		NOWA::GraphicsModule::getInstance()->updateCameraPosition(this->camera, positionVector);
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

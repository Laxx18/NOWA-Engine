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

#if 1
    void FirstPersonCamera::moveCamera(Ogre::Real dt)
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

        // Get player position and orientation
        Ogre::Vector3 playerPosition = this->sceneNode->_getDerivedPositionUpdated();
        Ogre::Quaternion playerOrientation = this->sceneNode->_getDerivedOrientationUpdated();

        // Get player's forward direction based on orientation and default direction
        Ogre::Vector3 playerForward = playerOrientation * this->defaultDirection;

        // Create a local coordinate system
        Ogre::Vector3 localUp = surfaceNormal;

        // We need to replicate the MathHelper::lookAt function from your original code
        // that was used to calculate the targetOrientation

        // First, determine the forward vector (direction the player is looking)
        Ogre::Vector3 forward = playerOrientation * this->defaultDirection;

        // Project this forward vector onto the local tangent plane
        Ogre::Vector3 projectedForward = forward - (forward.dotProduct(localUp) * localUp);
        if (projectedForward.isZeroLength())
        {
            // If the forward vector happens to be parallel to up, use a fallback
            projectedForward = playerOrientation * Ogre::Vector3::UNIT_Z;
            projectedForward = projectedForward - (projectedForward.dotProduct(localUp) * localUp);
            if (projectedForward.isZeroLength())
            {
                projectedForward = localUp.perpendicular();
            }
        }
        projectedForward.normalise();

        // Calculate right vector
        Ogre::Vector3 right = projectedForward.crossProduct(localUp);
        right.normalise();

        // Recalculate forward to ensure orthogonality
        forward = localUp.crossProduct(right);
        forward.normalise();

        // Create rotation matrix and convert to quaternion
        Ogre::Matrix3 rotMatrix;
        rotMatrix.FromAxes(right, localUp, -forward); // Negative forward to match lookAt behavior
        Ogre::Quaternion targetOrientation = Ogre::Quaternion(rotMatrix);

        // Apply smooth rotation
        Ogre::Quaternion delta = Ogre::Quaternion::Slerp(dt * 60.0f * this->rotateSpeed * this->rotateCameraWeight, this->camera->getOrientation(), targetOrientation, true);

        // Set camera orientation
        this->camera->setOrientation(delta);

        // Calculate camera position based on player position and offset
        // Transform offset by the delta orientation to get the correct position in world space
        Ogre::Vector3 transformedOffset = delta * this->offsetPosition;

        // Calculate target position
        Ogre::Vector3 targetVector = playerPosition + (transformedOffset * this->moveCameraWeight);

        // Apply smooth movement
        Ogre::Vector3 smoothedPosition = (targetVector * this->smoothValue) + (this->camera->getPosition() * (1.0f - this->smoothValue));

        // Set camera position
        this->camera->setPosition(smoothedPosition);
    }
#else
    void FirstPersonCamera::moveCamera(Ogre::Real dt)
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

        // Get player position and orientation
        Ogre::Vector3 playerPosition = this->sceneNode->_getDerivedPositionUpdated();
        Ogre::Quaternion playerOrientation = this->sceneNode->_getDerivedOrientationUpdated();

        // Get player's forward direction based on orientation and default direction
        Ogre::Vector3 playerForward = playerOrientation * this->defaultDirection;

        // Create a local coordinate system
        Ogre::Vector3 localUp = surfaceNormal;

        // Determine the forward vector (direction the player is looking)
        Ogre::Vector3 forward = playerOrientation * this->defaultDirection;

        // Project this forward vector onto the local tangent plane
        Ogre::Vector3 projectedForward = forward - (forward.dotProduct(localUp) * localUp);
        if (projectedForward.isZeroLength())
        {
            // If the forward vector happens to be parallel to up, use a fallback
            projectedForward = playerOrientation * Ogre::Vector3::UNIT_Z;
            projectedForward = projectedForward - (projectedForward.dotProduct(localUp) * localUp);
            if (projectedForward.isZeroLength())
            {
                projectedForward = localUp.perpendicular();
            }
        }
        projectedForward.normalise();

        // Calculate right vector
        Ogre::Vector3 right = projectedForward.crossProduct(localUp);
        right.normalise();

        // Recalculate forward to ensure orthogonality
        forward = localUp.crossProduct(right);
        forward.normalise();

        // Create rotation matrix and convert to quaternion
        Ogre::Matrix3 rotMatrix;
        rotMatrix.FromAxes(right, localUp, -forward); // Negative forward to match lookAt behavior
        Ogre::Quaternion targetOrientation = Ogre::Quaternion(rotMatrix);

        // Normalize rotation speed based on frame time
        Ogre::Real normalizedRotateSpeed = this->rotateSpeed * dt * 60.0f * this->rotateCameraWeight;

        // Apply smooth rotation with frame-independent interpolation
        Ogre::Quaternion delta = Ogre::Quaternion::Slerp(normalizedRotateSpeed, this->camera->getOrientation(), targetOrientation, true);

        // Set camera orientation
        this->camera->setOrientation(delta);

        // Transform offset by the delta orientation to get the correct position in world space
        Ogre::Vector3 transformedOffset = delta * this->offsetPosition;

        // Normalize camera movement based on frame time
        Ogre::Real normalizedMoveWeight = this->moveCameraWeight * dt * 60.0f;

        // Calculate target position
        Ogre::Vector3 targetVector = playerPosition + (transformedOffset * normalizedMoveWeight);

        // Apply smooth movement with frame-independent interpolation
        Ogre::Vector3 smoothedPosition = (targetVector * this->smoothValue) +
            (this->camera->getPosition() * (1.0f - this->smoothValue));

        // Set camera position
        this->camera->setPosition(smoothedPosition);
    }
#endif

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

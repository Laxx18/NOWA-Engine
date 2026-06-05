#include "NOWAPrecompiled.h"
#include "FirstPersonCamera.h"
#include "gameobject/GameObjectComponent.h"
#include "utilities/MathHelper.h"
#include "main/Core.h"
#include "modules/GraphicsModule.h"

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

    void FirstPersonCamera::moveCamera(Ogre::Real dt)
    {
        if (nullptr == this->sceneNode)
        {
            return;
        }

        Ogre::Vector3 gravityDir = Ogre::Vector3::UNIT_Y;
        if (false == this->gravityDirection.isZeroLength())
        {
            gravityDir = -this->gravityDirection;
        }

        Ogre::Vector3 surfaceNormal = gravityDir;

        // Read directly from the physics body — NOT from the SceneNode.
        // SceneNode is updated by the render thread one frame later.
        Ogre::Vector3 playerPosition = this->sceneNode->_getDerivedPositionUpdated();
        Ogre::Quaternion playerOrientation = this->sceneNode->_getDerivedOrientationUpdated();

        // Everything below unchanged
        Ogre::Vector3 localUp = surfaceNormal;

        Ogre::Vector3 forward = playerOrientation * this->defaultDirection;

        Ogre::Vector3 projectedForward = forward - (forward.dotProduct(localUp) * localUp);
        if (projectedForward.isZeroLength())
        {
            projectedForward = playerOrientation * Ogre::Vector3::UNIT_Z;
            projectedForward = projectedForward - (projectedForward.dotProduct(localUp) * localUp);
            if (projectedForward.isZeroLength())
            {
                projectedForward = localUp.perpendicular();
            }
        }
        projectedForward.normalise();

        Ogre::Vector3 right = projectedForward.crossProduct(localUp);
        right.normalise();

        forward = localUp.crossProduct(right);
        forward.normalise();

        Ogre::Matrix3 rotMatrix;
        rotMatrix.FromAxes(right, localUp, -forward);
        Ogre::Quaternion targetOrientation = Ogre::Quaternion(rotMatrix);

        Ogre::Quaternion delta = Ogre::Quaternion::Slerp(dt * 60.0f * this->rotateSpeed * this->rotateCameraWeight, this->camera->getOrientation(), targetOrientation, true);

        Ogre::Vector3 transformedOffset = delta * this->offsetPosition;
        Ogre::Vector3 targetVector = playerPosition + (transformedOffset * this->moveCameraWeight);
        Ogre::Vector3 smoothedPosition = (targetVector * this->smoothValue) + (this->camera->getPosition() * (1.0f - this->smoothValue));

        NOWA::GraphicsModule::getInstance()->updateCameraTransform(this->camera, smoothedPosition, delta);
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

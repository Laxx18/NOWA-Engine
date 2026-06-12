#include "NOWAPrecompiled.h"
#include "ThirdPersonCamera.h"
#include "gameobject/GameObjectComponent.h"
#include "main/Core.h"
#include "modules/GraphicsModule.h"
#include "utilities/MathHelper.h"

namespace NOWA
{
    ThirdPersonCamera::ThirdPersonCamera(unsigned int id, Ogre::SceneNode* sceneNode, const Ogre::Vector3& defaultDirection, const Ogre::Vector3& offsetPosition, const Ogre::Vector3& lookAtOffset, Ogre::Real cameraSpring, Ogre::Real cameraFriction,
        Ogre::Real cameraSpringLength) :
        BaseCamera(id, 0.0f, 0.0f, 0.0f, defaultDirection),
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
            ENQUEUE_RENDER_COMMAND_WAIT("ThirdPersonCamera::onSetData", {
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

    //void ThirdPersonCamera::moveCamera(Ogre::Real dt)
    //{
    //    if (nullptr == this->sceneNode)
    //    {
    //        return;
    //    }

    //    // Determine local up (planetary support)
    //    Ogre::Vector3 gravityDir = this->gravityDirection;
    //    if (gravityDir.isZeroLength())
    //    {
    //        gravityDir = Ogre::Vector3::NEGATIVE_UNIT_Y;
    //    }
    //    Ogre::Vector3 localUp = -gravityDir.normalisedCopy();

    //    // Read directly from the physics body (m_curPosit / m_curRotation) --
    //    // NOT from the SceneNode. The SceneNode is updated by the render thread
    //    // one frame later via enqueued _setDerivedPosition. At high speed that
    //    // one-frame lag is large enough to make the camera visibly chase the ship.
    //    Ogre::Vector3 playerPosition;
    //    Ogre::Quaternion playerOrientation;

    //    if (nullptr != this->physicsBody)
    //    {
    //        playerPosition = this->physicsBody->getPosition();
    //        playerOrientation = this->physicsBody->getOrientation();
    //    }
    //    else
    //    {
    //        playerPosition = this->sceneNode->_getDerivedPositionUpdated();
    //        playerOrientation = this->sceneNode->_getDerivedOrientationUpdated();
    //    }

    //    Ogre::Vector3 cameraPosition = this->camera->getPosition();

    //    // Apply lookAt offset to get visual center of the player
    //    playerPosition += this->lookAtOffset;

    //    // ----------------------
    //    // Build a canonical camera frame from the player's actual surface-projected
    //    // forward and up vectors. This makes offsetPosition and the support target
    //    // axis-agnostic: (0, 4, 7) always means "4 up, 7 behind the player"
    //    // regardless of whether defaultDirection is -Z, +X, or anything else.
    //    Ogre::Vector3 camForward = playerOrientation * this->defaultDirection;
    //    camForward = camForward - camForward.dotProduct(localUp) * localUp;
    //    if (camForward.squaredLength() < 1e-6f)
    //    {
    //        // Fallback: use a stable world reference projected onto the horizontal plane
    //        Ogre::Vector3 worldRef = (Ogre::Math::Abs(localUp.dotProduct(Ogre::Vector3::UNIT_Z)) < 0.9f) ? Ogre::Vector3::UNIT_Z : Ogre::Vector3::UNIT_X;
    //        camForward = worldRef - worldRef.dotProduct(localUp) * localUp;
    //        camForward.normalise();
    //    }
    //    else
    //    {
    //        camForward.normalise();
    //    }
    //    Ogre::Vector3 camRight = localUp.crossProduct(camForward);
    //    camRight.normalise();

    //    // offsetPosition convention: x = right, y = up, z = behind (positive z = behind player)
    //    Ogre::Vector3 localOffset = camRight * this->offsetPosition.x + localUp * this->offsetPosition.y + (-camForward) * this->offsetPosition.z;

    //    // ----------------------
    //    // Stable horizontal frame from gravity only (never from player orientation).
    //    // Used for the orbit spring angle so the camera does not tilt with the player.
    //    Ogre::Vector3 worldRef = (Ogre::Math::Abs(localUp.dotProduct(Ogre::Vector3::UNIT_Z)) < 0.9f) ? Ogre::Vector3::UNIT_Z : Ogre::Vector3::UNIT_X;
    //    Ogre::Vector3 stableRight = localUp.crossProduct(worldRef).normalisedCopy();
    //    Ogre::Vector3 stableForward = stableRight.crossProduct(localUp).normalisedCopy();

    //    // ----------------------
    //    // Hooks-style Camera Spring

    //    Ogre::Vector3 direction = cameraPosition - playerPosition;
    //    Ogre::Vector3 directionH = direction - direction.dotProduct(localUp) * localUp;
    //    Ogre::Radian angle = Ogre::Math::ATan2(directionH.z, directionH.x);

    //    Ogre::Vector3 offsetXZ = Ogre::Math::Cos(angle) * stableRight + Ogre::Math::Sin(angle) * stableForward;

    //    // Target position: player + spring orbit + player-local offset
    //    Ogre::Vector3 targetPosition = playerPosition + offsetXZ * this->cameraSpringLength + localOffset;

    //    // Velocity vector (primary spring movement toward target)
    //    Ogre::Vector3 velocityVector = (targetPosition - cameraPosition) * this->cameraSpring;
    //    velocityVector *= this->cameraFriction;

    //    // Support force -- pulls camera toward player's back using the canonical frame.
    //    Ogre::Vector3 tVector = -camForward * this->cameraSpringLength;
    //    Ogre::Vector3 supportTarget = playerPosition + tVector + localOffset;

    //    Ogre::Vector3 vVector = (supportTarget - cameraPosition) * this->cameraSpring * this->moveCameraWeight;
    //    vVector *= this->cameraFriction;

    //    // Final camera position
    //    Ogre::Vector3 totalDisplacement = velocityVector + vVector;
    //    Ogre::Vector3 positionVector = cameraPosition + totalDisplacement;

    //    // Final look-at orientation
    //    Ogre::Quaternion resultOrientation = MathHelper::getInstance()->computeLookAtQuaternion(positionVector, playerPosition, localUp);

    //    this->camera->setFixedYawAxis(true, localUp);
    //    NOWA::GraphicsModule::getInstance()->updateCameraOrientation(this->camera, resultOrientation);
    //    NOWA::GraphicsModule::getInstance()->updateCameraPosition(this->camera, positionVector);
    //}

void ThirdPersonCamera::moveCamera(Ogre::Real dt)
    {
        if (nullptr == this->sceneNode)
        {
            return;
        }

        // Determine local up (planetary support)
        Ogre::Vector3 gravityDir = this->gravityDirection;
        if (gravityDir.isZeroLength())
        {
            gravityDir = Ogre::Vector3::NEGATIVE_UNIT_Y;
        }
        Ogre::Vector3 localUp = -gravityDir.normalisedCopy();

        // Read directly from the physics body (m_curPosit / m_curRotation) --
        // NOT from the SceneNode. The SceneNode is updated by the render thread
        // one frame later via enqueued _setDerivedPosition. At high speed that
        // one-frame lag is large enough to make the camera visibly chase the ship.
        Ogre::Vector3 playerPosition;
        Ogre::Quaternion playerOrientation;

        if (nullptr != this->physicsBody)
        {
            playerPosition = this->physicsBody->getPosition();
            playerOrientation = this->physicsBody->getOrientation();
        }
        else
        {
            playerPosition = this->sceneNode->_getDerivedPositionUpdated();
            playerOrientation = this->sceneNode->_getDerivedOrientationUpdated();
        }

        Ogre::Vector3 cameraPosition = this->camera->getPosition();

        // Apply lookAt offset to get visual center of the player
        playerPosition += this->lookAtOffset;

        // ----------------------
        // Build a canonical camera frame using ONLY the ship's yaw projected
        // onto the gravity plane — roll and pitch are intentionally stripped.
        // This means the camera stays level even if the ship is rolled 90°.
        Ogre::Vector3 camForward = playerOrientation * this->defaultDirection;

        // Project onto the horizontal plane defined by localUp — strips pitch and roll
        camForward = camForward - camForward.dotProduct(localUp) * localUp;

        if (camForward.squaredLength() < 1e-6f)
        {
            // Ship is pointing straight up/down — fall back to a stable world reference
            Ogre::Vector3 worldRef = (Ogre::Math::Abs(localUp.dotProduct(Ogre::Vector3::UNIT_Z)) < 0.9f) ? Ogre::Vector3::UNIT_Z : Ogre::Vector3::UNIT_X;
            camForward = worldRef - worldRef.dotProduct(localUp) * localUp;
        }
        camForward.normalise();

        // Right is always derived from localUp x camForward — never from ship's right axis.
        // This guarantees the camera frame is always upright regardless of ship roll.
        Ogre::Vector3 camRight = localUp.crossProduct(camForward);
        camRight.normalise();

        // offsetPosition convention: x = right, y = up, z = behind (positive z = behind player)
        Ogre::Vector3 localOffset = camRight * this->offsetPosition.x + localUp * this->offsetPosition.y + (-camForward) * this->offsetPosition.z;

        // ----------------------
        // Stable horizontal frame from gravity only (never from player orientation).
        // Used for the orbit spring angle so the camera does not tilt with the player.
        Ogre::Vector3 worldRef2 = (Ogre::Math::Abs(localUp.dotProduct(Ogre::Vector3::UNIT_Z)) < 0.9f) ? Ogre::Vector3::UNIT_Z : Ogre::Vector3::UNIT_X;
        Ogre::Vector3 stableRight = localUp.crossProduct(worldRef2).normalisedCopy();
        Ogre::Vector3 stableForward = stableRight.crossProduct(localUp).normalisedCopy();

        // ----------------------
        // Hooks-style Camera Spring

        Ogre::Vector3 direction = cameraPosition - playerPosition;
        Ogre::Vector3 directionH = direction - direction.dotProduct(localUp) * localUp;
        Ogre::Radian angle = Ogre::Math::ATan2(directionH.z, directionH.x);

        Ogre::Vector3 offsetXZ = Ogre::Math::Cos(angle) * stableRight + Ogre::Math::Sin(angle) * stableForward;

        // Target position: player + spring orbit + player-local offset
        Ogre::Vector3 targetPosition = playerPosition + offsetXZ * this->cameraSpringLength + localOffset;

        // Velocity vector (primary spring movement toward target)
        Ogre::Vector3 velocityVector = (targetPosition - cameraPosition) * this->cameraSpring;
        velocityVector *= this->cameraFriction;

        // Support force -- pulls camera toward player's back using the canonical frame.
        Ogre::Vector3 tVector = -camForward * this->cameraSpringLength;
        Ogre::Vector3 supportTarget = playerPosition + tVector + localOffset;

        Ogre::Vector3 vVector = (supportTarget - cameraPosition) * this->cameraSpring * this->moveCameraWeight;
        vVector *= this->cameraFriction;

        // Final camera position
        Ogre::Vector3 totalDisplacement = velocityVector + vVector;
        Ogre::Vector3 positionVector = cameraPosition + totalDisplacement;

        // Final look-at orientation
        Ogre::Quaternion resultOrientation = MathHelper::getInstance()->computeLookAtQuaternion(positionVector, playerPosition, localUp);

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

}; // namespace end
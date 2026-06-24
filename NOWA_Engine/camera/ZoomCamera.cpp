#include "NOWAPrecompiled.h"
#include "ZoomCamera.h"
#include "gameobject/GameObject.h"
#include "gameobject/PhysicsActiveComponent.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "modules/GraphicsModule.h"
#include "utilities/MathHelper.h"

namespace NOWA
{
    ZoomCamera::ZoomCamera(unsigned int id, const Ogre::Vector3& offsetPosition, Ogre::Real smoothValue) :
        BaseCamera(id, 0, 0, smoothValue),
        sceneManager(nullptr),
        smoothValue(smoothValue),
        categoryId(static_cast<unsigned int>(0)),
        growMultiplicator(2.0f),
        desiredPosition(Ogre::Vector3::ZERO),
        oldProjectionType(Ogre::PT_PERSPECTIVE),
        oldOrthogonalSize(10.0f, 10.0f),
        moveVelocity(Ogre::Vector3::ZERO),
        zoomSpeed(0.0f)
    {
        // Smoothing creates jitter if simulation updates are 30 ticks per second or below, hence disable smoothing
        if (Core::getSingletonPtr()->getOptionDesiredSimulationUpdates() <= 30)
        {
            this->smoothValue = 0.0f;
        }
    }

    ZoomCamera::~ZoomCamera()
    {
    }

    void ZoomCamera::onSetData(void)
    {
        BaseCamera::onSetData();
        this->firstTimeValueSet = true;

        // Camera must be orthographic!
        this->oldProjectionType = this->camera->getProjectionType();

        NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
        {
            this->camera->setProjectionType(Ogre::PT_ORTHOGRAPHIC);

            // Calcluates the desired position.
            this->calcAveragePosition();

            // Sets the camera's position to the desired position without damping.
            this->camera->setPosition(this->desiredPosition);

            this->oldOrthogonalSize.x = this->camera->getOrthoWindowWidth();
            this->oldOrthogonalSize.y = this->camera->getOrthoWindowHeight();

            // Find and set the required size of the camera.
            // Note: Internally in setOrthoWindowWidth the value is / aspectRatio
            Ogre::Real size = this->calcRequiredSize();
            this->camera->setOrthoWindowWidth(size);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ZoomCamera::onSetData");

        // Optimal position: -16 18 3
        // Optimal orientation: -80 -60 0
    }

    void ZoomCamera::onClearData(void)
    {
        NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
        {
            this->camera->setProjectionType(this->oldProjectionType);
            this->camera->setOrthoWindowWidth(this->oldOrthogonalSize.x);
            this->camera->setOrthoWindowHeight(this->oldOrthogonalSize.y);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ZoomCamera::onClearData");
    }

    void ZoomCamera::setCategory(const Ogre::String& category)
    {
        this->category = category;
        // What is with several categories, e.g. Player + Enemy, check it here
        // and store internal a combined category id
        this->categoryId = AppStateManager::getSingletonPtr()->getGameObjectController()->generateCategoryId(this->category);
    }

    void ZoomCamera::setDefaultDirection(const Ogre::Vector3& direction)
    {
        this->defaultDirection = direction;
        this->firstTimeValueSet = true;
    }

    void ZoomCamera::setGrowMultiplicator(Ogre::Real growMultiplicator)
    {
        this->growMultiplicator = growMultiplicator;
    }

    void ZoomCamera::calcAveragePosition(void)
    {
        Ogre::Vector3 averagePosition = Ogre::Vector3::ZERO;
        unsigned int numTargets = 0;

        std::vector<GameObjectPtr> gameObjects = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsFromCategoryId(this->categoryId);

        for (size_t i = 0; i < gameObjects.size(); i++)
        {
            GameObject* gameObject = gameObjects[i].get();

            // Read from physics body if available — SceneNode is one frame behind.
            Ogre::Vector3 pos;
            auto physComp = NOWA::makeStrongPtr(gameObject->getComponent<PhysicsComponent>());
            if (nullptr != this->physicsBody)
            {
                pos = this->physicsBody->getPosition();
            }
            else
            {
                pos = gameObject->getPosition();
            }

            averagePosition += pos;
            numTargets++;
        }

        if (numTargets > 0)
        {
            averagePosition /= numTargets;
        }

        averagePosition.y = this->camera->getPosition().y;
        this->desiredPosition = averagePosition;
    }

    Ogre::Real ZoomCamera::calcRequiredSize(void)
    {
        Ogre::Quaternion localOrientation = Ogre::Quaternion::IDENTITY;
        Ogre::Vector3 desiredLocalPosition = Ogre::Vector3::ZERO;
        MathHelper::getInstance()->globalToLocal(this->camera->getParentSceneNode(), Ogre::Quaternion::IDENTITY, this->desiredPosition, localOrientation, desiredLocalPosition);

        Ogre::Real size = 0.0f;

        std::vector<GameObjectPtr> gameObjects = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsFromCategoryId(this->categoryId);

        for (size_t i = 0; i < gameObjects.size(); i++)
        {
            GameObject* gameObject = gameObjects[i].get();

            // Read from physics body if available
            Ogre::Vector3 pos;
            Ogre::Quaternion orient;
            auto physComp = NOWA::makeStrongPtr(gameObject->getComponent<PhysicsComponent>());
            if (nullptr != this->physicsBody)
            {
                pos = this->physicsBody->getPosition();
                orient = this->physicsBody->getOrientation();
            }
            else
            {
                pos = gameObject->getPosition();
                orient = gameObject->getOrientation();
            }

            Ogre::Quaternion localOrient = Ogre::Quaternion::IDENTITY;
            Ogre::Vector3 targetLocalPosition = Ogre::Vector3::ZERO;
            MathHelper::getInstance()->globalToLocal(this->camera->getParentSceneNode(), orient, pos, localOrient, desiredLocalPosition);

            Ogre::Vector3 desiredPosToTarget = targetLocalPosition - desiredLocalPosition;
            desiredPosToTarget *= this->growMultiplicator;

            size = std::fmaxf(size, Ogre::Math::Abs(desiredPosToTarget.y));
            size = std::fmaxf(size, Ogre::Math::Abs(desiredPosToTarget.z));
            size = std::fmaxf(size, Ogre::Math::Abs(desiredPosToTarget.x));
        }

        size += 8.0f;
        size = std::fmaxf(size, 13.0f);

        return size;
    }

    void ZoomCamera::zoomCamera(void)
    {
        auto closureFunction = [this](Ogre::Real renderDt)
        {
            // Find the required size based on the desired position and smoothly transition to that size.
            Ogre::Real requiredSize = calcRequiredSize();
            // TODO: Set from outside?
            Ogre::Real dampTime = 0.2f;
            this->camera->setOrthoWindowWidth(MathHelper::getInstance()->smoothDamp(this->camera->getOrthoWindowWidth(), requiredSize, this->zoomSpeed, dampTime));
        };
        Ogre::String id = "ZoomCamera::zoomCamera";
        NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction);
    }

    void ZoomCamera::moveCamera(Ogre::Real dt)
    {
        this->calcAveragePosition();

        if (0.0f != this->smoothValue)
        {
            // TODO: Set from outside?
            Ogre::Real dampTime = 0.2f;
            // this->camera->setPosition(MathHelper::getInstance()->smoothDamp(this->camera->getPosition(), this->desiredPosition, this->moveVelocity, dampTime));
            NOWA::GraphicsModule::getInstance()->updateCameraPosition(this->camera, MathHelper::getInstance()->smoothDamp(this->camera->getPosition(), this->desiredPosition, this->moveVelocity, dampTime));
        }
        this->lastMoveValue = this->camera->getPosition();

        this->zoomCamera();
    }

    void ZoomCamera::rotateCamera(Ogre::Real dt, bool forJoyStick)
    {
    }

    Ogre::Vector3 ZoomCamera::getPosition(void)
    {
        return this->camera->getPosition();
    }

    Ogre::Quaternion ZoomCamera::getOrientation(void)
    {
        return this->camera->getOrientation();
    }

}; // namespace end
#include "NOWAPrecompiled.h"
#include "FollowCamera2D.h"
#include "gameobject/GameObject.h"
#include "gameobject/GameObjectController.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "modules/GraphicsModule.h"
#include "utilities/MathHelper.h"

namespace NOWA
{

    FollowCamera2D::FollowCamera2D(unsigned int id, Ogre::SceneNode* sceneNode, const Ogre::Vector3& offsetPosition, Ogre::Real smoothValue) :
        BaseCamera(id, 0, 0, smoothValue),
        sceneNode(sceneNode),
        sceneManager(nullptr),
        lastMoveValue(Ogre::Vector3::ZERO),
        firstTimeValueSet(true),
        firstTimeMoveValueSet(true),
        offset(offsetPosition),
        borderOffset(Ogre::Vector3(50.0f, 0.0f, 0.0f)),
        showGameObject(false),
        raySceneQuery(nullptr),
        hiddenSceneNode(nullptr),
        fadeValue(0.0f),
        fadingFinished(true),
        mostRightUp(Ogre::Vector3::ZERO),
        smoothValue(smoothValue),
        minimumBounds(Ogre::Vector3::ZERO),
        maximumBounds(Ogre::Vector3::ZERO),
        trackedCameraPosition(Ogre::Vector3::ZERO),
        pDebugLine(nullptr),
        edgeOrthographicClosureRegistered(false),
        edgeOrthographicActive(false),
        edgePlayerPosition(Ogre::Vector3::ZERO),
        edgeCameraPosition(Ogre::Vector3::ZERO),
        edgeOrthoWindow(Ogre::Vector2::ZERO),
        edgeCameraClampedX(false),
        edgeCameraClampedY(false)
    {
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &FollowCamera2D::handleUpdateBounds), EventDataBoundsUpdated::getStaticEventType());
    }

    FollowCamera2D::~FollowCamera2D()
    {
        if (true == this->edgeOrthographicClosureRegistered)
        {
            NOWA::GraphicsModule::getInstance()->removeTrackedClosure("FollowCamera2D::edgeOrthographic");

            this->edgeOrthographicClosureRegistered = false;
        }

        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &FollowCamera2D::handleUpdateBounds), EventDataBoundsUpdated::getStaticEventType());

        this->sceneNode = nullptr;

        if (this->raySceneQuery)
        {
            NOWA::GraphicsModule::RenderCommand oceanRdCmd = [this]
            {
                this->sceneManager->destroyQuery(this->raySceneQuery);
            };

            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(oceanRdCmd), "FollowCamera2D::~FollowCamera2D");
        }
    }

    void FollowCamera2D::onSetData(void)
    {
        BaseCamera::onSetData();
        this->firstTimeMoveValueSet = true;
    }

    void FollowCamera2D::onClearData(void)
    {
        BaseCamera::onClearData();
    }

    void FollowCamera2D::setOffset(const Ogre::Vector3& offset)
    {
        this->offset = offset;
        this->firstTimeMoveValueSet = true;
    }

    void FollowCamera2D::handleUpdateBounds(NOWA::EventDataPtr eventData)
    {
        boost::shared_ptr<NOWA::EventDataBoundsUpdated> castEventData = boost::static_pointer_cast<EventDataBoundsUpdated>(eventData);

        this->setBounds(castEventData->getCalculatedBounds().first, castEventData->getCalculatedBounds().second);
    }

    void FollowCamera2D::setBorderOffset(const Ogre::Vector3& borderOffset)
    {
        this->borderOffset = borderOffset;
        this->firstTimeMoveValueSet = true;
    }

    void FollowCamera2D::setBounds(const Ogre::Vector3& minimumBounds, const Ogre::Vector3& maximumBounds)
    {
        this->minimumBounds = minimumBounds;
        this->maximumBounds = maximumBounds;

        Ogre::LogManager::getSingleton().logMessage(Ogre::LML_NORMAL, "[FollowCamera2D] minimum bounds: " + Ogre::StringConverter::toString(this->minimumBounds) + " maximum bounds: " + Ogre::StringConverter::toString(this->maximumBounds));

        if (nullptr == this->camera)
        {
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[FollowCamera2D] Error: Cannot set bounds because the camera does not exist yet. Please call first CameraManager->addCameraBehavior(...)!");

            throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[FollowCamera2D] Error: Cannot set bounds because the camera does not exist yet. Please call first CameraManager->addCameraBehavior(...)!\n", "NOWA");
        }

        const Ogre::Real distanceToPlayPlane = Ogre::Math::Abs(this->offset.z);

        const Ogre::Radian halfFovY = this->camera->getFOVy() * 0.5f;

        const Ogre::Real halfHeight = distanceToPlayPlane * Ogre::Math::Tan(halfFovY);

        const Ogre::Real halfWidth = halfHeight * this->camera->getAspectRatio();

        this->mostRightUp = Ogre::Vector3(halfWidth, halfHeight, 0.0f);

        this->mostRightUp -= Ogre::Vector3(this->borderOffset.x, this->borderOffset.y, 0.0f);

        Ogre::LogManager::getSingleton().logMessage(Ogre::LML_NORMAL, "[FollowCamera2D] mostRightUp: " + Ogre::StringConverter::toString(this->mostRightUp));

        this->firstTimeValueSet = true;
        this->firstTimeMoveValueSet = true;
    }

    void FollowCamera2D::alwaysShowGameObject(bool show, const Ogre::String& category, Ogre::SceneManager* sceneManager)
    {
        NOWA::GraphicsModule::RenderCommand oceanRdCmd = [this, show, category, sceneManager]
        {
            this->showGameObject = show;
            this->category = category;
            this->sceneManager = sceneManager;

            if (!this->showGameObject)
            {
                if (this->raySceneQuery)
                {
                    this->sceneManager->destroyQuery(this->raySceneQuery);

                    this->raySceneQuery = nullptr;
                }
            }
            else
            {
                this->raySceneQuery = this->sceneManager->createRayQuery(Ogre::Ray());
            }
        };

        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(oceanRdCmd), "FollowCamera2D::alwaysShowGameObject");
    }

    void FollowCamera2D::setSceneNode(Ogre::SceneNode* sceneNode)
    {
        this->sceneNode = sceneNode;
    }

    void FollowCamera2D::updateEdgeOrthographic(const Ogre::Vector3& playerPosition, const Ogre::Vector3& cameraPosition, bool cameraClampedX, bool cameraClampedY)
    {
        this->edgePlayerPosition = playerPosition;
        this->edgeCameraPosition = cameraPosition;
        this->edgeCameraClampedX = cameraClampedX;
        this->edgeCameraClampedY = cameraClampedY;

        const Ogre::Real cameraMinX = this->minimumBounds.x + this->mostRightUp.x;

        const Ogre::Real cameraMaxX = this->maximumBounds.x - this->mostRightUp.x;

        // The camera is smoothed, therefore it does not necessarily land
        // exactly on the mathematical clamp position.
        const Ogre::Real edgeTolerance = 1.0f;

        const bool cameraAtLeft = cameraPosition.x <= cameraMinX + edgeTolerance;

        const bool cameraAtRight = cameraPosition.x >= cameraMaxX - edgeTolerance;

        const bool playerBeyondLeft = cameraAtLeft && playerPosition.x < cameraPosition.x;

        const bool playerBeyondRight = cameraAtRight && playerPosition.x > cameraPosition.x;

        const bool shouldEnterOrtho = playerBeyondLeft || playerBeyondRight;

        // Keep the mode active for a small distance when the player
        // crosses back over the camera center. This prevents rapid
        // perspective/orthographic flickering.
        const Ogre::Real releaseDistance = 0.5f;

        bool shouldStayOrtho = false;

        if (true == this->edgeOrthographicActive)
        {
            if (cameraAtLeft)
            {
                shouldStayOrtho = playerPosition.x < cameraPosition.x + releaseDistance;
            }
            else if (cameraAtRight)
            {
                shouldStayOrtho = playerPosition.x > cameraPosition.x - releaseDistance;
            }
        }

        this->edgeOrthographicActive = shouldEnterOrtho || shouldStayOrtho;

        // Keep the horizontal orthographic size identical to the
        // perspective view, but add a small vertical safety margin.
        //
        // Without this margin the orthographic bottom edge can land
        // exactly on minimumBounds.y. A one-unit-high platform below
        // the player can then disappear when the player falls back
        // down from a higher platform.
        const Ogre::Real orthographicVerticalMargin = 1.0f;

        this->edgeOrthoWindow = Ogre::Vector2(this->mostRightUp.x * 2.0f, this->mostRightUp.y * 2.0f + orthographicVerticalMargin);

        if (false == this->edgeOrthographicClosureRegistered)
        {
            this->edgeOrthographicClosureRegistered = true;

            auto closureFunction = [this](Ogre::Real renderDt)
            {
                if (nullptr == this->camera)
                {
                    return;
                }

                static bool lastOrthographicState = false;

                if (true == this->edgeOrthographicActive)
                {
                    if (false == lastOrthographicState)
                    {
                        Ogre::String side = "UNKNOWN";

                        const Ogre::Real cameraMinX = this->minimumBounds.x + this->mostRightUp.x;

                        const Ogre::Real cameraMaxX = this->maximumBounds.x - this->mostRightUp.x;

                        if (this->edgeCameraPosition.x >= cameraMaxX - 1.0f)
                        {
                            side = "RIGHT";
                        }
                        else if (this->edgeCameraPosition.x <= cameraMinX + 1.0f)
                        {
                            side = "LEFT";
                        }

                        Ogre::LogManager::getSingleton().logMessage(Ogre::LML_NORMAL, "[FollowCamera2D][Ortho] START " + side + " player=" + Ogre::StringConverter::toString(this->edgePlayerPosition) +
                                                                                          " camera=" + Ogre::StringConverter::toString(this->edgeCameraPosition) + " clampedX=" + Ogre::StringConverter::toString(this->edgeCameraClampedX) +
                                                                                          " window=" + Ogre::StringConverter::toString(this->edgeOrthoWindow));
                    }

                    this->camera->setProjectionType(Ogre::PT_ORTHOGRAPHIC);

                    this->camera->setOrthoWindow(this->edgeOrthoWindow.x, this->edgeOrthoWindow.y);
                }
                else
                {
                    if (true == lastOrthographicState)
                    {
                        Ogre::LogManager::getSingleton().logMessage(Ogre::LML_NORMAL,
                            "[FollowCamera2D][Ortho] END player=" + Ogre::StringConverter::toString(this->edgePlayerPosition) + " camera=" + Ogre::StringConverter::toString(this->edgeCameraPosition));
                    }

                    this->camera->setProjectionType(Ogre::PT_PERSPECTIVE);
                }

                lastOrthographicState = this->edgeOrthographicActive;
            };

            NOWA::GraphicsModule::getInstance()->updateTrackedClosure("FollowCamera2D::edgeOrthographic", closureFunction, false);
        }
    }

    void FollowCamera2D::moveCamera(Ogre::Real dt)
    {
        if (Ogre::Vector3::ZERO == this->mostRightUp)
        {
            return;
        }

        if (nullptr == this->sceneNode)
        {
            return;
        }

        if (nullptr == this->camera)
        {
            return;
        }

        if (true == this->firstTimeMoveValueSet)
        {
            this->lastMoveValue = Ogre::Vector3::ZERO;

            Ogre::Vector3 targetNodePosition = this->sceneNode->_getDerivedPositionUpdated();

            const Ogre::Vector3 initialPosition = targetNodePosition + this->offset;

            GraphicsModule::getInstance()->setCameraTransform(this->camera, initialPosition, this->camera->getOrientation());

            this->trackedCameraPosition = initialPosition;

            this->firstTimeMoveValueSet = false;
        }

        Ogre::Vector3 playerPosition;

        if (nullptr != this->physicsBody)
        {
            playerPosition = this->physicsBody->getPosition();
        }
        else
        {
            playerPosition = this->sceneNode->getPosition();
        }

        if (nullptr == this->camera)
        {
            return;
        }

        const Ogre::Vector3 cameraPosition = this->trackedCameraPosition;

        Ogre::Vector3 velocity = Ogre::Vector3::ZERO;

        if (playerPosition.x + this->offset.x - this->mostRightUp.x > this->minimumBounds.x && playerPosition.x + this->offset.x + this->mostRightUp.x < this->maximumBounds.x)
        {
            velocity.x = playerPosition.x - cameraPosition.x + this->offset.x;

            if (Ogre::Math::RealEqual(velocity.x, 0.0f))
            {
                velocity.x = 0.0f;
            }
        }

        if (playerPosition.y + this->offset.y - this->mostRightUp.y > this->minimumBounds.y && playerPosition.y + this->offset.y + this->mostRightUp.y < this->maximumBounds.y)
        {
            velocity.y = playerPosition.y - cameraPosition.y + this->offset.y;

            if (Ogre::Math::RealEqual(velocity.y, 0.0f))
            {
                velocity.y = 0.0f;
            }
        }

        velocity.x = NOWA::MathHelper::getInstance()->lowPassFilter(velocity.x, this->lastMoveValue.x, this->smoothValue);

        velocity.y = NOWA::MathHelper::getInstance()->lowPassFilter(velocity.y, this->lastMoveValue.y, this->smoothValue);

        this->lastMoveValue = velocity;

        Ogre::Vector3 finalPosition = cameraPosition + (velocity * this->moveCameraWeight);

        bool cameraClampedX = false;
        bool cameraClampedY = false;

        if (finalPosition.x + this->mostRightUp.x > this->maximumBounds.x)
        {
            finalPosition.x = this->maximumBounds.x - this->mostRightUp.x;

            cameraClampedX = true;
        }
        else if (finalPosition.x - this->mostRightUp.x < this->minimumBounds.x)
        {
            finalPosition.x = this->minimumBounds.x + this->mostRightUp.x;

            cameraClampedX = true;
        }

        if (finalPosition.y + this->mostRightUp.y > this->maximumBounds.y)
        {
            finalPosition.y = this->maximumBounds.y - this->mostRightUp.y;

            cameraClampedY = true;
        }
        else if (finalPosition.y - this->mostRightUp.y < this->minimumBounds.y)
        {
            finalPosition.y = this->minimumBounds.y + this->mostRightUp.y;

            cameraClampedY = true;
        }

        const Ogre::Real cameraMinX = this->minimumBounds.x + this->mostRightUp.x;

        const Ogre::Real cameraMaxX = this->maximumBounds.x - this->mostRightUp.x;

        const Ogre::Real cameraEdgeTolerance = 1.0f;

        if (Ogre::Math::Abs(finalPosition.x - cameraMinX) <= cameraEdgeTolerance || Ogre::Math::Abs(finalPosition.x - cameraMaxX) <= cameraEdgeTolerance)
        {
            cameraClampedX = true;
        }

        GraphicsModule::getInstance()->updateCameraPosition(this->camera, finalPosition);

        this->trackedCameraPosition = finalPosition;

        this->updateEdgeOrthographic(playerPosition, finalPosition, cameraClampedX, cameraClampedY);
    }

    void FollowCamera2D::rotateCamera(Ogre::Real dt, bool forJoyStick)
    {
    }

    Ogre::Vector3 FollowCamera2D::getPosition(void)
    {
        return this->camera->getPosition();
    }

    Ogre::Quaternion FollowCamera2D::getOrientation(void)
    {
        return this->camera->getOrientation();
    }

}; // namespace end
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
    namespace
    {
        // Attention: the closure id MUST be unique per instance. It used to be the constant
        // string "FollowCamera2D::moveCamera", shared by every FollowCamera2D that ever exists.
        // Two instances then fight over the same slot: registering in one overwrites the other,
        // and the destructor of one removes the closure belonging to the other. That is exactly
        // what happens on a stop/start cycle, where the old behavior is deleted while a new one
        // is already being created. The instance address is unique for as long as the object
        // lives, which is precisely the lifetime the closure has to match.
        Ogre::String buildMoveCameraClosureId(const FollowCamera2D* instance)
        {
            return "FollowCamera2D::moveCamera_" + Ogre::StringConverter::toString(reinterpret_cast<size_t>(instance));
        }

        // Attention: removeTrackedClosure() is ASYNCHRONOUS when called from the logic thread.
        // It only posts a removal command which the render thread processes at its next safe
        // point. CameraManager::removeCameraBehavior() does
        //
        //     cameraBehavior->onClearData();
        //     delete cameraBehavior;
        //
        // with nothing in between, so the render thread can still execute the closure - and
        // therefore dereference 'this' - after the object has already been freed. That is the
        // crash: it disappears as soon as the closure is not registered at all.
        //
        // Routing the removal through enqueueAndWait puts us ON the render thread, where
        // removeTrackedClosure() takes its direct path, and blocks until the closure is
        // provably gone. Only then may the object be destroyed.
        void removeMoveCameraClosureBlocking(const Ogre::String& closureId)
        {
            NOWA::GraphicsModule::RenderCommand removeCommand = [closureId]()
            {
                NOWA::GraphicsModule::getInstance()->removeTrackedClosure(closureId);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(removeCommand), "FollowCamera2D::removeMoveCameraClosure");
        }
    }

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
        pDebugLine(nullptr)
    {
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &FollowCamera2D::handleUpdateBounds), EventDataBoundsUpdated::getStaticEventType());
    }

    FollowCamera2D::~FollowCamera2D()
    {
        // Attention: this must be the FIRST statement and it must block. Everything below
        // starts tearing the object down, and the closure captures 'this'.
        removeMoveCameraClosureBlocking(buildMoveCameraClosureId(this));

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

        // Drop any closure left over from a previous activation before moveCamera() registers
        // a fresh one, so the two can never overlap.
        removeMoveCameraClosureBlocking(buildMoveCameraClosureId(this));
    }

    void FollowCamera2D::onClearData(void)
    {
        BaseCamera::onClearData();

        // Attention: CameraManager::removeCameraBehavior() calls this immediately before
        // 'delete cameraBehavior'. The removal therefore has to be finished when we return,
        // not merely queued - see removeMoveCameraClosureBlocking().
        removeMoveCameraClosureBlocking(buildMoveCameraClosureId(this));
    }

    void FollowCamera2D::setOffset(const Ogre::Vector3& offset)
    {
        this->offset = offset;
        this->firstTimeMoveValueSet = true;
    }

    void FollowCamera2D::handleUpdateBounds(NOWA::EventDataPtr eventData)
    {
        // When a new game object has been added to the scene, update the bounds for follow camera 2D
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
        this->minimumBounds = minimumBounds /* + this->borderOffset*/;
        this->maximumBounds = maximumBounds /* - this->borderOffset*/;

        Ogre::LogManager::getSingleton().logMessage(Ogre::LML_NORMAL, "[FollowCamera2D] minimum bounds: " + Ogre::StringConverter::toString(this->minimumBounds) + "maximum bounds: " + Ogre::StringConverter::toString(this->maximumBounds));

        if (nullptr == this->camera)
        {
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[FollowCamera2D] Error: Cannot set bounds because the camera does not exist yet. Please call first CameraManager->addCameraBehavior(...)!");
            throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[FollowCamera2D] Error: Cannot set bounds because the camera does not exist yet. Please call first CameraManager->addCameraBehavior(...)!\n", "NOWA");
        }
        // BUGFIX: this used to be (viewMatrix * corners[4] / 2.5f) / aspectRatio - corners[4] is
        // the FAR clip plane (Ogre fills getWorldSpaceCorners() with the near plane's four corners
        // at indices 0-3, then the far plane's four at 4-7), so this value scaled directly with
        // FarClipDistance (500 in this scene). FarClipDistance is a rendering/culling setting; it
        // has nothing to do with how wide the camera's view actually is at the 2D PLAY PLANE, which
        // is the only distance this "half extent" is meant to describe. The "2.5 is exactly the
        // value" comment was an empirically-found constant that happened to cancel out the far-clip
        // scaling for whatever FOV/far-clip combination it was tuned against - it does not hold in
        // general, and did not hold here: back-computing from the logged positions gave
        // mostRightUp.x =~ 148, LARGER than this level's entire ~111-unit width. That crosses the
        // two clamp targets in moveCamera() (minimumBounds.x + mostRightUp.x ends up bigger than
        // maximumBounds.x - mostRightUp.x), and the position then ping-pongs between those two
        // impossible values depending on which clamp branch fires - exactly the two-value flicker
        // seen in testing, and fully explained without needing to involve the render thread at all.
        //
        // Replaced with the direct, analytic formula for what is actually wanted: half the visible
        // height at a given distance is distance * tan(fovy / 2); half the visible width is that
        // times the aspect ratio. The distance that matters is how far the camera actually sits from
        // the 2D play plane the followed scene node lives on - which is exactly |offset.z|, since
        // offset is expressed along the camera's own view axis. This depends on nothing but FOVy,
        // aspect ratio and the offset the designer already set, so it stays correct however FOVy,
        // aspect ratio or FarClipDistance are configured, with no empirical constant involved.
        const Ogre::Real distanceToPlayPlane = Ogre::Math::Abs(this->offset.z);
        const Ogre::Radian halfFovY = this->camera->getFOVy() * 0.5f;
        const Ogre::Real halfHeight = distanceToPlayPlane * Ogre::Math::Tan(halfFovY);
        const Ogre::Real halfWidth = halfHeight * this->camera->getAspectRatio();

        this->mostRightUp = Ogre::Vector3(halfWidth, halfHeight, 0.0f);

        Ogre::LogManager::getSingleton().logMessage(Ogre::LML_NORMAL,
            "[FollowCamera2D] mostRightUp (half-extent at play plane, distance=" + Ogre::StringConverter::toString(distanceToPlayPlane) + "): " + Ogre::StringConverter::toString(this->mostRightUp));

        // BUGFIX: this subtracted borderOffset.z from BOTH mostRightUp.x and mostRightUp.y,
        // ignoring borderOffset.x and borderOffset.y entirely. Invisible with the constructor's
        // default borderOffset (50, 0, 0) - .z is 0, so the line was a no-op - but set any nonzero
        // borderOffset.x/.y via setBorderOffset() and it had no effect at all, while a nonzero .z
        // would shrink both axes together instead of the one it presumably names.
        this->mostRightUp -= Ogre::Vector3(this->borderOffset.x, this->borderOffset.y, 0.0f);
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
                }
            }
            else
            {
                // Check if the game object should be always shown,
                // if this is the case create ray scene query to throw a ray and check if the player will always be hit
                // hide all game objects that are in front of the player
                this->raySceneQuery = this->sceneManager->createRayQuery(Ogre::Ray());
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(oceanRdCmd), "FollowCamera2D::alwaysShowGameObject");
    }

    void FollowCamera2D::setSceneNode(Ogre::SceneNode* sceneNode)
    {
        this->sceneNode = sceneNode;
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

        // Attention: the camera can be gone while this behavior object is still alive.
        // CameraComponent::setActivatedFlag(false) calls CameraManager::removeCamera(), and
        // WorkspaceBaseComponent::removeWorkspace() runs in the same render command. Writing
        // through a dead camera below would be a use-after-free.
        if (nullptr == this->camera)
        {
            return;
        }

        if (true == this->firstTimeMoveValueSet)
        {
            this->lastMoveValue = Ogre::Vector3::ZERO;

            // Single write, position only, current orientation preserved - see the header comment
            // on trackedCameraPosition, and the class-wide BUGFIX note below, for why this used to
            // be two conflicting writes and why orientation must never come from the scene node.
            Ogre::Vector3 targetNodePosition = this->sceneNode->_getDerivedPositionUpdated();
            const Ogre::Vector3 initialPosition = targetNodePosition + this->offset;
            GraphicsModule::getInstance()->setCameraTransform(this->camera, initialPosition, this->camera->getOrientation());

            // BUGFIX: this class used to have no memory of where it had last told the camera to be,
            // and read this->camera->getPosition() back instead whenever it needed that value. That
            // read is answered by the RENDER thread's copy of the camera, which lags the LOGIC
            // thread - where moveCamera() runs - by however many frames the render queue is behind.
            // trackedCameraPosition is this class's own, immediately-consistent record instead; see
            // its full explanation further down where it is actually used for the first time.
            this->trackedCameraPosition = initialPosition;

            this->firstTimeMoveValueSet = false;
        }

        // Read directly from the physics body — NOT from the SceneNode.
        // SceneNode is updated by the render thread one frame later.
        Ogre::Vector3 playerPosition;
        if (nullptr != this->physicsBody)
        {
            playerPosition = this->physicsBody->getPosition();
        }
        else
        {
            playerPosition = this->sceneNode->getPosition();
        }

        // Always use closure functions in update functions to prevent graphical flickering
        auto closureFunction = [this, playerPosition](Ogre::Real renderDt)
        {
            // Attention: a persistent closure keeps running on the render thread until it is
            // explicitly removed. The camera may be destroyed in between - re-check on every
            // execution, not just when the closure is registered.
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

            // BUGFIX: bounds clamping used to be up to FOUR separate updateCameraPosition() calls in
            // this one function - the main movement write, then an X clamp, then a Y clamp, each of
            // them queued independently and each computed from a DIFFERENT position snapshot:
            //   - The main write used `cameraPosition` (this frame's starting point) plus `velocity`.
            //   - The X clamp reused the SAME `cameraPosition` for the axes it was not correcting -
            //     meaning if it fired, it reset Y and Z back to where they were BEFORE this frame's
            //     movement, silently discarding whatever Y motion the main write had just computed.
            //   - The Y clamp did the same to X, and read this->camera->getPosition().y freshly rather
            //     than using `cameraPosition.y` at all - a THIRD independent, and possibly differently
            //     stale, read of the camera's position within the same function call.
            //   - All four were queued render commands; whichever one the render thread happened to
            //     apply LAST for a given frame silently won, with no ordering guarantee visible from
            //     this thread.
            //
            // Folded into one vector, adjusted in place, with exactly ONE write issued at the end - a
            // clamp on one axis can no longer step on a movement just computed for another, and there
            // is only ever one answer to "where does the camera end up this frame".
            Ogre::Vector3 finalPosition = cameraPosition + (velocity * this->moveCameraWeight);

            if (finalPosition.x + this->mostRightUp.x > this->maximumBounds.x)
            {
                finalPosition.x = this->maximumBounds.x - this->mostRightUp.x;
            }
            else if (finalPosition.x - this->mostRightUp.x < this->minimumBounds.x)
            {
                finalPosition.x = this->minimumBounds.x + this->mostRightUp.x;
            }

            if (finalPosition.y + this->mostRightUp.y > this->maximumBounds.y)
            {
                finalPosition.y = this->maximumBounds.y - this->mostRightUp.y;
            }
            else if (finalPosition.y - this->mostRightUp.y < this->minimumBounds.y)
            {
                finalPosition.y = this->minimumBounds.y + this->mostRightUp.y;
            }

            // Even closure is used and we are already on render thread, never the less this interpolation method still must be used to prevent graphical object jitter!
            GraphicsModule::getInstance()->updateCameraPosition(this->camera, finalPosition);
            this->trackedCameraPosition = finalPosition;
        };

        NOWA::GraphicsModule::getInstance()->updateTrackedClosure(buildMoveCameraClosureId(this), closureFunction);
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

    // void FollowCamera2D::followGameObject(const Ogre::Vector3& position, const Ogre::Vector3& offset, const Ogre::Vector3& direction, Ogre::Real dt)
    //{
    //	Ogre::Vector3 cameraPosition = position;

    //	if (this->firstTimeMoveValueSet)
    //	{
    //		this->lastMoveValue = position;
    //		this->camera->moveRelative(offset);
    //		this->firstTimeMoveValueSet = false;
    //	}

    //	cameraPosition.x = NOWA::MathHelper::getInstance()->lowPassFilter(cameraPosition.x, this->lastMoveValue.x, this->smoothValue);
    //	cameraPosition.y = NOWA::MathHelper::getInstance()->lowPassFilter(cameraPosition.y, this->lastMoveValue.y, this->smoothValue);
    //	cameraPosition.z = NOWA::MathHelper::getInstance()->lowPassFilter(cameraPosition.z, this->lastMoveValue.z, this->smoothValue);

    //	Ogre::Vector3 velocity = position - this->lastMoveValue;

    //	this->camera->moveRelative(velocity);
    //	// this->camera->setDirection(direction);

    //	this->lastMoveValue = position;
    //}

}; // namespace end
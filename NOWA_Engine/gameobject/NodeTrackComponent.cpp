#include "NOWAPrecompiled.h"
#include "NodeTrackComponent.h"
#include "CameraComponent.h"
#include "GameObjectController.h"
#include "NodeComponent.h"
#include "main/AppStateManager.h"
#include "modules/LuaScriptApi.h"
#include "utilities/XMLConverter.h"

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    NodeTrackComponent::NodeTrackComponent() :
        GameObjectComponent(),
        activated(new Variant(NodeTrackComponent::AttrActivated(), true, this->attributes)),
        animation(nullptr),
        animationTrack(nullptr),
        animationState(nullptr),
        trackingActive(false),
        endOfPathReached(false),
        lastTimePosition(0.0f)
    {
        std::vector<Ogre::String> interpolationModes{"Spline", "Linear"};
        this->interpolationMode = new Variant(NodeTrackComponent::AttrInterpolationMode(), interpolationModes, this->attributes);

        std::vector<Ogre::String> rotationModes{"Linear", "Spherical"};
        this->rotationMode = new Variant(NodeTrackComponent::AttrRotationMode(), rotationModes, this->attributes);

        this->repeat = new Variant(NodeTrackComponent::AttrRepeat(), true, this->attributes);

        this->reverse = new Variant(NodeTrackComponent::AttrReverse(), false, this->attributes);

        this->autoOrientation = new Variant(NodeTrackComponent::AttrAutoOrientation(), false, this->attributes);

        this->nodeTrackCount = new Variant(NodeTrackComponent::AttrNodeTrackCount(), 0, this->attributes);

        // Since when node track count is changed, the whole properties must be refreshed, so that new field may come for node tracks
        this->nodeTrackCount->addUserData(GameObject::AttrActionNeedRefresh());
    }

    NodeTrackComponent::~NodeTrackComponent()
    {
    }

    bool NodeTrackComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
        {
            this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "InterpolationMode")
        {
            this->interpolationMode->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RotationMode")
        {
            this->rotationMode->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Repeat")
        {
            this->repeat->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Reverse")
        {
            // FIX/FEATURE: direct setValue(), deliberately NOT setReverse() - the persisted
            // NodeTrackId values were already written out in whatever order reflects any
            // prior reversal (writeXML() saves the CURRENT positional values), so calling
            // the real setReverse() here would flip an already-correctly-ordered list back
            // to front. Same reasoning as loading "Activated" via setValue() instead of
            // setActivated() above.
            this->reverse->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AutoOrientation")
        {
            // Direct setValue(), same reasoning as Reverse just above: this is a plain data
            // load, not a live edit - setAutoOrientation() has no side effect of its own to
            // avoid here (unlike setReverse(), it never reorders anything), but going
            // through the real setter would be inconsistent with how every other flag on
            // this component is loaded.
            this->autoOrientation->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "NodeTrackCount")
        {
            this->nodeTrackCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data", 1));
            propertyElement = propertyElement->next_sibling("property");
        }

        // Only create new variant, if fresh loading. If snapshot is done, no new variant
        // must be created! Because the algorithm is working changed flag of each existing variant!
        if (this->nodeTrackIds.size() < this->nodeTrackCount->getUInt())
        {
            this->nodeTrackIds.resize(this->nodeTrackCount->getUInt());
            this->timePositions.resize(this->nodeTrackCount->getUInt());
        }

        for (size_t i = 0; i < this->nodeTrackIds.size(); i++)
        {
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "NodeTrackId" + Ogre::StringConverter::toString(i))
            {
                if (nullptr == this->nodeTrackIds[i])
                {
                    this->nodeTrackIds[i] = new Variant(NodeTrackComponent::AttrNodeTrackId() + Ogre::StringConverter::toString(i), XMLConverter::getAttribUnsignedLong(propertyElement, "data", 0), this->attributes);
                }
                else
                {
                    this->nodeTrackIds[i]->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
                }
                propertyElement = propertyElement->next_sibling("property");
            }
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TimePosition" + Ogre::StringConverter::toString(i))
            {
                if (nullptr == this->timePositions[i])
                {
                    this->timePositions[i] = new Variant(NodeTrackComponent::AttrTimePosition() + Ogre::StringConverter::toString(i), XMLConverter::getAttribReal(propertyElement, "data", 1.0f), this->attributes);
                }
                else
                {
                    this->timePositions[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
                }
                propertyElement = propertyElement->next_sibling("property");
                this->timePositions[i]->addUserData(GameObject::AttrActionSeparator());
            }
        }
        return true;
    }

    GameObjectCompPtr NodeTrackComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        NodeTrackCompPtr clonedCompPtr(boost::make_shared<NodeTrackComponent>());

        clonedCompPtr->setActivated(this->activated->getBool());
        clonedCompPtr->setNodeTrackCount(this->nodeTrackCount->getUInt());

        for (unsigned int i = 0; i < static_cast<unsigned int>(this->nodeTrackIds.size()); i++)
        {
            clonedCompPtr->setNodeTrackId(i, this->nodeTrackIds[i]->getULong());
            clonedCompPtr->setTimePosition(i, this->timePositions[i]->getReal());
        }

        clonedCompPtr->setInterpolationMode(this->interpolationMode->getListSelectedValue());
        clonedCompPtr->setRotationMode(this->rotationMode->getListSelectedValue());
        clonedCompPtr->setRepeat(this->repeat->getBool());

        // FIX/FEATURE: direct member access, deliberately NOT setReverse() - the
        // nodeTrackIds copied just above already reflect this component's CURRENT order
        // (already reversed, if this->reverse is true). Calling the real setReverse()
        // here would detect a change on the freshly constructed clone (default false) and
        // flip that already-correct order a second time.
        clonedCompPtr->reverse->setValue(this->reverse->getBool());

        clonedCompPtr->setAutoOrientation(this->autoOrientation->getBool());

        clonedGameObjectPtr->addComponent(clonedCompPtr);
        clonedCompPtr->setOwner(clonedGameObjectPtr);

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
        return clonedCompPtr;
    }

    bool NodeTrackComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[NodeTrackComponent] Init node track component for game object: " + this->gameObjectPtr->getName());

        // this->animationTrack->setUseShortestRotationPath

        return true;
    }

    void NodeTrackComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[NodeTrackComponent] Destructor node track component for game object: " + this->gameObjectPtr->getName());
        this->camera = nullptr;

        // Defensive: normally already removed via disconnect()/onRemoveComponent(),
        // but never leave a stale closure referencing a soon-to-be-destroyed `this`
        // registered on the render thread.
        Ogre::String trackedClosureId = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
        NOWA::GraphicsModule::getInstance()->removeTrackedClosure(trackedClosureId);

        if (nullptr != this->animation)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
            {
                this->animation->destroyAllNodeTracks();
                this->gameObjectPtr->getSceneManager()->destroyAnimation(this->animation->getName());

                // Already destroyed
                // this->gameObjectPtr->getSceneManager()->destroyAnimationState(this->animationState->getAnimationName());
                this->animation = nullptr;
                this->animationTrack = nullptr;
                this->animationState = nullptr;
            };
            // Blocking: this runs inside the destructor, right before `this` is
            // deallocated - the lambda must finish before we return here.
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "NodeTrackComponent::~NodeTrackComponent");
        }
    }

    bool NodeTrackComponent::connect(void)
    {
        GameObjectComponent::connect();

        this->setActivated(this->activated->getBool());

        return true;
    }

    void NodeTrackComponent::buildAndActivateAnimation(void)
    {
        if (true == this->timePositions.empty())
        {
            return;
        }

        NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
        {
            const Ogre::Real totalAnimationLength = this->timePositions[this->nodeTrackIds.size() - 1]->getReal();

            if (nullptr != this->animation)
            {
                this->animation->destroyAllNodeTracks();
                this->gameObjectPtr->getSceneManager()->destroyAnimation(this->animation->getName());
                this->animation = nullptr;
                this->animationTrack = nullptr;
            }

            // FEATURE: if this GameObject also has a CameraComponent, the actual
            // on-screen camera is NOT reliably driven by this SceneNode - an
            // ACTIVE camera is moved directly (camera behavior -> Ogre::Camera),
            // and CameraComponent::update() deliberately never writes node ->
            // camera in that case (see the big comment in CameraComponent::
            // createCamera() - avoiding that write is what prevents gizmo
            // jitter). So animating just the node here would move nothing
            // visible for an active camera. Resolve the real Ogre::Camera* once
            // here and push the animated transform onto it directly every frame
            // in update() below, the same way BaseCamera::moveCamera/rotateCamera
            // already do via GraphicsModule::updateCameraPosition/-Orientation.
            this->camera = nullptr;
            const auto cameraCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<CameraComponent>());
            if (nullptr != cameraCompPtr)
            {
                this->camera = cameraCompPtr->getCamera();
            }

            // FIX/FEATURE (Option 1): if the sequence's total length collapses to
            // (effectively) 0 - a single waypoint configured at time 0 - there is
            // nothing to interpolate. Treat this as a genuine instant snap
            // instead: set the final transform directly, build no animation/
            // animationState at all.
            if (totalAnimationLength <= 0.0001f)
            {
                GameObjectPtr lastWaypointGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->nodeTrackIds[this->nodeTrackIds.size() - 1]->getULong());
                if (nullptr != lastWaypointGameObjectPtr)
                {
                    auto nodeCompPtr = NOWA::makeStrongPtr(lastWaypointGameObjectPtr->getComponent<NodeComponent>());
                    if (nullptr != nodeCompPtr)
                    {
                        if (nullptr != this->camera)
                        {
                            this->camera->setPosition(nodeCompPtr->getPosition());
                            // Orientation stays whatever the camera/object already has -
                            // never the waypoint marker's own (often arbitrary) rotation.
                        }
                        else
                        {
                            this->gameObjectPtr->getSceneNode()->setPosition(nodeCompPtr->getPosition());
                        }
                    }
                }
                return;
            }

            this->animation = this->gameObjectPtr->getSceneManager()->createAnimation("Path" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()) + "_" + Ogre::StringConverter::toString(this->index), totalAnimationLength);

            // Create a node track for animation
            this->animationTrack = this->animation->createNodeTrack(this->gameObjectPtr->getSceneNode());

            /*
            Values are interpolated along straight lines.
                IM_LINEAR,
            Values are interpolated along a spline, resulting in smoother changes in direction.
                IM_SPLINE
            */

            if ("Linear" == this->interpolationMode->getListSelectedValue())
            {
                this->animation->setInterpolationMode(Ogre::v1::Animation::IM_LINEAR);
            }
            else if ("Spline" == this->interpolationMode->getListSelectedValue())
            {
                this->animation->setInterpolationMode(Ogre::v1::Animation::IM_SPLINE);
            }

            /* Values are interpolated linearly. This is faster but does not necessarily give a completely accurate result.
                RIM_LINEAR,
                 Values are interpolated spherically. This is more accurate but has a higher cost.
                RIM_SPHERICAL
            */
            if ("Linear" == this->rotationMode->getListSelectedValue())
            {
                this->animation->setRotationInterpolationMode(Ogre::v1::Animation::RIM_LINEAR);
            }
            else if ("Spherical" == this->rotationMode->getListSelectedValue())
            {
                this->animation->setRotationInterpolationMode(Ogre::v1::Animation::RIM_SPHERICAL);
            }

            // Counts the keyframes that will ACTUALLY be created, not the number of
            // configured waypoints - those differ. The synthetic start keyframe only
            // exists when the first waypoint sits after t=0, and waypoints whose game
            // object cannot be resolved contribute nothing. Two waypoints with the first
            // one at t=0 therefore produced exactly TWO keyframes, while this guard,
            // counting waypoints, saw "2" and happily allowed spline mode.
            //
            // Ogre's spline needs at least three keyframes to derive meaningful tangents;
            // with two it extrapolates sideways instead of running straight between them.
            const bool needsSyntheticStartKeyFrame = (this->timePositions[0]->getReal() > 0.0001f);
            unsigned int expectedKeyFrameCount = needsSyntheticStartKeyFrame ? 1u : 0u;

            for (size_t i = 0; i < this->nodeTrackIds.size(); i++)
            {
                GameObjectPtr countWaypointGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->nodeTrackIds[i]->getULong());
                if (nullptr == countWaypointGameObjectPtr)
                {
                    continue;
                }
                if (nullptr == NOWA::makeStrongPtr(countWaypointGameObjectPtr->getComponent<NodeComponent>()))
                {
                    continue;
                }
                expectedKeyFrameCount++;
            }

            if (expectedKeyFrameCount < 3u)
            {
                this->animation->setRotationInterpolationMode(Ogre::v1::Animation::RIM_LINEAR);
                this->animation->setInterpolationMode(Ogre::v1::Animation::IM_LINEAR);
            }

            this->animationState = this->gameObjectPtr->getSceneManager()->createAnimationState("Path" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()) + "_" + Ogre::StringConverter::toString(this->index));
            this->animationState->setLoop(this->repeat->getBool());

            const Ogre::Vector3 travellingObjectScale = this->gameObjectPtr->getSceneNode()->getScale();
            // FIX: keep the travelling object's OWN orientation constant across
            // every keyframe, instead of rotating to match each waypoint
            // marker's own orientation (those markers were placed purely for
            // position reference, never with a deliberate rotation in mind).
            const Ogre::Quaternion travellingObjectOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();

            // ── AutoOrientation ──────────────────────────────────────────────
            // Precomputes one rotation PER KEYFRAME that will actually be created below,
            // gathered in the exact same order and with the exact same skip rules (missing
            // game object / missing NodeComponent) as the two keyframe-creation blocks
            // further down - so orientationKeyFrameRotations[i] always corresponds to the
            // i-th keyframe actually created, without having to re-resolve anything twice.
            //
            // Precomputing first, THEN creating keyframes, is what lets keyframe i look
            // toward keyframe i+1's position: that position is not known yet at the point
            // keyframe i itself gets created in the loop below.
            std::vector<Ogre::Vector3> orientationKeyFramePositions;
            std::vector<Ogre::Quaternion> orientationKeyFrameRotations;

            if (true == this->autoOrientation->getBool())
            {
                if (this->timePositions[0]->getReal() > 0.0001f)
                {
                    orientationKeyFramePositions.push_back(this->gameObjectPtr->getSceneNode()->getPosition());
                }

                for (size_t i = 0; i < this->nodeTrackIds.size(); i++)
                {
                    GameObjectPtr orientationWaypointGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->nodeTrackIds[i]->getULong());
                    if (nullptr == orientationWaypointGameObjectPtr)
                    {
                        continue;
                    }

                    auto orientationNodeCompPtr = NOWA::makeStrongPtr(orientationWaypointGameObjectPtr->getComponent<NodeComponent>());
                    if (nullptr == orientationNodeCompPtr)
                    {
                        continue;
                    }

                    orientationKeyFramePositions.push_back(orientationNodeCompPtr->getPosition());
                }
            }

            if (false == orientationKeyFramePositions.empty())
            {
                orientationKeyFrameRotations.resize(orientationKeyFramePositions.size(), travellingObjectOrientation);

                // The axis this component treats as "forward" when auto-orienting. Ogre's
                // own camera convention looks down local -Z by default, which matters here
                // because this component's first-class use case (see the class description)
                // is driving an actual Ogre::Camera (this->camera above). If a non-camera
                // mesh is authored facing a different local axis, this is the one place to
                // change.
                const Ogre::Vector3 forwardAxis = this->gameObjectPtr->getDefaultDirection();

                // Starting fallback direction, used only if even the FIRST segment turns out
                // to be zero-length (two waypoints at the same spot) - keeps the object at
                // whatever it was already facing rather than producing an undefined rotation.
                Ogre::Vector3 lastValidDirection = travellingObjectOrientation * forwardAxis;

                for (size_t i = 0; i < orientationKeyFramePositions.size(); i++)
                {
                    Ogre::Vector3 direction;
                    if (i + 1 < orientationKeyFramePositions.size())
                    {
                        direction = orientationKeyFramePositions[i + 1] - orientationKeyFramePositions[i];
                    }
                    else if (i > 0)
                    {
                        // Last keyframe: nothing ahead to look toward - keep facing the
                        // direction of the final leg instead of snapping to an undefined
                        // orientation.
                        direction = orientationKeyFramePositions[i] - orientationKeyFramePositions[i - 1];
                    }
                    else
                    {
                        // Only one keyframe total - nothing to derive a direction from at all.
                        direction = lastValidDirection;
                    }

                    if (direction.squaredLength() > 0.0001f)
                    {
                        direction.normalise();
                        lastValidDirection = direction;
                    }
                    else
                    {
                        // Zero-length segment (two waypoints at the same spot, or a
                        // synthetic start keyframe sitting exactly on the first waypoint) -
                        // no direction can be derived from it, so keep facing whatever
                        // direction was last valid.
                        direction = lastValidDirection;
                    }

                    // Note: if a path doubles straight back on itself, direction ends up
                    // exactly opposite forwardAxis (a 180 degree turn), for which Ogre's
                    // getRotationTo() has to pick an arbitrary perpendicular roll axis since
                    // infinitely many are equally valid - a real, unavoidable ambiguity of a
                    // true reversal, not a bug in this computation.
                    orientationKeyFrameRotations[i] = forwardAxis.getRotationTo(direction);
                }
            }

            // Only add a synthetic "current position" start keyframe when there
            // is an actual gap before the first configured waypoint - if the
            // first waypoint is already at time 0, IT is the t=0 keyframe;
            // adding another one at the exact same time would create two
            // competing keyframes at the same instant.
            // Indexes orientationKeyFrameRotations in lockstep with the keyframes actually
            // created below - both loops resolve waypoints with the identical skip rules, in
            // the identical order, so this always lines up.
            size_t orientationIndex = 0;

            if (this->timePositions[0]->getReal() > 0.0001f)
            {
                Ogre::v1::TransformKeyFrame* startKeyFrame = this->animationTrack->createNodeKeyFrame(0.0f);
                startKeyFrame->setTranslate(this->gameObjectPtr->getSceneNode()->getPosition());
                startKeyFrame->setRotation(true == this->autoOrientation->getBool() && orientationIndex < orientationKeyFrameRotations.size() ? orientationKeyFrameRotations[orientationIndex] : travellingObjectOrientation);
                startKeyFrame->setScale(travellingObjectScale);
                orientationIndex++;
            }

            for (size_t i = 0; i < this->nodeTrackIds.size(); i++)
            {
                // Bug: the keyframe used to be created BEFORE checking whether the
                // waypoint could be resolved at all. An unresolvable one - a freshly added
                // row still at id 0, or a deleted waypoint - then left a keyframe with the
                // default translation (0,0,0) in the track, and the object flew off to the
                // world origin and back.
                GameObjectPtr waypointGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->nodeTrackIds[i]->getULong());
                if (nullptr == waypointGameObjectPtr)
                {
                    continue;
                }

                auto nodeCompPtr = NOWA::makeStrongPtr(waypointGameObjectPtr->getComponent<NodeComponent>());
                if (nullptr == nodeCompPtr)
                {
                    continue;
                }

                Ogre::v1::TransformKeyFrame* transformKeyFrame = this->animationTrack->createNodeKeyFrame(this->timePositions[i]->getReal());
                transformKeyFrame->setTranslate(nodeCompPtr->getPosition());
                transformKeyFrame->setRotation(true == this->autoOrientation->getBool() && orientationIndex < orientationKeyFrameRotations.size() ? orientationKeyFrameRotations[orientationIndex] : travellingObjectOrientation);
                transformKeyFrame->setScale(travellingObjectScale);
                orientationIndex++;
            }

            // A rebuilt animation starts over, so the end of path latch has to as well -
            // otherwise a restart would swallow the first arrival.
            this->endOfPathReached = false;
            this->lastTimePosition = 0.0f;

            this->animationState->setEnabled(true);
        };
        // Blocking: the caller (setActivated) may rely on this->animationState/
        // this->animation already existing right after this returns.
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "NodeTrackComponent::buildAndActivateAnimation");
    }

    bool NodeTrackComponent::disconnect(void)
    {
        GameObjectComponent::disconnect();
        // Never leave a dangling Ogre::Camera* around once we stop driving it -
        // the CameraComponent (and its underlying Ogre::Camera) may be destroyed
        // or recreated independently of this component's own lifecycle.
        this->camera = nullptr;

        Ogre::String trackedClosureId = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
        NOWA::GraphicsModule::getInstance()->removeTrackedClosure(trackedClosureId);

        // The end of path latch has to be cleared here too, not only when the animation is
        // rebuilt - otherwise a stop/start cycle would leave it set and swallow the first
        // arrival of the next run.
        this->endOfPathReached = false;
        this->lastTimePosition = 0.0f;

        if (nullptr != this->animationTrack)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
            {
                this->animationTrack->removeAllKeyFrames();
                // The animation state is created together with the track, but guard it
                // anyway - the two are separate pointers and a partial teardown would
                // otherwise dereference null here.
                if (nullptr != this->animationState)
                {
                    this->animationState->setEnabled(false);
                    this->animationState->setTimePosition(0.0f);
                }
            };
            // Blocking: disconnect() returns right after this, and callers expect
            // the animation to be fully torn down/reset by the time it does.
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "NodeTrackComponent::disconnect");
        }
        return true;
    }

    bool NodeTrackComponent::onCloned(void)
    {
        // Search for the prior id of the cloned game object and set the new id and set the new id, if not found set better 0, else the game objects may be corrupt!
        for (size_t i = 0; i < this->nodeTrackIds.size(); i++)
        {
            GameObjectPtr nodeTrackGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getClonedGameObjectFromPriorId(this->nodeTrackIds[i]->getULong());
            if (nullptr != nodeTrackGameObjectPtr)
            {
                this->nodeTrackIds[i]->setValue(nodeTrackGameObjectPtr->getId());
            }
            else
            {
                this->nodeTrackIds[i]->setValue(static_cast<unsigned long>(0));
            }
            // Since connect is called during cloning process, it does not make sense to process furher here, but only when simulation started!
        }
        return true;
    }

    void NodeTrackComponent::update(Ogre::Real dt, bool notSimulating)
    {
        Ogre::String trackedClosureId = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);

        if (true == notSimulating || nullptr == this->animationState || false == this->activated->getBool())
        {
            // Not supposed to be moving right now - make sure the render thread
            // stops calling addTime() for this component instead of leaving a
            // stale closure registered.
            NOWA::GraphicsModule::getInstance()->removeTrackedClosure(trackedClosureId);
            return;
        }

        // FIX: addTime() must run on the render thread with the real render-frame
        // delta, not here on the main/logic thread with dt - calling it directly
        // from update() (as before) could advance far faster than real elapsed
        // render time, racing through all keyframes almost immediately ("die
        // Kamera ist sofort am Ziel"). updateTrackedClosure() is refreshed every
        // tick here, same pattern as SpeechBubbleComponent's drawSpeechBubble.
        auto closureFunction = [this](Ogre::Real renderDt)
        {
            if (nullptr == this->animationState)
            {
                return;
            }

            this->animationState->addTime(renderDt);

            if (nullptr != this->camera)
            {
                Ogre::Vector3 worldPosition = this->gameObjectPtr->getSceneNode()->_getDerivedPositionUpdated();
                Ogre::Quaternion worldOrientation = this->gameObjectPtr->getSceneNode()->_getDerivedOrientationUpdated();

                NOWA::GraphicsModule::getInstance()->updateCameraPosition(this->camera, worldPosition);
                NOWA::GraphicsModule::getInstance()->updateCameraOrientation(this->camera, worldOrientation);
            }

            // End of path detection. "Has finished" is a STATE that stays true for every
            // following frame, so it is latched on the rising edge - otherwise the closure
            // would fire once per frame for as long as the object rests at the last node.
            const Ogre::Real timePosition = this->animationState->getTimePosition();
            const Ogre::Real animationLength = this->animationState->getLength();

            bool atEndNow = false;
            if (true == this->repeat->getBool())
            {
                // Looping: addTime() wraps the time position around, so a wrap marks a
                // completed lap.
                atEndNow = (timePosition < this->lastTimePosition);
            }
            else
            {
                atEndNow = (animationLength > 0.0f && timePosition >= animationLength - 0.0001f);
            }
            this->lastTimePosition = timePosition;

            const bool justReachedEnd = atEndNow && (false == this->endOfPathReached);
            this->endOfPathReached = atEndNow;

            if (false == justReachedEnd)
            {
                return;
            }

            // A finished, non repeating track deactivates ITSELF - that is the actual root
            // cause of the whole problem. Ogre applies EVERY enabled animation state that
            // targets this scene node and ADDS their translations together, so a track left
            // running at its end keeps contributing its last keyframe forever. Measured in
            // the diagnostics log: a finished track sat at "time: 4 / 4 enabled: true" on
            // its final waypoint (23.9149, 4.45382, -16) while the next one was running, and
            // the node reported (47.8298, 8.90764, -32) - exactly the sum of both.
            //
            // Going through setActivated(false) rather than just disabling the animation
            // state keeps the 'activated' attribute in sync: it used to stay true after the
            // path had finished, so isActivated() lied to lua and the editor.
            //
            // Deferred to the logic thread, because this closure runs on the render thread
            // while setActivated() touches the variant and the tracked closure registry.
            if (false == this->repeat->getBool())
            {
                boost::weak_ptr<GameObjectComponent> weakThis = this->shared_from_this();

                NOWA::AppStateManager::LogicCommand logicCommand = [this, weakThis]()
                {
                    boost::shared_ptr<GameObjectComponent> strongThis = weakThis.lock();
                    if (nullptr == strongThis)
                    {
                        return;
                    }

                    this->setActivated(false);
                };
                NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
            }

            if (false == this->endOfPathClosureFunction.is_valid())
            {
                return;
            }

            // This closure runs on the RENDER thread, and lua must only ever be touched
            // from the logic thread, so the call is deferred. The closure object itself is
            // deliberately NOT copied - disconnect() clears it, and the is_valid() check
            // inside the command is what notices a teardown. The weak pointer covers the
            // other case: the component being destroyed rather than merely disconnected.
            boost::weak_ptr<GameObjectComponent> weakThis = this->shared_from_this();

            NOWA::AppStateManager::LogicCommand logicCommand = [this, weakThis]()
            {
                boost::shared_ptr<GameObjectComponent> strongThis = weakThis.lock();
                if (nullptr == strongThis)
                {
                    return;
                }

                if (false == this->endOfPathClosureFunction.is_valid())
                {
                    return;
                }

                try
                {
                    luabind::call_function<void>(this->endOfPathClosureFunction, this->gameObjectPtr.get());
                }
                catch (luabind::error& error)
                {
                    luabind::object errorMsg(luabind::from_stack(error.state(), -1));
                    std::stringstream msg;
                    msg << errorMsg;

                    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[NodeTrackComponent] Caught error in 'reactOnEndOfPathReached' Error: " + Ogre::String(error.what()) + " details: " + msg.str());
                }
            };
            NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
        };
        NOWA::GraphicsModule::getInstance()->updateTrackedClosure(trackedClosureId, closureFunction, false);
    }

    void NodeTrackComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (NodeTrackComponent::AttrActivated() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
        }
        else if (NodeTrackComponent::AttrNodeTrackCount() == attribute->getName())
        {
            this->setNodeTrackCount(attribute->getUInt());
        }
        else if (NodeTrackComponent::AttrInterpolationMode() == attribute->getName())
        {
            this->setInterpolationMode(attribute->getListSelectedValue());
        }
        else if (NodeTrackComponent::AttrRotationMode() == attribute->getName())
        {
            this->setRotationMode(attribute->getListSelectedValue());
        }
        else if (NodeTrackComponent::AttrAutoOrientation() == attribute->getName())
        {
            this->setAutoOrientation(attribute->getBool());
        }
        else if (NodeTrackComponent::AttrRepeat() == attribute->getName())
        {
            this->setRepeat(attribute->getBool());
        }
        else if (NodeTrackComponent::AttrReverse() == attribute->getName())
        {
            this->setReverse(attribute->getBool());
        }
        else
        {
            for (unsigned int i = 0; i < static_cast<unsigned int>(this->nodeTrackIds.size()); i++)
            {
                if (NodeTrackComponent::AttrNodeTrackId() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->nodeTrackIds[i]->setValue(attribute->getULong());
                }
                else if (NodeTrackComponent::AttrTimePosition() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setTimePosition(i, attribute->getReal());
                }
            }
        }
    }

    void NodeTrackComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        // 2 = int
        // 6 = real
        // 7 = string
        // 8 = vector2
        // 9 = vector3
        // 10 = vector4 -> also quaternion
        // 12 = bool
        GameObjectComponent::writeXML(propertiesXML, doc);

        xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "InterpolationMode"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->interpolationMode->getListSelectedValue())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "RotationMode"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rotationMode->getListSelectedValue())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Repeat"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->repeat->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Reverse"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->reverse->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "AutoOrientation"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->autoOrientation->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "NodeTrackCount"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->nodeTrackCount->getUInt())));
        propertiesXML->append_node(propertyXML);

        for (size_t i = 0; i < this->nodeTrackIds.size(); i++)
        {
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "NodeTrackId" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->nodeTrackIds[i]->getULong())));
            propertiesXML->append_node(propertyXML);

            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "TimePosition" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->timePositions[i]->getReal())));
            propertiesXML->append_node(propertyXML);
        }
    }

    Ogre::String NodeTrackComponent::getClassName(void) const
    {
        return "NodeTrackComponent";
    }

    Ogre::String NodeTrackComponent::getParentClassName(void) const
    {
        return "GameObjectComponent";
    }

    void NodeTrackComponent::deactivateOtherNodeTracks(void)
    {
        if (nullptr == this->gameObjectPtr)
        {
            return;
        }

        for (size_t i = 0; i < this->gameObjectPtr->getComponents()->size(); i++)
        {
            // Working here with shared_ptrs is evil, because of bidirectional referecing
            auto component = std::get<COMPONENT>(this->gameObjectPtr->getComponents()->at(i)).get();
            if (component == this)
            {
                continue;
            }

            NodeTrackComponent* otherNodeTrackComponent = dynamic_cast<NodeTrackComponent*>(component);
            if (nullptr == otherNodeTrackComponent)
            {
                continue;
            }

            if (true == otherNodeTrackComponent->isActivated())
            {
                otherNodeTrackComponent->setActivated(false);
            }
        }
    }

    void NodeTrackComponent::setActivated(bool activated)
    {
        this->activated->setValue(activated);

        // setActivated() calls coming from XML/property deserialization at
        // scene LOAD time (before connect() ever ran, this->bConnected still
        // false) would try to touch this->animationState/build the animation
        // way too early.
        if (false == this->bConnected)
        {
            return;
        }

        if (false == activated)
        {
            if (nullptr != this->animationState)
            {
                this->animationState->setEnabled(false);
            }
            return;
        }

        // Switch off every OTHER NodeTrackComponent of this game object first.
        //
        // They all animate the one and same scene node, and Ogre applies EVERY enabled
        // animation state targeting that node, ADDING their translations together. A track
        // left running - either still mid-path or simply finished but never disabled -
        // therefore keeps contributing its last keyframe on top of the new one. Measured:
        // a finished track sitting on (23.9149, 4.45382, -16) plus a starting track on the
        // same waypoint put the object at (47.8298, 8.90764, -32), exactly the sum, which
        // looked like it shot off along a wrong axis.
        //
        // Doing it here rather than in lua, because a missing
        // "previousTrack:setActivated(false)" is far too easy to overlook in a script.
        this->deactivateOtherNodeTracks();

        // FIX: the whole animation (including its "current position" start
        // keyframe) is now built HERE, fresh, every time this component is
        // actually switched on - not once back in connect(). That's what
        // makes the start keyframe reflect wherever the object truly is RIGHT
        // NOW, instead of a stale snapshot from simulation start - critical
        // when multiple NodeTrackComponents on the same GameObject get
        // activated one after another (the second must continue from where
        // the first left off, not reset back to the object's original spot).
        this->buildAndActivateAnimation();
    }

    bool NodeTrackComponent::isActivated(void) const
    {
        return this->activated->getBool();
    }

    void NodeTrackComponent::reactOnEndOfPathReached(luabind::object closureFunction)
    {
        // Replacing, not appending: calling this repeatedly - e.g. from a script function
        // that runs every frame - leaves exactly one reaction registered.
        this->endOfPathClosureFunction = closureFunction;
    }

    void NodeTrackComponent::setNodeTrackCount(unsigned int nodeTrackCount)
    {
        this->nodeTrackCount->setValue(nodeTrackCount);

        size_t oldSize = this->nodeTrackIds.size();

        if (nodeTrackCount > oldSize)
        {
            // Resize the waypoints array for count
            this->nodeTrackIds.resize(nodeTrackCount);
            this->timePositions.resize(nodeTrackCount);

            for (size_t i = oldSize; i < this->nodeTrackIds.size(); i++)
            {
                this->nodeTrackIds[i] = new Variant(NodeTrackComponent::AttrNodeTrackId() + Ogre::StringConverter::toString(i), static_cast<unsigned long>(0), this->attributes, true);

                // Seeded from the PREVIOUS entry plus one second, not from the raw index.
                // Using the index meant a newly added waypoint always got exactly 'i'
                // seconds - so after adjusting waypoint 0 to, say, 5 seconds, adding
                // waypoint 1 handed it 1 second, i.e. a time BEFORE its predecessor, and
                // that segment ran backwards.
                Ogre::Real previousTimePosition = 0.0f;
                if (i > 0 && nullptr != this->timePositions[i - 1])
                {
                    previousTimePosition = this->timePositions[i - 1]->getReal();
                }

                this->timePositions[i] = new Variant(NodeTrackComponent::AttrTimePosition() + Ogre::StringConverter::toString(i), previousTimePosition + 1.0f, this->attributes);
                this->timePositions[i]->addUserData(GameObject::AttrActionSeparator());
            }
        }
        else if (nodeTrackCount < oldSize)
        {
            this->eraseVariants(this->nodeTrackIds, nodeTrackCount);
            this->eraseVariants(this->timePositions, nodeTrackCount);
        }
    }

    unsigned int NodeTrackComponent::getNodeTrackCount(void) const
    {
        return this->nodeTrackCount->getUInt();
    }

    void NodeTrackComponent::setNodeTrackId(unsigned int index, unsigned long id)
    {
        if (index >= this->nodeTrackIds.size())
        {
            index = static_cast<unsigned int>(this->nodeTrackIds.size()) - 1;
        }
        this->nodeTrackIds[index]->setValue(id);
    }

    unsigned long NodeTrackComponent::getNodeTrackId(unsigned int index)
    {
        if (index >= this->nodeTrackIds.size())
        {
            return 0;
        }
        return this->nodeTrackIds[index]->getULong();
    }

    void NodeTrackComponent::setTimePosition(unsigned int index, Ogre::Real timePosition)
    {
        // FIX: reverted the earlier "never <= 0" clamp - that was wrong for
        // multi-waypoint sequences, where the FIRST waypoint legitimately sits
        // at time 0 (reached instantly, then travel continues to the later
        // ones) as long as the LAST waypoint's time (which determines the
        // whole animation's length) is > 0. A time position of exactly 0 is
        // now a meaningful, intentional value - connect() treats an animation
        // whose total length collapses to 0 (i.e. the LAST waypoint is at
        // time 0) as an instant snap instead of forcing an artificial minimum
        // interpolation time.
        if (timePosition < 0.0f)
        {
            timePosition = 0.0f;
        }

        if (index >= this->timePositions.size())
        {
            index = static_cast<unsigned int>(this->timePositions.size()) - 1;
        }

        this->timePositions[index]->setValue(timePosition);
    }

    Ogre::Real NodeTrackComponent::getTimePosition(unsigned int index)
    {
        if (index >= this->timePositions.size())
        {
            return 0;
        }
        return this->timePositions[index]->getReal();
    }

    void NodeTrackComponent::setInterpolationMode(const Ogre::String& interpolationMode)
    {
        this->interpolationMode->setListSelectedValue(interpolationMode);
        if (nullptr != this->animation)
        {
            if ("Spline" == this->interpolationMode->getListSelectedValue())
            {
                this->animation->setInterpolationMode(Ogre::v1::Animation::IM_LINEAR);
            }
            else if ("Spline" == this->interpolationMode->getListSelectedValue())
            {
                this->animation->setInterpolationMode(Ogre::v1::Animation::IM_SPLINE);
            }
        }
    }

    Ogre::String NodeTrackComponent::getInterpolationMode(void) const
    {
        return this->interpolationMode->getListSelectedValue();
    }

    void NodeTrackComponent::setRotationMode(const Ogre::String& rotationMode)
    {
        this->rotationMode->setListSelectedValue(rotationMode);
        if (nullptr != this->animation)
        {
            if ("Linear" == this->rotationMode->getListSelectedValue())
            {
                this->animation->setRotationInterpolationMode(Ogre::v1::Animation::RIM_LINEAR);
            }
            else if ("Spherical" == this->rotationMode->getListSelectedValue())
            {
                this->animation->setRotationInterpolationMode(Ogre::v1::Animation::RIM_SPHERICAL);
            }
        }
    }

    Ogre::String NodeTrackComponent::getRotationMode(void) const
    {
        return this->rotationMode->getListSelectedValue();
    }

    void NodeTrackComponent::setRepeat(bool repeat)
    {
        this->repeat->setValue(repeat);
    }

    bool NodeTrackComponent::getRepeat(void) const
    {
        return this->repeat->getBool();
    }

    void NodeTrackComponent::setReverse(bool reverse)
    {
        // Only actually reorder on a genuine toggle. Reversal happens immediately, in
        // place, right here - not lazily when the animation is next built - so calling
        // this twice with the same value must NOT flip the order back and forth. Guards
        // against e.g. Lua re-applying the same value, or actualizeValue() firing more than
        // once for the same edit.
        if (reverse == this->reverse->getBool())
        {
            return;
        }

        this->reverse->setValue(reverse);

        // Swap the VALUES between symmetric index pairs (0 <-> last, 1 <-> second-last,
        // ...) rather than reversing the pointer array itself. Each Variant's own name
        // ("Node Track Id 0", "Node Track Id 1", ...) is meant to stay fixed to its
        // position - writeXML()/init() both address an entry by loop index, not by the
        // Variant's internal name - so swapping pointers instead of values would leave
        // every Variant's displayed name mismatched with the slot it now occupies.
        //
        // Only the waypoint ids are reordered, NOT the time positions: TimePosition[i]
        // describes "arrival time from the start of the animation for whichever waypoint
        // sits at index i", independent of that waypoint's identity. Leaving the time
        // positions untouched means the configured pacing (how long each leg of the
        // journey takes) stays exactly as authored, just walked in the opposite order -
        // the waypoint that used to be last is now reached first, at whatever time
        // position used to belong to index 0, and so on.
        const size_t count = this->nodeTrackIds.size();
        for (size_t i = 0; i < count / 2; i++)
        {
            const unsigned long temp = this->nodeTrackIds[i]->getULong();
            this->nodeTrackIds[i]->setValue(this->nodeTrackIds[count - 1 - i]->getULong());
            this->nodeTrackIds[count - 1 - i]->setValue(temp);
        }
    }

    bool NodeTrackComponent::getReverse(void) const
    {
        return this->reverse->getBool();
    }

    void NodeTrackComponent::setAutoOrientation(bool autoOrientation)
    {
        // Plain data setter, same convention as setReverse(): takes effect on the next
        // (re-)build. Nothing to reorder or patch live here - the actual keyframe rotations
        // it affects only get computed inside buildAndActivateAnimation().
        this->autoOrientation->setValue(autoOrientation);
    }

    bool NodeTrackComponent::getAutoOrientation(void) const
    {
        return this->autoOrientation->getBool();
    }

    Ogre::v1::Animation* NodeTrackComponent::getAnimation(void) const
    {
        return this->animation;
    }

    Ogre::v1::NodeAnimationTrack* NodeTrackComponent::getAnimationTrack(void) const
    {
        return this->animationTrack;
    }

    // -----------------------------------------------------------------------------------
    // Lua registration part
    // -----------------------------------------------------------------------------------

    NodeTrackComponent* getNodeTrackComponent(GameObject* gameObject, unsigned int occurrenceIndex)
    {
        return NOWA::makeStrongPtr(gameObject->getComponentWithOccurrence<NodeTrackComponent>(occurrenceIndex)).get();
    }

    NodeTrackComponent* getNodeTrackComponent(GameObject* gameObject)
    {
        return NOWA::makeStrongPtr(gameObject->getComponent<NodeTrackComponent>()).get();
    }

    NodeTrackComponent* getNodeTrackComponentFromName(GameObject* gameObject, const Ogre::String& name)
    {
        return NOWA::makeStrongPtr(gameObject->getComponentFromName<NodeTrackComponent>(name)).get();
    }

    // A node track id is a GameObject id - too large to round-trip safely through a Lua
    // number - so, like TagPointComponent::setSourceId, it is exposed to Lua as a String
    // and converted here, rather than binding NodeTrackComponent::setNodeTrackId/
    // getNodeTrackId (unsigned long) directly.
    void setNodeTrackIdForLua(NodeTrackComponent* instance, unsigned int index, const Ogre::String& trackId)
    {
        instance->setNodeTrackId(index, Ogre::StringConverter::parseUnsignedLong(trackId));
    }

    Ogre::String getNodeTrackIdForLua(NodeTrackComponent* instance, unsigned int index)
    {
        return Ogre::StringConverter::toString(instance->getNodeTrackId(index));
    }

    void NodeTrackComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
    {
        luabind::module(lua)[luabind::class_<NodeTrackComponent, GameObjectComponent>("NodeTrackComponent")
                .def("setActivated", &NodeTrackComponent::setActivated)
                .def("isActivated", &NodeTrackComponent::isActivated)
                .def("setNodeTrackCount", &NodeTrackComponent::setNodeTrackCount)
                .def("getNodeTrackCount", &NodeTrackComponent::getNodeTrackCount)
                .def("setNodeTrackId", &setNodeTrackIdForLua)
                .def("getNodeTrackId", &getNodeTrackIdForLua)
                .def("setTimePosition", &NodeTrackComponent::setTimePosition)
                .def("getTimePosition", &NodeTrackComponent::getTimePosition)
                .def("setInterpolationMode", &NodeTrackComponent::setInterpolationMode)
                .def("getInterpolationMode", &NodeTrackComponent::getInterpolationMode)
                .def("setRotationMode", &NodeTrackComponent::setRotationMode)
                .def("getRotationMode", &NodeTrackComponent::getRotationMode)
                .def("setRepeat", &NodeTrackComponent::setRepeat)
                .def("getRepeat", &NodeTrackComponent::getRepeat)
                .def("setReverse", &NodeTrackComponent::setReverse)
                .def("getReverse", &NodeTrackComponent::getReverse)
                .def("setAutoOrientation", &NodeTrackComponent::setAutoOrientation)
                .def("getAutoOrientation", &NodeTrackComponent::getAutoOrientation)
                .def("reactOnEndOfPathReached", &NodeTrackComponent::reactOnEndOfPathReached)];

        LuaScriptApi::getInstance()->addClassToCollection("NodeTrackComponent", "class inherits GameObjectComponent", NodeTrackComponent::getStaticInfoText());
        LuaScriptApi::getInstance()->addClassToCollection("NodeTrackComponent", "void setActivated(bool activated)", "Sets whether this node track is activated or not.");
        LuaScriptApi::getInstance()->addClassToCollection("NodeTrackComponent", "bool isActivated()", "Gets whether this node track is activated or not.");
        LuaScriptApi::getInstance()->addClassToCollection("NodeTrackComponent", "void setNodeTrackCount(unsigned int nodeTrackCount)", "Sets the node track count (how many nodes are used for the tracking).");
        LuaScriptApi::getInstance()->addClassToCollection("NodeTrackComponent", "number getNodeTrackCount()", "Gets the node track count.");
        LuaScriptApi::getInstance()->addClassToCollection("NodeTrackComponent", "void setNodeTrackId(unsigned int index, String id)",
            "Sets the node track id for the given index in the node track list with @nodeTrackCount elements. Note: The order is controlled by the index, from which node to which node this game object will be tracked. If 'Reverse' is set, "
            "index 0 refers to the waypoint that was configured LAST, since the list is reordered immediately when Reverse is set.");
        LuaScriptApi::getInstance()->addClassToCollection("NodeTrackComponent", "String getNodeTrackId(unsigned int index)", "Gets node track id from the given node track index from list.");
        LuaScriptApi::getInstance()->addClassToCollection("NodeTrackComponent", "void setTimePosition(unsigned int index, float timePosition)",
            "Sets time position in milliseconds after which this game object should be tracked at the node from the given index.");
        LuaScriptApi::getInstance()->addClassToCollection("NodeTrackComponent", "float getTimePosition(unsigned int index)", "Gets time position in milliseconds for the node with the given index.");
        LuaScriptApi::getInstance()->addClassToCollection("NodeTrackComponent", "void setInterpolationMode(String interpolationMode)", "Sets the curve interpolation mode how the game object will be moved. Possible values are: 'Spline', 'Linear'");
        LuaScriptApi::getInstance()->addClassToCollection("NodeTrackComponent", "String getInterpolationMode()", "Gets the curve interpolation mode how the game object is moved. Possible values are: 'Spline', 'Linear'");
        LuaScriptApi::getInstance()->addClassToCollection("NodeTrackComponent", "void setRotationMode(String rotationMode)", "Sets the rotation mode how the game object will be rotated during movement. Possible values are: 'Linear', 'Spherical'");
        LuaScriptApi::getInstance()->addClassToCollection("NodeTrackComponent", "String getRotationMode(void)", "Gets the rotation mode how the game object is rotated during movement. Possible values are: 'Linear', 'Spherical'");
        LuaScriptApi::getInstance()->addClassToCollection("NodeTrackComponent", "void setRepeat(bool repeat)", "Sets whether the path is played over and over again. If disabled, the game object stops at the last node.");
        LuaScriptApi::getInstance()->addClassToCollection("NodeTrackComponent", "bool getRepeat()", "Gets whether the path is played over and over again.");
        LuaScriptApi::getInstance()->addClassToCollection("NodeTrackComponent", "void setReverse(bool reverse)",
            "Sets whether the waypoint order should be reversed: the waypoint that was configured LAST is reached FIRST, and the whole path is walked back to front. Takes effect IMMEDIATELY - the underlying waypoint id list is reordered "
            "right away, synchronously, not lazily when the animation is next built. Calling this again with the value it already has is a no-op, it will NOT flip the order back. Only the waypoint ids are reordered, the configured "
            "TimePosition of each index is left untouched, so the already authored pacing (how long each leg takes) is kept, just walked in the opposite direction. Typical usage from Lua: setReverse(true) followed by setActivated(true), "
            "which then (re-)builds the animation from the now reordered ids.");
        LuaScriptApi::getInstance()->addClassToCollection("NodeTrackComponent", "bool getReverse()", "Gets whether the waypoint order is currently reversed.");
        LuaScriptApi::getInstance()->addClassToCollection("NodeTrackComponent", "void setAutoOrientation(bool autoOrientation)",
            "Sets whether the travelling game object should be rotated to face the direction it is currently moving in, instead of keeping its own fixed orientation for the whole path. At each waypoint it faces the NEXT waypoint; the final "
            "waypoint keeps facing the direction of the last leg. The existing 'Rotation Mode' property still controls how smoothly the turns are blended. Takes effect on the next (re-)build - typical usage from Lua is "
            "setAutoOrientation(true) followed by setActivated(true).");
        LuaScriptApi::getInstance()->addClassToCollection("NodeTrackComponent", "bool getAutoOrientation()", "Gets whether the travelling game object is rotated to face its direction of travel.");
        LuaScriptApi::getInstance()->addClassToCollection("NodeTrackComponent", "void reactOnEndOfPathReached(func closureFunction)",
            "Sets the closure function which is called when the LAST node of the path has been reached. The closure receives the game object as parameter. With 'Repeat' enabled it fires on every completed lap. Calling this again replaces the "
            "previous closure, so it is safe to call from a function that runs every frame.");

        gameObjectClass.def("getNodeTrackComponentFromName", &getNodeTrackComponentFromName);
        gameObjectClass.def("getNodeTrackComponent", (NodeTrackComponent * (*)(GameObject*)) & getNodeTrackComponent);
        // If its desired to create several of this components for one game object
        gameObjectClass.def("getNodeTrackComponent2", (NodeTrackComponent * (*)(GameObject*, unsigned int)) & getNodeTrackComponent);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "NodeTrackComponent getNodeTrackComponent2(unsigned int occurrenceIndex)",
            "Gets the component by the given occurence index, since a game object may have this component several times.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "NodeTrackComponent getNodeTrackComponent()", "Gets the component. This can be used if the game object has this component just once.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "NodeTrackComponent getNodeTrackComponentFromName(String name)", "Gets the component from name.");

        gameObjectControllerClass.def("castNodeTrackComponent", &GameObjectController::cast<NodeTrackComponent>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "NodeTrackComponent castNodeTrackComponent(NodeTrackComponent other)", "Casts an incoming type from function for lua auto completion.");
    }

}; // namespace end
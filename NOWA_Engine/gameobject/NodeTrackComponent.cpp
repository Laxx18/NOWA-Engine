#include "NOWAPrecompiled.h"
#include "NodeTrackComponent.h"
#include "CameraComponent.h"
#include "NodeComponent.h"
#include "main/AppStateManager.h"
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

            // Only add a synthetic "current position" start keyframe when there
            // is an actual gap before the first configured waypoint - if the
            // first waypoint is already at time 0, IT is the t=0 keyframe;
            // adding another one at the exact same time would create two
            // competing keyframes at the same instant.
            if (this->timePositions[0]->getReal() > 0.0001f)
            {
                Ogre::v1::TransformKeyFrame* startKeyFrame = this->animationTrack->createNodeKeyFrame(0.0f);
                startKeyFrame->setTranslate(this->gameObjectPtr->getSceneNode()->getPosition());
                startKeyFrame->setRotation(travellingObjectOrientation);
                startKeyFrame->setScale(travellingObjectScale);
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
                transformKeyFrame->setRotation(travellingObjectOrientation);
                transformKeyFrame->setScale(travellingObjectScale);
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
        else if (NodeTrackComponent::AttrRepeat() == attribute->getName())
        {
            this->setRepeat(attribute->getBool());
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

    Ogre::v1::Animation* NodeTrackComponent::getAnimation(void) const
    {
        return this->animation;
    }

    Ogre::v1::NodeAnimationTrack* NodeTrackComponent::getAnimationTrack(void) const
    {
        return this->animationTrack;
    }

}; // namespace end
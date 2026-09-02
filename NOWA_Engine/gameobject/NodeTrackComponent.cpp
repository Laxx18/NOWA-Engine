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

    NodeTrackComponent::NodeTrackComponent()
        : GameObjectComponent(),
        activated(new Variant(NodeTrackComponent::AttrActivated(), true, this->attributes)),
        animation(nullptr),
        animationTrack(nullptr),
        animationState(nullptr),
        trackingActive(false)
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
        Ogre::String trackedClosureId = this->gameObjectPtr->getName() + this->getClassName() + "::update";
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
        if (true == this->timePositions.empty())
        {
            return true;
        }

        NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
        {
            if (nullptr != this->animation)
            {
                this->animation->destroyAllNodeTracks();
                this->gameObjectPtr->getSceneManager()->destroyAnimation(this->animation->getName());
            }

            this->animation = this->gameObjectPtr->getSceneManager()->createAnimation("Path" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()), this->timePositions[this->nodeTrackIds.size() - 1]->getReal());

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

            this->animationState = this->gameObjectPtr->getSceneManager()->createAnimationState("Path" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()));
            this->animationState->setLoop(this->repeat->getBool());

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

            // FIX/FEATURE: the object's CURRENT transform at connect/activation
            // time is always the effective first waypoint - without this, Ogre's
            // NodeAnimationTrack has no keyframe before timePositions[0] and simply
            // clamps to that first configured waypoint's transform the instant
            // addTime() runs, causing a hard snap/jump instead of a smooth
            // transition from wherever the object actually is. Inserting a t=0
            // keyframe with the live scene-node transform fixes that: playback now
            // always starts by smoothly interpolating from "here" to the first
            // configured waypoint (assuming timePositions[0] > 0 - if it's 0, this
            // keyframe and the first configured one just coincide, which is harmless).
            Ogre::v1::TransformKeyFrame* startKeyFrame = this->animationTrack->createNodeKeyFrame(0.0f);
            startKeyFrame->setTranslate(this->gameObjectPtr->getSceneNode()->getPosition());
            startKeyFrame->setRotation(this->gameObjectPtr->getSceneNode()->getOrientation());
            startKeyFrame->setScale(this->gameObjectPtr->getSceneNode()->getScale());

            for (size_t i = 0; i < this->nodeTrackIds.size(); i++)
            {
                Ogre::v1::TransformKeyFrame* transformKeyFrame = this->animationTrack->createNodeKeyFrame(this->timePositions[i]->getReal());
                GameObjectPtr waypointGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->nodeTrackIds[i]->getULong());
                if (nullptr != waypointGameObjectPtr)
                {
                    auto nodeCompPtr = NOWA::makeStrongPtr(waypointGameObjectPtr->getComponent<NodeComponent>());
                    if (nullptr != nodeCompPtr)
                    {
                        transformKeyFrame->setTranslate(nodeCompPtr->getPosition());
                        transformKeyFrame->setRotation(nodeCompPtr->getOrientation());
                        transformKeyFrame->setScale(nodeCompPtr->getOwner()->getScale());
                    }
                }
            }

            if (true == this->activated->getBool())
            {
                this->animationState->setEnabled(true);
            }
        };
        // Blocking: connect() returns right after this and callers/other
        // components may immediately rely on this->animationState/this->animation
        // already existing (e.g. setActivated() called right after connect()).
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "NodeTrackComponent::connect");

        return true;
    }

    bool NodeTrackComponent::disconnect(void)
    {
        // Never leave a dangling Ogre::Camera* around once we stop driving it -
        // the CameraComponent (and its underlying Ogre::Camera) may be destroyed
        // or recreated independently of this component's own lifecycle.
        this->camera = nullptr;

        Ogre::String trackedClosureId = this->gameObjectPtr->getName() + this->getClassName() + "::update";
        NOWA::GraphicsModule::getInstance()->removeTrackedClosure(trackedClosureId);

        if (nullptr != this->animationTrack)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
            {
                this->animationTrack->removeAllKeyFrames();
                this->animationState->setEnabled(false);
                this->animationState->setTimePosition(0.0f);
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
        Ogre::String trackedClosureId = this->gameObjectPtr->getName() + this->getClassName() + "::update";

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

    void NodeTrackComponent::setActivated(bool activated)
    {
        this->activated->setValue(activated);

        // FIX: this used to only store the flag - the actual movement is driven
        // by this->animationState->setEnabled(true)/addTime() in update(), and
        // that enable-call previously only ever happened once, inside connect(),
        // gated behind whatever this->activated was AT CONNECT TIME. Calling
        // setActivated(true) later (e.g. from a delayed Lua callback, after
        // connect() already ran with activated == false) changed the stored
        // flag but never actually enabled the AnimationState - addTime() on a
        // disabled state is a no-op, so nothing visibly moved despite waypoints
        // being present and update() running every frame.
        if (nullptr != this->animationState)
        {
            this->animationState->setEnabled(activated);
        }
    }

    bool NodeTrackComponent::isActivated(void) const
    {
        return this->activated->getBool();
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
                this->timePositions[i] = new Variant(NodeTrackComponent::AttrTimePosition() + Ogre::StringConverter::toString(i), static_cast<Ogre::Real>(i), this->attributes);
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
        if (index >= this->timePositions.size())
        {
            index = static_cast<unsigned int>(this->timePositions.size()) - 1;
        }

        Ogre::Real oldTimePosition = 0.0f;
        Ogre::Real newTimePosition = 0.0f;

        if (index > 0)
        {
            oldTimePosition = this->timePositions[index - 1]->getReal();
        }

        if (timePosition >= oldTimePosition)
        {
            newTimePosition = timePosition;
        }
        else
        {
            newTimePosition = oldTimePosition + 1;
        }

        this->timePositions[index]->setValue(newTimePosition);
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
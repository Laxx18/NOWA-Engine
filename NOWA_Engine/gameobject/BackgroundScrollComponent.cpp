#include "NOWAPrecompiled.h"
#include "BackgroundScrollComponent.h"
#include "GameObjectController.h"
#include "WorkspaceComponents.h"
#include "main/AppStateManager.h"
#include "main/Events.h"
#include "utilities/MathHelper.h"
#include "utilities/XMLConverter.h"

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    BackgroundScrollComponent::BackgroundScrollComponent() :
        GameObjectComponent(),
        targetSceneNode(nullptr),
        lastPosition(Ogre::Vector2::ZERO),
        pausedLastPosition(Ogre::Vector2::ZERO),
        firstTimePositionSet(true),
        xScroll(0.0f),
        yScroll(0.0f),
        lastVelocity(Ogre::Vector2::ZERO),
        workspaceBackgroundComponent(nullptr),
        active(new Variant(BackgroundScrollComponent::AttrActive(), true, this->attributes)),
        targetId(new Variant(BackgroundScrollComponent::AttrTargetId(), static_cast<unsigned long>(0), this->attributes, true)),
        moveSpeedX(new Variant(BackgroundScrollComponent::AttrMoveSpeedX(), 0.01f, this->attributes)),
        moveSpeedY(new Variant(BackgroundScrollComponent::AttrMoveSpeedY(), 0.0f, this->attributes)),
        followGameObjectX(new Variant(BackgroundScrollComponent::AttrFollowGameObjectX(), false, this->attributes)),
        followGameObjectY(new Variant(BackgroundScrollComponent::AttrFollowGameObjectY(), false, this->attributes)),
        followGameObjectZ(new Variant(BackgroundScrollComponent::AttrFollowGameObjectZ(), false, this->attributes)),
        backgroundName(new Variant(BackgroundScrollComponent::AttrBackgroundName(), Ogre::String("Sky.png"), this->attributes))
    {
        // moveSpeedX: far = 0.2, middle = 0.5, near = 1.0
        this->active->setDescription("Activates the background scroll operations.");
        this->targetId->setDescription("The target id for the game object the background should follow");
        this->moveSpeedX->setDescription("The move speed x. It can also be a negative value to change the x scroll direction. If set to 0 no x scrolling is done.");
        this->moveSpeedY->setDescription("The move speed y. It can also be a negative value to change the y scroll direction. If set to 0 no y scrolling is done.");
        this->followGameObjectX->setDescription("Whether to follow the game object's x position (if target id is valid).");
        this->followGameObjectY->setDescription("Whether to follow the game object's y position (if target id is valid).");
        this->followGameObjectZ->setDescription("Whether to follow the game object's z position instead of y (if target id is valid).");
        this->backgroundName->setDescription("The background far texture name. If empty, no background far will be rendered.");
        this->backgroundName->addUserData(GameObject::AttrActionFileOpenDialog(), "Backgrounds");
    }

    BackgroundScrollComponent::~BackgroundScrollComponent()
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[BackgroundScrollComponent] Destructor background scroll component for game object: " + this->gameObjectPtr->getName());

        this->workspaceBackgroundComponent = nullptr;
    }

    bool BackgroundScrollComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Active")
        {
            this->setActivated(XMLConverter::getAttribBool(propertyElement, "data", false));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetId")
        {
            this->targetId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MoveSpeedX")
        {
            this->moveSpeedX->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MoveSpeedY")
        {
            this->moveSpeedY->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FollowGameObjectX")
        {
            this->followGameObjectX->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FollowGameObjectY")
        {
            this->followGameObjectY->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FollowGameObjectZ")
        {
            this->followGameObjectZ->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BackgroundName")
        {
            this->backgroundName->setValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        return true;
    }

    bool BackgroundScrollComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[BackgroundScrollComponent] Init background scroll component for game object: " + this->gameObjectPtr->getName());

        return this->setupBackground();
    }

    bool BackgroundScrollComponent::setupBackground(void)
    {
        if (nullptr == this->workspaceBackgroundComponent)
        {
            auto workspaceBackgroundCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<WorkspaceBackgroundComponent>());
            if (nullptr != workspaceBackgroundCompPtr)
            {
                this->workspaceBackgroundComponent = workspaceBackgroundCompPtr.get();
                this->setBackgroundName(this->backgroundName->getString());
            }
            else
            {
                Ogre::String message = "[BackgroundScrollComponent] Could not get prior WorkspaceBackgroundComponent. This component must be set under the WorkspaceBackgroundComponent! Affected game object: " + this->gameObjectPtr->getName();
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, message);

                boost::shared_ptr<EventDataFeedback> eventDataFeedback(new EventDataFeedback(false, message));
                NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataFeedback);
                return false;
            }
        }
        return true;
    }

    GameObjectCompPtr BackgroundScrollComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        // No cloning, this component may only exist once per scene
        return nullptr;
    }

    void BackgroundScrollComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();

        this->workspaceBackgroundComponent = nullptr;
    }

    void BackgroundScrollComponent::onOtherComponentRemoved(unsigned int index)
    {
        auto gameObjectCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponentByIndex(index));
        if (nullptr != gameObjectCompPtr)
        {
            auto workspaceBackgroundCompPtr = boost::dynamic_pointer_cast<WorkspaceBackgroundComponent>(gameObjectCompPtr);
            if (nullptr != workspaceBackgroundCompPtr)
            {
                // Remove this component if prior workspace background component has been removed, because this component does not make any sense without workspace background component
                // this->workspaceBackgroundComponent = nullptr;
                this->gameObjectPtr->deleteComponent(this->getClassName());
            }
        }
    }

    bool BackgroundScrollComponent::connect(void)
    {
        GameObjectPtr targetGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->targetId->getULong());
        if (nullptr != targetGameObjectPtr)
        {
            targetGameObjectPtr->setDynamic(true);
            this->targetSceneNode = targetGameObjectPtr->getSceneNode();
        }

        bool result = this->setupBackground();

        // Force this layer's visible scroll position to a known, correct baseline (0) the
        // moment we connect, BEFORE the player sees a frame. Without this, whatever
        // speedsXValues[index]/speedsYValues[index] was left over from a PREVIOUS play session
        // (disconnect() only ever stopped the scroll VELOCITY, never reset the position - see
        // there) stayed on screen as the starting offset, and the first few update() ticks had to
        // visibly "catch up" from that stale offset - the "flies in from the right" symptom.
        if (nullptr != this->workspaceBackgroundComponent)
        {
            const int layerIndex = static_cast<int>(this->gameObjectPtr->getOccurrenceIndexFromComponent(this));
            if (layerIndex >= 0 && layerIndex < 9)
            {
                this->workspaceBackgroundComponent->resetBackgroundScrollPosition(static_cast<unsigned short>(layerIndex));
            }
        }

        this->firstTimePositionSet = true;

        return result;
    }

    bool BackgroundScrollComponent::disconnect(void)
    {
        this->firstTimePositionSet = true;
        this->targetSceneNode = nullptr;
        this->lastPosition = Ogre::Vector2::ZERO;
        this->pausedLastPosition = Ogre::Vector2::ZERO;
        this->xScroll = 0.0f;
        this->yScroll = 0.0f;
        this->lastVelocity = Ogre::Vector2::ZERO;

        if (nullptr == this->workspaceBackgroundComponent)
        {
            return true;
        }

        const int layerIndex = static_cast<int>(this->gameObjectPtr->getOccurrenceIndexFromComponent(this));
        if (layerIndex >= 0 && layerIndex < 9)
        {
            // setBackgroundScrollSpeedX/Y(i, 0.0f) only stops the scroll VELOCITY now, it no
            // longer resets the visible position (that comment was written for the old,
            // position-based version). Explicit reset instead, so the layer is actually back at
            // UV 0 on disconnect.
            this->workspaceBackgroundComponent->setBackgroundScrollSpeedX(static_cast<unsigned short>(layerIndex), 0.0f);
            this->workspaceBackgroundComponent->setBackgroundScrollSpeedY(static_cast<unsigned short>(layerIndex), 0.0f);
            this->workspaceBackgroundComponent->resetBackgroundScrollPosition(static_cast<unsigned short>(layerIndex));
        }

        return true;
    }

    void BackgroundScrollComponent::pause(void)
    {
        // Remember the background scroll position
        this->pausedLastPosition = this->lastPosition;
    }

    void BackgroundScrollComponent::resume(void)
    {
        // Set the remembered background scroll position
        this->lastPosition = this->pausedLastPosition;
    }

    void BackgroundScrollComponent::update(Ogre::Real dt, bool notSimulating)
    {
        if (false == notSimulating && true == this->active->getBool() && nullptr != this->workspaceBackgroundComponent && dt > 0.0f)
        {
            const int layerIndex = static_cast<int>(this->gameObjectPtr->getOccurrenceIndexFromComponent(this));
            if (layerIndex < 0 || layerIndex >= 9)
            {
                return;
            }

            const bool followX = this->followGameObjectX->getBool();
            const bool followY = this->followGameObjectY->getBool();
            const bool followZ = this->followGameObjectZ->getBool();

            const Ogre::Real signX = 1.0f;
            const Ogre::Real signY = -1.0f;

            Ogre::Real velocityX = 0.0f;
            Ogre::Real velocityY = 0.0f;

            if (true == followX || true == followY || true == followZ)
            {
                Ogre::Camera* camera = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
                if (nullptr != camera)
                {
                    const Ogre::Vector3 cameraPosition = camera->getPosition();
                    const Ogre::Real cameraHorizontal = cameraPosition.x;
                    const Ogre::Real cameraVertical = (true == followY) ? cameraPosition.y : cameraPosition.z;

                    const Ogre::Real maxPlausibleCameraDelta = 10.0f;
                    const bool isTeleport = true == this->firstTimePositionSet || Ogre::Math::Abs(cameraHorizontal - this->lastPosition.x) > maxPlausibleCameraDelta || Ogre::Math::Abs(cameraVertical - this->lastPosition.y) > maxPlausibleCameraDelta;

                    if (true == isTeleport)
                    {
                        this->lastPosition.x = cameraHorizontal;
                        this->lastPosition.y = cameraVertical;
                        // A teleport must also cut any smoothed velocity dead - otherwise the
                        // filter below would smoothly interpolate FROM whatever velocity it had
                        // TOWARD zero over a few frames, instead of just being zero immediately.
                        this->lastVelocity = Ogre::Vector2::ZERO;
                        this->firstTimePositionSet = false;
                    }
                    else
                    {
                        // Raw delta/dt - exactly 0 when the camera did not move, no filter here.
                        if (true == followX)
                        {
                            velocityX = signX * (cameraHorizontal - this->lastPosition.x) * this->moveSpeedX->getReal() / dt;
                        }
                        if (true == followY || true == followZ)
                        {
                            velocityY = signY * (cameraVertical - this->lastPosition.y) * this->moveSpeedY->getReal() / dt;
                        }
                        this->lastPosition.x = cameraHorizontal;
                        this->lastPosition.y = cameraVertical;
                    }
                }
            }

            if (false == followX)
            {
                velocityX = signX * this->moveSpeedX->getReal();
            }
            if (false == followY && false == followZ)
            {
                velocityY = signY * this->moveSpeedY->getReal();
            }

            // FIX: filter the VELOCITY itself, not the camera position. Velocity is legitimately
            // exactly 0 once the camera stops (raw delta/dt above), and filtering a signal that
            // has become exactly 0 decays smoothly to exactly 0 and stays there - a few smoothed
            // frames of taper-off, no permanent residual creep (unlike filtering position, which
            // chases a moving target and never fully arrives - that was the old bug).
            const Ogre::Real velocitySmoothingTimeSeconds = 0.05f;
            const Ogre::Real velocitySmoothing = 1.0f - std::exp(-dt / velocitySmoothingTimeSeconds);
            velocityX = NOWA::MathHelper::getInstance()->lowPassFilter(velocityX, this->lastVelocity.x, velocitySmoothing);
            velocityY = NOWA::MathHelper::getInstance()->lowPassFilter(velocityY, this->lastVelocity.y, velocitySmoothing);
            this->lastVelocity.x = velocityX;
            this->lastVelocity.y = velocityY;

            this->workspaceBackgroundComponent->setBackgroundScrollSpeedX(static_cast<unsigned short>(layerIndex), velocityX);
            this->workspaceBackgroundComponent->setBackgroundScrollSpeedY(static_cast<unsigned short>(layerIndex), velocityY);
        }
    }

    void BackgroundScrollComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (BackgroundScrollComponent::AttrActive() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
        }
        else if (BackgroundScrollComponent::AttrTargetId() == attribute->getName())
        {
            this->setTargetId(attribute->getULong());
        }
        else if (BackgroundScrollComponent::AttrMoveSpeedX() == attribute->getName())
        {
            this->setMoveSpeedX(attribute->getReal());
        }
        else if (BackgroundScrollComponent::AttrMoveSpeedY() == attribute->getName())
        {
            this->setMoveSpeedY(attribute->getReal());
        }
        else if (BackgroundScrollComponent::AttrFollowGameObjectX() == attribute->getName())
        {
            this->setFollowGameObjectX(attribute->getBool());
        }
        else if (BackgroundScrollComponent::AttrFollowGameObjectY() == attribute->getName())
        {
            this->setFollowGameObjectY(attribute->getBool());
        }
        else if (BackgroundScrollComponent::AttrFollowGameObjectZ() == attribute->getName())
        {
            this->setFollowGameObjectZ(attribute->getBool());
        }
        else if (BackgroundScrollComponent::AttrBackgroundName() == attribute->getName())
        {
            this->setBackgroundName(attribute->getString());
        }
    }

    void BackgroundScrollComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
        propertyXML->append_attribute(doc.allocate_attribute("name", "Active"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->active->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "TargetId"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->targetId->getULong())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "MoveSpeedX"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->moveSpeedX->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "MoveSpeedY"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->moveSpeedY->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "FollowGameObjectX"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->followGameObjectX->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "FollowGameObjectY"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->followGameObjectY->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "FollowGameObjectZ"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->followGameObjectZ->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "BackgroundName"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->backgroundName->getString())));
        propertiesXML->append_node(propertyXML);
    }

    void BackgroundScrollComponent::setActivated(bool activated)
    {
        this->active->setValue(activated);
    }

    Ogre::String BackgroundScrollComponent::getClassName(void) const
    {
        return "BackgroundScrollComponent";
    }

    Ogre::String BackgroundScrollComponent::getParentClassName(void) const
    {
        return "GameObjectComponent";
    }

    bool BackgroundScrollComponent::isActivated(void) const
    {
        return this->active->getBool();
    }

    void BackgroundScrollComponent::setTargetId(unsigned long targetId)
    {
        this->targetId->setValue(targetId);
        if (0 == targetId)
        {
            this->setupBackground();
        }
    }

    unsigned long BackgroundScrollComponent::getTargetId(void) const
    {
        return this->targetId->getULong();
    }

    void BackgroundScrollComponent::setMoveSpeedX(Ogre::Real moveSpeedX)
    {
        this->moveSpeedX->setValue(moveSpeedX);
    }

    Ogre::Real BackgroundScrollComponent::getMoveSpeedX(void) const
    {
        return this->moveSpeedX->getReal();
    }

    void BackgroundScrollComponent::setMoveSpeedY(Ogre::Real moveSpeedY)
    {
        this->moveSpeedY->setValue(moveSpeedY);
    }

    Ogre::Real BackgroundScrollComponent::getMoveSpeedY(void) const
    {
        return this->moveSpeedY->getReal();
    }

    void BackgroundScrollComponent::setFollowGameObjectX(bool followGameObjectX)
    {
        this->followGameObjectX->setValue(followGameObjectX);
    }

    bool BackgroundScrollComponent::getFollowGameObjectX(void) const
    {
        return this->followGameObjectX->getBool();
    }

    void BackgroundScrollComponent::setFollowGameObjectY(bool followGameObjectY)
    {
        this->followGameObjectY->setValue(followGameObjectY);
        if (true == followGameObjectY)
        {
            this->followGameObjectZ->setValue(false);
        }
    }

    bool BackgroundScrollComponent::getFollowGameObjectY(void) const
    {
        return this->followGameObjectY->getBool();
    }

    void BackgroundScrollComponent::setFollowGameObjectZ(bool followGameObjectZ)
    {
        this->followGameObjectZ->setValue(followGameObjectZ);
        if (true == followGameObjectZ)
        {
            this->followGameObjectY->setValue(false);
        }
    }

    bool BackgroundScrollComponent::getFollowGameObjectZ(void) const
    {
        return this->followGameObjectZ->getBool();
    }

    void BackgroundScrollComponent::setBackgroundName(const Ogre::String& backgroundName)
    {
        if (false == Ogre::ResourceGroupManager::getSingleton().resourceExistsInAnyGroup(backgroundName))
        {
            // Sent event with feedback
            boost::shared_ptr<EventDataFeedback> eventDataNavigationMeshFeedback(new EventDataFeedback(false, "#{TextureMissing}"));
            NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataNavigationMeshFeedback);
            return;
        }

        // Somehow textures with "_" do not work??? like Mountain_Near_2.png
        this->backgroundName->setValue(backgroundName);

        if (nullptr != workspaceBackgroundComponent)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this, backgroundName]()
            {
                for (unsigned short i = 0; i < 9; i++)
                {
                    if (i == this->gameObjectPtr->getOccurrenceIndexFromComponent(this))
                    {
                        this->workspaceBackgroundComponent->changeBackground(i, backgroundName);
                        break;
                    }
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "BackgroundScrollComponent::setBackgroundName");
        }
    }

    Ogre::String BackgroundScrollComponent::getBackgroundName(void) const
    {
        return this->backgroundName->getString();
    }

}; // namespace end
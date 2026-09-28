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

        return this->setupBackground();
    }

    bool BackgroundScrollComponent::disconnect(void)
    {
        this->firstTimePositionSet = true;
        this->targetSceneNode = nullptr;
        this->lastPosition = Ogre::Vector2::ZERO;
        this->pausedLastPosition = Ogre::Vector2::ZERO;
        this->xScroll = 0.0f;
        this->yScroll = 0.0f;

        if (nullptr == this->workspaceBackgroundComponent)
        {
            return true;
        }

        for (unsigned short i = 0; i < 9; i++)
        {
            // Set uv back to zero
            if (i == this->gameObjectPtr->getOccurrenceIndexFromComponent(this))
            {
                this->workspaceBackgroundComponent->setBackgroundScrollSpeedX(i, 0.0f);
                this->workspaceBackgroundComponent->setBackgroundScrollSpeedY(i, 0.0f);
                // Compiles the materials and its shaders again, so that default uv values are set (uv set to 0)
                break;
            }
        }

        // this->workspaceBackgroundComponent->compileBackgroundMaterial();
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
        if (false == notSimulating && true == this->active->getBool() && nullptr != this->workspaceBackgroundComponent)
        {
            // Resolve the layer this component drives ONCE, not per frame in a 9 iteration loop.
            const int layerIndex = static_cast<int>(this->gameObjectPtr->getOccurrenceIndexFromComponent(this));
            if (layerIndex < 0 || layerIndex >= 9)
            {
                return;
            }

            const bool followX = this->followGameObjectX->getBool();
            const bool followY = this->followGameObjectY->getBool();
            const bool followZ = this->followGameObjectZ->getBool();

            // Scroll direction per axis. If a layer moves WITH the camera instead of against it,
            // flip the sign here - this is the only place that decides the direction.
            const Ogre::Real signX = 1.0f;
            const Ogre::Real signY = -1.0f;

            if (true == followX || true == followY || true == followZ)
            {
                // Parallax is driven by how far the CAMERA moved. Camera stands still -> the layer
                // stands still, exactly.
                Ogre::Camera* camera = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
                if (nullptr != camera)
                {
                    const Ogre::Vector3 cameraPosition = camera->getPosition();
                    const Ogre::Real cameraHorizontal = cameraPosition.x;
                    // Vertical source: y for a side scroller, z for a top down layer.
                    const Ogre::Real cameraVertical = (true == followY) ? cameraPosition.y : cameraPosition.z;

                    // Camera teleports (FollowCamera2D placing the camera on the player at the start,
                    // level or camera switches) must not scroll the layer. A real camera cannot move
                    // this far in one update, so snap to the new position without scrolling.
                    const Ogre::Real maxPlausibleCameraDelta = 10.0f;

                    if (true == this->firstTimePositionSet || Ogre::Math::Abs(cameraHorizontal - this->lastPosition.x) > maxPlausibleCameraDelta || Ogre::Math::Abs(cameraVertical - this->lastPosition.y) > maxPlausibleCameraDelta)
                    {
                        this->lastPosition.x = cameraHorizontal;
                        this->lastPosition.y = cameraVertical;
                        this->firstTimePositionSet = false;
                    }

                    // FIX: the old alpha "0.1f * dt" is 0.1 per SECOND, i.e. a time constant of about
                    // 10 s: the layer needed seconds to react and kept drifting for 10-30 s after the
                    // camera had stopped. Frame rate independent alpha instead; smoothingTimeSeconds is
                    // the only value to tune (bigger = smoother but lags more, 0.05 - 0.15 is sane).
                    const Ogre::Real smoothingTimeSeconds = 0.08f;
                    const Ogre::Real smoothing = 1.0f - std::exp(-dt / smoothingTimeSeconds);

                    if (true == followX)
                    {
                        const Ogre::Real filteredX = NOWA::MathHelper::getInstance()->lowPassFilter(cameraHorizontal, this->lastPosition.x, smoothing);
                        // moveSpeed = uv units per world unit the camera moved. The delta is already
                        // per frame, so deliberately no "* dt" here.
                        this->xScroll += signX * (filteredX - this->lastPosition.x) * this->moveSpeedX->getReal();
                        this->xScroll = fmodf(this->xScroll + 2.0f, 2.0f); // Wrap between [0,2)
                        this->lastPosition.x = filteredX;
                    }

                    if (true == followY || true == followZ)
                    {
                        const Ogre::Real filteredY = NOWA::MathHelper::getInstance()->lowPassFilter(cameraVertical, this->lastPosition.y, smoothing);
                        this->yScroll += signY * (filteredY - this->lastPosition.y) * this->moveSpeedY->getReal();
                        this->yScroll = fmodf(this->yScroll + 2.0f, 2.0f); // Wrap between [0,2)
                        this->lastPosition.y = filteredY;
                    }
                }
            }

            // Axes that do NOT follow scroll on their own (clouds etc.): moveSpeed = uv units per
            // second, constant, no filter.
            if (false == followX)
            {
                this->xScroll += signX * this->moveSpeedX->getReal() * dt;
                this->xScroll = fmodf(this->xScroll + 2.0f, 2.0f);
            }
            if (false == followY && false == followZ)
            {
                this->yScroll += signY * this->moveSpeedY->getReal() * dt;
                this->yScroll = fmodf(this->yScroll + 2.0f, 2.0f);
            }

            this->workspaceBackgroundComponent->setBackgroundScrollSpeedX(static_cast<unsigned short>(layerIndex), this->xScroll);
            this->workspaceBackgroundComponent->setBackgroundScrollSpeedY(static_cast<unsigned short>(layerIndex), this->yScroll);
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
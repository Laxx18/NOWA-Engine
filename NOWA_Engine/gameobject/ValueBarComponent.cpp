#include "NOWAPrecompiled.h"
#include "ValueBarComponent.h"
#include "GameObjectController.h"
#include "GameObjectTitleComponent.h"
#include "LuaScriptComponent.h"
#include "main/AppStateManager.h"
#include "utilities/MathHelper.h"
#include "utilities/XMLConverter.h"

#include "RenderQueueEnums.h"

namespace
{
    // How fast the fill follows the value (exponential, per second). High = snappy.
    const Ogre::Real FILL_FOLLOW_SPEED = 18.0f;
    // The damage trail holds still this long after a hit, then shrinks towards the fill ...
    const Ogre::Real TRAIL_DELAY = 0.3f;
    // ... with this speed, in bar widths per second.
    const Ogre::Real TRAIL_SPEED = 1.2f;

    Ogre::ColourValue toColour(const Ogre::Vector3& color, Ogre::Real factor, Ogre::Real add)
    {
        return Ogre::ColourValue(std::min(color.x * factor + add, 1.0f), std::min(color.y * factor + add, 1.0f), std::min(color.z * factor + add, 1.0f), 1.0f);
    }
}

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    ValueBarComponent::ValueBarComponent() :
        GameObjectComponent(),
        lineNode(nullptr),
        manualObject(nullptr),
        indices(0),
        orientationTargetGameObject(nullptr),
        gameObjectTitleComponent(nullptr),
        couldDraw(false),
        activated(new Variant(ValueBarComponent::AttrActivated(), true, this->attributes)),
        twoSided(new Variant(ValueBarComponent::AttrTwoSided(), true, this->attributes)),
        innerColor(new Variant(ValueBarComponent::AttrInnerColor(), Ogre::Vector3(1.0f, 0.0f, 0.0f), this->attributes)),
        outerColor(new Variant(ValueBarComponent::AttrOuterColor(), Ogre::Vector3::UNIT_SCALE, this->attributes)),
        borderSize(new Variant(ValueBarComponent::AttrBorderSize(), 0.25f, this->attributes)),
        offsetPosition(new Variant(ValueBarComponent::AttrOffsetPosition(), Ogre::Vector3::ZERO, this->attributes)),
        offsetOrientation(new Variant(ValueBarComponent::AttrOffsetOrientation(), Ogre::Vector3::ZERO, this->attributes)),
        orientationTargetId(new Variant(ValueBarComponent::AttrOrientationTargetId(), static_cast<unsigned long>(0), this->attributes, true)),
        width(new Variant(ValueBarComponent::AttrWidth(), 10.0f, this->attributes)),
        height(new Variant(ValueBarComponent::AttrHeight(), 2.0f, this->attributes)),
        maxValue(new Variant(ValueBarComponent::AttrMaxValue(), static_cast<unsigned int>(100), this->attributes)),
        currentValue(new Variant(ValueBarComponent::AttrCurrentValue(), static_cast<unsigned int>(50), this->attributes)),
        faceCamera(new Variant(ValueBarComponent::AttrFaceCamera(), true, this->attributes)),
        displayedFraction(1.0f),
        trailFraction(1.0f),
        trailDelayTimer(0.0f),
        lastQuadCount(0)
    {
        this->faceCamera->setDescription("If set, the bar always faces the active camera and 'Offset Position' is applied in world space ('Offset Orientation' is ignored). "
                                         "An 'Orientation Target Id' still takes precedence.");
        this->innerColor->addUserData(GameObject::AttrActionColorDialog());
        this->outerColor->addUserData(GameObject::AttrActionColorDialog());
    }

    ValueBarComponent::~ValueBarComponent()
    {

    }

    bool ValueBarComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
        {
            this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TwoSided")
        {
            this->twoSided->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "InnerColor")
        {
            this->innerColor->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OuterColor")
        {
            this->outerColor->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BorderSize")
        {
            this->borderSize->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OffsetPosition")
        {
            this->offsetPosition->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OffsetOrientation")
        {
            this->offsetOrientation->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OrientationTargetId")
        {
            this->orientationTargetId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Width")
        {
            this->width->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Height")
        {
            this->height->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxValue")
        {
            this->maxValue->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CurrentValue")
        {
            this->currentValue->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        // Scenes saved before this attribute existed keep the constructor default (true).
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FaceCamera")
        {
            this->faceCamera->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        return true;
    }

    GameObjectCompPtr ValueBarComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        ValueBarCompPtr clonedCompPtr(boost::make_shared<ValueBarComponent>());

        clonedCompPtr->setTwoSided(this->twoSided->getBool());
        clonedCompPtr->setInnerColor(this->innerColor->getVector3());
        clonedCompPtr->setOuterColor(this->outerColor->getVector3());
        clonedCompPtr->setBorderSize(this->borderSize->getReal());
        clonedCompPtr->setOffsetPosition(this->offsetPosition->getVector3());
        clonedCompPtr->setOffsetOrientation(this->offsetOrientation->getVector3());
        clonedCompPtr->setOrientationTargetId(this->orientationTargetId->getULong());
        clonedCompPtr->setWidth(this->width->getReal());
        clonedCompPtr->setHeight(this->height->getReal());
        clonedCompPtr->setMaxValue(this->maxValue->getUInt());
        clonedCompPtr->setCurrentValue(this->currentValue->getUInt());
        clonedCompPtr->setFaceCamera(this->faceCamera->getBool());

        clonedCompPtr->setOwner(clonedGameObjectPtr);

        clonedCompPtr->setActivated(this->activated->getBool());

        clonedGameObjectPtr->addComponent(clonedCompPtr);

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
        return clonedCompPtr;
    }

    bool ValueBarComponent::onCloned(void)
    {
        // Search for the prior id of the cloned game object and set the new id and set the new id, if not found set better 0, else the game objects may be corrupt!
        // Attention: How about parent id etc. because when a widget is cloned, its internal id will be re-generated, so the parent id from another widget that should point to this widget is no more valid!
        GameObjectPtr gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getClonedGameObjectFromPriorId(this->orientationTargetId->getULong());
        if (nullptr != gameObjectPtr)
        {
            this->orientationTargetId->setValue(gameObjectPtr->getId());
        }
        else
        {
            this->orientationTargetId->setValue(static_cast<unsigned long>(0));
        }

        // Since connect is called during cloning process, it does not make sense to process furher here, but only when simulation started!
        return true;
    }

    bool ValueBarComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ValueBarComponent] Init value bar component for game object: " + this->gameObjectPtr->getName());

        return true;
    }

    void ValueBarComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ValueBarComponent] Destructor value bar component for game object: " + this->gameObjectPtr->getName());

        this->destroyValueBar();
    }

    bool ValueBarComponent::connect(void)
    {
        GameObjectComponent::connect();

        if (0 != this->orientationTargetId->getULong())
        {
            auto gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->orientationTargetId->getULong());
            if (nullptr != gameObjectPtr)
            {
                this->orientationTargetGameObject = gameObjectPtr.get();
            }
        }

        auto gameObjectTitleCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<GameObjectTitleComponent>());
        if (nullptr != gameObjectTitleCompPtr)
        {
            this->gameObjectTitleComponent = gameObjectTitleCompPtr.get();
            this->gameObjectTitleComponent->setOrientationTargetId(0);
        }

        // No animation on the first appearance.
        this->displayedFraction = this->getValueFraction();
        this->trailFraction = this->displayedFraction;
        this->trailDelayTimer = 0.0f;

        this->setActivated(this->activated->getBool());

        return true;
    }

    bool ValueBarComponent::disconnect(void)
    {
        GameObjectComponent::disconnect();

        this->couldDraw = false;

        this->destroyValueBar();

        this->orientationTargetGameObject = nullptr;
        this->gameObjectTitleComponent = nullptr;

        return true;
    }

    void ValueBarComponent::createValueBar(void)
    {
        if (nullptr == this->manualObject)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
            {
                if (nullptr == this->lineNode)
                {
                    this->lineNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();
                }
                this->manualObject = this->gameObjectPtr->getSceneManager()->createManualObject();
                this->manualObject->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MANUAL);
                this->manualObject->setName("ValueBar_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()) + "_" + Ogre::StringConverter::toString(index));
                this->manualObject->setQueryFlags(0 << 0);

                // Set local AABB
                /*Ogre::Vector3 min(-this->width->getReal() - this->borderSize->getReal(), -this->height->getReal() - this->borderSize->getReal(), -0.01f);
                Ogre::Vector3 max(this->width->getReal() + this->borderSize->getReal(), this->height->getReal() + this->borderSize->getReal(), 0.01f);
                manualObject->setLocalAabb(Ogre::Aabb(min, max));*/

                this->lineNode->attachObject(this->manualObject);
                this->manualObject->setCastShadows(false);

                // A fresh manual object has no section yet.
                this->lastQuadCount = 0;
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ValueBarComponent::createValueBar");
        }
    }

    void ValueBarComponent::destroyValueBar(void)
    {
        if (this->lineNode != nullptr)
        {
            Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
            NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

            Ogre::SceneNode* lineNode = this->lineNode;
            Ogre::ManualObject* manualObject = this->manualObject;
            Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();

            // TODO: Wait?
            NOWA::GraphicsModule::RenderCommand renderCommand = [lineNode, manualObject, sceneManager]()
            {
                lineNode->detachAllObjects();
                sceneManager->destroyManualObject(manualObject);
                lineNode->getParentSceneNode()->removeAndDestroyChild(lineNode);
            };

            NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "ValueBarComponent::destroyValueBar");

            this->manualObject = nullptr;
            this->lineNode = nullptr;
        }
    }

    void ValueBarComponent::update(Ogre::Real dt, bool notSimulating)
    {
        if (false == notSimulating)
        {
            if (nullptr == this->manualObject)
            {
                return;
            }

            //// Do a basic visibility check (e.g., frustum distance, screen proximity, etc.)
            // Ogre::Camera* mainCamera = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
            // if (nullptr != mainCamera)
            //{
            //	// Get world AABB of the ManualObject
            //	const Ogre::Aabb& worldAabb = this->manualObject->getWorldAabbUpdated();

            //	// Skip rendering if not visible
            //	Ogre::AxisAlignedBox legacyBox(worldAabb.getMinimum(), worldAabb.getMaximum());
            //	if (false == mainCamera->isVisible(legacyBox))
            //	{
            //		return;
            //	}
            //}

            if (false == this->gameObjectPtr->isVisible())
            {
                return;
            }

            // The fill follows the value quickly, the damage trail waits a moment and then shrinks
            // towards the fill. A heal snaps both (see setCurrentValue).
            const Ogre::Real targetFraction = this->getValueFraction();

            Ogre::Real followFactor = dt * FILL_FOLLOW_SPEED;
            if (followFactor > 1.0f)
            {
                followFactor = 1.0f;
            }
            this->displayedFraction += (targetFraction - this->displayedFraction) * followFactor;

            if (this->trailDelayTimer > 0.0f)
            {
                this->trailDelayTimer -= dt;
            }
            else if (this->trailFraction > this->displayedFraction)
            {
                this->trailFraction -= TRAIL_SPEED * dt;
            }

            if (this->trailFraction < this->displayedFraction)
            {
                this->trailFraction = this->displayedFraction;
            }

            auto closureFunction = [this](Ogre::Real renderDt)
            {
                // Defensive: the component may have been destroyed (and destroyValueBar() may
                // already have nulled this out) between this closure being queued and actually
                // running - removeTrackedClosure() there stops FUTURE frames, but an
                // already-dequeued invocation for the CURRENT frame can still race past it.
                if (nullptr == this->manualObject)
                {
                    return;
                }

                this->indices = 0;

                const bool isBillboard = (nullptr == this->orientationTargetGameObject && true == this->faceCamera->getBool());
                unsigned int quadCount = 5;
                if (true == this->twoSided->getBool() && false == isBillboard)
                {
                    quadCount = 10;
                }

                if (this->manualObject->getNumSections() > 0 && quadCount == this->lastQuadCount)
                {
                    this->manualObject->beginUpdate(0);
                }
                else
                {
                    this->manualObject->clear();
                    this->manualObject->begin("WhiteNoLightingBackground", Ogre::OT_TRIANGLE_LIST);
                }

                this->drawValueBar();

                if (true == this->couldDraw)
                {
                    this->manualObject->index(0);
                    this->manualObject->end();
                    this->lastQuadCount = quadCount;
                }
                else
                {
                    this->manualObject->clear();
                    this->lastQuadCount = 0;
                }
            };
            Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
            NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);
        }
    }

    void ValueBarComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (ValueBarComponent::AttrActivated() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
        }
        else if (ValueBarComponent::AttrTwoSided() == attribute->getName())
        {
            this->setTwoSided(attribute->getBool());
        }
        else if (ValueBarComponent::AttrInnerColor() == attribute->getName())
        {
            this->setInnerColor(attribute->getVector3());
        }
        else if (ValueBarComponent::AttrOuterColor() == attribute->getName())
        {
            this->setOuterColor(attribute->getVector3());
        }
        else if (ValueBarComponent::AttrOffsetPosition() == attribute->getName())
        {
            this->setOffsetPosition(attribute->getVector3());
        }
        else if (ValueBarComponent::AttrOffsetOrientation() == attribute->getName())
        {
            this->setOffsetOrientation(attribute->getVector3());
        }
        else if (ValueBarComponent::AttrOrientationTargetId() == attribute->getName())
        {
            this->setOrientationTargetId(attribute->getULong());
        }
        else if (ValueBarComponent::AttrBorderSize() == attribute->getName())
        {
            this->setBorderSize(attribute->getReal());
        }
        else if (ValueBarComponent::AttrWidth() == attribute->getName())
        {
            this->setWidth(attribute->getReal());
        }
        else if (ValueBarComponent::AttrHeight() == attribute->getName())
        {
            this->setHeight(attribute->getReal());
        }
        else if (ValueBarComponent::AttrMaxValue() == attribute->getName())
        {
            this->setMaxValue(attribute->getUInt());
        }
        else if (ValueBarComponent::AttrCurrentValue() == attribute->getName())
        {
            this->setCurrentValue(attribute->getUInt());
        }
        else if (ValueBarComponent::AttrFaceCamera() == attribute->getName())
        {
            this->setFaceCamera(attribute->getBool());
        }
    }

    void ValueBarComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "TwoSided"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->twoSided->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "InnerColor"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->innerColor->getVector3())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "OuterColor"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->outerColor->getVector3())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "BorderSize"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->borderSize->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "OffsetPosition"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->offsetPosition->getVector3())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "OffsetOrientation"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->offsetOrientation->getVector3())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "OrientationTargetId"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->orientationTargetId->getULong())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Width"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->width->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Height"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->height->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "MaxValue"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxValue->getUInt())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "CurrentValue"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->currentValue->getUInt())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "FaceCamera"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->faceCamera->getBool())));
        propertiesXML->append_node(propertyXML);
    }

    Ogre::String ValueBarComponent::getClassName(void) const
    {
        return "ValueBarComponent";
    }

    Ogre::String ValueBarComponent::getParentClassName(void) const
    {
        return "GameObjectComponent";
    }

    void ValueBarComponent::setActivated(bool activated)
    {
        this->activated->setValue(activated);

        if (false == activated)
        {
            this->destroyValueBar();
        }
        else
        {
            this->createValueBar();
        }
    }

    bool ValueBarComponent::isActivated(void) const
    {
        return this->activated->getBool();
    }

    void ValueBarComponent::setTwoSided(bool twoSided)
    {
        this->twoSided->setValue(twoSided);
    }

    bool ValueBarComponent::getTwoSided(void) const
    {
        return this->twoSided->getBool();
    }

    void ValueBarComponent::setInnerColor(const Ogre::Vector3& color)
    {
        this->innerColor->setValue(color);
    }

    Ogre::Vector3 ValueBarComponent::getInnerColor(void) const
    {
        return this->innerColor->getVector3();
    }

    void ValueBarComponent::setOuterColor(const Ogre::Vector3& color)
    {
        this->outerColor->setValue(color);
    }

    Ogre::Vector3 ValueBarComponent::getOuterColor(void) const
    {
        return this->outerColor->getVector3();
    }

    void ValueBarComponent::setBorderSize(Ogre::Real borderSize)
    {
        if (borderSize < 0.0f)
        {
            borderSize = 0.0f;
        }
        if (borderSize > 10.0f)
        {
            borderSize = 10.0f;
        }
        this->borderSize->setValue(borderSize);
    }

    Ogre::Real ValueBarComponent::getBorderSize(void) const
    {
        return this->borderSize->getReal();
    }

    void ValueBarComponent::setOffsetPosition(const Ogre::Vector3& offsetPosition)
    {
        this->offsetPosition->setValue(offsetPosition);
    }

    Ogre::Vector3 ValueBarComponent::getOffsetPosition(void) const
    {
        return this->offsetPosition->getVector3();
    }

    void ValueBarComponent::setOffsetOrientation(const Ogre::Vector3& offsetOrientation)
    {
        this->offsetOrientation->setValue(offsetOrientation);
    }

    Ogre::Vector3 ValueBarComponent::getOffsetOrientation(void) const
    {
        return this->offsetOrientation->getVector3();
    }

    void ValueBarComponent::setOrientationTargetId(unsigned long targetId)
    {
        this->orientationTargetId->setValue(targetId);
    }

    unsigned long ValueBarComponent::getOrientationTargetId(unsigned int index) const
    {
        return this->orientationTargetId->getULong();
    }

    void ValueBarComponent::setWidth(Ogre::Real width)
    {
        if (width < 0.1f)
        {
            width = 0.1f;
        }
        this->width->setValue(width);
    }

    Ogre::Real ValueBarComponent::getWidth(void) const
    {
        return this->width->getReal();
    }

    void ValueBarComponent::setHeight(Ogre::Real height)
    {
        if (height < 0.1f)
        {
            height = 0.1f;
        }
        this->height->setValue(height);
    }

    Ogre::Real ValueBarComponent::getHeight(void) const
    {
        return this->height->getReal();
    }

    void ValueBarComponent::setMaxValue(unsigned int maxValue)
    {
        this->maxValue->setValue(maxValue);

        // A new scale, nothing to animate.
        this->displayedFraction = this->getValueFraction();
        this->trailFraction = this->displayedFraction;
        this->trailDelayTimer = 0.0f;
    }

    unsigned int ValueBarComponent::getMaxValue(void) const
    {
        return this->maxValue->getUInt();
    }

    void ValueBarComponent::setCurrentValue(unsigned int currentValue)
    {
        if (currentValue > this->maxValue->getUInt())
        {
            currentValue = this->maxValue->getUInt();
        }

        const unsigned int oldValue = this->currentValue->getUInt();
        this->currentValue->setValue(currentValue);

        if (currentValue < oldValue)
        {
            // Damage: the fill animates down in update(), the trail stays for a moment at the old value.
            this->trailDelayTimer = TRAIL_DELAY;
        }
        else if (currentValue > oldValue)
        {
            // Heal: snap, a growing trail would look like damage.
            this->displayedFraction = this->getValueFraction();
            this->trailFraction = this->displayedFraction;
        }
    }

    unsigned int ValueBarComponent::getCurrentValue(void) const
    {
        return this->currentValue->getUInt();
    }

    void ValueBarComponent::setFaceCamera(bool faceCamera)
    {
        this->faceCamera->setValue(faceCamera);
    }

    bool ValueBarComponent::getFaceCamera(void) const
    {
        return this->faceCamera->getBool();
    }

    Ogre::Real ValueBarComponent::getValueFraction(void) const
    {
        const unsigned int maxValue = this->maxValue->getUInt();
        if (0 == maxValue)
        {
            return 0.0f;
        }

        Ogre::Real fraction = static_cast<Ogre::Real>(this->currentValue->getUInt()) / static_cast<Ogre::Real>(maxValue);
        if (fraction > 1.0f)
        {
            fraction = 1.0f;
        }
        return fraction;
    }

    void ValueBarComponent::emitQuad(const Ogre::Vector3& center, const Ogre::Quaternion& orientation, Ogre::Real left, Ogre::Real right, Ogre::Real bottom, Ogre::Real top, Ogre::Real depth, const Ogre::ColourValue& topColor,
        const Ogre::ColourValue& bottomColor, bool backFace)
    {
        Ogre::Real z = depth;
        Ogre::Real normalZ = 1.0f;
        if (true == backFace)
        {
            // The back side stacks its layers the other way round.
            z = -depth;
            normalZ = -1.0f;
        }

        const Ogre::Vector3 normal = orientation * Ogre::Vector3(0.0f, 0.0f, normalZ);

        const Ogre::Vector3 leftTop = center + orientation * Ogre::Vector3(left, top, z);
        const Ogre::Vector3 leftBottom = center + orientation * Ogre::Vector3(left, bottom, z);
        const Ogre::Vector3 rightBottom = center + orientation * Ogre::Vector3(right, bottom, z);
        const Ogre::Vector3 rightTop = center + orientation * Ogre::Vector3(right, top, z);

        // Counter clockwise as seen from the side the quad faces.
        if (false == backFace)
        {
            this->manualObject->position(leftTop);
            this->manualObject->colour(topColor);
            this->manualObject->normal(normal);
            this->manualObject->textureCoord(0.0f, 0.0f);

            this->manualObject->position(leftBottom);
            this->manualObject->colour(bottomColor);
            this->manualObject->normal(normal);
            this->manualObject->textureCoord(0.0f, 1.0f);

            this->manualObject->position(rightBottom);
            this->manualObject->colour(bottomColor);
            this->manualObject->normal(normal);
            this->manualObject->textureCoord(1.0f, 1.0f);

            this->manualObject->position(rightTop);
            this->manualObject->colour(topColor);
            this->manualObject->normal(normal);
            this->manualObject->textureCoord(1.0f, 0.0f);
        }
        else
        {
            this->manualObject->position(rightTop);
            this->manualObject->colour(topColor);
            this->manualObject->normal(normal);
            this->manualObject->textureCoord(0.0f, 0.0f);

            this->manualObject->position(rightBottom);
            this->manualObject->colour(bottomColor);
            this->manualObject->normal(normal);
            this->manualObject->textureCoord(0.0f, 1.0f);

            this->manualObject->position(leftBottom);
            this->manualObject->colour(bottomColor);
            this->manualObject->normal(normal);
            this->manualObject->textureCoord(1.0f, 1.0f);

            this->manualObject->position(leftTop);
            this->manualObject->colour(topColor);
            this->manualObject->normal(normal);
            this->manualObject->textureCoord(1.0f, 0.0f);
        }

        this->manualObject->quad(this->indices + 0, this->indices + 1, this->indices + 2, this->indices + 3);
        this->indices += 4;
    }

    void ValueBarComponent::drawValueBar(void)
    {
        this->couldDraw = false;

        // Layout in bar space, centered on the game object position + offset:
        //
        //    +---------------- frame (outer color) ----------------+
        //    | +------------------ background -------------------+ |
        //    | |######## fill #########::::: trail :::::         | |  height
        //    | +-------------------------------------------------+ |
        //    +-----------------------------------------------------+
        //                            width
        //
        // Attention: every layer gets its own small depth offset towards the viewer. The former code
        // drew frame and fill both at z = 0, so they fought in the depth buffer and the fill showed
        // through or vanished behind the frame. It additionally had degenerate quads (two identical
        // corners), which rendered as the skewed triangles.

        const Ogre::Vector3 fillColor = this->innerColor->getVector3();
        const Ogre::Vector3 frameColor = this->outerColor->getVector3();

        const Ogre::Vector3 position = this->gameObjectPtr->getPosition();
        const Ogre::Vector3 offset = this->offsetPosition->getVector3();

        Ogre::Vector3 center = position;
        Ogre::Quaternion orientation = Ogre::Quaternion::IDENTITY;
        bool isBillboard = false;

        if (nullptr != this->orientationTargetGameObject)
        {
            const Ogre::Vector3 direction = position - this->orientationTargetGameObject->getPosition();
            orientation = MathHelper::getInstance()->lookAt(direction) * MathHelper::getInstance()->degreesToQuat(this->offsetOrientation->getVector3());
            center = position + (orientation * offset);
        }
        else if (true == this->faceCamera->getBool())
        {
            // Billboard: always faces the camera and fills from left to right as the player sees it,
            // no matter which way the game object looks. The offset stays in world space, so the bar
            // stays above the head when the game object turns around.
            Ogre::Camera* camera = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
            if (nullptr != camera)
            {
                orientation = camera->getDerivedOrientation();
            }
            center = position + offset;
            isBillboard = true;
        }
        else
        {
            orientation = this->gameObjectPtr->getOrientation() * MathHelper::getInstance()->degreesToQuat(this->offsetOrientation->getVector3());
            center = position + (orientation * offset);
        }

        if (nullptr != this->gameObjectTitleComponent)
        {
            this->gameObjectTitleComponent->getMovableText()->setTextYOffset(-1.0f);
            // Note: Order is really important! First set orientation, then position, else strange side effects do occur!
            this->gameObjectTitleComponent->getMovableText()->getParentSceneNode()->_setDerivedOrientation(orientation);
            this->gameObjectTitleComponent->getMovableText()->getParentSceneNode()->_setDerivedPosition(center);
        }

        const Ogre::Real halfWidth = this->width->getReal() * 0.5f;
        const Ogre::Real halfHeight = this->height->getReal() * 0.5f;
        const Ogre::Real border = this->borderSize->getReal();

        // Depth step between two layers. Relative to the bar size, so it works for tiny and huge bars.
        const Ogre::Real depthStep = 0.001f + this->height->getReal() * 0.02f;

        const Ogre::Real fillRight = -halfWidth + 2.0f * halfWidth * this->displayedFraction;
        const Ogre::Real trailRight = -halfWidth + 2.0f * halfWidth * this->trailFraction;

        // Colors: dark recessed background, warm light trail, fill with a vertical gradient and a glossy
        // highlight on its upper part.
        const Ogre::ColourValue frameTop = toColour(frameColor, 1.0f, 0.06f);
        const Ogre::ColourValue frameBottom = toColour(frameColor, 1.0f, 0.0f);
        const Ogre::ColourValue backgroundTop = toColour(fillColor, 0.10f, 0.02f);
        const Ogre::ColourValue backgroundBottom = toColour(fillColor, 0.18f, 0.03f);
        const Ogre::ColourValue trailTop(1.0f, 0.92f, 0.70f, 1.0f);
        const Ogre::ColourValue trailBottom(0.95f, 0.70f, 0.40f, 1.0f);
        const Ogre::ColourValue fillTop = toColour(fillColor, 1.15f, 0.05f);
        const Ogre::ColourValue fillBottom = toColour(fillColor, 0.55f, 0.0f);
        const Ogre::ColourValue highlightTop = toColour(fillColor, 1.0f, 0.45f);
        const Ogre::ColourValue highlightBottom = toColour(fillColor, 1.15f, 0.12f);

        const Ogre::Real highlightBottomY = halfHeight * 0.2f;

        unsigned int sides = 1;
        if (true == this->twoSided->getBool() && false == isBillboard)
        {
            sides = 2;
        }

        for (unsigned int side = 0; side < sides; side++)
        {
            const bool backFace = (1 == side);

            // 1) Frame
            this->emitQuad(center, orientation, -halfWidth - border, halfWidth + border, -halfHeight - border, halfHeight + border, 0.0f, frameTop, frameBottom, backFace);
            // 2) Background (the empty part)
            this->emitQuad(center, orientation, -halfWidth, halfWidth, -halfHeight, halfHeight, depthStep, backgroundTop, backgroundBottom, backFace);
            // 3) Damage trail: the part that was just lost
            this->emitQuad(center, orientation, -halfWidth, trailRight, -halfHeight, halfHeight, 2.0f * depthStep, trailTop, trailBottom, backFace);
            // 4) Fill
            this->emitQuad(center, orientation, -halfWidth, fillRight, -halfHeight, halfHeight, 3.0f * depthStep, fillTop, fillBottom, backFace);
            // 5) Gloss on the upper part of the fill
            this->emitQuad(center, orientation, -halfWidth, fillRight, highlightBottomY, halfHeight, 4.0f * depthStep, highlightTop, highlightBottom, backFace);
        }

        this->couldDraw = true;
    }

}; // namespace end
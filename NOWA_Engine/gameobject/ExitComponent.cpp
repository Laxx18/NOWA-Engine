#include "NOWAPrecompiled.h"
#include "ExitComponent.h"
#include "gameobject/GameObjectController.h"
#include "gameobject/PlayerStartComponent.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "main/ProcessManager.h"
#include "modules/GameProgressModule.h"
#include "utilities/XMLConverter.h"

#include <algorithm>
#include <cmath>

namespace
{
    // How close (along the exit direction) the source must get to the exit before the scene changes. Everything beyond
    // the exit counts as well, so a fast player who moves more than this distance per frame cannot skip it.
    const Ogre::Real EXIT_TRIGGER_DISTANCE = 0.2f;

    // How far in front of the exit (into the level, against the exit direction) the source is placed, when it arrives
    // through this exit from another scene. Must be clearly bigger than EXIT_TRIGGER_DISTANCE, otherwise the arrival
    // would immediately trigger the exit again.
    const Ogre::Real EXIT_ARRIVAL_DISTANCE = 1.5f;
}

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    ExitComponent::ExitComponent() :
        GameObjectComponent(),
        activated(new Variant(ExitComponent::AttrActivated(), true, this->attributes)),
        targetSceneName(new Variant(ExitComponent::AttrTargetSceneName(), Ogre::String(""), this->attributes)),
        targetLocationName(new Variant(ExitComponent::AttrTargetLocationName(), Ogre::String(""), this->attributes)),
        sourceGameObjectId(new Variant(ExitComponent::AttrSourceId(), static_cast<unsigned long>(0), this->attributes, true)),
        exitDirection(new Variant(ExitComponent::AttrExitDirection(), Ogre::Vector2::ZERO, this->attributes)),
        axis(new Variant(ExitComponent::AttrAxis(), std::vector<Ogre::String>{"X,Y", "X,Z"}, this->attributes)),
        sourceGameObject(nullptr),
        processAlreadyAttached(false)
    {
        this->targetSceneName->setDescription("Sets the target scene name, that should be loaded. Leave it empty for an exit that is only an entrance (arrival point).");
        this->targetLocationName->setDescription("The NAME of the game object in the target scene, at which the source game object arrives. Usually the matching exit ('door') "
                                                 "of the target scene, which leads back here - then the source is placed in front of that door, looking into the level. "
                                                 "May also be a game object with a PlayerStartComponent. Every exit is a door: its own game object name is its location name.");
        this->sourceGameObjectId->setDescription("The id for the source game object for target location placement.");
        this->exitDirection->setDescription("Points OUT of the level: the direction in which the source walks to leave the scene through this exit, e.g. '1 0' for an exit at the right border, '-1 0' for one at the left border. "
                                            "Arriving through this exit, the source is placed in front of it, in the opposite direction.");
        this->axis->setDescription("The axis for exit direction. For Jump'n'Run e.g. 'X,Y' is correct and for a casual 3D scene 'X,Z'.");

        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &ExitComponent::handlePhysicsTrigger), EventPhysicsTrigger::getStaticEventType());
    }

    ExitComponent::~ExitComponent()
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ExitComponent] Destructor exit component for game object: " + this->gameObjectPtr->getName());
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ExitComponent::handlePhysicsTrigger), EventPhysicsTrigger::getStaticEventType());
    }

    bool ExitComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
        {
            // Attention: was getAttrib() - a string written into a bool variant.
            this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetSceneName")
        {
            this->targetSceneName->setValue(XMLConverter::getAttrib(propertyElement, "data", ""));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetLocationName")
        {
            this->targetLocationName->setValue(XMLConverter::getAttrib(propertyElement, "data", ""));
            propertyElement = propertyElement->next_sibling("property");
        }
        // Note: stays valid across scenes, as long as the source (the player) is a global game object - its id is the
        // same in every scene.
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SourceGameObjectId")
        {
            this->sourceGameObjectId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ExitDirection")
        {
            this->exitDirection->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Axis")
        {
            this->axis->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        return true;
    }

    bool ExitComponent::postInit(void)
    {
        AppStateManager::getSingletonPtr()->getGameProgressModule()->addScene(Core::getSingletonPtr()->getProjectName() + "/" + Core::getSingletonPtr()->getFileNameFromPath(Core::getSingletonPtr()->getCurrentScenePath()),
            this->targetSceneName->getString(), this->targetLocationName->getString(), this->exitDirection->getVector2(), this->gameObjectPtr->getPosition(), this->axis->getListSelectedValue() == "X,Y" ? true : false);
        return true;
    }

    bool ExitComponent::connect(void)
    {
        // Attention: must be reset here. It was never reset, so after a simulation stop and restart in the editor (without
        // a scene change) the exit did not work anymore.
        this->processAlreadyAttached = false;

        // Attention: was 'this->sourceGameObjectId != 0', which compares the Variant POINTER with 0 and is always true.
        if (0 == this->sourceGameObjectId->getULong())
        {
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[ExitComponent] Error: Can not use exit component of game object: '" + this->gameObjectPtr->getName() + "', because no source game object id is set.");
            return false;
        }

        auto sourceGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->sourceGameObjectId->getULong());
        if (nullptr == sourceGameObjectPtr)
        {
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[ExitComponent] Error: Can not use exit component of game object: '" + this->gameObjectPtr->getName() + "', because the source game object id: '" +
                                                                                Ogre::StringConverter::toString(this->sourceGameObjectId->getULong()) + "' does not exist.");
            return false;
        }

        this->sourceGameObject = sourceGameObjectPtr.get();
        // Announce the player to game progress module
        AppStateManager::getSingletonPtr()->getGameProgressModule()->setPlayerName(this->sourceGameObject->getName());

        // Note: the scene graph data (addScene) is registered once in postInit. The former second registration here used
        // GameProgressModule::getCurrentSceneName() as key instead of the scene path postInit uses.
        return true;
    }

    bool ExitComponent::disconnect(void)
    {
        this->sourceGameObject = nullptr;
        this->processAlreadyAttached = false;
        return true;
    }

    bool ExitComponent::onCloned(void)
    {
        // Nothing to clone?
        /*// Search for the prior id of the cloned game object and set the new id and set the new id, if not found set better 0, else the game objects may be corrupt!
        auto sourceGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getClonedGameObjectFromPriorId(this->sourceGameObjectId->getULong());
        if (nullptr != gameObjectPtr)
        {
            this->sourceGameObject = sourceGameObjectPtr.get();
            // Only the id is important!
            this->setSourceId(this->sourceGameObject->getId());
        }
        else
            this->setSourceId(0);*/
        return true;
    }

    Ogre::String ExitComponent::getClassName(void) const
    {
        return "ExitComponent";
    }

    Ogre::String ExitComponent::getParentClassName(void) const
    {
        return "GameObjectComponent";
    }

    void ExitComponent::setActivated(bool activated)
    {
        this->activated->setValue(activated);
    }

    bool ExitComponent::isActivated(void) const
    {
        return this->activated->getBool();
    }

    GameObjectCompPtr ExitComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        // Nothing to clone

        return ExitCompPtr();
    }

    void ExitComponent::handlePhysicsTrigger(NOWA::EventDataPtr eventData)
    {
        boost::shared_ptr<NOWA::EventPhysicsTrigger> castEventData = boost::static_pointer_cast<EventPhysicsTrigger>(eventData);
        unsigned long visitorGameObjectId = castEventData->getVisitorGameObjectId();
        bool entered = castEventData->getHasEntered();
        // Event has been triggered by a PhysicsTriggerComponent inside this game object
        if (visitorGameObjectId != this->gameObjectPtr->getId() || false == entered)
        {
            return;
        }

        // Attention: the same guards as in update(). Without them the trigger and the distance check could both request a
        // scene change in the same frame, and a deactivated exit still worked via the trigger.
        if (true == this->processAlreadyAttached || false == this->activated->getBool() || true == this->targetSceneName->getString().empty())
        {
            return;
        }

        AppStateManager::getSingletonPtr()->getGameProgressModule()->setRequestedTargetLocationName(this->targetLocationName->getString());
        AppStateManager::getSingletonPtr()->getGameProgressModule()->changeScene(this->targetSceneName->getString());
        this->processAlreadyAttached = true;
    }

    void ExitComponent::update(Ogre::Real dt, bool notSimulating)
    {
        if (true == notSimulating || true == this->processAlreadyAttached || false == this->activated->getBool() || nullptr == this->sourceGameObject)
        {
            return;
        }

        if (true == this->targetSceneName->getString().empty())
        {
            return;
        }

        Ogre::Vector2 direction = this->exitDirection->getVector2();
        if (true == direction.isZeroLength())
        {
            return;
        }
        direction.normalise();

        // Attention: the former check measured only ONE coordinate (x for a horizontal exit), so the exit fired at any
        // height as soon as the player's x came within 0.2 of it - also far above or below the exit. It also used a
        // +-0.2 window, which a fast player can skip in a single frame, and read the Vector2 exit direction as Vector3.
        //
        // Now: the offset of the source relative to the exit, in the plane of the chosen axes, split into
        //   along  - distance in exit direction (negative = still inside the level), and
        //   across - distance perpendicular to it, limited by the size of the exit game object.
        const Ogre::Vector3 exitPosition = this->gameObjectPtr->getPosition();
        const Ogre::Vector3 sourcePosition = this->sourceGameObject->getPosition();

        Ogre::Vector2 offset(sourcePosition.x - exitPosition.x, sourcePosition.y - exitPosition.y);
        if ("X,Z" == this->axis->getListSelectedValue())
        {
            offset = Ogre::Vector2(sourcePosition.x - exitPosition.x, sourcePosition.z - exitPosition.z);
        }

        // A corrupt Newton ragdoll can produce NaN positions - that must not load a new level.
        if (true == std::isnan(offset.x) || true == std::isnan(offset.y))
        {
            return;
        }

        const Ogre::Real along = offset.dotProduct(direction);
        const Ogre::Real across = Ogre::Math::Abs(offset.dotProduct(direction.perpendicular()));

        const Ogre::Vector3 exitSize = this->gameObjectPtr->getSize();
        const Ogre::Real halfExtent = 0.5f * std::max(exitSize.x, std::max(exitSize.y, exitSize.z));

        if (along >= -EXIT_TRIGGER_DISTANCE && across <= halfExtent)
        {
            // The game object with this name in the target scene places the player: the matching exit ("door") via
            // ExitComponent::applyArrival, or a PlayerStartComponent.
            AppStateManager::getSingletonPtr()->getGameProgressModule()->setRequestedTargetLocationName(this->targetLocationName->getString());
            AppStateManager::getSingletonPtr()->getGameProgressModule()->changeScene(this->targetSceneName->getString());
            this->processAlreadyAttached = true;
        }
    }

    void ExitComponent::applyArrival(void)
    {
        // Called by the DotSceneImportModule after all post inits (like PlayerStartComponent::postInit), so the global source
        // game object already exists. Only the exit ("door") whose game object name was requested by the exit of the previous
        // scene places the source.
        // Loading a save game snapshot: the saved transform of the source wins.
        if (true == AppStateManager::getSingletonPtr()->getGameProgressModule()->getKeepPlayerTransform())
        {
            return;
        }

        const Ogre::String requestedName = AppStateManager::getSingletonPtr()->getGameProgressModule()->getRequestedTargetLocationName();
        if (true == requestedName.empty() || requestedName != this->gameObjectPtr->getName())
        {
            return;
        }

        GameObjectPtr sourceGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->sourceGameObjectId->getULong());
        if (nullptr == sourceGameObjectPtr)
        {
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[ExitComponent] Error: Can not place the arriving source at exit: '" + this->gameObjectPtr->getName() + "', because the source game object id: '" +
                                                                                Ogre::StringConverter::toString(this->sourceGameObjectId->getULong()) + "' does not exist.");
            return;
        }

        const Ogre::Vector3 exitPosition = this->gameObjectPtr->getPosition();
        Ogre::Vector2 direction = this->exitDirection->getVector2();
        if (true == direction.isZeroLength())
        {
            // No direction: no "in front of" either. Place it right at the exit and keep its orientation.
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[ExitComponent] Warning: Exit: '" + this->gameObjectPtr->getName() + "' has no exit direction. Placing the source directly at the exit.");
            PlayerStartComponent::placeWithAttachments(sourceGameObjectPtr.get(), exitPosition, sourceGameObjectPtr->getOrientation());
            return;
        }
        direction.normalise();

        // Into the level = against the exit direction, in the plane of the chosen axes.
        Ogre::Vector3 inward(-direction.x, -direction.y, 0.0f);
        if ("X,Z" == this->axis->getListSelectedValue())
        {
            inward = Ogre::Vector3(-direction.x, 0.0f, -direction.y);
        }

        const Ogre::Vector3 position = exitPosition + inward * EXIT_ARRIVAL_DISTANCE;

        // Look into the level: a yaw around the up axis only, so the source stays upright. Its default direction (the
        // direction its mesh looks at without rotation) is turned onto the horizontal part of 'inward'. For a vertical exit
        // in a Jump'n'Run (a shaft, direction '0 1' or '0 -1') there is no horizontal part - then the orientation is kept.
        Ogre::Quaternion orientation = sourceGameObjectPtr->getOrientation();
        const Ogre::Vector3 defaultDirection = sourceGameObjectPtr->getDefaultDirection();
        const Ogre::Vector3 horizontalInward(inward.x, 0.0f, inward.z);
        const Ogre::Vector3 horizontalDefault(defaultDirection.x, 0.0f, defaultDirection.z);
        if (false == horizontalInward.isZeroLength() && false == horizontalDefault.isZeroLength())
        {
            const Ogre::Real yaw = std::atan2(horizontalInward.x, horizontalInward.z) - std::atan2(horizontalDefault.x, horizontalDefault.z);
            orientation = Ogre::Quaternion(Ogre::Radian(yaw), Ogre::Vector3::UNIT_Y);
        }

        PlayerStartComponent::placeWithAttachments(sourceGameObjectPtr.get(), position, orientation);

        Ogre::LogManager::getSingleton().logMessage(Ogre::LML_NORMAL,
            "[ExitComponent] Source: '" + sourceGameObjectPtr->getName() + "' arrived at exit: '" + this->gameObjectPtr->getName() + "', position: " + Ogre::StringConverter::toString(position));
    }

    void ExitComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (ExitComponent::AttrActivated() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
        }
        else if (ExitComponent::AttrTargetSceneName() == attribute->getName())
        {
            this->targetSceneName->setValue(attribute->getString());
        }
        else if (ExitComponent::AttrTargetLocationName() == attribute->getName())
        {
            this->targetLocationName->setValue(attribute->getString());
        }
        else if (ExitComponent::AttrSourceId() == attribute->getName())
        {
            this->sourceGameObjectId->setValue(attribute->getULong());
        }
        else if (ExitComponent::AttrExitDirection() == attribute->getName())
        {
            this->exitDirection->setValue(attribute->getVector2());
        }
        else if (ExitComponent::AttrAxis() == attribute->getName())
        {
            this->axis->setListSelectedValue(attribute->getListSelectedValue());
        }
    }

    void ExitComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
        propertyXML->append_attribute(doc.allocate_attribute("name", "TargetSceneName"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->targetSceneName->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "TargetLocationName"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->targetLocationName->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "SourceGameObjectId"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->sourceGameObjectId->getULong())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "ExitDirection"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->exitDirection->getVector2())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Axis"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->axis->getListSelectedValue())));
        propertiesXML->append_node(propertyXML);
    }

    Ogre::String ExitComponent::getTargetSceneName(void) const
    {
        return this->targetSceneName->getString();
    }

    Ogre::String ExitComponent::getCurrentSceneName(void) const
    {
        return AppStateManager::getSingletonPtr()->getGameProgressModule()->getCurrentSceneName();
    }

    Ogre::Vector2 ExitComponent::getExitDirection(void) const
    {
        return this->exitDirection->getVector2();
    }

    Ogre::String ExitComponent::getAxis(void) const
    {
        // Attention: was this->exitDirection->getListSelectedValue() - the wrong variant.
        return this->axis->getListSelectedValue();
    }
}; // namespace end
#include "NOWAPrecompiled.h"
#include "PlayerStartComponent.h"
#include "gameobject/ExitComponent.h"
#include "gameobject/GameObjectController.h"
#include "gameobject/PhysicsActiveComponent.h"
#include "gameobject/PhysicsActiveKinematicComponent.h"
#include "gameobject/TagPointComponent.h"
#include "main/AppStateManager.h"
#include "modules/GameProgressModule.h"
#include "utilities/XMLConverter.h"

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    PlayerStartComponent::PlayerStartComponent() :
        GameObjectComponent(),
        activated(new Variant(PlayerStartComponent::AttrActivated(), true, this->attributes)),
        targetId(new Variant(PlayerStartComponent::AttrTargetId(), static_cast<unsigned long>(0), this->attributes, true))
    {
        this->targetId->setDescription("The id of the game object (e.g. the global player) that is placed at this spawn point when the scene is opened directly. "
                                       "Game objects attached to it via a TagPointComponent are moved along. Arrivals through an exit are placed by the exits themselves.");
    }

    PlayerStartComponent::~PlayerStartComponent()
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlayerStartComponent] Destructor player start component for game object: " + this->gameObjectPtr->getName());
    }

    bool PlayerStartComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
        {
            this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetId")
        {
            this->targetId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        // Attention: scenes saved with an intermediate version may still contain 'SpawnName' and 'IsDefault'. They are skipped
        // here, otherwise they would be left over for the parser of the next component. Saving the scene once removes them.
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SpawnName")
        {
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "IsDefault")
        {
            propertyElement = propertyElement->next_sibling("property");
        }

        return true;
    }

    bool PlayerStartComponent::postInit(void)
    {
        // Runs on every scene load - also for a plain "open this scene" in NOWA-Design - so the target always
        // stands at this level's spawn point, no matter in which level it was placed last.
        if (false == this->activated->getBool() || false == this->isSelected())
        {
            return true;
        }

        this->applyToTarget();
        return true;
    }

    bool PlayerStartComponent::isSelected(void) const
    {
        // Loading a save game snapshot: the saved transform of the target wins.
        if (true == AppStateManager::getSingletonPtr()->getGameProgressModule()->getKeepPlayerTransform())
        {
            return false;
        }

        const Ogre::String requestedName = AppStateManager::getSingletonPtr()->getGameProgressModule()->getRequestedTargetLocationName();

        // Scene opened directly (NOWA-Design, first scene of a game): nothing was requested, this spawn point is used.
        if (true == requestedName.empty() || requestedName == this->gameObjectPtr->getName())
        {
            return true;
        }

        // Entered through an exit, but the requested location is not this one. If it exists - the matching exit ("door"), which
        // places the player itself, or another spawn point - this one stays out. postInit of this component runs after all game
        // objects of the scene are created, so the lookup is reliable here.
        GameObjectPtr requestedGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromName(requestedName);
        if (nullptr != requestedGameObjectPtr)
        {
            const bool isSpawnPoint = nullptr != NOWA::makeStrongPtr(requestedGameObjectPtr->getComponent<PlayerStartComponent>());
            const bool isExit = nullptr != NOWA::makeStrongPtr(requestedGameObjectPtr->getComponent<ExitComponent>());
            if (true == isSpawnPoint || true == isExit)
            {
                return false;
            }
        }

        // Nothing with that name (typo in 'Target Location Name'): this spawn point steps in, otherwise the player would stay
        // wherever he was in the previous scene.
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PlayerStartComponent] Warning: Nothing named: '" + requestedName + "' in this scene. Using spawn point: '" + this->gameObjectPtr->getName() + "' instead.");
        return true;
    }

    bool PlayerStartComponent::connect(void)
    {
        // Note: no placement here - it already happened when the scene was loaded. Doing it again on every simulation start
        // would snap the player back to the spawn point, even if it was moved on purpose in the editor for testing.

        if (false == this->activated->getBool())
        {
            return true;
        }

        // Announce the target as player, like ExitComponent::connect does. Otherwise a level without any exit had no player
        // name, and AppState::handleSceneLoaded did not activate the player controller.
        GameObjectPtr targetGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->targetId->getULong());
        if (nullptr != targetGameObjectPtr)
        {
            AppStateManager::getSingletonPtr()->getGameProgressModule()->setPlayerName(targetGameObjectPtr->getName());
        }
        return true;
    }

    bool PlayerStartComponent::disconnect(void)
    {
        return true;
    }

    bool PlayerStartComponent::onCloned(void)
    {
        return true;
    }

    void PlayerStartComponent::update(Ogre::Real dt, bool notSimulating)
    {
        // Nothing to do per frame.
    }

    Ogre::String PlayerStartComponent::getClassName(void) const
    {
        return "PlayerStartComponent";
    }

    Ogre::String PlayerStartComponent::getParentClassName(void) const
    {
        return "GameObjectComponent";
    }

    void PlayerStartComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (PlayerStartComponent::AttrActivated() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
        }
        else if (PlayerStartComponent::AttrTargetId() == attribute->getName())
        {
            this->setTargetId(attribute->getULong());
        }
    }

    void PlayerStartComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        // 2 = int
        // 12 = bool
        GameObjectComponent::writeXML(propertiesXML, doc);

        xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "TargetId"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->targetId->getULong())));
        propertiesXML->append_node(propertyXML);
    }

    GameObjectCompPtr PlayerStartComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        PlayerStartCompPtr clonedCompPtr(boost::make_shared<PlayerStartComponent>());

        clonedCompPtr->setActivated(this->activated->getBool());
        clonedCompPtr->setTargetId(this->targetId->getULong());

        clonedGameObjectPtr->addComponent(clonedCompPtr);
        clonedCompPtr->setOwner(clonedGameObjectPtr);

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
        return clonedCompPtr;
    }

    void PlayerStartComponent::setActivated(bool activated)
    {
        this->activated->setValue(activated);
    }

    bool PlayerStartComponent::isActivated(void) const
    {
        return this->activated->getBool();
    }

    void PlayerStartComponent::setTargetId(unsigned long targetId)
    {
        this->targetId->setValue(targetId);
    }

    unsigned long PlayerStartComponent::getTargetId(void) const
    {
        return this->targetId->getULong();
    }

    bool PlayerStartComponent::applyToTarget(void)
    {
        GameObjectPtr targetGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->targetId->getULong());
        if (nullptr == targetGameObjectPtr)
        {
            // Not an error: a level can be opened standalone (e.g. for level design) without the target.
            return false;
        }

        return PlayerStartComponent::placeWithAttachments(targetGameObjectPtr.get(), this->gameObjectPtr->getPosition(), this->gameObjectPtr->getOrientation());
    }

    bool PlayerStartComponent::placeWithAttachments(GameObject* targetGameObject, const Ogre::Vector3& position, const Ogre::Quaternion& orientation)
    {
        if (nullptr == targetGameObject)
        {
            return false;
        }

        // The target moves from 'oldTransform' to the new transform. Everything attached to it gets the very same rigid move:
        // newPose = newTransform * inverse(oldTransform) * attachedPose. So a weapon keeps exactly the pose it had in the hand -
        // no matter how far the target travels or how it is rotated.
        const Ogre::Vector3 oldPosition = targetGameObject->getPosition();
        const Ogre::Quaternion oldOrientation = targetGameObject->getOrientation();

        const Ogre::Quaternion deltaOrientation = orientation * oldOrientation.Inverse();

        // Attached game objects first, while the target still stands at its old transform. A target may carry several
        // TagPointComponents (weapon, shield, ...), each with its own source.
        unsigned int occurrenceIndex = 0;
        while (true)
        {
            auto tagPointCompPtr = NOWA::makeStrongPtr(targetGameObject->getComponentWithOccurrence<TagPointComponent>(occurrenceIndex));
            if (nullptr == tagPointCompPtr)
            {
                break;
            }
            occurrenceIndex++;

            GameObjectPtr attachedGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(tagPointCompPtr->getSourceId());
            if (nullptr == attachedGameObjectPtr)
            {
                continue;
            }

            const Ogre::Vector3 attachedPosition = position + deltaOrientation * (attachedGameObjectPtr->getPosition() - oldPosition);
            const Ogre::Quaternion attachedOrientation = deltaOrientation * attachedGameObjectPtr->getOrientation();

            PlayerStartComponent::placeGameObject(attachedGameObjectPtr.get(), attachedPosition, attachedOrientation);
        }

        PlayerStartComponent::placeGameObject(targetGameObject, position, orientation);
        return true;
    }

    void PlayerStartComponent::placeGameObject(GameObject* gameObject, const Ogre::Vector3& position, const Ogre::Quaternion& orientation)
    {
        auto physicsActiveCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<PhysicsActiveComponent>());
        if (nullptr == physicsActiveCompPtr)
        {
            // Not physically simulated - a plain transform set is fine.
            GraphicsModule::getInstance()->teleportNodePosition(gameObject->getSceneNode(), position, true);
            GraphicsModule::getInstance()->teleportNodeOrientation(gameObject->getSceneNode(), orientation);
            return;
        }

        auto kinematicComponent = dynamic_cast<PhysicsActiveKinematicComponent*>(physicsActiveCompPtr.get());
        if (nullptr != kinematicComponent)
        {
            // Attention: PhysicsComponent::setPosition() does not move a kinematic body (see
            // TagPointComponent::updateV2PhysicsFromTagPoint), and outside of the simulation nothing moves its
            // scene node after the body. So both are set: the body for the physics, the node for what is seen.
            kinematicComponent->setKinematicPositionOrientation(position, orientation);
            GraphicsModule::getInstance()->teleportNodePosition(gameObject->getSceneNode(), position, true);
            GraphicsModule::getInstance()->teleportNodeOrientation(gameObject->getSceneNode(), orientation);
            return;
        }

        physicsActiveCompPtr->setPosition(position);
        physicsActiveCompPtr->setOrientation(orientation);
    }

}; // namespace end
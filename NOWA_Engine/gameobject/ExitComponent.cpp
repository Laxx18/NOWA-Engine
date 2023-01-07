#include "NOWAPrecompiled.h"
#include "ExitComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/GameProgressModule.h"
#include "gameobject/GameObjectController.h"
#include "main/ProcessManager.h"
#include "main/Core.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	ExitComponent::ExitComponent()
		: GameObjectComponent(),
		activated(new Variant(ExitComponent::AttrActivated(), true, this->attributes)),
		targetWorldName(new Variant(ExitComponent::AttrTargetWorldName(), Ogre::String(""), this->attributes)),
		targetLocationName(new Variant(ExitComponent::AttrTargetLocationName(), Ogre::String(""), this->attributes)),
		sourceGameObjectId(new Variant(ExitComponent::AttrSourceId(), static_cast<unsigned long>(0), this->attributes, true)),
		exitDirection(new Variant(ExitComponent::AttrExitDirection(), Ogre::Vector2::ZERO, this->attributes)),
		axis(new Variant(ExitComponent::AttrAxis(), std::vector<Ogre::String>{ "X,Y", "X,Z" }, this->attributes)),
		sourceGameObject(nullptr),
		processAlreadyAttached(false)
	{
		this->targetWorldName->setDescription("Sets the target world name, that should be loaded.");
		this->targetLocationName->setDescription("Sets the target location name (GameObject name with exit component in target world name), at which the source game object should be placed.");
		this->sourceGameObjectId->setDescription("The id for the source game object for target location placement.");
		this->exitDirection->setDescription("The exit direction at which the source game object will leave the current world.");
		this->axis->setDescription("The axis for exit direction. For Jump'n'Run e.g. 'X,Y' is correct and for a casual 3D world 'X,Z'.");
		
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &ExitComponent::handlePhysicsTrigger), EventPhysicsTrigger::getStaticEventType());
	}

	ExitComponent::~ExitComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ExitComponent] Destructor exit component for game object: " + this->gameObjectPtr->getName());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ExitComponent::handlePhysicsTrigger), EventPhysicsTrigger::getStaticEventType());
	}

	bool ExitComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttrib(propertyElement, "data", ""));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetWorldName")
		{
			this->targetWorldName->setValue(XMLConverter::getAttrib(propertyElement, "data", ""));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetLocationName")
		{
			this->targetLocationName->setValue(XMLConverter::getAttrib(propertyElement, "data", ""));
			propertyElement = propertyElement->next_sibling("property");
		}
// Attention: SourceGameObjectId will change in next world!?
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
		AppStateManager::getSingletonPtr()->getGameProgressModule()->addWorld(Core::getSingletonPtr()->getProjectName() + "/" + Core::getSingletonPtr()->getFileNameFromPath(Core::getSingletonPtr()->getCurrentWorldPath()), 
			this->targetWorldName->getString(), this->targetLocationName->getString(), this->exitDirection->getVector2(), 
			this->gameObjectPtr->getPosition(), this->axis->getListSelectedValue() == "X,Y" ? true : false);
		return true;
	}
	
	bool ExitComponent::connect(void)
	{
		this->gameObjectPtr->getSceneNode()->setVisible(false);

		if (this->sourceGameObjectId != 0)
		{
			auto& sourceGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->sourceGameObjectId->getULong());
			
			if (nullptr != sourceGameObjectPtr)
			{
				this->sourceGameObject = sourceGameObjectPtr.get();
				// Announce the player to game progress module
				AppStateManager::getSingletonPtr()->getGameProgressModule()->setPlayerName(this->sourceGameObject->getName());
			}
			else
			{
				Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[ExitComponent] Error: Can not create exit component, because the source game object id: '" 
					+ Ogre::StringConverter::toString(this->sourceGameObjectId->getULong()) + "' does not exist.");
				return false;
			}
			AppStateManager::getSingletonPtr()->getGameProgressModule()->addWorld(this->getCurrentWorldName(), this->targetWorldName->getString(), this->targetLocationName->getString(), 
				this->exitDirection->getVector2(), this->gameObjectPtr->getPosition(), this->axis->getListSelectedValue() == "X,Y" ? true : false);
		}
		else
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[ExitComponent] Error: Can not create exit component, because the source game object name is empty.");
			return false;
		}
		return true;
	}

	bool ExitComponent::disconnect(void)
	{
		this->sourceGameObject = nullptr;
		this->gameObjectPtr->getSceneNode()->setVisible(true);
		return true;
	}

	bool ExitComponent::onCloned(void)
	{
		// Nothing to clone?
		/*// Search for the prior id of the cloned game object and set the new id and set the new id, if not found set better 0, else the game objects may be corrupt!
		auto& sourceGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getClonedGameObjectFromPriorId(this->sourceGameObjectId->getULong());
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
		if (visitorGameObjectId == this->gameObjectPtr->getId())
		{
			if (true == entered)
			{
				if (false == this->targetWorldName->getString().empty())
				{
					// Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "------------->[handlePhysicsTrigger");
					AppStateManager::getSingletonPtr()->getGameProgressModule()->changeWorld(this->targetWorldName->getString());
					this->processAlreadyAttached = true;
				}
			}
		}
	}

	void ExitComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == this->processAlreadyAttached && false == notSimulating && true == this->activated->getBool() && nullptr != this->sourceGameObject)
		{
			Ogre::Real distanceToGoal = 1.0f;
			if (0.0f != this->exitDirection->getVector2().x)
			{
				distanceToGoal = Ogre::Math::Abs((this->getPosition().x - this->gameObjectPtr->getBottomOffset().x) - this->sourceGameObject->getPosition().x);
			}
			
			if ("X,Y" == this->axis->getListSelectedValue())
			{
				if (0.0f != this->exitDirection->getVector2().y)
				{
					distanceToGoal = Ogre::Math::Abs((this->getPosition().y - this->gameObjectPtr->getBottomOffset().y) - this->sourceGameObject->getPosition().y);
				}
			}
			else
			{
				if (0.0f != this->exitDirection->getVector3().z)
				{
					distanceToGoal = Ogre::Math::Abs((this->getPosition().z - this->gameObjectPtr->getBottomOffset().z) - this->sourceGameObject->getPosition().z);
				}
			}
			
			// Newton ragdoll may become corrupt and pos will be nan and so new level would be loaded
			if (distanceToGoal <= 0.2f && !isnan(distanceToGoal))
			{
				if (false == this->targetWorldName->getString().empty())
				{
					// change application state to target one
					AppStateManager::getSingletonPtr()->getGameProgressModule()->changeWorld(this->targetWorldName->getString());
					this->processAlreadyAttached = true;
				}
			}
		}
	}

	void ExitComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (ExitComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (ExitComponent::AttrTargetWorldName() == attribute->getName())
		{
			this->targetWorldName->setValue(attribute->getString());
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

	void ExitComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		GameObjectComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->activated->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TargetWorldName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->targetWorldName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TargetLocationName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->targetLocationName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SourceGameObjectId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->sourceGameObjectId->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ExitDirection"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->exitDirection->getVector2())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Axis"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->axis->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String ExitComponent::getTargetWorldName(void) const
	{
		return this->targetWorldName->getString();
	}

	Ogre::String ExitComponent::getCurrentWorldName(void) const
	{
		return AppStateManager::getSingletonPtr()->getGameProgressModule()->getCurrentWorldName();
	}

	Ogre::Vector2 ExitComponent::getExitDirection(void) const
	{
		return this->exitDirection->getVector2();
	}

	Ogre::String ExitComponent::getAxis(void) const
	{
		return this->exitDirection->getListSelectedValue();
	}
}; // namespace end
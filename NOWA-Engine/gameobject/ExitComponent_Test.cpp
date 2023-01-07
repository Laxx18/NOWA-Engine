#include "NOWAPrecompiled.h"
#include "ExitComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/GameProgressModule.h"
#include "gameobject/GameObjectController.h"
#include "main/ProcessManager.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;

	ExitComponent::ExitComponent()
		: GameObjectComponent(),
		activated(new Variant(ExitComponent::AttrActivated(), true, this->attributes)),
		targetAppStateName(new Variant(ExitComponent::AttrTargetAppStateName(), Ogre::String(""), this->attributes)),
		targetWorldName(new Variant(ExitComponent::AttrTargetWorldName(), Ogre::String(""), this->attributes)),
		targetLocationName(new Variant(ExitComponent::AttrTargetLocationName(), Ogre::String(""), this->attributes)),
		sourceGameObjectId(new Variant(ExitComponent::AttrSourceId(), static_cast<unsigned long>(0), this->attributes)),
		exitDirection(new Variant(ExitComponent::AttrExitDirection(), Ogre::Vector3::ZERO, this->attributes)),
		sourceGameObject(nullptr),
		processAlreadyAttached(false)
	{

	}

	ExitComponent::~ExitComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ExitComponent] Destructor exit component for game object: " + this->gameObjectPtr->getName());
	}

	bool ExitComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		propertyElement = propertyElement->next_sibling("property");
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttrib(propertyElement, "data", ""));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetAppStateName")
		{
			this->targetAppStateName->setValue(XMLConverter::getAttrib(propertyElement, "data", ""));
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
			this->exitDirection->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	bool ExitComponent::postInit(void)
	{
		
		return true;
	}
	
	bool ExitComponent::connect(void)
	{
		this->gameObjectPtr->getSceneNode()->setVisible(false);

		if (this->sourceGameObjectId != 0)
		{
			auto& sourceGameObjectPtr = GameObjectController::getInstance()->getGameObjectFromId(this->sourceGameObjectId->getULong());
			
			if (nullptr != sourceGameObjectPtr)
			{
				this->sourceGameObject = sourceGameObjectPtr.get();
				// Announce the player to game progress module
				GameProgressModule::getInstance()->setPlayerName(this->sourceGameObject->getName());
			}
			else
			{
				Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[ExitComponent] Error: Can not create exit component, because the source game object id: '" 
					+ Ogre::StringConverter::toString(this->sourceGameObjectId->getULong()) + "' does not exist.");
				return false;
			}
// Attention: How to get current world, and is it necessary?
			GameProgressModule::getInstance()->addAppState(AppStateManager::getSingletonPtr()->getCurrentAppStateName(), this->getCurrentWorldName(), 
						this->targetAppStateName->getString(), this->targetWorldName->getString(), this->targetLocationName->getString(), this->exitDirection->getVector3(), this->gameObjectPtr->getPosition());
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

	GameObjectCompPtr ExitComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		// Nothing to clone

		return ExitCompPtr();
	}

	void ExitComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == this->processAlreadyAttached && true == notSimulating && true == this->activated->getBool())
		{
			Ogre::Real distanceToGoal = 1.0f;
			if (0.0f != this->exitDirection->getVector3().x)
			{
				distanceToGoal = Ogre::Math::Abs(this->getPosition().x - this->sourceGameObject->getPosition().x);
			}
			else if (0.0f != this->exitDirection->getVector3().y)
			{
				distanceToGoal = Ogre::Math::Abs(this->getPosition().y - this->sourceGameObject->getPosition().y);
			}
			else if (0.0f != this->exitDirection->getVector3().z)
			{
				distanceToGoal = Ogre::Math::Abs(this->getPosition().z - this->sourceGameObject->getPosition().z);
			}
			// change application state to target one
			if (distanceToGoal <= 0.1f)
			{
				if (false == this->targetWorldName->getString().empty())
				{
					// GameProgressModule::getInstance()->addAppState(AppStateManager::getSingletonPtr()->getCurrentAppStateName(), GameProgressModule::getInstance()->getWorldName(), 
					// 	this->targetAppStateName->getString(), this->targetWorldName->getString(), this->targetLocationName->getString(), this->exitDirection->getVector3(), this->gameObjectPtr->getPosition());
					GameProgressModule::getInstance()->loadWorld(this->targetAppStateName->getString(), this->targetWorldName->getString());
					this->processAlreadyAttached = true;
				}
			}
		}
	}

	void ExitComponent::actualizeValue(Variant* attribute)
	{
		if (ExitComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (ExitComponent::AttrTargetAppStateName() == attribute->getName())
		{
			this->targetWorldName->setValue(attribute->getString());
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
			this->exitDirection->setValue(attribute->getVector3());
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
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ComponentExit"));
		propertyXML->append_attribute(doc.allocate_attribute("data", "ExitComponent"));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->activated->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TargetAppStateName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->targetAppStateName->getString())));
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ExitDirection"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->exitDirection->getVector3())));
		propertiesXML->append_node(propertyXML);
	}
	
	Ogre::String ExitComponent::getTargetAppStateName(void) const
	{
		return this->targetAppStateName->getString();
	}

	Ogre::String ExitComponent::getTargetWorldName(void) const
	{
		return this->targetWorldName->getString();
	}
	
	Ogre::String ExitComponent::getCurrentAppStateName(void) const
	{
		return AppStateManager::getSingletonPtr()->getCurrentAppStateName();
	}

	Ogre::String ExitComponent::getCurrentWorldName(void) const
	{
		return GameProgressModule::getInstance()->getCurrentWorldName();
	}

	Ogre::Vector3 ExitComponent::getExitDirection(void) const
	{
		return this->exitDirection->getVector3();
	}

}; // namespace end
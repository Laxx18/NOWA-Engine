#include "NOWAPrecompiled.h"
#include "TimeTriggerComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"

#include "OgreAbiUtils.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	TimeTriggerComponent::TimeTriggerComponent()
		: GameObjectComponent(),
		name("TimeTriggerComponent"),
		timeDt(0.0f),
		startCounting(false),
		firstTimeActivated(true),
		activated(new Variant(TimeTriggerComponent::AttrActivated(), true, this->attributes)),
		activateOne(new Variant(TimeTriggerComponent::AttrActivateOne(), true, this->attributes)),
		startTime(new Variant(TimeTriggerComponent::AttrStartTime(), 10.0f, this->attributes)),
		duration(new Variant(TimeTriggerComponent::AttrDuration(), 10.0f, this->attributes)),
		repeat(new Variant(TimeTriggerComponent::AttrRepeat(), false, this->attributes)),
		deactivateAfterwards(new Variant(TimeTriggerComponent::AttrDeactivateAfterwards(), true, this->attributes))
	{
		this->activateOne->setDescription("Sets whether to activate just the one prior (predecessor) component, which is one above this component, or activate all components for this game object.");
		this->startTime->setDescription("Sets the start time in Seconds, at which the component immediately above should be activated.");
		this->duration->setDescription("Sets the duration in Seconds, for which the component immediately above should remain activated.");
		this->repeat->setDescription("Sets whether time triggering the component immediately above should be repeated.");
		this->deactivateAfterwards->setDescription("Sets whether to deactivate the component above after the time is over or let the component remain activated.");
	}

	TimeTriggerComponent::~TimeTriggerComponent(void)
	{
		
	}

	const Ogre::String& TimeTriggerComponent::getName() const
	{
		return this->name;
	}

	void TimeTriggerComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<TimeTriggerComponent>(TimeTriggerComponent::getStaticClassId(), TimeTriggerComponent::getStaticClassName());
	}
	
	void TimeTriggerComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool TimeTriggerComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ActivateOne")
		{
			this->activateOne->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "StartTime")
		{
			this->startTime->setValue(XMLConverter::getAttribReal(propertyElement, "data", 10.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Duration")
		{
			this->duration->setValue(XMLConverter::getAttribReal(propertyElement, "data", 10.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Repeat")
		{
			this->repeat->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DeactivateAfterwards")
		{
			this->deactivateAfterwards->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}

		return true;
	}

	GameObjectCompPtr TimeTriggerComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		TimeTriggerComponentPtr clonedCompPtr(boost::make_shared<TimeTriggerComponent>());

		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setActivateOne(this->activateOne->getBool());
		clonedCompPtr->setStartTime(this->startTime->getReal());
		clonedCompPtr->setDuration(this->duration->getReal());
		clonedCompPtr->setRepeat(this->repeat->getBool());
		clonedCompPtr->setDeactivateAfterwards(this->deactivateAfterwards->getBool());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool TimeTriggerComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[TimeTriggerComponent] Init node component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	void TimeTriggerComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			if (false == this->activated->getBool())
			{
				return;
			}

			if (true == this->startCounting)
			{
				this->timeDt += dt;
				if (this->timeDt >= this->startTime->getReal())
				{
					if (true == this->firstTimeActivated)
					{
						this->activateComponent(true);
						this->firstTimeActivated = false;
					}
					if (this->timeDt >= this->startTime->getReal() + this->duration->getReal())
					{
						if (true == this->deactivateAfterwards->getBool())
						{
							this->activateComponent(false);
						}
						this->timeDt = 0.0f;
						this->firstTimeActivated = true;
						if (false == this->repeat->getBool())
						{
							this->startCounting = false;
						}
					}
				}
			}
		}
	}

	void TimeTriggerComponent::activateComponent(bool bActivate)
	{
		for (size_t i = 0; i < this->gameObjectPtr->getComponents()->size(); i++)
		{
			// Working here with shared_ptrs is evil, because of bidirectional referecing
			auto component = std::get<COMPONENT>(this->gameObjectPtr->getComponents()->at(i)).get();
			// Seek for this component and go to the previous one to process
			if (true == this->activateOne->getBool())
			{
				if (component == this)
				{
					if (i > 0)
					{
						component = std::get<COMPONENT>(this->gameObjectPtr->getComponents()->at(i - 1)).get();
						component->setActivated(bActivate);
						break;
					}
				}
			}
			else
			{
				// Activate all components at once
				component = std::get<COMPONENT>(this->gameObjectPtr->getComponents()->at(i)).get();
				if (component != this)
				{
					component->setActivated(bActivate);
				}
			}
		}
	}

	void TimeTriggerComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (TimeTriggerComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (TimeTriggerComponent::AttrActivateOne() == attribute->getName())
		{
			this->setActivateOne(attribute->getBool());
		}
		else if (TimeTriggerComponent::AttrStartTime() == attribute->getName())
		{
			this->setStartTime(attribute->getReal());
		}
		else if (TimeTriggerComponent::AttrDuration() == attribute->getName())
		{
			this->setDuration(attribute->getReal());
		}
		else if (TimeTriggerComponent::AttrRepeat() == attribute->getName())
		{
			this->setRepeat(attribute->getBool());
		}
		else if (TimeTriggerComponent::AttrDeactivateAfterwards() == attribute->getName())
		{
			this->setDeactivateAfterwards(attribute->getBool());
		}
	}

	void TimeTriggerComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "ActivateOne"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activateOne->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "StartTime"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->startTime->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Duration"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->duration->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Repeat"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->repeat->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DeactivateAfterwards"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->deactivateAfterwards->getBool())));
		propertiesXML->append_node(propertyXML);
	}

	bool TimeTriggerComponent::connect(void)
	{
		// First always deactivate
		this->activateComponent(false);
		this->startCounting = true;
		this->timeDt = 0.0f;
		return true;
	}

	bool TimeTriggerComponent::disconnect(void)
	{
		this->startCounting = false;
		this->timeDt = 0.0f;
		this->firstTimeActivated = true;
		// Activate, when simulation is not active, else maybe nothing would be visible
		if (false == this->deactivateAfterwards->getBool())
		{
			this->activateComponent(true);
		}
		return true;
	}

	void TimeTriggerComponent::setActivateOne(bool activateOne)
	{
		this->activateOne->setValue(activateOne);
	}

	bool TimeTriggerComponent::getActivateOne(void) const
	{
		return this->activateOne->getBool();
	}

	Ogre::String TimeTriggerComponent::getClassName(void) const
	{
		return "TimeTriggerComponent";
	}

	Ogre::String TimeTriggerComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void TimeTriggerComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
		if (false == activated)
		{
			this->timeDt = 0.0f;
			this->firstTimeActivated = true;
			this->startCounting = true;
		}
	}

	bool TimeTriggerComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void TimeTriggerComponent::setStartTime(Ogre::Real startTime)
	{
		this->startTime->setValue(startTime);
	}

	Ogre::Real TimeTriggerComponent::getStartTime(void) const
	{
		return this->startTime->getReal();
	}

	void TimeTriggerComponent::setDuration(Ogre::Real duration)
	{
		this->duration->setValue(duration);
	}

	Ogre::Real TimeTriggerComponent::getDuration(void) const
	{
		return this->duration->getReal();
	}

	void TimeTriggerComponent::setRepeat(bool repeat)
	{
		this->repeat->setValue(repeat);
	}

	bool TimeTriggerComponent::getRepeat(void) const
	{
		return this->repeat->getBool();
	}

	void TimeTriggerComponent::setDeactivateAfterwards(bool deactivateAfterwards)
	{
		this->deactivateAfterwards->setValue(deactivateAfterwards);
	}

	bool TimeTriggerComponent::getDeactivateAfterwards(void) const
	{
		return this->deactivateAfterwards->getBool();
	}

	// Lua registration part

	TimeTriggerComponent* getTimeTriggerComponentFromIndex(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<TimeTriggerComponent>(gameObject->getComponentWithOccurrence<TimeTriggerComponent>(occurrenceIndex)).get();
	}

	TimeTriggerComponent* getTimeTriggerComponent(GameObject* gameObject)
	{
		return makeStrongPtr<TimeTriggerComponent>(gameObject->getComponent<TimeTriggerComponent>()).get();
	}

	TimeTriggerComponent* getTimeTriggerComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<TimeTriggerComponent>(gameObject->getComponentFromName<TimeTriggerComponent>(name)).get();
	}

	void TimeTriggerComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<TimeTriggerComponent, GameObjectComponent>("TimeTriggerComponent")
			.def("setActivated", &TimeTriggerComponent::setActivated)
			.def("isActivated", &TimeTriggerComponent::isActivated)
		];

		LuaScriptApi::getInstance()->addClassToCollection("TimeTriggerComponent", "class inherits GameObjectComponent", TimeTriggerComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("TimeTriggerComponent", "void setActivated(bool activated)", "Sets whether time trigger can start or not.");
		LuaScriptApi::getInstance()->addClassToCollection("TimeTriggerComponent", "bool isActivated()", "Gets whether this time trigger is activated or not.");

		gameObjectClass.def("getTimeTriggerComponentFromName", &getTimeTriggerComponentFromName);
		gameObjectClass.def("getTimeTriggerComponent", (TimeTriggerComponent * (*)(GameObject*)) & getTimeTriggerComponent);
		// If its desired to create several of this components for one game object
		gameObjectClass.def("getTimeTriggerComponentFromIndex", (TimeTriggerComponent * (*)(GameObject*, unsigned int)) & getTimeTriggerComponentFromIndex);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "TimeTriggerComponent getTimeTriggerComponentFromIndex(unsigned int occurrenceIndex)", "Gets the component by the given occurence index, since a game object may this component maybe several times.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "TimeTriggerComponent getTimeTriggerComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "TimeTriggerComponent getTimeTriggerComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castTimeTriggerComponent", &GameObjectController::cast<TimeTriggerComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "TimeTriggerComponent castTimeTriggerComponent(TimeTriggerComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool TimeTriggerComponent::canStaticAddComponent(GameObject* gameObject)
	{
		return true;
	}

}; //namespace end
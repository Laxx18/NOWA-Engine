#include "NOWAPrecompiled.h"
#include "PluginTemplate.h"
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

	PluginTemplate::PluginTemplate()
		: GameObjectComponent(),
		name("PluginTemplate"),
		activated(new Variant(PluginTemplate::AttrActivated(), true, this->attributes))
	{
		this->activated->setDescription("Description.");
	}

	PluginTemplate::~PluginTemplate(void)
	{
		
	}

	const Ogre::String& PluginTemplate::getName() const
	{
		return this->name;
	}

	void PluginTemplate::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<PluginTemplate>(PluginTemplate::getStaticClassId(), PluginTemplate::getStaticClassName());
	}
	
	void PluginTemplate::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool PluginTemplate::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		/*if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ShortTimeActivation")
		{
			this->shortTimeActivation->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}*/
		
		return true;
	}

	GameObjectCompPtr PluginTemplate::clone(GameObjectPtr clonedGameObjectPtr)
	{
		PluginTemplatePtr clonedCompPtr(boost::make_shared<PluginTemplate>());

		clonedCompPtr->setActivated(this->activated->getBool());
		

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool PluginTemplate::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PluginTemplate] Init component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool PluginTemplate::connect(void)
	{
		GameObjectComponent::connect();
		
		// Hello World test :)
		auto physicsActiveCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsActiveComponent>());
		if (nullptr != physicsActiveCompPtr)
		{
			physicsActiveCompPtr->applyRequiredForceForVelocity(Ogre::Vector3(0.0f, 10.0f, 0.0f));
		}
		else
		{
			this->gameObjectPtr->getSceneNode()->translate(Ogre::Vector3(0.0f, 10.0f, 0.0f));
		}
		
		
		return true;
	}

	bool PluginTemplate::disconnect(void)
	{
		GameObjectComponent::disconnect();
		return true;
	}

	bool PluginTemplate::onCloned(void)
	{
		
		return true;
	}

	void PluginTemplate::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();
	}
	
	void PluginTemplate::onOtherComponentRemoved(unsigned int index)
	{
		
	}
	
	void PluginTemplate::onOtherComponentAdded(unsigned int index)
	{
		
	}
	
	void PluginTemplate::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			// Do something
		}
	}

	void PluginTemplate::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (PluginTemplate::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		/*else if(PluginTemplate::AttrShortTimeActivation() == attribute->getName())
		{
			this->setShortTimeActivation(attribute->getBool());
		}*/
	}

	void PluginTemplate::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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

		/*propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ShortTimeActivation"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->shortTimeActivation->getBool())));
		propertiesXML->append_node(propertyXML);*/
	}

	Ogre::String PluginTemplate::getClassName(void) const
	{
		return "PluginTemplate";
	}

	Ogre::String PluginTemplate::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void PluginTemplate::setActivated(bool activated)
	{
		this->activated->setValue(activated);
	}

	bool PluginTemplate::isActivated(void) const
	{
		return this->activated->getBool();
	}

	// Lua registration part

	PluginTemplate* getPluginTemplate(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<PluginTemplate>(gameObject->getComponentWithOccurrence<PluginTemplate>(occurrenceIndex)).get();
	}

	PluginTemplate* getPluginTemplate(GameObject* gameObject)
	{
		return makeStrongPtr<PluginTemplate>(gameObject->getComponent<PluginTemplate>()).get();
	}

	PluginTemplate* getPluginTemplateFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<PluginTemplate>(gameObject->getComponentFromName<PluginTemplate>(name)).get();
	}

	void PluginTemplate::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<PluginTemplate, GameObjectComponent>("PluginTemplate")
			.def("setActivated", &PluginTemplate::setActivated)
			.def("isActivated", &PluginTemplate::isActivated)
		];

		LuaScriptApi::getInstance()->addClassToCollection("PluginTemplate", "class inherits GameObjectComponent", PluginTemplate::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("PluginTemplate", "void setActivated(bool activated)", "Sets whether this component should be activated or not.");
		LuaScriptApi::getInstance()->addClassToCollection("PluginTemplate", "bool isActivated()", "Gets whether this component is activated.");
		LuaScriptApi::getInstance()->addClassToCollection("PluginTemplate", "GameObject getOwner()", "Gets the owner game object.");

		gameObjectClass.def("getPluginTemplateFromName", &getPluginTemplateFromName);
		gameObjectClass.def("getPluginTemplate", (PluginTemplate * (*)(GameObject*)) & getPluginTemplate);
		// If its desired to create several of this components for one game object
		gameObjectClass.def("getPluginTemplateFromIndex", (PluginTemplate * (*)(GameObject*, unsigned int)) & getPluginTemplate);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PluginTemplate getPluginTemplateFromIndex(unsigned int occurrenceIndex)", "Gets the component by the given occurence index, since a game object may this component maybe several times.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PluginTemplate getPluginTemplate()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PluginTemplate getPluginTemplateFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castPluginTemplate", &GameObjectController::cast<PluginTemplate>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "PluginTemplate castPluginTemplate(PluginTemplate other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool PluginTemplate::canStaticAddComponent(GameObject* gameObject)
	{
		// No constraints so far, just add
		return true;
	}

}; //namespace end
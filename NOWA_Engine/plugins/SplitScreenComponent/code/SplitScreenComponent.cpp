#include "NOWAPrecompiled.h"
#include "SplitScreenComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"
#include "modules/WorkspaceModule.h"

#include "OgreAbiUtils.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	SplitScreenComponent::SplitScreenComponent()
		: GameObjectComponent(),
		name("SplitScreenComponent"),
		activated(new Variant(SplitScreenComponent::AttrActivated(), true, this->attributes))
	{
		this->activated->setDescription("Description.");
	}

	SplitScreenComponent::~SplitScreenComponent(void)
	{
		
	}

	const Ogre::String& SplitScreenComponent::getName() const
	{
		return this->name;
	}

	void SplitScreenComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<SplitScreenComponent>(SplitScreenComponent::getStaticClassId(), SplitScreenComponent::getStaticClassName());
	}
	
	void SplitScreenComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool SplitScreenComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		// Priority connect!
		this->bConnectPriority = true;

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

	GameObjectCompPtr SplitScreenComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		return SplitScreenComponentPtr();
	}

	bool SplitScreenComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[SplitScreenComponent] Init component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool SplitScreenComponent::connect(void)
	{	
		// Use only in simulation not on editor mode!
		WorkspaceModule::getInstance()->setUseSplitScreen(true == this->activated->getBool());
		return true;
	}

	bool SplitScreenComponent::disconnect(void)
	{
		// Do not create a work space if engine is being shut down
		if (false == AppStateManager::getSingletonPtr()->getIsShutdown())
		{
			WorkspaceModule::getInstance()->setUseSplitScreen(false);
		}
		return true;
	}

	bool SplitScreenComponent::onCloned(void)
	{
		
		return true;
	}

	void SplitScreenComponent::onRemoveComponent(void)
	{
		// TODO: What if editor is exited, in this case nothing must be done!
		if (false == AppStateManager::getSingletonPtr()->getIsShutdown())
		{
			WorkspaceModule::getInstance()->setUseSplitScreen(false);
		}
	}
	
	void SplitScreenComponent::onOtherComponentRemoved(unsigned int index)
	{
		
	}
	
	void SplitScreenComponent::onOtherComponentAdded(unsigned int index)
	{
		
	}
	
	void SplitScreenComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			// Do something
		}
	}

	void SplitScreenComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (SplitScreenComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
	}

	void SplitScreenComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
		propertiesXML->append_node(propertyXML);	
	}

	Ogre::String SplitScreenComponent::getClassName(void) const
	{
		return "SplitScreenComponent";
	}

	Ogre::String SplitScreenComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void SplitScreenComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
	}

	bool SplitScreenComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	// Lua registration part

	SplitScreenComponent* getSplitScreenComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<SplitScreenComponent>(gameObject->getComponentWithOccurrence<SplitScreenComponent>(occurrenceIndex)).get();
	}

	SplitScreenComponent* getSplitScreenComponent(GameObject* gameObject)
	{
		return makeStrongPtr<SplitScreenComponent>(gameObject->getComponent<SplitScreenComponent>()).get();
	}

	SplitScreenComponent* getSplitScreenComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<SplitScreenComponent>(gameObject->getComponentFromName<SplitScreenComponent>(name)).get();
	}

	void SplitScreenComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<SplitScreenComponent, GameObjectComponent>("SplitScreenComponent")
			.def("setActivated", &SplitScreenComponent::setActivated)
			.def("isActivated", &SplitScreenComponent::isActivated)
		];

		LuaScriptApi::getInstance()->addClassToCollection("SplitScreenComponent", "class inherits GameObjectComponent", SplitScreenComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("SplitScreenComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not.");
		LuaScriptApi::getInstance()->addClassToCollection("SplitScreenComponent", "bool isActivated()", "Gets whether this component is activated.");

		gameObjectClass.def("getSplitScreenComponentFromName", &getSplitScreenComponentFromName);
		gameObjectClass.def("getSplitScreenComponent", (SplitScreenComponent * (*)(GameObject*)) & getSplitScreenComponent);
		// If its desired to create several of this components for one game object
		gameObjectClass.def("getSplitScreenComponent2", (SplitScreenComponent * (*)(GameObject*, unsigned int)) & getSplitScreenComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "SplitScreenComponent getSplitScreenComponent2(unsigned int occurrenceIndex)", "Gets the component by the given occurence index, since a game object may this component maybe several times.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "SplitScreenComponent getSplitScreenComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "SplitScreenComponent getSplitScreenComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castSplitScreenComponent", &GameObjectController::cast<SplitScreenComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "SplitScreenComponent castSplitScreenComponent(SplitScreenComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool SplitScreenComponent::canStaticAddComponent(GameObject* gameObject)
	{
		auto selectGameObjectsCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<SplitScreenComponent>());
		// Constraints: Can only be placed under a main game object and only once
		if (gameObject->getId() == GameObjectController::MAIN_GAMEOBJECT_ID && nullptr == selectGameObjectsCompPtr)
		{
			return true;
		}
		return false;
	}

}; //namespace end
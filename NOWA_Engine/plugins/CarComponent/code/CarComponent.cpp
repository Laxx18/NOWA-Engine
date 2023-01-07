#include "NOWAPrecompiled.h"
#include "CarComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	CarComponent::CarComponent()
		: GameObjectComponent(),
		name("CarComponent"),
		activated(new Variant(CarComponent::AttrActivated(), true, this->attributes))
	{
		this->activated->setDescription("Description.");
	}

	CarComponent::~CarComponent(void)
	{
		
	}

	void CarComponent::initialise()
	{

	}

	const Ogre::String& CarComponent::getName() const
	{
		return this->name;
	}

	void CarComponent::install()
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<CarComponent>(CarComponent::getStaticClassId(), CarComponent::getStaticClassName());
	}

	void CarComponent::shutdown()
	{

	}

	void CarComponent::uninstall()
	{

	}

	bool CarComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

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

	GameObjectCompPtr CarComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		CarComponentPtr clonedCompPtr(boost::make_shared<CarComponent>());

		clonedCompPtr->setActivated(this->activated->getBool());
		

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool CarComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CarComponent] Init component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool CarComponent::connect(void)
	{
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

	bool CarComponent::disconnect(void)
	{
		
		return true;
	}

	bool CarComponent::onCloned(void)
	{
		
		return true;
	}

	void CarComponent::onRemoveComponent(void)
	{
		
	}

	void CarComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			// Do something
		}
	}

	void CarComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (CarComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		/*else if(CarComponent::AttrShortTimeActivation() == attribute->getName())
		{
			this->setShortTimeActivation(attribute->getBool());
		}*/
	}

	void CarComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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

		/*propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ShortTimeActivation"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->shortTimeActivation->getBool())));
		propertiesXML->append_node(propertyXML);*/
	}

	Ogre::String CarComponent::getClassName(void) const
	{
		return "CarComponent";
	}

	Ogre::String CarComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void CarComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
	}

	bool CarComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	// Lua registration part

	CarComponent* getCarComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<CarComponent>(gameObject->getComponentWithOccurrence<CarComponent>(occurrenceIndex)).get();
	}

	CarComponent* getCarComponent(GameObject* gameObject)
	{
		return makeStrongPtr<CarComponent>(gameObject->getComponent<CarComponent>()).get();
	}

	CarComponent* getCarComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<CarComponent>(gameObject->getComponentFromName<CarComponent>(name)).get();
	}

	void CarComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<CarComponent, GameObjectComponent>("CarComponent")
			.def("setActivated", &CarComponent::setActivated)
			.def("isActivated", &CarComponent::isActivated)
		];

		LuaScriptApi::getInstance()->addClassToCollection("CarComponent", "class inherits GameObjectComponent", CarComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("CarComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not.");
		LuaScriptApi::getInstance()->addClassToCollection("CarComponent", "bool isActivated()", "Gets whether this component is activated.");

		gameObjectClass.def("getCarComponentFromName", &getCarComponentFromName);
		gameObjectClass.def("getCarComponent", (CarComponent * (*)(GameObject*)) & getCarComponent);
		// If its desired to create several of this components for one game object
		gameObjectClass.def("getCarComponent2", (CarComponent * (*)(GameObject*, unsigned int)) & getCarComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "AnimationComponent getCarComponent2(unsigned int occurrenceIndex)", "Gets the plugin template component by the given occurence index, since a game object may have besides other components several area of interest components.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "AnimationComponent getCarComponent()", "Gets the plugin template component. This can be used if the game object just has one area of plugin template component.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "AnimationComponent getCarComponentFromName(String name)", "Gets the plugin template component.");

		gameObjectControllerClass.def("castCarComponent", &GameObjectController::cast<CarComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "CarComponent castCarComponent(CarComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool CarComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// No constraints so far, just add
		return true;
	}

}; //namespace end
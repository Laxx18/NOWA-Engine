#include "NOWAPrecompiled.h"
#include "InputDeviceComponent.h"
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

	InputDeviceComponent::InputDeviceComponent()
		: GameObjectComponent(),
		name("InputDeviceComponent"),
		inputDeviceModule(nullptr),
		bValidDevice(false),
		activated(new Variant(InputDeviceComponent::AttrActivated(), true, this->attributes)),
		deviceName(new Variant(InputDeviceComponent::AttrDeviceName(), std::vector<Ogre::String>(), this->attributes))

	{
		this->activated->setDescription("Activates the device for this owner game object.");
	}

	InputDeviceComponent::~InputDeviceComponent(void)
	{
		
	}

	const Ogre::String& InputDeviceComponent::getName() const
	{
		return this->name;
	}

	void InputDeviceComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<InputDeviceComponent>(InputDeviceComponent::getStaticClassId(), InputDeviceComponent::getStaticClassName());
	}
	
	void InputDeviceComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool InputDeviceComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == InputDeviceComponent::AttrDeviceName())
		{
			this->deviceName->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", ""));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr InputDeviceComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		return nullptr;
	}

	bool InputDeviceComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[InputDeviceComponent] Init component for game object: " + this->gameObjectPtr->getName());

		this->setActivated(this->activated->getBool());

		this->getActualizedDeviceList();

		return true;
	}

	bool InputDeviceComponent::connect(void)
	{
		GameObjectComponent::connect();
		
		return true;
	}

	bool InputDeviceComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();

		InputDeviceCore::getSingletonPtr()->releaseDevice(this->gameObjectPtr->getId());

		return true;
	}

	bool InputDeviceComponent::onCloned(void)
	{
		
		return true;
	}

	void InputDeviceComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		InputDeviceCore::getSingletonPtr()->releaseDevice(this->gameObjectPtr->getId());
		this->inputDeviceModule = nullptr;
		this->bValidDevice = false;
		this->getActualizedDeviceList();
	}
	
	void InputDeviceComponent::onOtherComponentRemoved(unsigned int index)
	{
		
	}
	
	void InputDeviceComponent::onOtherComponentAdded(unsigned int index)
	{
		
	}
	
	void InputDeviceComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			// Do something
		}
	}

	void InputDeviceComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (InputDeviceComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (InputDeviceComponent::AttrDeviceName() == attribute->getName())
		{
			this->setDeviceName(attribute->getListSelectedValue());
		}
	}

	void InputDeviceComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "DeviceName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->deviceName->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String InputDeviceComponent::getClassName(void) const
	{
		return "InputDeviceComponent";
	}

	Ogre::String InputDeviceComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void InputDeviceComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
		if (true == activated)
		{
			this->setDeviceName(this->deviceName->getListSelectedValue());
		}
		else
		{
			InputDeviceCore::getSingletonPtr()->releaseDevice(this->gameObjectPtr->getId());
			this->inputDeviceModule = nullptr;
			this->bValidDevice = false;
			this->getActualizedDeviceList();
		}
	}

	bool InputDeviceComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	Ogre::String InputDeviceComponent::getDeviceName(void) const
	{
		return this->deviceName->getListSelectedValue();
	}

	void InputDeviceComponent::setDeviceName(const Ogre::String& deviceName)
	{
		this->bValidDevice = false;
		if (false == this->activated->getBool())
		{
			return;
		}

		std::vector<Ogre::String> availableDeviceNames = this->getActualizedDeviceList();

		for (const auto& currentDeviceName : availableDeviceNames)
		{
			if (currentDeviceName == deviceName)
			{
				this->inputDeviceModule = InputDeviceCore::getSingletonPtr()->assignDevice(deviceName, this->gameObjectPtr->getId());
				this->deviceName->setListSelectedValue(deviceName);
				this->bValidDevice = this->checkDevice(deviceName);
			}
		}
	}

	std::vector<Ogre::String> InputDeviceComponent::getActualizedDeviceList(void)
	{
		std::vector<Ogre::String> availableDeviceNames;
		for (const auto& joystickInputDeviceModule : InputDeviceCore::getSingletonPtr()->getJoystickInputDeviceModules())
		{
			if (0 == joystickInputDeviceModule->getOccupiedId())
			{
				availableDeviceNames.push_back(joystickInputDeviceModule->getDeviceName());
			}
		}
		for (const auto& keyboardInputDeviceModule : InputDeviceCore::getSingletonPtr()->getKeyboardInputDeviceModules())
		{
			if (0 == keyboardInputDeviceModule->getOccupiedId())
			{
				availableDeviceNames.push_back(keyboardInputDeviceModule->getDeviceName());
			}
		}

		if (true == availableDeviceNames.empty())
		{
			availableDeviceNames.push_back("No Device Available");
		}
		else
		{
			availableDeviceNames.insert(availableDeviceNames.begin(), "Choose Device");
		}

		this->deviceName->setValue(availableDeviceNames);
		this->deviceName->setUserData(GameObject::AttrActionNeedRefresh());
		return availableDeviceNames;
	}

	NOWA::InputDeviceModule* InputDeviceComponent::getInputDeviceModule(void)
	{
		return this->inputDeviceModule;
	}

	bool InputDeviceComponent::checkDevice(const Ogre::String& deviceName)
	{
		bool valid = deviceName != "Choose Device" && deviceName != "No Device Available";
		if (false == valid)
		{
			return false;
		}
		for (size_t i = 0; i < this->deviceName->getList().size(); i++)
		{
			if (this->deviceName->getList()[i] == deviceName)
			{
				return true;
			}
		}
		return false;
	}

	bool InputDeviceComponent::hasValidDevice(void) const
	{
		return this->bValidDevice;
	}

	// Lua registration part

	InputDeviceComponent* getInputDeviceComponent(GameObject* gameObject)
	{
		return makeStrongPtr<InputDeviceComponent>(gameObject->getComponent<InputDeviceComponent>()).get();
	}

	InputDeviceComponent* getInputDeviceComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<InputDeviceComponent>(gameObject->getComponentFromName<InputDeviceComponent>(name)).get();
	}

	// Only possible way to do it! Here with instance does not work!, Because we are in a plugin!
	luabind::object InputDeviceComponent::getLuaActualizedDeviceList()
	{
		luabind::object obj = luabind::newtable(NOWA::LuaScriptApi::getInstance()->getLua());

		const auto& deviceNames = this->getActualizedDeviceList();

		unsigned int i = 0;
		for (auto& it = deviceNames.cbegin(); it != deviceNames.cend(); ++it)
		{
			obj[i++] = *it;
		}

		return obj;
	}

	void InputDeviceComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<InputDeviceComponent, GameObjectComponent>("InputDeviceComponent")
			.def("setActivated", &InputDeviceComponent::setActivated)
			.def("isActivated", &InputDeviceComponent::isActivated)
			.def("setDeviceName", &InputDeviceComponent::setDeviceName)
			.def("getDeviceName", &InputDeviceComponent::getDeviceName)
			.def("getActualizedDeviceList", &InputDeviceComponent::getLuaActualizedDeviceList)
			.def("getInputDeviceModule", &InputDeviceComponent::getInputDeviceModule)
			.def("checkDevice", &InputDeviceComponent::checkDevice)
			.def("hasValidDevice", &InputDeviceComponent::hasValidDevice)

		];

		LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "class inherits GameObjectComponent", InputDeviceComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "void setActivated(bool activated)", "Activates the device for this owner game object.");
		LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "bool isActivated()", "Gets whether the device is activated for this owner game object.");
		LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "void setDeviceName(string deviceName)", "Sets the given device name. Note: It should only be chosen delivered from the @getActualizedDeviceList().");
		LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "string getDeviceName()", "Gets the selected device name.");
		LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "Table[number][string] getActualizedDeviceList()", "Gets all available devices. If none available or all occupied, 'No Device Available' will be delivered. "
			"By default 'Choose Device' is set, which is an invalid item and should be checked if chosen. Use @checkDevice or @hasValidDevice to check if the given selected device is valid, instead of checking those strings.");
		LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "InputDeviceModule getInputDeviceModule()", "Gets the assigned input device module instance.");
		LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "bool checkDevice(string deviceName)", "Gets for the given device name, whether its a valid device or not.");
		LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "bool hasValidDevice()", "Gets after calling @setDeviceName, whether the set device is valid.");

		gameObjectClass.def("getInputDeviceComponent", (InputDeviceComponent * (*)(GameObject*)) & getInputDeviceComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "InputDeviceComponent getInputDeviceComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "InputDeviceComponent getInputDeviceComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castInputDeviceComponent", &GameObjectController::cast<InputDeviceComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "InputDeviceComponent castInputDeviceComponent(InputDeviceComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool InputDeviceComponent::canStaticAddComponent(GameObject* gameObject)
	{
		if (gameObject->getComponentCount<InputDeviceComponent>() < 2)
		{
			return true;
		}
		return false;
	}

}; //namespace end
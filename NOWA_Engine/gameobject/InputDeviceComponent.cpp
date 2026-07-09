#include "NOWAPrecompiled.h"
#include "InputDeviceComponent.h"
#include "main/AppStateManager.h"
#include "main/EventManager.h"
#include "main/InputDeviceCore.h"
#include "modules/InputDeviceModule.h"
#include "modules/LuaScriptApi.h"
#include "utilities/XMLConverter.h"

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    InputDeviceComponent::InputDeviceComponent() :
        GameObjectComponent(),
        inputDeviceModule(nullptr),
        bValidDevice(false),
        activated(new Variant(InputDeviceComponent::AttrActivated(), true, this->attributes)),
        deviceName(new Variant(InputDeviceComponent::AttrDeviceName(), std::vector<Ogre::String>(), this->attributes)),
        isExclusive(new Variant(InputDeviceComponent::AttrIsExclusive(), false, this->attributes))
    {
        this->activated->setDescription(
            "Activates the device for this owner game object. Note, if there are other game objects with the same device, they will be deactivated and the chosen device reset to 'Choose Device', if isExclusive is set to true. "
            "There can be no two active game objects with the same device at the same time. First one must be deactivated, then the other can use the device. This can also be done for lua if its necessary e.g. to change player control.");

        this->isExclusive->setDescription(
            "Sets whether the chosen device name for this this input device is exclusive, which means no other active input device for a game object may have the same device. This is useful e.g. for splitscreen or multiplayer scenarios. "
            "If set to false, still no two active input devices can have the same device, but if an input device will be activated, then all other game objects with their input devices will just be deactivated, but keep the same input device name. "
            " So its possible to switch between game objects and each time the input device name like keyboard will be used for the yet active input device of the given game object.");

        this->deviceName->addUserData(GameObject::AttrActionNeedRefresh());

        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &InputDeviceComponent::handleInputDeviceOccupied), EventDataInputDeviceOccupied::getStaticEventType());
    }

    InputDeviceComponent::~InputDeviceComponent(void)
    {
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
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "IsExclusive")
        {
            this->isExclusive->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
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

        this->setIsExcluse(this->isExclusive->getBool());

        this->setActivated(this->activated->getBool());

        this->getActualizedDeviceList();

        return true;
    }

    bool InputDeviceComponent::connect(void)
    {
        GameObjectComponent::connect();

        this->setActivated(this->activated->getBool());

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

        boost::shared_ptr<EventDataInputDeviceOccupied> eventDataInputDeviceOccupied(new EventDataInputDeviceOccupied(gameObjectPtr->getId(), false, this->deviceName->getListSelectedValue()));
        AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataInputDeviceOccupied);

        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &InputDeviceComponent::handleInputDeviceOccupied), EventDataInputDeviceOccupied::getStaticEventType());
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
        else if (InputDeviceComponent::AttrIsExclusive() == attribute->getName())
        {
            this->setIsExcluse(attribute->getBool());
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

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "IsExclusive"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->isExclusive->getBool())));
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

            boost::shared_ptr<EventDataInputDeviceOccupied> eventDataInputDeviceOccupied(new EventDataInputDeviceOccupied(gameObjectPtr->getId(), activated, this->deviceName->getListSelectedValue()));
            AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataInputDeviceOccupied);
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

    void InputDeviceComponent::setIsExcluse(bool isExclusive)
    {
        this->isExclusive->setValue(isExclusive);

        // All other components must have the same flag set
        const auto gameObjects = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsFromComponent(InputDeviceComponent::getStaticClassName());
        for (const auto& gameObjectPtr : gameObjects)
        {
            const auto otherInputDeviceComponent = NOWA::makeStrongPtr(gameObjectPtr->getComponent<InputDeviceComponent>());
            if (gameObjectPtr->getId() != this->gameObjectPtr->getId())
            {
                Ogre::String name = gameObjectPtr->getName();

                if (otherInputDeviceComponent->getIsExclusive() != isExclusive)
                {
                    otherInputDeviceComponent->setIsExcluse(isExclusive);
                }
            }
        }
    }

    bool InputDeviceComponent::getIsExclusive(void) const
    {
        return this->isExclusive->getBool();
    }

    void InputDeviceComponent::setDeviceName(const Ogre::String& deviceName)
    {
        this->bValidDevice = false;

        boost::shared_ptr<EventDataInputDeviceOccupied> eventDataInputDeviceOccupied(new EventDataInputDeviceOccupied(gameObjectPtr->getId(), activated, this->deviceName->getListSelectedValue()));
        AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataInputDeviceOccupied);

        Ogre::String oldDeviceName = this->deviceName->getListSelectedOldValue();
        bool valid = oldDeviceName != "Choose Device" && oldDeviceName != "No Device Available" && oldDeviceName != "";
        if (oldDeviceName != deviceName && true == valid)
        {
            InputDeviceCore::getSingletonPtr()->releaseDevice(this->gameObjectPtr->getId());
            this->inputDeviceModule = nullptr;
            this->bValidDevice = false;
        }

        std::vector<Ogre::String> availableDeviceNames = this->getActualizedDeviceList();

        for (const auto& currentDeviceName : availableDeviceNames)
        {
            if (currentDeviceName == deviceName)
            {
                this->inputDeviceModule = InputDeviceCore::getSingletonPtr()->assignDevice(deviceName, this->gameObjectPtr->getId());
                this->deviceName->setListSelectedValue(deviceName);
                this->bValidDevice = this->checkDevice(deviceName);
                break;
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
            if (0 == keyboardInputDeviceModule->getOccupiedId() || keyboardInputDeviceModule->getOccupiedId() == this->gameObjectPtr->getId())
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

    /////////////////////////////////////////////////////////////////////////////
    // Forwarding functions to the internal InputDeviceModule
    // Note: the public header only uses plain int for actions/buttons/keycodes
    // (instead of the real InputDeviceModule::Action / JoyStickButton / OIS::KeyCode
    // types), to avoid a circular include between InputDeviceComponent.h and
    // InputDeviceModule.h (via InputDeviceCore.h). The casts to/from the real
    // enum types happen here, where InputDeviceModule.h is fully included.
    /////////////////////////////////////////////////////////////////////////////

    bool InputDeviceComponent::isKeyboardDevice(void) const
    {
        if (nullptr == this->inputDeviceModule)
        {
            return false;
        }
        return this->inputDeviceModule->isKeyboardDevice();
    }

    int InputDeviceComponent::getMappedKey(int action)
    {
        if (nullptr == this->inputDeviceModule)
        {
            return static_cast<int>(OIS::KC_UNASSIGNED);
        }
        return static_cast<int>(this->inputDeviceModule->getMappedKey(static_cast<InputDeviceModule::Action>(action)));
    }

    Ogre::String InputDeviceComponent::getStringFromMappedKey(int keyCode)
    {
        if (nullptr == this->inputDeviceModule)
        {
            return Ogre::String();
        }
        return this->inputDeviceModule->getStringFromMappedKey(static_cast<OIS::KeyCode>(keyCode));
    }

    int InputDeviceComponent::getMappedKeyFromString(const Ogre::String& key)
    {
        if (nullptr == this->inputDeviceModule)
        {
            return static_cast<int>(OIS::KC_UNASSIGNED);
        }
        return static_cast<int>(this->inputDeviceModule->getMappedKeyFromString(key));
    }

    int InputDeviceComponent::getMappedButton(int action)
    {
        if (nullptr == this->inputDeviceModule)
        {
            return static_cast<int>(InputDeviceModule::BUTTON_NONE);
        }
        return static_cast<int>(this->inputDeviceModule->getMappedButton(static_cast<InputDeviceModule::Action>(action)));
    }

    Ogre::String InputDeviceComponent::getStringFromMappedButton(int joyStickButton)
    {
        if (nullptr == this->inputDeviceModule)
        {
            return Ogre::String();
        }
        return this->inputDeviceModule->getStringFromMappedButton(static_cast<InputDeviceModule::JoyStickButton>(joyStickButton));
    }

    void InputDeviceComponent::setJoyStickDeadZone(Ogre::Real deadZone)
    {
        if (nullptr != this->inputDeviceModule)
        {
            this->inputDeviceModule->setJoyStickDeadZone(deadZone);
        }
    }

    bool InputDeviceComponent::hasActiveJoyStick(void) const
    {
        if (nullptr == this->inputDeviceModule)
        {
            return false;
        }
        return this->inputDeviceModule->hasActiveJoyStick();
    }

    Ogre::Real InputDeviceComponent::getLeftStickHorizontalMovingStrength(void) const
    {
        if (nullptr == this->inputDeviceModule)
        {
            return 0.0f;
        }
        return this->inputDeviceModule->getLeftStickHorizontalMovingStrength();
    }

    Ogre::Real InputDeviceComponent::getLeftStickVerticalMovingStrength(void) const
    {
        if (nullptr == this->inputDeviceModule)
        {
            return 0.0f;
        }
        return this->inputDeviceModule->getLeftStickVerticalMovingStrength();
    }

    Ogre::Real InputDeviceComponent::getRightStickHorizontalMovingStrength(void) const
    {
        if (nullptr == this->inputDeviceModule)
        {
            return 0.0f;
        }
        return this->inputDeviceModule->getRightStickHorizontalMovingStrength();
    }

    Ogre::Real InputDeviceComponent::getRightStickVerticalMovingStrength(void) const
    {
        if (nullptr == this->inputDeviceModule)
        {
            return 0.0f;
        }
        return this->inputDeviceModule->getRightStickVerticalMovingStrength();
    }

    bool InputDeviceComponent::isKeyDown(int keyCode) const
    {
        if (nullptr == this->inputDeviceModule)
        {
            return false;
        }
        return this->inputDeviceModule->isKeyDown(static_cast<OIS::KeyCode>(keyCode));
    }

    bool InputDeviceComponent::isButtonDown(int button) const
    {
        if (nullptr == this->inputDeviceModule)
        {
            return false;
        }
        return this->inputDeviceModule->isButtonDown(static_cast<InputDeviceModule::JoyStickButton>(button));
    }

    bool InputDeviceComponent::isActionDown(int action)
    {
        if (nullptr == this->inputDeviceModule)
        {
            return false;
        }
        return this->inputDeviceModule->isActionDown(static_cast<InputDeviceModule::Action>(action));
    }

    bool InputDeviceComponent::isActionDownAmount(int action, Ogre::Real dt, Ogre::Real actionDuration)
    {
        if (nullptr == this->inputDeviceModule)
        {
            return false;
        }
        return this->inputDeviceModule->isActionDownAmount(static_cast<InputDeviceModule::Action>(action), dt, actionDuration);
    }

    bool InputDeviceComponent::isActionDownPressed(int action, Ogre::Real dt, Ogre::Real durationBetweenTheAction)
    {
        if (nullptr == this->inputDeviceModule)
        {
            return false;
        }
        return this->inputDeviceModule->isActionPressed(static_cast<InputDeviceModule::Action>(action), dt, durationBetweenTheAction);
    }

    bool InputDeviceComponent::areButtonsDown2(int button1, int button2) const
    {
        if (nullptr == this->inputDeviceModule)
        {
            return false;
        }
        return this->inputDeviceModule->areButtonsDown2(static_cast<InputDeviceModule::JoyStickButton>(button1), static_cast<InputDeviceModule::JoyStickButton>(button2));
    }

    bool InputDeviceComponent::areButtonsDown3(int button1, int button2, int button3) const
    {
        if (nullptr == this->inputDeviceModule)
        {
            return false;
        }
        return this->inputDeviceModule->areButtonsDown3(static_cast<InputDeviceModule::JoyStickButton>(button1), static_cast<InputDeviceModule::JoyStickButton>(button2), static_cast<InputDeviceModule::JoyStickButton>(button3));
    }

    bool InputDeviceComponent::areButtonsDown4(int button1, int button2, int button3, int button4) const
    {
        if (nullptr == this->inputDeviceModule)
        {
            return false;
        }
        return this->inputDeviceModule->areButtonsDown4(static_cast<InputDeviceModule::JoyStickButton>(button1), static_cast<InputDeviceModule::JoyStickButton>(button2), static_cast<InputDeviceModule::JoyStickButton>(button3),
            static_cast<InputDeviceModule::JoyStickButton>(button4));
    }

    int InputDeviceComponent::getPressedButton(void) const
    {
        if (nullptr == this->inputDeviceModule)
        {
            return static_cast<int>(InputDeviceModule::BUTTON_NONE);
        }
        return static_cast<int>(this->inputDeviceModule->getPressedButton());
    }

    // Only possible way to do it! Here with instance does not work!, Because we are in a plugin!
    luabind::object InputDeviceComponent::getLuaPressedButtons(void)
    {
        luabind::object obj = luabind::newtable(NOWA::LuaScriptApi::getInstance()->getLua());

        if (nullptr == this->inputDeviceModule)
        {
            return obj;
        }

        const auto pressedButtons = this->inputDeviceModule->getPressedButtons();

        unsigned int i = 0;
        for (auto it = pressedButtons.cbegin(); it != pressedButtons.cend(); ++it)
        {
            obj[i++] = static_cast<int>(*it);
        }

        return obj;
    }

    void InputDeviceComponent::setAnalogActionThreshold(Ogre::Real t)
    {
        if (nullptr != this->inputDeviceModule)
        {
            this->inputDeviceModule->setAnalogActionThreshold(t);
        }
    }

    Ogre::Real InputDeviceComponent::getAnalogActionThreshold(void) const
    {
        if (nullptr == this->inputDeviceModule)
        {
            return 0.0f;
        }
        return this->inputDeviceModule->getAnalogActionThreshold();
    }

    Ogre::Real InputDeviceComponent::getSteerAxis(void)
    {
        if (nullptr == this->inputDeviceModule)
        {
            return 0.0f;
        }
        return this->inputDeviceModule->getSteerAxis();
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

    void InputDeviceComponent::handleInputDeviceOccupied(NOWA::EventDataPtr eventData)
    {
        boost::shared_ptr<EventDataInputDeviceOccupied> castEventData = boost::static_pointer_cast<EventDataInputDeviceOccupied>(eventData);

        if (castEventData->getGameObjectId() == this->gameObjectPtr->getId())
        {
            return;
        }

        if (true == castEventData->getIsActivated() && castEventData->getDeviceName() == this->deviceName->getListSelectedValue())
        {
            if (true == this->isExclusive->getBool())
            {
                InputDeviceCore::getSingletonPtr()->releaseDevice(this->gameObjectPtr->getId());
                this->inputDeviceModule = nullptr;
                this->bValidDevice = false;
                this->deviceName->setListSelectedValue("Choose Device");
                this->activated->setValue(false);
            }
            else
            {
                InputDeviceCore::getSingletonPtr()->releaseDevice(this->gameObjectPtr->getId());
                this->inputDeviceModule = nullptr;
                this->activated->setValue(false);
            }
        }

        this->getActualizedDeviceList();
    }

    bool InputDeviceComponent::hasValidDevice(void) const
    {
        return this->bValidDevice;
    }

    void InputDeviceComponent::lockDevice(bool bLockDevice)
    {
        InputDeviceCore::getSingletonPtr()->lockDevices(bLockDevice);
    }

    bool InputDeviceComponent::isDeviceLocked(void) const
    {
        return InputDeviceCore::getSingletonPtr()->areDevicesLocked() || false == this->activated->getBool();
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

        const auto deviceNames = this->getActualizedDeviceList();

        unsigned int i = 0;
        for (auto it = deviceNames.cbegin(); it != deviceNames.cend(); ++it)
        {
            obj[i++] = *it;
        }

        return obj;
    }

    void InputDeviceComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
    {
        module(lua)[class_<InputDeviceComponent, GameObjectComponent>("InputDeviceComponent")
                .def("setActivated", &InputDeviceComponent::setActivated)
                .def("isActivated", &InputDeviceComponent::isActivated)
                .def("setDeviceName", &InputDeviceComponent::setDeviceName)
                .def("getDeviceName", &InputDeviceComponent::getDeviceName)
                .def("getActualizedDeviceList", &InputDeviceComponent::getLuaActualizedDeviceList)
                .def("getInputDeviceModule", &InputDeviceComponent::getInputDeviceModule)
                .def("checkDevice", &InputDeviceComponent::checkDevice)
                .def("hasValidDevice", &InputDeviceComponent::hasValidDevice)
                .def("lockDevice", &InputDeviceComponent::lockDevice)
                .def("isDeviceLocked", &InputDeviceComponent::isDeviceLocked)
                .def("isKeyboardDevice", &InputDeviceComponent::isKeyboardDevice)
                .def("getMappedKey", &InputDeviceComponent::getMappedKey)
                .def("getStringFromMappedKey", &InputDeviceComponent::getStringFromMappedKey)
                .def("getMappedKeyFromString", &InputDeviceComponent::getMappedKeyFromString)
                .def("getMappedButton", &InputDeviceComponent::getMappedButton)
                .def("getStringFromMappedButton", &InputDeviceComponent::getStringFromMappedButton)
                .def("setJoyStickDeadZone", &InputDeviceComponent::setJoyStickDeadZone)
                .def("hasActiveJoyStick", &InputDeviceComponent::hasActiveJoyStick)
                .def("getLeftStickHorizontalMovingStrength", &InputDeviceComponent::getLeftStickHorizontalMovingStrength)
                .def("getLeftStickVerticalMovingStrength", &InputDeviceComponent::getLeftStickVerticalMovingStrength)
                .def("getRightStickHorizontalMovingStrength", &InputDeviceComponent::getRightStickHorizontalMovingStrength)
                .def("getRightStickVerticalMovingStrength", &InputDeviceComponent::getRightStickVerticalMovingStrength)
                .def("isKeyDown", &InputDeviceComponent::isKeyDown)
                .def("isButtonDown", &InputDeviceComponent::isButtonDown)
                .def("isActionDown", &InputDeviceComponent::isActionDown)
                .def("isActionDownAmount", &InputDeviceComponent::isActionDownAmount)
                .def("isActionDownPressed", &InputDeviceComponent::isActionDownPressed)
                .def("areButtonsDown2", &InputDeviceComponent::areButtonsDown2)
                .def("areButtonsDown3", &InputDeviceComponent::areButtonsDown3)
                .def("areButtonsDown4", &InputDeviceComponent::areButtonsDown4)
                .def("getPressedButton", &InputDeviceComponent::getPressedButton)
                .def("getPressedButtons", &InputDeviceComponent::getLuaPressedButtons)
                .def("setAnalogActionThreshold", &InputDeviceComponent::setAnalogActionThreshold)
                .def("getAnalogActionThreshold", &InputDeviceComponent::getAnalogActionThreshold)
                .def("getSteerAxis", &InputDeviceComponent::getSteerAxis)];

        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "class inherits GameObjectComponent", InputDeviceComponent::getStaticInfoText());
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "void setActivated(bool activated)",
            "Activates the device for this owner game object. Note, if there are other game objects with the same device, they will be deactivated and the chosen device reset to 'Choose Device'. "
            "There can be no two active game objects with the same device at the same time. First one must be deactivated, then the other can use the device. This can also be done for lua if its necessary e.g. to change player control.");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "bool isActivated()", "Gets whether the device is activated for this owner game object.");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "void setDeviceName(string deviceName)", "Sets the given device name. Note: It should only be chosen delivered from the @getActualizedDeviceList().");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "string getDeviceName()", "Gets the selected device name.");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "Table[number][string] getActualizedDeviceList()",
            "Gets all available devices. If none available or all occupied, 'No Device Available' will be delivered. "
            "By default 'Choose Device' is set, which is an invalid item and should be checked if chosen. Use @checkDevice or @hasValidDevice to check if the given selected device is valid, instead of checking those strings.");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "InputDeviceModule getInputDeviceModule()", "Gets the assigned input device module instance.");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "bool checkDevice(string deviceName)", "Gets for the given device name, whether its a valid device or not.");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "bool hasValidDevice()", "Gets after calling @setDeviceName, whether the set device is valid.");

        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "void lockDevice(bool bLockDevice)",
            "Locks the device, so if set to true, no inputs are processed, until the device is again unlocked. Note: Actually all devices are locked/unlocked.");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "bool isDeviceLocked()",
            "Gets wether the device is locked and no inputs are processed. Note: if lock is set, all devices are locked, else if exclusive, then the device is locked, which is not activated.");

        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "bool isKeyboardDevice()", "Gets whether the assigned device is a keyboard device. If false, its a joystick device.");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "KeyCode getMappedKey(Action action)", "Gets the OIS key that is mapped as action.");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "String getStringFromMappedKey(KeyCode keyCode)", "Gets the given OIS key as string.");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "KeyCode getMappedKeyFromString(String key)", "Gets the OIS key from string key.");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "JoyStickButton getMappedButton(Action action)", "Gets the OIS joystick button that is mapped as action.");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "String getStringFromMappedButton(JoyStickButton button)", "Gets the given OIS joystick button as string.");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "void setJoyStickDeadZone(float deadZone)", "Sets the joystick dead zone.");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "bool hasActiveJoyStick()", "Gets whether a joystick is plugged in and active.");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "float getLeftStickHorizontalMovingStrength()",
            "Gets the strength of the left stick horizontal moving."
            " If 0 horizontal stick is not moved. When moved right values are in range (0, 1]. When moved left values are in range (0, -1].");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "float getLeftStickVerticalMovingStrength()",
            "Gets the strength of the left stick vertical moving."
            " If 0 vertical stick is not moved. When moved up values are in range (0, 1]. When moved down values are in range (0, -1].");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "float getRightStickHorizontalMovingStrength()",
            "Gets the strength of the right stick horizontal moving."
            " If 0 horizontal stick is not moved. When moved right values are in range (0, 1]. When moved left values are in range (0, -1].");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "float getRightStickVerticalMovingStrength()",
            "Gets the strength of the right stick vertical moving."
            " If 0 vertical stick is not moved. When moved up values are in range (0, 1]. When moved down values are in range (0, -1].");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "bool isKeyDown(KeyCode key)", "Gets whether a specific key is down.");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "bool isButtonDown(JoyStickButton button)", "Gets whether a specific joystick button is down.");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "bool isActionDown(Action action)", "Gets whether a specific mapped action is down.");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "bool isActionDownAmount(Action action, number dt, number actionDuration)",
            "Gets whether a specific mapped action is down, but only max for the specific action duration. Default value is 0.2 seconds.");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "bool isActionDownPressed(Action action, number dt, number durationBetweenTheAction)", "Gets whether a specific mapped action is pressed.");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "bool areButtonsDown2(JoyStickButton button1, JoyStickButton button2)", "Gets whether two specific joystick buttons are down at the same time.");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "bool areButtonsDown3(JoyStickButton button1, JoyStickButton button2, JoyStickButton button3)",
            "Gets whether three specific joystick buttons are down at the same time.");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "bool areButtonsDown4(JoyStickButton button1, JoyStickButton button2, JoyStickButton button3, JoyStickButton button4)",
            "Gets whether four specific joystick buttons are down at the same time.");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "JoyStickButton getPressedButton()", "Gets the currently pressed joystick button.");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "Table[number][JoyStickButton] getPressedButtons()", "Gets the currently simultaneously pressed joystick buttons.");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "void setAnalogActionThreshold(float t)", "Sets the threshold for treating stick as digital actions (LEFT/RIGHT/UP/DOWN).");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "float getAnalogActionThreshold()", "Gets the threshold for treating stick as digital actions (LEFT/RIGHT/UP/DOWN).");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "float getSteerAxis()", "Returns steering axis in [-1..1] from keyboard or left stick.");

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

}; // namespace end
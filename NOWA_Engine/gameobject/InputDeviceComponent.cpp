#include "NOWAPrecompiled.h"
#include "InputDeviceComponent.h"
#include "main/AppStateManager.h"
#include "main/EventManager.h"
#include "main/InputDeviceCore.h"
#include "modules/InputDeviceModule.h"
#include "modules/LuaScriptApi.h"
#include "utilities/XMLConverter.h"

namespace
{
    // Must match the logical names of the InputDeviceCore
    const Ogre::String DeviceChoose = "Choose Device";
    const Ogre::String DeviceNoneAvailable = "No Device Available";
    const Ogre::String DeviceAuto = "Auto";
    const Ogre::String DeviceJoin = "Join";

    bool isLogicalDeviceName(const Ogre::String& deviceName)
    {
        const std::vector<Ogre::String> logicalDeviceNames = NOWA::InputDeviceCore::getSingletonPtr()->getLogicalDeviceNames();
        for (const auto& logicalDeviceName : logicalDeviceNames)
        {
            if (logicalDeviceName == deviceName)
            {
                return true;
            }
        }
        return false;
    }

    /**
     * Converts physical OIS vendor names, which older scenes stored (e.g. "Win32InputManager" for the keyboard on Windows,
     * "X11InputManager" on Linux, or "Controller (XBOX 360 For Windows)_0"), into the logical device names.
     * A keyboard becomes "Auto", so that the same scene also works with a gamepad (Steam Deck).
     */
    Ogre::String convertLegacyDeviceName(const Ogre::String& deviceName)
    {
        if (true == deviceName.empty() || DeviceChoose == deviceName || DeviceNoneAvailable == deviceName)
        {
            return DeviceChoose;
        }

        if (true == isLogicalDeviceName(deviceName))
        {
            return deviceName;
        }

        NOWA::InputDeviceCore* inputDeviceCore = NOWA::InputDeviceCore::getSingletonPtr();
        NOWA::InputDeviceModule* module = inputDeviceCore->findModuleByName(deviceName);

        Ogre::String convertedName;
        if (nullptr != module)
        {
            if (true == module->isKeyboardDevice())
            {
                convertedName = DeviceAuto;
            }
            else
            {
                convertedName = inputDeviceCore->getLogicalDeviceName(module);
            }
        }
        else if (Ogre::String::npos != deviceName.find("InputManager"))
        {
            // Keyboard vendor name of another platform
            convertedName = DeviceAuto;
        }
        else
        {
            // Unknown gamepad from another machine: let the player join with any free device
            convertedName = DeviceJoin;
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[InputDeviceComponent] Converted legacy device name '" + deviceName + "' to logical device name '" + convertedName + "'. Save the scene to store the new name.");
        return convertedName;
    }
}

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

        this->deviceName->setDescription("Sets the logical input device. 'Auto': Keyboard and the first free gamepad at the same time (single player, Steam Deck, no keyboard -> gamepad only). "
                                         "'Join': No device until a free device presses Jump or Start (splitscreen, 'press a button to join'). 'Keyboard': Keyboard only. 'Gamepad N': The N-th connected gamepad only. "
                                         "The key and button mapping is taken from the configuration (Controls menu).");

        this->isExclusive->setDescription(
            "Sets whether the chosen device name for this this input device is exclusive, which means no other active input device for a game object may have the same device. This is useful e.g. for splitscreen or multiplayer scenarios. "
            "If set to false, still no two active input devices can have the same device, but if an input device will be activated, then all other game objects with their input devices will just be deactivated, but keep the same input device name. "
            " So its possible to switch between game objects and each time the input device name like keyboard will be used for the yet active input device of the given game object.");

        this->deviceName->addUserData(GameObject::AttrActionNeedRefresh());

        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &InputDeviceComponent::handleInputDeviceOccupied), EventDataInputDeviceOccupied::getStaticEventType());
    }

    InputDeviceComponent::~InputDeviceComponent(void)
    {
        // onRemoveComponent() is not guaranteed to run for every component instance - a full level/scene teardown destroys GameObjects and their components
        // directly. Since the constructor registers handleInputDeviceOccupied() with the global EventManager, it must be unregistered here,
        // else the next EventDataInputDeviceOccupied would call into destroyed memory.
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &InputDeviceComponent::handleInputDeviceOccupied), EventDataInputDeviceOccupied::getStaticEventType());
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
            // Legacy physical names are converted in postInit, when the device list is known
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
        boost::shared_ptr<InputDeviceComponent> clonedCompPtr(boost::make_shared<InputDeviceComponent>());

        clonedGameObjectPtr->addComponent(clonedCompPtr);
        clonedCompPtr->setOwner(clonedGameObjectPtr);

        clonedCompPtr->setIsExcluse(this->isExclusive->getBool());

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
        return clonedCompPtr;
    }

    bool InputDeviceComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[InputDeviceComponent] Init component for game object: " + this->gameObjectPtr->getName());

        const Ogre::String loadedDeviceName = convertLegacyDeviceName(this->deviceName->getListSelectedValue());

        this->getActualizedDeviceList();
        this->deviceName->setListSelectedValue(loadedDeviceName);

        this->setIsExcluse(this->isExclusive->getBool());

        this->setActivated(this->activated->getBool());

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

        // Note: In "Join" mode the device must be joined again after the next start
        this->inputDeviceModule = nullptr;
        this->bValidDevice = false;

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
        if (true == notSimulating || false == this->activated->getBool())
        {
            return;
        }

        // Device was taken over by another game object (e.g. "Gamepad 1" explicitly requested elsewhere)
        if (nullptr != this->inputDeviceModule && this->inputDeviceModule->getOccupiedId() != this->gameObjectPtr->getId())
        {
            this->handleDeviceLost();
            return;
        }

        // Splitscreen lobby: Wait until a free device presses Jump or Start
        if (nullptr == this->inputDeviceModule && DeviceJoin == this->deviceName->getListSelectedValue())
        {
            InputDeviceModule* joinModule = InputDeviceCore::getSingletonPtr()->findJoinRequestModule();
            if (nullptr != joinModule)
            {
                this->inputDeviceModule = InputDeviceCore::getSingletonPtr()->assignDevice(joinModule->getDeviceName(), this->gameObjectPtr->getId());
                this->bValidDevice = nullptr != this->inputDeviceModule;

                if (true == this->bValidDevice)
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[InputDeviceComponent] Game object '" + this->gameObjectPtr->getName() + "' joined with device: '" + this->getAssignedDeviceName() + "'");

                    boost::shared_ptr<EventDataInputDeviceOccupied> eventDataDeviceOccupied(new EventDataInputDeviceOccupied(this->gameObjectPtr->getId(), true, this->getAssignedDeviceName()));
                    AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataDeviceOccupied);
                }
            }
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

    Ogre::String InputDeviceComponent::getAssignedDeviceName(void) const
    {
        if (nullptr == this->inputDeviceModule)
        {
            return Ogre::String();
        }
        return InputDeviceCore::getSingletonPtr()->getLogicalDeviceName(this->inputDeviceModule);
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

        const Ogre::String logicalDeviceName = convertLegacyDeviceName(deviceName);

        this->getActualizedDeviceList();
        this->deviceName->setListSelectedValue(logicalDeviceName);

        // An inactive game object must not grab a device: Since devices are taken over on assignment now, it would kick out the active player.
        // The device is assigned in setActivated(true).
        if (false == this->activated->getBool() || false == this->checkDevice(logicalDeviceName))
        {
            InputDeviceCore::getSingletonPtr()->releaseDevice(this->gameObjectPtr->getId());
            this->inputDeviceModule = nullptr;
            return;
        }

        if (DeviceJoin == logicalDeviceName)
        {
            // Keep an already joined device, else wait in update() for a join request
            if (nullptr != this->inputDeviceModule && this->inputDeviceModule->getOccupiedId() == this->gameObjectPtr->getId())
            {
                this->bValidDevice = true;
            }
            else
            {
                InputDeviceCore::getSingletonPtr()->releaseDevice(this->gameObjectPtr->getId());
                this->inputDeviceModule = nullptr;
            }
            return;
        }

        this->inputDeviceModule = InputDeviceCore::getSingletonPtr()->assignDevice(logicalDeviceName, this->gameObjectPtr->getId());
        this->bValidDevice = nullptr != this->inputDeviceModule;

        if (false == this->bValidDevice)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[InputDeviceComponent] Device '" + logicalDeviceName + "' is not available for game object: '" + this->gameObjectPtr->getName() + "' (e.g. gamepad not connected).");
            return;
        }

        // Announce the assigned device, so that other InputDeviceComponents, which lost their device by this assignment, deactivate themselves
        boost::shared_ptr<EventDataInputDeviceOccupied> eventDataDeviceOccupied(new EventDataInputDeviceOccupied(this->gameObjectPtr->getId(), true, this->getAssignedDeviceName()));
        AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataDeviceOccupied);
    }

    std::vector<Ogre::String> InputDeviceComponent::getActualizedDeviceList(void)
    {
        // The list is static (logical names), so that the editor combo box does not change while devices are occupied
        std::vector<Ogre::String> availableDeviceNames = InputDeviceCore::getSingletonPtr()->getLogicalDeviceNames();
        availableDeviceNames.insert(availableDeviceNames.begin(), DeviceChoose);

        const Ogre::String selectedDeviceName = this->deviceName->getListSelectedValue();

        this->deviceName->setValue(availableDeviceNames);
        this->deviceName->setUserData(GameObject::AttrActionNeedRefresh());

        // Setting a new list may reset the selection, so restore it
        if (false == selectedDeviceName.empty())
        {
            this->deviceName->setListSelectedValue(selectedDeviceName);
        }
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
        // Guard against firing before this component is fully attached to its owning GameObject.
        // The constructor registers this handler with the EventManager BEFORE setOwner()/postInit() run.
        if (nullptr == this->gameObjectPtr)
        {
            return;
        }

        boost::shared_ptr<EventDataInputDeviceOccupied> castEventData = boost::static_pointer_cast<EventDataInputDeviceOccupied>(eventData);

        if (castEventData->getGameObjectId() == this->gameObjectPtr->getId())
        {
            return;
        }

        // Another game object took over the device of this one (the InputDeviceCore already released it for this game object).
        // Note: Comparing the device names is not enough anymore, since logical names like "Join" or "Auto" resolve to different physical devices.
        if (true == castEventData->getIsActivated() && nullptr != this->inputDeviceModule && this->inputDeviceModule->getOccupiedId() != this->gameObjectPtr->getId())
        {
            this->handleDeviceLost();
        }
    }

    void InputDeviceComponent::handleDeviceLost(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[InputDeviceComponent] Game object '" + this->gameObjectPtr->getName() + "' lost its device to another game object, deactivating.");

        InputDeviceCore::getSingletonPtr()->releaseDevice(this->gameObjectPtr->getId());
        this->inputDeviceModule = nullptr;
        this->bValidDevice = false;
        this->activated->setValue(false);

        if (true == this->isExclusive->getBool())
        {
            this->deviceName->setListSelectedValue(DeviceChoose);
        }
    }

    bool InputDeviceComponent::isLastInputFromJoyStick(void) const
    {
        if (nullptr == this->inputDeviceModule)
        {
            return false;
        }
        return this->inputDeviceModule->isLastInputFromJoyStick();
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
                .def("getAssignedDeviceName", &InputDeviceComponent::getAssignedDeviceName)
                .def("isLastInputFromJoyStick", &InputDeviceComponent::isLastInputFromJoyStick)
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
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "void setDeviceName(string deviceName)",
            "Sets the given logical device name ('Auto', 'Join', 'Keyboard', 'Gamepad 1' ...). Note: It should only be chosen delivered from the @getActualizedDeviceList(). "
            "Note: A device held by another game object is taken over, the other game object gets deactivated.");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "string getDeviceName()", "Gets the selected logical device name, e.g. 'Auto', 'Join', 'Keyboard' or 'Gamepad 1'.");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "string getAssignedDeviceName()",
            "Gets the device which is really assigned, e.g. 'Keyboard' or 'Gamepad 2' (useful after 'Join' or 'Auto'). Empty if no device is assigned yet.");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "bool isLastInputFromJoyStick()", "Gets whether the last input of the assigned device came from a gamepad. Use it to show gamepad or keyboard button prompts.");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "Table[number][string] getActualizedDeviceList()",
            "Gets all logical devices: 'Choose Device', 'Auto', 'Join', 'Keyboard', 'Gamepad 1' ... 'Gamepad N'. "
            "'Choose Device' is an invalid item. Use @checkDevice or @hasValidDevice to check if the given selected device is valid, instead of checking those strings.");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "InputDeviceModule getInputDeviceModule()", "Gets the assigned input device module instance.");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "bool checkDevice(string deviceName)", "Gets for the given device name, whether its a valid device or not.");
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "bool hasValidDevice()",
            "Gets after calling @setDeviceName, whether a device is really assigned. In 'Join' mode this becomes true, after a player pressed Jump or Start on a free device.");

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
        LuaScriptApi::getInstance()->addClassToCollection("InputDeviceComponent", "bool hasActiveJoyStick()", "Gets whether the assigned device can deliver gamepad input (a gamepad, or 'Auto' with a companion gamepad).");
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
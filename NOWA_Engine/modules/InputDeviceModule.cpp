#include "NOWAPrecompiled.h"
#include "InputDeviceModule.h"

namespace
{
    Ogre::Real Clamp1(Ogre::Real v)
    {
        return Ogre::Math::Clamp(v, -1.0f, 1.0f);
    }

    // Maps raw stick [-1..1] to a MUCH less sensitive steering [-1..1].
    // - deadzone: exact 0 around center
    // - precisionZone: first part outside deadzone where output stays very small
    // - precisionMaxOut: maximum output magnitude at the end of precisionZone
    //
    // Example good defaults:
    // deadzone = 0.10
    // precisionZone = 0.25
    // precisionMaxOut = 0.10  (=> first 25% only reaches 10% output)
    Ogre::Real MapSteeringPrecision(Ogre::Real in, Ogre::Real deadzone, Ogre::Real precisionZone, Ogre::Real precisionMaxOut)
    {
        in = Clamp1(in);

        const Ogre::Real a = Ogre::Math::Abs(in);
        if (a <= deadzone)
        {
            return 0.0f;
        }

        Ogre::Real sign = 1.0f;
        if (in < 0.0f)
        {
            sign = -1.0f;
        }

        // Normalize to t in [0..1] after deadzone
        const Ogre::Real t = (a - deadzone) / (1.0f - deadzone);

        // Precision zone in t-space
        const Ogre::Real p = Ogre::Math::Clamp(precisionZone, 0.0f, 0.999f);

        Ogre::Real out = 0.0f;

        if (t <= p)
        {
            // Very gentle near center: linear to precisionMaxOut
            const Ogre::Real u = t / p; // 0..1
            out = u * precisionMaxOut;
        }
        else
        {
            // After precision zone: ramp from precisionMaxOut to 1.0 with smoothstep
            const Ogre::Real u = (t - p) / (1.0f - p);           // 0..1
            const Ogre::Real smooth = u * u * (3.0f - 2.0f * u); // smoothstep
            out = precisionMaxOut + (1.0f - precisionMaxOut) * smooth;
        }

        return sign * Clamp1(out);
    }

    // Raw axis indices and trigger behavior of a joystick layout. -1 means: axis does not exist in this layout.
    struct LayoutAxes
    {
        int leftX;
        int leftY;
        int rightX;
        int rightY;
        int leftTrigger;
        int rightTrigger;
        // Linux evdev rescales triggers to [-32768..32767], so a released trigger sits at the minimum. XInput delivers [0..32767].
        bool triggerRestsAtMin;
    };

    LayoutAxes getLayoutAxes(NOWA::InputDeviceModule::JoyStickLayout joyStickLayout)
    {
        LayoutAxes axes;
        if (NOWA::InputDeviceModule::LAYOUT_XINPUT == joyStickLayout)
        {
            axes.leftY = 0;
            axes.leftX = 1;
            axes.rightY = 2;
            axes.rightX = 3;
            axes.leftTrigger = 4;
            axes.rightTrigger = 5;
            axes.triggerRestsAtMin = false;
        }
        else if (NOWA::InputDeviceModule::LAYOUT_LINUX_EVDEV == joyStickLayout)
        {
            axes.leftX = 0;
            axes.leftY = 1;
            axes.leftTrigger = 2;
            axes.rightX = 3;
            axes.rightY = 4;
            axes.rightTrigger = 5;
            axes.triggerRestsAtMin = true;
        }
        else
        {
            // Legacy NOWA layout, triggers are physical buttons
            axes.leftY = 0;
            axes.leftX = 1;
            axes.rightY = 2;
            axes.rightX = 3;
            axes.leftTrigger = -1;
            axes.rightTrigger = -1;
            axes.triggerRestsAtMin = false;
        }
        return axes;
    }

    Ogre::Real normalizeTrigger(int absValue, bool triggerRestsAtMin)
    {
        Ogre::Real value = 0.0f;
        if (true == triggerRestsAtMin)
        {
            value = (static_cast<Ogre::Real>(absValue) + 32768.0f) / 65535.0f;
        }
        else
        {
            value = static_cast<Ogre::Real>(absValue) / 32767.0f;
        }
        return Ogre::Math::Clamp(value, 0.0f, 1.0f);
    }

    const Ogre::Real TRIGGER_DIGITAL_THRESHOLD = 0.5f;
    const Ogre::Real RAW_AXIS_DIGITAL_THRESHOLD = 0.6f;

    struct KeyName
    {
        OIS::KeyCode keyCode;
        const char* name;
    };

    // One table for both directions (key -> string, string -> key) and for the list of all bindable keys.
    // Previously there were three separate lists which had drifted apart (e.g. "Y" could not be parsed back, "R-Control" parsed as L-Control, "Pause" was unreachable).
    const KeyName keyNames[] = {{OIS::KC_ESCAPE, "Esc"}, {OIS::KC_1, "1"}, {OIS::KC_2, "2"}, {OIS::KC_3, "3"}, {OIS::KC_4, "4"}, {OIS::KC_5, "5"}, {OIS::KC_6, "6"}, {OIS::KC_7, "7"}, {OIS::KC_8, "8"}, {OIS::KC_9, "9"}, {OIS::KC_0, "0"},
        {OIS::KC_MINUS, "-"}, {OIS::KC_EQUALS, "="}, {OIS::KC_BACK, "Backspace"}, {OIS::KC_TAB, "Tab"}, {OIS::KC_Q, "Q"}, {OIS::KC_W, "W"}, {OIS::KC_E, "E"}, {OIS::KC_R, "R"}, {OIS::KC_T, "T"}, {OIS::KC_Y, "Y"}, {OIS::KC_U, "U"}, {OIS::KC_I, "I"},
        {OIS::KC_O, "O"}, {OIS::KC_P, "P"}, {OIS::KC_LBRACKET, "("}, {OIS::KC_RBRACKET, ")"}, {OIS::KC_RETURN, "Return"}, {OIS::KC_LCONTROL, "L-Control"}, {OIS::KC_A, "A"}, {OIS::KC_S, "S"}, {OIS::KC_D, "D"}, {OIS::KC_F, "F"}, {OIS::KC_G, "G"},
        {OIS::KC_H, "H"}, {OIS::KC_J, "J"}, {OIS::KC_K, "K"}, {OIS::KC_L, "L"}, {OIS::KC_SEMICOLON, ";"}, {OIS::KC_APOSTROPHE, "´"}, {OIS::KC_GRAVE, "^"}, {OIS::KC_LSHIFT, "L-Shift"}, {OIS::KC_BACKSLASH, "\""}, {OIS::KC_Z, "Z"}, {OIS::KC_X, "X"},
        {OIS::KC_C, "C"}, {OIS::KC_V, "V"}, {OIS::KC_B, "B"}, {OIS::KC_N, "N"}, {OIS::KC_M, "M"}, {OIS::KC_COMMA, ","}, {OIS::KC_PERIOD, "."}, {OIS::KC_SLASH, "Slash"}, {OIS::KC_RSHIFT, "R-Shift"}, {OIS::KC_MULTIPLY, "*"}, {OIS::KC_LMENU, "L-Alt"},
        {OIS::KC_SPACE, "Space"}, {OIS::KC_CAPITAL, "Capital"}, {OIS::KC_F1, "F1"}, {OIS::KC_F2, "F2"}, {OIS::KC_F3, "F3"}, {OIS::KC_F4, "F4"}, {OIS::KC_F5, "F5"}, {OIS::KC_F6, "F6"}, {OIS::KC_F7, "F7"}, {OIS::KC_F8, "F8"}, {OIS::KC_F9, "F9"},
        {OIS::KC_F10, "F10"}, {OIS::KC_NUMLOCK, "Numlock"}, {OIS::KC_SCROLL, "Scroll"}, {OIS::KC_NUMPAD7, "Num 7"}, {OIS::KC_NUMPAD8, "Num 8"}, {OIS::KC_NUMPAD9, "Num 9"}, {OIS::KC_SUBTRACT, "Sub"}, {OIS::KC_NUMPAD4, "Num 4"},
        {OIS::KC_NUMPAD5, "Num 5"}, {OIS::KC_NUMPAD6, "Num 6"}, {OIS::KC_ADD, "+"}, {OIS::KC_NUMPAD1, "Num 1"}, {OIS::KC_NUMPAD2, "Num 2"}, {OIS::KC_NUMPAD3, "Num 3"}, {OIS::KC_NUMPAD0, "Num 0"}, {OIS::KC_DECIMAL, "Num ."}, {OIS::KC_OEM_102, "<"},
        {OIS::KC_F11, "F11"}, {OIS::KC_F12, "F12"}, {OIS::KC_RCONTROL, "R-Control"}, {OIS::KC_NUMPADCOMMA, "Num ,"}, {OIS::KC_DIVIDE, "Num /"}, {OIS::KC_SYSRQ, "SysRQ"}, {OIS::KC_RMENU, "R-Alt"}, {OIS::KC_PAUSE, "Pause"}, {OIS::KC_HOME, "Home"},
        {OIS::KC_UP, "Up"}, {OIS::KC_PGUP, "Page-Up"}, {OIS::KC_LEFT, "Left"}, {OIS::KC_RIGHT, "Right"}, {OIS::KC_END, "End"}, {OIS::KC_DOWN, "Down"}, {OIS::KC_PGDOWN, "Page-Down"}, {OIS::KC_INSERT, "Insert"}, {OIS::KC_DELETE, "Delete"}};

    struct ButtonName
    {
        NOWA::InputDeviceModule::JoyStickButton button;
        const char* name;
    };

    const ButtonName buttonNames[] = {{NOWA::InputDeviceModule::BUTTON_A, "A"}, {NOWA::InputDeviceModule::BUTTON_B, "B"}, {NOWA::InputDeviceModule::BUTTON_X, "X"}, {NOWA::InputDeviceModule::BUTTON_Y, "Y"}, {NOWA::InputDeviceModule::BUTTON_LB, "LB"},
        {NOWA::InputDeviceModule::BUTTON_RB, "RB"}, {NOWA::InputDeviceModule::BUTTON_LT, "LT"}, {NOWA::InputDeviceModule::BUTTON_RT, "RT"}, {NOWA::InputDeviceModule::BUTTON_SELECT, "Select"}, {NOWA::InputDeviceModule::BUTTON_START, "Start"},
        {NOWA::InputDeviceModule::BUTTON_LEFT_STICK, "Left Stick"}, {NOWA::InputDeviceModule::BUTTON_RIGHT_STICK, "Right Stick"}, {NOWA::InputDeviceModule::BUTTON_LEFT_STICK_UP, "Left Stick Up"},
        {NOWA::InputDeviceModule::BUTTON_LEFT_STICK_DOWN, "Left Stick Down"}, {NOWA::InputDeviceModule::BUTTON_LEFT_STICK_LEFT, "Left Stick Left"}, {NOWA::InputDeviceModule::BUTTON_LEFT_STICK_RIGHT, "Left Stick Right"},
        {NOWA::InputDeviceModule::BUTTON_RIGHT_STICK_UP, "Right Stick Up"}, {NOWA::InputDeviceModule::BUTTON_RIGHT_STICK_DOWN, "Right Stick Down"}, {NOWA::InputDeviceModule::BUTTON_RIGHT_STICK_LEFT, "Right Stick Left"},
        {NOWA::InputDeviceModule::BUTTON_RIGHT_STICK_RIGHT, "Right Stick Right"}, {NOWA::InputDeviceModule::BUTTON_DPAD_UP, "D-Pad Up"}, {NOWA::InputDeviceModule::BUTTON_DPAD_DOWN, "D-Pad Down"},
        {NOWA::InputDeviceModule::BUTTON_DPAD_LEFT, "D-Pad Left"}, {NOWA::InputDeviceModule::BUTTON_DPAD_RIGHT, "D-Pad Right"}};

    // Older captions, still accepted when parsing
    const ButtonName legacyButtonNames[] = {{NOWA::InputDeviceModule::BUTTON_LEFT_STICK_UP, "Up"}, {NOWA::InputDeviceModule::BUTTON_LEFT_STICK_DOWN, "Down"}, {NOWA::InputDeviceModule::BUTTON_LEFT_STICK_LEFT, "Left"},
        {NOWA::InputDeviceModule::BUTTON_LEFT_STICK_RIGHT, "Right"}};

    struct ActionName
    {
        NOWA::InputDeviceModule::Action action;
        const char* name;
    };

    const ActionName actionNames[] = {{NOWA::InputDeviceModule::UP, "UP"}, {NOWA::InputDeviceModule::DOWN, "DOWN"}, {NOWA::InputDeviceModule::LEFT, "LEFT"}, {NOWA::InputDeviceModule::RIGHT, "RIGHT"}, {NOWA::InputDeviceModule::JUMP, "JUMP"},
        {NOWA::InputDeviceModule::RUN, "RUN"}, {NOWA::InputDeviceModule::COWER, "COWER"}, {NOWA::InputDeviceModule::ATTACK_1, "ATTACK_1"}, {NOWA::InputDeviceModule::ATTACK_2, "ATTACK_2"}, {NOWA::InputDeviceModule::DUCK, "DUCK"},
        {NOWA::InputDeviceModule::SNEAK, "SNEAK"}, {NOWA::InputDeviceModule::ACTION, "ACTION"}, {NOWA::InputDeviceModule::RELOAD, "RELOAD"}, {NOWA::InputDeviceModule::INVENTORY, "INVENTORY"}, {NOWA::InputDeviceModule::MAP, "MAP"},
        {NOWA::InputDeviceModule::SELECT, "SELECT"}, {NOWA::InputDeviceModule::START, "START"}, {NOWA::InputDeviceModule::SAVE, "SAVE"}, {NOWA::InputDeviceModule::LOAD, "LOAD"}, {NOWA::InputDeviceModule::CAMERA_FORWARD, "CAMERA_FORWARD"},
        {NOWA::InputDeviceModule::CAMERA_BACKWARD, "CAMERA_BACKWARD"}, {NOWA::InputDeviceModule::CAMERA_LEFT, "CAMERA_LEFT"}, {NOWA::InputDeviceModule::CAMERA_RIGHT, "CAMERA_RIGHT"}, {NOWA::InputDeviceModule::CAMERA_UP, "CAMERA_UP"},
        {NOWA::InputDeviceModule::CAMERA_DOWN, "CAMERA_DOWN"}, {NOWA::InputDeviceModule::CONSOLE, "CONSOLE"}, {NOWA::InputDeviceModule::WEAPON_CHANGE_FORWARD, "WEAPON_CHANGE_FORWARD"},
        {NOWA::InputDeviceModule::WEAPON_CHANGE_BACKWARD, "WEAPON_CHANGE_BACKWARD"}, {NOWA::InputDeviceModule::FLASH_LIGHT, "FLASH_LIGHT"}, {NOWA::InputDeviceModule::PAUSE, "PAUSE"}, {NOWA::InputDeviceModule::GRID, "GRID"}};
}

namespace NOWA
{
    InputDeviceModule::InputDeviceModule(const Ogre::String& deviceName, bool isKeyboard, OIS::Object* deviceObject) :
        deviceName(deviceName),
        isKeyboard(isKeyboard),
        occuppiedId(0),
        deviceObject(deviceObject),
        joyStickDeadZone(0.08f), // 0.05 was not enough for stick, so when user left the stick alone, it had a small move strengh remaining
        rightStickMovement(Ogre::Vector2::ZERO),
        leftStickMovement(Ogre::Vector2::ZERO),
        povMovement(Ogre::Vector2::ZERO),
        pressedButton(JoyStickButton::BUTTON_NONE),
        pressedButtonMask(0u),
        capturedButtonMask(0u),
        lastCapturedButton(JoyStickButton::BUTTON_NONE),
        analogActionThreshold(0.3f),
        bLock(false),
        joyStickLayout(LAYOUT_GENERIC),
        companionModule(nullptr),
        companionSoft(false),
        companionOwner(nullptr),
        lastInputFromJoyStick(false == isKeyboard)
    {
        this->pressedPov[0] = Action::NONE; // pov = point of view = D-pad
        this->pressedPov[1] = Action::NONE;
        this->pressedPov[2] = Action::NONE;
        this->pressedPov[3] = Action::NONE;

        // Each action has its own timers. Previously one timer and one canPress flag were shared by ALL actions,
        // so querying e.g. isActionPressed(JUMP) and isActionPressed(ATTACK_1) in the same frame disturbed each other.
        for (unsigned short i = 0; i < ACTION_SLOT_COUNT; i++)
        {
            this->timeSinceLastActionDown[i] = 0.2f;
            this->timeSinceLastActionPressed[i] = 0.0f;
            this->canPress[i] = false;
        }

        this->buildRawButtonTable();

        this->setDefaultKeyMapping();

        this->setDefaultButtonMapping();

        for (const KeyName& keyName : keyNames)
        {
            this->allOISKeys.emplace(keyName.keyCode);
        }

        for (const ButtonName& buttonName : buttonNames)
        {
            this->allButtons.emplace(buttonName.button);
        }
    }

    OIS::KeyCode InputDeviceModule::getMappedKeyFromString(const Ogre::String& key)
    {
        for (const KeyName& keyName : keyNames)
        {
            if (key == keyName.name)
            {
                return keyName.keyCode;
            }
        }

        // Legacy caption of the numpad divide key
        if ("/" == key)
        {
            return OIS::KC_DIVIDE;
        }

        return OIS::KC_UNASSIGNED;
    }

    InputDeviceModule::JoyStickButton InputDeviceModule::getMappedButtonFromString(const Ogre::String& button)
    {
        for (const ButtonName& buttonName : buttonNames)
        {
            if (button == buttonName.name)
            {
                return buttonName.button;
            }
        }

        for (const ButtonName& buttonName : legacyButtonNames)
        {
            if (button == buttonName.name)
            {
                return buttonName.button;
            }
        }

        return JoyStickButton::BUTTON_NONE;
    }

    const Ogre::String& InputDeviceModule::getDeviceName(void) const
    {
        return this->deviceName;
    }

    bool InputDeviceModule::isKeyboardDevice(void) const
    {
        return this->isKeyboard;
    }

    bool InputDeviceModule::isOccupied(void) const
    {
        return this->occuppiedId != 0;
    }

    unsigned long InputDeviceModule::getOccupiedId(void) const
    {
        return this->occuppiedId;
    }

    void InputDeviceModule::setOccupiedId(unsigned long id)
    {
        this->occuppiedId = id;
    }

    void InputDeviceModule::releaseOccupation(void)
    {
        this->occuppiedId = 0;
    }

    unsigned short InputDeviceModule::getKeyMappingCount(void) const
    {
        return static_cast<unsigned short>(this->keyboardMapping.size());
    }

    unsigned short InputDeviceModule::getButtonMappingCount(void) const
    {
        return static_cast<unsigned short>(this->buttonMapping.size());
    }

    OIS::KeyCode InputDeviceModule::getMappedKey(InputDeviceModule::Action keyboardAction)
    {
        // Note: Do not use operator[] here, it would insert new entries for unmapped actions and change the mapping count
        auto foundMapping = this->keyboardMapping.find(keyboardAction);
        if (foundMapping != this->keyboardMapping.cend())
        {
            return foundMapping->second;
        }
        return OIS::KC_UNASSIGNED;
    }

    Ogre::String InputDeviceModule::getStringFromMappedKeyAction(InputDeviceModule::Action action)
    {
        auto mappedKey = this->getMappedKey(action);
        return this->getStringFromMappedKey(mappedKey);
    }

    InputDeviceModule::JoyStickButton InputDeviceModule::getMappedButton(InputDeviceModule::Action action)
    {
        auto foundMapping = this->buttonMapping.find(action);
        if (foundMapping != this->buttonMapping.cend())
        {
            return foundMapping->second;
        }
        return InputDeviceModule::BUTTON_NONE;
    }

    Ogre::String InputDeviceModule::getStringFromMappedButtonAction(InputDeviceModule::Action action)
    {
        auto mappedButton = this->getMappedButton(action);
        return this->getStringFromMappedButton(mappedButton);
    }

    Ogre::String InputDeviceModule::getStringFromMappedButton(JoyStickButton joyStickButton)
    {
        for (const ButtonName& buttonName : buttonNames)
        {
            if (joyStickButton == buttonName.button)
            {
                return buttonName.name;
            }
        }
        return "None";
    }

    Ogre::String InputDeviceModule::getStringFromMappedKey(OIS::KeyCode keyCode)
    {
        for (const KeyName& keyName : keyNames)
        {
            if (keyCode == keyName.keyCode)
            {
                return keyName.name;
            }
        }
        return "";
    }

    std::vector<Ogre::String> InputDeviceModule::getAllKeyStrings(void)
    {
        std::vector<Ogre::String> keys;
        keys.reserve(this->allOISKeys.size());
        for (OIS::KeyCode key : this->allOISKeys)
        {
            keys.emplace_back(this->getStringFromMappedKey(key));
        }
        return keys;
    }

    std::vector<Ogre::String> InputDeviceModule::getAllButtonStrings(void)
    {
        std::vector<Ogre::String> buttons;
        buttons.reserve(this->allButtons.size());
        for (JoyStickButton button : this->allButtons)
        {
            buttons.emplace_back(this->getStringFromMappedButton(button));
        }
        return buttons;
    }

    void InputDeviceModule::remapKey(InputDeviceModule::Action keyboardAction, OIS::KeyCode keyCode)
    {
        this->keyboardMapping[keyboardAction] = keyCode;
    }

    void InputDeviceModule::remapButton(InputDeviceModule::Action action, JoyStickButton joyStickButton)
    {
        this->buttonMapping[action] = joyStickButton;
    }

    void InputDeviceModule::setJoyStickDeadZone(Ogre::Real deadZone)
    {
        this->joyStickDeadZone = deadZone;
        if (nullptr != this->companionModule)
        {
            this->companionModule->setJoyStickDeadZone(deadZone);
        }
    }

    bool InputDeviceModule::hasActiveJoyStick(void) const
    {
        // Previously this returned whether ANY joystick exists globally, which is wrong for splitscreen.
        if (false == this->isKeyboard)
        {
            return nullptr != this->deviceObject;
        }
        return nullptr != this->companionModule;
    }

    void InputDeviceModule::setAnalogActionThreshold(Ogre::Real t)
    {
        this->analogActionThreshold = Ogre::Math::Clamp(t, 0.0f, 1.0f);
        if (nullptr != this->companionModule)
        {
            this->companionModule->setAnalogActionThreshold(t);
        }
    }

    Ogre::Real InputDeviceModule::getAnalogActionThreshold(void) const
    {
        return this->analogActionThreshold;
    }

    Ogre::Real InputDeviceModule::getSteerAxis(void)
    {
        if (true == this->bLock)
        {
            return 0.0f;
        }

        // Keyboard device: compose LEFT/RIGHT keys into an axis
        if (true == this->isKeyboard)
        {
            const bool left = this->isActionDownOwnDevice(InputDeviceModule::LEFT);
            const bool right = this->isActionDownOwnDevice(InputDeviceModule::RIGHT);

            if (true == left && false == right)
            {
                return -1.0f;
            }
            if (true == right && false == left)
            {
                return 1.0f;
            }

            // No key pressed: use the analog stick of the companion gamepad (if any)
            if (nullptr != this->companionModule)
            {
                return this->companionModule->getSteerAxis();
            }
            return 0.0f;
        }

        // Joystick: return analog stick directly
        return Ogre::Math::Clamp(this->leftStickMovement.x, -1.0f, 1.0f);
    }

    void InputDeviceModule::clearKeyMapping(unsigned short tilIndex)
    {
        if (tilIndex > ACTION_MAPPING_COUNT)
        {
            tilIndex = ACTION_MAPPING_COUNT;
        }

        for (unsigned short i = 0; i < tilIndex; i++)
        {
            this->keyboardMapping[static_cast<InputDeviceModule::Action>(i)] = OIS::KC_UNASSIGNED;
        }
    }

    void InputDeviceModule::clearButtonMapping(unsigned short tilIndex)
    {
        if (tilIndex > ACTION_MAPPING_COUNT)
        {
            tilIndex = ACTION_MAPPING_COUNT;
        }

        for (unsigned short i = 0; i < tilIndex; i++)
        {
            this->buttonMapping[static_cast<InputDeviceModule::Action>(i)] = InputDeviceModule::BUTTON_NONE;
        }
    }

    void InputDeviceModule::setDefaultKeyMapping(void)
    {
        // Start with all actions unassigned, so that a reset also removes custom bindings of actions which have no default key
        this->keyboardMapping.clear();
        this->clearKeyMapping(ACTION_MAPPING_COUNT);

        this->keyboardMapping[Action::JUMP] = OIS::KC_SPACE;
        this->keyboardMapping[Action::RUN] = OIS::KC_RCONTROL;
        this->keyboardMapping[Action::COWER] = OIS::KC_X;
        this->keyboardMapping[Action::ATTACK_1] = OIS::KC_RETURN;
        this->keyboardMapping[Action::ATTACK_2] = OIS::KC_Y;
        this->keyboardMapping[Action::DUCK] = OIS::KC_C;
        this->keyboardMapping[Action::SNEAK] = OIS::KC_RSHIFT;
        this->keyboardMapping[Action::ACTION] = OIS::KC_E;
        this->keyboardMapping[Action::RELOAD] = OIS::KC_R;
        this->keyboardMapping[Action::INVENTORY] = OIS::KC_I;
        this->keyboardMapping[Action::MAP] = OIS::KC_TAB;
        this->keyboardMapping[Action::PAUSE] = OIS::KC_P;
        this->keyboardMapping[Action::START] = OIS::KC_ESCAPE;
        this->keyboardMapping[Action::SAVE] = OIS::KC_F5;
        this->keyboardMapping[Action::LOAD] = OIS::KC_F4;
        this->keyboardMapping[Action::CAMERA_FORWARD] = OIS::KC_W;
        this->keyboardMapping[Action::CAMERA_BACKWARD] = OIS::KC_S;
        this->keyboardMapping[Action::CAMERA_LEFT] = OIS::KC_A;
        this->keyboardMapping[Action::CAMERA_RIGHT] = OIS::KC_D;
        this->keyboardMapping[Action::CAMERA_UP] = OIS::KC_PGUP;
        this->keyboardMapping[Action::CAMERA_DOWN] = OIS::KC_PGDOWN;
        this->keyboardMapping[Action::CONSOLE] = OIS::KC_GRAVE;
        this->keyboardMapping[Action::WEAPON_CHANGE_FORWARD] = OIS::KC_2;
        this->keyboardMapping[Action::WEAPON_CHANGE_BACKWARD] = OIS::KC_1;
        this->keyboardMapping[Action::FLASH_LIGHT] = OIS::KC_F;
        this->keyboardMapping[Action::SELECT] = OIS::KC_LSHIFT;
        this->keyboardMapping[Action::GRID] = OIS::KC_LCONTROL;
        this->keyboardMapping[Action::UP] = OIS::KC_UP;
        this->keyboardMapping[Action::DOWN] = OIS::KC_DOWN;
        this->keyboardMapping[Action::LEFT] = OIS::KC_LEFT;
        this->keyboardMapping[Action::RIGHT] = OIS::KC_RIGHT;
    }

    void InputDeviceModule::setDefaultButtonMapping(void)
    {
        // Logical buttons (Xbox naming). Because raw indices are translated via the joystick layout,
        // this default is correct for Xbox pads, the Steam Deck and generic pads alike.
        // Note: The D-pad always moves (UP/DOWN/LEFT/RIGHT), so it is intentionally not used for other actions.
        this->buttonMapping.clear();
        this->clearButtonMapping(ACTION_MAPPING_COUNT);

        this->buttonMapping[Action::JUMP] = JoyStickButton::BUTTON_A;
        this->buttonMapping[Action::ACTION] = JoyStickButton::BUTTON_B;
        this->buttonMapping[Action::ATTACK_1] = JoyStickButton::BUTTON_X;
        this->buttonMapping[Action::ATTACK_2] = JoyStickButton::BUTTON_Y;
        this->buttonMapping[Action::RUN] = JoyStickButton::BUTTON_RT;
        this->buttonMapping[Action::SNEAK] = JoyStickButton::BUTTON_LT;
        this->buttonMapping[Action::WEAPON_CHANGE_FORWARD] = JoyStickButton::BUTTON_RB;
        this->buttonMapping[Action::WEAPON_CHANGE_BACKWARD] = JoyStickButton::BUTTON_LB;
        this->buttonMapping[Action::COWER] = JoyStickButton::BUTTON_LEFT_STICK;
        this->buttonMapping[Action::RELOAD] = JoyStickButton::BUTTON_RIGHT_STICK;
        this->buttonMapping[Action::MAP] = JoyStickButton::BUTTON_SELECT;
        this->buttonMapping[Action::START] = JoyStickButton::BUTTON_START;
        this->buttonMapping[Action::PAUSE] = JoyStickButton::BUTTON_START;
        this->buttonMapping[Action::UP] = JoyStickButton::BUTTON_LEFT_STICK_UP;
        this->buttonMapping[Action::DOWN] = JoyStickButton::BUTTON_LEFT_STICK_DOWN;
        this->buttonMapping[Action::LEFT] = JoyStickButton::BUTTON_LEFT_STICK_LEFT;
        this->buttonMapping[Action::RIGHT] = JoyStickButton::BUTTON_LEFT_STICK_RIGHT;
        this->buttonMapping[Action::CAMERA_FORWARD] = JoyStickButton::BUTTON_RIGHT_STICK_UP;
        this->buttonMapping[Action::CAMERA_BACKWARD] = JoyStickButton::BUTTON_RIGHT_STICK_DOWN;
        this->buttonMapping[Action::CAMERA_LEFT] = JoyStickButton::BUTTON_RIGHT_STICK_LEFT;
        this->buttonMapping[Action::CAMERA_RIGHT] = JoyStickButton::BUTTON_RIGHT_STICK_RIGHT;
    }

    Ogre::Real InputDeviceModule::getLeftStickHorizontalMovingStrength(void) const
    {
        if (true == this->isKeyboard)
        {
            if (nullptr != this->companionModule)
            {
                return this->companionModule->getLeftStickHorizontalMovingStrength();
            }
            return 0.0f;
        }
        return this->leftStickMovement.x;
    }

    Ogre::Real InputDeviceModule::getLeftStickVerticalMovingStrength(void) const
    {
        if (true == this->isKeyboard)
        {
            if (nullptr != this->companionModule)
            {
                return this->companionModule->getLeftStickVerticalMovingStrength();
            }
            return 0.0f;
        }
        return this->leftStickMovement.y;
    }

    Ogre::Real InputDeviceModule::getRightStickHorizontalMovingStrength(void) const
    {
        if (true == this->isKeyboard)
        {
            if (nullptr != this->companionModule)
            {
                return this->companionModule->getRightStickHorizontalMovingStrength();
            }
            return 0.0f;
        }
        return this->rightStickMovement.x;
    }

    Ogre::Real InputDeviceModule::getRightStickVerticalMovingStrength(void) const
    {
        if (true == this->isKeyboard)
        {
            if (nullptr != this->companionModule)
            {
                return this->companionModule->getRightStickVerticalMovingStrength();
            }
            return 0.0f;
        }
        return this->rightStickMovement.y;
    }

    bool InputDeviceModule::isActionDownOwnDevice(InputDeviceModule::Action action)
    {
        if (true == this->bLock)
        {
            return false;
        }

        // Note: Previously the keyboard path was chosen via "getKeyboardInputDeviceModules().front() == this". The main keyboard module was a separate
        // instance, so it always went down the joystick path and e.g. isActionDown(NOWA_A_MAP) on the main module never worked.
        if (true == this->isKeyboard)
        {
            OIS::Keyboard* keyboard = static_cast<OIS::Keyboard*>(this->deviceObject);
            if (nullptr == keyboard)
            {
                return false;
            }
            const OIS::KeyCode keyCode = this->getMappedKey(action);
            if (OIS::KC_UNASSIGNED == keyCode)
            {
                return false;
            }
            return keyboard->isKeyDown(keyCode);
        }

        // ==============================
        // JOYSTICK PATH
        // ==============================

        bool somethingDown = false;

        // Treat analog stick as "action down" only if it crosses a threshold.
        // Otherwise tiny stick drift / tiny deflection triggers LEFT/RIGHT immediately.
        const Ogre::Real actionThreshold = this->analogActionThreshold;

        if (InputDeviceModule::UP == action)
        {
            if (Ogre::Math::Abs(this->leftStickMovement.y) >= actionThreshold)
            {
                somethingDown |= (this->leftStickMovement.y < 0.0f);
            }
            else
            {
                somethingDown |= (this->pressedPov[0] == Action::UP);
            }
        }
        else if (InputDeviceModule::DOWN == action)
        {
            if (Ogre::Math::Abs(this->leftStickMovement.y) >= actionThreshold)
            {
                somethingDown |= (this->leftStickMovement.y > 0.0f);
            }
            else
            {
                somethingDown |= (this->pressedPov[0] == Action::DOWN);
            }
        }
        else if (InputDeviceModule::LEFT == action)
        {
            if (Ogre::Math::Abs(this->leftStickMovement.x) >= actionThreshold)
            {
                somethingDown |= (this->leftStickMovement.x < 0.0f);
            }
            else
            {
                somethingDown |= (this->pressedPov[1] == Action::LEFT);
            }
        }
        else if (InputDeviceModule::RIGHT == action)
        {
            if (Ogre::Math::Abs(this->leftStickMovement.x) >= actionThreshold)
            {
                somethingDown |= (this->leftStickMovement.x > 0.0f);
            }
            else
            {
                somethingDown |= (this->pressedPov[1] == Action::RIGHT);
            }
        }

        // Additionally the mapped button (this also allows remapping directions, e.g. to the D-pad of a second stick)
        const JoyStickButton mappedButton = this->getMappedButton(action);
        if (JoyStickButton::BUTTON_NONE != mappedButton)
        {
            somethingDown |= this->isButtonDownOwnDevice(mappedButton);
        }

        return somethingDown;
    }

    bool InputDeviceModule::isActionDown(InputDeviceModule::Action action)
    {
        if (true == this->bLock)
        {
            return false;
        }

        bool somethingDown = this->isActionDownOwnDevice(action);

        // "Auto" device: keyboard and companion gamepad are merged
        if (false == somethingDown && nullptr != this->companionModule)
        {
            somethingDown = this->companionModule->isActionDownOwnDevice(action);
        }

        return somethingDown;
    }

    bool InputDeviceModule::isKeyDown(OIS::KeyCode keyCode) const
    {
        if (true == this->bLock)
        {
            return false;
        }

        // A gamepad module must not see the keyboard, else in splitscreen a gamepad player would react on keyboard keys
        if (false == this->isKeyboard)
        {
            return false;
        }

        OIS::Keyboard* keyboard = static_cast<OIS::Keyboard*>(this->deviceObject);
        if (nullptr == keyboard)
        {
            return false;
        }

        return keyboard->isKeyDown(keyCode);
    }

    bool InputDeviceModule::isActionDownAmount(InputDeviceModule::Action action, Ogre::Real dt, Ogre::Real actionDuration)
    {
        if (true == this->bLock)
        {
            return false;
        }

        const unsigned short index = static_cast<unsigned short>(action);
        if (index >= ACTION_SLOT_COUNT)
        {
            return false;
        }

        const bool down = this->isActionDown(action);

        if (this->timeSinceLastActionDown[index] >= 0.0f)
        {
            this->timeSinceLastActionDown[index] = this->timeSinceLastActionDown[index] - dt;
            if (true == down)
            {
                return true;
            }
        }

        if (false == down)
        {
            this->timeSinceLastActionDown[index] = actionDuration;
        }

        return false;
    }

    bool InputDeviceModule::isActionPressed(InputDeviceModule::Action action, Ogre::Real dt, Ogre::Real durationBetweenTheAction)
    {
        if (true == this->bLock)
        {
            return false;
        }

        const unsigned short index = static_cast<unsigned short>(action);
        if (index >= ACTION_SLOT_COUNT)
        {
            return false;
        }

        if (this->timeSinceLastActionPressed[index] >= 0.0f)
        {
            this->timeSinceLastActionPressed[index] = this->timeSinceLastActionPressed[index] - dt;
        }

        if (this->timeSinceLastActionPressed[index] <= 0.0f)
        {
            const bool down = this->isActionDown(action);

            if (false == down)
            {
                this->canPress[index] = true;
            }
            else if (true == this->canPress[index])
            {
                this->canPress[index] = false;
                this->timeSinceLastActionPressed[index] = durationBetweenTheAction;
                return true;
            }
        }

        return false;
    }

    bool InputDeviceModule::isButtonDownOwnDevice(JoyStickButton button) const
    {
        if (true == this->bLock || JoyStickButton::BUTTON_NONE == button)
        {
            return false;
        }

        const unsigned int buttonIndex = static_cast<unsigned int>(button);
        if (buttonIndex >= MAX_BUTTON_MASK_BITS)
        {
            return false;
        }

        // Attention: One atomic read of the complete button state, see pressedButtonMask. Previously a std::vector was iterated here, which the
        // capture (render) thread cleared and refilled at the same time -> "vector subscript out of range".
        // Several buttons at once still work (e.g. holding RUN while pressing JUMP), because every button has its own bit.
        return 0u != (this->pressedButtonMask.load(std::memory_order_acquire) & (1u << buttonIndex));
    }

    bool InputDeviceModule::isButtonDown(JoyStickButton button) const
    {
        if (true == this->isKeyboard)
        {
            if (nullptr != this->companionModule)
            {
                return this->companionModule->isButtonDownOwnDevice(button);
            }
            return false;
        }
        return this->isButtonDownOwnDevice(button);
    }

    bool InputDeviceModule::areButtonsDown2(JoyStickButton button1, JoyStickButton button2) const
    {
        return true == this->isButtonDown(button1) && true == this->isButtonDown(button2);
    }

    bool InputDeviceModule::areButtonsDown3(JoyStickButton button1, JoyStickButton button2, JoyStickButton button3) const
    {
        return true == this->areButtonsDown2(button1, button2) && true == this->isButtonDown(button3);
    }

    bool InputDeviceModule::areButtonsDown4(JoyStickButton button1, JoyStickButton button2, JoyStickButton button3, JoyStickButton button4) const
    {
        return true == this->areButtonsDown3(button1, button2, button3) && true == this->isButtonDown(button4);
    }

    InputDeviceModule::JoyStickButton InputDeviceModule::getPressedButton(void) const
    {
        if (true == this->isKeyboard)
        {
            if (nullptr != this->companionModule)
            {
                return this->companionModule->getPressedButton();
            }
            return JoyStickButton::BUTTON_NONE;
        }
        return this->pressedButton.load(std::memory_order_relaxed);
    }

    std::vector<InputDeviceModule::JoyStickButton> InputDeviceModule::getPressedButtons(void) const
    {
        if (true == this->isKeyboard)
        {
            if (nullptr != this->companionModule)
            {
                return this->companionModule->getPressedButtons();
            }
            return std::vector<InputDeviceModule::JoyStickButton>();
        }
        std::vector<JoyStickButton> pressedButtons;

        const unsigned int mask = this->pressedButtonMask.load(std::memory_order_acquire);
        for (unsigned int i = 0; i < MAX_BUTTON_MASK_BITS; i++)
        {
            if (0u != (mask & (1u << i)))
            {
                pressedButtons.emplace_back(static_cast<JoyStickButton>(i));
            }
        }
        return pressedButtons;
    }

    void InputDeviceModule::addPressedButton(JoyStickButton button)
    {
        const unsigned int buttonIndex = static_cast<unsigned int>(button);
        if (JoyStickButton::BUTTON_NONE == button || buttonIndex >= MAX_BUTTON_MASK_BITS)
        {
            return;
        }

        // Only collected here, the complete state is published at the end of update, see pressedButtonMask
        this->capturedButtonMask |= (1u << buttonIndex);
        this->lastCapturedButton = button;
    }

    void InputDeviceModule::update(Ogre::Real dt)
    {
        if (true == this->bLock)
        {
            return;
        }

        if (true == this->isKeyboard || nullptr == this->deviceObject)
        {
            return;
        }

        OIS::JoyStick* joyStick = static_cast<OIS::JoyStick*>(this->deviceObject);

        // Evaluate which buttons are pressed (maybe more at once) and collect them in the mask
        this->capturedButtonMask = 0u;
        this->lastCapturedButton = JoyStickButton::BUTTON_NONE;

        const OIS::JoyStickState& joystickState = joyStick->getJoyStickState();
        const LayoutAxes layoutAxes = getLayoutAxes(this->joyStickLayout);

        auto readAxis = [&](int axisIndex) -> Ogre::Real
        {
            if (axisIndex < 0 || axisIndex >= static_cast<int>(joystickState.mAxes.size()))
            {
                return 0.0f;
            }
            return Clamp1(static_cast<Ogre::Real>(joystickState.mAxes[axisIndex].abs) / 32767.0f);
        };

        auto applyDeadzone = [&](Ogre::Real v, Ogre::Real deadzone) -> Ogre::Real
        {
            if (Ogre::Math::Abs(v) < deadzone)
            {
                return 0.0f;
            }
            return v;
        };

        // Makes tiny stick motions less aggressive (good for steering feel)
        // expo = 0.0 -> linear, expo = 0.5 -> softer center, expo = 0.7 -> very soft center
        auto applyExpo = [&](Ogre::Real v, Ogre::Real expo) -> Ogre::Real
        {
            const Ogre::Real a = Ogre::Math::Abs(v);
            Ogre::Real s = 1.0f;
            if (v < 0.0f)
            {
                s = -1.0f;
            }
            const Ogre::Real cubic = a * a * a;
            const Ogre::Real out = (1.0f - expo) * a + expo * cubic;
            return s * out;
        };

        // Only treat stick as a DIGITAL button if pushed far enough.
        const Ogre::Real stickDigitalThreshold = 0.55f;
        const Ogre::Real stickExpo = 0.70f;
        const Ogre::Real digitalThreshold = 0.90f;

        // ----------------------------
        // POV / D-Pad
        // ----------------------------
        this->pressedPov[0] = Action::NONE;
        this->pressedPov[1] = Action::NONE;
        this->pressedPov[2] = Action::NONE;
        this->pressedPov[3] = Action::NONE;

        const int povCount = joyStick->getNumberOfComponents(OIS::OIS_POV);

        if (povCount > 0)
        {
            const int direction = joystickState.mPOV[0].direction;

            // Note: Previously only west/east were evaluated, so the D-pad could not be used for up/down at all
            if (0 != (direction & OIS::Pov::North))
            {
                this->pressedPov[0] = Action::UP;
                this->addPressedButton(BUTTON_DPAD_UP);
            }
            else if (0 != (direction & OIS::Pov::South))
            {
                this->pressedPov[0] = Action::DOWN;
                this->addPressedButton(BUTTON_DPAD_DOWN);
            }

            if (0 != (direction & OIS::Pov::West))
            {
                this->pressedPov[1] = Action::LEFT;
                this->addPressedButton(BUTTON_DPAD_LEFT);
            }
            else if (0 != (direction & OIS::Pov::East))
            {
                this->pressedPov[1] = Action::RIGHT;
                this->addPressedButton(BUTTON_DPAD_RIGHT);
            }
        }

        // Some old generic pads report the right stick as additional hats (legacy behavior, only for the generic layout)
        if (LAYOUT_GENERIC == this->joyStickLayout && povCount > 3)
        {
            if (0 != (joystickState.mPOV[2].direction & OIS::Pov::North))
            {
                this->addPressedButton(BUTTON_RIGHT_STICK_UP);
            }
            else if (0 != (joystickState.mPOV[2].direction & OIS::Pov::South))
            {
                this->addPressedButton(BUTTON_RIGHT_STICK_DOWN);
            }

            if (0 != (joystickState.mPOV[3].direction & OIS::Pov::West))
            {
                this->addPressedButton(BUTTON_RIGHT_STICK_LEFT);
            }
            else if (0 != (joystickState.mPOV[3].direction & OIS::Pov::East))
            {
                this->addPressedButton(BUTTON_RIGHT_STICK_RIGHT);
            }
        }

        // ----------------------------
        // Analog sticks
        // ----------------------------

        // Left stick Y (up is negative)
        this->leftStickMovement.y = readAxis(layoutAxes.leftY);
        this->leftStickMovement.y = applyDeadzone(this->leftStickMovement.y, this->joyStickDeadZone);
        this->leftStickMovement.y = applyExpo(this->leftStickMovement.y, stickExpo);

        if (this->leftStickMovement.y <= -stickDigitalThreshold)
        {
            this->addPressedButton(BUTTON_LEFT_STICK_UP);
        }
        else if (this->leftStickMovement.y >= stickDigitalThreshold)
        {
            this->addPressedButton(BUTTON_LEFT_STICK_DOWN);
        }

        // Left stick X, range is -1..+1, full output is already reached at 90% deflection
        Ogre::Real rawLX = readAxis(layoutAxes.leftX);
        rawLX = rawLX / (1.0f - 0.10f);
        rawLX = Clamp1(rawLX);

        // Precision steering mapping
        this->leftStickMovement.x = MapSteeringPrecision(rawLX, 0.18f, 0.55f, 0.08f);

        if (this->leftStickMovement.x <= -digitalThreshold)
        {
            this->addPressedButton(BUTTON_LEFT_STICK_LEFT);
        }
        else if (this->leftStickMovement.x >= digitalThreshold)
        {
            this->addPressedButton(BUTTON_LEFT_STICK_RIGHT);
        }

        // Right stick Y
        this->rightStickMovement.y = readAxis(layoutAxes.rightY);
        this->rightStickMovement.y = applyDeadzone(this->rightStickMovement.y, this->joyStickDeadZone);
        this->rightStickMovement.y = applyExpo(this->rightStickMovement.y, stickExpo);

        if (this->rightStickMovement.y <= -stickDigitalThreshold)
        {
            this->addPressedButton(BUTTON_RIGHT_STICK_UP);
        }
        else if (this->rightStickMovement.y >= stickDigitalThreshold)
        {
            this->addPressedButton(BUTTON_RIGHT_STICK_DOWN);
        }

        // Right stick X
        Ogre::Real rawRX = readAxis(layoutAxes.rightX);
        rawRX = rawRX / (1.0f - 0.10f);
        rawRX = Clamp1(rawRX);

        this->rightStickMovement.x = MapSteeringPrecision(rawRX, 0.18f, 0.55f, 0.08f);

        if (this->rightStickMovement.x <= -digitalThreshold)
        {
            this->addPressedButton(BUTTON_RIGHT_STICK_LEFT);
        }
        else if (this->rightStickMovement.x >= digitalThreshold)
        {
            this->addPressedButton(BUTTON_RIGHT_STICK_RIGHT);
        }

        // ----------------------------
        // Analog triggers (XInput / Linux evdev deliver LT/RT as axes)
        // ----------------------------
        if (layoutAxes.leftTrigger >= 0 && layoutAxes.leftTrigger < static_cast<int>(joystickState.mAxes.size()))
        {
            if (normalizeTrigger(joystickState.mAxes[layoutAxes.leftTrigger].abs, layoutAxes.triggerRestsAtMin) > TRIGGER_DIGITAL_THRESHOLD)
            {
                this->addPressedButton(BUTTON_LT);
            }
        }
        if (layoutAxes.rightTrigger >= 0 && layoutAxes.rightTrigger < static_cast<int>(joystickState.mAxes.size()))
        {
            if (normalizeTrigger(joystickState.mAxes[layoutAxes.rightTrigger].abs, layoutAxes.triggerRestsAtMin) > TRIGGER_DIGITAL_THRESHOLD)
            {
                this->addPressedButton(BUTTON_RT);
            }
        }

        // ----------------------------
        // Physical controller buttons, translated from raw index into logical buttons
        // ----------------------------
        for (size_t j = 0; j < joystickState.mButtons.size(); j++)
        {
            if (true == joystickState.mButtons[j])
            {
                this->addPressedButton(this->translateRawButton(static_cast<int>(j)));
            }
        }

        // Note: Buttons have priority and will overwrite pov/sticks as "the" pressed button
        this->pressedButton.store(this->lastCapturedButton, std::memory_order_relaxed);

        // Attention: The complete button state is published in ONE atomic store, so that the logic thread never sees a half updated state.
        this->pressedButtonMask.store(this->capturedButtonMask, std::memory_order_release);
    }

    void InputDeviceModule::lockDevice(bool bLock)
    {
        this->bLock = bLock;
    }

    OIS::Object* InputDeviceModule::getDeviceObject(void) const
    {
        return this->deviceObject;
    }

    void InputDeviceModule::setJoyStickLayout(JoyStickLayout joyStickLayout)
    {
        this->joyStickLayout = joyStickLayout;
        this->buildRawButtonTable();
    }

    InputDeviceModule::JoyStickLayout InputDeviceModule::getJoyStickLayout(void) const
    {
        return this->joyStickLayout;
    }

    void InputDeviceModule::buildRawButtonTable(void)
    {
        for (unsigned short i = 0; i < MAX_RAW_BUTTONS; i++)
        {
            this->rawButtonTable[i] = BUTTON_NONE;
        }

        if (LAYOUT_XINPUT == this->joyStickLayout)
        {
            // OIS Win32 XInput path: raw index i = XInput wButtons bit (i + 4)
            this->rawButtonTable[0] = BUTTON_START;
            this->rawButtonTable[1] = BUTTON_SELECT;
            this->rawButtonTable[2] = BUTTON_LEFT_STICK;
            this->rawButtonTable[3] = BUTTON_RIGHT_STICK;
            this->rawButtonTable[4] = BUTTON_LB;
            this->rawButtonTable[5] = BUTTON_RB;
            this->rawButtonTable[8] = BUTTON_A;
            this->rawButtonTable[9] = BUTTON_B;
            this->rawButtonTable[10] = BUTTON_X;
            this->rawButtonTable[11] = BUTTON_Y;
        }
        else if (LAYOUT_LINUX_EVDEV == this->joyStickLayout)
        {
            // OIS Linux: buttons are numbered in ascending evdev code order (BTN_SOUTH, BTN_EAST, BTN_NORTH, BTN_WEST, BTN_TL, BTN_TR, BTN_SELECT, BTN_START, BTN_MODE, BTN_THUMBL, BTN_THUMBR)
            this->rawButtonTable[0] = BUTTON_A;
            this->rawButtonTable[1] = BUTTON_B;
            this->rawButtonTable[2] = BUTTON_X;
            this->rawButtonTable[3] = BUTTON_Y;
            this->rawButtonTable[4] = BUTTON_LB;
            this->rawButtonTable[5] = BUTTON_RB;
            this->rawButtonTable[6] = BUTTON_SELECT;
            this->rawButtonTable[7] = BUTTON_START;
            // 8 = Guide button, reserved by Steam
            this->rawButtonTable[9] = BUTTON_LEFT_STICK;
            this->rawButtonTable[10] = BUTTON_RIGHT_STICK;
        }
        else
        {
            // Legacy NOWA layout: raw index == enum value for the 12 physical buttons.
            // Note: Previously ANY raw index was casted, so e.g. a 13th physical button was reported as BUTTON_LEFT_STICK_UP.
            for (unsigned short i = 0; i <= static_cast<unsigned short>(BUTTON_RIGHT_STICK); i++)
            {
                this->rawButtonTable[i] = static_cast<JoyStickButton>(i);
            }
        }
    }

    InputDeviceModule::JoyStickButton InputDeviceModule::translateRawButton(int rawButton) const
    {
        if (rawButton < 0 || rawButton >= static_cast<int>(MAX_RAW_BUTTONS))
        {
            return BUTTON_NONE;
        }
        return this->rawButtonTable[rawButton];
    }

    InputDeviceModule::JoyStickButton InputDeviceModule::translateRawAxis(int axis, int absValue) const
    {
        const LayoutAxes layoutAxes = getLayoutAxes(this->joyStickLayout);

        if (axis == layoutAxes.leftTrigger)
        {
            if (normalizeTrigger(absValue, layoutAxes.triggerRestsAtMin) > TRIGGER_DIGITAL_THRESHOLD)
            {
                return BUTTON_LT;
            }
            return BUTTON_NONE;
        }
        if (axis == layoutAxes.rightTrigger)
        {
            if (normalizeTrigger(absValue, layoutAxes.triggerRestsAtMin) > TRIGGER_DIGITAL_THRESHOLD)
            {
                return BUTTON_RT;
            }
            return BUTTON_NONE;
        }

        const Ogre::Real value = Clamp1(static_cast<Ogre::Real>(absValue) / 32767.0f);

        if (axis == layoutAxes.leftX)
        {
            if (value <= -RAW_AXIS_DIGITAL_THRESHOLD)
            {
                return BUTTON_LEFT_STICK_LEFT;
            }
            if (value >= RAW_AXIS_DIGITAL_THRESHOLD)
            {
                return BUTTON_LEFT_STICK_RIGHT;
            }
        }
        else if (axis == layoutAxes.leftY)
        {
            if (value <= -RAW_AXIS_DIGITAL_THRESHOLD)
            {
                return BUTTON_LEFT_STICK_UP;
            }
            if (value >= RAW_AXIS_DIGITAL_THRESHOLD)
            {
                return BUTTON_LEFT_STICK_DOWN;
            }
        }
        else if (axis == layoutAxes.rightX)
        {
            if (value <= -RAW_AXIS_DIGITAL_THRESHOLD)
            {
                return BUTTON_RIGHT_STICK_LEFT;
            }
            if (value >= RAW_AXIS_DIGITAL_THRESHOLD)
            {
                return BUTTON_RIGHT_STICK_RIGHT;
            }
        }
        else if (axis == layoutAxes.rightY)
        {
            if (value <= -RAW_AXIS_DIGITAL_THRESHOLD)
            {
                return BUTTON_RIGHT_STICK_UP;
            }
            if (value >= RAW_AXIS_DIGITAL_THRESHOLD)
            {
                return BUTTON_RIGHT_STICK_DOWN;
            }
        }

        return BUTTON_NONE;
    }

    InputDeviceModule::JoyStickButton InputDeviceModule::translatePov(int povDirection) const
    {
        if (0 != (povDirection & OIS::Pov::North))
        {
            return BUTTON_DPAD_UP;
        }
        if (0 != (povDirection & OIS::Pov::South))
        {
            return BUTTON_DPAD_DOWN;
        }
        if (0 != (povDirection & OIS::Pov::West))
        {
            return BUTTON_DPAD_LEFT;
        }
        if (0 != (povDirection & OIS::Pov::East))
        {
            return BUTTON_DPAD_RIGHT;
        }
        return BUTTON_NONE;
    }

    void InputDeviceModule::setCompanionModule(InputDeviceModule* companionModule, bool soft)
    {
        if (nullptr != this->companionModule && this->companionModule != companionModule)
        {
            this->companionModule->companionOwner = nullptr;
        }

        this->companionModule = companionModule;
        this->companionSoft = soft;

        if (nullptr != this->companionModule)
        {
            this->companionModule->companionOwner = this;
        }
    }

    InputDeviceModule* InputDeviceModule::getCompanionModule(void) const
    {
        return this->companionModule;
    }

    bool InputDeviceModule::isCompanionSoft(void) const
    {
        return this->companionSoft;
    }

    InputDeviceModule* InputDeviceModule::getCompanionOwner(void) const
    {
        return this->companionOwner;
    }

    void InputDeviceModule::setLastInputFromJoyStick(bool lastInputFromJoyStick)
    {
        this->lastInputFromJoyStick = lastInputFromJoyStick;
    }

    bool InputDeviceModule::isLastInputFromJoyStick(void) const
    {
        return this->lastInputFromJoyStick;
    }

    bool InputDeviceModule::isJoinInputDown(void)
    {
        return true == this->isActionDownOwnDevice(InputDeviceModule::JUMP) || true == this->isActionDownOwnDevice(InputDeviceModule::START);
    }

    Ogre::String InputDeviceModule::getActionName(Action action)
    {
        for (const ActionName& actionName : actionNames)
        {
            if (action == actionName.action)
            {
                return actionName.name;
            }
        }
        return "NONE";
    }

    InputDeviceModule::Action InputDeviceModule::getActionFromName(const Ogre::String& actionName)
    {
        for (const ActionName& entry : actionNames)
        {
            if (actionName == entry.name)
            {
                return entry.action;
            }
        }
        return Action::NONE;
    }

    Ogre::String InputDeviceModule::getJoyStickLayoutName(JoyStickLayout joyStickLayout)
    {
        if (LAYOUT_XINPUT == joyStickLayout)
        {
            return "XInput";
        }
        else if (LAYOUT_LINUX_EVDEV == joyStickLayout)
        {
            return "LinuxEvdev";
        }
        return "Generic";
    }

    InputDeviceModule::JoyStickLayout InputDeviceModule::getJoyStickLayoutFromName(const Ogre::String& layoutName)
    {
        if ("XInput" == layoutName)
        {
            return LAYOUT_XINPUT;
        }
        else if ("LinuxEvdev" == layoutName)
        {
            return LAYOUT_LINUX_EVDEV;
        }
        return LAYOUT_GENERIC;
    }

    InputDeviceModule::JoyStickLayout InputDeviceModule::detectJoyStickLayout(OIS::JoyStick* joyStick)
    {
#if defined OIS_LINUX_PLATFORM
        return LAYOUT_LINUX_EVDEV;
#else
        if (nullptr == joyStick)
        {
            return LAYOUT_GENERIC;
        }

        // OIS translates every XInput device (Xbox pads, Steam Input virtual pad, Steam Deck under Proton) to exactly 12 buttons, 6 axes and 1 pov
        const int buttonCount = joyStick->getNumberOfComponents(OIS::OIS_Button);
        const int axisCount = joyStick->getNumberOfComponents(OIS::OIS_Axis);
        const int povCount = joyStick->getNumberOfComponents(OIS::OIS_POV);

        if (12 == buttonCount && 6 == axisCount && 1 == povCount)
        {
            return LAYOUT_XINPUT;
        }
        return LAYOUT_GENERIC;
#endif
    }

} // namespace end
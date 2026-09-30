#include "NOWAPrecompiled.h"
#include "InputDeviceCore.h"
#include "console/LuaConsole.h"
#include "gameobject/MyGUIItemBoxComponent.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "modules/GraphicsModule.h"
#include "modules/InputDeviceModule.h"

namespace
{
    /*
     * Short answer: MyGUI does not derive capital letters from Shift for you
     * it inserts whatever you pass as the text argument of injectKeyPress.
     * In your path, you forward OIS events across threads and you never adjust text based on modifier state.
     * If OIS delivers 'a' (or even 0) for KC_A while Shift is down, MyGUI will still insert lowercase (or nothing).
     * Because you enqueue to the render thread, even the order of “Shift down” -> “A down” can be one frame apart, so relying on MyGUI’s internal modifier state is fragile.
     * So this is the fix:
     */
    MyGUI::Char applyModifiers(MyGUI::Char ch, const OIS::Keyboard* kb)
    {
        if (!kb)
        {
            return ch;
        }

        const bool lshift = kb->isKeyDown(OIS::KC_LSHIFT);
        const bool rshift = kb->isKeyDown(OIS::KC_RSHIFT);
        const bool shift = lshift || rshift;

        // OIS doesn’t expose CapsLock state as a toggle easily.
        // If you maintain it elsewhere, fold it in here:
        const bool caps = false; // or your own tracked caps toggle

        if (ch >= 'a' && ch <= 'z')
        {
            if (shift ^ caps)
            {
                ch = static_cast<MyGUI::Char>(ch - 'a' + 'A');
            }
            return ch;
        }

        if (false == shift)
        {
            return ch;
        }

        // German QWERTZ layout - Shift mapping for the number row and the
        // German-specific keys. This replaces the previous US-layout table,
        // which was wrong for every symbol except '-' -> '_'.
        //
        // German number row (unshifted -> shifted):
        //   1 ! | 2 " | 3 § | 4 $ | 5 % | 6 & | 7 / | 8 ( | 9 ) | 0 =
        // German-specific keys:
        //   ß -> ?      (the most common source of '?' on a German keyboard)
        //   ´ -> `      (dead key, rarely relevant for direct text input)
        //   + -> *
        //   # -> '
        //   ü -> Ü | ö -> Ö | ä -> Ä
        //   , -> ; | . -> : | - -> _
        switch (ch)
        {
        case '1':
            ch = '!';
            break;
        case '2':
            ch = '"';
            break;
        case '3':
            ch = static_cast<MyGUI::Char>(0x00A7);
            break; // §
        case '4':
            ch = '$';
            break;
        case '5':
            ch = '%';
            break;
        case '6':
            ch = '&';
            break;
        case '7':
            ch = '/';
            break;
        case '8':
            ch = '(';
            break;
        case '9':
            ch = ')';
            break;
        case '0':
            ch = '=';
            break;
        case static_cast<MyGUI::Char>(0x00DF):
            ch = '?';
            break; // ß -> ?
        case static_cast<MyGUI::Char>(0x00B4):
            ch = '`';
            break; // ´ -> `
        case '+':
            ch = '*';
            break;
        case '#':
            ch = '\'';
            break;
        case static_cast<MyGUI::Char>(0x00FC):
            ch = static_cast<MyGUI::Char>(0x00DC);
            break; // ü -> Ü
        case static_cast<MyGUI::Char>(0x00F6):
            ch = static_cast<MyGUI::Char>(0x00D6);
            break; // ö -> Ö
        case static_cast<MyGUI::Char>(0x00E4):
            ch = static_cast<MyGUI::Char>(0x00C4);
            break; // ä -> Ä
        case ',':
            ch = ';';
            break;
        case '.':
            ch = ':';
            break;
        case '-':
            ch = '_';
            break;
        default:
            break;
        }

        return ch;
    }
}

namespace NOWA
{
    // Logical device names. The same strings are used by the InputDeviceComponent.
    //  "Auto":       Keyboard plus the first free gamepad, merged into one device (single player, Steam Deck). If there is no keyboard, only the gamepad is used.
    //  "Join":       No device until a free device presses JUMP or START (splitscreen lobby). Handled by the InputDeviceComponent.
    //  "Keyboard":   The (first) keyboard only.
    //  "Gamepad N":  The N-th connected gamepad only (1-based).
    // Physical OIS vendor names (e.g. "Win32InputManager") are still accepted.
    const Ogre::String InputDeviceCore_DeviceAuto = "Auto";
    const Ogre::String InputDeviceCore_DeviceJoin = "Join";
    const Ogre::String InputDeviceCore_DeviceKeyboard = "Keyboard";
    const Ogre::String InputDeviceCore_DeviceGamepadPrefix = "Gamepad ";
    const unsigned short InputDeviceCore_MinListedGamepads = 4;

    InputDeviceCore::InputDeviceCore() :
        mouse(nullptr),
        keyboard(nullptr),
        inputSystem(nullptr),
        mainInputDeviceModule(nullptr),
        bSelectDown(false),
        bLock(false),
        keyDispatchDepth(0),
        mouseDispatchDepth(0),
        joystickDispatchDepth(0),
        joyStickLayoutOverride("Auto"),
        diagnosticRawButtonLogCount(0)
    {
    }

    InputDeviceCore::~InputDeviceCore()
    {
    }

    InputDeviceCore* InputDeviceCore::getSingletonPtr(void)
    {
        return msSingleton;
    }

    InputDeviceCore& InputDeviceCore::getSingleton(void)
    {
        assert(msSingleton);
        return (*msSingleton);
    }

    void InputDeviceCore::destroyContent(void)
    {
        if (nullptr != this->inputSystem)
        {
            // Note: The main keyboard module is keyboardInputDeviceModules[0] (no separate instance anymore), so it must not be deleted twice
            this->mainInputDeviceModule = nullptr;

            for (size_t i = 0; i < this->keyboardInputDeviceModules.size(); i++)
            {
                InputDeviceModule* inputDeviceModule = this->keyboardInputDeviceModules[i];
                delete inputDeviceModule;
            }
            this->keyboardInputDeviceModules.clear();

            for (size_t i = 0; i < this->joystickInputDeviceModules.size(); i++)
            {
                InputDeviceModule* inputDeviceModule = this->joystickInputDeviceModules[i];
                delete inputDeviceModule;
            }
            this->joystickInputDeviceModules.clear();

            if (this->mouse)
            {
                this->inputSystem->destroyInputObject(this->mouse);
                this->mouse = 0;
            }

            for (size_t i = 0; i < this->additionalKeyboards.size(); i++)
            {
                this->inputSystem->destroyInputObject(this->additionalKeyboards[i]);
            }
            this->additionalKeyboards.clear();

            if (this->keyboard)
            {
                this->inputSystem->destroyInputObject(this->keyboard);
                this->keyboard = 0;
            }

            if (this->joysticks.size() > 0)
            {
                auto itJoystick = this->joysticks.begin();
                auto itJoystickEnd = this->joysticks.end();
                for (; itJoystick != itJoystickEnd; ++itJoystick)
                {
                    this->inputSystem->destroyInputObject(*itJoystick);
                }

                this->joysticks.clear();
            }

            // If you use OIS1.0RC1 or above, uncomment this line
            // and comment the line below it
            this->inputSystem->destroyInputSystem(this->inputSystem);
            // this->inputSystem->destroyInputSystem();
            this->inputSystem = nullptr;
            this->bSelectDown = false;
            this->bLock = false;

            // Clear Listeners
            this->keyListenerStack.clear();
            this->mouseListenerStack.clear();
            this->joystickListenerStack.clear();

            this->keyListenerIndex.clear();
            this->mouseListenerIndex.clear();
            this->joystickListenerIndex.clear();

            this->pendingRemoveKeys.clear();
            this->pendingRemoveMice.clear();
            this->pendingRemoveJoysticks.clear();

            this->keyDispatchDepth = 0;
            this->mouseDispatchDepth = 0;
            this->joystickDispatchDepth = 0;
        }
    }

    void InputDeviceCore::initialise(Ogre::Window* renderWindow)
    {
        if (nullptr == this->inputSystem)
        {
            OIS::ParamList paramList;
            size_t windowHnd = 0;
            std::stringstream windowHndStr;

            renderWindow->getCustomAttribute("WINDOW", &windowHnd);
            windowHndStr << windowHnd;
            paramList.insert({"WINDOW", windowHndStr.str()});

#if defined OIS_LINUX_PLATFORM
            paramList.insert({"x11_mouse_grab", "false"});
            paramList.insert({"x11_keyboard_grab", "false"});
#endif

            this->inputSystem = OIS::InputManager::createInputSystem(paramList);

            int numKeyboards = this->inputSystem->getNumberOfDevices(OIS::OISKeyboard);
            for (int keyboardIndex = 0; keyboardIndex < numKeyboards; keyboardIndex++)
            {
                OIS::Keyboard* kb = static_cast<OIS::Keyboard*>(this->inputSystem->createInputObject(OIS::OISKeyboard, true));
                kb->setEventCallback(this);

                if (keyboardIndex == 0)
                {
                    this->keyboard = kb;
                }
                else
                {
                    this->additionalKeyboards.push_back(kb);
                }

                std::string deviceName = kb->vendor();
                if (deviceName.empty())
                {
                    deviceName = "Keyboard" + std::to_string(keyboardIndex);
                }
                else if (keyboardIndex > 0)
                {
                    deviceName += "_" + Ogre::StringConverter::toString(keyboardIndex);
                }
                this->addDevice(deviceName, true, kb);
            }

            // No keyboard at all (e.g. console like setups): create a virtual keyboard module without device object.
            // Its isActionDown etc. simply deliver false, but it can still carry a companion gamepad, so that all code using
            // getMainKeyboardInputDeviceModule() keeps working and menus can be controlled with the gamepad.
            if (true == this->keyboardInputDeviceModules.empty())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[InputDeviceCore] No keyboard found, creating virtual keyboard module. Gamepads will be used.");
                this->addDevice("VirtualKeyboard", true, nullptr);
            }

            // BUGFIX: Previously the main module was a SEPARATE InputDeviceModule instance ("MainKeyboard"). The configuration (Core::loadCustomConfiguration and the
            // ConfigurationState) remapped keys on that instance, but game objects got keyboardInputDeviceModules[0] assigned via their InputDeviceComponent,
            // which never saw the remapped keys. Now the main module IS the first keyboard module.
            this->mainInputDeviceModule = this->keyboardInputDeviceModules[0];

            if (this->inputSystem->getNumberOfDevices(OIS::OISMouse) > 0)
            {
                this->mouse = static_cast<OIS::Mouse*>(this->inputSystem->createInputObject(OIS::OISMouse, true));
                this->mouse->setEventCallback(this);

                // Get window size
                unsigned int width, height;
                int left, top;
                renderWindow->getMetrics(width, height, left, top);

                // Set mouse region
                this->setWindowExtents(width, height);
            }

            int numJoysticks = this->inputSystem->getNumberOfDevices(OIS::OISJoyStick);
            for (int joystickIndex = 0; joystickIndex < numJoysticks; joystickIndex++)
            {
                OIS::JoyStick* joystick = static_cast<OIS::JoyStick*>(this->inputSystem->createInputObject(OIS::OISJoyStick, true));
                this->joysticks.push_back(joystick);

                std::string deviceName = joystick->vendor();
                if (deviceName.empty())
                {
                    deviceName = "Joystick" + std::to_string(joystickIndex);
                }
                else
                {
                    deviceName += "_" + Ogre::StringConverter::toString(joystickIndex);
                }

                joystick->setEventCallback(this);

                this->joyStickConfig.max = joystick->MAX_AXIS - 4000;
                this->joyStickConfig.deadZone = static_cast<int>(this->joyStickConfig.max * 0.1f);
                this->joyStickConfig.yaw = 0;
                this->joyStickConfig.pitch = 0;
                this->joyStickConfig.swivel = 0;
                joystick->setVector3Sensitivity(0.001f);

                this->addDevice(deviceName, false, joystick);
            }

            this->updateSoftCompanion();
        }
    }

    void InputDeviceCore::capture(Ogre::Real dt)
    {
        // Need to capture / update each device every frame
        if (this->mouse)
        {
            this->mouse->capture();
        }

        if (this->keyboard)
        {
            this->keyboard->capture();
        }

        for (size_t i = 0; i < this->additionalKeyboards.size(); i++)
        {
            this->additionalKeyboards[i]->capture();
        }

        // BUGFIX: Previously only ONE joystick was captured per frame (round robin), but all were updated.
        // With 4 gamepads each pad was only read every 4th frame, which caused laggy / missed inputs in splitscreen.
        for (size_t i = 0; i < this->joysticks.size(); i++)
        {
            this->joysticks[i]->capture();
        }

        for (size_t i = 0; i < this->joystickInputDeviceModules.size(); i++)
        {
            this->joystickInputDeviceModules[i]->update(dt);
        }
    }

    void InputDeviceCore::addKeyListener(OIS::KeyListener* keyListener, const Ogre::String& instanceName)
    {
        if (nullptr == this->keyboard)
        {
            return;
        }

        auto it = this->keyListenerIndex.find(instanceName);
        if (it != this->keyListenerIndex.end())
        {
            const size_t idx = it->second;
            this->keyListenerStack[idx].second = keyListener;
            return;
        }

        this->keyListenerStack.emplace_back(instanceName, keyListener);
        this->keyListenerIndex[instanceName] = this->keyListenerStack.size() - 1;
    }

    void InputDeviceCore::addMouseListener(OIS::MouseListener* mouseListener, const Ogre::String& instanceName)
    {
        if (nullptr == this->mouse)
        {
            return;
        }

        auto it = this->mouseListenerIndex.find(instanceName);
        if (it != this->mouseListenerIndex.end())
        {
            const size_t idx = it->second;
            this->mouseListenerStack[idx].second = mouseListener;
            return;
        }

        this->mouseListenerStack.emplace_back(instanceName, mouseListener);
        this->mouseListenerIndex[instanceName] = this->mouseListenerStack.size() - 1;
    }

    void InputDeviceCore::addJoystickListener(OIS::JoyStickListener* joystickListener, const Ogre::String& instanceName)
    {
        if (true == this->joysticks.empty())
        {
            return;
        }

        auto it = this->joystickListenerIndex.find(instanceName);
        if (it != this->joystickListenerIndex.end())
        {
            const size_t idx = it->second;
            this->joystickListenerStack[idx].second = joystickListener;
            return;
        }

        this->joystickListenerStack.emplace_back(instanceName, joystickListener);
        this->joystickListenerIndex[instanceName] = this->joystickListenerStack.size() - 1;
    }

    void InputDeviceCore::removeKeyListener(const Ogre::String& instanceName)
    {
        if (this->keyDispatchDepth > 0)
        {
            this->pendingRemoveKeys.emplace_back(instanceName);
            return;
        }

        auto it = this->keyListenerIndex.find(instanceName);
        if (it == this->keyListenerIndex.end())
        {
            return;
        }

        const size_t idx = it->second;
        const size_t lastIdx = this->keyListenerStack.size() - 1;

        if (idx != lastIdx)
        {
            this->keyListenerStack[idx] = this->keyListenerStack[lastIdx];
            this->keyListenerIndex[this->keyListenerStack[idx].first] = idx;
        }

        this->keyListenerStack.pop_back();
        this->keyListenerIndex.erase(it);
    }

    void InputDeviceCore::removeMouseListener(const Ogre::String& instanceName)
    {
        if (this->mouseDispatchDepth > 0)
        {
            this->pendingRemoveMice.emplace_back(instanceName);
            return;
        }

        auto it = this->mouseListenerIndex.find(instanceName);
        if (it == this->mouseListenerIndex.end())
        {
            return;
        }

        const size_t idx = it->second;
        const size_t lastIdx = this->mouseListenerStack.size() - 1;

        if (idx != lastIdx)
        {
            this->mouseListenerStack[idx] = this->mouseListenerStack[lastIdx];
            this->mouseListenerIndex[this->mouseListenerStack[idx].first] = idx;
        }

        this->mouseListenerStack.pop_back();
        this->mouseListenerIndex.erase(it);
    }

    void InputDeviceCore::removeJoystickListener(const Ogre::String& instanceName)
    {
        if (this->joystickDispatchDepth > 0)
        {
            this->pendingRemoveJoysticks.emplace_back(instanceName);
            return;
        }

        auto it = this->joystickListenerIndex.find(instanceName);
        if (it == this->joystickListenerIndex.end())
        {
            return;
        }

        const size_t idx = it->second;
        const size_t lastIdx = this->joystickListenerStack.size() - 1;

        if (idx != lastIdx)
        {
            this->joystickListenerStack[idx] = this->joystickListenerStack[lastIdx];
            this->joystickListenerIndex[this->joystickListenerStack[idx].first] = idx;
        }

        this->joystickListenerStack.pop_back();
        this->joystickListenerIndex.erase(it);
    }

    void InputDeviceCore::removeKeyListener(OIS::KeyListener* keyListener)
    {
        for (size_t i = 0; i < this->keyListenerStack.size(); i++)
        {
            if (this->keyListenerStack[i].second == keyListener)
            {
                this->removeKeyListener(this->keyListenerStack[i].first);
                break;
            }
        }
    }

    void InputDeviceCore::removeMouseListener(OIS::MouseListener* mouseListener)
    {
        for (size_t i = 0; i < this->mouseListenerStack.size(); i++)
        {
            if (this->mouseListenerStack[i].second == mouseListener)
            {
                this->removeMouseListener(this->mouseListenerStack[i].first);
                break;
            }
        }
    }

    void InputDeviceCore::removeJoystickListener(OIS::JoyStickListener* joystickListener)
    {
        for (size_t i = 0; i < this->joystickListenerStack.size(); i++)
        {
            if (this->joystickListenerStack[i].second == joystickListener)
            {
                this->removeJoystickListener(this->joystickListenerStack[i].first);
                break;
            }
        }
    }

    void InputDeviceCore::removeAllListeners(void)
    {
        this->keyListenerStack.clear();
        this->mouseListenerStack.clear();
        this->joystickListenerStack.clear();

        this->keyListenerIndex.clear();
        this->mouseListenerIndex.clear();
        this->joystickListenerIndex.clear();

        this->pendingRemoveKeys.clear();
        this->pendingRemoveMice.clear();
        this->pendingRemoveJoysticks.clear();
    }

    void InputDeviceCore::removeAllKeyListeners(void)
    {
        this->keyListenerStack.clear();
        this->keyListenerIndex.clear();
        this->pendingRemoveKeys.clear();
    }

    void InputDeviceCore::removeAllMouseListeners(void)
    {
        this->mouseListenerStack.clear();
        this->mouseListenerIndex.clear();
        this->pendingRemoveMice.clear();
    }

    void InputDeviceCore::removeAllJoystickListeners(void)
    {
        this->joystickListenerStack.clear();
        this->joystickListenerIndex.clear();
        this->pendingRemoveJoysticks.clear();
    }

    void InputDeviceCore::setWindowExtents(int width, int height)
    {
        // Set mouse region (if window resizes, we should alter this to reflect as well)
        const OIS::MouseState& mouseState = this->mouse->getMouseState();
        mouseState.width = width;
        mouseState.height = height;
    }

    OIS::Mouse* InputDeviceCore::getMouse(void)
    {
        return this->mouse;
    }

    OIS::Keyboard* InputDeviceCore::getKeyboard(void)
    {
        return this->keyboard;
    }

    OIS::JoyStick* InputDeviceCore::getJoystick(unsigned int index)
    {
        if (false == this->joysticks.empty())
        {
            return this->joysticks[index % this->joysticks.size()];
        }
        return nullptr;
    }

    bool InputDeviceCore::keyPressed(const OIS::KeyEvent& e)
    {
        OIS::KeyEvent tempKeyEvent = e;
        // Somehow ois sents text always 0 when numpad is key is pressed, so remap for my gui
        switch (e.key)
        {
        case OIS::KC_NUMPAD0:
            tempKeyEvent.text = '0';
            break;
        case OIS::KC_NUMPAD1:
            tempKeyEvent.text = '1';
            break;
        case OIS::KC_NUMPAD2:
            tempKeyEvent.text = '2';
            break;
        case OIS::KC_NUMPAD3:
            tempKeyEvent.text = '3';
            break;
        case OIS::KC_NUMPAD4:
            tempKeyEvent.text = '4';
            break;
        case OIS::KC_NUMPAD5:
            tempKeyEvent.text = '5';
            break;
        case OIS::KC_NUMPAD6:
            tempKeyEvent.text = '6';
            break;
        case OIS::KC_NUMPAD7:
            tempKeyEvent.text = '7';
            break;
        case OIS::KC_NUMPAD8:
            tempKeyEvent.text = '8';
            break;
        case OIS::KC_NUMPAD9:
            tempKeyEvent.text = '9';
            break;
        }

        // Keyboard input: button prompts should show keys again
        for (const auto& module : this->keyboardInputDeviceModules)
        {
            module->setLastInputFromJoyStick(false);
        }

        if (NOWA_K_SELECT == tempKeyEvent.key)
        {
            this->bSelectDown = true;
        }

        MyGUI::Char finalChar = applyModifiers(static_cast<MyGUI::Char>(tempKeyEvent.text), this->keyboard);

        MyGUI::InputManager::getInstancePtr()->injectKeyPress(MyGUI::KeyCode::Enum(tempKeyEvent.key), finalChar);

        //// Do not react on input if there is any interaction with a mygui widget
        // MyGUI::Widget* widget = MyGUI::InputManager::getInstance().getMouseFocusWidget();
        // if (nullptr != widget)
        //{
        //	return false;
        // }

        this->keyDispatchDepth++;

        for (size_t i = this->keyListenerStack.size(); i-- > 0;)
        {
            OIS::KeyListener* listener = this->keyListenerStack[i].second;
            if (nullptr == listener)
            {
                continue;
            }

            if (!listener->keyPressed(e))
            {
                this->keyDispatchDepth--;
                if (0 == this->keyDispatchDepth)
                {
                    this->flushPendingKeyRemovals();
                }
                return true;
            }
        }

        this->keyDispatchDepth--;
        if (0 == this->keyDispatchDepth)
        {
            this->flushPendingKeyRemovals();
        }

        return true;
    }

    bool InputDeviceCore::keyReleased(const OIS::KeyEvent& e)
    {
        if (NOWA_K_SELECT == e.key)
        {
            this->bSelectDown = false;
        }

        MyGUI::InputManager::getInstancePtr()->injectKeyRelease(MyGUI::KeyCode::Enum(e.key));

        // Do not react on input if there is any interaction with a mygui widget
        /*MyGUI::Widget* widget = MyGUI::InputManager::getInstance().getMouseFocusWidget();
        if (nullptr != widget)
        {
            return false;
        }*/

        this->keyDispatchDepth++;

        for (size_t i = this->keyListenerStack.size(); i-- > 0;)
        {
            OIS::KeyListener* listener = this->keyListenerStack[i].second;
            if (nullptr == listener)
            {
                continue;
            }

            if (!listener->keyReleased(e))
            {
                this->keyDispatchDepth--;
                if (0 == this->keyDispatchDepth)
                {
                    this->flushPendingKeyRemovals();
                }
                return true;
            }
        }

        this->keyDispatchDepth--;
        if (0 == this->keyDispatchDepth)
        {
            this->flushPendingKeyRemovals();
        }

        return true;
    }

    bool InputDeviceCore::mouseMoved(const OIS::MouseEvent& e)
    {
        const OIS::MouseState& state = e.state;

        if (MyGUI::InputManager::getInstancePtr())
        {
            MyGUI::InputManager::getInstancePtr()->injectMouseMove(state.X.abs, state.Y.abs, state.Z.abs);
        }

        // Dispatch to the registered mouse listeners
        this->mouseDispatchDepth++;

        for (size_t i = this->mouseListenerStack.size(); i-- > 0;)
        {
            OIS::MouseListener* listener = this->mouseListenerStack[i].second;

            if (nullptr == listener)
            {
                continue;
            }

            if (!listener->mouseMoved(e))
            {
                break;
            }
        }

        this->mouseDispatchDepth--;

        if (0 == this->mouseDispatchDepth)
        {
            this->flushPendingMouseRemovals();
        }

        return true;
    }

    bool InputDeviceCore::mousePressed(const OIS::MouseEvent& e, OIS::MouseButtonID id)
    {
        if (nullptr != this->keyboard)
        {
            this->bSelectDown = this->keyboard->isKeyDown(NOWA_K_SELECT);
        }

        int mX = e.state.X.abs;
        int mY = e.state.Y.abs;

        if (auto* inputMgr = MyGUI::InputManager::getInstancePtr())
        {
            inputMgr->injectMousePress(mX, mY, MyGUI::MouseButton::Enum(id));
        }

        this->mouseDispatchDepth++;

        for (size_t i = this->mouseListenerStack.size(); i-- > 0;)
        {
            OIS::MouseListener* listener = this->mouseListenerStack[i].second;
            if (nullptr == listener)
            {
                continue;
            }

            if (!listener->mousePressed(e, id))
            {
                this->mouseDispatchDepth--;
                if (0 == this->mouseDispatchDepth)
                {
                    this->flushPendingMouseRemovals();
                }
                return true;
            }
        }

        this->mouseDispatchDepth--;
        if (0 == this->mouseDispatchDepth)
        {
            this->flushPendingMouseRemovals();
        }

        return true;
    }

    bool InputDeviceCore::mouseReleased(const OIS::MouseEvent& e, OIS::MouseButtonID id)
    {
        int mX = e.state.X.abs;
        int mY = e.state.Y.abs;

        if (auto* inputMgr = MyGUI::InputManager::getInstancePtr())
        {
            inputMgr->injectMouseRelease(mX, mY, MyGUI::MouseButton::Enum(id));
        }

        this->mouseDispatchDepth++;

        for (size_t i = this->mouseListenerStack.size(); i-- > 0;)
        {
            OIS::MouseListener* listener = this->mouseListenerStack[i].second;
            if (nullptr == listener)
            {
                continue;
            }

            if (!listener->mouseReleased(e, id))
            {
                this->mouseDispatchDepth--;
                if (0 == this->mouseDispatchDepth)
                {
                    this->flushPendingMouseRemovals();
                }
                return true;
            }
        }

        this->mouseDispatchDepth--;
        if (0 == this->mouseDispatchDepth)
        {
            this->flushPendingMouseRemovals();
        }

        return true;
    }

    bool InputDeviceCore::povMoved(const OIS::JoyStickEvent& e, int pov)
    {
        if (OIS::Pov::Centered != e.state.mPOV[pov].direction)
        {
            this->markJoyStickInput(e.device);
        }

        this->joystickDispatchDepth++;

        for (size_t i = this->joystickListenerStack.size(); i-- > 0;)
        {
            OIS::JoyStickListener* listener = this->joystickListenerStack[i].second;
            if (nullptr == listener)
            {
                continue;
            }

            if (!listener->povMoved(e, pov))
            {
                this->joystickDispatchDepth--;
                if (0 == this->joystickDispatchDepth)
                {
                    this->flushPendingJoystickRemovals();
                }
                return true;
            }
        }

        this->joystickDispatchDepth--;
        if (0 == this->joystickDispatchDepth)
        {
            this->flushPendingJoystickRemovals();
        }

        return true;
    }

    bool InputDeviceCore::axisMoved(const OIS::JoyStickEvent& e, int axis)
    {
        InputDeviceModule* axisModule = this->getInputDeviceModuleFromDeviceObject(e.device);
        if (nullptr != axisModule && axis >= 0 && axis < static_cast<int>(e.state.mAxes.size()))
        {
            if (InputDeviceModule::BUTTON_NONE != axisModule->translateRawAxis(axis, e.state.mAxes[axis].abs))
            {
                this->markJoyStickInput(e.device);
            }
        }

        this->joystickDispatchDepth++;

        for (size_t i = this->joystickListenerStack.size(); i-- > 0;)
        {
            OIS::JoyStickListener* listener = this->joystickListenerStack[i].second;
            if (nullptr == listener)
            {
                continue;
            }

            if (!listener->axisMoved(e, axis))
            {
                this->joystickDispatchDepth--;
                if (0 == this->joystickDispatchDepth)
                {
                    this->flushPendingJoystickRemovals();
                }
                return true;
            }
        }

        this->joystickDispatchDepth--;
        if (0 == this->joystickDispatchDepth)
        {
            this->flushPendingJoystickRemovals();
        }

        return true;
    }

    bool InputDeviceCore::sliderMoved(const OIS::JoyStickEvent& e, int sliderID)
    {
        this->joystickDispatchDepth++;

        for (size_t i = this->joystickListenerStack.size(); i-- > 0;)
        {
            OIS::JoyStickListener* listener = this->joystickListenerStack[i].second;
            if (nullptr == listener)
            {
                continue;
            }

            if (!listener->sliderMoved(e, sliderID))
            {
                this->joystickDispatchDepth--;
                if (0 == this->joystickDispatchDepth)
                {
                    this->flushPendingJoystickRemovals();
                }
                return true;
            }
        }

        this->joystickDispatchDepth--;
        if (0 == this->joystickDispatchDepth)
        {
            this->flushPendingJoystickRemovals();
        }

        return true;
    }

    bool InputDeviceCore::buttonPressed(const OIS::JoyStickEvent& e, int button)
    {
        this->markJoyStickInput(e.device);

        // Throttled diagnostic: shows how raw OIS button indices are translated, to verify the detected layout of a new controller
        InputDeviceModule* buttonModule = this->getInputDeviceModuleFromDeviceObject(e.device);
        if (nullptr != buttonModule)
        {
            this->diagnosticRawButtonLogCount++;
            if (this->diagnosticRawButtonLogCount <= 32 || 0 == this->diagnosticRawButtonLogCount % 50)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[InputDeviceCore] Diagnostic: '" + buttonModule->getDeviceName() + "' raw button " + Ogre::StringConverter::toString(button) + " -> '" +
                                                                                      buttonModule->getStringFromMappedButton(buttonModule->translateRawButton(button)) +
                                                                                      "' (layout: " + InputDeviceModule::getJoyStickLayoutName(buttonModule->getJoyStickLayout()) + ")");
            }
        }

        this->joystickDispatchDepth++;

        for (size_t i = this->joystickListenerStack.size(); i-- > 0;)
        {
            OIS::JoyStickListener* listener = this->joystickListenerStack[i].second;
            if (nullptr == listener)
            {
                continue;
            }

            if (!listener->buttonPressed(e, button))
            {
                this->joystickDispatchDepth--;
                if (0 == this->joystickDispatchDepth)
                {
                    this->flushPendingJoystickRemovals();
                }
                return true;
            }
        }

        this->joystickDispatchDepth--;
        if (0 == this->joystickDispatchDepth)
        {
            this->flushPendingJoystickRemovals();
        }

        return true;
    }

    bool InputDeviceCore::buttonReleased(const OIS::JoyStickEvent& e, int button)
    {
        this->joystickDispatchDepth++;

        for (size_t i = this->joystickListenerStack.size(); i-- > 0;)
        {
            OIS::JoyStickListener* listener = this->joystickListenerStack[i].second;
            if (nullptr == listener)
            {
                continue;
            }

            if (!listener->buttonReleased(e, button))
            {
                this->joystickDispatchDepth--;
                if (0 == this->joystickDispatchDepth)
                {
                    this->flushPendingJoystickRemovals();
                }
                return true;
            }
        }

        this->joystickDispatchDepth--;
        if (0 == this->joystickDispatchDepth)
        {
            this->flushPendingJoystickRemovals();
        }

        return true;
    }

    void InputDeviceCore::flushPendingKeyRemovals(void)
    {
        for (size_t i = 0; i < this->pendingRemoveKeys.size(); i++)
        {
            this->removeKeyListener(this->pendingRemoveKeys[i]);
        }
        this->pendingRemoveKeys.clear();
    }

    void InputDeviceCore::flushPendingMouseRemovals(void)
    {
        for (size_t i = 0; i < this->pendingRemoveMice.size(); i++)
        {
            this->removeMouseListener(this->pendingRemoveMice[i]);
        }
        this->pendingRemoveMice.clear();
    }

    void InputDeviceCore::flushPendingJoystickRemovals(void)
    {
        for (size_t i = 0; i < this->pendingRemoveJoysticks.size(); i++)
        {
            this->removeJoystickListener(this->pendingRemoveJoysticks[i]);
        }
        this->pendingRemoveJoysticks.clear();
    }

    void InputDeviceCore::addDevice(const Ogre::String& deviceName, bool isKeyboard, OIS::Object* deviceObject)
    {
        if (true == isKeyboard)
        {
            InputDeviceModule* keyboardModule = new InputDeviceModule(deviceName, isKeyboard, deviceObject);
            this->keyboardInputDeviceModules.push_back(keyboardModule);

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[InputDeviceCore] Keyboard device added: '" + deviceName + "' (logical name: '" + InputDeviceCore_DeviceKeyboard + "')");
        }
        else
        {
            InputDeviceModule* joystickModule = new InputDeviceModule(deviceName, isKeyboard, deviceObject);
            this->joystickInputDeviceModules.push_back(joystickModule);

            this->applyJoyStickLayout(joystickModule);

            // Gamepads share the gamepad button profile, which is stored in the main keyboard module (see remapGamepadButton)
            if (nullptr != this->mainInputDeviceModule)
            {
                for (unsigned short i = 0; i < InputDeviceModule::ACTION_MAPPING_COUNT; i++)
                {
                    const InputDeviceModule::Action action = static_cast<InputDeviceModule::Action>(i);
                    joystickModule->remapButton(action, this->mainInputDeviceModule->getMappedButton(action));
                }
            }

            OIS::JoyStick* joyStick = static_cast<OIS::JoyStick*>(deviceObject);
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL,
                "[InputDeviceCore] Gamepad device added: '" + deviceName + "' (logical name: '" + InputDeviceCore_DeviceGamepadPrefix + Ogre::StringConverter::toString(this->joystickInputDeviceModules.size()) +
                    "'), buttons: " + Ogre::StringConverter::toString(joyStick->getNumberOfComponents(OIS::OIS_Button)) + ", axes: " + Ogre::StringConverter::toString(joyStick->getNumberOfComponents(OIS::OIS_Axis)) +
                    ", povs: " + Ogre::StringConverter::toString(joyStick->getNumberOfComponents(OIS::OIS_POV)) + ", layout: " + InputDeviceModule::getJoyStickLayoutName(joystickModule->getJoyStickLayout()));
        }
    }

    void InputDeviceCore::applyJoyStickLayout(InputDeviceModule* joystickModule)
    {
        if (nullptr == joystickModule || true == joystickModule->isKeyboardDevice())
        {
            return;
        }

        if ("Auto" == this->joyStickLayoutOverride || true == this->joyStickLayoutOverride.empty())
        {
            joystickModule->setJoyStickLayout(InputDeviceModule::detectJoyStickLayout(static_cast<OIS::JoyStick*>(joystickModule->getDeviceObject())));
        }
        else
        {
            joystickModule->setJoyStickLayout(InputDeviceModule::getJoyStickLayoutFromName(this->joyStickLayoutOverride));
        }
    }

    void InputDeviceCore::setJoyStickLayoutOverride(const Ogre::String& layoutName)
    {
        this->joyStickLayoutOverride = layoutName;
        if (true == this->joyStickLayoutOverride.empty())
        {
            this->joyStickLayoutOverride = "Auto";
        }

        for (size_t i = 0; i < this->joystickInputDeviceModules.size(); i++)
        {
            this->applyJoyStickLayout(this->joystickInputDeviceModules[i]);
        }
    }

    const Ogre::String& InputDeviceCore::getJoyStickLayoutOverride(void) const
    {
        return this->joyStickLayoutOverride;
    }

    unsigned short InputDeviceCore::getJoyStickCount(void) const
    {
        return static_cast<unsigned short>(this->joysticks.size());
    };

    std::vector<OIS::JoyStick*> InputDeviceCore::getJoySticks(void) const
    {
        return this->joysticks;
    }

    std::vector<Ogre::String> InputDeviceCore::getLogicalDeviceNames(void) const
    {
        std::vector<Ogre::String> deviceNames;
        deviceNames.push_back(InputDeviceCore_DeviceAuto);
        deviceNames.push_back(InputDeviceCore_DeviceJoin);
        deviceNames.push_back(InputDeviceCore_DeviceKeyboard);

        // Always offer at least 4 gamepads, so that splitscreen scenes can be authored without all pads being plugged in
        size_t gamepadCount = this->joystickInputDeviceModules.size();
        if (gamepadCount < InputDeviceCore_MinListedGamepads)
        {
            gamepadCount = InputDeviceCore_MinListedGamepads;
        }
        for (size_t i = 0; i < gamepadCount; i++)
        {
            deviceNames.push_back(InputDeviceCore_DeviceGamepadPrefix + Ogre::StringConverter::toString(i + 1));
        }
        return deviceNames;
    }

    Ogre::String InputDeviceCore::getLogicalDeviceName(InputDeviceModule* inputDeviceModule) const
    {
        if (nullptr == inputDeviceModule)
        {
            return Ogre::String();
        }

        if (true == inputDeviceModule->isKeyboardDevice())
        {
            return InputDeviceCore_DeviceKeyboard;
        }

        for (size_t i = 0; i < this->joystickInputDeviceModules.size(); i++)
        {
            if (inputDeviceModule == this->joystickInputDeviceModules[i])
            {
                return InputDeviceCore_DeviceGamepadPrefix + Ogre::StringConverter::toString(i + 1);
            }
        }
        return inputDeviceModule->getDeviceName();
    }

    InputDeviceModule* InputDeviceCore::findModuleByName(const Ogre::String& deviceName) const
    {
        if (InputDeviceCore_DeviceKeyboard == deviceName)
        {
            if (false == this->keyboardInputDeviceModules.empty())
            {
                return this->keyboardInputDeviceModules[0];
            }
            return nullptr;
        }

        if (0 == deviceName.find(InputDeviceCore_DeviceGamepadPrefix))
        {
            const int gamepadNumber = Ogre::StringConverter::parseInt(deviceName.substr(InputDeviceCore_DeviceGamepadPrefix.size()), 0);
            if (gamepadNumber >= 1 && gamepadNumber <= static_cast<int>(this->joystickInputDeviceModules.size()))
            {
                return this->joystickInputDeviceModules[gamepadNumber - 1];
            }
            return nullptr;
        }

        // Physical vendor name (legacy)
        for (const auto& module : this->keyboardInputDeviceModules)
        {
            if (module->getDeviceName() == deviceName)
            {
                return module;
            }
        }
        for (const auto& module : this->joystickInputDeviceModules)
        {
            if (module->getDeviceName() == deviceName)
            {
                return module;
            }
        }
        return nullptr;
    }

    InputDeviceModule* InputDeviceCore::getPrimaryModuleOf(unsigned long id) const
    {
        if (0 == id)
        {
            return nullptr;
        }

        for (const auto& module : this->keyboardInputDeviceModules)
        {
            if (id == module->getOccupiedId())
            {
                return module;
            }
        }
        for (const auto& module : this->joystickInputDeviceModules)
        {
            // A hard companion belongs to its keyboard owner, it is not the primary module
            if (id == module->getOccupiedId() && nullptr == module->getCompanionOwner())
            {
                return module;
            }
        }
        return nullptr;
    }

    void InputDeviceCore::detachFromCompanionOwner(InputDeviceModule* module)
    {
        if (nullptr != module && nullptr != module->getCompanionOwner())
        {
            module->getCompanionOwner()->setCompanionModule(nullptr, false);
        }
    }

    InputDeviceModule* InputDeviceCore::occupyModule(InputDeviceModule* module, unsigned long id)
    {
        const unsigned long otherId = module->getOccupiedId();

        if (0 != otherId && otherId != id)
        {
            InputDeviceModule* owner = module->getCompanionOwner();
            if (nullptr != owner && false == owner->isCompanionSoft())
            {
                // The module is only the companion gamepad of another "Auto" player: Take just the gamepad, the other player keeps the keyboard
                owner->setCompanionModule(nullptr, false);
                module->releaseOccupation();
            }
            else
            {
                // The module is the primary device of another game object: Evict that game object completely.
                // Its InputDeviceComponent notices this via EventDataInputDeviceOccupied and deactivates itself.
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
                    "[InputDeviceCore] Device '" + module->getDeviceName() + "' taken over from game object id: " + Ogre::StringConverter::toString(otherId) + " by game object id: " + Ogre::StringConverter::toString(id));
                this->releaseDeviceInternal(otherId);
            }
        }

        // A free gamepad may still be the soft (menu) companion of the unoccupied main keyboard
        this->detachFromCompanionOwner(module);

        // A keyboard which is occupied explicitly, must not keep the soft (menu) companion
        if (true == module->isKeyboardDevice() && nullptr != module->getCompanionModule() && true == module->isCompanionSoft())
        {
            module->setCompanionModule(nullptr, false);
        }

        module->setOccupiedId(id);
        return module;
    }

    InputDeviceModule* InputDeviceCore::getFirstFreeJoystickModule(void) const
    {
        for (const auto& module : this->joystickInputDeviceModules)
        {
            if (false == module->isOccupied())
            {
                return module;
            }
        }
        return nullptr;
    }

    InputDeviceModule* InputDeviceCore::assignAutoDevice(unsigned long id)
    {
        InputDeviceModule* keyboardModule = nullptr;
        if (false == this->keyboardInputDeviceModules.empty() && nullptr != this->keyboardInputDeviceModules[0]->getDeviceObject())
        {
            keyboardModule = this->keyboardInputDeviceModules[0];
        }

        InputDeviceModule* currentPrimary = this->getPrimaryModuleOf(id);

        if (nullptr != keyboardModule)
        {
            if (currentPrimary != keyboardModule)
            {
                this->releaseDeviceInternal(id);
                this->occupyModule(keyboardModule, id);
            }

            // Attach a gamepad, if not yet done and one is free
            if (nullptr == keyboardModule->getCompanionModule() || true == keyboardModule->isCompanionSoft())
            {
                if (nullptr != keyboardModule->getCompanionModule())
                {
                    keyboardModule->setCompanionModule(nullptr, false);
                }

                InputDeviceModule* joystickModule = this->getFirstFreeJoystickModule();
                if (nullptr != joystickModule)
                {
                    this->detachFromCompanionOwner(joystickModule);
                    joystickModule->setOccupiedId(id);
                    keyboardModule->setCompanionModule(joystickModule, false);
                }
            }
            return keyboardModule;
        }

        // No keyboard: Gamepad only
        if (nullptr != currentPrimary && false == currentPrimary->isKeyboardDevice())
        {
            return currentPrimary;
        }

        this->releaseDeviceInternal(id);

        InputDeviceModule* joystickModule = this->getFirstFreeJoystickModule();
        if (nullptr == joystickModule)
        {
            return nullptr;
        }
        return this->occupyModule(joystickModule, id);
    }

    InputDeviceModule* InputDeviceCore::assignDevice(const Ogre::String& deviceName, unsigned long id)
    {
        if (0 == id)
        {
            return nullptr;
        }

        InputDeviceModule* resultModule = nullptr;

        if (InputDeviceCore_DeviceAuto == deviceName)
        {
            resultModule = this->assignAutoDevice(id);
        }
        else
        {
            InputDeviceModule* module = this->findModuleByName(deviceName);
            if (nullptr != module)
            {
                if (module == this->getPrimaryModuleOf(id))
                {
                    resultModule = module;
                }
                else
                {
                    // Switching device: release everything this game object held before
                    this->releaseDeviceInternal(id);
                    resultModule = this->occupyModule(module, id);
                }
            }
        }

        this->updateSoftCompanion();
        return resultModule;
    }

    void InputDeviceCore::releaseDeviceInternal(unsigned long id)
    {
        if (0 == id)
        {
            return;
        }

        // Note: Previously the loops stopped after the first module, but an "Auto" device occupies keyboard AND gamepad
        for (const auto& module : this->keyboardInputDeviceModules)
        {
            if (id == module->getOccupiedId())
            {
                InputDeviceModule* companionModule = module->getCompanionModule();
                if (nullptr != companionModule && false == module->isCompanionSoft())
                {
                    module->setCompanionModule(nullptr, false);
                    companionModule->releaseOccupation();
                }
                module->releaseOccupation();
            }
        }
        for (const auto& module : this->joystickInputDeviceModules)
        {
            if (id == module->getOccupiedId())
            {
                this->detachFromCompanionOwner(module);
                module->releaseOccupation();
            }
        }
    }

    void InputDeviceCore::releaseDevice(unsigned long id)
    {
        this->releaseDeviceInternal(id);
        this->updateSoftCompanion();
    }

    void InputDeviceCore::updateSoftCompanion(void)
    {
        if (nullptr == this->mainInputDeviceModule)
        {
            return;
        }

        InputDeviceModule* mainModule = this->mainInputDeviceModule;

        if (true == mainModule->isOccupied())
        {
            if (nullptr != mainModule->getCompanionModule() && true == mainModule->isCompanionSoft())
            {
                mainModule->setCompanionModule(nullptr, false);
            }
            return;
        }

        // The unoccupied main keyboard (used by menus and code which works with getMainKeyboardInputDeviceModule()) gets the first free gamepad as soft companion,
        // so that menus and e.g. the map toggle also work with a gamepad (Steam Deck). The gamepad stays free and can be assigned any time.
        InputDeviceModule* companionModule = mainModule->getCompanionModule();
        if (nullptr != companionModule && false == companionModule->isOccupied())
        {
            return;
        }

        InputDeviceModule* joystickModule = this->getFirstFreeJoystickModule();
        if (joystickModule != companionModule)
        {
            if (nullptr != joystickModule)
            {
                this->detachFromCompanionOwner(joystickModule);
            }
            mainModule->setCompanionModule(joystickModule, true);
        }
    }

    InputDeviceModule* InputDeviceCore::findJoinRequestModule(void)
    {
        for (const auto& module : this->keyboardInputDeviceModules)
        {
            if (false == module->isOccupied() && nullptr != module->getDeviceObject() && true == module->isJoinInputDown())
            {
                return module;
            }
        }
        for (const auto& module : this->joystickInputDeviceModules)
        {
            if (false == module->isOccupied() && true == module->isJoinInputDown())
            {
                return module;
            }
        }
        return nullptr;
    }

    bool InputDeviceCore::isActionDownOnAnyDevice(unsigned short action)
    {
        if (true == this->bLock || action >= InputDeviceModule::ACTION_SLOT_COUNT)
        {
            return false;
        }

        const InputDeviceModule::Action inputAction = static_cast<InputDeviceModule::Action>(action);

        for (const auto& module : this->keyboardInputDeviceModules)
        {
            if (true == module->isActionDownOwnDevice(inputAction))
            {
                return true;
            }
        }
        for (const auto& module : this->joystickInputDeviceModules)
        {
            if (true == module->isActionDownOwnDevice(inputAction))
            {
                return true;
            }
        }
        return false;
    }

    void InputDeviceCore::remapGamepadButton(unsigned short action, unsigned short button)
    {
        if (action >= InputDeviceModule::ACTION_MAPPING_COUNT)
        {
            return;
        }

        const InputDeviceModule::Action inputAction = static_cast<InputDeviceModule::Action>(action);
        const InputDeviceModule::JoyStickButton joyStickButton = static_cast<InputDeviceModule::JoyStickButton>(button);

        // The main keyboard module stores the gamepad profile, so it is saved/loaded even if no gamepad is connected
        if (nullptr != this->mainInputDeviceModule)
        {
            this->mainInputDeviceModule->remapButton(inputAction, joyStickButton);
        }

        for (const auto& module : this->joystickInputDeviceModules)
        {
            module->remapButton(inputAction, joyStickButton);
        }
    }

    void InputDeviceCore::applyGamepadButtonProfile(void)
    {
        if (nullptr == this->mainInputDeviceModule)
        {
            return;
        }

        for (const auto& module : this->joystickInputDeviceModules)
        {
            for (unsigned short i = 0; i < InputDeviceModule::ACTION_MAPPING_COUNT; i++)
            {
                const InputDeviceModule::Action action = static_cast<InputDeviceModule::Action>(i);
                module->remapButton(action, this->mainInputDeviceModule->getMappedButton(action));
            }
        }
    }

    void InputDeviceCore::applyKeyboardMappingToAllKeyboards(void)
    {
        if (nullptr == this->mainInputDeviceModule)
        {
            return;
        }

        for (const auto& module : this->keyboardInputDeviceModules)
        {
            if (module == this->mainInputDeviceModule)
            {
                continue;
            }
            for (unsigned short i = 0; i < InputDeviceModule::ACTION_MAPPING_COUNT; i++)
            {
                const InputDeviceModule::Action action = static_cast<InputDeviceModule::Action>(i);
                module->remapKey(action, this->mainInputDeviceModule->getMappedKey(action));
            }
        }
    }

    void InputDeviceCore::lockDevices(bool bLock)
    {
        this->bLock = bLock;
        for (const auto& module : this->keyboardInputDeviceModules)
        {
            module->lockDevice(bLock);
        }
        for (const auto& module : this->joystickInputDeviceModules)
        {
            module->lockDevice(bLock);
        }
    }

    bool InputDeviceCore::areDevicesLocked(void) const
    {
        return this->bLock;
    }

    InputDeviceModule* InputDeviceCore::getMainKeyboardInputDeviceModule(void) const
    {
        return this->mainInputDeviceModule;
    }

    InputDeviceModule* InputDeviceCore::getKeyboardInputDeviceModule(unsigned long id) const
    {
        for (const auto& module : this->keyboardInputDeviceModules)
        {
            if (id == module->getOccupiedId())
            {
                return module;
            }
        }
        return nullptr;
    }

    InputDeviceModule* InputDeviceCore::getJoystickInputDeviceModule(unsigned long id) const
    {
        // Note: Looks up by the OCCUPYING game object id, not by index. Use getJoystickInputDeviceModuleByIndex for the n-th gamepad.
        for (const auto& module : this->joystickInputDeviceModules)
        {
            if (id == module->getOccupiedId())
            {
                return module;
            }
        }
        return nullptr;
    }

    InputDeviceModule* InputDeviceCore::getJoystickInputDeviceModuleByIndex(size_t index) const
    {
        if (index < this->joystickInputDeviceModules.size())
        {
            return this->joystickInputDeviceModules[index];
        }
        return nullptr;
    }

    InputDeviceModule* InputDeviceCore::getInputDeviceModuleFromDeviceObject(const OIS::Object* deviceObject) const
    {
        if (nullptr == deviceObject)
        {
            return nullptr;
        }

        for (const auto& module : this->joystickInputDeviceModules)
        {
            if (deviceObject == module->getDeviceObject())
            {
                return module;
            }
        }
        for (const auto& module : this->keyboardInputDeviceModules)
        {
            if (deviceObject == module->getDeviceObject())
            {
                return module;
            }
        }
        return nullptr;
    }

    std::vector<InputDeviceModule*> InputDeviceCore::getKeyboardInputDeviceModules(void) const
    {
        return this->keyboardInputDeviceModules;
    }

    std::vector<InputDeviceModule*> InputDeviceCore::getJoystickInputDeviceModules(void) const
    {
        return this->joystickInputDeviceModules;
    }

    bool InputDeviceCore::isSelectDown(void) const
    {
        return this->bSelectDown;
    }

    void InputDeviceCore::markJoyStickInput(const OIS::Object* deviceObject)
    {
        InputDeviceModule* module = this->getInputDeviceModuleFromDeviceObject(deviceObject);
        if (nullptr == module)
        {
            return;
        }

        module->setLastInputFromJoyStick(true);
        if (nullptr != module->getCompanionOwner())
        {
            module->getCompanionOwner()->setLastInputFromJoyStick(true);
        }
    }

    void InputDeviceCore::setMousePosition(int x, int y)
    {
        if (nullptr == this->mouse)
        {
            return;
        }

#if defined(_WIN32)
        HWND hwnd = nullptr;
        NOWA::Core::getSingletonPtr()->getOgreRenderWindow()->getCustomAttribute("WINDOW", &hwnd);

        if (nullptr == hwnd)
        {
            return;
        }

        POINT screenPosition{x, y};

        if (!ClientToScreen(hwnd, &screenPosition))
        {
            return;
        }

        SetCursorPos(screenPosition.x, screenPosition.y);
#endif

        // Synchronize the OIS absolute position.
        // Do not use GetCursorPos() in the next event.
        OIS::MouseState& state = const_cast<OIS::MouseState&>(this->mouse->getMouseState());

        state.X.abs = x;
        state.Y.abs = y;

        if (MyGUI::InputManager::getInstancePtr())
        {
            MyGUI::InputManager::getInstancePtr()->injectMouseMove(x, y, 0);
        }
    }

} // namespace end
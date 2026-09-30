#ifndef INPUT_DEVICE_CORE_H
#define INPUT_DEVICE_CORE_H

#include "defines.h"
#include <atomic>
#include <map>

#include <OISInputManager.h>
#include <OISJoyStick.h>
#include <OISKeyboard.h>
#include <OISMouse.h>

#include <OgreWindow.h>

#include "MyGUI_InputManager.h"

namespace NOWA
{
    struct JoyStickConfig
    {
        int deadZone;
        int max;

        int yaw;
        int pitch;

        int swivel;
        //	int swivelL;
    };

    class InputDeviceModule;

    class EXPORTED InputDeviceCore : public Ogre::Singleton<InputDeviceCore>, public OIS::KeyListener, public OIS::MouseListener, public OIS::JoyStickListener
    {
    public:
        friend class Core;

        /**
         * @brief		Gets access to the singleton instance via reference
         * @note			Since this library gets exported, this two methods must be overwritten, in order to export this library as singleton too
         */
        static InputDeviceCore& getSingleton(void);

        /**
         * @brief		Gets access to the singleton instance via pointer
         * @note			Since this library gets exported, this two methods must be overwritten, in order to export this library as singleton too
         */
        static InputDeviceCore* getSingletonPtr(void);

        void destroyContent(void);

        void initialise(Ogre::Window* renderWindow);
        void capture(Ogre::Real dt);

        void addKeyListener(OIS::KeyListener* keyListener, const Ogre::String& instanceName);
        void addMouseListener(OIS::MouseListener* mouseListener, const Ogre::String& instanceName);
        void addJoystickListener(OIS::JoyStickListener* joystickListener, const Ogre::String& instanceName);

        void removeKeyListener(const Ogre::String& instanceName);
        void removeMouseListener(const Ogre::String& instanceName);
        void removeJoystickListener(const Ogre::String& instanceName);

        void removeKeyListener(OIS::KeyListener* keyListener);
        void removeMouseListener(OIS::MouseListener* mouseListener);
        void removeJoystickListener(OIS::JoyStickListener* joystickListener);

        void removeAllListeners(void);
        void removeAllKeyListeners(void);
        void removeAllMouseListeners(void);
        void removeAllJoystickListeners(void);

        void setWindowExtents(int width, int height);

        OIS::Mouse* getMouse(void);
        OIS::Keyboard* getKeyboard(void);
        OIS::JoyStick* getJoystick(unsigned int index = 0);

        unsigned short getJoyStickCount(void) const;

        std::vector<OIS::JoyStick*> getJoySticks(void) const;

        InputDeviceModule* assignDevice(const Ogre::String& deviceName, unsigned long id);

        void releaseDevice(unsigned long id);

        void lockDevices(bool bLock);

        bool areDevicesLocked(void) const;

        InputDeviceModule* getMainKeyboardInputDeviceModule(void) const;

        InputDeviceModule* getKeyboardInputDeviceModule(unsigned long id) const;

        InputDeviceModule* getJoystickInputDeviceModule(unsigned long id) const;

        std::vector<InputDeviceModule*> getKeyboardInputDeviceModules(void) const;

        std::vector<InputDeviceModule*> getJoystickInputDeviceModules(void) const;

        bool isSelectDown(void) const;

        void setMousePosition(int x, int y);

        /**
         * @brief		Gets the logical device names: "Auto", "Join", "Keyboard", "Gamepad 1" ... "Gamepad N" (at least 4 gamepads are listed).
         */
        std::vector<Ogre::String> getLogicalDeviceNames(void) const;

        /**
         * @brief		Gets the logical name ("Keyboard" or "Gamepad N") of the given module.
         */
        Ogre::String getLogicalDeviceName(InputDeviceModule* inputDeviceModule) const;

        /**
         * @brief		Finds a module by logical name ("Keyboard", "Gamepad N") or physical OIS vendor name. "Auto" and "Join" are not resolved here.
         */
        InputDeviceModule* findModuleByName(const Ogre::String& deviceName) const;

        /**
         * @brief		Gets the n-th gamepad module (0-based), independent of occupation.
         */
        InputDeviceModule* getJoystickInputDeviceModuleByIndex(size_t index) const;

        /**
         * @brief		Gets the module for an OIS device (e.g. OIS::JoyStickEvent::device), so that listeners know which gamepad fired an event.
         */
        InputDeviceModule* getInputDeviceModuleFromDeviceObject(const OIS::Object* deviceObject) const;

        /**
         * @brief		Gets a free device (keyboard or gamepad), on which currently the mapped JUMP or START is pressed, or null. Used for "Join" (splitscreen).
         */
        InputDeviceModule* findJoinRequestModule(void);

        /**
         * @brief		Gets whether the action is down on ANY device (all keyboards and gamepads, occupied or not). Useful for menus.
         * @param[in]	action	InputDeviceModule::Action as number.
         */
        bool isActionDownOnAnyDevice(unsigned short action);

        /**
         * @brief		Remaps a gamepad button in the gamepad profile (stored in the main keyboard module, so it is saved even without connected gamepad) and applies it to all gamepads.
         * @param[in]	action	InputDeviceModule::Action as number.
         * @param[in]	button	InputDeviceModule::JoyStickButton as number.
         */
        void remapGamepadButton(unsigned short action, unsigned short button);

        /**
         * @brief		Copies the gamepad profile of the main keyboard module to all gamepad modules (after loading the configuration).
         */
        void applyGamepadButtonProfile(void);

        /**
         * @brief		Copies the key mapping of the main keyboard module to all other keyboard modules.
         */
        void applyKeyboardMappingToAllKeyboards(void);

        /**
         * @brief		Sets the raw gamepad layout: "Auto" (detected per gamepad), "Generic", "XInput" or "LinuxEvdev".
         */
        void setJoyStickLayoutOverride(const Ogre::String& layoutName);

        const Ogre::String& getJoyStickLayoutOverride(void) const;
    private:
        InputDeviceCore();

        ~InputDeviceCore();

        bool keyPressed(const OIS::KeyEvent& e);
        bool keyReleased(const OIS::KeyEvent& e);

        bool mouseMoved(const OIS::MouseEvent& e);
        bool mousePressed(const OIS::MouseEvent& e, OIS::MouseButtonID id);
        bool mouseReleased(const OIS::MouseEvent& e, OIS::MouseButtonID id);

        bool povMoved(const OIS::JoyStickEvent& e, int pov);
        bool axisMoved(const OIS::JoyStickEvent& e, int axis);
        bool sliderMoved(const OIS::JoyStickEvent& e, int sliderID);
        bool buttonPressed(const OIS::JoyStickEvent& e, int button);
        bool buttonReleased(const OIS::JoyStickEvent& e, int button);

        void flushPendingKeyRemovals(void);

        void flushPendingMouseRemovals(void);

        void flushPendingJoystickRemovals(void);

        void addDevice(const Ogre::String& deviceName, bool isKeyboard, OIS::Object* deviceObject);

        void applyJoyStickLayout(InputDeviceModule* joystickModule);

        InputDeviceModule* getPrimaryModuleOf(unsigned long id) const;

        void detachFromCompanionOwner(InputDeviceModule* module);

        InputDeviceModule* occupyModule(InputDeviceModule* module, unsigned long id);

        InputDeviceModule* getFirstFreeJoystickModule(void) const;

        InputDeviceModule* assignAutoDevice(unsigned long id);

        void releaseDeviceInternal(unsigned long id);

        void updateSoftCompanion(void);

        void markJoyStickInput(const OIS::Object* deviceObject);
    private:
        OIS::Mouse* mouse;
        OIS::Keyboard* keyboard;
        OIS::InputManager* inputSystem;

        std::vector<OIS::JoyStick*> joysticks;

        InputDeviceModule* mainInputDeviceModule;
        std::vector<InputDeviceModule*> keyboardInputDeviceModules;
        std::vector<InputDeviceModule*> joystickInputDeviceModules;

        // Ordered stacks (deepest last -> processed first)
        std::vector<std::pair<Ogre::String, OIS::KeyListener*>> keyListenerStack;
        std::vector<std::pair<Ogre::String, OIS::MouseListener*>> mouseListenerStack;
        std::vector<std::pair<Ogre::String, OIS::JoyStickListener*>> joystickListenerStack;

        // Name -> index for O(1) remove
        std::unordered_map<Ogre::String, size_t> keyListenerIndex;
        std::unordered_map<Ogre::String, size_t> mouseListenerIndex;
        std::unordered_map<Ogre::String, size_t> joystickListenerIndex;

        // Safe removal while dispatching
        int keyDispatchDepth;
        int mouseDispatchDepth;
        int joystickDispatchDepth;

        std::vector<Ogre::String> pendingRemoveKeys;
        std::vector<Ogre::String> pendingRemoveMice;
        std::vector<Ogre::String> pendingRemoveJoysticks;

        JoyStickConfig joyStickConfig;

        bool bSelectDown;
        bool bLock;

        std::vector<OIS::Keyboard*> additionalKeyboards;
        Ogre::String joyStickLayoutOverride;
        unsigned int diagnosticRawButtonLogCount;
    };

}; // namespace end

template <> NOWA::InputDeviceCore* Ogre::Singleton<NOWA::InputDeviceCore>::msSingleton = 0;

#endif
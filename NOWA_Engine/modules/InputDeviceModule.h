#ifndef INPUT_DEVICE_MODULE_H
#define INPUT_DEVICE_MODULE_H

#include <map>
#include "defines.h"
#include "main/InputDeviceCore.h"

namespace NOWA
{
	class EXPORTED InputDeviceModule
	{
	public:
		friend class InputDeviceCore;

		enum Action
		{
			UP = 0,
			DOWN,
			LEFT,
			RIGHT,
			JUMP,
			RUN,
			COWER,
			ATTACK_1,
			ATTACK_2,
			DUCK,
			SNEAK,
			ACTION,
			RELOAD,
			INVENTORY,
			MAP,
			SELECT,
			START,
			SAVE,
			LOAD,
			CAMERA_FORWARD,
			CAMERA_BACKWARD,
			CAMERA_LEFT,
			CAMERA_RIGHT,
			CAMERA_UP,
			CAMERA_DOWN,
			CONSOLE,
			WEAPON_CHANGE_FORWARD,
			WEAPON_CHANGE_BACKWARD,
			FLASH_LIGHT,
			PAUSE,
			GRID,
			NONE = 50
		};

		enum JoyStickButton
		{
			BUTTON_X = 0,
			BUTTON_A,
			BUTTON_B,
			BUTTON_Y,
			BUTTON_LB,
			BUTTON_RB,
			BUTTON_LT,
			BUTTON_RT,
			BUTTON_SELECT,
			BUTTON_START,
			BUTTON_LEFT_STICK,
			BUTTON_RIGHT_STICK,
			BUTTON_LEFT_STICK_UP,
			BUTTON_LEFT_STICK_DOWN, 
			BUTTON_LEFT_STICK_LEFT,
			BUTTON_LEFT_STICK_RIGHT,
			BUTTON_RIGHT_STICK_UP,
			BUTTON_RIGHT_STICK_DOWN, 
			BUTTON_RIGHT_STICK_LEFT,
			BUTTON_RIGHT_STICK_RIGHT,
			BUTTON_NONE = 50,
		};

	public:
		const Ogre::String& getDeviceName(void) const;

		bool isKeyboardDevice(void) const;

		bool isOccupied(void) const;

		unsigned long getOccupiedId(void) const;

		void setOccupiedId(unsigned long id);

		unsigned short getKeyMappingCount(void) const;

		unsigned short getButtonMappingCount(void) const;

		OIS::KeyCode getMappedKey(Action action);

		JoyStickButton getMappedButton(Action action);

		Ogre::String getStringFromMappedKey(OIS::KeyCode keyCode);

		Ogre::String getStringFromMappedKeyAction(Action action);

		Ogre::String getStringFromMappedButton(JoyStickButton joyStickButton);

		Ogre::String getStringFromMappedButtonAction(Action action);
		
		OIS::KeyCode getMappedKeyFromString(const Ogre::String& key);

		JoyStickButton getMappedButtonFromString(const Ogre::String& button);
		
		std::vector<Ogre::String> getAllKeyStrings(void);

		std::vector<Ogre::String> getAllButtonStrings(void);

		void remapKey(Action action, OIS::KeyCode keyCode);

		void remapButton(Action action, JoyStickButton joyStickButton);

		void clearKeyMapping(unsigned short tilIndex);

		void clearButtonMapping(unsigned short tilIndex);

		void setDefaultKeyMapping(void);

		void setDefaultButtonMapping(void);
		
		void setJoyStickDeadZone(Ogre::Real deadZone);

		bool hasActiveJoyStick(void) const;

		void setAnalogActionThreshold(Ogre::Real t);

		Ogre::Real getAnalogActionThreshold(void) const;

		Ogre::Real getSteerAxis(void); // [-1..+1]

		/**
		 * @brief		Gets the strength of the left stick horizontal moving
		 * @return		strength, if 0 horizontal stick is not moved. When moved right values are in range (0, 1]. When moved left values are in range (0, -1].
		 */
		Ogre::Real getLeftStickHorizontalMovingStrength(void) const;

		/**
		 * @brief		Gets the strength of the left stick vertical moving
		 * @return		strength, if 0 vertical stick is not moved. When moved up values are in range (0, 1]. When moved down values are in range (0, -1].
		 */
		Ogre::Real getLeftStickVerticalMovingStrength(void) const;

		/**
		 * @brief		Gets the strength of the right stick horizontal moving
		 * @return		strength, if 0 horizontal stick is not moved. When moved right values are in range (0, 1]. When moved left values are in range (0, -1].
		 */
		Ogre::Real getRightStickHorizontalMovingStrength(void) const;

		/**
		 * @brief		Gets the strength of the right stick vertical moving
		 * @return		strength, if 0 vertical stick is not moved. When moved up values are in range (0, 1]. When moved down values are in range (0, -1].
		 */
		Ogre::Real getRightStickVerticalMovingStrength(void) const;

		bool isKeyDown(OIS::KeyCode keyCode) const;

		bool isActionDown(InputDeviceModule::Action action);

		/**
		 * @brief		Gets whether a specific action is down on any device, but max. for a the action duration time.
		 */
		bool isActionDownAmount(InputDeviceModule::Action action, Ogre::Real dt, Ogre::Real actionDuration = 0.2f);

		/**
		 * @brief		Gets whether a specific action is pressed on any device.
		 */
		bool isActionPressed(InputDeviceModule::Action action, Ogre::Real dt, Ogre::Real durationBetweenTheAction = 0.2f);

		bool isButtonDown(JoyStickButton button) const;

		bool areButtonsDown2(JoyStickButton button1, JoyStickButton button2) const;

		bool areButtonsDown3(JoyStickButton button1, JoyStickButton button2, JoyStickButton button3) const;

		bool areButtonsDown4(JoyStickButton button1, JoyStickButton button2, JoyStickButton button3, JoyStickButton button4) const;

		JoyStickButton getPressedButton(void) const;

		std::vector<JoyStickButton> getPressedButtons(void) const;

		void update(Ogre::Real dt);
	private:
		InputDeviceModule(const Ogre::String& deviceName, bool isKeyboard, OIS::Object* deviceObject);

		~InputDeviceModule()
		{

		};

		void releaseOccupation(void);
	private:
		Ogre::String deviceName;
		bool isKeyboard;
		unsigned long occuppiedId; // 0 means unoccupied

		OIS::Object* deviceObject;

		std::map<Action, OIS::KeyCode> keyboardMapping;
		std::map<Action, JoyStickButton> buttonMapping;
		std::set<OIS::KeyCode> allOISKeys;
		std::set<JoyStickButton> allButtons;
		Ogre::Real joyStickDeadZone;
		Ogre::Vector2 rightStickMovement;
		Ogre::Vector2 leftStickMovement;
		Ogre::Vector2 povMovement;
		JoyStickButton pressedButton;
		Action pressedPov[4]; // two buttons of left and right Steuerkreuz can be pressed simultanously
		std::vector<JoyStickButton> pressedButtons;
		Ogre::Real analogActionThreshold;

		Ogre::Real timeSinceLastActionDown;
		Ogre::Real timeSinceLastActionPressed;
		bool canPress;
	};

	// Shortcuts for currently mapped keys
	#define NOWA_K_JUMP NOWA::InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule()->getMappedKey(NOWA::InputDeviceModule::JUMP)
	#define NOWA_K_RUN NOWA::InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule()->getMappedKey(NOWA::InputDeviceModule::RUN)
	#define NOWA_K_COWER NOWA::InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule()->getMappedKey(NOWA::InputDeviceModule::COWER)
	#define NOWA_K_DUCK NOWA::InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule()->getMappedKey(NOWA::InputDeviceModule::DUCK)
	#define NOWA_K_SNEAK NOWA::InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule()->getMappedKey(NOWA::InputDeviceModule::SNEAK)
	#define NOWA_K_ATTACK_1 NOWA::InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule()->getMappedKey(NOWA::InputDeviceModule::ATTACK_1)
	#define NOWA_K_ATTACK_2 NOWA::InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule()->getMappedKey(NOWA::InputDeviceModule::ATTACK_2)
	#define NOWA_K_ACTION NOWA::InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule()->getMappedKey(NOWA::InputDeviceModule::ACTION)
	#define NOWA_K_RELOAD NOWA::InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule()->getMappedKey(NOWA::InputDeviceModule::RELOAD)
	#define NOWA_K_INVENTORY NOWA::InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule()->getMappedKey(NOWA::InputDeviceModule::INVENTORY)
	#define NOWA_K_MAP NOWA::InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule()->getMappedKey(NOWA::InputDeviceModule::MAP)
	#define NOWA_K_SELECT NOWA::InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule()->getMappedKey(NOWA::InputDeviceModule::SELECT)
	#define NOWA_K_START NOWA::InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule()->getMappedKey(NOWA::InputDeviceModule::START)
	#define NOWA_K_SAVE NOWA::InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule()->getMappedKey(NOWA::InputDeviceModule::SAVE) 
	#define NOWA_K_LOAD NOWA::InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule()->getMappedKey(NOWA::InputDeviceModule::LOAD)
	#define NOWA_K_CAMERA_FORWARD NOWA::InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule()->getMappedKey(NOWA::InputDeviceModule::CAMERA_FORWARD)
	#define NOWA_K_CAMERA_BACKWARD NOWA::InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule()->getMappedKey(NOWA::InputDeviceModule::CAMERA_BACKWARD)
	#define NOWA_K_CAMERA_LEFT NOWA::InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule()->getMappedKey(NOWA::InputDeviceModule::CAMERA_LEFT)
	#define NOWA_K_CAMERA_RIGHT NOWA::InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule()->getMappedKey(NOWA::InputDeviceModule::CAMERA_RIGHT)
	#define NOWA_K_CAMERA_UP NOWA::InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule()->getMappedKey(NOWA::InputDeviceModule::CAMERA_UP)
	#define NOWA_K_CAMERA_DOWN NOWA::InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule()->getMappedKey(NOWA::InputDeviceModule::CAMERA_DOWN)
	#define NOWA_K_CONSOLE NOWA::InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule()->getMappedKey(NOWA::InputDeviceModule::CONSOLE)
	#define NOWA_K_WEAPON_CHANGE_FORWARD NOWA::InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule()->getMappedKey(NOWA::InputDeviceModule::WEAPON_CHANGE_FORWARD)
	#define NOWA_K_WEAPON_CHANGE_BACKWARD NOWA::InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule()->getMappedKey(NOWA::InputDeviceModule::WEAPON_CHANGE_BACKWARD)
	#define NOWA_K_FLASH_LIGHT NOWA::InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule()->getMappedKey(NOWA::InputDeviceModule::FLASH_LIGHT)
	#define NOWA_K_PAUSE NOWA::InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule()->getMappedKey(NOWA::InputDeviceModule::PAUSE)
	#define NOWA_K_GRID NOWA::InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule()->getMappedKey(NOWA::InputDeviceModule::GRID)
	#define NOWA_K_UP NOWA::InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule()->getMappedKey(NOWA::InputDeviceModule::UP)
	#define NOWA_K_DOWN NOWA::InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule()->getMappedKey(NOWA::InputDeviceModule::DOWN)
	#define NOWA_K_LEFT NOWA::InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule()->getMappedKey(NOWA::InputDeviceModule::LEFT)
	#define NOWA_K_RIGHT NOWA::InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule()->getMappedKey(NOWA::InputDeviceModule::RIGHT)

	// For both
	#define NOWA_A_JUMP NOWA::InputDeviceModule::JUMP
	#define NOWA_A_RUN NOWA::InputDeviceModule::RUN
	#define NOWA_A_COWER NOWA::InputDeviceModule::COWER
	#define NOWA_A_DUCK NOWA::InputDeviceModule::DUCK
	#define NOWA_A_SNEAK NOWA::InputDeviceModule::SNEAK
	#define NOWA_A_ATTACK_1 NOWA::InputDeviceModule::ATTACK_1
	#define NOWA_A_ATTACK_2 NOWA::InputDeviceModule::ATTACK_2
	#define NOWA_A_ACTION NOWA::InputDeviceModule::ACTION
	#define NOWA_A_RELOAD NOWA::InputDeviceModule::RELOAD
	#define NOWA_A_INVENTORY NOWA::InputDeviceModule::INVENTORY
	#define NOWA_A_MAP NOWA::InputDeviceModule::MAP
	#define NOWA_A_SELECT NOWA::InputDeviceModule::SELECT
	#define NOWA_A_START NOWA::InputDeviceModule::START
	#define NOWA_A_SAVE NOWA::InputDeviceModule::SAVE 
	#define NOWA_A_LOAD NOWA::InputDeviceModule::LOAD
	#define NOWA_A_CAMERA_FORWARD NOWA::InputDeviceModule::CAMERA_FORWARD
	#define NOWA_A_CAMERA_BACKWARD NOWA::InputDeviceModule::CAMERA_BACKWARD
	#define NOWA_A_CAMERA_LEFT NOWA::InputDeviceModule::CAMERA_LEFT
	#define NOWA_A_CAMERA_RIGHT NOWA::InputDeviceModule::CAMERA_RIGHT
	#define NOWA_A_CAMERA_UP NOWA::InputDeviceModule::CAMERA_UP
	#define NOWA_A_CAMERA_DOWN NOWA::InputDeviceModule::CAMERA_DOWN
	#define NOWA_A_CONSOLE NOWA::InputDeviceModule::CONSOLE
	#define NOWA_A_WEAPON_CHANGE_FORWARD NOWA::InputDeviceModule::WEAPON_CHANGE_FORWARD
	#define NOWA_A_WEAPON_CHANGE_BACKWARD NOWA::InputDeviceModule::WEAPON_CHANGE_BACKWARD
	#define NOWA_A_FLASH_LIGHT NOWA::InputDeviceModule::FLASH_LIGHT
	#define NOWA_A_PAUSE NOWA::InputDeviceModule::PAUSE
	#define NOWA_A_GRID NOWA::InputDeviceModule::GRID
	#define NOWA_A_UP NOWA::InputDeviceModule::UP
	#define NOWA_A_DOWN NOWA::InputDeviceModule::DOWN
	#define NOWA_A_LEFT NOWA::InputDeviceModule::LEFT
	#define NOWA_A_RIGHT NOWA::InputDeviceModule::RIGHT

}; //namespace end

#endif

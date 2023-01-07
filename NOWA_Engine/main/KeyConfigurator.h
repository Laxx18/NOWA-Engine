#ifndef INPUT_DEVICE_MODULE_H
#define INPUT_DEVICE_MODULE_H

#include <map>
#include "defines.h"

namespace NOWA
{
	class EXPORTED InputDeviceModule
	{
	public:
		enum Action
		{
			UP = 0,
			DOWN,
			LEFT,
			RIGHT,
			JUMP,
			RUN,
			COWER,
			DUCK,
			SNEAK,
			ATTACK_1,
			ATTACK_2,
			ACTION,
			RELOAD,
			INVENTORY,
			MAP,
			PAUSE,
			MENU,
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
			SELECT,
			GRID
		};

		enum JoyStickButton
		{
			BUTTON_A = 0,
			BUTTON_B = 1,
			BUTTON_X = 2,
			BUTTON_Y = 3,
			BUTTON_LB = 4,
			BUTTON_RB = 5,
			BUTTON_SELECT = 6,
			BUTTON_MENU = 7,
			BUTTON_LEFT_STICK = 8,
			BUTTON_RIGHT_STICK = 9,
			BUTTON_NONE = 10

		};

public:

		unsigned short getKeyMappingCount(void) const;

		unsigned short getButtonMappingCount(void) const;

		OIS::KeyCode getMappedKey(Action action);

		JoyStickButton getMappedButton(Action action);

		Ogre::String getStringFromMappedKey(OIS::KeyCode keyCode);

		Ogre::String getStringFromMappedButton(JoyStickButton joyStickButton);
		
		OIS::KeyCode getMappedKeyFromString(const Ogre::String& key);

		JoyStickButton getMappedButtonFromString(const Ogre::String& button);
		
		std::vector<Ogre::String> getAllKeyStrings(void);

		std::vector<Ogre::String> getAllButtonStrings(void);

		void remapKey(Action action, OIS::KeyCode keyCode);

		void remapButton(Action action, JoyStickButton joyStickButton);
		
		void setJoyStickDeadZone(Ogre::Real deadZone);

		bool hasActiveJoyStick(void) const;

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

		JoyStickButton getSinglePressedButton(void) const;

		void update(Ogre::Real dt);
	public:
		static InputDeviceModule* getInstance();
	private:
		InputDeviceModule();

		~InputDeviceModule()
		{

		};
	private:
		std::map<Action, OIS::KeyCode> keyboardMapping;
		std::map<Action, JoyStickButton> buttonMapping;
		std::set<OIS::KeyCode> allOISKeys;
		std::set< JoyStickButton> allButtons;
		Ogre::Real joyStickDeadZone;
		Ogre::Vector2 rightStickMovement;
		Ogre::Vector2 leftStickMovement;
		Ogre::Vector2 povMovement;
		JoyStickButton pressedButton;
	};

	// Shortcuts for currently mapped keys
	#define NOWA_K_UP NOWA::InputDeviceModule::getInstance()->getMappedKey(InputDeviceModule::UP)
	#define NOWA_K_DOWN NOWA::InputDeviceModule::getInstance()->getMappedKey(InputDeviceModule::DOWN)
	#define NOWA_K_LEFT NOWA::InputDeviceModule::getInstance()->getMappedKey(InputDeviceModule::LEFT)
	#define NOWA_K_RIGHT NOWA::InputDeviceModule::getInstance()->getMappedKey(InputDeviceModule::RIGHT)
	#define NOWA_K_JUMP NOWA::InputDeviceModule::getInstance()->getMappedKey(InputDeviceModule::JUMP)
	#define NOWA_K_RUN NOWA::InputDeviceModule::getInstance()->getMappedKey(InputDeviceModule::RUN)
	#define NOWA_K_COWER NOWA::InputDeviceModule::getInstance()->getMappedKey(InputDeviceModule::COWER)
	#define NOWA_K_DUCK NOWA::InputDeviceModule::getInstance()->getMappedKey(InputDeviceModule::DUCK)
	#define NOWA_K_SNEAK NOWA::InputDeviceModule::getInstance()->getMappedKey(InputDeviceModule::SNEAK)
	#define NOWA_K_ATTACK_1 NOWA::InputDeviceModule::getInstance()->getMappedKey(InputDeviceModule::ATTACK_1)
	#define NOWA_K_ATTACK_2 NOWA::InputDeviceModule::getInstance()->getMappedKey(InputDeviceModule::ATTACK_2)
	#define NOWA_K_ACTION NOWA::InputDeviceModule::getInstance()->getMappedKey(InputDeviceModule::ACTION) // This is new and must be set in ogre.xml and defaultConfig.xml
	#define NOWA_K_RELOAD NOWA::InputDeviceModule::getInstance()->getMappedKey(InputDeviceModule::RELOAD)
	#define NOWA_K_INVENTORY NOWA::InputDeviceModule::getInstance()->getMappedKey(InputDeviceModule::INVENTORY)
	#define NOWA_K_MAP NOWA::InputDeviceModule::getInstance()->getMappedKey(InputDeviceModule::MAP)
	#define NOWA_K_PAUSE NOWA::InputDeviceModule::getInstance()->getMappedKey(InputDeviceModule::PAUSE)  // This is new and must be set in ogre.xml and defaultConfig.xml
	#define NOWA_K_MENU NOWA::InputDeviceModule::getInstance()->getMappedKey(InputDeviceModule::MENU)  // This is new and must be set in ogre.xml and defaultConfig.xml
	#define NOWA_K_SAVE NOWA::InputDeviceModule::getInstance()->getMappedKey(InputDeviceModule::SAVE) 
	#define NOWA_K_LOAD NOWA::InputDeviceModule::getInstance()->getMappedKey(InputDeviceModule::LOAD)
	#define NOWA_K_CAMERA_FORWARD NOWA::InputDeviceModule::getInstance()->getMappedKey(InputDeviceModule::CAMERA_FORWARD)
	#define NOWA_K_CAMERA_BACKWARD NOWA::InputDeviceModule::getInstance()->getMappedKey(InputDeviceModule::CAMERA_BACKWARD)
	#define NOWA_K_CAMERA_LEFT NOWA::InputDeviceModule::getInstance()->getMappedKey(InputDeviceModule::CAMERA_LEFT)
	#define NOWA_K_CAMERA_RIGHT NOWA::InputDeviceModule::getInstance()->getMappedKey(InputDeviceModule::CAMERA_RIGHT)
	#define NOWA_K_CAMERA_UP NOWA::InputDeviceModule::getInstance()->getMappedKey(InputDeviceModule::CAMERA_UP)
	#define NOWA_K_CAMERA_DOWN NOWA::InputDeviceModule::getInstance()->getMappedKey(InputDeviceModule::CAMERA_DOWN)
	#define NOWA_K_CONSOLE NOWA::InputDeviceModule::getInstance()->getMappedKey(InputDeviceModule::CONSOLE)
	#define NOWA_K_WEAPON_CHANGE_FORWARD NOWA::InputDeviceModule::getInstance()->getMappedKey(InputDeviceModule::WEAPON_CHANGE_FORWARD)
	#define NOWA_K_WEAPON_CHANGE_BACKWARD NOWA::InputDeviceModule::getInstance()->getMappedKey(InputDeviceModule::WEAPON_CHANGE_BACKWARD)
	#define NOWA_K_FLASH_LIGHT NOWA::InputDeviceModule::getInstance()->getMappedKey(InputDeviceModule::FLASH_LIGHT)
	#define NOWA_K_SELECT NOWA::InputDeviceModule::getInstance()->getMappedKey(InputDeviceModule::SELECT)
	#define NOWA_K_GRID NOWA::InputDeviceModule::getInstance()->getMappedKey(InputDeviceModule::GRID)

	#define NOWA_B_JUMP NOWA::InputDeviceModule::getInstance()->getMappedButton(InputDeviceModule::JUMP)
	#define NOWA_B_RUN NOWA::InputDeviceModule::getInstance()->getMappedButton(InputDeviceModule::RUN)
	#define NOWA_B_ATTACK_1 NOWA::InputDeviceModule::getInstance()->getMappedButton(InputDeviceModule::ATTACK_1)
	#define NOWA_B_ACTION NOWA::InputDeviceModule::getInstance()->getMappedButton(InputDeviceModule::ACTION)
	#define NOWA_B_RELOAD NOWA::InputDeviceModule::getInstance()->getMappedButton(InputDeviceModule::RELOAD)
	#define NOWA_B_INVENTORY NOWA::InputDeviceModule::getInstance()->getMappedButton(InputDeviceModule::INVENTORY)
	#define NOWA_B_MAP NOWA::InputDeviceModule::getInstance()->getMappedButton(InputDeviceModule::MAP)
	#define NOWA_B_PAUSE NOWA::InputDeviceModule::getInstance()->getMappedButton(InputDeviceModule::PAUSE)
	#define NOWA_B_MENU NOWA::InputDeviceModule::getInstance()->getMappedButton(InputDeviceModule::MENU)
	#define NOWA_B_FLASH_LIGHT NOWA::InputDeviceModule::getInstance()->getMappedButton(InputDeviceModule::FLASH_LIGHT)
}; //namespace end

#endif

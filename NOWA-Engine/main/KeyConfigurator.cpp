#include "NOWAPrecompiled.h"
#include "InputDeviceModule.h"
#include "Core.h"

namespace NOWA
{
	InputDeviceModule::InputDeviceModule()
		: joyStickDeadZone(0.05f),
		rightStickMovement(Ogre::Vector2::ZERO),
		leftStickMovement(Ogre::Vector2::ZERO),
		povMovement(Ogre::Vector2::ZERO),
		pressedButton(JoyStickButton::BUTTON_NONE)
	{
		this->keyboardMapping[Action::UP] = OIS::KC_UP;
		this->keyboardMapping[Action::DOWN] = OIS::KC_DOWN;
		this->keyboardMapping[Action::LEFT] = OIS::KC_LEFT;
		this->keyboardMapping[Action::RIGHT] = OIS::KC_RIGHT;
		this->keyboardMapping[Action::JUMP] = OIS::KC_SPACE;
		this->keyboardMapping[Action::RUN] = OIS::KC_RCONTROL;
		this->keyboardMapping[Action::COWER] = OIS::KC_X;
		this->keyboardMapping[Action::DUCK] = OIS::KC_C;
		this->keyboardMapping[Action::SNEAK] = OIS::KC_RSHIFT;
		this->keyboardMapping[Action::ATTACK_1] = OIS::KC_RETURN;
		this->keyboardMapping[Action::ATTACK_2] = OIS::KC_LSHIFT;
		this->keyboardMapping[Action::ACTION] = OIS::KC_E;
		this->keyboardMapping[Action::RELOAD] = OIS::KC_R;
		this->keyboardMapping[Action::INVENTORY] = OIS::KC_I;
		this->keyboardMapping[Action::MAP] = OIS::KC_TAB;
		this->keyboardMapping[Action::SAVE] = OIS::KC_F5;
		this->keyboardMapping[Action::LOAD] = OIS::KC_F4;
		this->keyboardMapping[Action::PAUSE] = OIS::KC_P;
		this->keyboardMapping[Action::MENU] = OIS::KC_ESCAPE;
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

		this->buttonMapping[Action::JUMP] = JoyStickButton::BUTTON_B;
		this->buttonMapping[Action::RUN] = JoyStickButton::BUTTON_A;
		this->buttonMapping[Action::ATTACK_1] = JoyStickButton::BUTTON_X;
		this->buttonMapping[Action::ACTION] = JoyStickButton::BUTTON_Y;
		this->buttonMapping[Action::RELOAD] = JoyStickButton::BUTTON_RB;
		this->buttonMapping[Action::INVENTORY] = JoyStickButton::BUTTON_SELECT;
		this->buttonMapping[Action::MAP] = JoyStickButton::BUTTON_RB;
		this->buttonMapping[Action::PAUSE] = JoyStickButton::BUTTON_LEFT_STICK;
		this->buttonMapping[Action::MENU] = JoyStickButton::BUTTON_MENU;
		this->buttonMapping[Action::FLASH_LIGHT] = JoyStickButton::BUTTON_RIGHT_STICK;
		
		this->allOISKeys.emplace(OIS::KC_ESCAPE);
		this->allOISKeys.emplace(OIS::KC_1);
		this->allOISKeys.emplace(OIS::KC_2);
		this->allOISKeys.emplace(OIS::KC_3);
		this->allOISKeys.emplace(OIS::KC_4);
		this->allOISKeys.emplace(OIS::KC_5);
		this->allOISKeys.emplace(OIS::KC_6);
		this->allOISKeys.emplace(OIS::KC_7);
		this->allOISKeys.emplace(OIS::KC_8);
		this->allOISKeys.emplace(OIS::KC_9);
		this->allOISKeys.emplace(OIS::KC_0);
		this->allOISKeys.emplace(OIS::KC_MINUS);
		this->allOISKeys.emplace(OIS::KC_EQUALS);
		this->allOISKeys.emplace(OIS::KC_BACK);
		this->allOISKeys.emplace(OIS::KC_TAB);
		this->allOISKeys.emplace(OIS::KC_Q);
		this->allOISKeys.emplace(OIS::KC_W);
		this->allOISKeys.emplace(OIS::KC_E);
		this->allOISKeys.emplace(OIS::KC_R);
		this->allOISKeys.emplace(OIS::KC_T);
		this->allOISKeys.emplace(OIS::KC_Y);
		this->allOISKeys.emplace(OIS::KC_U);
		this->allOISKeys.emplace(OIS::KC_I);
		this->allOISKeys.emplace(OIS::KC_O);
		this->allOISKeys.emplace(OIS::KC_P);
		this->allOISKeys.emplace(OIS::KC_LBRACKET);
		this->allOISKeys.emplace(OIS::KC_RBRACKET);
		this->allOISKeys.emplace(OIS::KC_RETURN);
		this->allOISKeys.emplace(OIS::KC_LCONTROL);
		this->allOISKeys.emplace(OIS::KC_A);
		this->allOISKeys.emplace(OIS::KC_S);
		this->allOISKeys.emplace(OIS::KC_D);
		this->allOISKeys.emplace(OIS::KC_F);
		this->allOISKeys.emplace(OIS::KC_G);
		this->allOISKeys.emplace(OIS::KC_H);
		this->allOISKeys.emplace(OIS::KC_J);
		this->allOISKeys.emplace(OIS::KC_K);
		this->allOISKeys.emplace(OIS::KC_L);
		this->allOISKeys.emplace(OIS::KC_SEMICOLON);
		this->allOISKeys.emplace(OIS::KC_APOSTROPHE);
		this->allOISKeys.emplace(OIS::KC_GRAVE);
		this->allOISKeys.emplace(OIS::KC_LSHIFT);
		this->allOISKeys.emplace(OIS::KC_BACKSLASH);
		this->allOISKeys.emplace(OIS::KC_Z);
		this->allOISKeys.emplace(OIS::KC_X);
		this->allOISKeys.emplace(OIS::KC_C);
		this->allOISKeys.emplace(OIS::KC_V);
		this->allOISKeys.emplace(OIS::KC_B);
		this->allOISKeys.emplace(OIS::KC_N);
		this->allOISKeys.emplace(OIS::KC_M);
		this->allOISKeys.emplace(OIS::KC_COMMA);
		this->allOISKeys.emplace(OIS::KC_PERIOD);
		this->allOISKeys.emplace(OIS::KC_SLASH);
		this->allOISKeys.emplace(OIS::KC_RSHIFT);
		this->allOISKeys.emplace(OIS::KC_MULTIPLY);
		this->allOISKeys.emplace(OIS::KC_LMENU);
		this->allOISKeys.emplace(OIS::KC_SPACE);
		this->allOISKeys.emplace(OIS::KC_CAPITAL);
		this->allOISKeys.emplace(OIS::KC_F1);
		this->allOISKeys.emplace(OIS::KC_F2);
		this->allOISKeys.emplace(OIS::KC_F3);
		this->allOISKeys.emplace(OIS::KC_F4);
		this->allOISKeys.emplace(OIS::KC_F5);
		this->allOISKeys.emplace(OIS::KC_F6);
		this->allOISKeys.emplace(OIS::KC_F7);
		this->allOISKeys.emplace(OIS::KC_F8);
		this->allOISKeys.emplace(OIS::KC_F9);
		this->allOISKeys.emplace(OIS::KC_F10);
		this->allOISKeys.emplace(OIS::KC_NUMLOCK);
		this->allOISKeys.emplace(OIS::KC_SCROLL);
		this->allOISKeys.emplace(OIS::KC_NUMPAD7);
		this->allOISKeys.emplace(OIS::KC_NUMPAD8);
		this->allOISKeys.emplace(OIS::KC_NUMPAD9);
		this->allOISKeys.emplace(OIS::KC_SUBTRACT);
		this->allOISKeys.emplace(OIS::KC_NUMPAD4);
		this->allOISKeys.emplace(OIS::KC_NUMPAD5);
		this->allOISKeys.emplace(OIS::KC_NUMPAD6);
		this->allOISKeys.emplace(OIS::KC_ADD);
		this->allOISKeys.emplace(OIS::KC_NUMPAD1);
		this->allOISKeys.emplace(OIS::KC_NUMPAD2);
		this->allOISKeys.emplace(OIS::KC_NUMPAD3);
		this->allOISKeys.emplace(OIS::KC_NUMPAD0);
		this->allOISKeys.emplace(OIS::KC_DECIMAL);
		this->allOISKeys.emplace(OIS::KC_OEM_102);
		this->allOISKeys.emplace(OIS::KC_F11);
		this->allOISKeys.emplace(OIS::KC_F12);
		this->allOISKeys.emplace(OIS::KC_RCONTROL);
		this->allOISKeys.emplace(OIS::KC_NUMPADCOMMA);
		this->allOISKeys.emplace(OIS::KC_DIVIDE);
		this->allOISKeys.emplace(OIS::KC_SYSRQ);
		this->allOISKeys.emplace(OIS::KC_RMENU);
		this->allOISKeys.emplace(OIS::KC_PAUSE);
		this->allOISKeys.emplace(OIS::KC_HOME);
		this->allOISKeys.emplace(OIS::KC_UP);
		this->allOISKeys.emplace(OIS::KC_PGUP);
		this->allOISKeys.emplace(OIS::KC_LEFT);
		this->allOISKeys.emplace(OIS::KC_RIGHT);
		this->allOISKeys.emplace(OIS::KC_END);
		this->allOISKeys.emplace(OIS::KC_DOWN);
		this->allOISKeys.emplace(OIS::KC_PGDOWN);
		this->allOISKeys.emplace(OIS::KC_INSERT);
		this->allOISKeys.emplace(OIS::KC_DELETE);

		this->allButtons.emplace(JoyStickButton::BUTTON_A);
		this->allButtons.emplace(JoyStickButton::BUTTON_B);
		this->allButtons.emplace(JoyStickButton::BUTTON_X);
		this->allButtons.emplace(JoyStickButton::BUTTON_Y);
		this->allButtons.emplace(JoyStickButton::BUTTON_LB);
		this->allButtons.emplace(JoyStickButton::BUTTON_RB);
		this->allButtons.emplace(JoyStickButton::BUTTON_SELECT);
		this->allButtons.emplace(JoyStickButton::BUTTON_MENU);
		this->allButtons.emplace(JoyStickButton::BUTTON_LEFT_STICK);
		this->allButtons.emplace(JoyStickButton::BUTTON_RIGHT_STICK);
	}
	
	OIS::KeyCode InputDeviceModule::getMappedKeyFromString(const Ogre::String& key)
	{
		if ("Esc" == key)
			return OIS::KC_ESCAPE;
		else if ("1" == key)
			return OIS::KC_1;
		else if ("2" == key)
			return OIS::KC_2;
		else if ("3" == key)
			return OIS::KC_3;
		else if ("4" == key)
			return OIS::KC_4;
		else if ("5" == key)
			return OIS::KC_5;
		else if ("6" == key)
			return OIS::KC_6;
		else if ("7" == key)
			return OIS::KC_7;
		else if ("8" == key)
			return OIS::KC_8;
		else if ("9" == key)
			return OIS::KC_9;
		else if ("0" == key)
			return OIS::KC_0;
		else if ("-" == key)
			return OIS::KC_MINUS;
		else if ("=" == key)
			return OIS::KC_EQUALS;
		else if ("Backspace" == key)
			return OIS::KC_BACK;
		else if ("Tab" == key)
			return OIS::KC_TAB;
		else if ("Q" == key)
			return OIS::KC_Q;
		else if ("W" == key)
			return OIS::KC_W;
		else if ("E" == key)
			return OIS::KC_E;
		else if ("R" == key)
			return OIS::KC_R;
		else if ("T" == key)
			return OIS::KC_T;
		else if ("U" == key)
			return OIS::KC_U;
		else if ("I" == key)
			return OIS::KC_I;
		else if ("O" == key)
			return OIS::KC_O;
		else if ("P" == key)
			return OIS::KC_P;
		else if ("(" == key)
			return OIS::KC_LBRACKET;
		else if (")" == key)
			return OIS::KC_RBRACKET;
		else if ("Return" == key)
			return OIS::KC_RETURN;
		else if ("L-Control" == key)
			return OIS::KC_LCONTROL;
		else if ("A" == key)
			return OIS::KC_A;
		else if ("S" == key)
			return OIS::KC_S;
		else if ("D" == key)
			return OIS::KC_D;
		else if ("F" == key)
			return OIS::KC_F;
		else if ("G" == key)
			return OIS::KC_G;
		else if ("H" == key)
			return OIS::KC_H;
		else if ("J" == key)
			return OIS::KC_J;
		else if ("K" == key)
			return OIS::KC_K;
		else if ("L" == key)
			return OIS::KC_L;
		else if (";" == key)
			return OIS::KC_SEMICOLON;
		else if ("´" == key)
			return OIS::KC_APOSTROPHE;
		else if ("^" == key)
			return OIS::KC_GRAVE;
		else if ("L-Shift" == key)
			return OIS::KC_LSHIFT;
		else if ("\"" == key)
			return OIS::KC_BACKSLASH;
		else if ("Z" == key)
			return OIS::KC_Z;
		else if ("X" == key)
			return OIS::KC_X;
		else if ("C" == key)
			return OIS::KC_C;
		else if ("V" == key)
			return OIS::KC_V;
		else if ("B" == key)
			return OIS::KC_B;
		else if ("N" == key)
			return OIS::KC_N;
		else if ("M" == key)
			return OIS::KC_M;
		else if ("," == key)
			return OIS::KC_COMMA;
		else if ("." == key)
			return OIS::KC_PERIOD;
		else if ("Slash" == key)
			return OIS::KC_SLASH;
		else if ("R-Shift" == key)
			return OIS::KC_RSHIFT;
		else if ("*" == key)
			return OIS::KC_MULTIPLY;
		else if ("L-Alt" == key)
			return OIS::KC_LMENU;
		else if ("Space" == key)
			return OIS::KC_SPACE;
		else if ("Capital" == key)
			return OIS::KC_CAPITAL;
		else if ("F1" == key)
			return OIS::KC_F1;
		else if ("F2" == key)
			return OIS::KC_F2;
		else if ("F3" == key)
			return OIS::KC_F3;
		else if ("F4" == key)
			return OIS::KC_F4;
		else if ("F5" == key)
			return OIS::KC_F5;
		else if ("F6" == key)
			return OIS::KC_F6;
		else if ("F7" == key)
			return OIS::KC_F7;
		else if ("F8" == key)
			return OIS::KC_F8;
		else if ("F9" == key)
			return OIS::KC_F9;
		else if ("F10" == key)
			return OIS::KC_F10;
		else if ("Numlock" == key)
			return OIS::KC_NUMLOCK;
		else if ("Scroll" == key)
			return OIS::KC_SCROLL;
		else if ("Num 7" == key)
			return OIS::KC_NUMPAD7;
		else if ("Num 8" == key)
			return OIS::KC_NUMPAD8;
		else if ("Num 9" == key)
			return OIS::KC_NUMPAD9;
		else if ("Sub" == key)
			return OIS::KC_SUBTRACT;
		else if ("Num 4" == key)
			return OIS::KC_NUMPAD4;
		else if ("Num 5" == key)
			return OIS::KC_NUMPAD5;
		else if ("Num 6" == key)
			return OIS::KC_NUMPAD6;
		else if ("+" == key)
			return OIS::KC_ADD;
		else if ("Num 1" == key)
			return OIS::KC_NUMPAD1;
		else if ("Num 2" == key)
			return OIS::KC_NUMPAD2;
		else if ("Num 3" == key)
			return OIS::KC_NUMPAD3;
		else if ("Num 0" == key)
			return OIS::KC_NUMPAD0;
		else if ("Num ." == key)
			return OIS::KC_DECIMAL;
		else if ("F11" == key)
			return OIS::KC_F11;
		else if ("F12" == key)
			return OIS::KC_F12;
		else if ("L-Control" == key)
			return OIS::KC_RCONTROL;
		else if ("Num ," == key)
			return OIS::KC_NUMPADCOMMA;
		else if ("/" == key)
			return OIS::KC_DIVIDE;
		else if ("Esc" == key)
			return OIS::KC_SYSRQ;
		else if ("SysRQ" == key)
			return OIS::KC_RMENU;
		else if ("R-Alt" == key)
			return OIS::KC_PAUSE;
		else if ("Home" == key)
			return OIS::KC_HOME;
		else if ("Up" == key)
			return OIS::KC_UP;
		else if ("Page-Up" == key)
			return OIS::KC_PGUP;
		else if ("Left" == key)
			return OIS::KC_LEFT;
		else if ("Right" == key)
			return OIS::KC_RIGHT;
		else if ("End" == key)
			return OIS::KC_END;
		else if ("Down" == key)
			return OIS::KC_DOWN;
		else if ("Page-Down" == key)
			return OIS::KC_PGDOWN;
		else if ("Insert" == key)
			return OIS::KC_INSERT;
		else if ("Delete" == key)
			return OIS::KC_DELETE;

		return static_cast<OIS::KeyCode>(0);
	}

	InputDeviceModule::JoyStickButton InputDeviceModule::getMappedButtonFromString(const Ogre::String& button)
	{
		if ("A" == button)
			return JoyStickButton::BUTTON_A;
		else if ("B" == button)
			return JoyStickButton::BUTTON_B;
		else if ("X" == button)
			return JoyStickButton::BUTTON_X;
		else if ("Y" == button)
			return JoyStickButton::BUTTON_Y;
		else if ("LB" == button)
			return JoyStickButton::BUTTON_LB;
		else if ("RB" == button)
			return JoyStickButton::BUTTON_RB;
		else if ("Select" == button)
			return JoyStickButton::BUTTON_SELECT;
		else if ("Menu" == button)
			return JoyStickButton::BUTTON_MENU;
		else if ("LeftStick" == button)
			return JoyStickButton::BUTTON_LEFT_STICK;
		else if ("RightStick" == button)
			return JoyStickButton::BUTTON_RIGHT_STICK;
		else
			return JoyStickButton::BUTTON_NONE;
	}

	InputDeviceModule* InputDeviceModule::getInstance()
	{
		static InputDeviceModule instance;

		return &instance;
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
		return this->keyboardMapping[keyboardAction];
	}

	InputDeviceModule::JoyStickButton InputDeviceModule::getMappedButton(InputDeviceModule::Action action)
	{
		return this->buttonMapping[action];
	}

	Ogre::String InputDeviceModule::getStringFromMappedButton(JoyStickButton joyStickButton)
	{
		if (joyStickButton == JoyStickButton::BUTTON_A)
		{
			return "A";
		}
		else if (joyStickButton == JoyStickButton::BUTTON_B)
		{
			return "B";
		}
		else if (joyStickButton == JoyStickButton::BUTTON_X)
		{
			return "X";
		}
		else if (joyStickButton == JoyStickButton::BUTTON_Y)
		{
			return "Y";
		}
		else if (joyStickButton == JoyStickButton::BUTTON_LB)
		{
			return "LB";
		}
		else if (joyStickButton == JoyStickButton::BUTTON_RB)
		{
			return "RB";
		}
		else if (joyStickButton == JoyStickButton::BUTTON_SELECT)
		{
			return "Select";
		}
		else if (joyStickButton == JoyStickButton::BUTTON_MENU)
		{
			return "Menu";
		}
		else if (joyStickButton == JoyStickButton::BUTTON_LEFT_STICK)
		{
			return "LeftStick";
		}
		else if (joyStickButton == JoyStickButton::BUTTON_RIGHT_STICK)
		{
			return "RightStick";
		}
		else
			return "None";
	}

	Ogre::String InputDeviceModule::getStringFromMappedKey(OIS::KeyCode keyCode)
	{
		if (keyCode == OIS::KC_ESCAPE)
		{
			return "Esc";
		}
		else if (keyCode == OIS::KC_1)
		{
			return "1";
		}
		else if (keyCode == OIS::KC_2)
		{
			return "2";
		}
		else if (keyCode == OIS::KC_3)
		{
			return "3";
		}
		else if (keyCode == OIS::KC_4)
		{
			return "4";
		}
		else if (keyCode == OIS::KC_5)
		{
			return "5";
		}
		else if (keyCode == OIS::KC_6)
		{
			return "6";
		}
		else if (keyCode == OIS::KC_7)
		{
			return "7";
		}
		else if (keyCode == OIS::KC_8)
		{
			return "8";
		}
		else if (keyCode == OIS::KC_9)
		{
			return "9";
		}
		else if (keyCode == OIS::KC_0)
		{
			return "0";
		}
		else if (keyCode == OIS::KC_MINUS)    // - on main keyboard
		{
			return "-";
		}
		else if (keyCode == OIS::KC_EQUALS)
		{
			return "=";
		}
		else if (keyCode == OIS::KC_BACK)    // backspace
		{
			return "Backspace";
		}
		else if (keyCode == OIS::KC_TAB)
		{
			return "Tab";
		}
		else if (keyCode == OIS::KC_Q)
		{
			return "Q";
		}
		else if (keyCode == OIS::KC_W)
		{
			return "W";
		}
		else if (keyCode == OIS::KC_E)
		{
			return "E";
		}
		else if (keyCode == OIS::KC_R)
		{
			return "R";
		}
		else if (keyCode == OIS::KC_T)
		{
			return "T";
		}
		else if (keyCode == OIS::KC_Y)
		{
			return "Y";
		}
		else if (keyCode == OIS::KC_U)
		{
			return "U";
		}
		else if (keyCode == OIS::KC_I)
		{
			return "I";
		}
		else if (keyCode == OIS::KC_O)
		{
			return "O";
		}
		else if (keyCode == OIS::KC_P)
		{
			return "P";
		}
		else if (keyCode == OIS::KC_LBRACKET)
		{
			return "(";
		}
		else if (keyCode == OIS::KC_RBRACKET)
		{
			return ")";
		}
		else if (keyCode == OIS::KC_RETURN)    // Enter on main keyboard
		{
			return "Return";
		}
		else if (keyCode == OIS::KC_LCONTROL)
		{
			return "L-Control";
		}
		else if (keyCode == OIS::KC_A)
		{
			return "A";
		}
		else if (keyCode == OIS::KC_S)
		{
			return "S";
		}
		else if (keyCode == OIS::KC_D)
		{
			return "D";
		}
		else if (keyCode == OIS::KC_F)
		{
			return "F";
		}
		else if (keyCode == OIS::KC_G)
		{
			return "G";
		}
		else if (keyCode == OIS::KC_H)
		{
			return "H";
		}
		else if (keyCode == OIS::KC_J)
		{
			return "J";
		}
		else if (keyCode == OIS::KC_K)
		{
			return "K";
		}
		else if (keyCode == OIS::KC_L)
		{
			return "L";
		}
		else if (keyCode == OIS::KC_SEMICOLON)
		{
			return ";";
		}
		else if (keyCode == OIS::KC_APOSTROPHE)
		{
			return "´";
		}
		else if (keyCode == OIS::KC_GRAVE)    // accent
		{
			return "^";
		}
		else if (keyCode == OIS::KC_LSHIFT)
		{
			return "L-Shift";
		}
		else if (keyCode == OIS::KC_BACKSLASH)
		{
			return "\"";
		}
		else if (keyCode == OIS::KC_Z)
		{
			return "Z";
		}
		else if (keyCode == OIS::KC_X)
		{
			return "X";
		}
		else if (keyCode == OIS::KC_C)
		{
			return "C";
		}
		else if (keyCode == OIS::KC_V)
		{
			return "V";
		}
		else if (keyCode == OIS::KC_B)
		{
			return "B";
		}
		else if (keyCode == OIS::KC_N)
		{
			return "N";
		}
		else if (keyCode == OIS::KC_M)
		{
			return "M";
		}
		else if (keyCode == OIS::KC_COMMA)
		{
			return ",";
		}
		else if (keyCode == OIS::KC_PERIOD)    // . on main keyboard
		{
			return ".";
		}
		else if (keyCode == OIS::KC_SLASH)    // / on main keyboard
		{
			return "Slash";
		}
		else if (keyCode == OIS::KC_RSHIFT)
		{
			return "R-Shift";
		}
		else if (keyCode == OIS::KC_MULTIPLY)    // * on numeric keypad
		{
			return "*";
		}
		else if (keyCode == OIS::KC_LMENU)    // left Alt
		{
			return "L-Alt";
		}
		else if (keyCode == OIS::KC_SPACE)
		{
			return "Space";
		}
		else if (keyCode == OIS::KC_CAPITAL)
		{
			return "Capital";
		}
		else if (keyCode == OIS::KC_F1)
		{
			return "F1";
		}
		else if (keyCode == OIS::KC_F2)
		{
			return "F2";
		}
		else if (keyCode == OIS::KC_F3)
		{
			return "F3";
		}
		else if (keyCode == OIS::KC_F4)
		{
			return "F4";
		}
		else if (keyCode == OIS::KC_F5)
		{
			return "F5";
		}
		else if (keyCode == OIS::KC_F6)
		{
			return "F6";
		}
		else if (keyCode == OIS::KC_F7)
		{
			return "F7";
		}
		else if (keyCode == OIS::KC_F8)
		{
			return "F8";
		}
		else if (keyCode == OIS::KC_F9)
		{
			return "F9";
		}
		else if (keyCode == OIS::KC_F10)
		{
			return "F10";
		}
		else if (keyCode == OIS::KC_NUMLOCK)
		{
			return "Numlock";
		}
		else if (keyCode == OIS::KC_SCROLL)    // Scroll Lock
		{
			return "Scroll";
		}
		else if (keyCode == OIS::KC_NUMPAD7)
		{
			return "Num 7";
		}
		else if (keyCode == OIS::KC_NUMPAD8)
		{
			return "Num 8";
		}
		else if (keyCode == OIS::KC_NUMPAD9)
		{
			return "Num 9";
		}
		else if (keyCode == OIS::KC_SUBTRACT)    // - on numeric keypad
		{
			return "Sub";
		}
		else if (keyCode == OIS::KC_NUMPAD4)
		{
			return "Num 4";
		}
		else if (keyCode == OIS::KC_NUMPAD5)
		{
			return "Num 5";
		}
		else if (keyCode == OIS::KC_NUMPAD6)
		{
			return "Num 6";
		}
		else if (keyCode == OIS::KC_ADD)    // + on numeric keypad
		{
			return "+";
		}
		else if (keyCode == OIS::KC_NUMPAD1)
		{
			return "Num 1";
		}
		else if (keyCode == OIS::KC_NUMPAD2)
		{
			return "Num 2";
		}
		else if (keyCode == OIS::KC_NUMPAD3)
		{
			return "Num 3";
		}
		else if (keyCode == OIS::KC_NUMPAD0)
		{
			return "Num 0";
		}
		else if (keyCode == OIS::KC_DECIMAL)    // . on numeric keypad
		{
			return "Num .";
		}
		else if (keyCode == OIS::KC_OEM_102)    // < > | on UK/Germany keyboards
		{
			return "<";
		}
		else if (keyCode == OIS::KC_F11)
		{
			return "F11";
		}
		else if (keyCode == OIS::KC_F12)
		{
			return "F12";
		}
		else if (keyCode == OIS::KC_RCONTROL)
		{
			return "R-Control";
		}
		else if (keyCode == OIS::KC_NUMPADCOMMA) // on numeric keypad (NEC PC98)
		{
			return "Num ,";
		}
		else if (keyCode == OIS::KC_DIVIDE)    // / on numeric keypad
		{
			return "Num /";
		}
		else if (keyCode == OIS::KC_SYSRQ)
		{
			return "SysRQ";
		}
		else if (keyCode == OIS::KC_RMENU)    // right Alt
		{
			return "R-Alt";
		}
		else if (keyCode == OIS::KC_PAUSE)    // Pause
		{
			return "Pause";
		}
		else if (keyCode == OIS::KC_HOME)    // Home on arrow keypad
		{
			return "Home";
		}
		else if (keyCode == OIS::KC_UP)    // UpArrow on arrow keypad
		{
			return "Up";
		}
		else if (keyCode == OIS::KC_PGUP)    // PgUp on arrow keypad
		{
			return "Page-Up";
		}
		else if (keyCode == OIS::KC_LEFT)    // LeftArrow on arrow keypad
		{
			return "Left";
		}
		else if (keyCode == OIS::KC_RIGHT)    // RightArrow on arrow keypad
		{
			return "Right";
		}
		else if (keyCode == OIS::KC_END)    // End on arrow keypad
		{
			return "End";
		}
		else if (keyCode == OIS::KC_DOWN)    // DownArrow on arrow keypad
		{
			return "Down";
		}
		else if (keyCode == OIS::KC_PGDOWN)    // PgDn on arrow keypad
		{
			return "Page-Down";
		}
		else if (keyCode == OIS::KC_INSERT)    // Insert on arrow keypad
		{
			return "Insert";
		}
		else if (keyCode == OIS::KC_DELETE)    // Delete on arrow keypad
		{
			return "Delete";
		}
		else
		{
			return "";
		}
	}
	
	std::vector<Ogre::String> InputDeviceModule::getAllKeyStrings(void)
	{
		std::vector<Ogre::String> keys(this->allOISKeys.size());
		unsigned short i = 0;
		for (OIS::KeyCode key : this->allOISKeys)
		{
			keys[i++] = this->getStringFromMappedKey(key);
		}
		return keys;
	}

	std::vector<Ogre::String> InputDeviceModule::getAllButtonStrings(void)
	{
		std::vector<Ogre::String> buttons(this->allButtons.size());
		unsigned short i = 0;
		for (JoyStickButton button : this->allButtons)
		{
			buttons[i++] = this->getStringFromMappedButton(button);
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
	}

	bool InputDeviceModule::hasActiveJoyStick(void) const
	{
		return NOWA::Core::getSingletonPtr()->getJoyStick() != nullptr;
	}

	Ogre::Real InputDeviceModule::getLeftStickHorizontalMovingStrength(void) const
	{
		return this->leftStickMovement.x;
	}

	Ogre::Real InputDeviceModule::getLeftStickVerticalMovingStrength(void) const
	{
		return this->leftStickMovement.y;
	}

	Ogre::Real InputDeviceModule::getRightStickHorizontalMovingStrength(void) const
	{
		return this->rightStickMovement.x;
	}

	Ogre::Real InputDeviceModule::getRightStickVerticalMovingStrength(void) const
	{
		return this->rightStickMovement.y;
	}

	InputDeviceModule::JoyStickButton InputDeviceModule::getSinglePressedButton(void) const
	{
		return this->pressedButton;
	}

	void InputDeviceModule::update(Ogre::Real dt)
	{
		OIS::JoyStick* joyStick = NOWA::Core::getSingletonPtr()->getJoyStick();

		if (nullptr != joyStick)
		{
			const OIS::JoyStickState& joystickState = joyStick->getJoyStickState();

			// We receive the full analog range of the axes, and so have to implement our
			// own dead zone.  This is an empirical value, since some joysticks have more
			// jitter or creep around the center point than others.  We'll use 5% of the
			// range as the dead zone, but generally you would want to give the user the
			// option to change this.

			// -1.f for full down to +1.f for full up.
			this->leftStickMovement.y = (Ogre::Real)joystickState.mAxes[0].abs / 32767.0f;
			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[DesignState]: moveHorizontal " + Ogre::StringConverter::toString(moveHorizontal));
			if (Ogre::Math::Abs(this->leftStickMovement.y) < this->joyStickDeadZone)
				this->leftStickMovement.y = 0.0f;

			// Range is -1.f for full left to +1.f for full right
			this->leftStickMovement.x = (Ogre::Real)joystickState.mAxes[1].abs / 32767.0f;
			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[DesignState]: moveVertical " + Ogre::StringConverter::toString(moveVertical));
			if (Ogre::Math::Abs(this->leftStickMovement.x) < this->joyStickDeadZone)
				this->leftStickMovement.x = 0.0f;
			
			// -1.f for full down to +1.f for full up.
			this->rightStickMovement.y = (Ogre::Real)joystickState.mAxes[2].abs / 32767.0f;
			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[DesignState]: moveHorizontal " + Ogre::StringConverter::toString(moveHorizontal));
			if (Ogre::Math::Abs(this->rightStickMovement.y) < this->joyStickDeadZone)
				this->rightStickMovement.y = 0.0f;

			// Range is -1.f for full left to +1.f for full right
			this->rightStickMovement.x = (Ogre::Real)joystickState.mAxes[3].abs / 32767.0f;
			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[DesignState]: moveVertical " + Ogre::StringConverter::toString(moveVertical));
			if (Ogre::Math::Abs(this->rightStickMovement.x) < this->joyStickDeadZone)
				this->rightStickMovement.x = 0.0f;

			JoyStickButton foundButton = JoyStickButton::BUTTON_NONE;
			unsigned short j = 0;
			for (std::vector<bool>::const_iterator i = joyStick->getJoyStickState().mButtons.begin(), e = joyStick->getJoyStickState().mButtons.end(); i != e; ++i)
			{
				if (*i == true)
				{
					foundButton = static_cast<JoyStickButton>(*i);
					break;
				}
				j++;
			}

			this->pressedButton = foundButton;

			// Here invent function: getPressedKey() in which a bitmask of keys is returned in a form of a vector to check which keys had been pressed at once
			

			// Here also pov
		}
	}

} // namespace end
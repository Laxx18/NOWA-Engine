#ifndef INPUT_DEVICE_CORE_H
#define INPUT_DEVICE_CORE_H

#include <map>
#include "defines.h"

#include <OISMouse.h>
#include <OISKeyboard.h>
#include <OISJoyStick.h>
#include <OISInputManager.h>

#include <OgreWindow.h>

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

		InputDeviceModule* getMainKeyboardInputDeviceModule(void) const;

		InputDeviceModule* getKeyboardInputDeviceModule(unsigned long id) const;

		InputDeviceModule* getJoystickInputDeviceModule(unsigned long id) const;

		std::vector<InputDeviceModule*> getKeyboardInputDeviceModules(void) const;

		std::vector<InputDeviceModule*> getJoystickInputDeviceModules(void) const;

		bool isSelectDown(void) const;
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

		OIS::Mouse* mouse;
		OIS::Keyboard* keyboard;
		OIS::InputManager* inputSystem;

		std::vector<OIS::JoyStick*> joysticks;

		std::map<Ogre::String, OIS::KeyListener*> keyListeners;
		std::map<Ogre::String, OIS::MouseListener*> mouseListeners;
		std::map<Ogre::String, OIS::JoyStickListener*> joystickListeners;

		InputDeviceModule* mainInputDeviceModule;
		std::vector<InputDeviceModule*> keyboardInputDeviceModules;
		std::vector<InputDeviceModule*> joystickInputDeviceModules;

		JoyStickConfig joyStickConfig;

		unsigned short joystickIndex;
		bool listenerAboutToBeRemoved;

		bool bSelectDown;

		void addDevice(const Ogre::String& deviceName, bool isKeyboard, OIS::Object* deviceObject);
	};

}; //namespace end

template<> NOWA::InputDeviceCore* Ogre::Singleton<NOWA::InputDeviceCore>::msSingleton = 0;

#endif

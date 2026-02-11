#include "NOWAPrecompiled.h"
#include "InputDeviceCore.h"
#include "main/Core.h"
#include "modules/InputDeviceModule.h"
#include "modules/GraphicsModule.h"
#include "console/LuaConsole.h"
#include "MyGUI_InputManager.h"

namespace
{
	/*
	 * Short answer: MyGUI does not derive capital letters from Shift for you
	 * it inserts whatever you pass as the text argument of injectKeyPress.
	 * In your path, you forward OIS events across threads and you never adjust text based on modifier state.
	 * If OIS delivers 'a' (or even 0) for KC_A while Shift is down, MyGUI will still insert lowercase (or nothing).
	 * Because you enqueue to the render thread, even the order of “Shift down” → “A down” can be one frame apart, so relying on MyGUI’s internal modifier state is fragile.
	 * So this is the fix:
	 */
	MyGUI::Char applyModifiers(MyGUI::Char ch, const OIS::Keyboard* kb)
	{
		if (!kb) return ch;

		const bool lshift = kb->isKeyDown(OIS::KC_LSHIFT);
		const bool rshift = kb->isKeyDown(OIS::KC_RSHIFT);
		const bool shift = lshift || rshift;

		// OIS doesn’t expose CapsLock state as a toggle easily.
		// If you maintain it elsewhere, fold it in here:
		const bool caps = false; // or your own tracked caps toggle

		if (ch >= 'a' && ch <= 'z')
		{
			if (shift ^ caps) ch = static_cast<MyGUI::Char>(ch - 'a' + 'A');
			return ch;
		}

		// Optional: handle common US-DE layout shifted symbols for number row.
		if (shift)
		{
			switch (ch)
			{
			case '1': ch = '!'; break;
			case '2': ch = '@'; break; // adjust for your layout if not US
			case '3': ch = '#'; break;
			case '4': ch = '$'; break;
			case '5': ch = '%'; break;
			case '6': ch = '^'; break;
			case '7': ch = '&'; break;
			case '8': ch = '*'; break;
			case '9': ch = '('; break;
			case '0': ch = ')'; break;
			case '\'': ch = '"'; break;
			case ';': ch = ':'; break;
			case ',': ch = '<'; break;
			case '.': ch = '>'; break;
			case '/': ch = '?'; break;
			case '-': ch = '_'; break;
			case '=': ch = '+'; break;
			case '[': ch = '{'; break;
			case ']': ch = '}'; break;
			case '\\': ch = '|'; break;
			}
		}
		return ch;
	}

	void normalizeMouseEventCoords(const OIS::MouseEvent& e)
	{
		HWND hwnd = nullptr;
		NOWA::Core::getSingletonPtr()->getOgreRenderWindow()->getCustomAttribute("WINDOW", &hwnd);

		if (!hwnd)
			return;

		POINT p;
		if (!GetCursorPos(&p))
			return;

		if (!ScreenToClient(hwnd, &p))
			return;

		// We want everyone to see client coords in e.state.X.abs / Y.abs,
		// so we overwrite them (OIS is not particularly const-correct anyway).
		auto& state = const_cast<OIS::MouseState&>(e.state);
		state.X.abs = p.x;
		state.Y.abs = p.y;
	}

}

namespace NOWA
{
	InputDeviceCore::InputDeviceCore()
		: mouse(nullptr),
		keyboard(nullptr),
		inputSystem(nullptr),
		mainInputDeviceModule(nullptr),
		joystickIndex(0),
		bSelectDown(false),
		keyDispatchDepth(0),
		mouseDispatchDepth(0),
		joystickDispatchDepth(0)
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
			if (nullptr != this->mainInputDeviceModule)
			{
				delete this->mainInputDeviceModule;
				this->mainInputDeviceModule = nullptr;
			}

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

			if (this->keyboard)
			{
				this->inputSystem->destroyInputObject(this->keyboard);
				this->keyboard = 0;
			}

			if (this->joysticks.size() > 0)
			{
				auto&itJoystick = this->joysticks.begin();
				auto& itJoystickEnd = this->joysticks.end();
				for (; itJoystick != itJoystickEnd; ++itJoystick)
				{
					this->inputSystem->destroyInputObject(*itJoystick);
				}

				this->joysticks.clear();
			}

			// If you use OIS1.0RC1 or above, uncomment this line
			// and comment the line below it
			this->inputSystem->destroyInputSystem(this->inputSystem);
			//this->inputSystem->destroyInputSystem();
			this->inputSystem = nullptr;

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
			paramList.insert({ "WINDOW", windowHndStr.str() });

#if defined OIS_LINUX_PLATFORM
			paramList.insert({ "x11_mouse_grab", "false" });
			paramList.insert({ "x11_keyboard_grab", "false" });
#endif

			this->inputSystem = OIS::InputManager::createInputSystem(paramList);

			try
			{
				int keyboardIndex = 0;
				while (true)
				{
					if (0 == keyboardIndex)
					{
						this->keyboard = static_cast<OIS::Keyboard*>(this->inputSystem->createInputObject(OIS::OISKeyboard, true));
						this->mainInputDeviceModule = new InputDeviceModule("MainKeyboard", true, this->keyboard);
						this->keyboard->setEventCallback(this);

						std::string deviceName = keyboard->vendor();
						if (deviceName.empty())
						{
							deviceName = "Keyboard" + std::to_string(keyboardIndex);
						}
						this->addDevice(deviceName, true, keyboard);
					}
					else
					{
						OIS::Keyboard* keyboard = static_cast<OIS::Keyboard*>(this->inputSystem->createInputObject(OIS::OISKeyboard, true));
						this->keyboard->setEventCallback(this);

						// Use the joystick's vendor name as its unique identifier
						std::string deviceName = keyboard->vendor();
						if (deviceName.empty())
						{
							deviceName = "Keyboard" + std::to_string(keyboardIndex);
						}
						else
						{
							deviceName += "_" + Ogre::StringConverter::toString(keyboardIndex);
						}
						this->addDevice(deviceName, true, keyboard);
					}
					++keyboardIndex;
				}
			}
			catch (...)
			{
				// No more keyboards available
			}

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

			try
			{
				int joystickIndex = 0;
				while (true)
				{
					OIS::JoyStick* joystick = static_cast<OIS::JoyStick*>(this->inputSystem->createInputObject(OIS::OISJoyStick, true));
					this->joysticks.push_back(joystick);

					// Use the joystick's vendor name as its unique identifier
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
					// Attention: static_cast is new, is it correct casted?
					this->joyStickConfig.deadZone = static_cast<int>(this->joyStickConfig.max * 0.1f);

					this->joyStickConfig.yaw = 0;
					this->joyStickConfig.pitch = 0;

					this->joyStickConfig.swivel = 0;

					joystick->setVector3Sensitivity(0.001f);

					this->addDevice(deviceName, false, joystick);

					++joystickIndex;
				}
			}
			catch (...)
			{
				// No more joysticks available
			}
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

		if (false == this->joysticks.empty())
		{
			this->joysticks[this->joystickIndex]->capture();
			this->joystickIndex = (this->joystickIndex + 1) % this->getJoyStickCount();
		}

		for (size_t i = 0; i < this->getJoyStickCount(); i++)
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

		if (NOWA_K_SELECT == tempKeyEvent.key)
		{
			this->bSelectDown = true;
		}

		MyGUI::Char finalChar = applyModifiers(static_cast<MyGUI::Char>(tempKeyEvent.text), this->keyboard);

		ENQUEUE_RENDER_COMMAND_MULTI("InputDeviceCore::keyPressed", _2(tempKeyEvent, finalChar),
		{
			MyGUI::InputManager::getInstancePtr()->injectKeyPress(MyGUI::KeyCode::Enum(tempKeyEvent.key), finalChar);
		});

		//// Do not react on input if there is any interaction with a mygui widget
		//MyGUI::Widget* widget = MyGUI::InputManager::getInstance().getMouseFocusWidget();
		//if (nullptr != widget)
		//{
		//	return false;
		//}

		this->keyDispatchDepth++;

		for (size_t i = this->keyListenerStack.size(); i-- > 0; )
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

		ENQUEUE_RENDER_COMMAND_MULTI("InputDeviceCore::keyReleased", _1(e),
		{
			MyGUI::InputManager::getInstancePtr()->injectKeyRelease(MyGUI::KeyCode::Enum(e.key));
		});

		// Do not react on input if there is any interaction with a mygui widget
		/*MyGUI::Widget* widget = MyGUI::InputManager::getInstance().getMouseFocusWidget();
		if (nullptr != widget)
		{
			return false;
		}*/

		this->keyDispatchDepth++;

		for (size_t i = this->keyListenerStack.size(); i-- > 0; )
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
		// Normalize OIS coords once:
		normalizeMouseEventCoords(e);

		int mX = e.state.X.abs;
		int mY = e.state.Y.abs;
		int mZ = e.state.Z.abs;

		ENQUEUE_RENDER_COMMAND_MULTI("InputDeviceCore::mouseMoved", _3(mX, mY, mZ),
			{
				if (auto* inputMgr = MyGUI::InputManager::getInstancePtr())
					inputMgr->injectMouseMove(mX, mY, mZ);
			});

		this->mouseDispatchDepth++;

		for (size_t i = this->mouseListenerStack.size(); i-- > 0; )
		{
			OIS::MouseListener* listener = this->mouseListenerStack[i].second;
			if (nullptr == listener)
			{
				continue;
			}

			if (!listener->mouseMoved(e))
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

	bool InputDeviceCore::mousePressed(const OIS::MouseEvent& e, OIS::MouseButtonID id)
	{
		// Normalize coordinates first
		normalizeMouseEventCoords(e);

		this->bSelectDown = this->getKeyboard()->isKeyDown(NOWA_K_SELECT);

		int mX = e.state.X.abs;
		int mY = e.state.Y.abs;

		ENQUEUE_RENDER_COMMAND_MULTI("InputDeviceCore::mousePressed", _3(mX, mY, id),
			{
				if (auto* inputMgr = MyGUI::InputManager::getInstancePtr())
					inputMgr->injectMousePress(mX, mY, MyGUI::MouseButton::Enum(id));
			});

		this->mouseDispatchDepth++;

		for (size_t i = this->mouseListenerStack.size(); i-- > 0; )
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
		// Normalize coordinates first
		normalizeMouseEventCoords(e);

		int mX = e.state.X.abs;
		int mY = e.state.Y.abs;

		ENQUEUE_RENDER_COMMAND_MULTI("InputDeviceCore::mouseReleased", _3(mX, mY, id),
			{
				if (auto* inputMgr = MyGUI::InputManager::getInstancePtr())
					inputMgr->injectMouseRelease(mX, mY, MyGUI::MouseButton::Enum(id));
			});

		this->mouseDispatchDepth++;

		for (size_t i = this->mouseListenerStack.size(); i-- > 0; )
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
		this->joystickDispatchDepth++;

		for (size_t i = this->joystickListenerStack.size(); i-- > 0; )
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
		this->joystickDispatchDepth++;

		for (size_t i = this->joystickListenerStack.size(); i-- > 0; )
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

		for (size_t i = this->joystickListenerStack.size(); i-- > 0; )
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
		this->joystickDispatchDepth++;

		for (size_t i = this->joystickListenerStack.size(); i-- > 0; )
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

		for (size_t i = this->joystickListenerStack.size(); i-- > 0; )
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
			this->keyboardInputDeviceModules.push_back(new InputDeviceModule(deviceName, isKeyboard, deviceObject));
		}
		else
		{
			this->joystickInputDeviceModules.push_back(new InputDeviceModule(deviceName, isKeyboard, deviceObject));
		}
	}

	unsigned short InputDeviceCore::getJoyStickCount(void) const
	{ 
		return static_cast<unsigned short>(this->joysticks.size());
	};

	std::vector<OIS::JoyStick*> InputDeviceCore::getJoySticks(void) const
	{
		return this->joysticks;
	}

	InputDeviceModule* InputDeviceCore::assignDevice(const Ogre::String& deviceName, unsigned long id)
	{
		for (auto& module : this->keyboardInputDeviceModules)
		{
			if ((module->getDeviceName() == deviceName && false == module->isOccupied()) || module->getOccupiedId() == id)
			{
				module->setOccupiedId(id);
				return module;
			}
		}
		for (auto& module : this->joystickInputDeviceModules)
		{
			if ((module->getDeviceName() == deviceName && false == module->isOccupied()) || module->getOccupiedId() == id)
			{
				module->setOccupiedId(id);
				return module;
			}
		}
		return nullptr;
	}

	void InputDeviceCore::releaseDevice(unsigned long id)
	{
		for (auto& module : this->keyboardInputDeviceModules)
		{
			if (module->getOccupiedId() == id && module->isOccupied())
			{
				module->releaseOccupation();
				break;
			}
		}
		for (auto& module : this->joystickInputDeviceModules)
		{
			if (module->getOccupiedId() == id && module->isOccupied())
			{
				module->releaseOccupation();
				break;
			}
		}
	}

	InputDeviceModule* InputDeviceCore::getMainKeyboardInputDeviceModule(void) const
	{
		return this->mainInputDeviceModule;
	}

	InputDeviceModule* InputDeviceCore::getKeyboardInputDeviceModule(unsigned long id) const
	{
		for (auto& module : this->keyboardInputDeviceModules)
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
		for (auto& module : this->joystickInputDeviceModules)
		{
			if (id == module->getOccupiedId())
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

} // namespace end
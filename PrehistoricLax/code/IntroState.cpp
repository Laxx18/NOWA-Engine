#include "NOWAPrecompiled.h"
#include "IntroState.h"

IntroState::IntroState()
	: NOWA::AppState()
{
	// Do not initialize here anything! Do it in the 'start' function, because due to performance reasons, an AppState will not be destroyed when changed to another one.
}

void IntroState::enter(void)
{
	this->currentSceneName = "PrehistoricLax/Intro/Intro.scene";

	NOWA::FaderProcess::showBlackScreenImmediate();

	NOWA::AppState::enter();
}

void IntroState::start(const NOWA::SceneParameter& sceneParameter)
{
	NOWA::ProcessManager::getInstance()->attachProcess(NOWA::ProcessPtr(new NOWA::FaderProcess(NOWA::FaderProcess::FadeOperation::FADE_IN, 2.0f, NOWA::Interpolator::EaseInCubic)));

	// World loaded finished. Get scene manager, camera, ogrenewt etc. from scene parameter here for custom functionality
}

void IntroState::exit(void)
{
	NOWA::AppState::exit();
}

void IntroState::update(Ogre::Real dt)
{
	NOWA::AppState::update(dt);
}

void IntroState::renderUpdate(Ogre::Real dt)
{
	this->processUnbufferedKeyInput(dt);
	this->processUnbufferedMouseInput(dt);

	NOWA::AppState::renderUpdate(dt);
}

bool IntroState::keyPressed(const OIS::KeyEvent &keyEventRef)
{
	NOWA::Core::getSingletonPtr()->keyPressed(keyEventRef);

	// Prevent scene manipulation, when user does something in GUI
	/*if (nullptr != MyGUI::InputManager::getInstance().getMouseFocusWidget())
	{
		return true;
	}*/
	if (NOWA::LuaConsole::getSingletonPtr() && NOWA::LuaConsole::getSingletonPtr()->isVisible())
	{
		return true;
	}

	switch (keyEventRef.key)
	{
		case OIS::KC_ESCAPE:
		{
			MyGUI::PointerManager::getInstancePtr()->setVisible(true);
			this->bQuit = true;
			return true;
		}
		case OIS::KC_TAB:
		{
			if (GetAsyncKeyState(KF_ALTDOWN))
			{
				NOWA::Core::getSingletonPtr()->moveWindowToTaskbar();
			}
			return true;
		}
	}

	return true;
}

bool IntroState::keyReleased(const OIS::KeyEvent &keyEventRef)
{
	NOWA::Core::getSingletonPtr()->keyReleased(keyEventRef);

	if (NOWA::LuaConsole::getSingletonPtr() && NOWA::LuaConsole::getSingletonPtr()->isVisible())
	{
		return true;
	}
	return true;
}

void IntroState::processUnbufferedKeyInput(Ogre::Real dt)
{
	const auto& keyboard = NOWA::InputDeviceCore::getSingletonPtr()->getKeyboard();

	if (keyboard->isKeyDown(OIS::KC_F4) && keyboard->isKeyDown(OIS::KC_LMENU))
	{
		bQuit = true;
		return;
	}

	if (NOWA::LuaConsole::getSingletonPtr() && NOWA::LuaConsole::getSingletonPtr()->isVisible())
	{
		return;
	}
}

void IntroState::processUnbufferedMouseInput(Ogre::Real dt)
{

}

bool IntroState::mouseMoved(const OIS::MouseEvent& evt)
{
	NOWA::Core::getSingletonPtr()->mouseMoved(evt);
	
	return true;
}

bool IntroState::mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
{
	NOWA::Core::getSingletonPtr()->mousePressed(evt, id);

	return true;
}

bool IntroState::mouseReleased(const OIS::MouseEvent &evt, OIS::MouseButtonID id)
{
	NOWA::Core::getSingletonPtr()->mouseReleased(evt, id);
	

	return true;
}

bool IntroState::axisMoved(const OIS::JoyStickEvent& evt, int axis)
{

	return true;
}

bool IntroState::buttonPressed(const OIS::JoyStickEvent& evt, int button)
{
	return true;
}

bool IntroState::buttonReleased(const OIS::JoyStickEvent& evt, int button)
{
	return true;
}
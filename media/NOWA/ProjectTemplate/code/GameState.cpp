#include "NOWAPrecompiled.h"
#include "GameState.h"

GameState::GameState()
	: NOWA::AppState()
{
	// Do not initialize here anything! Do it in the 'start' function, because due to performance reasons, an AppState will not be destroyed when changed to another one.
}

void GameState::enter(void)
{
	this->currentWorldName = "ProjectTemplate/World1";
	
	NOWA::AppState::enter();

	NOWA::ProcessManager::getInstance()->attachProcess(NOWA::ProcessPtr(new NOWA::FaderProcess(NOWA::FaderProcess::FadeOperation::FADE_IN, 1.0f)));
}

void GameState::start(const NOWA::SceneParameter& sceneParameter)
{
	NOWA::ProcessManager::getInstance()->attachProcess(NOWA::ProcessPtr(new NOWA::FaderProcess(NOWA::FaderProcess::FadeOperation::FADE_IN, 1.0f)));

	// World loaded finished. Get scene manager, camera, ogrenewt etc. from scene parameter here for custom functionality
}

void GameState::notifyMessageBoxEnd(MyGUI::Message* _sender, MyGUI::MessageBoxStyle _result)
{
	if (_result == MyGUI::MessageBoxStyle::Yes)
	{
		this->bQuit = true;
	}
	else
	{
		this->canUpdate = true;
	}
}

void GameState::update(Ogre::Real dt)
{
	this->processUnbufferedKeyInput(dt);
	this->processUnbufferedMouseInput(dt);

	NOWA::AppState::update(dt);
}

void GameState::lateUpdate(Ogre::Real dt)
{
	NOWA::AppState::lateUpdate(dt);
	
	const OIS::MouseState& ms = NOWA::InputDeviceCore::getSingletonPtr()->getMouse()->getMouseState();
		
	NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->moveCamera(dt);
	
	if (ms.buttonDown(OIS::MB_Right))
	{
		NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->rotateCamera(dt, false);
	}
}

bool GameState::keyPressed(const OIS::KeyEvent &keyEventRef)
{
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
			this->canUpdate = false;
			// Stop simulation if simulating
			// Ask user whether he really wants to quit the application
			MyGUI::Message* messageBox = MyGUI::Message::createMessageBox("Menue", MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{Quit_Application}"),
				MyGUI::MessageBoxStyle::IconWarning | MyGUI::MessageBoxStyle::Yes | MyGUI::MessageBoxStyle::No, "Popup", true);

			messageBox->eventMessageBoxResult += MyGUI::newDelegate(this, &GameState::notifyMessageBoxEnd);
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

bool GameState::keyReleased(const OIS::KeyEvent &keyEventRef)
{
	if (NOWA::LuaConsole::getSingletonPtr() && NOWA::LuaConsole::getSingletonPtr()->isVisible())
	{
		return true;
	}
	return true;
}

void GameState::processUnbufferedKeyInput(Ogre::Real dt)
{
	if (NOWA::LuaConsole::getSingletonPtr() && NOWA::LuaConsole::getSingletonPtr()->isVisible())
	{
		return;
	}
}

void GameState::processUnbufferedMouseInput(Ogre::Real dt)
{

}

bool GameState::mouseMoved(const OIS::MouseEvent& evt)
{
	return true;
}

bool GameState::mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
{
	return true;
}

bool GameState::mouseReleased(const OIS::MouseEvent &evt, OIS::MouseButtonID id)
{
	return true;
}

bool GameState::axisMoved(const OIS::JoyStickEvent& evt, int axis)
{

	return true;
}

bool GameState::buttonPressed(const OIS::JoyStickEvent& evt, int button)
{
	return true;
}

bool GameState::buttonReleased(const OIS::JoyStickEvent& evt, int button)
{
	return true;
}
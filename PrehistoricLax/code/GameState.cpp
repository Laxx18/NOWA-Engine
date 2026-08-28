#include "NOWAPrecompiled.h"
#include "GameState.h"

GameState::GameState()
	: NOWA::AppState()
{
	// Do not initialize here anything! Do it in the 'start' function, because due to performance reasons, an AppState will not be destroyed when changed to another one.
}

void GameState::enter(void)
{
	this->currentSceneName = "PrehistoricLax/Level1/Level1.scene";
	
	NOWA::AppState::enter();
	
	NOWA::FaderProcess::showBlackScreenImmediate();
}

void GameState::start(const NOWA::SceneParameter& sceneParameter)
{
	// Scene loaded finished. Get scene manager, camera, ogrenewt etc. from scene parameter here for custom functionality
	// and are handed back here via sceneParameter - do not create them again.
	this->sceneManager = sceneParameter.sceneManager;
	this->camera = sceneParameter.mainCamera;

	NOWA::ProcessManager::getInstance()->attachProcess(NOWA::ProcessPtr(new NOWA::FaderProcess(NOWA::FaderProcess::FadeOperation::FADE_IN, 2.0f, NOWA::Interpolator::EaseInCubic)));
}

void GameState::exit(void)
{
	NOWA::AppState::exit();
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
	NOWA::AppState::update(dt);
}

void GameState::renderUpdate(Ogre::Real dt)
{
	this->processUnbufferedKeyInput(dt);
	this->processUnbufferedMouseInput(dt);

	NOWA::AppState::renderUpdate(dt);
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
			NOWA::AppState* menuState = this->findByName("MenuState");
			if (nullptr != menuState)
			{
				this->pushAppState(menuState);
			}
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
#include "NOWAPrecompiled.h"
#include "LoadMenuState.h"

LoadMenuState::LoadMenuState()
	: NOWA::AppState()
{
	// Do not initialize here anything! Do it in the 'start' function, because due to performance reasons, an AppState will not be destroyed when changed to another one.
}

void LoadMenuState::enter(void)
{
	this->currentSceneName = "PrehistoricLax/LoadMenu/LoadMenu.scene";

	NOWA::AppState::enter();

	NOWA::FaderProcess::showBlackScreenImmediate();
}

void LoadMenuState::start(const NOWA::SceneParameter& sceneParameter)
{
	NOWA::ProcessManager::getInstance()->attachProcess(NOWA::ProcessPtr(new NOWA::FaderProcess(NOWA::FaderProcess::FadeOperation::FADE_IN, 2.0f, NOWA::Interpolator::EaseInCubic)));

	// World loaded finished. Get scene manager, camera, ogrenewt etc. from scene parameter here for custom functionality
}

void LoadMenuState::exit(void)
{
	NOWA::AppState::exit();
}

void LoadMenuState::notifyMessageBoxEnd(MyGUI::Message* _sender, MyGUI::MessageBoxStyle _result)
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

void LoadMenuState::update(Ogre::Real dt)
{
	NOWA::AppState::update(dt);
}

void LoadMenuState::renderUpdate(Ogre::Real dt)
{
	this->processUnbufferedKeyInput(dt);
	this->processUnbufferedMouseInput(dt);

	NOWA::AppState::renderUpdate(dt);
}

bool LoadMenuState::keyPressed(const OIS::KeyEvent& keyEventRef)
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
		NOWA::GraphicsModule::RenderCommand cmd = [this]()
		{
			MyGUI::PointerManager::getInstancePtr()->setVisible(true);

			this->canUpdate = false;
			// Stop simulation if simulating
			// Ask user whether he really wants to quit the application
			MyGUI::Message* messageBox = MyGUI::Message::createMessageBox("Menue", MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{Quit_Application}"),
				MyGUI::MessageBoxStyle::IconWarning | MyGUI::MessageBoxStyle::Yes | MyGUI::MessageBoxStyle::No, "Popup", true);

			messageBox->eventMessageBoxResult += MyGUI::newDelegate(this, &LoadMenuState::notifyMessageBoxEnd);
		};
		NOWA::GraphicsModule::getInstance()->enqueue(std::move(cmd), "LoadMenuState::keyPressed Escape");
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

bool LoadMenuState::keyReleased(const OIS::KeyEvent& keyEventRef)
{
	NOWA::Core::getSingletonPtr()->keyReleased(keyEventRef);

	if (NOWA::LuaConsole::getSingletonPtr() && NOWA::LuaConsole::getSingletonPtr()->isVisible())
	{
		return true;
	}
	return true;
}

void LoadMenuState::processUnbufferedKeyInput(Ogre::Real dt)
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

void LoadMenuState::processUnbufferedMouseInput(Ogre::Real dt)
{

}

bool LoadMenuState::mouseMoved(const OIS::MouseEvent& evt)
{
	NOWA::Core::getSingletonPtr()->mouseMoved(evt);

	return true;
}

bool LoadMenuState::mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
{
	NOWA::Core::getSingletonPtr()->mousePressed(evt, id);

	return true;
}

bool LoadMenuState::mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
{
	NOWA::Core::getSingletonPtr()->mouseReleased(evt, id);


	return true;
}

bool LoadMenuState::axisMoved(const OIS::JoyStickEvent& evt, int axis)
{

	return true;
}

bool LoadMenuState::buttonPressed(const OIS::JoyStickEvent& evt, int button)
{
	return true;
}

bool LoadMenuState::buttonReleased(const OIS::JoyStickEvent& evt, int button)
{
	return true;
}
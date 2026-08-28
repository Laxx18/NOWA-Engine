#include "NOWAPrecompiled.h"
#include "MenuState.h"

MenuState::MenuState()
	: NOWA::AppState()
{
	// Do not initialize here anything! Do it in the 'start' function, because due to performance reasons, an AppState will not be destroyed when changed to another one.
}

void MenuState::enter(void)
{
	this->currentSceneName = "PrehistoricLax/Menu/Menu.scene";

	NOWA::AppState::enter();

	NOWA::FaderProcess::showBlackScreenImmediate();
}

void MenuState::start(const NOWA::SceneParameter& sceneParameter)
{
	NOWA::ProcessManager::getInstance()->attachProcess(NOWA::ProcessPtr(new NOWA::FaderProcess(NOWA::FaderProcess::FadeOperation::FADE_IN, 2.0f, NOWA::Interpolator::EaseInCubic)));

	// sceneManager and camera were already created by AppState::initializeModules(true, true)
	// and are handed back here via sceneParameter - do not create them again.
	this->sceneManager = sceneParameter.sceneManager;
	this->camera = sceneParameter.mainCamera;

    this->menuMusic = NOWA::OgreALModule::getInstance()->getSound(this->sceneManager, "MainGameObject_Menu - Mossgate Sanctuary.ogg");
	if (nullptr == this->menuMusic)
	{
		this->menuMusic = NOWA::OgreALModule::getInstance()->createSound(this->sceneManager, "MainGameObject_Menu - Mossgate Sanctuary.ogg", "Menu - Mossgate Sanctuary.ogg", true, true);
		this->menuMusic->play();
	}
}

void MenuState::exit(void)
{
	this->menuMusic->stop();
	NOWA::OgreALModule::getInstance()->deleteSound(this->sceneManager, this->menuMusic);

	NOWA::AppState::exit();
}

void MenuState::notifyMessageBoxEnd(MyGUI::Message* _sender, MyGUI::MessageBoxStyle _result)
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

void MenuState::update(Ogre::Real dt)
{
	NOWA::AppState::update(dt);
}

void MenuState::renderUpdate(Ogre::Real dt)
{
	this->processUnbufferedKeyInput(dt);
	this->processUnbufferedMouseInput(dt);

	NOWA::AppState::renderUpdate(dt);
}

bool MenuState::keyPressed(const OIS::KeyEvent& keyEventRef)
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

			messageBox->eventMessageBoxResult += MyGUI::newDelegate(this, &MenuState::notifyMessageBoxEnd);
		};
		NOWA::GraphicsModule::getInstance()->enqueue(std::move(cmd), "MenuState::keyPressed Escape");
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

bool MenuState::keyReleased(const OIS::KeyEvent& keyEventRef)
{
	NOWA::Core::getSingletonPtr()->keyReleased(keyEventRef);

	if (NOWA::LuaConsole::getSingletonPtr() && NOWA::LuaConsole::getSingletonPtr()->isVisible())
	{
		return true;
	}
	return true;
}

void MenuState::processUnbufferedKeyInput(Ogre::Real dt)
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

void MenuState::processUnbufferedMouseInput(Ogre::Real dt)
{

}

bool MenuState::mouseMoved(const OIS::MouseEvent& evt)
{
	NOWA::Core::getSingletonPtr()->mouseMoved(evt);

	return true;
}

bool MenuState::mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
{
	NOWA::Core::getSingletonPtr()->mousePressed(evt, id);

	return true;
}

bool MenuState::mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
{
	NOWA::Core::getSingletonPtr()->mouseReleased(evt, id);


	return true;
}

bool MenuState::axisMoved(const OIS::JoyStickEvent& evt, int axis)
{

	return true;
}

bool MenuState::buttonPressed(const OIS::JoyStickEvent& evt, int button)
{
	return true;
}

bool MenuState::buttonReleased(const OIS::JoyStickEvent& evt, int button)
{
	return true;
}
#include "NOWAPrecompiled.h"
#include "NOWA.h"
#include "Api.h"
#include <codecvt>

/////////////////////////////////////////////Helper Functions///////////////////////////////////

std::wstring utf8ToUtf16(const std::string& utf8Str)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
	return conv.from_bytes(utf8Str);
}

std::string utf16ToUtf8(const std::wstring& utf16Str)
{
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> conv;
	return conv.to_bytes(utf16Str);
}

///////////////////////////////////////////////Base SimulationState//////////////////////////////

class SimulationState : public NOWA::AppState
{
public:
	typedef void (__cdecl* OnHandleSceneLoadedFuncPtr) (bool sceneChanged, NOWA::ProjectParameter projectParameter);
	// Weitere callback für key, mousemove etc.!?
public:
	DECLARE_APPSTATE_CLASS(SimulationState)

	SimulationState()
		: NOWA::AppState()
	{

	}

	void enter(void)
	{
		this->ogreNewt = nullptr;
		this->canUpdate = true;
		this->onHandleSceneLoadedFuncPtr = nullptr;
		// React when scene has been loaded to get data
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &SimulationState::handleSceneLoaded), NOWA::EventDataSceneLoaded::getStaticEventType());

		this->createScene();
	}

	void exit(void)
	{
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &SimulationState::handleSceneLoaded), NOWA::EventDataSceneLoaded::getStaticEventType());
		NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->stop();
		this->canUpdate = false;
		this->destroyModules();
	}

	void createScene(void)
	{
		this->initializeModules(true, true);

		// Attention: Load scene is loaded at an different frame, so after that camera, etc is not available, use EventDataSceneChanged event to get data
		Ogre::String projectName = NOWA::Core::getSingletonPtr()->getProjectName();
		Ogre::String sceneName = NOWA::Core::getSingletonPtr()->getSceneName();

		if (false == projectName.empty())
		{
			NOWA::AppStateManager::getSingletonPtr()->getGameProgressModule()->loadScene(projectName + "/" + sceneName + "/" + sceneName);
		}

		// MyGUI::PointerManager::getInstancePtr()->setVisible(false);

		NOWA::ProcessManager::getInstance()->attachProcess(NOWA::ProcessPtr(new NOWA::FaderProcess(NOWA::FaderProcess::FadeOperation::FADE_IN, 10.0f)));
	}

	void handleSceneLoaded(NOWA::EventDataPtr eventData)
	{
		boost::shared_ptr<NOWA::EventDataSceneLoaded> castEventData = boost::static_pointer_cast<NOWA::EventDataSceneLoaded>(eventData);

		if (nullptr != this->onHandleSceneLoadedFuncPtr)
		{
			this->onHandleSceneLoadedFuncPtr(castEventData->getSceneChanged(), castEventData->getProjectParameter());
		}

		// setSceneCallback(castEventData->getSceneChanged(), castEventData->getProjectParameter());

		//if (true == castEventData->getSceneChanged())
		//{
		//	this->camera = NOWA::GameProgressModule::instance()->get()->getData().mainCamera;
		//	// Start game
		//	NOWA::GameObjectController::instance()->get()->start();
		//	// Set the start position for the player
		//	NOWA::GameProgressModule::instance()->get()->determinePlayerStartLocation(castEventData->getProjectParameter().sceneName);
		//	// Activate player controller, so that user can move player
		//	NOWA::GameObjectPtr player = NOWA::GameObjectController::instance()->get()->getGameObjectFromName(NOWA::GameProgressModule::instance()->get()->getPlayerName());
		//	if (nullptr != player)
		//	{
		//		NOWA::GameObjectController::instance()->get()->activatePlayerController(true, player->getId(), true);
		//	}
		//}
	}

	void notifyMessageBoxEnd(MyGUI::Message* _sender, MyGUI::MessageBoxStyle _result)
	{
		if (_result == MyGUI::MessageBoxStyle::Yes)
		{
			this->bQuit = true;
		}
		else
		{
			this->canUpdate = true;
			// MyGUI::PointerManager::getInstancePtr()->setVisible(false);
		}
	}

	// void setupMyGUIWidgets(void)
	// {
	// Attention: Is this required?
		// Load images for buttons
		// MyGUI::ResourceManager::getInstance().load("ButtonsImages.xml");
		// Load skin for +- expand button for each panel cell
		// MyGUI::ResourceManager::getInstance().load("ButtonExpandSkin.xml");
	// }

	void update(Ogre::Real dt)
	{
		this->processUnbufferedKeyInput(dt);
		this->processUnbufferedMouseInput(dt);

		if (true == canUpdate)
		{
			if (false == NOWA::AppStateManager::getSingletonPtr()->getIsStalled() && false == this->gameProgressModule->isSceneLoading())
			{
				this->ogreNewtModule->update(dt);
				// Update the GameObjects
				this->gameObjectController->update(dt);

				this->ogreRecastModule->update(dt);
				// PlayerControllerComponents are using this directly for smoke effect etc.
				this->particleUniverseModule->update(dt);
			}
		}

		if (true == this->bQuit)
		{
			this->shutdown();
		}
	}

	bool keyPressed(const OIS::KeyEvent& keyEventRef)
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
			// MyGUI::PointerManager::getInstancePtr()->setVisible(true);
			this->canUpdate = false;
			// Stop simulation if simulating
			// Ask user whether he really wants to quit the application
			MyGUI::Message* messageBox = MyGUI::Message::createMessageBox("Menue", MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{Quit_Application}"),
				MyGUI::MessageBoxStyle::IconWarning | MyGUI::MessageBoxStyle::Yes | MyGUI::MessageBoxStyle::No, "Popup", true);

			messageBox->eventMessageBoxResult += MyGUI::newDelegate(this, &SimulationState::notifyMessageBoxEnd);
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

	bool keyReleased(const OIS::KeyEvent& keyEventRef)
	{
		NOWA::Core::getSingletonPtr()->keyReleased(keyEventRef);

		if (NOWA::LuaConsole::getSingletonPtr() && NOWA::LuaConsole::getSingletonPtr()->isVisible())
		{
			return true;
		}
		return true;
	}

	void processUnbufferedKeyInput(Ogre::Real dt)
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

	void processUnbufferedMouseInput(Ogre::Real dt)
	{

	}

	bool mouseMoved(const OIS::MouseEvent& evt)
	{
		NOWA::Core::getSingletonPtr()->mouseMoved(evt);

		return true;
	}

	bool mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
	{
		NOWA::Core::getSingletonPtr()->mousePressed(evt, id);

		return true;
	}

	bool mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
	{
		NOWA::Core::getSingletonPtr()->mouseReleased(evt, id);


		return true;
	}

	bool axisMoved(const OIS::JoyStickEvent& evt, int axis)
	{

		return true;
	}

	bool buttonPressed(const OIS::JoyStickEvent& evt, int button)
	{
		return true;
	}

	bool buttonReleased(const OIS::JoyStickEvent& evt, int button)
	{
		return true;
	}

	void setSceneLoadedCallback(OnHandleSceneLoadedFuncPtr onHandleSceneLoadedFuncPtr)
	{
		this->onHandleSceneLoadedFuncPtr = onHandleSceneLoadedFuncPtr;
	}
private:
	OgreNewt::World* ogreNewt;
	bool canUpdate;
	OnHandleSceneLoadedFuncPtr onHandleSceneLoadedFuncPtr;
};

namespace NOWA
{
	int init(const wchar_t* applicationName, const wchar_t* graphicsConfigurationName, const wchar_t* applicationConfigurationName, wchar_t* startProjectName, int iconId)
	{
		// Create framework singleton
		new NOWA::Core();

		// Second the application state manager
		new NOWA::AppStateManager();
		// Note: This are special case singletons, since they are created and deleted manually, to avoid side effects when e.g. core would be else maybe deleted before AppStateManager

		// Feed core configuration with some information
		NOWA::CoreConfiguration coreConfiguration;
		coreConfiguration.graphicsConfigName = utf16ToUtf8(graphicsConfigurationName); // transmitted via args in main, since may variate when used network scenario
		coreConfiguration.wndTitle = utf16ToUtf8(applicationName);
		coreConfiguration.resourcesName = utf16ToUtf8(applicationConfigurationName);
		if (nullptr != startProjectName)
		{
			coreConfiguration.startProjectName = utf16ToUtf8(startProjectName);
		}
		// Initialize ogre
		bool isInitializedCorrectly = NOWA::Core::getSingletonPtr()->initialize(coreConfiguration);
		if (false == isInitializedCorrectly)
		{
			return 0;
		}

		// Add application icon
		if (iconId != 0)
		{
			NOWA::Core::getSingletonPtr()->createApplicationIcon(iconId);
		}

		Ogre::LogManager::getSingletonPtr()->logMessage(coreConfiguration.wndTitle + " initialized!");

		// Create states
		// Attention: always use NOWA:: + name of the state
		SimulationState::create(NOWA::AppStateManager::getSingletonPtr(), "SimulationState", "SimulationState");

		// Lets start with the Game
		NOWA::AppStateManager::getSingletonPtr()->start("SimulationState", false, NOWA::AppStateManager::RESTRICTED_INTERPOLATED);

		return 1;
	}

	int exit(void)
	{
		// Application state manager must always be deleted before core will be deleted in order to avoid ugly side effects
		if (AppStateManager::getSingletonPtr())
		{
			// delete core singleton
			delete AppStateManager::getSingletonPtr();
		}
		if (Core::getSingletonPtr())
		{
			// delete core singleton
			delete Core::getSingletonPtr();
		}

		return 1;
	}

	void setSceneLoadedCallback(HandleSceneLoaded callback)
	{
		SimulationState* simulationState = static_cast<SimulationState*>(NOWA::AppStateManager::getSingletonPtr()->findByName("SimulationState"));
		simulationState->setSceneLoadedCallback((SimulationState::OnHandleSceneLoadedFuncPtr) callback);
	}

}; // namespace end

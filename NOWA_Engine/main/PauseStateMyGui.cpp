#include "NOWAPrecompiled.h"
#include "PauseStateMyGui.h"
#include "Core.h"
#include "InputDeviceCore.h"
#include "utilities/FaderProcess.h"
#include "modules/WorkspaceModule.h"
#include "main/AppStateManager.h"

namespace NOWA
{

	PauseStateMyGui::PauseStateMyGui()
		: AppState()
	{
	}

	void PauseStateMyGui::enter(void)
	{
		NOWA::AppState::enter();

		ProcessManager::getInstance()->attachProcess(ProcessPtr(new FaderProcess(FaderProcess::FadeOperation::FADE_IN, 1.0f)));
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PauseStateMyGui] Entering...");

		// Create scene manager
		this->sceneManager = Core::getSingletonPtr()->getOgreRoot()->createSceneManager(Ogre::ST_GENERIC, 1, "PauseStateMyGui");
		// this->sceneManager->setAmbientLight(Ogre::ColourValue(0.7f, 0.7f, 0.7f));
		this->sceneManager->addRenderQueueListener(Core::getSingletonPtr()->getOverlaySystem());
		this->sceneManager->getRenderQueue()->setSortRenderQueue(Ogre::v1::OverlayManager::getSingleton().mDefaultRenderQueueId, Ogre::RenderQueue::StableSort);

		// Create camera
		this->camera = this->sceneManager->createCamera("PauseCamera");
		this->camera->setPosition(Ogre::Vector3(0, 25, -50));
		this->camera->lookAt(Ogre::Vector3(0, 0, 0));
		this->camera->setNearClipDistance(1);
		this->camera->setAutoAspectRatio(true);

		WorkspaceModule::getInstance()->setPrimaryWorkspace(this->sceneManager, this->camera, nullptr);

		this->initializeModules(false, false);

		// Create Gui
		Core::getSingletonPtr()->setSceneManagerForMyGuiPlatform(this->sceneManager);
		MyGUI::Gui::getInstancePtr()->createWidget<MyGUI::ImageBox>("RotatingSkin",
			MyGUI::IntCoord(0, 0, Core::getSingletonPtr()->getOgreRenderWindow()->getWidth(), Core::getSingletonPtr()->getOgreRenderWindow()->getHeight()),
			MyGUI::Align::Default, "Overlapped", "Background")->setImageTexture("BackgroundShadeBlue.png");

		this->widgets = MyGUI::LayoutManager::getInstancePtr()->loadLayout("PauseState.layout");
		MyGUI::FloatPoint windowPosition;
		windowPosition.left = 0.4f;
		windowPosition.top = 0.4f;
		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Window>("PauseStateMyGui")->setRealPosition(windowPosition);
		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("ContinueButton")->eventMouseButtonClick += MyGUI::newDelegate(this, &PauseStateMyGui::buttonHit);
		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("BackToMenueButton")->eventMouseButtonClick += MyGUI::newDelegate(this, &PauseStateMyGui::buttonHit);
		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("QuitButton")->eventMouseButtonClick += MyGUI::newDelegate(this, &PauseStateMyGui::buttonHit);

		this->bQuit = false;

		this->createScene();
	}

	void PauseStateMyGui::createScene(void)
	{

	}

	void PauseStateMyGui::exit(void)
	{
		NOWA::AppState::exit();

		ProcessManager::getInstance()->attachProcess(ProcessPtr(new FaderProcess(FaderProcess::FadeOperation::FADE_OUT, 1.0f)));
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PauseStateMyGui] Leaving...");

		MyGUI::Gui::getInstancePtr()->destroyWidget(MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::ImageBox>("Background"));
		MyGUI::LayoutManager::getInstancePtr()->unloadLayout(this->widgets);
		
		this->destroyModules();
	}

	bool PauseStateMyGui::keyPressed(const OIS::KeyEvent &keyEventRef)
	{
		NOWA::Core::getSingletonPtr()->keyPressed(keyEventRef);
		if (InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(OIS::KC_P))
		{
			this->bQuit = true;
			return true;
		}
		return true;
	}

	bool PauseStateMyGui::keyReleased(const OIS::KeyEvent &keyEventRef)
	{
		NOWA::Core::getSingletonPtr()->keyReleased(keyEventRef);
		return true;
	}

	bool PauseStateMyGui::mouseMoved(const OIS::MouseEvent& evt)
	{
		NOWA::Core::getSingletonPtr()->mouseMoved(evt);
		return true;
	}

	bool PauseStateMyGui::mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
	{
		NOWA::Core::getSingletonPtr()->mousePressed(evt, id);
		return true;
	}

	bool PauseStateMyGui::mouseReleased(const OIS::MouseEvent &evt, OIS::MouseButtonID id)
	{
		NOWA::Core::getSingletonPtr()->mouseReleased(evt, id);
		return true;
	}

	bool PauseStateMyGui::axisMoved(const OIS::JoyStickEvent& evt, int axis)
	{
		return true;
	}

	bool PauseStateMyGui::buttonPressed(const OIS::JoyStickEvent& evt, int button)
	{
		return true;
	}

	bool PauseStateMyGui::buttonReleased(const OIS::JoyStickEvent& evt, int button)
	{
		return true;
	}

	void PauseStateMyGui::update(Ogre::Real dt)
	{
		if (this->bQuit == true)
		{
			this->popAppState();
			return;
		}
	}

	void PauseStateMyGui::buttonHit(MyGUI::WidgetPtr _sender)
	{
		if ("ContinueButton" == _sender->getName())
		{
			this->bQuit = true;
		}
		else if ("BackToMenueButton" == _sender->getName())
		{
			this->popAllAndPushAppState(this->findByName("MenuStateMyGui"));
		}
		else if ("QuitButton" == _sender->getName())
		{
			MyGUI::Message* messageBox = MyGUI::Message::createMessageBox(Ogre::String("Pause"), MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{Quit_Application}"),
				MyGUI::MessageBoxStyle::IconWarning | MyGUI::MessageBoxStyle::Yes | MyGUI::MessageBoxStyle::No, "Popup", true);

			messageBox->eventMessageBoxResult += MyGUI::newDelegate(this, &PauseStateMyGui::notifyMessageBoxEnd);
		}
	}

	void PauseStateMyGui::notifyMessageBoxEnd(MyGUI::Message* _sender, MyGUI::MessageBoxStyle _result)
	{
		if (_result == MyGUI::MessageBoxStyle::Yes)
		{
			shutdown();
		}
	}

}; // namespace end

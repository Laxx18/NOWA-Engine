#include "main/PauseState.h"
#include "main/WiiManager.h"
#include "main/Core.h"

namespace NOWA
{

	PauseState::PauseState()
		: AppState()
	{
	}

	void PauseState::enter(void)
	{
		FaderPlugin::getSingletonPtr()->startFadeIn();
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PauseState] Entering...");

		// Create scene manager
		this->sceneManager = Core::getSingletonPtr()->getOgreRoot()->createSceneManager(Ogre::ST_GENERIC, "PauseSceneManager");
		this->sceneManager->setAmbientLight(Ogre::ColourValue(0.7f, 0.7f, 0.7f));
		this->sceneManager->addRenderQueueListener(Core::getSingletonPtr()->getOverlaySystem());

		// Create camera
		this->camera = this->sceneManager->createCamera("PauseCamera");
		this->camera->setPosition(Ogre::Vector3(0, 25, -50));
		this->camera->lookAt(Ogre::Vector3(0, 0, 0));
		this->camera->setNearClipDistance(1);

		this->camera->setAutoAspectRatio(true);

		Core::getSingletonPtr()->getOgreViewport()->setCamera(this->camera);

		// Create GUI
		Core::getSingletonPtr()->getTrayManager()->destroyAllWidgets();
		Core::getSingletonPtr()->getTrayManager()->showCursor();
		Core::getSingletonPtr()->getTrayManager()->createButton(OgreBites::TL_CENTER, "BackToGameButton", "Continue", 250);
		Core::getSingletonPtr()->getTrayManager()->createButton(OgreBites::TL_CENTER, "BackToMenuButton", "Back to menue", 250);
		Core::getSingletonPtr()->getTrayManager()->createButton(OgreBites::TL_CENTER, "ExitButton", "Exit application", 250);
		Core::getSingletonPtr()->getTrayManager()->createLabel(OgreBites::TL_TOP, "PauseLabel", "Pause", 250);

		this->bQuit = false;

		this->createScene();
	}

	void PauseState::createScene(void)
	{

	}

	void PauseState::exit(void)
	{
		FaderPlugin::getSingletonPtr()->startFadeOut(1.0f);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PauseState] Leaving...");

		this->sceneManager->removeRenderQueueListener(Core::getSingletonPtr()->getOverlaySystem());
		Core::getSingletonPtr()->deleteGui();

		//Wii Infrared camera should not be usable
		Core::getSingletonPtr()->getWiiManager()->useInfrateCameraDirectly(false);

		Core::getSingletonPtr()->destroyScene(this->sceneManager);
	}

	bool PauseState::keyPressed(const OIS::KeyEvent &keyEventRef)
	{
		if (Core::getSingletonPtr()->getKeyboard()->isKeyDown(OIS::KC_P))
		{
			this->bQuit = true;
			return true;
		}

		Core::getSingletonPtr()->keyPressed(keyEventRef);
		return true;
	}

	bool PauseState::keyReleased(const OIS::KeyEvent &keyEventRef)
	{
		Core::getSingletonPtr()->keyReleased(keyEventRef);
		return true;
	}

	bool PauseState::mouseMoved(const OIS::MouseEvent &evt)
	{
		if (Core::getSingletonPtr()->getTrayManager()->injectMouseMove(evt))
		{
			return true;
		}
		return true;
	}

	bool PauseState::mousePressed(const OIS::MouseEvent &evt, OIS::MouseButtonID id)
	{
		if (Core::getSingletonPtr()->getTrayManager()->injectMouseDown(evt, id))
		{
			return true;
		}
		return true;
	}

	bool PauseState::mouseReleased(const OIS::MouseEvent &evt, OIS::MouseButtonID id)
	{
		if (Core::getSingletonPtr()->getTrayManager()->injectMouseUp(evt, id))
		{
			return true;
		}
		return true;
	}

	void PauseState::update(Ogre::Real dt)
	{
		if (this->bQuit == true)
		{
			this->popAppState();
			return;
		}
	}

	void PauseState::buttonHit(OgreBites::Button *pButton)
	{
		if (pButton->getName() == "ExitButton")
		{
			Core::getSingletonPtr()->getTrayManager()->showYesNoDialog("Simulation", "Back to Windows?");
		}
		else if (pButton->getName() == "BackToGameButton")
		{
			this->bQuit = true;
		}
		else if (pButton->getName() == "BackToMenuButton")
		{
			Core::getSingletonPtr()->getTrayManager()->showYesNoDialog("Simulation", "Back to menue? In this case the current state will be lost.");
			this->popAllAndPushAppState(this->findByName("MenuState"));
		}
	}


	void PauseState::yesNoDialogClosed(const Ogre::DisplayString &question, bool yesHit)
	{
		if (yesHit == true)
			shutdown();
		else
			Core::getSingletonPtr()->getTrayManager()->closeDialog();
	}

}; // namespace end

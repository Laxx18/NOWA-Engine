#include "NOWAPrecompiled.h"
#include "IntroState.h"
#include "Core.h"
#include "utilities/FaderProcess.h"
#include "modules/WorkspaceModule.h"
#include "AppStateManager.h"

namespace NOWA
{

	IntroState::IntroState()
		: AppState()
	{
	}

	void IntroState::enter(void)
	{
		NOWA::AppState::enter();

		ProcessManager::getInstance()->attachProcess(ProcessPtr(new FaderProcess(FaderProcess::FadeOperation::FADE_IN, 1.0f)));
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[IntroState] Entering...");

		ENQUEUE_RENDER_COMMAND_WAIT("IntroState::enter",
		{
			// Create scene manager
			this->sceneManager = Core::getSingletonPtr()->getOgreRoot()->createSceneManager(Ogre::ST_GENERIC, 1, "IntroState");
			// this->sceneManager->setAmbientLight(Ogre::ColourValue(0.7f, 0.7f, 0.7f));
			this->sceneManager->addRenderQueueListener(Core::getSingletonPtr()->getOverlaySystem());
			this->sceneManager->getRenderQueue()->setSortRenderQueue(Ogre::v1::OverlayManager::getSingleton().mDefaultRenderQueueId, Ogre::RenderQueue::StableSort);

			// Create camera
			this->camera = this->sceneManager->createCamera("IntroCamera");
			this->camera->setPosition(Ogre::Vector3(0, 25, -50));
			this->camera->lookAt(Ogre::Vector3(0, 0, 0));
			this->camera->setNearClipDistance(1);
			this->camera->setAutoAspectRatio(true);

			WorkspaceModule::getInstance()->setPrimaryWorkspace(this->sceneManager, this->camera, nullptr);

			this->initializeModules(false, false);

			this->bQuit = false;
			this->introTimeDt = 10.0f;

			this->createScene();
		});
	}

	void IntroState::createScene(void)
	{

	}

	void IntroState::exit(void)
	{
		NOWA::AppState::exit();

		ProcessManager::getInstance()->attachProcess(ProcessPtr(new FaderProcess(FaderProcess::FadeOperation::FADE_OUT, 1.0f)));
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[IntroState] Leaving...");

		this->destroyModules();
	}

	bool IntroState::keyPressed(const OIS::KeyEvent &keyEventRef)
	{
		NOWA::Core::getSingletonPtr()->keyPressed(keyEventRef);
		return true;
	}

	bool IntroState::keyReleased(const OIS::KeyEvent &keyEventRef)
	{
		NOWA::Core::getSingletonPtr()->keyReleased(keyEventRef);
		return true;
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

	void IntroState::update(Ogre::Real dt)
	{
		if (this->introTimeDt >= 0.0f)
		{
			this->introTimeDt -= dt;
		}
		else
		{
			this->popAppState();
		}
	}

}; // namespace end

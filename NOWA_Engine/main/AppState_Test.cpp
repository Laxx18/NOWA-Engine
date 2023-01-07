#include "NOWAPrecompiled.h"
#include "AppState.h"
#include "Core.h"
#include "utilities/FaderProcess.h"

#include "AppStateManager.h"
#include "modules/ParticleUniverseModule.h"
#include "modules/OgreALModule.h"
#include "modules/OgreNewtModule.h"
#include "camera/CameraManager.h"
#include "modules/WorkspaceModule.h"
#include "modules/GameProgressModule.h"
#include "gameobject/GameObjectController.h"
#include "modules/LuaScriptModule.h"

namespace NOWA
{

	AppState::AppState()
		:	bQuit(false),
		sceneManager(nullptr),
		camera(nullptr)
	{
		
	}

	void AppState::destroy(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AppState] Destroy");
		delete this;
	}

	bool AppState::pause(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AppState] Pausing State...");
		return true;
	}

	void AppState::resume(void)
	{
		ProcessManager::getInstance()->attachProcess(ProcessPtr(new FaderProcess(FaderProcess::FadeOperation::FADE_IN, 2.5f)));
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AppState] Resuming State...");

// Attention: is this correct?
		Core::getSingletonPtr()->setSceneManagerForMyGuiPlatform(this->sceneManager);
		this->bQuit = false;
	}

	void AppState::update(Ogre::Real dt)
	{
		
	}

	void AppState::lateUpdate(Ogre::Real dt)
	{

	}

	AppState* AppState::findByName(Ogre::String stateName)
	{
		return this->appStateManager->findByName(stateName);
	}

	AppState* AppState::getNextState(AppState* currentAppState)
	{
		return this->appStateManager->getNextState(currentAppState);
	}

	void AppState::changeAppState(AppState* state)
	{
		this->bQuit = false;
		this->appStateManager->changeAppState(state);
	}

	bool AppState::pushAppState(AppState* state)
	{
		this->bQuit = false;
		return this->appStateManager->pushAppState(state);
	}

	void AppState::popAppState(void)
	{
		this->appStateManager->popAppState();
	}

	void AppState::shutdown(void)
	{
		this->appStateManager->shutdown();
	}

	void AppState::popAllAndPushAppState(AppState* state)
	{
		this->bQuit = false;
		this->appStateManager->popAllAndPushAppState(state);
	}

	Ogre::String AppState::getName(void) const
	{
		return this->appStateName;
	}

	AppStateManager* AppState::getAppStateManager(void) const
	{
		return this->appStateManager;
	}

	void AppState::initializeModules(void)
	{
		bool canInitialize = true;
		if (nullptr == this->sceneManager)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AppState]: Could not initialize modules, because the scene manager is null");
			canInitialize = false;
		}
		
		if (nullptr == this->camera)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AppState]: Could not initialize modules, because the camera is null");
			canInitialize = false;
		}

		if (true == canInitialize)
		{
			this->sceneManager->addRenderQueueListener(Core::getSingletonPtr()->getOverlaySystem());
			GameProgressModule::getInstance()->initialize(this->sceneManager);
			ParticleUniverseModule::getInstance()->initParticleUniverseModule(this->sceneManager);
			WorkspaceModule::getInstance()->createBasicPbsWorkspace(this->sceneManager, this->camera);
			OgreALModule::getInstance()->createOgreAL(this->sceneManager);
		}
	}

	void AppState::destroyModules(void)
	{
		bool canDestroy = true;
		if (nullptr == this->sceneManager)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AppState]: Could not destroy modules, because the scene manager is null");
			canDestroy = false;
		}
		
		if (true == canDestroy)
		{
			this->sceneManager->removeRenderQueueListener(Core::getSingletonPtr()->getOverlaySystem());

			WorkspaceModule::getInstance()->destroyContent();

			CameraManager::getInstance()->destroyContent();

			GameObjectController::getInstance()->destroyContent();

			ParticleUniverseModule::getInstance()->destroyContent();

			GameProgressModule::getInstance()->destroyContent();

			LuaScriptModule::getInstance()->destroyAllScripts();
			// Must be destroyed after game object controller destroyes all game objects, but before the scene manager destroys the scene
			OgreALModule::getInstance()->destroyContent();
			Core::getSingletonPtr()->destroyScene(this->sceneManager);

			OgreNewtModule::getInstance()->destroyContent();
		}
	}

}; // namespace end
#include "NOWAPrecompiled.h"
#include "AppState.h"
#include "Core.h"

#include "utilities/FaderProcess.h"
#include "modules/LuaScriptApi.h"
#include "modules/InputDeviceModule.h"
#include "modules/OgreALModule.h"
#include "main/AppStateManager.h"
#include "gameobject/WorkspaceComponents.h"

namespace NOWA
{
	AppState::AppState()
		: sceneManager(nullptr),
		camera(nullptr),
		ogreNewt(nullptr),
		bQuit(false),
		canUpdate(false),
		gameObjectController(nullptr),
		gameProgressModule(nullptr),
		rakNetModule(nullptr),
		miniMapModule(nullptr),
		ogreNewtModule(nullptr),
		meshDecalGeneratorModule(nullptr),
		cameraManager(nullptr),
		ogreRecastModule(nullptr),
		particleUniverseModule(nullptr),
		luaScriptModule(nullptr),
		eventManager(nullptr),
		scriptEventManager(nullptr),
		gpuParticlesModule(nullptr),
		hasStarted(false),
		workspaceBaseComponent(nullptr)
	{
		
	}

	// static function for macro
	void AppState::create(AppStateListener* appStateManager, const Ogre::String name, const Ogre::String nextAppStateName)
	{
	}

	void AppState::enter(void)
	{
		this->hasStarted = true;
		this->canUpdate = true;

		this->initializeModules(true, true);
		// Note: All listener must be added after the modules are initialized
		// React when world has been loaded to get data
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &AppState::handleWorldLoaded), NOWA::EventDataWorldLoaded::getStaticEventType());

		// Attention: Load world is loaded at an different frame, so after that camera, etc is not available, use EventDataWorldChanged event to get data
		if (false == this->currentWorldName.empty())
		{
			NOWA::AppStateManager::getSingletonPtr()->getGameProgressModule()->loadWorld(this->currentWorldName);
		}
		else
		{
			// If no world name specified, just call start and set default parameter
			NOWA::SceneParameter sceneParameter;
			sceneParameter.sceneManager = this->sceneManager;
			sceneParameter.dotSceneImportModule = nullptr;
			sceneParameter.mainCamera = this->camera;
			// User must created manually if want to use
			sceneParameter.ogreNewt = nullptr;

			this->start(sceneParameter);
		}
	}

	void AppState::exit(void)
	{
		this->canUpdate = false;
		this->hasStarted = false;

		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &AppState::handleWorldLoaded), NOWA::EventDataWorldLoaded::getStaticEventType());
		NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->stop();
		this->destroyModules();
	}

	void AppState::handleWorldLoaded(NOWA::EventDataPtr eventData)
	{
		boost::shared_ptr<NOWA::EventDataWorldLoaded> castEventData = boost::static_pointer_cast<NOWA::EventDataWorldLoaded>(eventData);

		this->sceneManager = castEventData->getSceneParameter().sceneManager;
		this->camera = castEventData->getSceneParameter().mainCamera;
		this->ogreNewt = castEventData->getSceneParameter().ogreNewt;

		// Start game
		NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->start();
		// Set the start position for the player
		NOWA::AppStateManager::getSingletonPtr()->getGameProgressModule()->determinePlayerStartLocation(castEventData->getProjectParameter().sceneName);
		// Activate player controller, so that user can move player
		NOWA::GameObjectPtr player = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromName(NOWA::AppStateManager::getSingletonPtr()->getGameProgressModule()->getPlayerName());
		if (nullptr != player)
		{
			NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->activatePlayerController(true, player->getId(), true);
		}

		this->start(castEventData->getSceneParameter());
	}

	void AppState::destroy(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AppState] Destroy");
		delete this;
	}

	bool AppState::pause(void)
	{
		this->gameObjectController->pause();
		// Remember the active workspace
		this->workspaceBaseComponent = WorkspaceModule::getInstance()->getPrimaryWorkspaceComponent();
		this->canUpdate = false;
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AppState] Pausing State...");
		return true;
	}

	void AppState::resume(void)
	{
		// If there was an active workspace, set the workspace in order to continue rendering
		if (nullptr != this->workspaceBaseComponent)
		{
			this->workspaceBaseComponent->createWorkspace();
			WorkspaceModule::getInstance()->setPrimaryWorkspace(this->sceneManager, AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera(), this->workspaceBaseComponent);
		}
		else
		{
			Ogre::String message = "[WorkspaceModule] Error: Cannot resume AppState there is no active workspace set!";
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, message);
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, message + "\n", "NOWA");
		}
		this->canUpdate = true;
		ProcessManager::getInstance()->attachProcess(ProcessPtr(new FaderProcess(FaderProcess::FadeOperation::FADE_IN, 2.5f)));
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AppState] Resuming State...");

// Attention: is this correct?
		Core::getSingletonPtr()->setSceneManagerForMyGuiPlatform(this->sceneManager);
		OgreALModule::getInstance()->init(this->sceneManager);

		this->bQuit = false;
		this->gameObjectController->resume();
	}

	void AppState::update(Ogre::Real dt)
	{
		if (true == this->canUpdate)
		{
			this->updateModules(dt);
		}

		if (true == this->bQuit)
		{
			this->shutdown();
		}
	}

	void AppState::lateUpdate(Ogre::Real dt)
	{
		if (true == canUpdate)
		{
			this->lateUpdateModules(dt);
		}
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

	AppStateListener* AppState::getAppStateManager(void) const
	{
		return this->appStateManager;
	}

	void AppState::initializeModules(bool initSceneManager, bool initCamera)
	{
		bool canInitialize = true;

		if (nullptr == this->gameObjectController)
		{
			this->gameObjectController = new GameObjectController(this->appStateName);
			this->gameProgressModule = new GameProgressModule(this->appStateName);
			this->rakNetModule = new RakNetModule(this->appStateName);
			this->miniMapModule = new MiniMapModule(this->appStateName);
			this->ogreNewtModule = new OgreNewtModule(this->appStateName);
			this->meshDecalGeneratorModule = new MeshDecalGeneratorModule(this->appStateName);
			this->cameraManager = new CameraManager(this->appStateName);
			this->ogreRecastModule = new OgreRecastModule(this->appStateName);
			this->particleUniverseModule = new ParticleUniverseModule(this->appStateName);
			this->luaScriptModule = new LuaScriptModule(this->appStateName);
			this->eventManager = new EventManager(this->appStateName);
			this->scriptEventManager = new ScriptEventManager(this->appStateName);
			this->gpuParticlesModule = new GpuParticlesModule(this->appStateName);
		}

		if (true == initSceneManager)
		{
			// http://www.ogre3d.org/2016/01/01/ogre-progress-report-december-2015
			// Longer loading times, but faster, test it
			// Ogre::v1::Mesh::msOptimizeForShadowMapping = true;
			// constexpr size_t numThreads = 1;
#if OGRE_DEBUG_MODE
		//Debugging multithreaded code is a PITA, disable it.
			const size_t numThreads = 1;
#else
		//getNumLogicalCores() may return 0 if couldn't detect
			const size_t numThreads = std::max<size_t>(1, Ogre::PlatformInformation::getNumLogicalCores());
#endif
			// Loads textures in background in multiple threads
			Ogre::TextureGpuManager* hlmsTextureManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();
			hlmsTextureManager->setMultiLoadPool(numThreads);

			// Create the SceneManager, in this case a generic one
			this->sceneManager = NOWA::Core::getSingletonPtr()->getOgreRoot()->createSceneManager(Ogre::ST_GENERIC, numThreads, this->appStateName + "_SceneManager");
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[AppState]: Using " + Ogre::StringConverter::toString(numThreads) + " threads.");
		}

		if (true == initCamera)
		{
			this->camera = this->sceneManager->createCamera(this->appStateName + "_Camera");
			NOWA::Core::getSingletonPtr()->setMenuSettingsForCamera(this->camera);
			this->camera->setFOVy(Ogre::Degree(90.0f));
			this->camera->setNearClipDistance(0.1f);
			this->camera->setFarClipDistance(500.0f);
			this->camera->setQueryFlags(0 << 0);
			this->camera->setPosition(0.0f, 5.0f, -2.0f);

			// this->cameraManager->destroyContent();
			this->cameraManager->init("CameraManager1", this->camera);
			this->cameraManager->addCameraBehavior(new NOWA::BaseCamera());
			this->cameraManager->setActiveCameraBehavior(NOWA::BaseCamera::BehaviorType());
		}

		if (nullptr == this->sceneManager)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AppState]: Could not initialize modules, because the scene manager is null.");
			canInitialize = false;
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[AppState] Could not initialize modules, because the scene manager is null.\n", "NOWA");
		}
		
		if (nullptr == this->camera)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AppState]: Could not initialize modules, because the camera is null.");
			canInitialize = false;
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[AppState] Could not initialize modules, because the camera is null.\n", "NOWA");
		}
			
		if (true == canInitialize)
		{
			Core::getSingletonPtr()->setSceneManagerForMyGuiPlatform(this->sceneManager);

			Ogre::RenderQueue::RqSortMode sortMode = Ogre::RenderQueue::RqSortMode::StableSort;

			// http://www.ogre3d.org/forums/viewtopic.php?f=25&t=88607 important to choose what to render in what render queue!
			// Really important to set the renderqueue mode to 250 (overlay), and then setting the wished manual objects render queue group to that number
			// so that they are always visible

			/*
			RenderQueue ID range [0; 100) & [200; 225) default to FAST (i.e. for v2 objects, like Items)
			RenderQueue ID range [100; 200) & [225; 256) default to V1_FAST (i.e. for v1 objects, like v1::Entity)
			By default new Items and other v2 objects are placed in RenderQueue ID 10
			By default new v1::Entity and other v1 objects are placed in RenderQueue ID 110
			*/

			this->sceneManager->getRenderQueue()->setRenderQueueMode(NOWA::RENDER_QUEUE_V2_MESH, Ogre::RenderQueue::Modes::FAST);
			this->sceneManager->getRenderQueue()->setSortRenderQueue(NOWA::RENDER_QUEUE_V2_MESH, sortMode);

			this->sceneManager->getRenderQueue()->setRenderQueueMode(NOWA::RENDER_QUEUE_DISTORTION, Ogre::RenderQueue::Modes::FAST);

			this->sceneManager->getRenderQueue()->setRenderQueueMode(NOWA::RENDER_QUEUE_V1_MESH, Ogre::RenderQueue::Modes::V1_FAST);
			this->sceneManager->getRenderQueue()->setSortRenderQueue(NOWA::RENDER_QUEUE_V1_MESH, sortMode);

			this->sceneManager->getRenderQueue()->setRenderQueueMode(NOWA::RENDER_QUEUE_PARTICLE_STUFF, Ogre::RenderQueue::Modes::V1_FAST);
			this->sceneManager->getRenderQueue()->setSortRenderQueue(NOWA::RENDER_QUEUE_PARTICLE_STUFF, sortMode);

			this->sceneManager->getRenderQueue()->setRenderQueueMode(NOWA::RENDER_QUEUE_LEGACY, Ogre::RenderQueue::Modes::V1_LEGACY);
			this->sceneManager->getRenderQueue()->setSortRenderQueue(NOWA::RENDER_QUEUE_LEGACY, sortMode);

			this->sceneManager->getRenderQueue()->setRenderQueueMode(NOWA::RENDER_QUEUE_V2_OBJECTS_ALWAYS_IN_FOREGROUND, Ogre::RenderQueue::Modes::FAST);
			this->sceneManager->getRenderQueue()->setSortRenderQueue(NOWA::RENDER_QUEUE_V2_OBJECTS_ALWAYS_IN_FOREGROUND, sortMode);

			this->sceneManager->getRenderQueue()->setRenderQueueMode(NOWA::RENDER_QUEUE_V1_OBJECTS_ALWAYS_IN_FOREGROUND, Ogre::RenderQueue::Modes::V1_FAST);
			this->sceneManager->getRenderQueue()->setSortRenderQueue(NOWA::RENDER_QUEUE_V1_OBJECTS_ALWAYS_IN_FOREGROUND, sortMode);

			this->sceneManager->getRenderQueue()->setRenderQueueMode(NOWA::RENDER_QUEUE_GIZMO, Ogre::RenderQueue::Modes::V1_FAST);
			this->sceneManager->getRenderQueue()->setSortRenderQueue(NOWA::RENDER_QUEUE_GIZMO, sortMode);

			this->sceneManager->getRenderQueue()->setRenderQueueMode(NOWA::RENDER_QUEUE_MAX, Ogre::RenderQueue::V1_FAST);
			this->sceneManager->getRenderQueue()->setSortRenderQueue(NOWA::RENDER_QUEUE_MAX, sortMode);

			
			this->sceneManager->addRenderQueueListener(Core::getSingletonPtr()->getOverlaySystem());
			this->sceneManager->getRenderQueue()->setSortRenderQueue(Ogre::v1::OverlayManager::getSingleton().mDefaultRenderQueueId, Ogre::RenderQueue::StableSort);
			
			// LuaScriptApi::getInstance()->destroyAllScripts();

			// this->gameProgressModule->destroyContent();
			this->gameProgressModule->init(this->sceneManager);
			// this->particleUniverseModule->destroyContent();
			this->particleUniverseModule->init(this->sceneManager);
			// WorkspaceModule::getInstance()->destroyContent();
			// Create dummy workspace, since there is no one yet created
			WorkspaceModule::getInstance()->setPrimaryWorkspace(this->sceneManager, this->camera, nullptr);

			OgreALModule::getInstance()->init(this->sceneManager);

			// TODO: Like in OgreRecastModule: config the other parameters!
			// this->gpuParticlesModule->init(this->sceneManager);

			// Set unused mask for all camera, because log is spammed with exceptions
			/*Ogre::SceneManager::CameraIterator it = this->sceneManager->getCameraIterator();
			while (it.hasMoreElements())
			{
				Ogre::Camera* tempCamera = it.getNext();
				tempCamera->setQueryFlags(Core::getSingletonPtr()->UNUSEDMASK);
			}*/
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

			this->gameObjectController->destroyContent();
			delete this->gameObjectController;
			this->gameObjectController = nullptr;

			this->cameraManager->destroyContent();
			delete this->cameraManager;
			this->cameraManager = nullptr;

			this->ogreRecastModule->destroyContent();
			delete this->ogreRecastModule;
			this->ogreRecastModule = nullptr;

			this->particleUniverseModule->destroyContent();
			delete this->particleUniverseModule;
			this->particleUniverseModule = nullptr;

			this->gameProgressModule->destroyContent();
			delete this->gameProgressModule;
			this->gameProgressModule = nullptr;

			this->miniMapModule->destroyContent();
			delete this->miniMapModule;
			this->miniMapModule = nullptr;

			delete this->meshDecalGeneratorModule;
			this->meshDecalGeneratorModule = nullptr;

			// Destroy all scripts for just this AppState
			this->luaScriptModule->destroyContent();
			delete this->luaScriptModule;
			this->luaScriptModule = nullptr;

			delete this->eventManager;
			this->eventManager = nullptr;

			this->scriptEventManager->destroyContent();
			delete this->scriptEventManager;
			this->scriptEventManager = nullptr;

			this->gpuParticlesModule->destroyContent();
			delete this->gpuParticlesModule;
			this->gpuParticlesModule = nullptr;

			if (nullptr != this->rakNetModule)
			{
				this->rakNetModule->destroyContent();
				delete this->rakNetModule;
				this->rakNetModule = nullptr;
			}

			WorkspaceModule::getInstance()->destroyContent();

			// If another states continues, do not destroy sounds
			if (AppStateManager::getSingletonPtr()->getAppStatesCount() > 1)
				OgreALModule::getInstance()->destroySounds(this->sceneManager);
			else
				OgreALModule::getInstance()->destroyContent();

			Core::getSingletonPtr()->destroyScene(this->sceneManager);

			this->ogreNewtModule->destroyContent();
			delete this->ogreNewtModule;
			this->ogreNewtModule = nullptr;
		}
	}

	void AppState::updateModules(Ogre::Real dt)
	{
		if (false == AppStateManager::getSingletonPtr()->getIsStalled() && false == this->gameProgressModule->isWorldLoading())
		{
			this->ogreNewtModule->update(dt);
			this->ogreRecastModule->update(dt);
			this->particleUniverseModule->update(dt); // PlayerControllerComponents are using this directly for smoke effect etc.
			// Update the GameObjects
			this->gameObjectController->update(dt, false);
		}
	}

	void AppState::lateUpdateModules(Ogre::Real dt)
	{
		if (false == AppStateManager::getSingletonPtr()->getIsStalled() && false == this->gameProgressModule->isWorldLoading())
		{
			// Late update the GameObjects
			this->gameObjectController->lateUpdate(dt, false);
		}
	}

	bool AppState::getHasStarted(void) const
	{
		return this->hasStarted;
	}

}; // namespace end
#include "NOWAPrecompiled.h"
#include "AppState.h"
#include "Core.h"

#include "gameobject/WorkspaceComponents.h"
#include "main/AppStateManager.h"
#include "modules/InputDeviceModule.h"
#include "modules/LuaScriptApi.h"
#include "modules/OgreALModule.h"
#include "utilities/FaderProcess.h"

#include "RenderQueueEnums.h"

namespace NOWA
{
    AppState::AppState() :
        sceneManager(nullptr),
        camera(nullptr),
        ogreNewt(nullptr),
        bQuit(false),
        canUpdate(false),
        gameObjectController(nullptr),
        gameProgressModule(nullptr),
        rakNetModule(nullptr),
        miniMapModule(nullptr),
        ogreNewtModule(nullptr),
        decalsModule(nullptr),
        cameraManager(nullptr),
        ogreRecastModule(nullptr),
        particleFxModule(nullptr),
        luaScriptModule(nullptr),
        eventManager(nullptr),
        scriptEventManager(nullptr),
        hasStarted(false),
        workspaceBaseComponent(nullptr)
    {
    }

    void AppState::startRendering(void)
    {
        // Attention: this is an engine-lifetime call, not a per-state one. It is invoked
        // exactly once, from AppStateManager::internalChangeAppState() with initial == true.
        // The render thread then lives until GraphicsModule::doCleanup() joins it.
        NOWA::GraphicsModule::getInstance()->startRendering();
    }

    void AppState::stopRendering(void)
    {
        // Attention: this kills the render thread for good - it is NOT a pause. Everything
        // that runs afterwards (AppState::exit(), GameObjectController::stop(),
        // destroyModules()) still pushes render commands and waits for them, so calling this
        // before the teardown produces a shutdown deadlock. That is why
        // AppStateManager::shutdown() no longer calls it; the render thread is stopped and
        // joined in GraphicsModule::doCleanup() at the very end of AppStateManager::start().
        //
        // To pause rendering without killing the thread, use bStall / beginShutdownDrain()
        // style flags instead.
        NOWA::GraphicsModule::getInstance()->stopRendering();
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
        // React when scene has been loaded to get data
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &AppState::handleSceneLoaded), NOWA::EventDataSceneLoaded::getStaticEventType());

        // Attention: Load scene is loaded at an different frame, so after that camera, etc is not available, use EventDataSceneChanged event to get data
        if (false == this->currentSceneName.empty())
        {
            NOWA::AppStateManager::getSingletonPtr()->getGameProgressModule()->loadScene(this->currentSceneName);
        }
        else
        {
            // If no scene name specified, just call start and set default parameter
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
        // Guard against a second exit(): destroyModules() has already nulled every module
        // pointer, so running through here again would dereference null.
        if (false == this->hasStarted)
        {
            return;
        }

        this->canUpdate = false;
        this->hasStarted = false;

        // Guard against the shutdown case: the EventManager may already have been
        // destroyed by the time AppState::exit() runs (bShutdown = true in AppStateManager).
        // Calling removeListener on a null pointer crashes inside lock_guard's constructor.
        NOWA::EventManager* eventManager = NOWA::AppStateManager::getSingletonPtr()->getEventManager();
        if (nullptr != eventManager)
        {
            eventManager->removeListener(fastdelegate::MakeDelegate(this, &AppState::handleSceneLoaded), NOWA::EventDataSceneLoaded::getStaticEventType());
        }

        // Same guard as for the other modules below - during shutdown this can be null too.
        auto* gameProgressModule = NOWA::AppStateManager::getSingletonPtr()->getGameProgressModule(this->appStateName);
        if (nullptr != gameProgressModule)
        {
            // Delete all user defined attributes (when lua script has been disconnected and re-connected, this is required)
            gameProgressModule->stop();
        }

        auto* ogreRecastModule = NOWA::AppStateManager::getSingletonPtr()->getOgreRecastModule(this->appStateName);
        if (nullptr != ogreRecastModule)
        {
            ogreRecastModule->stopSimulation();
        }

        auto* gameObjectController = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController();
        if (nullptr != gameObjectController)
        {
            gameObjectController->stop();
        }

        this->destroyModules();
    }

    void AppState::handleSceneLoaded(NOWA::EventDataPtr eventData)
    {
        boost::shared_ptr<NOWA::EventDataSceneLoaded> castEventData = boost::static_pointer_cast<NOWA::EventDataSceneLoaded>(eventData);

        // Event not for this state
        if (castEventData->getSceneParameter().appStateName != this->appStateName)
        {
            return;
        }

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

        // Sets mouse to 0 0 to prevent sudden hover on mygui elements
        InputDeviceCore::getSingletonPtr()->setMousePosition(0, 0);

        this->start(castEventData->getSceneParameter());
    }

        // -----------------------------------------------------------------------------------
    // Reserves a permanent, invisible slot in the ObjectMemoryManager for a high-numbered
    // render queue, so ObjectMemoryManager::getNumRenderQueues() - and therefore
    // Ogre::SceneManager::cullFrustum()'s loop bound - reaches far enough to cover NOWA's
    // particle render queues, regardless of what the scene actually contains.
    //
    // Root cause: cullFrustum() only iterates render queue ids up to the highest slot
    // actually occupied by a real MovableObject, and ParticleSystemManager2::
    // _addToRenderQueue() is called from inside that very loop. RENDER_QUEUE_PARTICLE_STUFF
    // (155) and RENDER_QUEUE_PARTICLE_TRANSPARENT (214) were therefore silently skipped in
    // the standalone game, where the highest occupied slot was 110 (v1 default). The editor
    // only worked by accident: its gizmo geometry at RENDER_QUEUE_V2_OBJECTS_ALWAYS_IN_
    // FOREGROUND (220) is a real Item on a SceneNode and stretched the iterated range.
    // RENDER_QUEUE_GIZMO (252) and RENDER_QUEUE_MAX (254) never do this - they are only ever
    // configured via setRenderQueueMode(), never populated with a MovableObject, and MyGUI
    // draws through its own compositor pass entirely outside the cull path.
    //
    // The anchor MUST be an Item (v2) in a queue whose mode is FAST. Render queue ids in
    // [100; 200) are v1 territory - putting an Item there throws in SubItem::
    // getRenderOperation. Ids configured as PARTICLE_SYSTEM never collect Items at all.
    // RENDER_QUEUE_GIZMO (252) satisfies both constraints and sits above every particle
    // queue in use.
    // -----------------------------------------------------------------------------------
    void AppState::reserveRenderQueueSlots(void)
    {
        if (nullptr == this->sceneManager)
        {
            return;
        }

        // Never reserve twice for the same scene manager - this would leak one Item plus one
        // SceneNode per call, and applyRenderQueueModes() may run more than once.
        if (false == this->renderQueueAnchorItems.empty())
        {
            return;
        }

        const Ogre::SceneMemoryMgrTypes sceneType = Ogre::SCENE_STATIC;

        static const Ogre::uint8 queuesToReserve[] = {NOWA::RENDER_QUEUE_GIZMO};

        for (Ogre::uint8 rqId : queuesToReserve)
        {
            // "Node.mesh" is NOWA's existing empty utility v2 mesh, already used for invisible
            // dummy game objects - no new resource needed.
            Ogre::Item* anchorItem = this->sceneManager->createItem("Node.mesh", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, sceneType);
            anchorItem->setName("RenderQueueAnchor_" + Ogre::StringConverter::toString(rqId));
            anchorItem->setRenderQueueGroup(rqId);
            anchorItem->setVisible(false);
            anchorItem->setQueryFlags(0);
            anchorItem->setCastShadows(false);

            this->sceneManager->getRootSceneNode(sceneType)->createChildSceneNode(sceneType)->attachObject(anchorItem);

            this->renderQueueAnchorItems.emplace_back(anchorItem);
        }
    }

    // -----------------------------------------------------------------------------------
    // Destroys the anchor items created by reserveRenderQueueSlots(), including the scene
    // nodes they were attached to. Must run before the scene manager itself is destroyed.
    // -----------------------------------------------------------------------------------
    void AppState::destroyRenderQueueSlots(void)
    {
        if (nullptr == this->sceneManager)
        {
            this->renderQueueAnchorItems.clear();
            return;
        }

        for (Ogre::Item* anchorItem : this->renderQueueAnchorItems)
        {
            if (nullptr == anchorItem)
            {
                continue;
            }

            Ogre::SceneNode* anchorNode = anchorItem->getParentSceneNode();
            if (nullptr != anchorNode)
            {
                anchorNode->detachObject(anchorItem);
                this->sceneManager->destroySceneNode(anchorNode);
            }

            this->sceneManager->destroyItem(anchorItem);
        }

        this->renderQueueAnchorItems.clear();
    }

    void AppState::destroy(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AppState] Destroy");
        delete this;
    }

    bool AppState::pause(void)
    {
        if (nullptr != this->gameObjectController)
        {
            this->gameObjectController->pause();
        }
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
            // Attention: do NOT throw here. pause() caches whatever WorkspaceModule reported as
            // primary workspace at that moment. If this state was pushed away before its scene
            // had finished loading, or if it simply has no workspace component, the cached
            // pointer is null - and killing the application on the way back out of a pause menu
            // is far worse than continuing without restoring a workspace.
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AppState] Warning: Cannot restore a workspace on resume, because none was active when this state was paused. App state: " + this->appStateName);
        }

        this->canUpdate = true;
        ProcessManager::getInstance()->attachProcess(ProcessPtr(new FaderProcess(FaderProcess::FadeOperation::FADE_IN, 2.5f)));
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AppState] Resuming State...");

        Core::getSingletonPtr()->setSceneManagerForMyGuiPlatform(this->sceneManager);
        OgreALModule::getInstance()->init(this->sceneManager);

        this->bQuit = false;

        if (nullptr != this->gameObjectController)
        {
            this->gameObjectController->resume();
        }
    }

    void AppState::update(Ogre::Real dt)
    {
        if (true == this->canUpdate)
        {
            if (false == AppStateManager::getSingletonPtr()->bStall && false == this->gameProgressModule->bSceneLoading)
            {
                bool isSimulating = this->gameObjectController->getIsSimulating();

                if (true == isSimulating)
                {
                    this->ogreRecastModule->update(dt);
                    this->particleFxModule->update(dt);
                }

                // Update the GameObjects
                this->gameObjectController->update(dt);

                if (true == isSimulating)
                {
                    this->ogreNewtModule->update(dt);
                    this->cameraManager->moveCamera(dt);
                }
            }
        }

        if (true == this->bQuit)
        {
            this->shutdown();
        }
    }

    void AppState::renderUpdate(Ogre::Real dt)
    {
        NOWA::InputDeviceCore::getSingletonPtr()->getMainKeyboardInputDeviceModule()->update(dt);

        const OIS::MouseState& ms = NOWA::InputDeviceCore::getSingletonPtr()->getMouse()->getMouseState();

        if (ms.buttonDown(OIS::MB_Right))
        {
            NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->rotateCamera(dt, false);
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
            this->decalsModule = new DecalsModule(this->appStateName);
            this->cameraManager = new CameraManager(this->appStateName);
            this->ogreRecastModule = new OgreRecastModule(this->appStateName);
            this->particleFxModule = new ParticleFxModule(this->appStateName);
            this->luaScriptModule = new LuaScriptModule(this->appStateName);
            this->eventManager = new EventManager(this->appStateName);
            this->scriptEventManager = new ScriptEventManager(this->appStateName);
        }

        if (true == initSceneManager)
        {
            size_t numThreads = 1;
#ifdef _DEBUG
            // Debugging multithreaded code is a PITA, disable it.
            numThreads = 1;
#else
            // getNumLogicalCores() may return 0 if couldn't detect
            numThreads = std::max<size_t>(1, Ogre::PlatformInformation::getNumLogicalCores());
#endif

            NOWA::GraphicsModule::RenderCommand renderCommand = [this, numThreads]()
            {
                // Loads textures in background in multiple threads
                Ogre::TextureGpuManager* hlmsTextureManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();
                hlmsTextureManager->setMultiLoadPool(numThreads);

                // Create the SceneManager, in this case a generic one
                this->sceneManager = NOWA::Core::getSingletonPtr()->getOgreRoot()->createSceneManager(Ogre::ST_GENERIC, numThreads, this->appStateName + "_SceneManager");
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[AppState]: Using " + Ogre::StringConverter::toString(numThreads) + " threads.");

                Ogre::Root::getSingletonPtr()->getRenderSystem()->setMetricsRecordingEnabled(true);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "AppState::initializeModules sceneManager");
        }

        if (true == initCamera)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
            {
                this->camera = this->sceneManager->createCamera(this->appStateName + "_Camera");
                Ogre::Vector3 position = this->camera->getParentSceneNode()->convertLocalToWorldPositionUpdated(Ogre::Vector3(0.0f, 5.0f, -2.0f));
                this->camera->setPosition(position);
                this->camera->setNearClipDistance(0.1f);
                this->camera->setFarClipDistance(500.0f);
                this->camera->setQueryFlags(0 << 0);

                // this->cameraManager->destroyContent();
                this->cameraManager->init("CameraManager1", this->camera);
                auto baseCamera = new BaseCamera(this->cameraManager->getCameraBehaviorId());
                this->cameraManager->addCameraBehavior(this->camera, baseCamera);

                this->cameraManager->setActiveCameraBehavior(this->camera, baseCamera->getBehaviorType());
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "AppState::initializeModules camera");
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
            NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
            {
                Core::getSingletonPtr()->setSceneManagerForMyGuiPlatform(this->sceneManager);

                this->applyRenderQueueModes();

                // LuaScriptApi::getInstance()->destroyAllScripts();

                // this->gameProgressModule->destroyContent();
                this->gameProgressModule->init(this->sceneManager);
                // this->particleFxModule->destroyContent();
                this->particleFxModule->init(this->sceneManager);
                // WorkspaceModule::getInstance()->destroyContent();
                // Create dummy workspace, since there is no one yet created
                WorkspaceModule::getInstance()->setPrimaryWorkspace(this->sceneManager, this->camera, nullptr);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "AppState::initializeModules renderqueue and init");

            OgreALModule::getInstance()->init(this->sceneManager);
        }
    }

        void AppState::destroyModules(void)
    {
        // Must run while the scene manager is still alive.
        this->destroyRenderQueueSlots();
        AppStateManager::getSingletonPtr()->clearLogicQueue();

        if (nullptr == this->sceneManager)
        {
            // Attention: without the cleanup below every module pointer would stay non-null.
            // initializeModules() only creates the modules when gameObjectController is null,
            // so on the next enter() this state would silently keep running with the modules
            // of the previous run, all of them bound to a scene manager that no longer exists.
            // Deleting them without their destroyContent() is not clean either, but it is far
            // better than handing out dangling modules.
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[AppState]: Could not destroy modules properly, because the scene manager is null. Deleting modules without content cleanup for app state: " + this->appStateName);

            delete this->gameObjectController;
            this->gameObjectController = nullptr;
            delete this->cameraManager;
            this->cameraManager = nullptr;
            delete this->ogreRecastModule;
            this->ogreRecastModule = nullptr;
            delete this->particleFxModule;
            this->particleFxModule = nullptr;
            delete this->gameProgressModule;
            this->gameProgressModule = nullptr;
            delete this->miniMapModule;
            this->miniMapModule = nullptr;
            delete this->decalsModule;
            this->decalsModule = nullptr;
            delete this->luaScriptModule;
            this->luaScriptModule = nullptr;
            delete this->rakNetModule;
            this->rakNetModule = nullptr;
            delete this->eventManager;
            this->eventManager = nullptr;
            delete this->scriptEventManager;
            this->scriptEventManager = nullptr;
            delete this->ogreNewtModule;
            this->ogreNewtModule = nullptr;

            return;
        }

        // Internally destroys all datablocks and movable objects (lines)
        this->ogreNewtModule->showOgreNewtCollisionLines(false);

        this->gameObjectController->destroyContent();
        delete this->gameObjectController;
        this->gameObjectController = nullptr;

        this->cameraManager->destroyContent();
        delete this->cameraManager;
        this->cameraManager = nullptr;

        this->ogreRecastModule->destroyContent();
        delete this->ogreRecastModule;
        this->ogreRecastModule = nullptr;

        this->particleFxModule->destroyContent();
        delete this->particleFxModule;
        this->particleFxModule = nullptr;

        this->gameProgressModule->destroyContent();
        delete this->gameProgressModule;
        this->gameProgressModule = nullptr;

        this->miniMapModule->destroyContent();
        delete this->miniMapModule;
        this->miniMapModule = nullptr;

        delete this->decalsModule;
        this->decalsModule = nullptr;

        // Destroy all scripts for just this AppState
        this->luaScriptModule->destroyContent();
        delete this->luaScriptModule;
        this->luaScriptModule = nullptr;

        if (nullptr != this->rakNetModule)
        {
            this->rakNetModule->destroyContent();
            delete this->rakNetModule;
            this->rakNetModule = nullptr;
        }

        WorkspaceModule::getInstance()->destroyContent();

        // If another states continues, do not destroy sounds.
        // Attention: this relies on exit() being called BEFORE the state is popped from the
        // stack, so the count still includes this state. One remaining state means "this is
        // the last one" and the sounds go away completely.
        if (AppStateManager::getSingletonPtr()->getAppStatesCount() > 1 && true == OgreALModule::getInstance()->getIsContinued())
        {
            OgreALModule::getInstance()->destroySounds(this->sceneManager);
        }
        else
        {
            OgreALModule::getInstance()->destroyContent();
        }

        Core::getSingletonPtr()->destroyScene(this->sceneManager);

        delete this->eventManager;
        this->eventManager = nullptr;

        this->scriptEventManager->destroyContent();
        delete this->scriptEventManager;
        this->scriptEventManager = nullptr;

        this->ogreNewtModule->destroyContent();
        delete this->ogreNewtModule;
        this->ogreNewtModule = nullptr;
    }

    bool AppState::getHasStarted(void) const
    {
        return this->hasStarted;
    }

    void AppState::applyRenderQueueModes(void)
    {
        if (nullptr == this->sceneManager)
        {
            return;
        }

        Ogre::RenderQueue::RqSortMode sortMode = Ogre::RenderQueue::RqSortMode::StableSort;

        /*
        RenderQueue ID range [0; 100) & [200; 225) default to FAST (i.e. for v2 objects, like Items)
        RenderQueue ID range [100; 200) & [225; 256) default to V1_FAST (i.e. for v1 objects, like v1::Entity)
        By default new Items and other v2 objects are placed in RenderQueue ID 10
        By default new v1::Entity and other v1 objects are placed in RenderQueue ID 110
        */

        this->sceneManager->getRenderQueue()->setRenderQueueMode(NOWA::RENDER_QUEUE_EARLY_FIRST, Ogre::RenderQueue::Modes::FAST);
        this->sceneManager->getRenderQueue()->setSortRenderQueue(NOWA::RENDER_QUEUE_EARLY_FIRST, sortMode);

        this->sceneManager->getRenderQueue()->setRenderQueueMode(NOWA::RENDER_QUEUE_V2_MESH, Ogre::RenderQueue::Modes::FAST);
        this->sceneManager->getRenderQueue()->setSortRenderQueue(NOWA::RENDER_QUEUE_V2_MESH, sortMode);

        this->sceneManager->getRenderQueue()->setRenderQueueMode(NOWA::RENDER_QUEUE_DISTORTION, Ogre::RenderQueue::Modes::FAST);

        // Ogre places every ParticleSystemDef on kParticleSystemDefaultRenderQueueId (15) until
        // something moves it. Registering that id as a particle queue as well means a definition
        // that kept its default still renders instead of silently disappearing.
        this->sceneManager->getRenderQueue()->setRenderQueueMode(15, Ogre::RenderQueue::Modes::PARTICLE_SYSTEM);
        this->sceneManager->getRenderQueue()->setSortRenderQueue(15, sortMode);

        this->sceneManager->getRenderQueue()->setRenderQueueMode(NOWA::RENDER_QUEUE_PARTICLE_STUFF, Ogre::RenderQueue::Modes::PARTICLE_SYSTEM);
        this->sceneManager->getRenderQueue()->setSortRenderQueue(NOWA::RENDER_QUEUE_PARTICLE_STUFF, sortMode);

        this->sceneManager->getRenderQueue()->setRenderQueueMode(NOWA::RENDER_QUEUE_PARTICLE_TRANSPARENT, Ogre::RenderQueue::Modes::PARTICLE_SYSTEM);
        this->sceneManager->getRenderQueue()->setSortRenderQueue(NOWA::RENDER_QUEUE_PARTICLE_TRANSPARENT, sortMode);

        this->sceneManager->getRenderQueue()->setRenderQueueMode(NOWA::RENDER_QUEUE_LEGACY, Ogre::RenderQueue::Modes::V1_LEGACY);
        this->sceneManager->getRenderQueue()->setSortRenderQueue(NOWA::RENDER_QUEUE_LEGACY, sortMode);

        this->sceneManager->getRenderQueue()->setRenderQueueMode(NOWA::RENDER_QUEUE_V2_OBJECTS_ALWAYS_IN_FOREGROUND, Ogre::RenderQueue::Modes::FAST);
        this->sceneManager->getRenderQueue()->setSortRenderQueue(NOWA::RENDER_QUEUE_V2_OBJECTS_ALWAYS_IN_FOREGROUND, sortMode);

        this->sceneManager->getRenderQueue()->setRenderQueueMode(NOWA::RENDER_QUEUE_V2_TRANSPARENT, Ogre::RenderQueue::Modes::FAST);
        this->sceneManager->getRenderQueue()->setSortRenderQueue(NOWA::RENDER_QUEUE_V2_TRANSPARENT, sortMode);

        this->sceneManager->getRenderQueue()->setRenderQueueMode(NOWA::RENDER_QUEUE_GIZMO, Ogre::RenderQueue::Modes::FAST);
        this->sceneManager->getRenderQueue()->setSortRenderQueue(NOWA::RENDER_QUEUE_GIZMO, sortMode);

        // MyGUI's Ogre2RenderManager already sets this queue to FAST + DisableSort internally
        // (see setSceneManager(), called via setSceneManagerForMyGuiPlatform). Repeated here so
        // this function stays the single source of truth for every queue's mode and doesn't rely
        // on init-order luck.
        // IMPORTANT: DisableSort, not sortMode (StableSort) - MyGUI relies on strict
        // painter's/submission order for correct widget layering.
        this->sceneManager->getRenderQueue()->setRenderQueueMode(NOWA::RENDER_QUEUE_MAX, Ogre::RenderQueue::Modes::FAST);
        this->sceneManager->getRenderQueue()->setSortRenderQueue(NOWA::RENDER_QUEUE_MAX, Ogre::RenderQueue::RqSortMode::DisableSort);

        // FIX: guarantee ObjectMemoryManager::getNumRenderQueues() covers the particle queues
        // regardless of scene content - see reserveRenderQueueSlots() comment above for the
        // full root-cause explanation (cullFrustum()'s loop bound problem).
        this->reserveRenderQueueSlots();
    }

    Ogre::SceneManager* AppState::getSceneManager(void) const
    {
        return this->sceneManager;
    }

}; // namespace end
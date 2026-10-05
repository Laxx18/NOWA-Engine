#include "NOWAPrecompiled.h"
#include "GameProgressModule.h"
#include "gameObject/AttributesComponent.h"
#include "gameObject/CameraComponent.h"
#include "gameobject/GameObjectController.h"
#include "gameobject/PhysicsComponent.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "main/ProcessManager.h"

#include "GraphicsModule.h"
#include "OgreNewtModule.h"
#include "WorkspaceModule.h"
#include "camera/CameraManager.h"

namespace NOWA
{
    namespace
    {
        /**
         * @brief	Lets the app state, that owns this game progress module, prepare the screen for a scene load, e.g. GameState shows
         *			the black fade screen. The same hook the AppStateManager calls before an app state change.
         *
         * Attention: must be called when the load is REQUESTED, not in the load process. There rendering is suspended, so a black
         * screen set there would never be presented and the teardown / parse of the scene would be visible (via the loading frames).
         * The fade in is done by the app state, when the scene has been loaded (AppState::start, e.g. GameState::start).
         */
        void notifyBeforeSceneLoaded(const Ogre::String& appStateName)
        {
            AppState* appState = AppStateManager::getSingletonPtr()->findByName(appStateName);
            if (nullptr != appState)
            {
                appState->beforeSceneLoaded();
            }
        }
    }

    /**
     * @brief	Suspends rendering for the lifetime of this object and guarantees that no renderOneFrame() is running
     *			once the constructor returns.
     *
     * Attention: needed around EVERY scene teardown. resetContent() destroys cameras and workspaces on the logic thread
     * while the render thread keeps rendering them. Neither bSceneLoading nor bStall reliably stopped that: the render
     * thread reads bSceneLoading through getActiveGameProgressModuleSafe(), and during the teardown it was observed false -
     * the render thread was inside renderOneFrame() -> CompositorPassScene::execute() on the already destroyed MainCamera
     * while the logic thread waited in WorkspaceBaseComponent::removeWorkspace() for the GameCamera.
     *
     * Setting a flag alone is not enough either: the render thread may be in the middle of renderOneFrame() at that very
     * moment. Hence the empty enqueueAndWait() below as a barrier. Commands are processed at the top of the render loop,
     * BEFORE the suspend flag is read, so once the empty command has run, the frame that was possibly in flight is
     * finished and every further iteration sees the flag and skips renderOneFrame().
     *
     * Must NOT be replaced by requestStall(): a stalled render thread does not service commands, and the teardown itself
     * uses enqueueAndWait() (e.g. removeWorkspace), which would deadlock.
     */
    class RenderSuspendBarrier
    {
    public:
        RenderSuspendBarrier() : wasSuspended(GraphicsModule::getInstance()->isRenderingSuspended())
        {
            GraphicsModule::getInstance()->suspendRendering(true);

            GraphicsModule::RenderCommand barrierCommand = []()
            {
            };
            GraphicsModule::getInstance()->enqueueAndWait(std::move(barrierCommand), "GameProgressModule::RenderSuspendBarrier");
        }

        ~RenderSuspendBarrier()
        {
            // Nesting safe: only resumes, if rendering was running before.
            if (false == this->wasSuspended)
            {
                GraphicsModule::getInstance()->suspendRendering(false);
            }
        }

        RenderSuspendBarrier(const RenderSuspendBarrier&) = delete;
        RenderSuspendBarrier& operator=(const RenderSuspendBarrier&) = delete;

    private:
        bool wasSuspended;
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    class LoadSceneProcess : public NOWA::Process
    {
    public:
        explicit LoadSceneProcess(const Ogre::String& appStateName, DotSceneImportModule* dotSceneImportModule, const Ogre::String& nextSceneName, const Ogre::String& playerName, bool sceneChanged, bool showProgress = false) :
            appStateName(appStateName),
            dotSceneImportModule(dotSceneImportModule),
            nextSceneName(nextSceneName),
            playerName(playerName),
            sceneChanged(sceneChanged),
            showProgress(showProgress)
        {
        }

    protected:
        virtual void onInit(void) override
        {
            this->succeed();

            auto* appStateMgr = AppStateManager::getSingletonPtr();
            auto* core = Core::getSingletonPtr();
            auto* gameProgressModule = appStateMgr->getGameProgressModule(this->appStateName);

            // No rendering from here until the new scene is parsed, see RenderSuspendBarrier. Resumes automatically on every
            // return path below.
            RenderSuspendBarrier renderSuspendBarrier;

            gameProgressModule->resetContent();

            // Run parsing scene on render thread
            this->dotSceneImportModule->setShowLoadingIndicator(true);
            bool success = this->dotSceneImportModule->parseScene(core->getProjectName(), this->nextSceneName, "Projects", nullptr);

            // The PlayerStartComponents of the new scene have evaluated the requested target location during the parse
            // (postInitData). Clear it, so that a later plain scene load (e.g. opening a scene in NOWA-Design) uses the
            // default spawn point again.
            gameProgressModule->setRequestedTargetLocationName("");

            if (false == success)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameProgressModule]: Error: Could not parse scene: '" + this->nextSceneName + "' correctly!");
                gameProgressModule->bSceneLoading = false;
                return;
            }

            // Scene was parsed successfully, construct scene parameter
            NOWA::SceneParameter sceneParameter;
            sceneParameter.appStateName = this->appStateName;
            sceneParameter.sceneManager = this->dotSceneImportModule->getSceneManager();
            sceneParameter.mainCamera = this->dotSceneImportModule->getMainCamera();
            sceneParameter.sunLight = this->dotSceneImportModule->getSunLight();
            sceneParameter.ogreNewt = appStateMgr->getOgreNewtModule(this->appStateName)->getOgreNewt();
            sceneParameter.dotSceneImportModule = this->dotSceneImportModule;

            // Trigger loaded event
            boost::shared_ptr<EventDataSceneLoaded> sceneLoadedEvent = boost::make_shared<EventDataSceneLoaded>(this->sceneChanged, this->dotSceneImportModule->getProjectParameter(), sceneParameter);

            appStateMgr->getEventManager(this->appStateName)->queueEvent(sceneLoadedEvent);

            gameProgressModule->bSceneLoading = false;
        }

        virtual void onUpdate(float dt) override
        {
            this->succeed();
        }

    private:
        Ogre::String appStateName;
        DotSceneImportModule* dotSceneImportModule;
        Ogre::String nextSceneName;
        Ogre::String playerName;
        bool sceneChanged;
        bool showProgress;
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    class LoadProgressProcess : public NOWA::Process
    {
    public:
        explicit LoadProgressProcess(const Ogre::String& appStateName, DotSceneImportModule* dotSceneImportModule, const Ogre::String& saveName, bool crypted, bool sceneSnapshot, const Ogre::String& globalAttributesStream,
            const Ogre::String& playerName, bool sceneChanged, bool showProgress = false) :
            appStateName(appStateName),
            dotSceneImportModule(dotSceneImportModule),
            saveName(saveName),
            crypted(crypted),
            sceneSnapshot(sceneSnapshot),
            globalAttributesStream(globalAttributesStream),
            playerName(playerName),
            sceneChanged(sceneChanged),
            showProgress(showProgress)
        {
        }

    protected:
        virtual void onInit(void) override
        {
            this->succeed();

            // See RenderSuspendBarrier: the snapshot branch below tears the scene down.
            RenderSuspendBarrier renderSuspendBarrier;

            NOWA::SceneParameter sceneParameter;
            // Set the data for a state
            sceneParameter.appStateName = this->appStateName;
            sceneParameter.sceneManager = this->dotSceneImportModule->getSceneManager();
            sceneParameter.mainCamera = this->dotSceneImportModule->getMainCamera();
            sceneParameter.sunLight = this->dotSceneImportModule->getSunLight();
            sceneParameter.ogreNewt = AppStateManager::getSingletonPtr()->getOgreNewtModule(this->appStateName)->getOgreNewt();
            sceneParameter.dotSceneImportModule = this->dotSceneImportModule;

            if (true == this->sceneSnapshot)
            {
                Ogre::String openFilePathName = Core::getSingletonPtr()->getSaveFilePathName(this->saveName, ".scene");

                std::pair<Ogre::String, Ogre::String> snapshotProjectAndSceneName = this->dotSceneImportModule->getProjectAndSceneName(openFilePathName, crypted);
                if (true == openFilePathName.empty())
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameProgressModule]: Error: Could not parse saved name: '" + this->saveName + "' because its unclear to which scene the save snapshot belongs!");
                    AppStateManager::getSingletonPtr()->getGameProgressModule(this->appStateName)->bSceneLoading = false;
                    return;
                }

                // Note: the screen is already black, see notifyBeforeSceneLoaded() in loadProgress(). A fade started HERE is never
                // presented: rendering is suspended and the scene is torn down in the very same call.
                AppStateManager::getSingletonPtr()->getGameProgressModule(this->appStateName)->resetContent();

                this->dotSceneImportModule->setShowLoadingIndicator(this->showProgress);
                // The snapshot contains the saved transform of the player - neither a PlayerStartComponent nor an exit must move it.
                AppStateManager::getSingletonPtr()->getGameProgressModule(this->appStateName)->setKeepPlayerTransform(true);
                bool success = this->dotSceneImportModule->parseSceneSnapshot(snapshotProjectAndSceneName.first, snapshotProjectAndSceneName.second, "Projects", openFilePathName, this->crypted);
                AppStateManager::getSingletonPtr()->getGameProgressModule(this->appStateName)->setKeepPlayerTransform(false);

                if (false == success)
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameProgressModule]: Error: Could not parse scene: '" + this->saveName + "' correctly!");
                    throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[GameProgressModule]: Error: Could not parse scene: '" + this->saveName + "' correctly!" + "'\n", "NOWA");
                }

                std::pair<Ogre::Real, Ogre::Real> continueData(0.0f, 0.0f);

                // Continue fading after loading scene, if there is enough time left
                /*if (continueData.second > 1.0f)
                {
                    NOWA::ProcessManager::getInstance()->attachProcess(NOWA::ProcessPtr(new NOWA::FaderProcess(NOWA::FaderProcess::FadeOperation::FADE_IN, 10.0f, continueData.first, continueData.second)));
                }*/

                // NOWA::ProcessManager::getInstance()->attachProcess(NOWA::ProcessPtr(new NOWA::FaderProcess(NOWA::FaderProcess::FadeOperation::FADE_IN, 1.0f)));
                // Ogre::Root::getSingletonPtr()->renderOneFrame();

                // NOWA::AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->start();
            }

            bool success = AppStateManager::getSingletonPtr()->getGameProgressModule(this->appStateName)->internalReadGlobalAttributes(globalAttributesStream);
            if (false == success)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameProgressModule]: Error: Could not read global attributes for saved name: '" + this->saveName + "' correctly!");
            }

            // Send event, that scene has been loaded. Note: When scene has been changed, send that flag. E.g. in DesignState, if true, also call GameObjectController::start, so that when in simulation
            // and the scene has been changed, remain in simulation and maybe activate player controller, so that the player may continue his game play
            // This will also call connect for all lua scripts, so that data is really actualized at runtime
            boost::shared_ptr<EventDataSceneLoaded> EventDataSceneLoaded(new EventDataSceneLoaded(this->sceneChanged, this->dotSceneImportModule->getProjectParameter(), sceneParameter));
            NOWA::AppStateManager::getSingletonPtr()->getEventManager(this->appStateName)->triggerEvent(EventDataSceneLoaded);

            /*for (unsigned short i = 0; i < 2; i++)
            {
                Ogre::Root::getSingletonPtr()->renderOneFrame();
            }*/

            if (true == Core::getSingletonPtr()->getIsGame())
            {
                AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->start();

                // NOWA::GameObjectPtr player = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromName(NOWA::AppStateManager::getSingletonPtr()->getGameProgressModule()->getPlayerName());
                // if (nullptr != player)
                // {
                // 	NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->activatePlayerController(true, player->getId(), true);
                // }
            }

            AppStateManager::getSingletonPtr()->getGameProgressModule(this->appStateName)->bSceneLoading = false;
        }

        virtual void onUpdate(float dt) override
        {
            this->succeed();
        }

    private:
        Ogre::String appStateName;
        DotSceneImportModule* dotSceneImportModule;
        Ogre::String saveName;
        Ogre::String playerName;
        bool crypted;
        bool sceneSnapshot;
        Ogre::String globalAttributesStream;
        bool sceneChanged;
        bool showProgress;
    };

    class SetPlayerLocationProcess : public NOWA::Process
    {
    public:
		explicit SetPlayerLocationProcess(PhysicsComponent* physicsComponent, const Ogre::Vector3& position, const Ogre::Quaternion& orientation)
			: position(position),
			orientation(orientation),
			physicsComponent(physicsComponent)
        {
        }

    protected:
        virtual void onInit(void) override
        {
            this->succeed();
            physicsComponent->getBody()->setPositionOrientation(this->position, this->orientation);
        }

        virtual void onUpdate(float dt) override
        {
            this->succeed();
        }

    private:
        DotSceneImportModule* dotSceneImportModule;
        Ogre::Vector3 position;
        Ogre::Quaternion orientation;
        PhysicsComponent* physicsComponent;
    };

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GameProgressModule::GameProgressModule(const Ogre::String& appStateName)
		: appStateName(appStateName),
		player(nullptr),
		dotSceneImportModule(nullptr),
		dotSceneExportModule(nullptr),
		bSceneLoading(false),
		keepPlayerTransform(false)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[GameProgressModule] Module created");
    }

    GameProgressModule::~GameProgressModule()
    {
    }

    Ogre::SceneManager* GameProgressModule::getCurrentSceneManager(void)
    {
        if (nullptr != this->dotSceneImportModule)
        {
            return this->dotSceneImportModule->getSceneManager();
        }
        return nullptr;
    }

    void GameProgressModule::destroyContent(void)
    {
        if (this->dotSceneImportModule)
        {
            delete this->dotSceneImportModule;
            this->dotSceneImportModule = nullptr;
        }
        if (this->dotSceneExportModule)
        {
            delete this->dotSceneExportModule;
            this->dotSceneExportModule = nullptr;
        }
        this->sceneMap.clear();

        this->bSceneLoading = false;
    }

    void GameProgressModule::resetContent(void)
    {
        // Note: For an AppState one scene after another can be loaded, so at least some resources must be stopped and cleared
        boost::shared_ptr<EventDataLuaScriptModfied> eventDataLuaScriptModified(new EventDataLuaScriptModfied(0L, ""));
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataLuaScriptModified);

        // Dangerous, as on own exec state change will not work amymore!
        // GraphicsModule::getInstance()->clearSceneResources();
        AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->stop();
        AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->destroyContent(/*excludeGameObjects*/);
        AppStateManager::getSingletonPtr()->getOgreNewtModule(this->appStateName)->destroyContent();
        AppStateManager::getSingletonPtr()->getLuaScriptModule(this->appStateName)->destroyContent();
        // Must not be destroyed! else no particle effect etc.
        // AppStateManager::getSingletonPtr()->getParticleFxModule(this->appStateName)->destroyContent();
        // AppStateManager::getSingletonPtr()->getOgreRecastModule(this->appStateName)->destroyContent();

        // TODO what else must be deleted?
        AppStateManager::getSingletonPtr()->getWorkspaceModule(this->appStateName)->destroyContent(this->sceneManager);

        // Attention: the player game object has just been destroyed with the rest of the content. Without this,
        // getPlayerName() and every later changeScene() read a dangling pointer until an ExitComponent of the new scene
        // announces the new player instance in its connect().
        this->player = nullptr;
    }

    void GameProgressModule::start(void)
    {
    }

    void GameProgressModule::stop(void)
    {
        // TODO: Remove this lines, if it does work: Global attributes shall live through the whole application and all states!
#if 0
		// Delete all global defined attributes (when lua script has been disconnected and re-connected, this is required)
		auto it = this->globalAttributesMap.begin();
		while (it != this->globalAttributesMap.end())
		{
			Variant* globalAttribute = it->second;
			delete globalAttribute;
			globalAttribute = nullptr;
			++it;
		}
		this->globalAttributesMap.clear();

#endif
    }

    void GameProgressModule::init(Ogre::SceneManager* sceneManager)
    {
        this->sceneManager = sceneManager;
        if (nullptr != this->dotSceneImportModule)
        {
            delete this->dotSceneImportModule;
        }
        this->dotSceneImportModule = new NOWA::DotSceneImportModule(sceneManager, nullptr, nullptr);
    }

    void GameProgressModule::setPlayerName(const Ogre::String& playerName)
    {
        // note: this function will be called in each exit component for each state, so the addresses of a player may variate
        this->player = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromName(playerName).get();
        if (nullptr == this->player)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameProgressModule] GameObject for player name: " + playerName + " does not exist!");
            // throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[GameProgressModule] GameObject for player name: " + playerName + " does not exist!\n", "NOWA");
        }
    }

    Ogre::String GameProgressModule::getPlayerName(void) const
    {
        if (nullptr != this->player)
        {
            return this->player->getName();
        }
        else
        {
            return "";
        }
    }

    void GameProgressModule::addScene(const Ogre::String& currentSceneName, const Ogre::String& reachableSceneName, const Ogre::String& targetLocationName, const Ogre::Vector2& exitDirection, const Ogre::Vector3& startPosition, bool xyAxis)
    {
        this->currentSceneName = currentSceneName;
        Ogre::String reachableSceneProjectName;
        if (!reachableSceneName.empty())
        {
            reachableSceneProjectName = Core::getSingletonPtr()->getProjectName() + "/" + reachableSceneName + "/" + reachableSceneName + ".scene";
        }

        auto it = this->sceneMap.find(currentSceneName);
        if (this->sceneMap.end() == it)
        {
            GameProgressModule::SceneData sceneData(currentSceneName, targetLocationName, exitDirection, startPosition, xyAxis);
            if (!reachableSceneName.empty())
            {
                sceneData.addReachableScene(reachableSceneProjectName);
            }
            this->sceneMap.emplace(currentSceneName, sceneData);
            // actualize iterator for predecessor settings below
            it = this->sceneMap.find(currentSceneName);
        }
        else
        {
            if (!currentSceneName.empty())
            {
                it->second.setSceneName(currentSceneName);
            }
            if (!reachableSceneName.empty())
            {
                it->second.addReachableScene(reachableSceneProjectName);
            }
            if (Ogre::Vector2::ZERO != exitDirection)
            {
                it->second.setExitDirection(exitDirection);
            }
            if (Ogre::Vector3::ZERO != startPosition)
            {
                it->second.setStartPosition(startPosition);
            }
            it->second.setUseXYAxis(xyAxis);
        }
        if (!reachableSceneName.empty() && this->sceneMap.end() != it)
        {
            // check if the reachable app state name already exist, if yes, it is the predecessor!
            const auto it2 = this->sceneMap.find(reachableSceneProjectName);
            if (this->sceneMap.end() != it2)
            {
                // the currents app state predecessor is the reachable one
                it->second.predecessorSceneData = &(it2->second);
                // ?? e.g. SimulationState is predecessor of SimulationState2 and vice versa??
                // it2->second.predecessorScene = &(it->second);
            }
        }
    }

    std::vector<Ogre::String>* GameProgressModule::getReachableScenes(const Ogre::String& currentSceneName)
    {
        const auto it = this->sceneMap.find(currentSceneName);
        if (this->sceneMap.end() != it)
        {
            return &it->second.getReachableScenes();
        }
        return nullptr;
    }

    GameProgressModule::SceneData* GameProgressModule::getPredecessorSceneData(const Ogre::String& currentSceneName)
    {
        const auto it = this->sceneMap.find(currentSceneName);
        if (this->sceneMap.end() != it)
        {
            return it->second.getPredecessorSceneData();
        }
        return nullptr;
    }

    GameProgressModule::SceneData* GameProgressModule::getSceneData(const Ogre::String& currentSceneName)
    {
        const auto it = this->sceneMap.find(currentSceneName);
        if (this->sceneMap.end() != it)
        {
            return &(it->second);
        }
        return nullptr;
    }

    unsigned int GameProgressModule::getScenesCount(void) const
    {
        return static_cast<unsigned int>(this->sceneMap.size());
    }

    void GameProgressModule::setRequestedTargetLocationName(const Ogre::String& targetLocationName)
    {
        this->requestedTargetLocationName = targetLocationName;
    }

    Ogre::String GameProgressModule::getRequestedTargetLocationName(void) const
    {
        return this->requestedTargetLocationName;
    }

    void GameProgressModule::setKeepPlayerTransform(bool keepPlayerTransform)
    {
        this->keepPlayerTransform = keepPlayerTransform;
    }

    bool GameProgressModule::getKeepPlayerTransform(void) const
    {
        return this->keepPlayerTransform;
    }

    bool GameProgressModule::internalIsSceneLoadAlreadyRequested(const Ogre::String& sceneName)
    {
        // Attention: a second request while one is pending must be ignored. It used to attach a second LoadSceneProcess,
        // whose teardown then ran after the first one had already set bSceneLoading back to false - i.e. with rendering
        // active. Two exits touched in the same frame, or an exit's trigger and its distance check firing together, were
        // enough for that.
        if (true == this->bSceneLoading.load())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[GameProgressModule] Ignoring the request to load scene: '" + sceneName + "', because another scene is already being loaded.");
            return true;
        }
        return false;
    }

    void GameProgressModule::loadScene(const Ogre::String& sceneName)
    {
        if (true == this->internalIsSceneLoadAlreadyRequested(sceneName))
        {
            return;
        }

        this->bSceneLoading = true;

        Ogre::String projectName = NOWA::Core::getSingletonPtr()->getProjectNameFromPath(sceneName);
        NOWA::Core::getSingletonPtr()->setProjectName(projectName);
        Ogre::String fileName = NOWA::Core::getSingletonPtr()->getFileNameFromPath(sceneName);

        // Black before the teardown, see notifyBeforeSceneLoaded().
        notifyBeforeSceneLoaded(this->appStateName);

        NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.5f));
        // Creates the delay process and changes the scene at another tick. Note, this is necessary
        // because changing the scene destroys all game objects and its components.
        // So changing the state directly inside a component would create a mess, since everything will be destroyed
        // and the game object map in update loop becomes invalid while its iterating
        delayProcess->attachChild(NOWA::ProcessPtr(new LoadSceneProcess(this->appStateName, this->dotSceneImportModule, fileName, nullptr != this->player ? this->player->getName() : "", false)));
        NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
    }

    void GameProgressModule::loadSceneShowProgress(const Ogre::String& sceneName)
    {
        if (true == this->internalIsSceneLoadAlreadyRequested(sceneName))
        {
            return;
        }

        this->bSceneLoading = true;

        Ogre::String projectName = NOWA::Core::getSingletonPtr()->getProjectNameFromPath(sceneName);
        NOWA::Core::getSingletonPtr()->setProjectName(projectName);
        Ogre::String fileName = NOWA::Core::getSingletonPtr()->getFileNameFromPath(sceneName);

        // Black before the teardown, see notifyBeforeSceneLoaded().
        notifyBeforeSceneLoaded(this->appStateName);

        NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.5f));
        // Creates the delay process and changes the scene at another tick. Note, this is necessary
        // because changing the scene destroys all game objects and its components.
        // So changing the state directly inside a component would create a mess, since everything will be destroyed
        // and the game object map in update loop becomes invalid while its iterating
        delayProcess->attachChild(NOWA::ProcessPtr(new LoadSceneProcess(this->appStateName, this->dotSceneImportModule, fileName, nullptr != this->player ? this->player->getName() : "", false, true)));
        NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
    }

    void GameProgressModule::changeScene(const Ogre::String& sceneName)
    {
        if (true == this->internalIsSceneLoadAlreadyRequested(sceneName))
        {
            return;
        }

        this->bSceneLoading = true;

        // Black before the teardown, see notifyBeforeSceneLoaded().
        notifyBeforeSceneLoaded(this->appStateName);

        NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.5f));
        // Creates the delay process and changes the scene at another tick. Note, this is necessary
        // because changing the scene destroys all game objects and its components.
        // So changing the state directly inside a component would create a mess, since everything will be destroyed
        // and the game object map in update loop becomes invalid while its iterating
        delayProcess->attachChild(NOWA::ProcessPtr(new LoadSceneProcess(this->appStateName, this->dotSceneImportModule, sceneName, nullptr != this->player ? this->player->getName() : "", true)));
        NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
    }

    void GameProgressModule::changeSceneShowProgress(const Ogre::String& sceneName)
    {
        if (true == this->internalIsSceneLoadAlreadyRequested(sceneName))
        {
            return;
        }

        this->bSceneLoading = true;

        // Black before the teardown, see notifyBeforeSceneLoaded().
        notifyBeforeSceneLoaded(this->appStateName);

        NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.5f));
        // Creates the delay process and changes the scene at another tick. Note, this is necessary
        // because changing the scene destroys all game objects and its components.
        // So changing the state directly inside a component would create a mess, since everything will be destroyed
        // and the game object map in update loop becomes invalid while its iterating
        delayProcess->attachChild(NOWA::ProcessPtr(new LoadSceneProcess(this->appStateName, this->dotSceneImportModule, sceneName, nullptr != this->player ? this->player->getName() : "", true, true)));
        NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
    }

    Ogre::String GameProgressModule::getCurrentSceneName(void)
    {
        return this->currentSceneName;
    }

    void GameProgressModule::saveProgress(const Ogre::String& saveName, bool crypted, bool sceneSnapshot)
    {
        this->saveName = saveName;
        Ogre::String saveFilePathName = Core::getSingletonPtr()->getSaveFilePathName(saveName);
        if (false == saveFilePathName.empty())
        {
            if (nullptr != this->dotSceneExportModule)
            {
                delete this->dotSceneExportModule;
            }
            this->dotSceneExportModule = new NOWA::DotSceneExportModule(sceneManager, AppStateManager::getSingletonPtr()->getOgreNewtModule(this->appStateName)->getOgreNewt());

            if (true == sceneSnapshot)
            {
                this->dotSceneExportModule->saveSceneSnapshot(saveFilePathName + ".scene", crypted);
            }

            AppStateManager::getSingletonPtr()->saveProgress(saveFilePathName, crypted, sceneSnapshot);
        }
    }

	void GameProgressModule::loadProgress(const Ogre::String& saveName, bool sceneSnapshot, bool showProgress)
    {
		// Attention: this used to return a bool that was ALWAYS false - 'success' was set to false and never touched again, because the real
		// loading happens later in the LoadProgressProcess. Callers that tested it therefore always saw a failure. Use hasSaveGame to find
		// out whether there is something to load.
        if (true == saveName.empty())
        {
			return;
        }

        if (true == this->internalIsSceneLoadAlreadyRequested(saveName))
        {
			return;
        }

        this->bSceneLoading = true;

        this->saveName = saveName;
        Ogre::String tempSaveName = saveName;

        auto resultPair = Core::getSingletonPtr()->removePartsFromString(saveName, ".scene");
        if (true == resultPair.first)
        {
            tempSaveName = resultPair.second;
        }

        auto streamData = this->getSaveFileContent(tempSaveName);
        if (true == streamData.second.empty())
        {
            this->bSceneLoading = false;
			return;
        }

        // Only a snapshot tears the scene down - black before that, see notifyBeforeSceneLoaded().
        if (true == sceneSnapshot)
        {
            notifyBeforeSceneLoaded(this->appStateName);
        }

        NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.5f));
        // Creates the delay process and changes the scene at another tick. Note, this is necessary
        // because changing the scene destroys all game objects and its components.
        // So changing the state directly inside a component would create a mess, since everything will be destroyed
        // and the game object map in update loop becomes invalid while its iterating
        delayProcess->attachChild(NOWA::ProcessPtr(new LoadProgressProcess(this->appStateName, this->dotSceneImportModule, saveName, streamData.first, sceneSnapshot, streamData.second, nullptr != this->player ? this->player->getName() : "", true)));
        NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
	}

	bool GameProgressModule::hasSaveGame(const Ogre::String& saveName)
	{
		if (true == saveName.empty())
		{
			return false;
		}

		// Attention: deliberately NOT getSaveFileContent, that one overwrites this->saveName as a side effect and reads the whole file.
		const Ogre::String openFilePathName = Core::getSingletonPtr()->getSaveFilePathName(saveName);
		if (true == openFilePathName.empty())
		{
			return false;
		}

		std::ifstream inFile(openFilePathName.c_str(), std::ios::in);
		return true == inFile.good();
	}

	void GameProgressModule::clearGlobalValues(void)
	{
		AppStateManager::getSingletonPtr()->clearGlobalValues();
    }

    bool GameProgressModule::internalReadGlobalAttributes(const Ogre::String& globalAttributesStream)
    {
        return AppStateManager::getSingletonPtr()->internalReadGlobalAttributes(globalAttributesStream);
    }

    void GameProgressModule::saveValues(const Ogre::String& saveName, unsigned long gameObjectId, bool crypted)
    {
        this->saveName = saveName;

        std::ostringstream oStream;
        bool foundSpecific = false;

        // Get the content from file
        auto streamData = this->getSaveFileContent(saveName);
        if (false == streamData.second.empty())
        {
            Ogre::String strGameObjectId = Ogre::StringConverter::toString(gameObjectId);
            size_t foundIdPos = streamData.second.find(Ogre::StringConverter::toString(gameObjectId));
            if (Ogre::String::npos != foundIdPos)
            {
                // + 2 -> ]\n
                foundIdPos += strGameObjectId.size() + 2;

                // Get the game object controller for this app state name
                auto gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(gameObjectId);
                if (nullptr != gameObjectPtr)
                {
                    // Save attributes for the given game object id
                    boost::shared_ptr<AttributesComponent> attributesCompPtr = NOWA::makeStrongPtr(gameObjectPtr->getComponent<AttributesComponent>());
                    if (nullptr != attributesCompPtr)
                    {
                        attributesCompPtr->internalSave(oStream);
                    }
                }
                // Get the content before the attributes
                Ogre::String beforeAttributesContent = streamData.second.substr(0, foundIdPos);
                beforeAttributesContent += oStream.str();

                Ogre::String afterAttributesContent;

                size_t foundNextIdPos = streamData.second.find('[', foundIdPos);
                if (Ogre::String::npos != foundNextIdPos)
                {
                    int remainingSize = static_cast<int>(streamData.second.size() - foundNextIdPos);

                    // Get the content after the attributes
                    if (remainingSize > 0)
                    {
                        afterAttributesContent = streamData.second.substr(foundNextIdPos, remainingSize);
                    }
                }
                // Must be cleared, because its content is already used in before attributes content
                oStream.str("");
                oStream << beforeAttributesContent + afterAttributesContent;

                foundSpecific = true;
            }
        }

        Ogre::String saveFilePathName = Core::getSingletonPtr()->getSaveFilePathName(this->saveName);
        if (false == saveFilePathName.empty())
        {
            std::ofstream outFile;
            outFile.open(saveFilePathName.c_str());
            if (!outFile)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameProgressModule] ERROR: Could not create file for path: " + saveFilePathName + "'");
                return;
            }

            // Store whether the file should be crypted or not
            outFile << crypted << "\n";

            if (false == foundSpecific)
            {
                // Get the game object controller for this app state name
                auto gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(gameObjectId);
                if (nullptr != gameObjectPtr)
                {
                    // https://thinkcpp.wordpress.com/2012/04/16/file-to-map-inputoutput/

                    // First save the game object id, if it does have an attributes component
                    boost::shared_ptr<AttributesComponent> attributesCompPtr = NOWA::makeStrongPtr(gameObjectPtr->getComponent<AttributesComponent>());
                    if (nullptr != attributesCompPtr)
                    {
                        oStream << "[" << Ogre::StringConverter::toString(gameObjectPtr->getId()) << "]\n";
                        attributesCompPtr->internalSave(oStream);
                    }
                }
            }

            if (true == crypted)
            {
                streamData.second = Core::getSingletonPtr()->encode64(oStream.str(), true);
            }
            else
            {
                streamData.second = oStream.str();
            }

            outFile << streamData.second;
            outFile.close();
        }
        else
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameProgressModule] ERROR: Could not get file path name for saving data: " + saveFilePathName + "'");
        }
    }

    void GameProgressModule::saveValue(const Ogre::String& saveName, unsigned long gameObjectId, unsigned int attributeIndex, bool crypted)
    {
        this->saveName = saveName;

        std::ostringstream oStream;
        bool foundSpecific = false;
        auto streamData = this->getSaveFileContent(saveName);
        if (false == streamData.second.empty())
        {
            Ogre::String strGameObjectId = Ogre::StringConverter::toString(gameObjectId);
            size_t foundIdPos = streamData.second.find(strGameObjectId);
            if (Ogre::String::npos != foundIdPos)
            {
                // + 2 -> ]\n
                foundIdPos += strGameObjectId.size() + 2;

                // Get the game object controller for this app state name
                auto gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(gameObjectId);
                if (nullptr != gameObjectPtr)
                {
                    boost::shared_ptr<AttributesComponent> attributesCompPtr = NOWA::makeStrongPtr(gameObjectPtr->getComponent<AttributesComponent>());
                    if (nullptr != attributesCompPtr)
                    {
                        Ogre::String attributesContent;
                        size_t foundNextIdPos = streamData.second.find('[', foundIdPos);
                        if (Ogre::String::npos != foundNextIdPos)
                        {
                            attributesContent = streamData.second.substr(foundIdPos, foundNextIdPos - foundIdPos);
                        }
                        else
                        {
                            // If it was the last game object and nothing comes after that
                            attributesContent = streamData.second.substr(foundIdPos, streamData.second.size() - foundIdPos);
                        }
                        // Replace content internally
                        attributesCompPtr->internalSave(attributesContent, attributeIndex);

                        // Get the content before the attributes
                        Ogre::String beforeAttributesContent = streamData.second.substr(0, foundIdPos);
                        beforeAttributesContent += attributesContent;

                        Ogre::String afterAttributesContent;

                        // Get the content after the attributes
                        if (Ogre::String::npos != foundNextIdPos)
                        {
                            int remainingSize = static_cast<unsigned int>(streamData.second.size() - foundNextIdPos);

                            if (remainingSize > 0)
                            {
                                afterAttributesContent = streamData.second.substr(foundNextIdPos, remainingSize);
                            }
                        }

                        oStream << beforeAttributesContent + afterAttributesContent;

                        foundSpecific = true;
                    }
                }
            }
        }

        Ogre::String saveFilePathName = Core::getSingletonPtr()->getSaveFilePathName(this->saveName);
        if (false == saveFilePathName.empty())
        {
            std::ofstream outFile;
            outFile.open(saveFilePathName.c_str());
            if (!outFile)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameProgressModule] ERROR: Could not create file for path: " + saveFilePathName + "'");
                return;
            }

            // Store whether the file should be crypted or not
            outFile << crypted << "\n";

            if (false == foundSpecific)
            {
                // Get the game object controller for this app state name
                auto gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(gameObjectId);
                if (nullptr != gameObjectPtr)
                {
                    // https://thinkcpp.wordpress.com/2012/04/16/file-to-map-inputoutput/

                    // First save the game object id, if it does have an attributes component
                    boost::shared_ptr<AttributesComponent> attributesCompPtr = NOWA::makeStrongPtr(gameObjectPtr->getComponent<AttributesComponent>());
                    if (nullptr != attributesCompPtr)
                    {
                        oStream << "[" << Ogre::StringConverter::toString(gameObjectPtr->getId()) << "]\n";
                        attributesCompPtr->internalSave(oStream);
                    }
                }
            }

            if (true == crypted)
            {
                streamData.second = Core::getSingletonPtr()->encode64(oStream.str(), true);
            }
            else
            {
                streamData.second = oStream.str();
            }

            outFile << streamData.second;
            outFile.close();
        }
        else
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameProgressModule] ERROR: Could not get file path name for saving data: " + saveFilePathName + "'");
        }
    }

    std::pair<bool, Ogre::String> GameProgressModule::getSaveFileContent(const Ogre::String& saveName)
    {
        bool crypted = false;
        Ogre::String strStream;
        this->saveName = saveName;
        Ogre::String openFilePathName = Core::getSingletonPtr()->getSaveFilePathName(this->saveName);
        if (false == openFilePathName.empty())
        {
            std::ifstream inFile;
            inFile.open(openFilePathName.c_str(), std::ios::in);
            if (!inFile)
            {
                // Nothing to read
                return std::make_pair(false, strStream);
            }

            // Get the first line to check if its crypted
            Ogre::String line;
            inFile >> line;

            // Crypted?
            std::istringstream(line) >> crypted;

            Ogre::String content = std::string((std::istreambuf_iterator<char>(inFile)), (std::istreambuf_iterator<char>()));

            if (true == content.empty())
            {
                return std::make_pair(false, strStream);
            }

            // eat the \n
            if (content[0] == '\n')
            {
                content = content.substr(1, content.size());
            }

            inFile.close();

            if (true == crypted)
            {
                strStream = Core::getSingletonPtr()->decode64(content, true);
            }
            else
            {
                strStream = content;
            }
        }
        return std::make_pair(crypted, strStream);
    }

    bool GameProgressModule::loadValues(const Ogre::String& saveName, unsigned long gameObjectId)
    {
        bool success = false;
        auto streamData = this->getSaveFileContent(saveName);
        if (true == streamData.second.empty())
        {
            return success;
        }

        std::istringstream inStream(streamData.second);
        Ogre::String line;

        while (std::getline(inStream, line)) // This is used, because white spaces are also read
        {
            if (true == line.empty())
            {
                continue;
            }

            // GameObject
            Ogre::String strGameObjectId = line.substr(1, line.size() - 2);
            unsigned long id;
            std::istringstream(strGameObjectId) >> id;

            if (id == gameObjectId)
            {
                // Get the game object controller for this app state name
                auto gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(id);
                if (nullptr != gameObjectPtr)
                {
                    boost::shared_ptr<AttributesComponent> attributesCompPtr = NOWA::makeStrongPtr(gameObjectPtr->getComponent<AttributesComponent>());
                    if (nullptr != attributesCompPtr)
                    {
                        // Read data and set for attributes component
                        success = attributesCompPtr->internalRead(inStream);
                    }
                }
                break;
            }
        }
        return success;
    }

    bool GameProgressModule::loadValue(const Ogre::String& saveName, unsigned long gameObjectId, unsigned int attributeIndex)
    {
        bool success = false;
        auto streamData = this->getSaveFileContent(saveName);
        if (true == streamData.second.empty())
        {
            return success;
        }

        std::istringstream inStream(streamData.second);

        Ogre::String line;

        while (std::getline(inStream, line)) // This is used, because white spaces are also read
        {
            if (true == line.empty())
            {
                continue;
            }

            // GameObject
            Ogre::String strGameObjectId = line.substr(1, line.size() - 2);
            unsigned long id;
            std::istringstream(strGameObjectId) >> id;

            if (id == gameObjectId)
            {
                // Get the game object controller for this app state name
                auto gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(id);
                if (nullptr != gameObjectPtr)
                {
                    boost::shared_ptr<AttributesComponent> attributesCompPtr = NOWA::makeStrongPtr(gameObjectPtr->getComponent<AttributesComponent>());
                    if (nullptr != attributesCompPtr)
                    {
                        // Read data and set for attributes component
                        success = attributesCompPtr->internalRead(inStream, attributeIndex);
                    }
                }
                break;
            }
        }
        return success;
    }

    /*void GameProgressModule::saveUserValue(const Ogre::String& saveName, const Ogre::String& attributeName);
    {

    }

    void GameProgressModule::saveUserValue(const Ogre::String& saveName, Variant* globalAttribute)
    {

    }

    void GameProgressModule::saveUserValues(const Ogre::String& saveName)
    {

    }

    Variant* GameProgressModule::loadUserValue(const Ogre::String& saveName, const Ogre::String& attributeName)
    {

    }

    bool GameProgressModule::loadUserValues(const Ogre::String& saveName)
    {

    }*/

    Variant* GameProgressModule::getGlobalValue(const Ogre::String& attributeName)
    {
        return AppStateManager::getSingletonPtr()->getGlobalValue(attributeName);
    }

    Variant* GameProgressModule::setGlobalBoolValue(const Ogre::String& attributeName, bool value)
    {
        return AppStateManager::getSingletonPtr()->setGlobalBoolValue(attributeName, value);
    }

    Variant* GameProgressModule::setGlobalIntValue(const Ogre::String& attributeName, int value)
    {
        return AppStateManager::getSingletonPtr()->setGlobalIntValue(attributeName, value);
    }

    Variant* GameProgressModule::setGlobalUIntValue(const Ogre::String& attributeName, unsigned int value)
    {
        return AppStateManager::getSingletonPtr()->setGlobalUIntValue(attributeName, value);
    }

    Variant* GameProgressModule::setGlobalULongValue(const Ogre::String& attributeName, unsigned long value)
    {
        return AppStateManager::getSingletonPtr()->setGlobalULongValue(attributeName, value);
    }

    Variant* GameProgressModule::setGlobalRealValue(const Ogre::String& attributeName, Ogre::Real value)
    {
        return AppStateManager::getSingletonPtr()->setGlobalRealValue(attributeName, value);
    }

    Variant* GameProgressModule::setGlobalStringValue(const Ogre::String& attributeName, Ogre::String value)
    {
        return AppStateManager::getSingletonPtr()->setGlobalStringValue(attributeName, value);
    }

    Variant* GameProgressModule::setGlobalVector2Value(const Ogre::String& attributeName, Ogre::Vector2 value)
    {
        return AppStateManager::getSingletonPtr()->setGlobalVector2Value(attributeName, value);
    }

    Variant* GameProgressModule::setGlobalVector3Value(const Ogre::String& attributeName, Ogre::Vector3 value)
    {
        return AppStateManager::getSingletonPtr()->setGlobalVector3Value(attributeName, value);
    }

    Variant* GameProgressModule::setGlobalVector4Value(const Ogre::String& attributeName, Ogre::Vector4 value)
    {
        return AppStateManager::getSingletonPtr()->setGlobalVector4Value(attributeName, value);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    GameProgressModule::SceneData::SceneData(const Ogre::String& stateName)
        // Attention: was 'sceneName(sceneName)', i.e. the member initialized with itself (garbage), and xyAxis was not
        // initialized at all.
        :
        sceneName(stateName),
        exitDirection(Ogre::Vector2::ZERO),
        startPosition(Ogre::Vector3::ZERO),
        xyAxis(true),
        predecessorSceneData(nullptr)
    {
    }

    GameProgressModule::SceneData::SceneData(const Ogre::String& sceneName, const Ogre::String& targetLocationName, const Ogre::Vector2& exitDirection, const Ogre::Vector3& startPosition, bool xyAxis) :
        sceneName(sceneName),
        targetLocationName(targetLocationName),
        exitDirection(exitDirection),
        startPosition(startPosition),
        xyAxis(xyAxis),
        predecessorSceneData(nullptr)
    {
    }

    void GameProgressModule::SceneData::addReachableScene(const Ogre::String& reachableSceneName)
    {
        bool foundScene = false;
        for (size_t i = 0; i < this->reachableScenes.size(); i++)
        {
            if (reachableSceneName == this->reachableScenes[i])
            {
                foundScene = true;
            }
        }
        if (false == foundScene)
        {
            this->reachableScenes.emplace_back(reachableSceneName);
        }
    }

    std::vector<Ogre::String>& GameProgressModule::SceneData::getReachableScenes(void)
    {
        return this->reachableScenes;
    }

    void GameProgressModule::SceneData::setExitDirection(const Ogre::Vector2& exitDirection)
    {
        this->exitDirection = exitDirection;
    }

    Ogre::Vector2 GameProgressModule::SceneData::getExitDirection(void) const
    {
        return this->exitDirection;
    }

    void GameProgressModule::SceneData::setStartPosition(const Ogre::Vector3& startPosition)
    {
        this->startPosition = startPosition;
    }

    Ogre::Vector3 GameProgressModule::SceneData::getStartPosition(void) const
    {
        return this->startPosition;
    }

    void GameProgressModule::SceneData::setUseXYAxis(bool xyAxis)
    {
        this->xyAxis = xyAxis;
    }

    bool GameProgressModule::SceneData::getIsXYAxisUsed(void) const
    {
        return this->xyAxis;
    }

    GameProgressModule::SceneData* GameProgressModule::SceneData::getPredecessorSceneData(void) const
    {
        return this->predecessorSceneData;
    }

    void GameProgressModule::SceneData::setSceneName(const Ogre::String& sceneName)
    {
        this->sceneName = sceneName;
    }

    Ogre::String GameProgressModule::SceneData::getSceneName(void) const
    {
        return this->sceneName;
    }

    Ogre::String GameProgressModule::SceneData::getTargetLocationName(void) const
    {
        return this->targetLocationName;
    }

} // namespace end
#include "NOWAPrecompiled.h"
#include "DotSceneImportModule.h"
#include "OgreConfigFile.h"
#include "OgreLodConfig.h"
#include "OgreLodStrategyManager.h"
#include "OgreMesh2Serializer.h"
#include "OgreMeshLodGenerator.h"
#include "OgreMeshManager2.h"
#include "OgrePixelCountLodStrategy.h"
#include "gameObject/ExitComponent.h"
#include "gameObject/LuaScriptComponent.h"
#include "gameobject/CameraComponent.h"
#include "gameobject/GameObjectController.h"
#include "gameobject/LightDirectionalComponent.h"
#include "gameobject/PhysicsActiveComponent.h"
#include "gameobject/PhysicsComponent.h"
#include "gameobject/PlanarReflectionComponent.h"
#include "gameobject/TerraComponent.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "modules/WorkspaceModule.h"
#include "utilities/MathHelper.h"
#include "utilities/MyGUIUtilities.h"
#include "utilities/XMLConverter.h"

#include "res/resource.h"

#include "DeployResourceModule.h"
#include "GameProgressModule.h"
#include "OgreNewtModule.h"

#include <filesystem>
#include <functional>

namespace
{
    // Attention: Sets AppStateManager::bStall for the duration of the import. While the flag is
    // set, the render thread skips renderOneFrame() and only services the command queue. That
    // turns an enqueueAndWait round trip from a full VSync frame (~16 ms) into microseconds.
    // Without it EVERY round trip waits for the running frame to finish - at roughly a dozen
    // round trips per game object and 100 objects that alone was well over 15 seconds of pure
    // waiting, which is what the [TIMING] measurements showed as 'enqueueWait=15.9'.
    //
    // Attention: bStall is also set and cleared by AppStateManager::internalChangeAppState,
    // internalPushAppState and the shutdown path. The guard therefore writes back the PREVIOUS
    // value instead of a blind false - otherwise an import that happens during a state change
    // would release that state's stall too early.
    //
    // Note on what bStall additionally disables while it is set:
    //   - isSafeToDispatchEvents() returns false, so events queued during the import are only
    //     dispatched afterwards. That is the intended behaviour here.
    //   - The main loop skips renderUpdate() and update(). Irrelevant, because the logic thread
    //     is blocked inside the import anyway and does not turn the loop.
    //   - advanceFrameAndDestroyOld() and updateAllTransforms() do not run, so deferred destroy
    //     commands pile up until the import finishes. Harmless for a load, but worth knowing
    //     when reloading a scene over an existing one.
    //   - No input capture and no MyGUI redraw: the window visibly freezes during the import.
    class SceneLoadingStallGuard
    {
    public:
        // Attention: the two callbacks are the loading indicator's begin/end. They run inside the
        // constructor resp. the destructor, so the indicator is torn down even if parsing throws.
        // Passing nullptr for either is allowed.
        SceneLoadingStallGuard(const std::function<void()>& onBegin, const std::function<void()>& onEnd) : onEnd(onEnd)
        {
            // Attention: This uses GraphicsModule's own suspend flag, NOT
            // AppStateManager::bStall and NOT GameProgressModule::bSceneLoading. Both of those
            // were tried first and neither reliably reached the render loop, so every
            // enqueueAndWait kept waiting for the running renderOneFrame(). The [TIMING-GO]
            // measurements showed this as a flat ~16 ms per round trip - four round trips per
            // game object, 63 ms each, 13 seconds for 100 objects.
            NOWA::GraphicsModule::getInstance()->suspendRendering(true);

            // Attention: AFTER suspendRendering, because beginLoadingIndicator dispatches a render
            // command and the render thread has to be servicing the queue for that to complete.
            if (nullptr != onBegin)
            {
                onBegin();
            }
        }

        ~SceneLoadingStallGuard()
        {
            // Attention: BEFORE suspendRendering(false), so the indicator's widgets and its
            // temporary camera are gone before the normal render loop resumes and starts
            // interpolating and rendering again.
            if (nullptr != this->onEnd)
            {
                this->onEnd();
            }

            NOWA::GraphicsModule::getInstance()->suspendRendering(false);
        }

        SceneLoadingStallGuard(const SceneLoadingStallGuard&) = delete;
        SceneLoadingStallGuard& operator=(const SceneLoadingStallGuard&) = delete;

    private:
        std::function<void()> onEnd;
    };
}

namespace NOWA
{
    DotSceneImportModule::DotSceneImportModule(Ogre::SceneManager* sceneManager) :
        sceneManager(sceneManager),
        ogreNewt(nullptr),
        mainCamera(nullptr),
        pagesCount(0),
        needCollisionRebuild(false),
        forceCreation(false),
        bSceneParsed(false),
        bIsSnapshot(false),
        bNewScene(false),
        mostLeftNearPosition(Ogre::Vector3(Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY)),
        mostRightFarPosition(Ogre::Vector3(Ogre::Math::NEG_INFINITY, Ogre::Math::NEG_INFINITY, Ogre::Math::NEG_INFINITY)),
        sunLight(nullptr),
        showLoadingIndicator(false),
        loadingIndicator(nullptr),
        ownedLoadingIndicator(nullptr),
        temporaryLoadingCamera(nullptr)
    {
    }

    DotSceneImportModule::DotSceneImportModule(Ogre::SceneManager* sceneManager, Ogre::Camera* mainCamera, OgreNewt::World* ogreNewt) :
        sceneManager(sceneManager),
        mainCamera(mainCamera),
        ogreNewt(ogreNewt),
        sunLight(nullptr),
        pagesCount(0),
        needCollisionRebuild(false),
        forceCreation(false),
        bSceneParsed(false),
        bIsSnapshot(false),
        bNewScene(false),
        mostLeftNearPosition(Ogre::Vector3(Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY)),
        mostRightFarPosition(Ogre::Vector3(Ogre::Math::NEG_INFINITY, Ogre::Math::NEG_INFINITY, Ogre::Math::NEG_INFINITY)),
        showLoadingIndicator(false),
        loadingIndicator(nullptr),
        ownedLoadingIndicator(nullptr),
        temporaryLoadingCamera(nullptr)
    {
        // Add delegates to be called when the corresponding event had fired
        AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &DotSceneImportModule::parseGameObjectDelegate), EventDataParseGameObject::getStaticEventType());
    }

    DotSceneImportModule::~DotSceneImportModule()
    {
        // Remove delegates
        AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &DotSceneImportModule::parseGameObjectDelegate), EventDataParseGameObject::getStaticEventType());
        // Do not destroy here, because scene loader is also used in undo command!!
        // AppStateManager::getSingletonPtr()->getGameObjectController()->destroyContent();
    }

    DotSceneImportModule::DotSceneImportModule(Ogre::SceneManager* sceneManager, const Ogre::String& projectName, const Ogre::String& sceneName, const Ogre::String& resourceGroupName) :
        sceneManager(sceneManager),
        mainCamera(nullptr),
        ogreNewt(nullptr),
        sunLight(nullptr),
        pagesCount(0),
        needCollisionRebuild(false),
        forceCreation(false),
        bSceneParsed(false),
        bIsSnapshot(false),
        bNewScene(false),
        mostLeftNearPosition(Ogre::Vector3(Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY)),
        mostRightFarPosition(Ogre::Vector3(Ogre::Math::NEG_INFINITY, Ogre::Math::NEG_INFINITY, Ogre::Math::NEG_INFINITY)),
        showLoadingIndicator(false),
        loadingIndicator(nullptr),
        ownedLoadingIndicator(nullptr),
        temporaryLoadingCamera(nullptr)
    {
        // Remove .scene
        Ogre::String tempSceneName = sceneName;
        size_t found = tempSceneName.find(".scene");
        if (found != std::wstring::npos)
        {
            tempSceneName = tempSceneName.substr(0, tempSceneName.size() - 6);
        }
        this->projectParameter.projectName = projectName;
        this->projectParameter.sceneName = tempSceneName;
        this->resourceGroupName = resourceGroupName;

        if (true == this->resourceGroupName.empty())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Error: Can not load scene, because the groupname is empty.");
            throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[DotSceneImportModule] Can not load scene, because the groupname is empty.\n", "NOWA");
        }

        Ogre::String projectPath = Core::getSingletonPtr()->getSectionPath(this->resourceGroupName)[0];

        // Project is always: "Projects/ProjectName/SceneName.scene"
        // E.g.: "Projects/Plattformer/Level1/Level1.scene", "Projects/Plattformer/Level2/Level2.scene", "Projects/Plattformer/Level3/Level3.scene"
        this->scenePath = projectPath + "/" + this->projectParameter.projectName + "/" + this->projectParameter.sceneName + "/" + this->projectParameter.sceneName + ".scene";
    }

    void DotSceneImportModule::parseGameObjectDelegate(EventDataPtr eventData)
    {
        boost::shared_ptr<NOWA::EventDataParseGameObject> castEventData = boost::static_pointer_cast<EventDataParseGameObject>(eventData);

        this->parsedGameObjectIds.clear();
        Ogre::String gameObjectName = castEventData->getGameObjectName();
        unsigned int controlledByClientID = castEventData->getControlledByClientID();

        if (!gameObjectName.empty())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DotSceneImportModule]: Parse game object from virtual environment for name: " + gameObjectName);
            this->forceCreation = true;
            this->parseGameObject(gameObjectName);
            this->forceCreation = false;
        }
        else if (0 != controlledByClientID)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DotSceneImportModule]: Parse game objects from virtual environment for controlled client id: " + Ogre::StringConverter::toString(controlledByClientID));
            this->forceCreation = true;
            this->parseGameObjects(controlledByClientID);
            this->forceCreation = false;
        }
    }

    bool DotSceneImportModule::parseGlobalScene(bool crypted)
    {
        rapidxml::xml_document<> XMLDoc;
        rapidxml::xml_node<>* xmlRoot;

        auto sections = Core::getSingletonPtr()->getSectionPath(this->resourceGroupName);
        if (true == sections.empty())
        {
            return false;
        }

        Ogre::String projectPath = sections[0];

        this->bSceneParsed = true;

        // Import global scene, if it does exist

        Ogre::String globalSceneFilePathName;

        if (true == this->savedGameFilePathName.empty())
        {
            globalSceneFilePathName = projectPath + "/" + this->projectParameter.projectName + "/global.scene";
        }
        else
        {
            Ogre::String projectFilePathName = Core::getSingletonPtr()->getProjectFilePathNameFromPath(this->savedGameFilePathName);
            globalSceneFilePathName = projectFilePathName + "/global.scene";
        }

        // Project is always: "Projects/ProjectName/global.scene"
        std::ifstream ifs(globalSceneFilePathName);
        // If it does not exist, then there are no global objects and it does not matter
        if (false == ifs.good())
        {
            return false;
        }
        std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));

        if (GetFileAttributes(globalSceneFilePathName.data()) & FileFlag || crypted)
        {
            content = Core::getSingletonPtr()->decode64(content, true);
        }
        content += '\0';

        boost::shared_ptr<EventDataProjectEncoded> eventDataProjectEncoded(new EventDataProjectEncoded(Core::getSingletonPtr()->projectEncoded));
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataProjectEncoded);

        try
        {
            XMLDoc.parse<0>(&content[0]);
        }
        catch (rapidxml::parse_error& error)
        {
            throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[DotSceneImportModule] Could not parse global scene. Error: " + Ogre::String(error.what()) + " at: " + Ogre::String(error.where<char>()) + "\n", "NOWA");
        }

        xmlRoot = XMLDoc.first_node("scene");
        if (nullptr == xmlRoot)
        {
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Error: Invalid global.scene File. Missing <scene>");
            return false;
        }
        if (XMLConverter::getAttrib(xmlRoot, "formatVersion", "") == "")
        {
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Error: Invalid global.scene File. Missing <scene>");
            return false;
        }

        // Process the global.scene
        this->processScene(xmlRoot);

        this->bSceneParsed = false;

        return true;
    }

    bool DotSceneImportModule::parseScene(const Ogre::String& projectName, const Ogre::String& sceneName, const Ogre::String& resourceGroupName, Ogre::Light* sunLight)
    {
        GraphicsModule::getInstance()->clearAllClosures();

        // Note: No crypted flag used, because if its a usual scene and shall be crypted, the whole project and all scene files will be crypted from the outside at once and also decoded at once.
        bool success = true;

        // Remove .scene
        Ogre::String tempSceneName = sceneName;
        size_t found = tempSceneName.find(".scene");
        if (found != std::wstring::npos)
        {
            tempSceneName = tempSceneName.substr(0, tempSceneName.size() - 6);
        }

        this->projectParameter.projectName = projectName;
        this->projectParameter.sceneName = tempSceneName;
        this->resourceGroupName = resourceGroupName;
        this->sunLight = sunLight;
        this->parsedGameObjectIds.clear();

        if (true == this->resourceGroupName.empty())
        {
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Error: Can not load scene, because the groupname is empty.");
            throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[DotSceneImportModule] Can not load scene, because the groupname is empty.\n", "NOWA");
        }

        Ogre::String projectPath = Core::getSingletonPtr()->getSectionPath(this->resourceGroupName)[0];

        // Project is always: "Projects/ProjectName/SceneName.scene"
        // E.g.: "Projects/Plattformer/Level1/Level1.scene", "Projects/Plattformer/Level2.scene", "Projects/Plattformer/Level3.scene"
        this->scenePath = projectPath + "/" + this->projectParameter.projectName + "/" + this->projectParameter.sceneName + "/" + this->projectParameter.sceneName + ".scene";

        if (false == std::filesystem::exists(this->scenePath))
        {
            return false;
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DotSceneImportModule]: Begin Parsing scene: '" + this->scenePath + "' for resource group: '" + resourceGroupName + "'");

        return this->internalParseScene(this->scenePath);
    }

    bool DotSceneImportModule::internalParseScene(const Ogre::String& filePathName, bool crypted)
    {
        // Attention: MUST come before anything that uses enqueueAndWait, and the guard has to
        // outlive the whole function - including postInitData(), which dispatches further render
        // commands per game object. See the class comment for what it does and what it disables.
        SceneLoadingStallGuard sceneLoadingStallGuard(
            [this]()
            {
                this->beginLoadingIndicator();
            },
            [this]()
            {
                this->endLoadingIndicator();
            });

        float currentTime = static_cast<Ogre::Real>(Core::getSingletonPtr()->getOgreTimer()->getMilliseconds()) * 0.001f;

        Core::getSingletonPtr()->preLoadSceneTextures(filePathName);
        // Attention: This used to log getMilliseconds() directly, which is the timer's ABSOLUTE
        // value since engine start, not a duration. It now logs the actual elapsed time.
        Ogre::LogManager::getSingleton().logMessage("[DotSceneImportModule] Texture preload: " + Ogre::StringConverter::toString((static_cast<Ogre::Real>(Core::getSingletonPtr()->getOgreTimer()->getMilliseconds()) * 0.001f) - currentTime) +
                                                    " seconds");

        Core::getSingletonPtr()->createFolders(this->scenePath);
        Core::getSingletonPtr()->setCurrentScenePath(this->scenePath);
        this->bSceneParsed = true;

        std::ifstream ifs(filePathName);
        if (false == ifs.good())
        {
            this->bNewScene = true;
            bool success = this->parseGlobalScene(crypted);
            if (true == success)
            {
                this->postInitData();
            }
            this->bNewScene = false;
            return success;
        }

        std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
        DWORD dwFileAttributes = GetFileAttributes(filePathName.data());
        if (dwFileAttributes & FileFlag || crypted)
        {
            content = Core::getSingletonPtr()->decode64(content, true);
            Core::getSingletonPtr()->projectEncoded = true;
        }
        else
        {
            Core::getSingletonPtr()->projectEncoded = false;
        }
        content += '\0';

        boost::shared_ptr<EventDataProjectEncoded> eventDataProjectEncoded(new EventDataProjectEncoded(Core::getSingletonPtr()->projectEncoded));
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataProjectEncoded);

        rapidxml::xml_document<> XMLDoc;
        rapidxml::xml_node<>* xmlRoot = nullptr;
        try
        {
            XMLDoc.parse<0>(&content[0]);
        }
        catch (rapidxml::parse_error& error)
        {
            throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[DotSceneImportModule] Could not parse scene: " + this->projectParameter.sceneName + " error: " + Ogre::String(error.what()) + " at: " + Ogre::String(error.where<char>()) + "\n",
                "NOWA");
        }

        xmlRoot = XMLDoc.first_node("scene");
        if (nullptr == xmlRoot)
        {
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Error: Invalid .scene File. Missing <scene>");
            return false;
        }
        if (XMLConverter::getAttrib(xmlRoot, "formatVersion", "") == "")
        {
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Error: Invalid .scene File. Missing formatVersion");
            return false;
        }

        // Pre-count nodes so the progress bar has a denominator
        size_t totalObjects = 0;
        {
            rapidxml::xml_node<>* nodesElem = xmlRoot->first_node("nodes");
            if (nullptr != nodesElem)
            {
                for (auto* n = nodesElem->first_node("node"); nullptr != n; n = n->next_sibling("node"))
                {
                    ++totalObjects;
                }
            }
        }
        Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[DotSceneImportModule] Scene node count: " + Ogre::StringConverter::toString(totalObjects));

        // Creates MyGUI widgets on the render thread (enqueueAndWait from main thread — safe during game loop)

        this->processScene(xmlRoot);

        if (false == this->projectParameter.ignoreGlobalScene)
        {
            this->parseGlobalScene(crypted);
        }

        NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
        {
            Ogre::TextureGpuManager* textureManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getTextureGpuManager();
            textureManager->waitForStreamingCompletion();
            this->postInitData();
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "DotSceneImportModule::waitForStreamingCompletion");

        this->bSceneParsed = false;
        this->savedGameFilePathName.clear();

        boost::shared_ptr<EventDataSceneParsed> eventDataSceneParsed(new EventDataSceneParsed());
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataSceneParsed);

        float dt = (static_cast<Ogre::Real>(Core::getSingletonPtr()->getOgreTimer()->getMilliseconds()) * 0.001f) - currentTime;
        Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[DotSceneImportModule] Parse end scene: " + this->projectParameter.sceneName + " duration: " + Ogre::StringConverter::toString(dt) + " seconds");

        return true;
    }

    bool NOWA::DotSceneImportModule::parseSceneSnapshot(const Ogre::String& projectName, const Ogre::String& sceneName, const Ogre::String& resourceGroupName, const Ogre::String& savedGameFilePathName, bool crypted)
    {
        // If a saved game shall be parsed, the user can say, whether everything is crypted and needs to be decoded.
        bool success = true;
        this->bIsSnapshot = true;
        this->bSceneParsed = true;

        float currentTime = static_cast<Ogre::Real>(Core::getSingletonPtr()->getOgreTimer()->getMilliseconds()) * 0.001f;

        rapidxml::xml_document<> XMLDoc;

        std::ifstream ifs(savedGameFilePathName);
        if (false == ifs.good())
        {
            success = false;
            return success;
        }

        this->projectParameter.projectName = projectName;
        this->projectParameter.sceneName = sceneName;
        this->resourceGroupName = resourceGroupName;
        this->savedGameFilePathName = savedGameFilePathName;
        this->parsedGameObjectIds.clear();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
            "[DotSceneImportModule]: Begin Parsing scene: '" + this->projectParameter.projectName + "/" + this->projectParameter.sceneName + ".scene' for resource group: '" + resourceGroupName + "'");

        return this->internalParseScene(savedGameFilePathName, crypted);
    }

    void DotSceneImportModule::postInitData()
    {
        auto mainCameraGameObject = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(NOWA::GameObjectController::MAIN_CAMERA_ID);
        if (nullptr == mainCameraGameObject && false == this->bNewScene)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Error: Can not load scene, because the MainCamera could not be created! See log for further information.");
            throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[DotSceneImportModule] Error: Can not load scene, because the MainCamera could not be created! See log for further information.\n", "NOWA");
        }
        else if (nullptr != mainCameraGameObject)
        {
            mainCameraGameObject->postInit();
            if (nullptr == this->mainCamera && false == this->bNewScene)
            {
                this->mainCamera = NOWA::makeStrongPtr(mainCameraGameObject->getComponent<CameraComponent>())->getCamera();
            }
        }

        auto mainLightGameObject = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(NOWA::GameObjectController::MAIN_LIGHT_ID);
        if (nullptr == mainLightGameObject && false == this->bNewScene)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Error: Can not load scene, because the MainLight could not be created! See log for further information.");
            throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[DotSceneImportModule] Error: Can not load scene, because the MainLight could not be created! See log for further information.\n", "NOWA");
        }
        else if (nullptr != mainLightGameObject)
        {
            mainLightGameObject->postInit();
            if (nullptr == this->sunLight)
            {
                this->sunLight = NOWA::makeStrongPtr(mainLightGameObject->getComponent<LightDirectionalComponent>())->getOgreLight();
            }
        }

        auto mainGameObject = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(NOWA::GameObjectController::MAIN_GAMEOBJECT_ID);
        if (nullptr == mainGameObject && false == this->bNewScene)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                "[DotSceneImportModule] Error: Can not load scene, because the MainGameObject could not be created. Maybe this is an old scene, which does not have a MainGameObject! See log for further information.");
            throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[DotSceneImportModule] Error: Can not load scene, because the MainGameObject could not be created! See log for further information.\n", "NOWA");
        }
        else if (nullptr != mainGameObject)
        {
            mainGameObject->postInit();
        }

        std::vector<GameObject*> clampGameObjects;

        auto gameObjects = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects();

        // Now that all gameobject's have been fully created, run the post init phase (now all other components are also available for each game object)
        for (auto it = gameObjects->cbegin(); it != gameObjects->cend(); ++it)
        {
            // Attention: postInitData() runs entirely inside ONE render command, so the render
            // thread never returns to the command pump loop while it is running - and that loop is
            // what normally drives the loading indicator. On a scene with many planets this phase
            // alone takes several seconds, during which the indicator visibly froze.
            // We are already ON the render thread here, so we can draw a frame directly. The call
            // throttles itself and is a no-op when no indicator is active.
            NOWA::GraphicsModule::getInstance()->renderLoadingFrameThrottled();

            const auto gameObjectPtr = it->second;
            if (gameObjectPtr->getId() != NOWA::GameObjectController::MAIN_CAMERA_ID && gameObjectPtr->getId() != NOWA::GameObjectController::MAIN_LIGHT_ID && gameObjectPtr->getId() != NOWA::GameObjectController::MAIN_GAMEOBJECT_ID)
            {
                if (false == gameObjectPtr->postInit())
                {
                    AppStateManager::getSingletonPtr()->getGameObjectController()->deleteGameObjectImmediately(gameObjectPtr->getId());
                }
                else if (true == gameObjectPtr->getClampY())
                {
                    clampGameObjects.emplace_back(gameObjectPtr.get());
                }
            }
        }

        for (const auto& clampGameObject : clampGameObjects)
        {
            if (clampGameObject->getId() != NOWA::GameObjectController::MAIN_CAMERA_ID && clampGameObject->getId() != NOWA::GameObjectController::MAIN_LIGHT_ID && clampGameObject->getId() != NOWA::GameObjectController::MAIN_GAMEOBJECT_ID)
            {
                // If everything is loaded, perform raycast for y clamping for each game object, which has the corresponding attribute activated
                clampGameObject->performRaycastForYClamping();
            }
        }

        if (AppStateManager::getSingletonPtr()->getOgreRecastModule()->hasNavigationMeshElements() && true == this->projectParameter.hasRecast)
        {
            // wrong transform values at this early stage
        }
        else
        {
            AppStateManager::getSingletonPtr()->getOgreRecastModule()->destroyContent();
        }

        // Set the bounds, to have it in core for public access
        Core::getSingletonPtr()->setCurrentSceneBounds(this->mostLeftNearPosition, this->mostRightFarPosition);

        if (false == NOWA::AppStateManager::getSingletonPtr()->getOgreRecastModule()->loadNavigationMesh())
        {
            // No .nav file yet — build from scratch and auto-save
            NOWA::GraphicsModule::getInstance()->renderLoadingFrameThrottled();
            NOWA::AppStateManager::getSingletonPtr()->getOgreRecastModule()->buildNavigationMesh();
            NOWA::GraphicsModule::getInstance()->renderLoadingFrameThrottled();
        }

        // After scene loading, when all initial GameObjects are registered.
        size_t gameObjectCount = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects()->size();
        if (gameObjectCount > 0)
        {
            AppStateManager::getSingletonPtr()->getGameObjectController()->reserveGameObjectCapacity(gameObjectCount);
        }
        NOWA::GraphicsModule::getInstance()->renderLoadingFrameThrottled();
        Core::getSingletonPtr()->setSettings(this->sceneManager, this->sunLight, NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera(), this->projectParameter, false);
        NOWA::GraphicsModule::getInstance()->renderLoadingFrameThrottled();
    }

    std::vector<unsigned long> DotSceneImportModule::parseGroup(const Ogre::String& fileName, const Ogre::String& resourceGroupName)
    {
        Ogre::String groupFileName = "Groups/" + fileName;

        this->parsedGameObjectIds.clear();
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DotSceneImportModule]: Parsing group: " + groupFileName);

        rapidxml::xml_document<> XMLDoc;

        Ogre::DataStreamPtr stream = Ogre::ResourceGroupManager::getSingleton().openResource(groupFileName, resourceGroupName);
        Ogre::String strScene = stream->getAsString();
        std::vector<char> sceneCopy(strScene.begin(), strScene.end());
        sceneCopy.emplace_back('\0');
        try
        {
            XMLDoc.parse<0>(&sceneCopy[0]);
        }
        catch (rapidxml::parse_error& error)
        {
            throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[DotSceneImportModule] Could not parse group: " + groupFileName + " error: " + Ogre::String(error.what()) + " at: " + Ogre::String(error.where<char>()) + "\n", "NOWA");
        }

        rapidxml::xml_node<>* pElement;

        pElement = XMLDoc.first_node("resourceLocations");
        if (pElement)
        {
            this->processResourceLocations(pElement);
        }

        pElement = XMLDoc.first_node("nodes");
        if (pElement)
        {
            this->processNodes(pElement);
        }

        // Post init all parsed game objects
        for (size_t i = 0; i < this->parsedGameObjectIds.size(); i++)
        {
            const auto gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->parsedGameObjectIds[i]);
            if (nullptr == gameObjectPtr)
            {
                continue;
            }

            GameObjectComponents* components = gameObjectPtr->getComponents();
            for (auto it = components->begin(); it != components->end(); ++it)
            {
                auto luaScriptCompPtr = boost::dynamic_pointer_cast<LuaScriptComponent>(std::get<COMPONENT>(*it));
                if (nullptr == luaScriptCompPtr)
                {
                    continue;
                }

                // Copy script from Groups folder to current scene folder.
                // The script in Groups now has the correct new IDs (fixed in exportGroup).
                // Only copy if the script does not already exist in the scene folder
                // (prevents overwriting a different game object's script with the same name).
                Ogre::ResourceGroupManager::LocationList resLocationsList = Ogre::ResourceGroupManager::getSingleton().getResourceLocationList(resourceGroupName);

                for (auto rit = resLocationsList.cbegin(); rit != resLocationsList.cend(); ++rit)
                {
                    Ogre::String groupsFilePath = (*rit)->archive->getName() + "/Groups";
                    Ogre::String scriptSourceFilePathName = groupsFilePath + "/" + luaScriptCompPtr->getScriptFile();
                    Ogre::String scriptDestFilePathName;

                    if (!gameObjectPtr->getGlobal())
                    {
                        scriptDestFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "/" + luaScriptCompPtr->getScriptFile();
                    }
                    else
                    {
                        scriptDestFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + luaScriptCompPtr->getScriptFile();
                    }

                    // Copy only if destination doesn't exist yet
                    std::ifstream destCheck(scriptDestFilePathName, std::ios::in);
                    if (!destCheck.good())
                    {
                        AppStateManager::getSingletonPtr()->getLuaScriptModule()->copyScriptAbsolutePath(scriptSourceFilePathName, scriptDestFilePathName, false, gameObjectPtr->getGlobal());
                    }
                    break;
                }

                luaScriptCompPtr->setComponentCloned(true);

                boost::shared_ptr<EventDataGroupLoaded> eventDataGroupLoaded(new EventDataGroupLoaded(gameObjectPtr->getId()));
                AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataGroupLoaded);
            }

            gameObjectPtr->postInit();
        }

        std::vector<unsigned long> tempParsedGameObjectIds = this->parsedGameObjectIds;
        this->parsedGameObjectIds.clear();
        return tempParsedGameObjectIds;
    }

    bool DotSceneImportModule::parseGameObject(const Ogre::String& name)
    {
        // do not show progress in gui because it will crash
        bool success = false;
        // get the xml file from the resourcegroup
        Ogre::DataStreamPtr stream = Ogre::ResourceGroupManager::getSingleton().openResource(this->projectParameter.projectName + "/" + this->projectParameter.sceneName + "/" + this->projectParameter.sceneName + ".scene", this->resourceGroupName);

        char* scene = _strdup(stream->getAsString().c_str());

        rapidxml::xml_document<> XMLDoc;
        // parse the xml document
        XMLDoc.parse<0>(scene);

        // go to the node, at which the scene nodes occur
        rapidxml::xml_node<>* XMLRoot = XMLDoc.first_node("scene");
        rapidxml::xml_node<>* nodesElement = XMLRoot->first_node("nodes");
        rapidxml::xml_node<>* nodeElement = nodesElement->first_node("node");
        // go through all nodes

        while (nodeElement)
        {
            if (nodeElement && name == nodeElement->first_attribute("name")->value())
            {
                // loads the game object internally into the game object controller
                this->processNode(nodeElement);
                success = true;
                break;
            }
            else
            {
                nodeElement = nodeElement->next_sibling("node");
            }
        }

        auto gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromName(name);
        if (nullptr != gameObjectPtr)
        {
            if (!gameObjectPtr->postInit())
            {
                AppStateManager::getSingletonPtr()->getGameObjectController()->deleteGameObjectImmediately(gameObjectPtr->getId());
            }
        }

        free(scene);
        return success;
    }

    bool DotSceneImportModule::parseGameObjects(unsigned int controlledByClientID)
    {
        // Do not show progress in gui because it will crash
        bool success = false;
        this->parsedGameObjectIds.clear();
        // Get the xml file from the resourcegroup
        Ogre::DataStreamPtr stream = Ogre::ResourceGroupManager::getSingleton().openResource(this->projectParameter.projectName + "/" + this->projectParameter.sceneName + "/" + this->projectParameter.sceneName + ".scene", this->resourceGroupName);

        char* scene = _strdup(stream->getAsString().c_str());

        rapidxml::xml_document<> XMLDoc;
        // Parse the xml document
        XMLDoc.parse<0>(scene);

        // Go to the node, at which the scene nodes occur
        rapidxml::xml_node<>* XMLRoot = XMLDoc.first_node("scene");
        rapidxml::xml_node<>* nodesElement = XMLRoot->first_node("nodes");
        rapidxml::xml_node<>* nodeElement = nodesElement->first_node("node");
        // Go through all nodes

        while (nodeElement)
        {
            // Search for the entity or item
            rapidxml::xml_node<>* entityElement = nodeElement->first_node("entity");
            if (nullptr == entityElement)
            {
                entityElement = nodeElement->first_node("item");
            }
            if (nullptr != entityElement)
            {
                rapidxml::xml_node<>* userDataElement = entityElement->first_node("userData");
                if (userDataElement)
                {
                    rapidxml::xml_node<>* propertyElement = userDataElement->first_node("property");

                    while (propertyElement && propertyElement->first_attribute("name")->value() != Ogre::String("ControlledByClient"))
                    {
                        propertyElement = propertyElement->next_sibling("property");
                    }

                    // If there is no ControlledByClient property, then all property elements had been visited and the element is still null,
                    // so go to the next node
                    if (!propertyElement)
                    {
                        nodeElement = nodeElement->next_sibling("node");
                        continue;
                    }

                    // Check if this game object matches the controlled client id
                    if (static_cast<unsigned int>(XMLConverter::getAttribReal(propertyElement, "data", 0)) == controlledByClientID)
                    {
                        // Loads the game object internally into the game object controller
                        this->processNode(nodeElement);
                        success = true;
                    }
                }
            }
            // Go to the next node
            nodeElement = nodeElement->next_sibling("node");
        }
        free(scene);

        // Now that all gameobject's have been fully created, run the post init phase (now all other components are also available for each game object)
        for (size_t i = 0; i < this->parsedGameObjectIds.size(); i++)
        {
            const auto gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->parsedGameObjectIds[i]);
            if (nullptr != gameObjectPtr)
            {
                if (!gameObjectPtr->postInit())
                {
                    AppStateManager::getSingletonPtr()->getGameObjectController()->deleteGameObjectImmediately(gameObjectPtr->getId());
                }
            }
        }

        return success;
    }

    std::pair<Ogre::Vector3, Ogre::Vector3> DotSceneImportModule::parseBounds(void)
    {
        // Do not show progress in gui because it will crash
        bool success = false;
        // Get the xml file from the resourcegroup
        Ogre::DataStreamPtr stream = Ogre::ResourceGroupManager::getSingleton().openResource(this->projectParameter.projectName + "/" + this->projectParameter.sceneName + "/" + this->projectParameter.sceneName + ".scene", this->resourceGroupName);

        char* scene = _strdup(stream->getAsString().c_str());

        rapidxml::xml_document<> XMLDoc;
        // Parse the xml document
        XMLDoc.parse<0>(scene);

        // Go to the node, at which the scene nodes occur
        rapidxml::xml_node<>* XMLRoot = XMLDoc.first_node("scene");
        rapidxml::xml_node<>* XMLEnvironment = XMLRoot->first_node("environment");
        rapidxml::xml_node<>* boundsElement = XMLEnvironment->first_node("bounds");

        if (nullptr != boundsElement)
        {
            this->mostLeftNearPosition = XMLConverter::getAttribVector3(boundsElement, "mostLeftNearPosition", Ogre::Vector3(Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY));
            this->mostRightFarPosition = XMLConverter::getAttribVector3(boundsElement, "mostRightFarPosition", Ogre::Vector3(Ogre::Math::NEG_INFINITY, Ogre::Math::NEG_INFINITY, Ogre::Math::NEG_INFINITY));
        }
        free(scene);
        return std::make_pair(this->mostLeftNearPosition, this->mostRightFarPosition);
    }

    std::vector<std::pair<Ogre::Vector2, Ogre::String>> DotSceneImportModule::parseExitDirectionsNextScenes(void)
    {
        std::vector<std::pair<Ogre::Vector2, Ogre::String>> exitDirectionsNextScenes;

        bool success = false;
        // Get the xml file from the resourcegroup
        Ogre::DataStreamPtr stream = Ogre::ResourceGroupManager::getSingleton().openResource(this->projectParameter.projectName + "/" + this->projectParameter.sceneName + "/" + this->projectParameter.sceneName + ".scene", this->resourceGroupName);

        char* scene = _strdup(stream->getAsString().c_str());

        rapidxml::xml_document<> XMLDoc;
        // Parse the xml document
        XMLDoc.parse<0>(scene);

        // Go to the node, at which the scene nodes occur
        rapidxml::xml_node<>* XMLRoot = XMLDoc.first_node("scene");
        rapidxml::xml_node<>* nodesElement = XMLRoot->first_node("nodes");
        rapidxml::xml_node<>* nodeElement = nodesElement->first_node("node");
        // Go through all nodes

        while (nodeElement)
        {
            // Search for the entity or item
            rapidxml::xml_node<>* entityElement = nodeElement->first_node("entity");
            if (nullptr == entityElement)
            {
                entityElement = nodeElement->first_node("item");
            }
            if (nullptr != entityElement)
            {
                rapidxml::xml_node<>* userDataElement = entityElement->first_node("userData");
                if (userDataElement)
                {
                    rapidxml::xml_node<>* propertyElement = userDataElement->first_node("property");

                    while (propertyElement && propertyElement->first_attribute("data")->value() != ExitComponent::getStaticClassName())
                    {
                        propertyElement = propertyElement->next_sibling("property");
                    }

                    if (!propertyElement)
                    {
                        nodeElement = nodeElement->next_sibling("node");
                        continue;
                    }

                    std::pair<Ogre::Vector2, Ogre::String> data;
                    bool foundExitDirection = false;
                    bool foundTargetSceneName = false;

                    while (nullptr != propertyElement && (false == foundExitDirection || false == foundTargetSceneName))
                    {
                        if (XMLConverter::getAttrib(propertyElement, "name") == "TargetSceneName")
                        {
                            data.second = XMLConverter::getAttrib(propertyElement, "data");
                            foundTargetSceneName = true;
                        }
                        else if (XMLConverter::getAttrib(propertyElement, "name") == "ExitDirection")
                        {
                            data.first = XMLConverter::getAttribVector2(propertyElement, "data");
                            foundExitDirection = true;
                        }
                        propertyElement = propertyElement->next_sibling("property");
                    }

                    if (true == foundExitDirection && true == foundTargetSceneName)
                    {
                        exitDirectionsNextScenes.emplace_back(data);
                    }
                }
            }
            // Go to the next node
            nodeElement = nodeElement->next_sibling("node");
        }
        free(scene);

        return exitDirectionsNextScenes;
    }

    std::pair<bool, Ogre::Vector3> DotSceneImportModule::parseGameObjectPosition(unsigned long id)
    {
        Ogre::Vector3 gameObjectPosition = Ogre::Vector3::ZERO;

        bool success = false;
        // Get the xml file from the resourcegroup
        Ogre::DataStreamPtr stream = Ogre::ResourceGroupManager::getSingleton().openResource(this->projectParameter.projectName + "/" + this->projectParameter.sceneName + "/" + this->projectParameter.sceneName + ".scene", this->resourceGroupName);

        char* scene = _strdup(stream->getAsString().c_str());

        rapidxml::xml_document<> XMLDoc;
        // Parse the xml document
        XMLDoc.parse<0>(scene);

        // Go to the node, at which the scene nodes occur
        rapidxml::xml_node<>* XMLRoot = XMLDoc.first_node("scene");
        rapidxml::xml_node<>* nodesElement = XMLRoot->first_node("nodes");
        rapidxml::xml_node<>* nodeElement = nodesElement->first_node("node");

        // Go through all nodes
        while (nodeElement && false == success)
        {
            // Search for the entity or item
            rapidxml::xml_node<>* entityElement = nodeElement->first_node("entity");
            if (nullptr == entityElement)
            {
                entityElement = nodeElement->first_node("item");
            }
            if (nullptr != entityElement)
            {
                rapidxml::xml_node<>* userDataElement = entityElement->first_node("userData");
                if (userDataElement)
                {
                    rapidxml::xml_node<>* propertyElement = userDataElement->first_node("property");

                    while (propertyElement)
                    {
                        if (propertyElement->first_attribute("name")->value() != "Id")
                        {
                            propertyElement = propertyElement->next_sibling("property");
                        }
                        else
                        {
                            unsigned long tempId = XMLConverter::getAttribUnsignedLong(propertyElement, "data");
                            if (tempId == id)
                            {
                                rapidxml::xml_node<>* positionElement = nodeElement->first_node("position");
                                if (positionElement)
                                {
                                    gameObjectPosition = XMLConverter::getAttribVector3(propertyElement, "data");
                                    success = true;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
            // Go to the next node
            nodeElement = nodeElement->next_sibling("node");
        }
        free(scene);

        return std::make_pair(success, gameObjectPosition);
    }

    void DotSceneImportModule::processScene(rapidxml::xml_node<>* xmlRoot, bool justSetValues)
    {
        bool skip = false;
        // Process the scene parameters
        Ogre::String version = XMLConverter::getAttrib(xmlRoot, "formatVersion", "unknown");
        size_t found = version.find(NOWA_DOT_SCENE_FILEVERSION_STR);
        if (found == Ogre::String::npos)
        {
            Ogre::String message = "This scene has been created with an older version, it may be that some components will not work correctly! Please check the log and especially when using id's of other components!";
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, message);
            boost::shared_ptr<EventDataFeedback> eventDataFeedback(new EventDataFeedback(false, message));
            NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataFeedback);
        }

        Ogre::String message = "[DotSceneImportModule] Parsing Scene file with version " + version;
        if (xmlRoot->first_attribute("ID"))
        {
            message += ", id " + Ogre::String(xmlRoot->first_attribute("ID")->value());
        }
        if (xmlRoot->first_attribute("sceneManager"))
        {
            message += ", scene manager " + Ogre::String(xmlRoot->first_attribute("sceneManager")->value());
        }
        if (xmlRoot->first_attribute("minOgreVersion"))
        {
            message += ", min. Ogre version " + Ogre::String(xmlRoot->first_attribute("minOgreVersion")->value());
        }
        if (xmlRoot->first_attribute("author"))
        {
            message += ", author " + Ogre::String(xmlRoot->first_attribute("author")->value());
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, message);

        rapidxml::xml_node<>* pElement;

        // Process resource locations (?)
        pElement = xmlRoot->first_node("resourceLocations");
        if (pElement)
        {
            this->processResourceLocations(pElement);
        }

        // Process environment (?)
        pElement = xmlRoot->first_node("environment");
        if (pElement)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this, &pElement]()
            {
                this->processEnvironment(pElement);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "DotSceneImportModule::processEnvironment");
        }

        // Process OgreNewt
        pElement = xmlRoot->first_node("OgreNewt");
        if (nullptr != pElement)
        {
            this->processOgreNewt(pElement);
        }

        // Process OgreRecast
        pElement = xmlRoot->first_node("OgreRecast");
        if (nullptr != pElement)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this, &pElement]()
            {
                this->processOgreRecast(pElement);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "DotSceneImportModule::processOgreRecast");
        }

        // Process nodes (?)
        pElement = xmlRoot->first_node("nodes");
        if (nullptr != pElement)
        {
            this->processNodes(pElement, nullptr, justSetValues);
        }
    }

    void DotSceneImportModule::processResourceLocations(rapidxml::xml_node<>* xmlNode)
    {
        rapidxml::xml_node<>* pElement;

        // resourceGroupName, type, path
        std::vector<std::tuple<Ogre::String, Ogre::String, Ogre::String>> missingResourceGroupNamesForScene;

        pElement = xmlNode->first_node("resourceLocation");
        while (pElement)
        {
            Ogre::String usedName = pElement->first_attribute("name")->value();
            Ogre::String usedType = pElement->first_attribute("type")->value();
            Ogre::String usedPath = pElement->first_attribute("path")->value();

            // Get all currently defined resource group names
            auto resourceLocations = Ogre::ResourceGroupManager::getSingleton().getResourceGroups();
            bool foundResourceGroupName = false;
            bool foundResourceGroupPath = false;
            for (const auto& resourceGroupName : resourceLocations)
            {
                if (usedName == resourceGroupName)
                {
                    foundResourceGroupName = true;
                    // here no break, because a resource group may match, but it may have several locations and it could be that a location is missing
                }
                for (const auto& path : Ogre::ResourceGroupManager::getSingleton().getResourceLocationList(resourceGroupName))
                {
                    if (usedPath == path->archive->getName())
                    {
                        foundResourceGroupPath = true;
                        break;
                    }
                }

                if (true == foundResourceGroupPath)
                {
                    break;
                }
            }

            // Check if a resource is not defined in the corresponding cfg file and add it to the missing list, to late-initialize the resource, so that the scene can be loaded properly
            if (false == foundResourceGroupName || false == foundResourceGroupPath)
            {
                missingResourceGroupNamesForScene.emplace_back(usedName, usedType, usedPath);
            }

            pElement = pElement->next_sibling("resourceLocation");
        }

        for (size_t i = 0; i < missingResourceGroupNamesForScene.size(); i++)
        {
            Ogre::ResourceGroupManager::getSingleton().addResourceLocation(std::get<2>(missingResourceGroupNamesForScene[i]), std::get<1>(missingResourceGroupNamesForScene[i]), std::get<0>(missingResourceGroupNamesForScene[i]));

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Warning: The resource group: '" + std::get<0>(missingResourceGroupNamesForScene[i]) + "' or its location: '" +
                                                                                    std::get<2>(missingResourceGroupNamesForScene[i]) +
                                                                                    "' has not been defined. It will now be initialized during level loading progress. "
                                                                                    "This may take some time! Please define the data in the corresponding cfg file, so that all necessary resources are loaded at the starup of the application!");
            Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup(std::get<0>(missingResourceGroupNamesForScene[i]), false);
        }
    }

    void DotSceneImportModule::processEnvironment(rapidxml::xml_node<>* xmlNode)
    {
        rapidxml::xml_node<>* pElement;
        bool skip = false;

        // Process modules (user data of scene manager) (?)
        pElement = xmlNode->first_node("scenemanager");
        if (pElement)
        {
            Ogre::String name = pElement->first_attribute("name")->value();
            // Scenemanager has no name anymore
        }

        // Colour ambient
        {
            pElement = xmlNode->first_node("ambient");
            if (pElement)
            {
                rapidxml::xml_node<>* subElement = pElement->first_node("ambientLightUpperHemisphere");
                if (subElement)
                {
                    this->projectParameter.ambientLightUpperHemisphere = XMLConverter::parseColour(subElement);
                }
                subElement = pElement->first_node("ambientLightLowerHemisphere");
                if (subElement)
                {
                    this->projectParameter.ambientLightLowerHemisphere = XMLConverter::parseColour(subElement);
                }
                subElement = pElement->first_node("hemisphereDir");
                if (subElement)
                {
                    this->projectParameter.hemisphereDir = XMLConverter::parseVector3(subElement);
                }
                subElement = pElement->first_node("envmapScale");
                if (subElement)
                {
                    this->projectParameter.envmapScale = XMLConverter::getAttribReal(subElement, "envmapScale", 1.0f);
                }
            }
        }

        // Shadows
        {
            pElement = xmlNode->first_node("shadows");
            if (pElement)
            {
                rapidxml::xml_node<>* subElement = pElement->first_node("shadowFarDistance");
                if (subElement)
                {
                    this->projectParameter.shadowFarDistance = XMLConverter::getAttribReal(subElement, "distance", 50.0f);
                }
                subElement = pElement->first_node("shadowDirectionalLightExtrusionDistance");
                if (subElement)
                {
                    this->projectParameter.shadowDirectionalLightExtrusionDistance = XMLConverter::getAttribReal(subElement, "distance", 50.0f);
                }
                subElement = pElement->first_node("shadowDirLightTextureOffset");
                if (subElement)
                {
                    this->projectParameter.shadowDirLightTextureOffset = XMLConverter::getAttribReal(subElement, "offset", 0.0f);
                }
                subElement = pElement->first_node("shadowColor");
                if (subElement)
                {
                    this->projectParameter.shadowColor.r = XMLConverter::getAttribReal(subElement, "r", 0.25f);
                    this->projectParameter.shadowColor.g = XMLConverter::getAttribReal(subElement, "g", 0.25f);
                    this->projectParameter.shadowColor.b = XMLConverter::getAttribReal(subElement, "b", 0.25f);
                }
                subElement = pElement->first_node("shadowQuality");
                if (subElement)
                {
                    this->projectParameter.shadowQualityIndex = XMLConverter::getAttribInt(subElement, "index", 2);
                }
                subElement = pElement->first_node("ambientLightMode");
                if (subElement)
                {
                    this->projectParameter.ambientLightModeIndex = XMLConverter::getAttribInt(subElement, "index", 0);
                }
            }
        }

        // Forward mode
        {
            pElement = xmlNode->first_node("lightFoward");
            if (pElement)
            {
                rapidxml::xml_node<>* subElement = pElement->first_node("forwardMode");
                if (subElement)
                {
                    this->projectParameter.forwardMode = XMLConverter::getAttribInt(subElement, "value", 0);
                }
                subElement = pElement->first_node("lightWidth");
                if (subElement)
                {
                    this->projectParameter.lightWidth = XMLConverter::getAttribInt(subElement, "value", 0);
                }
                subElement = pElement->first_node("lightHeight");
                if (subElement)
                {
                    this->projectParameter.lightHeight = XMLConverter::getAttribInt(subElement, "value", 0);
                }
                subElement = pElement->first_node("numLightSlices");
                if (subElement)
                {
                    this->projectParameter.numLightSlices = XMLConverter::getAttribInt(subElement, "value", 0);
                }
                subElement = pElement->first_node("lightsPerCell");
                if (subElement)
                {
                    this->projectParameter.lightsPerCell = XMLConverter::getAttribInt(subElement, "value", 0);
                }
                subElement = pElement->first_node("decalsPerCell");
                if (subElement)
                {
                    this->projectParameter.decalsPerCell = XMLConverter::getAttribInt(subElement, "value", 0);
                }
                subElement = pElement->first_node("cubemapProbesPerCell");
                if (subElement)
                {
                    this->projectParameter.cubemapProbesPerCell = XMLConverter::getAttribInt(subElement, "value", 0);
                }
                subElement = pElement->first_node("minLightDistance");
                if (subElement)
                {
                    this->projectParameter.minLightDistance = XMLConverter::getAttribReal(subElement, "value", 0.0f);
                }
                subElement = pElement->first_node("maxLightDistance");
                if (subElement)
                {
                    this->projectParameter.maxLightDistance = XMLConverter::getAttribReal(subElement, "value", 500.0f);
                }

                // Setting to scenemanager is done in postInitData setSettings...
            }
            else
            {
                // No forward parameter, set default
                this->projectParameter.forwardMode = 0;
            }
        }

        // Main Parameter
        {
            pElement = xmlNode->first_node("mainParameter");
            if (pElement)
            {
                rapidxml::xml_node<>* subElement = pElement->first_node("ignoreGlobalScene");
                if (subElement)
                {
                    this->projectParameter.ignoreGlobalScene = XMLConverter::getAttribBool(subElement, "value");
                }

                subElement = pElement->first_node("renderDistance");
                if (subElement)
                {
                    this->projectParameter.renderDistance = XMLConverter::getAttribReal(subElement, "renderDistance", Core::getSingletonPtr()->getGlobalRenderDistance());
                }
            }
        }

        pElement = xmlNode->first_node("bounds");
        if (nullptr != pElement)
        {
            this->mostLeftNearPosition = XMLConverter::getAttribVector3(pElement, "mostLeftNearPosition", Ogre::Vector3(Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY));
            this->mostRightFarPosition = XMLConverter::getAttribVector3(pElement, "mostRightFarPosition", Ogre::Vector3(Ogre::Math::NEG_INFINITY, Ogre::Math::NEG_INFINITY, Ogre::Math::NEG_INFINITY));
        }
    }

    void DotSceneImportModule::processOgreNewt(rapidxml::xml_node<>* xmlNode)
    {
        this->projectParameter.hasPhysics = true;
        // Process attributes
        this->projectParameter.solverModel = XMLConverter::getAttribInt(xmlNode, "solverModel", 1);
        // Friction model is no more used in ogre
        this->projectParameter.solverForSingleIsland = XMLConverter::getAttribInt(xmlNode, "multithreadSolverOnSingleIsland", 1);
        this->projectParameter.broadPhaseAlgorithm = XMLConverter::getAttribInt(xmlNode, "broadPhaseAlgorithm", 0);
        this->projectParameter.physicsThreadCount = XMLConverter::getAttribInt(xmlNode, "threadCount", 1);
        this->projectParameter.physicsUpdateRate = XMLConverter::getAttribReal(xmlNode, "desiredFps", 60.0f);
        this->projectParameter.linearDamping = XMLConverter::getAttribReal(xmlNode, "defaultLinearDamping", 0.1f);
        this->projectParameter.gravity = Ogre::Vector3(0.0f, -19.8f, 0.0f);
        this->projectParameter.angularDamping = Ogre::Vector3(0.01f, 0.01f, 0.01f);
        rapidxml::xml_node<>* pElement;
        pElement = xmlNode->first_node("defaultAngularDamping");
        if (nullptr != pElement)
        {
            this->projectParameter.angularDamping = XMLConverter::parseVector3(pElement);
        }

        pElement = xmlNode->first_node("globalGravity");
        if (nullptr != pElement)
        {
            this->projectParameter.gravity = XMLConverter::parseVector3(pElement);
        }

        AppStateManager::getSingletonPtr()->getOgreNewtModule()->setGlobalGravity(this->projectParameter.gravity);
        this->ogreNewt = AppStateManager::getSingletonPtr()->getOgreNewtModule()->createPhysics(AppStateManager::getSingletonPtr()->getCurrentAppStateName() + "_world", this->projectParameter.solverModel, this->projectParameter.broadPhaseAlgorithm,
            this->projectParameter.solverForSingleIsland, this->projectParameter.physicsThreadCount, this->projectParameter.physicsUpdateRate, this->projectParameter.linearDamping, this->projectParameter.angularDamping);
    }

    void DotSceneImportModule::processOgreRecast(rapidxml::xml_node<>* xmlNode)
    {
        if (nullptr != xmlNode)
        {
            this->projectParameter.hasRecast = true;

            this->projectParameter.cellSize = XMLConverter::getAttribReal(xmlNode, "CellSize", 0.6f);
            this->projectParameter.cellHeight = XMLConverter::getAttribReal(xmlNode, "CellHeight", 0.2f);
            this->projectParameter.agentMaxSlope = XMLConverter::getAttribReal(xmlNode, "AgentMaxSlope", 45);
            this->projectParameter.agentMaxClimb = XMLConverter::getAttribReal(xmlNode, "AgentMaxClimb", 2.5);
            this->projectParameter.agentHeight = XMLConverter::getAttribReal(xmlNode, "AgentHeight", 1);
            this->projectParameter.agentRadius = XMLConverter::getAttribReal(xmlNode, "AgentRadius", 0.8f);
            this->projectParameter.edgeMaxLen = XMLConverter::getAttribReal(xmlNode, "EdgeMaxLen", 12);
            this->projectParameter.edgeMaxError = XMLConverter::getAttribReal(xmlNode, "EdgeMaxError", 1.3f);
            this->projectParameter.regionMinSize = XMLConverter::getAttribReal(xmlNode, "RegionMinSize", 50);
            this->projectParameter.regionMergeSize = XMLConverter::getAttribReal(xmlNode, "RegionMergeSize", 20);
            this->projectParameter.vertsPerPoly = XMLConverter::getAttribInt(xmlNode, "VertsPerPoly", 5);
            this->projectParameter.detailSampleDist = XMLConverter::getAttribReal(xmlNode, "DetailSampleDist", 6);
            this->projectParameter.detailSampleMaxError = XMLConverter::getAttribReal(xmlNode, "DetailSampleMaxError", 1);
            this->projectParameter.keepInterResults = XMLConverter::getAttribBool(xmlNode, "KeepInterResults", false);

            OgreRecastConfigParams params;
            params.setCellSize(this->projectParameter.cellSize);
            params.setCellHeight(this->projectParameter.cellHeight);
            params.setAgentMaxSlope(this->projectParameter.agentMaxSlope);
            params.setAgentMaxClimb(this->projectParameter.agentMaxClimb);
            params.setAgentHeight(this->projectParameter.agentHeight);
            params.setAgentRadius(this->projectParameter.agentRadius);
            params.setEdgeMaxLen(this->projectParameter.edgeMaxLen);
            params.setEdgeMaxError(this->projectParameter.edgeMaxError);
            params.setRegionMinSize(this->projectParameter.regionMergeSize);
            params.setRegionMergeSize(this->projectParameter.regionMergeSize);
            params.setVertsPerPoly(this->projectParameter.vertsPerPoly);
            params.setDetailSampleDist(this->projectParameter.detailSampleDist);
            params.setDetailSampleMaxError(this->projectParameter.detailSampleMaxError);
            params.setKeepInterResults(this->projectParameter.keepInterResults);

            rapidxml::xml_node<>* pElement;
            pElement = xmlNode->first_node("PointExtends");
            this->projectParameter.pointExtends = XMLConverter::parseVector3(pElement);

            AppStateManager::getSingletonPtr()->getOgreRecastModule()->createOgreRecast(this->sceneManager, params, this->projectParameter.pointExtends);
        }
    }

    void DotSceneImportModule::processNodes(rapidxml::xml_node<>* xmlNode, Ogre::SceneNode* parent, bool justSetValues)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DotSceneImportModule] Parse nodes");
        float currentTime = static_cast<Ogre::Real>(Core::getSingletonPtr()->getOgreTimer()->getMilliseconds()) * 0.001f;

        rapidxml::xml_node<>* pElement;

        // Process node (*)
        pElement = xmlNode->first_node("node");
        while (pElement)
        {
            this->processNode(pElement, parent, justSetValues);
            pElement = pElement->next_sibling("node");
        }

        if (false == this->missingGameObjectIds.empty())
        {
            for (size_t i = 0; i < this->missingGameObjectIds.size(); i++)
            {
                // If its just for snapshot values but the game object was destroyed during simulation, post init must be called at last!

                // Now that the gameobject has been fully created, run the post init phase
                GameObjectPtr gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->missingGameObjectIds[i]);
                if (nullptr == gameObjectPtr)
                {
                    Ogre::String message =
                        "[DotSceneImportModule] Cannot undo deletion of game object id: " + Ogre::StringConverter::toString(this->missingGameObjectIds[i]) + " because somehow the game object has not been snapshotted as the simulation mode started.";
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, message);
                    throw Ogre::Exception(Ogre::Exception::ERR_ITEM_NOT_FOUND, message + "\n", "NOWA");
                }
                if (false == gameObjectPtr->postInit())
                {
                    AppStateManager::getSingletonPtr()->getGameObjectController()->deleteGameObjectImmediately(gameObjectPtr->getId());
                }
            }
        }

        this->missingGameObjectIds.clear();

        float dt = (static_cast<Ogre::Real>(Core::getSingletonPtr()->getOgreTimer()->getMilliseconds()) * 0.001f) - currentTime;
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DotSceneImportModule] Parse end nodes duration: " + Ogre::StringConverter::toString(dt) + " seconds.");
    }

    void DotSceneImportModule::processNode(rapidxml::xml_node<>* xmlNode, Ogre::SceneNode* parent, bool justSetValues)
    {
        Ogre::String nodeName = XMLConverter::getAttrib(xmlNode, "name", "unknown");
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DotSceneImportModule] Processing node: " + nodeName);

        Ogre::String name = XMLConverter::getAttrib(xmlNode, "name");

        Ogre::SceneNode* pNode = nullptr;
        bool foundNode = false;

        // Snapshot is loaded, scene nodes should exist already, find the node
        if (false == name.empty() && true == this->bIsSnapshot)
        {
            // Damn it, scene node names are not unique!
            const auto nodesList = this->sceneManager->findSceneNodes(name);
            if (false == nodesList.empty())
            {
                if (1 == nodesList.size())
                {
                    pNode = nodesList[0];
                    foundNode = true;
                }
                else
                {
                    for (size_t i = 0; i < nodesList.size(); i++)
                    {
                        Ogre::Node* node = nodesList[i];
                        if (nullptr == node)
                        {
                            continue;
                        }

                        const Ogre::Any& userAny = node->getUserObjectBindings().getUserAny();
                        if (userAny.isEmpty())
                        {
                            continue;
                        }

                        GameObject* gameObject = nullptr;
                        try
                        {
                            gameObject = Ogre::any_cast<GameObject*>(userAny);
                        }
                        catch (Ogre::Exception&)
                        {
                            continue;
                        }

                        if (gameObject->getName() == name)
                        {
                            pNode = nodesList[i];
                            foundNode = true;
                            break;
                        }
                    }
                }
            }
        }

        if (false == foundNode && false == name.empty())
        {
            // Must not set values, because node does not exist and game object does also not exist and must be created!
            justSetValues = false;
        }

        // Attention: The transform is parsed HERE, on the calling thread, and only the Ogre calls
        // go into the command. The former version issued four separate enqueueAndWait calls per node
        // (create, position, rotation, scale), i.e. four full blocking round trips to the render
        // thread for work that takes microseconds. With 100 nodes that alone was 400 round trips.
        const bool hasPosition = (nullptr != xmlNode->first_node("position"));
        const bool hasRotation = (nullptr != xmlNode->first_node("rotation"));
        const bool hasScale = (nullptr != xmlNode->first_node("scale"));

        const Ogre::Vector3 position = hasPosition ? XMLConverter::parseVector3(xmlNode->first_node("position")) : Ogre::Vector3::ZERO;
        const Ogre::Quaternion orientation = hasRotation ? XMLConverter::parseQuaternion(xmlNode->first_node("rotation")) : Ogre::Quaternion::IDENTITY;
        const Ogre::Vector3 scale = hasScale ? XMLConverter::parseVector3(xmlNode->first_node("scale")) : Ogre::Vector3::UNIT_SCALE;

        // Attention: pNode is captured by reference on purpose - the command writes the created node
        // back into this stack frame. That is ONLY safe as long as enqueueAndWait really blocks until
        // the command ran. See the note on the timeout path in GraphicsModule::enqueueAndWait: a
        // version that gives up on a live render thread turns this into a use-after-free.
        NOWA::GraphicsModule::RenderCommand renderCommand = [this, parent, name, foundNode, hasPosition, hasRotation, hasScale, position, orientation, scale, &pNode]()
        {
            if (false == foundNode)
            {
                pNode = (nullptr != parent) ? parent->createChildSceneNode(Ogre::SCENE_STATIC) : this->sceneManager->getRootSceneNode()->createChildSceneNode(Ogre::SCENE_STATIC);

                if (false == name.empty())
                {
                    pNode->setName(name);
                }
            }

            if (nullptr == pNode)
            {
                return;
            }

            if (true == hasPosition)
            {
                pNode->setPosition(position);
            }
            if (true == hasRotation)
            {
                pNode->setOrientation(orientation);
            }
            if (true == hasScale)
            {
                pNode->setScale(scale);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "DotSceneImportModule::processNode");

        if (nullptr == pNode)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Error: Could not create scene node for: " + nodeName);
            return;
        }

        rapidxml::xml_node<>* pElement;

        // Process node (*)
        pElement = xmlNode->first_node("node");
        while (pElement)
        {
            this->processNode(pElement, pNode, justSetValues);
            pElement = pElement->next_sibling("node");
        }

        // Process item (*)
        pElement = xmlNode->first_node("item");
        while (pElement)
        {
            this->processItem(pElement, pNode, justSetValues);
            pElement = pElement->next_sibling("item");
        }

        // Process terra (*)
        pElement = xmlNode->first_node("terra");
        while (pElement)
        {
            this->processTerra(pElement, pNode, justSetValues);
            pElement = pElement->next_sibling("terra");
        }

        // Process ocean (*)
        pElement = xmlNode->first_node("ocean");
        while (pElement)
        {
            this->processOcean(pElement, pNode, justSetValues);
            pElement = pElement->next_sibling("ocean");
        }

        // Process plane (*)
        pElement = xmlNode->first_node("plane");
        while (pElement)
        {
            this->processPlane(pElement, pNode, justSetValues);
            pElement = pElement->next_sibling("plane");
        }
    }

    void NOWA::DotSceneImportModule::setMissingGameObjectIds(const std::vector<unsigned long>& missingGameObjectIds)
    {
        this->missingGameObjectIds = missingGameObjectIds;
    }

    void DotSceneImportModule::findGameObjectId(rapidxml::xml_node<>*& propertyElement, unsigned long& missingGameObjectId)
    {
        if (nullptr != propertyElement)
        {
            bool found = false;
            do
            {
                Ogre::String attrib = XMLConverter::getAttrib(propertyElement, "name");
                if (propertyElement && attrib == "Id")
                {
                    unsigned long tempMissingGameObjectId = XMLConverter::getAttribUnsignedLong(propertyElement, "data");

                    for (size_t i = 0; i < this->missingGameObjectIds.size(); i++)
                    {
                        if (tempMissingGameObjectId == this->missingGameObjectIds[i])
                        {
                            missingGameObjectId = tempMissingGameObjectId;
                            found = true;
                        }
                        if (true == found)
                        {
                            break;
                        }
                    }
                    if (true == found)
                    {
                        break;
                    }
                }
                propertyElement = propertyElement->next_sibling("property");

            } while (nullptr != propertyElement && false == found);
        }
    }

    void DotSceneImportModule::processItem(rapidxml::xml_node<>* xmlNode, Ogre::SceneNode* parent, bool justSetValues)
    {
        // Process attributes
        Ogre::String name = XMLConverter::getAttrib(xmlNode, "name");
        Ogre::String id = XMLConverter::getAttrib(xmlNode, "id");
        Ogre::String meshFile = XMLConverter::getAttrib(xmlNode, "meshFile");
        bool castShadows = XMLConverter::getAttribBool(xmlNode, "castShadows", true);
        bool visible = XMLConverter::getAttribBool(xmlNode, "visible", true);
        Ogre::Real lodDistance = XMLConverter::getAttribReal(xmlNode, "lodDistance", 0.0f);

        bool isProceduralMesh = (meshFile == "Procedural.mesh");
        Ogre::String tempMeshFile = meshFile;

        if (Ogre::String::npos != tempMeshFile.find("Plane"))
        {
            tempMeshFile = "Missing.mesh";
        }

        // Attention: the two Ogre::Timer::getMilliseconds() calls that used to bracket this function
        // are gone. The resulting 'dt' was never logged or used, and on Windows Ogre's Timer may wrap
        // QueryPerformanceCounter in two SetThreadAffinityMask syscalls - so this was two syscalls and
        // a thread migration per item, for a value nobody read.

        Ogre::Item* item = nullptr;

        unsigned long missingGameObjectId = 0;
        if (true == justSetValues && false == this->missingGameObjectIds.empty())
        {
            rapidxml::xml_node<>* propertyElement = xmlNode->first_node("property");
            findGameObjectId(propertyElement, missingGameObjectId);
        }

        // Only load mesh if it's NOT procedural
        if (false == isProceduralMesh)
        {
            // Attention: 'parent' and 'tempMeshFile' are captured BY VALUE now. The former version
            // captured them by reference (&parent, &tempMeshFile) although both are plain locals of
            // this frame - a pointer captured by reference buys nothing and only widens the window in
            // which a command that outlives this frame reads dead memory. Only 'item' stays by
            // reference, because the command has to write the result back.
            GraphicsModule::RenderCommand renderCommand = [this, &item, justSetValues, xmlNode, parent, missingGameObjectId, meshFile, tempMeshFile, name, castShadows, visible]()
            {
                if (false == justSetValues || missingGameObjectId != 0)
                {
                    Ogre::MeshPtr v2Mesh = this->loadMeshV2Optimized(tempMeshFile, name, meshFile);

                    if (!v2Mesh)
                    {
                        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Failed to load mesh: " + tempMeshFile);
                        return;
                    }

                    Ogre::String path;
                    DeployResourceModule::getInstance()->tagResource(tempMeshFile, v2Mesh->getGroup(), path);

                    // Determine the static flag BEFORE the item exists, so that the item can be created
                    // directly in the correct memory manager. Attention: creating an item as
                    // SCENE_STATIC and then calling setStatic() migrates it between Ogre's object
                    // memory managers, which is far more expensive than getting it right immediately.
                    bool isStatic = false;
                    rapidxml::xml_node<>* userDataElement = xmlNode->first_node("userData");
                    if (nullptr != userDataElement)
                    {
                        rapidxml::xml_node<>* propertyElement = userDataElement->first_node("property");
                        if (nullptr != propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Static")
                        {
                            isStatic = XMLConverter::getAttribBool(propertyElement, "data", false);
                        }
                    }

                    item = this->sceneManager->createItem(v2Mesh, isStatic ? Ogre::SCENE_STATIC : Ogre::SCENE_DYNAMIC);
                    item->setName(name);
                    item->setCastShadows(castShadows);

                    if (nullptr != userDataElement)
                    {
                        parent->setStatic(isStatic);
                    }

                    parent->attachObject(item);
                    item->setVisible(visible);
                }

                // Set datablocks
                if (false == justSetValues || missingGameObjectId != 0)
                {
                    if (nullptr == item)
                    {
                        return;
                    }

                    rapidxml::xml_node<>* pElement = xmlNode->first_node("subitem");
                    size_t subItemIndexCount = 0;

                    while (nullptr != pElement)
                    {
                        Ogre::String materialFile = XMLConverter::getAttrib(pElement, "datablockName");
                        if (false == materialFile.empty())
                        {
                            // Attention: guard against a scene that lists more subitems than the mesh has.
                            // getSubItem() with an out of range index is undefined behaviour.
                            if (subItemIndexCount >= item->getNumSubItems())
                            {
                                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Warning: Scene lists more subitems than mesh '" + tempMeshFile + "' has, for game object: " + name);
                                break;
                            }

                            Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingleton().getHlmsManager();
                            Ogre::HlmsDatablock* block = hlmsManager->getDatablockNoDefault(materialFile);

                            if (nullptr != block)
                            {
                                // Attention: pass the resolved datablock pointer, not the name. The string
                                // overload makes Hlms look the datablock up a second time, and we already
                                // have it from getDatablockNoDefault above.
                                item->getSubItem(subItemIndexCount)->setDatablock(block);
                            }
                            else
                            {
                                break;
                            }

                            subItemIndexCount++;
                        }
                        pElement = pElement->next_sibling("subitem");
                    }
                }
            };

            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "DotSceneImportModule::processItem");
        }
        else
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DotSceneImport] Skipping procedural mesh for: " + name);
        }

        // Create GameObject
        rapidxml::xml_node<>* pElement = xmlNode->first_node("userData");
        if (nullptr != pElement)
        {
            GameObjectPtr gameObjectPtr = nullptr;
            if (false == justSetValues || missingGameObjectId != 0)
            {
                gameObjectPtr = GameObjectFactory::getInstance()->createOrSetGameObjectFromXML(pElement, this->sceneManager, parent, item, NOWA::ITEM, this->scenePath, this->forceCreation, this->bSceneParsed);
            }
            else
            {
                bool foundId = false;
                rapidxml::xml_node<>* propertyElement = pElement->first_node("property");

                if (nullptr != propertyElement)
                {
                    do
                    {
                        Ogre::String attrib = XMLConverter::getAttrib(propertyElement, "name");
                        if (nullptr != propertyElement && attrib == "Id")
                        {
                            unsigned long existingGameObjectId = XMLConverter::getAttribUnsignedLong(propertyElement, "data");
                            GameObjectPtr existingGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(existingGameObjectId);

                            if (nullptr == existingGameObjectPtr)
                            {
                                break;
                            }

                            gameObjectPtr = GameObjectFactory::getInstance()->createOrSetGameObjectFromXML(pElement, this->sceneManager, parent, item, NOWA::ITEM, this->scenePath, this->forceCreation, this->bSceneParsed, existingGameObjectPtr);
                            foundId = true;
                        }
                        else
                        {
                            propertyElement = propertyElement->next_sibling("property");
                        }
                    } while (nullptr != propertyElement && false == foundId);
                }
            }

            if (nullptr != gameObjectPtr)
            {
                gameObjectPtr->setOriginalMeshNameOnLoadFailure(meshFile);
                this->parsedGameObjectIds.emplace_back(gameObjectPtr->getId());

                if ("SunLight" == gameObjectPtr->getSceneNode()->getName())
                {
                    this->sunLight = NOWA::makeStrongPtr(gameObjectPtr->getComponent<LightDirectionalComponent>())->getOgreLight();
                }
            }
        }

        NOWA::GraphicsModule::getInstance()->renderLoadingFrameThrottled();
    }

    void DotSceneImportModule::processTerra(rapidxml::xml_node<>* xmlNode, Ogre::SceneNode* parent, bool justSetValues)
    {
        // Process attributes
        Ogre::String name = XMLConverter::getAttrib(xmlNode, "name");
        Ogre::String id = XMLConverter::getAttrib(xmlNode, "id");

        bool castShadows = false; // Shadows must not be casted for terra, else ugly crash shader cache is created
        bool visible = XMLConverter::getAttribBool(xmlNode, "visible", true);

        parent->setStatic(true);

        unsigned long missingGameObjectId = 0;
        if (true == justSetValues && false == this->missingGameObjectIds.empty())
        {
            rapidxml::xml_node<>* userDataElement = xmlNode->first_node("userData");
            if (userDataElement)
            {
                rapidxml::xml_node<>* propertyElement = userDataElement->first_node("property");
                this->findGameObjectId(propertyElement, missingGameObjectId);
            }
        }

        GameObjectPtr gameObjectPtr = nullptr;

        // Check if the entity element has user data, for game object creation
        rapidxml::xml_node<>* pElement = xmlNode->first_node("userData");

        // Maybe create, if its an already missing game object id
        if (false == justSetValues || missingGameObjectId != 0)
        {
            if (pElement)
            {
                gameObjectPtr = GameObjectFactory::getInstance()->createOrSetGameObjectFromXML(pElement, this->sceneManager, parent, nullptr, NOWA::TERRA, this->scenePath, this->forceCreation, false);

                if (nullptr != gameObjectPtr)
                {
                    this->parsedGameObjectIds.emplace_back(gameObjectPtr->getId());
                }
            }
        }
        else
        {
            bool foundId = false;
            if (pElement)
            {
                rapidxml::xml_node<>* propertyElement = pElement->first_node("property");
                if (nullptr != propertyElement)
                {
                    do
                    {
                        Ogre::String attrib = XMLConverter::getAttrib(propertyElement, "name");
                        if (propertyElement && attrib == "Id")
                        {
                            unsigned long existingGameObjectId = XMLConverter::getAttribUnsignedLong(propertyElement, "data");
                            GameObjectPtr existingGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(existingGameObjectId);

                            gameObjectPtr = GameObjectFactory::getInstance()->createOrSetGameObjectFromXML(pElement, this->sceneManager, parent, nullptr, NOWA::TERRA, this->scenePath, this->forceCreation, false, existingGameObjectPtr);

                            foundId = true;
                        }
                        else
                        {
                            propertyElement = propertyElement->next_sibling("property");
                        }
                    } while (nullptr != propertyElement && false == foundId);
                }
            }
        }

        NOWA::GraphicsModule::getInstance()->renderLoadingFrameThrottled();
    }

    void DotSceneImportModule::processOcean(rapidxml::xml_node<>* xmlNode, Ogre::SceneNode* parent, bool justSetValues)
    {
        // Process attributes
        Ogre::String name = XMLConverter::getAttrib(xmlNode, "name");
        Ogre::String id = XMLConverter::getAttrib(xmlNode, "id");

        bool castShadows = false; // Shadows must not be casted for ocean, else ugly crash shader cache is created
        bool visible = XMLConverter::getAttribBool(xmlNode, "visible", true);

        parent->setStatic(false);

        unsigned long missingGameObjectId = 0;
        if (true == justSetValues && false == this->missingGameObjectIds.empty())
        {
            rapidxml::xml_node<>* userDataElement = xmlNode->first_node("userData");
            if (userDataElement)
            {
                rapidxml::xml_node<>* propertyElement = userDataElement->first_node("property");
                this->findGameObjectId(propertyElement, missingGameObjectId);
            }
        }

        GameObjectPtr gameObjectPtr = nullptr;

        // Check if the entity element has user data, for game object creation
        rapidxml::xml_node<>* pElement = xmlNode->first_node("userData");

        // Maybe create, if its an already missing game object id
        if (false == justSetValues || missingGameObjectId != 0)
        {
            if (pElement)
            {
                gameObjectPtr = GameObjectFactory::getInstance()->createOrSetGameObjectFromXML(pElement, this->sceneManager, parent, nullptr, NOWA::OCEAN, this->scenePath, this->forceCreation, false);

                if (nullptr != gameObjectPtr)
                {
                    this->parsedGameObjectIds.emplace_back(gameObjectPtr->getId());
                }
            }
        }
        else
        {
            bool foundId = false;
            if (pElement)
            {
                rapidxml::xml_node<>* propertyElement = pElement->first_node("property");
                if (nullptr != propertyElement)
                {
                    do
                    {
                        Ogre::String attrib = XMLConverter::getAttrib(propertyElement, "name");
                        if (propertyElement && attrib == "Id")
                        {
                            unsigned long existingGameObjectId = XMLConverter::getAttribUnsignedLong(propertyElement, "data");
                            GameObjectPtr existingGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(existingGameObjectId);

                            gameObjectPtr = GameObjectFactory::getInstance()->createOrSetGameObjectFromXML(pElement, this->sceneManager, parent, nullptr, NOWA::OCEAN, this->scenePath, this->forceCreation, false, existingGameObjectPtr);

                            foundId = true;
                        }
                        else
                        {
                            propertyElement = propertyElement->next_sibling("property");
                        }
                    } while (nullptr != propertyElement && false == foundId);
                }
            }
        }

        NOWA::GraphicsModule::getInstance()->renderLoadingFrameThrottled();
    }

    void DotSceneImportModule::processPlane(rapidxml::xml_node<>* xmlNode, Ogre::SceneNode* parent, bool justSetValues)
    {
        Ogre::String name = XMLConverter::getAttrib(xmlNode, "name");
        Ogre::Real distance = XMLConverter::getAttribReal(xmlNode, "distance");
        Ogre::Real width = XMLConverter::getAttribReal(xmlNode, "width");
        Ogre::Real height = XMLConverter::getAttribReal(xmlNode, "height");
        int xSegments = Ogre::StringConverter::parseInt(XMLConverter::getAttrib(xmlNode, "xSegments"));
        int ySegments = Ogre::StringConverter::parseInt(XMLConverter::getAttrib(xmlNode, "ySegments"));
        int numTexCoordSets = Ogre::StringConverter::parseInt(XMLConverter::getAttrib(xmlNode, "numTexCoordSets"));
        Ogre::Real uTile = XMLConverter::getAttribReal(xmlNode, "uTile");
        Ogre::Real vTile = XMLConverter::getAttribReal(xmlNode, "vTile");
        Ogre::String materialFile = XMLConverter::getAttrib(xmlNode, "material");
        bool hasNormals = XMLConverter::getAttribBool(xmlNode, "hasNormals");

        Ogre::Vector3 normal = XMLConverter::parseVector3(xmlNode->first_node("normal"));
        Ogre::Vector3 up = XMLConverter::parseVector3(xmlNode->first_node("upVector"));

        Ogre::Item* item = nullptr;

        unsigned long missingGameObjectId = 0;
        if (true == justSetValues && false == this->missingGameObjectIds.empty())
        {
            rapidxml::xml_node<>* userDataElement = xmlNode->first_node("userData");
            if (userDataElement)
            {
                rapidxml::xml_node<>* propertyElement = userDataElement->first_node("property");
                findGameObjectId(propertyElement, missingGameObjectId);
            }
        }

        GraphicsModule::RenderCommand renderCommand = [this, &item, justSetValues, xmlNode, &parent, missingGameObjectId, normal, distance, numTexCoordSets, uTile, vTile, up, width, height, xSegments, ySegments, hasNormals, name, materialFile]()
        {
            // Maybe create, if its an already missing game object id
            if (false == justSetValues || missingGameObjectId != 0)
            {
                Ogre::Plane plane(normal, distance);
                Ogre::v1::MeshPtr planeMeshV1;

                planeMeshV1 = Ogre::v1::MeshManager::getSingletonPtr()->createPlane(name + "mesh", "General", plane, width, height, xSegments, ySegments, hasNormals, numTexCoordSets, uTile, vTile, up, Ogre::v1::HardwareBuffer::HBU_STATIC,
                    Ogre::v1::HardwareBuffer::HBU_STATIC);

                Ogre::String path;
                DeployResourceModule::getInstance()->tagResource(name + "mesh", planeMeshV1->getGroup(), path);

                Ogre::MeshPtr v2Mesh = Ogre::MeshManager::getSingletonPtr()->createByImportingV1(name, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, planeMeshV1.get(), true, true, true);
                planeMeshV1->unload();
                planeMeshV1.setNull();

                item = this->sceneManager->createItem(v2Mesh, Ogre::SCENE_STATIC);
                item->setName(name + "mesh");

                Ogre::MaterialPtr objectMaterial = Ogre::MaterialManager::getSingleton().getByName(materialFile);
                if (false == materialFile.empty())
                {
                    item->setDatablock(materialFile);
                }

                // Change the addressing mode of the roughness map to wrap via code.
                Ogre::HlmsPbsDatablock* datablock = static_cast<Ogre::HlmsPbsDatablock*>(item->getSubItem(0)->getDatablock());
                Ogre::HlmsSamplerblock samplerblock(*datablock->getSamplerblock(Ogre::PBSM_ROUGHNESS));
                samplerblock.mU = Ogre::TAM_WRAP;
                samplerblock.mV = Ogre::TAM_WRAP;
                samplerblock.mW = Ogre::TAM_WRAP;
                datablock->setSamplerblock(Ogre::PBSM_ROUGHNESS, samplerblock);

                for (size_t i = 0; i < item->getNumSubItems(); i++)
                {
                    auto sourceDataBlock = dynamic_cast<Ogre::HlmsPbsDatablock*>(item->getSubItem(i)->getDatablock());
                    if (nullptr != sourceDataBlock)
                    {
                        // Deactivate fresnel by default, because it looks ugly
                        if (sourceDataBlock->getWorkflow() != Ogre::HlmsPbsDatablock::SpecularAsFresnelWorkflow && sourceDataBlock->getWorkflow() != Ogre::HlmsPbsDatablock::MetallicWorkflow)
                        {
                            sourceDataBlock->setFresnel(Ogre::Vector3(0.01f, 0.01f, 0.01f), false);
                        }
                    }
                }
            }
        };

        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "DotSceneImportModule::processPlane");

        rapidxml::xml_node<>* element = xmlNode->next_sibling("userData");

        GameObjectPtr gameObjectPtr = nullptr;

        if (false == justSetValues || missingGameObjectId != 0)
        {
            gameObjectPtr = GameObjectFactory::getInstance()->createOrSetGameObjectFromXML(element, this->sceneManager, parent, item, NOWA::PLANE, this->scenePath, this->forceCreation, false);

            if (nullptr != gameObjectPtr)
            {
                this->parsedGameObjectIds.emplace_back(gameObjectPtr->getId());

                bool dynamic = true;
                rapidxml::xml_node<>* propertyElement = element->first_node("property");

                if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Static")
                {
                    dynamic = !XMLConverter::getAttribBool(propertyElement, "data", false);
                    propertyElement = propertyElement->next_sibling("property");
                }

                parent->setStatic(!dynamic);
                item->setStatic(!dynamic);
                parent->attachObject(item);
            }
        }
        else
        {
            bool foundId = false;
            rapidxml::xml_node<>* propertyElement = element->first_node("property");
            if (nullptr != propertyElement)
            {
                do
                {
                    Ogre::String attrib = XMLConverter::getAttrib(propertyElement, "name");
                    if (propertyElement && attrib == "Id")
                    {
                        unsigned long existingGameObjectId = XMLConverter::getAttribUnsignedLong(propertyElement, "data");
                        gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(existingGameObjectId);

                        gameObjectPtr = GameObjectFactory::getInstance()->createOrSetGameObjectFromXML(element, this->sceneManager, parent, item, NOWA::PLANE, this->scenePath, this->forceCreation, this->bSceneParsed, gameObjectPtr);
                        foundId = true;
                    }
                    else
                    {
                        propertyElement = propertyElement->next_sibling("property");
                    }
                } while (false == foundId && nullptr != propertyElement);
            }
        }

        NOWA::GraphicsModule::getInstance()->renderLoadingFrameThrottled();
    }

    Ogre::MeshPtr DotSceneImportModule::loadMeshV2Optimized(const Ogre::String& meshName, const Ogre::String& itemName, const Ogre::String& originalMeshFile)
    {
        Ogre::MeshPtr v2Mesh;

        // ============================================================================
        // FAST PATH: mesh already known to the V2 MeshManager
        // ============================================================================
        v2Mesh = Ogre::MeshManager::getSingletonPtr()->getByName(meshName, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

        if (v2Mesh)
        {
            // Attention: getByName() returns a handle for a resource that has been CREATED,
            // which is not the same as LOADED. Returning an unloaded mesh here pushes the
            // (synchronous, expensive) load into whoever creates the Item from it, at a point
            // where it is much harder to see in a profile.
            if (false == v2Mesh->isLoaded())
            {
                v2Mesh->load();
            }
            return v2Mesh;
        }

        // ============================================================================
        // TRY V2 LOAD (for pre-converted meshes)
        // ============================================================================
        try
        {
            v2Mesh = Ogre::MeshManager::getSingletonPtr()->load(meshName, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DotSceneImport] Loaded V2 mesh (fast path): " + meshName);

            return v2Mesh;
        }
        catch (Ogre::Exception&)
        {
            // Attention: this catch is CONTROL FLOW, and it is not free. Ogre first scans the
            // resource groups, then builds and throws an exception with a full description
            // string. If a scene has many legacy meshes this shows up in a profile. Converting
            // the meshes offline with OgreMeshTool removes the whole path.
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DotSceneImport] V2 load failed for '" + meshName + "', trying V1 import...");
        }

        // ============================================================================
        // SLOW PATH: V1 -> V2 CONVERSION (only for legacy meshes)
        // ============================================================================
        try
        {
            Ogre::v1::MeshPtr v1Mesh = Ogre::v1::MeshManager::getSingletonPtr()->getByName(meshName, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);

            if (!v1Mesh)
            {
                v1Mesh =
                    Ogre::v1::MeshManager::getSingletonPtr()->load(meshName, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, Ogre::v1::HardwareBuffer::HBU_STATIC_WRITE_ONLY, Ogre::v1::HardwareBuffer::HBU_STATIC_WRITE_ONLY, true, true);
            }

            if (!v1Mesh)
            {
                throw Ogre::Exception(0, "V1 mesh load failed", "loadMeshV2Optimized");
            }

            // Attention: the V2 mesh MUST be named after the mesh, never after the item.
            // Naming it per item creates one full V2 mesh (and one skeleton reference) per
            // spawned object instead of sharing a single one across all 100 monsters.
            v2Mesh = Ogre::MeshManager::getSingletonPtr()->createByImportingV1(meshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, v1Mesh.get(), true, true, true);

            v1Mesh->unload();

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DotSceneImport] WARNING: Converted V1->V2 at runtime (SLOW): " + meshName + " - Use OgreMeshTool to convert offline!");

            return v2Mesh;
        }
        catch (Ogre::Exception&)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DotSceneImport] Failed to load: " + meshName + ", loading Missing.mesh");
        }

        // ============================================================================
        // FALLBACK: Missing.mesh, shared by ALL failed items
        // ============================================================================
        try
        {
            // Attention: the former code built the fallback under 'itemName', i.e. a unique
            // name per object. Every broken item therefore got its own V2 copy of Missing.mesh
            // AND ran the destroyResourcePool/remove dance each time. One shared name is enough.
            const Ogre::String fallbackName = "NOWA_MissingMesh_V2";

            v2Mesh = Ogre::MeshManager::getSingletonPtr()->getByName(fallbackName, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
            if (v2Mesh)
            {
                if (false == v2Mesh->isLoaded())
                {
                    v2Mesh->load();
                }
                return v2Mesh;
            }

            Ogre::v1::MeshPtr missingV1 = Ogre::v1::MeshManager::getSingleton().load("Missing.mesh", Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, Ogre::v1::HardwareBuffer::HBU_STATIC, Ogre::v1::HardwareBuffer::HBU_STATIC);

            v2Mesh = Ogre::MeshManager::getSingletonPtr()->createByImportingV1(fallbackName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, missingV1.get(), true, true, true);

            missingV1->unload();

            return v2Mesh;
        }
        catch (Ogre::Exception& e2)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DotSceneImport] Critical error loading mesh: " + e2.getDescription());
            return nullptr;
        }
    }

    // ── Accessors ─────────────────────────────────────────────────────────────

    void DotSceneImportModule::setShowLoadingIndicator(bool show)
    {
        this->showLoadingIndicator = show;
    }

    void DotSceneImportModule::setLoadingIndicator(ILoadingIndicator* indicator)
    {
        this->loadingIndicator = indicator;
    }

    void DotSceneImportModule::beginLoadingIndicator(void)
    {
        if (false == this->showLoadingIndicator)
        {
            return;
        }

        NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
        {
            // Attention: A workspace is mandatory. In the editor ProjectManager::loadProject
            // destroys the scene BEFORE parsing, and in the game GameProgressModule tears the old
            // scene down the same way - so in the normal case there is nothing to render into and
            // renderOneFrame() would draw nothing at all. That is why every earlier attempt at a
            // loading screen failed.
            //
            // A camera is created only to give the dummy workspace something to render from. It is
            // deliberately NOT registered with the CameraManager: that would also pull in a camera
            // behavior, and this camera exists for a few hundred milliseconds.
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[LOADING-BEGIN] command entered, hasAnyWorkspace=" + Ogre::StringConverter::toString(WorkspaceModule::getInstance()->hasAnyWorkspace()));

            if (false == WorkspaceModule::getInstance()->hasAnyWorkspace())
            {
                if (nullptr == this->sceneManager)
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Cannot show the loading indicator, there is no scene manager.");
                    return;
                }

                this->temporaryLoadingCamera = this->sceneManager->createCamera("NOWA_LoadingIndicatorCamera");
                this->temporaryLoadingCamera->setFOVy(Ogre::Degree(90.0f));
                this->temporaryLoadingCamera->setNearClipDistance(0.1f);
                this->temporaryLoadingCamera->setFarClipDistance(100.0f);
                this->temporaryLoadingCamera->setQueryFlags(0 << 0);

                // nullptr as the component means: dummy workspace.
                WorkspaceModule::getInstance()->setPrimaryWorkspace(this->sceneManager, this->temporaryLoadingCamera, nullptr);

                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[LOADING-BEGIN] dummy workspace created, hasAnyWorkspace=" + Ogre::StringConverter::toString(WorkspaceModule::getInstance()->hasAnyWorkspace()));
            }

            // Fall back to the cheapest indicator if the caller did not provide one.
            if (nullptr == this->loadingIndicator)
            {
                this->ownedLoadingIndicator = new DotsTextLoadingIndicator();
                // this->ownedLoadingIndicator = new RotatingImageLoadingIndicator();
                this->loadingIndicator = this->ownedLoadingIndicator;
            }

            this->loadingIndicator->onShow();

            NOWA::GraphicsModule::getInstance()->setLoadingIndicator(this->loadingIndicator);

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[LOADING-BEGIN] indicator active");
        };

        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "DotSceneImportModule::beginLoadingIndicator");
    }

    void DotSceneImportModule::endLoadingIndicator(void)
    {
        if (false == this->showLoadingIndicator)
        {
            return;
        }

        NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[LOADING-END] command entered");

            // Stop driving it BEFORE the widgets are destroyed, otherwise the next loading frame
            // could still call onUpdate() on a dead widget.
            NOWA::GraphicsModule::getInstance()->setLoadingIndicator(nullptr);

            if (nullptr != this->loadingIndicator)
            {
                this->loadingIndicator->onHide();
            }

            if (nullptr != this->ownedLoadingIndicator)
            {
                delete this->ownedLoadingIndicator;
                this->ownedLoadingIndicator = nullptr;
                this->loadingIndicator = nullptr;
            }

            if (nullptr != this->temporaryLoadingCamera)
            {
                // Attention: removeCamera() also removes the dummy workspace and erases the map
                // entry. By now the parsed WorkspaceBaseComponent has usually replaced the dummy
                // already, in which case there is nothing left to find - that is fine, but the
                // camera itself still has to go.
                WorkspaceModule::getInstance()->removeCamera(this->temporaryLoadingCamera);

                if (nullptr != this->sceneManager)
                {
                    this->sceneManager->destroyCamera(this->temporaryLoadingCamera);
                }

                this->temporaryLoadingCamera = nullptr;
            }
        };

        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "DotSceneImportModule::endLoadingIndicator");
    }

    Ogre::SceneManager* DotSceneImportModule::getSceneManager(void) const
    {
        return this->sceneManager;
    }

    Ogre::Camera* DotSceneImportModule::getMainCamera(void) const
    {
        return this->mainCamera;
    }

    Ogre::Light* DotSceneImportModule::getSunLight(void) const
    {
        return this->sunLight;
    }

    const ProjectParameter& DotSceneImportModule::getProjectParameter(void) const
    {
        return this->projectParameter;
    }

    void DotSceneImportModule::setIsSnapshot(bool bIsSnapshot)
    {
        this->bIsSnapshot = bIsSnapshot;
    }

    std::pair<Ogre::String, Ogre::String> DotSceneImportModule::getProjectAndSceneName(const Ogre::String& filePathName, bool decrypt)
    {
        Ogre::String sceneName;
        Ogre::String projectName;
        std::ifstream ifs(filePathName);
        if (false == ifs.good())
        {
            return std::make_pair(projectName, sceneName);
        }

        std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
        DWORD dwFileAttributes = GetFileAttributes(filePathName.c_str());
        if (dwFileAttributes & FileFlag && true == decrypt)
        {
            content = Core::getSingletonPtr()->decode64(content, true);
        }
        content += '\0';

        {
            Ogre::String toFind = "projectName=\"";
            size_t projectNameTagPos = content.find(toFind);
            if (Ogre::String::npos != projectNameTagPos)
            {
                size_t projectNameTagEndPos = content.find("\"", projectNameTagPos + toFind.length());
                if (Ogre::String::npos != projectNameTagEndPos)
                {
                    projectName = content.substr(projectNameTagPos + toFind.length(), projectNameTagEndPos - (projectNameTagPos + toFind.length()));
                }
            }
        }

        {
            Ogre::String toFind = "sceneName=\"";
            size_t sceneNameTagPos = content.find(toFind);
            if (Ogre::String::npos != sceneNameTagPos)
            {
                size_t sceneNameTagEndPos = content.find("\"", sceneNameTagPos + toFind.length());
                if (Ogre::String::npos != sceneNameTagEndPos)
                {
                    sceneName = content.substr(sceneNameTagPos + toFind.length(), sceneNameTagEndPos - (sceneNameTagPos + toFind.length()));
                }
            }
        }
        ifs.close();
        return std::make_pair(projectName, sceneName);
    }

}; // Namespace end
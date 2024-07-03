#include "NOWAPrecompiled.h"
#include "DotSceneImportModule.h"
#include "main/Core.h" // required for the progressbar when loading the scene
#include "main/AppStateManager.h"
#include "gameobject/GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "gameobject/PhysicsComponent.h"
#include "gameobject/PhysicsActiveComponent.h"
#include "gameobject/CameraComponent.h"
#include "gameobject/LightDirectionalComponent.h"
#include "gameobject/PlanarReflectionComponent.h"
#include "gameobject/TerraComponent.h"
#include "gameObject/ExitComponent.h"
#include "gameObject/HdrEffectComponent.h"
#include "gameObject/LuaScriptComponent.h"
#include "modules/WorkspaceModule.h"
#include "OgreMeshManager2.h"
#include "OgreConfigFile.h"
#include "OgreLodConfig.h"
#include "OgreLodStrategyManager.h"
#include "OgreMeshLodGenerator.h"
#include "OgreMesh2Serializer.h"
#include "OgrePixelCountLodStrategy.h"

#include "res/resource.h"

#include "GameProgressModule.h"
#include "OgreNewtModule.h"
#include "DeployResourceModule.h"

namespace NOWA
{
	DotSceneImportModule::DotSceneImportModule(Ogre::SceneManager* sceneManager)
		: sceneManager(sceneManager),
		ogreNewt(nullptr),
		mainCamera(nullptr),
		pagesCount(0),
		needCollisionRebuild(false),
		worldLoaderCallback(nullptr),
		showProgress(false),
		forceCreation(false),
		bSceneParsed(false),
		bIsSnapshot(false),
		bNewScene(false),
		mostLeftNearPosition(Ogre::Vector3(Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY)),
		mostRightFarPosition(Ogre::Vector3(Ogre::Math::NEG_INFINITY, Ogre::Math::NEG_INFINITY, Ogre::Math::NEG_INFINITY)),
		sunLight(nullptr)
	{

	}

	DotSceneImportModule::DotSceneImportModule(Ogre::SceneManager* sceneManager, Ogre::Camera* mainCamera, OgreNewt::World* ogreNewt)
		: sceneManager(sceneManager),
		mainCamera(mainCamera),
		ogreNewt(ogreNewt),
		sunLight(nullptr),
		pagesCount(0),
		needCollisionRebuild(false),
		worldLoaderCallback(nullptr),
		showProgress(false),
		forceCreation(false),
		bSceneParsed(false),
		bIsSnapshot(false),
		bNewScene(false),
		mostLeftNearPosition(Ogre::Vector3(Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY)),
		mostRightFarPosition(Ogre::Vector3(Ogre::Math::NEG_INFINITY, Ogre::Math::NEG_INFINITY, Ogre::Math::NEG_INFINITY))
	{
		// Add delegates to be called when the corresponding event had fired
		AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &DotSceneImportModule::parseGameObjectDelegate), EventDataParseGameObject::getStaticEventType());
	}

	DotSceneImportModule::~DotSceneImportModule()
	{
		// Remove delegates
		AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &DotSceneImportModule::parseGameObjectDelegate), EventDataParseGameObject::getStaticEventType());
		// Do not destroy here, because world loader is also used in undo command!!
		// AppStateManager::getSingletonPtr()->getGameObjectController()->destroyContent();
	}

	DotSceneImportModule::DotSceneImportModule(Ogre::SceneManager* sceneManager, const Ogre::String& projectName, const Ogre::String& sceneName, const Ogre::String& resourceGroupName)
		: sceneManager(sceneManager),
		mainCamera(nullptr),
		ogreNewt(nullptr),
		sunLight(nullptr),
		pagesCount(0),
		needCollisionRebuild(false),
		worldLoaderCallback(nullptr),
		showProgress(false),
		forceCreation(false),
		bSceneParsed(false),
		bIsSnapshot(false),
		bNewScene(false),
		mostLeftNearPosition(Ogre::Vector3(Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY)),
		mostRightFarPosition(Ogre::Vector3(Ogre::Math::NEG_INFINITY, Ogre::Math::NEG_INFINITY, Ogre::Math::NEG_INFINITY))
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
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Error: Can not load world, because the groupname is empty.");
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[DotSceneImportModule] Can not load world, because the groupname is empty.\n", "NOWA");
		}

		Ogre::String projectPath = Core::getSingletonPtr()->getSectionPath(this->resourceGroupName)[0];

		// Project is always: "Projects/ProjectName/SceneName.scene"
		// E.g.: "Projects/Plattformer/Level1.scene", "Projects/Plattformer/Level2.scene", "Projects/Plattformer/Level3.scene"
		this->worldPath = projectPath + "/" + this->projectParameter.projectName + "/" + this->projectParameter.sceneName + ".scene";
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
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DotSceneImportModule]: Parse game objects from virtual environment for controlled client id: "
				+ Ogre::StringConverter::toString(controlledByClientID));
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

		Ogre::String globalSceneFilePathName = projectPath + "/" + this->projectParameter.projectName + "/global.scene";
		
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
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[DotSceneImportModule] Could not parse global scene. Error: " + Ogre::String(error.what())
				+ " at: " + Ogre::String(error.where<char>()) + "\n", "NOWA");
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

	bool DotSceneImportModule::parseScene(const Ogre::String& projectName, const Ogre::String& sceneName, const Ogre::String& resourceGroupName, Ogre::Light* sunLight,
		DotSceneImportModule::IWorldLoaderCallback* worldLoaderCallback, bool showProgress)
	{
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
		this->worldLoaderCallback = worldLoaderCallback;
		this->showProgress = showProgress;
		this->parsedGameObjectIds.clear();

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DotSceneImportModule]: Begin Parsing scene: '" 
			+ this->projectParameter.projectName + "/" + this->projectParameter.sceneName + ".scene' for resource group: '" + resourceGroupName + "'");

		if (true == this->resourceGroupName.empty())
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Error: Can not load world, because the groupname is empty.");
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[DotSceneImportModule] Can not load world, because the groupname is empty.\n", "NOWA");
		}
		
		Ogre::String projectPath = Core::getSingletonPtr()->getSectionPath(this->resourceGroupName)[0];
		
		// Project is always: "Projects/ProjectName/SceneName.scene"
		// E.g.: "Projects/Plattformer/Level1.scene", "Projects/Plattformer/Level2.scene", "Projects/Plattformer/Level3.scene"
		this->worldPath = projectPath + "/" + this->projectParameter.projectName + "/" + this->projectParameter.sceneName + ".scene";

		return this->internalParseScene(this->worldPath);
	}

	bool DotSceneImportModule::internalParseScene(const Ogre::String& filePathName, bool crypted)
	{
		// If a saved game shall be parsed, the user can say, whether everything is crypted and needs to be decoded.
		float currentTime = static_cast<Ogre::Real>(Core::getSingletonPtr()->getOgreTimer()->getMilliseconds()) * 0.001f;

		// Announce the current world path to core. Do not set file path name, as it could be from a saved game, which points to a whole different location, in which there are no lua scripts etc.
		Core::getSingletonPtr()->setCurrentWorldPath(this->worldPath);

		this->bSceneParsed = true;

		std::ifstream ifs(filePathName);
		if (false == ifs.good())
		{
			this->bNewScene = true;
			// If scene does not exist yet, maybe there is a global scene. Parse these one.
			bool success = this->parseGlobalScene(crypted);
			// If there is no scene, but just an existing global scene, post init must be done, because maybe main camera etc. are just in the global scene!
			if (true == success)
			{
				this->postInitData();
			}
			this->bNewScene = false;
			return success;
		}

		// First process the usual scene with all the other stuff
		rapidxml::xml_document<> XMLDoc;
		rapidxml::xml_node<>* xmlRoot;

		std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
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

		try
		{
			XMLDoc.parse<0>(&content[0]);
		}
		catch (rapidxml::parse_error& error)
		{
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[DotSceneImportModule] Could not parse scene: " + this->projectParameter.sceneName + " error: " + Ogre::String(error.what())
								  + " at: " + Ogre::String(error.where<char>()) + "\n", "NOWA");
		}
		/*catch (Ogre::FileNotFoundException& exception)
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Could not parse scene: " + sceneName
				+ " because it does not exist. Error: " + exception.what());
			return false;
		}*/

		xmlRoot = XMLDoc.first_node("scene");
		if (nullptr == xmlRoot)
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Error: Invalid global.scene File. Missing <scene>");
			return false;
		}
		if (XMLConverter::getAttrib(xmlRoot, "formatVersion", "") == "")
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Error: Invalid .scene File. Missing <scene>");
			return false;
		}

		// Process the scene
		this->processScene(xmlRoot);

		// Second process a possible global.scene with all global game objects, that are used for all scenes in the project
		if (false == this->projectParameter.ignoreGlobalScene)
		{
			this->parseGlobalScene(crypted);
		}

		// This game objects must be initialized before all other game objects are initialized, because they may need data from this game objects, like terra needs a camera
		this->postInitData();

		float dt = (static_cast<Ogre::Real>(Core::getSingletonPtr()->getOgreTimer()->getMilliseconds()) * 0.001f) - currentTime;
		Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[DotSceneImportModule] Parse end scene: " + this->projectParameter.sceneName + " duration: " + Ogre::StringConverter::toString(dt) + " seconds");

		// Ogre::Root::getSingletonPtr()->renderOneFrame();

		this->bSceneParsed = false;

		boost::shared_ptr<EventDataSceneParsed> eventDataSceneParsed(new EventDataSceneParsed());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataSceneParsed);

		return true;
	}

	bool NOWA::DotSceneImportModule::parseSceneSnapshot(const Ogre::String& projectName, const Ogre::String& sceneName, const Ogre::String& filePathName, bool crypted, bool showProgress)
	{
		// If a saved game shall be parsed, the user can say, whether everything is crypted and needs to be decoded.
		bool success = true;
		this->bIsSnapshot = true;
		this->bSceneParsed = true;
		this->showProgress = showProgress;

		float currentTime = static_cast<Ogre::Real>(Core::getSingletonPtr()->getOgreTimer()->getMilliseconds()) * 0.001f;

		rapidxml::xml_document<> XMLDoc;

		std::ifstream ifs(filePathName);
		if (false == ifs.good())
		{
			success = false;
			return success;
		}

		this->projectParameter.projectName = projectName;
		this->projectParameter.sceneName = sceneName;

		this->resourceGroupName = resourceGroupName;
		this->sunLight = sunLight;
		this->worldLoaderCallback = worldLoaderCallback;
		this->showProgress = showProgress;
		this->parsedGameObjectIds.clear();

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DotSceneImportModule]: Begin Parsing scene: '"
														+ this->projectParameter.projectName + "/" + this->projectParameter.sceneName + ".scene' for resource group: '" + resourceGroupName + "'");

		return this->internalParseScene(filePathName, crypted);
	}

	void DotSceneImportModule::postInitData()
	{
		auto& mainCameraGameObject = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(NOWA::GameObjectController::MAIN_CAMERA_ID);
		if (nullptr == mainCameraGameObject && false == this->bNewScene)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Error: Can not load world, because the MainCamera could not be created! See log for further information.");
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[DotSceneImportModule] Error: Can not load world, because the MainCamera could not be created! See log for further information.\n", "NOWA");
		}
		else if (nullptr != mainCameraGameObject)
		{
			mainCameraGameObject->postInit();
			if (nullptr == this->mainCamera && false == this->bNewScene)
			{
				this->mainCamera = NOWA::makeStrongPtr(mainCameraGameObject->getComponent<CameraComponent>())->getCamera();
			}
		}

		auto& mainLightGameObject = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(NOWA::GameObjectController::MAIN_LIGHT_ID);
		if (nullptr == mainLightGameObject && false == this->bNewScene)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Error: Can not load world, because the MainLight could not be created! See log for further information.");
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[DotSceneImportModule] Error: Can not load world, because the MainLight could not be created! See log for further information.\n", "NOWA");
		}
		else if (nullptr != mainLightGameObject)
		{
			mainLightGameObject->postInit();
			if (nullptr == this->sunLight)
			{
				this->sunLight = NOWA::makeStrongPtr(mainLightGameObject->getComponent<LightDirectionalComponent>())->getOgreLight();
			}
		}

		auto& mainGameObject = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(NOWA::GameObjectController::MAIN_GAMEOBJECT_ID);
		if (nullptr == mainGameObject && false == this->bNewScene)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Error: Can not load world, because the MainGameObject could not be created. Maybe this is an old scene, which does not have a MainGameObject! See log for further information.");
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[DotSceneImportModule] Error: Can not load world, because the MainGameObject could not be created! See log for further information.\n", "NOWA");
		}
		else if (nullptr != mainGameObject)
		{
			mainGameObject->postInit();
		}

		std::vector<GameObject*> clampGameObjects;

		// Now that all gameobject's have been fully created, run the post init phase (now all other components are also available for each game object)
		for (auto& it = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects()->cbegin();
				it != AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects()->cend(); ++it)
		{
			const auto& gameObjectPtr = it->second;
			if (gameObjectPtr->getId() != NOWA::GameObjectController::MAIN_CAMERA_ID
				&& gameObjectPtr->getId() != NOWA::GameObjectController::MAIN_LIGHT_ID
				&& gameObjectPtr->getId() != NOWA::GameObjectController::MAIN_GAMEOBJECT_ID)
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
			if (clampGameObject->getId() != NOWA::GameObjectController::MAIN_CAMERA_ID
				&& clampGameObject->getId() != NOWA::GameObjectController::MAIN_LIGHT_ID
				&& clampGameObject->getId() != NOWA::GameObjectController::MAIN_GAMEOBJECT_ID)
			{
				// If everything is loaded, perform raycast for y clamping for each game object, which has the corresponding attribute activated
				clampGameObject->performRaycastForYClamping();
			}
		}

		if (AppStateManager::getSingletonPtr()->getOgreRecastModule()->hasNavigationMeshElements())
		{
			bool skip = false;
			if (true == this->showProgress)
			{
				Core::getSingletonPtr()->getEngineResourceListener()->scriptParseStarted("Navigation Mesh", skip);
			}
			AppStateManager::getSingletonPtr()->getOgreRecastModule()->buildNavigationMesh();
			if (true == this->showProgress)
			{
				Core::getSingletonPtr()->getEngineResourceListener()->scriptParseEnded("Navigation Mesh finished", skip);
			}
		}

		if (nullptr != this->worldLoaderCallback)
		{
			delete this->worldLoaderCallback;
			this->worldLoaderCallback = nullptr;
		}
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
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[DotSceneImportModule] Could not parse group: " + groupFileName + " error: " + Ogre::String(error.what())
				+ " at: " + Ogre::String(error.where<char>()) + "\n", "NOWA");
		}

		rapidxml::xml_node<> *pElement;

		// Process resource locations (?)
		pElement = XMLDoc.first_node("resourceLocations");
		if (pElement)
		 	this->processResourceLocations(pElement);
		
		pElement = XMLDoc.first_node("nodes");
		if (pElement)
			this->processNodes(pElement);

		// Now that all gameobject's have been fully created, run the post init phase (now all other components are also available for each game object)
		for (size_t i = 0; i < this->parsedGameObjectIds.size(); i++)
		{
			const auto& gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->parsedGameObjectIds[i]);
			if (nullptr != gameObjectPtr)
			{
				// Deactivated, because its copied on another place^^
#if 0
				GameObjectComponents* components = gameObjectPtr->getComponents();
				for (auto& it = components->begin(); it != components->end(); ++it)
				{
					auto luaScriptCompPtr = boost::dynamic_pointer_cast<LuaScriptComponent>(std::get<COMPONENT>(*it));
					if (nullptr != luaScriptCompPtr)
					{
						Ogre::String projectPath = Core::getSingletonPtr()->getSectionPath(resourceGroupName)[0];
						Ogre::String scriptSourceFilePathName = projectPath + "/Groups/" + luaScriptCompPtr->getScriptFile();
						Ogre::String scriptDestFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + luaScriptCompPtr->getScriptFile();
						AppStateManager::getSingletonPtr()->getLuaScriptModule()->copyScriptAbsolutePath(scriptSourceFilePathName, scriptDestFilePathName, false);
					}
				}
#endif
				// Tells lua script component, that its being cloned (prevents multiple script creations)
				GameObjectComponents* components = gameObjectPtr->getComponents();
				for (auto& it = components->begin(); it != components->end(); ++it)
				{
					auto luaScriptCompPtr = boost::dynamic_pointer_cast<LuaScriptComponent>(std::get<COMPONENT>(*it));
					if (nullptr != luaScriptCompPtr)
					{
						luaScriptCompPtr->setComponentCloned(true);
					}
				}

				gameObjectPtr->postInit();
			}
		}

		// Copy the parsed game object ids and clear for next round
		std::vector<unsigned long> tempParsedGameObjectIds = this->parsedGameObjectIds;
		this->parsedGameObjectIds.clear();

		return std::move(tempParsedGameObjectIds);
	}

	bool DotSceneImportModule::parseGameObject(const Ogre::String& name)
	{
		// do not show progress in gui because it will crash
		this->showProgress = false;
		bool success = false;
		// get the xml file from the resourcegroup
		Ogre::DataStreamPtr stream = Ogre::ResourceGroupManager::getSingleton().openResource(this->projectParameter.projectName + "/" + this->projectParameter.sceneName + ".scene", this->resourceGroupName);

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

		auto& gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromName(name);
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
		this->showProgress = false;
		bool success = false;
		this->parsedGameObjectIds.clear();
		// Get the xml file from the resourcegroup
		Ogre::DataStreamPtr stream = Ogre::ResourceGroupManager::getSingleton().openResource(this->projectParameter.projectName + "/" + this->projectParameter.sceneName + ".scene", this->resourceGroupName);

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
			const auto& gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->parsedGameObjectIds[i]);
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
		this->showProgress = false;
		bool success = false;
		// Get the xml file from the resourcegroup
		Ogre::DataStreamPtr stream = Ogre::ResourceGroupManager::getSingleton().openResource(this->projectParameter.projectName + "/" + this->projectParameter.sceneName + ".scene", this->resourceGroupName);

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

	std::vector<std::pair<Ogre::Vector2, Ogre::String>> DotSceneImportModule::parseExitDirectionsNextWorlds(void)
	{
		std::vector<std::pair<Ogre::Vector2, Ogre::String>> exitDirectionsNextWorlds;

		// Do not show progress in gui because it will crash
		this->showProgress = false;
		bool success = false;
		// Get the xml file from the resourcegroup
		Ogre::DataStreamPtr stream = Ogre::ResourceGroupManager::getSingleton().openResource(this->projectParameter.projectName + "/" + this->projectParameter.sceneName + ".scene", this->resourceGroupName);

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
					bool foundTargetWorldName = false;

					while (nullptr != propertyElement && (false == foundExitDirection || false == foundTargetWorldName))
					{
						if (XMLConverter::getAttrib(propertyElement, "name") == "TargetWorldName")
						{
							data.second = XMLConverter::getAttrib(propertyElement, "data");
							foundTargetWorldName = true;
						}
						else if (XMLConverter::getAttrib(propertyElement, "name") == "ExitDirection")
						{
							data.first = XMLConverter::getAttribVector2(propertyElement, "data");
							foundExitDirection = true;
						}
						propertyElement = propertyElement->next_sibling("property");
					}

					if (true == foundExitDirection && true == foundTargetWorldName)
					{
						exitDirectionsNextWorlds.emplace_back(data);
					}
				}
			}
			// Go to the next node
			nodeElement = nodeElement->next_sibling("node");
		}
		free(scene);

		return exitDirectionsNextWorlds;
	}

	std::pair<bool, Ogre::Vector3> DotSceneImportModule::parseGameObjectPosition(unsigned long id)
	{
		Ogre::Vector3 gameObjectPosition = Ogre::Vector3::ZERO;

		// Do not show progress in gui because it will crash
		this->showProgress = false;
		bool success = false;
		// Get the xml file from the resourcegroup
		Ogre::DataStreamPtr stream = Ogre::ResourceGroupManager::getSingleton().openResource(this->projectParameter.projectName + "/" + this->projectParameter.sceneName + ".scene", this->resourceGroupName);

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
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,  message);
			boost::shared_ptr<EventDataFeedback> eventDataFeedback(new EventDataFeedback(false, message));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataFeedback);
		}

		Ogre::String message = "[DotSceneImportModule] Parsing Scene file with version " + version;
		if (xmlRoot->first_attribute("ID"))
			message += ", id " + Ogre::String(xmlRoot->first_attribute("ID")->value());
		if (xmlRoot->first_attribute("sceneManager"))
			message += ", scene manager " + Ogre::String(xmlRoot->first_attribute("sceneManager")->value());
		if (xmlRoot->first_attribute("minOgreVersion"))
			message += ", min. Ogre version " + Ogre::String(xmlRoot->first_attribute("minOgreVersion")->value());
		if (xmlRoot->first_attribute("author"))
			message += ", author " + Ogre::String(xmlRoot->first_attribute("author")->value());

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, message);

		rapidxml::xml_node<> *pElement;

		// Process resource locations (?)
#if 1
		pElement = xmlRoot->first_node("resourceLocations");
		if (pElement)
			this->processResourceLocations(pElement);
#endif

		// Process environment (?)
		pElement = xmlRoot->first_node("environment");
		if (pElement)
			this->processEnvironment(pElement);

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
			this->processOgreRecast(pElement);
		}

		// Process nodes (?)
		pElement = xmlRoot->first_node("nodes");
		if (nullptr != pElement)
		{
			this->processNodes(pElement, nullptr, justSetValues);
		}

		// Set the bounds, to have it in core for public access
		Core::getSingletonPtr()->setCurrentWorldBounds(this->mostLeftNearPosition, this->mostRightFarPosition);
	}

	void DotSceneImportModule::processResourceLocations(rapidxml::xml_node<>* xmlNode)
	{
		rapidxml::xml_node<> *pElement;
		
		// resourceGroupName, type, path 
		std::vector<std::tuple<Ogre::String, Ogre::String, Ogre::String>> missingResourceGroupNamesForWorld;

		pElement = xmlNode->first_node("resourceLocation");
		while (pElement)
		{
			Ogre::String usedName = pElement->first_attribute("name")->value();
			Ogre::String usedType = pElement->first_attribute("type")->value();
			Ogre::String usedPath = pElement->first_attribute("path")->value();

			// Get all currently defined resource group names
			auto& resourceLocations = Ogre::ResourceGroupManager::getSingleton().getResourceGroups();
			bool foundResourceGroupName = false;
			bool foundResourceGroupPath = false;
			for (auto& resourceGroupName : resourceLocations)
			{
				if (usedName == resourceGroupName)
				{
					foundResourceGroupName = true;
					// here no break, because a resource group may match, but it may have several locations and it could be that a location is missing
				}
				for (auto& path : Ogre::ResourceGroupManager::getSingleton().getResourceLocationList(resourceGroupName))
				{
					if (usedPath == path->archive->getName())
					{
						foundResourceGroupPath = true;
						break;
					}
				}

				if (true == foundResourceGroupPath)
					break;
			}

			// Check if a resource is not defined in the corresponding cfg file and add it to the missing list, to late-initialize the resource, so that the world can be loaded properly
			if (false == foundResourceGroupName || false == foundResourceGroupPath)
			{
				missingResourceGroupNamesForWorld.emplace_back(usedName, usedType, usedPath);
			}

			pElement = pElement->next_sibling("resourceLocation");
		}

		for (size_t i = 0; i < missingResourceGroupNamesForWorld.size(); i++)
		{
			Ogre::ResourceGroupManager::getSingleton().addResourceLocation(std::get<2>(missingResourceGroupNamesForWorld[i]), 
				std::get<1>(missingResourceGroupNamesForWorld[i]), std::get<0>(missingResourceGroupNamesForWorld[i]));

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Warning: The resource group: '" + std::get<0>(missingResourceGroupNamesForWorld[i])
			+ "' or its location: '" + std::get<2>(missingResourceGroupNamesForWorld[i]) + "' has not been defined. It will now be initialized during level loading progress. "
				"This may take some time! Please define the data in the corresponding cfg file, so that all necessary resources are loaded at the starup of the application!");
			Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup(std::get<0>(missingResourceGroupNamesForWorld[i]), false);

		}
	}

	void DotSceneImportModule::processEnvironment(rapidxml::xml_node<> *xmlNode)
	{
		rapidxml::xml_node<> *pElement;
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
					// ambientLightUpperHemisphere *= Ogre::Math::PI;
				}
				subElement = pElement->first_node("ambientLightLowerHemisphere");
				if (subElement)
				{
					this->projectParameter.ambientLightLowerHemisphere = XMLConverter::parseColour(subElement);
					// ambientLightLowerHemisphere *= Ogre::Math::PI;
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
				this->sceneManager->setAmbientLight(this->projectParameter.ambientLightUpperHemisphere, this->projectParameter.ambientLightLowerHemisphere, 
					this->projectParameter.hemisphereDir, this->projectParameter.envmapScale);
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
					this->projectParameter.shadowDirLightTextureOffset = XMLConverter::getAttribReal(subElement, "offset", 0.6f);
				}
				subElement = pElement->first_node("shadowColor");
				if (subElement)
				{
					this->projectParameter.shadowColor = XMLConverter::parseColour(subElement);
				}
				subElement = pElement->first_node("shadowQuality");
				if (subElement)
				{
					this->projectParameter.shadowQualityIndex = XMLConverter::getAttribUnsignedInt(subElement, "index", 2);
				}
				subElement = pElement->first_node("ambientLightMode");
				if (subElement)
				{
					this->projectParameter.ambientLightModeIndex = XMLConverter::getAttribUnsignedInt(subElement, "index", 0);
				}
				this->sceneManager->setShadowFarDistance(this->projectParameter.shadowFarDistance);
				this->sceneManager->setShadowDirectionalLightExtrusionDistance(this->projectParameter.shadowDirectionalLightExtrusionDistance);
				this->sceneManager->setShadowDirLightTextureOffset(this->projectParameter.shadowDirLightTextureOffset);
				this->sceneManager->setShadowColour(this->projectParameter.shadowColor);

				NOWA::WorkspaceModule::getInstance()->setShadowQuality(static_cast<Ogre::HlmsPbs::ShadowFilter>(this->projectParameter.shadowQualityIndex), false);
				NOWA::WorkspaceModule::getInstance()->setAmbientLightMode(static_cast<Ogre::HlmsPbs::AmbientLightMode>(this->projectParameter.ambientLightModeIndex));
			}
		}
		
		// Light Forward
		{
			pElement = xmlNode->first_node("lightFoward");
			if (pElement)
			{
				rapidxml::xml_node<>* subElement = pElement->first_node("forwardMode");
				if (subElement)
				{
					this->projectParameter.forwardMode = XMLConverter::getAttribUnsignedInt(subElement, "value");
				}
				if (0 != this->projectParameter.forwardMode)
				{
					subElement = pElement->first_node("lightWidth");
					if (subElement)
					{
						this->projectParameter.lightWidth = XMLConverter::getAttribUnsignedInt(subElement, "value");
					}
					subElement = pElement->first_node("lightHeight");
					if (subElement)
					{
						this->projectParameter.lightHeight = XMLConverter::getAttribUnsignedInt(subElement, "value");
					}
					subElement = pElement->first_node("numLightSlices");
					if (subElement)
					{
						this->projectParameter.numLightSlices = XMLConverter::getAttribUnsignedInt(subElement, "value");
					}
					subElement = pElement->first_node("lightsPerCell");
					if (subElement)
					{
						this->projectParameter.lightsPerCell = XMLConverter::getAttribUnsignedInt(subElement, "value");
					}
					subElement = pElement->first_node("decalsPerCell");
					if (subElement)
					{
						this->projectParameter.decalsPerCell = XMLConverter::getAttribUnsignedInt(subElement, "value");
					}
					subElement = pElement->first_node("cubemapProbesPerCell");
					if (subElement)
					{
						this->projectParameter.cubemapProbesPerCell = XMLConverter::getAttribUnsignedInt(subElement, "value");
					}
					subElement = pElement->first_node("minLightDistance");
					if (subElement)
					{
						this->projectParameter.minLightDistance = XMLConverter::getAttribReal(subElement, "value");
					}
					subElement = pElement->first_node("maxLightDistance");
					if (subElement)
					{
						this->projectParameter.maxLightDistance = XMLConverter::getAttribReal(subElement, "value");
					}
				}

				// Ogre tells that width/height must be modular devideable by 4
				if (this->projectParameter.lightWidth % ARRAY_PACKED_REALS != 0)
				{
					this->projectParameter.lightWidth = 4;
				}
				if (this->projectParameter.lightHeight % ARRAY_PACKED_REALS != 0)
				{
					this->projectParameter.lightHeight = 4;
				}

				if (0 == this->projectParameter.forwardMode)
				{
					this->sceneManager->setForward3D(false, this->projectParameter.lightWidth, this->projectParameter.lightHeight, this->projectParameter.numLightSlices, 
						this->projectParameter.lightsPerCell, this->projectParameter.minLightDistance, this->projectParameter.maxLightDistance);

					this->sceneManager->setForwardClustered(false, this->projectParameter.lightWidth, this->projectParameter.lightHeight, this->projectParameter.numLightSlices, 
						this->projectParameter.lightsPerCell, this->projectParameter.decalsPerCell, this->projectParameter.cubemapProbesPerCell, 
						this->projectParameter.minLightDistance, this->projectParameter.maxLightDistance);
				}
				else if (1 == this->projectParameter.forwardMode)
				{
					this->sceneManager->setForward3D(true, this->projectParameter.lightWidth, this->projectParameter.lightHeight, this->projectParameter.numLightSlices, 
						this->projectParameter.lightsPerCell, this->projectParameter.minLightDistance, this->projectParameter.maxLightDistance);
				}
				else if (2 == this->projectParameter.forwardMode)
				{
					this->sceneManager->setForwardClustered(true, this->projectParameter.lightWidth, this->projectParameter.lightHeight, this->projectParameter.numLightSlices, 
						this->projectParameter.lightsPerCell, this->projectParameter.decalsPerCell, this->projectParameter.cubemapProbesPerCell, 
						this->projectParameter.minLightDistance, this->projectParameter.maxLightDistance);
				}
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
					NOWA::Core::getSingletonPtr()->setGlobalRenderDistance(this->projectParameter.renderDistance);
				}

				subElement = pElement->first_node("useV2Item");
				if (subElement)
				{
					this->projectParameter.useV2Item = XMLConverter::getAttribBool(subElement, "value");
					Core::getSingletonPtr()->setUseEntityType(!this->projectParameter.useV2Item);
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
			this->projectParameter.angularDamping = XMLConverter::parseVector3(pElement);

		pElement = xmlNode->first_node("globalGravity");
		if (nullptr != pElement)
			this->projectParameter.gravity = XMLConverter::parseVector3(pElement);

		AppStateManager::getSingletonPtr()->getOgreNewtModule()->setGlobalGravity(this->projectParameter.gravity);
		this->ogreNewt = AppStateManager::getSingletonPtr()->getOgreNewtModule()->createPhysics(AppStateManager::getSingletonPtr()->getCurrentAppStateName() + "_world", 
			this->projectParameter.solverModel, this->projectParameter.broadPhaseAlgorithm, 
			this->projectParameter.solverForSingleIsland, this->projectParameter.physicsThreadCount,
			this->projectParameter.physicsUpdateRate, this->projectParameter.linearDamping, this->projectParameter.angularDamping);
	}

	void NOWA::DotSceneImportModule::processOgreRecast(rapidxml::xml_node<>* xmlNode)
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

		rapidxml::xml_node<> *pElement;

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
					Ogre::String message = "[DotSceneImportModule] Cannot undo deletion of game object id: " + Ogre::StringConverter::toString(this->missingGameObjectIds[i])
										+ " because somehow the game object has not been snapshotted as the simulation mode started.";
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
		bool skip = false;
		if (true == this->showProgress)
		{
			Core::getSingletonPtr()->getEngineResourceListener()->scriptParseStarted("Node", skip);
		}

		// Namen erstellen
		Ogre::String name = XMLConverter::getAttrib(xmlNode, "name");

		// Szenenknoten erstellen
		Ogre::SceneNode* pNode = 0;
		if (true == name.empty())
		{
			if (nullptr != parent)
			{
				pNode = parent->createChildSceneNode(Ogre::SCENE_STATIC);
			}
			else
			{
				pNode = this->sceneManager->getRootSceneNode()->createChildSceneNode(Ogre::SCENE_STATIC);
			}
		}
		else
		{
			bool foundNode = false;
			// Snapshot is loaded, scenenodes should exist already, find the node
			if (true == this->bIsSnapshot)
			{
				// Damn it, scene node names are not unique!
				const auto& nodesList = this->sceneManager->findSceneNodes(name);
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
							if (nullptr != node)
							{
								auto gameObject = Ogre::any_cast<GameObject*>(node->getUserObjectBindings().getUserAny());
								if (nullptr != gameObject)
								{
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
				}
			}
			
			if (false == foundNode)
			{
				// Must not set values, because node does not exist and game object does also not exist and must be created!
				justSetValues = false;
				if (nullptr != parent)
				{
					pNode = parent->createChildSceneNode(Ogre::SCENE_STATIC);
					pNode->setName(name);
				}
				else
				{
					pNode = this->sceneManager->getRootSceneNode()->createChildSceneNode(Ogre::SCENE_STATIC);
					pNode->setName(name);
				}
			}
		}

		// Go through other attributes
		Ogre::String id = XMLConverter::getAttrib(xmlNode, "id");
		bool isTarget = XMLConverter::getAttribBool(xmlNode, "isTarget");

		rapidxml::xml_node<>* pElement;

		// Position (?)
		pElement = xmlNode->first_node("position");
		if (pElement)
		{
			pNode->setPosition(XMLConverter::parseVector3(pElement));
		}

		// Rotation (?)
		pElement = xmlNode->first_node("rotation");
		if (pElement)
		{
			pNode->setOrientation(XMLConverter::parseQuaternion(pElement));
		}

		// Scale (?)
		pElement = xmlNode->first_node("scale");
		if (pElement)
		{
			pNode->setScale(XMLConverter::parseVector3(pElement));
		}
		// callback to react on postload
		if (this->worldLoaderCallback)
		{
			this->worldLoaderCallback->onPostLoadSceneNode(pNode);
		}
		// Process node (*)
		pElement = xmlNode->first_node("node");
		while (pElement)
		{
			// Recursion
			this->processNode(pElement, pNode, justSetValues);
			pElement = pElement->next_sibling("node");
		}

		if (this->showProgress)
		{
			Core::getSingletonPtr()->getEngineResourceListener()->scriptParseStarted("Objects", skip);
		}
		// Process entity (*)
		pElement = xmlNode->first_node("entity");
		while (pElement)
		{
			this->processEntity(pElement, pNode, justSetValues);
			pElement = pElement->next_sibling("entity");
		}

		// Process item (*)
		pElement = xmlNode->first_node("item");
		while (pElement)
		{
			this->processItem(pElement, pNode, justSetValues);
			pElement = pElement->next_sibling("item");
		}
		// Process manual object (*)
		pElement = xmlNode->first_node("manualObject");
		while (pElement)
		{
			this->processManualObject(pElement, pNode);
			pElement = pElement->next_sibling("manualObject");
		}
		// Process terra (*)
		pElement = xmlNode->first_node("terra");
		while (pElement)
		{
			this->processTerra(pElement, pNode, justSetValues);
			pElement = pElement->next_sibling("terra");
		}

		if (this->showProgress)
		{
			Core::getSingletonPtr()->getEngineResourceListener()->scriptParseEnded("Objects finished", skip);
		}

		// Process plane (*)
		pElement = xmlNode->first_node("plane");
		while (pElement)
		{
			processPlane(pElement, pNode, justSetValues);
			pElement = pElement->next_sibling("plane");
		}

		if (true == this->showProgress)
		{
			Core::getSingletonPtr()->getEngineResourceListener()->scriptParseEnded("Node", skip);
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

		Ogre::String tempMeshFile = meshFile;
		GameObject::eType type = GameObject::ITEM;

		if (Ogre::String::npos != tempMeshFile.find("Plane"))
		{
			tempMeshFile = "Missing.mesh";
		}

		float currentTime = static_cast<Ogre::Real>(Core::getSingletonPtr()->getOgreTimer()->getMilliseconds()) * 0.001f;
		// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DotSceneImportModule] Parse item: " + name + " mesh: " + meshFile);

		Ogre::Item* item = nullptr;

		unsigned long missingGameObjectId = 0;
		if (true == justSetValues && false == this->missingGameObjectIds.empty())
		{
			rapidxml::xml_node<>* propertyElement = xmlNode->first_node("property");
			findGameObjectId(propertyElement, missingGameObjectId);
		}
		if (false == justSetValues || missingGameObjectId != 0)
		{
			Ogre::v1::MeshPtr v1Mesh;
			Ogre::MeshPtr v2Mesh;

			try
			{
				if ((v1Mesh = Ogre::v1::MeshManager::getSingletonPtr()->getByName(tempMeshFile, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME)) == nullptr)
				{
					v1Mesh = Ogre::v1::MeshManager::getSingletonPtr()->load(tempMeshFile, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME,
																			Ogre::v1::HardwareBuffer::HBU_STATIC, Ogre::v1::HardwareBuffer::HBU_STATIC);
				}

				// Ogre::Root::getSingletonPtr()->renderOneFrame();

				if (nullptr == v1Mesh)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[EditorManager] Error cannot create entity, because the mesh: '"
																	+ tempMeshFile + "' could not be created.");
					return;
				}

				if ((v2Mesh = Ogre::MeshManager::getSingletonPtr()->getByName(tempMeshFile, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME)) == nullptr)
				{
					v2Mesh = Ogre::MeshManager::getSingletonPtr()->createByImportingV1(tempMeshFile, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, v1Mesh.get(), true, true, true);
				}
				v1Mesh->unload();
				
			}
			catch (Ogre::Exception& e)
			{
				try
				{
					if ((v2Mesh = Ogre::MeshManager::getSingletonPtr()->getByName(tempMeshFile, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME)) == nullptr)
					{
						v2Mesh = Ogre::MeshManager::getSingletonPtr()->load(tempMeshFile, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
					}
				}
				catch (...)
				{
					// Plane has no mesh yet, so skip error
					if (Ogre::String::npos == meshFile.find("Plane"))
					{
						Ogre::String description = e.getFullDescription();
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Error loading an item with the name: " + name + " and mesh file name: " + meshFile
																		+ "! So setting 'Missing.mesh'. Details: " + description);
					}
					// Try to load the missing.mesh to attend the user, that a resource is missing
					try
					{
						tempMeshFile = "Missing.mesh";
						v1Mesh = Ogre::v1::MeshManager::getSingleton().load(tempMeshFile, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, Ogre::v1::HardwareBuffer::HBU_STATIC, Ogre::v1::HardwareBuffer::HBU_STATIC);

						// Destroy a potential plane v2, because an error occurs (plane with name ... already exists)
						Ogre::ResourcePtr resourceV2 = Ogre::MeshManager::getSingletonPtr()->getResourceByName(name);
						if (nullptr != resourceV2)
						{
							Ogre::MeshManager::getSingletonPtr()->destroyResourcePool(name);
							Ogre::MeshManager::getSingletonPtr()->remove(resourceV2->getHandle());
						}

						if ((v2Mesh = Ogre::MeshManager::getSingletonPtr()->getByName(tempMeshFile, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME)) == nullptr)
						{
							v2Mesh = Ogre::MeshManager::getSingletonPtr()->createByImportingV1(name, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, v1Mesh.get(), true, true, true);
						}
						v1Mesh->unload();
					}
					catch (Ogre::Exception& e)
					{
						Ogre::String description = e.getFullDescription();
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Even 'Missing.mesh' from NOWA folder cannot be loaded, so nobody can help you anymore. Skipping this entity. Details: " + description);
						return;
					}
				}
			}
			DeployResourceModule::getInstance()->tagResource(tempMeshFile, v2Mesh->getGroup());

			// Always create scene dynamic and later in game object change the type, to what has been configured
			item = this->sceneManager->createItem(v2Mesh, Ogre::SCENE_STATIC);
			item->setQueryFlags(Core::getSingletonPtr()->UNUSEDMASK);
			item->setName(name);
			item->setCastShadows(castShadows);
			parent->attachObject(item);
			item->setVisible(visible);
		}

		// callback to react on postload
		if (this->worldLoaderCallback)
		{
			this->worldLoaderCallback->onPostLoadMovableObject(item);
		}

		if (false == justSetValues || missingGameObjectId != 0)
		{
			rapidxml::xml_node<>* pElement = xmlNode->first_node("subitem");

			size_t subItemIndexCount = 0;
			while (pElement)
			{
				// Read either maternal name (old) or data block name (new)
				Ogre::String materialFile = XMLConverter::getAttrib(pElement, "datablockName");
				if (false == materialFile.empty())
				{
					// Ogre::MaterialPtr materialPtr = Ogre::MaterialManager::getSingleton().getByName(materialFile);
					// if (false == materialPtr.isNull())
					Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingleton().getHlmsManager();
					Ogre::HlmsDatablock* block = hlmsManager->getDatablockNoDefault(materialFile);
					if (nullptr != block)
					{
						item->getSubItem(subItemIndexCount)->setDatablock(materialFile);
						const Ogre::String* currentDatablockName = item->getSubItem(subItemIndexCount)->getDatablock()->getNameStr();
						if (nullptr == currentDatablockName || *currentDatablockName != materialFile)
						{
							Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Warning: Could not set datablock name: " + materialFile + ", because propably the mesh has not tangents!");
						}
					}
					else
					{
						// Missing data block with ? comes from NOWA resources
						/*if (subEntityIndexCount < entity->getNumSubEntities())
						{
							entity->getSubEntity(subEntityIndexCount)->setDatablock("Missing");
						}*/
						// Since Missing.mesh has just one sub entity, do not iterate through sub entities!
						break;
					}

					// callback to react on postload
					if (this->worldLoaderCallback)
					{
						this->worldLoaderCallback->onPostLoadMovableObject(item);
					}
					subItemIndexCount++;
				}
				pElement = pElement->next_sibling("subitem");
			}
		}
		
		// Check if the entity element has user data, for game object creation
		rapidxml::xml_node<>* pElement = xmlNode->first_node("userData");
		if (pElement)
		{
			GameObjectPtr gameObjectPtr = nullptr;
			if (false == justSetValues || missingGameObjectId != 0)
			{
				gameObjectPtr = GameObjectFactory::getInstance()->createOrSetGameObjectFromXML(pElement, this->sceneManager, parent, item,
					GameObject::ITEM, this->worldPath, this->forceCreation, this->bSceneParsed);
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
						if (propertyElement && attrib == "Id")
						{
							unsigned long existingGameObjectId = XMLConverter::getAttribUnsignedLong(propertyElement, "data");
							GameObjectPtr existingGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(existingGameObjectId);
							if (nullptr == existingGameObjectPtr)
							{
								// Not found, is illegal do nothing
								break;
							}

							gameObjectPtr = GameObjectFactory::getInstance()->createOrSetGameObjectFromXML(pElement, this->sceneManager, parent, item,
								type, this->worldPath, this->forceCreation, this->bSceneParsed, existingGameObjectPtr);
							foundId = true;
						}
						else
						{
							propertyElement = propertyElement->next_sibling("property");
						}
					} while (nullptr != propertyElement && false == foundId);
				}
			}

			//  bSceneParsed: When a game object is loaded an clamp y attribute set to true, a raycast is performed and the game object's y adapted.
			//	This is useful e.g. if a player should each stand on the ground when a different world is loaded (scene parsed), that starts at a different height. Yet its dangerous e.g.
			//	when just a group is loaded, because this would cause in a wrong y position.

			if (nullptr != gameObjectPtr)
			{
				gameObjectPtr->setOriginalMeshNameOnLoadFailure(meshFile);
				this->parsedGameObjectIds.emplace_back(gameObjectPtr->getId());

				if ("SunLight" == gameObjectPtr->getSceneNode()->getName())
				{
					this->sunLight = NOWA::makeStrongPtr(gameObjectPtr->getComponent<LightDirectionalComponent>())->getOgreLight();
				}
			}

			float dt = (static_cast<Ogre::Real>(Core::getSingletonPtr()->getOgreTimer()->getMilliseconds()) * 0.001f) - currentTime;
			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DotSceneImportModule] Parse end item: " + name + " mesh: " + meshFile + " duration: " + Ogre::StringConverter::toString(dt) + " seconds.");
		}
	}

	void DotSceneImportModule::processEntity(rapidxml::xml_node<>* xmlNode, Ogre::SceneNode* parent, bool justSetValues)
	{
		// Process attributes
		Ogre::String name = XMLConverter::getAttrib(xmlNode, "name");
		Ogre::String id = XMLConverter::getAttrib(xmlNode, "id");
		Ogre::String meshFile = XMLConverter::getAttrib(xmlNode, "meshFile");
		bool castShadows = XMLConverter::getAttribBool(xmlNode, "castShadows", true);
		bool visible = XMLConverter::getAttribBool(xmlNode, "visible", true);
		Ogre::Real lodDistance = XMLConverter::getAttribReal(xmlNode, "lodDistance", 0.0f);

		Ogre::String tempMeshFile = meshFile;

		float currentTime = static_cast<Ogre::Real>(Core::getSingletonPtr()->getOgreTimer()->getMilliseconds()) * 0.001f;
		// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DotSceneImportModule] Parse entity: " + name + " mesh: " + meshFile);

		// later maybe for complex objects
		// http://www.ogre3d.org/tikiwiki/tiki-index.php?page=PixelCountLodStrategy&structure=Cookbook
		Ogre::v1::Entity* entity = nullptr;
		Ogre::Item* item = nullptr;
		Ogre::v1::MeshPtr v1Mesh;
		Ogre::MeshPtr v2Mesh;
		GameObject::eType type = GameObject::ENTITY;
		// Attention with hbu_static, there is also dynamic

		if (Ogre::String::npos != tempMeshFile.find("Plane"))
		{
			tempMeshFile = "Missing.mesh";
		}

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

		// Maybe create, if its an already missing game object id
		if (false == justSetValues || missingGameObjectId != 0)
		{
			try
			{
				v1Mesh = Ogre::v1::MeshManager::getSingletonPtr()->load(tempMeshFile, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, Ogre::v1::HardwareBuffer::HBU_STATIC, Ogre::v1::HardwareBuffer::HBU_STATIC);

				// This is really expensive!
#if 0
				// Prepares for shadow mapping
				const bool oldValue = Ogre::v1::Mesh::msOptimizeForShadowMapping;
				Ogre::v1::Mesh::msOptimizeForShadowMapping = true;
				v1Mesh->prepareForShadowMapping(false);
				Ogre::v1::Mesh::msOptimizeForShadowMapping = oldValue;
#endif
#if 0
				if (lodDistance > 0.0f)
				{
					// Generate LOD levels
					Ogre::LodConfig lodConfig;

					Ogre::MeshLodGenerator lodGenerator;
					lodGenerator.getAutoconfig(v1Mesh, lodConfig);

					lodConfig.strategy = Ogre::LodStrategyManager::getSingleton().getDefaultStrategy();

					for (auto it = lodConfig.levels.begin(); it != lodConfig.levels.end(); ++it)
					{
						// E.g. 10 meters = 5421,7295 * x -> x = 10 / 5421,7295 = 0.001844
						Ogre::Real value = lodDistance / it->distance;
						it->distance = lodConfig.strategy->transformUserValue(value);
					}

					lodGenerator.generateLodLevels(lodConfig);
				}
#endif
				/*Ogre::ResourceBackgroundQueue* rbq = Ogre::ResourceBackgroundQueue::getSingletonPtr();

				v1Mesh->setBackgroundLoaded(true);
				rbq->load(Ogre::MeshManager::getSingletonPtr()->getResourceType(), v1Mesh->getName(), Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, false, 0, 0, 0);*/
				DeployResourceModule::getInstance()->tagResource(tempMeshFile, v1Mesh->getGroup());
			}
			catch (Ogre::Exception& e)
			{
				// Plane has no mesh yet, so skip error
				if (Ogre::String::npos == meshFile.find("Plane"))
				{
					Ogre::String description = e.getFullDescription();
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Error loading an entity with the name: " + name + " and mesh file name: " + meshFile
						+ "! So setting 'Missing.mesh'. Details: " + description);
				}
				// Try to load the missing.mesh to attend the user, that a resource is missing
				try
				{
					tempMeshFile = "Missing.mesh";
					v1Mesh = Ogre::v1::MeshManager::getSingleton().load(tempMeshFile, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME, Ogre::v1::HardwareBuffer::HBU_STATIC, Ogre::v1::HardwareBuffer::HBU_STATIC);

					// Destroy a potential plane v2, because an error occurs (plane with name ... already exists)
					Ogre::ResourcePtr resourceV2 = Ogre::MeshManager::getSingletonPtr()->getResourceByName(name);
					if (nullptr != resourceV2)
					{
						Ogre::MeshManager::getSingletonPtr()->destroyResourcePool(name);
						Ogre::MeshManager::getSingletonPtr()->remove(resourceV2->getHandle());
					}

					v2Mesh = Ogre::MeshManager::getSingletonPtr()->createByImportingV1(name, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, v1Mesh.get(), true, true, true);
					v1Mesh->unload();
					v1Mesh.setNull();
				}
				catch (Ogre::Exception& e)
				{
					Ogre::String description = e.getFullDescription();
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Even 'Missing.mesh' from NOWA folder cannot be loaded, so nobody can help you anymore. Skipping this entity. Details: " + description);
					return;
				}

				type = GameObject::ITEM;
				DeployResourceModule::getInstance()->tagResource(tempMeshFile, v2Mesh->getGroup());
			}
		}

		if (false == justSetValues || missingGameObjectId != 0)
		{
			if (GameObject::ENTITY == type)
			{
				// Always create scene dynamic and later in game object change the type, to what has been configured
				entity = this->sceneManager->createEntity(v1Mesh, Ogre::SCENE_STATIC);
				entity->setQueryFlags(Core::getSingletonPtr()->UNUSEDMASK);
				parent->attachObject(entity);

				entity->setName(name);
				entity->setCastShadows(castShadows);
				// Note: visible must be done after attaching!
				entity->setVisible(visible);
			}
			else
			{
				item = this->sceneManager->createItem(v2Mesh, Ogre::SCENE_STATIC);
				item->setQueryFlags(Core::getSingletonPtr()->UNUSEDMASK);
				parent->attachObject(item);

				item->setName(name);
				item->setCastShadows(castShadows);
				// Note: visible must be done after attaching!
				item->setVisible(visible);
			}
		}

		// callback to react on postload
		if (this->worldLoaderCallback)
		{
			this->worldLoaderCallback->onPostLoadMovableObject(entity);
		}

		if (false == justSetValues || missingGameObjectId != 0)
		{
			rapidxml::xml_node<>* pElement = xmlNode->first_node("subentity");

			size_t subEntityIndexCount = 0;
			while (pElement)
			{
				// Read either maternal name (old) or data block name (new)
				Ogre::String materialFile = XMLConverter::getAttrib(pElement, "materialName");
				if (true == materialFile.empty())
				{
					materialFile = XMLConverter::getAttrib(pElement, "datablockName");
				}
				if (false == materialFile.empty())
				{
					// Ogre::MaterialPtr materialPtr = Ogre::MaterialManager::getSingleton().getByName(materialFile);
					// if (false == materialPtr.isNull())
					Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingleton().getHlmsManager();
					Ogre::HlmsDatablock* block = hlmsManager->getDatablockNoDefault(materialFile);
					if (nullptr != block)
					{
						if (GameObject::ENTITY == type)
						{
							entity->getSubEntity(subEntityIndexCount)->setDatablock(materialFile);
							const Ogre::String* currentDatablockName = entity->getSubEntity(subEntityIndexCount)->getDatablock()->getNameStr();
							if (nullptr == currentDatablockName || *currentDatablockName != materialFile)
							{
								Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Warning: Could not set datablock name: " + materialFile + ", because propably the mesh has not tangents!");
							}
						}
						else
						{
							item->getSubItem(subEntityIndexCount)->setDatablock(materialFile);
							const Ogre::String* currentDatablockName = item->getSubItem(subEntityIndexCount)->getDatablock()->getNameStr();
							if (nullptr == currentDatablockName || *currentDatablockName != materialFile)
							{
								Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Warning: Could not set datablock name: " + materialFile + ", because propably the mesh has not tangents!");
							}
						}

					}
					else
					{
						// Missing data block with ? comes from NOWA resources
						/*if (subEntityIndexCount < entity->getNumSubEntities())
						{
							entity->getSubEntity(subEntityIndexCount)->setDatablock("Missing");
						}*/
						// Since Missing.mesh has just one sub entity, do not iterate through sub entities!
						break;
					}

					// callback to react on postload
					if (this->worldLoaderCallback)
					{
						this->worldLoaderCallback->onPostLoadMovableObject(entity);
					}
					subEntityIndexCount++;
				}
				pElement = pElement->next_sibling("subentity");
			}
		}

		// Check if the entity element has user data, for game object creation
		rapidxml::xml_node<>* pElement = xmlNode->first_node("userData");
		if (pElement)
		{
			GameObjectPtr gameObjectPtr = nullptr;
			
			if (false == justSetValues || missingGameObjectId != 0)
			{
				// Backward compatibility: Plane is now v2, but if its loaded as if its an entity, it must be converted to item!
				if (GameObject::ENTITY == type)
				{
					gameObjectPtr = GameObjectFactory::getInstance()->createOrSetGameObjectFromXML(pElement, this->sceneManager, parent, entity,
						type, this->worldPath, this->forceCreation, this->bSceneParsed);
				}
				else
				{
					gameObjectPtr = GameObjectFactory::getInstance()->createOrSetGameObjectFromXML(pElement, this->sceneManager, parent, item,
						type, this->worldPath, this->forceCreation, this->bSceneParsed);
				}
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
						if (propertyElement && attrib == "Id")
						{
							unsigned long existingGameObjectId = XMLConverter::getAttribUnsignedLong(propertyElement, "data");
							GameObjectPtr existingGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(existingGameObjectId);

							gameObjectPtr = GameObjectFactory::getInstance()->createOrSetGameObjectFromXML(pElement, this->sceneManager, parent, entity,
								type, this->worldPath, this->forceCreation, this->bSceneParsed, existingGameObjectPtr);
							foundId = true;
						}
						else
						{
							propertyElement = propertyElement->next_sibling("property");
						}
					} while (nullptr != propertyElement && false == foundId);
				}
			}

			//  bSceneParsed: When a game object is loaded an clamp y attribute set to true, a raycast is performed and the game object's y adapted.
			//	This is useful e.g. if a player should each stand on the ground when a different world is loaded (scene parsed), that starts at a different height. Yet its dangerous e.g.
			//	when just a group is loaded, because this would cause in a wrong y position.

			if (nullptr != gameObjectPtr)
			{
				gameObjectPtr->setOriginalMeshNameOnLoadFailure(meshFile);
				this->parsedGameObjectIds.emplace_back(gameObjectPtr->getId());

				if ("SunLight" == gameObjectPtr->getSceneNode()->getName())
				{
					this->sunLight = NOWA::makeStrongPtr(gameObjectPtr->getComponent<LightDirectionalComponent>())->getOgreLight();
				}
			}

			float dt = (static_cast<Ogre::Real>(Core::getSingletonPtr()->getOgreTimer()->getMilliseconds()) * 0.001f) - currentTime;
			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DotSceneImportModule] Parse end entity: " + name + " mesh: " + meshFile + " duration: " + Ogre::StringConverter::toString(dt) + " seconds");
		}
	}

	void NOWA::DotSceneImportModule::processManualObject(rapidxml::xml_node<>* xmlNode, Ogre::SceneNode* parent)
	{
		// Process attributes
		Ogre::String name = XMLConverter::getAttrib(xmlNode, "name");
		Ogre::String id = XMLConverter::getAttrib(xmlNode, "id");
		//wird nicht benoetigt stammt von den Machern vom Ogitor
		//bool isStatic = XMLConverter::getAttribBool(XMLNode, "static", false);
		bool castShadows = XMLConverter::getAttribBool(xmlNode, "castShadows", true);
		bool visible = XMLConverter::getAttribBool(xmlNode, "visible", true);

		rapidxml::xml_node<>* pElement;

		// Check if the entity element has user data, for game object creation
		pElement = xmlNode->first_node("userData");
		if (pElement)
		{
			GameObjectPtr gameObjectPtr = GameObjectFactory::getInstance()->createOrSetGameObjectFromXML(pElement, this->sceneManager, parent, nullptr, GameObject::MANUAL_OBJECT,
				this->worldPath, this->forceCreation, this->bSceneParsed);

			//  bSceneParsed: When a game object is loaded an clamp y attribute set to true, a raycast is performed and the game object's y adapted.
			//	This is useful e.g. if a player should each stand on the ground when a different world is loaded (scene parsed), that starts at a different height. Yet its dangerous e.g.
			//	when just a group is loaded, because this would cause in a wrong y position.

			if (nullptr != gameObjectPtr)
			{
				this->parsedGameObjectIds.emplace_back(gameObjectPtr->getId());

				// Check if its the sun light and get the light pointer
				if ("SunLight" == gameObjectPtr->getSceneNode()->getName())
				{
					this->sunLight = NOWA::makeStrongPtr(gameObjectPtr->getComponent<LightDirectionalComponent>())->getOgreLight();
				}
			}
		}
	}

	void NOWA::DotSceneImportModule::processTerra(rapidxml::xml_node<>* xmlNode, Ogre::SceneNode* parent, bool justSetValues)
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
				findGameObjectId(propertyElement, missingGameObjectId);
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
				gameObjectPtr = GameObjectFactory::getInstance()->createOrSetGameObjectFromXML(pElement, this->sceneManager, parent, nullptr, GameObject::TERRA,
					this->worldPath, this->forceCreation, false);

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

							gameObjectPtr = GameObjectFactory::getInstance()->createOrSetGameObjectFromXML(pElement, this->sceneManager, parent, nullptr, GameObject::TERRA,
								this->worldPath, this->forceCreation, false, existingGameObjectPtr);

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

		// Maybe create, if its an already missing game object id
		if (false == justSetValues || missingGameObjectId != 0)
		{

			Ogre::Plane plane(normal, distance);
			Ogre::v1::MeshPtr planeMeshV1;

			planeMeshV1 = Ogre::v1::MeshManager::getSingletonPtr()->createPlane(name + "mesh", "General", plane, width, height, xSegments, ySegments, hasNormals,
				numTexCoordSets, uTile, vTile, up, Ogre::v1::HardwareBuffer::HBU_STATIC, Ogre::v1::HardwareBuffer::HBU_STATIC);

			DeployResourceModule::getInstance()->tagResource(name + "mesh", planeMeshV1->getGroup());

			Ogre::MeshPtr v2Mesh = Ogre::MeshManager::getSingletonPtr()->createByImportingV1(name, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, planeMeshV1.get(), true, true, true);
			planeMeshV1->unload();
			planeMeshV1.setNull();

			// Ogre::MeshPtr v2Mesh = Ogre::MeshManager::getSingleton().createManual("Plane", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
			// v2Mesh->importV1(planeMeshV1.get(), true, true, true);
			// planeMeshV1->unload();

			item = this->sceneManager->createItem(v2Mesh, Ogre::SCENE_STATIC);
			item->setName(name + "mesh");

			// MathHelper::getInstance()->ensureHasTangents(entity->getMesh());
			// MathHelper::getInstance()->substractOutTangentsForShader(entity);

			Ogre::MaterialPtr objectMaterial = Ogre::MaterialManager::getSingleton().getByName(materialFile);
			if (false == materialFile.empty())
			{
				item->setDatablock(materialFile);
			}

			// Change the addressing mode of the roughness map to wrap via code.
			// Detail maps default to wrap, but the rest to clamp.
			Ogre::HlmsPbsDatablock* datablock = static_cast<Ogre::HlmsPbsDatablock*>(item->getSubItem(0)->getDatablock());
			//Make a hard copy of the sampler block
			Ogre::HlmsSamplerblock samplerblock(*datablock->getSamplerblock(Ogre::PBSM_ROUGHNESS));
			samplerblock.mU = Ogre::TAM_WRAP;
			samplerblock.mV = Ogre::TAM_WRAP;
			samplerblock.mW = Ogre::TAM_WRAP;
			//Set the new samplerblock. The Hlms system will
			//automatically create the API block if necessary
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

		rapidxml::xml_node<>* element = xmlNode->next_sibling("userData");

		GameObjectPtr gameObjectPtr = nullptr;

		if (false == justSetValues || missingGameObjectId != 0)
		{
			gameObjectPtr = GameObjectFactory::getInstance()->createOrSetGameObjectFromXML(element, this->sceneManager, parent, item, 
				GameObject::PLANE, this->worldPath, this->forceCreation, false);

			if (nullptr != gameObjectPtr)
			{
				this->parsedGameObjectIds.emplace_back(gameObjectPtr->getId());
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

						gameObjectPtr = GameObjectFactory::getInstance()->createOrSetGameObjectFromXML(element, this->sceneManager, parent, item,
							GameObject::PLANE, this->worldPath, this->forceCreation, this->bSceneParsed, gameObjectPtr);
						foundId = true;
					}
					else
					{
						propertyElement = propertyElement->next_sibling("property");
					}
				} while (false == foundId && nullptr != propertyElement);
			}
		}
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

	void NOWA::DotSceneImportModule::setIsSnapshot(bool bIsSnapshot)
	{
		this->bIsSnapshot = bIsSnapshot;
	}

	std::pair<Ogre::String, Ogre::String> NOWA::DotSceneImportModule::getProjectAndSceneName(const Ogre::String& filePathName, bool decrypt)
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

}; //Namespace end

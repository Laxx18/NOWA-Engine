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
		hasCaelumSystem(false),
		showProgress(false),
		forceCreation(false),
		bSceneParsed(false),
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
		hasCaelumSystem(false),
		showProgress(false),
		forceCreation(false),
		bSceneParsed(false),
#ifndef PAGEDGEOMETRY_NOT_PORTED
		// pagedGeometryModule(nullptr),
#endif
#ifndef HYDRAX_NOT_PORTED
		// hydraxModule(nullptr),
#endif
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
#ifndef CAELUM_NOT_PORTED
		/*if (this->caelumModule)
		{
			delete this->caelumModule;
			this->caelumModule = nullptr;
		}*/
#endif
#ifndef PAGEDGEOMETRY_NOT_PORTED
		/*if (this->pagedGeometryModule)
		{
			delete this->pagedGeometryModule;
			this->pagedGeometryModule = nullptr;
		}*/
#endif
#ifndef HYDRAX_NOT_PORTED
		/*if (this->hydraxModule)
		{
			delete this->hydraxModule;
			th*/is->hydraxModule = nullptr;
		}
#endif
#ifndef TERRAIN_NOT_PORTED
		/*if (this->terrainGroup)
		{
			OGRE_DELETE this->terrainGroup;
			this->terrainGroup = nullptr;
		}*/
#endif
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
		hasCaelumSystem(false),
		showProgress(false),
		forceCreation(false),
		bSceneParsed(false),
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

		// First process the usual scene with all the other stuff
		rapidxml::xml_document<> XMLDoc;

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
	
	bool DotSceneImportModule::parseGlobalScene(bool justSetValues)
	{
		rapidxml::xml_document<> XMLDoc;
		rapidxml::xml_node<>* xmlRoot;
		
		Ogre::String projectPath = Core::getSingletonPtr()->getSectionPath(this->resourceGroupName)[0];

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

		if (GetFileAttributes(globalSceneFilePathName.data()) & FileFlag)
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
		this->processScene(xmlRoot, justSetValues);

		this->bSceneParsed = false;

		return true;
	}

	bool DotSceneImportModule::parseScene(const Ogre::String& projectName, const Ogre::String& sceneName, const Ogre::String& resourceGroupName, Ogre::Light* sunLight,
		DotSceneImportModule::IWorldLoaderCallback* worldLoaderCallback, bool showProgress)
	{
		bool success = true;

		float currentTime = static_cast<Ogre::Real>(Core::getSingletonPtr()->getOgreTimer()->getMilliseconds()) * 0.001f;
		this->projectParameter.projectName = projectName;

		this->bSceneParsed = true;

		// Remove .scene
		Ogre::String tempSceneName = sceneName;
		size_t found = tempSceneName.find(".scene");
		if (found != std::wstring::npos)
		{
			tempSceneName = tempSceneName.substr(0, tempSceneName.size() - 6);
		}

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

		// First process the usual scene with all the other stuff
		rapidxml::xml_document<> XMLDoc;
		rapidxml::xml_node<>* xmlRoot;
		
		Ogre::String projectPath = Core::getSingletonPtr()->getSectionPath(this->resourceGroupName)[0];
		
		// Project is always: "Projects/ProjectName/SceneName.scene"
		// E.g.: "Projects/Plattformer/Level1.scene", "Projects/Plattformer/Level2.scene", "Projects/Plattformer/Level3.scene"
		this->worldPath = projectPath + "/" + this->projectParameter.projectName + "/" + this->projectParameter.sceneName + ".scene";

		std::ifstream ifs(this->worldPath);
		if (false == ifs.good())
		{
			success = false;
			// If there is no scene yet, try to parse a possible global scene
			/*success = this->parseGlobalScene();
			if (false == success)
			{
				Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Could not parse scene: '" + this->worldPath
					+ "' because it does not exist.");
			}*/
			return success;
		}

		// Announce the current world path to core
		Core::getSingletonPtr()->setCurrentWorldPath(this->worldPath);

		std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
		DWORD dwFileAttributes	= GetFileAttributes(this->worldPath.data());
		if (dwFileAttributes & FileFlag)
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
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[DotSceneImportModule] Could not parse scene: " + sceneName + " error: " + Ogre::String(error.what())
				+ " at: " + Ogre::String(error.where<char>()) + "\n", "NOWA");
		}
		/*catch (Ogre::FileNotFoundException& exception)
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Could not parse scene: " + sceneName 
				+ " because it does not exist. Error: " + exception.what());
			return false;
		}*/

		xmlRoot = XMLDoc.first_node("scene");
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
			this->parseGlobalScene();
		}

		// This game objects must be initialized before all other game objects are initialized, because they may need data from this game objects, like terra needs a camera
		this->postInitData();
		
		float dt = (static_cast<Ogre::Real>(Core::getSingletonPtr()->getOgreTimer()->getMilliseconds()) * 0.001f) - currentTime;
		Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[DotSceneImportModule] Parse end scene: " + sceneName + " duration: " + Ogre::StringConverter::toString(dt) + " seconds");

		// Ogre::Root::getSingletonPtr()->renderOneFrame();

		this->bSceneParsed = false;

		boost::shared_ptr<EventDataSceneParsed> eventDataSceneParsed(new EventDataSceneParsed());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataSceneParsed);

		return true;
	}

	void NOWA::DotSceneImportModule::postInitData()
	{
		auto& mainCameraGameObject = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(NOWA::GameObjectController::MAIN_CAMERA_ID);
		if (nullptr == mainCameraGameObject)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Error: Can not load world, because the MainCamera could not be created! See log for further information.");
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[DotSceneImportModule] Error: Can not load world, because the MainCamera could not be created! See log for further information.\n", "NOWA");
		}
		mainCameraGameObject->postInit();
		if (nullptr == this->mainCamera)
		{
			this->mainCamera = NOWA::makeStrongPtr(mainCameraGameObject->getComponent<CameraComponent>())->getCamera();
		}

		auto& mainLightGameObject = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(NOWA::GameObjectController::MAIN_LIGHT_ID);
		if (nullptr == mainLightGameObject)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Error: Can not load world, because the MainLight could not be created! See log for further information.");
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[DotSceneImportModule] Error: Can not load world, because the MainLight could not be created! See log for further information.\n", "NOWA");
		}
		mainLightGameObject->postInit();
		if (nullptr == this->sunLight)
		{
			this->sunLight = NOWA::makeStrongPtr(mainLightGameObject->getComponent<LightDirectionalComponent>())->getOgreLight();
		}

		auto& mainGameObject = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(NOWA::GameObjectController::MAIN_GAMEOBJECT_ID);
		if (nullptr == mainGameObject)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Error: Can not load world, because the MainGameObject could not be created. Maybe this is an old scene, which does not have a MainGameObject! See log for further information.");
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[DotSceneImportModule] Error: Can not load world, because the MainGameObject could not be created! See log for further information.\n", "NOWA");
		}
		mainGameObject->postInit();

		// Now that all gameobject's have been fully created, run the post init phase (now all other components are also available for each game object)
		for (auto& it = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects()->cbegin(); it != AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects()->cend(); ++it)
		{
			const auto& gameObjectPtr = it->second;
			if (gameObjectPtr->getId() != NOWA::GameObjectController::MAIN_CAMERA_ID
				&& gameObjectPtr->getId() != NOWA::GameObjectController::MAIN_LIGHT_ID
				&& gameObjectPtr->getId() != NOWA::GameObjectController::MAIN_GAMEOBJECT_ID)
			{
				if (!gameObjectPtr->postInit())
				{
					AppStateManager::getSingletonPtr()->getGameObjectController()->deleteGameObject(gameObjectPtr);
				}
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

		// Set unused mask for all camera, because log is spammed with exceptions
		/*Ogre::SceneManager::CameraIterator it = this->sceneManager->getCameraIterator();
		while (it.hasMoreElements())
		{
		Ogre::Camera* tempCamera = it.getNext();
		tempCamera->setQueryFlags(Core::getSingletonPtr()->UNUSEDMASK);
		}*/
	}

	bool NOWA::DotSceneImportModule::loadSceneSnapshot(const Ogre::String& filePathName)
	{
		bool success = true;

		float currentTime = static_cast<Ogre::Real>(Core::getSingletonPtr()->getOgreTimer()->getMilliseconds()) * 0.001f;

		// First process the usual scene with all the other stuff
		rapidxml::xml_document<> XMLDoc;
		rapidxml::xml_node<>* xmlRoot;

		std::ifstream ifs(filePathName);
		if (false == ifs.good())
		{
			success = false;
			// If there is no scene yet, try to parse a possible global scene
			/*success = this->parseGlobalScene();
			if (false == success)
			{
				Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Could not parse scene: '" + this->worldPath
					+ "' because it does not exist.");
			}*/
			return success;
		}

		std::string content((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
		DWORD dwFileAttributes = GetFileAttributes(this->worldPath.data());
		if (dwFileAttributes & FileFlag)
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
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[DotSceneImportModule] Could not load scene snapshot from: " + filePathName + " error: " + Ogre::String(error.what())
				+ " at: " + Ogre::String(error.where<char>()) + "\n", "NOWA");
		}
		/*catch (Ogre::FileNotFoundException& exception)
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Could not parse scene: " + sceneName
				+ " because it does not exist. Error: " + exception.what());
			return false;
		}*/

		xmlRoot = XMLDoc.first_node("scene");
		if (XMLConverter::getAttrib(xmlRoot, "formatVersion", "") == "")
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Error: Invalid .scene File. Missing <scene>");
			return false;
		}

		// Process the scene
		this->processScene(xmlRoot, true);

		this->postInitData();

		float dt = (static_cast<Ogre::Real>(Core::getSingletonPtr()->getOgreTimer()->getMilliseconds()) * 0.001f) - currentTime;
		Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[DotSceneImportModule] Parse end scene: " + filePathName + " duration: " + Ogre::StringConverter::toString(dt) + " seconds");

		// Ogre::Root::getSingletonPtr()->renderOneFrame();

		this->bSceneParsed = false;

		return true;
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
				AppStateManager::getSingletonPtr()->getGameObjectController()->deleteGameObject(gameObjectPtr);
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
					AppStateManager::getSingletonPtr()->getGameObjectController()->deleteGameObject(gameObjectPtr);
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

		// if there is a caelum XML element in the world scene, do not create a sun light for Ogre::Terrain, but use that one
		// from caelum
		pElement = xmlRoot->first_node("caelum");
		if (pElement)
		{
			this->hasCaelumSystem = true;
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
			this->processOgreRecast(pElement);
		}

		// Process nodes (?)
		pElement = xmlRoot->first_node("nodes");
		if (nullptr != pElement)
		{
			this->processNodes(pElement, nullptr, justSetValues);
		}

		pElement = xmlRoot->first_node("caelum");
		if (pElement)
		{
			this->processCaelum(pElement);
		}

		pElement = xmlRoot->first_node("hydrax");
		if (pElement)
		{
			this->processHydrax(pElement);
		}
#ifndef CAELUM_NOT_PORTED
		//// exchange data between caelum and hydrax if existing
		//if (this->hydraxModule && this->caelumModule)
		//{
		//	this->hydraxModule->setCaelumModule(this->caelumModule);
		//	this->sunLight = this->caelumModule->getCaelumSystem()->getSun()->getMainLight();
		//}
#endif


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
					Core::getSingletonPtr()->setUseV2Mesh(this->projectParameter.useV2Item);
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

	void DotSceneImportModule::acceptTerrainShadows(void)
	{
#ifndef TERRAIN_NOT_PORTED
//		// Ogre::TerrainMaterialGeneratorA::SM2Profile* matProfile =
//		// static_cast<Ogre::TerrainMaterialGeneratorA::SM2Profile*>(this->terrainGlobalOptions->getDefaultMaterialGenerator()->getActiveProfile());
//
//		//Ogre::TerrainRTSSMaterialGenerator::SM2Profile* matProfile =
//		// 	static_cast<Ogre::TerrainRTSSMaterialGenerator::SM2Profile*>(this->terrainGlobalOptions->getDefaultMaterialGenerator()->getActiveProfile());
//
//		Ogre::TerrainRTSSMaterialGenerator* matGen = new Ogre::TerrainRTSSMaterialGenerator();
//		Ogre::TerrainMaterialGeneratorPtr ptr = Ogre::TerrainMaterialGeneratorPtr();
//
//		// Ogre::TerrainMaterialGeneratorC *matGen = new Ogre::TerrainMaterialGeneratorC(this->camera);
//		// Ogre::TerrainMaterialGeneratorPtr ptr = Ogre::TerrainMaterialGeneratorPtr();
//		ptr.bind(matGen);
//
//
//
//		// Assume we get a shader model 2 material profile
//		Ogre::TerrainMaterialGeneratorA::SM2Profile* matProfile;
//		terrainGlobalOptions->setDefaultMaterialGenerator(ptr);
//		matProfile = static_cast<Ogre::TerrainRTSSMaterialGenerator::SM2Profile*>(matGen->getActiveProfile());
//
//		matProfile->setReceiveDynamicShadowsEnabled(true);
//#ifdef SHADOWS_IN_LOW_LOD_MATERIAL
//		matProfile->setReceiveDynamicShadowsLowLod(true);
//#else
//		matProfile->setReceiveDynamicShadowsLowLod(false);
//#endif
//		matProfile->setReceiveDynamicShadowsDepth(ShaderModule::getInstance()->isPSSMDepthShadowsEnabled());
//		matProfile->setReceiveDynamicShadowsPSSM(static_cast<Ogre::PSSMShadowCameraSetup*>(NOWA::ShaderModule::getInstance()->getShadowCameraSetupPtr().get()));
//
//		matProfile->setLightmapEnabled(false);  // No lightmapped shadows
//		matProfile->setReceiveDynamicShadowsLowLod(true);  // On or off, no difference in my test
//		terrainGlobalOptions->setCastsDynamicShadows(true);  // Terrain shadow casting for doing self-shadowing
//		//addTextureShadowDebugOverlay(TL_RIGHT, 3);
#endif
	}

	void DotSceneImportModule::processTerrainPage(rapidxml::xml_node<> *xmlNode)
	{
#ifndef TERRAIN_NOT_PORTED
		//Ogre::String name = XMLConverter::getAttrib(xmlNode, "name");
		////name = Page_00000000.ogt
		//long pageX = Ogre::StringConverter::parseInt(xmlNode->first_attribute("pageX")->value());
		//long pageY = Ogre::StringConverter::parseInt(xmlNode->first_attribute("pageY")->value());
		////die Page als Vektor speichern in umgekehrter Reihenfolge, z.B. x = 0, y = 1
		//this->pages.push_front(Ogre::Vector2(pageX, pageY));

		//// add the page geometry quality details to the paged geometry module
		//int pagedGeometryPageSize = Ogre::StringConverter::parseInt(xmlNode->first_attribute("pagedGeometryPageSize")->value());

		//// error checking
		//if (pagedGeometryPageSize < 10)
		//{
		//	Ogre::LogManager::getSingleton().logMessage("[DotSceneImportModule] pagedGeometryPageSize value error!", Ogre::LML_CRITICAL);
		//	pagedGeometryPageSize = 10;
		//}

		//int pagedGeometryDetailDistance = Ogre::StringConverter::parseInt(xmlNode->first_attribute("pagedGeometryDetailDistance")->value());
		//if (pagedGeometryDetailDistance < 100)
		//{
		//	Ogre::LogManager::getSingleton().logMessage("[DotSceneImportModule] pagedGeometryDetailDistance value error!", Ogre::LML_CRITICAL);
		//	pagedGeometryDetailDistance = 100;
		//}
#ifndef PAGEDGEOMETRY_NOT_PORTED
		/*if (!this->pagedGeometryModule)
		{
			this->pagedGeometryModule = new PagedGeometryModule();
		}
		else
		{
			delete this->pagedGeometryModule;
			this->pagedGeometryModule = nullptr;
			this->pagedGeometryModule = new PagedGeometryModule();
		}

		this->pagedGeometryModule->setPagedGeometryPageSize(pagedGeometryPageSize);
		this->pagedGeometryModule->setPagedGeometryDetailDistance(pagedGeometryDetailDistance);*/
#endif
		////TestWorld\Terrain
		//if (Ogre::ResourceGroupManager::getSingleton().resourceExists(this->terrainGroup->getResourceGroup(), name))
		//{
		//	/*Ogre::ResourceGroupManager::LocationList locationList = Ogre::ResourceGroupManager::getSingleton().getResourceLocationList(this->terrainGroup->getResourceGroup());
		//	Ogre::ResourceGroupManager::LocationList::iterator it;
		//	for (it = locationList.begin(); it != locationList.end(); ++it)
		//	{
		//	Ogre::LogManager::getSingletonPtr()->logMessage("Location: " + (*it)->archive->getName());
		//	}*/
		//	char collisionName[256];
		//	strcpy(collisionName, this->strSceneName.c_str());
		//	//"TestWorld.scene"
		//	strtok(collisionName, ".");
		//	//"TestWorld"
		//	strcat(collisionName, "1.col");
		//	if (Ogre::ResourceGroupManager::getSingleton().resourceExists(this->resourceGroupName, Ogre::String(collisionName)))
		//	{
		//		time_t timePage = Ogre::ResourceGroupManager::getSingleton().resourceModifiedTime(this->terrainGroup->getResourceGroup(), name);
		//		struct tm *pSTimePage;
		//		pSTimePage = localtime(&timePage);

		//		//Hier muss alles in zwischenVariablen festgehalten werden, wegen einen nicht bekannten Fehler
		//		//da sonst die Uhrzeit bei den verschiedenen Dateien immer gleich waere!!!
		//		int pageYear = pSTimePage->tm_year;
		//		int pageMonth = pSTimePage->tm_mon;
		//		int pageDay = pSTimePage->tm_mday;
		//		int pageHour = pSTimePage->tm_hour;
		//		int pageMin = pSTimePage->tm_min;
		//		//Es langt sich die vergangenen Minuten abzuspeichern
		//		long minutesFromTimePage = ((365 * 24 * 60) * pageYear) + ((31 * 24 * 60) * pageMonth) + ((24 * 60) * pageDay) + ((60) * pageHour) + pageMin;

		//		//Ogre::LogManager::getSingletonPtr()->logMessage("m" + Ogre::StringConverter::toString(minutesFromTimePage)
		//		//	+ "d" + Ogre::StringConverter::toString(pageDay) + "h" + Ogre::StringConverter::toString(pageHour) + "min" + Ogre::StringConverter::toString(pageMin));
		//		time_t timeColPage = Ogre::ResourceGroupManager::getSingleton().resourceModifiedTime(this->resourceGroupName, Ogre::String(collisionName));
		//		struct tm *pSTimeColPage;
		//		pSTimeColPage = localtime(&timeColPage);
		//		int pageColYear = pSTimeColPage->tm_year;
		//		int pageColMonth = pSTimeColPage->tm_mon;
		//		int pageColDay = pSTimeColPage->tm_mday;
		//		int pageColHour = pSTimeColPage->tm_hour;
		//		int pageColMin = pSTimeColPage->tm_min;
		//		//Es lang sich die vergangenen Minuten abzuspeichern
		//		long minutesFromTimeColPage = ((365 * 24 * 60) * pageColYear) + ((31 * 24 * 60) * pageColMonth) + ((24 * 60) * pageColDay) + ((60) * pageColHour) + pageColMin;
		//		//Ogre::LogManager::getSingletonPtr()->logMessage("TimeCol " + Ogre::StringConverter::toString(minutesFromTimeColPage));


		//		//Wenn die Minuten seit der letzten Aenderung der Page groesser sind als die Minuten seit der letzten Aenderung der Kollisionskarte
		//		//Dann wurde offensichtlich das Terrain im Leveleditor geaendert und somit muss die Kollisionskarte neu berechnet werden
		//		if (minutesFromTimePage > minutesFromTimeColPage)
		//			this->needCollisionRebuild = true;
		//	}

		//	this->terrainGroup->defineTerrain(pageX, pageY, name);

		//	// grass layers
		//	rapidxml::xml_node<>* pElement = xmlNode->first_node("grassLayers");

		//	if (pElement)
		//	{
		//		this->processGrassLayers(pElement);
		//	}

		//	this->pagesCount++;
		//}
		//else
		//{
		//	Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Warning, The resource for the terrain page does not exist! Terrain can not be generated. Did you forget to specify the terrain folder in resources.cfg file? Example: FileSystem=../../media/Worlds/World/Terrain");
		//}
#endif
	}

	void DotSceneImportModule::processGrassLayers(rapidxml::xml_node<>* xmlNode)
	{
#ifndef PAGEDGEOMETRY_NOT_PORTED
		//Ogre::String mapName = XMLConverter::getAttrib(xmlNode, "densityMap");

		//if (!Ogre::ResourceGroupManager::getSingleton().resourceExists(this->terrainGroup->getResourceGroup(), mapName))
		//{
		//	throw Ogre::Exception(Ogre::Exception::ERR_FILE_NOT_FOUND, "[DotSceneImportModule] Can not the file: " + mapName, "NOWA");
		//}

		//this->terrainGlobalOptions->setVisibilityFlags(Ogre::StringConverter::parseUnsignedInt(xmlNode->first_attribute("visibilityFlags")->value()));

		//// create a temporary camera
		//Ogre::Camera* tempCamera = this->sceneManager->createCamera("ThIsNamEShoUlDnOtExisT");

		//this->pagedGeometryModule->createPagedGeometryForGrass(tempCamera);

		//// set the terrain group pointer
		//staticGroupPtr = this->terrainGroup;

		//// Supply a height function to GrassLoader so it can calculate grass Y values
		//this->pagedGeometryModule->getGrassLoader()->setHeightFunction(worldTerrainGroupHeightFunction);

		//// create the layers and load the options for them
		//rapidxml::xml_node<>* pElement = xmlNode->first_node("grassLayer");
		//
		//while (pElement)
		//{
		//	Forests::GrassLayer* grassLayer;
		//	rapidxml::xml_node<>* pSubElement;
		//	// grassLayer
		//	grassLayer = this->pagedGeometryModule->getGrassLoader()->addLayer(pElement->first_attribute("material")->value());
		//	// grassLayer->setId(Ogre::StringConverter::parseInt(pElement->first_attribute("id")->value()));
		//	// grassLayer->setEnabled(Ogre::StringConverter::parseBool(pElement->first_attribute("enabled")->value()));
		//	grassLayer->setMaxSlope(Ogre::StringConverter::parseReal(pElement->first_attribute("maxSlope")->value()));
		//	grassLayer->setLightingEnabled(Ogre::StringConverter::parseBool(pElement->first_attribute("lighting")->value()));

		//	// densityMapProps
		//	pSubElement = pElement->first_node("densityMapProps");

		//	Forests::MapChannel mapChannel = this->pagedGeometryModule->getMapChannel(pSubElement->first_attribute("channel")->value());

		//	grassLayer->setDensityMap(mapName, mapChannel);
		//	grassLayer->setDensity(Ogre::StringConverter::parseReal(pSubElement->first_attribute("density")->value()));

		//	// mapBounds
		//	pSubElement = pElement->first_node("mapBounds");
		//	grassLayer->setMapBounds(Forests::TBounds(
		//		Ogre::StringConverter::parseReal(pSubElement->first_attribute("left")->value()),  // left
		//		Ogre::StringConverter::parseReal(pSubElement->first_attribute("top")->value()),   // top
		//		Ogre::StringConverter::parseReal(pSubElement->first_attribute("right")->value()), // right
		//		Ogre::StringConverter::parseReal(pSubElement->first_attribute("bottom")->value()) // bottom
		//		));

		//	// grassSizes
		//	pSubElement = pElement->first_node("grassSizes");
		//	grassLayer->setMinimumSize(Ogre::StringConverter::parseReal(pSubElement->first_attribute("minWidth")->value()),   // width
		//		Ogre::StringConverter::parseReal(pSubElement->first_attribute("minHeight")->value()));// height
		//	grassLayer->setMaximumSize(Ogre::StringConverter::parseReal(pSubElement->first_attribute("maxWidth")->value()),   // width
		//		Ogre::StringConverter::parseReal(pSubElement->first_attribute("maxHeight")->value()));// height

		//	// techniques
		//	pSubElement = pElement->first_node("techniques");

		//	Forests::GrassTechnique renderTechnique = this->pagedGeometryModule->getRenderTechnique(pSubElement->first_attribute("renderTechnique")->value());
		//	grassLayer->setRenderTechnique(renderTechnique, Ogre::StringConverter::parseBool(pSubElement->first_attribute("blend")->value()));


		//	Forests::FadeTechnique fadeTechnique = this->pagedGeometryModule->getFadeTechnique(pSubElement->first_attribute("fadeTechnique")->value());
		//	grassLayer->setFadeTechnique(fadeTechnique);

		//	// animation
		//	pSubElement = pElement->first_node("animation");
		//	grassLayer->setAnimationEnabled(Ogre::StringConverter::parseBool(pSubElement->first_attribute("animate")->value()));
		//	grassLayer->setSwayLength(Ogre::StringConverter::parseReal(pSubElement->first_attribute("swayLength")->value()));
		//	grassLayer->setSwaySpeed(Ogre::StringConverter::parseReal(pSubElement->first_attribute("swaySpeed")->value()));
		//	grassLayer->setSwayDistribution(Ogre::StringConverter::parseReal(pSubElement->first_attribute("swayDistribution")->value()));

		//	// next layer
		//	pElement = pElement->next_sibling("grassLayer");
		//}

		//this->sceneManager->destroyCamera(tempCamera);
#endif
	}

	void DotSceneImportModule::processCaelum(rapidxml::xml_node<>* xmlNode)
	{
#ifndef CAELUM_NOT_PORTED
		//if (!this->caelumModule)
		//{
		//	this->caelumModule = new CaelumModule(this->sceneManager, this->camera);
		//}
		//else
		//{
		//	delete this->caelumModule;
		//	this->caelumModule = nullptr;
		//	this->caelumModule = new CaelumModule(this->sceneManager, this->camera);
		//}

		//// process sun
		//{
		//	rapidxml::xml_node<>* element = xmlNode->first_node("sun");

		//	bool enable = XMLConverter::getAttribBool(element, "enable");
		//	bool autoDisable = XMLConverter::getAttribBool(element, "autoDisable");
		//	Ogre::Vector3 position = XMLConverter::getAttribVector3(element, "position");
		//	Ogre::Vector4 color = XMLConverter::getAttribVector4(element, "color");
		//	Ogre::Vector4 lightColor = XMLConverter::getAttribVector4(element, "lightColour");
		//	Ogre::Vector4 ambientMultiplier = XMLConverter::getAttribVector4(element, "ambientMultiplier");
		//	Ogre::Vector4 diffuseMultiplier = XMLConverter::getAttribVector4(element, "diffuseMultiplier");
		//	Ogre::Vector4 specularMultiplier = XMLConverter::getAttribVector4(element, "ambientMultiplier");
		//	bool castShadow = XMLConverter::getAttribBool(element, "castShadow");

		//	this->caelumModule->setSunEnabled(enable);
		//	this->caelumModule->setSunAutoDisabled(autoDisable);
		//	this->caelumModule->setSunPosition(position);
		//	this->caelumModule->setSunColour(Ogre::ColourValue(color.x, color.y, color.z, color.w));
		//	this->caelumModule->setSunLightColour(Ogre::ColourValue(lightColour.x, lightColour.y, lightColour.z, lightColour.w));
		//	this->caelumModule->setSunAmbientMultiplier(Ogre::ColourValue(ambientMultiplier.x, ambientMultiplier.y,
		//		ambientMultiplier.z, ambientMultiplier.w));
		//	this->caelumModule->setSunDiffuseMultiplier(Ogre::ColourValue(diffuseMultiplier.x, diffuseMultiplier.y,
		//		diffuseMultiplier.z, diffuseMultiplier.w));
		//	this->caelumModule->setSunSpecularMultiplier(Ogre::ColourValue(specularMultiplier.x, specularMultiplier.y,
		//		specularMultiplier.z, specularMultiplier.w));
		//	this->caelumModule->setSunCastShadow(castShadow);

		//	{
		//		// process attenuation
		//		rapidxml::xml_node<>* subElement = element->first_node("attenuation");

		//		Ogre::Real distance = XMLConverter::getAttribReal(subElement, "distance");
		//		Ogre::Real constantMultiplier = XMLConverter::getAttribReal(subElement, "constantMultiplier");
		//		Ogre::Real linearMultiplier = XMLConverter::getAttribReal(subElement, "linearMultiplier");
		//		Ogre::Real quadricMultiplier = XMLConverter::getAttribReal(subElement, "quadricMultiplier");

		//		this->caelumModule->setSunAttenuationDistance(distance);
		//		this->caelumModule->setSunAttenuationConstantMultiplier(constantMultiplier);
		//		this->caelumModule->setSunAttenuationLinearMultiplier(linearMultiplier);
		//		this->caelumModule->setSunAttenuationQuadricMultiplier(quadricMultiplier);
		//	}
		//}

		//// process moon
		//{
		//	rapidxml::xml_node<>* element = xmlNode->first_node("moon");

		//	bool enable = XMLConverter::getAttribBool(element, "enable");
		//	bool autoDisable = XMLConverter::getAttribBool(element, "autoDisable");
		//	Ogre::Vector4 ambientMultiplier = XMLConverter::getAttribVector4(element, "ambientMultiplier");
		//	Ogre::Vector4 diffuseMultiplier = XMLConverter::getAttribVector4(element, "diffuseMultiplier");
		//	Ogre::Vector4 specularMultiplier = XMLConverter::getAttribVector4(element, "ambientMultiplier");
		//	bool castShadow = XMLConverter::getAttribBool(element, "castShadow");

		//	this->caelumModule->setMoonEnabled(enable);
		//	this->caelumModule->setMoonAutoDisabled(autoDisable);
		//	this->caelumModule->setMoonAmbientMultiplier(Ogre::ColourValue(ambientMultiplier.x, ambientMultiplier.y,
		//		ambientMultiplier.z, ambientMultiplier.w));
		//	this->caelumModule->setMoonDiffuseMultiplier(Ogre::ColourValue(diffuseMultiplier.x, diffuseMultiplier.y,
		//		diffuseMultiplier.z, diffuseMultiplier.w));
		//	this->caelumModule->setMoonSpecularMultiplier(Ogre::ColourValue(specularMultiplier.x, specularMultiplier.y,
		//		specularMultiplier.z, specularMultiplier.w));
		//	this->caelumModule->setMoonCastShadow(castShadow);

		//	{
		//		// process attenuation
		//		rapidxml::xml_node<>* subElement = element->first_node("attenuation");

		//		Ogre::Real distance = XMLConverter::getAttribReal(subElement, "distance");
		//		Ogre::Real constantMultiplier = XMLConverter::getAttribReal(subElement, "constantMultiplier");
		//		Ogre::Real linearMultiplier = XMLConverter::getAttribReal(subElement, "linearMultiplier");
		//		Ogre::Real quadricMultiplier = XMLConverter::getAttribReal(subElement, "quadricMultiplier");

		//		this->caelumModule->setMoonAttenuationDistance(distance);
		//		this->caelumModule->setMoonAttenuationConstantMultiplier(constantMultiplier);
		//		this->caelumModule->setMoonAttenuationLinearMultiplier(linearMultiplier);
		//		this->caelumModule->setMoonAttenuationQuadricMultiplier(quadricMultiplier);
		//	}
		//}

		//// process clock
		//{
		//	rapidxml::xml_node<>* element = xmlNode->first_node("clock");

		//	int year = static_cast<int>(XMLConverter::getAttribReal(element, "year"));
		//	int month = static_cast<int>(XMLConverter::getAttribReal(element, "month"));
		//	int day = static_cast<int>(XMLConverter::getAttribReal(element, "day"));
		//	int hour = static_cast<int>(XMLConverter::getAttribReal(element, "hour"));
		//	int minute = static_cast<int>(XMLConverter::getAttribReal(element, "minute"));
		//	int second = static_cast<int>(XMLConverter::getAttribReal(element, "second"));
		//	Ogre::Real speed = XMLConverter::getAttribReal(element, "speed");

		//	this->caelumModule->setClockYear(year);
		//	this->caelumModule->setClockMonth(month);
		//	this->caelumModule->setClockDay(day);
		//	this->caelumModule->setClockHour(hour);
		//	this->caelumModule->setClockMinute(minute);
		//	this->caelumModule->setClockSecond(second);
		//	this->caelumModule->setClockSpeed(speed);
		//}

		//// process observer
		//{
		//	rapidxml::xml_node<>* element = xmlNode->first_node("observer");

		//	int longitude = static_cast<int>(XMLConverter::getAttribReal(element, "longitude"));
		//	int latitude = static_cast<int>(XMLConverter::getAttribReal(element, "latitude"));

		//	this->caelumModule->setLongitude(longitude);
		//	this->caelumModule->setLatitude(latitude);
		//}

		//// process lighting
		//{
		//	rapidxml::xml_node<>* element = xmlNode->first_node("lighting");

		//	bool singleLightsource = XMLConverter::getAttribBool(element, "singleLightsource");
		//	bool singleShadowsource = XMLConverter::getAttribBool(element, "singleShadowsource");
		//	bool manageAmbientLight = XMLConverter::getAttribBool(element, "manageAmbientLight");
		//	Ogre::Vector4 minimumAmbientLight = XMLConverter::getAttribVector4(element, "minimumAmbientLight");

		//	this->caelumModule->setLightningSingleLightsource(singleLightsource);
		//	this->caelumModule->setLightningSingleShadowsource(singleShadowsource);
		//	this->caelumModule->setLightningManageAmbientLight(manageAmbientLight);
		//	this->caelumModule->setLightningMinimumAmbientLight(Ogre::ColourValue(minimumAmbientLight.x, minimumAmbientLight.y,
		//		minimumAmbientLight.z, minimumAmbientLight.w));

		//}

		//// process fog
		//{
		//	rapidxml::xml_node<>* element = xmlNode->first_node("fog");

		//	bool manage = XMLConverter::getAttribBool(element, "manage");
		//	Ogre::Real densityMultiplier = XMLConverter::getAttribReal(element, "densityMultiplier");

		//	this->caelumModule->setFogManaged(manage);
		//	this->caelumModule->setFogDensityMultiplier(densityMultiplier);
		//}

		//// process stars
		//{
		//	rapidxml::xml_node<>* element = xmlNode->first_node("stars");

		//	bool enable = XMLConverter::getAttribBool(element, "enable");
		//	Ogre::Real magnitudeScale = XMLConverter::getAttribReal(element, "magnitudeScale");
		//	int mag0PixelSize = static_cast<int>(XMLConverter::getAttribReal(element, "mag0PixelSize"));
		//	int minPixelSize = static_cast<int>(XMLConverter::getAttribReal(element, "minPixelSize"));
		//	int maxPixelSize = static_cast<int>(XMLConverter::getAttribReal(element, "maxPixelSize"));

		//	this->caelumModule->setStarsEnabled(enable);
		//	this->caelumModule->setStarsMagnitudeScale(magnitudeScale);
		//	this->caelumModule->setStarsMag0PixelSize(mag0PixelSize);
		//	this->caelumModule->setStarsMinPixelSize(minPixelSize);
		//	this->caelumModule->setStarsMaxPixelSize(maxPixelSize);
		//}

		//// process clouds
		//{
		//	rapidxml::xml_node<>* element = xmlNode->first_node("clouds");
		//	rapidxml::xml_node<>* subElement = element->first_node("layer");

		//	short i = 0;

		//	while (subElement)
		//	{
		//		bool enable = XMLConverter::getAttribBool(subElement, "enable");
		//		Ogre::Real coverage = XMLConverter::getAttribReal(subElement, "coverage");
		//		int height = static_cast<int>(XMLConverter::getAttribReal(subElement, "height"));
		//		Ogre::Vector2 speed = XMLConverter::getAttribVector2(subElement, "speed");

		//		CaelumModule::CloudsLayer cloudsLayer;
		//		cloudsLayer.enable = enable;
		//		cloudsLayer.coverage = coverage;
		//		cloudsLayer.height = height;
		//		cloudsLayer.speed = speed;
		//		this->caelumModule->setCloudsLayer(cloudsLayer, i);

		//		subElement = subElement->next_sibling("layer");
		//		++i;
		//	}
		//}
		//// create the caelum sky with all params
		//this->caelumModule->createCaelum();
		//// this->setSunLightForOgreTerrain(this->sunLight);
#endif
	}

	void DotSceneImportModule::processHydrax(rapidxml::xml_node<>* xmlNode)
	{
#ifndef HYDRAX_NOT_PORTED
		//if (nullptr == this->camera)
		//{
		//	Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[DotSceneImportModule] Error: Hydrax cannot be created, because it requires a valid camera pointer!");
		//	throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[DotSceneImportModule] Error: Hydrax cannot be created, because it requires a valid camera pointer!\n", "NOWA");
		//}
		//// rapidxml::xml_node<>* element = xmlNode->first_node("sun");

		//Ogre::String configFile = XMLConverter::getAttrib(xmlNode, "configFile");
		//bool caelumIntegration = XMLConverter::getAttribBool(xmlNode, "caelumIntegration");

		//Ogre::String configFilePathname = this->worldPath + "/" + configFile;

		//if (0 < configFile.length())
		//{
		//	if (!this->hydraxModule)
		//	{
		//		this->hydraxModule = new HydraxModule();
		//	}
		//	else
		//	{
		//		delete this->hydraxModule;
		//		this->hydraxModule = new HydraxModule();
		//	}

		//	this->hydraxModule->setConfigFile(configFilePathname);
		//}

		//// parse the user section, to check if another grid type should be created
		//Ogre::String name = XMLConverter::getAttrib(xmlNode, "name");
		//rapidxml::xml_node<>* element = xmlNode->first_node("userData");
		//if (element)
		//{
		//	rapidxml::xml_node<>* propertyElement = element->first_node("property");
		//	if (propertyElement)
		//	{
		//		if (Ogre::StringUtil::match(XMLConverter::getAttrib(propertyElement, "name"), "Module*", true))
		//		{
		//			Ogre::String moduleName = XMLConverter::getAttrib(propertyElement, "data");
		//			if ("Hydrax" == moduleName)
		//			{
		//				// OgreRecastConfigParams params;

		//				propertyElement = propertyElement->next_sibling("property");
		//				while (propertyElement)
		//				{
		//					Ogre::String name = XMLConverter::getAttrib(propertyElement, "name");
		//					
		//					if ("GridType" == name)
		//					{
		//						Ogre::String data = XMLConverter::getAttrib(propertyElement, "data", "ProjectedGrid");
		//						this->hydraxModule->setGridType(data);
		//					}
		//					else if ("GridSize" == name)
		//					{
		//						Ogre::Vector2 data = XMLConverter::getAttribVector2(propertyElement, "data");
		//						this->hydraxModule->setGridSize(std::move(data));
		//					}
		//					propertyElement = propertyElement->next_sibling("property");
		//				}
		//			}
		//		}
		//	}
		//}
		//if (this->sunLight)
		//{
		//	this->hydraxModule->setSunLight(this->sunLight);
		//}
		//this->hydraxModule->createHydrax(this->sceneManager, this->camera, Core::getSingletonPtr()->getOgreViewport());
#endif
	}

	void DotSceneImportModule::processPagedGeometry(rapidxml::xml_node<>* xmlNode, Ogre::SceneNode* parent)
	{
#ifndef PAGEDGEOMETRY_NOT_PORTED
		//// why here static filename
		//Ogre::String filename = "../Projects/SampleScene3/" + XMLConverter::getAttrib(xmlNode, "fileName");
		//Ogre::String model = XMLConverter::getAttrib(xmlNode, "model");
		//Ogre::Real pageSize = XMLConverter::getAttribReal(xmlNode, "pageSize");
		//Ogre::Real batchDistance = XMLConverter::getAttribReal(xmlNode, "batchDistance");
		//Ogre::Real impostorDistance = XMLConverter::getAttribReal(xmlNode, "impostorDistance");
		//Ogre::Vector4 bounds = Ogre::StringConverter::parseVector4(XMLConverter::getAttrib(xmlNode, "bounds"));

		//// is this correct with scenemanager camera?? or get it loader from the simulation state scene?
		//this->pagedGeometryModule->createPagedGeometryForTrees(this->sceneManager->getCameraIterator().begin()->second, pageSize, batchDistance, impostorDistance,
		//	bounds, filename, this->sceneManager, model);
#endif
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
				if (false == gameObjectPtr->postInit())
				{
					AppStateManager::getSingletonPtr()->getGameObjectController()->deleteGameObject(gameObjectPtr);
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
		if (name.empty())
		{
			if (parent)
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
			if (parent)
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
			// if ("MainCamera" != name)
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

		// Process paged geometry trees (*)
		pElement = xmlNode->first_node("pagedgeometry");
		while (pElement)
		{
			processPagedGeometry(pElement, pNode);
			pElement = pElement->next_sibling("pagedgeometry");
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

		float currentTime = static_cast<Ogre::Real>(Core::getSingletonPtr()->getOgreTimer()->getMilliseconds()) * 0.001f;
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DotSceneImportModule] Parse item: " + name + " mesh: " + meshFile);

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
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DotSceneImportModule] Parse end item: " + name + " mesh: " + meshFile + " duration: " + Ogre::StringConverter::toString(dt) + " seconds.");
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
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DotSceneImportModule] Parse entity: " + name + " mesh: " + meshFile);

		// later maybe for complex objects
		// http://www.ogre3d.org/tikiwiki/tiki-index.php?page=PixelCountLodStrategy&structure=Cookbook
		Ogre::v1::Entity* entity = nullptr;
		Ogre::Item* item = nullptr;
		Ogre::v1::MeshPtr v1Mesh;
		Ogre::MeshPtr v2Mesh;
		GameObject::eType type = GameObject::ENTITY;
		// Attention with hbu_static, there is also dynamic

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
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[DotSceneImportModule] Parse end entity: " + name + " mesh: " + meshFile + " duration: " + Ogre::StringConverter::toString(dt) + " seconds");
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

}; //Namespace end

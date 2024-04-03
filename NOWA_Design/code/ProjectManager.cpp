#include "NOWAPrecompiled.h"
#include "ProjectManager.h"
#include "GuiEvents.h"
#include "modules/WorkspaceModule.h"

#include "OpenSaveFileDialog/DialogManager.cpp"
#include "OpenSaveFileDialog/Dialog.cpp"
#include "OpenSaveFileDialog/OpenSaveFileDialog.cpp"

#include "RecentFilesManager.h"

ProjectManager::ProjectManager(Ogre::SceneManager* sceneManager)
	: sceneManager(sceneManager),
	sunLight(nullptr),
	dotSceneImportModule(nullptr),
	dotSceneExportModule(nullptr),
	ogreNewt(nullptr),
	openSaveFileDialog(nullptr),
	editorManager(nullptr)
{
	new tools::DialogManager();
	tools::DialogManager::getInstance().initialise();

	this->openSaveFileDialog = new OpenSaveFileDialogExtended();
	this->openSaveFileDialog->setFileMask("*.scene");
	this->openSaveFileDialog->eventEndDialog = MyGUI::newDelegate(this, &ProjectManager::notifyEndDialog);

	// this->openSaveFileDialog->eventEndDialog.connect(this, &ProjectControl::notifyEndDialogOpenSaveFile);
	// // this->openSaveFileDialog->setCurrentFolder(RecentFilesManager::getInstance().getRecentFolder());
	// this->openSaveFileDialog->setRecentFolders(RecentFilesManager::getInstance().getRecentFolders());
}

ProjectManager::~ProjectManager()
{
	if (this->dotSceneImportModule)
	{
		// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProjectManager]: Destroying World");
		delete this->dotSceneImportModule;
		this->dotSceneImportModule = nullptr;
	}
	if (this->dotSceneExportModule)
	{
		delete this->dotSceneExportModule;
		this->dotSceneExportModule = nullptr;
	}

	if (this->openSaveFileDialog)
	{
		delete this->openSaveFileDialog;
		this->openSaveFileDialog = nullptr;
	}

	tools::DialogManager::getInstance().shutdown();
	delete tools::DialogManager::getInstancePtr();

	this->editorManager = nullptr;
}

void ProjectManager::setEditorManager(NOWA::EditorManager* editorManager)
{
	this->editorManager = editorManager;
}

NOWA::EditorManager* ProjectManager::getEditorManager(void) const
{
	return this->editorManager;
}

Ogre::Light* ProjectManager::createSunLight(void)
{
	Ogre::Light* sunLight = nullptr;
	// There must be a main light in order to prevent render errors, so SunLight, is the main mandantory light! Which may not be deleted
	Ogre::SceneNode* lightNode = this->sceneManager->getRootSceneNode()->createChildSceneNode(Ogre::SCENE_DYNAMIC);
	lightNode->setPosition(0, 37.2738f, 50.0f);

	Ogre::v1::Entity* newEntity = this->sceneManager->createEntity("LightDirectional.mesh");
	lightNode->attachObject(newEntity);

	Ogre::String gameObjectName = "SunLight";
	newEntity->setName(gameObjectName);
	lightNode->setName(gameObjectName);

	NOWA::GameObjectPtr gameObjectPtr = NOWA::GameObjectFactory::getInstance()->createGameObject(this->sceneManager, lightNode, newEntity, NOWA::GameObject::LIGHT_DIRECTIONAL, NOWA::GameObjectController::MAIN_LIGHT_ID);
	if (nullptr != gameObjectPtr)
	{
		// Do not permit to change the name of the sun light
		gameObjectPtr->getAttribute(NOWA::GameObject::AttrName())->setReadOnly(true);
		// Add also the light direcitional component
		NOWA::LightDirectionalCompPtr lightComponentPtr = boost::dynamic_pointer_cast<NOWA::LightDirectionalComponent>(
			NOWA::GameObjectFactory::getInstance()->createComponent(gameObjectPtr, NOWA::LightDirectionalComponent::getStaticClassName()));

		lightComponentPtr->setPowerScale(Ogre::Math::PI);
		lightComponentPtr->setDirection(Ogre::Vector3(-1, -1, -1).normalisedCopy());
		lightComponentPtr->setSpecularColor(Ogre::Vector3(0.8f, 0.8f, 0.8f));
		sunLight = lightComponentPtr->getOgreLight();
		sunLight->setName("SunLight");

		// Add also hdr effect compponent by default
		// NOWA::GameObjectFactory::getInstance()->createComponent(gameObjectPtr, NOWA::HdrEffectComponent::getStaticClassName());

		// Register after the component has been created
		NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->registerGameObject(gameObjectPtr);
	}
	return sunLight;
}

Ogre::Camera* ProjectManager::createMainCamera(void)
{
	Ogre::Camera* mainCamera = nullptr;
	// There must be a main camera which may not be deleted
	Ogre::SceneNode* cameraNode = this->sceneManager->getRootSceneNode()->createChildSceneNode(Ogre::SCENE_DYNAMIC);

	Ogre::v1::Entity* newEntity = this->sceneManager->createEntity("Camera.mesh");
	cameraNode->attachObject(newEntity);

	Ogre::String gameObjectName = "MainCamera";
	newEntity->setName(gameObjectName);
	cameraNode->setName(gameObjectName);

	NOWA::GameObjectPtr gameObjectPtr = NOWA::GameObjectFactory::getInstance()->createGameObject(this->sceneManager, cameraNode, newEntity, NOWA::GameObject::CAMERA, NOWA::GameObjectController::MAIN_CAMERA_ID);
	if (nullptr != gameObjectPtr)
	{
		// Do not permit to change the name of the sun light
		gameObjectPtr->getAttribute(NOWA::GameObject::AttrName())->setReadOnly(true);
		// Add also the light direcitional component
		NOWA::CameraCompPtr cameraComponentPtr = boost::dynamic_pointer_cast<NOWA::CameraComponent>(
			NOWA::GameObjectFactory::getInstance()->createComponent(gameObjectPtr, NOWA::CameraComponent::getStaticClassName()));

		mainCamera = cameraComponentPtr->getCamera();
		mainCamera->setName("MainCamera");
		// Set camera a bit away from zero so that new added game objects can be seen
		cameraComponentPtr->setCameraPosition(Ogre::Vector3(0.0f, 5.0f, -2.0f));

		NOWA::GameObjectFactory::getInstance()->createComponent(gameObjectPtr, NOWA::WorkspacePbsComponent::getStaticClassName());
		cameraComponentPtr->setActivated(true);
		
		// Register after the component has been created
		NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->registerGameObject(gameObjectPtr);

		// Sent when a name has changed, so that the resources panel can be refreshed with new values
		boost::shared_ptr<EventDataRefreshResourcesPanel> eventDataRefreshResourcesPanel(new EventDataRefreshResourcesPanel());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshResourcesPanel);
	}
	return mainCamera;
}

void ProjectManager::createMainGameObject(void)
{
	Ogre::SceneNode* mainGameObjectNode = this->sceneManager->getRootSceneNode()->createChildSceneNode(Ogre::SCENE_DYNAMIC);
	// Attention: Never set to huge coordinates like -1000 -1000 -1000, else shadows will be corrupted, because the game object would be to far away from all other game objects!
	mainGameObjectNode->setPosition(0, 0, 0);

	Ogre::v1::Entity* mainGameObjectEntity = this->sceneManager->createEntity("Node.mesh");
	// mainGameObjectEntity->setStatic(true);
	mainGameObjectNode->attachObject(mainGameObjectEntity);
	mainGameObjectNode->setVisible(false);

	Ogre::String gameObjectName = "MainGameObject";
	mainGameObjectEntity->setName(gameObjectName);
	mainGameObjectNode->setName(gameObjectName);

	NOWA::GameObjectPtr gameObjectPtr = NOWA::GameObjectFactory::getInstance()->createGameObject(this->sceneManager, mainGameObjectNode, mainGameObjectEntity, NOWA::GameObject::SCENE_NODE, NOWA::GameObjectController::MAIN_GAMEOBJECT_ID);
	if (nullptr != gameObjectPtr)
	{
		// Do not permit to change the name of the sun light
		gameObjectPtr->getAttribute(NOWA::GameObject::AttrName())->setReadOnly(true);

		// Register after the component has been created
		NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->registerGameObject(gameObjectPtr);
	}
}

void ProjectManager::createNewProject(const NOWA::ProjectParameter& projectParameter)
{
	this->destroyScene();

	this->additionalMeshResources.clear();

	// Create the physics and set data internally in internalApplySettings
	this->ogreNewt = NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->createPhysics(NOWA::AppStateManager::getSingletonPtr()->getCurrentAppStateName() + "_world");
	this->ogreNewt->cleanUp();

	this->internalApplySettings();
	this->projectParameter = projectParameter;

	NOWA::AppStateManager::getSingletonPtr()->getGpuParticlesModule()->init(this->sceneManager);
	
	NOWA::Core::getSingletonPtr()->setProjectName(this->projectParameter.projectName);

	// this->ogreNewt = NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->createQualityPhysics(Ogre::Vector3(-500.0f, -10.0f, -500.0f), Ogre::Vector3(500.0f, 200.0f, 500.0f));
	// this->ogreNewt = NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->createPerformantPhysics(Ogre::Vector3(-500.0f, -100.0f, -500.0f), Ogre::Vector3(500.0f, 100.0f, 500.0f), 60.0f);

	// Before creating a new scene, load the scene, so that a possible existing global.scene can be loaded beforehand
	if (nullptr == this->dotSceneImportModule)
	{
		this->dotSceneImportModule = new NOWA::DotSceneImportModule(this->sceneManager, nullptr, nullptr);
	}
	try
	{
		// Note: When creating a new scene, the .scene file does not exist yet, but maybe there is already a globa.scene file existing
		// This file will be parsed and below also again exported along with the new scene file.
		// Mandatory game objects are only created below, if they do not exist already via the parsed globa.scene
		// So now no new scene creation does kill a valid valueable global.scene anymore!
		this->dotSceneImportModule->parseScene(this->projectParameter.projectName, this->projectParameter.sceneName, "Projects", nullptr, nullptr, false);
	}
	catch (const std::runtime_error& e)
	{
		MyGUI::Message* messageBox = MyGUI::Message::createMessageBox("NOWA-Design", MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{FailedToLoadProject}"),
			MyGUI::MessageBoxStyle::IconError | MyGUI::MessageBoxStyle::Ok, "Popup", true);
		return;
	}

	// Check after (maybe loading a valid global.scene, which mandatory game objects already exists
	const auto& mainCameraGameObjectPtr = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromName("MainCamera");
	if (nullptr == mainCameraGameObjectPtr)
	{
		Ogre::Camera* camera = this->createMainCamera();
		if (nullptr == camera)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProjectManager] Could not new project because the mandantory camera could not be created.");
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[ProjectManager] Could not new project because the mandantory camera could not be created.\n", "NOWA");
		}
		// Add and activate the main camera
		NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->addCamera(camera, true);
	}
	else
	{
		// Add and activate the main camera
		auto cameraComponent = NOWA::makeStrongPtr(mainCameraGameObjectPtr->getComponent<NOWA::CameraComponent>());
		if (nullptr != cameraComponent)
		{
			NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->addCamera(cameraComponent->getCamera(), true);
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProjectManager] Could not new project because the mandantory camera could not be created. Details: The camera component is missing.");
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[ProjectManager] Could not new project because the mandantory camera could not be created. Details: The camera component is missing.\n", "NOWA");
		}
	}

	// No workspace created during scene loading, create a dummy one
	if (false == NOWA::WorkspaceModule::getInstance()->hasAnyWorkspace())
	{
		NOWA::WorkspaceModule::getInstance()->setPrimaryWorkspace(this->sceneManager, NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera(), nullptr);
	}

	if (nullptr == NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromName("SunLight"))
	{
		// Important: Light must be created, else a dxd3d error occurs in the app folder, that viewDir is invalid
		this->sunLight = this->createSunLight();
		if (nullptr == sunLight)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProjectManager] Could not new project because the mandantory sun light could not be created.");
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[ProjectManager] Could not new project because the mandantory sun light could not be created.\n", "NOWA");
		}
	}

	if (nullptr == NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromName("MainGameObject"))
	{
		// Add a main game object
		this->createMainGameObject();
	}

	this->dotSceneExportModule = new NOWA::DotSceneExportModule(this->sceneManager, /*this->camera, sunLight,*/ this->ogreNewt, this->projectParameter);
	this->dotSceneExportModule->exportScene(this->projectParameter.projectName, this->projectParameter.sceneName, "Projects");

	NOWA::DeployResourceModule::getInstance()->createLuaInitScript(this->projectParameter.projectName);

	boost::shared_ptr<EventDataProjectManipulation> eventDataProjectManipulation(new EventDataProjectManipulation(eProjectMode::NEW));
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataProjectManipulation);
}

void ProjectManager::saveProject(const Ogre::String& optionalFileName)
{
	if (false == optionalFileName.empty())
	{
		size_t found = optionalFileName.find_last_of("/\\");
		if (Ogre::String::npos == found)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "Could not save project because the project name or scene name is wrong! See: '" + optionalFileName + "'.");
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[ProjectManager] Could not save project because the project name or scene name is wrong! See: '" + optionalFileName + "'.\n", "NOWA");
		}
		this->projectParameter.projectName = optionalFileName.substr(0, found);
		this->projectParameter.sceneName = optionalFileName.substr(found + 1, optionalFileName.size() - 1);
	}

	// Must be recreated each time, so that dotSceneExportModule has correct ogre newt
	if (nullptr != this->dotSceneExportModule)
	{
		delete this->dotSceneExportModule;
		this->dotSceneExportModule = new NOWA::DotSceneExportModule(this->sceneManager, this->ogreNewt, this->projectParameter);
		this->dotSceneExportModule->setAdditionalMeshResources(this->additionalMeshResources);
	}

	// Creates prior a backup of the scene
	NOWA::DeployResourceModule::getInstance()->createProjectBackup(this->projectParameter.projectName, this->projectParameter.sceneName);
	this->dotSceneExportModule->exportScene(this->projectParameter.projectName, this->projectParameter.sceneName, "Projects");

	// Add file name to recent file names
	RecentFilesManager::getInstance().addRecentFile(this->projectParameter.projectName + "/" + this->projectParameter.sceneName + ".scene");

	boost::shared_ptr<EventDataProjectManipulation> eventDataProjectManipulation(new EventDataProjectManipulation(eProjectMode::SAVE));
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataProjectManipulation);

	boost::shared_ptr<EventDataSceneValid> eventDataSceneValid(new EventDataSceneValid(true));
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataSceneValid);
}

void ProjectManager::loadProject(const Ogre::String& filePathName)
{
	Ogre::String defaultPointer = MyGUI::PointerManager::getInstancePtr()->getDefaultPointer();
	MyGUI::PointerManager::getInstancePtr()->setPointer("link");
	this->destroyScene();

	NOWA::AppStateManager::getSingletonPtr()->getGpuParticlesModule()->init(this->sceneManager);

	this->additionalMeshResources.clear();

	Ogre::String tempFileName = filePathName;
	tempFileName = NOWA::Core::getSingletonPtr()->getFileNameFromPath(tempFileName);

	Ogre::String tempProjectName = NOWA::Core::getSingletonPtr()->getProjectNameFromPath(filePathName);

	this->projectParameter.projectName = tempProjectName;
	NOWA::Core::getSingletonPtr()->setProjectName(tempProjectName);

	this->projectParameter.sceneName = tempFileName;
	// Remove .scene
	size_t found = this->projectParameter.sceneName.find(".scene");
	if (found != std::wstring::npos)
	{
		this->projectParameter.sceneName = this->projectParameter.sceneName.substr(0, this->projectParameter.sceneName.size() - 6);
	}

	Ogre::String projectFilePathName = this->projectParameter.projectName + "/" + this->projectParameter.sceneName + ".scene";

	if (nullptr == this->dotSceneImportModule)
	{
		this->dotSceneImportModule = new NOWA::DotSceneImportModule(this->sceneManager, nullptr, nullptr);
	}
	try
	{
		// Also create workspace before any game object is created for reflection (last true parameter)
		bool success = this->dotSceneImportModule->parseScene(this->projectParameter.projectName, this->projectParameter.sceneName, "Projects", nullptr, nullptr, false);
		if (false == success)
		{
			RecentFilesManager::getInstance().removeRecentFile(0);

			// Trigger for main menu bar, that the recent list will be updated via this event
			boost::shared_ptr<EventDataSceneValid> eventDataSceneValid(new EventDataSceneValid(false));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataSceneValid);

			this->createDummyCamera();

			MyGUI::Message* messageBox = MyGUI::Message::createMessageBox("NOWA-Design",
				MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{FailedToLoadProject}: " + projectFilePathName),
				MyGUI::MessageBoxStyle::IconError | MyGUI::MessageBoxStyle::Ok, "Popup", true);

			return;
		}

		// If the scene does no more exist, but there is a global scene, which has no active camera, create dummy one
		if (nullptr == NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera())
		{
			this->createDummyCamera();
		}

		// No workspace created during scene loading, create a dummy one
		if (false == NOWA::WorkspaceModule::getInstance()->hasAnyWorkspace())
		{
			NOWA::WorkspaceModule::getInstance()->setPrimaryWorkspace(this->sceneManager, NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera(), nullptr);
		}

		// Internally, OgreNewt has maybe been parsed, so get it here
		this->ogreNewt = NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->getOgreNewt();
		if (nullptr != this->ogreNewt)
		{
			this->ogreNewt->cleanUp();
		}
	}
	catch (const std::runtime_error & e)
	{
		MyGUI::Message* messageBox = MyGUI::Message::createMessageBox("NOWA-Design", MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{FailedToLoadProject}"),
			MyGUI::MessageBoxStyle::IconError | MyGUI::MessageBoxStyle::Ok, "Popup", true);
		return;
	}

	if (nullptr == this->dotSceneExportModule)
	{
		this->dotSceneExportModule = new NOWA::DotSceneExportModule(this->sceneManager, this->ogreNewt, this->projectParameter);
	}

	this->projectParameter = this->dotSceneImportModule->getProjectParameter();

	// Set the data from loaded scene
	this->sunLight = this->dotSceneImportModule->getSunLight();

	// Add file name to recent file names
	RecentFilesManager::getInstance().addRecentFile(projectFilePathName);

	RecentFilesManager::getInstance().saveSettings();

	// No workspace creation, because its created in DotSceneImportModule
	// this->createWorkspace();

	boost::shared_ptr<EventDataProjectManipulation> eventDataProjectManipulation(new EventDataProjectManipulation(eProjectMode::LOAD));
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataProjectManipulation);

	MyGUI::PointerManager::getInstancePtr()->setPointer(defaultPointer);
}

void ProjectManager::applySettings(const NOWA::ProjectParameter& projectParameter)
{
	this->projectParameter = projectParameter;
	this->internalApplySettings();
}

void ProjectManager::internalApplySettings(void)
{
	if (nullptr != this->sunLight)
	{
		/*
		Tip: Set the upperHemisphere to a cold colour (e.g. blueish sky) and lowerHemisphere
            to a warm colour (e.g. sun-yellowish, orange) and the hemisphereDir in the opposite
            direction of your main directional light for a convincing look.
		*/
		this->sceneManager->setAmbientLight(Ogre::ColourValue(this->projectParameter.ambientLightUpperHemisphere.r, this->projectParameter.ambientLightUpperHemisphere.g, this->projectParameter.ambientLightUpperHemisphere.b),
			Ogre::ColourValue(this->projectParameter.ambientLightLowerHemisphere.r, this->projectParameter.ambientLightLowerHemisphere.g, this->projectParameter.ambientLightLowerHemisphere.b)/* * 0.065f * 0.75f*/, -this->sunLight->getDirection()/* + Ogre::Vector3::UNIT_Y * 0.2f*/, this->projectParameter.envmapScale);
	}
	else
	{
		this->sceneManager->setAmbientLight(Ogre::ColourValue(this->projectParameter.ambientLightUpperHemisphere.r, this->projectParameter.ambientLightUpperHemisphere.g, this->projectParameter.ambientLightUpperHemisphere.b),
			Ogre::ColourValue(this->projectParameter.ambientLightLowerHemisphere.r, this->projectParameter.ambientLightLowerHemisphere.g, this->projectParameter.ambientLightLowerHemisphere.b)/* * 0.065f * 0.75f*/, this->projectParameter.hemisphereDir.normalisedCopy(), this->projectParameter.envmapScale);
	}

	//Set sane defaults for proper shadow mapping
	this->sceneManager->setShadowFarDistance(this->projectParameter.shadowFarDistance);
	this->sceneManager->setShadowDirectionalLightExtrusionDistance(this->projectParameter.shadowDirectionalLightExtrusionDistance);
	this->sceneManager->setShadowDirLightTextureOffset(this->projectParameter.shadowDirLightTextureOffset);

	// this->sceneManager->setShadowTextureFadeStart(0.8f);
	// this->sceneManager->setShadowTextureFadeEnd(0.9f);

	// Just a test: https://forums.ogre3d.org/viewtopic.php?t=96470
	Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingletonPtr()->getHlmsManager();
	Ogre::HlmsPbs* hlmsPbs = static_cast<Ogre::HlmsPbs*>(hlmsManager->getHlms(Ogre::HLMS_PBS));

	if (0 == this->projectParameter.forwardMode)
	{
		hlmsPbs->setUseLightBuffers(false);
		this->sceneManager->setForward3D(false, this->projectParameter.lightWidth, this->projectParameter.lightHeight, this->projectParameter.numLightSlices, this->projectParameter.lightsPerCell,
			this->projectParameter.minLightDistance, this->projectParameter.maxLightDistance);
		this->sceneManager->setForwardClustered(false, this->projectParameter.lightWidth, this->projectParameter.lightHeight, this->projectParameter.numLightSlices, this->projectParameter.lightsPerCell,
			10, 10, this->projectParameter.minLightDistance, this->projectParameter.maxLightDistance);
	}
	else if (1 == this->projectParameter.forwardMode)
	{
		hlmsPbs->setUseLightBuffers(true);
		this->sceneManager->setForward3D(true, this->projectParameter.lightWidth, this->projectParameter.lightHeight, this->projectParameter.numLightSlices, this->projectParameter.lightsPerCell,
                                            this->projectParameter.minLightDistance, this->projectParameter.maxLightDistance);
	}
	else if (2 == this->projectParameter.forwardMode)
	{
		hlmsPbs->setUseLightBuffers(true);
		this->sceneManager->setForwardClustered(true, this->projectParameter.lightWidth, this->projectParameter.lightHeight, this->projectParameter.numLightSlices, this->projectParameter.lightsPerCell,
                                            10, 10, this->projectParameter.minLightDistance, this->projectParameter.maxLightDistance);
	}

	NOWA::Core::getSingletonPtr()->setGlobalRenderDistance(this->projectParameter.renderDistance);

	OgreNewt::World* ogreNewt = NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->getOgreNewt();
	if (nullptr != ogreNewt)
	{
		ogreNewt->setSolverModel(this->projectParameter.solverModel);
		ogreNewt->setMultithreadSolverOnSingleIsland(true == this->projectParameter.solverForSingleIsland ? 1 : 0);
		ogreNewt->setBroadPhaseAlgorithm(this->projectParameter.broadPhaseAlgorithm);
		ogreNewt->setUpdateFPS(this->projectParameter.physicsUpdateRate, 5);
		ogreNewt->setThreadCount(this->projectParameter.physicsThreadCount);
		ogreNewt->setDefaultLinearDamping(this->projectParameter.linearDamping);
		ogreNewt->setDefaultAngularDamping(this->projectParameter.angularDamping);
		NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->setGlobalGravity(this->projectParameter.gravity);
	}

	if (true == this->projectParameter.hasRecast)
	{
		OgreRecastConfigParams params;

		params.setCellSize(projectParameter.cellSize);
		params.setCellHeight(projectParameter.cellHeight);
		params.setAgentMaxSlope(projectParameter.agentMaxSlope);
		params.setAgentMaxClimb(projectParameter.agentMaxClimb);
		params.setAgentHeight(projectParameter.agentHeight);
		params.setAgentRadius(projectParameter.agentRadius);
		params.setEdgeMaxLen(projectParameter.edgeMaxLen);
		params.setEdgeMaxError(projectParameter.edgeMaxError);
		params.setRegionMinSize(projectParameter.regionMinSize);
		params.setRegionMergeSize(projectParameter.regionMergeSize);
		params.setVertsPerPoly(projectParameter.vertsPerPoly);
		params.setDetailSampleDist(projectParameter.detailSampleDist);
		params.setDetailSampleMaxError(projectParameter.detailSampleMaxError);
		params.setKeepInterResults(projectParameter.keepInterResults);

		NOWA::AppStateManager::getSingletonPtr()->getOgreRecastModule()->createOgreRecast(this->sceneManager, params, projectParameter.pointExtends);

		// Sent event that scene has been modified
		boost::shared_ptr<NOWA::EventDataSceneModified> eventDataSceneModified(new NOWA::EventDataSceneModified());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataSceneModified);
	}

	// Remove .scene, if user typed
	size_t found = this->projectParameter.sceneName.find(".scene");
	if (found != std::wstring::npos)
	{
		this->projectParameter.sceneName = this->projectParameter.sceneName.substr(0, this->projectParameter.sceneName.size() - 6);
	}
	this->projectParameter.projectName = NOWA::Core::getSingletonPtr()->getProjectName();

	this->sceneManager->setShadowColour(Ogre::ColourValue(this->projectParameter.shadowColor.r, this->projectParameter.shadowColor.g, this->projectParameter.shadowColor.b, 1.0f));
	NOWA::WorkspaceModule::getInstance()->setShadowQuality(static_cast<Ogre::HlmsPbs::ShadowFilter>(this->projectParameter.shadowQualityIndex), true);
	NOWA::WorkspaceModule::getInstance()->setAmbientLightMode(static_cast<Ogre::HlmsPbs::AmbientLightMode>(this->projectParameter.ambientLightModeIndex));

	boost::shared_ptr<EventDataSceneValid> eventDataSceneValid(new EventDataSceneValid(true));
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataSceneValid);
}

void ProjectManager::destroyScene(void)
{
	// Delete camera that existed, before a project was created
	Ogre::Camera* camera = NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
	// If there is already a camera component with this camera, let it be destroyed by the camera component when NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->destroyContent(); is called
	bool foundCorrectCameraComponent = false;
	auto gameObjects = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects();
	for (auto& it = gameObjects->begin(); it != gameObjects->end(); ++it)
	{
		NOWA::GameObject* gameObject = it->second.get();
		auto cameraComponent = NOWA::makeStrongPtr(gameObject->getComponent<NOWA::CameraComponent>());
		if (nullptr != cameraComponent)
		{
			if (cameraComponent->getCamera() == camera)
			{
				foundCorrectCameraComponent = true;
				NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->removeCamera(camera);
			}
		}
	}
	// Else destroy it manually here
	if (nullptr != camera && false == foundCorrectCameraComponent)
	{
		NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->removeCamera(camera);
		this->sceneManager->destroyCamera(camera);
		camera = nullptr;
	}

	NOWA::AppStateManager::getSingletonPtr()->getOgreRecastModule()->destroyContent();
	NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->destroyContent();
	NOWA::WorkspaceModule::getInstance()->destroyContent();
	NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->destroyContent();

	if (nullptr != this->dotSceneExportModule)
	{
		delete this->dotSceneExportModule;
		this->dotSceneExportModule = nullptr;
	}
	if (nullptr != this->dotSceneImportModule)
	{
		delete this->dotSceneImportModule;
		this->dotSceneImportModule = nullptr;
	}
	this->sunLight = nullptr;
}

void ProjectManager::createDummyCamera(void)
{
	Ogre::Camera* camera = this->sceneManager->createCamera("GamePlayCamera");
	NOWA::Core::getSingletonPtr()->setMenuSettingsForCamera(camera);
	camera->setFOVy(Ogre::Degree(90.0f));
	camera->setNearClipDistance(0.1f);
	camera->setFarClipDistance(500.0f);
	camera->setQueryFlags(0 << 0);
	// camera->setPosition(0.0f, 5.0f, -2.0f);
	camera->setPosition(camera->getParentSceneNode()->convertLocalToWorldPositionUpdated(Ogre::Vector3(0.0f, 5.0f, -2.0f)));

	NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->init("CameraManager1", camera);
	NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->addCameraBehavior(new NOWA::BaseCamera());
	NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setActiveCameraBehavior(NOWA::BaseCamera::BehaviorType());
	// Create dummy workspace
	NOWA::WorkspaceModule::getInstance()->setPrimaryWorkspace(this->sceneManager, camera, nullptr);
}

void ProjectManager::saveGroup(const Ogre::String& filePathName)
{
	if (nullptr != this->dotSceneExportModule)
	{
		auto& selectedGameObjects = this->editorManager->getSelectionManager()->getSelectedGameObjects();
		if (selectedGameObjects.size() > 0)
		{
			std::vector<unsigned long> gameObjectIds(selectedGameObjects.size());
			unsigned int i = 0;
			for (auto& it = selectedGameObjects.cbegin(); it != selectedGameObjects.cend(); ++it)
			{
				gameObjectIds[i++] = it->second.gameObject->getId();
			}
			this->dotSceneExportModule->exportGroup(gameObjectIds, filePathName, "Projects");
		}
	}
	boost::shared_ptr<EventDataSceneValid> eventDataSceneValid(new EventDataSceneValid(true));
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataSceneValid);
}

void ProjectManager::showFileSaveDialog(const Ogre::String& action, const Ogre::String& fileMask, const Ogre::String& specifiedTargetFolder)
{
	this->openSaveFileDialog->setFileName("");
	
	boost::shared_ptr<EventDataSceneValid> eventDataSceneValid(new EventDataSceneValid(false));
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataSceneValid);
	
	Ogre::ResourceGroupManager::LocationList resLocationsList = Ogre::ResourceGroupManager::getSingleton().getResourceLocationList("Projects");
	Ogre::ResourceGroupManager::LocationList::const_iterator it = resLocationsList.cbegin();
	Ogre::ResourceGroupManager::LocationList::const_iterator itEnd = resLocationsList.cend();

	Ogre::String targetFolder;
	for (; it != itEnd; ++it)
	{
		targetFolder = (*it)->archive->getName();
		if (false == specifiedTargetFolder.empty())
		{
			targetFolder += "/" + specifiedTargetFolder;
		}
		break;
	}

	if ("*.group" == fileMask)
		targetFolder += "/Groups";

	// Set the target folder specified in world resource group
	this->openSaveFileDialog->setCurrentFolder(targetFolder);
	// this->openSaveFileDialog->setRecentFolders(RecentFilesManager::getInstance().getRecentFolders());
	this->openSaveFileDialog->setDialogInfo(MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{SaveFile}"),
		MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{SaveAs}"), MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{UpFolder}"), false);
	this->openSaveFileDialog->setMode(action);
	this->openSaveFileDialog->setFileMask(fileMask);
	this->openSaveFileDialog->doModal();
}

bool ProjectManager::showFileOpenDialog(const Ogre::String& action, const Ogre::String& fileMask)
{
	// For projects folder mode is correct
	bool folderMode = false;
	// Only when a scene is loaded, set the scene as invalid, not for groups etc!
	if ("*.scene" == fileMask)
	{
		folderMode = false;
		boost::shared_ptr<EventDataSceneValid> eventDataSceneValid(new EventDataSceneValid(false));
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataSceneValid);
	}
	else if ("" == fileMask)
	{
		folderMode = true;
	}
	Ogre::ResourceGroupManager::LocationList resLocationsList = Ogre::ResourceGroupManager::getSingleton().getResourceLocationList("Projects");
	Ogre::ResourceGroupManager::LocationList::const_iterator it = resLocationsList.cbegin();
	Ogre::ResourceGroupManager::LocationList::const_iterator itEnd = resLocationsList.cend();

	Ogre::String targetFolder;
	for (; it != itEnd; ++it)
	{
		targetFolder = (*it)->archive->getName();
		break;
	}

	if ("*.group" == fileMask)
		targetFolder += "/Groups";
	else if ("" == fileMask)
	{
		Ogre::ResourceGroupManager::LocationList resLocationsList = Ogre::ResourceGroupManager::getSingleton().getResourceLocationList("Models");
		Ogre::ResourceGroupManager::LocationList::const_iterator it = resLocationsList.cbegin();
		Ogre::ResourceGroupManager::LocationList::const_iterator itEnd = resLocationsList.cend();

		for (; it != itEnd; ++it)
		{
			targetFolder = (*it)->archive->getName();
			break;
		}
	}

	// Set the target folder specified in world resource group
	this->openSaveFileDialog->setCurrentFolder(targetFolder);
	// this->openSaveFileDialog->setRecentFolders(RecentFilesManager::getInstance().getRecentFolders());

	this->openSaveFileDialog->setDialogInfo(MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{OpenFile}"),
		MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{Open}"), MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{UpFolder}"), folderMode);

	this->openSaveFileDialog->setMode(action);
	this->openSaveFileDialog->setFileMask(fileMask);
	this->openSaveFileDialog->setFileName("");
	this->openSaveFileDialog->doModal();

	return true;
}

void ProjectManager::notifyEndDialog(tools::Dialog* sender, bool result)
{
	if (result)
	{
		if (this->openSaveFileDialog->getMode() == "SaveProject")
		{
			Ogre::String tempFileName = this->openSaveFileDialog->getFilePathName();
			
			Ogre::String tempProjectName = NOWA::Core::getSingletonPtr()->getProjectNameFromPath(tempFileName);
			NOWA::Core::getSingletonPtr()->setProjectName(tempProjectName);
			this->projectParameter.projectName = tempProjectName;

			size_t found = tempFileName.find(".");
			if (Ogre::String::npos != found)
			{
				tempFileName = tempFileName.substr(0, found);
			}
			this->projectParameter.sceneName = tempFileName;

			if (false == this->checkProjectExists(this->projectParameter.projectName + "/" + this->projectParameter.sceneName + ".scene"))
			{
				this->saveProject(this->projectParameter.projectName + "/" + this->projectParameter.sceneName + ".scene");
			}
		}
		else if (this->openSaveFileDialog->getMode() == "LoadProject")
		{
			this->loadProject(this->openSaveFileDialog->getFilePathName());
		}
		else if (this->openSaveFileDialog->getMode() == "SaveGroup")
		{
			Ogre::String tempFileName = this->openSaveFileDialog->getFileName();
			// Remove the group extension, because its added in dot scene import module automatically
			size_t found = tempFileName.find(".");
			if (Ogre::String::npos != found)
			{
				tempFileName = tempFileName.substr(0, found);
			}
			this->saveGroup(tempFileName);
		}
		else if (this->openSaveFileDialog->getMode() == "LoadGroup")
		{
			Ogre::String tempFileName = this->openSaveFileDialog->getFileName();
			if (nullptr == this->dotSceneImportModule)
			{
				this->dotSceneImportModule = new NOWA::DotSceneImportModule(this->sceneManager, NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera(), nullptr);
			}
			std::vector<unsigned long> gameObjectIds = this->dotSceneImportModule->parseGroup(tempFileName, "Projects");
			this->editorManager->attachGroupToPlaceNode(gameObjectIds);
			boost::shared_ptr<EventDataSceneValid> eventDataSceneValid(new EventDataSceneValid(true));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataSceneValid);
		}
		else if (this->openSaveFileDialog->getMode() == "AddMeshResources")
		{
			Ogre::String meshResourcesFolder = this->openSaveFileDialog->getCurrentFolder();
			Ogre::String resourceName = "Default";
			const size_t lastSlashIndex = meshResourcesFolder.rfind('\\');
			if (std::string::npos != lastSlashIndex)
			{
				resourceName = meshResourcesFolder.substr(lastSlashIndex + 1, meshResourcesFolder.size() - lastSlashIndex);
			}
			
			Ogre::String defaultPointer = MyGUI::PointerManager::getInstancePtr()->getDefaultPointer();
			MyGUI::PointerManager::getInstancePtr()->setPointer("beam");
			Ogre::ResourceGroupManager::getSingleton().addResourceLocation(meshResourcesFolder, "FileSystem", resourceName);
			Ogre::ResourceGroupManager::getSingleton().initialiseResourceGroup(resourceName, false);
			MyGUI::PointerManager::getInstancePtr()->setPointer(defaultPointer);

			this->additionalMeshResources.emplace_back(resourceName);

			// Sent when a name has changed, so that the resources panel can be refreshed with new values
			boost::shared_ptr<EventDataRefreshMeshResources> eventDataRefreshMeshResources(new EventDataRefreshMeshResources());
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshMeshResources);
		}
		else if (this->openSaveFileDialog->getMode() == "SaveDatablocks")
		{
			Ogre::String fileName = this->openSaveFileDialog->getCurrentFolder() + "\\" + this->openSaveFileDialog->getFileName();

			NOWA::DeployResourceModule::getInstance()->deploy(NOWA::Core::getSingletonPtr()->getApplicationName(), fileName);

			boost::shared_ptr<EventDataSceneValid> eventDataSceneValid(new EventDataSceneValid(true));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataSceneValid);
		}
		else if (this->openSaveFileDialog->getMode() == "CopyScene")
		{
			if (false == this->checkProjectExists(this->projectParameter.projectName + "/" + this->openSaveFileDialog->getFileName() + ".scene"))
			{
				// NOWA::DeployResourceModule::getInstance()->copyScene(currentFilePathName + ".scene", newFilePathName + ".scene");
				Ogre::String tempFileName = this->openSaveFileDialog->getFileName();
				// Remove the scene extension, because its added in dot scene import module automatically
				size_t found = tempFileName.find(".");
				if (Ogre::String::npos != found)
				{
					tempFileName = tempFileName.substr(0, found);
				}

				if (nullptr != this->dotSceneExportModule)
				{
					delete this->dotSceneExportModule;
					this->dotSceneExportModule = new NOWA::DotSceneExportModule(this->sceneManager, this->ogreNewt, this->projectParameter);
				}
				this->dotSceneExportModule->copyScene(this->projectParameter.sceneName, this->projectParameter.projectName, tempFileName, "Projects");
			}
		}
	}
	else
	{
		// Only set scene valid, if a world has already been loaded
		if (false == NOWA::Core::getSingletonPtr()->getCurrentWorldPath().empty())
		{
			boost::shared_ptr<EventDataSceneValid> eventDataSceneValid(new EventDataSceneValid(true));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataSceneValid);
		}
	}

	this->openSaveFileDialog->endModal();
	// MyGUI::InputManager::getInstancePtr()->removeWidgetModal(sender);
}

bool ProjectManager::checkProjectExists(const Ogre::String& fileName)
{
	Ogre::String filePathName;

	Ogre::ResourceGroupManager::LocationList resLocationsList = Ogre::ResourceGroupManager::getSingleton().getResourceLocationList("Projects");
	Ogre::ResourceGroupManager::LocationList::const_iterator it = resLocationsList.cbegin();
	Ogre::ResourceGroupManager::LocationList::const_iterator itEnd = resLocationsList.cend();

	for (; it != itEnd; ++it)
	{
		// Project is always: "projects/projectName/sceneName.scene"
		filePathName = (*it)->archive->getName() + "/" + fileName;
	
		// Check if a project with the same name does already exist
		std::ifstream ifs(filePathName);
		if (true == ifs.good())
		{
			MyGUI::Message* messageBox = MyGUI::Message::createMessageBox("Menue", MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{Overwrite}"),
				MyGUI::MessageBoxStyle::IconWarning | MyGUI::MessageBoxStyle::Yes | MyGUI::MessageBoxStyle::No, "Popup", true);

			messageBox->eventMessageBoxResult += MyGUI::newDelegate(this, &ProjectManager::notifyMessageBoxEnd);
			return true;
		}
		break;
	}
	return false;
}

void ProjectManager::notifyMessageBoxEnd(MyGUI::Message* sender, MyGUI::MessageBoxStyle result)
{
	if (result == MyGUI::MessageBoxStyle::Yes)
	{
		this->saveProject(this->projectParameter.projectName + "/" + this->projectParameter.sceneName + ".scene");
	}
	boost::shared_ptr<EventDataSceneValid> eventDataSceneValid(new EventDataSceneValid(true));
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataSceneValid);
}

Ogre::String ProjectManager::getSceneFileName(void) const
{
	return this->projectParameter.sceneName;
}

void ProjectManager::setOgreNewt(OgreNewt::World* ogreNewt)
{
	this->ogreNewt = ogreNewt;
}

OgreNewt::World* ProjectManager::getOgreNewt(void) const
{
	return this->ogreNewt;
}

void ProjectManager::setProjectParameter(NOWA::ProjectParameter projectParameter)
{
	this->projectParameter = projectParameter;
}

NOWA::ProjectParameter ProjectManager::getProjectParameter(void)
{
	return this->projectParameter;
}
#include "NOWAPrecompiled.h"
#include "GameProgressModule.h"
#include "gameobject/GameObjectController.h"
#include "gameobject/PhysicsComponent.h"
#include "gameObject/CameraComponent.h"
#include "gameObject/AttributesComponent.h"
#include "main/ProcessManager.h"
#include "main/AppStateManager.h"
#include "main/Core.h"

#include "OgreNewtModule.h"
#include "camera/CameraManager.h"
#include "WorkspaceModule.h"

namespace NOWA
{
	class LoadWorldProcess : public NOWA::Process
	{
	public:
		explicit LoadWorldProcess(const Ogre::String& appStateName, DotSceneImportModule* dotSceneImportModule, const Ogre::String& nextWorldName, const Ogre::String& playerName, 
			bool worldChanged, bool showProgress = false)
			: appStateName(appStateName),
			dotSceneImportModule(dotSceneImportModule),
			nextWorldName(nextWorldName),
			playerName(playerName),
			worldChanged(worldChanged),
			showProgress(showProgress)
		{

		}

	protected:
		virtual void onInit(void) override
		{
			// Shows black picture
			// FaderProcess faderProcess(NOWA::FaderProcess::FadeOperation::FADE_OUT, 0.0f);
			// Ogre::Root::getSingletonPtr()->renderOneFrame();
			
			this->succeed();
			
			AppStateManager::getSingletonPtr()->getGameProgressModule(this->appStateName)->resetContent();

			EngineResourceFadeListener* engineResourceListener = nullptr;

			if (true == this->showProgress)
			{
				engineResourceListener = new EngineResourceFadeListener(Core::getSingletonPtr()->getOgreRenderWindow());
				Core::getSingletonPtr()->setEngineResourceListener(engineResourceListener);

				engineResourceListener->showLoadingBar();
			}

			bool success = this->dotSceneImportModule->parseScene(Core::getSingletonPtr()->getProjectName(), this->nextWorldName, "Projects", nullptr, nullptr, showProgress);
			
			std::pair<Ogre::Real, Ogre::Real> continueData(0.0f, 0.0f);

			if (true == this->showProgress)
			{
				continueData = engineResourceListener->hideLoadingBar();
				Core::getSingletonPtr()->resetEngineResourceListener();

				delete engineResourceListener;
				engineResourceListener = nullptr;
			}
			
			if (false == success)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameProgressModule]: Error: Could not parse world: '" + this->nextWorldName + "' correctly!");
				AppStateManager::getSingletonPtr()->getGameProgressModule(this->appStateName)->setIsWorldLoading(false);
				return;
			}
			// Continue fading after loading world, if there is enough time left
			/*if (continueData.second > 1.0f)
			{
				NOWA::ProcessManager::getInstance()->attachProcess(NOWA::ProcessPtr(new NOWA::FaderProcess(NOWA::FaderProcess::FadeOperation::FADE_IN, 10.0f, continueData.first, continueData.second)));
			}*/

			// NOWA::ProcessManager::getInstance()->attachProcess(NOWA::ProcessPtr(new NOWA::FaderProcess(NOWA::FaderProcess::FadeOperation::FADE_IN, 1.0f)));
			// Ogre::Root::getSingletonPtr()->renderOneFrame();

			// Set the data for a state
			NOWA::SceneParameter sceneParameter;
			sceneParameter.appStateName = this->appStateName;
			sceneParameter.sceneManager = this->dotSceneImportModule->getSceneManager();
			sceneParameter.mainCamera = this->dotSceneImportModule->getMainCamera();
			sceneParameter.sunLight = this->dotSceneImportModule->getSunLight();
			sceneParameter.ogreNewt = AppStateManager::getSingletonPtr()->getOgreNewtModule(this->appStateName)->getOgreNewt();
			sceneParameter.dotSceneImportModule = this->dotSceneImportModule;

			// NOWA::AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->start();

			// NOWA::AppStateManager::getSingletonPtr()->getGameProgressModule(this->appStateName)->determinePlayerStartLocation(this->nextWorldName);

			// Send event, that world has been loaded. Note: When world has been changed, send that flag. E.g. in DesignState, if true, also call GameObjectController::start, so that when in simulation
			// and the world has been changed, remain in simulation and maybe activate player controller, so that the player may continue his game play
			boost::shared_ptr<EventDataWorldLoaded> eventDataWorldLoaded(new EventDataWorldLoaded(this->worldChanged, this->dotSceneImportModule->getProjectParameter(), sceneParameter));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager(this->appStateName)->triggerEvent(eventDataWorldLoaded);
			
			 for (unsigned short i = 0; i < 2; i++)
			 {
			 	Ogre::Root::getSingletonPtr()->renderOneFrame();
			 }

			AppStateManager::getSingletonPtr()->getGameProgressModule(this->appStateName)->setIsWorldLoading(false);
		}

		virtual void onUpdate(float dt) override
		{
			this->succeed();
		}
	private:
		Ogre::String appStateName;
		DotSceneImportModule* dotSceneImportModule;
		Ogre::String nextWorldName;
		Ogre::String playerName;
		bool worldChanged;
		bool showProgress;
	};

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class LoadProgressProcess : public NOWA::Process
	{
	public:
		explicit LoadProgressProcess(const Ogre::String& appStateName, DotSceneImportModule* dotSceneImportModule, const Ogre::String& saveName, bool crypted, bool sceneSnapshot, const Ogre::String& globalAttributesStream, const Ogre::String& playerName, bool worldChanged, bool showProgress = false)
			: appStateName(appStateName),
			dotSceneImportModule(dotSceneImportModule),
			saveName(saveName),
			crypted(crypted),
			sceneSnapshot(sceneSnapshot),
			globalAttributesStream(globalAttributesStream),
			playerName(playerName),
			worldChanged(worldChanged),
			showProgress(showProgress)
		{

		}

	protected:
		virtual void onInit(void) override
		{
			this->succeed();

			if (true == this->sceneSnapshot)
			{
				Ogre::String openFilePathName = Core::getSingletonPtr()->getSaveFilePathName(this->saveName, ".scene");

				std::pair<Ogre::String, Ogre::String> snapshotProjectAndSceneName = this->dotSceneImportModule->getProjectAndSceneName(openFilePathName, crypted);
				if (true == openFilePathName.empty())
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameProgressModule]: Error: Could not parse saved name: '" + this->saveName + "' because its unclear to which scene the save snapshot belongs!");
					AppStateManager::getSingletonPtr()->getGameProgressModule(this->appStateName)->setIsWorldLoading(false);
					return;
				}

				// Shows black picture
				// FaderProcess faderProcess(NOWA::FaderProcess::FadeOperation::FADE_OUT, 0.0f);
				// Ogre::Root::getSingletonPtr()->renderOneFrame();

				AppStateManager::getSingletonPtr()->getGameProgressModule(this->appStateName)->resetContent();

				EngineResourceFadeListener* engineResourceListener = nullptr;

				if (true == this->showProgress)
				{
					engineResourceListener = new EngineResourceFadeListener(Core::getSingletonPtr()->getOgreRenderWindow());
					Core::getSingletonPtr()->setEngineResourceListener(engineResourceListener);

					engineResourceListener->showLoadingBar();
				}

				bool success = this->dotSceneImportModule->parseSceneSnapshot(snapshotProjectAndSceneName.first, snapshotProjectAndSceneName.second, openFilePathName, this->crypted, this->showProgress);
				if (false == success)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameProgressModule]: Error: Could not parse scene: '" + this->saveName + "' correctly!");
					throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[GameProgressModule]: Error: Could not parse scene: '" + this->saveName + "' correctly!" + "'\n", "NOWA");
				}

				std::pair<Ogre::Real, Ogre::Real> continueData(0.0f, 0.0f);

				if (true == this->showProgress)
				{
					continueData = engineResourceListener->hideLoadingBar();
					Core::getSingletonPtr()->resetEngineResourceListener();

					delete engineResourceListener;
					engineResourceListener = nullptr;
				}

				// Continue fading after loading world, if there is enough time left
				/*if (continueData.second > 1.0f)
				{
					NOWA::ProcessManager::getInstance()->attachProcess(NOWA::ProcessPtr(new NOWA::FaderProcess(NOWA::FaderProcess::FadeOperation::FADE_IN, 10.0f, continueData.first, continueData.second)));
				}*/

				// NOWA::ProcessManager::getInstance()->attachProcess(NOWA::ProcessPtr(new NOWA::FaderProcess(NOWA::FaderProcess::FadeOperation::FADE_IN, 1.0f)));
				// Ogre::Root::getSingletonPtr()->renderOneFrame();

				// Set the data for a state
				NOWA::SceneParameter sceneParameter;
				sceneParameter.appStateName = this->appStateName;
				sceneParameter.sceneManager = this->dotSceneImportModule->getSceneManager();
				sceneParameter.mainCamera = this->dotSceneImportModule->getMainCamera();
				sceneParameter.sunLight = this->dotSceneImportModule->getSunLight();
				sceneParameter.ogreNewt = AppStateManager::getSingletonPtr()->getOgreNewtModule(this->appStateName)->getOgreNewt();
				sceneParameter.dotSceneImportModule = this->dotSceneImportModule;

				// NOWA::AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->start();

				// NOWA::AppStateManager::getSingletonPtr()->getGameProgressModule(this->appStateName)->determinePlayerStartLocation(this->nextWorldName);

				// Send event, that world has been loaded. Note: When world has been changed, send that flag. E.g. in DesignState, if true, also call GameObjectController::start, so that when in simulation
				// and the world has been changed, remain in simulation and maybe activate player controller, so that the player may continue his game play
				boost::shared_ptr<EventDataWorldLoaded> eventDataWorldLoaded(new EventDataWorldLoaded(this->worldChanged, this->dotSceneImportModule->getProjectParameter(), sceneParameter));
				NOWA::AppStateManager::getSingletonPtr()->getEventManager(this->appStateName)->triggerEvent(eventDataWorldLoaded);
			}

			bool success = AppStateManager::getSingletonPtr()->getGameProgressModule(this->appStateName)->internalReadGlobalAttributes(globalAttributesStream);
			if (false == success)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameProgressModule]: Error: Could not read global attributes for saved name: '" + this->saveName + "' correctly!");
			}

			for (unsigned short i = 0; i < 2; i++)
			{
				Ogre::Root::getSingletonPtr()->renderOneFrame();
			}

			AppStateManager::getSingletonPtr()->getGameProgressModule(this->appStateName)->setIsWorldLoading(false);
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
		bool worldChanged;
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
		bWorldLoading(false)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[GameProgressModule] Module created");
	}

	GameProgressModule::~GameProgressModule()
	{

	}

	bool GameProgressModule::isWorldLoading(void) const
	{
		return this->bWorldLoading;
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
		this->worldMap.clear();
		
		// Delete all global attributes
		auto& it = this->globalAttributesMap.begin();

		while (it != this->globalAttributesMap.end())
		{
			Variant* globalAttribute = it->second;
			delete globalAttribute;
			globalAttribute = nullptr;
			++it;
		}
		this->globalAttributesMap.clear();
		this->bWorldLoading = false;
	}

	void GameProgressModule::resetContent(void)
	{
		// this->sceneManager->destroyAllCameras();
		// std::vector<Ogre::String> excludeGameObjects = { "MainCamera" };
		AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->stop();
		AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->destroyContent(/*excludeGameObjects*/);
		WorkspaceModule::getInstance()->destroyContent();

		// AppStateManager::getSingletonPtr()->getCameraManager(this->appStateName)->destroyContent();

		// this->sceneManager->clearScene(true);

		AppStateManager::getSingletonPtr()->getOgreNewtModule(this->appStateName)->destroyContent();
	}
	
	void GameProgressModule::start(void)
	{
		
	}
	
	void GameProgressModule::stop(void)
	{
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
			return this->player->getName();
		else
			return "";
	}

	void GameProgressModule::addWorld(const Ogre::String& currentWorldName, const Ogre::String& reachableWorldName, 
		const Ogre::String& targetLocationName, const Ogre::Vector2& exitDirection, const Ogre::Vector3& startPosition, bool xyAxis)
	{
		this->currentWorldName = currentWorldName;
		Ogre::String reachableWorldProjectName;
		if (!reachableWorldName.empty())
		{
			reachableWorldProjectName = Core::getSingletonPtr()->getProjectName() + "/" + reachableWorldName + ".scene";
		}

		auto& it = this->worldMap.find(currentWorldName);
		if (this->worldMap.end() == it)
		{
			GameProgressModule::WorldData worldData(currentWorldName, targetLocationName, exitDirection, startPosition, xyAxis);
			if (!reachableWorldName.empty())
			{
				worldData.addReachableWorld(reachableWorldProjectName);
			}
			this->worldMap.emplace(currentWorldName, worldData);
			// actualize iterator for predecessor settings below
			it = this->worldMap.find(currentWorldName);
		}
		else
		{
			if (!currentWorldName.empty())
			{
				it->second.setWorldName(currentWorldName);
			}
			if (!reachableWorldName.empty())
			{
				it->second.addReachableWorld(reachableWorldProjectName);
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
		if (!reachableWorldName.empty() && this->worldMap.end() != it)
		{
			// check if the reachable app state name already exist, if yes, it is the predecessor!
			const auto& it2 = this->worldMap.find(reachableWorldProjectName);
			if (this->worldMap.end() != it2)
			{
				// the currents app state predecessor is the reachable one
				it->second.predecessorWorldData = &(it2->second);
				// ?? e.g. SimulationState is predecessor of SimulationState2 and vice versa??
				// it2->second.predecessorWorld = &(it->second);
			}
		}
	}

	std::vector<Ogre::String>* GameProgressModule::getReachableWorlds(const Ogre::String& currentWorldName)
	{
		const auto& it = this->worldMap.find(currentWorldName);
		if (this->worldMap.end() != it)
		{
			return &it->second.getReachableWorlds();
		}
		return nullptr;
	}

	GameProgressModule::WorldData* GameProgressModule::getPredecessorWorldData(const Ogre::String& currentWorldName)
	{
		const auto& it = this->worldMap.find(currentWorldName);
		if (this->worldMap.end() != it)
		{
			return it->second.getPredecessorWorldData();
		}
		return nullptr;
	}

	GameProgressModule::WorldData* GameProgressModule::getWorldData(const Ogre::String& currentWorldName)
	{
		const auto& it = this->worldMap.find(currentWorldName);
		if (this->worldMap.end() != it)
		{
			return &(it->second);
		}
		return nullptr;
	}

	unsigned int GameProgressModule::getWorldsCount(void) const
	{
		return static_cast<unsigned int>(this->worldMap.size());
	}

	void GameProgressModule::determinePlayerStartLocation(const Ogre::String& currentWorldName)
	{
		Ogre::Vector3 startPosition = Ogre::Vector3::ZERO;
		Ogre::Quaternion startOrientation = Ogre::Quaternion::IDENTITY;

		// if this is not the first loaded app state, calculate the player position, where to start in the app state
		if (0 < this->worldMap.size())
		{
			const auto& predessorWorldData = this->getPredecessorWorldData(Core::getSingletonPtr()->getProjectName() + "/" + currentWorldName + ".scene");
			if (nullptr != predessorWorldData)
			{
				const auto& targetLocation = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromName(predessorWorldData->getTargetLocationName()).get();
				if (nullptr != targetLocation)
				{
				/*	Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameProgressModule] Cannot determine start position, because there is no such location name: '"
						+ predessorWorldData->getTargetLocationName() + "'!");
					throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[GameProgressModule] Cannot determine start position, because there is no such location name: '"
						+ predessorWorldData->getTargetLocationName() + "'!\n", "NOWA");*/

						// get the position of the current app state and the exit direction of the previews one, to determine the final position and orientation
					startPosition = targetLocation->getPosition();
					Ogre::Vector2 exitDirection = predessorWorldData->getExitDirection();
					bool xyAxis = predessorWorldData->getIsXYAxisUsed();
					// offset the player start position, so that the player does not land on an exit but nearby

					if (true == xyAxis)
					{
						startPosition += (Ogre::Vector3(exitDirection.x, exitDirection.y, 0.0f) * 0.5f);
					}
					else
					{
						// use x,z
						startPosition += (Ogre::Vector3(exitDirection.x, 0.0f, exitDirection.y) * 0.5f);
					}

					Ogre::Vector3 playerSize = this->player->getSize();
					Ogre::Vector3 middleOfPlayer = this->player->getMiddle();
					Ogre::Vector3 centerBottom = playerSize - middleOfPlayer;

// Attention: Will that work in a 3d scenario (no jump n run)?
					startPosition.y = startPosition.y + centerBottom.y;

					if (nullptr == this->player)
					{
						Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[GameProgressModule] Error: Cannot determine start position, because no player had been announced to GameProgressModule!");
						throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[GameProgressModule] Error: Cannot determine start position, because no player had been announced to GameProgressModule!\n", "NOWA");
					}

					auto& physicsCompPtr = NOWA::makeStrongPtr(this->player->getComponent<NOWA::PhysicsComponent>());
					if (nullptr == physicsCompPtr)
					{
						Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[GameProgressModule] Error: Cannot determine start position because the player has no physics component!");
						throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[GameProgressModule] Error: Cannot determine start position because the player has no physics component!\n", "NOWA");
					}

					// Attention: Is that correct with 180 quirk??
					if (true == xyAxis)
					{
						startOrientation = this->player->getOrientation().xAxis().getRotationTo(Ogre::Vector3(exitDirection.x, exitDirection.y, 0.0f)).Inverse();
					}
					else
					{
						// use x,z
						startOrientation = this->player->getOrientation().xAxis().getRotationTo(Ogre::Vector3(exitDirection.x, 0.0f, exitDirection.y)).Inverse();
					}

					physicsCompPtr->setPosition(startPosition);
					physicsCompPtr->setOrientation(startOrientation);

					// ugly hack, because when an application state is changed, and for an physics component a position or orientation set, after OgreNewt processed its update
					// the position and orientation are thrash! It may be a ghost bug in OgreNewt. Hence the position orientation setting is done delayed, after OgreNewt has been updated,
					// so that the trash values will be overwritten with the valid ones
					NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.1f));
					delayProcess->attachChild(NOWA::ProcessPtr(new SetPlayerLocationProcess(physicsCompPtr.get(), startPosition, startOrientation)));
					NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
				}
			}
		}
	}

	void GameProgressModule::loadWorld(const Ogre::String& worldName)
	{
		this->setIsWorldLoading(true);

		Ogre::String projectName = NOWA::Core::getSingletonPtr()->getProjectNameFromPath(worldName);
		NOWA::Core::getSingletonPtr()->setProjectName(projectName);
		Ogre::String fileName = NOWA::Core::getSingletonPtr()->getFileNameFromPath(worldName);

		NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.5f));
		// Creates the delay process and changes the world at another tick. Note, this is necessary
		// because changing the world destroys all game objects and its components.
		// So changing the state directly inside a component would create a mess, since everything will be destroyed
		// and the game object map in update loop becomes invalid while its iterating
		delayProcess->attachChild(NOWA::ProcessPtr(new LoadWorldProcess(this->appStateName, this->dotSceneImportModule, fileName, nullptr != this->player ? this->player->getName() : "", false)));
		NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
	}

	void GameProgressModule::loadWorldShowProgress(const Ogre::String& worldName)
	{
		this->setIsWorldLoading(true);


		Ogre::String projectName = NOWA::Core::getSingletonPtr()->getProjectNameFromPath(worldName);
		NOWA::Core::getSingletonPtr()->setProjectName(projectName);
		Ogre::String fileName = NOWA::Core::getSingletonPtr()->getFileNameFromPath(worldName);

		NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.5f));
		// Creates the delay process and changes the world at another tick. Note, this is necessary
		// because changing the world destroys all game objects and its components.
		// So changing the state directly inside a component would create a mess, since everything will be destroyed
		// and the game object map in update loop becomes invalid while its iterating
		delayProcess->attachChild(NOWA::ProcessPtr(new LoadWorldProcess(this->appStateName, this->dotSceneImportModule, fileName, nullptr != this->player ? this->player->getName() : "", false, true)));
		NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
	}

	void GameProgressModule::changeWorld(const Ogre::String& worldName)
	{
		this->setIsWorldLoading(true);

		NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.5f));
		// Creates the delay process and changes the world at another tick. Note, this is necessary
		// because changing the world destroys all game objects and its components.
		// So changing the state directly inside a component would create a mess, since everything will be destroyed
		// and the game object map in update loop becomes invalid while its iterating
		delayProcess->attachChild(NOWA::ProcessPtr(new LoadWorldProcess(this->appStateName, this->dotSceneImportModule, worldName, nullptr != this->player ? this->player->getName() : "", true)));
		NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
	}

	void GameProgressModule::changeWorldShowProgress(const Ogre::String& worldName)
	{
		this->setIsWorldLoading(true);

		NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.5f));
		// Creates the delay process and changes the world at another tick. Note, this is necessary
		// because changing the world destroys all game objects and its components.
		// So changing the state directly inside a component would create a mess, since everything will be destroyed
		// and the game object map in update loop becomes invalid while its iterating
		delayProcess->attachChild(NOWA::ProcessPtr(new LoadWorldProcess(this->appStateName, this->dotSceneImportModule, worldName,  nullptr != this->player ? this->player->getName() : "", true, true)));
		NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
	}

	Ogre::String GameProgressModule::getCurrentWorldName(void)
	{
		return this->currentWorldName;
	}

	void GameProgressModule::saveProgress(const Ogre::String& saveName, bool crypted, bool sceneSnapshot)
	{
		this->saveName = saveName;
		Ogre::String saveFilePathName = Core::getSingletonPtr()->getSaveFilePathName(this->saveName);
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

			Ogre::String strStream;
			std::ofstream outFile;
			outFile.open(saveFilePathName.c_str());
			if (!outFile)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameProgressModule] ERROR: Could not create file for path: "
					+ saveFilePathName + "'");
				return;
			}

			// Store whether the file should be crypted or not
			outFile << crypted << "\n";

			// Get the game object controller for this app state name
			auto gameObjects = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjects();

			std::ostringstream oStream;

			for (auto& it = gameObjects->cbegin(); it != gameObjects->cend(); ++it)
			{
				const auto& gameObjectPtr = it->second;

				// https://thinkcpp.wordpress.com/2012/04/16/file-to-map-inputoutput/

				// First save the game object id, if it does have an attributes component
				boost::shared_ptr<AttributesComponent> attributesCompPtr = NOWA::makeStrongPtr(gameObjectPtr->getComponent<AttributesComponent>());
				if (nullptr != attributesCompPtr)
				{
					oStream << "[" << Ogre::StringConverter::toString(gameObjectPtr->getId()) << "]\n";
					attributesCompPtr->internalSave(oStream);
				}
			}

			// Store global defined attributes
			oStream << "[GlobalAttributes]" << "\n";
			for (auto& it = this->globalAttributesMap.cbegin(); it != this->globalAttributesMap.cend(); ++it)
			{
				Variant* globalAttribute = it->second;
				int type = globalAttribute->getType();
				Ogre::String name = globalAttribute->getName();
				if (Variant::VAR_BOOL == type)
				{
					oStream << globalAttribute->getName() << "=Bool=" << globalAttribute->getBool() << "\n";
				}
				else if (Variant::VAR_INT == type)
				{
					oStream << globalAttribute->getName() << "=Int=" << globalAttribute->getInt() << "\n";
				}
				else if (Variant::VAR_UINT == type)
				{
					oStream << globalAttribute->getName() << "=UInt=" << globalAttribute->getUInt() << "\n";
				}
				else if (Variant::VAR_ULONG == type)
				{
					oStream << globalAttribute->getName() << "=ULong=" << globalAttribute->getULong() << "\n";
				}
				else if (Variant::VAR_REAL == type)
				{
					oStream << globalAttribute->getName() << "=Real=" << globalAttribute->getReal() << "\n";
				}
				else if (Variant::VAR_STRING == type)
				{
					oStream << globalAttribute->getName() << "=String=" << globalAttribute->getString() << "\n";
				}
				else if (Variant::VAR_VEC2 == type)
				{
					oStream << globalAttribute->getName() << "=Vector2=" << globalAttribute->getVector2() << "\n";
				}
				else if (Variant::VAR_VEC3 == type)
				{
					oStream << globalAttribute->getName() << "=Vector3=" << globalAttribute->getVector3() << "\n";
				}
				else if (Variant::VAR_VEC4 == type)
				{
					oStream << globalAttribute->getName() << "=Vector4=" << globalAttribute->getVector4() << "\n";
				}
				/*else if (VAR_LIST == type)
				{
					oStream << this->attributeNames[i]->getString() << "=StringList="<< this->attributeValues[i]->getVector4() << "\n";
				}*/
			}
			
			// If crypted then encode
			if (true == crypted)
			{
				strStream = Core::getSingletonPtr()->encode64(oStream.str(), true);
			}
			else
			{
				strStream = oStream.str();
			}

			outFile << strStream;
			outFile.close();
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameProgressModule] ERROR: Could not get file path name for saving data: "
				+ saveFilePathName + "'");
		}
	}

	bool GameProgressModule::loadProgress(const Ogre::String& saveName, bool sceneSnapshot, bool showProgress)
	{
		this->setIsWorldLoading(true);

		bool success = false;
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
			return success;
		}

		NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.5f));
		// Creates the delay process and changes the world at another tick. Note, this is necessary
		// because changing the world destroys all game objects and its components.
		// So changing the state directly inside a component would create a mess, since everything will be destroyed
		// and the game object map in update loop becomes invalid while its iterating
		delayProcess->attachChild(NOWA::ProcessPtr(new LoadProgressProcess(this->appStateName, this->dotSceneImportModule, saveName, streamData.first, sceneSnapshot, streamData.second, nullptr != this->player ? this->player->getName() : "", true)));
		NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);

		return success;
	}

	bool GameProgressModule::internalReadGlobalAttributes(const Ogre::String& globalAttributesStream)
	{
		bool success = true;
		std::istringstream inStream(globalAttributesStream);

		Ogre::String line;

		while (std::getline(inStream, line)) // This is used, because white spaces are also read
		{
			if (true == line.empty())
				continue;

			// Read till global attributes section
			size_t foundGlobalAttributeSection = line.find("[GlobalAttributes]");
			if (foundGlobalAttributeSection != Ogre::String::npos)
			{
				break;
			}

			// GameObject
			Ogre::String gameObjectId = line.substr(1, line.size() - 2);
			unsigned long id;
			std::istringstream(gameObjectId) >> id;

			// Get the game object controller for this app state name
			auto& gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(id);
			if (nullptr != gameObjectPtr)
			{
				boost::shared_ptr<AttributesComponent> attributesCompPtr = NOWA::makeStrongPtr(gameObjectPtr->getComponent<AttributesComponent>());
				if (nullptr != attributesCompPtr)
				{
					// Read data and set for attributes component
					success = attributesCompPtr->internalRead(inStream);
				}
			}
		}

		// Read possible global attributes
		Ogre::StringVector data;

		// parse til eof
		while (std::getline(inStream, line)) // This is used, because white spaces are also read
		{
			// Parse til next game object
			if (line.find("[") != Ogre::String::npos)
				break;

			data = Ogre::StringUtil::split(line, "=");
			if (data.size() < 3)
				continue;

			Variant* globalAttribute = nullptr;
			auto& it = this->globalAttributesMap.find(data[0]);
			if (it != this->globalAttributesMap.cend())
			{
				globalAttribute = it->second;
			}
			else
			{
				globalAttribute = new Variant(data[0]);
				globalAttribute->setValue(data[1]);
				this->globalAttributesMap.emplace(data[0], globalAttribute);
			}

			if (nullptr != globalAttribute)
			{
				if ("Bool" == data[1])
				{
					globalAttribute->setValue(Ogre::StringConverter::parseBool(data[2]));
					success = true;
				}
				else if ("Int" == data[1])
				{
					globalAttribute->setValue(Ogre::StringConverter::parseInt(data[2]));
					success = true;
				}
				else if ("UInt" == data[1])
				{
					globalAttribute->setValue(Ogre::StringConverter::parseUnsignedInt(data[2]));
					success = true;
				}
				else if ("ULong" == data[1])
				{
					globalAttribute->setValue(Ogre::StringConverter::parseUnsignedLong(data[2]));
					success = true;
				}
				else if ("Real" == data[1])
				{
					globalAttribute->setValue(Ogre::StringConverter::parseReal(data[2]));
					success = true;
				}
				else if ("String" == data[1])
				{
					globalAttribute->setValue(data[2]);
					success = true;
				}
				else if ("Vector2" == data[1])
				{
					globalAttribute->setValue(Ogre::StringConverter::parseVector2(data[2]));
					success = true;
				}
				else if ("Vector3" == data[1])
				{
					globalAttribute->setValue(Ogre::StringConverter::parseVector3(data[2]));
					success = true;
				}
				else if ("Vector4" == data[1])
				{
					globalAttribute->setValue(Ogre::StringConverter::parseVector4(data[2]));
					success = true;
				}
			}
		}
		return success;
	}

	void GameProgressModule::setIsWorldLoading(bool bWorldLoading)
	{
		this->bWorldLoading = bWorldLoading;
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
				auto& gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(gameObjectId);
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
						afterAttributesContent = streamData.second.substr(foundNextIdPos, remainingSize);
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
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameProgressModule] ERROR: Could not create file for path: "
					+ saveFilePathName + "'");
				return;
			}

			// Store whether the file should be crypted or not
			outFile << crypted << "\n";
			
			if (false == foundSpecific)
			{
				// Get the game object controller for this app state name
				auto& gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(gameObjectId);
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
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameProgressModule] ERROR: Could not get file path name for saving data: "
				+ saveFilePathName + "'");
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
				auto& gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(gameObjectId);
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
								afterAttributesContent = streamData.second.substr(foundNextIdPos, remainingSize);
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
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameProgressModule] ERROR: Could not create file for path: "
					+ saveFilePathName + "'");
				return;
			}

			// Store whether the file should be crypted or not
			outFile << crypted << "\n";
			
			if (false == foundSpecific)
			{
				// Get the game object controller for this app state name
				auto& gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(gameObjectId);
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
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[GameProgressModule] ERROR: Could not get file path name for saving data: "
				+ saveFilePathName + "'");
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
				continue;

			// GameObject
			Ogre::String strGameObjectId = line.substr(1, line.size() - 2);
			unsigned long id;
			std::istringstream(strGameObjectId) >> id;
			
			if (id == gameObjectId)
			{
				// Get the game object controller for this app state name
				auto& gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(id);
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
				continue;

			// GameObject
			Ogre::String strGameObjectId = line.substr(1, line.size() - 2);
			unsigned long id;
			std::istringstream(strGameObjectId) >> id;
			
			if (id == gameObjectId)
			{
				// Get the game object controller for this app state name
				auto& gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(id);
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
		Variant* globalAttribute = nullptr;
		auto& it = this->globalAttributesMap.find(attributeName);
		if (it != this->globalAttributesMap.cend())
		{
			globalAttribute = it->second;
		}
		return globalAttribute;
	}

	Variant* GameProgressModule::setGlobalBoolValue(const Ogre::String& attributeName, bool value)
	{
		Variant* globalAttribute = nullptr;
		auto& it = this->globalAttributesMap.find(attributeName);
		if (it != this->globalAttributesMap.cend())
		{
			globalAttribute = it->second;
			globalAttribute->setValue(value);
		}
		else
		{
			globalAttribute = new Variant(attributeName);
			globalAttribute->setValue(value);
			this->globalAttributesMap.emplace(attributeName, globalAttribute);
		}
		return globalAttribute;
	}

	Variant* GameProgressModule::setGlobalIntValue(const Ogre::String& attributeName, int value)
	{
		Variant* globalAttribute = nullptr;
		auto& it = this->globalAttributesMap.find(attributeName);
		if (it != this->globalAttributesMap.cend())
		{
			globalAttribute = it->second;
			globalAttribute->setValue(value);
		}
		else
		{
			globalAttribute = new Variant(attributeName);
			globalAttribute->setValue(value);
			this->globalAttributesMap.emplace(attributeName, globalAttribute);
		}
		return globalAttribute;
	}

	Variant* GameProgressModule::setGlobalUIntValue(const Ogre::String& attributeName, unsigned int value)
	{
		Variant* globalAttribute = nullptr;
		auto& it = this->globalAttributesMap.find(attributeName);
		if (it != this->globalAttributesMap.cend())
		{
			globalAttribute = it->second;
			globalAttribute->setValue(value);
		}
		else
		{
			globalAttribute = new Variant(attributeName);
			globalAttribute->setValue(value);
			this->globalAttributesMap.emplace(attributeName, globalAttribute);
		}
		return globalAttribute;
	}

	Variant* GameProgressModule::setGlobalULongValue(const Ogre::String& attributeName, unsigned long value)
	{
		Variant* globalAttribute = nullptr;
		auto& it = this->globalAttributesMap.find(attributeName);
		if (it != this->globalAttributesMap.cend())
		{
			globalAttribute = it->second;
			globalAttribute->setValue(value);
		}
		else
		{
			globalAttribute = new Variant(attributeName);
			globalAttribute->setValue(value);
			this->globalAttributesMap.emplace(attributeName, globalAttribute);
		}
		return globalAttribute;
	}

	Variant* GameProgressModule::setGlobalRealValue(const Ogre::String& attributeName, Ogre::Real value)
	{
		Variant* globalAttribute = nullptr;
		auto& it = this->globalAttributesMap.find(attributeName);
		if (it != this->globalAttributesMap.cend())
		{
			globalAttribute = it->second;
			globalAttribute->setValue(value);
		}
		else
		{
			globalAttribute = new Variant(attributeName);
			globalAttribute->setValue(value);
			this->globalAttributesMap.emplace(attributeName, globalAttribute);
		}
		return globalAttribute;
	}

	Variant* GameProgressModule::setGlobalStringValue(const Ogre::String& attributeName, Ogre::String value)
	{
		Variant* globalAttribute = nullptr;
		auto& it = this->globalAttributesMap.find(attributeName);
		if (it != this->globalAttributesMap.cend())
		{
			globalAttribute = it->second;
			globalAttribute->setValue(value);
		}
		else
		{
			globalAttribute = new Variant(attributeName);
			globalAttribute->setValue(value);
			this->globalAttributesMap.emplace(attributeName, globalAttribute);
		}
		return globalAttribute;
	}

	Variant* GameProgressModule::setGlobalVector2Value(const Ogre::String& attributeName, Ogre::Vector2 value)
	{
		Variant* globalAttribute = nullptr;
		auto& it = this->globalAttributesMap.find(attributeName);
		if (it != this->globalAttributesMap.cend())
		{
			globalAttribute = it->second;
			globalAttribute->setValue(value);
		}
		else
		{
			globalAttribute = new Variant(attributeName);
			globalAttribute->setValue(value);
			this->globalAttributesMap.emplace(attributeName, globalAttribute);
		}
		return globalAttribute;
	}

	Variant* GameProgressModule::setGlobalVector3Value(const Ogre::String& attributeName, Ogre::Vector3 value)
	{
		Variant* globalAttribute = nullptr;
		auto& it = this->globalAttributesMap.find(attributeName);
		if (it != this->globalAttributesMap.cend())
		{
			globalAttribute = it->second;
			globalAttribute->setValue(value);
		}
		else
		{
			globalAttribute = new Variant(attributeName);
			globalAttribute->setValue(value);
			this->globalAttributesMap.emplace(attributeName, globalAttribute);
		}
		return globalAttribute;
	}

	Variant* GameProgressModule::setGlobalVector4Value(const Ogre::String& attributeName, Ogre::Vector4 value)
	{
		Variant* globalAttribute = nullptr;
		auto& it = this->globalAttributesMap.find(attributeName);
		if (it != this->globalAttributesMap.cend())
		{
			globalAttribute = it->second;
			globalAttribute->setValue(value);
		}
		else
		{
			globalAttribute = new Variant(attributeName);
			globalAttribute->setValue(value);
			this->globalAttributesMap.emplace(attributeName, globalAttribute);
		}
		return globalAttribute;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	GameProgressModule::WorldData::WorldData(const Ogre::String& stateName)
		: worldName(worldName),
		exitDirection(Ogre::Vector2::ZERO),
		startPosition(Ogre::Vector3::ZERO),
		predecessorWorldData(nullptr)
	{

	}

	GameProgressModule::WorldData::WorldData(const Ogre::String& worldName, const Ogre::String& targetLocationName,
		const Ogre::Vector2& exitDirection, const Ogre::Vector3& startPosition, bool xyAxis)
		: worldName(worldName),
		targetLocationName(targetLocationName),
		exitDirection(exitDirection),
		startPosition(startPosition),
		xyAxis(xyAxis),
		predecessorWorldData(nullptr)
	{

	}

	void GameProgressModule::WorldData::addReachableWorld(const Ogre::String& reachableWorldName)
	{
		bool foundWorld = false;
		for (size_t i = 0; i < this->reachableWorlds.size(); i++)
		{
			if (reachableWorldName == this->reachableWorlds[i])
			{
				foundWorld = true;
			}
		}
		if (false == foundWorld)
			this->reachableWorlds.emplace_back(reachableWorldName);
	}

	std::vector<Ogre::String>& GameProgressModule::WorldData::getReachableWorlds(void)
	{
		return this->reachableWorlds;
	}

	void GameProgressModule::WorldData::setExitDirection(const Ogre::Vector2& exitDirection)
	{
		this->exitDirection = exitDirection;
	}

	Ogre::Vector2 GameProgressModule::WorldData::getExitDirection(void) const
	{
		return this->exitDirection;
	}

	void GameProgressModule::WorldData::setStartPosition(const Ogre::Vector3& startPosition)
	{
		this->startPosition = startPosition;
	}

	Ogre::Vector3 GameProgressModule::WorldData::getStartPosition(void) const
	{
		return this->startPosition;
	}

	void GameProgressModule::WorldData::setUseXYAxis(bool xyAxis)
	{
		this->xyAxis = xyAxis;
	}

	bool GameProgressModule::WorldData::getIsXYAxisUsed(void) const
	{
		return this->xyAxis;
	}

	GameProgressModule::WorldData* GameProgressModule::WorldData::getPredecessorWorldData(void) const
	{
		return this->predecessorWorldData;
	}

	void GameProgressModule::WorldData::setWorldName(const Ogre::String& worldName)
	{
		this->worldName = worldName;
	}

	Ogre::String GameProgressModule::WorldData::getWorldName(void) const
	{
		return this->worldName;
	}

	Ogre::String GameProgressModule::WorldData::getTargetLocationName(void) const
	{
		return this->targetLocationName;
	}

} // namespace end
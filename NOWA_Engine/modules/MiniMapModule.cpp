#include "NOWAPrecompiled.h"
#include "MiniMapModule.h"
#include "gameobject/GameObjectController.h"
#include "main/Core.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	MiniMapModule::MiniMapModule(const Ogre::String& appStateName)
		: appStateName(appStateName),
		sceneManager(nullptr)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MiniMapModule] Module created");
	}

	MiniMapModule::~MiniMapModule()
	{

	}
	
	void MiniMapModule::destroyContent(void)
	{
		this->sceneManager = nullptr;
		this->resultMiniMapDataList.clear();
	}

	void MiniMapModule::calculatePositionsForChildren(const std::pair<Ogre::Vector2, Ogre::String>& exitData, bool xyAxis, const Ogre::Vector2& viewPortSize,
		MiniMapModule::MiniMapTile& parentMiniMapTile, std::vector<MiniMapData>& resultMiniMapDataList, std::vector<Ogre::String>& visitedScenes)
	{
		for (size_t i = 0; i < visitedScenes.size(); i++)
		{
			// If the scene has already been visited skip
			if (exitData.second + ".scene" == visitedScenes[i])
				return;
		}
		
		std::unique_ptr<DotSceneImportModule> dotSceneImportModulePtr = std::make_unique<DotSceneImportModule>(
			DotSceneImportModule(this->sceneManager, Core::getSingletonPtr()->getProjectName(), exitData.second + ".scene", "Projects"));

		auto bounds = dotSceneImportModulePtr->parseBounds();

		if (bounds.first.x != Ogre::Math::POS_INFINITY)
		{
			Ogre::Vector2 size = Ogre::Vector2::ZERO;
			Ogre::Vector2 origin = Ogre::Vector2::ZERO;

			if (true == xyAxis)
			{
				size.x = Ogre::Math::Abs(bounds.second.x - bounds.first.x);
				size.y = Ogre::Math::Abs(bounds.second.y - bounds.first.y);
				size /= viewPortSize;
				origin = Ogre::Vector2(bounds.first.x, bounds.first.y);
				origin /= viewPortSize;
			}
			else
			{
				size.x = Ogre::Math::Abs(bounds.second.z - bounds.first.z);
				size.y = Ogre::Math::Abs(bounds.second.y - bounds.first.y);
				size /= viewPortSize;
				origin = Ogre::Vector2(bounds.first.x, bounds.first.z);
				origin /= viewPortSize;
			}

			MiniMapModule::MiniMapTile miniMapTile;

			miniMapTile.sceneName = exitData.second;
			miniMapTile.size = size;
			miniMapTile.entryDirection = exitData.first;
			parentMiniMapTile.children.emplace_back(miniMapTile);
			miniMapTile.parentMiniMapTile = &parentMiniMapTile;

			// Calculate the position depending on the parent mini map tile position and size and the entry direction
			miniMapTile.position.x = parentMiniMapTile.position.x + (parentMiniMapTile.size.x * miniMapTile.entryDirection.x);

			// Note for y: take negative entry direction, because x is growing from left to right, but y is growing from top to bottom (down instead up)
			miniMapTile.position.y = parentMiniMapTile.position.y + (parentMiniMapTile.size.y * -miniMapTile.entryDirection.y);

			// Add to visited scenes
			visitedScenes.emplace_back(exitData.second + ".scene");

			// Add to flat result mini map data list
			MiniMapModule::MiniMapData resultMiniMapData;
			resultMiniMapData.size = miniMapTile.size;
			resultMiniMapData.position = miniMapTile.position;
			resultMiniMapData.origin = origin;
			resultMiniMapData.sceneName = miniMapTile.sceneName;
			

			resultMiniMapDataList.emplace_back(resultMiniMapData);

			std::vector<std::pair<Ogre::Vector2, Ogre::String>> exitData = dotSceneImportModulePtr->parseExitDirectionsNextWorlds();

			for (int i = 0; i < exitData.size(); i++)
			{
				this->calculatePositionsForChildren(exitData[i], xyAxis, viewPortSize, miniMapTile, resultMiniMapDataList, visitedScenes);
			}
		}
		else
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[MiniMapModule] Could not parse bounds, because none are set so far for scene: '" + exitData.second + "'");
		}
	}

	std::vector<MiniMapModule::MiniMapData> MiniMapModule::parseMinimaps(Ogre::SceneManager* sceneManager, bool xyAxis, const Ogre::Vector2& startPosition, 
		const Ogre::Vector2& viewPortSize)
	{
		this->resultMiniMapDataList.clear();

		if (0.0f == viewPortSize.x || 0.0f == viewPortSize.y)
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[MiniMapModule] Could not create mini map because the view port size is zero!");
			return this->resultMiniMapDataList;
		}
		
		// Only parse once
		// if (nullptr == this->sceneManager)
		{
			this->sceneManager = sceneManager;

			std::vector<Ogre::String> sceneFileNames = Core::getSingletonPtr()->getSceneFileNames("Projects", Core::getSingletonPtr()->getProjectName());
			
			auto itr = std::find(sceneFileNames.begin(), sceneFileNames.end(), "global.scene");
			if (itr != sceneFileNames.end())
				sceneFileNames.erase(itr);

			if (true == sceneFileNames.empty())
				return resultMiniMapDataList;

			std::unique_ptr<DotSceneImportModule> dotSceneImportModulePtr = std::make_unique<DotSceneImportModule>(
				DotSceneImportModule(this->sceneManager, Core::getSingletonPtr()->getProjectName(), sceneFileNames[0], "Projects"));

			auto bounds = dotSceneImportModulePtr->parseBounds();

			if (bounds.first.x != Ogre::Math::POS_INFINITY)
			{
				// Gather and prepare data

				Ogre::Vector2 size = Ogre::Vector2::ZERO;
				Ogre::Vector2 origin = Ogre::Vector2::ZERO;

				if (true == xyAxis)
				{
					size.x = Ogre::Math::Abs(bounds.second.x - bounds.first.x);
					size.y = Ogre::Math::Abs(bounds.second.y - bounds.first.y);
					size /= viewPortSize;
					origin = Ogre::Vector2(bounds.first.x, bounds.first.y);
					origin /= viewPortSize;
				}
				else
				{
					size.x = Ogre::Math::Abs(bounds.second.z - bounds.first.z);
					size.y = Ogre::Math::Abs(bounds.second.y - bounds.first.y);
					size /= viewPortSize;
					origin = Ogre::Vector2(bounds.first.x, bounds.first.z);
					origin /= viewPortSize;
				}

				MiniMapModule::MiniMapTile miniMapTile;

				Ogre::String sceneName = sceneFileNames[0];
				size_t dot = sceneName.rfind(".scene");
				if (dot != std::string::npos)
				{
					sceneName.resize(dot);
				}

				miniMapTile.sceneName = sceneName;
				miniMapTile.size = size;
				miniMapTile.position = startPosition;

				// Add to flat result mini map data list
				MiniMapModule::MiniMapData resultMiniMapData;
				resultMiniMapData.size = miniMapTile.size;
				resultMiniMapData.position = miniMapTile.position;
				resultMiniMapData.origin = origin;
				resultMiniMapData.sceneName = miniMapTile.sceneName;
				resultMiniMapDataList.emplace_back(resultMiniMapData);

				std::vector<std::pair<Ogre::Vector2, Ogre::String>> exitData = dotSceneImportModulePtr->parseExitDirectionsNextWorlds();
				
				// Calculate the position for each world in contrast to its neighbour worlds

				std::vector<Ogre::String> visitedScenes;
				visitedScenes.emplace_back(sceneFileNames[0]);

				for (int i = 0; i < exitData.size(); i++)
				{
					this->calculatePositionsForChildren(exitData[i], xyAxis, viewPortSize, miniMapTile, resultMiniMapDataList, visitedScenes);
				}
			}
			else
			{
				Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[MiniMapModule] Could not parse bounds, because none are set so far for scene: '" + sceneFileNames[0] + "'");
			}
		}
		return resultMiniMapDataList;
	}

	std::pair<bool, Ogre::Vector2> MiniMapModule::parseGameObjectMinimapPosition(const Ogre::String& sceneName, unsigned long id, bool xyAxis, const Ogre::Vector2& viewPortSize)
	{
		bool success = false;
		Ogre::Vector2 resultPosition = Ogre::Vector2::ZERO;

		// e.g. 522345323
		if (true == sceneName.empty())
		{
			auto gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(id);
			if (nullptr != gameObjectPtr)
			{
				auto data = this->calculateGameObjectPosition(gameObjectPtr->getPosition(), xyAxis, viewPortSize);
				success = data.first;
				resultPosition = data.second;
			}
		}
		else
		{
			// e.g. scene4:522345323
			std::unique_ptr<DotSceneImportModule> dotSceneImportModulePtr = std::make_unique<DotSceneImportModule>(
				DotSceneImportModule(this->sceneManager, Core::getSingletonPtr()->getProjectName(), sceneName + ".scene", "Projects"));
			auto positionData = dotSceneImportModulePtr->parseGameObjectPosition(id);
			if (true == positionData.first)
			{
				auto data = this->calculateGameObjectPosition(positionData.second, xyAxis, viewPortSize);
				success = data.first;
				resultPosition = data.second;
			}
		}

		return std::make_pair(success, resultPosition);
	}

	std::pair<bool, Ogre::Vector2> MiniMapModule::calculateGameObjectPosition(const Ogre::Vector3& position, bool xyAxis, const Ogre::Vector2 & viewPortSize)
	{
		bool success = false;

		// Get the current world
		Ogre::String currentWorld = Core::getSingletonPtr()->getWorldName();
		
		// Found global id like player, get its current position
		Ogre::Vector3 currentPosition = position;

		Ogre::Vector2 miniMapPosition = Ogre::Vector2::ZERO;

		miniMapPosition = Ogre::Vector2(currentPosition.x, xyAxis ? currentPosition.y : currentPosition.z);
		miniMapPosition /= viewPortSize;

		Ogre::Vector2 currentWorldPosition = Ogre::Vector2::ZERO;
		Ogre::Vector2 currentWorldSize = Ogre::Vector2::ZERO;
		Ogre::Vector2 currentWorldOrigin = Ogre::Vector2::ZERO;

		for (size_t i = 0; i < this->resultMiniMapDataList.size(); i++)
		{
			if (this->resultMiniMapDataList[i].sceneName == currentWorld)
			{
				currentWorldPosition = this->resultMiniMapDataList[i].position;
				currentWorldSize = this->resultMiniMapDataList[i].size;
				currentWorldOrigin = this->resultMiniMapDataList[i].origin;
				break;
			}
		}

		/*
		E.g.	Player.x = -11
				Level.x1 = -21
				Level.x2 = 157
				Level.w = 178
				Player.Level.x = Player.x - Level.x


		*/
		if (Ogre::Vector2::ZERO != currentWorldSize)
		{
			miniMapPosition.x = (miniMapPosition.x - currentWorldOrigin.x) + currentWorldPosition.x;
			miniMapPosition.y = (currentWorldOrigin.y - miniMapPosition.y) + currentWorldPosition.y + currentWorldSize.y;
			success = true;
		}

		return std::make_pair(success, miniMapPosition);
	}

} // namespace end
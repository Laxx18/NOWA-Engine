#ifndef MINI_MAP_MODULE_H
#define MINI_MAP_MODULE_H

#include "defines.h"
#include <vector>
#include "DotSceneImportModule.h"
#include "OgreVector2.h"

namespace NOWA
{
	class Ogre::Vector2;

	class EXPORTED MiniMapModule
	{
	public:
		friend class ExitComponent;
		friend class AppState; // Only AppState may create this class

		struct MiniMapData
		{
			MiniMapData()
				: size(Ogre::Vector2::ZERO),
				origin(Ogre::Vector2::ZERO)
			{

			}

			void reset(void)
			{
				this->sceneName = "";
				this->size = Ogre::Vector2::ZERO;
				this->origin = Ogre::Vector2::ZERO;
			}

			Ogre::String sceneName;
			Ogre::Vector2 position;
			Ogre::Vector2 size; // x = width, y = height, depending on bounds calculation axis
			Ogre::Vector2 origin;
		};

		struct MiniMapTile
		{
			MiniMapTile()
				: size(Ogre::Vector2::ZERO),
				position(Ogre::Vector2::ZERO),
				entryDirection(Ogre::Vector2::ZERO),
				parentMiniMapTile(nullptr)
			{

			}

			Ogre::String sceneName;
			Ogre::Vector2 position;
			Ogre::Vector2 size;
			Ogre::Vector2 entryDirection;

			MiniMapTile* parentMiniMapTile;

			std::vector<MiniMapTile> children;
		};
	public:
		void destroyContent(void);
		
		std::vector<MiniMapData> parseMinimaps(Ogre::SceneManager* sceneManager, bool xyAxis, const Ogre::Vector2& startPosition, const Ogre::Vector2& viewPortSize);
	
		std::pair<bool, Ogre::Vector2> parseGameObjectMinimapPosition(const Ogre::String& sceneName, unsigned long id, bool xyAxis, const Ogre::Vector2& viewPortSize);
	private:
		void calculatePositionsForChildren(const std::pair<Ogre::Vector2, Ogre::String>& exitData, bool xyAxis, const Ogre::Vector2& viewPortSize,
			MiniMapModule::MiniMapTile& parentMiniMapTile, std::vector<MiniMapData>& resultMiniMapDataList, std::vector<Ogre::String>& visitedScenes);
		std::pair<bool, Ogre::Vector2> calculateGameObjectPosition(const Ogre::Vector3& position, bool xyAxis, const Ogre::Vector2& viewPortSize);
	private:
		MiniMapModule(const Ogre::String& appStateName);
		~MiniMapModule();
	private:
		Ogre::String appStateName;
		Ogre::SceneManager* sceneManager;
		std::vector<MiniMapData> resultMiniMapDataList;
	};

}; //namespace end

#endif

#ifndef GAME_PROGRESS_MODULE_H
#define GAME_PROGRESS_MODULE_H

#include "defines.h"
#include <map>
#include <vector>
#include "DotSceneImportModule.h"
#include "DotSceneExportModule.h"

namespace NOWA
{
	class GameObject;

	class EXPORTED GameProgressModule
	{
	public:
		friend class ExitComponent;
		friend class LoadWorldProcess;
		friend class AppState; // Only AppState may create this class
	private:
		class WorldData
		{
		public:
			WorldData(const Ogre::String& worldName);

			WorldData(const Ogre::String& worldName, const Ogre::String& targetLocationName,
				const Ogre::Vector2& exitDirection, const Ogre::Vector3& startPosition, bool xyAxis);

			Ogre::String getWorldName(void) const;

			Ogre::String getTargetLocationName(void) const;

			std::vector<Ogre::String>& getReachableWorlds(void);

			Ogre::Vector2 getExitDirection(void) const;

			Ogre::Vector3 getStartPosition(void) const;

			bool getIsXYAxisUsed(void) const;

			WorldData* getPredecessorWorldData(void) const;

		private:
			friend class GameProgressModule;

			void setWorldName(const Ogre::String& appStateName);

			void addReachableWorld(const Ogre::String& reachableWorldName);

			void setExitDirection(const Ogre::Vector2& exitDirection);

			void setStartPosition(const Ogre::Vector3& startPosition);

			void setUseXYAxis(bool xyAxis);
		private:
			Ogre::String worldName;
			Ogre::String targetLocationName;
			std::vector<Ogre::String> reachableWorlds;
			Ogre::Vector2 exitDirection;
			Ogre::Vector3 startPosition;
			bool xyAxis;
			WorldData* predecessorWorldData;
		};
	public:
		void destroyContent(void);

		void resetContent(void);
		
		void start(void);
		
		void stop(void);

		void init(Ogre::SceneManager* sceneManager);

		void addWorld(const Ogre::String& currentWorldName, const Ogre::String& reachableWorldName, 
			const Ogre::String& targetLocationName, const Ogre::Vector2& exitDirection, const Ogre::Vector3& startPosition, bool xyAxis);

		std::vector<Ogre::String>* getReachableWorlds(const Ogre::String& currentWorldName);

		WorldData* getPredecessorWorldData(const Ogre::String& currentWorldName);

		WorldData* getWorldData(const Ogre::String& currentWorldName);

		unsigned int getWorldsCount(void) const;

		Ogre::String getPlayerName(void) const;

		void determinePlayerStartLocation(const Ogre::String& currentWorldName);

		/**
		* @brief		Loads the given world.
		* @param[in]	worldName The world to load. If the world does not exist, or there is an internal error during load, an exception will be thrown.
		* @note			In order to get data like mainCamera, sunLight etc. after world has been loaded, use NOWA::EventDataWorldLoaded event.
		*				In that event call NOWA::AppStateManager::getSingletonptr()->getGameProgressModule()->getData() ... because the world will not be loaded immediately but in the next frame, so that object may be finished in the current frame.
		*/
		void loadWorld(const Ogre::String& worldName);

		void loadWorldShowProgress(const Ogre::String& worldName);

		/**
		* @brief		Gets the current world name.
		* @return		The world name
		*/
		Ogre::String getCurrentWorldName(void);
		
		// This values are dependant of game objects and attributes components

	   /**
		* @brief		Saves the current progress for all game objects with its attribute components. 
		* @param[in]	saveName The save name to set.
		* @param[in]	crypted			Optionally crypts the content, so that it is not readable anymore.
		* @param[in]	sceneSnapshot	Optionally whether to save also a snapshot of the current scene.
		*/
		void saveProgress(const Ogre::String& saveName, bool crypted, bool sceneSnapshot = false);
		
	   /**
		* @brief		Saves all values for the given game object id and its attribute components and its attribute index.
		* @param[in]	saveName	 The save name to set.
		* @param[in]	gameObjectId The game object id to set.
		* @param[in]	crypted	 Optionally crypts the content, so that it is not readable anymore.
		*/
		void saveValues(const Ogre::String& saveName, unsigned long gameObjectId, bool crypted);
		
		/**
		* @brief		Saves a value for the given game object id.
		* @param[in]	saveName		The save name to set.
		* @param[in]	gameObjectId	The game object id to set.
		* @param[in]	attributeIndex	The attribute index to set.
		* @param[in]	crypted	 Optionally crypts the content, so that it is not readable anymore.
		*/
		void saveValue(const Ogre::String& saveName, unsigned long gameObjectId, unsigned int attributeIndex, bool crypted);
		
	   /**
		* @brief		Loads all values for all game objects with attributes components for the given save name.
		* @param[in]	saveName The save name to set.
		* @param[in]	sceneSnapshot	Optionally whether to load also a snapshot of the current scene.
		* @return		success	 Whether the progress could be loaded (file does exist).
		*/
		bool loadProgress(const Ogre::String& saveName, bool sceneSnapshot = false);
		
	   /**
		* @brief		Loads all values for the given game object id and the given save name.
		* @param[in]	saveName The save name to set.
		* @return		success	 Whether the progress could be loaded (file does exist).
		*/
		bool loadValues(const Ogre::String& saveName, unsigned long gameObjectId);
		
		/**
		* @brief		Loads a value for the given game object id and attribute index and the given save name.
		* @param[in]	saveName The save name to set.
		* @return		success	 Whether the progress could be loaded (file does exist).
		*/
		bool loadValue(const Ogre::String& saveName, unsigned long gameObjectId, unsigned int attributeIndex);
		
		// This values are user values and can be created in lua script directly without the need of attributes component
		/*void saveUserValue(const Ogre::String& saveName, const Ogre::String& attributeName);
		
		void saveUserValue(const Ogre::String& saveName, Variant* userAttribute);
		
		void saveUserValues(const Ogre::String& saveName);
		
		Variant* loadUserValue(const Ogre::String& saveName, const Ogre::String& attributeName);
		
		bool loadUserValues(const Ogre::String& saveName);*/
		
	   /**
		* @brief		Gets the Variant global value from attribute name.
		* @note			Global values are stored directly in GameProgressModule. They can be used for the whole game logic, like which boss has been defeated etc.
		* @param[in]	attributeName The attribute name to set.
		* @return		variant	 The variant containing the current value.
		*/
		Variant* getGlobalValue(const Ogre::String& attributeName);
		
		/**
		* @brief		Sets the bool value for the given attribute name and returns the global Variant.
		* @note			Global values are stored directly in GameProgressModule. They can be used for the whole game logic, like which boss has been defeated etc.
		* @param[in]	attributeName The attribute name to set.
		* @return		variant	 The variant containing the current value.
		*/
		Variant* setGlobalBoolValue(const Ogre::String& attributeName, bool value);
		
		Variant* setGlobalIntValue(const Ogre::String& attributeName, int value);
		
		Variant* setGlobalUIntValue(const Ogre::String& attributeName, unsigned int value);
		
		Variant* setGlobalULongValue(const Ogre::String& attributeName, unsigned long value);
		
		Variant* setGlobalRealValue(const Ogre::String& attributeName, Ogre::Real value);
		
		Variant* setGlobalStringValue(const Ogre::String& attributeName, Ogre::String value);
		
		Variant* setGlobalVector2Value(const Ogre::String& attributeName, Ogre::Vector2 value);
		
		Variant* setGlobalVector3Value(const Ogre::String& attributeName, Ogre::Vector3 value);
		
		Variant* setGlobalVector4Value(const Ogre::String& attributeName, Ogre::Vector4 value);

		// Does not work yet
		// Variant* setGlobalListValue(const Ogre::String& attributeName, const std::vector<Ogre::String>& list);

	   /**
		* @brief		Changes the current world to the new specified one.
		* @param[in]	worldName The world name set.
		*/
		void changeWorld(const Ogre::String& worldName);

	   /**
		* @brief		Changes the current world to the new specified one. Also shows the load worlding progress.
		* @param[in]	worldName The world name set.
		*/
		void changeWorldShowProgress(const Ogre::String& worldName);

		/**
		 * @brief		Stalls the updates, when a world is changed, because it takes some time and all pointers like camera etc. will be exchanged during runtime.
		 */
		bool stallUpdates(void);
	private:
		GameProgressModule(const Ogre::String& appStateName);
		~GameProgressModule();

		void setPlayerName(const Ogre::String& playerName);
	private:
		Ogre::String getSaveFileContent(const Ogre::String& saveName);
	private:
		Ogre::String appStateName;

		std::map<Ogre::String, WorldData> worldMap;
		GameObject* player;
		
		DotSceneImportModule* dotSceneImportModule;
		DotSceneExportModule* dotSceneExportModule;

		Ogre::SceneManager* sceneManager;

		Ogre::String saveName;
		Ogre::String userSaveName;
		// For user defined attributes, without the need for a game objects attributes component
		std::map<Ogre::String, Variant*> globalAttributesMap;

		Ogre::String currentWorldName;

		bool bStallUpdates;
		
	};

}; //namespace end

#endif

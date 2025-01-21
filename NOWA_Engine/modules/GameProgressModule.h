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
		friend class LoadSceneProcess;
		friend class LoadProgressProcess;
		friend class AppState; // Only AppState may create this class
	private:
		class SceneData
		{
		public:
			SceneData(const Ogre::String& sceneName);

			SceneData(const Ogre::String& sceneName, const Ogre::String& targetLocationName,
				const Ogre::Vector2& exitDirection, const Ogre::Vector3& startPosition, bool xyAxis);

			Ogre::String getSceneName(void) const;

			Ogre::String getTargetLocationName(void) const;

			std::vector<Ogre::String>& getReachableScenes(void);

			Ogre::Vector2 getExitDirection(void) const;

			Ogre::Vector3 getStartPosition(void) const;

			bool getIsXYAxisUsed(void) const;

			SceneData* getPredecessorSceneData(void) const;

		private:
			friend class GameProgressModule;

			void setSceneName(const Ogre::String& appStateName);

			void addReachableScene(const Ogre::String& reachableSceneName);

			void setExitDirection(const Ogre::Vector2& exitDirection);

			void setStartPosition(const Ogre::Vector3& startPosition);

			void setUseXYAxis(bool xyAxis);
		private:
			Ogre::String sceneName;
			Ogre::String targetLocationName;
			std::vector<Ogre::String> reachableScenes;
			Ogre::Vector2 exitDirection;
			Ogre::Vector3 startPosition;
			bool xyAxis;
			SceneData* predecessorSceneData;
		};
	public:
		void destroyContent(void);

		void resetContent(void);
		
		void start(void);
		
		void stop(void);

		void init(Ogre::SceneManager* sceneManager);

		void addScene(const Ogre::String& currentSceneName, const Ogre::String& reachableSceneName, 
			const Ogre::String& targetLocationName, const Ogre::Vector2& exitDirection, const Ogre::Vector3& startPosition, bool xyAxis);

		std::vector<Ogre::String>* getReachableScenes(const Ogre::String& currentSceneName);

		SceneData* getPredecessorSceneData(const Ogre::String& currentSceneName);

		SceneData* getSceneData(const Ogre::String& currentSceneName);

		unsigned int getScenesCount(void) const;

		Ogre::String getPlayerName(void) const;

		void determinePlayerStartLocation(const Ogre::String& currentSceneName);

		/**
		* @brief		Loads the given scene.
		* @param[in]	sceneName The scene to load. If the scene does not exist, or there is an internal error during load, an exception will be thrown.
		* @note			In order to get data like mainCamera, sunLight etc. after scene has been loaded, use NOWA::EventDataSceneLoaded event.
		*				In that event call NOWA::AppStateManager::getSingletonptr()->getGameProgressModule()->getData() ... because the scene will not be loaded immediately but in the next frame, so that object may be finished in the current frame.
		*/
		void loadScene(const Ogre::String& sceneName);

		void loadSceneShowProgress(const Ogre::String& sceneName);

		/**
		* @brief		Gets the current scene name.
		* @return		The scene name
		*/
		Ogre::String getCurrentSceneName(void);
		
		// This values are dependant of game objects and attributes components

	   /**
		* @brief		Saves the current progress for all game objects with its attribute components. 
		* @param[in]	saveName The save name to set.
		* @param[in]	crypted			Optionally crypts the content, so that it is not readable anymore.
		* @param[in]	sceneSnapshot	Optionally whether to save also a snapshot of the current scene.
		* @details		Note: The saved game is saved as scene and all global gameobjects are also packed in to the saved scene folder!
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
		* @details		If AttributeComponents are used, those values are set after the scene snapshot has been loaded, so they have most priority.
		*/
		bool loadProgress(const Ogre::String& saveName, bool sceneSnapshot = false, bool showProgress = false);
		
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
		* @brief		Changes the current scene to the new specified one.
		* @param[in]	sceneName The scene name set.
		*/
		void changeScene(const Ogre::String& sceneName);

	   /**
		* @brief		Changes the current scene to the new specified one. Also shows the load scene progress.
		* @param[in]	sceneName The scene name set.
		*/
		void changeSceneShowProgress(const Ogre::String& sceneName);

		/**
		 * @brief		Gets whether the state is in the middle of scene loading.
		 */
		bool isSceneLoading(void) const;

		/**
		 * @brief		Gets the current (for this scene) scene manager, or null if not existing.
		 */
		Ogre::SceneManager* getCurrentSceneManager(void);
	private:
		GameProgressModule(const Ogre::String& appStateName);
		~GameProgressModule();

		void setPlayerName(const Ogre::String& playerName);

		bool internalReadGlobalAttributes(const Ogre::String& globalAttributesStream);

		void setIsSceneLoading(bool bSceneLoading);
	private:
		std::pair<bool, Ogre::String> getSaveFileContent(const Ogre::String& saveName);
	private:
		Ogre::String appStateName;

		std::map<Ogre::String, SceneData> sceneMap;
		GameObject* player;
		
		DotSceneImportModule* dotSceneImportModule;
		DotSceneExportModule* dotSceneExportModule;

		Ogre::SceneManager* sceneManager;

		Ogre::String saveName;
		Ogre::String userSaveName;
		// For user defined attributes, without the need for a game objects attributes component
		std::map<Ogre::String, Variant*> globalAttributesMap;

		Ogre::String currentSceneName;

		bool bSceneLoading;
		
	};

}; //namespace end

#endif

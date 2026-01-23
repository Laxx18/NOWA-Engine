#ifndef APP_STATE_MANAGER_H
#define APP_STATE_MANAGER_H

#include "AppState.h"
#include "defines.h"
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>

#include "utilities/concurrentqueue.h"

namespace NOWA
{
	/**
	* @class AppStateManager
	* @brief The application state manager, manages prior created appliction states. It starts the game loop and starts with an state.
	*/
	class EXPORTED AppStateManager : public Ogre::Singleton<AppStateManager>, public AppStateListener
	{
	public:
		friend class ChangeAppStateProcess;
		friend class GameProgressModule;

		typedef struct
		{
			Ogre::String name;
			AppState* state;
		} StateInfo;

		using LogicCommand = std::function<void()>;

		AppStateManager();
		~AppStateManager();

		void manageAppState(Ogre::String stateName, AppState* state);

		/*
		 * @brief	Finds the app state by the given name.
		 * @param[in] stateName			The state name to set.
		 * @return	The AppState if found, or nullptr if state name does not exist.
		 */
		AppState* findByName(const Ogre::String& stateName);

		AppState* getNextState(AppState* currentAppState);

		/**
		* @brief		Starts the simulation with the given application state name. This includes starting the game loop etc.
		* @param[in]	stateName	The application state name to start.
		* @param[in]	renderWhenInactive	Whether to render the graphics scene even if the window is not active e.g.in task bar.This can be useful if creating a network scenario
		*									with several instances on one monitor and the developer wants to switch to a different application instance to e.g. move an object and watch
		*									whats happening on the other application scenes
		*/
		void start(const Ogre::String& stateName, bool renderWhenInactive = false);

		void changeAppState(AppState* state);
		bool pushAppState(AppState* state);
		void popAppState(void);
		void popAllAndPushAppState(AppState* state);

		void shutdown(void);

		void changeAppState(const Ogre::String& stateName);
		bool pushAppState(const Ogre::String& stateName);
		void popAllAndPushAppState(const Ogre::String& stateName);

		/**
		 * @brief		Gets whether the given AppState has started.
		 * @param[in]	stateName	The application state name to check.
		 * @return 		hasStarted true, if the AppState has started, else false.
		 * @note		This can be used to ask from another AppState like a MenuState, whether a GameState has already started, so that e.g. a continue button can be shown in the Menu.
		 */
		bool hasAppStateStarted(const Ogre::String& stateName);

		/**
		 * @brief		Gets whether the simulation is in the given app state.
		 * @param[in]	stateName	The application state name to check.
		 * @return 		true, if the simulation is in the given app state, else false.
		 * @note		This can be used to ask if the simulation e.g. is in a MenuState, in order to remove some behavior from global game objects. This could be necessary if global game objects need different behavior e.g. in a GameState as in a MenuState.
		 */
		bool isInAppstate(const Ogre::String& stateName);

		Ogre::String getCurrentAppStateName(void) const;

		AppState* getCurrentAppState(void) const;

		/*
		 * @brief	Sets slow motion in milliseconds
		 * @param[in] slowMotionMS			The slowmotion to set in milliseconds (default is 0)
		 */
		void setSlowMotion(unsigned int slowMotionMS);

		/*
		* @brief	Update the engine by the specified desired updates.
		* @Note	This function is clamped to desiredUpdates.
		* @param[in] desiredUpdates		The desired updates per second (default is screen vsync)
		*/
		void setDesiredUpdates(unsigned int desiredUpdates);

		/**
		 * @brief		Exits the game and destroys all application states.
		 */
		void exitGame(void);

		/*
		 * @brief	Gets whether to render the scene, even the render window is not active (not visible).
		 * @note	Usefull for network scenario testing, in which several instances (clients) are opened.
		 * @return renderWhenInactive
		 */
		bool getRenderWhenInactive(void) const;

		// Rendering thread should wait until logic frame is ready
		void waitForLogicFrameFinish(void);

		size_t getAppStatesCount(void) const;

		GameObjectController* getGameObjectController(void) const;

		GameProgressModule* getGameProgressModule(void) const;

		RakNetModule* getRakNetModule(void) const;

		MiniMapModule* getMiniMapModule(void) const;

		OgreNewtModule* getOgreNewtModule(void) const;

		MeshDecalGeneratorModule* getMeshDecalGeneratorModule(void) const;

		CameraManager* getCameraManager(void) const;

		OgreRecastModule* getOgreRecastModule(void) const;

		ParticleUniverseModule* getParticleUniverseModule(void) const;

		LuaScriptModule* getLuaScriptModule(void) const;

		EventManager* getEventManager(void) const;

		ScriptEventManager* getScriptEventManager(void) const;

		GameObjectController* getGameObjectController(const Ogre::String& stateName);

		GameProgressModule* getGameProgressModule(const Ogre::String& stateName);

		RakNetModule* getRakNetModule(const Ogre::String& stateName);

		MiniMapModule* getMiniMapModule(const Ogre::String& stateName);

		OgreNewtModule* getOgreNewtModule(const Ogre::String& stateName);

		MeshDecalGeneratorModule* getMeshDecalGeneratorModule(const Ogre::String& stateName);

		CameraManager* getCameraManager(const Ogre::String& stateName);

		OgreRecastModule* getOgreRecastModule(const Ogre::String& stateName);

		ParticleUniverseModule* getParticleUniverseModule(const Ogre::String& stateName);

		LuaScriptModule* getLuaScriptModule(const Ogre::String& stateName);

		EventManager* getEventManager(const Ogre::String& stateName);

		ScriptEventManager* getScriptEventManager(const Ogre::String& stateName);

		void enqueue(LogicCommand&& command);

		void enqueueAndWait(LogicCommand&& command);

		void clearLogicQueue(void);

		// Sets the logic thread ID
		void markCurrentThreadAsLogicThread(void);

		// Checks if we are on the render thread
		bool isLogicThread(void) const;
	public:
		/*
		* @brief	Gets whether this application has been shut down
		* @return bShutDown	True if the application has been shut down, else false
		*/
		std::atomic<bool> bShutdown;

		/*
		 * @brief	Gets whether the AppStateManager is stalled, due to loading another AppState.
		 * @return bStalled	True if the AppStateManager is busy, else false.
		 */
		std::atomic<bool> bStall;

		std::atomic<bool> bCanProcessRenderQueue;

		GameProgressModule* getActiveGameProgressModuleSafe(void) const;
	public:
		static AppStateManager& getSingleton(void);

		static AppStateManager* getSingletonPtr(void);

	private:
		void multiThreadedRendering(void);

		void internalChangeAppState(AppState* state, bool initial = false);
		bool internalPushAppState(AppState* state);
		void internalPopAppState(void);
		void internalPopAllAndPushAppState(AppState* state);
		void internalExitGame(void);

		// Signals that one logic frame is complete
		void signalLogicFrameFinished(void);

		// CRITICAL: This must be protected by a mutex if accessed by Logic/Main thread
		// when setting it, but is only read by the Render Thread.
		// Making it atomic ensures safe read/write access to the pointer itself.
		std::atomic<GameProgressModule*> activeGameProgressModule;

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

		bool internalReadGlobalAttributes(const Ogre::String& globalAttributesStream);

		void saveProgress(const Ogre::String& saveFilePathName, bool crypted, bool sceneSnapshot);

		void processAll(void);
	protected:
		void linkInputWithCore(AppState* oldState, AppState* state);
		std::vector<AppState*> activeStateStack;
		std::vector<StateInfo> states;
		bool renderWhenInactive;
		
		unsigned int desiredUpdates;
		unsigned int renderDelta;
		unsigned long lastTime;
		bool vsyncOn;

		unsigned int slowMotionMS;

		// For user defined attributes, without the need for a game objects attributes component
		std::map<Ogre::String, Variant*> globalAttributesMap;

		std::mutex logicFrameMutex;
		std::condition_variable logicFrameCondVar;
		bool logicFrameFinished = false;
		moodycamel::ConcurrentQueue<LogicCommand> queue;
		std::atomic<std::thread::id> logicThreadId;
	};

}; // namespace end

template<> NOWA::AppStateManager* Ogre::Singleton<NOWA::AppStateManager>::msSingleton = 0;

#endif

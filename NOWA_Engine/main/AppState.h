#ifndef APP_STATE_H
#define APP_STATE_H

#include "defines.h"

#include "MyGUI.h"
#include "MyGUI_Ogre2Platform.h"
#include "MessageBox/MessageBox.h"

#include "modules/WorkspaceModule.h"
#include "modules/OgreNewtModule.h"
#include "modules/OgreRecastModule.h"
#include "modules/RakNetModule.h"
#include "modules/OgreNewtModule.h"
#include "modules/MiniMapModule.h"
#include "modules/MeshDecalGeneratorModule.h"
#include "modules/GameProgressModule.h"
#include "modules/ParticleUniverseModule.h"
#include "modules/LuaScriptModule.h"
#include "modules/GraphicsModule.h"
#include "gameobject/GameObjectController.h"
#include "camera/CameraManager.h"
#include "main/EventManager.h"
#include "main/ScriptEventManager.h"

namespace NOWA
{
	class AppState;
	class WorkspaceBaseComponent;

	class EXPORTED AppStateListener
	{
	public:
		AppStateListener() {};

		virtual ~AppStateListener() {};
		virtual void manageAppState(Ogre::String stateName, AppState* appState) = 0;
		virtual AppState* findByName(const Ogre::String& stateName) = 0;
		virtual AppState* getNextState(AppState* currentAppState) = 0;
		virtual void changeAppState(AppState* appState) = 0;
		virtual bool pushAppState(AppState* appState) = 0;
		virtual void popAppState(void) = 0;
		virtual void shutdown(void) = 0;
		virtual void popAllAndPushAppState(AppState* appState) = 0;
	};

	/**
	 * @brief	This class represents an app state in which a whole scene is simulated.
	 */
	class EXPORTED AppState : public OIS::KeyListener, public OIS::MouseListener, public OIS::JoyStickListener
	{
	public:
		friend class AppStateListener;
		friend class AppStateManager;

		virtual ~AppState() {};

		/**
		 * @brief	Creates a new application state like (scene1 or level1 state)
		 * @note	Call the macro DECLARE_APPSTATE_CLASS in the header of the own application state. For example after the constructor: DECLARE_APPSTATE_CLASS(YourState)
		 *			After that call YourState::create(...) when the state should be registered
		 * @param	appStateManager The instance that manages all appstates and the gameloop
		 * @param	name The name of the application state
		 * @param	nextAppStateName The name of the successor of the current application state, may be ""
		 */
		static void create(AppStateListener* appStateManager, const Ogre::String name, const Ogre::String nextAppStateName);

		/**
		 * @brief	Can be used to init some variables and is called when the new application state gets entered
		 * @note	Init all variables in enter() and not in the constructor! Since the constructor is only called for the first enter.
		 *			When for example this application state is exited to go to the menue and then back to this application state, 
		 *			the constructor is not called!
		 */
		virtual void enter(void);

		/**
		 * @brief	Can be used to destroy all objects.
		 * @note	like in @enter() do not use the destructor do destroy some variables, 
		 *			because the destructor is only called if the whole application is quitted.
		 */
		virtual void exit(void);

		/**
		 * @brief	Must be used to when a world has been loaded in order to use scene manager, camera, ogrenewt etc. to add custom functionality.
		 * @param[in]		sceneParameter   The scene parameter to use to get information about the current project.
		 * @note							 If no scene has been loaded, there is no ogrenewt and user must create via @OgreNewtModule::createPhysics(...) its own physics instance.
		 */
		virtual void start(const NOWA::SceneParameter& sceneParameter) { };

		/**
		 * @brief		Can be used to update e.g. camera transform. Its called as often as graphics is rendered.
		 * @param[in]	dt The delta time in seconds. For example if the game runs with 700 fps.
		 */
		virtual void renderUpdate(Ogre::Real dt);

		/**
		 * @brief		Can be used to update all Objects and control logic. Updates also all modules and game object controller
		 * @param[in]	dt The delta time in seconds. For example if the game runs with 60 fps. 
		 *				The delta time between two update calls is 0.0016 sec.
		 * @note		When you overwrite the update() function, call in your update function of your state: AppState::update(dt); too,
		 *				because in the update() of the base application state are some updates of importent components executed, 
		 *				like the ProcessManager to handle own processes.
		 */
		virtual void update(Ogre::Real dt);

		/**
		 * @brief	Pauses this application state (nothing is updated during this time)
		 */
		virtual bool pause(void);  

		/**
		 * @brief	Resumes this application state from a prior pause
		 */
		virtual void resume(void);

		/**
		 * @brief	Destroys the appstate when the whole application gets closed.
		 * @note	Do not call this function, the appstate gets destroyed implicitly
		 */
		void destroy();

		/**
		 * @brief	Gets the name of this application state
		 * @return 	name The name
		 */
		Ogre::String getName(void) const;

		/**
		 * @brief	Gets whether this AppState has started.
		 * @return 	hasStarted true, if the AppState has started, else false.
		 * @note	This can be used to ask from another AppState like a MenuState, whether a GameState has already started, so that e.g. a continue button can be shown in the Menu.
		 */
		bool getHasStarted(void) const;
	protected:
		AppState();

		/**
		 * @brief		Starts the render thread with its render game loop.
		 */
		void startRendering(void);

		/**
		 * @brief		Stop the render thread game loop.
		 */
		void stopRendering(void);

		/**
		 * @brief		Finds a state by a given name
		 * @param[in]	stateName The state to find by name
		 * @return 		appState The found state or NULL if not found
		 */
		AppState* findByName(Ogre::String stateName);

		/**
		 * @brief		Returns the successor application state of the current one.
		 * @param[in]	currentAppState The current state
		 * @return		appState The found state or NULL if not found
		 */
		AppState* getNextState(AppState* currentAppState);

		/**
		 * @brief		Ends the current application state and starts the one specified
		 * @param[in]	appState The next appilcation state to start
		 */
		void changeAppState(AppState* appState);

		/**
		 * @brief		Pushes the specified application state, but does not end the current application state
		 * @param[in]	appState The next appilcation state to start
		 * @return		success True if the current state is not paused, 
		 *				else false, which means that the next application state could not be pushed.
		 */
		bool pushAppState(AppState* appState);

		/**
		 * @brief	Ends the current application state, removes it and continues with another application state.
		 *			If there is no other state the whole application will be closed.
		 */
		void popAppState(void);

		/**
		 * @brief		Ends the current application state, removes all present application states and starts the specified one.
		 * @param[in]	appState The appilcation state to start
		 */
		void popAllAndPushAppState(AppState* appState);

		/**
		 * @brief	Calling this function forces the whole application to be shoot down.
		 */
		void shutdown(void);

		AppStateListener* getAppStateManager(void) const;

		/**
		 * @brief	Initializes all necessary modules: GameProgressModule, ParticleUniverseModule, WorkspaceModule, OgreALModule
		 *			Also adds the render queue listener for overlay.
		 * @param[in]	initSceneManager Whether to init scene manager automatically, or the user may do it with own settings
		 * @param[in]	initCamera Whether to init camera automatically, or the user may do it with own settings
		 */
		void initializeModules(bool initSceneManager, bool initCamera);

		/**
		 * @brief	Destroys all modules: WorkspaceModule, CameraManager, GameObjectController, ParticleUniverseModule, GameProgressModule, LuaScriptApi, OgreALModule, Ogre graphics, OgreNewtModule
		 *			Also removes the render queue listener from overlay.
		 * @note	The scene manager must be valid!
		 */
		void destroyModules(void);
	private:
		void handleSceneLoaded(NOWA::EventDataPtr eventData);
	protected:
		AppStateListener* appStateManager;
		Ogre::String nextAppStateName;
		Ogre::String appStateName;
		bool hasStarted;
	protected:
		Ogre::SceneManager* sceneManager;
		Ogre::Camera* camera;
		OgreNewt::World* ogreNewt;
		bool bQuit;
		bool canUpdate;
		Ogre::String currentSceneName;
		GameObjectController* gameObjectController;
		GameProgressModule* gameProgressModule;
		RakNetModule* rakNetModule;
		MiniMapModule* miniMapModule;
		OgreNewtModule* ogreNewtModule;
		MeshDecalGeneratorModule* meshDecalGeneratorModule;
		CameraManager* cameraManager;
		OgreRecastModule* ogreRecastModule;
		ParticleUniverseModule* particleUniverseModule;
		LuaScriptModule* luaScriptModule;
		EventManager* eventManager;
		ScriptEventManager* scriptEventManager;

		WorkspaceBaseComponent* workspaceBaseComponent;
	};

}; // namespace end

//|||||||||||||||||||||||||||||||||||||||||||||||
//Makro zum Erstellen von Zustaenden
//pAppState bsp. Gamestate oder Menustate werden über das Makro erstellt
//T steht z.B. fuer Menustate, die Klasse wird verknuepft mit dem AppstateListner und somit mit dem AppStateManager, welcher alle States verwaltet
//parent ist der AppStateManager, d.h. der AppStateManager ist der Vater vom aktuellen Zustand (MenuState, PauseState, GameState)

/**
 * @brief	Creates an appliction state for the given name and registers it to the application state manager
 */
#define DECLARE_APPSTATE_CLASS(T)																							\
static void create(NOWA::AppStateListener* appStateManager, const Ogre::String& name, const Ogre::String& nextAppStateName)	\
{																															\
	T* appState = new T();																									\
	appState->appStateManager = appStateManager;																			\
	appState->appStateName = name;																							\
	appState->nextAppStateName = nextAppStateName;																			\
	appStateManager->manageAppState(name, appState);																		\
}

#endif


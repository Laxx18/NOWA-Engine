#ifndef APP_STATE_H
#define APP_STATE_H

#include "defines.h"

#include "MyGUI.h"
#include "MyGUI_Ogre2Platform.h"
#include "MessageBox/MessageBox.h"

namespace NOWA
{
	//Abstrakte Klasse: AppStateManager ist abgeleitet und implementiert diese Funktionen
	//Jeglicher Versuch den AppStateListener zu entfernen und alles direkt ueber den AppStateManager zu regeln ist gescheitert
	//wegen dem Macro, das Macro akzeptiert scheinbar nur ein abstraktes Objekt
	//Im Forum wurden Nachforschungen angestellt, jedoch ohne Erfolg der Autor hat das Ganze selbst zum Teil als Loesung schon so gefunden
	//http://www.ogre3d.org/forums/viewtopic.php?f=5&t=61001
	//https://www.ogre3d.org/forums/viewtopic.php?f=5&t=51994&start=0

	class EXPORTED AppState : public OIS::KeyListener, public OIS::MouseListener, public OIS::JoyStickListener
	{
	public:
		friend class AppStateManager;

		/**
		 * @brief	Can be used to init some variables and is called when the new application state gets entered
		 * @note	Init all variables in enter() and not in the constructor! Since the constructor is only called for the first enter.
		 *			When for example this application state is exited to go to the menue and then back to this application state, 
		 *			the constructor is not called!
		 */
		virtual void enter(void) = 0;

		/**
		 * @brief	Can be used to destroy all objects.
		 * @note	like in @enter() do not use the destructor do destroy some variables, 
		 *			because the destructor is only called if the whole application is quitted.
		 */
		virtual void exit(void) = 0;

		/**
		 * @brief	Clones the given app state
		 * @note	Must be implemented
		 */
		virtual AppState* clone(void) = 0;

		/**
		 * @brief		Can be used to update all Objects and control logic.
		 * @param[in] dt The delta time in seconds. For example if the game runs with 60 fps. 
		 *				The delta time between two update calls is 0.016 sec.
		 * @note		When you overwrite the update() function, call in your update function of your state: AppState::update(dt); too,
		 *				because in the update() of the base application state are some updates of importent components executed, 
		 *				like the ProcessManager to handle own processes.
		 */
		virtual void update(Ogre::Real dt);

		/**
		* @brief		LateUpdate is called once per frame, after Update has finished. 
		*				Any calculations that are performed in Update will have completed when LateUpdate begins. 
		*				A common use for LateUpdate would be a following third-person camera. 
		*				If you make your character move and turn inside Update, you can perform all camera movement and rotation calculations in LateUpdate. 
		*				This will ensure that the character has moved completely before the camera tracks its position.
		* @param[in]	dt The delta time in seconds. For example if the game runs with 60 fps.
		*				The delta time between two update calls is 0.016 sec.
		*/
		virtual void lateUpdate(Ogre::Real dt);

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
	protected:
		AppState();

		/**
		 * @brief	Finds a state by a given name
		 * @param	stateName The state to find by name
		 * @return 	appState The found state or NULL if not found
		 */
		AppState* findByName(Ogre::String stateName);

		/**
		 * @brief	Returns the successor application state of the current one.
		 * @param	currentAppState The current state
		 * @return 	appState The found state or NULL if not found
		 */
		AppState* getNextState(AppState* currentAppState);

		/**
		 * @brief	Ends the current application state and starts the one specified
		 * @param	appState The next appilcation state to start
		 */
		void changeAppState(AppState* appState);

		/**
		 * @brief	Pushes the specified application state, but does not end the current application state
		 * @param	appState The next appilcation state to start
		 * @return	success True if the current state is not paused, 
		 *			else false, which means that the next application state could not be pushed.
		 */
		bool pushAppState(AppState* appState);

		/**
		 * @brief	Ends the current application state, removes it and continues with another application state.
		 *			If there is no other state the whole application will be closed.
		 */
		void popAppState(void);

		/**
		 * @brief	Ends the current application state, removes all present application states and starts the specified one.
		 * @param	appState The appilcation state to start
		 */
		void popAllAndPushAppState(AppState* appState);

		/**
		 * @brief	Calling this function forces the whole application to be shoot down.
		 */
		void shutdown(void);

		AppStateManager* getAppStateManager(void) const;

		/**
		 * @brief	Initializes all necessary modules: GameProgressModule, ParticleUniverseModule, WorkspaceModule, OgreALModule
		 *			Also adds the render queue listener for overlay.
		 * @note	The scene manager must be valid!
		 */
		void initializeModules(void);

		/**
		 * @brief	Destroys all modules: WorkspaceModule, CameraManager, GameObjectController, ParticleUniverseModule, GameProgressModule, LuaScriptModule, OgreALModule, Ogre graphics, OgreNewtModule
		 *			Also removes the render queue listener from overlay.
		 * @note	The scene manager must be valid!
		 */
		void destroyModules(void);

	protected:
		AppStateManager* appStateManager;
		Ogre::String appStateName;
		Ogre::String worldName;
		Ogre::String nextAppStateName;
		Ogre::String nextWorldName;
		
	protected:
		Ogre::SceneManager* sceneManager;
		Ogre::Camera* camera;
		bool bQuit;
	};

}; // namespace end

#endif


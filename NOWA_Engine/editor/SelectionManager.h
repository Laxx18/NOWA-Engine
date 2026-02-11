#ifndef SELECTION_MANAGER_H
#define SELECTION_MANAGER_H

#include "defines.h"

#include "SelectionRectangle.h"
#include "main/EventManager.h"
#include "modules/CommandModule.h"

namespace NOWA
{
	class Ogre::SceneManager;
	class Ogre::Camera;
	class Ogre::Viewport;
	class Ogre::SceneNode;
	class Ogre::ManualObject;
	class GameObject;
	
	/**
	* @class SelectionManager
	* @brief This class can be used to select game objects
	*/
	class EXPORTED SelectionManager
	{
	public:
		friend class EditorManager;
	public:
		struct EXPORTED SelectionData
		{
			SelectionData()
				: gameObject(nullptr),
				initiallyDynamic(true)
			{
				
			}
			GameObject* gameObject;
			// Whether the game object was dynamic when it has been selected (for restore when selecting another game object)
			bool initiallyDynamic;
		};

		/**
		* @class ISelectionObserver
		* @brief This interface must be implemented to react each time a game object will be selected, e.g. to set how to select the game object
		*/
		class EXPORTED ISelectionObserver
		{
		public:
			virtual ~ISelectionObserver()
			{
			}

			/**
			* @brief		Is called when the selection is handled and must be implemented, because here its no possible to know, how the user wants to select the entity,
			*				e.g. with which outline template strategy
			* @param[in]	gameObject	The game object to handle
			* @param[in]	selected	Whether the game object has been seleted or unselected
			*/
			virtual void onHandleSelection(NOWA::GameObject* gameObject, bool selected) = 0;
		};
		
		
		SelectionManager();

		virtual ~SelectionManager();

		/*
		* @brief Sets the selection observer to react at the moment when a game object has been selected.
		* @param[in]	selectionObserver	The selection observer
		* @note	A newly created selection observer heap pointer must be passed for the ISpawnObserver. It will be deleted internally,
		*		if the object is not required anymore.
		*/

		/**
		* @brief		Initializes the selection manager
		* @param[in]	sceneManager		The ogre scene manager to use
		* @param[in]	camera				The ogre camera to use
		* @param[in]	categories			The categories of game objects that are selectable
		* @param[in]	mouseButtonId		Which mouse button should handle the selection
		* @param[in]	selectionObserver	Sets the selection observer to react at the moment when a game object has been selected.
		*									A newly created selection observer heap pointer must be passed for the ISpawnObserver. It will be deleted internally,
		*									if the object is not required anymore.
		* @param[in]	materialName		The material name to use for the rectangle. Default is "Select" which comes from the NOWA folder
		* @Note			The shift key can be used to add or remove objects to/from selection
		*/
		void init(Ogre::SceneManager* sceneManager, Ogre::Camera* camera, const Ogre::String& categories, OIS::MouseButtonID mouseButtonId, ISelectionObserver* selectionObserver, 
			 const Ogre::String& materialName = "Select");

		void handleKeyPress(const OIS::KeyEvent& keyEventRef);

		void handleMouseMove(const OIS::MouseEvent& evt);
		
		void handleKeyRelease(const OIS::KeyEvent& keyEventRef);

		void handleMousePress(const OIS::MouseEvent& evt, OIS::MouseButtonID id);

		void handleMouseRelease(const OIS::MouseEvent& evt, OIS::MouseButtonID id);

		/**
		 * @brief		Adapts the category (query mask) at runtime, to change which game object types can be selected.
		 * @param[in]	categories	The categories to use
		 * @return		categoryId	The generated category id
		 */
		unsigned int filterCategories(Ogre::String& categories);

		/**
		* @brief		Changes the material name for the rectangle
		* @param[in]	materialName	The new material name to set
		*/
		void changeMaterialName(const Ogre::String& materialName);

		const Ogre::String& getMaterialName(void);

		const Ogre::String& getCategories(void);

		/**
		 * @brief		Gets whether the selection manager is spanning a rectangle
		 * @return		true if the selection manager is creating a selection rectangle, else false
		 */
		bool getIsSelectingRectangle(void) const;

		/**
		* @brief		Gets whether the selection manager is selecting
		* @return		true if the selection manager is selecting
		*/
		bool getIsSelecting(void) const;

		/*
		* @brief Sets the selection observer to react at the moment when a game object has been selected.
		* @param[in]	selectionObserver	The selection observer
		* @note	A newly created selection observer heap pointer must be passed for the ISpawnObserver. It will be deleted internally,
		*		if the object is not required anymore.
		*/
		void changeSelectionObserver(ISelectionObserver* selectionObserver);

		ISelectionObserver* getSelectionObserver(void) const;

		void selectGameObjects(const Ogre::Vector2& leftTopCorner, const Ogre::Vector2& bottomRightCorner);

		void clearSelection(void);

		void select(const unsigned long id, bool bSelect = true);

		std::unordered_map<unsigned long, SelectionData>& getSelectedGameObjects(void);

		void snapshotGameObjectSelection(void);
		
		void selectionUndo(void);

		void selectionRedo(void);

		bool canSelectionUndo(void);

		bool canSelectionRedo(void);

	private:
		void queueSelectionEvent(unsigned long id, bool selected);

		void handleDeleteGameObject(EventDataPtr eventData);

		void applySelectInternal(GameObject* gameObject, bool bSelect);
	private:
		Ogre::Camera* camera;
		Ogre::SceneManager* sceneManager;
		Ogre::PlaneBoundedVolumeListSceneQuery* volumeQuery;
		Ogre::RaySceneQuery* selectQuery;
		Ogre::SceneNode* selectionNode;
		NOWA::SelectionRectangle* selectionRect;
		
		OIS::MouseButtonID mouseButtonId;
		bool isSelectingRectangle;
		bool isSelecting;
		bool mouseIdPressed;
		bool shiftDown;
		Ogre::Vector2 selectBegin;
		Ogre::Vector2 selectEnd;
		Ogre::String categories;
		unsigned int categoryId;
		Ogre::String materialName;
		
		std::unordered_map<unsigned long, SelectionData> selectedGameObjects;
		ISelectionObserver* selectionObserver;

		CommandModule selectionCommandModule;
	};

};  //namespace end

#endif
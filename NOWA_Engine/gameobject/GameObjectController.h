#ifndef GAME_OBJECT_CONTROLLER_H
#define GAME_OBJECT_CONTROLLER_H

#include "defines.h"
#include "GameObject.h"
#include "ki/MovingBehavior.h"
// #include "ki/MovingBehavior2D.h"
#include "modules/DotSceneExportModule.h"
#include "modules/DotSceneImportModule.h"
#include "modules/CommandModule.h"

#include <thread>

namespace NOWA
{
	class JointComponent;
	class PlayerControllerComponent;
	class PhysicsCompoundConnectionComponent;
	
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class DeleteGameObjectsUndoCommand : public ICommand
	{
	public:

		DeleteGameObjectsUndoCommand(const Ogre::String& appStateName, std::vector<unsigned long> gameObjectIds);

		virtual ~DeleteGameObjectsUndoCommand();

		virtual void undo(void) override;

		virtual void redo(void) override;

		std::vector<unsigned long> getIds(void) const;
	private:
		void deleteGameObjects(void);

		void createGameObjects(void);
	private:
		Ogre::String appStateName;
		std::vector<unsigned long> gameObjectIds;
		Ogre::String gameObjectsToAddStream;
		DotSceneExportModule* dotSceneExportModule;
		DotSceneImportModule* dotSceneImportModule;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class SnapshotGameObjectsCommand : public ICommand
	{
	public:

		SnapshotGameObjectsCommand(const Ogre::String& appStateName);

		virtual ~SnapshotGameObjectsCommand();

		virtual void undo(void) override;

		virtual void redo(void) override;
	private:
		void snapshotGameObjects(void);

		void resetGameObjects(void);
	private:
		Ogre::String appStateName;
		std::vector<unsigned long> gameObjectsIdsBeforeSnapShot;
		Ogre::String gameObjectsToAddStream;
		DotSceneExportModule* dotSceneExportModule;
		DotSceneImportModule* dotSceneImportModule;
	};
	
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED GameObjectController
	{
		friend class PhysicsActiveComponent;
		friend class AppState; // Only AppState may create this class
	public:
		typedef boost::shared_ptr<GameObject> GameObjectPtr;
		typedef boost::shared_ptr<JointComponent> JointCompPtr;
		typedef boost::shared_ptr<PlayerControllerComponent> PlayerControllerCompPtr;
		typedef boost::shared_ptr<PhysicsCompoundConnectionComponent> PhysicsCompoundConnectionCompPtr;

		typedef std::map<unsigned long, GameObjectPtr> GameObjects;
	public:
		/**
		* @class ITriggerSphereQueryObserver
		* @brief This interface can be implemented to react each time a game object enters the sphere area and leaves.
		*/
		class EXPORTED ITriggerSphereQueryObserver
		{
		public:
			/**
			* @brief		Called when a game object enters the sphere area.
			*				Note that this function will be call as long as the game object is in the area at the given update frequency specified in the @initAreaForActiveObjects.
			* @param[in]	gameObject	The game object to manipulate
			*/
			virtual void onEnter(NOWA::GameObject* gameObject) = 0;

			/**
			* @brief		Called when a game object leaves the sphere area.
			*				Note that this function will be called just once.
			* @param[in]	gameObject	The game object to manipulate
			*/
			virtual void onLeave(NOWA::GameObject* gameObject) = 0;
		};

	public:
		/**
		 * @brief		Destroys the whole content in the GameObjectController (game objects, categories, etc.)
		 * @param[in]	excludeGameObjectNames		Optional a list of game object names, that should not be destroyed, e.g. MainCamera etc.
		 * @note		This function should be called when a new scene is created. E.g. entering a new AppState, call it in the @AppState::exit() function. Because all game objects are used for one scene manager.
		 *              Do not call this function in an update method, else hell is raised.
		 */
		void destroyContent(std::vector<Ogre::String>& excludeGameObjectNames = std::vector<Ogre::String>());

		/**
		 * @brief		Updates all game objects and its components
		 * @param[in]	dt		The delta time in seconds between two frames (update cycles).
		 * @param[in]	notSimulating	If set to true all data is updated, if set to false only necessary data is updated.
		 * @note		This can be used e.g. for a level editor, in which there is a play mode. If the play mode is on, everything is updated. If off, only necessary data like
		 *				game object bounding box is updated.
		 */
		void update(Ogre::Real dt, bool notSimulating = false);

		/**
		 * @brief		Late updates all game objects and its components.
		 * @param[in]	dt		The delta time in seconds between two frames (update cycles).
		 * @param[in]	notSimulating	If set to true all data is updated, if set to false only necessary data is updated.
		 * @note		This can be used e.g. for a level editor, in which there is a play mode. If the play mode is on, everything is updated. If off, only necessary data like
		 *				game object bounding box is updated.
		 */
		void lateUpdate(Ogre::Real dt, bool notSimulating = false);

		/**
		* @brief		Deletes the given game object ptr immediately.
		* @param[in]	gameObjectPtr	The game object ptr to delete
		*/
		void deleteGameObjectImmediately(GameObjectPtr gameObjectPtr);

		/**
		* @brief		Deletes the given game object ptr immediately.
		* @param[in]	id	The game object id to delete
		*/
		void deleteGameObjectImmediately(unsigned long id);

		/**
		* @brief		Deletes the game object by id. Note: The game object by the given id will not be deleted immediately but in the next update turn.
		* @param[in]	id		The id to receive the game object from.
		* @note			This function can be used e.g. from a component of the game object, to delete it later, so that the component which calls this function will be deleted to.
		*/
		void deleteGameObject(const unsigned long id);
		
		/**
		* @brief		Deletes the game object by id. Note: The game object by the given id will not be deleted immediately but in the next update turn. It will also be pushed into an undo queue, so it can be undo on demand.
		* @param[in]	id		The id to receive the game object from.
		* @note			This function can be used e.g. from a component of the game object, to delete it later, so that the component which calls this function will be deleted to.
		*/
		void deleteGameObjectWithUndo(const unsigned long id);

		/**
		* @brief		Creates a snapshot of the game objects. It will also be pushed into an undo queue, so it can be undo on demand.
		*/
		void snapshotGameObjects(void);
		
		/**
		 * @brief		Undo's a game object deletion.
		 * @note			This function can be used e.g. if in NOWA-Design in simulation mode a coin is picked up and shall vanish. Yet if simulation has ended, it shall appear again.
		 */
		void undo(void);
		
		/**
		 * @brief		Undo's all game object deletions.
		 * @note		This function can be used e.g. if in NOWA-Design in simulation mode a coin is picked up and shall vanish. Yet if simulation has ended, it shall appear again.
		 */
		void undoAll(void);
		
		/**
		 * @brief		Redo's a game object deletion.
		 */
		void redo(void);
		
		/**
		 * @brief		Redo's all game object deletions.
		 */
		void redoAll(void);

		/**
		 * @brief		Gets whether the engine is in simulation state (between start and stop)
		 * @return		true	If engine is in simulating state, else false.
		 */
		bool getIsSimulating(void) const;

		/**
		* @brief		Delete the game object by its pointer. Note: The game object by the given id will not be deleted immediately but in the next update turn.
		* @param[in]	gameObjectPtr	The game object ptr to delete
		*/
		void deleteGameObject(GameObjectPtr gameObjectPtr);

		/**
		* @brief		The game object by the given id will not be deleted immediately but at a later time offset,
		* @param[in]	id				The id to receive the game object from.
		* @param[in]	timeOffsetSec	The relative time offset when to game object should be deleted.
		* @note			This function can be used e.g. from a component of the game object, to delete it later, e.g. when a bomb explodes and there is an particle effect which has a certain
		*				duration, so that the game object will be deleted after the effect.
		*/
		void deleteDelayedGameObject(const unsigned long id, Ogre::Real timeOffsetSec);

		

		/**
		 * @brief		Make a clone of a game object with the given name.
		 * @param[in]	id					The original id of the game object to be cloned. It is the scene node name.
		 * @param[in]	parentNode			The parent node to create the child and attach it under that node. If the node is null, 
		 *									the new scene node will be created under the root.
		 * @param[in]	targetId			Optional target id to set, to that the cloned game object will have that id. This is usefull for undo/redo
		 * @param[in]	targetPosition		The target position at which location the cloned game object should be spawned.
		 * @param[in]	targetOrientation	The target orientation at which the cloned game object should be orientated.
		 * @param[in]	targetScale			The target scale whether the game object should be bigger or smaller than the original game object.
		 * @note								If none location parameter are set the cloned game object will take the location parameter of the original game object.
		 *									Important: Be careful what game object you will clone due to performance reasons. When a game object e.g. is composed of a lot of heavy components
		 *									Everything inside will be cloned. Imagine a bomb which has 4 components:
		 *										PhysicsActiveComponent, PhysicsExplosionComponent, ParticleUniverseComponent, SimpleSoundComponent
		 *									Now each time a bomb is cloned, the heavy particle system is cloned to and the sounds. It would be better to set the bomb visible false
		 *									and reset the position each time, a player throws the bomb, instead of cloning the bomb and deleting when thrown.
		 *									
		 * @return		clonedGameObject	The cloned game object
		 */
		GameObjectPtr clone(const Ogre::String& originalGameObjectName, Ogre::SceneNode* parentNode = nullptr, unsigned long targetId = 0, const Ogre::Vector3& targetPosition = Ogre::Vector3::ZERO,
			const Ogre::Quaternion& targetOrientation = Ogre::Quaternion::IDENTITY, const Ogre::Vector3& targetScale = Ogre::Vector3(1.0f, 1.0f, 1.0f));

		/**
		 * @see		GameObjectController::clone()
		 */
		GameObjectPtr clone(unsigned long originalGameObjectId, Ogre::SceneNode* parentNode = nullptr, unsigned long targetId = 0, const Ogre::Vector3& targetPosition = Ogre::Vector3::ZERO,
			const Ogre::Quaternion& targetOrientation = Ogre::Quaternion::IDENTITY, const Ogre::Vector3& targetScale = Ogre::Vector3(1.0f, 1.0f, 1.0f));

		/**
		 * @brief		Registers a game object. Internally adds the game object to a map for management and sets a category id for scene queries.
		 * @param[in]	gameObjectPtr	The game object to register
		 */
		void registerGameObject(GameObjectPtr gameObjectPtr);
		
		/**
		 * @brief		Registers an internal type from the given game object ptr and its category. Internally called when calling @GameObjectController::registerGameObject()
		 * @param[in]	gameObjectPtr	The game object to register the type for
		 * @param[in]	category		The category to register the type for
		 */
		void registerType(boost::shared_ptr<GameObject> gameObjectPtr, const Ogre::String& category);

		/**
		* @brief		Registers an internal type from the given game object and its category. Internally called when calling @GameObjectController::registerGameObject()
		* @param[in]	gameObject	The game object raw pointer to register the type for
		* @param[in]	category		The category to register the type for
		*/
		void registerType(GameObject* gameObject, const Ogre::String& category);

		/**
		* @brief		Just registers a category. Which is then available by this class besides other categories.
		* @param[in]	category		The category name to register
		* @return       The assigned category id
		*/
		unsigned int registerCategory(const Ogre::String& category);

		/**
		* @brief		Changes the category name, this game object belongs to
		* @param[in]	gameObject		The game object to change the category
		* @param[in]	oldCategory		The old category name to check if it does exist, after a new category is set
		* @param[in]	newCategory		The new category name to set
		*/
		void changeCategory(GameObject* gameObject, const Ogre::String& oldCategory, const Ogre::String& newCategory);

		/**
		* @brief		Frees the given category. So when there is no game object using this category any more, it will be deleted
		* @param[in]	category		The category to un-register from game object
		*/
		void freeCategoryFromGameObject(const Ogre::String& category);

		/**
		 * @brief		Gets the game object from the given id.
		 * @param[in]	id	The id to get the game object from
		 * @return		gameObjectPtr	The game object shared ptr.
		 * @note		Be careful with this game object as the life cycle will be extended. E.g. when using this game object as a part in the app state. Call gameObjectPtr.reset() in the @AppState::exit() method.
		 */
		GameObjectPtr getGameObjectFromId(const unsigned long id) const;

		/**
		 * @brief		Gets all game object ids.
		 * @return		gameObjectIds	The game object ids to get.
		 */
		std::vector<unsigned long> getAllGameObjectIds(void);

		/**
		 * @brief		Gets the new cloned game object by its prior id, since it may have been cloned and has now a new id
		 * @param[in]	priorId		The prior id to find the game object
		 * @return		Not the game object, that has been found by prior Id, but the new one, that has been cloned
		 * @note		This method should be used in GameObjectComponent::onCloned() method to get the new cloned game object from the prior one.
		 */
		GameObjectPtr getClonedGameObjectFromPriorId(const unsigned long priorId) const;

		/**
		 * @brief		Gets all game objects in the map container for manipulation
		 * @return		gameObjects	The map with all game objects for read only operations. If nothing can be found, an empty list will be delivered.
		 */
		GameObjects const* getGameObjects(void) const;

		/**
		 * @brief		Gets all game objects belonging to the given category name.
		 * @param[in]	category	The category name to get the list of game objects from
		 * @return		gameObjectList	A list with game object ptr's. If nothing can be found, an empty list will be delivered.
		 * @note		Be careful with this game object as the life cycle will be extended. E.g. when using this game object as a part in the app state. Call gameObjectPtr.reset() in the @AppState::exit() method.
		 */
		std::vector<GameObjectPtr> getGameObjectsFromCategory(const Ogre::String& category);

		/**
		 * @brief		Gets all game objects which contain at least one of this component class name sorted by name.
		 * @param[in]	getStaticClassName()	The component class name to get the game objects for.
		 * @return		gameObjectList	A by name sorted list with game object ptr's. If nothing can be found, an empty list will be delivered.
		 */

		std::vector<GameObjectPtr> getGameObjectsFromComponent(const Ogre::String& componentClassName);
		
		/**
		 * @brief		Gets all game objects belonging to the given category id.
		 * @param[in]	categoryId	The category id to get the list of game objects from. It can be a combined id, generated via @generateCategoryId
		 * @return		gameObjectList	A list with game object ptr's. If nothing can be found, an empty list will be delivered.
		 * @note		Be careful with this game object as the life cycle will be extended. E.g. when using this game object as a part in the app state. Call gameObjectPtr.reset() in the @AppState::exit() method.
		 */
		std::vector<GameObjectPtr> getGameObjectsFromCategoryId(unsigned int categoryId);

		/**
		 * @brief		Gets all game objects belonging to the given category name but separate list by the given game object names.
		 * @param[in]	category			The category name to get the list of game objects from
		 * @param[in]	excludeGameObjects	The list of game objects that should be in a separate list
		 * @return		gameObjectList, separatedGameObjectList	A list with game object ptr's. If nothing can be found, an empty list will be delivered.
		 * @note		Be careful with this game object as the life cycle will be extended. E.g. when using this game object as a part in the app state. Call gameObjectPtr.reset() in the @AppState::exit() method.
		 */
		std::pair<std::vector<GameObjectPtr>, std::vector<GameObjectPtr>> getGameObjectsFromCategorySeparate(const Ogre::String& category, const std::vector<Ogre::String> separateGameObjects);

		/**
		 * @brief		Gets the next game object from the given group id.
		 * @param[in]	categoryIds			The category ids for filtering. Using ALL_CATEGORIES_ID, everything is selectable.
		 * @return		The game object raw pointer
		 */
		GameObject* getNextGameObject(unsigned int categoryIds);

		/**
		 * @brief		Gets the next game object from the given string group id. E.g. "Player+Enemy".
		 * @param[in]	categoryIds			The category ids for filtering. Using ALL_CATEGORIES_ID, everything is selectable.
		 * @return		The game object raw pointer
		 */
		GameObject* getNextGameObject(const Ogre::String& strCategoryIds);

		/**
		 * @brief		Gets all game object belonging to the given network client id.
		 * @param[in]	clientID	The client id to get the list of game objects from
		 * @return		gameObjectList	A list with game object ptr's. If nothing can be found, an empty list will be delivered.
		 * @note		Be careful with this game object as the life cycle will be extended. E.g. when using this game object as a part in the app state. Call gameObjectPtr.reset() in the @AppState::exit() method.
		 */
		std::vector<GameObjectPtr> getGameObjectsControlledByClientId(unsigned int clientID) const;

		/**
		 * @brief		Gets the game object from the given reference id.
		 * @param[in]	referenceId	The reference id to get the game object from
		 * @return		gameObjectPtr	The game object shared ptr.
		 * @note		Be careful with this game object as the life cycle will be extended. E.g. when using this game object as a part in the app state. Call gameObjectPtr.reset() in the @AppState::exit() method.
		 */
		GameObjectPtr getGameObjectFromReferenceId(const unsigned long referenceId) const;

		/**
		 * @brief		Gets the game object components from the given reference id.
		 * @param[in]	referenceId	The reference id to get the game object components from
		 * @return		gameObjectPtr	The game object shared ptr.
		 * @note		Be careful with this game object as the life cycle will be extended. E.g. when using this game object as a part in the app state. Call gameObjectPtr.reset() in the @AppState::exit() method.
		 */
		std::vector<GameObjectCompPtr> getGameObjectComponentsFromReferenceId(const unsigned long referenceId) const;

		/**
		 * @brief		Gets all game object components of the given game object component type.
		 * @return		gameObjectComponents	The game object component list.
		 */
		template <class ComponentType>
		std::vector<boost::shared_ptr<ComponentType>> getGameObjectComponents(void)
		{
			std::vector<boost::shared_ptr<ComponentType>> vec;
			for (auto& it = this->gameObjects->cbegin(); it != this->gameObjects->cend(); ++it)
			{
				GameObjectComponents* gameobjectComponents = it->second->getComponents();
				for (size_t i = 0; i < gameobjectComponents->size(); i++)
				{
					boost::shared_ptr<ComponentType> gameObjectCompPtr = boost::dynamic_pointer_cast<ComponentType>(std::get<COMPONENT>(gameobjectComponents->at(i)));
					if (nullptr != gameObjectCompPtr)
					{
						vec.emplace_back(gameObjectCompPtr);
					}
				}
			}
			
			return std::move(vec);
		}

		/**
		 * @brief		Activates all game object components, that have the same id, as the referenced game object.
		 * @param[in]	referenceId	The reference id for activation
		 * @return		gameObjectPtr	The game object shared ptr.
		 */
		void activateGameObjectComponentsFromReferenceId(const unsigned long referenceId, bool activate);

		/**
		 * @brief		Gets all registered categories at this time point.
		 * @return		categoryList	A list with all yet registered category names. If nothing can be found, an empty list will be delivered.
		 */
		std::vector<Ogre::String> getAllCategoriesSoFar(void) const;

		/**
		* @brief		Checks whether the given category is registered
		* @param[in]	category	The category name to check
		* @return		true, if the category is registered, else false
		*/
		bool hasCategory(const Ogre::String& category) const;

		/**
		 * @brief		Gets all game object id's belonging to the given category name.
		 * @param[in]	category	The category name to get the list of game objects id's from
		 * @return		gameObjectIdList	A list with game object id's. If nothing can be found, an empty list will be delivered.
		 */
		std::vector<unsigned long> getIdsFromCategory(const Ogre::String& category) const;

		/**
		 * @brief		Gets all other game object id's belonging to the given category name except the excluded one.
		 * @param[in]	excludedGameObjectPtr	The excluded game object ptr
		 * @param[in]	category	The category name to get the list of game objects id's from
		 * @return		gameObjectIdList	A list with game object id's. If nothing can be found, an empty list will be delivered.
		 */
		std::vector<unsigned long> getOtherIdsFromCategory(GameObjectPtr excludedGameObjectPtr, const Ogre::String& category) const;

		/**
		 * @brief		Gets the game object from the given name.
		 * @param[in]	name	The name to get the game object from
		 * @return		gameObjectPtr	The game object shared ptr.
		 * @note		Be careful with this game object as the life cycle will be extended. E.g. when using this game object as a part in the app state. Call gameObjectPtr.reset() in the @AppState::exit() method.
		 * 				Better use @GameObjectController::getGameObjectFromId() function, as it is faster in lookup and saver as game object names may change.
		 */
		GameObjectPtr getGameObjectFromName(const Ogre::String& name) const;

		/**
		 * @brief		Gets a list of game object's that do match the pattern. E.g. pattern = "Enemy*" would get "Enemy1", "Enemy2", "EnemyEvil" etc.
		 * @param[in]	pattern	The pattern to get the matched game object's from
		 * @return		gameObjectPtrList	The game object shared ptr list. If nothing can be found, an empty list will be delivered.
		 * @note		Be careful with this game object as the life cycle will be extended. E.g. when using this game object as a part in the app state. Call gameObjectPtr.reset() in the @AppState::exit() method.
		 * 				Better use @GameObjectController::getGameObjectFromId() function, as it is faster in lookup and saver as game object names may change.
		 */
		std::vector<GameObjectPtr> getGameObjectsFromNamePrefix(const Ogre::String& pattern) const;

		/**
		 * @brief		Gets a list of game object's that do match the given GameObject type like ENTITY, ITEM, TERRA, OCEAN, MIRROR etc.
		 * @param[in]	type	The game object type to set
		 * @return		gameObjectPtrList	The game object shared ptr list. If nothing can be found, an empty list will be delivered.
		 * @note		Be careful with this game object as the life cycle will be extended. E.g. when using this game object as a part in the app state. Call gameObjectPtr.reset() in the @AppState::exit() method.
		 * 				Better use @GameObjectController::getGameObjectFromId() function, as it is faster in lookup and saver as game object names may change.
		 */
		std::vector<GameObjectPtr> getGameObjectsFromType(GameObject::eType type) const;

		/**
		 * @brief		Gets the first game object that matches the pattern. E.g. pattern = "Enemy*" would get "Enemy1Abdcdedf". This function can used if the developer does not now the name exactly.
		 * @param[in]	pattern	The pattern to get the matched game object from
		 * @return		gameObjectPtr	The game object shared ptr.
		 * @note		Be careful with this game object as the life cycle will be extended. E.g. when using this game object as a part in the app state. Call gameObjectPtr.reset() in the @AppState::exit() method.
		 * 				Better use @GameObjectController::getGameObjectFromId() function, as it is faster in lookup and saver as game object names may change.
		 */
		GameObjectPtr getGameObjectFromNamePrefix(const Ogre::String& pattern) const;

		/**
		 * @brief		Gets a list of game object's that do match the given tag name
		 * @param[in]	tagName	The tag name to set
		 * @return		gameObjectPtrList	The game object shared ptr list. If nothing can be found, an empty list will be delivered.
		 * @note		Be careful with this game object as the life cycle will be extended. E.g. when using this game object as a part in the app state. Call gameObjectPtr.reset() in the @AppState::exit() method.
		 * 				Better use @GameObjectController::getGameObjectFromId() function, as it is faster in lookup and saver as game object names may change.
		 */
		std::vector<GameObjectPtr> getGameObjectsFromTagName(const Ogre::String& tagName) const;

		/**
		 * @brief		Gets a list of game object's that are using the given mesh name.
		 * @param[in]	meshname	The mesh name to set
		 * @return		gameObjectPtrList	The game object shared ptr list. If nothing can be found, an empty list will be delivered.
		 */
		std::vector<GameObjectPtr> getGameObjectsFromMeshName(const Ogre::String& meshName) const;

		/**
		 * @brief		Gets a list of game object's which do overlap each other.
		 * @return		gameObjectPtrList	The game object shared ptr list. If nothing can be found, an empty list will be delivered.
		 */
		std::vector<GameObjectPtr> getOverlappingGameObjects() const;

		/**
		 * @brief		Gets the category id from the given category name.
		 * @param[in]	category	The category name to get the id from
		 * @return		categoryId	The category id
		 * @note		If the category name cannot be found, 0 will be delivered.
		 */
		unsigned int getCategoryId(const Ogre::String& category);

		/**
		 * @brief		Gets the generated type id from category names. This is useful for selection of specific objects etc.
		 * @param[in]	categoryNames	The category names as a string sequence
		 * @Note		A [+] prefix following by the category name adds those category to the type mask and a [-] subtracts it from a specified category name.
		 *				The category name 'ALL' has a special meaning. It enables all categories. Example:
		 *				- generateCategoryId("+All-House-Floor") generates an id, with which all objects but houses and floors are
		 *				  for example selectable.
		 *				- generateCategoryId("+Player+Enemy") would mark Players and Enemies to be selected
		 *				The category names are case independent. The best way is to generate the id when starting the application
		 *				and not during runtime.
		 *				Attention: Since the categories are defined by a designer, specifying a category that does not exist
		 *				prints just a warning in the log.
		 * @return		generated Id	The generated id from the category names
		 */
		unsigned int generateCategoryId(const Ogre::String& categoryNames);

		/**
		 * @brief		Gets the category name pieces from the given category names string.
		 * @param[in]	categoryNames	The category names to get pieces of categories in a list.
		 * @return		categoryNamesList	The category names list. If not found, 0 will be delivered.
		 * @note		If categoryNames = "+All-House-Floor", all category names except 'House' and 'Floor' will be delivered.
		 */
		std::vector<Ogre::String> getAffectedCategories(const Ogre::String& categoryNames);

		/**
		 * @brief		Gets the ogre newt physics material Id from the game object.
		 * @param[in]	gameObject	The game object to get the material id from
		 * @param[in]	ogreNewt	The ogre newt world the game object does belong to
		 * @return		materialId	The material id. If the material id cannot be found, 0 will be delivered.
		 * @note		Material id's are used to connect physics components of game objects together for more advanced physics actions, like building a trampoline or reacting when a collision occurred.
		 */
		const OgreNewt::MaterialID* getMaterialID(GameObject* gameObject, OgreNewt::World* ogreNewt);

		/**
		 * @brief		Gets the ogre newt physics material Id from the given category.
		 * @param[in]	gameObject	The category name to get material id from
		 * @param[in]	ogreNewt	The ogre newt world the game object does belong to
		 * @return		materialId	The material id. If the material id cannot be found, 0 will be delivered.
		 * @note		Material id's are used to connect physics components of game objects together for more advanced physics actions, like building a trampoline or reacting when a collision occurred.
		 */
		const OgreNewt::MaterialID* getMaterialIDFromCategory(const Ogre::String& category, OgreNewt::World* ogreNewt);

		/**
		 * @brief		Adds a joint component to a list, so that when @GameObjectController::connectGameObjects() is called, all connections are established for working physics constraints.
		 * @param[in]	jointCompWeakPtr	The weak joint component ptr to add
		 */
		void addJointComponent(boost::weak_ptr<JointComponent> jointCompWeakPtr);

		/**
		 * @brief		Removes a joint component from the list
		 * @param[in]	jointId	The joint component id to remove
		 */
		void removeJointComponent(const unsigned long jointId);

		/**
		 * @brief		Removes a joint component from the list and does break the joint chain of all other joint components, that are involved, because the whole joint will become invalid, anyway
		 * @param[in]	jointId	The joint component id to remove
		 */
		void removeJointComponentBreakJointChain(const unsigned long jointId);

		/**
		 * @brief		Gets a joint component weak ptr from the given joint id.
		 * @param[in]	jointId	The joint component id to get the joint component from
		 * @return		jointCompWeakPtr	The weak joint component ptr to get.
		 * @note		In order to be able to work with the joint component further, call @NOWA::makeStrongPtr() for this weak ptr.
		 */
		boost::weak_ptr<JointComponent> getJointComponent(const unsigned long jointId) const;

		/**
		 * @brief		Adds a player component to a list, so that when @GameObjectController::connectGameObjects() is called, all connections are established in order to be able to control a player.
		 * @param[in]	playerControllerCompWeakPtr	The weak player component ptr to add
		 */
		void addPlayerController(boost::weak_ptr<PlayerControllerComponent> playerControllerCompWeakPtr);

		/**
		 * @brief		Removes a player component from the list
		 * @param[in]	gameObjectId	The game object id to remove its player component from the list
		 */
		void removePlayerController(const unsigned long id);

		/**
		 * @brief		Activates the given player component from the given game object id.
		 * @param[in]	active	If set to true, the given player component will be activated, else deactivated
		 * @param[in]	gameObjectId	The game object id to get the player component from for activation
		 * @param[in]	onlyOneActive	Sets whether only one player instance can be controller. If set to false more player can be controlled, that is each player, that is currently selected.
		 */
		void activatePlayerController(bool active, const unsigned long id, bool onlyOneActive = true);

		/**
		 * @brief		Deactivates all player components.
		 */
		void deactivateAllPlayerController(void);

		/**
		 * @brief		Adds a physics compound connection component to a list, so that when @GameObjectController::connectGameObjects() is called, 
		 *				all connections are established in order to be able to create more complex physics active collision combinations.
		 * @param[in]	physicsCompoundConnectionCompWeakPtr	The weak physics compound connection component ptr to add
		 */
		void addPhysicsCompoundConnectionComponent(boost::weak_ptr<PhysicsCompoundConnectionComponent> physicsCompoundConnectionCompWeakPtr);

		/**
		 * @brief		Removes a physics compound connection component from the list
		 * @param[in]	gameObjectId	The game object id to remove its physics compound connection component from the list
		 */
		void removePhysicsCompoundConnectionComponent(const unsigned long id);

		/**
		 * @brief		Gets the root physics compound connection component weak ptr from the given game object id. This one is internally used as root and all other physics active collisions are connected to this one.
		 * @param[in]	rootId	The game object id to get the root physics compound connection component from
		 * @return		physicsCompoundConnectionCompWeakPtr	The weak joint component ptr to get.
		 * @note		In order to be able to work with this pointer further, call @NOWA::makeStrongPtr() for this weak ptr.
		 */
		boost::weak_ptr<PhysicsCompoundConnectionComponent> getPhysicsRootCompoundConnectionComponent(const unsigned long rootId) const;

		/**
		 * @brief		Adds a moving behavior to a list, so that when @GameObjectController::connectGameObjects() is called, all connections are established in order to be able perform more advance artificial behaviors.
		 * @param[in]	gameObjectId	The game object id to create and add the moving behavior.
		 * @note		This is realized this way, because a game object may have several ai game object components which are sharing one moving behavior to from actions like wander, flee etc.
		 */
		boost::shared_ptr<NOWA::KI::MovingBehavior> addMovingBehavior(const unsigned long gameObjectId);

		/**
		 * @brief		Adds a moving behavior 2D to a list, so that when @GameObjectController::connectGameObjects() is called, all connections are established in order to be able perform more advance artificial behaviors.
		 * @param[in]	gameObjectId	The game object id to create and add the moving behavior.
		 * @note		This is realized this way, because a game object may have several ai game object components which are sharing one moving behavior to from actions like wander, flee etc.
		 */
		// boost::shared_ptr<NOWA::KI::MovingBehavior2D> addMovingBehavior2D(const unsigned long gameObjectId);

		/**
		 * @brief		Removes the moving behavior from the list
		 * @param[in]	gameObjectId	The game object id to remove its moving behavior from the list
		 */
		void removeMovingBehavior(const unsigned long gameObjectId);

		/**
		 * @brief		Gets moving behavior weak ptr from the given game object id.
		 * @param[in]	gameObjectId	The game object id to get the moving behavior from
		 * @return		movingBehaviorWeakPtr	The weak moving behavior ptr to get.
		 * @note		In order to be able to work with this pointer further, call @NOWA::makeStrongPtr() for this weak ptr.
		 */
		boost::shared_ptr<NOWA::KI::MovingBehavior> getMovingBehavior(const unsigned long gameObjectId);

		/**
		 * @brief		Starts the simulation. Internally connects data between all game objects like joints, compounds etc.
		 * @Note		This can be called at any time, e.g. after a world has been loaded, but @stop() should be called first, if already connected.
		 */
		void start(void);
		
		/**
		 * @brief		Stops the simulation. Internally data between all game objects like joints, compounds etc. will be disconnected
		 */
		void stop(void);

		/**
		 * @brief		Pauses all game objects. When the AppState is just paused.
		 */
		void pause(void);

		/**
		 * @brief		Resumes all game objects. When the AppState is resumed.
		 */
		void resume(void);

		/**
		 * @brief		Connects the selected cloned game objects like joints, compounds etc. In each component @GameObjectComponent::onCloned is called so
		 *								so that each component, that has connections to other game objects, can react and find the target game object, that has been cloned by its prior id
		 * @param[in]	gameObjectIds	The game object ids that should be connect.
		 * @note		This should be called after a game object has been cloned
		 */
		void connectClonedGameObjects(const std::vector<unsigned long> gameObjectIds);

		/**
		 * @brief		Connects data between all joint components.
		 * @Note		This is called inside @GameObjectController::connectGameObjects().
		 */
		void connectJoints(void);

		// void disconnectJoints(void);

		/**
		 * @brief		Connects data between all physics active compound components.
		 * @Note		This is called inside @GameObjectController::connectGameObjects().
		 */
		void connectCompoundCollisions(void);

		/**
		 * @brief		Disconnects data between all physics active compound components.
		 * @Note		This is called inside @GameObjectController::disconnectGameObjects().
		 */
		void disconnectCompoundCollisions(void);

		/**
		 * @brief		Connects data between all physics active components forming a vehicle behavior.
		 * @Note		This is called inside @GameObjectController::connectGameObjects().
		 */
		void connectVehicles(void);

		/**
		 * @brief		Disconnects data between all physics active components.
		 * @Note		This is called inside @GameObjectController::disconnectGameObjects().
		 */
		void disconnectVehicles(void);

		/**
		 * @brief		Gets the game object by the given mouse coordinates and the scene query.
		 * @param[in]	x					The abs x mouse coordinate
		 * @param[in]	y					The abs y mouse coordinate
		 * @param[in]	camera				The current camera
		 * @param[in]	raySceneQuery		The ray scene query for filtering (which game object can be selected, controlled by categories)
		 * @param[in]	raycastFromPoint	If set to true, a ray detection will be processed against the geometry from the given scene orientation and mouse coordinates which may be slower but more precisely.
		 *									When set to false, the ray detection will just be processed against the bounding box of the entity.
		 * @return		The game object raw pointer
		 */
		GameObject* selectGameObject(int x, int y, Ogre::Camera* camera, Ogre::RaySceneQuery* raySceneQuery, bool raycastFromPoint = true);
		
		/**
		 * @brief		Gets the game object by the given mouse coordinates and the category id. The ray scene query is done via ogre newt and processed against the given collision hull, which is faster but may become more
		 *				imprecisely when convex hull is used.
		 * @param[in]	x					The abs x mouse coordinate
		 * @param[in]	y					The abs y mouse coordinate
		 * @param[in]	camera				The current camera
		 * @param[in]	ogreNewt			The ogre newt world, the game object belongs to
		 * @param[in]	categoryIds			The category ids for filtering. Using ALL_CATEGORIES_ID, everything is selectable.
		 * @param[in]	maxDistance			The max distance the ray can be shoot, game object more far away will not be detected.
		 * @param[in]	sorted				When set to true, the results are sorted by distance, so that if two game objects are hit, the game object that is more near to the origin of the ray will be delivered.
		 * @return		The game object raw pointer
		 */
		GameObject* selectGameObject(int x, int y, Ogre::Camera* camera, OgreNewt::World* ogreNewt, unsigned int categoryIds = ALL_CATEGORIES_ID, Ogre::Real maxDistance = 500.0f, 
			bool sorted = true);
		
		/**
		 * @brief		Gets the target position of the game object by the given mouse coordinates and the category id. The ray scene query is done via ogre newt and processed against the given collision hull, which is faster but may become more
		 *				imprecisely when convex hull is used.
		 * @param[in]	x					The abs x mouse coordinate
		 * @param[in]	y					The abs y mouse coordinate
		 * @param[in]	camera				The current camera
		 * @param[in]	ogreNewt			The ogre newt world, the game object belongs to
		 * @param[in]	categoryIds			The category ids for filtering. Using ALL_CATEGORIES_ID, everything is selectable.
		 * @param[in]	maxDistance			The max distance the ray can be shoot, game object more far away will not be detected.
		 * @param[in]	sorted				When set to true, the results are sorted by distance, so that if two game objects are hit, the game object that is more near to the origin of the ray will be delivered.
		 * @return		first: success, whether a game object has been hit or not; second: The game objects position
		 */
		std::pair<bool, Ogre::Vector3> getTargetBodyPosition(int x, int y, Ogre::Camera* camera, OgreNewt::World* ogreNewt, unsigned int categoryIds = ALL_CATEGORIES_ID, 
			Ogre::Real maxDistance = 500.0f, bool sorted = true);

		// void startConcurrentThread(OgreNewt::World* ogreNewt, const Ogre::Vector3& startPoint, const Ogre::Vector3& endPoint);

		// void endConcurrentThread(void);

		/**
		* @brief		Initialises the sphere scene query for checking active game objects in range
		* @param[in]	sceneManager					The scene manager for sphere scene query creation
		* @param[in]	position						The position of the sphere
		* @param[in]	distance						The distance of the area to check
		* @param[in]	categoryIds						The category ids to get game objects. Note that category ids can be composed e.g. via the function @generateCategoryId
		* @param[in]	triggerSphereQueryCallback		The trigger sphere query callback to get called when a game object is in range or get called when a game object leaves the range.
		* @param[in]	updateFrequency					The update frequency how often the query should be performed. Default is 0.5 which means 2x a second (1 / 0.5 = 2).
		*												E.g. Just activating a component must not be called often, but moving an game object should. 
		*												So setting the frequency to zero will update as often as the function is called in the game loop.
		* @Note											The trigger sphere query callback must be created on a heap, but not destroyed, since it will be destroyed automatically.
		*/
		void initAreaForActiveObjects(Ogre::SceneManager* sceneManager, const Ogre::Vector3& position, Ogre::Real distance, unsigned int categoryIds, Ogre::Real updateFrequency = 0.5f);

		/**
		* @brief		Attaches an observer, that is called when a game object enters the area or leaves the area
		* @param[in]	triggerSphereQueryObserver		The trigger sphere query observer to get notified when a game object is in range or get called when a game object leaves the range.
		* @Note											The trigger sphere query observer must be created on a heap, but not destroyed, since it will be destroyed automatically.
		*/
		void attachTriggerObserver(ITriggerSphereQueryObserver* triggerSphereQueryObserver);

		/**
		 * @brief		Detaches and destroys an observer
		 * @param[in]	triggerSphereQueryObserver		The trigger sphere query observer to detach
		 * @Note											The trigger sphere query observer will be destroyed automatically when detached.
		 */
		void detachAndDestroyTriggerObserver(ITriggerSphereQueryObserver* triggerSphereQueryObserver);

		/**
		 * @brief		Detaches and destroys all observer
		 */
		void detachAndDestroyAllTriggerObserver(void);

		/**
		* @brief		Checks via sphere scene query active game objects in range
		* @param[in]	position		The position of the sphere
		* @param[in]	dt				The delta time
		* @param[in]	distance		The distance of the area to check.
		* @param[in]	categoryIds		The category ids to get game objects. Note the category ids are optional here. If nothing is set, the one is used from the init function.
		*								So only specify one, if several scene queries with different distances, categories should be performed.
		* @Note							This function should be called in a loop.
		*/
		void checkAreaForActiveObjects(const Ogre::Vector3& position, Ogre::Real dt, Ogre::Real distance, unsigned int categoryIds = 0);

		/**
		* @brief		Gets a validated game object name. That is, if the given one does already exist, a slithly different and unique name will be delivered
		* @param[in]	gameObjectName	The game object name to validate
		* @param[in]	excludeId		The id to exclude, because this game object may be in the joints list already and would find itself else
		*/
		void getValidatedGameObjectName(Ogre::String& gameObjectName, unsigned long excludeId = 0);

		/**
		 * @brief		Creates all available game objects and meshes and places them in a matrix into the scene.
		 * @param[in]	sceneManager		The scene manager for scene node creation
		 * @Note		This must be used in an application to create all game objects. Afterwards fly with the camera and watch everything in each direction.
		 *				When using this, heavy lacks will be occuring, because the shader cache is build. For directX, this cache can be shipped along the appliction
		 *				on any PC. So this function can be used as a test scene to build the cache. Afterwards the lacks will disappear!
		 *				When craeted a scene and used that function, the application can be closed. Take a look in the release/debug folder if a *.cache file has been generated.
		 */
		void createAllGameObjectsForShaderCacheGeneration(Ogre::SceneManager* sceneManager);

		/**
		* @brief		Gets whether the game object controller has started destruction of all game objects.
		* @return		true, if destroying, else false.
		*/
		bool getIsDestroying(void) const;

		/**
		 * @brief		For lua autocompletion in order to register the correct type from a function call.
		*/
		template <class Type>
		Type* cast(Type* other)
		{
			return static_cast<Type*>(other);
		}
	public:
		static const unsigned int ALL_CATEGORIES_ID = 0xFFFFFFFF;

		static const unsigned long MAIN_GAMEOBJECT_ID = 1111111111L;
		static const unsigned long MAIN_CAMERA_ID = 1111111112L;
		static const unsigned long MAIN_LIGHT_ID = 1111111113L;
	public:
		GameObjectPtr internalClone(GameObjectPtr originalGameObjectPtr, Ogre::SceneNode* parentNode = nullptr, unsigned long targetId = 0, const Ogre::Vector3& targetPosition = Ogre::Vector3::ZERO,
			const Ogre::Quaternion& targetOrientation = Ogre::Quaternion::IDENTITY, const Ogre::Vector3& targetScale = Ogre::Vector3(1.0f, 1.0f, 1.0f));
		void internalRemoveJointComponentBreakJointChain(const unsigned long jointId);
	private:

		/**
		 * @brief		Creates the game object controller for the given application state name.
		 * @param[in]	appStateName		The application state name to set.
		 * @Note		This is necessary, since a class, that is managed by an application state, must know, from which application state, it has been created.
		 *				This is really important for an scenario such as, saving a GameState progress out from the MenuState:
		 *				local success = AppStateManager:getGameProgressModule2("GameState"):saveProgress(saveGameText, false);
		 *				Without the corresponding application state name, internally the GameObjectController, which is used in GameProgressModule, would be called for the latest AppState
		 *				which would be the MenuState, instead of the GameState and wrong GameObjects would be saved.
		 */
		GameObjectController(const Ogre::String& appStateName);
		~GameObjectController();

		void deleteJointDelegate(EventDataPtr eventData);
	private:
		Ogre::String appStateName;

		// active objects are not coming from the world loader and are more in the hand of the developer
		// and they are updated() corresponding their update() function whereas passive objects are not
		GameObjects* gameObjects;

		// name, id, occupied
		std::map<Ogre::String, std::pair<unsigned int, bool>> typeDBMap;
		unsigned int shiftIndex;
		std::list<GameObjectPtr> queryList;

		std::map<Ogre::String, OgreNewt::MaterialID*> materialIDMap;

		std::unordered_map<unsigned long, JointCompPtr> jointComponentMap;
		std::unordered_map<unsigned long, PlayerControllerCompPtr> playerControllerComponentMap;
		std::unordered_map<unsigned long, PhysicsCompoundConnectionCompPtr> physicsCompoundConnectionComponentMap;

		// for vehicle bodies
		std::map<std::pair<Ogre::String, PhysicsActiveComponent*>, std::vector<PhysicsActiveComponent*> > vehicleCollisionMap;
		std::map<Ogre::String, PhysicsActiveComponent*> tempVehicleObjectMap;
		std::map<Ogre::String, std::vector<PhysicsActiveComponent*> > vehicleChildTempMap;

		Ogre::SphereSceneQuery* sphereSceneQuery;
		std::vector<ITriggerSphereQueryObserver*> triggerSphereQueryObservers;
		std::unordered_map<unsigned long, GameObject*> triggeredGameObjects;
		Ogre::Real triggerUpdateTimer;
		Ogre::Real sphereQueryUpdateFrequency;
		Ogre::SceneManager* currentSceneManager;
		bool alreadyDestroyed;
		bool isSimulating;
		std::unordered_map<unsigned long, boost::shared_ptr<NOWA::KI::MovingBehavior>> movingBehaviors;
		// std::unordered_map<unsigned long, boost::shared_ptr<NOWA::KI::MovingBehavior2D>> movingBehaviors2D;
		CommandModule commandModule;

		std::set<unsigned long> delayedDeleterList;
		std::shared_ptr<DeleteGameObjectsUndoCommand> deleteGameObjectsUndoCommand;
		size_t nextGameObjectIndex;

		bool bIsDestroying;
		bool bAddListenerFirstTime;
	};

}; //namespace end NOWA

#endif
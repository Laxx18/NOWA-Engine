#ifndef GAME_OBJECT_H
#define GAME_OBJECT_H

#include "defines.h"
#include "utilities/rapidxml.hpp"
#include "utilities/BoundingBoxDraw.h"
#include "utilities/Variant.h"
#include "OgreWireAabb.h"
#include <tuple>

namespace NOWA
{
	// Always define the classes outside the class to specify typedefs, that are available to other classes
	class GameObjectComponent;
	class GameObject;
	class LuaScript;

	typedef boost::shared_ptr<GameObject> GameObjectPtr;
	typedef boost::shared_ptr<GameObjectComponent> GameObjectCompPtr;

	enum GameObjectComponentsTuple
	{
		CLASS_ID = 0,
		PARENT_CLASS_ID = 1,
		PARENT_PARENT_CLASS_ID = 2,
		COMPONENT = 3
	};

	// A vector is used instead of a map, because the insert order is important, so that a component which depends on another component can be inserted later
	// <SubtypeId, ParentId, ParentParent Id, GameObjectCompPtr> Note, the parent Id is really necessary, in order to have the possibility to call getComponent<Parent>()
	// even if it had never been registered
	// e.g. its possible to call getComponent<PhysicsComponent>(), to e.g. set a position for all derived types of PhysicsComponent, that are PhysicsActiveComponent, PhysicsRagdollComponent etc.
	// without the parent id, the position must be set for all possible derived types separately
	typedef std::vector<std::tuple<unsigned int, unsigned int, unsigned int, GameObjectCompPtr>> GameObjectComponents;

	class EXPORTED GameObject
	{
	public:

		enum eType
		{
			NONE = 0,
			ENTITY = 1,
			ITEM,
			SCENE_NODE, // E.g. for waypoints
			PLANE,
			MIRROR,
			CAMERA,
			REFLECTION_CAMERA,
			TERRA,
			OCEAN,
			LIGHT_DIRECTIONAL,
			LIGHT_SPOT,
			LIGHT_POINT,
			LIGHT_AREA,
			MANUAL_OBJECT,
			RECTANGLE,
			LINES,
			DECAL,
			BILL_BOARD,
			BILL_BOARD_CHAIN,
			SIMPLE_RENDERABLE
		};

		friend class GameObjectController;
		friend class GameObjectFactory;
		friend class SelectionManager;
		friend class TagPointComponent;
		friend class TerraComponent;
		friend class LuaScriptComponent;
		friend class CameraComponent;
	public:
		GameObject(Ogre::SceneManager* sceneManager, Ogre::SceneNode* sceneNode, Ogre::MovableObject* movableObject, const Ogre::String& category = "Default", const Ogre::String& renderCategory = "Default",
			bool dynamic = true, eType type = eType::ENTITY, unsigned long id = 0);

		virtual ~GameObject();

		/**
		 * @brief		Initializes the game object and its components by parsing the property element from XML
		 * @param[in]	newEntity	Optional new entity that has been created by a component to set and overwrite this entity
		 * @return		success		true, if all the game object could be initialised, else false
		 */
		virtual bool init(Ogre::MovableObject* newMovableObject = nullptr);

		/**
		 * @brief		Post initializes the game object and all its components. This is called when a game object is loaded or newly created.
		 * @return		success			true, if the post initialisation did work, else false
		 */
		virtual bool postInit(void);

		/**
		 * @brief		Connects a game object when all game objects and components are already post-initialized and available. This can be called manually by the @GameObjectController
		 *				e.g. to start a simulation. Everything is ready and will be connected.
		 * @param[in]	cloned			if set to true, this connection should be used to get the target game object by its prior id, since it has been cloned
		 * return		success			true, if the final initialisation did work, else false
		 */
		virtual bool connect(void);

		/**
		 * @brief		Disconnects data to other game objects
		 * @return		success			true, if the disconnection did work, else false
		 */
		virtual bool disconnect(void);

		/**
		 * @brief		In each component @onCloned is called so that each component, that has connections to other game objects, 
		 *				can react and find the target game object, that has been cloned by its prior id. So this function should be implemented by each component,
		 *				that has connection to other game objects, to get the new id when being cloned.
		 * return		success			true, if the cloning did work, else false
		 */
		virtual bool onCloned(void);

		/**
		 * @brief		Destroyes the game object
		 */
		virtual void destroy(void);

		/**
		 * @brief		Pauses the game object. When the AppState is just paused.
		 */
		virtual void pause(void);

		/**
		 * @brief		Resumes the game object. When the AppState is resumed.
		 */
		virtual void resume(void);

		/**
		 * @brief		Called on each update tick
		 * @param[in]	dt								The delta time in seconds between two frames (update cycles).
		 * @param[in]	notSimulating	If set to true only graphics for the simulation is updated and shown, if set to false all graphics (even debug graphics, such as light mesh) is shown.
		 * @note		This function can be used to update periodically some data. When vsync is on, this function will be called approx. 60 times so at best
		 *				td is 0.016 seconds or 16 milliseconds
		 *				This can be used e.g. for a level editor, in which there is a play mode. If the play mode is on, everything is updated. If off, only necessary data like
		 *				game object bounding box is udpated.
		 */
		virtual void update(Ogre::Real dt, bool notSimulating = false);

		/**
		* @brief		Called after all updates are called. This can be used for camera transform actions.
		* @param[in]	dt								The delta time in seconds between two frames (update cycles).
		* @param[in]	notSimulating	If set to true only graphics for the simulation is updated and shown, if set to false all graphics (even debug graphics, such as light mesh) is shown.
		* @note		This function can be used to update periodically some data. When vsync is on, this function will be called approx. 60 times so at best
		*				td is 0.016 seconds or 16 milliseconds
		*				This can be used e.g. for a level editor, in which there is a play mode. If the play mode is on, everything is updated. If off, only necessary data like
		*				game object bounding box is udpated.
		*/
		virtual void lateUpdate(Ogre::Real dt, bool notSimulating = false);

		/**
		* @brief		Called after all logic updates in order to render the final interpolated transforms.
		* @param[in]	alpha								The interpolation alpha.
		*/
		virtual void render(Ogre::Real alpha);

		/**
		 * @brief		Actualizes the value for the given attribute
		 * @param[in]	attribute	The attribute to trigger the actualization of a value of either the game object or a component
		 */
		virtual void actualizeValue(Variant* attribute);

		/**
		 * @brief		Writes the attributes to XML
		 * @param[in]	propertiesXML		The properties XML node
		 * @param[in]	doc					The XML document
		 */
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc);

		/**
		 * @brief		Shows some debug data, if called a second time debug data will not be shown
		 */
		virtual void showDebugData(void);

		/**
		 * @brief		Gets if debug data is currently shown
		 * @return		show			true, if debug data is shown, else false
		 */
		bool getShowDebugData(void) const;
		
		/**
		 * @brief	Gets the id of this game object
		 * @return	id		The id of this game object
		 */
		unsigned long getId(void) const;

		/**
		 * @brief		Sets a name for this game object
		 * @param[in]	name		The name to set.
		 * @note		If the name does already exist, a number will be appended and incremented.
		 */
		void setName(const Ogre::String& name);

		/**
		 * @brief	Gets the name of this game object
		 * @return	name	The name of this game object
		 */
		const Ogre::String getName(void) const;

		/**
		 * @brief	Gets the unique name, that is its name and the id
		 * @return	name	The unique name of this game object
		 */
		const Ogre::String getUniqueName(void) const;

		/**
		 * @brief	Gets the scene manager for graphics handling for this level state, that also this game object does use.
		 * @return	sceneManager	The scene manager to get.
		 */
		Ogre::SceneManager* getSceneManager(void) const;

		/**
		 * @brief	Gets the scene node for graphics handling for this game object.
		 * @return	sceneNode	The scene node to get.
		 */
		Ogre::SceneNode* getSceneNode(void) const;

		/**
		 * @brief		Changes the category name, this game object belongs to
		 * @param[in]	oldCategory		The old category name to check if it does exist, after a new category is set
		 * @param[in]	newCategory		The new category name to set
		 */
		void changeCategory(const Ogre::String& oldCategory, const Ogre::String& newCategory);

		/**
		* @brief		Changes the category name, this game object belongs to
		* @param[in]	newCategory		The new category name to set
		*/
		void changeCategory(const Ogre::String& newCategory);

		/**
		 * @brief		Changes the render category name, this game object belongs to
		 * @param[in]	oldRenderCategory		The old render category name to check if it does exist, after a new render category is set
		 * @param[in]	newRenderCategory		The new render category name to set
		 */
		void changeRenderCategory(const Ogre::String& oldRenderCategory, const Ogre::String& newRenderCategory);

		/**
		* @brief		Changes the render category name, this game object belongs to
		* @param[in]	newRenderCategory		The new render category name to set
		*/
		void changeRenderCategory(const Ogre::String& newRenderCategory);
		
		/**
		 * @brief	Gets the category to which this game object does belong.
		 * @return	category	The category name to get.
		 * @note Categories are specified in a level editor and are read from XML.
		 *		 This is useful when doing ray-casts on graphics base or physics base or creating physics materials between categories.
		 */
		Ogre::String getCategory(void) const;

		/**
		 * @brief	Gets the render category to which this game object does belong.
		 * @return	renderCategory	The render category name to get.
		 * @note Render Categories are specified in a level editor and are read from XML.
		 *		 This is useful in order to manage, which game objects shall be renderd for which camera.
		 */
		Ogre::String getRenderCategory(void) const;

		/**
		 * @brief	Gets the category id to which this game object does belong.
		 * @return	categoryId	The category id to get.
		 * @note Categories are specified in a level editor and are read from XML. 
		 *		 This is useful when doing ray-casts on graphics base or physics base or creating physics materials between categories.
		 */
		unsigned int getCategoryId(void) const;

		/**
		 * @brief	Gets the render category to which this game object does belong.
		 * @return	renderCategoryId	The render category id to get.
		 * @note Render Categories are specified in a level editor and are read from XML.
		 *		 This is useful in order to manage, which game objects shall be renderd for which camera.
		 */
		unsigned int getRenderCategoryId(void) const;

		/**
		 * @brief		Sets an optional tag name for this game object, for better identification.
		 * @param[in]	tagName	The tag name to set
		 */
		void setTagName(const Ogre::String& tagName);

		/**
		 * @brief	Gets the tag name of game object.
		 * @return	tagName	The tag name to get.
		 * @note Tags are like sub-categories. E.g. several game objects may belong to the category 'Enemy', but one group may have a tag name 'Stone', the other 'Ship1', 'Ship2' etc.
		 *		 This is useful when doing ray-casts on graphics base or physics base or creating physics materials between categories, to further distinquish, which tag has been hit in order to remove different energy amount.
		 */
		Ogre::String getTagName(void) const;

		/**
		 * @brief	Gets the list of components of this game object.
		 * @return	componentsList	The components list to get
		 */
		GameObjectComponents* getComponents(void);

		/**
		 * @brief		Adds a new component to this game object.
		 * @param[in]	gameObjectCompPtr	The component ptr to add
		 * @note		Do not use this function inside a @GameObjectComponent::connect function. It will likely cause a crash. Instead use @addDelayedComponent
		 */
		void addComponent(GameObjectCompPtr gameObjectCompPtr);

		/**
		* @brief		Adds a new component to this game object. But it will not be added immediately but in the next update turn.
		* @param[in]	gameObjectCompPtr	The component ptr to add
		* @param[in]	bConnect	True if the added component shall also be connected
		*/
		void addDelayedComponent(GameObjectCompPtr gameObjectCompPtr, bool bConnect);

		/**
		 * @brief		Inserts a new component at the given index to this game object.
		 * @param[in]	gameObjectCompPtr	The component ptr to add
		 * @param[in]	index	The index where to insert
		 * @return		success Whether the index is in bounds and the component could be inserted
		 */
		bool insertComponent(GameObjectCompPtr gameObjectCompPtr, unsigned int index);

		/**
		 * @brief		Moves the component up, started by the given index
		 * @param[in]	index	The index to move up.
		 * @return		success Whether the index is in bounds and the component could be moved up
		 */
		bool moveComponentUp(unsigned int index);

		/**
		* @brief		Moves the component down, started by the given index
		* @param[in]	index	The index to move down.
		* @return		success Whether the index is in bounds and the component could be moved down
		*/
		bool moveComponentDown(unsigned int index);

		/**
		 * @brief		Moves the component to the given index
		 * @param[in]	index	The index to move.
		 * @return		success Whether the index is in bounds and the component could be moved
		 */
		bool moveComponent(unsigned int index);

		/**
		 * @brief		Gets the game object component base class weak pointer for the given index.
		 * @param[in]	index	The index to get the component for
		 * @return		The component weak ptr which must be made to a strong ptr by using the function @makeStrongPtr() or lock(), or nullptr if not found.
		 */
		boost::weak_ptr<GameObjectComponent> getComponentByIndex(unsigned int index);

		/**
		 * @brief		Gets index in list from the given component pointer
		 * @param[in]	index	The game object Component pointer to get the index in list
		 * @return		index	When the game object Component could not be found -1 will be delivered
		 */
		int getIndexFromComponent(GameObjectComponent* gameObjectComponent);

		/**
		* @brief		Gets occurrence index in list from the given component pointer
		* @param[in]	index	The game object Component pointer to get the occurrence index in list
		* @return		index	When the game object Component could not be found -1 will be delivered
		*/
		int getOccurrenceIndexFromComponent(GameObjectComponent* gameObjectComponent);

		/**
		 * @brief		Deletes the component of the given component class name and optional its occurence index
		 * @param[in]	componentClassName		The component class name to delete
		 * @param[in]	componentOccurrenceIndex	The optional component index. That is, a game object may have several components of the same type. E.g.
		 *										AnimationComponent, AnimationComponent, PhysicsActiveComponent, AnimationComponent. So which Animation component wants the developer delete?
		 *										This can be controlled by specifying the index. So for this example getComponent(AnimationComponent, 2) will delete the 3rd AnimationComponent.
		 * @return		success					Whether the component has been found and deleted
		 */
		bool deleteComponent(const Ogre::String& componentClassName, unsigned int componentOccurrenceIndex = 0);

		/**
		 * @brief		Deletes the component of the given pointer
		 * @param[in]	gameObjectComponent		The component pointer to delete
		 * @return		success					Whether the component has been found and deleted
		 */
		bool deleteComponent(GameObjectComponent* gameObjectComponent);

		/**
		 * @brief		Deletes the component of the given game object by index
		 * @param[in]	index	The index to delete at
		 * @return		success Whether the index is in bounds and the component could be deleted
		 */
		bool deleteComponentByIndex(unsigned int index);

		/**
		 * @brief		Sets the attribute position for this game object
		 * @param[in]	position	The position to set
		 */
		void setAttributePosition(const Ogre::Vector3& position);

		/**
		* @brief		Sets the attribute scale for this game object
		* @param[in]	scale	The scale to set
		*/
		void setAttributeScale(const Ogre::Vector3& scale);

		/**
		* @brief		Sets the attribute orientation for this game object
		* @param[in]	orientation	The orientation to set
		*/
		void setAttributeOrientation(const Ogre::Quaternion& orientation);

		/**
		* @brief		Sets the default direction vector for this game object
		* @param[in]	defaultDirection	The default direction to set
		* @Note			When an GameObject is created, take a look, how it is orientated in the scene, if e.g. the player does look to the camera,
		*				its default direction is 0,0,1, if the camera does look at the back of the player, its default direction is 0,0,-1
		*				A correct default direction is necessary, because other components will rely on that direction and work with it.
		*/
		void setDefaultDirection(const Ogre::Vector3& defaultDirection);

		/**
		 * @brief	Gets the default direction vector of this game object.
		 * @return	defaultDirection	the default direction vector of the game object
		 */
		Ogre::Vector3 getDefaultDirection(void) const;
		
		/**
		 * @brief	Gets the position of this game object.
		 * @return	position	the position of the game object
		 */
		///! @info: Those must never return a reference, because it may be that the values come from other engine and are not calculated yet and so scrap will be delivered!
		Ogre::Vector3 getPosition(void) const;
		
		/**
		 * @brief	Gets the orientation of this game object.
		 * @return	orientation	the orientation of the game object
		 */
		Ogre::Quaternion getOrientation(void) const;

		/**
		 * @brief	Gets the scale of this game object.
		 * @return	scale	the scale of the game object
		 */
		Ogre::Vector3 getScale(void) const;

		/**
		 * @brief	Gets the bounding box size of this game object.
		 * @return	scale	the scale of the game object
		 */
		const Ogre::Vector3 getSize(void) const;

		/**
		 * @brief	Gets offset vector by which the mesh start construction point is away from the center of its bounding box.
		 * @return	centerOffset	the center offset to get
		 */
		const Ogre::Vector3 getCenterOffset(void) const;

		/**
		 * @brief	Gets offset vector by which the mesh start construction point is away from the bottom of its bounding box.
		 * @return	centerOffset	the center offset to get
		 */
		const Ogre::Vector3 getBottomOffset(void) const;

		/**
		 * @brief	Gets the middle vector of the mesh size
		 * @return	middle	the middle vector to get
		 */
		Ogre::Vector3 getMiddle(void) const;

		/**
		 * @brief		Sets the client id, by which this game object will be controller. This is used in a network scenario.
		 * @param[in]	controlledByClientID	The client id to set
		 */
		void setControlledByClientID(unsigned int controlledByClientID);

		/**
		 * @brief	Gets the client id, by which this game object will be controller. This is used in a network scenario.
		 * @return	client Id	the client id
		 */
		unsigned int getControlledByClientID(void) const;

		/**
		 * @brief		Sets whether this game object is frequently moved (dynamic), if not set false here, this will increase the rendering performance.
		 * @param[in]	controlledByClientID	The client id to set
		 */
		void setDynamic(bool dynamic);

		/**
		 * @brief	Gets whether this game object is frequently moved (dynamic)
		 * @return	dynamic True if this game object is frequently moved, else false
		 */
		bool isDynamic(void) const;

		/**
		 * @brief		Sets whether this game object should be visible.
		 * @param[in]	visible				The visibility to set
		 */
		void setVisible(bool visible);

		/**
		* @brief	Gets whether this game object is visible
		* @return	visible True if this game object is visible, else false
		*/
		bool isVisible(void) const;

		/**
		* @brief		Activates this game object
		* @param[in]	activated		If set to true, the game object will be activated
		* @note		This can be used e.g. for all components that do something when activated
		*/
		void setActivated(bool activated);

		/**
		 * @brief		Sets this game object should be reflected.
		 * @param[in]	useReflection		If set to true, the game object will be reflected in scene.
		 * @note		This can only be used, if the 'ReflectionWorkspace' is set.
		 */
		void setUseReflection(bool useReflection);

		/**
		 * @brief	Gets whether this game object uses reflection
		 * @return	useReflection True if this game object uses reflection, else false
		 */
		bool getUseReflection(void) const;
		
		/**
		 * @brief		Sets whether this game object is global.
		 * @param[in]	global	If set to true, the game object will be global
		 * @note		This is useful if a project does consist of several scenes, which do share this game object, so when a change is made, it will be available for all scenes.
		 */
		void setGlobal(bool global);
		
		/**
		 * @brief	Gets whether this game object is global
		 * @return	global True if this game object is global, else false
		 */
		bool getGlobal(void) const;

		/**
		 * @brief		Sets whether to clamp y coordinate of center bottom of the game object to the next below/above game object.
		 * @param[in]	clampY	If set to true, the game object y coordinated will be clamped
		 * @note		This is useful when game object is loaded, so that it will be automatically placed upon the next lower game object.
		 *				Especially when the game object is a global one and will be loaded for different scenes, that start at a different height.
		 *				If there is no game object below, the next game object above is searched. If this also does not exist, the current y coordinate is just used.
		 */
		void setClampY(bool clampY);

		/**
		 * @brief	Gets whether this game object's center bottom y coordinate is clamped to the next below/above game object.
		 * @return	clampY True if this game object's y cooridnate is clamped, else false
		 */
		bool getClampY(void) const;

		/**
		 * @brief Sets reference id for this game object.
		 * @Note If this game object is referenced by another game object, a component with the same id as this game object can be get for activation etc.
		 * @param[in] referenceId The reference Id to set
		 */
		void setReferenceId(unsigned long referenceId);

		/**
		 * @brief Gets the reference id for the game object and component, that can be used in lua to create special mechanics effects.
		 * @return referenceId The reference Id to get
		 */
		unsigned long getReferenceId(void) const;

		/**
		 * @brief Sets the render queue index for this game object. That is the order in which this game object will be rendered.
		 * @param[in] renderQueueIndex The render queue index Possible values are from 0 to 255
		 */
		void setRenderQueueIndex(unsigned int renderQueueIndex);

		/**
		 * @brief Gets the render queue index. That is the order in which this game object will be rendered.
		 * @return renderQueueIndex The render queue index to get
		 */
		unsigned int getRenderQueueIndex(void) const;

		/**
		 * @brief Sets the render distance in meters til which the game object will be rendered.
		 * @param[in] renderDistance The render distance. Default set to 0, which means the game object will always be rendered.
		 */
		void setRenderDistance(unsigned int renderDistance);

		/**
		 * @brief  Gets the render distance in meters til which the game object will be rendered.
		 * @return renderDistance The render distance to get. Default set to 0, which means the game object will always be rendered.
		 */
		unsigned int getRenderDistance(void) const;

		/**
		 * @brief Sets the lod distance in meters til which the game object vertex count will be reduced.
		 * @param[in] lodDistance The lod distance.
		 * @note	Default set to 0, which means that for this game object no lod levels will be generated and the mesh never reduced.
		 *			There are always 4 levels of reduction, beginning at the given lod distance and increasing based on the bounding radius of the mesh of the game object.
		 */
		void setLodDistance(Ogre::Real lodDistance);

		/**
		 * @brief  Gets the lod distance in meters til which the game object vertex count will be reduced.
		 * @return renderDistance The lod distance to get.
		 */
		Ogre::Real getLodDistance(void) const;

		/**
		 * @brief Sets the shadow rendering distance in meters til which the game object's shadow will be rendered.
		 * @param[in] shadowRenderingDistance The shadow rendering distance. Default set to 20 meters, which means the game object's shadow will be rendered up to 20 meters. If setting to 0, it will always be rendered.
		 */
		void setShadowRenderingDistance(unsigned int shadowRenderingDistance);

		/**
		 * @brief  Gets the shadow rendering distance in meters til which the game object's shadow will be rendered.
		 * @return shadowRenderingDistance The shadow rendering distance to get. Default set to 20, which means the game object's shadow will be rendered up to 20 meters. If setting to 0, it will always be rendered.
		 */
		unsigned int getShadowRenderingDistance(void) const;

		/**
		 * Performs a ray cast and clamps the game object y coordinate
		 * @note		This is useful when game object is loaded, so that it will be automatically placed upon the next lower game object.
		 *				Especially when the game object is a global one and will be loaded for different scenes, that start at a different height.
		 *				If there is no game object below, the next game object above is searched.If this also does not exist, the current y coordinate is just used.
		 * @return	    success, clampedY If success is true the clamped y coordinate can be used
		 */
		std::pair<bool, Ogre::Real> performRaycastForYClamping(void);

		/**
		 * @brief Sets whether to show a bounding box around this game object.
		 * @param[in] show If set to true, the bounding box will be shown, else not.
		 */
		void showBoundingBox(bool show);

		/**
		 * @brief  Gets attribute pointer for the given attribue name, or null if does not exist.
		 * @return attribute The attribute pointer to get.
		 */
		Variant* getAttribute(const Ogre::String& attributeName);

		/**
		 * @brief  Gets a paired list of attributes (name, attribute pointer).
		 * @return attributeList The attribute list to get.
		 */
		std::vector<std::pair<Ogre::String, Variant*>>& getAttributes(void);

		/**
		 * @brief  Gets the game object type.
		 * @return type The game object type to get.
		 */
		GameObject::eType getType(void) const;

		/**
		 * @brief		Sets the original mesh name, when the mesh could not be loaded. This is necessary to set because:
		 *				When a mesh could not be loaded (resource group not set properly, etc.) then 'missing.mesh' is used as placeholder, but when a scene is saved,
		 *				'missing.mesh' would be stored, which would corrupt the scene. So original mesh name will be stored.
		 * @param[in]	originalMeshName	The original mesh name to set
		 */
		void setOriginalMeshNameOnLoadFailure(const Ogre::String& originalMeshName);

		/**
		 * @brief	Gets original mesh name when a scene is stored, so that there are no 'missing.mesh' names stored.
		 * @return	originalMeshName The original mesh name to get
		 */
		Ogre::String getOriginalMeshNameOnLoadFailure(void) const;

		/**
		* @brief		Sets custom data for this game object as a string
		* @param[in]	customDataString	The custom data string to set.
		* @note			This can be used e.g. by a prior created component to set custom data for another component, that is created later to behave different.
		*/
		void setCustomDataString(const Ogre::String& customDataString);

		/**
		* @brief	Gets custom data for this game object as a string
		* @return	customDataString	The custom data string to get.
		* @note			This can be used e.g. by a prior created component to set custom data for another component, that is created later to behave different.
		*/
		Ogre::String getCustomDataString(void) const;

		/**
		* @brief		Gets whether this game object has been selected or not
		* @return		selected	True if the game object is selected, else false
		*/
		bool isSelected(void) const;

		/**
		* @brief		Sets whether internally the movable object should be destroyed or not
		* @param[in]	doNotDestroy	If set to true, the movable object will not be destroyed
		* @note			This must be used when a movable object is created outside this game object and cannot be destroyed by the scene manager, like e.g. terra, or ocean etc.
		*/
		void setDoNotDestroyMovableObject(bool doNotDestroy);
		
		/**
		* @brief		Gets the lua script pointer, if a component of this game object is of the type LuaScriptComponent
		* @return		luaScript	The lua script pointer to get. NULL if it does not exist
		*/
		LuaScript* getLuaScript(void) const;

		/**
		* @brief		Gets the connected game object. E.g. via TagPointComponent a weapon is tagged to a player, so get back from the weapon the player game objects that holds the weapon.
		* @return		weakConnectedGameObjectPtr	The weak connected game object pointer
		*/
		boost::weak_ptr<GameObject> getConnectedGameObjectPtr(void) const;

		/**
		* @brief		Gets the data block names this game object owns.
		* @return		datablockNames	The list of datablock names.
		*/
		std::vector<Ogre::String> getDatablockNames(void);

		/**
		* @brief		Sets whether all components of the game objects shall be connected before all other game objects and components.
		*				Necessary e.g. if a vehicle chassis is the prior game object for all other tire joints.
		* @param[in]	doNotDestroy	If set to true, the movable object will not be destroyed
		* @note			This must be used when a movable object is created outside this game object and cannot be destroyed by the scene manager, like e.g. terra, or ocean etc.
		*/
		void setConnectPriority(bool bConnectPriority);
	public:

		/**
		 * @brief	Gets the movable object for graphics handling for this game object for the given type.
		 * @return	movableObject	The movable object to get.
		 */
		template <class MovableType>
		MovableType* getMovableObject(void) const
		{
			return dynamic_cast<MovableType*>(this->movableObject);
		}

		/**
		 * @brief	Gets the movable object (unsafe) for graphics handling for this game object for the given type.
		 * @return	movableObject	The movable object to get.
		 * @note	Only use this function, if you are sure, its the kind of movable object you wish, its faster but unsafe.
		 */
		template <class MovableType>
		MovableType* getMovableObjectUnsafe(void) const
		{
			return static_cast<MovableType*>(this->movableObject);
		}

		/**
		 * @brief	Gets the movable object for graphics handling for this game object.
		 * @return	movableObject	The movable object to get.
		 */
		Ogre::MovableObject* getMovableObject(void) const
		{
			return this->movableObject;
		}
		
		/**
		 * @brief		Gets the component weak pointer from component id.
		 * @tparam		ComponentType				The concrete component type to get
		 * @param[in]	componentClassId			The component class id to get the component for
		 * @param[in]	componentOccurrenceIndex	The optional component index (just 0 if not required). That is, a game object may have several components of the same type. E.g.
		 *											AnimationComponent, AnimationComponent, PhysicsActiveComponent, AnimationComponent. So which Animation component wants the developer have?
		 *											This can be controlled by specifying the index. So for this example getComponent(AnimationComponent, 2) will deliver the 3rd AnimationComponent. 
		 *											If there is no such component at the given index nullptr will be delivered.
		 * @return		The component weak ptr which must be made to a strong ptr by using the function @makeStrongPtr() or lock(), or nullptr if not found.
		 */
		template <class ComponentType>
		boost::weak_ptr<ComponentType> getComponent(unsigned int componentClassId, unsigned int componentOccurrenceIndex)
		{
			for (const auto& component : this->gameObjectComponents)
			{
				if (std::get<CLASS_ID>(component) == componentClassId || std::get<PARENT_CLASS_ID>(component) == componentClassId || std::get<PARENT_PARENT_CLASS_ID>(component) == componentClassId)
				{
					if (std::get<COMPONENT>(component)->getOccurrenceIndex() == componentOccurrenceIndex)
					{
						GameObjectCompPtr baseCompPtr(std::get<COMPONENT>(component));
						// If already tagged for destruction, do not get it!
						if (true == baseCompPtr->bTaggedForRemovement)
						{
							return boost::weak_ptr<ComponentType>();
						}

						// Cast from base the the sub type
						boost::shared_ptr<ComponentType> subCompPtr(boost::dynamic_pointer_cast<ComponentType>(baseCompPtr));

						// Because of circular references between GameObject and GameObjectComponent, the Components, that are belonging to the GameObject
						// must have a weak binding
						boost::weak_ptr<ComponentType> weakSubCompPtr(subCompPtr);
						return weakSubCompPtr;
					}
				}
			}
			// Component not found, return empty component
			return boost::weak_ptr<ComponentType>();
		}

		/**
		 * @brief		Gets the component type count.
		 * @tparam		ComponentType				The concrete component type to calculate its count.
		 * @return		The count or 0 if does not exist.
		 */
		template <class ComponentType>
		unsigned short getComponentCount(void)
		{
			unsigned short count = 0;
			for (const auto& component : this->gameObjectComponents)
			{
				GameObjectCompPtr baseCompPtr(std::get<COMPONENT>(component));
				// Cast from base the the sub type
				boost::shared_ptr<ComponentType> subCompPtr(boost::dynamic_pointer_cast<ComponentType>(baseCompPtr));
				if (nullptr != subCompPtr)
				{
					count++;
				}
			}

			return count;
		}

		/**
		 * @brief		Gets the component type count from the given component name.
		 * @param[in]	componentName				The name to count for
		 * @param[in]	allowDerivatives			The optional flag to set if derivatives of the given class name are allowed. E.g. componentName = "PhysicsActiveComponent", 
		 *											but actual component is "PhysicsRagDollComponent" which is a derivative and this will also be counted.
		 * @return		The count or 0 if does not exist.
		 */
		unsigned short getComponentCount(const Ogre::String& componentName, bool allowDerivatives = false);

		/**
		 * @brief		Gets the component weak pointer from component name.
		 * @tparam		ComponentType				The concrete component type to get
		 * @param[in]	componentName				The name to get the component for
		 * @param[in]	allowDerivatives			The optional flag to set if derivatives of the given class name are allowed. E.g. componentName = "PhysicsActiveComponent", 
		 *											but actual component is "PhysicsRagDollComponent" which is a derivative and valid pointer will also be received.
		 * @return		The component weak ptr which must be made to a strong ptr by using the function @makeStrongPtr() or lock(), or nullptr if not found. Default flag is set to false.
		 */
		template <class ComponentType>
		boost::weak_ptr<ComponentType> getComponentFromName(const Ogre::String& componentName, bool allowDerivatives = false)
		{
			if (true == componentName.empty())
			{
				return boost::weak_ptr<ComponentType>();
			}

			for (const auto& component : this->gameObjectComponents)
			{
				if (false == allowDerivatives)
				{
					// Note: Name is a custom component name, which can be set by the designer, class name is the real component name
					if (std::get<COMPONENT>(component)->getName() == componentName || std::get<COMPONENT>(component)->getClassName() == componentName)
					{
						GameObjectCompPtr baseCompPtr(std::get<COMPONENT>(component));
						// If already tagged for destruction, do not get it!
						if (true == baseCompPtr->bTaggedForRemovement)
						{
							return boost::weak_ptr<ComponentType>();
						}

						// Cast from base the the sub type
						boost::shared_ptr<ComponentType> subCompPtr(boost::dynamic_pointer_cast<ComponentType>(baseCompPtr));

						// Because of circular references between GameObject and GameObjectComponent, the Components, that are belonging to the GameObject
						// must have a weak binding
						boost::weak_ptr<ComponentType> weakSubCompPtr(subCompPtr);
						return weakSubCompPtr;
					}
				}
				else
				{
					// Note: Name is a custom component name, which can be set by the designer, class name is the real component name
					if (std::get<COMPONENT>(component)->getName() == componentName || std::get<COMPONENT>(component)->getClassName() == componentName || std::get<COMPONENT>(component)->getParentClassName() == componentName || std::get<COMPONENT>(component)->getParentParentClassName() == componentName)
					{
						GameObjectCompPtr baseCompPtr(std::get<COMPONENT>(component));
						// If already tagged for destruction, do not get it!
						if (true == baseCompPtr->bTaggedForRemovement)
						{
							return boost::weak_ptr<ComponentType>();
						}

						// Cast from base the the sub type
						boost::shared_ptr<ComponentType> subCompPtr(boost::dynamic_pointer_cast<ComponentType>(baseCompPtr));
						// Because of circular references between GameObject and GameObjectComponent, the Components, that are belonging to the GameObject
						// must have a weak binding
						boost::weak_ptr<ComponentType> weakSubCompPtr(subCompPtr);
						return weakSubCompPtr;
					}
				}
			}
			// Component not found, return empty component
			return boost::weak_ptr<ComponentType>();
		}

		/**
		* @brief		Deletes the component of the given game object id
		* @tparam		ComponentType		The component type template name
		* @param[in]	componentOccurrenceIndex	The optional component index. That is, a game object may have several components of the same type. E.g.
		*										AnimationComponent, AnimationComponent, PhysicsActiveComponent, AnimationComponent. So which Animation component wants the developer delete?
		*										This can be controlled by specifying the index. So for this example getComponent(AnimationComponent, 2) will delete the 3rd AnimationComponent.
		* @return		success					Whether the component has been found and deleted
		*/
		template <class ComponentType>
		bool deleteComponent(unsigned int componentOccurrenceIndex = 0)
		{
			unsigned int componentClassId = ComponentType::getStaticClassId();
			Ogre::String componentClassName = ComponentType::getStaticClassName();
			return this->internalDeleteComponent(componentClassName, componentClassId, componentOccurrenceIndex);
		}

		/**
		 * @brief		Gets the component weak pointer from component name.
		 * @tparam		ComponentType			The concrete component type to get
		 * @param[in]	componentClassName		The component class name to get the component for
		 * @param[in]	componentOccurrenceIndex	The optional component index. That is, a game object may have several components of the same type. E.g.
		 *										AnimationComponent, AnimationComponent, PhysicsActiveComponent, AnimationComponent. So which Animation component wants the developer have?
		 *										This can be controlled by specifying the index. So for this example getComponent(AnimationComponent, 2) will deliver the 3rd AnimationComponent. 
		 *										If there is no such component at the given index nullptr will be delivered.
		 * @return		The component weak ptr which must be made to a strong ptr by using the function @makeStrongPtr() or lock(), or nullptr if not found.
		 */
		template <class ComponentType>
		boost::weak_ptr<ComponentType> getComponent(const Ogre::String& componentClassName, unsigned int componentOccurrenceIndex = 0)
		{
			return getComponent<ComponentType>(NOWA::getIdFromName(componentClassName), componentOccurrenceIndex);
		}

		/**
		 * @brief		Gets the component weak pointer directly from component type.
		 * @note		This is the easiest to use function to get from a game object its component.
		 * @tparam		ComponentType			The concrete component type to get
		 * @param[in]	componentOccurrenceIndex	The optional component index. That is, a game object may have several components of the same type. E.g.
		 *										AnimationComponent, AnimationComponent, PhysicsActiveComponent, AnimationComponent. So which Animation component wants the developer have?
		 *										This can be controlled by specifying the index. So for this example getComponent(AnimationComponent, 2) will deliver the 3rd AnimationComponent.
		 *										If there is no such component at the given index nullptr will be delivered.
		 * @return		The component weak ptr which must be made to a strong ptr by using the function @makeStrongPtr() or lock(), or nullptr if not found.
		 */
		template <class ComponentType>
		boost::weak_ptr<ComponentType> getComponent(unsigned int componentOccurrenceIndex = 0)
		{
			return getComponent<ComponentType>(ComponentType::getStaticClassId(), componentOccurrenceIndex);
		}

		/// Is required for lua to be unique function
		template <class ComponentType>
		boost::weak_ptr<ComponentType> getComponentWithOccurrence(unsigned int componentOccurrenceIndex = 0)
		{
			return getComponent<ComponentType>(ComponentType::getStaticClassId(), componentOccurrenceIndex);
		}
	public:
		// Attribute constants
		static const Ogre::String AttrId(void) { return "Id"; }
		static const Ogre::String AttrName(void) { return "Name"; }
		static const Ogre::String AttrCategoryId(void) { return "Category Id"; }
		static const Ogre::String AttrCategory(void) { return "Category"; }
		static const Ogre::String AttrRenderCategoryId(void) { return "Render Category Id"; }
		static const Ogre::String AttrRenderCategory(void) { return "Render Category"; }
		static const Ogre::String AttrMeshName(void) { return "Mesh Name"; }
		static const Ogre::String AttrTagName(void) { return "Tag Name"; }
		static const Ogre::String AttrDataBlock(void) { return "Data Block "; }
		static const Ogre::String AttrUseReflection(void) { return "Use Reflection"; }
		static const Ogre::String AttrControlledByClientID(void) { return "Client Id"; }
		static const Ogre::String AttrCastShadows(void) { return "Cast Shadows"; }
		static const Ogre::String AttrVisible(void) { return "Visible"; }
		static const Ogre::String AttrDynamic(void) { return "Dynamic"; }
		static const Ogre::String AttrSize(void) { return "Size"; }
		static const Ogre::String AttrPosition(void) { return "Position"; }
		static const Ogre::String AttrScale(void) { return "Scale"; }
		static const Ogre::String AttrOrientation(void) { return "Orientation"; }
		static const Ogre::String AttrDefaultDirection(void) { return "Global Mesh Direction"; }
		static const Ogre::String AttrGlobal(void) { return "Global"; }
		static const Ogre::String AttrClampY(void) { return "Clamp Y"; }
		static const Ogre::String AttrReferenceId(void) { return "Reference Id"; }
		static const Ogre::String AttrRenderQueueIndex(void) { return "Render Queue Index"; }
		static const Ogre::String AttrRenderDistance(void) { return "Render Distance"; }
		static const Ogre::String AttrLodDistance(void) { return "Lod Distance"; }
		static const Ogre::String AttrLodLevels(void) { return "Lod Levels"; }
		static const Ogre::String AttrShadowDistance(void) { return "Shadow Distance"; }

		// Attribute actions
		static const Ogre::String AttrActionNeedRefresh(void) { return "NeedRefresh"; }
		static const Ogre::String AttrActionColorDialog(void) { return "ColorDialog"; }
		static const Ogre::String AttrActionForceSet(void) { return "ForceSet"; }
		static const Ogre::String AttrActionMultiLine(void) { return "MultiLine"; }
		static const Ogre::String AttrActionFileOpenDialog(void) { return "FileOpenDialog"; }
		static const Ogre::String AttrActionSeparator(void) { return "Separator"; }
		static const Ogre::String AttrActionLabel(void) { return "Label"; }
		static const Ogre::String AttrActionImage(void) { return "Image"; }
		static const Ogre::String AttrActionNoUndo(void) { return "NoUndo"; }
		static const Ogre::String AttrActionGenerateLuaFunction(void) { return "GenerateLuaFunction"; }
		static const Ogre::String AttrActionReadOnly(void) { return "ReadOnly"; }
		static const Ogre::String AttrActionLuaScript(void) { return "LuaScript"; }

		// Custom data strings
		static const Ogre::String AttrCustomDataSkipCreation(void) { return "SkipCreation"; }
	private:
		bool internalDeleteComponent(const Ogre::String& componentClassName, unsigned int componentClassId, unsigned int componentOccurrenceIndex = 0);

		void refreshSize(void);
		void setDatablock(NOWA::Variant* attribute);
		void setDataBlocks(const std::vector<Ogre::String>& loadedDatablocks);
		void setConnectedGameObject(boost::weak_ptr<GameObject> weakConnectedGameObjectPtr);
		void setDataBlockPbsReflectionTextureName(const Ogre::String& textureName);
		void resetVariants();
		void resetChanges();
		void actualizeComponentsIndices(void);

		void earlyConnect(void);

		bool connectPriority(void);

		// Is necessary, because there is an event, which is sent to all listener, that this game object has been made global at runtime, in order to cut/paste potential resources to the corresponding folder
		// This function is only called in actualizeValue and may not be called on other places! Please call the official @setGlobal(...) if desired.
		void setInternalAttributeGlobal(bool isGlobal);

		void setLodLevels(unsigned int lodLevels);
	protected:
		Ogre::SceneManager* sceneManager;
		Ogre::SceneNode* sceneNode;
		Ogre::MovableObject* movableObject;
		eType type;
		Ogre::RaySceneQuery* clampObjectQuery;

		Variant* id;
		Variant* name;
		Variant* categoryId;
		Variant* category;
		Variant* renderCategoryId;
		Variant* renderCategory;
		Variant* meshName;
		Variant* tagName;
		std::vector<Variant*> dataBlocks;
		Variant* useReflection;
		GameObjectComponents gameObjectComponents;
		Variant* controlledByClientID;
		Variant* size;
		Ogre::Vector3 centerOffset;
		Variant* castShadows;
		Variant* visible;
		Variant* dynamic;
		Variant* position;
		Variant* scale;
		Variant* orientation;
		Variant* defaultDirection;
		Variant* global;
		Variant* clampY;
		Variant* referenceId;
		Variant* renderQueueIndex;
		Variant* renderDistance;
		Variant* lodDistance;
		Variant* lodLevels;
		Variant* shadowRenderingDistance;

		Ogre::WireAabb* boundingBoxDraw;
		unsigned long priorId;
		GameObjectPtr connectedGameObjectPtr;
		// Custom order is important
		std::vector<std::pair<Ogre::String, Variant*>> attributes;
		Ogre::String originalMeshName;
		Ogre::Vector3 oldScale;
		Ogre::String customDataString;
		bool selected;
		bool doNotDestroyMovableObject;
		bool bShowDebugData;
		LuaScript* luaScript;
		bool bConnectPriority;
		bool doNotTouchVisibleAttribute;

		std::vector<std::pair<GameObjectCompPtr, bool>> delayedAddCommponentList;
	};

}; // namespace end

#endif
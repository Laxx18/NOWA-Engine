#ifndef GAME_OBJECT_COMPONENT_H
#define GAME_OBJECT_COMPONENT_H

#include "utilities/rapidxml.hpp"
#include "GameObject.h"
#include "boost/enable_shared_from_this.hpp"

struct lua_state;

namespace NOWA
{
	class EXPORTED GameObjectComponent : public boost::enable_shared_from_this<GameObjectComponent>
	{
		friend class GameObjectFactory;
		friend class GameObject;
	public:
		GameObjectComponent();

		virtual ~GameObjectComponent();

		/**
		 * @brief		Initializes the component by parsing the property element from XML
		 * @note		Only create new variants in init function, if they do not exist, do not create each time new variants here,
		 *				because init is called x-times, if snapshot of game objects is created and values reset.
		 * @param[in]	propertyElement	The property element to parse
		 * @param[in]	file			The optional file, if specified
		 * @return		success			true, if all the elements could be parsed, else false
		 */
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& file = Ogre::String());
		
		/**
		 * @brief		Post initializes the component. At this time its game object owner is available. This is called when a game object component is loaded or newly created.
		 * @return		success			true, if the post initialisation did work, else false
		 */
		virtual bool postInit(void);

		/**
		 * @brief		This function is called when when all game objects and components are already post-initialized and available. It can be used to connect data with other objects.
		 * @param[in]	cloned			if set to true, this connection should be used to get the target game object by its prior id, since it has been cloned
		 * @return		success			true, if the connection did work, else false
		 */
		virtual bool connect(void);

		/**
		 * @brief		Disconnects data to other game objects
		 * @return		success			true, if the disconnection did work, else false
		 */
		virtual bool disconnect(void) { return true; }

		/**
		 * @brief		In each component @onCloned is called so that each component, that has connections to other game objects,
		 *				can react and find the target game object, that has been cloned by its prior id. So this function should be implemented by each component,
		 *				that has connection to other game objects, to get the new id when being cloned.
		 * return		success			true, if the cloning did work, else false
		 */
		virtual bool onCloned(void) { return true; }

		/**
		 * @brief		Chance to react when this component is removed
		 */
		virtual void onRemoveComponent(void);

		/**
		* @brief		Chance to react when another component (than this one) has been removed
		* @param[in]	index	The index of the removed component
		*/
		virtual void onOtherComponentRemoved(unsigned int index);

		/**
		* @brief		Chance to react when another component has been added
		* @param[in]	index	The index of the added component
		*/
		virtual void onOtherComponentAdded(unsigned int index);

		/**
		 * @brief		Pauses the game object component. When the AppState is just paused.
		 */
		virtual void pause(void) { };

		/**
		 * @brief		Resumes the game object component. When the AppState is resumed.
		 */
		virtual void resume(void) { };

		/**
		 * @brief		Called on each update tick
		 * @param[in]	dt								The delta time in seconds between two frames (update cycles).
		 * @param[in]	notSimulating	If set to false (simulating) only graphics for the simulation is updated and shown, if set to true (not simulating) 
		 *								all graphics (even debug graphics, such as light mesh) is shown.
		 * @note		This function can be used to update periodically some data. When vsync is on, this function will be called approx. 60 times so at best
		 *				td is 0.016 seconds or 16 milliseconds
		 *				This can be used e.g. for a level editor, in which there is a play mode. If the play mode is on, everything is updated. If off, only necessary data like
		 *				game object bounding box is udpated.
		 */
		virtual void update(Ogre::Real dt, bool notSimulating = false)
		{

		}

		/**
		 * @brief		Actualizes the value for the given attribute
		 * @param[in]	attribute	The attribute to trigger the actualization of a value of a component
		 */
		virtual void actualizeValue(Variant* attribute);

		/**
		 * @brief		Writes the attributes to XML
		 * @param[in]	propertiesXML	The properties XML node
		 * @param[in]	doc				The XML document
		 * @param[in]	filePath		The file path (file path til folder without slash at the end) at which the resources are exported
		 */
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath);

		/**
		 * @brief	Gets the component name
		 * @return	name	The component name to get
		 */
		virtual Ogre::String getClassName(void) const = 0;

		/**
		 * @brief	Gets the components parent name
		 * @return	parentName	The components parent name to get
		 * @note	When a component is derived from a base or parent component the parent name will be delivered.
		 */
		virtual Ogre::String getParentClassName(void) const = 0;

		/**
		* @brief	Gets the components parent name
		* @return	parentParentName	The components parent parent name to get
		* @note	When a component is derived from a base or parent parent component the parent parent name will be delivered.
		*/
		virtual Ogre::String getParentParentClassName(void) const
		{
			return "";
		}

		/**
		 * @brief		Clones a component if all its attributes (where possible) and delivers a fresh new copy.
		 * @param[in]	clonedGameObjectPtr		The game object to set as owner for the cloned component.
		 * @return		clonnedComponent	The fresh new cloned component or nullptr if not possible to clone
		 */
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) = 0;

		/**
		 * @brief	Gets the position of this component.
		 * @return	position	the position of the component
		 */
		virtual Ogre::Vector3 getPosition(void) const;

		/**
		 * @brief	Gets the orientation of this component.
		 * @return	orientation	the orientation of the component
		 */
		virtual Ogre::Quaternion getOrientation(void) const;

		/**
		 * @brief	Gets the id of this component
		 * @return	id		The id of this component
		 */
		virtual unsigned int getClassId(void) const;

		/**
		 * @brief	Gets the id of the parent of this component
		 * @return	id		The id the parent of this component
		 */
		virtual unsigned int getParentClassId(void) const;

		/**
		* @brief	Gets the id of the parent parent of this component
		* @return	id		The id the parent parent of this component
		*/
		virtual unsigned int getParentParentClassId(void) const;

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
		 * @brief		Activates this component
		 * @param[in]	activated		If set to true, the component will be activated
		 * @note		This can be used e.g. by the TimeTriggerComponent to control, when a component should do something and for how long.
		 *				E.g. Starting an active slider joint component moving after 10 seconds.
		 */
		virtual void setActivated(bool activated)
		{

		}

		/**
		 * @brief		Gets whether this component is activated
		 * @return		true If the component is activated, else false
		 */
		virtual bool isActivated(void) const
		{
			return true;
		}

		/**
		 * @brief	Gets the owner game object for this component.
		 * @return	owner		The owner game object to get
		 */
		GameObjectPtr getOwner(void) const;

		/**
		 * @brief		Sets the owner game object for this component.
		 * @param[in]	owner		The owner game object to set
		 */
		void setOwner(GameObjectPtr gameObjectPtr);

		/**
		 * @brief		Gets the attribute from the given attribute name for direct manipulation, or null of does not exist.
		 * @param[in]	attributeName	The attribute name to set
		 * @return		attribute		The attribute pointer to get
		 */
		Variant* getAttribute(const Ogre::String& attributeName);

		/**
		 * @brief		Gets all attributes.
		 * @return		attributsList		The attributes list to get
		 */
		std::vector<std::pair<Ogre::String, Variant*>>& getAttributes(void);

		/**
		 * @brief	Gets the occurrence index of this component. That is if there are two components of the same name and this component is the second one, then the index is 1
		 * @return	occurrenceIndex		The occurrence index to get
		 */
		unsigned int getOccurrenceIndex(void) const;

		/**
		 * @brief	Gets the index of this component. A game object may have x-components so this functions gets the index of this component.
		 * @return	index		The index to get
		 */
		unsigned int getIndex(void) const;

		/**
		 * @brief Sets an optional name for this component.
		 * @param[in] name The name to set
		 * @note The name must be unique within a game object and all its components. If an already existing name is set, internally the name will be incremented by an index.
		 */
		void setName(const Ogre::String& name);

		/**
		 * @brief Gets the name of this component
		 * @return name The name to get
		 */
		Ogre::String getName(void) const;

		/**
		 * @brief Sets reference id for this component.
		 * @Note If it is set to the same id as the game object and the game object is referenced by another game object, this component can be get for activation etc.
		 * @param[in] referenceId The reference Id to set
		 */
		void setReferenceId(unsigned long referenceId);

		/**
		 * @brief Gets the reference id for the game object and component, that can be used in lua to create special mechanics effects.
		 * @return referenceId The reference Id to get
		 */
		unsigned long getReferenceId(void) const;

		/**
		 * @brief Sets whether this component is expaned. Just useful, if an editor is used for UI.
		 * @param[in] bIsExpanded The flag to set
		 */
		void setIsExpanded(bool bIsExpanded);

		/**
		 * @brief Gets whether this component is expaned. Just useful, if an editor is used for UI.
		 * @return true if the component is expanded, else false.
		 */
		bool getIsExpanded(void) const;

		/**
		 * @brief Gets whether this component has the flag connect priority set. In this case this component will be connected before all other components.
		 * @return true if the component has the flag connect priority set, else false.
		 */
		bool getConnectPriority(void) const;
	public:
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("GameObjectComponent");
		}

		/**
		 * @brief	Gets an info text for this component. Like requirements, which must be met in order to add the component or what this component does.
		 * @return	infoText		The info text
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "";
		}

		/**
		 * @brief	This function is called for all components, when lua api is constructed. Thus, a component can use this function to register, itself and its methods for lua.
		 * @param[in]		lua						The existing lua state for registration
		 * @param[in|out]	gameObject				The existing game object lua registration, in order to register getMyComponent, getMyComponent(unsigned int occurenceIndex), getMyComponentFromName
		 * @param[in|out]	gameObjectController	The existing game object controller lua registration, in order to register correct cast (castMyComponent), so that intellisense in zero brane studio will work correctly
		 */
		
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		 * @brief		Gets whether this component can be added to the game object.
		 * @param[in]	gameObject		The existing game object, for which this component should be added.
		 * @return		true If the component can be added, else false
		 * @note		Each component can set its constraints. That is, if its valid to add the component on the given game object' components constellation.
		 *				A simple case would be: It should not be possible to add a physics component for one game object twice.
		 */
		static bool canStaticAddComponent(GameObject* gameObject)
		{
			return true;
		}

		/**
		 * @brief		Creates a strong pointer from a weak pointer
		 * @tparam		Type		The type of class for strong pointer creation.
		 * @param[in]	weakPtr		The weak pointer to created the strong shared pointer from.
		 */
		template <class Type>
		static boost::shared_ptr<Type> makeStrongPtr(boost::weak_ptr<Type> weakPtr)
		{
			if (!weakPtr.expired())
			{
				return boost::shared_ptr<Type>(weakPtr);
			}
			else
			{
				return boost::shared_ptr<Type>();
			}
		}
	public:
		static const Ogre::String AttrName(void) { return "Name"; }
		static const Ogre::String AttrReferenceId(void) { return "Reference Id"; }

		// Custom data strings
		static const Ogre::String AttrCustomDataNewCreation(void) { return "NewCreation"; }
	protected:
		void eraseVariants(std::vector<Variant*>& container, size_t offset);

		void eraseVariant(std::vector<Variant*>& container, size_t index);

		void resetVariants();

		void cloneBase(GameObjectCompPtr& otherGameObjectCompPtr);

		template <class T>
		T* getPriorComponentByType(void) const
		{
			T* resultComponent = nullptr;

			for (short i = this->index - 1; i >= 0; i--)
			{
				auto component = std::get<COMPONENT>(this->gameObjectPtr->getComponents()->at(i));
				auto compPtr = boost::dynamic_pointer_cast<T>(component);
				if (nullptr != compPtr)
				{
					resultComponent = compPtr.get();
					break;
				}
			}

			return resultComponent;
		}

		template <class T>
		T* getNextComponentByType(void) const
		{
			T* resultComponent = nullptr;

			for (short i = this->index + 1; i < this->gameObjectPtr->getComponents()->size(); i++)
			{
				auto component = std::get<COMPONENT>(this->gameObjectPtr->getComponents()->at(i));
				auto compPtr = boost::dynamic_pointer_cast<T>(component);
				if (nullptr != compPtr)
				{
					resultComponent = compPtr.get();
					break;
				}
			}

			return resultComponent;
		}

		/**
		 * @brief		Gets a validated component name. That is, if the given one does already exist, a slithly different and unique name will be delivered.
		 * @param[in]	componentName		The component name to validate
		 */
		void getValidatedComponentName(Ogre::String& componentName);

		/**
		 * @brief		Adds file path data for the given attribute, file location path and file open dialog in the correct folder.
		 * @param[in]	attribute		The attribute to add the file path data.
		 */
		void addAttributeFilePathData(Variant* attribute);
	private:
		void resetChanges();
	protected:
		GameObjectPtr gameObjectPtr;
		// Custom order is important
		std::vector<std::pair<Ogre::String, Variant*>> attributes;
		Variant* name;
		Variant* referenceId;
		short occurrenceIndex;
		short index;
		bool bShowDebugData;
		Ogre::String customDataString;
		bool bConnectPriority;
	private:
		bool bConnectedSuccess;
		bool bIsExpanded;
		bool bTaggedForRemovement;
	};

}; //Namespace end

#endif
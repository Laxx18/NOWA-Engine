#ifndef GAME_OBJECT_FACTORY_H
#define GAME_OBJECT_FACTORY_H

#include "defines.h"
#include <map>
#include <boost/weak_ptr.hpp>
#include "GameObjectComponent.h"
#include "GenericFactory.h"

namespace NOWA
{

	class EXPORTED GameObjectFactory
	{
	protected:
		GenericObjectFactory<GameObjectComponent, unsigned int> componentFactory;

	public:
		typedef boost::shared_ptr<GameObject> GameObjectPtr;
		// forward declaration of typedefs is not possible, therefore the the typedef is 
		// transfered to the first class of the 2 classes that are bidirectional associated
		typedef boost::shared_ptr<GameObjectComponent> GameObjectCompPtr;
	public:
		friend class DotSceneImportModule;

		/*
		* @brief Creates a new game object from XML or sets data for existing game object
		*/
		GameObjectPtr createOrSetGameObjectFromXML(rapidxml::xml_node<>* xmlNode, Ogre::SceneManager* sceneManager, Ogre::SceneNode* sceneNode,
			Ogre::MovableObject* movableObject, GameObject::eType type, const Ogre::String& filename = Ogre::String(), bool forceCreation = false, bool forceClampY = false, GameObjectPtr existingGameObjectPtr = nullptr);

		GenericObjectFactory<GameObjectComponent, unsigned int>* getComponentFactory(void);

		GameObjectPtr createGameObject(Ogre::SceneManager* sceneManager, Ogre::SceneNode* sceneNode, Ogre::MovableObject* movableObject, GameObject::eType type, 
			const unsigned long id = NOWA::makeUniqueID());

		// This function can be overridden by a subclass so you can create game-specific C++ components.  If you do
		// this, make sure you call the base-class version first.  If it returns NULL, you know it's not an engine component.
		// Note: propertyElement is a refernce to pointer, since the variable is used in sub functions to propagate through elements
		// and after returning from the subfunctions the current propagated position must be present, in order not to propagate in the
		// main functions twice through the same elements

		/**
		* @brief		Creates a component for this game object, with actual values from XML.
		* @param[in]	propertyElement				The XML property element for the component values
		* @param[in]	filename					The file name where the data is located. Is used maybe for collision hull serialization.
		* @param[in]	gameObjectPtr				The owner game object to set.
		* @param[in]	existingGameObjectCompPtr	If null the component will be created, else the existing one will be used
		* @Note		The file name can be received via: Core::getSingletonPtr()->getSectionPath("Projects")[0], if in the cfg, a world is specified for the section
		*/
		virtual GameObjectCompPtr createComponent(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename, GameObjectPtr gameObjectPtr, GameObjectCompPtr existingGameObjectCompPtr = nullptr);
		
		/**
		 * @brief		Creates a new component for this game object.
		 * @param[in]	gameObjectPtr		The game object the create the component for
		 * @param[in]	componentClassName	The component class name for creation
		 */
		GameObjectCompPtr createComponent(GameObjectPtr gameObjectPtr, const Ogre::String& componentClassName);

	public:
		static GameObjectFactory* getInstance();
	private:
		GameObjectFactory();
		~GameObjectFactory();
	};

}; //Namespace end

#endif
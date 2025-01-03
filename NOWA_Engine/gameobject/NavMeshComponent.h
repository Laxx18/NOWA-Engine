#ifndef NAV_MESH_COMPONENT_H
#define NAV_MESH_COMPONENT_H

#include "NOWAPrecompiled.h"
#include "GameObjectComponent.h"
#include "utilities/XMLConverter.h"
#include "main/Events.h"

namespace NOWA
{
	class EXPORTED NavMeshComponent : public GameObjectComponent
	{
	public:
		typedef boost::shared_ptr<GameObject> GameObjectPtr;
		typedef boost::shared_ptr<NavMeshComponent> NavMeshCompPtr;
	public:

		NavMeshComponent();

		virtual ~NavMeshComponent();

		/**
		* @see		GameObjectComponent::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

		/**
		* @see		GameObjectComponent::postInit
		*/
		virtual bool postInit(void) override;

		/**
		* @see		GameObjectComponent::connect
		*/
		virtual bool connect(void) override;

		/**
		* @see		GameObjectComponent::disconnect
		*/
		virtual bool disconnect(void) override;

		/**
		* @see		GameObjectComponent::getClassName
		*/
		virtual Ogre::String getClassName(void) const override;

		/**
		* @see		GameObjectComponent::getParentClassName
		*/
		virtual Ogre::String getParentClassName(void) const override;

		/**
		* @see		GameObjectComponent::clone
		*/
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		/**
		* @see		GameObjectComponent::getStaticClassId
		*/
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("NavMeshComponent");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "NavMeshComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		* @see	GameObjectComponent::getStaticInfoText
		*/
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: The game object will be marked as obstacle for path finding. "
				"Requirements: This component can be used, if OgreRecast A* path navigation is actived.";
		}

		/**
		* @see		GameObjectComponent::update
		*/
		virtual void update(Ogre::Real dt, bool notSimulating) override;

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		/**
		 * @see		GameObjectComponent::writeXML
		 */
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		/**
		 * @see		GameObjectComponent::setActivated
		 */
		virtual void setActivated(bool activated) override;

		/**
		 * @see		GameObjectComponent::isActivated
		 */
		virtual bool isActivated(void) const override;

		void setNavigationType(const Ogre::String& navigationType);

		Ogre::String getNavigationType(void) const;

		void setWalkable(bool walkable);

		bool getWalkable(void) const;
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrNavigationType(void) { return "Navigation Type"; }
		static const Ogre::String AttrWalkable(void) { return "Walkable"; }
	private:
		void manageNavMesh(void);
	private:
		Variant* activated;
		Variant* navigationType;
		Variant* walkable;
		Ogre::String oldNavigationType;
		Ogre::Real transformUpdateTimer;
		Ogre::Vector3 oldPosition;
		Ogre::Quaternion oldOrientation;
		unsigned char type;
	};

}; //namespace end

#endif
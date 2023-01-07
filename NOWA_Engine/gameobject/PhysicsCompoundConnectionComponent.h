#ifndef PHYSICS_COMPOUND_CONNECTION_COMPONENT_H
#define PHYSICS_COMPOUND_CONNECTION_COMPONENT_H

#include "GameObjectComponent.h"
#include "OgreNewt.h"

#include "main/Events.h"

namespace NOWA
{
	class PhysicsActiveComponent;
	
	class EXPORTED PhysicsCompoundConnectionComponent : public GameObjectComponent
	{
	public:
		typedef boost::shared_ptr<PhysicsCompoundConnectionComponent> PhysicsCompoundConnectionCompPtr;
		friend class GameObjectController;
	public:
		PhysicsCompoundConnectionComponent(void);

		virtual ~PhysicsCompoundConnectionComponent();

		/**
		* @see		GameObjectComponent::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

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
		* @see		GameObjectComponent::onCloned
		*/
		virtual bool onCloned(void) override;

		/**
		* @see		GameObjectComponent::getClassName
		*/
		virtual Ogre::String getClassName(void) const override;

		/**
		* @see		GameObjectComponent::getParentClassName
		*/
		virtual Ogre::String getParentClassName(void) const override;

		/**
		* @see		GameObjectComponent::update
		*/
		virtual void update(Ogre::Real dt, bool notSimulating = false) override
		{

		}

		/**
		* @see		GameObjectComponent::clone
		*/
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("PhysicsCompoundConnectionComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "PhysicsCompoundConnectionComponent";
		}

		/**
		 * @see		GameObjectComponent::canStaticAddComponent
		 */
		static bool canStaticAddComponent(GameObject* gameObject);

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		* @see	GameObjectComponent::getStaticInfoText
		*/
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: This Component is used for physics motion, the more complex collision hull is build of several game objects with physics active components.";
		}

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath);

		bool createCompoundCollision();

		void destroyCompoundCollision(void);

		/**
		* @brief	Gets the unique id of this joint handler instance
		* @return	id		The id of this joint handler instance
		*/
		unsigned long getId(void) const;

		void setRootId(unsigned long rootId);

		unsigned long getRootId(void) const;

		unsigned long getPriorId(void) const;

		void addPhysicsActiveComponent(PhysicsActiveComponent* physicsActiveComponent);
	public:
		static const Ogre::String AttrRootId(void) { return "Root Id"; }
	protected:
		void internalSetPriorId(unsigned long priorId);
		void handleRootComponentDeleted(NOWA::EventDataPtr eventData);
	protected:
		Variant* rootId;
		unsigned long priorId;
		std::vector<PhysicsActiveComponent*> physicsActiveComponentList;
	};

}; // namespace end

#endif
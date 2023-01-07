#ifndef EXIT_COMPONENT_H
#define EXIT_COMPONENT_H

#include "GameObjectComponent.h"
#include "main/Events.h"

namespace NOWA
{
	class EXPORTED ExitComponent : public GameObjectComponent
	{
	public:
		typedef boost::shared_ptr<GameObject> GameObjectPtr;
		typedef boost::shared_ptr<ExitComponent> ExitCompPtr;
	public:

		ExitComponent();
		~ExitComponent();

		/**
		 * @see		GameObjectComponent::init
		 */
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename) override;

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
		 * @see		GameObjectComponent::clone
		 */
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("ExitComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "ExitComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		/**
		 * @see		GameObjectComponent::actualizeValue
		 */
		virtual void actualizeValue(Variant* attribute) override;

		/**
		 * @see		GameObjectComponent::writeXML
		 */
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;
		
		/**
		 * @see		GameObjectComponent::setActivated
		 */
		virtual void setActivated(bool activated) override;
		
		virtual bool isActivated(void) const override;
		
		/**
		* @see	GameObjectComponent::getStaticInfoText
		*/
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: Adds possibility to load a specified new world, when the source GameObject reaches the GameObject with the ExitComponent. "
				   "Note: Each GameObject may have only one ExitComponent. When this GameObject does have a PhysicsTriggerComponent, that component is used to trigger the new world when the source GameObject entered the trigger.";
		}

		Ogre::String getTargetWorldName(void) const;

		Ogre::String getCurrentWorldName(void) const;

		Ogre::Vector2 getExitDirection(void) const;

		Ogre::String getAxis(void) const;
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrTargetWorldName(void) { return "Target World Name"; }
		static const Ogre::String AttrTargetLocationName(void) { return "Target Location Name"; }
		static const Ogre::String AttrSourceId(void) { return "Source Id"; }
		static const Ogre::String AttrExitDirection(void) { return "Exit Direction"; }
		static const Ogre::String AttrAxis(void) { return "Axis"; }
	private:
		void handlePhysicsTrigger(NOWA::EventDataPtr eventData);
	private:
		Variant* activated;
		Variant* targetWorldName;
		Variant* targetLocationName;
		Variant* sourceGameObjectId;
		Variant* exitDirection;
		Variant* axis;
		GameObject* sourceGameObject;
		bool processAlreadyAttached;
	};

}; //namespace end

#endif
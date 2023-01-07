#ifndef EXIT_COMPONENT_H
#define EXIT_COMPONENT_H

#include "GameObjectComponent.h"

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

		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

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
		
		bool isActivated(void) const;
		
		Ogre::String getTargetAppStateName(void) const;

		Ogre::String getTargetWorldName(void) const;

		Ogre::String getCurrentAppStateName(void) const;
		
		Ogre::String getCurrentWorldName(void) const;

		Ogre::Vector3 getExitDirection(void) const;
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrTargetAppStateName(void) { return "Target App State Name"; }
		static const Ogre::String AttrTargetWorldName(void) { return "Target World Name"; }
		static const Ogre::String AttrTargetLocationName(void) { return "Target Location Name"; }
		static const Ogre::String AttrSourceId(void) { return "Source Id"; }
		static const Ogre::String AttrExitDirection(void) { return "Exit Direction"; }
	private:
		Variant* activated;
		Variant* targetAppStateName;
		Variant* targetWorldName;
		Variant* targetLocationName;
		Variant* sourceGameObjectId;
		Variant* exitDirection;
		GameObject* sourceGameObject;
		bool processAlreadyAttached;
	};

}; //namespace end

#endif
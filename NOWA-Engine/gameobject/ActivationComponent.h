#ifndef ACTIVATION_COMPONENT_H
#define ACTIVATION_COMPONENT_H

#include "GameObjectComponent.h"

namespace NOWA
{
	class EXPORTED ActivationComponent : public GameObjectComponent
	{
	public:
		typedef boost::shared_ptr<ActivationComponent> ActivationCompPtr;
	public:
		ActivationComponent();

		virtual ~ActivationComponent();

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
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		/**
		* @see		GameObjectComponent::setActivated
		*/
		virtual void setActivated(bool activated) override;
	public:
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("ActivationComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "ActivationComponent";
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
			return "Usage: Is used by AreaOfInterestComponent in order to automatically activate this component, when it comes into the area of the target.";
		}
	};

}; //namespace end

#endif
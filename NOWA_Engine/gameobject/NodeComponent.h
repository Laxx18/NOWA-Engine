#ifndef NODE_COMPONENT_H
#define NODE_COMPONENT_H

#include "GameObjectComponent.h"

namespace NOWA
{
	class EXPORTED NodeComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<NOWA::NodeComponent> NodeCompPtr;
	public:

		NodeComponent();

		virtual ~NodeComponent();

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
			return NOWA::getIdFromName("NodeComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "NodeComponent";
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
			return "Usage: This Component is used as waypoint for several cases, e.g. AI-Pathfollow, Camera tracking etc.";
		}

		virtual void update(Ogre::Real dt, bool notSimulating) override;

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		void setShow(bool show);

		bool getShow(void) const;
	public:
		static const Ogre::String AttrShow(void) { return "Show"; }

	private:
		Ogre::v1::Entity* dummyEntity;

		Variant* show;
	};

}; //namespace end

#endif
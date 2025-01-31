/*
Copyright (c) 2024 Lukas Kalinowski

GPL v3
*/

#ifndef KEYHOLEEFFECTCOMPONENT_H
#define KEYHOLEEFFECTCOMPONENT_H

#include "gameobject/CompositorEffectComponents.h"
#include "main/Events.h"
#include "OgrePlugin.h"

namespace NOWA
{

	/**
	  * @brief		Creates a keyhole effect which is applied to the screen. Also another custom shape can be used.
	  */
	class EXPORTED KeyholeEffectComponent : public CompositorEffectBaseComponent, public Ogre::Plugin
	{
	public:
		typedef boost::shared_ptr<KeyholeEffectComponent> KeyholeEffectComponentPtr;
	public:

		KeyholeEffectComponent();

		virtual ~KeyholeEffectComponent();

		/**
		* @see		Ogre::Plugin::install
		*/
		virtual void install(const Ogre::NameValuePairList* options) override;

		/**
		* @see		Ogre::Plugin::initialise
		* @note		Do nothing here, because its called far to early and nothing is there of NOWA-Engine yet!
		*/
		virtual void initialise() override {};

		/**
		* @see		Ogre::Plugin::shutdown
		* @note		Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
		*/
		virtual void shutdown() override {};

		/**
		* @see		Ogre::Plugin::uninstall
		* @note		Do nothing here, because its called far to late and nothing is there of NOWA-Engine anymore! Use @onRemoveComponent in order to destroy something.
		*/
		virtual void uninstall() override {};

		/**
		* @see		Ogre::Plugin::getName
		*/
		virtual const Ogre::String& getName() const override;
		
		/**
		* @see		Ogre::Plugin::getAbiCookie
		*/
		virtual void getAbiCookie(Ogre::AbiCookie& outAbiCookie) override;

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
		* @see		GameObjectComponent::onCloned
		*/
		virtual bool onCloned(void) override;

		/**
		* @see		GameObjectComponent::onRemoveComponent
		*/
		virtual void onRemoveComponent(void);
		
		/**
		 * @see		GameObjectComponent::onOtherComponentRemoved
		 */
		virtual void onOtherComponentRemoved(unsigned int index) override;

		/**
		 * @see		GameObjectComponent::onOtherComponentAdded
		 */
		virtual void onOtherComponentAdded(unsigned int index) override;

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
		* @see		GameObjectComponent::update
		*/
		virtual void update(Ogre::Real dt, bool notSimulating = false) override;

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		void setRadius(Ogre::Real radius); 

		Ogre::Real getRadius(void) const;

		void setCenter(const Ogre::Vector2& center); 

		Ogre::Vector2 getCenter(void) const;

		void setGrow(bool grow); 

		bool getGrow(void) const;

		void setShapeMask(const Ogre::String& shapeMask); 

		Ogre::String getShapeMask(void) const;

		void setSpeed(Ogre::Real speed); 

		Ogre::Real getSpeed(void) const;

	public:
		/**
		* @see		GameObjectComponent::getStaticClassId
		*/
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("KeyholeEffectComponent");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "KeyholeEffectComponent";
		}
	
		/**
		* @see		GameObjectComponent::canStaticAddComponent
		*/
		static bool canStaticAddComponent(GameObject* gameObject);

		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: Creates a keyhole effect which is applied to the screen. Also another custom shape can be used.";
		}
		
		/**
		 * @see	GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);
	public:
		static const Ogre::String AttrRadius(void) { return "Radius"; }
		static const Ogre::String AttrCenter(void) { return "Center"; }
		static const Ogre::String AttrGrow(void) { return "Grow"; }
		static const Ogre::String AttrShapeMask(void) { return "ShapeMask"; }
		static const Ogre::String AttrSpeed(void) { return "Speed"; }
	private:
		void createKeyholeCompositorEffect(void);
		void destroyShape(void);
	private:
		Ogre::String name;
		Ogre::TextureGpu* shapeTexture;
		Ogre::StagingTexture* shapeStagingTexture;
		Ogre::MaterialPtr material;
		Ogre::Pass* pass;
		Ogre::Real tempRadius;
		bool tempGrow;

		Variant* radius;
		Variant* center;
		Variant* grow;
		Variant* shapeMask;
		Variant* speed;
	};

}; // namespace end

#endif


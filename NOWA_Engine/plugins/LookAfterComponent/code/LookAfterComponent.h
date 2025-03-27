/*
Copyright (c) 2025 Lukas Kalinowski

GPL v3
*/
#ifndef LOOK_AFTER_COMPONENTS_H
#define LOOK_AFTER_COMPONENTS_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"

namespace NOWA
{
	/**
	 * @brief	Usage: The game objects bone head will look after a target. Requirements: An AnimationComponent.
	 */
	class EXPORTED LookAfterComponent : public GameObjectComponent, public Ogre::Plugin
	{
	public:

		typedef boost::shared_ptr<LookAfterComponent> LookAfterCompPtr;
	public:

		LookAfterComponent();

		virtual ~LookAfterComponent();

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
		* @see		GameObjectComponent::onRemoveComponent
		*/
		virtual void onRemoveComponent(void);
		
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
		* @see		GameObjectComponent::update
		*/
		virtual void update(Ogre::Real dt, bool notSimulating = false) override;
		
		/**
		* @see		GameObjectComponent::clone
		*/
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		/**
		* @see		GameObjectComponent::getClassName
		*/
		virtual Ogre::String getClassName(void) const override;

		/**
		* @see		GameObjectComponent::getParentClassName
		*/
		virtual Ogre::String getParentClassName(void) const override;

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

		virtual bool isActivated(void) const override;

	public:
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("LookAfterComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "LookAfterComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController);

		/**
		* @see	GameObjectComponent::getStaticInfoText
		*/
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: The game objects bone head will look after a target. "
				"Requirements: An AnimationComponent.";
		}

		/**
		* @see		GameObjectComponent::canStaticAddComponent
		*/
		static bool canStaticAddComponent(GameObject* gameObject);
	public:

		void setHeadBoneName(const Ogre::String& headBoneName);

		Ogre::String getHeadBoneName(void) const;

		void setTargetId(unsigned long targetId);

		unsigned long getTargetId(void) const;

		void setLookSpeed(Ogre::Real lookSpeed);

		Ogre::Real getLookSpeed(void) const;
		
		void setMaxPitch(Ogre::Real maxPitch);

		Ogre::Real getMaxPitch(void) const;
		
		void setMaxYaw(Ogre::Real maxYaw);

		Ogre::Real getMaxYaw(void) const;
		
		Ogre::SceneNode* getTargetSceneNode(void) const;
		
		Ogre::v1::OldBone* getHeadBone(void) const;
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrDefaultOrientation(void) { return "Default Orientation"; }
		static const Ogre::String AttrHeadBoneName(void) { return "Head Bone Name"; }
		static const Ogre::String AttrTargetId(void) { return "Target Id"; }
		static const Ogre::String AttrLookSpeed(void) { return "Look Speed"; }
		static const Ogre::String AttrMaxPitch(void) { return "Max. Pitch"; }
		static const Ogre::String AttrMaxYaw(void) { return "Max. Yaw"; }
	protected:
		Ogre::String name;
		Variant* activated;
		Variant* defaultOrientation;
		Variant* headBoneName;
		Variant* targetId;
		Variant* lookSpeed; 
		Variant* maxPitch;
		Variant* maxYaw;
		Ogre::SceneNode* targetSceneNode;
		Ogre::v1::OldBone* headBone;
	};

}; //namespace end

#endif
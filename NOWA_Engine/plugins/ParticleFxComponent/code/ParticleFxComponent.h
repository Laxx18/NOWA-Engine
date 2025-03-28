/*
Copyright (c) 2025 Lukas Kalinowski

GPL v3
*/

#ifndef PARTICLEFXCOMPONENT_H
#define PARTICLEFXCOMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"
#include "OgreParticleSystem.h"

namespace NOWA
{

	/**
	  * @brief		Creates an Ogre FX particle effect.
	  */
	class EXPORTED ParticleFxComponent : public GameObjectComponent, public Ogre::Plugin
	{
	public:
		typedef boost::shared_ptr<ParticleFxComponent> ParticleFxComponentPtr;
	public:

		ParticleFxComponent();

		virtual ~ParticleFxComponent();

		/**
		* @see		Ogre::Plugin::install
		*/
		virtual void install(const Ogre::NameValuePairList* options) override;

		/**
		* @see		Ogre::Plugin::initialise
		*/
		virtual void initialise() override;

		/**
		* @see		Ogre::Plugin::shutdown
		*/
		virtual void shutdown() override;

		/**
		* @see		Ogre::Plugin::uninstall
		*/
		virtual void uninstall() override;

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

		/**
		* @see		GameObjectComponent::setActivated
		*/
		virtual void setActivated(bool activated) override;

		/**
		* @see		GameObjectComponent::isActivated
		*/
		virtual bool isActivated(void) const override;

		void setParticleTemplateName(const Ogre::String& particleTemplateName);

		Ogre::String getParticleTemplateName(void) const;

		void setRepeat(bool repeat);

		bool getRepeat(void) const;

		void setParticlePlayTimeMS(Ogre::Real playTime);

		Ogre::Real getParticlePlayTimeMS(void) const;

		void setParticlePlaySpeed(Ogre::Real playSpeed);

		Ogre::Real getParticlePlaySpeed(void) const;

		void setParticleOffsetPosition(const Ogre::Vector3& particleOffsetPosition);

		Ogre::Vector3 getParticleOffsetPosition(void);

		void setParticleOffsetOrientation(const Ogre::Vector3& particleOffsetOrientation);

		Ogre::Vector3 getParticleOffsetOrientation(void);

		void setParticleScale(const Ogre::Vector3& particleScale);

		Ogre::Vector3 getParticleScale(void);

		ParticleUniverse::ParticleSystem* getParticle(void) const;

	public:
		/**
		* @see		GameObjectComponent::getStaticClassId
		*/
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("ParticleFxComponent");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "ParticleFxComponent";
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
			return "Usage: My usage text.";
		}
		
		/**
		 * @see	GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);
	public:
		static const Ogre::String AttrActivated(void) { return "Activated"; }
		static const Ogre::String AttrParticleName(void) { return "Particle Name"; }
		static const Ogre::String AttrRepeat(void) { return "Repeat"; }
		static const Ogre::String AttrPlayTime(void) { return "Play Time"; }
		static const Ogre::String AttrPlaySpeed(void) { return "Play Speed"; }
		static const Ogre::String AttrOffsetPosition(void) { return "Offset Position"; }
		static const Ogre::String AttrOffsetOrientation(void) { return "Offset Orientation"; }
		static const Ogre::String AttrScale(void) { return "Scale"; }
	private:
		bool createParticleEffect(void);
		void destroyParticleEffect(void);
	private:
		Ogre::String name;

		Ogre::ParticleSystem* particleSystem;
		Ogre::SceneNode* particleNode;
		Variant* activated;
		Variant* particleTemplateName;
		Variant* repeat;
		Ogre::Real particlePlayTime;
		Variant* particleInitialPlayTime;
		Variant* particlePlaySpeed;
		Variant* particleOffsetPosition;
		Variant* particleOffsetOrientation;
		Variant* particleScale;
		Ogre::String oldParticleTemplateName;
		bool oldActivated;

		// fastForward ParticleSystem attribute
		// setNonVisibleUpdateTimeout
		// setKeepParticlesInLocalSpace
	};

}; // namespace end

#endif


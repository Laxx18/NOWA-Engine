/*
Copyright (c) 2022 Lukas Kalinowski

The Massachusetts Institute of Technology ("MIT"), a non-profit institution of higher education, agrees to make the downloadable software and documentation, if any, 
(collectively, the "Software") available to you without charge for demonstrational and non-commercial purposes, subject to the following terms and conditions.

Restrictions: You may modify or create derivative works based on the Software as long as the modified code is also made accessible and for non-commercial usage. 
You may copy the Software. But you may not sublicense, rent, lease, or otherwise transfer rights to the Software. You may not remove any proprietary notices or labels on the Software.

No Other Rights. MIT claims and reserves title to the Software and all rights and benefits afforded under any available United States and international 
laws protecting copyright and other intellectual property rights in the Software.

Disclaimer of Warranty. You accept the Software on an "AS IS" basis.

MIT MAKES NO REPRESENTATIONS OR WARRANTIES CONCERNING THE SOFTWARE, 
AND EXPRESSLY DISCLAIMS ALL SUCH WARRANTIES, INCLUDING WITHOUT 
LIMITATION ANY EXPRESS OR IMPLIED WARRANTY OF MERCHANTABILITY, FITNESS 
FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT OF INTELLECTUAL PROPERTY RIGHTS. 

MIT has no obligation to assist in your installation or use of the Software or to provide services or maintenance of any type with respect to the Software.
The entire risk as to the quality and performance of the Software is borne by you. You acknowledge that the Software may contain errors or bugs. 
You must determine whether the Software sufficiently meets your requirements. This disclaimer of warranty constitutes an essential part of this Agreement.
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


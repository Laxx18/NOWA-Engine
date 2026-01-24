/*
Copyright (c) 2025 Lukas Kalinowski

GPL v3
*/

#ifndef PARTICLEFXCOMPONENT_H
#define PARTICLEFXCOMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"

// Ogre-next ParticleFX2 includes
#include "ParticleSystem/OgreParticleSystem2.h"

namespace NOWA
{

	/**
	  * @brief		Creates an Ogre-next ParticleFX2 particle effect.
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

		/**
		 * @brief Sets the particle template name (material/datablock name starting with "Particle/")
		 * @param particleTemplateName The name of the particle template
		 */
		void setParticleTemplateName(const Ogre::String& particleTemplateName);

		/**
		 * @brief Gets the particle template name
		 * @return The particle template name
		 */
		Ogre::String getParticleTemplateName(void) const;

		/**
		 * @brief Sets whether the particle effect should repeat
		 * @param repeat True to repeat, false otherwise
		 */
		void setRepeat(bool repeat);

		/**
		 * @brief Gets whether the particle effect repeats
		 * @return True if repeating, false otherwise
		 */
		bool getRepeat(void) const;

		/**
		 * @brief Sets the particle play time in milliseconds
		 * @param playTime The play time in milliseconds (0 = infinite)
		 */
		void setParticlePlayTimeMS(Ogre::Real playTime);

		/**
		 * @brief Gets the particle play time in milliseconds
		 * @return The play time in milliseconds
		 */
		Ogre::Real getParticlePlayTimeMS(void) const;

		/**
		 * @brief Sets the particle play speed multiplier
		 * @param playSpeed The speed multiplier (1.0 = normal)
		 */
		void setParticlePlaySpeed(Ogre::Real playSpeed);

		/**
		 * @brief Gets the particle play speed multiplier
		 * @return The speed multiplier
		 */
		Ogre::Real getParticlePlaySpeed(void) const;

		/**
		 * @brief Sets the offset position relative to the game object
		 * @param particleOffsetPosition The offset position
		 */
		void setParticleOffsetPosition(const Ogre::Vector3& particleOffsetPosition);

		/**
		 * @brief Gets the offset position
		 * @return The offset position
		 */
		Ogre::Vector3 getParticleOffsetPosition(void) const;

		/**
		 * @brief Sets the offset orientation (in degrees) relative to the game object
		 * @param particleOffsetOrientation The offset orientation in degrees (pitch, yaw, roll)
		 */
		void setParticleOffsetOrientation(const Ogre::Vector3& particleOffsetOrientation);

		/**
		 * @brief Gets the offset orientation in degrees
		 * @return The offset orientation in degrees
		 */
		Ogre::Vector3 getParticleOffsetOrientation(void) const;

		/**
		 * @brief Sets the particle scale
		 * @param particleScale The scale vector
		 */
		void setParticleScale(const Ogre::Vector3& particleScale);

		/**
		 * @brief Gets the particle scale
		 * @return The scale vector
		 */
		Ogre::Vector3 getParticleScale(void) const;

		/**
		 * @brief Gets the Ogre ParticleSystem2 pointer
		 * @return Pointer to the particle system, or nullptr if not created
		 */
		Ogre::ParticleSystem2* getParticleSystem(void) const;

		/**
		 * @brief Checks if the particle effect is currently playing
		 * @return True if playing, false otherwise
		 */
		bool isPlaying(void) const;

		/**
		 * @brief Sets the global position for the particle effect
		 * @param particlePosition The world position
		 */
		void setGlobalPosition(const Ogre::Vector3& particlePosition);

		/**
		 * @brief Sets the global orientation for the particle effect
		 * @param particleOrientation The orientation in degrees
		 */
		void setGlobalOrientation(const Ogre::Vector3& particleOrientation);

		/**
		 * @brief Gets the particle system definition (template)
		 * @return Pointer to the particle system definition, or nullptr if not available
		 */
		const Ogre::ParticleSystemDef* getParticleSystemDef(void) const;

		/**
		 * @brief Gets the current number of active particles
		 * @return Number of active particles
		 */
		size_t getNumActiveParticles(void) const;

		/**
		 * @brief Gets the particle quota (maximum particles allowed)
		 * @return The particle quota
		 */
		Ogre::uint32 getParticleQuota(void) const;

		/**
		 * @brief Gets the number of emitters in the particle system
		 * @return Number of emitters
		 */
		size_t getNumEmitters(void) const;

		/**
		 * @brief Gets the number of affectors in the particle system
		 * @return Number of affectors
		 */
		size_t getNumAffectors(void) const;

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
			return "Usage: This Component is for playing particle effects using the Ogre-next ParticleFX2 system. "
				"Select a particle template from the list (materials starting with 'Particle/'). "
				"Set play time to 0 for infinite duration.";
		}

		/**
		 * @see	GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

	public:
		static const Ogre::String AttrActivated(void)
		{
			return "Activated";
		}
		static const Ogre::String AttrParticleName(void)
		{
			return "Particle Name";
		}
		static const Ogre::String AttrRepeat(void)
		{
			return "Repeat";
		}
		static const Ogre::String AttrPlayTime(void)
		{
			return "Play Time";
		}
		static const Ogre::String AttrPlaySpeed(void)
		{
			return "Play Speed";
		}
		static const Ogre::String AttrOffsetPosition(void)
		{
			return "Offset Position";
		}
		static const Ogre::String AttrOffsetOrientation(void)
		{
			return "Offset Orientation";
		}
		static const Ogre::String AttrScale(void)
		{
			return "Scale";
		}

	private:
		/**
		 * @brief Creates the particle effect based on the current settings
		 * @return True if successful, false otherwise
		 */
		bool createParticleEffect(void);

		/**
		 * @brief Destroys the current particle effect
		 */
		void destroyParticleEffect(void);

		/**
		 * @brief Gathers all available particle template names from HLMS datablocks
		 */
		void gatherParticleNames(void);

		/**
		 * @brief Internal helper to start the particle effect emission
		 */
		void startParticleEffect(void);

		/**
		 * @brief Internal helper to stop the particle effect emission
		 */
		void stopParticleEffect(void);

	private:
		Ogre::String name;

		Ogre::ParticleSystem2* particleSystem;
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
		bool isEmitting;  // Track emission state
	};

}; // namespace end

#endif
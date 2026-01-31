/*
Copyright (c) 2025 Lukas Kalinowski

GPL v3
*/

#ifndef PARTICLEFXCOMPONENT_H
#define PARTICLEFXCOMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"
#include "modules/ParticleFxModule.h"

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
		 * @brief Sets the particle play speed multiplier (affects both play time and particle flow velocity)
		 * @param playSpeed The speed multiplier (1.0 = normal, >1.0 = faster, <1.0 = slower)
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
		void setParticleScale(const Ogre::Vector2& particleScale);

		/**
		 * @brief Gets the particle scale
		 * @return The scale vector
		 */
		Ogre::Vector2 getParticleScale(void) const;

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
		 * @brief Gets the particle quota
		 * @return The particle quota
		 */
		Ogre::uint32 getParticleQuota(void) const;

		/**
		 * @brief Gets the number of emitters
		 * @return Number of emitters
		 */
		size_t getNumEmitters(void) const;

		/**
		 * @brief Gets the number of affectors
		 * @return Number of affectors
		 */
		size_t getNumAffectors(void) const;

		/**
		 * @brief Sets the blending method for particle rendering
		 * @param blendingMethod The blending method to use
		 * WHEN TO USE EACH MODE:

		1. AlphaHashing (0)
		   - Use for: Grass, leaves, fences, cutout effects
		   - Texture: Any texture, creates stipple pattern
		   - Depth write: ON
		   - Performance: Best
		   - Quality: Dithered/stippled look

		2. AlphaHashingA2C (1)
		   - Use for: Same as AlphaHashing but better quality
		   - Texture: Any texture
		   - Depth write: ON
		   - Performance: Good (requires MSAA)
		   - Quality: Smoother edges with MSAA

		3. AlphaBlending (2) - SBT_ADD
		   - Use for: Rain, fire, sparks, stars, explosions, glows
		   - Texture: White/bright on BLACK background (no alpha channel needed)
		   - Depth write: OFF
		   - Performance: Good
		   - Quality: Smooth, glowing effect
		   - Black (0,0,0) becomes transparent

		4. AlphaTransparent (3) - SBT_TRANSPARENT_ALPHA
		   - Use for: Smoke, clouds, semi-transparent sprites
		   - Texture: MUST have proper alpha channel
		   - Depth write: OFF
		   - Performance: Good
		   - Quality: True smooth transparency
		   - Uses texture's alpha channel for transparency
		*/
		void setBlendingMethod(ParticleBlendingMethod::ParticleBlendingMethod blendingMethod);

		/**
		 * @brief Gets the current blending method
		 * @return The current blending method
		 */
		ParticleBlendingMethod::ParticleBlendingMethod getBlendingMethod(void) const;

		/**
		 * @brief Sets whether the particle effect should fade in when starting
		 * @param fadeIn True to enable fade in, false otherwise
		 */
		void setFadeIn(bool fadeIn);

		/**
		 * @brief Gets whether the particle effect fades in when starting
		 * @return True if fade in is enabled, false otherwise
		 */
		bool getFadeIn(void) const;

		/**
		 * @brief Sets the fade in duration in milliseconds
		 * @param fadeInTimeMS The fade in duration in milliseconds
		 */
		void setFadeInTimeMS(Ogre::Real fadeInTimeMS);

		/**
		 * @brief Gets the fade in duration in milliseconds
		 * @return The fade in duration in milliseconds
		 */
		Ogre::Real getFadeInTimeMS(void) const;

		/**
		 * @brief Sets whether the particle effect should fade out when stopping
		 * @param fadeOut True to enable fade out, false otherwise
		 */
		void setFadeOut(bool fadeOut);

		/**
		 * @brief Gets whether the particle effect fades out when stopping
		 * @return True if fade out is enabled, false otherwise
		 */
		bool getFadeOut(void) const;

		/**
		 * @brief Sets the fade out duration in milliseconds
		 * @param fadeOutTimeMS The fade out duration in milliseconds
		 */
		void setFadeOutTimeMS(Ogre::Real fadeOutTimeMS);

		/**
		 * @brief Gets the fade out duration in milliseconds
		 * @return The fade out duration in milliseconds
		 */
		Ogre::Real getFadeOutTimeMS(void) const;

		/**
		 * @brief Gets the current fade state
		 * @return The current fade state (None, FadingIn, FadingOut)
		 */
		ParticleFadeState::ParticleFadeState getFadeState(void) const;

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
				"Set play time to 0 for infinite duration. "
				"Enable FadeIn/FadeOut for smooth particle transitions. "
				"Use PlaySpeed to control how fast particles flow."
				"\n\n"
				"WHEN TO USE EACH MODE:\n"
				"\n"
				"1. AlphaHashing (0)\n"
				"   - Use for: Grass, leaves, fences, cutout effects\n"
				"   - Texture: Any texture, creates stipple pattern\n"
				"   - Depth write: ON\n"
				"   - Performance: Best\n"
				"   - Quality: Dithered/stippled look\n"
				"\n"
				"2. AlphaHashingA2C (1)\n"
				"   - Use for: Same as AlphaHashing but better quality\n"
				"   - Texture: Any texture\n"
				"   - Depth write: ON\n"
				"   - Performance: Good (requires MSAA)\n"
				"   - Quality: Smoother edges with MSAA\n"
				"\n"
				"3. AlphaBlending (2) - SBT_ADD\n"
				"   - Use for: Rain, fire, sparks, stars, explosions, glows\n"
				"   - Texture: White/bright on BLACK background (no alpha channel needed)\n"
				"   - Depth write: OFF\n"
				"   - Performance: Good\n"
				"   - Quality: Smooth, glowing effect\n"
				"   - Black (0,0,0) becomes transparent\n"
				"\n"
				"4. AlphaTransparent (3) - SBT_TRANSPARENT_ALPHA\n"
				"   - Use for: Smoke, clouds, semi-transparent sprites\n"
				"   - Texture: MUST have proper alpha channel\n"
				"   - Depth write: OFF\n"
				"   - Performance: Good\n"
				"   - Quality: True smooth transparency\n"
				"   - Uses texture's alpha channel for transparency";
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
		static const Ogre::String AttrBlendingMethod(void)
		{
			return "Blending Method";
		}
		static const Ogre::String AttrFadeIn(void)
		{
			return "Fade In";
		}
		static const Ogre::String AttrFadeInTime(void)
		{
			return "Fade In Time";
		}
		static const Ogre::String AttrFadeOut(void)
		{
			return "Fade Out";
		}
		static const Ogre::String AttrFadeOutTime(void)
		{
			return "Fade Out Time";
		}

	private:

		/**
		 * @brief Gets the unique particle name for this component instance
		 * @return The unique particle name
		 */
		Ogre::String getUniqueParticleName(void) const;

		/**
		 * @brief Updates the particle node's transform to follow the GameObject
		 */
		void updateParticleTransform(void);

		/**
		 * @brief Recreates the particle with current settings
		 */
		void recreateParticle(void);

	private:
		Ogre::String name;
		Ogre::String uniqueParticleName;  // Unique name for this component's particle in the module
		ParticleFxModule* particleFxModule;

		Variant* activated;
		Variant* particleTemplateName;
		Variant* repeat;
		Variant* particleInitialPlayTime;
		Variant* particlePlaySpeed;
		Variant* particleOffsetPosition;
		Variant* particleOffsetOrientation;
		Variant* particleScale;
		Variant* blendingMethod;
		Variant* fadeIn;
		Variant* fadeInTime;
		Variant* fadeOut;
		Variant* fadeOutTime;

		bool oldActivated;
	};

}; // namespace end

#endif

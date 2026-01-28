/*
Copyright (c) 2025 Lukas Kalinowski

GPL v3
*/

#ifndef PARTICLE_FX_MODULE_H_
#define PARTICLE_FX_MODULE_H_

#include "defines.h"

// Ogre-next ParticleFX2 includes
#include "ParticleSystem/OgreParticleSystem2.h"
#include "ParticleSystem/OgreEmitter2.h"

namespace NOWA
{
	/**
	 * @brief Blending methods for particle rendering
	 */
	namespace ParticleBlendingMethod
	{
		enum ParticleBlendingMethod
		{
			AlphaHashing = 0,      ///< Alpha hashing without A2C
			AlphaHashingA2C = 1,   ///< Alpha hashing with Alpha-to-Coverage (best quality with MSAA)
			AlphaBlending = 2,      ///< Traditional alpha blending
			AlphaTransparent = 3,
			TransparentColour = 4,
			FromMaterial = 5          ///< Determine blending from material comments
		};
	}

	/**
	 * @brief Fade state for particle effect transitions
	 */
	namespace ParticleFadeState
	{
		enum ParticleFadeState
		{
			None = 0,        ///< No fading active
			FadingIn = 1,    ///< Currently fading in (increasing emission rate)
			FadingOut = 2    ///< Currently fading out (decreasing emission rate)
		};
	}

	/**
	 * @brief Data structure holding particle effect state and properties
	 */
	struct EXPORTED ParticleFxData
	{
		ParticleFxData()
			: particleSystem(nullptr),
			particleNode(nullptr),
			particlePlayTime(0.0f),
			particleInitialPlayTime(0.0f),
			particlePlaySpeed(1.0f),
			particleOffsetPosition(Ogre::Vector3::ZERO),
			particleOffsetOrientation(Ogre::Quaternion::IDENTITY),
			particleScale(Ogre::Vector2::UNIT_SCALE),
			blendingMethod(ParticleBlendingMethod::AlphaBlending),
			fadeIn(false),
			fadeInTimeMS(1000.0f),
			fadeOut(false),
			fadeOutTimeMS(1000.0f),
			repeat(false),
			activated(false),
			isEmitting(false),
			fadeState(ParticleFadeState::None),
			fadeProgress(0.0f),
			pendingDrainTimeMs(0.0f),
			pendingRestartAfterDrain(false),
			blendingModeInitialized(false)
		{
		}

		Ogre::ParticleSystem2* particleSystem;
		Ogre::SceneNode* particleNode;
		Ogre::String particleTemplateName;
		Ogre::String baseParticleTemplateName;
		Ogre::String clonedDefName;
		Ogre::Real particlePlayTime;
		Ogre::Real particleInitialPlayTime;
		Ogre::Real particlePlaySpeed;
		Ogre::Vector3 particleOffsetPosition;
		Ogre::Quaternion particleOffsetOrientation;
		Ogre::Vector2 particleScale;
		ParticleBlendingMethod::ParticleBlendingMethod blendingMethod;
		bool fadeIn;
		Ogre::Real fadeInTimeMS;
		bool fadeOut;
		Ogre::Real fadeOutTimeMS;
		bool repeat;
		bool activated;
		bool isEmitting;
		ParticleFadeState::ParticleFadeState fadeState;
		Ogre::Real fadeProgress;
		std::vector<Ogre::Real> originalEmissionRates;
		std::vector<Ogre::Real> originalMinVelocities;
		std::vector<Ogre::Real> originalMaxVelocities;
		Ogre::Real pendingDrainTimeMs;
		bool pendingRestartAfterDrain;
		bool blendingModeInitialized;
	};

	/**
	 * @brief Module for managing Ogre-next ParticleFX2 particle effects
	 * 
	 * This module provides a centralized API for creating and managing particle effects
	 * without requiring a GameObject component. It supports:
	 * - Creating particle effects from templates
	 * - Playing, stopping, pausing, and resuming effects
	 * - Fade in/out transitions
	 * - Repeat/looping
	 * - Play speed control
	 * - Scale and transformation control
	 * - Multiple blending methods (Alpha Hashing, Alpha Hashing + A2C, Alpha Blending)
	 */
	class EXPORTED ParticleFxModule
	{
	private:
		struct TextureAtlasInfo
		{
			bool hasAtlas;
			int rows;
			int columns;
			Ogre::Real fps;
			Ogre::Real currentTime;
			int currentFrame;

			TextureAtlasInfo()
				: hasAtlas(false),
				rows(1),
				columns(1),
				fps(16.0f),
				currentTime(0.0f),
				currentFrame(0)
			{

			}
			TextureAtlasInfo(int r, int c, Ogre::Real f)
				: hasAtlas(true),
				rows(r),
				columns(c),
				fps(f),
				currentTime(0.0f),
				currentFrame(0)
			{
			}
		};

	public:
		friend class AppState; // Only AppState may create this class

		/**
		 * @brief Initialize the module with a scene manager
		 * @param sceneManager The Ogre scene manager to use for particle systems
		 */
		void init(Ogre::SceneManager* sceneManager);

		/**
		 * @brief Create a new particle effect
		 * @param name Unique name for this particle effect instance
		 * @param templateName The particle template name (material/datablock starting with "Particle/")
		 * @param playTimeMS The play time in milliseconds (0 = infinite)
		 * @param orientation The initial orientation
		 * @param position The initial position
		 * @param scale The particle scale (default = unit scale)
		 * @param repeat Whether the effect should loop (default = false)
		 * @param playSpeed The playback speed multiplier (default = 1.0)
		 * @param blendingMethod The blending method to use (default = AlphaBlending)
		 * @param fadeIn Whether to fade in when starting (default = false)
		 * @param fadeInTimeMS The fade in duration in milliseconds (default = 1000)
		 * @param fadeOut Whether to fade out when stopping (default = false)
		 * @param fadeOutTimeMS The fade out duration in milliseconds (default = 1000)
		 * @return particleBlendingMethod the chosen blending method.
		 * 
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
		ParticleBlendingMethod::ParticleBlendingMethod createParticleSystem(const Ogre::String& name,
			const Ogre::String& templateName,
			Ogre::Real playTimeMS,
			const Ogre::Quaternion& orientation = Ogre::Quaternion::IDENTITY,
			const Ogre::Vector3& position = Ogre::Vector3::ZERO,
			const Ogre::Vector2& scale = Ogre::Vector2::UNIT_SCALE,
			bool repeat = false,
			Ogre::Real playSpeed = 1.0f,
			ParticleBlendingMethod::ParticleBlendingMethod blendingMethod = ParticleBlendingMethod::AlphaBlending,
			bool fadeIn = false,
			Ogre::Real fadeInTimeMS = 1000.0f,
			bool fadeOut = false,
			Ogre::Real fadeOutTimeMS = 1000.0f);

		/**
		 * @brief Start playing a particle effect
		 * @param name The name of the particle effect to play
		 */
		void playParticleSystem(const Ogre::String& name);

		/**
		 * @brief Stop a particle effect (with optional fade out)
		 * @param name The name of the particle effect to stop
		 */
		void stopParticleSystem(const Ogre::String& name);

		/**
		 * @brief Pause a particle effect
		 * @param name The name of the particle effect to pause
		 */
		void pauseParticleSystem(const Ogre::String& name);

		/**
		 * @brief Resume a paused particle effect
		 * @param name The name of the particle effect to resume
		 */
		void resumeParticleSystem(const Ogre::String& name);

		/**
		 * @brief Update all active particle effects (call every frame)
		 * @param dt Delta time in seconds
		 */
		void update(Ogre::Real dt);

		/**
		 * @brief Destroy all particle effects and clean up resources
		 */
		void destroyContent(void);

		/**
		 * @brief Get particle effect data by name
		 * @param name The name of the particle effect
		 * @return Pointer to the particle data, or nullptr if not found
		 */
		ParticleFxData* getParticle(const Ogre::String& name);

		/**
		 * @brief Remove and destroy a specific particle effect
		 * @param name The name of the particle effect to remove
		 */
		void removeParticle(const Ogre::String& name);

		/**
		 * @brief Set the global position for a particle effect
		 * @param name The name of the particle effect
		 * @param position The world position
		 */
		void setGlobalPosition(const Ogre::String& name, const Ogre::Vector3& position);

		/**
		 * @brief Set the global orientation for a particle effect
		 * @param name The name of the particle effect
		 * @param orientation The orientation quaternion
		 */
		void setGlobalOrientation(const Ogre::String& name, const Ogre::Quaternion& orientation);

		/**
		 * @brief Set the scale for a particle effect
		 * @param name The name of the particle effect
		 * @param scale The scale vector
		 */
		void setScale(const Ogre::String& name, const Ogre::Vector2& scale);

		/**
		 * @brief Set the play speed for a particle effect
		 * @param name The name of the particle effect
		 * @param playSpeed The speed multiplier (>1.0 = faster, <1.0 = slower)
		 */
		void setPlaySpeed(const Ogre::String& name, Ogre::Real playSpeed);

		/**
		 * @brief Check if a particle effect is currently playing
		 * @param name The name of the particle effect
		 * @return True if playing, false otherwise
		 */
		bool isPlaying(const Ogre::String& name) const;

		/**
		 * @brief Get the number of active particles in an effect
		 * @param name The name of the particle effect
		 * @return Number of active particles
		 */
		size_t getNumActiveParticles(const Ogre::String& name) const;

	private:
		ParticleFxModule(const Ogre::String& appStateName);
		~ParticleFxModule();

		/**
		 * @brief Internal helper to create the actual particle effect
		 * @param particleData Reference to the particle data structure
		 * @return True if successful, false otherwise
		 */
		bool createParticleEffect(ParticleFxData& particleData);

		/**
		 * @brief Internal helper to destroy a particle effect
		 * @param particleData Reference to the particle data structure
		 */
		void destroyParticleEffect(ParticleFxData& particleData);

		/**
		 * @brief Destroy everything including clones
		 * @param particleData Reference to the particle data structure
		 */
		void destroyEverything(ParticleFxData& particleData);

		/**
		 * @brief Start particle emission
		 * @param particleData Reference to the particle data structure
		 */
		void startParticleEffect(ParticleFxData& particleData);

		/**
		 * @brief Stop particle emission
		 * @param particleData Reference to the particle data structure
		 */
		void stopParticleEffect(ParticleFxData& particleData);

		/**
		 * @brief Apply blending method to particle material
		 * @param particleData Reference to the particle data structure
		 */
		void applyBlendingMethod(ParticleFxData& particleData);

		/**
		 * @brief Update fade in progress
		 * @param particleData Reference to the particle data structure
		 * @param dt Delta time in seconds
		 */
		void updateFadeIn(ParticleFxData& particleData, Ogre::Real dt);

		/**
		 * @brief Update fade out progress
		 * @param particleData Reference to the particle data structure
		 * @param dt Delta time in seconds
		 */
		void updateFadeOut(ParticleFxData& particleData, Ogre::Real dt);

		/**
		 * @brief Set emission rate factor for all emitters (0.0 to 1.0)
		 * @param particleData Reference to the particle data structure
		 * @param factor The emission rate factor
		 */
		void setEmissionRateFactor(ParticleFxData& particleData, Ogre::Real factor);

		/**
		 * @brief Apply velocity speed factor for all emitters
		 * @param particleData Reference to the particle data structure
		 * @param speedFactor The velocity speed factor
		 */
		void applyVelocitySpeedFactor(ParticleFxData& particleData, Ogre::Real speedFactor);

		/**
		 * @brief Store original emission rates and velocities
		 * @param particleData Reference to the particle data structure
		 */
		void storeOriginalEmissionRates(ParticleFxData& particleData);

		/**
		 * @brief Restore original emission rates and velocities
		 * @param particleData Reference to the particle data structure
		 */
		void restoreOriginalEmissionRates(ParticleFxData& particleData);

		/**
		 * @brief Reset clone to base template values with current scale applied
		 * @param particleData Reference to the particle data structure
		 * @param particleSystemDefInstance The cloned ParticleSystemDef to modify
		 */
		void resetClone(ParticleFxData& particleData, Ogre::ParticleSystemDef* particleSystemDefInstance);

		/**
		 * @brief Get the maximum time-to-live from particle definition
		 * @param def The particle system definition
		 * @return Maximum TTL in seconds
		 */
		Ogre::Real getMaxTtlSecondsFromDef(const Ogre::ParticleSystemDef* def) const;

		/**
		 * @brief Begin the fade out process
		 * @param particleData Reference to the particle data structure
		 */
		void beginFadeOut(ParticleFxData& particleData);

		ParticleBlendingMethod::ParticleBlendingMethod parseMaterialFileForBlendingMode(const Ogre::String& fileContent, const Ogre::String& materialName);

		ParticleBlendingMethod::ParticleBlendingMethod detectBlendingModeFromMaterial(const Ogre::String& materialName);

		ParticleBlendingMethod::ParticleBlendingMethod getBlendingModeForTemplate(const Ogre::String& templateName);

		void precacheAllMaterialAnnotations(void);

		void parseAndCacheAllAnnotationsInFile(const Ogre::String& fileContent);

		TextureAtlasInfo getTextureAtlasForMaterial(const Ogre::String& materialName);

		void updateTextureAtlasAnimations(Ogre::Real deltaTime);

		void updateTextureAtlasFrame(const Ogre::String& materialName, TextureAtlasInfo& atlasInfo);

		void clearBlendingModeCache(void);

		void clearBlendingModeCacheForMaterial(const Ogre::String& materialName);
	private:
		Ogre::String appStateName;
		Ogre::SceneManager* sceneManager;
		std::map<Ogre::String, ParticleFxData> particles;
		Ogre::ParticleSystemManager2* particleManager;
		// Level 1 Cache: Material name → Blending mode (persistent across all particles)
		std::map<Ogre::String, ParticleBlendingMethod::ParticleBlendingMethod> materialBlendingModeCache;
		std::map<Ogre::String, TextureAtlasInfo> materialTextureAtlasCache;
	};

}; // namespace end

#endif /* PARTICLE_FX_MODULE_H_ */

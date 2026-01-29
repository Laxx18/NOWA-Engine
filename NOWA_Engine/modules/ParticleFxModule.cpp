/*
Copyright (c) 2025 Lukas Kalinowski

GPL v3
*/

#include "NOWAPrecompiled.h"
#include "ParticleFxModule.h"
#include "modules/GraphicsModule.h"
#include "utilities/MathHelper.h"
#include "main/Core.h"
#include "main/AppStateManager.h"

#include "OgreSceneManager.h"
#include "OgreHlmsManager.h"
#include "OgreHlms.h"
#include "OgreHlmsPbs.h"
#include "OgreHlmsUnlit.h"
#include "ParticleSystem/OgreParticleSystemManager2.h"

namespace NOWA
{
	ParticleFxModule::ParticleFxModule(const Ogre::String& appStateName)
		: appStateName(appStateName),
		sceneManager(nullptr)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[ParticleFxModule] Module created");
	}

	ParticleFxModule::~ParticleFxModule()
	{
	}

	void ParticleFxModule::init(Ogre::SceneManager* sceneManager)
	{
		this->sceneManager = sceneManager;

		// Set max particles quota
		this->particleManager = sceneManager->getParticleSystemManager2();
		if (nullptr != this->particleManager)
		{
			// Max particles per system (NOT total systems!)
			this->particleManager->setHighestPossibleQuota(512, 0);
		}

		this->precacheAllMaterialAnnotations();
	}

	void ParticleFxModule::destroyContent(void)
	{
		for (auto& it = this->particles.begin(); it != this->particles.end(); ++it)
		{
			this->destroyEverything(it->second);
		}

		// Explicitly destroy all particle system definitions
		// before the SceneManager destructor tries to do it
		if (nullptr != this->particleManager)
		{
			this->particleManager->destroyAllParticleSystems();
			this->particleManager->destroyAllBillboardSets();
		}

		this->particles.clear();
		this->sceneManager = nullptr;
	}

	ParticleBlendingMethod::ParticleBlendingMethod ParticleFxModule::createParticleSystem(const Ogre::String& name, const Ogre::String& templateName,
		Ogre::Real playTimeMS, const Ogre::Quaternion& orientation, const Ogre::Vector3& position, const Ogre::Vector2& scale, bool repeat, Ogre::Real playSpeed, 
		ParticleBlendingMethod::ParticleBlendingMethod blendingMethod, bool fadeIn, Ogre::Real fadeInTimeMS, bool fadeOut, Ogre::Real fadeOutTimeMS)
	{
		ParticleBlendingMethod::ParticleBlendingMethod detectedMode = blendingMethod;

		auto it = this->particles.find(name);

		if (it == this->particles.end())
		{
			// ========================================
			// FIRST TIME CREATION
			// ========================================
			ParticleFxData particleData;
			particleData.particleTemplateName = templateName;
			particleData.particlePlayTime = playTimeMS;
			particleData.particleInitialPlayTime = playTimeMS;
			particleData.particlePlaySpeed = playSpeed;
			particleData.particleOffsetOrientation = orientation;
			particleData.particleOffsetPosition = position;
			particleData.particleScale = scale;

			// Determine blending mode
			if (blendingMethod != ParticleBlendingMethod::FromMaterial)
			{
				// User explicitly set mode
				particleData.blendingMethod = blendingMethod;
				particleData.blendingModeInitialized = true;
				detectedMode = blendingMethod;
			}
			else
			{
				// Auto-detect from material (with caching!)
				detectedMode = this->getBlendingModeForTemplate(templateName);
				particleData.blendingMethod = detectedMode;
				particleData.blendingModeInitialized = true;
			}

			particleData.fadeIn = fadeIn;
			particleData.fadeInTimeMS = fadeInTimeMS;
			particleData.fadeOut = fadeOut;
			particleData.fadeOutTimeMS = fadeOutTimeMS;
			particleData.repeat = repeat;
			particleData.activated = false;

			this->particles.emplace(name, particleData);
		}
		else
		{
			// ========================================
			// PARTICLE ALREADY EXISTS (play/stop/play scenario)
			// ========================================
			ParticleFxData& existingData = it->second;

			// Update playback parameters
			existingData.particlePlayTime = playTimeMS;
			existingData.particleInitialPlayTime = playTimeMS;
			existingData.particlePlaySpeed = playSpeed;
			existingData.repeat = repeat;

			// CACHE HIT: Blending mode already set, no need to detect again!
			if (existingData.blendingModeInitialized)
			{
				detectedMode = existingData.blendingMethod;

				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
					"[ParticleFxModule] Using cached blending mode for: " + name);
			}
			else
			{
				// First time setting blending mode for existing particle
				if (blendingMethod != ParticleBlendingMethod::FromMaterial)
				{
					existingData.blendingMethod = blendingMethod;
					existingData.blendingModeInitialized = true;
					detectedMode = blendingMethod;
				}
				else
				{
					detectedMode = this->getBlendingModeForTemplate(templateName);
					existingData.blendingMethod = detectedMode;
					existingData.blendingModeInitialized = true;
				}
			}
		}

		return detectedMode;
	}

	void ParticleFxModule::playParticleSystem(const Ogre::String& name)
	{
		auto it = this->particles.find(name);
		if (it != this->particles.end())
		{
			// Reset the time
			it->second.particlePlayTime = it->second.particleInitialPlayTime;
			it->second.activated = true;

			// Cancel any pending drain/fade
			it->second.pendingDrainTimeMs = 0.0f;
			it->second.pendingRestartAfterDrain = false;
			it->second.fadeState = ParticleFadeState::None;
			it->second.fadeProgress = 1.0f;

			this->startParticleEffect(it->second);
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ParticleFxModule] Error: Cannot play particle system, because the particle name: '" + name + "' does not exist.");
		}
	}

	void ParticleFxModule::stopParticleSystem(const Ogre::String& name)
	{
		auto it = this->particles.find(name);
		if (it != this->particles.end())
		{
			it->second.activated = false;
			it->second.pendingRestartAfterDrain = false;

			if (it->second.fadeOut && it->second.fadeState != ParticleFadeState::FadingOut && nullptr != it->second.particleSystem)
			{
				this->beginFadeOut(it->second);
			}
			else
			{
				this->stopParticleEffect(it->second);
				it->second.isEmitting = false;
			}

			it->second.particlePlayTime = it->second.particleInitialPlayTime;
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ParticleFxModule] Error: Cannot stop particle system, because the particle name: '" + name + "' does not exist.");
		}
	}

	void ParticleFxModule::pauseParticleSystem(const Ogre::String& name)
	{
		auto it = this->particles.find(name);
		if (it != this->particles.end())
		{
			// Stop emission but keep existing particles alive
			it->second.isEmitting = false;
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ParticleFxModule] Error: Cannot pause particle system, because the particle name: '" + name + "' does not exist.");
		}
	}

	void ParticleFxModule::resumeParticleSystem(const Ogre::String& name)
	{
		auto it = this->particles.find(name);
		if (it != this->particles.end())
		{
			if (it->second.activated && nullptr != it->second.particleSystem)
			{
				it->second.isEmitting = true;
			}
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,  "[ParticleFxModule] Error: Cannot resume particle system, because the particle name: '" + name + "' does not exist.");
		}
	}

	void ParticleFxModule::update(Ogre::Real dt)
	{
		if (true == this->particles.empty())
		{
			return;
		}

		auto closureFunction = [this](Ogre::Real renderDt)
		{
			for (auto& particleEntry : this->particles)
			{
				ParticleFxData& particleData = particleEntry.second;

				if (nullptr == particleData.particleSystem)
				{
					continue;
				}


				// Update camera position for GPU particles
				// TODO: What about splitscreen?
				Ogre::Camera* camera = NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
				if (nullptr != camera)
				{
					this->particleManager->setCameraPosition(camera->getDerivedPosition());
				}

				this->updateTextureAtlasAnimations(renderDt);

				// Handle fade states
				if (particleData.fadeState == ParticleFadeState::FadingIn)
				{
					this->updateFadeIn(particleData, renderDt);
				}
				else if (particleData.fadeState == ParticleFadeState::FadingOut)
				{
					this->updateFadeOut(particleData, renderDt);
				}

				// Handle draining after fade out
				if (particleData.pendingDrainTimeMs > 0.0f)
				{
					particleData.pendingDrainTimeMs -= renderDt * 1000.0f;

					if (particleData.pendingDrainTimeMs <= 0.0f)
					{
						particleData.pendingDrainTimeMs = 0.0f;

						this->stopParticleEffect(particleData);

						particleData.isEmitting = false;

						if (particleData.pendingRestartAfterDrain)
						{
							particleData.pendingRestartAfterDrain = false;
							particleData.particlePlayTime = particleData.particleInitialPlayTime;
							this->startParticleEffect(particleData);
						}
					}
				}

				// Only play activated particle effects
				if (true == particleData.activated && particleData.isEmitting)
				{
					Ogre::Real& playTime = particleData.particlePlayTime;

					// Play particle effect forever or when user stops manually
					if (0.0f == particleData.particleInitialPlayTime)
					{
						continue;
					}

					// Control the execution time
					if (playTime > 0.0f)
					{
						playTime -= renderDt * 1000.0f * particleData.particlePlaySpeed;
					}
					else
					{
						// Time expired
						if (particleData.repeat)
						{
							// Restart
							if (particleData.fadeOut)
							{
								// Schedule restart after drain
								this->beginFadeOut(particleData);
								particleData.pendingRestartAfterDrain = true;
							}
							else
							{
								// Immediate restart
								particleData.particlePlayTime = particleData.particleInitialPlayTime;
								this->stopParticleEffect(particleData);
								this->startParticleEffect(particleData);
							}
						}
						else
						{
							// Stop
							if (particleData.fadeOut)
							{
								this->beginFadeOut(particleData);
							}
							else
							{
								this->stopParticleEffect(particleData);
								particleData.activated = false;
								particleData.isEmitting = false;
							}
						}
					}
				}
			}
		};
		Ogre::String id = "ParticleFxModule::update";
		NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction);
	}

	ParticleFxData* ParticleFxModule::getParticle(const Ogre::String& name)
	{
		auto it = this->particles.find(name);
		if (it != this->particles.end())
		{
			return &it->second;
		}
		return nullptr;
	}

	void ParticleFxModule::removeParticle(const Ogre::String& name)
	{
		auto it = this->particles.find(name);
		if (it != this->particles.end())
		{
			this->destroyEverything(it->second);
			this->particles.erase(it);
		}
	}

	void ParticleFxModule::setGlobalPosition(const Ogre::String& name, const Ogre::Vector3& position)
	{
		auto it = this->particles.find(name);
		if (it != this->particles.end() && nullptr != it->second.particleNode)
		{
			NOWA::GraphicsModule::getInstance()->updateNodePosition(it->second.particleNode, position);
		}
	}

	void ParticleFxModule::setGlobalOrientation(const Ogre::String& name, const Ogre::Quaternion& orientation)
	{
		auto it = this->particles.find(name);
		if (it != this->particles.end() && nullptr != it->second.particleNode)
		{
			NOWA::GraphicsModule::getInstance()->updateNodeOrientation(it->second.particleNode, orientation);
		}
	}

	void ParticleFxModule::setScale(const Ogre::String& name, const Ogre::Vector2& scale)
	{
		auto it = this->particles.find(name);
		if (it != this->particles.end())
		{
			ParticleFxData& particleData = it->second;

			if (nullptr == particleData.particleSystem)
			{
				particleData.particleScale = scale;
				return;
			}

			// Store state to preserve it
			bool wasPlaying = particleData.isEmitting && particleData.activated;
			Ogre::Real remainingTime = particleData.particlePlayTime;

			particleData.particleScale = scale;

			// Destroy instance (but not clone)
			this->destroyParticleEffect(particleData);

			// Recreate with new scale (reuses existing clone via resetClone)
			this->createParticleEffect(particleData);

			// Restore state
			if (true == wasPlaying)
			{
				particleData.isEmitting = true;
			}
			particleData.particlePlayTime = remainingTime;
		}
	}

	void ParticleFxModule::setPlaySpeed(const Ogre::String& name, Ogre::Real playSpeed)
	{
		auto it = this->particles.find(name);
		if (it != this->particles.end())
		{
			it->second.particlePlaySpeed = playSpeed;
			this->applyVelocitySpeedFactor(it->second, playSpeed);
		}
	}

	bool ParticleFxModule::isPlaying(const Ogre::String& name) const
	{
		auto it = this->particles.find(name);
		if (it != this->particles.end())
		{
			return it->second.isEmitting && it->second.activated;
		}
		return false;
	}

	size_t ParticleFxModule::getNumActiveParticles(const Ogre::String& name) const
	{
		auto it = this->particles.find(name);
		if (it != this->particles.end() && nullptr != it->second.particleSystem)
		{
			const Ogre::ParticleSystemDef* particleSystemDef = it->second.particleSystem->getParticleSystemDef();
			if (nullptr != particleSystemDef)
			{
				return particleSystemDef->getNumSimdActiveParticles();
			}
		}
		return 0;
	}

	bool ParticleFxModule::createParticleEffect(ParticleFxData& particleData)
	{
		if (nullptr == this->sceneManager)
		{
			return false;
		}

		const Ogre::String& templateName = particleData.particleTemplateName;
		if (true == templateName.empty())
		{
			return false;
		}

		// Clone must be unique per template and name
		const Ogre::String cloneName = "ParticleFX_NOWA_" + templateName + "_" + Ogre::StringConverter::toString(rand());

		// Template changed? Destroy EVERYTHING and start fresh
		if (false == particleData.baseParticleTemplateName.empty() && particleData.baseParticleTemplateName != templateName)
		{
			this->destroyEverything(particleData);
		}

		// Already created?
		if (nullptr != particleData.particleSystem)
		{
			return true;
		}

		GraphicsModule::RenderCommand renderCommand = [this, &particleData, templateName, cloneName]()
		{
			Ogre::ParticleSystemManager2* particleManager = this->sceneManager->getParticleSystemManager2();

			Ogre::ParticleSystemDef* particleSystemDefInstance = nullptr;

			// If our clone exists, reuse it
			if (particleManager && true == particleManager->hasParticleSystemDef(cloneName, false))
			{
				particleSystemDefInstance = particleManager->getParticleSystemDef(cloneName, false);
				this->resetClone(particleData, particleSystemDefInstance);
			}
			else
			{
				Ogre::ParticleSystemDef* baseDef = particleManager->getParticleSystemDef(templateName);

				try
				{
					particleSystemDefInstance = baseDef->clone(cloneName, particleManager);
				}
				catch (...)
				{
					particleSystemDefInstance = particleManager->getParticleSystemDef(cloneName, false);
				}

				this->resetClone(particleData, particleSystemDefInstance);
			}

			if (particleSystemDefInstance && false == particleSystemDefInstance->isInitialized())
			{
				Ogre::VaoManager* vaoManager = Core::getSingletonPtr()->getOgreRoot()->getRenderSystem()->getVaoManager();
				particleSystemDefInstance->init(vaoManager);
			}

			particleData.particleSystem = this->sceneManager->createParticleSystem2(cloneName);
			if (nullptr == particleData.particleSystem)
			{
				return;
			}

			particleData.particleNode = this->sceneManager->getRootSceneNode()->createChildSceneNode();
			particleData.particleNode->attachObject(particleData.particleSystem);

			particleData.particleSystem->setRenderQueueGroup(RENDER_QUEUE_PARTICLE_STUFF);
			particleData.particleSystem->setCastShadows(false);

			// Apply blending mode (existing)
			this->applyBlendingMethod(particleData);

			this->storeOriginalEmissionRates(particleData);

			Ogre::Real speed = particleData.particlePlaySpeed;
			if (speed != 1.0f)
			{
				this->applyVelocitySpeedFactor(particleData, speed);
			}

			if (particleData.fadeIn)
			{
				particleData.fadeState = ParticleFadeState::FadingIn;
				particleData.fadeProgress = 0.0f;
				this->setEmissionRateFactor(particleData, 0.0f);
			}
			else
			{
				particleData.fadeState = ParticleFadeState::None;
				particleData.fadeProgress = 1.0f;
			}
		};
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ParticleFxModule::createParticleEffect");

		particleData.clonedDefName = cloneName;
		particleData.baseParticleTemplateName = templateName;

		return (particleData.particleSystem != nullptr);
	}

	void ParticleFxModule::destroyParticleEffect(ParticleFxData& particleData)
	{
		if (nullptr == particleData.particleSystem && nullptr == particleData.particleNode)
		{
			return;
		}

		NOWA::GraphicsModule::getInstance()->removeTrackedNode(particleData.particleNode);

		GraphicsModule::RenderCommand renderCommand = [this, &particleData]()
		{
			try
			{
				// Restore clone defaults so next play is not stuck at emissionRate=0 after fade out
				this->restoreOriginalEmissionRates(particleData);

				if (particleData.particleNode && particleData.particleSystem && particleData.particleSystem->isAttached())
				{
					particleData.particleNode->detachObject(particleData.particleSystem);
				}

				if (particleData.particleSystem)
				{
					this->sceneManager->destroyParticleSystem2(particleData.particleSystem);
					particleData.particleSystem = nullptr;
				}

				if (particleData.particleNode)
				{
					this->sceneManager->getRootSceneNode()->removeAndDestroyChild(particleData.particleNode);
					particleData.particleNode = nullptr;
				}
			}
			catch (const Ogre::Exception& e)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ParticleFxModule] Exception destroying particle effect: " + e.getFullDescription());
			}
		};
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ParticleFxModule::destroyParticleEffect");

		particleData.particleSystem = nullptr;
		particleData.particleNode = nullptr;
		particleData.isEmitting = false;
	}

	void ParticleFxModule::destroyEverything(ParticleFxData& particleData)
	{
		// First destroy the instance and node
		this->destroyParticleEffect(particleData);

		// Now destroy the clone
		if (false == particleData.clonedDefName.empty())
		{
			GraphicsModule::RenderCommand renderCommand = [this, &particleData]()
			{
				try
				{
					Ogre::ParticleSystemManager2* particleManager = this->sceneManager->getParticleSystemManager2();

					if (particleManager && particleManager->hasParticleSystemDef(particleData.clonedDefName, false))
					{
						Ogre::ParticleSystemDef* clone = particleManager->getParticleSystemDef(particleData.clonedDefName, false);

						// CRITICAL: Must destroy all instances first
						clone->_destroyAllParticleSystems();

						// Now destroy the def itself
						particleManager->destroyAllParticleSystems();
					}
				}
				catch (const Ogre::Exception& e)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ParticleFxModule] Exception destroying clone: " + e.getFullDescription());
				}
			};
			NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ParticleFxModule::destroyEverything");

			particleData.clonedDefName.clear();
			particleData.baseParticleTemplateName.clear();
		}
	}

	void ParticleFxModule::startParticleEffect(ParticleFxData& particleData)
	{
		// Destroy old instance
		this->destroyParticleEffect(particleData);

		// Create new instance (reuses existing clone)
		this->createParticleEffect(particleData);

		particleData.particlePlayTime = particleData.particleInitialPlayTime;
		particleData.isEmitting = true;
	}

	void ParticleFxModule::stopParticleEffect(ParticleFxData& particleData)
	{
		this->destroyParticleEffect(particleData);
		particleData.isEmitting = false;
	}

#if 0
	void ParticleFxModule::applyBlendingMethod(ParticleFxData& particleData)
	{
		if (nullptr == particleData.particleSystem)
		{
			return;
		}

		Ogre::ParticleSystem2* particleSystem2 = particleData.particleSystem;

		const Ogre::ParticleSystemDef* particleSystemDef = particleSystem2->getParticleSystemDef();
		if (nullptr == particleSystemDef)
		{
			return;
		}

		Ogre::HlmsManager* hlmsManager = Core::getSingletonPtr()->getOgreRoot()->getHlmsManager();
		Ogre::HlmsDatablock* datablock = hlmsManager->getDatablockNoDefault(particleSystemDef->getDatablockName());

		if (nullptr == datablock)
		{
			return;
		}

		if (datablock->getCreator()->getType() == Ogre::HLMS_UNLIT)
		{
			Ogre::HlmsUnlitDatablock* unlitDatablock = static_cast<Ogre::HlmsUnlitDatablock*>(datablock);

			switch (particleData.blendingMethod)
			{
			case ParticleBlendingMethod::AlphaHashing:
				unlitDatablock->setUseAlphaFromTextures(true);
				unlitDatablock->setAlphaTest(Ogre::CMPF_ALWAYS_PASS);
				unlitDatablock->setAlphaHashing(true);
				unlitDatablock->setAlphaToCoverage(false, Ogre::HlmsMacroblock::msaa_auto, false);
				unlitDatablock->setTransparency(0.0f, Ogre::HlmsUnlitDatablock::None);
				break;

			case ParticleBlendingMethod::AlphaHashingA2C:
				unlitDatablock->setUseAlphaFromTextures(true);
				unlitDatablock->setAlphaTest(Ogre::CMPF_ALWAYS_PASS);
				unlitDatablock->setAlphaHashing(true);
				unlitDatablock->setAlphaToCoverage(true, Ogre::HlmsMacroblock::msaa_auto, false);
				unlitDatablock->setTransparency(0.0f, Ogre::HlmsUnlitDatablock::None);
				break;

			case ParticleBlendingMethod::AlphaBlending:
				unlitDatablock->setUseAlphaFromTextures(true);
				unlitDatablock->setAlphaTest(Ogre::CMPF_ALWAYS_PASS);
				unlitDatablock->setAlphaHashing(false);
				unlitDatablock->setAlphaToCoverage(false, Ogre::HlmsMacroblock::msaa_auto, false);
				unlitDatablock->setTransparency(1.0f, Ogre::HlmsUnlitDatablock::Transparent);
				break;
			}
		}
	}
#endif

	void ParticleFxModule::applyBlendingMethod(ParticleFxData& particleData)
	{
		if (nullptr == particleData.particleSystem)
		{
			return;
		}

		const Ogre::ParticleSystemDef* particleSystemDef = particleData.particleSystem->getParticleSystemDef();
		if (nullptr == particleSystemDef)
		{
			return;
		}

		const Ogre::String& materialName = particleSystemDef->getMaterialName();
		if (true == materialName.empty())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
				"[ParticleFxComponent] Particle system has no material assigned.");
			return;
		}

		Ogre::HlmsManager* hlmsManager = Core::getSingletonPtr()->getOgreRoot()->getHlmsManager();
		Ogre::HlmsDatablock* datablock = hlmsManager->getDatablock(materialName);

		if (nullptr == datablock)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
				"[ParticleFxComponent] Could not find datablock for particle material: " + materialName);
			return;
		}

		const ParticleBlendingMethod::ParticleBlendingMethod currentMethod = particleData.blendingMethod;

		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("ParticleFxComponent::applyBlendingMethod", _2(datablock, currentMethod),
		{
			if (currentMethod == ParticleBlendingMethod::AlphaHashing ||
				currentMethod == ParticleBlendingMethod::AlphaHashingA2C)
			{
				// ========================================
				// ALPHA HASHING MODE
				// ========================================
				Ogre::HlmsBlendblock blendblock = *datablock->getBlendblock();
				blendblock.setBlendType(Ogre::SBT_REPLACE);
				blendblock.mAlphaToCoverage = (currentMethod == ParticleBlendingMethod::AlphaHashingA2C)
						? Ogre::HlmsBlendblock::A2cEnabledMsaaOnly
						: Ogre::HlmsBlendblock::A2cDisabled;
				datablock->setBlendblock(blendblock);

				Ogre::HlmsMacroblock macroblock = *datablock->getMacroblock();
				macroblock.mDepthWrite = true;
				datablock->setMacroblock(macroblock);

				datablock->setAlphaHashing(true);
			}
			else if (currentMethod == ParticleBlendingMethod::AlphaBlending)
			{
				// ========================================
				// ADDITIVE BLENDING (SBT_ADD)
				// ========================================
				// Best for: Rain, fire, sparks, stars, explosions
				// White on black textures - black becomes transparent

				Ogre::HlmsBlendblock blendblock = *datablock->getBlendblock();
				blendblock.setBlendType(Ogre::SBT_ADD);
				blendblock.mAlphaToCoverage = Ogre::HlmsBlendblock::A2cDisabled;
				datablock->setBlendblock(blendblock);

				Ogre::HlmsMacroblock macroblock = *datablock->getMacroblock();
				macroblock.mDepthWrite = false;
				macroblock.mDepthCheck = true;
				datablock->setMacroblock(macroblock);

				datablock->setAlphaHashing(false);
			}
			else if (currentMethod == ParticleBlendingMethod::AlphaTransparent)
			{
				// ========================================
				// TRADITIONAL ALPHA BLENDING (SBT_TRANSPARENT_ALPHA)
				// ========================================
				// Best for: Smoke, clouds, sprites with proper alpha channel
				// Requires texture to have alpha channel!

				Ogre::HlmsBlendblock blendblock = *datablock->getBlendblock();
				blendblock.setBlendType(Ogre::SBT_TRANSPARENT_ALPHA);
				blendblock.mAlphaToCoverage = Ogre::HlmsBlendblock::A2cDisabled;
				datablock->setBlendblock(blendblock);

				Ogre::HlmsMacroblock macroblock = *datablock->getMacroblock();
				macroblock.mDepthWrite = false;
				macroblock.mDepthCheck = true;
				datablock->setMacroblock(macroblock);

				datablock->setAlphaHashing(false);
			}
			else if (currentMethod == ParticleBlendingMethod::TransparentColour)
			{
				Ogre::HlmsBlendblock blendblock = *datablock->getBlendblock();
				blendblock.setBlendType(Ogre::SBT_TRANSPARENT_COLOUR);
				blendblock.mAlphaToCoverage = Ogre::HlmsBlendblock::A2cDisabled;
				datablock->setBlendblock(blendblock);

				Ogre::HlmsMacroblock macroblock = *datablock->getMacroblock();
				macroblock.mDepthWrite = false;
				macroblock.mDepthCheck = true;
				datablock->setMacroblock(macroblock);

				datablock->setAlphaHashing(false);
			}
			
		});
	}

	void ParticleFxModule::updateFadeIn(ParticleFxData& particleData, Ogre::Real dt)
	{
		if (particleData.fadeState != ParticleFadeState::FadingIn)
		{
			return;
		}

		const Ogre::Real fadeInTimeSeconds = particleData.fadeInTimeMS / 1000.0f;

		if (fadeInTimeSeconds <= 0.0f)
		{
			// Instant fade in
			particleData.fadeProgress = 1.0f;
			particleData.fadeState = ParticleFadeState::None;
			this->setEmissionRateFactor(particleData, 1.0f);
			return;
		}

		// Advance fade progress
		particleData.fadeProgress += (dt * particleData.particlePlaySpeed) / fadeInTimeSeconds;

		if (particleData.fadeProgress >= 1.0f)
		{
			particleData.fadeProgress = 1.0f;
			particleData.fadeState = ParticleFadeState::None;
		}

		// Apply smooth easing (ease-in-out)
		Ogre::Real easedProgress = particleData.fadeProgress * particleData.fadeProgress * (3.0f - 2.0f * particleData.fadeProgress);

		this->setEmissionRateFactor(particleData, easedProgress);
	}

	void ParticleFxModule::updateFadeOut(ParticleFxData& particleData, Ogre::Real dt)
	{
		const Ogre::Real fadeOutMs = particleData.fadeOutTimeMS;
		if (fadeOutMs <= 0.0f)
		{
			particleData.fadeProgress = 0.0f;
			this->setEmissionRateFactor(particleData, 0.0f);
		}
		else
		{
			particleData.fadeProgress -= (dt * 1000.0f) / fadeOutMs;
			if (particleData.fadeProgress < 0.0f)
				particleData.fadeProgress = 0.0f;

			this->setEmissionRateFactor(particleData, particleData.fadeProgress);
		}

		// Once fully faded, start drain exactly once
		if (particleData.fadeProgress <= 0.0f && particleData.pendingDrainTimeMs <= 0.0f)
		{
			const Ogre::ParticleSystemDef* def = nullptr;
			if (particleData.particleSystem)
				def = particleData.particleSystem->getParticleSystemDef();

			const Ogre::Real maxTtlSec = this->getMaxTtlSecondsFromDef(def);
			particleData.pendingDrainTimeMs = (maxTtlSec * 1000.0f) + 50.0f;
		}
	}

	void ParticleFxModule::setEmissionRateFactor(ParticleFxData& particleData, Ogre::Real factor)
	{
		if (nullptr == particleData.particleSystem)
		{
			return;
		}

		Ogre::ParticleSystemDef* particleSystemDef = const_cast<Ogre::ParticleSystemDef*>(particleData.particleSystem->getParticleSystemDef());
		if (nullptr == particleSystemDef)
		{
			return;
		}

		auto& emitters = particleSystemDef->getEmitters();

		for (size_t i = 0; i < emitters.size() && i < particleData.originalEmissionRates.size(); ++i)
		{
			Ogre::ParticleEmitter* particleEmitterWrite = emitters[i]->asParticleEmitter();
			Ogre::Real newRate = particleData.originalEmissionRates[i] * factor;
			particleEmitterWrite->setEmissionRate(newRate);
		}
	}

	void ParticleFxModule::applyVelocitySpeedFactor(ParticleFxData& particleData, Ogre::Real speedFactor)
	{
		if (nullptr == particleData.particleSystem)
		{
			return;
		}

		Ogre::ParticleSystemDef* particleSystemDef = const_cast<Ogre::ParticleSystemDef*>(particleData.particleSystem->getParticleSystemDef());
		if (nullptr == particleSystemDef)
		{
			return;
		}

		auto& emitters = particleSystemDef->getEmitters();

		for (size_t i = 0; i < emitters.size() && i < particleData.originalMinVelocities.size(); ++i)
		{
			Ogre::ParticleEmitter* particleEmitterWrite = emitters[i]->asParticleEmitter();
			Ogre::Real newMinVel = particleData.originalMinVelocities[i] * speedFactor;
			Ogre::Real newMaxVel = particleData.originalMaxVelocities[i] * speedFactor;
			particleEmitterWrite->setParticleVelocity(newMinVel, newMaxVel);
		}
	}

	void ParticleFxModule::storeOriginalEmissionRates(ParticleFxData& particleData)
	{
		particleData.originalEmissionRates.clear();
		particleData.originalMinVelocities.clear();
		particleData.originalMaxVelocities.clear();

		if (nullptr == particleData.particleSystem)
			return;

		const Ogre::ParticleSystemDef* particleSystemDef = particleData.particleSystem->getParticleSystemDef();
		if (nullptr == particleSystemDef)
		{
			return;
		}

		const auto& emitters = particleSystemDef->getEmitters();
		particleData.originalEmissionRates.reserve(emitters.size());
		particleData.originalMinVelocities.reserve(emitters.size());
		particleData.originalMaxVelocities.reserve(emitters.size());

		for (const Ogre::EmitterDefData* emitter : emitters)
		{
			const Ogre::ParticleEmitter* particleEmitterRead = emitter->asParticleEmitter();
			particleData.originalEmissionRates.push_back(particleEmitterRead->getEmissionRate());
			particleData.originalMinVelocities.push_back(particleEmitterRead->getMinParticleVelocity());
			particleData.originalMaxVelocities.push_back(particleEmitterRead->getMaxParticleVelocity());
		}
	}

	void ParticleFxModule::restoreOriginalEmissionRates(ParticleFxData& particleData)
	{
		if (nullptr == particleData.particleSystem)
		{
			return;
		}

		Ogre::ParticleSystemDef* particleSystemDef = const_cast<Ogre::ParticleSystemDef*>(particleData.particleSystem->getParticleSystemDef());
		if (nullptr == particleSystemDef)
		{
			return;
		}

		auto& emitters = particleSystemDef->getEmitters();

		for (size_t i = 0; i < emitters.size() && i < particleData.originalEmissionRates.size() && i < particleData.originalMinVelocities.size() && i < particleData.originalMaxVelocities.size(); ++i)
		{
			Ogre::ParticleEmitter* particleEmitterWrite = emitters[i]->asParticleEmitter();
			particleEmitterWrite->setEmissionRate(particleData.originalEmissionRates[i]);
			particleEmitterWrite->setParticleVelocity(particleData.originalMinVelocities[i], particleData.originalMaxVelocities[i]);
		}
	}

	void ParticleFxModule::resetClone(ParticleFxData& particleData, Ogre::ParticleSystemDef* particleSystemDefInstance)
	{
		if (nullptr == particleSystemDefInstance)
		{
			return;
		}

		const Ogre::Vector2 scale = particleData.particleScale;
		const Ogre::Real uniformScale = (scale.x + scale.y) * 0.5f;

		Ogre::ParticleSystemManager2* particleManager = this->sceneManager->getParticleSystemManager2();
		Ogre::ParticleSystemDef* baseDef = particleManager->getParticleSystemDef(particleData.particleTemplateName);

		const auto& baseEmitters = baseDef->getEmitters();
		auto& cloneEmitters = particleSystemDefInstance->getEmitters();

		for (size_t i = 0; i < cloneEmitters.size() && i < baseEmitters.size(); ++i)
		{
			Ogre::EmitterDefData* cloneEmitter = cloneEmitters[i];
			const Ogre::EmitterDefData* baseEmitter = baseEmitters[i];

			Ogre::ParticleEmitter* cloneEmitterWrite = cloneEmitter->asParticleEmitter();
			const Ogre::ParticleEmitter* baseEmitterRead = baseEmitter->asParticleEmitter();

			// --- RESTORE BASE DEFAULTS FIRST ---
			cloneEmitterWrite->setAngle(baseEmitterRead->getAngle());
			cloneEmitterWrite->setDirection(baseEmitterRead->getDirection());
			cloneEmitterWrite->setUp(baseEmitterRead->getUp());

			cloneEmitterWrite->setColourRangeStart(baseEmitterRead->getColourRangeStart());
			cloneEmitterWrite->setColourRangeEnd(baseEmitterRead->getColourRangeEnd());

			cloneEmitterWrite->setMinTimeToLive(baseEmitterRead->getMinTimeToLive());
			cloneEmitterWrite->setMaxTimeToLive(baseEmitterRead->getMaxTimeToLive());

			cloneEmitterWrite->setEmissionRate(baseEmitterRead->getEmissionRate());

			cloneEmitterWrite->setDuration(baseEmitterRead->getMinDuration(), baseEmitterRead->getMaxDuration());
			cloneEmitterWrite->setRepeatDelay(baseEmitterRead->getMinRepeatDelay(), baseEmitterRead->getMaxRepeatDelay());

			const Ogre::Real baseStartTime = baseEmitterRead->getStartTime();
			const bool baseEnabled = baseEmitterRead->getEnabled();

			if (baseStartTime > 0.0f)
			{
				cloneEmitterWrite->setStartTime(baseStartTime);
			}
			else
			{
				cloneEmitterWrite->setEnabled(baseEnabled);
			}

			cloneEmitterWrite->resetDimensions();

			// Scale particle dimensions (this is a size parameter)
			Ogre::Vector2 baseDim = baseEmitter->getInitialDimensions();
			Ogre::Vector2 scaledDim(baseDim.x * scale.x, baseDim.y * scale.y);
			cloneEmitter->setInitialDimensions(scaledDim);

			// Don't scale position - it's an offset, not a size
			Ogre::Vector3 basePos = baseEmitterRead->getPosition();
			// cloneEmitterWrite->setPosition(basePos);  // Removed: * uniformScale
			
			// CORRECT FIX: Reset emitter position to ZERO
			// The emitter should emit from the ParticleNode's position, not offset
			// ParticleNode is already positioned correctly via particleOffsetPosition
			// In particle script, position should be always 0 0 0
			cloneEmitterWrite->setPosition(basePos);

			// Scale velocity (affects particle movement speed)
			Ogre::Real baseMinVel = baseEmitterRead->getMinParticleVelocity();
			Ogre::Real baseMaxVel = baseEmitterRead->getMaxParticleVelocity();
			cloneEmitterWrite->setParticleVelocity(baseMinVel * uniformScale, baseMaxVel * uniformScale);

			// Scale emitter box dimensions (these are size parameters)
			const Ogre::String baseWidthStr = baseEmitterRead->getParameter("width");
			if (false == baseWidthStr.empty())
			{
				Ogre::Real baseWidth = Ogre::StringConverter::parseReal(baseWidthStr);
				cloneEmitterWrite->setParameter("width", Ogre::StringConverter::toString(baseWidth * uniformScale));
			}

			const Ogre::String baseHeightStr = baseEmitterRead->getParameter("height");
			if (false == baseHeightStr.empty())
			{
				Ogre::Real baseHeight = Ogre::StringConverter::parseReal(baseHeightStr);
				cloneEmitterWrite->setParameter("height", Ogre::StringConverter::toString(baseHeight * uniformScale));
			}

			const Ogre::String baseDepthStr = baseEmitterRead->getParameter("depth");
			if (false == baseDepthStr.empty())
			{
				Ogre::Real baseDepth = Ogre::StringConverter::parseReal(baseDepthStr);
				cloneEmitterWrite->setParameter("depth", Ogre::StringConverter::toString(baseDepth * uniformScale));
			}

			// Final sanity check
			if (baseStartTime <= 0.0f && true == baseEnabled)
			{
				cloneEmitterWrite->setEnabled(true);
			}
		}
	}

	Ogre::Real ParticleFxModule::getMaxTtlSecondsFromDef(const Ogre::ParticleSystemDef* def) const
	{
		if (nullptr == def)
		{
			return 0.0f;
		}

		Ogre::Real maxTtl = 0.0f;

		const auto& emitters = def->getEmitters();
		for (const Ogre::EmitterDefData* emitters : emitters)
		{
			const Ogre::ParticleEmitter* emitter = emitters->asParticleEmitter();
			if (emitter)
			{
				maxTtl = std::max(maxTtl, emitter->getMaxTimeToLive());
			}
		}

		return maxTtl;
	}

	void ParticleFxModule::beginFadeOut(ParticleFxData& particleData)
	{
		if (particleData.fadeState == ParticleFadeState::FadingOut)
		{
			return;
		}

		particleData.fadeState = ParticleFadeState::FadingOut;

		if (particleData.fadeProgress <= 0.0f)
		{
			particleData.fadeProgress = 1.0f;
		}

		// Drain will be armed by updateFadeOut once fadeProgress reaches 0
		particleData.pendingDrainTimeMs = 0.0f;
	}

	ParticleBlendingMethod::ParticleBlendingMethod ParticleFxModule::detectBlendingModeFromMaterial(const Ogre::String& materialName)
	{
		// Default to AlphaBlending if not found
		ParticleBlendingMethod::ParticleBlendingMethod defaultMode = ParticleBlendingMethod::AlphaBlending;

		try
		{
			// Find the material script file(s) in resource groups
			Ogre::ResourceGroupManager& rgm = Ogre::ResourceGroupManager::getSingleton();

			// Check ParticleFX2 group first (your particle materials)
			Ogre::StringVectorPtr scriptNames = rgm.findResourceNames("ParticleFX2", "*.material");

			if (!scriptNames || scriptNames->empty())
			{
				// Try other common resource groups
				scriptNames = rgm.findResourceNames("General", "*.material");
			}

			if (!scriptNames || scriptNames->empty())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
					"[ParticleFxModule] No material files found, using default blending mode");
				return defaultMode;
			}

			// Search through material files
			for (const Ogre::String& scriptName : *scriptNames)
			{
				try
				{
					Ogre::DataStreamPtr stream = rgm.openResource(scriptName, "ParticleFX2");
					if (!stream)
						stream = rgm.openResource(scriptName, "General");

					if (!stream)
						continue;

					// Read entire file
					Ogre::String content = stream->getAsString();
					stream->close();

					// Parse the file to find this material
					ParticleBlendingMethod::ParticleBlendingMethod mode = this->parseMaterialFileForBlendingMode(content, materialName);

					if (mode != defaultMode)
					{
						// Found it!
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ParticleFxModule] Auto-detected blending mode for material: " + materialName);
						return mode;
					}
				}
				catch (...)
				{
					// Continue to next file
				}
			}
		}
		catch (Ogre::Exception& e)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ParticleFxModule] Error detecting blending mode: " + e.getFullDescription());
		}

		return defaultMode;
	}

	ParticleBlendingMethod::ParticleBlendingMethod ParticleFxModule::parseMaterialFileForBlendingMode(const Ogre::String& fileContent, const Ogre::String& materialName)
	{
		// Default
		ParticleBlendingMethod::ParticleBlendingMethod mode = ParticleBlendingMethod::AlphaBlending;

		// Find the material definition in the file
		size_t materialPos = fileContent.find(materialName);
		if (materialPos == Ogre::String::npos)
		{
			return mode;  // Material not found in this file
		}

		// Make sure we found the actual material definition, not just the name somewhere
		// Look for "hlms MaterialName" or "material MaterialName"
		size_t checkPos = materialPos;

		// Search backwards to find "hlms" or "material" keyword
		size_t searchStart = (materialPos > 100) ? materialPos - 100 : 0;
		Ogre::String beforeMaterial = fileContent.substr(searchStart, materialPos - searchStart);

		// Check if we're actually at a material definition
		size_t hlmsPos = beforeMaterial.rfind("hlms");
		size_t matPos = beforeMaterial.rfind("material");

		if (hlmsPos == Ogre::String::npos && matPos == Ogre::String::npos)
		{
			// Material name appears in wrong context (e.g., in a comment), skip
			return mode;
		}

		// Look backwards from material definition to find the comment
		// Format: // bm: AlphaBlending
		// Search up to 500 chars before the material definition
		searchStart = (materialPos > 500) ? materialPos - 500 : 0;
		Ogre::String searchArea = fileContent.substr(searchStart, materialPos - searchStart);

		// Find "// bm:" comment (last occurrence before material)
		size_t bmPos = searchArea.rfind("// bm:");

		if (bmPos != Ogre::String::npos)
		{
			// Make sure the comment is close to the material definition
			// (not more than 200 chars before it)
			size_t distanceToMaterial = searchArea.length() - bmPos;
			if (distanceToMaterial > 200)
			{
				// Too far away, probably for a different material
				return mode;
			}

			// Extract the blending mode string
			size_t modeStart = bmPos + 7;  // Length of "// bm: "
			size_t modeEnd = searchArea.find_first_of("\r\n", modeStart);

			if (modeEnd != Ogre::String::npos)
			{
				Ogre::String modeStr = searchArea.substr(modeStart, modeEnd - modeStart);

				// trim returns the trimmed string, doesn't modify in place
				Ogre::StringUtil::trim(modeStr);

				// Parse mode string
				if (modeStr == "AlphaHashing")
				{
					mode = ParticleBlendingMethod::AlphaHashing;
				}
				else if (modeStr == "AlphaHashingA2C")
				{
					mode = ParticleBlendingMethod::AlphaHashingA2C;
				}
				else if (modeStr == "AlphaBlending")
				{
					mode = ParticleBlendingMethod::AlphaBlending;
				}
				else if (modeStr == "AlphaTransparent")
				{
					mode = ParticleBlendingMethod::AlphaTransparent;
				}

				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
					"[ParticleFxModule] Found blending mode annotation: '" + modeStr +
					"' for material: " + materialName);
			}
		}
		else
		{
			// No annotation found
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
				"[ParticleFxModule] No blending mode annotation found for material: " + materialName);
		}

		return mode;
	}

	ParticleBlendingMethod::ParticleBlendingMethod ParticleFxModule::getBlendingModeForTemplate(const Ogre::String& templateName)
	{
		// Default
		ParticleBlendingMethod::ParticleBlendingMethod mode = ParticleBlendingMethod::AlphaBlending;

		try
		{
			// Get material name from particle template
			Ogre::ParticleSystemManager2* particleManager = this->sceneManager->getParticleSystemManager2();
			if (!particleManager)
			{
				return mode;
			}

			Ogre::ParticleSystemDef* particleSystemDef = particleManager->getParticleSystemDef(templateName);
			if (!particleSystemDef)
			{
				return mode;
			}

			const Ogre::String& materialName = particleSystemDef->getMaterialName();
			if (materialName.empty())
			{
				return mode;
			}

			// LEVEL 1 CACHE CHECK: Have we detected this material before?
			auto cacheIt = this->materialBlendingModeCache.find(materialName);
			if (cacheIt != this->materialBlendingModeCache.end())
			{
				// Cache hit!
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ParticleFxModule] Material blending mode cache hit: " + materialName);
				return cacheIt->second;
			}

			// CACHE MISS: Detect from material file and cache the result
			mode = this->detectBlendingModeFromMaterial(materialName);

			// Store in cache for future use
			this->materialBlendingModeCache[materialName] = mode;

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ParticleFxModule] Cached blending mode for material: " + materialName);

			return mode;
		}
		catch (...)
		{
			return mode;
		}
	}

	void ParticleFxModule::precacheAllMaterialAnnotations(void)
	{
		try
		{
			Ogre::ResourceGroupManager& rgm = Ogre::ResourceGroupManager::getSingleton();

			// Find all material files
			Ogre::StringVectorPtr scriptNames = rgm.findResourceNames("ParticleFX2", "*.material");
			if (!scriptNames || scriptNames->empty())
			{
				scriptNames = rgm.findResourceNames("General", "*.material");
			}

			if (!scriptNames || scriptNames->empty())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ParticleFxModule] No material files found for annotation caching");
				return;
			}

			// Parse all materials and cache their annotations
			for (const Ogre::String& scriptName : *scriptNames)
			{
				try
				{
					Ogre::DataStreamPtr stream = rgm.openResource(scriptName, "ParticleFX2");
					if (!stream)
						stream = rgm.openResource(scriptName, "General");

					if (!stream)
						continue;

					Ogre::String content = stream->getAsString();
					stream->close();

					// Parse all annotations in this file
					this->parseAndCacheAllAnnotationsInFile(content);
				}
				catch (...)
				{
					// Continue to next file
				}
			}

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[ParticleFxModule] Cached annotations for " +
				Ogre::StringConverter::toString(this->materialBlendingModeCache.size()) + " materials (" +
				Ogre::StringConverter::toString(this->materialBlendingModeCache.size()) + " with texture atlas)");
		}
		catch (Ogre::Exception& e)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
				"[ParticleFxModule] Error caching material annotations: " + e.getFullDescription());
		}
	}

	void ParticleFxModule::parseAndCacheAllAnnotationsInFile(const Ogre::String& fileContent)
	{
		// Parse file for all annotations
		size_t pos = 0;

		while (pos < fileContent.length())
		{
			// Find next material definition
			size_t hlmsPos = fileContent.find("hlms ", pos);
			size_t matPos = fileContent.find("material ", pos);

			size_t materialPos = Ogre::String::npos;
			size_t keywordEnd = 0;

			if (hlmsPos != Ogre::String::npos &&
				(matPos == Ogre::String::npos || hlmsPos < matPos))
			{
				materialPos = hlmsPos;
				keywordEnd = hlmsPos + 5; // "hlms "
			}
			else if (matPos != Ogre::String::npos)
			{
				materialPos = matPos;
				keywordEnd = matPos + 9; // "material "
			}

			if (materialPos == Ogre::String::npos)
				break; // No more materials

			// Extract material name
			size_t nameStart = fileContent.find_first_not_of(" \t", keywordEnd);
			size_t nameEnd = fileContent.find_first_of(" \t\r\n{", nameStart);

			if (nameStart == Ogre::String::npos || nameEnd == Ogre::String::npos)
			{
				pos = materialPos + 1;
				continue;
			}

			Ogre::String materialName = fileContent.substr(nameStart, nameEnd - nameStart);

			// Look backwards for annotations (within 200 chars before material)
			size_t searchStart = (materialPos > 200) ? materialPos - 200 : 0;
			Ogre::String searchArea = fileContent.substr(searchStart, materialPos - searchStart);

			// ========================================
			// Parse "// bm:" annotation (Blending Mode)
			// ========================================
			size_t bmPos = searchArea.rfind("// bm:");
			if (bmPos != Ogre::String::npos)
			{
				size_t modeStart = bmPos + 7;
				size_t modeEnd = searchArea.find_first_of("\r\n", modeStart);

				if (modeEnd != Ogre::String::npos)
				{
					Ogre::String modeStr = searchArea.substr(modeStart, modeEnd - modeStart);
					Ogre::StringUtil::trim(modeStr);

					ParticleBlendingMethod::ParticleBlendingMethod mode = ParticleBlendingMethod::AlphaBlending;

					if (modeStr == "AlphaHashing")
						mode = ParticleBlendingMethod::AlphaHashing;
					else if (modeStr == "AlphaHashingA2C")
						mode = ParticleBlendingMethod::AlphaHashingA2C;
					else if (modeStr == "AlphaBlending")
						mode = ParticleBlendingMethod::AlphaBlending;
					else if (modeStr == "AlphaTransparent")
						mode = ParticleBlendingMethod::AlphaTransparent;

					// Cache blending mode
					this->materialBlendingModeCache[materialName] = mode;
				}
			}

			// ========================================
			// Parse "// ta:" annotation (Texture Atlas)
			// ========================================
			size_t taPos = searchArea.rfind("// ta:");
			if (taPos != Ogre::String::npos)
			{
				size_t valueStart = taPos + 7;
				size_t valueEnd = searchArea.find_first_of("\r\n", valueStart);

				if (valueEnd != Ogre::String::npos)
				{
					Ogre::String taStr = searchArea.substr(valueStart, valueEnd - valueStart);
					Ogre::StringUtil::trim(taStr);

					// Parse format: "4x4@16" (rows x columns @ fps)
					// Or: "4x4" (rows x columns, default fps)
					size_t xPos = taStr.find('x');
					size_t atPos = taStr.find('@');

					if (xPos != Ogre::String::npos)
					{
						try
						{
							Ogre::String rowsStr = taStr.substr(0, xPos);
							Ogre::String colsStr;
							Ogre::String fpsStr;

							if (atPos != Ogre::String::npos)
							{
								colsStr = taStr.substr(xPos + 1, atPos - xPos - 1);
								fpsStr = taStr.substr(atPos + 1);
							}
							else
							{
								colsStr = taStr.substr(xPos + 1);
								fpsStr = "16"; // Default FPS
							}

							Ogre::StringUtil::trim(rowsStr);
							Ogre::StringUtil::trim(colsStr);
							Ogre::StringUtil::trim(fpsStr);

							int rows = Ogre::StringConverter::parseInt(rowsStr);
							int columns = Ogre::StringConverter::parseInt(colsStr);
							Ogre::Real fps = Ogre::StringConverter::parseReal(fpsStr);

							if (rows > 1 && columns > 1)
							{
								// Cache texture atlas info
								this->materialTextureAtlasCache[materialName] = TextureAtlasInfo(rows, columns, fps);

								Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ParticleFxModule] Cached texture atlas: " + taStr + " for material: " + materialName);
							}
						}
						catch (...)
						{
							// Invalid format, skip
						}
					}
				}
			}

			pos = materialPos + 1;
		}
	}

	ParticleFxModule::TextureAtlasInfo ParticleFxModule::getTextureAtlasForMaterial(const Ogre::String& materialName)
	{
		// Check cache
		auto it = this->materialTextureAtlasCache.find(materialName);
		if (it != this->materialTextureAtlasCache.end())
		{
			return it->second;
		}

		// Not found, return default (no atlas)
		return TextureAtlasInfo();
	}

	void ParticleFxModule::updateTextureAtlasAnimations(Ogre::Real deltaTime)
	{
		// Update all materials with texture atlas animation
		for (auto& pair : this->materialTextureAtlasCache)
		{
			const Ogre::String& materialName = pair.first;
			TextureAtlasInfo& atlasInfo = pair.second;

			if (!atlasInfo.hasAtlas)
				continue;

			// Update animation time
			atlasInfo.currentTime += deltaTime;

			// Calculate which frame to show based on FPS
			Ogre::Real frameTime = 1.0f / atlasInfo.fps;  // Time per frame
			int totalFrames = atlasInfo.rows * atlasInfo.columns;

			// Calculate current frame index
			int newFrame = static_cast<int>(atlasInfo.currentTime / frameTime) % totalFrames;

			// Only update if frame changed
			if (newFrame != atlasInfo.currentFrame)
			{
				atlasInfo.currentFrame = newFrame;
				this->updateTextureAtlasFrame(materialName, atlasInfo);
			}
		}
	}

	void ParticleFxModule::updateTextureAtlasFrame(const Ogre::String& materialName, TextureAtlasInfo& atlasInfo)
	{
		try
		{
			Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingleton().getHlmsManager();
			Ogre::HlmsDatablock* datablock = hlmsManager->getDatablock(materialName);

			if (!datablock || datablock->getCreator()->getType() != Ogre::HLMS_UNLIT)
				return;

			Ogre::HlmsUnlitDatablock* unlitDatablock = static_cast<Ogre::HlmsUnlitDatablock*>(datablock);

			// Calculate UV for current frame (like MyGUI's setImageRect)
			int frameIndex = atlasInfo.currentFrame;
			int row = frameIndex / atlasInfo.columns;  // Which row (0-3 for 4x4)
			int col = frameIndex % atlasInfo.columns;  // Which column (0-3 for 4x4)

			// Calculate UV offset and size for this frame
			Ogre::Real uSize = 1.0f / atlasInfo.columns;  // e.g., 0.25 for 4 columns
			Ogre::Real vSize = 1.0f / atlasInfo.rows;     // e.g., 0.25 for 4 rows

			Ogre::Real uOffset = col * uSize;  // e.g., frame 5 = col 1 = 0.25
			Ogre::Real vOffset = row * vSize;  // e.g., frame 5 = row 1 = 0.25

			// Create UV transform matrix (like MyGUI's IntRect)
			Ogre::Matrix4 uvTransform;

			// Position and scale
			Ogre::Vector3 offset(uOffset, vOffset, 0.0f);
			Ogre::Vector3 scale(uSize, vSize, 1.0f);

			// Build transform matrix
			uvTransform.makeTransform(offset, scale, Ogre::Quaternion::IDENTITY);

			// Apply to material
			unlitDatablock->setEnableAnimationMatrix(0, true);
			unlitDatablock->setAnimationMatrix(0, uvTransform);

		}
		catch (Ogre::Exception& e)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ParticleFxModule] Error updating texture atlas frame: " + e.getFullDescription());
		}
	}


	// ============================================================================
	// Optional - Clear cache method (for runtime material changes)
	// ============================================================================

	void ParticleFxModule::clearBlendingModeCache(void)
	{
		this->materialBlendingModeCache.clear();

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[ParticleFxModule] Blending mode cache cleared");
	}

	void ParticleFxModule::clearBlendingModeCacheForMaterial(const Ogre::String& materialName)
	{
		auto it = this->materialBlendingModeCache.find(materialName);
		if (it != this->materialBlendingModeCache.end())
		{
			this->materialBlendingModeCache.erase(it);

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ParticleFxModule] Cleared blending mode cache for material: " + materialName);
		}
	}

}; // namespace end

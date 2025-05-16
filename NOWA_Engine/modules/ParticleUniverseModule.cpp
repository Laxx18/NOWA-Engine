#include "NOWAPrecompiled.h"
#include "ParticleUniverseModule.h"
#include "ParticleUniverseSystemManager.h"
#include "DeployResourceModule.h"
#include "modules/RenderCommandQueueModule.h"

namespace NOWA
{
	ParticleUniverseModule::ParticleUniverseModule(const Ogre::String& appStateName)
		: appStateName(appStateName),
		particleUniverseSystem(nullptr),
		sceneManager(nullptr)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[ParticleUniverseModule] Module created");
	}

	ParticleUniverseModule::~ParticleUniverseModule()
	{
		
	}

	void ParticleUniverseModule::destroyContent(void)
	{
		//if (this->particleUniverseSystem)
		//{
		//	this->particleUniverseSystem->stop();
		//	// this->particleNode->detachObject(this->particleUniverseSystem);
		//	this->particleSystemManager->destroyParticleSystem(this->particleUniverseSystem, this->sceneManager);
		//	this->particleUniverseSystem = nullptr;
		//}
		ENQUEUE_RENDER_COMMAND_WAIT("ParticleUniverseModule::destroyContent",
		{
			for (auto& it = this->particles.begin(); it != this->particles.end(); ++it)
			{
				ParticleUniverse::ParticleSystem* particle = it->second.particle;
				if (particle)
				{
				
						particle->stop();
						it->second.particleNode->detachObject(particle);
						this->sceneManager->getRootSceneNode()->removeAndDestroyChild(it->second.particleNode);
						ParticleUniverse::ParticleSystemManager::getSingletonPtr()->destroyParticleSystem(particle, this->sceneManager);
					particle = nullptr;
				}
			}
			this->sceneManager = nullptr;
			this->particles.clear();
		});
	}

	void ParticleUniverseModule::init(Ogre::SceneManager* sceneManager)
	{
		this->sceneManager = sceneManager;
	}

	void ParticleUniverseModule::createParticleSystem(const Ogre::String& name, const Ogre::String& templateName,
		Ogre::Real playTimeMS, Ogre::Quaternion orientation, Ogre::Vector3 position, Ogre::Real scale)
	{

		auto it = this->particles.find(name);
		if (it == this->particles.end())
		{
			//Partikeleffekt erstellen und abspielen
			//Achtung: immer im SkriptOrdner nach system "Name" schauen, das ist der Templatename der benutzt werden kann
			//Weil Name da nicht stand, ging kein Partikel!!

			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("ParticleUniverseModule::createParticleSystem", _6(name, templateName, playTimeMS, orientation, position, scale),
				{
					ParticleUniverseData particleUniverseData;
					particleUniverseData.particleTemplateName = templateName;
					particleUniverseData.particlePlayTime = playTimeMS;
					particleUniverseData.particleInitialPlayTime = playTimeMS;
					particleUniverseData.particleOffsetOrientation = orientation;
					particleUniverseData.particleOffsetPosition = position;
					particleUniverseData.particleScale = scale;

					particleUniverseData.particle = ParticleUniverse::ParticleSystemManager::getSingletonPtr()->getParticleSystem(name);
					if (nullptr != particleUniverseData.particle)
					{
						ParticleUniverse::ParticleSystemManager::getSingletonPtr()->destroyParticleSystem(particleUniverseData.particle, this->sceneManager);
					}

					particleUniverseData.particle = ParticleUniverse::ParticleSystemManager::getSingletonPtr()->createParticleSystem(name,
						particleUniverseData.particleTemplateName, this->sceneManager);

					if (!particleUniverseData.particle)
					{
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ParticleUniverseModule] Error: Could not create particle effect: " + name);
						throw Ogre::Exception(Ogre::Exception::ERR_ITEM_NOT_FOUND, "[ParticleUniverseModule] Error: Could not create particle effect: " + name, "NOWA");
					}

					particleUniverseData.particleNode = this->sceneManager->getRootSceneNode()->createChildSceneNode();
					particleUniverseData.particleNode->setName(name + "_Node");

					particleUniverseData.particleNode->setOrientation(particleUniverseData.particleOffsetOrientation);
					particleUniverseData.particleNode->setPosition(particleUniverseData.particleOffsetPosition);
					particleUniverseData.particleNode->attachObject(particleUniverseData.particle);
					particleUniverseData.particle->setDefaultQueryFlags(0 << 0);
					particleUniverseData.particle->setRenderQueueGroup(RENDER_QUEUE_PARTICLE_STUFF);
					particleUniverseData.particle->setCastShadows(false);
					particleUniverseData.particle->setScale(particleUniverseData.particleScale);
					particleUniverseData.particle->setScaleVelocity(particleUniverseData.particleScale.x);
					particleUniverseData.particle->prepare();
					particleUniverseData.particle->start();
					particleUniverseData.particle->stop();

					this->particles.emplace(name, particleUniverseData);
				}
			);
		}
	}

	void ParticleUniverseModule::playParticleSystem(const Ogre::String& name)
	{
		auto it = this->particles.find(name);
		if (it != this->particles.end())
		{
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("ParticleUniverseModule::playParticleSystem", _2(&it, name), {
				// reset the time
				it->second.particlePlayTime = it->second.particleInitialPlayTime;
				it->second.activated = true;
				it->second.particle->prepare();
				it->second.particle->start();
			});
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ParticleUniverseModule] Error: Cannot play particle system, because the particle name: '" + name + "' does not exist.");
		}
	}

	void ParticleUniverseModule::playAndStopFadeParticleSystem(const Ogre::String& name, Ogre::Real stopTimeMS)
	{
		auto it = this->particles.find(name);
		if (it != this->particles.end())
		{
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("ParticleUniverseModule::playAndStopFadeParticleSystem", _3(&it, name, stopTimeMS), {
				// reset the time
				it->second.particlePlayTime = it->second.particleInitialPlayTime;
				it->second.activated = true;
				it->second.particle->prepare();
				it->second.particle->startAndStopFade(stopTimeMS);
			});
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ParticleUniverseModule] Error: Cannot play and stop fade particle system, because the particle name: '" + name + "' does not exist.");
		}
	}

	void ParticleUniverseModule::stopParticleSystem(const Ogre::String& name)
	{
		auto it = this->particles.find(name);
		if (it != this->particles.end())
		{
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("ParticleUniverseModule::stopParticleSystem", _2(&it, name), {
				// do not deactive since it will be deactivated, when the particle playout time is over
				// it->second.activated = false;
				it->second.particle->stop();
			});
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ParticleUniverseModule] Error: Cannot stop particle system, because the particle name: '" + name + "' does not exist.");
		}
	}

	void ParticleUniverseModule::pauseParticleSystem(const Ogre::String& name, Ogre::Real pauseTimeMS)
	{
		auto it = this->particles.find(name);
		if (it != this->particles.end())
		{
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("ParticleUniverseModule::pauseParticleSystem", _3(&it, name, pauseTimeMS), {
				if (0.0f != pauseTimeMS)
				{
					it->second.particle->pause(pauseTimeMS);
				}
				else
				{
					it->second.particle->pause();
				}
			});
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ParticleUniverseModule] Error: Cannot pause particle system, because the particle name: '" + name + "' does not exist.");
		}
	}

	void ParticleUniverseModule::resumeParticleSystem(const Ogre::String& name)
	{
		auto it = this->particles.find(name);
		if (it != this->particles.end())
		{
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("ParticleUniverseModule::resumeParticleSystem", _2(&it, name), {
				it->second.particle->resume();
			});
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ParticleUniverseModule] Error: Cannot resume particle system, because the particle name: '" + name + "' does not exist.");
		}
	}

	void ParticleUniverseModule::update(Ogre::Real dt)
	{
		if (true == this->particles.empty())
			return;

		for (auto& particleUniverseData : this->particles)
		{
			// only play activated particle effects
			if (true == particleUniverseData.second.activated)
			{
				// decrement the particleUniverseData.second.particlePlayTime
				Ogre::Real& playTime = particleUniverseData.second.particlePlayTime;
				// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "playTime: " + Ogre::StringConverter::toString(playTime));
				// play particle effect forever or when user stops manually
				if (0.0f == particleUniverseData.second.particleInitialPlayTime)
				{
					return;
				}
				// control the execution time
				if (playTime > 0.0f)
				{
					playTime -= dt * 1000.0f;
				}
				else
				{
					ENQUEUE_RENDER_COMMAND_MULTI_WAIT("ParticleUniverseModule::update", _1(particleUniverseData), {
						particleUniverseData.second.particle->stop();
					});
					// set activated to false, so that the particle can be activated at a later time
					particleUniverseData.second.activated = false;
				}
			}
		}
	}

	ParticleUniverseData* ParticleUniverseModule::getParticle(const Ogre::String& name)
	{
		auto it = this->particles.find(name);
		if (it != this->particles.end())
		{
			return &it->second;
		}
		return nullptr;
	}

	void ParticleUniverseModule::removeParticle(const Ogre::String& name)
	{
		auto it = this->particles.find(name);
		if (it != this->particles.end())
		{
			ParticleUniverse::ParticleSystem* particle = it->second.particle;

			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("ParticleUniverseModule::removeParticle", _3(&it, name, particle), {
				DeployResourceModule::getInstance()->removeResource(it->second.particle->getName());
				particle->stop();
				if (it->second.particleNode->numAttachedObjects() > 0 &&
					it->second.particleNode->getAttachedObject(particle->getName()) == particle)
				{
					it->second.particleNode->detachObject(particle);
				}
				this->sceneManager->getRootSceneNode()->removeAndDestroyChild(it->second.particleNode);
				ParticleUniverse::ParticleSystemManager::getSingletonPtr()->destroyParticleSystem(particle, this->sceneManager);
			});
			particle = nullptr;
			this->particles.erase(it);
		}
	}

}; // namespace end

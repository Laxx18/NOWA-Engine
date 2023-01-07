#include "NOWAPrecompiled.h"
#include "OgreALModule.h"
#include "main/Events.h"
#include "main/AppStateManager.h"
#include "DeployResourceModule.h"

namespace NOWA
{
	OgreALModule::OgreALModule()
		: soundManager(nullptr), // at this time, only one sound manager instance is supported
		soundVolume(100),
		musicVolume(100),
		sceneManager(nullptr)
	{

	}

	OgreALModule::~OgreALModule()
	{

	}

	OgreALModule* OgreALModule::getInstance()
	{
		static OgreALModule instance;

		return &instance;
	}

	void OgreALModule::init(Ogre::SceneManager* sceneManager)
	{
		// Do not change scene manager if sounds shall continue
		if (true == this->bContinue)
			return;

		if (this->soundManager == nullptr)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[OgreALModule] Module created");
			Ogre::LogManager::getSingletonPtr()->logMessage("*** Initializing OgreAL ***");
			this->sceneManager = sceneManager;
			this->soundManager = new OgreAL::SoundManager(sceneManager);
			Ogre::LogManager::getSingletonPtr()->logMessage("*** Finished: Initializing OgreAL ***");
		}
		else
		{
			this->sceneManager = sceneManager;
			this->soundManager->init(sceneManager);
		}
	}

	void OgreALModule::destroySounds(Ogre::SceneManager* sceneManager)
	{
		// Do not destroy sounds if sounds shall continue, when switching back to the origin AppState
		if (true == this->bContinue)
			return;

		if (this->sceneManager != sceneManager)
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[OgreALModule] Error: Destroy sounds with a different unknown scene manager is not allowed! Maybe you forgot to call @setContinue(false).");
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[OgreALModule] Error: Destroy sounds with a different unknown scene manager is not allowed! Maybe you forgot to call @setContinue(false).\n", "NOWA");
		}

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[OgreALModule] OgreAL module destroyed");
		if (this->soundManager)
		{
			this->soundManager->destroyAllSounds(sceneManager);
		}
	}

	void OgreALModule::destroyContent(void)
	{
		this->sceneManager = nullptr;
		if (this->soundManager)
		{
			delete this->soundManager;
			this->soundManager = 0;
		}
	}

	void OgreALModule::setContinue(bool bContinue)
	{
		this->bContinue = bContinue;
	}

	bool OgreALModule::getIsContinued(void) const
	{
		return this->bContinue;
	}

	void OgreALModule::setupVolumes(int soundVolume, int musicVolume)
	{
		this->soundVolume = soundVolume;
		this->musicVolume = musicVolume;
	}

	void OgreALModule::deleteSound(Ogre::SceneManager* sceneManager, OgreAL::Sound*& sound)
	{
		// attention sound is of type *&, because the sound is not deleted here, but in the soundManager
		// so without the &, setting the sound = nullptr had set this sound to 0, but not the outside sound variable
		// for which this function had been called, so when this function had been called twice, an error occured, because
		//// the outside variable was not 0, but was not valid too
		try
		{
			if (sound)
			{
				if (this->soundManager->hasSound(sceneManager, sound->getName()))
				{
					DeployResourceModule::getInstance()->removeResource(sound->getName());
					//stop sound
					sound->stop();
					// release soundsource (Attention: the source must not be deleted, else there are crashes after a while
					this->soundManager->_releaseSource(sound);
					this->soundManager->destroySound(sceneManager, sound);
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[OgreALModule] SoundObject: " + sound->getFileName() + " destroyed.");
				}
				else
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[OgreALModule] The soundname: " + sound->getFileName() + "has been deleted already!");
				}
				sound = nullptr;
			}
			else
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[OgreALModule] The commited pointer to the sound is already null");
			}
		}
		catch (...)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[OgreALModule] Something has messed up with the sound pointer in the 'deleteSound(...)' function.");
		}
	}

	OgreAL::Sound* OgreALModule::createSound(Ogre::SceneManager* sceneManager, const Ogre::String& name, const Ogre::String& resourceName, bool loop, bool stream)
	{
		if (this->soundManager)
		{
			if (!this->soundManager->hasSound(sceneManager, name))
			{
				OgreAL::Sound* sound = nullptr;
				try
				{
					sound = this->soundManager->createSound(sceneManager, name, resourceName, loop, stream);
				}
				catch (const Ogre::Exception& exception)
				{
					Ogre::String message = "[OgreALModule] Could not create sound : " + name + " description : " + exception.getDescription();
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, message);
					boost::shared_ptr<EventDataFeedback> eventDataNavigationMeshFeedback(new EventDataFeedback(false, message));
					NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataNavigationMeshFeedback);
					return nullptr;
				}
				// setup volume
				sound->setGain(Ogre::Real(this->soundVolume) / 100.0f);
				DeployResourceModule::getInstance()->tagResource(sound->getFileName(), "Audio");
				// fadein causes non correct behavior, because the volume is at its maximum after the operation
				//pSound->fadeIn(pSound->getGain());
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[OgreALModule] Sound: " + name + " created.");
				return sound;
			}
			else
			{
				return this->getSound(sceneManager, name);
			}
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[OgreALModule] The SoundManager does not exist. Have you forgotten to create one? Call ' NOWA::Core::getSingletonPtr()->createOgreAL(void)' first.");
			return nullptr;
		}
	}

	void OgreALModule::setSoundDopplerEffect(Ogre::Real value)
	{
		assert((this->soundManager != nullptr) && "[OgreALModule::setSoundDopplerEffect] SoundManager is already NULL");
		this->soundManager->setDopplerFactor(value);
	}

	void OgreALModule::setSoundSpeed(Ogre::Real speed)
	{
		assert((this->soundManager != nullptr) && "[OgreALModule::setSoundSpeed] SoundManager is already NULL");
		this->soundManager->setSpeedOfSound(speed);
	}

	void OgreALModule::setSoundCullDistance(Ogre::SceneManager* sceneManager, Ogre::Real distance)
	{
		assert((this->soundManager != nullptr) && "[OgreALModule::setSoundCullDistance] SoundManager is already NULL");
		this->soundManager->setCullDistance(sceneManager, distance);
	}

	OgreAL::SoundManager* OgreALModule::getSoundManager(void) const
	{
		assert((this->soundManager != nullptr) && "[OgreALModule::getSoundManager] SoundManager is already NULL");
		return this->soundManager;
	}

	OgreAL::Sound* OgreALModule::getSound(Ogre::SceneManager* sceneManager, const Ogre::String& soundName)
	{
		assert((this->soundManager != nullptr) && "[OgreALModule::getSound] SoundManager is already NULL");
		if (this->soundManager->hasSound(sceneManager, soundName))
		{
			return this->soundManager->getSound(sceneManager, soundName);
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[OgreALModule] There is no such sound");
			return nullptr;
		}
	}

	int OgreALModule::getSoundVolume(void) const
	{
		return this->soundVolume;
	}

	int OgreALModule::getMusicVolume(void) const
	{
		return this->musicVolume;
	}

} // namespace end
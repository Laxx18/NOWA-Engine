#include "NOWAPrecompiled.h"
#include "OgreALModule.h"
#include "main/Events.h"
#include "main/AppStateManager.h"
#include "DeployResourceModule.h"
#include "modules/GraphicsModule.h"

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

	// TODO: Which one to use?
#if 0
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
#else
	void OgreALModule::init(Ogre::SceneManager* sceneManager)
	{
		// Do not change scene manager if sounds shall continue
		if (true == this->bContinue)
			return;

		// Queue initialization if soundManager hasn't been created
		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("OgreALModule::init", _1(sceneManager),
		{
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
		});
	}
#endif

	void OgreALModule::destroySounds(Ogre::SceneManager* sceneManager)
	{
		// Do not destroy sounds if sounds shall continue, when switching back to the origin AppState
		if (this->bContinue)
		{
			return;
		}

		if (this->sceneManager != sceneManager)
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[OgreALModule] Error: Destroy sounds with a different unknown scene manager is not allowed! Maybe you forgot to call @setContinue(false).");
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[OgreALModule] Error: Destroy sounds with a different unknown scene manager is not allowed! Maybe you forgot to call @setContinue(false).\n", "NOWA");
		}

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[OgreALModule] OgreAL module destroyed");

		auto soundManager = this->soundManager;

		if (soundManager)
		{
			ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("OgreALModule::destroySounds", _2(soundManager, sceneManager),
			{
				soundManager->destroyAllSounds(sceneManager);
			});
		}
	}

	void OgreALModule::destroyContent(void)
	{
		/*this->sceneManager = nullptr;
		if (this->soundManager)
		{
			delete this->soundManager;
			this->soundManager = 0;
		}*/

		if (nullptr != this->soundManager)
		{
			auto soundManager = this->soundManager;
			this->soundManager = nullptr;

			ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("OgreALModule::destroyContent", _1(soundManager),
			{
				delete soundManager;
			});
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

#if 0
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
#endif

	
#if 0
	void OgreALModule::deleteSound(Ogre::SceneManager* sceneManager, OgreAL::Sound*& sound)
	{
		RenderCommandQueueModule::RenderCommand renderCommand = [this, sceneManager, soundPtr = &sound]() {
			// Ensure sound is valid before attempting to delete
			try
			{
				if (*soundPtr)
				{
					if (this->soundManager->hasSound(sceneManager, (*soundPtr)->getName()))
					{
						// Remove resource
						DeployResourceModule::getInstance()->removeResource((*soundPtr)->getName());

						// Stop sound
						(*soundPtr)->stop();

						// Release sound source (without deleting to prevent crashes)
						this->soundManager->_releaseSource(*soundPtr);

						// Destroy the sound
						this->soundManager->destroySound(sceneManager, *soundPtr);
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[OgreALModule] SoundObject: " + (*soundPtr)->getFileName() + " destroyed.");
					}
					else
					{
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[OgreALModule] The soundname: " + (*soundPtr)->getFileName() + " has already been deleted!");
					}
					// Set the pointer to null after deletion
					*soundPtr = nullptr;
				}
				else
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[OgreALModule] The committed pointer to the sound is already null");
				}
			}
			catch (...)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[OgreALModule] Something has messed up with the sound pointer in the 'deleteSound(...)' function.");
			}
			};

		// Enqueue the render command to delete the sound
		RenderCommandQueueModule::getInstance()->enqueue(renderCommand, "OgreALModule::deleteSound");
	}
#else
void OgreALModule::deleteSound(Ogre::SceneManager* sceneManager, OgreAL::Sound*& sound)
	{
		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("OgreALModule::deleteSound", _2(sceneManager, soundPtr = &sound),
		{
			// Ensure sound is valid before attempting to delete
			try
			{
				if (*soundPtr)
				{
					if (this->soundManager->hasSound(sceneManager, (*soundPtr)->getName()))
					{
						// Remove resource
						DeployResourceModule::getInstance()->removeResource((*soundPtr)->getName());

						// Stop sound
						(*soundPtr)->stop();

						// Release sound source (without deleting to prevent crashes)
						this->soundManager->_releaseSource(*soundPtr);

						// Destroy the sound
						this->soundManager->destroySound(sceneManager, *soundPtr);
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[OgreALModule] SoundObject: " + (*soundPtr)->getFileName() + " destroyed.");
					}
					else
					{
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[OgreALModule] The soundname: " + (*soundPtr)->getFileName() + " has already been deleted!");
					}
					// Set the pointer to null after deletion
					*soundPtr = nullptr;
				}
				else
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[OgreALModule] The committed pointer to the sound is already null");
				}
			}
			catch (...)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[OgreALModule] Something has messed up with the sound pointer in the 'deleteSound(...)' function.");
			}
		});
	}

#endif

#if 0
	void OgreALModule::createSound(Ogre::SceneManager* sceneManager, const Ogre::String& name, const Ogre::String& resourceName, bool loop, bool stream, SoundCreationCallback callback)
	{
		// First create the lambda and store it in a variable
		RenderCommandQueueModule::RenderCommand renderCommand = [this, sceneManager, name, resourceName, loop, stream, callback]() {
			OgreAL::Sound* sound = nullptr;
			if (this->soundManager)
			{
				if (!this->soundManager->hasSound(sceneManager, name))
				{
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
					}
					// Setup volume
					if (sound)
					{
						sound->setGain(Ogre::Real(this->soundVolume) / 100.0f);
						DeployResourceModule::getInstance()->tagResource(sound->getFileName(), "Audio");
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[OgreALModule] Sound: " + name + " created.");
					}
				}
				else
				{
					sound = this->getSound(sceneManager, name);
				}
			}
			else
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[OgreALModule] The SoundManager does not exist.");
			}
			// Execute the callback, passing the sound (could be nullptr if creation failed)
			if (callback)
			{
				callback(sound);
			}
		};

		// Then pass the variable to enqueue
		RenderCommandQueueModule::getInstance()->enqueue(renderCommand);
	}

#endif 

#if 1
	OgreAL::Sound* OgreALModule::createSound(Ogre::SceneManager* sceneManager, const Ogre::String& name, const Ogre::String& resourceName, bool loop, bool stream)
	{
		if (!this->soundManager)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[OgreALModule] The SoundManager does not exist. Have you forgotten to create one? Call ' NOWA::Core::getSingletonPtr()->createOgreAL(void)' first.");
			return nullptr;
		}

		if (this->soundManager->hasSound(sceneManager, name))
		{
			return this->getSound(sceneManager, name);
		}

		OgreAL::Sound* sound = nullptr;

		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("OgreALModule::createSound", _6(sceneManager, name, resourceName, loop, stream, &sound),
		{
			try
			{
				sound = this->soundManager->createSound(sceneManager, name, resourceName, loop, stream);

				// Set volume
				sound->setGain(Ogre::Real(this->soundVolume) / 100.0f);

				// Tag resource for cleanup
				DeployResourceModule::getInstance()->tagResource(sound->getFileName(), "Audio");

				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[OgreALModule] Sound: " + name + " created.");
			}
			catch (const Ogre::Exception& exception)
			{
				Ogre::String message = "[OgreALModule] Could not create sound : " + name + " description : " + exception.getDescription();
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, message);
				boost::shared_ptr<EventDataFeedback> eventDataNavigationMeshFeedback(new EventDataFeedback(false, message));
				NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataNavigationMeshFeedback);
			}
		});  // `sound` will be set after the render thread completes

		return sound;  // This will now return after the promise is resolved
	}
#endif

	void OgreALModule::setSoundDopplerEffect(Ogre::Real value)
	{
		assert((this->soundManager != nullptr) && "[OgreALModule::setSoundDopplerEffect] SoundManager is already NULL");

		// Queue the call to ensure thread safety, if needed
		// ENQUEUE_RENDER_COMMAND_MULTI("OgreALModule::setSoundDopplerEffect", _1(value),
		// {
			this->soundManager->setDopplerFactor(value);
		// });
	}

	void OgreALModule::setSoundSpeed(Ogre::Real speed)
	{
		assert((this->soundManager != nullptr) && "[OgreALModule::setSoundSpeed] SoundManager is already NULL");

		// Queue the call if it interacts with GPU resources
		// ENQUEUE_RENDER_COMMAND_MULTI("OgreALModule::setSoundSpeed", _1(speed),
		// {
			this->soundManager->setSpeedOfSound(speed);
		// });
	}

	void OgreALModule::setSoundCullDistance(Ogre::SceneManager* sceneManager, Ogre::Real distance)
	{
		assert((this->soundManager != nullptr) && "[OgreALModule::setSoundCullDistance] SoundManager is already NULL");

		// Queue if it involves render resources or scene manager interaction
		// ENQUEUE_RENDER_COMMAND_MULTI("OgreALModule::setSoundCullDistance", _2(sceneManager, distance),
		// {
			this->soundManager->setCullDistance(sceneManager, distance);
		// });
	}

	OgreAL::SoundManager* OgreALModule::getSoundManager(void) const
	{
		assert((this->soundManager != nullptr) && "[OgreALModule::getSoundManager] SoundManager is already NULL");
		return this->soundManager;
	}

	OgreAL::Sound* OgreALModule::getSound(Ogre::SceneManager* sceneManager, const Ogre::String& soundName)
	{
		// TODO: Queue?
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
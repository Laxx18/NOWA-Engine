/*---------------------------------------------------------------------------*\
** This source file is part of OgreAL                                        **
** an OpenAL plugin for the Ogre Rendering Engine.                           **
**                                                                           **
** Copyright 2006 Casey Borders                                              **
**                                                                           **
** OgreAL is free software; you can redistribute it and/or modify it under   **
** the terms of the GNU Lesser General Public License as published by the    **
** Free Software Foundation; either version 2, or (at your option) any later **
** version.                                                                  **
**                                                                           **
** The developer really likes screenshots and while he recognises that the   **
** fact that this is an AUDIO plugin means that the fruits of his labor will **
** never been seen in these images he would like to kindly ask that you send **
** screenshots of your application using his library to                      **
** screenshots@mooproductions.org                                            **
**                                                                           **
** Please bear in mind that the sending of these screenshots means that you  **
** are agreeing to allow the developer to display them in the media of his   **
** choice.  They will, however, be fully credited to the person sending the  **
** email or, if you wish them to be credited differently, please state that  **
** in the body of the email.                                                 **
**                                                                           **
** OgreAL is distributed in the hope that it will be useful, but WITHOUT     **
** ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or     **
** FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for **
** more details.                                                             **
**                                                                           **
** You should have received a copy of the GNU General Public License along   **
** with OgreAL; see the file LICENSE.  If not, write to the                  **
** Free Software Foundation, Inc.,                                           **
** 59 Temple Place - Suite 330,                                              **
** Boston, MA 02111-1307, USA.                                               **
\*---------------------------------------------------------------------------*/

#include "OgreALException.h"
#include "OgreALSoundManager.h"
#include <sstream>

#include <algorithm>
#include <execution>

template<> OgreAL::SoundManager* Ogre::Singleton<OgreAL::SoundManager>::msSingleton = 0;

#if OGREAL_THREADED
	boost::thread *OgreAL::SoundManager::mOgreALThread = 0;
	bool OgreAL::SoundManager::mShuttingDown = false;
#endif

namespace OgreAL {
	const Ogre::String SoundManager::SOUND_FILE = "SoundFile";
	const Ogre::String SoundManager::LOOP_STATE = "LoopState";
	const Ogre::String SoundManager::STREAM = "Stream";

	const ALenum SoundManager::xRamAuto = alGetEnumValue("AL_STORAGE_AUTO");
	const ALenum SoundManager::xRamHardware = alGetEnumValue("AL_STORAGE_HARDWARE");
	const ALenum SoundManager::xRamAccessible = alGetEnumValue("AL_STORAGE_ACCESSIBLE");

  SoundManager::SoundManager(Ogre::SceneManager* sceneMgr, const Ogre::String& deviceName, int maxNumSources) : 
		mEAXSupport(false),
		mEAXVersion(0),
		mXRAMSupport(false),
		mEFXSupport(false),
		mSendsPerSource(10),
		mContext(0),
		mDevice(0),
		mDopplerFactor(1.0),
		mSpeedOfSound(343.3),
		mCullDistance(-1.0),
		mMaxNumSources(maxNumSources),
		mMajorVersion(0),
		mMinorVersion(0),
		mLastDeltaTime(0.0),
        mRenderQueueId(0)
	{
		Ogre::LogManager::getSingleton().logMessage("*-*-* OgreAL Initialization");

		mSoundContainer.emplace(sceneMgr, std::map<Ogre::String, Sound*>());

		// Create and register Sound and Listener Factories
		mSoundFactory = new SoundFactory();
		mListenerFactory = new ListenerFactory();
		
		Ogre::Root::getSingleton().addMovableObjectFactory(mSoundFactory);
		Ogre::Root::getSingleton().addMovableObjectFactory(mListenerFactory);

		Ogre::LogManager::getSingleton().logMessage("*-*-* Creating OpenAL");

		initializeDevice(deviceName);

		checkFeatureSupport();

		createListener(sceneMgr);

		#if OGREAL_THREADED
			// Kick off the update thread
			Ogre::LogManager::getSingleton().logMessage("Creating OgreAL Thread");
			mOgreALThread = new boost::thread(boost::function0<void>(&SoundManager::threadUpdate));
		#else
			// Register for frame events
			Ogre::Root::getSingleton().addFrameListener(this);
		#endif
	}

	SoundManager::~SoundManager()
	{
		Ogre::LogManager::getSingleton().logMessage("*-*-* OgreAL Shutdown");

		#if OGREAL_THREADED
			// Clean up the threading
			Ogre::LogManager::getSingleton().logMessage("Shutting Down OgreAL Thread");
			mShuttingDown = true;
			mOgreALThread->join();
			delete mOgreALThread;
			mOgreALThread = 0;
		#else
			// Unregister for frame events
			Ogre::Root::getSingleton().removeFrameListener(this);
		#endif

		// delete all Sound pointers in the SoundMap
		// destroyAllSounds();

		// Destroy the Listener and all Sounds
		Listener *listener = Listener::getSingletonPtr();
		if (nullptr != listener)
		{
			delete listener;
			listener = nullptr;
		}

		// Clean out mActiveSounds and mQueuedSounds
		mActiveSounds.clear();
		mQueuedSounds.clear();

		// Clean up the SourcePool
		while(!mSourcePool.empty())
		{
			alDeleteSources(1, &mSourcePool.front());
			CheckError(alGetError(), "Failed to destroy source");
			mSourcePool.pop();
		}

		// delete all FormatData pointers in the FormatMap;
		std::for_each(mSupportedFormats.begin(), mSupportedFormats.end(), DeleteSecond());
		mSupportedFormats.clear();

		// Unregister the Sound and Listener Factories
		Ogre::Root::getSingleton().removeMovableObjectFactory(mSoundFactory);
		Ogre::Root::getSingleton().removeMovableObjectFactory(mListenerFactory);
		delete mListenerFactory;
		delete mSoundFactory;

		Ogre::LogManager::getSingleton().logMessage("*-*-* Releasing OpenAL");

		// Release the OpenAL Context and the Audio device
		alcMakeContextCurrent(NULL);
		alcDestroyContext(mContext);
		alcCloseDevice(mDevice);
	}

	SoundManager* SoundManager::getSingletonPtr(void)
	{
		return msSingleton;
	}

	SoundManager& SoundManager::getSingleton(void)
	{  
		assert( msSingleton );  return (*msSingleton);  
	}

	void SoundManager::init(Ogre::SceneManager* sceneMgr)
	{
		mSoundContainer.emplace(sceneMgr, std::map<Ogre::String, Sound*>());

		createListener(sceneMgr);
	}

	Sound* SoundManager::createSound(Ogre::SceneManager* sceneMgr, const Ogre::String& name, 
		const Ogre::String& fileName, bool loop, bool stream)
	{
		// Lock Mutex
		OGREAL_LOCK_AUTO_MUTEX

		bool soundExists = this->hasSound(sceneMgr, name);
		CheckCondition(!soundExists, 13, "A Sound with name '" + name + "' already exists.");

		Ogre::NameValuePairList fileTypePair;
		fileTypePair[SOUND_FILE] = fileName;
		fileTypePair[LOOP_STATE] = Ogre::StringConverter::toString(loop);
		fileTypePair[STREAM] = Ogre::StringConverter::toString(stream);
        fileTypePair["name"]=name;
		
		Sound *newSound = static_cast<Sound*>(mSoundFactory->createInstance( 
                Ogre::Id::generateNewId<Ogre::MovableObject>(), 
                &sceneMgr->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC), 
                sceneMgr, &fileTypePair));

		// mSoundMap[name] = newSound;

		const auto& it = mSoundContainer.find(sceneMgr);
		if (it != mSoundContainer.cend())
		{
			it->second.insert(std::make_pair(name, newSound));
		}

		return newSound;
	}

	Sound* SoundManager::getSound(Ogre::SceneManager* sceneMgr, const Ogre::String& name) const
	{
		// Lock Mutex
		OGREAL_LOCK_AUTO_MUTEX

		SoundContainer::const_iterator soundItr = mSoundContainer.find(sceneMgr);
		if(soundItr == mSoundContainer.cend())
		{
			throw Ogre::Exception(Ogre::Exception::ERR_ITEM_NOT_FOUND, 
					"SceneManager does not exist.", 
					"SceneManager::getMovableObject");
		}

		auto& subIt = soundItr->second.find(name);
		if(subIt == soundItr->second.cend())
		{
			throw Ogre::Exception(Ogre::Exception::ERR_ITEM_NOT_FOUND, 
					"Object named '" + name + "' does not exist.", 
					"SceneManager::getMovableObject");
		}

		return subIt->second;
	}

	bool SoundManager::hasSound(Ogre::SceneManager* sceneMgr, const Ogre::String& name) const
	{
		// Lock Mutex
		OGREAL_LOCK_AUTO_MUTEX

		SoundContainer::const_iterator soundItr = mSoundContainer.find(sceneMgr);
		if(soundItr != mSoundContainer.cend())
		{
			auto& subIt = soundItr->second.find(name);
			if(subIt != soundItr->second.cend())
			{
				return true;
			}
		}
		return false;
	}

    SoundContainerIterPair SoundManager::getSoundIterator() {
        return SoundContainerIterPair(mSoundContainer.begin(), mSoundContainer.end());
    }

	void SoundManager::destroySound(Ogre::SceneManager* sceneMgr, Sound *sound)
	{
		// Lock Mutex
		OGREAL_LOCK_AUTO_MUTEX

		destroySound(sceneMgr, sound->getName());
	}

	void SoundManager::destroySound(Ogre::SceneManager* sceneMgr, const Ogre::String& name)
	{
		// Lock Mutex
		OGREAL_LOCK_AUTO_MUTEX

		const auto& soundItr = mSoundContainer.find(sceneMgr);
		if(soundItr != mSoundContainer.cend())
		{
			const auto& subIt = soundItr->second.find(name);
			if(subIt != soundItr->second.end())
			{		
				mSoundsToDestroy.push_back(subIt->second);
				std::map<Ogre::String, Sound*>& soundMap = soundItr->second;
				soundMap.erase(subIt->first);
			}
		}
	}

	void SoundManager::destroyAllSounds(Ogre::SceneManager* sceneMgr)
	{
		// Lock Mutex
		OGREAL_LOCK_AUTO_MUTEX

		const auto& soundItr = mSoundContainer.find(sceneMgr);
		if(soundItr != mSoundContainer.cend())
		{
			// Delete all Sound pointers in the SoundMap.
			std::map<Ogre::String, Sound*>& soundMap = soundItr->second;
			std::for_each(soundMap.begin(), soundMap.end(), DeleteSecond());
			soundMap.clear();
			mSoundContainer.erase(sceneMgr);
		}

		// Delete all Sounds scheduled for deletion.
		performDeleteQueueCycle();

		// Also flush gain values.
		mSoundGainMap.clear();

		// Destroy the Listener and all Sounds
		Listener *listener = Listener::getSingletonPtr();
		if (nullptr != listener)
		{
			delete listener;
			listener = nullptr;
		}
	}
	
	void SoundManager::pauseAllSounds(Ogre::SceneManager* sceneMgr)
	{
		// Lock Mutex
		OGREAL_LOCK_AUTO_MUTEX

		SoundContainer::const_iterator soundItr = mSoundContainer.find(sceneMgr);
		if(soundItr != mSoundContainer.cend())
		{
			const std::map<Ogre::String, Sound*>& soundMap = soundItr->second;
			for(auto& subIt = soundMap.cbegin(); subIt != soundMap.cend(); ++subIt)
			{
				if(subIt->second->isPlaying())
				{
					subIt->second->pause();
					mPauseResumeAll.push_back(subIt->second);
				}
			}
		}
	}

	void SoundManager::resumeAllSounds()
	{
		// Lock Mutex
		OGREAL_LOCK_AUTO_MUTEX

		SoundList::iterator soundItr;
		for(soundItr = mPauseResumeAll.begin(); soundItr != mPauseResumeAll.end(); ++soundItr)
		{
			(*soundItr)->play();
		}
		mPauseResumeAll.clear();
	} 

	void SoundManager::createListener(Ogre::SceneManager* sceneMgr)
	{
		Listener* listener = Listener::getSingletonPtr();
		if (nullptr != listener)
		{
			delete listener;
			listener = nullptr;
		}

		Ogre::NameValuePairList fileTypePair;
        fileTypePair["name"]="Default_Listener";

		listener = static_cast<Listener*>
			(mListenerFactory->createInstance( 
            Ogre::Id::generateNewId<Ogre::MovableObject>(),
            &sceneMgr->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC), 
            sceneMgr, &fileTypePair));
	}

	Listener* SoundManager::getListener() const
	{
		// Lock Mutex
		OGREAL_LOCK_AUTO_MUTEX

		return Listener::getSingletonPtr();
	}


	struct SoundManager::SortLowToHigh
	{
		bool operator()(const Sound *sound1, const Sound *sound2)const
		{
			// First we see if either sound has stopped and not given up its source
			if(sound1->isStopped() && !sound2->isStopped())
				return true;
			else if(sound2->isStopped() && !sound1->isStopped())
				return false;
			else if(sound1->isStopped() && sound2->isStopped())
				return sound1 < sound2;

			// If they are both playing we'll test priority
			if(sound1->getPriority() < sound2->getPriority())
				return true;
			else if (sound1->getPriority() > sound2->getPriority())
				return false;

			// If they are the same priority we'll test against the
			// distance from the listener
			Ogre::Real distSound1, distSound2;
			Ogre::Vector3 listenerPos = Listener::getSingleton().getDerivedPosition();
			if(sound1->isRelativeToListener())
			{
				distSound1 = sound1->getPosition().length();
			}
			else
			{
				distSound1 = sound1->getDerivedPosition().distance(listenerPos);
			}

			if(sound2->isRelativeToListener())
			{
				distSound2 = sound1->getPosition().length();
			}
			else
			{
				distSound2 = sound2->getDerivedPosition().distance(listenerPos);
			}

			if(distSound1 > distSound2)
				return true;
			else if(distSound1 < distSound2)
				return false;

			// If they are at the same priority and distance from the listener then
			// they are both equally well suited to being sacrificed so we compare
			// their pointers since it really doesn't matter
			return sound1 < sound2;
		}
	};

	struct SoundManager::SortHighToLow
	{
		bool operator()(const Sound *sound1, const Sound *sound2)const
		{
			// First we'll test priority
			if(sound1->getPriority() > sound2->getPriority())
				return true;
			else if (sound1->getPriority() < sound2->getPriority())
				return false;

			// If they are the same priority we'll test against the
			// distance from the listener
			Ogre::Real distSound1, distSound2;
			Ogre::Vector3 listenerPos = Listener::getSingleton().getDerivedPosition();
			if(sound1->isRelativeToListener())
			{
				distSound1 = sound1->getPosition().length();
			}
			else
			{
				distSound1 = sound1->getDerivedPosition().distance(listenerPos);
			}

			if(sound2->isRelativeToListener())
			{
				distSound2 = sound1->getPosition().length();
			}
			else
			{
				distSound2 = sound2->getDerivedPosition().distance(listenerPos);
			}

			if(distSound1 < distSound2)
				return true;
			else if(distSound1 > distSound2)
				return false;

			// If they are at the same priority and distance from the listener then
			// they are both equally well suited to stealing a source so we compare
			// their pointers since it really doesn't matter
			return sound1 < sound2;
		}
	};

	bool SoundManager::frameStarted(const Ogre::FrameEvent& evt)
	{
		// Do this before any fading gets updated
		mLastDeltaTime = evt.timeSinceLastFrame;
		
		// If there are sounds to delete, do so now.
		if(mSoundsToDestroy.size() > 0) {
		  performDeleteQueueCycle();
		}
		
		updateSounds();
		return true;
	}

	void SoundManager::setDopplerFactor(Ogre::Real dopplerFactor)
	{
		// Negative values are invalid
		if(dopplerFactor < 0) return;

		// Lock Mutex
		OGREAL_LOCK_AUTO_MUTEX

		mDopplerFactor = dopplerFactor;
		alDopplerFactor(mDopplerFactor);
		CheckError(alGetError(), "Failed to set Doppler Factor");
	}

	void SoundManager::setSpeedOfSound(Ogre::Real speedOfSound)
	{
		// Negative values are invalid
		if (speedOfSound < 0) return;

		// Lock Mutex
		OGREAL_LOCK_AUTO_MUTEX

			mSpeedOfSound = speedOfSound;
		alSpeedOfSound(mSpeedOfSound);
		CheckError(alGetError(), "Failed to set Speed of Sound");
	}

	Ogre::Real SoundManager::getCullDistance() const
	{
		return mCullDistance;
	}

	void SoundManager::setCullDistance(Ogre::SceneManager* sceneMgr, Ogre::Real distance)
	{
		// Assign new cull distance.
		mCullDistance = distance;

		SoundContainer::const_iterator soundItr = mSoundContainer.find(sceneMgr);
		if (soundItr != mSoundContainer.cend())
		{
			const std::map<Ogre::String, Sound*>& soundMap = soundItr->second;
			// Go over all sounds.
			for (auto& subIt = soundMap.cbegin(); subIt != soundMap.cend(); ++subIt)
			{
				// Perform a cull cycle on the sound, redoing all culls based on the 
				// newly set cull distance.
				performSoundCull(subIt->second);
			}
		}
	}

	Ogre::StringVector SoundManager::getDeviceList()
	{
		const ALCchar* deviceList = alcGetString(NULL, ALC_DEVICE_SPECIFIER);

		Ogre::StringVector deviceVector;
		/*
		** The list returned by the call to alcGetString has the names of the
		** devices seperated by NULL characters and the list is terminated by
		** two NULL characters, so we can cast the list into a string and it
		** will automatically stop at the first NULL that it sees, then we
		** can move the pointer ahead by the lenght of that string + 1 and we
		** will be at the begining of the next string.  Once we hit an empty 
		** string we know that we've found the double NULL that terminates the
		** list and we can stop there.
		*/
		while(*deviceList != '\0')
		{
			try
			{
				ALCdevice *device = alcOpenDevice(deviceList);

				if(device)
				{
				  CheckError(alcGetError(device), "Unable to open device");

					// Device seems to be valid
					ALCcontext *context = alcCreateContext(device, NULL);
					CheckError(alcGetError(device), "Unable to create context");
					if(context)
					{
						// Context seems to be valid
						alcMakeContextCurrent(context);
						CheckError(alcGetError(device), "Unable to make context current");
						deviceVector.push_back(alcGetString(device, ALC_DEVICE_SPECIFIER));
						alcMakeContextCurrent(NULL);
						CheckError(alcGetError(device), "Unable to clear current context");
						alcDestroyContext(context);
						CheckError(alcGetError(device), "Unable to destroy context");
					}
					alcCloseDevice(device);
				}else{
				  // There is a chance that because the device could not be
				  // opened, the error flag was set, clear it.
				  alcGetError(device);
				}
			}
			catch(...)
			{
				// Don't die here, we'll just skip this device.
			}

			deviceList += strlen(deviceList) + 1;
		}

		return deviceVector;
	}

	FormatMapIterator SoundManager::getSupportedFormatIterator()
	{
		// Lock Mutex
		OGREAL_LOCK_AUTO_MUTEX

		return FormatMapIterator(mSupportedFormats.begin(), mSupportedFormats.end());
	}

	const FormatData* SoundManager::retrieveFormatData(AudioFormat format) const
	{
		// Lock Mutex
		OGREAL_LOCK_AUTO_MUTEX

		FormatMap::const_iterator itr = mSupportedFormats.find(format);
		if(itr != mSupportedFormats.end())
			return itr->second;
		else
			return 0;
	}

	//ALboolean SoundManager::eaxSetBufferMode(Size numBuffers, BufferRef *buffers, EAXMode bufferMode)
	//{
	//	// Lock Mutex
	//	OGREAL_LOCK_AUTO_MUTEX

	//	Ogre::LogManager::getSingleton().logMessage(" === Setting X-RAM Buffer Mode");
	//	if(bufferMode == xRamHardware)
	//	{
	//		ALint size;
	//		alGetBufferi(*buffers, AL_SIZE, &size);
	//		// If the buffer won't fit in xram, return false
	//		if(mXRamSize < (mXRamFree + size)) return AL_FALSE;

	//		Ogre::LogManager::getSingleton().logMessage(" === Allocating " + Ogre::StringConverter::toString(size) +
	//			" bytes of X-RAM");
	//	}
	//	// Try setting the buffer mode, if it fails return false
	//	if(mSetXRamMode(numBuffers, buffers, bufferMode) == AL_FALSE) return AL_FALSE;

	//	mXRamFree = alGetEnumValue("AL_EAX_RAM_FREE");
	//	return AL_TRUE;
	//}
	//
	//ALenum SoundManager::eaxGetBufferMode(BufferRef buffer, ALint *reserved)
	//{
	//	// Lock Mutex
	//	OGREAL_LOCK_AUTO_MUTEX

	//	return mGetXRamMode(buffer, reserved);
	//}

	void SoundManager::_removeBufferRef(const Ogre::String& bufferName)
	{
		// Lock Mutex
		OGREAL_LOCK_AUTO_MUTEX

		mSoundFactory->_removeBufferRef(bufferName);
	}

	void SoundManager::_addBufferRef(const Ogre::String& bufferName, BufferRef buffer)
	{
		// Lock Mutex
		OGREAL_LOCK_AUTO_MUTEX

		mSoundFactory->_addBufferRef(bufferName, buffer);
	}

	SourceRef SoundManager::_requestSource(Sound *sound)
	{
		// Lock Mutex
		OGREAL_LOCK_AUTO_MUTEX

		if(sound->getSource() != AL_NONE)
		{
			// This sound already has a source, so we'll just return the same one
			return sound->getSource();
		}

		SoundList::iterator soundItr;
		for(soundItr = mQueuedSounds.begin(); soundItr != mQueuedSounds.end(); ++soundItr)
		{
			if((*soundItr) == sound)
			{
				// This sound has already requested a source
				return AL_NONE;
				break;
			}
		}

		if(!mSourcePool.empty())
		{
			mActiveSounds.push_back(sound);
			SourceRef source = mSourcePool.front();
			mSourcePool.pop();
			return source;
		}
		// Beefed up check. Wasn't safe before.
		else if(!mActiveSounds.empty())
		{
			Sound *activeSound = mActiveSounds.front();
			Ogre::Vector3 listenerPos = Listener::getSingleton().getDerivedPosition();
			Ogre::Real distSound = sound->getDerivedPosition().distance(listenerPos);
			Ogre::Real distActiveSound = activeSound->getDerivedPosition().distance(listenerPos);

			if(sound->getPriority() > activeSound->getPriority() ||
				sound->getPriority() == activeSound->getPriority() && distSound < distActiveSound)
			{
				activeSound->pause();
				SourceRef source = activeSound->getSource();
				mActiveSounds.erase(mActiveSounds.begin());
				mQueuedSounds.push_back(activeSound);

				mActiveSounds.push_back(sound);
				return source;
			}
			else
			{
				mQueuedSounds.push_back(sound);
				return AL_NONE;
			}
		}

		// No sources in the source pool, no active sounds to steal from.
		return AL_NONE;
	}

	SourceRef SoundManager::_releaseSource(Sound *sound)
	{
		// Lock Mutex
		OGREAL_LOCK_AUTO_MUTEX

		bool soundFound = false;
		SoundList::iterator soundItr;
		// Look for the sound in the active sounds list.
		for(soundItr = mActiveSounds.begin(); soundItr != mActiveSounds.end(); ++soundItr) {
		  // If we've found the sound.
		  if((*soundItr) == sound) {
		    // Erase and flag as found.
		    mActiveSounds.erase(soundItr);
		    soundFound = true;
		    break;
		  }
		}

		// If it's not in the active list, check the queued list.
		if(!soundFound)	{
		  for(soundItr = mQueuedSounds.begin(); soundItr != mQueuedSounds.end(); ++soundItr) {
		    if((*soundItr) == sound) {
		      mQueuedSounds.erase(soundItr);
		      break;
		    }
		  }
		}

		// At this stage we're going to assume that we have found the sound in either
		// the active or queued sounds list and that we've removed it. If we've removed
		// it and it has a valid source, then this source can be assigned to the first
		// sound in line for a source (a sound in the queued sounds list).
		SourceRef source = sound->getSource();
		// If the sound released had a valid source.
		if(source != AL_NONE) {
		  // Unbind the buffer from the source. This also decrements the reference count, 
		  // which needs to be done before we try to release the buffer. In any case we
		  // want the next sound to have a clean buffer.
		  alSourcei(source, AL_BUFFER, 0);
		  CheckError(alGetError(), "Failed to unbind buffer from source.");

		  // If there are queued sounds.
		  if(!mQueuedSounds.empty()) {
		    // Give the first queued sound the source.
		    Sound *queuedSound = mQueuedSounds.front();
		    mQueuedSounds.erase(mQueuedSounds.begin());
		    
		    queuedSound->setSource(source);
		    queuedSound->play();
		    mActiveSounds.push_back(queuedSound);

		    // No queued sounds.
		  }else{
		    mSourcePool.push(source);
		  }
		}

		return AL_NONE;
	}

	Ogre::ObjectMemoryManager* SoundManager::getDefaultMemoryManager() const
	{
		Ogre::SceneManager* sceneManager = mSoundContainer.begin()->first;
		return &sceneManager->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC);
	}

	int SoundManager::createSourcePool()
	{
	  SourceRef source;

	  // Starting with no sources.
	  int numSources = 0;
	  // So 
	  while(alGetError() == AL_NO_ERROR && numSources < mMaxNumSources) {
	    // Clear source handle and generate the source.
	    source = 0;
	    alGenSources(1, &source);

	    // If source creation worked.
	    if(source != 0) {
	      // Store the source in the list and keep track of count.
	      mSourcePool.push(source);
	      numSources++;
	      
	    }else{
	      // Failed to generate all required sources. Clear state.
	      alGetError();
	      // Discontinue generation of sources.
	      break;
	    }
	  }
	  
	  // If we could not generate all necessary sources.
	  if(numSources != mMaxNumSources) {
	    // Notify the user that they are limited.
	    Ogre::LogManager::getSingleton().logMessage("OgreAL: Warning: Failed to generate all required sources.");
	  }

	  return numSources;
	}

	void SoundManager::initializeDevice(const Ogre::String& deviceName)
	{
		alcGetIntegerv(NULL, ALC_MAJOR_VERSION, sizeof(mMajorVersion), &mMajorVersion);
		CheckError(alcGetError(NULL), "Failed to retrieve version info");
		alcGetIntegerv(NULL, ALC_MINOR_VERSION, sizeof(mMinorVersion), &mMinorVersion);
		CheckError(alcGetError(NULL), "Failed to retrieve version info");
		
		Ogre::LogManager::getSingleton().logMessage("OpenAL Version: " +
			Ogre::StringConverter::toString(mMajorVersion) + "." +
			Ogre::StringConverter::toString(mMinorVersion));

		/*
		** OpenAL versions prior to 1.0 DO NOT support device enumeration, so we
		** need to test the current version and decide if we should try to find 
		** an appropriate device or if we should just open the default device.
		*/
		bool deviceInList = false;
		if(mMajorVersion >= 1 && mMinorVersion >= 1)
		{
			Ogre::LogManager::getSingleton().logMessage("Available Devices");
			Ogre::LogManager::getSingleton().logMessage("-----------------");

			// List devices in log and see if the sugested device is in the list
			Ogre::StringVector deviceList = getDeviceList();
			std::stringstream ss;
			Ogre::StringVector::iterator deviceItr;
			for(deviceItr = deviceList.begin(); deviceItr != deviceList.end() && (*deviceItr).compare("") != 0; ++deviceItr)
			{
				deviceInList = (*deviceItr).compare(deviceName) == 0;
				ss << " * " << (*deviceItr);
				Ogre::LogManager::getSingleton().logMessage(ss.str());
				ss.clear(); ss.str("");
			}
		}

		// If the suggested device is in the list we use it, otherwise select the default device
		mDevice = alcOpenDevice(deviceInList ? deviceName.c_str() : NULL);
		CheckError(alcGetError(mDevice), "Failed to open Device");

		try
		{
			CheckCondition(mDevice != NULL, 13, "Failed to open audio device");
		}
		catch (...)
		{

		}

		Ogre::LogManager::getSingleton().logMessage("Choosing: " + Ogre::String(alcGetString(mDevice, ALC_DEVICE_SPECIFIER)));

		// Create OpenAL Context
		mContext = alcCreateContext(mDevice, NULL);
		CheckError(alcGetError(mDevice), "Failed to create Context");

		try
		{
			CheckCondition(mContext != NULL, 13, "Failed to create OpenAL Context");
		}
		catch (...)
		{

		}

		alcMakeContextCurrent(mContext);
		CheckError(alcGetError(mDevice), "Failed to set current context");
	}

	void SoundManager::checkFeatureSupport()
	{
		// Check for Supported Formats
		ALenum eBufferFormat = 0;
		eBufferFormat = alcGetEnumValue(mDevice, "AL_FORMAT_MONO16");
		if(eBufferFormat) mSupportedFormats[MONO_CHANNEL] = 
			new FormatData(eBufferFormat, "AL_FORMAT_MONO16", "Monophonic Sound");
		eBufferFormat = alcGetEnumValue(mDevice, "AL_FORMAT_STEREO16");
		if(eBufferFormat) mSupportedFormats[STEREO_CHANNEL] = 
			new FormatData(eBufferFormat, "AL_FORMAT_STEREO16", "Stereo Sound");
		eBufferFormat = alcGetEnumValue(mDevice, "AL_FORMAT_QUAD16");
		if(eBufferFormat) mSupportedFormats[QUAD_CHANNEL] = 
			new FormatData(eBufferFormat, "AL_FORMAT_QUAD16", "4 Channel Sound");
		eBufferFormat = alcGetEnumValue(mDevice, "AL_FORMAT_51CHN16");
		if(eBufferFormat) mSupportedFormats[MULTI_CHANNEL_51] = 
			new FormatData(eBufferFormat, "AL_FORMAT_51CHN16", "5.1 Surround Sound");
		eBufferFormat = alcGetEnumValue(mDevice, "AL_FORMAT_61CHN16");
		if(eBufferFormat) mSupportedFormats[MULTI_CHANNEL_61] = 
			new FormatData(eBufferFormat, "AL_FORMAT_61CHN16", "6.1 Surround Sound");
		eBufferFormat = alcGetEnumValue(mDevice, "AL_FORMAT_71CHN16");
		if(eBufferFormat) mSupportedFormats[MULTI_CHANNEL_71] = 
			new FormatData(eBufferFormat, "AL_FORMAT_71CHN16", "7.1 Surround Sound");

		// Log supported formats
		FormatMapIterator itr = getSupportedFormatIterator();
		Ogre::LogManager::getSingleton().logMessage("Supported Formats");
		Ogre::LogManager::getSingleton().logMessage("-----------------");
		while(itr.hasMoreElements())
		{
			Ogre::LogManager::getSingleton().logMessage(" * " + std::string(static_cast<char*>
				(itr.peekNextValue()->formatName)) + ", " + itr.peekNextValue()->formatDescription);
			itr.getNext();
		}

		// Check for EAX Support
		std::stringbuf versionString;
		for(int version = 5; version >= 2; version--)
		{
			versionString.str("EAX"+Ogre::StringConverter::toString(version)+".0");
			if(alIsExtensionPresent(versionString.str().data()) == AL_TRUE)
			{
				mEAXSupport = true;
				mEAXVersion = version;
				versionString.str("EAX " + Ogre::StringConverter::toString(version) + ".0 Detected");
				Ogre::LogManager::getSingleton().logMessage(versionString.str());
				break;
			}
		}

		// Check for EFX Support
		if(alcIsExtensionPresent(mDevice, "ALC_EXT_EFX") == AL_TRUE)
		{
			Ogre::LogManager::getSingleton().logMessage("EFX Extension Found");
		}

		// Check for X-RAM extension
		/*if(alIsExtensionPresent("EAX-RAM") == AL_TRUE)
		{
			mXRAMSupport = true;
			Ogre::LogManager::getSingleton().logMessage("X-RAM Detected");

			EAXSetBufferMode setXRamMode = (EAXSetBufferMode)alGetProcAddress("EAXSetBufferMode");
			EAXGetBufferMode getXRamMode = (EAXGetBufferMode)alGetProcAddress("EAXGetBufferMode");
			mXRamSize = alGetEnumValue("AL_EAX_RAM_SIZE");
			mXRamFree = alGetEnumValue("AL_EAX_RAM_FREE");
			
			Ogre::LogManager::getSingleton().logMessage("X-RAM: " + Ogre::StringConverter::toString(mXRamSize) +
				" (" + Ogre::StringConverter::toString(mXRamFree) + " free)");
		}*/

		// Create our pool of sources
		mMaxNumSources = createSourcePool();
	}

	void SoundManager::updateSounds()
	{
		// Lock Mutex
		OGREAL_LOCK_AUTO_MUTEX

		
		for (auto it = mSoundContainer.cbegin(); it != mSoundContainer.cend(); ++it)
		{
			const std::map<Ogre::String, Sound*>& soundMap = it->second;
			// Go over all sounds.
			for (auto& subIt = soundMap.cbegin(); subIt != soundMap.cend(); ++subIt)
			{
				// Update the Sound and Listeners if necessary	

				// Perform a sound update.
				subIt->second->updateSound();

				// If culling is enabled, perform cull on the sound.
				if (mCullDistance > 0.0)
				{
					performSoundCull(subIt->second);
				}
			}
		}

		if (nullptr != Listener::getSingletonPtr())
			Listener::getSingletonPtr()->updateListener();

		// Before sorting, lock states to use cache. Otherwise it is possible for
		// a sound to change state in the middle of sorting and cause a segmentation
		// fault. It has happened to me on numerous occasions.
		for (auto it = mSoundContainer.cbegin(); it != mSoundContainer.cend(); ++it)
		{
			const std::map<Ogre::String, Sound*>& soundMap = it->second;
			// Go over all sounds.
			for (auto& subIt = soundMap.cbegin(); subIt != soundMap.cend(); ++subIt)
			{
				subIt->second->setStateCached(true);
			}
		}
		// Sort the active and queued sounds
		std::sort(std::execution::par, mActiveSounds.begin(), mActiveSounds.end(), SortLowToHigh());
		std::sort(std::execution::par, mQueuedSounds.begin(), mQueuedSounds.end(), SortHighToLow());
		// Revert back to live states.
		for (auto it = mSoundContainer.cbegin(); it != mSoundContainer.cend(); ++it)
		{
			const std::map<Ogre::String, Sound*>& soundMap = it->second;
			// Go over all sounds.
			for (auto& subIt = soundMap.cbegin(); subIt != soundMap.cend(); ++subIt)
			{
				 subIt->second->setStateCached(false);
			}
		}

		// Check to see if we should sacrifice any sounds
		updateSourceAllocations();
	}

	void SoundManager::updateSourceAllocations()
	{
		while(!mQueuedSounds.empty() && !mActiveSounds.empty())
		{
			Sound *queuedSound = mQueuedSounds.front();
			Sound *activeSound = mActiveSounds.front();

			Ogre::Real distQueuedSound, distActiveSound;
			Ogre::Vector3 listenerPos = Listener::getSingleton().getDerivedPosition();
			if(queuedSound->isRelativeToListener())
			{
				distQueuedSound = queuedSound->getPosition().length();
			}
			else
			{
				distQueuedSound = queuedSound->getDerivedPosition().distance(listenerPos);
			}

			if(activeSound->isRelativeToListener())
			{
				distActiveSound = activeSound->getPosition().length();
			}
			else
			{
				distActiveSound = activeSound->getDerivedPosition().distance(listenerPos);
			}

			if(queuedSound->getPriority() > activeSound->getPriority() ||
			   queuedSound->getPriority() == activeSound->getPriority() && 
			   distQueuedSound < distActiveSound)
			{
				// Remove the sounds from their respective lists
				mActiveSounds.erase(mActiveSounds.begin());
				mQueuedSounds.erase(mQueuedSounds.begin());

				// Steal the source from the currently active sound
				activeSound->pause();
				activeSound->unqueueBuffers();
				SourceRef source = activeSound->getSource();
				activeSound->setSource(AL_NONE);

				// Kickstart the currently queued sound
				queuedSound->setSource(source);
				queuedSound->play();

				// Add the sound back to the correct lists
				mActiveSounds.push_back(queuedSound);
				mQueuedSounds.push_back(activeSound);
			}
			else
			{
				// We have no more sounds that we can sacrifice
				break;
			}
		}
	}

  void SoundManager::performDeleteQueueCycle() {
    // Destroy any sounds that were queued last frame
    SoundList::iterator soundItr = mSoundsToDestroy.begin();
    while(!mSoundsToDestroy.empty())
      {
	// Grab the pointer to the sound.
	Sound* sound = *soundItr;

	// If the sound has stored gain for distance culling.
	SoundGainMap::iterator soundGainItr = mSoundGainMap.find(sound);
	if(soundGainItr != mSoundGainMap.end()) {
	  // Delete it along with the sound.
	  mSoundGainMap.erase(soundGainItr);
	}

	// Free sound instance.
	delete sound;
	// Remove from sound list.
	soundItr = mSoundsToDestroy.erase(soundItr);
      }
  }

  void SoundManager::performSoundCull(Sound* sound) {
    // Get the distance between the sound and the listener.
    Ogre::Vector3 soundToListener = sound->getDerivedPosition() - getListener()->getPosition();
    // Attempt to get the iterator for the sound from the cull map.
    SoundGainMap::iterator soundGainItr = mSoundGainMap.find(sound);

    // If the sound is further than the cull distance from the listener and
    // is not already culled, cull it.
    if(soundToListener.length() > mCullDistance &&
       soundGainItr == mSoundGainMap.end()) {
      cullSound(sound);

      // Or if the sound is closer than the cull distance to the listener,
      // or the culling has been turned off but the sound is culled, uncull.
    }else if((soundToListener.length() <= mCullDistance || 
	      mCullDistance <= 0.0) &&
	     soundGainItr != mSoundGainMap.end()) {
      uncullSound(soundGainItr);
    }
  }

  // Callers of this function must guarantee that the sound is valid and not 
  // already culled.
  void SoundManager::cullSound(Sound* sound) {
    // Add to the culling map.
    mSoundGainMap[sound] = sound->getGain();
    // Set gain of the sound to zero.
    sound->setGain(0.0);
  }

  // Callers of this function must guarantee that the sound is valid and that
  // it is already culled (ie. has an entry in the sound gain map.
  void SoundManager::uncullSound(Sound* sound) {
    // Attempt to find the sound.
    SoundGainMap::iterator i = mSoundGainMap.find(sound);
    // If the sound exists, uncull it.
    if(i != mSoundGainMap.end()) {
      uncullSound(i);
    }
  }

  void SoundManager::uncullSound(SoundGainMap::iterator& soundGainItr) {
    // Restore the sound's gain.
    soundGainItr->first->setGain(soundGainItr->second);
    // Remove the entry from the gain map.
    mSoundGainMap.erase(soundGainItr);
  }

} // Namespace

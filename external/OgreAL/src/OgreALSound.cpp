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
#include "OgreALSound.h"
#include "OgreALOggSound.h"
#include "OgreALWavSound.h"
#include "OgreALSoundManager.h"

namespace OgreAL
{
	Sound::Listener::Listener() : stops(false), loops(false)
	{

	}

	Sound::Listener::~Listener()
	{

	}

	void Sound::Listener::soundStopped(Sound* sound)
	{

	}

	void Sound::Listener::soundLooped(Sound* sound)
	{

	}

	void Sound::Listener::soundAtOffset(Sound* sound, Ogre::Real requestedOffset, Ogre::Real actualOffset)
	{

	}

	Sound::Sound(Ogre::SceneManager* sceneMgr) :
		mSource(0),
		mFormat(0),
		mFreq(0),
		mSize(0),
		mBPS(0),
		mChannels(0),
		mLengthInSeconds(0),
		mManualRestart(false),
		mManualStop(false),
		mPreviousOffset(0),
		mBuffers(0),
		mBufferSize(0),
		mBuffersLoaded(false),
		mBuffersQueued(false),
		mPriority(NORMAL),
		mFinishedCallback(0),
		mLoopedCallback(0),
		mSpectrumCallback(0),
		mStartTime(0),
		mStream(false),
		mSumDataRead(0),
		MovableObject(Ogre::Id::generateNewId<MovableObject>(),
			SoundManager::getSingleton().getDefaultMemoryManager(),
			sceneMgr,
			SoundManager::getSingleton().getDefaultRenderQueue()),
		mPitch(1.0f),
		mGain(1.0f),
		mMaxGain(1.0f),
		mMinGain(0.0f),
		mFadeMode(FADE_NONE),
		mFadeTime(0.0f),
		mMaxDistance(3400.0f),
		mRolloffFactor(1.0f),
		mReferenceDistance(150.0f),
		mOuterConeGain(0.0f),
		mInnerConeAngle(360.0f),
		mOuterConeAngle(360.0f),
		mPosition(Ogre::Vector3::ZERO),
		mVelocity(Ogre::Vector3::ZERO),
		mDirection(Ogre::Vector3::NEGATIVE_UNIT_Z),
		mFileName(""),
		mStateCached(false),
		mState(AL_INITIAL),
		mSourceRelative(AL_FALSE),
		mDerivedPosition(Ogre::Vector3::ZERO),
		mDerivedDirection(Ogre::Vector3::NEGATIVE_UNIT_Z),
		mSpectrumParameter(nullptr),
		mSpectrumProcessingSize(1024),
		mSpectrumNumberOfBands(20),
		mMathWindowType(MathWindows::RECTANGULAR),
		mSpectrumPreparationType(AudioProcessor::LOGARITHMIC),
		mAudioProcessor(nullptr),
		mCurrentSpectrumPos(0.0f),
		mFractionSum(0.0f),
		mRenderDelta(0),
		mLastTime(0),
		mTotalElapsedTime(0.0f),
		mFirstTimeReady(true),
		mTargetDeltaMS(0),
		mDataRead(0)
	{
		mParentNode = nullptr;
	}

	Sound::Sound(Ogre::SceneManager* sceneMgr, const Ogre::String& name, const Ogre::String& fileName, bool stream) :
		mSource(0),
		mFormat(0),
		mFreq(0),
		mSize(0),
		mBPS(0),
		mChannels(0),
		mLengthInSeconds(0),
		mManualRestart(false),
		mManualStop(false),
		mPreviousOffset(0),
		mBuffers(0),
		mBufferSize(0),
		mBuffersLoaded(false),
		mBuffersQueued(false),
		mNumBuffers(stream ? 2 : 1),
		mPriority(NORMAL),
		mFinishedCallback(0),
		mLoopedCallback(0),
		mSpectrumCallback(0),
		mStartTime(0),
		mStream(stream),
		mSumDataRead(0),
		MovableObject(Ogre::Id::generateNewId<MovableObject>(),
			SoundManager::getSingleton().getDefaultMemoryManager(),
			sceneMgr,
			SoundManager::getSingleton().getDefaultRenderQueue()),
		mPitch(1.0f), mGain(1.0f),
		mMaxGain(1.0f),
		mMinGain(0.0f),
		mFadeMode(FADE_NONE),
		mFadeTime(0.0f),
		mMaxDistance(3400.0f),
		mRolloffFactor(1.0f),
		mReferenceDistance(150.0f),
		mOuterConeGain(0.0f),
		mInnerConeAngle(360.0f),
		mOuterConeAngle(360.0f),
		mPosition(Ogre::Vector3::ZERO),
		mVelocity(Ogre::Vector3::ZERO),
		mDirection(Ogre::Vector3::NEGATIVE_UNIT_Z),
		mFileName(fileName),
		mStateCached(false),
		mState(AL_INITIAL),
		mSourceRelative(AL_FALSE),
		mDerivedPosition(Ogre::Vector3::ZERO),
		mDerivedDirection(Ogre::Vector3::NEGATIVE_UNIT_Z),
		mSpectrumParameter(nullptr),
		mSpectrumProcessingSize(1024),
		mSpectrumNumberOfBands(20),
		mMathWindowType(MathWindows::RECTANGULAR),
		mSpectrumPreparationType(AudioProcessor::LOGARITHMIC),
		mAudioProcessor(nullptr),
		mCurrentSpectrumPos(0.0f),
		mFractionSum(0.0f),
		mRenderDelta(0),
		mLastTime(0),
		mTotalElapsedTime(0.0f),
		mFirstTimeReady(true),
		mTargetDeltaMS(0),
		mDataRead(0)
	{
		setName(name);
		mParentNode = nullptr;
	}


	Sound::Sound(Ogre::SceneManager* sceneMgr, BufferRef buffer, const Ogre::String& name, const Ogre::String& fileName, bool loop) :
		mSource(0),
		mFormat(0),
		mFreq(0),
		mSize(0),
		mBPS(0),
		mChannels(0),
		mLengthInSeconds(0),
		mManualRestart(false),
		mManualStop(false),
		mPreviousOffset(0),
		mBufferSize(0),
		mBuffersLoaded(true),
		mBuffersQueued(false),
		mNumBuffers(1),
		mPriority(NORMAL),
		mFinishedCallback(0),
		mLoopedCallback(0),
		mSpectrumCallback(0),
		mStartTime(0),
		mStream(false),
		mSumDataRead(0),
		MovableObject(Ogre::Id::generateNewId<MovableObject>(),
			SoundManager::getSingleton().getDefaultMemoryManager(),
			sceneMgr,
			SoundManager::getSingleton().getDefaultRenderQueue()),
		mLoop(loop ? AL_TRUE : AL_FALSE),
		mPitch(1.0f), mGain(1.0f),
		mMaxGain(1.0f), mMinGain(0.0f),
		mFadeMode(FADE_NONE),
		mFadeTime(0.0f),
		mMaxDistance(3400.0f),
		mRolloffFactor(1.0f),
		mReferenceDistance(150.0f),
		mOuterConeGain(0.0f),
		mInnerConeAngle(360.0f),
		mOuterConeAngle(360.0f),
		mPosition(Ogre::Vector3::ZERO),
		mVelocity(Ogre::Vector3::ZERO),
		mDirection(Ogre::Vector3::NEGATIVE_UNIT_Z),
		mFileName(fileName),
		mStateCached(false),
		mState(AL_INITIAL),
		mSourceRelative(AL_FALSE),
		mDerivedPosition(Ogre::Vector3::ZERO),
		mDerivedDirection(Ogre::Vector3::NEGATIVE_UNIT_Z),
		mSpectrumParameter(nullptr),
		mSpectrumProcessingSize(1024),
		mSpectrumNumberOfBands(20),
		mMathWindowType(MathWindows::RECTANGULAR),
		mSpectrumPreparationType(AudioProcessor::LOGARITHMIC),
		mAudioProcessor(nullptr),
		mRenderDelta(0),
		mLastTime(0),
		mTotalElapsedTime(0.0f),
		mFirstTimeReady(true),
		mTargetDeltaMS(0),
		mCurrentSpectrumPos(0.0f),
		mFractionSum(0.0f)
	{
		setName(name);
		mParentNode = nullptr;

		// Allocate buffer entry.
		mBuffers = new BufferRef[mNumBuffers];
		mBuffers[0] = buffer;
		// Notify the sound manager of our use of this buffer.
		SoundManager::getSingleton()._addBufferRef(mFileName, buffer);

		alGetBufferi(mBuffers[0], AL_FREQUENCY, &mFreq);
		alGetBufferi(mBuffers[0], AL_BITS, &mBPS);
		alGetBufferi(mBuffers[0], AL_CHANNELS, &mChannels);
		alGetBufferi(mBuffers[0], AL_SIZE, &mSize);

		calculateFormatInfo();

		// mBufferSize is equal to 1/4 of a second of audio
		mLengthInSeconds = (Ogre::Real)mSize / (Ogre::Real)(mBufferSize * 4);
	}

	Sound::~Sound()
	{
		// We need to be stopped in order for the buffer to be releasable.
		stop();

		// The sound manager needs to also release the source of this sound
		// object if it was valid (it will figure that out). Put it back into
		// the source pool and all that.
		SoundManager::getSingleton()._releaseSource(this);

		// Attempting to release buffers now that we've possibly decremented
		// the reference count by setting a null buffer on our source.
		try
		{
			// If we have buffers to deallocate, do so.
			if (mBuffers != NULL)
			{
				// Buffer deletion is dependent on streaming.
				if (mStream)
				{
					// We are streaming, so this buffer is all ours. In this case
					// we are responsible for the cleanup.
					alDeleteBuffers(mNumBuffers, mBuffers);
					CheckError(alGetError(), "Failed to delete a streamed buffer.");

				}
				else
				{
					// We are using a non-streamed buffer, so this buffer could very
					// well be shared among a number of sounds. We leave proper 
					// destruction and deletion to the sound factory.
					SoundManager::getSingleton()._removeBufferRef(mFileName);
				}
			}
		}
		catch (...)
		{
			// Don't die because of this.
		}


		delete[] mBuffers;

		if (nullptr != mFinishedCallback)
		{
			delete mFinishedCallback;
			mFinishedCallback = nullptr;
		}
		if (nullptr != mLoopedCallback)
		{
			delete mLoopedCallback;
			mLoopedCallback = nullptr;
		}
		if (nullptr != mSpectrumCallback)
		{
			delete mSpectrumCallback;
			mSpectrumCallback = nullptr;
		}
	}

	bool Sound::play()
	{
		// If we're already playing, bail out.
		if (isPlaying()) return true;

		if (!isPaused())
		{
			mManualRestart = false;
			mManualStop = false;
			mPreviousOffset = 0;
		}

		if (mStartTime == 0)
		{
			time(&mStartTime);
		}
		else if (!isPaused())
		{
			time_t currentTime;
			time(&currentTime);

			setSecondOffset(currentTime - mStartTime);
		}

		if (mSource != AL_NONE || (mSource = SoundManager::getSingleton()._requestSource(this)) != AL_NONE)
		{
			if (!mBuffersLoaded)
			{
				mBuffersLoaded = loadBuffers();
			}

			if (!mBuffersQueued)
			{
				// Unqueue any buffers that may be left over
				unqueueBuffers();
				queueBuffers();
			}

			initSource();

			alSourcePlay(mSource);
			CheckError(alGetError(), "Failed to play sound");
		}

		return false;
	}

	bool Sound::isPlaying() const
	{
		if (mSource == AL_NONE)
		{
			return false;
		}

		// Perform a cached state refresh. No refresh will be done if using cache.
		updateStateCache();

		return (mState == AL_PLAYING);
	}

	bool Sound::pause()
	{
		if (!isPlaying()) return true;

		if (mSource != AL_NONE)
		{
			alSourcePause(mSource);
			CheckError(alGetError(), "Failed to pause sound");
		}

		return false;
	}

	bool Sound::isPaused() const
	{
		if (mSource == AL_NONE)
		{
			return false;
		}

		// Perform a cached state refresh. No refresh will be done if using cache.ddww

		return (mState == AL_PAUSED);
	}

	bool Sound::stop()
	{
		mSumDataRead = 0;
		mFirstTimeReady = true;

		mTimer.reset();
		mFractionSum = 0.0f;
		mTargetDeltaMS = 0;
		mRenderDelta = 0;
		mLastTime = 0;
		mTotalElapsedTime = 0.0f;

		if (nullptr != mSpectrumCallback)
		{
			mDataRead = 0;
			mCurrentSpectrumPos = 0.0f;

			if (nullptr != mAudioProcessor)
			{
				delete mAudioProcessor;
				mAudioProcessor = nullptr;
			}

			if (nullptr != mSpectrumParameter)
			{
				delete mSpectrumParameter;
				mSpectrumParameter = nullptr;
			}
		}

		if (isStopped())
		{
			if (mSource != AL_NONE)
			{
				mSource = SoundManager::getSingleton()._releaseSource(this);
				return true;
			}
		}
		else if (mSource != AL_NONE)
		{
			mManualStop = true;

			// D GILL to avoid AL lib: (EE) alc_cleanup: 1 device not closed ogre 
			setLoop(false);

			// Stop the source
			alSourceStop(mSource);
			CheckError(alGetError(), "Failed to stop sound");

			unqueueBuffers();

			mBuffersLoaded = unloadBuffers();
		}

		mSource = SoundManager::getSingleton()._releaseSource(this);

		mStartTime = 0;

		return true;
	}

	bool Sound::isStopped() const
	{
		if (mSource == AL_NONE)
		{
			return true;
		}

		// Perform a cached state refresh. No refresh will be done if using cache.
		updateStateCache();

		return (mState == AL_STOPPED);
	}

	bool Sound::isInitial() const
	{
		if (mSource == AL_NONE)
		{
			return true;
		}

		// Perform a cached state refresh. No refresh will be done if using cache.
		updateStateCache();

		return (mState == AL_INITIAL);
	}

	bool Sound::isFading() const
	{
		return mFadeMode != FADE_NONE;
	}

	bool Sound::fadeIn(Ogre::Real fadeTime)
	{
		// If we're playing, then we're going to fade-in from the current gain
		// value. This means that in order to perform the fade-in, we must also
		// not already be at the maximum gain. Also, this only makes sense while
		// we're fading out or not fading at all. I have not thought of a 
		// semantic where calling a fade-in while fading in is worthwhile.
		if (isPlaying() &&
			mGain != mMaxGain &&
			(mFadeMode == FADE_NONE || mFadeMode == FADE_OUT))
		{
			// Swap fade direction.
			mFadeMode = FADE_IN;

			// What we're going to do is use the existing updateFading code by
			// modifying the fadeTime and running times so that, when the caller
			// has asked for a fade-in from the current gain to the max gain over
			// fadeTime, we create such a fadeTime and running time as to already
			// have arrived at these values.
			//
			// A                   B          C
			// |===================|----------|
			//
			// Consider A as our min-gain, B as our current gain and C as our
			// max-gain. If we are to fade in from B to C over fadeTime, then
			// this is essentially the same as fadeTime being scaled by the
			// ratio of AC/BC and running time being placed at B.
			mFadeTime = ((mMaxGain - mMinGain) / (mMaxGain - mGain)) * fadeTime;
			mRunning = ((mGain - mMinGain) / (mMaxGain - mMinGain)) * mFadeTime;

			return true;

		}
		else
		{
			// Not playing. Can safely assume the caller wants the sound faded in 
			// from minimum gain, rather than bursting it in and going from the
			// currently applied gain; this was the default behaviour.
			mFadeMode = FADE_IN;
			mFadeTime = fadeTime;
			mRunning = 0.0;
			// Start at min gain..
			setGain(mMinGain);
			// ..and play
			return play();
		}


		// A fade-in has been requested either:
		//
		//  * When a fade-in is already in progress.
		//  * When a sound is playing but is already at max-gain.
		//  * When a fade-out is in progress but we're at max-gain.
		//
		return false;
	}

	bool Sound::fadeOut(Ogre::Real fadeTime)
	{
		// The constructs in this fade-out code are extremely similar to that of
		// the fade-in code. For details about operation, consult the respective
		// function. The difference with fade outs is it only makes sense to do
		// one iff:
		//
		//  * We're playing the sound.
		//  * We're not at min-gain already.
		//  * A fade-out is not already in progress.
		//
		if (isPlaying() &&
			mGain != mMinGain &&
			(mFadeMode == FADE_NONE || mFadeMode == FADE_IN))
		{
			// Swap fade direction.
			mFadeMode = FADE_OUT;

			// Construct fadeTime and running times so that they interpolate nicely
			// across the supplied time interval. This time we go from B to A.
			mFadeTime = ((mMaxGain - mMinGain) / (mGain - mMinGain)) * fadeTime;
			mRunning = ((mMaxGain - mGain) / (mMaxGain - mMinGain)) * mFadeTime;

			return true;

		}

		// A fade-out was requested while:
		//
		//  * The sound is not playing.
		//  * The sound is already at the minimum gain.
		//  * A fade-out is already in progress.
		return false;
	}

	bool Sound::cancelFade()
	{
		// If we have a running fade, turn it off.
		if (isFading())
		{
			mFadeMode = FADE_NONE;
			return true;
		}

		// No fading was occuring.
		return false;
	}

	void Sound::setPitch(Ogre::Real pitch)
	{
		if (pitch <= 0) return;

		mPitch = pitch;

		if (mSource != AL_NONE)
		{
			alSourcef(mSource, AL_PITCH, mPitch);
			CheckError(alGetError(), "Failed to set Pitch");
		}
	}

	void Sound::setGain(Ogre::Real gain)
	{
		if (gain < 0) return;

		mGain = gain;

		if (mSource != AL_NONE)
		{
			alSourcef(mSource, AL_GAIN, mGain);
			CheckError(alGetError(), "Failed to set Gain");
		}
	}

	void Sound::setMaxGain(Ogre::Real maxGain)
	{
		if (maxGain < 0 || maxGain > 1) return;

		mMaxGain = maxGain;

		if (mSource != AL_NONE)
		{
			alSourcef(mSource, AL_MAX_GAIN, mMaxGain);
			CheckError(alGetError(), "Failed to set Max Gain");
		}
	}

	void Sound::setMinGain(Ogre::Real minGain)
	{
		if (minGain < 0 || minGain > 1) return;

		mMinGain = minGain;

		if (mSource != AL_NONE)
		{
			alSourcef(mSource, AL_MIN_GAIN, mMinGain);
			CheckError(alGetError(), "Failed to set Min Gain");
		}
	}

	void Sound::setMaxDistance(Ogre::Real maxDistance)
	{
		if (maxDistance < 0) return;

		mMaxDistance = maxDistance;

		if (mSource != AL_NONE)
		{
			alSourcef(mSource, AL_MAX_DISTANCE, mMaxDistance);
			CheckError(alGetError(), "Failed to set Max Distance");
		}
	}

	void Sound::setRolloffFactor(Ogre::Real rolloffFactor)
	{
		if (rolloffFactor < 0) return;

		mRolloffFactor = rolloffFactor;

		if (mSource != AL_NONE)
		{
			alSourcef(mSource, AL_ROLLOFF_FACTOR, mRolloffFactor);
			CheckError(alGetError(), "Failed to set Rolloff Factor");
		}
	}

	void Sound::setReferenceDistance(Ogre::Real refDistance)
	{
		if (refDistance < 0) return;

		mReferenceDistance = refDistance;

		if (mSource != AL_NONE)
		{
			alSourcef(mSource, AL_REFERENCE_DISTANCE, mReferenceDistance);
			CheckError(alGetError(), "Failed to set Reference Distance");
		}
	}

	void Sound::setDistanceValues(Ogre::Real maxDistance, Ogre::Real rolloffFactor, Ogre::Real refDistance)
	{
		setMaxDistance(maxDistance);
		setRolloffFactor(rolloffFactor);
		setReferenceDistance(refDistance);
	}

	void Sound::setVelocity(Ogre::Real x, Ogre::Real y, Ogre::Real z)
	{
		mVelocity.x = x;
		mVelocity.y = y;
		mVelocity.z = z;

		if (mSource != AL_NONE)
		{
			alSource3f(mSource, AL_VELOCITY, mVelocity.x, mVelocity.y, mVelocity.z);
			CheckError(alGetError(), "Failed to set Velocity");
		}
	}

	void Sound::setVelocity(const Ogre::Vector3& vec)
	{
		setVelocity(vec.x, vec.y, vec.z);
	}

	void Sound::setRelativeToListener(bool relative)
	{
		// Do not set to relative if it's connected to a node
		if (mParentNode) return;

		mSourceRelative = relative;

		if (mSource != AL_NONE)
		{
			alSourcei(mSource, AL_SOURCE_RELATIVE, mSourceRelative);
			CheckError(alGetError(), "Failed to set Source Relative");
		}
	}

	void Sound::setOuterConeGain(Ogre::Real outerConeGain)
	{
		if (outerConeGain < 0 || outerConeGain > 1) return;

		mOuterConeGain = outerConeGain;

		if (mSource != AL_NONE)
		{
			alSourcef(mSource, AL_CONE_OUTER_GAIN, mOuterConeGain);
			CheckError(alGetError(), "Failed to set Outer Cone Gain");
		}
	}

	void Sound::setInnerConeAngle(Ogre::Real innerConeAngle)
	{
		mInnerConeAngle = innerConeAngle;

		if (mSource != AL_NONE)
		{
			alSourcef(mSource, AL_CONE_INNER_ANGLE, mInnerConeAngle);
			CheckError(alGetError(), "Failed to set Inner Cone Angle");
		}
	}

	void Sound::setOuterConeAngle(Ogre::Real outerConeAngle)
	{
		mOuterConeAngle = outerConeAngle;

		if (mSource != AL_NONE)
		{
			alSourcef(mSource, AL_CONE_OUTER_ANGLE, mOuterConeAngle);
			CheckError(alGetError(), "Failed to set Outer Cone Angle");
		}
	}

	void Sound::setLoop(bool loop)
	{
		mLoop = loop ? AL_TRUE : AL_FALSE;

		if (mSource != AL_NONE && !mStream)
		{
			alSourcei(mSource, AL_LOOPING, mLoop);
			CheckError(alGetError(), "Failed to set Looping");
		}
	}

	void Sound::setSecondOffset(Ogre::Real seconds)
	{
		if (seconds < 0) return;

		if (mSource != AL_NONE)
		{
			alSourcef(mSource, AL_SEC_OFFSET, seconds);
			CheckError(alGetError(), "Failed to set offset");
		}
	}

	Ogre::Real Sound::getSecondOffset()
	{
		Ogre::Real offset = 0;

		if (mSource != AL_NONE)
		{
			alGetSourcef(mSource, AL_SEC_OFFSET, &offset);
			CheckError(alGetError(), "Failed to set Looping");
		}

		return offset;
	}

	void Sound::addListener(Listener* listener)
	{
		// Attempt to find the listener in the list of listeners.
		ListenerList::iterator i = std::find(mListeners.begin(), mListeners.end(), listener);

		// Add the listener only if it's not already registered.
		if (i == mListeners.end())
		{
			mListeners.push_back(listener);
		}
	}

	void Sound::removeListener(Listener* listener)
	{
		// Attempt to find the listener in the list of listeners.
		ListenerList::iterator i = std::find(mListeners.begin(), mListeners.end(), listener);

		// If it exists, remove it from the list.
		if (i != mListeners.end())
		{
			mListeners.erase(i);
		}
	}


	void Sound::setPosition(Ogre::Real x, Ogre::Real y, Ogre::Real z)
	{
		mPosition.x = x;
		mPosition.y = y;
		mPosition.z = z;
		mLocalTransformDirty = true;
	}

	void Sound::setPosition(const Ogre::Vector3& vec)
	{
		mPosition = vec;
		mLocalTransformDirty = true;
	}

	void Sound::setDirection(Ogre::Real x, Ogre::Real y, Ogre::Real z)
	{
		mDirection.x = x;
		mDirection.y = y;
		mDirection.z = z;
		mLocalTransformDirty = true;
	}

	void Sound::setDirection(const Ogre::Vector3& vec)
	{
		mDirection = vec;
		mLocalTransformDirty = true;
	}

	void Sound::_update() const
	{
		if (mParentNode)
		{
			if (!(mParentNode->_getDerivedOrientation() == mLastParentOrientation &&
				mParentNode->_getDerivedPosition() == mLastParentPosition)
				|| mLocalTransformDirty)
			{
				// Ok, we're out of date with SceneNode we're attached to
				mLastParentOrientation = mParentNode->_getDerivedOrientation();
				mLastParentPosition = mParentNode->_getDerivedPosition();
				mDerivedDirection = mLastParentOrientation * mDirection;
				mDerivedPosition = (mLastParentOrientation * mPosition) + mLastParentPosition;
			}
		}
		else
		{
			mDerivedPosition = mPosition;
			mDerivedDirection = mDirection;
		}

		mLocalTransformDirty = false;
	}

	void Sound::_updateFading()
	{
		if (mFadeMode != FADE_NONE)
		{
			mRunning += SoundManager::getSingletonPtr()->_getLastDeltaTime();
			// Calculate volume between min and max Gain over fade time
			Ogre::Real delta = mMaxGain - mMinGain;
			Ogre::Real gain;

			if (mFadeMode == FADE_IN)
			{
				gain = mMinGain + (delta * mRunning / mFadeTime);
				// Clamp & stop if needed
				if (gain > mMaxGain)
				{
					gain = mMaxGain;
					mFadeMode = FADE_NONE;
				}
			}
			else if (mFadeMode == FADE_OUT)
			{
				gain = mMaxGain - (delta * mRunning / mFadeTime);
				// Clamp & stop if needed
				if (gain < mMinGain)
				{
					gain = mMinGain;
					mFadeMode = FADE_NONE;
				}
			}

			// Set the adjusted gain
			setGain(gain);
		}
	}

	bool Sound::updateSound()
	{
		_update();

		if (mSource != AL_NONE)
		{
			// If the sound isn't playing, but we have a source and the sound has
			// ended naturally (was not stopped by a call to stop()).
			if (isStopped() && !mManualStop)
			{
				// If we have a callback, notify the callback of the sound's halt.
				if (mFinishedCallback)
				{
					mFinishedCallback->execute(this);
				}

				// We have ended. Release the source for reuse.
				mSource = SoundManager::getSingleton()._releaseSource(this);

				// To prevent constant re-firing of this event due to
				// the per-frame update nature of the Sound, we flip the
				// manual-stop flag. This flag is only used for callback
				// purposes.
				mManualStop = true;

				// The rest of the code concerns fades, which we don't need to 
				// process since we're stopped and now have no source to play from.
				return true;
			}

			// Get the current offset.
			Ogre::Real currOffset = getSecondOffset();
			// If we have looped around, because our current offset is less than
			// our previous, then we call the callback if we have one.
			if (isLooping() && currOffset < mPreviousOffset && !mManualRestart && mLoopedCallback)
			{
				mLoopedCallback->execute(this);
			}

			// Go through all listeners.
			for (ListenerList::iterator i = mListeners.begin(); i != mListeners.end(); ++i)
			{
				// Go through all registered offsets.
				for (Listener::OffsetList::iterator j = (*i)->offsets.begin(); j != (*i)->offsets.end(); ++j)
				{
					// Grab the requested offset.
					Ogre::Real reqOffset = *j;

					// Here's a bit of a hairy one. If the current offset is greater than
					// our previous offset, then we have simply advanced in the file normally.
					if (currOffset >= mPreviousOffset)
					{
						if (reqOffset > mPreviousOffset&& reqOffset <= currOffset)
						{
							// Good to go. Notify the listener that we have reached or passed the
							// offset.
							(*i)->soundAtOffset(this, reqOffset, currOffset);
						}

						// Otherwise our current offset is less than the previous, in which
						// case we probably looped. Whatever the cause, we notify listeners
						// at or beyond the previous offset up until the duration, and from
						// the zero offset to the current; A and B in the diagram.
						//
						//   _____________________
						//  |                     |
						// |---->..............----|
						// AAAAAA              BBBBB
						//
					}
					else
					{
						if ((reqOffset >= 0.0 && reqOffset <= currOffset) || // A
							(reqOffset > mPreviousOffset&& reqOffset <= getSecondDuration()))
						{ // B
// We looped around. The logic of figuring our that we've looped
// is up to the listener.
							(*i)->soundAtOffset(this, reqOffset, currOffset);
						}
					}

				}
			}

			// Done with these comparisons. Last offset becomes current for next frame.
			mPreviousOffset = currOffset;

			alSource3f(mSource, AL_POSITION, mDerivedPosition.x, mDerivedPosition.y, mDerivedPosition.z);
			CheckError(alGetError(), "Failed to set Position");

			alSource3f(mSource, AL_DIRECTION, mDerivedDirection.x, mDerivedDirection.y, mDerivedDirection.z);
			CheckError(alGetError(), "Failed to set Direction");

			// Fading
			_updateFading();
		}

		return true;
	}

	void Sound::initSource()
	{
		if (mSource == AL_NONE)
		{
			return;
		}

		alSourcef(mSource, AL_PITCH, mPitch);
		alSourcef(mSource, AL_GAIN, mGain);
		alSourcef(mSource, AL_MAX_GAIN, mMaxGain);
		alSourcef(mSource, AL_MIN_GAIN, mMinGain);
		alSourcef(mSource, AL_MAX_DISTANCE, mMaxDistance);
		alSourcef(mSource, AL_ROLLOFF_FACTOR, mRolloffFactor);
		alSourcef(mSource, AL_REFERENCE_DISTANCE, mReferenceDistance);
		alSourcef(mSource, AL_CONE_OUTER_GAIN, mOuterConeGain);
		alSourcef(mSource, AL_CONE_INNER_ANGLE, mInnerConeAngle);
		alSourcef(mSource, AL_CONE_OUTER_ANGLE, mOuterConeAngle);
		alSource3f(mSource, AL_POSITION, mDerivedPosition.x, mDerivedPosition.y, mDerivedPosition.z);
		alSource3f(mSource, AL_VELOCITY, mVelocity.x, mVelocity.y, mVelocity.z);
		alSource3f(mSource, AL_DIRECTION, mDerivedDirection.x, mDerivedDirection.y, mDerivedDirection.z);
		alSourcei(mSource, AL_SOURCE_RELATIVE, mSourceRelative);
		// There is an issue with looping and streaming, so we will
		// disable looping and deal with it on our own.
		alSourcei(mSource, AL_LOOPING, mStream ? AL_FALSE : mLoop);
		CheckError(alGetError(), "Failed to initialize source");
	}

	void Sound::generateBuffers()
	{
		// Create the buffers.
		mBuffers = new BufferRef[mNumBuffers];
		alGenBuffers(mNumBuffers, mBuffers);
		CheckError(alGetError(), "Could not generate buffer");

		// If we are not streaming.
		if (!mStream)
		{
			// Notify the sound manager of our use of this buffer so that it can be shared.
			SoundManager::getSingleton()._addBufferRef(mFileName, mBuffers[0]);
		}

		/*if (SoundManager::getSingleton().xRamSupport())
		{
			SoundManager::getSingleton().eaxSetBufferMode(mNumBuffers, mBuffers, SoundManager::xRamHardware);
		}*/
	}

	void Sound::calculateFormatInfo()
	{
		/*if(alIsExtensionPresent("AL_EXT_float32") == AL_TRUE)
		{
			 #define AL_FORMAT_MONO_FLOAT32                   0x10010
				#define AL_FORMAT_STEREO_FLOAT32                 0x10011
		}*/

		switch (mChannels)
		{
		case 1:
			if (mBPS == 8)
			{
				mFormat = AL_FORMAT_MONO8;
				// Set BufferSize to 250ms (Frequency divided by 4 (quarter of a second))
				mBufferSize = mFreq / 4;
			}
			else
			{
				mFormat = AL_FORMAT_MONO16;
				// Set BufferSize to 250ms (Frequency * 2 (16bit) divided by 4 (quarter of a second))
				mBufferSize = mFreq >> 1;
				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				mBufferSize -= (mBufferSize % 2);
			}
			break;
		case 2:
			if (mBPS == 8)
			{
				mFormat = AL_FORMAT_STEREO16;
				// Set BufferSize to 250ms (Frequency * 2 (8bit stereo) divided by 4 (quarter of a second))
				mBufferSize = mFreq >> 1;
				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				mBufferSize -= (mBufferSize % 2);
			}
			else
			{
				mFormat = AL_FORMAT_STEREO16;
				// Set BufferSize to 250ms (Frequency * 4 (16bit stereo) divided by 4 (quarter of a second))
				mBufferSize = mFreq;
				// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
				mBufferSize -= (mBufferSize % 4);
			}
			break;
		case 4:
			mFormat = alGetEnumValue("AL_FORMAT_QUAD16");
			// Set BufferSize to 250ms (Frequency * 8 (16bit 4-channel) divided by 4 (quarter of a second))
			mBufferSize = mFreq * 2;
			// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
			mBufferSize -= (mBufferSize % 8);
			break;
		case 6:
			mFormat = alGetEnumValue("AL_FORMAT_51CHN16");
			// Set BufferSize to 250ms (Frequency * 12 (16bit 6-channel) divided by 4 (quarter of a second))
			mBufferSize = mFreq * 3;
			// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
			mBufferSize -= (mBufferSize % 12);
			break;
		case 7:
			mFormat = alGetEnumValue("AL_FORMAT_61CHN16");
			// Set BufferSize to 250ms (Frequency * 16 (16bit 7-channel) divided by 4 (quarter of a second))
			mBufferSize = mFreq * 4;
			// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
			mBufferSize -= (mBufferSize % 16);
			break;
		case 8:
			mFormat = alGetEnumValue("AL_FORMAT_71CHN16");
			// Set BufferSize to 250ms (Frequency * 20 (16bit 8-channel) divided by 4 (quarter of a second))
			mBufferSize = mFreq * 5;
			// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
			mBufferSize -= (mBufferSize % 20);
			break;
		default:
			// Couldn't determine buffer format so log the error and default to mono
			Ogre::LogManager::getSingleton().logMessage("!!WARNING!! Could not determine buffer format!  Defaulting to MONO");

			mFormat = AL_FORMAT_MONO16;
			// Set BufferSize to 250ms (Frequency * 2 (16bit) divided by 4 (quarter of a second))
			mBufferSize = mFreq >> 1;
			// IMPORTANT : The Buffer Size must be an exact multiple of the BlockAlignment ...
			mBufferSize -= (mBufferSize % 2);
			break;
		}
	}

	void Sound::queueBuffers()
	{
		alSourceQueueBuffers(mSource, mNumBuffers, mBuffers);
		CheckError(alGetError(), "Failed to bind Buffer");
	}

	void Sound::unqueueBuffers()
	{
		/*
		** If the sound doesn't have a state yet it causes an
		** error when you try to unqueue the buffers! :S  In
		** order to get around this we stop the source even if
		** it wasn't playing.
		*/
		alSourceStop(mSource);
		CheckError(alGetError(), "Failed to stop sound");

		int queued;
		alGetSourcei(mSource, AL_BUFFERS_QUEUED, &queued);
		CheckError(alGetError(), "Failed to get Source");

		alSourceUnqueueBuffers(mSource, queued, mBuffers);
		CheckError(alGetError(), "Failed to unqueue Buffers");


		mBuffersQueued = false;
	}

	bool Sound::isStateCached() const
	{
		return mStateCached;
	}

	void Sound::setStateCached(bool stateCached)
	{
		mStateCached = stateCached;
	}

	void Sound::updateStateCache() const
	{
		// If we are not to use the cached state, in other words, if the cached state is
		// free to be used, modified and updated.
		if (!mStateCached)
		{
			// Perform a state cache update.
			alGetSourcei(mSource, AL_SOURCE_STATE, &mState);
			CheckError(alGetError(), "Failed to get State");
		}
	}

	const Ogre::String& Sound::getMovableType(void) const
	{
		return SoundFactory::FACTORY_TYPE_NAME;
	}

	const Ogre::AxisAlignedBox& Sound::getBoundingBox(void) const
	{
		// Null, Sounds are not visible
		static Ogre::AxisAlignedBox box;
		return box;
	}

	void Sound::_updateRenderQueue(Ogre::RenderQueue* queue)
	{
		// Sounds are not visible so do nothing
	}

	void Sound::_notifyAttached(Ogre::Node* parent, bool isTagPoint)
	{
		// Set the source not relative to the listener if it's attached to a node
		if (mSourceRelative)
		{
			mSourceRelative = false;
			if (mSource != AL_NONE)
			{
				alSourcei(mSource, AL_SOURCE_RELATIVE, AL_FALSE);
				CheckCondition(alGetError() == AL_NO_ERROR, 13, "Inalid Value");
			}
		}
		mParentNode = parent;
		_update();
	}

	void Sound::removeSoundSpectrumHandler(void)
	{
		if (nullptr != mSpectrumCallback)
		{
			delete mSpectrumCallback;
			mSpectrumCallback = nullptr;
		}
	}

	void Sound::analyseSpectrum(int arraySize, const Buffer& data)
	{		
		if (nullptr == mSpectrumParameter)
			return;

		mDataRead += arraySize;
	    // Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "spectrum dataread: " + Ogre::StringConverter::toString(mDataRead));

		for (unsigned short i = 0; i < arraySize; i++)
		{
			mSpectrumParameter->VUpoints[i] = static_cast<Ogre::Real>(mData[i]) / 85.0f;
		}

		mAudioProcessor->process(mCurrentSpectrumPos, mSpectrumParameter->VUpoints);

		const std::vector<Ogre::Real>& magnitudeSpectrum = mAudioProcessor->getMagnitudeSpectrum();
		memcpy(&mSpectrumParameter->amplitude[0], &magnitudeSpectrum[0], magnitudeSpectrum.size() * sizeof(Ogre::Real));

		const std::vector<Ogre::Real>& levelSpectrum = mAudioProcessor->getLevelSpectrum();
		memcpy(&mSpectrumParameter->level[0], &levelSpectrum[0], levelSpectrum.size() * sizeof(Ogre::Real));

		const std::vector<int>& frequencyList = mAudioProcessor->getFrequencyList();
		memcpy(&mSpectrumParameter->frequency[0], &frequencyList[0], frequencyList.size() * sizeof(int));

		const std::vector<Ogre::Real>& phaseList = mAudioProcessor->getPhaseList();
		memcpy(&mSpectrumParameter->phase[0], &phaseList[0], phaseList.size() * sizeof(Ogre::Real));

		mSpectrumCallback->execute(static_cast<Sound*>(this));	
	}

	//-----------------OgreAL::SoundFactory-----------------//

	SoundFactory::BufferInfo::BufferInfo() : reference(AL_NONE),
		refCount(0u)
	{

	}

	SoundFactory::BufferInfo::BufferInfo(BufferRef reference, unsigned int refCount) : reference(reference),
		refCount(refCount)
	{

	}

	SoundFactory::BufferInfo::BufferInfo(const BufferInfo& bufferInfo) : reference(bufferInfo.reference),
		refCount(bufferInfo.refCount)
	{

	}

	// Default sound factory name for ogre registration and by-name factory retrieval.
	Ogre::String SoundFactory::FACTORY_TYPE_NAME = "OgreAL_Sound";

	SoundFactory::SoundFactory()
	{

	}

	SoundFactory::~SoundFactory()
	{
		BufferMap::iterator bufferItr = mBufferMap.begin();
		while (bufferItr != mBufferMap.end())
		{
			alDeleteBuffers(1, &bufferItr->second.reference);
			++bufferItr;
		}

		mBufferMap.clear();
	}

	const Ogre::String& SoundFactory::getType(void) const
	{
		return FACTORY_TYPE_NAME;
	}


	Ogre::MovableObject* SoundFactory::createInstanceImpl(
		Ogre::IdType id, Ogre::ObjectMemoryManager* objectMemoryManager, Ogre::SceneManager* manager,
		const Ogre::NameValuePairList* params)
	{
		// Get the file name of the requested sound file to be sourced.
		Ogre::String fileName = params->find(SoundManager::SOUND_FILE)->second;
		// Loop flag.
		bool loop = Ogre::StringConverter::parseBool(params->find(SoundManager::LOOP_STATE)->second);
		// Are we streaming or loading the whole thing?
		bool stream = Ogre::StringConverter::parseBool(params->find(SoundManager::STREAM)->second);

		Ogre::String name = params->find("name")->second;
		// Check to see if we can just piggy back another buffer.
		if (!stream)
		{
			// Attempt to find the buffer by name.
			BufferMap::iterator bufferItr = mBufferMap.find(fileName);
			// If we have found the buffer.
			if (bufferItr != mBufferMap.end())
			{
				// We have this buffer loaded already!
				return new Sound(manager, (BufferRef)bufferItr->second.reference, name, fileName, loop);
			}
		}

		// By this stage we are either streaming and need to load a new copy of the file or
		// we are not streaming but do not have the buffer. Open the file.
		Ogre::ResourceGroupManager* groupManager = Ogre::ResourceGroupManager::getSingletonPtr();
		Ogre::String group = groupManager->findGroupContainingResource(fileName);
		Ogre::DataStreamPtr soundData = groupManager->openResource(fileName, group);

		// If we identify the file as an OGG sound, create an OGG instance.
		if (fileName.find(".ogg") != std::string::npos || fileName.find(".OGG") != std::string::npos)
		{
			return new OggSound(manager, name, soundData, loop, stream);

			// If we identify the file as a wave file. , create a WAV instance.
		}
		else if (fileName.find(".wav") != std::string::npos || fileName.find(".WAV") != std::string::npos)
		{
			return new WavSound(manager, name, soundData, loop, stream);

			// Unknown file type.
		}
		else
		{
			throw Ogre::Exception(Ogre::Exception::ERR_INVALIDPARAMS,
				"Sound file '" + fileName + "' is of an unsupported file type, ",
				"OgreAL::SoundManager::_createSound");
		}
	}

	void SoundFactory::destroyInstance(Ogre::MovableObject* obj)
	{
		delete obj;
	}

	void SoundFactory::_removeBufferRef(const Ogre::String& bufferName)
	{
		// Attempt to find the buffer requested.
		BufferMap::iterator bufferItr = mBufferMap.find(bufferName);
		// If the buffer information exists in our map.
		if (bufferItr != mBufferMap.end())
		{
			// The reference count is unsigned, we have to take care not to flip it.
			if (bufferItr->second.refCount > 0)
			{
				// Above zero. Buffer unused, decrement count.
				bufferItr->second.refCount--;
			}
			else
			{
				// That's not good! Reference count zero and we got a decrement =/.
				Ogre::LogManager::getSingleton().logMessage("OgreAL: Internal Error: Reference count decrement on zero buffer.");
			}

			// If our reference count is zero, delete the buffer.
			if (bufferItr->second.refCount == 0u)
			{
				// First delete it OpenAL style.
				alDeleteBuffers(1, &(bufferItr->second.reference));
				CheckError(alGetError(), "Failed to delete non-stream buffer with zero reference count.");
				// Then delete it out of our container.
				mBufferMap.erase(bufferItr);
			}

			// Oh ohes! Someone requested a buffer refcount removal without an entry.
		}
		else
		{
			Ogre::LogManager::getSingleton().logMessage("OgreAL: Internal Error: Reference count decrement on non-existent buffer.");
		}
	}

	void SoundFactory::_addBufferRef(const Ogre::String& bufferName, BufferRef buffer)
	{
		// Attempt to find the buffer.
		BufferMap::iterator bufferItr = mBufferMap.find(bufferName);
		// If we have found the buffer.
		if (bufferItr != mBufferMap.end())
		{
			// Then the reference in that buffer should be the same as that supplied!
			if (bufferItr->second.reference != buffer)
			{
				// Sweet jesus, something is horribly wrong.
				Ogre::LogManager::getSingleton().logMessage("OgreAL: Internal Error: Reference mismatch in reference count incrementation.");
			}

			// Cool, increment reference count.
			bufferItr->second.refCount++;

		}
		else
		{
			// No buffer reference by that name. Add it with an initial count of 1.
			mBufferMap[bufferName] = BufferInfo(buffer, 1u);
		}
	}

} // Namespace

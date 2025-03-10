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
#include "OgreALOggSound.h"

/*
** These next four methods are custom accessor functions to allow the Ogg Vorbis
** libraries to be able to stream audio data directly from an Ogre::DataStreamPtr
*/
size_t OgreALOggStreamRead(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	Ogre::DataStreamPtr dataStream = *reinterpret_cast<Ogre::DataStreamPtr*>(datasource);
	return dataStream->read(ptr, size);
}

int OgreALOggStreamSeek(void *datasource, ogg_int64_t offset, int whence)
{
	Ogre::DataStreamPtr dataStream = *reinterpret_cast<Ogre::DataStreamPtr*>(datasource);
	switch(whence)
	{
	case SEEK_SET:
		dataStream->seek(offset);
		break;
	case SEEK_END:
		dataStream->seek(dataStream->size() + 8000);
		// Falling through purposefully here
		break;
	case SEEK_CUR:
		dataStream->skip(offset);
		break;
	}

	return 0;
}

int OgreALOggStreamClose(void *datasource)
{
	return 0;
}

long OgreALOggStreamTell(void *datasource)
{
	Ogre::DataStreamPtr dataStream = *reinterpret_cast<Ogre::DataStreamPtr*>(datasource);
	return static_cast<long>(dataStream->tell());
}

/*
** End Custome Accessors
*/

namespace OgreAL {
	OggSound::OggSound(Ogre::SceneManager* sceneMgr, const Ogre::String& name, const Ogre::DataStreamPtr& soundStream, bool loop, bool stream) :
		Sound(sceneMgr, name, soundStream->getName(), stream)
	{
		try
		{
			mSoundStream = soundStream;

			// Set up custom accessors
			ov_callbacks callbacks;
			callbacks.close_func = OgreALOggStreamClose;
			callbacks.tell_func = OgreALOggStreamTell;
			callbacks.read_func = OgreALOggStreamRead;
			callbacks.seek_func = OgreALOggStreamSeek;

			// Open the Ogre::DataStreamPtr
			CheckCondition(ov_open_callbacks(&mSoundStream, &mOggStream, NULL, 0, callbacks) >= 0, 1, "Could not open Ogg stream.");

			mVorbisInfo = ov_info(&mOggStream, -1);
			unsigned long channels = mVorbisInfo->channels;
			mFreq = mVorbisInfo->rate;
			mChannels = mVorbisInfo->channels;
			mBPS = 16;
			mLoop = loop;

			calculateFormatInfo();

			mDataSize = mOggStream.end;

			mLengthInSeconds = ov_time_total(&mOggStream, -1);
			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "Length: " + Ogre::StringConverter::toString(mLengthInSeconds));

			generateBuffers();
			mBuffersLoaded = loadBuffers();
		}
		catch(Ogre::Exception e)
		{
			for(int i = 0; i < mNumBuffers; i++)
			{
				if (mBuffers[i] && alIsBuffer(mBuffers[i]) == AL_TRUE)
				{
					alDeleteBuffers(1, &mBuffers[i]);
					CheckError(alGetError(), "Failed to delete Buffer");
				}
			}

			ov_clear(&mOggStream);

			throw (e);
		}
	}

	OggSound::~OggSound()
	{
		ov_clear(&mOggStream);
	}

	bool OggSound::loadBuffers()
	{
		for(int i = 0; i < mNumBuffers; i++)
		{
			CheckCondition(AL_NONE != mBuffers[i], 13, "Could not generate buffer");
			Buffer buffer = bufferData(&mOggStream, mStream ? mBufferSize : 0);
			alBufferData(mBuffers[i], mFormat, &buffer[0], static_cast<Size>(buffer.size()), mFreq);
			CheckError(alGetError(), "Could not load buffer data");
		}

		return true;
	}

	bool OggSound::unloadBuffers()
	{
		if(mStream)
		{
			ov_time_seek(&mOggStream, 0);
			return false;
		}
		else
		{
			return true;
		}
	}

	void OggSound::setSecondOffset(Ogre::Real seconds)
	{
		if(seconds < 0) return;

		if(!mStream)
		{
			Sound::setSecondOffset(seconds);
		}
		else
		{
			bool wasPlaying = isPlaying();
			
			pause();
			ov_time_seek(&mOggStream, seconds);

			if(wasPlaying) play();
		}
	}

	Ogre::Real OggSound::getSecondOffset()
	{
		if(!mStream)
		{
			return Sound::getSecondOffset();
		}
		else
		{
			/*
			** We know that we are playing a buffer and that we have another buffer loaded.
			** We also know that each buffer is 1/4 of a second when full.
			** We can get the current offset in the OggStream which will be after both bufers
			** and subtract from that 1/4 of a second for the waiting buffer and 1/4 of a second
			** minus the offset into the current buffer to get the current overall offset.
			*/

			Ogre::Real oggStreamOffset = ov_time_tell(&mOggStream);
			Ogre::Real bufferOffset = Sound::getSecondOffset();

			Ogre::Real totalOffset = oggStreamOffset - (0.25 + (0.25 - bufferOffset));
			return totalOffset;
		}
	}

	void OggSound::enableSpectrumAnalysis(bool enable, int processingSize, int numberOfBands, MathWindows::WindowType windowType, 
		AudioProcessor::SpectrumPreparationType spectrumPreparationType, Ogre::Real smoothFactor)
	{
		if (true == enable)
		{
			/*if (processingSize < 1024)
			{
				processingSize = 1024;

				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "OgreALSound Warning: Note: Because of VSYNC which minimum dt is 16MS, a spectrum size of 512 will never work, "
					" because mTargetDeltaMS would result in 10MS, which cannot work! So minimum spectrum size is 1024, which will result in mTargetDeltaMS of 21MS!");
			}*/

			mSpectrumProcessingSize = processingSize;
			mSpectrumNumberOfBands = numberOfBands;
			mMathWindowType = windowType;
			mSpectrumPreparationType = spectrumPreparationType;

			const unsigned int arraySize = mSpectrumProcessingSize * 2;

			if (nullptr == mSpectrumParameter)
			{
				Ogre::ResourceGroupManager* groupManager = Ogre::ResourceGroupManager::getSingletonPtr();
				Ogre::String group = groupManager->findGroupContainingResource(mSoundStream->getName());
				mSpectrumSoundStream = groupManager->openResource(mSoundStream->getName(), group);

				/*auto locations = groupManager->listResourceFileInfo(group);
				Ogre::String filePathName;

				for (size_t i = 0; i < locations->size(); i++)
				{
					if (locations->at(i).filename == mSoundStream->getName())
					{
						filePathName = locations->at(i).archive->getName() + "/" + locations->at(i).filename;
						break;
					}
				}*/

				// Open the Ogre::DataStreamPtr
				// CheckCondition(ov_fopen(filePathName.c_str(), &mSpectrumOggStream) >= 0, 1, "Could not open Ogg spectrum stream.");
				
				// Set up custom accessors
				ov_callbacks callbacks;
				callbacks.close_func = OgreALOggStreamClose;
				callbacks.tell_func = OgreALOggStreamTell;
				callbacks.read_func = OgreALOggStreamRead;
				callbacks.seek_func = OgreALOggStreamSeek;

				// Open the Ogre::DataStreamPtr
				CheckCondition(ov_open_callbacks(&mSpectrumSoundStream, &mSpectrumOggStream, NULL, 0, callbacks) >= 0, 1, "Could not open Ogg spectrum stream.");

				// mBufferSize * 4 = samples per second
				mAudioProcessor = new AudioProcessor(static_cast<int>(arraySize), mFreq, mSpectrumNumberOfBands, mBufferSize * 4, mMathWindowType, mSpectrumPreparationType, smoothFactor);

				mSpectrumParameter = new SpectrumParameter;
				mSpectrumParameter->VUpoints.resize(arraySize, 0.0f);
				mSpectrumParameter->phase.resize(mSpectrumProcessingSize, 0.0f);
				mSpectrumParameter->frequency.resize(mSpectrumProcessingSize, 0.0f);
				mSpectrumParameter->amplitude.resize(mSpectrumProcessingSize, 0.0f);
				mSpectrumParameter->level.resize(mSpectrumNumberOfBands, 0.0f);
				mSpectrumParameter->actualSpectrumSize = mAudioProcessor->getLevelSpectrum().size();

				mLastTime = mTimer.getMilliseconds();

				// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "sumSamplings: " + Ogre::StringConverter::toString(sumSamplings));
				// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "mTargetDelta: " + Ogre::StringConverter::toString(mTargetDeltaSec));
			}
		}
		else
		{
			mSumDataRead = 0;
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
		}
	}

	bool OggSound::updateSound()
	{
		// Call the parent method to update the position
		Sound::updateSound();

		bool eof = false;

		if (mStream && (mSource != AL_NONE) && isPlaying())
		{
			// Update the stream
			int processed;

			alGetSourcei(mSource, AL_BUFFERS_PROCESSED, &processed);
			CheckError(alGetError(), "Failed to get source");

			while (processed--)
			{
				ALuint buffer;

				alSourceUnqueueBuffers(mSource, 1, &buffer);
				CheckError(alGetError(), "Failed to unqueue buffers");

				Buffer data = bufferData(&mOggStream, mBufferSize);

				if (!eof)
				{
					if (!data.empty())
					{
						alBufferData(buffer, mFormat, &data[0], static_cast<Size>(data.size()), mFreq);
					}
				}

				eof = (mOggStream.offset == mOggStream.end);

				alSourceQueueBuffers(mSource, 1, &buffer);
				CheckError(alGetError(), "Failed to queue buffers");

				if (eof && !mSpectrumParameter)
				{
					if (mLoop)
					{
						eof = false;
						ov_time_seek(&mOggStream, 0);
						if (mLoopedCallback)
							mLoopedCallback->execute(static_cast<Sound*>(this));
					}
					else
					{
						stop();
						if (mFinishedCallback)
							mFinishedCallback->execute(static_cast<Sound*>(this));
					}
				}
			}

			if (mSpectrumCallback)
			{
				const unsigned int arraySize = mSpectrumProcessingSize * 2;

				if (mFirstTimeReady)
				{
					if (mSpectrumSoundStream.isNull())
					{
						stop();
						return true;
					}

					unsigned int startOffset = mBufferSize / arraySize;
					ov_time_seek(&mSpectrumOggStream, startOffset * arraySize);
					mFirstTimeReady = false;
				}

				const Ogre::Real sumSamplingsPerSec = static_cast<Ogre::Real>(mBufferSize * 4.0f);
				const Ogre::Real samplesCount = sumSamplingsPerSec / static_cast<Ogre::Real>(arraySize * mChannels);
				Ogre::Real realTargetDeltaMS = (1.0f / samplesCount) * 1000.0f;
				mTargetDeltaMS = static_cast<unsigned int>(realTargetDeltaMS);

				Ogre::Real fraction;
				Ogre::Real intPart;
				fraction = modf(realTargetDeltaMS, &intPart);
				mFractionSum += fraction;

				if (mFractionSum >= 1.0f)
				{
					mTargetDeltaMS += 1;
					mFractionSum -= 1.0f;
				}

				unsigned long curtime = mTimer.getMilliseconds();
				int timediff = static_cast<int>(curtime - mLastTime);
				if (timediff < 0)
					timediff = 0;

				mTotalElapsedTime += static_cast<Ogre::Real>(timediff) * 0.001f;
				mCurrentSpectrumPos = mTotalElapsedTime;
				mLastTime = curtime;

				mRenderDelta += timediff;

				if (mRenderDelta >= mTargetDeltaMS)
				{
					mRenderDelta -= mTargetDeltaMS;  // Adjust to avoid drift

					if (mChannels > 1)
					{
						Buffer tempData = bufferDataSpectrum(&mSpectrumOggStream, arraySize * mChannels);
						mData.resize(tempData.size());

						int numProcessed = 0;
						Ogre::Real combinedChannelAverage = 0.0f;
						for (int i = 0; i < tempData.size(); i++)
						{
							combinedChannelAverage += tempData[i];

							if ((i + 1) % mChannels == 0)
							{
								mData[numProcessed] = combinedChannelAverage / mChannels;
								numProcessed++;
								combinedChannelAverage = 0.0f;
							}
						}
					}
					else
					{
						mData = bufferData(&mSpectrumOggStream, arraySize);
					}

					mDataRead += mData.size();

					bool spectrumEof = (mSpectrumOggStream.offset == mSpectrumOggStream.end);

					if (!spectrumEof && !mData.empty())
					{
						this->analyseSpectrum(arraySize, mData);
					}
					else
					{
						if (mLoop)
						{
							spectrumEof = false;
							mFirstTimeReady = true;
							eof = false;

							if (nullptr != mAudioProcessor)
							{
								mAudioProcessor->setProcessingSize(this->getSpectrumProcessingSize() * 2);
							}

							ov_time_seek(&mOggStream, 0);
							unsigned long loopResetTime = mTimer.getMilliseconds();
							ov_time_seek(&mSpectrumOggStream, 0);

							unsigned long timeSinceLastUpdate = loopResetTime - mLastTime;
							mTotalElapsedTime += static_cast<Ogre::Real>(timeSinceLastUpdate) * 0.001f;

							mCurrentSpectrumPos = 0;
							mLastTime = loopResetTime;
							mRenderDelta = 0;

							if (mLoopedCallback)
								mLoopedCallback->execute(static_cast<Sound*>(this));
						}
						else
						{
							stop();
							mTotalElapsedTime = 0.0f;
							if (mFinishedCallback)
								mFinishedCallback->execute(static_cast<Sound*>(this));
						}
					}
				}
			}
		}

		return !eof;
	}

	Buffer OggSound::bufferData(OggVorbis_File *oggVorbisFile, int size)
	{
		Buffer buffer;
		char *data = new char[mBufferSize];
		int section, sizeRead = 0;

		if(size == 0)
		{
			// Read the rest of the file
			do
			{
				sizeRead = ov_read(oggVorbisFile, data, mBufferSize, 0, 2, 1, &section);
				buffer.insert(buffer.end(), data, data + sizeRead);
			}while(sizeRead > 0);
		}
		else
		{
			// Read only what was asked for
			while(buffer.size() < size)
			{
				sizeRead = ov_read(oggVorbisFile, data, mBufferSize, 0, 2, 1, &section);
				if(sizeRead == 0) break;
				buffer.insert(buffer.end(), data, data + sizeRead);
			}
		}
		
		delete[] data;
		return buffer;
	}

	Buffer OggSound::bufferDataSpectrum(OggVorbis_File *oggVorbisFile, int size)
	{
		Buffer buffer;
		char *data = new char[size];
		int section, sizeRead = 0;

		if(size == 0)
		{
			// Read the rest of the file
			do
			{
				sizeRead = ov_read(oggVorbisFile, data, size, 0, 2, 1, &section);
				buffer.insert(buffer.end(), data, data + sizeRead);
			}while(sizeRead > 0);
		}
		else
		{
			// Read only what was asked for
			while(buffer.size() < size)
			{
				sizeRead = ov_read(oggVorbisFile, data, size, 0, 2, 1, &section);
				if(sizeRead == 0) break;
				buffer.insert(buffer.end(), data, data + sizeRead);
			}
		}
		
		delete[] data;
		return buffer;
	}
} // Namespace

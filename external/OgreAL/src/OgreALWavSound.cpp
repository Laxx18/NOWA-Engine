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
#include "OgreALWavSound.h"
#include "OgreALSoundManager.h"

namespace OgreAL {
	WavSound::WavSound(Ogre::SceneManager* sceneMgr, const Ogre::String& name, const Ogre::DataStreamPtr& soundStream, bool loop, bool stream) :
		Sound(sceneMgr, name, soundStream->getName(), stream)
	{
		try
		{
			mSoundStream = soundStream;
			mLoop = loop ? AL_TRUE : AL_FALSE;

			// buffers
			char magic[5];
			magic[4] = '\0';
			unsigned char buffer32[4];
			unsigned char buffer16[2];

			// check magic
			CheckCondition(mSoundStream->read(magic, 4) == 4, 13, "Cannot read wav file " + mFileName);
			CheckCondition(std::string(magic) == "RIFF", 13, "Wrong wav file format. (no RIFF magic): " + mFileName);

			// The next 4 bytes are the file size, we can skip this since we get the size from the DataStream
			mSoundStream->skip(4);
			mSize = static_cast<Size>(mSoundStream->size());

			// check file format
			CheckCondition(mSoundStream->read(magic, 4) == 4, 13, "Cannot read wav file " + mFileName);
			CheckCondition(std::string(magic) == "WAVE", 13, "Wrong wav file format. (no WAVE format): " + mFileName);

			// check 'fmt ' sub chunk (1)
			CheckCondition(mSoundStream->read(magic, 4) == 4, 13, "Cannot read wav file " + mFileName);
			CheckCondition(std::string(magic) == "fmt ", 13, "Wrong wav file format. (no 'fmt ' subchunk): " + mFileName);

			// read (1)'s size
			CheckCondition(mSoundStream->read(buffer32, 4) == 4, 13, "Cannot read wav file " + mFileName);
			unsigned long subChunk1Size = readByte32(buffer32);
			CheckCondition(subChunk1Size >= 16, 13, "Wrong wav file format. ('fmt ' chunk too small, truncated file?): " + mFileName);

			// check PCM audio format
			CheckCondition(mSoundStream->read(buffer16, 2) == 2, 13, "Cannot read wav file " + mFileName);
			unsigned short audioFormat = readByte16(buffer16);
			CheckCondition(audioFormat == 1, 13, "Wrong wav file format. This file is not a .wav file (audio format is not PCM): " + mFileName);

			// read number of channels
			CheckCondition(mSoundStream->read(buffer16, 2) == 2, 13, "Cannot read wav file " + mFileName);
			mChannels = readByte16(buffer16);

			// read frequency (sample rate)
			CheckCondition(mSoundStream->read(buffer32, 4) == 4, 13, "Cannot read wav file " + mFileName);
			mFreq = readByte32(buffer32);

			// skip 6 bytes (Byte rate (4), Block align (2))
			mSoundStream->skip(6);

			// read bits per sample
			CheckCondition(mSoundStream->read(buffer16, 2) == 2, 13, "Cannot read wav file " + mFileName);
			mBPS = readByte16(buffer16);

			// check 'data' sub chunk (2)
			CheckCondition(mSoundStream->read(magic, 4) == 4, 13, "Cannot read wav file " + mFileName);
			CheckCondition(std::string(magic) == "data" || std::string(magic) == "fact", 13,
				"Wrong wav file format. (no data subchunk): " + mFileName);

			// fact is an option section we don't need to worry about
			if (std::string(magic) == "fact")
			{
				mSoundStream->skip(8);

				// Now we shoudl hit the data chunk
				CheckCondition(mSoundStream->read(magic, 4) == 4, 13, "Cannot read wav file " + mFileName);
				CheckCondition(std::string(magic) == "data", 13, "Wrong wav file format. (no data subchunk): " + mFileName);
			}

			// The next four bytes are the size remaing of the file
			CheckCondition(mSoundStream->read(buffer32, 4) == 4, 13, "Cannot read wav file " + mFileName);
			mDataSize = readByte32(buffer32);
			mDataStart = mSoundStream->tell();

			mSoundStream->seek(mDataStart);
			if (nullptr != mSpectrumSoundStream)
			{
				mSoundStream->seek(mDataStart);
			}

			calculateFormatInfo();

			// mBufferSize is equal to 1/4 of a second of audio
			mLengthInSeconds = (Ogre::Real)mDataSize / ((Ogre::Real)mBufferSize * 4);
			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "Length: " + Ogre::StringConverter::toString(mLengthInSeconds));

			generateBuffers();

			mBuffersLoaded = loadBuffers();
			if (false == mBuffersLoaded)
			{
				throw Ogre::Exception(1, "Buffer could not be loaded for wav file: '" + name + "'", "OgreALWavSound");
			}
		}
		catch (Ogre::Exception e)
		{
			// If we have gone so far as to load the buffers, unload them.
			if (mBuffers != NULL)
			{
				for (int i = 0; i < mNumBuffers; i++)
				{
					if (mBuffers[i] && alIsBuffer(mBuffers[i]) == AL_TRUE)
					{
						alDeleteBuffers(1, &mBuffers[i]);
						CheckError(alGetError(), "Failed to delete Buffer");
					}
				}
			}

			// Prevent the ~Sound() destructor from double-freeing.
			mBuffers = NULL;
			// Propagate.
			throw (e);
		}
	}

	WavSound::~WavSound()
	{}

	bool WavSound::loadBuffers()
	{
		for(int i = 0; i < mNumBuffers; i++)
		{
			CheckCondition(AL_NONE != mBuffers[i], 13, "Could not generate buffer");
			Buffer buffer = bufferData(mSoundStream, mStream?mBufferSize:mDataSize);

			if (true == buffer.empty())
				return false;

			mSumDataRead += buffer.size();
			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "dataRead: " + Ogre::StringConverter::toString(mSumDataRead));

			alBufferData(mBuffers[i], mFormat, &buffer[0], static_cast<Size>(buffer.size()), mFreq);
			CheckError(alGetError(), "Could not load buffer data");
		}

		// Attention: Schrott OpenAL implementation: data must be buffered prior, else no sound will play because mSource will be AL_NONE = 0
		// But e.g. if mFreq = 48000 and stream is set to on, mNumBuffers = 2, so 78000 bytes are buffered, event the sound has not played yet
		// so spectrum analysis could never meet with sound bytes position, so start spectrum also at that bytes!

		return true;
	}

	bool WavSound::unloadBuffers()
	{
		mDataRead = 0;
		if(mStream)
		{
			mSoundStream->seek(mDataStart);
			if (nullptr != mSpectrumSoundStream)
			{
				mSpectrumSoundStream->seek(mDataStart);
			}
			return false;
		}
		else
		{
			return true;
		}
	}

	void WavSound::setSecondOffset(Ogre::Real seconds)
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

			// mBufferSize is 1/4 of a second
			size_t dataOffset = static_cast<size_t>(seconds * mBufferSize * 4);
			mSoundStream->seek(dataOffset + mDataStart);
			if (nullptr != mSpectrumSoundStream)
			{
				mSpectrumSoundStream->seek(dataOffset + mDataStart);
			}

			if(wasPlaying) play();
		}
	}

	Ogre::Real WavSound::getSecondOffset()
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

			// mBufferSize is 1/4 of a second
			Ogre::Real wavStreamOffset = (Ogre::Real)(mSoundStream->tell() - mDataStart) / (Ogre::Real)(mBufferSize * 4);
			Ogre::Real bufferOffset = Sound::getSecondOffset();

			Ogre::Real totalOffset = wavStreamOffset + (0.25 - bufferOffset);
			return totalOffset;
		}
	}

	void WavSound::enableSpectrumAnalysis(bool enable, int processingSize, int numberOfBands, MathWindows::WindowType windowType, 
		AudioProcessor::SpectrumPreparationType spectrumPreparationType, Ogre::Real smoothFactor)
	{
		if (true == enable)
		{
			if (processingSize < 1000)
			{
				processingSize = 1024;

				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "OgreALSound Warning: Note: Because of VSYNC which minimum dt is 16MS, a spectrum size of 512 will never work, "
					" because mTargetDeltaMS would result in 10MS, which cannot work! So minimum spectrum size is 1024, which will result in mTargetDeltaMS of 21MS!");
			}

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
					
				/*int dataOffset = (mBufferSize * 2) / arraySize;
				mSpectrumSoundStream->seek(dataOffset * arraySize);
				mDataRead = (dataOffset * arraySize);*/

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
  
	bool WavSound::updateSound()
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

			// Got through several channels:
			// https://forum.unity.com/threads/fft-how-to.253192/

			// Several channels:
			// https://medium.com/giant-scam/algorithmic-beat-mapping-in-unity-preprocessed-audio-analysis-d41c339c135a
			while (processed--)
			{
				// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "processed: " + Ogre::StringConverter::toString(processed));

				ALuint buffer;

				alSourceUnqueueBuffers(mSource, 1, &buffer);
				CheckError(alGetError(), "Failed to unqueue buffers");

				Buffer data = bufferData(mSoundStream, mBufferSize);

				if (false == eof)
				{
					mSumDataRead += data.size();
					if (false == data.empty())
					{
						alBufferData(buffer, mFormat, &data[0], static_cast<Size>(data.size()), mFreq);
					}
					// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "dataRead: " + Ogre::StringConverter::toString(mSumDataRead));
				}
				eof = mSoundStream->eof();

				/*if (true == eof)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "eof: " + Ogre::StringConverter::toString(eof));
				}*/

				alSourceQueueBuffers(mSource, 1, &buffer);
				CheckError(alGetError(), "Failed to queue buffers");

				if (true == eof && nullptr == mSpectrumCallback)
				{
					if (mLoop)
					{
						eof = false;
						mSoundStream->seek(mDataStart);
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

			if (nullptr != mSpectrumCallback)
			{

				const unsigned int arraySize = mSpectrumProcessingSize * 2;

				// Since the sound must already be buffered and has x-samples in buffer, the spectrum would start always behind the sound, so adjust the offset
				if (true == mFirstTimeReady)
				{
					if (mSpectrumSoundStream.isNull())
					{
						stop();
						return true;
					}
					// Char to float max test
					// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "limit min: " + Ogre::StringConverter::toString(std::numeric_limits<char>::min()));
					// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "limit max: " + Ogre::StringConverter::toString(std::numeric_limits<char>::max()));
					// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "limit minmax: " + Ogre::StringConverter::toString(Ogre::Math::Abs(std::numeric_limits<char>::min()) + std::numeric_limits<char>::max()));
					// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "limit float max: " + Ogre::StringConverter::toString((Ogre::Math::Abs(std::numeric_limits<char>::min()) + std::numeric_limits<char>::max()) / 127.5f /* 255.0f*/));
					unsigned int startOffset = /*mChannels **/ mBufferSize / arraySize;
					mSpectrumSoundStream->seek(startOffset * arraySize);
					mFirstTimeReady = false;
				}

				// Note: Because of VSYNC which minimum dt is 16MS, a spectrum size of 512 will never work, because
				// mTargetDeltaMS would result in 10MS, which cannot work!
				// So minimum spectrum size is 1024, which will result in mTargetDeltaMS of 21MS!
				// Experiment: Setting VSYNC to false, will work, as long as the scene is performant!

				const Ogre::Real sumSamplingsPerSec = static_cast<Ogre::Real>(mBufferSize * 4.0f);

				const Ogre::Real samplesCount = sumSamplingsPerSec / static_cast<Ogre::Real>(arraySize * mChannels);

				// realTargetDeltaMS could be 21,33 MS, but mTargetDeltaMS is just 21, so fraction is 0.33, which must be added up
				Ogre::Real realTargetDeltaMS = (1.0f / samplesCount) * 1000.0f;

				mTargetDeltaMS = static_cast<unsigned int>(realTargetDeltaMS);
				Ogre::Real fraction;
				Ogre::Real intPart;
				fraction = modf (realTargetDeltaMS, &intPart);

				mFractionSum += fraction;
				// Ogre::LogManager::getSingletonPtr()->logMessage("mFractionSum: " + Ogre::StringConverter::toString(mFractionSum));
				// Ogre::LogManager::getSingletonPtr()->logMessage("intPart: " + Ogre::StringConverter::toString(intPart));

				if (mFractionSum >= 1.0f)
				{
					mTargetDeltaMS += 1;
					mFractionSum -= 1.0f;
				}

				unsigned long curtime = mTimer.getMilliseconds();
				int timediff = static_cast<int>(curtime - mLastTime);
				if (timediff < 0)
					timediff = 0;

				mCurrentSpectrumPos += static_cast<Ogre::Real>(timediff) * 0.001f;

				mLastTime = curtime;

				// add the time difference. E.g. for 100 fps timediff each 10 times 1
				mRenderDelta += timediff;
				
				// Ogre::LogManager::getSingletonPtr()->logMessage("mRenderDelta: " + Ogre::StringConverter::toString(mRenderDelta));

				if (mRenderDelta >= mTargetDeltaMS)
				{
					mRenderDelta = mRenderDelta % mTargetDeltaMS;

					// Ogre::LogManager::getSingletonPtr()->logMessage("mUpdateDeltaSec after: " + Ogre::StringConverter::toString(mUpdateDeltaSec));

					if (mChannels > 1)
					{
						Buffer tempData = bufferData(mSpectrumSoundStream, arraySize * mChannels);
						mData.resize(arraySize);

						int numProcessed = 0;
						Ogre::Real combinedChannelAverage = 0.0f;
						for (int i = 0; i < tempData.size(); i++)
						{
							combinedChannelAverage += tempData[i];

							// Each time we have processed all channels samples for a point in time, we will store the average of the channels combined
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
						mData = bufferData(mSpectrumSoundStream, arraySize);
					}

					bool spectrumEof = mSpectrumSoundStream->eof();

					// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "spectrumEof: " + Ogre::StringConverter::toString(spectrumEof));

					// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "mCurrentSpectrumPos: " + Ogre::StringConverter::toString(mCurrentSpectrumPos));

					if (false == spectrumEof && mData.size() > 0)
					{
						this->analyseSpectrum(arraySize, mData);
					}
					else
					{
						// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "spectrumEof: " + Ogre::StringConverter::toString(spectrumEof));
						if (mLoop)
						{
							spectrumEof = false;
							mSpectrumSoundStream->seek(mDataStart);
							mFirstTimeReady = true;
							eof = false;
							mSoundStream->seek(mDataStart);
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
			}
		}

		return !eof;
	}

	Buffer WavSound::bufferData(Ogre::DataStreamPtr dataStream, int size)
	{
		// Ogre::MemoryDataStream chunk(dataStream); 

		size_t bytes;
		std::vector<char> data;
		char *array = new char[size];

		while(data.size() != size)
		{
			// Read up to a buffer's worth of decoded sound data
			bytes = dataStream->read(array, size);

			if (bytes <= 0)
				break;

			if (data.size() + bytes > size)
				bytes = size - data.size();

			// Append to end of buffer
			data.insert(data.end(), array, array + bytes);
		}

		delete []array;
		array = NULL;

		return data;
	}

} // Namespace

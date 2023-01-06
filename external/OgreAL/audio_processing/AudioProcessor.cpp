#include "AudioProcessor.h"
#include <assert.h>
#include <complex>
#include "OgreLogManager.h"
#include "OgreStringConverter.h"

namespace OgreAL
{

	AudioProcessor::AudioProcessor(int processingSize, int frequency, int numberOfBands, int sumSamplingsPerSec, MathWindows::WindowType windowType, 
		SpectrumPreparationType spectrumPreparationType, Ogre::Real smoothFactor) noexcept
		: processingSize(processingSize),
		samplingFrequency(frequency),
		numberOfBands(numberOfBands),
		sumSamplingsPerSec(sumSamplingsPerSec),
		windowType(windowType),
		spectrumPreparationType(spectrumPreparationType),
		smoothFactor(smoothFactor),
		kissFFT(nullptr),
		spectrumPreparation(nullptr)
	{
		this->setProcessingSize(this->processingSize);
	}

	AudioProcessor::~AudioProcessor()
	{
		if (nullptr != this->kissFFT)
		{
			free(this->kissFFT);
			this->kissFFT = nullptr;
			this->kissFFTOut.clear();
			this->kissFFTIn.clear();
		}
		if (nullptr != this->spectrumPreparation)
		{
			delete this->spectrumPreparation;
			this->spectrumPreparation = nullptr;
		}
	}

	void AudioProcessor::setProcessingSize(int processingSize) noexcept
	{
		this->processingSize = processingSize;
		this->bufferData.resize(this->processingSize);
		this->windowFunction = MathWindows::generateWindow(this->processingSize, windowType);

		this->fftReal.resize(this->processingSize);
		this->fftImag.resize(this->processingSize);
		this->magnitudeSpectrum.resize(this->processingSize / 2);
		this->levelSpectrum.resize(this->numberOfBands);

		this->initializeFFT();

		this->initializeBeatDetector();
	}

	void AudioProcessor::setSamplingFrequency(int frequency) noexcept
	{
		this->samplingFrequency = frequency;
	}

	int AudioProcessor::getProcessingSize(void) noexcept
	{
		return this->processingSize;
	}

	int AudioProcessor::getSamplingFrequency(void) noexcept
	{
		return this->samplingFrequency;
	}

	inline Ogre::Real AudioProcessor::mapValue(Ogre::Real valueToMap, Ogre::Real targetMin, Ogre::Real targetMax) noexcept
	{
		return targetMin + valueToMap * (targetMax - targetMin);
	}

	inline Ogre::Real AudioProcessor::mapValue(Ogre::Real valueToMap, Ogre::Real sourceMin, Ogre::Real sourceMax, Ogre::Real targetMin, Ogre::Real targetMax) noexcept
	{
		return targetMin + ((targetMax - targetMin) * (valueToMap - sourceMin)) / (sourceMax - sourceMin);
	}

	inline Ogre::Real AudioProcessor::amplitudeToDecibels(Ogre::Real amplitude) noexcept
	{
		return amplitude > Ogre::Real() ? std::max (Ogre::Math::NEG_INFINITY, static_cast<Ogre::Real> (std::log10 (amplitude)) * Ogre::Real (20.0f)) : Ogre::Math::NEG_INFINITY;
	}
 
	inline Ogre::Real AudioProcessor::decibelsToAmplitude(Ogre::Real decibels) noexcept
	{
		return pow(10.0f, decibels / 20.0f);
	}

	inline Ogre::Real AudioProcessor::limit(Ogre::Real lowerValue, Ogre::Real upperValue, Ogre::Real limitValue) noexcept
	{
		return limitValue < lowerValue ? lowerValue : (upperValue < limitValue ? upperValue : limitValue);
	}

	void AudioProcessor::initializeFFT(void)
	{
		if (nullptr != this->kissFFT)
		{
			free(this->kissFFT);
			this->kissFFT = nullptr;
			this->kissFFTOut.clear();
			this->kissFFTIn.clear();
		}

		this->kissFFTIn.resize(this->processingSize);
		this->kissFFTOut.resize(this->processingSize);
		this->kissFFT = kiss_fft_alloc(this->processingSize, 0, 0, 0);

		if (SpectrumPreparationType::RAW == this->spectrumPreparationType)
		{
			this->spectrumPreparation = new SpectrumPreparationRaw();
		}
		else if (SpectrumPreparationType::LOGARITHMIC == this->spectrumPreparationType)
		{
			this->spectrumPreparation = new SpectrumPreparationLogarithm();
		}
		else
		{
			this->spectrumPreparation = new SpectrumPreparationLinear();
		}

		this->frequencyList.resize(this->processingSize / 2, 0);
		this->phaseList.resize(this->processingSize / 2, 0.0f);

		// Note: Its possible that the actualSpectrumSize is different (less) from numberOfBands, e.g. when using logarithm approach and a bandsize of 20, the actual spectrum size is 11
		int actualSpectrumSize = this->spectrumPreparation->init(this->processingSize, this->numberOfBands, this->samplingFrequency, this->smoothFactor);
		this->levelSpectrum.resize(actualSpectrumSize, 0.0f);
	}

	void AudioProcessor::initializeBeatDetector()
	{
		int bandSize = this->samplingFrequency / this->processingSize;

		this->beatDetectorBandLimits.clear();
		this->beatDetectorBandLimits.reserve((AudioProcessor::SpectrumArea::EXTREMELY_HIGH_END + AudioProcessor::Instrument::MAX_VALUE - 1) * 2); // bass + lowMidRange * 2

		// 0 DEEP_BASS 20hz-40hz
		this->beatDetectorBandLimits.push_back(20 / bandSize);
		this->beatDetectorBandLimits.push_back(40 / bandSize);

		// 1 LOW_BASS 40hz-80hz
		this->beatDetectorBandLimits.push_back(40 / bandSize);
		this->beatDetectorBandLimits.push_back(80 / bandSize);

		// 2 MID_BASS 80hz-160hz
		this->beatDetectorBandLimits.push_back(80 / bandSize);
		this->beatDetectorBandLimits.push_back(160 / bandSize);

		// 3 UPPER_BASS 20hz-300hz
		this->beatDetectorBandLimits.push_back(160 / bandSize);
		this->beatDetectorBandLimits.push_back(300 / bandSize);

		// 4 LOWER_MIDRANGE 300hz-600hz
		this->beatDetectorBandLimits.push_back(300 / bandSize);
		this->beatDetectorBandLimits.push_back(600 / bandSize);

		// 5 MIDDLE_MIDRANGE 600hz-1200hz
		this->beatDetectorBandLimits.push_back(600 / bandSize);
		this->beatDetectorBandLimits.push_back(1200 / bandSize);

		// 6 UPPER_MIDRANGE 1200hz-2400hz
		this->beatDetectorBandLimits.push_back(1200 / bandSize);
		this->beatDetectorBandLimits.push_back(2400 / bandSize);

		// 7 PRESENCE_RANGE 2400hz-5000hz
		this->beatDetectorBandLimits.push_back(2400 / bandSize);
		this->beatDetectorBandLimits.push_back(5000 / bandSize);

		// 8 HIGH_END 5000hz-10000hz
		this->beatDetectorBandLimits.push_back(5000 / bandSize);
		this->beatDetectorBandLimits.push_back(10000 / bandSize);

		// 9 EXTREMELY_HIGH_END 10000hz-20000hz
		this->beatDetectorBandLimits.push_back(10000 / bandSize);
		this->beatDetectorBandLimits.push_back(20000 / bandSize);
																													
		// 10 KICK_DRUM 60hz-130hz
		this->beatDetectorBandLimits.push_back(60 / bandSize);
		this->beatDetectorBandLimits.push_back(130 / bandSize);

		// 11 SNARE_DRUM 301hz-750hz
		this->beatDetectorBandLimits.push_back(301 / bandSize);
		this->beatDetectorBandLimits.push_back(750 / bandSize);

		this->fftHistoryBeatDetector.clear();

		this->spectrumDivision.resize(AudioProcessor::SpectrumArea::EXTREMELY_HIGH_END + AudioProcessor::Instrument::MAX_VALUE - 1, false);
	}

	void AudioProcessor::detectBeat(std::vector<Ogre::Real>& spectrum, std::vector<Ogre::Real>& averageSpectrum)
	{
		// Orientation on: https://www.parallelcube.com/2018/03/30/beat-detection-algorithm/

		// Only read / display half of the buffer typically for analysis 
		// as the 2nd half is usually the same data reversed due to the nature of the way FFT works.
		const int length = this->processingSize / 2;

		if (length > 0)
		{
			int numBands = this->beatDetectorBandLimits.size() / 2;

			for (int numBand = 0; numBand < numBands; ++numBand)
			{
				int bandBoundIndex = numBand * 2;
				for (int indexFFT = this->beatDetectorBandLimits[bandBoundIndex]; indexFFT < this->beatDetectorBandLimits[bandBoundIndex + 1]; ++indexFFT)
				{
					spectrum[numBand] += this->magnitudeSpectrum[indexFFT];
						// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "numBand: " + Ogre::StringConverter::toString(numBand)
						// + " indexFFT: " + Ogre::StringConverter::toString(indexFFT));
				}
				spectrum[numBand] /= (this->beatDetectorBandLimits[bandBoundIndex + 1] - this->beatDetectorBandLimits[bandBoundIndex]);
			}

			if (this->fftHistoryBeatDetector.size() > 0)
			{
				fillAverageSpectrum(averageSpectrum, numBands, this->fftHistoryBeatDetector);

				std::vector<Ogre::Real> varianceSpectrum(numBands, 0.0f);
				fillVarianceSpectrum(varianceSpectrum, numBands, this->fftHistoryBeatDetector, averageSpectrum);

				this->spectrumDivision[DEEP_BASS] = (spectrum[DEEP_BASS] - 0.005f) > beatThreshold(varianceSpectrum[DEEP_BASS]) * averageSpectrum[DEEP_BASS];
				this->spectrumDivision[LOW_BASS] = (spectrum[LOW_BASS] - 0.005f) > beatThreshold(varianceSpectrum[LOW_BASS]) * averageSpectrum[LOW_BASS];
				this->spectrumDivision[MID_BASS] = (spectrum[MID_BASS] - 0.005f) > beatThreshold(varianceSpectrum[MID_BASS]) * averageSpectrum[MID_BASS];
				this->spectrumDivision[UPPER_BASS] = (spectrum[UPPER_BASS] - 0.005f) > beatThreshold(varianceSpectrum[UPPER_BASS]) * averageSpectrum[UPPER_BASS];
				this->spectrumDivision[LOWER_MIDRANGE] = (spectrum[LOWER_MIDRANGE] - 0.0005f) > beatThreshold(varianceSpectrum[LOWER_MIDRANGE]) * averageSpectrum[LOWER_MIDRANGE];
				this->spectrumDivision[MIDDLE_MIDRANGE] = (spectrum[MIDDLE_MIDRANGE] - 0.0005f) > beatThreshold(varianceSpectrum[MIDDLE_MIDRANGE]) * averageSpectrum[MIDDLE_MIDRANGE];
				this->spectrumDivision[UPPER_MIDRANGE] = (spectrum[UPPER_MIDRANGE] - 0.0005f) > beatThreshold(varianceSpectrum[UPPER_MIDRANGE]) * averageSpectrum[UPPER_MIDRANGE];
				this->spectrumDivision[PRESENCE_RANGE] = (spectrum[PRESENCE_RANGE] - 0.0005f) > beatThreshold(varianceSpectrum[PRESENCE_RANGE]) * averageSpectrum[PRESENCE_RANGE];
				this->spectrumDivision[HIGH_END] = (spectrum[HIGH_END] - 0.0005f) > threshold(-15.0f, varianceSpectrum[HIGH_END], 1.55f) * averageSpectrum[HIGH_END];
				this->spectrumDivision[EXTREMELY_HIGH_END] = (spectrum[EXTREMELY_HIGH_END] - 0.0005f) > threshold(-15.0f, varianceSpectrum[EXTREMELY_HIGH_END], 1.55f) * averageSpectrum[EXTREMELY_HIGH_END];

				this->spectrumDivision[KICK_DRUM] = (spectrum[KICK_DRUM] - 0.05f) > beatThreshold(varianceSpectrum[KICK_DRUM]) * averageSpectrum[KICK_DRUM];
				this->spectrumDivision[SNARE_DRUM] = (spectrum[SNARE_DRUM] - 0.005f) > beatThreshold(varianceSpectrum[SNARE_DRUM]) * averageSpectrum[SNARE_DRUM];
			}

			std::vector<Ogre::Real> fftResult;
			fftResult.reserve(numBands);
			for (int index = 0; index < numBands; ++index)
			{
				fftResult.push_back(spectrum[index]);
			}

			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "processingSize: " + Ogre::StringConverter::toString(processingSize));

			if (this->fftHistoryBeatDetector.size() >= this->samplingFrequency / this->processingSize)
			{
				this->fftHistoryBeatDetector.pop_front();
			}

			this->fftHistoryBeatDetector.push_back(fftResult);
		}
	}

	void AudioProcessor::fillAverageSpectrum(std::vector<Ogre::Real>& averageSpectrum, int numBands, const FFTHistoryContainer& fftHistory)
	{
		for (FFTHistoryContainer::const_iterator it = fftHistory.cbegin(); it != fftHistory.cend(); ++it)
		{
			const std::vector<Ogre::Real>& fftResult = *it;

			for (int index = 0; index < fftResult.size(); ++index)
			{
				averageSpectrum[index] += fftResult[index];
			}
		}

		for (int index = 0; index < numBands; ++index)
		{
			averageSpectrum[index] /= (fftHistory.size());
		}
	}

	void AudioProcessor::fillVarianceSpectrum(std::vector<Ogre::Real>& varianceSpectrum, int numBands, const FFTHistoryContainer& fftHistory, const std::vector<Ogre::Real>& averageSpectrum)
	{
		for (FFTHistoryContainer::const_iterator it = fftHistory.cbegin(); it != fftHistory.cend(); ++it)
		{
			const std::vector<Ogre::Real>& fftResult = *it;

			for (int index = 0; index < fftResult.size(); ++index)
			{
				varianceSpectrum[index] += (fftResult[index] - averageSpectrum[index]) * (fftResult[index] - averageSpectrum[index]);
			}
		}

		for (int index = 0; index < numBands; ++index)
		{
			varianceSpectrum[index] /= (fftHistory.size());
		}
	}

	Ogre::Real AudioProcessor::beatThreshold(Ogre::Real variance)
	{
		return -15.0f * variance + 1.55f;
	}

	Ogre::Real AudioProcessor::threshold(Ogre::Real firstValue, Ogre::Real variance, Ogre::Real offset)
	{
		return firstValue * variance + offset;
	}

	void AudioProcessor::process(Ogre::Real timerSeconds, const std::vector<Ogre::Real>& data) noexcept
	{
		assert(data.size() == this->bufferData.size());
		std::copy(data.begin(), data.end(), this->bufferData.begin());

		Ogre::Real windowSum = 0.0f;
		for (int i = 0; i < this->processingSize; i++)
		{
			this->kissFFTIn[i].r = this->bufferData[i] * this->windowFunction[i];
			this->kissFFTIn[i].i = 0.0;

			windowSum += this->windowFunction[i];
		}

		kiss_fft(this->kissFFT, &this->kissFFTIn[0], &this->kissFFTOut[0]);

		for (int i = 0; i < this->processingSize; i++)
		{
			this->fftReal[i] = this->kissFFTOut[i].r;
			this->fftImag[i] = this->kissFFTOut[i].i;
		}

		// processingSize == arraySize
		const int maxSize = this->processingSize;
		const int nyquist = (this->processingSize / 2);
		
		Ogre::Real normalizer = 2.0f / windowSum;
		int j = 0;

		constexpr Ogre::Real mindB = -100.0f;
		constexpr Ogre::Real maxdB = 0.0f;
		constexpr Ogre::Real damping = 0.8f;


		for (int i = 0; i < nyquist; i++)
		{
			Ogre::Real re = this->fftReal[i];
			Ogre::Real im = this->fftImag[i];

			this->phaseList[j] = atan2(im, re) + Ogre::Math::PI / 2;
			if (this->phaseList[j] > Ogre::Math::PI)
			{
				this->phaseList[j] -= 2.0f * Ogre::Math::PI; // cos to sin
			}
			this->phaseList[j] = this->phaseList[j] *  180.0f / Ogre::Math::PI; // rad to deg
			this->frequencyList[j] = (this->sumSamplingsPerSec * i) / maxSize;
			++j;

			const Ogre::Real limitValue = 0.001f;
			const Ogre::Real absmin = limitValue * limitValue * this->processingSize * this->processingSize;
			Ogre::Real abs = (re * re) + (im * im);

			//if (abs < absmin)
			//	continue; // throw low harmonics away

			// this->magnitudeSpectrum[i] = Ogre::Math::Sqrt(abs) * normalizer;

			// this->magnitudeSpectrum[i] = 2.0f * Ogre::Math::Sqrt(abs) / static_cast<Ogre::Real>(this->processingSize);

			// Note: 20 * log10(Ogre::Math::Sqrt(abs)) is the same as 10 * log10(abs) (without root!)?
			this->magnitudeSpectrum[i] = /*2.0f **/ Ogre::Math::Sqrt(abs) / static_cast<Ogre::Real>(nyquist);

			
			// convert to decibel and normalize (0 silent and 1 max noise)
			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "fftDataIndex: " + Ogre::StringConverter::toString(fftDataIndex));
			const auto level = mapValue(limit(mindB, maxdB, amplitudeToDecibels(this->magnitudeSpectrum[i]) - (amplitudeToDecibels(static_cast<Ogre::Real>(nyquist)))), mindB, maxdB, 0.0f, 1.0f);
			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "level: " + Ogre::StringConverter::toString(level));

			// this->magnitudeSpectrum[i] = (damping) * this->magnitudeSpectrum[i] + (1.0f - damping) * level;  
			this->magnitudeSpectrum[i] = level;          
		}

		//for (int i = 0; i < this->processingSize / 2; i++)
		//{
		//	// this->magnitudeSpectrum[i] = sqrt((this->fftReal[i] * this->fftReal[i]) + (this->fftImag[i] * this->fftImag[i]));

		//	this->magnitudeSpectrum[i] *= Ogre::Math::Abs(this->fftReal[i]) * 2.0f / windowSum;
		//}

		std::vector<Ogre::Real> spectrum(this->processingSize / 2, 0.0f);
		std::vector<Ogre::Real> averageSpectrum(this->processingSize / 2, 0.0f);

		

		this->levelSpectrum = this->spectrumPreparation->getSpectrum(this->magnitudeSpectrum);

		this->detectBeat(this->levelSpectrum, averageSpectrum);

		//// Find max volume
		//auto maxIterator = std::max_element(&this->levelSpectrum[0], &this->levelSpectrum[this->levelSpectrum.size() - 1]);
		//float maxVol = *maxIterator;
 
		//// Normalize
		//if (maxVol != 0)
		//  std::transform(&this->levelSpectrum[0], &this->levelSpectrum[this->levelSpectrum.size() - 1], &this->levelSpectrum[0], [maxVol] (float dB) -> float { return dB / maxVol; });

	}

	const std::vector<Ogre::Real>& AudioProcessor::getMagnitudeSpectrum(void) noexcept
	{
		return this->magnitudeSpectrum;
	}

	const std::vector<Ogre::Real>& AudioProcessor::getLevelSpectrum(void) noexcept
	{
		return this->levelSpectrum;
	}

	const std::vector<int>& AudioProcessor::getFrequencyList(void) noexcept
	{
		return this->frequencyList;
	}

	const std::vector<Ogre::Real>& AudioProcessor::getPhaseList(void) noexcept
	{
		return this->phaseList;
	}

	bool AudioProcessor::isSpectrumArea(SpectrumArea spectrumArea) const noexcept
	{
		return this->spectrumDivision[spectrumArea];
	}

	bool AudioProcessor::isInstrument(Instrument instrument) const noexcept
	{
		return this->spectrumDivision[instrument];
	}

};
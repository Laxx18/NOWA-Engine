#include "AudioProcessor.h"
#include <assert.h>
#include <complex>
#include "OgreLogManager.h"
#include "OgreStringConverter.h"
#include <numeric>
#include <array>

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
		spectrumPreparation(nullptr),
		fftHistoryMaxSize(20),
		previousEnergy(0.0f),
		firstFrame(true)
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
		this->magnitudeSpectrum.clear();
		this->magnitudeSpectrum.resize(this->processingSize / 2);
		this->levelSpectrum.clear();
		this->levelSpectrum.resize(this->numberOfBands);
		this->previousEnergy = 0.0f;
		this->firstFrame = true;
		this->rollingWindows.clear();
		this->beatDetectorBandLimits.clear();
		this->previousSmoothedValues.clear();
		this->previousDetection.clear();
		this->fftHistoryBeatDetector.clear();

		this->frequencyList.clear();
		this->spectrumDivision.clear();
		this->phaseList.clear();

		this->kissFFTIn.clear();
		this->kissFFTOut.clear();

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

		if (nullptr != this->spectrumPreparation)
		{
			delete this->spectrumPreparation;
			this->spectrumPreparation = nullptr;
		}

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
		// processingSize: 2048, numberOfBands: 30, smoothFactor: 0
		const int actualSpectrumSize = this->spectrumPreparation->init(this->processingSize, this->numberOfBands, this->samplingFrequency, this->smoothFactor);
		this->levelSpectrum.resize(actualSpectrumSize, 0.0f);
	}

	void AudioProcessor::initializeBeatDetector()
	{
		const int bandSize = this->samplingFrequency / this->processingSize;

		this->beatDetectorBandLimits.clear();
		this->beatDetectorBandLimits.reserve((AudioProcessor::SpectrumArea::HI_HAT + AudioProcessor::SpectrumArea::MAX_VALUE - 1) * 2); // bass + lowMidRange * 2

		// Frequency ranges (adjusted for better separation)
		this->beatDetectorBandLimits.push_back(20 / bandSize);  // 0 DEEP_BASS 20Hz-40Hz (sub-bass)
		this->beatDetectorBandLimits.push_back(40 / bandSize);

		this->beatDetectorBandLimits.push_back(40 / bandSize);  // 1 LOW_BASS 40Hz-80Hz
		this->beatDetectorBandLimits.push_back(80 / bandSize);

		this->beatDetectorBandLimits.push_back(80 / bandSize);  // 2 MID_BASS 80Hz-160Hz
		this->beatDetectorBandLimits.push_back(160 / bandSize);

		this->beatDetectorBandLimits.push_back(60 / bandSize);  // 3 KICK_DRUM 60Hz-130Hz (separate detection)
		this->beatDetectorBandLimits.push_back(130 / bandSize);

		this->beatDetectorBandLimits.push_back(160 / bandSize); // 4 UPPER_BASS 160Hz-300Hz
		this->beatDetectorBandLimits.push_back(300 / bandSize);

		this->beatDetectorBandLimits.push_back(300 / bandSize); // 5 LOWER_MIDRANGE 300Hz-600Hz
		this->beatDetectorBandLimits.push_back(600 / bandSize);

		this->beatDetectorBandLimits.push_back(301 / bandSize); // 6 SNARE_DRUM 300Hz-750Hz (snare)
		this->beatDetectorBandLimits.push_back(750 / bandSize);

		this->beatDetectorBandLimits.push_back(600 / bandSize); // 7 MIDDLE_MIDRANGE 600Hz-1200Hz
		this->beatDetectorBandLimits.push_back(1200 / bandSize);

		this->beatDetectorBandLimits.push_back(1200 / bandSize); // 8 UPPER_MIDRANGE 1200Hz-2400Hz
		this->beatDetectorBandLimits.push_back(2400 / bandSize);

		this->beatDetectorBandLimits.push_back(2400 / bandSize); // 9 PRESENCE_RANGE 2400Hz-5000Hz
		this->beatDetectorBandLimits.push_back(5000 / bandSize);

		this->beatDetectorBandLimits.push_back(5000 / bandSize); // 10 HIGH_END 5000Hz-10000Hz
		this->beatDetectorBandLimits.push_back(10000 / bandSize);

		this->beatDetectorBandLimits.push_back(10000 / bandSize); // 11 EXTREMELY_HIGH_END 10000Hz-20000Hz
		this->beatDetectorBandLimits.push_back(20000 / bandSize);

		this->beatDetectorBandLimits.push_back(7000 / bandSize); // 12 HI_HAT 7000Hz-12000Hz
		this->beatDetectorBandLimits.push_back(12000 / bandSize);

		// Adding additional ranges for other percussion or instrumentation if needed
		// e.g., toms, cymbals, hi-hats, etc.
		// ...

		this->fftHistoryBeatDetector.clear();

		this->spectrumDivision.resize(AudioProcessor::SpectrumArea::HI_HAT + AudioProcessor::SpectrumArea::MAX_VALUE - 1, false);
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

		const int maxSize = this->processingSize;
		const int nyquist = (this->processingSize / 2);

		Ogre::Real normalizer = 2.0f / windowSum;
		int j = 0;

		constexpr Ogre::Real mindB = -100.0f;
		constexpr Ogre::Real maxdB = 0.0f;

		for (int i = 0; i < nyquist; i++)
		{
			const Ogre::Real re = this->fftReal[i];
			const Ogre::Real im = this->fftImag[i];

			this->phaseList[j] = atan2(im, re) + Ogre::Math::PI / 2;
			if (this->phaseList[j] > Ogre::Math::PI)
			{
				this->phaseList[j] -= 2.0f * Ogre::Math::PI;
			}
			this->phaseList[j] = this->phaseList[j] * 180.0f / Ogre::Math::PI;
			this->frequencyList[j] = (this->sumSamplingsPerSec * i) / maxSize;
			++j;

			const Ogre::Real limitValue = 0.001f;
			const Ogre::Real absmin = limitValue * limitValue * this->processingSize * this->processingSize;
			const Ogre::Real abs = (re * re) + (im * im);

			this->magnitudeSpectrum[i] = Ogre::Math::Sqrt(abs) / static_cast<Ogre::Real>(nyquist);

			const auto level = mapValue(limit(mindB, maxdB, amplitudeToDecibels(this->magnitudeSpectrum[i]) - (amplitudeToDecibels(static_cast<Ogre::Real>(nyquist)))), mindB, maxdB, 0.0f, 1.0f);

			this->magnitudeSpectrum[i] = level;
		}

		std::vector<Ogre::Real> spectrum(this->processingSize / 2, 0.0f);
		std::vector<Ogre::Real> averageSpectrum(this->processingSize / 2, 0.0f);

		this->levelSpectrum = this->spectrumPreparation->getSpectrum(this->magnitudeSpectrum);

		// Run the beat detection logic with the updated spectrum
		this->detectBeat(this->levelSpectrum, averageSpectrum);

		this->updateSpectrumVisualization(this->levelSpectrum, averageSpectrum);
	}

	void AudioProcessor::fillAverageSpectrum(std::vector<Ogre::Real>& averageSpectrum, int numBands, const std::deque<std::vector<Ogre::Real>>& fftHistory)
	{
		if (fftHistory.empty())
		{
			std::fill(averageSpectrum.begin(), averageSpectrum.end(), 0.0f);
			return;
		}

		constexpr Ogre::Real smoothingFactor = 0.3f; // Adjust for more/less smoothing
		std::vector<Ogre::Real> sum(numBands, 0.0f);

		// Ensure correct bounds when summing over fftHistory
		for (const auto& pastSpectrum : fftHistory)
		{
			int spectrumSize = pastSpectrum.size();
			for (int i = 0; i < std::min(numBands, spectrumSize); ++i)
			{
				sum[i] += pastSpectrum[i];
			}
		}

		for (int i = 0; i < numBands; ++i)
		{
			Ogre::Real avg = sum[i] / fftHistory.size();
			averageSpectrum[i] = (smoothingFactor * avg) + ((1.0f - smoothingFactor) * averageSpectrum[i]);
		}
	}

	void AudioProcessor::fillVarianceSpectrum(std::vector<Ogre::Real>& varianceSpectrum, int numBands, const std::deque<std::vector<Ogre::Real>>& fftHistory, const std::vector<Ogre::Real>& averageSpectrum)
	{
		if (fftHistory.empty())
		{
			std::fill(varianceSpectrum.begin(), varianceSpectrum.end(), 0.0f);
			return;
		}

		constexpr Ogre::Real smoothingFactor = 0.3f; // Adjust for more/less variance smoothing
		std::vector<Ogre::Real> varianceSum(numBands, 0.0f);

		// Ensure correct bounds when summing over fftHistory
		for (const auto& pastSpectrum : fftHistory)
		{
			int spectrumSize = pastSpectrum.size();
			for (int i = 0; i < std::min(numBands, spectrumSize); ++i)
			{
				Ogre::Real diff = pastSpectrum[i] - averageSpectrum[i];
				varianceSum[i] += diff * diff;
			}
		}

		for (int i = 0; i < numBands; ++i)
		{
			Ogre::Real variance = varianceSum[i] / fftHistory.size();
			varianceSpectrum[i] = (smoothingFactor * variance) + ((1.0f - smoothingFactor) * varianceSpectrum[i]);
		}
	}

	void AudioProcessor::updateSpectrumVisualization(std::vector<Ogre::Real>& spectrum, std::vector<Ogre::Real>& averageSpectrum)
	{
		const int numBands = spectrum.size();
		if (numBands == 0) return; // Safety check to prevent empty spectrum processing

		// Ensure correct size for previous detection values
		this->previousDetection.resize(numBands, 0.0f);
		Ogre::Real alpha = this->firstFrame ? 0.8f : 0.2f; // Higher smoothing at first frame
		Ogre::Real maxDelta = 0.1f;

		// Rolling median filter for smoothing
		this->rollingWindows.resize(numBands);
		const size_t windowSize = 4;

		for (int numBand = 0; numBand < numBands; ++numBand)
		{
			// First frame initialization: avoid sudden jumps
			if (this->firstFrame)
			{
				this->previousDetection[numBand] = 0.0f;
			}

			// Ensure rolling window is correctly populated
			if (this->rollingWindows[numBand].size() >= windowSize)
			{
				this->rollingWindows[numBand].pop_front();
			}
			this->rollingWindows[numBand].push_back(spectrum[numBand]);

			// Compute median only if we have enough elements
			std::vector<Ogre::Real> sortedWindow(this->rollingWindows[numBand].begin(), this->rollingWindows[numBand].end());
			std::sort(sortedWindow.begin(), sortedWindow.end());

			if (!sortedWindow.empty()) // Ensure we don't access an empty vector
			{
				size_t medianIndex = sortedWindow.size() / 2;
				spectrum[numBand] = sortedWindow[medianIndex];
			}
			else
			{
				spectrum[numBand] = 0.0f; // Fallback in case something went wrong
			}

			// Apply Exponential Moving Average (EMA)
			spectrum[numBand] = alpha * spectrum[numBand] + (1 - alpha) * this->previousDetection[numBand];
			this->previousDetection[numBand] = spectrum[numBand];

			// Logarithmic Scaling (Ensure non-negative input for log)
			spectrum[numBand] = std::log(1.0f + std::max(spectrum[numBand], 0.0f) * 10.0f) / std::log(11.0f);

			// Frame-to-frame change limitation
			Ogre::Real delta = spectrum[numBand] - this->previousDetection[numBand];
			if (std::abs(delta) > maxDelta)
			{
				spectrum[numBand] = this->previousDetection[numBand] + (delta > 0 ? maxDelta : -maxDelta);
			}

			// Apply EMA to average spectrum
			averageSpectrum[numBand] = alpha * spectrum[numBand] + (1 - alpha) * averageSpectrum[numBand];
		}

		// Set firstFrame to false after initialization
		this->firstFrame = false;

		// Store spectrum in history
		if (!this->fftHistoryBeatDetector.empty())
		{
			this->fillAverageSpectrum(averageSpectrum, numBands, this->fftHistoryBeatDetector);
			std::vector<Ogre::Real> varianceSpectrum(numBands, 0.0f);
			this->fillVarianceSpectrum(varianceSpectrum, numBands, this->fftHistoryBeatDetector, averageSpectrum);

			std::vector<Ogre::Real> fftResult(spectrum.begin(), spectrum.end());
			if (this->fftHistoryBeatDetector.size() >= this->fftHistoryMaxSize)
			{
				this->fftHistoryBeatDetector.pop_front();
			}
			this->fftHistoryBeatDetector.push_back(fftResult);
		}
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
				}
				spectrum[numBand] /= (this->beatDetectorBandLimits[bandBoundIndex + 1] - this->beatDetectorBandLimits[bandBoundIndex]);
			}

			if (this->fftHistoryBeatDetector.size() > 0)
			{
				fillAverageSpectrum(averageSpectrum, numBands, this->fftHistoryBeatDetector);

				std::vector<Ogre::Real> varianceSpectrum(numBands, 0.0f);
				fillVarianceSpectrum(varianceSpectrum, numBands, this->fftHistoryBeatDetector, averageSpectrum);

				// Update beat detection logic with the smoothed spectrum
				this->spectrumDivision[DEEP_BASS] = (spectrum[DEEP_BASS] - 0.005f) > beatThreshold(varianceSpectrum[DEEP_BASS]) * averageSpectrum[DEEP_BASS];
				this->spectrumDivision[LOW_BASS] = (spectrum[LOW_BASS] - 0.005f) > beatThreshold(varianceSpectrum[LOW_BASS]) * averageSpectrum[LOW_BASS];
				this->spectrumDivision[MID_BASS] = (spectrum[MID_BASS] - 0.005f) > beatThreshold(varianceSpectrum[MID_BASS]) * averageSpectrum[MID_BASS];
				this->spectrumDivision[KICK_DRUM] = (spectrum[KICK_DRUM] - 0.06f) > beatThreshold(varianceSpectrum[KICK_DRUM]) * averageSpectrum[KICK_DRUM];
				this->spectrumDivision[UPPER_BASS] = (spectrum[UPPER_BASS] - 0.005f) > beatThreshold(varianceSpectrum[UPPER_BASS]) * averageSpectrum[UPPER_BASS];
				this->spectrumDivision[LOWER_MIDRANGE] = (spectrum[LOWER_MIDRANGE] - 0.0005f) > beatThreshold(varianceSpectrum[LOWER_MIDRANGE]) * averageSpectrum[LOWER_MIDRANGE];
				this->spectrumDivision[MIDDLE_MIDRANGE] = (spectrum[MIDDLE_MIDRANGE] - 0.0005f) > beatThreshold(varianceSpectrum[MIDDLE_MIDRANGE]) * averageSpectrum[MIDDLE_MIDRANGE];
				this->spectrumDivision[UPPER_MIDRANGE] = (spectrum[UPPER_MIDRANGE] - 0.0005f) > beatThreshold(varianceSpectrum[UPPER_MIDRANGE]) * averageSpectrum[UPPER_MIDRANGE];
				this->spectrumDivision[PRESENCE_RANGE] = (spectrum[PRESENCE_RANGE] - 0.0005f) > beatThreshold(varianceSpectrum[PRESENCE_RANGE]) * averageSpectrum[PRESENCE_RANGE];
				this->spectrumDivision[HIGH_END] = (spectrum[HIGH_END] - 0.0005f) > beatThreshold(varianceSpectrum[HIGH_END]) * averageSpectrum[HIGH_END];
				this->spectrumDivision[EXTREMELY_HIGH_END] = (spectrum[EXTREMELY_HIGH_END] - 0.0005f) > beatThreshold(varianceSpectrum[EXTREMELY_HIGH_END]) * averageSpectrum[EXTREMELY_HIGH_END];

				// HI-HAT detection: High frequency range, sharp transients
				this->spectrumDivision[HI_HAT] = (spectrum[HI_HAT] - 0.0003f) > threshold(-14.0f, varianceSpectrum[HI_HAT], 1.65f) * averageSpectrum[HI_HAT];

				// Ensure SNARE_DRUM detection logic is in place
				this->spectrumDivision[SNARE_DRUM] = (spectrum[SNARE_DRUM] - 0.005f) > beatThreshold(varianceSpectrum[SNARE_DRUM]) * averageSpectrum[SNARE_DRUM];

				// Snare Drum Detection: Fine-tuning
				// float snareThreshold = beatThreshold(varianceSpectrum[SNARE_DRUM]) * averageSpectrum[SNARE_DRUM];

				// Fine-tune the subtraction factor (0.005f to a smaller or larger value for sensitivity adjustment)
				// float snareAdjustment = 0.003f; // Try lowering or increasing this for more/less sensitivity

				// Apply threshold adjustment with a more sensitive detection (adjust the subtraction factor for fine-tuning)
				// this->spectrumDivision[SNARE_DRUM] = (spectrum[SNARE_DRUM] - snareAdjustment) > snareThreshold;

				// Alternative: More aggressive detection with a higher threshold and a more negative offset
				// this->spectrumDivision[SNARE_DRUM] = (spectrum[SNARE_DRUM] - 0.01f) > snareThreshold;
			}

			std::vector<Ogre::Real> fftResult;
			fftResult.reserve(numBands);
			for (int index = 0; index < numBands; ++index)
			{
				fftResult.push_back(spectrum[index]);
			}

			if (this->fftHistoryBeatDetector.size() >= this->samplingFrequency / this->processingSize)
			{
				this->fftHistoryBeatDetector.pop_front();
			}

			this->fftHistoryBeatDetector.push_back(fftResult);
		}
	}

	Ogre::Real AudioProcessor::threshold(Ogre::Real firstValue, Ogre::Real variance, Ogre::Real offset)
	{
		return firstValue * variance + offset;
	}

	Ogre::Real AudioProcessor::beatThreshold(Ogre::Real variance)
	{
		return -15.0f * variance + 1.55f;
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

};
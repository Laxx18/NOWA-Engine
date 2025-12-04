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

		this->fftHistoryBeatDetector.clear();
		this->fftHistoryMaxSize = static_cast<int>(this->samplingFrequency / this->processingSize);

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

			// Normalize magnitude using window normalization
			Ogre::Real magnitude = Ogre::Math::Sqrt(abs) * normalizer;
			if (magnitude <= 0.0f)
			{
				magnitude = 1.0e-8f;
			}

			// Magnitude -> dB
			Ogre::Real db = amplitudeToDecibels(magnitude);

			// *** IMPORTANT: bring back the old offset behavior ***
			// This mimics your original "minus amplitudeToDecibels(nyquist)" logic,
			// so the spectrum sits much lower in dB space.
			db -= amplitudeToDecibels(static_cast<Ogre::Real>(nyquist));

			// Clamp and map to [0,1]
			db = limit(mindB, maxdB, db);
			const Ogre::Real level = mapValue(db, mindB, maxdB, 0.0f, 1.0f);

			this->magnitudeSpectrum[i] = level;
		}

		std::vector<Ogre::Real> spectrum(this->processingSize / 2, 0.0f);
		std::vector<Ogre::Real> averageSpectrum(this->processingSize / 2, 0.0f);

		this->levelSpectrum = this->spectrumPreparation->getSpectrum(this->magnitudeSpectrum);

		// Run the beat detection logic with the updated spectrum
		this->detectBeat(this->levelSpectrum, averageSpectrum);

		this->updateSpectrumVisualization(this->levelSpectrum, averageSpectrum);
	}

#if 1
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
#else
	void AudioProcessor::fillAverageSpectrum(std::vector<Ogre::Real>& averageSpectrum, int numBands, const std::deque<std::vector<Ogre::Real>>& fftHistory)
	{
		if (fftHistory.empty())
		{
			std::fill(averageSpectrum.begin(), averageSpectrum.end(), 0.0f);
			return;
		}

		std::fill(averageSpectrum.begin(), averageSpectrum.end(), 0.0f);

		for (const auto& pastSpectrum : fftHistory)
		{
			const int spectrumSize = static_cast<int>(pastSpectrum.size());
			for (int i = 0; i < std::min(numBands, spectrumSize); ++i)
			{
				averageSpectrum[i] += pastSpectrum[i];
			}
		}

		for (int i = 0; i < numBands; ++i)
		{
			averageSpectrum[i] /= static_cast<Ogre::Real>(fftHistory.size());
		}
	}

	void AudioProcessor::fillVarianceSpectrum(std::vector<Ogre::Real>& varianceSpectrum, int numBands, const std::deque<std::vector<Ogre::Real>>& fftHistory, const std::vector<Ogre::Real>& averageSpectrum)
	{
		if (fftHistory.empty())
		{
			std::fill(varianceSpectrum.begin(), varianceSpectrum.end(), 0.0f);
			return;
		}

		std::fill(varianceSpectrum.begin(), varianceSpectrum.end(), 0.0f);

		for (const auto& pastSpectrum : fftHistory)
		{
			const int spectrumSize = static_cast<int>(pastSpectrum.size());
			for (int i = 0; i < std::min(numBands, spectrumSize); ++i)
			{
				const Ogre::Real diff = pastSpectrum[i] - averageSpectrum[i];
				varianceSpectrum[i] += diff * diff;
			}
		}

		for (int i = 0; i < numBands; ++i)
		{
			varianceSpectrum[i] /= static_cast<Ogre::Real>(fftHistory.size());
		}
	}


#endif

	void AudioProcessor::updateSpectrumVisualization(std::vector<Ogre::Real>& spectrum, std::vector<Ogre::Real>& averageSpectrum)
	{
		const int numBands = static_cast<int>(spectrum.size());
		if (numBands == 0)
		{
			return;
		}

		// Ensure storage is correctly sized
		this->previousDetection.resize(numBands, 0.0f);
		this->rollingWindows.resize(numBands);

		const Ogre::Real alpha = this->firstFrame ? 0.8f : 0.2f; // stronger smoothing on first frame
		const Ogre::Real maxDelta = 0.1f;
		const size_t windowSize = 4;

		for (int numBand = 0; numBand < numBands; ++numBand)
		{
			// Rolling window for median filter (linear domain)
			if (this->rollingWindows[numBand].size() >= windowSize)
			{
				this->rollingWindows[numBand].pop_front();
			}
			this->rollingWindows[numBand].push_back(spectrum[numBand]);

			std::vector<Ogre::Real> sortedWindow(this->rollingWindows[numBand].begin(), this->rollingWindows[numBand].end());
			std::sort(sortedWindow.begin(), sortedWindow.end());

			Ogre::Real medianValue = 0.0f;
			if (!sortedWindow.empty())
			{
				const size_t medianIndex = sortedWindow.size() / 2;
				medianValue = sortedWindow[medianIndex];
			}

			// Log scaling (ensure non-negative input for log)
			Ogre::Real logVal = std::log(1.0f + std::max(medianValue, 0.0f) * 10.0f) / std::log(11.0f);

			// Exponential Moving Average in log-space
			const Ogre::Real prevLog = this->previousDetection[numBand];
			Ogre::Real smoothed = alpha * logVal + (1.0f - alpha) * prevLog;

			// Frame-to-frame change limitation in the same domain (log-space)
			Ogre::Real delta = smoothed - prevLog;
			if (std::abs(delta) > maxDelta)
			{
				smoothed = prevLog + (delta > 0.0f ? maxDelta : -maxDelta);
			}

			// Output and store for next frame
			spectrum[numBand] = smoothed;
			this->previousDetection[numBand] = smoothed;

			// EMA for average spectrum (also in log-space)
			averageSpectrum[numBand] = alpha * smoothed + (1.0f - alpha) * averageSpectrum[numBand];
		}

		this->firstFrame = false;

		// No fftHistoryBeatDetector handling here anymore:
		// history is updated solely in detectBeat(), to avoid double-pushing.
	}

	void AudioProcessor::detectBeat(std::vector<Ogre::Real>& spectrum, std::vector<Ogre::Real>& averageSpectrum)
	{
		// Orientation on: https://www.parallelcube.com/2018/03/30/beat-detection-algorithm/

		// Only read / display half of the buffer typically for analysis 
		// as the 2nd half is usually the same data reversed due to the nature of the way FFT works.
		const int length = this->processingSize / 2;

		if (length > 0)
		{
			const int numBands = static_cast<int>(this->beatDetectorBandLimits.size() / 2);
			if (numBands <= 0)
			{
				return;
			}

			// Ensure vectors are large enough
			if (static_cast<int>(spectrum.size()) < numBands)
			{
				spectrum.resize(numBands, 0.0f);
			}
			if (static_cast<int>(averageSpectrum.size()) < numBands)
			{
				averageSpectrum.resize(numBands, 0.0f);
			}

			// Reset band accumulators every frame (fixes accumulation bug)
			for (int numBand = 0; numBand < numBands; ++numBand)
			{
				spectrum[numBand] = 0.0f;
			}

			// Average FFT magnitudes into frequency bands
			for (int numBand = 0; numBand < numBands; ++numBand)
			{
				const int bandBoundIndex = numBand * 2;
				const int startIdx = this->beatDetectorBandLimits[bandBoundIndex];
				const int endIdx = this->beatDetectorBandLimits[bandBoundIndex + 1];

				const int bandWidth = endIdx - startIdx;
				if (bandWidth <= 0)
				{
					continue;
				}

				for (int indexFFT = startIdx; indexFFT < endIdx; ++indexFFT)
				{
					// magnitudeSpectrum has length "length" (nyquist)
					if (indexFFT >= 0 && indexFFT < static_cast<int>(this->magnitudeSpectrum.size()))
					{
						spectrum[numBand] += this->magnitudeSpectrum[indexFFT];
					}
				}

				spectrum[numBand] /= static_cast<Ogre::Real>(bandWidth);
			}

			// Use history to compute average + variance, but only if we have past frames
			if (!this->fftHistoryBeatDetector.empty())
			{
				fillAverageSpectrum(averageSpectrum, numBands, this->fftHistoryBeatDetector);

				std::vector<Ogre::Real> varianceSpectrum(numBands, 0.0f);
				fillVarianceSpectrum(varianceSpectrum, numBands, this->fftHistoryBeatDetector, averageSpectrum);

				// -------------------------
				// Beat / instrument logic
				// -------------------------

				// Sub-bass / deep bass
				this->spectrumDivision[DEEP_BASS] =
					(spectrum[DEEP_BASS] - 0.005f) > beatThreshold(varianceSpectrum[DEEP_BASS]) * averageSpectrum[DEEP_BASS];

				// Low bass
				this->spectrumDivision[LOW_BASS] =
					(spectrum[LOW_BASS] - 0.005f) > beatThreshold(varianceSpectrum[LOW_BASS]) * averageSpectrum[LOW_BASS];

				// Mid bass
				this->spectrumDivision[MID_BASS] =
					(spectrum[MID_BASS] - 0.005f) > beatThreshold(varianceSpectrum[MID_BASS]) * averageSpectrum[MID_BASS];

				// Kick drum – more aggressive threshold
				this->spectrumDivision[KICK_DRUM] =
					(spectrum[KICK_DRUM] - 0.05f) > beatThreshold(varianceSpectrum[KICK_DRUM]) * averageSpectrum[KICK_DRUM];

				// Upper bass
				this->spectrumDivision[UPPER_BASS] =
					(spectrum[UPPER_BASS] - 0.005f) > beatThreshold(varianceSpectrum[UPPER_BASS]) * averageSpectrum[UPPER_BASS];

				// Lower midrange
				this->spectrumDivision[LOWER_MIDRANGE] =
					(spectrum[LOWER_MIDRANGE] - 0.0005f) > beatThreshold(varianceSpectrum[LOWER_MIDRANGE]) * averageSpectrum[LOWER_MIDRANGE];

				// Middle midrange
				this->spectrumDivision[MIDDLE_MIDRANGE] =
					(spectrum[MIDDLE_MIDRANGE] - 0.0005f) > beatThreshold(varianceSpectrum[MIDDLE_MIDRANGE]) * averageSpectrum[MIDDLE_MIDRANGE];

				// Upper midrange
				this->spectrumDivision[UPPER_MIDRANGE] =
					(spectrum[UPPER_MIDRANGE] - 0.0005f) > beatThreshold(varianceSpectrum[UPPER_MIDRANGE]) * averageSpectrum[UPPER_MIDRANGE];

				// Presence range
				this->spectrumDivision[PRESENCE_RANGE] =
					(spectrum[PRESENCE_RANGE] - 0.0005f) > beatThreshold(varianceSpectrum[PRESENCE_RANGE]) * averageSpectrum[PRESENCE_RANGE];

				// High end
				this->spectrumDivision[HIGH_END] =
					(spectrum[HIGH_END] - 0.0005f) > beatThreshold(varianceSpectrum[HIGH_END]) * averageSpectrum[HIGH_END];

				// Extremely high end
				this->spectrumDivision[EXTREMELY_HIGH_END] =
					(spectrum[EXTREMELY_HIGH_END] - 0.0005f) > beatThreshold(varianceSpectrum[EXTREMELY_HIGH_END]) * averageSpectrum[EXTREMELY_HIGH_END];

				// HI-HAT detection: high frequency range, sharp transients
				this->spectrumDivision[HI_HAT] =
					(spectrum[HI_HAT] - 0.0003f) >
					threshold(-14.0f, varianceSpectrum[HI_HAT], 1.65f) * averageSpectrum[HI_HAT];

				// Ensure SNARE_DRUM detection logic is in place
				this->spectrumDivision[SNARE_DRUM] =
					(spectrum[SNARE_DRUM] - 0.005f) > beatThreshold(varianceSpectrum[SNARE_DRUM]) * averageSpectrum[SNARE_DRUM];
			}

			// --------------------------------
			// Update history for next frames
			// --------------------------------
			std::vector<Ogre::Real> fftResult;
			fftResult.reserve(numBands);
			for (int index = 0; index < numBands; ++index)
			{
				fftResult.push_back(spectrum[index]);
			}

			// Limit history length roughly to "frames per second"
			if (this->fftHistoryBeatDetector.size() >= static_cast<size_t>(this->samplingFrequency / this->processingSize))
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
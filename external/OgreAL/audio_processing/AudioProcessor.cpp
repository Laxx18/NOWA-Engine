#include "AudioProcessor.h"
#include <assert.h>

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
		firstFrame(true),
		beatHoldFramesMax(4)
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
		this->beatBandSpectrum.clear();

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

		// We have one [start, end] pair per band, from DEEP_BASS (0) up to HI_HAT (12).
		// That is SpectrumArea::MAX_VALUE bands in total.
		this->beatDetectorBandLimits.reserve(static_cast<int>(AudioProcessor::SpectrumArea::MAX_VALUE) * 2);

		// Important: Order MUST match the SpectrumArea enum in AudioProcessor.h:
		// DEEP_BASS, LOW_BASS, KICK_DRUM, MID_BASS, UPPER_BASS,
		// LOWER_MIDRANGE, SNARE_DRUM, MIDDLE_MIDRANGE, UPPER_MIDRANGE,
		// PRESENCE_RANGE, HIGH_END, EXTREMELY_HIGH_END, HI_HAT

		// 0 DEEP_BASS 20Hz-40Hz
		this->beatDetectorBandLimits.push_back(20 / bandSize);
		this->beatDetectorBandLimits.push_back(40 / bandSize);

		// 1 LOW_BASS 40Hz-80Hz
		this->beatDetectorBandLimits.push_back(40 / bandSize);
		this->beatDetectorBandLimits.push_back(80 / bandSize);

		// 2 KICK_DRUM 60Hz-130Hz
		this->beatDetectorBandLimits.push_back(60 / bandSize);
		this->beatDetectorBandLimits.push_back(130 / bandSize);

		// 3 MID_BASS 80Hz-160Hz
		this->beatDetectorBandLimits.push_back(80 / bandSize);
		this->beatDetectorBandLimits.push_back(160 / bandSize);

		// 4 UPPER_BASS 160Hz-300Hz
		this->beatDetectorBandLimits.push_back(160 / bandSize);
		this->beatDetectorBandLimits.push_back(300 / bandSize);

		// 5 LOWER_MIDRANGE 300Hz-600Hz
		this->beatDetectorBandLimits.push_back(300 / bandSize);
		this->beatDetectorBandLimits.push_back(600 / bandSize);

		// 6 SNARE_DRUM 301Hz-750Hz
		this->beatDetectorBandLimits.push_back(301 / bandSize);
		this->beatDetectorBandLimits.push_back(750 / bandSize);

		// 7 MIDDLE_MIDRANGE 600Hz-1200Hz
		this->beatDetectorBandLimits.push_back(600 / bandSize);
		this->beatDetectorBandLimits.push_back(1200 / bandSize);

		// 8 UPPER_MIDRANGE 1200Hz-2400Hz
		this->beatDetectorBandLimits.push_back(1200 / bandSize);
		this->beatDetectorBandLimits.push_back(2400 / bandSize);

		// 9 PRESENCE_RANGE 2400Hz-5000Hz
		this->beatDetectorBandLimits.push_back(2400 / bandSize);
		this->beatDetectorBandLimits.push_back(5000 / bandSize);

		// 10 HIGH_END 5000Hz-10000Hz
		this->beatDetectorBandLimits.push_back(5000 / bandSize);
		this->beatDetectorBandLimits.push_back(10000 / bandSize);

		// 11 EXTREMELY_HIGH_END 10000Hz-20000Hz
		this->beatDetectorBandLimits.push_back(10000 / bandSize);
		this->beatDetectorBandLimits.push_back(20000 / bandSize);

		// 12 HI_HAT 7000Hz-12000Hz (overlapping range just for hi-hat)
		this->beatDetectorBandLimits.push_back(7000 / bandSize);
		this->beatDetectorBandLimits.push_back(12000 / bandSize);

		// History and flags
		this->fftHistoryBeatDetector.clear();
		this->fftHistoryMaxSize = static_cast<int>(this->samplingFrequency / this->processingSize);

		// One bool per enum entry, indexable via SpectrumArea
		this->spectrumDivision.assign(static_cast<size_t>(AudioProcessor::SpectrumArea::MAX_VALUE), false);

		// Init beat hold state (same size as spectrumDivision)
		this->beatHoldFrames.assign(static_cast<size_t>(AudioProcessor::SpectrumArea::MAX_VALUE), 0);

		// How many frames a beat stays "on" after detection (tweak to taste)
		this->beatHoldFramesMax = 4;
	}

	void AudioProcessor::updateBeatHold(SpectrumArea band, bool beatNow)
	{
		const int idx = static_cast<int>(band);

		if (true == beatNow)
		{
			this->beatHoldFrames[idx] = this->beatHoldFramesMax;
		}
		else if (this->beatHoldFrames[idx] > 0)
		{
			--this->beatHoldFrames[idx];
		}

		this->spectrumDivision[idx] = (this->beatHoldFrames[idx] > 0);
	}

	void AudioProcessor::process(Ogre::Real timerSeconds, const std::vector<Ogre::Real>& data) noexcept
	{
		(void)timerSeconds;

		assert(data.size() == this->bufferData.size());
		std::copy(data.begin(), data.end(), this->bufferData.begin());

		Ogre::Real windowSum = 0.0f;
		for (int i = 0; i < this->processingSize; i++)
		{
			this->kissFFTIn[i].r = this->bufferData[i] * this->windowFunction[i];
			this->kissFFTIn[i].i = 0.0f;

			windowSum += this->windowFunction[i];
		}

		kiss_fft(this->kissFFT, &this->kissFFTIn[0], &this->kissFFTOut[0]);

		for (int i = 0; i < this->processingSize; i++)
		{
			this->fftReal[i] = this->kissFFTOut[i].r;
			this->fftImag[i] = this->kissFFTOut[i].i;
		}

		const int maxSize = this->processingSize;
		const int nyquist = this->processingSize / 2;

		Ogre::Real normalizer = 0.0f;
		if (windowSum > 0.0f)
		{
			normalizer = 2.0f / windowSum;
		}

		if (this->magnitudeSpectrum.size() < static_cast<size_t>(nyquist))
		{
			this->magnitudeSpectrum.resize(nyquist, 0.0f);
		}
		if (this->phaseList.size() < static_cast<size_t>(nyquist))
		{
			this->phaseList.resize(nyquist, 0.0f);
		}
		if (this->frequencyList.size() < static_cast<size_t>(nyquist))
		{
			this->frequencyList.resize(nyquist, 0);
		}

		int j = 0;

		for (int i = 0; i < nyquist; i++)
		{
			const Ogre::Real re = this->fftReal[i];
			const Ogre::Real im = this->fftImag[i];

			this->phaseList[j] = atan2(im, re) + Ogre::Math::PI / 2.0f;
			if (this->phaseList[j] > Ogre::Math::PI)
			{
				this->phaseList[j] -= 2.0f * Ogre::Math::PI;
			}
			this->phaseList[j] = this->phaseList[j] * 180.0f / Ogre::Math::PI;

			this->frequencyList[j] = static_cast<int>((this->samplingFrequency * i) / maxSize);
			++j;

			const Ogre::Real abs = (re * re) + (im * im);

			Ogre::Real magnitude = Ogre::Math::Sqrt(abs) * normalizer;

			if (magnitude < 0.0f)
			{
				magnitude = 0.0f;
			}
			if (magnitude > 1.0f)
			{
				magnitude = 1.0f;
			}

			this->magnitudeSpectrum[i] = magnitude;
		}

		std::vector<Ogre::Real> spectrum(this->processingSize / 2, 0.0f);
		std::vector<Ogre::Real> averageSpectrum(this->processingSize / 2, 0.0f);

		this->levelSpectrum = this->spectrumPreparation->getSpectrum(this->magnitudeSpectrum);

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

#if 1
	void AudioProcessor::detectBeat(std::vector<Ogre::Real>& spectrum, std::vector<Ogre::Real>& averageSpectrum)
	{
		// Orientation on: https://www.parallelcube.com/2018/03/30/beat-detection-algorithm/

		// Only read / display half of the buffer typically for analysis 
		// as the 2nd half is usually the same data reversed due to the nature of the way FFT works.
		const int length = this->processingSize / 2;

		if (length > 0)
		{
			// Number of bands defined by our enum (must match initializeBeatDetector())
			const int numBandsEnum = static_cast<int>(AudioProcessor::SpectrumArea::MAX_VALUE);
			const int bandsFromLimits = static_cast<int>(this->beatDetectorBandLimits.size() / 2);

			// Be defensive: use the smaller of the two, but we EXPECT them to match (13).
			const int numBands = std::min(numBandsEnum, bandsFromLimits);

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

			// -----------------------------
			// Average FFT into our bands
			// -----------------------------
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

			// Store a copy for external intensity queries (Lua, etc.)
			this->beatBandSpectrum = spectrum;

			// ---------------------------------------------
			// Average + variance over history (Parallelcube)
			// ---------------------------------------------
			if (!this->fftHistoryBeatDetector.empty())
			{
				fillAverageSpectrum(averageSpectrum, numBands, this->fftHistoryBeatDetector);

				std::vector<Ogre::Real> varianceSpectrum(numBands, 0.0f);
				fillVarianceSpectrum(varianceSpectrum, numBands, this->fftHistoryBeatDetector, averageSpectrum);

				// -------------------------
				// Beat / instrument logic
				// -------------------------

				auto detectBand = [&](SpectrumArea band,
					Ogre::Real gain,           // linear gain for this band
					Ogre::Real minRelIncrease) // required relative increase over average (e.g. 0.3 = +30%)
					{
						const int idx = static_cast<int>(band);

						Ogre::Real current = spectrum[idx] * gain;
						Ogre::Real average = averageSpectrum[idx];
						Ogre::Real variance = varianceSpectrum[idx];

						if (average <= 0.0f)
						{
							this->updateBeatHold(band, false);
							return;
						}

						// Dynamic threshold based on variance (clamped in beatThreshold())
						const Ogre::Real th = beatThreshold(variance);
						const Ogre::Real thValue = th * average;

						// Extra guard: require a minimum relative increase above the moving average.
						// rel = (current - average) / average
						const Ogre::Real rel = (current - average) / average;

						const bool beatNow = (current > thValue) && (rel > minRelIncrease);

						this->updateBeatHold(band, beatNow);
					};

				// Low frequencies
				detectBand(DEEP_BASS, 1.0f, 0.15f);
				detectBand(LOW_BASS, 1.0f, 0.15f);
				detectBand(MID_BASS, 1.0f, 0.15f);
				detectBand(UPPER_BASS, 1.0f, 0.15f);

				// Kick drum – a bit of gain, but require a clear peak (~35% above avg)
				detectBand(KICK_DRUM, 1.15f, 0.35f);

				// Lower midrange
				detectBand(LOWER_MIDRANGE, 1.0f, 0.20f);

				// Snare drum – reduce gain and require strong transient (~50% above avg)
				detectBand(SNARE_DRUM, 1.25f, 0.30f);

				// Middle midrange
				detectBand(MIDDLE_MIDRANGE, 1.0f, 0.20f);
				detectBand(UPPER_MIDRANGE, 1.0f, 0.20f);
				detectBand(PRESENCE_RANGE, 1.0f, 0.20f);
				detectBand(HIGH_END, 1.0f, 0.20f);
				detectBand(EXTREMELY_HIGH_END, 1.0f, 0.20f);

				// Hi-hat: keep special threshold(), but still require a relative increase
				{
					const int idx = static_cast<int>(HI_HAT);

					Ogre::Real current = spectrum[idx] * 1.5f;  // mild boost for high freqs
					Ogre::Real average = averageSpectrum[idx];
					Ogre::Real variance = varianceSpectrum[idx];

					if (average <= 0.0f)
					{
						this->updateBeatHold(HI_HAT, false);
					}
					else
					{
						// threshold(-14 * var + 1.65) with clamping applied inside threshold()
						const Ogre::Real th = threshold(-14.0f, variance, 1.65f);
						const Ogre::Real thValue = th * average;

						const Ogre::Real rel = (current - average) / average;

						const bool beatHiHat = (current > thValue) && (rel > 0.40f); // require +40% over avg
						this->updateBeatHold(HI_HAT, beatHiHat);
					}
				}

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

			if (this->fftHistoryBeatDetector.size() >= this->fftHistoryMaxSize)
			{
				this->fftHistoryBeatDetector.pop_front();
			}

			this->fftHistoryBeatDetector.push_back(fftResult);
		}
	}
#else
void AudioProcessor::detectBeat(std::vector<Ogre::Real>& spectrum, std::vector<Ogre::Real>& averageSpectrum)
{
	// Orientation on: https://www.parallelcube.com/2018/03/30/beat-detection-algorithm/

	// Only read / display half of the buffer typically for analysis 
	// as the 2nd half is usually the same data reversed due to the nature of the way FFT works.
	const int length = this->processingSize / 2;

	if (length > 0)
	{
		// Number of bands defined by our enum (must match initializeBeatDetector())
		const int numBandsEnum = static_cast<int>(AudioProcessor::SpectrumArea::MAX_VALUE);
		const int bandsFromLimits = static_cast<int>(this->beatDetectorBandLimits.size() / 2);

		// Be defensive: use the smaller of the two, but we EXPECT them to match (13).
		const int numBands = std::min(numBandsEnum, bandsFromLimits);

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

		// -----------------------------
		// Average FFT into our bands
		// -----------------------------
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

		// Store a copy for external intensity queries (Lua, etc.)
		this->beatBandSpectrum = spectrum;

		// ---------------------------------------------
		// Average + variance over history (Parallelcube)
		// ---------------------------------------------
		if (!this->fftHistoryBeatDetector.empty())
		{
			fillAverageSpectrum(averageSpectrum, numBands, this->fftHistoryBeatDetector);

			std::vector<Ogre::Real> varianceSpectrum(numBands, 0.0f);
			fillVarianceSpectrum(varianceSpectrum, numBands, this->fftHistoryBeatDetector, averageSpectrum);

			// -------------------------
			// Beat / instrument logic
			// -------------------------

			auto simpleBeat = [&](SpectrumArea band) -> bool
				{
					const int idx = static_cast<int>(band);

					const Ogre::Real current = spectrum[idx];
					const Ogre::Real average = averageSpectrum[idx];
					const Ogre::Real variance = varianceSpectrum[idx];

					if (average <= 0.0f)
						return false;

					const Ogre::Real th = beatThreshold(variance);     // -15*var + 1.55
					const Ogre::Real thValue = th * average;

					return current > thValue;
				};

			// Basic detection for all non-drum “bands” (we don’t conflict-resolve these)
			this->spectrumDivision[DEEP_BASS] = simpleBeat(DEEP_BASS);
			this->spectrumDivision[LOW_BASS] = simpleBeat(LOW_BASS);
			this->spectrumDivision[MID_BASS] = simpleBeat(MID_BASS);
			this->spectrumDivision[UPPER_BASS] = simpleBeat(UPPER_BASS);
			this->spectrumDivision[LOWER_MIDRANGE] = simpleBeat(LOWER_MIDRANGE);
			this->spectrumDivision[MIDDLE_MIDRANGE] = simpleBeat(MIDDLE_MIDRANGE);
			this->spectrumDivision[UPPER_MIDRANGE] = simpleBeat(UPPER_MIDRANGE);
			this->spectrumDivision[PRESENCE_RANGE] = simpleBeat(PRESENCE_RANGE);
			this->spectrumDivision[HIGH_END] = simpleBeat(HIGH_END);
			this->spectrumDivision[EXTREMELY_HIGH_END] = simpleBeat(EXTREMELY_HIGH_END);

			// -------------------------
			// Hi-hat: special threshold
			// -------------------------
			{
				const int idx = static_cast<int>(HI_HAT);

				const Ogre::Real current = spectrum[idx];
				const Ogre::Real average = averageSpectrum[idx];
				const Ogre::Real variance = varianceSpectrum[idx];

				bool beatHiHat = false;

				if (average > 0.0f)
				{
					// Your hi-hat rule from before: threshold(-14 * var + 1.65)
					Ogre::Real th = threshold(-14.0f, variance, 1.65f);
					Ogre::Real thValue = th * average;

					// Optional: small relative increase requirement (~+30% over average)
					Ogre::Real relInc = (current - average) / (average + 1e-6f);

					beatHiHat = (current > thValue) && (relInc > 0.3f);
				}

				this->spectrumDivision[HI_HAT] = beatHiHat;
			}

			// -------------------------
			// Kick vs snare: conflict-aware
			// -------------------------
			{
				const int kIdx = static_cast<int>(KICK_DRUM);
				const int sIdx = static_cast<int>(SNARE_DRUM);

				const Ogre::Real curKick = spectrum[kIdx];
				const Ogre::Real avgKick = averageSpectrum[kIdx];
				const Ogre::Real varKick = varianceSpectrum[kIdx];
				const Ogre::Real curSnare = spectrum[sIdx];
				const Ogre::Real avgSnare = averageSpectrum[sIdx];
				const Ogre::Real varSnare = varianceSpectrum[sIdx];

				bool beatKick = false;
				bool beatSnare = false;

				Ogre::Real scoreKick = -1.0f;
				Ogre::Real scoreSnare = -1.0f;

				const Ogre::Real eps = 1e-6f;

				if (avgKick > 0.0f)
				{
					const Ogre::Real thKick = beatThreshold(varKick);        // -15*var + 1.55
					const Ogre::Real thVal = thKick * avgKick;
					const Ogre::Real diffKick = curKick - thVal;

					// Normalize by average so we can compare across bands
					scoreKick = diffKick / (avgKick + eps);

					// Require a small positive margin over the threshold
					beatKick = scoreKick > 0.05f;  // ~5% above threshold
				}

				if (avgSnare > 0.0f)
				{
					const Ogre::Real thSnare = beatThreshold(varSnare);
					const Ogre::Real thVal = thSnare * avgSnare;
					const Ogre::Real diffSnare = curSnare - thVal;

					scoreSnare = diffSnare / (avgSnare + eps);

					// Snare is usually more masked by guitars -> demand a bit more
					beatSnare = scoreSnare > 0.10f; // ~10% above threshold
				}

				// If only one fired, nothing to do
				if (!(beatKick && beatSnare))
				{
					this->spectrumDivision[KICK_DRUM] = beatKick;
					this->spectrumDivision[SNARE_DRUM] = beatSnare;
				}
				else
				{
					// Both bands say "beat": decide which one dominates.
					// Use a dominance margin so a slightly higher score doesn't immediately suppress the other.
					const Ogre::Real dominance = 1.2f; // 20% stronger

					if (scoreKick > scoreSnare * dominance)
					{
						// Kick clearly dominates -> suppress snare
						beatSnare = false;
					}
					else if (scoreSnare > scoreKick * dominance)
					{
						// Snare clearly dominates -> suppress kick
						beatKick = false;
					}
					else
					{
						// Scores are similar: fall back to absolute energy comparison
						if (curKick >= curSnare)
							beatSnare = false;
						else
							beatKick = false;
					}

					this->spectrumDivision[KICK_DRUM] = beatKick;
					this->spectrumDivision[SNARE_DRUM] = beatSnare;
				}
			}


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

		if (this->fftHistoryBeatDetector.size() >= this->fftHistoryMaxSize)
		{
			this->fftHistoryBeatDetector.pop_front();
		}

		this->fftHistoryBeatDetector.push_back(fftResult);
	}
}

#endif

	Ogre::Real AudioProcessor::getSpectrumAreaLevel(SpectrumArea area) const noexcept
	{
		const int idx = static_cast<int>(area);

		if (idx < 0 || idx >= static_cast<int>(this->beatBandSpectrum.size()))
		{
			return 0.0f;
		}

		// You can clamp if you want it always non-negative and <= 1:
		Ogre::Real val = this->beatBandSpectrum[idx];
		if (val < 0.0f)
			val = 0.0f;
		return val;
	}

#if 0
	Ogre::Real AudioProcessor::threshold(Ogre::Real firstValue, Ogre::Real variance, Ogre::Real offset)
	{
		return firstValue * variance + offset;
	}

	Ogre::Real AudioProcessor::beatThreshold(Ogre::Real variance)
	{
		return -15.0f * variance + 1.55f;
	}
#else
	Ogre::Real AudioProcessor::threshold(Ogre::Real firstValue, Ogre::Real variance, Ogre::Real offset)
	{
		// Generic linear threshold: firstValue * variance + offset
		Ogre::Real t = firstValue * variance + offset;

		// Clamp to avoid absurdly low or high thresholds with our normalized data
		if (t < 1.1f)
			t = 1.1f;
		else if (t > 3.0f)
			t = 3.0f;

		return t;
	}

	Ogre::Real AudioProcessor::beatThreshold(Ogre::Real variance)
	{
		// Parallelcube-inspired: -15 * variance + 1.55
		Ogre::Real t = -15.0f * variance + 1.55f;

		// With your normalized magnitudes, this can drop below 1.0 and cause
		// "always-on" beats. Clamp it into a safe range.
		if (t < 1.1f)
			t = 1.1f;
		else if (t > 3.0f)
			t = 3.0f;

		return t;
	}

#endif

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
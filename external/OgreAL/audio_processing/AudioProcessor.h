#ifndef AUDIO_PROCESSOR
#define AUDIO_PROCESSOR

#include "../kiss_fft130/kiss_fft.h"
#include "MathWindows.h"
#include "SpectrumPreparation.h"
#include "OgreALPrereqs.h"

namespace OgreAL
{

	class OgreAL_Export AudioProcessor
	{
	public:
		enum SpectrumPreparationType
		{
			RAW,
			LINEAR,
			LOGARITHMIC
		};

		// https://blog.landr.com/eq-basics-everything-musicians-need-know-eq/
		// https://www.teachmeaudio.com/mixing/techniques/frequencies/
		// https://www.teachmeaudio.com/mixing/techniques/audio-spectrum/
		enum SpectrumArea
		{
			DEEP_BASS = 0,			// 20hz-40hz
			LOW_BASS = 1,			// 40hz-80hz
			MID_BASS = 2,			// 80hz-160hz
			UPPER_BASS = 3,			// 160hz-300hz
			LOWER_MIDRANGE = 4,		// 300hz-600hz
			MIDDLE_MIDRANGE = 5,	// 600hz-1200hz
			UPPER_MIDRANGE = 6,		// 1200hz-2400hz
			PRESENCE_RANGE = 7,		// 2400hz-5000hz
			HIGH_END = 8,			// 5000hz-10000hz
			EXTREMELY_HIGH_END = 9, // 10000hz-20000hz
		};

		enum Instrument
		{
			KICK_DRUM = 10,			// 60hz-130hz
			SNARE_DRUM = 11,		// 301hz-750hz
			MAX_VALUE = 12
		};

		typedef std::deque<std::vector<Ogre::Real>> FFTHistoryContainer;

		AudioProcessor(int processingSize, int frequency, int numberOfBands, int sumSamplingsPerSec, MathWindows::WindowType windowType = MathWindows::BLACKMAN,
			SpectrumPreparationType spectrumPreparationType = SpectrumPreparationType::LOGARITHMIC, Ogre::Real smoothFactor = 0.0f) noexcept;
		~AudioProcessor() noexcept;

		void setProcessingSize(int processingSize) noexcept;
		void setSamplingFrequency(int frequency) noexcept;

		int getProcessingSize(void) noexcept;
		int getSamplingFrequency(void) noexcept;

		void process(Ogre::Real timerSeconds, const std::vector<Ogre::Real>& data) noexcept;

		const std::vector<Ogre::Real>& getMagnitudeSpectrum(void) noexcept;

		const std::vector<Ogre::Real>& getLevelSpectrum(void) noexcept;

		const std::vector<int>& getFrequencyList(void) noexcept;

		const std::vector<Ogre::Real>& getPhaseList(void) noexcept;

		bool isSpectrumArea(SpectrumArea spectrumArea) const noexcept;

		bool isInstrument(Instrument instrument) const noexcept;
	private:
		void initializeFFT();
		void initializeBeatDetector();
		void detectBeat(std::vector<Ogre::Real>& spectrum, std::vector<Ogre::Real>& averageSpectrum);

		inline Ogre::Real mapValue(Ogre::Real valueToMap, Ogre::Real targetMin, Ogre::Real targetMax) noexcept;

		inline Ogre::Real mapValue(Ogre::Real valueToMap, Ogre::Real sourceMin, Ogre::Real sourceMax, Ogre::Real targetMin, Ogre::Real targetMax) noexcept;

		inline Ogre::Real amplitudeToDecibels(Ogre::Real amplitude) noexcept;
 
		inline Ogre::Real decibelsToAmplitude(Ogre::Real decibels) noexcept;

		inline Ogre::Real limit(Ogre::Real lowerValue, Ogre::Real upperValue, Ogre::Real limitValue) noexcept;
	private:
		static void fillAverageSpectrum(std::vector<Ogre::Real>& averageSpectrum, int numBands, const FFTHistoryContainer& fftHistory);
		static void fillVarianceSpectrum(std::vector<Ogre::Real>& varianceSpectrum, int numBands, const FFTHistoryContainer& fftHistory, const std::vector<Ogre::Real>& averageSpectrum);
		static Ogre::Real beatThreshold(Ogre::Real variance);
		static Ogre::Real threshold(Ogre::Real firstValue, Ogre::Real variance, Ogre::Real offset);
	private:
		std::vector<Ogre::Real> fftReal;
		std::vector<Ogre::Real> fftImag;
		std::vector<Ogre::Real> bufferData;
		std::vector<Ogre::Real> windowFunction;
		std::vector<Ogre::Real> magnitudeSpectrum;
		std::vector<Ogre::Real> levelSpectrum;
		std::vector<int> frequencyList;
		std::vector<bool> spectrumDivision;
		std::vector<Ogre::Real> phaseList;

		kiss_fft_cfg kissFFT;
		std::vector<kiss_fft_cpx> kissFFTIn;
		std::vector<kiss_fft_cpx> kissFFTOut;

		int processingSize;
		int samplingFrequency;
		int numberOfBands;
		int sumSamplingsPerSec;
		MathWindows::WindowType windowType;

		std::vector<int> beatDetectorBandLimits;
		FFTHistoryContainer fftHistoryBeatDetector;

		SpectrumPreparationType spectrumPreparationType;
		SpectrumPreparation* spectrumPreparation;
		Ogre::Real smoothFactor;
	};

};

#endif

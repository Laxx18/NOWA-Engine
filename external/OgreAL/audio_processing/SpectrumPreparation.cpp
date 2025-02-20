#include "SpectrumPreparation.h"

namespace OgreAL
{

	SpectrumPreparation::SpectrumPreparation()
		: spectrumProcessingSize(0),
		smoothFactor(1.0f)
	{
	}

	SpectrumPreparation::~SpectrumPreparation()
	{
	}


	//////////////////////////////////////////////////////////

	SpectrumPreparationRaw::SpectrumPreparationRaw()
		: SpectrumPreparation()
	{
	}

	SpectrumPreparationRaw::~SpectrumPreparationRaw()
	{
	}

	int SpectrumPreparationRaw::init(int spectrumProcessingSize, int numberOfBands, int frequency, Ogre::Real smoothFactor)
	{
		this->spectrumProcessingSize = spectrumProcessingSize;
		if (smoothFactor > 1.0f)
		{
			smoothFactor = 1.0f;
		}
		this->smoothFactor = smoothFactor;
		return numberOfBands;
	}

	std::vector<Ogre::Real> SpectrumPreparationRaw::getSpectrum(std::vector<Ogre::Real>& data)
	{
		return std::move(data);
	}

	//////////////////////////////////////////////////////////

	SpectrumPreparationLinear::SpectrumPreparationLinear()
		: SpectrumPreparation()
	{
	}

	SpectrumPreparationLinear::~SpectrumPreparationLinear()
	{
	}

	int SpectrumPreparationLinear::init(int spectrumProcessingSize, int numberOfBands, int frequency, Ogre::Real smoothFactor)
	{
		this->spectrumProcessingSize = spectrumProcessingSize;
		if (smoothFactor > 1.0f)
		{
			smoothFactor = 1.0f;
		}
		this->smoothFactor = smoothFactor;

		int barSamples = (spectrumProcessingSize / 2) / numberOfBands;

		// Calculates num fft samples per bar
		this->numberOfSamplesPerBar.clear();
		for (int i = 0; i < numberOfBands; ++i)
		{
			this->numberOfSamplesPerBar.emplace_back(barSamples);
		}
		return this->numberOfSamplesPerBar.size();
	}

	std::vector<Ogre::Real> SpectrumPreparationLinear::getSpectrum(std::vector<Ogre::Real>& data)
	{
		// The problem of this spectrum is that a lot of useful information is grouped together in the first bars losing important details in the lower frequencies.
		// Most of the useful information in the spectrum is all below 15000 Hz.

		// Only read / display half of the buffer typically for analysis 
		// as the 2nd half is usually the same data reversed due to the nature of the way FFT works.
		int length = this->spectrumProcessingSize / 2;

		std::vector<Ogre::Real> spectrum(this->numberOfSamplesPerBar.size(), 0.0f);

		if (length > 0)
		{
			Ogre::Real previousSpectrum = 0.0f;
			int indexFFT = 0;
			for (int index = 0; index < this->numberOfSamplesPerBar.size(); ++index)
			{
				previousSpectrum = spectrum[index];
				for (int frec = 0; frec < this->numberOfSamplesPerBar[index] && indexFFT < data.size(); ++frec)
				{
					spectrum[index] += data[indexFFT];
					++indexFFT;
				}
				if (this->numberOfSamplesPerBar[index] > 0)
				{
					spectrum[index] /= static_cast<Ogre::Real>(this->numberOfSamplesPerBar[index]);
				}

				if (this->smoothFactor > 0.0f)
				{
					spectrum[index] = this->emaFilter(spectrum[index], previousSpectrum, this->smoothFactor);
				}
			}
		}

		return std::move(spectrum);
	}

	//////////////////////////////////////////////////////////

	SpectrumPreparationLogarithm::SpectrumPreparationLogarithm()
		: SpectrumPreparation()
	{
	}

	SpectrumPreparationLogarithm::~SpectrumPreparationLogarithm()
	{
	}

	int SpectrumPreparationLogarithm::init(int spectrumProcessingSize, int numberOfBands, int frequency, Ogre::Real smoothFactor)
	{
		this->spectrumProcessingSize = spectrumProcessingSize;
		if (smoothFactor > 1.0f)
		{
			smoothFactor = 1.0f;
		}
		this->smoothFactor = smoothFactor;

		// Calculates octave frequency
		std::vector<int> frequencyOctaves;
		frequencyOctaves.push_back(0);

		// A natural way to do this is for each average to span an octave. We could group frequencies like so (this assumes a sampling rate of 44100 Hz):

		// 0 to 11 Hz
		// 11 to 22 Hz
		// 22 to 43 Hz
		// 43 to 86 Hz
		// 86 to 172 Hz
		// 172 to 344 Hz
		// 344 to 689 Hz
		// 689 to 1378 Hz
		// 1378 to 2756 Hz
		// 2756 to 5512 Hz
		// 5512 to 11025 Hz
		// 11025 to 22050 Hz

		// This gives us only 12 bands, but already it is more useful than the 32 linear bands. If we want more than 12 bands, 
		// we could split each octave in two, or three, the fineness would be limited only by the size of the FFT.
		// We can precalculate this values using the next code:
		for (int i = 1; i < 13; ++i)
		{
			frequencyOctaves.push_back(static_cast<int>((frequency / 2) / static_cast<Ogre::Real>(pow(2, 12 - i))));
		}

		int bandWidth = (frequency / spectrumProcessingSize);
		int bandsPerOctave = numberOfBands / 12; // Octaves

		// Calculates num fft samples per bar
		this->numberOfSamplesPerBar.clear();
		for (int octave = 0; octave < 12; ++octave)
		{
			int indexLow = frequencyOctaves[octave] / bandWidth;
			int indexHigh = (frequencyOctaves[octave + 1]) / bandWidth;
			int octavaIndexes = (indexHigh - indexLow);

			if (octavaIndexes > 0)
			{
				if (octavaIndexes <= bandsPerOctave)
				{
					for (int count = 0; count < octavaIndexes; ++count)
					{
						this->numberOfSamplesPerBar.emplace_back(1.0f);
					}
				}
				else
				{
					for (int count = 0; count < bandsPerOctave; ++count)
					{
						this->numberOfSamplesPerBar.emplace_back(octavaIndexes / bandsPerOctave);
					}
				}
			}
		}
		return this->numberOfSamplesPerBar.size();
	}

	std::vector<Ogre::Real> SpectrumPreparationLogarithm::getSpectrum(std::vector<Ogre::Real>& data)
	{
		std::vector<Ogre::Real> spectrum(this->numberOfSamplesPerBar.size(), 0.0f);

		// Only read / display half of the buffer for analysis
		int length = this->spectrumProcessingSize / 2;

		if (length > 0)
		{
			Ogre::Real previousSpectrum = 0.0f;
			int indexFFT = 0;
			for (int index = 0; index < this->numberOfSamplesPerBar.size(); ++index)
			{
				previousSpectrum = spectrum[index];
				for (int frec = 0; frec < this->numberOfSamplesPerBar[index] && indexFFT < data.size(); ++frec)
				{
					spectrum[index] += data[indexFFT];
					++indexFFT;
				}

				if (this->numberOfSamplesPerBar[index] > 0)
				{
					spectrum[index] /= static_cast<Ogre::Real>(this->numberOfSamplesPerBar[index]);
				}

				// Apply logarithmic scaling to reduce large peaks and enhance small signals
				spectrum[index] = std::log(1.0f + spectrum[index]);

				// Apply smoothing if needed
				if (this->smoothFactor > 0.0f)
				{
					spectrum[index] = this->emaFilter(spectrum[index], previousSpectrum, this->smoothFactor);
				}
			}
		}

		return std::move(spectrum);
	}
};
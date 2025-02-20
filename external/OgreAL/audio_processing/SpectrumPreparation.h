#ifndef SPECTRUM_PREPARATION
#define SPECTRUM_PREPARATION

#include "OgreMath.h"

namespace OgreAL
{

	class SpectrumPreparation
	{
	public:
		SpectrumPreparation();

		virtual ~SpectrumPreparation();
		/** Returns the actual used spectrum size. */
		virtual int init(int spectrumProcessingSize, int numberOfBands, int frequency, Ogre::Real smoothFactor) = 0;

		virtual std::vector<Ogre::Real> getSpectrum(std::vector<Ogre::Real>& data) = 0;
	protected:

		inline Ogre::Real lowPassFilter(Ogre::Real value, Ogre::Real oldValue, Ogre::Real scale) noexcept
		{
			return Ogre::Real((1.0f - scale) * oldValue + scale * value);
		}

		inline Ogre::Real emaFilter(Ogre::Real current, Ogre::Real previous, Ogre::Real alpha) noexcept
		{
			return alpha * current + (1.0f - alpha) * previous;
		}
	protected:
		std::vector<Ogre::Real> numberOfSamplesPerBar;
		int spectrumProcessingSize;
		Ogre::Real smoothFactor;
	};

	////////////////////////////////////////////////////////////////

	class SpectrumPreparationRaw : public SpectrumPreparation
	{
	public:
		SpectrumPreparationRaw();

		virtual ~SpectrumPreparationRaw();

		virtual int init(int spectrumProcessingSize, int numberOfBands, int frequency, Ogre::Real smoothFactor) override;

		virtual std::vector<Ogre::Real> getSpectrum(std::vector<Ogre::Real>& data) override;
	};

	////////////////////////////////////////////////////////////////

	class SpectrumPreparationLinear : public SpectrumPreparation
	{
	public:
		SpectrumPreparationLinear();

		virtual ~SpectrumPreparationLinear();

		virtual int init(int spectrumProcessingSize, int numberOfBands, int frequency, Ogre::Real smoothFactor) override;

		virtual std::vector<Ogre::Real> getSpectrum(std::vector<Ogre::Real>& data) override;
	};

	////////////////////////////////////////////////////////////////

	class SpectrumPreparationLogarithm : public SpectrumPreparation
	{
	public:
		SpectrumPreparationLogarithm();

		virtual ~SpectrumPreparationLogarithm();

		virtual int init(int spectrumProcessingSize, int numberOfBands, int frequency, Ogre::Real smoothFactor) override;

		virtual std::vector<Ogre::Real> getSpectrum(std::vector<Ogre::Real>& data) override;
	};

};

#endif

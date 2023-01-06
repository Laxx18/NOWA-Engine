#include "MathWindows.h"

namespace OgreAL
{

	std::vector<Ogre::Real> MathWindows::generateWindow(int numSamples, WindowType windowType)
	{
		if (windowType == MathWindows::HANNING)
		{
			std::vector<Ogre::Real> window = generateHanningWindow(numSamples);
			return window;
		}
		else if (windowType == MathWindows::HAMMING)
		{
			std::vector<Ogre::Real> window = generateHammingWindow(numSamples);
			return window;
		}
		else if (windowType == MathWindows::BLACKMAN)
		{
			std::vector<Ogre::Real> window = generateBlackmanWindow(numSamples);
			return window;
		}
		else if (windowType == MathWindows::BLACKMAN_HARRIS)
		{
			std::vector<Ogre::Real> window = generateBlackmanHarrisWindow(numSamples);
			return window;
		}
		else if (windowType == MathWindows::TUKEY)
		{
			std::vector<Ogre::Real> window = generateTukeyWindow(numSamples);
			return window;
		}
		else
		{
			std::vector<Ogre::Real> window = generateRectangularWindow(numSamples);
			return window;
		}
	}

	std::vector<Ogre::Real> MathWindows::generateRectangularWindow(int numSamples)
	{
		std::vector<Ogre::Real> window(numSamples);

		for (int i = 0; i < numSamples; i++)
		{
			window[i] = 1.0f;
		}
		return window;
	}

	std::vector<Ogre::Real> MathWindows::generateBlackmanWindow(int numSamples)
	{
		std::vector<Ogre::Real> window(numSamples);

		Ogre::Real numSamplesMinus1 = static_cast<Ogre::Real>(numSamples - 1);

		for (int i = 0; i < numSamples; i++)
			window[i] = 0.42f - (0.5f * Ogre::Math::Cos(2.0f * Ogre::Math::PI * ((Ogre::Real)i / numSamplesMinus1))) + (0.08f * Ogre::Math::Cos(4.0f * Ogre::Math::PI * ((Ogre::Real)i / numSamplesMinus1)));

		return window;
	}

	std::vector<Ogre::Real> MathWindows::generateBlackmanHarrisWindow(int numSamples)
	{
		std::vector<Ogre::Real> window(numSamples);

		const Ogre::Real a0 = 0.35875f;
		const Ogre::Real a1 = 0.48829f;
		const Ogre::Real a2 = 0.14128f;
		const Ogre::Real a3 = 0.01168f;

		Ogre::Real scale = 2 * Ogre::Math::PI / numSamples;

		for (unsigned short i = 0; i < numSamples; i++)
		{
			Ogre::Real i5 = i + 0.5f;
			Ogre::Real w = a0 - a1 * Ogre::Math::Cos(scale * i5) + a2 * Ogre::Math::Cos(scale * i5 * 2) - a3 * Ogre::Math::Cos(scale * i5 * 3);
			window[i] = w;
		}

		return window;
	}

	std::vector<Ogre::Real> MathWindows::generateTukeyWindow(int numSamples, Ogre::Real alpha)
	{
		std::vector<Ogre::Real> window(numSamples);

		Ogre::Real numSamplesMinus1 = (Ogre::Real) (numSamples - 1);

		Ogre::Real value = (Ogre::Real) (-1 * ((numSamples / 2))) + 1;

		for (int i = 0; i < numSamples; i++)
		{
			if ((value >= 0) && (value <= (alpha * (numSamplesMinus1 / 2))))
			{
				window[i] = 1.0f;
			}
			else if ((value <= 0) && (value >= (-1 * alpha * (numSamplesMinus1 / 2))))
			{
				window[i] = 1.0f;
			}
			else
			{
				window[i] = 0.5f * (1.0f + Ogre::Math::Cos(Ogre::Math::PI * (((2.0f * value) / (alpha * numSamplesMinus1)) - 1)));
			}

			value += 1;
		}

		return window;
	}

	std::vector<Ogre::Real> MathWindows::generateHanningWindow(int numSamples)
	{
		std::vector<Ogre::Real> window(numSamples);

		Ogre::Real numSamplesMinus1 = static_cast<Ogre::Real>(numSamples - 1);

		for (int i = 0; i < numSamples; i++)
		{
			window[i] = 0.5f * (1.0f - Ogre::Math::Cos(2.0f * Ogre::Math::PI * (i / numSamplesMinus1)));
		}

		return window;
	}

	std::vector<Ogre::Real> MathWindows::generateHammingWindow(int numSamples)
	{
		std::vector<Ogre::Real> window(numSamples);

		Ogre::Real numSamplesMinus1 = static_cast<Ogre::Real>(numSamples - 1);

		for (int i = 0; i < numSamples; i++)
		{
			window[i] = 0.54f - (0.46f * Ogre::Math::Cos(2.0f * Ogre::Math::PI * ((Ogre::Real)i / numSamplesMinus1)));
		}
		return window;
	}

	std::vector<Ogre::Real> MathWindows::generateBarlettWindow(int numSamples)
	{
		std::vector<Ogre::Real> window(numSamples, 0.0f);

		Ogre::Real a = 2.0f / (numSamples - 1);

		for (int i = 0; i < 2.0f / (numSamples - 1); i++)
		{
			window[i] = i * a;
		}

		for (int i = 0; i < numSamples; i++)
		{
			window[i] += 2.0f - (i * a);
		}
		return window;
	}

};
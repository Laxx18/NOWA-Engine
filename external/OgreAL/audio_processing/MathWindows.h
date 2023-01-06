#ifndef MATH_WINDOWS
#define MATH_WINDOWS

#include "OgreMath.h"
namespace OgreAL
{

	class MathWindows
	{
	public:
		enum WindowType
		{
			RECTANGULAR,
			BLACKMAN,
			BLACKMAN_HARRIS,
			TUKEY,
			HANNING,
			HAMMING,
			BARLETT
		};

		static std::vector<Ogre::Real> generateWindow(int numSamples, WindowType windowType);
		static std::vector<Ogre::Real> generateHanningWindow(int numSamples);
		static std::vector<Ogre::Real> generateHammingWindow(int numSamples);
		static std::vector<Ogre::Real> generateBlackmanWindow(int numSamples);
		static std::vector<Ogre::Real> generateBlackmanHarrisWindow(int numSamples);
		static std::vector<Ogre::Real> generateTukeyWindow(int numSamples, Ogre::Real alpha = 0.5f);
		static std::vector<Ogre::Real> generateRectangularWindow(int numSamples);
		static std::vector<Ogre::Real> generateBarlettWindow(int numSamples); // Triangle
	};

};

#endif

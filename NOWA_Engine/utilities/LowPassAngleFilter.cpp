#include "NOWAPrecompiled.h"
#include "LowPassAngleFilter.h"

namespace NOWA
{
	LowPassAngleFilter::LowPassAngleFilter()
		: alpha(0.05f),
		lastVal(0.0f),
		firstUpdate(true)
	{

	}

	void LowPassAngleFilter::setAlpha(Ogre::Real alpha)
	{
		this->alpha = alpha;
	}

	Ogre::Real LowPassAngleFilter::update(Ogre::Real val)
	{
		if (true == this->firstUpdate)
		{
			this->lastVal = val;
			this->firstUpdate = false;
		}
		else
		{
			Ogre::Real diff = val - this->lastVal;

			// Detect if the angle crosses the 180 degree boundary
			if (diff > 180.0f)
			{
				this->lastVal += 360.0f; // Adjust lastVal for continuity
			}
			else if (diff < -180.0f)
			{
				this->lastVal -= 360.0f; // Adjust lastVal for continuity
			}

			this->lastVal = (this->alpha * val) + ((1.0f - this->alpha) * this->lastVal);
		}

		// Normalize lastVal to be within -180 to 180 degrees
		if (this->lastVal > 180.0f)
		{
			this->lastVal -= 360.0f;
		}
		else if (lastVal < -180.0f)
		{
			this->lastVal += 360.0f;
		}

		return this->lastVal;
	}

}; // namespace end
#ifndef LOW_PASS_ANGLE_FILTER_H
#define LOW_PASS_ANGLE_FILTER_H

#include "defines.h"

namespace NOWA
{
	class EXPORTED LowPassAngleFilter
	{
	public:
		LowPassAngleFilter();

		void setAlpha(Ogre::Real alpha);

		Ogre::Real update(Ogre::Real val);

	private:
		Ogre::Real alpha;
		Ogre::Real lastVal;
		bool firstUpdate;
	};

}; // namespace end

#endif

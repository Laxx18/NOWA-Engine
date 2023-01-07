#ifndef VEHICLE_H
#define VEHICLE_H

#include "defines.h"

namespace NOWA
{
	class EXPORTED Tire
	{
	public:

		Tire();

		virtual ~Tire();

	private:
		Ogre::String vehicleName;

		
	};

}; //namespace end

#endif
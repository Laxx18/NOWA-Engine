#ifndef VEHICLE_DRIVE_MANIPULATION_H
#define VEHICLE_DRIVE_MANIPULATION_H

#include "defines.h"

namespace NOWA
{
	class EXPORTED VehicleDrivingManipulation
	{
	public:
		VehicleDrivingManipulation();

		~VehicleDrivingManipulation();

		void setSteerAngle(Ogre::Real steerAngle);

		Ogre::Real getSteerAngle(void) const;

		void setMotorForce(Ogre::Real motorForce);

		Ogre::Real getMotorForce(void) const;

		void setHandBrake(Ogre::Real handBrake);

		Ogre::Real getHandBrake(void) const;

		void setBrake(Ogre::Real brake);

		Ogre::Real getBrake(void) const;

	private:
		Ogre::Real steerAngle;
		Ogre::Real motorForce;
		Ogre::Real handBrake;
		Ogre::Real brake;
	};

}; //namespace end

#endif
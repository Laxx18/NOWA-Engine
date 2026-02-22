#include "NOWAPrecompiled.h"
#include "VehicleDrivingManipulation.h"

namespace NOWA
{
	VehicleDrivingManipulation::VehicleDrivingManipulation()
		: steerAngle(0.0f),
		motorForce(0.0f),
		handBrake(0.0f),
		brake(0.0f)
	{

	}

	VehicleDrivingManipulation::~VehicleDrivingManipulation()
	{

	}

	void VehicleDrivingManipulation::setSteerAngle(Ogre::Real steerAngle)
	{
		this->steerAngle = steerAngle;
	}

	Ogre::Real VehicleDrivingManipulation::getSteerAngle(void) const
	{
		return this->steerAngle;
	}

	void VehicleDrivingManipulation::setMotorForce(Ogre::Real motorForce)
	{
		this->motorForce = motorForce;
	}

	Ogre::Real VehicleDrivingManipulation::getMotorForce(void) const
	{
		return this->motorForce;
	}

	void VehicleDrivingManipulation::setHandBrake(Ogre::Real handBrake)
	{
		this->handBrake = handBrake;
	}

	Ogre::Real VehicleDrivingManipulation::getHandBrake(void) const
	{
		return this->handBrake;
	}

	void VehicleDrivingManipulation::setBrake(Ogre::Real brake)
	{
		this->brake = brake;
	}

	Ogre::Real VehicleDrivingManipulation::getBrake(void) const
	{
		return this->brake;
	}
}; // namespace end
/*
OgreNewt Library

Ogre implementation of Newton Game Dynamics SDK

OgreNewt basically has no license, you may use any or all of the library however you desire... I hope it can help you in any way.

by Walaber
some changes by melven*/

// Attention: Vehicle has been totally refactored in newton and must be adapted!

#ifndef _INCLUDE_OGRENEWT_VEHICLE
#define _INCLUDE_OGRENEWT_VEHICLE

#include "OgreNewt_Prerequisites.h"
#include "dCustomHinge.h"
#include "dCustomGear.h"
#include "dCustomDoubleHinge.h"
// #include "dCustomVehicleControllerManager.h"

#if 0

namespace OgreNewt
{
	class Body;

	struct CarDefinition
	{
		Ogre::Real m_vehicleMass;
		Ogre::Real m_engineMass;
		Ogre::Real m_tireMass;
		Ogre::Real m_engineRotorRadio;
		Ogre::Real m_frontSteeringAngle;
		Ogre::Real m_rearSteeringAngle;
		Ogre::Real m_vehicleWeightDistribution;
		Ogre::Real m_cluthFrictionTorque;
		Ogre::Real m_engineIdleTorque;
		Ogre::Real m_engineRPMAtIdleTorque;
		Ogre::Real m_enginePeakTorque;
		Ogre::Real m_engineRPMAtPeakTorque;
		Ogre::Real m_enginePeakHorsePower;
		Ogre::Real m_egineRPMAtPeakHorsePower;
		Ogre::Real m_engineRPMAtRedLine;
		Ogre::Real m_vehicleTopSpeed;
		Ogre::Real m_corneringStiffness;
		Ogre::Real m_tireAligningMomemtTrail;
		Ogre::Real m_tireSuspensionSpringConstant;
		Ogre::Real m_tireSuspensionDamperConstant;
		Ogre::Real m_tireSuspensionLength;
		Ogre::Real m_tireBrakesTorque;
		Ogre::Real m_tirePivotOffset;
		Ogre::Real m_tireVerticalOffsetTweak;
		Ogre::Real m_transmissionGearRatio0;
		Ogre::Real m_transmissionGearRatio1;
		Ogre::Real m_transmissionGearRatio2;
		Ogre::Real m_transmissionGearRatio3;
		Ogre::Real m_transmissionGearRatio4;
		Ogre::Real m_transmissionGearRatio6;
		Ogre::Real m_transmissionRevereGearRatio;
		Ogre::Real m_chassisYaxisComBias;
		Ogre::Real m_aerodynamicsDownForceWeightCoeffecient0;
		Ogre::Real m_aerodynamicsDownForceWeightCoeffecient1;
		Ogre::Real m_aerodynamicsDownForceSpeedFactor;
		int m_wheelHasCollisionFenders;
		int m_differentialType; // 0 rear wheel drive, 1 front wheel drive, 3 four wheel drive
		dSuspensionType m_tireSuspensionType;
	};

	class _OgreNewtExport Vehicle
	{
	public:
		Vehicle(CarDefinition& carDefinition);

		~Vehicle();

		void createVehicle(const Ogre::Vector3& position, const Ogre::Quaternion& orientation, 
			OgreNewt::Body* chassis, OgreNewt::Body* lfTire, OgreNewt::Body* rfTire, OgreNewt::Body* lrTire, OgreNewt::Body* rrTire);

		dVehicleDriverInput& getDriverInput(void);

		void update(Ogre::Real dt);
	private:
		void setGearMap(dEngineController* const engine);

		dWheelJoint* addTire(OgreNewt::Body* tire, Ogre::Real pivotOffset, Ogre::Real maxSteeringAngle);
	private:
		static void tireTransformCallback(const NewtonBody* const tireBody, const dFloat* const tireMatrix, int threadIndex);
	private:
		OgreNewt::Body* m_chassis;
		OgreNewt::Body* m_lfTire;
		OgreNewt::Body* m_rfTire;
		OgreNewt::Body* m_lrTire;
		OgreNewt::Body* m_rrTire;
		CarDefinition m_carDefinition;
		dCustomVehicleController* m_customVehicleController;
		dCustomVehicleControllerManager* m_customVehicleControllerManager;
		int m_gearMap[10];
		dVehicleDriverInput m_driverInput;
	};

};

#endif

#endif


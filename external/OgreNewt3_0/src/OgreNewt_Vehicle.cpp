#include "OgreNewt_Stdafx.h"
#include "OgreNewt_Vehicle.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_World.h"

#if 0

namespace OgreNewt
{
	Vehicle::Vehicle(CarDefinition& carDefinition)
		: m_carDefinition(carDefinition),
		m_chassis(nullptr),
		m_lfTire(nullptr),
		m_rfTire(nullptr),
		m_lrTire(nullptr),
		m_rrTire(nullptr),
		m_customVehicleController(nullptr),
		m_customVehicleControllerManager(nullptr)
	{

	}

	Vehicle::~Vehicle()
	{
		((dCustomVehicleControllerManager*)m_customVehicleController->GetManager())->DestroyController(m_customVehicleController);
	}

	void Vehicle::createVehicle(const Ogre::Vector3& position, const Ogre::Quaternion& orientation, OgreNewt::Body* chassis,
		OgreNewt::Body* lfTire, OgreNewt::Body* rfTire, OgreNewt::Body* lrTire, OgreNewt::Body* rrTire)
	{
		int materialList[] = { chassis->getMaterialGroupID()->getID() };

		m_customVehicleControllerManager = new dCustomVehicleControllerManager(chassis->getWorld()->getNewtonWorld(), 1, materialList);

		dMatrix chassisMatrix;
#if 1
		chassisMatrix.m_front = dVector(1.0f, 0.0f, 0.0f, 0.0f);			// this is the vehicle direction of travel
#else
		chassisMatrix.m_front = dVector(0.0f, 0.0f, 1.0f, 0.0f);			// this is the vehicle direction of travel
#endif
		chassisMatrix.m_up = dVector(0.0f, 1.0f, 0.0f, 0.0f);			// this is the downward vehicle direction
		chassisMatrix.m_right = chassisMatrix.m_front.CrossProduct(chassisMatrix.m_up);	// this is in the side vehicle direction (the plane of the wheels)
		chassisMatrix.m_posit = dVector(0.0f, 0.0f, 0.0f, 1.0f);

		// create a default vehicle 
		m_customVehicleController = m_customVehicleControllerManager->CreateVehicle(chassis->getNewtonCollision(), chassisMatrix, 
			m_carDefinition.m_vehicleMass, chassis->newtonForceTorqueCallback, chassis->getGravity().y);

		// get body from player
		NewtonBody* const body = m_customVehicleController->GetBody();

		// set the user data
		NewtonBodySetUserData(body, this);

		// set the transform callback
		NewtonBodySetTransformCallback(body, chassis->newtonTransformCallback);

		dFloat location[16];
		OgreNewt::Converters::QuatPosToMatrix(orientation, position, location);

		// set the player matrix 
		NewtonBodySetMatrix(body, &location[0]);

		// destroy the collision helper shape 
		// NewtonDestroyCollision(chassisCollision);

		for (int i = 0; i < int((sizeof(m_gearMap) / sizeof(m_gearMap[0]))); i++)
		{
			m_gearMap[i] = i;
		}



		// step one: find the location of each tire, in the visual mesh and add them one by one to the vehicle controller 
		
		// Muscle cars have the front engine, we need to shift the center of mass to the front to represent that
		m_customVehicleController->SetCenterOfGravity(dVector(0.0f, m_carDefinition.m_chassisYaxisComBias, 0.0f, 0.0f));

		// add front axle
		// a car may have different size front an rear tire, therefore we do this separate for front and rear tires
		//		dVector offset (0.0f, 0.0f, 0.0f, 0.0f);
		dWheelJoint* const leftFrontTire = addTire(lfTire, 0.25f, m_carDefinition.m_frontSteeringAngle);
		dWheelJoint* const rightFrontTire = addTire(rfTire, -0.25f, m_carDefinition.m_frontSteeringAngle);

		// add rear axle
		// a car may have different size front an rear tire, therefore we do this separate for front and rear tires
		// CalculateTireDimensions("rl_tire", width, radius);
		dWheelJoint* const leftRearTire = addTire(lrTire, 0.0f, m_carDefinition.m_rearSteeringAngle);
		dWheelJoint* const rightRearTire = addTire(rrTire, 0.0f, m_carDefinition.m_rearSteeringAngle);

		// add a steering Wheel component
		dSteeringController* const steering = new dSteeringController(m_customVehicleController);
		steering->AddTire(leftFrontTire);
		steering->AddTire(rightFrontTire);
		//steering->AddTire(leftRearTire);
		//steering->AddTire(rightRearTire);
		m_customVehicleController->SetSteering(steering);

		// add vehicle brakes
		dBrakeController* const brakes = new dBrakeController(m_customVehicleController, m_carDefinition.m_tireBrakesTorque);
		brakes->AddTire(leftFrontTire);
		brakes->AddTire(rightFrontTire);
		brakes->AddTire(leftRearTire);
		brakes->AddTire(rightRearTire);
		m_customVehicleController->SetBrakes(brakes);

		// add vehicle hand brakes
		dBrakeController* const handBrakes = new dBrakeController(m_customVehicleController, m_carDefinition.m_tireBrakesTorque * 2.0f);
		handBrakes->AddTire(leftRearTire);
		handBrakes->AddTire(rightRearTire);
		m_customVehicleController->SetHandBrakes(handBrakes);

		// add the engine, differential and transmission 
		dEngineInfo engineInfo;
		engineInfo.m_mass = m_carDefinition.m_engineMass;
		engineInfo.m_radio = m_carDefinition.m_engineRotorRadio;
		engineInfo.m_vehicleTopSpeed = m_carDefinition.m_vehicleTopSpeed;
		engineInfo.m_clutchFrictionTorque = m_carDefinition.m_cluthFrictionTorque;

		engineInfo.m_idleTorque = m_carDefinition.m_engineIdleTorque;
		engineInfo.m_rpmAtIdleTorque = m_carDefinition.m_engineRPMAtIdleTorque;
		engineInfo.m_peakTorque = m_carDefinition.m_enginePeakTorque;
		engineInfo.m_rpmAtPeakTorque = m_carDefinition.m_engineRPMAtPeakTorque;
		engineInfo.m_peakHorsePower = m_carDefinition.m_enginePeakHorsePower;
		engineInfo.m_rpmAtPeakHorsePower = m_carDefinition.m_egineRPMAtPeakHorsePower;
		engineInfo.m_rpmAtRedLine = m_carDefinition.m_engineRPMAtRedLine;

		engineInfo.m_gearsCount = 6;
		engineInfo.m_gearRatios[0] = m_carDefinition.m_transmissionGearRatio0;
		engineInfo.m_gearRatios[1] = m_carDefinition.m_transmissionGearRatio1;
		engineInfo.m_gearRatios[2] = m_carDefinition.m_transmissionGearRatio2;
		engineInfo.m_gearRatios[3] = m_carDefinition.m_transmissionGearRatio3;
		engineInfo.m_gearRatios[4] = m_carDefinition.m_transmissionGearRatio4;
		engineInfo.m_gearRatios[5] = m_carDefinition.m_transmissionGearRatio6;
		engineInfo.m_reverseGearRatio = m_carDefinition.m_transmissionRevereGearRatio;
		engineInfo.m_gearRatiosSign = 1.0f;

		dDifferentialJoint* differential = nullptr;
		switch (m_carDefinition.m_differentialType)
		{
			case 0:
			{
				// rear wheel drive differential vehicle
				engineInfo.m_gearRatiosSign = 1.0f;
				differential = m_customVehicleController->AddDifferential(leftRearTire, rightRearTire);
				break;
			}

			case 1:
			{
				// front wheel drive vehicle with differential
				engineInfo.m_gearRatiosSign = 1.0f;
				differential = m_customVehicleController->AddDifferential(leftFrontTire, rightFrontTire);
				break;
			}

			default:
			{
				dDifferentialJoint* const rearDifferential = m_customVehicleController->AddDifferential(leftRearTire, rightRearTire);
				dDifferentialJoint* const frontDifferential = m_customVehicleController->AddDifferential(leftFrontTire, rightFrontTire);
				differential = m_customVehicleController->AddDifferential(rearDifferential, frontDifferential);

				engineInfo.m_gearRatiosSign = -1.0f;
			}
		}
		dAssert(differential);

		engineInfo.m_differentialLock = 0;
		engineInfo.m_aerodynamicDownforceFactor = m_carDefinition.m_aerodynamicsDownForceWeightCoeffecient0;
		engineInfo.m_aerodynamicDownforceFactorAtTopSpeed = m_carDefinition.m_aerodynamicsDownForceWeightCoeffecient1;
		engineInfo.m_aerodynamicDownForceSurfaceCoeficident = m_carDefinition.m_aerodynamicsDownForceSpeedFactor / m_carDefinition.m_vehicleTopSpeed;

		m_customVehicleController->AddEngineJoint(engineInfo.m_mass, engineInfo.m_radio);
		dEngineController* const engineControl = new dEngineController(m_customVehicleController, engineInfo, differential, rightRearTire);
		m_customVehicleController->SetEngine(engineControl);

		// the the default transmission type
		// engineControl->SetTransmissionMode(m_automaticTransmission.GetPushButtonState() ? true : false);
	// Attention: false, or true?? Maybe a parameter
		engineControl->SetTransmissionMode(0);
		// engineControl->SetTransmissionMode(m_automaticTransmission.GetPushButtonState() ? true : false);
		engineControl->SetIgnition(true);

		// trace the engine curve
		//engineControl->PlotEngineCurve ();

		// set the gear look up table
		setGearMap(engineControl);


		// set the vehicle weigh distribution 
		m_customVehicleController->SetWeightDistribution(m_carDefinition.m_vehicleWeightDistribution);

		// set the vehicle aerodynamics
		dFloat weightRatio0 = m_carDefinition.m_aerodynamicsDownForceWeightCoeffecient0;
		dFloat weightRatio1 = m_carDefinition.m_aerodynamicsDownForceWeightCoeffecient1;
		dFloat speedFactor = m_carDefinition.m_aerodynamicsDownForceSpeedFactor / m_carDefinition.m_vehicleTopSpeed;
		m_customVehicleController->SetAerodynamicsDownforceCoefficient(weightRatio0, speedFactor, weightRatio1);

		// do not forget to call finalize after all components are added or after any change is made to the vehicle
		m_customVehicleController->Finalize();
	}

	dVehicleDriverInput & Vehicle::getDriverInput(void)
	{
		return m_driverInput;
	}

	void Vehicle::update(Ogre::Real dt)
	{
		if (nullptr != m_customVehicleController)
		{
			dEngineController* const engine = m_customVehicleController->GetEngine();

			int gear = engine ? engine->GetGear() : 0;

			m_customVehicleController->ApplyDefualtDriver(m_driverInput, dt);
		}
	}

	void Vehicle::setGearMap(dEngineController* const engine)
	{
		int start = engine->GetFirstGear();
		int count = engine->GetLastGear() - start;
		for (int i = 0; i < count; i++)
		{
			m_gearMap[i + 2] = start + i;
		}
		m_gearMap[0] = engine->GetNeutralGear();
		m_gearMap[1] = engine->GetReverseGear();
	}

	// this transform make sure the tire matrix is relative to the chassis 
	// Note: this transform us only use because the tire in the model are children of the chassis
	void Vehicle::tireTransformCallback(const NewtonBody* const tireBody, const dFloat* const tireMatrix, int threadIndex)
	{
		OgreNewt::Body* const tire = (OgreNewt::Body*)NewtonBodyGetUserData(tireBody);

		dCustomVehicleController* const controller = OgreNewt::any_cast<dCustomVehicleController*>(tire->getUserData2());

		NewtonBody* const chassisBody = controller->GetBody();

		// calculate the local space matrix,
		dMatrix parentMatrix;
		dMatrix matrix(tireMatrix);
		NewtonBodyGetMatrix(chassisBody, &parentMatrix[0][0]);
		matrix = matrix * parentMatrix.Inverse();
		dQuaternion rot(matrix);

		tire->setPositionOrientation(Ogre::Vector3(matrix.m_posit.m_x, matrix.m_posit.m_y, matrix.m_posit.m_z), Ogre::Quaternion(rot.m_x, rot.m_y, rot.m_z, rot.m_w));
	}

	dWheelJoint* Vehicle::addTire(OgreNewt::Body* tire, Ogre::Real pivotOffset, Ogre::Real maxSteeringAngle)
	{
		NewtonBody* const chassisBody = m_customVehicleController->GetBody();

		// Store the vehicle controller for transform callback
		tire->setUserData2(OgreNewt::Any(dynamic_cast<dCustomVehicleController*>(m_customVehicleController)));

		dFloat location[16];
		OgreNewt::Converters::QuatPosToMatrix(tire->getOrientation(), tire->getPosition(), location);

		// for simplicity, tires are position in global space
		dMatrix tireMatrix(&location[0]);

		// add the offset to the tire position to account for suspension span
		//tireMatrix.m_posit += m_controller->GetUpAxis().Scale (definition.m_tirePivotOffset);
		tireMatrix.m_posit -= m_customVehicleController->GetUpAxis().Scale(m_carDefinition.m_tireVerticalOffsetTweak);

		// add the tire to the vehicle
		dTireInfo tireInfo;

		tireInfo.m_mass = m_carDefinition.m_tireMass;
		// Attention, is that correct?
		tireInfo.m_radio = tire->getAABB().getSize().z * 0.5f;
		tireInfo.m_width = tire->getAABB().getSize().x;
		tireInfo.m_pivotOffset = pivotOffset;
		tireInfo.m_maxSteeringAngle = maxSteeringAngle * dDegreeToRad;
		tireInfo.m_dampingRatio = m_carDefinition.m_tireSuspensionDamperConstant;
		tireInfo.m_springStrength = m_carDefinition.m_tireSuspensionSpringConstant;
		tireInfo.m_suspensionLength = m_carDefinition.m_tireSuspensionLength;
		tireInfo.m_corneringStiffness = m_carDefinition.m_corneringStiffness;
		tireInfo.m_aligningMomentTrail = m_carDefinition.m_tireAligningMomemtTrail;
		tireInfo.m_hasFender = m_carDefinition.m_wheelHasCollisionFenders;
		tireInfo.m_suspentionType = m_carDefinition.m_tireSuspensionType;

		dWheelJoint* const tireJoint = m_customVehicleController->AddTire(tireMatrix, tireInfo);
		NewtonBody* const tireBody = tireJoint->GetTireBody();

		// add the user data and a tire transform callback that calculate the tire local space matrix
		NewtonBodySetUserData(tireBody, tire);
		NewtonBodySetTransformCallback(tireBody, tireTransformCallback);
		return tireJoint;
	}
};

#endif
#include "OgreNewt_Stdafx.h"
#include "OgreNewt_Vehicle.h"

#include <algorithm>

using namespace OgreNewt;

// -----------------------------------------------------------------------------
// RayCastTire
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
// RayCastTire
// -----------------------------------------------------------------------------
RayCastTire::RayCastTire(ndWorld* world, const ndMatrix& pinAndPivotFrame, const ndVector& pin, 
	Body* child, Body* parentChassisBody, Vehicle* parent, const TireConfiguration& tireConfiguration, ndReal radius)
	// : ndJointBilateralConstraint(3, parentChassisBody->getNewtonBody(), parentChassisBody->getNewtonBody()->GetScene()->GetSentinelBody(), /*pinAndPivotFrame*/ndGetIdentityMatrix())
	 // : ndJointBilateralConstraint(4,                      // allow up to 4 rows (brake + friction etc.)
	 // 	child->getNewtonBody(),                          // child: tire
	 // 	parentChassisBody->getNewtonBody(),              // parent: chassis
	 // 	pinAndPivotFrame)
	// Exactly like ND3:
	: ndJointBilateralConstraint(3, parentChassisBody->getNewtonBody(), world->GetSentinelBody(), /*pinAndPivotFrame*/ndGetIdentityMatrix())
	, m_hitParam(1.1f)
	, m_hitBody(nullptr)
	, m_penetration(0.0f)
	, m_contactID(0)
	, m_hitContact(ndVector::m_zero)
	, m_hitNormal(ndVector::m_zero)
	, m_vehicle(parent)
	, m_tireConfiguration(tireConfiguration)
	, m_lateralPin(ndVector::m_zero)
	, m_longitudinalPin(ndVector::m_zero)
	, m_localAxis(ndVector::m_zero)
	, m_brakeForce(0.0f)
	, m_motorForce(0.0f)
	, m_isOnContact(false)
	, m_chassisBody(parentChassisBody->getNewtonBody())
	, m_thisBody(child)
	, m_pin(pin)
	, m_globalTireMatrix(ndGetIdentityMatrix())
	, m_suspensionMatrix(ndGetIdentityMatrix())
	, m_hardPoint(ndVector::m_zero)
	, m_jointBrakeForce(0.0f)
	, m_tireRenderMatrix(ndGetIdentityMatrix())
	, m_frameLocalMatrix(ndGetIdentityMatrix())
	, m_angleMatrix(ndGetIdentityMatrix())
	, m_steerAngle(0.0f)
	, m_isOnContactEx(false)
	, m_arealpos(0.0f)
	, m_spinAngle(0.0f)
	, m_posit_y(0.0f)
	, m_radius(radius)
	, m_width(0.15f)
	, m_tireLoad(0.0f)
	, m_suspenssionHardLimit(0.0f)
	, m_chassisSpeed(0.0f)
	, m_tireSpeed(0.0f)
	, m_realvelocity(ndVector(0.0f))
	, m_TireAxelPosit(ndVector(1.0f))
	, m_LocalAxelPosit(ndVector(1.0f))
	, m_TireAxelVeloc(ndVector(0.0f))
	, m_suspenssionFactor(0.5f)
	, m_suspenssionStep(1.0f / 60.0f)
	, m_tireID(-1)
	, m_handForce(0.0f)
{
	// SetSolverModel(ndJointBilateralSolverModel::m_jointIterativeSoft);

	// m_body0 = vehicleBody or chassis body
	// m_body0 = parentChassisBody->getNewtonBody();

	// m_body1 = The current tire body
	// m_body1 = child->getNewtonBody();
	// m_thisBody = the current tire body also
	// m_body1 = nullptr;

	m_collision = parentChassisBody->getNewtonCollision();


	ndVector com(parent->m_combackup);
	com.m_w = 1.0f;

	// Transform of chassis
	ndMatrix chassisMatrix = m_chassisBody->GetMatrix();

	chassisMatrix.m_posit += chassisMatrix.RotateVector(com);
	
	CalculateLocalMatrix(chassisMatrix, m_localMatrix0, m_localMatrix1);

	// Tire matrix: transform of this tire body, then converted to local matrices
	ndMatrix tireMatrix = child->getNewtonBody()->GetMatrix();

	// Note: m_localTireMatrix must be really local. 
	// // E.g. if Car is created in the air with y = 2.5, the tire must still be local to its chassis and having just a small amount of y!
	CalculateLocalMatrix(tireMatrix, m_localTireMatrix, m_globalTireMatrix);

	m_vehicle->SetChassisMatrix(chassisMatrix);

	// Equivalent for: setJointRecursiveCollision?
	this->SetCollidable(false);

	// Is already done in basic joints VehicleTire 
	// world->AddJoint(this);
}

RayCastTire::~RayCastTire()
{
}

void RayCastTire::UpdateParameters()
{
	// ND3 used SubmitConstraints here; ND4 uses internal Jacobian building.
	// We keep all scalar state updated from updateGroundProbe / Vehicle::Update,
	// but do not rebuild rows here – your current ND4 setup uses impulses instead.
}

Ogre::Real RayCastTire::ApplySuspenssionLimit()
{
	Ogre::Real distance;
	distance = (m_tireConfiguration.springLength - m_posit_y);
	//if (distance > m_springLength)
	if (distance >= m_tireConfiguration.springLength)
	{
		distance = (m_tireConfiguration.springLength - m_posit_y) + m_suspenssionHardLimit;
	}
	return distance;
}

void OgreNewt::RayCastTire::ProcessConvexContacts(OgreNewt::BasicConvexcast::ConvexcastContactInfo& info)
{
	if (info.mBody)
	{
		// TODO: skip the chassis body also?
		/*if (info.mBody == m_vehicle)
		{
			return;
		}*/

		info.mBody->unFreeze();

		ndVector massMatrix = info.mBody->getNewtonBody()->GetMassMatrix();
		ndFloat32 mass = massMatrix.m_w;
		ndFloat32 invMass = info.mBody->getNewtonBody()->GetInvMass();

		// Skip static/kinematic bodies
		/*if (invMass == 0.0f)
		{
			return;
		}*/

		if (mass > 0.0f)
		{
			// TODO: 1.0f or 0.0f for w?
			ndVector uForce(
				info.mContactNormal[0] * -(m_vehicle->m_mass + mass) * 2.0f,
				info.mContactNormal[1] * -(m_vehicle->m_mass + mass) * 2.0f,
				info.mContactNormal[2] * -(m_vehicle->m_mass + mass) * 2.0f,
				0.0f);

			m_vehicle->ApplyForceAndTorque(info.mBody->getNewtonBody(),
				uForce, ndVector(info.mContactPoint[0], info.mContactPoint[1], info.mContactPoint[2], 1.0f));
		}
	}
}

ndMatrix RayCastTire::CalculateSuspensionMatrixLocal(Ogre::Real distance)
{
	// Info: Do not touch, just controls the visualisation of the tires
	ndMatrix matrix;
	// calculate the steering angle matrix for the axis of rotation
	matrix.m_front = m_localAxis.Scale(-1.0f);
	matrix.m_up = ndVector(0.0f, 1.0f, 0.0f, 0.0f);
	matrix.m_right = ndVector(m_localAxis.m_z, 0.0f, -m_localAxis.m_x, 0.0f);
	matrix.m_posit = m_hardPoint + m_localMatrix0.m_up.Scale(distance);

	return matrix;
}

ndMatrix RayCastTire::CalculateTireMatrixAbsolute(Ogre::Real sSide)
{
	ndMatrix am;
	if (m_pin.m_z == 1.0f || m_pin.m_z == -1.0f)
	{
		m_angleMatrix = m_angleMatrix * ndRollMatrix((m_spinAngle * sSide) * 60.0f * ndDegreeToRad);
	}
	else if (m_pin.m_x == 1.0f || m_pin.m_x == -1.0f)
	{
		m_angleMatrix = m_angleMatrix * ndPitchMatrix((m_spinAngle * sSide) * 60.0f * ndDegreeToRad);
	}
	else
	{
		m_angleMatrix = m_angleMatrix * ndYawMatrix((m_spinAngle * sSide) * 60.0f * ndDegreeToRad);
	}

	am = m_angleMatrix * CalculateSuspensionMatrixLocal(m_tireConfiguration.springLength);
	return  am;
}

void RayCastTire::ApplyBrakes(Ogre::Real bkforce, ndConstraintDescritor& desc)
{
	m_jointBrakeForce = bkforce;
	this->AddLinearRowJacobian(desc, &m_TireAxelPosit[0], &m_TireAxelPosit[0], &m_longitudinalPin.m_x);
	this->SetHighFriction(desc, bkforce * 1000.0f);
	this->SetLowerFriction(desc, -bkforce * 1000.0f);
}

ndMatrix RayCastTire::GetLocalMatrix0()
{
	return m_localMatrix0;
}

void RayCastTire::LongitudinalAndLateralFriction(ndVector tireposit, ndVector lateralpin, ndReal turnfriction, ndReal sidingfriction, ndConstraintDescritor& desc)
{
	ndReal invMag2 = 0.0f;
	ndReal frictionCircleMag = 0.0f;
	ndReal lateralFrictionForceMag = 0.0f;
	ndReal longitudinalFrictionForceMag = 0.0f;

	if (m_tireLoad > 0.0f)
	{
		frictionCircleMag = m_tireLoad * turnfriction;
		lateralFrictionForceMag = frictionCircleMag;
		
		longitudinalFrictionForceMag = m_tireLoad * sidingfriction;

		invMag2 = frictionCircleMag / ndSqrt(lateralFrictionForceMag * lateralFrictionForceMag + longitudinalFrictionForceMag * longitudinalFrictionForceMag);

		lateralFrictionForceMag = lateralFrictionForceMag * invMag2;

		if (desc.m_rowsCount < m_maxDof)
		{
			AddLinearRowJacobian(desc, &tireposit[0], &tireposit[0], &lateralpin.m_x);
			SetHighFriction(desc, lateralFrictionForceMag);
			SetLowerFriction(desc, -lateralFrictionForceMag);
		}
	}
}

void RayCastTire::ProcessPreUpdate(void)
{
	if (false == m_vehicle->m_canDrive)
	{
		return;
	}

	m_hitBody = nullptr;
	m_isOnContact = false;
	m_isOnContactEx = false;
	m_penetration = 0.0f;

	ndMatrix bodyMatrix = m_body0->GetMatrix();
	const ndMatrix& localMatrix0 = GetLocalMatrix0();

	ndMatrix absoluteChassisMatrix(bodyMatrix * localMatrix0);

	m_frameLocalMatrix = m_localMatrix0;

	if (m_tireConfiguration.tireSteer == tsSteer)
	{
		const ndFloat32 angle = ndFloat32(m_steerAngle * (int)m_tireConfiguration.steerSide) * ndDegreeToRad;
		m_localAxis.m_x = ndSin(angle);
		m_localAxis.m_z = ndCos(angle);
	}

	m_posit_y = m_tireConfiguration.springLength;

	ndMatrix curSuspenssionMatrix = m_suspensionMatrix * absoluteChassisMatrix;

	ndFloat32 tiredist = m_tireConfiguration.springLength;
	m_hitParam = ndFloat32(1.1f);

	ndVector mRayDestination = curSuspenssionMatrix.TransformVector(m_frameLocalMatrix.m_up.Scale(-tiredist));
	ndVector p0 = curSuspenssionMatrix.m_posit;
	ndVector p1 = mRayDestination;

	const Ogre::Vector3 startpt(p0.m_x, p0.m_y, p0.m_z);
	const Ogre::Vector3 endpt(p1.m_x, p1.m_y, p1.m_z);

	Ogre::Quaternion orientation;
	Ogre::Vector3 dummyPos;
	OgreNewt::Converters::MatrixToQuatPos(curSuspenssionMatrix, orientation, dummyPos);

	OgreNewt::World* world = m_vehicle->getWorld();

	const ndShapeInstance& tireShape = *m_collision;

	OgreNewt::BasicConvexcast convex(world, tireShape, startpt, orientation, endpt, 1,  0, m_thisBody);

	BasicConvexcast::ConvexcastContactInfo info = world->convexcastBlocking(tireShape, startpt, orientation, endpt, m_thisBody, 1, 0);

	OgreNewt::Body* m_hitOgreNewtBody = nullptr;

	if (convex.getContactsCount() > 0)
	{
		info = convex.getInfoAt(0);
		m_hitOgreNewtBody = info.mBody;

		if (nullptr != m_hitOgreNewtBody)
		{
			for (ndList<RayCastTire*>::ndNode* node = m_vehicle->m_tires.GetFirst(); node; node = node->GetNext())
			{
				RayCastTire* otherTire = node->GetInfo();
				if (m_hitOgreNewtBody == otherTire->m_thisBody)
				{
					m_hitOgreNewtBody = nullptr;
					break;
				}
			}
		}

		if (nullptr != m_hitOgreNewtBody)
		{
			m_hitBody = m_hitOgreNewtBody->getNewtonBody();

			m_hitParam = static_cast<ndFloat32>(convex.getDistanceToFirstHit());
			m_contactID = 0;

			m_hitContact = ndVector(info.mContactPoint.x, info.mContactPoint.y, info.mContactPoint.z, ndFloat32(1.0f));
			m_hitNormal = ndVector(info.mContactNormal.x, info.mContactNormal.y, info.mContactNormal.z, ndFloat32(0.0f));

			m_penetration = static_cast<ndFloat32>(info.mContactPenetration);

			m_vehicle->GetVehicleCallback()->onTireContact(this, m_thisBody->getOgreNode()->getName(),
				m_hitOgreNewtBody, info.mContactPoint, info.mContactNormal, m_penetration);
		}
	}

	if (m_hitBody)
	{
		m_vehicle->m_noHitCounter = -1;
		m_isOnContactEx = true;
		m_isOnContact = true;

		ndFloat32 intesectionDist = ndFloat32(0.0f);

		intesectionDist = (tiredist * m_hitParam);

		if (intesectionDist < ndFloat32(0.0f))
		{
			intesectionDist = ndFloat32(0.0f);
		}
		else if (intesectionDist > ndFloat32(m_tireConfiguration.springLength))
		{
			intesectionDist = ndFloat32(m_tireConfiguration.springLength);
		}

		m_posit_y = intesectionDist;

		ndVector mChassisVelocity = m_vehicle->getNewtonBody()->GetVelocity();
		ndVector mChassisOmega = m_vehicle->getNewtonBody()->GetOmega();

		m_TireAxelPosit = absoluteChassisMatrix.TransformVector(m_hardPoint - m_frameLocalMatrix.m_up.Scale(m_posit_y));
		m_LocalAxelPosit = m_TireAxelPosit - absoluteChassisMatrix.m_posit;
		m_TireAxelVeloc = mChassisVelocity + mChassisOmega.CrossProduct(m_LocalAxelPosit);

		if (m_posit_y < m_tireConfiguration.springLength)
		{
			ndVector hitBodyVeloc = m_hitBody->GetVelocity();

			const ndVector relVeloc = m_TireAxelVeloc - hitBodyVeloc;
			m_realvelocity = relVeloc;
			m_tireSpeed = -m_realvelocity.DotProduct(absoluteChassisMatrix.m_up).GetScalar();

			const Ogre::Real distance = ApplySuspenssionLimit();

			const ndFloat32 springConst = static_cast<ndFloat32>(m_tireConfiguration.springConst);
			const ndFloat32 springDamp = static_cast<ndFloat32>(m_tireConfiguration.springDamp);
			const ndFloat32 dt = static_cast<ndFloat32>(m_suspenssionStep);
			const ndFloat32 relSpeed = static_cast<ndFloat32>(m_tireSpeed);

			m_tireLoad = -CalculateSpringDamperAcceleration(dt, springConst, static_cast<ndFloat32>(distance), springDamp, relSpeed);
			// The method is a bit wrong here, I need to find a better method for integrate the tire mass.
			// This method uses the tire mass and interacting on the spring smoothness.
			m_tireLoad = (m_tireLoad * m_vehicle->m_mass * (m_tireConfiguration.smass / m_radius) * m_suspenssionFactor * m_suspenssionStep);

			const ndVector suspensionForce = absoluteChassisMatrix.m_up.Scale(static_cast<ndFloat32>(m_tireLoad));

			m_vehicle->ApplyForceAndTorque(m_body0, suspensionForce, m_TireAxelPosit);

			if (m_handForce > 0.0f)
			{
				m_motorForce = 0.0f;
			}

			if (m_tireConfiguration.tireAccel == tsAccel)
			{
				if (Ogre::Math::Abs(m_motorForce) > Ogre::Real(0.0f))
				{
					const ndVector r_tireForce = absoluteChassisMatrix.m_front.Scale(static_cast<ndFloat32>(m_motorForce));
					m_vehicle->ApplyForceAndTorque(m_body0, r_tireForce, m_TireAxelPosit);
				}
			}

			ProcessConvexContacts(info);
		}
	}
	else
	{
		m_isOnContact = false;
		m_isOnContactEx = false;
	}
}

void RayCastTire::SetTireConfiguration(const TireConfiguration& cfg)
{
	m_tireConfiguration = cfg;
}

const RayCastTire::TireConfiguration& RayCastTire::GetTireConfiguration(void) const
{
	return m_tireConfiguration;
}

void OgreNewt::RayCastTire::JacobianDerivative(ndConstraintDescritor& desc)
{
	// Assert inside, because its an interface
	// ndJointBilateralConstraint::JacobianDerivative(desc);

	if (m_jointBrakeForce != 0.0f)
	{
		this->AddLinearRowJacobian(desc, &m_TireAxelPosit[0], &m_TireAxelPosit[0], &m_longitudinalPin.m_x);
		this->SetHighFriction(desc, m_jointBrakeForce * 1000.0f);
		this->SetLowerFriction(desc, -m_jointBrakeForce * 1000.0f);
		m_jointBrakeForce = 0.0f;
	}

	ndMatrix matrix0;
	ndMatrix matrix1;

	m_body0->SetSleepState(false);

	// Calculates the position of the pivot point and the Jacobian direction vectors, in global space. 
	CalculateGlobalMatrix(matrix0, matrix1);

	ndMatrix absoluteChassisMatrix = m_body0->GetMatrix();

	m_vehicle->SetChassisMatrix(absoluteChassisMatrix);

	ndReal r_angularVelocity = 0.0f;

	if (m_tireConfiguration.brakeMode == tsNoBrake)
	{
		m_brakeForce = 0.0f;
	}

	m_lateralPin = absoluteChassisMatrix.RotateVector(m_localAxis);
	m_longitudinalPin = absoluteChassisMatrix.m_up.CrossProduct(m_lateralPin);
	//
	if ((m_brakeForce <= 0.0f) && (m_handForce <= 0.0f))
	{
		if ((ndAbs(m_motorForce) > 0.0f) || m_isOnContact)
		{
			r_angularVelocity = m_TireAxelVeloc.DotProduct(m_longitudinalPin).GetScalar();

			m_spinAngle = m_spinAngle - r_angularVelocity * desc.m_timestep * Ogre::Math::PI;
		}
	}

	if (m_isOnContact)
	{
		m_isOnContactEx = true;

		if ((m_tireConfiguration.brakeMode == tsBrake) && (m_brakeForce > 0.0f))
		{
			ApplyBrakes(m_brakeForce, desc);
		}
		else if (m_handForce > 0.0f)
		{
			ApplyBrakes(m_handForce, desc);
		}

		LongitudinalAndLateralFriction(m_TireAxelPosit, m_lateralPin, m_tireConfiguration.longitudinalFriction, m_tireConfiguration.lateralFriction, desc);

		m_isOnContact = false;
	}
}

// -----------------------------------------------------------------------------
// Vehicle
// -----------------------------------------------------------------------------
Vehicle::Vehicle(OgreNewt::World* world, Ogre::SceneManager* sceneManager, const Ogre::Vector3& defaultDirection,
	const OgreNewt::CollisionPtr& col, Ogre::Real vhmass, const Ogre::Vector3& collisionPosition,
	const Ogre::Vector3& massOrigin, const Ogre::Vector3& gravity, VehicleCallback* vehicleCallback) :
	OgreNewt::Body(world, sceneManager, col, Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC)
	, m_tireCount(0)
	, m_tires()
	, m_defaultDirection(ndVector(defaultDirection.x, defaultDirection.y, defaultDirection.z, 0.0f))
	, m_mass(vhmass)
	, m_massOrigin(ndVector(massOrigin.x, massOrigin.y, massOrigin.z, 0.0f))
	, m_collisionPosition(ndVector(collisionPosition.x, collisionPosition.y, collisionPosition.z, 0.0f))
	, m_vehicleForce(Ogre::Vector3::ZERO)
	, m_vehicleCallback(vehicleCallback)
	, m_debugtire(false)
	, m_initMassDataDone(false)
	, m_combackup(ndVector::m_zero)
	, m_canDrive(false)
{
	Ogre::Real mass = vhmass;

	this->getNewtonBody()->SetMassMatrix(mass, *this->getNewtonCollision());

	ndVector massMatrix = this->getNewtonBody()->GetMassMatrix();

	ndFloat32 Ixx = massMatrix.m_x;
	ndFloat32 Iyy = massMatrix.m_y;
	ndFloat32 Izz = massMatrix.m_z;
	ndFloat32 mass2 = massMatrix.m_w;

	Ixx /= 1.5f;
	Iyy /= 1.5f;
	Izz /= 1.5f;

	this->getNewtonBody()->SetMassMatrix(Ixx, Iyy, Izz, mass2);

	m_combackup = this->getNewtonBody()->GetCentreOfMass();
	m_combackup -= m_collisionPosition;
}

Vehicle::~Vehicle()
{
	RemoveAllTires();

	if (m_vehicleCallback)
	{
		delete m_vehicleCallback;
		m_vehicleCallback = nullptr;
	}
}

bool Vehicle::RemoveTire(RayCastTire* tire)
{
	m_tires.Remove(tire);
	return true;
}

void Vehicle::RemoveAllTires()
{
	m_tires.RemoveAll();
	m_tireCount = 0;
}

void Vehicle::AddTire(RayCastTire* tire)
{
	if (!tire)
	{
		return;
	}

	// TODO: is that correct?
	tire->SetCollidable(false);

	// area position along chassis Y
	tire->m_arealpos = (tire->m_localTireMatrix * m_chassisMatrix).m_posit.m_y;

	// local axis (forward) in chassis local space
	tire->m_localAxis = tire->GetLocalMatrix0().UnrotateVector(ndVector(0.0f, 0.0f, 1.0f, 0.0f));
	tire->m_localAxis.m_w = 0.0f;

	// suspension base position and hard point
	tire->m_suspensionMatrix.m_posit = tire->m_localTireMatrix.m_posit;
	tire->m_hardPoint = tire->GetLocalMatrix0().UntransformVector(tire->m_localTireMatrix.m_posit);

	// suspension limits
	tire->m_posit_y = tire->m_tireConfiguration.springLength;
	tire->m_suspenssionHardLimit = tire->m_radius * 0.5f;

	// tire id
	tire->m_tireID = m_tireCount;

	m_tires.Append(tire);
	m_tireCount++;
}

Ogre::Real Vehicle::VectorLength(const ndVector& aVec)
{
	return ndSqrt(aVec[0] * aVec[0] + aVec[1] * aVec[1] + aVec[2] * aVec[2]);
}

ndVector Vehicle::Rel2AbsPoint(ndBodyKinematic* vBody, const ndVector& vPointRel)
{
	ndMatrix M = vBody->GetMatrix();
	ndVector P, A;

	P = ndVector(vPointRel[0], vPointRel[1], vPointRel[2], 1.0f);
	A = M.TransformVector(P);
	return ndVector(A.m_x, A.m_y, A.m_z, 0.0f);
}

void Vehicle::AddForceAtPos(ndBodyKinematic* vBody, const ndVector& vForce, const ndVector& vPoint)
{
	const ndVector com = m_combackup;
	const ndVector R = vPoint - Rel2AbsPoint(vBody, com);
	const ndVector torque = R.CrossProduct(vForce);

	const ndVector prevF = vBody->GetForce();
	const ndVector prevT = vBody->GetTorque();
	vBody->SetForce(prevF + vForce);
	vBody->SetTorque(prevT + torque);
}

void Vehicle::AddForceAtRelPos(ndBodyKinematic* vBody, const ndVector& vForce, const ndVector& vPoint)
{
	if (VectorLength(vForce) != 0.0f)
	{
		AddForceAtPos(vBody, vForce, Rel2AbsPoint(vBody, vPoint));
	}
}

void Vehicle::ApplyForceAndTorque(ndBodyKinematic* vBody, const ndVector& vForce, const ndVector& vPoint)
{
	m_vehicleForce = Ogre::Vector3(vForce.m_x, vForce.m_y, vForce.m_z);
	AddForceAtPos(vBody, vForce, vPoint);
}

void Vehicle::SetChassisMatrix(const ndMatrix& matrix)
{
	m_chassisMatrix = matrix;
}

VehicleCallback* Vehicle::GetVehicleCallback(void) const
{
	return m_vehicleCallback;
}

Ogre::Vector3 Vehicle::getVehicleForce() const
{
	return m_vehicleForce;
}

void Vehicle::setCanDrive(bool canDrive)
{
	m_canDrive = canDrive;
}

void Vehicle::InitMassData()
{
	ndVector comChassis = this->getNewtonBody()->GetCentreOfMass();
	comChassis -= m_collisionPosition;
	comChassis = comChassis + m_massOrigin;

	this->getNewtonBody()->SetCentreOfMass(&comChassis[0]);
}

void Vehicle::setGravity(const Ogre::Vector3& gravity)
{
	m_gravity = gravity;
}

Ogre::Vector3 Vehicle::getGravity() const
{
	return m_gravity;
}

void Vehicle::Update(Ogre::Real timestep)
{
	if (false == m_canDrive)
	{
		return;
	}

	if (false == m_initMassDataDone)
	{
		InitMassData();
		m_initMassDataDone = true;
	}

	// === ND3 OnUpdateTransform(root bone) equivalent ===
	ndBodyKinematic* const vehicleBody = this->getNewtonBody();
	if (nullptr == vehicleBody)
	{
		return;
	}

	ndMatrix localVehicleMatrix = vehicleBody->GetMatrix();

	Ogre::Quaternion vehicleOrient;
	Ogre::Vector3 vehiclePos;
	OgreNewt::Converters::MatrixToQuatPos(&localVehicleMatrix[0][0], vehicleOrient, vehiclePos);

	m_curPosit = vehiclePos;
	m_curRotation = vehicleOrient;
	m_prevPosit = m_curPosit;
	m_prevRotation = m_curRotation;

	// --- update tire transforms and spin (from ND3 OnUpdateTransform) ---
	for (ndList<RayCastTire*>::ndNode* node = m_tires.GetFirst(); node; node = node->GetNext())
	{
		RayCastTire* tire = node->GetInfo();
		if (!tire || !tire->m_thisBody)
		{
			continue;
		}

		Ogre::Real sign = 1.0f;
		if (tire->m_pin.m_x != 0.0f)
		{
			sign = tire->m_pin.m_x;
		}
		else if (tire->m_pin.m_y != 0.0f)
		{
			sign = tire->m_pin.m_y;
		}
		else if (tire->m_pin.m_z != 0.0f)
		{
			sign = tire->m_pin.m_z;
		}

		ndMatrix absoluteTireMatrix = tire->CalculateTireMatrixAbsolute(1.0f * sign) * ndYawMatrix(-90.0f * ndDegreeToRad * sign);

		ndVector atireposit = tire->m_globalTireMatrix.m_posit;
		atireposit.m_y -= tire->m_posit_y - tire->m_radius * 0.5f;

		Ogre::Quaternion globalTireOrient;
		Ogre::Vector3 notUsedTirePos;
		OgreNewt::Converters::MatrixToQuatPos(&absoluteTireMatrix[0][0], globalTireOrient, notUsedTirePos);

		Ogre::Quaternion localTireOrient;
		Ogre::Vector3 localTirePos;
		OgreNewt::Converters::MatrixToQuatPos(&tire->m_localTireMatrix[0][0], localTireOrient, localTirePos);

		tire->m_thisBody->m_curPosit = vehiclePos + (vehicleOrient * localTirePos);
		tire->m_thisBody->m_curRotation = vehicleOrient * globalTireOrient * localTireOrient.Inverse();

		tire->m_thisBody->m_prevPosit = tire->m_thisBody->m_curPosit;
		tire->m_thisBody->m_prevRotation = tire->m_thisBody->m_curRotation;

		// Spin behaviour in the air
		if (false == tire->m_isOnContactEx)
		{
			if (tire->m_spinAngle > 0.0f)
			{
				tire->m_spinAngle = tire->m_spinAngle - 0.00001f;
			}
			else if (tire->m_spinAngle < 0.0f)
			{
				tire->m_spinAngle = tire->m_spinAngle + 0.00001f;
			}
		}
		else
		{
			tire->m_spinAngle = 0.0f;
		}

		updateDriverInput(tire, timestep);
		tire->ProcessPreUpdate();
	}
}

// -----------------------------------------------------------------------------
// Input update
// -----------------------------------------------------------------------------
void Vehicle::updateDriverInput(RayCastTire* tire, Ogre::Real timestep)
{
	if (false == m_canDrive || !tire || !m_vehicleCallback)
	{
		return;
	}

	// Reset forces
	tire->m_motorForce = 0.0f;
	tire->m_brakeForce = 0.0f;
	tire->m_handForce = 0.0f;

	tire->m_steerAngle = m_vehicleCallback->onSteerAngleChanged(this, tire, timestep);
	tire->m_motorForce = m_vehicleCallback->onMotorForceChanged(this, tire, timestep);

	const Ogre::Real handBrakeVal = m_vehicleCallback->onHandBrakeChanged(this, tire, timestep);
	if (handBrakeVal > 0.0f)
	{
		// ND3: Handbremse auf Reifen 0,1,2,3
		if ((tire->m_tireID == 0) || (tire->m_tireID == 1) || (tire->m_tireID == 2) || (tire->m_tireID == 3))
		{
			tire->m_handForce = handBrakeVal;
		}
	}

	const Ogre::Real brakeVal = m_vehicleCallback->onBrakeChanged(this, tire, timestep);
	if (brakeVal > 0.0f)
	{
		// ND3: normale Bremse nur auf Reifen 2 und 3
		if ((tire->m_tireID == 2) || (tire->m_tireID == 3))
		{
			tire->m_brakeForce = brakeVal;
		}
		else
		{
			tire->m_brakeForce = 0.0f;
		}
	}
}
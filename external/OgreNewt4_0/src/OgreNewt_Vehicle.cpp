#include "OgreNewt_Stdafx.h"
#include "OgreNewt_Vehicle.h"
#include "OgreNewt_VehicleNotify.h"

using namespace OgreNewt;

// -----------------------------------------------------------------------------
// RayCastTire
// -----------------------------------------------------------------------------
RayCastTire::RayCastTire(ndWorld* world, const ndMatrix& pinAndPivotFrame, const ndVector& pin, 
	Body* child, Body* parentChassisBody, Vehicle* parent, const TireConfiguration& tireConfiguration, ndReal radius)
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
	m_collision = child->getNewtonCollision();

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

	m_vehicle->setChassisMatrix(chassisMatrix);

	child->setJointRecursiveCollision(false);

	// Is already done in basic joints VehicleTire 
	// world->AddJoint(this);
}

RayCastTire::~RayCastTire()
{
}

void RayCastTire::UpdateParameters()
{
	// ND4 uses internal Jacobian building.
	// We keep all scalar state updated from updateGroundProbe / Vehicle::Update,
	// but do not rebuild rows here – your current ND4 setup uses impulses instead.
}

Ogre::Real RayCastTire::applySuspenssionLimit()
{
	Ogre::Real distance;
	distance = (m_tireConfiguration.springLength - m_posit_y);

	if (distance >= m_tireConfiguration.springLength)
	{
		distance = (m_tireConfiguration.springLength - m_posit_y) + m_suspenssionHardLimit;
	}
	return distance;
}

void OgreNewt::RayCastTire::processConvexContacts(OgreNewt::BasicConvexcast::ConvexcastContactInfo& info, Ogre::Real timestep)
{
	if (info.mBody)
	{
		// TODO: skip the chassis body also?
		if (info.mBody == m_vehicle)
		{
			return;
		}

		info.mBody->unFreeze();

		ndVector massMatrix = info.mBody->getNewtonBody()->GetMassMatrix();
		ndFloat32 mass = massMatrix.m_w;
		ndFloat32 invMass = info.mBody->getNewtonBody()->GetInvMass();

		// Skip static/kinematic bodies
		if (invMass == 0.0f)
		{
			return;
		}

		if (mass > 0.0f)
		{
			// TODO: 1.0f or 0.0f for w?
			ndVector uForce(
				info.mContactNormal[0] * -(m_vehicle->m_mass + mass) * 2.0f,
				info.mContactNormal[1] * -(m_vehicle->m_mass + mass) * 2.0f,
				info.mContactNormal[2] * -(m_vehicle->m_mass + mass) * 2.0f,
				0.0f);

			m_vehicle->applyForceAndTorque(info.mBody->getNewtonBody(), uForce, ndVector(info.mContactPoint[0], info.mContactPoint[1], info.mContactPoint[2], 1.0f), timestep);
		}
	}
}

ndMatrix RayCastTire::calculateSuspensionMatrixLocal(Ogre::Real distance)
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

ndMatrix RayCastTire::calculateTireMatrixAbsolute(Ogre::Real sSide)
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

	am = m_angleMatrix * calculateSuspensionMatrixLocal(m_tireConfiguration.springLength);
	return  am;
}

#if 0
void RayCastTire::applyBrakes(Ogre::Real bkforce, ndConstraintDescritor& desc)
{
	m_jointBrakeForce = bkforce;
	// Do not apply twice, its already done in jacobianderivative
	if (desc.m_rowsCount < m_maxDof)
	{
		AddLinearRowJacobian(desc, m_TireAxelPosit, m_TireAxelPosit, m_longitudinalPin);

		// brake friction limited by available tire load (prevents instant hard lock)
		const ndReal maxBrake = ndMax(ndReal(0.0f), m_tireLoad * m_tireConfiguration.longitudinalFriction * 0.025f);

		// bkforce is your input strength; treat it as a 0..1-ish scalar
		const ndReal requested = ndClamp((ndReal)bkforce * maxBrake, ndReal(0.0f), maxBrake);

		SetHighFriction(desc, requested);
		SetLowerFriction(desc, -requested);

	}
}
#else
void RayCastTire::applyBrakes(Ogre::Real bkforce, ndConstraintDescritor& desc)
{
	m_jointBrakeForce = bkforce;

	if (desc.m_rowsCount >= m_maxDof)
		return;

	// Longitudinal brake row
	AddLinearRowJacobian(desc, m_TireAxelPosit, m_TireAxelPosit, m_longitudinalPin);

	// Use tire load directly (NO tiny magic factor)
	const ndReal maxBrake =
		ndMax(ndReal(0.0f), m_tireLoad * m_tireConfiguration.longitudinalFriction * 0.025f);

	// bkforce is signed -> works forward & backward
	const ndReal requested = ndClamp(
		(ndReal)bkforce * maxBrake,
		-maxBrake, +maxBrake);

	SetHighFriction(desc, +ndAbs(requested));
	SetLowerFriction(desc, -ndAbs(requested));
}
#endif

ndMatrix RayCastTire::getLocalMatrix0()
{
	return m_localMatrix0;
}

void RayCastTire::longitudinalAndLateralFriction(const ndVector& contactPos, const ndVector& lateralPinIn, const ndVector& /*longitudinalPinIn*/, ndReal lateralMu,
	ndReal /*longitudinalMu*/, ndConstraintDescritor& desc)
{
	if (m_tireLoad <= ndReal(0.0f))
		return;

	// Direction must be a pure vector
	ndVector lateralPin = lateralPinIn;
	lateralPin.m_w = ndFloat32(0.0f);

	// Guard against zero-length dir (would trigger diag > 0 asserts)
	const ndFloat32 len2 = lateralPin.DotProduct(lateralPin).GetScalar();
	if (len2 < ndFloat32(1.0e-8f))
		return;

	// Normalize (important for stable friction magnitudes)
	lateralPin = lateralPin.Scale(ndRsqrt(len2));

	if (desc.m_rowsCount < m_maxDof)
	{
#if 0
		ndReal maxLat = m_tireLoad * lateralMu;

		Ogre::Real speed = m_vehicle->getVelocity().length();

		if (m_handForce > 0.0f && m_tireConfiguration.tireSteer == tsNoSteer || speed < 10.0f) // rear wheels
		{
			maxLat = m_tireLoad * lateralMu * 0.5f;
		}
#else
		// If this is a steering tire, boost grip with speed (arcade help)
		if (m_tireConfiguration.tireSteer == tsSteer)
		{
			const Ogre::Real speed = m_vehicle->getVelocity().length(); // make sure this is in m/s
			const Ogre::Real mul = calcArcadeGripMul(speed, Ogre::Real(25.0f), Ogre::Real(2.2f)); // tune
			lateralMu *= mul;
		}

		const ndReal maxLat = m_tireLoad * lateralMu;

#endif

		AddLinearRowJacobian(desc, contactPos, contactPos, lateralPin);
		SetHighFriction(desc, maxLat);
		SetLowerFriction(desc, -maxLat);
	}
}

void RayCastTire::processPreUpdate(Ogre::Real timestep, int threadIndex)
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

	// ndFloat32 tiredist = m_tireConfiguration.springLength;
	ndFloat32 tiredist = ndFloat32(m_tireConfiguration.springLength + m_radius);
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

	OgreNewt::BasicConvexcast convex(world, tireShape, startpt, orientation, endpt, 1, threadIndex, m_thisBody);

	// BasicConvexcast::ConvexcastContactInfo info = world->convexcastBlocking(tireShape, startpt, orientation, endpt, m_thisBody, 1, threadIndex);
	BasicConvexcast::ConvexcastContactInfo info;
	OgreNewt::Body* m_hitOgreNewtBody = nullptr;

	if (convex.getContactsCount() > 0)
	{
		info = convex.getInfoAt(0);
		m_hitOgreNewtBody = info.mBody;

		// Ignore chassis body hits (critical; otherwise the probe hits the vehicle itself)
		if (m_hitOgreNewtBody && m_hitOgreNewtBody->getNewtonBody() == m_chassisBody)
		{
			m_hitOgreNewtBody = nullptr;
		}

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

			// If the convex shape is starting inside the ground, avoid "stuck forever":
			// force a small compression so the spring pushes out next step.
			if (m_penetration > ndFloat32(0.0f))
			{
				// Pull the axle up a little (more compression) so suspension generates load.
				// (Clamp so we never exceed springLength.)
				m_posit_y = ndMin(ndFloat32(m_tireConfiguration.springLength), ndFloat32(0.02f));
			}

			m_vehicle->getVehicleCallback()->onTireContact(this, m_thisBody->getOgreNode()->getName(),
				m_hitOgreNewtBody, info.mContactPoint, info.mContactNormal, m_penetration);
		}
	}

	if (m_hitBody)
	{
		m_vehicle->m_noHitCounter = -1;
		m_isOnContactEx = true;
		m_isOnContact = true;

		const ndFloat32 slack = ndFloat32(0.05f); // 5cm tolerance, tune 0.03..0.08
		ndFloat32 tiredist = ndFloat32(m_tireConfiguration.springLength + slack);

		ndFloat32 intesectionDist = ndFloat32(convex.getDistanceToFirstHit());

		// allow a bit "below 0" to absorb start-inside cases
		if (intesectionDist < -slack)
			intesectionDist = -slack;
		if (intesectionDist > ndFloat32(m_tireConfiguration.springLength))
			intesectionDist = ndFloat32(m_tireConfiguration.springLength);

		// shift into [0..springLength] domain used by your axle formula
		m_posit_y = intesectionDist;
		if (m_posit_y < ndFloat32(0.0f))
			m_posit_y = ndFloat32(0.0f);




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

			const Ogre::Real distance = applySuspenssionLimit();

			const ndFloat32 springConst = static_cast<ndFloat32>(m_tireConfiguration.springConst);
			const ndFloat32 springDamp = static_cast<ndFloat32>(m_tireConfiguration.springDamp);
			const ndFloat32 relSpeed = static_cast<ndFloat32>(m_tireSpeed);

			m_tireLoad = -CalculateSpringDamperAcceleration(timestep, springConst, static_cast<ndFloat32>(distance), springDamp, relSpeed);
			// The method is a bit wrong here, I need to find a better method for integrate the tire mass.
			// This method uses the tire mass and interacting on the spring smoothness.
			m_tireLoad = (m_tireLoad * m_vehicle->m_mass * (m_tireConfiguration.smass / m_radius) * m_suspenssionFactor * m_suspenssionStep);

			const ndVector suspensionForce = absoluteChassisMatrix.m_up.Scale(static_cast<ndFloat32>(m_tireLoad));

			// m_body0 is chassis m_vehicle!
			m_vehicle->applyForceAndTorque(m_body0, suspensionForce, m_TireAxelPosit, timestep);

			if (m_handForce > 0.0f)
			{
				m_motorForce = 0.0f;
			}

			if (m_tireConfiguration.tireAccel == tsAccel)
			{
				if (Ogre::Math::Abs(m_motorForce) > Ogre::Real(0.0f))
				{
					const ndVector r_tireForce = absoluteChassisMatrix.m_front.Scale(static_cast<ndFloat32>(m_motorForce));
					m_vehicle->applyForceAndTorque(m_body0, r_tireForce, m_TireAxelPosit, timestep);
				}
			}

			processConvexContacts(info, timestep);
		}
	}
	else
	{
		m_isOnContact = false;
		m_isOnContactEx = false;
	}
}

void RayCastTire::setTireConfiguration(const TireConfiguration& cfg)
{
	m_tireConfiguration = cfg;
}

const RayCastTire::TireConfiguration& RayCastTire::getTireConfiguration(void) const
{
	return m_tireConfiguration;
}

void OgreNewt::RayCastTire::JacobianDerivative(ndConstraintDescritor& desc)
{
	// Assert inside, because its an interface
	// ndJointBilateralConstraint::JacobianDerivative(desc);
	m_jointBrakeForce = 0.0f;

	ndMatrix matrix0;
	ndMatrix matrix1;

	// Calculates the position of the pivot point and the Jacobian direction vectors, in global space. 
	CalculateGlobalMatrix(matrix0, matrix1);

	ndMatrix absoluteChassisMatrix = m_body0->GetMatrix();

	m_vehicle->setChassisMatrix(absoluteChassisMatrix);

	ndReal r_angularVelocity = 0.0f;

	if (m_tireConfiguration.brakeMode == tsNoBrake)
	{
		m_brakeForce = 0.0f;
	}

	// Build pins (ensure w=0)
	m_lateralPin = absoluteChassisMatrix.RotateVector(m_localAxis);
	m_lateralPin.m_w = 0.0f;

	m_longitudinalPin = absoluteChassisMatrix.m_up.CrossProduct(m_lateralPin);
	m_longitudinalPin.m_w = 0.0f;

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
		if (false == m_vehicle->m_useTilting)
		{
			m_isOnContactEx = true;

			if ((m_tireConfiguration.brakeMode == tsBrake) && (m_brakeForce > 0.0f))
			{
				applyBrakes(m_brakeForce, desc);
			}
			else if (m_handForce > 0.0f)
			{
				applyBrakes(m_handForce, desc);
			}

			Ogre::Real latMu = m_tireConfiguration.lateralFriction;

			// speed-based lateral loss
			if (m_handForce > 0.0f && m_tireConfiguration.tireSteer == tsNoSteer) // rear wheels
			{
				const Ogre::Real speed = m_vehicle->getVelocity().length();

				const Ogre::Real slipFactor = ndClamp(speed / Ogre::Real(20.0f), Ogre::Real(0.0f), Ogre::Real(1.0f));

				latMu *= (Ogre::Real(1.0f) - slipFactor * Ogre::Real(0.75f));
			}

			
			longitudinalAndLateralFriction(m_hitContact, m_lateralPin,
				m_longitudinalPin,                    // unused in lateral-only version
				latMu,  // side grip
				m_tireConfiguration.longitudinalFriction, desc);

			m_isOnContact = false;
		}
		else
		{
			// Bool flag tilting -> nice for spaceship or boat on water!
			// brakes (your working code)
			if ((m_tireConfiguration.brakeMode == tsBrake) && (m_brakeForce > 0.0f))
				applyBrakes(m_brakeForce, desc);
			else if (m_handForce > 0.0f)
				applyBrakes(m_handForce, desc);

			// lateral friction (one row)
			if (desc.m_rowsCount < m_maxDof)
			{
				const ndReal maxLat = m_tireLoad * m_tireConfiguration.lateralFriction;

				AddLinearRowJacobian(desc, m_TireAxelPosit, m_TireAxelPosit, m_lateralPin);
				SetHighFriction(desc, maxLat);
				SetLowerFriction(desc, -maxLat);
			}

			m_isOnContact = false;
		}
	}
}

Ogre::Real RayCastTire::calcArcadeGripMul(Ogre::Real speed, Ogre::Real speedRef, Ogre::Real maxMul)
{
	// speedRef ~ where you want "full help" (e.g. 25 m/s ~ 90 km/h)
	if (speedRef <= 0.0f) return 1.0f;

	const Ogre::Real t = Ogre::Math::Clamp(speed / speedRef, Ogre::Real(0.0f), Ogre::Real(1.0f));
	// Smoothstep-ish
	const Ogre::Real s = t * t * (Ogre::Real(3.0f) - Ogre::Real(2.0f) * t);

	return Ogre::Math::Clamp(Ogre::Real(1.0f) + s * (maxMul - Ogre::Real(1.0f)), Ogre::Real(1.0f), maxMul);
}

// -----------------------------------------------------------------------------
// Vehicle
// -----------------------------------------------------------------------------
Vehicle::Vehicle(OgreNewt::World* world, Ogre::SceneManager* sceneManager, const Ogre::Vector3& defaultDirection,
	const OgreNewt::CollisionPtr& col, Ogre::Real vhmass, const Ogre::Vector3& collisionPosition,
	const Ogre::Vector3& massOrigin, const Ogre::Vector3& gravity, VehicleCallback* vehicleCallback)
	: OgreNewt::Body(world, sceneManager, col, Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC, Body::NotifyKind::Vehicle)
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
	, m_useTilting(false)
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

	setJointRecursiveCollision(false);
}

Vehicle::~Vehicle()
{
	removeAllTires();

	if (m_vehicleCallback)
	{
		delete m_vehicleCallback;
		m_vehicleCallback = nullptr;
	}
}

bool Vehicle::removeTire(RayCastTire* tire)
{
	m_tires.Remove(tire);
	return true;
}

void Vehicle::removeAllTires()
{
	m_tires.RemoveAll();
	m_tireCount = 0;
}

void Vehicle::addTire(RayCastTire* tire)
{
	if (!tire)
	{
		return;
	}

	if (tire->m_thisBody)
	{
		tire->m_thisBody->setCollidable(false);
	}

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

Ogre::Real Vehicle::vectorLength(const ndVector& aVec)
{
	return ndSqrt(aVec[0] * aVec[0] + aVec[1] * aVec[1] + aVec[2] * aVec[2]);
}

ndVector Vehicle::rel2AbsPoint(ndBodyKinematic* vBody, const ndVector& vPointRel)
{
	ndMatrix M = vBody->GetMatrix();
	ndVector P, A;

	P = ndVector(vPointRel[0], vPointRel[1], vPointRel[2], 1.0f);
	A = M.TransformVector(P);
	return ndVector(A.m_x, A.m_y, A.m_z, 0.0f);
}

void Vehicle::addForceAtPos(ndBodyKinematic* vBody, const ndVector& vForce, const ndVector& vPoint, Ogre::Real timestep)
{
	if (!vBody)
		return;

	ndBodyDynamic* dyn = vBody->GetAsBodyDynamic();
	if (!dyn)
		return;

	const ndVector com = m_combackup;
	const ndVector R = vPoint - rel2AbsPoint(vBody, com);
	const ndVector torque = R.CrossProduct(vForce);

	const ndVector prevF = vBody->GetForce();
	const ndVector prevT = vBody->GetTorque();

	// vBody->SetForce(prevF + vForce);
	// vBody->SetTorque(prevT + torque);


	// Convert force -> impulse for this step
	const ndVector linearImpulse = vForce.Scale(timestep);
	const ndVector angularImpulse = torque.Scale(timestep);
	// This feeds m_impulseForce/m_impulseTorque internally, which ND4 applies reliably.
	dyn->ApplyImpulsePair(linearImpulse, angularImpulse, timestep);
}

void Vehicle::addForceAtRelPos(ndBodyKinematic* vBody, const ndVector& vForce, const ndVector& vPoint, Ogre::Real timestep)
{
	if (vectorLength(vForce) != 0.0f)
	{
		addForceAtPos(vBody, vForce, rel2AbsPoint(vBody, vPoint), timestep);
	}
}

void Vehicle::applyForceAndTorque(ndBodyKinematic* vBody, const ndVector& vForce, const ndVector& vPoint, Ogre::Real timestep)
{
	m_vehicleForce = Ogre::Vector3(vForce.m_x, vForce.m_y, vForce.m_z);
	addForceAtPos(vBody, vForce, vPoint, timestep);
}

void Vehicle::setChassisMatrix(const ndMatrix& matrix)
{
	m_chassisMatrix = matrix;
}

VehicleCallback* Vehicle::getVehicleCallback(void) const
{
	return m_vehicleCallback;
}

Ogre::Vector3 Vehicle::getVehicleForce() const
{
	return m_vehicleForce;
}

void OgreNewt::Vehicle::setUseTilting(bool tilt)
{
	m_useTilting = tilt;
}

bool OgreNewt::Vehicle::getUseTilting() const
{
	return m_useTilting;
}

void Vehicle::setCanDrive(bool canDrive)
{
	m_canDrive = canDrive;
}

void Vehicle::initMassData()
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

void Vehicle::updateUnstuck(Ogre::Real timestep)
{
	// --- Measure chassis movement ---
	const Ogre::Vector3 v = this->getVelocity();
	const Ogre::Real speed = Ogre::Math::Sqrt(v.dotProduct(v));

	// --- Count wheels in contact + detect driver torque request ---
	int wheelsOnGround = 0;
	Ogre::Real maxAbsMotor = 0.0f;

	for (ndList<RayCastTire*>::ndNode* node = m_tires.GetFirst(); node; node = node->GetNext())
	{
		RayCastTire* tire = node->GetInfo();
		if (!tire || !tire->m_thisBody)
			continue;

		if (tire->m_isOnContactEx)
			wheelsOnGround++;

		maxAbsMotor = std::max(maxAbsMotor, Ogre::Math::Abs(tire->m_motorForce));
	}

	// --- Is the driver trying to move? ---
	const bool wantsMove = (maxAbsMotor > Ogre::Real(0.1f));

	// --- Detect "stuck": torque requested, wheels grounded, but barely moving ---
	if (wantsMove && wheelsOnGround >= 2 && speed < Ogre::Real(0.3f))
	{
		m_stuck.stuckTimer += timestep;
	}
	else
	{
		m_stuck.stuckTimer = 0.0f;
		m_stuck.rescueActive = false;
	}

	// --- Activate rescue mode ---
	if (!m_stuck.rescueActive && m_stuck.stuckTimer > Ogre::Real(0.75f))
	{
		m_stuck.rescueActive = true;
		m_stuck.rescueTimer = Ogre::Real(1.2f);
		m_stuck.pulseTimer = Ogre::Real(0.0f);
	}

	// --- Rescue behavior ---
	if (m_stuck.rescueActive)
	{
		m_stuck.rescueTimer -= timestep;
		m_stuck.pulseTimer += timestep;

		const bool forwardPulse = std::fmod((float)m_stuck.pulseTimer, 0.30f) < 0.15f;

		const Ogre::Real rescueForce = forwardPulse ? Ogre::Real(600.0f) : Ogre::Real(-600.0f);
		const Ogre::Real wiggleDeg = forwardPulse ? Ogre::Real(6.0f) : Ogre::Real(-6.0f);

		for (ndList<RayCastTire*>::ndNode* node = m_tires.GetFirst(); node; node = node->GetNext())
		{
			RayCastTire* tire = node->GetInfo();
			if (!tire || !tire->m_thisBody)
				continue;

			// Apply rescue motor force only to driven wheels (Accel)
			if (tire->m_tireConfiguration.tireAccel == tsAccel)
				tire->m_motorForce = rescueForce;

			// Apply wiggle only to steered wheels
			if (tire->m_tireConfiguration.tireSteer == tsSteer)
				tire->m_steerAngle += wiggleDeg;
		}

		if (m_stuck.rescueTimer <= Ogre::Real(0.0f))
		{
			m_stuck.rescueActive = false;
			m_stuck.stuckTimer = Ogre::Real(0.0f);
		}
	}
}

void Vehicle::updateAirborneRescue(Ogre::Real timestep)
{
	// cooldown
	if (m_rescue.cooldown > 0.0f)
		m_rescue.cooldown -= timestep;

	// count grounded tires + get max motor request
	int wheelsOnGround = 0;
	Ogre::Real maxAbsMotor = 0.0f;

	for (ndList<RayCastTire*>::ndNode* node = m_tires.GetFirst(); node; node = node->GetNext())
	{
		RayCastTire* tire = node->GetInfo();
		if (!tire || !tire->m_thisBody)
			continue;

		if (tire->m_isOnContactEx)
			wheelsOnGround++;

		maxAbsMotor = std::max(maxAbsMotor, Ogre::Math::Abs(tire->m_motorForce));
	}

	const bool wantsMove = (maxAbsMotor > Ogre::Real(0.1f));

	// "airborne/high-centered" condition: no tire contact, driver tries to move
	if (wantsMove && wheelsOnGround == 0)
		m_rescue.airborneTimer += timestep;
	else
		m_rescue.airborneTimer = 0.0f;

	// Trigger after a 5 seconds delay, and only if not on cooldown
	if (m_rescue.airborneTimer < Ogre::Real(2.0f) || m_rescue.cooldown > Ogre::Real(0.0f))
		return;


	// Apply one-shot impulse: up + forward/back (alternating)
	ndBodyDynamic* chassis = this->getNewtonBody()->GetAsBodyDynamic();
	if (!chassis)
		return;

	const ndMatrix m = chassis->GetMatrix();

	ndVector up = m.m_up;
	up.m_w = ndFloat32(0.0f);

	ndVector fwd = m.m_front;
	fwd.m_w = ndFloat32(0.0f);

	if (!m_rescue.toggleDir)
		fwd = fwd.Scale(ndFloat32(1.0f));
	else
		fwd = fwd.Scale(ndFloat32(-1.0f));
	m_rescue.toggleDir = !m_rescue.toggleDir;

	// Normalize directions (safety)
	const ndFloat32 upLen2 = up.DotProduct(up).GetScalar();
	const ndFloat32 fLen2 = fwd.DotProduct(fwd).GetScalar();
	if (upLen2 > ndFloat32(1.0e-6f)) up = up.Scale(ndRsqrt(upLen2));
	if (fLen2 > ndFloat32(1.0e-6f)) fwd = fwd.Scale(ndRsqrt(fLen2));

	// Direction: mostly up, a bit forward/back
	ndVector dir = up.Scale(ndFloat32(0.85f)) + fwd.Scale(ndFloat32(0.15f));
	dir.m_w = ndFloat32(0.0f);

	const ndFloat32 dirLen2 = dir.DotProduct(dir).GetScalar();
	if (dirLen2 > ndFloat32(1.0e-6f))
		dir = dir.Scale(ndRsqrt(dirLen2));

	// Impulse magnitude: mass * targetDeltaV
	const ndFloat32 mass = chassis->GetMassMatrix().m_w; // if this is not mass in your build, use your cached Vehicle mass
	const ndFloat32 targetDeltaV = ndFloat32(10.0f);     // 1 m/s "kick" (tune 0.6..1.8)
	const ndVector linearImpulse = dir.Scale(mass * targetDeltaV);

	// Apply at center of mass (pure translation, no spin)
	const ndVector angularImpulse = ndVector::m_zero;

	chassis->ApplyImpulsePair(linearImpulse, angularImpulse, ndFloat32(timestep));

	// Reset timers + set cooldown so it doesn't spam
	m_rescue.airborneTimer = 0.0f;
	m_rescue.cooldown = Ogre::Real(1.0f); // tune 0.8..2.0
}

bool Vehicle::isAirborne() const
{
	int contacts = 0;
	for (ndList<RayCastTire*>::ndNode* node = m_tires.GetFirst(); node; node = node->GetNext())
	{
		RayCastTire* t = node->GetInfo();
		if (t && t->m_isOnContactEx)
			++contacts;
	}
	// tune threshold: <=1 contact feels airborne enough for tricks
	return contacts <= 1;
}

void Vehicle::applyPitch(Ogre::Real strength, Ogre::Real timestep)
{
	// pitchInput: -1..+1 (e.g. stick up/down)
	if (Ogre::Math::Abs(strength) < 1e-4f)
		return;

	ndBodyDynamic* dyn = this->getNewtonBody() ? this->getNewtonBody()->GetAsBodyDynamic() : nullptr;
	if (!dyn)
		return;

	if (!isAirborne())
	 	return;

	// Local right axis = pitch axis
	const ndMatrix m = dyn->GetMatrix();
	ndVector right = m.m_right;
	right.m_w = 0.0f;

	// Apply angular impulse (stable + timestep aware)
	// strength is in "torque-like" units; start with something like 1500..6000 depending on mass
	const ndVector angularImpulse = right.Scale((ndFloat32)(strength * timestep));
	const ndVector linearImpulse = ndVector::m_zero;

	dyn->ApplyImpulsePair(linearImpulse, angularImpulse, (ndFloat32)timestep);
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
		// Handbrake only on rear tires (2,3) for drifting
		if ((tire->m_tireID == 2) || (tire->m_tireID == 3))
		{
			tire->m_handForce = handBrakeVal;
		}
	}

	const Ogre::Real brakeVal = m_vehicleCallback->onBrakeChanged(this, tire, timestep);
	if (brakeVal > 0.0f)
	{
		// Brake only on tires 2 and 3
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

void Vehicle::physicsPreUpdate(Ogre::Real timestep, int threadIndex)
{
	if (!m_canDrive)
		return;

	if (!m_initMassDataDone)
	{
		initMassData();
		m_initMassDataDone = true;
	}

	// IMPORTANT:
	// Do NOT read Ogre scene nodes here.
	// Use physics state only.

	// Driver input should already be stored in member vars (steer/throttle/brake),
	// so just apply it to tires here.
	for (ndList<RayCastTire*>::ndNode* node = m_tires.GetFirst(); node; node = node->GetNext())
	{
		RayCastTire* tire = node->GetInfo();
		if (!tire) continue;

		updateDriverInput(tire, timestep);     // must not touch Ogre nodes
		tire->processPreUpdate(timestep, threadIndex); // raycast + suspension + forces
	}

	updateAirborneRescue(timestep);

	updateUnstuck(timestep);
}

void Vehicle::physicsOnTransform(const ndMatrix& localMatrix)
{
	if (!m_canDrive)
		return;

	// Update chassis cached transform from physics result
	Ogre::Quaternion vehicleOrient;
	Ogre::Vector3 vehiclePos;
	OgreNewt::Converters::MatrixToQuatPos(&localMatrix[0][0], vehicleOrient, vehiclePos);

	// Store into your chassis Body wrapper fields (or into Vehicle members)
	// NOTE: do NOT call updateNode() here if it touches Ogre scene graph on the physics thread.
	// Just store the pose; your render-thread sync should consume it later.
	m_curPosit = vehiclePos;
	m_curRotation = vehicleOrient;
	m_prevPosit = m_curPosit;
	m_prevRotation = m_curRotation;

	// Now update tire proxy bodies EXACTLY like ND3 did.
	for (ndList<RayCastTire*>::ndNode* node = m_tires.GetFirst(); node; node = node->GetNext())
	{
		RayCastTire* tire = node->GetInfo();
		if (!tire || !tire->m_thisBody)
			continue;

		Ogre::Real sign = 1.0f;
		if (tire->m_pin.m_x != 0.0f) sign = tire->m_pin.m_x;
		else if (tire->m_pin.m_y != 0.0f) sign = tire->m_pin.m_y;
		else if (tire->m_pin.m_z != 0.0f) sign = tire->m_pin.m_z;

		ndMatrix absoluteTireMatrix =
			tire->calculateTireMatrixAbsolute(1.0f * sign) *
			ndYawMatrix(-90.0f * ndDegreeToRad * sign);

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

		// Keep this (as you demanded)
		if (ndBodyKinematic* const tireBody = tire->m_thisBody->getNewtonBody())
		{
			ndMatrix m;
			OgreNewt::Converters::QuatPosToMatrix(tire->m_thisBody->m_curRotation,
				tire->m_thisBody->m_curPosit, m);
			tireBody->SetMatrix(m);
			tireBody->SetVelocity(ndVector::m_zero);
			tireBody->SetOmega(ndVector::m_zero);
		}

		// Air spin damping (same as ND3)
		if (!tire->m_isOnContactEx)
		{
			if (tire->m_spinAngle > 0.0f) tire->m_spinAngle -= 0.00001f;
			else if (tire->m_spinAngle < 0.0f) tire->m_spinAngle += 0.00001f;
		}
		else
		{
			tire->m_spinAngle = 0.0f;
		}
	}
}

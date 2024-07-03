#include "OgreNewt_Stdafx.h"
#include "OgreNewt_Vehicle.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_World.h"
#include "OgreNewt_RayCast.h"
#include "OgreNewt_ConvexCast.h"

namespace OgreNewt
{

	dFloat VehicleRayCastFilter(const NewtonBody* const body, const NewtonCollision* const collisionHit, const dFloat* const contact, const dFloat* const normal, dLong collisionID, void* const userData, dFloat intersetParam)
	{
		RayCastTire* const me = (RayCastTire*)userData;
		if (intersetParam < me->m_hitParam)
		{

			me->m_hitParam = intersetParam;
			me->m_hitBody = (NewtonBody*)body;
			// OgreNewt::Body* const b = (OgreNewt::Body*)userData;
			// Ogre::String name = b->getOgreNode()->getName();
			me->m_hitContact = dVector(contact[0], contact[1], contact[2], contact[3]);
			me->m_hitNormal = dVector(normal[0], normal[1], normal[2], normal[3]);
		}
		return intersetParam;
	}

	unsigned VehicleRayPrefilterCallback(const NewtonBody* const body, const NewtonCollision* const collision, void* const userData)
	{
		// TODO: Check this:
		Vehicle* const me = (Vehicle*)userData;
		Body* thisBody = (Body*)NewtonBodyGetUserData(body);
		Ogre::String name = thisBody->getOgreNode()->getName();
		if (me->getNewtonBody() != body)
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}

	void TireJointDeleteCallback(const dCustomJoint* const me)
	{
		// do nothing...
	}

	RayCastTire::RayCastTire(NewtonWorld* world, const dMatrix& pinAndPivotFrame, const dVector& pin, Body* child, Body* parentChassisBody, Vehicle* parent, const TireConfiguration& tireConfiguration, dFloat radius)
	    : dCustomJoint(2, parentChassisBody->getNewtonBody(), nullptr)
		, m_hitParam(1.1f)
		, m_hitBody(nullptr)
		, m_penetration(0.0f)
		, m_contactID(0)
		, m_hitContact(dVector(0.0f))
		, m_hitNormal(dVector(0.0f))
		, m_vehicle(parent)
		, m_tireConfiguration(tireConfiguration)
		, m_lateralPin(dVector(0.0f))
		, m_longitudinalPin(dVector(0.0f))
		, m_localAxis(dVector(0.0f))
		, m_brakeForce(0.0f)
		, m_motorForce(0.0f)
		, m_isOnContact(false)
		, m_chassisBody(parentChassisBody->getNewtonBody())
		, m_thisBody(child)
		, m_pin(pin)
		, m_globalTireMatrix(dGetIdentityMatrix())
		, m_suspensionMatrix(dGetIdentityMatrix())
		, m_hardPoint(dVector(0.0f))
		, m_tireRenderMatrix(dGetIdentityMatrix())
		, m_frameLocalMatrix(dGetIdentityMatrix())
		, m_angleMatrix(dGetIdentityMatrix())
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
		, m_realvelocity(dVector(0.0f))
		, m_TireAxelPosit(dVector(1.0f))
		, m_LocalAxelPosit(dVector(1.0f))
		, m_TireAxelVeloc(dVector(0.0f))
		, m_suspenssionFactor(0.5f)
		, m_suspenssionStep(1.0f / 60.0f)
		, m_tireID(-1)
		, m_handForce(0.0f)
	{
		SetSolverModel(2);
		dCustomJoint::m_world = world;

		m_body0 = parentChassisBody->getNewtonBody();

		// m_body1 = child->getNewtonBody();
		m_body1 = nullptr;

		m_collision = NewtonBodyGetCollision(parentChassisBody->getNewtonBody());

		dMatrix chassisMatrix;
		
		dVector com(parent->m_combackup);
		com.m_w = 1.0f;
		
		// Transform of chassis
		NewtonBodyGetMatrix(m_body0, &chassisMatrix[0][0]);
		
		chassisMatrix.m_posit += chassisMatrix.RotateVector(com);

		CalculateLocalMatrix(chassisMatrix, m_localMatrix0, m_localMatrix1);

		dMatrix tireMatrix;
		// Transform of this tire
		NewtonBodyGetMatrix(child->getNewtonBody(), &tireMatrix[0][0]);

		// Note: m_localTireMatrix must be really local. E.g. if Car is created in the air with y = 2.5, the tire must still be local to its chassis and having just a small amount of y!
		CalculateLocalMatrix(tireMatrix, m_localTireMatrix, m_globalTireMatrix);

		m_vehicle->SetChassisMatrix(chassisMatrix);

		NewtonBodySetJointRecursiveCollision(child->getNewtonBody(), 0);
	}

	RayCastTire::~RayCastTire()
	{

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

	void RayCastTire::ProcessWorldContacts(dFloat const rhitparam, NewtonBody* const rhitBody, dFloat const rpenetration, dLong const rcontactid, dVector const rhitContact, dVector const rhitNormal)
	{
		// Ray world contacts process for reproduce collision with others dynamics bodies.
				// Only experimental, Need a lot of work here to get it right.
		if (m_hitBody)
		{
			NewtonBodySetSleepState(m_hitBody, 0);
			dFloat ixx, iyy, izz, bmass;
			NewtonBodyGetMass(m_hitBody, &bmass, &ixx, &iyy, &izz);
			if (bmass > 0.0f)
			{
				//dVector bodyVel = m_realvelocity.Scale(-0.01f);
				//NewtonBodySetVelocity(m_hitBody, &bodyVel[0]);
				//
				dVector uForce(dVector(rhitNormal[0] * -(m_vehicle->m_mass + bmass) * 2.0f, rhitNormal[1] * -(m_vehicle->m_mass + bmass) * 2.0f, rhitNormal[2] * -(m_vehicle->m_mass + bmass) * 2.0f));
				m_vehicle->ApplyForceAndTorque((NewtonBody*)m_hitBody, uForce, dVector(rhitContact[0], rhitContact[1], rhitContact[2], 1.0f));
				//
				//dVector bodyVel = m_realvelocity.Scale(-0.01f);
				//NewtonBodySetVelocity(m_hitBody, &bodyVel[0]);
				//
				//m_posit_y = m_posit_y + info.m_penetration;
			}
			else
			{
				//printf("a:%.3f b:%.3f c:%.3f \n", rhitNormal[0], rhitNormal[1], rhitNormal[2]);
			}
		}
	}

	void RayCastTire::ProcessConvexContacts(NewtonWorldConvexCastReturnInfo const info)
	{
		// Only experimental, Need a lot of work here to get it right.
		if (info.m_hitBody)
		{
			NewtonBodySetSleepState(info.m_hitBody, 0);
			dFloat ixx, iyy, izz, bmass;
			NewtonBodyGetMass(info.m_hitBody, &bmass, &ixx, &iyy, &izz);

			if (bmass > 0.0f)
			{
				//dVector bodyVel = m_realvelocity.Scale(-0.01f);
				//NewtonBodySetVelocity(info.m_hitBody, &bodyVel[0]);
				//
				dVector uForce(dVector(info.m_normal[0] * -(m_vehicle->m_mass + bmass) * 2.0f, info.m_normal[1] * -(m_vehicle->m_mass + bmass) * 2.0f, info.m_normal[2] * -(m_vehicle->m_mass + bmass) * 2.0f));
				m_vehicle->ApplyForceAndTorque((NewtonBody*)info.m_hitBody, uForce, dVector(info.m_point[0], info.m_point[1], info.m_point[2], 1.0f));
				//
				//dVector bodyVel = m_realvelocity.Scale(-0.01f);
				//NewtonBodySetVelocity(info.m_hitBody, &bodyVel[0]);
				//
				//m_posit_y = m_posit_y + info.m_penetration;
			}
			else
			{
				//printf("a:%.3f b:%.3f c:%.3f \n", info.m_normal[0], info.m_normal[1], info.m_normal[2]);
			}
		}
	}

	dMatrix RayCastTire::CalculateSuspensionMatrixLocal(Ogre::Real distance)
	{
		// Info: Do not touch, just controls the visualisation of the tires
		dMatrix matrix;
		// calculate the steering angle matrix for the axis of rotation
		matrix.m_front = m_localAxis.Scale(-1.0f);
		matrix.m_up = dVector(0.0f, 1.0f, 0.0f, 0.0f);
		matrix.m_right = dVector(m_localAxis.m_z, 0.0f, -m_localAxis.m_x, 0.0f);
		matrix.m_posit = m_hardPoint + m_localMatrix0.m_up.Scale(distance);
		//
		return matrix;
	}

	dMatrix RayCastTire::CalculateTireMatrixAbsolute(Ogre::Real sSide)
	{
		dMatrix am;
		if (m_pin.m_z == 1.0f || m_pin.m_z == -1.0f)
		{
			m_angleMatrix = m_angleMatrix * dRollMatrix((m_spinAngle * sSide) * 60.0f * dDegreeToRad);
		}
		else if (m_pin.m_x == 1.0f || m_pin.m_x == -1.0f)
		{
			m_angleMatrix = m_angleMatrix * dPitchMatrix((m_spinAngle * sSide) * 60.0f * dDegreeToRad);
		}
		else
		{
			m_angleMatrix = m_angleMatrix * dYawMatrix((m_spinAngle * sSide) * 60.0f * dDegreeToRad);
		}
		
		am = m_angleMatrix * CalculateSuspensionMatrixLocal(m_tireConfiguration.springLength);
		return  am;
	}

	void RayCastTire::ApplyBrakes(Ogre::Real bkforce)
	{
		// very simple brake method.
		NewtonUserJointAddLinearRow(m_joint, &m_TireAxelPosit[0], &m_TireAxelPosit[0], &m_longitudinalPin.m_x);
		NewtonUserJointSetRowMaximumFriction(m_joint, bkforce * 1000.0f);
		NewtonUserJointSetRowMinimumFriction(m_joint, -bkforce * 1000.0f);
	}

	dMatrix RayCastTire::GetLocalMatrix0()
	{
		return m_localMatrix0;
	}

	void RayCastTire::LongitudinalAndLateralFriction(dVector tireposit, dVector lateralpin, dFloat turnfriction, dFloat sidingfriction, dFloat timestep)
	{
		dFloat invMag2 = 0.0f;
		dFloat frictionCircleMag = 0.0f;
		dFloat lateralFrictionForceMag = 0.0f;
		dFloat longitudinalFrictionForceMag = 0.0f;
		//
		if (m_tireLoad > 0.0f)
		{
			frictionCircleMag = m_tireLoad * turnfriction;
			lateralFrictionForceMag = frictionCircleMag;
			//
			longitudinalFrictionForceMag = m_tireLoad * sidingfriction;
			//
			invMag2 = frictionCircleMag / dSqrt(lateralFrictionForceMag * lateralFrictionForceMag + longitudinalFrictionForceMag * longitudinalFrictionForceMag);
			//
			lateralFrictionForceMag = lateralFrictionForceMag * invMag2;

			NewtonUserJointAddLinearRow(m_joint, &tireposit[0], &tireposit[0], &lateralpin[0]);
			NewtonUserJointSetRowMaximumFriction(m_joint, lateralFrictionForceMag);
			NewtonUserJointSetRowMinimumFriction(m_joint, -lateralFrictionForceMag);
		}
	}

	void RayCastTire::ProcessPreUpdate(Vehicle* vehicle, dFloat timestep, int threadID)
	{
		NewtonWorldConvexCastReturnInfo info;
		//
		// Initialized empty.
		info.m_contactID = -1;
		info.m_hitBody = nullptr;
		info.m_penetration = 0.0f;
		info.m_normal[0] = 0.0f;
		info.m_normal[1] = 0.0f;
		info.m_normal[2] = 0.0f;
		info.m_normal[3] = 0.0f;
		info.m_point[0] = 0.0f;
		info.m_point[1] = 0.0f;
		info.m_point[2] = 0.0f;
		info.m_point[3] = 1.0f;
		//
		m_hitBody = nullptr;
		m_isOnContactEx = false;

		dMatrix bodyMatrix;
		NewtonBodyGetMatrix(m_body0, &bodyMatrix[0][0]);

		dMatrix absoluteChassisMatrix(bodyMatrix * m_localMatrix0);

		m_frameLocalMatrix = m_localMatrix0;
		
		if (m_tireConfiguration.tireSteer == tsSteer)
		{
			m_localAxis.m_x = dSin((m_steerAngle * (int)m_tireConfiguration.steerSide) * dDegreeToRad);
			m_localAxis.m_z = dCos((m_steerAngle * (int)m_tireConfiguration.steerSide) * dDegreeToRad);
		}
		//
		m_posit_y = m_tireConfiguration.springLength;
		
		dMatrix curSuspenssionMatrix = m_suspensionMatrix * absoluteChassisMatrix;
		
		dFloat tiredist = m_tireConfiguration.springLength;
		//
		m_hitParam = 1.1f;
		//
		if (m_vehicle->m_raytype == rctWorld)
		{
			tiredist = (m_tireConfiguration.springLength + m_radius);
			dVector rayDestination = curSuspenssionMatrix.TransformVector(m_frameLocalMatrix.m_up.Scale(-tiredist));
			//
			dVector p0(curSuspenssionMatrix.m_posit);
			dVector p1(rayDestination);
			//
			NewtonWorldRayCast(dCustomJoint::m_world, &p0[0], &p1[0], VehicleRayCastFilter, this, VehicleRayPrefilterCallback, threadID);
		}
		else if (m_vehicle->m_raytype == rctConvex)
		{
			tiredist = (m_tireConfiguration.springLength);
				
#if 1
			dVector mRayDestination = curSuspenssionMatrix.TransformVector(m_frameLocalMatrix.m_up.Scale(-tiredist));
			if (NewtonWorldConvexCast(dCustomJoint::m_world, &curSuspenssionMatrix[0][0], &mRayDestination[0], m_collision, &m_hitParam, m_vehicle,
				VehicleRayPrefilterCallback, &info, 1, threadID))
			{
				m_hitBody = (NewtonBody*)info.m_hitBody;
				Body* m_hitOgreNewtBody = (Body*)NewtonBodyGetUserData(m_hitBody);

				Body* thisBody = (Body*)NewtonBodyGetUserData(m_hitBody);
				Ogre::String name = thisBody->getOgreNode()->getName();

				// Tires should not hit other tires
				for (dList<RayCastTire*>::dListNode* node = vehicle->m_tires.GetFirst(); node; node = node->GetNext())
				{
					RayCastTire* otherTire = node->GetInfo();
					if (m_hitOgreNewtBody == otherTire->m_thisBody)
					{
						return;
					}
				}

				m_hitContact = dVector(info.m_point[0], info.m_point[1], info.m_point[2], info.m_point[3]);
				m_hitNormal = dVector(info.m_normal[0], info.m_normal[1], info.m_normal[2], info.m_normal[3]);
				m_penetration = info.m_penetration;
				m_contactID = info.m_contactID;

				vehicle->GetVehicleCallback()->onTireContact(this, m_thisBody->getOgreNode()->getName(), m_hitOgreNewtBody, Ogre::Vector3(m_hitContact.m_x, m_hitContact.m_y, m_hitContact.m_z), Ogre::Vector3(m_hitNormal.m_x, m_hitNormal.m_y, m_hitNormal.m_z), m_penetration);
			}
			else
			{
				/*vehicle->m_noHitCounter++;
				if (200 == vehicle->m_noHitCounter)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Vehicle] Cannot hit the floor with tire: " + m_thisBody->getOgreNode()->getName() + " with the spring length of:  " + Ogre::StringConverter::toString(m_tireConfiguration.springLength)
						+ " or because of the collision hull. Try to adjust the spring length. Or use a different collision hull like capsule and adjust the collision position etc.");
				}*/
			}
#else
			dVector mRayDestination = CurSuspenssionMatrix.TransformVector(m_frameLocalMatrix.m_up.Scale(-tiredist * 4.0f));

			Ogre::Quaternion vehicleOrient;
			Ogre::Vector3 vehiclePos;

			OgreNewt::Converters::MatrixToQuatPos(&CurSuspenssionMatrix[0][0], vehicleOrient, vehiclePos);

			// OgreNewt::RayCast ray(m_vehicle->getWorld(), vehiclePos, Ogre::Vector3(mRayDestination.m_x, mRayDestination.m_y, mRayDestination.m_z), true);

			OgreNewt::BasicConvexcast ray(m_vehicle->getWorld(), m_thisBody->getNewtonCollision(), vehiclePos, vehicleOrient, Ogre::Vector3(mRayDestination.m_x, 
											mRayDestination.m_y, mRayDestination.m_z), 1, threadID);

			int contactCount = ray.getContactsCount();
			OgreNewt::BasicConvexcast::ConvexcastContactInfo info = ray.getInfoAt(contactCount - 1);
			if (info.mBody)
			{
				m_hitBody = (NewtonBody*)info.mBody->getNewtonBody();
				Body* thisBody = (Body*)NewtonBodyGetUserData(m_hitBody);
				Ogre::String name = thisBody->getOgreNode()->getName();
				m_hitContact = dVector(info.mContactPoint.x, info.mContactPoint.y, info.mContactPoint.z);
				m_hitNormal = dVector(info.mContactNormal.x, info.mContactNormal.y, info.mContactNormal.z);
				m_penetration = info.mContactPenetration;
				m_contactID = info.mCollisionID;
			}
#endif
		}
		//
		if (m_hitBody)
		{
			// Ok, hit something, ray does work!
			vehicle->m_noHitCounter = -1;
			m_isOnContactEx = true;
			m_isOnContact = true;
			//
			dFloat intesectionDist = 0.0f;
			//
			if (m_vehicle->m_raytype == rctWorld)
			{
				intesectionDist = (tiredist * m_hitParam - m_radius);
			}
			else
			{
				intesectionDist = (tiredist * m_hitParam);
			}
			//
			if (intesectionDist < 0.0f)
			{
				intesectionDist = 0.0f;
			}
			else if (intesectionDist > m_tireConfiguration.springLength)
			{
				intesectionDist = m_tireConfiguration.springLength;
			}
			//
			m_posit_y = intesectionDist;
			//
			dVector mChassisVelocity;
			dVector mChassisOmega;
			//
			NewtonBodyGetVelocity(m_vehicle->getNewtonBody(), &mChassisVelocity[0]);
			NewtonBodyGetOmega(m_vehicle->getNewtonBody(), &mChassisOmega[0]);
			 
			m_TireAxelPosit = absoluteChassisMatrix.TransformVector(m_hardPoint - m_frameLocalMatrix.m_up.Scale(m_posit_y));
			m_LocalAxelPosit = m_TireAxelPosit - absoluteChassisMatrix.m_posit;
			m_TireAxelVeloc = mChassisVelocity + mChassisOmega.CrossProduct(m_LocalAxelPosit);
			//
			//dVector anorm = AbsoluteChassisMatrix.m_right;
			//
			if (m_posit_y < m_tireConfiguration.springLength)
			{
				//
				dVector hitBodyVeloc;
				NewtonBodyGetVelocity(m_hitBody, &hitBodyVeloc[0]);

				const dVector relVeloc = m_TireAxelVeloc - hitBodyVeloc;
				m_realvelocity = relVeloc;
				m_tireSpeed = -m_realvelocity.DotProduct3(absoluteChassisMatrix.m_up);

				const dFloat distance = ApplySuspenssionLimit();

				m_tireLoad = -NewtonCalculateSpringDamperAcceleration(timestep, m_tireConfiguration.springConst, distance, m_tireConfiguration.springDamp, m_tireSpeed);
				// The method is a bit wrong here, I need to find a better method for integrate the tire mass.
				// This method uses the tire mass and interacting on the spring smoothness.
				m_tireLoad = (m_tireLoad * m_vehicle->m_mass * (m_tireConfiguration.smass / m_radius) * m_suspenssionFactor * m_suspenssionStep);
				
				const dVector suspensionForce = absoluteChassisMatrix.m_up.Scale(m_tireLoad);
				m_vehicle->ApplyForceAndTorque(m_body0, suspensionForce, m_TireAxelPosit);
				//
				// Only with handbrake
				if (m_handForce > 0.0f)
				{
					m_motorForce = 0.0f;
				}
				//
				if (m_tireConfiguration.tireAccel == tsAccel)
				{
					if (dAbs(m_motorForce) > 0.0f)
					{
						const dVector r_tireForce(absoluteChassisMatrix.m_front.Scale(m_motorForce));
						m_vehicle->ApplyForceAndTorque(m_body0, r_tireForce, m_TireAxelPosit);
					}
				}
				//
				// WIP: I'm not sure if it is the right place for process the contacts.
				// I need to do more test, And i'm not so sure about the way to go with contacts vs the force to apply or velocity.
				if (m_vehicle->m_raytype == rctWorld)
				{
					// No penetration info with the world raycast.
					// I let's it present just in case that later I find a method for introducing a penetration value.
					m_penetration = 0.0f;
					ProcessWorldContacts(m_hitParam, m_hitBody, m_penetration, m_contactID, m_hitContact, m_hitNormal);
				}
				else // With force restitution.
				{
					ProcessConvexContacts(info);
				}
			}
		}
	}

	void RayCastTire::SetTireConfiguration(const  RayCastTire::TireConfiguration& tireConfiguration)
	{
		m_tireConfiguration = tireConfiguration;
	}

	RayCastTire::TireConfiguration RayCastTire::GetTireConfiguration(void) const
	{
		return m_tireConfiguration;
	}

	void RayCastTire::SubmitConstraints(dFloat timestep, int threadIndex)
	{
		dMatrix matrix0;
		dMatrix matrix1;
		dMatrix absoluteChassisMatrix;
		//
		NewtonBodySetSleepState(m_body0, 0);
		// NewtonBodySetSleepState(m_thisBody->getNewtonBody(), 0);
		
		// Calculates the position of the pivot point and the Jacobian direction vectors, in global space. 
		CalculateGlobalMatrix(matrix0, matrix1);
		NewtonBodyGetMatrix(m_body0, &absoluteChassisMatrix[0][0]);
		
		m_vehicle->SetChassisMatrix(absoluteChassisMatrix);
		
		Ogre::Real r_angularVelocity = 0.0f;
		//
		if (m_tireConfiguration.brakeMode == tsNoBrake)
		{
			m_brakeForce = 0.0f;
		}
		//
		m_lateralPin = absoluteChassisMatrix.RotateVector(m_localAxis);
		m_longitudinalPin = absoluteChassisMatrix.m_up.CrossProduct(m_lateralPin);
		//
		if ((m_brakeForce <= 0.0f) && (m_handForce <= 0.0f))
		{
			if ((dAbs(m_motorForce) > 0.0f) || m_isOnContact)
			{
				r_angularVelocity = m_TireAxelVeloc.DotProduct3(m_longitudinalPin);
				m_spinAngle = m_spinAngle - r_angularVelocity * timestep * Ogre::Math::PI;
			}
		}
		//
		if (m_isOnContact)
		{
			m_isOnContactEx = true;
			//
			// Need a better implementation for deal about the brake and handbrake...
			if ((m_tireConfiguration.brakeMode == tsBrake) && (m_brakeForce > 0.0f))
			{
				ApplyBrakes(m_brakeForce);
			}
			else if (m_handForce > 0.0f)
			{
				ApplyBrakes(m_handForce);
			}
			//
			LongitudinalAndLateralFriction(m_TireAxelPosit, m_lateralPin, m_tireConfiguration.longitudinalFriction, m_tireConfiguration.lateralFriction, timestep);
			//
			m_isOnContact = false;
		}
	}

	void RayCastTire::Deserialize(NewtonDeserializeCallback callback, void* const userData)
	{
		int state;
		callback(userData, &state, sizeof(state));
	}

	void RayCastTire::Serialize(NewtonSerializeCallback callback, void* const userData) const
	{
		dCustomJoint::Serialize(callback, userData);
		int state = 0;
		callback(userData, &state, sizeof(state));
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////

	Vehicle::Vehicle(World* world, Ogre::SceneManager* sceneManager, const Ogre::Vector3& defaultDirection, const OgreNewt::CollisionPtr& col, Ogre::Real vhmass, const Ogre::Vector3& massOrigin, const Ogre::Vector3& collisionPosition, VehicleCallback* vehicleCallback)
		: Body(world, sceneManager, col)
		, dModelRootNode(nullptr, dGetIdentityMatrix())
		// , m_raytype(rctWorld)
		, m_raytype(rctConvex)
		, m_tireCount(0)
		, m_tires()
		, m_defaultDirection(dVector(defaultDirection.x, defaultDirection.y, defaultDirection.z))
		, m_mass(vhmass)
		, m_massOrigin(dVector(massOrigin.x, massOrigin.y, massOrigin.z))
		, m_collisionPosition(dVector(collisionPosition.x, collisionPosition.y, collisionPosition.z))
		, m_vehicleForce(Ogre::Vector3::ZERO)
		, m_vehicleCallback(vehicleCallback)
		, m_debugtire(false)
		, m_initMassDataDone(false)
		, m_combackup(dVector(0.0f))
	{
		//dVector aAngularDamping(0.0f);
		//NewtonBodySetLinearDamping(Body::m_body, 0);
		//NewtonBodySetAngularDamping(Body::m_body, &aAngularDamping[0]);

		Ogre::Real mass = vhmass;
		//// set vehicle mass, inertia and center of mass
		const NewtonCollision* collision = NewtonBodyGetCollision(Body::m_body);
		NewtonBodySetMassProperties(Body::m_body, mass, collision);

		dFloat Ixx;
		dFloat Iyy;
		dFloat Izz;
		NewtonBodyGetMass(Body::m_body, &mass, &Ixx, &Iyy, &Izz);
		Ixx /= 1.5f;
		Iyy /= 1.5f;
		Izz /= 1.5f;

		NewtonBodySetMassMatrix(Body::m_body, mass, Ixx, Iyy, Izz);

		NewtonDestroyCollision(col->getNewtonCollision());

		NewtonBodyGetCentreOfMass(Body::m_body, &m_combackup[0]);

		m_combackup -= m_collisionPosition;

		//// NewtonBodySetDestructorCallback(m_body, newtonDestructor);

		m_raycastVehicleManager = new RayCastVehicleManager(this);
		// The model to calculate the graphics transformation according to result physics matrices
		this->SetTransformMode(true);

		dModelRootNode::m_body = Body::m_body;
		NewtonBodySetJointRecursiveCollision(Body::m_body, 0);
	}

	Vehicle::~Vehicle()
	{
		for (dList<RayCastTire*>::dListNode* node = m_tires.GetFirst(); node; node = node->GetNext())
		{
			RayCastTire* atire = node->GetInfo();
			delete atire;
		}
		m_tires.RemoveAll();

		// Will be automatically deleted!
		/*if (nullptr != m_raycastVehicleManager)
		{
			delete m_raycastVehicleManager;
			m_raycastVehicleManager = nullptr;
		}*/

		if (nullptr != m_vehicleCallback)
		{
			delete m_vehicleCallback;
			m_vehicleCallback = nullptr;
		}
	};

	void Vehicle::SetRayCastMode(const VehicleRaycastType raytype)
	{
		m_raytype = raytype;
	}

	bool Vehicle::RemoveTire(RayCastTire* tire)
	{
		for (dList<RayCastTire*>::dListNode* node = m_tires.GetFirst(); node; node = node->GetNext())
		{
			if (((RayCastTire*)node->GetInfo()) == tire)
			{
				m_tires.Remove(tire);
				return true;
			}
		}
		return false;
	}

	void Vehicle::RemoveAllTires(void)
	{
		m_tires.RemoveAll();
	}


	void Vehicle::AddTire(RayCastTire* tire)
	{
		// Somehow radius will become etc. 1.0 instead of 0.17 and width 1.0 instead of 0.2
#if 0
		// Makes a convex hull collision shape to assist in calculation of the tire shape size
		// const dMatrix& meshMatrix = tirePart->GetMeshMatrix();
		const dMatrix& meshMatrix = dGetIdentityMatrix();

		Ogre::v1::MeshPtr tireMesh = static_cast<Ogre::v1::Entity*>(((Ogre::SceneNode*)tire->m_thisBody->getOgreNode())->getAttachedObject(0))->getMesh();

		// Support for only one mesh (sub mesh)
		Ogre::v1::SubMesh* subMesh = tireMesh->getSubMesh(0);
		Ogre::v1::VertexData* vertexData;
		if (subMesh->useSharedVertices)
		{
			vertexData = tireMesh->sharedVertexData[0];
		}
		else
		{
			vertexData = subMesh->vertexData[0];
		}

		Ogre::v1::IndexData* indexData = subMesh->indexData[0];

		const Ogre::v1::VertexElement* posElem = vertexData->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);
		Ogre::v1::HardwareVertexBufferSharedPtr vbuf = vertexData->vertexBufferBinding->getBuffer(posElem->getSource());
		Ogre::v1::HardwareIndexBufferSharedPtr ibuf = indexData->indexBuffer;

		float* vertices = static_cast<float*>(vbuf->lock(Ogre::v1::HardwareBuffer::HBL_WRITE_ONLY));
		float* indices = static_cast<float*>(ibuf->lock(Ogre::v1::HardwareBuffer::HBL_WRITE_ONLY));

		dArray<dVector> temp(vertexData->vertexCount);
		meshMatrix.TransformTriplex(&temp[0].m_x, sizeof(dVector), vertices, 3 * sizeof(dFloat), vertexData->vertexCount);
		NewtonCollision* const collision = NewtonCreateConvexHull(m_world->getNewtonWorld(), vertexData->vertexCount, &temp[0].m_x, sizeof(dVector), 0, 0, NULL);

		vbuf->unlock();
		ibuf->unlock();

		// Finds the support points that can be used to define the width and height of the tire collision mesh
		dVector extremePoint(0.0f);
		// Attention:
		dVector upDir(tire->GetLocalMatrix0().UnrotateVector(dVector(0.0f, 1.0f, 0.0f, 0.0f)));
		NewtonCollisionSupportVertex(collision, &upDir[0], &extremePoint[0]);
		tire->m_radius = dAbs(upDir.DotProduct3(extremePoint));

		// Attention:
		dVector widthDir(tire->GetLocalMatrix0().UnrotateVector(dVector(1.0f, 0.0f, 0.0f, 0.0f)));
		NewtonCollisionSupportVertex(collision, &widthDir[0], &extremePoint[0]);
		tire->m_width = widthDir.DotProduct3(extremePoint);

		widthDir = widthDir.Scale(-1.0f);
		NewtonCollisionSupportVertex(collision, &widthDir[0], &extremePoint[0]);
		tire->m_width += widthDir.DotProduct3(extremePoint);

		tire->m_width *= 0.5f;

		// tire->m_radius = 0.355f * 0.5f;
		// tire->m_width = 0.21f;

		// Destroys the auxiliary collision
		NewtonDestroyCollision(collision);
#endif

		tire->SetBodiesCollisionState(0);
		// NewtonBodySetCollidable(tire->m_thisBody->getNewtonBody(), 0);

		// tire->SetUserDestructorCallback(nullptr);
		// Equivalent to demo:
		// m_tire->m_tireMatrix = (m_tire->m_tireEntity->GetCurrentMatrix() * m_chassisMatrix);
		// m_tire->m_arealpos = m_tire->m_tireMatrix.m_posit.m_y;

		tire->m_arealpos = (tire->m_localTireMatrix * m_chassisMatrix).m_posit.m_y;
		
		tire->m_localAxis = tire->GetLocalMatrix0().UnrotateVector(dVector(0.0f, 0.0f, 1.0f, 0.0f));
		tire->m_localAxis.m_w = 0.0f;
		
		// Must be local, but in demo there are 2 local tire matrices:
		// m_tire->GetLocalMatrix0();
		// -0.21 0.18 0.0024
		// m_tire->m_localtireMatrix;
		// What is this one?
		// 1.16 -0.0707 -0.707803

		// Value comes from function add tire in demo!
		// get the location of this tire relative to the car chassis
		// dMatrix tireMatrix(tirePart->CalculateGlobalMatrix(vehEntity));

		tire->m_suspensionMatrix.m_posit = tire->m_localTireMatrix.m_posit;
		tire->m_hardPoint = tire->GetLocalMatrix0().UntransformVector(tire->m_localTireMatrix.m_posit);

		// dYawMatrix(90.0f).TransformVector

		// tire->m_hardPoint = tire->m_localTireMatrix.m_posit;

		tire->m_posit_y = tire->m_tireConfiguration.springLength;
		tire->m_suspenssionHardLimit = tire->m_radius * 0.5f;

		tire->m_tireID = m_tireCount;

		m_tires.Append(tire);
		m_tireCount++;
	}

	Ogre::Real Vehicle::VectorLength(dVector aVec)
	{
		return dSqrt(aVec[0] * aVec[0] + aVec[1] * aVec[1] + aVec[2] * aVec[2]);
	}

	dVector Vehicle::Rel2AbsPoint(NewtonBody* Body, dVector Pointrel)
	{
		dMatrix M;
		dVector P, A;
		NewtonBodyGetMatrix(Body, &M[0][0]);
		P = dVector(Pointrel[0], Pointrel[1], Pointrel[2], 1.0f);
		A = M.TransformVector(P);
		return dVector(A.m_x, A.m_y, A.m_z, 0.0f);
	}

	void Vehicle::AddForceAtPos(NewtonBody* Body, dVector Force, dVector Point)
	{
		dVector R, Torque, com;
		com = m_combackup;
		R = Point - Rel2AbsPoint(Body, com);
		Torque = R.CrossProduct(Force);
		NewtonBodyAddForce(Body, &Force[0]);
		NewtonBodyAddTorque(Body, &Torque[0]);
	}

	void Vehicle::AddForceAtRelPos(NewtonBody* Body, dVector Force, dVector Point)
	{
		if (VectorLength(Force) != 0.0f)
		{
			AddForceAtPos(Body, Force, Rel2AbsPoint(Body, Point));
		}
	}

	void Vehicle::ApplyForceAndTorque(NewtonBody* vBody, dVector vForce, dVector vPoint)
	{
		m_vehicleForce = Ogre::Vector3(vForce.m_x, vForce.m_y, vForce.m_z);
		AddForceAtPos(vBody, vForce, vPoint);
	}

	void Vehicle::SetChassisMatrix(dMatrix vhmatrix)
	{
		m_chassisMatrix = vhmatrix;
	}

	VehicleCallback* Vehicle::GetVehicleCallback(void) const
	{
		return m_vehicleCallback;
	}

	Ogre::Vector3 Vehicle::getVehicleForce(void) const
	{
		return m_vehicleForce;
	}

	void Vehicle::InitMassData(void)
	{
		dVector comChassis;
		NewtonBodyGetCentreOfMass(Body::m_body, &comChassis[0]);
		comChassis -= m_collisionPosition;
		comChassis = comChassis + m_massOrigin;

		NewtonBodySetCentreOfMass(Body::m_body, &comChassis[0]);
	}

	////////////////////////////////////////////////////////////////////////////////////////////

	// TODO: Manager only once for all bodies??

	RayCastVehicleManager::RayCastVehicleManager(Vehicle* vehicle)
		: dModelManager(vehicle->getWorld()->getNewtonWorld())
	{
		// Adds the model to the manager
		AddRoot(vehicle);
	}

	RayCastVehicleManager::~RayCastVehicleManager()
	{

	}

	void RayCastVehicleManager::OnPreUpdate(dModelRootNode* const model, dFloat timestep, int threadID) const
	{
		Vehicle* vehicle = (Vehicle*)model;

		for (dList<RayCastTire*>::dListNode* node = vehicle->m_tires.GetFirst(); node; node = node->GetNext())
		{
			RayCastTire* tire = node->GetInfo();
			UpdateDriverInput(vehicle, tire, timestep);
			tire->ProcessPreUpdate(vehicle, timestep, threadID);
		}
	}

	void RayCastVehicleManager::OnPostUpdate(dModelRootNode* const model, dFloat timestep, int threadID) const
	{
	}

	void RayCastVehicleManager::OnUpdateTransform(const dModelNode* const bone, const dMatrix& localMatrix) const
	{
		Vehicle* vehicle = (Vehicle*)bone;
		NewtonBody* const body = bone->GetBody();

		if (false == vehicle->m_initMassDataDone)
		{
			vehicle->InitMassData();
			vehicle->m_initMassDataDone = true;
		}

		Ogre::Quaternion vehicleOrient;
		Ogre::Vector3 vehiclePos;

		OgreNewt::Converters::MatrixToQuatPos(&localMatrix[0][0], vehicleOrient, vehiclePos);

		vehicle->m_curPosit = vehiclePos;
		vehicle->m_curRotation = vehicleOrient;

		vehicle->m_prevPosit = vehicle->m_curPosit;
		vehicle->m_prevRotation = vehicle->m_curRotation;

		int tirecnt = 0;
		for (dList<RayCastTire*>::dListNode* node = vehicle->m_tires.GetFirst(); node; node = node->GetNext())
		{
			RayCastTire* tire = node->GetInfo();

			
#if 0
			dMatrix absoluteTireMatrix = dGetIdentityMatrix();

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

			Ogre::String tireName = tire->m_thisBody->getOgreNode()->getName();

			// Adjust the right and left tire mesh side and rotation side.
			if (tire->m_tireConfiguration.tireSide == tsTireSideA)
			{
				absoluteTireMatrix = (tire->CalculateTireMatrixAbsolute(-1.0f) * dYawMatrix(-90.0f * dDegreeToRad * sign));
			}
			else
			{
				absoluteTireMatrix = (tire->CalculateTireMatrixAbsolute(1.0f) * dYawMatrix(-90.0f * dDegreeToRad * sign));
			}
#else
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
			dMatrix absoluteTireMatrix = (tire->CalculateTireMatrixAbsolute(1.0f * sign) * dYawMatrix(-90.0f * dDegreeToRad * sign));
#endif

			dVector atireposit = tire->m_globalTireMatrix.m_posit;
			atireposit.m_y -= tire->m_posit_y - tire->m_radius * 0.5f;

			Ogre::Quaternion globalTireOrient;
			Ogre::Vector3 notUsedTirePos;
			OgreNewt::Converters::MatrixToQuatPos(&absoluteTireMatrix[0][0], globalTireOrient, notUsedTirePos);

			Ogre::Vector3 globalTirePos = Ogre::Vector3(atireposit.m_x, atireposit.m_y, atireposit.m_z);

			Ogre::Quaternion localTireOrient;
			Ogre::Vector3 localTirePos;

			OgreNewt::Converters::MatrixToQuatPos(&tire->m_localTireMatrix[0][0], localTireOrient, localTirePos);
			

			/*if (tirecnt == 0)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Vehicle] localTirePos: " + Ogre::StringConverter::toString(localTirePos)
																+ " local localTireOrient: " + Ogre::StringConverter::toString(localTireOrient.getYaw().valueDegrees())
																+ " globalTirePos: " + Ogre::StringConverter::toString(globalTirePos)
																+ " globalTireOrient: " + Ogre::StringConverter::toString(globalTireOrient.getYaw().valueDegrees()));
			}*/

			tire->m_thisBody->m_curPosit = vehiclePos + (vehicleOrient * localTirePos);
			tire->m_thisBody->m_curRotation = vehicleOrient * globalTireOrient * localTireOrient.Inverse();

			tire->m_thisBody->m_prevPosit = tire->m_thisBody->m_curPosit;
			tire->m_thisBody->m_prevRotation = tire->m_thisBody->m_curRotation;
			
			
			if (!tire->m_isOnContactEx)
			{
				// Let's spin the tire on the air with very minimal spin reduction.
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

			tirecnt++;
		}
	}

	void RayCastVehicleManager::OnDebug(dModelRootNode* const model, dCustomJoint::dDebugDisplay* const debugContext)
	{
		Vehicle* controller = (Vehicle*)model;
		// printf("db \n"); debug is not call and not working with last git sdk commit. 
		if (!controller->m_debugtire) return;
		//
		for (dList<RayCastTire*>::dListNode* node = controller->m_tires.GetFirst(); node; node = node->GetNext())
		{
			RayCastTire* tire = node->GetInfo();
			// temporary surely need to change it later.
			dMatrix vhchassismatrix;
			NewtonBodyGetMatrix(controller->GetBody(), &vhchassismatrix[0][0]);
			//tire->m_tireRenderMatrix = tire->m_localtireMatrix * vhchassismatrix;
			//tire->m_tireRenderMatrix.m_posit.m_y -= tire->m_posit_y - tire->m_radius * 0.5f;
			//
			// 
			// TODO: What here?
			// tire->m_tireRenderMatrix = tire->m_tireEntity->GetNextMatrix() * vhchassismatrix;
			//
			
			NewtonCollisionForEachPolygonDo(tire->m_collision, &tire->m_tireRenderMatrix[0][0], RenderDebugRayTire, debugContext);
		}
	}

	void RayCastVehicleManager::UpdateDriverInput(Vehicle* const vehicle, RayCastTire* vhtire, dFloat timestep)
	{
		// vhtire->m_steerAngle = 0.0f;
		vhtire->m_motorForce = 0.0f;
		vhtire->m_brakeForce = 0.0f;
		vhtire->m_handForce = 0.0f;

		vhtire->m_steerAngle = vehicle->GetVehicleCallback()->onSteerAngleChanged(vehicle, vhtire, timestep);
		vhtire->m_motorForce = vehicle->GetVehicleCallback()->onMotorForceChanged(vehicle, vhtire, timestep);

		const Ogre::Real handBrakeVal = vehicle->GetVehicleCallback()->onHandBrakeChanged(vehicle, vhtire, timestep);
		if (handBrakeVal > 0.0f)
		{
			if ((vhtire->m_tireID == 0) || (vhtire->m_tireID == 1) || (vhtire->m_tireID == 2) || (vhtire->m_tireID == 3))
			{
				vhtire->m_handForce = handBrakeVal;
			}
		}

		const Ogre::Real brakeVal = vehicle->GetVehicleCallback()->onBrakeChanged(vehicle, vhtire, timestep);
		if (brakeVal > 0.0f)
		{
			if ((vhtire->m_tireID == 2) || (vhtire->m_tireID == 3))
			{
				vhtire->m_brakeForce = brakeVal;
			}
			else
			{
				vhtire->m_brakeForce = 0.0f;
			}
		}

		// Need a better implementation here about the brake and handbrake...

#if 0
		// TODO: like in player controller
		vhtire->m_steerAngle = (dFloat(vehicle->m_scene->GetKeyState('A') * 45.0f) - dFloat(vehicle->m_scene->GetKeyState('D') * 45.0f));
		vhtire->m_motorForce = (dFloat(vehicle->m_scene->GetKeyState('W') * 5000.0f * 120.0f * timestep) - dFloat(vehicle->m_scene->GetKeyState('S') * 4500.0f * 120.0f * timestep));

		dFloat brakeval = dFloat(vehicle->m_scene->GetKeyState(' '));
		if (dAbs(brakeval) > 0.0f)
			if ((vhtire->m_tireID == 0) || (vhtire->m_tireID == 1) || (vhtire->m_tireID == 2) || (vhtire->m_tireID == 3))
			{
				vhtire->m_handForce = brakeval * 5.5f;
			}
		//
		dFloat brakeval2 = dFloat(vehicle->m_scene->GetKeyState('B'));
		if (dAbs(brakeval2) > 0.0f)
			if ((vhtire->m_tireID == 2) || (vhtire->m_tireID == 3))
			{
				vhtire->m_brakeForce = brakeval2 * 7.5f;
			}
			else
			{
				vhtire->m_brakeForce = 0.0f;
			}

		else
			if (vehiclemodel == truck)
			{
				vhtire->m_steerAngle = (dFloat(vehiclemodel->m_scene->GetKeyState('A') * 35.0f) - dFloat(vehiclemodel->m_scene->GetKeyState('D') * 35.0f));
				vhtire->m_motorForce = (dFloat(vehiclemodel->m_scene->GetKeyState('W') * 3500.0f * 120.0f * timestep) - dFloat(vehiclemodel->m_scene->GetKeyState('S') * 2000.0f * 120.0f * timestep));
				dFloat brakeval = dFloat(vehiclemodel->m_scene->GetKeyState(' '));
				if (dAbs(brakeval) > 0.0f)
					if ((vhtire->m_tireID == 0) || (vhtire->m_tireID == 1) || (vhtire->m_tireID == 2) || (vhtire->m_tireID == 3))
					{
						vhtire->m_handForce = brakeval * 5.5f;
					}
				//
				dFloat brakeval2 = dFloat(vehiclemodel->m_scene->GetKeyState('B'));
				if (dAbs(brakeval2) > 0.0f)
					if ((vhtire->m_tireID == 2) || (vhtire->m_tireID == 3))
					{
						vhtire->m_brakeForce = brakeval2 * 7.5f;
					}
					else
					{
						vhtire->m_brakeForce = 0.0f;
					}
			}
#endif
	}
}

#include "OgreNewt_Stdafx.h"
#include "OgreNewt_World.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_BasicJoints.h"

#include "dCustomHinge.h"
#include "dCustomSlider.h"
#include "dCustomSlidingContact.h"
#include "dCustomBallAndSocket.h"
// #include "dCustomRagdollMotor.h"
#include "dCustomCorkScrew.h"
#include "dCustomUpVector.h"
#include "dCustomKinematicController.h"
#include "dCustomDryRollingFriction.h"
#include "dCustomPathFollow.h"
#include "dBezierSpline.h"
#include "dCustomGear.h"
#include "dCustomRackAndPinion.h"
#include "dCustomPulley.h"
// #include "dCustomUniversal.h" // DoubleHinge now
#include "dCustomDoubleHinge.h"
#include "dCustom6dof.h"
#include "dCustomFixDistance.h"

#ifdef __APPLE__
#   include <Ogre/OgreLogManager.h>
#   include <Ogre/OgreStringConverter.h>
#else
#   include <OgreLogManager.h>
#   include <OgreStringConverter.h>
#endif

namespace OgreNewt
{
	PointToPoint::PointToPoint(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos1, const Ogre::Vector3& pos2)
		: Joint()
	{
		dCustomJoint* supportJoint;

		// create a Newton Custom joint and set it at the support joint	
		supportJoint = new dCustomFixDistance(dVector(pos1.x, pos1.y, pos1.z, 1.0f), dVector(pos2.x, pos2.y, pos2.z, 1.0f),
			child->getNewtonBody(), parent ? parent->getNewtonBody() : nullptr);
		SetSupportJoint(supportJoint);
	}

	PointToPoint::~PointToPoint()
	{
		// nothing, the ~Joint() function will take care of us.
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	BallAndSocket::BallAndSocket(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos)
		: Joint()
	{
		dCustomJoint* supportJoint;

		// make the joint matrix 
		dMatrix pinsAndPivoFrame(dGetIdentityMatrix());
		pinsAndPivoFrame.m_posit = dVector(pos.x, pos.y, pos.z, 1.0f);

		// create a Newton Custom joint and set it at the support joint	
		supportJoint = new dCustomBallAndSocket(pinsAndPivoFrame, child->getNewtonBody(), parent ? parent->getNewtonBody() : nullptr);
		SetSupportJoint(supportJoint);
	}

	BallAndSocket::~BallAndSocket()
	{
		// nothing, the ~Joint() function will take care of us.
	}

	void BallAndSocket::enableTwist(bool state)
	{
		dCustomBallAndSocket* joint = (dCustomBallAndSocket*)GetSupportJoint();
		joint->EnableTwist(state);

		getBody0()->unFreeze();
		if (getBody1())
			getBody1()->unFreeze();
	}

	void BallAndSocket::setTwistLimits(const Ogre::Degree& minAngle, const Ogre::Degree& maxAngle)
	{
		dCustomBallAndSocket* joint = (dCustomBallAndSocket*)GetSupportJoint();
		joint->SetTwistLimits((float)minAngle.valueRadians(), (float)maxAngle.valueRadians());

		getBody0()->unFreeze();
		if (getBody1())
			getBody1()->unFreeze();
	}

	void BallAndSocket::getTwistLimits(Ogre::Degree& minAngle, Ogre::Degree& maxAngle) const
	{
		dCustomBallAndSocket* joint = (dCustomBallAndSocket*)GetSupportJoint();
		float tempMinAngle;
		float tempMaxAngle;
		joint->GetTwistLimits(tempMinAngle, tempMaxAngle);
		minAngle = Ogre::Radian(tempMinAngle);
		maxAngle = Ogre::Radian(tempMaxAngle);

		getBody0()->unFreeze();
		if (getBody1())
			getBody1()->unFreeze();
	}

	void BallAndSocket::enableCone(bool state)
	{
		dCustomBallAndSocket* joint = (dCustomBallAndSocket*)GetSupportJoint();
		joint->EnableCone(state);

		getBody0()->unFreeze();
		if (getBody1())
			getBody1()->unFreeze();
	}

	Ogre::Degree BallAndSocket::getConeLimits() const
	{
		dCustomBallAndSocket* joint = (dCustomBallAndSocket*)GetSupportJoint();

		Ogre::Degree deg = Ogre::Radian(joint->GetConeLimits());
		return deg;
	}

	void BallAndSocket::setConeLimits(const Ogre::Degree& maxAngle)
	{
		dCustomBallAndSocket* joint = (dCustomBallAndSocket*)GetSupportJoint();
		joint->SetConeLimits((float)maxAngle.valueRadians());

		getBody0()->unFreeze();
		if (getBody1())
			getBody1()->unFreeze();
	}

	void BallAndSocket::setTwistFriction(Ogre::Real frictionTorque)
	{
		dCustomBallAndSocket* joint = (dCustomBallAndSocket*)GetSupportJoint();
		joint->SetTwistFriction(frictionTorque);

		getBody0()->unFreeze();
		if (getBody1())
			getBody1()->unFreeze();
	}

	Ogre::Real BallAndSocket::getTwistFriction(Ogre::Real frictionTorque) const
	{
		dCustomBallAndSocket* joint = (dCustomBallAndSocket*)GetSupportJoint();
		// 0 unnecessary, small bug in newton
		return joint->GetTwistFriction(0);
	}

	void BallAndSocket::setConeFriction(Ogre::Real frictionTorque)
	{
		dCustomBallAndSocket* joint = (dCustomBallAndSocket*)GetSupportJoint();
		joint->SetConeFriction(frictionTorque);

		getBody0()->unFreeze();
		if (getBody1())
			getBody1()->unFreeze();
	}

	Ogre::Real BallAndSocket::getConeFriction(Ogre::Real frictionTorque) const
	{
		dCustomBallAndSocket* joint = (dCustomBallAndSocket*)GetSupportJoint();
		// 0 unnecessary, small bug in newton
		return joint->GetConeFriction(0);
	}

	void BallAndSocket::setTwistSpringDamper(bool state, Ogre::Real springDamperRelaxation, Ogre::Real spring, Ogre::Real damper)
	{
		dCustomBallAndSocket* joint = (dCustomBallAndSocket*)GetSupportJoint();
		joint->SetTwistSpringDamper(state, springDamperRelaxation, spring, damper);

		getBody0()->unFreeze();
		if (getBody1())
			getBody1()->unFreeze();
	}

	Ogre::Vector3 BallAndSocket::getJointForce()
	{
		Ogre::Vector3 ret = Ogre::Vector3::ZERO;
		NewtonBallGetJointForce(m_joint->GetJoint(), &ret.x);
		return ret;
	}

	Ogre::Vector3 BallAndSocket::getJointAngle()
	{
		Ogre::Vector3 ret = Ogre::Vector3::ZERO;
		NewtonBallGetJointAngle(m_joint->GetJoint(), &ret.x);
		return ret;
	}

	Ogre::Vector3 BallAndSocket::getJointOmega()
	{
		Ogre::Vector3 ret = Ogre::Vector3::ZERO;
		NewtonBallGetJointOmega(m_joint->GetJoint(), &ret.x);
		return ret;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	//ControlledBallAndSocket::ControlledBallAndSocket(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos)
	//	: Joint()
	//{
	//	dCustomJoint* supportJoint;

	//	// make the joint matrix 
	//	dMatrix pinsAndPivoFrame(dGetIdentityMatrix());
	//	pinsAndPivoFrame.m_posit = dVector(pos.x, pos.y, pos.z, 1.0f);

	//	// create a Newton Custom joint and set it at the support joint	
	//	supportJoint = new dCustomControlledBallAndSocket(pinsAndPivoFrame, child->getNewtonBody(), parent ? parent->getNewtonBody() : nullptr);

	//	SetSupportJoint(supportJoint);
	//}

	//ControlledBallAndSocket::~ControlledBallAndSocket()
	//{
	//	// nothing, the ~Joint() function will take care of us.
	//}

	//void ControlledBallAndSocket::setAngularVelocity(Ogre::Real angularVelocity)
	//{
	//	dCustomControlledBallAndSocket* joint = (dCustomControlledBallAndSocket*)GetSupportJoint();
	//	joint->SetAngularVelocity(angularVelocity);
	//}

	//void ControlledBallAndSocket::setPitchYawRollAngle(Ogre::Degree pitch, Ogre::Degree yaw, Ogre::Degree roll) const
	//{
	//	dCustomControlledBallAndSocket* joint = (dCustomControlledBallAndSocket*)GetSupportJoint();
	//	joint->SetPitchAngle((float)pitch.valueRadians());
	//	joint->SetYawAngle((float)yaw.valueRadians());
	//	joint->SetRollAngle((float)roll.valueRadians());
	//}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Joint6Dof::Joint6Dof(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos1, const Ogre::Vector3& pos2)
		: Joint()
	{
		dCustomJoint* supportJoint;

		// make the joint matrix 
		dMatrix pinsAndPivoFrame1(dGetIdentityMatrix());
		pinsAndPivoFrame1.m_posit = dVector(pos1.x, pos1.y, pos1.z, 1.0f);

		dMatrix pinsAndPivoFrame2(dGetIdentityMatrix());
		pinsAndPivoFrame2.m_posit = dVector(pos2.x, pos2.y, pos2.z, 1.0f);

		// create a Newton Custom joint and set it at the support joint	
		supportJoint = new dCustom6dof(pinsAndPivoFrame1, pinsAndPivoFrame2, child->getNewtonBody(), parent ? parent->getNewtonBody() : nullptr);
		SetSupportJoint(supportJoint);
	}

	Joint6Dof::~Joint6Dof()
	{
		// nothing, the ~Joint() function will take care of us.
	}

	void Joint6Dof::setLinearLimits(const Ogre::Vector3& minLinearLimits, const Ogre::Vector3& maxLinearLimits) const
	{
		dCustom6dof* joint = (dCustom6dof*)GetSupportJoint();
		joint->SetLinearLimits(dVector(minLinearLimits.x, minLinearLimits.y, minLinearLimits.z, 1.0f), dVector(maxLinearLimits.x, maxLinearLimits.y, maxLinearLimits.z, 1.0f));
	}

	void Joint6Dof::setYawLimits(Ogre::Radian minLimits, Ogre::Radian maxLimits) const
	{
		dCustom6dof* joint = (dCustom6dof*)GetSupportJoint();
		joint->SetYawLimits((float)minLimits.valueRadians(), (float)maxLimits.valueRadians());
	}

	void Joint6Dof::setPitchLimits(Ogre::Radian minLimits, Ogre::Radian maxLimits) const
	{
		dCustom6dof* joint = (dCustom6dof*)GetSupportJoint();
		joint->SetPitchLimits((float)minLimits.valueRadians(), (float)maxLimits.valueRadians());
	}

	void Joint6Dof::setRollLimits(Ogre::Radian minLimits, Ogre::Radian maxLimits) const
	{
		dCustom6dof* joint = (dCustom6dof*)GetSupportJoint();
		joint->SetRollLimits((float)minLimits.valueRadians(), (float)maxLimits.valueRadians());
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	//RagDollMotor::RagDollMotor(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos, unsigned int dofCount)
	//	: Joint(),
	//	m_dofCount(dofCount)
	//{
	//	dCustomJoint* supportJoint;

	//	// make the joint matrix 
	//	dMatrix pinsAndPivoFrame(dGetIdentityMatrix());
	//	pinsAndPivoFrame.m_posit = dVector(pos.x, pos.y, pos.z, 1.0f);

	//	// create a Newton Custom joint and set it at the support joint	
	//	if (1 == dofCount)
	//	{
	//		supportJoint = new dCustomRagdollMotor_1dof(pinsAndPivoFrame, child->getNewtonBody(), parent ? parent->getNewtonBody() : nullptr);
	//	}
	//	else if (2 == dofCount)
	//	{
	//		supportJoint = new dCustomRagdollMotor_2dof(pinsAndPivoFrame, child->getNewtonBody(), parent ? parent->getNewtonBody() : nullptr);
	//	}
	//	else
	//	{
	//		supportJoint = new dCustomRagdollMotor_3dof(pinsAndPivoFrame, child->getNewtonBody(), parent ? parent->getNewtonBody() : nullptr);
	//	}
	//	SetSupportJoint(supportJoint);
	//}

	//RagDollMotor::~RagDollMotor()
	//{
	//	// nothing, the ~Joint() function will take care of us.
	//}

	//void RagDollMotor::setLimits(Ogre::Degree maxCone, Ogre::Degree minTwist, Ogre::Degree maxTwist) const
	//{
	//	if (1 == m_dofCount)
	//	{
	//		dCustomRagdollMotor_1dof* joint = (dCustomRagdollMotor_1dof*)GetSupportJoint();
	//		joint->SetTwistAngle((float)minTwist.valueRadians(), (float)maxTwist.valueRadians());
	//	}
	//	else if (2 == m_dofCount)
	//	{
	//		dCustomRagdollMotor_2dof* joint = (dCustomRagdollMotor_2dof*)GetSupportJoint();
	//		joint->SetConeAngle((float)maxCone.valueRadians());
	//	}
	//	else
	//	{
	//		dCustomRagdollMotor_3dof* joint = (dCustomRagdollMotor_3dof*)GetSupportJoint();
	//		joint->SetConeAngle((float)maxCone.valueRadians());
	//		joint->SetTwistAngle((float)minTwist.valueRadians(), (float)maxTwist.valueRadians());
	//	}
	//}

	//void RagDollMotor::setMode(bool ragDollOrMotor)
	//{
	//	dCustomRagdollMotor* joint = (dCustomRagdollMotor*)GetSupportJoint();
	//	joint->SetMode(ragDollOrMotor);
	//}

	//void RagDollMotor::setTorgue(Ogre::Real torgue)
	//{
	//	dCustomRagdollMotor* joint = (dCustomRagdollMotor*)GetSupportJoint();
	//	joint->SetJointTorque(torgue);
	//}

	///////////////////////////////////////////////////////////////////////////////////////////////

	CorkScrew::CorkScrew(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos) : Joint()
	{
		dCustomJoint* supportJoint;

		// make the joint matrix 
		dMatrix pinsAndPivoFrame(dGetIdentityMatrix());
		pinsAndPivoFrame.m_posit = dVector(pos.x, pos.y, pos.z, 1.0f);

		// crate a Newton Custom joint and set it at the support joint	
		supportJoint = new dCustomCorkScrew(pinsAndPivoFrame, child->getNewtonBody(), parent ? parent->getNewtonBody() : nullptr);
		SetSupportJoint(supportJoint);
	}

	CorkScrew::~CorkScrew()
	{
		// nothing, the ~Joint() function will take care of us.
	}

	void CorkScrew::enableLimits(bool linearLimits, bool angularLimits)
	{
		dCustomCorkScrew* joint = (dCustomCorkScrew*)GetSupportJoint();
		joint->EnableLimits(linearLimits);
		joint->EnableAngularLimits(angularLimits);
	}

	void CorkScrew::setLinearLimits(Ogre::Real minLinearDistance, Ogre::Real maxLinearDistance)
	{
		dCustomCorkScrew* joint = (dCustomCorkScrew*)GetSupportJoint();
		joint->SetLimits(static_cast<float>(minLinearDistance), static_cast<float>(maxLinearDistance));
	}

	void CorkScrew::setAngularLimits(Ogre::Degree minAngularAngle, Ogre::Degree maxAngularAngle)
	{
		dCustomCorkScrew* joint = (dCustomCorkScrew*)GetSupportJoint();
		joint->SetAngularLimits(static_cast<float>(minAngularAngle.valueRadians()), static_cast<float>(maxAngularAngle.valueRadians()));
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////
	Hinge::Hinge(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos, const Ogre::Vector3& pin)
		: Joint(),
		m_constraintType(CONSTRAINT_NONE),
		m_lastConstraintType(CONSTRAINT_NONE),
		m_desiredOmega(0.0f),
		m_directionChange(false),
		m_oppositeDir(1.0f),
		m_minAngle(Ogre::Degree(0.0f)),
		m_maxAngle(Ogre::Degree(0.0f)),
		m_desiredAngle(0.0f),
		m_motorTorque(0.0f),
		m_motorMinFriction(0.0f),
		m_motorMaxFriction(0.0f),
		m_brakeMaxForce(0.0f),
		m_torqueOn(false)
	{
		dCustomJoint* supportJoint;

		// make the joint matrix 
		dVector dir(pin.x, pin.y, pin.z, 0.0f);
		dMatrix pinsAndPivoFrame(dGrammSchmidt(dir));
		pinsAndPivoFrame.m_posit = dVector(pos.x, pos.y, pos.z, 1.0f);

		// crate a Newton Custom joint and set it at the support joint	
		supportJoint = new dCustomHinge(pinsAndPivoFrame, child->getNewtonBody(), parent ? parent->getNewtonBody() : nullptr);
		SetSupportJoint(supportJoint);
	}

	Hinge::~Hinge()
	{
	}


	void Hinge::enableLimits(bool state)
	{
		dCustomHinge* joint = (dCustomHinge*)GetSupportJoint();
		joint->EnableLimits(state);

		getBody0()->unFreeze();
		if (getBody1())
			getBody1()->unFreeze();
	}

	void Hinge::setLimits(Ogre::Degree minAngle, Ogre::Degree maxAngle)
	{
		dCustomHinge* joint = (dCustomHinge*)GetSupportJoint();
		m_minAngle = minAngle;
		m_maxAngle = maxAngle;
		if (m_minAngle.valueDegrees() != 0.0f || m_maxAngle.valueDegrees() != 0.0f)
		{
			m_directionChange = true;
		}

		joint->SetLimits(minAngle.valueRadians(), maxAngle.valueRadians());

		getBody0()->unFreeze();
		if (getBody1())
			getBody1()->unFreeze();
	}

	void Hinge::setDesiredOmega(Ogre::Degree omega)
	{
		m_desiredOmega = omega.valueRadians();

		m_constraintType = CONSTRAINT_OMEGA;

		getBody0()->unFreeze();
		if (getBody1())
			getBody1()->unFreeze();
	}

	void Hinge::setTorque(Ogre::Real torque)
	{
		m_motorTorque = torque;
		m_desiredOmega = 0.0f;

		m_torqueOn = true;

		dCustomHinge* joint = (dCustomHinge*)GetSupportJoint();
		if (0.0f != torque)
			joint->EnableMotor(true, torque);
		else
			joint->EnableMotor(false, 0.0f);

		getBody0()->unFreeze();
		if (getBody1())
			getBody1()->unFreeze();
	}

	void Hinge::setDesiredAngle(Ogre::Degree angle, Ogre::Real minFriction, Ogre::Real maxFriction)
	{
		m_desiredAngle = angle.valueRadians();
		m_motorMinFriction = minFriction;
		m_motorMaxFriction = maxFriction;

		m_constraintType = CONSTRAINT_ANGLE;

		getBody0()->unFreeze();
		if (getBody1())
			getBody1()->unFreeze();
	}

	void Hinge::setBrake(Ogre::Real maxForce)
	{
		m_brakeMaxForce = maxForce;
		m_constraintType = CONSTRAINT_BRAKE;
	}

	Ogre::Radian Hinge::getJointAngle() const
	{
		dCustomHinge* joint = (dCustomHinge*)GetSupportJoint();

		return Ogre::Radian(joint->GetJointAngle());
	}

	Ogre::Radian Hinge::getJointAngularVelocity() const
	{
		dCustomHinge* joint = (dCustomHinge*)GetSupportJoint();

		return Ogre::Radian(joint->GetJointOmega());
	}

	Ogre::Vector3 Hinge::getJointPin() const
	{
		dCustomHinge* joint = (dCustomHinge*)GetSupportJoint();
		dVector pin(joint->GetPinAxis());

		return Ogre::Vector3(pin.m_x, pin.m_y, pin.m_z);
	}

	void Hinge::setSpringAndDamping(bool state, Ogre::Real springDamperRelaxation, Ogre::Real spring, Ogre::Real damper)
	{
		dCustomHinge* joint = (dCustomHinge*)GetSupportJoint();
		joint->SetAsSpringDamper(state, springDamperRelaxation, spring, damper);
	}

	// Due to new newton architecture, this is not called anymore
	void Hinge::submitConstraint(Ogre::Real timestep, int threadindex)
	{
		if (m_constraintType == CONSTRAINT_BRAKE)
		{
			Ogre::Vector3 pin(getJointPin());

			addAngularRow(Ogre::Radian(0), pin);
			setRowStiffness(1.0f);

			if (m_brakeMaxForce > 0.0f)
			{
				setRowMinimumFriction(-m_brakeMaxForce);
				setRowMaximumFriction(m_brakeMaxForce);
			}
		}
		else if (m_constraintType == CONSTRAINT_OMEGA)
		{
			if (m_directionChange)
			{
				if (getJointAngle().valueDegrees() >= m_maxAngle.valueDegrees())
				{
					m_oppositeDir *= -1.0f;
				}
				else if (getJointAngle().valueDegrees() <= m_minAngle.valueDegrees())
				{
					m_oppositeDir *= -1.0f;
				}

			}
			// Ogre::LogManager::getSingletonPtr()->logMessage("getJointAngle().valueDegrees(): " + Ogre::StringConverter::toString(getJointAngle().valueDegrees()));
			Ogre::Radian relativeOmega = ((m_desiredOmega * m_oppositeDir) - getJointAngularVelocity());

			Ogre::Real acceleration = relativeOmega.valueRadians() / timestep;
			Ogre::Vector3 pin(getJointPin());

			addAngularRow(Ogre::Radian(0.0f), pin);
			setRowAcceleration(acceleration);
		}
		else if (m_constraintType == CONSTRAINT_ANGLE)
		{
			Ogre::Radian relativeAngle = getJointAngle() - m_desiredAngle;

			Ogre::Vector3 pin(getJointPin());

			addAngularRow(relativeAngle, pin);
			setRowStiffness(1.0f);

			if (m_motorMinFriction < 0.0f)
			{
				setRowMinimumFriction(m_motorMinFriction);
			}

			if (m_motorMaxFriction > 0.0f)
			{
				setRowMaximumFriction(m_motorMaxFriction);
			}
		}

		if (m_torqueOn)
		{
			if (getBody0() != nullptr)
			{
				Ogre::Vector3 pin(getJointPin());

				getBody0()->addTorque(pin * m_motorTorque);

				if (getBody1() != nullptr)
				{
					getBody1()->addTorque(pin * -m_motorTorque);
				}
			}
		}

		m_lastConstraintType = m_constraintType;
	}



	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	Slider::Slider(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos, const Ogre::Vector3& pin) : Joint()
	{
		dCustomJoint* supportJoint;

		// make the joint matrix 
		dVector dir(pin.x, pin.y, pin.z, 0.0f);
		dMatrix pinsAndPivoFrame(dGrammSchmidt(dir));
		pinsAndPivoFrame.m_posit = dVector(pos.x, pos.y, pos.z, 1.0f);

		// crate a Newton Custom joint and set it at the support joint	
		supportJoint = new dCustomSlider(pinsAndPivoFrame, child->getNewtonBody(), parent ? parent->getNewtonBody() : nullptr);
		SetSupportJoint(supportJoint);
	}

	Slider::~Slider()
	{

	}

	void Slider::enableLimits(bool state)
	{
		dCustomSlider* joint = (dCustomSlider*)GetSupportJoint();
		joint->EnableLimits(state);
	}

	void Slider::setLimits(Ogre::Real minStopDist, Ogre::Real maxStopDist)
	{
		dCustomSlider* joint = (dCustomSlider*)GetSupportJoint();
		joint->SetLimits(minStopDist, maxStopDist);
	}

	void Slider::setSpringAndDamping(bool state, Ogre::Real springDamperRelaxation, Ogre::Real spring, Ogre::Real damper)
	{
		dCustomSlider* joint = (dCustomSlider*)GetSupportJoint();
		joint->SetAsSpringDamper(state, springDamperRelaxation, spring, damper);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	class CustomActiveSlider : public dCustomSlider
	{
	public:
		CustomActiveSlider(const dMatrix& pinAndPivotFrame, NewtonBody* const child, NewtonBody* const parent, const Ogre::Vector3& pin,
			Ogre::Real minStopDistance, Ogre::Real maxStopDistance, Ogre::Real moveSpeed, bool repeat, bool directionChange, bool activated)
			: dCustomSlider(pinAndPivotFrame, child, parent),
			pin(pin),
			minStopDistance(minStopDistance),
			maxStopDistance(maxStopDistance),
			moveSpeed(moveSpeed),
			repeat(repeat),
			directionChange(directionChange),
			activated(activated),
			oppositeDir(1.0f),
			progress(0.0f),
			round(0)
		{
			// Active slider body should never sleep, because newton checks in SubmitConstraints, if after the function, something phyically change, if 2-times nothing changed
			// SubmitConstraints is no more called, until the body will touched by some kind of force
			Body* body0 = (Body*)NewtonBodyGetUserData(m_body0);
			if (nullptr != body0)
			{
				body0->setAutoSleep(false == activated);
			}
		}

		virtual ~CustomActiveSlider()
		{
		}

		// the important function that applies the joint.
		virtual void SubmitConstraints(Ogre::Real timestep, int threadindex) override
		{
			dCustomSlider::SubmitConstraints(timestep, threadindex);
			// http://www.ogre3d.org/addonforums/viewtopic.php?f=4&t=3969&p=22907&hilit=setRowSpringDamper#p22907
			// http://newtondynamics.com/forum/viewtopic.php?f=9&t=6639&p=46682&hilit=NewtonUserJointSetRowSpringDamperAcceleration#p46682
			//  0.991{K spring stifness}, 0.991{D spring damper});
			// this->setStiffness(0.1f * timestep);
			// this->setRowSpringDamper(0.9f, 0.1f);
			// this->setRowStiffness(0.1f * timestep);
			// this->setRowMaximumFriction(0.9f);
			// this->setRowMinimumFriction(0.8f);

			if (false == this->activated)
			{
				return;
			}

			// this function is called e.g. 60 times per second, so scale with timestep (0.016)

			// Ogre::LogManager::getSingletonPtr()->logMessage("progress: " + Ogre::StringConverter::toString(this->progress));

			if (Ogre::Math::RealEqual(this->oppositeDir, 1.0f))
			{
				this->progress += this->moveSpeed * timestep;
				if (Ogre::Math::RealEqual(this->progress, this->maxStopDistance - 0.1f, timestep))
				{
					// Ogre::LogManager::getSingletonPtr()->logMessage("dir change from +  to -");
					if (this->directionChange)
					{
						oppositeDir *= -1.0f;
					}
					round++;
				}
			}
			else
			{
				this->progress -= this->moveSpeed * timestep;
				// if (this->progress <= currentJointProperties.minStopDistance)
				if (Ogre::Math::RealEqual(this->progress, this->minStopDistance + 0.1f, timestep))
				{
					// Ogre::LogManager::getSingletonPtr()->logMessage("dir change from -  to +");
					if (this->directionChange)
					{
						oppositeDir *= -1.0f;
					}
					round++;
				}
			}
			// if the slider took 2 rounds (1x forward and 1x back, then its enough, if repeat is off)
			// also take the progress into account, the slider started at zero and should stop at zero
			// maybe here attribute in xml, to set how many rounds the slider should proceed
			if (round == 2 && this->progress >= 0.0f)
			{
				// if repeat is of, only change the direction one time, to get back to its origin and leave
				if (!this->repeat)
				{
					this->directionChange = false;
				}
			}
			// when repeat is on, proceed
			// when direction change is on, proceed
			if (this->repeat || this->directionChange || round)
			{
				Body* body0 = (Body*)NewtonBodyGetUserData(m_body0);
				if (nullptr != body0)
				{
					body0->setVelocity(this->pin * (this->moveSpeed * oppositeDir));
				}
				// kann nur 1x aufgerufen werden und gibt die tangetiale ausrichtung an!
				// this->addAngularRow(Ogre::Radian(0.1f), this->currentJointProperties.pin);
				// daher vll active slider kein slider sondern ein body mit movecallback und dort dann diese funktion?
			}
			else
			{
				Body* body0 = (Body*)NewtonBodyGetUserData(m_body0);
				if (nullptr != body0)
				{
					body0->setVelocity(Ogre::Vector3::ZERO);
				}
			}
		}

		void setActivated(bool activated)
		{
			this->activated = activated;
			Body* body0 = (Body*)NewtonBodyGetUserData(m_body0);
			if (nullptr != body0)
			{
				body0->setAutoSleep(false == activated);
			}
		}

		void setMoveSpeed(Ogre::Real moveSpeed)
		{
			this->moveSpeed = moveSpeed;
		}

		void setRepeat(bool repeat)
		{
			this->repeat = repeat;
		}

		void setDirectionChange(bool directionChange)
		{
			this->directionChange = directionChange;
		}

		int getCurrentDirection(void) const
		{
			return static_cast<int>(this->oppositeDir);
		}

	public:
		Ogre::Vector3 pin;
		Ogre::Real minStopDistance;
		Ogre::Real maxStopDistance;
		Ogre::Real moveSpeed;
		bool repeat;
		bool directionChange;
		bool activated;
		Ogre::Real oppositeDir;
		Ogre::Real progress;
		short round;
	};


	ActiveSliderJoint::ActiveSliderJoint(OgreNewt::Body* currentBody, OgreNewt::Body* predecessorBody, const Ogre::Vector3& anchorPosition, const Ogre::Vector3& pin,
		Ogre::Real minStopDistance, Ogre::Real maxStopDistance, Ogre::Real moveSpeed, bool repeat, bool directionChange, bool activated)
		: Joint()
	{
		
		// make the joint matrix 
		dVector dir(pin.x, pin.y, pin.z, 0.0f);
		dMatrix pinsAndPivoFrame(dGrammSchmidt(dir));
		pinsAndPivoFrame.m_posit = dVector(anchorPosition.x, anchorPosition.y, anchorPosition.z, 1.0f);

		// crate a Newton Custom joint and set it at the support joint	
		CustomActiveSlider* supportJoint = new CustomActiveSlider(pinsAndPivoFrame, currentBody->getNewtonBody(), predecessorBody ? predecessorBody->getNewtonBody() : nullptr,
			pin, minStopDistance, maxStopDistance, moveSpeed, repeat, directionChange, activated);
		SetSupportJoint(supportJoint);

		if (0.0f != minStopDistance || 0.0f != maxStopDistance)
		{
			supportJoint->EnableLimits(true);
			supportJoint->SetLimits(minStopDistance, maxStopDistance);
		}
		else
		{
			supportJoint->EnableLimits(false);
		}
	}

	void ActiveSliderJoint::setActivated(bool activated)
	{
		CustomActiveSlider* joint = (CustomActiveSlider*)GetSupportJoint();
		joint->setActivated(activated);
	}

	void ActiveSliderJoint::enableLimits(bool state)
	{
		CustomActiveSlider* joint = (CustomActiveSlider*)GetSupportJoint();
		joint->EnableLimits(state);
	}


	void ActiveSliderJoint::setLimits(Ogre::Real minStopDist, Ogre::Real maxStopDist)
	{
		CustomActiveSlider* joint = (CustomActiveSlider*)GetSupportJoint();

		if (0.0f != minStopDist || 0.0f != maxStopDist)
		{
			joint->EnableLimits(true);
		}
		else
		{
			joint->EnableLimits(false);
		}

		joint->SetLimits(minStopDist, maxStopDist);
	}

	void ActiveSliderJoint::setMoveSpeed(Ogre::Real moveSpeed)
	{
		CustomActiveSlider* joint = (CustomActiveSlider*)GetSupportJoint();
		joint->setMoveSpeed(moveSpeed);
	}

	void ActiveSliderJoint::setRepeat(bool repeat)
	{
		CustomActiveSlider* joint = (CustomActiveSlider*)GetSupportJoint();
		joint->setRepeat(repeat);
	}

	void ActiveSliderJoint::setDirectionChange(bool directionChange)
	{
		CustomActiveSlider* joint = (CustomActiveSlider*)GetSupportJoint();
		joint->setDirectionChange(directionChange);
	}

	int ActiveSliderJoint::getCurrentDirection(void) const
	{
		CustomActiveSlider* joint = (CustomActiveSlider*)GetSupportJoint();
		return joint->getCurrentDirection();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	SlidingContact::SlidingContact(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos, const Ogre::Vector3& pin)
		: Joint()
	{
		dCustomJoint* supportJoint;

		// make the joint matrix 
		dVector dir(pin.x, pin.y, pin.z, 0.0f);
		dMatrix pinsAndPivoFrame(dGrammSchmidt(dir));
		pinsAndPivoFrame.m_posit = dVector(pos.x, pos.y, pos.z, 1.0f);

		// crate a Newton Custom joint and set it at the support joint	
		supportJoint = new dCustomSlidingContact(pinsAndPivoFrame, child->getNewtonBody(), parent ? parent->getNewtonBody() : nullptr);
		SetSupportJoint(supportJoint);
	}

	SlidingContact::~SlidingContact()
	{
	}

	void SlidingContact::enableLinearLimits(bool state)
	{
		dCustomSlidingContact* joint = (dCustomSlidingContact*)GetSupportJoint();
		joint->EnableLimits(state);
	}

	void SlidingContact::enableAngularLimits(bool state)
	{
		dCustomSlidingContact* joint = (dCustomSlidingContact*)GetSupportJoint();
		joint->EnableAngularLimits(state);
	}


	void SlidingContact::setLinearLimits(Ogre::Real minStopDist, Ogre::Real maxStopDist)
	{
		dCustomSlidingContact* joint = (dCustomSlidingContact*)GetSupportJoint();
		joint->SetLimits(minStopDist, maxStopDist);
	}

	void SlidingContact::setAngularLimits(Ogre::Degree minAngle, Ogre::Degree maxAngle)
	{
		dCustomSlidingContact* joint = (dCustomSlidingContact*)GetSupportJoint();
		joint->SetAngularLimits((float)minAngle.valueRadians(), (float)maxAngle.valueRadians());
	}

	void SlidingContact::setSpringAndDamping(bool state, Ogre::Real springDamperRelaxation, Ogre::Real spring, Ogre::Real damper)
	{
		dCustomSlidingContact* joint = (dCustomSlidingContact*)GetSupportJoint();
		joint->SetAsSpringDamper(state, springDamperRelaxation, spring, damper);
	}

	/////////////////////////////////////////////////////////////////////////////////////////

	UpVector::UpVector(const Body* body, const Ogre::Vector3& pin)
		: Joint(),
		m_pin(pin.normalisedCopy())
	{
		dVector dPin(m_pin.x, m_pin.y, m_pin.z, 1.0f);
		dCustomUpVector* supportJoint = new dCustomUpVector(dPin, body->getNewtonBody());
		SetSupportJoint(supportJoint);
	}

	UpVector::~UpVector()
	{
	}

	void UpVector::setPin(const Ogre::Vector3& pin)
	{
		dCustomUpVector* supportJoint = static_cast<dCustomUpVector*>(m_joint);
		m_pin = pin.normalisedCopy();
		dVector dPin(m_pin.x, m_pin.y, m_pin.z, 1.0f);
		supportJoint->SetPinDir(dPin);
	}

	const Ogre::Vector3& UpVector::getPin() const
	{
		return m_pin;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////

	KinematicController::KinematicController(const OgreNewt::Body* child, const Ogre::Vector3& pos)
	{
		dCustomJoint* supportJoint;

		// make the joint matrix 
		dVector attachement(pos.x, pos.y, pos.z, 1.0f);

		// crate a Newton Custom joint and set it at the support joint	
		supportJoint = new dCustomKinematicController(child->getNewtonBody(), attachement);
		SetSupportJoint(supportJoint);
	}

	KinematicController::~KinematicController()
	{
	}

	void KinematicController::setPickingMode(int mode)
	{
		dCustomKinematicController* joint = (dCustomKinematicController*)GetSupportJoint();
		joint->SetPickMode(mode);
	}

	void KinematicController::setMaxLinearFriction(Ogre::Real accel)
	{
		dCustomKinematicController* joint = (dCustomKinematicController*)GetSupportJoint();
		joint->SetMaxLinearFriction(accel);
	}

	void KinematicController::setMaxAngularFriction(Ogre::Real alpha)
	{
		dCustomKinematicController* joint = (dCustomKinematicController*)GetSupportJoint();
		joint->SetMaxAngularFriction(alpha);
	}

	void KinematicController::setTargetPosit(const Ogre::Vector3& position)
	{
		dVector posit(position.x, position.y, position.z, 0.0f);
		dCustomKinematicController* joint = (dCustomKinematicController*)GetSupportJoint();
		joint->SetTargetPosit(posit);
	}

	void KinematicController::setTargetRotation(const Ogre::Quaternion& rotation)
	{
		dQuaternion rotat(rotation.w, rotation.x, rotation.y, rotation.z);

		dCustomKinematicController* joint = (dCustomKinematicController*)GetSupportJoint();
		joint->SetTargetRotation(rotat);
	}

	void KinematicController::setTargetMatrix(const Ogre::Vector3& position, const Ogre::Quaternion& rotation)
	{
		dMatrix matrix;
		OgreNewt::Converters::QuatPosToMatrix(rotation, position, &matrix[0][0]);

		dCustomKinematicController* joint = (dCustomKinematicController*)GetSupportJoint();
		joint->SetTargetMatrix(matrix);
	}

	void KinematicController::getTargetMatrix(Ogre::Vector3& position, Ogre::Quaternion& rotation) const
	{
		dCustomKinematicController* joint = (dCustomKinematicController*)GetSupportJoint();
		dMatrix matrix(joint->GetTargetMatrix());

		OgreNewt::Converters::MatrixToQuatPos(&matrix[0][0], rotation, position);
	}

	// copied from CustomDryRollingFriction joint in newton
	// rolling friction works as follow: the idealization of the contact of a spherical object 
	// with a another surface is a point that pass by the center of the sphere.
	// in most cases this is enough to model the collision but in insufficient for modeling 
	// the rolling friction. In reality contact with the sphere with the other surface is not 
	// a point but a contact patch. A contact patch has the property the it generates a fix 
	// constant rolling torque that opposes the movement of the sphere.
	// we can model this torque by adding a clamped torque aligned to the instantaneously axis 
	// of rotation of the ball. and with a magnitude of the stopping angular acceleration.

	//////////////////////////////////CustomDryRollingFriction///////////////////////////////////////////////////

	CustomDryRollingFriction::CustomDryRollingFriction(OgreNewt::Body* child, Ogre::Real radius, Ogre::Real rollingFrictionCoefficient)
	{

		dCustomJoint* supportJoint;

		// crate a Newton Custom joint and set it at the support joint	
		supportJoint = new dCustomDryRollingFriction(child->getNewtonBody(), radius, rollingFrictionCoefficient);
		SetSupportJoint(supportJoint);
	}

	CustomDryRollingFriction::~CustomDryRollingFriction()
	{
	}

	class CustomPathFollow : public dCustomPathFollow
	{
	public:
		CustomPathFollow(const dMatrix& pinAndPivotFrame, NewtonBody* const body, NewtonBody* const pathBody)
			: dCustomPathFollow(pinAndPivotFrame, body, pathBody)
		{
		}

		void GetPointAndTangentAtLocation(const dVector& location, dVector& positOut, dVector& tangentOut) const
		{
			Body* const pathBody = (Body*)NewtonBodyGetUserData(GetBody1());
			const dBezierSpline& spline = pathBody->getSpline();

			dMatrix matrix;
			NewtonBodyGetMatrix(GetBody1(), &matrix[0][0]);

			dVector p(matrix.UntransformVector(location));
			dBigVector point;
			dFloat64 knot = spline.FindClosestKnot(point, p, 4);
			dBigVector tangent(spline.CurveDerivative(knot));
			tangent = tangent.Scale(1.0 / sqrt(tangent.DotProduct3(tangent)));

			// positOut = matrix.TransformVector (dVector (point.m_x, point.m_y, point.m_z));
			positOut = matrix.TransformVector(point);
			tangentOut = tangent;
		}
	};

	///////////////////////////////////////////////FollowPath///////////////////////////////

	// GetPointAndTangentAtLocation must be overwritten, see sdk examples!
	PathFollow::PathFollow(OgreNewt::Body* child, OgreNewt::Body* pathBody, const Ogre::Vector3& pos)
		: Joint(),
		m_childBody(child),
		m_pathBody(pathBody),
		m_pos(pos)
	{
		
	}

	PathFollow::~PathFollow()
	{
		// nothing, the ~Joint() function will take care of us.
	}

	void PathFollow::setKnots(const std::vector<Ogre::Vector3>& knots)
	{
		// create a Bezier Spline path
		dBezierSpline spline;

		std::vector<double> knotsIndices(knots.size() - 1);
		for (size_t i = 0; i < knotsIndices.size(); i++)
			knotsIndices[i] = static_cast<double>(i) / static_cast<double>(knotsIndices.size() - 1);

		std::vector<dBigVector> internalKnots(knots.size());

		for (size_t i = 0; i < internalKnots.size(); i++)
			internalKnots[i] = dBigVector(knots[i].x, knots[i].y, knots[i].z);

		spline.CreateFromKnotVectorAndControlPoints(2, sizeof(knotsIndices) / sizeof(knotsIndices[0]) + 1, &knotsIndices[0], &internalKnots[0]);

		m_pathBody->setSpline(spline);

		dCustomJoint* supportJoint;

		// make the joint matrix 
		dMatrix pinsAndPivoFrame(dGetIdentityMatrix());
		pinsAndPivoFrame.m_posit = dVector(m_pos.x, m_pos.y, m_pos.z, 1.0f);

		// crate a Newton Custom joint and set it at the support joint	
		supportJoint = new CustomPathFollow(pinsAndPivoFrame, m_childBody->getNewtonBody(), m_pathBody->getNewtonBody());
		SetSupportJoint(supportJoint);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////
	Gear::Gear(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& childPin, const Ogre::Vector3& parentPin, Ogre::Real gearRatio)
		: Joint()
	{
		dCustomJoint* supportJoint;

		dVector childDir(childPin.x, childPin.y, childPin.z, 1.0f);
		dVector parentDir(parentPin.x, parentPin.y, parentPin.z, 1.0f);

		// create a Newton Custom joint and set it at the support joint	
		supportJoint = new dCustomGear(gearRatio, childDir, parentDir, child->getNewtonBody(), parent ? parent->getNewtonBody() : nullptr);
		SetSupportJoint(supportJoint);
	}

	Gear::~Gear()
	{

	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////
	WormGear::WormGear(const OgreNewt::Body* rotationalBody, const OgreNewt::Body* linearBody, const Ogre::Vector3& rotationalPin, const Ogre::Vector3& linearPin, Ogre::Real gearRatio)
		: Joint()
	{
		dCustomJoint* supportJoint;

		dVector rationalDir(rotationalPin.x, rotationalPin.y, rotationalPin.z, 1.0f);
		dVector linearDir(linearPin.x, linearPin.y, linearPin.z, 1.0f);

		// create a Newton Custom joint and set it at the support joint	
		supportJoint = new dCustomRackAndPinion(gearRatio, rationalDir, linearDir, rotationalBody->getNewtonBody(), linearBody ? linearBody->getNewtonBody() : nullptr);
		SetSupportJoint(supportJoint);

		// supportJoint->SetStiffness(1.0f);
	}

	WormGear::~WormGear()
	{

	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////
	CustomGearAndSlide::CustomGearAndSlide(const OgreNewt::Body* rotationalBody, const OgreNewt::Body* linearBody, const Ogre::Vector3& rotationalPin, 
		const Ogre::Vector3& linearPin, Ogre::Real gearRatio, Ogre::Real slideRatio)
		: Joint()
	{
		dCustomJoint* supportJoint;

		dVector rationalDir(rotationalPin.x, rotationalPin.y, rotationalPin.z, 1.0f);
		dVector linearDir(linearPin.x, linearPin.y, linearPin.z, 1.0f);

		// create a Newton Custom joint and set it at the support joint	
		supportJoint = new dCustomGearAndSlide(gearRatio, slideRatio, rationalDir, linearDir, rotationalBody->getNewtonBody(), linearBody ? linearBody->getNewtonBody() : nullptr);
		SetSupportJoint(supportJoint);

		// supportJoint->SetStiffness(1.0f);
	}

	CustomGearAndSlide::~CustomGearAndSlide()
	{

	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	Pulley::Pulley(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& childPin, const Ogre::Vector3& parentPin, Ogre::Real pulleyRatio)
		: Joint()
	{
		dCustomJoint* supportJoint;

		dVector childDir(childPin.x, childPin.y, childPin.z, 1.0f);
		dVector parentDir(parentPin.x, parentPin.y, parentPin.z, 1.0f);

		// create a Newton Custom joint and set it at the support joint	
		supportJoint = new dCustomPulley(pulleyRatio, childDir, parentDir, child->getNewtonBody(), parent ? parent->getNewtonBody() : nullptr);
		SetSupportJoint(supportJoint);
	}

	Pulley::~Pulley()
	{

	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////

	Universal::Universal(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos)
		: Joint()
	{
		dCustomJoint* supportJoint;

		// make the joint matrix 
		dMatrix pinsAndPivoFrame(dGetIdentityMatrix());
		pinsAndPivoFrame.m_posit = dVector(pos.x, pos.y, pos.z, 1.0f);

		// Create a Newton Custom joint and set it at the support joint	
		supportJoint = new dCustomDoubleHinge(pinsAndPivoFrame, child->getNewtonBody(), parent ? parent->getNewtonBody() : nullptr);
		SetSupportJoint(supportJoint);
	}

	Universal::~Universal()
	{
		// nothing, the ~Joint() function will take care of us.
	}

	void Universal::enableLimits0(bool state0)
	{
		dCustomDoubleHinge* joint = (dCustomDoubleHinge*)GetSupportJoint();
		joint->EnableLimits(state0);
	}

	void Universal::setLimits0(Ogre::Degree minAngle0, Ogre::Degree maxAngle0)
	{
		dCustomDoubleHinge* joint = (dCustomDoubleHinge*)GetSupportJoint();
		joint->SetLimits((float)minAngle0.valueRadians(), (float)maxAngle0.valueRadians());
	}

	void Universal::enableLimits1(bool state1)
	{
		dCustomDoubleHinge* joint = (dCustomDoubleHinge*)GetSupportJoint();
		joint->EnableLimits1(state1);
	}

	void Universal::setLimits1(Ogre::Degree minAngle1, Ogre::Degree maxAngle1)
	{
		dCustomDoubleHinge* joint = (dCustomDoubleHinge*)GetSupportJoint();
		joint->SetLimits1((float)minAngle1.valueRadians(), (float)maxAngle1.valueRadians());
	}

	void Universal::enableMotor0(bool state0, Ogre::Real motorSpeed0)
	{
		dCustomDoubleHinge* joint = (dCustomDoubleHinge*)GetSupportJoint();
		joint->EnableMotor(state0, motorSpeed0);
	}

	void Universal::setFriction0(Ogre::Real frictionTorque)
	{
		dCustomDoubleHinge* joint = (dCustomDoubleHinge*)GetSupportJoint();
		joint->SetFriction(frictionTorque);
	}

	void Universal::setFriction1(Ogre::Real frictionTorque)
	{
		dCustomDoubleHinge* joint = (dCustomDoubleHinge*)GetSupportJoint();
		joint->SetFriction1(frictionTorque);
	}

	void Universal::setAsSpringDamper0(bool state, Ogre::Real springDamperRelaxation, Ogre::Real spring, Ogre::Real damper)
	{
		dCustomDoubleHinge* joint = (dCustomDoubleHinge*)GetSupportJoint();
		joint->SetAsSpringDamper(state, springDamperRelaxation, spring, damper);
	}

	void Universal::setAsSpringDamper1(bool state, Ogre::Real springDamperRelaxation, Ogre::Real spring, Ogre::Real damper)
	{
		dCustomDoubleHinge* joint = (dCustomDoubleHinge*)GetSupportJoint();
		joint->SetAsSpringDamper1(state, springDamperRelaxation, spring, damper);
	}

	void Universal::setHardMiddleAxis(bool state)
	{
		dCustomDoubleHinge* joint = (dCustomDoubleHinge*)GetSupportJoint();
		joint->SetHardMiddleAxis(state);
	}

#if 0
	Ogre::Vector3 Slider::getJointForce() const
	{
		Ogre::Vector3 ret;

		NewtonSliderGetJointForce(m_joint, &ret.x);

		return ret;
	}

	unsigned _CDECL Slider::newtonCallback(const NewtonJoint* slider, NewtonHingeSliderUpdateDesc* desc)
	{
		Slider* me = (Slider*)NewtonJointGetUserData(slider);

		me->m_desc = desc;
		me->m_retval = 0;

		if (me->m_callback)
			(*me->m_callback)(me);

		me->m_desc = nullptr;

		return me->m_retval;
	}


	/////// CALLBACK FUNCTIONS ///////
	void Slider::setCallbackAccel(Ogre::Real accel)
	{
		if (m_desc)
		{
			m_retval = 1;
			m_desc->m_accel = (float)accel;
		}
	}

	void Slider::setCallbackFrictionMin(Ogre::Real min)
	{
		if (m_desc)
		{
			m_retval = 1;
			m_desc->m_minFriction = (float)min;
		}
	}

	void Slider::setCallbackFrictionMax(Ogre::Real max)
	{
		if (m_desc)
		{
			m_retval = 1;
			m_desc->m_maxFriction = (float)max;
		}
	}

	Ogre::Real Slider::getCallbackTimestep() const
	{
		if (m_desc)
			return (Ogre::Real)m_desc->m_timestep;
		else
			return 0.0;
	}

	Ogre::Real Slider::calculateStopAccel(Ogre::Real dist) const
	{
		if (m_desc)
			return (Ogre::Real)NewtonSliderCalculateStopAccel(m_joint, m_desc, (float)dist);
		else
			return 0.0;
	}



	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////



	Universal::Universal(const World* world, const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos, const Ogre::Vector3& pin0, const Ogre::Vector3& pin1) : Joint()
	{
		m_world = world;

		if (parent)
		{
			m_joint = NewtonConstraintCreateUniversal(world->getNewtonWorld(), &pos.x, &pin0.x, &pin1.x,
				child->getNewtonBody(), parent->getNewtonBody());
		}
		else
		{
			m_joint = NewtonConstraintCreateUniversal(world->getNewtonWorld(), &pos.x, &pin0.x, &pin1.x,
				child->getNewtonBody(), nullptr);
		}

		NewtonJointSetUserData(m_joint, this);
		NewtonJointSetDestructor(m_joint, destructor);
		NewtonUniversalSetUserCallback(m_joint, newtonCallback);

		m_callback = nullptr;
	}

	Universal::~Universal()
	{
	}

	Ogre::Vector3 Universal::getJointForce() const
	{
		Ogre::Vector3 ret;

		NewtonUniversalGetJointForce(m_joint, &ret.x);

		return ret;
	}

	unsigned _CDECL Universal::newtonCallback(const NewtonJoint* universal, NewtonHingeSliderUpdateDesc* desc)
	{
		Universal* me = (Universal*)NewtonJointGetUserData(universal);

		me->m_desc = desc;
		me->m_retval = 0;

		if (me->m_callback)
			(*me->m_callback)(me);

		me->m_desc = nullptr;

		return me->m_retval;
	}


	/////// CALLBACK FUNCTIONS ///////
	void Universal::setCallbackAccel(Ogre::Real accel, unsigned int axis)
	{
		if (axis > 1)
		{
			return;
		}

		if (m_desc)
		{
			m_retval |= axis;
			m_desc[axis].m_accel = (float)accel;
		}
	}

	void Universal::setCallbackFrictionMax(Ogre::Real max, unsigned int axis)
	{
		if (axis > 1)
		{
			return;
		}

		if (m_desc)
		{
			m_retval |= axis;
			m_desc[axis].m_maxFriction = (float)max;
		}
	}

	void Universal::setCallbackFrictionMin(Ogre::Real min, unsigned int axis)
	{
		if (axis > 1)
		{
			return;
		}

		if (m_desc)
		{
			m_retval |= axis;
			m_desc[axis].m_minFriction = (float)min;
		}
	}

	Ogre::Real Universal::getCallbackTimestep() const
	{
		if (m_desc)
			return (Ogre::Real)m_desc->m_timestep;
		else
			return 0.0;
	}

	Ogre::Real Universal::calculateStopAlpha0(Ogre::Real angle) const
	{
		if (m_desc)
			return (Ogre::Real)NewtonUniversalCalculateStopAlpha0(m_joint, m_desc, (float)angle);
		else
			return 0.0;
	}

	Ogre::Real Universal::calculateStopAlpha1(Ogre::Real angle) const
	{
		if (m_desc)
			return (Ogre::Real)NewtonUniversalCalculateStopAlpha1(m_joint, m_desc, (float)angle);
		else
			return 0.0;
	}


	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////



	UpVector::UpVector(const World* world, const Body* body, const Ogre::Vector3& pin)
	{
		m_world = world;

		m_joint = NewtonConstraintCreateUpVector(world->getNewtonWorld(), &pin.x, body->getNewtonBody());

		NewtonJointSetUserData(m_joint, this);
		NewtonJointSetDestructor(m_joint, destructor);

	}

	UpVector::~UpVector()
	{
	}

	Ogre::Vector3 UpVector::getPin() const
	{
		Ogre::Vector3 ret;

		NewtonUpVectorGetPin(m_joint, &ret.x);

		return ret;
	}


#endif







#if 0

	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////



	///////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////////////////////


	namespace PrebuiltCustomJoints
	{

		Custom2DJoint::Custom2DJoint(const OgreNewt::Body* body, const Ogre::Vector3& pin) : CustomJoint(4, body, nullptr)
		{
			mPin = pin;
			Ogre::Quaternion bodyorient;
			Ogre::Vector3 bodypos;

			body->getPositionOrientation(bodypos, bodyorient);

			pinAndDirToLocal(bodypos, pin, mLocalOrient0, mLocalPos0, mLocalOrient1, mLocalPos1);

			// initialize variables
			mMin = mMax = Ogre::Degree(0);
			mLimitsOn = false;
			mAccel = 0.0f;

		}


		void Custom2DJoint::submitConstraint(Ogre::Real timeStep, int threadIndex)
		{
			// get the global orientations.
			Ogre::Quaternion globalOrient0, globalOrient1;
			Ogre::Vector3 globalPos0, globalPos1;

			localToGlobal(mLocalOrient0, mLocalPos0, globalOrient0, globalPos0, 0);
			localToGlobal(mLocalOrient1, mLocalPos1, globalOrient1, globalPos1, 1);

			// calculate all main 6 vectors.
			Ogre::Vector3 bod0X = globalOrient0 * Ogre::Vector3(Ogre::Vector3::UNIT_X);
			Ogre::Vector3 bod0Y = globalOrient0 * Ogre::Vector3(Ogre::Vector3::UNIT_Y);
			Ogre::Vector3 bod0Z = globalOrient0 * Ogre::Vector3(Ogre::Vector3::UNIT_Z);

			Ogre::Vector3 bod1X = globalOrient1 * Ogre::Vector3(Ogre::Vector3::UNIT_X);
			Ogre::Vector3 bod1Y = globalOrient1 * Ogre::Vector3(Ogre::Vector3::UNIT_Y);
			Ogre::Vector3 bod1Z = globalOrient1 * Ogre::Vector3(Ogre::Vector3::UNIT_Z);

#ifdef _DEBUG
			Ogre::LogManager::getSingletonPtr()->logMessage(" Submit Constraint   bod0X:" + Ogre::StringConverter::toString(bod0X) +
				"   bod1X:" + Ogre::StringConverter::toString(bod1X));
#endif

			// ---------------------------------------------------------------
			// first add a linear row to keep the body on the plane.
			addLinearRow(globalPos0, globalPos1, bod0X);

			// have we strayed from the plane along the normal?
			Ogre::Plane thePlane(bod0X, globalPos0);
			Ogre::Real stray = thePlane.getDistance(globalPos1);
			if (stray > 0.0001f)
			{
				// we have strayed, apply acceleration to move back to 0 in one timestep.
				Ogre::Real accel = (stray / timeStep);
				if (thePlane.getSide(globalPos1) == Ogre::Plane::NEGATIVE_SIDE)
				{
					accel = -accel;
				}

				setRowAcceleration(accel);
			}

			// see if the main axis (pin) has wandered off.
			Ogre::Vector3 latDir = bod0X.crossProduct(bod1X);
			Ogre::Real latMag = latDir.squaredLength();

			if (latMag > 1.0e-6f)
			{
				// has wandered a bit, we need to correct.  first find the angle off.
				latMag = Ogre::Math::Sqrt(latMag);
				latDir.normalise();
				Ogre::Radian angle = Ogre::Math::ASin(latMag);

				// ---------------------------------------------------------------
				addAngularRow(angle, latDir);

				// ---------------------------------------------------------------
				// secondary correction for stability.
				Ogre::Vector3 latDir2 = latDir.crossProduct(bod1X);
				addAngularRow(Ogre::Radian(0.0f), latDir2);
			}
			else
			{
				// ---------------------------------------------------------------
				// no major change, just add 2 simple constraints.
				addAngularRow(Ogre::Radian(0.0f), bod1Y);
				addAngularRow(Ogre::Radian(0.0f), bod1Z);
			}

			// calculate the current angle.
			Ogre::Real cos = bod0Y.dotProduct(bod1Y);
			Ogre::Real sin = (bod0Y.crossProduct(bod1Y)).dotProduct(bod0X);

			mAngle = Ogre::Math::ATan2(sin, cos);

			if (mLimitsOn)
			{
				if (mAngle > mMax)
				{
					Ogre::Radian diff = mAngle - mMax;

					addAngularRow(diff, bod0X);
					setRowStiffness(1.0f);
				}
				else if (mAngle < mMin)
				{
					Ogre::Radian diff = mAngle - mMin;

					addAngularRow(diff, bod0X);
					setRowStiffness(1.0f);
				}
			}
			else
			{
				if (mAccel != 0.0f)
				{
					addAngularRow(Ogre::Radian(0.0f), bod0X);
					setRowAcceleration(mAccel);

					mAccel = 0.0f;
				}
			}

		}


		CustomRigidJoint::CustomRigidJoint(OgreNewt::Body *child, OgreNewt::Body *parent, Ogre::Vector3 dir, Ogre::Vector3 pos) : OgreNewt::CustomJoint(6, child, parent)
		{
			// calculate local offsets.
			pinAndDirToLocal(pos, dir, mLocalOrient0, mLocalPos0, mLocalOrient1, mLocalPos1);
		}

		CustomRigidJoint::~CustomRigidJoint()
		{
		}

		void CustomRigidJoint::submitConstraint(Ogre::Real timeStep, int threadIndex)
		{
			// get globals.
			Ogre::Vector3 globalPos0, globalPos1;
			Ogre::Quaternion globalOrient0, globalOrient1;

			localToGlobal(mLocalOrient0, mLocalPos0, globalOrient0, globalPos0, 0);
			localToGlobal(mLocalOrient1, mLocalPos1, globalOrient1, globalPos1, 1);

			// apply the constraints!
			addLinearRow(globalPos0, globalPos1, globalOrient0 * Ogre::Vector3::UNIT_X);
			addLinearRow(globalPos0, globalPos1, globalOrient0 * Ogre::Vector3::UNIT_Y);
			addLinearRow(globalPos0, globalPos1, globalOrient0 * Ogre::Vector3::UNIT_Z);

			// now find a point off 10 units away.
			globalPos0 = globalPos0 + (globalOrient0 * (Ogre::Vector3::UNIT_X * 10.0f));
			globalPos1 = globalPos1 + (globalOrient1 * (Ogre::Vector3::UNIT_X * 10.0f));

			// apply the constraints!
			addLinearRow(globalPos0, globalPos1, globalOrient0 * Ogre::Vector3::UNIT_Y);
			addLinearRow(globalPos0, globalPos1, globalOrient0 * Ogre::Vector3::UNIT_Z);

			Ogre::Vector3 xdir0 = globalOrient0 * Ogre::Vector3::UNIT_X;
			Ogre::Vector3 xdir1 = globalOrient1 * Ogre::Vector3::UNIT_X;

			Ogre::Radian angle = Ogre::Math::ACos(xdir0.dotProduct(xdir1));
			addAngularRow(angle, globalOrient0 * Ogre::Vector3::UNIT_X);
		}

	}   // end NAMESPACE PrebuiltCustomJoints


#endif

}   // end NAMESPACE OgreNewt


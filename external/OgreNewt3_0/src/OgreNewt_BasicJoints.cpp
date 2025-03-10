#include "OgreNewt_Stdafx.h"
#include "OgreNewt_World.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_BasicJoints.h"

#include "dCustomHinge.h"
#include "dCustomHingeActuator.h"
#include "dCustomDoubleHingeActuator.h"
#include "dCustomSlider.h"
#include "dCustomSliderActuator.h"
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
#include "dCustomDoubleHinge.h"
#include "dCustomSixDof.h"
#include "dCustomFixDistance.h"
#include "dCustomPlane.h"
#include "dCustomMotor.h"
#include "dCustomWheel.h"

#ifdef __APPLE__
#   include <Ogre/OgreLogManager.h>
#   include <Ogre/OgreStringConverter.h>
#else
#   include <OgreLogManager.h>
#   include <OgreStringConverter.h>
#endif

namespace
{
	const unsigned char findKnotSubSteps = 10;
}

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

// New command: NewtonBodySetGyroscopicTorque(box0, 1);

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
		// Has been removed by newton
		// joint->SetTwistSpringDamper(state, springDamperRelaxation, spring, damper);

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
		supportJoint = new dCustomSixdof(pinsAndPivoFrame1, pinsAndPivoFrame2, child->getNewtonBody(), parent ? parent->getNewtonBody() : nullptr);
		SetSupportJoint(supportJoint);
	}

	Joint6Dof::~Joint6Dof()
	{
		// nothing, the ~Joint() function will take care of us.
	}

	void Joint6Dof::setLinearLimits(const Ogre::Vector3& minLinearLimits, const Ogre::Vector3& maxLinearLimits) const
	{
		dCustomSixdof* joint = (dCustomSixdof*)GetSupportJoint();
		joint->SetLinearLimits(dVector(minLinearLimits.x, minLinearLimits.y, minLinearLimits.z, 1.0f), dVector(maxLinearLimits.x, maxLinearLimits.y, maxLinearLimits.z, 1.0f));
	}

	void Joint6Dof::setYawLimits(const Ogre::Degree& minLimits, const Ogre::Degree& maxLimits) const
	{
		dCustomSixdof* joint = (dCustomSixdof*)GetSupportJoint();
		joint->SetYawLimits((float)minLimits.valueRadians(), (float)maxLimits.valueRadians());
	}

	void Joint6Dof::setPitchLimits(const Ogre::Degree& minLimits, const Ogre::Degree& maxLimits) const
	{
		dCustomSixdof* joint = (dCustomSixdof*)GetSupportJoint();
		joint->SetPitchLimits((float)minLimits.valueRadians(), (float)maxLimits.valueRadians());
	}

	void Joint6Dof::setRollLimits(const Ogre::Degree& minLimits, const Ogre::Degree& maxLimits) const
	{
		dCustomSixdof* joint = (dCustomSixdof*)GetSupportJoint();
		joint->SetRollLimits((float)minLimits.valueRadians(), (float)maxLimits.valueRadians());
	}
	
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
	
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Hinge::Hinge(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos, const Ogre::Vector3& pin)
		: Joint()
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

	Hinge::Hinge(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& childPos, const Ogre::Vector3& childPin, const Ogre::Vector3& parentPos, const Ogre::Vector3& parentPin)
		: Joint()
	{
		dCustomJoint* supportJoint;

		// make the joint matrix 
		dVector childDir(childPin.x, childPin.y, childPin.z, 0.0f);
		dMatrix pinsAndPivoFrameChild(dGrammSchmidt(childDir));
		pinsAndPivoFrameChild.m_posit = dVector(childPos.x, childPos.y, childPos.z, 1.0f);

		dVector parentDir(parentPin.x, parentPin.y, parentPin.z, 0.0f);
		dMatrix pinsAndPivoFrameParent(dGrammSchmidt(parentDir));
		pinsAndPivoFrameParent.m_posit = dVector(parentPos.x, parentPos.y, parentPos.z, 1.0f);

		// crate a Newton Custom joint and set it at the support joint	
		supportJoint = new dCustomHinge(pinsAndPivoFrameChild, pinsAndPivoFrameParent, child->getNewtonBody(), parent ? parent->getNewtonBody() : nullptr);
		SetSupportJoint(supportJoint);
	}

	Hinge::~Hinge()
	{
	}


	void Hinge::EnableLimits(bool state)
	{
		dCustomHinge* joint = (dCustomHinge*)GetSupportJoint();
		joint->EnableLimits(state);

		getBody0()->unFreeze();
		if (getBody1())
			getBody1()->unFreeze();
	}

	void Hinge::SetLimits(Ogre::Degree minAngle, Ogre::Degree maxAngle)
	{
		dCustomHinge* joint = (dCustomHinge*)GetSupportJoint();

		joint->SetLimits(minAngle.valueRadians(), maxAngle.valueRadians());

		getBody0()->unFreeze();
		if (getBody1())
			getBody1()->unFreeze();
	}

	void Hinge::SetTorque(Ogre::Real torque)
	{
		getBody0()->unFreeze();
		if (getBody1())
			getBody1()->unFreeze();
		
		dCustomHinge* joint = (dCustomHinge*)GetSupportJoint();
		if (0.0f != torque)
		{
			joint->EnableMotor(true, torque);
		}
		else
		{
			joint->EnableMotor(false, torque);
		}
	}

	Ogre::Radian Hinge::GetJointAngle() const
	{
		dCustomHinge* joint = (dCustomHinge*)GetSupportJoint();
		return Ogre::Radian(joint->GetJointAngle());
	}

	Ogre::Radian Hinge::GetJointAngularVelocity() const
	{
		dCustomHinge* joint = (dCustomHinge*)GetSupportJoint();
		return Ogre::Radian(joint->GetJointOmega());
	}

	Ogre::Vector3 Hinge::GetJointPin() const
	{
		dCustomHinge* joint = (dCustomHinge*)GetSupportJoint();
		dVector pin(joint->GetPinAxis());
		return Ogre::Vector3(pin.m_x, pin.m_y, pin.m_z);
	}

	void Hinge::SetSpringAndDamping(bool enable, bool massIndependent, Ogre::Real springDamperRelaxation, Ogre::Real spring, Ogre::Real damper)
	{
		dCustomHinge* joint = (dCustomHinge*)GetSupportJoint();
		if (false == massIndependent)
		{
			joint->SetAsSpringDamper(enable, spring, damper);
		}
		else
		{
			joint->SetMassIndependentSpringDamper(enable, springDamperRelaxation, spring, damper);
		}
	}
	
	void Hinge::SetFriction(Ogre::Real friction)
	{
		dCustomHinge* joint = (dCustomHinge*)GetSupportJoint();
		joint->SetFriction(friction);
	}
	
	///////////////////////////////////////////////////////////////////////////////////////////////
	
	HingeActuator::HingeActuator(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos, const Ogre::Vector3& pin, const Ogre::Degree& angularRate, const Ogre::Degree& minAngle, const Ogre::Degree& maxAngle)
		: Joint()
	{
		dCustomJoint* supportJoint;
		// make the joint matrix 
		dVector dir(pin.x, pin.y, pin.z, 0.0f);
		dMatrix pinsAndPivoFrame(dGrammSchmidt(dir));
		pinsAndPivoFrame.m_posit = dVector(pos.x, pos.y, pos.z, 1.0f);

		// Create a Newton Custom joint and set it at the support joint	
		supportJoint = new dCustomHingeActuator(pinsAndPivoFrame, angularRate.valueRadians(), minAngle.valueRadians(), maxAngle.valueRadians(), child->getNewtonBody(), parent ? parent->getNewtonBody() : nullptr);
		SetSupportJoint(supportJoint);
	}

	HingeActuator::~HingeActuator()
	{
	}

	void HingeActuator::SetMinAngularLimit(const Ogre::Degree& limit)
	{
		dCustomHingeActuator* joint = (dCustomHingeActuator*)GetSupportJoint();
		joint->SetMinAngularLimit(limit.valueRadians());
	}

	void HingeActuator::SetMaxAngularLimit(const Ogre::Degree& limit)
	{
		dCustomHingeActuator* joint = (dCustomHingeActuator*)GetSupportJoint();
		joint->SetMaxAngularLimit(limit.valueRadians());
	}

	void HingeActuator::SetAngularRate(const Ogre::Degree& rate)
	{
		dCustomHingeActuator* joint = (dCustomHingeActuator*)GetSupportJoint();
		joint->SetAngularRate(rate.valueRadians());
	}

	void HingeActuator::SetTargetAngle(const Ogre::Degree& angle)
	{
		dCustomHingeActuator* joint = (dCustomHingeActuator*)GetSupportJoint();
		joint->SetTargetAngle(angle.valueRadians());
	}

	Ogre::Real HingeActuator::GetActuatorAngle() const
	{
		dCustomHingeActuator* joint = (dCustomHingeActuator*)GetSupportJoint();
		return Ogre::Radian(joint->GetJointAngle()).valueDegrees();
	}

	void HingeActuator::SetMaxTorque(Ogre::Real torque)
	{
		dCustomHingeActuator* joint = (dCustomHingeActuator*)GetSupportJoint();
		joint->SetMaxTorque(torque);
	}

	void HingeActuator::SetSpringAndDamping(bool enable, bool massIndependent, Ogre::Real springDamperRelaxation, Ogre::Real spring, Ogre::Real damper)
	{
		dCustomHingeActuator* joint = (dCustomHingeActuator*)GetSupportJoint();
		if (false == massIndependent)
		{
			joint->SetAsSpringDamper(enable, spring, damper);
		}
		else
		{
			joint->SetMassIndependentSpringDamper(enable, springDamperRelaxation, spring, damper);
		}
	}

	// error: max_dof will be 7 instead 6!
	/*void HingeActuator::SetFriction(Ogre::Real friction)
	{
		dCustomHingeActuator* joint = (dCustomHingeActuator*)GetSupportJoint();
		joint->SetFriction(friction);
	}*/

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

	void Slider::setFriction(Ogre::Real friction)
	{
		dCustomSlider* joint = (dCustomSlider*)GetSupportJoint();
		joint->SetFriction(friction);
	}

	void Slider::setSpringAndDamping(bool state, Ogre::Real springDamperRelaxation, Ogre::Real spring, Ogre::Real damper)
	{
		dCustomSlider* joint = (dCustomSlider*)GetSupportJoint();
		joint->SetAsSpringDamper(state, springDamperRelaxation, spring, damper);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

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
	
	///////////////////////////////////////////////////////////////////////////////////////////////
	
	SliderActuator::SliderActuator(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos, const Ogre::Vector3& pin, Ogre::Real linearRate, Ogre::Real minPosition, Ogre::Real maxPosition)
		: Joint()
	{
		dCustomJoint* supportJoint;
		// make the joint matrix 
		dVector dir(pin.x, pin.y, pin.z, 0.0f);
		dMatrix pinsAndPivoFrame(dGrammSchmidt(dir));
		pinsAndPivoFrame.m_posit = dVector(pos.x, pos.y, pos.z, 1.0f);

		// Create a Newton Custom joint and set it at the support joint	
		supportJoint = new dCustomSliderActuator(pinsAndPivoFrame, linearRate, minPosition, maxPosition, child->getNewtonBody(), parent ? parent->getNewtonBody() : nullptr);
		SetSupportJoint(supportJoint);
	}

	SliderActuator::~SliderActuator()
	{
	}
	
	Ogre::Real SliderActuator::GetActuatorPosition() const
	{
		dCustomSliderActuator* joint = (dCustomSliderActuator*)GetSupportJoint();
		return joint->GetActuatorPosit();
	}
	
	void SliderActuator::SetTargetPosition(Ogre::Real targetPosition)
	{
		dCustomSliderActuator* joint = (dCustomSliderActuator*)GetSupportJoint();
		joint->SetTargetPosit(targetPosition);
	}
	
	void SliderActuator::SetLinearRate(Ogre::Real linearRate)
	{
		dCustomSliderActuator* joint = (dCustomSliderActuator*)GetSupportJoint();
		joint->SetLinearRate(linearRate);
	}

	void SliderActuator::SetMinPositionLimit(Ogre::Real limit)
	{
		dCustomSliderActuator* joint = (dCustomSliderActuator*)GetSupportJoint();
		joint->SetMinPositLimit(limit);
	}

	void SliderActuator::SetMaxPositionLimit(Ogre::Real limit)
	{
		dCustomSliderActuator* joint = (dCustomSliderActuator*)GetSupportJoint();
		joint->SetMaxPositLimit(limit);
	}
	
	void SliderActuator::SetMinForce(Ogre::Real force)
	{
		dCustomSliderActuator* joint = (dCustomSliderActuator*)GetSupportJoint();
		joint->SetMinForce(force);
	}
	
	void SliderActuator::SetMaxForce(Ogre::Real force)
	{
		dCustomSliderActuator* joint = (dCustomSliderActuator*)GetSupportJoint();
		joint->SetMaxForce(force);
	}

	Ogre::Real SliderActuator::GetForce() const
	{
		dCustomSliderActuator* joint = (dCustomSliderActuator*)GetSupportJoint();
		return joint->GetForce();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	class dFlexyPipeHandle : public dCustomJoint
	{
	public:
		dFlexyPipeHandle(NewtonBody* const body, const dVector& pin)
			:dCustomJoint(6, body, nullptr)
		{
			m_localMatrix0 = dGrammSchmidt(pin);

			SetSolverModel(3);
			m_angularFriction = 200.0f;
			m_linearFriction = 3000.0f;
		}

		void SubmitConstraints(dFloat timestep, int threadIndex)
		{
			dMatrix matrix;
			NewtonBodyGetMatrix(m_body0, &matrix[0][0]);
			matrix = GetMatrix0() * matrix;

			for (int i = 0; i < 3; i++)
			{
				NewtonUserJointAddLinearRow(m_joint, &matrix.m_posit[0], &matrix.m_posit[0], &matrix[i][0]);
				NewtonUserJointSetRowStiffness(m_joint, m_stiffness);

				const dFloat stopAccel = NewtonUserJointCalculateRowZeroAcceleration(m_joint);
				NewtonUserJointSetRowAcceleration(m_joint, stopAccel);
				NewtonUserJointSetRowMinimumFriction(m_joint, -m_linearFriction);
				NewtonUserJointSetRowMaximumFriction(m_joint, m_linearFriction);
			}

			// because this is a velocity base dry friction, we can use a small angule appriximation 
			for (int i = 0; i < 3; i++)
			{
				NewtonUserJointAddAngularRow(m_joint, 0.0f, &matrix[i][0]);
				NewtonUserJointSetRowStiffness(m_joint, m_stiffness);

				const dFloat stopAccel = NewtonUserJointCalculateRowZeroAcceleration(m_joint);
				NewtonUserJointSetRowAcceleration(m_joint, stopAccel);
				NewtonUserJointSetRowMinimumFriction(m_joint, -m_angularFriction);
				NewtonUserJointSetRowMaximumFriction(m_joint, m_angularFriction);
			}

			// TestControls(timestep);
		}

		/*void TestControls(dFloat timestep)
		{
			NewtonWorld* const world = NewtonBodyGetWorld(m_body0);
			DemoEntityManager* const scene = (DemoEntityManager*) NewtonWorldGetUserData(world);

			dFloat linearSpeed = 2.0f * (scene->GetKeyState('O') - scene->GetKeyState('P'));
			if (linearSpeed) {
				dMatrix matrix;
				NewtonBodyGetMatrix(m_body0, &matrix[0][0]);
				matrix = GetMatrix0() * matrix;

				dVector veloc(matrix[0].Scale(linearSpeed));
				SetVelocity(veloc, timestep);
			}

			dFloat angularSpeed = 6.0f * (scene->GetKeyState('K') - scene->GetKeyState('L'));
			if (angularSpeed) {
				dMatrix matrix;
				NewtonBodyGetMatrix(m_body0, &matrix[0][0]);
				matrix = GetMatrix0() * matrix;

				dVector omega(matrix[0].Scale(angularSpeed));
				SetOmega(omega, timestep);
			}
		}*/

		void SetVelocity(const dVector& veloc, dFloat timestep) 
		{
			dVector bodyVeloc(0.0f);
			dFloat Ixx;
			dFloat Iyy;
			dFloat Izz;
			dFloat mass;

			// Get body mass
			NewtonBodyGetMass(m_body0, &mass, &Ixx, &Iyy, &Izz);

			// get body internal data
			NewtonBodyGetVelocity(m_body0, &bodyVeloc[0]);

			// calculate angular velocity error 
			dVector velocError(veloc - bodyVeloc);
			dFloat vMag2 = velocError.DotProduct3(velocError);
			if (vMag2 > dFloat (1.0e-4f))
			{
				dVector dir (velocError.Normalize());

				// calculate impulse
				dVector linearImpulse(velocError.Scale (mass) + dir.Scale (m_linearFriction * timestep));
				//dVector linearImpulse(velocError.Scale (mass));

				// apply impulse to achieve desired velocity
				dVector angularImpulse(0.0f);
				NewtonBodyApplyImpulsePair(m_body0, &linearImpulse[0], &angularImpulse[0], timestep);
			}
		}

		void SetOmega(const dVector& omega, dFloat timestep)
		{
			dMatrix bodyInertia;
			dVector bodyOmega(0.0f);

			// get body internal data
			NewtonBodyGetOmega(m_body0, &bodyOmega[0]);
			NewtonBodyGetInertiaMatrix(m_body0, &bodyInertia[0][0]);

			// calculate angular velocity error 
			dVector omegaError(omega - bodyOmega);

			dFloat wMag2 = omegaError.DotProduct3(omegaError);
			if (wMag2 > dFloat (1.0e-4f))
			{
				dVector dir (omegaError.Normalize());

				// calculate impulse
				dVector angularImpulse(bodyInertia.RotateVector(omegaError) + dir.Scale (m_angularFriction * timestep));

				// apply impulse to achieve desired omega
				dVector linearImpulse(0.0f);
				NewtonBodyApplyImpulsePair(m_body0, &linearImpulse[0], &angularImpulse[0], timestep);
			}
		}

		dFloat m_linearFriction;
		dFloat m_angularFriction;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////

	FlexyPipeHandleJoint::FlexyPipeHandleJoint(OgreNewt::Body* currentBody, const Ogre::Vector3& pin)
		: Joint()
	{
		// make the joint matrix 
		dVector dir(pin.x, pin.y, pin.z, 0.0f);
		// dMatrix pinsAndPivoFrame(dGrammSchmidt(dir));
		// pinsAndPivoFrame.m_posit = dVector(anchorPosition.x, anchorPosition.y, anchorPosition.z, 1.0f);

		// Create a Newton Custom joint and set it at the support joint	
		dFlexyPipeHandle* supportJoint = new dFlexyPipeHandle(currentBody->getNewtonBody(), dir);
		SetSupportJoint(supportJoint);
	}

	void FlexyPipeHandleJoint::setVelocity(const Ogre::Vector3& velocity, Ogre::Real dt)
	{
		dFlexyPipeHandle* joint = (dFlexyPipeHandle*)GetSupportJoint();
		joint->SetVelocity(dVector(velocity.x, velocity.y, velocity.z, 1.0f), dt);
	}

	void FlexyPipeHandleJoint::setOmega(const Ogre::Vector3& omega, Ogre::Real dt)
	{
		dFlexyPipeHandle* joint = (dFlexyPipeHandle*)GetSupportJoint();
		joint->SetOmega(dVector(omega.x, omega.y, omega.z, 1.0f), dt);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	class dFlexyPipeSpinner: public dCustomBallAndSocket
	{
	public: 
		dFlexyPipeSpinner (const dMatrix& pinAndPivotFrame, NewtonBody* const child, NewtonBody* const parent)
			: dCustomBallAndSocket(pinAndPivotFrame, child, parent)
		{
			EnableTwist(false);
			EnableCone(false);
		}

		void ApplyTwistAction(const dMatrix& matrix0, const dMatrix& matrix1, dFloat timestep)
		{
			dFloat jacobian0[6];
			dFloat jacobian1[6];

			dVector pin0(matrix0.m_front);
			dVector pin1(matrix1.m_front.Scale(-1.0f));

			jacobian0[0] = 0.0f;
			jacobian0[1] = 0.0f;
			jacobian0[2] = 0.0f;
			jacobian0[3] = pin0.m_x;
			jacobian0[4] = pin0.m_y;
			jacobian0[5] = pin0.m_z;

			jacobian1[0] = 0.0f;
			jacobian1[1] = 0.0f;
			jacobian1[2] = 0.0f;
			jacobian1[3] = pin1.m_x;
			jacobian1[4] = pin1.m_y;
			jacobian1[5] = pin1.m_z;

			dVector omega0(0.0f);
			dVector omega1(0.0f);
			NewtonBodyGetOmega(m_body0, &omega0[0]);
			NewtonBodyGetOmega(m_body1, &omega1[0]);

			dFloat relOmega = omega0.DotProduct3(pin0) + omega1.DotProduct3(pin1);
			dFloat relAccel = -relOmega / timestep;
			NewtonUserJointAddGeneralRow(m_joint, jacobian0, jacobian1);
			NewtonUserJointSetRowAcceleration(m_joint, relAccel);
		}

		void ApplyElasticConeAction(const dMatrix& matrix0, const dMatrix& matrix1, dFloat timestep)
		{
			dFloat relaxation = 0.01f;
			dFloat spring = 1000.0f;
			dFloat damper = 50.0f;
			dFloat maxConeAngle = 45.0f * dDegreeToRad;

			dFloat cosAngleCos = matrix1.m_front.DotProduct3(matrix0.m_front);
			if (cosAngleCos >= dFloat(0.998f))
			{
				// when angle is very small we use Cartesian approximation 
				dFloat angle0 = CalculateAngle(matrix0.m_front, matrix1.m_front, matrix1.m_up);
				NewtonUserJointAddAngularRow(m_joint, angle0, &matrix1.m_up[0]);
				NewtonUserJointSetRowMassIndependentSpringDamperAcceleration(m_joint, relaxation, spring, damper);
			
				dFloat angle1 = CalculateAngle(matrix0.m_front, matrix1.m_front, matrix1.m_right);
				NewtonUserJointAddAngularRow(m_joint, angle1, &matrix1.m_right[0]);
				NewtonUserJointSetRowMassIndependentSpringDamperAcceleration(m_joint, relaxation, spring, damper);
			}
			else
			{
				// angle is large enough that we calculable the actual cone angle
				dVector lateralDir(matrix1[0].CrossProduct(matrix0[0]));
				dAssert(lateralDir.DotProduct3(lateralDir) > 1.0e-6f);
				lateralDir = lateralDir.Normalize();
				dFloat coneAngle = dAcos(dClamp(matrix1.m_front.DotProduct3(matrix0.m_front), dFloat(-1.0f), dFloat(1.0f)));
				dMatrix coneRotation(dQuaternion(lateralDir, coneAngle), matrix1.m_posit);
			
				// flexible spring action happens alone the cone angle only
				if (coneAngle > maxConeAngle)
				{
					NewtonUserJointAddAngularRow(m_joint, maxConeAngle - coneAngle, &lateralDir[0]);
					NewtonUserJointSetRowMaximumFriction(m_joint, 0.0f);

				}
				else
				{
					// apply spring damper action
					NewtonUserJointAddAngularRow(m_joint, -coneAngle, &lateralDir[0]);
					NewtonUserJointSetRowMassIndependentSpringDamperAcceleration(m_joint, relaxation, spring, damper);	
				}
			}
		}

		void SubmitConstraints(dFloat timestep, int threadIndex)
		{
			dMatrix matrix0;
			dMatrix matrix1;
			dCustomBallAndSocket::SubmitConstraints(timestep, threadIndex);

			CalculateGlobalMatrix(matrix0, matrix1);
			ApplyTwistAction (matrix0, matrix1, timestep);
			ApplyElasticConeAction(matrix0, matrix1, timestep);
		}
	};

	///////////////////////////////////////////////////////////////////////////////////////////////

	FlexyPipeSpinnerJoint::FlexyPipeSpinnerJoint(OgreNewt::Body* currentBody, OgreNewt::Body* predecessorBody, const Ogre::Vector3& anchorPosition, const Ogre::Vector3& pin)
		: Joint()
	{
		// make the joint matrix 
		dVector dir(pin.x, pin.y, pin.z, 0.0f);
		dMatrix pinsAndPivoFrame(dGrammSchmidt(dir));
		pinsAndPivoFrame.m_posit = dVector(anchorPosition.x, anchorPosition.y, anchorPosition.z, 1.0f);

		// Create a Newton Custom joint and set it at the support joint	
		dFlexyPipeSpinner* supportJoint = new dFlexyPipeSpinner(pinsAndPivoFrame, currentBody->getNewtonBody(), predecessorBody ? predecessorBody->getNewtonBody() : nullptr);
		SetSupportJoint(supportJoint);
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
				if (this->progress >= this->maxStopDistance)
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
				if (this->progress <= this->minStopDistance)
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
				round = 0;
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
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ActiveSliderJoint::ActiveSliderJoint(OgreNewt::Body* currentBody, OgreNewt::Body* predecessorBody, const Ogre::Vector3& anchorPosition, const Ogre::Vector3& pin,
		Ogre::Real minStopDistance, Ogre::Real maxStopDistance, Ogre::Real moveSpeed, bool repeat, bool directionChange, bool activated)
		: Joint()
	{
		
		// make the joint matrix 
		dVector dir(pin.x, pin.y, pin.z, 0.0f);
		dMatrix pinsAndPivoFrame(dGrammSchmidt(dir));
		pinsAndPivoFrame.m_posit = dVector(anchorPosition.x, anchorPosition.y, anchorPosition.z, 1.0f);

		// Create a Newton Custom joint and set it at the support joint	
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

	/////////////////////////////////////////////////////////////////////////////////////////

	//class Plane2dUpVector : public dCustomPlane
	//{
	//public:
	//	Plane2dUpVector(const dVector& pivot, const dVector& normal, const dVector& pin, NewtonBody* const child)
	//		: dCustomPlane(pivot, normal, child),
	//		m_normal(normal)
	//	{
	//		
	//		dMatrix pinAndPivotFrame(dGrammSchmidt(pin));
	//		pinAndPivotFrame.m_posit = pivot;
	//		pinAndPivotFrame.m_posit.m_w = 1.0f;
	//		// calculate the two local matrix of the pivot point
	//		CalculateLocalMatrix(pinAndPivotFrame, m_localMatrix0, m_localMatrix1);
	//	}

	//	virtual ~Plane2dUpVector()
	//	{
	//	}

	//	void SubmitConstraints(dFloat timestep, int threadIndex)
	//	{
	//		dMatrix matrix0;
	//		dMatrix matrix1;

	//		// add one row to prevent body from rotating on the plane of motion
	//		CalculateGlobalMatrix(matrix0, matrix1);

	//		dFloat pitchAngle = CalculateAngle(matrix0.m_up, matrix1.m_up, matrix1.m_front);
	//		NewtonUserJointAddAngularRow(m_joint, pitchAngle, &matrix1.m_front[0]);

	//		// Restrict the movement on the pivot point along all two orthonormal axis direction perpendicular to the motion
	//		const dVector& dir = m_normal;
	//		const dVector& p0 = matrix0.m_posit;
	//		const dVector& p1 = matrix1.m_posit;
	//		NewtonUserJointAddLinearRow(m_joint, &p0[0], &p1[0], &dir[0]);

	//		const dFloat invTimeStep = 1.0f / timestep;
	//		const dFloat dist = 0.25f * dir.DotProduct3(p1 - p0);
	//		const dFloat accel = NewtonUserJointCalculateRowZeroAcceleration(m_joint) + dist * invTimeStep * invTimeStep;
	//		NewtonUserJointSetRowAcceleration(m_joint, accel);
	//		NewtonUserJointSetRowStiffness(m_joint, m_stiffness);

	//		// construct an orthogonal coordinate system with these two vectors
	//		NewtonUserJointAddAngularRow(m_joint, CalculateAngle(matrix0.m_front, matrix1.m_front, matrix1.m_up), &matrix1.m_up[0]);
	//		NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
	//		NewtonUserJointAddAngularRow(m_joint, CalculateAngle(matrix0.m_front, matrix1.m_front, matrix1.m_right), &matrix1.m_right[0]);
	//		NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
	//	}
	//private:
	//	dVector m_normal;
	//};

	class Plane2dUpVector : public dCustomPlane
	{
	public:
		Plane2dUpVector(const dVector& pivot, const dVector& normal, const dVector& pin, NewtonBody* const child)
			: dCustomPlane(pivot, normal, child),
			m_pin(pin)
		{
			EnableControlRotation(false);
		}

		virtual ~Plane2dUpVector()
		{
		}

		virtual void SubmitConstraints(dFloat timestep, int threadIndex) override
		{
			dMatrix matrix0;
			dMatrix matrix1;

			// add rows to fix matrix body the the plane
			dCustomPlane::SubmitConstraints(timestep, threadIndex);

			// add one row to prevent body from rotating on the plane of motion
			CalculateGlobalMatrix(matrix0, matrix1);

			// m_right = x
			// m_front = z
			// m_up = y

			if (m_pin.m_y == 1.0f)
			{
				NewtonUserJointAddAngularRow(m_joint, CalculateAngle(matrix0.m_up, matrix1.m_up, matrix1.m_front), &matrix1.m_front[0]);
				NewtonUserJointAddAngularRow(m_joint, CalculateAngle(matrix0.m_up, matrix1.m_up, matrix1.m_front), &matrix1.m_right[0]);
			}
			else if (m_pin.m_x == 1.0f)
			{
				NewtonUserJointAddAngularRow(m_joint, CalculateAngle(matrix0.m_right, matrix1.m_right, matrix1.m_up), &matrix1.m_up[0]);
				NewtonUserJointAddAngularRow(m_joint, CalculateAngle(matrix0.m_right, matrix1.m_right, matrix1.m_up), &matrix1.m_front[0]);
			}
			else if (m_pin.m_z == 1.0f)
			{
				NewtonUserJointAddAngularRow(m_joint, CalculateAngle(matrix0.m_front, matrix1.m_front, matrix1.m_right), &matrix1.m_right[0]);
				NewtonUserJointAddAngularRow(m_joint, CalculateAngle(matrix0.m_front, matrix1.m_front, matrix1.m_right), &matrix1.m_up[0]);
			}
		}

		void setPin(const Ogre::Vector3& pin)
		{
			m_pin = dVector(pin.x, pin.y, pin.z);
		}
	private:
		dVector m_pin;
	};

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Plane2DUpVectorJoint::Plane2DUpVectorJoint(const OgreNewt::Body* child, const Ogre::Vector3& pos, const Ogre::Vector3& normal, const Ogre::Vector3& pin)
		: Joint()
	{
		dVector dPos(pos.x, pos.y, pos.z, 1.0f);
		dVector dNormal(normal.x, normal.y, normal.z, 1.0f);
		dVector dPin(pin.x, pin.y, pin.z, 1.0f);
		Plane2dUpVector* supportJoint = new Plane2dUpVector(dPos, dNormal, dPin, child->getNewtonBody());
		SetSupportJoint(supportJoint);
	}

	void Plane2DUpVectorJoint::setPin(const Ogre::Vector3& pin)
	{
		Plane2dUpVector* supportJoint = static_cast<Plane2dUpVector*>(m_joint);
		supportJoint->setPin(pin);
	}

	/////////////////////////////////////////////////////////////////////////////////////////

	class CustomPlane : public dCustomJoint
	{
	public:
		CustomPlane(const dVector& pivot, const dVector& normal, NewtonBody* const child, NewtonBody* const parent)
			: dCustomJoint(2, child, parent)
		{
			dMatrix pinAndPivotFrame(dGrammSchmidt(normal));
			pinAndPivotFrame.m_posit = pivot;
			pinAndPivotFrame.m_posit.m_w = 1.0f;
			// calculate the two local matrix of the pivot point
			CalculateLocalMatrix(pinAndPivotFrame, m_localMatrix0, m_localMatrix1);
		}

		virtual ~CustomPlane()
		{
		}

		virtual void SubmitConstraints(dFloat timestep, int threadIndex) override
		{
			dMatrix matrix0;
			dMatrix matrix1;

			// calculate the position of the pivot point and the Jacobian direction vectors, in global space. 
			CalculateGlobalMatrix(matrix0, matrix1);

			// Restrict the movement on the pivot point along all two orthonormal axis direction perpendicular to the motion
			const dVector& dir = matrix1[0];
			const dVector& p0 = matrix0.m_posit;
			const dVector& p1 = matrix1.m_posit;
			NewtonUserJointAddLinearRow(m_joint, &p0[0], &p1[0], &dir[0]);

			const dFloat invTimeStep = 1.0f / timestep;
			const dFloat dist = 0.25f * dir.DotProduct3(p1 - p0);
			const dFloat accel = NewtonUserJointCalculateRowZeroAcceleration(m_joint) + dist * invTimeStep * invTimeStep;
			NewtonUserJointSetRowAcceleration(m_joint, accel);
			NewtonUserJointSetRowStiffness(m_joint, m_stiffness);
		}
	};

	PlaneConstraint::PlaneConstraint(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos, const Ogre::Vector3& normal)
		: Joint()
	{
		dVector dPos(pos.x, pos.y, pos.z, 1.0f);
		dVector dNormal(normal.x, normal.y, normal.z, 1.0f);
		CustomPlane* supportJoint = new CustomPlane(dPos, dNormal, child->getNewtonBody(), parent ? parent->getNewtonBody() : nullptr);
		SetSupportJoint(supportJoint);
	}

	PlaneConstraint::~PlaneConstraint()
	{
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
		/*
		KinematricController idea:
		You can get the control to work with only two controller.
		the first is the gripper just the same way you are doing,
		the second is by using the dCustomKinematicController.you connect the base of the gripper to an dCustomKinematicController 
		and then by setting the point and the orientation the joint will moves the whole thing to the target position, just like real robot do. 
		The way is configure now is for forward dynamics, the way I say will be invers dynamics.
		then by setting good friction for the intermediate joint it sod be quit cool.
		the nice part is that after you have set up, them you cam have the kinematic joint following a spline path and have the robot doing some industrial repetitive task.
		I will probably do the same for there robot demos in the SDK
		
		*/

		dCustomJoint* supportJoint;

		// make the joint matrix 
		dVector attachement(pos.x, pos.y, pos.z, 1.0f);

		// crate a Newton Custom joint and set it at the support joint	
		supportJoint = new dCustomKinematicController(child->getNewtonBody(), attachement);
		SetSupportJoint(supportJoint);

		((dCustomKinematicController*)supportJoint)->ResetAutoSleep();

		// ((dCustomKinematicController*)supportJoint)->SetSolverModel(1);
	}

	KinematicController::KinematicController(const OgreNewt::Body* child, const Ogre::Vector3& pos, const OgreNewt::Body* referenceBody)
	{
		dCustomJoint* supportJoint;

		// make the joint matrix 
		dVector attachement(pos.x, pos.y, pos.z, 1.0f);

		// crate a Newton Custom joint and set it at the support joint	
		supportJoint = new dCustomKinematicController(child->getNewtonBody(), attachement, referenceBody ? referenceBody->getNewtonBody() : nullptr);
		SetSupportJoint(supportJoint);

		((dCustomKinematicController*)supportJoint)->ResetAutoSleep();
	}

	KinematicController::~KinematicController()
	{
	}

	void KinematicController::setPickingMode(int mode)
	{
		dCustomKinematicController* joint = (dCustomKinematicController*)GetSupportJoint();

		joint->SetControlMode(static_cast<dCustomKinematicController::dControlModes>(mode));
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

	void KinematicController::setMaxSpeed(Ogre::Real speedInMetersPerSeconds)
	{
		dCustomKinematicController* joint = (dCustomKinematicController*)GetSupportJoint();
		joint->SetMaxSpeed(speedInMetersPerSeconds);
	}

	void KinematicController::setMaxOmega(const Ogre::Degree& speedInDegreesPerSeconds)
	{
		dCustomKinematicController* joint = (dCustomKinematicController*)GetSupportJoint();
		joint->SetMaxOmega((float)speedInDegreesPerSeconds.valueRadians());
	}

	void KinematicController::setAngularViscousFrictionCoefficient(Ogre::Real coefficient)
	{
		dCustomKinematicController* joint = (dCustomKinematicController*)GetSupportJoint();
		joint->SetAngularViscuosFrictionCoefficient(coefficient);
	}

	void KinematicController::resetAutoSleep(void)
	{
		dCustomKinematicController* joint = (dCustomKinematicController*)GetSupportJoint();
		joint->ResetAutoSleep();
	}

	void KinematicController::setSolverModel(unsigned short solverModel)
	{
		dCustomKinematicController* joint = (dCustomKinematicController*)GetSupportJoint();
		joint->SetSolverModel(solverModel);
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

		virtual ~CustomPathFollow()
		{
		}

		virtual void GetPointAndTangentAtLocation(const dVector& location, dVector& positOut, dVector& tangentOut) const override
		{
			Body* const pathBody = (Body*)NewtonBodyGetUserData(GetBody1());
			const dBezierSpline& spline = pathBody->getSpline();

			dMatrix matrix;
			NewtonBodyGetMatrix(GetBody1(), &matrix[0][0]);

			dVector p(matrix.UntransformVector(location));
			dBigVector point;
			// 4 steps are not enough, setting to 8!
			dFloat64 knot = spline.FindClosestKnot(point, p, findKnotSubSteps);

			/*if (knot > 0.95)
			{
				knot = 0.0;
			}*/

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
		// NewtonBodySetMassMatrix(m_pathBody->getNewtonBody(), 0.0f, 0.0f, 0.0f, 0.0f);
	}

	PathFollow::~PathFollow()
	{
		// nothing, the ~Joint() function will take care of us.
	}

	bool PathFollow::setKnots(const std::vector<Ogre::Vector3>& knots)
	{
		if (knots.size() <= 1)
		{
			return false;
		}

		// create a Bezier Spline path
		dBezierSpline spline;

		std::vector<dFloat64> internalKnots(knots.size() - 1);
		for (size_t i = 0; i < internalKnots.size(); i++)
		{
			internalKnots[i] = static_cast<dFloat64>(i) / static_cast<dFloat64>(internalKnots.size() - 1);
		}

		m_internalControlPoints.resize(knots.size() + 1);

		for (size_t i = 0; i < knots.size(); i++)
		{
			m_internalControlPoints[i] = dBigVector(knots[i].x, knots[i].y, knots[i].z);
		}

		m_internalControlPoints[knots.size()] = m_internalControlPoints[0];

		/*
		A Bézier curve is defined by a set of control points P0 through Pn, where n is called the order of the curve (n = 1 for linear, 2 for quadratic, 3 for cubic, etc.). 
		The first and last control points are always the endpoints of the curve; however, the intermediate control points generally do not lie on the curve. 

		As the algorithm is recursive, we can build Bezier curves of any order, that is: using 5, 6 or more control points. But in practice many points are less useful. Usually we take 2-3 points, and for complex lines glue several curves together.
		* */

		spline.CreateFromKnotVectorAndControlPoints(3, internalKnots.size(), &internalKnots[0], &m_internalControlPoints[0]);

		m_pathBody->setSpline(spline);

		return true;
	}

	void PathFollow::createPathJoint(void)
	{
		dCustomJoint* supportJoint;

		dMatrix pathBodyMatrix;
		NewtonBodyGetMatrix(m_pathBody->getNewtonBody(), &pathBodyMatrix[0][0]);

		// make the joint matrix
		const Ogre::Vector3 dir = this->getMoveDirection(0);
		dVector newtonDir = dVector(dir.x, dir.y, dir.z);

		dMatrix pinsAndPivoFrame(dGrammSchmidt(newtonDir));
		// pinsAndPivoFrame.m_posit = dVector(m_pos.x, m_pos.y, m_pos.z, 1.0f);
		pinsAndPivoFrame.m_posit = pathBodyMatrix.TransformVector(dVector(m_pos.x, m_pos.y, m_pos.z, 1.0));

#if 0
		dMatrix matrix;
		matrix.m_front = newtonDir;
		matrix.m_right = matrix.m_front.CrossProduct(matrix.m_up);
		matrix.m_right = matrix.m_right.Scale(1.0f / dSqrt(matrix.m_right.DotProduct3(matrix.m_right)));
		matrix.m_up = matrix.m_right.CrossProduct(matrix.m_front);
		matrix.m_posit = pathBodyMatrix.TransformVector(dVector(m_pos.x, m_pos.y, m_pos.z, 1.0));
#endif

		// crate a Newton Custom joint and set it at the support joint	
		supportJoint = new CustomPathFollow(pinsAndPivoFrame, m_childBody->getNewtonBody(), m_pathBody->getNewtonBody());
		SetSupportJoint(supportJoint);
	}

	Ogre::Vector3 PathFollow::getCurrentMoveDirection(const Ogre::Vector3& currentBodyPosition)
	{
		dBigVector point0;
		const auto& spline = m_pathBody->getSpline();
		Ogre::Real knot = static_cast<Ogre::Real>(spline.FindClosestKnot(point0, dBigVector(dVector(currentBodyPosition.x, currentBodyPosition.y, currentBodyPosition.z, 0.0f)), findKnotSubSteps));

		dBigVector point1;
		dBigVector tangent(spline.CurveDerivative(knot));
		tangent = tangent.Scale(1.0 / dSqrt(tangent.DotProduct3(tangent)));
		knot = spline.FindClosestKnot(point1, dBigVector(point0 + tangent.Scale(2.0f)), findKnotSubSteps);

		dBigVector dir = point1 - point0;
		dir = dir.Normalize();

		return Ogre::Vector3(dir.m_x, dir.m_y, dir.m_z);
	}

	Ogre::Vector3 PathFollow::getMoveDirection(unsigned int index)
	{
		if (index >= m_internalControlPoints.size())
		{
			return Ogre::Vector3::ZERO;
		}

		dBigVector point0;
		const auto& spline = m_pathBody->getSpline();
		double knot = spline.FindClosestKnot(point0, dBigVector(m_internalControlPoints[index].m_x, m_internalControlPoints[index].m_y, m_internalControlPoints[index].m_z), findKnotSubSteps);

		dBigVector point1;
		dBigVector tangent(spline.CurveDerivative(knot));
		tangent = tangent.Scale(1.0 / dSqrt(tangent.DotProduct3(tangent)));
		knot = spline.FindClosestKnot(point1, dBigVector(point0 + tangent.Scale(2.0f)), findKnotSubSteps);

		dBigVector dir = point1 - point0;
		dir = dir.Normalize();

		return Ogre::Vector3(dir.m_x, dir.m_y, dir.m_z);
	}

	Ogre::Real PathFollow::getPathLength(void)
	{
		const auto& spline = m_pathBody->getSpline();
		return spline.CalculateLength(0.1);
	}

	std::pair<Ogre::Real, Ogre::Vector3> PathFollow::getPathProgressAndPosition(const Ogre::Vector3& currentBodyPosition)
	{
		dBigVector point0;
		const auto& spline = m_pathBody->getSpline();
		Ogre::Real knot = static_cast<Ogre::Real>(spline.FindClosestKnot(point0, dBigVector(dVector(currentBodyPosition.x, currentBodyPosition.y, currentBodyPosition.z, 0.0f)), findKnotSubSteps));
		Ogre::Vector3 ogrePoint = Ogre::Vector3(point0.m_x, point0.m_y, point0.m_z);

		return std::make_pair(knot, ogrePoint);
	}

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

	Universal::Universal(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos, const Ogre::Vector3& pin)
		: Joint()
	{
		dCustomJoint* supportJoint;

		// make the joint matrix 
		dVector dir(pin.x, pin.y, pin.z, 0.0f);
		dMatrix pinsAndPivoFrame(Ogre::Vector3::ZERO != pin ? dGrammSchmidt(dir) : dGetIdentityMatrix());
		pinsAndPivoFrame.m_posit = dVector(pos.x, pos.y, pos.z, 1.0f);

		// Create a Newton Custom joint and set it at the support joint	
		supportJoint = new dCustomDoubleHinge(pinsAndPivoFrame, child->getNewtonBody(), parent ? parent->getNewtonBody() : nullptr);
		SetSupportJoint(supportJoint);
	}

	Universal::Universal(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& childPos, const Ogre::Vector3& childPin, const Ogre::Vector3& parentPos, const Ogre::Vector3& parentPin)
		: Joint()
	{
		dCustomJoint* supportJoint;

		// make the joint matrix 
		dVector childDir(childPin.x, childPin.y, childPin.z, 0.0f);
		dMatrix pinsAndPivoFrameChild(dGrammSchmidt(childDir));
		pinsAndPivoFrameChild.m_posit = dVector(childPos.x, childPos.y, childPos.z, 1.0f);

		dVector parentDir(parentPin.x, parentPin.y, parentPin.z, 0.0f);
		dMatrix pinsAndPivoFrameParent(dGrammSchmidt(parentDir));
		pinsAndPivoFrameParent.m_posit = dVector(parentPos.x, parentPos.y, parentPos.z, 1.0f);

		// crate a Newton Custom joint and set it at the support joint	
		supportJoint = new dCustomDoubleHinge(pinsAndPivoFrameChild, pinsAndPivoFrameParent, child->getNewtonBody(), parent ? parent->getNewtonBody() : nullptr);
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
		// joint->SetAsSpringDamper(state, springDamperRelaxation, spring, damper);
		joint->SetMassIndependentSpringDamper(state, springDamperRelaxation, spring, damper);
	}

	void Universal::setAsSpringDamper1(bool state, Ogre::Real springDamperRelaxation, Ogre::Real spring, Ogre::Real damper)
	{
		dCustomDoubleHinge* joint = (dCustomDoubleHinge*)GetSupportJoint();
		// joint->SetAsSpringDamper1(state, springDamperRelaxation, spring, damper);
		joint->SetMassIndependentSpringDamper1(state, springDamperRelaxation, spring, damper);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	
	UniversalActuator::UniversalActuator(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos, 
						const Ogre::Degree& angularRate0, const Ogre::Degree& minAngle0, const Ogre::Degree& maxAngle0,
						const Ogre::Degree& angularRate1, const Ogre::Degree& minAngle1, const Ogre::Degree& maxAngle1)
		: Joint()
	{
		dCustomJoint* supportJoint;
		// make the joint matrix 
		dMatrix pinsAndPivoFrame(dGetIdentityMatrix());
		pinsAndPivoFrame.m_posit = dVector(pos.x, pos.y, pos.z, 1.0f);

		// Create a Newton Custom joint and set it at the support joint	
		supportJoint = new dCustomDoubleHingeActuator(pinsAndPivoFrame, angularRate0.valueRadians(), minAngle0.valueRadians(), maxAngle0.valueRadians(), 
														angularRate1.valueRadians(), minAngle1.valueRadians(), maxAngle1.valueRadians(),
														child->getNewtonBody(), parent ? parent->getNewtonBody() : nullptr);
		SetSupportJoint(supportJoint);
	}

	UniversalActuator::~UniversalActuator()
	{
	}

	void UniversalActuator::SetMinAngularLimit0(const Ogre::Degree& limit0)
	{
		dCustomDoubleHingeActuator* joint = (dCustomDoubleHingeActuator*)GetSupportJoint();
		joint->SetMinAngularLimit0(limit0.valueRadians());
	}

	void UniversalActuator::SetMaxAngularLimit0(const Ogre::Degree& limit0)
	{
		dCustomDoubleHingeActuator* joint = (dCustomDoubleHingeActuator*)GetSupportJoint();
		joint->SetMaxAngularLimit0(limit0.valueRadians());
	}

	void UniversalActuator::SetAngularRate0(const Ogre::Degree& rate0)
	{
		dCustomDoubleHingeActuator* joint = (dCustomDoubleHingeActuator*)GetSupportJoint();
		joint->SetAngularRate0(rate0.valueRadians());
	}

	void UniversalActuator::SetTargetAngle0(const Ogre::Degree& angle0)
	{
		dCustomDoubleHingeActuator* joint = (dCustomDoubleHingeActuator*)GetSupportJoint();
		joint->SetTargetAngle0(angle0.valueRadians());
	}

	Ogre::Real UniversalActuator::GetActuatorAngle0() const
	{
		dCustomDoubleHingeActuator* joint = (dCustomDoubleHingeActuator*)GetSupportJoint();
		return Ogre::Radian(joint->GetActuatorAngle0()).valueDegrees();
	}

	void UniversalActuator::SetMaxTorque0(Ogre::Real torque0)
	{
		dCustomDoubleHingeActuator* joint = (dCustomDoubleHingeActuator*)GetSupportJoint();
		joint->SetMaxTorque0(torque0);
	}
	
	void UniversalActuator::SetMinAngularLimit1(const Ogre::Degree& limit1)
	{
		dCustomDoubleHingeActuator* joint = (dCustomDoubleHingeActuator*)GetSupportJoint();
		joint->SetMinAngularLimit1(limit1.valueRadians());
	}

	void UniversalActuator::SetMaxAngularLimit1(const Ogre::Degree& limit1)
	{
		dCustomDoubleHingeActuator* joint = (dCustomDoubleHingeActuator*)GetSupportJoint();
		joint->SetMaxAngularLimit1(limit1.valueRadians());
	}

	void UniversalActuator::SetAngularRate1(const Ogre::Degree& rate1)
	{
		dCustomDoubleHingeActuator* joint = (dCustomDoubleHingeActuator*)GetSupportJoint();
		joint->SetAngularRate1(rate1.valueRadians());
	}

	void UniversalActuator::SetTargetAngle1(const Ogre::Degree& angle1)
	{
		dCustomDoubleHingeActuator* joint = (dCustomDoubleHingeActuator*)GetSupportJoint();
		joint->SetTargetAngle1(angle1.valueRadians());
	}

	Ogre::Real UniversalActuator::GetActuatorAngle1() const
	{
		dCustomDoubleHingeActuator* joint = (dCustomDoubleHingeActuator*)GetSupportJoint();
		return Ogre::Radian(joint->GetActuatorAngle1()).valueDegrees();
	}

	void UniversalActuator::SetMaxTorque1(Ogre::Real torque1)
	{
		dCustomDoubleHingeActuator* joint = (dCustomDoubleHingeActuator*)GetSupportJoint();
		joint->SetMaxTorque1(torque1);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	Motor::Motor(const OgreNewt::Body* child, const OgreNewt::Body* parent0, const OgreNewt::Body* parent1, const Ogre::Vector3& pin0, const Ogre::Vector3& pin1)
	: Joint(),
	  m_isMotor2(false)
	{
		dCustomJoint* supportJoint;

		// make the joint matrix 
		dVector dir0(pin0.x, pin0.y, pin0.z, 0.0f);
		dVector dir1(pin1.x, pin1.y, pin1.z, 0.0f);

		// create a Newton Custom joint and set it at the support joint	
		if (nullptr != parent1)
		{
			supportJoint = new dCustomMotor2(dir0, dir1, child->getNewtonBody(), parent0->getNewtonBody(), parent1->getNewtonBody());
			m_isMotor2 = true;
		}
		else
		{
			supportJoint = new dCustomMotor(dir0, child->getNewtonBody(), parent0->getNewtonBody());
		}

		SetSupportJoint(supportJoint);
	}

	Motor::~Motor()
	{
	}

	Ogre::Real Motor::GetSpeed0() const
	{
		if (false == m_isMotor2)
		{
			dCustomMotor* joint = (dCustomMotor*)GetSupportJoint();
			return joint->GetSpeed();
		}
		else
		{
			dCustomMotor2* joint = (dCustomMotor2*)GetSupportJoint();
			return joint->GetSpeed();
		}
	}

	Ogre::Real Motor::GetSpeed1() const
	{
		if (true == m_isMotor2)
		{
			dCustomMotor2* joint = (dCustomMotor2*)GetSupportJoint();
			return joint->GetSpeed1();
		}
		else
		{
			return 0.0f;
		}
	}

	void Motor::SetSpeed0(Ogre::Real speed0)
	{
		if (false == m_isMotor2)
		{
			dCustomMotor* joint = (dCustomMotor*)GetSupportJoint();
			joint->SetSpeed(speed0);
		}
		else
		{
			dCustomMotor2* joint = (dCustomMotor2*)GetSupportJoint();
			joint->SetSpeed(speed0);
		}
	}

	void Motor::SetSpeed1(Ogre::Real speed1)
	{
		if (true == m_isMotor2)
		{
			dCustomMotor2* joint = (dCustomMotor2*)GetSupportJoint();
			joint->SetSpeed1(speed1);
		}
	}

	void Motor::SetTorque0(Ogre::Real torque0)
	{
		if (false == m_isMotor2)
		{
			dCustomMotor* joint = (dCustomMotor*)GetSupportJoint();
			joint->SetTorque(torque0);
		}
		else
		{
			dCustomMotor2* joint = (dCustomMotor2*)GetSupportJoint();
			joint->SetTorque(torque0);
		}
	}

	void Motor::SetTorque1(Ogre::Real torque1)
	{
		if (true == m_isMotor2)
		{
			dCustomMotor2* joint = (dCustomMotor2*)GetSupportJoint();
			joint->SetTorque1(torque1);
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	Wheel::Wheel(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos, const Ogre::Vector3& pin)
		: Joint()
	{
		dCustomJoint* supportJoint;

		// make the joint matrix 
		dVector dir(pin.x, pin.y, pin.z, 0.0f);
		dMatrix pinsAndPivoFrame(dGrammSchmidt(dir));
		pinsAndPivoFrame.m_posit = dVector(pos.x, pos.y, pos.z, 1.0f);

		// crate a Newton Custom joint and set it at the support joint	
		supportJoint = new dCustomWheel(pinsAndPivoFrame, child->getNewtonBody(), parent ? parent->getNewtonBody() : nullptr);
		SetSupportJoint(supportJoint);
	}

	Wheel::Wheel(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& childPos, const Ogre::Vector3& childPin, const Ogre::Vector3& parentPos, const Ogre::Vector3& parentPin)
		: Joint()
	{
		dCustomJoint* supportJoint;

		// make the joint matrix 
		dVector childDir(childPin.x, childPin.y, childPin.z, 0.0f);
		dMatrix pinsAndPivoFrameChild(dGrammSchmidt(childDir));
		pinsAndPivoFrameChild.m_posit = dVector(childPos.x, childPos.y, childPos.z, 1.0f);

		dVector parentDir(parentPin.x, parentPin.y, parentPin.z, 0.0f);
		dMatrix pinsAndPivoFrameParent(dGrammSchmidt(parentDir));
		pinsAndPivoFrameParent.m_posit = dVector(parentPos.x, parentPos.y, parentPos.z, 1.0f);

		// crate a Newton Custom joint and set it at the support joint	
		supportJoint = new dCustomWheel(pinsAndPivoFrameChild, pinsAndPivoFrameParent, child->getNewtonBody(), parent ? parent->getNewtonBody() : nullptr);
		SetSupportJoint(supportJoint);
	}

	Wheel::~Wheel()
	{

	}

	Ogre::Real Wheel::GetSteerAngle() const
	{
		dCustomWheel* joint = (dCustomWheel*)GetSupportJoint();
		return joint->GetSteerAngle();
	}

	void Wheel::SetTargetSteerAngle(const Ogre::Degree& targetAngle)
	{
		dCustomWheel* joint = (dCustomWheel*)GetSupportJoint();
		joint->SetTargetSteerAngle(targetAngle.valueRadians());
	}

	Ogre::Real Wheel::GetTargetSteerAngle() const
	{
		dCustomWheel* joint = (dCustomWheel*)GetSupportJoint();
		return joint->GetTargetSteerAngle();
	}

	void Wheel::SetSteerRate(Ogre::Real steerRate)
	{
		dCustomWheel* joint = (dCustomWheel*)GetSupportJoint();
		joint->SetSteerRate(steerRate);
	}

	Ogre::Real Wheel::GetSteerRate() const
	{
		dCustomWheel* joint = (dCustomWheel*)GetSupportJoint();
		return joint->GetSteerRate();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	VehicleTire::VehicleTire(OgreNewt::Body* child, OgreNewt::Body* parentBody, const Ogre::Vector3& pos, const Ogre::Vector3& pin, Vehicle* parent, Ogre::Real radius)
		: Joint(),
		m_tireConfiguration(RayCastTire::TireConfiguration())
	{
		dCustomJoint* supportJoint;

		// make the joint matrix 
		dVector dir(pin.x, pin.y, pin.z, 0.0f);
		dMatrix pinsAndPivoFrame(dGrammSchmidt(dir));
		pinsAndPivoFrame.m_posit = dVector(pos.x, pos.y, pos.z, 1.0f);

		// New command: NewtonBodySetGyroscopicTorque(box0, 1);

		// create a Newton Custom joint and set it at the support joint	
		supportJoint = new RayCastTire(child->getWorld()->getNewtonWorld(), pinsAndPivoFrame, dir, child, parentBody, parent, m_tireConfiguration, radius);
		SetSupportJoint(supportJoint);
	}

	VehicleTire::~VehicleTire()
	{
	}

	void VehicleTire::setVehicleTireSide(VehicleTireSide tireSide)
	{
		RayCastTire* joint = (RayCastTire*)GetSupportJoint();

		// TODO: Check if each time the config is reset, due to bad reference management
		m_tireConfiguration.tireSide = tireSide;

		joint->SetTireConfiguration(m_tireConfiguration);
	}

	VehicleTireSide VehicleTire::getVehicleTireSide(void) const
	{
		return m_tireConfiguration.tireSide;
	}

	void VehicleTire::setVehicleTireSteer(VehicleTireSteer tireSteer)
	{
		RayCastTire* joint = (RayCastTire*)GetSupportJoint();

		m_tireConfiguration.tireSteer = tireSteer;

		joint->SetTireConfiguration(m_tireConfiguration);
	}

	VehicleTireSteer VehicleTire::getVehicleTireSteer(void) const
	{
		return m_tireConfiguration.tireSteer;
	}

	void VehicleTire::setVehicleSteerSide(VehicleSteerSide steerSide)
	{
		RayCastTire* joint = (RayCastTire*)GetSupportJoint();

		m_tireConfiguration.steerSide = steerSide;

		joint->SetTireConfiguration(m_tireConfiguration);
	}

	VehicleSteerSide VehicleTire::getVehicleSteerSide(void) const
	{
		return m_tireConfiguration.steerSide;
	}

	void VehicleTire::setVehicleTireAccel(VehicleTireAccel tireAccel)
	{
		RayCastTire* joint = (RayCastTire*)GetSupportJoint();

		m_tireConfiguration.tireAccel = tireAccel;

		joint->SetTireConfiguration(m_tireConfiguration);
	}

	VehicleTireAccel VehicleTire::getVehicleTireAccel(void) const
	{
		return m_tireConfiguration.tireAccel;
	}

	void VehicleTire::setVehicleTireBrake(VehicleTireBrake brakeMode)
	{
		RayCastTire* joint = (RayCastTire*)GetSupportJoint();

		m_tireConfiguration.brakeMode = brakeMode;

		joint->SetTireConfiguration(m_tireConfiguration);
	}

	VehicleTireBrake VehicleTire::getVehicleTireBrake(void) const
	{
		return m_tireConfiguration.brakeMode;
	}

	void VehicleTire::setLateralFriction(Ogre::Real lateralFriction)
	{
		RayCastTire* joint = (RayCastTire*)GetSupportJoint();

		m_tireConfiguration.lateralFriction = lateralFriction;

		joint->SetTireConfiguration(m_tireConfiguration);
	}

	Ogre::Real VehicleTire::getLateralFriction(void) const
	{
		return m_tireConfiguration.lateralFriction;
	}

	void VehicleTire::setLongitudinalFriction(Ogre::Real longitudinalFriction)
	{
		RayCastTire* joint = (RayCastTire*)GetSupportJoint();

		m_tireConfiguration.longitudinalFriction = longitudinalFriction;

		joint->SetTireConfiguration(m_tireConfiguration);
	}

	Ogre::Real VehicleTire::getLongitudinalFriction(void) const
	{
		return m_tireConfiguration.longitudinalFriction;
	}

	void VehicleTire::setSpringLength(Ogre::Real springLength)
	{
		RayCastTire* joint = (RayCastTire*)GetSupportJoint();

		m_tireConfiguration.springLength = springLength;

		joint->SetTireConfiguration(m_tireConfiguration);
	}

	Ogre::Real VehicleTire::getSpringLength(void) const
	{
		return m_tireConfiguration.springLength;
	}

	void VehicleTire::setSmass(Ogre::Real smass)
	{
		RayCastTire* joint = (RayCastTire*)GetSupportJoint();

		m_tireConfiguration.smass = smass;

		joint->SetTireConfiguration(m_tireConfiguration);
	}

	Ogre::Real VehicleTire::getSmass(void) const
	{
		return m_tireConfiguration.smass;
	}

	void VehicleTire::setSpringConst(Ogre::Real springConst)
	{
		RayCastTire* joint = (RayCastTire*)GetSupportJoint();

		m_tireConfiguration.springConst = springConst;

		joint->SetTireConfiguration(m_tireConfiguration);
	}

	Ogre::Real VehicleTire::getSpringConst(void) const
	{
		return m_tireConfiguration.springConst;
	}

	void VehicleTire::setSpringDamp(Ogre::Real springDamp)
	{
		RayCastTire* joint = (RayCastTire*)GetSupportJoint();

		m_tireConfiguration.springDamp = springDamp;

		joint->SetTireConfiguration(m_tireConfiguration);
	}

	Ogre::Real VehicleTire::getSpringDamp(void) const
	{
		return m_tireConfiguration.springDamp;
	}

	RayCastTire* VehicleTire::getRayCastTire(void)
	{
		return (RayCastTire*)GetSupportJoint();
	}

}   // end NAMESPACE OgreNewt
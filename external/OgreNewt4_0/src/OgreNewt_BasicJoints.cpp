#include "OgreNewt_Stdafx.h"
#include "OgreNewt_World.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_BasicJoints.h"

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
	BallAndSocket::BallAndSocket(const Body* child, const Body* parent, const Ogre::Vector3& pos)
		: m_child(const_cast<Body*>(child)),
		m_parent(const_cast<Body*>(parent)),
		m_twistEnabled(false),
		m_coneEnabled(false),
		m_twistFriction(0.0f),
		m_coneFriction(0.0f)
	{
		ndBodyKinematic* b0 = child ? const_cast<ndBodyKinematic*>(child->getNewtonBody()) : nullptr;
		ndBodyKinematic* b1 = parent ? const_cast<ndBodyKinematic*>(parent->getNewtonBody()) : nullptr;

		ndMatrix pivot(ndGetIdentityMatrix());
		pivot.m_posit = ndVector(pos.x, pos.y, pos.z, 1.0f);

		ndJointSpherical* joint = new ndJointSpherical(pivot, b0, b1);
		SetSupportJoint(joint);
	}

	BallAndSocket::~BallAndSocket()
	{
		// base Joint destructor handles cleanup
	}

	void BallAndSocket::enableTwist(bool state)
	{
		m_twistEnabled = state;
	}

	void BallAndSocket::setTwistLimits(const Ogre::Degree& minAngle, const Ogre::Degree& maxAngle)
	{
		if (auto* j = asNd())
			j->SetTwistLimits(static_cast<ndFloat32>(Ogre::Radian(minAngle).valueRadians()), static_cast<ndFloat32>(Ogre::Radian(maxAngle).valueRadians()));
		if (m_child)
			m_child->unFreeze();
		if (m_parent)
			m_parent->unFreeze();
	}

	void BallAndSocket::getTwistLimits(Ogre::Degree& minAngle, Ogre::Degree& maxAngle) const
	{
		if (auto* j = asNd())
		{
			ndFloat32 minR, maxR;
			j->GetTwistLimits(minR, maxR);
			minAngle = Ogre::Radian(minR);
			maxAngle = Ogre::Radian(maxR);
		}
	}

	void BallAndSocket::enableCone(bool state)
	{
		m_coneEnabled = state;
	}

	Ogre::Degree BallAndSocket::getConeLimits() const
	{
		if (auto* j = asNd())
			return Ogre::Radian(j->GetConeLimit());
		return Ogre::Degree(0);
	}

	void BallAndSocket::setConeLimits(const Ogre::Degree& maxAngle)
	{
		if (auto* j = asNd())
			j->SetConeLimit(static_cast<ndFloat32>(Ogre::Radian(maxAngle).valueRadians()));
		if (m_child)
			m_child->unFreeze();
		if (m_parent)
			m_parent->unFreeze();
	}

	void BallAndSocket::setTwistFriction(Ogre::Real frictionTorque)
	{
		m_twistFriction = frictionTorque;
		if (auto* j = asNd())
			j->SetAsSpringDamper(0.01f, 0.0f, static_cast<ndFloat32>(frictionTorque)); // mimic friction

		if (m_child)
			m_child->unFreeze();
		if (m_parent)
			m_parent->unFreeze();
	}

	Ogre::Real BallAndSocket::getTwistFriction(Ogre::Real) const
	{
		return m_twistFriction;
	}

	void BallAndSocket::setConeFriction(Ogre::Real frictionTorque)
	{
		m_coneFriction = frictionTorque;
		if (auto* j = asNd())
			j->SetAsSpringDamper(0.01f, 0.0f, static_cast<ndFloat32>(frictionTorque));

		if (m_child)
			m_child->unFreeze();
		if (m_parent)
			m_parent->unFreeze();
	}

	Ogre::Real BallAndSocket::getConeFriction(Ogre::Real) const
	{
		return m_coneFriction;
	}

	void BallAndSocket::setTwistSpringDamper(bool /*state*/, Ogre::Real springDamperRelaxation, Ogre::Real spring, Ogre::Real damper)
	{
		if (auto* j = asNd())
		{
			j->SetAsSpringDamper(static_cast<ndFloat32>(springDamperRelaxation), static_cast<ndFloat32>(spring), static_cast<ndFloat32>(damper));
		}
		if (m_child)
			m_child->unFreeze();
		if (m_parent)
			m_parent->unFreeze();
	}

	Ogre::Vector3 BallAndSocket::getJointForce()
	{
		// ND4 has no joint-specific force API; approximate via body forces difference
		if (!m_child) return Ogre::Vector3::ZERO;

		ndVector f0 = m_child->getNewtonBody()->GetForce();
		ndVector f1 = m_parent ? m_parent->getNewtonBody()->GetForce() : ndVector::m_zero;
		ndVector result = f0 - f1;
		return Ogre::Vector3(result.m_x, result.m_y, result.m_z);
	}

	Ogre::Vector3 BallAndSocket::getJointAngle()
	{
		// Not directly exposed; return zero or future-calculated value
		return Ogre::Vector3::ZERO;
	}

	Ogre::Vector3 BallAndSocket::getJointOmega()
	{
		// Compute relative angular velocity
		if (!m_child) return Ogre::Vector3::ZERO;
		ndVector w0 = m_child->getNewtonBody()->GetOmega();
		ndVector w1 = m_parent ? m_parent->getNewtonBody()->GetOmega() : ndVector::m_zero;
		ndVector diff = w0 - w1;
		return Ogre::Vector3(diff.m_x, diff.m_y, diff.m_z);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	Hinge::Hinge(const Body* child, const Body* parent, const Ogre::Vector3& pos, const Ogre::Vector3& pin)
	{
		m_body0 = const_cast<Body*>(child);
		m_body1 = const_cast<Body*>(parent);

		ndBodyKinematic* b0 = child ? const_cast<ndBodyKinematic*>(child->getNewtonBody()) : nullptr;
		ndBodyKinematic* b1 = parent ? const_cast<ndBodyKinematic*>(parent->getNewtonBody()) : nullptr;

		// build ND frame from Ogre quaternion returned by grammSchmidt
		Ogre::Quaternion q = OgreNewt::Converters::grammSchmidt(pin);
		Ogre::Matrix3 m3;
		q.ToRotationMatrix(m3);

		ndMatrix frame(ndGetIdentityMatrix());
		frame.m_front = ndVector(m3.GetColumn(0).x, m3.GetColumn(0).y, m3.GetColumn(0).z, 0.0f);
		frame.m_up = ndVector(m3.GetColumn(1).x, m3.GetColumn(1).y, m3.GetColumn(1).z, 0.0f);
		frame.m_right = ndVector(m3.GetColumn(2).x, m3.GetColumn(2).y, m3.GetColumn(2).z, 0.0f);
		frame.m_posit = ndVector(pos.x, pos.y, pos.z, 1.0f);

		// store local pin for later world reconstruction
		{
			Ogre::Quaternion q0; Ogre::Vector3 p0;
			if (m_body0)
				m_body0->getPositionOrientation(p0, q0);

			m_childPinLocal = q0.Inverse() * pin.normalisedCopy();
		}

		auto* joint = new ndJointHinge(frame, b0, b1);
		SetSupportJoint(joint);
	}

	Hinge::Hinge(const Body* child, const Body* parent, const Ogre::Vector3& childPos, const Ogre::Vector3& childPin, const Ogre::Vector3& parentPos, const Ogre::Vector3& parentPin)
	{
		m_body0 = const_cast<Body*>(child);
		m_body1 = const_cast<Body*>(parent);

		ndBodyKinematic* b0 = child ? const_cast<ndBodyKinematic*>(child->getNewtonBody()) : nullptr;
		ndBodyKinematic* b1 = parent ? const_cast<ndBodyKinematic*>(parent->getNewtonBody()) : nullptr;

		Ogre::Vector3 mid = (childPos + parentPos) * 0.5f;

		Ogre::Quaternion q = OgreNewt::Converters::grammSchmidt(childPin);
		Ogre::Matrix3 m3;
		q.ToRotationMatrix(m3);

		ndMatrix frame(ndGetIdentityMatrix());
		frame.m_front = ndVector(m3.GetColumn(0).x, m3.GetColumn(0).y, m3.GetColumn(0).z, 0.0f);
		frame.m_up = ndVector(m3.GetColumn(1).x, m3.GetColumn(1).y, m3.GetColumn(1).z, 0.0f);
		frame.m_right = ndVector(m3.GetColumn(2).x, m3.GetColumn(2).y, m3.GetColumn(2).z, 0.0f);
		frame.m_posit = ndVector(mid.x, mid.y, mid.z, 1.0f);

		{
			Ogre::Quaternion q0; Ogre::Vector3 p0;

			if (m_body0)
				m_body0->getPositionOrientation(p0, q0);

			m_childPinLocal = q0.Inverse() * childPin.normalisedCopy();
		}

		auto* joint = new ndJointHinge(frame, b0, b1);
		SetSupportJoint(joint);
	}

	Hinge::~Hinge()
	{
		// Joint base handles clearing the native pointer/ownership integration
	}

	void Hinge::EnableLimits(bool state)
	{
		m_limitsEnabled = state;
		if (auto* j = asNd())
			j->SetLimitState(state);

		if (m_body0)
			m_body0->unFreeze();
		if (m_body1)
			m_body1->unFreeze();
	}

	void Hinge::SetLimits(Ogre::Degree minAngle, Ogre::Degree maxAngle)
	{
		if (auto* j = asNd())
		{
			const ndFloat32 minR = static_cast<ndFloat32>(Ogre::Radian(minAngle).valueRadians());
			const ndFloat32 maxR = static_cast<ndFloat32>(Ogre::Radian(maxAngle).valueRadians());
			j->SetLimits(minR, maxR);
		}
		if (m_body0)
			m_body0->unFreeze();
		if (m_body1)
			m_body1->unFreeze();
	}

	Ogre::Radian Hinge::GetJointAngle() const
	{
		if (auto* j = const_cast<Hinge*>(this)->asNd())
			return Ogre::Radian(j->GetAngle());
		return Ogre::Radian(0.0f);
	}

	Ogre::Radian Hinge::GetJointAngularVelocity() const
	{
		// If ndJointHinge exposes GetOmega(), use it; otherwise project relative omega onto hinge axis.
		if (auto* j = const_cast<Hinge*>(this)->asNd())
		{
			// Prefer direct API if available:
			// return Ogre::Radian(j->GetOmega());

			// Fallback: compute relative angular velocity · pinAxis
			Ogre::Quaternion q0;
			Ogre::Vector3 p0;

			if (!m_body0)
				return Ogre::Radian(0.0f);

			m_body0->getPositionOrientation(p0, q0);
			const Ogre::Vector3 pinWorld = q0 * m_childPinLocal;

			const ndVector w0 = m_body0->getNewtonBody()->GetOmega();
			const ndVector w1 = (m_body1 ? m_body1->getNewtonBody()->GetOmega() : ndVector::m_zero);
			const Ogre::Vector3 wRel(w0.m_x - w1.m_x, w0.m_y - w1.m_y, w0.m_z - w1.m_z);

			const Ogre::Real omega = wRel.dotProduct(pinWorld);
			return Ogre::Radian(omega);
		}
		return Ogre::Radian(0.0f);
	}

	Ogre::Vector3 Hinge::GetJointPin() const
	{
		// Reconstruct from child local pin and current child orientation
		Ogre::Quaternion q0; Ogre::Vector3 p0;

		if (!m_body0)
			return Ogre::Vector3::UNIT_X;

		m_body0->getPositionOrientation(p0, q0);

		Ogre::Vector3 pinWorld = q0 * m_childPinLocal;
		pinWorld.normalise();
		return pinWorld;
	}

	void Hinge::SetTorque(Ogre::Real torque)
	{
		// ND3 had EnableMotor(torque). ND4 hinge shows PD/target-angle style control.
		// We emulate "motor torque" as viscous torque around the hinge axis by bumping damper.
		// If you later build a motorized subclass, you can drive SetTargetAngle each step.
		m_motorTorque = torque;

		if (auto* j = asNd())
		{
			const ndFloat32 reg = static_cast<ndFloat32>(m_lastRegularizer);
			const ndFloat32 spring = 0.0f;
			const ndFloat32 damper = static_cast<ndFloat32>(std::fabs(torque));
			j->SetAsSpringDamper(reg, spring, damper);
		}

		if (m_body0)
			m_body0->unFreeze();
		if (m_body1)
			m_body1->unFreeze();
	}

	void Hinge::SetSpringAndDamping(bool enable, bool massIndependent,
		Ogre::Real springDamperRelaxation,
		Ogre::Real spring, Ogre::Real damper)
	{
		m_lastRegularizer = springDamperRelaxation;

		if (auto* j = asNd())
		{
			if (enable)
			{
				// ND4 does not differentiate “mass independent” in this API;
				// regularizer acts similarly to ERP/CFM tuning.
				j->SetAsSpringDamper(static_cast<ndFloat32>(springDamperRelaxation), static_cast<ndFloat32>(spring), static_cast<ndFloat32>(damper));
			}
			else
			{
				// disable by zeroing gains
				j->SetAsSpringDamper(static_cast<ndFloat32>(springDamperRelaxation), 0.0f, 0.0f);
			}
		}

		if (m_body0)
			m_body0->unFreeze();

		if (m_body1)
			m_body1->unFreeze();
	}

	void Hinge::SetFriction(Ogre::Real friction)
	{
		// ND3 had SetFriction on hinge; ND4 examples use viscous friction via SetAsSpringDamper(reg, 0, friction)
		m_setFriction = friction;

		if (auto* j = asNd())
			j->SetAsSpringDamper(static_cast<ndFloat32>(m_lastRegularizer), 0.0f, static_cast<ndFloat32>(friction));

		if (m_body0)
			m_body0->unFreeze();
		if (m_body1)
			m_body1->unFreeze();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	Slider::Slider(const Body* child, const Body* parent,
		const Ogre::Vector3& pos, const Ogre::Vector3& pin)
	{
		m_body0 = const_cast<Body*>(child);
		m_body1 = const_cast<Body*>(parent);

		ndBodyKinematic* b0 = child ? const_cast<ndBodyKinematic*>(child->getNewtonBody()) : nullptr;
		ndBodyKinematic* b1 = parent ? const_cast<ndBodyKinematic*>(parent->getNewtonBody()) : nullptr;

		// use converter to build ndMatrix
		Ogre::Quaternion q = OgreNewt::Converters::grammSchmidt(pin);
		ndMatrix frame;
		OgreNewt::Converters::QuatPosToMatrix(q, pos, frame);

		auto* joint = new ndJointSlider(frame, b0, b1);
		SetSupportJoint(joint);
	}

	Slider::~Slider()
	{
		// base Joint destructor handles clearing native joint
	}

	void Slider::enableLimits(bool state)
	{
		m_limitsEnabled = state;
		if (auto* j = asNd())
			j->SetLimitState(state);

		if (m_body0)
			m_body0->unFreeze();
		if (m_body1)
			m_body1->unFreeze();
	}

	void Slider::setLimits(Ogre::Real minStopDist, Ogre::Real maxStopDist)
	{
		if (auto* j = asNd())
			j->SetLimits(static_cast<ndFloat32>(minStopDist), static_cast<ndFloat32>(maxStopDist));

		if (m_body0)
			m_body0->unFreeze();
		if (m_body1)
			m_body1->unFreeze();
	}

	void Slider::setFriction(Ogre::Real friction)
	{
		// ND4 has no SetFriction(), emulate with SetAsSpringDamper
		m_friction = friction;
		if (auto* j = asNd())
			j->SetAsSpringDamper(static_cast<ndFloat32>(m_lastRegularizer), 0.0f, static_cast<ndFloat32>(friction));

		if (m_body0)
			m_body0->unFreeze();
		if (m_body1)
			m_body1->unFreeze();
	}

	void Slider::setSpringAndDamping(bool state, Ogre::Real springDamperRelaxation, Ogre::Real spring, Ogre::Real damper)
	{
		m_lastRegularizer = springDamperRelaxation;

		if (auto* j = asNd())
		{
			if (state)
			{
				j->SetAsSpringDamper(static_cast<ndFloat32>(springDamperRelaxation),
					static_cast<ndFloat32>(spring),
					static_cast<ndFloat32>(damper));
			}
			else
			{
				// disabled spring/damper → zero stiffness/damping
				j->SetAsSpringDamper(static_cast<ndFloat32>(springDamperRelaxation), 0.0f, 0.0f);
			}
		}

		if (m_body0) m_body0->unFreeze();
		if (m_body1) m_body1->unFreeze();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	Gear::Gear(const Body* child, const Body* parent, const Ogre::Vector3& childPin, const Ogre::Vector3& parentPin, Ogre::Real gearRatio)
	{
		ndBodyKinematic* b0 = child ? const_cast<ndBodyKinematic*>(child->getNewtonBody()) : nullptr;
		ndBodyKinematic* b1 = parent ? const_cast<ndBodyKinematic*>(parent->getNewtonBody()) : nullptr;

		// Convert pin directions
		ndVector pin0(childPin.x, childPin.y, childPin.z, 0.0f);
		ndVector pin1(parentPin.x, parentPin.y, parentPin.z, 0.0f);

		// Create the Newton 4 joint
		ndJointGear* joint = new ndJointGear(static_cast<ndFloat32>(gearRatio), pin0, b0, pin1, b1);
		SetSupportJoint(joint);
	}

	Gear::~Gear()
	{
		// base Joint handles cleanup
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	Universal::Universal(const Body* child, const Body* parent, const Ogre::Vector3& pos, const Ogre::Vector3& pin)
	{
		m_body0 = const_cast<Body*>(child);
		m_body1 = const_cast<Body*>(parent);

		ndBodyKinematic* b0 = child ? const_cast<ndBodyKinematic*>(child->getNewtonBody()) : nullptr;
		ndBodyKinematic* b1 = parent ? const_cast<ndBodyKinematic*>(parent->getNewtonBody()) : nullptr;

		// Build frame from your canonical converter
		Ogre::Quaternion q = OgreNewt::Converters::grammSchmidt(pin);
		ndMatrix frame;
		OgreNewt::Converters::QuatPosToMatrix(q, pos, frame);

		auto* joint = new NdUniversalAdapter(frame, b0, b1);
		SetSupportJoint(joint);
	}

	Universal::Universal(const Body* child, const Body* parent, const Ogre::Vector3& childPos, const Ogre::Vector3& childPin, const Ogre::Vector3& parentPos, const Ogre::Vector3& parentPin)
	{
		m_body0 = const_cast<Body*>(child);
		m_body1 = const_cast<Body*>(parent);

		ndBodyKinematic* b0 = child ? const_cast<ndBodyKinematic*>(child->getNewtonBody()) : nullptr;
		ndBodyKinematic* b1 = parent ? const_cast<ndBodyKinematic*>(parent->getNewtonBody()) : nullptr;

		// Use mid-point for pivot; align with childPin
		const Ogre::Vector3 mid = (childPos + parentPos) * 0.5f;
		Ogre::Quaternion q = OgreNewt::Converters::grammSchmidt(childPin);
		ndMatrix frame;
		OgreNewt::Converters::QuatPosToMatrix(q, mid, frame);

		auto* joint = new NdUniversalAdapter(frame, b0, b1);
		SetSupportJoint(joint);
	}

	Universal::~Universal()
	{
		// base Joint handles native cleanup
	}

	// ----------------------------------------------------
	// Limits (Axis 0)
	// ----------------------------------------------------
	void Universal::enableLimits0(bool state0)
	{
		if (auto* j = asNd())
			j->m_limits0Enabled = state0;

		if (m_body0)
			m_body0->unFreeze();
		if (m_body1)
			m_body1->unFreeze();
	}

	void Universal::setLimits0(Ogre::Degree minAngle0, Ogre::Degree maxAngle0)
	{
		const ndFloat32 minR = static_cast<ndFloat32>(Ogre::Radian(minAngle0).valueRadians());
		const ndFloat32 maxR = static_cast<ndFloat32>(Ogre::Radian(maxAngle0).valueRadians());

		if (auto* j = asNd())
		{
			j->m_min0 = minR;
			j->m_max0 = maxR;

			if (j->m_limits0Enabled)
				j->SetLimits0(minR, maxR);
			else
				j->SetLimits0(-ndFloat32(1.0e10f), ndFloat32(1.0e10f)); // effectively disabled
		}

		if (m_body0)
			m_body0->unFreeze();
		if (m_body1)
			m_body1->unFreeze();
	}

	// ----------------------------------------------------
	// Limits (Axis 1)
	// ----------------------------------------------------
	void Universal::enableLimits1(bool state1)
	{
		if (auto* j = asNd())
			j->m_limits1Enabled = state1;

		if (m_body0)
			m_body0->unFreeze();
		if (m_body1)
			m_body1->unFreeze();
	}

	void Universal::setLimits1(Ogre::Degree minAngle1, Ogre::Degree maxAngle1)
	{
		const ndFloat32 minR = static_cast<ndFloat32>(Ogre::Radian(minAngle1).valueRadians());
		const ndFloat32 maxR = static_cast<ndFloat32>(Ogre::Radian(maxAngle1).valueRadians());

		if (auto* j = asNd())
		{
			j->m_min1 = minR;
			j->m_max1 = maxR;
			if (j->m_limits1Enabled)
				j->SetLimits1(minR, maxR);
			else
				j->SetLimits1(-ndFloat32(1.0e10f), ndFloat32(1.0e10f));
		}

		if (m_body0)
			m_body0->unFreeze();
		if (m_body1)
			m_body1->unFreeze();
	}

	// ----------------------------------------------------
	// Motor emulation (Axis 0)
	// ----------------------------------------------------
	void Universal::enableMotor0(bool state0, Ogre::Real motorSpeed0)
	{
		if (auto* j = asNd())
		{
			j->m_motor0Enabled = state0;
			j->m_motor0Speed = static_cast<ndFloat32>(motorSpeed0);
		}

		if (m_body0)
			m_body0->unFreeze();
		if (m_body1)
			m_body1->unFreeze();
	}

	// ----------------------------------------------------
	// Friction (Axis 0/1) → viscous damping via damper term
	// ----------------------------------------------------
	void Universal::setFriction0(Ogre::Real frictionTorque)
	{
		if (auto* j = asNd())
			j->m_friction0 = static_cast<ndFloat32>(frictionTorque);

		if (m_body0)
			m_body0->unFreeze();
		if (m_body1)
			m_body1->unFreeze();
	}

	void Universal::setFriction1(Ogre::Real frictionTorque)
	{
		if (auto* j = asNd())
			j->m_friction1 = static_cast<ndFloat32>(frictionTorque);

		if (m_body0)
			m_body0->unFreeze();
		if (m_body1)
			m_body1->unFreeze();
	}

	// ----------------------------------------------------
	// Spring/Damper (Axis 0/1)
	// ----------------------------------------------------
	void Universal::setAsSpringDamper0(bool state, Ogre::Real springDamperRelaxation,
		Ogre::Real spring, Ogre::Real damper)
	{
		if (auto* j = asNd())
		{
			j->m_sd0Enabled = state;
			j->m_reg0 = static_cast<ndFloat32>(springDamperRelaxation);
			j->m_k0 = state ? static_cast<ndFloat32>(spring) : 0.0f;
			j->m_c0 = state ? static_cast<ndFloat32>(damper) : 0.0f;
			// actual application is done each step in JacobianDerivative
		}

		if (m_body0)
			m_body0->unFreeze();
		if (m_body1)
			m_body1->unFreeze();
	}

	void Universal::setAsSpringDamper1(bool state, Ogre::Real springDamperRelaxation, Ogre::Real spring, Ogre::Real damper)
	{
		if (auto* j = asNd())
		{
			j->m_sd1Enabled = state;
			j->m_reg1 = static_cast<ndFloat32>(springDamperRelaxation);
			j->m_k1 = state ? static_cast<ndFloat32>(spring) : 0.0f;
			j->m_c1 = state ? static_cast<ndFloat32>(damper) : 0.0f;
			// application in JacobianDerivative
		}

		if (m_body0)
			m_body0->unFreeze();
		if (m_body1)
			m_body1->unFreeze();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	CorkScrew::CorkScrew(const Body* child, const Body* parent, const Ogre::Vector3& pos)
	{
		ndBodyKinematic* b0 = child ? const_cast<ndBodyKinematic*>(child->getNewtonBody()) : nullptr;
		ndBodyKinematic* b1 = parent ? const_cast<ndBodyKinematic*>(parent->getNewtonBody()) : nullptr;

		ndMatrix frame(ndGetIdentityMatrix());
		frame.m_posit = ndVector(pos.x, pos.y, pos.z, 1.0f);

		auto* joint = new ndJointRoller(frame, b0, b1);
		SetSupportJoint(joint);
	}

	CorkScrew::~CorkScrew()
	{
		// Base Joint handles release
	}

	void CorkScrew::enableLimits(bool linearLimits, bool angularLimits)
	{
		if (auto* j = getJoint())
		{
			j->SetLimitStatePosit(linearLimits);
			j->SetLimitStateAngle(angularLimits);
		}
	}

	void CorkScrew::setLinearLimits(Ogre::Real minLinearDistance, Ogre::Real maxLinearDistance)
	{
		if (auto* j = getJoint())
		{
			j->SetLimitsPosit(static_cast<ndFloat32>(minLinearDistance), static_cast<ndFloat32>(maxLinearDistance));
		}
	}

	void CorkScrew::setAngularLimits(Ogre::Degree minAngularAngle, Ogre::Degree maxAngularAngle)
	{
		if (auto* j = getJoint())
		{
			j->SetLimitsAngle(static_cast<ndFloat32>(Ogre::Radian(minAngularAngle).valueRadians()), static_cast<ndFloat32>(Ogre::Radian(maxAngularAngle).valueRadians()));
		}
	}

	void CorkScrew::setSpringDamperPosit(bool enable, Ogre::Real regularizer, Ogre::Real spring, Ogre::Real damper)
	{
		if (auto* j = getJoint())
		{
			if (enable)
				j->SetAsSpringDamperPosit(static_cast<ndFloat32>(regularizer), static_cast<ndFloat32>(spring), static_cast<ndFloat32>(damper));
			else
				j->SetAsSpringDamperPosit(static_cast<ndFloat32>(regularizer), 0.0f, 0.0f);
		}
	}

	void CorkScrew::setSpringDamperAngle(bool enable, Ogre::Real regularizer, Ogre::Real spring, Ogre::Real damper)
	{
		if (auto* j = getJoint())
		{
			if (enable)
				j->SetAsSpringDamperAngle(static_cast<ndFloat32>(regularizer), static_cast<ndFloat32>(spring), static_cast<ndFloat32>(damper));
			else
				j->SetAsSpringDamperAngle(static_cast<ndFloat32>(regularizer), 0.0f, 0.0f);
		}
	}

	// ---- Rotation (angular) ----
	Ogre::Radian CorkScrew::getAngle() const
	{
		return Ogre::Radian(getJoint() ? getJoint()->GetAngle() : 0.0f);
	}

	Ogre::Radian CorkScrew::getOffsetAngle() const
	{
		return Ogre::Radian(getJoint() ? getJoint()->GetOffsetAngle() : 0.0f);
	}

	Ogre::Radian CorkScrew::getOmega() const
	{
		return Ogre::Radian(getJoint() ? getJoint()->GetOmega() : 0.0f);
	}

	void CorkScrew::setOffsetAngle(Ogre::Radian angle)
	{
		if (auto* j = getJoint())
			j->SetOffsetAngle(static_cast<ndFloat32>(angle.valueRadians()));
	}

	void CorkScrew::setTargetOffsetAngle(Ogre::Radian offset)
	{
		if (auto* j = getJoint())
			j->SetOffsetAngle(static_cast<ndFloat32>(offset.valueRadians()));
	}

	void CorkScrew::getAngleLimits(Ogre::Radian& min, Ogre::Radian& max) const
	{
		if (auto* j = getJoint())
		{
			ndFloat32 mn, mx;
			j->GetLimitsAngle(mn, mx);
			min = Ogre::Radian(mn);
			max = Ogre::Radian(mx);
		}
		else
		{
			min = max = Ogre::Radian(0);
		}
	}

	bool CorkScrew::getLimitStateAngle() const
	{
		return getJoint() ? getJoint()->GetLimitStateAngle() : false;
	}

	// ---- Translation (linear) ----
	Ogre::Real CorkScrew::getPosit() const
	{
		return getJoint() ? getJoint()->GetPosit() : 0.0f;
	}

	Ogre::Real CorkScrew::getTargetPosit() const
	{
		return getJoint() ? getJoint()->GetTargetPosit() : 0.0f;
	}

	void CorkScrew::setTargetPosit(Ogre::Real offset)
	{
		if (auto* j = getJoint())
			j->SetTargetPosit(static_cast<ndFloat32>(offset));
	}

	void CorkScrew::getLinearLimits(Ogre::Real& min, Ogre::Real& max) const
	{
		if (auto* j = getJoint())
		{
			ndFloat32 mn, mx;
			j->GetLimitsPosit(mn, mx);
			min = mn;
			max = mx;
		}
		else
		{
			min = max = 0.0f;
		}
	}

	bool CorkScrew::getLimitStatePosit() const
	{
		return getJoint() ? getJoint()->GetLimitStatePosit() : false;
	}

	// ---- Spring/Damper queries ----
	void CorkScrew::getSpringDamperAngle(Ogre::Real& regularizer, Ogre::Real& spring, Ogre::Real& damper) const
	{
		if (auto* j = getJoint())
		{
			ndFloat32 r, k, c;
			j->GetSpringDamperAngle(r, k, c);
			regularizer = r;
			spring = k;
			damper = c;
		}
		else
		{
			regularizer = spring = damper = 0.0f;
		}
	}

	void CorkScrew::getSpringDamperPosit(Ogre::Real& regularizer, Ogre::Real& spring, Ogre::Real& damper) const
	{
		if (auto* j = getJoint())
		{
			ndFloat32 r, k, c;
			j->GetSpringDamperPosit(r, k, c);
			regularizer = r;
			spring = k;
			damper = c;
		}
		else
		{
			regularizer = spring = damper = 0.0f;
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	SlidingContact::SlidingContact(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos, const Ogre::Vector3& pin)
		: Joint()
	{
		m_body0 = const_cast<OgreNewt::Body*>(child);
		m_body1 = const_cast<OgreNewt::Body*>(parent);

		// Build a frame from pin (front axis) and pos
		const Ogre::Quaternion q = OgreNewt::Converters::grammSchmidt(pin);
		ndMatrix frame(ndGetIdentityMatrix());
		OgreNewt::Converters::QuatPosToMatrix(q, pos, frame);

		// Create Newton4 cylindrical joint (same DOFs as 3.x dCustomSlidingContact)
		ndBodyKinematic* b0 = m_body0 ? const_cast<ndBodyKinematic*>(m_body0->getNewtonBody()) : nullptr;
		ndBodyKinematic* b1 = m_body1 ? const_cast<ndBodyKinematic*>(m_body1->getNewtonBody()) : nullptr;

		ndJointCylinder* j = new ndJointCylinder(frame, b0, b1 ? b1 : nullptr);
		SetSupportJoint(j);

		// Defaults: limits disabled and no spring/damper (parity with old ctor)
		j->SetLimitStatePosit(false);
		j->SetLimitStateAngle(false);
		j->SetAsSpringDamperPosit(0.1f, 0.0f, 0.0f);
		j->SetAsSpringDamperAngle(0.1f, 0.0f, 0.0f);
	}

	SlidingContact::~SlidingContact()
	{
		// Joint base handles deletion/ownership according to your world integration
	}

	static inline void wake(OgreNewt::Body* a, OgreNewt::Body* b)
	{
		if (a)
			a->unFreeze();
		if (b)
			b->unFreeze();
	}

	void SlidingContact::enableLinearLimits(bool state)
	{
		if (auto* j = asNd())
		{
			j->SetLimitStatePosit(state);
			wake(m_body0, m_body1);
		}
	}

	void SlidingContact::enableAngularLimits(bool state)
	{
		if (auto* j = asNd())
		{
			j->SetLimitStateAngle(state);
			wake(m_body0, m_body1);
		}
	}

	void SlidingContact::setLinearLimits(Ogre::Real minStopDist, Ogre::Real maxStopDist)
	{
		if (auto* j = asNd())
		{
			j->SetLimitsPosit(ndFloat32(minStopDist), ndFloat32(maxStopDist));
			wake(m_body0, m_body1);
		}
	}

	void SlidingContact::setAngularLimits(Ogre::Degree minAngle, Ogre::Degree maxAngle)
	{
		if (auto* j = asNd())
		{
			j->SetLimitsAngle(ndFloat32(minAngle.valueRadians()), ndFloat32(maxAngle.valueRadians()));
			wake(m_body0, m_body1);
		}
	}

	void SlidingContact::setSpringAndDamping(bool state, Ogre::Real springDamperRelaxation, Ogre::Real spring, Ogre::Real damper)
	{
		if (auto* j = asNd())
		{
			// Cache last requested values so toggling on/off preserves user intent
			m_sdEnabled = state;
			m_reg = ndFloat32(springDamperRelaxation);
			m_k = ndFloat32(spring);
			m_c = ndFloat32(damper);

			if (state)
			{
				// Apply to BOTH channels (linear & angular) to emulate 3.x single API
				j->SetAsSpringDamperPosit(m_reg, m_k, m_c);
				j->SetAsSpringDamperAngle(m_reg, m_k, m_c);
			}
			else
			{
				// disable: zero spring & damper (keeps regularizer reasonable)
				j->SetAsSpringDamperPosit(0.1f, 0.0f, 0.0f);
				j->SetAsSpringDamperAngle(0.1f, 0.0f, 0.0f);
			}
			wake(m_body0, m_body1);
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	PointToPoint::PointToPoint(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos1, const Ogre::Vector3& pos2)
		: Joint()
	{
		ndBodyKinematic* b0 = child ? const_cast<ndBodyKinematic*>(child->getNewtonBody()) : nullptr;
		ndBodyKinematic* b1 = parent ? const_cast<ndBodyKinematic*>(parent->getNewtonBody()) : nullptr;

		const ndVector p0(pos1.x, pos1.y, pos1.z, 1.0f);
		const ndVector p1(pos2.x, pos2.y, pos2.z, 1.0f);

		auto* j = new ndJointFixDistance(p0, p1, b0, b1);
		SetSupportJoint(j);
	}

	PointToPoint::~PointToPoint()
	{
		// Base Joint handles cleanup/lifetime
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Joint6Dof::Joint6Dof(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos1, const Ogre::Vector3& pos2)
		: Joint()
	{
		// convert positions to ndMatrix
		ndMatrix frame0(ndGetIdentityMatrix());
		ndMatrix frame1(ndGetIdentityMatrix());
		frame0.m_posit = ndVector(pos1.x, pos1.y, pos1.z, 1.0f);
		frame1.m_posit = ndVector(pos2.x, pos2.y, pos2.z, 1.0f);

		// get Newton bodies
		ndBodyKinematic* const body0 = const_cast<ndBodyKinematic*>(child->getNewtonBody());
		ndBodyKinematic* const body1 = parent ? const_cast<ndBodyKinematic*>(parent->getNewtonBody()) : nullptr;

		// create the 6DoF constraint
		auto* joint = new ndJointFix6dof(body0, body1, frame0, frame1);

		SetSupportJoint(joint);
	}

	Joint6Dof::~Joint6Dof()
	{
		// handled by base Joint destructor
	}

	void Joint6Dof::setLinearLimits(const Ogre::Vector3& minLinearLimits,
		const Ogre::Vector3& maxLinearLimits) const
	{
		// NOTE: ndJointFix6dof in ND4 doesn't yet have per-axis limit methods.
		// We'll leave this function for API compatibility and potential extension.
		// For now, users can implement linear limits manually with additional constraints.
		(void)minLinearLimits;
		(void)maxLinearLimits;
	}

	void Joint6Dof::setYawLimits(const Ogre::Degree& minLimits, const Ogre::Degree& maxLimits) const
	{
		// Placeholder – current ND4 Fix6dof does not expose separate yaw/pitch/roll setters.
		(void)minLimits;
		(void)maxLimits;
	}

	void Joint6Dof::setPitchLimits(const Ogre::Degree& minLimits, const Ogre::Degree& maxLimits) const
	{
		(void)minLimits;
		(void)maxLimits;
	}

	void Joint6Dof::setRollLimits(const Ogre::Degree& minLimits, const Ogre::Degree& maxLimits) const
	{
		(void)minLimits;
		(void)maxLimits;
	}

	void Joint6Dof::setAsSoftJoint(bool enabled)
	{
		if (auto* j = getJoint())
			j->SetAsSoftJoint(enabled);
	}

	void Joint6Dof::setRegularizer(Ogre::Real regularizer)
	{
		if (auto* j = getJoint())
			j->SetRegularizer(static_cast<ndFloat32>(regularizer));
	}

	Ogre::Real Joint6Dof::getRegularizer() const
	{
		if (auto* j = getJoint())
			return j->GetRegularizer();
		return 0.0f;
	}

	Ogre::Real Joint6Dof::getMaxForce() const
	{
		if (auto* j = getJoint())
			return j->GetMaxForce();
		return 0.0f;
	}

	Ogre::Real Joint6Dof::getMaxTorque() const
	{
		if (auto* j = getJoint())
			return j->GetMaxTorque();
		return 0.0f;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	static inline ndFloat32 toRad(Ogre::Degree d) { return static_cast<ndFloat32>(Ogre::Radian(d).valueRadians()); }
	static inline Ogre::Real toDeg(ndFloat32 r) { return Ogre::Radian(r).valueDegrees(); }
	static inline ndFloat32 clampf(ndFloat32 v, ndFloat32 a, ndFloat32 b) { return (v < a) ? a : (v > b) ? b : v; }

	HingeActuator::HingeActuator(const Body* child, const Body* parent,
		const Ogre::Vector3& pos, const Ogre::Vector3& pin,
		const Ogre::Degree& angularRate,
		const Ogre::Degree& minAngle, const Ogre::Degree& maxAngle)
	{
		m_body0 = const_cast<Body*>(child);
		m_body1 = const_cast<Body*>(parent);

		ndBodyKinematic* b0 = child ? const_cast<ndBodyKinematic*>(child->getNewtonBody()) : nullptr;
		ndBodyKinematic* b1 = parent ? const_cast<ndBodyKinematic*>(parent->getNewtonBody()) : nullptr;

		// Build a stable orthonormal frame from 'pin' at world position 'pos'
		Ogre::Quaternion q = OgreNewt::Converters::grammSchmidt(pin);
		Ogre::Matrix3 m3; q.ToRotationMatrix(m3);

		ndMatrix frame(ndGetIdentityMatrix());
		frame.m_front = ndVector(m3.GetColumn(0).x, m3.GetColumn(0).y, m3.GetColumn(0).z, 0.0f);
		frame.m_up = ndVector(m3.GetColumn(1).x, m3.GetColumn(1).y, m3.GetColumn(1).z, 0.0f);
		frame.m_right = ndVector(m3.GetColumn(2).x, m3.GetColumn(2).y, m3.GetColumn(2).z, 0.0f);
		frame.m_posit = ndVector(pos.x, pos.y, pos.z, 1.0f);

		// cache pin in child local space
		{
			Ogre::Quaternion q0; Ogre::Vector3 p0;
			if (m_body0) m_body0->getPositionOrientation(p0, q0);
			m_childPinLocal = q0.Inverse() * pin.normalisedCopy();
		}

		auto* joint = new ndJointHinge(frame, b0, b1);
		SetSupportJoint(joint);

		// init limits + rate
		m_minRad = toRad(minAngle);
		m_maxRad = toRad(maxAngle);
		m_maxOmega = toRad(angularRate);

		if (auto* j = asNd())
		{
			j->SetLimitState(true);
			j->SetLimits(m_minRad, m_maxRad);
			j->SetAsSpringDamper(m_regularizer, 0.0f, 0.0f); // PD disabled by default
			// Initialize targets to current angle
			const ndFloat32 a = j->GetAngle();
			m_targetCmd = clampf(a, m_minRad, m_maxRad);
			m_targetSlew = m_targetCmd;
			j->SetTargetAngle(m_targetSlew);
		}
	}

	HingeActuator::~HingeActuator()
	{
		// Joint base manages native pointer
	}

	void HingeActuator::SetMinAngularLimit(const Ogre::Degree& limit)
	{
		m_minRad = toRad(limit);
		if (auto* j = asNd())
			j->SetLimits(m_minRad, m_maxRad);
		_wake();
	}

	void HingeActuator::SetMaxAngularLimit(const Ogre::Degree& limit)
	{
		m_maxRad = toRad(limit);
		if (auto* j = asNd())
			j->SetLimits(m_minRad, m_maxRad);
		_wake();
	}

	void HingeActuator::SetAngularRate(const Ogre::Degree& rate)
	{
		m_maxOmega = toRad(rate);
		_wake();
	}

	void HingeActuator::SetTargetAngle(const Ogre::Degree& angle)
	{
		m_targetCmd = clampf(toRad(angle), m_minRad, m_maxRad);
		_wake();
	}

	Ogre::Real HingeActuator::GetActuatorAngle() const
	{
		if (auto* j = const_cast<HingeActuator*>(this)->asNd())
			return toDeg(j->GetAngle());
		return 0.0f;
	}

	void HingeActuator::SetMaxTorque(Ogre::Real torque)
	{
		m_maxTorque = static_cast<ndFloat32>(std::max<Ogre::Real>(0.0f, torque));
		_wake();
	}

	Ogre::Real HingeActuator::GetTargetAngleDeg() const
	{
		if (auto* j = const_cast<HingeActuator*>(this)->asNd())
			return Ogre::Radian(j->GetTargetAngle()).valueDegrees();
		return 0.0f;
	}

	Ogre::Real HingeActuator::GetOmegaDegPerSec() const
	{
		if (auto* j = const_cast<HingeActuator*>(this)->asNd())
			return Ogre::Radian(j->GetOmega()).valueDegrees();
		return 0.0f;
	}

	void HingeActuator::SetSpringAndDamping(bool enable, bool /*massIndependent*/,
		Ogre::Real springDamperRelaxation,
		Ogre::Real spring, Ogre::Real damper)
	{
		m_regularizer = static_cast<ndFloat32>(springDamperRelaxation);
		m_pdEnabled = enable;
		m_kp = enable ? static_cast<ndFloat32>(spring) : 0.0f;
		m_kd = enable ? static_cast<ndFloat32>(damper) : 0.0f;

		if (auto* j = asNd())
			j->SetAsSpringDamper(m_regularizer, m_kp, m_kd);

		_wake();
	}

	void HingeActuator::ControllerUpdate(ndFloat32 dt)
	{
		auto* j = asNd();
		if (!j || dt <= ndFloat32(0)) return;

		// 1) Slew-limit the target angle
		const ndFloat32 cur = j->GetAngle();
		const ndFloat32 tgt = clampf(m_targetCmd, m_minRad, m_maxRad);
		const ndFloat32 err = tgt - cur;

		const ndFloat32 maxStep = m_maxOmega * dt;
		const ndFloat32 step = (err < -maxStep) ? -maxStep : (err > maxStep) ? maxStep : err;

		m_targetSlew = cur + step;
		m_targetSlew = clampf(m_targetSlew, m_minRad, m_maxRad);

		// 2) If PD disabled, just set target (hinge will not drive)
		if (!m_pdEnabled)
		{
			j->SetTargetAngle(m_targetSlew);
			return;
		}

		// 3) Soft torque cap by scaling PD gains for this step if needed
		// Desired PD torque about hinge axis:
		const ndFloat32 omega = j->GetOmega(); // relative hinge angular velocity (rad/s)
		ndFloat32 errToTarget = m_targetSlew - cur;     // rad
		ndFloat32 desiredTau = m_kp * errToTarget - m_kd * omega;

		ndFloat32 scale = 1.0f;
		if (m_maxTorque > ndFloat32(0.0f))
		{
			const ndFloat32 absTau = std::fabs(desiredTau);
			if (absTau > m_maxTorque + ndFloat32(1e-6))
				scale = m_maxTorque / absTau;
		}

		const ndFloat32 kpStep = m_kp * scale;
		const ndFloat32 kdStep = m_kd * scale;

		// 4) Apply step gains + new target to the ND hinge
		j->SetAsSpringDamper(m_regularizer, kpStep, kdStep);
		j->SetTargetAngle(m_targetSlew);

		// Optional: wake bodies if we are actively driving
		if (std::fabs(desiredTau) > ndFloat32(1e-6))
			_wake();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	static inline ndFloat32 clampf(ndFloat32 v, ndFloat32 a, ndFloat32 b) { return (v < a) ? a : (v > b) ? b : v; }

	SliderActuator::SliderActuator(const Body* child, const Body* parent,
		const Ogre::Vector3& pos, const Ogre::Vector3& pin,
		Ogre::Real linearRate, Ogre::Real minPosition, Ogre::Real maxPosition)
	{
		m_body0 = const_cast<Body*>(child);
		m_body1 = const_cast<Body*>(parent);

		ndBodyKinematic* b0 = child ? const_cast<ndBodyKinematic*>(child->getNewtonBody()) : nullptr;
		ndBodyKinematic* b1 = parent ? const_cast<ndBodyKinematic*>(parent->getNewtonBody()) : nullptr;

		// Build orthonormal frame from pin at world position pos
		Ogre::Quaternion q = OgreNewt::Converters::grammSchmidt(pin);
		ndMatrix frame;
		OgreNewt::Converters::QuatPosToMatrix(q, pos, frame);

		auto* j = new ndJointSlider(frame, b0, b1);
		SetSupportJoint(j);

		m_minPos = static_cast<ndFloat32>(std::min(minPosition, maxPosition));
		m_maxPos = static_cast<ndFloat32>(std::max(minPosition, maxPosition));
		m_maxSpeed = static_cast<ndFloat32>(std::max<Ogre::Real>(1e-6f, linearRate));

		j->SetLimitState(true);
		j->SetLimits(m_minPos, m_maxPos);

		// default regularizer; PD gains will be chosen in _retuneGains()
		j->SetAsSpringDamper(m_regularizer, 0.0f, 0.0f);

		// initialize targets to current position
		const ndFloat32 p = j->GetPosit();
		m_targetCmd = clampf(p, m_minPos, m_maxPos);
		m_targetSlew = m_targetCmd;
		j->SetTargetPosit(m_targetSlew);

		// program initial max force (abs) and choose gains
		_applyMaxForceToNative();
		_retuneGains();
	}

	SliderActuator::~SliderActuator()
	{
		// base Joint cleans up native joint
	}

	Ogre::Real SliderActuator::GetActuatorPosition() const
	{
		if (auto* j = const_cast<SliderActuator*>(this)->asNd())
			return j->GetPosit();
		return 0.0f;
	}

	void SliderActuator::SetTargetPosition(Ogre::Real targetPosition)
	{
		m_targetCmd = static_cast<ndFloat32>(targetPosition);
		m_targetCmd = clampf(m_targetCmd, m_minPos, m_maxPos);
		_wake();
	}

	void SliderActuator::SetLinearRate(Ogre::Real linearRate)
	{
		m_maxSpeed = static_cast<ndFloat32>(std::max<Ogre::Real>(1e-6f, linearRate));
		_wake();
	}

	void SliderActuator::SetMinPositionLimit(Ogre::Real limit)
	{
		m_minPos = static_cast<ndFloat32>(limit);
		if (auto* j = asNd())
			j->SetLimits(m_minPos, m_maxPos);
		_retuneGains();
		_wake();
	}

	void SliderActuator::SetMaxPositionLimit(Ogre::Real limit)
	{
		m_maxPos = static_cast<ndFloat32>(limit);
		if (auto* j = asNd())
			j->SetLimits(m_minPos, m_maxPos);
		_retuneGains();
		_wake();
	}

	void SliderActuator::SetMinForce(Ogre::Real force)
	{
		m_minForce = static_cast<ndFloat32>(force);
		_applyMaxForceToNative();
		_retuneGains();
		_wake();
	}

	void SliderActuator::SetMaxForce(Ogre::Real force)
	{
		m_maxForce = static_cast<ndFloat32>(force);
		_applyMaxForceToNative();
		_retuneGains();
		_wake();
	}

	Ogre::Real SliderActuator::GetForce() const
	{
		return m_lastForce;
	}

	Ogre::Real SliderActuator::GetTargetPosition() const
	{
		if (auto* j = const_cast<SliderActuator*>(this)->asNd())
			return j->GetTargetPosit();
		return 0.0f;
	}

	Ogre::Real SliderActuator::GetSpeed() const
	{
		if (auto* j = const_cast<SliderActuator*>(this)->asNd())
			return j->GetSpeed();
		return 0.0f;
	}

	void SliderActuator::_applyMaxForceToNative()
	{
		auto* j = asNd();
		if (!j) return;

		// ND4 only supports absolute max force (+ enable state)
		const ndFloat32 absMax = std::max(std::fabs(m_minForce), std::fabs(m_maxForce));
		j->SetMaxForceState(true);
		j->SetMaxForce(absMax);
	}

	void SliderActuator::_retuneGains()
	{
		auto* j = asNd();
		if (!j) return;

		// Pick PD gains so that a full-span error requests roughly the available force.
		// This keeps behavior sane without exposing extra API knobs.
		const ndFloat32 span = std::max(ndFloat32(1e-4f), m_maxPos - m_minPos);
		const ndFloat32 absMax = std::max(std::fabs(m_minForce), std::fabs(m_maxForce));

		// Simple heuristic:
		// kp such that kp * (span * 0.5) ~= 0.7 * absMax  --> moderate stiffness
		const ndFloat32 kp = (absMax > 1e-6f) ? (absMax * 0.7f) / (span * 0.5f) : 0.0f;

		// Critical-ish damping around small mass assumptions: kd ~= 2*sqrt(kp*m_eff); with unknown mass,
		// use a proportional term to rate: larger rate -> more damping, but keep bounded.
		const ndFloat32 kd = kp > 0.0f ? ndFloat32(0.25f) * std::max(ndFloat32(1.0f), m_maxSpeed) : 0.0f;

		m_kp = kp;
		m_kd = kd;

		j->SetAsSpringDamper(m_regularizer, m_kp, m_kd);
	}

	void SliderActuator::ControllerUpdate(ndFloat32 dt)
	{
		auto* j = asNd();
		if (!j || dt <= ndFloat32(0)) return;

		// 1) Slew-limit the target position
		const ndFloat32 cur = j->GetPosit();
		const ndFloat32 tgt = clampf(m_targetCmd, m_minPos, m_maxPos);
		const ndFloat32 err = tgt - cur;
		const ndFloat32 maxDx = m_maxSpeed * dt;
		const ndFloat32 step = (err < -maxDx) ? -maxDx : (err > maxDx) ? maxDx : err;

		m_targetSlew = clampf(cur + step, m_minPos, m_maxPos);

		// 2) Compute PD force request (signed)
		const ndFloat32 speed = j->GetSpeed();
		ndFloat32 desiredF = m_kp * (m_targetSlew - cur) - m_kd * speed;

		// 3) Soft directional clamp
		desiredF = std::min(std::max(desiredF, m_minForce), m_maxForce);
		m_lastForce = desiredF;

		// 4) Program ND4: set gains (already set in _retuneGains), set target pos, and ensure absolute max force guard is on.
		// Note: ND4 applies spring/damper internal drive; we can optionally scale gains per-step to mimic a hard cap,
		// but the soft clamp plus ND4 absolute guard is usually sufficient.
		j->SetAsSpringDamper(m_regularizer, m_kp, m_kd);
		_applyMaxForceToNative();
		j->SetTargetPosit(m_targetSlew);

		// 5) Wake bodies if we’re actively driving
		if (std::fabs(m_lastForce) > ndFloat32(1e-6))
			_wake();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	class ndFlexyPipeSpinner : public ndJointBilateralConstraint
	{
	public:
		ndFlexyPipeSpinner(const ndMatrix& pinAndPivotFrame, ndBodyKinematic* const child, ndBodyKinematic* const parent)
			: ndJointBilateralConstraint(3, child, parent, pinAndPivotFrame)
		{
			SetSolverModel(ndJointBilateralSolverModel::m_jointIterativeSoft);
		}

		void JacobianDerivative(ndConstraintDescritor& desc) override
		{
			ndMatrix m0, m1;
			CalculateGlobalMatrix(m0, m1);

			ApplyTwistAction(desc, m0, m1);
			ApplyElasticConeAction(desc, m0, m1);
		}

		void ApplyTwistAction(ndConstraintDescritor& desc, const ndMatrix& m0, const ndMatrix& m1)
		{
			ndVector pin0 = m0.m_front;
			ndVector pin1 = m1.m_front.Scale(-1.0f);
			ndFloat32 relOmega = m_body0->GetOmega().DotProduct(pin0).GetScalar() + m_body1->GetOmega().DotProduct(pin1).GetScalar();
			ndFloat32 relAccel = -relOmega / desc.m_timestep;
			AddAngularRowJacobian(desc, pin0, 0.0f);
			SetMotorAcceleration(desc, relAccel);
		}

		void ApplyElasticConeAction(ndConstraintDescritor& desc, const ndMatrix& m0, const ndMatrix& m1)
		{
			const ndFloat32 relaxation = 0.01f;
			const ndFloat32 spring = 1000.0f;
			const ndFloat32 damper = 50.0f;
			const ndFloat32 maxConeAngle = ndDegreeToRad * 45.0f;

			ndFloat32 cosAng = m1.m_front.DotProduct(m0.m_front).GetScalar();
			if (cosAng >= 0.998f)
			{
				ndFloat32 a0 = CalculateAngle(m0.m_front, m1.m_front, m1.m_up);
				AddAngularRowJacobian(desc, m1.m_up, a0);
				SetMassSpringDamperAcceleration(desc, relaxation, spring, damper);

				ndFloat32 a1 = CalculateAngle(m0.m_front, m1.m_front, m1.m_right);
				AddAngularRowJacobian(desc, m1.m_right, a1);
				SetMassSpringDamperAcceleration(desc, relaxation, spring, damper);
			}
			else
			{
				ndVector lateralDir = m1.m_front.CrossProduct(m0.m_front);
				ndFloat32 len2 = lateralDir.DotProduct(lateralDir).GetScalar();
				if (len2 > 1.0e-6f)
				{
					lateralDir = lateralDir.Normalize();
					ndFloat32 coneAngle = acos(ndClamp(cosAng, -1.0f, 1.0f));
					if (coneAngle > maxConeAngle)
					{
						AddAngularRowJacobian(desc, lateralDir, maxConeAngle - coneAngle);
						SetHighFriction(desc, 0.0f);
					}
					else
					{
						AddAngularRowJacobian(desc, lateralDir, -coneAngle);
						SetMassSpringDamperAcceleration(desc, relaxation, spring, damper);
					}
				}
			}
		}
	};

	// OgreNewt wrapper
	class _OgreNewtExport FlexyPipeSpinnerJoint : public OgreNewt::Joint
	{
	public:
		FlexyPipeSpinnerJoint(OgreNewt::Body* currentBody, OgreNewt::Body* predecessorBody,
			const Ogre::Vector3& anchorPosition, const Ogre::Vector3& pin)
		{
			ndBodyKinematic* b0 = const_cast<ndBodyKinematic*>(currentBody->getNewtonBody());
			ndBodyKinematic* b1 = predecessorBody ? const_cast<ndBodyKinematic*>(predecessorBody->getNewtonBody()) : nullptr;

			Ogre::Quaternion q = OgreNewt::Converters::grammSchmidt(pin);
			ndMatrix frame;
			OgreNewt::Converters::QuatPosToMatrix(q, anchorPosition, frame);

			auto* joint = new ndFlexyPipeSpinner(frame, b0, b1);
			SetSupportJoint(joint);
		}
	};

	///////////////////////////////////////////////////////////////////////////////////////////////

	ActiveSliderJoint::ActiveSliderJoint(OgreNewt::Body* currentBody,
		OgreNewt::Body* predecessorBody,
		const Ogre::Vector3& anchorPosition,
		const Ogre::Vector3& pin,
		Ogre::Real minStopDistance,
		Ogre::Real maxStopDistance,
		Ogre::Real moveSpeed,
		bool repeat,
		bool directionChange,
		bool activated)
	{
		m_body0 = currentBody;
		m_body1 = predecessorBody;
		m_pin = pin.normalisedCopy();
		m_minStop = minStopDistance;
		m_maxStop = maxStopDistance;
		m_moveSpeed = moveSpeed;
		m_repeat = repeat;
		m_directionChange = directionChange;
		m_activated = activated;
		m_oppositeDir = 1.0f;
		m_round = 0;

		ndBodyKinematic* b0 = currentBody ? const_cast<ndBodyKinematic*>(currentBody->getNewtonBody()) : nullptr;
		ndBodyKinematic* b1 = predecessorBody ? const_cast<ndBodyKinematic*>(predecessorBody->getNewtonBody()) : nullptr;

		Ogre::Quaternion q = OgreNewt::Converters::grammSchmidt(pin);
		ndMatrix frame;
		OgreNewt::Converters::QuatPosToMatrix(q, anchorPosition, frame);

		auto* joint = new ndJointSlider(frame, b0, b1);
		SetSupportJoint(joint);

		joint->SetLimitState(true);
		joint->SetLimits(static_cast<ndFloat32>(m_minStop), static_cast<ndFloat32>(m_maxStop));
		joint->SetAsSpringDamper(0.1f, 1000.0f, 100.0f); // base PD

		if (m_body0)
			m_body0->setAutoSleep(!m_activated);
	}

	ActiveSliderJoint::~ActiveSliderJoint()
	{
	}

	void ActiveSliderJoint::_wake()
	{
		if (m_body0) m_body0->unFreeze();
		if (m_body1) m_body1->unFreeze();
	}

	// ----------------------------------------------------
	void ActiveSliderJoint::setActivated(bool activated)
	{
		m_activated = activated;
		if (m_body0)
			m_body0->setAutoSleep(!activated);
		_wake();
	}

	void ActiveSliderJoint::enableLimits(bool state)
	{
		if (auto* j = asNd())
			j->SetLimitState(state);
		_wake();
	}

	void ActiveSliderJoint::setLimits(Ogre::Real minStopDist, Ogre::Real maxStopDist)
	{
		m_minStop = minStopDist;
		m_maxStop = maxStopDist;
		if (auto* j = asNd())
			j->SetLimits(static_cast<ndFloat32>(m_minStop), static_cast<ndFloat32>(m_maxStop));
		_wake();
	}

	void ActiveSliderJoint::setMoveSpeed(Ogre::Real moveSpeed)
	{
		m_moveSpeed = moveSpeed;
		_wake();
	}

	void ActiveSliderJoint::setRepeat(bool repeat)
	{
		m_repeat = repeat;
		_wake();
	}

	void ActiveSliderJoint::setDirectionChange(bool directionChange)
	{
		m_directionChange = directionChange;
		_wake();
	}

	int ActiveSliderJoint::getCurrentDirection() const
	{
		return static_cast<int>(m_oppositeDir);
	}

	// ----------------------------------------------------
	// ControllerUpdate — replaces SubmitConstraints
	// ----------------------------------------------------
	void ActiveSliderJoint::ControllerUpdate(Ogre::Real dt)
	{
		if (!m_activated || dt <= 0.0f)
			return;

		auto* j = asNd();
		if (!j)
			return;

		const Ogre::Real pos = j->GetPosit();
		const Ogre::Real vel = j->GetSpeed();
		const Ogre::Real targ = j->GetTargetPosit();

		const Ogre::Real posTol = std::max(0.001f, 0.5f * m_moveSpeed * dt);
		const Ogre::Real velTol = std::max(0.001f, 0.1f * m_moveSpeed);

		const bool nearTargetPos = std::abs(pos - targ) <= posTol;
		const bool nearlyStopped = std::abs(vel) <= velTol;
		const bool arrived = nearTargetPos && nearlyStopped;
		bool flippedThisFrame = false;

		// 1) Advance progress
		Ogre::Real newProgress = targ;

		if (m_oppositeDir == 1.0f)
		{
			newProgress += m_moveSpeed * dt;
			if (newProgress >= m_maxStop)
			{
				if (m_directionChange)
					m_oppositeDir *= -1.0f;
				m_round++;
				flippedThisFrame = true;
			}
		}
		else
		{
			newProgress -= m_moveSpeed * dt;
			if (newProgress <= m_minStop)
			{
				if (m_directionChange)
					m_oppositeDir *= -1.0f;
				m_round++;
				flippedThisFrame = true;
			}
		}

		// 2) Handle repeat/stop logic
		if (m_round == 2)
		{
			m_round = 0;
			if (!m_repeat)
				m_directionChange = false;
		}

		// 3) If still active (repeat or directionChange), continue driving
		if (m_repeat || m_directionChange || flippedThisFrame)
		{
			newProgress = std::clamp(newProgress, m_minStop, m_maxStop);
			j->SetTargetPosit(static_cast<ndFloat32>(newProgress));
		}
		else
		{
			// Stop moving — hold current position
			j->SetTargetPosit(static_cast<ndFloat32>(pos));
		}

		// 4) optional: tweak PD each step (optional refinement)
		j->SetAsSpringDamper(0.1f, 1000.0f, 100.0f);

		_wake();
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

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////

	Plane2DUpVectorJoint::Plane2DUpVectorJoint(const OgreNewt::Body* child,
		const Ogre::Vector3& pos,
		const Ogre::Vector3& normal,
		const Ogre::Vector3& pin)
		: Joint(),
		m_pin(pin)
	{
		// Convert vectors to Newton4 format
		const ndVector ndPivot(pos.x, pos.y, pos.z, 1.0f);
		const ndVector ndNormal(normal.x, normal.y, normal.z, 0.0f);

		// Retrieve body references
		ndBodyKinematic* const childBody = const_cast<ndBodyKinematic*>(child->getNewtonBody());
		ndBodyKinematic* const worldBody = nullptr;

		// Create the Newton4 plane joint
		auto* joint = new ndJointPlane(ndPivot, ndNormal, childBody, worldBody);

		// Initially disable rotation control (like old Plane2dUpVector)
		joint->EnableControlRotation(false);

		SetSupportJoint(joint);
	}

	Plane2DUpVectorJoint::~Plane2DUpVectorJoint()
	{
		// nothing, handled by Joint base destructor
	}

	void Plane2DUpVectorJoint::setPin(const Ogre::Vector3& pin)
	{
		m_pin = pin;

		// optional extension: we could modify plane normal or orientation based on pin
		// but in ND4, ndJointPlane only supports fixed normal, not dynamic "pin" updates.
		// This is mainly stored for user code compatibility.
	}

	void Plane2DUpVectorJoint::setEnableControlRotation(bool state)
	{
		if (auto* j = getJoint())
			j->EnableControlRotation(state);
	}

	bool Plane2DUpVectorJoint::getEnableControlRotation() const
	{
		if (auto* j = getJoint())
			return j->GetEnableControlRotation();
		return false;
	}

	/////////////////////////////////////////////////////////////////////////////////////////

	// Internal Newton joint wrapper (hidden inside this translation unit)
	class ndPlaneConstraintWrapper : public ndJointPlane
	{
	public:
		ndPlaneConstraintWrapper(const ndVector& pivot,
			const ndVector& normal,
			ndBodyKinematic* const child,
			ndBodyKinematic* const parent)
			: ndJointPlane(pivot, normal, child, parent)
		{
			// Set to a stable iterative solver
			SetSolverModel(ndJointBilateralSolverModel::m_jointIterativeSoft);

			// Disable control rotation by default for pure planar motion
			EnableControlRotation(false);
		}

		~ndPlaneConstraintWrapper() override = default;
	};

	// -----------------------------------------------------------------------------
	// PlaneConstraint Implementation
	// -----------------------------------------------------------------------------
	PlaneConstraint::PlaneConstraint(const OgreNewt::Body* child,
		const OgreNewt::Body* parent,
		const Ogre::Vector3& pos,
		const Ogre::Vector3& normal)
		: Joint()
	{
		ndBodyKinematic* const b0 = const_cast<ndBodyKinematic*>(child->getNewtonBody());
		ndBodyKinematic* const b1 = parent ? const_cast<ndBodyKinematic*>(parent->getNewtonBody()) : nullptr;

		ndVector ndPos(pos.x, pos.y, pos.z, 1.0f);
		ndVector ndNormal(normal.x, normal.y, normal.z, 0.0f);

		auto* joint = new ndPlaneConstraintWrapper(ndPos, ndNormal, b0, b1);
		SetSupportJoint(joint);
	}

	PlaneConstraint::~PlaneConstraint() = default;

	// -----------------------------------------------------------------------------
	void PlaneConstraint::enableControlRotation(bool enable)
	{
		if (auto* j = static_cast<ndPlaneConstraintWrapper*>(GetSupportJoint()))
		{
			j->EnableControlRotation(enable);
		}
	}

	// -----------------------------------------------------------------------------
	bool PlaneConstraint::getControlRotationEnabled() const
	{
		if (auto* j = static_cast<ndPlaneConstraintWrapper*>(GetSupportJoint()))
		{
			return j->GetEnableControlRotation();
		}
		return false;
	}

	/////////////////////////////////////////////////////////////////////////////////////////

	UpVector::UpVector(const Body* body, const Ogre::Vector3& pin)
		: m_pin(pin)
	{
		ndBodyKinematic* const child = const_cast<ndBodyKinematic*>(body->getNewtonBody());
		ndBodyKinematic* const parent = nullptr; // attach to world sentinel

		// Convert Ogre::Vector3 → ndVector
		const ndVector up(pin.x, pin.y, pin.z, 0.0f);

		// Create the ND4 up-vector joint
		auto* j = new ndJointUpVector(up, child, parent);
		SetSupportJoint(j);
	}

	UpVector::~UpVector()
	{
		// Joint base cleans up automatically
	}

	void UpVector::setPin(const Body* body, const Ogre::Vector3& pin)
	{
		m_pin = pin;

		ndBodyKinematic* const child = const_cast<ndBodyKinematic*>(body->getNewtonBody());
		if (!child)
			return;

		if (auto* j = getJoint())
		{
			// Update the ND4 joint's pin direction in real time
			const ndVector up(pin.x, pin.y, pin.z, 0.0f);
			j->SetPinDir(up);
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////

	KinematicController::KinematicController(const OgreNewt::Body* child, const Ogre::Vector3& pos)
	{
		const ndVector attachment(pos.x, pos.y, pos.z, 1.0f);
		ndBodyKinematic* const body = const_cast<ndBodyKinematic*>(child->getNewtonBody());
		ndBodyKinematic* const worldBody = nullptr;

		auto* joint = new ndJointKinematicController(worldBody, body, attachment);
		SetSupportJoint(joint);

		joint->SetControlMode(ndJointKinematicController::m_full6dof);
	}

	KinematicController::KinematicController(const OgreNewt::Body* child, const Ogre::Vector3& pos, const OgreNewt::Body* referenceBody)
	{
		const ndVector attachment(pos.x, pos.y, pos.z, 1.0f);
		ndBodyKinematic* const body = const_cast<ndBodyKinematic*>(child->getNewtonBody());
		ndBodyKinematic* const refBody = referenceBody ? const_cast<ndBodyKinematic*>(referenceBody->getNewtonBody()) : nullptr;

		auto* joint = new ndJointKinematicController(refBody, body, attachment);
		SetSupportJoint(joint);

		joint->SetControlMode(ndJointKinematicController::m_full6dof);
	}

	KinematicController::~KinematicController()
	{
		// base Joint handles cleanup
	}

	void KinematicController::setPickingMode(int mode)
	{
		if (auto* j = getJoint())
			j->SetControlMode(static_cast<ndJointKinematicController::ndControlModes>(mode));
	}

	int KinematicController::getPickingMode() const
	{
		if (auto* j = getJoint())
			return static_cast<int>(j->GetControlMode());
		return 0;
	}

	void KinematicController::setMaxLinearFriction(Ogre::Real force)
	{
		if (auto* j = getJoint())
			j->SetMaxLinearFriction(static_cast<ndFloat32>(force));
	}

	Ogre::Real KinematicController::getMaxLinearFriction() const
	{
		if (auto* j = getJoint())
			return j->GetMaxLinearFriction();
		return 0.0f;
	}

	void KinematicController::setMaxAngularFriction(Ogre::Real torque)
	{
		if (auto* j = getJoint())
			j->SetMaxAngularFriction(static_cast<ndFloat32>(torque));
	}

	Ogre::Real KinematicController::getMaxAngularFriction() const
	{
		if (auto* j = getJoint())
			return j->GetMaxAngularFriction();
		return 0.0f;
	}

	void KinematicController::setAngularViscousFrictionCoefficient(Ogre::Real coefficient)
	{
		if (auto* j = getJoint())
			j->SetAngularViscousFrictionCoefficient(static_cast<ndFloat32>(coefficient));
	}

	Ogre::Real KinematicController::getAngularViscousFrictionCoefficient() const
	{
		if (auto* j = getJoint())
			return j->GetAngularViscousFrictionCoefficient();
		return 0.0f;
	}

	void KinematicController::setMaxSpeed(Ogre::Real speedInMetersPerSeconds)
	{
		if (auto* j = getJoint())
			j->SetMaxSpeed(static_cast<ndFloat32>(speedInMetersPerSeconds));
	}

	Ogre::Real KinematicController::getMaxSpeed() const
	{
		if (auto* j = getJoint())
			return j->GetMaxSpeed();
		return 0.0f;
	}

	void KinematicController::setMaxOmega(const Ogre::Degree& speedInDegreesPerSeconds)
	{
		if (auto* j = getJoint())
			j->SetMaxOmega(static_cast<ndFloat32>(speedInDegreesPerSeconds.valueRadians()));
	}

	Ogre::Real KinematicController::getMaxOmega() const
	{
		if (auto* j = getJoint())
			return j->GetMaxOmega();
		return 0.0f;
	}

	void KinematicController::setTargetPosit(const Ogre::Vector3& position)
	{
		if (auto* j = getJoint())
			j->SetTargetPosit(ndVector(position.x, position.y, position.z, 1.0f));
	}

	void KinematicController::setTargetRotation(const Ogre::Quaternion& rotation)
	{
		if (auto* j = getJoint())
		{
			ndQuaternion q(rotation.w, rotation.x, rotation.y, rotation.z);
			j->SetTargetRotation(q);
		}
	}

	void KinematicController::setTargetMatrix(const Ogre::Vector3& position, const Ogre::Quaternion& rotation)
	{
		if (auto* j = getJoint())
		{
			ndMatrix matrix;
			OgreNewt::Converters::QuatPosToMatrix(rotation, position, matrix);
			j->SetTargetMatrix(matrix);
		}
	}

	void KinematicController::getTargetMatrix(Ogre::Vector3& position, Ogre::Quaternion& rotation) const
	{
		if (auto* j = getJoint())
		{
			const ndMatrix m(j->GetTargetMatrix());
			OgreNewt::Converters::MatrixToQuatPos(m, rotation, position);
		}
	}

	void KinematicController::setSolverModel(unsigned short /*solverModel*/)
	{
		// Placeholder: ndJointKinematicController doesn’t expose solver model directly
	}

	bool KinematicController::isBilateral() const
	{
		if (auto* j = getJoint())
			return j->IsBilateral();
		return false;
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

	DryRollingFriction::DryRollingFriction(const Body* child,
		const Body* parent,
		Ogre::Real friction)
	{
		ndBodyKinematic* b0 = child ? const_cast<ndBodyKinematic*>(child->getNewtonBody()) : nullptr;
		ndBodyKinematic* b1 = parent ? const_cast<ndBodyKinematic*>(parent->getNewtonBody()) : nullptr;

		// create newton4 dry rolling friction joint
		auto* j = new ndJointDryRollingFriction(b0, b1, static_cast<ndFloat32>(friction));
		SetSupportJoint(j);
	}

	DryRollingFriction::~DryRollingFriction()
	{
		// handled by Joint base
	}

	void DryRollingFriction::setFrictionCoefficient(Ogre::Real coeff)
	{
		if (auto* j = getJoint())
			j->SetFrictionCoefficient(static_cast<ndFloat32>(coeff));
	}

	Ogre::Real DryRollingFriction::getFrictionCoefficient() const
	{
		if (auto* j = getJoint())
			return j->GetFrictionCoefficient();
		return 0.0f;
	}

	void DryRollingFriction::setContactTrail(Ogre::Real trail)
	{
		if (auto* j = getJoint())
			j->SetContactTrail(static_cast<ndFloat32>(trail));
	}

	Ogre::Real DryRollingFriction::getContactTrail() const
	{
		if (auto* j = getJoint())
			return j->GetContactTrail();
		return 0.0f;
	}

	/////////////////////////////////////////////////////////////////////////////////////

	// =============================================================
	//   ND4 Adapter: implements spline query via OgreNewt::Body spline
	// =============================================================
	class NdCustomPathFollowAdapter : public ndJointFollowPath
	{
	public:
		NdCustomPathFollowAdapter(const ndMatrix& pinAndPivotFrame,
			ndBodyDynamic* const child,
			ndBodyDynamic* const pathBody,
			OgreNewt::Body* const pathBodyWrapper)
			: ndJointFollowPath(pinAndPivotFrame, child, pathBody),
			m_pathBodyWrapper(pathBodyWrapper)
		{
		}

		void GetPointAndTangentAtLocation(const ndVector& location,
			ndVector& positOut,
			ndVector& tangentOut) const override
		{
			// Get spline from the OgreNewt path body
			const ndBezierSpline& spline = m_pathBodyWrapper->getSpline();
			const ndBodyKinematic* const pathKine = GetBody1();
			const ndMatrix pathM(pathKine->GetMatrix());

			const ndVector pLocal(pathM.UntransformVector(location));

			ndBigVector closestPoint;
			const ndFloat64 knot = spline.FindClosestKnot(closestPoint, pLocal, 8);
			ndBigVector tangent(spline.CurveDerivative(knot));
			tangent = tangent.Scale(1.0 / ndSqrt(tangent.DotProduct(tangent).GetScalar()));

			positOut = pathM.TransformVector(closestPoint);
			tangentOut = ndVector(ndFloat32(tangent.m_x), ndFloat32(tangent.m_y), ndFloat32(tangent.m_z), 0.0f);
		}

	private:
		OgreNewt::Body* const m_pathBodyWrapper;
	};

	// =============================================================
	// PathFollow implementation
	// =============================================================

	PathFollow::PathFollow(OgreNewt::Body* child, OgreNewt::Body* pathBody, const Ogre::Vector3& pos)
		: m_childBody(child),
		m_pathBody(pathBody),
		m_pos(pos)
	{
	}

	PathFollow::~PathFollow()
	{
		// Base Joint class cleans up joint object
	}

	bool PathFollow::setKnots(const std::vector<Ogre::Vector3>& knots)
	{
		m_internalControlPoints.clear();
		m_internalControlPoints.reserve(knots.size());

		for (const auto& p : knots)
			m_internalControlPoints.emplace_back(ndBigVector(p.x, p.y, p.z, 1.0));

		return !m_internalControlPoints.empty();
	}

	void PathFollow::createPathJoint(void)
	{
		// Build orientation frame from direction
		const Ogre::Vector3 dir = getMoveDirection(0);
		Ogre::Quaternion q = OgreNewt::Converters::grammSchmidt(dir);

		// convert Ogre quaternion + position to ndMatrix
		ndMatrix frame;
		OgreNewt::Converters::QuatPosToMatrix(q, Ogre::Vector3::ZERO, frame);

		// Position joint along the path body transform
		const ndBodyKinematic* pathKine = (ndBodyKinematic*)m_pathBody->getNewtonBody();
		const ndMatrix pathM(pathKine->GetMatrix());
		frame.m_posit = pathM.TransformVector(ndVector(m_pos.x, m_pos.y, m_pos.z, 1.0f));

		ndBodyDynamic* const childDyn = ((ndBodyKinematic*)m_childBody->getNewtonBody())->GetAsBodyDynamic();
		ndBodyDynamic* const pathDyn = ((ndBodyKinematic*)m_pathBody->getNewtonBody())->GetAsBodyDynamic();

		auto* joint = new NdCustomPathFollowAdapter(frame, childDyn, pathDyn, m_pathBody);
		SetSupportJoint(joint);
	}

	ndVector PathFollow::computeDirection(unsigned int index) const
	{
		if (m_internalControlPoints.empty() || index + 1 >= m_internalControlPoints.size())
			return ndVector(1.0f, 0.0f, 0.0f, 0.0f);

		const ndBigVector a = m_internalControlPoints[index + 0];
		const ndBigVector b = m_internalControlPoints[index + 1];
		const ndVector dir((ndFloat32)(b.m_x - a.m_x), (ndFloat32)(b.m_y - a.m_y), (ndFloat32)(b.m_z - a.m_z), 0.0f);

		const ndFloat32 len2 = dir.DotProduct(dir).GetScalar();
		return len2 > 1.0e-6f ? dir.Scale(1.0f / ndSqrt(len2)) : ndVector(1.0f, 0.0f, 0.0f, 0.0f);
	}

	Ogre::Vector3 PathFollow::getMoveDirection(unsigned int index)
	{
		const ndVector v = computeDirection(index);
		return Ogre::Vector3(v.m_x, v.m_y, v.m_z);
	}

	Ogre::Vector3 PathFollow::getCurrentMoveDirection(const Ogre::Vector3& currentBodyPosition)
	{
		if (m_internalControlPoints.empty())
			return Ogre::Vector3::ZERO;

		ndVector pos(currentBodyPosition.x, currentBodyPosition.y, currentBodyPosition.z, 1.0f);

		// Find nearest segment
		ndFloat32 minDist2 = 1e20f;
		ndVector bestDir(1.0f, 0.0f, 0.0f, 0.0f);
		for (size_t i = 0; i + 1 < m_internalControlPoints.size(); ++i)
		{
			const ndVector a(m_internalControlPoints[i]);
			const ndVector b(m_internalControlPoints[i + 1]);
			const ndVector ab = b - a;
			const ndVector ap = pos - a;
			const ndFloat32 t = ndClamp(ap.DotProduct(ab).GetScalar() / ab.DotProduct(ab).GetScalar(), 0.0f, 1.0f);
			const ndVector p = a + ab.Scale(t);
			const ndFloat32 d2 = (pos - p).DotProduct(pos - p).GetScalar();
			if (d2 < minDist2)
			{
				minDist2 = d2;
				bestDir = ab.Scale(1.0f / ndSqrt(ab.DotProduct(ab).GetScalar()));
			}
		}
		return Ogre::Vector3(bestDir.m_x, bestDir.m_y, bestDir.m_z);
	}

	Ogre::Real PathFollow::getPathLength(void)
	{
		if (m_internalControlPoints.size() < 2)
			return 0.0f;

		Ogre::Real length = 0.0f;
		for (size_t i = 1; i < m_internalControlPoints.size(); ++i)
		{
			const ndBigVector a = m_internalControlPoints[i - 1];
			const ndBigVector b = m_internalControlPoints[i];
			const ndBigVector d = b - a;
			length += Ogre::Real(ndSqrt(d.DotProduct(d).GetScalar()));
		}
		return length;
	}

	std::pair<Ogre::Real, Ogre::Vector3> PathFollow::getPathProgressAndPosition(const Ogre::Vector3& currentBodyPosition)
	{
		if (m_internalControlPoints.size() < 2)
			return { 0.0f, currentBodyPosition };

		const ndVector pos(currentBodyPosition.x, currentBodyPosition.y, currentBodyPosition.z, 1.0f);

		Ogre::Real totalLen = 0.0f;
		std::vector<Ogre::Real> segLens;
		segLens.reserve(m_internalControlPoints.size() - 1);

		for (size_t i = 1; i < m_internalControlPoints.size(); ++i)
		{
			const ndBigVector a = m_internalControlPoints[i - 1];
			const ndBigVector b = m_internalControlPoints[i];
			const ndBigVector d = b - a;
			const Ogre::Real l = Ogre::Real(ndSqrt(d.DotProduct(d).GetScalar()));
			segLens.push_back(l);
			totalLen += l;
		}

		Ogre::Real progress = 0.0f;
		Ogre::Vector3 nearest(currentBodyPosition);

		Ogre::Real accum = 0.0f;
		Ogre::Real minDist2 = 1e20f;
		for (size_t i = 0; i + 1 < m_internalControlPoints.size(); ++i)
		{
			const ndVector a(m_internalControlPoints[i]);
			const ndVector b(m_internalControlPoints[i + 1]);
			const ndVector ab = b - a;
			const ndVector ap = pos - a;
			const ndFloat32 t = ndClamp(ap.DotProduct(ab).GetScalar() / ab.DotProduct(ab).GetScalar(), 0.0f, 1.0f);
			const ndVector p = a + ab.Scale(t);
			const ndFloat32 d2 = (pos - p).DotProduct(pos - p).GetScalar();
			if (d2 < minDist2)
			{
				minDist2 = d2;
				nearest = Ogre::Vector3(p.m_x, p.m_y, p.m_z);
				progress = (accum + segLens[i] * t) / totalLen;
			}
			accum += segLens[i];
		}
		return { progress, nearest };
	}


	///////////////////////////////////////////////////////////////////////////////////////////////

	// -----------------------------------------------------------------------------
// Internal ND4 constraint adapter (hidden in this translation unit)
// -----------------------------------------------------------------------------
	class ndJointWormGear : public ndJointBilateralConstraint
	{
	public:
		ndJointWormGear(ndFloat32 gearRatio, const ndVector& rotAxis, const ndVector& linAxis, ndBodyKinematic* const rotBody, ndBodyKinematic* const linBody)
			: ndJointBilateralConstraint(1, rotBody, linBody, ndGetIdentityMatrix())
			, m_gearRatio(gearRatio)
		{
			// Construct local frames for both bodies from their pin axes
			ndMatrix dummy;
			{
				ndMatrix pinFrame0(ndGramSchmidtMatrix(rotAxis));
				CalculateLocalMatrix(pinFrame0, m_localMatrix0, dummy);
				m_localMatrix0.m_posit = ndVector::m_wOne;
			}
			{
				ndMatrix pinFrame1(ndGramSchmidtMatrix(linAxis));
				CalculateLocalMatrix(pinFrame1, dummy, m_localMatrix1);
				m_localMatrix1.m_posit = ndVector::m_wOne;
			}

			// Same solver mode as ndJointGear (stable)
			SetSolverModel(ndJointBilateralSolverModel::m_jointkinematicOpenLoop);
		}

		void JacobianDerivative(ndConstraintDescritor& desc) override
		{
			ndMatrix m0, m1;
			CalculateGlobalMatrix(m0, m1);

			// Add an angular constraint row, then override it
			AddAngularRowJacobian(desc, m0.m_front, ndFloat32(0.0f));

			ndJacobian& J0 = desc.m_jacobian[desc.m_rowsCount - 1].m_jacobianM0;
			ndJacobian& J1 = desc.m_jacobian[desc.m_rowsCount - 1].m_jacobianM1;

			J0.m_linear = ndVector::m_zero;
			J0.m_angular = ndVector::m_zero;
			J1.m_linear = ndVector::m_zero;
			J1.m_angular = ndVector::m_zero;

			// Body0 rotation contributes ω0·axisRot * gearRatio
			J0.m_angular = m0.m_front.Scale(m_gearRatio);

			// Body1 translation contributes v1·axisLin
			J1.m_linear = m1.m_front;

			// Compute current relative speed
			const ndFloat32 w0 = m_body0->GetOmega().DotProduct(m0.m_front).GetScalar();
			const ndFloat32 v1 = m_body1 ? m_body1->GetVelocity().DotProduct(m1.m_front).GetScalar() : 0.0f;
			const ndFloat32 relSpeed = m_gearRatio * w0 + v1;

			// Drive to zero (maintain gear relation)
			SetMotorAcceleration(desc, -relSpeed * desc.m_invTimestep);
		}

	private:
		ndFloat32 m_gearRatio;
	};

	// -----------------------------------------------------------------------------
	// OgreNewt wrapper
	// -----------------------------------------------------------------------------
	using namespace OgreNewt;

	WormGear::WormGear(const OgreNewt::Body* rotationalBody, const OgreNewt::Body* linearBody, const Ogre::Vector3& rotationalPin, const Ogre::Vector3& linearPin, Ogre::Real gearRatio)
		: Joint()
	{
		ndBodyKinematic* bRot = rotationalBody ? const_cast<ndBodyKinematic*>(rotationalBody->getNewtonBody()) : nullptr;
		ndBodyKinematic* bLin = linearBody ? const_cast<ndBodyKinematic*>(linearBody->getNewtonBody()) : nullptr;

		ndVector rotAxis(rotationalPin.x, rotationalPin.y, rotationalPin.z, 0.0f);
		ndVector linAxis(linearPin.x, linearPin.y, linearPin.z, 0.0f);

		auto* joint = new ndJointWormGear(static_cast<ndFloat32>(gearRatio), rotAxis, linAxis, bRot, bLin);
		SetSupportJoint(joint);
	}

	WormGear::~WormGear() = default;

	///////////////////////////////////////////////////////////////////////////////////////////////

	// ----- Internal ND4 constraint adapter (hidden in this translation unit) -----
	class ndJointGearAndSlide : public ndJointBilateralConstraint
	{
	public:
		ndJointGearAndSlide(ndFloat32 gearRatio, ndFloat32 slideRatio, const ndVector& rotPin, ndBodyKinematic* const rotBody,
			const ndVector& linPin, ndBodyKinematic* const linBody)
			: ndJointBilateralConstraint(1, rotBody, linBody, ndGetIdentityMatrix())
			, m_gearRatio(gearRatio)
			, m_slideRatio(slideRatio)
		{
			// Build local frames like ndJointGear: orientation from each pin, no pivot needed.
			ndMatrix dummy;
			{
				ndMatrix pinFrame0(ndGramSchmidtMatrix(rotPin));
				CalculateLocalMatrix(pinFrame0, m_localMatrix0, dummy);
				m_localMatrix0.m_posit = ndVector::m_wOne;
			}
			{
				ndMatrix pinFrame1(ndGramSchmidtMatrix(linPin));
				CalculateLocalMatrix(pinFrame1, dummy, m_localMatrix1);
				m_localMatrix1.m_posit = ndVector::m_wOne;
			}

			// Use kinematic open loop (matches ndJointGear default)
			SetSolverModel(ndJointBilateralSolverModel::m_jointkinematicOpenLoop);
		}

		void JacobianDerivative(ndConstraintDescritor& desc) override
		{
			ndMatrix m0, m1;
			CalculateGlobalMatrix(m0, m1);

			// We create ONE custom bilateral row and then set its Jacobians explicitly.
			// Start from an angular row so ND sets up bookkeeping; then overwrite.
			AddAngularRowJacobian(desc, m0.m_front, ndFloat32(0.0f));

			// Access the last row and edit jacobians
			ndJacobian& J0 = desc.m_jacobian[desc.m_rowsCount - 1].m_jacobianM0;
			ndJacobian& J1 = desc.m_jacobian[desc.m_rowsCount - 1].m_jacobianM1;

			// zero everything first
			J0.m_linear = ndVector::m_zero;
			J0.m_angular = ndVector::m_zero;
			J1.m_linear = ndVector::m_zero;
			J1.m_angular = ndVector::m_zero;

			// angular part on body0 around its pin (scaled by gearRatio)
			//   contributes ω0 · (gearRatio * axis0)
			J0.m_angular = m0.m_front.Scale(m_gearRatio);

			// linear part on body1 along its pin (scaled by slideRatio)
			//   contributes v1 · (slideRatio * axis1)
			J1.m_linear = m1.m_front.Scale(m_slideRatio);

			// Compute current relative “speed” along this constraint to add damping to zero it:
			// rel = gear*ω0·axis0 + slide*v1·axis1
			const ndFloat32 w0 = m_body0->GetOmega().DotProduct(m0.m_front).GetScalar();
			const ndFloat32 v1 = m_body1 ? m_body1->GetVelocity().DotProduct(m1.m_front).GetScalar() : ndFloat32(0.0f);
			const ndFloat32 relSpeed = m_gearRatio * w0 + m_slideRatio * v1;

			// drive that speed to zero this step
			SetMotorAcceleration(desc, -relSpeed * desc.m_invTimestep);
		}

	private:
		ndFloat32 m_gearRatio;   // scales body0 angular axis
		ndFloat32 m_slideRatio;  // scales body1 linear axis
	};

	// ------------------------ OgreNewt wrapper ------------------------
	using namespace OgreNewt;

	CustomGearAndSlide::CustomGearAndSlide(const OgreNewt::Body* rotationalBody, const OgreNewt::Body* linearBody, const Ogre::Vector3& rotationalPin, const Ogre::Vector3& linearPin,
		Ogre::Real gearRatio, Ogre::Real slideRatio)
		: Joint()
	{
		ndBodyKinematic* bRot = rotationalBody ? const_cast<ndBodyKinematic*>(rotationalBody->getNewtonBody()) : nullptr;
		ndBodyKinematic* bLin = linearBody ? const_cast<ndBodyKinematic*>(linearBody->getNewtonBody()) : nullptr;

		ndVector rotPin(rotationalPin.x, rotationalPin.y, rotationalPin.z, 0.0f);
		ndVector linPin(linearPin.x, linearPin.y, linearPin.z, 0.0f);

		auto* joint = new ndJointGearAndSlide(static_cast<ndFloat32>(gearRatio),
			static_cast<ndFloat32>(slideRatio),
			rotPin, bRot, linPin, bLin);
		SetSupportJoint(joint);
	}

	CustomGearAndSlide::~CustomGearAndSlide() = default;

	///////////////////////////////////////////////////////////////////////////////////////////////

	Pulley::Pulley(const OgreNewt::Body* child,
		const OgreNewt::Body* parent,
		const Ogre::Vector3& childPin,
		const Ogre::Vector3& parentPin,
		Ogre::Real pulleyRatio)
		: Joint()
	{
		// convert pins to ndVector
		const ndVector body0Pin(childPin.x, childPin.y, childPin.z, 0.0f);
		const ndVector body1Pin(parentPin.x, parentPin.y, parentPin.z, 0.0f);

		// cast to Newton4 bodies
		ndBodyKinematic* const body0 = child ? const_cast<ndBodyKinematic*>(child->getNewtonBody()) : nullptr;
		ndBodyKinematic* const body1 = parent ? const_cast<ndBodyKinematic*>(parent->getNewtonBody()) : nullptr;

		// create Newton4 pulley joint
		auto* joint = new ndJointPulley(static_cast<ndFloat32>(pulleyRatio), body0Pin, body0, body1Pin, body1);

		SetSupportJoint(joint);
	}

	Pulley::~Pulley()
	{
		// handled by base Joint
	}

	void Pulley::setRatio(Ogre::Real ratio)
	{
		if (auto* j = getJoint())
			j->SetRatio(static_cast<ndFloat32>(ratio));
	}

	Ogre::Real Pulley::getRatio() const
	{
		if (auto* j = getJoint())
			return j->GetRatio();
		return 1.0f;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	UniversalActuator::UniversalActuator(const Body* child, const Body* parent, const Ogre::Vector3& pos,
		const Ogre::Degree& angularRate0, const Ogre::Degree& minAngle0, const Ogre::Degree& maxAngle0,
		const Ogre::Degree& angularRate1, const Ogre::Degree& minAngle1, const Ogre::Degree& maxAngle1)
	{
		m_body0 = const_cast<Body*>(child);
		m_body1 = const_cast<Body*>(parent);

		ndBodyKinematic* b0 = child ? const_cast<ndBodyKinematic*>(child->getNewtonBody()) : nullptr;
		ndBodyKinematic* b1 = parent ? const_cast<ndBodyKinematic*>(parent->getNewtonBody()) : nullptr;

		// Frame: legacy ND3 actuator took identity rotation + pivot at pos.
		// We keep that convention: identity basis, pivot at 'pos'.
		ndMatrix frame(ndGetIdentityMatrix());
		frame.m_posit = ndVector(pos.x, pos.y, pos.z, 1.0f);

		auto* joint = new ndJointDoubleHinge(frame, b0, b1);
		SetSupportJoint(joint);

		// store limits/rates
		m_min0 = toRad(minAngle0);
		m_max0 = toRad(maxAngle0);
		m_min1 = toRad(minAngle1);
		m_max1 = toRad(maxAngle1);
		m_rate0 = std::max<ndFloat32>(toRad(angularRate0), ndFloat32(1e-6f));
		m_rate1 = std::max<ndFloat32>(toRad(angularRate1), ndFloat32(1e-6f));

		// program limits
		joint->SetLimits0(m_min0, m_max0);
		joint->SetLimits1(m_min1, m_max1);

		// default PD off until we pick gains; regularizer kept at 0.1
		joint->SetAsSpringDamper0(m_reg0, 0.0f, 0.0f);
		joint->SetAsSpringDamper1(m_reg1, 0.0f, 0.0f);

		// init targets to current angles (clamped)
		const ndFloat32 a0 = clampf(joint->GetAngle0(), m_min0, m_max0);
		const ndFloat32 a1 = clampf(joint->GetAngle1(), m_min1, m_max1);
		m_cmd0 = m_slew0 = a0;
		m_cmd1 = m_slew1 = a1;

		// set offsets to current (so no initial jump)
		joint->SetOffsetAngle0(m_slew0);
		joint->SetOffsetAngle1(m_slew1);

		_retuneAxisGains();
	}

	UniversalActuator::~UniversalActuator()
	{
		// base Joint cleans native
	}

	// ---------------- Axis 0 API ----------------
	Ogre::Real UniversalActuator::GetActuatorAngle0() const
	{
		if (auto* j = const_cast<UniversalActuator*>(this)->asNd())
			return toDeg(j->GetAngle0());
		return 0.0f;
	}

	void UniversalActuator::SetTargetAngle0(const Ogre::Degree& angle0)
	{
		m_cmd0 = clampf(toRad(angle0), m_min0, m_max0);
		_wake();
	}

	void UniversalActuator::SetMinAngularLimit0(const Ogre::Degree& limit0)
	{
		m_min0 = toRad(limit0);
		if (auto* j = asNd())
			j->SetLimits0(m_min0, m_max0);
		_retuneAxisGains();
		_wake();
	}

	void UniversalActuator::SetMaxAngularLimit0(const Ogre::Degree& limit0)
	{
		m_max0 = toRad(limit0);
		if (auto* j = asNd())
			j->SetLimits0(m_min0, m_max0);
		_retuneAxisGains();
		_wake();
	}

	void UniversalActuator::SetAngularRate0(const Ogre::Degree& rate0)
	{
		m_rate0 = std::max<ndFloat32>(toRad(rate0), ndFloat32(1e-6f));
		_wake();
	}

	void UniversalActuator::SetMaxTorque0(Ogre::Real torque0)
	{
		m_maxTau0 = static_cast<ndFloat32>(std::max<Ogre::Real>(0.0f, torque0));
		_retuneAxisGains();
		_wake();
	}

	// ---------------- Axis 1 API ----------------
	Ogre::Real UniversalActuator::GetActuatorAngle1() const
	{
		if (auto* j = const_cast<UniversalActuator*>(this)->asNd())
			return toDeg(j->GetAngle1());
		return 0.0f;
	}

	void UniversalActuator::SetTargetAngle1(const Ogre::Degree& angle1)
	{
		m_cmd1 = clampf(toRad(angle1), m_min1, m_max1);
		_wake();
	}

	void UniversalActuator::SetMinAngularLimit1(const Ogre::Degree& limit1)
	{
		m_min1 = toRad(limit1);
		if (auto* j = asNd())
			j->SetLimits1(m_min1, m_max1);
		_retuneAxisGains();
		_wake();
	}

	void UniversalActuator::SetMaxAngularLimit1(const Ogre::Degree& limit1)
	{
		m_max1 = toRad(limit1);
		if (auto* j = asNd())
			j->SetLimits1(m_min1, m_max1);
		_retuneAxisGains();
		_wake();
	}

	void UniversalActuator::SetAngularRate1(const Ogre::Degree& rate1)
	{
		m_rate1 = std::max<ndFloat32>(toRad(rate1), ndFloat32(1e-6f));
		_wake();
	}

	void UniversalActuator::SetMaxTorque1(Ogre::Real torque1)
	{
		m_maxTau1 = static_cast<ndFloat32>(std::max<Ogre::Real>(0.0f, torque1));
		_retuneAxisGains();
		_wake();
	}

	Ogre::Real UniversalActuator::GetTargetAngle0() const
	{
		if (auto* j = const_cast<UniversalActuator*>(this)->asNd())
			return Ogre::Radian(j->GetOffsetAngle0()).valueDegrees();
		return 0.0f;
	}
	Ogre::Real UniversalActuator::GetOmega0() const
	{
		if (auto* j = const_cast<UniversalActuator*>(this)->asNd())
			return Ogre::Radian(j->GetOmega0()).valueDegrees();
		return 0.0f;
	}

	Ogre::Real UniversalActuator::GetTargetAngle1() const
	{
		if (auto* j = const_cast<UniversalActuator*>(this)->asNd())
			return Ogre::Radian(j->GetOffsetAngle1()).valueDegrees();
		return 0.0f;
	}

	Ogre::Real UniversalActuator::GetOmega1() const
	{
		if (auto* j = const_cast<UniversalActuator*>(this)->asNd())
			return Ogre::Radian(j->GetOmega1()).valueDegrees();
		return 0.0f;
	}

	// --------------- gains heuristic ---------------
	void UniversalActuator::_retuneAxisGains()
	{
		auto* j = asNd();
		if (!j) return;

		// Choose KP so that half-span error ~ 0.7 * maxTorque (per axis).
		const ndFloat32 span0 = std::max(ndFloat32(1e-4f), m_max0 - m_min0);
		const ndFloat32 span1 = std::max(ndFloat32(1e-4f), m_max1 - m_min1);

		m_kp0 = (m_maxTau0 > 1e-6f) ? (m_maxTau0 * 0.7f) / (span0 * 0.5f) : 0.0f;
		m_kp1 = (m_maxTau1 > 1e-6f) ? (m_maxTau1 * 0.7f) / (span1 * 0.5f) : 0.0f;

		// Simple damping heuristic tied to rate; keep bounded & nonzero if kp>0
		m_kd0 = (m_kp0 > 0.0f) ? ndFloat32(0.25f) * std::max(m_rate0, ndFloat32(1.0f)) : 0.0f;
		m_kd1 = (m_kp1 > 0.0f) ? ndFloat32(0.25f) * std::max(m_rate1, ndFloat32(1.0f)) : 0.0f;

		j->SetAsSpringDamper0(m_reg0, m_kp0, m_kd0);
		j->SetAsSpringDamper1(m_reg1, m_kp1, m_kd1);
	}

	// --------------- per-step drive ---------------
	void UniversalActuator::ControllerUpdate(ndFloat32 dt)
	{
		auto* j = asNd();
		if (!j || dt <= ndFloat32(0)) return;

		// Axis 0
		{
			const ndFloat32 a = j->GetAngle0();
			const ndFloat32 w = j->GetOmega0();
			const ndFloat32 tg = clampf(m_cmd0, m_min0, m_max0);

			const ndFloat32 err = tg - a;
			const ndFloat32 maxStep = m_rate0 * dt;
			const ndFloat32 step = (err < -maxStep) ? -maxStep : (err > maxStep) ? maxStep : err;
			m_slew0 = clampf(a + step, m_min0, m_max0);

			// desired torque from base gains
			ndFloat32 tau = m_kp0 * (m_slew0 - a) - m_kd0 * w;

			// soft clamp by scaling gains
			ndFloat32 scale = 1.0f;
			if (m_maxTau0 > ndFloat32(0.0f))
			{
				const ndFloat32 absTau = std::fabs(tau);
				if (absTau > m_maxTau0 + ndFloat32(1e-6f))
					scale = m_maxTau0 / absTau;
			}

			const ndFloat32 kpS = m_kp0 * scale;
			const ndFloat32 kdS = m_kd0 * scale;

			j->SetAsSpringDamper0(m_reg0, kpS, kdS);
			j->SetOffsetAngle0(m_slew0);
		}

		// Axis 1
		{
			const ndFloat32 a = j->GetAngle1();
			const ndFloat32 w = j->GetOmega1();
			const ndFloat32 tg = clampf(m_cmd1, m_min1, m_max1);

			const ndFloat32 err = tg - a;
			const ndFloat32 maxStep = m_rate1 * dt;
			const ndFloat32 step = (err < -maxStep) ? -maxStep : (err > maxStep) ? maxStep : err;
			m_slew1 = clampf(a + step, m_min1, m_max1);

			ndFloat32 tau = m_kp1 * (m_slew1 - a) - m_kd1 * w;

			ndFloat32 scale = 1.0f;
			if (m_maxTau1 > ndFloat32(0.0f))
			{
				const ndFloat32 absTau = std::fabs(tau);
				if (absTau > m_maxTau1 + ndFloat32(1e-6f))
					scale = m_maxTau1 / absTau;
			}

			const ndFloat32 kpS = m_kp1 * scale;
			const ndFloat32 kdS = m_kd1 * scale;

			j->SetAsSpringDamper1(m_reg1, kpS, kdS);
			j->SetOffsetAngle1(m_slew1);
		}

		// wake if driving
		_wake();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	Motor::Motor(const Body* child,
		const Body* parent0,
		const Body* parent1,
		const Ogre::Vector3& pin0,
		const Ogre::Vector3& pin1)
		: Hinge(child, parent0, Ogre::Vector3::ZERO, pin0)
		, m_isMotor2(false)
	{
		if (parent1)
			m_isMotor2 = true;

		m_targetSpeed0 = 0.0f;
		m_targetSpeed1 = 0.0f;
		m_torque0 = 0.0f;
		m_torque1 = 0.0f;
	}

	Motor::~Motor() = default;

	// ---------------------------------------------------------------------
	Ogre::Real Motor::GetSpeed0() const
	{
		// reuse base Hinge::GetJointAngularVelocity()
		return Ogre::Real(GetJointAngularVelocity().valueRadians());
	}

	Ogre::Real Motor::GetSpeed1() const
	{
		if (m_isMotor2)
			return m_targetSpeed1;
		return 0.0f;
	}

	// ---------------------------------------------------------------------
	void Motor::SetSpeed0(Ogre::Real speed0)
	{
		m_targetSpeed0 = speed0;

		if (auto* j = asNd())
		{
			// Compute acceleration toward desired speed
			const ndFloat32 accel = static_cast<ndFloat32>(speed0 * 60.0f); // heuristically scaled
			ndConstraintDescritor desc{};
			j->SetMotorAcceleration(desc, accel);
		}
	}

	// ---------------------------------------------------------------------
	void Motor::SetSpeed1(Ogre::Real speed1)
	{
		m_targetSpeed1 = speed1;
		// optional 2-motor case could be extended later
	}

	// ---------------------------------------------------------------------
	void Motor::SetTorque0(Ogre::Real torque0)
	{
		m_torque0 = torque0;

		if (auto* j = asNd())
		{
			// Emulate motor torque as viscous friction around hinge axis
			const ndFloat32 reg = static_cast<ndFloat32>(m_lastRegularizer);
			const ndFloat32 damper = static_cast<ndFloat32>(std::fabs(torque0));
			j->SetAsSpringDamper(reg, 0.0f, damper);
		}
	}

	// ---------------------------------------------------------------------
	void Motor::SetTorque1(Ogre::Real torque1)
	{
		m_torque1 = torque1;
		// optional: dual hinge variant can apply similar SetAsSpringDamper
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	ServoHinge::ServoHinge(const Body* child, const Body* parent,
		const Ogre::Vector3& pos, const Ogre::Vector3& pin)
		: Hinge(child, parent, pos, pin),
		m_targetAngle(0.0f),
		m_stiffness(500.0f),
		m_damping(50.0f)
	{
	}

	ServoHinge::~ServoHinge() = default;

	void ServoHinge::SetTargetAngle(Ogre::Radian angle)
	{
		m_targetAngle = angle;
		if (m_body0) m_body0->unFreeze();
		if (m_body1) m_body1->unFreeze();
	}

	void ServoHinge::SetSpringDamper(Ogre::Real stiffness, Ogre::Real damping)
	{
		m_stiffness = stiffness;
		m_damping = damping;
		if (m_body0) m_body0->unFreeze();
		if (m_body1) m_body1->unFreeze();
	}

	void ServoHinge::Update(Ogre::Real timestep)
	{
		if (auto* j = asNd())
		{
			const ndFloat32 dt = static_cast<ndFloat32>(timestep);
			const ndFloat32 ks = static_cast<ndFloat32>(m_stiffness);
			const ndFloat32 kd = static_cast<ndFloat32>(m_damping);

			// current relative angle
			const Ogre::Real current = GetJointAngle().valueRadians();
			const Ogre::Real error = m_targetAngle.valueRadians() - current;
			const Ogre::Real velocity = GetJointAngularVelocity().valueRadians();

			// apply spring-damper acceleration
			const ndFloat32 accel = j->CalculateSpringDamperAcceleration(dt, ks, static_cast<ndFloat32>(error), kd, static_cast<ndFloat32>(velocity));

			ndConstraintDescritor desc{};
			j->SetMotorAcceleration(desc, accel);
		}
	}

#if 0
	// Example: Create new Joint in engine!

	auto* servo = new OgreNewt::ServoHinge(doorBody, wallBody,
		doorPos, Ogre::Vector3::UNIT_Y);
	servo->SetSpringDamper(1000.0f, 80.0f);
	servo->SetTargetAngle(Ogre::Degree(90));

	...

	servo->Update(deltaTime); // called per frame
#endif

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	Wheel::Wheel(const OgreNewt::Body* child,
		const OgreNewt::Body* parent,
		const Ogre::Vector3& pos,
		const Ogre::Vector3& pin)
		: Joint()
	{
		// build pin matrix using our existing converter
		Ogre::Quaternion q = OgreNewt::Converters::grammSchmidt(pin);
		ndMatrix pinAndPivotFrame;
		OgreNewt::Converters::QuatPosToMatrix(q, pos, pinAndPivotFrame);

		// basic wheel descriptor
		ndWheelDescriptor desc;

		ndBodyKinematic* const childBody = const_cast<ndBodyKinematic*>(child->getNewtonBody());
		ndBodyKinematic* const parentBody = parent ? const_cast<ndBodyKinematic*>(parent->getNewtonBody()) : nullptr;

		auto* joint = new ndJointWheel(pinAndPivotFrame, childBody, parentBody, desc);
		SetSupportJoint(joint);
	}

	Wheel::Wheel(const OgreNewt::Body* child,
		const OgreNewt::Body* parent,
		const Ogre::Vector3& childPos,
		const Ogre::Vector3& childPin,
		const Ogre::Vector3& parentPos,
		const Ogre::Vector3& parentPin)
		: Joint()
	{
		Ogre::Quaternion qChild = OgreNewt::Converters::grammSchmidt(childPin);
		Ogre::Quaternion qParent = OgreNewt::Converters::grammSchmidt(parentPin);

		ndMatrix frameChild;
		ndMatrix frameParent;

		OgreNewt::Converters::QuatPosToMatrix(qChild, childPos, frameChild);
		OgreNewt::Converters::QuatPosToMatrix(qParent, parentPos, frameParent);

		ndWheelDescriptor desc;

		ndBodyKinematic* const childBody = const_cast<ndBodyKinematic*>(child->getNewtonBody());
		ndBodyKinematic* const parentBody = parent ? const_cast<ndBodyKinematic*>(parent->getNewtonBody()) : nullptr;

		auto* joint = new ndJointWheel(frameChild, childBody, parentBody, desc);
		SetSupportJoint(joint);
	}

	Wheel::~Wheel()
	{
		// automatic via base Joint destructor
	}

	Ogre::Real Wheel::GetSteerAngle() const
	{
		if (auto* j = getJoint())
			return j->GetSteering();
		return 0.0f;
	}

	void Wheel::SetTargetSteerAngle(const Ogre::Degree& targetAngle)
	{
		if (auto* j = getJoint())
			j->SetSteering(static_cast<ndFloat32>(targetAngle.valueRadians()));
	}

	Ogre::Real Wheel::GetTargetSteerAngle() const
	{
		if (auto* j = getJoint())
			return j->GetSteering();
		return 0.0f;
	}

	void Wheel::SetSteerRate(Ogre::Real steerRate)
	{
		// ND4’s ndJointWheel doesn’t yet expose SetSteerRate — emulate via SetSteering.
		if (auto* j = getJoint())
			j->SetSteering(static_cast<ndFloat32>(steerRate));
	}

	Ogre::Real Wheel::GetSteerRate() const
	{
		// placeholder; ND4 doesn’t expose internal rate variable.
		if (auto* j = getJoint())
			return j->GetSteering();
		return 0.0f;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	VehicleTire::VehicleTire(OgreNewt::Body* child,
		OgreNewt::Body* parentBody,
		const Ogre::Vector3& pos,
		const Ogre::Vector3& pin,
		Vehicle* parent,
		Ogre::Real radius)
		: Joint(),
		m_vehicle(parent),
		m_radius(radius),
		m_tireSide(tsTireSideA),
		m_tireSteer(tsNoSteer),
		m_steerSide(tsSteerSideA),
		m_tireAccel(tsNoAccel),
		m_brakeMode(tsNoBrake)
	{
		Ogre::Quaternion q = OgreNewt::Converters::grammSchmidt(pin);
		ndMatrix pinAndPivotFrame;
		OgreNewt::Converters::QuatPosToMatrix(q, pos, pinAndPivotFrame);

		// setup default descriptor similar to RayCastTire behavior
		m_desc.m_springK = 2000.0f;
		m_desc.m_damperC = 50.0f;
		m_desc.m_upperStop = radius * 0.2f;
		m_desc.m_lowerStop = -radius * 0.1f;
		m_desc.m_regularizer = 0.1f;
		m_desc.m_brakeTorque = 0.0f;
		m_desc.m_steeringAngle = 0.0f;
		m_desc.m_handBrakeTorque = 0.0f;

		ndBodyKinematic* const childBody = const_cast<ndBodyKinematic*>(child->getNewtonBody());
		ndBodyKinematic* const parentBodyK = parentBody ? const_cast<ndBodyKinematic*>(parentBody->getNewtonBody()) : nullptr;

		auto* joint = new ndJointWheel(pinAndPivotFrame, childBody, parentBodyK, m_desc);
		SetSupportJoint(joint);
	}

	VehicleTire::~VehicleTire()
	{
	}

	void VehicleTire::setVehicleTireSide(VehicleTireSide tireSide)
	{
		m_tireSide = tireSide;
	}

	VehicleTireSide VehicleTire::getVehicleTireSide() const
	{
		return m_tireSide;
	}

	void VehicleTire::setVehicleTireSteer(VehicleTireSteer tireSteer)
	{
		m_tireSteer = tireSteer;
	}

	VehicleTireSteer VehicleTire::getVehicleTireSteer() const
	{
		return m_tireSteer;
	}

	void VehicleTire::setVehicleSteerSide(VehicleSteerSide steerSide)
	{
		m_steerSide = steerSide;
	}

	VehicleSteerSide VehicleTire::getVehicleSteerSide() const
	{
		return m_steerSide;
	}

	void VehicleTire::setVehicleTireAccel(VehicleTireAccel tireAccel)
	{
		m_tireAccel = tireAccel;
	}

	VehicleTireAccel VehicleTire::getVehicleTireAccel() const
	{
		return m_tireAccel;
	}

	void VehicleTire::setVehicleTireBrake(VehicleTireBrake brakeMode)
	{
		m_brakeMode = brakeMode;
	}

	VehicleTireBrake VehicleTire::getVehicleTireBrake() const
	{
		return m_brakeMode;
	}

	void VehicleTire::setLateralFriction(Ogre::Real lateralFriction)
	{
		m_desc.m_regularizer = static_cast<ndFloat32>(lateralFriction);
		if (auto* j = getJoint()) j->SetInfo(m_desc);
	}

	Ogre::Real VehicleTire::getLateralFriction() const
	{
		return m_desc.m_regularizer;
	}

	void VehicleTire::setLongitudinalFriction(Ogre::Real longitudinalFriction)
	{
		// For now, use same field (Newton 4 has no separate friction in ndJointWheel)
		m_desc.m_regularizer = static_cast<ndFloat32>(longitudinalFriction);
		if (auto* j = getJoint()) j->SetInfo(m_desc);
	}

	Ogre::Real VehicleTire::getLongitudinalFriction() const
	{
		return m_desc.m_regularizer;
	}

	void VehicleTire::setSpringLength(Ogre::Real springLength)
	{
		// Map to upper/lower stops
		m_desc.m_upperStop = springLength * 0.5f;
		m_desc.m_lowerStop = -springLength * 0.5f;
		if (auto* j = getJoint()) j->SetInfo(m_desc);
	}

	Ogre::Real VehicleTire::getSpringLength() const
	{
		return m_desc.m_upperStop - m_desc.m_lowerStop;
	}

	void VehicleTire::setSmass(Ogre::Real smass)
	{
		// store but no direct ND4 mapping yet
		m_desc.m_regularizer = static_cast<ndFloat32>(smass);
	}

	Ogre::Real VehicleTire::getSmass() const
	{
		return m_desc.m_regularizer;
	}

	void VehicleTire::setSpringConst(Ogre::Real springConst)
	{
		m_desc.m_springK = static_cast<ndFloat32>(springConst);
		if (auto* j = getJoint()) j->SetInfo(m_desc);
	}

	Ogre::Real VehicleTire::getSpringConst() const
	{
		return m_desc.m_springK;
	}

	void VehicleTire::setSpringDamp(Ogre::Real springDamp)
	{
		m_desc.m_damperC = static_cast<ndFloat32>(springDamp);
		if (auto* j = getJoint()) j->SetInfo(m_desc);
	}

	Ogre::Real VehicleTire::getSpringDamp() const
	{
		return m_desc.m_damperC;
	}

	RayCastTire* VehicleTire::getRayCastTire()
	{
		return (RayCastTire*)GetSupportJoint();
	}

}   // end NAMESPACE OgreNewt
#include "OgreNewt_Stdafx.h"
#include "OgreNewt_World.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_BasicJoints.h"

namespace
{
	ndFloat32 toRad(Ogre::Degree d)
	{
		return static_cast<ndFloat32>(Ogre::Radian(d).valueRadians());
	}

	Ogre::Real toDeg(ndFloat32 r)
	{
		return Ogre::Radian(r).valueDegrees();
	}

	ndFloat32 clampf(ndFloat32 v, ndFloat32 a, ndFloat32 b)
	{
		return (v < a) ? a : (v > b) ? b : v;
	}

	void BuildMotorFrame(const Ogre::Vector3& pin0, const Ogre::Vector3& pin1, const Ogre::Vector3& pos, ndMatrix& outMatrix)
	{
		Ogre::Vector3 fwd = pin0;
		if (fwd.isZeroLength())
			fwd = Ogre::Vector3::UNIT_Z;
		fwd.normalise();

		Ogre::Vector3 up = pin1;
		if (up.isZeroLength())
			up = Ogre::Vector3::UNIT_Y;

		// make sure up is not parallel to fwd
		if (Ogre::Math::Abs(fwd.dotProduct(up)) > 0.999f)
			up = Ogre::Vector3::UNIT_X;

		Ogre::Vector3 right = fwd.crossProduct(up);
		right.normalise();
		up = right.crossProduct(fwd);
		up.normalise();

		Ogre::Matrix3 m3;
		// Ogre uses (right, up, forward) as axes
		m3.FromAxes(right, up, fwd);
		Ogre::Quaternion q(m3);

		OgreNewt::Converters::QuatPosToMatrix(q, pos, outMatrix);
		outMatrix.m_posit = ndVector(pos.x, pos.y, pos.z, ndFloat32(1.0f));
	}

	OgreNewt::World* getWorldFromBodies(const OgreNewt::Body* body0, const OgreNewt::Body* body1)
	{
		if (body0)
			return body0->getWorld();
		if (body1)
			return body1->getWorld();
		return nullptr;
	}

	ndFloat32 OGRENEWT_LARGE_TORQUE = ndFloat32(1.0e8f);
	const ndFloat32 OGRENEWT_LARGE_FORCE = ndFloat32(1.0e8f);
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
		ndBodyKinematic* b1 = parent ? const_cast<ndBodyKinematic*>(parent->getNewtonBody()) : child->getWorld()->getNewtonWorld()->GetSentinelBody();

		ndMatrix pivot(ndGetIdentityMatrix());
		pivot.m_posit = ndVector(pos.x, pos.y, pos.z, 1.0f);

		ndJointSpherical* joint = new ndJointSpherical(pivot, b0, b1);

		// This is key: prefer the iterative soft model for stability, else whole simulation can become unstable!
		// joint->SetSolverModel(m_jointIterativeSoft);
		joint->SetSolverModel(m_jointkinematicOpenLoop);
		
		SetSupportJoint(child->getWorld(), joint);
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
		if (!m_world)
			return;

		ndJointSpherical* joint = asNd();
		if (!joint)
			return;

		const ndFloat32 minRad = static_cast<ndFloat32>(Ogre::Radian(minAngle).valueRadians());
		const ndFloat32 maxRad = static_cast<ndFloat32>(Ogre::Radian(maxAngle).valueRadians());

		joint->SetTwistLimits(minRad, maxRad);

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
		{
			j->SetConeLimit(static_cast<ndFloat32>(Ogre::Radian(maxAngle).valueRadians()));
		}
			
		if (m_child)
			m_child->unFreeze();
		if (m_parent)
			m_parent->unFreeze();
	}

	void BallAndSocket::setTwistFriction(Ogre::Real frictionTorque)
	{
		m_twistFriction = frictionTorque;
		// if (auto* j = asNd())
		// 	j->SetAsSpringDamper(0.01f, 0.0f, static_cast<ndFloat32>(frictionTorque)); // mimic friction

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
		// if (auto* j = asNd())
		// 	j->SetAsSpringDamper(0.01f, 0.0f, static_cast<ndFloat32>(frictionTorque));

		if (m_child)
			m_child->unFreeze();
		if (m_parent)
			m_parent->unFreeze();
	}

	Ogre::Real BallAndSocket::getConeFriction(Ogre::Real) const
	{
		return m_coneFriction;
	}

	void BallAndSocket::setTwistSpringDamper(bool state, Ogre::Real springDamperRelaxation, Ogre::Real spring, Ogre::Real damper)
	{
		if (!m_world)
			return;

		ndJointSpherical* joint = asNd();
		if (!joint)
			return;

		const ndFloat32 relax = static_cast<ndFloat32>(springDamperRelaxation);
		const ndFloat32 k = state ? static_cast<ndFloat32>(spring) : 0.0f;
		const ndFloat32 c = state ? static_cast<ndFloat32>(damper) : 0.0f;

		joint->SetAsSpringDamper(relax, k, c);

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
		ndVector f1 = m_parent ? m_parent->getNewtonBody()->GetForce() : ndVector(0.0f, 0.0f, 0.0f, 0.0f);
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
		ndVector w1 = m_parent ? m_parent->getNewtonBody()->GetOmega() : ndVector(0.0f, 0.0f, 0.0f, 0.0f);
		ndVector diff = w0 - w1;
		return Ogre::Vector3(diff.m_x, diff.m_y, diff.m_z);
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	Hinge::Hinge(const Body* child, const Body* parent, const Ogre::Vector3& pos, const Ogre::Vector3& pin)
		: Joint(),
		m_childPinLocal(pin)
	{
		// build ND4 frame from Ogre pin + position (same as Newton3 grammSchmidt logic)
		const Ogre::Quaternion q = OgreNewt::Converters::grammSchmidt(pin);
		ndMatrix pivotFrame(ndGetIdentityMatrix());
		OgreNewt::Converters::QuatPosToMatrix(q, pos, pivotFrame);

		// resolve bodies (use sentinel if no parent)
		ndBodyKinematic* const childBody = child ? const_cast<ndBodyKinematic*>(child->getNewtonBody()) : nullptr;
		ndBodyKinematic* const parentBody = parent ? const_cast<ndBodyKinematic*>(parent->getNewtonBody())
			: (child ? child->getWorld()->getNewtonWorld()->GetSentinelBody() : nullptr);

		// create actual ND4 joint
		ndJointHinge* const joint = new ndJointHinge(pivotFrame, childBody, parentBody);

		// This is key: prefer the iterative soft model for stability, else whole simulation can become unstable!
		joint->SetSolverModel(m_jointIterativeSoft);
		// store for OgreNewt lifetime mgmt
		SetSupportJoint(child->getWorld(), joint);
	}

	Hinge::Hinge(const Body* child, const Body* parent, const Ogre::Vector3& childPos, const Ogre::Vector3& childPin, const Ogre::Vector3& parentPos, const Ogre::Vector3& parentPin)
		: Joint(),
		m_childPinLocal(childPin)
	{
		// convert child frame
		Ogre::Quaternion qChild = OgreNewt::Converters::grammSchmidt(childPin);
		ndMatrix frameChild(ndGetIdentityMatrix());
		OgreNewt::Converters::QuatPosToMatrix(qChild, childPos, frameChild);

		// convert parent frame
		Ogre::Quaternion qParent = OgreNewt::Converters::grammSchmidt(parentPin);
		ndMatrix frameParent(ndGetIdentityMatrix());
		OgreNewt::Converters::QuatPosToMatrix(qParent, parentPos, frameParent);

		ndBodyKinematic* const childBody = child ? const_cast<ndBodyKinematic*>(child->getNewtonBody()) : nullptr;
		ndBodyKinematic* const parentBody = parent ? const_cast<ndBodyKinematic*>(parent->getNewtonBody())
			: (child ? child->getWorld()->getNewtonWorld()->GetSentinelBody() : nullptr);

		// ND4 hinge can also be created from separate frames
		ndJointHinge* const joint = new ndJointHinge(frameChild, frameParent, childBody, parentBody);

		SetSupportJoint(child->getWorld(), joint);
	}

	Hinge::~Hinge()
	{
		// Joint base handles clearing the native pointer/ownership integration
	}

	void Hinge::EnableLimits(bool state)
	{
		m_limitsEnabled = state;

		if (!m_world)
			return;

		ndJointHinge* joint = asNd();
		if (!joint)
			return;

		const ndInt32 enable = state ? 1 : 0;

		joint->SetLimitState(enable);

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
			const ndVector w1 = (m_body1 ? m_body1->getNewtonBody()->GetOmega() : ndVector(0.0f, 0.0f, 0.0f, 0.0f));
			const Ogre::Vector3 wRel(w0.m_x - w1.m_x, w0.m_y - w1.m_y, w0.m_z - w1.m_z);

			const Ogre::Real omega = wRel.dotProduct(pinWorld);
			return Ogre::Radian(omega);
		}
		return Ogre::Radian(0.0f);
	}

	Ogre::Vector3 Hinge::GetJointPin() const
	{
		return m_childPinLocal;
	}

	void Hinge::SetTorque(Ogre::Real torque)
	{
		m_motorTorque = torque;

		if (!m_world)
			return;

		ndJointHinge* joint = asNd();
		if (!joint)
			return;

		const ndFloat32 reg = static_cast<ndFloat32>(m_lastRegularizer);
		const ndFloat32 spring = 0.0f;
		const ndFloat32 damper = static_cast<ndFloat32>(std::fabs(torque));

		joint->SetAsSpringDamper(reg, spring, damper);

		if (m_body0)
			m_body0->unFreeze();
		if (m_body1)
			m_body1->unFreeze();
	}

	void Hinge::SetSpringAndDamping(bool enable, bool massIndependent, Ogre::Real springDamperRelaxation, Ogre::Real spring, Ogre::Real damper)
	{
		m_lastRegularizer = springDamperRelaxation;

		if (!m_world)
			return;

		ndJointHinge* joint = asNd();
		if (!joint)
			return;

		const ndFloat32 relax = static_cast<ndFloat32>(springDamperRelaxation);
		const ndFloat32 k = enable ? static_cast<ndFloat32>(spring) : 0.0f;
		const ndFloat32 c = enable ? static_cast<ndFloat32>(damper) : 0.0f;

		joint->SetAsSpringDamper(relax, k, c);

		if (m_body0)
			m_body0->unFreeze();

		if (m_body1)
			m_body1->unFreeze();
	}

	void Hinge::SetFriction(Ogre::Real friction)
	{
		m_setFriction = friction;

		if (!m_world)
			return;

		ndJointHinge* joint = asNd();
		if (!joint)
			return;

		const ndFloat32 reg = static_cast<ndFloat32>(m_lastRegularizer);
		const ndFloat32 spring = 0.0f;
		const ndFloat32 damper = static_cast<ndFloat32>(friction);

		joint->SetAsSpringDamper(reg, spring, damper);

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
		ndBodyKinematic* b1 = parent ? const_cast<ndBodyKinematic*>(parent->getNewtonBody()) : child->getWorld()->getNewtonWorld()->GetSentinelBody();

		// use converter to build ndMatrix
		Ogre::Quaternion q = OgreNewt::Converters::grammSchmidt(pin);
		ndMatrix frame;
		OgreNewt::Converters::QuatPosToMatrix(q, pos, frame);

		auto* joint = new ndJointSlider(frame, b0, b1);

		// This is key: prefer the iterative soft model for stability, else whole simulation can become unstable!
		joint->SetSolverModel(m_jointIterativeSoft);

		SetSupportJoint(child->getWorld(), joint);
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
		ndBodyKinematic* b1 = parent ? const_cast<ndBodyKinematic*>(parent->getNewtonBody()) : child->getWorld()->getNewtonWorld()->GetSentinelBody();

		// Convert pin directions
		ndVector pin0(childPin.x, childPin.y, childPin.z, 0.0f);
		ndVector pin1(parentPin.x, parentPin.y, parentPin.z, 0.0f);

		// Create the Newton 4 joint
		ndJointGear* joint = new ndJointGear(static_cast<ndFloat32>(gearRatio), pin0, b0, pin1, b1);
		SetSupportJoint(child->getWorld(), joint);
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
		ndBodyKinematic* b1 = parent ? const_cast<ndBodyKinematic*>(parent->getNewtonBody()) : child->getWorld()->getNewtonWorld()->GetSentinelBody();

		// Build frame from your canonical converter
		Ogre::Quaternion q = OgreNewt::Converters::grammSchmidt(pin);
		ndMatrix frame;
		OgreNewt::Converters::QuatPosToMatrix(q, pos, frame);

		auto* joint = new NdUniversalAdapter(frame, b0, b1);
		SetSupportJoint(child->getWorld(), joint);
	}

	Universal::Universal(const Body* child, const Body* parent, const Ogre::Vector3& childPos, const Ogre::Vector3& childPin, const Ogre::Vector3& parentPos, const Ogre::Vector3& parentPin)
	{
		m_body0 = const_cast<Body*>(child);
		m_body1 = const_cast<Body*>(parent);

		ndBodyKinematic* b0 = child ? const_cast<ndBodyKinematic*>(child->getNewtonBody()) : nullptr;
		ndBodyKinematic* b1 = parent ? const_cast<ndBodyKinematic*>(parent->getNewtonBody()) : child->getWorld()->getNewtonWorld()->GetSentinelBody();

		// Use mid-point for pivot; align with childPin
		const Ogre::Vector3 mid = (childPos + parentPos) * 0.5f;
		Ogre::Quaternion q = OgreNewt::Converters::grammSchmidt(childPin);
		ndMatrix frame;
		OgreNewt::Converters::QuatPosToMatrix(q, mid, frame);

		auto* joint = new NdUniversalAdapter(frame, b0, b1);
		SetSupportJoint(child->getWorld(), joint);
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
		ndBodyKinematic* b1 = parent ? const_cast<ndBodyKinematic*>(parent->getNewtonBody()) : child->getWorld()->getNewtonWorld()->GetSentinelBody();

		ndMatrix frame(ndGetIdentityMatrix());
		frame.m_posit = ndVector(pos.x, pos.y, pos.z, 1.0f);

		auto* joint = new ndJointRoller(frame, b0, b1);
		SetSupportJoint(child->getWorld(), joint);
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
		ndBodyKinematic* b0 = child ? const_cast<ndBodyKinematic*>(child->getNewtonBody()) : nullptr;
		ndBodyKinematic* b1 = parent ? const_cast<ndBodyKinematic*>(parent->getNewtonBody()) : child->getWorld()->getNewtonWorld()->GetSentinelBody();

		ndJointCylinder* joint = new ndJointCylinder(frame, b0, b1 ? b1 : nullptr);
		SetSupportJoint(child->getWorld(), joint);

		// Defaults: limits disabled and no spring/damper (parity with old ctor)
		joint->SetLimitStatePosit(false);
		joint->SetLimitStateAngle(false);
		joint->SetAsSpringDamperPosit(0.1f, 0.0f, 0.0f);
		joint->SetAsSpringDamperAngle(0.1f, 0.0f, 0.0f);
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
		ndBodyKinematic* b1 = parent ? const_cast<ndBodyKinematic*>(parent->getNewtonBody()) : child->getWorld()->getNewtonWorld()->GetSentinelBody();

		const ndVector p0(pos1.x, pos1.y, pos1.z, 1.0f);
		const ndVector p1(pos2.x, pos2.y, pos2.z, 1.0f);

		auto* joint = new ndJointFixDistance(p0, p1, b0, b1);
		SetSupportJoint(child->getWorld(), joint);
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
		ndBodyKinematic* const body1 = parent ? const_cast<ndBodyKinematic*>(parent->getNewtonBody()) : child->getWorld()->getNewtonWorld()->GetSentinelBody();

		// create the 6DoF constraint
		auto* joint = new ndJointFix6dof(body0, body1, frame0, frame1);

		SetSupportJoint(child->getWorld(), joint);
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

	ndOgreHingeActuator::ndOgreHingeActuator(
		const ndMatrix& pinAndPivotFrame,
		ndBodyKinematic* const child,
		ndBodyKinematic* const parent,
		ndFloat32 angularRate,
		ndFloat32 minAngle,
		ndFloat32 maxAngle)
		: ndJointHinge(pinAndPivotFrame, child, parent)
		, m_motorSpeed(ndAbs(angularRate))
		, m_maxTorque(OGRENEWT_LARGE_TORQUE)
	{
		// We use our own limit handling (like ND3 custom joint),
		// so disable the built-in hinge limit state.
		SetLimitState(false);

		// initial limits & target angle
		m_minLimit = minAngle;
		m_maxLimit = maxAngle;
		m_targetAngle = m_angle;
	}

	void ndOgreHingeActuator::UpdateParameters()
	{
		ndJointHinge::UpdateParameters();
	}

	ndFloat32 ndOgreHingeActuator::GetActuatorAngle() const
	{
		// In ND3: GetActuatorAngle() just returned GetJointAngle().
		// Here we use ndJointHinge::GetAngle(), which uses m_angle updated in UpdateParameters().
		return GetAngle();
	}

	ndFloat32 ndOgreHingeActuator::GetTargetAngle() const
	{
		return m_targetAngle;
	}

	ndFloat32 ndOgreHingeActuator::GetMinAngularLimit() const
	{
		return m_minLimit;
	}

	ndFloat32 ndOgreHingeActuator::GetMaxAngularLimit() const
	{
		return m_maxLimit;
	}

	ndFloat32 ndOgreHingeActuator::GetAngularRate() const
	{
		return m_motorSpeed;
	}

	ndFloat32 ndOgreHingeActuator::GetMaxTorque() const
	{
		return m_maxTorque;
	}

	void ndOgreHingeActuator::SetMinAngularLimit(ndFloat32 limit)
	{
		m_minLimit = limit;
	}

	void ndOgreHingeActuator::SetMaxAngularLimit(ndFloat32 limit)
	{
		m_maxLimit = limit;
	}

	void ndOgreHingeActuator::SetAngularRate(ndFloat32 rate)
	{
		// ND3: EnableMotor(false, rate) -> store |rate|, no actual "motor" flag.
		m_motorSpeed = ndAbs(rate);
	}

	void ndOgreHingeActuator::SetMaxTorque(ndFloat32 torque)
	{
		m_maxTorque = ndAbs(torque);
	}

	void ndOgreHingeActuator::WakeBodies()
	{
		if (m_body0)
			m_body0->SetSleepState(false);
		if (m_body1)
			m_body1->SetSleepState(false);
	}

	void ndOgreHingeActuator::SetTargetAngle(ndFloat32 angle)
	{
		// Direct ND4 clone of ND3 SetTargetAngle:
		// angle = clamp(angle, m_minAngle, m_maxAngle);
		// if |angle - m_targetAngle| > 1e-3, wake body and update m_targetAngle.
		angle = ndClamp(angle, m_minLimit, m_maxLimit);
		if (ndAbs(angle - m_targetAngle) > ndFloat32(1.0e-3f))
		{
			WakeBodies();
			m_targetAngle = angle;
		}
	}

	void ndOgreHingeActuator::SetSpringAndDamping(
		bool enable,
		bool massIndependent,
		ndFloat32 springDamperRelaxation,
		ndFloat32 spring,
		ndFloat32 damper)
	{
		// ND3 used SetAsSpringDamper / SetMassIndependentSpringDamper
		// which boiled down to a spring-damper stiffness on the constraint row.
		// In ND4, ndJointHinge already has SetAsSpringDamper(regularizer, spring, damper),
		// which is mass-independent; we ignore massIndependent but keep behaviour.
		if (enable)
		{
			SetAsSpringDamper(springDamperRelaxation, spring, damper);
		}
		else
		{
			// "Disable" spring/damper by setting zero
			SetAsSpringDamper(ndFloat32(0.0f), ndFloat32(0.0f), ndFloat32(0.0f));
		}
	}

	// This is the core of the ND3 dCustomHingeActuator::SubmitAngularRow port.
	// We:
	// 1) Apply the base hinge rows (3 linear + 2 angular) via ApplyBaseRows.
	// 2) Add one angular row which behaves like the ND3 actuator servo.
	// 3) Use friction limits (±m_maxTorque) and limits (m_minLimit/m_maxLimit)
	//    exactly like the ND3 code.
	void ndOgreHingeActuator::JacobianDerivative(ndConstraintDescritor& desc)
	{
		ndMatrix matrix0;
		ndMatrix matrix1;
		CalculateGlobalMatrix(matrix0, matrix1);

		// 1) base hinge constraints (position + basic twist alignment)
		ApplyBaseRows(desc, matrix0, matrix1);

		// 2) actuator row, ported from dCustomHingeActuator::SubmitAngularRow
		//    (we use m_angle & m_targetAngle instead of m_curJointAngle & m_targetAngle.GetAngle())
		const ndFloat32 timestep = desc.m_timestep;
		const ndFloat32 invTimeStep = desc.m_invTimestep;

		const ndFloat32 angle = m_angle;
		const ndFloat32 targetAngle = m_targetAngle;

		const ndFloat32 step = m_motorSpeed * timestep;
		ndFloat32       currentSpeed = ndFloat32(0.0f);

		if (angle < (targetAngle - step))
		{
			currentSpeed = m_motorSpeed;
		}
		else if (angle < targetAngle)
		{
			currentSpeed = ndFloat32(0.3f) * (targetAngle - angle) * invTimeStep;
		}
		else if (angle > (targetAngle + step))
		{
			currentSpeed = -m_motorSpeed;
		}
		else if (angle > targetAngle)
		{
			currentSpeed = ndFloat32(0.3f) * (targetAngle - angle) * invTimeStep;
		}

		// This is equivalent to NewtonUserJointAddAngularRow(m_joint, 0.0f, &matrix0.m_front[0]);
		AddAngularRowJacobian(desc, &matrix0.m_front[0], ndFloat32(0.0f));

		// accel = NewtonUserJointCalculateRowZeroAcceleration + currentSpeed * invTimeStep;
		const ndFloat32 accel = GetMotorZeroAcceleration(desc) + currentSpeed * invTimeStep;
		SetMotorAcceleration(desc, accel);

		// 3) Limit handling via friction, just like ND3:
		//    if angle > max -> only positive torque
		//    if angle < min -> only negative torque
		//    else           -> +-m_maxTorque
		if (angle > m_maxLimit)
		{
			SetHighFriction(desc, m_maxTorque);
		}
		else if (angle < m_minLimit)
		{
			SetLowerFriction(desc, -m_maxTorque);
		}
		else
		{
			SetLowerFriction(desc, -m_maxTorque);
			SetHighFriction(desc, m_maxTorque);
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////
	
	HingeActuator::HingeActuator(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos,
		const Ogre::Vector3& pin, const Ogre::Degree& angularRate, const Ogre::Degree& minAngle, const Ogre::Degree& maxAngle)
		: Joint()
	{
		// Build hinge frame from pin + position, just like ND3 (dGrammSchmidt)
		Ogre::Quaternion q = OgreNewt::Converters::grammSchmidt(pin);

		ndMatrix pinAndPivotFrame;
		OgreNewt::Converters::QuatPosToMatrix(q, pos, pinAndPivotFrame);
		pinAndPivotFrame.m_posit = ndVector(pos.x, pos.y, pos.z, ndFloat32(1.0f));

		ndBodyKinematic* const b0 = child ?
			const_cast<ndBodyKinematic*>(child->getNewtonBody()) :
			nullptr;

		ndBodyKinematic* const b1 = parent ?
			const_cast<ndBodyKinematic*>(parent->getNewtonBody()) :
			(child ? child->getWorld()->getNewtonWorld()->GetSentinelBody() : nullptr);

		const ndFloat32 angularRateRad = angularRate.valueRadians();
		const ndFloat32 minAngleRad = minAngle.valueRadians();
		const ndFloat32 maxAngleRad = maxAngle.valueRadians();

		ndOgreHingeActuator* const joint = new ndOgreHingeActuator(pinAndPivotFrame, b0, b1, angularRateRad, minAngleRad, maxAngleRad);

		SetSupportJoint(child->getWorld(), joint);

		// hinge->SetSolverModel(m_jointIterativeSoft);
	}

	HingeActuator::~HingeActuator()
	{
	}

	// Degrees wrapper for component code:
	Ogre::Real HingeActuator::GetActuatorAngle() const
	{
		const ndOgreHingeActuator* const joint =
			static_cast<const ndOgreHingeActuator*>(GetSupportJoint());
		// joint angle is in radians → convert to degrees for your component
		return Ogre::Radian(joint->GetActuatorAngle()).valueDegrees();
	}

	void HingeActuator::SetTargetAngle(const Ogre::Degree& angle)
	{
		ndOgreHingeActuator* const joint =
			static_cast<ndOgreHingeActuator*>(GetSupportJoint());
		joint->SetTargetAngle(angle.valueRadians());
	}

	void HingeActuator::SetMinAngularLimit(const Ogre::Degree& limit)
	{
		ndOgreHingeActuator* const joint =
			static_cast<ndOgreHingeActuator*>(GetSupportJoint());
		joint->SetMinAngularLimit(limit.valueRadians());
	}

	void HingeActuator::SetMaxAngularLimit(const Ogre::Degree& limit)
	{
		ndOgreHingeActuator* const joint =
			static_cast<ndOgreHingeActuator*>(GetSupportJoint());
		joint->SetMaxAngularLimit(limit.valueRadians());
	}

	void HingeActuator::SetAngularRate(const Ogre::Degree& rate)
	{
		ndOgreHingeActuator* const joint =
			static_cast<ndOgreHingeActuator*>(GetSupportJoint());
		joint->SetAngularRate(rate.valueRadians());
	}

	void HingeActuator::SetMaxTorque(Ogre::Real torque)
	{
		ndOgreHingeActuator* const joint =
			static_cast<ndOgreHingeActuator*>(GetSupportJoint());
		joint->SetMaxTorque(static_cast<ndFloat32>(torque));
	}

	Ogre::Real HingeActuator::GetMaxTorque() const
	{
		const ndOgreHingeActuator* const joint =
			static_cast<const ndOgreHingeActuator*>(GetSupportJoint());
		return static_cast<Ogre::Real>(joint->GetMaxTorque());
	}

	void HingeActuator::SetSpringAndDamping(bool enable,
		bool massIndependent,
		Ogre::Real springDamperRelaxation,
		Ogre::Real spring,
		Ogre::Real damper)
	{
		ndOgreHingeActuator* const joint =
			static_cast<ndOgreHingeActuator*>(GetSupportJoint());
		joint->SetSpringAndDamping(
			enable,
			massIndependent,
			static_cast<ndFloat32>(springDamperRelaxation),
			static_cast<ndFloat32>(spring),
			static_cast<ndFloat32>(damper));
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	ndOgreSliderActuator::ndOgreSliderActuator(
		const ndMatrix& pinAndPivotFrame,
		ndBodyKinematic* const child,
		ndBodyKinematic* const parent,
		ndFloat32 linearRate,
		ndFloat32 minPosit,
		ndFloat32 maxPosit)
		: ndJointSlider(pinAndPivotFrame, child, parent)
		, m_targetPosit(ndFloat32(0.0f))
		, m_linearRate(linearRate)
		, m_maxForce(OGRENEWT_LARGE_FORCE)
		, m_minForce(-OGRENEWT_LARGE_FORCE)
		, m_force(ndFloat32(0.0f))
	{
		// we will handle limits & forces manually (like ND3 custom joint),
		// so disable built-in limit/maxForce state
		SetLimitState(false);
		SetMaxForceState(false);

		// set limits on base joint
		SetLimits(minPosit, maxPosit);

		// initialize target at current posit
		// (UpdateParameters will fill m_posit before JacobianDerivative)
		m_targetPosit = GetTargetPosit();
	}

	void ndOgreSliderActuator::UpdateParameters()
	{
		ndJointSlider::UpdateParameters();
	}

	void ndOgreSliderActuator::WakeBodies()
	{
		if (m_body0)
			m_body0->SetSleepState(false);
		if (m_body1)
			m_body1->SetSleepState(false);
	}

	ndFloat32 ndOgreSliderActuator::GetActuatorPosit() const
	{
		// equivalent to GetJointPosit() in ND3
		return GetPosit();
	}

	ndFloat32 ndOgreSliderActuator::GetTargetPosit() const
	{
		return m_targetPosit;
	}

	void ndOgreSliderActuator::SetTargetPosit(ndFloat32 posit)
	{
		ndFloat32 minLimit, maxLimit;
		GetLimits(minLimit, maxLimit);

		posit = ndClamp(posit, minLimit, maxLimit);
		if (ndAbs(posit - m_targetPosit) > ndFloat32(1.0e-3f))
		{
			WakeBodies();
			m_targetPosit = ndClamp(posit, minLimit, maxLimit);
		}
	}

	ndFloat32 ndOgreSliderActuator::GetLinearRate() const
	{
		return m_linearRate;
	}

	void ndOgreSliderActuator::SetLinearRate(ndFloat32 rate)
	{
		m_linearRate = ndAbs(rate);
	}

	ndFloat32 ndOgreSliderActuator::GetMinPositLimit() const
	{
		ndFloat32 minLimit, maxLimit;
		GetLimits(minLimit, maxLimit);
		return minLimit;
	}

	ndFloat32 ndOgreSliderActuator::GetMaxPositLimit() const
	{
		ndFloat32 minLimit, maxLimit;
		GetLimits(minLimit, maxLimit);
		return maxLimit;
	}

	void ndOgreSliderActuator::SetMinPositLimit(ndFloat32 limit)
	{
		ndFloat32 minLimit, maxLimit;
		GetLimits(minLimit, maxLimit);
		SetLimits(limit, maxLimit);
	}

	void ndOgreSliderActuator::SetMaxPositLimit(ndFloat32 limit)
	{
		ndFloat32 minLimit, maxLimit;
		GetLimits(minLimit, maxLimit);
		SetLimits(minLimit, limit);
	}

	ndFloat32 ndOgreSliderActuator::GetMaxForce() const
	{
		return m_maxForce;
	}

	ndFloat32 ndOgreSliderActuator::GetMinForce() const
	{
		return m_minForce;
	}

	void ndOgreSliderActuator::SetMaxForce(ndFloat32 force)
	{
		m_maxForce = ndAbs(force);
	}

	void ndOgreSliderActuator::SetMinForce(ndFloat32 force)
	{
		m_minForce = -ndAbs(force);
	}

	ndFloat32 ndOgreSliderActuator::GetForce() const
	{
		return m_force;
	}

	void ndOgreSliderActuator::JacobianDerivative(ndConstraintDescritor& desc)
	{
		ndMatrix matrix0;
		ndMatrix matrix1;
		CalculateGlobalMatrix(matrix0, matrix1);

		// 1) base slider constraints (align axes, lock unwanted degrees)
		ApplyBaseRows(desc, matrix0, matrix1);

		const ndFloat32 timestep = desc.m_timestep;
		const ndFloat32 invTimeStep = desc.m_invTimestep;

		const ndFloat32 posit = m_posit;      // updated by UpdateParameters()
		const ndFloat32 targetPosit = m_targetPosit;

		ndFloat32 minLimit, maxLimit;
		GetLimits(minLimit, maxLimit);

		// ND3 logic:
		// step = m_linearRate * timestep;
		const ndFloat32 step = m_linearRate * timestep;
		ndFloat32 currentSpeed = ndFloat32(0.0f);

		if (posit < (targetPosit - step))
		{
			currentSpeed = m_linearRate;
		}
		else if (posit < targetPosit)
		{
			currentSpeed = ndFloat32(0.3f) * (targetPosit - posit) * invTimeStep;
		}
		else if (posit > (targetPosit + step))
		{
			currentSpeed = -m_linearRate;
		}
		else if (posit > targetPosit)
		{
			currentSpeed = ndFloat32(0.3f) * (targetPosit - posit) * invTimeStep;
		}

		// 2) "active dof" row, port of ND3 SubmitAngularRow body:
		//
		// ND3:
		//   NewtonUserJointAddLinearRow(m_joint, &matrix0.m_posit[0],
		//                               &matrix1.m_posit[0], &matrix1.m_front[0]);
		//
		AddLinearRowJacobian(desc, &matrix0.m_posit[0], &matrix1.m_posit[0], &matrix1.m_front[0]);

		const ndFloat32 accel = GetMotorZeroAcceleration(desc) + currentSpeed * invTimeStep;
		SetMotorAcceleration(desc, accel);
		// row stiffness is handled inside base; we don't need explicit call here

		// 3) friction / force limits like ND3:
		if (posit > maxLimit)
		{
			SetHighFriction(desc, m_maxForce);
		}
		else if (posit < minLimit)
		{
			SetLowerFriction(desc, m_minForce);
		}
		else
		{
			SetLowerFriction(desc, m_minForce);
			SetHighFriction(desc, m_maxForce);
		}

		// For now we approximate m_force as the "expected" force range.
		// If you later need the exact constraint row force, we can refine this
		// once we hook into ndJointBilateralConstraint's row force accessors.
		//
		// Here we just store a signed value representing the active range.
		if (posit > targetPosit)
		{
			m_force = m_minForce;
		}
		else if (posit < targetPosit)
		{
			m_force = m_maxForce;
		}
		else
		{
			m_force = ndFloat32(0.0f);
		}
	}

	SliderActuator::SliderActuator(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos, const Ogre::Vector3& pin,
		Ogre::Real linearRate, Ogre::Real minPosition, Ogre::Real maxPosition)
		: Joint()
	{
		// Same way as old OgreNewt3 code: build frame from pin + pos
		Ogre::Quaternion q = OgreNewt::Converters::grammSchmidt(pin);

		ndMatrix pinAndPivotFrame;
		OgreNewt::Converters::QuatPosToMatrix(q, pos, pinAndPivotFrame);
		pinAndPivotFrame.m_posit = ndVector(pos.x, pos.y, pos.z, ndFloat32(1.0f));

		ndBodyKinematic* const b0 = child ?
			const_cast<ndBodyKinematic*>(child->getNewtonBody()) :
			nullptr;

		ndBodyKinematic* const b1 = parent ?
			const_cast<ndBodyKinematic*>(parent->getNewtonBody()) :
			(child ? child->getWorld()->getNewtonWorld()->GetSentinelBody() : nullptr);

		ndOgreSliderActuator* const joint = new ndOgreSliderActuator(pinAndPivotFrame, b0, b1, static_cast<ndFloat32>(linearRate), static_cast<ndFloat32>(minPosition), static_cast<ndFloat32>(maxPosition));

		SetSupportJoint(child->getWorld(), joint);
	}

	SliderActuator::~SliderActuator()
	{
	}

	Ogre::Real SliderActuator::GetActuatorPosition() const
	{
		const ndOgreSliderActuator* const joint =
			static_cast<const ndOgreSliderActuator*>(GetSupportJoint());
		return static_cast<Ogre::Real>(joint->GetActuatorPosit());
	}

	void SliderActuator::SetTargetPosition(Ogre::Real targetPosition)
	{
		ndOgreSliderActuator* const joint =
			static_cast<ndOgreSliderActuator*>(GetSupportJoint());
		joint->SetTargetPosit(static_cast<ndFloat32>(targetPosition));
	}

	void SliderActuator::SetLinearRate(Ogre::Real linearRate)
	{
		ndOgreSliderActuator* const joint =
			static_cast<ndOgreSliderActuator*>(GetSupportJoint());
		joint->SetLinearRate(static_cast<ndFloat32>(linearRate));
	}

	void SliderActuator::SetMinPositionLimit(Ogre::Real limit)
	{
		ndOgreSliderActuator* const joint =
			static_cast<ndOgreSliderActuator*>(GetSupportJoint());
		joint->SetMinPositLimit(static_cast<ndFloat32>(limit));
	}

	void SliderActuator::SetMaxPositionLimit(Ogre::Real limit)
	{
		ndOgreSliderActuator* const joint =
			static_cast<ndOgreSliderActuator*>(GetSupportJoint());
		joint->SetMaxPositLimit(static_cast<ndFloat32>(limit));
	}

	void SliderActuator::SetMinForce(Ogre::Real force)
	{
		ndOgreSliderActuator* const joint =
			static_cast<ndOgreSliderActuator*>(GetSupportJoint());
		joint->SetMinForce(static_cast<ndFloat32>(force));
	}

	void SliderActuator::SetMaxForce(Ogre::Real force)
	{
		ndOgreSliderActuator* const joint =
			static_cast<ndOgreSliderActuator*>(GetSupportJoint());
		joint->SetMaxForce(static_cast<ndFloat32>(force));
	}

	Ogre::Real SliderActuator::GetForce() const
	{
		const ndOgreSliderActuator* const joint =
			static_cast<const ndOgreSliderActuator*>(GetSupportJoint());
		return static_cast<Ogre::Real>(joint->GetForce());
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	using ndFloat = ndFloat32;

	class ndFlexyPipeHandle : public ndJointBilateralConstraint
	{
	public:
		ndFlexyPipeHandle(ndBodyKinematic* const body0,
			const ndVector& pin,
			const ndVector& anchorPosition)
			: ndJointBilateralConstraint(6, body0, body0->GetScene()->GetSentinelBody(), ndGetIdentityMatrix())
			, m_linearFriction(3000.0f)
			, m_angularFriction(200.0f)
		{
			Ogre::Quaternion q = OgreNewt::Converters::grammSchmidt(
				Ogre::Vector3(pin.m_x, pin.m_y, pin.m_z));
			ndMatrix globalFrame;
			OgreNewt::Converters::QuatPosToMatrix(
				q, Ogre::Vector3(anchorPosition.m_x, anchorPosition.m_y, anchorPosition.m_z), globalFrame);

			CalculateLocalMatrix(globalFrame, m_localMatrix0, m_localMatrix1);
			SetSolverModel(ndJointBilateralSolverModel::m_jointkinematicAttachment);
		}

		void UpdateParameters() override
		{
			
		}

		void JacobianDerivative(ndConstraintDescritor& desc) override
		{
			const ndMatrix m0 = CalculateGlobalMatrix0();

			for (int i = 0; i < 3; ++i)
			{
				AddLinearRowJacobian(desc, m0.m_posit, m0.m_posit, m0[i]);
				SetJointErrorPosit(desc, 0.0f);
				SetLowerFriction(desc, -m_linearFriction);
				SetHighFriction(desc, m_linearFriction);
			}

			for (int i = 0; i < 3; ++i)
			{
				AddAngularRowJacobian(desc, m0[i], 0.0f);
				SetJointErrorPosit(desc, 0.0f);
				SetLowerFriction(desc, -m_angularFriction);
				SetHighFriction(desc, m_angularFriction);
			}
		}

		void SetVelocity(const ndVector& desiredVel, ndFloat dt)
		{
			const ndVector massProps = m_body0->GetMassMatrix(); // (Ixx, Iyy, Izz, mass)
			const ndFloat mass = massProps.m_w;
			const ndVector curVel = m_body0->GetVelocity();
			const ndVector err = desiredVel - curVel;

			if (err.DotProduct(err).GetScalar() > ndFloat(1.0e-6f))
			{
				const ndVector dir = err.Normalize();
				const ndVector impulse = err.Scale(mass) + dir.Scale(m_linearFriction * dt);
				m_body0->ApplyImpulsePair(impulse, ndVector::m_wOne * 0.0f, dt);
			}
		}

		void SetOmega(const ndVector& desiredOmega, ndFloat dt)
		{
			const ndVector massProps = m_body0->GetMassMatrix(); // (Ixx, Iyy, Izz, mass)
			const ndFloat Ixx = massProps.m_x, Iyy = massProps.m_y, Izz = massProps.m_z;
			const ndVector curOmega = m_body0->GetOmega();
			const ndVector err = desiredOmega - curOmega;

			if (err.DotProduct(err).GetScalar() <= ndFloat(1.0e-6f))
				return;

			const ndMatrix R = m_body0->GetMatrix();
			const ndVector& r0 = R.m_front;
			const ndVector& r1 = R.m_up;
			const ndVector& r2 = R.m_right;

			const ndFloat e0 = err.DotProduct(r0).GetScalar();
			const ndFloat e1 = err.DotProduct(r1).GetScalar();
			const ndFloat e2 = err.DotProduct(r2).GetScalar();

			ndVector angImpulse = r0.Scale(Ixx * e0) + r1.Scale(Iyy * e1) + r2.Scale(Izz * e2);
			const ndVector dir = err.Normalize();
			angImpulse += dir.Scale(m_angularFriction * dt);

			m_body0->ApplyImpulsePair(ndVector::m_wOne * 0.0f, angImpulse, dt);
		}

		ndFloat m_linearFriction;
		ndFloat m_angularFriction;
	};

	class ndFlexyPipeSpinner : public ndJointBilateralConstraint
	{
	public:
		ndFlexyPipeSpinner(const ndMatrix& pinAndPivotFrame,
			ndBodyKinematic* const child,
			ndBodyKinematic* const parent)
			: ndJointBilateralConstraint(6, child, parent, pinAndPivotFrame)
		{
			SetSolverModel(ndJointBilateralSolverModel::m_jointIterativeSoft);
		}

		void UpdateParameters() override
		{
			// nothing special here for now
		}

		void JacobianDerivative(ndConstraintDescritor& desc) override
		{
			ndMatrix m0, m1;
			CalculateGlobalMatrix(m0, m1);

			// -------------------------------------------------
			// 1) Ball-and-socket positional constraint
			//    (keep pivot of child and parent together)
			// -------------------------------------------------
			const ndVector& p0 = m0.m_posit;
			const ndVector& p1 = m1.m_posit;

			// Standard way: 3 orthogonal directions from the child frame
			for (int i = 0; i < 3; ++i)
			{
				const ndVector& dir = m0[i];
				AddLinearRowJacobian(desc, p0, p1, dir);

				// optional soft spring on position (can be tuned or removed)
				// this gives a bit of compliance like the original ball/socket
				SetMassSpringDamperAcceleration(desc, ndFloat32(0.01f),
					ndFloat32(1000.0f),
					ndFloat32(50.0f));
			}

			// -------------------------------------------------
			// 2) Angular part: your twist + cone behavior
			// -------------------------------------------------
			ApplyTwistAction(desc, m0, m1);
			ApplyElasticConeAction(desc, m0, m1);
		}

		void ApplyTwistAction(ndConstraintDescritor& desc,
			const ndMatrix& m0,
			const ndMatrix& m1)
		{
			const ndVector pin0 = m0.m_front;
			const ndVector pin1 = m1.m_front.Scale(-1.0f);

			const ndFloat relOmega =
				m_body0->GetOmega().DotProduct(pin0).GetScalar() +
				m_body1->GetOmega().DotProduct(pin1).GetScalar();

			const ndFloat relAccel = -relOmega / desc.m_timestep;

			AddAngularRowJacobian(desc, pin0, 0.0f);
			SetMotorAcceleration(desc, relAccel);
		}

		void ApplyElasticConeAction(ndConstraintDescritor& desc,
			const ndMatrix& m0,
			const ndMatrix& m1)
		{
			const ndFloat relaxation = ndFloat32(0.01f);
			const ndFloat spring = ndFloat32(1000.0f);
			const ndFloat damper = ndFloat32(50.0f);
			const ndFloat maxConeAngle = ndDegreeToRad * ndFloat32(45.0f);

			ndFloat cosAng = m1.m_front.DotProduct(m0.m_front).GetScalar();
			if (cosAng >= ndFloat32(0.998f))
			{
				const ndFloat a0 = CalculateAngle(m0.m_front, m1.m_front, m1.m_up);
				AddAngularRowJacobian(desc, m1.m_up, a0);
				SetMassSpringDamperAcceleration(desc, relaxation, spring, damper);

				const ndFloat a1 = CalculateAngle(m0.m_front, m1.m_front, m1.m_right);
				AddAngularRowJacobian(desc, m1.m_right, a1);
				SetMassSpringDamperAcceleration(desc, relaxation, spring, damper);
			}
			else
			{
				ndVector lateralDir = m1.m_front.CrossProduct(m0.m_front);
				if (lateralDir.DotProduct(lateralDir).GetScalar() > ndFloat32(1.0e-6f))
				{
					lateralDir = lateralDir.Normalize();
					const ndFloat coneAngle = ndAcos(ndClamp(cosAng,
						ndFloat32(-1.0f),
						ndFloat32(1.0f)));

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

	FlexyPipeHandleJoint::FlexyPipeHandleJoint(OgreNewt::Body* currentBody, const Ogre::Vector3& pin)
	{
		ndBodyKinematic* b0 = const_cast<ndBodyKinematic*>(currentBody->getNewtonBody());

		Ogre::Vector3 pos;
		Ogre::Quaternion q;
		currentBody->getPositionOrientation(pos, q);

		const ndVector ndPin(pin.x, pin.y, pin.z, 0.0f);
		const ndVector ndPos(pos.x, pos.y, pos.z, 1.0f);

		auto* joint = new ndFlexyPipeHandle(b0, ndPin, ndPos);
		SetSupportJoint(currentBody->getWorld(), joint);
	}

	void FlexyPipeHandleJoint::setVelocity(const Ogre::Vector3& velocity, Ogre::Real dt)
	{
		if (auto* j = static_cast<ndFlexyPipeHandle*>(GetSupportJoint()))
			j->SetVelocity(ndVector(velocity.x, velocity.y, velocity.z, 0.0f), static_cast<ndFloat32>(dt));
	}

	void FlexyPipeHandleJoint::setOmega(const Ogre::Vector3& omega, Ogre::Real dt)
	{
		if (auto* j = static_cast<ndFlexyPipeHandle*>(GetSupportJoint()))
			j->SetOmega(ndVector(omega.x, omega.y, omega.z, 0.0f), static_cast<ndFloat32>(dt));
	}

	// Spinner wrapper
	FlexyPipeSpinnerJoint::FlexyPipeSpinnerJoint(OgreNewt::Body* currentBody, OgreNewt::Body* predecessorBody, const Ogre::Vector3& anchorPosition, const Ogre::Vector3& pin)
	{
		ndBodyKinematic* b0 = currentBody ? const_cast<ndBodyKinematic*>(currentBody->getNewtonBody()) : nullptr;
		ndBodyKinematic* b1 = predecessorBody ? const_cast<ndBodyKinematic*>(predecessorBody->getNewtonBody()) : currentBody->getWorld()->getNewtonWorld()->GetSentinelBody();

		Ogre::Quaternion q = OgreNewt::Converters::grammSchmidt(pin);
		ndMatrix frame;
		OgreNewt::Converters::QuatPosToMatrix(q, anchorPosition, frame);

		auto* joint = new ndFlexyPipeSpinner(frame, b0, b1);
		SetSupportJoint(currentBody->getWorld(), joint);
	}

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
		ndBodyKinematic* b1 = predecessorBody ? const_cast<ndBodyKinematic*>(predecessorBody->getNewtonBody()) : currentBody->getWorld()->getNewtonWorld()->GetSentinelBody();

		Ogre::Quaternion q = OgreNewt::Converters::grammSchmidt(pin);
		ndMatrix frame;
		OgreNewt::Converters::QuatPosToMatrix(q, anchorPosition, frame);

		auto* joint = new ndJointSlider(frame, b0, b1);
		SetSupportJoint(currentBody->getWorld(), joint);

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

	Plane2DUpVectorJoint::Plane2DUpVectorJoint(const OgreNewt::Body* child, const Ogre::Vector3& pos, const Ogre::Vector3& normal, const Ogre::Vector3& pin)
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

		SetSupportJoint(child->getWorld(), joint);
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

		void UpdateParameters() override
		{
			ndJointPlane::UpdateParameters();
		}
	};

	// -----------------------------------------------------------------------------
	// PlaneConstraint Implementation
	// -----------------------------------------------------------------------------
	PlaneConstraint::PlaneConstraint(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos, const Ogre::Vector3& normal)
		: Joint()
	{
		ndBodyKinematic* const b0 = const_cast<ndBodyKinematic*>(child->getNewtonBody());
		ndBodyKinematic* const b1 = parent ? const_cast<ndBodyKinematic*>(parent->getNewtonBody()) : child->getWorld()->getNewtonWorld()->GetSentinelBody();

		ndVector ndPos(pos.x, pos.y, pos.z, 1.0f);
		ndVector ndNormal(normal.x, normal.y, normal.z, 0.0f);

		auto* joint = new ndPlaneConstraintWrapper(ndPos, ndNormal, b0, b1);
		SetSupportJoint(child->getWorld(), joint);
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
		ndBodyKinematic* const child = static_cast<ndBodyKinematic*>(body->getNewtonBody());
		ndBodyKinematic* const sentinelBody = body->getWorld()->getNewtonWorld()->GetSentinelBody();

		// Convert Ogre::Vector3 → ndVector
		const ndVector up(pin.x, pin.y, pin.z, 0.0f);

		// Create the ND4 up-vector joint
		auto* joint = new ndJointUpVector(up, child, sentinelBody);

		// This is key: prefer the iterative soft model for stability, else whole simulation can become unstable!
		joint->SetSolverModel(m_jointIterativeSoft);

		SetSupportJoint(body->getWorld(), joint);
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
		if (child->getNewtonBody()->GetInvMass() <= 0.0f)
		{
			return;
		}

		const ndVector attachment(pos.x, pos.y, pos.z, 1.0f);
		ndBodyKinematic* const body = const_cast<ndBodyKinematic*>(child->getNewtonBody());
		ndBodyKinematic* const sentinelBody = child->getWorld()->getNewtonWorld()->GetSentinelBody();

		auto* joint = new ndJointKinematicController(body, sentinelBody, attachment);
		SetSupportJoint(child->getWorld(), joint);

		joint->SetControlMode(ndJointKinematicController::m_full6dof);
	}

	KinematicController::KinematicController(const OgreNewt::Body* child, const Ogre::Vector3& pos, const OgreNewt::Body* referenceBody)
	{
		const ndVector attachment(pos.x, pos.y, pos.z, 1.0f);
		ndBodyKinematic* b0 = child ? const_cast<ndBodyKinematic*>(child->getNewtonBody()) : nullptr;
		ndBodyKinematic* b1 = referenceBody ? const_cast<ndBodyKinematic*>(referenceBody->getNewtonBody()) : child->getWorld()->getNewtonWorld()->GetSentinelBody();

		auto* joint = new ndJointKinematicController(b1, b0, attachment);
		SetSupportJoint(child->getWorld(), joint);

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

	CustomDryRollingFriction::CustomDryRollingFriction(const Body* child, const Body* parent, Ogre::Real friction)
	{
		ndBodyKinematic* b0 = child ? const_cast<ndBodyKinematic*>(child->getNewtonBody()) : nullptr;
		ndBodyKinematic* b1 = parent ? const_cast<ndBodyKinematic*>(parent->getNewtonBody()) : child->getWorld()->getNewtonWorld()->GetSentinelBody();

		// create newton4 dry rolling friction joint
		auto* joint = new ndJointDryRollingFriction(b0, b1, static_cast<ndFloat32>(friction));
		SetSupportJoint(child->getWorld(), joint);
	}

	CustomDryRollingFriction::~CustomDryRollingFriction()
	{
		// handled by Joint base
	}

	void CustomDryRollingFriction::setFrictionCoefficient(Ogre::Real coeff)
	{
		if (auto* j = getJoint())
			j->SetFrictionCoefficient(static_cast<ndFloat32>(coeff));
	}

	Ogre::Real CustomDryRollingFriction::getFrictionCoefficient() const
	{
		if (auto* j = getJoint())
			return j->GetFrictionCoefficient();
		return 0.0f;
	}

	void CustomDryRollingFriction::setContactTrail(Ogre::Real trail)
	{
		if (auto* j = getJoint())
			j->SetContactTrail(static_cast<ndFloat32>(trail));
	}

	Ogre::Real CustomDryRollingFriction::getContactTrail() const
	{
		if (auto* j = getJoint())
			return j->GetContactTrail();
		return 0.0f;
	}

	/////////////////////////////////////////////////////////////////////////////////////

	// =============================================================
//   ND4 Adapter: implements spline query via PathFollow::m_spline
// =============================================================
	class NdCustomPathFollowAdapter : public ndJointFollowPath
	{
	public:
		NdCustomPathFollowAdapter(const ndMatrix& pinAndPivotFrame,
			ndBodyDynamic* const child,
			ndBodyDynamic* const pathBody,
			const ndBezierSpline* spline)
			: ndJointFollowPath(pinAndPivotFrame, child, pathBody)
			, m_spline(spline)
		{
		}

		void UpdateParameters() override
		{
			ndJointFollowPath::UpdateParameters();
		}

		void GetPointAndTangentAtLocation(const ndVector& location,
			ndVector& positOut,
			ndVector& tangentOut) const override
		{
			const ndBodyKinematic* const pathKine = GetBody1();
			const ndMatrix pathM(pathKine->GetMatrix());

			// local position relative to path body
			const ndVector pLocal(pathM.UntransformVector(location));

			ndBigVector closestPoint;
			const ndFloat64 knot = m_spline->FindClosestKnot(closestPoint, pLocal, 8);

			ndBigVector tangent(m_spline->CurveDerivative(knot));
			tangent = tangent.Scale(1.0 / ndSqrt(tangent.DotProduct(tangent).GetScalar()));

			positOut = pathM.TransformVector(closestPoint);
			tangentOut = ndVector(
				(ndFloat32)tangent.m_x,
				(ndFloat32)tangent.m_y,
				(ndFloat32)tangent.m_z,
				0.0f);
		}

	private:
		const ndBezierSpline* m_spline;
	};

	// =============================================================
	// PathFollow implementation
	// =============================================================

	PathFollow::PathFollow(OgreNewt::Body* child, OgreNewt::Body* pathBody, const Ogre::Vector3& pos)
		: m_childBody(child),
		m_pathBody(pathBody),
		m_pos(pos),
		m_loop(false),
		m_clockwise(true)
	{
	}

	PathFollow::~PathFollow()
	{
		// Base Joint class cleans up joint object
	}

	void PathFollow::setLoop(bool value)
	{
		m_loop = value;
		if (!m_sourceControlPoints.empty())
		{
			rebuildSpline();
		}
	}

	bool PathFollow::getLoop() const
	{
		return m_loop;
	}

	void PathFollow::setClockwise(bool value)
	{
		m_clockwise = value;
		if (!m_sourceControlPoints.empty())
		{
			rebuildSpline();
		}
	}

	bool PathFollow::getClockwise() const
	{
		return m_clockwise;
	}

	bool PathFollow::setKnots(const std::vector<Ogre::Vector3>& knots)
	{
		m_sourceControlPoints.clear();
		m_sourceControlPoints.reserve(knots.size());

		// store world-space points as given by the component
		for (const auto& p : knots)
		{
			m_sourceControlPoints.emplace_back(ndBigVector(p.x, p.y, p.z, 1.0));
		}

		if (m_sourceControlPoints.size() < 2)
			return false;

		// Path body must exist
		if (!m_pathBody || !m_pathBody->getNewtonBody())
			return false;

		rebuildSpline();
		return true;
	}

	void PathFollow::rebuildSpline()
	{
		m_internalControlPoints.clear();

		// nothing to do without source data
		if (m_sourceControlPoints.size() < 2)
			return;

		// Start from original points
		m_internalControlPoints = m_sourceControlPoints;

		// --------------------------------------------------
		// Apply clockwise: keep first point, reverse the rest
		// --------------------------------------------------
		if (!m_clockwise && m_internalControlPoints.size() > 2)
		{
			// [0] remains anchor/start, [1..N-1] get reversed
			std::reverse(m_internalControlPoints.begin() + 1, m_internalControlPoints.end());
		}

		// --------------------------------------------------
		// Apply loop: duplicate first at end if not already
		// --------------------------------------------------
		if (m_loop && !m_internalControlPoints.empty())
		{
			const ndBigVector& first = m_internalControlPoints.front();
			const ndBigVector& last = m_internalControlPoints.back();
			ndBigVector diff = last - first;
			const ndFloat64 dist2 = diff.DotProduct(diff).GetScalar();
			if (dist2 > 1.0e-8)
			{
				m_internalControlPoints.push_back(first);
			}
		}

		if (!m_pathBody || !m_pathBody->getNewtonBody())
			return;

		const ndBodyKinematic* pathKine = (ndBodyKinematic*)m_pathBody->getNewtonBody();
		ndMatrix pathM(pathKine->GetMatrix());

		// Convert world -> local
		std::vector<ndBigVector> localPoints;
		localPoints.reserve(m_internalControlPoints.size());

		for (const auto& pWorldBig : m_internalControlPoints)
		{
			ndVector pWorld((ndFloat32)pWorldBig.m_x, (ndFloat32)pWorldBig.m_y, (ndFloat32)pWorldBig.m_z, 1.0f);

			ndVector pLocal = pathM.UntransformVector(pWorld);
			localPoints.emplace_back(ndBigVector(pLocal.m_x, pLocal.m_y, pLocal.m_z, 1.0));
		}

		const ndInt32 count = (ndInt32)localPoints.size();
		if (count < 2)
			return;

		ndBigVector firstTangent = localPoints[1] - localPoints[0];
		ndBigVector lastTangent = localPoints[count - 1] - localPoints[count - 2];

		// Build the real spline
		m_spline.GlobalCubicInterpolation(count, &localPoints[0], firstTangent, lastTangent);
	}

	void PathFollow::createPathJoint(void)
	{
		// direction from first segment (already consistent with clockwise order)
		const Ogre::Vector3 dir = getMoveDirection(0);
		Ogre::Quaternion q = OgreNewt::Converters::grammSchmidt(dir);

		ndMatrix frame;
		OgreNewt::Converters::QuatPosToMatrix(q, Ogre::Vector3::ZERO, frame);

		const ndBodyKinematic* pathKine = (ndBodyKinematic*)m_pathBody->getNewtonBody();
		const ndMatrix pathM(pathKine->GetMatrix());
		frame.m_posit = pathM.TransformVector(ndVector(m_pos.x, m_pos.y, m_pos.z, 1.0f));

		ndBodyDynamic* const childDyn = ((ndBodyKinematic*)m_childBody->getNewtonBody())->GetAsBodyDynamic();
		ndBodyDynamic* const pathDyn = ((ndBodyKinematic*)m_pathBody->getNewtonBody())->GetAsBodyDynamic();

		auto* joint = new NdCustomPathFollowAdapter(frame, childDyn, pathDyn, &m_spline);
		SetSupportJoint(m_childBody->getWorld(), joint);
	}

	ndVector PathFollow::computeDirection(unsigned int index) const
	{
		if (m_internalControlPoints.empty() || index + 1 >= m_internalControlPoints.size())
			return ndVector(1.0f, 0.0f, 0.0f, 0.0f);

		const ndBigVector a = m_internalControlPoints[index + 0];
		const ndBigVector b = m_internalControlPoints[index + 1];
		ndVector dir((ndFloat32)(b.m_x - a.m_x), (ndFloat32)(b.m_y - a.m_y), (ndFloat32)(b.m_z - a.m_z), 0.0f);

		const ndFloat32 len2 = dir.DotProduct(dir).GetScalar();
		if (len2 <= 1.0e-6f)
			dir = ndVector(1.0f, 0.0f, 0.0f, 0.0f);
		else
			dir = dir.Scale(1.0f / ndSqrt(len2));

		return dir;
	}


	Ogre::Vector3 PathFollow::getMoveDirection(unsigned int index)
	{
		ndVector v = computeDirection(index);
		return Ogre::Vector3(v.m_x, v.m_y, v.m_z);
	}

	Ogre::Vector3 PathFollow::getCurrentMoveDirection(const Ogre::Vector3& currentBodyPosition)
	{
		if (m_internalControlPoints.empty())
			return Ogre::Vector3::ZERO;

		ndVector pos(currentBodyPosition.x, currentBodyPosition.y, currentBodyPosition.z, 1.0f);

		// Find nearest segment (same as before)
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
				m_localMatrix0.m_posit = ndVector(0.0f, 0.0f, 0.0f, 1.0f);
			}
			{
				ndMatrix pinFrame1(ndGramSchmidtMatrix(linAxis));
				CalculateLocalMatrix(pinFrame1, dummy, m_localMatrix1);
				m_localMatrix1.m_posit = ndVector(0.0f, 0.0f, 0.0f, 1.0f);
			}

			// Same solver mode as ndJointGear (stable)
			SetSolverModel(ndJointBilateralSolverModel::m_jointkinematicOpenLoop);
		}

		void UpdateParameters() override
		{
			
		}

		void JacobianDerivative(ndConstraintDescritor& desc) override
		{
			ndMatrix m0, m1;
			CalculateGlobalMatrix(m0, m1);

			// Add an angular constraint row, then override it
			AddAngularRowJacobian(desc, m0.m_front, ndFloat32(0.0f));

			ndJacobian& J0 = desc.m_jacobian[desc.m_rowsCount - 1].m_jacobianM0;
			ndJacobian& J1 = desc.m_jacobian[desc.m_rowsCount - 1].m_jacobianM1;

			J0.m_linear = ndVector(0.0f, 0.0f, 0.0f, 0.0f);
			J0.m_angular = ndVector(0.0f, 0.0f, 0.0f, 0.0f);
			J1.m_linear = ndVector(0.0f, 0.0f, 0.0f, 0.0f);
			J1.m_angular = ndVector(0.0f, 0.0f, 0.0f, 0.0f);

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
		SetSupportJoint(rotationalBody->getWorld(), joint);
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
				m_localMatrix0.m_posit = ndVector(0.0f, 0.0f, 0.0f, 1.0f);
			}
			{
				ndMatrix pinFrame1(ndGramSchmidtMatrix(linPin));
				CalculateLocalMatrix(pinFrame1, dummy, m_localMatrix1);
				m_localMatrix1.m_posit = ndVector(0.0f, 0.0f, 0.0f, 1.0f);
			}

			// Use kinematic open loop (matches ndJointGear default)
			SetSolverModel(ndJointBilateralSolverModel::m_jointkinematicOpenLoop);
		}

		void UpdateParameters() override
		{
			
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
			J0.m_linear = ndVector(0.0f, 0.0f, 0.0f, 0.0f);
			J0.m_angular = ndVector(0.0f, 0.0f, 0.0f, 0.0f);
			J1.m_linear = ndVector(0.0f, 0.0f, 0.0f, 0.0f);
			J1.m_angular = ndVector(0.0f, 0.0f, 0.0f, 0.0f);

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
		SetSupportJoint(rotationalBody->getWorld(), joint);
	}

	CustomGearAndSlide::~CustomGearAndSlide() = default;

	///////////////////////////////////////////////////////////////////////////////////////////////

	Pulley::Pulley(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& childPin, const Ogre::Vector3& parentPin, Ogre::Real pulleyRatio)
		: Joint()
	{
		// convert pins to ndVector
		const ndVector body0Pin(childPin.x, childPin.y, childPin.z, 0.0f);
		const ndVector body1Pin(parentPin.x, parentPin.y, parentPin.z, 0.0f);

		// cast to Newton4 bodies
		ndBodyKinematic* const body0 = child ? const_cast<ndBodyKinematic*>(child->getNewtonBody()) : nullptr;
		ndBodyKinematic* const body1 = parent ? const_cast<ndBodyKinematic*>(parent->getNewtonBody()) : child->getWorld()->getNewtonWorld()->GetSentinelBody();

		// create Newton4 pulley joint
		auto* joint = new ndJointPulley(static_cast<ndFloat32>(pulleyRatio), body0Pin, body0, body1Pin, body1);

		SetSupportJoint(child->getWorld(), joint);
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

	ndOgreDoubleHingeActuator::ndOgreDoubleHingeActuator(
		const ndMatrix& pinAndPivotFrame,
		ndBodyKinematic* const child,
		ndBodyKinematic* const parent,
		ndFloat32 angularRate0, ndFloat32 minAngle0, ndFloat32 maxAngle0,
		ndFloat32 angularRate1, ndFloat32 minAngle1, ndFloat32 maxAngle1)
		: ndJointDoubleHinge(pinAndPivotFrame, child, parent)
		, m_targetAngle0(ndFloat32(0.0f))
		, m_targetAngle1(ndFloat32(0.0f))
		, m_angularRate0(ndAbs(angularRate0))
		, m_angularRate1(ndAbs(angularRate1))
		, m_maxTorque0(OGRENEWT_LARGE_TORQUE)
		, m_maxTorque1(OGRENEWT_LARGE_TORQUE)
		, m_axis0Enable(true)
		, m_axis1Enable(true)
	{
		// We handle limits via our own servo logic
		SetLimitState0(false);
		SetLimitState1(false);

		SetLimits0(minAngle0, maxAngle0);
		SetLimits1(minAngle1, maxAngle1);

		// Initialize targets to current angles (after first update)
		m_targetAngle0 = GetAngle0();
		m_targetAngle1 = GetAngle1();
	}

	void ndOgreDoubleHingeActuator::UpdateParameters()
	{
		ndJointDoubleHinge::UpdateParameters();
	}

	void ndOgreDoubleHingeActuator::WakeBodies()
	{
		if (m_body0)
			m_body0->SetSleepState(false);
		if (m_body1)
			m_body1->SetSleepState(false);
	}

	// ---------------- Axis 0 ----------------

	ndFloat32 ndOgreDoubleHingeActuator::GetActuatorAngle0() const
	{
		return GetAngle0();
	}

	void ndOgreDoubleHingeActuator::SetTargetAngle0(ndFloat32 angle)
	{
		ndFloat32 minLimit, maxLimit;
		GetLimits0(minLimit, maxLimit);

		angle = ndClamp(angle, minLimit, maxLimit);
		if (ndAbs(angle - m_targetAngle0) > ndFloat32(1.0e-3f))
		{
			WakeBodies();
			m_targetAngle0 = angle;
		}
	}

	void ndOgreDoubleHingeActuator::SetMinAngularLimit0(ndFloat32 limit)
	{
		ndFloat32 minLimit, maxLimit;
		GetLimits0(minLimit, maxLimit);
		SetLimits0(limit, maxLimit);

		// ensure target stays in range
		SetTargetAngle0(m_targetAngle0);
	}

	void ndOgreDoubleHingeActuator::SetMaxAngularLimit0(ndFloat32 limit)
	{
		ndFloat32 minLimit, maxLimit;
		GetLimits0(minLimit, maxLimit);
		SetLimits0(minLimit, limit);

		SetTargetAngle0(m_targetAngle0);
	}

	void ndOgreDoubleHingeActuator::SetAngularRate0(ndFloat32 rate)
	{
		m_angularRate0 = ndAbs(rate);
	}

	void ndOgreDoubleHingeActuator::SetMaxTorque0(ndFloat32 torque)
	{
		m_maxTorque0 = ndAbs(torque);
	}

	ndFloat32 ndOgreDoubleHingeActuator::GetMaxTorque0() const
	{
		return m_maxTorque0;
	}

	// ---------------- Axis 1 ----------------

	ndFloat32 ndOgreDoubleHingeActuator::GetActuatorAngle1() const
	{
		return GetAngle1();
	}

	void ndOgreDoubleHingeActuator::SetTargetAngle1(ndFloat32 angle)
	{
		ndFloat32 minLimit, maxLimit;
		GetLimits1(minLimit, maxLimit);

		angle = ndClamp(angle, minLimit, maxLimit);
		if (ndAbs(angle - m_targetAngle1) > ndFloat32(1.0e-3f))
		{
			WakeBodies();
			m_targetAngle1 = angle;
		}
	}

	void ndOgreDoubleHingeActuator::SetMinAngularLimit1(ndFloat32 limit)
	{
		ndFloat32 minLimit, maxLimit;
		GetLimits1(minLimit, maxLimit);
		SetLimits1(limit, maxLimit);

		SetTargetAngle1(m_targetAngle1);
	}

	void ndOgreDoubleHingeActuator::SetMaxAngularLimit1(ndFloat32 limit)
	{
		ndFloat32 minLimit, maxLimit;
		GetLimits1(minLimit, maxLimit);
		SetLimits1(minLimit, limit);

		SetTargetAngle1(m_targetAngle1);
	}

	void ndOgreDoubleHingeActuator::SetAngularRate1(ndFloat32 rate)
	{
		m_angularRate1 = ndAbs(rate);
	}

	void ndOgreDoubleHingeActuator::SetMaxTorque1(ndFloat32 torque)
	{
		m_maxTorque1 = ndAbs(torque);
	}

	ndFloat32 ndOgreDoubleHingeActuator::GetMaxTorque1() const
	{
		return m_maxTorque1;
	}

	// ---------------- Jacobian ----------------

	void ndOgreDoubleHingeActuator::JacobianDerivative(ndConstraintDescritor& desc)
	{
		ndMatrix matrix0;
		ndMatrix matrix1;
		CalculateGlobalMatrix(matrix0, matrix1);

		// Base rows: same idea as dCustomDoubleHinge::SubmitAngularRow base call.
		ApplyBaseRows(desc, matrix0, matrix1);

		const ndFloat32 timestep = desc.m_timestep;
		const ndFloat32 invTimeStep = desc.m_invTimestep;

		const ndFloat32 angle0 = GetAngle0();
		const ndFloat32 angle1 = GetAngle1();

		const ndFloat32 target0 = m_targetAngle0;
		const ndFloat32 target1 = m_targetAngle1;

		ndFloat32 minLimit0, maxLimit0;
		ndFloat32 minLimit1, maxLimit1;
		GetLimits0(minLimit0, maxLimit0);
		GetLimits1(minLimit1, maxLimit1);

		// -------- Axis 0 servo (like first part of SubmitAngularRow) --------
		if (m_axis0Enable && (m_angularRate0 > ndFloat32(0.0f)))
		{
			const ndFloat32 step0 = m_angularRate0 * timestep;
			ndFloat32 currentSpeed0 = ndFloat32(0.0f);

			if (angle0 < (target0 - step0))
			{
				currentSpeed0 = m_angularRate0;
			}
			else if (angle0 < target0)
			{
				currentSpeed0 = ndFloat32(0.3f) * (target0 - angle0) * invTimeStep;
			}
			else if (angle0 > (target0 + step0))
			{
				currentSpeed0 = -m_angularRate0;
			}
			else if (angle0 > target0)
			{
				currentSpeed0 = ndFloat32(0.3f) * (target0 - angle0) * invTimeStep;
			}

			// ND3 row: NewtonUserJointAddAngularRow(m_joint, 0.0f, &matrix0.m_front[0]);
			AddAngularRowJacobian(desc, &matrix0.m_front[0], ndFloat32(0.0f));

			const ndFloat32 accel0 = GetMotorZeroAcceleration(desc) + currentSpeed0 * invTimeStep;
			SetMotorAcceleration(desc, accel0);
			SetLowerFriction(desc, -m_maxTorque0);
			SetHighFriction(desc, m_maxTorque0);
		}

		// -------- Axis 1 servo (second part of SubmitAngularRow) --------
		if (m_axis1Enable && (m_angularRate1 > ndFloat32(0.0f)))
		{
			const ndFloat32 step1 = m_angularRate1 * timestep;
			ndFloat32 currentSpeed1 = ndFloat32(0.0f);

			if (angle1 < (target1 - step1))
			{
				currentSpeed1 = m_angularRate1;
			}
			else if (angle1 < target1)
			{
				currentSpeed1 = ndFloat32(0.3f) * (target1 - angle1) * invTimeStep;
			}
			else if (angle1 > (target1 + step1))
			{
				currentSpeed1 = -m_angularRate1;
			}
			else if (angle1 > target1)
			{
				currentSpeed1 = ndFloat32(0.3f) * (target1 - angle1) * invTimeStep;
			}

			// ND3 row: NewtonUserJointAddAngularRow(m_joint, 0.0f, &matrix1.m_up[0]);
			AddAngularRowJacobian(desc, &matrix1.m_up[0], ndFloat32(0.0f));

			const ndFloat32 accel1 = GetMotorZeroAcceleration(desc) + currentSpeed1 * invTimeStep;
			SetMotorAcceleration(desc, accel1);
			SetLowerFriction(desc, -m_maxTorque1);
			SetHighFriction(desc, m_maxTorque1);
		}
	}

	UniversalActuator::UniversalActuator(const OgreNewt::Body* child,
		const OgreNewt::Body* parent,
		const Ogre::Vector3& pos,
		const Ogre::Degree& angularRate0,
		const Ogre::Degree& minAngle0,
		const Ogre::Degree& maxAngle0,
		const Ogre::Degree& angularRate1,
		const Ogre::Degree& minAngle1,
		const Ogre::Degree& maxAngle1)
		: Joint()
	{
		// Same as ND3: identity orientation, only position specified.
		ndMatrix pinAndPivotFrame(ndGetIdentityMatrix());
		pinAndPivotFrame.m_posit = ndVector(pos.x, pos.y, pos.z, ndFloat32(1.0f));

		ndBodyKinematic* const b0 = child ?
			const_cast<ndBodyKinematic*>(child->getNewtonBody()) :
			nullptr;

		ndBodyKinematic* const b1 = parent ?
			const_cast<ndBodyKinematic*>(parent->getNewtonBody()) :
			(child ? child->getWorld()->getNewtonWorld()->GetSentinelBody() : nullptr);

		ndFloat32 rate0Rad = static_cast<ndFloat32>(angularRate0.valueRadians());
		ndFloat32 min0Rad = static_cast<ndFloat32>(minAngle0.valueRadians());
		ndFloat32 max0Rad = static_cast<ndFloat32>(maxAngle0.valueRadians());

		ndFloat32 rate1Rad = static_cast<ndFloat32>(angularRate1.valueRadians());
		ndFloat32 min1Rad = static_cast<ndFloat32>(minAngle1.valueRadians());
		ndFloat32 max1Rad = static_cast<ndFloat32>(maxAngle1.valueRadians());

		ndOgreDoubleHingeActuator* const joint =
			new ndOgreDoubleHingeActuator(
				pinAndPivotFrame,
				b0,
				b1,
				rate0Rad, min0Rad, max0Rad,
				rate1Rad, min1Rad, max1Rad);

		SetSupportJoint(child->getWorld(), joint);
	}

	UniversalActuator::~UniversalActuator()
	{
	}

	// -------- Axis 0 --------

	Ogre::Real UniversalActuator::GetActuatorAngle0() const
	{
		const ndOgreDoubleHingeActuator* const joint =
			static_cast<const ndOgreDoubleHingeActuator*>(GetSupportJoint());
		return Ogre::Radian(joint->GetActuatorAngle0()).valueDegrees();
	}

	void UniversalActuator::SetTargetAngle0(const Ogre::Degree& angle0)
	{
		ndOgreDoubleHingeActuator* const joint =
			static_cast<ndOgreDoubleHingeActuator*>(GetSupportJoint());
		joint->SetTargetAngle0(static_cast<ndFloat32>(angle0.valueRadians()));
	}

	void UniversalActuator::SetMinAngularLimit0(const Ogre::Degree& limit0)
	{
		ndOgreDoubleHingeActuator* const joint =
			static_cast<ndOgreDoubleHingeActuator*>(GetSupportJoint());
		joint->SetMinAngularLimit0(static_cast<ndFloat32>(limit0.valueRadians()));
	}

	void UniversalActuator::SetMaxAngularLimit0(const Ogre::Degree& limit0)
	{
		ndOgreDoubleHingeActuator* const joint =
			static_cast<ndOgreDoubleHingeActuator*>(GetSupportJoint());
		joint->SetMaxAngularLimit0(static_cast<ndFloat32>(limit0.valueRadians()));
	}

	void UniversalActuator::SetAngularRate0(const Ogre::Degree& rate0)
	{
		ndOgreDoubleHingeActuator* const joint =
			static_cast<ndOgreDoubleHingeActuator*>(GetSupportJoint());
		joint->SetAngularRate0(static_cast<ndFloat32>(rate0.valueRadians()));
	}

	void UniversalActuator::SetMaxTorque0(Ogre::Real torque0)
	{
		ndOgreDoubleHingeActuator* const joint =
			static_cast<ndOgreDoubleHingeActuator*>(GetSupportJoint());
		joint->SetMaxTorque0(static_cast<ndFloat32>(torque0));
	}

	Ogre::Real UniversalActuator::GetMaxTorque0() const
	{
		const ndOgreDoubleHingeActuator* const joint =
			static_cast<const ndOgreDoubleHingeActuator*>(GetSupportJoint());
		return static_cast<Ogre::Real>(joint->GetMaxTorque0());
	}

	// -------- Axis 1 --------

	Ogre::Real UniversalActuator::GetActuatorAngle1() const
	{
		const ndOgreDoubleHingeActuator* const joint =
			static_cast<const ndOgreDoubleHingeActuator*>(GetSupportJoint());
		return Ogre::Radian(joint->GetActuatorAngle1()).valueDegrees();
	}

	void UniversalActuator::SetTargetAngle1(const Ogre::Degree& angle1)
	{
		ndOgreDoubleHingeActuator* const joint =
			static_cast<ndOgreDoubleHingeActuator*>(GetSupportJoint());
		joint->SetTargetAngle1(static_cast<ndFloat32>(angle1.valueRadians()));
	}

	void UniversalActuator::SetMinAngularLimit1(const Ogre::Degree& limit1)
	{
		ndOgreDoubleHingeActuator* const joint =
			static_cast<ndOgreDoubleHingeActuator*>(GetSupportJoint());
		joint->SetMinAngularLimit1(static_cast<ndFloat32>(limit1.valueRadians()));
	}

	void UniversalActuator::SetMaxAngularLimit1(const Ogre::Degree& limit1)
	{
		ndOgreDoubleHingeActuator* const joint =
			static_cast<ndOgreDoubleHingeActuator*>(GetSupportJoint());
		joint->SetMaxAngularLimit1(static_cast<ndFloat32>(limit1.valueRadians()));
	}

	void UniversalActuator::SetAngularRate1(const Ogre::Degree& rate1)
	{
		ndOgreDoubleHingeActuator* const joint =
			static_cast<ndOgreDoubleHingeActuator*>(GetSupportJoint());
		joint->SetAngularRate1(static_cast<ndFloat32>(rate1.valueRadians()));
	}

	void UniversalActuator::SetMaxTorque1(Ogre::Real torque1)
	{
		ndOgreDoubleHingeActuator* const joint =
			static_cast<ndOgreDoubleHingeActuator*>(GetSupportJoint());
		joint->SetMaxTorque1(static_cast<ndFloat32>(torque1));
	}

	Ogre::Real UniversalActuator::GetMaxTorque1() const
	{
		const ndOgreDoubleHingeActuator* const joint =
			static_cast<const ndOgreDoubleHingeActuator*>(GetSupportJoint());
		return static_cast<Ogre::Real>(joint->GetMaxTorque1());
	}

	///////////////////////////////////////////////////////////////////////////////////////////////

	class ndOgreMotorHinge : public ndJointHinge
	{
	public:
		D_CLASS_REFLECTION(ndOgreMotorHinge, ndJointHinge)

			ndOgreMotorHinge(const ndMatrix& pinAndPivotFrame,
				ndBodyKinematic* const child,
				ndBodyKinematic* const parent)
			: ndJointHinge(pinAndPivotFrame, child, parent)
			, m_speed(ndFloat32(0.0f))
			, m_maxTorque(ndFloat32(1000.0f))
		{
			// Start with zero offset = current angle
			UpdateParameters();
			m_targetAngle = GetAngle();

			// spring-damper like in your actuator
			const ndFloat32 regularizer = ndFloat32(0.1f);
			const ndFloat32 spring = ndFloat32(50.0f);
			const ndFloat32 damper = ndFloat32(5.0f);
			SetAsSpringDamper(regularizer, spring, damper);
		}

		void SetSpeed(ndFloat32 speed)
		{
			m_speed = speed;
			if (ndAbs(speed) > ndFloat32(1.0e-6f))
			{
				WakeBodies();
			}
		}

		ndFloat32 GetSpeed() const
		{
			return m_speed;
		}

		void SetMaxTorque(ndFloat32 torque)
		{
			m_maxTorque = ndAbs(torque);
		}

		ndFloat32 GetMaxTorque() const
		{
			return m_maxTorque;
		}

	protected:
		void WakeBodies()
		{
			if (m_body0)
				m_body0->SetSleepState(false);
			if (m_body1)
				m_body1->SetSleepState(false);
		}

		void UpdateParameters() override
		{
			// keep hinge internal angle/omega up to date
			ndJointHinge::UpdateParameters();
		}

		void JacobianDerivative(ndConstraintDescritor& desc) override
		{
			ndMatrix matrix0;
			ndMatrix matrix1;
			CalculateGlobalMatrix(matrix0, matrix1);

			// base hinge constraints
			ApplyBaseRows(desc, matrix0, matrix1);

			// update target angle: rotate continuously with speed
			const ndFloat32 timestep = desc.m_timestep;
			const ndFloat32 invTimeStep = desc.m_invTimestep;

			const ndFloat32 angle = GetAngle();
			m_targetAngle += m_speed * timestep;

			// same servo logic as in ndOgreHingeActuator, but no min/max limits
			const ndFloat32 targetAngle = m_targetAngle;
			const ndFloat32 step = ndAbs(m_speed) * timestep;

			// ndFloat32 currentSpeed = m_speed;
			ndFloat32 currentSpeed = ndFloat32(0.0f);

			if (angle < (targetAngle - step))
			{
				currentSpeed = ndAbs(m_speed);
			}
			else if (angle < targetAngle)
			{
				currentSpeed = ndFloat32(0.3f) * (targetAngle - angle) * invTimeStep;
			}
			else if (angle > (targetAngle + step))
			{
				currentSpeed = -ndAbs(m_speed);
			}
			else if (angle > targetAngle)
			{
				currentSpeed = ndFloat32(0.3f) * (targetAngle - angle) * invTimeStep;
			}

			// motor row around hinge axis (matrix0.m_front)
			AddAngularRowJacobian(desc, &matrix0.m_front[0], ndFloat32(0.0f));

			const ndFloat32 accel = GetMotorZeroAcceleration(desc) + currentSpeed * invTimeStep;
			SetMotorAcceleration(desc, accel);

			// symmetric torque limits
			SetLowerFriction(desc, -m_maxTorque);
			SetHighFriction(desc, m_maxTorque);
		}

	private:
		ndFloat32 m_speed;       // rad/sec
		ndFloat32 m_targetAngle; // rad
		ndFloat32 m_maxTorque;
	};

	// ---------------------------------------------------------------------
// Motor implementation (single hinge motor)
// ---------------------------------------------------------------------

	Motor::Motor(const Body* child, const Body* parent, const Ogre::Vector3& pin)
		: Joint()
		, m_targetSpeed(0.0f)
	{
		if (!child)
		{
			return;
		}

		// Build motor frame from the child bodys position and the given pin
		const Ogre::Vector3 pos = child->getPosition();

		Ogre::Quaternion q = OgreNewt::Converters::grammSchmidt(pin);

		ndMatrix frame;
		OgreNewt::Converters::QuatPosToMatrix(q, pos, frame);
		frame.m_posit = ndVector(pos.x, pos.y, pos.z, ndFloat32(1.0f));

		ndBodyKinematic* const childBody = const_cast<ndBodyKinematic*>(child->getNewtonBody());
		ndBodyKinematic* const parentBody = parent ? const_cast<ndBodyKinematic*>(parent->getNewtonBody()) : child->getWorld()->getNewtonWorld()->GetSentinelBody();

		ndOgreMotorHinge* const joint = new ndOgreMotorHinge(frame, childBody, parentBody);
		SetSupportJoint(child->getWorld(), joint);
	}

	Motor::~Motor() = default;

	// Helper to get the underlying ndOgreMotorHinge
	ndOgreMotorHinge* Motor::getMotor() const
	{
		return static_cast<ndOgreMotorHinge*>(GetSupportJoint());
	}

	Ogre::Real Motor::GetSpeed() const
	{
		if (auto* j = getMotor())
		{
			// Return actual hinge angular velocity (rad/s)
			return static_cast<Ogre::Real>(j->GetOmega());
		}
		return Ogre::Real(0.0f);
	}

	void Motor::SetSpeed(Ogre::Real speed)
	{
		m_targetSpeed = speed;

		if (auto* j = getMotor())
		{
			// We interpret speed0 as radians per second on the hinge axis
			j->SetSpeed(static_cast<ndFloat32>(speed));
		}
	}

	Ogre::Real Motor::GetCurrentAngleDeg(void) const
	{
		if (auto* j = getMotor())
		{
			// Return actual hinge angle in degrees
			return Ogre::Radian(j->GetAngle()).valueDegrees();
		}
		return Ogre::Real(0.0f);
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

	Wheel::Wheel(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos, const Ogre::Vector3& pin)
		: Joint()
	{
		// build pin matrix using our existing converter
		Ogre::Quaternion q = OgreNewt::Converters::grammSchmidt(pin);
		ndMatrix pinAndPivotFrame;
		OgreNewt::Converters::QuatPosToMatrix(q, pos, pinAndPivotFrame);

		// basic wheel descriptor
		ndWheelDescriptor desc;

		ndBodyKinematic* const childBody = const_cast<ndBodyKinematic*>(child->getNewtonBody());
		ndBodyKinematic* const parentBody = parent ? const_cast<ndBodyKinematic*>(parent->getNewtonBody()) : child->getWorld()->getNewtonWorld()->GetSentinelBody();

		auto* joint = new ndJointWheel(pinAndPivotFrame, childBody, parentBody, desc);
		SetSupportJoint(child->getWorld(), joint);
	}

	Wheel::Wheel(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& childPos, const Ogre::Vector3& childPin, const Ogre::Vector3& parentPos, const Ogre::Vector3& parentPin)
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
		ndBodyKinematic* const parentBody = parent ? const_cast<ndBodyKinematic*>(parent->getNewtonBody()) : child->getWorld()->getNewtonWorld()->GetSentinelBody();

		auto* joint = new ndJointWheel(frameChild, childBody, parentBody, desc);
		SetSupportJoint(child->getWorld(), joint);
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

	// -------------------------------------------------------------------------------------

	VehicleTire::VehicleTire(OgreNewt::Body* child, OgreNewt::Body* parentBody, const Ogre::Vector3& pos, const Ogre::Vector3& pin, Vehicle* parent, Ogre::Real radius)
		: Joint()
		, m_tireConfiguration(RayCastTire::TireConfiguration())
	{
		ndVector dir(pin.x, pin.y, pin.z, 0.0f);
		ndMatrix pinsAndPivoFrame(ndGramSchmidtMatrix(dir));
		pinsAndPivoFrame.m_posit = ndVector(pos.x, pos.y, pos.z, 1.0f);

		// create a Newton custom joint (our RayCastTire) and register as support joint
		RayCastTire* joint = new RayCastTire(child->getWorld()->getNewtonWorld(), pinsAndPivoFrame, dir, child, parentBody, parent, m_tireConfiguration, radius);
		SetSupportJoint(child->getWorld(), joint);
	}

	VehicleTire::~VehicleTire()
	{
	}

	void VehicleTire::setVehicleTireSide(VehicleTireSide tireSide)
	{
		RayCastTire* joint = static_cast<RayCastTire*>(GetSupportJoint());

		m_tireConfiguration.tireSide = tireSide;
		joint->setTireConfiguration(m_tireConfiguration);
	}

	VehicleTireSide VehicleTire::getVehicleTireSide(void) const
	{
		return m_tireConfiguration.tireSide;
	}

	void VehicleTire::setVehicleTireSteer(VehicleTireSteer tireSteer)
	{
		RayCastTire* joint = static_cast<RayCastTire*>(GetSupportJoint());

		m_tireConfiguration.tireSteer = tireSteer;
		joint->setTireConfiguration(m_tireConfiguration);
	}

	VehicleTireSteer VehicleTire::getVehicleTireSteer(void) const
	{
		return m_tireConfiguration.tireSteer;
	}

	void VehicleTire::setVehicleSteerSide(VehicleSteerSide steerSide)
	{
		RayCastTire* joint = static_cast<RayCastTire*>(GetSupportJoint());

		m_tireConfiguration.steerSide = steerSide;
		joint->setTireConfiguration(m_tireConfiguration);
	}

	VehicleSteerSide VehicleTire::getVehicleSteerSide(void) const
	{
		return m_tireConfiguration.steerSide;
	}

	void VehicleTire::setVehicleTireAccel(VehicleTireAccel tireAccel)
	{
		RayCastTire* joint = static_cast<RayCastTire*>(GetSupportJoint());

		m_tireConfiguration.tireAccel = tireAccel;
		joint->setTireConfiguration(m_tireConfiguration);
	}

	VehicleTireAccel VehicleTire::getVehicleTireAccel(void) const
	{
		return m_tireConfiguration.tireAccel;
	}

	void VehicleTire::setVehicleTireBrake(VehicleTireBrake brakeMode)
	{
		RayCastTire* joint = static_cast<RayCastTire*>(GetSupportJoint());

		m_tireConfiguration.brakeMode = brakeMode;
		joint->setTireConfiguration(m_tireConfiguration);
	}

	VehicleTireBrake VehicleTire::getVehicleTireBrake(void) const
	{
		return m_tireConfiguration.brakeMode;
	}

	void VehicleTire::setLateralFriction(Ogre::Real lateralFriction)
	{
		RayCastTire* joint = static_cast<RayCastTire*>(GetSupportJoint());

		m_tireConfiguration.lateralFriction = lateralFriction;
		joint->setTireConfiguration(m_tireConfiguration);
	}

	Ogre::Real VehicleTire::getLateralFriction(void) const
	{
		return m_tireConfiguration.lateralFriction;
	}

	void VehicleTire::setLongitudinalFriction(Ogre::Real longitudinalFriction)
	{
		RayCastTire* joint = static_cast<RayCastTire*>(GetSupportJoint());

		m_tireConfiguration.longitudinalFriction = longitudinalFriction;
		joint->setTireConfiguration(m_tireConfiguration);
	}

	Ogre::Real VehicleTire::getLongitudinalFriction(void) const
	{
		return m_tireConfiguration.longitudinalFriction;
	}

	void VehicleTire::setSpringLength(Ogre::Real springLength)
	{
		RayCastTire* joint = static_cast<RayCastTire*>(GetSupportJoint());

		m_tireConfiguration.springLength = springLength;
		joint->setTireConfiguration(m_tireConfiguration);
	}

	Ogre::Real VehicleTire::getSpringLength(void) const
	{
		return m_tireConfiguration.springLength;
	}

	void VehicleTire::setSmass(Ogre::Real smass)
	{
		RayCastTire* joint = static_cast<RayCastTire*>(GetSupportJoint());

		m_tireConfiguration.smass = smass;
		joint->setTireConfiguration(m_tireConfiguration);
	}

	Ogre::Real VehicleTire::getSmass(void) const
	{
		return m_tireConfiguration.smass;
	}

	void VehicleTire::setSpringConst(Ogre::Real springConst)
	{
		RayCastTire* joint = static_cast<RayCastTire*>(GetSupportJoint());

		m_tireConfiguration.springConst = springConst;
		joint->setTireConfiguration(m_tireConfiguration);
	}

	Ogre::Real VehicleTire::getSpringConst(void) const
	{
		return m_tireConfiguration.springConst;
	}

	void VehicleTire::setSpringDamp(Ogre::Real springDamp)
	{
		RayCastTire* joint = static_cast<RayCastTire*>(GetSupportJoint());

		m_tireConfiguration.springDamp = springDamp;
		joint->setTireConfiguration(m_tireConfiguration);
	}

	Ogre::Real VehicleTire::getSpringDamp(void) const
	{
		return m_tireConfiguration.springDamp;
	}

	RayCastTire* VehicleTire::getRayCastTire(void)
	{
		return static_cast<RayCastTire*>(GetSupportJoint());
	}

}   // end NAMESPACE OgreNewt
/*
	OgreNewt Library

	Ogre implementation of Newton Game Dynamics SDK

	OgreNewt basically has no license, you may use any or all of the library however you desire... I hope it can help you in any way.

		by Walaber
		some changes by melven and Lax

*/
#ifndef _INCLUDE_OGRENEWT_BASICJOINTS
#define _INCLUDE_OGRENEWT_BASICJOINTS

#include "OgreNewt_Prerequisites.h"
#include "OgreNewt_Joint.h"
#include "OgreNewt_Vehicle.h"
#include "ndBodyKinematic.h"
#include "ndJointSpherical.h"
#include "ndJointHinge.h"
#include "ndJointSlider.h"
#include "ndJointGear.h"
#include "ndJointDoubleHinge.h"
#include "ndJointRoller.h"
#include "ndJointCylinder.h"
#include "ndJointFixDistance.h"
#include "ndJointDryRollingFriction.h"
#include "ndJointFollowPath.h"
#include "ndBezierSpline.h"
#include "ndJointUpVector.h"
#include "ndJointPulley.h"
#include "ndJointPlane.h"
#include "ndJointFix6Dof.h"
#include "ndJointKinematicController.h"
#include "ndJointWheel.h"

// OgreNewt namespace.  all functions and classes use this namespace.
namespace OgreNewt
{
	//! Ball and Socket joint (Newton4 implementation, Newton3-compatible API)
	class _OgreNewtExport BallAndSocket : public Joint
	{
	public:
		BallAndSocket(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos);
		~BallAndSocket();

		// Same API as before:
		void enableTwist(bool state);
		void setTwistLimits(const Ogre::Degree& minAngle, const Ogre::Degree& maxAngle);
		void getTwistLimits(Ogre::Degree& minAngle, Ogre::Degree& maxAngle) const;

		void enableCone(bool state);
		Ogre::Degree getConeLimits() const;
		void setConeLimits(const Ogre::Degree& maxAngle);

		void setTwistFriction(Ogre::Real frictionTorque);
		Ogre::Real getTwistFriction(Ogre::Real frictionTorque) const;

		void setConeFriction(Ogre::Real frictionTorque);
		Ogre::Real getConeFriction(Ogre::Real frictionTorque) const;

		void setTwistSpringDamper(bool state, Ogre::Real springDamperRelaxation, Ogre::Real spring, Ogre::Real damper);

		Ogre::Vector3 getJointForce();
		Ogre::Vector3 getJointAngle();
		Ogre::Vector3 getJointOmega();

	private:
		ndJointSpherical* asNd() const { return static_cast<ndJointSpherical*>(GetSupportJoint()); }
		OgreNewt::Body* m_child;
		OgreNewt::Body* m_parent;
		bool m_twistEnabled;
		bool m_coneEnabled;
		Ogre::Real m_twistFriction;
		Ogre::Real m_coneFriction;
	};

	//! hinge joint.
	/*!
	simple hinge joint.  implement motors/limits through a callback.
	*/
	class _OgreNewtExport Hinge : public Joint
	{
	public:
		// ctor: one global frame (pos + pin)
		Hinge(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos, const Ogre::Vector3& pin);

		// ctor: two local frames (child/parent)
		Hinge(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& childPos, const Ogre::Vector3& childPin, const Ogre::Vector3& parentPos, const Ogre::Vector3& parentPin);

		~Hinge();

		void EnableLimits(bool state);

		void SetLimits(Ogre::Degree minAngle, Ogre::Degree maxAngle);

		Ogre::Radian GetJointAngle() const;

		Ogre::Radian GetJointAngularVelocity() const;

		Ogre::Vector3 GetJointPin() const;

		void SetTorque(Ogre::Real torque);

		void SetSpringAndDamping(bool enable, bool massIndependent, Ogre::Real springDamperRelaxation, Ogre::Real spring, Ogre::Real damper);

		void SetFriction(Ogre::Real friction);
	protected:
		ndJointHinge* asNd() const
		{
			return static_cast<ndJointHinge*>(GetSupportJoint());
		}
	protected:
		// cached bodies for wake-ups and relative calcs
		OgreNewt::Body* m_body0 = nullptr;
		OgreNewt::Body* m_body1 = nullptr;

		// we keep a stable "local pin in child space" to reconstruct world pin at runtime
		Ogre::Vector3   m_childPinLocal = Ogre::Vector3::UNIT_X;

		// emulate features that ND4 does not expose 1:1
		bool            m_limitsEnabled = false;
		Ogre::Real      m_lastRegularizer = 0.1f;  // reused for SetFriction emulation
		Ogre::Real      m_setFriction = 0.0f;      // last SetFriction value (viscous)
		Ogre::Real      m_motorTorque = 0.0f;      // SetTorque() requested torque (emulated)
	};

	/*!
	Gear joint.
	*/
	class _OgreNewtExport Gear : public Joint
	{
	public:
		//! Constructor
		/*!
			\param child pointer to the child rigid body.
			\param parent pointer to the parent rigid body (nullptr -> world)
			\param childPin direction of the child hinge/spin axis (in global space)
			\param parentPin direction of the parent hinge/spin axis (in global space)
			\param gearRatio ratio between rotational speeds (W0 * r0 = -W1 * r1)
		*/
		Gear(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& childPin, const Ogre::Vector3& parentPin, Ogre::Real gearRatio);

		~Gear();

	private:
		ndJointGear* asNd() const
		{
			return static_cast<ndJointGear*>(GetSupportJoint());
		}
	};

	//! universal joint.
	/*!
	Universal joint, that can be pinned and rotate about two axes either manually or automatically via motor and limits.
	*/
	class _OgreNewtExport Universal : public Joint
	{
	public:
		Universal(const OgreNewt::Body* child,
			const OgreNewt::Body* parent,
			const Ogre::Vector3& pos,
			const Ogre::Vector3& pin);

		Universal(const OgreNewt::Body* child,
			const OgreNewt::Body* parent,
			const Ogre::Vector3& childPos,
			const Ogre::Vector3& childPin,
			const Ogre::Vector3& parentPos,
			const Ogre::Vector3& parentPin);

		~Universal();

		// OgreNewt3.0 API — preserved
		void enableLimits0(bool state0);
		void setLimits0(Ogre::Degree minAngle0, Ogre::Degree maxAngle0);

		void enableLimits1(bool state1);
		void setLimits1(Ogre::Degree minAngle1, Ogre::Degree maxAngle1);

		void enableMotor0(bool state0, Ogre::Real motorSpeed0);

		void setFriction0(Ogre::Real frictionTorque);
		void setFriction1(Ogre::Real frictionTorque);

		void setAsSpringDamper0(bool state, Ogre::Real springDamperRelaxation,
			Ogre::Real spring, Ogre::Real damper);
		void setAsSpringDamper1(bool state, Ogre::Real springDamperRelaxation,
			Ogre::Real spring, Ogre::Real damper);

	private:
		// internal helper: our custom ND4 joint with per-step controls
		class NdUniversalAdapter : public ndJointDoubleHinge
		{
		public:
			NdUniversalAdapter(const ndMatrix& frame, ndBodyKinematic* child, ndBodyKinematic* parent)
				: ndJointDoubleHinge(frame, child, parent)
			{
			}

			// state controlled by the OgreNewt wrapper:
			bool        m_limits0Enabled = false;
			bool        m_limits1Enabled = false;
			ndFloat32   m_min0 = -ndFloat32(1.0e10f), m_max0 = ndFloat32(1.0e10f);
			ndFloat32   m_min1 = -ndFloat32(1.0e10f), m_max1 = ndFloat32(1.0e10f);

			// “Friction” emulation: use damper only (spring=0)
			ndFloat32   m_friction0 = 0.0f;
			ndFloat32   m_friction1 = 0.0f;

			// “Motor” emulation on axis0: advance offset angle each step
			bool        m_motor0Enabled = false;
			ndFloat32   m_motor0Speed = 0.0f;

			// extra store for user-set spring/damper (axis0/1)
			bool        m_sd0Enabled = false, m_sd1Enabled = false;
			ndFloat32   m_reg0 = 0.1f, m_k0 = 0.0f, m_c0 = 0.0f;
			ndFloat32   m_reg1 = 0.1f, m_k1 = 0.0f, m_c1 = 0.0f;

			void JacobianDerivative(ndConstraintDescritor& desc) override
			{
				// limits (apply wide limits if disabled)
				ndFloat32 limMin0 = m_limits0Enabled ? m_min0 : -ndFloat32(1.0e10f);
				ndFloat32 limMax0 = m_limits0Enabled ? m_max0 : ndFloat32(1.0e10f);
				ndFloat32 limMin1 = m_limits1Enabled ? m_min1 : -ndFloat32(1.0e10f);
				ndFloat32 limMax1 = m_limits1Enabled ? m_max1 : ndFloat32(1.0e10f);
				SetLimits0(limMin0, limMax0);
				SetLimits1(limMin1, limMax1);

				// friction emulation (viscous damping): damper only, no spring
				// merge with user spring/damper configuration:
				ndFloat32 damper0 = m_c0 + m_friction0;
				ndFloat32 damper1 = m_c1 + m_friction1;

				SetAsSpringDamper0(m_reg0, m_k0, damper0);
				SetAsSpringDamper1(m_reg1, m_k1, damper1);

				// motor emulation on axis0: advance offset angle with speed
				if (m_motor0Enabled)
				{
					const ndFloat32 angle0 = GetAngle0();
					SetOffsetAngle0(angle0 + m_motor0Speed * desc.m_timestep);
				}

				// run base implementation
				ndJointDoubleHinge::JacobianDerivative(desc);
			}
		};

		NdUniversalAdapter* asNd() const
		{
			return static_cast<NdUniversalAdapter*>(GetSupportJoint());
		}

		// wake-up helpers
		OgreNewt::Body* m_body0 = nullptr;
		OgreNewt::Body* m_body1 = nullptr;
	};

	//! Cork screw joint.
	/*!
	simple cork screw joint, with limits.
	*/
	class _OgreNewtExport CorkScrew : public Joint
	{
	public:
		//! constructor
		/*!
			\param child pointer to the child rigid body.
			\param parent pointer to the parent rigid body.
			\param pos position of the joint pivot in global space.
		*/
		CorkScrew(const OgreNewt::Body* child,
			const OgreNewt::Body* parent,
			const Ogre::Vector3& pos);

		//! destructor
		~CorkScrew();

		//! enable/disable linear and angular limits
		/*!
			\param linearLimits enable/disable linear motion limits
			\param angularLimits enable/disable angular rotation limits
		*/
		void enableLimits(bool linearLimits, bool angularLimits);

		//! set minimum and maximum linear stop distances
		/*!
			\param minLinearDistance minimum distance along the slider axis
			\param maxLinearDistance maximum distance along the slider axis
		*/
		void setLinearLimits(Ogre::Real minLinearDistance, Ogre::Real maxLinearDistance);

		//! set minimum and maximum angular rotation limits
		/*!
			\param minAngularAngle minimum angle in degrees
			\param maxAngularAngle maximum angle in degrees
		*/
		void setAngularLimits(Ogre::Degree minAngularAngle, Ogre::Degree maxAngularAngle);

		//! optional helper: apply spring/damper along the linear axis
		void setSpringDamperPosit(bool enable, Ogre::Real regularizer, Ogre::Real spring, Ogre::Real damper);

		//! optional helper: apply spring/damper along the angular axis
		void setSpringDamperAngle(bool enable, Ogre::Real regularizer, Ogre::Real spring, Ogre::Real damper);

		// rotation
		Ogre::Radian getAngle() const;
		Ogre::Radian getOffsetAngle() const;
		Ogre::Radian getOmega() const;
		void setOffsetAngle(Ogre::Radian angle);
		void setTargetOffsetAngle(Ogre::Radian offset); // alias for clarity
		void getAngleLimits(Ogre::Radian& min, Ogre::Radian& max) const;
		bool getLimitStateAngle() const;

		// translation
		Ogre::Real getPosit() const;
		Ogre::Real getTargetPosit() const;
		void setTargetPosit(Ogre::Real offset);
		void getLinearLimits(Ogre::Real& min, Ogre::Real& max) const;
		bool getLimitStatePosit() const;

		// spring–damper parameters
		void getSpringDamperAngle(Ogre::Real& regularizer, Ogre::Real& spring, Ogre::Real& damper) const;
		void getSpringDamperPosit(Ogre::Real& regularizer, Ogre::Real& spring, Ogre::Real& damper) const;

	private:
		ndJointRoller* getJoint() const
		{
			return static_cast<ndJointRoller*>(GetSupportJoint());
		}
	};

	//! Point to point joint
	class _OgreNewtExport PointToPoint : public Joint
	{

	public:
		//! constructor
		/*!
		\param child pointer to the child rigid body.
		\param parent pointer to the parent rigid body. pass nullptr to make the world itself the parent (aka a rigid joint)
		\param pos1 position of the first joint in global space
		\param pos2 position of the second joint in global space
		*/
		PointToPoint(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos1, const Ogre::Vector3& pos2);

		//! destructor.
		~PointToPoint();
	};

	//! Joint with 6 degree of freedom
	class _OgreNewtExport Joint6Dof : public Joint
	{
	public:
		/*!
			\param child pointer to the child rigid body.
			\param parent pointer to the parent rigid body (or nullptr for world).
			\param pos1 global position of the first anchor.
			\param pos2 global position of the second anchor.
		*/
		Joint6Dof(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos1, const Ogre::Vector3& pos2);

		~Joint6Dof();

		//! Set linear limits for X, Y, Z translation
		void setLinearLimits(const Ogre::Vector3& minLinearLimits, const Ogre::Vector3& maxLinearLimits) const;

		//! Set yaw (rotation around Y) angular limits
		void setYawLimits(const Ogre::Degree& minLimits, const Ogre::Degree& maxLimits) const;

		//! Set pitch (rotation around X) angular limits
		void setPitchLimits(const Ogre::Degree& minLimits, const Ogre::Degree& maxLimits) const;

		//! Set roll (rotation around Z) angular limits
		void setRollLimits(const Ogre::Degree& minLimits, const Ogre::Degree& maxLimits) const;

		//! Enable/disable soft joint behavior (spring-like flexibility)
		void setAsSoftJoint(bool enabled);

		//! Set joint regularizer (stiffness tuning)
		void setRegularizer(Ogre::Real regularizer);

		//! Get current regularizer value
		Ogre::Real getRegularizer() const;

		//! Retrieve maximum force and torque constraints
		Ogre::Real getMaxForce() const;
		Ogre::Real getMaxTorque() const;

	private:
		inline ndJointFix6dof* getJoint() const
		{
			return static_cast<ndJointFix6dof*>(GetSupportJoint());
		}
	};

	class ndOgreHingeActuator : public ndJointHinge
	{
	public:
		D_CLASS_REFLECTION(ndOgreHingeActuator, ndJointHinge)

			ndOgreHingeActuator(
				const ndMatrix& pinAndPivotFrame,
				ndBodyKinematic* const child,
				ndBodyKinematic* const parent,
				ndFloat32 angularRate,
				ndFloat32 minAngle,
				ndFloat32 maxAngle);

		virtual ~ndOgreHingeActuator() {}

		// API equivalent to old dCustomHingeActuator,
		// but in radians for Newton4 side.
		ndFloat32 GetActuatorAngle() const;

		ndFloat32 GetTargetAngle() const;
		void      SetTargetAngle(ndFloat32 angle);

		ndFloat32 GetMinAngularLimit() const;
		ndFloat32 GetMaxAngularLimit() const;
		void      SetMinAngularLimit(ndFloat32 limit);
		void      SetMaxAngularLimit(ndFloat32 limit);

		ndFloat32 GetAngularRate() const;
		void      SetAngularRate(ndFloat32 rate);

		ndFloat32 GetMaxTorque() const;
		void      SetMaxTorque(ndFloat32 torque);

		// massIndependent is ignored in ND4 (hinge spring is already mass-independent),
		// but we keep the signature for OgreNewt compatibility.
		void SetSpringAndDamping(
			bool enable,
			bool massIndependent,
			ndFloat32 springDamperRelaxation,
			ndFloat32 spring,
			ndFloat32 damper);

		// main constraint builder (ND3 SubmitAngularRow equivalent + hinge base rows)
		void JacobianDerivative(ndConstraintDescritor& desc) override;

	protected:
		// we do NOT override UpdateParameters; we reuse ndJointHinge::UpdateParameters
		// which fills m_angle and m_omega for us.

		void WakeBodies();

		void UpdateParameters() override;

		ndFloat32 m_motorSpeed;   // like m_motorSpeed in ND3 actuator
		ndFloat32 m_maxTorque;    // like m_maxTorque in ND3 actuator

		// we reuse ndJointHinge protected fields:
		//   m_angle, m_omega, m_minLimit, m_maxLimit, m_targetAngle, ...
	};

	/*!
		Hinge actuator joint (ND4 port).
		API kept compatible with the old ND3-based OgreNewt::HingeActuator.
	*/
	class _OgreNewtExport HingeActuator : public Joint
	{
	public:
		HingeActuator(const OgreNewt::Body* child,
			const OgreNewt::Body* parent,
			const Ogre::Vector3& pos,
			const Ogre::Vector3& pin,
			const Ogre::Degree& angularRate,
			const Ogre::Degree& minAngle,
			const Ogre::Degree& maxAngle);

		~HingeActuator();

		Ogre::Real GetActuatorAngle() const;

		void SetTargetAngle(const Ogre::Degree& angle);

		void SetMinAngularLimit(const Ogre::Degree& limit);
		void SetMaxAngularLimit(const Ogre::Degree& limit);

		void SetAngularRate(const Ogre::Degree& rate);

		void SetMaxTorque(Ogre::Real torque);
		Ogre::Real GetMaxTorque() const;

		void SetSpringAndDamping(bool enable,
			bool massIndependent,
			Ogre::Real springDamperRelaxation,
			Ogre::Real spring,
			Ogre::Real damper);
	};

	//! slider joint.
	/*!
	simple slider joint.  implement motors/limits through a callback.
	*/
	class _OgreNewtExport Slider : public Joint
	{
	public:
		Slider(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos, const Ogre::Vector3& pin);

		~Slider();

		void enableLimits(bool state);
		void setLimits(Ogre::Real minStopDist, Ogre::Real maxStopDist);
		void setFriction(Ogre::Real friction);
		void setSpringAndDamping(bool state, Ogre::Real springDamperRelaxation, Ogre::Real spring, Ogre::Real damper);

	private:
		ndJointSlider* asNd() const
		{
			return static_cast<ndJointSlider*>(GetSupportJoint());
		}

		OgreNewt::Body* m_body0 = nullptr;
		OgreNewt::Body* m_body1 = nullptr;

		bool  m_limitsEnabled = false;
		Ogre::Real m_lastRegularizer = 0.1f;
		Ogre::Real m_friction = 0.0f;
	};

	//! Sliding contact joint.
	/*!
	Sliding contact joint.
	*/
	class _OgreNewtExport SlidingContact : public Joint
	{
	public:
		SlidingContact(const OgreNewt::Body* child,
			const OgreNewt::Body* parent,
			const Ogre::Vector3& pos,
			const Ogre::Vector3& pin);

		~SlidingContact();

		void enableLinearLimits(bool state);
		void enableAngularLimits(bool state);

		void setLinearLimits(Ogre::Real minStopDist, Ogre::Real maxStopDist);
		void setAngularLimits(Ogre::Degree minAngle, Ogre::Degree maxAngle);

		void setSpringAndDamping(bool state, Ogre::Real springDamperRelaxation, Ogre::Real spring, Ogre::Real damper);

	private:
		inline ndJointCylinder* asNd() const
		{
			return static_cast<ndJointCylinder*>(GetSupportJoint());
		}

		// used only to wake the bodies when parameters change (behavior parity with 3.x)
		OgreNewt::Body* m_body0 = nullptr;
		OgreNewt::Body* m_body1 = nullptr;

		// cache last “enabled” spring/damper so toggling on/off is consistent
		bool      m_sdEnabled = false;
		ndFloat32 m_reg = 0.1f;
		ndFloat32 m_k = 0.0f;
		ndFloat32 m_c = 0.0f;
	};

	class ndOgreSliderActuator : public ndJointSlider
	{
	public:
		D_CLASS_REFLECTION(ndOgreSliderActuator, ndJointSlider)

			ndOgreSliderActuator(
				const ndMatrix& pinAndPivotFrame,
				ndBodyKinematic* const child,
				ndBodyKinematic* const parent,
				ndFloat32 linearRate,
				ndFloat32 minPosit,
				ndFloat32 maxPosit);

		virtual ~ndOgreSliderActuator() {}

		// --- ND3-equivalent API (radians/meters) ---

		// position along slider axis (like GetJointPosit)
		ndFloat32 GetActuatorPosit() const;

		// target
		ndFloat32 GetTargetPosit() const;
		void      SetTargetPosit(ndFloat32 posit);

		// limits
		ndFloat32 GetLinearRate() const;
		void      SetLinearRate(ndFloat32 rate);

		ndFloat32 GetMinPositLimit() const;
		ndFloat32 GetMaxPositLimit() const;
		void      SetMinPositLimit(ndFloat32 limit);
		void      SetMaxPositLimit(ndFloat32 limit);

		// forces
		ndFloat32 GetMaxForce() const;
		ndFloat32 GetMinForce() const;
		void      SetMaxForce(ndFloat32 force);
		void      SetMinForce(ndFloat32 force);

		ndFloat32 GetForce() const;

		// main constraint builder (ND3 SubmitAngularRow equivalent + slider base rows)
		void JacobianDerivative(ndConstraintDescritor& desc) override;

	protected:
		void UpdateParameters() override;

		void WakeBodies();

		ndFloat32 m_targetPosit;
		ndFloat32 m_linearRate;
		ndFloat32 m_maxForce;
		ndFloat32 m_minForce;
		ndFloat32 m_force;        // last constraint row force (approx like ND3 m_force)
	};

	//! Slider actuator joint.
	/*!
	Slider actuator joint.
	*/
	class _OgreNewtExport SliderActuator : public Joint
	{
	public:
		//! constructor
		/*!
		\param child pointer to the child rigid body.
		\param parent pointer to the parent rigid body. pass nullptr to make the world itself the parent (aka a rigid joint)
		\param pos of the joint in global space
		\param pin direction of the joint pin in global space
		*/
		SliderActuator(const OgreNewt::Body* child,
			const OgreNewt::Body* parent,
			const Ogre::Vector3& pos,
			const Ogre::Vector3& pin,
			Ogre::Real linearRate,
			Ogre::Real minPosition,
			Ogre::Real maxPosition);

		//! destructor
		~SliderActuator();

		Ogre::Real GetActuatorPosition() const;

		void SetTargetPosition(Ogre::Real targetPosition);

		void SetLinearRate(Ogre::Real linearRate);

		void SetMinPositionLimit(Ogre::Real limit);
		void SetMaxPositionLimit(Ogre::Real limit);

		void SetMinForce(Ogre::Real force);
		void SetMaxForce(Ogre::Real force);

		Ogre::Real GetForce() const;
	};

	class _OgreNewtExport FlexyPipeHandleJoint : public Joint
	{
	public:
		// Keep legacy signature: only body + pin
		FlexyPipeHandleJoint(OgreNewt::Body* currentBody, const Ogre::Vector3& pin);

		// Legacy drive apis preserved
		void setVelocity(const Ogre::Vector3& velocity, Ogre::Real dt);
		void setOmega(const Ogre::Vector3& omega, Ogre::Real dt);
	};

	// Spinner (ND4)
	class _OgreNewtExport FlexyPipeSpinnerJoint : public Joint
	{
	public:
		// Keep legacy signature
		FlexyPipeSpinnerJoint(OgreNewt::Body* currentBody, OgreNewt::Body* predecessorBody, const Ogre::Vector3& anchorPosition, const Ogre::Vector3& pin);
	};

	class _OgreNewtExport ActiveSliderJoint : public OgreNewt::Joint
	{
	public:
		ActiveSliderJoint(OgreNewt::Body* currentBody, OgreNewt::Body* predecessorBody, const Ogre::Vector3& anchorPosition,
			const Ogre::Vector3& pin, Ogre::Real minStopDistance, Ogre::Real maxStopDistance,
			Ogre::Real moveSpeed, bool repeat, bool directionChange, bool activated);

		virtual ~ActiveSliderJoint();

		void setActivated(bool activated);
		void enableLimits(bool state);
		void setLimits(Ogre::Real minStopDist, Ogre::Real maxStopDist);
		void setMoveSpeed(Ogre::Real moveSpeed);
		void setRepeat(bool repeat);
		void setDirectionChange(bool directionChange);
		int  getCurrentDirection() const;

		// call once per physics tick
		void ControllerUpdate(Ogre::Real dt);

	private:
		ndJointSlider* asNd() const { return static_cast<ndJointSlider*>(GetSupportJoint()); }
		void _wake();

		OgreNewt::Body* m_body0 = nullptr;
		OgreNewt::Body* m_body1 = nullptr;

		Ogre::Vector3 m_pin;
		Ogre::Real m_minStop = 0.0f;
		Ogre::Real m_maxStop = 0.0f;
		Ogre::Real m_moveSpeed = 0.0f; // units/sec
		bool m_repeat = false;
		bool m_directionChange = false;
		bool m_activated = false;

		Ogre::Real m_oppositeDir = 1.0f;
		Ogre::Real m_round = 0;
	};

	class _OgreNewtExport Plane2DUpVectorJoint : public Joint
	{
	public:
		/*!
			\param child pointer to the body constrained to the plane.
			\param pos pivot position in global coordinates.
			\param normal normal vector defining the plane.
			\param pin optional pin vector to lock the up direction.
		*/
		Plane2DUpVectorJoint(const OgreNewt::Body* child, const Ogre::Vector3& pos, const Ogre::Vector3& normal, const Ogre::Vector3& pin);

		virtual ~Plane2DUpVectorJoint();

		//! Change the pin direction in world space.
		void setPin(const Ogre::Vector3& pin);

		//! Retrieve the current pin direction.
		const Ogre::Vector3& getPin() const { return m_pin; }

		//! Enables or disables rotation control on the plane joint.
		void setEnableControlRotation(bool state);

		//! Returns whether control rotation is enabled.
		bool getEnableControlRotation() const;

	private:
		Ogre::Vector3 m_pin;

		inline ndJointPlane* getJoint() const
		{
			return static_cast<ndJointPlane*>(GetSupportJoint());
		}
	};

	//! UpVector joint.
	/*!
	simple upvector joint.  upvectors remove all rotation except for a single pin.  useful for character controllers, etc.
	*/
	class _OgreNewtExport UpVector : public Joint
	{
	public:
		//! constructor
		/*!
			\param body pointer to the body to apply the upvector to
			\param pin world-space direction the upvector will align with
		*/
		UpVector(const Body* body, const Ogre::Vector3& pin);

		//! destructor
		~UpVector();

		//! set the pin direction (can be called every frame)
		void setPin(const Body* body, const Ogre::Vector3& pin);

		//! get the current pin direction
		const Ogre::Vector3& getPin() const;

	private:
		Ogre::Vector3 m_pin;

		inline ndJointUpVector* getJoint() const
		{
			return static_cast<ndJointUpVector*>(GetSupportJoint());
		}
	};

	//! Kinematic controller joint.
	/*!
	simple Kinematic controller. used it to make a simple object follow a path or a position.
	*/
	class _OgreNewtExport KinematicController : public Joint
	{
	public:
		//! Constructor with attachment to world
		KinematicController(const OgreNewt::World* world, const OgreNewt::Body* child, const Ogre::Vector3& pos);

		//! Constructor with reference body
		KinematicController(const OgreNewt::Body* child, const Ogre::Vector3& pos, const OgreNewt::Body* referenceBody);

		~KinematicController();

		//! Set control mode (0=linear, 1=full6dof, 2=linear+angular friction)
		void setPickingMode(int mode);
		int getPickingMode() const;

		//! Set/get maximum linear friction
		void setMaxLinearFriction(Ogre::Real force);
		Ogre::Real getMaxLinearFriction() const;

		//! Set/get maximum angular friction
		void setMaxAngularFriction(Ogre::Real torque);
		Ogre::Real getMaxAngularFriction() const;

		//! Set/get viscous angular friction coefficient
		void setAngularViscousFrictionCoefficient(Ogre::Real coefficient);
		Ogre::Real getAngularViscousFrictionCoefficient() const;

		//! Set/get maximum linear speed
		void setMaxSpeed(Ogre::Real speedInMetersPerSeconds);
		Ogre::Real getMaxSpeed() const;

		//! Set/get maximum angular speed (omega)
		void setMaxOmega(const Ogre::Degree& speedInDegreesPerSeconds);
		Ogre::Real getMaxOmega() const;

		//! Set target transforms
		void setTargetPosit(const Ogre::Vector3& position);
		void setTargetRotation(const Ogre::Quaternion& rotation);
		void setTargetMatrix(const Ogre::Vector3& position, const Ogre::Quaternion& rotation);
		void getTargetMatrix(Ogre::Vector3& position, Ogre::Quaternion& rotation) const;

		//! Solver model setter (for future compatibility)
		void setSolverModel(unsigned short solverModel);

		//! Query if the constraint is bilateral
		bool isBilateral() const;

	private:
		inline ndJointKinematicController* getJoint() const
		{
			return static_cast<ndJointKinematicController*>(GetSupportJoint());
		}
	};

	//! Path follow joint.
	/*!
	Simple path follow joint. Used it to make a simple object follow a path or a position.
	*/
	class _OgreNewtExport PathFollow : public Joint
	{
	public:
		//! Constructor
		PathFollow(OgreNewt::Body* child, OgreNewt::Body* pathBody, const Ogre::Vector3& pos);

		//! Destructor
		~PathFollow();

		//! Set spline knots (control points)
		bool setKnots(const std::vector<Ogre::Vector3>& knots);

		//! Create and attach the actual Newton joint
		void createPathJoint(void);

		//! Get path move direction for given control point index
		Ogre::Vector3 getMoveDirection(unsigned int index);

		//! Get interpolated direction for current body position
		Ogre::Vector3 getCurrentMoveDirection(const Ogre::Vector3& currentBodyPosition);

		//! Gets the path length in meters
		Ogre::Real getPathLength(void);

		//! Gets progress (0–1) and path position for current body position
		std::pair<Ogre::Real, Ogre::Vector3> getPathProgressAndPosition(const Ogre::Vector3& currentBodyPosition);

		const ndBezierSpline& getSpline() const { return m_spline; }

		void setLoop(bool loop);
		bool getLoop() const;

		void setClockwise(bool clockwise);
		bool getClockwise() const;
	private:
		OgreNewt::Body* m_childBody;
		OgreNewt::Body* m_pathBody;
		Ogre::Vector3 m_pos;
		ndBezierSpline m_spline;
		bool m_loop;
		bool m_clockwise;
		std::vector<ndBigVector> m_internalControlPoints;
		std::vector<ndBigVector> m_sourceControlPoints;

		// helper to compute direction in Newton space
		ndVector computeDirection(unsigned int index) const;

		void rebuildSpline();
	};

	///! CustomDryRollingFriction
	/*!
	* This joint is usefully to simulate the rolling friction of a rolling ball over a flat surface.
	* Normally this is not important for non spherical objects, but for games like poll, pinball, bolling, golf
	* or any other where the movement of balls is the main objective the rolling friction is a real big problem.
	*/

	class _OgreNewtExport CustomDryRollingFriction : public Joint
	{
	public:
		//! constructor
		/*!
			\param child pointer to the child rigid body.
			\param parent pointer to the parent rigid body (can be nullptr for world sentinel body)
			\param friction friction coefficient for rolling resistance.
		*/
		CustomDryRollingFriction(const OgreNewt::Body* child, const OgreNewt::Body* parent, Ogre::Real friction);

		//! destructor
		~CustomDryRollingFriction();

		//! set the rolling friction coefficient
		void setFrictionCoefficient(Ogre::Real coeff);

		//! get the rolling friction coefficient
		Ogre::Real getFrictionCoefficient() const;

		//! set the contact trail length
		void setContactTrail(Ogre::Real trail);

		//! get the contact trail length
		Ogre::Real getContactTrail() const;

	private:
		inline ndJointDryRollingFriction* getJoint() const
		{
			return static_cast<ndJointDryRollingFriction*>(GetSupportJoint());
		}
	};

	/*!
		@brief Couples the rotation of one body to the linear motion of another.

		Enforces the relation:
			gearRatio * (ω_rot · axisRot) + (v_lin · axisLin) = 0

		Useful for rack-and-pinion, worm-gear, or screw-like constraints.
	*/
	class _OgreNewtExport WormGear : public Joint
	{
	public:
		//! Constructor
		/*!
			\param rotationalBody  The rotating body (e.g., gear or worm)
			\param linearBody      The translating body (e.g., rack)
			\param rotationalPin   Rotation axis of the gear body
			\param linearPin       Translation direction of the linear body
			\param gearRatio       Coupling ratio (translation per radian)
		*/
		WormGear(const OgreNewt::Body* rotationalBody, const OgreNewt::Body* linearBody, const Ogre::Vector3& rotationalPin, const Ogre::Vector3& linearPin, Ogre::Real gearRatio);

		//! Destructor
		~WormGear() override;
	};

	/*!
		Gear and slide joint.
		Couples angular velocity on `rotationalBody` around `rotationalPin` to
		linear velocity of `linearBody` along `linearPin`:

			gearRatio * (ω_rot · rotationalPin) + slideRatio * (v_lin · linearPin) = 0

		Signs of the ratios control the sense of coupling.
	*/
	class _OgreNewtExport CustomGearAndSlide : public Joint
	{
	public:
		CustomGearAndSlide(const OgreNewt::Body* rotationalBody, const OgreNewt::Body* linearBody, const Ogre::Vector3& rotationalPin,
			const Ogre::Vector3& linearPin, Ogre::Real gearRatio, Ogre::Real slideRatio);

		~CustomGearAndSlide() override;
	};

	//! Pulley.
	/*!
	Pulley joint.
	*/
	class _OgreNewtExport Pulley : public Joint
	{
	public:
		/*!
			\param child pointer to the first body
			\param parent pointer to the second body (or nullptr for world)
			\param childPin direction of the pulley on the first body
			\param parentPin direction of the pulley on the second body
			\param pulleyRatio ratio of motion between both sides (n:1)
		*/
		Pulley(const OgreNewt::Body* child,
			const OgreNewt::Body* parent,
			const Ogre::Vector3& childPin,
			const Ogre::Vector3& parentPin,
			Ogre::Real pulleyRatio);

		~Pulley();

		//! set pulley ratio at runtime
		void setRatio(Ogre::Real ratio);

		//! get current pulley ratio
		Ogre::Real getRatio() const;

	private:
		inline ndJointPulley* getJoint() const
		{
			return static_cast<ndJointPulley*>(GetSupportJoint());
		}
	};

	class ndOgreDoubleHingeActuator : public ndJointDoubleHinge
	{
	public:
		D_CLASS_REFLECTION(ndOgreDoubleHingeActuator, ndJointDoubleHinge)

			ndOgreDoubleHingeActuator(
				const ndMatrix& pinAndPivotFrame,
				ndBodyKinematic* const child,
				ndBodyKinematic* const parent,
				ndFloat32 angularRate0, ndFloat32 minAngle0, ndFloat32 maxAngle0,
				ndFloat32 angularRate1, ndFloat32 minAngle1, ndFloat32 maxAngle1);

		virtual ~ndOgreDoubleHingeActuator() {}

		// Axis 0 (like *_0 methods in the ND3 custom joint)
		ndFloat32 GetActuatorAngle0() const;
		void      SetTargetAngle0(ndFloat32 angle);
		void      SetMinAngularLimit0(ndFloat32 limit);
		void      SetMaxAngularLimit0(ndFloat32 limit);
		void      SetAngularRate0(ndFloat32 rate);
		void      SetMaxTorque0(ndFloat32 torque);
		ndFloat32 GetMaxTorque0() const;

		// Axis 1
		ndFloat32 GetActuatorAngle1() const;
		void      SetTargetAngle1(ndFloat32 angle);
		void      SetMinAngularLimit1(ndFloat32 limit);
		void      SetMaxAngularLimit1(ndFloat32 limit);
		void      SetAngularRate1(ndFloat32 rate);
		void      SetMaxTorque1(ndFloat32 torque);
		ndFloat32 GetMaxTorque1() const;

		void JacobianDerivative(ndConstraintDescritor& desc) override;

	protected:
		void WakeBodies();

		void UpdateParameters() override;

		ndFloat32 m_targetAngle0;
		ndFloat32 m_targetAngle1;
		ndFloat32 m_angularRate0;
		ndFloat32 m_angularRate1;
		ndFloat32 m_maxTorque0;
		ndFloat32 m_maxTorque1;
		bool      m_axis0Enable;
		bool      m_axis1Enable;
	};

	//! universal actuator joint.
	/*!
	universal actuator joint.
	*/
	class _OgreNewtExport UniversalActuator : public Joint
	{
	public:
		//! constructor
		/*!
		\param child pointer to the child rigid body.
		\param parent pointer to the parent rigid body. pass nullptr to make the world itself the parent (aka a rigid joint)
		\param pos position of the joint in global space
		*/
		UniversalActuator(const OgreNewt::Body* child,
			const OgreNewt::Body* parent,
			const Ogre::Vector3& pos,
			const Ogre::Degree& angularRate0,
			const Ogre::Degree& minAngle0,
			const Ogre::Degree& maxAngle0,
			const Ogre::Degree& angularRate1,
			const Ogre::Degree& minAngle1,
			const Ogre::Degree& maxAngle1);

		~UniversalActuator();

		Ogre::Real GetActuatorAngle0() const;

		void SetTargetAngle0(const Ogre::Degree& angle0);
		void SetMinAngularLimit0(const Ogre::Degree& limit0);
		void SetMaxAngularLimit0(const Ogre::Degree& limit0);
		void SetAngularRate0(const Ogre::Degree& rate0);
		void SetMaxTorque0(Ogre::Real torque0);
		Ogre::Real GetMaxTorque0() const;

		Ogre::Real GetActuatorAngle1() const;

		void SetTargetAngle1(const Ogre::Degree& angle1);
		void SetMinAngularLimit1(const Ogre::Degree& limit1);
		void SetMaxAngularLimit1(const Ogre::Degree& limit1);
		void SetAngularRate1(const Ogre::Degree& rate1);
		void SetMaxTorque1(Ogre::Real torque1);
		Ogre::Real GetMaxTorque1() const;
	};

	//! Plane constraint.
	/*!
	simple plane constraint.
	*/
	class _OgreNewtExport PlaneConstraint : public Joint
	{
	public:
		//! Constructor
		PlaneConstraint(const OgreNewt::Body* child,
			const OgreNewt::Body* parent,
			const Ogre::Vector3& pos,
			const Ogre::Vector3& normal);

		//! Destructor
		~PlaneConstraint() override;

		//! Enable or disable plane rotation control (optional)
		void enableControlRotation(bool enable);

		//! Query if control rotation is enabled
		bool getControlRotationEnabled() const;
	};

	/*!
		@brief Simple motorized hinge joint.

		Extends Hinge with motor-like behaviour: target angular velocity,
		torque control and optional dual-motor setup (two axes / two parents).
	*/
	class _OgreNewtExport Motor : public Hinge
	{
	public:
		Motor(const Body* child,
			const Body* parent0,
			const Body* parent1,
			const Ogre::Vector3& pin0,
			const Ogre::Vector3& pin1);

		~Motor() override;

		Ogre::Real GetSpeed0() const;
		Ogre::Real GetSpeed1() const;

		void SetSpeed0(Ogre::Real speed0);
		void SetSpeed1(Ogre::Real speed1);

		void SetTorque0(Ogre::Real torque0);
		void SetTorque1(Ogre::Real torque1);

	private:
		bool m_isMotor2;
		Ogre::Real m_targetSpeed0{ 0.0f };
		Ogre::Real m_targetSpeed1{ 0.0f };
		Ogre::Real m_torque0{ 0.0f };
		Ogre::Real m_torque1{ 0.0f };
	};

	/*!
		@brief PD (servo) hinge joint for Newton Dynamics 4.
		Drives the hinge toward a target angle using spring/damper control.
	*/
	class _OgreNewtExport ServoHinge : public Hinge
	{
	public:
		ServoHinge(const Body* child, const Body* parent,
			const Ogre::Vector3& pos, const Ogre::Vector3& pin);
		~ServoHinge() override;

		//! Set desired target angle (in radians)
		void SetTargetAngle(Ogre::Radian angle);

		//! Set stiffness and damping gains
		void SetSpringDamper(Ogre::Real stiffness, Ogre::Real damping);

		//! Update control (call per frame)
		void Update(Ogre::Real timestep);

	private:
		Ogre::Radian m_targetAngle;
		Ogre::Real m_stiffness;
		Ogre::Real m_damping;
	};

	//! wheel joint.
	/*!
	simple wheel joint.
	*/
	class _OgreNewtExport Wheel : public Joint
	{
	public:
		//! Constructor with a single pivot frame.
		Wheel(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos, const Ogre::Vector3& pin);

		//! Constructor with separate child and parent pivot frames.
		Wheel(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& childPos, const Ogre::Vector3& childPin, const Ogre::Vector3& parentPos, const Ogre::Vector3& parentPin);

		//! Destructor.
		~Wheel();

		//! Get the current steering angle (in radians).
		Ogre::Real GetSteerAngle() const;

		//! Set the target steering angle (in degrees).
		void SetTargetSteerAngle(const Ogre::Degree& targetAngle);

		//! Get the current target steering angle (in radians).
		Ogre::Real GetTargetSteerAngle() const;

		//! Set the steering rate (speed at which steering moves toward target).
		void SetSteerRate(Ogre::Real steerRate);

		//! Get the steering rate.
		Ogre::Real GetSteerRate() const;

	private:
		inline ndJointWheel* getJoint() const
		{
			return static_cast<ndJointWheel*>(GetSupportJoint());
		}
	};

	//! Vehicle Tire joint.
	class _OgreNewtExport VehicleTire : public Joint
	{
	public:
		VehicleTire(OgreNewt::Body* child,
			OgreNewt::Body* parentBody,
			const Ogre::Vector3& pos,
			const Ogre::Vector3& pin,
			Vehicle* parent,
			Ogre::Real radius);

		virtual ~VehicleTire();

		// configuration enums (kept for compatibility)
		void setVehicleTireSide(VehicleTireSide tireSide);
		VehicleTireSide getVehicleTireSide() const;

		void setVehicleTireSteer(VehicleTireSteer tireSteer);
		VehicleTireSteer getVehicleTireSteer() const;

		void setVehicleSteerSide(VehicleSteerSide steerSide);
		VehicleSteerSide getVehicleSteerSide() const;

		void setVehicleTireAccel(VehicleTireAccel tireAccel);
		VehicleTireAccel getVehicleTireAccel() const;

		void setVehicleTireBrake(VehicleTireBrake brakeMode);
		VehicleTireBrake getVehicleTireBrake() const;

		// physics params
		void setLateralFriction(Ogre::Real lateralFriction);
		Ogre::Real getLateralFriction() const;

		void setLongitudinalFriction(Ogre::Real longitudinalFriction);
		Ogre::Real getLongitudinalFriction() const;

		void setSpringLength(Ogre::Real springLength);
		Ogre::Real getSpringLength() const;

		void setSmass(Ogre::Real smass);
		Ogre::Real getSmass() const;

		void setSpringConst(Ogre::Real springConst);
		Ogre::Real getSpringConst() const;

		void setSpringDamp(Ogre::Real springDamp);
		Ogre::Real getSpringDamp() const;

		RayCastTire* getRayCastTire();

	private:
		Vehicle* m_vehicle;
		Ogre::Real m_radius;
		ndWheelDescriptor m_desc;

		VehicleTireSide m_tireSide;
		VehicleTireSteer m_tireSteer;
		VehicleSteerSide m_steerSide;
		VehicleTireAccel m_tireAccel;
		VehicleTireBrake m_brakeMode;

		inline ndJointWheel* getJoint() const
		{
			return static_cast<ndJointWheel*>(GetSupportJoint());
		}
	};

	class _OgreNewtExport VehicleMotor : public Joint
	{
	public:
		// Creates an internal ND4 motor in the vehicle model (preferred API)
		VehicleMotor(Vehicle* vehicle, Ogre::Real mass, Ogre::Real radius);

		virtual ~VehicleMotor();

		// ND4 motor controls
		void setMaxRpm(Ogre::Real redLineRpm);
		void setOmegaAccel(Ogre::Real rpmStep);
		void setFrictionLoss(Ogre::Real newtonMeters);
		void setTorqueAndRpm(Ogre::Real rpm, Ogre::Real torque);
		Ogre::Real getRpm() const;

		// Access to underlying ND4 joint
		inline ndMultiBodyVehicleMotor* getJoint() const
		{
			return static_cast<ndMultiBodyVehicleMotor*>(GetSupportJoint());
		}

	private:
		Vehicle* m_vehicle;
		Ogre::Real m_mass;
		Ogre::Real m_radius;
	};

}   // end NAMESPACE OgreNewt

#endif
// _INCLUDE_OGRENEWT_BASICJOINTS

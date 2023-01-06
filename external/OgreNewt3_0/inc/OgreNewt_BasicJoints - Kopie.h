/* 
    OgreNewt Library

    Ogre implementation of Newton Game Dynamics SDK

    OgreNewt basically has no license, you may use any or all of the library however you desire... I hope it can help you in any way.

        by Walaber
        some changes by melven

*/
#ifndef _INCLUDE_OGRENEWT_BASICJOINTS
#define _INCLUDE_OGRENEWT_BASICJOINTS

#include "OgreNewt_Prerequisites.h"
#include "OgreNewt_Joint.h"


// OgreNewt namespace.  all functions and classes use this namespace.
namespace OgreNewt
{

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
	
	//! Ball and Socket joint.
	/*!
	simple ball and socket joint, with limits.
	*/
	class _OgreNewtExport BallAndSocket : public Joint
	{

	public:
		//! constructor
		/*!
		\param child pointer to the child rigid body.
		\param parent pointer to the parent rigid body. pass nullptr to make the world itself the parent (aka a rigid joint)
		\param pos position of the joint in global space
		*/
		BallAndSocket( const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos);

		//! destructor.
		~BallAndSocket();

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
	};

	//! Joint with 6 degree of freedom
	class _OgreNewtExport Joint6Dof : public Joint
	{

	public:
		//! constructor
		/*!
		\param child pointer to the child rigid body.
		\param parent pointer to the parent rigid body. pass nullptr to make the world itself the parent (aka a rigid joint)
		\param pos1 position of the first joint in global space
		\param pos2 position of the second joint in global space
		*/
		Joint6Dof(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos1, const Ogre::Vector3& pos2);

		//! destructor.
		~Joint6Dof();

		void setLinearLimits(const Ogre::Vector3& minLinearLimits, const Ogre::Vector3& maxLinearLimits) const;
		
		void setYawLimits(Ogre::Radian minLimits, Ogre::Radian maxLimits) const;

		void setPitchLimits(Ogre::Radian minLimits, Ogre::Radian maxLimits) const;

		void setRollLimits(Ogre::Radian minLimits, Ogre::Radian maxLimits) const;
	};

	//! Controlled Ball and Socket joint.
	/*!
	ball and socket joint, that is rotated controlled
	*/
	//class _OgreNewtExport ControlledBallAndSocket : public Joint
	//{

	//public:
	//	//! constructor
	//	/*!
	//	\param child pointer to the child rigid body.
	//	\param parent pointer to the parent rigid body. pass nullptr to make the world itself the parent (aka a rigid joint)
	//	\param pos position of the joint in global space
	//	*/
	//	ControlledBallAndSocket(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos);

	//	//! destructor.
	//	~ControlledBallAndSocket();

	//	void setAngularVelocity(Ogre::Real angularVelocity);

	//	//! set data for the joints rotation
	//	/*!
	//	\param pitch
	//	\param yaw
	//	\param roll
	//	*/
	//	void setPitchYawRollAngle(Ogre::Degree pitch, Ogre::Degree yaw, Ogre::Degree roll) const;

	//private:
	//	Ogre::Real m_angularVelocity;
	//	Ogre::Real m_pitch;
	//	Ogre::Real m_yaw;
	//	Ogre::Real m_roll;
	//};

	//! Ragdoll motor joint.
	/*!
	similiar to ball and socket joint but for ragdolls and motor mode
	*/
	//class _OgreNewtExport RagDollMotor : public Joint
	//{

	//public:
	//	//! constructor
	//	/*!
	//	\param child pointer to the child rigid body.
	//	\param parent pointer to the parent rigid body. pass nullptr to make the world itself the parent (aka a rigid joint)
	//	\param pos position of the joint in global space
	//	\param dofCount Sets the degree of freedom level (1, 2, 3)
	//	*/
	//	RagDollMotor(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos, unsigned int dofCount);

	//	//! destructor.
	//	~RagDollMotor();

	//	//! set limits for the joints rotation
	//	/*!
	//	\param maxCone max angle for "swing"
	//	\param minTwist min angle for "twist"
	//	\param maxTwist max angle for "twist"
	//	*/
	//	void setLimits(Ogre::Degree maxCone, Ogre::Degree minTwist, Ogre::Degree maxTwist) const;
	//	
	//	void setMode(bool ragDollOrMotor);

	//	//! Sets torque for motor
	//	/*!
	//	torque can be applied at the same time as other constraints (omega/angle/brake)
	//	\param torque torque to apply
	//	*/
	//	void setTorgue(Ogre::Real torgue);
	//private:
	//	unsigned int m_dofCount;
	//};

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
		\param parent pointer to the parent rigid body. pass nullptr to make the world itself the parent (aka a rigid joint)
		\param pos position of the joint in global space
		*/
		CorkScrew( const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos );

		//! destructor.
		~CorkScrew();

		//! enables limits for the joints rotation
		/*!
		\param linearLimits
		\param angularLimits
		*/
		void enableLimits(bool linearLimits, bool angularLimits);

		//! set limits for the joints rotation
		/*!
		\param minLinearDistance min distance 
		\param maxLinearDistance max distance
		*/
		void setLinearLimits(Ogre::Real minLinearDistance, Ogre::Real maxLinearDistance);


		//! set limits for the joints rotation
		/*!
		\param minAngularAngle min angle for rotation
		\param maxAngularAngle max angle for rotation
		*/
		void setAngularLimits(Ogre::Degree minAngularAngle, Ogre::Degree maxAngularAngle);

	};



	//! hinge joint.
	/*!
	simple hinge joint.  implement motors/limits through a callback.
	*/
	class _OgreNewtExport Hinge : public Joint
	{

	public:

		//! constructor
		/*!
		\param child pointer to the child rigid body.
		\param parent pointer to the parent rigid body. pass nullptr to make the world itself the parent (aka a rigid joint)
		\param pin direction of the joint pin in global space
		*/
		Hinge (const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos, const Ogre::Vector3& pin );

		//! destructor
		~Hinge();

		//! enable hinge limits.
		void enableLimits(bool state);

		//! set minimum and maximum hinge limits.
		void setLimits(Ogre::Degree minAngle, Ogre::Degree maxAngle);

		//! get the relative joint angle around the hinge pin.
		Ogre::Radian getJointAngle () const;

		//! get the relative joint angle around the hinge pin.
		Ogre::Radian getJointAngularVelocity () const;

		//! get the joint pin in global space
		Ogre::Vector3 getJointPin () const;

		//! sets desired rotational velocity of the joint
		//! E.g. 360 will rotate one round in one second, 180 will rotate a round in 2 seconds, 90 will rotate a round in 4 seconds etc.
		void setDesiredOmega(Ogre::Degree omega);

		//! set desired angle and optionally minimum and maximum friction
		void setDesiredAngle(Ogre::Degree angle, Ogre::Real minFriction = 0.0f, Ogre::Real maxFriction = 0.0f);

		//! sets hinge torque
		/*!
			torque can be applied at the same time as other constraints (omega/angle/brake)
			\param torque torque to apply
		*/
		void setTorque(Ogre::Real torque);

		//! apply hinge brake for one frame.
		//   0: brake with infinite force
		// > 0: brake with limited force
		void setBrake(Ogre::Real maxForce = 0);

		void setSpringAndDamping(bool state, Ogre::Real springDamperRelaxation, Ogre::Real spring, Ogre::Real damper);

		//! clears the motor constraints
		void clearConstraints() { m_constraintType = CONSTRAINT_NONE; m_torqueOn = false; }

		virtual void submitConstraint(Ogre::Real timestep, int threadindex);

	protected:
		enum ConstraintType { CONSTRAINT_NONE, CONSTRAINT_OMEGA, CONSTRAINT_ANGLE, CONSTRAINT_BRAKE };

		ConstraintType m_constraintType;
		ConstraintType m_lastConstraintType;

		Ogre::Radian m_desiredOmega;
		bool m_directionChange;
		Ogre::Real m_oppositeDir;
		Ogre::Degree m_minAngle;
		Ogre::Degree m_maxAngle;

		Ogre::Radian m_desiredAngle;

		Ogre::Real m_motorTorque;
		Ogre::Real m_motorMinFriction;
		Ogre::Real m_motorMaxFriction;
		Ogre::Real m_brakeMaxForce;
		
		bool m_torqueOn;
	};


	//! slider joint.
	/*!
	simple slider joint.  implement motors/limits through a callback.
	*/
	class _OgreNewtExport Slider : public Joint
	{
	public:

		//! constructor
		/*!
		\param child pointer to the child rigid body.
		\param parent pointer to the parent rigid body. pass nullptr to make the world itself the parent (aka a rigid joint)
		\param pin direction of the joint pin in global space
		*/
		Slider(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos, const Ogre::Vector3& pin );

		//! destructor.
		~Slider();

		//! enable limits.
		void enableLimits(bool state);

		//! set minimum and maximum stops limits.
		void setLimits(Ogre::Real minStopDist, Ogre::Real maxStopDist);

		void setSpringAndDamping(bool state, Ogre::Real springDamperRelaxation, Ogre::Real spring, Ogre::Real damper);
	};



	class _OgreNewtExport ActiveSliderJoint : public Joint
	{
	public:
		ActiveSliderJoint(OgreNewt::Body* currentBody, OgreNewt::Body* predecessorBody, const Ogre::Vector3& anchorPosition, const Ogre::Vector3& pin,
			Ogre::Real minStopDistance, Ogre::Real maxStopDistance, Ogre::Real moveSpeed, bool repeat, bool directionChange, bool activated);

		virtual ~ActiveSliderJoint()
		{
		}

		void setActivated(bool activated);

		//! enable limits.
		void enableLimits(bool state);

		//! set minimum and maximum stops limits.
		void setLimits(Ogre::Real minStopDist, Ogre::Real maxStopDist);

		void setMoveSpeed(Ogre::Real moveSpeed);

		void setRepeat(bool repeat);

		void setDirectionChange(bool directionChange);

		int getCurrentDirection(void) const;
	};

	//! Sliding contact joint.
	/*!
	Sliding contact joint.
	*/
	class _OgreNewtExport SlidingContact : public Joint
	{
	public:

		//! constructor
		/*!
		\param child pointer to the child rigid body.
		\param parent pointer to the parent rigid body. pass nullptr to make the world itself the parent (aka a rigid joint)
		\param pin direction of the joint pin in global space
		*/
		SlidingContact(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos, const Ogre::Vector3& pin);

		//! destructor.
		~SlidingContact();

		//! enable linear limits.
		void enableLinearLimits(bool state);

		//! enable angular limits.
		void enableAngularLimits(bool state);

		//! set minimum and maximum stops limits.
		void setLinearLimits(Ogre::Real minStopDist, Ogre::Real maxStopDist);

		//! set minimum and maximum angular limits.
		void setAngularLimits(Ogre::Degree minAngle, Ogre::Degree maxAngle);

		void setSpringAndDamping(bool state, Ogre::Real springDamperRelaxation, Ogre::Real spring, Ogre::Real damper);
	};


	//! UpVector joint.
	/*!
	simple upvector joint.  upvectors remove all rotation except for a single pin.  useful for character controllers, etc.
	*/
	class _OgreNewtExport UpVector : public Joint
	{
	public:
		//! constructor
		/*
		\param world pointer to the OgreNewt::World.
		\param body pointer to the body to apply the upvector to.
		\param pin direction of the upvector in global space.
		*/
		UpVector( const Body* body, const Ogre::Vector3& pin );

		//! destructor
		~UpVector();

		//! set the pin direction.
		/*
		by calling this function in realtime, you can effectively "animate" the pin.
		*/
		void setPin( const Ogre::Vector3& pin );

		//! get the current pin direction.
		const Ogre::Vector3& getPin() const;

	private:
		Ogre::Vector3 m_pin;
	};

#if 0
	//! this class represents a Universal joint.
	/*!
	simple universal joint.  implement motors/limits through a callback.
	*/
	class _OgreNewtExport Universal : public Joint
	{

	public:

		//! custom universal callback function.
		/*!
		use the setCallback() function to assign your custom function to the joint.
		*/
		typedef void(*UniversalCallback)( Universal* me );

		//! constructor
		/*!
		\param world pointer to the OgreNewt::World
		\param child pointer to the child rigid body.
		\param parent pointer to the parent rigid body. pass nullptr to make the world itself the parent (aka a rigid joint)
		\param pos position of the joint in global space
		\param pin0 direction of the first axis of rotation in global space
		\param pin1 direction of the second axis of rotation in global space
		*/
		Universal( const World* world, const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos, const Ogre::Vector3& pin0, const Ogre::Vector3& pin1 );

		//! destructor
		~Universal();

		//! get the angle around pin0.
		Ogre::Radian getJointAngle0() const { return Ogre::Radian(NewtonUniversalGetJointAngle0( m_joint )); }

		//! get the angle around pin1.
		Ogre::Radian getJointAngle1() const { return Ogre::Radian(NewtonUniversalGetJointAngle1( m_joint )); }

		//! get the rotational velocity around pin0.
		Ogre::Real getJointOmega0() const { return (Ogre::Real)NewtonUniversalGetJointOmega0( m_joint ); }

		//! get the rotational velocity around pin1.
		Ogre::Real getJointOmega1() const { return (Ogre::Real)NewtonUniversalGetJointOmega1( m_joint ); }

		//! get the force on the joint.
		Ogre::Vector3 getJointForce() const;

		//! set a custom callback for controlling this joint.
		/*!
		Joint callbacks allow you to make complex joint behavior such as limits or motors.  just make a custom static function that
		accepts a pointer to a OgreNewt::BasicJoints::Universal as the single parameter.  this function will be called automatically every
		time you upate the World.
		*/
		void setCallback( UniversalCallback callback ) { m_callback = callback; }

		////////// CALLBACK COMMANDS ///////////
		// the following commands are only valid from inside a hinge callback function

		//! set the acceleration around a particular pin.
		/*
		this function can only be called from within a custom callback.
		\param accel desired acceleration
		\param axis which pin to use (0 or 1)
		*/
		void setCallbackAccel( Ogre::Real accel, unsigned axis );

		//! set the minimum friction around a particular pin
		/*
		this function can only be called from within a custom callback.
		\param min minimum friction
		\param axis which pin to use (0 or 1)
		*/
		void setCallbackFrictionMin( Ogre::Real min, unsigned axis );

		//! set the maximum friction around a particular pin.
		/*
		this function can only be called from within a custom callback.
		\param max maximum friction
		\param axis which pin to use (0 or 1)
		*/
		void setCallbackFrictionMax( Ogre::Real max, unsigned axis );

		//! get the current phsics timestep.
		/*
		this function can only be called from within a custom callback.
		*/
		Ogre::Real getCallbackTimestep() const;

		//! calculate the acceleration neccesary to stop the joint at the specified angle on pin 0.
		/*!
		For implementing joint limits.
		This command is only valid when used inside a custom  callback.
		*/
		Ogre::Real calculateStopAlpha0( Ogre::Real angle ) const;

		//! calculate the acceleration neccesary to stop the joint at the specified angle on pin 1.
		/*!
		For implementing joint limits.
		This command is only valid when used inside a custom  callback.
		*/
		Ogre::Real calculateStopAlpha1( Ogre::Real angle ) const;

	protected:

		//! newton callback.  used internally.
		static unsigned _CDECL newtonCallback( const NewtonJoint* universal, NewtonHingeSliderUpdateDesc* desc );

		UniversalCallback m_callback;
		NewtonHingeSliderUpdateDesc* m_desc;

		unsigned m_retval;
	};



	//! UpVector joint.
	/*!
	simple upvector joint.  upvectors remove all rotation except for a single pin.  useful for character controllers, etc.
	*/
	class _OgreNewtExport UpVector : public Joint
	{

	public:
		//! constructor
		/*
		\param world pointer to the OgreNewt::World.
		\param body pointer to the body to apply the upvector to.
		\param pin direction of the upvector in global space.
		*/
		UpVector( const World* world, const Body* body, const Ogre::Vector3& pin );

		//! destructor
		~UpVector();

		//! set the pin direction.
		/*
		by calling this function in realtime, you can effectively "animate" the pin.
		*/
		void setPin( const Ogre::Vector3& pin ) const { NewtonUpVectorSetPin( m_joint, &pin.x ); }

		//! get the current pin direction.
		Ogre::Vector3 getPin() const;
	};



	//! namespace for pre-built custom joints
	namespace PrebuiltCustomJoints
	{

		//! Custom2DJoint class
		/*!
		This class represents a joint that limits movement to a plane, and rotation only around the normal of that
		plane.  This can be used to create simple 2D simulations.  it also supports limits and acceleration for spinning.
		This joint has been used in a few projects, but is not 100% fully-tested.
		*/
		class _OgreNewtExport Custom2DJoint : public OgreNewt::CustomJoint
		{
		public:
			//! constructor
			Custom2DJoint( const OgreNewt::Body* body, const Ogre::Vector3& pin );

			//! destructor
			~Custom2DJoint() {}

			//! overloaded function that applies the actual constraint.
			void submitConstraint( Ogre::Real timeStep, int threadIndex );

			//! get the current angle of the joint.
			Ogre::Radian getAngle() const { return mAngle; }

			//! set rotational limits for the joint.
			void setLimits( Ogre::Degree min, Ogre::Degree max ) { mMin = min, mMax = max; }

			//! sets whether to enable limits or not for the joint.
			void setLimitsOn( bool onoff ) { mLimitsOn = onoff; }

			//! returns whether limits are turned on or off for the joint.
			bool getLimitsOn() const { return mLimitsOn; }

			//! adds rotational acceleration to the joint (like a motor)
			void addAccel( Ogre::Real accel ) { mAccel = accel; }

			//! resets the joint angle to 0.  this simply sets the internal variable to zero.
			//! you might want to call this for example after resetting a body.
			void resetAngle() { mAngle = Ogre::Radian(0.0f); }

			//! get the pin.
			Ogre::Vector3 getPin() { return mPin; }

		private:
			Ogre::Vector3 mPin;
			Ogre::Quaternion mLocalOrient0, mLocalOrient1;
			Ogre::Vector3 mLocalPos0, mLocalPos1;

			Ogre::Radian mAngle;

			Ogre::Radian mMin;
			Ogre::Radian mMax;

			bool mLimitsOn;

			Ogre::Real mAccel;
		};

		//! CustomFixedJoint
		/*!
		This joint implements a fully fixed joint, which removes all DOF, creating a completely fixed connection between bodies.
		This is probably the most expensive kind of joint, and should only be used when really needed.
		*/
		class _OgreNewtExport CustomRigidJoint : public OgreNewt::CustomJoint
		{
		public:
			CustomRigidJoint( OgreNewt::Body* child, OgreNewt::Body* parent, Ogre::Vector3 dir, Ogre::Vector3 pos);
			~CustomRigidJoint();

			void submitConstraint( Ogre::Real timeStep, int threadIndex );

		private:
			Ogre::Vector3 mLocalPos0;
			Ogre::Vector3 mLocalPos1;

			Ogre::Quaternion mLocalOrient0;
			Ogre::Quaternion mLocalOrient1;
		};

	}   // end NAMESPACE PrebuiltCustomJoints

#endif


	//! Kinematic controller joint.
	/*!
	simple Kinematic controller. used it to make a simple object follow a path or a position.
	*/
	class _OgreNewtExport KinematicController : public Joint
	{

		public:

		//! constructor
		/*!
		\param child pointer to the controlled rigid body.
		\param attachment point in global space
		*/
		KinematicController(const OgreNewt::Body* child, const Ogre::Vector3& pos);

		//! destructor.
		~KinematicController();

		//! enable limits. 
		/*!
		\param mode 1 at like control by position and rotation, 0 control rotation only
		*/
		void setPickingMode (int mode);

		//! set the linear acceleration the joint can take before the joint is violated . 
		/*!
		\param accel maximum acceleration at the attachment point
		*/
		void setMaxLinearFriction(Ogre::Real accel); 

		//! set the angular acceleration the joint can take before the joint is violated . 
		/*!
		\param alpha maximum acceleration at the attachment point
		*/
		void setMaxAngularFriction(Ogre::Real alpha); 

		//! set the position part of the attachment matrix
		/*!
		\param position new destination position in global space
		*/
		void setTargetPosit (const Ogre::Vector3& position);

		//! set the orientation part of the attachment matrix
		/*!
		\param rotation new destination position in global space
		*/
		void setTargetRotation (const Ogre::Quaternion& rotation); 

		//! set the position and orientation part of the attachment matrix
		/*!
		\param position new destination position in global space
		\param rotation new destination position in global space
		*/
		void setTargetMatrix (const Ogre::Vector3& position, const Ogre::Quaternion& rotation); 

		//! set the position and orientation part of the attachment matrix
		/*!
		\param position new destination position in global space
		\param rotation new destination position in global space
		*/
		void getTargetMatrix (Ogre::Vector3& position, Ogre::Quaternion& rotation) const;

	};

	//! Path follow joint.
	/*!
	Simple path follow joint. Used it to make a simple object follow a path or a position.
	*/
	class _OgreNewtExport PathFollow : public Joint
	{

	public:

		//! constructor
		/*!
		\param child pointer to the controlled rigid body.
		\param position point in global space
		*/
		PathFollow(OgreNewt::Body* child, OgreNewt::Body* pathBody, const Ogre::Vector3& pos);

		~PathFollow();

		void setKnots(const std::vector<Ogre::Vector3>& knots);
	private:
		OgreNewt::Body* m_childBody;
		OgreNewt::Body* m_pathBody;
		Ogre::Vector3 m_pos;
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
		CustomDryRollingFriction(OgreNewt::Body* child, Ogre::Real radius, Ogre::Real rollingFrictionCoefficient);
		~CustomDryRollingFriction();
	};

	//! hinge Gear.
	/*!
	Gear joint.
	*/
	class _OgreNewtExport Gear : public Joint
	{
	public:

		/*
		* @brief This joint is for use in conjunction with Hinge or other spherical joints
		* it is useful for establishing synchronization between the phase angle and the 
		* relative angular velocity of two spinning disk according to the law of gears
		* velErro = -(W0 * r0 + W1 *  r1)
		* where w0 and W1 are the angular velocity
		* r0 and r1 are the radius of the spinning disk
		*/
		
		//! constructor
		/*!
		\param child pointer to the child rigid body.
		\param parent pointer to the parent rigid body. pass nullptr to make the world itself the parent (aka a rigid joint)
		\param pin direction of the joint pin in global space
		*/
		Gear(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& childPin, const Ogre::Vector3& parentPin, Ogre::Real gearRatio);

		~Gear();
	};

	//! hinge Gear.
	/*!
	Gear joint.
	*/
	class _OgreNewtExport WormGear : public Joint
	{
	public:

		/*
		* @brief This joint is for used in conjunction with Hinge or other spherical joints
		* it is useful for establishing synchronization between the phase angle of the 
		* relative angular velocity of two spinning disk according to the law of gears
		* velErro = -(W0 * r0 + W1 * r1) where w0 and W1 are the angular velocity
		* r0 and r1 are the radius of the spinning disk
		*/

		//! constructor
		/*!
		\param child pointer to the child rigid body.
		\param parent pointer to the parent rigid body. pass nullptr to make the world itself the parent (aka a rigid joint)
		\param pin direction of the joint pin in global space
		*/
		WormGear(const OgreNewt::Body* rotationalBody, const OgreNewt::Body* linearBody, const Ogre::Vector3& rotationalPin, const Ogre::Vector3& linearPin, Ogre::Real gearRatio);

		~WormGear();
	};

	//! hinge Gear.
	/*!
	Gear and slide joint.
	*/
	class _OgreNewtExport CustomGearAndSlide : public Joint
	{
	public:

		//! constructor
		/*!
		\param child pointer to the child rigid body.
		\param parent pointer to the parent rigid body. pass nullptr to make the world itself the parent (aka a rigid joint)
		\param pin direction of the joint pin in global space
		*/
		CustomGearAndSlide(const OgreNewt::Body* rotationalBody, const OgreNewt::Body* linearBody, const Ogre::Vector3& rotationalPin, const Ogre::Vector3& linearPin, 
			Ogre::Real gearRatio, Ogre::Real slidRatio);

		~CustomGearAndSlide();
	};

	//! Pulley.
	/*!
	Pulley joint.
	*/
	class _OgreNewtExport Pulley : public Joint
	{
	public:

		/*
		* @brief This joint is for used in conjunction with Slider of other linear joints
		* it is useful for establishing synchronization between the relative position 
		* or relative linear velocity of two sliding bodies according to the law of pulley
		* velErro = -(v0 + n * v1) where v0 and v1 are the linear velocity
		* n is the number of turn on the pulley, in the case of this joint N could
		* be a value with friction, as this joint is a generalization of the pulley idea.
		*/

		//! constructor
		/*!
		\param child pointer to the child rigid body.
		\param parent pointer to the parent rigid body. pass nullptr to make the world itself the parent (aka a rigid joint)
		\param pin direction of the joint pin in global space
		*/
		Pulley(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& childPin, const Ogre::Vector3& parentPin, Ogre::Real pulleyRatio);

		~Pulley();
	};

	//! universal joint.
	/*!
	Universal joint, that can be pinned and rotate about two axes either manually or automatically via motor and limits.
	*/
	class _OgreNewtExport Universal : public Joint
	{

	public:
		//! constructor
		/*!
		\param child pointer to the child rigid body.
		\param parent pointer to the parent rigid body. pass nullptr to make the world itself the parent (aka a rigid joint)
		\param pos position of the joint in global space
		*/
		Universal(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos);

		//! destructor.
		~Universal();

		//! enable universal joint limits for first axis.
		void enableLimits0(bool state0);

		//! set minimum and maximum universal joint limits for first axis.
		void setLimits0(Ogre::Degree minAngle0, Ogre::Degree maxAngle0);

		//! enable universal joint limits for second axis.
		void enableLimits1(bool state1);

		//! set minimum and maximum universal joint limits for second axis.
		void setLimits1(Ogre::Degree minAngle1, Ogre::Degree maxAngle1);

		//! enable motor for first axis.
		void enableMotor0(bool state0, Ogre::Real motorSpeed0);

		void setFriction0(Ogre::Real frictionTorque);

		void setFriction1(Ogre::Real frictionTorque);

		void setAsSpringDamper0(bool state, Ogre::Real springDamperRelaxation, Ogre::Real spring, Ogre::Real damper);

		void setAsSpringDamper1(bool state, Ogre::Real springDamperRelaxation, Ogre::Real spring, Ogre::Real damper);

		void setHardMiddleAxis(bool state);
		
	};

}   // end NAMESPACE OgreNewt

#endif
// _INCLUDE_OGRENEWT_BASICJOINTS


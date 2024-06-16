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
		
		void setYawLimits(const Ogre::Degree& minLimits, const Ogre::Degree& maxLimits) const;

		void setPitchLimits(const Ogre::Degree& minLimits, const Ogre::Degree& maxLimits) const;

		void setRollLimits(const Ogre::Degree& minLimits, const Ogre::Degree& maxLimits) const;
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
		\param parent pointer to the parent rigid body. pass nullptr to make the world itself the parent (aka a rigid joint)
		\param pos position of the joint in global space
		*/
		CorkScrew(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos);

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
		Hinge(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos, const Ogre::Vector3& pin);

		//! constructor
		/*!
		\param child pointer to the child rigid body.
		\param parent pointer to the parent rigid body. pass nullptr to make the world itself the parent (aka a rigid joint)
		\param pin direction of the joint pin in global space
		*/
		Hinge(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& childPos, const Ogre::Vector3& childPin, const Ogre::Vector3& parentPos, const Ogre::Vector3& parentPin);

		//! destructor
		~Hinge();

		//! enable hinge limits.
		void EnableLimits(bool state);

		//! set minimum and maximum hinge limits.
		void SetLimits(Ogre::Degree minAngle, Ogre::Degree maxAngle);

		//! get the relative joint angle around the hinge pin.
		Ogre::Radian GetJointAngle() const;

		//! get the relative joint angle around the hinge pin.
		Ogre::Radian GetJointAngularVelocity() const;

		//! get the joint pin in global space
		Ogre::Vector3 GetJointPin() const;

		//! sets hinge torque
		/*!
			torque can be applied at the same time as other constraints (omega/angle/brake)
			\param torque torque to apply
		*/
		void SetTorque(Ogre::Real torque);

		void SetSpringAndDamping(bool enable, bool massIndependent, Ogre::Real springDamperRelaxation, Ogre::Real spring, Ogre::Real damper);
		
		void SetFriction(Ogre::Real friction);
	};
	
	//! hinge actuator joint.
	/*!
	hinge actuator joint.
	*/
	class _OgreNewtExport HingeActuator : public Joint
	{

	public:

		//! constructor
		/*!
		\param child pointer to the child rigid body.
		\param parent pointer to the parent rigid body. pass nullptr to make the world itself the parent (aka a rigid joint)
		\param pos of the joint in global space
		\param pin direction of the joint pin in global space
		*/
		HingeActuator(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos, const Ogre::Vector3& pin, const Ogre::Degree& angularRate, const Ogre::Degree& minAngle, const Ogre::Degree& maxAngle);

		//! destructor
		~HingeActuator();

		Ogre::Real GetActuatorAngle() const;
		
		void SetTargetAngle(const Ogre::Degree& angle);
		
		void SetMinAngularLimit(const Ogre::Degree& limit);
		
		void SetMaxAngularLimit(const Ogre::Degree& limit);
		
		void SetAngularRate(const Ogre::Degree& rate);
		
		void SetMaxTorque(Ogre::Real torque);
		
		Ogre::Real GetMaxTorque() const;

		void SetSpringAndDamping(bool enable, bool massIndependent, Ogre::Real springDamperRelaxation, Ogre::Real spring, Ogre::Real damper);

		// error: max_dof will be 7 instead 6!
		// void SetFriction(Ogre::Real friction);
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

		void setFriction(Ogre::Real friction);

		void setSpringAndDamping(bool state, Ogre::Real springDamperRelaxation, Ogre::Real spring, Ogre::Real damper);
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
		SliderActuator(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos, const Ogre::Vector3& pin, Ogre::Real linearRate, Ogre::Real minPosition, Ogre::Real maxPosition);

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
		FlexyPipeHandleJoint(OgreNewt::Body* currentBody, const Ogre::Vector3& pin);

		virtual ~FlexyPipeHandleJoint()
		{
		}

		void setVelocity(const Ogre::Vector3& velocity, Ogre::Real dt);

		void setOmega(const Ogre::Vector3& omega, Ogre::Real dt);
	};

	class _OgreNewtExport FlexyPipeSpinnerJoint : public Joint
	{
	public:
		FlexyPipeSpinnerJoint(OgreNewt::Body* currentBody, OgreNewt::Body* predecessorBody, const Ogre::Vector3& anchorPosition, const Ogre::Vector3& pin);

		virtual ~FlexyPipeSpinnerJoint()
		{
		}
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

	class _OgreNewtExport Plane2DUpVectorJoint : public Joint
	{
	public:
		Plane2DUpVectorJoint(const OgreNewt::Body* child, const Ogre::Vector3& pos, const Ogre::Vector3& normal, const Ogre::Vector3& pin);

		virtual ~Plane2DUpVectorJoint()
		{
		}

		void setPin(const Ogre::Vector3& pin);
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

		KinematicController(const OgreNewt::Body* child, const Ogre::Vector3& pos, const OgreNewt::Body* referenceBody);

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
		\param rotation new destination rotation in global space
		*/
		void setTargetRotation (const Ogre::Quaternion& rotation); 

		//! set the position and orientation part of the attachment matrix
		/*!
		\param position new destination position in global space
		\param rotation new destination rotation in global space
		*/
		void setTargetMatrix (const Ogre::Vector3& position, const Ogre::Quaternion& rotation); 

		//! set the position and orientation part of the attachment matrix
		/*!
		\param position new destination position in global space
		\param rotation new destination rotation in global space
		*/
		void getTargetMatrix (Ogre::Vector3& position, Ogre::Quaternion& rotation) const;

		void setMaxSpeed(Ogre::Real speedInMetersPerSeconds);

		void setMaxOmega(const Ogre::Degree& speedInDegreesPerSeconds);

		void setAngularViscousFrictionCoefficient(Ogre::Real coefficient);

		void resetAutoSleep(void);

		void setSolverModel(unsigned short solverModel);
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

		bool setKnots(const std::vector<Ogre::Vector3>& knots);

		void createPathJoint(void);

		Ogre::Vector3 getMoveDirection(unsigned int index);

		Ogre::Vector3 getCurrentMoveDirection(const Ogre::Vector3& currentBodyPosition);

		/*!
		* Gets the path length in meters
		*/
		Ogre::Real getPathLength(void);

		/*!
		* Gets the path progress in percent and the body position on the path for the given current body position
		*/
		std::pair<Ogre::Real, Ogre::Vector3> getPathProgressAndPosition(const Ogre::Vector3& currentBodyPosition);
	private:
		OgreNewt::Body* m_childBody;
		OgreNewt::Body* m_pathBody;
		Ogre::Vector3 m_pos;
		std::vector<dBigVector> m_internalControlPoints;
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
		\param pin direction of the joint pin in global space
		*/
		Universal(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos, const Ogre::Vector3& pin);

		Universal(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& childPos, const Ogre::Vector3& childPin, const Ogre::Vector3& parentPos, const Ogre::Vector3& parentPin);

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
		\param world pointer to the OgreNewt::World
		\param child pointer to the child rigid body.
		\param parent pointer to the parent rigid body. pass nullptr to make the world itself the parent (aka a rigid joint)
		\param pos position of the joint in global space
		*/
		UniversalActuator(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos, 
						const Ogre::Degree& angularRate0, const Ogre::Degree& minAngle0, const Ogre::Degree& maxAngle0,
						const Ogre::Degree& angularRate1, const Ogre::Degree& minAngle1, const Ogre::Degree& maxAngle1);

		//! destructor
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
		//! constructor
		/*
		\param child pointer to the child rigid body.
		\param parent pointer to the parent rigid body. pass nullptr to make the world itself the parent (aka a rigid joint)
		\param pos of the plane in global space.
		\param normal of the plane.
		*/
		PlaneConstraint(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos, const Ogre::Vector3& normal);

		//! destructor
		~PlaneConstraint();
	};

	//! motor joint.
	/*!
	simple motor joint.
	*/
	class _OgreNewtExport Motor : public Joint
	{
	public:

		//! constructor
		/*!
		\param child pointer to the child rigid body.
		\param parent0 pointer to the parent0 rigid body.
		\param parent1 pointer to the parent1 rigid body. May be null if just one motor is sufficient.
		\param pin0 direction of the first joint pin in global space
		\param pin1 direction of the second joint pin in global space. May be Vector3::ZERO if just one motor is sufficient.
		*/
		Motor(const OgreNewt::Body* child, const OgreNewt::Body* parent0, const OgreNewt::Body* parent1, const Ogre::Vector3& pin0, const Ogre::Vector3& pin1);

		//! destructor
		~Motor();

		//! get the speed 0
		Ogre::Real GetSpeed0() const;

		//! get the speed 1
		Ogre::Real GetSpeed1() const;

		//! sets motor speed 0
		/*!
			\param speed0 speed0 to apply
		*/
		void SetSpeed0(Ogre::Real speed0);

		//! sets motor speed 1
		/*!
			\param speed1 speed1 to apply
		*/
		void SetSpeed1(Ogre::Real speed1);

		//! sets motor torque0
		/*!
			\param torque0 torque0 to apply
		*/
		void SetTorque0(Ogre::Real torque0);

		//! sets motor torque0
		/*!
			\param torque0 torque0 to apply
		*/
		void SetTorque1(Ogre::Real torque1);
	private:
		bool m_isMotor2;
	};

	//! wheel joint.
	/*!
	simple wheel joint.
	*/
	class _OgreNewtExport Wheel : public Joint
	{

	public:
		//! constructor
		/*!
		\param child pointer to the child rigid body.
		\param parent pointer to the parent rigid body. pass nullptr to make the world itself the parent (aka a rigid joint)
		\param pos position in global space
		\param pin direction of the joint pin in global space
		*/
		Wheel(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& pos, const Ogre::Vector3& pin);

		//! constructor
		/*!
		\param child pointer to the child rigid body.
		\param parent pointer to the parent rigid body. pass nullptr to make the world itself the parent (aka a rigid joint)
		\param childPos child position of the joint in global space
		\param childPin direction of the child joint pin in global space
		\param parentPos parent position of the joint in global space
		\param parentPin direction of the parent joint pin in global space
		*/
		Wheel(const OgreNewt::Body* child, const OgreNewt::Body* parent, const Ogre::Vector3& childPos, const Ogre::Vector3& childPin, const Ogre::Vector3& parentPos, const Ogre::Vector3& parentPin);

		//! destructor
		~Wheel();

		//! get the current steering angle
		Ogre::Real GetSteerAngle() const;

		//! sets the target steering angle in degree
		/*!
			\param targetAngle target steerting angle to apply
		*/
		void SetTargetSteerAngle(const Ogre::Degree& targetAngle);

		//! get the target steering angle
		Ogre::Real GetTargetSteerAngle() const;

		//! sets the target steering angle in degree
		/*!
			\param targetAngle target steerting angle to apply
		*/
		void SetSteerRate(Ogre::Real steerRate);

		//! get the steering rate
		Ogre::Real GetSteerRate() const;
	};

	//! Vehicle Tire joint.
	class _OgreNewtExport VehicleTire : public Joint
	{

	public:
		//! constructor
		/*!
		\param child pointer to the child rigid body.
		\param parent pointer to the parent rigid body. pass nullptr to make the world itself the parent (aka a rigid joint)
		\param pos position of the joint in global space
		*/
		VehicleTire(OgreNewt::Body* child, OgreNewt::Body* parentBody, const Ogre::Vector3& pos, const Ogre::Vector3& pin, Vehicle* parent, Ogre::Real radius);

		//! destructor.
		virtual ~VehicleTire();

		void setVehicleTireSide(VehicleTireSide tireSide);

		VehicleTireSide getVehicleTireSide(void) const;

		void setVehicleTireSteer(VehicleTireSteer tireSteer);

		VehicleTireSteer getVehicleTireSteer(void) const;

		void setVehicleSteerSide(VehicleSteerSide steerSide);

		VehicleSteerSide getVehicleSteerSide(void) const;

		void setVehicleTireAccel(VehicleTireAccel tireAccel);

		VehicleTireAccel getVehicleTireAccel(void) const;

		void setVehicleTireBrake(VehicleTireBrake brakeMode);
		
		VehicleTireBrake getVehicleTireBrake(void) const;

		void setLateralFriction(Ogre::Real lateralFriction);

		Ogre::Real getLateralFriction(void) const;

		void setLongitudinalFriction(Ogre::Real longitudinalFriction);

		Ogre::Real getLongitudinalFriction(void) const;

		void setSpringLength(Ogre::Real springLength);

		Ogre::Real getSpringLength(void) const;
		
		void setSmass(Ogre::Real smass);

		Ogre::Real getSmass(void) const;

		void setSpringConst(Ogre::Real springConst);

		Ogre::Real getSpringConst(void) const;

		void setSpringDamp(Ogre::Real springDamp);

		Ogre::Real getSpringDamp(void) const;

		RayCastTire* getRayCastTire(void);
	private:
		RayCastTire::TireConfiguration m_tireConfiguration;
	};

}   // end NAMESPACE OgreNewt

#endif
// _INCLUDE_OGRENEWT_BASICJOINTS


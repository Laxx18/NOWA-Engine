

#ifndef _INCLUDE_OGRENEWT_VEHICLE
#define _INCLUDE_OGRENEWT_VEHICLE

#include "OgreNewt_Prerequisites.h"
#include "OgreNewt_World.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_ConvexCast.h"

#include "ndWorld.h"
#include "ndMatrix.h"
#include "ndVector.h"
#include <OgreVector3.h>

namespace OgreNewt
{
	// -------------------------------------------------------------------------
	// Enums
	// -------------------------------------------------------------------------
	enum VehicleTireSteer
	{
		tsNoSteer = 0,
		tsSteer = 1
	};

	enum VehicleSteerSide
	{
		tsSteerSideA = -1,
		tsSteerSideB = 1
	};

	enum VehicleTireAccel
	{
		tsNoAccel = 0,
		tsAccel = 1
	};

	enum VehicleTireBrake
	{
		tsNoBrake = 0,
		tsBrake = 1
	};

	enum VehicleTireSide
	{
		tsTireSideA = 0,
		tsTireSideB = 1
	};

	// Kept from ND3 version in case you still use it somewhere.
	struct RayCastTireInfo
	{
		RayCastTireInfo(const OgreNewt::Body* body) :
			m_param(1.1f),
			m_me(body),
			m_hitBody(nullptr),
			m_contactID(0),
			m_normal(0.0f, 0.0f, 0.0f, 0.0f)
		{
		}

		Ogre::Real m_param;
		ndVector m_normal;
		const OgreNewt::Body* m_me;
		const OgreNewt::Body* m_hitBody;
		int m_contactID;
	};

	struct VehicleStuckState
	{
		Ogre::Real stuckTimer = 0.0f;     // how long we've been stuck
		Ogre::Real rescueTimer = 0.0f;    // active rescue duration
		Ogre::Real pulseTimer = 0.0f;     // forward/back pulse timing
		bool  rescueActive = false;
	};

	struct RescueState
	{
		Ogre::Real airborneTimer = 0.0f;
		Ogre::Real cooldown = 0.0f;
		bool toggleDir = false;
	};

	class RayCastTire;
	class Vehicle;

	// -------------------------------------------------------------------------
	// VehicleCallback
	// -------------------------------------------------------------------------
	class _OgreNewtExport VehicleCallback
	{
	public:
		virtual ~VehicleCallback()
		{
		}

		/**
		 * @brief Controls via device input the steer angle.
		 */
		virtual Ogre::Real onSteerAngleChanged(const OgreNewt::Vehicle* visitor, const OgreNewt::RayCastTire* tire, Ogre::Real timestep)
		{
			return 0.0f;
		}

		/**
		 * @brief Controls via device input the motor force.
		 */
		virtual Ogre::Real onMotorForceChanged(const OgreNewt::Vehicle* visitor, const OgreNewt::RayCastTire* tire, Ogre::Real timestep)
		{
			return 0.0f;
		}

		/**
		 * @brief Controls via device input the hand brake force.
		 * @note  A good value is 5.5.
		 */
		virtual Ogre::Real onHandBrakeChanged(const OgreNewt::Vehicle* visitor, const OgreNewt::RayCastTire* tire, Ogre::Real timestep)
		{
			return 0.0f;
		}

		/**
		 * @brief Controls via device input the brake force.
		 * @note  A good value is 7.5.
		 */
		virtual Ogre::Real onBrakeChanged(const OgreNewt::Vehicle* visitor, const OgreNewt::RayCastTire* tire, Ogre::Real timestep)
		{
			return 0.0f;
		}

		/**
		 * @brief Is called if the tire hits another game object below.
		 */
		virtual void onTireContact(const OgreNewt::RayCastTire* tire, const Ogre::String& tireName, OgreNewt::Body* hitBody,
			const Ogre::Vector3& contactPosition, const Ogre::Vector3& contactNormal, Ogre::Real penetration)
		{
		}
	};

	// -------------------------------------------------------------------------
	// RayCastTire – per-wheel wrapper, derived from ndJointBilateralConstraint
	// -------------------------------------------------------------------------
	class _OgreNewtExport RayCastTire : public ndJointBilateralConstraint
	{
	public:
		friend class Vehicle;

	public:
		struct TireConfiguration
		{
			VehicleTireSide      tireSide;
			VehicleTireSteer     tireSteer;
			VehicleTireAccel     tireAccel;
			VehicleTireBrake     brakeMode;
			VehicleSteerSide     steerSide;

			Ogre::Real           springConst;
			Ogre::Real           springLength;
			Ogre::Real           springDamp;
			Ogre::Real           longitudinalFriction;
			Ogre::Real           lateralFriction;
			Ogre::Real           smass;

			TireConfiguration() :
				tireSide(tsTireSideA),
				tireSteer(tsNoSteer),
				tireAccel(tsNoAccel),
				brakeMode(tsNoBrake),
				steerSide(tsSteerSideA),
				springConst(100.0f),
				springLength(0.5f),
				springDamp(10.0f),
				longitudinalFriction(0.9f),
				lateralFriction(0.9f),
				smass(0.2f)
			{
			}
		};

	public:
		RayCastTire(ndWorld* world, const ndMatrix& pinAndPivotFrame, const ndVector& pin, Body* child, Body* parentChassisBody, Vehicle* parent, const TireConfiguration& tireConfiguration, ndReal radius);

		virtual ~RayCastTire();

		virtual void UpdateParameters() override;

		void processConvexContacts(OgreNewt::BasicConvexcast::ConvexcastContactInfo& info, Ogre::Real timestep);

		ndMatrix calculateSuspensionMatrixLocal(ndReal distance);
		
		ndMatrix calculateTireMatrixAbsolute(ndReal sSide);

		void applyBrakes(ndReal bkforce, ndConstraintDescritor& desc);

		void longitudinalAndLateralFriction(const ndVector& contactPos, const ndVector& lateralPinIn, const ndVector& longitudinalPinIn,
			ndReal lateralMu, ndReal longitudinalMu, ndConstraintDescritor& desc);

		ndMatrix getLocalMatrix0();

		void setTireConfiguration(const TireConfiguration& cfg);
		const TireConfiguration& getTireConfiguration(void) const;
	protected:
		void JacobianDerivative(ndConstraintDescritor& desc) override;

	private:
		void processPreUpdate(Ogre::Real timestep, int threadIndex);
		Ogre::Real applySuspenssionLimit();
	public:
		Body* m_thisBody;
		ndBodyKinematic* m_hitBody;
		ndBodyKinematic* m_chassisBody;
		ndReal m_hitParam;
		ndReal m_penetration;
		long m_contactID;
		ndVector m_hitContact;
		ndVector m_hitNormal;
		ndShapeInstance* m_collision;

		ndVector m_pin;

		ndMatrix m_tireRenderMatrix;
		ndMatrix m_globalTireMatrix;
		ndMatrix m_localTireMatrix;
		ndMatrix m_suspensionMatrix;
		ndMatrix m_frameLocalMatrix;
		ndVector m_hardPoint;
		ndVector m_localAxis;
		ndMatrix m_angleMatrix;

		ndReal m_arealpos;
		ndReal m_posit_y;
		ndReal m_radius;
		ndReal m_width;
		ndReal m_tireLoad;
		ndReal m_suspenssionHardLimit;
		ndReal m_chassisSpeed;
		ndReal m_tireSpeed;
		ndReal m_suspenssionFactor;
		ndReal m_suspenssionStep;
		ndReal m_spinAngle;
		ndVector m_realvelocity;
		ndVector m_TireAxelPosit;
		ndVector m_LocalAxelPosit;
		ndVector m_TireAxelVeloc;
		ndReal m_jointBrakeForce;
		bool m_isOnContactEx;
		int m_tireID;

		ndReal m_steerAngle;
		ndReal m_brakeForce;
		ndReal m_handForce;
		ndReal m_motorForce;
	private:
		//
		Vehicle* m_vehicle;
		ndVector m_lateralPin;
		ndVector m_longitudinalPin;
		bool m_isOnContact;
		TireConfiguration m_tireConfiguration;
		
	};

	// -------------------------------------------------------------------------
	// Vehicle – ND4 model wrapper
	// -------------------------------------------------------------------------
	class _OgreNewtExport Vehicle : public OgreNewt::Body
	{
	public:
		friend class RayCastTire;
		friend class VehicleNotify;
	public:
		Vehicle(World* world, Ogre::SceneManager* sceneManager, const Ogre::Vector3& defaultDirection,
			const OgreNewt::CollisionPtr& col, Ogre::Real vhmass, const Ogre::Vector3& collisionPosition,
			const Ogre::Vector3& massOrigin, const Ogre::Vector3& gravity, VehicleCallback* vehicleCallback);

		virtual ~Vehicle();

		void addTire(RayCastTire* tire);
		bool removeTire(RayCastTire* tire);
		void removeAllTires(void);

		Ogre::Real vectorLength(const ndVector& aVec);

		// Force helper (world space)
		ndVector rel2AbsPoint(ndBodyKinematic* vBody, const ndVector& vPointRel);

		void addForceAtPos(ndBodyKinematic* vBody, const ndVector& vForce, const ndVector& vPoint, Ogre::Real timestep);

		void addForceAtRelPos(ndBodyKinematic* vBody, const ndVector& vForce, const ndVector& vPoint, Ogre::Real timestep);

		void setGravity(const Ogre::Vector3& gravity);
		Ogre::Vector3 getGravity() const;

		void setCanDrive(bool canDrive);

		void setChassisMatrix(const ndMatrix& matrix);

		VehicleCallback* getVehicleCallback(void) const;

		Ogre::Vector3 getVehicleForce() const;

		void setUseTilting(bool tilt);

		bool getUseTilting() const;
	private:
		void update(Ogre::Real timestep, int threadIndex);
		void updateUnstuck(Ogre::Real timestep);
		void updateAirborneRescue(Ogre::Real timestep);
		void initMassData();
		void applyForceAndTorque(ndBodyKinematic* vBody, const ndVector& vForce, const ndVector& vPoint, Ogre::Real timestep);
		void updateDriverInput(RayCastTire* tire, Ogre::Real timestep);
	public:
		int m_tireCount;
		bool m_debugtire;
		bool m_initMassDataDone;
		ndVector m_defaultDirection;
		ndReal m_mass;
		ndVector m_massOrigin;
		ndVector m_collisionPosition;
		ndVector m_combackup;
		Ogre::Vector3 m_vehicleForce;
		ndList<RayCastTire*> m_tires;
		int m_noHitCounter;
		bool m_canDrive;
	private:
		ndMatrix m_chassisMatrix;
		VehicleCallback* m_vehicleCallback;
		VehicleStuckState m_stuck;
		RescueState m_rescue;
		bool m_useTilting;
	};

} // namespace OgreNewt

#endif

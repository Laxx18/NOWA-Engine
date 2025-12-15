

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

		float m_param;
		ndVector m_normal;
		const OgreNewt::Body* m_me;
		const OgreNewt::Body* m_hitBody;
		int m_contactID;
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

		void ProcessConvexContacts(OgreNewt::BasicConvexcast::ConvexcastContactInfo& info);

		ndMatrix CalculateSuspensionMatrixLocal(ndReal distance);
		
		ndMatrix CalculateTireMatrixAbsolute(ndReal sSide);

		void ApplyBrakes(ndReal bkforce, ndConstraintDescritor& desc);

		void LongitudinalAndLateralFriction(ndVector tireposit, ndVector lateralpin, ndReal turnfriction, ndReal sidingfriction, ndConstraintDescritor& desc);
		
		ndMatrix GetLocalMatrix0();

		void SetTireConfiguration(const TireConfiguration& cfg);
		const TireConfiguration& GetTireConfiguration(void) const;
	protected:
		void JacobianDerivative(ndConstraintDescritor& desc) override;

	private:
		void ProcessPreUpdate();
		Ogre::Real ApplySuspenssionLimit();
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
	// Vehicle – ND4 model wrapper, API similar to OgreNewt3 Vehicle
	// -------------------------------------------------------------------------
	class _OgreNewtExport Vehicle : public OgreNewt::Body
	{
	public:
		friend class RayCastTire;

	public:
		Vehicle(World* world,
			Ogre::SceneManager* sceneManager,
			const Ogre::Vector3& defaultDirection,
			const OgreNewt::CollisionPtr& col,
			Ogre::Real vhmass,
			const Ogre::Vector3& collisionPosition,
			const Ogre::Vector3& massOrigin,
			const Ogre::Vector3& gravity,
			VehicleCallback* vehicleCallback);

		virtual ~Vehicle();

		void AddTire(RayCastTire* tire);
		bool RemoveTire(RayCastTire* tire);
		void RemoveAllTires(void);

		Ogre::Real VectorLength(const ndVector& aVec);

		void Update(Ogre::Real timestep);

		// Force helper (world space), ND3 math
		ndVector Rel2AbsPoint(ndBodyKinematic* vBody, const ndVector& vPointRel);

		void AddForceAtPos(ndBodyKinematic* vBody, const ndVector& vForce, const ndVector& vPoint);

		void AddForceAtRelPos(ndBodyKinematic* vBody, const ndVector& vForce, const ndVector& vPoint);

		void setGravity(const Ogre::Vector3& gravity);
		Ogre::Vector3 getGravity() const;

		void setCanDrive(bool canDrive);

		void SetChassisMatrix(const ndMatrix& matrix);

		VehicleCallback* GetVehicleCallback(void) const;

		Ogre::Vector3 getVehicleForce() const;

	private:
		void InitMassData();
		void ApplyForceAndTorque(ndBodyKinematic* vBody, const ndVector& vForce, const ndVector& vPoint);
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
	};

} // namespace OgreNewt

#endif

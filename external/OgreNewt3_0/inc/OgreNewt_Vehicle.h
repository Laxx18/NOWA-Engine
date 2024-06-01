/* Copyright (c) <2003-2019> <Newton Game Dynamics>
*
* This software is provided 'as-is', without any express or implied
* warranty. In no event will the authors be held liable for any damages
* arising from the use of this software.
*
* Permission is granted to anyone to use this software for any purpose,
* including commercial applications, and to alter it and redistribute it
* freely
*/

// My Vehicle RayCast code contribution for Newton Dynamics SDK.
// WIP Tutorial: Simple Vehicle RayCast class by Dave Gravel - 2020.
// My Youtube channel https://www.youtube.com/user/EvadLevarg
//
// Know bug or unfinished parts:
// Need fix about (chassis, suspension) wobble effect when the accel force is apply.
// The problem don't happen any times, Sometimes the wobble effect is not present at all.
// This wobble effect is not present in my engine sdk. 
// I try to compare my engine code for find and fix the problem, But I don't have find for now.
//
// The tire can have interaction with dynamics object but it is experimental and not fully implemented.
//
// In debug mode it can crash or get some matrix error or assert.
// Major part of the time it happen when the vehicle tire collide with a other vehicle.
//
// In debug I get warning!! bilateral joint duplication between bodied.
// If I'm not wrong I think it happen because the chassis body is connected with multiple tire joints.
//
// The vehicle engine and force parts is not fully implemented for get smoother behaviors.
// It need a better implementation for apply the force gradually and make smoother accel & steer effect.
//
// With the truck the tire radius don't look to fit right with the visual mesh and the collision...
//
// The demo look to cause problem when it is reset by F1 key and when you use multithreads, With one thread it don't look to cause problem.
//
// The tire debug visual is not working in the last newton sdk commit, I'm not sure why it working good in my older newton sdk.
// It look like the OnDebug function is not call anymore.
//
// Some variables need to become in private and the vehicle class need some Get and Set function to make it more clear.
//
// I surely need help from Julio or any newton user for fix and clean and make the code better.

#ifndef _INCLUDE_OGRENEWT_VEHICLE
#define _INCLUDE_OGRENEWT_VEHICLE

#include "OgreNewt_Prerequisites.h"
#include "dModelRootNode.h"
#include "dModelManager.h"
#include "OgreNewt_Body.h"

// OgreNewt namespace.  all functions and classes use this namespace.
namespace OgreNewt
{

	enum VehicleRaycastType
	{
		rctWorld = 0,
		rctConvex = 1
	};

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

	static dFloat VehicleRayCastFilter(const NewtonBody* const body, const NewtonCollision* const collisionHit, const dFloat* const contact, const dFloat* const normal, dLong collisionID, void* const userData, dFloat intersetParam);
	static unsigned VehicleRayPrefilterCallback(const NewtonBody* const body, const NewtonCollision* const collision, void* const userData);
	//
	static void TireJointDeleteCallback(const dCustomJoint* const me);

	// RayCastTireInfo

	struct RayCastTireInfo
	{
		RayCastTireInfo(const OgreNewt::Body* body)
		{
			m_param = 1.1f;
			m_me = body;
			m_hitBody = NULL;
			m_contactID = 0;
			m_normal = dVector(0.0f, 0.0f, 0.0f, 0.0f);
		}
		dFloat m_param;
		dVector m_normal;
		const OgreNewt::Body* m_me;
		const OgreNewt::Body* m_hitBody;
		int m_contactID;
	};

	static void RenderDebugRayTire(void* userData, int vertexCount, const dFloat* const faceVertec, int id)
	{
		// TODO: Ogre Line rendering
#if 0
		dCustomJoint::dDebugDisplay* const debugContext = (dCustomJoint::dDebugDisplay*)userData;

		int index = vertexCount - 1;
		dVector p0(faceVertec[index * 3 + 0], faceVertec[index * 3 + 1], faceVertec[index * 3 + 2]);
		for (int i = 0; i < vertexCount; i++)
		{
			dVector p1(faceVertec[i * 3 + 0], faceVertec[i * 3 + 1], faceVertec[i * 3 + 2]);
			debugContext->DrawLine(p0, p1);
			p0 = p1;
		}
#endif
	}

	class _OgreNewtExport RayCastTire : public dCustomJoint
	{
	public:
		friend class Vehicle;
		friend class RayCastVehicleManager;

		struct TireConfiguration
		{
			VehicleTireSide tireSide = tsTireSideA;
			VehicleTireSteer tireSteer = tsNoSteer;
			VehicleSteerSide steerSide = tsSteerSideB;
			VehicleTireAccel tireAccel = tsNoAccel;
			VehicleTireBrake brakeMode = tsNoBrake;
			dFloat lateralFriction = 1.0f;
			dFloat longitudinalFriction = 1.0f;
			dFloat springLength = 0.2f;
			dFloat smass = 18.0f;
			dFloat springConst = 200.0f;
			dFloat springDamp = 10.0f;
		};
	public:

		RayCastTire(NewtonWorld* world, const dMatrix& pinAndPivotFrame, const dVector& pin, Body* child, Body* parentChassisBody, Vehicle* parent, const TireConfiguration& tireConfiguration, dFloat radius);

		virtual ~RayCastTire();

		dFloat ApplySuspenssionLimit();
		//
		void ProcessWorldContacts(dFloat const rhitparam, NewtonBody* const rhitBody, dFloat const rpenetration, dLong const rcontactid, dVector const rhitContact, dVector const rhitNormal);

		void ProcessConvexContacts(NewtonWorldConvexCastReturnInfo const info);
		//
		dMatrix CalculateSuspensionMatrixLocal(dFloat distance);
		//
		dMatrix CalculateTireMatrixAbsolute(dFloat sSide);

		//
		void ApplyBrakes(dFloat bkforce);
		//
		dMatrix GetLocalMatrix0();
		//
		void LongitudinalAndLateralFriction(dVector tireposit, dVector lateralpin, dFloat turnfriction, dFloat sidingfriction, dFloat timestep);
		//
		void ProcessPreUpdate(Vehicle* vehicle, dFloat timestep, int threadID);

		void SetTireConfiguration(const TireConfiguration& tireConfiguration);

		TireConfiguration GetTireConfiguration(void) const;

		Body* m_thisBody;
		NewtonBody* m_chassisBody;
		NewtonBody* m_hitBody;
		dFloat m_hitParam;
		dFloat m_penetration;
		dLong m_contactID;
		dVector m_hitContact;
		dVector m_hitNormal;
		NewtonCollision* m_collision;

		dVector m_pin;
	
		dMatrix m_tireRenderMatrix;
		dMatrix m_globalTireMatrix;
		dMatrix m_localTireMatrix;
		dMatrix m_suspensionMatrix;
		dMatrix m_frameLocalMatrix;
		dVector m_hardPoint;
		dVector m_localAxis;
		dMatrix m_angleMatrix;

		dFloat m_arealpos;
		dFloat m_posit_y;
		dFloat m_radius;
		dFloat m_width;
		dFloat m_tireLoad;
		dFloat m_suspenssionHardLimit;
		dFloat m_chassisSpeed;
		dFloat m_tireSpeed;
		dFloat m_suspenssionFactor;
		dFloat m_suspenssionStep;
		dFloat m_spinAngle;
		dVector m_realvelocity;
		dVector m_TireAxelPosit;
		dVector m_LocalAxelPosit;
		dVector m_TireAxelVeloc;
		bool m_isOnContactEx;
		int m_tireID;

		dFloat m_steerAngle;
		dFloat m_brakeForce;
		dFloat m_handForce;
		dFloat m_motorForce;
	private:
		//
		Vehicle* m_vehicle;
		dVector m_lateralPin;
		dVector m_longitudinalPin;
		bool m_isOnContact;
		TireConfiguration m_tireConfiguration;
	protected:
		virtual void SubmitConstraints(dFloat timestep, int threadIndex);
		//
		virtual void Deserialize(NewtonDeserializeCallback callback, void* const userData);

		virtual void Serialize(NewtonSerializeCallback callback, void* const userData) const;
	};

	////////////////////////////////////////////////////////////////////////////////////////////

	class RayCastVehicleManager;
	class VehicleCallback;

	class _OgreNewtExport Vehicle : public Body, public dModelRootNode
	{
	public:
		friend class RayCastVehicleManager;
	public:
		Vehicle(World* world, Ogre::SceneManager* sceneManager, const OgreNewt::CollisionPtr& col, Ogre::Real vhmass, const Ogre::Vector3& collisionPosition, const Ogre::Vector3& massOrigin, VehicleCallback* vehicleCallback);

		virtual ~Vehicle();
		//
		void SetRayCastMode(const VehicleRaycastType raytype);
		//
		// void CalculateTireDimensions(const Ogre::String& tireName, Ogre::Real& width, Ogre::Real& radius);
		//
		// void AddTire(const Ogre::String& tireName, VehicleTireSide tireside, VehicleTireSteer tiresteer, VehicleSteerSide steerside, VehicleTireAccel tireaccel, VehicleTireBrake tirebrake,
		// 			 Ogre::Real lateralFriction = 1.0f, Ogre::Real longitudinalFriction = 1.0f, Ogre::Real slimitlength = 0.25f, Ogre::Real smass = 14.0f, Ogre::Real spingCont = 200.0f, Ogre::Real sprintDamp = 10.0f);
		
		void AddTire(RayCastTire* tire);
		
		bool RemoveTire(RayCastTire* tire);

		void RemoveAllTires(void);
		//
		Ogre::Real VectorLength(dVector aVec);
		//
		dVector Rel2AbsPoint(NewtonBody* Body, dVector Pointrel);
		//
		void AddForceAtPos(NewtonBody* Body, dVector Force, dVector Point);
		//
		void AddForceAtRelPos(NewtonBody* Body, dVector Force, dVector Point);
		//
		void ApplyForceAndTorque(NewtonBody* vBody, dVector vForce, dVector vPoint);
		//
		void SetChassisMatrix(dMatrix vhmatrix);

		VehicleCallback* GetVehicleCallback(void) const;
	private:
		void InitMassData(void);
	public:
		int m_tireCount;
		bool m_debugtire;
		bool m_initMassDataDone;
		dFloat m_mass;
		dVector m_massOrigin;
		dVector m_collisionPosition;
		dVector m_combackup;
		VehicleRaycastType m_raytype;
		dList<RayCastTire*> m_tires;
		int m_noHitCounter;
	private:
		dMatrix m_chassisMatrix;
		RayCastVehicleManager* m_raycastVehicleManager;
		VehicleCallback* m_vehicleCallback;
	};

	////////////////////////////////////////////////////////////////////////////////////////////

	class _OgreNewtExport VehicleCallback
	{
	public:
		VehicleCallback()
		{
		}

		~VehicleCallback()
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
		 * @brief Controsl via device input the brake force.
		 * @note  A good value is 7.5.
		 */
		virtual Ogre::Real onBrakeChanged(const OgreNewt::Vehicle* visitor, const OgreNewt::RayCastTire* tire, Ogre::Real timestep)
		{
			return 0.0f;
		}
	};

	////////////////////////////////////////////////////////////////////////////////////////////

	class RayCastVehicleManager : public dModelManager
	{
	public:
		RayCastVehicleManager(Vehicle* vehicle);

		virtual ~RayCastVehicleManager();

		virtual void OnPreUpdate(dModelRootNode* const model, dFloat timestep, int threadID) const;
		//
		virtual void OnPostUpdate(dModelRootNode* const model, dFloat timestep, int threadID) const;
		//
		virtual void OnUpdateTransform(const dModelNode* const bone, const dMatrix& localMatrix) const;
		//
		virtual void OnDebug(dModelRootNode* const model, dCustomJoint::dDebugDisplay* const debugContext);

		static void UpdateDriverInput(Vehicle* const vehicle, RayCastTire* vhtire, dFloat timestep);
	};

};

#endif


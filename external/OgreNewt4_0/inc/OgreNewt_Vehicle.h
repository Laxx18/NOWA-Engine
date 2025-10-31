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
#include "OgreNewt_World.h"
#include "OgreNewt_Body.h"

#include "ndWorld.h"
#include "ndMatrix.h"
#include "ndVector.h"
#include "ndMultiBodyVehicle.h"
#include "ndVehicleCommon.h"   // uses ndVehicleDectriptor::ndTireDefinition
#include "ndMultiBodyVehicleMotor.h"

#include <OgreVector3.h>
#include <OgreQuaternion.h>
#include <OgreString.h>
#include <vector>
#include <memory>

namespace OgreNewt
{
    // --- enums kept for compatibility ---
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

    struct RayCastTireInfo
    {
        RayCastTireInfo(const OgreNewt::Body* body)
            : m_param(1.1f),
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

    // --- callback interface (unchanged) ---
    class _OgreNewtExport VehicleCallback
    {
    public:
        virtual ~VehicleCallback() = default;

        virtual Ogre::Real onSteerAngleChanged(const OgreNewt::Vehicle* visitor, const OgreNewt::RayCastTire* tire, Ogre::Real timestep)
        {
            return 0.0f;
        }

        virtual Ogre::Real onMotorForceChanged(const OgreNewt::Vehicle* visitor, const OgreNewt::RayCastTire* tire, Ogre::Real timestep)
        {
            return 0.0f;
        }

        virtual Ogre::Real onHandBrakeChanged(const OgreNewt::Vehicle* visitor, const OgreNewt::RayCastTire* tire, Ogre::Real timestep)
        {
            return 0.0f;
        }

        virtual Ogre::Real onBrakeChanged(const OgreNewt::Vehicle* visitor, const OgreNewt::RayCastTire* tire, Ogre::Real timestep)
        {
            return 0.0f;
        }

        virtual void onTireContact(const OgreNewt::RayCastTire* tire, const Ogre::String& tireName, OgreNewt::Body* hitBody, 
            const Ogre::Vector3& contactPosition, const Ogre::Vector3& contactNormal, Ogre::Real penetration)
        {
        }
    };

    // --- per-wheel wrapper ---
    class _OgreNewtExport RayCastTire
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

            Ogre::Real springConst;
            Ogre::Real springLength;
            Ogre::Real springDamp;
            Ogre::Real longitudinalFriction;
            Ogre::Real lateralFriction;
            Ogre::Real smass;

            TireConfiguration()
                : tireSide(tsTireSideA)
                , tireSteer(tsNoSteer)
                , tireAccel(tsNoAccel)
                , brakeMode(tsNoBrake)
                , steerSide(tsSteerSideA)
                , springConst(100.0f)
                , springLength(0.5f)
                , springDamp(10.0f)
                , longitudinalFriction(0.9f)
                , lateralFriction(0.9f)
                , smass(0.2f)
            {
            }
        };

    public:
        RayCastTire(OgreNewt::Body* child, OgreNewt::Body* parentBody, const Ogre::Vector3& pos, const Ogre::Vector3& pin,
            class Vehicle* parent, const TireConfiguration& tireConfiguration, Ogre::Real radius);

        ~RayCastTire();

        // tuning (maps to ndVehicleDectriptor::ndTireDefinition fields we keep locally)
        void setLateralFriction(Ogre::Real v);

        Ogre::Real getLateralFriction() const;

        void setLongitudinalFriction(Ogre::Real v);

        Ogre::Real getLongitudinalFriction() const;

        void setSpringLength(Ogre::Real v);

        Ogre::Real getSpringLength() const;

        void setSpringConst(Ogre::Real v);

        Ogre::Real getSpringConst() const;

        void setSpringDamp(Ogre::Real v);

        Ogre::Real getSpringDamp() const;

        // controls
        void setSteerAngleDeg(Ogre::Real angleDeg);

        void setBrakeForce(Ogre::Real normalized);

        void setHandBrake(Ogre::Real normalized);

        void setMotorForce(Ogre::Real normalized);

        // state/telemetry
        bool isGrounded() const;

        Ogre::Real getPenetration() const;

        Ogre::String getTireName() const;

        // accessors
        ndMultiBodyVehicleTireJoint* getTireJoint() const;

        OgreNewt::Body* getBody() const;

        // expose tire definition for inspection
        ndVehicleDectriptor::ndTireDefinition getDefinition() const;

    private:
        void updateGroundProbe();
    private:

        class Vehicle* m_vehicle{ nullptr };
        Ogre::Real m_radius{ 0.0f };

        OgreNewt::Body* m_thisBody{ nullptr };
        ndVehicleDectriptor::ndTireDefinition m_def{}; // local copy used at creation time
        ndMultiBodyVehicleTireJoint* m_tireJoint{ nullptr };

        bool m_grounded{ false };
        Ogre::Real m_penetration{ 0.0f };
        Ogre::Real m_steerAngleDeg{ 0.0f };
        Ogre::Real m_brakeNorm{ 0.0f };
        Ogre::Real m_handNorm{ 0.0f };
        Ogre::Real m_motorNorm{ 0.0f };

        TireConfiguration m_tireConfiguration;
    };

    // --- vehicle container ---
    class _OgreNewtExport Vehicle : public OgreNewt::Body
    {
    public:
        friend class RayCastTire;
    public:

        Vehicle(World* world, Ogre::SceneManager* sceneManager, const Ogre::Vector3& defaultDirection, const OgreNewt::CollisionPtr& col, Ogre::Real vhmass,
            const Ogre::Vector3& collisionPosition, const Ogre::Vector3& massOrigin, VehicleCallback* vehicleCallback);

        ~Vehicle();

        void SetRayCastMode(VehicleRaycastType raytype);
        void AddTire(RayCastTire* tire);
        bool RemoveTire(RayCastTire* tire);
        void RemoveAllTires(void);
        void PreUpdate(Ogre::Real timestep);

        VehicleCallback* GetVehicleCallback() const { return m_vehicleCallback; }
        OgreNewt::World* getWorld() const { return m_world; }
        OgreNewt::Body* getChassis() const { return m_chassis; }

        // Force helper (ND4 via wrapper)
        void AddForceAtPos(OgreNewt::Body* body, const Ogre::Vector3& forceWS, const Ogre::Vector3& pointWS);

        // Optional: let engine feed gravity scalar used for stiffness presets
        void setGravityScalar(Ogre::Real g) { m_gravity = g; }
        Ogre::Real getGravityScalar() const { return m_gravity; }


        // Option A: you already have a motor body in your engine (preferred)
        // Call this once, before the first PreUpdate / ensureVehicleModel().
        void setMotorBody(OgreNewt::Body* motorBody);

        // Motor config (you can also expose these via your VehicleCallback if desired)
        void setMotorMaxRpm(Ogre::Real rpm);
        void setMotorOmegaAccel(Ogre::Real rpmPerSec);
        void setMotorFrictionLoss(Ogre::Real newtonMeters);
        void setMotorTorqueScale(Ogre::Real nmPerUnit); // scales normalized throttle to torque
        void setCanDrive(bool canDrive);

        Ogre::Vector3 getVehicleForce() const;

        inline ndMultiBodyVehicle* getVehicleModel() const
        {
            return m_vehicleModel;
        }

        // store a convenience pointer to the motor (optional but handy)
        inline void setMotor(ndMultiBodyVehicleMotor* motor)
        {
            m_motor = motor;
        }

        inline ndMultiBodyVehicleMotor* getMotor() const
        {
            return m_motor;
        }
    private:
        void InitMassData();

        void ensureVehicleModel();

        void ApplyForceAndTorque(OgreNewt::Body* vBody, const Ogre::Vector3& vForce, const Ogre::Vector3& vPoint);

        void updateDriverInput(RayCastTire* tire, Ogre::Real timestep);
    private:

        World* m_world{ nullptr };
        VehicleCallback* m_vehicleCallback{ nullptr };

        // chassis is a Body wrapper (ND4)
        OgreNewt::Body* m_chassis{ nullptr };

        ndMultiBodyVehicle* m_vehicleModel{ nullptr };
        bool m_vehicleAddedToWorld{ false };

        std::vector<RayCastTire*> m_tires;
        int m_tireCount{ 0 };
        Ogre::Real m_mass{ 0 };
        Ogre::Vector3 m_comLocal{ 0,0,0 };
        ndMatrix m_chassisMatrix;
        bool m_canDrive{ false };
        bool m_initMassDataDone{ false };
        Ogre::Real m_gravity{ 9.81f };
        Ogre::Vector3 m_vehicleForce{ Ogre::Vector3::ZERO };

        // engine joint and optional body (if provided by engine)
        OgreNewt::Body* m_motorWrapper{ nullptr };
        ndMultiBodyVehicleMotor* m_motor{ nullptr };

        // simple tuning
        ndFloat32 m_motorMaxRpm{ 6000.0f };
        ndFloat32 m_motorOmegaAccel{ 1500.0f };     // rpm/sec
        ndFloat32 m_motorFrictionLoss{ 15.0f };     // N·m
        ndFloat32 m_motorTorqueScale{ 1000.0f };    // N·m per 1.0 normalized throttle
    };
}


#endif


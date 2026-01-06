#ifndef _INCLUDE_OGRENEWT_COMPLEX_VEHICLE
#define _INCLUDE_OGRENEWT_COMPLEX_VEHICLE

#include "OgreNewt_BasicJoints.h"
#include "ndMultiBodyVehicle.h"
#include "ndMultiBodyVehicleMotor.h"
#include "ndMultiBodyVehicleTireJoint.h"
#include "ndMultiBodyVehicleDifferential.h"

namespace OgreNewt
{
    class ComplexVehicle;

    //! Drive layout for ComplexVehicle
    enum ComplexDriveLayout
    {
        cdlFrontWheelDrive = 0,
        cdlRearWheelDrive,
        cdlAllWheelDrive
    };

    //! ComplexVehicleTire – ND4 tire representation attached to an OgreNewt body
    class _OgreNewtExport ComplexVehicleTire : public Joint
    {
    public:
        enum Axle
        {
            axleFront = 0,
            axleRear
        };

        enum Side
        {
            sideLeft = 0,
            sideRight
        };

        //! Configuration similar to old RayCastTire::TireConfiguration but independent of RayCastTire
        struct TireConfiguration
        {
            VehicleTireSide tireSide;
            VehicleTireSteer tireSteer;
            VehicleSteerSide steerSide;
            VehicleTireAccel tireAccel;
            VehicleTireBrake brakeMode;

            Ogre::Real lateralFriction;
            Ogre::Real longitudinalFriction;
            Ogre::Real springLength;
            Ogre::Real smass;
            Ogre::Real springConst;
            Ogre::Real springDamp;

            TireConfiguration() :
                tireSide(tsTireSideA),
                tireSteer(tsNoSteer),
                steerSide(tsSteerSideA),
                tireAccel(tsNoAccel),
                brakeMode(tsNoBrake),
                lateralFriction(2.0f),
                longitudinalFriction(4.0f),
                springLength(0.2f),
                smass(50.0f),
                springConst(200.0f),
                springDamp(12.0f)
            {
            }
        };

    public:
        ComplexVehicleTire(OgreNewt::Body* child, OgreNewt::Body* parentBody, const Ogre::Vector3& pos,
            const Ogre::Vector3& pin, ComplexVehicle* parentVehicle, Axle axle, Side side, Ogre::Real radius);

        virtual ~ComplexVehicleTire(void);

        ComplexVehicle* getComplexVehicle(void) const
        {
            return m_complexVehicle;
        }

        Axle getAxle(void) const
        {
            return m_axle;
        }

        Side getSide(void) const
        {
            return m_side;
        }

        OgreNewt::Body* getTireBody(void) const
        {
            return m_childBody;
        }

        Ogre::Real getRadius(void) const
        {
            return m_radius;
        }

        // --- Configuration (compatible names with VehicleTire API) ---

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

        const TireConfiguration& getTireConfiguration(void) const
        {
            return m_tireConfiguration;
        }

        // --- Runtime control values used per frame by ComplexVehicle::updateDriverInput ---

        void setSteerAngleDeg(Ogre::Real angleDeg)
        {
            m_steerAngleDeg = angleDeg;
        }

        Ogre::Real getSteerAngleDeg(void) const
        {
            return m_steerAngleDeg;
        }

        void setMotorForce(Ogre::Real normalized)
        {
            m_motorNorm = normalized;
        }

        Ogre::Real getMotorForce(void) const
        {
            return m_motorNorm;
        }

        void setBrakeForce(Ogre::Real normalized)
        {
            m_brakeNorm = normalized;
        }

        Ogre::Real getBrakeForce(void) const
        {
            return m_brakeNorm;
        }

        void setHandBrake(Ogre::Real normalized)
        {
            m_handBrakeNorm = normalized;
        }

        Ogre::Real getHandBrake(void) const
        {
            return m_handBrakeNorm;
        }

        // ND4 tire joint accessor
        ndMultiBodyVehicleTireJoint* getTireJoint(void) const
        {
            return m_tireJoint;
        }

        void setTireJoint(ndMultiBodyVehicleTireJoint* joint)
        {
            m_tireJoint = joint;
        }

        // Helper: build ND4 joint info for this tire (uses local frame and configuration)
        ndMultiBodyVehicleTireJointInfo buildJointInfo(void) const;
    private:
        ComplexVehicle* m_complexVehicle;
        Axle m_axle;
        Side m_side;

        OgreNewt::Body* m_childBody;
        OgreNewt::Body* m_parentBody;
        Ogre::Vector3 m_localPos;
        Ogre::Vector3 m_pin;
        Ogre::Real m_radius;

        TireConfiguration m_tireConfiguration;

        // Runtime control state
        Ogre::Real m_steerAngleDeg;
        Ogre::Real m_motorNorm;
        Ogre::Real m_brakeNorm;
        Ogre::Real m_handBrakeNorm;

        // ND4 handle (result of m_vehicleModel->AddTire)
        ndMultiBodyVehicleTireJoint* m_tireJoint;
    };

    // -------------------------------------------------------------------------
    // ComplexVehicleCallback
    // -------------------------------------------------------------------------
    class _OgreNewtExport ComplexVehicleCallback
    {
    public:
        virtual ~ComplexVehicleCallback()
        {
        }

        virtual Ogre::Real onSteerAngleChanged(const OgreNewt::ComplexVehicle* visitor, const ComplexVehicleTire* tire, Ogre::Real timestep)
        {
            return 0.0f;
        }

        virtual Ogre::Real onMotorForceChanged(const OgreNewt::ComplexVehicle* visitor, const ComplexVehicleTire* tire, Ogre::Real timestep)
        {
            return 0.0f;
        }

        virtual Ogre::Real onHandBrakeChanged(const OgreNewt::ComplexVehicle* visitor, const ComplexVehicleTire* tire, Ogre::Real timestep)
        {
            return 0.0f;
        }

        virtual Ogre::Real onBrakeChanged(const OgreNewt::ComplexVehicle* visitor, const ComplexVehicleTire* tire, Ogre::Real timestep)
        {
            return 0.0f;
        }

        virtual void onTireContact(const ComplexVehicleTire* tire, const Ogre::String& tireName, OgreNewt::Body* hitBody,
            const Ogre::Vector3& contactPosition, const Ogre::Vector3& contactNormal, Ogre::Real penetration)
        {
        }
    };

    //! ComplexVehicle – ND4 multi-body vehicle with differentials
    class _OgreNewtExport ComplexVehicle : public OgreNewt::Body
    {
    public:
        friend class ComplexVehicleNotify;
    public:
        ComplexVehicle(World* world, Ogre::SceneManager* sceneManager, const Ogre::Vector3& defaultDirection,
            const OgreNewt::CollisionPtr& col, Ogre::Real vhmass, const Ogre::Vector3& collisionPosition,
            const Ogre::Vector3& massOrigin, ComplexVehicleCallback* vehicleCallback);

        virtual ~ComplexVehicle(void);

        //! Register a complex tire (front / rear, left / right).
        void addTire(ComplexVehicleTire* tire);
        bool removeTire(ComplexVehicleTire* tire);
        void removeAllTires(void);

        ComplexVehicleCallback* getVehicleCallback() const
        {
            return m_vehicleCallback;
        }

        OgreNewt::World* getWorld() const
        {
            return m_world;
        }

        OgreNewt::Body* getChassis() const
        {
            return const_cast<ComplexVehicle*>(this);
        }

        // Force helper (world space)
        void addForceAtPos(OgreNewt::Body* body, const Ogre::Vector3& forceWS, const Ogre::Vector3& pointWS);

        // Optional gravity scalar used to pre-tune spring parameters
        void setGravityScalar(Ogre::Real g)
        {
            m_gravity = g;
        }

        Ogre::Real getGravityScalar() const
        {
            return m_gravity;
        }

        // Engine / motor helpers
        void createMotor(Ogre::Real mass, Ogre::Real radius);
        void setMotorMaxRpm(Ogre::Real rpm);
        void setMotorOmegaAccel(Ogre::Real rpmPerSec);
        void setMotorFrictionLoss(Ogre::Real newtonMeters);
        void setMotorTorqueScale(Ogre::Real nmPerUnit);
        void setCanDrive(bool canDrive);

        Ogre::Vector3 getVehicleForce() const;

        ndMultiBodyVehicle* getVehicleModel() const
        {
            return m_vehicleModel;
        }

        ndMultiBodyVehicleMotor* getMotor() const
        {
            return m_motor;
        }

        //! Configure simple 4-wheel layout with front/rear diffs (and optional center diff)
        void buildDrivetrain(ComplexDriveLayout layout, Ogre::Real diffMass, Ogre::Real diffRadius,
            Ogre::Real slipOmegaLockFront, Ogre::Real slipOmegaLockRear, Ogre::Real slipOmegaLockCenter = 0.0f);

        ComplexDriveLayout getDriveLayout(void) const
        {
            return m_driveLayout;
        }

        ndMultiBodyVehicleDifferential* getFrontDifferential(void) const
        {
            return m_frontDiff;
        }

        ndMultiBodyVehicleDifferential* getRearDifferential(void) const
        {
            return m_rearDiff;
        }

        ndMultiBodyVehicleDifferential* getCenterDifferential(void) const
        {
            return m_centerDiff;
        }
    private:
        void findAxleTires(ComplexVehicleTire*& frontLeft, ComplexVehicleTire*& frontRight, ComplexVehicleTire*& rearLeft, ComplexVehicleTire*& rearRight);

        void ensureVehicleModel();
        void applyForceAndTorque(OgreNewt::Body* vBody, const Ogre::Vector3& vForce, const Ogre::Vector3& vPoint);
        void updateDriverInput(ComplexVehicleTire* tire, Ogre::Real timestep);
        void update(Ogre::Real timestep, int threadIndex);

    private:
        ComplexDriveLayout m_driveLayout;

        World* m_world;
        ComplexVehicleCallback* m_vehicleCallback;

        ndMultiBodyVehicle* m_vehicleModel;
        bool m_vehicleAddedToWorld;

        std::vector<ComplexVehicleTire*> m_complexTires;
        int m_tireCount;

        Ogre::Real m_mass;
        Ogre::Vector3 m_comLocal;

        bool m_canDrive;
        Ogre::Real m_gravity;

        Ogre::Vector3 m_vehicleForce;

        ndMultiBodyVehicleMotor* m_motor;
        Ogre::Real m_motorMaxRpm;
        Ogre::Real m_motorOmegaAccel;
        Ogre::Real m_motorFrictionLoss;
        Ogre::Real m_motorTorqueScale;

        ndMultiBodyVehicleDifferential* m_frontDiff;
        ndMultiBodyVehicleDifferential* m_rearDiff;
        ndMultiBodyVehicleDifferential* m_centerDiff;
    };

} // namespace OgreNewt

#endif

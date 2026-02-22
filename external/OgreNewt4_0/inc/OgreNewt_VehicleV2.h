/* Copyright (c) <2003-2022> <Newton Game Dynamics>
 *
 * OgreNewt4 impulse-based vehicle – no raycast, no joints.
 * Single ndBodyDynamic with chassis collision.
 * ApplyImpulsePair() drives longitudinal / lateral / turning every substep.
 * Tire spin math is fully encapsulated in computeTireTransforms().
 */

#ifndef OGRENEWT_VEHICLEV2_H
#define OGRENEWT_VEHICLEV2_H

#include "OgreNewt_Body.h"
#include "OgreNewt_Prerequisites.h"
#include <vector>

namespace OgreNewt
{

    // =============================================================================
    // VehicleDrivingManipulationV2
    // Passed into each of the four Lua callbacks so the script sets the desired
    // values.  Interface mirrors the old VehicleDrivingManipulation so Lua scripts
    // need only minor renaming.
    // =============================================================================
    class _OgreNewtExport VehicleDrivingManipulationV2
    {
    public:
        VehicleDrivingManipulationV2();

        /** Steering angle in degrees [-maxAngle .. maxAngle].
         *  Positive = right, negative = left (same convention as old vehicle). */
        void setSteerAngle(Ogre::Real steerAngle);
        Ogre::Real getSteerAngle() const;

        /** Motor (drive) force in Newton-like units.
         *  Positive = forward, negative = reverse.
         *  Scale: same order of magnitude as old vehicle (e.g. 10000 * 120 * dt). */
        void setMotorForce(Ogre::Real motorForce);
        Ogre::Real getMotorForce() const;

        /** Hand-brake factor (e.g. 5.5).
         *  Scales lateral + longitudinal velocity cancellation. */
        void setHandBrake(Ogre::Real handBrake);
        Ogre::Real getHandBrake() const;

        /** Regular brake factor (e.g. 7.5).
         *  Damps forward/backward velocity. */
        void setBrake(Ogre::Real brake);
        Ogre::Real getBrake() const;

    private:
        friend class VehicleV2;
        Ogre::Real m_steerAngle;
        Ogre::Real m_motorForce;
        Ogre::Real m_handBrake;
        Ogre::Real m_brake;
    };

    // =============================================================================
    // VehicleV2Callback
    // Four separate methods mirror the old VehicleCallback API exactly so that
    // existing Lua callback table entries work with minimal changes.
    // All methods run on the Newton worker thread – no Ogre scene calls here.
    // =============================================================================
    class _OgreNewtExport VehicleV2Callback
    {
    public:
        virtual ~VehicleV2Callback() = default;

        /** Return the desired steer angle (degrees).
         *  Call manip->setSteerAngle() inside. */
        virtual Ogre::Real onSteerAngleChanged(VehicleDrivingManipulationV2* manip, Ogre::Real dt) = 0;

        /** Return the motor force.
         *  Call manip->setMotorForce() inside. */
        virtual Ogre::Real onMotorForceChanged(VehicleDrivingManipulationV2* manip, Ogre::Real dt) = 0;

        /** Return the hand-brake factor (e.g. 5.5).
         *  Call manip->setHandBrake() inside. */
        virtual Ogre::Real onHandBrakeChanged(VehicleDrivingManipulationV2* manip, Ogre::Real dt) = 0;

        /** Return the brake factor (e.g. 7.5).
         *  Call manip->setBrake() inside. */
        virtual Ogre::Real onBrakeChanged(VehicleDrivingManipulationV2* manip, Ogre::Real dt) = 0;
    };

    // =============================================================================
    // TireInfoV2  – per-tire data, fully managed inside VehicleV2
    // =============================================================================
    struct TireInfoV2
    {
        Ogre::SceneNode* sceneNode;   ///< Root-level tire scene node (not child of chassis)
        Ogre::Vector3 localPos;       ///< Tire centre in chassis-local space at bind time
        Ogre::Quaternion localOrient; ///< Tire orientation in chassis-local space at bind time
        Ogre::Real invRadius;         ///< Signed 1/radius: positive = right, negative = left
        Ogre::Real spinAngle;         ///< Accumulated spin (radians)
        bool isFrontTire;
        // Written by computeTireTransforms(); read by the component for GraphicsModule.
        Ogre::Vector3 worldPos;
        Ogre::Quaternion worldOrient;
    };

    // =============================================================================
    // VehicleV2
    // =============================================================================
    /**
     * @brief Impulse-based rigid-body vehicle for Newton Dynamics 4 / OgreNewt4.
     *
     * Design (mirrors ndBasicModel from the Newton4 demo):
     *  - Single ndBodyDynamic, chassis collision only.
     *  - applyImpulses() is called every physics substep from the body's force
     *    callback. It queries the four VehicleV2Callback methods for steer /
     *    motor / handBrake / brake, then applies three impulse pairs:
     *      1. Longitudinal  – motorForce drives forward/reverse speed
     *      2. Lateral       – cancels sideways sliding (scaled by brake/handBrake)
     *      3. Yaw           – steerAngle drives yaw rate correction
     *  - computeTireTransforms(dt) is called once per logic frame from the
     *    component's update(). It accumulates per-tire spin angle and writes the
     *    resulting world position / orientation into TireInfoV2::worldPos and
     *    TireInfoV2::worldOrient. The component then hands these to
     *    GraphicsModule::updateNodeTransform().
     *
     * No joints. No raycast. No Ogre scene graph calls inside applyImpulses().
     */
    class _OgreNewtExport VehicleV2 : public Body
    {
    public:
        /**
         * @param world        OgreNewt world
         * @param sceneManager Ogre scene manager
         * @param collision    Pre-built chassis collision shape
         * @param mass         Vehicle mass in kg (e.g. 1200)
         * @param massOrigin   Centre-of-mass in chassis-local space
         * @param gravity      Gravity vector (only for mass setup; actual gravity
         *                     is applied by the base force callback)
         * @param callback     Control callback – VehicleV2 takes ownership
         */
        VehicleV2(OgreNewt::World* world, Ogre::SceneManager* sceneManager, OgreNewt::CollisionPtr collision, Ogre::Real mass, const Ogre::Vector3& massOrigin, const Ogre::Vector3& gravity, const Ogre::Vector3& defaultDirection,
            VehicleV2Callback* callback);

        virtual ~VehicleV2();

        // ── Tire management ───────────────────────────────────────────────────────

        /**
         * @param sceneNode   Root-level tire scene node
         * @param localPos    Tire centre in chassis-local space
         * @param localOrient Tire orientation in chassis-local space
         * @param radius      Tire radius in world units (> 0.05)
         * @param isLeft      true = left side (spin direction negated vs right)
         */
        void addTire(Ogre::SceneNode* sceneNode, const Ogre::Vector3& localPos, const Ogre::Quaternion& localOrient, Ogre::Real radius, bool isLeft);

        void clearTires();

        std::vector<TireInfoV2>& getTires();
        const std::vector<TireInfoV2>& getTires() const;

        // ── Per-frame tire math (logic thread, once per frame) ───────────────────

        /**
         * @brief Accumulate spin angle and compute worldPos / worldOrient for
         *        every registered tire.  Must be called from the logic thread.
         *
         * The component reads TireInfoV2::worldPos and TireInfoV2::worldOrient
         * after this call and passes them to GraphicsModule::updateNodeTransform().
         *
         * @param dt Logic-frame delta time (seconds)
         */
        void computeTireTransforms(Ogre::Real dt);

        // ── Control / state queries ───────────────────────────────────────────────

        void setCanDrive(bool canDrive);
        bool getCanDrive() const;

        /** true if at least one active contact exists on the chassis body. */
        bool isOnGround() const;
        bool isAirborne() const;

        /** Last longitudinal force applied (for HUD / camera). */
        Ogre::Vector3 getVehicleForce() const;

        // ── Physics tick (Newton worker thread) ───────────────────────────────────

        /**
         * @brief Called from the body's force callback every substep.
         *        Queries the callback, then applies longitudinal / lateral / yaw
         *        impulses.  Must NOT touch any Ogre objects.
         */
        void applyImpulses(Ogre::Real dt);

        void setTireDirectionSwap(bool swap);

        bool getTireDirectionSwap() const;

        void setSteeringStrength(Ogre::Real strength);

        Ogre::Real getSteeringStrength() const;

        void setSpinAxis(int axis);

        int getSpinAxis() const;

         // Called from main thread (mirrors SpiderBody::updateMovementInput)
        void updateVehicleInput(Ogre::Real dt);

    private:
        void applyLongitudinalImpulse(Ogre::Real motorForce, Ogre::Real brakeForce, Ogre::Real handBrake, Ogre::Real dt);
        void applyLateralImpulse(Ogre::Real handBrake, Ogre::Real dt);
        void applyTurningImpulse(Ogre::Real steerDeg, Ogre::Real motorForce, Ogre::Real dt);

    private:
        struct CachedInputs
        {
            Ogre::Real steer{0}, motor{0}, handBrake{0}, brake{0};
        };
        mutable std::mutex m_inputMutex;
        CachedInputs m_cachedInputs;
    private:
        VehicleV2Callback* m_callback;
        VehicleDrivingManipulationV2 m_manip;
        std::vector<TireInfoV2> m_tires;
        bool m_canDrive;
        mutable Ogre::Vector3 m_vehicleForce;
        Ogre::Vector3 m_defaultDirection;
        Ogre::Real m_currentSteerAngle;
        bool m_tireDirectionSwap;
        Ogre::Real m_steeringStrength;
        Ogre::Real m_frontAxleLocalX; // avg forward offset of front tires from CoM
        Ogre::Real m_rearAxleLocalX;  // avg forward offset of rear tires from CoM
        int m_frontTireCount;         // used during addTire() averaging
        int m_rearTireCount;
        int m_spinAxis; // 0=X, 1=Y, 2=Z
    };

} // namespace OgreNewt

#endif // OGRENEWT_VEHICLEV2_H

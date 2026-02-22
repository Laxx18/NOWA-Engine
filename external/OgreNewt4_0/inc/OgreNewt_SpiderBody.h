/* OgreNewt_SpiderBody.h
 *
 * Procedural IK spider body for Newton Dynamics 4 / OgreNewt4.
 * One ndBodyDynamic for the torso. All leg segments are purely visual
 * SceneNodes driven by a procedural gait + 2-link analytical IK.
 *
 * Mirrors the VehicleV2 / ndQuadSpiderPlayer architecture:
 *   - applyLocomotionImpulses()  called from the force callback (Newton thread)
 *   - computeLegTransforms(dt)   called from the logic thread once per frame;
 *     fills LegChainInfo world transforms for the component to push to GraphicsModule
 */

#ifndef OGRENEWT_SPIDERBODY_H
#define OGRENEWT_SPIDERBODY_H

#include "OgreNewt_Body.h"
#include "OgreNewt_Prerequisites.h"
#include <vector>

namespace OgreNewt
{
    // =========================================================================
    // SpiderMovementManipulation
    // Passed to the single Lua callback so scripts set desired movement values.
    // Mirrors VehicleDrivingManipulationV2's interface style.
    // =========================================================================
    class _OgreNewtExport SpiderMovementManipulation
    {
    public:
        SpiderMovementManipulation();

        /** Forward stride length per step (0 = stand still, e.g. 0.3). */
        void setStride(Ogre::Real stride);
        Ogre::Real getStride() const;

        /** Yaw rate (rad/s). Positive = turn right, negative = left.
         *  Typical value ±0.25. */
        void setOmega(Ogre::Real omega);
        Ogre::Real getOmega() const;

        /** Body sway offset along the chassis right axis (optional cosmetic). */
        void setBodySwayX(Ogre::Real sway);
        Ogre::Real getBodySwayX() const;

        /** Body sway offset along the chassis forward axis (optional cosmetic). */
        void setBodySwayZ(Ogre::Real sway);
        Ogre::Real getBodySwayZ() const;

    private:
        friend class SpiderBody;
        Ogre::Real m_stride;
        Ogre::Real m_omega;
        Ogre::Real m_bodySwayX;
        Ogre::Real m_bodySwayZ;
    };

    // =========================================================================
    // SpiderCallback (abstract)
    // One method mirrors the ndModelNotify::Update callback.
    // Runs on the Newton worker thread – no Ogre scene calls here.
    // =========================================================================
    class _OgreNewtExport SpiderCallback
    {
    public:
        virtual ~SpiderCallback() = default;

        /** Set desired movement on @manip.  Called every physics substep. */
        virtual void onMovementChanged(SpiderMovementManipulation* manip, Ogre::Real dt) = 0;
    };

    // =========================================================================
    // LegChainInfo
    // Per-leg visual nodes + bind-pose geometry + IK outputs.
    // =========================================================================
    struct _OgreNewtExport LegChainInfo
    {
        // Visual scene nodes (root-level – NOT children of the chassis node).
        // Each node's local origin must sit at its joint (hip, knee, ankle).
        Ogre::SceneNode* thigh = nullptr;
        Ogre::SceneNode* calf = nullptr;
        Ogre::SceneNode* heel = nullptr;

        // Hip socket in torso-LOCAL space (captured at bind time).
        Ogre::Vector3 hipLocalPos = Ogre::Vector3::ZERO;

        // Initial resting foot position in torso-LOCAL space (captured at bind time).
        Ogre::Vector3 footRestLocal = Ogre::Vector3::ZERO;

        // Bone lengths measured from the bind pose world positions.
        Ogre::Real thighLength = 0.5f; // hip → knee
        Ogre::Real calfLength = 0.5f;  // knee → ankle

        // Which way the knee bends: +1 outward from body centre, -1 inward.
        // Derived automatically from the sign of hipLocalPos along the lateral axis.
        Ogre::Real swivelSign = 1.0f;

        // Local axis along which each limb mesh extends (0=X, 1=Y, 2=Z).
        // Must match the artist's rig.  Default 1 (Y-up bones, Blender default).
        int boneAxis = 1;

        // ── Outputs written by computeLegTransforms() ─────────────────────────
        // The component reads these and calls GraphicsModule::updateNodeTransform.
        Ogre::Vector3 thighWorldPos;
        Ogre::Quaternion thighWorldOrient;

        Ogre::Vector3 kneeWorldPos; // knee (calf node origin)
        Ogre::Quaternion calfWorldOrient;

        Ogre::Vector3 footWorldPos; // ankle (heel node origin)
        Ogre::Quaternion heelWorldOrient;
    };

    // =========================================================================
    // LegGaitState
    // Per-leg procedural gait data (ported from ndProdeduralGaitGenerator).
    // All positions are in TORSO-LOCAL space.
    // =========================================================================
    struct _OgreNewtExport LegGaitState
    {
        Ogre::Vector3 base;  ///< Rest foot position (torso-local)
        Ogre::Vector3 posit; ///< Current foot position (torso-local)
        Ogre::Vector3 start; ///< Start of current interpolation
        Ogre::Vector3 end;   ///< End of current interpolation

        // Parabolic arch Y coefficients: y = a0 + a1*t + a2*t^2  (t in [0,1])
        Ogre::Real a0, a1, a2;

        Ogre::Real time;    ///< Elapsed time in current phase
        Ogre::Real maxTime; ///< Duration of current phase
    };

    // =========================================================================
    // SpiderBody
    // =========================================================================
    /**
     * Procedural IK spider body.
     *
     * Gait timing mirrors ndGeneratorWalkGait:
     *   [0 .. 25% cycle]  = air phase  (foot swings forward)
     *   [25% .. 100%]     = ground phase (foot slides backward / body advances)
     *
     * Each leg is phase-offset by 0.25 * gaitSequence[i], producing the
     * classic diagonal walk when gaitSequence = {3,1,2,0}.
     */
    class _OgreNewtExport SpiderBody : public Body
    {
    public:
        /**
         * @param callback   Ownership is transferred.  Destroyed in ~SpiderBody().
         */
        SpiderBody(OgreNewt::World* world, Ogre::SceneManager* sceneManager, OgreNewt::CollisionPtr collision, Ogre::Real mass, const Ogre::Vector3& massOrigin, const Ogre::Vector3& gravity, const Ogre::Vector3& defaultDirection,
            SpiderCallback* callback);

        virtual ~SpiderBody();

        // ── Leg registration (called once in connect / registerLegChains) ─────

        /**
         * Register one leg chain.
         *
         * @param thigh          Thigh scene node  (origin = hip joint)
         * @param calf           Calf  scene node  (origin = knee joint)
         * @param heel           Heel  scene node  (origin = ankle joint)
         * @param hipLocalPos    Hip socket in chassis-local space
         * @param footRestLocal  Initial resting foot pos in chassis-local space
         * @param thighLength    Distance hip→knee at bind pose
         * @param calfLength     Distance knee→ankle at bind pose
         * @param swivelSign     +1 = knee bends away from body centre
         * @param boneAxis       0=X, 1=Y, 2=Z – axis along which each mesh extends
         */
        void addLegChain(Ogre::SceneNode* thigh, Ogre::SceneNode* calf, Ogre::SceneNode* heel, const Ogre::Vector3& hipLocalPos, const Ogre::Vector3& footRestLocal, Ogre::Real thighLength, Ogre::Real calfLength, Ogre::Real swivelSign,
            int boneAxis = 1);

        void clearLegs();

        // ── Per-frame leg math (logic thread, once per frame) ─────────────────

        /**
         * Tick gait state machine + solve 2-link IK for all legs.
         * Writes world pos/orient into LegChainInfo for each segment.
         * The component then pushes these to GraphicsModule::updateNodeTransform().
         * No Ogre scene graph calls are made here.
         *
         * @param dt  Logic-frame delta time (seconds)
         */
        void computeLegTransforms(Ogre::Real dt);

        // ── Physics tick (Newton worker thread) ───────────────────────────────

        /**
         * Called from the body's force callback every substep.
         * Queries SpiderCallback for stride/omega, applies directional + yaw
         * impulses.  Must NOT touch any Ogre objects.
         */
        void applyLocomotionImpulses(Ogre::Real dt);

        // ── Accessors ────────────────────────────────────────────────────────

        const std::vector<LegChainInfo>& getLegChains() const;
        const std::vector<LegGaitState>& getGaitStates() const;

        void setCanMove(bool canMove);
        bool getCanMove() const;

        void setWalkCycleDuration(Ogre::Real duration);
        Ogre::Real getWalkCycleDuration() const;

        void setStepHeight(Ogre::Real height);
        Ogre::Real getStepHeight() const;

        /** Gait sequence: phase-ordering per leg index (e.g. {3,1,2,0}).
         *  Must be set BEFORE addLegChain() or immediately after clearLegs(). */
        void setGaitSequence(const std::vector<int>& seq);
        const std::vector<int>& getGaitSequence() const;

        bool isOnGround() const;

        // Called from the MAIN THREAD (update/computeLegTransforms).
        // Runs the callback, caches the result so applyLocomotionImpulses can read it safely.
        void updateMovementInput(Ogre::Real dt);


    private:
        // ─── Gait ──────────────────────────────────────────────────────────
        enum class GaitPhase
        {
            onGround,
            groundToAir,
            onAir,
            airToGround
        };

        // Manip values written by the main thread, read by the Newton thread.
        // Protected by m_manipMutex.
        struct CachedManip
        {
            Ogre::Real stride = 0.0f;
            Ogre::Real omega = 0.0f;
            Ogre::Real bodySwayX = 0.0f;
            Ogre::Real bodySwayZ = 0.0f;
        };
        CachedManip m_cachedManip;
        mutable std::mutex m_manipMutex;

        GaitPhase getGaitPhase(int legIndex, Ogre::Real timestep) const;
        void integrateLeg(int legIndex, Ogre::Real timestep);

        // ─── IK ────────────────────────────────────────────────────────────
        /**
         * Solves 2-link IK (hip→knee→ankle) using law of cosines.
         * @param leg         Pre-filled thighWorldPos and footWorldPos.
         * @param chassisUp   World-space chassis up vector (for bend-plane + orient stability).
         */
        void solveIK(LegChainInfo& leg, const Ogre::Vector3& chassisUp);

        /**
         * Build world orientation for one bone segment.
         * Aligns boneAxis to boneDir; uses upHint to stabilise roll.
         */
        static Ogre::Quaternion buildBoneOrient(const Ogre::Vector3& boneDir, const Ogre::Vector3& upHint, int boneAxis);

    private:
        SpiderCallback* m_callback;
        SpiderMovementManipulation m_manip; // filled by callback, read by applyLocomotionImpulses

        std::vector<LegChainInfo> m_legs;
        std::vector<LegGaitState> m_gait;
        std::vector<int> m_gaitSequence; // phase ordering per leg

        Ogre::Vector3 m_defaultDirection; // forward axis in chassis-local space
        Ogre::Real m_walkCycleDuration;   // seconds for one full gait cycle
        Ogre::Real m_stepHeight;          // parabolic arch amplitude
        Ogre::Real m_timeAcc;             // gait time accumulator (s)
        bool m_canMove;
    };

} // namespace OgreNewt

#endif // OGRENEWT_SPIDERBODY_H
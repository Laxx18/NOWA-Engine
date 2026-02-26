/*
    OgreNewt_SpiderBody.h  (OgreNewt4 / NOWA-Engine)

    Architecture mirrors Newton Dynamics 4 ndQuadSpiderPlayer demo exactly:

      SpiderArticulation  — ndModelArticulation subclass, holds raw ptrs to bodies/joints
      SpiderModelNotify   — ndModelNotify subclass, runs gait + IK each physics step
      SpiderBody          — public OgreNewt wrapper (inherits OgreNewt::Body for the torso)

    Gait is a faithful port of ndProdeduralGaitGenerator / ndGeneratorWalkGait:
      - duration=2s, timeLine[1]=0.5s (25% air), timeLine[2]=2.0s (75% ground)
      - per-leg phase offset: fmod(timeAcc + gaitSeq[i]*0.25*dur, dur)
      - states: groundToAir, onAir, airToGround, onGround
      - positions live in effector LOCAL space (hip-pivot frame, from GetEffectorPosit())

    Body sway mirrors ndBodySwingControl:
      localMatrix.TransformVector(posit) → apply sway → UntransformVector → SetLocalTargetPosition

    Effector constructor used correctly:
      ndIkSwivelPositionEffector(
          pinAndPivotParentInGlobalSpace,   // torso orient + hip socket world pos
          parentBody (torso),
          childPivotInGlobalSpace,          // foot/contact world pos
          childBody (heel))
*/
#pragma once

#include "OgreNewt_Body.h"
#include "OgreNewt_BodyNotify.h"
#include "OgreNewt_Prerequisites.h"
#include "OgreNewt_World.h"

#include "ndNewton.h"

#include <OgreQuaternion.h>
#include <OgreSceneNode.h>
#include <OgreVector3.h>
#include <array>
#include <atomic>
#include <mutex>
#include <vector>

namespace OgreNewt
{

    // ─────────────────────────────────────────────────────────────────────────────
    //  Gait phase state (per leg)
    // ─────────────────────────────────────────────────────────────────────────────
    enum class LegGaitPhase
    {
        onGround,
        groundToAir,
        onAir,
        airToGround
    };

    // Newton-style pose (all vectors in effector LOCAL space — hip-pivot frame)
    struct LegPose
    {
        ndVector base;     // rest (neutral) position, from effector->GetEffectorPosit()
        ndVector start;    // start of current phase
        ndVector end;      // end target of current phase
        ndVector posit;    // current interpolated position (sent to SetLocalTargetPosition)
        ndFloat32 time;    // time elapsed in current phase
        ndFloat32 maxTime; // duration of current phase
        ndFloat32 a0;      // parabola Y = a0 + a1*u + a2*u*u  (air phase only)
        ndFloat32 a1;
        ndFloat32 a2;
    };

    // ─────────────────────────────────────────────────────────────────────────────
    //  Leg transform cache — written by physics thread (PostTransformUpdate),
    //  read by main thread (component::updateLegNodes).
    //  Protected by a mutex; only 4 legs and small structs so contention is minimal.
    // ─────────────────────────────────────────────────────────────────────────────
    struct LegTransformCache
    {
        Ogre::Vector3 thighPos{};
        Ogre::Quaternion thighOrient;
        Ogre::Vector3 calfPos{};
        Ogre::Quaternion calfOrient;
        Ogre::Vector3 heelPos{};
        Ogre::Quaternion heelOrient;
        bool valid{false};
    };

    // ─────────────────────────────────────────────────────────────────────────────
    //  Lua-facing movement manipulation (main-thread read/write only)
    // ─────────────────────────────────────────────────────────────────────────────
    class _OgreNewtExport SpiderMovementManipulation
    {
    public:
        void setStride(float v)
        {
            m_stride = v;
        }
        float getStride() const
        {
            return m_stride;
        }
        void setOmega(float v)
        {
            m_omega = v;
        }
        float getOmega() const
        {
            return m_omega;
        }
        void setBodySwayX(float v)
        {
            m_bodySwayX = v;
        }
        float getBodySwayX() const
        {
            return m_bodySwayX;
        }
        void setBodySwayZ(float v)
        {
            m_bodySwayZ = v;
        }
        float getBodySwayZ() const
        {
            return m_bodySwayZ;
        }

        // Internal: copy into atomics on SpiderModelNotify
        float m_stride = 0.0f;
        float m_omega = 0.0f;
        float m_bodySwayX = 0.0f; // lateral lean (cosmetic)
        float m_bodySwayZ = 0.0f; // forward tilt (cosmetic)
    };

    // ─────────────────────────────────────────────────────────────────────────────
    //  Callback interface
    // ─────────────────────────────────────────────────────────────────────────────
    class _OgreNewtExport SpiderCallback
    {
    public:
        virtual ~SpiderCallback() = default;
        // Called every frame on the MAIN THREAD to let Lua set stride/omega.
        virtual void onMovementChanged(SpiderMovementManipulation* manip, float dt) = 0;
    };

    // ─────────────────────────────────────────────────────────────────────────────
    //  SpiderArticulation — articulated model tree (no per-step logic here)
    // ─────────────────────────────────────────────────────────────────────────────
    class SpiderArticulation : public ndModelArticulation
    {
    public:
        SpiderArticulation() = default;
        ~SpiderArticulation() override = default;

        // Torso
        ndBodyDynamic* torsoNd{nullptr};

        // Leg bodies (raw ptrs into ndSharedPtr-owned objects in the tree)
        static constexpr int MAX_LEGS = 4;
        std::array<ndBodyDynamic*, MAX_LEGS> thighNd{};
        std::array<ndBodyDynamic*, MAX_LEGS> calfNd{};
        std::array<ndBodyDynamic*, MAX_LEGS> heelNd{};
        std::array<ndBodyDynamic*, MAX_LEGS> contactNd{};

        // Joints
        std::array<ndIkSwivelPositionEffector*, MAX_LEGS> effectors{};
        std::array<ndIkJointHinge*, MAX_LEGS> kneeJoints{};
        std::array<ndJointHinge*, MAX_LEGS> heelJoints{};

        // Gait state — only written/read by SpiderModelNotify (physics thread)
        std::array<LegPose, MAX_LEGS> legPose{};
        std::array<LegGaitPhase, MAX_LEGS> legPhase{};
        std::array<ndFloat32, MAX_LEGS> legCycleTime{};

        // Gait configuration (set once before simulation)
        ndFloat32 duration{2.0f};     // total cycle duration (s)
        ndFloat32 airFraction{0.25f}; // fraction of cycle that is air phase
        std::array<int, MAX_LEGS> gaitSequence{3, 1, 2, 0};
        int activeLegCount{0};

        // Stride / omega: written by main thread (fireLuaCallback), read by physics thread.
        // Body sway: same threading model.
        std::atomic<ndFloat32> stride{0.0f};
        std::atomic<ndFloat32> omega{0.0f};
        std::atomic<ndFloat32> bodySwayX{0.0f};
        std::atomic<ndFloat32> bodySwayZ{0.0f};
        std::atomic<bool> canMove{true};

        // Step height (set once via setStepHeight, read by physics thread — stable value)
        ndFloat32 stepHeight{0.20f};

        // Transform cache written by PostTransformUpdate, read by main thread
        mutable std::mutex transformMutex;
        std::array<LegTransformCache, MAX_LEGS> cachedTransforms{};
    };

    // ─────────────────────────────────────────────────────────────────────────────
    //  SpiderModelNotify — gait engine, runs on physics thread
    //  Faithfully ports ndProdeduralGaitGenerator + ndGeneratorWalkGait + ndBodySwingControl
    // ─────────────────────────────────────────────────────────────────────────────
    class SpiderModelNotify : public ndModelNotify
    {
    public:
        explicit SpiderModelNotify(SpiderArticulation* model);
        ~SpiderModelNotify() override = default;

        // Called by Newton every physics step (physics thread)
        void Update(ndFloat32 timestep) override;

        // Called by Newton after transform integration (physics thread)
        // Captures leg body world transforms into the cache.
        void PostTransformUpdate(ndFloat32 timestep) override;

    private:
        // Compute next timeAcc (angular wrap — matches Newton's CalculateTime)
        ndFloat32 advanceTime(ndFloat32 current, ndFloat32 timestep) const;

        // Determine the transition state for a given leg this tick.
        // Matches Newton's ndProdeduralGaitGenerator::GetState.
        LegGaitPhase getState(int legIndex, ndFloat32 timeAcc, ndFloat32 timestep) const;

        // Integrate one leg's gait pose. Matches IntegrateLeg.
        void integrateLeg(int legIndex, LegGaitPhase state, ndFloat32 stride, ndFloat32 omega, ndFloat32 timestep);

        // Apply gait poses to effectors (including body sway + swivel + knee coupling).
        // Matches ndController::Update logic.
        void applyEffectors();

        SpiderArticulation* m_model{nullptr};
        ndFloat32 m_timeAcc{0.0f}; // global gait clock
    };

    // ─────────────────────────────────────────────────────────────────────────────
    //  SpiderBody — OgreNewt wrapper (IS the torso OgreNewt::Body)
    // ─────────────────────────────────────────────────────────────────────────────
    class _OgreNewtExport SpiderBody : public OgreNewt::Body
    {
    public:
        //! Create torso physics body and empty articulated model.
        //! Call addLegChain() for each leg, then finalizeModel() before simulation.
        SpiderBody(OgreNewt::World* world, Ogre::SceneManager* sceneManager, const OgreNewt::CollisionPtr& collision, float mass, const Ogre::Vector3& massOrigin, const Ogre::Vector3& gravity, const Ogre::Vector3& defaultDirection,
            const Ogre::Vector3& initialPosition, const Ogre::Quaternion& initialOrientation, SpiderCallback* callback);

        ~SpiderBody() override;

        //! Build one leg chain. Matches ndController::CreateArticulatedModel leg loop.
        //! hipLocal / footRestLocal are in TORSO-LOCAL space.
        //! thighLen / calfLen are bone lengths (physics capsule lengths).
        //! swivelSign: +1 for right legs, -1 for left legs.
        //! boneAxis: 0=X, 1=Y, 2=Z — long axis of provided capsule (ignored for physics
        //!   capsule orientation which is always derived from the joint chain).
        void addLegChain(Ogre::SceneNode* thighNode, Ogre::SceneNode* calfNode, Ogre::SceneNode* heelNode, const Ogre::Vector3& hipLocal, const Ogre::Vector3& footRestLocal, float thighLen, float calfLen, float swivelSign, int boneAxis);

        //! Must be called once after all addLegChain() calls.
        //! Creates the ndModelArticulation notify and adds model to world.
        void finalizeModel();

        //! Remove all legs (called on disconnect before destruction).
        void clearLegs();

        // ── Gait parameters ───────────────────────────────────────────────────────
        void setWalkCycleDuration(float seconds);
        void setStepHeight(float meters);
        void setGaitSequence(const std::vector<int>& seq);
        void setCanMove(bool b);

        //! Fire the Lua callback (main thread). Reads manip values → pushes to atomics.
        void fireLuaCallback(float dt);

        //! Read cached leg transforms (main thread, after physics step).
        const LegTransformCache& getCachedLegTransform(int legIndex) const;
        int getActiveLegCount() const;

        bool isFinalized() const
        {
            return m_finalized;
        }

        bool isOnGround() const;

    private:
        // Returns the torso ndBodyDynamic. Valid only after finalizeModel().
        ndBodyDynamic* getTorsoNd() const;

    private:

        SpiderArticulation* m_artModel{nullptr}; // owned by ndWorld after finalizeModel()
        SpiderModelNotify* m_notify{nullptr};    // owned by ndSharedPtr inside m_artModel

        // OgreNewt wrappers for leg segment visuals
        // These do NOT add to world (bodies are owned by the articulation)
        struct LegWrappers
        {
            Ogre::SceneNode* thighNode{nullptr};
            Ogre::SceneNode* calfNode{nullptr};
            Ogre::SceneNode* heelNode{nullptr};
        };
        std::array<LegWrappers, SpiderArticulation::MAX_LEGS> m_legWrappers{};

        SpiderCallback* m_callback{nullptr}; // owned externally (PhysicsSpiderCallback)
        SpiderMovementManipulation m_manip;

        OgreNewt::CollisionPtr m_pendingCollision;
        float m_pendingMass{20.0f};
        Ogre::Vector3 m_pendingMassOrigin{Ogre::Vector3::ZERO};
        Ogre::Vector3 m_pendingGravity{Ogre::Vector3(0.0f, -9.8f, 0.0f)};
        Ogre::Vector3 m_pendingPosition{Ogre::Vector3::ZERO};
        Ogre::Quaternion m_pendingOrientation{Ogre::Quaternion::IDENTITY};

        bool m_finalized{false};
    };

} // namespace OgreNewt
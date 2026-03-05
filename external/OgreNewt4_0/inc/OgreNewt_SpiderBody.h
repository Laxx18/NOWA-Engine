/*
    OgreNewt_SpiderBody.h  (OgreNewt4 / NOWA-Engine)

    Architecture mirrors Newton Dynamics 4 ndQuadSpiderPlayer demo exactly:

      SpiderArticulation  — ndModelArticulation subclass, holds all bodies/joints
      SpiderModelNotify   — ndModelNotify subclass, runs gait + IK each physics step
      SpiderBody          — public OgreNewt wrapper (inherits OgreNewt::Body for the torso)

    Gait is a faithful port of ndProdeduralGaitGenerator / ndGeneratorWalkGait:
      - duration=2s, timeLine[1]=0.5s (25% air), timeLine[2]=2.0s (75% ground)
      - per-leg phase offset: fmod(timeAcc + gaitSeq[i]*0.25*dur, dur)
      - states: groundToAir, onAir, airToGround, onGround
      - positions live in effector LOCAL space (hip-pivot frame, from GetEffectorPosit())

    Body sway mirrors ndBodySwingControl:
      localMatrix.TransformVector(posit) -> apply sway -> UntransformVector -> SetLocalTargetPosition

    Effector constructor used correctly:
      ndIkSwivelPositionEffector(
          pinAndPivotParentInGlobalSpace,   // torso orient + hip socket world pos
          parentBody (torso),
          childPivotInGlobalSpace,          // foot/contact world pos
          childBody (heel))

    Threading model:
      - ALL Newton allocations (bodies, shapes, joints) happen inside
        enqueuePhysicsAndWait() in finalizeModel(), on Newton's own thread.
        This avoids races with ndFreeListAlloc which is NOT thread-safe from outside.
      - addLegChain() does pure math and stashes PendingLeg — zero Newton calls.
      - stride/omega/sway/canMove are std::atomic — no mutex needed.
      - cachedTransforms are guarded by transformMutex (physics writes, main reads).
      - anyLegOnGround is std::atomic<bool> written by PostTransformUpdate.
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

    // ─────────────────────────────────────────────────────────────────────────
    //  Gait phase state (per leg)
    // ─────────────────────────────────────────────────────────────────────────
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

    // ─────────────────────────────────────────────────────────────────────────
    //  Leg transform cache — written by physics thread (PostTransformUpdate),
    //  read by main thread (component::updateLegNodes), guarded by transformMutex.
    // ─────────────────────────────────────────────────────────────────────────
    struct LegTransformCache
    {
        Ogre::Vector3 thighPos{};
        Ogre::Quaternion thighOrient;
        Ogre::Vector3 calfPos{};
        Ogre::Quaternion calfOrient;
        Ogre::Vector3 heelPos{};
        Ogre::Quaternion heelOrient;
        bool valid{false};
        float thighLen{1.0f};
        float calfLen{1.0f};
        float heelLen{1.0f};
    };

    // ─────────────────────────────────────────────────────────────────────────
    //  Lua-facing movement manipulation (main-thread read/write only)
    // ─────────────────────────────────────────────────────────────────────────
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

        float m_stride = 0.0f;
        float m_omega = 0.0f;
        float m_bodySwayX = 0.0f;
        float m_bodySwayZ = 0.0f;
    };

    // ─────────────────────────────────────────────────────────────────────────
    //  Callback interface
    // ─────────────────────────────────────────────────────────────────────────
    class _OgreNewtExport SpiderCallback
    {
    public:
        virtual ~SpiderCallback() = default;
        // Called every frame on the MAIN THREAD to let Lua set stride/omega.
        virtual void onMovementChanged(SpiderMovementManipulation* manip, float dt) = 0;
    };

    // ─────────────────────────────────────────────────────────────────────────
    //  SpiderArticulation — articulated model tree (no per-step logic here)
    // ─────────────────────────────────────────────────────────────────────────
    class SpiderArticulation : public ndModelArticulation
    {
    public:
        SpiderArticulation() = default;
        ~SpiderArticulation() override = default;

        static constexpr int MAX_LEGS = 4;

        // ── Pending leg geometry — filled by addLegChain, consumed by finalizeModel ──
        struct PendingLeg
        {
            ndVector hipWorld;
            ndVector kneeWorld;
            ndVector ankleWorld;
            ndVector footWorld; // contact capsule bottom
            ndVector thighDir;
            ndVector calfDir;
            ndVector hingeAxis;
            ndVector thighCentre;
            ndVector calfCentre;
            ndVector heelCentre;
            ndVector contactCentre;
            ndFloat32 thighLen = 0.0f;
            ndFloat32 calfLen = 0.0f;
            ndFloat32 thighRadius = 0.0f;
            ndFloat32 calfRadius = 0.0f;
            ndMatrix effectPivot;   // torso orient + hip socket world pos
            ndVector effChildPivot; // ankle world pos
            ndFloat32 effStrength = 0.0f;
            // Visual nodes (Ogre-owned, not Newton)
            Ogre::SceneNode* thighNode = nullptr;
            Ogre::SceneNode* calfNode = nullptr;
            Ogre::SceneNode* heelNode = nullptr;
        };
        std::vector<PendingLeg> pendingLegs; // cleared by finalizeModel after enqueue

        // ── Physics bodies (raw observer ptrs — owned by articulation tree) ──
        ndBodyDynamic* torsoNd{nullptr};
        std::array<ndBodyDynamic*, MAX_LEGS> thighNd{};
        std::array<ndBodyDynamic*, MAX_LEGS> calfNd{};
        std::array<ndBodyDynamic*, MAX_LEGS> heelNd{};
        std::array<ndBodyDynamic*, MAX_LEGS> contactNd{};

        // ── Joint raw observer ptrs (for Update() hot path) ──────────────────
        std::array<ndIkSwivelPositionEffector*, MAX_LEGS> effectors{};
        std::array<ndIkJointHinge*, MAX_LEGS> kneeJoints{};
        std::array<ndJointHinge*, MAX_LEGS> heelJoints{};

        // ── Owning shared ptrs — keep joints alive for the model's lifetime ──
        // AddLimb/AddCloseLoop may or may not keep a ref internally; we always do.
        std::array<ndSharedPtr<ndJointBilateralConstraint>, MAX_LEGS> hipJointPtrs;
        std::array<ndSharedPtr<ndJointBilateralConstraint>, MAX_LEGS> kneeJointPtrs;
        std::array<ndSharedPtr<ndJointBilateralConstraint>, MAX_LEGS> heelJointPtrs;
        std::array<ndSharedPtr<ndJointBilateralConstraint>, MAX_LEGS> contactJointPtrs;
        std::array<ndSharedPtr<ndJointBilateralConstraint>, MAX_LEGS> effectorPtrs;
        ndSharedPtr<ndJointBilateralConstraint> upVectorJoint;

        // ── Gait state — only written/read by SpiderModelNotify (physics thread) ──
        std::array<LegPose, MAX_LEGS> legPose{};
        std::array<LegGaitPhase, MAX_LEGS> legPhase{};
        std::array<ndFloat32, MAX_LEGS> legCycleTime{};
        ndFloat32 thighLenArr[MAX_LEGS]{};
        ndFloat32 calfLenArr[MAX_LEGS]{};

        // ── Gait configuration (set once before simulation, main thread) ─────
        ndFloat32 duration{2.0f};
        ndFloat32 airFraction{0.25f};
        std::array<int, MAX_LEGS> gaitSequence{3, 1, 2, 0};
        int activeLegCount{0};
        ndFloat32 stepHeight{0.20f};

        // ── Cross-thread: main writes, physics reads ──────────────────────────
        std::atomic<ndFloat32> stride{0.0f};
        std::atomic<ndFloat32> omega{0.0f};
        std::atomic<ndFloat32> bodySwayX{0.0f};
        std::atomic<ndFloat32> bodySwayZ{0.0f};
        std::atomic<bool> canMove{true};

        // ── Cross-thread: physics writes, main reads ──────────────────────────
        // transformMutex guards cachedTransforms (bulk struct copy needs atomicity).
        // anyLegOnGround is a single bool — std::atomic suffices.
        mutable std::mutex transformMutex;
        std::array<LegTransformCache, MAX_LEGS> cachedTransforms{};
        std::atomic<bool> anyLegOnGround{false};
    };

    // ─────────────────────────────────────────────────────────────────────────
    //  SpiderModelNotify — gait engine, runs on physics thread
    // ─────────────────────────────────────────────────────────────────────────
    class SpiderModelNotify : public ndModelNotify
    {
    public:
        explicit SpiderModelNotify(SpiderArticulation* model);
        ~SpiderModelNotify() override = default;

        void Update(ndFloat32 timestep) override;
        void PostTransformUpdate(ndFloat32 timestep) override;

    private:
        ndFloat32 advanceTime(ndFloat32 current, ndFloat32 timestep) const;
        LegGaitPhase getState(int legIndex, ndFloat32 timeAcc, ndFloat32 timestep) const;
        void integrateLeg(int legIndex, LegGaitPhase state, ndFloat32 stride, ndFloat32 omega, ndFloat32 timestep);
        void applyEffectors();

        SpiderArticulation* m_model{nullptr};
        ndFloat32 m_timeAcc{0.0f};
    };

    // ─────────────────────────────────────────────────────────────────────────
    //  SpiderBody — OgreNewt wrapper (IS-A OgreNewt::Body for the torso)
    // ─────────────────────────────────────────────────────────────────────────
    class _OgreNewtExport SpiderBody : public OgreNewt::Body
    {
    public:
        SpiderBody(OgreNewt::World* world, Ogre::SceneManager* sceneManager, const OgreNewt::CollisionPtr& collision, float mass, const Ogre::Vector3& massOrigin, const Ogre::Vector3& gravity, const Ogre::Vector3& defaultDirection,
            const Ogre::Vector3& initialPosition, const Ogre::Quaternion& initialOrientation, SpiderCallback* callback);

        ~SpiderBody();

        // Torso is owned by articulation, not by m_bodyPtr.
        ndBodyKinematic* getNewtonBody() const override
        {
            return m_artModel ? m_artModel->torsoNd : nullptr;
        }

        // Register one leg. hipLocal / footRestLocal are in TORSO-LOCAL space.
        // Pure math + stash — no Newton allocations. Safe to call from main thread
        // before finalizeModel() / before simulation starts ticking.
        void addLegChain(Ogre::SceneNode* thighNode, Ogre::SceneNode* calfNode, Ogre::SceneNode* heelNode, const Ogre::Vector3& hipLocal, const Ogre::Vector3& footRestLocal, float thighLen, float calfLen, float swivelSign, int boneAxis);

        // Call once after all addLegChain() calls.
        // All Newton allocations happen here, deferred via enqueuePhysicsAndWait.
        void finalizeModel();

        // Remove leg visual node references (called on disconnect).
        void clearLegs();

        // ── Gait parameters ───────────────────────────────────────────────────
        void setWalkCycleDuration(float seconds);
        void setStepHeight(float meters);
        void setGaitSequence(const std::vector<int>& seq);
        void setCanMove(bool b);

        // Fire Lua callback (main thread). Reads manip -> pushes to atomics.
        void fireLuaCallback(float dt);

        // Read cached leg transforms (main thread).
        const LegTransformCache& getCachedLegTransform(int legIndex) const;
        int getActiveLegCount() const;
        bool isFinalized() const
        {
            return m_finalized;
        }
        bool isOnGround() const;

        void snapshotTransforms(std::array<OgreNewt::LegTransformCache, SpiderArticulation::MAX_LEGS>& out, int& legCountOut) const;

    private:
        SpiderArticulation* m_artModel{nullptr};
        ndSharedPtr<ndModel> m_artModelSharedPtr; // keeps model alive after AddModel()
        SpiderModelNotify* m_notify{nullptr};     // owned by ndSharedPtr inside m_artModel

        struct LegWrappers
        {
            Ogre::SceneNode* thighNode{nullptr};
            Ogre::SceneNode* calfNode{nullptr};
            Ogre::SceneNode* heelNode{nullptr};
        };
        std::array<LegWrappers, SpiderArticulation::MAX_LEGS> m_legWrappers{};

        SpiderCallback* m_callback{nullptr};
        SpiderMovementManipulation m_manip;

        // Spawn matrix computed once in constructor, used by addLegChain
        ndMatrix m_spawnMatrix;

        OgreNewt::CollisionPtr m_pendingCollision;
        float m_pendingMass{20.0f};
        Ogre::Vector3 m_pendingMassOrigin{Ogre::Vector3::ZERO};
        Ogre::Vector3 m_pendingGravity{Ogre::Vector3(0.0f, -9.8f, 0.0f)};
        Ogre::Vector3 m_pendingPosition{Ogre::Vector3::ZERO};
        Ogre::Quaternion m_pendingOrientation{Ogre::Quaternion::IDENTITY};

        bool m_finalized{false};
    };

} // namespace OgreNewt
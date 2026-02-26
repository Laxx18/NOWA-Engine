/*
    OgreNewt_SpiderBody.h

    Articulated spider for OgreNewt4 / NOWA-Engine.

    Structure:
      SpiderArticulation  : ndModelArticulation  — pure body/joint tree
      SpiderModelNotify   : ndModelNotify         — per-step gait + IK logic
      SpiderBody                                  — OgreNewt wrapper, public API

    Tree per leg (AddLimb chain):
      torso → thigh  (ndIkJointSpherical, hip)
      thigh → calf   (ndIkJointHinge, knee ±limits)
      calf  → heel   (ndJointHinge, ankle spring-damper)
      heel  → contact(ndJointSlider, foot spring-damper)

    Close loops (AddCloseLoop):
      4x ndIkSwivelPositionEffector  (one per leg, gait drives these)
      1x ndJointUpVector             (keeps torso upright)
*/
#pragma once

#include "OgreNewt_Body.h"
#include "OgreNewt_BodyNotify.h"
#include "OgreNewt_Prerequisites.h"
#include "OgreNewt_World.h"

#include "ndNewton.h"

#include <OgreSceneNode.h>
#include <array>
#include <atomic>

namespace OgreNewt
{

    // ─────────────────────────────────────────────────────────────────────────────
    //  Constants
    // ─────────────────────────────────────────────────────────────────────────────
    namespace SpiderConstants
    {
        constexpr ndFloat32 TORSO_MASS = 20.0f;
        constexpr ndFloat32 TORSO_SIZE_X = 0.45f;
        constexpr ndFloat32 TORSO_SIZE_Y = 0.10f;
        constexpr ndFloat32 TORSO_SIZE_Z = 0.25f;

        constexpr ndFloat32 THIGH_MASS = 1.0f;
        constexpr ndFloat32 CALF_MASS = 1.0f;
        constexpr ndFloat32 HEEL_MASS = 0.5f;
        constexpr ndFloat32 CONTACT_MASS = 0.2f;

        constexpr ndFloat32 THIGH_RADIUS = 0.025f;
        constexpr ndFloat32 THIGH_LENGTH = 0.20f;

        constexpr ndFloat32 CALF_RADIUS = 0.025f;
        constexpr ndFloat32 CALF_LENGTH = 0.30f;

        constexpr ndFloat32 HEEL_RADIUS = 0.020f;
        constexpr ndFloat32 HEEL_LENGTH = 0.15f;

        constexpr ndFloat32 CONTACT_RADIUS = 0.030f;
        constexpr ndFloat32 CONTACT_LENGTH = 0.05f;

        // Hip attachment offsets (torso-local)
        // Leg order: 0=front-right  1=front-left  2=rear-right  3=rear-left
        constexpr ndFloat32 LEG_OFFSET_X_FRONT = 0.15f;
        constexpr ndFloat32 LEG_OFFSET_X_REAR = -0.15f;
        constexpr ndFloat32 LEG_OFFSET_Z_RIGHT = 0.10f;
        constexpr ndFloat32 LEG_OFFSET_Z_LEFT = -0.10f;
        constexpr ndFloat32 LEG_OFFSET_Y = -0.05f;

        constexpr ndFloat32 CALF_MIN_ANGLE = -80.0f * ndDegreeToRad;
        constexpr ndFloat32 CALF_MAX_ANGLE = 60.0f * ndDegreeToRad;

        // SetAsSpringDamper(regularizer, spring, damper)
        constexpr ndFloat32 HEEL_REGULARIZER = 0.9f;
        constexpr ndFloat32 HEEL_SPRING = 200.0f;
        constexpr ndFloat32 HEEL_DAMPER = 15.0f;

        constexpr ndFloat32 CONTACT_REGULARIZER = 0.9f;
        constexpr ndFloat32 CONTACT_SPRING = 400.0f;
        constexpr ndFloat32 CONTACT_DAMPER = 25.0f;

        constexpr ndFloat32 WALK_DURATION = 2.0f;
        constexpr ndFloat32 WALK_STRIDE = 0.40f;
        constexpr ndFloat32 STRIDE_HEIGHT = 0.20f;
        constexpr ndFloat32 PHASE_SHIFT = 0.25f;
    }

    // ─────────────────────────────────────────────────────────────────────────────
    //  Gait state per leg
    // ─────────────────────────────────────────────────────────────────────────────
    enum class LegPhase
    {
        onGround,
        groundToAir,
        onAir,
        airToGround
    };

    struct LegGaitState
    {
        LegPhase phase{LegPhase::onGround};
        ndFloat32 cycleTime{0.0f};
        ndVector restPos{ndVector::m_zero};
        ndVector liftPos{ndVector::m_zero};
        ndVector landPos{ndVector::m_zero};
        ndFloat32 arcA0{0.0f}, arcA1{0.0f}, arcA2{0.0f};
    };

    // ─────────────────────────────────────────────────────────────────────────────
    //  SpiderArticulation — pure tree structure, no per-step logic
    // ─────────────────────────────────────────────────────────────────────────────
    class SpiderArticulation : public ndModelArticulation
    {
    public:
        SpiderArticulation() = default;
        ~SpiderArticulation() override = default;

        // Raw pointers into ndSharedPtr-owned bodies (set by SpiderBody builder)
        ndBodyDynamic* torsoNd{nullptr};
        std::array<ndBodyDynamic*, 4> thighNd{};
        std::array<ndBodyDynamic*, 4> calfNd{};
        std::array<ndBodyDynamic*, 4> heelNd{};
        std::array<ndBodyDynamic*, 4> contactNd{};

        // Raw joint pointers for runtime queries
        std::array<ndIkSwivelPositionEffector*, 4> effectors{};
        std::array<ndIkJointHinge*, 4> kneeJoints{};
        std::array<ndJointHinge*, 4> heelJoints{};

        // Gait state and defaults — written and read by SpiderModelNotify only
        std::array<LegGaitState, 4> legGait{};
        std::array<ndVector, 4> defaultFootLocal{};
    };

    // ─────────────────────────────────────────────────────────────────────────────
    //  SpiderModelNotify — gait + IK, runs on physics thread each step
    // ─────────────────────────────────────────────────────────────────────────────
    class SpiderModelNotify : public ndModelNotify
    {
    public:
        explicit SpiderModelNotify(SpiderArticulation* model);
        ~SpiderModelNotify() override = default;

        // Called by Newton every physics step
        void Update(ndFloat32 timestep) override;

        // Safe to call from game/Lua thread
        void setStride(ndFloat32 s)
        {
            m_stride.store(s, std::memory_order_relaxed);
        }
        void setOmega(ndFloat32 o)
        {
            m_omega.store(o, std::memory_order_relaxed);
        }
        void setSwingAmplitude(ndFloat32 a)
        {
            m_swingAmplitude = a;
        }

    private:
        void updateGait(ndFloat32 timestep);
        void applyEffectors();
        ndVector computeFootTarget(int leg) const;

        SpiderArticulation* m_model{nullptr};
        std::atomic<ndFloat32> m_stride{0.0f};
        std::atomic<ndFloat32> m_omega{0.0f};
        ndFloat32 m_swingAmplitude{SpiderConstants::STRIDE_HEIGHT};
    };

    // ─────────────────────────────────────────────────────────────────────────────
    //  SpiderBody — public OgreNewt wrapper
    // ─────────────────────────────────────────────────────────────────────────────
    class _OgreNewtExport SpiderBody
    {
    public:
        explicit SpiderBody(OgreNewt::World* world);
        ~SpiderBody();

        //! Build the articulated model and add it to the physics world.
        //!
        //! legNodes[leg*3+0]=thigh  [leg*3+1]=calf  [leg*3+2]=heel
        //! Leg order: 0=front-right  1=front-left  2=rear-right  3=rear-left
        void create(Ogre::SceneManager* sceneManager, Ogre::SceneNode* torsoNode, const std::array<Ogre::SceneNode*, 12>& legNodes, const ndMatrix& spawnMatrix);

        void setStride(float stride);
        void setOmega(float omega);

        //! Teleport entire spider maintaining relative leg poses
        void setPosition(const Ogre::Vector3& pos);

        OgreNewt::Body* getTorsoBody() const
        {
            return m_torsoBody;
        }
        bool isCreated() const
        {
            return m_created;
        }

    private:
        void buildTorso(const ndMatrix& spawn, Ogre::SceneManager* sceneManager, Ogre::SceneNode* torsoNode);
        void buildLeg(int legIndex, Ogre::SceneManager* sceneManager, Ogre::SceneNode* thighNode, Ogre::SceneNode* calfNode, Ogre::SceneNode* heelNode);

        OgreNewt::World* m_world{nullptr};
        SpiderArticulation* m_model{nullptr}; // owned by ndWorld after AddModel()
        SpiderModelNotify* m_notify{nullptr}; // owned by ndSharedPtr on model
        OgreNewt::Body* m_torsoBody{nullptr};

        struct LegWrappers
        {
            OgreNewt::Body* thigh{nullptr};
            OgreNewt::Body* calf{nullptr};
            OgreNewt::Body* heel{nullptr};
            OgreNewt::Body* contact{nullptr};
        };
        std::array<LegWrappers, 4> m_legWrappers{};

        bool m_created{false};
    };

} // namespace OgreNewt
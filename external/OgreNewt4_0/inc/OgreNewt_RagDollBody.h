#pragma once

#ifndef OGRENEWT_RAGDOLLBODY_H
#define OGRENEWT_RAGDOLLBODY_H

#include "OgreNewt_Prerequisites.h"
#include "OgreNewt_Body.h"
#include "OgreNewt_World.h"
#include "OgreNewt_Collision.h"
#include "OgreNewt_CollisionPrimitives.h"
#include "OgreNewt_BasicJoints.h"

#include <OgreItem.h>
#include <Animation/OgreSkeletonInstance.h>
#include <Animation/OgreBone.h>

#include <unordered_map>
#include <vector>
#include <string>

namespace OgreNewt
{
    /**
     * @brief RagDollBody - An articulated ragdoll built on Newton Dynamics 4's ndModelArticulation.
     *
     * Derives from OgreNewt::Body. The base class IS the root body (Hips).
     * In INACTIVE state, the root body exists as a normal physics body.
     * In RAGDOLLING state, child bodies are created and linked via ndModelArticulation.
     *
     * Each bone body is a full OgreNewt::Body with BodyNotify, force callbacks, etc.
     * The caller (PhysicsRagDollComponentV3) sets the custom force callback on each bone body.
     *
     * Usage:
     *   RagDollBody::RagConfig config = ...; // filled from .rag XML
     *   auto ragdoll = new OgreNewt::RagDollBody(world, sceneManager, rootCollision, config, item, sceneNode);
     *   // At this point, ragdoll IS-A Body with a real m_body (root collision). Works for setMaterialGroupID etc.
     *   ragdoll->startRagdolling();
     *   // each frame on render thread:
     *   ragdoll->applyRagdollStateToModel();
     *   // to stop:
     *   ragdoll->endRagdolling();
     */
    class _OgreNewtExport RagDollBody : public Body
    {
    public:
        // ── Config structs (filled externally, e.g. by NOWA from .rag XML) ──

        struct BoneDef
        {
            Ogre::String name;
            Ogre::String skeletonBone;
            Ogre::String shape;          // "Capsule", "Ellipsoid", "Box", "Hull", "Cylinder", "Cone"
            Ogre::Vector3 size;
            Ogre::Real mass;

            BoneDef()
                : size(Ogre::Vector3::UNIT_SCALE)
                , mass(1.0f)
            {
            }
        };

        struct JointDef
        {
            Ogre::String type;           // "JointHingeComponent", "JointBallAndSocketComponent"
            Ogre::String childName;
            Ogre::String parentName;
            Ogre::Real friction;
            Ogre::Real minTwistAngle;    // degrees
            Ogre::Real maxTwistAngle;
            Ogre::Real maxConeAngle;     // degrees (ball-and-socket)
            Ogre::Vector3 pin;           // hinge axis

            JointDef()
                : friction(0.0f)
                , minTwistAngle(0.0f)
                , maxTwistAngle(0.0f)
                , maxConeAngle(0.0f)
                , pin(Ogre::Vector3::ZERO)
            {
            }
        };

        struct RagConfig
        {
            Ogre::Vector3 orientationOffset;  // degrees
            Ogre::Vector3 positionOffset;
            std::vector<BoneDef> bones;
            std::vector<JointDef> joints;

            RagConfig()
                : orientationOffset(Ogre::Vector3::ZERO)
                , positionOffset(Ogre::Vector3::ZERO)
            {
            }
        };

        // ── Per-bone runtime data ──

        struct RagBoneData
        {
            Ogre::String name;
            Ogre::Bone* ogreBone;
            OgreNewt::Body* body;                     // OgreNewt wrapper (set after articulation built)
            ndBodyDynamic* rawNewtonBody;              // Raw Newton body (before wrapping)
            ndModelArticulation::ndNode* articulationNode;
            OgreNewt::CollisionPtr collision;

            RagBoneData()
                : ogreBone(nullptr)
                , body(nullptr)
                , rawNewtonBody(nullptr)
                , articulationNode(nullptr)
            {
            }
        };

        enum RagDollState
        {
            INACTIVE,
            RAGDOLLING,
            PARTIAL_RAGDOLLING
        };

    public:
        /**
         * @brief Construct a RagDollBody.
         * @param world          OgreNewt world
         * @param sceneManager   Ogre scene manager
         * @param rootCollision  Collision for the root body (used in INACTIVE state as the single body)
         * @param config         Pre-parsed ragdoll configuration (bones + joints)
         * @param item           The Ogre::Item whose skeleton will be driven
         * @param sceneNode      The scene node the item is attached to
         *
         * The constructor creates a real Body with rootCollision, so m_body is valid immediately.
         * All Body methods (setMaterialGroupID, setType, etc.) work from the start.
         */
        RagDollBody(World* world,
            Ogre::SceneManager* sceneManager,
            const OgreNewt::CollisionPtr& rootCollision,
            const RagConfig& config,
            Ogre::Item* item,
            Ogre::SceneNode* sceneNode);

        virtual ~RagDollBody();

        // ── Lifecycle ──

        void startRagdolling();
        void startPartialRagdolling();
        void endRagdolling();

        /** Sync physics -> bones. Call on RENDER THREAD. */
        void applyRagdollStateToModel();

        /** Sync animation pose -> physics. Called internally by start*. */
        void applyModelStateToRagdoll();

        // ── Accessors ──

        RagDollState getState() const
        {
            return m_state;
        }
        const RagConfig& getConfig() const
        {
            return m_config;
        }
        const std::vector<RagBoneData>& getBoneData() const
        {
            return m_ragBones;
        }

        /** Get a bone's OgreNewt::Body by ragdoll name. */
        OgreNewt::Body* getBoneBody(const Ogre::String& ragBoneName) const;

        /** Get a bone's Ogre::Bone by ragdoll name. */
        Ogre::Bone* getOgreBone(const Ogre::String& ragBoneName) const;

        ndModelArticulation* getArticulation() const
        {
            return m_articulation;
        }

        // ── Constraints ──

        void setConstraintAxis(const Ogre::Vector3& axis);
        void releaseConstraintAxis();

        /** Apply velocity + angular velocity to all bone bodies (OgreNewt API). */
        void inheritVelOmega(const Ogre::Vector3& vel, const Ogre::Vector3& omega,
            const Ogre::Vector3& mainPos, size_t startIdx = 0);

    private:
        /** Build entire ragdoll: bodies + articulation + world registration.
         *  Single enqueuePhysicsAndWait call to avoid deadlock. */
        void buildRagdoll();
        void destroyRagdoll();

        OgreNewt::CollisionPtr createCollisionForBone(const BoneDef& boneDef);

        static void extractBoneDerivedTransform(Ogre::Bone* bone,
            Ogre::Vector3& outPos,
            Ogre::Quaternion& outOrient);

        RagBoneData* findRagBone(const Ogre::String& name);
        const RagBoneData* findRagBone(const Ogre::String& name) const;

    private:
        Ogre::Item* m_item;
        Ogre::SkeletonInstance* m_skeletonInstance;
        Ogre::SceneNode* m_sceneNode;

        RagConfig m_config;
        RagDollState m_state;

        std::vector<RagBoneData> m_ragBones;
        std::unordered_map<std::string, size_t> m_boneNameIndex;

        ndModelArticulation* m_articulation;
        OgreNewt::PlaneConstraint* m_planeConstraint;
    };

} // namespace OgreNewt

#endif
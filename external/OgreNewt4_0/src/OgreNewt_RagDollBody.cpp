#include "OgreNewt_Stdafx.h"
#include "OgreNewt_RagDollBody.h"
#include "OgreNewt_BodyNotify.h"
#include "OgreNewt_Collision.h"
#include "OgreNewt_CollisionPrimitives.h"
#include "OgreNewt_Tools.h"
#include "OgreNewt_World.h"

#include <OgreLogManager.h>
#include <OgreMath.h>
#include <OgreStringConverter.h>

namespace OgreNewt
{
    // ========================================================================
    // RagDollModelNotify - disables self-collision within the articulation
    // ========================================================================
    class RagDollModelNotify : public ndModelNotify
    {
    public:
        RagDollModelNotify() : ndModelNotify()
        {
        }

        bool OnContactGeneration(const ndBodyKinematic*, const ndBodyKinematic*) override
        {
            return false;
        }
    };

    // ========================================================================
    // Constructor
    //
    // Uses Body(world, sceneManager, collision) which creates a real
    // ndBodyDynamic with BodyNotify, adds to world, etc.
    // body is ALWAYS valid after construction.
    // ========================================================================

    RagDollBody::RagDollBody(World* world, Ogre::SceneManager* sceneManager, const OgreNewt::CollisionPtr& rootCollision, const RagConfig& config, Ogre::Item* item, Ogre::SceneNode* sceneNode) :
        Body(world, sceneManager, rootCollision), // Creates real body with BodyNotify!
        m_item(item),
        m_skeletonInstance(nullptr),
        m_sceneNode(sceneNode),
        m_config(config),
        m_state(INACTIVE),
        m_articulation(nullptr),
        m_planeConstraint(nullptr)
    {
        if (m_item)
        {
            m_skeletonInstance = m_item->getSkeletonInstance();
        }
    }

    RagDollBody::~RagDollBody()
    {
        if (m_state != INACTIVE)
        {
            endRagdolling();
        }
        // ~Body() handles destroying body (the root body)
    }

    // ========================================================================
    // Helpers
    // ========================================================================

    void RagDollBody::extractBoneDerivedTransform(Ogre::Bone* bone, Ogre::Vector3& outPos, Ogre::Quaternion& outOrient)
    {
        const Ogre::SimpleMatrixAf4x3& t = bone->_getLocalSpaceTransform();
        Ogre::Matrix4 mat4;
        t.store(&mat4);
        Ogre::Vector3 scale;
        mat4.decomposition(outPos, scale, outOrient);
    }

    RagDollBody::RagBoneData* RagDollBody::findRagBone(const Ogre::String& name)
    {
        auto it = m_boneNameIndex.find(std::string(name.c_str()));
        return (it != m_boneNameIndex.end()) ? &m_ragBones[it->second] : nullptr;
    }

    const RagDollBody::RagBoneData* RagDollBody::findRagBone(const Ogre::String& name) const
    {
        auto it = m_boneNameIndex.find(std::string(name.c_str()));
        return (it != m_boneNameIndex.end()) ? &m_ragBones[it->second] : nullptr;
    }

    OgreNewt::Body* RagDollBody::getBoneBody(const Ogre::String& ragBoneName) const
    {
        const RagBoneData* d = findRagBone(ragBoneName);
        // Only root (idx 0) has an OgreNewt::Body* wrapper
        return d ? d->body : nullptr;
    }

    Ogre::Bone* RagDollBody::getOgreBone(const Ogre::String& ragBoneName) const
    {
        const RagBoneData* d = findRagBone(ragBoneName);
        return d ? d->ogreBone : nullptr;
    }

    // ========================================================================
    // Collision creation from BoneDef (same mapping as V2)
    // ========================================================================

    OgreNewt::CollisionPtr RagDollBody::createCollisionForBone(const BoneDef& boneDef)
    {
        Ogre::Quaternion orientOffset = Ogre::Quaternion::IDENTITY;
        if (m_config.orientationOffset != Ogre::Vector3::ZERO)
        {
            orientOffset = Ogre::Quaternion(Ogre::Degree(m_config.orientationOffset.x), Ogre::Vector3::UNIT_X) * Ogre::Quaternion(Ogre::Degree(m_config.orientationOffset.y), Ogre::Vector3::UNIT_Y) *
                           Ogre::Quaternion(Ogre::Degree(m_config.orientationOffset.z), Ogre::Vector3::UNIT_Z);
        }

        const Ogre::Vector3& posOffset = m_config.positionOffset;
        const World* w = m_world;

        if (boneDef.shape == "Capsule")
        {
            // V2 mapping: Capsule(world, radius=size.y, height=size.x, ...)
            return OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::Capsule(w, boneDef.size.y, boneDef.size.x, 0, orientOffset, posOffset));
        }
        else if (boneDef.shape == "Ellipsoid")
        {
            return OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::Ellipsoid(w, boneDef.size, 0, orientOffset, posOffset));
        }
        else if (boneDef.shape == "Box")
        {
            return OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::Box(w, boneDef.size, 0, orientOffset, posOffset));
        }
        else if (boneDef.shape == "Cylinder")
        {
            return OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::Cylinder(w, boneDef.size.y, boneDef.size.x, 0, orientOffset, posOffset));
        }
        else if (boneDef.shape == "Cone")
        {
            return OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::Cone(w, boneDef.size.y, boneDef.size.x, 0, orientOffset, posOffset));
        }

        Ogre::LogManager::getSingleton().logMessage("[RagDollBody] Unknown shape '" + boneDef.shape + "' for '" + boneDef.name + "', using Box");
        return OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::Box(w, boneDef.size, 0, orientOffset, posOffset));
    }

    // ========================================================================
    // buildRagdoll - Creates all bodies, joints, and articulation in ONE call.
    //
    // Newton 4 lifecycle (from ndBasicRagdoll.cpp):
    //   1. Create ndBodyDynamic objects (do NOT add to world individually)
    //   2. Build ndModelArticulation: AddRootBody + AddLimb for each joint
    //   3. world->AddModel(model)
    //   4. model->AddBodiesAndJointsToWorld()
    //
    // The root body ('this'/body) is already in the world from the Body
    // constructor. We must remove it FIRST, then let the articulation re-add
    // all bodies together.
    //
    // This method does all Newton setup in a SINGLE enqueuePhysicsAndWait
    // to avoid nested waits (which deadlock if called from render thread).
    // ========================================================================

    void RagDollBody::buildRagdoll()
    {
        if (!m_skeletonInstance || m_config.bones.empty())
        {
            return;
        }

        const Ogre::Vector3 nodePos = m_sceneNode->_getDerivedPosition();
        const Ogre::Quaternion nodeOri = m_sceneNode->_getDerivedOrientation();
        const Ogre::Vector3 nodeScale = m_sceneNode->_getDerivedScale();

        m_ragBones.resize(m_config.bones.size());

        // ── Phase 1: Create all ndBodyDynamic objects (CPU-only, no world mutation) ──

        for (size_t i = 0; i < m_config.bones.size(); i++)
        {
            const BoneDef& boneDef = m_config.bones[i];
            RagBoneData& rb = m_ragBones[i];
            rb.name = boneDef.name;

            // Find Ogre bone
            if (!boneDef.skeletonBone.empty() && m_skeletonInstance->hasBone(Ogre::IdString(boneDef.skeletonBone)))
            {
                rb.ogreBone = m_skeletonInstance->getBone(Ogre::IdString(boneDef.skeletonBone));
            }
            else
            {
                Ogre::LogManager::getSingleton().logMessage("[RagDollBody] WARNING: Skeleton bone '" + boneDef.skeletonBone + "' not found for '" + boneDef.name + "'");
                continue;
            }

            Ogre::Vector3 boneSkelPos;
            Ogre::Quaternion boneSkelOri;
            extractBoneDerivedTransform(rb.ogreBone, boneSkelPos, boneSkelOri);

            Ogre::Vector3 boneWorldPos = nodePos + nodeOri * (nodeScale * boneSkelPos);
            Ogre::Quaternion boneWorldOri = nodeOri * boneSkelOri;

            ndMatrix ndMat;
            OgreNewt::Converters::QuatPosToMatrix(boneWorldOri, boneWorldPos, ndMat);

            rb.collision = createCollisionForBone(boneDef);

            if (i == 0)
            {
                // Root bone: create a NEW ndBodyDynamic for the articulation.
                // body (from Body constructor) stays as the "inactive state" body.
                // The articulation root is separate to avoid ndSharedPtr ownership conflicts.
                ndBodyDynamic* rootNd = new ndBodyDynamic();
                rootNd->SetMatrix(ndMat);

                ndShapeInstance* srcInst = rb.collision->getShapeInstance();
                if (srcInst)
                {
                    ndShapeInstance shapeInst(*srcInst);
                    rootNd->SetCollisionShape(shapeInst);
                }
                else
                {
                    ndShapeInstance shapeInst(rb.collision->getNewtonCollision());
                    rootNd->SetCollisionShape(shapeInst);
                }

                rootNd->SetMassMatrix(boneDef.mass, rootNd->GetCollisionShape());

                // Copy notify from body so force callbacks work
                ndSharedPtr<ndBodyNotify> notify(new BodyNotify(this));
                rootNd->SetNotifyCallback(notify);

                rb.rawNewtonBody = rootNd;
                rb.body = this; // for NOWA API access
            }
            else
            {
                // Child bone: new ndBodyDynamic, NOT added to world.
                ndBodyDynamic* childNd = new ndBodyDynamic();
                childNd->SetMatrix(ndMat);

                ndShapeInstance* srcInst = rb.collision->getShapeInstance();
                if (srcInst)
                {
                    ndShapeInstance shapeInst(*srcInst);
                    childNd->SetCollisionShape(shapeInst);
                }
                else
                {
                    ndShapeInstance shapeInst(rb.collision->getNewtonCollision());
                    childNd->SetCollisionShape(shapeInst);
                }

                childNd->SetMassMatrix(boneDef.mass, childNd->GetCollisionShape());

                // BodyNotify for force/transform callbacks
                ndSharedPtr<ndBodyNotify> notify(new BodyNotify(nullptr));
                childNd->SetNotifyCallback(notify);

                rb.rawNewtonBody = childNd;
                rb.body = nullptr;
            }

            m_boneNameIndex[std::string(boneDef.name.c_str())] = i;

            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[RagDollBody] Body '" + boneDef.name + "' at " + Ogre::StringConverter::toString(boneWorldPos));
        }

        // ── Phase 2: Build ndModelArticulation (CPU-only, no world mutation) ──

        m_articulation = new ndModelArticulation();

        ndSharedPtr<ndModelNotify> controller(new RagDollModelNotify());
        m_articulation->SetNotifyCallback(controller);

        // Root body
        ndSharedPtr<ndBody> rootBodyPtr(m_ragBones[0].rawNewtonBody);
        m_ragBones[0].articulationNode = m_articulation->AddRootBody(rootBodyPtr);

        // Joints
        for (const JointDef& jd : m_config.joints)
        {
            RagBoneData* childData = findRagBone(jd.childName);
            RagBoneData* parentData = findRagBone(jd.parentName);

            if (!childData || !parentData || !childData->rawNewtonBody || !parentData->rawNewtonBody)
            {
                Ogre::LogManager::getSingleton().logMessage("[RagDollBody] Joint missing bone: '" + jd.childName + "' -> '" + jd.parentName + "'");
                continue;
            }

            ndBodyKinematic* childNd = childData->rawNewtonBody;
            ndBodyKinematic* parentNd = parentData->rawNewtonBody;
            ndMatrix pivotMatrix(childNd->GetMatrix());

            ndSharedPtr<ndJointBilateralConstraint> joint;

            if (jd.type == "JointBallAndSocketComponent")
            {
                ndJointSpherical* j = new ndJointSpherical(pivotMatrix, childNd, parentNd);
                j->SetConeLimit(jd.maxConeAngle * ndDegreeToRad);
                j->SetTwistLimits(jd.minTwistAngle * ndDegreeToRad, jd.maxTwistAngle * ndDegreeToRad);
                if (jd.friction > 0.0f)
                {
                    j->SetAsSpringDamper(0.025f, jd.friction, 0.5f);
                }
                joint = ndSharedPtr<ndJointBilateralConstraint>(j);
            }
            else if (jd.type == "JointHingeComponent")
            {
                ndMatrix hingeMatrix(pivotMatrix);
                if (jd.pin != Ogre::Vector3::ZERO)
                {
                    Ogre::Vector3 pin = jd.pin;
                    pin.normalise();
                    ndVector ndPin(pin.x, pin.y, pin.z, 0.0f);
                    hingeMatrix = ndGramSchmidtMatrix(ndPin);
                    hingeMatrix.m_posit = pivotMatrix.m_posit;
                }

                ndJointHinge* j = new ndJointHinge(hingeMatrix, childNd, parentNd);
                j->SetLimitState(true);
                j->SetLimits(jd.minTwistAngle * ndDegreeToRad, jd.maxTwistAngle * ndDegreeToRad);
                if (jd.friction > 0.0f)
                {
                    j->SetAsSpringDamper(0.025f, jd.friction, 0.5f);
                }
                joint = ndSharedPtr<ndJointBilateralConstraint>(j);
            }
            else
            {
                Ogre::LogManager::getSingleton().logMessage("[RagDollBody] Unknown joint type '" + jd.type + "'");
                continue;
            }

            if (parentData->articulationNode)
            {
                ndSharedPtr<ndBody> childBodyPtr(childNd);
                childData->articulationNode = m_articulation->AddLimb(parentData->articulationNode, childBodyPtr, joint);
            }
            else
            {
                Ogre::LogManager::getSingleton().logMessage("[RagDollBody] Parent '" + jd.parentName + "' has no articulation node");
            }
        }

        // ── Phase 3: SINGLE world mutation ──
        //
        // Remove root body from world (Body constructor added it),
        // then add articulation which re-adds root + all children.

        m_world->enqueuePhysicsAndWait(
            [this](World& w)
            {
                // Freeze body (inactive-state body) so it doesn't interfere
                getNewtonBody()->SetSleepState(true);

                // Add articulation model + all bodies/joints to world
                ndWorld* ndW = w.getNewtonWorld();
                ndSharedPtr<ndModel> modelPtr(m_articulation);
                ndW->AddModel(modelPtr);

                // TODO: What is the equivalent? Look at newton demo! -> Ah its gone. See forum and spiderbody (finalizeModel).
                // m_articulation->AddBodiesAndJointsToWorld();
            });

        Ogre::LogManager::getSingleton().logMessage("[RagDollBody] Articulation created: " + Ogre::StringConverter::toString(m_ragBones.size()) + " bodies");
    }

    // ========================================================================
    // Destroy ragdoll (articulation + child bodies)
    // ========================================================================

    void RagDollBody::destroyRagdoll()
    {
        releaseConstraintAxis();

        if (m_articulation)
        {
            m_world->enqueuePhysicsAndWait(
                [this](World& w)
                {
                    // Remove all articulation bodies and joints from world
                    // TODO: What is the equivalent? Look at newton demo! -> Ah its gone. See forum and spiderbody (finalizeModel).
                    // m_articulation->RemoveBodiesAndJointsFromWorld();

                    // Wake body back up (it was frozen during ragdolling)
                    getNewtonBody()->SetSleepState(false);
                });
            m_articulation = nullptr;
        }

        // Child ndBodyDynamic* are owned by articulation's ndSharedPtr.
        // Just clear our pointers.
        for (size_t i = 1; i < m_ragBones.size(); i++)
        {
            m_ragBones[i].body = nullptr;
            m_ragBones[i].rawNewtonBody = nullptr;
            m_ragBones[i].articulationNode = nullptr;
        }

        if (!m_ragBones.empty())
        {
            m_ragBones[0].body = nullptr;
            m_ragBones[0].rawNewtonBody = nullptr;
            m_ragBones[0].articulationNode = nullptr;
        }

        m_ragBones.clear();
        m_boneNameIndex.clear();
    }

    // ========================================================================
    // Transfer animation pose -> physics bodies
    // ========================================================================

    void RagDollBody::applyModelStateToRagdoll()
    {
        const Ogre::Vector3 nodePos = m_sceneNode->_getDerivedPosition();
        const Ogre::Quaternion nodeOri = m_sceneNode->_getDerivedOrientation();
        const Ogre::Vector3 nodeScale = m_sceneNode->_getDerivedScale();

        size_t startIdx = (m_state == PARTIAL_RAGDOLLING) ? 1 : 0;

        for (size_t i = startIdx; i < m_ragBones.size(); i++)
        {
            RagBoneData& rb = m_ragBones[i];
            if (!rb.ogreBone || !rb.rawNewtonBody)
            {
                continue;
            }

            Ogre::Vector3 boneSkelPos;
            Ogre::Quaternion boneSkelOri;
            extractBoneDerivedTransform(rb.ogreBone, boneSkelPos, boneSkelOri);

            Ogre::Vector3 boneWorldPos = nodePos + nodeOri * (nodeScale * boneSkelPos);
            Ogre::Quaternion boneWorldOri = nodeOri * boneSkelOri;

            ndMatrix ndMat;
            OgreNewt::Converters::QuatPosToMatrix(boneWorldOri, boneWorldPos, ndMat);
            rb.rawNewtonBody->SetMatrix(ndMat);
        }
    }

    // ========================================================================
    // Transfer physics bodies -> skeleton bones (RENDER THREAD)
    // ========================================================================

    void RagDollBody::applyRagdollStateToModel()
    {
        if (m_ragBones.empty())
        {
            return;
        }

        size_t startIdx = (m_state == PARTIAL_RAGDOLLING) ? 1 : 0;

        const Ogre::Vector3 nodePos = m_sceneNode->_getDerivedPosition();
        const Ogre::Quaternion nodeOri = m_sceneNode->_getDerivedOrientation();
        const Ogre::Vector3 nodeScale = m_sceneNode->_getDerivedScale();
        const Ogre::Quaternion invNodeOri = nodeOri.Inverse();

        for (size_t i = startIdx; i < m_ragBones.size(); i++)
        {
            RagBoneData& rb = m_ragBones[i];
            if (!rb.ogreBone || !rb.rawNewtonBody)
            {
                continue;
            }

            Ogre::Bone* bone = rb.ogreBone;

            // Read physics transform from Newton body
            const ndMatrix& mat = rb.rawNewtonBody->GetMatrix();
            Ogre::Vector3 bodyWorldPos(mat.m_posit.m_x, mat.m_posit.m_y, mat.m_posit.m_z);
            ndQuaternion q(mat);
            Ogre::Quaternion bodyWorldOri(q.m_w, q.m_x, q.m_y, q.m_z);

            // World -> skeleton space
            Ogre::Vector3 skelPos = invNodeOri * (bodyWorldPos - nodePos);
            skelPos /= nodeScale;
            Ogre::Quaternion skelOri = invNodeOri * bodyWorldOri;

            // Skeleton-space -> parent-local (inheritOrientation=TRUE)
            Ogre::Bone* parentBone = bone->getParent();
            Ogre::Vector3 boneLocalPos;
            Ogre::Quaternion boneLocalOri;

            if (parentBone)
            {
                Ogre::Vector3 parentPos;
                Ogre::Quaternion parentOri;
                extractBoneDerivedTransform(parentBone, parentPos, parentOri);

                Ogre::Quaternion invParentOri = parentOri.Inverse();
                boneLocalPos = invParentOri * (skelPos - parentPos);
                boneLocalOri = invParentOri * skelOri;
            }
            else
            {
                boneLocalPos = skelPos;
                boneLocalOri = skelOri;
            }

            bone->setPosition(boneLocalPos);
            bone->setOrientation(boneLocalOri);
        }
    }

    // ========================================================================
    // Start ragdolling
    // ========================================================================

    void RagDollBody::startRagdolling()
    {
        if (!m_skeletonInstance || m_state != INACTIVE)
        {
            return;
        }

        m_state = RAGDOLLING;

        // Disable all animations
        Ogre::SkeletonAnimationVec& anims = m_skeletonInstance->getAnimationsNonConst();
        for (auto& a : anims)
        {
            a.setEnabled(false);
            a.mWeight = 0.0f;
            a.setTime(0.0f);
        }

        // Set all skeleton bones as manual. KEEP inheritOrientation=TRUE.
        for (size_t idx = 1; idx < m_skeletonInstance->getNumBones(); idx++)
        {
            m_skeletonInstance->setManualBone(m_skeletonInstance->getBone(idx), true);
        }

        buildRagdoll();

        Ogre::LogManager::getSingleton().logMessage("[RagDollBody] startRagdolling: " + Ogre::StringConverter::toString(m_ragBones.size()) + " bones");
    }

    void RagDollBody::startPartialRagdolling()
    {
        if (!m_skeletonInstance || m_state != INACTIVE)
        {
            return;
        }

        m_state = PARTIAL_RAGDOLLING;

        for (size_t idx = 1; idx < m_skeletonInstance->getNumBones(); idx++)
        {
            m_skeletonInstance->setManualBone(m_skeletonInstance->getBone(idx), true);
        }

        buildRagdoll();
    }

    // ========================================================================
    // End ragdolling
    // ========================================================================

    void RagDollBody::endRagdolling()
    {
        if (!m_skeletonInstance || m_state == INACTIVE)
        {
            return;
        }

        destroyRagdoll();

        for (size_t idx = 1; idx < m_skeletonInstance->getNumBones(); idx++)
        {
            m_skeletonInstance->setManualBone(m_skeletonInstance->getBone(idx), false);
        }

        m_skeletonInstance->resetToPose();

        Ogre::SkeletonAnimationVec& anims = m_skeletonInstance->getAnimationsNonConst();
        for (auto& a : anims)
        {
            a.setEnabled(false);
            a.mWeight = 0.0f;
            a.setTime(0.0f);
        }

        m_state = INACTIVE;
        Ogre::LogManager::getSingleton().logMessage("[RagDollBody] endRagdolling");
    }

    // ========================================================================
    // Constraint helpers
    // ========================================================================

    void RagDollBody::setConstraintAxis(const Ogre::Vector3& axis)
    {
        releaseConstraintAxis();

        if (axis == Ogre::Vector3::ZERO || !getNewtonBody())
        {
            return;
        }

        m_planeConstraint = new OgreNewt::PlaneConstraint(this, nullptr, getPosition(), axis);
    }

    void RagDollBody::releaseConstraintAxis()
    {
        if (m_planeConstraint)
        {
            m_planeConstraint->destroyJoint(m_world);
            delete m_planeConstraint;
            m_planeConstraint = nullptr;
        }
    }

    // ========================================================================
    // inheritVelOmega - set velocity on all ragdoll bodies
    // ========================================================================

    void RagDollBody::inheritVelOmega(const Ogre::Vector3& vel, const Ogre::Vector3& omega, const Ogre::Vector3& mainPos, size_t startIdx)
    {
        for (size_t i = startIdx; i < m_ragBones.size(); i++)
        {
            if (!m_ragBones[i].rawNewtonBody)
            {
                continue;
            }

            const ndMatrix& mat = m_ragBones[i].rawNewtonBody->GetMatrix();
            Ogre::Vector3 pos(mat.m_posit.m_x, mat.m_posit.m_y, mat.m_posit.m_z);
            Ogre::Vector3 v = vel + omega.crossProduct(pos - mainPos);

            ndVector ndVel(v.x, v.y, v.z, 0.0f);
            m_ragBones[i].rawNewtonBody->SetVelocity(ndVel);
        }
    }

} // namespace OgreNewt
/*
    OgreNewt_SpiderBody.cpp

    Faithful port of Newton Dynamics 4  ndQuadSpiderPlayer demo.

    Key corrections vs old version:
      1. ndIkSwivelPositionEffector constructor fixed:
             (pinAndPivotParentInGlobalSpace, parentBody=torso,
              childPivotInGlobalSpace, childBody=heel)
         where pinAndPivot = torso orientation + hip socket world pos
         and   childPivot  = foot/contact world pos.

      2. Gait algorithm replaced with faithful port of ndProdeduralGaitGenerator:
         - global timeAcc advanced via angular-wrap (matches Newton's CalculateTime)
         - per-leg phase offset: fmod(timeAcc + seq[i]*0.25*dur, dur)
         - state machine: groundToAir / onAir / airToGround / onGround
         - all positions in effector LOCAL space (hip-pivot frame)
         - parabola coefficients computed exactly as Newton does

      3. Body sway mirrors ndBodySwingControl:
         localMat.TransformVector(posit) → sway → UntransformVector

      4. Knee safety guard: SetAsReducedDof() when near limits.

      5. Spring/damper values from Newton's demo:
           heel   regularizer=0.001, spring=2000, damper=50
           contact regularizer=0.002, spring=2000, damper=100
           effector linear/angular spring=4000 damper=50

      6. Effector settings: SetWorkSpaceConstraints, SetMaxForce, SetMaxTorque.

      7. AddCloseLoop() called without extra string argument.
*/

#include "OgreNewt_Stdafx.h"
#include "OgreNewt_SpiderBody.h"
#include "OgreNewt_Tools.h"
#include "OgreNewt_World.h"
#include <OgreQuaternion.h>
#include <OgreSceneNode.h>

using namespace OgreNewt;

// ─────────────────────────────────────────────────────────────────────────────
//  Internal helpers
// ─────────────────────────────────────────────────────────────────────────────
namespace
{

    // Build a pin matrix: X column = direction, position = pivot
    static ndMatrix pinMatrix(const ndVector& pivot, const ndVector& dir)
    {
        ndMatrix m = ndGramSchmidtMatrix(dir);
        m.m_posit = pivot;
        m.m_posit.m_w = 1.0f;
        return m;
    }

    // Build a capsule body: long axis = dir, centre = pos
    static ndBodyDynamic* makeCapsule(ndFloat32 radius, ndFloat32 length, ndFloat32 mass, const ndVector& centre, const ndVector& dir)
    {
        ndMatrix mat = ndGramSchmidtMatrix(dir);
        mat.m_posit = centre;
        mat.m_posit.m_w = 1.0f;

        ndShapeInstance inst(new ndShapeCapsule(radius, radius, length));
        auto* body = new ndBodyDynamic();
        body->SetCollisionShape(inst);
        body->SetMassMatrix(mass, inst); // engine computes inertia from shape
        body->SetMatrix(mat);
        return body;
    }

    // Convert ndMatrix to Ogre position + orientation
    static void ndMatrixToOgre(const ndMatrix& m, Ogre::Vector3& pos, Ogre::Quaternion& orient)
    {
        pos = Ogre::Vector3(m.m_posit.m_x, m.m_posit.m_y, m.m_posit.m_z);

        // Build a 3×3 rotation from Newton's column vectors
        Ogre::Matrix3 rot3;
        rot3[0][0] = m.m_front.m_x;
        rot3[0][1] = m.m_up.m_x;
        rot3[0][2] = m.m_right.m_x;
        rot3[1][0] = m.m_front.m_y;
        rot3[1][1] = m.m_up.m_y;
        rot3[1][2] = m.m_right.m_y;
        rot3[2][0] = m.m_front.m_z;
        rot3[2][1] = m.m_up.m_z;
        rot3[2][2] = m.m_right.m_z;
        orient.FromRotationMatrix(rot3);
    }

    // Capsule dimensions (used for physics bodies; visual comes from user's meshes)
    namespace Dim
    {
        constexpr ndFloat32 TORSO_MASS = 20.0f;

        constexpr ndFloat32 THIGH_MASS = 0.25f;
        constexpr ndFloat32 CALF_MASS = 0.25f;
        constexpr ndFloat32 HEEL_MASS = 0.125f;
        constexpr ndFloat32 CONTACT_MASS = 0.125f;

        // Contact capsule geometry (small sphere-like)
        constexpr ndFloat32 CONTACT_RADIUS = 0.030f;
        constexpr ndFloat32 CONTACT_LENGTH = 0.050f;

        // Newton demo values
        constexpr ndFloat32 HEEL_REG = 0.001f;
        constexpr ndFloat32 HEEL_SPRING = 2000.0f;
        constexpr ndFloat32 HEEL_DAMPER = 50.0f;

        constexpr ndFloat32 CONTACT_REG = 0.002f;
        constexpr ndFloat32 CONTACT_SPRING = 2000.0f;
        constexpr ndFloat32 CONTACT_DAMPER = 50.0f;

        constexpr ndFloat32 EFF_REG = 0.001f;
        constexpr ndFloat32 EFF_SPRING = 1500.0f;
        constexpr ndFloat32 EFF_DAMPER = 50.0f;

        constexpr ndFloat32 GRAVITY_MAG = 9.8f; // used in MaxForce calc
    }

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
//  SpiderModelNotify
// ─────────────────────────────────────────────────────────────────────────────
SpiderModelNotify::SpiderModelNotify(SpiderArticulation* model) : ndModelNotify(), m_model(model), m_timeAcc(0.0f)
{
}

// Matches Newton's ndProdeduralGaitGenerator::CalculateTime()
// Uses angular arithmetic to wrap cleanly.
ndFloat32 SpiderModelNotify::advanceTime(ndFloat32 current, ndFloat32 timestep) const
{
    const ndFloat32 dur = m_model->duration;
    const ndFloat32 twoPi = ndFloat32(2.0f) * ndPi;

    ndFloat32 angle0 = current * twoPi / dur;
    ndFloat32 dAngle = timestep * twoPi / dur;
    ndFloat32 angle1 = ndAnglesAdd(angle0, dAngle);
    if (angle1 < 0.0f)
    {
        angle1 += twoPi;
    }
    return angle1 * dur / twoPi;
}

// Matches Newton's ndProdeduralGaitGenerator::GetState()
LegGaitPhase SpiderModelNotify::getState(int legIndex, ndFloat32 timeAcc, ndFloat32 timestep) const
{
    const ndFloat32 dur = m_model->duration;
    const ndFloat32 airEnd = dur * m_model->airFraction; // timeLine[1]
    const int seq = m_model->gaitSequence[legIndex];
    const ndFloat32 offset = ndFloat32(seq) * 0.25f * dur;

    ndFloat32 t0 = ndMod(timeAcc + offset, dur);
    ndFloat32 t1 = t0 + timestep;

    // Air phase: [0 .. airEnd)
    if (t0 <= airEnd)
    {
        if (t1 > airEnd)
        {
            return LegGaitPhase::airToGround;
        }
        return LegGaitPhase::onAir;
    }

    // Ground phase: [airEnd .. dur)
    if (t1 > dur)
    {
        return LegGaitPhase::groundToAir;
    }
    return LegGaitPhase::onGround;
}

// Matches Newton's ndProdeduralGaitGenerator::IntegrateLeg()
void SpiderModelNotify::integrateLeg(int legIndex, LegGaitPhase state, ndFloat32 stride, ndFloat32 omega, ndFloat32 timestep)
{
    LegPose& pose = m_model->legPose[legIndex];
    LegGaitPhase& phase = m_model->legPhase[legIndex];

    const ndFloat32 dur = m_model->duration;
    const ndFloat32 airEnd = dur * m_model->airFraction;
    const ndFloat32 groundDur = dur - airEnd;
    const ndFloat32 stepH = m_model->stepHeight;

    switch (state)
    {
    // ── Last ground tick: set up the upcoming AIR phase ─────────────────────
    case LegGaitPhase::groundToAir:
    {
        // Step forward: target is base + stride/2 in the local X direction
        ndVector airTarget = pose.base;
        airTarget.m_x += stride * 0.5f;

        pose.end = airTarget;
        pose.start = pose.posit;

        pose.maxTime = airEnd;
        pose.time = 0.0f;

        // Parabolic arch: y goes from start.y to end.y via a peak above midpoint
        ndFloat32 y0 = pose.start.m_y;
        ndFloat32 y1 = pose.end.m_y;
        ndFloat32 h = 0.5f * (y0 + y1) + stepH;

        pose.a0 = y0;
        pose.a1 = 4.0f * (h - y0) - (y1 - y0);
        pose.a2 = y1 - y0 - pose.a1;

        // Output stays at current ground position this tick
        phase = LegGaitPhase::onAir; // next tick will be onAir
        break;
    }

    // ── Last air tick: set up the upcoming GROUND (plant) phase ─────────────
    case LegGaitPhase::airToGround:
    {
        // Step back: target is base - stride/2, rotated by total yaw during ground phase
        ndVector groundTarget = pose.base;
        groundTarget.m_x -= stride * 0.5f;

        ndFloat32 totalAngle = omega * groundDur;
        ndMatrix yawMat(ndYawMatrix(totalAngle));
        groundTarget = yawMat.RotateVector(groundTarget);
        groundTarget.m_w = 0.0f;

        pose.start = pose.posit; // foot lands where air phase left it
        pose.end = groundTarget;
        pose.maxTime = groundDur;
        pose.time = 0.0f;

        // Output stays at current air position this tick
        phase = LegGaitPhase::onGround; // next tick will be onGround
        break;
    }

    // ── Foot is swinging: parabolic arc ─────────────────────────────────────
    case LegGaitPhase::onAir:
    {
        pose.time += timestep;
        ndFloat32 u = ndClamp(pose.time / pose.maxTime, 0.0f, 1.0f);

        ndVector step = pose.end - pose.start;
        pose.posit = pose.start + step.Scale(u);
        pose.posit.m_y = pose.a0 + pose.a1 * u + pose.a2 * u * u;
        pose.posit.m_w = 0.0f;
        break;
    }

    // ── Foot is planted: slide linearly start → end ──────────────────────────
    case LegGaitPhase::onGround:
    default:
    {
        pose.time += timestep;
        ndFloat32 u = ndClamp(pose.time / pose.maxTime, 0.0f, 1.0f);

        ndVector step = pose.end - pose.start;
        pose.posit = pose.start + step.Scale(u);
        pose.posit.m_w = 0.0f;
        break;
    }
    }
}

// Matches ndController::Update + ndBodySwingControl::Evaluate
void SpiderModelNotify::applyEffectors()
{
    if (!m_model->torsoNd)
    {
        return;
    }

    const ndFloat32 bSwayX = m_model->bodySwayX.load(std::memory_order_relaxed);
    const ndFloat32 bSwayZ = m_model->bodySwayZ.load(std::memory_order_relaxed);

    // Body sway matrix (mirrors ndBodySwingControl, no pitch/roll for now)
    // X and Z offsets are negated per Newton's convention.
    ndMatrix swayMat(ndGetIdentityMatrix());
    swayMat.m_posit.m_x = -bSwayX;
    swayMat.m_posit.m_z = -bSwayZ;

    const ndVector upVector = m_model->torsoNd->GetMatrix().m_up;

    for (int i = 0; i < m_model->activeLegCount; ++i)
    {
        ndIkSwivelPositionEffector* const eff = m_model->effectors[i];
        if (!eff)
        {
            continue;
        }

        // ── Apply body sway (mirrors ndBodySwingControl::Evaluate) ────────────
        ndMatrix localMat = eff->GetLocalMatrix1();                        // hip-pivot frame on torso
        ndVector p0 = localMat.TransformVector(m_model->legPose[i].posit); // local → torso
        ndVector p1 = swayMat.TransformVector(p0);                         // apply sway
        ndVector p2 = localMat.UntransformVector(p1);                      // torso → local
        p2.m_w = 0.0f;

        // ── Swivel angle: knee points away from body (matches CalculateLookAtSwivelAngle) ─
        ndFloat32 swivelAngle = eff->CalculateLookAtSwivelAngle(upVector);
        eff->SetSwivelAngle(swivelAngle);

        // ── Knee safety guard (matches ndController::Update) ─────────────────
        ndIkJointHinge* const knee = m_model->kneeJoints[i];
        if (knee)
        {
            ndFloat32 minAngle, maxAngle;
            knee->GetLimits(minAngle, maxAngle);
            ndFloat32 safeGuard = 3.0f * ndDegreeToRad;
            maxAngle = ndMax(0.0f, maxAngle - safeGuard);
            minAngle = ndMin(0.0f, minAngle + safeGuard);

            ndFloat32 kneeAngle = knee->GetAngle();
            if (kneeAngle > maxAngle || kneeAngle < minAngle)
            {
                eff->SetAsReducedDof();
            }

            // ── Heel coupling: heel counter-rotates with knee ─────────────────
            ndJointHinge* const heel = m_model->heelJoints[i];
            if (heel)
            {
                heel->SetTargetAngle(-kneeAngle * 0.5f);
            }
        }

        // ── Push target to effector ───────────────────────────────────────────
        eff->SetLocalTargetPosition(p2);
    }
}

void SpiderModelNotify::Update(ndFloat32 timestep)
{
    if (!m_model || !m_model->torsoNd)
    {
        return;
    }

    // Keep torso awake
    m_model->torsoNd->SetSleepState(false);

    if (!m_model->canMove.load(std::memory_order_relaxed))
    {
        return;
    }

    const ndFloat32 stride = m_model->stride.load(std::memory_order_relaxed);
    const ndFloat32 omega = m_model->omega.load(std::memory_order_relaxed);

    // ── Advance gait clock (matches ndProdeduralGaitGenerator::CalculatePose) ─
    const ndFloat32 nextTime = advanceTime(m_timeAcc, timestep);

    for (int i = 0; i < m_model->activeLegCount; ++i)
    {
        LegGaitPhase state = getState(i, m_timeAcc, timestep);
        integrateLeg(i, state, stride, omega, timestep);
    }

    m_timeAcc = nextTime;

    // ── Drive effectors ───────────────────────────────────────────────────────
    applyEffectors();
}

void SpiderModelNotify::PostTransformUpdate(ndFloat32 /*timestep*/)
{
    // Capture leg body transforms into the thread-safe cache so the
    // main thread can push them to GraphicsModule.
    std::lock_guard<std::mutex> lock(m_model->transformMutex);

    for (int i = 0; i < m_model->activeLegCount; ++i)
    {
        LegTransformCache& tc = m_model->cachedTransforms[i];
        tc.valid = true;

        if (m_model->thighNd[i])
        {
            ndMatrixToOgre(m_model->thighNd[i]->GetMatrix(), tc.thighPos, tc.thighOrient);
        }
        if (m_model->calfNd[i])
        {
            ndMatrixToOgre(m_model->calfNd[i]->GetMatrix(), tc.calfPos, tc.calfOrient);
        }
        if (m_model->heelNd[i])
        {
            ndMatrixToOgre(m_model->heelNd[i]->GetMatrix(), tc.heelPos, tc.heelOrient);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  SpiderBody
// ─────────────────────────────────────────────────────────────────────────────
SpiderBody::SpiderBody(OgreNewt::World* world, Ogre::SceneManager* sceneManager, const OgreNewt::CollisionPtr& collision, float mass, const Ogre::Vector3& massOrigin, const Ogre::Vector3& gravity, const Ogre::Vector3& defaultDirection,
    const Ogre::Vector3& initialPosition, const Ogre::Quaternion& initialOrientation, SpiderCallback * callback) :
    OgreNewt::Body(world, sceneManager),
    m_callback(callback),
    m_pendingCollision(collision),
    m_pendingMass(mass),
    m_pendingMassOrigin(massOrigin),
    m_pendingGravity(gravity),
    m_pendingPosition(initialPosition),
    m_pendingOrientation(initialOrientation)
{
    m_artModel = new SpiderArticulation();

    // ── Create torso ndBodyDynamic NOW so addLegChain() can use it ─────────────
    auto* torsoNd = new ndBodyDynamic();

    ndShapeInstance* srcInst = m_pendingCollision->getShapeInstance();
    if (srcInst)
    {
        torsoNd->SetCollisionShape(ndShapeInstance(*srcInst));
    }
    else
    {
        torsoNd->SetCollisionShape(ndShapeInstance(m_pendingCollision->getNewtonCollision()));
    }

    // Recompute mass from new shape
    ndShapeInstance& inst = torsoNd->GetCollisionShape();
    torsoNd->SetMassMatrix(ndFloat32(m_pendingMass), inst);

    // Apply center of mass offset properly
    if (m_pendingMassOrigin != Ogre::Vector3::ZERO)
    {
        ndVector com(m_pendingMassOrigin.x, m_pendingMassOrigin.y, m_pendingMassOrigin.z, 0.0f);

        torsoNd->SetCentreOfMass(com);
    }

    // Build spawn matrix from initial position/orientation
    ndMatrix spawnMatrix(ndGetIdentityMatrix());
    OgreNewt::Converters::QuatPosToMatrix(Ogre::Quaternion(m_pendingOrientation.w, m_pendingOrientation.x, m_pendingOrientation.y, m_pendingOrientation.z), Ogre::Vector3(m_pendingPosition.x, m_pendingPosition.y, m_pendingPosition.z), spawnMatrix);
    torsoNd->SetMatrix(spawnMatrix);

    // Store in articulation — addLegChain reads this directly
    m_artModel->torsoNd = torsoNd;

    // Register as articulation root — required before any AddLimb call
    m_artModel->AddRootBody(ndSharedPtr<ndBody>(torsoNd));

    // m_body stays nullptr until finalizeModel() — base class destructor safe
}

SpiderBody::~SpiderBody()
{
    clearLegs();
    // m_artModel is owned by ndWorld after finalizeModel(); do NOT delete here.
    // If finalizeModel() was never called, we own it.
    if (!m_finalized)
    {
        delete m_artModel;
    }
    delete m_callback;
}

bool SpiderBody::isOnGround() const
{
    if (!m_artModel || m_artModel->activeLegCount == 0)
    {
        return false;
    }
    for (int i = 0; i < m_artModel->activeLegCount; ++i)
    {
        const LegGaitPhase phase = m_artModel->legPhase[i];
        if (phase == LegGaitPhase::onGround || phase == LegGaitPhase::airToGround)
        {
            return true;
        }
    }
    return false;
}

ndBodyDynamic* SpiderBody::getTorsoNd() const
{
    // m_body is set in finalizeModel(); safe to cast since we created it as ndBodyDynamic
    return static_cast<ndBodyDynamic*>(m_body);
}

void SpiderBody::addLegChain(Ogre::SceneNode* thighNode, Ogre::SceneNode* calfNode, Ogre::SceneNode* heelNode, const Ogre::Vector3& hipLocalOgre, const Ogre::Vector3& footRestLocalOgre, float thighLen, float calfLen, float swivelSign,
    int /*boneAxis*/) // boneAxis affects visual only, not physics
{
    if (m_finalized)
    {
        return;
    }

    // Use m_artModel->torsoNd directly — always valid after constructor
    ndBodyDynamic* const torsoNd = m_artModel->torsoNd;
    if (!torsoNd)
    {
        return;
    }

    const int legIndex = m_artModel->activeLegCount;
    if (legIndex >= SpiderArticulation::MAX_LEGS)
    {
        return;
    }

    const ndMatrix torsoMat = torsoNd->GetMatrix();

    // ── Convert torso-local positions to world space ──────────────────────────
    ndVector hipLocal(hipLocalOgre.x, hipLocalOgre.y, hipLocalOgre.z, 1.0f);
    ndVector footRestLocal(footRestLocalOgre.x, footRestLocalOgre.y, footRestLocalOgre.z, 1.0f);

    ndVector hipWorld = torsoMat.TransformVector(hipLocal);
    hipWorld.m_w = 1.0f;

    // Compute knee and ankle world positions from hip→foot direction.
    // The direction is from hip toward the foot rest position (projected to world space).
    ndVector footRestWorld = torsoMat.TransformVector(footRestLocal);
    footRestWorld.m_w = 0.0f;

    ndVector hipToFoot = footRestWorld - hipWorld;
    hipToFoot.m_w = 0.0f;
    ndFloat32 totalLen = hipToFoot.DotProduct(hipToFoot).GetScalar();
    if (totalLen < 1e-6f)
    {
        return;
    }
    totalLen = ndSqrt(totalLen);

    // Split direction: thigh goes along hip→foot for thighLen, then calf continues
    ndVector legDir = hipToFoot.Scale(ndFloat32(1.0f) / totalLen);

    ndVector kneeWorld = hipWorld + legDir.Scale(thighLen);
    ndVector ankleWorld = kneeWorld + legDir.Scale(calfLen);
    kneeWorld.m_w = 1.0f;
    ankleWorld.m_w = 1.0f;

    // Use upward direction, ndGramSchmidtMatrix handles it correctly
    ndVector heelDir(0.0f, -1.0f, 0.0f, 0.0f);  // keep for position math
    ndVector heelDirUp(0.0f, 1.0f, 0.0f, 0.0f); // use this for matrix construction

    ndVector footWorld = ankleWorld + heelDir.Scale(Dim::CONTACT_LENGTH * 0.5f);
    footWorld.m_w = 1.0f;

    ndVector thighCentre = hipWorld + legDir.Scale(thighLen * 0.5f);
    ndVector calfCentre = kneeWorld + legDir.Scale(calfLen * 0.5f);
    ndVector heelCentre = ankleWorld + heelDir.Scale(Dim::CONTACT_LENGTH * 0.25f);
    ndVector contactCentre = ankleWorld + heelDir.Scale(Dim::CONTACT_LENGTH * 0.75f);

    thighCentre.m_w = calfCentre.m_w = heelCentre.m_w = contactCentre.m_w = 1.0f;

    // Capsule radii derived from bone lengths (thin limbs)
    const ndFloat32 thighRadius = ndMax(thighLen * 0.08f, 0.015f);
    const ndFloat32 calfRadius = ndMax(calfLen * 0.07f, 0.012f);
    const ndFloat32 heelRadius = Dim::CONTACT_RADIUS;

    // ── Create physics bodies ─────────────────────────────────────────────────
    auto* thighNd = makeCapsule(thighRadius, thighLen, Dim::THIGH_MASS, thighCentre, legDir);
    auto* calfNd = makeCapsule(calfRadius, calfLen, Dim::CALF_MASS, calfCentre, legDir);
    auto* heelNd = makeCapsule(heelRadius, Dim::CONTACT_LENGTH, Dim::HEEL_MASS, heelCentre, heelDirUp);
    auto* contactNd = makeCapsule(heelRadius, Dim::CONTACT_LENGTH, Dim::CONTACT_MASS, contactCentre, heelDirUp);

    m_artModel->thighNd[legIndex] = thighNd;
    m_artModel->calfNd[legIndex] = calfNd;
    m_artModel->heelNd[legIndex] = heelNd;
    m_artModel->contactNd[legIndex] = contactNd;

    // ── Hinge axis = torso's lateral direction (Z in world frame at spawn) ─────
    ndVector hingeAxis = torsoMat.m_front; // Newton convention: m_front = X forward

    // ── Hip spherical joint (ball socket) — matches ndJointSpherical in Newton demo ──
    ndMatrix hipFrame = ndGetIdentityMatrix();
    hipFrame.m_posit = hipWorld;
    auto hipJoint = ndSharedPtr<ndJointBilateralConstraint>(new ndJointSpherical(hipFrame, thighNd, torsoNd));

    // ── Knee IK hinge — with limits ───────────────────────────────────────────
    auto kneeJoint = ndSharedPtr<ndJointBilateralConstraint>(new ndIkJointHinge(pinMatrix(kneeWorld, hingeAxis), calfNd, thighNd));
    auto* kneeRaw = static_cast<ndIkJointHinge*>(kneeJoint.operator->());
    kneeRaw->SetLimitState(true);
    kneeRaw->SetLimits(-80.0f * ndDegreeToRad, 60.0f * ndDegreeToRad);
    m_artModel->kneeJoints[legIndex] = kneeRaw;

    // ── Ankle hinge — spring-damper (matches Newton demo exactly) ─────────────
    auto heelJoint = ndSharedPtr<ndJointBilateralConstraint>(new ndJointHinge(pinMatrix(ankleWorld, hingeAxis), heelNd, calfNd));
    auto* heelRaw = static_cast<ndJointHinge*>(heelJoint.operator->());
    heelRaw->SetAsSpringDamper(Dim::HEEL_REG, Dim::HEEL_SPRING, Dim::HEEL_DAMPER);
    m_artModel->heelJoints[legIndex] = heelRaw;

    // ── Contact slider — spring-damper ────────────────────────────────────────
    auto contactJoint = ndSharedPtr<ndJointBilateralConstraint>(new ndJointSlider(pinMatrix(footWorld, heelDirUp), contactNd, heelNd));
    auto* contactRaw = static_cast<ndJointSlider*>(contactJoint.operator->());
    contactRaw->SetLimitState(true);
    contactRaw->SetLimits(-0.05f, 0.05f);
    contactRaw->SetAsSpringDamper(Dim::CONTACT_REG, Dim::CONTACT_SPRING, Dim::CONTACT_DAMPER);

    // ── Build articulation tree ───────────────────────────────────────────────
    // Root is already set; legs hang off it.
    ndModelArticulation::ndNode* const rootNode = m_artModel->GetRoot();
    auto* thighNode_ = m_artModel->AddLimb(rootNode, ndSharedPtr<ndBody>(thighNd), hipJoint);
    auto* calfNode_ = m_artModel->AddLimb(thighNode_, ndSharedPtr<ndBody>(calfNd), kneeJoint);
    auto* heelNode_ = m_artModel->AddLimb(calfNode_, ndSharedPtr<ndBody>(heelNd), heelJoint);
    (void)m_artModel->AddLimb(heelNode_, ndSharedPtr<ndBody>(contactNd), contactJoint);

    // ── IK effector (close loop) — CORRECT CONSTRUCTOR ────────────────────────
    //
    //  ndIkSwivelPositionEffector(
    //      pinAndPivotParentInGlobalSpace,  <- torso orientation + hip socket world pos
    //      parentBody (torso),
    //      childPivotInGlobalSpace,         <- foot/contact point world pos
    //      childBody (heel))
    //
    //  Mirrors Newton's demo:
    //      effectPivot = rootBody->GetMatrix();
    //      effectPivot.m_posit = thighMatrix.m_posit;   // hip socket in world
    //      effectOffset = footMesh->CalculateGlobalMatrix().m_posit;  // foot world pos
    //      new ndIkSwivelPositionEffector(effectPivot, rootBody, effectOffset, heelBody)

    ndMatrix effectPivot = torsoMat;
    effectPivot.m_posit = hipWorld;
    effectPivot.m_posit.m_w = 1.0f;

    // The foot pivot on the child (heel body) is the bottom of the contact area
    ndVector effectorChildPivot = ankleWorld; // ankle / end-of-heel world pos
    effectorChildPivot.m_w = 1.0f;

    auto effJoint = ndSharedPtr<ndJointBilateralConstraint>(new ndIkSwivelPositionEffector(effectPivot, // parent pivot frame (world): torso orient + hip pos
        torsoNd,                                                                                        // parent body
        effectorChildPivot,                                                                             // child pivot (world): foot position
        heelNd));                                                                                       // child body (heel, NOT contact)

    auto* effRaw = static_cast<ndIkSwivelPositionEffector*>(effJoint.operator->());

    // Match Newton's demo effector settings exactly
    effRaw->SetLinearSpringDamper(Dim::EFF_REG, Dim::EFF_SPRING, Dim::EFF_DAMPER);
    effRaw->SetAngularSpringDamper(Dim::EFF_REG, Dim::EFF_SPRING, Dim::EFF_DAMPER);
    // And the workspace minimum of 0.0f causes singularity when the foot is near the hip:
    effRaw->SetWorkSpaceConstraints(0.05f, (thighLen + calfLen) * 0.9f);

    // was: ndAbs(500.0f * Dim::THIGH_MASS * Dim::GRAVITY_MAG)  →  ~1225 N
    // use torso mass to get a meaningful limit:
    const ndFloat32 effStrength = 500.0f * ndFloat32(m_pendingMass) * Dim::GRAVITY_MAG;
    // with mass=20: 500 * 20 * 9.8 = 98000 N — much more reasonable for articulation
    effRaw->SetMaxForce(effStrength);
    effRaw->SetMaxTorque(effStrength);

    m_artModel->effectors[legIndex] = effRaw;

    // AddCloseLoop — no label argument (was buggy in previous version)
    m_artModel->AddCloseLoop(effJoint);

    // ── Initialise gait state from effector's rest position ───────────────────
    // GetEffectorPosit() returns foot position in the effector's LocalMatrix1 frame
    // (hip-pivot local space). This is the "base" that all gait positions are
    // relative to — exactly as Newton does with:
    //   m_pose[i].m_base = leg.m_effector->GetEffectorPosit();
    LegPose& pose = m_artModel->legPose[legIndex];
    pose.base = effRaw->GetEffectorPosit();
    pose.posit = pose.base;
    pose.start = pose.base;
    pose.end = pose.base;
    pose.time = 0.0f;
    pose.maxTime = 1.0f;
    pose.a0 = pose.base.m_y;
    pose.a1 = 0.0f;
    pose.a2 = 0.0f;

    m_artModel->legPhase[legIndex] = LegGaitPhase::onGround;
    m_artModel->legCycleTime[legIndex] = ndFloat32(legIndex) * 0.25f * m_artModel->duration;

    m_artModel->activeLegCount = legIndex + 1;

    // ── Ogre visual wrappers ──────────────────────────────────────────────────
    // We create minimal OgreNewt::Body wrappers so the scene nodes get
    // their transforms updated via the standard BodyNotify mechanism.
    // NOTE: OgreNewt::Body constructor may add the ndBodyDynamic to the
    // Newton world automatically.  If Newton complains about double-add
    // when AddModel is called, remove each leg body from the world first:
    //   m_world->getNewtonWorld()->RemoveBody(thighNd);  etc.
    // then call world->AddModel(m_artModel) which re-adds via the articulation.

    /*auto makeWrapper = [this](ndBodyDynamic* nd, Ogre::SceneNode* node) -> OgreNewt::Body*
    {
        if (!node)
        {
            return nullptr;
        }
        auto* wrapper = new OgreNewt::Body(m_world, nullptr, nd);
        wrapper->setBodyNotify(new OgreNewt::BodyNotify(wrapper));
        wrapper->attachNode(node);
        return wrapper;
    };

    m_legWrappers[legIndex].thigh = makeWrapper(thighNd, thighNode);
    m_legWrappers[legIndex].calf = makeWrapper(calfNd, calfNode);
    m_legWrappers[legIndex].heel = makeWrapper(heelNd, heelNode);*/

    // At the bottom of addLegChain, just store the scene nodes for PostTransformUpdate
    m_legWrappers[legIndex].thighNode = thighNode;
    m_legWrappers[legIndex].calfNode = calfNode;
    m_legWrappers[legIndex].heelNode = heelNode;
}

void SpiderBody::finalizeModel()
{
    if (m_finalized || m_artModel->activeLegCount == 0)
    {
        return;
    }

    ndBodyDynamic* const torsoNd = m_artModel->torsoNd;

    // ── Hand torso body to base OgreNewt::Body so all property methods work ───
    m_body = torsoNd;
    m_isOwner = true;
    m_gravity = m_pendingGravity;

    // Attach BodyNotify so OnApplyExternalForce fires the force callback.
    // Do NOT call w.addBody() — AddModel handles everything.
    torsoNd->SetNotifyCallback(m_bodyNotify);

    // ── UpVector — keeps torso upright ────────────────────────────────────────
    ndBodyKinematic* sentinel = m_world->getNewtonWorld()->GetSentinelBody();
    m_artModel->AddCloseLoop(ndSharedPtr<ndJointBilateralConstraint>(new ndJointUpVector(ndVector(0.0f, 1.0f, 0.0f, 0.0f), torsoNd, sentinel)));

    // ── Create model notify ───────────────────────────────────────────────────
    m_notify = new SpiderModelNotify(m_artModel);
    m_artModel->SetNotifyCallback(ndSharedPtr<ndModelNotify>(m_notify));

    // ── Add model to world — adds ALL bodies and joints in one shot ───────────
    m_world->enqueuePhysicsAndWait(
        [this](OgreNewt::World&)
        {
            m_world->getNewtonWorld()->AddModel(m_artModel);
            m_artModel->AddBodiesAndJointsToWorld();
        });

    m_finalized = true;
}

void SpiderBody::clearLegs()
{
    // SceneNodes are owned by Ogre, not by us — just null the pointers.
    // Physics bodies are owned by the articulation (ndWorld teardown cleans them).
    for (auto& lw : m_legWrappers)
    {
        lw.thighNode = nullptr;
        lw.calfNode = nullptr;
        lw.heelNode = nullptr;
    }
}

void SpiderBody::setWalkCycleDuration(float seconds)
{
    if (seconds > 0.1f)
    {
        m_artModel->duration = ndFloat32(seconds);
    }
}

void SpiderBody::setStepHeight(float meters)
{
    m_artModel->stepHeight = ndFloat32(meters);
}

void SpiderBody::setGaitSequence(const std::vector<int>& seq)
{
    for (int i = 0; i < SpiderArticulation::MAX_LEGS && i < static_cast<int>(seq.size()); ++i)
    {
        m_artModel->gaitSequence[i] = seq[i];
    }
}

void SpiderBody::setCanMove(bool b)
{
    m_artModel->canMove.store(b, std::memory_order_relaxed);
}

void SpiderBody::fireLuaCallback(float dt)
{
    if (!m_callback)
    {
        return;
    }

    m_callback->onMovementChanged(&m_manip, dt);

    // Push manip values into atomics so physics thread reads them next step
    m_artModel->stride.store(ndFloat32(m_manip.m_stride), std::memory_order_relaxed);
    m_artModel->omega.store(ndFloat32(m_manip.m_omega), std::memory_order_relaxed);
    m_artModel->bodySwayX.store(ndFloat32(m_manip.m_bodySwayX), std::memory_order_relaxed);
    m_artModel->bodySwayZ.store(ndFloat32(m_manip.m_bodySwayZ), std::memory_order_relaxed);
}

const LegTransformCache& SpiderBody::getCachedLegTransform(int legIndex) const
{
    // Caller must hold the mutex or call only after physics step is done.
    // Component calls this from update() (main thread) after Newton has completed
    // the physics step for this frame — which is safe under serial stepping.
    return m_artModel->cachedTransforms[legIndex];
}

int SpiderBody::getActiveLegCount() const
{
    return m_artModel->activeLegCount;
}
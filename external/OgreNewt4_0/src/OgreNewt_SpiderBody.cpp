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

    m_model->torsoNd->SetSleepState(false);

    // Gait advancement only when commanded to move
    if (m_model->canMove.load(std::memory_order_relaxed))
    {
        const ndFloat32 stride = m_model->stride.load(std::memory_order_relaxed);
        const ndFloat32 omega = m_model->omega.load(std::memory_order_relaxed);
        const ndFloat32 nextTime = advanceTime(m_timeAcc, timestep);

        for (int i = 0; i < m_model->activeLegCount; ++i)
        {
            LegGaitPhase state = getState(i, m_timeAcc, timestep);
            integrateLeg(i, state, stride, omega, timestep);
        }

        m_timeAcc = nextTime;
    }

    // ALWAYS apply effectors — this is what holds the legs up even when standing still.
    // Without this, IK constraints receive no target and legs collapse under gravity.
    applyEffectors();
}

void SpiderModelNotify::PostTransformUpdate(ndFloat32 /*timestep*/)
{
    std::lock_guard<std::mutex> lock(m_model->transformMutex);

    for (int i = 0; i < m_model->activeLegCount; ++i)
    {
        LegTransformCache& tc = m_model->cachedTransforms[i];
        tc.valid = true;

        tc.thighLen = m_model->thighLenArr[i];
        tc.calfLen = m_model->calfLenArr[i];
        tc.heelLen = m_model->calfLenArr[i]; // heel visual = same length as calf

        if (m_model->thighNd[i])
        {
            ndMatrixToOgre(m_model->thighNd[i]->GetMatrix(), tc.thighPos, tc.thighOrient);
        }
        if (m_model->calfNd[i])
        {
            ndMatrixToOgre(m_model->calfNd[i]->GetMatrix(), tc.calfPos, tc.calfOrient);
        }
        if (m_model->contactNd[i])
        {
            ndMatrixToOgre(m_model->contactNd[i]->GetMatrix(), tc.heelPos, tc.heelOrient);
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
    int /*boneAxis*/)
{
    if (m_finalized)
    {
        return;
    }

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

    ndVector legDir = hipToFoot.Scale(ndFloat32(1.0f) / totalLen);

    // ── Outward direction from body centre through hip (horizontal only) ──────
    // Tells the knee which side to bend toward.
    ndVector hipHoriz(hipLocal.m_x, 0.0f, hipLocal.m_z, 0.0f);
    ndFloat32 hipHorizLen = ndSqrt(hipHoriz.DotProduct(hipHoriz).GetScalar());
    ndVector outward = (hipHorizLen > 1e-6f) ? hipHoriz.Scale(ndFloat32(1.0f) / hipHorizLen) : ndVector(1.0f, 0.0f, 0.0f, 0.0f);
    outward.m_w = 0.0f;

    // ── Hinge axis: perpendicular to legDir, in the plane of legDir and outward ─
    ndVector hingeAxis = legDir.CrossProduct(outward);
    ndFloat32 haLen = ndSqrt(hingeAxis.DotProduct(hingeAxis).GetScalar());
    if (haLen < 1e-6f)
    {
        hingeAxis = ndVector(0.0f, 0.0f, 1.0f, 0.0f);
    }
    else
    {
        hingeAxis = hingeAxis.Scale(ndFloat32(1.0f) / haLen);
    }
    hingeAxis.m_w = 0.0f;

    // ── 2-link IK: find bent knee position using law of cosines ──────────────
    ndFloat32 cosA = (thighLen * thighLen + totalLen * totalLen - calfLen * calfLen) / (2.0f * thighLen * totalLen);
    cosA = ndClamp(cosA, ndFloat32(-1.0f), ndFloat32(1.0f));
    ndFloat32 sinA = ndSqrt(ndMax(ndFloat32(0.0f), ndFloat32(1.0f) - cosA * cosA));

    ndVector perpDir = hingeAxis.CrossProduct(legDir);
    ndFloat32 perpLen = ndSqrt(perpDir.DotProduct(perpDir).GetScalar());
    if (perpLen > 1e-6f)
    {
        perpDir = perpDir.Scale(ndFloat32(1.0f) / perpLen);
    }
    perpDir.m_w = 0.0f;

    ndVector thighDir = legDir.Scale(cosA) + perpDir.Scale(sinA);
    thighDir.m_w = 0.0f;

    ndVector kneeWorld = hipWorld + thighDir.Scale(thighLen);
    kneeWorld.m_w = 1.0f;

    ndVector kneeToFoot = footRestWorld - kneeWorld;
    kneeToFoot.m_w = 0.0f;
    ndFloat32 ktfLen = ndSqrt(kneeToFoot.DotProduct(kneeToFoot).GetScalar());
    ndVector calfDir = (ktfLen > 1e-6f) ? kneeToFoot.Scale(ndFloat32(1.0f) / ktfLen) : legDir;
    calfDir.m_w = 0.0f;

    ndVector ankleWorld = kneeWorld + calfDir.Scale(calfLen);
    ankleWorld.m_w = 1.0f;

    // ── Heel/contact geometry ─────────────────────────────────────────────────
    ndVector heelDir(0.0f, -1.0f, 0.0f, 0.0f);  // position math only
    ndVector heelDirUp(0.0f, 1.0f, 0.0f, 0.0f); // matrix construction

    ndVector footWorld = ankleWorld + heelDir.Scale(Dim::CONTACT_LENGTH * 0.5f);
    footWorld.m_w = 1.0f;

    ndVector thighCentre = hipWorld + thighDir.Scale(thighLen * 0.5f);
    ndVector calfCentre = kneeWorld + calfDir.Scale(calfLen * 0.5f);
    ndVector heelCentre = ankleWorld + heelDir.Scale(Dim::CONTACT_LENGTH * 0.25f);
    ndVector contactCentre = ankleWorld + heelDir.Scale(Dim::CONTACT_LENGTH * 0.75f);
    thighCentre.m_w = calfCentre.m_w = heelCentre.m_w = contactCentre.m_w = 1.0f;

    const ndFloat32 thighRadius = ndMax(thighLen * 0.08f, 0.015f);
    const ndFloat32 calfRadius = ndMax(calfLen * 0.07f, 0.012f);
    const ndFloat32 heelRadius = Dim::CONTACT_RADIUS;

    // ── Create physics bodies ─────────────────────────────────────────────────
    auto* thighNd = makeCapsule(thighRadius, thighLen, Dim::THIGH_MASS, thighCentre, thighDir);
    auto* calfNd = makeCapsule(calfRadius, calfLen, Dim::CALF_MASS, calfCentre, calfDir);
    auto* heelNd = makeCapsule(heelRadius, Dim::CONTACT_LENGTH, Dim::HEEL_MASS, heelCentre, heelDirUp);
    auto* contactNd = makeCapsule(heelRadius, Dim::CONTACT_LENGTH, Dim::CONTACT_MASS, contactCentre, heelDirUp);

    m_artModel->thighNd[legIndex] = thighNd;
    m_artModel->calfNd[legIndex] = calfNd;
    m_artModel->heelNd[legIndex] = heelNd;
    m_artModel->contactNd[legIndex] = contactNd;

    // ── Hip spherical joint ───────────────────────────────────────────────────
    ndMatrix hipFrame = ndGetIdentityMatrix();
    hipFrame.m_posit = hipWorld;
    auto hipJoint = ndSharedPtr<ndJointBilateralConstraint>(new ndJointSpherical(hipFrame, thighNd, torsoNd));

    // ── Knee IK hinge — hingeAxis already computed above ─────────────────────
    auto kneeJoint = ndSharedPtr<ndJointBilateralConstraint>(new ndIkJointHinge(pinMatrix(kneeWorld, hingeAxis), calfNd, thighNd));
    auto* kneeRaw = static_cast<ndIkJointHinge*>(kneeJoint.operator->());
    kneeRaw->SetLimitState(true);
    kneeRaw->SetLimits(-80.0f * ndDegreeToRad, 60.0f * ndDegreeToRad);
    m_artModel->kneeJoints[legIndex] = kneeRaw;

    // ── Ankle hinge — spring-damper ───────────────────────────────────────────
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
    ndModelArticulation::ndNode* const rootNode = m_artModel->GetRoot();
    auto* thighNode_ = m_artModel->AddLimb(rootNode, ndSharedPtr<ndBody>(thighNd), hipJoint);
    auto* calfNode_ = m_artModel->AddLimb(thighNode_, ndSharedPtr<ndBody>(calfNd), kneeJoint);
    auto* heelNode_ = m_artModel->AddLimb(calfNode_, ndSharedPtr<ndBody>(heelNd), heelJoint);
    (void)m_artModel->AddLimb(heelNode_, ndSharedPtr<ndBody>(contactNd), contactJoint);

    // ── IK effector (close loop) ──────────────────────────────────────────────
    ndMatrix effectPivot = torsoMat;
    effectPivot.m_posit = hipWorld;
    effectPivot.m_posit.m_w = 1.0f;

    ndVector effectorChildPivot = ankleWorld;
    effectorChildPivot.m_w = 1.0f;

    auto effJoint = ndSharedPtr<ndJointBilateralConstraint>(new ndIkSwivelPositionEffector(effectPivot, torsoNd, effectorChildPivot, heelNd));

    auto* effRaw = static_cast<ndIkSwivelPositionEffector*>(effJoint.operator->());
    effRaw->SetLinearSpringDamper(Dim::EFF_REG, Dim::EFF_SPRING, Dim::EFF_DAMPER);
    effRaw->SetAngularSpringDamper(Dim::EFF_REG, Dim::EFF_SPRING, Dim::EFF_DAMPER);
    effRaw->SetWorkSpaceConstraints(0.05f, (thighLen + calfLen) * 0.9f);

    const ndFloat32 effStrength = 500.0f * ndFloat32(m_pendingMass) * Dim::GRAVITY_MAG;
    effRaw->SetMaxForce(effStrength);
    effRaw->SetMaxTorque(effStrength);

    m_artModel->effectors[legIndex] = effRaw;
    m_artModel->AddCloseLoop(effJoint);

    // ── Initialise gait pose from effector rest position ──────────────────────
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

    m_artModel->thighLenArr[legIndex] = thighLen;
    m_artModel->calfLenArr[legIndex] = calfLen;

    m_artModel->activeLegCount = legIndex + 1;

    // ── Store scene nodes for PostTransformUpdate ─────────────────────────────
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

    // Empty constructor left m_bodyNotify null — create it now that m_body is valid
    if (!m_bodyNotify)
    {
        m_bodyNotify = new BodyNotify(this);
    }

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
            // Set AFTER world registration so it isn't overwritten
            m_artModel->torsoNd->SetNotifyCallback(m_bodyNotify);

            // Disable collision between all bodies within the articulation.
            // Without this, torso<->leg and leg<->leg contacts generate huge impulses.
            // m_artModel->SetSelfCollision(false);

            // Give all leg bodies a dedicated material ID that has no collision pair
            // registered with itself — Newton will skip them via GetMaterial returning null.
            const ndUnsigned32 legMatId = 0xFFFFFFFF; // reserved "spider limb" ID, register no pair for it
            for (int i = 0; i < m_artModel->activeLegCount; ++i)
            {
                if (m_artModel->thighNd[i])
                {
                    m_artModel->thighNd[i]->GetCollisionShape().m_shapeMaterial.m_userId = legMatId;
                }
                if (m_artModel->calfNd[i])
                {
                    m_artModel->calfNd[i]->GetCollisionShape().m_shapeMaterial.m_userId = legMatId;
                }
            }
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
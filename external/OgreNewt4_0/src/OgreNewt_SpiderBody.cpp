/*
    OgreNewt_SpiderBody.cpp

    Faithful port of Newton Dynamics 4 ndQuadSpiderPlayer demo.

    Threading discipline:
      addLegChain()  — pure math + stash, ZERO Newton allocations.
      finalizeModel() — one enqueuePhysicsAndWait() creates ALL Newton objects
                        (bodies, shapes, joints) on Newton's own thread, avoiding
                        races with ndFreeListAlloc.
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
    // Build a pin matrix: front column = dir, position = pivot
    static ndMatrix pinMatrix(const ndVector& pivot, const ndVector& dir)
    {
        ndMatrix m = ndGramSchmidtMatrix(dir);
        m.m_posit = pivot;
        m.m_posit.m_w = 1.0f;
        return m;
    }

    // Build a capsule body: long axis = dir, centre = pos.
    // MUST be called from Newton's thread (uses ndFreeListAlloc internally).
    static ndBodyDynamic* makeCapsule(ndFloat32 radius, ndFloat32 length, ndFloat32 mass, const ndVector& centre, const ndVector& dir)
    {
        ndMatrix mat = ndGramSchmidtMatrix(dir);
        mat.m_posit = centre;
        mat.m_posit.m_w = 1.0f;

        ndShapeInstance inst(new ndShapeCapsule(radius, radius, length));
        auto* body = new ndBodyDynamic();
        body->SetCollisionShape(inst);
        body->SetMassMatrix(mass, inst);
        body->SetMatrix(mat);
        return body;
    }

    // Convert ndMatrix to Ogre position + orientation
    static void ndMatrixToOgre(const ndMatrix& m, Ogre::Vector3& pos, Ogre::Quaternion& orient)
    {
        pos = Ogre::Vector3(m.m_posit.m_x, m.m_posit.m_y, m.m_posit.m_z);
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

    namespace Dim
    {
        constexpr ndFloat32 THIGH_MASS = 0.25f;
        constexpr ndFloat32 CALF_MASS = 0.25f;
        constexpr ndFloat32 HEEL_MASS = 0.125f;
        constexpr ndFloat32 CONTACT_MASS = 0.125f;

        constexpr ndFloat32 CONTACT_RADIUS = 0.030f;
        constexpr ndFloat32 CONTACT_LENGTH = 0.050f;

        constexpr ndFloat32 HEEL_REG = 0.001f;
        constexpr ndFloat32 HEEL_SPRING = 2000.0f;
        constexpr ndFloat32 HEEL_DAMPER = 50.0f;

        constexpr ndFloat32 CONTACT_REG = 0.002f;
        constexpr ndFloat32 CONTACT_SPRING = 2000.0f;
        constexpr ndFloat32 CONTACT_DAMPER = 50.0f;

        constexpr ndFloat32 EFF_REG = 0.001f;
        constexpr ndFloat32 EFF_SPRING = 1500.0f;
        constexpr ndFloat32 EFF_DAMPER = 50.0f;

        constexpr ndFloat32 GRAVITY_MAG = 9.8f;
    }
} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
//  SpiderModelNotify
// ─────────────────────────────────────────────────────────────────────────────
SpiderModelNotify::SpiderModelNotify(SpiderArticulation* model) : ndModelNotify(), m_model(model), m_timeAcc(0.0f)
{
}

// Matches ndProdeduralGaitGenerator::CalculateTime() — angular wrap
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

// Matches ndProdeduralGaitGenerator::GetState()
LegGaitPhase SpiderModelNotify::getState(int legIndex, ndFloat32 timeAcc, ndFloat32 timestep) const
{
    const ndFloat32 dur = m_model->duration;
    const ndFloat32 airEnd = dur * m_model->airFraction; // timeLine[1]
    const int seq = m_model->gaitSequence[legIndex];
    const ndFloat32 offset = ndFloat32(seq) * 0.25f * dur;

    ndFloat32 t0 = ndMod(timeAcc + offset, dur);
    ndFloat32 t1 = t0 + timestep;

    if (t0 <= airEnd)
    {
        return (t1 > airEnd) ? LegGaitPhase::airToGround : LegGaitPhase::onAir;
    }
    return (t1 > dur) ? LegGaitPhase::groundToAir : LegGaitPhase::onGround;
}

// Matches ndProdeduralGaitGenerator::IntegrateLeg()
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
    case LegGaitPhase::groundToAir:
    {
        // Last ground tick: set up the upcoming AIR phase.
        ndVector airTarget = pose.base;
        airTarget.m_x += stride * 0.5f;
        pose.end = airTarget;
        pose.start = pose.posit;
        pose.maxTime = airEnd;
        pose.time = 0.0f;

        // Parabolic arch Y coefficients
        ndFloat32 y0 = pose.start.m_y;
        ndFloat32 y1 = pose.end.m_y;
        ndFloat32 h = 0.5f * (y0 + y1) + stepH;
        pose.a0 = y0;
        pose.a1 = 4.0f * (h - y0) - (y1 - y0);
        pose.a2 = y1 - y0 - pose.a1;

        phase = LegGaitPhase::onAir;
        break;
    }

    case LegGaitPhase::airToGround:
    {
        // Last air tick: set up the upcoming GROUND (plant) phase.
        ndVector groundTarget = pose.base;
        groundTarget.m_x -= stride * 0.5f;

        ndFloat32 totalAngle = omega * groundDur;
        ndMatrix yawMat(ndYawMatrix(totalAngle));
        groundTarget = yawMat.RotateVector(groundTarget);
        groundTarget.m_w = 0.0f;

        pose.start = pose.posit;
        pose.end = groundTarget;
        pose.maxTime = groundDur;
        pose.time = 0.0f;

        phase = LegGaitPhase::onGround;
        break;
    }

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

        // Body sway (mirrors ndBodySwingControl::Evaluate)
        ndMatrix localMat = eff->GetLocalMatrix1();
        ndVector p0 = localMat.TransformVector(m_model->legPose[i].posit);
        ndVector p1 = swayMat.TransformVector(p0);
        ndVector p2 = localMat.UntransformVector(p1);
        p2.m_w = 0.0f;

        // Swivel
        ndFloat32 swivelAngle = eff->CalculateLookAtSwivelAngle(upVector);
        eff->SetSwivelAngle(swivelAngle);

        // Knee safety guard + heel coupling (matches ndController::Update)
        ndIkJointHinge* const knee = m_model->kneeJoints[i];
        if (knee)
        {
            ndFloat32 minAngle, maxAngle;
            knee->GetLimits(minAngle, maxAngle);
            const ndFloat32 safeGuard = 3.0f * ndDegreeToRad;
            maxAngle = ndMax(0.0f, maxAngle - safeGuard);
            minAngle = ndMin(0.0f, minAngle + safeGuard);

            ndFloat32 kneeAngle = knee->GetAngle();
            if (kneeAngle > maxAngle || kneeAngle < minAngle)
            {
                eff->SetAsReducedDof();
            }

            ndJointHinge* const heel = m_model->heelJoints[i];
            if (heel)
            {
                heel->SetTargetAngle(-kneeAngle * 0.5f);
            }
        }

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

    // Always apply — holds legs up even when standing still
    applyEffectors();
}

void SpiderModelNotify::PostTransformUpdate(ndFloat32 /*timestep*/)
{
    // Compute anyLegOnGround before taking the mutex (avoids holding it longer)
    bool anyOnGround = false;
    for (int i = 0; i < m_model->activeLegCount; ++i)
    {
        if (m_model->legPhase[i] == LegGaitPhase::onGround || m_model->legPhase[i] == LegGaitPhase::airToGround)
        {
            anyOnGround = true;
            break;
        }
    }
    m_model->anyLegOnGround.store(anyOnGround, std::memory_order_relaxed);

    // Write transform cache under mutex so main thread gets consistent snapshots
    std::lock_guard<std::mutex> lock(m_model->transformMutex);
    for (int i = 0; i < m_model->activeLegCount; ++i)
    {
        LegTransformCache& tc = m_model->cachedTransforms[i];
        tc.valid = true;
        tc.thighLen = m_model->thighLenArr[i];
        tc.calfLen = m_model->calfLenArr[i];
        tc.heelLen = m_model->calfLenArr[i];

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
SpiderBody::SpiderBody(OgreNewt::World* world, Ogre::SceneManager* sceneManager, const OgreNewt::CollisionPtr& collision, float mass, const Ogre::Vector3& massOrigin, const Ogre::Vector3& gravity, const Ogre::Vector3& /*defaultDirection*/,
    const Ogre::Vector3& initialPosition, const Ogre::Quaternion& initialOrientation, SpiderCallback* callback) :
    OgreNewt::Body(world, sceneManager),
    m_callback(callback),
    m_pendingCollision(collision),
    m_pendingMass(mass),
    m_pendingMassOrigin(massOrigin),
    m_pendingGravity(gravity),
    m_pendingPosition(initialPosition),
    m_pendingOrientation(initialOrientation)
{
    // Compute spawn matrix now so addLegChain() can use it for geometry math.
    // No Newton allocations here — addLegChain / finalizeModel are deferred.
    m_spawnMatrix = ndGetIdentityMatrix();
    OgreNewt::Converters::QuatPosToMatrix(Ogre::Quaternion(initialOrientation.w, initialOrientation.x, initialOrientation.y, initialOrientation.z), Ogre::Vector3(initialPosition.x, initialPosition.y, initialPosition.z), m_spawnMatrix);

    m_artModel = new SpiderArticulation();
    // torsoNd stays null until the enqueue in finalizeModel() fires.
}

SpiderBody::~SpiderBody()
{
    // Sever notify back-pointer before anything else
    if (m_artModel && m_artModel->torsoNd)
    {
        auto& np = m_artModel->torsoNd->GetNotifyCallback();
        if (np)
        {
            if (auto* bn = dynamic_cast<BodyNotify*>(*np))
            {
                bn->SetOgreNewtBody(nullptr);
            }
        }
    }

    clearLegs();

    // m_artModelSharedPtr destructs here:
    //   Before finalizeModel: sole owner -> deletes m_artModel.
    //   After  finalizeModel: world co-owns -> our ref drops, world drops on teardown.
    delete m_callback;
}

bool SpiderBody::isOnGround() const
{
    if (!m_artModel)
    {
        return false;
    }
    return m_artModel->anyLegOnGround.load(std::memory_order_relaxed);
}

void SpiderBody::snapshotTransforms(std::array<OgreNewt::LegTransformCache, SpiderArticulation::MAX_LEGS>& out, int& legCountOut) const
{
    std::lock_guard<std::mutex> lk(m_artModel->transformMutex);
    legCountOut = m_artModel->activeLegCount;
    for (int i = 0; i < legCountOut; ++i)
    {
        out[i] = m_artModel->cachedTransforms[i];
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  addLegChain — PURE MATH + STASH. Zero Newton allocations.
// ─────────────────────────────────────────────────────────────────────────────
void SpiderBody::addLegChain(Ogre::SceneNode* thighNode, Ogre::SceneNode* calfNode, Ogre::SceneNode* heelNode, const Ogre::Vector3& hipLocalOgre, const Ogre::Vector3& footRestLocalOgre, float thighLen, float calfLen, float /*swivelSign*/,
    int /*boneAxis*/)
{
    if (m_finalized)
    {
        return;
    }

    const int legIndex = m_artModel->activeLegCount;
    if (legIndex >= SpiderArticulation::MAX_LEGS)
    {
        return;
    }

    const ndMatrix& torsoMat = m_spawnMatrix;

    // ── Convert torso-local positions to world space ──────────────────────────
    ndVector hipLocal(hipLocalOgre.x, hipLocalOgre.y, hipLocalOgre.z, 1.0f);
    ndVector footRestLocal(footRestLocalOgre.x, footRestLocalOgre.y, footRestLocalOgre.z, 1.0f);

    ndVector hipWorld = torsoMat.TransformVector(hipLocal);
    hipWorld.m_w = 1.0f;
    ndVector footRestWorld = torsoMat.TransformVector(footRestLocal);
    footRestWorld.m_w = 0.0f;

    ndVector hipToFoot = footRestWorld - hipWorld;
    hipToFoot.m_w = 0.0f;
    ndFloat32 totalLen = ndSqrt(hipToFoot.DotProduct(hipToFoot).GetScalar());
    if (totalLen < 1e-6f)
    {
        return;
    }

    ndVector legDir = hipToFoot.Scale(1.0f / totalLen);

    // Outward direction (horizontal projection of hip offset from body centre)
    ndVector hipHoriz(hipLocal.m_x, 0.0f, hipLocal.m_z, 0.0f);
    ndFloat32 hipHorizLen = ndSqrt(hipHoriz.DotProduct(hipHoriz).GetScalar());
    ndVector outward = (hipHorizLen > 1e-6f) ? hipHoriz.Scale(1.0f / hipHorizLen) : ndVector(1.0f, 0.0f, 0.0f, 0.0f);
    outward.m_w = 0.0f;

    // Hinge axis: perpendicular to legDir in the plane of legDir and outward
    ndVector hingeAxis = legDir.CrossProduct(outward);
    ndFloat32 haLen = ndSqrt(hingeAxis.DotProduct(hingeAxis).GetScalar());
    hingeAxis = (haLen > 1e-6f) ? hingeAxis.Scale(1.0f / haLen) : ndVector(0.0f, 0.0f, 1.0f, 0.0f);
    hingeAxis.m_w = 0.0f;

    // 2-link IK: bent knee via law of cosines
    ndFloat32 cosA = ndClamp((thighLen * thighLen + totalLen * totalLen - calfLen * calfLen) / (2.0f * thighLen * totalLen), -1.0f, 1.0f);
    ndFloat32 sinA = ndSqrt(ndMax(0.0f, 1.0f - cosA * cosA));

    ndVector perpDir = hingeAxis.CrossProduct(legDir);
    ndFloat32 perpLen = ndSqrt(perpDir.DotProduct(perpDir).GetScalar());
    if (perpLen > 1e-6f)
    {
        perpDir = perpDir.Scale(1.0f / perpLen);
    }
    perpDir.m_w = 0.0f;

    ndVector thighDir = legDir.Scale(cosA) + perpDir.Scale(sinA);
    thighDir.m_w = 0.0f;
    ndVector kneeWorld = hipWorld + thighDir.Scale(thighLen);
    kneeWorld.m_w = 1.0f;
    ndVector kToF = footRestWorld - kneeWorld;
    kToF.m_w = 0.0f;
    ndFloat32 ktfLen = ndSqrt(kToF.DotProduct(kToF).GetScalar());
    ndVector calfDir = (ktfLen > 1e-6f) ? kToF.Scale(1.0f / ktfLen) : legDir;
    calfDir.m_w = 0.0f;

    ndVector ankleWorld = kneeWorld + calfDir.Scale(calfLen);
    ankleWorld.m_w = 1.0f;

    ndVector heelDown(0.0f, -1.0f, 0.0f, 0.0f);

    // Stash all geometry — Newton allocations happen in finalizeModel's enqueue
    SpiderArticulation::PendingLeg pl;
    pl.hipWorld = hipWorld;
    pl.kneeWorld = kneeWorld;
    pl.ankleWorld = ankleWorld;
    pl.footWorld = ankleWorld + heelDown.Scale(Dim::CONTACT_LENGTH * 0.5f);
    pl.footWorld.m_w = 1.0f;
    pl.thighDir = thighDir;
    pl.calfDir = calfDir;
    pl.hingeAxis = hingeAxis;
    pl.thighCentre = hipWorld + thighDir.Scale(ndFloat32(thighLen) * 0.5f);
    pl.calfCentre = kneeWorld + calfDir.Scale(ndFloat32(calfLen) * 0.5f);
    pl.heelCentre = ankleWorld + heelDown.Scale(Dim::CONTACT_LENGTH * 0.25f);
    pl.contactCentre = ankleWorld + heelDown.Scale(Dim::CONTACT_LENGTH * 0.75f);
    pl.thighCentre.m_w = pl.calfCentre.m_w = pl.heelCentre.m_w = pl.contactCentre.m_w = 1.0f;
    pl.thighLen = ndFloat32(thighLen);
    pl.calfLen = ndFloat32(calfLen);
    pl.thighRadius = ndMax(ndFloat32(thighLen) * 0.08f, 0.015f);
    pl.calfRadius = ndMax(ndFloat32(calfLen) * 0.07f, 0.012f);
    pl.effectPivot = torsoMat;
    pl.effectPivot.m_posit = hipWorld;
    pl.effectPivot.m_posit.m_w = 1.0f;
    pl.effChildPivot = ankleWorld;
    pl.effStrength = 500.0f * ndFloat32(m_pendingMass) * Dim::GRAVITY_MAG;
    pl.thighNode = thighNode;
    pl.calfNode = calfNode;
    pl.heelNode = heelNode;

    m_artModel->pendingLegs.push_back(pl);

    // Gait timing (pure data, no Newton)
    m_artModel->legPose[legIndex] = LegPose{};
    m_artModel->legPose[legIndex].maxTime = 1.0f;
    m_artModel->legPhase[legIndex] = LegGaitPhase::onGround;
    m_artModel->legCycleTime[legIndex] = ndFloat32(legIndex) * 0.25f * m_artModel->duration;
    m_artModel->thighLenArr[legIndex] = ndFloat32(thighLen);
    m_artModel->calfLenArr[legIndex] = ndFloat32(calfLen);

    // Visual wrappers
    m_legWrappers[legIndex].thighNode = thighNode;
    m_legWrappers[legIndex].calfNode = calfNode;
    m_legWrappers[legIndex].heelNode = heelNode;

    m_artModel->activeLegCount = legIndex + 1;
}

// ─────────────────────────────────────────────────────────────────────────────
//  finalizeModel — ALL Newton allocations in one enqueuePhysicsAndWait
// ─────────────────────────────────────────────────────────────────────────────
void SpiderBody::finalizeModel()
{
    if (m_finalized || m_artModel->activeLegCount == 0)
    {
        return;
    }

    m_bodyPtr = nullptr; // Articulation owns torso, not m_bodyPtr
    m_isOwner = false;
    m_gravity = m_pendingGravity;

    if (!m_bodyNotifyPtr)
    {
        m_bodyNotifyPtr = new BodyNotify(this);
    }

    m_notify = new SpiderModelNotify(m_artModel);
    m_artModel->SetNotifyCallback(ndSharedPtr<ndModelNotify>(m_notify));

    // Build owning shared ptr BEFORE enqueue so refcount is already 1.
    // AddModel() increments it to 2. SpiderBody holds the other ref.
    m_artModelSharedPtr = ndSharedPtr<ndModel>(m_artModel);

    // Capture everything the lambda needs by value
    auto pendingLegs = std::move(m_artModel->pendingLegs); // move out, lambda owns
    const int legCount = m_artModel->activeLegCount;

    m_world->enqueuePhysicsAndWait(
        [this, pendingLegs = std::move(pendingLegs), legCount](OgreNewt::World&)
        {
            // ── Torso body ───────────────────────────────────────────────────────
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

            ndShapeInstance& inst = torsoNd->GetCollisionShape();
            torsoNd->SetMassMatrix(ndFloat32(m_pendingMass), inst);

            if (m_pendingMassOrigin != Ogre::Vector3::ZERO)
            {
                ndVector com(m_pendingMassOrigin.x, m_pendingMassOrigin.y, m_pendingMassOrigin.z, 0.0f);
                torsoNd->SetCentreOfMass(com);
            }

            torsoNd->SetMatrix(m_spawnMatrix);
            torsoNd->SetNotifyCallback(m_bodyNotifyPtr);

            m_artModel->torsoNd = torsoNd;

            // Register as articulation root — must be before any AddLimb
            ndModelArticulation::ndNode* const rootNode = m_artModel->AddRootBody(ndSharedPtr<ndBody>(torsoNd));

            // ── Leg chains ───────────────────────────────────────────────────────
            const ndUnsigned32 legMatId = 0xFFFFFFFF; // skip material pair lookup for limbs

            for (int i = 0; i < legCount; ++i)
            {
                const SpiderArticulation::PendingLeg& pl = pendingLegs[i];

                // Bodies — all Newton allocations here, on Newton's thread
                auto* thighNd = makeCapsule(pl.thighRadius, pl.thighLen, Dim::THIGH_MASS, pl.thighCentre, pl.thighDir);
                auto* calfNd = makeCapsule(pl.calfRadius, pl.calfLen, Dim::CALF_MASS, pl.calfCentre, pl.calfDir);
                auto* heelNd = makeCapsule(Dim::CONTACT_RADIUS, Dim::CONTACT_LENGTH, Dim::HEEL_MASS, pl.heelCentre, ndVector(0.0f, 1.0f, 0.0f, 0.0f));
                auto* contactNd = makeCapsule(Dim::CONTACT_RADIUS, Dim::CONTACT_LENGTH, Dim::CONTACT_MASS, pl.contactCentre, ndVector(0.0f, 1.0f, 0.0f, 0.0f));

                // Tag limb bodies so ContactNotify early-outs for them
                thighNd->GetCollisionShape().m_shapeMaterial.m_userId = legMatId;
                calfNd->GetCollisionShape().m_shapeMaterial.m_userId = legMatId;
                heelNd->GetCollisionShape().m_shapeMaterial.m_userId = legMatId;
                contactNd->GetCollisionShape().m_shapeMaterial.m_userId = legMatId;

                // Prevent ndShapeConvexPolygon::GenerateConvexCap assert:
                // these tiny capsules can reach static mesh corner junctions where
                // the polygon edge direction is nearly parallel to the adjacent face normal.
                // A skin margin keeps them slightly above the surface, avoiding that
                // degenerate cross product. Value must be >= contact capsule radius (0.030)
                // so the effective contact distance stays physically sensible.
                heelNd->GetCollisionShape().m_skinMargin = 0.04f;
                contactNd->GetCollisionShape().m_skinMargin = 0.04f;

                m_artModel->thighNd[i] = thighNd;
                m_artModel->calfNd[i] = calfNd;
                m_artModel->heelNd[i] = heelNd;
                m_artModel->contactNd[i] = contactNd;

                // ── Joints — owned by shared ptrs in SpiderArticulation ───────────
                m_artModel->hipJointPtrs[i] = ndSharedPtr<ndJointBilateralConstraint>(new ndJointSpherical(pl.effectPivot, thighNd, torsoNd));

                m_artModel->kneeJointPtrs[i] = ndSharedPtr<ndJointBilateralConstraint>(new ndIkJointHinge(pinMatrix(pl.kneeWorld, pl.hingeAxis), calfNd, thighNd));

                m_artModel->heelJointPtrs[i] = ndSharedPtr<ndJointBilateralConstraint>(new ndJointHinge(pinMatrix(pl.ankleWorld, pl.hingeAxis), heelNd, calfNd));

                m_artModel->contactJointPtrs[i] = ndSharedPtr<ndJointBilateralConstraint>(new ndJointSlider(pinMatrix(pl.footWorld, ndVector(0.0f, 1.0f, 0.0f, 0.0f)), contactNd, heelNd));

                m_artModel->effectorPtrs[i] = ndSharedPtr<ndJointBilateralConstraint>(new ndIkSwivelPositionEffector(pl.effectPivot, torsoNd, pl.effChildPivot, heelNd));

                // Configure joints via raw observer ptrs
                auto* kneeRaw = static_cast<ndIkJointHinge*>(m_artModel->kneeJointPtrs[i].operator->());
                kneeRaw->SetLimitState(true);
                kneeRaw->SetLimits(-80.0f * ndDegreeToRad, 60.0f * ndDegreeToRad);
                m_artModel->kneeJoints[i] = kneeRaw;

                auto* heelRaw = static_cast<ndJointHinge*>(m_artModel->heelJointPtrs[i].operator->());
                heelRaw->SetAsSpringDamper(Dim::HEEL_REG, Dim::HEEL_SPRING, Dim::HEEL_DAMPER);
                m_artModel->heelJoints[i] = heelRaw;

                auto* contactRaw = static_cast<ndJointSlider*>(m_artModel->contactJointPtrs[i].operator->());
                contactRaw->SetLimitState(true);
                contactRaw->SetLimits(-0.05f, 0.05f);
                contactRaw->SetAsSpringDamper(Dim::CONTACT_REG, Dim::CONTACT_SPRING, Dim::CONTACT_DAMPER);

                auto* effRaw = static_cast<ndIkSwivelPositionEffector*>(m_artModel->effectorPtrs[i].operator->());
                effRaw->SetLinearSpringDamper(Dim::EFF_REG, Dim::EFF_SPRING, Dim::EFF_DAMPER);
                effRaw->SetAngularSpringDamper(Dim::EFF_REG, Dim::EFF_SPRING, Dim::EFF_DAMPER);
                effRaw->SetWorkSpaceConstraints(0.05f, (pl.thighLen + pl.calfLen) * 0.9f);
                effRaw->SetMaxForce(pl.effStrength);
                effRaw->SetMaxTorque(pl.effStrength);
                m_artModel->effectors[i] = effRaw;

                // ── Build articulation tree ───────────────────────────────────────
                auto* thighNode_ = m_artModel->AddLimb(rootNode, ndSharedPtr<ndBody>(thighNd), m_artModel->hipJointPtrs[i]);
                auto* calfNode_ = m_artModel->AddLimb(thighNode_, ndSharedPtr<ndBody>(calfNd), m_artModel->kneeJointPtrs[i]);
                auto* heelNode_ = m_artModel->AddLimb(calfNode_, ndSharedPtr<ndBody>(heelNd), m_artModel->heelJointPtrs[i]);
                (void)m_artModel->AddLimb(heelNode_, ndSharedPtr<ndBody>(contactNd), m_artModel->contactJointPtrs[i]);
                m_artModel->AddCloseLoop(m_artModel->effectorPtrs[i]);

                // Initialise gait pose base from effector rest position
                LegPose& pose = m_artModel->legPose[i];
                pose.base = effRaw->GetEffectorPosit();
                pose.posit = pose.base;
                pose.start = pose.base;
                pose.end = pose.base;
                pose.a0 = pose.base.m_y;
                pose.a1 = pose.a2 = 0.0f;
                pose.time = 0.0f;
                pose.maxTime = 1.0f;
            }

            // ── UpVector close loop (keeps spider upright) ───────────────────────
            ndBodyKinematic* sentinel = m_world->getNewtonWorld()->GetSentinelBody();
            m_artModel->upVectorJoint = ndSharedPtr<ndJointBilateralConstraint>(new ndJointUpVector(ndVector(0.0f, 1.0f, 0.0f, 0.0f), torsoNd, sentinel));
            m_artModel->AddCloseLoop(m_artModel->upVectorJoint);

            // ── Register model with Newton world ─────────────────────────────────
            m_world->getNewtonWorld()->AddModel(m_artModelSharedPtr);
        });

    m_finalized = true;
}

void SpiderBody::clearLegs()
{
    // SceneNodes are Ogre-owned — just null our pointers.
    // Physics bodies are owned by the articulation tree.
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
    m_artModel->stride.store(ndFloat32(m_manip.m_stride), std::memory_order_relaxed);
    m_artModel->omega.store(ndFloat32(m_manip.m_omega), std::memory_order_relaxed);
    m_artModel->bodySwayX.store(ndFloat32(m_manip.m_bodySwayX), std::memory_order_relaxed);
    m_artModel->bodySwayZ.store(ndFloat32(m_manip.m_bodySwayZ), std::memory_order_relaxed);
}

const LegTransformCache& SpiderBody::getCachedLegTransform(int legIndex) const
{
    return m_artModel->cachedTransforms[legIndex];
}

int SpiderBody::getActiveLegCount() const
{
    return m_artModel->activeLegCount;
}
/*
    OgreNewt_SpiderBody.cpp

    Corrections vs previous version:
      - Update/PostUpdate on SpiderModelNotify (ndModelNotify), not on ndModelArticulation
      - SetAsSpringDamper(regularizer, spring, damper)  — no bool
      - AddLimb(parentNode, ndSharedPtr<ndBody>, ndSharedPtr<ndJoint>)
      - AddCloseLoop(ndSharedPtr<ndJoint>)  for effectors + upVector
      - SetMassMatrix(mass, shapeInstance)  — engine computes inertia
      - model->SetNotifyCallback(ndSharedPtr<ndModelNotify>) sets the notify
*/

#include "OgreNewt_Stdafx.h"
#include "OgreNewt_SpiderBody.h"
#include "OgreNewt_Tools.h"
#include <OgreSceneNode.h>

using namespace OgreNewt;
using namespace OgreNewt::SpiderConstants;

// ─────────────────────────────────────────────────────────────────────────────
//  Internal helpers
// ─────────────────────────────────────────────────────────────────────────────

// Pin-and-pivot matrix: X column = pinAxis, position = pivot
static ndMatrix pinMatrix(const ndVector& pivot, const ndVector& pinAxis)
{
    ndMatrix m = ndGramSchmidtMatrix(pinAxis);
    m.m_posit = pivot;
    m.m_posit.m_w = 1.0f;
    return m;
}

// Capsule matrix: X column = long axis direction, position = centre
static ndMatrix capsuleMatrix(const ndVector& centre, const ndVector& dir)
{
    ndMatrix m = ndGramSchmidtMatrix(dir);
    m.m_posit = centre;
    m.m_posit.m_w = 1.0f;
    return m;
}

static ndBodyDynamic* makeCapsuleBody(ndFloat32 radius, ndFloat32 length, ndFloat32 mass, const ndMatrix& mat)
{
    ndShapeInstance inst(new ndShapeCapsule(radius, radius, length));
    auto* body = new ndBodyDynamic();
    body->SetCollisionShape(inst);
    body->SetMassMatrix(mass, inst); // engine computes inertia from shape
    body->SetMatrix(mat);
    return body;
}

static OgreNewt::Body* wrapBody(OgreNewt::World* world, ndBodyDynamic* ndBody, Ogre::SceneManager* sceneManager, Ogre::SceneNode* node = nullptr)
{
    // Non-owning: ndSharedPtr inside articulation owns the ndBodyDynamic
    auto* ogreBody = new OgreNewt::Body(world, sceneManager, ndBody);
    ogreBody->setBodyNotify(new OgreNewt::BodyNotify(ogreBody));
    if (node)
    {
        ogreBody->attachNode(node);
    }
    return ogreBody;
}

static ndVector defaultFootLocal(int legIndex)
{
    ndFloat32 x = (legIndex < 2) ? LEG_OFFSET_X_FRONT : LEG_OFFSET_X_REAR;
    ndFloat32 z = (legIndex == 0 || legIndex == 2) ? LEG_OFFSET_Z_RIGHT : LEG_OFFSET_Z_LEFT;
    ndFloat32 reach = (THIGH_LENGTH + CALF_LENGTH) * 0.9f;
    return ndVector(x, -reach * 0.5f, z * 2.2f, 0.0f);
}

// ─────────────────────────────────────────────────────────────────────────────
//  SpiderModelNotify
// ─────────────────────────────────────────────────────────────────────────────
SpiderModelNotify::SpiderModelNotify(SpiderArticulation* model) : ndModelNotify(), m_model(model)
{
}

void SpiderModelNotify::Update(ndFloat32 timestep)
{
    if (!m_model || !m_model->torsoNd)
    {
        return;
    }
    updateGait(timestep);
    applyEffectors();
}

void SpiderModelNotify::updateGait(ndFloat32 timestep)
{
    const ndFloat32 stride = m_stride.load(std::memory_order_relaxed);
    const ndFloat32 omega = m_omega.load(std::memory_order_relaxed);
    const ndMatrix torsoMat = m_model->torsoNd->GetMatrix();
    const ndFloat32 airEnd = WALK_DURATION * PHASE_SHIFT;

    for (int i = 0; i < 4; ++i)
    {
        LegGaitState& g = m_model->legGait[i];

        g.cycleTime += timestep;
        if (g.cycleTime >= WALK_DURATION)
        {
            g.cycleTime -= WALK_DURATION;
        }

        // Per-leg phase-shifted time
        ndFloat32 t = g.cycleTime - i * WALK_DURATION * PHASE_SHIFT;
        if (t < 0.0f)
        {
            t += WALK_DURATION;
        }

        // Compute neutral foot position in world space this frame
        ndVector restLocal = m_model->defaultFootLocal[i];
        restLocal.m_x += stride * 0.5f;
        if (ndAbs(omega) > 1e-4f)
        {
            ndMatrix yaw = ndYawMatrix(omega * timestep);
            restLocal = yaw.TransformVector(restLocal);
        }
        ndVector restWorld = torsoMat.TransformVector(restLocal);
        restWorld.m_w = 0.0f;

        switch (g.phase)
        {
        case LegPhase::onGround:
        {
            ndVector delta = restWorld - g.restPos;
            ndFloat32 distSq = delta.DotProduct(delta).GetScalar();
            if (ndAbs(stride) > 0.01f && distSq > (WALK_STRIDE * 0.5f) * (WALK_STRIDE * 0.5f))
            {
                g.liftPos = g.restPos;
                g.phase = LegPhase::groundToAir;
            }
            else
            {
                // Foot slowly tracks target while grounded
                g.restPos = g.restPos + (restWorld - g.restPos) * 0.05f;
            }
            break;
        }

        case LegPhase::groundToAir:
        {
            g.landPos = restWorld;
            ndFloat32 liftY = g.liftPos.m_y;
            ndFloat32 landY = g.landPos.m_y;
            ndFloat32 peakY = ndMax(liftY, landY) + m_swingAmplitude;
            ndFloat32 T2 = airEnd * 0.5f;

            g.arcA0 = liftY;
            g.arcA2 = 2.0f * (liftY - 2.0f * peakY + landY) / (airEnd * airEnd);
            g.arcA1 = (peakY - g.arcA0 - g.arcA2 * T2 * T2) / T2;
            g.phase = LegPhase::onAir;
            break;
        }

        case LegPhase::onAir:
            if (t >= airEnd)
            {
                g.restPos = g.landPos;
                g.phase = LegPhase::airToGround;
            }
            break;

        case LegPhase::airToGround:
            g.phase = LegPhase::onGround;
            break;
        }
    }
}

ndVector SpiderModelNotify::computeFootTarget(int leg) const
{
    const LegGaitState& g = m_model->legGait[leg];

    if (g.phase == LegPhase::onAir || g.phase == LegPhase::groundToAir)
    {
        const ndFloat32 airEnd = WALK_DURATION * PHASE_SHIFT;
        ndFloat32 t = m_model->legGait[leg].cycleTime - leg * WALK_DURATION * PHASE_SHIFT;
        if (t < 0.0f)
        {
            t += WALK_DURATION;
        }
        t = ndClamp(t, 0.0f, airEnd);

        ndFloat32 u = t / airEnd;
        ndVector pos = g.liftPos + (g.landPos - g.liftPos) * u;
        pos.m_y = g.arcA0 + g.arcA1 * t + g.arcA2 * t * t;
        return pos;
    }

    return g.restPos;
}

void SpiderModelNotify::applyEffectors()
{
    const ndMatrix torsoMat = m_model->torsoNd->GetMatrix();
    const ndMatrix torsoInv = torsoMat.OrthoInverse();

    for (int i = 0; i < 4; ++i)
    {
        if (!m_model->effectors[i])
        {
            continue;
        }

        ndVector worldTarget = computeFootTarget(i);
        ndVector localTarget = torsoInv.TransformVector(worldTarget);
        m_model->effectors[i]->SetLocalTargetPosition(localTarget);

        // Swivel knee outward
        ndFloat32 swivelSign = (i == 0 || i == 2) ? 1.0f : -1.0f;
        m_model->effectors[i]->SetSwivelAngle(30.0f * ndDegreeToRad * swivelSign);

        // Couple heel angle to knee angle
        if (m_model->kneeJoints[i] && m_model->heelJoints[i])
        {
            ndFloat32 kneeAngle = m_model->kneeJoints[i]->GetAngle();
            m_model->heelJoints[i]->SetTargetAngle(-kneeAngle * 0.5f);
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  SpiderBody
// ─────────────────────────────────────────────────────────────────────────────
SpiderBody::SpiderBody(OgreNewt::World* world) : m_world(world)
{
    m_model = new SpiderArticulation();
    m_notify = new SpiderModelNotify(m_model);
    // SetNotifyCallback takes ndSharedPtr<ndModelNotify>; m_notify stays valid
    // as long as we hold m_model, which lives until world teardown.
    m_model->SetNotifyCallback(ndSharedPtr<ndModelNotify>(m_notify));
}

SpiderBody::~SpiderBody()
{
    // m_model is owned by ndWorld after AddModel(); do not delete here.
    // m_notify is owned by ndSharedPtr inside m_model; do not delete here.
    delete m_torsoBody;
    for (auto& lw : m_legWrappers)
    {
        delete lw.thigh;
        delete lw.calf;
        delete lw.heel;
        delete lw.contact;
    }
}

void SpiderBody::buildTorso(const ndMatrix& spawn, Ogre::SceneManager* sceneManager, Ogre::SceneNode* torsoNode)
{
    ndShapeInstance inst(new ndShapeBox(TORSO_SIZE_X, TORSO_SIZE_Y, TORSO_SIZE_Z));

    auto* torsoNd = new ndBodyDynamic();
    torsoNd->SetCollisionShape(inst);
    torsoNd->SetMassMatrix(TORSO_MASS, inst);
    torsoNd->SetMatrix(spawn);
    m_model->torsoNd = torsoNd;

    m_model->AddRootBody(ndSharedPtr<ndBody>(torsoNd));

    // UpVector — close loop, keeps torso upright
    ndBodyKinematic* sentinel = m_world->getNewtonWorld()->GetSentinelBody();
    m_model->AddCloseLoop(ndSharedPtr<ndJointBilateralConstraint>(new ndJointUpVector(ndVector(0.0f, 1.0f, 0.0f, 0.0f), torsoNd, sentinel)), "upVector");

    m_torsoBody = wrapBody(m_world, torsoNd, sceneManager, torsoNode);
}

void SpiderBody::buildLeg(int legIndex, Ogre::SceneManager* sceneManager, Ogre::SceneNode* thighNode, Ogre::SceneNode* calfNode, Ogre::SceneNode* heelNode)
{
    ndModelArticulation::ndNode* const treeRoot = m_model->GetRoot();
    const ndMatrix torsoMat = m_model->torsoNd->GetMatrix();

    const ndFloat32 sideSign = (legIndex == 0 || legIndex == 2) ? 1.0f : -1.0f;
    const ndFloat32 frontSign = (legIndex < 2) ? 1.0f : -1.0f;

    // ── World-space anchor points ─────────────────────────────────────────────
    ndVector hipLocal(frontSign * LEG_OFFSET_X_FRONT, LEG_OFFSET_Y, sideSign * LEG_OFFSET_Z_RIGHT, 1.0f);
    ndVector hipWorld = torsoMat.TransformVector(hipLocal);
    hipWorld.m_w = 1.0f;

    ndVector thighDir = ndVector(0.3f * sideSign, -0.9f, 0.0f, 0.0f).Normalize();
    ndVector kneeWorld = hipWorld + thighDir * THIGH_LENGTH;
    kneeWorld.m_w = 1.0f;
    ndVector thighCentre = hipWorld + thighDir * (THIGH_LENGTH * 0.5f);
    thighCentre.m_w = 1.0f;

    ndVector calfDir = ndVector(0.05f * sideSign, -0.999f, 0.0f, 0.0f).Normalize();
    ndVector ankleWorld = kneeWorld + calfDir * CALF_LENGTH;
    ankleWorld.m_w = 1.0f;
    ndVector calfCentre = kneeWorld + calfDir * (CALF_LENGTH * 0.5f);
    calfCentre.m_w = 1.0f;

    ndVector heelDir(0.0f, -1.0f, 0.0f, 0.0f);
    ndVector footWorld = ankleWorld + heelDir * HEEL_LENGTH;
    footWorld.m_w = 1.0f;
    ndVector heelCentre = ankleWorld + heelDir * (HEEL_LENGTH * 0.5f);
    heelCentre.m_w = 1.0f;

    ndVector contactCentre = footWorld + heelDir * (CONTACT_LENGTH * 0.5f);
    contactCentre.m_w = 1.0f;

    // ── Bodies ────────────────────────────────────────────────────────────────
    auto* thighNd = makeCapsuleBody(THIGH_RADIUS, THIGH_LENGTH, THIGH_MASS, capsuleMatrix(thighCentre, thighDir));
    auto* calfNd = makeCapsuleBody(CALF_RADIUS, CALF_LENGTH, CALF_MASS, capsuleMatrix(calfCentre, calfDir));
    auto* heelNd = makeCapsuleBody(HEEL_RADIUS, HEEL_LENGTH, HEEL_MASS, capsuleMatrix(heelCentre, heelDir));
    auto* contactNd = makeCapsuleBody(CONTACT_RADIUS, CONTACT_LENGTH, CONTACT_MASS, capsuleMatrix(contactCentre, heelDir));

    m_model->thighNd[legIndex] = thighNd;
    m_model->calfNd[legIndex] = calfNd;
    m_model->heelNd[legIndex] = heelNd;
    m_model->contactNd[legIndex] = contactNd;

    // ── Joints ────────────────────────────────────────────────────────────────
    // Hinge axis is the torso lateral direction (Z in world frame at spawn)
    ndVector hingeAxis = torsoMat.m_front; // Newton Z column

    // Hip spherical — frame just needs the pivot position
    ndMatrix hipFrame = ndGetIdentityMatrix();
    hipFrame.m_posit = hipWorld;

    auto hipPtr = ndSharedPtr<ndJointBilateralConstraint>(new ndIkJointSpherical(hipFrame, thighNd, m_model->torsoNd));

    // Knee IK hinge — X column = hinge axis
    auto kneePtr = ndSharedPtr<ndJointBilateralConstraint>(new ndIkJointHinge(pinMatrix(kneeWorld, hingeAxis), calfNd, thighNd));
    auto* kneeRaw = static_cast<ndIkJointHinge*>(kneePtr.operator->());
    kneeRaw->SetLimits(CALF_MIN_ANGLE, CALF_MAX_ANGLE);
    m_model->kneeJoints[legIndex] = kneeRaw;

    // Ankle hinge — spring-damper, corrected API: no bool
    auto heelPtr = ndSharedPtr<ndJointBilateralConstraint>(new ndJointHinge(pinMatrix(ankleWorld, hingeAxis), heelNd, calfNd));
    auto* heelRaw = static_cast<ndJointHinge*>(heelPtr.operator->());
    heelRaw->SetAsSpringDamper(HEEL_REGULARIZER, HEEL_SPRING, HEEL_DAMPER);
    m_model->heelJoints[legIndex] = heelRaw;

    // Contact slider — spring-damper, pin axis = slide direction
    auto contactPtr = ndSharedPtr<ndJointBilateralConstraint>(new ndJointSlider(pinMatrix(footWorld, heelDir), contactNd, heelNd));
    static_cast<ndJointSlider*>(contactPtr.operator->())->SetAsSpringDamper(CONTACT_REGULARIZER, CONTACT_SPRING, CONTACT_DAMPER);

    // ── AddLimb — build tree ──────────────────────────────────────────────────
    auto* thighNode_ = m_model->AddLimb(treeRoot, ndSharedPtr<ndBody>(thighNd), hipPtr);
    auto* calfNode_ = m_model->AddLimb(thighNode_, ndSharedPtr<ndBody>(calfNd), kneePtr);
    auto* heelNode_ = m_model->AddLimb(calfNode_, ndSharedPtr<ndBody>(heelNd), heelPtr);
    auto* contactNode_ = m_model->AddLimb(heelNode_, ndSharedPtr<ndBody>(contactNd), contactPtr);
    (void)contactNode_;

    // ── IK effector — close loop ──────────────────────────────────────────────
    // Frame expressed in torso-local space; X = effector pin (use heelDir in local)
    ndMatrix torsoInv = torsoMat.OrthoInverse();
    ndVector contactLocal = torsoInv.TransformVector(contactCentre);
    contactLocal.m_w = 1.0f;
    ndMatrix effectorFrame = ndGramSchmidtMatrix(torsoInv.TransformVector(heelDir).Normalize());
    effectorFrame.m_posit = contactLocal;

    // TODO: Wrong constructor call!
    // auto effectorPtr = ndSharedPtr<ndJointBilateralConstraint>(new ndIkSwivelPositionEffector(effectorFrame, contactNd, m_model->torsoNd));
    // m_model->effectors[legIndex] = static_cast<ndIkSwivelPositionEffector*>(effectorPtr.operator->());
    // m_model->AddCloseLoop(effectorPtr);

    // ── Gait init ─────────────────────────────────────────────────────────────
    m_model->defaultFootLocal[legIndex] = defaultFootLocal(legIndex);

    LegGaitState& gs = m_model->legGait[legIndex];
    gs.restPos = contactCentre;
    gs.restPos.m_w = 0.0f;
    gs.phase = LegPhase::onGround;
    gs.cycleTime = legIndex * WALK_DURATION * PHASE_SHIFT;

    // ── Ogre wrappers ─────────────────────────────────────────────────────────
    m_legWrappers[legIndex].thigh = wrapBody(m_world, thighNd, sceneManager, thighNode);
    m_legWrappers[legIndex].calf = wrapBody(m_world, calfNd, sceneManager, calfNode);
    m_legWrappers[legIndex].heel = wrapBody(m_world, heelNd, sceneManager, heelNode);
    m_legWrappers[legIndex].contact = wrapBody(m_world, contactNd, nullptr);
}

void SpiderBody::create(Ogre::SceneManager* sceneManager, Ogre::SceneNode* torsoNode, const std::array<Ogre::SceneNode*, 12>& legNodes, const ndMatrix& spawnMatrix)
{
    if (m_created)
    {
        return;
    }

    buildTorso(spawnMatrix, sceneManager, torsoNode);

    for (int i = 0; i < 4; ++i)
    {
        buildLeg(i, sceneManager, legNodes[i * 3 + 0], legNodes[i * 3 + 1], legNodes[i * 3 + 2]);
    }

    // ndWorld::AddModel calls AddBodiesAndJointsToWorld() internally
    m_world->getNewtonWorld()->AddModel(m_model);

    m_created = true;
}

void SpiderBody::setStride(float s)
{
    if (m_notify)
    {
        m_notify->setStride(static_cast<ndFloat32>(s));
    }
}

void SpiderBody::setOmega(float o)
{
    if (m_notify)
    {
        m_notify->setOmega(static_cast<ndFloat32>(o));
    }
}

void SpiderBody::setPosition(const Ogre::Vector3& pos)
{
    if (!m_created)
    {
        return;
    }
    ndMatrix mat = m_model->torsoNd->GetMatrix();
    mat.m_posit = ndVector(pos.x, pos.y, pos.z, 1.0f);
    m_model->SetTransform(mat); // moves all bodies maintaining relative poses
}
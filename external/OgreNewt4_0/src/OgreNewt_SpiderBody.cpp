/* OgreNewt_SpiderBody.cpp
 *
 * Procedural gait ported from ndQuadSpiderPlayer / ndProdeduralGaitGenerator.
 * 2-link analytical IK replaces the Newton IK effector joints.
 * All positions are in TORSO-LOCAL space throughout the gait system; they are
 * transformed to world space only at the end of computeLegTransforms().
 */

#include "OgreNewt_Stdafx.h"
#include "OgreNewt_SpiderBody.h"
#include "OgreNewt_World.h"
#include "ndNewton.h"

#include <algorithm>
#include <cmath>

namespace OgreNewt
{
    // =========================================================================
    // SpiderMovementManipulation
    // =========================================================================
    SpiderMovementManipulation::SpiderMovementManipulation() : m_stride(0.0f), m_omega(0.0f), m_bodySwayX(0.0f), m_bodySwayZ(0.0f)
    {
    }

    void SpiderMovementManipulation::setStride(Ogre::Real v)
    {
        m_stride = v;
    }
    Ogre::Real SpiderMovementManipulation::getStride() const
    {
        return m_stride;
    }
    void SpiderMovementManipulation::setOmega(Ogre::Real v)
    {
        m_omega = v;
    }
    Ogre::Real SpiderMovementManipulation::getOmega() const
    {
        return m_omega;
    }
    void SpiderMovementManipulation::setBodySwayX(Ogre::Real v)
    {
        m_bodySwayX = v;
    }
    Ogre::Real SpiderMovementManipulation::getBodySwayX() const
    {
        return m_bodySwayX;
    }
    void SpiderMovementManipulation::setBodySwayZ(Ogre::Real v)
    {
        m_bodySwayZ = v;
    }
    Ogre::Real SpiderMovementManipulation::getBodySwayZ() const
    {
        return m_bodySwayZ;
    }

    // =========================================================================
    // SpiderBody – constructor / destructor
    // =========================================================================
    SpiderBody::SpiderBody(OgreNewt::World* world, Ogre::SceneManager* sceneManager, OgreNewt::CollisionPtr collision, Ogre::Real mass, const Ogre::Vector3& massOrigin, const Ogre::Vector3& /*gravity*/, const Ogre::Vector3& defaultDirection,
        SpiderCallback* callback) :
        Body(world, sceneManager, collision),
        m_callback(callback),
        m_defaultDirection(defaultDirection),
        m_walkCycleDuration(2.0f),
        m_stepHeight(0.2f),
        m_timeAcc(0.0f),
        m_canMove(true)
    {
        ndBodyDynamic* ndBody = static_cast<ndBodyDynamic*>(m_body);
        ndShapeInstance& shapeInst = ndBody->GetCollisionShape();
        ndBody->SetMassMatrix(static_cast<ndFloat32>(mass), shapeInst);

        if (massOrigin != Ogre::Vector3::ZERO)
        {
            ndBody->SetCentreOfMass(ndVector(static_cast<ndFloat32>(massOrigin.x), static_cast<ndFloat32>(massOrigin.y), static_cast<ndFloat32>(massOrigin.z), ndFloat32(1.0f)));
        }
    }

    SpiderBody::~SpiderBody()
    {
        delete m_callback;
        m_callback = nullptr;
    }

    // =========================================================================
    // Leg registration
    // =========================================================================
    void SpiderBody::addLegChain(Ogre::SceneNode* thigh, Ogre::SceneNode* calf, Ogre::SceneNode* heel, const Ogre::Vector3& hipLocalPos, const Ogre::Vector3& footRestLocal, Ogre::Real thighLength, Ogre::Real calfLength, Ogre::Real swivelSign,
        int boneAxis)
    {
        LegChainInfo leg;
        leg.thigh = thigh;
        leg.calf = calf;
        leg.heel = heel;
        leg.hipLocalPos = hipLocalPos;
        leg.footRestLocal = footRestLocal;
        leg.thighLength = std::max(thighLength, 0.05f);
        leg.calfLength = std::max(calfLength, 0.05f);
        leg.swivelSign = (swivelSign >= 0.0f) ? 1.0f : -1.0f;
        leg.boneAxis = Ogre::Math::Clamp(boneAxis, 0, 2);
        m_legs.push_back(leg);

        // Initialise gait state with the heel's rest position.
        LegGaitState gs;
        gs.base = footRestLocal;
        gs.posit = footRestLocal;
        gs.start = footRestLocal;
        gs.end = footRestLocal;
        gs.a0 = footRestLocal.y;
        gs.a1 = 0.0f;
        gs.a2 = 0.0f;
        gs.time = 0.0f;
        gs.maxTime = 1.0f;
        m_gait.push_back(gs);

        // Ensure the gait sequence vector has an entry for this leg.
        const int idx = static_cast<int>(m_legs.size()) - 1;
        if (static_cast<int>(m_gaitSequence.size()) <= idx)
        {
            m_gaitSequence.push_back(idx);
        }
    }

    void SpiderBody::clearLegs()
    {
        m_legs.clear();
        m_gait.clear();
        m_gaitSequence.clear();
        m_timeAcc = 0.0f;
    }

    // =========================================================================
    // Gait phase query  (mirrors ndProdeduralGaitGenerator::GetState)
    // =========================================================================
    SpiderBody::GaitPhase SpiderBody::getGaitPhase(int legIndex, Ogre::Real timestep) const
    {
        // Phase offset from gait sequence: each entry is 0..legCount-1.
        // We use the same 0.25-per-slot convention as ndQuadSpiderPlayer.
        const int seqVal = (legIndex < static_cast<int>(m_gaitSequence.size())) ? m_gaitSequence[legIndex] : legIndex;

        const Ogre::Real sequenceOffset = 0.25f * static_cast<Ogre::Real>(seqVal);
        const Ogre::Real t0 = std::fmod(m_timeAcc + sequenceOffset * m_walkCycleDuration, m_walkCycleDuration);
        const Ogre::Real t1 = t0 + timestep;

        // Air phase: [0 .. 25% of cycle]
        const Ogre::Real airEnd = 0.25f * m_walkCycleDuration;
        const Ogre::Real cycleEnd = m_walkCycleDuration;

        if (t0 <= airEnd)
        {
            return (t1 > airEnd) ? GaitPhase::airToGround : GaitPhase::onAir;
        }
        // Ground phase: [25% .. 100%]
        if (t0 <= cycleEnd)
        {
            return (t1 > cycleEnd) ? GaitPhase::groundToAir : GaitPhase::onGround;
        }
        return GaitPhase::onGround; // fallback (shouldn't happen)
    }

    // =========================================================================
    // Leg integration  (mirrors ndProdeduralGaitGenerator::IntegrateLeg)
    // All positions in TORSO-LOCAL space.
    // =========================================================================
    void SpiderBody::integrateLeg(int i, Ogre::Real timestep)
    {
        LegGaitState& gs = m_gait[i];
        const Ogre::Real stride = m_manip.m_stride;
        const Ogre::Real omega = m_manip.m_omega;

        const Ogre::Real airDuration = 0.25f * m_walkCycleDuration;
        const Ogre::Real groundDuration = 0.75f * m_walkCycleDuration;

        switch (getGaitPhase(i, timestep))
        {
        // ── Lift-off transition: foot leaves ground, swing phase begins ──────
        case GaitPhase::groundToAir:
        {
            // Target = rest position + half-stride forward.
            // Forward is along m_defaultDirection in chassis-local space.
            Ogre::Vector3 target = gs.base;
            target += m_defaultDirection * (stride * 0.5f);

            gs.start = gs.posit;
            gs.end = target;
            gs.maxTime = airDuration;
            gs.time = 0.0f;

            // Parabolic arch coefficients (quadratic in normalised time):
            //   y(t) = a0 + a1*t + a2*t^2   (t in [0,1])
            // Passes through y0 at t=0, y1 at t=1, peaks at h = 0.5*(y0+y1)+stepHeight.
            const Ogre::Real y0 = gs.start.y;
            const Ogre::Real y1 = gs.end.y;
            const Ogre::Real h = 0.5f * (y0 + y1) + m_stepHeight;
            gs.a0 = y0;
            gs.a1 = 4.0f * (h - y0) - (y1 - y0);
            gs.a2 = y1 - y0 - gs.a1;

            // posit unchanged this transition frame
            break;
        }

        // ── Swing (air): interpolate along parabolic arc ─────────────────────
        case GaitPhase::onAir:
        {
            gs.time += timestep;
            const Ogre::Real param = (gs.maxTime > 0.0f) ? std::min(gs.time / gs.maxTime, 1.0f) : 1.0f;

            const Ogre::Vector3 step = gs.end - gs.start;
            gs.posit = gs.start + step * param;
            gs.posit.y = gs.a0 + gs.a1 * param + gs.a2 * param * param;
            break;
        }

        // ── Touch-down transition: finish last air frame, set up ground slide ─
        case GaitPhase::airToGround:
        {
            // Finish the remaining air integration so the foot lands smoothly.
            gs.time += timestep;
            {
                const Ogre::Real param = (gs.maxTime > 0.0f) ? std::min(gs.time / gs.maxTime, 1.0f) : 1.0f;
                const Ogre::Vector3 step = gs.end - gs.start;
                gs.posit = gs.start + step * param;
                gs.posit.y = gs.a0 + gs.a1 * param + gs.a2 * param * param;
            }

            // Set up ground phase: foot target = rest - half-stride behind,
            // rotated by omega * groundDuration to account for turning.
            Ogre::Vector3 target = gs.base;
            target -= m_defaultDirection * (stride * 0.5f);

            const Ogre::Real angle = omega * groundDuration;
            const Ogre::Quaternion yawRot(Ogre::Radian(angle), Ogre::Vector3::UNIT_Y);
            target = yawRot * target;

            gs.start = gs.posit;
            gs.end = target;
            gs.maxTime = groundDuration;
            gs.time = 0.0f;
            break;
        }

        // ── Ground (stance): foot slides backward as body advances ───────────
        case GaitPhase::onGround:
        default:
        {
            gs.time += timestep;
            const Ogre::Real param = (gs.maxTime > 0.0f) ? std::min(gs.time / gs.maxTime, 1.0f) : 1.0f;
            gs.posit = gs.start + (gs.end - gs.start) * param;
            break;
        }
        }
    }

    // =========================================================================
    // IK helpers
    // =========================================================================
    Ogre::Quaternion SpiderBody::buildBoneOrient(const Ogre::Vector3& boneDir, const Ogre::Vector3& upHint, int boneAxis)
    {
        // Build an orthonormal frame whose primary column = boneDir.
        // upHint is used to anchor the "up" direction so roll stays stable.
        const Ogre::Vector3 primary = boneDir.normalisedCopy();

        // right = primary × upHint  (cross in this order keeps right pointing
        // consistently away from the body for default Y-up bones)
        Ogre::Vector3 right = primary.crossProduct(upHint);
        if (right.squaredLength() < 1e-6f)
        {
            // Degenerate (primary || upHint): pick a different reference.
            const Ogre::Vector3 alt = (std::abs(primary.x) < 0.9f) ? Ogre::Vector3::UNIT_X : Ogre::Vector3::UNIT_Z;
            right = primary.crossProduct(alt);
        }
        right.normalise();

        const Ogre::Vector3 upActual = right.crossProduct(primary).normalisedCopy();

        // Build matrix columns so column[boneAxis] = primary.
        Ogre::Matrix3 mat;
        switch (boneAxis)
        {
        default:
        case 1: // Y is the bone direction (Blender default, most common)
            mat.SetColumn(0, right);
            mat.SetColumn(1, primary);
            mat.SetColumn(2, upActual);
            break;
        case 0: // X is the bone direction
            mat.SetColumn(0, primary);
            mat.SetColumn(1, upActual);
            mat.SetColumn(2, right);
            break;
        case 2: // Z is the bone direction
            mat.SetColumn(0, upActual);
            mat.SetColumn(1, right);
            mat.SetColumn(2, primary);
            break;
        }

        Ogre::Quaternion q(mat);
        q.normalise();
        return q;
    }

    void SpiderBody::solveIK(LegChainInfo& leg, const Ogre::Vector3& chassisUp)
    {
        // thighWorldPos and footWorldPos must be set by the caller
        // before this function is invoked.
        const Ogre::Vector3& hip = leg.thighWorldPos;
        const Ogre::Real L1 = leg.thighLength;
        const Ogre::Real L2 = leg.calfLength;
        const Ogre::Real eps = 1e-4f;

        // ── Clamp target to reachable workspace ──────────────────────────────
        {
            const Ogre::Vector3 toFoot = leg.footWorldPos - hip;
            const Ogre::Real dist = toFoot.length();
            const Ogre::Real maxR = L1 + L2 - eps;
            const Ogre::Real minR = std::abs(L1 - L2) + eps;
            if (dist > maxR)
            {
                leg.footWorldPos = hip + toFoot.normalisedCopy() * maxR;
            }
            else if (dist < minR && dist > eps)
            {
                leg.footWorldPos = hip + toFoot.normalisedCopy() * minR;
            }
        }

        const Ogre::Vector3 hipToFoot = leg.footWorldPos - hip;
        const Ogre::Real d = hipToFoot.length();

        // Direction hip → foot
        const Ogre::Vector3 fwd = (d > eps) ? hipToFoot / d : m_defaultDirection;

        // ── Law of cosines: angle at hip ─────────────────────────────────────
        const Ogre::Real cosA = (d * d + L1 * L1 - L2 * L2) / (2.0f * d * L1);
        const Ogre::Real angle = std::acos(Ogre::Math::Clamp(cosA, -1.0f, 1.0f));

        // ── Swivel / bend direction ───────────────────────────────────────────
        // Project out the forward component from chassisUp to get the "bend plane up".
        Ogre::Vector3 bendDir = chassisUp - fwd * fwd.dotProduct(chassisUp);
        if (bendDir.squaredLength() < eps * eps)
        {
            // fwd is parallel to chassisUp: fall back to a lateral reference.
            const Ogre::Vector3 alt = (std::abs(fwd.x) < 0.9f) ? Ogre::Vector3::UNIT_X : Ogre::Vector3::UNIT_Z;
            bendDir = fwd.crossProduct(alt).crossProduct(fwd);
        }
        bendDir.normalise();
        bendDir *= leg.swivelSign;

        // ── Knee position ─────────────────────────────────────────────────────
        leg.kneeWorldPos = hip + fwd * (std::cos(angle) * L1) + bendDir * (std::sin(angle) * L1);

        // ── Build bone orientations using an up-hint for roll stability ───────
        const Ogre::Vector3& upHint = chassisUp;

        {
            const Ogre::Vector3 dir = (leg.kneeWorldPos - hip).normalisedCopy();
            leg.thighWorldOrient = buildBoneOrient(dir, upHint, leg.boneAxis);
        }
        {
            const Ogre::Vector3 dir = (leg.footWorldPos - leg.kneeWorldPos).normalisedCopy();
            leg.calfWorldOrient = buildBoneOrient(dir, upHint, leg.boneAxis);
        }
        // Heel orientation follows calf (foot lies flat, no terrain alignment needed
        // for a purely visual rig; can be overridden later for terrain adaptation).
        leg.heelWorldOrient = leg.calfWorldOrient;
    }

    // =========================================================================
    // computeLegTransforms  (logic thread, once per frame)
    // =========================================================================
    void SpiderBody::computeLegTransforms(Ogre::Real dt)
    {
        if (m_legs.empty())
        {
            return;
        }

        const Ogre::Vector3 chassisPos = this->getPosition();
        const Ogre::Quaternion chassisOrient = this->getOrientation();
        const Ogre::Vector3 chassisUp = chassisOrient * Ogre::Vector3::UNIT_Y;

        // ── Advance gait time accumulator (mirrors CalculateTime) ─────────────
        // Compute next time first, process legs with the current accumulator,
        // then commit the new value – matching ndProdeduralGaitGenerator::CalculatePose.
        const Ogre::Real nextTimeAcc = [&]()
        {
            const Ogre::Real angle0 = m_timeAcc * Ogre::Math::TWO_PI / m_walkCycleDuration;
            const Ogre::Real deltaAngle = dt * Ogre::Math::TWO_PI / m_walkCycleDuration;
            Ogre::Real angle1 = angle0 + deltaAngle;
            while (angle1 > Ogre::Math::TWO_PI)
            {
                angle1 -= Ogre::Math::TWO_PI;
            }
            while (angle1 < 0.0f)
            {
                angle1 += Ogre::Math::TWO_PI;
            }
            return angle1 * m_walkCycleDuration / Ogre::Math::TWO_PI;
        }();

        // ── Body sway offset in chassis-local space ───────────────────────────
        // Mirrors ndBodySwingControl applying m_x / m_z offset to each effector.
        Ogre::Vector3 swayOffset;
        {
            std::lock_guard<std::mutex> lock(m_manipMutex);
            swayOffset = Ogre::Vector3(-m_cachedManip.bodySwayX, 0.0f, -m_cachedManip.bodySwayZ);
        }

        // ── Process each leg ──────────────────────────────────────────────────
        // IK is solved in world space (required for correct geometry), then the
        // resulting positions and orientations are converted to torso-local space
        // before storage.  Callers (e.g. updateNodeTransform on child nodes) receive
        // parent-relative values directly – no extra conversion needed.
        const Ogre::Quaternion chassisOrientInv = chassisOrient.Inverse();
        const int legCount = static_cast<int>(m_legs.size());
        for (int i = 0; i < legCount; ++i)
        {
            LegChainInfo& leg = m_legs[i];
            LegGaitState& gs = m_gait[i];

            // Gait tick (updates gs.posit in torso-local space)
            integrateLeg(i, dt);

            // Hip world position (chassis-local hip shifted by sway, then to world)
            leg.thighWorldPos = chassisPos + chassisOrient * (leg.hipLocalPos + swayOffset);
            leg.footWorldPos  = chassisPos + chassisOrient * gs.posit;

            // Foot world position (gait posit is torso-local)
            solveIK(leg, chassisUp);

            // Convert world results to torso-local (parent-relative for child scene nodes).
            leg.thighWorldPos  = chassisOrientInv * (leg.thighWorldPos  - chassisPos);
            leg.kneeWorldPos   = chassisOrientInv * (leg.kneeWorldPos   - chassisPos);
            leg.footWorldPos   = chassisOrientInv * (leg.footWorldPos   - chassisPos);
            leg.thighWorldOrient = chassisOrientInv * leg.thighWorldOrient;
            leg.calfWorldOrient  = chassisOrientInv * leg.calfWorldOrient;
            leg.heelWorldOrient  = chassisOrientInv * leg.heelWorldOrient;
        }

        // Commit the advanced time accumulator AFTER processing all legs.
        m_timeAcc = nextTimeAcc;
    }

    // =========================================================================
    // applyLocomotionImpulses  (Newton worker thread)
    // Mirrors ndController::Update impulse logic from ndBasicModel.
    // =========================================================================
    void SpiderBody::applyLocomotionImpulses(Ogre::Real dt)
    {
        if (!m_canMove)
        {
            return;
        }

        // Read cached values – written by main thread, never call Lua here.
        CachedManip cached;
        {
            std::lock_guard<std::mutex> lock(m_manipMutex);
            cached = m_cachedManip;
        }

        if (!isOnGround())
        {
            return;
        }

        ndBodyDynamic* ndBody = static_cast<ndBodyDynamic*>(m_body);
        const Ogre::Quaternion orient = this->getOrientation();
        const Ogre::Vector3 forward = orient * m_defaultDirection;
        const Ogre::Vector3 up = orient * Ogre::Vector3::UNIT_Y;
        const Ogre::Vector3 vel = this->getVelocity();

        Ogre::Real mass;
        Ogre::Vector3 inertia;
        this->getMassMatrix(mass, inertia);

        // Forward drive
        if (std::abs(cached.stride) > 0.001f)
        {
            const Ogre::Real targetSpeed = cached.stride * 4.0f;
            const Ogre::Real currentSpeed = forward.dotProduct(vel);
            const Ogre::Real deltaSpeed = targetSpeed - currentSpeed;
            const Ogre::Vector3 driveImpulse = forward * (deltaSpeed * mass * 0.3f);

            ndBody->ApplyImpulsePair(ndVector(driveImpulse.x, driveImpulse.y, driveImpulse.z, 0.0f), ndVector::m_zero, static_cast<ndFloat32>(dt));
        }
        else
        {
            const Ogre::Real fwdSpd = forward.dotProduct(vel);
            if (std::abs(fwdSpd) > 0.01f)
            {
                const Ogre::Vector3 brakeImpulse = -forward * (fwdSpd * mass * 0.5f * dt);
                ndBody->ApplyImpulsePair(ndVector(brakeImpulse.x, brakeImpulse.y, brakeImpulse.z, 0.0f), ndVector::m_zero, static_cast<ndFloat32>(dt));
            }
        }

        // Lateral grip
        {
            const Ogre::Vector3 lateral = forward.crossProduct(up);
            const Ogre::Real latSpd = lateral.dotProduct(vel);
            const Ogre::Vector3 gripImpulse = -lateral * (latSpd * mass * 0.8f);
            ndBody->ApplyImpulsePair(ndVector(gripImpulse.x, gripImpulse.y, gripImpulse.z, 0.0f), ndVector::m_zero, static_cast<ndFloat32>(dt));
        }

        // Yaw
        if (std::abs(cached.omega) > 0.001f)
        {
            const Ogre::Vector3 omega = this->getOmega();
            const Ogre::Real currentYaw = up.dotProduct(omega);
            const Ogre::Real deltaYaw = cached.omega - currentYaw;
            const Ogre::Real inertiaDotUp = inertia.dotProduct(Ogre::Vector3(std::abs(up.x), std::abs(up.y), std::abs(up.z)));
            const Ogre::Vector3 yawImpulse = up * (deltaYaw * inertiaDotUp * 0.5f);

            ndBody->ApplyImpulsePair(ndVector::m_zero, ndVector(yawImpulse.x, yawImpulse.y, yawImpulse.z, 0.0f), static_cast<ndFloat32>(dt));
        }
    }

    // =========================================================================
    // isOnGround  (mirrors VehicleV2::isOnGround)
    // =========================================================================
    bool SpiderBody::isOnGround() const
    {
        const ndBodyKinematic* ndBody = static_cast<const ndBodyKinematic*>(m_body);
        if (!ndBody)
        {
            return false;
        }
        ndBodyKinematic::ndContactMap::Iterator it(const_cast<ndBodyKinematic*>(ndBody)->GetContactMap());
        for (it.Begin(); it; it++)
        {
            const ndContact* c = it.GetNode()->GetInfo();
            if (c && c->IsActive())
            {
                return true;
            }
        }
        return false;
    }

    // New method – called from main thread
    void SpiderBody::updateMovementInput(Ogre::Real dt)
    {
        if (!m_callback)
        {
            return;
        }

        // Reset manip, call the Lua callback on the MAIN THREAD.
        m_manip.m_stride = 0.0f;
        m_manip.m_omega = 0.0f;
        m_manip.m_bodySwayX = 0.0f;
        m_manip.m_bodySwayZ = 0.0f;

        m_callback->onMovementChanged(&m_manip, dt);

        // Cache the result so the Newton thread can read it safely.
        {
            std::lock_guard<std::mutex> lock(m_manipMutex);
            m_cachedManip.stride = m_manip.m_stride;
            m_cachedManip.omega = m_manip.m_omega;
            m_cachedManip.bodySwayX = m_manip.m_bodySwayX;
            m_cachedManip.bodySwayZ = m_manip.m_bodySwayZ;
        }
    }

    // =========================================================================
    // Setters / getters
    // =========================================================================
    const std::vector<LegChainInfo>& SpiderBody::getLegChains() const
    {
        return m_legs;
    }

    const std::vector<LegGaitState>& SpiderBody::getGaitStates() const
    {
        return m_gait;
    }

    void SpiderBody::setCanMove(bool v)
    {
        m_canMove = v;
    }

    bool SpiderBody::getCanMove() const
    {
        return m_canMove;
    }

    void SpiderBody::setWalkCycleDuration(Ogre::Real d)
    {
        m_walkCycleDuration = std::max(0.1f, d);
    }

    Ogre::Real SpiderBody::getWalkCycleDuration() const
    {
        return m_walkCycleDuration;
    }

    void SpiderBody::setStepHeight(Ogre::Real h)
    {
        m_stepHeight = h;
    }

    Ogre::Real SpiderBody::getStepHeight() const
    {
        return m_stepHeight;
    }

    void SpiderBody::setGaitSequence(const std::vector<int>& seq)
    {
        m_gaitSequence = seq;
    }

    const std::vector<int>& SpiderBody::getGaitSequence() const
    {
        return m_gaitSequence;
    }

} // namespace OgreNewt
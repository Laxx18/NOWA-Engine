/* Copyright (c) <2003-2022> <Newton Game Dynamics>
 *
 * OgreNewt4 impulse-based vehicle implementation.
 */

#include "OgreNewt_Stdafx.h"
#include "OgreNewt_VehicleV2.h"
#include "OgreNewt_World.h"

// Newton Dynamics 4 core – adjust path for your build
#include "ndNewton.h"

namespace OgreNewt
{

    // =============================================================================
    // VehicleDrivingManipulationV2
    // =============================================================================
    VehicleDrivingManipulationV2::VehicleDrivingManipulationV2() : m_steerAngle(0.0f), m_motorForce(0.0f), m_handBrake(0.0f), m_brake(0.0f)
    {
    }

    void VehicleDrivingManipulationV2::setSteerAngle(Ogre::Real v)
    {
        m_steerAngle = v;
    }
    Ogre::Real VehicleDrivingManipulationV2::getSteerAngle() const
    {
        return m_steerAngle;
    }

    void VehicleDrivingManipulationV2::setMotorForce(Ogre::Real v)
    {
        m_motorForce = v;
    }
    Ogre::Real VehicleDrivingManipulationV2::getMotorForce() const
    {
        return m_motorForce;
    }

    void VehicleDrivingManipulationV2::setHandBrake(Ogre::Real v)
    {
        m_handBrake = v;
    }
    Ogre::Real VehicleDrivingManipulationV2::getHandBrake() const
    {
        return m_handBrake;
    }

    void VehicleDrivingManipulationV2::setBrake(Ogre::Real v)
    {
        m_brake = v;
    }
    Ogre::Real VehicleDrivingManipulationV2::getBrake() const
    {
        return m_brake;
    }

    // =============================================================================
    // VehicleV2 – constructor / destructor
    // =============================================================================
    VehicleV2::VehicleV2(OgreNewt::World* world, Ogre::SceneManager* sceneManager, OgreNewt::CollisionPtr collision, Ogre::Real mass, const Ogre::Vector3& massOrigin, const Ogre::Vector3& gravity, const Ogre::Vector3& defaultDirection,
        VehicleV2Callback* callback) :
        Body(world, sceneManager, collision),
        m_defaultDirection(defaultDirection),
        m_callback(callback),
        m_canDrive(true),
        m_vehicleForce(Ogre::Vector3::ZERO),
        m_currentSteerAngle(0.0f),
        m_tireDirectionSwap(false),
        m_steeringStrength(1.0f),
        m_spinAxis(0),
        m_frontAxleLocalX(0.0f),
        m_rearAxleLocalX(0.0f),
        m_frontTireCount(0),
        m_rearTireCount(0)
    {
        // ── Mass / inertia ────────────────────────────────────────────────────────
        // m_body is the protected ndBodyDynamic* in OgreNewt::Body.
        ndBodyDynamic* ndBody = static_cast<ndBodyDynamic*>(m_body);

        ndShapeInstance& shapeInst = ndBody->GetCollisionShape();

        // Use the same overload as ndBasicModel:
        //   SetMassMatrix(mass, shapeInstance)
        // This lets Newton derive Ixx/Iyy/Izz directly from the collision geometry.
        //
        // The older two-step approach (CalculateInertia() → SetMassMatrix(mass, matrix))
        // is WRONG: CalculateInertia() returns a unit-mass matrix, so passing it together
        // with `mass` leaves the inertia un-scaled, producing near-zero rotational
        // resistance and causing the vehicle to tip over at the first impulse.
        ndBody->SetMassMatrix(static_cast<ndFloat32>(mass), shapeInst);

        // ── Centre of mass ────────────────────────────────────────────────────────
        if (massOrigin != Ogre::Vector3::ZERO)
        {
            ndVector com(static_cast<ndFloat32>(massOrigin.x), static_cast<ndFloat32>(massOrigin.y), static_cast<ndFloat32>(massOrigin.z), ndFloat32(1.0f));

            ndBody->SetCentreOfMass(com);
        }
    }

    VehicleV2::~VehicleV2()
    {
        delete m_callback;
        m_callback = nullptr;
    }

    // =============================================================================
    // Tire management
    // =============================================================================
    void VehicleV2::addTire(Ogre::SceneNode* sceneNode, const Ogre::Vector3& localPos, const Ogre::Quaternion& localOrient, Ogre::Real radius, bool isLeft)
    {
        TireInfoV2 tire;
        tire.sceneNode = sceneNode;
        tire.localPos = localPos;
        tire.localOrient = localOrient;
        tire.spinAngle = 0.0f;
        tire.worldPos = Ogre::Vector3::ZERO;
        tire.worldOrient = Ogre::Quaternion::IDENTITY;

        // Uniform invRadius – NO left/right distinction.
        //
        // The spin quaternion is applied in CHASSIS space (BEFORE localOrient) around
        // UNIT_Z (the wheel axle direction in chassis frame).  Because the spin is
        // independent of the tire's bind-pose rotation, both the left and right tires
        // on an axle roll in the same direction and need the same sign.
        //
        // Rotation by +angle around +Z:  contact patch (-Y) moves toward +X (forward) ✓
        //
        // TireDirectionSwap negates this for meshes whose bind pose is flipped.
        const Ogre::Real r = std::max(std::abs(radius), 0.05f);
        tire.invRadius = m_tireDirectionSwap ? (-1.0f / r) : (1.0f / r);

        // Front tires are those whose local-space position lies ahead of the
        // chassis origin along the vehicle's default forward direction.
        tire.isFrontTire = localPos.dotProduct(m_defaultDirection) > 0.0f;

        // Track average axle offsets (used by applyTurningImpulse / applyLateralImpulse
        // to apply forces at the correct positions and shift the pivot to the rear axle).
        const Ogre::Real axleX = localPos.dotProduct(m_defaultDirection);
        if (tire.isFrontTire)
        {
            m_frontAxleLocalX = (m_frontAxleLocalX * m_frontTireCount + axleX) / (m_frontTireCount + 1);
            ++m_frontTireCount;
        }
        else
        {
            m_rearAxleLocalX = (m_rearAxleLocalX * m_rearTireCount + axleX) / (m_rearTireCount + 1);
            ++m_rearTireCount;
        }

        m_tires.push_back(tire);
    }

    void VehicleV2::clearTires()
    {
        m_tires.clear();
        m_frontAxleLocalX = 0.0f;
        m_rearAxleLocalX = 0.0f;
        m_frontTireCount = 0;
        m_rearTireCount = 0;
    }

    void VehicleV2::setTireDirectionSwap(bool swap)
    {
        if (m_tireDirectionSwap == swap)
        {
            return;
        }
        m_tireDirectionSwap = swap;
        // Flip the sign of every already-registered tire so live editing works.
        for (TireInfoV2& tire : m_tires)
        {
            tire.invRadius = -tire.invRadius;
        }
    }

    bool VehicleV2::getTireDirectionSwap() const
    {
        return m_tireDirectionSwap;
    }

    void VehicleV2::setSteeringStrength(Ogre::Real strength)
    {
        // Clamp to a sane range; negative values would invert steering direction.
        m_steeringStrength = std::max(0.1f, strength);
    }

    Ogre::Real VehicleV2::getSteeringStrength() const
    {
        return m_steeringStrength;
    }

    void VehicleV2::setSpinAxis(int axis)
    {
        // 0 = X, 1 = Y, 2 = Z
        m_spinAxis = Ogre::Math::Clamp(axis, 0, 2);
    }

    int VehicleV2::getSpinAxis() const
    {
        return m_spinAxis;
    }

    // ── NEW: main-thread counterpart of SpiderBody::updateMovementInput ──────────
    void VehicleV2::updateVehicleInput(Ogre::Real dt)
    {
        if (!m_callback)
        {
            return;
        }

        // Reset, call Lua on the MAIN THREAD (mirrors spider's updateMovementInput).
        m_manip.m_steerAngle = 0.0f;
        m_manip.m_motorForce = 0.0f;
        m_manip.m_handBrake = 0.0f;
        m_manip.m_brake = 0.0f;

        m_callback->onSteerAngleChanged(&m_manip, dt);
        m_callback->onMotorForceChanged(&m_manip, dt);
        m_callback->onHandBrakeChanged(&m_manip, dt);
        m_callback->onBrakeChanged(&m_manip, dt);

        // Cache for the Newton worker thread (mirrors spider's lock_guard block).
        {
            std::lock_guard<std::mutex> lock(m_inputMutex);
            m_cachedInputs.steer = m_manip.m_steerAngle;
            m_cachedInputs.motor = m_manip.m_motorForce;
            m_cachedInputs.handBrake = m_manip.m_handBrake;
            m_cachedInputs.brake = m_manip.m_brake;
        }
    }

    // ── ApplyImpulses — read cache only, never call Lua ────────────────
    void VehicleV2::applyImpulses(Ogre::Real dt)
    {
        if (!m_canDrive || !m_callback)
        {
            return;
        }

        // Read cached values — written by main thread (mirrors spider's applyLocomotionImpulses).
        CachedInputs cached;
        {
            std::lock_guard<std::mutex> lock(m_inputMutex);
            cached = m_cachedInputs;
        }

        m_currentSteerAngle = Ogre::Degree(cached.steer).valueRadians();

        if (isOnGround())
        {
            applyLongitudinalImpulse(cached.motor, cached.brake, cached.handBrake, dt);
            applyLateralImpulse(cached.handBrake, dt);
            applyTurningImpulse(cached.steer, cached.motor, dt);
        }
    }

    std::vector<TireInfoV2>& VehicleV2::getTires()
    {
        return m_tires;
    }

    const std::vector<TireInfoV2>& VehicleV2::getTires() const
    {
        return m_tires;
    }

    // =============================================================================
    // computeTireTransforms  (logic thread, once per frame)
    // =============================================================================
    void VehicleV2::computeTireTransforms(Ogre::Real dt)
    {
        if (m_tires.empty())
        {
            return;
        }

        const Ogre::Vector3 chassisPos = this->getPosition();
        const Ogre::Quaternion chassisOrient = this->getOrientation();
        const Ogre::Vector3 velocity = this->getVelocity();

        // Forward speed along the vehicle's configured forward axis.
        const Ogre::Vector3 forward = chassisOrient * m_defaultDirection;
        const Ogre::Real speed = forward.dotProduct(velocity);
        const Ogre::Real step = speed * dt;

        // Steer quaternion around chassis +Y, applied in chassis space before localOrient.
        // m_currentSteerAngle is the raw Degree→Radian conversion of steerDeg:
        //   steerDeg < 0 (right turn) → negative rad → Ry(negative) takes +X toward +Z → tire faces right ✓
        //   steerDeg > 0 (left  turn) → positive rad → Ry(positive) takes +X toward -Z → tire faces left  ✓
        const Ogre::Quaternion steerQ(Ogre::Radian(m_currentSteerAngle), Ogre::Vector3::UNIT_Y);

        for (TireInfoV2& tire : m_tires)
        {
            if (!tire.sceneNode)
            {
                continue;
            }

            // ── Accumulate spin ───────────────────────────────────────────────────
            tire.spinAngle += step * tire.invRadius;

            // Keep angle in [-PI, PI] to avoid float drift over time
            const Ogre::Real twoPi = Ogre::Math::TWO_PI;
            while (tire.spinAngle > Ogre::Math::PI)
            {
                tire.spinAngle -= twoPi;
            }
            while (tire.spinAngle < -Ogre::Math::PI)
            {
                tire.spinAngle += twoPi;
            }

            // ── Build world transform ─────────────────────────────────────────────
            // Spin axis is configurable (m_spinAxis: 0=X, 1=Y, 2=Z).
            // spinQ is placed BEFORE localOrient so the bind-pose orientation of
            // the mesh does NOT affect spin direction — front and rear tires behave
            // identically regardless of artist mirroring.
            //
            // Default is X.  Switch to Y or Z in the editor (TireSpinAxis) if the
            // tires flip or spin on the wrong axis.
            // Use TireDirectionSwap if the axis is correct but the direction is reversed.
            //
            // Final world orientation = chassisOrient * [steerQ] * spinQ * localOrient
            static const Ogre::Vector3 spinAxes[3] = {Ogre::Vector3::UNIT_X, Ogre::Vector3::UNIT_Y, Ogre::Vector3::UNIT_Z};
            const Ogre::Quaternion spinQ(Ogre::Radian(tire.spinAngle), spinAxes[m_spinAxis]);

            tire.worldPos = chassisPos + chassisOrient * tire.localPos;

            if (tire.isFrontTire)
            {
                tire.worldOrient = chassisOrient * steerQ * spinQ * tire.localOrient;
            }
            else
            {
                tire.worldOrient = chassisOrient * spinQ * tire.localOrient;
            }
        }
    }

    // =============================================================================
    // Control / state queries
    // =============================================================================
    void VehicleV2::setCanDrive(bool v)
    {
        m_canDrive = v;
    }
    bool VehicleV2::getCanDrive() const
    {
        return m_canDrive;
    }

    bool VehicleV2::isOnGround() const
    {
        const ndBodyKinematic* ndBody = static_cast<const ndBodyKinematic*>(m_body);
        if (!ndBody)
        {
            return false;
        }

        ndBodyKinematic::ndContactMap::Iterator it(const_cast<ndBodyKinematic*>(ndBody)->GetContactMap());
        for (it.Begin(); it; it++)
        {
            const ndContact* contact = it.GetNode()->GetInfo();
            if (contact && contact->IsActive())
            {
                return true;
            }
        }
        return false;
    }

    bool VehicleV2::isAirborne() const
    {
        return !isOnGround();
    }

    Ogre::Vector3 VehicleV2::getVehicleForce() const
    {
        return m_vehicleForce;
    }

    // ── Longitudinal ──────────────────────────────────────────────────────────────
    // motorForce drives the vehicle forward/backward.
    // brakeForce and handBrake cancel the forward velocity component.
    void VehicleV2::applyLongitudinalImpulse(Ogre::Real motorForce, Ogre::Real brakeForce, Ogre::Real handBrake, Ogre::Real dt)
    {
        ndBodyDynamic* ndBody = static_cast<ndBodyDynamic*>(m_body);

        const Ogre::Quaternion orient = this->getOrientation();
        const Ogre::Vector3 forward = orient * m_defaultDirection; // respects mesh orientation
        const Ogre::Vector3 vel = this->getVelocity();
        const Ogre::Real fwdSpd = forward.dotProduct(vel);

        Ogre::Real mass;
        Ogre::Vector3 inertia;
        this->getMassMatrix(mass, inertia);

        // ── Drive impulse ─────────────────────────────────────────────────────────
        // motorForce is in the same units as the old vehicle (large Newton-force
        // value × dt).  Apply directly as an impulse in the forward direction.
        Ogre::Vector3 driveImpulse = forward * motorForce;
        m_vehicleForce = driveImpulse;

        ndBody->ApplyImpulsePair(ndVector(driveImpulse.x, driveImpulse.y, driveImpulse.z, 0.0f), ndVector::m_zero, static_cast<ndFloat32>(dt));

        // ── Brake / hand-brake: cancel forward momentum ───────────────────────────
        const Ogre::Real totalBrake = brakeForce + handBrake;
        if (totalBrake > 0.0f)
        {
            // Cancel a fraction of the current forward velocity proportional to
            // the brake factor.  Clamp so we do not overshoot zero.
            const Ogre::Real cancelFraction = Ogre::Math::Clamp(totalBrake * 0.05f * dt, 0.0f, 1.0f);
            const Ogre::Vector3 brakeImpulse = -forward * fwdSpd * mass * cancelFraction;

            ndBody->ApplyImpulsePair(ndVector(brakeImpulse.x, brakeImpulse.y, brakeImpulse.z, 0.0f), ndVector::m_zero, static_cast<ndFloat32>(dt));
        }
    }

    // ── Lateral (anti-slide) ──────────────────────────────────────────────────────
    // Apply lateral grip at the REAR AXLE offset, not at the CoM.
    //
    // Applying grip only at the rear (as a linear + angular impulse pair) allows
    // the steering force at the front to push the nose sideways.  The rear acts as
    // the pivot, so the car turns around a point near the rear wheels — matching
    // how real Ackermann/front-wheel steering works.
    void VehicleV2::applyLateralImpulse(Ogre::Real handBrake, Ogre::Real dt)
    {
        ndBodyDynamic* ndBody = static_cast<ndBodyDynamic*>(m_body);

        const Ogre::Quaternion orient = this->getOrientation();
        const Ogre::Vector3 forward = orient * m_defaultDirection;
        const Ogre::Vector3 up = orient * Ogre::Vector3::UNIT_Y;
        const Ogre::Vector3 vel = this->getVelocity();
        const Ogre::Vector3 omega = this->getOmega();

        // World-space rear axle offset from CoM (will be negative along forward when rear tires
        // are behind the CoM, which is the normal case).
        const Ogre::Vector3 rearOffsetWorld = orient * (m_defaultDirection * m_rearAxleLocalX);

        // Velocity at the rear axle = CoM velocity + omega × r_rear
        const Ogre::Vector3 velAtRear = vel + omega.crossProduct(rearOffsetWorld);

        // Lateral component at the rear axle
        const Ogre::Vector3 sideVelAtRear = velAtRear - forward * forward.dotProduct(velAtRear) - up * up.dotProduct(velAtRear);

        Ogre::Real mass;
        Ogre::Vector3 inertia;
        this->getMassMatrix(mass, inertia);

        // Hand-brake reduces lateral grip to enable drifting
        const Ogre::Real cancelFactor = (handBrake > 0.0f) ? 0.3f : 0.8f;

        // Linear impulse: opposes rear lateral velocity
        const Ogre::Vector3 impulse = -sideVelAtRear * mass * cancelFactor;

        // Angular impulse: r_rear × F (makes the impulse act at the rear offset)
        const Ogre::Vector3 angImpulse = rearOffsetWorld.crossProduct(impulse);

        ndBody->ApplyImpulsePair(ndVector(impulse.x, impulse.y, impulse.z, 0.0f), ndVector(angImpulse.x, angImpulse.y, angImpulse.z, 0.0f), static_cast<ndFloat32>(dt));
    }

    // ── Yaw (turning) ─────────────────────────────────────────────────────────────
    // Applies a lateral force at the FRONT AXLE offset (not a pure CoM torque).
    //
    // Combined with the rear-axle lateral grip in applyLateralImpulse, this shifts
    // the instantaneous pivot point toward the rear axle — exactly how Ackermann /
    // front-wheel steering behaves physically.
    //
    // Gate: only turn when the vehicle has meaningful speed.  This prevents spinning
    // on the spot when the player steers without motor force (like ndBasicModel's
    // "if m_desiredSpeed == 0 return").
    void VehicleV2::applyTurningImpulse(Ogre::Real steerDeg, Ogre::Real motorForce, Ogre::Real dt)
    {
        if (steerDeg == 0.0f)
        {
            return;
        }

        if (std::abs(m_frontAxleLocalX) < 0.01f)
        {
            return; // front axle not yet known (no tires registered)
        }

        ndBodyDynamic* ndBody = static_cast<ndBodyDynamic*>(m_body);

        const Ogre::Quaternion orient = this->getOrientation();
        const Ogre::Vector3 forward = orient * m_defaultDirection;
        const Ogre::Vector3 up = orient * Ogre::Vector3::UNIT_Y;
        const Ogre::Vector3 vel = this->getVelocity();
        const Ogre::Vector3 omega = this->getOmega();

        // Forward speed — used to gate turning and for sign of reverse steering.
        const Ogre::Real speed = forward.dotProduct(vel);

        // Do NOT rotate on the spot: only steer while the vehicle is moving.
        // This matches ndBasicModel's "if desiredSpeed == 0 return" philosophy.
        if (std::abs(speed) < 0.5f)
        {
            return;
        }

        Ogre::Real mass;
        Ogre::Vector3 inertia;
        this->getMassMatrix(mass, inertia);

        // Flip steering direction when reversing (ndBasicModel convention).
        const Ogre::Real turningSign = (speed >= 0.0f) ? 1.0f : -1.0f;

        // Effective yaw rate target (rad/s).  steerDeg / 45 normalises to [-1..1].
        const Ogre::Real effectiveYawRate = turningSign * (steerDeg / 45.0f) * m_steeringStrength;

        // ── Front axle lateral kinematics ─────────────────────────────────────────
        // World-space front axle offset from CoM.
        const Ogre::Vector3 frontOffsetWorld = orient * (m_defaultDirection * m_frontAxleLocalX);

        // Velocity at front axle = CoM velocity + omega × r_front
        const Ogre::Vector3 velAtFront = vel + omega.crossProduct(frontOffsetWorld);

        // Chassis lateral direction: forward × up  (the +/- sign is consistent with
        // the rest of the impulse math — both applyLateralImpulse and here use the
        // same world vectors so cancellation / reinforcement is physically correct).
        const Ogre::Vector3 lateral = forward.crossProduct(up);

        // Current lateral speed at the front axle
        const Ogre::Real currentLatSpeedFront = lateral.dotProduct(velAtFront);

        // Target lateral speed at the front axle from yaw kinematics:
        //   v_lat_front = -omega_Y * r_front
        // (derived from omega × r_front, Z-component with r_front along forward).
        const Ogre::Real targetLatSpeedFront = -effectiveYawRate * std::abs(m_frontAxleLocalX);

        const Ogre::Real deltaLatSpeed = targetLatSpeedFront - currentLatSpeedFront;

        // Steering force at front axle.
        // Gain 0.5 is conservative — strong enough to turn without fighting
        // the rear grip.  m_steeringStrength already scales the target.
        const Ogre::Vector3 steerForce = lateral * (deltaLatSpeed * mass * 0.5f);

        // Angular component: r_front × F (makes force act at the front axle)
        const Ogre::Vector3 steerTorque = frontOffsetWorld.crossProduct(steerForce);

        ndBody->ApplyImpulsePair(ndVector(steerForce.x, steerForce.y, steerForce.z, 0.0f), ndVector(steerTorque.x, steerTorque.y, steerTorque.z, 0.0f), static_cast<ndFloat32>(dt));
    }

} // namespace OgreNewt
#include "OgreNewt_Stdafx.h"
#include "OgreNewt_Vehicle.h"
#include "OgreNewt_ContactNotify.h"

#include <algorithm>
#include <cassert>

using namespace OgreNewt;

static inline Ogre::Vector3 toO(const ndVector& v)
{
    return Ogre::Vector3(v.m_x, v.m_y, v.m_z);
}

RayCastTire::RayCastTire(OgreNewt::Body* child, OgreNewt::Body* parentBody, const Ogre::Vector3& pos, const Ogre::Vector3& pin,
    Vehicle* parent, const TireConfiguration& tireConfiguration, Ogre::Real radius)
    : m_vehicle(parent),
    m_tireConfiguration(tireConfiguration),
    m_radius(radius)
{
    m_thisBody = child;

    // Default tire definition similar to ND4 demos, but no DEMO_GRAVITY dependency.
    // You can adjust at runtime using the setters below.
    m_def.m_mass = 20.0f;
    m_def.m_springK = 1000.0f;
    m_def.m_damperC = 20.0f;
    m_def.m_regularizer = 0.1f;
    m_def.m_lowerStop = -radius * 0.1f;
    m_def.m_upperStop = radius * 0.2f;
    m_def.m_verticalOffset = 0.0f;
    m_def.m_brakeTorque = 1500.0f;
    m_def.m_handBrakeTorque = 1500.0f;
    m_def.m_steeringAngle = 35.0f * ndDegreeToRad;

    // Note: ND4 tire friction curve is handled internally (ndTireFrictionModel).
    // The joint exposes GetFrictionModel() const for reading; no public setter for stiffness.
    // If you need a tuning proxy, map your editor’s “friction” sliders to m_def.m_regularizer
    // and re-apply via tireJoint->SetInfo(m_def) at runtime.
    
    /*Ogre::Real longStiff = 10.0f * parent->getGravityScalar();
    Ogre::Real latStiff = 2.0f * longStiff;
    m_def.m_longitudinalStiffness = static_cast<ndFloat32>(longStiff);
    m_def.m_laterialStiffness = static_cast<ndFloat32>(latStiff);*/

    // ndMultiBodyVehicleTireJoint is created by Vehicle::AddTire when model is available
    m_tireJoint = nullptr;
}

RayCastTire::~RayCastTire()
{
    m_tireJoint = nullptr;
}

void RayCastTire::setSpringLength(Ogre::Real v)
{
    m_def.m_upperStop = 0.5f * static_cast<ndFloat32>(v);
    m_def.m_lowerStop = -m_def.m_upperStop;
}

Ogre::Real RayCastTire::getSpringLength() const
{
    return (m_def.m_upperStop - m_def.m_lowerStop);
}

void RayCastTire::setSpringConst(Ogre::Real v)
{
    m_def.m_springK = static_cast<ndFloat32>(v);
}

Ogre::Real RayCastTire::getSpringConst() const
{
    return m_def.m_springK;
}

void RayCastTire::setSpringDamp(Ogre::Real v)
{
    m_def.m_damperC = static_cast<ndFloat32>(v);
}

Ogre::Real RayCastTire::getSpringDamp() const
{
    return m_def.m_damperC;
}

void RayCastTire::setSteerAngleDeg(Ogre::Real angleDeg)
{
    m_steerAngleDeg = angleDeg;
    if (m_tireJoint)
    {
        const float maxDeg = 35.0f;
        float norm = std::max(-1.0f, std::min(1.0f, static_cast<float>(angleDeg) / maxDeg));
        m_tireJoint->SetSteering(norm);
    }
}

void RayCastTire::setBrakeForce(Ogre::Real normalized)
{
    m_brakeNorm = std::clamp(static_cast<float>(normalized), 0.0f, 1.0f);
    if (m_tireJoint)
    {
        m_tireJoint->SetBreak(m_brakeNorm);
    }
}

void RayCastTire::setHandBrake(Ogre::Real normalized)
{
    m_handNorm = std::clamp(static_cast<float>(normalized), 0.0f, 1.0f);
    if (m_tireJoint)
    {
        m_tireJoint->SetHandBreak(m_handNorm);
    }
}

void RayCastTire::setMotorForce(Ogre::Real normalized)
{
    m_motorNorm = std::clamp(static_cast<float>(normalized), 0.0f, 1.0f);
}

bool RayCastTire::isGrounded() const
{
    return m_grounded;
}

Ogre::Real RayCastTire::getPenetration() const
{
    return m_penetration;
}

Ogre::String RayCastTire::getTireName() const
{
    if (!m_thisBody || !m_thisBody->getOgreNode())
    {
        return Ogre::String();
    }
    return m_thisBody->getOgreNode()->getName();
}

ndMultiBodyVehicleTireJoint* RayCastTire::getTireJoint() const
{
    return m_tireJoint;
}

OgreNewt::Body* RayCastTire::getBody() const
{
    return m_thisBody;
}

ndVehicleDectriptor::ndTireDefinition RayCastTire::getDefinition() const
{
    return m_def;
}

void RayCastTire::updateGroundProbe()
{
    // Preconditions
    if (!m_vehicle || !m_vehicle->getWorld() || !m_thisBody)
    {
        m_grounded = false;
        m_penetration = 0.0f;
        return;
    }

    ndWorld* const world = m_vehicle->getWorld()->getNewtonWorld();
    ndBodyKinematic* const selfBody = m_thisBody->getNewtonBody();
    if (!world || !selfBody)
    {
        m_grounded = false;
        m_penetration = 0.0f;
        return;
    }

    // Build a ray that goes "down" along the tire/suspension axis in world space.
    // We take the tire's current transform and use its -up axis as the cast direction.
    ndMatrix tireM = selfBody->GetMatrix();

    const ndVector rayDir = tireM.m_up.Scale(-1.0f); // down
    const ndVector rayStart = tireM.m_posit;

    // Ray length: tire radius + some suspension travel (upperStop is positive, lowerStop negative)
    const ndFloat32 travel = ndFloat32(fabs(m_def.m_upperStop) + fabs(m_def.m_lowerStop));
    const ndFloat32 rayLen = ndFloat32(m_radius) + travel;
    const ndVector  rayEnd = rayStart + rayDir.Scale(rayLen);

    // Closest-hit callback that ignores the tire's own body (and optionally the chassis)
    class TireRayCast : public ndRayCastClosestHitCallback
    {
    public:
        TireRayCast(const ndBodyKinematic* self, const ndBodyKinematic* chassis)
            : m_self(self), m_chassis(chassis)
        {
        }

        ndUnsigned32 OnRayPrecastAction(const ndBody* const body, const ndShapeInstance* const) override
        {
            // skip the tire body itself and (optionally) the chassis
            if (body == m_self || body == m_chassis)
            {
                return 0;
            }
            return 1;
        }

        const ndBodyKinematic* m_self
        {
            nullptr
        };
        const ndBodyKinematic* m_chassis{ nullptr };
    };

    const ndBodyKinematic* chassisBody = m_vehicle->getChassis() ? m_vehicle->getChassis()->getNewtonBody() : nullptr;
    TireRayCast cb(selfBody, chassisBody);

    // Cast the ray
    world->RayCast(cb, rayStart, rayEnd);

    if (cb.m_param <= 1.0f)
    {
        // We have a hit. Compute contact point/normal and penetration.
        const ndVector dir = rayEnd - rayStart;
        const ndVector hitPoint = rayStart + dir.Scale(cb.m_param);
        const ndVector hitNormal = cb.m_contact.m_normal;

        // Penetration as "how much of our cast length is left" (clamped)
        const ndFloat32 hitDist = cb.m_param * rayLen;
        const ndFloat32 maxTravel = rayLen; // total cast distance
        const ndFloat32 rawPen = maxTravel - hitDist;
        m_penetration = (rawPen > 0.0f) ? rawPen : 0.0f;

        // Map hit body to OgreNewt::Body via your notify adapter
        OgreNewt::Body* ogreHit = nullptr;
        if (cb.m_contact.m_body1) // prefer dynamic body if present
        {
            if (auto* notify = dynamic_cast<ContactNotify*>(cb.m_contact.m_body1->GetNotifyCallback()))
            {
                ogreHit = notify->getOgreNewtBody();
            }
        }
        if (!ogreHit && cb.m_contact.m_body0) // fallback
        {
            if (auto* notify = dynamic_cast<ContactNotify*>(cb.m_contact.m_body0->GetNotifyCallback()))
            {
                ogreHit = notify->getOgreNewtBody();
            }
        }

        m_grounded = (ogreHit != nullptr);

        if (ogreHit && m_vehicle)
        {
            auto hitBody = cb.m_contact.m_body1 ? cb.m_contact.m_body1 : cb.m_contact.m_body0;

            if (hitBody)
            {
                ndVector massMatrix = hitBody->GetMassMatrix();
                ndFloat32 mass = massMatrix.m_w;

                if (mass > 0.0f)
                {
                    const Ogre::Vector3 contactNorm(hitNormal.m_x, hitNormal.m_y, hitNormal.m_z);
                    const Ogre::Vector3 contactPos(hitPoint.m_x, hitPoint.m_y, hitPoint.m_z);

                    // mimic ND3: use combined mass * 2 along contact normal
                    const Ogre::Real strength = -(m_vehicle->getChassis()->getMass() + mass) * 2.0f;
                    const Ogre::Vector3 pushForce = contactNorm * strength;

                    // ND3-style call (records m_vehicleForce + applies to hit body)
                    m_vehicle->ApplyForceAndTorque(ogreHit, pushForce, contactPos);
                }
            }
        }

        if (m_grounded && m_vehicle->GetVehicleCallback())
        {
            const Ogre::Vector3 contactPos(hitPoint.m_x, hitPoint.m_y, hitPoint.m_z);
            const Ogre::Vector3 contactNorm(hitNormal.m_x, hitNormal.m_y, hitNormal.m_z);

            m_vehicle->GetVehicleCallback()->onTireContact(this, getTireName(), ogreHit, contactPos, contactNorm, m_penetration);
        }
    }
    else
    {
        m_grounded = false;
        m_penetration = 0.0f;
    }
}

// ---------------- Vehicle ----------------

Vehicle::Vehicle(OgreNewt::World* world,
    Ogre::SceneManager* sceneManager,
    const Ogre::Vector3& defaultDirection,
    const OgreNewt::CollisionPtr& col,
    Ogre::Real vhmass,
    const Ogre::Vector3& massOrigin,
    const Ogre::Vector3& collisionPosition,
    VehicleCallback* vehicleCallback)
    : OgreNewt::Body(world, sceneManager, col, Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC)
    , m_vehicleCallback(vehicleCallback)
    , m_vehicleModel(nullptr)
    , m_canDrive(true)
    , m_initMassDataDone(false)
{
    // Chassis physical setup (this Vehicle *is* the Body)
    setMassMatrix(vhmass, Ogre::Vector3(1.0f, 1.0f, 1.0f)); // replace inertia with your calc if available
    setCenterOfMass(massOrigin);

    // Orient by defaultDirection (same convention you use elsewhere)
    const Ogre::Quaternion q = OgreNewt::Converters::grammSchmidt(defaultDirection);
    setPositionOrientation(collisionPosition, q);

    // --- ND4 vehicle model creation (gravity magnitude param) ---
    const ndFloat32 gravityMag = ndFloat32(this->getGravity().length()); // or hardcode 10.0f if you prefer
    m_vehicleModel = new ndMultiBodyVehicle(gravityMag);

    // Add this chassis body to the model
    ndBodyKinematic* const chassisBody = getNewtonBody();
    // ND4 expects an ndSharedPtr<ndBody>. It is safe to wrap your existing body pointer here.
    m_vehicleModel->AddChassis(ndSharedPtr<ndBody>(chassisBody));

    // Register the model with the ND4 world (so Update/PostUpdate/Debug run)
    world->getNewtonWorld()->AddModel(m_vehicleModel);

    // mild default damping (optional)
    setLinearDamping(0.1f);
    setAngularDamping(Ogre::Vector3(0.1f, 0.1f, 0.1f));
}

#if 0
Vehicle::~Vehicle()
{
    m_tires.clear();
    delete m_chassis;
    m_chassis = nullptr;
    delete m_vehicleCallback;
    m_vehicleCallback = nullptr;
}
#else
Vehicle::~Vehicle()
{
    if (m_vehicleModel)
    {
        if (auto* const ndWorld = getWorld() ? getWorld()->getNewtonWorld() : nullptr)
        {
            ndWorld->RemoveModel(m_vehicleModel);
        }
        delete m_vehicleModel;
        m_vehicleModel = nullptr;
    }
}
#endif

void Vehicle::SetRayCastMode(VehicleRaycastType)
{

}

bool Vehicle::RemoveTire(RayCastTire* tire)
{
    auto it = std::find(m_tires.begin(), m_tires.end(), tire);
    if (it != m_tires.end()) {
        m_tires.erase(it);
        m_tireCount = static_cast<int>(m_tires.size());
        return true;
    }
    return false;
}

void Vehicle::RemoveAllTires()
{
    m_tires.clear();
    m_tireCount = 0;
}

void Vehicle::AddTire(RayCastTire* tire)
{
    m_tires.push_back(tire);
    m_tireCount = static_cast<int>(m_tires.size());
    ensureVehicleModel();

    if (m_vehicleModel)
    {
        ndVehicleDectriptor::ndTireDefinition def = tire->getDefinition();
        ndBodyKinematic* ndTire = tire->m_thisBody->getNewtonBody();
        ndSharedPtr<ndBody> tirePtr(ndTire);
        tire->m_tireJoint = m_vehicleModel->AddTire(def, tirePtr);
    }
}

void Vehicle::InitMassData()
{
    // If you need to offset COM relative to collision/mass origin, compute and set here via wrapper.
    // m_chassis->setCenterOfMass(desiredCOM);
}

void Vehicle::ApplyForceAndTorque(OgreNewt::Body* vBody, const Ogre::Vector3& vForce, const Ogre::Vector3& vPoint)
{
    if (!vBody)
    {
        return;
    }

    // store for telemetry / Lua
    m_vehicleForce = vForce;

    // physically apply (your helper computes torque = (point - com) × force)
    AddForceAtPos(vBody, vForce, vPoint);
}

void Vehicle::AddForceAtPos(OgreNewt::Body* body, const Ogre::Vector3& forceWS, const Ogre::Vector3& pointWS)
{
    // τ = (r × F), r = pointWS - comWS
    ndBodyKinematic* ndB = body->getNewtonBody();
    ndMatrix m;
    m = ndB->GetMatrix();
    // COM is local-space; transform to world
    Ogre::Vector3 comLocal = body->getCenterOfMass();
    ndVector comWSv = m.TransformVector(ndVector(comLocal.x, comLocal.y, comLocal.z, 1.0f));
    Ogre::Vector3 comWS = toO(comWSv);
    Ogre::Vector3 r = pointWS - comWS;
    Ogre::Vector3 torqueWS = r.crossProduct(forceWS);

    body->addForce(forceWS);
    body->addTorque(torqueWS);
}

void Vehicle::PreUpdate(Ogre::Real timestep)
{
    if (!m_canDrive)
    {
        return;
    }

    if (!m_initMassDataDone)
    {
        InitMassData();
        m_initMassDataDone = true;
    }

    // For each RayCastTire attached to this vehicle
    for (auto* tire : m_tires)
    {
        if (!tire)
        {
            continue;
        }

        // 1️ Update steering, throttle, braking from user callback
        updateDriverInput(tire, timestep);

        // optional: lateral damping similar to ND demo
        if (auto* chassis = getChassis())
        {
            ndBodyKinematic* const body = chassis->getNewtonBody();
            ndMatrix m = body->GetMatrix();
            ndVector velocity = body->GetVelocity();

            // remove sideways component (stabilize)
            ndVector forward = m.m_front;
            ndVector up = m.m_up;
            ndVector sideVel = velocity - forward.Scale(velocity.DotProduct(forward).GetScalar()) - up.Scale(velocity.DotProduct(up).GetScalar());
            ndVector correction = sideVel.Scale(-0.8f * body->GetMassMatrix().m_w);
            body->ApplyImpulsePair(correction, ndVector(0.0f, 0.0f, 0.0f, 0.0f), static_cast<ndFloat32>(timestep));
        }

        // 2️ Perform ND4 raycast to detect ground contact and trigger onTireContact()
        tire->updateGroundProbe();

        // 3️ (Optionally) you could collect debug or suspension data here later
        // e.g. tire->getPenetration(), tire->isGrounded(), etc.
    }
}

void Vehicle::ensureVehicleModel()
{
    if (m_vehicleModel != nullptr)
    {
        return;
    }

    ndWorld* worldND = m_world->getNewtonWorld();
    m_vehicleModel = new ndMultiBodyVehicle();

    // hook chassis
    ndBodyKinematic* ndChassis = m_chassis->getNewtonBody();
    ndSharedPtr<ndBody> chassisPtr(ndChassis);
    m_vehicleModel->AddChassis(chassisPtr);

    // --- create motor joint if motor body is provided by engine ---
    if (m_motorWrapper != nullptr)
    {
        ndBodyKinematic* motorBody = m_motorWrapper->getNewtonBody();
        // Attach the motor to the *vehicle model* (correct ND4 ctor)
        m_motor = new ndMultiBodyVehicleMotor(motorBody, m_vehicleModel);
        m_motor->SetVehicleOwner(m_vehicleModel);

        // basic motor parameters
        m_motor->SetMaxRpm(m_motorMaxRpm);
        m_motor->SetOmegaAccel(m_motorOmegaAccel);
        m_motor->SetFrictionLoss(m_motorFrictionLoss);
        // initial torque (0 rpm / 0 Nm)
        m_motor->SetTorqueAndRpm(0.0f, 0.0f);
    }

    if (!m_vehicleAddedToWorld)
    {
        worldND->AddModel(ndSharedPtr<ndModel>(m_vehicleModel));
        m_vehicleAddedToWorld = true;
    }
}

void Vehicle::setMotorBody(OgreNewt::Body* motorBody)
{
    m_motorWrapper = motorBody;
}

void Vehicle::setMotorMaxRpm(Ogre::Real rpm)
{
    m_motorMaxRpm = static_cast<ndFloat32>(rpm);
    if (m_motor)
    {
        m_motor->SetMaxRpm(m_motorMaxRpm);
    }
}

void Vehicle::setMotorOmegaAccel(Ogre::Real rpmPerSec)
{
    m_motorOmegaAccel = static_cast<ndFloat32>(rpmPerSec);
    if (m_motor)
    {
        m_motor->SetOmegaAccel(m_motorOmegaAccel);
    }
}

void Vehicle::setMotorFrictionLoss(Ogre::Real newtonMeters)
{
    m_motorFrictionLoss = static_cast<ndFloat32>(newtonMeters);
    if (m_motor)
    {
        m_motor->SetFrictionLoss(m_motorFrictionLoss);
    }
}

void Vehicle::setMotorTorqueScale(Ogre::Real nmPerUnit)
{
    m_motorTorqueScale = static_cast<ndFloat32>(nmPerUnit);
}

void Vehicle::setCanDrive(bool canDrive)
{
    m_canDrive = canDrive;
}

Ogre::Vector3 Vehicle::getVehicleForce() const
{
    return m_vehicleForce;
}

void Vehicle::updateDriverInput(RayCastTire* tire, Ogre::Real timestep)
{
    if (!tire || !m_canDrive)
    {
        return;
    }

    VehicleCallback* cb = GetVehicleCallback();
    if (!cb)
    {
        return;
    }

    // Read inputs from your callback for this tire
    const Ogre::Real steerDeg = cb->onSteerAngleChanged(this, tire, timestep);
    const Ogre::Real motorIn = cb->onMotorForceChanged(this, tire, timestep);
    const Ogre::Real handIn = cb->onHandBrakeChanged(this, tire, timestep);
    const Ogre::Real brakeIn = cb->onBrakeChanged(this, tire, timestep);

    // --- Steering (only if this tire is steer-capable) ---
    if (tire->m_tireConfiguration.tireSteer == tsSteer)
    {
        // Apply steering side (invert if SteerSideB)
        const Ogre::Real sideSign = (tire->m_tireConfiguration.steerSide == tsSteerSideA) ? 1.0f : -1.0f;
        tire->setSteerAngleDeg(steerDeg * sideSign);
    }
    else
    {
        tire->setSteerAngleDeg(0.0f);
    }

    // --- Acceleration (only if this tire is a drive wheel) ---
    if (tire->m_tireConfiguration.tireAccel == tsAccel)
    {
        tire->setMotorForce(std::clamp(motorIn, 0.0f, 1.0f));
    }
    else
    {
        tire->setMotorForce(0.0f);
    }

    // --- Regular brake (only if this tire supports braking) ---
    if (tire->m_tireConfiguration.brakeMode == tsBrake)
    {
        tire->setBrakeForce(std::clamp(brakeIn, 0.0f, 1.0f));
    }
    else
    {
        tire->setBrakeForce(0.0f);
    }

    // --- Handbrake (global input; keep simple like old code) ---
    tire->setHandBrake(std::clamp(handIn, 0.0f, 1.0f));

    // --- Engine / motor joint (use MOTOR input, not handbrake) ---
    if (m_motor && tire->m_tireConfiguration.tireAccel == tsAccel)
    {
        const ndFloat32 motorNorm = static_cast<ndFloat32>(std::clamp(motorIn, 0.0f, 1.0f));
        const ndFloat32 targetRpm = motorNorm * m_motorMaxRpm;
        const ndFloat32 torqueNm = motorNorm * m_motorTorqueScale;
        m_motor->SetTorqueAndRpm(targetRpm, torqueNm);
    }
}
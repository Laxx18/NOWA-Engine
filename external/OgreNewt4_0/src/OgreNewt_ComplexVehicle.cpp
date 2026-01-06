#include "OgreNewt_Stdafx.h"
#include <algorithm>
#include "OgreNewt_ComplexVehicle.h"
#include "OgreNewt_VehicleNotify.h"

namespace OgreNewt
{
    // -------------------------------------------------------------------------
    // ComplexVehicleTire
    // -------------------------------------------------------------------------
    ComplexVehicleTire::ComplexVehicleTire(OgreNewt::Body* child, OgreNewt::Body* parentBody, const Ogre::Vector3& pos,
        const Ogre::Vector3& pin, ComplexVehicle* parentVehicle, Axle axle, Side side, Ogre::Real radius)
        : Joint(),
        m_complexVehicle(parentVehicle),
        m_axle(axle),
        m_side(side),
        m_childBody(child),
        m_parentBody(parentBody),
        m_localPos(Ogre::Vector3::ZERO),
        m_pin(pin),
        m_radius(radius),
        m_steerAngleDeg(0.0f),
        m_motorNorm(0.0f),
        m_brakeNorm(0.0f),
        m_handBrakeNorm(0.0f),
        m_tireJoint(nullptr)
    {
        if (m_childBody)
        {
            const Ogre::Vector3 bodyPos = m_childBody->getPosition();
            const Ogre::Quaternion bodyOri = m_childBody->getOrientation();
            m_localPos = bodyOri.Inverse() * (pos - bodyPos);
        }
    }

    ComplexVehicleTire::~ComplexVehicleTire(void)
    {
    }

    void ComplexVehicleTire::setVehicleTireSide(VehicleTireSide tireSide)
    {
        m_tireConfiguration.tireSide = tireSide;
    }

    VehicleTireSide ComplexVehicleTire::getVehicleTireSide(void) const
    {
        return m_tireConfiguration.tireSide;
    }

    void ComplexVehicleTire::setVehicleTireSteer(VehicleTireSteer tireSteer)
    {
        m_tireConfiguration.tireSteer = tireSteer;
    }

    VehicleTireSteer ComplexVehicleTire::getVehicleTireSteer(void) const
    {
        return m_tireConfiguration.tireSteer;
    }

    void ComplexVehicleTire::setVehicleSteerSide(VehicleSteerSide steerSide)
    {
        m_tireConfiguration.steerSide = steerSide;
    }

    VehicleSteerSide ComplexVehicleTire::getVehicleSteerSide(void) const
    {
        return m_tireConfiguration.steerSide;
    }

    void ComplexVehicleTire::setVehicleTireAccel(VehicleTireAccel tireAccel)
    {
        m_tireConfiguration.tireAccel = tireAccel;
    }

    VehicleTireAccel ComplexVehicleTire::getVehicleTireAccel(void) const
    {
        return m_tireConfiguration.tireAccel;
    }

    void ComplexVehicleTire::setVehicleTireBrake(VehicleTireBrake brakeMode)
    {
        m_tireConfiguration.brakeMode = brakeMode;
    }

    VehicleTireBrake ComplexVehicleTire::getVehicleTireBrake(void) const
    {
        return m_tireConfiguration.brakeMode;
    }

    void ComplexVehicleTire::setLateralFriction(Ogre::Real lateralFriction)
    {
        m_tireConfiguration.lateralFriction = lateralFriction;
    }

    Ogre::Real ComplexVehicleTire::getLateralFriction(void) const
    {
        return m_tireConfiguration.lateralFriction;
    }

    void ComplexVehicleTire::setLongitudinalFriction(Ogre::Real longitudinalFriction)
    {
        m_tireConfiguration.longitudinalFriction = longitudinalFriction;
    }

    Ogre::Real ComplexVehicleTire::getLongitudinalFriction(void) const
    {
        return m_tireConfiguration.longitudinalFriction;
    }

    void ComplexVehicleTire::setSpringLength(Ogre::Real springLength)
    {
        m_tireConfiguration.springLength = springLength;
    }

    Ogre::Real ComplexVehicleTire::getSpringLength(void) const
    {
        return m_tireConfiguration.springLength;
    }

    void ComplexVehicleTire::setSmass(Ogre::Real smass)
    {
        m_tireConfiguration.smass = smass;
    }

    Ogre::Real ComplexVehicleTire::getSmass(void) const
    {
        return m_tireConfiguration.smass;
    }

    void ComplexVehicleTire::setSpringConst(Ogre::Real springConst)
    {
        m_tireConfiguration.springConst = springConst;
    }

    Ogre::Real ComplexVehicleTire::getSpringConst(void) const
    {
        return m_tireConfiguration.springConst;
    }

    void ComplexVehicleTire::setSpringDamp(Ogre::Real springDamp)
    {
        m_tireConfiguration.springDamp = springDamp;
    }

    Ogre::Real ComplexVehicleTire::getSpringDamp(void) const
    {
        return m_tireConfiguration.springDamp;
    }

    ndMultiBodyVehicleTireJointInfo ComplexVehicleTire::buildJointInfo(void) const
    {
        // We do not touch internal fields of ndWheelDescriptor / ndTireFrictionModel here,
        // we only rely on their default constructors. If you need more detailed tuning
        // (radius, width, mass, suspension, friction curves, etc.) you can expand this
        // later once you see those types.
        ndWheelDescriptor wheelDesc;
        ndTireFrictionModel frictionModel;
        ndMultiBodyVehicleTireJointInfo info(wheelDesc, frictionModel);
        return info;
    }

    // -------------------------------------------------------------------------
    // ComplexVehicle
    // -------------------------------------------------------------------------
    ComplexVehicle::ComplexVehicle(World* world, Ogre::SceneManager* sceneManager, const Ogre::Vector3& defaultDirection,
        const OgreNewt::CollisionPtr& col, Ogre::Real vhmass, const Ogre::Vector3& collisionPosition, const Ogre::Vector3& massOrigin, ComplexVehicleCallback* vehicleCallback)
        : OgreNewt::Body(world, sceneManager, col, Ogre::SceneMemoryMgrTypes::SCENE_DYNAMIC, Body::NotifyKind::ComplexVehicle),
        m_driveLayout(cdlRearWheelDrive),
        m_world(world),
        m_vehicleCallback(vehicleCallback),
        m_vehicleModel(nullptr),
        m_vehicleAddedToWorld(false),
        m_tireCount(0),
        m_mass(vhmass),
        m_comLocal(massOrigin),
        m_canDrive(true),
        m_gravity(9.81f),
        m_vehicleForce(Ogre::Vector3::ZERO),
        m_motor(nullptr),
        m_motorMaxRpm(6000.0f),
        m_motorOmegaAccel(1500.0f),
        m_motorFrictionLoss(15.0f),
        m_motorTorqueScale(1000.0f),
        m_frontDiff(nullptr),
        m_rearDiff(nullptr),
        m_centerDiff(nullptr)
    {
        OGRE_UNUSED(defaultDirection);
        OGRE_UNUSED(collisionPosition);
    }

    ComplexVehicle::~ComplexVehicle(void)
    {
        if (m_vehicleModel)
        {
            if (auto* const ndWorld = m_world ? m_world->getNewtonWorld() : nullptr)
            {
                ndWorld->RemoveModel(m_vehicleModel);
            }
            delete m_vehicleModel;
            m_vehicleModel = nullptr;
        }
        m_frontDiff = nullptr;
        m_rearDiff = nullptr;
        m_centerDiff = nullptr;
        m_complexTires.clear();
    }

    bool ComplexVehicle::removeTire(ComplexVehicleTire* tire)
    {
        auto it = std::find(m_complexTires.begin(), m_complexTires.end(), tire);
        if (it != m_complexTires.end())
        {
            m_complexTires.erase(it);
            m_tireCount = static_cast<int>(m_complexTires.size());
            return true;
        }
        return false;
    }

    void ComplexVehicle::removeAllTires()
    {
        m_complexTires.clear();
        m_tireCount = 0;
    }

    void ComplexVehicle::addTire(ComplexVehicleTire* tire)
    {
        if (!tire)
        {
            return;
        }

        OgreNewt::Body* tireBody = tire->getTireBody();
        if (!tireBody)
        {
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL,
                "[OgreNewt::ComplexVehicle] AddTire called with tire that has no body (m_childBody == nullptr).");
            return;
        }

        m_complexTires.push_back(tire);
        m_tireCount = static_cast<int>(m_complexTires.size());

        ensureVehicleModel();
        if (!m_vehicleModel)
        {
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL,
                "[OgreNewt::ComplexVehicle] AddTire: m_vehicleModel is null – vehicle model was not created.");
            return;
        }

        ndMultiBodyVehicleTireJointInfo info = tire->buildJointInfo();
        ndBodyKinematic* ndTireBody = tireBody->getNewtonBody();

        ndSharedPtr<ndBody> tireShared(ndTireBody);
        ndMultiBodyVehicleTireJoint* joint = m_vehicleModel->AddTire(info, tireShared);

        tire->setTireJoint(joint);
    }

    void ComplexVehicle::findAxleTires(ComplexVehicleTire*& frontLeft, ComplexVehicleTire*& frontRight, ComplexVehicleTire*& rearLeft, ComplexVehicleTire*& rearRight)
    {
        frontLeft = nullptr;
        frontRight = nullptr;
        rearLeft = nullptr;
        rearRight = nullptr;

        for (ComplexVehicleTire* tire : m_complexTires)
        {
            if (!tire)
            {
                continue;
            }

            if (tire->getAxle() == ComplexVehicleTire::axleFront)
            {
                if (tire->getSide() == ComplexVehicleTire::sideLeft)
                {
                    frontLeft = tire;
                }
                else
                {
                    frontRight = tire;
                }
            }
            else
            {
                if (tire->getSide() == ComplexVehicleTire::sideLeft)
                {
                    rearLeft = tire;
                }
                else
                {
                    rearRight = tire;
                }
            }
        }
    }

    void ComplexVehicle::buildDrivetrain(ComplexDriveLayout layout, Ogre::Real diffMass, Ogre::Real diffRadius,
        Ogre::Real slipOmegaLockFront, Ogre::Real slipOmegaLockRear, Ogre::Real slipOmegaLockCenter)
    {
        m_driveLayout = layout;

        ComplexVehicleTire* fl = nullptr;
        ComplexVehicleTire* fr = nullptr;
        ComplexVehicleTire* rl = nullptr;
        ComplexVehicleTire* rr = nullptr;

        findAxleTires(fl, fr, rl, rr);

        ndMultiBodyVehicle* const model = getVehicleModel();
        if (!model)
        {
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[ComplexVehicle] BuildDrivetrain: vehicle model is null.");
            return;
        }

        // Front differential (if both front tires exist)
        if (fl && fr)
        {
            ndMultiBodyVehicleTireJoint* const flJoint = fl->getTireJoint();
            ndMultiBodyVehicleTireJoint* const frJoint = fr->getTireJoint();

            if (flJoint && frJoint)
            {
                m_frontDiff = model->AddDifferential(static_cast<ndFloat32>(diffMass), static_cast<ndFloat32>(diffRadius),
                    flJoint, frJoint, static_cast<ndFloat32>(slipOmegaLockFront));
            }
        }

        // Rear differential (if both rear tires exist)
        if (rl && rr)
        {
            ndMultiBodyVehicleTireJoint* const rlJoint = rl->getTireJoint();
            ndMultiBodyVehicleTireJoint* const rrJoint = rr->getTireJoint();

            if (rlJoint && rrJoint)
            {
                m_rearDiff = model->AddDifferential(static_cast<ndFloat32>(diffMass), static_cast<ndFloat32>(diffRadius),
                    rlJoint, rrJoint, static_cast<ndFloat32>(slipOmegaLockRear));
            }
        }

        // Center differential only if AWD and both front+rear diffs exist
        m_centerDiff = nullptr;

        if (layout == cdlAllWheelDrive && m_frontDiff && m_rearDiff)
        {
            m_centerDiff = model->AddDifferential(static_cast<ndFloat32>(diffMass), static_cast<ndFloat32>(diffRadius),
                m_frontDiff, m_rearDiff, static_cast<ndFloat32>(slipOmegaLockCenter));
        }

        Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[ComplexVehicle] BuildDrivetrain finished. Layout: " + Ogre::StringConverter::toString(static_cast<int>(layout)));
    }

    void ComplexVehicle::ensureVehicleModel()
    {
        if (m_vehicleModel)
        {
            return;
        }

        if (!m_world)
        {
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[OgreNewt::ComplexVehicle] ensureVehicleModel: World is null.");
            return;
        }

        ndWorld* ndWorld = m_world->getNewtonWorld();
        if (!ndWorld)
        {
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[OgreNewt::ComplexVehicle] ensureVehicleModel: Newton world is null.");
            return;
        }

        ndBodyKinematic* ndChassis = getNewtonBody();
        if (!ndChassis)
        {
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[OgreNewt::ComplexVehicle] ensureVehicleModel: Chassis Newton body is null.");
            return;
        }

        m_vehicleModel = new ndMultiBodyVehicle(static_cast<ndFloat32>(m_gravity));

        ndSharedPtr<ndBody> chassisShared(ndChassis);
        m_vehicleModel->AddChassis(chassisShared);

        ndWorld->AddModel(m_vehicleModel);
        m_vehicleAddedToWorld = true;
    }

    void ComplexVehicle::applyForceAndTorque(OgreNewt::Body* vBody, const Ogre::Vector3& vForce, const Ogre::Vector3& vPoint)
    {
        if (!vBody)
        {
            return;
        }

        vBody->addGlobalForce(vForce, vPoint);
    }

    void ComplexVehicle::addForceAtPos(OgreNewt::Body* body, const Ogre::Vector3& forceWS, const Ogre::Vector3& pointWS)
    {
        if (!body)
        {
            return;
        }

        applyForceAndTorque(body, forceWS, pointWS);
    }

    void ComplexVehicle::update(Ogre::Real timestep, int threadIndex)
    {
        if (!m_vehicleModel)
        {
            return;
        }

        for (ComplexVehicleTire* tire : m_complexTires)
        {
            if (tire)
            {
                updateDriverInput(tire, timestep);
            }
        }

        // The ndWorld will step the model alongside the world step.
    }

    void ComplexVehicle::createMotor(Ogre::Real mass, Ogre::Real radius)
    {
        ensureVehicleModel();
        if (!m_vehicleModel)
        {
            return;
        }

        m_motor = m_vehicleModel->AddMotor(static_cast<ndFloat32>(mass), static_cast<ndFloat32>(radius));
        if (m_motor)
        {
            m_motor->SetMaxRpm(static_cast<ndFloat32>(m_motorMaxRpm));
            m_motor->SetOmegaAccel(static_cast<ndFloat32>(m_motorOmegaAccel));
            m_motor->SetFrictionLoss(static_cast<ndFloat32>(m_motorFrictionLoss));
        }
    }

    void ComplexVehicle::setMotorMaxRpm(Ogre::Real rpm)
    {
        m_motorMaxRpm = rpm;
        if (m_motor)
        {
            m_motor->SetMaxRpm(static_cast<ndFloat32>(rpm));
        }
    }

    void ComplexVehicle::setMotorOmegaAccel(Ogre::Real rpmPerSec)
    {
        m_motorOmegaAccel = rpmPerSec;
        if (m_motor)
        {
            m_motor->SetOmegaAccel(static_cast<ndFloat32>(rpmPerSec));
        }
    }

    void ComplexVehicle::setMotorFrictionLoss(Ogre::Real newtonMeters)
    {
        m_motorFrictionLoss = newtonMeters;
        if (m_motor)
        {
            m_motor->SetFrictionLoss(static_cast<ndFloat32>(newtonMeters));
        }
    }

    void ComplexVehicle::setMotorTorqueScale(Ogre::Real nmPerUnit)
    {
        m_motorTorqueScale = nmPerUnit;
    }

    void ComplexVehicle::setCanDrive(bool canDrive)
    {
        m_canDrive = canDrive;
    }

    Ogre::Vector3 ComplexVehicle::getVehicleForce() const
    {
        return m_vehicleForce;
    }

    void ComplexVehicle::updateDriverInput(ComplexVehicleTire* tire, Ogre::Real timestep)
    {
        if (!tire || !m_canDrive)
        {
            return;
        }

        ComplexVehicleCallback* cb = getVehicleCallback();
        if (!cb)
        {
            return;
        }

        const ComplexVehicleTire::TireConfiguration& cfg = tire->getTireConfiguration();

        const Ogre::Real steerDeg = cb->onSteerAngleChanged(this, tire, timestep);
        const Ogre::Real motorIn = cb->onMotorForceChanged(this, tire, timestep);
        const Ogre::Real handIn = cb->onHandBrakeChanged(this, tire, timestep);
        const Ogre::Real brakeIn = cb->onBrakeChanged(this, tire, timestep);

        // Steering
        if (cfg.tireSteer == tsSteer)
        {
            const Ogre::Real sideSign = (cfg.steerSide == tsSteerSideA) ? 1.0f : -1.0f;
            tire->setSteerAngleDeg(steerDeg * sideSign);
        }
        else
        {
            tire->setSteerAngleDeg(0.0f);
        }

        // Accel
        if (cfg.tireAccel == tsAccel)
        {
            tire->setMotorForce(motorIn);
        }
        else
        {
            tire->setMotorForce(0.0f);
        }

        // Regular brake
        if (cfg.brakeMode == tsBrake)
        {
            tire->setBrakeForce(std::clamp(brakeIn, Ogre::Real(0.0f), Ogre::Real(1.0f)));
        }
        else
        {
            tire->setBrakeForce(0.0f);
        }

        // Handbrake
        tire->setHandBrake(std::clamp(handIn, Ogre::Real(0.0f), Ogre::Real(1.0f)));

        // Motor torque
        if (m_motor)
        {
            const Ogre::Real throttleClamped = std::clamp(motorIn, Ogre::Real(0.0f), Ogre::Real(1.0f));
            const ndFloat32 torqueNm = static_cast<ndFloat32>(throttleClamped * m_motorTorqueScale);
            const ndFloat32 targetRpm = static_cast<ndFloat32>(throttleClamped * m_motorMaxRpm);

            // ND4 API: newtonMeters, rpm
            m_motor->SetTorqueAndRpm(torqueNm, targetRpm);
        }
    }

} // namespace OgreNewt

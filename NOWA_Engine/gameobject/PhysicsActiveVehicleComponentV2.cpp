#include "NOWAPrecompiled.h"
#include "PhysicsActiveVehicleComponentV2.h"
#include "LuaScriptComponent.h"
#include "PhysicsComponent.h"
#include "gameobject/GameObjectController.h"
#include "main/AppStateManager.h"
#include "modules/LuaScriptApi.h"
#include "utilities/MathHelper.h"
#include "utilities/XMLConverter.h"

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    PhysicsActiveVehicleComponentV2::PhysicsVehicleV2Callback::PhysicsVehicleV2Callback(GameObject* owner, LuaScript* luaScript, OgreNewt::World* ogreNewt, const Ogre::String& onSteerAngleChangedFunctionName,
        const Ogre::String& onMotorForceChangedFunctionName, const Ogre::String& onHandBrakeChangedFunctionName, const Ogre::String& onBrakeChangedFunctionName) :
        OgreNewt::VehicleV2Callback(),
        m_owner(owner),
        m_luaScript(luaScript),
        m_ogreNewt(ogreNewt),
        onSteerAngleChangedFunctionName(onSteerAngleChangedFunctionName),
        onMotorForceChangedFunctionName(onMotorForceChangedFunctionName),
        onHandBrakeChangedFunctionName(onHandBrakeChangedFunctionName),
        onBrakeChangedFunctionName(onBrakeChangedFunctionName)
    {
        if (!m_luaScript)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveVehicleComponentV2] GameObject '" + owner->getName() + "' has no LuaScriptComponent. Vehicle cannot be controlled via Lua.");
        }
    }

    PhysicsActiveVehicleComponentV2::PhysicsVehicleV2Callback::~PhysicsVehicleV2Callback()
    {
    }

    Ogre::Real PhysicsActiveVehicleComponentV2::PhysicsVehicleV2Callback::onSteerAngleChanged(OgreNewt::VehicleDrivingManipulationV2* manip, Ogre::Real dt)
    {
        manip->setSteerAngle(0.0f);
        if (m_luaScript && m_luaScript->isCompiled() && !this->onSteerAngleChangedFunctionName.empty())
        {
            m_luaScript->callTableFunction(this->onSteerAngleChangedFunctionName, manip, dt);
        }
        return manip->getSteerAngle();
    }

    Ogre::Real PhysicsActiveVehicleComponentV2::PhysicsVehicleV2Callback::onMotorForceChanged(OgreNewt::VehicleDrivingManipulationV2* manip, Ogre::Real dt)
    {
        manip->setMotorForce(0.0f);
        if (m_luaScript && m_luaScript->isCompiled() && !this->onMotorForceChangedFunctionName.empty())
        {
            m_luaScript->callTableFunction(this->onMotorForceChangedFunctionName, manip, dt);
        }
        return manip->getMotorForce();
    }

    Ogre::Real PhysicsActiveVehicleComponentV2::PhysicsVehicleV2Callback::onHandBrakeChanged(OgreNewt::VehicleDrivingManipulationV2* manip, Ogre::Real dt)
    {
        manip->setHandBrake(0.0f);
        if (m_luaScript && m_luaScript->isCompiled() && !this->onHandBrakeChangedFunctionName.empty())
        {
            m_luaScript->callTableFunction(this->onHandBrakeChangedFunctionName, manip, dt);
        }
        return manip->getHandBrake();
    }

    Ogre::Real PhysicsActiveVehicleComponentV2::PhysicsVehicleV2Callback::onBrakeChanged(OgreNewt::VehicleDrivingManipulationV2* manip, Ogre::Real dt)
    {
        manip->setBrake(0.0f);
        if (m_luaScript && m_luaScript->isCompiled() && !this->onBrakeChangedFunctionName.empty())
        {
            m_luaScript->callTableFunction(this->onBrakeChangedFunctionName, manip, dt);
        }
        return manip->getBrake();
    }

    PhysicsActiveVehicleComponentV2::PhysicsActiveVehicleComponentV2() :
        PhysicsActiveComponent(),
        onSteerAngleChangedFunctionName(new Variant(PhysicsActiveVehicleComponentV2::AttrOnSteerAngleChangedFunctionName(), Ogre::String(""), this->attributes)),
        onMotorForceChangedFunctionName(new Variant(PhysicsActiveVehicleComponentV2::AttrOnMotorForceChangedFunctionName(), Ogre::String(""), this->attributes)),
        onHandBrakeChangedFunctionName(new Variant(PhysicsActiveVehicleComponentV2::AttrOnHandBrakeChangedFunctionName(), Ogre::String(""), this->attributes)),
        onBrakeChangedFunctionName(new Variant(PhysicsActiveVehicleComponentV2::AttrOnBrakeChangedFunctionName(), Ogre::String(""), this->attributes)),
        tireDirectionSwap(new Variant(PhysicsActiveVehicleComponentV2::AttrTireDirectionSwap(), false, this->attributes)),
        steeringStrength(new Variant(PhysicsActiveVehicleComponentV2::AttrSteeringStrength(), 1.0f, this->attributes)),
        tireSpinAxis(new Variant(PhysicsActiveVehicleComponentV2::AttrTireSpinAxis(), std::vector<Ogre::String>({"X", "Y", "Z"}), this->attributes))
    {
        // Create all 8 tire-ID slots
        for (int i = 0; i < MAX_TIRES; ++i)
        {
            // Important: For cloning or group save load, last parameter: isId -> must be true so that ids are overwritten in a unique manner for group is saved to prevent id collisions
            this->tireIds[i] = new Variant(PhysicsActiveVehicleComponentV2::AttrTireId(i), static_cast<unsigned long>(0UL), this->attributes, true);
            this->tireIds[i]->setDescription("Id of tire GameObject for slot " + Ogre::StringConverter::toString(i) + ". 0 = unused.");
        }

        this->asSoftBody->setVisible(false);

        // Sensible vehicle defaults.
        // Use ConvexHull so Newton derives inertia from the actual mesh geometry –
        // a Capsule/Box with made-up dimensions produces near-zero rotational
        // inertia and causes the vehicle to tip over at the first impulse.
        this->mass->setValue(1200.0f);
        this->massOrigin->setValue(Ogre::Vector3(0.0f, -0.3f, 0.0f)); // low CoM stabilises roll
        this->massOrigin->setDescription("Centre-of-mass offset in local space. "
                                         "Negative Y lowers the CoM and improves roll stability.");
        this->collisionSize->setValue(Ogre::Vector3::ZERO); // ZERO = use mesh extents
        this->collisionPosition->setValue(Ogre::Vector3::ZERO);
        this->collisionType->setListSelectedValue("ConvexHull"); // correct inertia from real mesh

        // Damp pitch (X) and roll (Z) strongly; leave yaw (Y) light for steering.
        // These values resist tipping without fighting the turning impulse.
        this->angularDamping->setValue(Ogre::Vector3(0.9f, 0.05f, 0.9f));
        this->linearDamping->setValue(0.05f);

        this->tireDirectionSwap->setDescription("Flip the spin direction of all tires. "
                                            "Enable this if your tires roll backward when driving forward. "
                                            "The correct setting depends on which way your tire mesh's local X axis points.");

        this->steeringStrength->setDescription("Multiplier on the yaw-rate target used when steering (default 1.0 = 1 rad/s at full lock). "
                                           "Increase (e.g. 2.0–4.0) if the vehicle turns too sluggishly. "
                                           "Decrease if it over-steers or spins out.");

        this->tireSpinAxis->setDescription("Chassis-space axis the tires spin around when rolling (default X). "
                                       "X: axle = +X (left-right); typical for cars facing +Z in the modeller. "
                                       "Z: axle = +Z (left-right); typical for cars facing +X in the modeller. "
                                       "Y: axle = +Y (vertical); uncommon. "
                                       "Start with X. If tires flip like a coin change to Z. "
                                       "Use TireDirectionSwap to reverse direction once the axis is correct.");
        this->tireSpinAxis->setListSelectedValue("X");

        this->onSteerAngleChangedFunctionName->setDescription("Lua callback: onSteerAngleChanged(manip, dt). "
                                                          "Call manip:setSteerAngle(degrees) with [-45..45].");
        this->onSteerAngleChangedFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), this->onSteerAngleChangedFunctionName->getString() + "(manip, dt)");

        this->onMotorForceChangedFunctionName->setDescription("Lua callback: onMotorForceChanged(manip, dt). "
                                                          "Call manip:setMotorForce(force). Same scale as old vehicle "
                                                          "(e.g. 10000 * 120 * dt forward).");
        this->onMotorForceChangedFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), this->onMotorForceChangedFunctionName->getString() + "(manip, dt)");

        this->onHandBrakeChangedFunctionName->setDescription("Lua callback: onHandBrakeChanged(manip, dt). "
                                                         "Call manip:setHandBrake(value). Good value: 5.5.");
        this->onHandBrakeChangedFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), this->onHandBrakeChangedFunctionName->getString() + "(manip, dt)");

        this->onBrakeChangedFunctionName->setDescription("Lua callback: onBrakeChanged(manip, dt). "
                                                     "Call manip:setBrake(value). Good value: 7.5.");
        this->onBrakeChangedFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), this->onBrakeChangedFunctionName->getString() + "(manip, dt)");
    }

    PhysicsActiveVehicleComponentV2::~PhysicsActiveVehicleComponentV2()
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveVehicleComponentV2] Destructor for: " + this->gameObjectPtr->getName());
    }

    bool PhysicsActiveVehicleComponentV2::init(rapidxml::xml_node<>*& propertyElement)
    {
        PhysicsActiveComponent::parseCommonProperties(propertyElement);

        for (int i = 0; i < MAX_TIRES; ++i)
        {
            const Ogre::String attrName = "TireId_" + Ogre::StringConverter::toString(i);
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == attrName)
            {
                this->setTireId(i, XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
                propertyElement = propertyElement->next_sibling("property");
            }
        }

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OnSteerAngleFunctionName")
        {
            this->setOnSteerAngleChangedFunctionName(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OnMotorForceFunctionName")
        {
            this->setOnMotorForceChangedFunctionName(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OnHandBrakeFunctionName")
        {
            this->setOnHandBrakeChangedFunctionName(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OnBrakeFunctionName")
        {
            this->setOnBrakeChangedFunctionName(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TireDirectionSwap")
        {
            this->setTireDirectionSwap(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SteeringStrength")
        {
            this->setSteeringStrength(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TireSpinAxis")
        {
            this->setTireSpinAxis(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        return true;
    }

    bool PhysicsActiveVehicleComponentV2::postInit(void)
    {
        // Use PhysicsComponent::postInit() – body creation is deferred to connect()
        bool success = PhysicsComponent::postInit();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveVehicleComponentV2] postInit for: " + this->gameObjectPtr->getName());

        this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
        this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
        this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();

        this->gameObjectPtr->setDynamic(true);
        this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
        this->gameObjectPtr->getAttribute(GameObject::AttrDefaultDirection())->setValue(Ogre::Vector3::UNIT_Z);

        if (!this->createDynamicBody())
        {
            return false;
        }

        return success;
    }

    void PhysicsActiveVehicleComponentV2::onRemoveComponent(void)
    {
        if (this->physicsBody)
        {
            static_cast<OgreNewt::VehicleV2*>(this->physicsBody)->clearTires();
        }
    }

    GameObjectCompPtr PhysicsActiveVehicleComponentV2::clone(GameObjectPtr clonedGameObjectPtr)
    {
        PhysicsActiveVehicleComponentV2CompPtr cloned(boost::make_shared<PhysicsActiveVehicleComponentV2>());

        cloned->setActivated(this->activated->getBool());
        cloned->setMass(this->mass->getReal());
        cloned->setMassOrigin(this->massOrigin->getVector3());
        cloned->setLinearDamping(this->linearDamping->getReal());
        cloned->setAngularDamping(this->angularDamping->getVector3());
        cloned->setGravity(this->gravity->getVector3());
        cloned->setGravitySourceCategory(this->gravitySourceCategory->getString());
        cloned->setConstraintDirection(this->constraintDirection->getVector3());
        cloned->setSpeed(this->speed->getReal());
        cloned->setCollisionType(this->collisionType->getListSelectedValue());
        cloned->setCollisionDirection(this->collisionDirection->getVector3());

        clonedGameObjectPtr->addComponent(cloned);
        cloned->setOwner(clonedGameObjectPtr);

        for (int i = 0; i < MAX_TIRES; ++i)
        {
            cloned->setTireId(i, this->tireIds[i]->getULong());
        }

        cloned->setOnSteerAngleChangedFunctionName(this->onSteerAngleChangedFunctionName->getString());
        cloned->setOnMotorForceChangedFunctionName(this->onMotorForceChangedFunctionName->getString());
        cloned->setOnHandBrakeChangedFunctionName(this->onHandBrakeChangedFunctionName->getString());
        cloned->setOnBrakeChangedFunctionName(this->onBrakeChangedFunctionName->getString());
        cloned->setTireDirectionSwap(this->tireDirectionSwap->getBool());
        cloned->setSteeringStrength(this->steeringStrength->getReal());
        cloned->setTireSpinAxis(this->tireSpinAxis->getListSelectedValue());

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(cloned));
        return cloned;
    }

    bool PhysicsActiveVehicleComponentV2::onCloned(void)
    {
        for (int i = 0; i < MAX_TIRES; ++i)
        {
            const unsigned long priorId = this->tireIds[i]->getULong();
            if (0UL == priorId)
            {
                continue;
            }

            GameObjectPtr tireGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getClonedGameObjectFromPriorId(priorId);

            Ogre::LogManager::getSingletonPtr()->logMessage("[VehicleV2] onCloned slot " + Ogre::StringConverter::toString(i) + " priorId=" + Ogre::StringConverter::toString(priorId) +
                                                            " found=" + (tireGameObjectPtr ? tireGameObjectPtr->getName() : "NULL"));

            if (nullptr != tireGameObjectPtr)
            {
                this->tireIds[i]->setValue(tireGameObjectPtr->getId());
            }
            else
            {
                this->tireIds[i]->setValue(static_cast<unsigned long>(0));
            }
        }

        return true;
    }

    // =========================================================================
    // connect  (simulation start)
    // =========================================================================
    bool PhysicsActiveVehicleComponentV2::connect(void)
    {
        bool success = PhysicsActiveComponent::connect();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveVehicleComponentV2] connect for: " + this->gameObjectPtr->getName());

        /*if (false == this->gameObjectPtr->hasComponent("MeshModifyComponent"))
        {
            if (!this->createDynamicBody())
            {
                return false;
            }
        }*/

        NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
        {
            // Step 3: re-register tires against the new body.
            this->registerTireNodes();
            this->setCanDrive(this->activated->getBool());

            if (this->physicsBody && this->bShowDebugData)
            {
                this->physicsBody->showDebugCollision(false, this->bShowDebugData);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PhysicsActiveVehicleComponentV2::reCreateDynamicBodyForItem");

        return success;
    }

    void PhysicsActiveVehicleComponentV2::reCreateDynamicBodyForItem(Ogre::Item* item)
    {
        if (nullptr == item)
        {
            return;
        }

        // Step 1: destroy existing body and collision
        this->destroyCollision();
        this->destroyBody();

        // Step 2: recreate the full Vehicle.
        // swapMovableObject() has already attached the new Ogre::Item to the scene node,
        // so createDynamicBody() -> createDynamicCollision() picks up the new mesh automatically.
        if (false == this->createDynamicBody())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveVehicleComponent] reCreateDynamicBodyForItem: createDynamicBody failed for: " + this->gameObjectPtr->getName());
            return;
        }

        NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
        {
            // Step 3: re-register tires against the new body.
            this->registerTireNodes();
            this->setCanDrive(this->activated->getBool());

            if (this->physicsBody && this->bShowDebugData)
            {
                this->physicsBody->showDebugCollision(false, this->bShowDebugData);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PhysicsActiveVehicleComponentV2::reCreateDynamicBodyForItem");
    }

    // =========================================================================
    // disconnect  (simulation stop)
    // =========================================================================
    bool PhysicsActiveVehicleComponentV2::disconnect(void)
    {
        bool success = PhysicsActiveComponent::disconnect();

        if (this->physicsBody)
        {
            OgreNewt::VehicleV2* vehicle = static_cast<OgreNewt::VehicleV2*>(this->physicsBody);

            // Stop GraphicsModule from continuing to drive these nodes from stale
            // buffered data. Without this, any later direct write to a tire node
            // (e.g. undo) gets silently overwritten on the very next render frame,
            // because updateAllTransforms() unconditionally re-applies whatever is
            // sitting in the trackedNodes buffer for any node that is still active.
            for (const OgreNewt::TireInfoV2& tire : vehicle->getTires())
            {
                if (nullptr != tire.sceneNode)
                {
                    NOWA::GraphicsModule::getInstance()->removeTrackedNode(tire.sceneNode);
                }
            }

            vehicle->clearTires();
            // this->destroyCollision();
            // this->destroyBody();
        }

        return success;
    }

    // =========================================================================
    // update  (logic thread, every frame)
    // =========================================================================
    void PhysicsActiveVehicleComponentV2::update(Ogre::Real dt, bool notSimulating)
    {
        PhysicsActiveComponent::update(dt, notSimulating);

        if (!notSimulating && this->physicsBody)
        {
            this->updateTireNodes(dt);
        }
    }

    bool PhysicsActiveVehicleComponentV2::createDynamicBody(void)
    {
        // this->destroyCollision();
        // this->destroyBody();

        Ogre::Vector3 inertia = Ogre::Vector3(1.0f, 1.0f, 1.0f);
        Ogre::Quaternion collisionOrientation = Ogre::Quaternion::IDENTITY;

        if (Ogre::Vector3::ZERO != this->collisionDirection->getVector3())
        {
            collisionOrientation = MathHelper::getInstance()->degreesToQuat(this->collisionDirection->getVector3());
        }

        Ogre::Vector3 calculatedMassOrigin = Ogre::Vector3::ZERO;

        GraphicsModule::RenderCommand renderCommand = [this, &inertia, collisionOrientation, &calculatedMassOrigin]()
        {
            this->collisionPtr = this->createDynamicCollision(inertia, this->collisionSize->getVector3(), this->collisionPosition->getVector3(), collisionOrientation, calculatedMassOrigin, this->gameObjectPtr->getCategoryId());
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PhysicsActiveVehicleComponentV2::createDynamicBody");

        if (nullptr == this->collisionPtr)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveVehicleComponentV2] Collision is null for: " + this->gameObjectPtr->getName());
            return false;
        }

        if (Ogre::Vector3::ZERO != this->massOrigin->getVector3())
        {
            calculatedMassOrigin = this->massOrigin->getVector3();
        }

        const Ogre::Real weightedMass = this->mass->getReal();
        LuaScript* luaScript = this->gameObjectPtr->getLuaScript();

        this->physicsBody = new OgreNewt::VehicleV2(this->ogreNewt, this->gameObjectPtr->getSceneManager(), this->collisionPtr, weightedMass, calculatedMassOrigin, this->gravity->getVector3(), this->gameObjectPtr->getDefaultDirection(),
            new PhysicsVehicleV2Callback(this->gameObjectPtr.get(), luaScript, this->ogreNewt, this->onSteerAngleChangedFunctionName->getString(), this->onMotorForceChangedFunctionName->getString(), this->onHandBrakeChangedFunctionName->getString(),
                this->onBrakeChangedFunctionName->getString()));

        // Push configurable parameters into the vehicle before registerTireNodes() runs.
        OgreNewt::VehicleV2* vehicle = static_cast<OgreNewt::VehicleV2*>(this->physicsBody);
        vehicle->setTireDirectionSwap(this->tireDirectionSwap->getBool());
        vehicle->setSteeringStrength(this->steeringStrength->getReal());
        this->applySpinAxisToVehicle(vehicle);

        NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->registerRenderCallbackForBody(this->physicsBody);

        this->physicsBody->setGravity(this->gravity->getVector3());

        if (this->linearDamping->getReal() != 0.0f)
        {
            this->physicsBody->setLinearDamping(this->linearDamping->getReal());
        }
        if (this->angularDamping->getVector3() != Ogre::Vector3::ZERO)
        {
            this->physicsBody->setAngularDamping(this->angularDamping->getVector3());
        }

        this->physicsBody->setUserData(OgreNewt::Any(dynamic_cast<PhysicsComponent*>(this)));
        this->physicsBody->attachNode(this->gameObjectPtr->getSceneNode());

        this->setPosition(this->initialPosition);
        this->setOrientation(this->initialOrientation);

        this->setConstraintAxis(this->constraintAxis->getVector3());
        this->setConstraintDirection(this->constraintDirection->getVector3());
        this->setCollidable(this->collidable->getBool());

        this->physicsBody->setType(this->gameObjectPtr->getCategoryId());

        const auto materialId = AppStateManager::getSingletonPtr()->getGameObjectController()->getMaterialID(this->gameObjectPtr.get(), this->ogreNewt);
        this->physicsBody->setMaterialGroupID(materialId);

        this->setActivated(this->activated->getBool());
        this->setGyroscopicTorqueEnabled(this->gyroscopicTorque->getBool());

        return true;
    }

    // =========================================================================
    // vehicleMoveCallback  (Newton worker thread)
    // =========================================================================
    void PhysicsActiveVehicleComponentV2::vehicleMoveCallback(OgreNewt::Body* body, Ogre::Real dt, int threadIndex)
    {
        // 1. Gravity, speed limits, constraint axes – unchanged base behaviour
        PhysicsActiveComponent::moveCallback(body, dt, threadIndex);

        // 2. Vehicle control impulses (longitudinal / lateral / yaw)
        if (this->physicsBody)
        {
            static_cast<OgreNewt::VehicleV2*>(this->physicsBody)->applyImpulses(dt);
        }
    }

    void PhysicsActiveVehicleComponentV2::registerTireNodes(void)
    {
        if (nullptr == this->physicsBody)
        {
            return;
        }

        OgreNewt::VehicleV2* vehicle = static_cast<OgreNewt::VehicleV2*>(this->physicsBody);
        vehicle->clearTires();

        const Ogre::Vector3 chassisWorldPos = this->physicsBody->getPosition();
        const Ogre::Quaternion chassisWorldOrient = this->physicsBody->getOrientation();
        const Ogre::Quaternion chassisWorldInv = chassisWorldOrient.Inverse();

        for (int i = 0; i < MAX_TIRES; ++i)
        {
            const unsigned long tireId = this->tireIds[i]->getULong();
            if (tireId == 0UL)
            {
                continue;
            }

            GameObjectPtr tireGO = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(tireId);

            if (nullptr == tireGO)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveVehicleComponentV2] TireId_" + Ogre::StringConverter::toString(i) + " = " + Ogre::StringConverter::toString(tireId) + " not found. Skipping.");
                continue;
            }

            Ogre::SceneNode* tireNode = tireGO->getSceneNode();
            if (!tireNode)
            {
                continue;
            }

            // ── Chassis-local transform ───────────────────────────────────────
            const Ogre::Vector3 tireWorldPos = tireNode->_getDerivedPosition();
            const Ogre::Quaternion tireWorldOrient = tireNode->_getDerivedOrientation();

            const Ogre::Vector3 localPos = chassisWorldInv * (tireWorldPos - chassisWorldPos);
            const Ogre::Quaternion localOrient = chassisWorldInv * tireWorldOrient;

            // ── Tire radius from Ogre V2 Aabb ─────────────────────────────────
            // getWorldAabbUpdated() returns Ogre::Aabb with mCenter and mHalfSize.
            // mHalfSize already contains half-extents – no division needed.
            Ogre::Real radius = 0.4f;
            Ogre::MovableObject* movable = (tireNode->numAttachedObjects() > 0) ? tireNode->getAttachedObject(0) : nullptr;

            if (movable)
            {
                const Ogre::Aabb aabb = movable->getWorldAabbUpdated();
                // Use the smallest horizontal half-extent as the rolling radius.
                // For a typical tire mesh:  X = radius, Y = height, Z = width
                const Ogre::Real rx = aabb.mHalfSize.x;
                const Ogre::Real rz = aabb.mHalfSize.z;
                radius = std::min(rx, rz);
                if (radius <= 0.001f)
                {
                    radius = aabb.mHalfSize.y; // fallback: height half-extent
                }
                radius = std::max(radius, 0.05f);
            }

            // ── Left / right (vehicle faces +X; left = positive local Z) ─────
            const bool isLeft = (localPos.z > 0.0f);

            vehicle->addTire(tireNode, localPos, localOrient, radius, isLeft);

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveVehicleComponentV2] Registered tire '" + tireGO->getName() + "' slot " + Ogre::StringConverter::toString(i) +
                                                                                   " localPos=" + Ogre::StringConverter::toString(localPos) + " radius=" + Ogre::StringConverter::toString(radius) + (isLeft ? " LEFT" : " RIGHT"));
        }
    }

    // =========================================================================
    // updateTireNodes  (logic thread, every frame)
    // =========================================================================
    void PhysicsActiveVehicleComponentV2::updateTireNodes(Ogre::Real dt)
    {
        OgreNewt::VehicleV2* vehicle = static_cast<OgreNewt::VehicleV2*>(this->physicsBody);

        if (!vehicle || vehicle->getTires().empty())
        {
            return;
        }

        // ── MAIN THREAD: query Lua, cache steer/motor/brake for the Newton thread ──
        vehicle->updateVehicleInput(dt);

        // All spin / world-transform math is encapsulated in VehicleV2.
        vehicle->computeTireTransforms(dt);

        // Push results into GraphicsModule's lock-free transform buffer.
        for (const OgreNewt::TireInfoV2& tire : vehicle->getTires())
        {
            if (nullptr == tire.sceneNode)
            {
                continue;
            }

            GraphicsModule::getInstance()->updateNodeTransform(tire.sceneNode, tire.worldPos, tire.worldOrient);
        }
    }

    // =========================================================================
    // Stunt helpers  (same signatures as old PhysicsActiveVehicleComponent)
    // =========================================================================
    void PhysicsActiveVehicleComponentV2::applyWheelie(Ogre::Real strength)
    {
        // Lift the front by applying a pitch angular impulse around local Z
        this->applyOmegaForce(this->getOrientation() * Ogre::Vector3(0.0f, 0.0f, strength));
    }

    void PhysicsActiveVehicleComponentV2::applyDrift(bool left, Ogre::Real strength, Ogre::Real steeringStrength)
    {
        this->applyForce(Ogre::Vector3(0.0f, strength, 0.0f));
        if (left)
        {
            this->applyOmegaForce(this->getOrientation() * Ogre::Vector3(0.0f, steeringStrength, 0.0f));
        }
        else
        {
            this->applyOmegaForce(this->getOrientation() * Ogre::Vector3(0.0f, -steeringStrength, 0.0f));
        }
    }

    void PhysicsActiveVehicleComponentV2::applyPitch(Ogre::Real strength, Ogre::Real dt)
    {
        // Apply a stable angular impulse around the local Z axis (pitch)
        if (!this->physicsBody)
        {
            return;
        }
        Ogre::Real mass;
        Ogre::Vector3 inertia;
        this->physicsBody->getMassMatrix(mass, inertia);
        const Ogre::Vector3 pitchAxis = this->getOrientation() * Ogre::Vector3::UNIT_Z;
        this->applyOmegaForce(pitchAxis * (strength * dt));
    }

    // =========================================================================
    // Attribute setters / getters
    // =========================================================================
    void PhysicsActiveVehicleComponentV2::setTireId(int index, unsigned long tireId)
    {
        if (index < 0 || index >= MAX_TIRES)
        {
            return;
        }
        this->tireIds[index]->setValue(tireId);
    }

    unsigned long PhysicsActiveVehicleComponentV2::getTireId(int index) const
    {
        if (index < 0 || index >= MAX_TIRES)
        {
            return 0UL;
        }
        return this->tireIds[index]->getULong();
    }

    void PhysicsActiveVehicleComponentV2::setOnSteerAngleChangedFunctionName(const Ogre::String& name)
    {
        this->onSteerAngleChangedFunctionName->setValue(name);
        this->onSteerAngleChangedFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), name + "(manip, dt)");
    }
    Ogre::String PhysicsActiveVehicleComponentV2::getOnSteerAngleChangedFunctionName() const
    {
        return this->onSteerAngleChangedFunctionName->getString();
    }

    void PhysicsActiveVehicleComponentV2::setOnMotorForceChangedFunctionName(const Ogre::String& name)
    {
        this->onMotorForceChangedFunctionName->setValue(name);
        this->onMotorForceChangedFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), name + "(manip, dt)");
    }
    Ogre::String PhysicsActiveVehicleComponentV2::getOnMotorForceChangedFunctionName() const
    {
        return this->onMotorForceChangedFunctionName->getString();
    }

    void PhysicsActiveVehicleComponentV2::setOnHandBrakeChangedFunctionName(const Ogre::String& name)
    {
        this->onHandBrakeChangedFunctionName->setValue(name);
        this->onHandBrakeChangedFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), name + "(manip, dt)");
    }
    Ogre::String PhysicsActiveVehicleComponentV2::getOnHandBrakeChangedFunctionName() const
    {
        return this->onHandBrakeChangedFunctionName->getString();
    }

    void PhysicsActiveVehicleComponentV2::setOnBrakeChangedFunctionName(const Ogre::String& name)
    {
        this->onBrakeChangedFunctionName->setValue(name);
        this->onBrakeChangedFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), name + "(manip, dt)");
    }
    Ogre::String PhysicsActiveVehicleComponentV2::getOnBrakeChangedFunctionName() const
    {
        return this->onBrakeChangedFunctionName->getString();
    }

    void PhysicsActiveVehicleComponentV2::setCanDrive(bool canDrive)
    {
        if (this->physicsBody)
        {
            static_cast<OgreNewt::VehicleV2*>(this->physicsBody)->setCanDrive(canDrive);
        }
    }

    void PhysicsActiveVehicleComponentV2::setTireDirectionSwap(bool swap)
    {
        this->tireDirectionSwap->setValue(swap);
        if (this->physicsBody)
        {
            // Live update: flips all already-registered tire invRadius values.
            static_cast<OgreNewt::VehicleV2*>(this->physicsBody)->setTireDirectionSwap(swap);
        }
    }

    bool PhysicsActiveVehicleComponentV2::getTireDirectionSwap() const
    {
        return this->tireDirectionSwap->getBool();
    }

    void PhysicsActiveVehicleComponentV2::setSteeringStrength(Ogre::Real strength)
    {
        this->steeringStrength->setValue(strength);
        if (this->physicsBody)
        {
            static_cast<OgreNewt::VehicleV2*>(this->physicsBody)->setSteeringStrength(strength);
        }
    }

    Ogre::Real PhysicsActiveVehicleComponentV2::getSteeringStrength() const
    {
        return this->steeringStrength->getReal();
    }

    void PhysicsActiveVehicleComponentV2::applySpinAxisToVehicle(OgreNewt::VehicleV2* vehicle)
    {
        if (!vehicle)
        {
            return;
        }
        const Ogre::String axis = this->tireSpinAxis->getListSelectedValue();
        int idx = 0; // default X
        if (axis == "Y")
        {
            idx = 1;
        }
        else if (axis == "Z")
        {
            idx = 2;
        }
        vehicle->setSpinAxis(idx);
    }

    void PhysicsActiveVehicleComponentV2::setTireSpinAxis(const Ogre::String& axis)
    {
        this->tireSpinAxis->setListSelectedValue(axis);
        if (this->physicsBody)
        {
            this->applySpinAxisToVehicle(static_cast<OgreNewt::VehicleV2*>(this->physicsBody));
        }
    }

    Ogre::String PhysicsActiveVehicleComponentV2::getTireSpinAxis() const
    {
        return this->tireSpinAxis->getListSelectedValue();
    }

    bool PhysicsActiveVehicleComponentV2::isAirborne(void) const
    {
        if (this->physicsBody)
        {
            return static_cast<OgreNewt::VehicleV2*>(this->physicsBody)->isAirborne();
        }
        return false;
    }

    Ogre::Vector3 PhysicsActiveVehicleComponentV2::getVehicleForce(void) const
    {
        if (this->physicsBody)
        {
            return static_cast<OgreNewt::VehicleV2*>(this->physicsBody)->getVehicleForce();
        }
        return Ogre::Vector3::ZERO;
    }

    OgreNewt::VehicleV2* PhysicsActiveVehicleComponentV2::getVehicleV2(void) const
    {
        return static_cast<OgreNewt::VehicleV2*>(this->physicsBody);
    }

    void PhysicsActiveVehicleComponentV2::setActivated(bool activated)
    {
        PhysicsComponent::setActivated(activated);
        this->activated->setValue(activated);

        if (nullptr == this->physicsBody)
        {
            return;
        }

        if (nullptr != this->ogreNewt && this->ogreNewt->isShuttingDown())
        {
            return;
        }

        if (false == activated)
        {
            this->physicsBody->removeForceAndTorqueCallback();

            // Only remove from world if it was actually added.
            // During createDynamicBody the body is not yet in the world —
            // in that case just store the flag and skip, the body will
            // simply never be added (enqueuePhysics checks m_isInWorld).
            if (this->physicsBody->isInWorld())
            {
                this->ogreNewt->Sync();
                this->physicsBody->removeFromWorld();
            }
        }
        else
        {
            // Re-add to world if not already in it
            if (false == this->physicsBody->isInWorld())
            {
                this->ogreNewt->Sync();
                this->physicsBody->addToWorld();
            }

            if (this->savedMass > 0.0f)
            {
                this->physicsBody->setMassMatrix(this->savedMass, this->savedInertia);
            }
            this->physicsBody->unFreeze();
            // Custom force callback: base gravity + vehicle impulses
            this->physicsBody->setCustomForceAndTorqueCallback<PhysicsActiveVehicleComponentV2>(&PhysicsActiveVehicleComponentV2::vehicleMoveCallback, this);
        }
    }

    void PhysicsActiveVehicleComponentV2::setGhost(bool ghost)
    {
        if (nullptr == this->physicsBody)
        {
            return;
        }

        if (nullptr != this->ogreNewt && this->ogreNewt->isShuttingDown())
        {
            return;
        }

        this->ghostActive = ghost;

        if (true == ghost)
        {
            // Remove force callback first — no more movement
            this->physicsBody->removeForceAndTorqueCallback();

            // Zero mass makes nd4 treat this as static in the solver,
            // but the body stays in the world so contacts still fire.
            // Safe to call from the logic thread because setMassMatrix
            // only writes m_mass/m_invMass — not touched by CalculateContacts.
            this->physicsBody->setMassMatrix(0.0f, Ogre::Vector3::ZERO);
            this->physicsBody->unFreeze();
            this->physicsBody->freeze();

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveVehicleComponentV2] Ghost ON for: " + this->gameObjectPtr->getName());
        }
        else
        {
            // Restore full dynamic simulation
            if (this->savedMass > 0.0f)
            {
                this->physicsBody->setMassMatrix(this->savedMass, this->savedInertia);
            }
            this->physicsBody->unFreeze();
            this->physicsBody->setCustomForceAndTorqueCallback<PhysicsActiveVehicleComponentV2>(&PhysicsActiveVehicleComponentV2::moveCallback, this);

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveVehicleComponentV2] Ghost OFF for: " + this->gameObjectPtr->getName());
        }
    }

    // =========================================================================
    // actualizeValue  (NOWA-Design live editing)
    // =========================================================================
    void PhysicsActiveVehicleComponentV2::actualizeValue(Variant* attribute)
    {
        PhysicsActiveComponent::actualizeCommonValue(attribute);

        if (AttrOnSteerAngleChangedFunctionName() == attribute->getName())
        {
            this->setOnSteerAngleChangedFunctionName(attribute->getString());
        }
        else if (AttrOnMotorForceChangedFunctionName() == attribute->getName())
        {
            this->setOnMotorForceChangedFunctionName(attribute->getString());
        }
        else if (AttrOnHandBrakeChangedFunctionName() == attribute->getName())
        {
            this->setOnHandBrakeChangedFunctionName(attribute->getString());
        }
        else if (AttrOnBrakeChangedFunctionName() == attribute->getName())
        {
            this->setOnBrakeChangedFunctionName(attribute->getString());
        }
        else if (AttrTireDirectionSwap() == attribute->getName())
        {
            this->setTireDirectionSwap(attribute->getBool());
        }
        else if (AttrSteeringStrength() == attribute->getName())
        {
            this->setSteeringStrength(attribute->getReal());
        }
        else if (AttrTireSpinAxis() == attribute->getName())
        {
            this->setTireSpinAxis(attribute->getListSelectedValue());
        }
        else
        {
            for (int i = 0; i < MAX_TIRES; ++i)
            {
                if (AttrTireId(i) == attribute->getName())
                {
                    this->setTireId(i, attribute->getULong());
                    break;
                }
            }
        }
    }

    // =========================================================================
    // writeXML
    // =========================================================================
    void PhysicsActiveVehicleComponentV2::writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc)
    {
        PhysicsActiveComponent::writeCommonProperties(propertiesXML, doc);

        // 2=int  6=real  7=string  8=vec2  9=vec3  10=vec4/quat  12=bool

        for (int i = 0; i < MAX_TIRES; ++i)
        {
            const Ogre::String attrName = "TireId_" + Ogre::StringConverter::toString(i);
            xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, attrName)));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->tireIds[i]->getULong())));
            propertiesXML->append_node(propertyXML);
        }

        auto writeStr = [&](const char* name, const Ogre::String& value)
        {
            xml_node<>* n = doc.allocate_node(node_element, "property");
            n->append_attribute(doc.allocate_attribute("type", "7"));
            n->append_attribute(doc.allocate_attribute("name", name));
            n->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, value)));
            propertiesXML->append_node(n);
        };

        writeStr("OnSteerAngleFunctionName", this->onSteerAngleChangedFunctionName->getString());
        writeStr("OnMotorForceFunctionName", this->onMotorForceChangedFunctionName->getString());
        writeStr("OnHandBrakeFunctionName", this->onHandBrakeChangedFunctionName->getString());
        writeStr("OnBrakeFunctionName", this->onBrakeChangedFunctionName->getString());

        // TireDirectionSwap (bool, type 12)
        {
            xml_node<>* n = doc.allocate_node(node_element, "property");
            n->append_attribute(doc.allocate_attribute("type", "12"));
            n->append_attribute(doc.allocate_attribute("name", "TireDirectionSwap"));
            n->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->tireDirectionSwap->getBool())));
            propertiesXML->append_node(n);
        }
        // SteeringStrength (real, type 6)
        {
            xml_node<>* n = doc.allocate_node(node_element, "property");
            n->append_attribute(doc.allocate_attribute("type", "6"));
            n->append_attribute(doc.allocate_attribute("name", "SteeringStrength"));
            n->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->steeringStrength->getReal())));
            propertiesXML->append_node(n);
        }
        // TireSpinAxis (string, type 7)
        {
            xml_node<>* n = doc.allocate_node(node_element, "property");
            n->append_attribute(doc.allocate_attribute("type", "7"));
            n->append_attribute(doc.allocate_attribute("name", "TireSpinAxis"));
            n->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->tireSpinAxis->getListSelectedValue())));
            propertiesXML->append_node(n);
        }
    }

    // =========================================================================
    // Name helpers
    // =========================================================================
    Ogre::String PhysicsActiveVehicleComponentV2::getClassName() const
    {
        return "PhysicsActiveVehicleComponentV2";
    }
    Ogre::String PhysicsActiveVehicleComponentV2::getParentClassName() const
    {
        return "PhysicsActiveComponent";
    }
    Ogre::String PhysicsActiveVehicleComponentV2::getParentParentClassName() const
    {
        return "PhysicsComponent";
    }

    // =========================================================================
    // Lua bindings
    // =========================================================================

    PhysicsActiveVehicleComponentV2* getPhysicsActiveVehicleComponentV2(GameObject* gameObject)
    {
        return makeStrongPtr<PhysicsActiveVehicleComponentV2>(gameObject->getComponent<PhysicsActiveVehicleComponentV2>()).get();
    }

    PhysicsActiveVehicleComponentV2* getPhysicsActiveVehicleComponentV2FromIndex(GameObject* gameObject, unsigned int occurrenceIndex)
    {
        return makeStrongPtr<PhysicsActiveVehicleComponentV2>(gameObject->getComponentWithOccurrence<PhysicsActiveVehicleComponentV2>(occurrenceIndex)).get();
    }

    PhysicsActiveVehicleComponentV2* getPhysicsActiveVehicleComponentV2FromName(GameObject* gameObject, const Ogre::String& name)
    {
        return makeStrongPtr<PhysicsActiveVehicleComponentV2>(gameObject->getComponentFromName<PhysicsActiveVehicleComponentV2>(name)).get();
    }

    void PhysicsActiveVehicleComponentV2::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
    {
        module(lua)
        [
            class_<OgreNewt::VehicleDrivingManipulationV2>("VehicleDrivingManipulationV2")
            .def("setSteerAngle", &OgreNewt::VehicleDrivingManipulationV2::setSteerAngle)
            .def("getSteerAngle", &OgreNewt::VehicleDrivingManipulationV2::getSteerAngle)
            .def("setMotorForce", &OgreNewt::VehicleDrivingManipulationV2::setMotorForce)
            .def("getMotorForce", &OgreNewt::VehicleDrivingManipulationV2::getMotorForce)
            .def("setHandBrake", &OgreNewt::VehicleDrivingManipulationV2::setHandBrake)
            .def("getHandBrake", &OgreNewt::VehicleDrivingManipulationV2::getHandBrake)
            .def("setBrake", &OgreNewt::VehicleDrivingManipulationV2::setBrake)
            .def("getBrake", &OgreNewt::VehicleDrivingManipulationV2::getBrake),

            class_<PhysicsActiveVehicleComponentV2, PhysicsActiveComponent>("PhysicsActiveVehicleComponentV2")
            .def("getParentClassName", &PhysicsActiveVehicleComponentV2::getParentClassName)
            .def("getVehicleForce", &PhysicsActiveVehicleComponentV2::getVehicleForce)
            .def("isAirborne", &PhysicsActiveVehicleComponentV2::isAirborne)
            .def("setCanDrive", &PhysicsActiveVehicleComponentV2::setCanDrive)
            .def("setTireDirectionSwap", &PhysicsActiveVehicleComponentV2::setTireDirectionSwap)
            .def("getTireDirectionSwap", &PhysicsActiveVehicleComponentV2::getTireDirectionSwap)
            .def("setSteeringStrength", &PhysicsActiveVehicleComponentV2::setSteeringStrength)
            .def("getSteeringStrength", &PhysicsActiveVehicleComponentV2::getSteeringStrength)
            .def("setTireSpinAxis", &PhysicsActiveVehicleComponentV2::setTireSpinAxis)
            .def("getTireSpinAxis", &PhysicsActiveVehicleComponentV2::getTireSpinAxis)
            .def("applyWheelie", &PhysicsActiveVehicleComponentV2::applyWheelie)
            .def("applyDrift", &PhysicsActiveVehicleComponentV2::applyDrift)
            .def("applyPitch", &PhysicsActiveVehicleComponentV2::applyPitch)
            .def("setOnSteerAngleChangedFunctionName", &PhysicsActiveVehicleComponentV2::setOnSteerAngleChangedFunctionName)
            .def("getOnSteerAngleChangedFunctionName", &PhysicsActiveVehicleComponentV2::getOnSteerAngleChangedFunctionName)
            .def("setOnMotorForceChangedFunctionName", &PhysicsActiveVehicleComponentV2::setOnMotorForceChangedFunctionName)
            .def("getOnMotorForceChangedFunctionName", &PhysicsActiveVehicleComponentV2::getOnMotorForceChangedFunctionName)
            .def("setOnHandBrakeChangedFunctionName", &PhysicsActiveVehicleComponentV2::setOnHandBrakeChangedFunctionName)
            .def("getOnHandBrakeChangedFunctionName", &PhysicsActiveVehicleComponentV2::getOnHandBrakeChangedFunctionName)
            .def("setOnBrakeChangedFunctionName", &PhysicsActiveVehicleComponentV2::setOnBrakeChangedFunctionName)
            .def("getOnBrakeChangedFunctionName", &PhysicsActiveVehicleComponentV2::getOnBrakeChangedFunctionName)
        ];

        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveVehicleComponentV2", "class inherits PhysicsActiveComponent", "Impulse-based vehicle. No joints or raycast required.");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveVehicleComponentV2", "Vector3 getVehicleForce()", "Gets current vehicle longitudinal force.");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveVehicleComponentV2", "boolean isAirborne()", "Gets whether the vehicle is airborne.");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveVehicleComponentV2", "void setCanDrive(boolean canDrive)", "Enables or disables vehicle control without removing the body.");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveVehicleComponentV2", "void setTireDirectionSwap(boolean swap)", "Flips the spin direction of all tires. Enable if tires roll backward when driving forward.");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveVehicleComponentV2", "boolean getTireDirectionSwap()", "Gets whether tire spin direction is swapped.");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveVehicleComponentV2", "void setSteeringStrength(number strength)",
            "Scales the yaw-rate target (default 1.0). Increase (2–4) for sharper turns, decrease for less responsive steering.");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveVehicleComponentV2", "number getSteeringStrength()", "Gets the current steering strength multiplier.");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveVehicleComponentV2", "void setTireSpinAxis(string axis)", "Sets the chassis-space spin axis: 'X', 'Y' or 'Z'. Start with X; switch to Z if tires flip like a coin.");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveVehicleComponentV2", "string getTireSpinAxis()", "Gets the current tire spin axis ('X', 'Y' or 'Z').");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveVehicleComponentV2", "void applyWheelie(number strength)", "Applies a wheelie stunt by lifting the front.");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveVehicleComponentV2", "void applyDrift(boolean left, number strength, number steeringStrength)", "Applies a drift stunt at the given strength and steering strength.");
        LuaScriptApi::getInstance()->addClassToCollection("PhysicsActiveVehicleComponentV2", "void applyPitch(number strength, number dt)", "Applies pitch angular impulse. Units: torque-like, e.g. 1500..6000.");

        LuaScriptApi::getInstance()->addClassToCollection("VehicleDrivingManipulationV2", "VehicleDrivingManipulationV2", "Used in vehicle V2 callbacks to set driving parameters.");
        LuaScriptApi::getInstance()->addClassToCollection("VehicleDrivingManipulationV2", "void setSteerAngle(number degrees)", "Sets steer angle in degrees (e.g. -45..45).");
        LuaScriptApi::getInstance()->addClassToCollection("VehicleDrivingManipulationV2", "number getSteerAngle()", "Gets the current steer angle.");
        LuaScriptApi::getInstance()->addClassToCollection("VehicleDrivingManipulationV2", "void setMotorForce(number force)", "Sets motor force. Same scale as old vehicle (e.g. 10000*120*dt).");
        LuaScriptApi::getInstance()->addClassToCollection("VehicleDrivingManipulationV2", "number getMotorForce()", "Gets the current motor force.");
        LuaScriptApi::getInstance()->addClassToCollection("VehicleDrivingManipulationV2", "void setHandBrake(number factor)", "Sets hand-brake factor (e.g. 5.5). Enables drifting.");
        LuaScriptApi::getInstance()->addClassToCollection("VehicleDrivingManipulationV2", "number getHandBrake()", "Gets the current hand-brake factor.");
        LuaScriptApi::getInstance()->addClassToCollection("VehicleDrivingManipulationV2", "void setBrake(number factor)", "Sets brake factor (e.g. 7.5). Damps forward velocity.");
        LuaScriptApi::getInstance()->addClassToCollection("VehicleDrivingManipulationV2", "number getBrake()", "Gets the current brake factor.");

        gameObjectClass.def("getPhysicsActiveVehicleComponentV2", (PhysicsActiveVehicleComponentV2 * (*)(GameObject*)) & getPhysicsActiveVehicleComponentV2);
        gameObjectClass.def("getPhysicsActiveVehicleComponentV2FromIndex", (PhysicsActiveVehicleComponentV2 * (*)(GameObject*, unsigned int)) & getPhysicsActiveVehicleComponentV2FromIndex);
        gameObjectClass.def("getPhysicsActiveVehicleComponentV2FromName", &getPhysicsActiveVehicleComponentV2FromName);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PhysicsActiveVehicleComponentV2 getPhysicsActiveVehicleComponentV2()", "Gets the component.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PhysicsActiveVehicleComponentV2 getPhysicsActiveVehicleComponentV2FromIndex(unsigned int occurrenceIndex)", "Gets the component by occurrence index.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PhysicsActiveVehicleComponentV2 getPhysicsActiveVehicleComponentV2FromName(String name)", "Gets the component from name.");

        gameObjectControllerClass.def("castPhysicsActiveVehicleComponentV2", &GameObjectController::cast<PhysicsActiveVehicleComponentV2>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "PhysicsActiveVehicleComponentV2 castPhysicsActiveVehicleComponentV2(PhysicsActiveVehicleComponentV2 other)",
            "Casts an incoming type from function for lua auto completion.");
    }

} // namespace NOWA
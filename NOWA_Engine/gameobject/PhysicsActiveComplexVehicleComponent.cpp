#include "NOWAPrecompiled.h"
#include "PhysicsActiveComplexVehicleComponent.h"
#include "PhysicsComponent.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "main/AppStateManager.h"

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    ///////////////////////////////////////////////////////////////
    // PhysicsComplexVehicleCallback
    ///////////////////////////////////////////////////////////////

    PhysicsActiveComplexVehicleComponent::PhysicsComplexVehicleCallback::PhysicsComplexVehicleCallback(PhysicsActiveComplexVehicleComponent* owner, LuaScript* luaScript, OgreNewt::World* ogreNewt,
        const Ogre::String& onSteerAngleChangedFunctionName, const Ogre::String& onMotorForceChangedFunctionName, const Ogre::String& onHandBrakeChangedFunctionName, const Ogre::String& onBrakeChangedFunctionName,
        const Ogre::String& onTireContactFunctionName)
        : OgreNewt::ComplexVehicleCallback(),
        owner(owner),
        luaScript(luaScript),
        ogreNewt(ogreNewt),
        onSteerAngleChangedFunctionName(onSteerAngleChangedFunctionName),
        onMotorForceChangedFunctionName(onMotorForceChangedFunctionName),
        onHandBrakeChangedFunctionName(onHandBrakeChangedFunctionName),
        onBrakeChangedFunctionName(onBrakeChangedFunctionName),
        onTireContactFunctionName(onTireContactFunctionName),
        vehicleDrivingManipulation(new ComplexVehicleDrivingManipulation())
    {
        
    }

    PhysicsActiveComplexVehicleComponent::PhysicsComplexVehicleCallback::~PhysicsComplexVehicleCallback()
    {
        delete this->vehicleDrivingManipulation;
        this->vehicleDrivingManipulation = nullptr;
    }

    Ogre::Real PhysicsActiveComplexVehicleComponent::PhysicsComplexVehicleCallback::onSteerAngleChanged(const OgreNewt::ComplexVehicle* visitor, const OgreNewt::ComplexVehicleTire* tire, Ogre::Real timestep)
    {
        this->vehicleDrivingManipulation->steerAngle = 0.0f;

        if (nullptr == this->owner)
        {
            return 0.0f;
        }

        PhysicsComponent* visitorPhysicsComponent = OgreNewt::any_cast<PhysicsComponent*>(visitor->getChassis()->getUserData());
        if (nullptr != visitorPhysicsComponent)
        {
            if (nullptr != this->luaScript && this->luaScript->isCompiled() &&
                false == this->onSteerAngleChangedFunctionName.empty())
            {
                // Is safe run in newton thread
                this->luaScript->callTableFunction(this->onSteerAngleChangedFunctionName, this->vehicleDrivingManipulation, timestep);
            }
        }

        // Use value provided on the component
        return this->vehicleDrivingManipulation->getSteerAngle();
    }

    Ogre::Real PhysicsActiveComplexVehicleComponent::PhysicsComplexVehicleCallback::onMotorForceChanged(const OgreNewt::ComplexVehicle* visitor, const OgreNewt::ComplexVehicleTire* tire, Ogre::Real timestep)
    {
        this->vehicleDrivingManipulation->motorForce = 0.0f;

        if (nullptr == this->owner)
        {
            return 0.0f;
        }

        PhysicsComponent* visitorPhysicsComponent = OgreNewt::any_cast<PhysicsComponent*>(visitor->getChassis()->getUserData());
        if (nullptr != visitorPhysicsComponent)
        {
            if (nullptr != this->luaScript && this->luaScript->isCompiled() &&
                false == this->onMotorForceChangedFunctionName.empty())
            {
                // Is safe run in newton thread
                this->luaScript->callTableFunction(this->onMotorForceChangedFunctionName, this->vehicleDrivingManipulation, timestep);
            }
        }

        return this->vehicleDrivingManipulation->getMotorForce();
    }

    Ogre::Real PhysicsActiveComplexVehicleComponent::PhysicsComplexVehicleCallback::onHandBrakeChanged(const OgreNewt::ComplexVehicle* visitor, const OgreNewt::ComplexVehicleTire* tire, Ogre::Real timestep)
    {
        this->vehicleDrivingManipulation->handBrake = 0.0f;

        if (nullptr == this->owner)
        {
            return 0.0f;
        }

        PhysicsComponent* visitorPhysicsComponent = OgreNewt::any_cast<PhysicsComponent*>(visitor->getChassis()->getUserData());
        if (nullptr != visitorPhysicsComponent)
        {
            if (nullptr != this->luaScript && this->luaScript->isCompiled() &&
                false == this->onHandBrakeChangedFunctionName.empty())
            {
                // Is safe run in newton thread
                this->luaScript->callTableFunction(this->onHandBrakeChangedFunctionName, this->vehicleDrivingManipulation, timestep);
            }
        }

        return this->vehicleDrivingManipulation->getHandBrake();
    }

    Ogre::Real PhysicsActiveComplexVehicleComponent::PhysicsComplexVehicleCallback::onBrakeChanged(const OgreNewt::ComplexVehicle* visitor, const OgreNewt::ComplexVehicleTire* tire, Ogre::Real timestep)
    {
        this->vehicleDrivingManipulation->brake = 0.0f;

        if (nullptr == this->owner)
        {
            return 0.0f;
        }

        PhysicsComponent* visitorPhysicsComponent = OgreNewt::any_cast<PhysicsComponent*>(visitor->getChassis()->getUserData());
        if (nullptr != visitorPhysicsComponent)
        {
            if (nullptr != this->luaScript && this->luaScript->isCompiled() &&
                false == this->onBrakeChangedFunctionName.empty())
            {
                // Is safe run in newton thread
                this->luaScript->callTableFunction(this->onBrakeChangedFunctionName, this->vehicleDrivingManipulation, timestep);
            }
        }

        return this->vehicleDrivingManipulation->getBrake();
    }

    void PhysicsActiveComplexVehicleComponent::PhysicsComplexVehicleCallback::onTireContact(const OgreNewt::ComplexVehicleTire* tire, const Ogre::String& tireName, OgreNewt::Body* hitBody,
        const Ogre::Vector3& contactPosition, const Ogre::Vector3& contactNormal, Ogre::Real penetration)
    {
        if (nullptr == this->owner || nullptr == hitBody)
        {
            return;
        }

        PhysicsComponent* hitPhysicsComponent = OgreNewt::any_cast<PhysicsComponent*>(hitBody->getUserData());
        if (nullptr == hitPhysicsComponent)
        {
            return;
        }

        auto gameObjectPtr = hitPhysicsComponent->getOwner();
        if (!gameObjectPtr)
        {
            return;
        }

        /*this->vehicleDrivingManipulation->setHitGameObject(gameObjectPtr);
        this->vehicleDrivingManipulation->setContactPosition(contactPosition);
        this->vehicleDrivingManipulation->setContactNormal(contactNormal);
        this->vehicleDrivingManipulation->setPenetration(penetration);
        this->vehicleDrivingManipulation->setTireName(tireName);*/

        if (nullptr != this->luaScript && this->luaScript->isCompiled() &&
            false == this->onTireContactFunctionName.empty())
        {
            // Is safe run in newton thread
            this->luaScript->callTableFunction(this->onTireContactFunctionName, this->vehicleDrivingManipulation);
        }
    }

    ///////////////////////////////////////////////////////////////
    // PhysicsActiveComplexVehicleComponent
    ///////////////////////////////////////////////////////////////

    PhysicsActiveComplexVehicleComponent::PhysicsActiveComplexVehicleComponent(void)
        : PhysicsActiveComponent(),
        onSteerAngleChangedFunctionName(new Variant(PhysicsActiveComplexVehicleComponent::AttrOnSteerAngleChangedFunctionName(), Ogre::String(""), this->attributes)),
        onMotorForceChangedFunctionName(new Variant(PhysicsActiveComplexVehicleComponent::AttrOnMotorForceChangedFunctionName(), Ogre::String(""), this->attributes)),
        onHandBrakeChangedFunctionName(new Variant(PhysicsActiveComplexVehicleComponent::AttrOnHandBrakeChangedFunctionName(), Ogre::String(""), this->attributes)),
        onBrakeChangedFunctionName(new Variant(PhysicsActiveComplexVehicleComponent::AttrOnBrakeChangedFunctionName(), Ogre::String(""), this->attributes)),
        onTireContactFunctionName(new Variant(PhysicsActiveComplexVehicleComponent::AttrOnTireContactFunctionName(), Ogre::String(""), this->attributes)),
        complexVehicle(nullptr),
        vehicleCallback(nullptr),
        vehicleForce(Ogre::Vector3::ZERO),
        canDrive(true),
        stuckTime(0.0f),
        maxStuckTime(3.0f)
    {
        // Reasonable defaults similar to PhysicsActiveVehicleComponent
        this->mass->setValue(1200.0f);
        this->massOrigin->setValue(Ogre::Vector3(0.025f, 0.15f, 0.0f));

        this->collisionSize->setValue(Ogre::Vector3(0.4f, 0.3f, 0.0f));
        this->collisionPosition->setValue(Ogre::Vector3(0.0f, 0.4f, 0.0f));
        this->collisionType->setListSelectedValue("Capsule");
    }

    PhysicsActiveComplexVehicleComponent::~PhysicsActiveComplexVehicleComponent(void)
    {
        this->onSteerAngleChangedFunctionName = nullptr;
        this->onMotorForceChangedFunctionName = nullptr;
        this->onHandBrakeChangedFunctionName = nullptr;
        this->onBrakeChangedFunctionName = nullptr;
        this->onTireContactFunctionName = nullptr;

        if (this->complexVehicle)
        {
            delete this->complexVehicle;
            this->complexVehicle = nullptr;
        }

        if (this->vehicleCallback)
        {
            delete this->vehicleCallback;
            this->vehicleCallback = nullptr;
        }
    }

    bool PhysicsActiveComplexVehicleComponent::init(xml_node<>*& propertyElement)
    {
        PhysicsActiveComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OnSteerAngleChangedFunctionName")
        {
            this->onSteerAngleChangedFunctionName->setValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OnMotorForceChangedFunctionName")
        {
            this->onMotorForceChangedFunctionName->setValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OnHandBrakeChangedFunctionName")
        {
            this->onHandBrakeChangedFunctionName->setValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OnBrakeChangedFunctionName")
        {
            this->onBrakeChangedFunctionName->setValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OnTireContactFunctionName")
        {
            this->onTireContactFunctionName->setValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        return true;
    }

    bool PhysicsActiveComplexVehicleComponent::postInit(void)
    {
        PhysicsActiveComponent::postInit();
        return true;
    }

    void PhysicsActiveComplexVehicleComponent::onRemoveComponent(void)
    {
        PhysicsActiveComponent::onRemoveComponent();
    }

    Ogre::String PhysicsActiveComplexVehicleComponent::getClassName(void) const
    {
        return "PhysicsActiveComplexVehicleComponent";
    }

    Ogre::String PhysicsActiveComplexVehicleComponent::getParentClassName(void) const
    {
        return "PhysicsActiveComponent";
    }

    void PhysicsActiveComplexVehicleComponent::update(Ogre::Real dt, bool notSimulating)
    {
        PhysicsActiveComponent::update(dt, notSimulating);

        if (true == notSimulating)
        {
            return;
        }

        if (false == this->canDrive || nullptr == this->complexVehicle)
        {
            return;
        }

        // Cache total vehicle force for gameplay/GUI
        this->vehicleForce = this->complexVehicle->getVehicleForce();
    }

    GameObjectCompPtr PhysicsActiveComplexVehicleComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        PhysicsActiveComplexVehicleCompPtr clonedCompPtr(boost::make_shared<PhysicsActiveComplexVehicleComponent>());

        // clonedCompPtr->setType(this->type->getString());
        // clonedCompPtr->internalSetPriorId(this->id->getULong());

        clonedCompPtr->setActivated(this->activated->getBool());
        clonedCompPtr->setMass(this->mass->getReal());
        clonedCompPtr->setMassOrigin(this->massOrigin->getVector3());
        clonedCompPtr->setLinearDamping(this->linearDamping->getReal());
        clonedCompPtr->setAngularDamping(this->angularDamping->getVector3());
        clonedCompPtr->setGravity(this->gravity->getVector3());
        clonedCompPtr->setGravitySourceCategory(this->gravitySourceCategory->getString());
        clonedCompPtr->setConstraintDirection(this->constraintDirection->getVector3());
        clonedCompPtr->setSpeed(this->speed->getReal());
        clonedCompPtr->setMaxSpeed(this->maxSpeed->getReal());
        clonedCompPtr->setCollisionType(this->collisionType->getListSelectedValue());
        clonedCompPtr->setCollisionDirection(this->collisionDirection->getVector3());
        clonedCompPtr->setCollisionSize(this->collisionSize->getVector3());
        clonedCompPtr->setCollisionPosition(this->collisionPosition->getVector3());

        clonedCompPtr->onSteerAngleChangedFunctionName->setValue(this->onSteerAngleChangedFunctionName->getString());
        clonedCompPtr->onMotorForceChangedFunctionName->setValue(this->onMotorForceChangedFunctionName->getString());
        clonedCompPtr->onHandBrakeChangedFunctionName->setValue(this->onHandBrakeChangedFunctionName->getString());
        clonedCompPtr->onBrakeChangedFunctionName->setValue(this->onBrakeChangedFunctionName->getString());
        clonedCompPtr->onTireContactFunctionName->setValue(this->onTireContactFunctionName->getString());

        clonedGameObjectPtr->addComponent(clonedCompPtr);
        clonedCompPtr->setOwner(clonedGameObjectPtr);

        return clonedCompPtr;
    }

    bool PhysicsActiveComplexVehicleComponent::connect(void)
    {
        bool success = PhysicsActiveComponent::connect();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveVehicleComponent] Connect physics active vehicle component for game object: "
            + this->gameObjectPtr->getName());


        // Note: Since vehicle is created on the fly during connect, also its init position must be set there, instead like in physicsactivecomponent in postinit!
        this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
        this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
        this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();

        // Special case: Must be done in connect, because lua script is involved, and order important.
        // Else: E.g. if creating this component, creating body to early, lua script would be 0, because the user would add the lua script component later
        if (false == this->createDynamicBody())
            return false;

        this->setCanDrive(this->activated->getBool());

        if (nullptr != this->physicsBody && true == this->bShowDebugData)
        {
            ENQUEUE_RENDER_COMMAND_WAIT("PhysicsActiveVehicleComponent::showDebugData",
                {
                    this->physicsBody->showDebugCollision(false, this->bShowDebugData);
                });
        }

        return success;
    }

    bool PhysicsActiveComplexVehicleComponent::disconnect(void)
    {
        bool success = PhysicsActiveComponent::disconnect();

        this->stuckTime = 0.0f;

        if (nullptr != this->physicsBody)
        {
            static_cast<OgreNewt::Vehicle*>(this->physicsBody)->removeAllTires();
            delete this->physicsBody;
            this->physicsBody = nullptr;
        }

        return success;
    }

    void PhysicsActiveComplexVehicleComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        PhysicsActiveComponent::writeXML(propertiesXML, doc);

        xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "OnSteerAngleChangedFunctionName"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->onSteerAngleChangedFunctionName->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "OnMotorForceChangedFunctionName"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->onMotorForceChangedFunctionName->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "OnHandBrakeChangedFunctionName"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->onHandBrakeChangedFunctionName->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "OnBrakeChangedFunctionName"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->onBrakeChangedFunctionName->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "OnTireContactFunctionName"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->onTireContactFunctionName->getString())));
        propertiesXML->append_node(propertyXML);
    }

    bool PhysicsActiveComplexVehicleComponent::createDynamicBody(void)
    {
        this->destroyCollision();
        this->destroyBody();

        Ogre::Vector3 inertia = Ogre::Vector3(1.0f, 1.0f, 1.0f);

        Ogre::Quaternion collisionOrientation = Ogre::Quaternion::IDENTITY; // this->gameObjectPtr->getSceneNode()->getOrientation();
        if (Ogre::Vector3::ZERO != this->collisionDirection->getVector3())
        {
            collisionOrientation = MathHelper::getInstance()->degreesToQuat(this->collisionDirection->getVector3());
        }

        Ogre::Vector3 calculatedMassOrigin = Ogre::Vector3::ZERO;
        Ogre::Real weightedMass = this->mass->getReal(); /** scale.x * scale.y * scale.z;*/ // scale is not used anymore, because if big game objects are scaled down, the mass is to low!

        OgreNewt::CollisionPtr collisionPtr;

        ENQUEUE_RENDER_COMMAND_MULTI_WAIT("PhysicsComponent::createDynamicCollision", _4(&inertia, &collisionPtr, collisionOrientation, &calculatedMassOrigin),
            {
                collisionPtr = this->createDynamicCollision(inertia, this->collisionSize->getVector3(), this->collisionPosition->getVector3(),
                                                    collisionOrientation, calculatedMassOrigin, this->gameObjectPtr->getCategoryId());
            });

        this->physicsBody = new OgreNewt::ComplexVehicle(this->ogreNewt, this->gameObjectPtr->getSceneManager(), this->gameObjectPtr->getDefaultDirection(), collisionPtr, weightedMass,
            this->collisionPosition->getVector3(), this->massOrigin->getVector3(), new PhysicsComplexVehicleCallback(this, this->gameObjectPtr->getLuaScript(), this->ogreNewt,
                this->onSteerAngleChangedFunctionName->getString(),
                this->onMotorForceChangedFunctionName->getString(),
                this->onHandBrakeChangedFunctionName->getString(),
                this->onBrakeChangedFunctionName->getString(),
                this->onTireContactFunctionName->getString()));

        this->physicsBody->setGravity(this->gravity->getVector3());

        // set mass origin
        //this->physicsBody->setCenterOfMass(calculatedMassOrigin);

        //if (this->collisionType->getListSelectedValue() == "ConvexHull")
        //{
        //	this->physicsBody->setConvexIntertialMatrix(inertia, calculatedMassOrigin);
        //}

        //// Apply mass and scale to inertia (the bigger the object, the more mass)
        //inertia *= weightedMass;
        //this->physicsBody->setMassMatrix(weightedMass, inertia);

        if (this->linearDamping->getReal() != 0.0f)
        {
            this->physicsBody->setLinearDamping(this->linearDamping->getReal());
        }
        if (this->angularDamping->getVector3() != Ogre::Vector3::ZERO)
        {
            this->physicsBody->setAngularDamping(this->angularDamping->getVector3());
        }

        // Can be used if in vehicle: vehicle->m_curPosit = vehiclePos; etc. is not used!

        // For fixed Update: Does not work, called to often
        // this->physicsBody->setNodeUpdateNotify<PhysicsActiveComponent>(&PhysicsActiveComponent::updateCallback, this);

        this->setActivated(this->activated->getBool());
        this->setGyroscopicTorqueEnabled(this->gyroscopicTorque->getBool());

        // set user data for ogrenewt
        this->physicsBody->setUserData(OgreNewt::Any(dynamic_cast<PhysicsComponent*>(this)));
        this->physicsBody->attachNode(this->gameObjectPtr->getSceneNode());

        this->setPosition(this->initialPosition);
        this->setOrientation(this->initialOrientation);

        // Must be set after body set its position! And also the current orientation (initialOrientation) must be correct! Else weird rotation behavior occurs
        this->setConstraintAxis(this->constraintAxis->getVector3());
        // Pin the object stand in pose and not fall down
        this->setConstraintDirection(this->constraintDirection->getVector3());

        this->setCollidable(this->collidable->getBool());

        this->physicsBody->setType(this->gameObjectPtr->getCategoryId());

        const auto materialId = AppStateManager::getSingletonPtr()->getGameObjectController()->getMaterialID(this->gameObjectPtr.get(), this->ogreNewt);
        this->physicsBody->setMaterialGroupID(materialId);

        return true;
    }

    bool PhysicsActiveComplexVehicleComponent::isVehicleTippedOver(void)
    {
        // simple placeholder: you can copy your old logic here
        return false;
    }

    bool PhysicsActiveComplexVehicleComponent::isVehicleStuck(Ogre::Real dt)
    {
        // placeholder for more elaborate behavior
        return false;
    }

    void PhysicsActiveComplexVehicleComponent::correctVehicleOrientation(void)
    {
        // placeholder – fill with your old re-orientation logic if needed
    }

    OgreNewt::ComplexVehicle* PhysicsActiveComplexVehicleComponent::getComplexVehicle(void) const
    {
        return this->complexVehicle;
    }

    Ogre::Vector3 PhysicsActiveComplexVehicleComponent::getVehicleForce(void) const
    {
        return this->vehicleForce;
    }

    bool PhysicsActiveComplexVehicleComponent::getCanDrive(void) const
    {
        return this->canDrive;
    }

    void PhysicsActiveComplexVehicleComponent::setCanDrive(bool canDrive)
    {
        this->canDrive = canDrive;

        if (nullptr != this->complexVehicle)
        {
            this->complexVehicle->setCanDrive(canDrive);
        }
    }

    void PhysicsActiveComplexVehicleComponent::applyWheelie(Ogre::Real strength)
    {
        this->applyOmegaForce(this->getOrientation() * Ogre::Vector3(0.0f, 0.0f, strength));
    }

    void PhysicsActiveComplexVehicleComponent::applyDrift(bool left, Ogre::Real strength, Ogre::Real steeringStrength)
    {
        this->applyForce(Ogre::Vector3(0.0f, strength, 0.0f));

        if (true == left)
        {
            this->applyOmegaForce(this->getOrientation() * Ogre::Vector3(0.0f, steeringStrength, 0.0f));
        }
        else
        {
            this->applyOmegaForce(this->getOrientation() * Ogre::Vector3(0.0f, -steeringStrength, 0.0f));
        }
    }

    ///////////////////////////////////////////////////////////////
    // ComplexVehicleDrivingManipulation
    ///////////////////////////////////////////////////////////////

    ComplexVehicleDrivingManipulation::ComplexVehicleDrivingManipulation()
        : steerAngle(0.0f),
        motorForce(0.0f),
        handBrake(0.0f),
        brake(0.0f)
    {

    }

    ComplexVehicleDrivingManipulation::~ComplexVehicleDrivingManipulation()
    {

    }

    void ComplexVehicleDrivingManipulation::setSteerAngle(Ogre::Real steerAngle)
    {
        this->steerAngle = steerAngle;
    }

    Ogre::Real ComplexVehicleDrivingManipulation::getSteerAngle(void) const
    {
        return this->steerAngle;
    }

    void ComplexVehicleDrivingManipulation::setMotorForce(Ogre::Real motorForce)
    {
        this->motorForce = motorForce;
    }

    Ogre::Real ComplexVehicleDrivingManipulation::getMotorForce(void) const
    {
        return this->motorForce;
    }

    void ComplexVehicleDrivingManipulation::setHandBrake(Ogre::Real handBrake)
    {
        this->handBrake = handBrake;
    }

    Ogre::Real ComplexVehicleDrivingManipulation::getHandBrake(void) const
    {
        return this->handBrake;
    }

    void ComplexVehicleDrivingManipulation::setBrake(Ogre::Real brake)
    {
        this->brake = brake;
    }

    Ogre::Real ComplexVehicleDrivingManipulation::getBrake(void) const
    {
        return this->brake;
    }

}; // namespace NOWA

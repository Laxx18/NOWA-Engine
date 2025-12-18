#include "NOWAPrecompiled.h"
#include "PhysicsActiveVehicleComponent.h"
#include "PhysicsComponent.h"
#include "LuaScriptComponent.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	PhysicsActiveVehicleComponent::PhysicsVehicleCallback::PhysicsVehicleCallback(GameObject* owner, LuaScript* luaScript, OgreNewt::World* ogreNewt, const Ogre::String& onSteerAngleChangedFunctionName,
																				  const Ogre::String& onMotorForceChangedFunctionName, const Ogre::String& onHandBrakeChangedFunctionName, 
																				  const Ogre::String& onBrakeChangedFunctionName, const Ogre::String& onTireContactFunctionName)
		: OgreNewt::VehicleCallback(),
		owner(owner),
		luaScript(luaScript),
		ogreNewt(ogreNewt),
		onSteerAngleChangedFunctionName(onSteerAngleChangedFunctionName),
		onMotorForceChangedFunctionName(onMotorForceChangedFunctionName),
		onHandBrakeChangedFunctionName(onHandBrakeChangedFunctionName),
		onBrakeChangedFunctionName(onBrakeChangedFunctionName),
		onTireContactFunctionName(onTireContactFunctionName),
		vehicleDrivingManipulation(new VehicleDrivingManipulation())
	{
		if (nullptr == luaScript)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveVehicleComponent] Physics active vehicle cannot be controlled, because this game object: "
															+ owner->getName() + " has no lua script component.");
		}
	}

	PhysicsActiveVehicleComponent::PhysicsVehicleCallback::~PhysicsVehicleCallback()
	{
		if (nullptr != this->vehicleDrivingManipulation)
		{
			delete this->vehicleDrivingManipulation;
			this->vehicleDrivingManipulation = nullptr;
		}
	}

	Ogre::Real PhysicsActiveVehicleComponent::PhysicsVehicleCallback::onSteerAngleChanged(const OgreNewt::Vehicle* visitor, const OgreNewt::RayCastTire* tire, Ogre::Real dt)
	{
		// Resets the values, as they are volatile
		this->vehicleDrivingManipulation->steerAngle = 0.0f;

		PhysicsComponent* visitorPhysicsComponent = OgreNewt::any_cast<PhysicsComponent*>(visitor->getUserData());
		if (nullptr != visitorPhysicsComponent)
		{
			if (nullptr != this->luaScript && this->luaScript->isCompiled() && false == this->onSteerAngleChangedFunctionName.empty())
			{
				// Is safe run in newton thread
				this->luaScript->callTableFunction(this->onSteerAngleChangedFunctionName, this->vehicleDrivingManipulation, dt);
				return this->vehicleDrivingManipulation->steerAngle;
			}
		}
		return 0.0f;
	}

	Ogre::Real PhysicsActiveVehicleComponent::PhysicsVehicleCallback::onMotorForceChanged(const OgreNewt::Vehicle* visitor, const OgreNewt::RayCastTire* tire, Ogre::Real dt)
	{
		this->vehicleDrivingManipulation->motorForce = 0.0f;

		PhysicsComponent* visitorPhysicsComponent = OgreNewt::any_cast<PhysicsComponent*>(visitor->getUserData());
		if (nullptr != visitorPhysicsComponent)
		{
			if (nullptr != this->luaScript && this->luaScript->isCompiled() && false == this->onMotorForceChangedFunctionName.empty())
			{
				// Is safe run in newton thread
				this->luaScript->callTableFunction(this->onMotorForceChangedFunctionName, this->vehicleDrivingManipulation, dt);
				return this->vehicleDrivingManipulation->motorForce;
			}
		}
		return 0.0f;
	}

	Ogre::Real PhysicsActiveVehicleComponent::PhysicsVehicleCallback::onHandBrakeChanged(const OgreNewt::Vehicle* visitor, const OgreNewt::RayCastTire* tire, Ogre::Real dt)
	{
		this->vehicleDrivingManipulation->handBrake = 0.0f;

		PhysicsComponent* visitorPhysicsComponent = OgreNewt::any_cast<PhysicsComponent*>(visitor->getUserData());
		if (nullptr != visitorPhysicsComponent)
		{
			if (nullptr != this->luaScript && this->luaScript->isCompiled() && false == this->onHandBrakeChangedFunctionName.empty())
			{
				// Is safe run in newton thread
				this->luaScript->callTableFunction(this->onHandBrakeChangedFunctionName, this->vehicleDrivingManipulation, dt);
				return this->vehicleDrivingManipulation->handBrake;
			}
		}
		return 0.0f;
	}

	Ogre::Real PhysicsActiveVehicleComponent::PhysicsVehicleCallback::onBrakeChanged(const OgreNewt::Vehicle* visitor, const OgreNewt::RayCastTire* tire, Ogre::Real dt)
	{
		this->vehicleDrivingManipulation->brake = 0.0f;

		PhysicsComponent* visitorPhysicsComponent = OgreNewt::any_cast<PhysicsComponent*>(visitor->getUserData());
		if (nullptr != visitorPhysicsComponent)
		{
			if (nullptr != this->luaScript && this->luaScript->isCompiled() && false == this->onBrakeChangedFunctionName.empty())
			{
				// Is safe run in newton thread
				this->luaScript->callTableFunction(this->onBrakeChangedFunctionName, this->vehicleDrivingManipulation, dt);
				return this->vehicleDrivingManipulation->brake;
			}
		}
		return 0.0f;
	}

	void PhysicsActiveVehicleComponent::PhysicsVehicleCallback::onTireContact(const OgreNewt::RayCastTire* tire, const Ogre::String& tireName, OgreNewt::Body* hitBody, const Ogre::Vector3& contactPosition, const Ogre::Vector3& contactNormal, Ogre::Real penetration)
	{
		PhysicsComponent* hitPhysicsComponent = OgreNewt::any_cast<PhysicsComponent*>(hitBody->getUserData());
		if (nullptr != hitPhysicsComponent)
		{
			if (nullptr != this->luaScript && this->luaScript->isCompiled() && false == this->onTireContactFunctionName.empty())
			{
				// Is safe run in newton thread
				this->luaScript->callTableFunction(this->onTireContactFunctionName, tireName, hitPhysicsComponent, contactPosition, contactNormal, penetration);
			}
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	PhysicsActiveVehicleComponent::PhysicsActiveVehicleComponent()
		: PhysicsActiveComponent(),
		stuckTime(0.0f),
		maxStuckTime(5.0f),
		onSteerAngleChangedFunctionName(new Variant(PhysicsActiveVehicleComponent::AttrOnSteerAngleChangedFunctionName(), Ogre::String(""), this->attributes)),
		onMotorForceChangedFunctionName(new Variant(PhysicsActiveVehicleComponent::AttrOnMotorForceChangedFunctionName(), Ogre::String(""), this->attributes)),
		onHandBrakeChangedFunctionName(new Variant(PhysicsActiveVehicleComponent::AttrOnHandBrakeChangedFunctionName(), Ogre::String(""), this->attributes)),
		onBrakeChangedFunctionName(new Variant(PhysicsActiveVehicleComponent::AttrOnBrakeChangedFunctionName(), Ogre::String(""), this->attributes)),
		onTireContactFunctionName(new Variant(PhysicsActiveVehicleComponent::AttrOnTireContactFunctionName(), Ogre::String(""), this->attributes))
	{
		this->asSoftBody->setVisible(false);
		this->mass->setValue(1200.0f);
		this->massOrigin->setValue(Ogre::Vector3(0.025f, -0.25f, 0.0f));
		this->massOrigin->setDescription("For valid vehicle data, adjust this mass origin point to a valid vehicle mass center.");

		// Better values for creating a vehicle:
		this->collisionSize->setValue(Ogre::Vector3(0.4f, 0.3f, 0.0f));
		this->collisionPosition->setValue(Ogre::Vector3(0.0f, 0.4f, 0.0f));
		this->massOrigin->setValue(Ogre::Vector3(0.025f, 0.15f, 0.0f));
		this->collisionType->setListSelectedValue("Capsule");

		this->onSteerAngleChangedFunctionName->setDescription("Sets the function name to react in lua script to react when the steering angle for the specific tires shall change. E.g. onSteerAngleChanged(vehicleDrivingManipulation, dt)."
															"The function should set in the resulting steering angle via vehicle driving manipulation object, e.g. depending on user device input.");
		this->onSteerAngleChangedFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), onSteerAngleChangedFunctionName->getString() + "(vehicleDrivingManipulation, dt)");

		this->onMotorForceChangedFunctionName->setDescription("Sets the function name to react in lua script to react when the motor force for the specific tires shall change. E.g. onMotorForceChanged(vehicleDrivingManipulation, dt)."
															  "The function should set in the resulting motor force via vehicle driving manipulation object, e.g. depending on user device input.");
		this->onMotorForceChangedFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), onMotorForceChangedFunctionName->getString() + "(vehicleDrivingManipulation, dt)");

		this->onHandBrakeChangedFunctionName->setDescription("Sets the function name to react in lua script to react when the hand brake force shall change. E.g. onHandBrakeChanged(vehicleDrivingManipulation, dt)."
															  "The function should set in the resulting hand brake force via vehicle driving manipulation object, e.g. depending on user device input.");
		this->onHandBrakeChangedFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), onHandBrakeChangedFunctionName->getString() + "(vehicleDrivingManipulation, dt)");

		this->onBrakeChangedFunctionName->setDescription("Sets the function name to react in lua script to react when the brake force shall change. E.g. onBrakeChanged(vehicleDrivingManipulation, dt)."
															 "The function should set in the resulting brake force via vehicle driving manipulation object, e.g. depending on user device input.");
		this->onBrakeChangedFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), onBrakeChangedFunctionName->getString() + "(physicsComponent, vehicleDrivingManipulation, dt)");

		this->onTireContactFunctionName->setDescription("Sets the function name to react in lua script to react when the tire has hit a game object below. E.g. onTireContact(tireName, contactPosition, contactNormal, penetration).");
		this->onTireContactFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), onTireContactFunctionName->getString() + "(tireName, hitPhysicsComponent, contactPosition, contactNormal, penetration)");
	}

	PhysicsActiveVehicleComponent::~PhysicsActiveVehicleComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveVehicleComponent] Destructor physics active vehicle component for game object: " + this->gameObjectPtr->getName());
	}

	bool PhysicsActiveVehicleComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		PhysicsActiveComponent::parseCommonProperties(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OnSteerAngleChangedFunctionName")
		{
			this->setOnSteerAngleChangedFunctionName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OnMotorForceChangedFunctionName")
		{
			this->setOnMotorForceChangedFunctionName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OnHandBrakeChangedFunctionName")
		{
			this->setOnHandBrakeChangedFunctionName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OnBrakeChangedFunctionName")
		{
			this->setOnBrakeChangedFunctionName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OnTireContactFunctionName")
		{
			this->setOnTireContactFunctionName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		// this->constraintDirection->setValue(Ogre::Vector3::ZERO);

		return true;
	}

	GameObjectCompPtr PhysicsActiveVehicleComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		PhysicsActiveVehicleCompPtr clonedCompPtr(boost::make_shared<PhysicsActiveVehicleComponent>());

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
		// clonedCompPtr->setDefaultPoseName(this->defaultPoseName);
		clonedCompPtr->setCollisionType(this->collisionType->getListSelectedValue());
		// do not use constraintAxis variable, because its being manipulated during physics body creation
		// clonedCompPtr->setConstraintAxis(this->initConstraintAxis);

		clonedCompPtr->setCollisionDirection(this->collisionDirection->getVector3());
		// Bug in newton, setting afterwards collidable to true, will not work, hence do not clone this property
		// clonedCompPtr->setCollidable(this->collidable->getBool());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->setOnSteerAngleChangedFunctionName(this->onSteerAngleChangedFunctionName->getString());
		clonedCompPtr->setOnMotorForceChangedFunctionName(this->onMotorForceChangedFunctionName->getString());
		clonedCompPtr->setOnHandBrakeChangedFunctionName(this->onHandBrakeChangedFunctionName->getString());
		clonedCompPtr->setOnBrakeChangedFunctionName(this->onBrakeChangedFunctionName->getString());
		clonedCompPtr->setOnTireContactFunctionName(this->onTireContactFunctionName->getString());

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool PhysicsActiveVehicleComponent::postInit(void)
	{
		bool success = PhysicsComponent::postInit();

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveVehicleComponent] Init physics active vehicle component for game object: "
														+ this->gameObjectPtr->getName());

		this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
		this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
		this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();

		// Physics active component must be dynamic, else a mess occurs
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
		// Only model space x can be applied for cars, restrictions by newton dynamics engine.
		this->gameObjectPtr->getAttribute(GameObject::AttrDefaultDirection())->setValue(Ogre::Vector3::UNIT_X);


		// Priority connect! because other joints are refering to it!
		this->gameObjectPtr->setConnectPriority(true);

		// this->gameObjectPtr->getAttribute(PhysicsActiveComponent::AttrMass())->setVisible(false);

		return success;
	}

	void PhysicsActiveVehicleComponent::onRemoveComponent(void)
	{
		if (nullptr != this->physicsBody)
		{
			static_cast<OgreNewt::Vehicle*>(this->physicsBody)->removeAllTires();
		}
	}

	bool PhysicsActiveVehicleComponent::connect(void)
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

	bool PhysicsActiveVehicleComponent::disconnect(void)
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

	void PhysicsActiveVehicleComponent::update(Ogre::Real dt, bool notSimulating)
	{
		PhysicsActiveComponent::update(dt, notSimulating);

		if (false == notSimulating)
		{
			if (true == this->isVehicleTippedOver())
			{
				if (this->isVehicleStuck(dt))
				{
					this->correctVehicleOrientation();
				}
			}
		}
	}

	void PhysicsActiveVehicleComponent::actualizeValue(Variant* attribute)
	{
		PhysicsActiveComponent::actualizeCommonValue(attribute);

		if (PhysicsActiveVehicleComponent::AttrOnSteerAngleChangedFunctionName() == attribute->getName())
		{
			this->setOnSteerAngleChangedFunctionName(attribute->getString());
		}
		else if (PhysicsActiveVehicleComponent::AttrOnMotorForceChangedFunctionName() == attribute->getName())
		{
			this->setOnMotorForceChangedFunctionName(attribute->getString());
		}
		else if (PhysicsActiveVehicleComponent::AttrOnHandBrakeChangedFunctionName() == attribute->getName())
		{
			this->setOnHandBrakeChangedFunctionName(attribute->getString());
		}
		else if (PhysicsActiveVehicleComponent::AttrOnBrakeChangedFunctionName() == attribute->getName())
		{
			this->setOnBrakeChangedFunctionName(attribute->getString());
		}
		else if (PhysicsActiveVehicleComponent::AttrOnTireContactFunctionName() == attribute->getName())
		{
			this->setOnTireContactFunctionName(attribute->getString());
		}
	}

	void PhysicsActiveVehicleComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		PhysicsActiveComponent::writeCommonProperties(propertiesXML, doc);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool

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

	void PhysicsActiveVehicleComponent::setActivated(bool activated)
	{
		PhysicsActiveComponent::setActivated(activated);
	}

	void PhysicsActiveVehicleComponent::setOnSteerAngleChangedFunctionName(const Ogre::String& onSteerAngleChangedFunctionName)
	{
		this->onSteerAngleChangedFunctionName->setValue(onSteerAngleChangedFunctionName);
		this->onSteerAngleChangedFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), onSteerAngleChangedFunctionName + "(vehicleDrivingManipulation, dt)");
	}

	Ogre::String PhysicsActiveVehicleComponent::getOnSteerAngleChangedFunctionName(void) const
	{
		return this->onSteerAngleChangedFunctionName->getString();
	}

	void PhysicsActiveVehicleComponent::setOnMotorForceChangedFunctionName(const Ogre::String& onMotorForceChangedFunctionName)
	{
		this->onMotorForceChangedFunctionName->setValue(onMotorForceChangedFunctionName);
		this->onMotorForceChangedFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), onMotorForceChangedFunctionName + "(vehicleDrivingManipulation, dt)");
	}

	Ogre::String PhysicsActiveVehicleComponent::getOnMotorForceChangedFunctionName(void) const
	{
		return this->onMotorForceChangedFunctionName->getString();
	}

	void PhysicsActiveVehicleComponent::setOnHandBrakeChangedFunctionName(const Ogre::String& onHandBrakeChangedFunctionName)
	{
		this->onHandBrakeChangedFunctionName->setValue(onHandBrakeChangedFunctionName);
		this->onHandBrakeChangedFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), onHandBrakeChangedFunctionName + "(vehicleDrivingManipulation, dt)");
	}

	Ogre::String PhysicsActiveVehicleComponent::getOnHandBrakeChangedFunctionName(void) const
	{
		return this->onHandBrakeChangedFunctionName->getString();
	}

	void PhysicsActiveVehicleComponent::setOnBrakeChangedFunctionName(const Ogre::String& onBrakeChangedFunctionName)
	{
		this->onBrakeChangedFunctionName->setValue(onBrakeChangedFunctionName);
		this->onBrakeChangedFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), onBrakeChangedFunctionName + "(vehicleDrivingManipulation, dt)");
	}

	Ogre::String PhysicsActiveVehicleComponent::getOnBrakeChangedFunctionName(void) const
	{
		return this->onBrakeChangedFunctionName->getString();
	}

	void PhysicsActiveVehicleComponent::setOnTireContactFunctionName(const Ogre::String& onTireContactFunctionName)
	{
		this->onTireContactFunctionName->setValue(onTireContactFunctionName);
		this->onTireContactFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), onTireContactFunctionName + "(tireName, hitPhysicsComponent, contactPosition, contactNormal, penetration)");
	}

	Ogre::String PhysicsActiveVehicleComponent::getClassName(void) const
	{
		return "PhysicsActiveVehicleComponent";
	}

	Ogre::String PhysicsActiveVehicleComponent::getParentClassName(void) const
	{
		return "PhysicsActiveComponent";
	}

	Ogre::String PhysicsActiveVehicleComponent::getParentParentClassName(void) const
	{
		return "PhysicsComponent";
	}

	Ogre::String PhysicsActiveVehicleComponent::getOnTireContactFunctionName(void) const
	{
		return Ogre::String();
	}

	OgreNewt::Vehicle* PhysicsActiveVehicleComponent::getVehicle(void) const
	{
		return static_cast<OgreNewt::Vehicle*>(this->physicsBody);
	}

	Ogre::Vector3 PhysicsActiveVehicleComponent::getVehicleForce(void) const
	{
		return static_cast<OgreNewt::Vehicle*>(this->physicsBody)->getVehicleForce();
	}

	void PhysicsActiveVehicleComponent::setCanDrive(bool canDrive)
	{
		if (nullptr != this->physicsBody)
		{
			static_cast<OgreNewt::Vehicle*>(this->physicsBody)->setCanDrive(canDrive);
		}
	}

	void PhysicsActiveVehicleComponent::applyWheelie(Ogre::Real strength)
	{
		this->applyOmegaForce(this->getOrientation() * Ogre::Vector3(0.0f, 0.0f, strength));
	}

	void PhysicsActiveVehicleComponent::applyDrift(bool left, Ogre::Real strength, Ogre::Real steeringStrength)
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

	bool PhysicsActiveVehicleComponent::createDynamicBody(void)
	{
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

		this->physicsBody = new OgreNewt::Vehicle(this->ogreNewt, this->gameObjectPtr->getSceneManager(), this->gameObjectPtr->getDefaultDirection(), collisionPtr, weightedMass, 
												  this->collisionPosition->getVector3(), this->massOrigin->getVector3(), this->gravity->getVector3(), new PhysicsVehicleCallback(this->gameObjectPtr.get(), this->gameObjectPtr->getLuaScript(), this->ogreNewt,
												  this->onSteerAngleChangedFunctionName->getString(), this->onMotorForceChangedFunctionName->getString(), this->onHandBrakeChangedFunctionName->getString(), 
												  this->onBrakeChangedFunctionName->getString(), this->onTireContactFunctionName->getString()));

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

		this->physicsBody->setCustomForceAndTorqueCallback<PhysicsActiveComponent>(&PhysicsActiveComponent::moveCallback, this);

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

	bool PhysicsActiveVehicleComponent::isVehicleTippedOver(void)
	{
		if (nullptr == this->physicsBody)
		{
			return false;
		}
		// Get the car's current orientation
		Ogre::Quaternion currentOrientation = this->physicsBody->getOrientation();

		// Convert the orientation to Euler angles (roll, pitch, yaw)
		Ogre::Radian roll;
		Ogre::Radian pitch;
		Ogre::Radian yaw;
		Ogre::Matrix3 rotationMatrix;
		currentOrientation.ToRotationMatrix(rotationMatrix);
		rotationMatrix.ToEulerAnglesXYZ(yaw, pitch, roll);

		// Define the threshold angles for tipping over (in radians)
		Ogre::Radian maxAllowedPitch = Ogre::Degree(55.0f); // 55 degrees
		Ogre::Radian maxAllowedRoll = Ogre::Degree(55.0f);  // 55 degrees

		// Check if the car's pitch or roll exceeds the allowed thresholds
		// return std::abs(pitch.valueRadians()) > maxAllowedPitch.valueRadians() || std::abs(roll.valueRadians()) > maxAllowedRoll.valueRadians();
		return std::abs(pitch.valueRadians()) > maxAllowedPitch.valueRadians();
	}

	bool PhysicsActiveVehicleComponent::isVehicleStuck(Ogre::Real dt)
	{
		if (this->physicsBody->getVelocity().squaredLength() <= 0.1f * 0.1f)
		{
			this->stuckTime += dt;
			if (this->stuckTime >= this->maxStuckTime)
			{
				return true;
			}
		}
		else
		{
			this->stuckTime = 0.0f;
		}
		return false;
	}

	void PhysicsActiveVehicleComponent::correctVehicleOrientation(void)
	{
		// Get the car's current orientation
		Ogre::Quaternion currentOrientation = this->physicsBody->getOrientation();

		// Convert the orientation to a rotation matrix
		Ogre::Matrix3 rotationMatrix;
		currentOrientation.ToRotationMatrix(rotationMatrix);

		// Convert the rotation matrix to Euler angles (yaw, pitch, roll)
		Ogre::Radian roll;
		Ogre::Radian pitch;
		Ogre::Radian yaw;
		rotationMatrix.ToEulerAnglesYXZ(yaw, pitch, roll);  // YXZ to get pitch around X-axis

		// Calculate the opposite pitch to correct the orientation
		Ogre::Radian correctionPitch = -pitch;

		// Apply the correction pitch to the car's orientation
		this->physicsBody->setOmega(Ogre::Vector3(correctionPitch.valueRadians(), 0.0f, 0.0f));
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	VehicleDrivingManipulation::VehicleDrivingManipulation()
		: steerAngle(0.0f),
		motorForce(0.0f),
		handBrake(0.0f),
		brake(0.0f)
	{

	}

	VehicleDrivingManipulation::~VehicleDrivingManipulation()
	{

	}

	void VehicleDrivingManipulation::setSteerAngle(Ogre::Real steerAngle)
	{
		this->steerAngle = steerAngle;
	}

	Ogre::Real VehicleDrivingManipulation::getSteerAngle(void) const
	{
		return this->steerAngle;
	}

	void VehicleDrivingManipulation::setMotorForce(Ogre::Real motorForce)
	{
		this->motorForce = motorForce;
	}

	Ogre::Real VehicleDrivingManipulation::getMotorForce(void) const
	{
		return this->motorForce;
	}

	void VehicleDrivingManipulation::setHandBrake(Ogre::Real handBrake)
	{
		this->handBrake = handBrake;
	}

	Ogre::Real VehicleDrivingManipulation::getHandBrake(void) const
	{
		return this->handBrake;
	}

	void VehicleDrivingManipulation::setBrake(Ogre::Real brake)
	{
		this->brake = brake;
	}

	Ogre::Real VehicleDrivingManipulation::getBrake(void) const
	{
		return this->brake;
	}
}; // namespace end
#include "NOWAPrecompiled.h"
#include "PhysicsActiveComponent.h"
#include "PhysicsCompoundConnectionComponent.h"
#include "JointComponents.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "main/Events.h"
#include "Math/Simple/OgreAabb.h"
#include "LuaScriptComponent.h"
#include "main/AppStateManager.h"

#include <mutex>

namespace
{
	std::mutex mutex;
}

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	PhysicsActiveComponent::PhysicsActiveComponent()
		: PhysicsComponent(),
		activated(new Variant(PhysicsActiveComponent::AttrActivated(), true, this->attributes)),
		collisionSize(new Variant(PhysicsActiveComponent::AttrCollisionSize(), Ogre::Vector3::ZERO, this->attributes)),
		collisionPosition(new Variant(PhysicsActiveComponent::AttrCollisionPosition(), Ogre::Vector3::ZERO, this->attributes)),
		collisionDirection(new Variant(PhysicsActiveComponent::AttrCollisionDirection(), Ogre::Vector3::ZERO, this->attributes)),
		massOrigin(new Variant(PhysicsActiveComponent::AttrMassOrigin(), Ogre::Vector3::ZERO, this->attributes)),
		gravity(new Variant(PhysicsActiveComponent::AttrGravity(), AppStateManager::getSingletonPtr()->getOgreNewtModule()->getGlobalGravity(), this->attributes)),
		gravitySourceCategory(new Variant(PhysicsActiveComponent::AttrGravitySourceCategory(), Ogre::String(), this->attributes)),
		linearDamping(new Variant(PhysicsActiveComponent::AttrLinearDamping(), Ogre::Real(0.1f), this->attributes)),
		angularDamping(new Variant(PhysicsActiveComponent::AttrAngularDamping(), Ogre::Vector3(0.1f, 0.1f, 0.1f), this->attributes)),
		force(new Variant(PhysicsActiveComponent::AttrForce(), Ogre::Vector3::ZERO, this->attributes)),
		continuousCollision(new Variant(PhysicsActiveComponent::AttrContinuousCollision(), false, this->attributes)),
		maxSpeed(new Variant(PhysicsActiveComponent::AttrMaxSpeed(), Ogre::Real(5.0f), this->attributes)),
		speed(new Variant(PhysicsActiveComponent::AttrSpeed(), Ogre::Real(1.0f), this->attributes)),
		minSpeed(new Variant(PhysicsActiveComponent::AttrMinSpeed(), Ogre::Real(1.0f), this->attributes)),
		constraintDirection(new Variant(PhysicsActiveComponent::AttrConstraintDirection(), Ogre::Vector3::ZERO, this->attributes)),
		constraintAxis(new Variant(PhysicsActiveComponent::AttrConstraintAxis(), Ogre::Vector3::ZERO, this->attributes)),
		asSoftBody(new Variant(PhysicsActiveComponent::AttrAsSoftBody(), false, this->attributes)),
		gyroscopicTorque(new Variant(PhysicsActiveComponent::AttrEnableGyroscopicTorque(), false, this->attributes)),
		height(0.0f),
		rise(0.0f),
		upVector(nullptr),
		planeConstraint(nullptr),
		hasAttraction(false),
		hasSpring(false),
		omegaForce(Ogre::Vector3::ZERO),
		canAddOmegaForce(false),
		forceForVelocity(Ogre::Vector3::ZERO),
		canAddForceForVelocity(false),
		bResetForce(false),
		clampedOmega(0.0f),
		lastTime(0.0),
		dt(0.0f),
		isInSimulation(false),
		usesBounds(false),
		minBounds(Ogre::Vector3::ZERO),
		maxBounds(Ogre::Vector3::ZERO)
	{
		this->collisionType->setValue({ "ConvexHull", "ConcaveHull", "Box", "Capsule", "ChamferCylinder", "Cone", "Cylinder", "Ellipsoid", "Pyramid" });

		/*
			Apply the linear viscous damping coefficient to the body. the default value of linearDamp is clamped to a value
			between 0.0 and 1.0; the default value is 0.1, There is a non zero implicit attenuation value of 0.0001 assume by
			the integrator.
			The dampening viscous friction force is added to the external force applied to the body every frame before
			going to the solver-integrator. This force is proportional to the square of the magnitude of the velocity to
			the body in the opposite direction of the velocity of the body. An application can set linearDamp to zero
			when the application takes control of the external forces and torque applied to the body, should the application
			desire to have absolute control of the forces over that body. However, it is recommended that the linearDamp
			coefficient is set to a non-zero value for the majority of background bodies. This saves the application from
			having to control these forces and also prevents the integrator from adding very large velocities to a body
		*/
		this->linearDamping->setDescription("Range [0, 1]");
		this->linearDamping->setConstraints(0.0f, 1.0f);

		/*
		Apply the angular viscous damping coefficient to the body. the default value of angularDamp is clamped to
		a value between 0.0 and 1.0; the default value is 0.1, There is a non zero implicit attenuation value of 0.0001
		assumed by the integrator.
		*/
		this->angularDamping->setDescription("Range [0, 1]");
		// Constraints, 3 sliders?? How to do that?

		this->gravitySourceCategory->setDescription("The gravity source category, to which this game object should be attracted. Planetary systems builds are possible.");
		this->collisionPosition->setDescription("Sets an offset to the collision position of the collision hull. This may be necessary when e.g. capsule is used and the origin point of the mesh is not in the middle of geometry.");
		this->collisionDirection->setDescription("Sets an offset collision direction for the collision hull. The collision hull will be rotated according to the given direction.");
		this->gyroscopicTorque->setDescription("Enables gyroscopic precision for torque etc. See: https://www.youtube.com/watch?v=BCVQFoPO5qQ, https://www.youtube.com/watch?v=UlErvZoU7Q0");

		this->constraintDirection->setDescription("Sets the axis around which the game object only can be rotated.");
		this->constraintAxis->setDescription("Sets the normal axis at which the game object only can be moved. E.g. setting (0, 0, 1), the game object can be moved at x-y axis.");
	}

	PhysicsActiveComponent::~PhysicsActiveComponent()
	{
		/*if (nullptr != this->physicsBody)
			this->physicsBody->removeNodeUpdateNotify();*/

		this->releaseConstraintDirection();
		this->releaseConstraintAxis();
		
		this->destroyLineMap();

		this->physicsAttractors.clear();
		this->springs.clear();
		this->detachAndDestroyAllForceObserver();

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveComponent] Destructor physics active component for game object: " + this->gameObjectPtr->getName());
	}

	bool PhysicsActiveComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		this->parseCommonProperties(propertyElement, filename);

		return true;
	}

	void PhysicsActiveComponent::parseCommonProperties(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CollisionSize")
		{
			this->collisionSize->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OffsetPosition")
		{
			this->collisionPosition->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OffsetDirection")
		{
			this->collisionDirection->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Mass")
		{
			this->mass->setValue(XMLConverter::getAttribReal(propertyElement, "data", 10.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MassOrigin")
		{
			this->massOrigin->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "LinearDamping")
		{
			this->linearDamping->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.001f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AngularDamping")
		{
			this->angularDamping->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(0.01f, 0.01f, 0.01f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Gravity")
		{
			this->gravity->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(0.0f, -16.8f, 0.0f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "GravitySourceCategory")
		{
			this->gravitySourceCategory->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ConstraintDirection")
		{
			this->constraintDirection->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ContinuousCollision")
		{
			this->continuousCollision->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Speed")
		{
			this->speed->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MinSpeed")
		{
			this->minSpeed->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxSpeed")
		{
			this->maxSpeed->setValue(XMLConverter::getAttribReal(propertyElement, "data", 10.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		/*
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DefaultPoseName")
		{
		this->defaultPoseName = XMLConverter::getAttrib(propertyElement, "data", "");
		propertyElement = propertyElement->next_sibling("property");
		}*/
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CollisionType")
		{
			this->collisionType->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", ""));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ConstraintAxis")
		{
			this->constraintAxis->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3::ZERO));
			propertyElement = propertyElement->next_sibling("property");
		}

		/*if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AsSoftBody")
		{
			this->asSoftBody->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}*/

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "EnableGyroscopicTorque")
		{
			this->gyroscopicTorque->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Collidable")
		{
			this->collidable->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
	}

	GameObjectCompPtr PhysicsActiveComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		PhysicsActiveCompPtr clonedCompPtr(boost::make_shared<PhysicsActiveComponent>());

				// main properties
		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setMass(this->mass->getReal());
		clonedCompPtr->setMassOrigin(this->massOrigin->getVector3());
		clonedCompPtr->setLinearDamping(this->linearDamping->getReal());
		clonedCompPtr->setAngularDamping(this->angularDamping->getVector3());
		clonedCompPtr->setGravity(this->gravity->getVector3());
		clonedCompPtr->setGravitySourceCategory(this->gravitySourceCategory->getString());
		// clonedCompPtr->applyAngularImpulse(this->angularImpulse);
		// clonedCompPtr->applyForce(this->force->getVector3());
		clonedCompPtr->setConstraintDirection(this->constraintDirection->getVector3());
		clonedCompPtr->setContinuousCollision(this->continuousCollision->getBool());
		clonedCompPtr->setSpeed(this->speed->getReal());
		clonedCompPtr->setMinSpeed(this->minSpeed->getReal());
		clonedCompPtr->setMaxSpeed(this->maxSpeed->getReal());
		// clonedCompPtr->setDefaultPoseName(this->defaultPoseName);
		clonedCompPtr->setCollisionType(this->collisionType->getListSelectedValue());
		// do not use constraintAxis variable, because its being manipulated during physics body creation
		clonedCompPtr->setConstraintAxis(this->constraintAxis->getVector3());
		// Bug in newton, setting afterwards collidable to true, will not work, hence do not clone this property?
		// clonedCompPtr->setCollidable(this->collidable->getBool());

		clonedCompPtr->setCollisionSize(this->collisionSize->getVector3());
		clonedCompPtr->setCollisionPosition(this->collisionPosition->getVector3());
		clonedCompPtr->setCollisionDirection(this->collisionDirection->getVector3());
		clonedCompPtr->setAsSoftBody(this->asSoftBody->getBool());
		clonedCompPtr->setGyroscopicTorqueEnabled(this->gyroscopicTorque->getBool());
		

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool PhysicsActiveComponent::postInit(void)
	{
		bool success = PhysicsComponent::postInit();
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveComponent] Init physics active component for game object: " + this->gameObjectPtr->getName());

		this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
		this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
		this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();

		// Physics active component must be dynamic, else a mess occurs
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		auto& physicsCompoundConnectionCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsCompoundConnectionComponent>());
		if (nullptr == physicsCompoundConnectionCompPtr)
		{
			if (!this->createDynamicBody())
			{
				return false;
			}
		}

		return success;
	}

	bool PhysicsActiveComponent::connect(void)
	{
		PhysicsComponent::connect();

		this->lastTime = static_cast<double>(this->timer.getMilliseconds()) * 0.001;

		return true;
	}

	bool PhysicsActiveComponent::disconnect(void)
	{
		this->resetForce();
		this->destroyLineMap();

		return true;
	}

	void PhysicsActiveComponent::update(Ogre::Real dt, bool notSimulating)
	{
		this->isInSimulation = !notSimulating;

		if (true == this->usesBounds)
		{
			Ogre::Vector3 position = this->physicsBody->getPosition();
			Ogre::Vector3 newPosition = this->physicsBody->getPosition();

			if (position.x < this->minBounds.x)
			{
				newPosition.x = this->minBounds.x + dt;
			}
			else if (position.x > this->maxBounds.x)
			{
				newPosition.x = this->maxBounds.x - dt;
			}

			if (position.y < this->minBounds.y)
			{
				newPosition.y = this->minBounds.y + dt;
			}
			else if (position.y > this->maxBounds.y)
			{
				newPosition.y = this->maxBounds.y - dt;
			}

			if (position.z < this->minBounds.z)
			{
				newPosition.z = this->minBounds.z + dt;
			}
			else if (position.z > this->maxBounds.z)
			{
				newPosition.z = this->maxBounds.z - dt;
			}

			this->physicsBody->setPositionOrientation(newPosition, this->physicsBody->getOrientation());
		}
	}

	Ogre::String PhysicsActiveComponent::getClassName(void) const
	{
		return "PhysicsActiveComponent";
	}

	Ogre::String PhysicsActiveComponent::getParentClassName(void) const
	{
		return "PhysicsComponent";
	}

	bool PhysicsActiveComponent::canStaticAddComponent(GameObject* gameObject)
	{
		return true;
	}

	bool PhysicsActiveComponent::createDynamicBody(void)
	{
		// If a default pose name is set, first set the animation, this is used, because e.g. the T-pose may extend the convex hull to much
		//if (!this->defaultPoseName.empty())
		//{
		//	Ogre::v1::AnimationState* animationState = this->gameObjectPtr->getEntity()->getAnimationState(this->defaultPoseName);
		//	if (nullptr != animationState)
		//	{
		//		//animationState->setEnabled(true);
		//		//// animationState->setTimePosition(0.1f);
		//		//animationState->addTime(0.5f);
		//		//// Ogre::v1::Animation* defaultAnimation = this->gameObjectPtr->getEntity()->getSkeleton()->getAnimation(this->defaultPoseName);
		//		//// defaultAnimation->apply(this->gameObjectPtr->getEntity()->getSkeleton(), 0.5f);
		//		//this->gameObjectPtr->getEntity()->addSoftwareAnimationRequest(false);
		//		//this->gameObjectPtr->getEntity()->getAllAnimationStates()->_notifyDirty();
		//		//this->gameObjectPtr->getEntity()->_updateAnimation();
		//		// this->gameObjectPtr->getEntity()->getMesh()->createAnimation(this->defaultPoseName, animationState->getLength());
		//		// this->gameObjectPtr->getEntity()->getMesh()->getAnimation(this->defaultPoseName)->apply(0.1f);
		//		// this->gameObjectPtr->getEntity()->getMesh()->getAnimation(this->defaultPoseName)->_keyFrameListChanged();
		//	}
		//	else
		//	{
		//		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveComponent]: There is no such default pose name: " + this->defaultPoseName
		//			+ " for game object: " + this->gameObjectPtr->getName());
		//	}
		//}
		
		Ogre::Vector3 inertia = Ogre::Vector3(1.0f, 1.0f, 1.0f);

		Ogre::Quaternion collisionOrientation = Ogre::Quaternion::IDENTITY; // this->gameObjectPtr->getSceneNode()->getOrientation();
		if (Ogre::Vector3::ZERO != this->collisionDirection->getVector3())
		{
			collisionOrientation = MathHelper::getInstance()->degreesToQuat(this->collisionDirection->getVector3());
		}

		Ogre::Vector3 calculatedMassOrigin = Ogre::Vector3::ZERO;

		this->physicsBody = new OgreNewt::Body(this->ogreNewt, this->gameObjectPtr->getSceneManager(), this->createDynamicCollision(inertia, this->collisionSize->getVector3(), this->collisionPosition->getVector3(), 
			collisionOrientation, calculatedMassOrigin, this->gameObjectPtr->getCategoryId()));

		if (Ogre::Vector3::ZERO != this->massOrigin->getVector3())
		{
			calculatedMassOrigin = this->massOrigin->getVector3();
		}
		
		this->physicsBody->setGravity(this->gravity->getVector3());
		
		Ogre::Real weightedMass = this->mass->getReal(); /** scale.x * scale.y * scale.z;*/ // scale is not used anymore, because if big game objects are scaled down, the mass is to low!

		// set mass origin
		this->physicsBody->setCenterOfMass(calculatedMassOrigin);

		if (this->collisionType->getListSelectedValue() == "ConvexHull")
		{
			this->physicsBody->setConvexIntertialMatrix(inertia, calculatedMassOrigin);
		}

		// Apply mass and scale to inertia (the bigger the object, the more mass)
		inertia *= weightedMass;
		this->physicsBody->setMassMatrix(weightedMass, inertia);

		if (this->linearDamping->getReal() != 0.0f)
		{
			this->physicsBody->setLinearDamping(this->linearDamping->getReal());
		}
		if (this->angularDamping->getVector3() != Ogre::Vector3::ZERO)
		{
			this->physicsBody->setAngularDamping(this->angularDamping->getVector3());
		}

		this->physicsBody->setCustomForceAndTorqueCallback<PhysicsActiveComponent>(&PhysicsActiveComponent::moveCallback, this);

		// For fixed Update: Does not work, called to often
		// this->physicsBody->setNodeUpdateNotify<PhysicsActiveComponent>(&PhysicsActiveComponent::updateCallback, this);

		this->setActivated(this->activated->getBool());
		this->setContinuousCollision(this->continuousCollision->getBool());
		this->setGyroscopicTorqueEnabled(this->gyroscopicTorque->getBool());

		// set user data for ogrenewt
		this->physicsBody->setUserData(OgreNewt::Any(dynamic_cast<PhysicsComponent*>(this)));
		this->physicsBody->attachNode(this->gameObjectPtr->getSceneNode());

		this->physicsBody->setPositionOrientation(this->initialPosition, this->initialOrientation);
		// Must be set after body set its position! And also the current orientation (initialOrientation) must be correct! Else weird rotation behavior occurs
		this->setConstraintAxis(this->constraintAxis->getVector3());
		// Pin the object stand in pose and not fall down
		this->setConstraintDirection(this->constraintDirection->getVector3());

		this->setCollidable(this->collidable->getBool());

		this->physicsBody->setType(this->gameObjectPtr->getCategoryId());
		this->physicsBody->setMaterialGroupID(
			AppStateManager::getSingletonPtr()->getGameObjectController()->getMaterialID(this->gameObjectPtr.get(), this->ogreNewt));

		return true;
	}

	void PhysicsActiveComponent::createSoftBody(void)
	{
		// Collision must be recreated
		Ogre::Vector3 inertia = Ogre::Vector3(1.0f, 1.0f, 1.0f);

		Ogre::Quaternion collisionOrientation = Ogre::Quaternion::IDENTITY; // this->gameObjectPtr->getSceneNode()->getOrientation();
		if (Ogre::Vector3::ZERO != this->collisionDirection->getVector3())
		{
			collisionOrientation = MathHelper::getInstance()->degreesToQuat(this->collisionDirection->getVector3());
		}

		Ogre::Vector3 calculatedMassOrigin = Ogre::Vector3::ZERO;

		// Create collision for specific collision type
		OgreNewt::CollisionPtr collisionPtr = this->createDynamicCollision(inertia, this->collisionSize->getVector3(), this->collisionPosition->getVector3(), 
			collisionOrientation, calculatedMassOrigin, this->gameObjectPtr->getCategoryId());
		// Create from that collision a deformable collision
		OgreNewt::CollisionPtr deformableCollisionPtr = this->createDeformableCollision(collisionPtr);

		const OgreNewt::MaterialID* materialId = this->physicsBody->getMaterialGroupID();
		
		// this->physicsBody->setCollision(deformableCollisionPtr);
		if (nullptr != this->physicsBody)
		{
			this->physicsBody->detachNode();
			this->physicsBody->removeForceAndTorqueCallback();
			this->physicsBody->removeNodeUpdateNotify();
			this->physicsBody->removeDestructorCallback();
			delete this->physicsBody;
			this->physicsBody = nullptr;
		}

		this->physicsBody = new OgreNewt::Body(this->ogreNewt, this->gameObjectPtr->getSceneManager(), deformableCollisionPtr);
		this->physicsBody->setGravity(this->gravity->getVector3());

		if (Ogre::Vector3::ZERO != this->massOrigin->getVector3())
		{
			calculatedMassOrigin = this->massOrigin->getVector3();
		}

		Ogre::Real weightedMass = this->mass->getReal(); /** scale.x * scale.y * scale.z;*/ // scale is not used anymore, because if big game objects are scaled down, the mass is to low!

		// apply mass and scale to inertia (the bigger the object, the more mass)
		inertia *= weightedMass;
		this->physicsBody->setMassMatrix(weightedMass, inertia);

		// set mass origin
		this->physicsBody->setCenterOfMass(calculatedMassOrigin);

		this->physicsBody->setLinearDamping(this->linearDamping->getReal());
		this->physicsBody->setAngularDamping(this->angularDamping->getVector3());

		this->physicsBody->setCustomForceAndTorqueCallback<PhysicsActiveComponent>(&PhysicsActiveComponent::moveCallback, this);

		this->setActivated(this->activated->getBool());

		// set user data for ogrenewt
		this->physicsBody->setUserData(OgreNewt::Any(dynamic_cast<PhysicsComponent*>(this)));

		this->physicsBody->attachNode(this->gameObjectPtr->getSceneNode());

		this->physicsBody->setPositionOrientation(this->initialPosition, this->initialOrientation);

		this->setConstraintAxis(this->constraintAxis->getVector3());
		// Pin the object stand in pose and not fall down
		this->setConstraintDirection(this->constraintDirection->getVector3());

		this->physicsBody->setType(this->gameObjectPtr->getCategoryId());
		this->physicsBody->setMaterialGroupID(materialId);
	}

	void PhysicsActiveComponent::reCreateCollision(void)
	{
		if (nullptr == this->physicsBody)
			return;

		// Collision must be recreated
		Ogre::Vector3 inertia = Ogre::Vector3(1.0f, 1.0f, 1.0f);

		Ogre::Quaternion collisionOrientation = Ogre::Quaternion::IDENTITY; // this->gameObjectPtr->getSceneNode()->getOrientation();
		if (Ogre::Vector3::ZERO != this->collisionDirection->getVector3())
		{
			collisionOrientation = MathHelper::getInstance()->degreesToQuat(this->collisionDirection->getVector3());
		}

		Ogre::Vector3 calculatedMassOrigin = Ogre::Vector3::ZERO;
		// Set the new collision hull
		this->physicsBody->setCollision(this->createDynamicCollision(inertia, this->collisionSize->getVector3(), this->collisionPosition->getVector3(), 
			collisionOrientation, calculatedMassOrigin, this->gameObjectPtr->getCategoryId()));

		if (Ogre::Vector3::ZERO != this->massOrigin->getVector3())
		{
			calculatedMassOrigin = this->massOrigin->getVector3();
		}
		Ogre::Real weightedMass = this->mass->getReal();
		inertia *= weightedMass;
		this->physicsBody->setMassMatrix(weightedMass, inertia);
		this->physicsBody->setCenterOfMass(calculatedMassOrigin);
	}

	void PhysicsActiveComponent::createCompoundBody(const std::vector<PhysicsActiveComponent*>& physicsComponentList)
	{
		Ogre::Vector3 inertia = Ogre::Vector3(1.0f, 1.0f, 1.0f);
		Ogre::Vector3 collisionPosition = Ogre::Vector3::ZERO;
		Ogre::Vector3 cumMassOrigin = Ogre::Vector3::ZERO;
		Ogre::Real cumMass = 0.0f;
		std::vector<OgreNewt::CollisionPtr> compoundCollisionList;

		//First create for this root compound the collision data
		
		Ogre::Quaternion collisionOrientation = Ogre::Quaternion::IDENTITY;
		if (Ogre::Vector3::ZERO != this->collisionDirection->getVector3())
		{
			collisionOrientation = MathHelper::getInstance()->degreesToQuat(this->collisionDirection->getVector3());
		}
		// Add collision to list
		compoundCollisionList.emplace_back(this->createDynamicCollision(inertia, this->collisionSize->getVector3(), this->collisionPosition->getVector3(), 
			collisionOrientation, cumMassOrigin, this->gameObjectPtr->getCategoryId()));

		PhysicsActiveComponent* prevPhysicsComponent = nullptr;
		std::vector<PhysicsActiveComponent*>::const_iterator it;

		for (it = physicsComponentList.cbegin(); it != physicsComponentList.cend(); ++it)
		{

			Ogre::SceneNode* pItNode = (*it)->getOwner()->getSceneNode();
			Ogre::Vector3 resultPosition = this->gameObjectPtr->getSceneNode()->convertWorldToLocalPosition(pItNode->getPosition());
			Ogre::Quaternion resultOrientation = this->gameObjectPtr->getSceneNode()->convertWorldToLocalOrientation(pItNode->getOrientation());

			//// Check if its the root compound (has not root id, because its the root)
			//auto& physicsCompoundConnectionCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsCompoundConnectionComponent>());
			//if (nullptr != physicsCompoundConnectionCompPtr && 0 == physicsCompoundConnectionCompPtr->getRootId())
			{

				// since a scenenode can not be attachted to two nodes, detach it from the "WorldNode" and add it under the root compound node
				this->gameObjectPtr->getSceneNode()->getParentSceneNode()->removeChild(pItNode);
				this->gameObjectPtr->getSceneNode()->addChild(pItNode);
				pItNode->setPosition(resultPosition);
				pItNode->setOrientation(resultOrientation);
				pItNode->setInheritScale(false);
			}

			// if (prevPhysicsComponent)
			{
				collisionPosition = resultPosition;
				collisionOrientation = resultOrientation;
			}
// Attention: Is this correct
			cumMassOrigin += ((*it)->getOwner()->getMovableObject()->getLocalAabb().getSize() * (*it)->getOwner()->getSceneNode()->getScale() * (*it)->getMassOrigin());
			cumMass += (*it)->getMass();
			// Attention!
			Ogre::Vector3 massOrigin = (*it)->getMassOrigin();
			compoundCollisionList.emplace_back((*it)->createDynamicCollision(inertia, (*it)->getCollisionSize(), 
				collisionPosition, collisionOrientation, massOrigin, this->gameObjectPtr->getCategoryId()));

			prevPhysicsComponent = *it;
		}

		// cumMassOrigin /= physicsComponentList.size();

		OgreNewt::CollisionPtr compoundCollision = OgreNewt::CollisionPtr(
			new OgreNewt::CollisionPrimitives::CompoundCollision(this->ogreNewt, compoundCollisionList, this->gameObjectPtr->getCategoryId()));
		// what about volume? Since compound collision has no volume calculation

		/*if (nullptr != this->physicsBody)
		{
			this->physicsBody->detachNode();
			this->physicsBody->removeForceAndTorqueCallback();
			this->physicsBody->removeNodeUpdateNotify();
			this->physicsBody->removeDestructorCallback();
			delete this->physicsBody;
			this->physicsBody = nullptr;
		}*/

		if (nullptr == this->physicsBody)
			this->physicsBody = new OgreNewt::Body(this->ogreNewt, this->gameObjectPtr->getSceneManager(), compoundCollision);
		else
			this->physicsBody->setCollision(compoundCollision);

		this->physicsBody->setGravity(this->gravity->getVector3());

		/*// just take the scale factor into account if it is not to small
		if (scale.x * scale.y * scale.z > 0.1f) {
		newMass = scale.x * scale.y * scale.z * this->mass; //Masse ist standartmaessig 10, ausser wenn der Benutzer im Editor eine Masse uebergeben hat
		}*/
		inertia *= cumMass;

		this->physicsBody->setMassMatrix(cumMass, inertia);

		this->physicsBody->setCenterOfMass(cumMassOrigin);
	
		this->physicsBody->setLinearDamping(this->linearDamping->getReal());
		this->physicsBody->setAngularDamping(this->angularDamping->getVector3());

		this->setConstraintAxis(this->getConstraintAxis());
		
		// pin the object stand in pose and not fall down
		this->setConstraintDirection(this->getConstraintDirection());

		this->setActivated(this->activated->getBool());

		this->setContinuousCollision(this->continuousCollision->getBool());
		this->setGyroscopicTorqueEnabled(this->gyroscopicTorque->getBool());

		this->physicsBody->setType(this->gameObjectPtr->getCategoryId());

		// Set user data for ogrenewt
		this->physicsBody->setUserData(OgreNewt::Any(dynamic_cast<PhysicsComponent*>(this)));

		this->physicsBody->setCustomForceAndTorqueCallback<PhysicsActiveComponent>(&PhysicsActiveComponent::moveCallback, this);

		this->physicsBody->attachNode(this->gameObjectPtr->getSceneNode());

		this->physicsBody->setPositionOrientation(this->initialPosition, this->initialOrientation);


		this->physicsBody->setType(gameObjectPtr->getCategoryId());
		this->physicsBody->setMaterialGroupID(
			AppStateManager::getSingletonPtr()->getGameObjectController()->getMaterialID(this->gameObjectPtr.get(), this->ogreNewt));

		// If this root has a joint, set this new body
		unsigned int i = 0;
		boost::shared_ptr<JointComponent> jointCompPtr = nullptr;
		do
		{
			jointCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponentWithOccurrence<JointComponent>(i));
			if (nullptr != jointCompPtr)
			{
				jointCompPtr->setBody(this->physicsBody);
				i++;
			}
		} while (nullptr != jointCompPtr);
	}

	void PhysicsActiveComponent::destroyCompoundBody(const std::vector<PhysicsActiveComponent*>& physicsComponentList)
	{
		Ogre::Vector3 collisionPosition = Ogre::Vector3::ZERO;
		Ogre::Quaternion collisionOrientation = Ogre::Quaternion::IDENTITY;
		
		std::vector<PhysicsActiveComponent*>::const_iterator it;
		PhysicsActiveComponent* prevPhysicsComponent = nullptr;
		for (it = physicsComponentList.cbegin(); it != physicsComponentList.cend(); ++it)
		{
			Ogre::SceneNode* pItNode = (*it)->getOwner()->getSceneNode();
			Ogre::Vector3 resultPosition = this->gameObjectPtr->getSceneNode()->convertLocalToWorldPosition(pItNode->getPosition());
			Ogre::Quaternion resultOrientation = this->gameObjectPtr->getSceneNode()->convertLocalToWorldOrientation(pItNode->getOrientation());

			//// Check if its the root compound (has not root id, because its the root)
			//auto& physicsCompoundConnectionCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsCompoundConnectionComponent>());
			//if (nullptr != physicsCompoundConnectionCompPtr && 0 == physicsCompoundConnectionCompPtr->getRootId())
			{
				this->gameObjectPtr->getSceneNode()->removeChild(pItNode);
				this->gameObjectPtr->getSceneNode()->getParentSceneNode()->addChild(pItNode);

				pItNode->setPosition(resultPosition);
				pItNode->setOrientation(resultOrientation);
				pItNode->setInheritScale(false);
			}

			if (prevPhysicsComponent)
			{
				collisionPosition = resultPosition;
				collisionOrientation = resultOrientation;
			}
			(*it)->destroyCollision();

			prevPhysicsComponent = *it;
		}

		this->destroyBody();
	}

	void PhysicsActiveComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);

		if (false == activated)
		{
			if (nullptr != this->physicsBody)
			{
				this->physicsBody->freeze();
				// this->physicsBody->setAutoSleep(1);
			}
		}
		else
		{
			if (nullptr != this->physicsBody)
			{
				physicsBody->unFreeze();
				// this->physicsBody->setAutoSleep(0);
				//physicsBody->setVelocity(Ogre::Vector3(0.0f, 0.1f, 0.0f));
			}
		}
	}

	bool PhysicsActiveComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void PhysicsActiveComponent::setLinearDamping(Ogre::Real linearDamping)
	{
		this->linearDamping->setValue(linearDamping);
	}

	Ogre::Real PhysicsActiveComponent::getLinearDamping(void) const
	{
		return this->linearDamping->getReal();
	}

	void PhysicsActiveComponent::setAngularDamping(const Ogre::Vector3& angularDamping)
	{
		this->angularDamping->setValue(angularDamping);
	}

	const Ogre::Vector3 PhysicsActiveComponent::getAngularDamping(void) const
	{
		return this->angularDamping->getVector3();
	}

	void PhysicsActiveComponent::setGyroscopicTorqueEnabled(bool gyroscopicTorque)
	{
		this->gyroscopicTorque->setValue(gyroscopicTorque);
		if (nullptr != this->physicsBody)
		{
			this->physicsBody->enableGyroscopicTorque(gyroscopicTorque);
		}
	}

	bool PhysicsActiveComponent::getGyroscopicTorqueEnabled(void) const
	{
		return this->gyroscopicTorque->getBool();
	}
	
	void PhysicsActiveComponent::setOmegaVelocity(const Ogre::Vector3& omegaVelocity)
	{
		this->physicsBody->setOmega(omegaVelocity);
	}
		
	Ogre::Vector3 PhysicsActiveComponent::getOmegaVelocity(void) const
	{
		return this->physicsBody->getOmega();
	}

	void PhysicsActiveComponent::applyOmegaForce(const Ogre::Vector3& omegaForce)
	{
		this->omegaForce = omegaForce;
		this->canAddOmegaForce = true;
	}

	void PhysicsActiveComponent::applyOmegaForceRotateTo(const Ogre::Quaternion& resultOrientation, const Ogre::Vector3& axes, Ogre::Real strength)
	{
		if (nullptr == this->physicsBody)
			return;

		Ogre::Quaternion diffOrientation = this->physicsBody->getOrientation().Inverse() * resultOrientation;
		Ogre::Vector3 resultVector = Ogre::Vector3::ZERO;

		if (axes.x == 1.0f)
		{
			resultVector.x = diffOrientation.getPitch().valueRadians() * strength;
		}
		if (axes.y == 1.0f)
		{
			resultVector.y = diffOrientation.getYaw().valueRadians() * strength;
		}
		if (axes.z == 1.0f)
		{
			resultVector.z = diffOrientation.getRoll().valueRadians() * strength;
		}
	
		this->applyOmegaForce(resultVector);
	}

	void PhysicsActiveComponent::addImpulse(const Ogre::Vector3& deltaVector)
	{
		this->physicsBody->addImpulse(deltaVector, this->physicsBody->getPosition(), 1.0f / this->ogreNewt->getDesiredFps());
	}

	void PhysicsActiveComponent::setVelocity(const Ogre::Vector3& velocity)
	{
		this->physicsBody->setVelocity(velocity);
	}

	void PhysicsActiveComponent::setDirectionVelocity(Ogre::Real speed)
	{
		Ogre::Vector3 currentDirection = this->gameObjectPtr->getOrientation() * this->gameObjectPtr->getDefaultDirection();
		this->setVelocity(currentDirection * speed);
	}

	Ogre::Vector3 PhysicsActiveComponent::getVelocity(void) const
	{
		if (nullptr != this->physicsBody)
			return this->physicsBody->getVelocity();
		else
			return Ogre::Vector3::ZERO;
	}

	void PhysicsActiveComponent::resetForce(void)
	{
		this->bResetForce = true;
		if (nullptr != this->physicsBody)
		{
			this->physicsBody->setVelocity(Ogre::Vector3::ZERO);
			this->physicsBody->setOmega(Ogre::Vector3::ZERO);
		}
	}

	void PhysicsActiveComponent::applyForce(const Ogre::Vector3& force)
	{
		this->force->setValue(force);
	}

	void PhysicsActiveComponent::applyRequiredForceForVelocity(const Ogre::Vector3& velocity)
	{
		this->forceForVelocity = velocity;
		if (Ogre::Vector3::ZERO != this->forceForVelocity)
		{
			this->canAddForceForVelocity = true;
		}
	}

	void PhysicsActiveComponent::applyDirectionForce(Ogre::Real speed)
	{
		Ogre::Vector3 currentDirection = this->gameObjectPtr->getOrientation() * this->gameObjectPtr->getDefaultDirection();
		this->applyRequiredForceForVelocity(currentDirection * speed);
	}

	Ogre::Vector3 PhysicsActiveComponent::getCurrentForceForVelocity(void) const
	{
		return this->forceForVelocity;
	}

	Ogre::Vector3 PhysicsActiveComponent::getForce(void) const
	{
		if (nullptr != this->physicsBody)
			return this->physicsBody->getForce();
		else
			return Ogre::Vector3::ZERO;
	}

	void PhysicsActiveComponent::setMass(Ogre::Real mass)
	{
		if (mass <= 0.0f)
		{
			mass = 1.0f;
		}
		this->mass->setValue(mass);
	}

	Ogre::Real PhysicsActiveComponent::getMass(void) const
	{
		return this->mass->getReal();
	}

	void PhysicsActiveComponent::setMassOrigin(const Ogre::Vector3& massOrigin)
	{
		this->massOrigin->setValue(massOrigin);
	}

	const Ogre::Vector3 PhysicsActiveComponent::getMassOrigin(void) const
	{
		return this->massOrigin->getVector3();
	}

	void PhysicsActiveComponent::setGravity(const Ogre::Vector3& gravity)
	{
		this->gravity->setValue(gravity);
		if (nullptr != this->physicsBody)
			this->physicsBody->setGravity(gravity);
	}

	const Ogre::Vector3 PhysicsActiveComponent::getGravity(void) const
	{
		return this->gravity->getVector3();
	}

	void PhysicsActiveComponent::setMaxSpeed(Ogre::Real maxSpeed)
	{
		this->maxSpeed->setValue(maxSpeed);
	}

	Ogre::Real PhysicsActiveComponent::getMaxSpeed(void) const
	{
		return this->maxSpeed->getReal();
	}

	void PhysicsActiveComponent::setMinSpeed(Ogre::Real minSpeed)
	{
		this->minSpeed->setValue(minSpeed);
	}

	Ogre::Real PhysicsActiveComponent::getMinSpeed(void) const
	{
		return this->minSpeed->getReal();
	}

	void PhysicsActiveComponent::setSpeed(Ogre::Real speed)
	{
		this->speed->setValue(speed);
	}

	Ogre::Real PhysicsActiveComponent::getSpeed(void) const
	{
		return this->speed->getReal();
	}

#if 0
	void PhysicsActiveComponent::setConstraintDirection(const Ogre::Vector3& constraintDirection)
	{
		this->releaseConstraintDirection();

		this->constraintDirection->setValue(constraintDirection);
		// Only set pin, if there is no constraint axis, because constraint axis also uses pin
		if (nullptr != this->physicsBody && Ogre::Vector3::ZERO != this->constraintDirection->getVector3() && Ogre::Vector3::ZERO == this->constraintAxis->getVector3())
		{
			this->upVector = new OgreNewt::UpVector(this->physicsBody, this->constraintDirection->getVector3());
		}
		else
		{
			this->releaseConstraintDirection();
			if (nullptr != this->planeConstraint)
				this->planeConstraint->setPin(constraintDirection);
		}
	}

	const Ogre::Vector3 PhysicsActiveComponent::getConstraintDirection(void) const
	{
		return this->constraintDirection->getVector3();
	}

	void PhysicsActiveComponent::releaseConstraintDirection(void)
	{
		if (nullptr != this->upVector)
		{
			this->upVector->destroyJoint(this->ogreNewt);
			delete this->upVector;
			this->upVector = nullptr;
		}
	}

	void PhysicsActiveComponent::setConstraintAxis(const Ogre::Vector3& constraintAxis)
	{
		this->releaseConstraintAxis();
		
		// Representantive axis
		// E.g. 0 0 1, means motion is possible on x,y axis
		this->constraintAxis->setValue(constraintAxis);

		if (nullptr != this->physicsBody && Ogre::Vector3::ZERO != this->constraintAxis->getVector3())
		{
			// If constraint axis is set, release the constraint direction joint, because a special plane and up vector will be used, 
			// so that game object will be moved on plane and can still be rotated around y-axis
			this->releaseConstraintDirection();

			this->planeConstraint = new OgreNewt::Plane2DUpVectorJoint(this->physicsBody, this->physicsBody->getPosition(), this->constraintAxis->getVector3(), this->constraintDirection->getVector3());
		}
		else
		{
			this->releaseConstraintAxis();
		}
	}

	void PhysicsActiveComponent::releaseConstraintAxis(void)
	{
		if (nullptr != this->planeConstraint)
		{
			this->planeConstraint->destroyJoint(this->ogreNewt);
			delete this->planeConstraint;
			this->planeConstraint = nullptr;
		}
		// If plane has been release, it could be, that pinning should still be active, so set pin again, for just up vector
		this->setConstraintDirection(this->constraintDirection->getVector3());
	}
#endif

#if 1
	void PhysicsActiveComponent::setConstraintDirection(const Ogre::Vector3& constraintDirection)
	{
		this->releaseConstraintDirection();

		this->constraintDirection->setValue(constraintDirection);
		// Only set pin, if there is no constraint axis, because constraint axis also uses pin
		if (nullptr != this->physicsBody && Ogre::Vector3::ZERO != this->constraintDirection->getVector3())
		{
			this->upVector = new OgreNewt::UpVector(this->physicsBody, this->constraintDirection->getVector3());
		}
	}

	const Ogre::Vector3 PhysicsActiveComponent::getConstraintDirection(void) const
	{
		return this->constraintDirection->getVector3();
	}

	void PhysicsActiveComponent::releaseConstraintDirection(void)
	{
		if (nullptr != this->upVector)
		{
			this->upVector->destroyJoint(this->ogreNewt);
			delete this->upVector;
			this->upVector = nullptr;
		}
	}

	void PhysicsActiveComponent::setConstraintAxis(const Ogre::Vector3& constraintAxis)
	{
		this->releaseConstraintAxis();

		// Representantive axis
		// E.g. 0 0 1, means motion is possible on x,y axis
		this->constraintAxis->setValue(constraintAxis);

		if (nullptr != this->physicsBody && Ogre::Vector3::ZERO != this->constraintAxis->getVector3())
		{
			this->planeConstraint = new OgreNewt::PlaneConstraint(this->physicsBody, nullptr, this->physicsBody->getPosition(), this->constraintAxis->getVector3());
		}
	}

	void PhysicsActiveComponent::releaseConstraintAxis(void)
	{
		if (nullptr != this->planeConstraint)
		{
			this->planeConstraint->destroyJoint(this->ogreNewt);
			delete this->planeConstraint;
			this->planeConstraint = nullptr;
		}
	}
#endif

	void PhysicsActiveComponent::releaseConstraintAxisPin(void)
	{
		// Pin can also be set for plane constraint
		if (nullptr != this->planeConstraint)
		{
			// this->planeConstraint->setPin(Ogre::Vector3::ZERO);
		}
	}

	void PhysicsActiveComponent::setCollisionDirection(const Ogre::Vector3& collisionDirection)
	{
		this->collisionDirection->setValue(collisionDirection);
	}

	const Ogre::Vector3 PhysicsActiveComponent::getCollisionDirection(void) const
	{
		return this->collisionDirection->getVector3();
	}

	void PhysicsActiveComponent::setCollisionPosition(const Ogre::Vector3& collisionPosition)
	{
		this->collisionPosition->setValue(collisionPosition);
	}

	const Ogre::Vector3 PhysicsActiveComponent::getCollisionPosition(void) const
	{
		return this->collisionPosition->getVector3();
	}

	void PhysicsActiveComponent::setCollisionSize(const Ogre::Vector3& collisionSize)
	{
		this->collisionSize->setValue(collisionSize);
	}

	const Ogre::Vector3 PhysicsActiveComponent::getCollisionSize(void) const
	{
		return this->collisionSize->getVector3();
	}

	const Ogre::Vector3 PhysicsActiveComponent::getConstraintAxis(void) const
	{
		return this->constraintAxis->getVector3();
	}
	
	void PhysicsActiveComponent::setContinuousCollision(bool continuousCollision)
	{
		this->continuousCollision->setValue(continuousCollision);
		if (nullptr != this->physicsBody)
		{
			this->physicsBody->setContinuousCollisionMode(true == continuousCollision ? 1 : 0);
		}
	}
		
	bool PhysicsActiveComponent::getContinuousCollision(void) const
	{
		return this->continuousCollision->getBool();
	}

	void PhysicsActiveComponent::setClampOmega(Ogre::Real clampValue)
	{
		this->clampedOmega = clampValue;
	}
		
	void PhysicsActiveComponent::setAsSoftBody(bool asSoftBody)
	{
		this->asSoftBody->setValue(asSoftBody);
		if (true == asSoftBody)
		{
			this->createSoftBody();
		}
		/*else
		{
			this->reCreateCollision();
		}*/
	}

	bool PhysicsActiveComponent::getIsSoftBody(void) const
	{
		return this->asSoftBody->getBool();
	}

	void PhysicsActiveComponent::setGravitySourceCategory(const Ogre::String& gravitySourceCategory)
	{
		this->gravitySourceCategory->setValue(gravitySourceCategory);
	}

	Ogre::String PhysicsActiveComponent::getGravitySourceCategory(void) const
	{
		return this->gravitySourceCategory->getString();
	}

	/*void PhysicsActiveComponent::setDefaultPoseName(const Ogre::String& defaultPoseName)
	{
		this->defaultPoseName = defaultPoseName;
	}

	const Ogre::String PhysicsActiveComponent::getDefaultPoseName(void) const
	{
		return this->defaultPoseName;
	}*/

	bool PhysicsActiveComponent::getHasAttraction(void) const
	{
		return this->hasAttraction;
	}

	bool PhysicsActiveComponent::getHasSpring(void) const
	{
		return this->hasSpring;
	}

	void PhysicsActiveComponent::actualizeValue(Variant* attribute)
	{
		this->actualizeCommonValue(attribute);
	}

	void PhysicsActiveComponent::actualizeCommonValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (PhysicsActiveComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (GameObject::AttrClampY() == attribute->getName())
		{
			// Clamp physics y coordinate, if activated and something to clamp to
			if (nullptr != this->physicsBody)
			{
				this->setPosition(this->physicsBody->getPosition().x, this->gameObjectPtr->getSceneNode()->getPosition().y, this->physicsBody->getPosition().z);
			}
		}
		else if (PhysicsActiveComponent::AttrCollisionSize() == attribute->getName())
		{
			this->collisionSize->setValue(attribute->getVector3());
			this->reCreateCollision();
		}
		else if (PhysicsActiveComponent::AttrCollisionPosition() == attribute->getName())
		{
			this->collisionPosition->setValue(attribute->getVector3());
			this->reCreateCollision();
		}
		else if (PhysicsActiveComponent::AttrCollisionDirection() == attribute->getName())
		{
			this->collisionDirection->setValue(attribute->getVector3());
			this->reCreateCollision();
		}
		else if (PhysicsComponent::AttrMass() == attribute->getName())
		{
			this->setMass(attribute->getReal());
		}
		else if (PhysicsActiveComponent::AttrMassOrigin() == attribute->getName())
		{
			this->massOrigin->setValue(attribute->getVector3());
			if (nullptr != this->physicsBody)
			{
				this->physicsBody->setCenterOfMass(attribute->getVector3());
			}
		}
		else if (PhysicsActiveComponent::AttrLinearDamping() == attribute->getName())
		{
			this->linearDamping->setValue(attribute->getReal());
			if (nullptr != this->physicsBody)
			{
				this->physicsBody->setLinearDamping(attribute->getReal());
			}
		}
		else if (PhysicsActiveComponent::AttrAngularDamping() == attribute->getName())
		{
			this->angularDamping->setValue(attribute->getVector3());
			if (nullptr != this->physicsBody)
			{
				this->physicsBody->setAngularDamping(attribute->getVector3());
			}
		}
		else if (PhysicsActiveComponent::AttrGravity() == attribute->getName())
		{
			this->setGravity(attribute->getVector3());
		}
		else if (PhysicsActiveComponent::AttrGravitySourceCategory() == attribute->getName())
		{
			this->gravitySourceCategory->setValue(attribute->getString());
		}
		else if (PhysicsActiveComponent::AttrConstraintDirection() == attribute->getName())
		{
			this->setConstraintDirection(attribute->getVector3());
		}
		else if (PhysicsActiveComponent::AttrForce() == attribute->getName())
		{
			this->applyForce(attribute->getVector3());
		}
		else if (PhysicsActiveComponent::AttrContinuousCollision() == attribute->getName())
		{
			this->setContinuousCollision(attribute->getBool());
		}
		else if (PhysicsActiveComponent::AttrSpeed() == attribute->getName())
		{
			this->speed->setValue(attribute->getReal());
			// Is this necessary?
			// this->minSpeed->setValue(attribute->getReal());
		}
		else if (PhysicsActiveComponent::AttrMaxSpeed() == attribute->getName())
		{
			this->maxSpeed->setValue(attribute->getReal());
		}
		else if (PhysicsActiveComponent::AttrMinSpeed() == attribute->getName())
		{
			this->minSpeed->setValue(attribute->getReal());
		}
		else if (PhysicsComponent::AttrCollisionType() == attribute->getName())
		{
			// Only do something if the collision type changed
			this->collisionType->setListSelectedValue(attribute->getListSelectedValue());
			this->reCreateCollision();
		}
		else if (PhysicsActiveComponent::AttrConstraintAxis() == attribute->getName())
		{
			this->constraintAxis->setValue(attribute->getVector3());
			this->setConstraintAxis(attribute->getVector3());
		}
		else if (PhysicsActiveComponent::AttrAsSoftBody() == attribute->getName())
		{
			this->setAsSoftBody(attribute->getBool());
		}
		else if (PhysicsActiveComponent::AttrEnableGyroscopicTorque() == attribute->getName())
		{
			this->setGyroscopicTorqueEnabled(attribute->getBool());
		}
		else if (PhysicsComponent::AttrCollidable() == attribute->getName())
		{
			this->setCollidable(attribute->getBool());
		}
	}

	void PhysicsActiveComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		this->writeCommonProperties(propertiesXML, doc);
	}

	void PhysicsActiveComponent::writeCommonProperties(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		GameObjectComponent::writeXML(propertiesXML, doc, "");

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->activated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CollisionSize"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->collisionSize->getVector3())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OffsetPosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->collisionPosition->getVector3())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OffsetDirection"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->collisionDirection->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Mass"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->mass->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MassOrigin"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->massOrigin->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "LinearDamping"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->linearDamping->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AngularDamping"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->angularDamping->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Gravity"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->gravity->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "GravitySourceCategory"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->gravitySourceCategory->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ConstraintDirection"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->constraintDirection->getVector3())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ContinuousCollision"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->continuousCollision->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Speed"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->speed->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MinSpeed"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->minSpeed->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MaxSpeed"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->maxSpeed->getReal())));
		propertiesXML->append_node(propertyXML);

		/*propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DefaultPoseName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->defaultPoseName)));
		propertiesXML->append_node(propertyXML);*/

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CollisionType"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->collisionType->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ConstraintAxis"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->constraintAxis->getVector3())));
		propertiesXML->append_node(propertyXML);

		/*propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AsSoftBody"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->asSoftBody->getBool())));
		propertiesXML->append_node(propertyXML);*/

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "EnableGyroscopicTorque"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->gyroscopicTorque->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Collidable"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->collidable->getBool())));
		propertiesXML->append_node(propertyXML);
	}

	void PhysicsActiveComponent::showDebugData(void)
	{
		GameObjectComponent::showDebugData();
		if (nullptr != this->physicsBody)
		{
			this->physicsBody->showDebugCollision(false, this->bShowDebugData);
		}
	}

	PhysicsActiveComponent::ContactData PhysicsActiveComponent::getContactAhead(int index, const Ogre::Vector3& offset, Ogre::Real length, bool forceDrawLine, unsigned int categoryIds)
	{
		GameObject* gameObject = nullptr;
		Ogre::Real height = 500.0f;
		Ogre::Real slope = 0.0f;
		Ogre::Vector3 normal = Ogre::Vector3::UNIT_SCALE * 100.0f;

		// raycast in direction of the object
		Ogre::Vector3 direction = this->physicsBody->getOrientation() * this->gameObjectPtr->getDefaultDirection();
		// get the position relative to direction and an offset
		Ogre::Vector3 position = this->physicsBody->getPosition() + (this->physicsBody->getOrientation() * offset);

		// get the position relative to direction and an offset
		Ogre::Vector3 fromPosition = position;
		Ogre::Vector3 toPosition = position + (direction * length);

		// shoot the ray in that direction
		OgreNewt::BasicRaycast ray(this->ogreNewt, fromPosition, toPosition, true);
		
		if (true == this->bShowDebugData || true == forceDrawLine)
		{
			// Get a key from id and user index to match a line, when line drawing is set to on
			Ogre::String key = std::to_string(this->gameObjectPtr->getId()) + std::to_string(index) + "getContactAhead";
			auto& it = this->drawLineMap.find(key);
			if (it == this->drawLineMap.cend())
			{
				Ogre::SceneNode* debugLineNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();
				Ogre::ManualObject* debugLineObject = this->gameObjectPtr->getSceneManager()->createManualObject();
				debugLineObject->setQueryFlags(0 << 0);
				debugLineObject->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_OBJECTS_ALWAYS_IN_FOREGROUND);
				debugLineObject->setCastShadows(false);
				debugLineNode->attachObject(debugLineObject);
				this->drawLineMap.emplace(key, std::make_pair(debugLineNode, debugLineObject));
				debugLineObject->clear();
				debugLineObject->begin("RedNoLighting", Ogre::OperationType::OT_LINE_LIST);
				debugLineObject->position(fromPosition);
				debugLineObject->index(0);
				debugLineObject->position(toPosition);
				debugLineObject->index(1);
				debugLineObject->end();
			}
			else
			{
				Ogre::ManualObject* debugLineObject = it->second.second;
				debugLineObject->clear();
				debugLineObject->begin("RedNoLighting", Ogre::OperationType::OT_LINE_LIST);
				debugLineObject->position(fromPosition);
				debugLineObject->index(0);
				debugLineObject->position(toPosition);
				debugLineObject->index(1);
				debugLineObject->end();
			}
		}

		// get contact result
		OgreNewt::BasicRaycast::BasicRaycastInfo info = ray.getFirstHit();
		if (info.mBody)
		{
			unsigned int type = info.mBody->getType();
			unsigned int finalType = type & categoryIds;
			if (type == finalType)
			{
				//* 500 da bei der distanz ein Wert zwischen [0,1] rauskommt also zb. 0.0019
				//Die Distanz ist relativ zur L�nge des Raystrahls
				//d. h. wenn der Raystrahl nichts mehr trifft wird in diesem Fall
				//die Distanz > 500, da prozentural, es wird vorher skaliert
				height = info.mDistance * 500.0f;

				//Y
				//|
				//|___ Normale
				//Winkel zwischen Y-Richtung und der normalen eines Objektes errechnen
				Ogre::Vector3 vec = Ogre::Vector3::UNIT_Y;
				normal = info.mNormal;
				slope = Ogre::Math::ACos(vec.dotProduct(normal) / (vec.length() * normal.length())).valueDegrees();

				try
				{
					// here no shared_ptr because in this scope the game object should not extend the lifecycle! Only shared where really necessary
					Ogre::SceneNode* tempNode = static_cast<Ogre::SceneNode*>(info.mBody->getOgreNode());
					if (tempNode)
					{
						gameObject = Ogre::any_cast<GameObject*>(tempNode->getUserObjectBindings().getUserAny());
						// PhysicsComponent* physicsComponent = OgreNewt::any_cast<PhysicsComponent*>(info.mBody->getUserData());
						// GameObjectPtr gameObjectPtr = Ogre::any_cast<GameObject*>((*it).movable->getUserAny());
						// return physicsComponent->getOwner().get();
					}
					else
					{
						return std::move(PhysicsActiveComponent::ContactData(nullptr, height, normal, slope));
					}
				}
				catch (...)
				{
					// if its a game object or else, catch the throw and return from the function
					return std::move(PhysicsActiveComponent::ContactData(nullptr, height, normal, slope));
				}
			}
		}
		return std::move(PhysicsActiveComponent::ContactData(gameObject, height, normal, slope));

	}

	PhysicsActiveComponent::ContactData PhysicsActiveComponent::getContactToDirection(int index, const Ogre::Vector3& direction, const Ogre::Vector3& offset, 
		Ogre::Real from, Ogre::Real to, bool forceDrawLine, unsigned int categoryIds)
	{
		GameObject* gameObject = nullptr;
		Ogre::Real height = 500.0f;
		Ogre::Real slope = 0.0f;
		Ogre::Vector3 normal = Ogre::Vector3::UNIT_SCALE * 100.0f;
		// raycast in direction of the object
		Ogre::Vector3 position = this->physicsBody->getPosition() + (this->physicsBody->getOrientation() * offset);
	
		// get the position relative to direction and an offset
		Ogre::Vector3 fromPosition = position + (direction * from);
		Ogre::Vector3 toPosition = position + (direction * to);

		OgreNewt::BasicRaycast ray(this->ogreNewt, fromPosition, toPosition, true);
		
		if (true == this->bShowDebugData || true == forceDrawLine)
		{
			// Get a key from id and user index to match a line, when line drawing is set to on
			Ogre::String key = std::to_string(this->gameObjectPtr->getId()) + std::to_string(index) + "getContactToDirection";
			auto& it = this->drawLineMap.find(key);
			if (it == this->drawLineMap.cend())
			{
				Ogre::SceneNode* debugLineNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();
				Ogre::ManualObject* debugLineObject = this->gameObjectPtr->getSceneManager()->createManualObject();
				debugLineObject->setQueryFlags(0 << 0);
				debugLineObject->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_OBJECTS_ALWAYS_IN_FOREGROUND);
				debugLineObject->setCastShadows(false);
				debugLineNode->attachObject(debugLineObject);
				this->drawLineMap.emplace(key, std::make_pair(debugLineNode, debugLineObject));
				debugLineObject->clear();
				debugLineObject->begin("RedNoLighting", Ogre::OperationType::OT_LINE_LIST);
				debugLineObject->position(fromPosition);
				debugLineObject->index(0);
				debugLineObject->position(toPosition);
				debugLineObject->index(1);
				debugLineObject->end();
			}
			else
			{
				Ogre::ManualObject* debugLineObject = it->second.second;
				debugLineObject->clear();
				debugLineObject->begin("RedNoLighting", Ogre::OperationType::OT_LINE_LIST);
				debugLineObject->position(fromPosition);
				debugLineObject->index(0);
				debugLineObject->position(toPosition);
				debugLineObject->index(1);
				debugLineObject->end();
			}
		}
		
		// get contact result
		OgreNewt::BasicRaycast::BasicRaycastInfo info = ray.getFirstHit();
		if (info.mBody)
		{
			unsigned int type = info.mBody->getType();
			unsigned int finalType = type & categoryIds;
			if (type == finalType)
			{
				//* 500 da bei der distanz ein Wert zwischen [0,1] rauskommt also zb. 0.0019
				//Die Distanz ist relativ zur L�nge des Raystrahls
				//d. h. wenn der Raystrahl nichts mehr trifft wird in diesem Fall
				//die Distanz > 500, da prozentural, es wird vorher skaliert
				height = info.mDistance * 500.0f;

				//Y
				//|
				//|___ Normale
				//Winkel zwischen Y-Richtung und der normalen eines Objektes errechnen
				Ogre::Vector3 vec = Ogre::Vector3::UNIT_Y;
				normal = info.mNormal;
				slope = Ogre::Math::ACos(vec.dotProduct(normal) / (vec.length() * normal.length())).valueDegrees();

				try
				{
					// here no shared_ptr because in this scope the game object should not extend the lifecycle! Only shared where really necessary
					Ogre::SceneNode* tempNode = static_cast<Ogre::SceneNode*>(info.mBody->getOgreNode());
					if (tempNode)
					{
						gameObject = Ogre::any_cast<GameObject*>(tempNode->getUserObjectBindings().getUserAny());
						// PhysicsComponent* physicsComponent = OgreNewt::any_cast<PhysicsComponent*>(info.mBody->getUserData());
						// GameObjectPtr gameObjectPtr = Ogre::any_cast<GameObject*>((*it).movable->getUserAny());
						// return physicsComponent->getOwner().get();
					}
					else
					{
						return std::move(PhysicsActiveComponent::ContactData(nullptr, height, normal, slope));
					}
				}
				catch (...)
				{
					// if its a game object or else, catch the throw and return from the function
					return std::move(PhysicsActiveComponent::ContactData(nullptr, height, normal, slope));
				}
			}
		}
		return std::move(PhysicsActiveComponent::ContactData(gameObject, height, normal, slope));
	}

	PhysicsActiveComponent::ContactData PhysicsActiveComponent::getContactBelow(int index, const Ogre::Vector3& positionOffset, bool forceDrawLine, unsigned int categoryIds)
	{
		GameObject* gameObject = nullptr;
		Ogre::Real height = 500.0f; // Default invalid value for checking
		Ogre::Real slope = 0.0f;
		Ogre::Vector3 normal = Ogre::Vector3::ZERO;
		//Anfangsposition
		Ogre::Vector3 charPoint = this->physicsBody->getPosition() + positionOffset;
		//Straht von Anfangsposition bis 500 Meter nach unten erzeugen
		OgreNewt::BasicRaycast ray(this->ogreNewt, charPoint, charPoint + Ogre::Vector3::NEGATIVE_UNIT_Y * 500.0f, true);

		if (true == this->bShowDebugData || true == forceDrawLine)
		{
			Ogre::Vector3 fromPosition = charPoint;
			Ogre::Vector3 toPosition = charPoint + Ogre::Vector3::NEGATIVE_UNIT_Y * 500.0f;

			// Get a key from id and user index to match a line, when line drawing is set to on
			Ogre::String key = std::to_string(this->gameObjectPtr->getId()) + std::to_string(index) + "getContactBelow";
			auto& it = this->drawLineMap.find(key);
			if (it == this->drawLineMap.cend())
			{
				Ogre::SceneNode* debugLineNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();
				Ogre::ManualObject* debugLineObject = this->gameObjectPtr->getSceneManager()->createManualObject();
				debugLineObject->setQueryFlags(0 << 0);
				debugLineObject->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_OBJECTS_ALWAYS_IN_FOREGROUND);
				debugLineObject->setCastShadows(false);
				debugLineNode->attachObject(debugLineObject);
				this->drawLineMap.emplace(key, std::make_pair(debugLineNode, debugLineObject));
				debugLineObject->clear();
				debugLineObject->begin("RedNoLighting", Ogre::OperationType::OT_LINE_LIST);
				debugLineObject->position(fromPosition);
				debugLineObject->index(0);
				debugLineObject->position(toPosition);
				debugLineObject->index(1);
				debugLineObject->end();
			}
			else
			{
				Ogre::ManualObject* debugLineObject = it->second.second;
				debugLineObject->clear();
				debugLineObject->begin("RedNoLighting", Ogre::OperationType::OT_LINE_LIST);
				debugLineObject->position(fromPosition);
				debugLineObject->index(0);
				debugLineObject->position(toPosition);
				debugLineObject->index(1);
				debugLineObject->end();
			}
		}

		OgreNewt::BasicRaycast::BasicRaycastInfo info = ray.getFirstHit();
		if (info.mBody)
		{
			unsigned int type = info.mBody->getType();
			unsigned int finalType = type & categoryIds;
			if (type == finalType)
			{
				//* 500 da bei der distanz ein Wert zwischen [0,1] rauskommt also zb. 0.0019
				//Die Distanz ist relativ zur L�nge des Raystrahls
				//d. h. wenn der Raystrahl nichts mehr trifft wird in diesem Fall
				//die Distanz > 500, da prozentural, es wird vorher skaliert
				height = info.mDistance * 500.0f;

				//Y
				//|
				//|___ Normale
				//Winkel zwischen Y-Richtung und der normalen eines Objektes errechnen
				Ogre::Vector3 vec = Ogre::Vector3::UNIT_Y;
				normal = info.mNormal;
				slope = Ogre::Math::ACos(vec.dotProduct(normal) / (vec.length() * normal.length())).valueDegrees();

				try
				{
					// here no shared_ptr because in this scope the game object should not extend the lifecycle! Only shared where really necessary
					Ogre::SceneNode* tempNode = static_cast<Ogre::SceneNode*>(info.mBody->getOgreNode());
					if (tempNode)
					{
						gameObject = Ogre::any_cast<GameObject*>(tempNode->getUserObjectBindings().getUserAny());
						// PhysicsComponent* physicsComponent = OgreNewt::any_cast<PhysicsComponent*>(info.mBody->getUserData());
						// GameObjectPtr gameObjectPtr = Ogre::any_cast<GameObject*>((*it).movable->getUserAny());
						// return physicsComponent->getOwner().get();
					}
					else
					{
						return std::move(PhysicsActiveComponent::ContactData(nullptr, height, normal, slope));
					}
				}
				catch (...)
				{
					// if its a game object or else, catch the throw and return from the function
					return std::move(PhysicsActiveComponent::ContactData(nullptr, height, normal, slope));
				}
			}
		}
		return std::move(PhysicsActiveComponent::ContactData(gameObject, height, normal, slope));
	}

	GameObject* PhysicsActiveComponent::getContact(int index, const Ogre::Vector3& direction, const Ogre::Vector3& offset, 
		Ogre::Real from, Ogre::Real to, bool drawLine, unsigned int categoryIds)
	{
		Ogre::Vector3 tempDirection = this->physicsBody->getOrientation() * direction;
		GameObject* gameObject = nullptr;
		// raycast in direction of the object
		Ogre::Vector3 position = this->physicsBody->getPosition() + (this->physicsBody->getOrientation() * offset);
		// get the position relative to direction and an offset

		Ogre::Vector3 fromPosition = position + (tempDirection * from);
		Ogre::Vector3 toPosition = position + (tempDirection * to);

		OgreNewt::BasicRaycast ray(this->ogreNewt, fromPosition, toPosition, true);

		if (true == drawLine)
		{
			// Get a key from id and user index to match a line, when line drawing is set to on
			Ogre::String key = std::to_string(this->gameObjectPtr->getId()) + std::to_string(index) + "getContact";
			auto& it = this->drawLineMap.find(key);
			if (it == this->drawLineMap.cend())
			{
				Ogre::SceneNode* debugLineNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();
				Ogre::ManualObject* debugLineObject = this->gameObjectPtr->getSceneManager()->createManualObject();
				debugLineObject->setQueryFlags(0 << 0);
				debugLineObject->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_OBJECTS_ALWAYS_IN_FOREGROUND);
				debugLineObject->setCastShadows(false);
				debugLineNode->attachObject(debugLineObject);
				this->drawLineMap.emplace(key, std::make_pair(debugLineNode, debugLineObject));
			}
			else
			{
				Ogre::ManualObject* debugLineObject = it->second.second;
				debugLineObject->clear();
				debugLineObject->begin("BlueNoLighting", Ogre::OperationType::OT_LINE_LIST);
				debugLineObject->position(fromPosition);
				debugLineObject->index(0);
				debugLineObject->position(toPosition);
				debugLineObject->index(1);
				debugLineObject->end();
			}
		}

		// get contact result
		OgreNewt::BasicRaycast::BasicRaycastInfo info = ray.getFirstHit();
		if (info.mBody)
		{
			unsigned int type = info.mBody->getType();
			unsigned int finalType = type & categoryIds;
			if (type == finalType)
			{
				try
				{
					// here no shared_ptr because in this scope the game object should not extend the lifecycle! Only shared where really necessary
					Ogre::SceneNode* tempNode = static_cast<Ogre::SceneNode*>(info.mBody->getOgreNode());
					if (tempNode)
					{
						gameObject = Ogre::any_cast<GameObject*>(tempNode->getUserObjectBindings().getUserAny());
					}
					else
					{
						return gameObject;
					}
				}
				catch (...)
				{
					// if its a game object or else, catch the throw and return from the function
					return gameObject;
				}
			}
		}
		return gameObject;
	}

	bool PhysicsActiveComponent::getFixedContactToDirection(int index, const Ogre::Vector3& direction, const Ogre::Vector3& offset, Ogre::Real scale, unsigned int categoryIds)
	{
		// raycast in direction of the object
		Ogre::Vector3 position = this->physicsBody->getPosition() + (this->physicsBody->getOrientation() * offset);
		// get the position relative to direction and an offset
		OgreNewt::BasicRaycast ray(this->ogreNewt, position, position + (direction * scale), true);

		// get contact result
		OgreNewt::BasicRaycast::BasicRaycastInfo info = ray.getFirstHit();
		if (info.mBody)
		{
			unsigned int type = info.mBody->getType();
			unsigned int finalType = type & categoryIds;
			if (type == finalType)
			{
				try
				{
					PhysicsActiveComponent* phsicsActiveComponent = OgreNewt::any_cast<PhysicsActiveComponent*>(info.getBody()->getUserData());

					if (phsicsActiveComponent)
					{
						return true;
					}
					else
					{
						return false;
					}
				}
				catch (...)
				{
					// if its a game object or else, catch the throw and return from the function
					return false;
				}
			}
		}
		return false;
	}

	Ogre::Real PhysicsActiveComponent::determineGameObjectHeight(const Ogre::Vector3& positionOffset1, const Ogre::Vector3& positionOffset2, unsigned int categoryIds)
	{
		Ogre::Real height = 500.0f;
		//Anfangsposition
		Ogre::Vector3 charPoint = this->physicsBody->getPosition() + positionOffset1;
		//Straht von Anfangsposition bis 500 Meter nach unten erzeugen
		OgreNewt::BasicRaycast ray(this->ogreNewt, charPoint, charPoint + Ogre::Vector3::NEGATIVE_UNIT_Y * 500.0f, true);

		OgreNewt::BasicRaycast::BasicRaycastInfo& info = ray.getFirstHit();
		if (info.mBody)
		{
			unsigned int type = info.mBody->getType();
			unsigned int finalType = type & categoryIds;
			if (type == finalType)
			{
				height = info.mDistance * 500.0f;

				charPoint = this->physicsBody->getPosition() + positionOffset2;
				//Straht von Anfangsposition bis 500 Meter nach unten erzeugen
				ray = OgreNewt::BasicRaycast(this->ogreNewt, charPoint, charPoint + Ogre::Vector3::NEGATIVE_UNIT_Y * 500.0f, true);
				info = ray.getFirstHit();
				if (info.mBody)
				{
					if (info.mDistance * 500.0f < height)
					{
						height = info.mDistance * 500.0f;
					}
				}
			}
		}
		return height;
	}

	void PhysicsActiveComponent::setBounds(const Ogre::Vector3& minBounds, const Ogre::Vector3& maxBounds)
	{
		this->usesBounds = true;
		this->minBounds = minBounds;
		this->maxBounds = maxBounds;
	}

	void PhysicsActiveComponent::removeBounds(void)
	{
		this->usesBounds = false;
	}

	void PhysicsActiveComponent::attachForceObserver(IForceObserver* forceObserver)
	{
		auto& it = this->forceObservers.find(forceObserver->getName());
		if (it == this->forceObservers.end())
		{
			this->forceObservers.emplace(forceObserver->getName(), forceObserver);
		}
	}

	void PhysicsActiveComponent::detachAndDestroyForceObserver(const Ogre::String& name)
	{
		auto& it = this->forceObservers.find(name);
		if (it != this->forceObservers.end())
		{
			IForceObserver* observer = it->second;
			this->forceObservers.erase(it);
			delete observer;
			observer = nullptr;
		}
	}

	void PhysicsActiveComponent::setContactSolvingEnabled(bool enable)
	{
		if (true == enable)
		{
			this->physicsBody->setContactCallback<PhysicsActiveComponent>(&PhysicsActiveComponent::contactCallback, this);
		}
		else
		{
			this->physicsBody->removeContactCallback();
		}
	}

	void PhysicsActiveComponent::detachAndDestroyAllForceObserver(void)
	{
		for (auto& it = this->forceObservers.cbegin(); it != this->forceObservers.cend();)
		{
			IForceObserver* observer = it->second;
			it = this->forceObservers.erase(it);
			delete observer;
			observer = nullptr;
		}
		this->forceObservers.clear();
	}

	void PhysicsActiveComponent::addJointSpringComponent(unsigned long id)
	{
		bool alreadyFound = false;
		for (size_t i = 0; i < this->springs.size(); i++)
		{
			if (this->springs[i] == id)
			{
				alreadyFound = true;
				break;
			}
		}
		if (false == alreadyFound)
		{
			this->springs.emplace_back(id);
		}
		if (this->springs.size() > 0)
		{
			this->hasSpring = true;
		}
	}

	void PhysicsActiveComponent::removeJointSpringComponent(unsigned long id)
	{
		for (size_t i = 0; i < this->springs.size(); i++)
		{
			if (this->springs[i] == id)
			{
				this->springs.erase(this->springs.begin() + i);
				break;
			}
		}
		if (0 == this->springs.size())
		{
			this->hasSpring = false;
		}
	}

	void PhysicsActiveComponent::addJointAttractorComponent(unsigned long id)
	{
		bool alreadyFound = false;
		for (size_t i = 0; i < this->physicsAttractors.size(); i++)
		{
			if (this->physicsAttractors[i] == id)
			{
				alreadyFound = true;
				break;
			}
		}
		if (false == alreadyFound)
		{
			this->physicsAttractors.emplace_back(id);
		}
		if (this->physicsAttractors.size() > 0)
		{
			this->hasAttraction = true;
		}
	}

	void PhysicsActiveComponent::removeJointAttractorComponent(unsigned long id)
	{
		for (size_t i = 0; i < this->physicsAttractors.size(); i++)
		{
			if (this->physicsAttractors[i] == id)
			{
				this->physicsAttractors.erase(this->physicsAttractors.begin() + i);
				break;
			}
		}
		if (0 == this->physicsAttractors.size())
		{
			this->hasAttraction = false;
		}
	}

	void PhysicsActiveComponent::destroyLineMap(void)
	{
		for (auto& it = this->drawLineMap.cbegin(); it != this->drawLineMap.cend(); ++it)
		{
			Ogre::SceneNode* debugLineNode = it->second.first;
			Ogre::ManualObject* debugLineObject = it->second.second;
			debugLineObject->clear();

			debugLineNode->detachAllObjects();
			this->gameObjectPtr->getSceneManager()->destroySceneNode(debugLineNode);
			debugLineNode = nullptr;

			gameObjectPtr->getSceneManager()->destroyMovableObject(debugLineObject);
			debugLineObject = nullptr;
		}
		this->drawLineMap.clear();
	}

	void PhysicsActiveComponent::moveCallback(OgreNewt::Body* body, Ogre::Real timeStep, int threadIndex)
	{
		/////////////////////Standard gravity force/////////////////////////////////////
		Ogre::Vector3 wholeForce = body->getGravity();
		Ogre::Real mass = 0.0f;
		Ogre::Vector3 inertia = Ogre::Vector3::ZERO;

		// Dangerous: Causes newton crash!
		// Clamp omega, when activated (should only be rarely the case!)
		/*if (0.0f != this->clampedOmega)
		{
			Ogre::Vector3 omega = body->getOmega();
			Ogre::Real mag2 = omega.dotProduct(omega);
			if (mag2 > (this->clampedOmega * this->clampedOmega))
			{
				omega = omega.normalise() * this->clampedOmega;
				body->setOmega(omega);
			}
		}*/

		Ogre::Real nearestPlanetDistance = std::numeric_limits<Ogre::Real>::max();
		GameObjectPtr nearestGravitySourceObject;

		if (false == this->gravitySourceCategory->getString().empty())
		{
			// If there is a gravity source GO, calculate gravity in that direction of the source, but only work with the nearest object, else the force will mess up
			auto gravitySourceGameObjects = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsFromCategory(this->gravitySourceCategory->getString());
			for (size_t i = 0; i < gravitySourceGameObjects.size(); i++)
			{
				wholeForce = this->getPosition() - gravitySourceGameObjects[i]->getPosition();
				Ogre::Real squaredDistanceToGravitySource = wholeForce.squaredLength();
				if (squaredDistanceToGravitySource < nearestPlanetDistance)
				{
					nearestPlanetDistance = squaredDistanceToGravitySource;
					nearestGravitySourceObject = gravitySourceGameObjects[i];
				}
			}

			// Only do calculation for the nearest planet
			if (nullptr != nearestGravitySourceObject)
			{
				auto& gravitySourcePhysicsComponentPtr = NOWA::makeStrongPtr(nearestGravitySourceObject->getComponent<PhysicsComponent>());
				if (nullptr != gravitySourcePhysicsComponentPtr)
				{
					wholeForce = this->getPosition() - gravitySourcePhysicsComponentPtr->getPosition();
					Ogre::Real squaredDistanceToGravitySource = wholeForce.squaredLength();
					// * this->mass->getReal() is commented out, because mass is multiplicated below: wholeForce *= mass;
					Ogre::Real strength = (this->gravity->getVector3().y /** this->mass->getReal()*/ * gravitySourcePhysicsComponentPtr->getMass()) / (squaredDistanceToGravitySource);

					wholeForce.normalise();
					// Also orientate, if pin is active
					if (nullptr != this->upVector)
					{
						this->upVector->setPin(wholeForce);
					}
					wholeForce *= strength;
				}
			}
		}
		// calculate gravity
		body->getMassMatrix(mass, inertia);

		if (false == this->hasAttraction)
		{
			wholeForce *= mass;
		}

		body->addForce(wholeForce);

		// Add force does stress, when using simultanously on lots of objects, especially in a lua script!

		// Add custom force, if set from the outside
		if (Ogre::Vector3::ZERO != this->force->getVector3() || true == this->bResetForce)
		{
			body->addForce(this->force->getVector3());
			this->force->setValue(Ogre::Vector3::ZERO);
			this->bResetForce = false;
		}

		// Calculate required force for the given velocity
		if (true == this->canAddForceForVelocity)
		{
			// http://www.ogre3d.org/addonforums/viewtopic.php?f=4&t=9810&p=57245&hilit=force#p57245
			// http://newtondynamics.com/forum/viewtopic.php?f=9&t=7613&p=52138&hilit=NewtonBodyCalculateInverseDynamicsForce#p52138

			// Ogre::Vector3 currentVelocity = body->getVelocityAtPoint(body->getPosition());

			Ogre::Vector3 moveForce = (this->forceForVelocity - body->getVelocity()) * mass / timeStep;
			body->addForce(moveForce);

			this->forceForVelocity = Ogre::Vector3::ZERO;
			this->canAddForceForVelocity = false;
		}

		// add if set a angular torque to rotate an object
		//if (true == this->canAddAngularTorqueForce)
		//{
		//	body->setTorque(this->angularTorqueForce); // does not work? Crashes after a while especially when called from lua script
		//	this->angularTorqueForce = Ogre::Vector3::ZERO;
		//	this->canAddAngularTorqueForce = false;
		//}

		// add if set a angular impulse to rotate an object
		if (true == this->canAddOmegaForce || true == this->bResetForce)
		{
			body->setBodyAngularVelocity(this->omegaForce, timeStep);
			this->omegaForce = Ogre::Vector3::ZERO;
			this->canAddOmegaForce = false;
		}

		/////////////////Force observer//////////////////////////////////////////

		for (auto& it = this->forceObservers.cbegin(); it != this->forceObservers.cend(); ++it)
		{
			// notify the observer
			it->second->onForceAdd(body, timeStep, threadIndex);
		}

		/////////////////Magnetic attraction behaviour//////////////////////////////////////

		// https://www.physicsforums.com/threads/c-model-of-a-charged-particle-in-a-magnetic-field.582600/
		// http://stackoverflow.com/questions/37402504/adding-a-list-of-forces-to-a-particle-in-c
		// http://natureofcode.com/book/chapter-4-particle-systems/


		// A repeller may be attracted by several attractors with different magnetic strengths etc.
		for (auto& it = this->physicsAttractors.cbegin(); it != this->physicsAttractors.cend(); ++it)
		{
			boost::shared_ptr<JointAttractorComponent> jointAttractorCompPtr = boost::dynamic_pointer_cast<JointAttractorComponent>(
				NOWA::makeStrongPtr(AppStateManager::getSingletonPtr()->getGameObjectController()->getJointComponent(*it)));

			if (this->gameObjectPtr->getSceneNode()->getAttachedObject(0)->isVisible())
			{
				// Check if its within the attraction distance
				Ogre::Vector3 attractorDirection = body->getPosition() - jointAttractorCompPtr->getOwner()->getPosition();
				Ogre::Real squaredDistance = attractorDirection.squaredLength();
				attractorDirection.normalise();

				if (squaredDistance <= jointAttractorCompPtr->getAttractionDistance() * jointAttractorCompPtr->getAttractionDistance())
				{
					if (squaredDistance < 5.0f * 5.0f)
					{
						squaredDistance = 5.0f * 5.0f;
					}
					this->hasAttraction = true;
					// Depending on the magnetic strength and the distance to the attractor
					Ogre::Real attractorForce = (-1 * jointAttractorCompPtr->getMagneticStrength()) / (squaredDistance);
					/*Ogre::Real attractorForce = (-9.8f * NOWA::makeStrongPtr(jointAttractorComponent->getOwner()->getComponent<PhysicsActiveComponent>())->getMass() * body->getMass())
					/ (distance * distance);*/
					// (G * mass * m.mass) / (distance * distance)
					attractorDirection *= attractorForce;
					body->addForce(attractorDirection);
				}
				else
				{
					this->hasAttraction = false;
				}
			}
		}

		/////////////////////Spring Joint behaviour/////////////////////////////////////

		for (auto& it = this->springs.cbegin(); it != this->springs.cend(); ++it)
		{
			boost::shared_ptr<JointSpringComponent> jointSpringCompPtr = boost::dynamic_pointer_cast<JointSpringComponent>(
				NOWA::makeStrongPtr(AppStateManager::getSingletonPtr()->getGameObjectController()->getJointComponent(*it)));

			// Get the global position our cursor is at
			auto& predecessorJointSpringCompPtr = NOWA::makeStrongPtr(AppStateManager::getSingletonPtr()->getGameObjectController()->getJointComponent(jointSpringCompPtr->getPredecessorId()));
			if (nullptr != predecessorJointSpringCompPtr)
			{
				Ogre::Vector3 anchorPosition = predecessorJointSpringCompPtr->getBody()->getPosition() + jointSpringCompPtr->getAnchorOffsetPosition();
				Ogre::Vector3 springPosition = this->getPosition() + jointSpringCompPtr->getSpringOffsetPosition();

				// Calculate spring force
				Ogre::Vector3 dragForce = ((anchorPosition - springPosition) * mass * jointSpringCompPtr->getSpringStrength()) - body->getVelocity();

				if (jointSpringCompPtr->getShowLine())
				{
					jointSpringCompPtr->drawLine(anchorPosition, this->getPosition());
				}

				// addGlobalForce will create a bad effect, because when the spring is touched, it will start to rotate repeatedly, because global force also internally adds torque!
				// body->addGlobalForce(dragForce, springPosition); 

				// Add the spring force at the handle
				body->addForce(dragForce);
			}
		}
	}

	void PhysicsActiveComponent::contactCallback(OgreNewt::Body* otherBody)
	{
		if (nullptr != this->gameObjectPtr->getLuaScript())
		{
			PhysicsComponent* otherPhysicsComponent = OgreNewt::any_cast<PhysicsComponent*>(otherBody->getUserData());

			this->gameObjectPtr->getLuaScript()->callTableFunction("onContactSolving", otherPhysicsComponent);
		}
	}

	void PhysicsActiveComponent::updateCallback(OgreNewt::Body* body)
	{
		if (true == this->isInSimulation)
		{
			double dt = (static_cast<double>(this->timer.getMilliseconds()) * 0.001) - this->lastTime;
			this->lastTime += dt;

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "Physics dt: " + Ogre::StringConverter::toString(dt));
		}


		// Does not work, because its called to often, do not know, how to solve this, because OgreNewt::update is called from appstate manager in appstate in update function and called to often
		//if (nullptr != this->gameObjectPtr->getLuaScript())
		//{
		//	// call the fixed physics update
		//	this->gameObjectPtr->getLuaScript()->callTableFunction("physicsUpdate");
		//}
	}

}; // namespace end
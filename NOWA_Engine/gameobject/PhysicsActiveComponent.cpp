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
#include "editor/Picker.h"

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
		continuousCollision(new Variant(PhysicsActiveComponent::AttrContinuousCollision(), false, this->attributes)),
		maxSpeed(new Variant(PhysicsActiveComponent::AttrMaxSpeed(), Ogre::Real(5.0f), this->attributes)),
		speed(new Variant(PhysicsActiveComponent::AttrSpeed(), Ogre::Real(1.0f), this->attributes)),
		minSpeed(new Variant(PhysicsActiveComponent::AttrMinSpeed(), Ogre::Real(1.0f), this->attributes)),
		constraintDirection(new Variant(PhysicsActiveComponent::AttrConstraintDirection(), Ogre::Vector3::ZERO, this->attributes)),
		constraintAxis(new Variant(PhysicsActiveComponent::AttrConstraintAxis(), Ogre::Vector3::ZERO, this->attributes)),
		asSoftBody(new Variant(PhysicsActiveComponent::AttrAsSoftBody(), false, this->attributes)),
		gyroscopicTorque(new Variant(PhysicsActiveComponent::AttrEnableGyroscopicTorque(), false, this->attributes)),
		onContactFunctionName(new Variant(PhysicsActiveComponent::AttrOnContactFunctionName(), Ogre::String(""), this->attributes)),
		height(0.0f),
		rise(0.0f),
		upVector(nullptr),
		planeConstraint(nullptr),
		hasAttraction(false),
		hasSpring(false),
		lastTime(0.0),
		dt(0.0f),
		usesBounds(false),
		minBounds(Ogre::Vector3::ZERO),
		maxBounds(Ogre::Vector3::ZERO),
		gravityDirection(Ogre::Vector3::ZERO),
		currentGravityStrength(0.0f),
		up(Ogre::Vector3::ZERO),
		forward(Ogre::Vector3::ZERO),
		right(Ogre::Vector3::ZERO)
	{
		this->forceCommand.vectorValue = Ogre::Vector3::ZERO;
		this->forceCommand.pending.store(false);
		this->forceCommand.inProgress.store(false);

		this->requiredVelocityForForceCommand.vectorValue = Ogre::Vector3::ZERO;
		this->requiredVelocityForForceCommand.pending.store(false);
		this->requiredVelocityForForceCommand.inProgress.store(false);

		this->jumpForceCommand.vectorValue = Ogre::Vector3::ZERO;
		this->jumpForceCommand.pending.store(false);
		this->jumpForceCommand.inProgress.store(false);

		this->omegaForceCommand.vectorValue = Ogre::Vector3::ZERO;
		this->omegaForceCommand.pending.store(false);
		this->omegaForceCommand.inProgress.store(false);


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

		this->onContactFunctionName->setDescription("Sets the function name to react in lua script at the moment when a game object collided with another game object. E.g. onContact(otherGameObject, contact).");
		this->onContactFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), this->onContactFunctionName->getString() + "(otherGameObject, contact)");
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

	bool PhysicsActiveComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		this->parseCommonProperties(propertyElement);

		return true;
	}

	void PhysicsActiveComponent::parseCommonProperties(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

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
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OnContactFunctionName")
		{
			this->onContactFunctionName->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		// Snapshot loaded, game object pointer should exist, set transform
		if (nullptr != this->gameObjectPtr)
		{
			bool oldIsDynamic = this->gameObjectPtr->isDynamic();
			this->gameObjectPtr->setDynamic(true);
			Ogre::Vector3 position = this->gameObjectPtr->getPosition();
			Ogre::Quaternion orientation = this->gameObjectPtr->getOrientation();
			// This must be done this way, because pos, orientation must be set at once, else orientation will be the old one
			if (nullptr != this->physicsBody)
			{
				this->physicsBody->setPositionOrientation(position, orientation);
			}
			this->setScale(this->gameObjectPtr->getScale());
			this->gameObjectPtr->setDynamic(oldIsDynamic);
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

		// Gameobject available

		clonedCompPtr->setOnContactFunctionName(this->onContactFunctionName->getString());

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
			if (false == this->createDynamicBody())
			{
				return false;
			}
		}

		return success;
	}

	bool PhysicsActiveComponent::connect(void)
	{
		PhysicsComponent::connect();

		this->setOnContactFunctionName(this->onContactFunctionName->getString());

		this->lastTime = static_cast<double>(this->timer.getMilliseconds()) * 0.001;

		return true;
	}

	bool PhysicsActiveComponent::disconnect(void)
	{
		PhysicsComponent::disconnect();

		this->up = Ogre::Vector3::ZERO;
		this->forward = Ogre::Vector3::ZERO;
		this->right = Ogre::Vector3::ZERO;
		this->resetForce();
		this->destroyLineMap();

		return true;
	}

	void PhysicsActiveComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (true == this->bConnected)
		{
			// Get the gravity direction from the physics component
			if (true == this->gravityDirection.isZeroLength())
			{
				// Default on a plane or terrain
				this->gravityDirection = Ogre::Vector3::NEGATIVE_UNIT_Y;
			}

			// Calculates orientation vectors relative to planet surface
			this->up = -this->gravityDirection;
			// Gets the mesh's default direction
			Ogre::Vector3 defaultDirection = this->gameObjectPtr->getDefaultDirection();

			// Stores current entity rotation as quaternion
			Ogre::Quaternion currentRotation = this->getOrientation();

			// Calculates forward vector based on the current rotation and the default direction
			// First, gets the forward direction in the character's local space
			this->forward = currentRotation * defaultDirection;
			// Projects it onto the plane perpendicular to up vector
			this->forward = forward - up * forward.dotProduct(up);

			if (this->forward.squaredLength() < 0.001f * 0.001f) // = 0.000001f
			{
				// Fallback if forward is too small
				// Use a vector perpendicular to up that's close to our preferred direction
				Ogre::Vector3 worldForward;
				if (defaultDirection.dotProduct(Ogre::Vector3::UNIT_Z) > 0.7f)
				{
					worldForward = Ogre::Vector3::UNIT_Z;
				}
				else if (defaultDirection.dotProduct(Ogre::Vector3::UNIT_X) > 0.7f)
				{
					worldForward = Ogre::Vector3::UNIT_X;
				}
				else
				{
					worldForward = Ogre::Vector3::UNIT_Y;
				}

				// Find a suitable forward vector perpendicular to up
				this->forward = worldForward - up * worldForward.dotProduct(up);
				if (this->forward.squaredLength() < 0.001f * 0.001f)
				{
					this->forward = up.crossProduct(Ogre::Vector3(1.0f, 0.0f, 0.0f));
					if (this->forward.squaredLength() < 0.001f * 0.001f)
					{
						this->forward = up.crossProduct(Ogre::Vector3(0.0f, 0.0f, 1.0f));
					}
				}
			}
			this->forward.normalise();

			// Calculate right from forward and up (ensures orthogonality)
			this->right = this->forward.crossProduct(up);
			this->right.normalise();
		}

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

#if 0
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

		OgreNewt::CollisionPtr collisionPtr;

		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("PhysicsActiveComponent::createDynamicCollision", _4(&inertia, collisionOrientation, &calculatedMassOrigin, &collisionPtr),
		{
			collisionPtr = this->createDynamicCollision(inertia, this->collisionSize->getVector3(), this->collisionPosition->getVector3(),
				collisionOrientation, calculatedMassOrigin, this->gameObjectPtr->getCategoryId());
		});
		
		this->physicsBody = new OgreNewt::Body(this->ogreNewt, this->gameObjectPtr->getSceneManager(), collisionPtr);

		NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->registerRenderCallbackForBody(this->physicsBody);
		
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

		this->setContinuousCollision(this->continuousCollision->getBool());
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

		this->setActivated(this->activated->getBool());

		return true;
	}
#else
	bool PhysicsActiveComponent::createDynamicBody(void)
	{
		// Always snapshot the current transform as "initial" for this body
		// this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
		// this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
		// this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();

		// --- existing code from here on unchanged ---
		Ogre::Vector3 inertia = Ogre::Vector3(1.0f, 1.0f, 1.0f);

		Ogre::Quaternion collisionOrientation = Ogre::Quaternion::IDENTITY;
		if (Ogre::Vector3::ZERO != this->collisionDirection->getVector3())
		{
			collisionOrientation = MathHelper::getInstance()->degreesToQuat(this->collisionDirection->getVector3());
		}

		Ogre::Vector3 calculatedMassOrigin = Ogre::Vector3::ZERO;

		OgreNewt::CollisionPtr collisionPtr;

		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("PhysicsActiveComponent::createDynamicCollision", _4(&inertia, collisionOrientation, &calculatedMassOrigin, &collisionPtr),
		{
			collisionPtr = this->createDynamicCollision(inertia,this->collisionSize->getVector3(), this->collisionPosition->getVector3(), collisionOrientation, calculatedMassOrigin, this->gameObjectPtr->getCategoryId());
		});

		this->physicsBody = new OgreNewt::Body(this->ogreNewt, this->gameObjectPtr->getSceneManager(), collisionPtr);

		NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->registerRenderCallbackForBody(this->physicsBody);

		if (Ogre::Vector3::ZERO != this->massOrigin->getVector3())
		{
			calculatedMassOrigin = this->massOrigin->getVector3();
		}

		Ogre::Real weightedMass = this->mass->getReal();
		inertia *= weightedMass;
		this->physicsBody->setMassMatrix(weightedMass, inertia);
		this->physicsBody->setCenterOfMass(calculatedMassOrigin);

		this->physicsBody->setGravity(this->gravity->getVector3());
		this->physicsBody->setLinearDamping(this->linearDamping->getReal());
		this->physicsBody->setAngularDamping(this->angularDamping->getVector3());

		this->physicsBody->setCustomForceAndTorqueCallback<PhysicsActiveComponent>(&PhysicsActiveComponent::moveCallback, this);

		// set user data for ogrenewt
		this->physicsBody->setUserData(OgreNewt::Any(static_cast<PhysicsComponent*>(this)));
		this->physicsBody->attachNode(this->gameObjectPtr->getSceneNode());

		this->setPosition(this->initialPosition);
		this->setOrientation(this->initialOrientation);

		// Must be set after body set its position! And orientation must be correct.
		this->setConstraintAxis(this->constraintAxis->getVector3());
		this->setConstraintDirection(this->constraintDirection->getVector3());

		this->setCollidable(this->collidable->getBool());

		this->physicsBody->setType(this->gameObjectPtr->getCategoryId());

		const auto materialId = AppStateManager::getSingletonPtr()->getGameObjectController()->getMaterialID(
			this->gameObjectPtr.get(), this->ogreNewt);
		this->physicsBody->setMaterialGroupID(materialId);

		this->setActivated(this->activated->getBool());

		return true;
	}
#endif

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
		NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->registerRenderCallbackForBody(this->physicsBody);

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

	void PhysicsActiveComponent::reCreateCollision(bool overwrite)
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

		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("PhysicsComponent::createDynamicCollision", _3(&inertia, collisionOrientation, &calculatedMassOrigin),
		{
			// Set the new collision hull
			this->physicsBody->setCollision(this->createDynamicCollision(inertia, this->collisionSize->getVector3(), this->collisionPosition->getVector3(),
				collisionOrientation, calculatedMassOrigin, this->gameObjectPtr->getCategoryId()));
		});

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
		Ogre::SceneNode* rootNode = this->gameObjectPtr->getSceneNode();          // Bed
		Ogre::SceneNode* rootParent = rootNode->getParentSceneNode();            // RootSceneNode

		// Treat the current root transform as the new "initial" for the compound
		this->initialPosition = rootNode->getPosition();
		this->initialScale = rootNode->getScale();
		this->initialOrientation = rootNode->getOrientation();

		// Clean existing bodies/collisions once
		this->destroyBody();
		this->destroyCollision();
		for (PhysicsActiveComponent* partComp : physicsComponentList)
		{
			if (nullptr == partComp || partComp == this)
			{
				continue;                          // never treat root as child
			}
			partComp->destroyBody();
			partComp->destroyCollision();
		}

		Ogre::Vector3 inertia(1.0f, 1.0f, 1.0f);
		Ogre::Vector3 cumMassOrigin = Ogre::Vector3::ZERO;
		Ogre::Real cumMass = this->getMass();
		std::vector<OgreNewt::CollisionPtr> compoundCollisionList;

		// Root (bed) collision
		Ogre::Quaternion collisionOrientation = Ogre::Quaternion::IDENTITY;
		if (Ogre::Vector3::ZERO != this->collisionDirection->getVector3())
		{
			collisionOrientation = MathHelper::getInstance()->degreesToQuat(this->collisionDirection->getVector3());
		}

		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("PhysicsActiveComponent::createDynamicCollisionForCompound", _4(&inertia, &compoundCollisionList, &collisionOrientation, &cumMassOrigin),
		{
			compoundCollisionList.emplace_back(this->createDynamicCollision(inertia, this->collisionSize->getVector3(), this->collisionPosition->getVector3(), collisionOrientation, cumMassOrigin, this->gameObjectPtr->getCategoryId()));
		});

		// Children (chairs etc.)
		for (PhysicsActiveComponent* partComp : physicsComponentList)
		{
			if (!partComp || partComp == this) continue;

			Ogre::SceneNode* partNode = partComp->getOwner()->getSceneNode();
			if (nullptr == partNode)
			{
				continue;
			}

			// world pose before parenting change
			const Ogre::Vector3 worldPos = partNode->_getDerivedPosition();
			const Ogre::Quaternion worldOri = partNode->_getDerivedOrientation();

			// local pose relative to bed
			const Ogre::Vector3 localPos = rootNode->convertWorldToLocalPosition(worldPos);
			const Ogre::Quaternion localOri = rootNode->convertWorldToLocalOrientation(worldOri);

			// reparent Root -> Bed
			if (Ogre::SceneNode* currentParent = partNode->getParentSceneNode())
			{
				if (currentParent != rootNode)
				{
					currentParent->removeChild(partNode);
				}
			}
			if (partNode->getParentSceneNode() != rootNode)
			{
				rootNode->addChild(partNode);
			}

			// keep world transform
			partNode->setPosition(localPos);
			partNode->setOrientation(localOri);
			partNode->setInheritScale(false);

			// mass / COM
			cumMassOrigin += (partComp->getOwner()->getMovableObject()->getLocalAabb().getSize() * partComp->getOwner()->getSceneNode()->getScale() * partComp->getMassOrigin());
			cumMass += partComp->getMass();

			Ogre::Vector3 massOrigin = partComp->getMassOrigin();
			compoundCollisionList.emplace_back(partComp->createDynamicCollision(inertia, partComp->getCollisionSize(), localPos, localOri, massOrigin, this->gameObjectPtr->getCategoryId()));
		}

		OgreNewt::CollisionPrimitives::CompoundCollision* col;

		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("PhysicsActiveComponent::createDynamicCollision", _2(&col, &compoundCollisionList),
		{
			col = new OgreNewt::CollisionPrimitives::CompoundCollision(this->ogreNewt, compoundCollisionList, this->gameObjectPtr->getCategoryId());
		});
		// Compound collision/body
		this->collisionPtr = OgreNewt::CollisionPtr(col);

		this->physicsBody = new OgreNewt::Body(this->ogreNewt, this->gameObjectPtr->getSceneManager(), this->collisionPtr);
		NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->registerRenderCallbackForBody(this->physicsBody);

		this->physicsBody->setGravity(this->gravity->getVector3());

		inertia *= cumMass;
		this->physicsBody->setMassMatrix(cumMass, inertia);
		this->physicsBody->setCenterOfMass(cumMassOrigin);

		this->physicsBody->setLinearDamping(this->linearDamping->getReal());
		this->physicsBody->setAngularDamping(this->angularDamping->getVector3());

		this->setConstraintAxis(this->getConstraintAxis());
		this->setConstraintDirection(this->getConstraintDirection());

		this->setActivated(this->activated->getBool());
		this->setContinuousCollision(this->continuousCollision->getBool());
		this->setGyroscopicTorqueEnabled(this->gyroscopicTorque->getBool());

		this->physicsBody->setType(this->gameObjectPtr->getCategoryId());
		this->physicsBody->setUserData(OgreNewt::Any(static_cast<PhysicsComponent*>(this)));
		this->physicsBody->setCustomForceAndTorqueCallback<PhysicsActiveComponent>(&PhysicsActiveComponent::moveCallback, this);

		this->physicsBody->attachNode(rootNode);

		this->setPosition(this->initialPosition);
		this->setOrientation(this->initialOrientation);

		const auto materialId = AppStateManager::getSingletonPtr()->getGameObjectController()->getMaterialID(this->gameObjectPtr.get(), this->ogreNewt);
		this->physicsBody->setMaterialGroupID(materialId);

		unsigned int i = 0;
		boost::shared_ptr<JointComponent> jointCompPtr = nullptr;
		do
		{
			jointCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponentWithOccurrence<JointComponent>(i));
			if (jointCompPtr)
			{
				jointCompPtr->setBody(this->physicsBody);
				++i;
			}
		} while (jointCompPtr);
	}

	void PhysicsActiveComponent::destroyCompoundBody(const std::vector<PhysicsActiveComponent*>& physicsComponentList)
	{
		Ogre::SceneNode* rootNode = this->gameObjectPtr->getSceneNode();        // Bed
		Ogre::SceneNode* rootParent = rootNode->getParentSceneNode();          // RootSceneNode

		// 1) reparent children back under Root, keeping their world transform
		for (PhysicsActiveComponent* partComp : physicsComponentList)
		{
			if (nullptr == partComp || partComp == this)
			{
				continue;
			}

			Ogre::SceneNode* partNode = partComp->getOwner()->getSceneNode();
			if (nullptr == partNode)
			{
				continue;
			}

			// world pose while still under Bed
			const Ogre::Vector3 worldPos = partNode->_getDerivedPosition();
			const Ogre::Quaternion worldOri = partNode->_getDerivedOrientation();

			// detach from Bed
			if (partNode->getParentSceneNode() == rootNode)
			{
				rootNode->removeChild(partNode);
			}

			// attach directly under Root
			if (nullptr != rootParent)
			{
				rootParent->addChild(partNode);
			}

			// IMPORTANT: just use worldPos/worldOri directly as local, assuming Root is identity
			partNode->setPosition(worldPos);
			partNode->setOrientation(worldOri);

			// kill compound body for child and recreate simple dynamic body if needed
			partComp->destroyBody();
			partComp->destroyCollision();
			partComp->createDynamicBody();
		}

		// 2) destroy compound body and recreate normal body for bed
		this->destroyBody();
		this->destroyCollision();
		this->createDynamicBody();
	}

	void PhysicsActiveComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);

		if (false == activated)
		{
			if (nullptr != this->physicsBody)
			{
				this->physicsBody->unFreeze();
				this->physicsBody->freeze();
				// this->physicsBody->setAutoSleep(1);
			}
		}
		else
		{
			if (nullptr != this->physicsBody)
			{
				this->physicsBody->unFreeze();
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

	Ogre::Real PhysicsActiveComponent::getCurrentGravityStrength(void) const
	{
		return this->currentGravityStrength;
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

	Ogre::Vector3 PhysicsActiveComponent::getGravityDirection(void)
	{
		// Ensures the latest updated gravity value is read
		while (false == gravityUpdated.test_and_set());  // Waits until a new update is available
		return gravityDirection;
	}

	void PhysicsActiveComponent::setOmegaVelocityRotateTo(const Ogre::Quaternion& resultOrientation, const Ogre::Vector3& axes, Ogre::Real strength)
	{
		if (nullptr == this->physicsBody)
		{
			return;
		}

		// Compute difference in orientation
		Ogre::Quaternion diffOrientation = this->physicsBody->getOrientation().Inverse() * resultOrientation;

		// Convert quaternion difference to angular velocity
		Ogre::Vector3 angularVelocity;
		Ogre::Degree angle;
		Ogre::Vector3 rotationAxis;

		diffOrientation.ToAngleAxis(angle, rotationAxis);

		// Ensure the rotation axis is valid (ToAngleAxis can return zero rotation)
		if (rotationAxis.isZeroLength())
			return;

		// Scale by strength
		angularVelocity = rotationAxis * angle.valueRadians() * strength;

		// Filter by axes
		angularVelocity.x *= axes.x;
		angularVelocity.y *= axes.y;
		angularVelocity.z *= axes.z;

		// Apply omega force
		this->setOmegaVelocity(angularVelocity);
	}

	void PhysicsActiveComponent::setOmegaVelocityRotateToDirection(const Ogre::Vector3& resultDirection, Ogre::Real strength)
	{
		if (nullptr == this->physicsBody)
		{
			return;
		}

		// Get the current forward direction from the physics body's orientation
		Ogre::Vector3 currentDir = this->physicsBody->getOrientation() * this->gameObjectPtr->getDefaultDirection();

		// Compute the rotation required to align currentDir with targetDir
		Ogre::Quaternion rotationQuat = currentDir.getRotationTo(resultDirection);

		// Convert quaternion rotation into angular velocity
		Ogre::Vector3 angularVelocity;
		Ogre::Degree angle;
		Ogre::Vector3 rotationAxis;

		rotationQuat.ToAngleAxis(angle, rotationAxis);

		// Ensure the rotation axis is valid (ToAngleAxis can return zero rotation)
		if (rotationAxis.isZeroLength())
		{
			return;
		}

		// Scale angular velocity by rotation strength
		angularVelocity = rotationAxis * angle.valueRadians() * strength;

		// Apply omega force to rotate towards the desired direction
		this->applyOmegaForce(angularVelocity);
	}

	void PhysicsActiveComponent::applyOmegaForce(const Ogre::Vector3& omegaForce)
	{
		// Set the command values
		this->omegaForceCommand.vectorValue = omegaForce;

		// Mark as pending - this will be seen by the physics thread
		this->omegaForceCommand.pending.store(true);
	}

	void PhysicsActiveComponent::applyOmegaForceRotateTo(const Ogre::Quaternion& resultOrientation, const Ogre::Vector3& axes, Ogre::Real strength)
	{
		if (nullptr == this->physicsBody)
		{
			return;
		}

		// Compute difference in orientation
		Ogre::Quaternion diffOrientation = this->physicsBody->getOrientation().Inverse() * resultOrientation;

		// Convert quaternion difference to angular velocity
		Ogre::Vector3 angularVelocity;
		Ogre::Degree angle;
		Ogre::Vector3 rotationAxis;

		diffOrientation.ToAngleAxis(angle, rotationAxis);

		// Ensure the rotation axis is valid (ToAngleAxis can return zero rotation)
		if (rotationAxis.isZeroLength())
			return;

		// Scale by strength
		angularVelocity = rotationAxis * angle.valueRadians() * strength;

		// Filter by axes
		angularVelocity.x *= axes.x;
		angularVelocity.y *= axes.y;
		angularVelocity.z *= axes.z;

		// Apply omega force
		this->applyOmegaForce(angularVelocity);
	}

	void PhysicsActiveComponent::applyOmegaForceRotateToDirection(const Ogre::Vector3& resultDirection, Ogre::Real strength)
	{
		if (nullptr == this->physicsBody)
		{
			return;
		}

		// Get the current forward direction from the physics body's orientation
		Ogre::Vector3 currentDir = this->physicsBody->getOrientation() * this->gameObjectPtr->getDefaultDirection();

		// Compute the rotation required to align currentDir with targetDir
		Ogre::Quaternion rotationQuat = currentDir.getRotationTo(resultDirection);

		// Convert quaternion rotation into angular velocity
		Ogre::Vector3 angularVelocity;
		Ogre::Degree angle;
		Ogre::Vector3 rotationAxis;

		rotationQuat.ToAngleAxis(angle, rotationAxis);

		// Ensure the rotation axis is valid (ToAngleAxis can return zero rotation)
		if (rotationAxis.isZeroLength())
		{
			return;
		}

		// Scale angular velocity by rotation strength
		angularVelocity = rotationAxis * angle.valueRadians() * strength;

		// Apply omega force to rotate towards the desired direction
		this->applyOmegaForce(angularVelocity);
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
		if (nullptr != this->physicsBody)
		{
			this->physicsBody->setVelocity(Ogre::Vector3::ZERO);
			this->physicsBody->setOmega(Ogre::Vector3::ZERO);
		}
	}

	void PhysicsActiveComponent::applyForce(const Ogre::Vector3& force)
	{
		if (force != Ogre::Vector3::ZERO)
		{
			// Set the command values
			this->forceCommand.vectorValue = force;

			// Mark as pending - this will be seen by the physics thread
			this->forceCommand.pending.store(true);
		}
	}

	void PhysicsActiveComponent::applyRequiredForceForVelocity(const Ogre::Vector3& velocity)
	{
		if (velocity != Ogre::Vector3::ZERO)
		{
			// Set the command values
			this->requiredVelocityForForceCommand.vectorValue = velocity;

			// Mark as pending - this will be seen by the physics thread
			this->requiredVelocityForForceCommand.pending.store(true);
		}
	}

	void PhysicsActiveComponent::applyRequiredForceForJumpVelocity(const Ogre::Vector3& velocity)
	{
		if (velocity != Ogre::Vector3::ZERO)
		{
			// Set the command values
			this->jumpForceCommand.vectorValue = velocity;

			// Mark as pending - this will be seen by the physics thread
			this->jumpForceCommand.pending.store(true);
		}
	}

	void PhysicsActiveComponent::applyDirectionForce(Ogre::Real speed)
	{
		Ogre::Vector3 currentDirection = this->gameObjectPtr->getOrientation() * this->gameObjectPtr->getDefaultDirection();
		this->applyRequiredForceForVelocity(currentDirection * speed);
	}

	Ogre::Vector3 PhysicsActiveComponent::getCurrentForceForVelocity(void) const
	{
		return this->requiredVelocityForForceCommand.vectorValue;
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
		{
			this->physicsBody->setGravity(gravity);
		}
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
		if (true == constraintDirection.isZeroLength())
		{
			if (nullptr != this->upVector)
			{
				this->releaseConstraintDirection();
			}
		}

		this->constraintDirection->setValue(constraintDirection);
		// Only set pin, if there is no constraint axis, because constraint axis also uses pin
		if (nullptr != this->physicsBody && Ogre::Vector3::ZERO != constraintDirection && nullptr == this->upVector)
		{
			this->upVector = new OgreNewt::UpVector(this->physicsBody, constraintDirection);
		}
		else if (nullptr != this->upVector)
		{
			this->upVector->setPin(this->physicsBody, constraintDirection);
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

		if (nullptr != this->physicsBody)
		{
			// this->physicsBody->setIsSoftBody(asSoftBody);
		}
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
		PhysicsComponent::actualizeValue(attribute);

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
		else if (PhysicsActiveComponent::AttrOnContactFunctionName() == attribute->getName())
		{
			this->setOnContactFunctionName(attribute->getString());
		}
	}

	void PhysicsActiveComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		GameObjectComponent::writeXML(propertiesXML, doc);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CollisionSize"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->collisionSize->getVector3())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OffsetPosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->collisionPosition->getVector3())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OffsetDirection"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->collisionDirection->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Mass"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->mass->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MassOrigin"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->massOrigin->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "LinearDamping"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->linearDamping->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AngularDamping"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->angularDamping->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Gravity"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->gravity->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "GravitySourceCategory"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->gravitySourceCategory->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ConstraintDirection"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->constraintDirection->getVector3())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ContinuousCollision"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->continuousCollision->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Speed"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->speed->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MinSpeed"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->minSpeed->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MaxSpeed"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxSpeed->getReal())));
		propertiesXML->append_node(propertyXML);

		/*propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DefaultPoseName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->defaultPoseName)));
		propertiesXML->append_node(propertyXML);*/

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CollisionType"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->collisionType->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ConstraintAxis"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->constraintAxis->getVector3())));
		propertiesXML->append_node(propertyXML);

		/*propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AsSoftBody"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->asSoftBody->getBool())));
		propertiesXML->append_node(propertyXML);*/

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "EnableGyroscopicTorque"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->gyroscopicTorque->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Collidable"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->collidable->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OnContactFunctionName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->onContactFunctionName->getString())));
		propertiesXML->append_node(propertyXML);
	}

	void PhysicsActiveComponent::showDebugData(void)
	{
		GameObjectComponent::showDebugData();
		if (nullptr != this->physicsBody)
		{
			ENQUEUE_RENDER_COMMAND_WAIT("PhysicsActiveComponent::showDebugData",
			{
				this->physicsBody->showDebugCollision(false, this->bShowDebugData);
			});
		}
	}

	PhysicsActiveComponent::ContactData PhysicsActiveComponent::getContactBelow(int index, const Ogre::Vector3& offset, bool forceDrawLine, unsigned int categoryIds, bool useLocalOrientation)
	{
		GameObject* gameObject = nullptr;
		Ogre::Real height = 500.0f; // Default invalid value for checking
		Ogre::Real slope = 0.0f;
		Ogre::Vector3 normal = Ogre::Vector3::ZERO;

		Ogre::Vector3 charPoint = Ogre::Vector3::ZERO;
		Ogre::Vector3 rayEndPoint = Ogre::Vector3::ZERO;
		Ogre::Vector3 targetDir = Ogre::Vector3::ZERO;

		if (false == useLocalOrientation)
		{
			targetDir = this->gravityDirection.isZeroLength() ? Ogre::Vector3::NEGATIVE_UNIT_Y : this->gravityDirection;
			charPoint = this->physicsBody->getPosition() + offset;
			rayEndPoint = charPoint + (targetDir * 500.0f);
		}
		else
		{
			// Compute the world-space position of the offset point (where the ray starts)
			charPoint = this->physicsBody->getPosition() + (this->physicsBody->getOrientation() * offset);

			// Compute the local downward direction, transformed into world space
			targetDir = this->physicsBody->getOrientation() * Ogre::Vector3::NEGATIVE_UNIT_Y;

			// Compute the end point of the ray by extending it downward
			rayEndPoint = charPoint + (targetDir * 500.0f); // 500 units below
		}

		// Create ray along the rotated downward direction
		OgreNewt::BasicRaycast ray(this->ogreNewt, charPoint, rayEndPoint, true);

		if (this->bShowDebugData || forceDrawLine)
		{
			// Get a key from id and user index to match a line when drawing is enabled
			Ogre::String key = std::to_string(this->gameObjectPtr->getId()) + std::to_string(index) + "getContactBelow";
			auto& it = this->drawLineMap.find(key);
			if (it == this->drawLineMap.cend())
			{
				ENQUEUE_RENDER_COMMAND_MULTI_WAIT("PhysicsActiveComponent::getContactBelow", _3(key, charPoint, rayEndPoint),
				{
					Ogre::SceneNode* debugLineNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();
					debugLineNode->setName("getContactBelow");
					Ogre::ManualObject * debugLineObject = this->gameObjectPtr->getSceneManager()->createManualObject();
					debugLineObject->setQueryFlags(0 << 0);
					debugLineObject->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);
					debugLineObject->setCastShadows(false);
					debugLineNode->attachObject(debugLineObject);
					this->drawLineMap.emplace(key, std::make_pair(debugLineNode, debugLineObject));
					debugLineObject->clear();
					debugLineObject->begin("RedNoLighting", Ogre::OperationType::OT_LINE_LIST);
					debugLineObject->position(charPoint);
					debugLineObject->index(0);
					debugLineObject->position(rayEndPoint);
					debugLineObject->index(1);
					debugLineObject->end();
				});
			}
			else
			{
				// ENQUEUE_RENDER_COMMAND_MULTI("PhysicsActiveComponent::getContactBelow::Draw", _3(it, charPoint, rayEndPoint),
				// {
				auto closureFunction = [this, it, charPoint, rayEndPoint](Ogre::Real weight)
				{
					Ogre::ManualObject* debugLineObject = it->second.second;
					debugLineObject->clear();
					debugLineObject->begin("RedNoLighting", Ogre::OperationType::OT_LINE_LIST);
					debugLineObject->position(charPoint);
					debugLineObject->index(0);
					debugLineObject->position(rayEndPoint);
					debugLineObject->index(1);
					debugLineObject->end();
				};
				Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::drawLineMap" + Ogre::StringConverter::toString(this->index) + "_" + it->second.first->getName();
				NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);
				// });
			}
		}

		OgreNewt::BasicRaycast::BasicRaycastInfo info = ray.getFirstHit();
		if (info.mBody)
		{
			unsigned int type = info.mBody->getType();
			unsigned int finalType = type & categoryIds;
			
			if (type == finalType)
			{
				Ogre::String name = info.mBody->getOgreNode()->getName();
				// Scale distance according to the ray length
				height = info.mDistance * 500.0f;

				// Compute the angle between gravity direction and the surface normal
				normal = info.mNormal;
				slope = Ogre::Math::ACos(-targetDir.dotProduct(normal) / (targetDir.length() * normal.length())).valueDegrees();

				try
				{
					// Get associated GameObject
					Ogre::SceneNode* tempNode = static_cast<Ogre::SceneNode*>(info.mBody->getOgreNode());
					if (tempNode)
					{
						gameObject = Ogre::any_cast<GameObject*>(tempNode->getUserObjectBindings().getUserAny());
					}
					else
					{
						return PhysicsActiveComponent::ContactData(nullptr, height, normal, slope);
					}
				}
				catch (...)
				{
					return PhysicsActiveComponent::ContactData(nullptr, height, normal, slope);
				}
			}
		}
		return PhysicsActiveComponent::ContactData(gameObject, height, normal, slope);
	}

	PhysicsActiveComponent::ContactData PhysicsActiveComponent::getContactAhead(int index, const Ogre::Vector3& offset, Ogre::Real length, bool forceDrawLine, unsigned int categoryIds)
	{
		GameObject* gameObject = nullptr;
		Ogre::Real height = 500.0f;
		Ogre::Real slope = 0.0f;
		Ogre::Vector3 normal = Ogre::Vector3::UNIT_SCALE * 100.0f;
		Ogre::Vector3 rayEndPoint = Ogre::Vector3::ZERO;
		Ogre::Vector3 targetDir = Ogre::Vector3::ZERO;

		// Get the default direction (mesh forward direction)
		Ogre::Vector3 defaultDir = this->gameObjectPtr->getDefaultDirection();

		// Create a rotation from the Z-axis to the default direction
		Ogre::Quaternion defaultRot = Ogre::Vector3::UNIT_Z.getRotationTo(defaultDir);

		// Apply both the default rotation and the body's orientation to the offset
		Ogre::Vector3 charPoint = this->physicsBody->getPosition() +
			(this->physicsBody->getOrientation() * defaultRot * offset);

		// Get the forward direction in world space
		Ogre::Vector3 forwardDir = this->physicsBody->getOrientation() * defaultDir;

		// If we have a gravity direction, we need to adjust our "ahead" direction to be perpendicular to gravity
		if (false == this->gravityDirection.isZeroLength())
		{
			// Project the forward direction onto the plane perpendicular to gravity
			// by removing the component of forward that's parallel to gravity
			Ogre::Real dot = forwardDir.dotProduct(this->gravityDirection);
			forwardDir = forwardDir - (this->gravityDirection * dot);

			// If after projection we have a zero vector (rare case where forward was exactly parallel to gravity)
			// we need a fallback direction
			if (true == forwardDir.isZeroLength())
			{
				// Create an arbitrary perpendicular vector to gravity
				if (this->gravityDirection != Ogre::Vector3::UNIT_X)
				{
					forwardDir = this->gravityDirection.crossProduct(Ogre::Vector3::UNIT_X);
				}
				else
				{
					forwardDir = this->gravityDirection.crossProduct(Ogre::Vector3::UNIT_Y);
				}
			}

			// Normalize to ensure we have a unit direction
			forwardDir.normalise();
		}

		// Compute the end point of the ray by extending it in the forward direction
		rayEndPoint = charPoint + (forwardDir * length);

		// Create ray along the adjusted forward direction
		OgreNewt::BasicRaycast ray(this->ogreNewt, charPoint, rayEndPoint, true);
		
		if (true == this->bShowDebugData || true == forceDrawLine)
		{
			// Get a key from id and user index to match a line, when line drawing is set to on
			Ogre::String key = std::to_string(this->gameObjectPtr->getId()) + std::to_string(index) + "getContactAhead";
			auto& it = this->drawLineMap.find(key);
			if (it == this->drawLineMap.cend())
			{
				ENQUEUE_RENDER_COMMAND_MULTI_WAIT("PhysicsActiveComponent::getContactAhead", _3(key, charPoint, rayEndPoint),
				{
					Ogre::SceneNode * debugLineNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();
					debugLineNode->setName("getContactAhead");
					Ogre::ManualObject * debugLineObject = this->gameObjectPtr->getSceneManager()->createManualObject();
					debugLineObject->setQueryFlags(0 << 0);
					debugLineObject->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);
					debugLineObject->setCastShadows(false);
					debugLineNode->attachObject(debugLineObject);
					this->drawLineMap.emplace(key, std::make_pair(debugLineNode, debugLineObject));
					debugLineObject->clear();
					debugLineObject->begin("RedNoLighting", Ogre::OperationType::OT_LINE_LIST);
					debugLineObject->position(charPoint);
					debugLineObject->index(0);
					debugLineObject->position(rayEndPoint);
					debugLineObject->index(1);
					debugLineObject->end();
				});
			}
			else
			{
				// ENQUEUE_RENDER_COMMAND_MULTI("PhysicsActiveComponent::getContactAhead::Draw", _3(it, charPoint, rayEndPoint),
				// {
				auto closureFunction = [this, it, charPoint, rayEndPoint](Ogre::Real weight)
					{
						Ogre::ManualObject* debugLineObject = it->second.second;
						debugLineObject->clear();
						debugLineObject->begin("RedNoLighting", Ogre::OperationType::OT_LINE_LIST);
						debugLineObject->position(charPoint);
						debugLineObject->index(0);
						debugLineObject->position(rayEndPoint);
						debugLineObject->index(1);
						debugLineObject->end();
					};
				Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::drawLineMap" + Ogre::StringConverter::toString(this->index) + "_" + it->second.first->getName();
				NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);
				// });
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
				//Die Distanz ist relativ zur Länge des Raystrahls
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

	PhysicsActiveComponent::ContactData PhysicsActiveComponent::getContactAbove(int index, const Ogre::Vector3& offset, bool forceDrawLine, unsigned int categoryIds, bool useLocalOrientation)
	{
		GameObject* gameObject = nullptr;
		Ogre::Real height = 500.0f; // Default invalid value for checking
		Ogre::Real slope = 0.0f;
		Ogre::Vector3 normal = Ogre::Vector3::ZERO;

		Ogre::Vector3 charPoint = Ogre::Vector3::ZERO;
		Ogre::Vector3 rayEndPoint = Ogre::Vector3::ZERO;
		Ogre::Vector3 targetDir = Ogre::Vector3::ZERO;

		if (false == useLocalOrientation)
		{
			targetDir = this->gravityDirection.isZeroLength() ? Ogre::Vector3::UNIT_Y : -this->gravityDirection;
			charPoint = this->physicsBody->getPosition() + (this->physicsBody->getOrientation() * offset);
			rayEndPoint = charPoint + (targetDir * 500.0f);
		}
		else
		{
			// Compute the world-space position of the offset point (where the ray starts)
			charPoint = this->physicsBody->getPosition() + (this->physicsBody->getOrientation() * offset);

			// Compute the local downward direction, transformed into world space
			targetDir = this->physicsBody->getOrientation() * Ogre::Vector3::UNIT_Y;

			// Compute the end point of the ray by extending it downward
			rayEndPoint = charPoint + (targetDir * 500.0f); // 500 units below
		}

		// Create ray along the rotated downward direction
		OgreNewt::BasicRaycast ray(this->ogreNewt, charPoint, rayEndPoint, true);

		// Debug drawing
		if (this->bShowDebugData || forceDrawLine)
		{
			Ogre::Vector3 fromPosition = charPoint;
			Ogre::Vector3 toPosition = rayEndPoint;

			Ogre::String key = std::to_string(this->gameObjectPtr->getId()) + std::to_string(index) + "getContactAbove";
			auto& it = this->drawLineMap.find(key);
			if (it == this->drawLineMap.cend())
			{
				ENQUEUE_RENDER_COMMAND_MULTI_WAIT("PhysicsActiveComponent::getContactAbove", _3(key, fromPosition, toPosition),
				{
					Ogre::SceneNode * debugLineNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();
					debugLineNode->setName("getContactAbove");
					Ogre::ManualObject * debugLineObject = this->gameObjectPtr->getSceneManager()->createManualObject();
					debugLineObject->setQueryFlags(0 << 0);
					debugLineObject->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);
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
				});
			}
			else
			{
				// ENQUEUE_RENDER_COMMAND_MULTI("PhysicsActiveComponent::getContactAbove::Draw", _3(it, charPoint, rayEndPoint),
				// {
				auto closureFunction = [this, it, charPoint, rayEndPoint](Ogre::Real weight)
				{
					Ogre::ManualObject* debugLineObject = it->second.second;
					debugLineObject->clear();
					debugLineObject->begin("RedNoLighting", Ogre::OperationType::OT_LINE_LIST);
					debugLineObject->position(charPoint);
					debugLineObject->index(0);
					debugLineObject->position(rayEndPoint);
					debugLineObject->index(1);
					debugLineObject->end();
				};
				Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::drawLineMap" + Ogre::StringConverter::toString(this->index) + "_" + it->second.first->getName();
				NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);
				// });
			}
		}

		OgreNewt::BasicRaycast::BasicRaycastInfo info = ray.getFirstHit();
		if (info.mBody)
		{
			unsigned int type = info.mBody->getType();
			unsigned int finalType = type & categoryIds;
			if (type == finalType)
			{
				// Convert distance to world units
				height = info.mDistance * 500.0f;

				// Compute slope using gravity direction
				normal = info.mNormal;
				slope = Ogre::Math::ACos(-targetDir.dotProduct(normal) / (targetDir.length() * normal.length())).valueDegrees();

				try
				{
					Ogre::SceneNode* tempNode = static_cast<Ogre::SceneNode*>(info.mBody->getOgreNode());
					if (tempNode)
					{
						gameObject = Ogre::any_cast<GameObject*>(tempNode->getUserObjectBindings().getUserAny());
					}
					else
					{
						return PhysicsActiveComponent::ContactData(nullptr, height, normal, slope);
					}
				}
				catch (...)
				{
					return PhysicsActiveComponent::ContactData(nullptr, height, normal, slope);
				}
			}
		}

		return PhysicsActiveComponent::ContactData(gameObject, height, normal, slope);
	}

	PhysicsActiveComponent::ContactData PhysicsActiveComponent::getContactToDirection(int index, const Ogre::Vector3& direction, const Ogre::Vector3& offset,
		Ogre::Real from, Ogre::Real to, bool forceDrawLine, unsigned int categoryIds)
	{
		GameObject* gameObject = nullptr;
		Ogre::Real height = 500.0f;
		Ogre::Real slope = 0.0f;
		Ogre::Vector3 normal = Ogre::Vector3::UNIT_SCALE * 100.0f;

		// Get the default direction (mesh forward direction)
		Ogre::Vector3 defaultDir = this->gameObjectPtr->getDefaultDirection();

		// Create a rotation from the Z-axis to the default direction
		Ogre::Quaternion defaultRot = Ogre::Vector3::UNIT_Z.getRotationTo(defaultDir);

		// Apply both the default rotation and the body's orientation to the offset
		Ogre::Vector3 position = this->physicsBody->getPosition() +
			(this->physicsBody->getOrientation() * defaultRot * offset);

		// Adjust the direction based on gravity if needed
		Ogre::Vector3 adjustedDirection = direction;
		if (false == this->gravityDirection.isZeroLength())
		{
			// Project the direction onto the plane perpendicular to gravity
			Ogre::Real dot = direction.dotProduct(this->gravityDirection);
			adjustedDirection = direction - (this->gravityDirection * dot);

			// If after projection we have a zero vector, we need a fallback direction
			if (true == adjustedDirection.isZeroLength())
			{
				// Create an arbitrary perpendicular vector to gravity
				if (this->gravityDirection != Ogre::Vector3::UNIT_X)
				{
					adjustedDirection = this->gravityDirection.crossProduct(Ogre::Vector3::UNIT_X);
				}
				else
				{
					adjustedDirection = this->gravityDirection.crossProduct(Ogre::Vector3::UNIT_Y);
				}
			}

			// Normalize to ensure we have a unit direction
			adjustedDirection.normalise();
		}

		// Calculate the start and end positions for the ray
		Ogre::Vector3 fromPosition = position + (adjustedDirection * from);
		Ogre::Vector3 toPosition = position + (adjustedDirection * to);

		// Create ray
		OgreNewt::BasicRaycast ray(this->ogreNewt, fromPosition, toPosition, true);

		if (true == this->bShowDebugData || true == forceDrawLine)
		{
			// Get a key from id and user index to match a line, when line drawing is set to on
			Ogre::String key = std::to_string(this->gameObjectPtr->getId()) + std::to_string(index) + "getContactToDirection";
			auto& it = this->drawLineMap.find(key);
			if (it == this->drawLineMap.cend())
			{
				ENQUEUE_RENDER_COMMAND_MULTI_WAIT("PhysicsActiveComponent::getContactToDirection", _3(key, fromPosition, toPosition),
				{
					Ogre::SceneNode * debugLineNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();
					debugLineNode->setName("getContactToDirection");
					Ogre::ManualObject * debugLineObject = this->gameObjectPtr->getSceneManager()->createManualObject();
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
				});
			}
			else
			{
				// ENQUEUE_RENDER_COMMAND_MULTI("PhysicsActiveComponent::getContactToDirection::Draw", _3(it, charPoint, rayEndPoint),
				// {
				auto closureFunction = [this, it, fromPosition, toPosition](Ogre::Real weight)
					{
						Ogre::ManualObject* debugLineObject = it->second.second;
						debugLineObject->clear();
						debugLineObject->begin("RedNoLighting", Ogre::OperationType::OT_LINE_LIST);
						debugLineObject->position(fromPosition);
						debugLineObject->index(0);
						debugLineObject->position(toPosition);
						debugLineObject->index(1);
						debugLineObject->end();
					};
				Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::drawLineMap" + Ogre::StringConverter::toString(this->index) + "_" + it->second.first->getName();
				NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);
				// });
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
				//Die Distanz ist relativ zur Länge des Raystrahls
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
		Ogre::Real from, Ogre::Real to, bool forceDrawLine, unsigned int categoryIds)
	{
		GameObject* gameObject = nullptr;
		Ogre::Real height = 500.0f;
		Ogre::Real slope = 0.0f;
		Ogre::Vector3 normal = Ogre::Vector3::UNIT_SCALE * 100.0f;

		// Get the default direction (mesh forward direction)
		Ogre::Vector3 defaultDir = this->gameObjectPtr->getDefaultDirection();

		// Create a rotation from the Z-axis to the default direction
		Ogre::Quaternion defaultRot = Ogre::Vector3::UNIT_Z.getRotationTo(defaultDir);

		// Apply both the default rotation and the body's orientation to the offset
		Ogre::Vector3 position = this->physicsBody->getPosition() +
			(this->physicsBody->getOrientation() * defaultRot * offset);

		// Adjust the direction based on gravity if needed
		Ogre::Vector3 adjustedDirection = direction;
		if (false == this->gravityDirection.isZeroLength())
		{
			// Project the direction onto the plane perpendicular to gravity
			Ogre::Real dot = direction.dotProduct(this->gravityDirection);
			adjustedDirection = direction - (this->gravityDirection * dot);

			// If after projection we have a zero vector, we need a fallback direction
			if (true == adjustedDirection.isZeroLength())
			{
				// Create an arbitrary perpendicular vector to gravity
				if (this->gravityDirection != Ogre::Vector3::UNIT_X)
				{
					adjustedDirection = this->gravityDirection.crossProduct(Ogre::Vector3::UNIT_X);
				}
				else
				{
					adjustedDirection = this->gravityDirection.crossProduct(Ogre::Vector3::UNIT_Y);
				}
			}

			// Normalize to ensure we have a unit direction
			adjustedDirection.normalise();
		}

		// Calculate the start and end positions for the ray
		Ogre::Vector3 fromPosition = position + (adjustedDirection * from);
		Ogre::Vector3 toPosition = position + (adjustedDirection * to);

		// Create ray
		OgreNewt::BasicRaycast ray(this->ogreNewt, fromPosition, toPosition, true);

		if (true == forceDrawLine)
		{
			// Get a key from id and user index to match a line, when line drawing is set to on
			Ogre::String key = std::to_string(this->gameObjectPtr->getId()) + std::to_string(index) + "getContact";
			auto& it = this->drawLineMap.find(key);
			if (it == this->drawLineMap.cend())
			{
				ENQUEUE_RENDER_COMMAND_MULTI_WAIT("PhysicsActiveComponent::getContact", _1(key),
				{
					Ogre::SceneNode * debugLineNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();
					debugLineNode->setName("getContact");
					Ogre::ManualObject * debugLineObject = this->gameObjectPtr->getSceneManager()->createManualObject();
					debugLineObject->setQueryFlags(0 << 0);
					debugLineObject->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);
					debugLineObject->setCastShadows(false);
					debugLineNode->attachObject(debugLineObject);
					this->drawLineMap.emplace(key, std::make_pair(debugLineNode, debugLineObject));
				});
			}
			else
			{
				// ENQUEUE_RENDER_COMMAND_MULTI("PhysicsActiveComponent::getContact::Draw", _3(it, charPoint, rayEndPoint),
				// {
				auto closureFunction = [this, it, fromPosition, toPosition](Ogre::Real weight)
				{
					Ogre::ManualObject* debugLineObject = it->second.second;
					debugLineObject->clear();
					debugLineObject->begin("RedNoLighting", Ogre::OperationType::OT_LINE_LIST);
					debugLineObject->position(fromPosition);
					debugLineObject->index(0);
					debugLineObject->position(toPosition);
					debugLineObject->index(1);
					debugLineObject->end();
				};
				Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::drawLineMap" + Ogre::StringConverter::toString(this->index) + "_" + it->second.first->getName();
				NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);
				// });
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
		// Get the default direction (mesh forward direction)
		Ogre::Vector3 defaultDir = this->gameObjectPtr->getDefaultDirection();

		// Create a rotation from the Z-axis to the default direction
		Ogre::Quaternion defaultRot = Ogre::Vector3::UNIT_Z.getRotationTo(defaultDir);

		// Apply both the default rotation and the body's orientation to the offset
		Ogre::Vector3 position = this->physicsBody->getPosition() +
			(this->physicsBody->getOrientation() * defaultRot * offset);

		// Adjust the direction based on gravity if needed
		Ogre::Vector3 adjustedDirection = direction;
		if (false == this->gravityDirection.isZeroLength())
		{
			// Project the direction onto the plane perpendicular to gravity
			Ogre::Real dot = direction.dotProduct(this->gravityDirection);
			adjustedDirection = direction - (this->gravityDirection * dot);

			// If after projection we have a zero vector, we need a fallback direction
			if (true == adjustedDirection.isZeroLength())
			{
				// Create an arbitrary perpendicular vector to gravity
				if (this->gravityDirection != Ogre::Vector3::UNIT_X)
				{
					adjustedDirection = this->gravityDirection.crossProduct(Ogre::Vector3::UNIT_X);
				}
				else
				{
					adjustedDirection = this->gravityDirection.crossProduct(Ogre::Vector3::UNIT_Y);
				}
			}

			// Normalize to ensure we have a unit direction
			adjustedDirection.normalise();
		}

		// Calculate the start and end positions for the ray
		Ogre::Vector3 fromPosition = position + adjustedDirection;
		Ogre::Vector3 toPosition = position + (adjustedDirection * scale);

		// Create ray
		OgreNewt::BasicRaycast ray(this->ogreNewt, fromPosition, toPosition, true);

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

		// Get the default direction (mesh forward direction)
		Ogre::Vector3 defaultDir = this->gameObjectPtr->getDefaultDirection();

		// Create a rotation from the Z-axis to the default direction
		Ogre::Quaternion defaultRot = Ogre::Vector3::UNIT_Z.getRotationTo(defaultDir);

		// Apply both the default rotation and the body's orientation to the positionOffset1
		Ogre::Vector3 charPoint = this->physicsBody->getPosition() +
			(this->physicsBody->getOrientation() * defaultRot * positionOffset1);

		// Compute the local downward direction, transformed into world space
		Ogre::Vector3 localDown = this->physicsBody->getOrientation() * Ogre::Vector3::NEGATIVE_UNIT_Y;

		// Compute the end point of the ray by extending it downward
		Ogre::Vector3 rayEndPoint = charPoint + (localDown * 500.0f); // 500 units below

		// Create ray along the rotated downward direction
		OgreNewt::BasicRaycast ray(this->ogreNewt, charPoint, rayEndPoint, true);

		OgreNewt::BasicRaycast::BasicRaycastInfo& info = ray.getFirstHit();
		if (info.mBody)
		{
			unsigned int type = info.mBody->getType();
			unsigned int finalType = type & categoryIds;
			if (type == finalType)
			{
				height = info.mDistance * 500.0f;

				Ogre::Vector3 charPoint = this->physicsBody->getPosition() +
					(this->physicsBody->getOrientation() * defaultRot * positionOffset2);

				ray = OgreNewt::BasicRaycast(this->ogreNewt, charPoint, charPoint + rayEndPoint, true);
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
		// Take ownership immediately in a shared_ptr
		std::shared_ptr<IForceObserver> holder(forceObserver);

		std::unique_lock<std::shared_mutex> lk(this->forceObserversMutex);
		auto it = forceObservers.find(holder->getName());
		if (it == forceObservers.end())
		{
			forceObservers.emplace(holder->getName(), std::move(holder));
		}
	}

	void PhysicsActiveComponent::detachAndDestroyForceObserver(const Ogre::String& name)
	{
		std::unique_lock<std::shared_mutex> lk(this->forceObserversMutex);
		auto it = forceObservers.find(name);
		if (it != forceObservers.end())
		{
			// Make it inert for any in-flight calls
			it->second->deactivate();
			// Erase container reference. Real destruction happens after any
			// in-flight physics-thread copies (shared_ptr) go out of scope.
			forceObservers.erase(it);
		}
	}

	void PhysicsActiveComponent::detachAndDestroyAllForceObserver(void)
	{
		std::unique_lock<std::shared_mutex> lk(forceObserversMutex);
		for (auto& kv : forceObservers)
		{
			kv.second->deactivate();
		}
		forceObservers.clear(); // shared_ptr handles lifetime
	}

	void PhysicsActiveComponent::setContactSolvingEnabled(bool enable)
	{
		if (nullptr == this->gameObjectPtr || nullptr == this->gameObjectPtr->getLuaScript() || true == this->onContactFunctionName->getString().empty() || nullptr == this->physicsBody)
		{
			return;
		}
		if (true == enable)
		{
			this->physicsBody->setContactCallback<PhysicsActiveComponent>(&PhysicsActiveComponent::contactCallback, this);
		}
		else
		{
			this->physicsBody->removeContactCallback();
		}
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

	void PhysicsActiveComponent::setOnContactFunctionName(const Ogre::String& onContactFunctionName)
	{
		this->onContactFunctionName->setValue(onContactFunctionName);
		if (false == onContactFunctionName.empty())
		{
			this->onContactFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), onContactFunctionName + "(otherGameObject, contact)");
			this->setContactSolvingEnabled(true);
		}
		else
		{
			this->setContactSolvingEnabled(false);
		}
	}

	void PhysicsActiveComponent::destroyLineMap(void)
	{
		for (auto& it = this->drawLineMap.cbegin(); it != this->drawLineMap.cend(); ++it)
		{
			Ogre::SceneNode* debugLineNode = it->second.first;

			Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::drawLineMap" + Ogre::StringConverter::toString(this->index) + "_" + debugLineNode->getName();
			NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

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

	Ogre::Vector3 PhysicsActiveComponent::getUp(void) const
	{
		return this->up;
	}

	Ogre::Vector3 PhysicsActiveComponent::getRight(void) const
	{
		return this->right;
	}

	Ogre::Vector3 PhysicsActiveComponent::getForward(void) const
	{
		return this->forward;
	}

	void PhysicsActiveComponent::moveCallback(OgreNewt::Body* body, Ogre::Real timeStep, int threadIndex)
	{
		// This moveCallback is called in the physics thread!

		/////////////////////Standard gravity force/////////////////////////////////////
		Ogre::Vector3 wholeForce = body->getGravity();
		Ogre::Real mass = 0.0f;
		Ogre::Vector3 inertia = Ogre::Vector3::ZERO;
		this->gravityDirection = Ogre::Vector3::NEGATIVE_UNIT_Y;

		Ogre::Real nearestPlanetDistance = std::numeric_limits<Ogre::Real>::max();
		GameObjectPtr nearestGravitySourceObject;

		// calculate gravity
		body->getMassMatrix(mass, inertia);

		if (false == this->hasAttraction)
		{
			wholeForce *= mass;
		}

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
					Ogre::Vector3 directionToPlanet = this->getPosition() - gravitySourcePhysicsComponentPtr->getPosition();
					directionToPlanet.normalise();

					// Ensures constant acceleration of e.g. -19.8 m/s²
					Ogre::Real gravityAcceleration = -this->gravity->getVector3().length(); // Should be e.g. 19.8

					this->gravityDirection = -directionToPlanet;
					wholeForce = directionToPlanet * (mass * gravityAcceleration); // F = m * a

					// Store the current gravity strength for jump normalization
					this->currentGravityStrength = gravityAcceleration;
					// Mark gravity as updated
					this->gravityUpdated.test_and_set();

					// this->setConstraintDirection(this->gravityDirection);

					// More realistic planetary scenario: Depending on planet size, gravity is much harder or slower like on the moon:
					// Ogre::Real squaredDistanceToGravitySource = wholeForce.squaredLength();
					// Ogre::Real strength = (gravityAcceleration * gravitySourcePhysicsComponentPtr->getMass()) / squaredDistanceToGravitySource;
					// wholeForce = directionToPlanet * (mass * strength);
				}
			}
			else
			{
				this->gravityDirection = Ogre::Vector3::NEGATIVE_UNIT_Y;
				this->currentGravityStrength = 0.0f;

				this->gravityUpdated.clear();
			}
		}

		// Checks if a force command is pending
		if (this->forceCommand.pending.load())
		{
			// Try to claim this command
			bool expected = false;
			if (this->forceCommand.inProgress.compare_exchange_strong(expected, true))
			{
				Ogre::Vector3 forceToApply = this->forceCommand.vectorValue;
				body->addForce(forceToApply);

				// Mark command as no longer pending
				this->forceCommand.pending.store(false);

				// Release the command
				this->forceCommand.inProgress.store(false);
			}
		}

		// Checks if a required force for velocity command is pending
		if (this->requiredVelocityForForceCommand.pending.load())
		{
			// Try to claim this command
			bool expected = false;
			if (this->requiredVelocityForForceCommand.inProgress.compare_exchange_strong(expected, true))
			{
				// We've claimed the command

				// Get the velocity to apply
				Ogre::Vector3 velocityToApply = this->requiredVelocityForForceCommand.vectorValue;

				// Calculate and apply the force
				Ogre::Vector3 moveForce = (velocityToApply - body->getVelocity()) * mass / timeStep;
				body->addForce(moveForce);

				// Mark command as no longer pending
				this->requiredVelocityForForceCommand.pending.store(false);

				// Release the command
				this->requiredVelocityForForceCommand.inProgress.store(false);
			}
		}

		// Checks if a jump force command is pending
		if (this->jumpForceCommand.pending.load())
		{
			// Try to claim this command
			bool expected = false;
			if (this->jumpForceCommand.inProgress.compare_exchange_strong(expected, true))
			{
				// We've claimed the command

				// Get the velocity to apply
				Ogre::Vector3 velocityToApply = this->jumpForceCommand.vectorValue;

				// Calculate and apply the force
				Ogre::Vector3 moveForce = (velocityToApply - body->getVelocity()) * mass / timeStep;
				body->addForce(moveForce);

				// Mark command as no longer pending
				this->jumpForceCommand.pending.store(false);

				// Release the command
				this->jumpForceCommand.inProgress.store(false);
			}
		}

		// Checks if an omage force command is pending
		if (this->omegaForceCommand.pending.load())
		{
			// Try to claim this command
			bool expected = false;
			if (this->omegaForceCommand.inProgress.compare_exchange_strong(expected, true))
			{
				// Get the omega force to apply
				Ogre::Vector3 omegaForceToApply = this->omegaForceCommand.vectorValue;

				body->setBodyAngularVelocity(omegaForceToApply, timeStep);

				// Mark command as no longer pending
				this->omegaForceCommand.pending.store(false);

				// Release the command
				this->omegaForceCommand.inProgress.store(false);
			}
		}

		/////////////////Force observer//////////////////////////////////////////

		std::vector<std::shared_ptr<IForceObserver>> local;
		{
			std::shared_lock<std::shared_mutex> lk(forceObserversMutex);
			local.reserve(forceObservers.size());
			for (auto& kv : forceObservers)
			{
				local.push_back(kv.second);
			}
		}

		for (auto& obs : local)
		{
			obs->onForceAdd(body, timeStep, threadIndex);
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

			// Causes threading issues, if its required, use atomic bool
			// if (this->gameObjectPtr->getSceneNode()->getAttachedObject(0)->isVisible())
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
					// body->addForce(attractorDirection);
					wholeForce += attractorDirection;
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
				// body->addForce(dragForce);
				wholeForce += dragForce;
			}
		}

		body->addForce(wholeForce);
	}

	void PhysicsActiveComponent::contactCallback(OgreNewt::Body* otherBody, OgreNewt::Contact* contact)
	{
		PhysicsComponent* otherPhysicsComponent = OgreNewt::any_cast<PhysicsComponent*>(otherBody->getUserData());
		
		if (nullptr != this->gameObjectPtr->getLuaScript())
		{
			NOWA::AppStateManager::LogicCommand logicCommand = [this, otherPhysicsComponent, contact]()
			{
				this->gameObjectPtr->getLuaScript()->callTableFunction(this->onContactFunctionName->getString(), otherPhysicsComponent->getOwner(), contact);
			};
			NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
		}
	}

	void PhysicsActiveComponent::updateCallback(OgreNewt::Body* body)
	{
		/*if (true == this->bConnected)
		{
			double dt = (static_cast<double>(this->timer.getMilliseconds()) * 0.001) - this->lastTime;
			this->lastTime += dt;

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "Physics dt: " + Ogre::StringConverter::toString(dt));
		}*/


		// Does not work, because its called to often, do not know, how to solve this, because OgreNewt::update is called from appstate manager in appstate in update function and called to often
		//if (nullptr != this->gameObjectPtr->getLuaScript())
		//{
		//	// call the fixed physics update
		//	this->gameObjectPtr->getLuaScript()->callTableFunction("physicsUpdate");
		//}
	}

}; // namespace end
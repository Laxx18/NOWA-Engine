#include "NOWAPrecompiled.h"
#include "PhysicsActiveKinematicComponent.h"
#include "PhysicsComponent.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "tinyxml.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	PhysicsActiveKinematicComponent::PhysicsActiveKinematicComponent()
		: PhysicsActiveComponent()
	{
		this->asSoftBody->setVisible(false);
		this->gyroscopicTorque->setValue(false);
		this->gyroscopicTorque->setVisible(false);
		this->gravity->setValue(Ogre::Vector3::ZERO);
		this->gravity->setVisible(false);
		this->force->setVisible(false);
		this->gravitySourceCategory->setVisible(false);
		this->constraintDirection->setValue(Ogre::Vector3::ZERO);
		this->constraintDirection->setVisible(false);

		this->onKinematicContactFunctionName = new Variant(PhysicsActiveKinematicComponent::AttrOnKinematicContactFunctionName(), Ogre::String(""), this->attributes);

		this->onKinematicContactFunctionName->setDescription("Sets the function name to react in lua script at the moment when another game object collided with this kinematic game object. "
															 "It can also be used with (setCollidable(false)), so that ghost collision can be detected. E.g. onKinematicContact(otherGameObject).");
		this->onKinematicContactFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), this->onKinematicContactFunctionName->getString() + "(otherGameObject)");
	}

	PhysicsActiveKinematicComponent::~PhysicsActiveKinematicComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveKinematicComponent] Destructor physics active kinematic component for game object: " 
			+ this->gameObjectPtr->getName());
	}

	bool PhysicsActiveKinematicComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		PhysicsActiveComponent::parseCommonProperties(propertyElement, filename);

		this->constraintDirection->setValue(Ogre::Vector3::ZERO);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OnKinematicContactFunctionName")
		{
			this->onKinematicContactFunctionName->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		return true;
	}

	GameObjectCompPtr PhysicsActiveKinematicComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		PhysicsActiveKinematicCompPtr clonedCompPtr(boost::make_shared<PhysicsActiveKinematicComponent>());

		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setMass(this->mass->getReal());
		clonedCompPtr->setMassOrigin(this->massOrigin->getVector3());
		clonedCompPtr->setLinearDamping(this->linearDamping->getReal());
		clonedCompPtr->setAngularDamping(this->angularDamping->getVector3());
		clonedCompPtr->setGravity(this->gravity->getVector3());
		clonedCompPtr->setGravitySourceCategory(this->gravitySourceCategory->getString());
		// clonedCompPtr->applyAngularImpulse(this->angularImpulse);
		// clonedCompPtr->applyAngularVelocity(this->angularVelocity->getVector3());
		clonedCompPtr->applyForce(this->force->getVector3());
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

		clonedCompPtr->setOnKinematicContactFunctionName(this->onKinematicContactFunctionName->getString());

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool PhysicsActiveKinematicComponent::postInit(void)
	{
		bool success = PhysicsComponent::postInit();

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveKinematicComponent] Init physics active kinematic component for game object: " 
			+ this->gameObjectPtr->getName());

		this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
		this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
		this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();

		// Physics active component must be dynamic, else a mess occurs
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		// this->gameObjectPtr->getAttribute(PhysicsActiveComponent::AttrMass())->setVisible(false);

		if (false == this->createDynamicBody())
			return false;

		return success;
	}

	bool PhysicsActiveKinematicComponent::connect(void)
	{
		bool success = PhysicsActiveComponent::connect();

		this->setOnKinematicContactFunctionName(this->onKinematicContactFunctionName->getString());

		return success;
	}

	bool PhysicsActiveKinematicComponent::disconnect(void)
	{
		bool success = PhysicsActiveComponent::disconnect();
		return success;
	}

	void PhysicsActiveKinematicComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			static_cast<OgreNewt::KinematicBody*>(this->physicsBody)->integrateVelocity(dt);
		}
	}

	void PhysicsActiveKinematicComponent::actualizeValue(Variant* attribute)
	{
		PhysicsActiveComponent::actualizeCommonValue(attribute);

		if (PhysicsActiveKinematicComponent::AttrOnKinematicContactFunctionName() == attribute->getName())
		{
		 this->setOnKinematicContactFunctionName(attribute->getString());
		}
	}

	void PhysicsActiveKinematicComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "OnKinematicContactFunctionName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->onKinematicContactFunctionName->getString())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String PhysicsActiveKinematicComponent::getClassName(void) const
	{
		return "PhysicsActiveKinematicComponent";
	}

	Ogre::String PhysicsActiveKinematicComponent::getParentClassName(void) const
	{
		return "PhysicsActiveComponent";
	}

	Ogre::String PhysicsActiveKinematicComponent::getParentParentClassName(void) const
	{
		return "PhysicsComponent";
	}

	void PhysicsActiveKinematicComponent::setOnKinematicContactFunctionName(const Ogre::String& onKinematicContactFunctionName)
	{
		this->onKinematicContactFunctionName->setValue(onKinematicContactFunctionName);
		if (false == onKinematicContactFunctionName.empty())
		{
			this->onKinematicContactFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), onKinematicContactFunctionName + "(otherGameObject)");
			this->setKinematicContactSolvingEnabled(true);
		}
		else
		{
			this->setKinematicContactSolvingEnabled(false);
		}
	}

	bool PhysicsActiveKinematicComponent::createDynamicBody(void)
	{
		Ogre::Vector3 inertia = Ogre::Vector3(1.0f, 1.0f, 1.0f);

		Ogre::Quaternion collisionOrientation = Ogre::Quaternion::IDENTITY; // this->gameObjectPtr->getSceneNode()->getOrientation();
		if (Ogre::Vector3::ZERO != this->collisionDirection->getVector3())
		{
			collisionOrientation = MathHelper::getInstance()->degreesToQuat(this->collisionDirection->getVector3());
		}

		Ogre::Vector3 calculatedMassOrigin = Ogre::Vector3::ZERO;

		auto collisionPtr = this->createDynamicCollision(inertia, this->collisionSize->getVector3(), this->collisionPosition->getVector3(), collisionOrientation, calculatedMassOrigin, this->gameObjectPtr->getCategoryId());
		
		if (Ogre::Vector3::ZERO != this->massOrigin->getVector3())
		{
			calculatedMassOrigin = this->massOrigin->getVector3();
		}

		this->physicsBody = new OgreNewt::KinematicBody(this->ogreNewt, this->gameObjectPtr->getSceneManager(), collisionPtr);

		this->physicsBody->setGravity(Ogre::Vector3::ZERO);

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

		this->setCollidable(this->collidable->getBool());

		// Note: Kinematic Body has not force and torque callback!
		// this->physicsBody->setCustomForceAndTorqueCallback<PhysicsActiveKinematicComponent>(&PhysicsActiveComponent::moveCallback, this);
		this->physicsBody->setCustomForceAndTorqueCallback(nullptr);

		this->setActivated(this->activated->getBool());

		// set user data for ogrenewt
		this->physicsBody->setUserData(OgreNewt::Any(dynamic_cast<PhysicsComponent*>(this)));

		this->physicsBody->attachNode(this->gameObjectPtr->getSceneNode());

		this->setPosition(this->initialPosition);
		this->setOrientation(this->initialOrientation);

		this->setConstraintAxis(this->constraintAxis->getVector3());

		// pin the object stand in pose and not fall down
		this->setConstraintDirection(this->constraintDirection->getVector3());

		this->setCollidable(this->collidable->getBool());

		this->physicsBody->setType(this->gameObjectPtr->getCategoryId());
		this->physicsBody->setMaterialGroupID(
			AppStateManager::getSingletonPtr()->getGameObjectController()->getMaterialID(this->gameObjectPtr.get(), this->ogreNewt));

		return true;
	}

	void PhysicsActiveKinematicComponent::kinematicContactCallback(OgreNewt::Body* otherBody)
	{
		PhysicsComponent* otherPhysicsComponent = OgreNewt::any_cast<PhysicsComponent*>(otherBody->getUserData());

		if (nullptr != this->gameObjectPtr->getLuaScript())
		{
			this->gameObjectPtr->getLuaScript()->callTableFunction(this->onKinematicContactFunctionName->getString(), otherPhysicsComponent->getOwner());
		}
	}

	void PhysicsActiveKinematicComponent::setOmegaVelocityRotateTo(const Ogre::Quaternion& resultOrientation, const Ogre::Vector3& axes, Ogre::Real strength)
	{
		if (nullptr == this->physicsBody)
			return;

		Ogre::Quaternion diffOrientation = this->physicsBody->getOrientation().Inverse() * resultOrientation;
		Ogre::Vector3 resultVector = Ogre::Vector3::ZERO;

		if (axes.x == 1.0f)
		{
			resultVector.x = diffOrientation.getPitch().valueDegrees() * strength;
		}
		if (axes.y == 1.0f)
		{
			resultVector.y = diffOrientation.getYaw().valueDegrees() * strength;
		}
		if (axes.z == 1.0f)
		{
			resultVector.z = diffOrientation.getRoll().valueDegrees() * strength;
		}

		this->setOmegaVelocity(resultVector);
	}

	void PhysicsActiveKinematicComponent::setKinematicContactSolvingEnabled(bool enable)
	{
		if (nullptr == this->gameObjectPtr || nullptr == this->gameObjectPtr->getLuaScript() || true == this->onKinematicContactFunctionName->getString().empty() || nullptr == this->physicsBody)
		{
			return;
		}
		if (true == enable)
		{
			static_cast<OgreNewt::KinematicBody*>(this->physicsBody)->setKinematicContactCallback<PhysicsActiveKinematicComponent>(&PhysicsActiveKinematicComponent::kinematicContactCallback, this);
		}
		else
		{
			static_cast<OgreNewt::KinematicBody*>(this->physicsBody)->removeKinematicContactCallback();
		}
	}

}; // namespace end
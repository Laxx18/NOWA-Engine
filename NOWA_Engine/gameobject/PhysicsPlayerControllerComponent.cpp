#include "NOWAPrecompiled.h"
#include "PhysicsPlayerControllerComponent.h"
#include "LuaScriptComponent.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "main/AppStateManager.h"
#include "PhysicsMaterialComponent.h"
#include "tinyxml.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	PhysicsPlayerControllerComponent::PhysicsPlayerCallback::PhysicsPlayerCallback(GameObject* owner, LuaScript* luaScript, OgreNewt::World* ogreNewt, 
		const Ogre::String& onContactFrictionFunctionName, const Ogre::String& onContactFunctionName)
		: OgreNewt::PlayerCallback(),
		owner(owner),
		luaScript(luaScript),
		ogreNewt(ogreNewt),
		onContactFrictionFunctionName(onContactFrictionFunctionName),
		onContactFunctionName(onContactFunctionName),
		playerFrictionContact(new PlayerContact),
		playerContact(new PlayerContact)
	{

	}

	PhysicsPlayerControllerComponent::PhysicsPlayerCallback::~PhysicsPlayerCallback()
	{
		if (nullptr != this->playerFrictionContact)
		{
			delete this->playerFrictionContact;
			this->playerFrictionContact = nullptr;
		}
		if (nullptr != this->playerContact)
		{
			delete this->playerContact;
			this->playerContact = nullptr;
		}
	}

	Ogre::Real PhysicsPlayerControllerComponent::PhysicsPlayerCallback::onContactFriction(const OgreNewt::PlayerControllerBody* visitor, const Ogre::Vector3& position, const Ogre::Vector3& normal, int contactId, const OgreNewt::Body* other)
	{
		PhysicsComponent* visitorPhysicsComponent = OgreNewt::any_cast<PhysicsComponent*>(visitor->getUserData());
		if (nullptr != visitorPhysicsComponent)
		{
			GameObject* gameObject0 = visitorPhysicsComponent->getOwner().get();

			if (nullptr != luaScript && false == this->onContactFrictionFunctionName.empty())
			{
				if (nullptr == other)
				{
					return 2.0f;
				}

				GameObject* gameObject1 = nullptr;
				auto physicsComponent1 = OgreNewt::any_cast<PhysicsComponent*>(other->getUserData());
				if (nullptr != physicsComponent1)
				{
					gameObject1 = physicsComponent1->getOwner().get();

					this->playerFrictionContact->position = position;
					this->playerFrictionContact->normal = normal;

					luaScript->callTableFunction(this->onContactFrictionFunctionName, gameObject0, gameObject1, this->playerFrictionContact);

					return this->playerFrictionContact->resultFriction;
				}
			}
		}
		return 2.0f;
	}

	void PhysicsPlayerControllerComponent::PhysicsPlayerCallback::onContact(const OgreNewt::PlayerControllerBody* visitor, const Ogre::Vector3& position, const Ogre::Vector3& normal, Ogre::Real penetration, int contactId, const OgreNewt::Body* other)
	{
		PhysicsComponent* visitorPhysicsComponent = OgreNewt::any_cast<PhysicsComponent*>(visitor->getUserData());
		if (nullptr != visitorPhysicsComponent)
		{
			GameObject* gameObject0 = visitorPhysicsComponent->getOwner().get();

			Ogre::Real outFriction = 0.0f;
			if (nullptr != luaScript && false == this->onContactFunctionName.empty())
			{
				if (nullptr == other)
				{
					return;
				}

				GameObject* gameObject1 = nullptr;
				auto physicsComponent1 = OgreNewt::any_cast<PhysicsComponent*>(other->getUserData());
				if (nullptr != physicsComponent1)
				{
					gameObject1 = physicsComponent1->getOwner().get();

					this->playerContact->position = position;
					this->playerContact->normal = normal;
					this->playerContact->penetration = penetration;

					luaScript->callTableFunction(this->onContactFunctionName, gameObject0, gameObject1, this->playerContact);
				}
			}
		}
	}

	void PhysicsPlayerControllerComponent::PhysicsPlayerCallback::setLuaScript(LuaScript* luaScript)
	{
		this->luaScript = luaScript;
	}

	void PhysicsPlayerControllerComponent::PhysicsPlayerCallback::setCallbackFunctions(const Ogre::String& onContactFrictionFunctionName, const Ogre::String& onContactFunctionName)
	{
		this->onContactFrictionFunctionName = onContactFrictionFunctionName;
		this->onContactFunctionName = onContactFunctionName;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	PhysicsPlayerControllerComponent::PhysicsPlayerControllerComponent()
		: PhysicsActiveComponent(),
		playerController(nullptr),
		radius(new Variant(PhysicsPlayerControllerComponent::AttrRadius(), 0.4f, this->attributes)),
		stepHeight(new Variant(PhysicsPlayerControllerComponent::AttrStepHeight(), 0.3f, this->attributes)),
		height(new Variant(PhysicsPlayerControllerComponent::AttrHeight(), 1.9f, this->attributes)),
		jumpSpeed(new Variant(PhysicsPlayerControllerComponent::AttrJumpSpeed(), 10.0f, this->attributes)),
		onContactFrictionFunctionName(new Variant(PhysicsPlayerControllerComponent::AttrOnContactFrictionFunctionName(), Ogre::String(""), this->attributes)),
		onContactFunctionName(new Variant(PhysicsPlayerControllerComponent::AttrOnContactFunctionName(), Ogre::String(""), this->attributes)),
		newlyCreated(true),
		isInCloningProcess(false)
	{
		this->force->setVisible(false);
		this->collisionDirection->setVisible(false);
		this->collisionSize->setVisible(false);
		this->collisionPosition->setVisible(true);
		this->asSoftBody->setValue(false);
		this->asSoftBody->setVisible(false);
		this->collisionType->setVisible(false);
		this->constraintDirection->setValue(false);
		this->constraintDirection->setVisible(false);
		this->gyroscopicTorque->setValue(false);
		this->gyroscopicTorque->setVisible(false);

		this->onContactFrictionFunctionName->setDescription("Sets the function name to react in lua script at the moment when a player controller has friction with a game object below. E.g. onContactFriction(gameObject0, gameObject1, playerContact)."
													"The function should set in the player contact result friction. With that its possible to control how much friction the player will get on the ground.");
		this->onContactFrictionFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), onContactFrictionFunctionName->getString() + "(gameObject0, gameObject1, playerContact)");

		this->onContactFunctionName->setDescription("Sets the function name to react in lua script at the moment when a player controller collided with another game object. E.g. onContact(gameObject0, gameObject1, playerContact)."
			"The function should set in the player contact result friction. With that its possible to control how much friction the player will get on the ground.");
		this->onContactFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), onContactFunctionName->getString() + "(gameObject0, gameObject1, playerContact)");

		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &PhysicsPlayerControllerComponent::handleTranslateFinished), EventDataTranslateFinished::getStaticEventType());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &PhysicsPlayerControllerComponent::handleRotateFinished), EventDataRotateFinished::getStaticEventType());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &PhysicsPlayerControllerComponent::handleDefaultDirectionChanged), EventDataDefaultDirectionChanged::getStaticEventType());
	}

	PhysicsPlayerControllerComponent::~PhysicsPlayerControllerComponent()
	{
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &PhysicsPlayerControllerComponent::handleTranslateFinished), EventDataTranslateFinished::getStaticEventType());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &PhysicsPlayerControllerComponent::handleRotateFinished), EventDataRotateFinished::getStaticEventType());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &PhysicsPlayerControllerComponent::handleDefaultDirectionChanged), EventDataDefaultDirectionChanged::getStaticEventType());

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsPlayerControllerComponent] Destructor physics player controller component for game object: " 
			+ this->gameObjectPtr->getName());
	}

	bool PhysicsPlayerControllerComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		PhysicsActiveComponent::parseCommonProperties(propertyElement, filename);

		// either load parts from mesh compound config file, the parts will have its correct position and rotation offset
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Radius")
		{
			this->radius->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Height")
		{
			this->height->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "StepHeight")
		{
			this->stepHeight->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JumpSpeed")
		{
			this->jumpSpeed->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OnContactFrictionFunctionName")
		{
			this->setOnContactFrictionFunctionName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OnContactFunctionName")
		{
			this->setOnContactFunctionName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		this->newlyCreated = false;
		return true;
	}

	GameObjectCompPtr PhysicsPlayerControllerComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		PhysicsPlayerControllerCompPtr clonedCompPtr(boost::make_shared<PhysicsPlayerControllerComponent>());
		
		clonedCompPtr->isInCloningProcess = true;

		clonedCompPtr->setActivated(this->activated->getBool());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

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

		clonedCompPtr->setCollisionPosition(this->collisionPosition->getVector3());
		// Bug in newton, setting afterwards collidable to true, will not work, hence do not clone this property
		// clonedCompPtr->setCollidable(this->collidable->getBool());

		clonedCompPtr->setRadius(this->radius->getReal());
		clonedCompPtr->setHeight(this->height->getReal());
		clonedCompPtr->setStepHeight(this->stepHeight->getReal());
		clonedCompPtr->setJumpSpeed(this->jumpSpeed->getReal());

		clonedCompPtr->setOnContactFrictionFunctionName(this->onContactFrictionFunctionName->getString());
		clonedCompPtr->setOnContactFunctionName(this->onContactFunctionName->getString());;
		
		clonedCompPtr->isInCloningProcess = false;

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	void PhysicsPlayerControllerComponent::setPosition(Ogre::Real x, Ogre::Real y, Ogre::Real z)
	{
		this->setPosition(Ogre::Vector3(x, y, z));
	}

	void PhysicsPlayerControllerComponent::setPosition(const Ogre::Vector3& position)
	{
		PhysicsComponent::setPosition(this->initialPosition);

		this->gameObjectPtr->setAttributePosition(position);
		OgreNewt::PlayerControllerBody* playerControllerBody = static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody);
		if (nullptr != playerControllerBody)
		{
			this->initialPosition = position;

			LuaScript* luaScript = nullptr;
			auto& luaScriptCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<LuaScriptComponent>());
			if (nullptr != luaScriptCompPtr)
			{
				luaScript = luaScriptCompPtr->getLuaScript();
				if (nullptr == luaScript)
				{
					luaScriptCompPtr->postInit();
				}
			}

			if (nullptr == luaScript)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsPlayerControllerComponent] Physics active vehicle cannot be controlled, because this game object: "
																+ this->gameObjectPtr->getName() + " has no lua script component.");
			}

			static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody)->reCreatePlayer(this->initialOrientation, this->initialPosition,
				this->gameObjectPtr->getDefaultDirection(), this->mass->getReal(),
				this->radius->getReal(), this->height->getReal(), this->stepHeight->getReal(), this->gameObjectPtr->getCategoryId(), new PhysicsPlayerCallback(this->gameObjectPtr.get(), luaScript, this->ogreNewt,
					this->onContactFrictionFunctionName->getString(), this->onContactFunctionName->getString()));

		}
	}

	void PhysicsPlayerControllerComponent::setOrientation(const Ogre::Quaternion& orientation)
	{
		PhysicsComponent::setOrientation(this->initialOrientation);

		this->gameObjectPtr->setAttributeOrientation(orientation);
		OgreNewt::PlayerControllerBody* playerControllerBody = static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody);
		if (nullptr != playerControllerBody)
		{
			this->initialOrientation = orientation;
			playerControllerBody->setStartOrientation(orientation);

			static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody)->reCreatePlayer(this->initialOrientation, this->initialPosition,
				this->gameObjectPtr->getDefaultDirection(), this->mass->getReal(),
				this->radius->getReal(), this->height->getReal(), this->stepHeight->getReal(), this->gameObjectPtr->getCategoryId(), new PhysicsPlayerCallback(this->gameObjectPtr.get(), this->gameObjectPtr->getLuaScript(), this->ogreNewt,
					this->onContactFrictionFunctionName->getString(), this->onContactFunctionName->getString()));

		}
	}

	void PhysicsPlayerControllerComponent::setScale(const Ogre::Vector3& scale)
	{
		if (MathHelper::getInstance()->vector3Equals(this->gameObjectPtr->getScale(), scale))
		{
			return;
		}

		this->gameObjectPtr->setAttributeScale(scale);
		OgreNewt::PlayerControllerBody* playerControllerBody = static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody);
		if (nullptr != playerControllerBody)
		{
			if (this->initialScale != scale)
			{
				this->initialScale = scale;

				this->adjustScale();

				static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody)->reCreatePlayer(this->initialOrientation, this->initialPosition,
					this->gameObjectPtr->getDefaultDirection(), this->mass->getReal(),
					this->radius->getReal(), this->height->getReal(), this->stepHeight->getReal(), this->gameObjectPtr->getCategoryId(), new PhysicsPlayerCallback(this->gameObjectPtr.get(), this->gameObjectPtr->getLuaScript(), this->ogreNewt,
						this->onContactFrictionFunctionName->getString(), this->onContactFunctionName->getString()));
			}
		}

		PhysicsComponent::setScale(this->initialScale);
	}

	bool PhysicsPlayerControllerComponent::postInit(void)
	{
		bool success = PhysicsComponent::postInit();
		
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsPlayerControllerComponent] Init physics player controller component for game object: " 
			+ this->gameObjectPtr->getName());

		this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
		this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
		this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();

		// Physics active component must be dynamic, else a mess occurs
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		if (true == this->newlyCreated)
		{
			this->adjustScale();
		}

		if (false == this->createDynamicBody())
		{
			return false;
		}

		return success;
	}

	bool PhysicsPlayerControllerComponent::connect(void)
	{
		bool success = PhysicsActiveComponent::connect();

		if (nullptr != this->physicsBody)
		{
			auto playerCallback = static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody)->getPlayerCallback();
			static_cast<PhysicsPlayerControllerComponent::PhysicsPlayerCallback*>(playerCallback)->setCallbackFunctions(this->onContactFrictionFunctionName->getString(), this->onContactFunctionName->getString());

			static_cast<PhysicsPlayerControllerComponent::PhysicsPlayerCallback*>(playerCallback)->setLuaScript(this->gameObjectPtr->getLuaScript());
		}

		return success;
	}

	bool PhysicsPlayerControllerComponent::disconnect(void)
	{
		bool success = PhysicsActiveComponent::disconnect();

		// Rotate back to default direction
		this->stop();

		return success;
	}

	void PhysicsPlayerControllerComponent::actualizeValue(Variant* attribute)
	{
		// Some attribute changed, component is no longer new!
		this->newlyCreated = false;

		PhysicsActiveComponent::actualizeCommonValue(attribute);

		if (PhysicsActiveComponent::AttrCollisionPosition() == attribute->getName())
		{
			this->setCollisionPosition(attribute->getVector3());
		}
		else if (PhysicsActiveComponent::AttrMass() == attribute->getName())
		{
			this->setMass(attribute->getReal());
		}
		else if (PhysicsPlayerControllerComponent::AttrRadius() == attribute->getName())
		{
			this->setRadius(attribute->getReal());
		}
		else if (PhysicsPlayerControllerComponent::AttrHeight() == attribute->getName())
		{
			this->setHeight(attribute->getReal());
		}
		else if (PhysicsPlayerControllerComponent::AttrStepHeight() == attribute->getName())
		{
			this->setStepHeight(attribute->getReal());
		}
		else if (PhysicsPlayerControllerComponent::AttrJumpSpeed() == attribute->getName())
		{
			this->setJumpSpeed(attribute->getReal());
		}
		else if (PhysicsPlayerControllerComponent::AttrOnContactFrictionFunctionName() == attribute->getName())
		{
			this->setOnContactFrictionFunctionName(attribute->getString());
		}
		else if (PhysicsPlayerControllerComponent::AttrOnContactFunctionName() == attribute->getName())
		{
			this->setOnContactFunctionName(attribute->getString());
		}
	}

	void PhysicsPlayerControllerComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		PhysicsActiveComponent::writeCommonProperties(propertiesXML, doc);
		
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Radius"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->radius->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Height"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->height->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "StepHeight"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->stepHeight->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JumpSpeed"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->jumpSpeed->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OnContactFrictionFunctionName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->onContactFrictionFunctionName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OnContactFunctionName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->onContactFunctionName->getString())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String PhysicsPlayerControllerComponent::getClassName(void) const
	{
		return "PhysicsPlayerControllerComponent";
	}

	Ogre::String PhysicsPlayerControllerComponent::getParentClassName(void) const
	{
		return "PhysicsActiveComponent";
	}

	Ogre::String PhysicsPlayerControllerComponent::getParentParentClassName(void) const
	{
		return "PhysicsComponent";
	}

	bool PhysicsPlayerControllerComponent::createDynamicBody(void)
	{
		// If component is being cloned, do not create the dynamic body to early, because not all data is ready, hence e.g. the position would be 0 0 0
		if (true == this->isInCloningProcess)
		{
			return true;
		}

		Ogre::Vector3 inertia = Ogre::Vector3(1.0f, 1.0f, 1.0f);
		Ogre::Vector3 calculatedMassOrigin = Ogre::Vector3::ZERO;

		// height must be >= step height for newton else assert
		if (0.0f == this->height->getReal() || this->height->getReal() < this->stepHeight->getReal())
		{
			this->height->setValue(this->gameObjectPtr->getSize().y);
		}

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
		{
			this->physicsBody = new OgreNewt::PlayerControllerBody(this->ogreNewt, this->gameObjectPtr->getSceneManager(), this->initialOrientation, this->initialPosition,
				this->gameObjectPtr->getDefaultDirection(), this->mass->getReal(),
				this->radius->getReal(), this->height->getReal(), this->stepHeight->getReal(), this->gameObjectPtr->getCategoryId(), new PhysicsPlayerCallback(this->gameObjectPtr.get(), this->gameObjectPtr->getLuaScript(), this->ogreNewt,
					this->onContactFrictionFunctionName->getString(), this->onContactFunctionName->getString()));
			this->physicsBody->attachNode(this->gameObjectPtr->getSceneNode());
		}
		else
		{
			// Does make stress, when internal body is changed, so delete it, so that it can be recreated
			if (nullptr != this->upVector)
			{
				this->upVector->destroyJoint(this->ogreNewt);
				delete this->upVector;
				this->upVector = nullptr;
			}
			static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody)->reCreatePlayer(this->initialOrientation, this->initialPosition,
				this->gameObjectPtr->getDefaultDirection(), this->mass->getReal(),
				this->radius->getReal(), this->height->getReal(), this->stepHeight->getReal(), this->gameObjectPtr->getCategoryId(), new PhysicsPlayerCallback(this->gameObjectPtr.get(), this->gameObjectPtr->getLuaScript(), this->ogreNewt,
					this->onContactFrictionFunctionName->getString(), this->onContactFunctionName->getString()));
		}

		OgreNewt::PlayerControllerBody* playerControllerBody = static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody);
		// playerControllerBody->setWalkSpee(Ogre::Degree(this->climbSlope->getReal()));
		// playerControllerBody->setRestrainingDistance(this->restrainingDistance->getReal());

		this->setSpeed(this->speed->getReal());
		this->setJumpSpeed(this->jumpSpeed->getReal());

		// Set the body initial transform
		this->setOrientation(this->initialOrientation);
		this->setPosition(this->initialPosition);

		this->physicsBody->setGravity(this->gravity->getVector3());

		Ogre::Real mass = this->mass->getReal();
		
		Ogre::Vector3 massOrigin = this->massOrigin->getVector3();
		
		/*if (this->collisionType->getListSelectedValue() == "ConvexHull")
		{
			this->physicsBody->setConvexIntertialMatrix(inertia, calculatedMassOrigin);
		}*/

		// Apply mass and scale to inertia (the bigger the object, the more mass)
		inertia *= this->mass->getReal();
		this->physicsBody->setMassMatrix(this->mass->getReal(), inertia);

		// set mass origin
		this->physicsBody->setCenterOfMass(calculatedMassOrigin);

		if (this->linearDamping->getReal() != 0.0f)
		{
			this->physicsBody->setLinearDamping(this->linearDamping->getReal());
		}
		if (this->angularDamping->getVector3() != Ogre::Vector3::ZERO)
		{
			this->physicsBody->setAngularDamping(this->angularDamping->getVector3());
		}

		// this->physicsBody->setCustomForceAndTorqueCallback<PhysicsPlayerControllerComponent>(&PhysicsActiveComponent::moveCallback, this);

		// Must be set after body set its position! And also the current orientation (initialOrientation) must be correct! Else weird rotation behavior occurs
		this->setConstraintAxis(this->constraintAxis->getVector3());
		// Pin the object stand in pose and not fall down
		// this->setConstraintDirection(this->constraintDirection->getVector3());

		this->setActivated(this->activated->getBool());
		this->setGyroscopicTorqueEnabled(this->gyroscopicTorque->getBool());

		this->setCollidable(this->collidable->getBool());

		// set user data for ogrenewt
		this->physicsBody->setUserData(OgreNewt::Any(dynamic_cast<PhysicsComponent*>(this)));

		this->physicsBody->setType(this->gameObjectPtr->getCategoryId());
		this->physicsBody->setMaterialGroupID(AppStateManager::getSingletonPtr()->getGameObjectController()->getMaterialID(this->gameObjectPtr.get(), this->ogreNewt));

		return true;
	}

	void PhysicsPlayerControllerComponent::handleTranslateFinished(NOWA::EventDataPtr eventData)
	{
		boost::shared_ptr<NOWA::EventDataTranslateFinished> castEventData = boost::static_pointer_cast<NOWA::EventDataTranslateFinished>(eventData);

		if (castEventData->getGameObjectId() == this->gameObjectPtr->getId())
		{
			this->gameObjectPtr->setAttributePosition(castEventData->getNewPosition());
			OgreNewt::PlayerControllerBody* playerControllerBody = static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody);
			if (nullptr != playerControllerBody)
			{
				this->setPosition(castEventData->getNewPosition());

				this->initialPosition = castEventData->getNewPosition();
				static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody)->reCreatePlayer(this->initialOrientation, this->initialPosition,
					this->gameObjectPtr->getDefaultDirection(), this->mass->getReal(),
					this->radius->getReal(), this->height->getReal(), this->stepHeight->getReal(), this->gameObjectPtr->getCategoryId(), new PhysicsPlayerCallback(this->gameObjectPtr.get(), this->gameObjectPtr->getLuaScript(), this->ogreNewt,
						this->onContactFrictionFunctionName->getString(), this->onContactFunctionName->getString()));
			}
		}
	}

	void PhysicsPlayerControllerComponent::handleRotateFinished(NOWA::EventDataPtr eventData)
	{
		boost::shared_ptr<NOWA::EventDataRotateFinished> castEventData = boost::static_pointer_cast<NOWA::EventDataRotateFinished>(eventData);

		if (castEventData->getGameObjectId() == this->gameObjectPtr->getId())
		{
			this->gameObjectPtr->setAttributeOrientation(castEventData->getNewOrientation());
			OgreNewt::PlayerControllerBody* playerControllerBody = static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody);
			if (nullptr != playerControllerBody)
			{
				playerControllerBody->setPositionOrientation(this->physicsBody->getPosition(), castEventData->getNewOrientation());
				this->setOrientation(castEventData->getNewOrientation());

				playerControllerBody->setStartOrientation(castEventData->getNewOrientation());
				this->initialOrientation = castEventData->getNewOrientation();
				static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody)->reCreatePlayer(this->initialOrientation, this->initialPosition,
					this->gameObjectPtr->getDefaultDirection(), this->mass->getReal(),
					this->radius->getReal(), this->height->getReal(), this->stepHeight->getReal(), this->gameObjectPtr->getCategoryId(), new PhysicsPlayerCallback(this->gameObjectPtr.get(), this->gameObjectPtr->getLuaScript(), this->ogreNewt,
						this->onContactFrictionFunctionName->getString(), this->onContactFunctionName->getString()));
			}
		}
	}

	void PhysicsPlayerControllerComponent::handleDefaultDirectionChanged(NOWA::EventDataPtr eventData)
	{
		boost::shared_ptr<NOWA::EventDataDefaultDirectionChanged> castEventData = boost::static_pointer_cast<NOWA::EventDataDefaultDirectionChanged>(eventData);

		if (castEventData->getGameObjectId() == this->gameObjectPtr->getId())
		{
			this->setDefaultDirection(castEventData->getDefaultDirection());
		}
	}

	void PhysicsPlayerControllerComponent::adjustScale(void)
	{
		Ogre::Vector3 size = this->gameObjectPtr->getSize();

		this->height->setValue(size.y);
		Ogre::Real radiusFactor = 1.0f;

		if (size.x > size.z)
		{
			radiusFactor = size.x * 0.5f;
		}
		else if (size.z > size.x)
		{
			radiusFactor = size.z * 0.5f;
		}

		this->radius->setValue(radiusFactor);
	}

	void PhysicsPlayerControllerComponent::move(Ogre::Real forwardSpeed, Ogre::Real sideSpeed, const Ogre::Radian& headingAngleRad)
	{
		OgreNewt::PlayerControllerBody* playerControllerBody = static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody);
		if (nullptr != playerControllerBody)
		{
			playerControllerBody->move(forwardSpeed, sideSpeed, headingAngleRad);
		}
	}

	void PhysicsPlayerControllerComponent::setHeading(const Ogre::Radian& headingAngleRad)
	{
		OgreNewt::PlayerControllerBody* playerControllerBody = static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody);
		if (nullptr != playerControllerBody)
		{
			playerControllerBody->setHeading(headingAngleRad);
		}
	}

	void PhysicsPlayerControllerComponent::stop(void)
	{
		OgreNewt::PlayerControllerBody* playerControllerBody = static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody);
		if (nullptr != playerControllerBody)
		{
			playerControllerBody->stop();
		}
	}

	void PhysicsPlayerControllerComponent::toggleCrouch(void)
	{
		OgreNewt::PlayerControllerBody* playerControllerBody = static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody);
		if (nullptr != playerControllerBody)
		{
			playerControllerBody->toggleCrouch();
		}
	}

	void PhysicsPlayerControllerComponent::jump(void)
	{
		OgreNewt::PlayerControllerBody* playerControllerBody = static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody);
		if (nullptr != playerControllerBody)
		{
			playerControllerBody->setCanJump(true);
		}
	}

	void PhysicsPlayerControllerComponent::setDefaultDirection(const Ogre::Vector3& defaultDirection)
	{
		OgreNewt::PlayerControllerBody* playerControllerBody = static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody);
		if (nullptr != playerControllerBody)
		{
			if (static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody)->getDirection() != defaultDirection)
			{
				static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody)->reCreatePlayer(this->initialOrientation, this->initialPosition,
					defaultDirection, this->mass->getReal(),
					this->radius->getReal(), this->height->getReal(), this->stepHeight->getReal(), this->gameObjectPtr->getCategoryId(), new PhysicsPlayerCallback(this->gameObjectPtr.get(), this->gameObjectPtr->getLuaScript(), this->ogreNewt,
						this->onContactFrictionFunctionName->getString(), this->onContactFunctionName->getString()));
			}
		}
	}

	void PhysicsPlayerControllerComponent::setMass(Ogre::Real mass)
	{
		if (mass <= 0.0f)
		{
			mass = 1.0f;
		}
		this->mass->setValue(mass);
		OgreNewt::PlayerControllerBody* playerControllerBody = static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody);
		if (nullptr != playerControllerBody)
		{
			playerControllerBody->setMass(mass);
		}
		this->createDynamicBody();
	}

	void PhysicsPlayerControllerComponent::setVelocity(const Ogre::Vector3& velocity)
	{
		OgreNewt::PlayerControllerBody* playerControllerBody = static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody);
		if (nullptr != playerControllerBody)
		{
			playerControllerBody->setVelocity(velocity);
		}
	}

	Ogre::Vector3 PhysicsPlayerControllerComponent::getVelocity(void) const
	{
		Ogre::Vector3 velocity = Ogre::Vector3::ZERO;
		OgreNewt::PlayerControllerBody* playerControllerBody = static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody);
		if (nullptr != playerControllerBody)
		{
			velocity = playerControllerBody->getVelocity();
		}
		return velocity;
	}

	void PhysicsPlayerControllerComponent::setFrame(const Ogre::Quaternion& frame)
	{
		OgreNewt::PlayerControllerBody* playerControllerBody = static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody);
		if (nullptr != playerControllerBody)
		{
			playerControllerBody->setFrame(frame);
		}
	}

	Ogre::Quaternion PhysicsPlayerControllerComponent::getFrame(void) const
	{
		Ogre::Quaternion frame = Ogre::Quaternion::IDENTITY;
		OgreNewt::PlayerControllerBody* playerControllerBody = static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody);
		if (nullptr != playerControllerBody)
		{
			frame = playerControllerBody->getFrame();
		}
		return frame;
	}

	void PhysicsPlayerControllerComponent::setCollisionPosition(const Ogre::Vector3& collisionPosition)
	{
		this->collisionPosition->setValue(collisionPosition);
		OgreNewt::PlayerControllerBody* playerControllerBody = static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody);
		if (nullptr != playerControllerBody)
		{
			playerControllerBody->setCollisionPositionOffset(collisionPosition);
		}
		this->createDynamicBody();
	}

	void PhysicsPlayerControllerComponent::setRadius(Ogre::Real radius)
	{
		Ogre::Real tempRadius = radius;
		if (tempRadius <= 0.0f)
		{
			tempRadius = 0.1f;
		}

		this->radius->setValue(tempRadius);

		OgreNewt::PlayerControllerBody* playerControllerBody = static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody);
		if (nullptr != playerControllerBody)
		{
			playerControllerBody->setRadius(radius);
		}
		this->createDynamicBody();
	}

	Ogre::Real PhysicsPlayerControllerComponent::getRadius(void) const
	{
		return this->radius->getReal();
	}

	void PhysicsPlayerControllerComponent::setHeight(Ogre::Real height)
	{
		Ogre::Real tempHeight = height;
		if (tempHeight <= 0.0f)
		{
			tempHeight = 0.1f;
		}

		this->height->setValue(tempHeight);

		OgreNewt::PlayerControllerBody* playerControllerBody = static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody);
		if (nullptr != playerControllerBody)
		{
			playerControllerBody->setHeight(height);
		}
		this->createDynamicBody();
	}

	Ogre::Real PhysicsPlayerControllerComponent::getHeight(void) const
	{
		return this->height->getReal();
	}

	void PhysicsPlayerControllerComponent::setStepHeight(Ogre::Real stepHeight)
	{
		this->stepHeight->setValue(stepHeight);
		OgreNewt::PlayerControllerBody* playerControllerBody = static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody);
		if (nullptr != playerControllerBody)
		{
			playerControllerBody->setStepHeight(stepHeight);
		}
		this->createDynamicBody();
	}

	Ogre::Real PhysicsPlayerControllerComponent::getStepHeight(void) const
	{
		return this->stepHeight->getReal();
	}

	Ogre::Vector3 PhysicsPlayerControllerComponent::getUpDirection(void) const
	{
		Ogre::Vector3 upDirection = Ogre::Vector3::ZERO;
		OgreNewt::PlayerControllerBody* playerControllerBody = static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody);
		if (nullptr != playerControllerBody)
		{
			upDirection = playerControllerBody->getUpDirection();
		}
		return upDirection;
	}

	bool PhysicsPlayerControllerComponent::isInFreeFall(void) const
	{
		bool freeFall = false;
		OgreNewt::PlayerControllerBody* playerControllerBody = static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody);
		if (nullptr != playerControllerBody)
		{
			freeFall = playerControllerBody->isInFreeFall();
		}
		return freeFall;
	}

	bool PhysicsPlayerControllerComponent::isOnFloor(void) const
	{
		bool onFloor = false;
		OgreNewt::PlayerControllerBody* playerControllerBody = static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody);
		if (nullptr != playerControllerBody)
		{
			onFloor = playerControllerBody->isOnFloor();
		}
		return onFloor;
	}

	bool PhysicsPlayerControllerComponent::isCrouching(void) const
	{
		bool crouched = false;
		OgreNewt::PlayerControllerBody* playerControllerBody = static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody);
		if (nullptr != playerControllerBody)
		{
			crouched = playerControllerBody->isCrouched();
		}
		return crouched;
	}

	void PhysicsPlayerControllerComponent::setSpeed(Ogre::Real speed)
	{
		this->speed->setValue(speed);
		OgreNewt::PlayerControllerBody* playerControllerBody = static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody);
		if (nullptr != playerControllerBody)
		{
			playerControllerBody->setWalkSpeed(speed);
		}
	}

	void PhysicsPlayerControllerComponent::setJumpSpeed(Ogre::Real jumpSpeed)
	{
		this->jumpSpeed->setValue(jumpSpeed);
		OgreNewt::PlayerControllerBody* playerControllerBody = static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody);
		if (nullptr != playerControllerBody)
		{
			playerControllerBody->setJumpSpeed(jumpSpeed);
		}
	}

	Ogre::Real PhysicsPlayerControllerComponent::getJumpSpeed(void) const
	{
		return this->jumpSpeed->getReal();
	}

	Ogre::Real PhysicsPlayerControllerComponent::getForwardSpeed(void) const
	{
		Ogre::Real forwardSpeed = 0.0f;
		OgreNewt::PlayerControllerBody* playerControllerBody = static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody);
		if (nullptr != playerControllerBody)
		{
			forwardSpeed = playerControllerBody->getForwardSpeed();
		}
		return forwardSpeed;
	}

	Ogre::Real PhysicsPlayerControllerComponent::getSideSpeed(void) const
	{
		Ogre::Real sideSpeed = 0.0f;
		OgreNewt::PlayerControllerBody* playerControllerBody = static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody);
		if (nullptr != playerControllerBody)
		{
			sideSpeed = playerControllerBody->getSideSpeed();
		}
		return sideSpeed;
	}

	Ogre::Real PhysicsPlayerControllerComponent::getHeading(void) const
	{
		Ogre::Radian heading = Ogre::Radian(0.0f);
		OgreNewt::PlayerControllerBody* playerControllerBody = static_cast<OgreNewt::PlayerControllerBody*>(this->physicsBody);
		if (nullptr != playerControllerBody)
		{
			heading = playerControllerBody->getHeading();
		}
		return heading.valueDegrees();
	}

	void PhysicsPlayerControllerComponent::setOnContactFrictionFunctionName(const Ogre::String& onContactFrictionFunctionName)
	{
		this->onContactFrictionFunctionName->setValue(onContactFrictionFunctionName);
		this->onContactFrictionFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), onContactFrictionFunctionName + "(gameObject0, gameObject1, playerContact)");
	}

	Ogre::String PhysicsPlayerControllerComponent::getOnContactFrictionFunctionName(void) const
	{
		return this->onContactFrictionFunctionName->getString();
	}

	void PhysicsPlayerControllerComponent::setOnContactFunctionName(const Ogre::String& onContactFunctionName)
	{
		this->onContactFunctionName->setValue(onContactFunctionName);
		this->onContactFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), onContactFunctionName + "(gameObject0, gameObject1, playerContact)");
	}

	Ogre::String PhysicsPlayerControllerComponent::getOnContactFunctionName(void) const
	{
		return this->onContactFunctionName->getString();
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	PlayerContact::PlayerContact()
		: position(Ogre::Vector3::ZERO),
		normal(Ogre::Vector3::ZERO),
		penetration(0.0f),
		resultFriction(2.0f)
	{
	}

	PlayerContact::~PlayerContact()
	{
	}

	Ogre::Vector3 PlayerContact::getPosition(void) const
	{
		return this->position;
	}

	Ogre::Vector3 PlayerContact::getNormal(void) const
	{
		return this->normal;
	}

	void PlayerContact::setResultFriction(Ogre::Real resultFriction)
	{
		this->resultFriction = resultFriction;
	}

	Ogre::Real PlayerContact::getResultFriction(void) const
	{
		return this->resultFriction;
	}

	Ogre::Real PlayerContact::getPenetration(void) const
	{
		return this->penetration;
	}

}; // namespace end
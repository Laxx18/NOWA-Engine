#include "NOWAPrecompiled.h"
#include "PhysicsTriggerComponent.h"
#include "LuaScriptComponent.h"
#include "PhysicsComponent.h"
#include "modules/LuaScriptApi.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	PhysicsTriggerComponent::PhysicsTriggerCallback::PhysicsTriggerCallback(GameObject* owner, LuaScript* luaScript,
																			luabind::object& enterClosureFunction, luabind::object& insideClosureFunction, luabind::object& leaveClosureFunction)
		: OgreNewt::TriggerCallback(),
		owner(owner),
		luaScript(luaScript),
		bDebugData(false),
		enterClosureFunction(enterClosureFunction),
		insideClosureFunction(insideClosureFunction),
		leaveClosureFunction(leaveClosureFunction),
		onInsideFunctionAvailable(true)
	{
		if (false == this->insideClosureFunction.is_valid())
		{
			this->onInsideFunctionAvailable = false;
		}
	}

	PhysicsTriggerComponent::PhysicsTriggerCallback::~PhysicsTriggerCallback()
	{
		
	}

	void PhysicsTriggerComponent::PhysicsTriggerCallback::OnEnter(const OgreNewt::Body* visitor)
	{
		if (true == this->bDebugData)
		{
			// static int enter = 0;
			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsTriggerComponent] Enter: " + visitor->getOgreNode()->getName() + " " + Ogre::StringConverter::toString(enter++));
		}
		// Note visitor is not the one that has created this trigger, but the one that enters it
		PhysicsComponent* visitorPhysicsComponent = OgreNewt::any_cast<PhysicsComponent*>(visitor->getUserData());
		if (nullptr != visitorPhysicsComponent)
		{
			GameObject* visitorGameObject = visitorPhysicsComponent->getOwner().get();
			// Check for correct category
			unsigned int type = visitorGameObject->getCategoryId();
			unsigned int finalType = type & this->categoryId;
			if (type == finalType)
			{
				if (nullptr != luaScript)
				{
					if (this->enterClosureFunction.is_valid())
					{
						NOWA::AppStateManager::LogicCommand logicCommand = [this, visitorGameObject]()
							{
								try
								{
									luabind::call_function<void>(this->enterClosureFunction, visitorGameObject);
								}
								catch (luabind::error& error)
								{
									luabind::object errorMsg(luabind::from_stack(error.state(), -1));
									std::stringstream msg;
									msg << errorMsg;

									Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[PhysicsTriggerComponent::PhysicsTriggerCallback] Caught error in 'reactOnEnter' Error: " + Ogre::String(error.what())
										+ " details: " + msg.str());
								}
							};
						NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
					}
				}
				/*else
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsTriggerComponent] Cannot enter trigger, because there is no lua script");
				}*/
				// Trigger also an event, that a trigger has been entered
				boost::shared_ptr<EventPhysicsTrigger> eventPhysicsTrigger(new EventPhysicsTrigger(visitorGameObject->getId(), true));
				NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventPhysicsTrigger);
			}
		}
	}

	void PhysicsTriggerComponent::PhysicsTriggerCallback::OnInside(const OgreNewt::Body* visitor)
	{
		// For performance reasons only call lua table function permanentely if the function does exist in a lua script
		if (true == this->onInsideFunctionAvailable)
		{
			PhysicsComponent* physicsComponent = OgreNewt::any_cast<PhysicsComponent*>(visitor->getUserData());
			if (nullptr != physicsComponent)
			{
				GameObjectPtr visitorGameObject = physicsComponent->getOwner();
				// Check for correct category
				unsigned int type = visitorGameObject->getCategoryId();
				unsigned int finalType = type & this->categoryId;
				if (type == finalType)
				{
					if (nullptr != luaScript)
					{
						if (this->insideClosureFunction.is_valid())
						{
							NOWA::AppStateManager::LogicCommand logicCommand = [this, visitorGameObject]()
								{
									try
									{
										luabind::call_function<void>(this->insideClosureFunction, visitorGameObject);
									}
									catch (luabind::error& error)
									{
										luabind::object errorMsg(luabind::from_stack(error.state(), -1));
										std::stringstream msg;
										msg << errorMsg;

										Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[PhysicsTriggerComponent::PhysicsTriggerCallback] Caught error in 'reactOnInside' Error: " + Ogre::String(error.what())
											+ " details: " + msg.str());
									}
								};
							NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
						}
					}
				}
			}
		}
	}

	void PhysicsTriggerComponent::PhysicsTriggerCallback::OnExit(const OgreNewt::Body* visitor)
	{
		if (true == this->bDebugData)
		{
			// static int exit = 0;
			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsTriggerComponent] Exit: " + visitor->getOgreNode()->getName() + " " + Ogre::StringConverter::toString(exit++));
		}
		PhysicsComponent* visitorPhysicsComponent = OgreNewt::any_cast<PhysicsComponent*>(visitor->getUserData());
		if (nullptr != visitorPhysicsComponent)
		{
			GameObject* visitorGameObject = visitorPhysicsComponent->getOwner().get();
			// Check for correct category
			unsigned int type = visitorGameObject->getCategoryId();
			unsigned int finalType = type & this->categoryId;
			if (type == finalType)
			{
				if (nullptr != luaScript)
				{
					if (this->leaveClosureFunction.is_valid())
					{
						NOWA::AppStateManager::LogicCommand logicCommand = [this, visitorGameObject]()
							{
								try
								{
									luabind::call_function<void>(this->leaveClosureFunction, visitorGameObject);
								}
								catch (luabind::error& error)
								{
									luabind::object errorMsg(luabind::from_stack(error.state(), -1));
									std::stringstream msg;
									msg << errorMsg;

									Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[PhysicsTriggerComponent::PhysicsTriggerCallback] Caught error in 'reactOnLeave' Error: " + Ogre::String(error.what())
										+ " details: " + msg.str());
								}
							};
						NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
					}
				}
				// Trigger also an event, that a trigger has been exitted
				boost::shared_ptr<EventPhysicsTrigger> eventPhysicsTrigger(new EventPhysicsTrigger(visitorGameObject->getId(), false));
				NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventPhysicsTrigger);
			}
		}
	}

	void PhysicsTriggerComponent::PhysicsTriggerCallback::setLuaScript(LuaScript* luaScript)
	{
		this->luaScript = luaScript;
		if (false == this->insideClosureFunction.is_valid())
		{
			this->onInsideFunctionAvailable = false;
		}
	}

	void PhysicsTriggerComponent::PhysicsTriggerCallback::setDebugData(bool bDebugData)
	{
		this->bDebugData = bDebugData;
	}

	void PhysicsTriggerComponent::PhysicsTriggerCallback::setCategoryId(unsigned int categoryId)
	{
		this->categoryId = categoryId;
	}

	void PhysicsTriggerComponent::PhysicsTriggerCallback::setTriggerFunctions(luabind::object& enterClosureFunction, luabind::object& insideClosureFunction, luabind::object& leaveClosureFunction)
	{
		this->enterClosureFunction = enterClosureFunction;
		this->insideClosureFunction = insideClosureFunction;
		this->leaveClosureFunction = leaveClosureFunction;

		if (false == this->insideClosureFunction.is_valid())
		{
			this->onInsideFunctionAvailable = false;
		}
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	PhysicsTriggerComponent::PhysicsTriggerComponent()
		: PhysicsActiveComponent(),
		categoriesId(GameObjectController::ALL_CATEGORIES_ID)
	{
		std::vector<Ogre::String> collisionTypes(1);
		collisionTypes[0] = "ConvexHull";
		this->collisionType->setValue(collisionTypes);
		this->categories = new Variant(PhysicsTriggerComponent::AttrCategories(), Ogre::String("All"), this->attributes);

		this->asSoftBody->setVisible(false);
		this->gyroscopicTorque->setValue(false);
		this->gyroscopicTorque->setVisible(false);
		this->collidable->setValue(false);
		this->collidable->setVisible(false);
	}

	PhysicsTriggerComponent::~PhysicsTriggerComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsTriggerComponent] Destructor physics trigger component for game object: " 
			+ this->gameObjectPtr->getName());
	}

	bool PhysicsTriggerComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		PhysicsActiveComponent::parseCommonProperties(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Categories")
		{
			this->categories->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		return true;
	}

	GameObjectCompPtr PhysicsTriggerComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		PhysicsTriggerCompPtr clonedCompPtr(boost::make_shared<PhysicsTriggerComponent>());

		
		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setMass(this->mass->getReal());
		clonedCompPtr->setMassOrigin(this->massOrigin->getVector3());
		clonedCompPtr->setLinearDamping(this->linearDamping->getReal());
		clonedCompPtr->setAngularDamping(this->angularDamping->getVector3());
		clonedCompPtr->setGravity(this->gravity->getVector3());
		clonedCompPtr->setGravitySourceCategory(this->gravitySourceCategory->getString());
		// clonedCompPtr->applyAngularImpulse(this->angularImpulse);
		// clonedCompPtr->applyAngularVelocity(this->angularVelocity->getVector3());
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

		clonedCompPtr->setCategories(this->categories->getString());
		
		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool PhysicsTriggerComponent::postInit(void)
	{
		bool success = PhysicsComponent::postInit();
		
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsTriggerComponent] Init physics trigger component for game object: " + this->gameObjectPtr->getName());

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

	bool PhysicsTriggerComponent::connect(void)
	{
		bool success = PhysicsActiveComponent::connect();

		this->setCategories(this->categories->getString());
		
		// Depending on order, in postInit maybe there is no valid lua script yet, so set it here again
		if (nullptr != this->physicsBody)
		{
			auto triggerCallback = static_cast<OgreNewt::TriggerBody*>(this->physicsBody)->getTriggerCallback();
			static_cast<PhysicsTriggerComponent::PhysicsTriggerCallback*>(triggerCallback)->setTriggerFunctions(this->enterClosureFunction,
				this->insideClosureFunction, this->leaveClosureFunction);
			static_cast<PhysicsTriggerComponent::PhysicsTriggerCallback*>(triggerCallback)->setLuaScript(this->gameObjectPtr->getLuaScript());
			static_cast<PhysicsTriggerComponent::PhysicsTriggerCallback*>(triggerCallback)->setCategoryId(this->categoriesId);
		}

		return success;
	}

	void PhysicsTriggerComponent::update(Ogre::Real dt, bool notSimulating)
	{
		static_cast<OgreNewt::TriggerBody*>(this->physicsBody)->integrateVelocity(dt);
	}

	void PhysicsTriggerComponent::actualizeValue(Variant* attribute)
	{
		PhysicsActiveComponent::actualizeCommonValue(attribute);

		if (PhysicsTriggerComponent::AttrCategories() == attribute->getName())
		{
			this->setCategories(attribute->getString());
		}
	}

	void PhysicsTriggerComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Categories"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->categories->getString())));
		propertiesXML->append_node(propertyXML);
	}

	void PhysicsTriggerComponent::showDebugData(void)
	{
		GameObjectComponent::showDebugData();
		if (nullptr != this->physicsBody)
		{
			ENQUEUE_RENDER_COMMAND_WAIT("PhysicsRagDollComponent::internalShowDebugData",
			{
				this->physicsBody->showDebugCollision(!this->gameObjectPtr->isDynamic(), this->bShowDebugData);
			});
			auto triggerCallback = static_cast<OgreNewt::TriggerBody*>(this->physicsBody)->getTriggerCallback();
			if (nullptr != triggerCallback)
			{
				static_cast<PhysicsTriggerComponent::PhysicsTriggerCallback*>(triggerCallback)->setDebugData(bShowDebugData);
			}
		}
	}

	Ogre::String PhysicsTriggerComponent::getClassName(void) const
	{
		return "PhysicsTriggerComponent";
	}

	Ogre::String PhysicsTriggerComponent::getParentClassName(void) const
	{
		return "PhysicsComponent";
	}

	bool PhysicsTriggerComponent::createDynamicBody(void)
	{
		Ogre::Vector3 inertia = Ogre::Vector3(1.0f, 1.0f, 1.0f);

		Ogre::Quaternion collisionOrientation = Ogre::Quaternion::IDENTITY; // this->gameObjectPtr->getSceneNode()->getOrientation();
		if (Ogre::Vector3::ZERO != this->collisionDirection->getVector3())
		{
			collisionOrientation = MathHelper::getInstance()->degreesToQuat(this->collisionDirection->getVector3());
		}

		Ogre::Vector3 calculatedMassOrigin = Ogre::Vector3::ZERO;

		OgreNewt::CollisionPtr collisionPtr;
		if (this->collisionType->getListSelectedValue() == "Tree")
		{
			Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
			if (nullptr != entity)
			{
				collisionPtr = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::TreeCollision(this->ogreNewt, entity, true, this->gameObjectPtr->getCategoryId()));
			}
			else
			{
				Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
				if (nullptr != item)
				{
					collisionPtr = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::TreeCollision(this->ogreNewt, item, true, this->gameObjectPtr->getCategoryId()));
				}
			}
		}
		else
		{
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("PhysicsTriggerComponent::createDynamicCollision", _4(&inertia, &collisionPtr, collisionOrientation, &calculatedMassOrigin),
			{
				collisionPtr = this->createDynamicCollision(inertia, this->collisionSize->getVector3(), this->collisionPosition->getVector3(),
					collisionOrientation, calculatedMassOrigin, this->gameObjectPtr->getCategoryId());
			});
		}

		if (nullptr == this->physicsBody)
		{
			this->physicsBody = new OgreNewt::TriggerBody(this->ogreNewt, this->gameObjectPtr->getSceneManager(), collisionPtr,
				new PhysicsTriggerCallback(this->gameObjectPtr.get(), this->gameObjectPtr->getLuaScript(),
														  this->enterClosureFunction,
														  this->insideClosureFunction, this->leaveClosureFunction));
		}
		else
		{
			// Just re-create the trigger (internally the newton body will fortunately not change, so also this physics body remains the same, which avoids lots of problems, 
			// when used in combination with joint, undo, redo :)
			static_cast<OgreNewt::TriggerBody*>(this->physicsBody)->reCreateTrigger(collisionPtr);
		}

		this->physicsBody->setGravity(this->gravity->getVector3());

		Ogre::Real weightedMass = this->mass->getReal(); /** scale.x * scale.y * scale.z;*/ // scale is not used anymore, because if big game objects are scaled down, the mass is to low!

		if (this->collisionType->getListSelectedValue() == "ConvexHull")
		{
			this->physicsBody->setConvexIntertialMatrix(inertia, calculatedMassOrigin);
		}

		// Apply mass and scale to inertia (the bigger the object, the more mass)
		inertia *= weightedMass;
		this->physicsBody->setMassMatrix(weightedMass, inertia);

		if (Ogre::Vector3::ZERO != this->massOrigin->getVector3())
		{
			calculatedMassOrigin = this->massOrigin->getVector3();
		}

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

		// Note: Kinematic Body has not force and torque callback!
		// this->physicsBody->setCustomForceAndTorqueCallback<PhysicsActiveKinematicComponent>(&PhysicsActiveComponent::moveCallback, this);

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

		const auto materialId = AppStateManager::getSingletonPtr()->getGameObjectController()->getMaterialID(this->gameObjectPtr.get(), this->ogreNewt);
		this->physicsBody->setMaterialGroupID(materialId);

		return true;
	}

	void PhysicsTriggerComponent::reCreateCollision(bool overwrite)
	{
		this->createDynamicBody();
	}

	void PhysicsTriggerComponent::setCategories(const Ogre::String& categories)
	{
		this->categories->setValue(categories);
		this->categoriesId = AppStateManager::getSingletonPtr()->getGameObjectController()->generateCategoryId(categories);
	}

	Ogre::String PhysicsTriggerComponent::getCategories(void) const
	{
		return this->categories->getString();
	}

	void PhysicsTriggerComponent::reactOnEnter(luabind::object closureFunction)
	{
		this->enterClosureFunction = closureFunction;

		if (nullptr != this->physicsBody)
		{
			auto triggerCallback = static_cast<OgreNewt::TriggerBody*>(this->physicsBody)->getTriggerCallback();
			static_cast<PhysicsTriggerComponent::PhysicsTriggerCallback*>(triggerCallback)->setTriggerFunctions(this->enterClosureFunction,
				this->insideClosureFunction, this->leaveClosureFunction);
			static_cast<PhysicsTriggerComponent::PhysicsTriggerCallback*>(triggerCallback)->setLuaScript(this->gameObjectPtr->getLuaScript());
			static_cast<PhysicsTriggerComponent::PhysicsTriggerCallback*>(triggerCallback)->setCategoryId(this->categoriesId);
		}
	}

	void PhysicsTriggerComponent::reactOnInside(luabind::object closureFunction)
	{
		this->insideClosureFunction = closureFunction;

		if (nullptr != this->physicsBody)
		{
			auto triggerCallback = static_cast<OgreNewt::TriggerBody*>(this->physicsBody)->getTriggerCallback();
			static_cast<PhysicsTriggerComponent::PhysicsTriggerCallback*>(triggerCallback)->setTriggerFunctions(this->enterClosureFunction,
				this->insideClosureFunction, this->leaveClosureFunction);
			static_cast<PhysicsTriggerComponent::PhysicsTriggerCallback*>(triggerCallback)->setLuaScript(this->gameObjectPtr->getLuaScript());
			static_cast<PhysicsTriggerComponent::PhysicsTriggerCallback*>(triggerCallback)->setCategoryId(this->categoriesId);
		}
	}

	void PhysicsTriggerComponent::reactOnLeave(luabind::object closureFunction)
	{
		this->leaveClosureFunction = closureFunction;

		if (nullptr != this->physicsBody)
		{
			auto triggerCallback = static_cast<OgreNewt::TriggerBody*>(this->physicsBody)->getTriggerCallback();
			static_cast<PhysicsTriggerComponent::PhysicsTriggerCallback*>(triggerCallback)->setTriggerFunctions(this->enterClosureFunction,
				this->insideClosureFunction, this->leaveClosureFunction);
			static_cast<PhysicsTriggerComponent::PhysicsTriggerCallback*>(triggerCallback)->setLuaScript(this->gameObjectPtr->getLuaScript());
			static_cast<PhysicsTriggerComponent::PhysicsTriggerCallback*>(triggerCallback)->setCategoryId(this->categoriesId);
		}
	}

}; // namespace end
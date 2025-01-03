#include "NOWAPrecompiled.h"
#include "PhysicsBuoyancyComponent.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "LuaScriptComponent.h"
#include "modules/LuaScriptModule.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	PhysicsBuoyancyComponent::PhysicsBuoyancyTriggerCallback::PhysicsBuoyancyTriggerCallback(GameObject* owner, Ogre::Plane& fluidPlane, Ogre::Real waterToSolidVolumeRatio,
		Ogre::Real viscosity, const Ogre::Vector3& gravity, Ogre::Real offsetHeight, LuaScript* luaScript,
																							 luabind::object& enterClosureFunction, luabind::object& insideClosureFunction, luabind::object& leaveClosureFunction)
		: OgreNewt::BuoyancyForceTriggerCallback(fluidPlane, waterToSolidVolumeRatio, viscosity, gravity, offsetHeight),
		owner(owner),
		luaScript(luaScript),
		enterClosureFunction(enterClosureFunction),
		insideClosureFunction(insideClosureFunction),
		leaveClosureFunction(leaveClosureFunction)
	{
		if (false == this->insideClosureFunction.is_valid())
		{
			this->onInsideFunctionAvailable = false;
		}
	}

	PhysicsBuoyancyComponent::PhysicsBuoyancyTriggerCallback::~PhysicsBuoyancyTriggerCallback()
	{
		
	}

	void PhysicsBuoyancyComponent::PhysicsBuoyancyTriggerCallback::OnEnter(const OgreNewt::Body* visitor)
	{
		OgreNewt::BuoyancyForceTriggerCallback::OnEnter(visitor);
		// Note visitor is not the one that has created this trigger, but the one that enters it
		PhysicsComponent* visitorPhysicsComponent = OgreNewt::any_cast<PhysicsComponent*>(visitor->getUserData());
		if (nullptr != visitorPhysicsComponent)
		{
			GameObjectPtr visitorGameObjectPtr = visitorPhysicsComponent->getOwner();
			
			if (nullptr != luaScript)
			{
				if (this->enterClosureFunction.is_valid())
				{
					try
					{
						luabind::call_function<void>(this->enterClosureFunction, visitorGameObjectPtr.get());
					}
					catch (luabind::error& error)
					{
						luabind::object errorMsg(luabind::from_stack(error.state(), -1));
						std::stringstream msg;
						msg << errorMsg;

						Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnEnter' Error: " + Ogre::String(error.what())
																	+ " details: " + msg.str());
					}
				}
			}
		}
	}

	void PhysicsBuoyancyComponent::PhysicsBuoyancyTriggerCallback::OnInside(const OgreNewt::Body* visitor)
	{
		OgreNewt::BuoyancyForceTriggerCallback::OnInside(visitor);
		// Only call this permanentely when in the lua script those function has been implemented
		if (true == this->onInsideFunctionAvailable)
		{
			PhysicsComponent* visitorPhysicsComponent = OgreNewt::any_cast<PhysicsComponent*>(visitor->getUserData());
			if (nullptr != visitorPhysicsComponent)
			{
				GameObjectPtr visitorGameObjectPtr = visitorPhysicsComponent->getOwner();

				if (nullptr != luaScript)
				{
					if (this->enterClosureFunction.is_valid())
					{
						try
						{
							luabind::call_function<void>(this->enterClosureFunction, visitorGameObjectPtr.get());
						}
						catch (luabind::error& error)
						{
							luabind::object errorMsg(luabind::from_stack(error.state(), -1));
							std::stringstream msg;
							msg << errorMsg;

							Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnInside' Error: " + Ogre::String(error.what())
																		+ " details: " + msg.str());
						}
					}
				}
			}
		}
	}

	void PhysicsBuoyancyComponent::PhysicsBuoyancyTriggerCallback::OnExit(const OgreNewt::Body* visitor)
	{
		OgreNewt::BuoyancyForceTriggerCallback::OnExit(visitor);
		PhysicsComponent* visitorPhysicsComponent = OgreNewt::any_cast<PhysicsComponent*>(visitor->getUserData());
		if (nullptr != visitorPhysicsComponent)
		{
			GameObjectPtr visitorGameObjectPtr = visitorPhysicsComponent->getOwner();

			if (nullptr != luaScript)
			{
				if (this->enterClosureFunction.is_valid())
				{
					try
					{
						luabind::call_function<void>(this->enterClosureFunction, visitorGameObjectPtr.get());
					}
					catch (luabind::error& error)
					{
						luabind::object errorMsg(luabind::from_stack(error.state(), -1));
						std::stringstream msg;
						msg << errorMsg;

						Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnLeave' Error: " + Ogre::String(error.what())
																	+ " details: " + msg.str());
					}
				}
			}
		}
	}

	void PhysicsBuoyancyComponent::PhysicsBuoyancyTriggerCallback::setLuaScript(LuaScript* luaScript)
	{
		this->luaScript = luaScript;
		if (false == this->insideClosureFunction.is_valid())
		{
			this->onInsideFunctionAvailable = false;
		}
	}

	void PhysicsBuoyancyComponent::PhysicsBuoyancyTriggerCallback::setTriggerFunctions(luabind::object& enterClosureFunction, luabind::object& insideClosureFunction, luabind::object& leaveClosureFunction)
	{
		this->enterClosureFunction = enterClosureFunction;
		this->insideClosureFunction = insideClosureFunction;
		this->leaveClosureFunction = leaveClosureFunction;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	PhysicsBuoyancyComponent::PhysicsBuoyancyComponent()
		: PhysicsComponent(),
		waterToSolidVolumeRatio(new Variant(PhysicsBuoyancyComponent::AttrWaterToSolidVolumeRatio(), 0.9f, this->attributes)),
		viscosity(new Variant(PhysicsBuoyancyComponent::AttrViscosity(), 0.995f, this->attributes)),
		buoyancyGravity(new Variant(PhysicsBuoyancyComponent::AttrBuoyancyGravity(), Ogre::Vector3(0.0f, -15.0f, 0.0f), this->attributes)),
		offsetHeight(new Variant(PhysicsBuoyancyComponent::AttrOffsetHeight(), 6, this->attributes)),
		physicsBuoyancyTriggerCallback(nullptr),
		newleyCreated(true)
	{
		std::vector<Ogre::String> collisionTypes(1);
		collisionTypes[0] = "ConvexHull";
		this->collisionType->setValue(collisionTypes);
		this->collisionType->setVisible(false);
		this->mass->setVisible(false);
	}

	PhysicsBuoyancyComponent::~PhysicsBuoyancyComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsBuoyancyComponent] Destructor physics buoyancy component for game object: " + this->gameObjectPtr->getName());

		this->physicsBuoyancyTriggerCallback = nullptr;
	}

	bool PhysicsBuoyancyComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CollisionType")
		{
			this->collisionType->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", ""));
			// For artifact component only tree collision hull is possible
			// this->collisionType->setReadOnly(true);
			// Do not double the propagation, in gameobjectfactory there is also a propagation
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WaterToSolidVolumeRatio")
		{
			this->waterToSolidVolumeRatio->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Viscosity")
		{
			this->viscosity->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BuoyancyGravity")
		{
			this->buoyancyGravity->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OffsetHeight")
		{
			this->offsetHeight->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		this->newleyCreated = false;
		return true;
	}

	GameObjectCompPtr PhysicsBuoyancyComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		PhysicsBuoyancyCompPtr clonedCompPtr(boost::make_shared<PhysicsBuoyancyComponent>());

		
		clonedCompPtr->setCollisionType(this->collisionType->getListSelectedValue());
		
		clonedCompPtr->setWaterToSolidVolumeRatio(this->waterToSolidVolumeRatio->getReal());
		clonedCompPtr->setViscosity(this->viscosity->getReal());
		clonedCompPtr->setBuoyancyGravity(this->buoyancyGravity->getVector3());
		clonedCompPtr->setOffsetHeight(this->offsetHeight->getReal());
		// Bug in newton, setting afterwards collidable to true, will not work, hence do not clone this property
		// clonedCompPtr->setCollidable(this->collidable->getBool());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;

		// return boost::make_shared<PhysicsArtifactComponent>(*this);
	}

	bool PhysicsBuoyancyComponent::postInit(void)
	{
		bool success = PhysicsComponent::postInit();

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsBuoyancyComponent] Init physics buoyancy component for game object: " + this->gameObjectPtr->getName());

		if (!this->createStaticBody())
		{
			return false;
		}

		this->physicsBody->setType(gameObjectPtr->getCategoryId());
		this->physicsBody->setMaterialGroupID(AppStateManager::getSingletonPtr()->getGameObjectController()->getMaterialID(this->gameObjectPtr.get(), this->ogreNewt));

		if (true == this->newleyCreated)
		{
			this->offsetHeight->setValue(this->gameObjectPtr->getPosition().y + this->gameObjectPtr->getSize().y);
			this->newleyCreated = false;
		}

		return success;
	}

	bool PhysicsBuoyancyComponent::connect(void)
	{
		// Depending on order, in postInit maybe there is no valid lua script yet, so set it here again
		if (nullptr != this->physicsBody)
		{
			auto triggerCallback = static_cast<OgreNewt::BuoyancyBody*>(this->physicsBody)->getTriggerCallback();
			static_cast<PhysicsBuoyancyComponent::PhysicsBuoyancyTriggerCallback*>(triggerCallback)->setTriggerFunctions(this->enterClosureFunction,																							 this->insideClosureFunction, this->leaveClosureFunction);
			static_cast<PhysicsBuoyancyComponent::PhysicsBuoyancyTriggerCallback*>(triggerCallback)->setLuaScript(this->gameObjectPtr->getLuaScript());
		}
		
		return true;
	}

	Ogre::String PhysicsBuoyancyComponent::getClassName(void) const
	{
		return "PhysicsBuoyancyComponent";
	}

	Ogre::String PhysicsBuoyancyComponent::getParentClassName(void) const
	{
		return "PhysicsComponent";
	}

	void PhysicsBuoyancyComponent::showDebugData(void)
	{
		GameObjectComponent::showDebugData();
		if (nullptr != this->physicsBody)
		{
			this->physicsBody->showDebugCollision(false, this->bShowDebugData);
		}
	}
	
	bool PhysicsBuoyancyComponent::createStaticBody(void)
	{
		this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
		this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
		this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();

		Ogre::String meshName;
		// Collision for static objects
		OgreNewt::CollisionPtr collision;

		Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
		if (nullptr != entity)
		{
			meshName = entity->getMesh()->getName();
			if (Ogre::StringUtil::match(meshName, "Plane*", true))
			{
				Ogre::Vector3 size = entity->getMesh()->getBounds().getSize() * this->initialScale;
				size.y = 0.001f;
				collision = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::Box(this->ogreNewt, size, this->gameObjectPtr->getCategoryId(), Ogre::Quaternion::IDENTITY, Ogre::Vector3::ZERO));
			}
			else
			{
				// Note: Even if its a static body, collision hull must be convex hull and not tree collision! Because e.g. if a bottle is used, it is hollow inside so tree would not
				// fill the hollow and buoyancy just work on the border and maybe depending on what entity type, other objects would collide
				collision = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::ConvexHull(this->ogreNewt, entity, this->gameObjectPtr->getCategoryId()));
			}
		}
		else
		{
			Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
			if (nullptr != item)
			{
				meshName = item->getMesh()->getName();
				if (Ogre::StringUtil::match(meshName, "Plane*", true))
				{
					Ogre::Vector3 size = item->getMesh()->getAabb().getSize() * this->initialScale;
					size.y = 0.001f;
					collision = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::Box(this->ogreNewt, size, this->gameObjectPtr->getCategoryId(), Ogre::Quaternion::IDENTITY, Ogre::Vector3::ZERO));
				}
				else
				{
					// Note: Even if its a static body, collision hull must be convex hull and not tree collision! Because e.g. if a bottle is used, it is hollow inside so tree would not
					// fill the hollow and buoyancy just work on the border and maybe depending on what item type, other objects would collide
					collision = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::ConvexHull(this->ogreNewt, item, this->gameObjectPtr->getCategoryId()));
				}
			}
			else
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsTriggerComponent] Error cannot create static body, because the game object has no entity/item with mesh for game object: " + this->gameObjectPtr->getName());
				return false;
			}
		}

		if (nullptr == collision)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsTriggerComponent] Could not create collision file for game object: "
				+ this->gameObjectPtr->getName() + " and mesh: " + meshName + ". Maybe the mesh is corrupt.");
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[PhysicsTriggerComponent] Could not create collision file for game object: "
				+ this->gameObjectPtr->getName() + " and mesh: " + meshName + ". Maybe the mesh is corrupt.\n", "NOWA");
		}

		if (nullptr == this->physicsBuoyancyTriggerCallback)
		{
			this->physicsBuoyancyTriggerCallback = new PhysicsBuoyancyComponent::PhysicsBuoyancyTriggerCallback(this->gameObjectPtr.get(),
				Ogre::Plane(Ogre::Vector3(0.0f, 1.0f, 0.0f), this->offsetHeight->getReal()), // - because internally when plane is created the value is inverted
				this->waterToSolidVolumeRatio->getReal(), this->viscosity->getReal(), this->buoyancyGravity->getVector3(), this->offsetHeight->getReal(), this->gameObjectPtr->getLuaScript(),
																												this->enterClosureFunction,
																												this->insideClosureFunction, this->leaveClosureFunction);
		}

		if (nullptr == this->physicsBody)
		{
			this->physicsBody = new OgreNewt::BuoyancyBody(this->ogreNewt, this->gameObjectPtr->getSceneManager(), collision,
				this->physicsBuoyancyTriggerCallback);
		}
		else
		{
			// Just re-create the trigger (internally the newton body will fortunately not change, so also this physics body remains the same, which avoids lots of problems, 
			// when used in combinatin with joint, undo, redo :)
			static_cast<OgreNewt::BuoyancyBody*>(this->physicsBody)->reCreateTrigger(collision);
		}

		// Set mass to 0 = infinity = static
		this->physicsBody->setMassMatrix(0.0f, Ogre::Vector3::ZERO);

		this->physicsBody->setUserData(OgreNewt::Any(dynamic_cast<PhysicsComponent*>(this)));

		this->physicsBody->attachNode(this->gameObjectPtr->getSceneNode());

		this->setPosition(this->initialPosition);
		this->setOrientation(this->initialOrientation);

		// Artifact body should always sleep?
		// this->physicsBody->setAutoSleep(1);

		// Object will not be moved frequently
		// this->gameObjectPtr->setDynamic(false); // Am I shit? Let the designer choose, because else an object with artifact collision can be no more moved!

		return true;
	}

	void PhysicsBuoyancyComponent::reCreateCollision(bool overwrite)
	{
		if (nullptr != this->physicsBody)
		{
			this->destroyCollision();
			this->ogreNewt->RemoveSceneCollision(this->physicsBody->getNewtonCollision());
		}

		this->createStaticBody();
	}

	void PhysicsBuoyancyComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			this->physicsBuoyancyTriggerCallback->update(dt);
		}
	}

	void PhysicsBuoyancyComponent::actualizeValue(Variant* attribute)
	{
		PhysicsComponent::actualizeValue(attribute);

		if (PhysicsBuoyancyComponent::AttrWaterToSolidVolumeRatio() == attribute->getName())
		{
			this->setWaterToSolidVolumeRatio(attribute->getReal());
		}
		else if (PhysicsBuoyancyComponent::AttrViscosity() == attribute->getName())
		{
			this->setViscosity(attribute->getReal());
		}
		else if (PhysicsBuoyancyComponent::AttrBuoyancyGravity() == attribute->getName())
		{
			this->setBuoyancyGravity(attribute->getVector3());
		}
		else if (PhysicsBuoyancyComponent::AttrOffsetHeight() == attribute->getName())
		{
			this->setOffsetHeight(attribute->getReal());
		}
	}

	void PhysicsBuoyancyComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		GameObjectComponent::writeXML(propertiesXML, doc);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CollisionType"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->collisionType->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "WaterToSolidVolumeRatio"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->waterToSolidVolumeRatio->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Viscosity"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->viscosity->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BuoyancyGravity"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->buoyancyGravity->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OffsetHeight"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->offsetHeight->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	void PhysicsBuoyancyComponent::setWaterToSolidVolumeRatio(Ogre::Real waterToSolidVolumeRatio)
	{
		this->waterToSolidVolumeRatio->setValue(waterToSolidVolumeRatio);
		if (nullptr != this->physicsBuoyancyTriggerCallback)
		{
			this->physicsBuoyancyTriggerCallback->setWaterToSolidVolumeRatio(waterToSolidVolumeRatio);
		}
	}

	Ogre::Real PhysicsBuoyancyComponent::getWaterToSolidVolumeRatio(void) const
	{
		return this->waterToSolidVolumeRatio->getReal();
	}

	void PhysicsBuoyancyComponent::setViscosity(Ogre::Real viscosity)
	{
		this->viscosity->setValue(viscosity);
		if (nullptr != this->physicsBuoyancyTriggerCallback)
		{
			this->physicsBuoyancyTriggerCallback->setViscosity(viscosity);
		}
	}

	Ogre::Real PhysicsBuoyancyComponent::getViscosity(void) const
	{
		return this->viscosity->getReal();
	}

	void PhysicsBuoyancyComponent::setBuoyancyGravity(const Ogre::Vector3& buoyancyGravity)
	{
		this->buoyancyGravity->setValue(buoyancyGravity);
		if (nullptr != this->physicsBuoyancyTriggerCallback)
		{
			this->physicsBuoyancyTriggerCallback->setGravity(buoyancyGravity);
		}
	}

	const Ogre::Vector3 PhysicsBuoyancyComponent::getBuoyancyGravity(void) const
	{
		return this->buoyancyGravity->getVector3();
	}

	void PhysicsBuoyancyComponent::setOffsetHeight(Ogre::Real offsetHeight)
	{
		this->offsetHeight->setValue(offsetHeight);
		if (nullptr != this->physicsBuoyancyTriggerCallback)
		{
			// this->physicsBuoyancyTriggerCallback->setFluidPlane(Ogre::Plane(Ogre::Vector3(0.0f, 1.0f, 0.0f), this->offsetHeight->getReal())); // - because internally when plane is created the value is inverted
			this->physicsBuoyancyTriggerCallback->setWaterSurfaceRestHeight(this->offsetHeight->getReal());
		}
	}

	Ogre::Real PhysicsBuoyancyComponent::getOffsetHeight(void) const
	{
		return this->offsetHeight->getReal();
	}

	void PhysicsBuoyancyComponent::reactOnEnter(luabind::object closureFunction)
	{
		this->enterClosureFunction = closureFunction;
	}

	void PhysicsBuoyancyComponent::reactOnInside(luabind::object closureFunction)
	{
		this->insideClosureFunction = closureFunction;
	}

	void PhysicsBuoyancyComponent::reactOnLeave(luabind::object closureFunction)
	{
		this->leaveClosureFunction = closureFunction;
	}

}; // namespace end
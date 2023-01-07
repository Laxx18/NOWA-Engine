#include "NOWAPrecompiled.h"
#include "PhysicsTriggerComponent.h"
#include "LuaScriptComponent.h"
#include "PhysicsComponent.h"
#include "modules/OgreNewtModule.h"
#include "modules/LuaScriptModule.h"
#include "GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "main/EventManager.h"
#include "tinyxml.h"

namespace NOWA
{
	using namespace rapidxml;

	PhysicsTriggerComponent::PhysicsTriggerCallback::PhysicsTriggerCallback(GameObject* owner, LuaScript* luaScript)
		: OgreNewt::TriggerCallback(),
		owner(owner),
		luaScript(luaScript),
		bDebugData(false)
	{
		if (nullptr != luaScript)
		{
			this->onInsideFunctionAvailable = LuaScriptModule::getInstance()->checkLuaFunctionAvailable(luaScript->getName(), "onTriggerInside");
		}
	}

	PhysicsTriggerComponent::PhysicsTriggerCallback::~PhysicsTriggerCallback()
	{
		
	}

	void PhysicsTriggerComponent::PhysicsTriggerCallback::OnEnter(const OgreNewt::Body* visitor)
	{
		if (true == this->bDebugData)
		{
			static int enter = 0;
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsTriggerComponent] Enter: " + visitor->getOgreNode()->getName() + " " + Ogre::StringConverter::toString(enter++));
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
					luaScript->callTableFunction("onTriggerEnter", visitorGameObject);
				}
				/*else
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsTriggerComponent] Cannot enter trigger, because there is no lua script");
				}*/
				// Trigger also an event, that a trigger has been entered
				boost::shared_ptr<EventPhysicsTrigger> eventPhysicsTrigger(new EventPhysicsTrigger(visitorGameObject->getId(), true));
				NOWA::EventManager::getInstance()->queueEvent(eventPhysicsTrigger);
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
						luaScript->callTableFunction("onTriggerInside", visitorGameObject);
					}
				}
			}
		}
	}

	void PhysicsTriggerComponent::PhysicsTriggerCallback::OnExit(const OgreNewt::Body* visitor)
	{
		if (true == this->bDebugData)
		{
			static int exit = 0;
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsTriggerComponent] Exit: " + visitor->getOgreNode()->getName() + " " + Ogre::StringConverter::toString(exit++));
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
					luaScript->callTableFunction("onTriggerLeave", visitorGameObject);
				}
				// Trigger also an event, that a trigger has been exitted
				boost::shared_ptr<EventPhysicsTrigger> eventPhysicsTrigger(new EventPhysicsTrigger(visitorGameObject->getId(), false));
				NOWA::EventManager::getInstance()->queueEvent(eventPhysicsTrigger);
			}
		}
	}

	void PhysicsTriggerComponent::PhysicsTriggerCallback::setLuaScript(LuaScript* luaScript)
	{
		this->luaScript = luaScript;
		if (nullptr != this->luaScript)
		{
			this->onInsideFunctionAvailable = LuaScriptModule::getInstance()->checkLuaFunctionAvailable(luaScript->getName(), "onTriggerInside");
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
	}

	PhysicsTriggerComponent::~PhysicsTriggerComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsTriggerComponent] Destructor physics trigger component for game object: " 
			+ this->gameObjectPtr->getName());
	}

	bool PhysicsTriggerComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CollisionType")
		{
			this->collisionType->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", ""));
			// For artifact component only tree collision hull is possible
			// this->collisionType->setReadOnly(true);
			// Do not double the propagation, in gameobjectfactory there is also a propagation
			propertyElement = propertyElement->next_sibling("property");
		}
		else
		{
			return false;
		}
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

		clonedCompPtr->setCollisionType(this->collisionType->getListSelectedValue());
		clonedCompPtr->setCategories(this->categories->getString());
		
		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		return clonedCompPtr;
	}

	bool PhysicsTriggerComponent::postInit(void)
	{
		bool success = PhysicsComponent::postInit();
		
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsTriggerComponent] Init physics trigger component for game object: " + this->gameObjectPtr->getName());

		if (false == this->createStaticBody())
		{
			return false;
		}

		this->physicsBody->setType(gameObjectPtr->getCategoryId());
		this->physicsBody->setMaterialGroupID(GameObjectController::getInstance()->getMaterialID(this->gameObjectPtr.get(), this->ogreNewt));

		return success;
	}

	bool PhysicsTriggerComponent::connect(void)
	{
		this->setCategories(this->categories->getString());
		
		// Depending on order, in postInit maybe there is no valid lua script yet, so set it here again
		if (nullptr != this->physicsBody)
		{
			auto triggerCallback = static_cast<OgreNewt::TriggerBody*>(this->physicsBody)->getTriggerCallback();
			static_cast<PhysicsTriggerComponent::PhysicsTriggerCallback*>(triggerCallback)->setLuaScript(this->gameObjectPtr->getLuaScript());
			static_cast<PhysicsTriggerComponent::PhysicsTriggerCallback*>(triggerCallback)->setCategoryId(this->categoriesId);
		}

		return true;
	}

	void PhysicsTriggerComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (PhysicsTriggerComponent::AttrCategories() == attribute->getName())
		{
			this->setCategories(attribute->getString());
		}
	}

	void PhysicsTriggerComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		GameObjectComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CollisionType"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->collisionType->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Categories"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->categories->getString())));
		propertiesXML->append_node(propertyXML);
	}

	void PhysicsTriggerComponent::showDebugData(void)
	{
		GameObjectComponent::showDebugData();
		if (nullptr != this->physicsBody)
		{
			this->physicsBody->showDebugCollision(!this->gameObjectPtr->isDynamic(), bShowDebugData);
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
	
	bool PhysicsTriggerComponent::createStaticBody(void)
	{
		this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
		this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
		this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();
		Ogre::String meshName;

		// Collision for static objects
		OgreNewt::CollisionPtr staticCollision;

		Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
		if (nullptr != entity)
		{
			meshName = entity->getMesh()->getName();
			if (Ogre::StringUtil::match(meshName, "Plane*", true))
			{
				// if the mesh name is a plane, the tree collision does not work, so use box
	// Attention: Is this correct?
				Ogre::Vector3 size = entity->getMesh()->getBounds().getSize() * this->initialScale;
				size.y = 0.001f;
				staticCollision = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::Box(this->ogreNewt, size, 0, Ogre::Quaternion::IDENTITY, Ogre::Vector3::ZERO));
			}
			else
			{
				// Attention: For trigger ConvexHull must be used and no tree collision!! Else it will corrupt the trigger (permanentely calling enter/exit and collision detection)
				// See: http://newtondynamics.com/forum/viewtopic.php?f=12&t=9586&p=64900#p64900
				staticCollision = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::ConvexHull(this->ogreNewt, entity, 0));
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
					// if the mesh name is a plane, the tree collision does not work, so use box
		// Attention: Is this correct?
					Ogre::Vector3 size = item->getMesh()->getAabb().getSize() * this->initialScale;
					size.y = 0.001f;
					staticCollision = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::Box(this->ogreNewt, size, 0, Ogre::Quaternion::IDENTITY, Ogre::Vector3::ZERO));
				}
				else
				{
					// Attention: For trigger ConvexHull must be used and no tree collision!! Else it will corrupt the trigger (permanentely calling enter/exit and collision detection)
					// See: http://newtondynamics.com/forum/viewtopic.php?f=12&t=9586&p=64900#p64900
					staticCollision = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::ConvexHull(this->ogreNewt, item, 0));
				}
			}
			else
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsTriggerComponent] Error cannot create static body, because the game object has no entity/item with mesh for game object: " + this->gameObjectPtr->getName());
				return false;

			}
		}

		if (nullptr == staticCollision)
		{
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[PhysicsTriggerComponent] Could not create collision file for game object: "
				+ this->gameObjectPtr->getName() + " and mesh: " + meshName + ". Maybe the mesh is corrupt.\n", "NOWA");
		}

		if (nullptr == this->physicsBody)
		{
			this->physicsBody = new OgreNewt::TriggerBody(this->ogreNewt, this->gameObjectPtr->getSceneManager(), staticCollision,
				new PhysicsTriggerCallback(this->gameObjectPtr.get(), this->gameObjectPtr->getLuaScript()));
		}
		else
		{
			// Just re-create the trigger (internally the newton body will fortunately not change, so also this physics body remains the same, which avoids lots of problems, 
			// when used in combination with joint, undo, redo :)
			static_cast<OgreNewt::TriggerBody*>(this->physicsBody)->reCreateTrigger(staticCollision);
		}

		// Set mass to 0 = infinity = static
		// this->physicsBody->setMassMatrix(0.0f, Ogre::Vector3::ZERO);

		this->physicsBody->setUserData(OgreNewt::Any(dynamic_cast<PhysicsComponent*>(this)));

		this->physicsBody->attachNode(this->gameObjectPtr->getSceneNode());

		this->physicsBody->setPositionOrientation(this->initialPosition, this->initialOrientation);

		// Artifact body should always sleep?
		this->physicsBody->setAutoSleep(1);

		// Object will not be moved frequently
		// this->gameObjectPtr->setDynamic(false); // Am I shit? Let the designer choose, because else an object with artifact collision can be no more moved!

		return true;
	}

	void PhysicsTriggerComponent::reCreateCollision(void)
	{
		this->createStaticBody();
	}

	void PhysicsTriggerComponent::setCategories(const Ogre::String& categories)
	{
		this->categories->setValue(categories);
		this->categoriesId = GameObjectController::getInstance()->generateCategoryId(categories);
	}

	Ogre::String PhysicsTriggerComponent::getCategories(void) const
	{
		return this->categories->getString();
	}

}; // namespace end
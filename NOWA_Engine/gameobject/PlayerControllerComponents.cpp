#include "NOWAPrecompiled.h"
#include "PlayerControllerComponents.h"
#include "GameObjectController.h"
#include "PhysicsActiveComponent.h"
#include "PhysicsRagDollComponent.h"
#include "PhysicsPlayerControllerComponent.h"
#include "NodeComponent.h"
#include "LuaScriptComponent.h"
#include "main/Core.h"
#include "modules/InputDeviceModule.h"
#include "main/AppStateManager.h"

#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"

#include "camera/CameraManager.h"
#include "camera/BasePhysicsCamera.h"
#include "camera/FirstPersonCamera.h"
#include "camera/ThirdPersonCamera.h"
#include "camera/FollowCamera2D.h"
#include "CameraBehaviorComponents.h"

#include "modules/OgreRecastModule.h"
#include "modules/LuaScriptApi.h"
#include "modules/ParticleUniverseModule.h"
#include "AiComponents.h"

#include "utilities/AnimationBlender.h"
#include "utilities/AnimationBlenderV2.h"

#include "Animation/OgreSkeletonInstance.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	PlayerControllerComponent::AnimationBlenderObserver::AnimationBlenderObserver(luabind::object closureFunction, bool oneTime)
		: IAnimationBlender::IAnimationBlenderObserver(),
		closureFunction(closureFunction),
		oneTime(oneTime)
	{
		
	}

	PlayerControllerComponent::AnimationBlenderObserver::~AnimationBlenderObserver()
	{

	}

	void PlayerControllerComponent::AnimationBlenderObserver::onAnimationFinished(void)
	{
		if (this->closureFunction.is_valid())
		{
			try
			{
				luabind::call_function<void>(this->closureFunction);
			}
			catch (luabind::error& error)
			{
				luabind::object errorMsg(luabind::from_stack(error.state(), -1));
				std::stringstream msg;
				msg << errorMsg;

				Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnAnimationFinished' Error: " + Ogre::String(error.what())
					+ " details: " + msg.str());
			}
		}
	}

	bool PlayerControllerComponent::AnimationBlenderObserver::shouldReactOneTime(void) const
	{
		return this->oneTime;
	}

	void PlayerControllerComponent::AnimationBlenderObserver::setNewFunctionName(luabind::object closureFunction, bool oneTime)
	{
		this->closureFunction = closureFunction;
		this->oneTime = oneTime;
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////


	PlayerControllerComponent::PlayerControllerComponent()
		: GameObjectComponent(),
		activated(new Variant(PlayerControllerComponent::AttrActivated(), false, this->attributes)),
		rotationSpeed(new Variant(PlayerControllerComponent::AttrRotationSpeed(), 10.0f, this->attributes)),
		goalRadius(new Variant(PlayerControllerComponent::AttrGoalRadius(), 0.5f, this->attributes)),
		animationSpeed(new Variant(PlayerControllerComponent::AttrAnimationSpeed(), 1.0f, this->attributes)),
		acceleration(new Variant(PlayerControllerComponent::AttrAcceleration(), 0.0f, this->attributes)),
		categories(new Variant(PlayerControllerComponent::AttrCategories(), Ogre::String("All"), this->attributes)),
		physicsActiveComponent(nullptr),
		cameraBehaviorComponent(nullptr),
		animationBlender(nullptr),
		moveWeight(1.0f),
		jumpWeight(1.0f),
		height(500.0f),
		slope(0.0f),
		priorValidHeight(500.0f),
		normal(Ogre::Vector3::UNIT_SCALE * 100.0f),
		priorValidNormal(Ogre::Vector3::UNIT_SCALE * 100.0f),
		categoriesId(GameObjectController::ALL_CATEGORIES_ID),
		idle(true),
		canMove(true),
		canJump(false),
		hitGameObjectBelow(nullptr),
		hitGameObjectFront(nullptr),
		hitGameObjectUp(nullptr),
		animationBlenderObserver(nullptr),
		debugWaypointNode(nullptr)
	{
		// Make cameraManager singleton? And call getActiveCamera()
		/*this->cameraManager = new NOWA::CameraManager("CameraManager1", this->camera);
		this->cameraManager->addCameraBehavior(new NOWA::BaseCamera());
		this->cameraManager->setActiveCameraBehavior(NOWA::BaseCamera::BehaviorType());*/
		this->activated->setVisible(false);
		this->acceleration->setDescription("The acceleration rate, if set to 0, acceleration is disabled and player moves with full speed.");
	}

	PlayerControllerComponent::~PlayerControllerComponent()
	{
		AppStateManager::getSingletonPtr()->getGameObjectController()->removePlayerController(this->gameObjectPtr->getId());
	
		if (nullptr != this->animationBlenderObserver)
		{
			delete this->animationBlenderObserver;
			this->animationBlenderObserver = nullptr;
		}
		if (this->animationBlender)
		{
			delete this->animationBlender;
			this->animationBlender = nullptr;
		}
		this->physicsActiveComponent = nullptr;
		this->cameraBehaviorComponent = nullptr;
	}

	void PlayerControllerComponent::deleteDebugData(void)
	{
		if (nullptr != this->debugWaypointNode)
		{
			this->debugWaypointNode->detachAllObjects();

			for (auto it = this->debugWaypointNode->getAttachedObjectIterator().begin(); it != this->debugWaypointNode->getAttachedObjectIterator().end(); ++it)
			{
				this->gameObjectPtr->getSceneManager()->destroyMovableObject(*it);
			}

			this->gameObjectPtr->getSceneManager()->destroySceneNode(this->debugWaypointNode);
			this->debugWaypointNode = nullptr;
		}
	}

	void PlayerControllerComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		this->deleteDebugData();
	}

	bool PlayerControllerComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RotationSpeed")
		{
			this->setRotationSpeed(XMLConverter::getAttribReal(propertyElement, "data", 10.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "GoalRadius")
		{
			this->setGoalRadius(XMLConverter::getAttribReal(propertyElement, "data", 0.5f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnimationSpeed")
		{
			this->setAnimationSpeed(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Acceleration")
		{
			this->setAcceleration(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Categories")
		{
			this->categories->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	bool PlayerControllerComponent::postInit(void)
	{
		Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
		if (nullptr != entity)
		{
			this->animationBlender = new NOWA::AnimationBlender(entity);
		}
		else
		{
			Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
			if (nullptr != item)
			{
				this->animationBlender = new NOWA::AnimationBlenderV2(item);
			}
			else
			{
				return false;
			}
		}

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		return true;
	}

	bool PlayerControllerComponent::connect(void)
	{
		// In post init not all game objects are known, and so there are maybe no categories yet, so set the categories here
		this->setCategories(this->categories->getString());
		
		// Must be done here, because in post init, it may be, that a component does not yet exist, if its added after this component!
		auto& physicsPlayerControllerCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsPlayerControllerComponent>());
		if (nullptr == physicsPlayerControllerCompPtr)
		{
			auto& physicsActiveCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsActiveComponent>());
			if (nullptr == physicsActiveCompPtr)
			{
				return false;
			}
			else
			{
				this->physicsActiveComponent = physicsActiveCompPtr.get();
			}
		}
		else
		{
			this->physicsActiveComponent = dynamic_cast<PhysicsActiveComponent*>(physicsPlayerControllerCompPtr.get());
		}
		

		// Get optional camera behavior component, for activation in game object controller
		auto& cameraBehaviorCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<CameraBehaviorComponent>());
		if (nullptr != cameraBehaviorCompPtr)
			this->cameraBehaviorComponent = cameraBehaviorCompPtr.get();
		else
			this->cameraBehaviorComponent = nullptr;

		// Add the player controller to the player controller component map, but as weak ptr, because game object controller should not hold the life cycle of this components, because the game objects already do, which are
		// also hold shared by the game object controller
		AppStateManager::getSingletonPtr()->getGameObjectController()->addPlayerController(boost::dynamic_pointer_cast<PlayerControllerComponent>(shared_from_this()));

		this->internalShowDebugData();

		return true;
	}

	bool PlayerControllerComponent::disconnect(void)
	{
		AppStateManager::getSingletonPtr()->getGameObjectController()->removePlayerController(this->gameObjectPtr->getId());
		this->cameraBehaviorComponent = nullptr;
		this->moveLockOwner.clear();
		this->jumpWeightOwner.clear();

		AppStateManager::getSingletonPtr()->getOgreRecastModule()->removeDrawnPath();

		this->internalShowDebugData();
		return true;
	}

	void PlayerControllerComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating && nullptr != this->physicsActiveComponent /*&& true == this->activated->getBool()*/) // activated documented out, else idle animation will not work
		{
			auto widget = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Window>("manipulationWindow", false);

			this->setMoveWeight(1.0f);
			this->setJumpWeight(1.0f);

			Ogre::Vector3 direction = this->physicsActiveComponent->getOrientation() * this->gameObjectPtr->getDefaultDirection();

			Ogre::Vector3 playerSize = this->gameObjectPtr->getSize();
			Ogre::Vector3 bottomOffset = this->gameObjectPtr->getBottomOffset();
			Ogre::Vector3 centerOffset = this->gameObjectPtr->getCenterOffset();
			Ogre::Vector3 middleOfPlayer = this->gameObjectPtr->getMiddle();
			// Note on ogrenewt raycast, the startpoint is always the absolute game object position. Hence if root is on the middle of the game object, it will start e.g. plus 1 meter
			Ogre::Vector3 centerBottom = bottomOffset - playerSize.y;  /*middleOfPlayer - playerSize.y;*/
			const Ogre::Real fraction = 0.4f;
			
			bool showDebugData = false;
			// 1. Check objects that are below the player
			PhysicsActiveComponent::ContactData contactsDataBelow1 = this->physicsActiveComponent->getContactBelow(0, 
				Ogre::Vector3(-playerSize.z * fraction, centerBottom.y, 0.0f), showDebugData, this->categoriesId); // y must be 0!!
			
			PhysicsActiveComponent::ContactData contactsDataBelow2 = this->physicsActiveComponent->getContactBelow(1, 
				Ogre::Vector3(0.0f, centerBottom.y + 0.1f, playerSize.z * fraction), showDebugData, this->categoriesId); // y must be 0!!
			
			PhysicsActiveComponent::ContactData contactsDataBelow3 = this->physicsActiveComponent->getContactBelow(2, 
				Ogre::Vector3(playerSize.z * fraction, centerBottom.y + 0.1f, 0.0f), showDebugData, this->categoriesId); // y must be 0!!
			
			PhysicsActiveComponent::ContactData contactsDataBelowLine = this->physicsActiveComponent->getContactToDirection(0, Ogre::Vector3::UNIT_X, 
				Ogre::Vector3(0.0f, centerBottom.y + 0.1f, 0.0f), -playerSize.z * fraction, playerSize.z * fraction, showDebugData, this->categoriesId);

			this->hitGameObjectBelow = contactsDataBelow1.getHitGameObject();
			if (nullptr == this->hitGameObjectBelow)
			{
				this->hitGameObjectBelow = contactsDataBelow2.getHitGameObject();
			}
			if (nullptr == this->hitGameObjectBelow)
			{
				this->hitGameObjectBelow = contactsDataBelow3.getHitGameObject();
			}
			if (nullptr == this->hitGameObjectBelow)
			{
				this->hitGameObjectBelow = contactsDataBelowLine.getHitGameObject();
			}
			/*if (widget) widget->setCaption("h1: " + Ogre::StringConverter::toString(std::get<1>(contactsDataBelow1))
				+ " h2: " + Ogre::StringConverter::toString(std::get<1>(contactsDataBelow2))
				+ " h3: " + Ogre::StringConverter::toString(std::get<1>(contactsDataBelow3)));*/

			if (500.0f != this->height)
				this->priorValidHeight = this->height;

			if (Ogre::Vector3::UNIT_SCALE * 100.0f != this->normal)
				this->priorValidNormal = this->normal;

			this->height = std::min(contactsDataBelow2.getHeight(), std::min(contactsDataBelow1.getHeight(), contactsDataBelow3.getHeight()));
			this->normal = std::min(contactsDataBelow2.getNormal(), std::min(contactsDataBelow1.getNormal(), contactsDataBelow3.getNormal()));
			this->slope = std::min(contactsDataBelow2.getSlope(), std::min(contactsDataBelow1.getSlope(), contactsDataBelow3.getSlope()));

			// Nothing found below, player must be in air!
			if (nullptr == contactsDataBelow1.getHitGameObject() && nullptr == contactsDataBelow2.getHitGameObject() && contactsDataBelow3.getHitGameObject())
			{
				this->height = this->priorValidHeight; // in in air in any case!
				this->normal = this->priorValidNormal;
			}

			if (nullptr == this->hitGameObjectBelow)
			{
				this->height = this->priorValidHeight;
				this->normal = this->priorValidNormal;
			}

			/*if (widget)
				widget->setCaption("Height: " + Ogre::StringConverter::toString(this->height));*/

			// if (widget)
			// 	widget->setCaption("Normal: " + Ogre::StringConverter::toString(this->normal));

			// 2. Check all objects that are in front of the player

			// never change -0.2, because else the rope does not exist anymore and the player gets stuck on a wall

			PhysicsActiveComponent::ContactData contactDataFront[4];
			///*contactDataFront[0] = this->physicsActiveComponent->getContactToDirection(direction, middleOfPlayer + Ogre::Vector3(0.0f, playerSize.y, 0.0f), 1, 10);
			//contactDataFront[1] = this->physicsActiveComponent->getContactToDirection(direction, middleOfPlayer + Ogre::Vector3(0.0f, playerSize.y * 0.5f, 0.0f), 1, 10);
			//contactDataFront[2] = this->physicsActiveComponent->getContactToDirection(direction, middleOfPlayer + Ogre::Vector3(0.0f, 0.2f, 0.0f), 1, 10);*/

			contactDataFront[0] = this->physicsActiveComponent->getContactToDirection(1, direction, 
				Ogre::Vector3(0.0f, centerBottom.y * -0.7f, playerSize.z - 0.2f), 0.0f, 0.1f, showDebugData, this->categoriesId);
			
			contactDataFront[1] = this->physicsActiveComponent->getContactToDirection(2, direction, 
				Ogre::Vector3(0.0f, 0.0f, playerSize.z - 0.2f), 0.0f, 0.1f, showDebugData, this->categoriesId);
			
			contactDataFront[2] = this->physicsActiveComponent->getContactToDirection(3, direction, 
				Ogre::Vector3(0.0f, centerBottom.y * 0.5f, playerSize.z - 0.2f), 0.0f, 0.1f, showDebugData, this->categoriesId);

			//// Build:	T |
			////		A | -> Line to check front
			contactDataFront[3] = this->physicsActiveComponent->getContactToDirection(4, Ogre::Vector3::UNIT_Y, 
				Ogre::Vector3(0.0f, centerBottom.y + 0.1f, playerSize.z * 0.5f), -0.05f, playerSize.y + 0.05f, showDebugData, this->categoriesId); // 0.2 is to much, and player would stuck in certain ground
			//
			this->hitGameObjectFront = nullptr;

			if (nullptr != contactDataFront[0].getHitGameObject())
				this->hitGameObjectFront = contactDataFront[0].getHitGameObject();
			else if (nullptr != contactDataFront[1].getHitGameObject())
			 	this->hitGameObjectFront = contactDataFront[1].getHitGameObject();
			 else if (nullptr != contactDataFront[2].getHitGameObject())
			 	this->hitGameObjectFront = contactDataFront[2].getHitGameObject();
			
			if (nullptr != contactDataFront[3].getHitGameObject())
				this->hitGameObjectFront = contactDataFront[3].getHitGameObject();

			this->hitGameObjectUp = nullptr;
			this->hitGameObjectUp = this->physicsActiveComponent->getContactToDirection(5, Ogre::Vector3::UNIT_X, 
				Ogre::Vector3(0.0f, playerSize.y - bottomOffset.y, 0.0f), -playerSize.z * 0.2f, playerSize.z * 0.2f, showDebugData, this->categoriesId).getHitGameObject();
		}
	}

	void PlayerControllerComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (PlayerControllerComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (PlayerControllerComponent::AttrRotationSpeed() == attribute->getName())
		{
			this->setRotationSpeed(attribute->getReal());
		}
		else if (PlayerControllerComponent::AttrGoalRadius() == attribute->getName())
		{
			this->setGoalRadius(attribute->getReal());
		}
		else if (PlayerControllerComponent::AttrAnimationSpeed() == attribute->getName())
		{
			this->setAnimationSpeed(attribute->getReal());
		}
		else if (PlayerControllerComponent::AttrAcceleration() == attribute->getName())
		{
			this->setAcceleration(attribute->getReal());
		}
		else if (PlayerControllerComponent::AttrCategories() == attribute->getName())
		{
			this->setCategories(attribute->getString());
		}
	}

	void PlayerControllerComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RotationSpeed"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rotationSpeed->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "GoalRadius"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->goalRadius->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnimationSpeed"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animationSpeed->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Acceleration"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->acceleration->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Categories"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->categories->getString())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String PlayerControllerComponent::getClassName(void) const
	{
		return "PlayerControllerComponent";
	}

	Ogre::String PlayerControllerComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void PlayerControllerComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);

		// Sent event, that this player controller has been activated or deactivated
		boost::shared_ptr<EventDataActivatePlayerController> eventDataActivePlayerController(new EventDataActivatePlayerController(this->activated->getBool()));
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataActivePlayerController);
	}

	bool PlayerControllerComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void PlayerControllerComponent::internalShowDebugData(void)
	{
		if (nullptr != this->animationBlender)
		{
			this->animationBlender->setDebugLog(this->bShowDebugData);
		}
		if (false == this->bShowDebugData)
		{
			this->deleteDebugData();
		}
	}

	void PlayerControllerComponent::setRotationSpeed(Ogre::Real rotationSpeed)
	{
		if (rotationSpeed < 5.0f)
		{
			rotationSpeed = 5.0f;
		}
		else if (rotationSpeed > 15.0f)
		{
			rotationSpeed = 15.0f;
		}
		this->rotationSpeed->setValue(rotationSpeed);
	}

	Ogre::Real PlayerControllerComponent::getRotationSpeed(void) const
	{
		return this->rotationSpeed->getReal();
	}

	void PlayerControllerComponent::setGoalRadius(Ogre::Real goalRadius)
	{
		if (goalRadius < 0.2f)
		{
			goalRadius = 0.2f;
		}
		this->goalRadius->setValue(goalRadius);
	}

	Ogre::Real PlayerControllerComponent::getGoalRadius(void) const
	{
		return this->goalRadius->getReal();
	}

	IAnimationBlender* PlayerControllerComponent::getAnimationBlender(void) const
	{
		return this->animationBlender;
	}

	PhysicsActiveComponent* PlayerControllerComponent::getPhysicsComponent(void) const
	{
		return this->physicsActiveComponent;
	}

	PhysicsRagDollComponent* PlayerControllerComponent::getPhysicsRagDollComponent(void) const
	{
		return dynamic_cast<PhysicsRagDollComponent*>(this->physicsActiveComponent);
	}

	void PlayerControllerComponent::lockMovement(const Ogre::String& ownerName, bool lock)
	{
		if (true == lock)
		{
			if (true == this->moveLockOwner.empty())
			{
				this->moveLockOwner = ownerName;
				this->moveWeight = 0.0f;
				this->jumpWeight = 0.0f;
			}
			/*else
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PlayerControllerComponent] Could not request move weight because there is already an owner: '"
					+ this->moveLockOwner + "'. Please call first lockMovement with false for that owner!");
			}*/
		}
		else
		{
			if (this->moveLockOwner == ownerName)
			{
				this->moveLockOwner.clear();
			}
			/*else
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PlayerControllerComponent] Could not release move weight because there no such owner name: '"
					+ ownerName + "'.");
			}*/
		}
	}

	void PlayerControllerComponent::setMoveWeight(Ogre::Real moveWeight)
	{
		if (true == this->moveLockOwner.empty())
		{
			this->moveWeight = moveWeight;
			this->jumpWeight = moveWeight;
		}
	}

	Ogre::Real PlayerControllerComponent::getMoveWeight(void) const
	{
		return this->moveWeight;
	}

	void PlayerControllerComponent::setJumpWeight(Ogre::Real jumpWeight)
	{
		if (true == this->moveLockOwner.empty())
		{
			this->jumpWeight = jumpWeight;
		}
	}

	Ogre::Real PlayerControllerComponent::getJumpWeight(void) const
	{
		return this->jumpWeight;
	}

	void PlayerControllerComponent::setIdle(bool idle)
	{
		this->idle = idle;
	}

	bool PlayerControllerComponent::isIdle(void) const
	{
		return this->idle;
	}

	void PlayerControllerComponent::setAnimationSpeed(Ogre::Real animationSpeed)
	{
		if (animationSpeed < 0.0f)
			animationSpeed = 1.0f;
		this->animationSpeed->setValue(animationSpeed);
	}

	Ogre::Real PlayerControllerComponent::getAnimationSpeed(void) const
	{
		return this->animationSpeed->getReal();
	}

	void PlayerControllerComponent::setAcceleration(Ogre::Real acceleration)
	{
		if (acceleration < 0.0f)
			acceleration = 0.0f;
		this->acceleration->setValue(acceleration);
	}

	Ogre::Real PlayerControllerComponent::getAcceleration(void) const
	{
		return this->acceleration->getReal();
	}

	void PlayerControllerComponent::setCategories(const Ogre::String& categories)
	{
		this->categories->setValue(categories);
		this->categoriesId = AppStateManager::getSingletonPtr()->getGameObjectController()->generateCategoryId(categories);
	}

	Ogre::String PlayerControllerComponent::getCategories(void) const
	{
		return this->categories->getString();
	}

	void PlayerControllerComponent::setAnimationName(const Ogre::String& name, unsigned int index)
	{
		if (index > this->animations.size())
			return;
		this->animations[index]->setListSelectedValue(name);
	}

	Ogre::String PlayerControllerComponent::getAnimationName(unsigned int index)
	{
		if (index > this->animations.size())
			return 0;
		return this->animations[index]->getListSelectedValue();
	}

	CameraBehaviorComponent* PlayerControllerComponent::getCameraBehaviorComponent(void) const
	{
		return this->cameraBehaviorComponent;
	}
	
	Ogre::Real PlayerControllerComponent::getHeight(void) const
	{
		return this->height;
	}
	
	Ogre::Vector3 PlayerControllerComponent::getNormal(void) const
	{
		return this->normal;
	}

	Ogre::Real PlayerControllerComponent::getSlope(void) const
	{
		return this->slope;
	}

	GameObject* PlayerControllerComponent::getHitGameObjectBelow(void) const
	{
		return this->hitGameObjectBelow;
	}

	GameObject* PlayerControllerComponent::getHitGameObjectFront(void) const
	{
		return this->hitGameObjectFront;
	}

	GameObject* PlayerControllerComponent::getHitGameObjectUp(void) const
	{
		return this->hitGameObjectUp;
	}

	void PlayerControllerComponent::reactOnAnimationFinished(luabind::object closureFunction, bool oneTime)
	{
		if (nullptr == this->animationBlender)
			return;

		if (nullptr == this->animationBlenderObserver)
		{
			this->animationBlenderObserver = new AnimationBlenderObserver(closureFunction, oneTime);
		}
		else
		{
			static_cast<AnimationBlenderObserver*>(this->animationBlenderObserver)->setNewFunctionName(closureFunction, oneTime);
		}
		// Note: animation blender will delete observer automatically
		this->animationBlender->setAnimationBlenderObserver(this->animationBlenderObserver);
	}
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	PlayerControllerJumpNRunComponent::PlayerControllerJumpNRunComponent()
		: PlayerControllerComponent(),
		stateMachine(nullptr),
		jumpForce(new Variant(PlayerControllerJumpNRunComponent::AttrJumpForce(), 15.0f, this->attributes)),
		doubleJump(new Variant(PlayerControllerJumpNRunComponent::AttrDoubleJump(), false, this->attributes)),
		runAfterWalkTime(new Variant(PlayerControllerJumpNRunComponent::AttrRunAfterWalkTime(), 0.0f, this->attributes)),
		for2D(new Variant(PlayerControllerJumpNRunComponent::AttrFor2D(), false, this->attributes))
	{
		this->animations.resize(this->animationsCount);
		this->animations[0] = new Variant(PlayerControllerJumpNRunComponent::AttrAnimIdle1(), std::vector<Ogre::String>(), this->attributes);
		this->animations[1] = new Variant(PlayerControllerJumpNRunComponent::AttrAnimIdle2(), std::vector<Ogre::String>(), this->attributes);
		this->animations[2] = new Variant(PlayerControllerJumpNRunComponent::AttrAnimIdle3(), std::vector<Ogre::String>(), this->attributes);
		this->animations[3] = new Variant(PlayerControllerJumpNRunComponent::AttrAnimWalkNorth(), std::vector<Ogre::String>(), this->attributes);
		this->animations[4] = new Variant(PlayerControllerJumpNRunComponent::AttrAnimWalkSouth(), std::vector<Ogre::String>(), this->attributes);
		this->animations[5] = new Variant(PlayerControllerJumpNRunComponent::AttrAnimWalkWest(), std::vector<Ogre::String>(), this->attributes);
		this->animations[6] = new Variant(PlayerControllerJumpNRunComponent::AttrAnimWalkEast(), std::vector<Ogre::String>(), this->attributes);
		this->animations[7] = new Variant(PlayerControllerJumpNRunComponent::AttrAnimJumpStart(), std::vector<Ogre::String>(), this->attributes);
		this->animations[8] = new Variant(PlayerControllerJumpNRunComponent::AttrAnimJumpWalk(), std::vector<Ogre::String>(), this->attributes);
		this->animations[9] = new Variant(PlayerControllerJumpNRunComponent::AttrAnimHighJumpEnd(), std::vector<Ogre::String>(), this->attributes);
		this->animations[10] = new Variant(PlayerControllerJumpNRunComponent::AttrAnimJumpEnd(), std::vector<Ogre::String>(), this->attributes);
		this->animations[11] = new Variant(PlayerControllerJumpNRunComponent::AttrAnimRun(), std::vector<Ogre::String>(), this->attributes);
		this->animations[12] = new Variant(PlayerControllerJumpNRunComponent::AttrAnimSneak(), std::vector<Ogre::String>(), this->attributes);
		this->animations[13] = new Variant(PlayerControllerJumpNRunComponent::AttrAnimDuck(), std::vector<Ogre::String>(), this->attributes);

		this->runAfterWalkTime->setDescription("Specifies the time in seconds at which the player will start to run, after walking without interruption. If set to 0, the player will never run.");
		this->for2D->setDescription("If set to true, in the PhysicsActiveComponent the 'ConstraintAxis' attribute should be set to '0 0 1'. So that the player only can move on x and y axis.");
	}

	PlayerControllerJumpNRunComponent::~PlayerControllerJumpNRunComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlayerControllerJumpNRunComponent] Destructor player controller 3D component for game object: " + this->gameObjectPtr->getName());

		if (this->stateMachine)
		{
			delete this->stateMachine;
			this->stateMachine = nullptr;
		}

		AppStateManager::getSingletonPtr()->getParticleUniverseModule()->removeParticle("smoke" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()));
	}

	bool PlayerControllerJumpNRunComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = PlayerControllerComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JumpForce")
		{
			this->jumpForce->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DoubleJump")
		{
			this->doubleJump->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RunAfterWalkTime")
		{
			this->runAfterWalkTime->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "For2D")
		{
			this->for2D->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnimIdle1")
		{
			this->animations[0]->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "None"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnimIdle2")
		{
			this->animations[1]->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "None"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnimIdle3")
		{
			this->animations[2]->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "None"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnimWalkNorth")
		{
			this->animations[3]->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "None"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnimWalkSouth")
		{
			this->animations[4]->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "None"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnimWalkWest")
		{
			this->animations[5]->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "None"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnimWalkEast")
		{
			this->animations[6]->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "None"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnimJumpStart")
		{
			this->animations[7]->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "None"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnimJumpWalk")
		{
			this->animations[8]->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "None"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnimHighJumpEnd")
		{
			this->animations[9]->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "None"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnimJumpEnd")
		{
			this->animations[10]->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "None"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnimRun")
		{
			this->animations[11]->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "None"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnimSneak")
		{
			this->animations[12]->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "None"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnimDuck")
		{
			this->animations[13]->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "None"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return success;
	}

	GameObjectCompPtr PlayerControllerJumpNRunComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		PlayerControllerJumpNRunCompPtr clonedCompPtr(boost::make_shared<PlayerControllerJumpNRunComponent>());

		
		// Do not clone activated, since its no visible and switched manually in game object controller
		// clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setRotationSpeed(this->rotationSpeed->getReal());
		clonedCompPtr->setGoalRadius(this->goalRadius->getReal());
		clonedCompPtr->setJumpForce(this->jumpForce->getReal());
		clonedCompPtr->setDoubleJump(this->doubleJump->getReal());
		clonedCompPtr->setRunAfterWalkTime(this->runAfterWalkTime->getReal());
		clonedCompPtr->setFor2D(this->for2D->getBool());
		clonedCompPtr->setAnimationSpeed(this->animationSpeed->getReal());
		clonedCompPtr->setAcceleration(this->acceleration->getReal());
		clonedCompPtr->setCategories(this->categories->getString());

		for (unsigned int i = 0; i < static_cast<unsigned int>(this->animations.size()); i++)
		{
			clonedCompPtr->setAnimationName(this->animations[i]->getListSelectedValue(), i);
		}
		
		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool PlayerControllerJumpNRunComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlayerControllerJumpNRunComponent] Init player controller 3D component for game object: " + this->gameObjectPtr->getName());

		Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
		if (nullptr != entity)
		{
			std::vector<Ogre::String> animationNames;
			// Add also none, so that when choosen, no animation will be done, because it does not exist
			animationNames.emplace_back("None");

			Ogre::v1::AnimationStateSet* set = entity->getAllAnimationStates();
			if (nullptr != set)
			{
				Ogre::v1::AnimationStateIterator it = set->getAnimationStateIterator();
				// list all animations
				while (it.hasMoreElements())
				{
					Ogre::v1::AnimationState* anim = it.getNext();
					animationNames.emplace_back(anim->getAnimationName());
				}
				// Add all available animation names to list
				for (unsigned short i = 0; i < this->animationsCount; i++)
				{
					this->animations[i]->setValue(animationNames);
				}
			}
		}

		this->stateMachine = new NOWA::KI::StateMachine<GameObject>(this->gameObjectPtr.get());
		this->stateMachine->registerState<WalkingStateJumpNRun>(WalkingStateJumpNRun::getName());

		return PlayerControllerComponent::postInit();
	}

	bool PlayerControllerJumpNRunComponent::connect(void)
	{
		bool success = PlayerControllerComponent::connect();

		// Add some particle effect, when player is grounded from jump
		AppStateManager::getSingletonPtr()->getParticleUniverseModule()->createParticleSystem("smoke" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()), "smoke", 500.0f, Ogre::Quaternion::IDENTITY, this->gameObjectPtr->getPosition(), 0.05f);
		auto particleStruct = AppStateManager::getSingletonPtr()->getParticleUniverseModule()->getParticle("smoke" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()));
		if (nullptr != particleStruct)
		{
			particleStruct->particleOffsetPosition = Ogre::Vector3(0.0f, -0.1f, 0.0f);
			particleStruct->particle->setScaleTime(10.0f);
		}
		
		this->animationBlender->registerAnimation(NOWA::AnimationBlender::ANIM_IDLE_1, this->animations[0]->getListSelectedValue());
		this->animationBlender->registerAnimation(NOWA::AnimationBlender::ANIM_IDLE_2, this->animations[1]->getListSelectedValue());
		this->animationBlender->registerAnimation(NOWA::AnimationBlender::ANIM_IDLE_3, this->animations[2]->getListSelectedValue());
		this->animationBlender->registerAnimation(NOWA::AnimationBlender::ANIM_WALK_NORTH, this->animations[3]->getListSelectedValue());
		this->animationBlender->registerAnimation(NOWA::AnimationBlender::ANIM_WALK_SOUTH, this->animations[4]->getListSelectedValue());
		this->animationBlender->registerAnimation(NOWA::AnimationBlender::ANIM_WALK_WEST, this->animations[5]->getListSelectedValue());
		this->animationBlender->registerAnimation(NOWA::AnimationBlender::ANIM_WALK_EAST, this->animations[6]->getListSelectedValue());
		this->animationBlender->registerAnimation(NOWA::AnimationBlender::ANIM_JUMP_START, this->animations[7]->getListSelectedValue());
		this->animationBlender->registerAnimation(NOWA::AnimationBlender::ANIM_JUMP_WALK, this->animations[8]->getListSelectedValue());
		this->animationBlender->registerAnimation(NOWA::AnimationBlender::ANIM_HIGH_JUMP_END, this->animations[9]->getListSelectedValue());
		this->animationBlender->registerAnimation(NOWA::AnimationBlender::ANIM_JUMP_END, this->animations[10]->getListSelectedValue());
		this->animationBlender->registerAnimation(NOWA::AnimationBlender::ANIM_RUN, this->animations[11]->getListSelectedValue());
		this->animationBlender->registerAnimation(NOWA::AnimationBlender::ANIM_SNEAK, this->animations[12]->getListSelectedValue());
		this->animationBlender->registerAnimation(NOWA::AnimationBlender::ANIM_DUCK, this->animations[13]->getListSelectedValue());

		this->animationBlender->init(NOWA::AnimationBlender::ANIM_IDLE_1);

		this->stateMachine->setCurrentState(WalkingStateJumpNRun::getName());
		return success;
	}

	bool PlayerControllerJumpNRunComponent::disconnect(void)
	{
		bool success = PlayerControllerComponent::disconnect();

		if (nullptr != this->animationBlender)
		{
			// this->animationBlender->clearAnimations();
			this->animationBlender->init(NOWA::AnimationBlender::ANIM_IDLE_1);
			// Reset animation to T-Pose
			this->animationBlender->setSourceEnabled(false);

			AppStateManager::getSingletonPtr()->getParticleUniverseModule()->removeParticle("smoke" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()));
		}

		if (nullptr != this->stateMachine->getCurrentState())
		{
			this->stateMachine->getCurrentState()->exit(this->gameObjectPtr.get());
		}

		return success;
	}

	void PlayerControllerJumpNRunComponent::update(Ogre::Real dt, bool notSimulating)
	{
		PlayerControllerComponent::update(dt, notSimulating);
		if (false == notSimulating /*&& true == this->activated->getBool()*/) // activated documented out, else idle animation will not work
		{
			this->stateMachine->update(dt);
		}
	}

	void PlayerControllerJumpNRunComponent::actualizeValue(Variant* attribute)
	{
		PlayerControllerComponent::actualizeValue(attribute);

		if (PlayerControllerJumpNRunComponent::AttrJumpForce() == attribute->getName())
		{
			this->setJumpForce(attribute->getReal());
		}
		else if (PlayerControllerJumpNRunComponent::AttrDoubleJump() == attribute->getName())
		{
			this->setDoubleJump(attribute->getBool());
		}
		else if (PlayerControllerJumpNRunComponent::AttrRunAfterWalkTime() == attribute->getName())
		{
			this->setRunAfterWalkTime(attribute->getReal());
		}
		else if (PlayerControllerJumpNRunComponent::AttrFor2D() == attribute->getName())
		{
			this->setFor2D(attribute->getBool());
		}
		else if (PlayerControllerJumpNRunComponent::AttrAnimIdle1() == attribute->getName())
		{
			this->setAnimationName(attribute->getListSelectedValue(), 0);
		}
		else if (PlayerControllerJumpNRunComponent::AttrAnimIdle2() == attribute->getName())
		{
			this->setAnimationName(attribute->getListSelectedValue(), 1);
		}
		else if (PlayerControllerJumpNRunComponent::AttrAnimIdle3() == attribute->getName())
		{
			this->setAnimationName(attribute->getListSelectedValue(), 2);
		}
		else if (PlayerControllerJumpNRunComponent::AttrAnimWalkNorth() == attribute->getName())
		{
			this->setAnimationName(attribute->getListSelectedValue(), 3);
		}
		else if (PlayerControllerJumpNRunComponent::AttrAnimWalkSouth() == attribute->getName())
		{
			this->setAnimationName(attribute->getListSelectedValue(), 4);
		}
		else if (PlayerControllerJumpNRunComponent::AttrAnimWalkWest() == attribute->getName())
		{
			this->setAnimationName(attribute->getListSelectedValue(), 5);
		}
		else if (PlayerControllerJumpNRunComponent::AttrAnimWalkEast() == attribute->getName())
		{
			this->setAnimationName(attribute->getListSelectedValue(), 6);
		}
		else if (PlayerControllerJumpNRunComponent::AttrAnimJumpStart() == attribute->getName())
		{
			this->setAnimationName(attribute->getListSelectedValue(), 7);
		}
		else if (PlayerControllerJumpNRunComponent::AttrAnimJumpWalk() == attribute->getName())
		{
			this->setAnimationName(attribute->getListSelectedValue(), 8);
		}
		else if (PlayerControllerJumpNRunComponent::AttrAnimHighJumpEnd() == attribute->getName())
		{
			this->setAnimationName(attribute->getListSelectedValue(), 9);
		}
		else if (PlayerControllerJumpNRunComponent::AttrAnimJumpEnd() == attribute->getName())
		{
			this->setAnimationName(attribute->getListSelectedValue(), 10);
		}
		else if (PlayerControllerJumpNRunComponent::AttrAnimRun() == attribute->getName())
		{
			this->setAnimationName(attribute->getListSelectedValue(), 11);
		}
		else if (PlayerControllerJumpNRunComponent::AttrAnimSneak() == attribute->getName())
		{
			this->setAnimationName(attribute->getListSelectedValue(), 12);
		}
		else if (PlayerControllerJumpNRunComponent::AttrAnimDuck() == attribute->getName())
		{
			this->setAnimationName(attribute->getListSelectedValue(), 13);
		}
	}

	void PlayerControllerJumpNRunComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		PlayerControllerComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JumpForce"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->jumpForce->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DoubleJump"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->doubleJump->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RunAfterWalkTime"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->runAfterWalkTime->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "For2D"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->for2D->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnimIdle1"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animations[0]->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnimIdle2"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animations[1]->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnimIdle3"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animations[2]->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnimWalkNorth"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animations[3]->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnimWalkSouth"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animations[4]->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnimWalkWest"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animations[5]->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnimWalkEast"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animations[6]->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnimJumpStart"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animations[7]->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnimJumpWalk"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animations[8]->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnimHighJumpEnd"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animations[9]->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnimJumpEnd"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animations[10]->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnimRun"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animations[11]->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnimSneak"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animations[12]->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnimDuck"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animations[13]->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
	}

	KI::StateMachine<GameObject>* PlayerControllerJumpNRunComponent::getStateMaschine(void) const
	{
		return this->stateMachine;
	}

	void PlayerControllerJumpNRunComponent::setJumpForce(Ogre::Real jumpForce)
	{
		this->jumpForce->setValue(jumpForce);
	}

	Ogre::Real PlayerControllerJumpNRunComponent::getJumpForce(void) const
	{
		return this->jumpForce->getReal();
	}

	void PlayerControllerJumpNRunComponent::setDoubleJump(bool doubleJump)
	{
		this->doubleJump->setValue(doubleJump);
	}

	bool PlayerControllerJumpNRunComponent::getDoubleJump(void) const
	{
		return this->doubleJump->getBool();
	}
	
	void PlayerControllerJumpNRunComponent::setRunAfterWalkTime(Ogre::Real runAfterWalkTime)
	{
		this->runAfterWalkTime->setValue(runAfterWalkTime);
	}
	
	Ogre::Real PlayerControllerJumpNRunComponent::getRunAfterWalkTime(void) const
	{
		return this->runAfterWalkTime->getReal();
	}

	void PlayerControllerJumpNRunComponent::setFor2D(bool for2D)
	{
		this->for2D->setValue(for2D);
	}

	bool PlayerControllerJumpNRunComponent::getIsFor2D(void) const
	{
		return this->for2D->getBool();
	}

	Ogre::String PlayerControllerJumpNRunComponent::getClassName(void) const
	{
		return "PlayerControllerJumpNRunComponent";
	}

	Ogre::String PlayerControllerJumpNRunComponent::getParentClassName(void) const
	{
		return "PlayerControllerComponent";
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////
	
	PlayerControllerJumpNRunLuaComponent::PlayerControllerJumpNRunLuaComponent()
		: PlayerControllerComponent(),
		luaStateMachine(nullptr),
		startStateName(new Variant(PlayerControllerJumpNRunLuaComponent::AttrStartStateName(), "MyState", this->attributes))
	{
		
	}

	PlayerControllerJumpNRunLuaComponent::~PlayerControllerJumpNRunLuaComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlayerControllerJumpNRunLuaComponent] Destructor player controller 3D Lua component for game object: " + this->gameObjectPtr->getName());

		AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &PlayerControllerJumpNRunLuaComponent::handleLuaScriptConnected), EventDataLuaScriptConnected::getStaticEventType());

		if (this->luaStateMachine)
		{
			delete this->luaStateMachine;
			this->luaStateMachine = nullptr;
		}
	}

	bool PlayerControllerJumpNRunLuaComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = PlayerControllerComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "StartStateName")
		{
			this->startStateName->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &PlayerControllerJumpNRunLuaComponent::handleLuaScriptConnected), EventDataLuaScriptConnected::getStaticEventType());

		return success;
	}

	GameObjectCompPtr PlayerControllerJumpNRunLuaComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		PlayerControllerJumpNRunLuaCompPtr clonedCompPtr(boost::make_shared<PlayerControllerJumpNRunLuaComponent>());

		
		// Do not clone activated, since its no visible and switched manually in game object controller
		// clonedCompPtr->setActivated(this->activated->getBool());

		clonedCompPtr->setRotationSpeed(this->rotationSpeed->getReal());
		clonedCompPtr->setGoalRadius(this->goalRadius->getReal());
		clonedCompPtr->setAnimationSpeed(this->animationSpeed->getReal());
		clonedCompPtr->setAcceleration(this->acceleration->getReal());
		clonedCompPtr->setCategories(this->categories->getString());

		clonedCompPtr->setStartStateName(this->startStateName->getString());
		
		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool PlayerControllerJumpNRunLuaComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlayerControllerJumpNRunLuaComponent] Init player controller 3D Lua component for game object: " + this->gameObjectPtr->getName());

		this->luaStateMachine = new LuaStateMachine<GameObject>(this->gameObjectPtr.get());

		return PlayerControllerComponent::postInit();
	}

	bool PlayerControllerJumpNRunLuaComponent::connect(void)
	{
		bool success = PlayerControllerComponent::connect();

		return success;
	}

	void PlayerControllerJumpNRunLuaComponent::handleLuaScriptConnected(NOWA::EventDataPtr eventData)
	{
		boost::shared_ptr<EventDataLuaScriptConnected> castEventData = boost::static_pointer_cast<EventDataLuaScriptConnected>(eventData);
		// Found the game object
		if (this->gameObjectPtr->getId() == castEventData->getGameObjectId())
		{
			// Call enter on the start state
			if (nullptr != this->gameObjectPtr->getLuaScript())
			{
				// http://www.allacrost.org/wiki/index.php?title=Scripting_Engine

				this->gameObjectPtr->getLuaScript()->setInterfaceFunctionsTemplate(
					"\n" + this->startStateName->getString() + " = { };\n"
					"aiLuaComponent = nil;\n\n"
					+ this->startStateName->getString() + "[\"enter\"] = function(gameObject)\n"
					"\taiLuaComponent = gameObject:getAiLuaComponent();\nend\n\n"
					+ this->startStateName->getString() + "[\"execute\"] = function(gameObject, dt)\n\nend\n\n"
					+ this->startStateName->getString() + "[\"exit\"] = function(gameObject)\n\nend");

				if (true == this->gameObjectPtr->getLuaScript()->createLuaEnvironmentForStateTable(this->startStateName->getString()))
				{
					const luabind::object& compiledStateScriptReference = this->gameObjectPtr->getLuaScript()->getCompiledStateScriptReference();

					// Call the start state name to start the lua file with that state
					this->luaStateMachine->setCurrentState(compiledStateScriptReference);
				}
			}
		}
	}

	bool PlayerControllerJumpNRunLuaComponent::disconnect(void)
	{
		bool success = PlayerControllerComponent::disconnect();
		
		return success;
	}

	void PlayerControllerJumpNRunLuaComponent::update(Ogre::Real dt, bool notSimulating)
	{
		PlayerControllerComponent::update(dt, notSimulating);
		if (false == notSimulating /*&& true == this->activated->getBool()*/) // activated documented out, else idle animation will not work
		{
			this->luaStateMachine->update(dt);
		}
	}

	void PlayerControllerJumpNRunLuaComponent::actualizeValue(Variant* attribute)
	{
		PlayerControllerComponent::actualizeValue(attribute);

		if (PlayerControllerJumpNRunLuaComponent::AttrStartStateName() == attribute->getName())
		{
			this->setStartStateName(attribute->getString());
		}
	}

	void PlayerControllerJumpNRunLuaComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		PlayerControllerComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "StartStateName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->startStateName->getString())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String PlayerControllerJumpNRunLuaComponent::getClassName(void) const
	{
		return "PlayerControllerJumpNRunLuaComponent";
	}

	Ogre::String PlayerControllerJumpNRunLuaComponent::getParentClassName(void) const
	{
		return "PlayerControllerComponent";
	}

	NOWA::KI::LuaStateMachine<GameObject>* PlayerControllerJumpNRunLuaComponent::getStateMachine(void) const
	{
		return this->luaStateMachine;
	}

	void PlayerControllerJumpNRunLuaComponent::setStartStateName(const Ogre::String& startStateName)
	{
		this->startStateName->setValue(startStateName);
	}

	Ogre::String PlayerControllerJumpNRunLuaComponent::getStartStateName(void) const
	{
		return this->startStateName->getString();
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////

	PlayerControllerClickToPointComponent::PlayerControllerClickToPointComponent()
		: PlayerControllerComponent(),
		stateMachine(nullptr),
		// movingBehavior(nullptr),
		drawPath(false),
		raySceneQuery(nullptr),
		// categories(new Variant(PlayerControllerClickToPointComponent::AttrCategories(), Ogre::String("All"), this->attributes)),
		range(new Variant(PlayerControllerClickToPointComponent::AttrRange(), 100.0f, this->attributes)),
		pathSlot(new Variant(PlayerControllerClickToPointComponent::AttrPathSlot(), static_cast<int>(0), this->attributes))
	{
		this->animations.resize(this->animationsCount);
		this->animations[0] = new Variant(PlayerControllerClickToPointComponent::AttrAnimIdle1(), std::vector<Ogre::String>(), this->attributes);
		this->animations[1] = new Variant(PlayerControllerClickToPointComponent::AttrAnimIdle2(), std::vector<Ogre::String>(), this->attributes);
		this->animations[2] = new Variant(PlayerControllerClickToPointComponent::AttrAnimIdle3(), std::vector<Ogre::String>(), this->attributes);
		this->animations[3] = new Variant(PlayerControllerClickToPointComponent::AttrAnimWalkNorth(), std::vector<Ogre::String>(), this->attributes);
		this->animations[4] = new Variant(PlayerControllerClickToPointComponent::AttrAnimRun(), std::vector<Ogre::String>(), this->attributes);
		this->autoClick = new Variant(PlayerControllerClickToPointComponent::AttrAutoClick(), false, this->attributes);

		this->autoClick->setDescription("If set to true, the user can hold the middle button and the player will automatically update the waypoints. If set to false, the player must click each time to update the waypoints.");
	}

	PlayerControllerClickToPointComponent::~PlayerControllerClickToPointComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlayerControllerClickToPointComponent] Destructor player controller click to point component for game object: " + this->gameObjectPtr->getName());

		if (nullptr != this->stateMachine)
		{
			delete this->stateMachine;
			this->stateMachine = nullptr;
		}
		// Is done in onRemoveComponent
		/*if (nullptr != this->movingBehavior)
		{
			delete this->movingBehavior;
			this->movingBehavior = nullptr;
		}*/
	}

	bool PlayerControllerClickToPointComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = PlayerControllerComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Categories")
		{
			this->categories->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Range")
		{
			this->range->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PathSlot")
		{
			this->pathSlot->setValue(XMLConverter::getAttribInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnimIdle1")
		{
			this->animations[0]->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "None"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnimIdle2")
		{
			this->animations[1]->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "None"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnimIdle3")
		{
			this->animations[2]->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "None"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnimWalkNorth")
		{
			this->animations[3]->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "None"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnimRun")
		{
			this->animations[4]->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "None"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AutoClick")
		{
			this->autoClick->setValue(XMLConverter::getAttribBool(propertyElement, "data", "None"));
			propertyElement = propertyElement->next_sibling("property");
		}

		return success;
	}

	GameObjectCompPtr PlayerControllerClickToPointComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		PlayerControllerPointToClickCompPtr clonedCompPtr(boost::make_shared<PlayerControllerClickToPointComponent>());

		
		// Do not clone activated, since its no visible and switched manually in game object controller
		// clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setAnimationSpeed(this->animationSpeed->getReal());
		clonedCompPtr->setRotationSpeed(this->rotationSpeed->getReal());
		clonedCompPtr->setGoalRadius(this->goalRadius->getReal());
		clonedCompPtr->setAcceleration(this->acceleration->getReal());
		clonedCompPtr->setCategories(this->categories->getString());
		clonedCompPtr->setRange(this->range->getReal());
		clonedCompPtr->setPathSlot(this->pathSlot->getInt());

		for (unsigned int i = 0; i < static_cast<unsigned int>(this->animations.size()); i++)
		{
			clonedCompPtr->setAnimationName(this->animations[i]->getListSelectedValue(), i);
		}

		clonedCompPtr->setAutoClick(this->autoClick->getBool());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool PlayerControllerClickToPointComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlayerControllerClickToPointComponent] Init player controller click to point component for game object: " + this->gameObjectPtr->getName());

		bool success = PlayerControllerComponent::postInit();

		this->categoriesId = AppStateManager::getSingletonPtr()->getGameObjectController()->generateCategoryId(this->categories->getString());

		Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
		if (nullptr != entity)
		{
			std::vector<Ogre::String> animationNames = this->animationBlender->getAllAvailableAnimationNames();
			// Add also none, so that when choosen, no animation will be done, because it does not exist
			animationNames.insert(animationNames.cbegin(), "None");
			
			// Add all available animation names to list
			for (unsigned short i = 0; i < this->animationsCount; i++)
			{
				this->animations[i]->setValue(animationNames);
			}
			
		}
		else
		{
			Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
			if (nullptr != item)
			{
				std::vector<Ogre::String> animationNames = this->animationBlender->getAllAvailableAnimationNames();
				// Add also none, so that when choosen, no animation will be done, because it does not exist
				animationNames.insert(animationNames.cbegin(), "None");

				// Add all available animation names to list
				for (unsigned short i = 0; i < this->animationsCount; i++)
				{
					this->animations[i]->setValue(animationNames);
				}
			}
		}

		this->stateMachine = new NOWA::KI::StateMachine<GameObject>(this->gameObjectPtr.get());
		this->stateMachine->registerState<PathFollowState3D>(PathFollowState3D::getName());

		// Moving behavior is added and create in game object controller, because even this component, which is a player controller, could also have some other ai components
		// so the moving behavior is shared amongst all components
		this->movingBehaviorPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->addMovingBehavior(this->gameObjectPtr->getId());

		this->movingBehaviorPtr->addBehavior(NOWA::KI::MovingBehavior::FOLLOW_PATH);
		this->movingBehaviorPtr->setRotationSpeed(this->rotationSpeed->getReal());
		this->movingBehaviorPtr->setGoalRadius(this->goalRadius->getReal());
		this->movingBehaviorPtr->setGoalRadius(0.5f);
		this->movingBehaviorPtr->setStuckCheckTime(1000.0f);
		this->movingBehaviorPtr->setFlyMode(false);

		return success;
	}

	bool PlayerControllerClickToPointComponent::connect(void)
	{
		GameObjectComponent::connect(); // Lua script is requested
		bool success = PlayerControllerComponent::connect();

		this->movingBehaviorPtr->setRotationSpeed(this->rotationSpeed->getReal());
		this->movingBehaviorPtr->setGoalRadius(this->goalRadius->getReal());

		this->animationBlender->registerAnimation(NOWA::AnimationBlender::ANIM_IDLE_1, this->animations[0]->getListSelectedValue());
		this->animationBlender->registerAnimation(NOWA::AnimationBlender::ANIM_IDLE_2, this->animations[1]->getListSelectedValue());
		this->animationBlender->registerAnimation(NOWA::AnimationBlender::ANIM_IDLE_3, this->animations[2]->getListSelectedValue());
		this->animationBlender->registerAnimation(NOWA::AnimationBlender::ANIM_WALK_NORTH, this->animations[3]->getListSelectedValue());
		this->animationBlender->registerAnimation(NOWA::AnimationBlender::ANIM_RUN, this->animations[4]->getListSelectedValue());

		this->animationBlender->init(NOWA::AnimationBlender::ANIM_IDLE_1);

		this->stateMachine->setCurrentState(PathFollowState3D::getName());
		
		return success;
	}

	bool PlayerControllerClickToPointComponent::disconnect(void)
	{
		bool success = PlayerControllerComponent::disconnect();

		if (nullptr != this->movingBehaviorPtr && nullptr != this->movingBehaviorPtr->getPath())
		{
			this->movingBehaviorPtr->getPath()->clear();
		}

		if (nullptr != this->stateMachine->getCurrentState())
		{
			this->stateMachine->getCurrentState()->exit(this->gameObjectPtr.get());
		}

		this->animationBlender->clearAnimations();

		AppStateManager::getSingletonPtr()->getCameraManager()->setMoveCameraWeight(1.0f);
		AppStateManager::getSingletonPtr()->getCameraManager()->setRotateCameraWeight(1.0f);

		return success;
	}

	void PlayerControllerClickToPointComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		if (nullptr != this->raySceneQuery)
		{
			this->gameObjectPtr->getSceneManager()->destroyQuery(this->raySceneQuery);
			this->raySceneQuery = nullptr;
		}

		// Dangerous in destructor, as when exiting the simulation, the game object will be deleted and this function called, to seek for another ai component, that has been deleted
		// Thus handle it, just when a component is removed
		bool stillAiComponentActive = false;
		for (size_t i = 0; i < this->gameObjectPtr->getComponents()->size(); i++)
		{
			boost::shared_ptr<AiComponent> aiCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<AiComponent>());
			// Seek for this component and go to the previous one to process
			if (aiCompPtr != nullptr)
			{
				stillAiComponentActive = true;
				break;
			}
		}

		// If there is no ai component for this game object anymore, remove the behavior
		if (false == stillAiComponentActive)
		{
			AppStateManager::getSingletonPtr()->getGameObjectController()->removeMovingBehavior(this->gameObjectPtr->getId());
		}

		AppStateManager::getSingletonPtr()->getCameraManager()->setMoveCameraWeight(1.0f);
		AppStateManager::getSingletonPtr()->getCameraManager()->setRotateCameraWeight(1.0f);
	}

	void PlayerControllerClickToPointComponent::update(Ogre::Real dt, bool notSimulating)
	{
		PlayerControllerComponent::update(dt, notSimulating);
		if (false == notSimulating /*&& true == this->activated->getBool()*/) // activated documented out, else idle animation will not work
		{
			this->stateMachine->update(dt);
		}
		if (true == this->bShowDebugData)
		{
			if (nullptr != this->movingBehaviorPtr)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehavior] Current behavior: " + this->movingBehaviorPtr->getCurrentBehavior());
			}
		}
	}

	void PlayerControllerClickToPointComponent::actualizeValue(Variant* attribute)
	{
		PlayerControllerComponent::actualizeValue(attribute);

		if (PlayerControllerClickToPointComponent::AttrCategories() == attribute->getName())
		{
			this->setCategories(attribute->getString());
		}
		else if (PlayerControllerClickToPointComponent::AttrRange() == attribute->getName())
		{
			this->setRange(attribute->getReal());
		}
		else if (PlayerControllerClickToPointComponent::AttrPathSlot() == attribute->getName())
		{
			this->setPathSlot(attribute->getInt());
		}
		else if (PlayerControllerClickToPointComponent::AttrAnimIdle1() == attribute->getName())
		{
			this->setAnimationName(attribute->getListSelectedValue(), 0);
		}
		else if (PlayerControllerClickToPointComponent::AttrAnimIdle2() == attribute->getName())
		{
			this->setAnimationName(attribute->getListSelectedValue(), 1);
		}
		else if (PlayerControllerClickToPointComponent::AttrAnimIdle3() == attribute->getName())
		{
			this->setAnimationName(attribute->getListSelectedValue(), 2);
		}
		else if (PlayerControllerClickToPointComponent::AttrAnimWalkNorth() == attribute->getName())
		{
			this->setAnimationName(attribute->getListSelectedValue(), 3);
		}
		else if (PlayerControllerClickToPointComponent::AttrAnimRun() == attribute->getName())
		{
			this->setAnimationName(attribute->getListSelectedValue(), 4);
		}
		else if (PlayerControllerClickToPointComponent::AttrAutoClick() == attribute->getName())
		{
			this->setAutoClick(attribute->getBool());
		}
	}

	void PlayerControllerClickToPointComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		PlayerControllerComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Categories"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->categories->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Range"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->range->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "PathSlot"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->pathSlot->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnimIdle1"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animations[0]->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnimIdle2"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animations[1]->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnimIdle3"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animations[2]->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnimWalkNorth"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animations[3]->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnimRun"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->animations[4]->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AutoClick"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->autoClick->getBool())));
		propertiesXML->append_node(propertyXML);
	}

	void PlayerControllerClickToPointComponent::internalShowDebugData(void)
	{
		PlayerControllerComponent::internalShowDebugData();
		this->drawPath = this->bShowDebugData;
	}

	Ogre::String PlayerControllerClickToPointComponent::getClassName(void) const
	{
		return "PlayerControllerClickToPointComponent";
	}

	Ogre::String PlayerControllerClickToPointComponent::getParentClassName(void) const
	{
		return "PlayerControllerComponent";
	}

	KI::StateMachine<GameObject>* PlayerControllerClickToPointComponent::getStateMachine(void) const
	{
		return this->stateMachine;
	}

	NOWA::KI::MovingBehavior* PlayerControllerClickToPointComponent::getMovingBehavior(void) const
	{
		return this->movingBehaviorPtr.get();
	}

	void PlayerControllerClickToPointComponent::setRange(Ogre::Real range)
	{
		this->range->setValue(range);
	}

	Ogre::Real PlayerControllerClickToPointComponent::getRange(void) const
	{
		return this->range->getReal();
	}

	void PlayerControllerClickToPointComponent::setPathSlot(int pathSlot)
	{
		this->pathSlot->setValue(pathSlot);
	}

	int PlayerControllerClickToPointComponent::getPathSlot(void) const
	{
		return this->pathSlot->getUInt();
	}

	void PlayerControllerClickToPointComponent::setAutoClick(bool autoClick)
	{
		this->autoClick->setValue(autoClick);
	}

	bool PlayerControllerClickToPointComponent::getAutoClick(void) const
	{
		return this->autoClick->getBool();
	}

	void PlayerControllerClickToPointComponent::setCategories(const Ogre::String& categories)
	{
		this->categories->setValue(categories);
		this->categoriesId = AppStateManager::getSingletonPtr()->getGameObjectController()->generateCategoryId(categories);
	}

	Ogre::String PlayerControllerClickToPointComponent::getCategories(void) const
	{
		return this->categories->getString();
	}

	unsigned int PlayerControllerClickToPointComponent::getCategoriesId(void) const
	{
		return this->categoriesId;
	}

	bool PlayerControllerClickToPointComponent::getDrawPath(void) const
	{
		return this->drawPath;
	}

	Ogre::RaySceneQuery* PlayerControllerClickToPointComponent::getRaySceneQuery(void) const
	{
		return this->raySceneQuery;
	}

	//---------------------------WalkingState-------------------

	WalkingStateJumpNRun::WalkingStateJumpNRun()
		: playerController(nullptr),
		direction(Direction::NONE),
		directionChanged(false),
		oldDirection(Direction::NONE),
		isJumping(false),
		keyDirection(Ogre::Vector3::ZERO),
		boringTimer(5.0f),
		noMoveTimer(0.0f),
		jumpForce(0.0f),
		inAir(false),
		isAttacking(false),
		tryJump(false),
		highFalling(false),
		jumpKeyPressed(false),
		jumpCount(0),
		canDoubleJump(false),
		walkCount(0.0f),
		isOnRope(false),
		groundedOnce(false),
		duckedOnce(false),
		hasPhysicsPlayerControllerComponent(false),
		acceleration(0.5f)
	{
		
	}

	WalkingStateJumpNRun::~WalkingStateJumpNRun()
	{
		if (this->jumpSound)
		{
			OgreALModule::getInstance()->deleteSound(this->playerController->getOwner()->getSceneManager(), this->walkSound);
			this->walkSound = nullptr;
		}
		if (this->jumpSound)
		{
			OgreALModule::getInstance()->deleteSound(this->playerController->getOwner()->getSceneManager(), this->jumpSound);
			this->jumpSound = nullptr;
		}
	}

	void WalkingStateJumpNRun::enter(GameObject* player)
	{
		this->playerController = NOWA::makeStrongPtr(player->getComponent<PlayerControllerJumpNRunComponent>()).get();
		this->walkSound = OgreALModule::getInstance()->createSound(this->playerController->getOwner()->getSceneManager(), "PlayerWalk1", "Walk.wav");
		this->walkSound->setGain(0.5f);
		this->jumpSound = OgreALModule::getInstance()->createSound(this->playerController->getOwner()->getSceneManager(), "PlayerJump1", "Jump1.wav");
		this->jumpSound->setGain(0.5f);

		this->playerController->getAnimationBlender()->blend(NOWA::AnimationBlender::ANIM_IDLE_1, NOWA::AnimationBlender::BlendThenAnimate, 0.2f, true);
		this->boringTimer = 0.0f;
		this->noMoveTimer = 0.0f;
		// acquire the jump force from attributes
		// this->jumpForce = this->playerController->getAttributesComponent()->getAttribute("AttributeJumpForce")->getValueReal();
		this->jumpForce = this->playerController->getJumpForce();

		PhysicsPlayerControllerComponent* physicsPlayerControllerComponent = dynamic_cast<PhysicsPlayerControllerComponent*>(this->playerController->getPhysicsComponent());
		if (nullptr != physicsPlayerControllerComponent)
			this->hasPhysicsPlayerControllerComponent = true;
	}

	void WalkingStateJumpNRun::update(GameObject* player, Ogre::Real dt)
	{
		auto widget = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Window>("manipulationWindow", false);

		if (this->noMoveTimer > 0.1f)
		{
			this->playerController->setMoveWeight(0.0f);
			this->playerController->setJumpWeight(0.0f);
			this->noMoveTimer -= dt;
		}
		else if (this->noMoveTimer < 0.1f)
		{
			// Call this only once, hence noMoveTimer = 0.1f
			this->playerController->setMoveWeight(1.0f);
			this->playerController->setJumpWeight(1.0f);
			this->noMoveTimer = 0.1f;
		}

		
		if (widget) widget->setCaption("");

		Ogre::Real tempSpeed = 0.0f;
		Ogre::Real tempAnimationSpeed = this->playerController->getAnimationSpeed();
		Ogre::Radian heading = this->playerController->getOrientation().getYaw();

		// Idle stop movement at the beginning
		if (true == this->hasPhysicsPlayerControllerComponent)
		{
			static_cast<PhysicsPlayerControllerComponent*>(this->playerController->getPhysicsComponent())->move(0.0f, 0.0f, heading);
		}

		Ogre::Real height = this->playerController->getHeight();
		this->keyDirection = Ogre::Vector3::ZERO;

		auto b = this->playerController->getOwner()->getBottomOffset().y;

		this->inAir = height - this->playerController->getOwner()->getBottomOffset().y > 0.2f;

		///if (widget) widget->setCaption("In air: " + Ogre::StringConverter::toString(this->inAir));

		// Store the old direction, the check later whether a direction changed or not
		this->oldDirection = this->direction;
		// Set omega by default directly to zero, else there is an ugly small rotation...
		this->playerController->getPhysicsComponent()->getBody()->setOmega(Ogre::Vector3::ZERO);

		InputDeviceModule* inputDeviceModule = NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0);

		// if no key is pressed, go to idle state
		if (!inputDeviceModule->isActionDown(NOWA_A_UP) && !inputDeviceModule->isActionDown(NOWA_A_DOWN)
			&& !inputDeviceModule->isActionDown(NOWA_A_LEFT) && !inputDeviceModule->isActionDown(NOWA_A_RIGHT)
			&& !inputDeviceModule->isActionDown(NOWA_A_UP) && !this->jumpKeyPressed && !this->isJumping)
		{
			this->walkCount = 0.0f;
			this->keyDirection = Ogre::Vector3::ZERO;
			if (!this->playerController->getAnimationBlender()->isAnimationActive(AnimationBlender::ANIM_IDLE_1) /*&& this->playerController->getAnimationBlender()->isComplete()*/ && false == this->inAir)
			{
				this->playerController->getAnimationBlender()->blend(AnimationBlender::ANIM_IDLE_1, AnimationBlender::BlendWhileAnimating, 0.2f, true);
			}

			tempAnimationSpeed = this->playerController->getAnimationSpeed() * 0.5f;
			this->direction = Direction::NONE;

			// No acceleration, move with usual constant speed
			if (0.0f == this->playerController->getAcceleration())
				this->acceleration = 1.0f;
			else
			{
				this->acceleration -= this->playerController->getAcceleration() * dt;
				if (this->acceleration <= 0.5f)
					this->acceleration = 0.5f;
			}

			this->boringTimer += dt;

			if (true == this->inAir)
				this->boringTimer = 0;
			// if the user does nothing for 10 seconds, choose a random boring animation state
			if (this->boringTimer >= 10.0f)
			{
				this->boringTimer = 0.0f;
				int id = MathHelper::getInstance()->getRandomNumber<int>(0, 3);
				AnimationBlender::AnimID animID;
				if (0 == id)
				{
					animID = AnimationBlender::ANIM_IDLE_1;
				}
				else if (1 == id)
				{
					animID = AnimationBlender::ANIM_IDLE_2;
				}
				else
				{
					animID = AnimationBlender::ANIM_IDLE_3;
				}
				if (this->playerController->getAnimationBlender()->hasAnimation(animID) && this->playerController->getAnimationBlender()->isComplete())
				{
					tempAnimationSpeed = this->playerController->getAnimationSpeed();
					this->playerController->getAnimationBlender()->blend(animID, AnimationBlender::BlendWhileAnimating, 0.2f, true);
				}
			}

			if (this->playerController->isIdle() /*&& !this->playerController->getPhysicsComponent()->getHasBuoyancy()*/)
			{
				this->playerController->getPhysicsComponent()->setVelocity(this->playerController->getPhysicsComponent()->getVelocity() * Ogre::Vector3(0.0f, 1.0f, 0.0f));
			}
		}
		else if (NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->isActionDown(NOWA_A_UP) || NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->isActionDown(NOWA_A_DOWN)
			|| NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->isActionDown(NOWA_A_LEFT) || NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->isActionDown(NOWA_A_RIGHT))
		{
			this->boringTimer = 0;

			// No acceleration, move with usual constant speed
			if (0.0f == this->playerController->getAcceleration())
				this->acceleration = 1.0f;
			else
			{
				this->acceleration += this->playerController->getAcceleration() * dt;
				if (this->acceleration >= 1.0f)
					this->acceleration = 1.0f;
				else
				{
					Ogre::Real weight = this->acceleration + 0.5f;
					if (weight > 1.0f)
						weight = 1.0f;
					// this->playerController->getAnimationBlender()->setWeight(weight);
				}
			}
			
			tempAnimationSpeed = this->playerController->getAnimationSpeed();
			// walk if left or right is being pressed
			this->boringTimer = 0.0f;

			AnimationBlender::AnimID animId = AnimationBlender::AnimID::ANIM_NONE;

			// 3D movement
			if (false == this->playerController->getIsFor2D())
			{
				Ogre::Real yawAtSpeed = 0.0f;
				// set the key direction, so that the player will always run on x-axis
				if (NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->isActionDown(NOWA_A_UP))
				{
					tempSpeed = this->playerController->getPhysicsComponent()->getSpeed() * this->playerController->getMoveWeight();
					animId = AnimationBlender::ANIM_WALK_NORTH;
					this->direction = Direction::UP;
					this->keyDirection += this->playerController->getOrientation() * Ogre::Vector3::UNIT_Z;
				}
				else if (NOWA::InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->isActionDown(NOWA_A_DOWN))
				{
					tempAnimationSpeed = this->playerController->getAnimationSpeed() * 0.75f;
					// Free from stuck when moving back
					this->playerController->setMoveWeight(1.0f);
					tempSpeed = -this->playerController->getPhysicsComponent()->getSpeed() * 0.5f * this->playerController->getMoveWeight();
					animId = AnimationBlender::ANIM_WALK_SOUTH;
					this->direction = Direction::DOWN;
					this->keyDirection += this->playerController->getOrientation() * Ogre::Vector3::UNIT_Z;
				}
				if (inputDeviceModule->isActionDown(NOWA_A_LEFT))
				{
					animId = AnimationBlender::ANIM_WALK_WEST;
					this->direction = Direction::LEFT;
					yawAtSpeed = 3.0f * this->playerController->getRotationSpeed();
					heading = heading + Ogre::Radian(this->playerController->getRotationSpeed()) * 0.1f;
				}
				else if (inputDeviceModule->isActionDown(NOWA_A_RIGHT))
				{
					animId = AnimationBlender::ANIM_WALK_EAST;
					this->direction = Direction::RIGHT;
					yawAtSpeed = -3.0f * this->playerController->getRotationSpeed();
					heading = heading - Ogre::Radian(this->playerController->getRotationSpeed()) * 0.1f;
				}

				this->playerController->getPhysicsComponent()->getBody()->setOmega(Ogre::Vector3(0.0f, yawAtSpeed, 0.0f));
				// heading = this->playerController->getPhysicsComponent()->getOrientation().getYaw();
			}
			else
			{
				// JumpNRun 2D treatement: Rotate til 90 degree at once

				if (true == this->directionChanged)
				{
					// if the player changes the direction, set the weight to 0, so that the player does not move will turning
					// this->playerController->setMoveWeight(0.0f);
				}

				if (inputDeviceModule->isActionDown(NOWA_A_LEFT))
				{
					if (this->playerController->getRunAfterWalkTime() > 0.0f)
						this->walkCount += dt;
					// Walked long enough without pause -> run
					if (this->walkCount >= this->playerController->getRunAfterWalkTime())
					{
						animId = AnimationBlender::ANIM_RUN;
						tempSpeed = this->playerController->getPhysicsComponent()->getMaxSpeed() * this->playerController->getMoveWeight();
						tempAnimationSpeed *= 0.5f;
					}
					else
					{
						tempSpeed = this->playerController->getPhysicsComponent()->getSpeed() * this->playerController->getMoveWeight();
						animId = AnimationBlender::ANIM_WALK_NORTH;
					}
					this->direction = Direction::LEFT;
					this->keyDirection = Ogre::Vector3(-1.0f, 0.0f, 0.0f);
				}
				else if (inputDeviceModule->isActionDown(NOWA_A_RIGHT))
				{
					if (this->playerController->getRunAfterWalkTime() > 0.0f)
						this->walkCount += dt;
					// Walked long enough without pause -> run
					if (this->walkCount >= this->playerController->getRunAfterWalkTime())
					{
						animId = AnimationBlender::ANIM_RUN;
						tempSpeed = this->playerController->getPhysicsComponent()->getMaxSpeed() * this->playerController->getMoveWeight();
						tempAnimationSpeed *= 0.5f;
					}
					else
					{
						tempSpeed = this->playerController->getPhysicsComponent()->getSpeed() * this->playerController->getMoveWeight();
						animId = AnimationBlender::ANIM_WALK_NORTH;
					}
					this->direction = Direction::RIGHT;
					this->keyDirection = Ogre::Vector3(1.0f, 0.0f, 0.0f);
				}

				// If the old direction does not match the current direction
				// the player changed direction, but this only set the direction change to true, never to false, which is done, after a 90 degree turn has been accomplished
				if (this->oldDirection != this->direction)
				{
					this->directionChanged = true;
					this->walkCount = 0.0f;
				}

				// if the player changed direction, rotate the player from x to -x and vice version (+- 180 degree), after that directionChanged is set to false
				if (true == this->directionChanged)
				{
					this->boringTimer = 0.0f;

					Ogre::Real yawAtSpeed = 0.0f;
					Ogre::Real currentDegree = this->playerController->getPhysicsComponent()->getOrientation().getYaw().valueDegrees();
					if (Direction::RIGHT == this->direction)
					{
						yawAtSpeed = this->playerController->getRotationSpeed();
						if (currentDegree >= 90.0f)
						{
							this->directionChanged = false;
							yawAtSpeed = 0.0f;
							this->playerController->getPhysicsComponent()->setOrientation(Ogre::Quaternion(Ogre::Degree(90.0f), Ogre::Vector3::UNIT_Y));
						}
					}
					else if (Direction::LEFT == this->direction)
					{
						yawAtSpeed = -this->playerController->getRotationSpeed();
						if (currentDegree <= -90.0f)
						{
							this->directionChanged = false;
							yawAtSpeed = 0.0f;
							this->playerController->getPhysicsComponent()->setOrientation(Ogre::Quaternion(Ogre::Degree(-90.0f), Ogre::Vector3::UNIT_Y));
						}
					}
					this->playerController->getPhysicsComponent()->getBody()->setOmega(Ogre::Vector3(0.0f, yawAtSpeed, 0.0f));
				}
				else
				{
					// player should not rotate without intention
					if (this->direction == Direction::LEFT)
					{
						this->playerController->getPhysicsComponent()->setOrientation(Ogre::Quaternion(Ogre::Degree(-90.0f), Ogre::Vector3::UNIT_Y));
					}

					else
					{
						this->playerController->getPhysicsComponent()->setOrientation(Ogre::Quaternion(Ogre::Degree(90.0f), Ogre::Vector3::UNIT_Y));
					}
				}
			}

			// Run or sneak (only possible when moving forward)
			if (false == this->playerController->getIsFor2D())
			{
				if (inputDeviceModule->isActionDown(NOWA_A_RUN) /*&& NOWA::Core::getSingletonPtr()->getInputDeviceModule(0)->isActionDown(NOWA_A_UP)*/)
				{
					this->direction = Direction::UP;
					tempSpeed = this->playerController->getPhysicsComponent()->getMaxSpeed() * this->playerController->getMoveWeight();
					tempAnimationSpeed *= 0.8f;
					animId = AnimationBlender::ANIM_RUN;
				}
				else if (inputDeviceModule->isActionDown(NOWA_A_SNEAK) && inputDeviceModule->isActionDown(NOWA_A_UP))
				{
					this->direction = Direction::UP;
					tempSpeed = this->playerController->getPhysicsComponent()->getMinSpeed() * this->playerController->getMoveWeight();
					tempAnimationSpeed *= 0.5f;
					animId = AnimationBlender::ANIM_SNEAK;
				}
			}

			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ActionsStates] --->data: inAir" + Ogre::StringConverter::toString(this->inAir)
			// + " ");

			if (false == this->playerController->getAnimationBlender()->isAnimationActive(animId)
				&& (this->playerController->getAnimationBlender()->isComplete() || this->playerController->getAnimationBlender()->isAnimationActive(AnimationBlender::ANIM_IDLE_1)
					|| this->playerController->getAnimationBlender()->isAnimationActive(AnimationBlender::ANIM_IDLE_2)
					|| this->playerController->getAnimationBlender()->isAnimationActive(AnimationBlender::ANIM_IDLE_3))
					/*&& false == this->playerController->getPhysicsComponent()->getHasBuoyancy()*/
				&& false == this->inAir && !this->jumpKeyPressed && !this->isJumping)
			{
				tempAnimationSpeed = this->playerController->getAnimationSpeed();
				// 0.02f: Immediately blend to walk
				this->playerController->getAnimationBlender()->blend(animId, NOWA::AnimationBlender::BlendWhileAnimating, 0.2f, true);
			}
		}
// Attention: Does not work at the moment, some condition is wrong
		else if (inputDeviceModule->isActionDown(NOWA_A_DUCK))
		{
			this->boringTimer = 0;
		
			if (false == this->playerController->getAnimationBlender()->isAnimationActive(AnimationBlender::ANIM_DUCK) && false == this->duckedOnce)
			{
				this->duckedOnce = true;
				this->playerController->getAnimationBlender()->blend(AnimationBlender::ANIM_DUCK, AnimationBlender::BlendWhileAnimating, 0.1f, false);
				// this->playerController->getPhysicsComponent()->getBody()->scaleCollision(Ogre::Vector3(1.0f, 0.5f, 1.0f));
			}
			if (this->playerController->getAnimationBlender()->getTimePosition() >= this->playerController->getAnimationBlender()->getLength() - 0.3f)
			{
				this->playerController->getAnimationBlender()->setTimePosition(0.7f);
			}
		}
		else if (false == inputDeviceModule->isActionDown(NOWA_A_DUCK) && true == this->duckedOnce)
		{
			this->boringTimer = 0;
			this->playerController->getPhysicsComponent()->getBody()->scaleCollision(Ogre::Vector3(1.0f, 1.0f, 1.0f));
			this->duckedOnce = false;
		}

		Ogre::Vector3 directionMove = this->keyDirection * tempSpeed * this->acceleration;

		this->jumpKeyPressed = false;

		if (inputDeviceModule->isActionDown(NOWA_A_JUMP))
		{
			if (true == this->canDoubleJump)
			{
				this->jumpCount += 1;
				this->canDoubleJump = false;
				if (this->jumpCount > 2)
					this->jumpCount = 0;
			}

			if (false == this->tryJump)
				this->jumpKeyPressed = true;
		}

		if (false == inAir && this->jumpKeyPressed && false == this->isJumping)
		{
			this->boringTimer = 0;

			// Jump at place, back or sidewards
#if 0
			if ((Direction::NONE == this->direction || Direction::DOWN == this->direction || Direction::LEFT == this->direction || Direction::RIGHT == this->direction)
				/*&& false == this->playerController->getAnimationBlender()->isAnimationActive(AnimationBlender::ANIM_JUMP_START)*/
				/*&& this->playerController->getAnimationBlender()->isComplete()*/ /*&& this->playerController->getPhysicsComponent()->getVelocity().squaredLength() <= 2.0f*/)
#endif
			{
				// tempAnimationSpeed = this->playerController->getAnimationSpeed() * 0.5f;
				this->playerController->getAnimationBlender()->blend(AnimationBlender::ANIM_JUMP_START, AnimationBlender::BlendWhileAnimating, 0.5f, false);
		
				if (this->playerController->getAnimationBlender()->getTimePosition() >= this->playerController->getAnimationBlender()->getLength() - 0.3f)
				{
					this->playerController->getAnimationBlender()->setTimePosition(0.7f);
				}
				this->jumpSound->play();
			}
			// Jump forward
			//else if (Direction::UP == this->direction)
			//{
			//	// start jumping animation directly and not waiting until complete
			//	// if the player is running and jumping, choose the jump walk animation if it does exist
			//	if (this->playerController->getAnimationBlender()->hasAnimation(AnimationBlender::ANIM_JUMP_WALK)
			//		&& false == this->playerController->getAnimationBlender()->isAnimationActive(AnimationBlender::ANIM_JUMP_WALK))
			//	{
			//		// tempAnimationSpeed = this->playerController->getAnimationSpeed() * 0.75f;
			//		this->playerController->getAnimationBlender()->blend(AnimationBlender::ANIM_JUMP_WALK, AnimationBlender::BlendWhileAnimating, 0.5f, false);
			//		this->jumpSound->play();
			//	}
			//}
		}

		// When just falling, do and keep jump start animation (does not work yet)
#if 1
		else if (true == inAir && !this->jumpKeyPressed && this->playerController->getPhysicsComponent()->getVelocity().y < -1.0f)
			// if (false == inAir && !this->jumpKeyPressed && this->playerController->getPhysicsComponent()->getVelocity().y < -1.0f)
		{
			this->jumpCount = 0;
			//if (widget) widget->setCaption("Falling");
			if (false == this->playerController->getAnimationBlender()->isAnimationActive(AnimationBlender::ANIM_JUMP_START))
			{
				this->playerController->getAnimationBlender()->blend(AnimationBlender::ANIM_JUMP_START, AnimationBlender::BlendWhileAnimating, 0.5f, false);
			}
			else if (this->playerController->getAnimationBlender()->getTimePosition() >= this->playerController->getAnimationBlender()->getLength() - 0.3f)
			{
				this->playerController->getAnimationBlender()->setTimePosition(this->playerController->getAnimationBlender()->getTimePosition());
			}
		}

		if (this->playerController->getPhysicsComponent()->getVelocity().y < -1.0f)
		{
			//if (widget) widget->setCaption("Falling");
		}
#endif

#if 0
// Attention: HighFalling false, because when jumping from one platform to another, there is a gab between the plattforms that is more than 4 in any case
		// so the player will always play the ANIM_JUMP_END, even the player started at the same y position! So take y position into account!
		// Signal that player is high falling for jump end animation
		if (height > 5.0f + this->playerController->getMiddleOfPlayer().y)
		{
			this->highFalling = true;
		}

		// Player was high falling and is now at the floor
		if (true == this->highFalling && false == this->inAir && this->playerController->getPhysicsComponent()->getVelocity().y < -1.0f)
		{
			this->boringTimer = 0;
			this->acceleration = 0.0f;
			tempAnimationSpeed = this->playerController->getAnimationSpeed() * 3.0f;
			if (!this->playerController->getAnimationBlender()->isAnimationActive(AnimationBlender::ANIM_HIGH_JUMP_END)
				/*&& this->playerController->getAnimationBlender()->isComplete()*/)
			{
				// this->noMoveTimer = 1.0f;
				this->playerController->getAnimationBlender()->blend(AnimationBlender::ANIM_HIGH_JUMP_END, AnimationBlender::BlendWhileAnimating, 0.5f, false);

				///////////////if (widget) widget->setCaption("Land");
				this->playerController->getPhysicsComponent()->setVelocity(this->playerController->getPhysicsComponent()->getVelocity() * Ogre::Vector3(0.0f, 1.0f, 0.0f));

				this->highFalling = false;
				// if (widget) widget->setCaption("Jump End");
			}
		}
#endif

		if (false == this->inAir && this->playerController->getPhysicsComponent()->getVelocity().y < -1.0f && false == this->groundedOnce)
		{
			if (!this->playerController->getAnimationBlender()->isAnimationActive(AnimationBlender::ANIM_JUMP_END))
			{
				this->playerController->getAnimationBlender()->blend(AnimationBlender::ANIM_JUMP_END, AnimationBlender::BlendWhileAnimating, 0.5f, false);
			}
			this->boringTimer = 0;
			// Play only once!
			this->walkSound->setPitch(0.35f);
			this->walkSound->setGain(0.55f);
			this->walkSound->play();
			AppStateManager::getSingletonPtr()->getParticleUniverseModule()->getParticle("smoke" + Ogre::StringConverter::toString(this->playerController->getOwner()->getId()))->particleNode->setPosition(this->playerController->getPosition() + Ogre::Vector3(0.0f, -0.1f, 1.0f));
			AppStateManager::getSingletonPtr()->getParticleUniverseModule()->playParticleSystem("smoke" + Ogre::StringConverter::toString(this->playerController->getOwner()->getId()));
			this->groundedOnce = true;
		}

		if (height > 2.0f)
			this->groundedOnce = false;

		if (1.0f == this->playerController->getJumpWeight() && (this->jumpKeyPressed && (false == this->isJumping && false == this->inAir) || (this->playerController->getDoubleJump() && this->jumpCount == 2)))
		{
			this->boringTimer = 0.0f;
			// Jumpforce here no more required?
			Ogre::Vector3 velocity = this->playerController->getPhysicsComponent()->getVelocity();
			if (this->playerController->getPhysicsComponent()->getVelocity().y <= this->jumpForce * this->jumpCount)
			{
				if (false == this->hasPhysicsPlayerControllerComponent)
					this->playerController->getPhysicsComponent()->addImpulse(Ogre::Vector3(0.0f, this->jumpForce * this->playerController->getJumpWeight(), 0.0f));
				else
				{
					static_cast<PhysicsPlayerControllerComponent*>(this->playerController->getPhysicsComponent())->move(0.0f, tempSpeed, heading);
				}
				if (this->jumpCount >= 2)
					jumpCount = 0;
			}
			else
			{
				this->isJumping = true;
				this->playerController->setJumpWeight(0.0f);
				// if (widget) widget->setCaption("Plop");
			}
		}

		/*if (widget) widget->setCaption("Jump Forward: " + Ogre::StringConverter::toString(tempSpeed));*/

		// if the player is on ground and not jumping and the jump key pressed 
		if (true == this->jumpKeyPressed && (false == this->inAir && false == this->isJumping) /*|| this->playerController->getPhysicsComponent()->getHasBuoyancy()*/)
		{
			this->tryJump = true;
		}



		// if (widget) widget->setCaption("jumpCount: " + Ogre::StringConverter::toString(this->jumpCount));

		// if (widget) widget->setCaption("In Air: " + Ogre::StringConverter::toString(this->inAir));

		// this->playerController->getPhysicsComponent()->applyForce(playerForce);
		// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ActionsStates] force: " + Ogre::StringConverter::toString(playerForce));
		// 0.9
		if (/*height >= 0.6f && height <= 0.7f*/ true == inAir && true == this->isJumping)
		{
			// this->walkSound->play();
			// this->walkSound->setPitch(0.5f);
			// if (widget) widget->setCaption("Plop");
		}

		// in order that the player does not jump again and again if the jump key is hold, only set isJumping to false if the player released the jump key
		// so that he can jump if jump key is pressed again
		if (false == inputDeviceModule->isActionDown(NOWA_A_JUMP))
		{
			this->tryJump = false;
			this->isJumping = false;
			this->canDoubleJump = true;
		}

		//if (NOWA::Core::getSingletonPtr()->getInputDeviceModule(0)->isKeyDown(NOWA_K_ACTION_1) && !this->isAttacking)
		//{
		//	this->playerController->getStateMaschine()->setChildState(AttackingState2D::getName());
		//	this->isAttacking = true;
		//}

		//if (!NOWA::Core::getSingletonPtr()->getInputDeviceModule(0)->isKeyDown(NOWA_K_ACTION_1))
		//{
		//	this->isAttacking = false;
		//}

		//if (NOWA::Core::getSingletonPtr()->getInputDeviceModule(0)->isKeyDown(NOWA_K_UP) && !this->isOnRope)
		//{
		//	for (auto& getcontactDataFront : this->playerController->getcontactDataFront())
		//	{
		//		if (nullptr != getcontactDataFront && getcontactDataFront->getCategory() == "Rope")
		//		{
		//			this->playerController->getStateMaschine()->changeState(RopeState2D::getName());
		//		}
		//	}
		//}
		//if (!NOWA::Core::getSingletonPtr()->getInputDeviceModule(0)->isKeyDown(NOWA_K_UP))
		//{
		//	this->isOnRope = false;
		//}

		
		// Prevent, that the player does bounce after falling
		if (false == this->inAir && this->playerController->getPhysicsComponent()->getVelocity().y < -1.0f)
		{
			this->playerController->getPhysicsComponent()->applyRequiredForceForVelocity(directionMove);
			// this->playerController->getAnimationBlender()->setTimePosition(this->playerController->getAnimationBlender()->getLength() - 0.1f);
			// if (widget) widget->setCaption("Land");
		}
		else
		{
			if (false == this->hasPhysicsPlayerControllerComponent)
			{
				this->playerController->getPhysicsComponent()->applyRequiredForceForVelocity(this->playerController->getPhysicsComponent()->getVelocity() * Ogre::Vector3(0.0f, 1.0f, 0.0f) + directionMove);
			}
			else
			{
				static_cast<PhysicsPlayerControllerComponent*>(this->playerController->getPhysicsComponent())->move(0.0f, tempSpeed, heading);
			}

			if (/*!this->walkSound->isPlaying() &&*/ false == this->inAir && this->direction != Direction::NONE)
			{
				// this->walkSound->setSecondOffset(2.0f);
				this->walkSound->play();
				// this->walkSound->setRolloffFactor()
				this->walkSound->setVelocity(this->playerController->getPhysicsComponent()->getVelocity());
				this->walkSound->setPitch(0.65f);
				this->walkSound->setGain(0.35f);
			}
		}

		/*if (widget) widget->setCaption("inAir: " + Ogre::StringConverter::toString(this->inAir) 
			+ " height: " + Ogre::StringConverter::toString(height));*/
		
		// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "height: " + Ogre::StringConverter::toString(height));

		this->playerController->getAnimationBlender()->addTime(dt * tempAnimationSpeed / this->playerController->getAnimationBlender()->getLength());
	}

	void WalkingStateJumpNRun::exit(GameObject* player)
	{

	}

	//---------------------------PathFollowState3D-------------------

	class UpdateCameraBehaviorProcess : public NOWA::Process
	{
	public:
		explicit UpdateCameraBehaviorProcess()
		{

		}
	protected:
		virtual void onInit(void) override
		{
			AppStateManager::getSingletonPtr()->getCameraManager()->setMoveCameraWeight(1.0f);
			AppStateManager::getSingletonPtr()->getCameraManager()->setRotateCameraWeight(1.0f);

			this->succeed();
		}

		virtual void onUpdate(float dt) override
		{
			this->succeed();
		}
	};

	PathFollowState3D::PathFollowState3D()
		: playerController(nullptr),
		animationSpeed(1.0f),
		boringTimer(5.0f),
		movingBehavior(nullptr),
		hasGoal(false),
		raySceneQuery(nullptr),
		ogreRecastModule(AppStateManager::getSingletonPtr()->getOgreRecastModule()),
		canClick(true),
		mouseX(0),
		mouseY(0)
	{
		/*this->walkSound = OgreALModule::getInstance()->createSound("PlayerWalk1", "Walk.wav");
		this->walkSound->setGain(0.5f);
		this->jumpSound = OgreALModule::getInstance()->createSound("PlayerJump1", "Jump1.wav");
		this->jumpSound->setGain(0.5f);*/
	}

	PathFollowState3D::~PathFollowState3D()
	{
		/*if (this->jumpSound)
		{
			OgreALModule::getInstance()->deleteSound(this->walkSound);
			this->walkSound = nullptr;
		}
		if (this->jumpSound)
		{
			OgreALModule::getInstance()->deleteSound(this->jumpSound);
			this->jumpSound = nullptr;
		}*/
	}

	void PathFollowState3D::enter(GameObject* player)
	{
		this->playerController = NOWA::makeStrongPtr(player->getComponent<PlayerControllerClickToPointComponent>()).get();
		// this->playerController->getAnimationBlender()->init(NOWA::AnimationBlender::ANIM_IDLE_1);
		this->playerController->getAnimationBlender()->blend(NOWA::AnimationBlender::ANIM_IDLE_1, NOWA::AnimationBlender::BlendWhileAnimating, 0.5f, true);
		this->boringTimer = 6.0f;
		
		this->movingBehavior = this->playerController->getMovingBehavior();
		this->raySceneQuery = this->playerController->getRaySceneQuery();
		this->raySceneQuery = this->playerController->getOwner()->getSceneManager()->createRayQuery(Ogre::Ray(), this->playerController->getCategoriesId());
		this->raySceneQuery->setSortByDistance(true);

		this->movingBehavior->addBehavior(NOWA::KI::MovingBehavior::FOLLOW_PATH);

		if (nullptr != this->playerController->getCameraBehaviorComponent())
		{
			AppStateManager::getSingletonPtr()->getCameraManager()->setMoveCameraWeight(1.0f);
			AppStateManager::getSingletonPtr()->getCameraManager()->setRotateCameraWeight(1.0f);
		}

		if (false == this->playerController->getAutoClick())
		{
			this->canClick = false;
		}
	}

	void PathFollowState3D::update(GameObject* player, Ogre::Real dt)
	{
		const OIS::MouseState& ms = NOWA::InputDeviceCore::getSingletonPtr()->getMouse()->getMouseState();

		Ogre::Real tempSpeed = this->playerController->getPhysicsComponent()->getSpeed();
		Ogre::Real tempAnimationSpeed = this->playerController->getAnimationSpeed();

		if (true == this->playerController->getGameObject()->isSelected() && true == ms.buttonDown(OIS::MB_Middle))
		{
			// Attention: Why has this been used? Because during simulation the camera transform will be slow
			/*AppStateManager::getSingletonPtr()->getCameraManager()->setMoveCameraWeight(0.1f);
			AppStateManager::getSingletonPtr()->getCameraManager()->setRotateCameraWeight(0.1f);*/

			// Add delay to camera behavior if target location has been clicked, so that the scene will not be rotated to early
			if (nullptr != this->playerController->getCameraBehaviorComponent())
			{
				NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.5f));
				delayProcess->attachChild(NOWA::ProcessPtr(new UpdateCameraBehaviorProcess()));
				NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
			}

			// Get target position
			/*std::pair<bool, Ogre::Vector3> result = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getTargetBodyPosition(ms.X.abs, ms.Y.abs,
				AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera(), this->playerController->getPhysicsComponent()->getOgreNewt(),
				this->playerController->getCategoriesId(), this->playerController->getRange(), true);*/
			Ogre::Vector3 clickedPosition = Ogre::Vector3::ZERO;
			Ogre::Real closestDistance = 0.0f;
			Ogre::Vector3 normal = Ogre::Vector3::ZERO;
			Ogre::MovableObject* movableObject = nullptr;

			std::vector<Ogre::MovableObject*> excludeObjects;
			// Exclude the player, else the final way point can be placed on the top of the player^^
			excludeObjects.emplace_back(this->playerController->getOwner()->getMovableObject());
			// No: Else if unvalid path has been found, player can no more get to other target position
			//if (nullptr != this->playerController->debugWaypointNode)
			//{
			//	// Exclude also created waypoint entity
			//	excludeObjects.emplace_back(this->playerController->debugWaypointNode->getAttachedObject(0));
			//}

			if (true == this->canClick)
			{
				this->mouseX = ms.X.abs;
				this->mouseY = ms.Y.abs;

				if (false == this->playerController->getAutoClick())
				{
					this->canClick = false;
				}
			}

			bool success = MathHelper::getInstance()->getRaycastFromPoint(this->mouseX, this->mouseY, AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera(),
				Core::getSingletonPtr()->getOgreRenderWindow(), this->raySceneQuery, clickedPosition, (size_t&)movableObject, closestDistance, normal, &excludeObjects, false);

			if (true == success)
			{
				if (nullptr != this->ogreRecastModule)
				{
					this->ogreRecastModule->getOgreRecast()->getPath(this->playerController->getPathSlot()).clear();
					Ogre::Vector3 posOnNavMesh = Ogre::Vector3::ZERO;

					// http://www.stevefsp.org/projects/rcndoc/prod/classdtNavMeshQuery.html
					// https://forums.ogre3d.org/viewtopic.php?t=62079
					if (true == this->ogreRecastModule->getOgreRecast()->findNearestPointOnNavmesh(clickedPosition + Ogre::Vector3(0.0f, 0.3f, 0.0f), posOnNavMesh))
					{
						// y immer 0.5 statt höher argghhh
						// Ogre::LogManager::getSingletonPtr()->logMessage("findNearestPointOnNavmesh: " + Ogre::StringConverter::toString(posOnNavMesh));

						if (nullptr != this->movingBehavior->getPath())
						{
							this->movingBehavior->getPath()->clear();
						}

						// Attention: What is with targetSlot? Its here 0
						bool foundPath = this->ogreRecastModule->findPath(this->playerController->getPosition(), posOnNavMesh + Ogre::Vector3(0.0f, 0.3f, 0.0f), this->playerController->getPathSlot(), 0, this->playerController->getDrawPath());
						/*Ogre::LogManager::getSingletonPtr()->logMessage("#############findPath size: "
						+ Ogre::StringConverter::toString(this->ogreRecastModule->getOgreRecast()->getPath(0).size())
						+ " y offset: " + Ogre::StringConverter::toString(this->ogreRecastModule->getOgreRecast()->getNavmeshOffsetFromGround()));*/

						if (false == foundPath)
						{
							// Ogre::LogManager::getSingletonPtr()->logMessage("No path found!!!");
							// this->movingBehavior->reset();
							// this->movingBehavior->setBehavior(KI::MovingBehavior::NONE);
						}
						else
						{
							std::vector<Ogre::Vector3> path = this->ogreRecastModule->getOgreRecast()->getPath(this->playerController->getPathSlot());

							LuaScript* luaScript = this->playerController->getOwner()->getLuaScript();
							if (nullptr != luaScript)
							{
								luaScript->callTableFunction("onNavMeshClicked", posOnNavMesh);
							}
							
							this->hasGoal = true;
							this->movingBehavior->getPath()->clear();

							// Add each time behavior, because if goal is reached, or no path, the behavior will be removed
							this->movingBehavior->removeBehavior(NOWA::KI::MovingBehavior::FOLLOW_PATH);
							this->movingBehavior->addBehavior(NOWA::KI::MovingBehavior::FOLLOW_PATH);

							if (false == path.empty())
							{
								for (size_t i = 0; i < path.size(); i++)
								{
#if 0
									Ogre::Vector3 waypoint = path[i];
									waypoint.y += this->playerController->getOwner()->getPosition().y /** 2.0f*/;
									Ogre::Vector3 resultWaypoint = Ogre::Vector3::ZERO;

									std::vector<Ogre::MovableObject*> excludeObject = std::vector<Ogre::MovableObject*>(1);
									// Exclude the player, else the final way point can be placed on the top of the player^^
									excludeObject[0] = this->playerController->getOwner()->getMovableObject();
									bool success = MathHelper::getInstance()->getRaycastResult(waypoint, Ogre::Vector3::NEGATIVE_UNIT_Y, this->raySceneQuery, resultWaypoint, (size_t&)movableObject, &excludeObject);
									if (false == success)
									{
										waypoint = path[i];
									}
									else
									{
										waypoint.y = resultWaypoint.y;
									}
#endif

									Ogre::Vector3 resultWaypoint = path[i];
									// resultWaypoint.y += this->playerController->getOwner()->getPosition().y /** 2.0f*/;

									// First wp is useless at it is at the same position as the player
									if (i > 0)
									{
										this->movingBehavior->getPath()->addWayPoint(resultWaypoint);

										if (true == this->playerController->getDrawPath())
										{
											if (nullptr == this->playerController->debugWaypointNode)
											{
												this->playerController->debugWaypointNode = this->playerController->getOwner()->getSceneManager()->getRootSceneNode()->createChildSceneNode();
												Ogre::v1::Entity* entity = this->playerController->getOwner()->getSceneManager()->createEntity("Node.mesh");
												this->playerController->debugWaypointNode->attachObject(entity);
											}
											
											this->playerController->debugWaypointNode->setPosition(resultWaypoint);
										}
									}
								}

								this->playerController->setMoveWeight(1.0f);
								this->playerController->setJumpWeight(1.0f);

								if ((false == this->playerController->getAnimationBlender()->isComplete() && this->movingBehavior->getPath()->getRemainingWaypoints() > 0)
									&& false == this->playerController->getAnimationBlender()->isAnimationActive(AnimationBlender::ANIM_WALK_NORTH))
								{
									tempAnimationSpeed = this->playerController->getAnimationSpeed();
									// 0.02f: Immediately blend to walk
									// this->playerController->getAnimationBlender()->blend(AnimationBlender::ANIM_WALK_NORTH, NOWA::AnimationBlender::BlendSwitch, 0.2f, true);
									this->playerController->getAnimationBlender()->blend(AnimationBlender::ANIM_WALK_NORTH, NOWA::AnimationBlender::BlendWhileAnimating, 0.2f, true);
								}
							}
						}
					}
					else
					{
						if (nullptr != this->movingBehavior->getPath())
						{
							this->movingBehavior->getPath()->clear();
						}
				    }
				}
			}
		}
		else
		{
			this->canClick = true;
		}
		
		if (this->movingBehavior->getPath()->getRemainingWaypoints() == 0)
		{
			if (true == this->hasGoal)
			{
				// Start random idle immediately
				this->boringTimer = 4.9f;
				this->hasGoal = false;
			}

			this->boringTimer += dt;
			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PlayerControllerJumpNRunComponent] Timer: " + Ogre::StringConverter::toString(boringTimer));

			// if the user does nothing for 5 seconds, choose a random boring animation state
			if (this->boringTimer >= 5.0f)
			{
				int id = MathHelper::getInstance()->getRandomNumber<int>(0, 3);
				AnimationBlender::AnimID animID;
				if (0 == id)
				{
					animID = AnimationBlender::ANIM_IDLE_1;
				}
				else if (1 == id)
				{
					animID = AnimationBlender::ANIM_IDLE_2;
				}
				else
				{
					animID = AnimationBlender::ANIM_IDLE_3;
				}
				if (this->playerController->getAnimationBlender()->hasAnimation(animID))
				{
					if (false == this->playerController->getAnimationBlender()->isAnimationActive(animID) || this->playerController->getAnimationBlender()->isComplete())
					{
						this->playerController->getAnimationBlender()->blend(animID, AnimationBlender::BlendWhileAnimating, 0.5f, true);
						this->boringTimer = 0.0f;
					}
				}
				
			}
		}

		/*Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PlayerControllerJumpNRunComponent] Anim: " + this->playerController->getAnimationBlender()->getSource()->getAnimationName()
			+ " Progress: " + Ogre::StringConverter::toString(this->playerController->getAnimationBlender()->getProgress())
			+ " TimePosition: " + Ogre::StringConverter::toString(this->playerController->getAnimationBlender()->getTimePosition()) 
			+ " Length: " + Ogre::StringConverter::toString(this->playerController->getAnimationBlender()->getLength())
			+ " Complete: " + Ogre::StringConverter::toString(this->playerController->getAnimationBlender()->isComplete()));*/

		
		this->playerController->getAnimationBlender()->addTime(dt * tempAnimationSpeed / this->playerController->getAnimationBlender()->getLength());
	}

	void PathFollowState3D::exit(GameObject* player)
	{
		this->playerController->getAnimationBlender()->init(NOWA::AnimationBlender::ANIM_IDLE_1);
		this->playerController->getAnimationBlender()->blend(NOWA::AnimationBlender::ANIM_IDLE_1, NOWA::AnimationBlender::BlendWhileAnimating, 0.5f, true);
		this->boringTimer = 6.0f;
		this->movingBehavior->removeBehavior(NOWA::KI::MovingBehavior::FOLLOW_PATH);
	}

}; // namespace end
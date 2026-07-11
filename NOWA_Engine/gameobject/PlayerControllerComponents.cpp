#include "NOWAPrecompiled.h"
#include "PlayerControllerComponents.h"
#include "GameObjectController.h"
#include "PhysicsActiveComponent.h"
#include "PhysicsRagDollComponentV2.h"
#include "PhysicsPlayerControllerComponent.h"
#include "PhysicsActiveKinematicComponent.h"
#include "NodeComponent.h"
#include "LuaScriptComponent.h"
#include "main/Core.h"
#include "modules/InputDeviceModule.h"
#include "InputDeviceComponent.h"
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
#include "AiComponents.h"

#include "utilities/AnimationBlenderV2.h"

#include "Animation/OgreSkeletonInstance.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	PlayerControllerComponent::AnimationBlenderObserver::AnimationBlenderObserver(luabind::object closureFunction, bool oneTime)
		: AnimationBlenderV2::IAnimationBlenderObserver(),
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
			NOWA::AppStateManager::LogicCommand logicCommand = [this]()
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

						Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[PlayerControllerComponent::AnimationBlenderObserver] Caught error in 'reactOnAnimationFinished' Error: " + Ogre::String(error.what())
							+ " details: " + msg.str());
					}
				};
			NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
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
		useStandUp(new Variant(PlayerControllerComponent::AttrUseStandUp(), false, this->attributes)),
		physicsActiveComponent(nullptr),
		cameraBehaviorComponent(nullptr),
		inputDeviceComponent(nullptr),
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
		timeFallen(0.0f),
		isFallen(false),
		fallThreshold(0.7f),
		recoveryTime(2.0f),
		debugWaypointNode(nullptr)
	{
		this->acceleration->setDescription("The acceleration rate, if set to 0, acceleration is disabled and player moves with full speed.");
		this->useStandUp->setDescription("Sets whether to use stand up feature for a player, so that if he fell down, after 2 seconds, he will stand up again.");
	}

	PlayerControllerComponent::~PlayerControllerComponent()
	{
		AppStateManager::getSingletonPtr()->getGameObjectController()->removePlayerController(this->gameObjectPtr->getId());
	
		if (nullptr != this->animationBlender)
		{
			this->animationBlender->deleteAllObservers();
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

			NOWA::GraphicsModule::getInstance()->removeTrackedNode(this->debugWaypointNode);
			this->gameObjectPtr->getSceneManager()->destroySceneNode(this->debugWaypointNode);
			this->debugWaypointNode = nullptr;
		}
	}

	void PlayerControllerComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		this->deleteDebugData();
	}

	void PlayerControllerComponent::onOtherComponentRemoved(unsigned int index)
	{
		auto gameObjectCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponentByIndex(index));
		if (nullptr != gameObjectCompPtr)
		{
			auto inputDeviceCompPtr = boost::dynamic_pointer_cast<InputDeviceComponent>(gameObjectCompPtr);
			if (nullptr != inputDeviceCompPtr)
			{
				this->inputDeviceComponent = nullptr;
			}
		}
	}

	bool PlayerControllerComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

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
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseStandUp")
		{
			this->useStandUp->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}

		return true;
	}

	bool PlayerControllerComponent::postInit(void)
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

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		return true;
	}

	bool PlayerControllerComponent::connect(void)
	{
		GameObjectComponent::connect();

		this->setActivated(this->activated->getBool());

		return true;
	}

	bool PlayerControllerComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();

		if (nullptr != this->animationBlender)
		{
			this->animationBlender->init("", false);
		}

		AppStateManager::getSingletonPtr()->getGameObjectController()->removePlayerController(this->gameObjectPtr->getId());
		this->cameraBehaviorComponent = nullptr;
		this->physicsActiveComponent = nullptr;
		// Will cause crash in state, input device component shall be existing!
		// this->inputDeviceComponent = nullptr;
		this->moveLockOwner.clear();
		this->jumpWeightOwner.clear();

		this->height = 0.0f;
		this->normal = Ogre::Vector3::ZERO;
		this->priorValidHeight = 0.0f;
		this->priorValidNormal = Ogre::Vector3::ZERO;
		this->timeFallen = 0.0f;

		AppStateManager::getSingletonPtr()->getOgreRecastModule()->removeDrawnPath();

		this->internalShowDebugData();
		return true;
	}

	void PlayerControllerComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating && nullptr != this->physicsActiveComponent/* && true == this->activated->getBool()*/)
		{
			// auto widget = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Window>("manipulationWindow", false);

			this->setMoveWeight(1.0f);
			this->setJumpWeight(1.0f);

			Ogre::Vector3 playerSize = this->gameObjectPtr->getSize();
			Ogre::Vector3 bottomOffset = this->gameObjectPtr->getBottomOffset();
			Ogre::Vector3 centerOffset = this->gameObjectPtr->getCenterOffset();
			Ogre::Vector3 middleOfPlayer = this->gameObjectPtr->getMiddle();

			// Note on ogrenewt raycast, the startpoint is always the absolute game object position.
			// The root is in the middle of the game object, so we need to move the ray start down to the FEET.
			const Ogre::Real halfHeight = bottomOffset.y;							// e.g. ~1.0 for a 2m player
			Ogre::Vector3 centerBottom = Ogre::Vector3(0.0f, -halfHeight, 0.0f);	// local feet position
			const Ogre::Real fraction = 0.4f;

			bool showDebugData = false;

			// 1. Check objects that are below the player
			// Use feetY + small epsilon, so the ray starts slightly above the feet
			PhysicsActiveComponent::ContactData contactsDataBelow1 = this->physicsActiveComponent->getContactBelow(0,
				Ogre::Vector3(-playerSize.z * fraction, centerBottom.y + 0.2f, 0.0f), showDebugData, this->categoriesId, true); // y is near feet

			PhysicsActiveComponent::ContactData contactsDataBelow2 = this->physicsActiveComponent->getContactBelow(1,
				Ogre::Vector3(0.0f, centerBottom.y + 0.2f, playerSize.z * fraction), showDebugData, this->categoriesId, true);  // y is near feet

			PhysicsActiveComponent::ContactData contactsDataBelow3 = this->physicsActiveComponent->getContactBelow(2,
				Ogre::Vector3(playerSize.z * fraction, centerBottom.y + 0.2f, 0.0f), showDebugData, this->categoriesId, true);  // y is near feet

			/*PhysicsActiveComponent::ContactData contactsDataBelowLine = this->physicsActiveComponent->getContactToDirection(0, Ogre::Vector3::UNIT_X,
				Ogre::Vector3(0.0f, centerBottom.y + 0.1f, 0.0f), -playerSize.z * fraction, playerSize.z * fraction, showDebugData, this->categoriesId);*/

			this->hitGameObjectBelow = contactsDataBelow1.getHitGameObject();
			if (nullptr == this->hitGameObjectBelow)
			{
				this->hitGameObjectBelow = contactsDataBelow2.getHitGameObject();
			}
			if (nullptr == this->hitGameObjectBelow)
			{
				this->hitGameObjectBelow = contactsDataBelow3.getHitGameObject();
			}
			/*if (nullptr == this->hitGameObjectBelow)
			{
				this->hitGameObjectBelow = contactsDataBelowLine.getHitGameObject();
			}*/
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

			if (this->height >= 500.0f)
			{
				this->height = 0.0f;
			}

			/*if (widget)
				widget->setCaption("Height: " + Ogre::StringConverter::toString(this->height));*/

				// if (widget)
				// 	widget->setCaption("Normal: " + Ogre::StringConverter::toString(this->normal));

				// 2. Check all objects that are in front of the player

				// never change -0.2, because else the rope does not exist anymore and the player gets stuck on a wall

			Ogre::Vector3 direction = this->physicsActiveComponent->getOrientation() * this->gameObjectPtr->getDefaultDirection();

			PhysicsActiveComponent::ContactData contactDataFront[3];

#if 1
			contactDataFront[0] = this->physicsActiveComponent->getContactToDirection(1, direction,
				Ogre::Vector3(0.0f, centerBottom.y + playerSize.y, playerSize.z - 0.2f), 0.0f, 0.1f, showDebugData, this->categoriesId);

			contactDataFront[1] = this->physicsActiveComponent->getContactToDirection(2, direction,
				Ogre::Vector3(0.0f, 0.2f, playerSize.z - 0.2f), 0.0f, 0.1f, showDebugData, this->categoriesId);

			contactDataFront[2] = this->physicsActiveComponent->getContactToDirection(3, direction,
				Ogre::Vector3(0.0f, centerBottom.y + (playerSize.y * 0.5f), playerSize.z - 0.2f), 0.0f, 0.1f, showDebugData, this->categoriesId);
#else
			contactDataFront[0] = this->physicsActiveComponent->getContactAhead(1,
				Ogre::Vector3(0.0f, centerBottom.y + playerSize.y, playerSize.z - 0.2f), 0.1f, showDebugData, this->categoriesId);

			contactDataFront[1] = this->physicsActiveComponent->getContactAhead(2,
				Ogre::Vector3(0.0f, 0.2f, playerSize.z - 0.2f), 0.1f, showDebugData, this->categoriesId);

			contactDataFront[2] = this->physicsActiveComponent->getContactAhead(3,
				Ogre::Vector3(0.0f, centerBottom.y + (playerSize.y * 0.5f), playerSize.z - 0.2f), 0.1f, showDebugData, this->categoriesId);
#endif

			//// Build:	T |
			////		A | -> Line to check front
			//contactDataFront[3] = this->physicsActiveComponent->getContactToDirection(4, Ogre::Vector3::UNIT_Y, 
			//	Ogre::Vector3(0.0f, centerBottom.y + 0.1f, playerSize.z * 0.5f), -0.05f, playerSize.y + 0.05f, showDebugData, this->categoriesId); // 0.2 is to much, and player would stuck in certain ground

			this->hitGameObjectFront = nullptr;

			if (nullptr != contactDataFront[0].getHitGameObject())
			{
				this->hitGameObjectFront = contactDataFront[0].getHitGameObject();
			}
			else if (nullptr != contactDataFront[1].getHitGameObject())
			{
				this->hitGameObjectFront = contactDataFront[1].getHitGameObject();
			}
			else if (nullptr != contactDataFront[2].getHitGameObject())
			{
				this->hitGameObjectFront = contactDataFront[2].getHitGameObject();
			}
			/*if (nullptr != contactDataFront[3].getHitGameObject())
			{
				this->hitGameObjectFront = contactDataFront[3].getHitGameObject();
			}*/

			this->hitGameObjectUp = nullptr;
			this->hitGameObjectUp = this->physicsActiveComponent->getContactAbove(5, Ogre::Vector3(0.0f, playerSize.y + 0.1f, 0.0f), showDebugData, this->categoriesId, true).getHitGameObject();

			if (true == this->useStandUp->getBool())
			{
				// 90° * threshold (0.7 = 63°)
				Ogre::Real fallThresholdAngle = Ogre::Degree(90.0f * 0.7f).valueDegrees();

				// Gravity up direction (should always be stable)
				Ogre::Vector3 gravityUp = -this->physicsActiveComponent->getGravityDirection();

				// Player's current up vector based on orientation
				Ogre::Vector3 currentPlayerUp = this->physicsActiveComponent->getOrientation() * Ogre::Vector3::UNIT_Y;

				// Compute angle deviation between player's up and gravity up
				Ogre::Real angleDeviation = Ogre::Math::ACos(currentPlayerUp.dotProduct(gravityUp)).valueDegrees();

				// Player is tilted significantly
				if (angleDeviation > fallThresholdAngle)
				{
					if (false == this->isFallen) // Start counting time
					{
						this->timeFallen = 0.0f;
						this->isFallen = true;
					}
					else
					{
						timeFallen += dt; // Accumulate time
						if (timeFallen >= recoveryTime) // If fallen for 2 seconds
						{
							this->standUp();
							this->isFallen = false; // Reset state
						}
					}
				}
				else
				{
					this->isFallen = false; // Reset if player recovers naturally
					this->timeFallen = 0.0f;
				}
			}
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
		else if (PlayerControllerComponent::AttrUseStandUp() == attribute->getName())
		{
			this->setUseStandUp(attribute->getBool());
		}
	}

	void PlayerControllerComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UseStandUp"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useStandUp->getBool())));
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

	GameObjectCompPtr PlayerControllerComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		PlayerControllerCompPtr clonedCompPtr(boost::make_shared<PlayerControllerComponent>());

		// Do not clone activated, since its no visible and switched manually in game object controller
		// clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setRotationSpeed(this->rotationSpeed->getReal());
		clonedCompPtr->setGoalRadius(this->goalRadius->getReal());
		clonedCompPtr->setAnimationSpeed(this->animationSpeed->getReal());
		clonedCompPtr->setAcceleration(this->acceleration->getReal());
		clonedCompPtr->setCategories(this->categories->getString());
		clonedCompPtr->setUseStandUp(this->useStandUp->getBool());

		for (unsigned int i = 0; i < static_cast<unsigned int>(this->animations.size()); i++)
		{
			clonedCompPtr->setAnimationName(this->animations[i]->getListSelectedValue(), i);
		}

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	void PlayerControllerComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);

		// Add the player controller to the player controller component map, but as weak ptr, because game object controller should not hold the life cycle of this components, because the game objects already do, which are
		// also hold shared by the game object controller
		AppStateManager::getSingletonPtr()->getGameObjectController()->addPlayerController(boost::dynamic_pointer_cast<PlayerControllerComponent>(shared_from_this()));

		// Must be done here, because in post init, it may be, that a component does not yet exist, if its added after this component!
		auto physicsPlayerControllerCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsPlayerControllerComponent>());
		if (nullptr == physicsPlayerControllerCompPtr)
		{
			auto physicsActiveCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsActiveComponent>());
			if (nullptr == physicsActiveCompPtr)
			{
				auto physicsActiveKinematicCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsActiveKinematicComponent>());
				if (nullptr == physicsActiveKinematicCompPtr)
				{
					return;
				}
				else
				{
					this->physicsActiveComponent = dynamic_cast<PhysicsActiveKinematicComponent*>(physicsActiveKinematicCompPtr.get());
				}
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
		auto cameraBehaviorCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<CameraBehaviorComponent>());
		if (nullptr != cameraBehaviorCompPtr)
		{
			this->cameraBehaviorComponent = cameraBehaviorCompPtr.get();
		}
		else
		{
			this->cameraBehaviorComponent = nullptr;
		}

		if (true == activated)
		{
			// In post init not all game objects are known, and so there are maybe no categories yet, so set the categories here
			this->setCategories(this->categories->getString());

			this->internalShowDebugData();

			auto inputDeviceCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<InputDeviceComponent>());
			if (nullptr != inputDeviceCompPtr)
			{
				this->inputDeviceComponent = inputDeviceCompPtr.get();

				if (false == this->inputDeviceComponent->hasValidDevice())
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PlayerControllerComponent] Cannot use any state, because the InputDeviceComponent has not valid input device set.");
				}
			}
            else
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                    "[PlayerControllerComponent] Player controller will not work, because an input device component is missing for this game object: " + this->gameObjectPtr->getName());
            }
		}

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
			// this->animationBlender->setDebugLog(this->bShowDebugData);
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

	PhysicsRagDollComponentV2* PlayerControllerComponent::getPhysicsRagDollComponent(void) const
	{
		return dynamic_cast<PhysicsRagDollComponentV2*>(this->physicsActiveComponent);
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
		{
			animationSpeed = 1.0f;
		}
		this->animationSpeed->setValue(animationSpeed);
	}

	Ogre::Real PlayerControllerComponent::getAnimationSpeed(void) const
	{
		return this->animationSpeed->getReal();
	}

	void PlayerControllerComponent::setAcceleration(Ogre::Real acceleration)
	{
		if (acceleration < 0.0f)
		{
			acceleration = 0.0f;
		}
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

	void PlayerControllerComponent::setUseStandUp(bool useStandUp)
	{
		this->useStandUp->setValue(useStandUp);
	}

	bool PlayerControllerComponent::getUseStandUp(void) const
	{
		return this->useStandUp->getBool();
	}

	void PlayerControllerComponent::setAnimationName(const Ogre::String& name, unsigned int index)
	{
		if (index >= this->animations.size())
		{
			return;
		}
		this->animations[index]->setListSelectedValue(name);
	}

	Ogre::String PlayerControllerComponent::getAnimationName(unsigned int index)
	{
		if (index >= this->animations.size())
		{
			return "";
		}
		return this->animations[index]->getListSelectedValue();
	}

	CameraBehaviorComponent* PlayerControllerComponent::getCameraBehaviorComponent(void) const
	{
		return this->cameraBehaviorComponent;
	}

	InputDeviceComponent* PlayerControllerComponent::getInputDeviceComponent(void) const
	{
		return this->inputDeviceComponent;
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

	bool PlayerControllerComponent::getIsFallen(void) const
	{
		return this->isFallen;
	}

	void PlayerControllerComponent::standUp(void)
	{
#if 1
		// Hacks the physics, to let the player stand up
		Ogre::Quaternion uprightRotation = Ogre::Vector3::UNIT_Y.getRotationTo(-this->physicsActiveComponent->getGravityDirection());
		this->physicsActiveComponent->setOrientation(uprightRotation);
#else
		// Hacks the physics, to let the player stand up
		Ogre::Quaternion currentOrientation = this->physicsActiveComponent->getOrientation();
		Ogre::Quaternion targetOrientation = Ogre::Vector3::UNIT_Y.getRotationTo(-this->physicsActiveComponent->getGravityDirection());

		// Interpolate with Slerp (0.1 is the interpolation factor, adjust as needed)
		Ogre::Quaternion smoothOrientation = Ogre::Quaternion::Slerp(0.1f, currentOrientation, targetOrientation, true);

		this->physicsActiveComponent->setOrientation(smoothOrientation);
#endif
	}

	void PlayerControllerComponent::reactOnAnimationFinished(luabind::object closureFunction, bool oneTime)
	{
		if (nullptr == this->animationBlender)
		{
			return;
		}

		AnimationBlenderObserver* newObserver = new AnimationBlenderObserver(closureFunction, oneTime);
		this->animationBlender->addAnimationBlenderObserver(newObserver);
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
	}

	bool PlayerControllerJumpNRunComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		bool success = PlayerControllerComponent::init(propertyElement);

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
		clonedCompPtr->setUseStandUp(this->useStandUp->getBool());

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

		Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
		if (nullptr != item)
		{
			std::vector<Ogre::String> animationNames;
			// Add also none, so that when choosen, no animation will be done, because it does not exist
			animationNames.emplace_back("None");

			Ogre::SkeletonInstance* skeleton = item->getSkeletonInstance();
            if (nullptr != skeleton)
            {
                for (auto& anim : skeleton->getAnimationsNonConst())
                {
                    animationNames.emplace_back(anim.getName().getFriendlyText());
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

		PhysicsPlayerControllerComponent* physicsPlayerControllerComponent = dynamic_cast<PhysicsPlayerControllerComponent*>(this->physicsActiveComponent);
		if (nullptr != physicsPlayerControllerComponent)
		{
			// Deactivates the movement, because internally newtons player body update would run and let the player fall, even he is not active yet via the walking state on a planet
			physicsPlayerControllerComponent->setActivated(false);
		}

		return success;
	}

	bool PlayerControllerJumpNRunComponent::disconnect(void)
	{
		bool success = PlayerControllerComponent::disconnect();

		if (nullptr != this->animationBlender)
		{
			// this->animationBlender->clearAnimations();
			this->animationBlender->init(NOWA::AnimationBlenderV2::ANIM_IDLE_1);
			// Reset animation to T-Pose
			this->animationBlender->setSourceEnabled(false);
		}

		if (nullptr != this->stateMachine->getCurrentState())
		{
			this->stateMachine->getCurrentState()->exit(this->gameObjectPtr.get());
		}

		PhysicsPlayerControllerComponent* physicsPlayerControllerComponent = dynamic_cast<PhysicsPlayerControllerComponent*>(this->physicsActiveComponent);
		if (nullptr != physicsPlayerControllerComponent)
		{
			// Deactivates the movement, because internally newtons player body update would run and let the player fall, even he is not active yet via the walking state on a planet
			physicsPlayerControllerComponent->setActivated(false);
		}

		return success;
	}

	void PlayerControllerJumpNRunComponent::update(Ogre::Real dt, bool notSimulating)
	{
		PlayerControllerComponent::update(dt, notSimulating);
		if (false == notSimulating/* && true == this->activated->getBool()*/)
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

	void PlayerControllerJumpNRunComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		PlayerControllerComponent::writeXML(propertiesXML, doc);

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

	void PlayerControllerJumpNRunComponent::setActivated(bool activated)
	{
		PlayerControllerComponent::setActivated(activated);

		if (true == activated)
		{
			this->animationBlender->registerAnimation(NOWA::AnimationBlenderV2::ANIM_IDLE_1, this->animations[0]->getListSelectedValue());
            this->animationBlender->registerAnimation(NOWA::AnimationBlenderV2::ANIM_IDLE_2, this->animations[1]->getListSelectedValue());
            this->animationBlender->registerAnimation(NOWA::AnimationBlenderV2::ANIM_IDLE_3, this->animations[2]->getListSelectedValue());
            this->animationBlender->registerAnimation(NOWA::AnimationBlenderV2::ANIM_WALK_NORTH, this->animations[3]->getListSelectedValue());
            this->animationBlender->registerAnimation(NOWA::AnimationBlenderV2::ANIM_WALK_SOUTH, this->animations[4]->getListSelectedValue());
            this->animationBlender->registerAnimation(NOWA::AnimationBlenderV2::ANIM_WALK_WEST, this->animations[5]->getListSelectedValue());
            this->animationBlender->registerAnimation(NOWA::AnimationBlenderV2::ANIM_WALK_EAST, this->animations[6]->getListSelectedValue());
            this->animationBlender->registerAnimation(NOWA::AnimationBlenderV2::ANIM_JUMP_START, this->animations[7]->getListSelectedValue());
            this->animationBlender->registerAnimation(NOWA::AnimationBlenderV2::ANIM_JUMP_WALK, this->animations[8]->getListSelectedValue());
            this->animationBlender->registerAnimation(NOWA::AnimationBlenderV2::ANIM_HIGH_JUMP_END, this->animations[9]->getListSelectedValue());
            this->animationBlender->registerAnimation(NOWA::AnimationBlenderV2::ANIM_JUMP_END, this->animations[10]->getListSelectedValue());
            this->animationBlender->registerAnimation(NOWA::AnimationBlenderV2::ANIM_RUN, this->animations[11]->getListSelectedValue());
            this->animationBlender->registerAnimation(NOWA::AnimationBlenderV2::ANIM_SNEAK, this->animations[12]->getListSelectedValue());
            this->animationBlender->registerAnimation(NOWA::AnimationBlenderV2::ANIM_DUCK, this->animations[13]->getListSelectedValue());

			this->animationBlender->init(NOWA::AnimationBlenderV2::ANIM_IDLE_1);

			this->stateMachine->setCurrentState(WalkingStateJumpNRun::getName());
		}
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

	bool PlayerControllerJumpNRunLuaComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		bool success = PlayerControllerComponent::init(propertyElement);

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
		clonedCompPtr->setUseStandUp(this->useStandUp->getBool());

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
		if (false == notSimulating/* && true == this->activated->getBool()*/)
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

	void PlayerControllerJumpNRunLuaComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		PlayerControllerComponent::writeXML(propertiesXML, doc);

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

	void PlayerControllerJumpNRunLuaComponent::setActivated(bool activated)
	{
		PlayerControllerComponent::setActivated(activated);
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

	bool PlayerControllerClickToPointComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		bool success = PlayerControllerComponent::init(propertyElement);

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
		clonedCompPtr->setUseStandUp(this->useStandUp->getBool());
		clonedCompPtr->setRange(this->range->getReal());

		for (unsigned int i = 0; i < static_cast<unsigned int>(this->animations.size()); i++)
		{
			clonedCompPtr->setAnimationName(this->animations[i]->getListSelectedValue(), i);
		}

		clonedCompPtr->setAutoClick(this->autoClick->getBool());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->setPathSlot(this->pathSlot->getInt());

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool PlayerControllerClickToPointComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlayerControllerClickToPointComponent] Init player controller click to point component for game object: " + this->gameObjectPtr->getName());

		bool success = PlayerControllerComponent::postInit();

		this->categoriesId = AppStateManager::getSingletonPtr()->getGameObjectController()->generateCategoryId(this->categories->getString());

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

		this->stateMachine = new NOWA::KI::StateMachine<GameObject>(this->gameObjectPtr.get());
		this->stateMachine->registerState<PathFollowState3D>(PathFollowState3D::getName());

		// Moving behavior is added and create in game object controller, because even this component, which is a player controller, could also have some other ai components
		// so the moving behavior is shared amongst all components
		this->movingBehaviorPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->addMovingBehavior(this->gameObjectPtr->getId());

		if (nullptr == AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreRecast())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage("PlayerControllerClickToPointComponent: Cannot use click to point controller because, there is no valid OgreRecast path navigation configured. Is it not checked in the settings?");
			return true;
		}

		return success;
	}

	bool PlayerControllerClickToPointComponent::connect(void)
	{
		GameObjectComponent::connect(); // Lua script is requested
		bool success = PlayerControllerComponent::connect();

		if (nullptr == this->movingBehaviorPtr)
		{
			this->postInit();
		}

		if (nullptr != this->animationBlender)
		{
			this->animationBlender->registerAnimation(NOWA::AnimationBlenderV2::ANIM_IDLE_1, this->animations[0]->getListSelectedValue());
			this->animationBlender->registerAnimation(NOWA::AnimationBlenderV2::ANIM_IDLE_2, this->animations[1]->getListSelectedValue());
			this->animationBlender->registerAnimation(NOWA::AnimationBlenderV2::ANIM_IDLE_3, this->animations[2]->getListSelectedValue());
			this->animationBlender->registerAnimation(NOWA::AnimationBlenderV2::ANIM_WALK_NORTH, this->animations[3]->getListSelectedValue());
			this->animationBlender->registerAnimation(NOWA::AnimationBlenderV2::ANIM_RUN, this->animations[4]->getListSelectedValue());

			this->animationBlender->init(NOWA::AnimationBlenderV2::ANIM_IDLE_1);
		}

		if (nullptr != this->movingBehaviorPtr)
		{
			this->movingBehaviorPtr->addBehavior(NOWA::KI::MovingBehavior::FOLLOW_PATH);
			this->movingBehaviorPtr->setRotationSpeed(this->rotationSpeed->getReal());
			this->movingBehaviorPtr->setGoalRadius(this->goalRadius->getReal());
			// this->movingBehaviorPtr->setGoalRadius(0.5f);
			this->movingBehaviorPtr->setStuckCheckTime(5000.0f);
			this->movingBehaviorPtr->setFlyMode(false);
		}

		auto* ogreRecast = AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreRecast();
        if (nullptr != ogreRecast)
        {
            ogreRecast->createNavQueryForSlot(this->pathSlot->getInt()); // creates slot 0 if missing
        }

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

		if (nullptr != this->stateMachine && nullptr != this->stateMachine->getCurrentState())
		{
			this->stateMachine->getCurrentState()->exit(this->gameObjectPtr.get());
		}

		this->animationBlender->clearAnimations();

		if (nullptr != AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreRecast())
        {
            AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreRecast()->destroyNavQueryForSlot(this->pathSlot->getInt());
        }

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
		if (false == notSimulating /*&& true == this->activated->getBool()*/)
		{
			this->stateMachine->update(dt);
		}
		/*if (true == this->bShowDebugData)
		{
			if (nullptr != this->movingBehaviorPtr)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MovingBehavior] Current behavior: " + this->movingBehaviorPtr->getCurrentBehavior());
			}
		}*/
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

	void PlayerControllerClickToPointComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		PlayerControllerComponent::writeXML(propertiesXML, doc);

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

	void PlayerControllerClickToPointComponent::setActivated(bool activated)
	{
		PlayerControllerComponent::setActivated(activated);
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
        if (nullptr == this->gameObjectPtr)
        {
            return;
        }

        auto* ogreRecast = AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreRecast();

        if (nullptr != ogreRecast)
        {
            // Designer explicitly set a non-default slot in the editor — respect it.
            // Default slot 0 on a freshly cloned worker means "not yet assigned".
            if (this->pathSlot->getInt() == 0 && !ogreRecast->hasNavQueryForSlot(0))
            {
                // Slot 0 genuinely free — take it
                ogreRecast->createNavQueryForSlot(0);
            }
            else if (this->pathSlot->getInt() == 0)
            {
                // Slot 0 already taken by another worker — get a free one
                // So sharing = either :
                //    Corruption /
                //    crashes if pathfinding runs on multiple threads A serialization bottleneck if you add a mutex — defeating the whole point of having separate queries
                // So its bad idea to share slots! Each GameObject must have a different slot!
                int assigned = ogreRecast->acquireNextFreeSlot();
                this->pathSlot->setValue(assigned);

                // Sync sibling component silently (no destroy/create, slot is fresh)
                auto aiComp = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<AiRecastPathNavigationComponent>());
                if (nullptr != aiComp)
                {
                    aiComp->setPathSlot(assigned);
                }
            }
            else
            {
                // Designer-assigned slot — just ensure it exists
                ogreRecast->createNavQueryForSlot(this->pathSlot->getInt());
            }
        }
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
		acceleration(0.5f),
		hasInputDevice(true),
		sceneManager(nullptr),
		walkSound(nullptr),
		jumpSound(nullptr)
	{
		
	}

	WalkingStateJumpNRun::~WalkingStateJumpNRun()
	{
		if (nullptr != this->walkSound)
		{
			OgreALModule::getInstance()->deleteSound(this->sceneManager, this->walkSound);
			this->walkSound = nullptr;
		}
		if (nullptr != this->jumpSound)
		{
			OgreALModule::getInstance()->deleteSound(this->sceneManager, this->jumpSound);
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

		this->playerController->getAnimationBlender()->blend(NOWA::AnimationBlenderV2::ANIM_IDLE_1, NOWA::AnimationBlenderV2::BlendThenAnimate, 0.2f, true);
		this->boringTimer = 0.0f;
		this->noMoveTimer = 0.0f;
		// acquire the jump force from attributes
		// this->jumpForce = this->playerController->getAttributesComponent()->getAttribute("AttributeJumpForce")->getValueReal();
		this->jumpForce = this->playerController->getJumpForce();

		PhysicsPlayerControllerComponent* physicsPlayerControllerComponent = dynamic_cast<PhysicsPlayerControllerComponent*>(this->playerController->getPhysicsComponent());
		if (nullptr != physicsPlayerControllerComponent)
		{
			this->hasPhysicsPlayerControllerComponent = true;
			// Just here activate the player for potentional planetary movement
			physicsPlayerControllerComponent->setActivated(true);
		}

		this->sceneManager = this->playerController->getOwner()->getSceneManager();
	}

	void WalkingStateJumpNRun::update(GameObject* player, Ogre::Real dt)
    {
        this->hasInputDevice = true;

        if (nullptr == this->playerController->getInputDeviceComponent() || true == this->playerController->getInputDeviceComponent()->isDeviceLocked())
        {
            this->hasInputDevice = false;
        }

        // -------------------------------------------------------------------------
        // Early returns BEFORE beginFrame -- if we skip beginFrame we must also
        // skip addTime, which is fine (animation stays frozen during these states).
        // -------------------------------------------------------------------------
        if (false == this->hasInputDevice)
        {
            return;
        }

        if (true == this->playerController->getIsFallen())
        {
            return;
        }

        InputDeviceModule* inputDeviceModule = this->playerController->getInputDeviceComponent()->getInputDeviceModule();
        if (nullptr == inputDeviceModule)
        {
            return;
        }

        // beginFrame is now after all early returns -- it is always paired with addTime below.
        this->playerController->getAnimationBlender()->beginFrame();

        // -------------------------------------------------------------------------
        // Physics state -- gravity direction computed early because it is needed
        // for fall detection AND for jump / yaw logic later.
        // -------------------------------------------------------------------------
        Ogre::Vector3 gravityDir = this->playerController->getPhysicsComponent()->getGravityDirection();

        // Upward direction on any planet surface = opposite of gravity.
        Ogre::Vector3 upDir = -gravityDir;

        Ogre::Vector3 currentVelocity = this->playerController->getPhysicsComponent()->getVelocity();

        // Falling = velocity has a positive component ALONG gravity (toward planet).
        // This replaces all hardcoded "velocity.y < -1.0f" checks which only work
        // on flat Y-up worlds and break on spherical planet surfaces.
        const bool isFalling = currentVelocity.dotProduct(gravityDir) > 1.0f;

        Ogre::Real yawAtSpeed = 0.0f;
        Ogre::Real tempSpeed = 0.0f;
        Ogre::Real tempAnimationSpeed = this->playerController->getAnimationSpeed();
        Ogre::Radian heading = this->playerController->getOrientation().getYaw();

        // -------------------------------------------------------------------------
        // noMoveTimer -- countdown-based movement inhibit (e.g. after landing).
        // Fixed: old "else if (< 0.1f)" fired every frame when timer was 0.
        // New: simple countdown, restore weights only when timer expires.
        // -------------------------------------------------------------------------
        if (this->noMoveTimer > 0.0f)
        {
            this->playerController->setMoveWeight(0.0f);
            this->playerController->setJumpWeight(0.0f);
            this->noMoveTimer -= dt;
            if (this->noMoveTimer <= 0.0f)
            {
                this->noMoveTimer = 0.0f;
                this->playerController->setMoveWeight(1.0f);
                this->playerController->setJumpWeight(1.0f);
            }
        }

        // Stop translation at frame start (PhysicsPlayerController path).
        if (true == this->hasPhysicsPlayerControllerComponent)
        {
            static_cast<PhysicsPlayerControllerComponent*>(this->playerController->getPhysicsComponent())->move(0.0f, 0.0f, heading);
        }

        Ogre::Real height = this->playerController->getHeight();
        this->keyDirection = Ogre::Vector3::ZERO;

        this->inAir = height - this->playerController->getOwner()->getBottomOffset().y > 0.4f;

        // Store previous direction for 2D direction-change detection.
        this->oldDirection = this->direction;

        // -------------------------------------------------------------------------
        // Input handling
        // -------------------------------------------------------------------------
        const bool movingUp = inputDeviceModule->isActionDown(NOWA_A_UP);
        const bool movingDown = inputDeviceModule->isActionDown(NOWA_A_DOWN);
        const bool movingLeft = inputDeviceModule->isActionDown(NOWA_A_LEFT);
        const bool movingRight = inputDeviceModule->isActionDown(NOWA_A_RIGHT);
        const bool anyMove = movingUp || movingDown || movingLeft || movingRight;

        if (false == anyMove && false == this->jumpKeyPressed && false == this->isJumping)
        {
            // ------------------------------------------------------------------
            // IDLE
            // ------------------------------------------------------------------
            this->walkCount = 0.0f;
            this->keyDirection = Ogre::Vector3::ZERO;

            if (false == this->playerController->getAnimationBlender()->isAnimationActive(NOWA::AnimationBlenderV2::ANIM_IDLE_1) && false == this->inAir)
            {
                this->playerController->getAnimationBlender()->blend(NOWA::AnimationBlenderV2::ANIM_IDLE_1, NOWA::AnimationBlenderV2::BlendWhileAnimating, 0.2f, true);
            }

            tempAnimationSpeed = this->playerController->getAnimationSpeed() * 0.5f;
            this->direction = Direction::NONE;

            if (0.0f == this->playerController->getAcceleration())
            {
                this->acceleration = 1.0f;
            }
            else
            {
                this->acceleration -= this->playerController->getAcceleration() * dt;
                if (this->acceleration <= 0.5f)
                {
                    this->acceleration = 0.5f;
                }
            }

            this->boringTimer += dt;
            if (true == this->inAir)
            {
                this->boringTimer = 0.0f;
            }

            // Random boring animation after 10 seconds of no input.
            if (this->boringTimer >= 10.0f)
            {
                this->boringTimer = 0.0f;
                int id = MathHelper::getInstance()->getRandomNumber<int>(0, 3);
                NOWA::AnimationBlenderV2::AnimID animID;
                if (0 == id)
                {
                    animID = NOWA::AnimationBlenderV2::ANIM_IDLE_1;
                }
                else if (1 == id)
                {
                    animID = NOWA::AnimationBlenderV2::ANIM_IDLE_2;
                }
                else
                {
                    animID = NOWA::AnimationBlenderV2::ANIM_IDLE_3;
                }

                if (true == this->playerController->getAnimationBlender()->hasAnimation(animID) && true == this->playerController->getAnimationBlender()->isComplete() &&
                    false == this->playerController->getAnimationBlender()->isAnimationActive(animID))
                {
                    tempAnimationSpeed = this->playerController->getAnimationSpeed();
                    this->playerController->getAnimationBlender()->blend(animID, NOWA::AnimationBlenderV2::BlendWhileAnimating, 0.2f, true);
                }
            }
        }
        else if (true == anyMove)
        {
            // ------------------------------------------------------------------
            // MOVING
            // ------------------------------------------------------------------
            this->boringTimer = 0.0f;

            if (0.0f == this->playerController->getAcceleration())
            {
                this->acceleration = 1.0f;
            }
            else
            {
                this->acceleration += this->playerController->getAcceleration() * dt;
                if (this->acceleration >= 1.0f)
                {
                    this->acceleration = 1.0f;
                }
            }

            tempAnimationSpeed = this->playerController->getAnimationSpeed();

            NOWA::AnimationBlenderV2::AnimID animId = NOWA::AnimationBlenderV2::AnimID::ANIM_NONE;

            if (false == this->playerController->getIsFor2D())
            {
                // ---- 3D movement ------------------------------------------------
                if (true == movingUp)
                {
                    tempSpeed = this->playerController->getPhysicsComponent()->getSpeed() * this->playerController->getMoveWeight();
                    animId = NOWA::AnimationBlenderV2::ANIM_WALK_NORTH;
                    this->direction = Direction::UP;
                    this->keyDirection = this->playerController->getPhysicsComponent()->getForward();
                }
                else if (true == movingDown)
                {
                    tempAnimationSpeed = this->playerController->getAnimationSpeed() * 0.75f;
                    this->playerController->setMoveWeight(1.0f);
                    tempSpeed = -this->playerController->getPhysicsComponent()->getSpeed() * 0.5f * this->playerController->getMoveWeight();
                    animId = NOWA::AnimationBlenderV2::ANIM_WALK_SOUTH;
                    this->direction = Direction::DOWN;
                    this->keyDirection = this->playerController->getPhysicsComponent()->getForward();
                }

                if (true == movingLeft)
                {
                    animId = NOWA::AnimationBlenderV2::ANIM_WALK_WEST;
                    this->direction = Direction::LEFT;
                    yawAtSpeed = this->playerController->getRotationSpeed();
                    heading = heading + Ogre::Radian(this->playerController->getRotationSpeed()) * 0.1f;
                }
                else if (true == movingRight)
                {
                    animId = NOWA::AnimationBlenderV2::ANIM_WALK_EAST;
                    this->direction = Direction::RIGHT;
                    yawAtSpeed = -this->playerController->getRotationSpeed();
                    heading = heading - Ogre::Radian(this->playerController->getRotationSpeed()) * 0.1f;
                }

                // Run or sneak -- only when a forward direction is active.
                if (inputDeviceModule->isActionDown(NOWA_A_RUN))
                {
                    this->direction = Direction::UP;
                    tempSpeed = this->playerController->getPhysicsComponent()->getMaxSpeed() * this->playerController->getMoveWeight();
                    tempAnimationSpeed *= 0.8f;
                    animId = NOWA::AnimationBlenderV2::ANIM_RUN;
                }
                else if (inputDeviceModule->isActionDown(NOWA_A_SNEAK) && true == movingUp)
                {
                    this->direction = Direction::UP;
                    tempSpeed = this->playerController->getPhysicsComponent()->getMinSpeed() * this->playerController->getMoveWeight();
                    tempAnimationSpeed *= 0.5f;
                    animId = NOWA::AnimationBlenderV2::ANIM_SNEAK;
                }
            }
            else
            {
                // ---- 2D JumpNRun ------------------------------------------------
                if (true == this->directionChanged)
                {
                    // Direction just reversed -- movement weight could be zeroed here
                    // while the 90-degree turn completes.
                }

                if (true == movingLeft)
                {
                    if (this->playerController->getRunAfterWalkTime() > 0.0f)
                    {
                        this->walkCount += dt;
                    }
                    if (this->walkCount >= this->playerController->getRunAfterWalkTime())
                    {
                        animId = NOWA::AnimationBlenderV2::ANIM_RUN;
                        tempSpeed = this->playerController->getPhysicsComponent()->getMaxSpeed() * this->playerController->getMoveWeight();
                        tempAnimationSpeed *= 0.5f;
                    }
                    else
                    {
                        tempSpeed = this->playerController->getPhysicsComponent()->getSpeed() * this->playerController->getMoveWeight();
                        animId = NOWA::AnimationBlenderV2::ANIM_WALK_NORTH;
                    }
                    this->direction = Direction::LEFT;
                    this->keyDirection = Ogre::Vector3(-1.0f, 0.0f, 0.0f);
                }
                else if (true == movingRight)
                {
                    if (this->playerController->getRunAfterWalkTime() > 0.0f)
                    {
                        this->walkCount += dt;
                    }
                    if (this->walkCount >= this->playerController->getRunAfterWalkTime())
                    {
                        animId = NOWA::AnimationBlenderV2::ANIM_RUN;
                        tempSpeed = this->playerController->getPhysicsComponent()->getMaxSpeed() * this->playerController->getMoveWeight();
                        tempAnimationSpeed *= 0.5f;
                    }
                    else
                    {
                        tempSpeed = this->playerController->getPhysicsComponent()->getSpeed() * this->playerController->getMoveWeight();
                        animId = NOWA::AnimationBlenderV2::ANIM_WALK_NORTH;
                    }
                    this->direction = Direction::RIGHT;
                    this->keyDirection = Ogre::Vector3(1.0f, 0.0f, 0.0f);
                }

                if (this->oldDirection != this->direction)
                {
                    this->directionChanged = true;
                    this->walkCount = 0.0f;
                }

                if (true == this->directionChanged)
                {
                    this->boringTimer = 0.0f;

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
                    this->playerController->getPhysicsComponent()->applyOmegaForce(Ogre::Vector3(0.0f, yawAtSpeed, 0.0f));
                }
                else
                {
                    if (Direction::LEFT == this->direction)
                    {
                        this->playerController->getPhysicsComponent()->setOrientation(Ogre::Quaternion(Ogre::Degree(-90.0f), Ogre::Vector3::UNIT_Y));
                    }
                    else
                    {
                        this->playerController->getPhysicsComponent()->setOrientation(Ogre::Quaternion(Ogre::Degree(90.0f), Ogre::Vector3::UNIT_Y));
                    }
                }
            }

            // Blend to the movement animation -- only if not already active and
            // the player is on the ground and not in a jump transition.
            if (animId != NOWA::AnimationBlenderV2::AnimID::ANIM_NONE && false == this->playerController->getAnimationBlender()->isAnimationActive(animId) && false == this->inAir && false == this->jumpKeyPressed && false == this->isJumping &&
                (true == this->playerController->getAnimationBlender()->isComplete() || this->playerController->getAnimationBlender()->isAnimationActive(NOWA::AnimationBlenderV2::ANIM_IDLE_1) ||
                    this->playerController->getAnimationBlender()->isAnimationActive(NOWA::AnimationBlenderV2::ANIM_IDLE_2) || this->playerController->getAnimationBlender()->isAnimationActive(NOWA::AnimationBlenderV2::ANIM_IDLE_3)))
            {
                tempAnimationSpeed = this->playerController->getAnimationSpeed();
                this->playerController->getAnimationBlender()->blend(animId, NOWA::AnimationBlenderV2::BlendWhileAnimating, 0.2f, true);
            }
        }
        else if (inputDeviceModule->isActionDown(NOWA_A_DUCK))
        {
            // ------------------------------------------------------------------
            // DUCK
            // ------------------------------------------------------------------
            this->boringTimer = 0;

            if (false == this->playerController->getAnimationBlender()->isAnimationActive(NOWA::AnimationBlenderV2::ANIM_DUCK) && false == this->duckedOnce)
            {
                this->duckedOnce = true;
                this->playerController->getAnimationBlender()->blend(NOWA::AnimationBlenderV2::ANIM_DUCK, NOWA::AnimationBlenderV2::BlendWhileAnimating, 0.1f, false);

                // Scale collision hull down while ducking.
                this->playerController->getPhysicsComponent()->getBody()->scaleCollision(Ogre::Vector3(1.0f, 0.5f, 1.0f));
            }
            if (this->playerController->getAnimationBlender()->getTimePosition() >= this->playerController->getAnimationBlender()->getLength() - 0.3f)
            {
                this->playerController->getAnimationBlender()->setTimePosition(0.7f);
            }
        }
        else if (false == inputDeviceModule->isActionDown(NOWA_A_DUCK) && true == this->duckedOnce)
        {
            // ------------------------------------------------------------------
            // UNDUCK
            // ------------------------------------------------------------------
            this->boringTimer = 0;
            // Restore full collision hull when standing up.
            this->playerController->getPhysicsComponent()->getBody()->scaleCollision(Ogre::Vector3(1.0f, 1.0f, 1.0f));
            this->duckedOnce = false;
        }

        // -------------------------------------------------------------------------
        // Jump key -- single-press edge detection via tryJump flag.
        // tryJump is set true when a jump attempt failed (inAir / isJumping),
        // and cleared when the key is released. This prevents auto-repeat jumping.
        // -------------------------------------------------------------------------
        this->jumpKeyPressed = false;
        if (inputDeviceModule->isActionDown(NOWA_A_JUMP))
        {
            if (true == this->canDoubleJump)
            {
                this->jumpCount += 1;
                this->canDoubleJump = false;
                if (this->jumpCount > 2)
                {
                    this->jumpCount = 0;
                }
            }
            if (false == this->tryJump)
            {
                this->jumpKeyPressed = true;
            }
        }

        // -------------------------------------------------------------------------
        // Jump / fall / land animations
        // -------------------------------------------------------------------------

        // Jump start: player on ground, key just pressed, not already mid-jump.
        if (false == this->inAir && this->jumpKeyPressed && false == this->isJumping)
        {
            this->boringTimer = 0;

            if (false == this->playerController->getAnimationBlender()->isAnimationActive(NOWA::AnimationBlenderV2::ANIM_JUMP_START))
            {
                this->playerController->getAnimationBlender()->blend(NOWA::AnimationBlenderV2::ANIM_JUMP_START, NOWA::AnimationBlenderV2::BlendWhileAnimating, 0.5f, false);
            }
            if (this->playerController->getAnimationBlender()->getTimePosition() >= this->playerController->getAnimationBlender()->getLength() - 0.3f)
            {
                this->playerController->getAnimationBlender()->setTimePosition(0.7f);
            }
            this->jumpSound->play();
        }
        // Falling: in air, no jump key, moving with gravity.
        // Fixed: was "velocity.y < -1.0f" which breaks on tilted planet surfaces.
        else if (true == this->inAir && false == this->jumpKeyPressed && true == isFalling)
        {
            this->jumpCount = 0;
            if (false == this->playerController->getAnimationBlender()->isAnimationActive(NOWA::AnimationBlenderV2::ANIM_JUMP_START))
            {
                this->playerController->getAnimationBlender()->blend(NOWA::AnimationBlenderV2::ANIM_JUMP_START, NOWA::AnimationBlenderV2::BlendWhileAnimating, 0.5f, false);
            }
            else if (this->playerController->getAnimationBlender()->getTimePosition() >= this->playerController->getAnimationBlender()->getLength() - 0.3f)
            {
                // Hold last frame of jump_start while airborne.
                this->playerController->getAnimationBlender()->setTimePosition(this->playerController->getAnimationBlender()->getTimePosition());
            }
        }

        // Landing: just touched ground while moving with gravity.
        // groundedOnce prevents the landing animation from retriggering every frame.
        if (false == this->inAir && true == isFalling && false == this->groundedOnce)
        {
            if (false == this->playerController->getAnimationBlender()->isAnimationActive(NOWA::AnimationBlenderV2::ANIM_JUMP_END))
            {
                this->playerController->getAnimationBlender()->blend(NOWA::AnimationBlenderV2::ANIM_JUMP_END, NOWA::AnimationBlenderV2::BlendWhileAnimating, 0.5f, false);
            }
            this->boringTimer = 0;
            this->walkSound->setPitch(0.35f);
            this->walkSound->setGain(0.55f);
            this->walkSound->play();
            this->groundedOnce = true;
        }

        // Reset groundedOnce once the player has risen above the landing threshold.
        if (height > 2.0f)
        {
            this->groundedOnce = false;
        }

        // -------------------------------------------------------------------------
        // Jump force application
        // -------------------------------------------------------------------------
        Ogre::Vector3 jumpVelocity = Ogre::Vector3::ZERO;

        const bool doNormalJump = this->jumpKeyPressed && false == this->isJumping && false == this->inAir;
        const bool doDoubleJump = this->playerController->getDoubleJump() && this->jumpCount == 2;

        if (1.0f == this->playerController->getJumpWeight() && (true == doNormalJump || true == doDoubleJump))
        {
            this->boringTimer = 0.0f;

            // Use gravity-relative vertical speed for the jump threshold check.
            // Fixed: was "getVelocity().y" which breaks on tilted planet surfaces.
            const Ogre::Real verticalSpeed = currentVelocity.dotProduct(upDir);

            if (verticalSpeed <= this->jumpForce * static_cast<Ogre::Real>(this->jumpCount))
            {
                if (false == this->hasPhysicsPlayerControllerComponent)
                {
                    // Jump along the actual up direction relative to the planet surface.
                    jumpVelocity = upDir * this->jumpForce * this->playerController->getJumpWeight();
                    this->playerController->getPhysicsComponent()->applyRequiredForceForJumpVelocity(jumpVelocity);
                }
                else
                {
                    static_cast<PhysicsPlayerControllerComponent*>(this->playerController->getPhysicsComponent())->move(0.0f, tempSpeed, heading);
                    static_cast<PhysicsPlayerControllerComponent*>(this->playerController->getPhysicsComponent())->setJumpSpeed(this->jumpForce * this->playerController->getJumpWeight());
                    static_cast<PhysicsPlayerControllerComponent*>(this->playerController->getPhysicsComponent())->jump();
                }
                if (this->jumpCount >= 2)
                {
                    this->jumpCount = 0;
                }
            }
            else
            {
                this->isJumping = true;
            }
        }

        // Consume jump attempt if player was airborne or already jumping.
        if (true == this->jumpKeyPressed && (true == this->inAir || true == this->isJumping))
        {
            this->tryJump = true;
        }

        // -------------------------------------------------------------------------
        // Angular velocity (yaw) -- gravity-relative axis, already correct.
        // -------------------------------------------------------------------------
        Ogre::Vector3 desiredAngularVelocity = -gravityDir * yawAtSpeed;

        if (false == this->hasPhysicsPlayerControllerComponent)
        {
            if (yawAtSpeed != 0.0f)
            {
                this->playerController->getPhysicsComponent()->applyOmegaForce(desiredAngularVelocity);
            }
            else
            {
                this->playerController->getPhysicsComponent()->applyOmegaForceKeepUpright(10.0f);
            }
        }

        // -------------------------------------------------------------------------
        // Velocity decomposition -- preserve vertical (gravity) component, apply
        // horizontal movement in the key direction.
        // -------------------------------------------------------------------------
        Ogre::Vector3 verticalVelocity = gravityDir * currentVelocity.dotProduct(gravityDir);
        Ogre::Vector3 directionMove = this->keyDirection * tempSpeed * this->acceleration;
        Ogre::Vector3 newVelocity = verticalVelocity + directionMove;

        if (false == this->hasPhysicsPlayerControllerComponent)
        {
            this->playerController->getPhysicsComponent()->applyRequiredForceForVelocity(newVelocity);
        }
        else
        {
            static_cast<PhysicsPlayerControllerComponent*>(this->playerController->getPhysicsComponent())->move(0.0f, tempSpeed, heading);
        }

        // Walk sound -- only when grounded and actually moving.
        if (false == this->inAir && this->direction != Direction::NONE)
        {
            this->walkSound->play();
            this->walkSound->setVelocity(newVelocity);
            this->walkSound->setPitch(0.65f);
            this->walkSound->setGain(0.35f);
        }

        // Jump key release resets all jump state so the next press can trigger again.
        if (false == inputDeviceModule->isActionDown(NOWA_A_JUMP))
        {
            this->tryJump = false;
            this->isJumping = false;
            this->canDoubleJump = true;
        }

        // -------------------------------------------------------------------------
        // Single addTime call at the end -- always. Mirrors the click2point pattern.
        // -------------------------------------------------------------------------
        this->playerController->getAnimationBlender()->addTime(dt * tempAnimationSpeed / this->playerController->getAnimationBlender()->getLength(), this->playerController->getClassName());
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
		mouseY(0),
		maxHeightDifference(1.0f),
        middleButtonWasDown(false),
        lastPathMouseX(-1),
        lastPathMouseY(-1),
        pendingPath(false),
        pendingPosOnNavMesh(Ogre::Vector3::ZERO),
        raycastThrottleTimer(0.0f),
        raycastThrottleInterval(0.125f),
        lastKnownClickPosition(Ogre::Vector3::ZERO),
        hasLastKnownClickPosition(false)
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

        // init() in connect() already set ANIM_IDLE_1 at full weight via render command.
        // Do NOT call blend() here — blending idle->idle produces a 0.5s fake crossfade
        // from weight=1 down through bind pose and back up, causing the visible jitter.
        // blend() is only for transitions between DIFFERENT animation clips.

        this->boringTimer = 6.0f;
        this->movingBehavior = this->playerController->getMovingBehavior();
        this->raySceneQuery = this->playerController->getOwner()->getSceneManager()->createRayQuery(Ogre::Ray(), this->playerController->getCategoriesId());
        this->raySceneQuery->setSortByDistance(true);
        this->maxHeightDifference = AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreRecast()->getAgentHeight();
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

        if (nullptr == this->playerController->getPhysicsComponent())
        {
            return;
        }

        if (nullptr == this->playerController->getInputDeviceComponent() || true == this->playerController->getInputDeviceComponent()->isDeviceLocked())
        {
            return;
        }

        this->playerController->getAnimationBlender()->beginFrame();

        Ogre::Real tempSpeed = this->playerController->getPhysicsComponent()->getSpeed();
        Ogre::Real tempAnimationSpeed = this->playerController->getAnimationSpeed();

        // ── Poll: apply new waypoints as soon as the async path task finishes ─────
        //
        // This section runs every frame without blocking the logic thread.
        //
        // Design rationale — why polling instead of waitForPath():
        //   waitForPath() blocks the logic thread until Recast finishes. At 144fps
        //   that is invisible for a single click, but with autoClick + mouse movement
        //   it fires multiple times per second and causes visible frame stutter.
        //   Polling with isPathReady() (wait_for(0ms)) never blocks — it returns
        //   immediately with true/false and only collects the future result when ready.
        //
        // Design rationale — why the path is NOT cleared in the click handler:
        //   The old approach cleared movingBehavior->getPath() the moment a new click
        //   was detected, before the async path task had finished. The agent then had
        //   zero waypoints for 1-3 frames → stopped → stuttered → resumed. This was
        //   the root cause of the jerky autoClick movement.
        //   Now the agent keeps following the current path while the new one is being
        //   calculated. The path is cleared and replaced atomically here, only when
        //   the new path data is fully ready. The direction change is seamless.
        if (true == this->pendingPath && true == this->ogreRecastModule->isPathReady(this->playerController->getPathSlot()))
        {
            this->pendingPath = false;

            std::vector<Ogre::Vector3> path = this->ogreRecastModule->getOgreRecast()->getPath(this->playerController->getPathSlot());

            if (false == path.empty())
            {
                // Fire the Lua callback now that the target position is confirmed reachable.
                LuaScript* luaScript = this->playerController->getOwner()->getLuaScript();
                if (nullptr != luaScript)
                {
                    Ogre::Vector3 pos = this->pendingPosOnNavMesh;
                    NOWA::AppStateManager::LogicCommand logicCommand = [this, luaScript, pos]()
                    {
                        luaScript->callTableFunction("onNavMeshClicked", pos);
                    };
                    NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
                }

                this->hasGoal = true;

                // Clear the old path and install the new one NOW — path data is fully
                // written by the async task and isPathReady() has confirmed this.
                // No data race: the shared_lock in FindPathWithQuery was released before
                // the future became ready, and we hold no lock here on the logic thread.
                this->movingBehavior->getPath()->clear();

                // Add each time behavior, because if goal is reached, or no path, the behavior will be removed
                this->movingBehavior->removeBehavior(NOWA::KI::MovingBehavior::FOLLOW_PATH);
                this->movingBehavior->addBehavior(NOWA::KI::MovingBehavior::FOLLOW_PATH);

                AppStateManager::getSingletonPtr()->getCameraManager()->setMoveCameraWeight(0.1f);
                AppStateManager::getSingletonPtr()->getCameraManager()->setRotateCameraWeight(0.1f);

                if (true == this->playerController->getDrawPath())
                {
                    auto recastPtr = this->ogreRecastModule->getOgreRecast();
                    int slot = this->playerController->getPathSlot();
                    // Non-blocking enqueue: no logic thread stall, no staging buffer pressure.
                    // Path data is stable here so there is no race with CreateRecastPathLine
                    // reading m_PathStore on the render thread.
                    NOWA::GraphicsModule::RenderCommand renderCommand = [recastPtr, slot]()
                    {
                        recastPtr->CreateRecastPathLine(slot, true);
                    };
                    NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "PathFollowState3D::DrawPathLine");
                }

                auto playerController = this->playerController;

				NOWA::GraphicsModule::RenderCommand renderCommand = [this, path, playerController]()
                {
                    for (size_t i = 0; i < path.size(); i++)
                    {
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
                                    Ogre::Item* item = this->playerController->getOwner()->getSceneManager()->createItem("Node.mesh");
                                    this->playerController->debugWaypointNode->attachObject(item);
                                }
                                this->playerController->debugWaypointNode->setPosition(resultWaypoint);
                            }
                        }
                    }
                };
                NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "OgreRecastModule::AddWaypoints");

                this->playerController->setMoveWeight(1.0f);
                this->playerController->setJumpWeight(1.0f);
            }
        }

        // ── Click / hold detection ────────────────────────────────────────────────
        //
        // Edge trigger: only act on the TRANSITION from not-pressed to pressed.
        //
        // Why not a level trigger (ms.buttonDown every frame)?
        //   A level trigger fires every frame while the button is held — up to 144
        //   times per second. Each fire launches a raycast and a path request. Even
        //   with the async path design this causes unnecessary CPU/GPU load and can
        //   overwhelm the raycast cache warm-up (enqueueAndWait per unique mesh hit).
        //
        // buttonJustPressed is true for exactly ONE frame per physical click.
        // The animation state machine at the bottom always runs regardless.
        const bool buttonDown = ms.buttonDown(OIS::MB_Middle);
        const bool buttonJustPressed = buttonDown && false == this->middleButtonWasDown;
        this->middleButtonWasDown = buttonDown;

        // autoClick = false: fire only on the rising edge (one path per click).
        // autoClick = true:  Continous RTS Style — also fire while held if the mouse moved,
        //                    but throttled to RAYCAST_THROTTLE_INTERVAL seconds between
        //                    fires so getMeshInformation2 cache misses (enqueueAndWait,
        //                    ~7ms render-frame stall) happen at most 8x per second
        //                    instead of 144x. After the cache is warm all raycasts are
        //                    pure CPU and the throttle has negligible effect on feel.
        const bool mouseMoved = (ms.X.abs != this->lastPathMouseX || ms.Y.abs != this->lastPathMouseY);
        const bool autoClickFire = buttonDown && true == this->playerController->getAutoClick() && true == mouseMoved && this->raycastThrottleTimer <= 0.0f;
        const bool shouldProcess = buttonJustPressed || autoClickFire;

        // Count down the throttle timer every frame.
        if (this->raycastThrottleTimer > 0.0f)
        {
            this->raycastThrottleTimer -= dt;
        }

        if (true == this->playerController->getGameObject()->isSelected() && true == shouldProcess)
        {
            // Record mouse position so the next frame can detect movement.
            this->lastPathMouseX = ms.X.abs;
            this->lastPathMouseY = ms.Y.abs;

            // Reset throttle so the next autoClick fire is delayed.
            if (true == autoClickFire)
            {
                this->raycastThrottleTimer = 1.0f / 8.0f;
            }

            // Add delay to camera behavior if target location has been clicked, so that the scene will not be rotated to early
            if (nullptr != this->playerController->getCameraBehaviorComponent())
            {
                NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(1.0f));
                delayProcess->attachChild(NOWA::ProcessPtr(new UpdateCameraBehaviorProcess()));
                NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
            }

            std::vector<Ogre::MovableObject*> excludeObjects;
            excludeObjects.emplace_back(this->playerController->getOwner()->getMovableObject());

            Ogre::Vector3 clickedPosition = Ogre::Vector3::ZERO;

            /*bool success = MathHelper::getInstance()->getRaycastFromPoint(mouseX, mouseY, AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera(),
                Core::getSingletonPtr()->getOgreRenderWindow(), raySceneQuery, clickedPosition,
                movableObjectLocal, closestDistance, normal, &excludeObjects, false);*/

            // Raycast against the full scene (terrain, items, buildings, everything).
            // On the first hit of a new mesh getMeshInformation2 will enqueueAndWait
            // once to fill the CPU cache. All subsequent hits on the same mesh are
            // pure CPU (shared_ptr cache lookup, no GPU access, no fence). The throttle
            // above limits how often this first-hit stall can occur in autoClick mode.
            bool success = MathHelper::getInstance()->getRaycastForFrame(ms.X.abs, ms.Y.abs, AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera(), Core::getSingletonPtr()->getOgreRenderWindow(), this->raySceneQuery,
                excludeObjects, clickedPosition);

            if (true == success)
            {
                Ogre::String name = this->playerController->getOwner()->getName();

                this->ogreRecastModule->getOgreRecast()->getPath(this->playerController->getPathSlot()).clear();
                Ogre::Vector3 posOnNavMesh = Ogre::Vector3::ZERO;

                // http://www.stevefsp.org/projects/rcndoc/prod/classdtNavMeshQuery.html
                // https://forums.ogre3d.org/viewtopic.php?t=62079
                // Attention: This line will always find a path, even the user clicked on a non navigable place, so the nearest position to that place is used, which may not be what the user wants for his game
                if (this->ogreRecastModule->getOgreRecast()->findNearestPointOnNavmesh(clickedPosition + Ogre::Vector3(0.0f, 0.3f, 0.0f), posOnNavMesh))
                {
                    // Check if the result is within an acceptable height range
                    if (std::abs(posOnNavMesh.y - clickedPosition.y) > this->maxHeightDifference)
                    {
                        posOnNavMesh.y = clickedPosition.y; // Ignore the result if it's too far from the click
                    }
                    // y immer 0.5 statt höher argghhh
                    // Ogre::LogManager::getSingletonPtr()->logMessage("findNearestPointOnNavmesh: " + Ogre::StringConverter::toString(posOnNavMesh));

                    /*Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PathFollowState3D] " + this->playerController->getOwner()->getName() +
                                                                                            " attempting findPath with pathSlot=" + Ogre::StringConverter::toString(this->playerController->getPathSlot()) +
                                                                                            " isSelected=" + Ogre::StringConverter::toString(this->playerController->getGameObject()->isSelected()) +
                                                                                            " MB_Middle=" + Ogre::StringConverter::toString(ms.buttonDown(OIS::MB_Middle)));*/

                    // Attention: What is with targetSlot? Its here 0
                    //
                    // Launch the async path task — returns immediately without blocking.
                    // The task runs FindPathWithQuery on its own dtNavMeshQuery instance
                    // (one per pathSlot) so there is no shared state with other slots or
                    // the render thread.
                    //
                    // CRITICAL: we do NOT clear movingBehavior->getPath() here.
                    // The agent keeps following the current path while the new one is
                    // being calculated. Clearing it here would leave the agent with zero
                    // waypoints for 1-3 frames, causing the stutter / stop / resume
                    // pattern that was the root complaint. The path is cleared and
                    // replaced in the poll section above, only after isPathReady().
                    bool launched = this->ogreRecastModule->findPath(this->playerController->getPosition(), posOnNavMesh + Ogre::Vector3(0.0f, 0.3f, 0.0f), this->playerController->getPathSlot(), 0, false);

                    /*Ogre::LogManager::getSingletonPtr()->logMessage("#############findPath size: "
                    + Ogre::StringConverter::toString(this->ogreRecastModule->getOgreRecast()->getPath(0).size())
                    + " y offset: " + Ogre::StringConverter::toString(this->ogreRecastModule->getOgreRecast()->getNavmeshOffsetFromGround()));*/

                    if (true == launched)
                    {
                        // Mark that a path result is incoming. The poll section will pick
                        // it up in the next frame where isPathReady() returns true.
                        this->pendingPath = true;
                        this->pendingPosOnNavMesh = posOnNavMesh;
                    }
                    else if (false == this->playerController->getAutoClick())
                    {
                        // findPath returned false (pending obstacles, build in progress, etc.)
                        // For a single click (autoClick=false) it is correct to clear the path
                        // here — the user explicitly clicked somewhere unreachable right now.
                        // For autoClick the agent just keeps moving on the previous path until
                        // obstacles are resolved and the next throttle interval fires.
                        if (nullptr != this->movingBehavior->getPath())
                        {
                            this->movingBehavior->getPath()->clear();
                        }
                    }
                }
                else
                {
                    // findNearestPointOnNavmesh failed — click was completely off-navmesh.
                    if (nullptr != this->movingBehavior->getPath())
                    {
                        this->movingBehavior->getPath()->clear();
                    }
                }
            }
        }

        // -----------------------------------------------------------------------
        // Per-frame animation state machine.
        //
        // Evaluated unconditionally every frame — NOT only on click.
        // This guarantees transitions are never missed due to the one-frame async
        // window between ENQUEUE_RENDER_COMMAND_MULTI_WAIT (waypoint population)
        // and the blender's internalBlend command firing on the render thread.
        //
        // Rule: waypoints remaining -> walk. No waypoints -> idle / boring idles.
        // -----------------------------------------------------------------------
        const bool hasWaypoints = this->movingBehavior->getPath()->getRemainingWaypoints() > 0;

        if (true == hasWaypoints)
        {
            // Moving — drive walk animation if not already active.
            if (false == this->playerController->getAnimationBlender()->isAnimationActive(NOWA::AnimationBlenderV2::ANIM_WALK_NORTH))
            {
                tempAnimationSpeed = this->playerController->getAnimationSpeed();
                this->playerController->getAnimationBlender()->blend(NOWA::AnimationBlenderV2::ANIM_WALK_NORTH, NOWA::AnimationBlenderV2::BlendWhileAnimating, 0.2f, true);
            }
        }
        else
        {
            // TODO: Just a test
            // this->ogreRecastModule->debugLogPath(this->playerController->getPathSlot(), this->movingBehavior->getAgentId());
            // No waypoints — handle goal arrival and boring idle logic.
            if (true == this->hasGoal)
            {
                // Goal just reached — snap boringTimer close to threshold so a
                // random idle fires quickly instead of waiting a full 5 seconds.
                this->boringTimer = 4.9f;
                this->hasGoal = false;

                // Immediately blend back to idle_1 when path is exhausted.
                if (false == this->playerController->getAnimationBlender()->isAnimationActive(NOWA::AnimationBlenderV2::ANIM_IDLE_1))
                {
                    this->playerController->getAnimationBlender()->blend(NOWA::AnimationBlenderV2::ANIM_IDLE_1, NOWA::AnimationBlenderV2::BlendWhileAnimating, 0.3f, true);
                }
            }

            this->boringTimer += dt;

            // After 5 seconds of standing still, play a random idle variation.
            if (this->boringTimer >= 5.0f)
            {
                int id = MathHelper::getInstance()->getRandomNumber<int>(0, 3);
                NOWA::AnimationBlenderV2::AnimID animID;
                if (0 == id)
                {
                    animID = NOWA::AnimationBlenderV2::ANIM_IDLE_1;
                }
                else if (1 == id)
                {
                    animID = NOWA::AnimationBlenderV2::ANIM_IDLE_2;
                }
                else
                {
                    animID = NOWA::AnimationBlenderV2::ANIM_IDLE_3;
                }

                if (true == this->playerController->getAnimationBlender()->hasAnimation(animID))
                {
                    if (false == this->playerController->getAnimationBlender()->isAnimationActive(animID) || true == this->playerController->getAnimationBlender()->isComplete())
                    {
                        this->playerController->getAnimationBlender()->blend(animID, NOWA::AnimationBlenderV2::BlendWhileAnimating, 0.5f, true);
                        this->boringTimer = 0.0f;
                    }
                }
            }
        }

        // Advance animation — single call at the end, always.
        this->playerController->getAnimationBlender()->addTime(dt * tempAnimationSpeed, this->playerController->getClassName());
    }

	void PathFollowState3D::exit(GameObject* player)
	{
		this->playerController->getAnimationBlender()->init(NOWA::AnimationBlenderV2::ANIM_IDLE_1);
		this->playerController->getAnimationBlender()->blend(NOWA::AnimationBlenderV2::ANIM_IDLE_1, NOWA::AnimationBlenderV2::BlendWhileAnimating, 0.5f, true);
		this->boringTimer = 6.0f;
		this->movingBehavior->removeBehavior(NOWA::KI::MovingBehavior::FOLLOW_PATH);
	}

}; // namespace end
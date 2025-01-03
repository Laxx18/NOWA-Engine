#include "NOWAPrecompiled.h"
#include "AiComponents.h"
#include "GameObjectController.h"
#include "PhysicsActiveComponent.h"
#include "NodeComponent.h"
#include "AiLuaComponent.h"

#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"

#include "main/AppStateManager.h"
#include "main/ProcessManager.h"
#include "main/Core.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	AiComponent::AiComponent()
		: GameObjectComponent(),
		behaviorTypeId(NOWA::KI::MovingBehavior::NONE),
		activated(new Variant(AiComponent::AttrActivated(), true, this->attributes)),
		rotationSpeed(new Variant(AiComponent::AttrRotationSpeed(), 10.0f, this->attributes)),
		flyMode(new Variant(AiComponent::AttrFlyMode(), false, this->attributes)),
		stuckTime(new Variant(AiComponent::AttrStuckTime(), 0.0f, this->attributes)),
		autoOrientation(new Variant(AiComponent::AttrAutoOrientation(), true, this->attributes)),
		autoAnimation(new Variant(AiComponent::AttrAutoAnimation(), false, this->attributes)),
		agentId(new Variant(AiComponent::AttrAgentId(), static_cast<unsigned long>(0), this->attributes, true)),

		pathGoalObserver(nullptr),
		agentStuckObserver(nullptr)
	{
		this->stuckTime->setDescription("The time in seconds the agent is stuck, until the current behavior will be removed. 0 disables the stuck check.");
		this->autoAnimation->setDescription("Sets whether to use auto animation during ai movement. That is, the animation speed is adapted dynamically depending the velocity, which will create a much more realistic effect.Note: The game object must have a proper configured animation component.");
		this->agentId->setDescription("Sets an optional agent id, which shall be moved. E.g. setting a waypoint and the target agent shall move to it.");
	}

	AiComponent::~AiComponent()
	{
		// Dangerous in destructor, as when exiting the simulation, the game object will be deleted and this function called, to seek for another ai component, that has been deleted
		// Thus its done in onRemoveComponent
	}

	bool AiComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RotationSpeed")
		{
			this->rotationSpeed->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FlyMode")
		{
			this->flyMode->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "StuckTime")
		{
			this->stuckTime->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AutoOrientation")
		{
			this->autoOrientation->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AutoAnimation")
		{
			this->autoAnimation->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AgentId")
		{
			this->agentId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	bool AiComponent::postInit(void)
	{
		if (true == this->activated->getBool())
		{
			if (0 != this->agentId->getULong())
			{
				GameObjectPtr sourceGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->agentId->getULong());
				if (nullptr != sourceGameObjectPtr)
				{
					this->movingBehaviorPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->addMovingBehavior(this->agentId->getULong());
				}
			}
			else
			{
				this->movingBehaviorPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->addMovingBehavior(this->gameObjectPtr->getId());
			}
			if (nullptr == this->movingBehaviorPtr)
			{
				return false;
			}
		}

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		return true;
	}

	bool AiComponent::connect(void)
	{
		if (nullptr != this->movingBehaviorPtr)
		{
			// Since its a mask, add or remove from mask
			if (true == this->activated->getBool())
			{
				this->movingBehaviorPtr->addBehavior(this->behaviorTypeId);
			}
			else
			{
				this->movingBehaviorPtr->removeBehavior(this->behaviorTypeId);
			}
			this->movingBehaviorPtr->setRotationSpeed(this->rotationSpeed->getReal());
			this->movingBehaviorPtr->setFlyMode(this->flyMode->getBool());
			this->movingBehaviorPtr->setStuckCheckTime(this->stuckTime->getReal());
			this->movingBehaviorPtr->setAutoOrientation(this->autoOrientation->getBool());
			this->movingBehaviorPtr->setAutoAnimation(this->autoAnimation->getBool());
		}
		return true;
	}

	bool AiComponent::disconnect(void)
	{
		if (nullptr != this->movingBehaviorPtr)
		{
			this->movingBehaviorPtr->reset();
		}
		return true;
	}

	void AiComponent::tryRemoveMovingBehavior(void)
	{
		if (nullptr != this->movingBehaviorPtr)
		{
			this->movingBehaviorPtr->setAgentId(0);
		}

		// Dangerous in destructor, as when exiting the simulation, the game object will be deleted and this function called, to seek for another ai component, that has been deleted
		// Thus handle it, just when a component is removed
		bool stillAiComponentActive = false;
		for (size_t i = 0; i < this->gameObjectPtr->getComponents()->size(); i++)
		{
			AiCompPtr aiCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<AiComponent>());
			// Seek for this component and go to the previous one to process
			if (aiCompPtr != nullptr && aiCompPtr.get() != this)
			{
				stillAiComponentActive = true;
				break;
			}
			else
			{
				auto aiLuaCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<AiLuaComponent>());
				// Seek for this component and go to the previous one to process
				if (aiLuaCompPtr != nullptr)
				{
					stillAiComponentActive = true;
					break;
				}
			}
		}

		// If there is no ai component for this game object anymore, remove the behavior
		if (false == stillAiComponentActive)
		{
			if (0 != this->agentId->getULong())
			{
				AppStateManager::getSingletonPtr()->getGameObjectController()->removeMovingBehavior(this->agentId->getULong());
			}
			else
			{
				AppStateManager::getSingletonPtr()->getGameObjectController()->removeMovingBehavior(this->gameObjectPtr->getId());
			}
		}
	}

	void AiComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();
		
		this->tryRemoveMovingBehavior();
	}

	void AiComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);
		
		if (AiComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (AiComponent::AttrRotationSpeed() == attribute->getName())
		{
			this->setRotationSpeed(attribute->getReal());
		}
		else if (AiComponent::AttrFlyMode() == attribute->getName())
		{
			this->setFlyMode(attribute->getBool());
		}
		else if (AiComponent::AttrStuckTime() == attribute->getName())
		{
			this->setStuckTime(attribute->getReal());
		}
		else if (AiComponent::AttrAutoOrientation() == attribute->getName())
		{
			this->setAutoOrientation(attribute->getBool());
		}
		else if (AiComponent::AttrAutoAnimation() == attribute->getName())
		{
			this->setAutoAnimation(attribute->getBool());
		}
		else if (AiComponent::AttrAgentId() == attribute->getName())
		{
			this->setAgentId(attribute->getULong());
		}
	}

	void AiComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FlyMode"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->flyMode->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "StuckTime"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->stuckTime->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AutoOrientation"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->autoOrientation->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AutoAnimation"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->autoAnimation->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AgentId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->agentId->getULong())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String AiComponent::getClassName(void) const
	{
		return "AiComponent";
	}

	Ogre::String AiComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	bool AiComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// Check if there is a physics active component and no ai lua component (because it already has a moving behavior)
		auto aiLuaCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<AiLuaComponent>());
		if (nullptr == aiLuaCompPtr)
		{
			return true;
		}
		return false;
	}

	void AiComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
		if (true == activated)
		{
			if (0 != this->agentId->getULong())
			{
				GameObjectPtr sourceGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->agentId->getULong());
				if (nullptr != sourceGameObjectPtr)
				{
					this->movingBehaviorPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->addMovingBehavior(this->agentId->getULong());
				}
			}
			else
			{
				this->movingBehaviorPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->addMovingBehavior(this->gameObjectPtr->getId());
			}

			if (nullptr != this->movingBehaviorPtr)
			{
				this->movingBehaviorPtr->addBehavior(this->behaviorTypeId);
			}
		}
		else
		{
			if (nullptr != this->movingBehaviorPtr)
			{
				// Only delete later if not in the middle of destruction of all game objects
				if (false == Core::getSingletonPtr()->getIsSceneBeingDestroyed())
				{
					// Delete later, as this may be called from lua script and cause trouble else
					NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.25f));
					auto ptrFunction = [this]()
					{
						this->movingBehaviorPtr->removeBehavior(this->behaviorTypeId);
						this->tryRemoveMovingBehavior();
						delete this->pathGoalObserver;
						this->pathGoalObserver = nullptr;
						this->movingBehaviorPtr->setPathGoalObserver(nullptr);
						this->movingBehaviorPtr.reset();
					};
					NOWA::ProcessPtr closureProcess(new NOWA::ClosureProcess(ptrFunction));
					delayProcess->attachChild(closureProcess);
					NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
				}
				else
				{
					this->movingBehaviorPtr->removeBehavior(this->behaviorTypeId);
					this->tryRemoveMovingBehavior();
					delete this->pathGoalObserver;
					this->pathGoalObserver = nullptr;
					this->movingBehaviorPtr->setPathGoalObserver(nullptr);
					this->movingBehaviorPtr.reset();
				}
			}
		}
	}

	bool AiComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void AiComponent::setRotationSpeed(Ogre::Real rotationSpeed)
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

	Ogre::Real AiComponent::getRotationSpeed(void) const
	{
		return rotationSpeed->getReal();
	}

	void AiComponent::setFlyMode(bool flyMode)
	{
		this->flyMode->setValue(flyMode);
	}

	bool AiComponent::getIsInFlyMode(void) const
	{
		return this->flyMode->getBool();
	}

	void AiComponent::setStuckTime(Ogre::Real stuckTime)
	{
		this->stuckTime->setValue(stuckTime);
	}

	Ogre::Real AiComponent::getStuckeTime(void) const
	{
		return this->stuckTime->getReal();
	}

	void AiComponent::setAutoOrientation(bool autoOrientation)
	{
		this->autoOrientation->setValue(autoOrientation);
	}

	bool AiComponent::getIsAutoOrientated(void) const
	{
		return this->autoOrientation->getBool();
	}

	void AiComponent::setAutoAnimation(bool autoAnimation)
	{
		this->autoAnimation->setValue(autoAnimation);
	}

	bool AiComponent::getIsAutoAnimated(void) const
	{
		return this->autoAnimation->getBool();
	}

	boost::weak_ptr<KI::MovingBehavior> AiComponent::getMovingBehavior(void) const
	{
		return this->movingBehaviorPtr;
	}

	void AiComponent::reactOnPathGoalReached(luabind::object closureFunction)
	{
		if (nullptr == this->movingBehaviorPtr)
		{
			return;
		}

		if (nullptr == this->pathGoalObserver)
		{
			this->pathGoalObserver = new PathGoalObserver();
		}
		else
		{
			static_cast<PathGoalObserver*>(this->pathGoalObserver)->reactOnPathGoalReached(closureFunction);
		}
		this->movingBehaviorPtr->setPathGoalObserver(this->pathGoalObserver);
		static_cast<PathGoalObserver*>(this->pathGoalObserver)->reactOnPathGoalReached(closureFunction);
	}

	void AiComponent::reactOnAgentStuck(luabind::object closureFunction)
	{
		if (nullptr == this->movingBehaviorPtr)
		{
			return;
		}

		if (nullptr == this->agentStuckObserver)
		{
			this->agentStuckObserver = new AgentStuckObserver();
		}
		else
		{
			static_cast<AgentStuckObserver*>(this->agentStuckObserver)->reactOnAgentStuck(closureFunction);
		}
		this->movingBehaviorPtr->setAgentStuckObserver(this->agentStuckObserver);
		static_cast<AgentStuckObserver*>(this->agentStuckObserver)->reactOnAgentStuck(closureFunction);
	}

	void AiComponent::setAgentId(unsigned long agentId)
	{
		if (0 != this->agentId->getULong())
		{
			// Removes the old behavior
			GameObjectPtr sourceGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(agentId);
			if (nullptr != sourceGameObjectPtr)
			{
				AppStateManager::getSingletonPtr()->getGameObjectController()->removeMovingBehavior(agentId);
				this->movingBehaviorPtr.reset();

				this->movingBehaviorPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->addMovingBehavior(agentId);

				auto aiLuaCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<AiLuaComponent>());
				// If there is an ai lua component, also reset the moving behavior for that one
				if (aiLuaCompPtr != nullptr)
				{
					aiLuaCompPtr->disconnect();
					aiLuaCompPtr->connect();
				}
			}
		}

		this->agentId->setValue(agentId);
	}

	unsigned long AiComponent::getAgentId(void) const
	{
		return this->agentId->getULong();
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	AiMoveComponent::AiMoveComponent()
		: AiComponent(),
		behaviorType(new Variant(AiMoveComponent::AttrBehaviorType(), { "Move", "Seek", "Flee", "Arrive", "Pursuit", "Evade" }, this->attributes)),
		targetId(new Variant(AiMoveComponent::AttrTargetId(), static_cast<unsigned long>(0), this->attributes, true))
	{
		this->behaviorTypeId = KI::MovingBehavior::MOVE;
	}

	AiMoveComponent::~AiMoveComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AiMoveComponent] Destructor ai move component for game object: " + this->gameObjectPtr->getName());
	}

	bool AiMoveComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		bool success = AiComponent::init(propertyElement);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BehaviorType")
		{
			this->behaviorType->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "Move"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetId")
		{
			this->targetId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		return success;
	}

	GameObjectCompPtr AiMoveComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		AiMoveCompPtr clonedCompPtr(boost::make_shared<AiMoveComponent>());

		clonedCompPtr->setRotationSpeed(this->rotationSpeed->getReal());
		clonedCompPtr->setFlyMode(this->flyMode->getBool());
		clonedCompPtr->setStuckTime(this->stuckTime->getReal());
		clonedCompPtr->setAutoOrientation(this->autoOrientation->getBool());

		clonedCompPtr->setBehaviorType(this->behaviorType->getListSelectedValue());
		clonedCompPtr->setTargetId(this->targetId->getULong());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->setActivated(this->activated->getBool());

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool AiMoveComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AiMoveComponent] Init ai move component for game object: " + this->gameObjectPtr->getName());

		return AiComponent::postInit();
	}

	bool AiMoveComponent::connect(void)
	{
		if (nullptr != this->movingBehaviorPtr)
		{
			// Set the chosen behavior type, before its added in AiComponent::connect
			this->behaviorTypeId = this->movingBehaviorPtr->mapBehavior(this->behaviorType->getListSelectedValue());
		}
		
		bool success = AiComponent::connect();

		if (nullptr != this->movingBehaviorPtr)
		{
			this->movingBehaviorPtr->setTargetAgentId(this->targetId->getULong());
		}
		return success;
	}

	bool AiMoveComponent::disconnect(void)
	{
		return AiComponent::disconnect();
	}

	bool AiMoveComponent::onCloned(void)
	{
		// Search for the prior id of the cloned game object and set the new id and set the new id, if not found set better 0, else the game objects may be corrupt!
		if (0 != this->targetId->getULong())
		{
			GameObjectPtr targetGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getClonedGameObjectFromPriorId(this->targetId->getULong());
			if (nullptr != targetGameObjectPtr)
			{
				this->targetId->setValue(targetGameObjectPtr->getId());
			}
			// Since connect is called during cloning process, it does not make sense to process further here, but only when simulation started!
		}
		return true;
	}

	void AiMoveComponent::update(Ogre::Real dt, bool notSimulating)
	{
		// Moving behavior is updated in game object controller, because it has a map for moving behavior and the game object
		// because, a game object may have x ai components, which are working on one moving behavior and getting calculated priortized for the game object
	}

	void AiMoveComponent::actualizeValue(Variant* attribute)
	{
		AiComponent::actualizeValue(attribute);
		
		if (AiMoveComponent::AttrBehaviorType() == attribute->getName())
		{
			this->setBehaviorType(attribute->getListSelectedValue());
		}
		else if (AiMoveComponent::AttrTargetId() == attribute->getName())
		{
			this->setTargetId(attribute->getULong());
		}
	}

	void AiMoveComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		AiComponent::writeXML(propertiesXML, doc);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BehaviorType"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->behaviorType->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TargetId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->targetId->getULong())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String AiMoveComponent::getClassName(void) const
	{
		return "AiMoveComponent";
	}

	Ogre::String AiMoveComponent::getParentClassName(void) const
	{
		return "AiComponent";
	}
	
	void AiMoveComponent::setBehaviorType(const Ogre::String& behaviorType)
	{
		this->behaviorType->setListSelectedValue(behaviorType);
	}
	
	Ogre::String AiMoveComponent::getBehaviorType(void) const
	{
		return this->behaviorType->getListSelectedValue();
	}
	
	void AiMoveComponent::setTargetId(unsigned long targetId)
	{
		this->targetId->setValue(targetId);
	}

	unsigned long AiMoveComponent::getTargetId(void) const
	{
		return this->targetId->getULong();
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	AiMoveRandomlyComponent::AiMoveRandomlyComponent()
		: AiComponent()
	{
		this->behaviorTypeId = KI::MovingBehavior::MOVE_RANDOMLY;
	}

	AiMoveRandomlyComponent::~AiMoveRandomlyComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AiMoveRandomlyComponent] Destructor ai move randomly component for game object: " + this->gameObjectPtr->getName());
	}

	bool AiMoveRandomlyComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		bool success = AiComponent::init(propertyElement);

		return success;
	}

	GameObjectCompPtr AiMoveRandomlyComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		AiMoveRandomlyCompPtr clonedCompPtr(boost::make_shared<AiMoveRandomlyComponent>());

		clonedCompPtr->setRotationSpeed(this->rotationSpeed->getReal());
		clonedCompPtr->setFlyMode(this->flyMode->getBool());
		clonedCompPtr->setStuckTime(this->stuckTime->getReal());
		clonedCompPtr->setAutoOrientation(this->autoOrientation->getBool());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->setActivated(this->activated->getBool());

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool AiMoveRandomlyComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AiMoveRandomlyComponent] Init ai move component for game object: " + this->gameObjectPtr->getName());

		return AiComponent::postInit();
	}

	bool AiMoveRandomlyComponent::connect(void)
	{
		return AiComponent::connect();
	}

	bool AiMoveRandomlyComponent::disconnect(void)
	{
		return AiComponent::disconnect();
	}

	void AiMoveRandomlyComponent::update(Ogre::Real dt, bool notSimulating)
	{
		// Moving behavior is updated in game object controller, because it has a map for moving behavior and the game object
		// because, a game object may have x ai components, which are working on one moving behavior and getting calculated priortized for the game object
	}

	void AiMoveRandomlyComponent::actualizeValue(Variant* attribute)
	{
		AiComponent::actualizeValue(attribute);
	}

	void AiMoveRandomlyComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		AiComponent::writeXML(propertiesXML, doc);
	}

	Ogre::String AiMoveRandomlyComponent::getClassName(void) const
	{
		return "AiMoveRandomlyComponent";
	}

	Ogre::String AiMoveRandomlyComponent::getParentClassName(void) const
	{
		return "AiComponent";
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	AiPathFollowComponent::AiPathFollowComponent()
		: AiComponent()
	{
		this->repeat = new Variant(AiPathFollowComponent::AttrRepeat(), true, this->attributes);
		this->directionChange = new Variant(AiPathFollowComponent::AttrDirectionChange(), false, this->attributes);
		this->invertDirection = new Variant(AiPathFollowComponent::AttrInvertDirection(), false, this->attributes);
		this->goalRadius = new Variant(AiPathFollowComponent::AttrGoalRadius(), 0.2f, this->attributes);
		
		this->waypointsCount = new Variant(AiPathFollowComponent::AttrWaypointsCount(), 0, this->attributes);
		// Since when waypoints count is changed, the whole properties must be refreshed, so that new field may come for way points
		this->waypointsCount->addUserData(GameObject::AttrActionNeedRefresh());
		this->goalRadius->setDescription("Sets the goal radius tolerance, when the agent has reached the target way point [0.2, x].");

		this->behaviorTypeId = KI::MovingBehavior::FOLLOW_PATH;
	}

	AiPathFollowComponent::~AiPathFollowComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AiPathFollowComponent] Destructor ai path follow component for game object: " + this->gameObjectPtr->getName());
	}

	bool AiPathFollowComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		bool success = AiComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WaypointsCount")
		{
			this->waypointsCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data", 1));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		if (this->waypoints.size() < this->waypointsCount->getUInt())
		{
			this->waypoints.resize(this->waypointsCount->getUInt());
		}
		for (size_t i = 0; i < this->waypoints.size(); i++)
		{
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Waypoint" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->waypoints[i])
				{
					this->waypoints[i] = new Variant(AiPathFollowComponent::AttrWaypoint() + Ogre::StringConverter::toString(i), XMLConverter::getAttribUnsignedLong(propertyElement, "data", 0), this->attributes);
				}
				else
				{
					this->waypoints[i]->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data", 0));
				}
				propertyElement = propertyElement->next_sibling("property");
				this->waypoints[i]->addUserData(GameObject::AttrActionSeparator());
			}
		}

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Repeat")
		{
			this->repeat->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DirectionChange")
		{
			this->directionChange->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "InvertDirection")
		{
			this->invertDirection->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "GoalRadius")
		{
			this->goalRadius->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		return success;
	}

	GameObjectCompPtr AiPathFollowComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		AiPathFollowCompPtr clonedCompPtr(boost::make_shared<AiPathFollowComponent>());

		clonedCompPtr->setRotationSpeed(this->rotationSpeed->getReal());
		clonedCompPtr->setFlyMode(this->flyMode->getBool());
		clonedCompPtr->setStuckTime(this->stuckTime->getReal());
		clonedCompPtr->setAutoOrientation(this->autoOrientation->getBool());

		clonedCompPtr->setWaypointsCount(this->waypointsCount->getUInt());

		for (unsigned int i = 0; i < static_cast<unsigned int>(this->waypoints.size()); i++)
		{
			clonedCompPtr->setWaypointId(i, this->waypoints[i]->getULong());
		}
		clonedCompPtr->setRepeat(this->repeat->getBool());
		clonedCompPtr->setDirectionChange(this->directionChange->getBool());
		clonedCompPtr->setInvertDirection(this->invertDirection->getBool());
		clonedCompPtr->setGoalRadius(this->goalRadius->getReal());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->setActivated(this->activated->getBool());

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool AiPathFollowComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AiPathFollowComponent] Init ai path follow component for game object: " + this->gameObjectPtr->getName());

		return AiComponent::postInit();
	}

	bool AiPathFollowComponent::connect(void)
	{
		bool success = AiComponent::connect();

		if (this->movingBehaviorPtr != nullptr && nullptr != this->movingBehaviorPtr->getPath())
		{
			this->movingBehaviorPtr->getPath()->clear();
			for (size_t i = 0; i < this->waypoints.size(); i++)
			{
				GameObjectPtr waypointGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->waypoints[i]->getULong());
				if (nullptr != waypointGameObjectPtr)
				{
					auto nodeCompPtr = NOWA::makeStrongPtr(waypointGameObjectPtr->getComponent<NodeComponent>());
					if (nullptr != nodeCompPtr)
					{
						// Add the way points
						this->movingBehaviorPtr->getPath()->addWayPoint(nodeCompPtr->getPosition());
					}
				}
			}

			this->movingBehaviorPtr->getPath()->setRepeat(this->repeat->getBool());
			this->movingBehaviorPtr->getPath()->setDirectionChange(this->directionChange->getBool());
			this->movingBehaviorPtr->getPath()->setInvertDirection(this->invertDirection->getBool());
			this->movingBehaviorPtr->setGoalRadius(this->goalRadius->getReal());
		}
		
		return success;
	}

	bool AiPathFollowComponent::disconnect(void)
	{
		return AiComponent::disconnect();
	}

	bool AiPathFollowComponent::onCloned(void)
	{
		// Search for the prior id of the cloned game object and set the new id and set the new id, if not found set better 0, else the game objects may be corrupt!
		for (size_t i = 0; i < this->waypoints.size(); i++)
		{
			GameObjectPtr waypointGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getClonedGameObjectFromPriorId(this->waypoints[i]->getULong());
			if (nullptr != waypointGameObjectPtr)
			{
				this->waypoints[i]->setValue(waypointGameObjectPtr->getId());
			}
			else
			{
				this->waypoints[i]->setValue(static_cast<unsigned long>(0));
			}
			// Since connect is called during cloning process, it does not make sense to process furher here, but only when simulation started!
		}
		return true;
	}

	void AiPathFollowComponent::update(Ogre::Real dt, bool notSimulating)
	{
		// Moving behavior is updated in game object controller, because it has a map for moving behavior and the game object
		// because, a game object may have x ai components, which are working on one moving behavior and getting calculated priortized for the game object
	}

	void AiPathFollowComponent::actualizeValue(Variant* attribute)
	{
		AiComponent::actualizeValue(attribute);

		if (AiPathFollowComponent::AttrWaypointsCount() == attribute->getName())
		{
			this->setWaypointsCount(attribute->getUInt());
		}
		else if (AiPathFollowComponent::AttrRepeat() == attribute->getName())
		{
			this->setRepeat(attribute->getBool());
		}
		else if (AiPathFollowComponent::AttrDirectionChange() == attribute->getName())
		{
			this->setDirectionChange(attribute->getBool());
		}
		else if (AiPathFollowComponent::AttrInvertDirection() == attribute->getName())
		{
			this->setInvertDirection(attribute->getBool());
		}
		else if (AiPathFollowComponent::AttrGoalRadius() == attribute->getName())
		{
			this->setGoalRadius(attribute->getReal());
		}
		else
		{
			for (size_t i = 0; i < this->waypoints.size(); i++)
			{
				if (AiPathFollowComponent::AttrWaypoint() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->waypoints[i]->setValue(attribute->getULong());
				}
			}
		}
	}

	void AiPathFollowComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		AiComponent::writeXML(propertiesXML, doc);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "WaypointsCount"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->waypointsCount->getUInt())));
		propertiesXML->append_node(propertyXML);

		for (size_t i = 0; i < this->waypoints.size(); i++)
		{
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "Waypoint" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->waypoints[i]->getULong())));
			propertiesXML->append_node(propertyXML);
		}

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Repeat"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->repeat->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DirectionChange"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->directionChange->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "InvertDirection"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->invertDirection->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "GoalRadius"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->goalRadius->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	void AiPathFollowComponent::setActivated(bool activated)
	{
		AiComponent::setActivated(activated);

		if (true == activated)
		{
			this->connect();
		}
	}

	Ogre::String AiPathFollowComponent::getClassName(void) const
	{
		return "AiPathFollowComponent";
	}

	Ogre::String AiPathFollowComponent::getParentClassName(void) const
	{
		return "AiComponent";
	}

	void AiPathFollowComponent::setWaypointsCount(unsigned int waypointsCount)
	{
		this->waypointsCount->setValue(waypointsCount);
		
		size_t oldSize = this->waypoints.size();

		if (waypointsCount > oldSize)
		{
			// Resize the waypoints array for count
			this->waypoints.resize(waypointsCount);

			for (size_t i = oldSize; i < this->waypoints.size(); i++)
			{
				this->waypoints[i] = new Variant(AiPathFollowComponent::AttrWaypoint() + Ogre::StringConverter::toString(i), static_cast<unsigned long>(0), this->attributes, true);
				this->waypoints[i]->addUserData(GameObject::AttrActionSeparator());
			}
		}
		else if (waypointsCount < oldSize)
		{
			this->eraseVariants(this->waypoints, waypointsCount);
		}
	}

	unsigned int AiPathFollowComponent::getWaypointsCount(void) const
	{
		return this->waypointsCount->getUInt();
	}

	void AiPathFollowComponent::setWaypointId(unsigned int index, unsigned long id)
	{
		if (index >= this->waypoints.size())
			return;
		this->waypoints[index]->setValue(id);
	}

	void AiPathFollowComponent::addWaypointId(unsigned long id)
	{
		unsigned int count = this->waypointsCount->getUInt();
		this->waypointsCount->setValue(count + 1);
		this->waypoints.resize(count + 1);
		this->waypoints[count] = new Variant(AiPathFollowComponent::AttrWaypoint() + Ogre::StringConverter::toString(count), id, this->attributes);
	}

	unsigned long AiPathFollowComponent::getWaypointId(unsigned int index)
	{
		if (index > this->waypoints.size())
			return 0;
		return this->waypoints[index]->getULong();
	}

	void AiPathFollowComponent::setRepeat(bool repeat)
	{
		this->repeat->setValue(repeat);
	}

	bool AiPathFollowComponent::getRepeat(void) const
	{
		return this->repeat->getBool();
	}

	void AiPathFollowComponent::setDirectionChange(bool directionChange)
	{
		this->directionChange->setValue(directionChange);
	}

	bool AiPathFollowComponent::getDirectionChange(void) const
	{
		return this->directionChange->getBool();
	}

	void AiPathFollowComponent::setInvertDirection(bool invertDirection)
	{
		this->invertDirection->setValue(invertDirection);
	}

	bool AiPathFollowComponent::getInvertDirection(void) const
	{
		return this->invertDirection->getBool();
	}

	void AiPathFollowComponent::setGoalRadius(Ogre::Real goalRadius)
	{
		if (goalRadius < 0.2f)
			goalRadius = 0.2f;
		this->goalRadius->setValue(goalRadius);
	}

	Ogre::Real AiPathFollowComponent::getGoalRadius(void) const
	{
		return this->goalRadius->getReal();
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	AiWanderComponent::AiWanderComponent()
		: AiComponent(),
		wanderJitter(new Variant(AiWanderComponent::AttrWanderJitter(), 1.0f, this->attributes)),
		wanderRadius(new Variant(AiWanderComponent::AttrWanderRadius(), 1.2f, this->attributes)),
		wanderDistance(new Variant(AiWanderComponent::AttrWanderDistance(), 20.0f, this->attributes))
	{
		this->behaviorTypeId = KI::MovingBehavior::WANDER;
	}

	AiWanderComponent::~AiWanderComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AiWanderComponent] Destructor ai wander component for game object: " + this->gameObjectPtr->getName());
	}

	bool AiWanderComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		bool success = AiComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WanderJitter")
		{
			this->wanderJitter->setValue(XMLConverter::getAttribReal(propertyElement, "data", 10.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WanderRadius")
		{
			this->wanderRadius->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.2f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WanderDistance")
		{
			this->wanderDistance->setValue(XMLConverter::getAttribReal(propertyElement, "data", 2.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		return success;
	}

	GameObjectCompPtr AiWanderComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		AiWanderCompPtr clonedCompPtr(boost::make_shared<AiWanderComponent>());

		clonedCompPtr->setRotationSpeed(this->rotationSpeed->getReal());
		clonedCompPtr->setFlyMode(this->flyMode->getBool());
		clonedCompPtr->setStuckTime(this->stuckTime->getReal());
		clonedCompPtr->setAutoOrientation(this->autoOrientation->getBool());

		clonedCompPtr->setWanderJitter(this->wanderJitter->getReal());
		clonedCompPtr->setWanderRadius(this->wanderRadius->getReal());
		clonedCompPtr->setWanderDistance(this->wanderDistance->getReal());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->setActivated(this->activated->getBool());

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool AiWanderComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AiWanderComponent] Init ai wander component for game object: " + this->gameObjectPtr->getName());

		return AiComponent::postInit();
	}

	bool AiWanderComponent::connect(void)
	{
		bool success = AiComponent::connect();
		if (nullptr != this->movingBehaviorPtr)
		{
			this->movingBehaviorPtr->setWanderJitter(this->wanderJitter->getReal());
			this->movingBehaviorPtr->setWanderRadius(this->wanderRadius->getReal());
			this->movingBehaviorPtr->setWanderDistance(this->wanderDistance->getReal());
		}

		return success;
	}

	bool AiWanderComponent::disconnect(void)
	{
		return AiComponent::disconnect();
	}

	void AiWanderComponent::update(Ogre::Real dt, bool notSimulating)
	{
		// Moving behavior is updated in game object controller, because it has a map for moving behavior and the game object
		// because, a game object may have x ai components, which are working on one moving behavior and getting calculated priortized for the game object
	}

	void AiWanderComponent::actualizeValue(Variant* attribute)
	{
		AiComponent::actualizeValue(attribute);

		if (AiWanderComponent::AttrWanderJitter() == attribute->getName())
		{
			this->setWanderJitter(attribute->getReal());
		}
		else if (AiWanderComponent::AttrWanderRadius() == attribute->getName())
		{
			this->setWanderRadius(attribute->getReal());
		}
		else if (AiWanderComponent::AttrWanderDistance() == attribute->getName())
		{
			this->setWanderDistance(attribute->getReal());
		}
	}

	void AiWanderComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		AiComponent::writeXML(propertiesXML, doc);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "WanderJitter"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wanderJitter->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "WanderRadius"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wanderRadius->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "WanderDistance"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wanderDistance->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String AiWanderComponent::getClassName(void) const
	{
		return "AiWanderComponent";
	}

	Ogre::String AiWanderComponent::getParentClassName(void) const
	{
		return "AiComponent";
	}

	void AiWanderComponent::setWanderJitter(Ogre::Real wanderJitter)
	{
		this->wanderJitter->setValue(wanderJitter);
	}

	Ogre::Real AiWanderComponent::getWanderJitter(void) const
	{
		return this->wanderJitter->getReal();
	}

	void AiWanderComponent::setWanderRadius(Ogre::Real wanderRadius)
	{
		this->wanderRadius->setValue(wanderRadius);
	}

	Ogre::Real AiWanderComponent::getWanderRadius(void) const
	{
		return this->wanderRadius->getReal();
	}

	void AiWanderComponent::setWanderDistance(Ogre::Real wanderDistance)
	{
		this->wanderDistance->setValue(wanderDistance);
	}

	Ogre::Real AiWanderComponent::getWanderDistance(void) const
	{
		return this->wanderDistance->getReal();
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	AiFlockingComponent::AiFlockingComponent()
		: AiComponent(),
		neighborDistance(new Variant(AiFlockingComponent::AttrNeighborDistance(), 0.0f, this->attributes)),
		cohesion(new Variant(AiFlockingComponent::AttrCohesion(), true, this->attributes)),
		separation(new Variant(AiFlockingComponent::AttrSeparation(), true, this->attributes)),
		alignment(new Variant(AiFlockingComponent::AttrAlignment(), true, this->attributes)),
		border(new Variant(AiFlockingComponent::AttrBorder(), false, this->attributes)),
		obstacle(new Variant(AiFlockingComponent::AttrObstacle(), false, this->attributes)),
		flee(new Variant(AiFlockingComponent::AttrFlee(), false, this->attributes)),
		seek(new Variant(AiFlockingComponent::AttrSeek(), false, this->attributes)),
		targetId(new Variant(AiFlockingComponent::AttrTargetId(), static_cast<unsigned long>(0), this->attributes, true))
	{
		this->flee->setDescription("Flee can only work with an valid target id, that is not part of the flocking group.");
		this->seek->setDescription("Seek can only work with an valid target id, that is not part of the flocking group.");
		this->neighborDistance->setDescription("The neighbor distance between each flocking neighbor to set. If set only neighbors within this distance form a flocking cloud. "
												"Please place the game objects close enough to each other, so that the flocking can work. A good neighbor distance is 2 meters. If set to 0, all neighbors form the flocking cloud.");
		this->behaviorTypeId = KI::MovingBehavior::FLOCKING;
		
		this->flee->addUserData(GameObject::AttrActionNeedRefresh());
		this->seek->addUserData(GameObject::AttrActionNeedRefresh());
	}

	AiFlockingComponent::~AiFlockingComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AiFlockingComponent] Destructor ai flocking component for game object: " + this->gameObjectPtr->getName());
	}

	bool AiFlockingComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		bool success = AiComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "NeighborDistance")
		{
			this->neighborDistance->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.6f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Cohesion")
		{
			this->cohesion->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Separation")
		{
			this->separation->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Alignment")
		{
			this->alignment->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Border")
		{
			this->border->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Obstacle")
		{
			this->obstacle->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Flee")
		{
			this->flee->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Seek")
		{
			this->seek->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetId")
		{
			this->targetId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data", 0));
			propertyElement = propertyElement->next_sibling("property");
		}
		return success;
	}

	GameObjectCompPtr AiFlockingComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		AiFlockingCompPtr clonedCompPtr(boost::make_shared<AiFlockingComponent>());

		clonedCompPtr->setRotationSpeed(this->rotationSpeed->getReal());
		clonedCompPtr->setFlyMode(this->flyMode->getBool());
		clonedCompPtr->setStuckTime(this->stuckTime->getReal());
		clonedCompPtr->setAutoOrientation(this->autoOrientation->getBool());

		clonedCompPtr->setNeighborDistance(this->neighborDistance->getReal());
		clonedCompPtr->setCohesionBehavior(this->cohesion->getBool());
		clonedCompPtr->setSeparationBehavior(this->separation->getBool());
		clonedCompPtr->setAlignmentBehavior(this->alignment->getBool());
		clonedCompPtr->setBorderBehavior(this->border->getBool());
		clonedCompPtr->setObstacleBehavior(this->obstacle->getBool());
		clonedCompPtr->setFleeBehavior(this->flee->getBool());
		clonedCompPtr->setSeekBehavior(this->seek->getBool());
		clonedCompPtr->setTargetId(this->targetId->getULong());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->setActivated(this->activated->getBool());

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool AiFlockingComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AiFlockingComponent] Init ai flocking component for game object: " + this->gameObjectPtr->getName());

		return AiComponent::postInit();
	}

	bool AiFlockingComponent::connect(void)
	{
		bool success = AiComponent::connect();
		
		if (nullptr != this->movingBehaviorPtr)
		{
			auto flockingAgentIds = AppStateManager::getSingletonPtr()->getGameObjectController()->getIdsFromCategory(this->gameObjectPtr->getCategory());
			for (size_t i = 0; i < flockingAgentIds.size(); i++)
			{
				// Remove this agent, just add all the others as neighbours
				if (this->gameObjectPtr->getId() == flockingAgentIds[i])
				{
					flockingAgentIds.erase(flockingAgentIds.begin() + i);
					break;
				}
			}

			this->movingBehaviorPtr->setFlockingAgents(flockingAgentIds);
			this->movingBehaviorPtr->setTargetAgentId(this->targetId->getULong());
			this->movingBehaviorPtr->setNeighborDistance(this->neighborDistance->getReal());

			if (true == this->cohesion->getBool())
				this->movingBehaviorPtr->addBehavior(MovingBehavior::FLOCKING_COHESION);
			if (true == this->separation->getBool())
				this->movingBehaviorPtr->addBehavior(MovingBehavior::FLOCKING_SEPARATION);
			if (true == this->alignment->getBool())
				this->movingBehaviorPtr->addBehavior(MovingBehavior::FLOCKING_ALIGNMENT);
			// if (true == this->border->getBool())
			// 	this->movingBehaviorPtr->setBehavior(MovingBehavior::FLOCKING_BORDER);
			if (true == this->obstacle->getBool())
				this->movingBehaviorPtr->addBehavior(MovingBehavior::FLOCKING_OBSTACLE_AVOIDANCE);
			if (true == this->flee->getBool())
				this->movingBehaviorPtr->addBehavior(MovingBehavior::FLOCKING_FLEE);
			if (true == this->seek->getBool())
				this->movingBehaviorPtr->addBehavior(MovingBehavior::FLOCKING_SEEK);
		}
		return success;
	}

	bool AiFlockingComponent::onCloned(void)
	{
		// Search for the prior id of the cloned game object and set the new id and set the new id, if not found set better 0, else the game objects may be corrupt!
		if (0 != this->targetId->getULong())
		{
			GameObjectPtr waypointGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getClonedGameObjectFromPriorId(this->targetId->getULong());
			if (nullptr != waypointGameObjectPtr)
			{
				this->targetId->setValue(waypointGameObjectPtr->getId());
			}
			// Since connect is called during cloning process, it does not make sense to process furher here, but only when simulation started!
		}
		return true;
	}

	bool AiFlockingComponent::disconnect(void)
	{
		return AiComponent::disconnect();
	}

	void AiFlockingComponent::update(Ogre::Real dt, bool notSimulating)
	{
		// Moving behavior is updated in game object controller, because it has a map for moving behavior and the game object
		// because, a game object may have x ai components, which are working on one moving behavior and getting calculated priortized for the game object
	}

	void AiFlockingComponent::actualizeValue(Variant* attribute)
	{
		AiComponent::actualizeValue(attribute);

		if (AiFlockingComponent::AttrNeighborDistance() == attribute->getName())
		{
			this->setNeighborDistance(attribute->getReal());
		}
		else if (AiFlockingComponent::AttrCohesion() == attribute->getName())
		{
			this->setCohesionBehavior(attribute->getBool());
		}
		else if (AiFlockingComponent::AttrSeparation() == attribute->getName())
		{
			this->setSeparationBehavior(attribute->getBool());
		}
		else if (AiFlockingComponent::AttrAlignment() == attribute->getName())
		{
			this->setAlignmentBehavior(attribute->getBool());
		}
		else if (AiFlockingComponent::AttrBorder() == attribute->getName())
		{
			this->setBorderBehavior(attribute->getBool());
		}
		else if (AiFlockingComponent::AttrObstacle() == attribute->getName())
		{
			this->setObstacleBehavior(attribute->getBool());
		}
		else if (AiFlockingComponent::AttrFlee() == attribute->getName())
		{
			this->setFleeBehavior(attribute->getBool());
		}
		else if (AiFlockingComponent::AttrSeek() == attribute->getName())
		{
			this->setSeekBehavior(attribute->getBool());
		}
		else if (AiFlockingComponent::AttrTargetId() == attribute->getName())
		{
			this->setTargetId(attribute->getULong());
		}
	}

	void AiFlockingComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		AiComponent::writeXML(propertiesXML, doc);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "NeighborDistance"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->neighborDistance->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Cohesion"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->cohesion->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Separation"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->separation->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Alignment"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->alignment->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Border"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->border->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Obstacle"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->obstacle->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Flee"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->flee->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Seek"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->seek->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TargetId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->targetId->getULong())));
		propertiesXML->append_node(propertyXML);
	}

	void AiFlockingComponent::setNeighborDistance(Ogre::Real neighborDistance)
	{
		this->neighborDistance->setValue(neighborDistance);
	}

	Ogre::Real AiFlockingComponent::getNeighborDistance(void) const
	{
		return this->neighborDistance->getReal();
	}

	void AiFlockingComponent::setCohesionBehavior(bool cohesion)
	{
		this->cohesion->setValue(cohesion);
	}

	bool AiFlockingComponent::getCohesionBehavior(void) const
	{
		return this->cohesion->getBool();
	}

	void AiFlockingComponent::setSeparationBehavior(bool separation)
	{
		this->separation->setValue(separation);
	}

	bool AiFlockingComponent::getSeparationBehavior(void) const
	{
		return this->separation->getBool();
	}

	void AiFlockingComponent::setAlignmentBehavior(bool alignment)
	{
		this->alignment->setValue(alignment);
	}

	bool AiFlockingComponent::getAlignmentBehavior(void) const
	{
		return this->alignment->getBool();
	}

	void AiFlockingComponent::setBorderBehavior(bool border)
	{
		this->border->setValue(border);
	}

	bool AiFlockingComponent::getBorderBehavior(void) const
	{
		return this->border->getBool();;
	}

	void AiFlockingComponent::setObstacleBehavior(bool obstacle)
	{
		this->obstacle->setValue(obstacle);
	}

	bool AiFlockingComponent::getObstacleBehavior(void) const
	{
		return this->obstacle->getBool();
	}

	void AiFlockingComponent::setFleeBehavior(bool flee)
	{
		this->flee->setValue(flee);
		// Both does not work
		if (true == flee)
		{
			this->seek->setValue(false);
		}
	}

	bool AiFlockingComponent::getFleeBehavior(void) const
	{
		return this->flee->getBool();
	}

	void AiFlockingComponent::setSeekBehavior(bool seek)
	{
		this->seek->setValue(seek);
		// Both does not work
		if (true == seek)
		{
			this->flee->setValue(false);
		}
	}

	bool AiFlockingComponent::getSeekBehavior(void) const
	{
		return this->seek->getBool();
	}

	void AiFlockingComponent::setTargetId(unsigned long targetId)
	{
		this->targetId->setValue(targetId);
	}

	unsigned long AiFlockingComponent::getTargetId(void) const
	{
		return this->targetId->getULong();
	}

	Ogre::String AiFlockingComponent::getClassName(void) const
	{
		return "AiFlockingComponent";
	}

	Ogre::String AiFlockingComponent::getParentClassName(void) const
	{
		return "AiComponent";
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	AiRecastPathNavigationComponent::AiRecastPathNavigationComponent()
		: AiComponent(),
		repeat(new Variant(AiRecastPathNavigationComponent::AttrRepeat(), false, this->attributes)),
		directionChange(new Variant(AiRecastPathNavigationComponent::AttrDirectionChange(), false, this->attributes)),
		invertDirection(new Variant(AiRecastPathNavigationComponent::AttrInvertDirection(), false, this->attributes)),
		goalRadius(new Variant(AiRecastPathNavigationComponent::AttrGoalRadius(), 0.2f, this->attributes)),
		targetId(new Variant(AiRecastPathNavigationComponent::AttrTargetId(), static_cast<unsigned long>(0), this->attributes, true)),
		actualizePathDelay(new Variant(AiRecastPathNavigationComponent::AttrActualizePathDelay(), 1.0f, this->attributes)),
		pathSlot(new Variant(AiRecastPathNavigationComponent::AttrPathSlot(), static_cast<int>(0), this->attributes)),
		targetSlot(new Variant(AiRecastPathNavigationComponent::AttrPathSlot(), static_cast<int>(0), this->attributes)),
		targetGameObject(nullptr),
		drawPath(false)
	{
		this->behaviorTypeId = KI::MovingBehavior::FOLLOW_PATH;
		this->actualizePathDelay->setDescription("Sets the interval in seconds in which a changed path will be re-created.");
		this->pathSlot->setDescription("Number identifying the target the path leads to.");
		this->targetSlot->setDescription("The index number for the slot in which the found path is to be stored.");
	}

	AiRecastPathNavigationComponent::~AiRecastPathNavigationComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AiRecastPathNavigationComponent] Destructor ai recast navigation component for game object: " + this->gameObjectPtr->getName());
		this->targetGameObject = nullptr;
	}

	bool AiRecastPathNavigationComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		bool success = AiComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Repeat")
		{
			this->repeat->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DirectionChange")
		{
			this->directionChange->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "InvertDirection")
		{
			this->invertDirection->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "GoalRadius")
		{
			this->goalRadius->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetId")
		{
			this->targetId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data", 0));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ActualizePathDelay")
		{
			this->actualizePathDelay->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PathSlot")
		{
			this->pathSlot->setValue(XMLConverter::getAttribInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetSlot")
		{
			this->targetSlot->setValue(XMLConverter::getAttribInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return success;
	}

	GameObjectCompPtr AiRecastPathNavigationComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		AiRecastPathNavigationCompPtr clonedCompPtr(boost::make_shared<AiRecastPathNavigationComponent>());

		clonedCompPtr->setRotationSpeed(this->rotationSpeed->getReal());
		clonedCompPtr->setFlyMode(this->flyMode->getBool());
		clonedCompPtr->setStuckTime(this->stuckTime->getReal());
		clonedCompPtr->setAutoOrientation(this->autoOrientation->getBool());

		clonedCompPtr->setRepeat(this->repeat->getBool());
		clonedCompPtr->setDirectionChange(this->directionChange->getBool());
		clonedCompPtr->setInvertDirection(this->invertDirection->getBool());
		clonedCompPtr->setGoalRadius(this->goalRadius->getReal());
		clonedCompPtr->setTargetId(this->targetId->getULong());
		clonedCompPtr->setActualizePathDelaySec(this->actualizePathDelay->getReal());
		clonedCompPtr->setPathSlot(this->pathSlot->getInt());
		clonedCompPtr->setTargetId(this->targetSlot->getInt());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->setActivated(this->activated->getBool());

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool AiRecastPathNavigationComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AiRecastPathNavigationComponent] Init ai path follow component for game object: " + this->gameObjectPtr->getName());

		return AiComponent::postInit();
	}

	bool AiRecastPathNavigationComponent::connect(void)
	{
		bool success = AiComponent::connect();

		if (this->movingBehaviorPtr != nullptr && nullptr != this->movingBehaviorPtr->getPath())
		{
			auto& physicsActiveCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsActiveComponent>());
			if (nullptr == physicsActiveCompPtr)
			{
				this->movingBehaviorPtr->reset();
				return true;
			}

			GameObjectPtr targetGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->targetId->getULong());
			if (nullptr == targetGameObjectPtr)
			{
				this->movingBehaviorPtr->reset();
				return true;
			}
			else
			{
				this->targetGameObject = targetGameObjectPtr.get();
			}

			this->movingBehaviorPtr->findPath();
		}
		return success;
	}

	bool AiRecastPathNavigationComponent::disconnect(void)
	{
		if (this->movingBehaviorPtr != nullptr && nullptr != this->movingBehaviorPtr->getPath())
		{
			this->movingBehaviorPtr->getPath()->clear();
		}
		this->targetGameObject = nullptr;

		return AiComponent::disconnect();
	}

	bool AiRecastPathNavigationComponent::onCloned(void)
	{
		// Search for the prior id of the cloned game object and set the new id and set the new id, if not found set better 0, else the game objects may be corrupt!
		if (0 != this->targetId->getULong())
		{
			GameObjectPtr targetGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getClonedGameObjectFromPriorId(this->targetId->getULong());
			if (nullptr != targetGameObjectPtr)
			{
				this->targetId->setValue(targetGameObjectPtr->getId());
			}
			// Since connect is called during cloning process, it does not make sense to process furher here, but only when simulation started!
		}
		if (this->movingBehaviorPtr != nullptr)
			this->movingBehaviorPtr->reset();
		return true;
	}

	void AiRecastPathNavigationComponent::update(Ogre::Real dt, bool notSimulating)
	{
	
	}

	void AiRecastPathNavigationComponent::actualizeValue(Variant* attribute)
	{
		AiComponent::actualizeValue(attribute);

		if (AiRecastPathNavigationComponent::AttrRepeat() == attribute->getName())
		{
			this->setRepeat(attribute->getBool());
		}
		else if (AiRecastPathNavigationComponent::AttrDirectionChange() == attribute->getName())
		{
			this->setDirectionChange(attribute->getBool());
		}
		else if (AiRecastPathNavigationComponent::AttrInvertDirection() == attribute->getName())
		{
			this->setInvertDirection(attribute->getBool());
		}
		else if (AiRecastPathNavigationComponent::AttrGoalRadius() == attribute->getName())
		{
			this->setGoalRadius(attribute->getReal());
		}
		else if (AiRecastPathNavigationComponent::AttrTargetId() == attribute->getName())
		{
			this->setTargetId(attribute->getULong());
		}
		else if (AiRecastPathNavigationComponent::AttrActualizePathDelay() == attribute->getName())
		{
			this->setActualizePathDelaySec(attribute->getReal());
		}
		else if (AiRecastPathNavigationComponent::AttrPathSlot() == attribute->getName())
		{
			this->setPathSlot(attribute->getInt());
		}
		else if (AiRecastPathNavigationComponent::AttrTargetSlot() == attribute->getName())
		{
			this->setPathTargetSlot(attribute->getInt());
		}
	}

	void AiRecastPathNavigationComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		AiComponent::writeXML(propertiesXML, doc);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Repeat"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->repeat->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DirectionChange"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->directionChange->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "InvertDirection"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->invertDirection->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "GoalRadius"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->goalRadius->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TargetId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->targetId->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ActualizePathDelay"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->actualizePathDelay->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "PathSlot"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->pathSlot->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TargetSlot"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->targetSlot->getUInt())));
		propertiesXML->append_node(propertyXML);
	}

	void AiRecastPathNavigationComponent::showDebugData()
	{
		GameObjectComponent::showDebugData();
		this->drawPath = this->bShowDebugData;
	}

	Ogre::String AiRecastPathNavigationComponent::getClassName(void) const
	{
		return "AiRecastPathNavigationComponent";
	}

	Ogre::String AiRecastPathNavigationComponent::getParentClassName(void) const
	{
		return "AiComponent";
	}

	void AiRecastPathNavigationComponent::setRepeat(bool repeat)
	{
		this->repeat->setValue(repeat);
	}

	bool AiRecastPathNavigationComponent::getRepeat(void) const
	{
		return this->repeat->getBool();
	}

	void AiRecastPathNavigationComponent::setDirectionChange(bool directionChange)
	{
		this->directionChange->setValue(directionChange);
	}

	bool AiRecastPathNavigationComponent::getDirectionChange(void) const
	{
		return this->directionChange->getBool();
	}

	void AiRecastPathNavigationComponent::setInvertDirection(bool invertDirection)
	{
		this->invertDirection->setValue(invertDirection);
	}

	bool AiRecastPathNavigationComponent::getInvertDirection(void) const
	{
		return this->invertDirection->getBool();
	}

	void AiRecastPathNavigationComponent::setGoalRadius(Ogre::Real goalRadius)
	{
		if (goalRadius < 0.2f)
			goalRadius = 0.2f;
		this->goalRadius->setValue(goalRadius);
		if (nullptr != this->movingBehaviorPtr)
		{
			this->movingBehaviorPtr->setGoalRadius(goalRadius);
		}
	}

	Ogre::Real AiRecastPathNavigationComponent::getGoalRadius(void) const
	{
		return this->goalRadius->getReal();
	}

	void AiRecastPathNavigationComponent::setTargetId(unsigned long targetId)
	{
		this->targetId->setValue(targetId);
		if (nullptr != this->movingBehaviorPtr)
		{
			this->movingBehaviorPtr->setTargetAgentId(targetId);
		}
	}

	unsigned long AiRecastPathNavigationComponent::getTargetId(void) const
	{
		return this->targetId->getULong();
	}

	void AiRecastPathNavigationComponent::setActualizePathDelaySec(Ogre::Real actualizePathDelay)
	{
		this->actualizePathDelay->setValue(actualizePathDelay);
		if (nullptr != this->movingBehaviorPtr)
		{
			this->movingBehaviorPtr->setActualizePathDelaySec(actualizePathDelay);
		}
	}

	Ogre::Real AiRecastPathNavigationComponent::getActualizePathDelaySec(void) const
	{
		return this->actualizePathDelay->getReal();
	}

	void AiRecastPathNavigationComponent::setPathSlot(int pathSlot)
	{
		this->pathSlot->setValue(pathSlot);
		if (nullptr != this->movingBehaviorPtr)
		{
			this->movingBehaviorPtr->setPathSlot(pathSlot);
		}
	}

	int AiRecastPathNavigationComponent::getPathSlot(void) const
	{
		return this->pathSlot->getUInt();
	}

	void AiRecastPathNavigationComponent::setPathTargetSlot(int targetSlot)
	{
		this->targetSlot->setValue(targetSlot);
		if (nullptr != this->movingBehaviorPtr)
		{
			this->movingBehaviorPtr->setPathTargetSlot(targetSlot);
		}
	}

	int AiRecastPathNavigationComponent::getPathTargetSlot(void) const
	{
		return this->targetSlot->getUInt();
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	AiObstacleAvoidanceComponent::AiObstacleAvoidanceComponent()
		: AiComponent(),
		obstacleCategories(new Variant(AiObstacleAvoidanceComponent::AttrObstacleCategories(), Ogre::String(""), this->attributes)),
		avoidanceRadius(new Variant(AiObstacleAvoidanceComponent::AttrAvoidanceRadius(), 2.0f, this->attributes))
	{
		this->behaviorTypeId = KI::MovingBehavior::OBSTACLE_AVOIDANCE;
	}

	AiObstacleAvoidanceComponent::~AiObstacleAvoidanceComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AiObstacleAvoidanceComponent] Destructor ai obstacle avoidance component for game object: " + this->gameObjectPtr->getName());
	}

	bool AiObstacleAvoidanceComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		bool success = AiComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AvoidanceDistance")
		{
			this->avoidanceRadius->setValue(XMLConverter::getAttribReal(propertyElement, "data", 2.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ObstacleCategories")
		{
			this->obstacleCategories->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		return success;
	}

	GameObjectCompPtr AiObstacleAvoidanceComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		AiObstacleAvoidanceCompPtr clonedCompPtr(boost::make_shared<AiObstacleAvoidanceComponent>());

		clonedCompPtr->setRotationSpeed(this->rotationSpeed->getReal());
		clonedCompPtr->setFlyMode(this->flyMode->getBool());
		clonedCompPtr->setObstacleCategories(this->obstacleCategories->getString());
		clonedCompPtr->setAvoidanceRadius(this->avoidanceRadius->getReal());
		clonedCompPtr->setStuckTime(this->stuckTime->getReal());
		clonedCompPtr->setAutoOrientation(this->autoOrientation->getBool());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->setActivated(this->activated->getBool());

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool AiObstacleAvoidanceComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AiObstacleAvoidanceComponent] Init ai obstacle avoidance component for game object: " + this->gameObjectPtr->getName());

		return AiComponent::postInit();
	}

	bool AiObstacleAvoidanceComponent::connect(void)
	{
		bool success = AiComponent::connect();

		if (nullptr != this->movingBehaviorPtr)
		{
			this->movingBehaviorPtr->setObstacleAvoidanceData(this->obstacleCategories->getString(), this->avoidanceRadius->getReal());
		}

		return success;
	}

	bool AiObstacleAvoidanceComponent::disconnect(void)
	{
		return AiComponent::disconnect();
	}

	void AiObstacleAvoidanceComponent::update(Ogre::Real dt, bool notSimulating)
	{
		// Moving behavior is updated in game object controller, because it has a map for moving behavior and the game object
		// because, a game object may have x ai components, which are working on one moving behavior and getting calculated priortized for the game object
	}

	void AiObstacleAvoidanceComponent::actualizeValue(Variant* attribute)
	{
		AiComponent::actualizeValue(attribute);

		if (AiObstacleAvoidanceComponent::AttrAvoidanceRadius() == attribute->getName())
		{
			this->setAvoidanceRadius(attribute->getReal());
		}
		else if (AiObstacleAvoidanceComponent::AttrObstacleCategories() == attribute->getName())
		{
			this->setObstacleCategories(attribute->getString());
		}
	}

	void AiObstacleAvoidanceComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		AiComponent::writeXML(propertiesXML, doc);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AvoidanceRadius"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->avoidanceRadius->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ObstacleCategories"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->obstacleCategories->getString())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String AiObstacleAvoidanceComponent::getClassName(void) const
	{
		return "AiObstacleAvoidanceComponent";
	}

	Ogre::String AiObstacleAvoidanceComponent::getParentClassName(void) const
	{
		return "AiComponent";
	}

	void AiObstacleAvoidanceComponent::setAvoidanceRadius(Ogre::Real avoidanceRadius)
	{
		this->avoidanceRadius->setValue(avoidanceRadius);
	}

	Ogre::Real AiObstacleAvoidanceComponent::getAvoidanceRadius(void) const
	{
		return this->avoidanceRadius->getReal();
	}

	void AiObstacleAvoidanceComponent::setObstacleCategories(const Ogre::String& obstacleCategories)
	{
		this->obstacleCategories->setValue(obstacleCategories);
	}

	Ogre::String AiObstacleAvoidanceComponent::getObstacleCategories(void) const
	{
		return this->obstacleCategories->getString();
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	AiHideComponent::AiHideComponent()
		: AiComponent(),
		targetId(new Variant(AiHideComponent::AttrTargetId(), static_cast<unsigned long>(0), this->attributes, true)),
		obstacleCategories(new Variant(AiHideComponent::AttrObstacleCategories(), Ogre::String(""), this->attributes)),
		obstacleRangeRadius(new Variant(AiHideComponent::AttrObstacleRangeRadius(), 30.0f, this->attributes))
	{
		this->behaviorTypeId = KI::MovingBehavior::HIDE;
	}

	AiHideComponent::~AiHideComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AiHideComponent] Destructor ai hide component for game object: " + this->gameObjectPtr->getName());
	}

	bool AiHideComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		bool success = AiComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetId")
		{
			this->targetId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ObstacleRangeRadius")
		{
			this->obstacleRangeRadius->setValue(XMLConverter::getAttribReal(propertyElement, "data", 30.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ObstacleCategories")
		{
			this->obstacleCategories->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		return success;
	}

	GameObjectCompPtr AiHideComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		AiHideCompPtr clonedCompPtr(boost::make_shared<AiHideComponent>());

		clonedCompPtr->setRotationSpeed(this->rotationSpeed->getReal());
		clonedCompPtr->setFlyMode(this->flyMode->getBool());
		clonedCompPtr->setTargetId(this->targetId->getULong());
		clonedCompPtr->setObstacleRangeRadius(this->obstacleRangeRadius->getReal());
		clonedCompPtr->setObstacleCategories(this->obstacleCategories->getString());
		clonedCompPtr->setStuckTime(this->stuckTime->getReal());
		clonedCompPtr->setAutoOrientation(this->autoOrientation->getBool());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->setActivated(this->activated->getBool());

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool AiHideComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AiObstacleAvoidanceComponent] Init ai hide component for game object: " + this->gameObjectPtr->getName());

		return AiComponent::postInit();
	}

	bool AiHideComponent::connect(void)
	{
		bool success = AiComponent::connect();

		if (nullptr != this->movingBehaviorPtr)
		{
			this->movingBehaviorPtr->setObstacleHideData(this->obstacleCategories->getString(), this->obstacleRangeRadius->getReal());
			this->movingBehaviorPtr->setTargetAgentId(this->targetId->getULong());
		}

		return success;
	}

	bool AiHideComponent::disconnect(void)
	{
		return AiComponent::disconnect();
	}

	bool AiHideComponent::onCloned(void)
	{
		// Search for the prior id of the cloned game object and set the new id and set the new id, if not found set better 0, else the game objects may be corrupt!
		if (0 != this->targetId->getULong())
		{
			GameObjectPtr targetGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getClonedGameObjectFromPriorId(this->targetId->getULong());
			if (nullptr != targetGameObjectPtr)
			{
				this->targetId->setValue(targetGameObjectPtr->getId());
			}
			// Since connect is called during cloning process, it does not make sense to process further here, but only when simulation started!
		}
		return true;
	}

	void AiHideComponent::update(Ogre::Real dt, bool notSimulating)
	{
		// Moving behavior is updated in game object controller, because it has a map for moving behavior and the game object
		// because, a game object may have x ai components, which are working on one moving behavior and getting calculated priortized for the game object
	}

	void AiHideComponent::actualizeValue(Variant* attribute)
	{
		AiComponent::actualizeValue(attribute);

		if (AiHideComponent::AttrTargetId() == attribute->getName())
		{
			this->setTargetId(attribute->getULong());
		}
		else if (AiHideComponent::AttrObstacleRangeRadius() == attribute->getName())
		{
			this->setObstacleRangeRadius(attribute->getReal());
		}
		else if (AiHideComponent::AttrObstacleCategories() == attribute->getName())
		{
			this->setObstacleCategories(attribute->getString());
		}
	}

	void AiHideComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		AiComponent::writeXML(propertiesXML, doc);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TargetId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->targetId->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ObstacleRangeRadius"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->obstacleRangeRadius->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ObstacleCategories"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->obstacleCategories->getString())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String AiHideComponent::getClassName(void) const
	{
		return "AiHideComponent";
	}

	Ogre::String AiHideComponent::getParentClassName(void) const
	{
		return "AiComponent";
	}

	void AiHideComponent::setTargetId(unsigned long targetId)
	{
		this->targetId->setValue(targetId);
	}

	unsigned long AiHideComponent::getTargetId(void) const
	{
		return this->targetId->getULong();
	}

	void AiHideComponent::setObstacleRangeRadius(Ogre::Real obstacleRangeRadius)
	{
		this->obstacleRangeRadius->setValue(obstacleRangeRadius);
	}

	Ogre::Real AiHideComponent::getObstacleRangeRadius(void) const
	{
		return this->obstacleRangeRadius->getReal();
	}

	void AiHideComponent::setObstacleCategories(const Ogre::String& obstacleCategories)
	{
		this->obstacleCategories->setValue(obstacleCategories);
	}

	Ogre::String AiHideComponent::getObstacleCategories(void) const
	{
		return this->obstacleCategories->getString();
	}
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	AiMoveComponent2D::AiMoveComponent2D()
		: AiComponent(),
		behaviorType(new Variant(AiMoveComponent2D::AttrBehaviorType(), { "Seek2D", "Flee2D", "Arrive2D" }, this->attributes)),
		targetId(new Variant(AiMoveComponent2D::AttrTargetId(), static_cast<unsigned long>(0), this->attributes, true))
	{
		this->behaviorTypeId = KI::MovingBehavior::SEEK_2D;
	}

	AiMoveComponent2D::~AiMoveComponent2D()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AiMoveComponent2D] Destructor ai move component 2D for game object: " + this->gameObjectPtr->getName());
	}

	bool AiMoveComponent2D::init(rapidxml::xml_node<>*& propertyElement)
	{
		bool success = AiComponent::init(propertyElement);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BehaviorType")
		{
			this->behaviorType->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "Move"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetId")
		{
			this->targetId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		return success;
	}

	GameObjectCompPtr AiMoveComponent2D::clone(GameObjectPtr clonedGameObjectPtr)
	{
		AiMoveComp2DPtr clonedCompPtr(boost::make_shared<AiMoveComponent2D>());

		clonedCompPtr->setRotationSpeed(this->rotationSpeed->getReal());
		clonedCompPtr->setFlyMode(this->flyMode->getBool());
		clonedCompPtr->setStuckTime(this->stuckTime->getReal());
		clonedCompPtr->setAutoOrientation(this->autoOrientation->getBool());

		clonedCompPtr->setBehaviorType(this->behaviorType->getListSelectedValue());
		clonedCompPtr->setTargetId(this->targetId->getULong());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->setActivated(this->activated->getBool());

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool AiMoveComponent2D::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AiMoveComponent2D] Init ai move component 2D for game object: " + this->gameObjectPtr->getName());

		return AiComponent::postInit();
	}

	bool AiMoveComponent2D::connect(void)
	{
		if (nullptr != this->movingBehaviorPtr)
		{
			// Set the chosen behavior type, before its added in AiComponent2D::connect
			this->behaviorTypeId = this->movingBehaviorPtr->mapBehavior(this->behaviorType->getListSelectedValue());
		}
		
		bool success = AiComponent::connect();

		if (nullptr != this->movingBehaviorPtr)
		{
			this->movingBehaviorPtr->setTargetAgentId(this->targetId->getULong());
		}
		return success;
	}

	bool AiMoveComponent2D::disconnect(void)
	{
		return AiComponent::disconnect();
	}

	bool AiMoveComponent2D::onCloned(void)
	{
		// Search for the prior id of the cloned game object and set the new id and set the new id, if not found set better 0, else the game objects may be corrupt!
		if (0 != this->targetId->getULong())
		{
			GameObjectPtr targetGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getClonedGameObjectFromPriorId(this->targetId->getULong());
			if (nullptr != targetGameObjectPtr)
			{
				this->targetId->setValue(targetGameObjectPtr->getId());
			}
			// Since connect is called during cloning process, it does not make sense to process further here, but only when simulation started!
		}
		return true;
	}

	void AiMoveComponent2D::update(Ogre::Real dt, bool notSimulating)
	{
		// Moving behavior is updated in game object controller, because it has a map for moving behavior and the game object
		// because, a game object may have x ai components, which are working on one moving behavior and getting calculated priortized for the game object
	}

	void AiMoveComponent2D::actualizeValue(Variant* attribute)
	{
		AiComponent::actualizeValue(attribute);
		
		if (AiMoveComponent2D::AttrBehaviorType() == attribute->getName())
		{
			this->setBehaviorType(attribute->getListSelectedValue());
		}
		else if (AiMoveComponent2D::AttrTargetId() == attribute->getName())
		{
			this->setTargetId(attribute->getULong());
		}
	}

	void AiMoveComponent2D::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		AiComponent::writeXML(propertiesXML, doc);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BehaviorType"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->behaviorType->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TargetId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->targetId->getULong())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String AiMoveComponent2D::getClassName(void) const
	{
		return "AiMoveComponent2D";
	}

	Ogre::String AiMoveComponent2D::getParentClassName(void) const
	{
		return "AiComponent";
	}
	
	void AiMoveComponent2D::setBehaviorType(const Ogre::String& behaviorType)
	{
		this->behaviorType->setListSelectedValue(behaviorType);
	}
	
	Ogre::String AiMoveComponent2D::getBehaviorType(void) const
	{
		return this->behaviorType->getListSelectedValue();
	}
	
	void AiMoveComponent2D::setTargetId(unsigned long targetId)
	{
		this->targetId->setValue(targetId);
	}

	unsigned long AiMoveComponent2D::getTargetId(void) const
	{
		return this->targetId->getULong();
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	AiPathFollowComponent2D::AiPathFollowComponent2D()
		: AiComponent()
	{
		this->repeat = new Variant(AiPathFollowComponent2D::AttrRepeat(), true, this->attributes);
		this->directionChange = new Variant(AiPathFollowComponent2D::AttrDirectionChange(), false, this->attributes);
		this->invertDirection = new Variant(AiPathFollowComponent2D::AttrInvertDirection(), false, this->attributes);
		this->goalRadius = new Variant(AiPathFollowComponent2D::AttrGoalRadius(), 0.2f, this->attributes);
		
		this->waypointsCount = new Variant(AiPathFollowComponent2D::AttrWaypointsCount(), 0, this->attributes);
		// Since when waypoints count is changed, the whole properties must be refreshed, so that new field may come for way points
		this->waypointsCount->addUserData(GameObject::AttrActionNeedRefresh());
		this->goalRadius->setDescription("Sets the goal radius tolerance, when the agent has reached the target way point [0.2, x].");

		this->behaviorTypeId = KI::MovingBehavior::FOLLOW_PATH_2D;
	}

	AiPathFollowComponent2D::~AiPathFollowComponent2D()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AiPathFollowComponent2D] Destructor ai path follow component 2D for game object: " + this->gameObjectPtr->getName());
	}

	bool AiPathFollowComponent2D::init(rapidxml::xml_node<>*& propertyElement)
	{
		bool success = AiComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WaypointsCount")
		{
			this->waypointsCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data", 1));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		this->waypoints.resize(this->waypointsCount->getUInt());
		for (size_t i = 0; i < this->waypoints.size(); i++)
		{
			if (nullptr == this->waypoints[i])
			{
				this->waypoints[i] = new Variant(AiPathFollowComponent2D::AttrWaypoint() + Ogre::StringConverter::toString(i), XMLConverter::getAttribUnsignedLong(propertyElement, "data", 0), this->attributes);
			}
			else
			{
				this->waypoints[i]->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data", 0));
			}
			propertyElement = propertyElement->next_sibling("property");
			this->waypoints[i]->addUserData(GameObject::AttrActionSeparator());
		}

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Repeat")
		{
			this->repeat->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DirectionChange")
		{
			this->directionChange->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "InvertDirection")
		{
			this->invertDirection->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "GoalRadius")
		{
			this->goalRadius->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		return success;
	}

	GameObjectCompPtr AiPathFollowComponent2D::clone(GameObjectPtr clonedGameObjectPtr)
	{
		AiPathFollowComp2DPtr clonedCompPtr(boost::make_shared<AiPathFollowComponent2D>());

		clonedCompPtr->setRotationSpeed(this->rotationSpeed->getReal());
		clonedCompPtr->setFlyMode(this->flyMode->getBool());
		clonedCompPtr->setStuckTime(this->stuckTime->getReal());
		clonedCompPtr->setAutoOrientation(this->autoOrientation->getBool());

		clonedCompPtr->setWaypointsCount(this->waypointsCount->getUInt());

		for (unsigned int i = 0; i < static_cast<unsigned int>(this->waypoints.size()); i++)
		{
			clonedCompPtr->setWaypointId(i, this->waypoints[i]->getULong());
		}
		clonedCompPtr->setRepeat(this->repeat->getBool());
		clonedCompPtr->setDirectionChange(this->directionChange->getBool());
		clonedCompPtr->setInvertDirection(this->invertDirection->getBool());
		clonedCompPtr->setGoalRadius(this->goalRadius->getReal());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->setActivated(this->activated->getBool());

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool AiPathFollowComponent2D::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AiPathFollowComponent2D] Init ai path follow component for game object: " + this->gameObjectPtr->getName());

		return AiComponent::postInit();
	}

	bool AiPathFollowComponent2D::connect(void)
	{
		bool success = AiComponent::connect();

		if (this->movingBehaviorPtr != nullptr && nullptr != this->movingBehaviorPtr->getPath())
		{
			this->movingBehaviorPtr->getPath()->clear();
			for (size_t i = 0; i < this->waypoints.size(); i++)
			{
				GameObjectPtr waypointGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->waypoints[i]->getULong());
				if (nullptr != waypointGameObjectPtr)
				{
					auto nodeCompPtr = NOWA::makeStrongPtr(waypointGameObjectPtr->getComponent<NodeComponent>());
					if (nullptr != nodeCompPtr)
					{
						// Add the way points
						this->movingBehaviorPtr->getPath()->addWayPoint(nodeCompPtr->getPosition());
					}
				}
			}

			this->movingBehaviorPtr->getPath()->setRepeat(this->repeat->getBool());
			this->movingBehaviorPtr->getPath()->setDirectionChange(this->directionChange->getBool());
			this->movingBehaviorPtr->getPath()->setInvertDirection(this->invertDirection->getBool());
			this->movingBehaviorPtr->setGoalRadius(this->goalRadius->getReal());
		}
		
		return success;
	}

	bool AiPathFollowComponent2D::disconnect(void)
	{
		if (this->movingBehaviorPtr != nullptr && nullptr != this->movingBehaviorPtr->getPath())
		{
			this->movingBehaviorPtr->getPath()->clear();
		}
		
		return AiComponent::disconnect();
	}

	bool AiPathFollowComponent2D::onCloned(void)
	{
		// Search for the prior id of the cloned game object and set the new id and set the new id, if not found set better 0, else the game objects may be corrupt!
		for (size_t i = 0; i < this->waypoints.size(); i++)
		{
			GameObjectPtr waypointGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getClonedGameObjectFromPriorId(this->waypoints[i]->getULong());
			if (nullptr != waypointGameObjectPtr)
			{
				this->waypoints[i]->setValue(waypointGameObjectPtr->getId());
			}
			else
			{
				this->waypoints[i]->setValue(static_cast<unsigned long>(0));
			}
			// Since connect is called during cloning process, it does not make sense to process furher here, but only when simulation started!
		}
		return true;
	}

	void AiPathFollowComponent2D::update(Ogre::Real dt, bool notSimulating)
	{
		// Moving behavior is updated in game object controller, because it has a map for moving behavior and the game object
		// because, a game object may have x ai components, which are working on one moving behavior and getting calculated priortized for the game object
	}

	void AiPathFollowComponent2D::actualizeValue(Variant* attribute)
	{
		AiComponent::actualizeValue(attribute);

		if (AiPathFollowComponent2D::AttrWaypointsCount() == attribute->getName())
		{
			this->setWaypointsCount(attribute->getUInt());
		}
		else if (AiPathFollowComponent2D::AttrRepeat() == attribute->getName())
		{
			this->setRepeat(attribute->getBool());
		}
		else if (AiPathFollowComponent2D::AttrDirectionChange() == attribute->getName())
		{
			this->setDirectionChange(attribute->getBool());
		}
		else if (AiPathFollowComponent2D::AttrInvertDirection() == attribute->getName())
		{
			this->setInvertDirection(attribute->getBool());
		}
		else if (AiPathFollowComponent2D::AttrGoalRadius() == attribute->getName())
		{
			this->setGoalRadius(attribute->getReal());
		}
		else
		{
			for (size_t i = 0; i < this->waypoints.size(); i++)
			{
				if (AiPathFollowComponent2D::AttrWaypoint() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->waypoints[i]->setValue(attribute->getULong());
				}
			}
		}
	}

	void AiPathFollowComponent2D::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		AiComponent::writeXML(propertiesXML, doc);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "WaypointsCount"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->waypointsCount->getUInt())));
		propertiesXML->append_node(propertyXML);

		for (size_t i = 0; i < this->waypoints.size(); i++)
		{
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "Waypoint" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->waypoints[i]->getULong())));
			propertiesXML->append_node(propertyXML);
		}

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Repeat"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->repeat->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DirectionChange"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->directionChange->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "InvertDirection"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->invertDirection->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "GoalRadius"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->goalRadius->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	void AiPathFollowComponent2D::setActivated(bool activated)
	{
		AiComponent::setActivated(activated);

		if (true == activated)
		{
			this->connect();
		}
	}

	Ogre::String AiPathFollowComponent2D::getClassName(void) const
	{
		return "AiPathFollowComponent2D";
	}

	Ogre::String AiPathFollowComponent2D::getParentClassName(void) const
	{
		return "AiComponent";
	}

	void AiPathFollowComponent2D::setWaypointsCount(unsigned int waypointsCount)
	{
		this->waypointsCount->setValue(waypointsCount);
		
		size_t oldSize = this->waypoints.size();

		if (waypointsCount > oldSize)
		{
			// Resize the waypoints array for count
			this->waypoints.resize(waypointsCount);

			for (size_t i = oldSize; i < this->waypoints.size(); i++)
			{
				this->waypoints[i] = new Variant(AiPathFollowComponent2D::AttrWaypoint() + Ogre::StringConverter::toString(i), static_cast<unsigned long>(0), this->attributes, true);
			}
		}
		else if (waypointsCount < oldSize)
		{
			this->eraseVariants(this->waypoints, waypointsCount);
		}
	}

	unsigned int AiPathFollowComponent2D::getWaypointsCount(void) const
	{
		return this->waypointsCount->getUInt();
	}

	void AiPathFollowComponent2D::setWaypointId(unsigned int index, unsigned long id)
	{
		if (index > this->waypoints.size())
			return;
		this->waypoints[index]->setValue(id);
	}

	void AiPathFollowComponent2D::addWaypointId(unsigned long id)
	{
		unsigned int count = this->waypointsCount->getUInt();
		this->waypointsCount->setValue(count + 1);
		this->waypoints.resize(count);
		this->waypoints[count - 1]->setValue(id);
	}

	unsigned long AiPathFollowComponent2D::getWaypointId(unsigned int index)
	{
		if (index > this->waypoints.size())
			return 0;
		return this->waypoints[index]->getULong();
	}

	void AiPathFollowComponent2D::setRepeat(bool repeat)
	{
		this->repeat->setValue(repeat);
	}

	bool AiPathFollowComponent2D::getRepeat(void) const
	{
		return this->repeat->getBool();
	}

	void AiPathFollowComponent2D::setDirectionChange(bool directionChange)
	{
		this->directionChange->setValue(directionChange);
	}

	bool AiPathFollowComponent2D::getDirectionChange(void) const
	{
		return this->directionChange->getBool();
	}

	void AiPathFollowComponent2D::setInvertDirection(bool invertDirection)
	{
		this->invertDirection->setValue(invertDirection);
	}

	bool AiPathFollowComponent2D::getInvertDirection(void) const
	{
		return this->invertDirection->getBool();
	}

	void AiPathFollowComponent2D::setGoalRadius(Ogre::Real goalRadius)
	{
		if (goalRadius < 0.2f)
			goalRadius = 0.2f;
		this->goalRadius->setValue(goalRadius);
	}

	Ogre::Real AiPathFollowComponent2D::getGoalRadius(void) const
	{
		return this->goalRadius->getReal();
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	AiWanderComponent2D::AiWanderComponent2D()
		: AiComponent(),
		wanderJitter(new Variant(AiWanderComponent2D::AttrWanderJitter(), 1.0f, this->attributes)),
		wanderRadius(new Variant(AiWanderComponent2D::AttrWanderRadius(), 1.2f, this->attributes)),
		wanderDistance(new Variant(AiWanderComponent2D::AttrWanderDistance(), 20.0f, this->attributes))
	{
		this->behaviorTypeId = KI::MovingBehavior::WANDER_2D;
	}

	AiWanderComponent2D::~AiWanderComponent2D()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AiWanderComponent2D] Destructor ai wander component for game object: " + this->gameObjectPtr->getName());
	}

	bool AiWanderComponent2D::init(rapidxml::xml_node<>*& propertyElement)
	{
		bool success = AiComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WanderJitter")
		{
			this->wanderJitter->setValue(XMLConverter::getAttribReal(propertyElement, "data", 10.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WanderRadius")
		{
			this->wanderRadius->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.2f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WanderDistance")
		{
			this->wanderDistance->setValue(XMLConverter::getAttribReal(propertyElement, "data", 2.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		return success;
	}

	GameObjectCompPtr AiWanderComponent2D::clone(GameObjectPtr clonedGameObjectPtr)
	{
		AiWanderComp2DPtr clonedCompPtr(boost::make_shared<AiWanderComponent2D>());

		clonedCompPtr->setRotationSpeed(this->rotationSpeed->getReal());
		clonedCompPtr->setFlyMode(this->flyMode->getBool());
		clonedCompPtr->setStuckTime(this->stuckTime->getReal());
		clonedCompPtr->setAutoOrientation(this->autoOrientation->getBool());

		clonedCompPtr->setWanderJitter(this->wanderJitter->getReal());
		clonedCompPtr->setWanderRadius(this->wanderRadius->getReal());
		clonedCompPtr->setWanderDistance(this->wanderDistance->getReal());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->setActivated(this->activated->getBool());

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool AiWanderComponent2D::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AiWanderComponent2D] Init ai wander component for game object: " + this->gameObjectPtr->getName());

		return AiComponent::postInit();
	}

	bool AiWanderComponent2D::connect(void)
	{
		bool success = AiComponent::connect();
		if (nullptr != this->movingBehaviorPtr)
		{
			this->movingBehaviorPtr->setWanderJitter(this->wanderJitter->getReal());
			this->movingBehaviorPtr->setWanderRadius(this->wanderRadius->getReal());
			this->movingBehaviorPtr->setWanderDistance(this->wanderDistance->getReal());
		}

		return success;
	}

	bool AiWanderComponent2D::disconnect(void)
	{
		return AiComponent::disconnect();
	}

	void AiWanderComponent2D::update(Ogre::Real dt, bool notSimulating)
	{
		// Moving behavior is updated in game object controller, because it has a map for moving behavior and the game object
		// because, a game object may have x ai components, which are working on one moving behavior and getting calculated priortized for the game object
	}

	void AiWanderComponent2D::actualizeValue(Variant* attribute)
	{
		AiComponent::actualizeValue(attribute);

		if (AiWanderComponent2D::AttrWanderJitter() == attribute->getName())
		{
			this->setWanderJitter(attribute->getReal());
		}
		else if (AiWanderComponent2D::AttrWanderRadius() == attribute->getName())
		{
			this->setWanderRadius(attribute->getReal());
		}
		else if (AiWanderComponent2D::AttrWanderDistance() == attribute->getName())
		{
			this->setWanderDistance(attribute->getReal());
		}
	}

	void AiWanderComponent2D::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		AiComponent::writeXML(propertiesXML, doc);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "WanderJitter"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wanderJitter->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "WanderRadius"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wanderRadius->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "WanderDistance"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wanderDistance->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String AiWanderComponent2D::getClassName(void) const
	{
		return "AiWanderComponent2D";
	}

	Ogre::String AiWanderComponent2D::getParentClassName(void) const
	{
		return "AiComponent";
	}

	void AiWanderComponent2D::setWanderJitter(Ogre::Real wanderJitter)
	{
		this->wanderJitter->setValue(wanderJitter);
	}

	Ogre::Real AiWanderComponent2D::getWanderJitter(void) const
	{
		return this->wanderJitter->getReal();
	}

	void AiWanderComponent2D::setWanderRadius(Ogre::Real wanderRadius)
	{
		this->wanderRadius->setValue(wanderRadius);
	}

	Ogre::Real AiWanderComponent2D::getWanderRadius(void) const
	{
		return this->wanderRadius->getReal();
	}

	void AiWanderComponent2D::setWanderDistance(Ogre::Real wanderDistance)
	{
		this->wanderDistance->setValue(wanderDistance);
	}

	Ogre::Real AiWanderComponent2D::getWanderDistance(void) const
	{
		return this->wanderDistance->getReal();
	}

}; // namespace end
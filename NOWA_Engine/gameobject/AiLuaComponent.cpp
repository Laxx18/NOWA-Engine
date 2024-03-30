#include "NOWAPrecompiled.h"
#include "AiLuaComponent.h"
#include "AiComponents.h"
#include "utilities/XMLConverter.h"
#include "LuaScriptComponent.h"
#include "modules/LuaScriptApi.h"
#include "main/AppStateManager.h"
#include "PhysicsActiveComponent.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	AiLuaComponent::AiLuaComponent()
		: GameObjectComponent(),
		activated(new Variant(AiLuaComponent::AttrActivated(), true, this->attributes)),
		rotationSpeed(new Variant(AiLuaComponent::AttrRotationSpeed(), 10.0f, this->attributes)),
		flyMode(new Variant(AiLuaComponent::AttrFlyMode(), false, this->attributes)),
		startStateName(new Variant(AiLuaComponent::AttrStartStateName(), "MyState", this->attributes)),
		luaStateMachine(nullptr),
		pathGoalObserver(nullptr),
		agentStuckObserver(nullptr),
		ready(false),
		componentCloned(false)
	{

	}

	AiLuaComponent::~AiLuaComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AiLuaComponent] Destructor ai lua component for game object: " + this->gameObjectPtr->getName());
		AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &AiLuaComponent::handleLuaScriptConnected), EventDataLuaScriptConnected::getStaticEventType());

		if (this->luaStateMachine)
		{
			delete this->luaStateMachine;
			this->luaStateMachine = nullptr;
		}
		this->targetGameObject = nullptr;
	}

	bool AiLuaComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RotationSpeed")
		{
			this->rotationSpeed->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.1f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FlyMode")
		{
			this->flyMode->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "StartStateName")
		{
			this->startStateName->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &AiLuaComponent::handleLuaScriptConnected), EventDataLuaScriptConnected::getStaticEventType());
	
		return true;
	}

	GameObjectCompPtr AiLuaComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		AiLuaCompPtr clonedCompPtr(boost::make_shared<AiLuaComponent>());

		clonedCompPtr->componentCloned = true;

		clonedCompPtr->setRotationSpeed(this->rotationSpeed->getReal());
		clonedCompPtr->setFlyMode(this->flyMode->getBool());
		clonedCompPtr->setStartStateName(this->startStateName->getString());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->postInit();
		clonedCompPtr->setActivated(this->activated->getBool());

		clonedCompPtr->componentCloned = false;

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool AiLuaComponent::postInit(void)
	{
		// Do not set things a second time after cloning
		if (nullptr != this->luaStateMachine)
		{
			return true;
		}

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AiLuaComponent] Init ai lua component for game object: " + this->gameObjectPtr->getName());

		this->luaStateMachine = new LuaStateMachine<GameObject>(this->gameObjectPtr.get());

		this->movingBehaviorPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->addMovingBehavior(this->gameObjectPtr->getId());
		if (nullptr == this->movingBehaviorPtr)
			return false;

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		return true;
	}

	bool AiLuaComponent::connect(void)
	{
		this->movingBehaviorPtr->setRotationSpeed(this->rotationSpeed->getReal());
		this->movingBehaviorPtr->setFlyMode(this->flyMode->getBool());

		return true;
	}

	bool AiLuaComponent::disconnect(void)
	{
		if (nullptr != this->movingBehaviorPtr)
		{
			delete this->pathGoalObserver;
			this->pathGoalObserver = nullptr;
			this->movingBehaviorPtr->setPathGoalObserver(nullptr);
			this->movingBehaviorPtr->reset();
		}

		this->luaStateMachine->resetStates();

		this->ready = false;
		return true;
	}

	void AiLuaComponent::handleLuaScriptConnected(NOWA::EventDataPtr eventData)
	{
		boost::shared_ptr<EventDataLuaScriptConnected> castEventData = boost::static_pointer_cast<EventDataLuaScriptConnected>(eventData);
		// Found the game object
		if (this->gameObjectPtr->getId() == castEventData->getGameObjectId())
		{
			// Call enter on the start state
			this->setActivated(this->activated->getBool());
		}
	}

	void AiLuaComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();
		// Dangerous in destructor, as when exiting the simulation, the game object will be deleted and this function called, to seek for another ai component, that has been deleted
		// Thus handle it, just when a component is removed
		bool stillAiComponentActive = false;
		for (size_t i = 0; i < this->gameObjectPtr->getComponents()->size(); i++)
		{
			auto aiCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<AiComponent>());
			// Seek for this component and go to the previous one to process
			if (aiCompPtr != nullptr)
			{
				stillAiComponentActive = true;
				break;
			}
			else
			{
				auto aiLuaCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<AiLuaComponent>());
				// Seek for this component and go to the previous one to process
				if (aiLuaCompPtr != nullptr &&  aiLuaCompPtr.get() != this)
				{
					stillAiComponentActive = true;
					break;
				}
			}
		}

		// If there is no ai component for this game object anymore, remove the behavior
		if (false == stillAiComponentActive)
		{
			AppStateManager::getSingletonPtr()->getGameObjectController()->removeMovingBehavior(this->gameObjectPtr->getId());
		}
	}

	void AiLuaComponent::onOtherComponentRemoved(unsigned int index)
	{
		auto& gameObjectCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponentByIndex(index));
		if (nullptr != gameObjectCompPtr)
		{
			auto& physicsCompPtr = boost::dynamic_pointer_cast<PhysicsComponent>(gameObjectCompPtr);
			if (nullptr != physicsCompPtr)
			{
				if (nullptr != this->movingBehaviorPtr)
				{
					this->movingBehaviorPtr->setAgentId(0);
				}
			}
		}
	}

	void AiLuaComponent::onOtherComponentAdded(unsigned int index)
	{
		if (nullptr == this->gameObjectPtr)
		{
			return;
		}

		auto& gameObjectCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponentByIndex(index));
		if (nullptr != gameObjectCompPtr)
		{
			auto& physicsActiveCompPtr = boost::dynamic_pointer_cast<PhysicsActiveComponent>(gameObjectCompPtr);
			if (nullptr != physicsActiveCompPtr)
			{
				if (nullptr != this->movingBehaviorPtr)
				{
					this->movingBehaviorPtr->setAgentId(this->gameObjectPtr->getId());
				}
			}
		}
	}

	Ogre::String AiLuaComponent::getClassName(void) const
	{
		return "AiLuaComponent";
	}

	Ogre::String AiLuaComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void AiLuaComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (true == this->activated->getBool() && false == notSimulating)
		{
			// Is done in GameObjectController, since components for one game object are sharing one moving behavior
			// this->movingBehaviorPtr->update(dt);
			this->luaStateMachine->update(dt);
		}
	}

	void AiLuaComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (AiLuaComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (AiLuaComponent::AttrRotationSpeed() == attribute->getName())
		{
			this->setRotationSpeed(attribute->getReal());
		}
		else if (AiLuaComponent::AttrFlyMode() == attribute->getName())
		{
			this->setFlyMode(attribute->getBool());
		}
		else if (AiLuaComponent::AttrStartStateName() == attribute->getName())
		{
			this->setStartStateName(attribute->getString());
		}
	}

	void AiLuaComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// Note string is parsed for Component*, hence ComponentAiLua, but the real class name is AiLuaComponent
		
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FlyMode"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->flyMode->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "StartStateName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->startStateName->getString())));
		propertiesXML->append_node(propertyXML);
	}

	bool AiLuaComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// Check if there is already an ai lua component, just one allowed and physics active component is required
		auto aiLuaCompPtr = NOWA::makeStrongPtr(gameObject->getComponentFromName<GameObjectComponent>("AiLuaComponent"));
		auto aiLuaGoalCompPtr = NOWA::makeStrongPtr(gameObject->getComponentFromName<GameObjectComponent>("AiLuaGoalComponent", true));
		// Note: template is GameObjectComponent so that e.g. PhysicsRagDollComponent, which is derived from PhysicsActiveComponent will also count as valid component
		auto physicsCompPtr = NOWA::makeStrongPtr(gameObject->getComponentFromName<GameObjectComponent>("PhysicsActiveComponent"));
		if (nullptr == aiLuaCompPtr /*&& nullptr == aiLuaGoalCompPtr*/ && nullptr != physicsCompPtr)
		{
			return true;
		}
		return false;
	}

	void AiLuaComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);

		auto luaScriptComponent = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<LuaScriptComponent>());

		if (true == activated && nullptr != luaScriptComponent)
		{
			if (false == luaScriptComponent->isActivated())
			{
				// If not activated, first activate the lua script component, so that the script will be compiled, because its necessary for this component
				// luaScriptComponent->setActivated(true);
				boost::shared_ptr<EventDataPrintLuaError> eventDataPrintLuaError(new EventDataPrintLuaError(this->gameObjectPtr->getLuaScript()->getScriptName(), 0, 
					"Cannot start ai lua state + '" + this->startStateName->getString() + "', because the 'LuaScriptComponent' is not activated for game object: " + this->gameObjectPtr->getName()));
				AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataPrintLuaError);
				return;
			}

			// http://www.allacrost.org/wiki/index.php?title=Scripting_Engine

			this->gameObjectPtr->getLuaScript()->setInterfaceFunctionsTemplate(
				"\n" + this->startStateName->getString() + " = { };\n"
				"aiLuaComponent = nil;\n\n"
				+ this->startStateName->getString() + "[\"enter\"] = function(gameObject)\n"
				"\taiLuaComponent = gameObject:getAiLuaComponent();\nend\n\n"
				+ this->startStateName->getString() + "[\"execute\"] = function(gameObject, dt)\n\nend\n\n"
				+ this->startStateName->getString() + "[\"exit\"] = function(gameObject)\n\nend");

			bool startStateAvailable = AppStateManager::getSingletonPtr()->getLuaScriptModule()->checkLuaStateAvailable(this->gameObjectPtr->getLuaScript()->getName(), this->startStateName->getString());
			if (false == startStateAvailable && false == this->componentCloned)
			{
				boost::shared_ptr<EventDataPrintLuaError> eventDataPrintLuaError(new EventDataPrintLuaError(this->gameObjectPtr->getLuaScript()->getScriptName(), 0, 
					"Cannot start ai lua state, because the start state name: '" + this->startStateName->getString() + "' is not defined for game object: " + this->gameObjectPtr->getName()));
				AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataPrintLuaError);
				return;
			}

			if (false == this->componentCloned && true == this->gameObjectPtr->getLuaScript()->createLuaEnvironmentForStateTable(this->startStateName->getString()))
			{
				const luabind::object& compiledStateScriptReference = this->gameObjectPtr->getLuaScript()->getCompiledStateScriptReference();

				this->ready = true;
				// Call the start state name to start the lua file with that state
				this->luaStateMachine->setCurrentState(compiledStateScriptReference);
			}
		}
	}

	bool AiLuaComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void AiLuaComponent::setRotationSpeed(Ogre::Real rotationSpeed)
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
		if (nullptr != this->movingBehaviorPtr)
		{
			this->movingBehaviorPtr->setRotationSpeed(this->rotationSpeed->getReal());
		}
	}

	Ogre::Real AiLuaComponent::getRotationSpeed(void) const
	{
		return rotationSpeed->getReal();
	}

	void AiLuaComponent::setFlyMode(bool flyMode)
	{
		this->flyMode->setValue(flyMode);
		if (nullptr != this->movingBehaviorPtr)
			this->movingBehaviorPtr->setFlyMode(this->flyMode->getBool());
	}

	bool AiLuaComponent::getIsInFlyMode(void) const
	{
		return this->flyMode->getBool();
	}

	void AiLuaComponent::setStartStateName(const Ogre::String& startStateName)
	{
		this->startStateName->setValue(startStateName);
	}

	Ogre::String AiLuaComponent::getStartStateName(void) const
	{
		return this->startStateName->getString();
	}

	KI::LuaStateMachine<GameObject>* AiLuaComponent::getStateMachine(void) const
	{
		return this->luaStateMachine;
	}

	NOWA::KI::MovingBehavior* AiLuaComponent::getMovingBehavior(void) const
	{
		return this->movingBehaviorPtr.get();
	}

	void AiLuaComponent::setCurrentState(const luabind::object& currentState)
	{
		if (true == this->ready)
		{
			this->luaStateMachine->setCurrentState(currentState);
		}
		else
		{
			boost::shared_ptr<EventDataPrintLuaError> eventDataPrintLuaError(new EventDataPrintLuaError(this->gameObjectPtr->getLuaScript()->getScriptName(), 0, 
					"Cannot 'setCurrentState' in 'connect' function of lua script. It can only be set in an own state and ai lua component must be activated, for game object: " + this->gameObjectPtr->getName()));
			AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataPrintLuaError);
		}
	}

	void AiLuaComponent::setGlobalState(const luabind::object& globalState)
	{
		if (true == this->ready)
		{
			this->luaStateMachine->setGlobalState(globalState);
		}
		else
		{
			boost::shared_ptr<EventDataPrintLuaError> eventDataPrintLuaError(new EventDataPrintLuaError(this->gameObjectPtr->getLuaScript()->getScriptName(), 0, 
					"Cannot 'setGlobalState' in 'connect' function of lua script. It can only be set in an own state and ai lua component must be activated, for game object: " + this->gameObjectPtr->getName()));
			AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataPrintLuaError);
		}
	}

	void AiLuaComponent::setPreviousState(const luabind::object& previousState)
	{
		if (true == this->ready)
		{
			this->luaStateMachine->setPreviousState(previousState);
		}
		else
		{
			boost::shared_ptr<EventDataPrintLuaError> eventDataPrintLuaError(new EventDataPrintLuaError(this->gameObjectPtr->getLuaScript()->getScriptName(), 0, 
					"Cannot 'setPreviousState' in 'connect' function of lua script. It can only be set in an own state and ai lua component must be activated, for game object: " + this->gameObjectPtr->getName()));
			AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataPrintLuaError);
		}
	}

	void AiLuaComponent::changeState(const luabind::object& newState)
	{
		if (true == this->ready)
		{
			this->luaStateMachine->changeState(newState);
		}
		else
		{
			boost::shared_ptr<EventDataPrintLuaError> eventDataPrintLuaError(new EventDataPrintLuaError(this->gameObjectPtr->getLuaScript()->getScriptName(), 0, 
					"Cannot 'changeState' in 'connect' function of lua script. It can only be set in an own state and ai lua component must be activated, for game object: " + this->gameObjectPtr->getName()));
			AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataPrintLuaError);
		}
	}

	void AiLuaComponent::exitGlobalState(void)
	{
		if (true == this->ready)
		{
			this->luaStateMachine->exitGlobalState();
		}
		else
		{
			boost::shared_ptr<EventDataPrintLuaError> eventDataPrintLuaError(new EventDataPrintLuaError(this->gameObjectPtr->getLuaScript()->getScriptName(), 0, 
					"Cannot 'exitGlobalState' in 'connect' function of lua script. It can only be set in an own state and ai lua component must be activated, for game object: " + this->gameObjectPtr->getName()));
			AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataPrintLuaError);
		}
	}

	void AiLuaComponent::revertToPreviousState(void)
	{
		if (true == this->ready)
		{
			this->luaStateMachine->revertToPreviousState();
		}
		else
		{
			boost::shared_ptr<EventDataPrintLuaError> eventDataPrintLuaError(new EventDataPrintLuaError(this->gameObjectPtr->getLuaScript()->getScriptName(), 0, 
					"Cannot 'revertToPreviousState' in 'connect' function of lua script. It can only be set in an own state and ai lua component must be activated, for game object: " + this->gameObjectPtr->getName()));
			AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataPrintLuaError);
		}
	}

	const luabind::object& AiLuaComponent::getCurrentState(void) const
	{
		return this->luaStateMachine->getCurrentState();
	}

	const luabind::object& AiLuaComponent::getPreviousState(void) const
	{
		return this->luaStateMachine->getPreviousState();
	}

	const luabind::object& AiLuaComponent::getGlobalState(void) const
	{
		return this->luaStateMachine->getGlobalState();
	}

	void AiLuaComponent::reactOnPathGoalReached(luabind::object closureFunction)
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

	void AiLuaComponent::reactOnAgentStuck(luabind::object closureFunction)
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

}; // namespace end
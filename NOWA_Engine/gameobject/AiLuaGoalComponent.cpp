#include "NOWAPrecompiled.h"
#include "AiLuaGoalComponent.h"
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

	AiLuaGoalComponent::AiLuaGoalComponent()
		: GameObjectComponent(),
		activated(new Variant(AiLuaGoalComponent::AttrActivated(), true, this->attributes)),
		rotationSpeed(new Variant(AiLuaGoalComponent::AttrRotationSpeed(), 10.0f, this->attributes)),
		flyMode(new Variant(AiLuaGoalComponent::AttrFlyMode(), false, this->attributes)),
		rootGoalName(new Variant(AiLuaGoalComponent::AttrRootGoalName(), "MyRootGoal", this->attributes)),
		luaGoalMachine(nullptr),
		pathGoalObserver(nullptr),
		agentStuckObserver(nullptr),
		ready(false),
		componentCloned(false)
	{

	}

	AiLuaGoalComponent::~AiLuaGoalComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AiLuaGoalComponent] Destructor ai lua goal component for game object: " + this->gameObjectPtr->getName());
		AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &AiLuaGoalComponent::handleLuaScriptConnected), EventDataLuaScriptConnected::getStaticEventType());

		if (this->luaGoalMachine)
		{
			delete this->luaGoalMachine;
			this->luaGoalMachine = nullptr;
		}
		this->targetGameObject = nullptr;
	}

	bool AiLuaGoalComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

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
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RootGoalName")
		{
			this->rootGoalName->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &AiLuaGoalComponent::handleLuaScriptConnected), EventDataLuaScriptConnected::getStaticEventType());
	
		return true;
	}

	GameObjectCompPtr AiLuaGoalComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		AiLuaGoalCompPtr clonedCompPtr(boost::make_shared<AiLuaGoalComponent>());

		clonedCompPtr->componentCloned = true;

		clonedCompPtr->setRotationSpeed(this->rotationSpeed->getReal());
		clonedCompPtr->setFlyMode(this->flyMode->getBool());
		clonedCompPtr->setRootGoalName(this->rootGoalName->getString());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->postInit();
		clonedCompPtr->setActivated(this->activated->getBool());

		clonedCompPtr->componentCloned = false;

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool AiLuaGoalComponent::postInit(void)
	{
		// Do not set things a second time after cloning
		if (nullptr != this->luaGoalMachine)
		{
			return true;
		}

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AiLuaGoalComponent] Init ai lua component for game object: " + this->gameObjectPtr->getName());

		// Initialize LuaGoalMachine with an empty goal for now
		// TODO: Use type -> 0??
		this->luaGoalMachine = new LuaGoalComposite<GameObject>(this->gameObjectPtr.get(), luabind::object());

		this->movingBehaviorPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->addMovingBehavior(this->gameObjectPtr->getId());
		if (nullptr == this->movingBehaviorPtr)
		{
			return false;
		}

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		return true;
	}

	bool AiLuaGoalComponent::connect(void)
	{
		GameObjectComponent::connect();

		this->movingBehaviorPtr->setRotationSpeed(this->rotationSpeed->getReal());
		this->movingBehaviorPtr->setFlyMode(this->flyMode->getBool());

		return true;
	}

	bool AiLuaGoalComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();

		if (nullptr != this->movingBehaviorPtr)
		{
			delete this->pathGoalObserver;
			this->pathGoalObserver = nullptr;
			this->movingBehaviorPtr->setPathGoalObserver(nullptr);
			this->movingBehaviorPtr->reset();
		}

		this->luaGoalMachine->removeAllSubGoals<GameObject>();

		this->ready = false;
		return true;
	}

	void AiLuaGoalComponent::handleLuaScriptConnected(NOWA::EventDataPtr eventData)
	{
		boost::shared_ptr<EventDataLuaScriptConnected> castEventData = boost::static_pointer_cast<EventDataLuaScriptConnected>(eventData);
		// Found the game object
		if (this->gameObjectPtr->getId() == castEventData->getGameObjectId())
		{
			// Call enter on the start state
			this->setActivated(this->activated->getBool());
		}
	}

	void AiLuaGoalComponent::onRemoveComponent(void)
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
				auto aiLuaCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<AiLuaGoalComponent>());
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

	void AiLuaGoalComponent::onOtherComponentRemoved(unsigned int index)
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

	void AiLuaGoalComponent::onOtherComponentAdded(unsigned int index)
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

	Ogre::String AiLuaGoalComponent::getClassName(void) const
	{
		return "AiLuaGoalComponent";
	}

	Ogre::String AiLuaGoalComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void AiLuaGoalComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (true == this->activated->getBool() && false == notSimulating && true == this->ready)
		{
			// Is done in GameObjectController, since components for one game object are sharing one moving behavior
			// this->movingBehaviorPtr->update(dt);
			this->luaGoalMachine->process(dt);
		}
	}

	void AiLuaGoalComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (AiLuaGoalComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (AiLuaGoalComponent::AttrRotationSpeed() == attribute->getName())
		{
			this->setRotationSpeed(attribute->getReal());
		}
		else if (AiLuaGoalComponent::AttrFlyMode() == attribute->getName())
		{
			this->setFlyMode(attribute->getBool());
		}
		else if (AiLuaGoalComponent::AttrRootGoalName() == attribute->getName())
		{
			this->setRootGoalName(attribute->getString());
		}
	}

	void AiLuaGoalComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		// Note string is parsed for Component*, hence ComponentAiLua, but the real class name is AiLuaGoalComponent
		
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RootGoalName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rootGoalName->getString())));
		propertiesXML->append_node(propertyXML);
	}

	void AiLuaGoalComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);

		auto luaScriptComponent = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<LuaScriptComponent>());

		if (true == activated && nullptr != luaScriptComponent)
		{
			if (false == luaScriptComponent->isActivated())
			{
				// If not activated, first activate the lua script component, so that the script will be compiled, because its necessary for this component
				// luaScriptComponent->setActivated(true);
				boost::shared_ptr<EventDataPrintLuaError> eventDataPrintLuaError(new EventDataPrintLuaError(this->gameObjectPtr->getLuaScript()->getScriptName(), this->gameObjectPtr->getLuaScript()->getScriptFilePathName(), 0,
					"Cannot start ai lua state + '" + this->rootGoalName->getString() + "', because the 'LuaScriptComponent' is not activated for game object: " + this->gameObjectPtr->getName()));
				AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataPrintLuaError);
				return;
			}

			// http://www.allacrost.org/wiki/index.php?title=Scripting_Engine
#if 0
			this->gameObjectPtr->getLuaScript()->setInterfaceFunctionsTemplate(
				"\n" + this->rootGoalName->getString() + " = { };\n"
				"aiLuaComponent = nil;\n\n"
				+ this->rootGoalName->getString() + "[\"enter\"] = function(gameObject)\n"
				"\taiLuaComponent = gameObject:getAiLuaComponent();\nend\n\n"
				+ this->rootGoalName->getString() + "[\"execute\"] = function(gameObject, dt)\n\nend\n\n"
				+ this->rootGoalName->getString() + "[\"exit\"] = function(gameObject)\n\nend");
#endif
			bool rootGoalAvailable = AppStateManager::getSingletonPtr()->getLuaScriptModule()->checkLuaStateAvailable(this->gameObjectPtr->getLuaScript()->getName(), this->rootGoalName->getString());
			if (false == rootGoalAvailable && false == this->componentCloned)
			{
				boost::shared_ptr<EventDataPrintLuaError> eventDataPrintLuaError(new EventDataPrintLuaError(this->gameObjectPtr->getLuaScript()->getScriptName(), this->gameObjectPtr->getLuaScript()->getScriptFilePathName(), 0,
					"Cannot start ai lua state, because the start state name: '" + this->rootGoalName->getString() + "' is not defined for game object: " + this->gameObjectPtr->getName()));
				AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataPrintLuaError);
				return;
			}

			if (false == this->componentCloned && true == this->gameObjectPtr->getLuaScript()->createLuaEnvironmentForStateTable(this->rootGoalName->getString()))
			{
				const luabind::object& compiledStateScriptReference = this->gameObjectPtr->getLuaScript()->getCompiledStateScriptReference();

				this->ready = true;
				
				this->setRootGoal(compiledStateScriptReference);
			}
		}
	}

	// Set the root goal, which can be a LuaGoalComposite or other derived goal
	void AiLuaGoalComponent::setRootGoal(const luabind::object& rootGoal)
	{
		this->terminate();

		// Set the root goal, using the GameObject owner and the Lua goal
		this->luaGoalMachine = new LuaGoalComposite<GameObject>(this->gameObjectPtr.get(), rootGoal);
		this->luaGoalMachine->activate();
	}

	// Add a subgoal to the root goal (can be composite or atomic)
	void AiLuaGoalComponent::addSubGoal(const luabind::object& subGoal)
	{
		if (nullptr != this->luaGoalMachine)
		{
			this->luaGoalMachine->addSubGoal<GameObject>(subGoal);
		}
	}

	void AiLuaGoalComponent::terminate(void)
	{
		if (nullptr != this->luaGoalMachine)
		{
			this->luaGoalMachine->terminate();
			delete this->luaGoalMachine;
			this->luaGoalMachine = nullptr;
		}
	}

	void AiLuaGoalComponent::removeAllSubGoals(void)
	{
		this->luaGoalMachine->removeAllSubGoals<GameObject>();
	}

	bool AiLuaGoalComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void AiLuaGoalComponent::setRotationSpeed(Ogre::Real rotationSpeed)
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

	Ogre::Real AiLuaGoalComponent::getRotationSpeed(void) const
	{
		return rotationSpeed->getReal();
	}

	void AiLuaGoalComponent::setFlyMode(bool flyMode)
	{
		this->flyMode->setValue(flyMode);
		if (nullptr != this->movingBehaviorPtr)
			this->movingBehaviorPtr->setFlyMode(this->flyMode->getBool());
	}

	bool AiLuaGoalComponent::getIsInFlyMode(void) const
	{
		return this->flyMode->getBool();
	}

	void AiLuaGoalComponent::setRootGoalName(const Ogre::String& rootGoalName)
	{
		this->rootGoalName->setValue(rootGoalName);
	}

	Ogre::String AiLuaGoalComponent::getRootGoalName(void) const
	{
		return this->rootGoalName->getString();
	}

	KI::LuaGoalComposite<GameObject>* AiLuaGoalComponent::getGoalMachine(void) const
	{
		return this->luaGoalMachine;
	}

	NOWA::KI::MovingBehavior* AiLuaGoalComponent::getMovingBehavior(void) const
	{
		return this->movingBehaviorPtr.get();
	}
#if 0
	void AiLuaGoalComponent::setCurrentState(const luabind::object& currentState)
	{
		if (true == this->ready)
		{
			this->luaGoalMachine->setCurrentState(currentState);
		}
		else
		{
			boost::shared_ptr<EventDataPrintLuaError> eventDataPrintLuaError(new EventDataPrintLuaError(this->gameObjectPtr->getLuaScript()->getScriptName(), this->gameObjectPtr->getLuaScript()->getScriptFilePathName(), 0,
					"Cannot 'setCurrentState' in 'connect' function of lua script. It can only be set in an own state and ai lua component must be activated, for game object: " + this->gameObjectPtr->getName()));
			AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataPrintLuaError);
		}
	}

	void AiLuaGoalComponent::setGlobalState(const luabind::object& globalState)
	{
		if (true == this->ready)
		{
			this->luaGoalMachine->setGlobalState(globalState);
		}
		else
		{
			boost::shared_ptr<EventDataPrintLuaError> eventDataPrintLuaError(new EventDataPrintLuaError(this->gameObjectPtr->getLuaScript()->getScriptName(), this->gameObjectPtr->getLuaScript()->getScriptFilePathName(), 0,
					"Cannot 'setGlobalState' in 'connect' function of lua script. It can only be set in an own state and ai lua component must be activated, for game object: " + this->gameObjectPtr->getName()));
			AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataPrintLuaError);
		}
	}

	void AiLuaGoalComponent::setPreviousState(const luabind::object& previousState)
	{
		if (true == this->ready)
		{
			this->luaGoalMachine->setPreviousState(previousState);
		}
		else
		{
			boost::shared_ptr<EventDataPrintLuaError> eventDataPrintLuaError(new EventDataPrintLuaError(this->gameObjectPtr->getLuaScript()->getScriptName(), this->gameObjectPtr->getLuaScript()->getScriptFilePathName(), 0,
					"Cannot 'setPreviousState' in 'connect' function of lua script. It can only be set in an own state and ai lua component must be activated, for game object: " + this->gameObjectPtr->getName()));
			AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataPrintLuaError);
		}
	}

	void AiLuaGoalComponent::changeState(const luabind::object& newState)
	{
		if (true == this->ready)
		{
			this->luaGoalMachine->changeState(newState);
		}
		else
		{
			boost::shared_ptr<EventDataPrintLuaError> eventDataPrintLuaError(new EventDataPrintLuaError(this->gameObjectPtr->getLuaScript()->getScriptName(), this->gameObjectPtr->getLuaScript()->getScriptFilePathName(), 0,
					"Cannot 'changeState' in 'connect' function of lua script. It can only be set in an own state and ai lua component must be activated, for game object: " + this->gameObjectPtr->getName()));
			AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataPrintLuaError);
		}
	}

	void AiLuaGoalComponent::exitGlobalState(void)
	{
		if (true == this->ready)
		{
			this->luaGoalMachine->exitGlobalState();
		}
		else
		{
			boost::shared_ptr<EventDataPrintLuaError> eventDataPrintLuaError(new EventDataPrintLuaError(this->gameObjectPtr->getLuaScript()->getScriptName(), this->gameObjectPtr->getLuaScript()->getScriptFilePathName(), 0,
					"Cannot 'exitGlobalState' in 'connect' function of lua script. It can only be set in an own state and ai lua component must be activated, for game object: " + this->gameObjectPtr->getName()));
			AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataPrintLuaError);
		}
	}

	void AiLuaGoalComponent::revertToPreviousState(void)
	{
		if (true == this->ready)
		{
			this->luaGoalMachine->revertToPreviousState();
		}
		else
		{
			boost::shared_ptr<EventDataPrintLuaError> eventDataPrintLuaError(new EventDataPrintLuaError(this->gameObjectPtr->getLuaScript()->getScriptName(), this->gameObjectPtr->getLuaScript()->getScriptFilePathName(), 0,
					"Cannot 'revertToPreviousState' in 'connect' function of lua script. It can only be set in an own state and ai lua component must be activated, for game object: " + this->gameObjectPtr->getName()));
			AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataPrintLuaError);
		}
	}

	const luabind::object& AiLuaGoalComponent::getCurrentState(void) const
	{
		return this->luaGoalMachine->getCurrentState();
	}

	const luabind::object& AiLuaGoalComponent::getPreviousState(void) const
	{
		return this->luaGoalMachine->getPreviousState();
	}

	const luabind::object& AiLuaGoalComponent::getGlobalState(void) const
	{
		return this->luaGoalMachine->getGlobalState();
	}
#endif

	void AiLuaGoalComponent::reactOnPathGoalReached(luabind::object closureFunction)
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

	void AiLuaGoalComponent::reactOnAgentStuck(luabind::object closureFunction)
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

	// Lua registration part

	AiLuaGoalComponent* getAiLuaGoalComponent(GameObject* gameObject)
	{
		return makeStrongPtr<AiLuaGoalComponent>(gameObject->getComponent<AiLuaGoalComponent>()).get();
	}

	AiLuaGoalComponent* getAiLuaGoalComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<AiLuaGoalComponent>(gameObject->getComponentFromName<AiLuaGoalComponent>(name)).get();
	}

	void AiLuaGoalComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObject, class_<GameObjectController>& gameObjectController)
	{
		module(lua)
		[
			class_<NOWA::KI::GoalResult>("GoalResult")
			.enum_("LuaGoalState")
			[
				value("ACTIVE", NOWA::KI::GoalResult::LuaGoalState::ACTIVE),
				value("INACTIVE", NOWA::KI::GoalResult::LuaGoalState::INACTIVE),
				value("COMPLETED", NOWA::KI::GoalResult::LuaGoalState::COMPLETED),
				value("FAILED", NOWA::KI::GoalResult::LuaGoalState::FAILED)
			]
			.def(luabind::constructor<>()) // Default constructor
			.def("setStatus", &NOWA::KI::GoalResult::setStatus)
			.def("getStatus", &NOWA::KI::GoalResult::getStatus)
		];

		LuaScriptApi::getInstance()->addClassToCollection("GoalResult", "class", "Class representing the result of a goals execution.");
		LuaScriptApi::getInstance()->addClassToCollection("GoalResult", "GoalResult()", "Default constructor for GoalResult.");
		LuaScriptApi::getInstance()->addClassToCollection("GoalResult", "void setStatus(LuaGoalState status)", "Sets the status of the goal. Use one of the following states: ACTIVE, INACTIVE, COMPLETED, FAILED.");
		LuaScriptApi::getInstance()->addClassToCollection("GoalResult", "LuaGoalState getStatus()", "Gets the current status of the goal.");
		LuaScriptApi::getInstance()->addClassToCollection("GoalResult", "LuaGoalState", "Enumeration representing the possible states of a goal.");
		LuaScriptApi::getInstance()->addClassToCollection("GoalResult", "LuaGoalState ACTIVE", "The goal is currently active and being processed.");
		LuaScriptApi::getInstance()->addClassToCollection("GoalResult", "LuaGoalState INACTIVE", "The goal is inactive and not yet started.");
		LuaScriptApi::getInstance()->addClassToCollection("GoalResult", "LuaGoalState COMPLETED", "The goal has been successfully completed.");
		LuaScriptApi::getInstance()->addClassToCollection("GoalResult", "LuaGoalState FAILED", "The goal has failed to complete.");

		module(lua)
		[
			class_<NOWA::KI::LuaGoal<GameObject>>("LuaGoal")
			.def(luabind::constructor<GameObject*, luabind::object>()) // Exposes constructor
			.def("isComplete", &NOWA::KI::LuaGoal<GameObject>::isComplete)
			.def("isActive", &NOWA::KI::LuaGoal<GameObject>::isActive)
			.def("isInactive", &NOWA::KI::LuaGoal<GameObject>::isInactive)
			.def("hasFailed", &NOWA::KI::LuaGoal<GameObject>::hasFailed)
			// .def("getType", &NOWA::KI::LuaGoal<GameObject>::getType)
			.def("activate", &NOWA::KI::LuaGoal<GameObject>::activate)
		];

		LuaScriptApi::getInstance()->addClassToCollection("LuaGoal", "class LuaGoal", "A leaf atomic goal, which cannot have children anymore.");
		LuaScriptApi::getInstance()->addClassToCollection("LuaGoal", "bool isComplete()", "Gets whether this goal is completed.");
		LuaScriptApi::getInstance()->addClassToCollection("LuaGoal", "bool isActive()", "Gets whether this goal is still active.");
		LuaScriptApi::getInstance()->addClassToCollection("LuaGoal", "bool isInactive()", "Gets whether this goal is inactive.");
		LuaScriptApi::getInstance()->addClassToCollection("LuaGoal", "bool hasFailed()", "Gets whether this goal has failed.");

		module(lua)
		[
			class_<NOWA::KI::LuaGoalComposite<GameObject>, NOWA::KI::LuaGoal<GameObject>>("LuaGoalComposite")
			.def(luabind::constructor<GameObject*, luabind::object>()) // Exposes constructor
			.def("isComplete", &NOWA::KI::LuaGoalComposite<GameObject>::isComplete)
			.def("isActive", &NOWA::KI::LuaGoalComposite<GameObject>::isActive)
			.def("isInactive", &NOWA::KI::LuaGoalComposite<GameObject>::isInactive)
			.def("hasFailed", &NOWA::KI::LuaGoalComposite<GameObject>::hasFailed)
			// .def("getType", &NOWA::KI::LuaGoalComposite<GameObject>::getType)
			.def("addSubGoal", &NOWA::KI::LuaGoalComposite<GameObject>::addSubGoal<GameObject>)
			.def("removeAllSubGoals", &NOWA::KI::LuaGoalComposite<GameObject>::removeAllSubGoals<GameObject>)
			.def("activate", &NOWA::KI::LuaGoalComposite<GameObject>::activate)
			.def("process", &NOWA::KI::LuaGoalComposite<GameObject>::process)
		];

		LuaScriptApi::getInstance()->addClassToCollection("LuaGoalComposite", "class inherits LuaGoal", "Composite goal class used for managing hierarchical goal structures.");
		LuaScriptApi::getInstance()->addClassToCollection("LuaGoalComposite", "bool isComplete()", "Gets whether this goal is completed.");
		LuaScriptApi::getInstance()->addClassToCollection("LuaGoalComposite", "bool isActive()", "Gets whether this goal is still active.");
		LuaScriptApi::getInstance()->addClassToCollection("LuaGoalComposite", "bool isInactive()", "Gets whether this goal is inactive.");
		LuaScriptApi::getInstance()->addClassToCollection("LuaGoalComposite", "bool hasFailed()", "Gets whether this goal has failed.");
		// LuaScriptApi::getInstance()->addClassToCollection("LuaGoalComposite", "LuaGoalComposite(Owner* owner, Table luaGoalTable)", "Constructor for creating a composite goal. Requires an owner object and a Lua goal table.");
		LuaScriptApi::getInstance()->addClassToCollection("LuaGoalComposite", "void addSubGoal(Table luaGoalTable)", "Adds a new atomic or composite sub-goal to this composite goal.");
		LuaScriptApi::getInstance()->addClassToCollection("LuaGoalComposite", "void removeAllSubGoals()", "Removes and terminates all sub-goals managed by this composite goal.");
		LuaScriptApi::getInstance()->addClassToCollection("LuaGoalComposite", "void activate()", "Activates this composite goal by calling its 'activate' function in Lua.");
		LuaScriptApi::getInstance()->addClassToCollection("LuaGoalComposite", "void terminate()", "Terminates this composite goal and calls its 'terminate' function in Lua.");
		LuaScriptApi::getInstance()->addClassToCollection("LuaGoalComposite", "GoalResult process(Ogre::Real dt)", "Processes the composite goal and its sub-goals, calling their 'process' functions in Lua. Returns the result of the goal's processing.");

		module(lua)
		[
			class_<AiLuaGoalComponent, GameObjectComponent>("AiLuaGoalComponent")
			.def("setActivated", &AiLuaGoalComponent::setActivated)
			.def("isActivated", &AiLuaGoalComponent::isActivated)
			.def("setRotationSpeed", &AiLuaGoalComponent::setRotationSpeed)
			.def("getRotationSpeed", &AiLuaGoalComponent::getRotationSpeed)
			.def("setFlyMode", &AiLuaGoalComponent::setFlyMode)
			.def("getIsInFlyMode", &AiLuaGoalComponent::getIsInFlyMode)
			.def("setRootGoalName", &AiLuaGoalComponent::setRootGoalName)
			.def("getRootGoalName", &AiLuaGoalComponent::getRootGoalName)
			// No: Is done via AiLuaGoalComponent, because of some sanity checks??
			// .def("getStateMachine", &AiLuaGoalComponent::getStateMachine)
			.def("getMovingBehavior", &AiLuaGoalComponent::getMovingBehavior)
			.def("setRootGoal", &AiLuaGoalComponent::setRootGoal)
			.def("addSubGoal", &AiLuaGoalComponent::addSubGoal)
			// .def("getGoalMachine", &AiLuaGoalComponent::getGoalMachine)
				

			// .def("setGlobalState", &AiLuaGoalComponent::setGlobalState)
			// .def("exitGlobalState", &AiLuaGoalComponent::exitGlobalState)
			// .def("getCurrentState", &AiLuaGoalComponent::getCurrentState)
			// .def("getPreviousState", &AiLuaGoalComponent::getPreviousState)
			// .def("getGlobalState", &AiLuaGoalComponent::getGlobalState)
			// .def("changeState", &AiLuaGoalComponent::changeState)
			// .def("revertToPreviousState", &AiLuaGoalComponent::revertToPreviousState)
			.def("reactOnPathGoalReached", &AiLuaGoalComponent::reactOnPathGoalReached)
			.def("reactOnAgentStuck", &AiLuaGoalComponent::reactOnAgentStuck)
		];

		LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "class inherits GameObjectComponent", AiLuaGoalComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "void setActivated(bool activated)", "Activates the components behaviour, so that the lua script will be executed.");
		LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "bool isActivated()", "Gets whether the component behaviour is activated or not.");
		LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "void setRotationSpeed(float speed)", "Sets the agent rotation speed. Default value is 0.1.");
		LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "float getRotationSpeed()", "Gets the agent rotation speed. Default value is 0.1.");
		LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "void setFlyMode(bool flyMode)", "Sets whether to use fly mode (taking y-axis into account). Note: this can be used e.g. for birds flying around.");
		LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "bool isInFlyMode()", "Gets whether the agent is in flying mode (taking y-axis into account).");
		LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "void setRootGoalName(String rootGoalName)", "Sets the root goal name, which will be loaded in lua script and executed.");
		LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "String getRootGoalName()", "Gets the root goal name, which is loaded in lua script and executed.");
		// LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "LuaGoalMachine getGoalMachine()", "Gets the lua goal machine for manipulating states.");
		LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "MovingBehavior getMovingBehavior()", "Gets the moving behavior instance for this agent.");
		LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "void setRootGoal(Table rootGoal)", "Sets the root goal for this AI component. Can be a LuaGoalComposite or any derived goal.");
		LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "void addSubGoal(Table subGoal)", "Adds a subgoal to the root goal. Can be a LuaGoal or LuaGoalComposite.");

		// LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "void setGlobalState(Table stateName)", "Sets the global state name, which will be loaded in lua script and executed besides the current state. Important: Do not use state function in 'connect' function, because at this time the AiLuaComponent is not ready!");
		// LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "void exitGlobalState()", "Exits the global state, if it does exist.");
		// LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "String getCurrentState()", "Gets the current state name.");
		// LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "String getPreviousState()", "Gets the previous state name, that has been loaded bevor the current state name. Important: Do not use state function in 'connect' function, because at this time the AiLuaComponent is not ready!");
		// LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "String getGlobalState()", "Gets the global state name, if existing, else empty string. Important: Do not use state function in 'connect' function, because at this time the AiLuaComponent is not ready!");
		// LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "void changeState(Table newStateName)", "Changes the current state name to a new one. Calles 'exit' function on the current state and 'enter' on the new state in lua script. Important: Do not use state function in 'connect' function, because at this time the AiLuaComponent is not ready!");
		// LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "void revertToPreviousState()", "Changes the current state name to the previous one. Calles 'exit' function on the current state and 'enter' on the previous state in lua script. Important: Do not use state function in 'connect' function, because at this time the AiLuaComponent is not ready!");
		LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "void reactOnPathGoalReached(func closure, GameObject targetGameObject, string functionName, GameObject gameObject)",
			"Sets whether to react the agent reached the goal.");
		LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "void reactOnAgentStuck(func closure, GameObject targetGameObject, string functionName, GameObject gameObject)",
			"Sets whether to react the agent got stuck.");

		gameObject.def("getAiLuaGoalComponent", (AiLuaGoalComponent * (*)(GameObject*)) & getAiLuaGoalComponent);
		gameObject.def("getAiLuaGoalComponentFromName", &getAiLuaGoalComponentFromName);
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "AiLuaGoalComponent getAiLuaGoalComponent()", "Gets the ai lua goal component. This can be used if the game object just has one ai lua goal component.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "AiLuaGoalComponent getAiLuaGoalComponentFromName(String name)", "Gets the ai lua goal component.");

		gameObjectController.def("castAiLuaGoalComponentComponent", &GameObjectController::cast<AiLuaGoalComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "AiLuaGoalComponent castAiLuaGoalComponent(AiLuaGoalComponent other)", "Casts an incoming type from function for lua auto completion.");

		gameObjectController.def("castGoalResult", &GameObjectController::cast<GoalResult>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "GoalResult castGoalResult(GoalResult other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool AiLuaGoalComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// Check if there is already an ai lua component, just one allowed and physics active component is required
		// Note: template is GameObjectComponent so that e.g. PhysicsRagDollComponent, which is derived from PhysicsActiveComponent will also count as valid component
		auto aiLuaCompPtr = NOWA::makeStrongPtr(gameObject->getComponentFromName<GameObjectComponent>("AiLuaComponent"));
		auto aiLuaGoalCompPtr = NOWA::makeStrongPtr(gameObject->getComponentFromName<GameObjectComponent>("AiLuaGoalComponent"));
		auto physicsCompPtr = NOWA::makeStrongPtr(gameObject->getComponentFromName<GameObjectComponent>("PhysicsActiveComponent", true));
		if (nullptr == aiLuaCompPtr && nullptr == aiLuaGoalCompPtr && nullptr != physicsCompPtr)
		{
			return true;
		}
		return false;
	}

}; // namespace end
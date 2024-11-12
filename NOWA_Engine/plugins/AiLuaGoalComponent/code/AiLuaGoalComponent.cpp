#include "NOWAPrecompiled.h"
#include "AiLuaGoalComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"
#include "gameobject/PhysicsActiveComponent.h"

#include "OgreAbiUtils.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	AiLuaGoalComponent::AiLuaGoalComponent()
		: GameObjectComponent(),
		name("AiLuaGoalComponent"),
		activated(new Variant(AiLuaGoalComponent::AttrActivated(), true, this->attributes)),
		rotationSpeed(new Variant(AiLuaGoalComponent::AttrRotationSpeed(), 0.1f, this->attributes)),
		flyMode(new Variant(AiLuaGoalComponent::AttrFlyMode(), false, this->attributes)),
		startGoalCompositeName(new Variant(AiLuaGoalComponent::AttrStartGoalCompositeName(), "MyGoalComposite", this->attributes)),
		luaGoalComposite(nullptr),
		pathGoalObserver(nullptr),
		agentStuckObserver(nullptr),
		ready(false),
		componentCloned(false)
	{
		
	}

	AiLuaGoalComponent::~AiLuaGoalComponent(void)
	{
		
	}

	void AiLuaGoalComponent::initialise()
	{

	}

	const Ogre::String& AiLuaGoalComponent::getName() const
	{
		return this->name;
	}

	void AiLuaGoalComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<AiLuaGoalComponent>(AiLuaGoalComponent::getStaticClassId(), AiLuaGoalComponent::getStaticClassName());
	}

	void AiLuaGoalComponent::shutdown()
	{

	}

	void AiLuaGoalComponent::uninstall()
	{

	}
	
	void AiLuaGoalComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool AiLuaGoalComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
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
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "StartGoalCompositeName")
		{
			this->startGoalCompositeName->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		return true;
	}

	GameObjectCompPtr AiLuaGoalComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		AiLuaGoalComponentPtr clonedCompPtr(boost::make_shared<AiLuaGoalComponent>());

		clonedCompPtr->componentCloned = true;

		clonedCompPtr->setRotationSpeed(this->rotationSpeed->getReal());
		clonedCompPtr->setFlyMode(this->flyMode->getBool());
		clonedCompPtr->setStartGoalCompositeName(this->startGoalCompositeName->getString());

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
		if (nullptr != this->luaGoalComposite)
		{
			return true;
		}

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AiLuaGoalComponent] Init ai lua goal component for game object: " + this->gameObjectPtr->getName());

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
		this->movingBehaviorPtr->setRotationSpeed(this->rotationSpeed->getReal());
		this->movingBehaviorPtr->setFlyMode(this->flyMode->getBool());

		this->setActivated(this->activated->getBool());
		return true;
	}

	bool AiLuaGoalComponent::disconnect(void)
	{
		if (nullptr != this->movingBehaviorPtr)
		{
			this->movingBehaviorPtr->reset();
			// TODO: Is this correct?
			if (nullptr != this->luaGoalComposite)
			{
				this->luaGoalComposite->removeAllSubGoals<GameObject>();
				if (nullptr != this->luaGoalComposite)
				{
					delete this->luaGoalComposite;
					this->luaGoalComposite = nullptr;
				}
			}

			this->ready = false;
		}
		return true;
	}

	bool AiLuaGoalComponent::onCloned(void)
	{
		return true;
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
				auto aiLuaGoalCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<AiLuaGoalComponent>());
				// Seek for this component and go to the previous one to process
				if (aiLuaGoalCompPtr != nullptr && aiLuaGoalCompPtr.get() != this)
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

		if (nullptr != this->luaGoalComposite)
		{
			delete this->luaGoalComposite;
			this->luaGoalComposite = nullptr;
		}
		this->targetGameObject = nullptr;
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AiLuaGoalComponent] Destructor ai lua goal component for game object: " + this->gameObjectPtr->getName());
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

	void AiLuaGoalComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (true == this->activated->getBool() && false == notSimulating)
		{
			// Is done in GameObjectController, since components for one game object are sharing one moving behavior
			// this->movingBehaviorPtr->update(dt);
			if (nullptr != this->luaGoalComposite)
			{
				this->luaGoalComposite->process(dt);
			}
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
		else if (AiLuaGoalComponent::AttrStartGoalCompositeName() == attribute->getName())
		{
			this->setStartGoalCompositeName(attribute->getString());
		}
	}

	void AiLuaGoalComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FlyMode"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->flyMode->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "StartGoalCompositeName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->startGoalCompositeName->getString())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String AiLuaGoalComponent::getClassName(void) const
	{
		return "AiLuaGoalComponent";
	}

	Ogre::String AiLuaGoalComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
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
					"Cannot start ai lua state + '" + this->startGoalCompositeName->getString() + "', because the 'LuaScriptComponent' is not activated for game object: " + this->gameObjectPtr->getName()));
				AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataPrintLuaError);
				return;
			}

			// http://www.allacrost.org/wiki/index.php?title=Scripting_Engine

			this->gameObjectPtr->getLuaScript()->setInterfaceFunctionsTemplate(
				"\n" + this->startGoalCompositeName->getString() + " = { };\n"
				"aiLuaGoalComponent = nil;\n\n"
				+ this->startGoalCompositeName->getString() + "[\"activate\"] = function(gameObject)\n"
				"\aiLuaGoalComponent = gameObject:getAiLuaGoalComponent();\nend\n\n"
				+ this->startGoalCompositeName->getString() + "[\"process\"] = function(gameObject, dt, status)\n\nend\n\n"
				+ this->startGoalCompositeName->getString() + "[\"terminate\"] = function(gameObject)\n\nend");

			bool startStateAvailable = AppStateManager::getSingletonPtr()->getLuaScriptModule()->checkLuaStateAvailable(this->gameObjectPtr->getLuaScript()->getName(), this->startGoalCompositeName->getString());
			if (false == startStateAvailable && false == this->componentCloned)
			{
				boost::shared_ptr<EventDataPrintLuaError> eventDataPrintLuaError(new EventDataPrintLuaError(this->gameObjectPtr->getLuaScript()->getScriptName(), this->gameObjectPtr->getLuaScript()->getScriptFilePathName(), 0,
					"Cannot start ai lua goal, because the start goal name: '" + this->startGoalCompositeName->getString() + "' is not defined for game object: " + this->gameObjectPtr->getName()));
				AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataPrintLuaError);
				return;
			}

			if (false == this->componentCloned && true == this->gameObjectPtr->getLuaScript()->createLuaEnvironmentForStateTable(this->startGoalCompositeName->getString()))
			{
				const luabind::object& compiledStateScriptReference = this->gameObjectPtr->getLuaScript()->getCompiledStateScriptReference();

				this->ready = true;
				if (nullptr == this->luaGoalComposite)
				{
					// Call the start state name to start the lua file with that state
					this->luaGoalComposite = new LuaGoalComposite<GameObject>(this->gameObjectPtr.get(), compiledStateScriptReference);
					this->luaGoalComposite->setStartGoalComposite(compiledStateScriptReference);
				}
			}
		}
	}

	bool AiLuaGoalComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void AiLuaGoalComponent::setRotationSpeed(Ogre::Real rotationSpeed)
	{
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

	void AiLuaGoalComponent::setStartGoalCompositeName(const Ogre::String& startGoalCompositeName)
	{
		this->startGoalCompositeName->setValue(startGoalCompositeName);
	}

	Ogre::String AiLuaGoalComponent::getStartGoalCompositeName(void) const
	{
		return this->startGoalCompositeName->getString();
	}

	KI::LuaGoalComposite<GameObject>* AiLuaGoalComponent::getLuaGoalComposite(void) const
	{
		return this->luaGoalComposite;
	}

	NOWA::KI::MovingBehavior* AiLuaGoalComponent::getMovingBehavior(void) const
	{
		return this->movingBehaviorPtr.get();
	}

	void AiLuaGoalComponent::addSubGoal(const luabind::object& luaSubGoalTable)
	{
		this->luaGoalComposite->addSubGoal<GameObject>(luaSubGoalTable);
	}

	void AiLuaGoalComponent::removeAllSubGoals(void)
	{
		this->luaGoalComposite->removeAllSubGoals<GameObject>();
	}

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
	}

	// Lua registration part

	AiLuaGoalComponent* getAiLuaGoalComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<AiLuaGoalComponent>(gameObject->getComponentWithOccurrence<AiLuaGoalComponent>(occurrenceIndex)).get();
	}

	AiLuaGoalComponent* getAiLuaGoalComponent(GameObject* gameObject)
	{
		return makeStrongPtr<AiLuaGoalComponent>(gameObject->getComponent<AiLuaGoalComponent>()).get();
	}

	AiLuaGoalComponent* getAiLuaGoalComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<AiLuaGoalComponent>(gameObject->getComponentFromName<AiLuaGoalComponent>(name)).get();
	}

	void AiLuaGoalComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
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
			.def("setStatus", &NOWA::KI::GoalResult::setStatus)
			.def("getStatus", &NOWA::KI::GoalResult::getStatus)
		];

		LuaScriptApi::getInstance()->addClassToCollection("GoalResult", "class GoalResult", "A class that hold a status of the current goal.");
		LuaScriptApi::getInstance()->addClassToCollection("LuaGoal", "void setStatus(LuaGoalState status)", "Sets the lua goal state. Possible values: ACTIVE, INACTIVE, COMPLETED, Failed.");
		LuaScriptApi::getInstance()->addClassToCollection("LuaGoal", "LuaGoalState getStatus()", "Gets the lua goal state. Possible values: ACTIVE, INACTIVE, COMPLETED, Failed.");

		module(lua)
		[
			class_<NOWA::KI::LuaGoal<GameObject>>("LuaGoal")
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
			class_<NOWA::KI::LuaGoalComposite<GameObject>, NOWA::KI::LuaGoal<GameObject>>("LuaGoal")
			.def("isComplete", &NOWA::KI::LuaGoalComposite<GameObject>::isComplete)
			.def("isActive", &NOWA::KI::LuaGoalComposite<GameObject>::isActive)
			.def("isInactive", &NOWA::KI::LuaGoalComposite<GameObject>::isInactive)
			.def("hasFailed", &NOWA::KI::LuaGoalComposite<GameObject>::hasFailed)
			// .def("getType", &NOWA::KI::LuaGoalComposite<GameObject>::getType)
			.def("addSubGoal", &NOWA::KI::LuaGoalComposite<GameObject>::addSubGoal<GameObject>)
			.def("removeAllSubGoals", &NOWA::KI::LuaGoalComposite<GameObject>::removeAllSubGoals<GameObject>)
			.def("activate", &NOWA::KI::LuaGoalComposite<GameObject>::activate)
		];

		LuaScriptApi::getInstance()->addClassToCollection("LuaGoalComposite", "class inherits LuaGoal", "A composite goal can be brocken in to child composite sub goals or goals.");
		LuaScriptApi::getInstance()->addClassToCollection("LuaGoalComposite", "bool isComplete()", "Gets whether this goal is completed.");
		LuaScriptApi::getInstance()->addClassToCollection("LuaGoalComposite", "bool isActive()", "Gets whether this goal is still active.");
		LuaScriptApi::getInstance()->addClassToCollection("LuaGoalComposite", "bool isInactive()", "Gets whether this goal is inactive.");
		LuaScriptApi::getInstance()->addClassToCollection("LuaGoalComposite", "bool hasFailed()", "Gets whether this goal has failed.");
		LuaScriptApi::getInstance()->addClassToCollection("LuaGoalComposite", "void addSubGoal(table luaGoalTable)", "Adds a sub goal lua table.");
		LuaScriptApi::getInstance()->addClassToCollection("LuaGoalComposite", "void removeAllSubGoals()", "Removes all sub goals.");

		// TODO: Create component like ailuacomponent!
		module(lua)
		[
			class_<AiLuaGoalComponent, GameObjectComponent>("AiLuaGoalComponent")
			.def("setActivated", &AiLuaGoalComponent::setActivated)
			.def("isActivated", &AiLuaGoalComponent::isActivated)
			.def("setRotationSpeed", &AiLuaGoalComponent::setRotationSpeed)
			.def("getRotationSpeed", &AiLuaGoalComponent::getRotationSpeed)
			.def("setFlyMode", &AiLuaGoalComponent::setFlyMode)
			.def("getIsInFlyMode", &AiLuaGoalComponent::getIsInFlyMode)
			// Note: Start is always a composite goal! Because it may have children! A goal is a leaf and cannot have children anymore.
			.def("setStartGoalCompositeName", &AiLuaGoalComponent::setStartGoalCompositeName)
			.def("getStartGoalCompositeName", &AiLuaGoalComponent::getStartGoalCompositeName)
			.def("getMovingBehavior", &AiLuaGoalComponent::getMovingBehavior)

			.def("addSubGoal", &AiLuaGoalComponent::addSubGoal)
			.def("removeAllSubGoals", &AiLuaGoalComponent::removeAllSubGoals)
			.def("reactOnPathGoalReached", &AiLuaGoalComponent::reactOnPathGoalReached)
			.def("reactOnAgentStuck", &AiLuaGoalComponent::reactOnAgentStuck)
		];

		LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "class inherits GameObjectComponent", AiLuaGoalComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not.");
		LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "bool isActivated()", "Gets whether this component is activated.");
		LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "void setRotationSpeed(float speed)", "Sets the agent rotation speed. Default value is 0.1.");
		LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "float getRotationSpeed()", "Gets the agent rotation speed. Default value is 0.1.");
		LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "void setFlyMode(bool flyMode)", "Sets whether to use fly mode (taking y-axis into account). Note: this can be used e.g. for birds flying around.");
		LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "bool isInFlyMode()", "Gets whether the agent is in flying mode (taking y-axis into account).");
		LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "void setStartGoalCompositeName(String startGoalCompositeName)", "Sets the start goal composite name, which will be loaded in lua script and executed.");
		LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "String getStartStateName()", "Gets the start goal composite name, which is loaded in lua script and executed.");
		LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "MovingBehavior getMovingBehavior()", "Gets the moving behavior instance for this agent.");

		LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "void addSubGoal(table luaGoalTable)", "Adds a sub goal lua table.");
		LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "void removeAllSubGoals()", "Removes all sub goals.");
		LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "void reactOnPathGoalReached(GameObject targetGameObject, string functionName, GameObject gameObject)",
			"Sets wheather to react the agent reached the goal. The target object is, where this function should be called in lua script. "
			"Optional game object to use in the function. May be nil. Note: The target game object should be the game object with the lua script component, in which this function is called!");
		LuaScriptApi::getInstance()->addClassToCollection("AiLuaGoalComponent", "void reactOnAgentStuck(GameObject targetGameObject, string functionName, GameObject gameObject)",
			"Sets wheather to react the agent got stuck. The target object is, where this function should be called in lua script. "
			"Optional game object to use in the function. May be nil. Note: The target game object should be the game object with the lua script component, in which this function is called!");

		gameObjectClass.def("getAiLuaGoalComponentFromName", &getAiLuaGoalComponentFromName);
		gameObjectClass.def("getAiLuaGoalComponent", (AiLuaGoalComponent * (*)(GameObject*)) & getAiLuaGoalComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "AiLuaGoalComponent getAiLuaGoalComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "AiLuaGoalComponent getAiLuaGoalComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castAiLuaGoalComponent", &GameObjectController::cast<AiLuaGoalComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "AiLuaGoalComponent castAiLuaGoalComponent(AiLuaGoalComponent other)", "Casts an incoming type from function for lua auto completion.");
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

}; //namespace end
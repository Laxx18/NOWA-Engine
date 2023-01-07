#include "NOWAPrecompiled.h"
#include "CrowdComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "modules/OgreRecastModule.h"
#include "OgreDetourCrowd.h"
#include "OgreDetourTileCache.h"
#include "gameobject/PhysicsActiveComponent.h"
#include "gameobject/NavMeshComponent.h"

#include "OgreAbiUtils.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	CrowdComponent::CrowdComponent()
		: GameObjectComponent(),
		detourCrowd(nullptr),
		detourTileCache(nullptr),
		agentId(-1),
		agent(nullptr),
		physicsActiveComponent(nullptr),
		destination(Ogre::Vector3::ZERO),
		tempObstacle(0),
		manualVelocity(Ogre::Vector3::ZERO),
		stopped(true),
		inSimulation(false),
		goalRadius(0.5f),
		activated(new Variant(CrowdComponent::AttrActivated(), true, this->attributes)),
		controlled(new Variant(CrowdComponent::AttrManuallyControlled(), true, this->attributes))
	{
		this->activated->setDescription("Description.");
	}

	CrowdComponent::~CrowdComponent(void)
	{
		
	}

	bool CrowdComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ManuallyControlled")
		{
			this->controlled->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		return true;
	}

	GameObjectCompPtr CrowdComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		CrowdComponentPtr clonedCompPtr(boost::make_shared<CrowdComponent>());

		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setControlled(this->controlled->getBool());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool CrowdComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[CrowdComponent] Init component for game object: " + this->gameObjectPtr->getName());

		this->detourCrowd = AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreDetourCrowd();
		this->detourTileCache = AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreDetourTileCache();

		auto physicsActiveCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsActiveComponent>());
		if (nullptr != physicsActiveCompPtr)
		{
			this->physicsActiveComponent = physicsActiveCompPtr.get();
		}

		return true;
	}

	bool CrowdComponent::connect(void)
	{
		if (true == this->activated->getBool() && true == this->controlled->getBool())
		{
			// Remove agent from crowd and re-add at position
			if (-1 != this->agentId)
			{
				this->detourCrowd->removeAgent(this->agentId);
			}
			this->agentId = this->detourCrowd->addAgent(this->gameObjectPtr->getPosition());
			this->agent = this->detourCrowd->getAgent(this->agentId);
			this->inSimulation = true;
		}
		else
		{
			if (-1 != this->agentId)
			{
				this->detourCrowd->removeAgent(this->agentId);
			}
		}

		return true;
	}

	bool CrowdComponent::disconnect(void)
	{
		if (-1 != this->agentId)
		{
			this->detourCrowd->removeAgent(this->agentId);
			this->agentId = -1;
			this->agent = nullptr;
			this->stopped = true;
			this->destination = Ogre::Vector3::ZERO;
			this->manualVelocity = Ogre::Vector3::ZERO;
			this->controlled->setValue(false);
			this->inSimulation = false;
		}
		return true;
	}

	bool CrowdComponent::onCloned(void)
	{
		
		return true;
	}

	void CrowdComponent::onRemoveComponent(void)
	{
		
	}
	
	void CrowdComponent::onOtherComponentRemoved(unsigned int index)
	{
		auto& gameObjectCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponentByIndex(index));
		if (nullptr != gameObjectCompPtr)
		{
			auto& physicsActiveCompPtr = boost::dynamic_pointer_cast<PhysicsActiveComponent>(gameObjectCompPtr);
			if (nullptr != physicsActiveCompPtr)
			{
				this->physicsActiveComponent = nullptr;
			}
		}
	}
	
	void CrowdComponent::onOtherComponentAdded(unsigned int index)
	{
		
	}
	
	void CrowdComponent::update(Ogre::Real dt, bool notSimulating)
	{
		this->detourCrowd->updateTick(dt);
	}

	void CrowdComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (CrowdComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if(CrowdComponent::AttrManuallyControlled() == attribute->getName())
		{
			this->setControlled(attribute->getBool());
		}
	}

	void CrowdComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->activated->getBool())));
		propertiesXML->append_node(propertyXML);	

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ManuallyControlled"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->controlled->getBool())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String CrowdComponent::getClassName(void) const
	{
		return "CrowdComponent";
	}

	Ogre::String CrowdComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void CrowdComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
		this->setControlled(this->controlled->getBool());
	}

	bool CrowdComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	Ogre::Real CrowdComponent::getAgentHeight(void) const
	{
		return this->detourCrowd->getAgentHeight();
	}

	Ogre::Real CrowdComponent::getAgentRadius(void) const
	{
		return this->detourCrowd->getAgentRadius();
	}

	void CrowdComponent::setControlled(bool controlled)
	{
		if (true == this->controlled->getBool())
		{
			if (this->tempObstacle)
			{
				this->detourTileCache->removeTempObstacle(this->tempObstacle);
			}
			this->tempObstacle = 0;
			this->agentId = this->detourCrowd->addAgent(getPosition());
			this->destination = this->detourCrowd->getLastDestination();
			this->manualVelocity = Ogre::Vector3::ZERO;
			this->stopped = true;
		}
		else
		{
			this->tempObstacle = this->detourTileCache->addTempObstacle(getPosition());   // Add temp obstacle at current position
			this->detourCrowd->removeAgent(this->agentId);
			this->agentId = -1;
			this->destination = Ogre::Vector3::ZERO;     // TODO this is not ideal
			this->stopped = false;
		}
		this->controlled->setValue(controlled);
	}

	bool CrowdComponent::isControlled(void) const
	{
		return this->controlled->getBool();
	}

	bool CrowdComponent::isActive(void) const
	{
		if (nullptr != this->agent)
		{
			return this->agent->active;
		}
		return false;
	}

	void CrowdComponent::setGoalRadius(Ogre::Real goalRadius)
	{
		this->goalRadius = goalRadius;
	}

	void CrowdComponent::updateDestination(Ogre::Vector3 destination, bool updatePreviousPath)
	{
		if (false == this->controlled->getBool())
		{
			return;
		}

		// Find position on navmesh
		if (false == this->detourCrowd->m_recast->findNearestPointOnNavmesh(destination, destination))
		{
			return;
		}

		this->detourCrowd->setMoveTarget(this->agentId, destination, updatePreviousPath);
		this->destination = destination;
		this->stopped = false;
		this->manualVelocity = Ogre::Vector3::ZERO;
	}

	Ogre::Vector3 CrowdComponent::beginUpdateVelocity(void)
	{
		if (true == this->controlled->getBool())
		{
			if (true == this->agent->active)
			{
				/*Ogre::Vector3 agentPos;
				OgreRecast::FloatAToOgreVect3(this->agent->npos, agentPos);

				getNode()->setPosition(agentPos);*/

				return this->getVelocity();
			}
		}
		else
		{
			// Move character manually to new position
			if (this->getVelocity().isZeroLength())
			{
				return Ogre::Vector3::ZERO;
			}

			// Make other agents avoid first character by placing a temporary obstacle in its position
			this->detourTileCache->removeTempObstacle(this->tempObstacle);   // Remove old obstacle
			// getNode()->setPosition(getPosition() + dt * this->getVelocity());

			return this->getVelocity();
		}
		return Ogre::Vector3::ZERO;
	}

	void CrowdComponent::endUpdateVelocity(void)
	{
		if (false == this->controlled->getBool())
		{
			// TODO check whether this position is within navmesh
			this->tempObstacle = this->detourTileCache->addTempObstacle(getPosition());   // Add new obstacle
		}
	}

	void CrowdComponent::setDestination(const Ogre::Vector3& destination)
	{
		if (false == this->controlled->getBool())
		{
			return;
		}

		this->destination = destination;
		this->manualVelocity = Ogre::Vector3::ZERO;
		this->stopped = false;
	}

	Ogre::Vector3 CrowdComponent::getDestination(void) const
	{
		if (true == this->controlled->getBool())
		{
			return this->destination;
		}
		return Ogre::Vector3::ZERO;
	}

	void CrowdComponent::setPosition(Ogre::Vector3& position)
	{
		if (false == this->controlled->getBool())
		{
			if (nullptr != this->physicsActiveComponent)
			{
				this->physicsActiveComponent->setPosition(position);
			}
			else
			{
				this->gameObjectPtr->getSceneNode()->setPosition(position);
			}
			return;
		}

		// Find position on navmesh
		if (false == this->detourCrowd->m_recast->findNearestPointOnNavmesh(position, position))
		{
			return;
		}

		// Remove agent from crowd and re-add at position
		this->detourCrowd->removeAgent(this->agentId);
		this->agentId = this->detourCrowd->addAgent(position);
		this->agent = this->detourCrowd->getAgent(this->agentId);

		if (nullptr != this->physicsActiveComponent)
		{
			this->physicsActiveComponent->setPosition(position);
		}
		else
		{
			this->gameObjectPtr->getSceneNode()->setPosition(position);
		}
	}

	bool CrowdComponent::destinationReached(void)
	{
		if (this->gameObjectPtr->getPosition().squaredDistance(this->getDestination()) <= this->goalRadius)
		{
			return true;
		}

		return this->detourCrowd->destinationReached(this->agent, this->goalRadius);
	}

	void CrowdComponent::setVelocity(Ogre::Vector3 velocity)
	{
		this->manualVelocity = velocity;
		this->stopped = false;
		this->destination = Ogre::Vector3::ZERO;

		if (true == this->controlled->getBool())
		{
			this->detourCrowd->requestVelocity(this->agentId, this->manualVelocity);
		}
	}

	void CrowdComponent::stop(void)
	{
		if (false == this->controlled->getBool())
		{
			this->manualVelocity = Ogre::Vector3::ZERO;
			this->stopped = true;
			return;
		}

		if (this->detourCrowd->stopAgent(this->agentId))
		{
			this->destination = Ogre::Vector3::ZERO;
			this->manualVelocity = Ogre::Vector3::ZERO;
			this->stopped = true;
		}
	}

	Ogre::Vector3 CrowdComponent::getVelocity(void)
	{
		if (true == this->controlled->getBool())
		{
			Ogre::Vector3 velocity;
			OgreRecast::FloatAToOgreVect3(this->agent->nvel, velocity);
			return velocity;
		}
		else
		{
			return this->manualVelocity;
		}
	}

	Ogre::Real CrowdComponent::getSpeed(void)
	{
		return this->getVelocity().length();
	}

	Ogre::Real CrowdComponent::getMaxSpeed(void)
	{
		if (nullptr != this->agent)
		{
			return this->agent->params.maxSpeed;
		}
		else
		{
			return 0.0f;
		}
	}

	Ogre::Real CrowdComponent::getMaxAcceleration(void)
	{
		if (nullptr != this->agent)
		{
			return this->agent->params.maxAcceleration;
		}
		else
		{
			return 0.0f;
		}
	}

	bool CrowdComponent::isMoving(void)
	{
		return false == this->stopped || this->getSpeed() != 0.0f;
	}

	// Lua registration part

	CrowdComponent* getCrowdComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<CrowdComponent>(gameObject->getComponentWithOccurrence<CrowdComponent>(occurrenceIndex)).get();
	}

	CrowdComponent* getCrowdComponent(GameObject* gameObject)
	{
		return makeStrongPtr<CrowdComponent>(gameObject->getComponent<CrowdComponent>()).get();
	}

	CrowdComponent* getCrowdComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<CrowdComponent>(gameObject->getComponentFromName<CrowdComponent>(name)).get();
	}

	void CrowdComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<CrowdComponent, GameObjectComponent>("CrowdComponent")
			.def("setActivated", &CrowdComponent::setActivated)
			.def("isActivated", &CrowdComponent::isActivated)
			.def("setControlled", &CrowdComponent::setControlled)
			.def("isControlled", &CrowdComponent::isControlled)
		];

		LuaScriptApi::getInstance()->addClassToCollection("CrowdComponent", "class inherits GameObjectComponent", CrowdComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("CrowdComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not.");
		LuaScriptApi::getInstance()->addClassToCollection("CrowdComponent", "bool isActivated()", "Gets whether this component is activated.");
		LuaScriptApi::getInstance()->addClassToCollection("CrowdComponent", "void setControlled(bool controlled)", "Set whether this character is controlled by an agent or whether it will position itself independently based on the requested velocity. "
														  "Set to true to let the character be controlled by an agent.Set to false to manually control it without agent.");
		LuaScriptApi::getInstance()->addClassToCollection("CrowdComponent", "bool isControlled()", "Gets whether this component is controlled manually.");

		gameObjectClass.def("getCrowdComponentFromName", &getCrowdComponentFromName);
		gameObjectClass.def("getCrowdComponent", (CrowdComponent * (*)(GameObject*)) & getCrowdComponent);
		// If its desired to create several of this components for one game object
		gameObjectClass.def("getCrowdComponent2", (CrowdComponent * (*)(GameObject*, unsigned int)) & getCrowdComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "CrowdComponent getCrowdComponent2(unsigned int occurrenceIndex)", "Gets the component by the given occurence index, since a game object may this component maybe several times.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "CrowdComponent getCrowdComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "CrowdComponent getCrowdComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castCrowdComponent", &GameObjectController::cast<CrowdComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "CrowdComponent castCrowdComponent(CrowdComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool CrowdComponent::canStaticAddComponent(GameObject* gameObject)
	{
		auto physicsActiveCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<PhysicsActiveComponent>());
		auto navMeshCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<NavMeshComponent>());
		auto crowdCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<CrowdComponent>());
		
		if (nullptr != physicsActiveCompPtr && nullptr == navMeshCompPtr && nullptr != AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreRecast() && nullptr == crowdCompPtr)
		{
			return true;
		}
		return false;
	}

}; //namespace end
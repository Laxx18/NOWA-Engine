#include "NOWAPrecompiled.h"
#include "RaceGoalComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"

#include "OgreAbiUtils.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	RaceGoalComponent::RaceGoalComponent()
		: GameObjectComponent(),
		name("RaceGoalComponent"),
		finished(false),
		pPath(nullptr),
		physicsActiveComponent(nullptr),
		checkpointDefaultDirection(Ogre::Vector3::UNIT_Z),
		currentLap(0),
		lapTimeSec(0.0f),
		wrongDirection(false),
		oldDirection(true),
		speedInKmh(0.0f),
		checkpointsCount(new Variant(RaceGoalComponent::AttrCheckpointsCount(), 0, this->attributes)),
		lapsCount(new Variant(RaceGoalComponent::AttrLapsCount(), static_cast<unsigned int>(1), this->attributes)),
		onFeedbackRaceFunctionName(new Variant(RaceGoalComponent::AttrOnFeedbackRaceFunctionName(), Ogre::String(""), this->attributes))
	{
		// Since when waypoints count is changed, the whole properties must be refreshed, so that new field may come for way points
		this->checkpointsCount->addUserData(GameObject::AttrActionNeedRefresh());
		this->checkpointsCount->setDescription("Note: Id's must be of game objects with a PhysicsActiveKinematicComponent, which shall be placed as a wall, with which the vehicle does collide for checkpoint determination.");

		this->onFeedbackRaceFunctionName->setDescription("Sets the function name to react in lua script to get feedback whether the vehicle is driving in the wrong direction, or which lap has been passed, the lap time, whether the race is finished. E.g. onRaceFeedback(wrongDirection, currentLap, lapTimeSec, finished).");
		this->onFeedbackRaceFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), this->onFeedbackRaceFunctionName->getString() + "(wrongDirection, currentLap, lapTimeSec, finished)");
	}

	RaceGoalComponent::~RaceGoalComponent(void)
	{
		
	}

	const Ogre::String& RaceGoalComponent::getName() const
	{
		return this->name;
	}

	void RaceGoalComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<RaceGoalComponent>(RaceGoalComponent::getStaticClassId(), RaceGoalComponent::getStaticClassName());
	}
	
	void RaceGoalComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool RaceGoalComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CheckpointsCount")
		{
			this->checkpointsCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data", 1));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (this->checkpoints.size() < this->checkpointsCount->getUInt())
		{
			this->checkpoints.resize(this->checkpointsCount->getUInt());
		}
		for (size_t i = 0; i < this->checkpoints.size(); i++)
		{
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Checkpoint" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->checkpoints[i])
				{
					this->checkpoints[i] = new Variant(RaceGoalComponent::AttrCheckpoint() + Ogre::StringConverter::toString(i), XMLConverter::getAttribUnsignedLong(propertyElement, "data", 0), this->attributes);
				}
				else
				{
					this->checkpoints[i]->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data", 0));
				}
				propertyElement = propertyElement->next_sibling("property");
				this->checkpoints[i]->addUserData(GameObject::AttrActionSeparator());
			}
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "LapsCount")
		{
			this->lapsCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OnFeedbackRaceFunctionName")
		{
			this->onFeedbackRaceFunctionName->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		return true;
	}

	GameObjectCompPtr RaceGoalComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		RaceGoalComponentPtr clonedCompPtr(boost::make_shared<RaceGoalComponent>());

		clonedCompPtr->setCheckpointsCount(this->checkpointsCount->getUInt());

		for (unsigned int i = 0; i < static_cast<unsigned int>(this->checkpoints.size()); i++)
		{
			clonedCompPtr->setCheckpointId(i, this->checkpoints[i]->getULong());
		}
		clonedCompPtr->setLapsCount(this->lapsCount->getUInt());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->setOnFeedbackRaceFunctionName(this->onFeedbackRaceFunctionName->getString());

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool RaceGoalComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[RaceGoalComponent] Init component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool RaceGoalComponent::connect(void)
	{
		if (nullptr == this->pPath)
		{
			this->pPath = new Path();
		}

		this->pPath->clear();
		this->pPath->setRepeat(true);
		this->currentLap = 0;
		this->lapTimeSec = 0.0f;
		this->speedInKmh = 0.0f;

		this->setOnFeedbackRaceFunctionName(this->onFeedbackRaceFunctionName->getString());

		for (size_t i = 0; i < this->checkpoints.size(); i++)
		{
			if (nullptr == this->checkpoints[i])
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[RaceGoalComponent] Error, cannot use checkpoints, because there are no check point ids set for game object: " + this->gameObjectPtr->getName());
				return false;
			}

			GameObjectPtr checkpointGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->checkpoints[i]->getULong());
			if (nullptr != checkpointGameObjectPtr)
			{
				const auto& kinematicCompPtr = NOWA::makeStrongPtr(checkpointGameObjectPtr->getComponent<PhysicsActiveKinematicComponent>());
				if (nullptr != kinematicCompPtr)
				{
					// Must not be collidable, but contact detection still works :)
					kinematicCompPtr->setCollidable(false);
					this->kinematicComponents.emplace_back(kinematicCompPtr.get());
					// Add the way points and orientation
					this->pPath->addWayPoint(checkpointGameObjectPtr->getPosition(), checkpointGameObjectPtr->getOrientation());
					this->checkpointDefaultDirection = checkpointGameObjectPtr->getDefaultDirection();
				}
			}
		}

		this->physicsActiveComponent = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsActiveComponent>()).get();

		// const auto& kinematicComponent = this->kinematicComponents[this->pPath->getCurrentWaypointIndex()];

		for (const auto& kinematicComponent : this->kinematicComponents)
		{
			static_cast<OgreNewt::KinematicBody*>(kinematicComponent->getBody())->setKinematicContactCallback<RaceGoalComponent>([this, &kinematicComponent](RaceGoalComponent* instancedClassPointer, OgreNewt::Body* body)
			{
				// Checks if the checkpoint is the one, which also matches the current checkpoint index, e.g. it shall not possible to ride back and hit the last checkpoint, before hitting the first checkpoint
				size_t index = static_cast<size_t>(this->pPath->getCurrentWaypointIndex());
				if (kinematicComponent->getOwner()->getId() != this->checkpoints[index]->getULong())
				{
					return;
				}

				// Hole id von physicscomponent, ob diese gleich ist was object am index
				// this->pPath->getCurrentWaypointIndex()
				PhysicsComponent* otherPhysicsComponent = OgreNewt::any_cast<PhysicsComponent*>(body->getUserData());
				const auto& thisPhysicsComponent = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsComponent>());
				if (nullptr == thisPhysicsComponent)
				{
					return;
				}

				if (otherPhysicsComponent != thisPhysicsComponent.get())
				{
					return;
				}

				if (true == this->isMovingTowardsCheckpoint())
				{
					if (this->pPath->getCurrentWaypointIndex() + 1 >= this->checkpointsCount->getUInt())
					{
						this->currentLap++;
						this->lapTimeSec = 0.0f;
					}

					if (this->pPath->getCurrentWaypointIndex() + 1 >= this->lapsCount->getUInt() * this->checkpointsCount->getUInt())
					{
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[RaceGoalComponent] Finished track!");
						this->finished = true;
						this->pPath->clear();
					}
					this->pPath->setNextWayPoint();
				}
			}, this);
		}

		return true;
	}

	bool RaceGoalComponent::disconnect(void)
	{
		if (nullptr != this->pPath)
		{
			this->pPath->clear();
		}

		this->finished = false;
		for (const auto& kinematicComponent : this->kinematicComponents)
		{
			static_cast<OgreNewt::KinematicBody*>(kinematicComponent->getBody())->removeKinematicContactCallback();
		}

		this->kinematicComponents.clear();
		this->physicsActiveComponent = nullptr;
		
		return true;
	}

	bool RaceGoalComponent::onCloned(void)
	{
		
		return true;
	}

	void RaceGoalComponent::onRemoveComponent(void)
	{
		
	}
	
	void RaceGoalComponent::onOtherComponentRemoved(unsigned int index)
	{
		
	}
	
	void RaceGoalComponent::onOtherComponentAdded(unsigned int index)
	{
		
	}
	
	void RaceGoalComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			this->lapTimeSec += dt;

			this->speedInKmh = this->calculateSpeedInKmh();
		}
	}

	bool RaceGoalComponent::isMovingTowardsCheckpoint(void)
	{
		Ogre::Vector3 curPos = this->gameObjectPtr->getPosition();
		Ogre::Quaternion curOrient = this->gameObjectPtr->getOrientation();
		Ogre::Vector3 waypointPos = this->currentCheckpoint.second.first;
		Ogre::Quaternion waypointOrient = this->currentCheckpoint.second.second;

		// Calculate the vector to the goal
		Ogre::Vector3 toGoal(waypointPos.x - curPos.x, 0.0f, waypointPos.z - curPos.z);
		toGoal.normalise(); // Normalize the vector

		// Transform the velocity vector into the car's local coordinate system
		Ogre::Vector3 localVelocity = curOrient.Inverse() * this->physicsActiveComponent->getVelocity();

		// Calculate the car's forward direction vector
		Ogre::Vector3 carForward = curOrient * this->checkpointDefaultDirection; // Assuming forward is along the X axis
		carForward.normalise(); // Normalize the vector

		// Calculate the goal's forward direction vector
		Ogre::Vector3 goalForward = waypointOrient * this->checkpointDefaultDirection; // Assuming forward is along the Z axis
		goalForward.normalise(); // Normalize the vector

		// Calculate the angle between the car's forward vector and the goal's forward vector
		Ogre::Real forwardAlignmentAngle = std::acos(carForward.dotProduct(goalForward)) * (180.0f / Ogre::Math::PI); // Convert radians to degrees

		// Calculate the angle between the car's backward vector and the goal's forward vector
		Ogre::Vector3 carBackward = -carForward;
		Ogre::Real backwardAlignmentAngle = std::acos(carBackward.dotProduct(goalForward)) * (180.0f / Ogre::Math::PI); // Convert radians to degrees

		// Check if the car's movement is in the direction of the car's forward vector
		bool movingForward = localVelocity.x > 0;
		bool movingBackward = localVelocity.x < 0;

		const Ogre::Real toleranceDegrees = 65.0f;

		// Check if the car is moving towards the goal and within the tolerance
		bool isMovingCorrectly = false;
		if (movingForward && forwardAlignmentAngle <= toleranceDegrees)
		{
			// Moving forward and within tolerance
			isMovingCorrectly = true;
		}
		else if (movingBackward && backwardAlignmentAngle <= toleranceDegrees)
		{
			// Moving backward and within tolerance
			isMovingCorrectly = true;
		}

		this->oldDirection = this->wrongDirection;
		this->wrongDirection = !isMovingCorrectly;

		// Ensure the car is not moving in the wrong direction
		if (false == isMovingCorrectly)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "isMovingTowardsCheckpoint: Car is moving in the wrong direction or not properly aligned. Correcting course.");
			// Prevents spamming, as vehicle will collide and collide again
			if (this->oldDirection != this->wrongDirection)
			{
				if (nullptr != this->gameObjectPtr->getLuaScript())
				{
					this->gameObjectPtr->getLuaScript()->callTableFunction(this->onFeedbackRaceFunctionName->getString(), !this->wrongDirection, this->currentLap, this->lapTimeSec, this->finished);
				}
			}
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "isMovingTowardsCheckpoint: Car is moving correctly!.");
			if (nullptr != this->gameObjectPtr->getLuaScript())
			{
				this->gameObjectPtr->getLuaScript()->callTableFunction(this->onFeedbackRaceFunctionName->getString(), !this->wrongDirection, this->currentLap, this->lapTimeSec, this->finished);
			}
		}

		return isMovingCorrectly;
	}

	Ogre::Real RaceGoalComponent::calculateSpeedInKmh(void)
	{
		/*
		To determine the car's speed in kilometers per hour (km/h) based on its velocity, we need to calculate the magnitude of the car's velocity vector and convert this value from meters per second (m/s) to kilometers per hour.
			The formula for this conversion is:
			speed�(km/h) = speed�(m/s) � 3.6
		*/

		// Calculate the magnitude of the velocity vector (speed in m/s)
		Ogre::Real speedInMps = this->physicsActiveComponent->getVelocity().length();

		// Convert speed from m/s to km/h
		Ogre::Real speed = MathHelper::getInstance()->round(speedInMps * 3.6f, 0);


		return speed;
	}

	bool RaceGoalComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// Can only be added once
		auto raceGoalCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<RaceGoalComponent>());
		if (nullptr != raceGoalCompPtr)
		{
			return false;
		}

		// Check if the entity has at least one animation and no player controller, else animation component is senseless
		auto physicsActiveCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<PhysicsActiveComponent>());
		if (nullptr != physicsActiveCompPtr)
		{
			return false;
		}

		return true;
	}

	void RaceGoalComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (RaceGoalComponent::AttrCheckpointsCount() == attribute->getName())
		{
			this->setCheckpointsCount(attribute->getUInt());
		}
		else if(RaceGoalComponent::AttrLapsCount() == attribute->getName())
		{
			this->setLapsCount(attribute->getUInt());
		}
		else if (RaceGoalComponent::AttrOnFeedbackRaceFunctionName() == attribute->getName())
		{
			this->setOnFeedbackRaceFunctionName(attribute->getString());
		}
		else
		{
			for (size_t i = 0; i < this->checkpoints.size(); i++)
			{
				if (RaceGoalComponent::AttrCheckpoint() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->checkpoints[i]->setValue(attribute->getULong());
				}
			}
		}
	}

	void RaceGoalComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CheckpointsCount"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->checkpointsCount->getUInt())));
		propertiesXML->append_node(propertyXML);

		for (size_t i = 0; i < this->checkpoints.size(); i++)
		{
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "Checkpoint" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->checkpoints[i]->getULong())));
			propertiesXML->append_node(propertyXML);
		}

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "LapsCount"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->lapsCount->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OnFeedbackRaceFunctionName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->onFeedbackRaceFunctionName->getString())));
		propertiesXML->append_node(propertyXML);
	}

	void RaceGoalComponent::setCheckpointsCount(unsigned int checkpointsCount)
	{
		this->checkpointsCount->setValue(checkpointsCount);

		size_t oldSize = this->checkpoints.size();

		if (checkpointsCount > oldSize)
		{
			// Resize the waypoints array for count
			this->checkpoints.resize(checkpointsCount);

			for (size_t i = oldSize; i < this->checkpoints.size(); i++)
			{
				this->checkpoints[i] = new Variant(RaceGoalComponent::AttrCheckpoint() + Ogre::StringConverter::toString(i), static_cast<unsigned long>(0), this->attributes, true);
				this->checkpoints[i]->addUserData(GameObject::AttrActionSeparator());
			}
		}
		else if (checkpointsCount < oldSize)
		{
			this->eraseVariants(this->checkpoints, checkpointsCount);
		}
	}

	unsigned int RaceGoalComponent::getCheckpointsCount(void) const
	{
		return this->checkpointsCount->getUInt();
	}

	void RaceGoalComponent::setCheckpointId(unsigned int index, unsigned long id)
	{
		if (index >= this->checkpoints.size())
			return;
		this->checkpoints[index]->setValue(id);
	}

	void RaceGoalComponent::setLapsCount(unsigned int lapsCount)
	{
		this->lapsCount->setValue(lapsCount);
	}

	unsigned int RaceGoalComponent::getLapsCount(void) const
	{
		return this->lapsCount->getUInt();
	}

	bool RaceGoalComponent::getIsFinished(void) const
	{
		return this->finished;
	}

	Ogre::Real RaceGoalComponent::getSpeedInKmh(void) const
	{
		return this->speedInKmh;
	}

	void RaceGoalComponent::setOnFeedbackRaceFunctionName(const Ogre::String& onFeedbackRaceFunctionName)
	{
		this->onFeedbackRaceFunctionName->setValue(onFeedbackRaceFunctionName);
		if (false == onFeedbackRaceFunctionName.empty())
		{
			this->onFeedbackRaceFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), onFeedbackRaceFunctionName + "(wrongDirection, currentLap, lapTimeSec, finished)");
		}
	}


	Ogre::String RaceGoalComponent::getClassName(void) const
	{
		return "RaceGoalComponent";
	}

	Ogre::String RaceGoalComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	// Lua registration part

	RaceGoalComponent* getRaceGoalComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<RaceGoalComponent>(gameObject->getComponentWithOccurrence<RaceGoalComponent>(occurrenceIndex)).get();
	}

	RaceGoalComponent* getRaceGoalComponent(GameObject* gameObject)
	{
		return makeStrongPtr<RaceGoalComponent>(gameObject->getComponent<RaceGoalComponent>()).get();
	}

	RaceGoalComponent* getRaceGoalComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<RaceGoalComponent>(gameObject->getComponentFromName<RaceGoalComponent>(name)).get();
	}

	void RaceGoalComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<RaceGoalComponent, GameObjectComponent>("RaceGoalComponent")
			.def("getCheckpointsCount", &RaceGoalComponent::getCheckpointsCount)
			.def("getSpeedInKmh", &RaceGoalComponent::getSpeedInKmh)
			.def("getLapsCount", &RaceGoalComponent::getLapsCount)
		];

		LuaScriptApi::getInstance()->addClassToCollection("RaceGoalComponent", "class inherits GameObjectComponent", RaceGoalComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("RaceGoalComponent", "number getCheckpointsCount()", "Gets the count of set checkpoints on the race track.");
		LuaScriptApi::getInstance()->addClassToCollection("RaceGoalComponent", "number getSpeedInKmh()", "Gets the current speed in kilometers per hour.");
		LuaScriptApi::getInstance()->addClassToCollection("RaceGoalComponent", "number getLapsCount()", "Gets specified laps count.");

		gameObjectClass.def("getRaceGoalComponentFromName", &getRaceGoalComponentFromName);
		gameObjectClass.def("getRaceGoalComponent", (RaceGoalComponent * (*)(GameObject*)) & getRaceGoalComponent);
		// If its desired to create several of this components for one game object
		gameObjectClass.def("getRaceGoalComponent2", (RaceGoalComponent * (*)(GameObject*, unsigned int)) & getRaceGoalComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "RaceGoalComponent getRaceGoalComponent2(unsigned int occurrenceIndex)", "Gets the component by the given occurence index, since a game object may this component maybe several times.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "RaceGoalComponent getRaceGoalComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "RaceGoalComponent getRaceGoalComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castRaceGoalComponent", &GameObjectController::cast<RaceGoalComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "RaceGoalComponent castRaceGoalComponent(RaceGoalComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

}; //namespace end
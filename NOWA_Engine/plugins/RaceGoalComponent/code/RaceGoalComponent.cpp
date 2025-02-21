#include "NOWAPrecompiled.h"
#include "RaceGoalComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "main/Events.h"
#include "gameobject/GameObjectFactory.h"

#include "OgreAbiUtils.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;


	RaceGoalComponent::CountdownTimer::CountdownTimer(LuaScript* luaScript, const Ogre::String& onCountdownFunctionName)
		: luaScript(luaScript),
		onCountdownFunctionName(onCountdownFunctionName),
		count(3),
		active(false),
		lastUpdate(std::chrono::steady_clock::now())
	{

	}

	void RaceGoalComponent::CountdownTimer::activate(void)
	{
		if (false == this->active)
		{
			this->active = true;
			// Sends event e.g. to purepursuit component, that a countdown is active or not
			boost::shared_ptr<EventDataCountdownActive> eventDataCountdownActive(new EventDataCountdownActive(this->active));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataCountdownActive);
		}
	}

	void RaceGoalComponent::CountdownTimer::update(void)
	{
		if (true == this->active)
		{
			auto now = std::chrono::steady_clock::now();
			if (std::chrono::duration_cast<std::chrono::seconds>(now - lastUpdate).count() >= 2)
			{
				lastUpdate = now;
				count--;
				if (count <= 0)
				{
					this->active = false;
					// Sends event e.g. to purepursuit component, that a countdown is active or not
					boost::shared_ptr<EventDataCountdownActive> eventDataCountdownActive(new EventDataCountdownActive(this->active));
					NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataCountdownActive);
				}
			}
		}
	}

	void RaceGoalComponent::CountdownTimer::display(void)
	{
		if (true == this->active)
		{
			// std::cout << "Countdown: " << count << std::endl;
			if (nullptr != this->luaScript && false == this->onCountdownFunctionName.empty())
			{
				this->luaScript->callTableFunction(this->onCountdownFunctionName, this->count);
			}
		}
		else
		{
			this->luaScript->callTableFunction(this->onCountdownFunctionName, -1);
		}
	}

	bool RaceGoalComponent::CountdownTimer::getIsActive(void) const
	{
		return this->active;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////

	RaceGoalComponent::RaceGoalComponent()
		: GameObjectComponent(),
		name("RaceGoalComponent"),
		finished(false),
		pPath(nullptr),
		vehicleComponent(nullptr),
		purePursuitComponent(nullptr),
		checkpointDefaultDirection(Ogre::Vector3::UNIT_Z),
		currentLap(0),
		lapTimeSec(0.0f),
		wrongDirection(false),
		oldDirection(true),
		speedInKmh(0.0f),
		racingPosition(-1),
		distanceTraveled(0.0f),
		currentWaypointIndex(-1),
		oldCurrentWaypointIndex(-1),
		isReturningToTrack(false),
		returnToTrackStartPosition(Ogre::Vector3::ZERO),
		returnToTrackEndPosition(Ogre::Vector3::ZERO),
		returnToTrackProgress(0.0f),
		returnPhase(LIFT),
		lastKnownDirectionFlag(false),
		countdownTimer(nullptr),
		lapsCount(new Variant(RaceGoalComponent::AttrLapsCount(), static_cast<unsigned int>(1), this->attributes)),
		onFeedbackRaceFunctionName(new Variant(RaceGoalComponent::AttrOnFeedbackRaceFunctionName(), Ogre::String(""), this->attributes)),
		useCountdown(new Variant(RaceGoalComponent::AttrUseCountdown(), false, this->attributes)),
		onCountdownFunctionName(new Variant(RaceGoalComponent::AttrOnCountdownFunctionName(), Ogre::String(""), this->attributes)),
		onWrongDirectionFunctionName(new Variant(RaceGoalComponent::AttrOnWrongDirectionFunctionName(), Ogre::String(""), this->attributes)),
		checkpointsCount(new Variant(RaceGoalComponent::AttrCheckpointsCount(), 0, this->attributes))
	{
		// Since when waypoints count is changed, the whole properties must be refreshed, so that new field may come for way points
		this->checkpointsCount->addUserData(GameObject::AttrActionNeedRefresh());
		this->checkpointsCount->setDescription("Note: Id's must be of game objects with a PhysicsActiveKinematicComponent, which shall be placed as a wall, with which the vehicle does collide for checkpoint determination.");

		this->onFeedbackRaceFunctionName->setDescription("Sets the function name to react in lua script to get feedback whether the vehicle is driving in the wrong direction, or which lap has been passed, the lap time, whether the race is finished. E.g. onRaceFeedback(currentLap, lapTimeSec, finished).");
		this->onFeedbackRaceFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), this->onFeedbackRaceFunctionName->getString() + "(currentLap, lapTimeSec, finished)");

		this->useCountdown->setDescription("Sets whether countdown feature is used. That is, the vehicle can only start after the countdown of 3 seconds. Note: Its also possible to react in lua script for each countdown second, e.g. for printing the number or a beep sound.");
		this->useCountdown->addUserData(GameObject::AttrActionNeedRefresh());

		this->onCountdownFunctionName->setDescription("Sets the function name to react in lua script for the 3 seconds countdown, e.g. for printing the number or a beep sound. If ready to go the countdown number is -1. E.g. onCountDown(countdownNumber).");
		this->onCountdownFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), this->onCountdownFunctionName->getString() + "(countdownNumber)");

		this->onWrongDirectionFunctionName->setDescription("Sets the function name to react in lua script to get feedback whether the vehicle is driving in the wrong direction. E.g. onWrongDirectionDriving(wrongDirection).");
		this->onWrongDirectionFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), this->onWrongDirectionFunctionName->getString() + "(wrongDirection)");
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

	bool RaceGoalComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

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
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseCountdown")
		{
			this->useCountdown->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OnCountdownFunctionName")
		{
			this->onCountdownFunctionName->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OnWrongDirectionFunctionName")
		{
			this->onWrongDirectionFunctionName->setValue(XMLConverter::getAttrib(propertyElement, "data"));
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
		clonedCompPtr->setUseCountdown(this->useCountdown->getBool());
		clonedCompPtr->setOnCountdownFunctionName(this->onCountdownFunctionName->getString());
		clonedCompPtr->setOnWrongDirectionFunctionName(this->onWrongDirectionFunctionName->getString());

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
			this->pPath->setRepeat(true);
		}

		if (true == this->useCountdown->getBool())
		{
			if (nullptr == this->countdownTimer)
			{
				this->countdownTimer = new RaceGoalComponent::CountdownTimer(this->gameObjectPtr->getLuaScript(), this->onCountdownFunctionName->getString());
			}
			this->countdownTimer->activate();
		}

		if (true == this->useCountdown->getBool())
		{
			boost::shared_ptr<EventDataCountdownActive> eventDataCountdownActive(new EventDataCountdownActive(true));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataCountdownActive);
		}

		this->setOnFeedbackRaceFunctionName(this->onFeedbackRaceFunctionName->getString());
		this->setOnWrongDirectionFunctionName(this->onWrongDirectionFunctionName->getString());
		this->setOnCountdownFunctionName(this->onCountdownFunctionName->getString());

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

		// Adds all race goal components (including this one)
		const auto& gameObjectsRG = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsFromComponent(RaceGoalComponent::getStaticClassName());
		for (const auto& gameObjectPtr : gameObjectsRG)
		{
			const auto& raceGoalComponent = NOWA::makeStrongPtr(gameObjectPtr->getComponent<RaceGoalComponent>());
			this->allRaceGoals.emplace_back(raceGoalComponent.get());
		}

		this->currentCheckpoint = this->pPath->getCurrentWaypoint2();

		this->vehicleComponent = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsActiveVehicleComponent>()).get();
		const auto& purePursuitCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PurePursuitComponent>());
		if (nullptr != purePursuitCompPtr)
		{
			this->purePursuitComponent = purePursuitCompPtr.get();;
		}

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

				if (false == this->currentCheckpoint.first)
				{
					return;
				}

				// Driving wrong direction, to not check further for driving towards checkpoints
				if (true == this->wrongDirection)
				{
					return;
				}

				if (true == this->isMovingTowardsCheckpoint())
				{
					if (this->pPath->getCurrentWaypointIndex() + 1 >= this->checkpointsCount->getUInt())
					{
						this->currentLap++;
						if (nullptr != this->gameObjectPtr->getLuaScript() && false == this->onFeedbackRaceFunctionName->getString().empty())
						{
							this->lapTimeSec = MathHelper::getInstance()->round(this->lapTimeSec, 3);
							this->gameObjectPtr->getLuaScript()->callTableFunction(this->onFeedbackRaceFunctionName->getString(), this->currentLap, Ogre::StringConverter::toString(this->lapTimeSec), this->finished);
						}
						if (this->currentLap < this->lapsCount->getUInt())
						{
							this->lapTimeSec = 0.0f;
						}
					}

					this->pPath->setNextWayPoint();
					this->currentCheckpoint = this->pPath->getCurrentWaypoint2();

					if (this->currentLap >= this->lapsCount->getUInt())
					{
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[RaceGoalComponent] Finished track!");
						this->finished = true;
						this->pPath->clear();
						if (nullptr != this->gameObjectPtr->getLuaScript() && false == this->onFeedbackRaceFunctionName->getString().empty())
						{
							this->lapTimeSec = MathHelper::getInstance()->round(this->lapTimeSec, 3);
							this->gameObjectPtr->getLuaScript()->callTableFunction(this->onFeedbackRaceFunctionName->getString(), this->currentLap, Ogre::StringConverter::toString(this->lapTimeSec), this->finished);
							this->lapTimeSec = 0.0f;
						}
					}
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
			this->pPath->setRepeat(true);
		}

		if (true == this->useCountdown->getBool())
		{
			boost::shared_ptr<EventDataCountdownActive> eventDataCountdownActive(new EventDataCountdownActive(false));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataCountdownActive);
		}

		if (nullptr != this->countdownTimer)
		{
			delete this->countdownTimer;
			this->countdownTimer = nullptr;
		}

		this->finished = false;
		for (const auto& kinematicComponent : this->kinematicComponents)
		{
			static_cast<OgreNewt::KinematicBody*>(kinematicComponent->getBody())->removeKinematicContactCallback();
		}

		this->kinematicComponents.clear();
		this->vehicleComponent = nullptr;
		this->purePursuitComponent = nullptr;
		this->allRaceGoals.clear();
		this->lapTimeSec = 0.0f;
		this->speedInKmh = 0.0f;
		this->racingPosition = -1;
		this->distanceTraveled = 0.0f;
		this->currentLap = 0;
		this->currentWaypointIndex = 0;
		this->isReturningToTrack = false;
		this->returnToTrackStartPosition = Ogre::Vector3::ZERO;
		this->returnToTrackEndPosition = Ogre::Vector3::ZERO;
		this->returnToTrackProgress = 0.0f;
		this->returnPhase = LIFT;
		this->lastKnownDirectionFlag = false;

		return true;
	}

	bool RaceGoalComponent::onCloned(void)
	{
		
		return true;
	}

	void RaceGoalComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		if (nullptr != this->pPath)
		{
			delete this->pPath;
			this->pPath = nullptr;
		}
		if (nullptr != this->countdownTimer)
		{
			delete this->countdownTimer;
			this->countdownTimer = nullptr;
		}
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
			if (true == this->useCountdown->getBool())
			{
				this->countdownTimer->update();

				// Only drive if the countdown is over
				this->vehicleComponent->setCanDrive(!this->countdownTimer->getIsActive());
			}

			

			if (nullptr == this->countdownTimer || false == this->countdownTimer->getIsActive())
			{
				this->lapTimeSec += dt;

				this->speedInKmh = this->calculateSpeedInKmh();

				this->oldDirection = this->wrongDirection;
				this->wrongDirection = this->isCarDrivingWrongDirection();

				if (this->oldDirection != this->wrongDirection)
				{
					if (nullptr != this->gameObjectPtr->getLuaScript())
					{
						this->gameObjectPtr->getLuaScript()->callTableFunction(this->onWrongDirectionFunctionName->getString(), this->wrongDirection);
					}
				}

				this->calculateDistanceTraveled(dt);

				this->determineRacePositions();
			}

			if (true == this->useCountdown->getBool())
			{
				this->countdownTimer->display();
			}
		}
	}

	bool RaceGoalComponent::isMovingTowardsCheckpoint(void)
	{
		Ogre::Vector3 curPos = this->gameObjectPtr->getPosition();
		Ogre::Quaternion curOrient = this->gameObjectPtr->getOrientation();
		Ogre::Vector3 waypointPos = this->currentCheckpoint.second.first;
		Ogre::Quaternion waypointOrient = this->currentCheckpoint.second.second;

		// Transform the velocity vector into the car's local coordinate system
		Ogre::Vector3 localVelocity = curOrient.Inverse() * this->vehicleComponent->getVelocity();

		// Calculate the car's forward direction vector
		Ogre::Vector3 carForward = curOrient * this->gameObjectPtr->getDefaultDirection(); // Assuming forward is along the X axis
		carForward.normalise(); // Normalize the vector

		// Calculate the goal's forward direction vector
		Ogre::Vector3 goalForward = waypointOrient * this->checkpointDefaultDirection; // Assuming forward is along the X axis
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

		// Ensure the car is not moving in the wrong direction
		if (false == isMovingCorrectly)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "isMovingTowardsCheckpoint: Car is moving in the wrong direction or not properly aligned. Correcting course.");
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "isMovingTowardsCheckpoint: Car is moving correctly!.");
		}

		return isMovingCorrectly;
	}

	Ogre::Real RaceGoalComponent::calculateSpeedInKmh(void)
	{
		/*
		To determine the car's speed in kilometers per hour (km/h) based on its velocity, we need to calculate the magnitude of the car's velocity vector and convert this value from meters per second (m/s) to kilometers per hour.
			The formula for this conversion is:
			speed (km/h) = speed (m/s) × 3.6
		*/

		// Calculate the magnitude of the velocity vector (speed in m/s)
		Ogre::Real speedInMps = this->vehicleComponent->getVelocity().length();

		// Convert speed from m/s to km/h
		Ogre::Real speed = MathHelper::getInstance()->round(speedInMps * 3.6f, 0);


		return speed;
	}

	void RaceGoalComponent::calculateDistanceTraveled(Ogre::Real dt)
	{
		if (nullptr == this->purePursuitComponent)
		{
			return;
		}

		if (false == this->isReturningToTrack)
		{
			// Sum the distances between waypoints up to the current waypoint
			Ogre::Real distance = 0.0f;
			const auto& waypoints = this->purePursuitComponent->getPath()->getWayPoints();
			for (size_t i = 0; i < this->purePursuitComponent->getPath()->getCurrentWaypointIndex(); ++i)
			{
				distance += waypoints[i].first.distance(waypoints[i + 1].first);
			}
			// Add the distance from the current waypoint to the car's position
			/*if (this->purePursuitComponent->getPath()->getCurrentWaypointIndex() < waypoints.size() - 1)
			{
				distance -= waypoints[this->purePursuitComponent->getPath()->getCurrentWaypoint().first.distance(this->vehicleComponent->getPosition());
			}*/

#if 0
			Ogre::Vector2 vehiclePosXZ = Ogre::Vector2(this->vehicleComponent->getPosition().x, this->vehicleComponent->getPosition().z);
			Ogre::Vector2 waypointXZ = Ogre::Vector2(this->purePursuitComponent->getPath()->getCurrentWaypoint().second.x, this->purePursuitComponent->getPath()->getCurrentWaypoint().second.z);
	
			if (vehiclePosXZ.squaredDistance(waypointXZ) > 50.0f * 50.0f)
			{
				this->initiateReturnToTrack();
			}
#endif

			// After finish, waypoint index starts at 0 again, which would let the vehicle be on the last place again^^, hence take the laps count into account
			// But lap is incremented later, hence use the old current waypoint index, if the new one gets smaller as the old one
			this->oldCurrentWaypointIndex = this->currentWaypointIndex;
			this->currentWaypointIndex = (this->currentLap * waypoints.size()) + (this->purePursuitComponent->getPath()->getCurrentWaypointIndex() + 1);
			if (this->currentWaypointIndex < this->oldCurrentWaypointIndex)
			{
				this->currentWaypointIndex = this->oldCurrentWaypointIndex + 1;
			}

			this->distanceTraveled = distance;
		}
		else
		{
			this->updateReturnToTrack(dt);
		}
	}

	unsigned int RaceGoalComponent::getCurrentWaypointIndex(void) const
	{
		return this->currentWaypointIndex;
	}

	void RaceGoalComponent::initiateReturnToTrack(void)
	{
		if (nullptr == this->purePursuitComponent)
		{
			return;
		}

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "-----> initiateReturnToTrack");

		this->isReturningToTrack = true;
		this->returnToTrackStartPosition = this->vehicleComponent->getPosition();
		this->returnToTrackEndPosition = this->purePursuitComponent->getPath()->getCurrentWaypoint().first;
		this->returnToTrackProgress = 0.0f;
	}

	void RaceGoalComponent::updateReturnToTrack(Ogre::Real dt)
	{
		if (true == this->isReturningToTrack)
		{
			const Ogre::Real liftHeight = 5.0f; // Height to lift the car
			const  Ogre::Real returnSpeed = 1.0f; // Speed factor for return
			this->returnToTrackProgress += dt;

			if (this->returnPhase == RaceGoalComponent::LIFT)
			{
				this->vehicleComponent->setVelocity(Ogre::Vector3(0.0f, returnSpeed * liftHeight, 0.0f));

				// Lift the car up
				if (this->returnToTrackProgress >= 1.0f || this->vehicleComponent->getPosition().y > liftHeight)
				{
					this->returnToTrackProgress = 0.0f;
					this->returnPhase = RaceGoalComponent::MOVE;
				}
			}
			else if (this->returnPhase == RaceGoalComponent::MOVE)
			{
				// Move the car horizontally to the next waypoint
				Ogre::Vector3 direction = (this->returnToTrackEndPosition - this->vehicleComponent->getPosition()).normalisedCopy();
				this->vehicleComponent->setVelocity(direction * returnSpeed);

				if (this->vehicleComponent->getPosition().squaredDistance(this->returnToTrackEndPosition) < 1.0f)
				{
					this->isReturningToTrack = false;
				}
			}
		}
	}

	bool RaceGoalComponent::isCarDrivingWrongDirection(void)
	{
		if (nullptr == this->purePursuitComponent)
		{
			return false;
		}

		// Does only make sense for a by player controlled vehicle
		if (false == this->purePursuitComponent->getIsManuallyControlled())
		{
			return false;
		}

		if (this->vehicleComponent->getVelocity().squaredLength() < 1.0f)
		{
			return false;
		}

		std::pair<Ogre::Vector3, Ogre::Quaternion> currentWaypoint;
		bool foundWaypoint = false;
		unsigned int index = 0;
		unsigned int foundIndex = 0;

		Ogre::Vector2 currentPos = Ogre::Vector2(this->vehicleComponent->getPosition().x, this->vehicleComponent->getPosition().z);

		const auto& waypoints = this->purePursuitComponent->getPath()->getWayPoints();
		for (const auto& waypoint : waypoints)
		{
			Ogre::Vector2 wpPos = Ogre::Vector2(waypoint.first.x, waypoint.first.z);

			Ogre::Real distance = currentPos.distance(wpPos);
			if (distance <= this->purePursuitComponent->getLookaheadDistance())
			{
				currentWaypoint = waypoint;
				foundIndex = index;
				foundWaypoint = true;
				break;
			}
			index++;
		}

		if (false == foundWaypoint)
		{
			return this->lastKnownDirectionFlag;
		}

		Ogre::Vector3 curPos = this->gameObjectPtr->getPosition();
		Ogre::Quaternion curOrient = this->gameObjectPtr->getOrientation();

		// Transform the velocity vector into the car's local coordinate system
		Ogre::Vector3 localVelocity = curOrient.Inverse() * this->vehicleComponent->getVelocity();

		// Calculate the car's forward direction vector
		Ogre::Vector3 carForward = curOrient * this->gameObjectPtr->getDefaultDirection(); // Assuming forward is along the X axis
		carForward.normalise(); // Normalize the vector

		// Calculate the goal's forward direction vector
		Ogre::Vector3 goalForward = currentWaypoint.second * this->checkpointDefaultDirection; // Assuming forward is along the X axis
		goalForward.normalise(); // Normalize the vector

		// Calculate the angle between the car's forward vector and the goal's forward vector
		Ogre::Real forwardAlignmentAngle = std::acos(carForward.dotProduct(goalForward)) * (180.0f / Ogre::Math::PI); // Convert radians to degrees

		// Calculate the angle between the car's backward vector and the goal's forward vector
		Ogre::Vector3 carBackward = -carForward;
		Ogre::Real backwardAlignmentAngle = std::acos(carBackward.dotProduct(goalForward)) * (180.0f / Ogre::Math::PI); // Convert radians to degrees

		

		// Check if the car's movement is in the direction of the car's forward vector
		bool movingForward = localVelocity.x > 0;
		bool movingBackward = localVelocity.x < 0;

		const Ogre::Real toleranceDegrees = 90.0f;

		// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[RaceGoalComponent] foundIndex: " + Ogre::StringConverter::toString(foundIndex));
		// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[RaceGoalComponent] forwardAlignmentAngle: " + Ogre::StringConverter::toString(forwardAlignmentAngle));
		bool isMovingWrongDirection = true;
		if (movingForward && forwardAlignmentAngle <= toleranceDegrees)
		{
			isMovingWrongDirection = false;
		}
		else if (movingBackward && backwardAlignmentAngle <= toleranceDegrees)
		{
			isMovingWrongDirection = false;
		}

		this->lastKnownDirectionFlag = isMovingWrongDirection;

		return isMovingWrongDirection;
	}

	void RaceGoalComponent::determineRacePositions(void)
	{
		// Sort the cars by their distance traveled in descending order
		std::sort(this->allRaceGoals.begin(), this->allRaceGoals.end(), [](const RaceGoalComponent* a, const RaceGoalComponent* b)
				  {
					  return a->getCurrentWaypointIndex() > b->getCurrentWaypointIndex();
				  });
		
		for (size_t i = 0; i < this->allRaceGoals.size(); i++)
		{
			// Update the race positions
			const auto& vehicleComponent = this->allRaceGoals[i];
			this->allRaceGoals[i]->setRacingPosition(i + 1);
		}

		if (this->bShowDebugData)
		{
			for (size_t i = 0; i < this->allRaceGoals.size(); i++)
			{

				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[RaceGoalComponent] Racing position for gameObject: " + this->allRaceGoals[i]->getOwner()->getName()
																+ " race position: " + Ogre::StringConverter::toString(this->allRaceGoals[i]->getRacingPosition())
																+ " distance traveled: " + Ogre::StringConverter::toString(this->allRaceGoals[i]->getDistanceTraveled())
																+ " current Waypoint index: " + Ogre::StringConverter::toString(this->currentWaypointIndex));
			}
		}
	}

	Ogre::Vector3 RaceGoalComponent::calculateDirection(const Ogre::Vector3& currentPosition, const Ogre::Vector3& lastPosition)
	{
		Ogre::Vector3 direction = currentPosition - lastPosition;
		direction.y = 0.0f; // Ignore vertical movement
		direction.normalise(); // Normalize direction vector
		return direction;
	}

	bool RaceGoalComponent::isOvertaking(const PhysicsActiveVehicleComponent* vehicleComponent1, const PhysicsActiveVehicleComponent* vehicleComponent2)
	{
		Ogre::Vector3 direction1 = this->calculateDirection(vehicleComponent1->getPosition(), vehicleComponent1->getLastPosition());
		Ogre::Vector3 direction2 = this->calculateDirection(vehicleComponent2->getPosition(), vehicleComponent2->getLastPosition());

		Ogre::Vector3 relativePosition = vehicleComponent1->getPosition() - vehicleComponent2->getPosition();

		// Check if car1 is behind car2 and moving faster in the same direction
		Ogre::Real dotProduct = relativePosition.dotProduct(direction1);
		return dotProduct > 0 && direction1.dotProduct(direction2) < cos(Math::PI / 4); // Check if directions are sufficiently aligned
	}

#if 0
	void RaceGoalComponent::checkForOvertaking(void)
	{
		for (size_t i = 0; i < this->allVehicles.size(); i++)
		{
			for (size_t j = 0; j < this->allVehicles.size(); j++)
			{
				if (this->allVehicles[j]->getDistanceTraveled() > this->allVehicles[i]->getDistanceTraveled())
				{
					// Swap positions
					std::swap(this->allVehicles[i]->getRacingPosition(), this->allVehicles[j]->getRacingPosition());
					std::swap(this->allVehicles[i], this->allVehicles[j]);
				}
			}
		}
	}
#endif

	bool RaceGoalComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// Can only be added once
		auto raceGoalCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<RaceGoalComponent>());
		if (nullptr != raceGoalCompPtr)
		{
			return false;
		}

		auto physicsActiveCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<PhysicsActiveComponent>());
		if (nullptr == physicsActiveCompPtr)
		{
			auto physicsActiveVehicleCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<PhysicsActiveVehicleComponent>());
			if (nullptr == physicsActiveCompPtr)
			{
				return false;
			}
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
		else if (RaceGoalComponent::AttrUseCountdown() == attribute->getName())
		{
			this->setUseCountdown(attribute->getBool());
		}
		else if (RaceGoalComponent::AttrOnCountdownFunctionName() == attribute->getName())
		{
			this->setOnCountdownFunctionName(attribute->getString());
		}
		else if (RaceGoalComponent::AttrOnWrongDirectionFunctionName() == attribute->getName())
		{
			this->setOnWrongDirectionFunctionName(attribute->getString());
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

	void RaceGoalComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UseCountdown"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useCountdown->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OnCountdownFunctionName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->onCountdownFunctionName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OnWrongDirectionFunctionName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->onWrongDirectionFunctionName->getString())));
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
			this->onFeedbackRaceFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), onFeedbackRaceFunctionName + "(currentLap, lapTimeSec, finished)");
		}
	}

	void RaceGoalComponent::setOnWrongDirectionFunctionName(const Ogre::String& onWrongDirectionFunctionName)
	{
		this->onWrongDirectionFunctionName->setValue(onWrongDirectionFunctionName);
		if (false == onWrongDirectionFunctionName.empty())
		{
			this->onWrongDirectionFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), onWrongDirectionFunctionName + "(wrongDirection)");
		}
	}

	void RaceGoalComponent::setUseCountdown(bool useCountdown)
	{
		this->useCountdown->setValue(useCountdown);
		this->onCountdownFunctionName->setVisible(useCountdown);
	}

	bool RaceGoalComponent::getUseCountdown(void) const
	{
		return this->useCountdown->getBool();
	}

	void RaceGoalComponent::setOnCountdownFunctionName(const Ogre::String& onCountdownFunctionName)
	{
		this->onCountdownFunctionName->setValue(onCountdownFunctionName);
		if (false == onCountdownFunctionName.empty())
		{
			this->onCountdownFunctionName->addUserData(GameObject::AttrActionGenerateLuaFunction(), onCountdownFunctionName + "(countdownNumber)");
		}
	}

	void RaceGoalComponent::setRacingPosition(int racingPosition)
	{
		this->racingPosition = racingPosition;
	}

	int RaceGoalComponent::getRacingPosition(void) const
	{
		return this->racingPosition;
	}

	void RaceGoalComponent::setDistanceTraveled(Ogre::Real distanceTraveled)
	{
		this->distanceTraveled = distanceTraveled;
	}

	Ogre::Real RaceGoalComponent::getDistanceTraveled(void) const
	{
		return this->distanceTraveled;
	}

	bool RaceGoalComponent::getCanDrive(void) const
	{
		return this->countdownTimer->getIsActive();
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
			.def("getRacingPosition", &RaceGoalComponent::getRacingPosition)
			.def("getDistanceTraveled", &RaceGoalComponent::getDistanceTraveled)
			.def("getCurrentWaypointIndex", &RaceGoalComponent::getCurrentWaypointIndex)
			.def("getCanDrive", &RaceGoalComponent::getCanDrive)	
		];

		LuaScriptApi::getInstance()->addClassToCollection("RaceGoalComponent", "class inherits GameObjectComponent", RaceGoalComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("RaceGoalComponent", "number getCheckpointsCount()", "Gets the count of set checkpoints on the race track.");
		LuaScriptApi::getInstance()->addClassToCollection("RaceGoalComponent", "number getSpeedInKmh()", "Gets the current speed in kilometers per hour.");
		LuaScriptApi::getInstance()->addClassToCollection("RaceGoalComponent", "number getLapsCount()", "Gets specified laps count.");
		LuaScriptApi::getInstance()->addClassToCollection("RaceGoalComponent", "number getRacingPosition()", "Gets current racing position. If not valid, -1 will be delivered.");
		LuaScriptApi::getInstance()->addClassToCollection("RaceGoalComponent", "number getDistanceTraveled()", "Gets the forward distance traveled on the race. Note: If driving the wrong direction, the distance will be decreased.");
		LuaScriptApi::getInstance()->addClassToCollection("RaceGoalComponent", "number getCurrentWaypointIndex()", "Gets current waypoint index, which can be use to get the whole progress of the lap. Note: if -1, waypoint index is invalid.");
		LuaScriptApi::getInstance()->addClassToCollection("RaceGoalComponent", "boolean getCanDrive()", "Gets whether the vehicle can drive. E.g. is countdown is used, only if the countdown is over, the vehicle shall drive.");


		gameObjectClass.def("getRaceGoalComponentFromName", &getRaceGoalComponentFromName);
		gameObjectClass.def("getRaceGoalComponent", (RaceGoalComponent * (*)(GameObject*)) & getRaceGoalComponent);
		// If its desired to create several of this components for one game object
		gameObjectClass.def("getRaceGoalComponentFromIndex", (RaceGoalComponent * (*)(GameObject*, unsigned int)) & getRaceGoalComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "RaceGoalComponent getRaceGoalComponentFromIndex(unsigned int occurrenceIndex)", "Gets the component by the given occurence index, since a game object may this component maybe several times.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "RaceGoalComponent getRaceGoalComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "RaceGoalComponent getRaceGoalComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castRaceGoalComponent", &GameObjectController::cast<RaceGoalComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "RaceGoalComponent castRaceGoalComponent(RaceGoalComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

}; //namespace end
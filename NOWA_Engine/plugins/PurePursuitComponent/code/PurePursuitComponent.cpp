#include "NOWAPrecompiled.h"
#include "PurePursuitComponent.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"

#include "OgreAbiUtils.h"

int extractNumber(const Ogre::String& name)
{
	size_t i = 0;
	// Skip non-digit characters
	while (i < name.length() && !std::isdigit(name[i]))
	{
		++i;
	}
	// Extract the numeric part
	return std::stoi(name.substr(i));
}

bool compareGameObjectsByName(NOWA::GameObjectPtr a, NOWA::GameObjectPtr b)
{
	// Does not work: Because: Node_1, Node_10, Node_2 etc. but required is: Node_0, Node_1, Node_2...
	// return a->getName() < b->getName();

	int numA = extractNumber(a->getName());
	int numB = extractNumber(b->getName());

	if (numA != numB)
	{
		return numA < numB;
	}
	else
	{
		return a->getName() < b->getName();
	}
}

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	PurePursuitComponent::PurePursuitComponent()
		: GameObjectComponent(),
		name("PurePursuitComponent"),
		lastSteerAngle(0.0f),
		lastMotorForce(0.0f),
		motorForce(0.0f),
		steerAmount(0.0f),
		pitchAmount(0.0f),
		firstTimeSet(true),
		previousDistance(std::numeric_limits<Ogre::Real>::max()),
		previousPosition(0.0f),
		stuckTime(0.0f),
		maxStuckTime(2.0f),
		pPath(nullptr),
		activated(new Variant(PurePursuitComponent::AttrActivated(), true, this->attributes)),
		waypointsCount(new Variant(PurePursuitComponent::AttrWaypointsCount(), 0, this->attributes)),
		repeat(new Variant(PurePursuitComponent::AttrRepeat(), true, this->attributes)),
		lookaheadDistance(new Variant(PurePursuitComponent::AttrLookaheadDistance(), 20.0f, this->attributes)),
		curvatureThreshold(new Variant(PurePursuitComponent::AttrCurvatureThreshold(), 0.1f, this->attributes)),
		maxMotorForce(new Variant(PurePursuitComponent::AttrMaxMotorForce(), 5000.0f, this->attributes)),
		motorForceVariance(new Variant(PurePursuitComponent::AttrMotorForceVariance(), Ogre::Real(500.0f), this->attributes)),
		overtakingMotorForce(new Variant(PurePursuitComponent::AttrOvertakingMotorForce(), Ogre::Real(5000.0f), this->attributes)),
		minMaxSteerAngle(new Variant(PurePursuitComponent::AttrMinMaxSteerAngle(), Ogre::Vector2(-45.0f, 45.0f), this->attributes)),
		checkWaypointY(new Variant(PurePursuitComponent::AttrCheckWaypointY(), false, this->attributes)),
		waypointVariance(new Variant(PurePursuitComponent::AttrWaypointVariance(), Ogre::Real(0.25f), this->attributes)),
		varianceIndividual(new Variant(PurePursuitComponent::AttrVarianceIndividual(), false, this->attributes)),
		obstacleCategory(new Variant(PurePursuitComponent::AttrObstacleCategory(), Ogre::String(""), this->attributes))
	{
		this->activated->setDescription("If activated, the pure pursuit calcluation takes place.");
		// Since when waypoints count is changed, the whole properties must be refreshed, so that new field may come for way points
		this->waypointsCount->addUserData(GameObject::AttrActionNeedRefresh());

		this->lookaheadDistance->setDescription("The lookahead distance parameter determines how far ahead the car should look on the path to calculate the steering angle.");
		this->curvatureThreshold->setDescription("Sets the threashold at which the motorforce shall be adapted.");
		this->maxMotorForce->setDescription("Force applied by the motor.");
		this->motorForceVariance->setDescription("Sets some random motor force variance (-motorForceVariance, +motorForceVariance) which is added to the max motor force.");
		this->overtakingMotorForce->setDescription("Sets the force if a game object comes to close to another one and shall overtaking it. If set to 0, this behavior is not added. E.g. the player itsself shall not have this behavior.");
		this->minMaxSteerAngle->setDescription("The minimum and maximum steer angle");
		this->checkWaypointY->setDescription("Sets whether to check also the y coordinate of the game object when approaching to a waypoint. This could be necessary, if e.g. using a looping, so that the car targets against the waypoint "
											 ", but may be bad, if e.g. a car jumps via a ramp and the waypoint is below, hence he did not hit the waypoint and will travel back. "
											 " This flag can be switched on the fly, e.g. reaching a special waypoint using the getCurrentWaypointIndex function.");
		this->waypointVariance->setDescription("Sets some random waypoint variance radius. This ensures, that if several game objects are moved, there is more intuitive moving chaos involved.");
		this->varianceIndividual->setDescription("Sets whether the variance from attribute shall take place for each waypoint, or for all waypoints the same random variance.");
		this->obstacleCategory->setDescription("Sets one or several categories which may belong to obstacle game objects, which a game object will try to overcome. If empty, not obstacle detection takes place.");
	}

	PurePursuitComponent::~PurePursuitComponent(void)
	{
		
	}

	const Ogre::String& PurePursuitComponent::getName() const
	{
		return this->name;
	}

	void PurePursuitComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<PurePursuitComponent>(PurePursuitComponent::getStaticClassId(), PurePursuitComponent::getStaticClassName());
	}
	
	void PurePursuitComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool PurePursuitComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
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
					this->waypoints[i] = new Variant(PurePursuitComponent::AttrWaypoint() + Ogre::StringConverter::toString(i), XMLConverter::getAttribUnsignedLong(propertyElement, "data", 0), this->attributes);
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
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "LookaheadDistance")
		{
			this->lookaheadDistance->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CurvatureThreshold")
		{
			this->curvatureThreshold->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxMotorForce")
		{
			this->maxMotorForce->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MotorForceVariance")
		{
			this->motorForceVariance->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OvertakingMotorForce")
		{
			this->overtakingMotorForce->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MinMaxSteerAngle")
		{
			this->minMaxSteerAngle->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CheckWaypointY")
		{
			this->checkWaypointY->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WaypointVariance")
		{
			this->waypointVariance->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "VarianceIndividual")
		{
			this->varianceIndividual->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ObstacleCategory")
		{
			this->obstacleCategory->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		

		return true;
	}

	GameObjectCompPtr PurePursuitComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		PurePursuitComponentPtr clonedCompPtr(boost::make_shared<PurePursuitComponent>());
		
		clonedCompPtr->setWaypointsCount(this->waypointsCount->getUInt());

		for (unsigned int i = 0; i < static_cast<unsigned int>(this->waypoints.size()); i++)
		{
			clonedCompPtr->setWaypointId(i, this->waypoints[i]->getULong());
		}
		clonedCompPtr->setRepeat(this->repeat->getBool());
		clonedCompPtr->setLookaheadDistance(this->lookaheadDistance->getReal());
		clonedCompPtr->setCurvatureThreshold(this->curvatureThreshold->getReal());
		clonedCompPtr->setMaxMotorForce(this->maxMotorForce->getReal());
		clonedCompPtr->setMotorForceVariance(this->motorForceVariance->getReal());
		clonedCompPtr->setOvertakingMotorForce(this->overtakingMotorForce->getReal());
		clonedCompPtr->setMinMaxSteerAngle(this->minMaxSteerAngle->getVector2());
		clonedCompPtr->setCheckWaypointY(this->checkWaypointY->getBool());
		clonedCompPtr->setWaypointVariance(this->waypointVariance->getReal());
		clonedCompPtr->setVarianceIndividual(this->varianceIndividual->getBool());
		clonedCompPtr->setObstacleCategory(this->obstacleCategory->getString());
		
		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		// clonedCompPtr->setActivated(this->activated->getBool());
		// Do not call connect inside yet
		clonedCompPtr->activated->setValue(activated);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool PurePursuitComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PurePursuitComponent] Init component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool PurePursuitComponent::connect(void)
	{
		// Reorder again, since silly lua messes up with keys
		
		this->previousPosition = this->gameObjectPtr->getPosition();
		this->previousDistance = std::numeric_limits<Ogre::Real>::max();
		this->stuckTime = 0.0f;

		if (nullptr == this->pPath)
		{
			this->pPath = new Path();
		}

		this->pPath->clear();
	
		for (size_t i = 0; i < this->waypoints.size(); i++)
		{
			if (nullptr == this->waypoints[i])
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[RaceGoalComponent] Error, cannot use waypoints, because there are no check point ids set for game object: " + this->gameObjectPtr->getName());
				return false;
			}

			GameObjectPtr waypointGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->waypoints[i]->getULong());
			if (nullptr != waypointGameObjectPtr)
			{
				auto nodeCompPtr = NOWA::makeStrongPtr(waypointGameObjectPtr->getComponent<NodeComponent>());
				if (nullptr != nodeCompPtr)
				{
					// Add the way points
					this->pPath->addWayPoint(nodeCompPtr->getPosition());
				}
			}
		}

		this->addRandomVarianceToWaypoints();

		if (this->overtakingMotorForce->getReal() > 0.0f)
		{
			const auto& gameObjects = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsFromComponent(PurePursuitComponent::getStaticClassName());
			for (const auto& gameObjectPtr : gameObjects)
			{
				const auto& otherPurePursuitComponent = NOWA::makeStrongPtr(gameObjectPtr->getComponent<PurePursuitComponent>());
				if (gameObjectPtr->getId() != this->gameObjectPtr->getId())
				{
					this->otherPurePursuits.emplace_back(otherPurePursuitComponent.get());
				}
			}
		}

		// In post init not all game objects are known, and so there are maybe no categories yet, so set the categories here
		this->setObstacleCategory(this->obstacleCategory->getString());

		const auto& obstacleGameObjects = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsFromCategoryId(this->categoriesId);
		for (const auto& gameObjectPtr : obstacleGameObjects)
		{
			this->obstacles.emplace_back(gameObjectPtr.get());
		}

		this->pPath->setRepeat(this->repeat->getBool());

		this->highlightWaypoint("RedNoLightingBackground");
		
		return true;
	}

	bool PurePursuitComponent::disconnect(void)
	{
		if (nullptr != this->pPath)
		{
			this->pPath->clear();
		}
		this->firstTimeSet = true;
		this->motorForce = 0.0f;
		this->steerAmount = 0.0f;
		this->pitchAmount = 0.0f;
		this->otherPurePursuits.clear();
		this->obstacles.clear();
		return true;
	}

	bool PurePursuitComponent::onCloned(void)
	{
		// Not necessary as all components will share the waypoint ids!
		return true;
	}

	void PurePursuitComponent::onRemoveComponent(void)
	{
		if (nullptr != this->pPath)
		{
			delete this->pPath;
			this->pPath = nullptr;
		}
	}
	
	void PurePursuitComponent::onOtherComponentRemoved(unsigned int index)
	{
		
	}
	
	void PurePursuitComponent::onOtherComponentAdded(unsigned int index)
	{
		
	}
	
	void PurePursuitComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			if (this->pPath->getWayPoints().size() > 0)
			{
				this->currentWaypoint = this->pPath->getCurrentWaypoint();

				if (currentWaypoint.first)
				{
					// unsigned int index = this->pPath->getCurrentWaypointIndex();
					// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehaviour] currentWaypoint index: " + Ogre::StringConverter::toString(index));
			
					Ogre::Vector3 agentOriginPos = this->gameObjectPtr->getPosition() - this->gameObjectPtr->getBottomOffset();
					Ogre::Vector3 agentOriginPosXZ = agentOriginPos * Ogre::Vector3(1.0f, 0.0f, 1.0f);
					Ogre::Vector3 currentWaypointXZ = currentWaypoint.second * Ogre::Vector3(1.0f, 0.0f, 1.0f);
					Ogre::Real agentOriginPosY = agentOriginPos.y;
					Ogre::Real currentWaypointY = currentWaypoint.second.y;

					Ogre::Real distSqXZ = agentOriginPosXZ.squaredDistance(currentWaypointXZ);

					if (this->pPath->getRemainingWaypoints() > 1)
					{
						if (false == this->checkWaypointY->getBool())
						{
							if (distSqXZ <= this->lookaheadDistance->getReal() * this->lookaheadDistance->getReal())
							{
								this->pPath->setNextWayPoint();
								this->highlightWaypoint("RedNoLightingBackground");
								// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehaviour] setNextWayPoint");
							}
						}
						else
						{
							if (distSqXZ <= this->lookaheadDistance->getReal() * this->lookaheadDistance->getReal() && Ogre::Math::Abs(agentOriginPosY - currentWaypointY) < this->gameObjectPtr->getSize().y)
							{
								this->pPath->setNextWayPoint();
								this->highlightWaypoint("RedNoLightingBackground");
								// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehaviour] setNextWayPoint");
							}
						}
					}
					else
					{
						if (distSqXZ <= this->lookaheadDistance->getReal() * this->lookaheadDistance->getReal())
						{
							// Goalradius should be small when not in fly mode and the y pos comparison is more eased, so that the waypoint goal can be reached
							// as nearest as possible
							this->pPath->setNextWayPoint();
							this->highlightWaypoint("RedNoLightingBackground");
							// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehaviour] setNextWayPoint");
						}
					}

					if (false == this->pPath->isFinished())
					{
						this->currentWaypoint = this->pPath->getCurrentWaypoint();
						if (false == this->currentWaypoint.first)
						{
							return;
						}


						Ogre::Vector3 currentPosition = this->gameObjectPtr->getPosition();

						// Check if the car is stuck
						if (currentPosition.squaredDistance(this->previousPosition) < 0.01f)
						{
							this->stuckTime += dt;
							if (this->stuckTime > this->maxStuckTime)
							{
								// Increase motor force or reset steering angle to recover from being stuck
								// For demonstration, let's reset the steering angle
								// this->steerAmount = 0.0f;
								this->motorForce += 10000.0f;
								if (this->steerAmount > 0.0f)
								{
									this->steerAmount = this->steerAmount - dt * 60.0f;
								}
								else if(steerAmount < 0)
								{
									this->steerAmount = this->steerAmount + dt * 60.0f;
								}
								// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehaviour] stuck: " + Ogre::StringConverter::toString(this->steerAmount));

								return; // Skip further updates until the car is moving again
							}
						}
						else
						{
							this->stuckTime = 0.0f; // Reset stuck time if the car is not stuck
						}

						this->previousPosition = currentPosition;

						Ogre::Vector3 lookaheadPoint = this->currentWaypoint.second;

						// Detect and avoid obstacles
						if (this->detectObstacle(5.0f))
						{
							this->adjustPathAroundObstacle(lookaheadPoint);
						}

						this->pitchAmount = this->calculatePitchAngle(lookaheadPoint);

						this->motorForce = this->calculateMotorForce();

						this->handleGameObjectsToClose();

						this->motorForce = NOWA::MathHelper::getInstance()->lowPassFilter(this->motorForce, this->lastMotorForce, 0.025f);
						this->lastMotorForce = this->motorForce;

						this->steerAmount = this->calculateSteeringAngle(lookaheadPoint);
						this->steerAmount = MathHelper::getInstance()->clamp(this->steerAmount, this->minMaxSteerAngle->getVector2().x, this->minMaxSteerAngle->getVector2().y);

						Ogre::Real currentDistance = currentPosition.distance(lookaheadPoint);
						// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MovingBehaviour] dist: " + Ogre::StringConverter::toString(currentDistance));
						// If the car is driving away from the waypoint, adjust the steering
						if (currentDistance > this->previousDistance)
						{
							this->steerAmount += (currentDistance - this->previousDistance) * 0.1f; // Adjust steering based on the distance increase
							this->steerAmount = MathHelper::getInstance()->clamp(this->steerAmount, this->minMaxSteerAngle->getVector2().x, this->minMaxSteerAngle->getVector2().y);
						}

						this->previousDistance = currentDistance;
						
					}
					else
					{
						// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PurePursuitComponent] Reached the goal! For game object: " + this->gameObjectPtr->getName());
						this->pPath->clear();
					}
				}
			}
	
			// Handle unreachable waypoints
#if 0
			if (this->currentWaypoint > 0 && this->waypointPositions.back().distance(this->gameObjectPtr->getPosition()) < this->lookaheadDistance->getReal() / 2.0f)
			{
				this->waypointPositions.erase(this->waypointPositions.begin()); // Remove the unreachable waypoint
				std::cout << "Removed an unreachable waypoint." << std::endl;
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PurePursuitComponent] Removed an unreachable waypoint. For game object: " + this->gameObjectPtr->getName());
			}
#endif
		}
	}

	void PurePursuitComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (PurePursuitComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (PurePursuitComponent::AttrWaypointsCount() == attribute->getName())
		{
			this->setWaypointsCount(attribute->getUInt());
		}
		else if (PurePursuitComponent::AttrRepeat() == attribute->getName())
		{
			this->setRepeat(attribute->getBool());
		}
		else if (PurePursuitComponent::AttrLookaheadDistance() == attribute->getName())
		{
			this->setLookaheadDistance(attribute->getReal());
		}
		else if (PurePursuitComponent::AttrCurvatureThreshold() == attribute->getName())
		{
			this->setCurvatureThreshold(attribute->getReal());
		}
		else if (PurePursuitComponent::AttrMaxMotorForce() == attribute->getName())
		{
			this->setMaxMotorForce(attribute->getReal());
		}
		else if (PurePursuitComponent::AttrMotorForceVariance() == attribute->getName())
		{
			this->setMotorForceVariance(attribute->getReal());
		}
		else if (PurePursuitComponent::AttrOvertakingMotorForce() == attribute->getName())
		{
			this->setOvertakingMotorForce(attribute->getReal());
		}
		else if (PurePursuitComponent::AttrMinMaxSteerAngle() == attribute->getName())
		{
			this->setMinMaxSteerAngle(attribute->getVector2());
		}
		else if (PurePursuitComponent::AttrCheckWaypointY() == attribute->getName())
		{
			this->setCheckWaypointY(attribute->getBool());
		}
		else if (PurePursuitComponent::AttrWaypointVariance() == attribute->getName())
		{
			this->setWaypointVariance(attribute->getReal());
		}
		else if (PurePursuitComponent::AttrVarianceIndividual() == attribute->getName())
		{
			this->setVarianceIndividual(attribute->getBool());
		}
		else if (PurePursuitComponent::AttrObstacleCategory() == attribute->getName())
		{
			this->setObstacleCategory(attribute->getString());
		}
		else
		{
			for (size_t i = 0; i < this->waypoints.size(); i++)
			{
				if (PurePursuitComponent::AttrWaypoint() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->waypoints[i]->setValue(attribute->getULong());
				}
			}
		}
	}

	void PurePursuitComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "LookaheadDistance"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->lookaheadDistance->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CurvatureThreshold"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->curvatureThreshold->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MaxMotorForce"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxMotorForce->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MotorForceVariance"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->motorForceVariance->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OvertakingMotorForce"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->overtakingMotorForce->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MinMaxSteerAngle"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->minMaxSteerAngle->getVector2())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CheckWaypointY"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->checkWaypointY->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "WaypointVariance"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->waypointVariance->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "VarianceIndividual"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->varianceIndividual->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ObstacleCategory"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->obstacleCategory->getString())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String PurePursuitComponent::getClassName(void) const
	{
		return "PurePursuitComponent";
	}

	Ogre::String PurePursuitComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void PurePursuitComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);

		if (true == activated)
		{
			this->connect();
		}
	}

	bool PurePursuitComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void PurePursuitComponent::setWaypointsCount(unsigned int waypointsCount)
	{
		this->waypointsCount->setValue(waypointsCount);

		size_t oldSize = this->waypoints.size();

		if (waypointsCount > oldSize)
		{
			// Resize the waypoints array for count
			this->waypoints.resize(waypointsCount);

			for (size_t i = oldSize; i < this->waypoints.size(); i++)
			{
				this->waypoints[i] = new Variant(PurePursuitComponent::AttrWaypoint() + Ogre::StringConverter::toString(i), static_cast<unsigned long>(0), this->attributes, true);
				this->waypoints[i]->addUserData(GameObject::AttrActionSeparator());
			}
		}
		else if (waypointsCount < oldSize)
		{
			this->eraseVariants(this->waypoints, waypointsCount);
		}
	}

	unsigned int PurePursuitComponent::getWaypointsCount(void) const
	{
		return this->waypointsCount->getUInt();
	}

	void PurePursuitComponent::setWaypointId(unsigned int index, unsigned long id)
	{
		if (index >= this->waypoints.size())
			return;
		this->waypoints[index]->setValue(id);
	}

	void PurePursuitComponent::setWaypointStrId(unsigned int index, const Ogre::String& id)
	{
		this->setWaypointId(index, Ogre::StringConverter::parseUnsignedLong(id));
	}

	void PurePursuitComponent::addWaypointId(unsigned long id)
	{
		unsigned int count = this->waypointsCount->getUInt();
		this->waypointsCount->setValue(count + 1);
		this->waypoints.resize(count + 1);

		this->waypoints[count] = new Variant(PurePursuitComponent::AttrWaypoint() + Ogre::StringConverter::toString(count), id, this->attributes);
	}

	void PurePursuitComponent::addWaypointStrId(const Ogre::String& id)
	{
		this->addWaypointId(Ogre::StringConverter::parseUnsignedLong(id));
	}

	void PurePursuitComponent::removeWaypointStrId(const Ogre::String& id)
	{
		int foundIndex = -1;
		for (size_t i = 0; i < this->waypoints.size(); i++)
		{
			if (this->waypoints[i]->getULong() == Ogre::StringConverter::parseUnsignedLong(id))
			{
				foundIndex = i;
				break;
			}
		}

		if (-1 != foundIndex)
		{
			this->eraseVariant(this->waypoints, foundIndex);
			this->waypointsCount->setValue(static_cast<unsigned int>(this->waypoints.size()));
		}
	}

	void PurePursuitComponent::prependWaypointStrId(int index, const Ogre::String& id)
	{
		if (index > this->waypoints.size())
			return;

		this->waypointsCount->setValue(static_cast<unsigned int>(this->waypoints.size() + 1));

		auto idVariant = new Variant(PurePursuitComponent::AttrWaypoint() + Ogre::StringConverter::toString(index), id, this->attributes);
		this->waypoints.insert(this->waypoints.begin() + index, idVariant);
	}

	unsigned long PurePursuitComponent::getWaypointId(unsigned int index)
	{
		if (index > this->waypoints.size())
			return 0;
		return this->waypoints[index]->getULong();
	}

	Ogre::String PurePursuitComponent::getWaypointStrId(unsigned int index)
	{
		return Ogre::StringConverter::toString(this->getWaypointId(index));
	}

	void PurePursuitComponent::setRepeat(bool repeat)
	{
		this->repeat->setValue(repeat);
	}

	bool PurePursuitComponent::getRepeat(void) const
	{
		return this->repeat->getBool();
	}

	void PurePursuitComponent::setLookaheadDistance(Ogre::Real lookaheadDistance)
	{
		this->lookaheadDistance->setValue(lookaheadDistance);
	}

	Ogre::Real PurePursuitComponent::getLookaheadDistance(void) const
	{
		return this->lookaheadDistance->getReal();
	}

	void PurePursuitComponent::setCurvatureThreshold(Ogre::Real curvatureThreshold)
	{
		if (curvatureThreshold < 0.0f)
		{
			curvatureThreshold = 0.1f;
		}
		this->curvatureThreshold->setValue(curvatureThreshold);
	}

	Ogre::Real PurePursuitComponent::getCurvatureThreshold(void) const
	{
		return this->curvatureThreshold->getReal();
	}

	void PurePursuitComponent::setMaxMotorForce(Ogre::Real maxMotorForce)
	{
		if (maxMotorForce < 0.0f)
		{
			maxMotorForce = 0.1f;
		}
		this->maxMotorForce->setValue(maxMotorForce);
	}

	Ogre::Real PurePursuitComponent::getMaxMotorForce(void) const
	{
		return this->maxMotorForce->getReal();
	}

	void PurePursuitComponent::setMotorForceVariance(Ogre::Real motorForceVariance)
	{
		this->motorForceVariance->setValue(motorForceVariance);
	}

	Ogre::Real PurePursuitComponent::getMotorForceVariance(void) const
	{
		return this->motorForceVariance->getReal();
	}

	void PurePursuitComponent::setOvertakingMotorForce(Ogre::Real overtakingMotorForce)
	{
		if (overtakingMotorForce < 0.0f)
		{
			overtakingMotorForce = 5000.0f;
		}

		this->overtakingMotorForce->setValue(overtakingMotorForce);
	}

	Ogre::Real PurePursuitComponent::getOvertakingMotorForce(void) const
	{
		return this->overtakingMotorForce->getReal();
	}

	void PurePursuitComponent::setMinMaxSteerAngle(const Ogre::Vector2& minMaxSteerAngle)
	{
		Ogre::Vector2 tempMinMaxSteerAngle = minMaxSteerAngle;
		if (minMaxSteerAngle.x > minMaxSteerAngle.y)
		{
			tempMinMaxSteerAngle.x = -45.0f;
			tempMinMaxSteerAngle.y = 45.0f;
		}
		if (minMaxSteerAngle.x < -80.0f || minMaxSteerAngle.y > 80.0f)
		{
			tempMinMaxSteerAngle.x = -45.0f;
			tempMinMaxSteerAngle.y = 45.0f;
		}
		this->minMaxSteerAngle->setValue(tempMinMaxSteerAngle);
	}

	Ogre::Vector2 PurePursuitComponent::getMinMaxSteerAngle(void) const
	{
		return this->minMaxSteerAngle->getVector2();
	}

	void PurePursuitComponent::setCheckWaypointY(bool checkWaypointY)
	{
		this->checkWaypointY->setValue(checkWaypointY);
	}

	bool PurePursuitComponent::getCheckWaypointY(void) const
	{
		return this->checkWaypointY->getBool();
	}

	void PurePursuitComponent::setWaypointVariance(Ogre::Real waypointVariance)
	{
		this->waypointVariance->setValue(waypointVariance);
	}

	Ogre::Real PurePursuitComponent::getWaypointVariance(void) const
	{
		return this->waypointVariance->getReal();
	}

	void PurePursuitComponent::setVarianceIndividual(bool varianceIndividual)
	{
		this->varianceIndividual->setValue(varianceIndividual);
	}

	bool PurePursuitComponent::getVarianceIndividual(void) const
	{
		return this->varianceIndividual->getBool();
	}

	void PurePursuitComponent::setObstacleCategory(const Ogre::String& obstacleCategory)
	{
		this->obstacleCategory->setValue(obstacleCategory);
		this->categoriesId = AppStateManager::getSingletonPtr()->getGameObjectController()->generateCategoryId(obstacleCategory);
	}

	Ogre::String PurePursuitComponent::getObstacleCategory(void) const
	{
		return this->obstacleCategory->getString();
	}

	Ogre::Real PurePursuitComponent::calculateSteeringAngle(const Ogre::Vector3& lookaheadPoint)
	{
		Ogre::Vector3 currentPos = this->gameObjectPtr->getPosition();

		Ogre::Vector3 direction = (lookaheadPoint - currentPos).normalisedCopy();

		// Transform direction to local space
		Ogre::Quaternion orientation = this->gameObjectPtr->getOrientation().Inverse() * MathHelper::getInstance()->faceDirection(this->gameObjectPtr->getSceneNode(), direction, this->gameObjectPtr->getDefaultDirection());
		Ogre::Real angleToTarget = orientation.getYaw().valueDegrees();

		Ogre::Real steeringAngle = angleToTarget * 0.5f;

		steeringAngle = this->normalizeAngle(steeringAngle);

		if (true == this->firstTimeSet)
		{
			this->lastSteerAngle = steeringAngle;
			this->lastMotorForce = this->calculateMotorForce();
			this->firstTimeSet = false;
		}

		steeringAngle = NOWA::MathHelper::getInstance()->lowPassFilter(steeringAngle, this->lastSteerAngle, 0.025f);

		this->lastSteerAngle = steeringAngle;

		return steeringAngle;
	}

	Ogre::Real PurePursuitComponent::calculatePitchAngle(const Ogre::Vector3& lookaheadPoint)
	{
		Ogre::Vector3 position = this->gameObjectPtr->getPosition();
		Ogre::Vector3 direction = (lookaheadPoint - position).normalisedCopy();

		// Transform direction to local space
		Ogre::Quaternion orientation = this->gameObjectPtr->getOrientation().Inverse() * MathHelper::getInstance()->faceDirection(this->gameObjectPtr->getSceneNode(), direction, this->gameObjectPtr->getDefaultDirection());
		Ogre::Real pitchAngle = orientation.getPitch().valueDegrees();
		return pitchAngle;
	}

	Ogre::Real PurePursuitComponent::calculateCurvature(void)
	{
		if (waypoints.size() < 3)
		{
			return 0.0;
		}

		// Use the next three waypoints to calculate curvature
		std::vector<Ogre::Vector3> points;
		points.push_back(this->gameObjectPtr->getPosition());
		for (size_t i = 0; i < std::min<size_t>(3, this->pPath->getWayPoints().size()); ++i)
		{
			auto wp = this->pPath->getWayPoints()[i];
			points.push_back(wp.first);
		}

		// Fit a curve to the points (simplified method)
		// Calculate the average curvature using the circumscribed circle method
		Ogre::Real totalCurvature = 0.0;
		for (size_t i = 0; i < points.size() - 2; ++i)
		{
			Ogre::Real a = points[i].distance(points[i + 1]);
			Ogre::Real b = points[i + 1].distance(points[i + 2]);
			Ogre::Real c = points[i + 2].distance(points[i]);

			Ogre::Real s = (a + b + c) / 2.0;
			Ogre::Real area = Ogre::Math::Sqrt(s * (s - a) * (s - b) * (s - c));
			Ogre::Real curvature = (4.0f * area) / (a * b * c);

			totalCurvature += curvature;
		}

		return totalCurvature / (points.size() - 2);
	}

	bool PurePursuitComponent::detectObstacle(Ogre::Real detectionRange)
	{
		Ogre::Vector3 curPos = this->gameObjectPtr->getPosition();
		Quaternion curOrientation = this->gameObjectPtr->getOrientation();
		Ogre::Vector3 forwardDir = curOrientation * Vector3::UNIT_X;
		Ogre::Vector3 detectionPoint = curPos + forwardDir * detectionRange;

		for (const auto& obstacle : obstacles)
		{
			Ogre::v1::Entity* entity = obstacle->getMovableObject<Ogre::v1::Entity>();
			if (nullptr != entity)
			{
				const auto& boundingBox = entity->getLocalAabb();
				if (boundingBox.contains(detectionPoint))
				{
					return true;
				}
			}
			else
			{
				Ogre::Item* item = obstacle->getMovableObject<Ogre::Item>();
				if (nullptr != item)
				{
					const auto& boundingBox = item->getLocalAabb();
					if (boundingBox.contains(detectionPoint))
					{
						return true;
					}
				}
			}
		}
		return false;
	}

	void PurePursuitComponent::adjustPathAroundObstacle(Ogre::Vector3& lookaheadPoint)
	{
		Ogre::Quaternion curOrientation = this->gameObjectPtr->getOrientation();
		Ogre::Vector3 forwardDir = curOrientation * Vector3::UNIT_X;
		Ogre::Vector3 rightDir = curOrientation * Vector3::UNIT_Z;

		// Calculate new lookahead point to the right of the obstacle
		lookaheadPoint += rightDir * 5.0f;
	}

	void PurePursuitComponent::addRandomVarianceToWaypoints(void)
	{
		if (0.0f == this->waypointVariance->getReal())
		{
			return;
		}

		if (false == this->varianceIndividual->getBool())
		{
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_real_distribution<Ogre::Real> dis(-this->waypointVariance->getReal(), this->waypointVariance->getReal());

			for (auto& waypoint : this->pPath->getWayPoints())
			{
				waypoint.first.x += dis(gen);
				waypoint.first.z += dis(gen);
			}
		}
		else
		{
			for (auto& waypoint : this->pPath->getWayPoints())
			{
				std::random_device rd;
				std::mt19937 gen(rd());
				std::uniform_real_distribution<Ogre::Real> dis(-this->waypointVariance->getReal(), this->waypointVariance->getReal());

				waypoint.first.x += dis(gen);
				waypoint.first.z += dis(gen);
			}
		}
	}

	void PurePursuitComponent::handleGameObjectsToClose(void)
	{
		if (0.0f == this->overtakingMotorForce->getReal())
		{
			return;
		}

		for (size_t i = 0; i < this->otherPurePursuits.size(); i++)
		{
			// Avoidance or overtaking logic
			
			Ogre::Vector3 otherPos = this->otherPurePursuits[i]->getOwner()->getPosition();
			Ogre::Vector3 curPos = this->gameObjectPtr->getPosition();
			Ogre::Real distanceToOtherGameObject = curPos.distance(otherPos);

			if (distanceToOtherGameObject < this->lookaheadDistance->getReal() * 2.0f)
			{ 
				// If the cars are too close
				// Adjust speed to avoid collision
				if (curPos.squaredDistance(otherPos) < 1.0f)
				{
					this->motorForce -= 5000.0f;
				}
				else
				{
					this->motorForce += 5000.0f;
					if (this->motorForce > this->maxMotorForce->getReal() + 5000.0f)
					{
						this->motorForce = this->maxMotorForce->getReal() + 5000.0f;
					}
				}
			}

			// Ramming behavior
			Ogre::Vector3 toOtherGameObjectDirection = otherPos - curPos;
			if (toOtherGameObjectDirection.length() < this->lookaheadDistance->getReal())
			{
				Ogre::Real ramAngle = 0.3f * (rand() % 2 ? 1 : -1); // Randomly decide to ram left or right
				this->steerAmount += ramAngle;
			}
		}
	}

	void PurePursuitComponent::highlightWaypoint(const Ogre::String& datablockName)
	{
		if (false == this->currentWaypoint.first)
		{
			return;
		}

		if (false == this->bShowDebugData)
		{
			return;
		}

		for (size_t i = 0; i < this->waypoints.size(); i++)
		{
			GameObjectPtr waypointGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->waypoints[i]->getULong());
			if (nullptr != waypointGameObjectPtr)
			{
				auto nodeCompPtr = NOWA::makeStrongPtr(waypointGameObjectPtr->getComponent<NodeComponent>());
				if (nullptr != nodeCompPtr)
				{
					auto datablockAttribute = waypointGameObjectPtr->getAttribute(GameObject::AttrDataBlock() + "0");
					if (nullptr != datablockAttribute)
					{
						this->oldDatablockName = datablockAttribute->getString();
					}
					Ogre::Vector3 pos = nodeCompPtr->getOwner()->getPosition();
					if (true == MathHelper::getInstance()->vector3Equals(this->currentWaypoint.second, pos))
					{
						nodeCompPtr->getOwner()->getMovableObjectUnsafe<Ogre::v1::Entity>()->setDatablock("datablockName");
					}
					else
					{
						nodeCompPtr->getOwner()->getMovableObjectUnsafe<Ogre::v1::Entity>()->setDatablock(this->oldDatablockName);
					}
				}
			}
		}
	}

	Ogre::Real PurePursuitComponent::calculateMotorForce()
	{
		Ogre::Real curvature = this->calculateCurvature();

		if (curvature > this->curvatureThreshold->getReal())
		{
			return this->maxMotorForce->getReal() / (1.0 + curvature); // Reduce force in curves
		}
		else
		{
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_real_distribution<Ogre::Real> dis(-this->motorForceVariance->getReal(), this->motorForceVariance->getReal());

			return this->maxMotorForce->getReal() + dis(gen); // Full force on straight paths
		}
	}

	void PurePursuitComponent::reorderWaypoints(void)
	{
		std::vector<GameObjectPtr> waypointGameObjects;

		for (size_t i = 0; i < this->waypoints.size(); i++)
		{
			GameObjectPtr waypointGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->waypoints[i]->getULong());
			if (nullptr != waypointGameObjectPtr)
			{
				auto nodeCompPtr = NOWA::makeStrongPtr(waypointGameObjectPtr->getComponent<NodeComponent>());
				if (nullptr != nodeCompPtr)
				{
					// Add the way points
					// this->waypointPositions.emplace_back(nodeCompPtr->getPosition());
					waypointGameObjects.emplace_back(nodeCompPtr->getOwner());
				}
			}
		}

		std::sort(waypointGameObjects.begin(), waypointGameObjects.end(), compareGameObjectsByName);
		this->setWaypointsCount(0);

		for (size_t i = 0; i < waypointGameObjects.size(); i++)
		{
			this->addWaypointId(waypointGameObjects[i]->getId());
		}
	}

	Ogre::Real PurePursuitComponent::getMotorForce(void) const
	{
		return this->motorForce;
	}

	Ogre::Real PurePursuitComponent::getSteerAmount(void) const
	{
		return this->steerAmount;
	}

	Ogre::Real PurePursuitComponent::getPitchAmount(void) const
	{
		return this->pitchAmount;
	}

	int PurePursuitComponent::getCurrentWaypointIndex(void)
	{
		if (nullptr != this->pPath && this->pPath->getWayPoints().size() > 0)
		{
			return this->pPath->getCurrentWaypointIndex();
		}
		return -1;
	}

	Ogre::Vector3 PurePursuitComponent::getCurrentWaypoint(void) const
	{
		if (true == this->currentWaypoint.first)
		{
			return this->currentWaypoint.second;
		}
		return Ogre::Vector3::ZERO;
	}

	Ogre::Real PurePursuitComponent::normalizeAngle(Ogre::Real angle)
	{
		while (angle > 90.0f)
		{
			angle -= 2 * 90.0f;
		}
		while (angle < -90.0f)
		{
			angle += 2 * 90.0f;
		}
		return angle;
	}

	// Lua registration part

	PurePursuitComponent* getPurePursuitComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<PurePursuitComponent>(gameObject->getComponentWithOccurrence<PurePursuitComponent>(occurrenceIndex)).get();
	}

	PurePursuitComponent* getPurePursuitComponent(GameObject* gameObject)
	{
		return makeStrongPtr<PurePursuitComponent>(gameObject->getComponent<PurePursuitComponent>()).get();
	}

	PurePursuitComponent* getPurePursuitComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<PurePursuitComponent>(gameObject->getComponentFromName<PurePursuitComponent>(name)).get();
	}

	void setWaypointId(PurePursuitComponent* instance, unsigned int index, const Ogre::String& waypointId)
	{
		instance->setWaypointId(index, Ogre::StringConverter::parseUnsignedLong(waypointId));
	}

	void PurePursuitComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		// Somehow setWaypointId, addWaypointId etc. local functions with string are not called, but instead the ones with unsigned long, hence redirect to the str versions..

		module(lua)
		[
			class_<PurePursuitComponent, GameObjectComponent>("PurePursuitComponent")
			.def("setActivated", &PurePursuitComponent::setActivated)
			.def("isActivated", &PurePursuitComponent::isActivated)
			.def("setWaypointsCount", &PurePursuitComponent::setWaypointsCount)
			.def("getWaypointsCount", &PurePursuitComponent::getWaypointsCount)
			.def("setWaypointId", &PurePursuitComponent::setWaypointStrId)
			.def("addWaypointId", &PurePursuitComponent::addWaypointStrId)
			.def("getWaypointId", &PurePursuitComponent::getWaypointStrId)
			.def("setRepeat", &PurePursuitComponent::setRepeat)
			.def("getRepeat", &PurePursuitComponent::getRepeat)
			.def("setLookaheadDistance", &PurePursuitComponent::setLookaheadDistance)
			.def("getLookaheadDistance", &PurePursuitComponent::getLookaheadDistance)
			.def("setCurvatureThreshold", &PurePursuitComponent::setCurvatureThreshold)
			.def("getCurvatureThreshold", &PurePursuitComponent::getCurvatureThreshold)
			.def("setMaxMotorForce", &PurePursuitComponent::setMaxMotorForce)
			.def("getMaxMotorForce", &PurePursuitComponent::getMaxMotorForce)
			.def("setMotorForceVariance", &PurePursuitComponent::setMotorForceVariance)
			.def("getMotorForceVariance", &PurePursuitComponent::getMotorForceVariance)
			.def("setOvertakingMotorForce", &PurePursuitComponent::setOvertakingMotorForce)
			.def("getOvertakingMotorForce", &PurePursuitComponent::getOvertakingMotorForce)
			.def("setMinMaxSteerAngle", &PurePursuitComponent::setMinMaxSteerAngle)
			.def("getMinMaxSteerAngle", &PurePursuitComponent::getMinMaxSteerAngle)
			.def("setCheckWaypointY", &PurePursuitComponent::setCheckWaypointY)
			.def("getCheckWaypointY", &PurePursuitComponent::getCheckWaypointY)
			.def("setWaypointVariance", &PurePursuitComponent::setWaypointVariance)
			.def("getWaypointVariance", &PurePursuitComponent::getWaypointVariance)
			.def("setVarianceIndividual", &PurePursuitComponent::setVarianceIndividual)
			.def("getVarianceIndividual", &PurePursuitComponent::getVarianceIndividual)
			.def("setObstacleCategory", &PurePursuitComponent::setObstacleCategory)
			.def("getObstacleCategory", &PurePursuitComponent::getObstacleCategory)


			// .def("calculateSteeringAngle", &PurePursuitComponent::calculateSteeringAngle)
			// .def("calculatePitchAngle", &PurePursuitComponent::calculatePitchAngle)
			// .def("calculateMotorForce", &PurePursuitComponent::calculateMotorForce)
			.def("getMotorForce", &PurePursuitComponent::getMotorForce)
			.def("getSteerAmount", &PurePursuitComponent::getSteerAmount)
			.def("getPitchAmount", &PurePursuitComponent::getPitchAmount)
				
			.def("removeWaypointId", &PurePursuitComponent::removeWaypointStrId)
			.def("prependWaypointId", &PurePursuitComponent::prependWaypointStrId)
			.def("reorderWaypoints", &PurePursuitComponent::reorderWaypoints)
			.def("getCurrentWaypointIndex", &PurePursuitComponent::getCurrentWaypointIndex)
			.def("getCurrentWaypoint", &PurePursuitComponent::getCurrentWaypoint)	
		];

		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "class inherits GameObjectComponent", PurePursuitComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not for the pure pursuit calculation.");
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "bool isActivated()", "Gets whether this component is activated.");
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "void setWaypointsCount(int count)", "Sets the way points count.");
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "int getWaypointsCount()", "Gets the way points count.");
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "void setWaypointId(int index, String id)", "Sets the id of the GameObject with the NodeComponent for the given waypoint index.");
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "void addWaypointId(String id)", "Adds the id of the GameObject with the NodeComponent.");
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "String getWaypointId(int index)", "Gets the way point id from the given index.");
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "void setRepeat(bool repeat)", "Sets whether to repeat the path, when the game object reached the last way point.");
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "bool getRepeat()", "Gets whether to repeat the path, when the game object reached the last way point.");
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "void setLookaheadDistance(number lookaheadDistance)", "The lookahead distance parameter determines how far ahead the car should look in meters on the path to calculate the steering angle. Default value is 20 meters.");
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "number getLookaheadDistance()", "Gets the distance how far ahead the car is looking in meters on the path to calculate the steering angle. Default value is 20 meters.");
		
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "void setCurvatureThreshold(number curvatureThreshold)", "Sets the threashold at which the motorforce shall be adapted. Default value is 0.1.");
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "number getCurvatureThreshold()", "Gets the threashold at which the motorforce shall be adapted. Default value is 0.1.");
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "void setMaxMotorForce(number maxMotorForce)", "Sets the Force applied by the motor. Default value is 5000N.");
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "number getMaxMotorForce()", "Gets the Force applied by the motor. Default value is 5000N.");
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "void setMotorForceVariance(number motorForceVariance)", "Sets some random motor force variance (-motorForceVariance, +motorForceVariance) which is added to the max motor force.");
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "number getMotorForceVariance()", "Gets the motor force variance (-motorForceVariance, +motorForceVariance) which is added to the max motor force.");
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "void setOvertakingMotorForce(number overtakingMotorForce)", "Sets the force if a game object comes to close to another one and shall overtaking it. "
														  "If set to 0, this behavior is not added. E.g. the player itsself shall not have this behavior.");
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "number sgetOvertakingMotorForce()", "Gets the force if a game object comes to close to another one and shall overtaking it. "
														  "If 0, this behavior is not added. E.g. the player itsself shall not have this behavior.");


		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "void setMinMaxSteerAngle(Vector2 minMaxSteerAngle)", "Sets the minimum and maximum steering angle in degree. Valid values are from -80 to 80. Default is -45 to 45.");
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "Vector2 getMinMaxSteerAngle()", "Gets the minimum and maximum steering angle in degree. Valid values are from -80 to 80. Default is -45 to 45.");
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "void setCheckWaypointY(bool checkWaypointY)", "Sets whether to check also the y coordinate of the game object when approaching to a waypoint. This could be necessary, "
														  " if e.g. using a looping, so that the car targets against the waypoint "
														  ", but may be bad, if e.g. a car jumps via a ramp and the waypoint is below, hence he did not hit the waypoint and will travel back. "
														  " This flag can be switched on the fly, e.g. reaching a special waypoint using the getCurrentWaypointIndex function.");
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "bool getCheckWaypointY()", "Gets whether to check also the y coordinate of the game object when approaching to a waypoint. This could be necessary, "
														  " if e.g. using a looping, so that the car targets against the waypoint "
														  ", but may be bad, if e.g. a car jumps via a ramp and the waypoint is below, hence he did not hit the waypoint and will travel back. "
														  " This flag can be switched on the fly, e.g. reaching a special waypoint using the getCurrentWaypointIndex function.");
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "void setWaypointVariance(number waypointVariance)", "Sets some random waypoint variance radius. This ensures, that if several game objects are moved, there is more intuitive moving chaos involved.");
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "number getWaypointVariance()", "Gets the waypoint variance radius. This ensures, that if several game objects are moved, there is more intuitive moving chaos involved.");
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "void setVarianceIndividual(bool varianceIndividual)", "Sets whether the variance from attribute shall take place for each waypoint, or for all waypoints the same random variance.");
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "bool getVarianceIndividual()", "Gets whether the variance from attribute is taking place for each waypoint, or for all waypoints the same random variance.");

		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "void setObstacleCategory(string obstacleCategory)", "Sets one or several categories which may belong to obstacle game objects, which a game object will try to overcome. "
														  "If empty, not obstacle detection takes place.");
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "string getObstacleCategory()", "Gets one or several categories which may belong to obstacle game objects, which a game object will try to overcome. "
														  "If empty, not obstacle detection takes place.");

		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "number getMotorForce()", "Gets the calculated motor force, which can be applied.");
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "number getSteerAmount()", "Gets the calculated steer angle amount in degrees, which can be applied.");
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "number getPitchAmount()", "Gets the calculated pitch angle amount in degrees, which can be applied.");

		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "number getCurrentWaypointIndex()", "Gets the current waypoint index the game object is approaching, or -1 if there are no waypoints.");
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "Vector3 getCurrentWaypoint()", "Gets the current waypoint vector position the game object is approaching, or Vector3(0, 0, 0) if there are no waypoints.");
		
		
#if 0
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "number calculateSteeringAngle(Vector3 position, number heading)", "The calculate steering angle method computes the necessary steering angle to move towards the lookahead point in the x-y plane. "
																					"Returns the angle in degree.");
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "number calculatePitchAngle(Vector3 position, number pitch)", "The calculate pitch angle method computes the necessary pitch angle to move towards the lookahead point in the y-z plane."
																														"E.g. a flying airplane (or jumping car) whould get besides the heading angle also the pitch angle in order to fly towards the given waypoints. "
																														"Returns the angle in degree.");
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "number calculateMotorForce(Vector3 position)", "Adapts the motor force based on the detected curvature. "
														  "Internally determines the curvature of the path using the change in direction between consecutive waypoints. Returns the adjusted motor force to work with.");
#endif
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "void reorderWaypoints()", "Reorders the waypoints according to the node game object names. This is necessary, since lua may mess up with keys and the first node may be the last one. But order is important, hence call this function, after you add waypoints in lua!");
		

		gameObjectClass.def("getPurePursuitComponentFromName", &getPurePursuitComponentFromName);
		gameObjectClass.def("getPurePursuitComponent", (PurePursuitComponent * (*)(GameObject*)) & getPurePursuitComponent);
		// If its desired to create several of this components for one game object
		gameObjectClass.def("getPurePursuitComponent2", (PurePursuitComponent * (*)(GameObject*, unsigned int)) & getPurePursuitComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PurePursuitComponent getPurePursuitComponent2(unsigned int occurrenceIndex)", "Gets the component by the given occurence index, since a game object may this component maybe several times.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PurePursuitComponent getPurePursuitComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PurePursuitComponent getPurePursuitComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castPurePursuitComponent", &GameObjectController::cast<PurePursuitComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "PurePursuitComponent castPurePursuitComponent(PurePursuitComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool PurePursuitComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// Can only be added once
		auto purePursuitCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<PurePursuitComponent>());
		if (nullptr != purePursuitCompPtr)
		{
			return false;
		}

		return true;
	}

}; //namespace end
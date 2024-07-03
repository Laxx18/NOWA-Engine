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
		currentWaypoint(0),
		lastSteerAngle(0.0f),
		firstTimeSet(true),
		activated(new Variant(PurePursuitComponent::AttrActivated(), true, this->attributes)),
		waypointsCount(new Variant(PurePursuitComponent::AttrWaypointsCount(), 0, this->attributes)),
		repeat(new Variant(PurePursuitComponent::AttrRepeat(), true, this->attributes)),
		lookaheadDistance(new Variant(PurePursuitComponent::AttrLookaheadDistance(), 20.0f, this->attributes)),
		curvatureThreshold(new Variant(PurePursuitComponent::AttrCurvatureThreshold(), 0.1f, this->attributes)),
		maxMotorForce(new Variant(PurePursuitComponent::AttrMaxMotorForce(), 5000.0f, this->attributes)),
		minMaxSteerAngle(new Variant(PurePursuitComponent::AttrMinMaxSteerAngle(), Ogre::Vector2(-45.0f, 45.0f), this->attributes))
	{
		this->activated->setDescription("If activated, the pure pursuit calcluation takes place.");
		// Since when waypoints count is changed, the whole properties must be refreshed, so that new field may come for way points
		this->waypointsCount->addUserData(GameObject::AttrActionNeedRefresh());

		this->lookaheadDistance->setDescription("The lookahead distance parameter determines how far ahead the car should look on the path to calculate the steering angle.");
		this->curvatureThreshold->setDescription("Sets the threashold at which the motorforce shall be adapted.");
		this->maxMotorForce->setDescription("Force applied by the motor.");
		this->minMaxSteerAngle->setDescription("The minimum and maximum steer angle");
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
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MinMaxSteerAngle")
		{
			this->minMaxSteerAngle->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		return true;
	}

	GameObjectCompPtr PurePursuitComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		PurePursuitComponentPtr clonedCompPtr(boost::make_shared<PurePursuitComponent>());

		clonedCompPtr->setActivated(this->activated->getBool());
		
		clonedCompPtr->setWaypointsCount(this->waypointsCount->getUInt());

		for (unsigned int i = 0; i < static_cast<unsigned int>(this->waypoints.size()); i++)
		{
			clonedCompPtr->setWaypointId(i, this->waypoints[i]->getULong());
		}
		clonedCompPtr->setRepeat(this->repeat->getBool());
		clonedCompPtr->setLookaheadDistance(this->lookaheadDistance->getReal());
		clonedCompPtr->setCurvatureThreshold(this->curvatureThreshold->getReal());
		clonedCompPtr->setMaxMotorForce(this->maxMotorForce->getReal());
		clonedCompPtr->setMinMaxSteerAngle(this->minMaxSteerAngle->getVector2());
		
		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

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
		// Hello World test :)
		/*auto physicsActiveCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsActiveComponent>());
		if (nullptr != physicsActiveCompPtr)
		{
			physicsActiveCompPtr->applyRequiredForceForVelocity(Ogre::Vector3(0.0f, 10.0f, 0.0f));
		}
		else
		{
			this->gameObjectPtr->getSceneNode()->translate(Ogre::Vector3(0.0f, 10.0f, 0.0f));
		}*/

		// Reorder again, since silly lua messes up with keys
		
		this->waypointPositions.clear();
		
		for (size_t i = 0; i < this->waypoints.size(); i++)
		{
			GameObjectPtr waypointGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->waypoints[i]->getULong());
			if (nullptr != waypointGameObjectPtr)
			{
				auto nodeCompPtr = NOWA::makeStrongPtr(waypointGameObjectPtr->getComponent<NodeComponent>());
				if (nullptr != nodeCompPtr)
				{
					// Add the way points
					this->waypointPositions.emplace_back(nodeCompPtr->getPosition());
				}
			}
		}


#if 0
		// Last waypoint is the first one
		GameObjectPtr waypointGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->waypoints[0]->getULong());
		if (nullptr != waypointGameObjectPtr)
		{
			auto nodeCompPtr = NOWA::makeStrongPtr(waypointGameObjectPtr->getComponent<NodeComponent>());
			if (nullptr != nodeCompPtr)
			{
				this->waypointPositions.emplace_back(nodeCompPtr->getPosition());
			}
		}
#endif
		
		return true;
	}

	bool PurePursuitComponent::disconnect(void)
	{
		this->waypointPositions.clear();
		this->currentWaypoint = 0;
		this->firstTimeSet = true;
		return true;
	}

	bool PurePursuitComponent::onCloned(void)
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

	void PurePursuitComponent::onRemoveComponent(void)
	{
		
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
			if (this->waypointPositions[this->currentWaypoint].distance(this->gameObjectPtr->getPosition()) < this->lookaheadDistance->getReal())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PurePursuitComponent] Reached waypoint: " + Ogre::StringConverter::toString(this->currentWaypoint) + " for game object : " + this->gameObjectPtr->getName());
				this->currentWaypoint++;
			}

			// Check if the car has reached the last waypoint
			if (this->currentWaypoint == this->waypointPositions.size() - 1 && this->waypointPositions.back().distance(this->gameObjectPtr->getPosition()) < this->lookaheadDistance->getReal())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PurePursuitComponent] Reached the goal! For game object: " + this->gameObjectPtr->getName());
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
		else if (PurePursuitComponent::AttrMinMaxSteerAngle() == attribute->getName())
		{
			this->setMinMaxSteerAngle(attribute->getVector2());
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MinMaxSteerAngle"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->minMaxSteerAngle->getVector2())));
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

	Ogre::Real PurePursuitComponent::calculateSteeringAngle(const Ogre::Vector3& position, Ogre::Real heading)
	{
		const Ogre::Vector3& lookaheadPoint = this->findLookaheadPoint(position);
		Ogre::Real angleToLookaheadPoint = Ogre::Math::ATan2(lookaheadPoint.z - position.z, lookaheadPoint.x - position.x).valueRadians();
		Ogre::Real steeringAngle = angleToLookaheadPoint - heading;
		// steeringAngle = this->normalizeAngle(steeringAngle);

		// Clamp the steering angle between -45 and 45 degrees
		
		// steeringAngle = std::max(std::min(steeringAngle, Ogre::Degree(this->minMaxSteerAngle->getVector2().y).valueRadians()), Ogre::Degree(this->minMaxSteerAngle->getVector2().y).valueRadians());
		steeringAngle = MathHelper::getInstance()->clamp(steeringAngle, Ogre::Degree(this->minMaxSteerAngle->getVector2().x).valueRadians(), Ogre::Degree(this->minMaxSteerAngle->getVector2().y).valueRadians());

		if (true == this->firstTimeSet)
		{
			this->lastSteerAngle = steeringAngle;
			this->firstTimeSet = false;
		}

		steeringAngle = NOWA::MathHelper::getInstance()->lowPassFilter(steeringAngle, this->lastSteerAngle, 0.025f);

		this->lastSteerAngle = steeringAngle;

		return Ogre::Math::RadiansToDegrees(steeringAngle);
	}

	Ogre::Real PurePursuitComponent::calculatePitchAngle(const Ogre::Vector3& position, Ogre::Real pitch)
	{
		const Ogre::Vector3& lookaheadPoint = this->findLookaheadPoint(position);
		Ogre::Real distance = std::hypot(lookaheadPoint.y - position.y, lookaheadPoint.x - position.x);
		Ogre::Real angleToLookaheadPoint = Ogre::Math::ATan2(lookaheadPoint.z - position.z, distance).valueRadians();
		Ogre::Real pitchAngle = angleToLookaheadPoint - pitch;
		// pitchAngle = this->normalizeAngle(pitchAngle);

		return Ogre::Math::RadiansToDegrees(pitchAngle);
	}

	Ogre::Real PurePursuitComponent::calculateCurvature(const Ogre::Vector3& position)
	{
		if (this->waypointPositions.size() < 3)
		{
			return 0.0f;
		}

		Ogre::Vector3 p1 = position;
		Ogre::Vector3 p2 = this->waypointPositions[0];
		Ogre::Vector3 p3 = this->waypointPositions[1];

		Ogre::Real a = p1.distance(p2);
		Ogre::Real b = p2.distance(p3);
		Ogre::Real c = p3.distance(p1);

		Ogre::Real s = (a + b + c) / 2.0;
		Ogre::Real area = sqrt(s * (s - a) * (s - b) * (s - c));

		Ogre::Real curvature = (4.0 * area) / (a * b * c);
		return curvature;
	}

	Ogre::Real PurePursuitComponent::calculateMotorForce(const Ogre::Vector3& position)
	{
		Ogre::Real curvature = this->calculateCurvature(position);

		if (curvature > this->curvatureThreshold->getReal())
		{
			return this->maxMotorForce->getReal() / (1.0 + curvature); // Reduce force in curves
		}
		else
		{
			return this->maxMotorForce->getReal(); // Full force on straight paths
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

	Ogre::Vector3 PurePursuitComponent::findLookaheadPoint(const Ogre::Vector3& position)
	{
#if 1
		// Ki Path einbauen?
		// Hier noch rein mit currentWaypoint, sodass nicht auf alte gezeigt wird!
		for (const auto& point : this->waypointPositions)
		{
			Ogre::Real distance = point.distance(position);
			if (distance >= this->lookaheadDistance->getReal())
			{
				return point;
			}
		}
		// If no point is far enough, return the last waypoint
		return this->waypointPositions.back(); 
#else
		for (const auto& point : this->waypointPositions)
		{
			Ogre::Vector2 point2D(point.x, point.z);
			Ogre::Vector2 position2D(position.x, position.z);
			Ogre::Real distance = point2D.distance(position2D);
			if (distance >= this->lookaheadDistance->getReal())
			{
				return point;
			}
		}
		// If no point is far enough, return the last waypoint
		Ogre::Vector3 lastWaypoint(this->waypointPositions.back().x, 0.0f, this->waypointPositions.back().z);
		return lastWaypoint;
#endif
	}

	Ogre::Real PurePursuitComponent::normalizeAngle(Ogre::Real angle)
	{
		while (angle > Ogre::Math::PI)
		{
			angle -= 2 * Ogre::Math::PI;
		}
		while (angle < -Ogre::Math::PI)
		{
			angle += 2 * Ogre::Math::PI;
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
			.def("setMinMaxSteerAngle", &PurePursuitComponent::setMinMaxSteerAngle)
			.def("getMinMaxSteerAngle", &PurePursuitComponent::getMinMaxSteerAngle)
			.def("calculateSteeringAngle", &PurePursuitComponent::calculateSteeringAngle)
			.def("calculatePitchAngle", &PurePursuitComponent::calculatePitchAngle)
			.def("calculateMotorForce", &PurePursuitComponent::calculateMotorForce)
			.def("removeWaypointId", &PurePursuitComponent::removeWaypointStrId)
			.def("prependWaypointId", &PurePursuitComponent::prependWaypointStrId)
			.def("reorderWaypoints", &PurePursuitComponent::reorderWaypoints)
				
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
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "void setMaxMotorForce(number maxMotorForce)", "Sets the Force applied by the motor. Default value is 5000N");
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "number getMaxMotorForce()", "Gets the Force applied by the motor. Default value is 5000N.");
		
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "void setMinMaxSteerAngle(Vector2 minMaxSteerAngle)", "Sets the minimum and maximum steering angle in degree. Valid values are from -80 to 80. Default is -45 to 45.");
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "Vector2 getMinMaxSteerAngle()", "Gets the minimum and maximum steering angle in degree. Valid values are from -80 to 80. Default is -45 to 45.");
		

		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "number calculateSteeringAngle(Vector3 position, number heading)", "The calculate steering angle method computes the necessary steering angle to move towards the lookahead point in the x-y plane. "
																					"Returns the angle in degree.");
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "number calculatePitchAngle(Vector3 position, number pitch)", "The calculate pitch angle method computes the necessary pitch angle to move towards the lookahead point in the y-z plane."
																														"E.g. a flying airplane (or jumping car) whould get besides the heading angle also the pitch angle in order to fly towards the given waypoints. "
																														"Returns the angle in degree.");
		LuaScriptApi::getInstance()->addClassToCollection("PurePursuitComponent", "number calculateMotorForce(Vector3 position)", "Adapts the motor force based on the detected curvature. "
														  "Internally determines the curvature of the path using the change in direction between consecutive waypoints. Returns the adjusted motor force to work with.");

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
		// No constraints so far, just add
		return true;
	}

}; //namespace end
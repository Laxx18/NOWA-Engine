#include "NOWAPrecompiled.h"
#include "NavMeshComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/OgreRecastModule.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	NavMeshComponent::NavMeshComponent()
		: GameObjectComponent(),
		transformUpdateTimer(0.0f),
		oldPosition(Ogre::Vector3::ZERO),
		oldOrientation(Ogre::Quaternion::IDENTITY),
		type(0),
		activated(new Variant(NavMeshComponent::AttrActivated(), true, this->attributes))
	{
		std::vector<Ogre::String> navigationTypes(2);
		navigationTypes[0] = "Static Obstacle";
		navigationTypes[1] = "Dynamic Obstacle";
		this->navigationType = new Variant(NavMeshComponent::AttrNavigationType(), navigationTypes, this->attributes);
		this->navigationType->addUserData(GameObject::AttrActionNeedRefresh());
		this->walkable = new Variant(NavMeshComponent::AttrWalkable(), false, this->attributes);
		this->walkable->setVisible(false);
	}

	NavMeshComponent::~NavMeshComponent()
	{
		AppStateManager::getSingletonPtr()->getOgreRecastModule()->removeStaticObstacle(this->gameObjectPtr->getId());
		AppStateManager::getSingletonPtr()->getOgreRecastModule()->removeDynamicObstacle(this->gameObjectPtr->getId());

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[NavMeshComponent] Destructor navigation mesh component for game object: "
			+ this->gameObjectPtr->getName());
	}

	bool NavMeshComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "NavigationType")
		{
			this->navigationType->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "Obstacle"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Walkable")
		{
			this->walkable->setValue(XMLConverter::getAttribBool(propertyElement, "data", "Obstacle"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr NavMeshComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		NavMeshCompPtr clonedCompPtr(boost::make_shared<NavMeshComponent>());

		
		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);
		// Must be done after game object is set
		clonedCompPtr->setNavigationType(this->navigationType->getListSelectedValue());

		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setWalkable(this->walkable->getBool());

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool NavMeshComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[NavMeshComponent] Init navigation mesh component for game object: "
			+ this->gameObjectPtr->getName());

		this->oldPosition = this->gameObjectPtr->getPosition();
		this->oldOrientation = this->gameObjectPtr->getOrientation();
		
		this->manageNavMesh();
		return true;
	}

	void NavMeshComponent::manageNavMesh(void)
	{
		/*if ("Dynamic Obstacle" == this->oldNavigationType)
		{
			AppStateManager::getSingletonPtr()->getOgreRecastModule()->removeStaticObstacle(this->gameObjectPtr->getId());
		}
		else if ("Static Obstacle" == this->oldNavigationType)
		{
			AppStateManager::getSingletonPtr()->getOgreRecastModule()->removeDynamicObstacle(this->gameObjectPtr->getId());
		}
		*/
		if (this->navigationType->getListSelectedValue() == "Static Obstacle")
		{
			if (true == this->activated->getBool())
			{
				AppStateManager::getSingletonPtr()->getOgreRecastModule()->addStaticObstacle(this->gameObjectPtr->getId());
			}
			else
			{
				AppStateManager::getSingletonPtr()->getOgreRecastModule()->removeStaticObstacle(this->gameObjectPtr->getId());
			}
		}
		else if (this->navigationType->getListSelectedValue() == "Dynamic Obstacle")
		{
			/*if (false == this->oldNavigationType.empty())
			{
				AppStateManager::getSingletonPtr()->getOgreRecastModule()->removeStaticObstacle(this->gameObjectPtr->getId());
			}*/
			AppStateManager::getSingletonPtr()->getOgreRecastModule()->addDynamicObstacle(this->gameObjectPtr->getId(), this->walkable->getBool());

			AppStateManager::getSingletonPtr()->getOgreRecastModule()->activateDynamicObstacle(this->gameObjectPtr->getId(), this->activated->getBool());
		}
	}

	bool NavMeshComponent::connect(void)
	{
		this->manageNavMesh();
		return true;
	}

	bool NavMeshComponent::disconnect(void)
	{
		
		return true;
	}

	void NavMeshComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			this->transformUpdateTimer += dt;
			// Check area only every 5 seconds
			if (this->transformUpdateTimer >= 5.0f)
			{
				this->transformUpdateTimer = 0.0f;

				// Modify position if larger than epsilon
				if (this->gameObjectPtr->getPosition().squaredDistance(this->oldPosition) > 0.1f * 0.1f)
				{
					// Remove obstacle
					auto dataPair = AppStateManager::getSingletonPtr()->getOgreRecastModule()->removeDynamicObstacle(this->gameObjectPtr->getId(), false);
					ConvexVolume* convexVolume = dataPair.second;
					if (nullptr == convexVolume)
						return;

					// Transform hull to new location
					convexVolume->move(this->oldPosition - this->gameObjectPtr->getPosition());

					// Re-add hull as obstacle at new location
					AppStateManager::getSingletonPtr()->getOgreRecastModule()->addDynamicObstacle(this->gameObjectPtr->getId(), this->walkable->getBool());
				}

				// Modify orientation if difference larger than epsilon
				if (false == this->gameObjectPtr->getOrientation().equals(this->oldOrientation, Ogre::Degree(2.0f)))
				{
					// Remove old obstacle from tilecache
					auto dataPair = AppStateManager::getSingletonPtr()->getOgreRecastModule()->removeDynamicObstacle(this->gameObjectPtr->getId(), false);
					InputGeom* inputGeom = dataPair.first;
					ConvexVolume* convexVolume = dataPair.second;
					if (nullptr == inputGeom)
						return;

					// Apply rotation to the inputGeometry and calculate a new 2D convex hull

					// Calculate relative rotation from current rotation to the specified one
					Ogre::Quaternion relativeOrientation = this->oldOrientation * this->gameObjectPtr->getOrientation().Inverse(); 
					// Make sure quaternion is normalized
					relativeOrientation.normalise();   
					// Rotate around obstacle position (center or origin point)
					inputGeom->applyOrientation(relativeOrientation, this->gameObjectPtr->getPosition());   
					delete convexVolume;
					convexVolume = nullptr;

					convexVolume = inputGeom->getConvexHull(AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreRecast()->getAgentRadius());

					if (false == this->walkable->getBool())
						convexVolume->area = RC_NULL_AREA;   // Be sure to set the proper area for the convex shape!
					else
						convexVolume->area = RC_WALKABLE_AREA;

					convexVolume->hmin = convexVolume->hmin - 0.1f;

					// Add new hull as obstacle to tilecache
					AppStateManager::getSingletonPtr()->getOgreRecastModule()->addDynamicObstacle(this->gameObjectPtr->getId(), this->walkable->getBool(), inputGeom, convexVolume);
				}

				this->oldPosition = this->gameObjectPtr->getPosition();
				this->oldOrientation = this->gameObjectPtr->getOrientation();
			}
		}
	}

	void NavMeshComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (NavMeshComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (NavMeshComponent::AttrNavigationType() == attribute->getName())
		{
			this->setNavigationType(attribute->getListSelectedValue());
		}
		else if (NavMeshComponent::AttrWalkable() == attribute->getName())
		{
			this->setWalkable(attribute->getBool());
		}
	}

	void NavMeshComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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

		propertyXML = doc.allocate_node(rapidxml::node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "NavigationType"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->navigationType->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(rapidxml::node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Walkable"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->walkable->getBool())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String NavMeshComponent::getClassName(void) const
	{
		return "NavMeshComponent";
	}

	Ogre::String NavMeshComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void NavMeshComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
		
		if (true == activated)
			this->manageNavMesh();
	}

	bool NavMeshComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void NavMeshComponent::setNavigationType(const Ogre::String& navigationType)
	{
		if (this->oldNavigationType != navigationType)
		{
			this->navigationType->setListSelectedValue(navigationType);
			if ("Dynamic Obstacle" == navigationType)
			{
				this->type = 1;
				this->walkable->setVisible(true);
			}
			else
			{
				this->walkable->setVisible(false);
			}

			this->manageNavMesh();
		}

		this->oldNavigationType = navigationType;
	}

	Ogre::String NavMeshComponent::getNavigationType(void) const
	{
		return this->navigationType->getListSelectedValue();
	}

	void NavMeshComponent::setWalkable(bool walkable)
	{
		this->walkable->setValue(walkable);
		this->manageNavMesh();
	}

	bool NavMeshComponent::getWalkable(void) const
	{
		return this->walkable->getBool();
	}

}; // namespace end
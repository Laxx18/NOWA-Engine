#include "NOWAPrecompiled.h"
#include "NavMeshComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/OgreRecastModule.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	NavMeshComponent::NavMeshComponent() :
        GameObjectComponent(),
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

        // Set descriptions visible in the properties panel
        this->navigationType->setDescription("Static Obstacle: baked into navmesh geometry at build time, requires full rebuild."
                                             "Dynamic Obstacle: convex hull overlay on tile cache, added/removed in milliseconds."
                                             "WARNING: The obstacle shape is a CONVEX HULL of the mesh footprint. The entire"
                                             "enclosed area (including interior) becomes non-walkable. For walkable interiors"
                                             "(e.g. motorhome cabin) a separate interior navmesh with off-mesh connections"
                                             "is required — this is not currently supported by NavMeshComponent alone.");

        this->walkable->setDescription("Only applies to Dynamic Obstacle type."
                                       "false (default): RC_NULL_AREA — agents treat this area as impassable (buildings, walls)."
                                       "true: RC_WALKABLE_AREA — agents can walk through (bridges, ramps, platforms).");
    }

	NavMeshComponent::~NavMeshComponent()
	{
		AppStateManager::getSingletonPtr()->getOgreRecastModule()->removeStaticObstacle(this->gameObjectPtr->getId());
		AppStateManager::getSingletonPtr()->getOgreRecastModule()->removeDynamicObstacle(this->gameObjectPtr->getId());

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[NavMeshComponent] Destructor navigation mesh component for game object: "
			+ this->gameObjectPtr->getName());
	}

	bool NavMeshComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

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
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[NavMeshComponent] Init navigation mesh component for game object: " + this->gameObjectPtr->getName());

        this->oldPosition = this->gameObjectPtr->getPosition();
        this->oldOrientation = this->gameObjectPtr->getOrientation();

        // manageNavMesh() registers via addDynamicObstacle() -> tile cache instantly.
        // No event needed — everything is handled in the fast path.
        this->manageNavMesh();
        return true;
    }

    void NavMeshComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();

        // removeStaticObstacle/removeDynamicObstacle now delegate to the tile cache
        // directly — no event needed here either.
        // The destructor already calls both remove functions, so nothing extra required.
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

		if (false == this->activated->getBool())
		{
			return;
        }

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
        if (false == this->activated->getBool())
        {
            return true;
        }
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
            if (this->transformUpdateTimer >= 5.0f)
            {
                this->transformUpdateTimer = 0.0f;

                const bool posChanged = this->gameObjectPtr->getPosition().squaredDistance(this->oldPosition) > 0.1f * 0.1f;
                const bool rotChanged = !this->gameObjectPtr->getOrientation().equals(this->oldOrientation, Ogre::Degree(2.0f));

                if (posChanged || rotChanged)
                {
                    // Bug fix 1: Single remove for both branches.
                    // The old code had two independent if-blocks each calling
                    // removeDynamicObstacle. If both pos and rot changed, the
                    // first block removed+re-added the obstacle, then the second
                    // block removed the freshly re-added one — operating on a
                    // different InputGeom than the one that was moved.
                    auto dataPair = AppStateManager::getSingletonPtr()->getOgreRecastModule()->removeDynamicObstacle(this->gameObjectPtr->getId(), false);

                    InputGeom* inputGeom = dataPair.first;
                    ConvexVolume* convexVolume = dataPair.second;

                    // Bug fix 2: Early return no longer skips oldPosition/oldOrientation
                    // update. The old code had bare 'return' inside the if-blocks which
                    // skipped the reference update at the bottom, causing the same
                    // (failed) change to be retried every 5 seconds forever.
                    if (nullptr == inputGeom || nullptr == convexVolume)
                    {
                        this->oldPosition = this->gameObjectPtr->getPosition();
                        this->oldOrientation = this->gameObjectPtr->getOrientation();
                        return;
                    }

                    if (posChanged)
                    {
                        // Bug fix 3: Direction was inverted in the original code.
                        // (oldPosition - newPosition) moves the hull backward.
                        // We want to move it forward by the delta the object actually moved.
                        convexVolume->move(this->gameObjectPtr->getPosition() - this->oldPosition);
                    }

                    if (rotChanged)
                    {
                        // Bug fix 4: Relative orientation direction was wrong.
                        // old * new^-1 gives the rotation FROM new BACK TO old.
                        // We want new * old^-1: the rotation applied since last tick.
                        Ogre::Quaternion relativeOrientation = this->gameObjectPtr->getOrientation() * this->oldOrientation.Inverse();
                        relativeOrientation.normalise();

                        // Rotate the raw geometry vertices around the object's current
                        // world position (correct pivot whether or not pos also changed,
                        // because convexVolume->move() already shifted the hull above).
                        inputGeom->applyOrientation(relativeOrientation, this->gameObjectPtr->getPosition());

                        // Bug fix 5: The old position-only branch called
                        // addDynamicObstacle(id, walkable) without passing inputGeom and
                        // convexVolume — so the moved hull was discarded and a brand-new
                        // InputGeom was created from the live entity, wasting the move().
                        // We always recompute the hull from the modified inputGeom here.
                        delete convexVolume;
                        convexVolume = inputGeom->getConvexHull(AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreRecast()->getAgentRadius());

                        convexVolume->area = this->walkable->getBool() ? RC_WALKABLE_AREA : RC_NULL_AREA;
                        convexVolume->hmin -= 0.1f;
                    }

                    // Always re-add with the explicitly modified geometry so nothing
                    // is discarded. Works correctly for pos-only, rot-only, or both.
                    AppStateManager::getSingletonPtr()->getOgreRecastModule()->addDynamicObstacle(this->gameObjectPtr->getId(), this->walkable->getBool(), inputGeom, convexVolume);
                }

                // Bug fix 6: Reference values are updated unconditionally at the end,
                // not buried inside the if-blocks where an early return could skip them.
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

	void NavMeshComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
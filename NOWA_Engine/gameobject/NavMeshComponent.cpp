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

	void NavMeshComponent::update(Ogre::Real dt, bool notSimulating)
    {
        if (false == notSimulating)
        {
            this->transformUpdateTimer += dt;
            if (this->transformUpdateTimer >= 1.0f)
            {
                this->transformUpdateTimer = 0.0f;

                const bool posChanged = this->gameObjectPtr->getPosition().squaredDistance(this->oldPosition) > 0.1f * 0.1f;
                const bool rotChanged = !this->gameObjectPtr->getOrientation().equals(this->oldOrientation, Ogre::Degree(2.0f));

                if (posChanged || rotChanged)
                {
                    auto* recastModule = AppStateManager::getSingletonPtr()->getOgreRecastModule();

                    // Do not update while the tile cache is still processing a previous
                    // request.  Retry on next timer tick — oldPosition is NOT updated
                    // so the change is preserved.
                    if (nullptr != recastModule->getOgreDetourTileCache() && recastModule->getOgreDetourTileCache()->hasPendingObstacles())
                    {
                        return;
                    }

                    if (posChanged && false == rotChanged)
                    {
                        // ── Position-only change ──────────────────────────────────
                        // Destroy the old ConvexVolume entirely (bmin/bmax are stale
                        // — move() does not update them).  addDynamicObstacle without
                        // an external ConvexVolume creates a fresh one from the
                        // current item->getWorldAabb(), so coordinates are correct.
                        recastModule->removeDynamicObstacle(this->gameObjectPtr->getId(), true);
                        recastModule->addDynamicObstacle(this->gameObjectPtr->getId(), this->walkable->getBool());
                    }
                    else
                    {
                        // ── Rotation (with or without position) ──────────────────
                        // Take ownership of the existing geometry.
                        auto dataPair = recastModule->removeDynamicObstacle(this->gameObjectPtr->getId(), false);
                        InputGeom* inputGeom = dataPair.first;
                        ConvexVolume* convexVolume = dataPair.second;

                        if (nullptr == inputGeom)
                        {
                            // No InputGeom available (loaded-from-disk path).
                            // Fall back to fresh AABB — same as the position-only case.
                            delete convexVolume;
                            recastModule->addDynamicObstacle(this->gameObjectPtr->getId(), this->walkable->getBool());
                        }
                        else
                        {
                            // Apply translation first, then rotation, on the InputGeom
                            // vertex cloud so applyOrientation() pivots around the
                            // already-moved centre.
                            if (posChanged)
                            {
                                inputGeom->move(this->gameObjectPtr->getPosition() - this->oldPosition);
                            }

                            Ogre::Quaternion rel = this->gameObjectPtr->getOrientation() * this->oldOrientation.Inverse();
                            rel.normalise();
                            inputGeom->applyOrientation(rel, this->gameObjectPtr->getPosition());

                            // getConvexHull() recomputes bmin/bmax from the updated
                            // vertex cloud — correct by construction.
                            delete convexVolume;
                            convexVolume = inputGeom->getConvexHull(recastModule->getOgreRecast()->getAgentRadius() * 2.0f);
                            convexVolume->area = this->walkable->getBool() ? RC_WALKABLE_AREA : RC_NULL_AREA;
                            convexVolume->hmin -= 0.1f;

                            recastModule->addDynamicObstacle(this->gameObjectPtr->getId(), this->walkable->getBool(), inputGeom, convexVolume);
                        }
                    }
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
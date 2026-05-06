#ifndef NAV_MESH_COMPONENT_H
#define NAV_MESH_COMPONENT_H

#include "NOWAPrecompiled.h"
#include "GameObjectComponent.h"
#include "main/Events.h"
#include "utilities/XMLConverter.h"

namespace NOWA
{
    class EXPORTED NavMeshComponent : public GameObjectComponent
    {
    public:
        typedef boost::shared_ptr<GameObject> GameObjectPtr;
        typedef boost::shared_ptr<NavMeshComponent> NavMeshCompPtr;

    public:
        NavMeshComponent();

        virtual ~NavMeshComponent();

        /**
         * @see GameObjectComponent::init
         */
        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

        /**
         * @see GameObjectComponent::postInit
         */
        virtual bool postInit(void) override;

        /**
         * @see GameObjectComponent::onRemoveComponent
         */
        virtual void onRemoveComponent(void);

        /**
         * @see GameObjectComponent::connect
         */
        virtual bool connect(void) override;

        /**
         * @see GameObjectComponent::disconnect
         */
        virtual bool disconnect(void) override;

        /**
         * @see GameObjectComponent::getClassName
         */
        virtual Ogre::String getClassName(void) const override;

        /**
         * @see GameObjectComponent::getParentClassName
         */
        virtual Ogre::String getParentClassName(void) const override;

        /**
         * @see GameObjectComponent::clone
         */
        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

        /**
         * @see GameObjectComponent::getStaticClassId
         */
        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("NavMeshComponent");
        }

        /**
         * @see GameObjectComponent::getStaticClassName
         */
        static Ogre::String getStaticClassName(void)
        {
            return "NavMeshComponent";
        }

        /**
         * @see GameObjectComponent::createStaticApiForLua
         */
        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
        {
        }

        /**
         * @see GameObjectComponent::getStaticInfoText
         */
        static Ogre::String getStaticInfoText(void)
        {
            return "Usage: Marks this game object as an obstacle for agent pathfinding.\n"
                   "Requirements: OgreRecast navigation must be active in the scene.\n"
                   "\n"
                   "Navigation Types:\n"
                   "  Static Obstacle  - For objects that never move (buildings, walls, rocks).\n"
                   "                     Baked into the navmesh geometry at build time.\n"
                   "                     Requires a full navmesh rebuild to add or remove.\n"
                   "  Dynamic Obstacle - For objects that may move at runtime (vehicles, crates).\n"
                   "                     Applied as a convex shape overlay on the tile cache.\n"
                   "                     Added and removed in milliseconds without rebuilding.\n"
                   "                     Updated every 5 seconds if position or rotation changes.\n"
                   "\n"
                   "IMPORTANT - Convex Hull Behavior:\n"
                   "  The obstacle shape is computed as a CONVEX HULL of the mesh footprint.\n"
                   "  This means the smallest convex polygon enclosing the mesh outline is used.\n"
                   "  The ENTIRE enclosed area becomes non-walkable, including any interior space.\n"
                   "  Example: a hollow building is fully blocked — agents cannot enter it.\n"
                   "  Example: an L-shaped mesh gets a rectangular hull that covers the corners.\n"
                   "  This is by design: Recast uses convex polygons for performance.\n"
                   "\n"
                   "LIMITATION - Walkable Interiors (e.g. motorhome, ship cabin):\n"
                   "  If you need agents to navigate INSIDE a moving object, this component\n"
                   "  alone is NOT sufficient. That requires a separate interior navmesh plus\n"
                   "  off-mesh connection portals (entry/exit points) that move with the object.\n"
                   "  This is not currently implemented in NOWA-Engine.\n"
                   "\n"
                   "Walkable flag (Dynamic Obstacle only):\n"
                   "  false = RC_NULL_AREA: agents treat this area as impassable (default).\n"
                   "  true  = RC_WALKABLE_AREA: agents CAN walk through (use for bridges/ramps).";
        }

        /**
         * @see GameObjectComponent::update
         */
        virtual void update(Ogre::Real dt, bool notSimulating = false) override;

        /**
         * @see GameObjectComponent::actualizeValue
         */
        virtual void actualizeValue(Variant* attribute) override;

        /**
         * @see GameObjectComponent::writeXML
         */
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        /**
         * @see GameObjectComponent::setActivated
         */
        virtual void setActivated(bool activated) override;

        /**
         * @see GameObjectComponent::isActivated
         */
        virtual bool isActivated(void) const override;

        /**
         * @brief Sets the navigation obstacle type.
         * @param navigationType "Static Obstacle" or "Dynamic Obstacle".
         *        Static: baked into navmesh geometry, requires full rebuild.
         *        Dynamic: convex hull overlay on tile cache, updated instantly.
         */
        void setNavigationType(const Ogre::String& navigationType);

        Ogre::String getNavigationType(void) const;

        /**
         * @brief Sets whether the obstacle area is walkable.
         * @param walkable false = RC_NULL_AREA (impassable, default for buildings).
         *                 true  = RC_WALKABLE_AREA (passable, use for platforms/ramps).
         * Only relevant for Dynamic Obstacle type.
         */
        void setWalkable(bool walkable);

        bool getWalkable(void) const;

    public:
        static const Ogre::String AttrActivated(void)
        {
            return "Activated";
        }
        static const Ogre::String AttrNavigationType(void)
        {
            return "Navigation Type";
        }
        static const Ogre::String AttrWalkable(void)
        {
            return "Walkable";
        }

    private:
        /**
         * @brief Registers or unregisters this game object as a navmesh obstacle
         *        based on the current NavigationType and Activated state.
         *
         * Static Obstacle:  adds/removes from the static obstacle list used at
         *                   navmesh build time.
         * Dynamic Obstacle: adds/removes a convex hull shape directly to/from
         *                   the running tile cache. No rebuild required.
         *
         * NOTE: The convex hull encloses the ENTIRE mesh footprint including any
         * interior space. This is a Recast limitation — all obstacle areas must be
         * convex polygons. An L-shaped building gets a rectangular hull; a hollow
         * building is fully blocked. If walkable interior is needed (e.g. a ship
         * cabin), a separate interior navmesh with off-mesh connections is required.
         */
        void manageNavMesh(void);

    private:
        Variant* activated;
        Variant* navigationType;
        Variant* walkable;
        Ogre::String oldNavigationType;
        Ogre::Real transformUpdateTimer;
        Ogre::Vector3 oldPosition;
        Ogre::Quaternion oldOrientation;
        unsigned char type;
    };

}; // namespace end

#endif
/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#ifndef PROCEDURAL_BLOCK_COMPONENT_H
#define PROCEDURAL_BLOCK_COMPONENT_H

#include "OgrePlugin.h"
#include "gameobject/GameObjectComponent.h"

namespace NOWA
{
    class PhysicsArtifactComponent;

    /**
     * @class ProceduralBlockComponent
     * @brief A single solid rectangular block, sized in exact meters, for building straight
     *        platforms.
     *
     * The simplest of the procedural mesh components - even simpler than
     * ProceduralThornComponent. There is no Segment Mode, no mouse-driven editing, no undo/redo:
     * every dimension is a typed-in property, and changing one throws the block away and
     * regenerates it. One datablock covers the whole block.
     *
     * SIZE
     *
     * Length and Height are given as whole-meter counts rather than arbitrary Reals - 'Columns'
     * (length, along X) and 'Rows' (height, along Y), one meter each - so a platform built from
     * several of these blocks lines up on exact meter boundaries by construction, the same way a
     * tile-based level would. 'Depth' (along Z) is a plain Real in meters, not row/column based -
     * matching how ProceduralPlatformBoundaryComponent's own Depth is also the one dimension that
     * is never divided into cells.
     *
     * The block is built in the GameObject's local space, with its lower-left-front corner at
     * the origin:
     *
     *   x: 0 .. Columns (meters)
     *   y: 0 .. Rows (meters)
     *   z: -Depth/2 .. +Depth/2 (centred, matching every other component's depth convention)
     *
     * GEOMETRY
     *
     * A plain axis-aligned box, six quads, one datablock. Winding for each face is derived from
     * its intended outward normal via the same self-correcting cross-product check
     * ProceduralPlatformBoundaryComponent::addBoundaryQuad already uses, rather than working out
     * winding order by hand per face - that already-proven approach is reused here on purpose,
     * instead of repeating the manual-winding approach that needed several rounds of fixing on
     * ProceduralThornComponent's cones.
     */
    class EXPORTED ProceduralBlockComponent : public GameObjectComponent, public Ogre::Plugin
    {
    public:
        typedef boost::shared_ptr<ProceduralBlockComponent> ProceduralBlockCompPtr;

    public:
        ProceduralBlockComponent();

        virtual ~ProceduralBlockComponent();

        // Ogre::Plugin
        virtual const Ogre::String& getName() const override;

        virtual void install(const Ogre::NameValuePairList* options) override;

        virtual void shutdown() override;

        virtual void uninstall() override;

        virtual void initialise() override;

        virtual void getAbiCookie(Ogre::AbiCookie& outAbiCookie) override;

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
        virtual void onRemoveComponent(void) override;

        /**
         * @see GameObjectComponent::update
         */
        virtual void update(Ogre::Real dt, bool notSimulating) override;

        /**
         * @see GameObjectComponent::actualizeValue
         */
        virtual void actualizeValue(Variant* attribute) override;

        /**
         * @see GameObjectComponent::writeXML
         */
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        /**
         * @see GameObjectComponent::clone
         */
        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

        virtual Ogre::String getClassName(void) const override
        {
            return "ProceduralBlockComponent";
        }

        virtual Ogre::String getParentClassName(void) const override
        {
            return "GameObjectComponent";
        }

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("ProceduralBlockComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "ProceduralBlockComponent";
        }

        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

        static Ogre::String getStaticInfoText(void)
        {
            return "Usage: Creates a single solid rectangular block, sized in exact meters, for building straight platforms.\n\n"

                   "SIZE:\n"
                   "- 'Columns' is the block's length in whole meters, along the direction of travel (1 column = 1 meter).\n"
                   "- 'Rows' is the block's height in whole meters (1 row = 1 meter).\n"
                   "- 'Depth' is the block's depth in meters, a plain value rather than row/column based.\n"
                   "- Changing any of these regenerates the whole block from scratch. There is no mouse-driven editing\n"
                   "  and no undo/redo for this component - every dimension is typed in.\n\n"

                   "MATERIAL:\n"
                   "- 'Datablock' is the single material used for the whole block.\n"
                   "- 'UV Tiling' scales the texture in meters per repeat.\n\n"

                   "COLLISION:\n"
                   "- Add a PhysicsArtifactComponent to the same game object and the block is rebuilt as a collision\n"
                   "  hull automatically whenever its geometry changes.\n\n"

                   "LUA API:\n"
                   "- getProceduralBlockComponent() on a GameObject returns this component.\n"
                   "- setColumns(c), setRows(r), setDepth(d) set the size.\n"
                   "- setDatablock(name) sets the material.\n";
        }

        static std::optional<NOWA::GameObjectTypeDescriptor> getStaticTypeDescriptor()
        {
            NOWA::GameObjectTypeDescriptor desc;
            desc.type = eType::CUSTOM;
            desc.displayName = "Platform Block";
            desc.meshToDisplay = "Node.mesh";
            desc.needsMeshItem = false;
            desc.enterMeshModifyMode = false;
            desc.autoComponents = {"ProceduralBlockComponent"};
            desc.guardWithPluginCheck = true;
            return desc;
        }

    public:
        static const Ogre::String AttrActivated(void)
        {
            return "Activated";
        }
        static const Ogre::String AttrColumns(void)
        {
            return "Columns";
        }
        static const Ogre::String AttrRows(void)
        {
            return "Rows";
        }
        static const Ogre::String AttrDepth(void)
        {
            return "Depth";
        }
        static const Ogre::String AttrDatablock(void)
        {
            return "Datablock";
        }
        static const Ogre::String AttrUVTiling(void)
        {
            return "UV Tiling";
        }
        static const Ogre::String AttrUseGradient(void)
        {
            return "Use Gradient";
        }
        static const Ogre::String AttrUseBevel(void)
        {
            return "Use Bevel";
        }
        static const Ogre::String AttrBevelSize(void)
        {
            return "Bevel Size";
        }

    public:
        virtual void setActivated(bool activated) override;

        virtual bool isActivated(void) const override;

        void setColumns(int columns);

        int getColumns(void) const;

        void setRows(int rows);

        int getRows(void) const;

        void setDepth(Ogre::Real depth);

        Ogre::Real getDepth(void) const;

        /**
         * @return The block's actual length in meters, i.e. getColumns() * 1.0.
         */
        Ogre::Real getLength(void) const;

        /**
         * @return The block's actual height in meters, i.e. getRows() * 1.0.
         */
        Ogre::Real getHeight(void) const;

        void setDatablock(const Ogre::String& datablock);

        Ogre::String getDatablock(void) const;

        void setUVTiling(const Ogre::Vector2& tiling);

        Ogre::Vector2 getUVTiling(void) const;

        /**
         * @brief Sets whether the block is built as a sloped ramp instead of a straight box.
         * @note	The slope is not a separate angle to configure - it is exactly Rows / Columns
         *			(Height / Length), rising from y = 0 at x = 0 to y = Height at x = Length. Takes
         *			priority over Use Bevel: a ramp's own sloped top edge is not chamfered by this
         *			component.
         */
        void setUseGradient(bool useGradient);

        bool getUseGradient(void) const;

        /**
         * @brief Sets whether the block's top-front and top-back edges are chamfered instead of
         *        sharp, using 'Bevel Size' as the chamfer's extent along both axes it touches.
         * @note	Has no effect while Use Gradient is also true - see setUseGradient().
         */
        void setUseBevel(bool useBevel);

        bool getUseBevel(void) const;

        void setBevelSize(Ogre::Real bevelSize);

        Ogre::Real getBevelSize(void) const;    private:
        // ── Mesh generation ──────────────────────────────────────────────────────────────
        void rebuildMesh(void);

        void addBlockQuad(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& v3, const Ogre::Vector3& normal, Ogre::Real u0, Ogre::Real u1, Ogre::Real v0Coord, Ogre::Real v1Coord);

        void addBlockTriangle(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& normal, const Ogre::Vector2& uv0, const Ogre::Vector2& uv1, const Ogre::Vector2& uv2);

        void addBlockBox(const Ogre::Vector3& minCorner, const Ogre::Vector3& maxCorner);

        /**
         * @brief Emits a chamfered box: a hexagonal cross-section (flat bottom, vertical front
         *        and back walls cut short by 'bevel', flat top inset by 'bevel', two chamfer
         *        strips connecting them) extruded through the full depth.
         */
        void addBlockChamferedBox(Ogre::Real length, Ogre::Real height, Ogre::Real halfDepth, Ogre::Real bevel);

        /**
         * @brief Emits a right-triangular wedge: flat base, vertical back wall at x = length,
         *        and a sloped top surface from (0, 0) to (length, height) - extruded through
         *        the full depth.
         */
        void addBlockWedge(Ogre::Real length, Ogre::Real height, Ogre::Real halfDepth);

        void createBlockMesh(void);

        void createBlockMeshInternal(const std::vector<float>& verts, const std::vector<Ogre::uint32>& inds, size_t numVerts);

        void destroyBlockMesh(void);

        void updatePhysicsCollision(void);

    private:
        Ogre::String name;

        Variant* activated;
        Variant* columns;
        Variant* rows;
        Variant* depth;
        Variant* useGradient;
        Variant* useBevel;
        Variant* bevelSize;
        Variant* datablock;
        Variant* uvTiling;

        // Interleaved layout: pos.xyz, normal.xyz, tangent.xyzw, uv.xy = 12 floats per vertex -
        // the tangent is included from the start, exactly like ProceduralThornComponent, so a
        // normal-mapped datablock does not fail with "Renderable can't use normal maps but
        // datablock wants normal maps".
        std::vector<float> vertices;
        std::vector<Ogre::uint32> indices;
        Ogre::uint32 currentVertexIndex;

        Ogre::Item* blockItem;
        Ogre::String blockMeshName;

        PhysicsArtifactComponent* physicsArtifactComponent;
    };

}; // namespace end

#endif
/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#ifndef PROCEDURAL_THORN_COMPONENT_H
#define PROCEDURAL_THORN_COMPONENT_H

#include "OgrePlugin.h"
#include "gameobject/GameObjectComponent.h"

namespace NOWA
{
    class PhysicsArtifactComponent;

    /**
     * @class ProceduralThornComponent
     * @brief A row of procedurally generated spikes the player must jump over, sized in exact
     *        meters.
     *
     * The simplified, non-interactive sibling of ProceduralPlatformBoundaryComponent - see that
     * class for the general shape this one follows. Everything about editing is dropped on
     * purpose: there is no Segment Mode, no mouse-driven selection, no undo/redo, because a spike
     * strip has nothing to individually select or cut away. Every dimension is a typed-in
     * property; changing one throws the whole strip away and regenerates it, exactly like the
     * boundary component's own dimensions do.
     *
     * GEOMETRY
     *
     * BUGFIX (design correction): the first version built each spike as a triangular-prism
     * "roof" ridge - visually a row of sloped rooftops, not spikes at all (see the screenshot
     * that prompted this rewrite). Replaced with actual cones: a circular base tapering to a
     * single point, which is what "spitze Kegel" (pointed cones) means and reads correctly as a
     * bed of spikes from any angle, not just side-on.
     *
     * Cones are laid out on a 2D GRID spanning both 'Thorn Length' (x) and 'Thorn Width' (z),
     * not a single row - "auffuellen in Tiefe und Laenge" (fill in depth and length). Each cone's
     * base diameter is 'Thorn Base Size' (0.5m by default, i.e. a 0.5 x 0.5m footprint per cone
     * as specified), and that same value sets the grid spacing in both directions, so cones sit
     * base-to-base with no gaps a player could see through and no overlap. The strip is built in
     * the GameObject's local space, with the base circles at y = 0 and the grid's lower-left-front
     * corner at the origin:
     *
     *   x: 0 .. Thorn Length     (direction of travel, same convention as the level boundary)
     *   y: 0 .. Thorn Height     (base at the ground, apex at the top of each cone)
     *   z: -Width/2 .. +Width/2 (centred, matching the boundary's own depth convention)
     *
     * The grid cell counts along x and z are each Length / BaseSize and Width / BaseSize,
     * rounded to the nearest whole cone, with the actual spacing then recomputed per axis so the
     * cones fill the stated Length and Width exactly - the same "last cell absorbs the
     * remainder" approach ProceduralPlatformBoundaryComponent uses for its own doorway cells, so
     * the outer size stays exactly what was typed in.
     *
     * DAMAGE
     *
     * This component only builds the SHAPE and, when a PhysicsArtifactComponent is present, its
     * collision hull - exactly like the boundary component hands its geometry to
     * PhysicsArtifactComponent for collision. It does NOT decide what happens when the player
     * touches a spike: this project's actual contact/damage mechanism (an OgreNewt contact
     * callback, a Lua-side reaction, a dedicated damage component) was not part of what this
     * class was told to assume, and guessing at that API wrong would be worse than leaving it
     * out - see the accompanying reply for what is needed to wire that up.
     */
    class EXPORTED ProceduralThornComponent : public GameObjectComponent, public Ogre::Plugin
    {
    public:
        typedef boost::shared_ptr<ProceduralThornComponent> ProceduralThornCompPtr;

    public:
        ProceduralThornComponent();

        virtual ~ProceduralThornComponent();

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
         * @see GameObjectComponent::onAddComponent
         */
        virtual void onAddComponent(void) override;

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
         * @see GameObjectComponent::isProcedural
         */
        virtual bool isProcedural(void) const override
        {
            return true;
        }

        /**
         * @see GameObjectComponent::clone
         */
        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

        virtual Ogre::String getClassName(void) const override
        {
            return "ProceduralThornComponent";
        }

        virtual Ogre::String getParentClassName(void) const override
        {
            return "GameObjectComponent";
        }

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("ProceduralThornComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "ProceduralThornComponent";
        }

        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

        static Ogre::String getStaticInfoText(void)
        {
            return "Usage: Creates a row of procedural spikes the player must jump over, sized in exact meters.\n\n"

                   "SIZE:\n"
                   "- 'Thorn Length', 'Thorn Width' and 'Thorn Height' are the field's outer dimensions in meters -\n"
                   "  Length along the direction of travel, Width across the fixed depth axis, Height how tall each\n"
                   "  cone stands.\n"
                   "- 'Thorn Base Size' is each cone's base diameter in meters (0.5 x 0.5m by default), and also the\n"
                   "  grid spacing in both directions - cones are laid out on a grid filling the whole Length x\n"
                   "  Width area, base to base with no gaps. The actual grid counts are rounded so the cones fill\n"
                   "  the stated Length and Width exactly.\n"
                   "- Changing any of these regenerates the whole field from scratch. There is no mouse-driven\n"
                   "  editing and no undo/redo for this component - every dimension is typed in.\n\n"

                   "MATERIAL:\n"
                   "- 'Datablock' is the single material used for the whole strip.\n"
                   "- 'UV Tiling' scales the texture in meters per repeat.\n\n"

                   "COLLISION:\n"
                   "- Add a PhysicsArtifactComponent to the same game object and the field is rebuilt as a\n"
                   "  collision hull automatically whenever its geometry changes.\n"
                   "- This component only builds the shape and its collision hull; it does not itself apply\n"
                   "  damage on contact.\n\n"

                   "LUA API:\n"
                   "- getProceduralThornComponent() on a GameObject returns this component.\n"
                   "- setThornLength(l), setThornWidth(w), setThornHeight(h), setThornBaseSize(s) set the size in "
                   "meters.\n"
                   "- setDatablock(name) sets the material.\n";
        }
        
        static std::optional<NOWA::GameObjectTypeDescriptor> getStaticTypeDescriptor()
        {
            NOWA::GameObjectTypeDescriptor desc;
            desc.type = eType::CUSTOM;
            desc.displayName = "Platform Thorn";
            desc.meshToDisplay = "Node.mesh";
            desc.needsMeshItem = false;
            desc.enterMeshModifyMode = false;
            desc.autoComponents = {"ProceduralThornComponent"};
            desc.guardWithPluginCheck = true;
            return desc;
        }

    public:
        static const Ogre::String AttrActivated(void)
        {
            return "Activated";
        }
        static const Ogre::String AttrThornLength(void)
        {
            return "Thorn Length";
        }
        static const Ogre::String AttrThornWidth(void)
        {
            return "Thorn Width";
        }
        static const Ogre::String AttrThornHeight(void)
        {
            return "Thorn Height";
        }
        static const Ogre::String AttrThornBaseSize(void)
        {
            return "Thorn Base Size";
        }
        static const Ogre::String AttrDatablock(void)
        {
            return "Datablock";
        }
        static const Ogre::String AttrUVTiling(void)
        {
            return "UV Tiling";
        }

    public:
        virtual void setActivated(bool activated) override;

        virtual bool isActivated(void) const override;

        void setThornLength(Ogre::Real length);

        Ogre::Real getThornLength(void) const;

        void setThornWidth(Ogre::Real width);

        Ogre::Real getThornWidth(void) const;

        void setThornHeight(Ogre::Real height);

        Ogre::Real getThornHeight(void) const;

        void setThornBaseSize(Ogre::Real baseSize);

        Ogre::Real getThornBaseSize(void) const;

        void setDatablock(const Ogre::String& datablock);

        Ogre::String getDatablock(void) const;

        void setUVTiling(const Ogre::Vector2& tiling);

        Ogre::Vector2 getUVTiling(void) const;

    private:
        // ── Mesh generation ──────────────────────────────────────────────────────────────
        void rebuildMesh(void);

        /**
         * @brief Emits one cone-shaped spike: apex at (centerX, height, centerZ), circular base
         *        of the given radius at y = 0, built from CONE_SEGMENTS flat triangular wedges.
         */
        void addCone(Ogre::Real centerX, Ogre::Real centerZ, Ogre::Real height, Ogre::Real radius);

        void addThornTriangle(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& normal0, const Ogre::Vector3& normal1, const Ogre::Vector3& normal2, const Ogre::Vector3& tangent,
            const Ogre::Vector2& uv0, const Ogre::Vector2& uv1, const Ogre::Vector2& uv2);

        /**
         * @brief Emits one quad, self-correcting its winding from the supplied outward normal -
         *        same approach as ProceduralBlockComponent::addBlockQuad, used here instead of
         *        hand-picking winding order the way addCone's own triangles originally did,
         *        which needed two rounds of fixing before it was right.
         */
        void addThornQuad(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& v3, const Ogre::Vector3& normal, Ogre::Real u0, Ogre::Real u1, Ogre::Real v0Coord, Ogre::Real v1Coord);

        /**
         * @brief Emits the flat base plane the whole spike field sits on: a single quad at
         *        y = 0 spanning the full [0, length] x [-halfWidth, +halfWidth] footprint, same
         *        datablock and UV Tiling as the cones - so it is one continuous submesh with
         *        them, not a separate material.
         */
        void addBasePlane(Ogre::Real length, Ogre::Real halfWidth);

        void createThornMesh(void);

        void createThornMeshInternal(const std::vector<float>& verts, const std::vector<Ogre::uint32>& inds, size_t numVerts);

        void destroyThornMesh(void);

        void updatePhysicsCollision(void);

    private:
        Ogre::String name;

        Variant* activated;
        Variant* thornLength;
        Variant* thornWidth;
        Variant* thornHeight;
        Variant* thornBaseSize;
        Variant* datablock;
        Variant* uvTiling;

        // Interleaved layout: pos.xyz, normal.xyz, tangent.xyzw, uv.xy = 12 floats per vertex.
        // The tangent is carried from the very first version of this component, unlike the
        // boundary component's first draft - a datablock with a normal map otherwise fails to
        // apply with "Renderable can't use normal maps but datablock wants normal maps", the
        // exact failure that hit ProceduralPlatformBoundaryComponent before it had one.
        std::vector<float> vertices;
        std::vector<Ogre::uint32> indices;
        Ogre::uint32 currentVertexIndex;

        Ogre::Item* thornItem;
        Ogre::String thornMeshName;

        PhysicsArtifactComponent* physicsArtifactComponent;
    };

}; // namespace end

#endif
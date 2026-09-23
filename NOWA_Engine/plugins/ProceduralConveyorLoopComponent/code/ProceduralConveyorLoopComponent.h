/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#ifndef PROCEDURAL_CONVEYOR_LOOP_COMPONENT_H
#define PROCEDURAL_CONVEYOR_LOOP_COMPONENT_H

#include "OgrePlugin.h"
#include "gameobject/GameObjectComponent.h"

namespace NOWA
{
    class PhysicsArtifactComponent;

    /**
     * @class ProceduralConveyorLoopComponent
     * @brief A conveyor belt whose texture visibly scrolls around a closed stadium-shaped loop,
     *        sized in meters.
     *
     * PBS has no material-level scrolling texture animation the way the old fixed-function
     * material scripts did, so this animates the belt by editing the mesh's own U texture
     * coordinate every frame instead - the vertex POSITIONS never move, only their U value, via
     * a dynamic (BT_DYNAMIC_DEFAULT) vertex buffer re-uploaded each frame, the same pattern
     * MeshModifyComponent already uses for its own per-frame vertex edits. Cheap enough for the
     * handful of conveyor belts a level actually needs; not something to place by the hundred.
     *
     * GEOMETRY
     *
     * The belt's cross-section (in the local X-Y plane) is a stadium / discorectangle: two
     * straight sections (the belt's visible top and its return underside) connected by two
     * semicircular caps (the rollers at each end) - extruded through the full 'Depth' in Z, the
     * same convention every other procedural component in this project uses for its depth axis.
     * This shape is a CLOSED LOOP, unlike ProceduralBlockComponent's open extruded prism -
     * walking all the way around it returns to the start, which is exactly what lets a single
     * continuously increasing arc-length parameter drive a texture that scrolls all the way
     * around without a seam popping at either end.
     *
     *   x: 0 .. Belt Length                      (direction of travel)
     *   y: 0 .. 2 * Roller Radius                 (belt "height" - top surface at y = 2*radius)
     *   z: -Depth/2 .. +Depth/2                   (centred, matching every other component)
     *
     * TWO MATERIALS, TWO SUBMESHES
     *
     * 'Belt Datablock' covers the belt's own looping surface (the part that scrolls); 'End Cap
     * Datablock' covers the two flat stadium-shaped faces closing off each Z end (which never
     * scroll - they are a small, usually barely visible face, not part of the belt's own moving
     * surface). These are two independent submeshes with two independent vertex buffers: only
     * the belt submesh's buffer is BT_DYNAMIC_DEFAULT and gets re-uploaded every frame: the end
     * caps' own buffer is a plain BT_IMMUTABLE one, built once and never touched again.
     *
     * SEAMLESS SCROLLING
     *
     * Every perimeter point around the stadium has a cached cumulative arc-length distance from
     * an arbitrary start point. Each belt vertex's U texture coordinate is
     * (thatVertex'sArcLength + scrollOffset) * (BeltRepeatCount / totalPerimeter); update()
     * advances scrollOffset by Belt Speed * dt every frame, wrapped via fmod against the loop's
     * own total perimeter length.
     *
     * 'Belt Repeat Count' is deliberately an INTEGER, not a free "meters per repeat" tiling
     * value: a full lap around the loop must cover an EXACT whole number of texture repeats, or
     * the moment scrollOffset wraps, every vertex's U jumps by a fractional repeat instead of a
     * whole one, which is exactly the visible pop this component's own testing found before this
     * constraint was added. Dividing by totalPerimeter (recomputed every rebuild from Belt
     * Length and Roller Radius) means one full lap of scrollOffset is ALWAYS exactly
     * BeltRepeatCount repeats, by construction, regardless of the belt's own dimensions - the
     * same requirement a real, physically printed conveyor belt's pattern would have to satisfy
     * to loop without a seam.
     *
     * A visually correct scrolling belt is not the same thing as an object placed on top of it
     * actually being pushed along - that is a physics behaviour, already implemented separately
     * as PhysicsMaterialComponent's conveyor ConveyorContactCallback (Contact Behavior
     * 'ConveyorPlayer'/'ConveyorObject'). Pair this component with a PhysicsMaterialComponent
     * set up that way, with its Contact Direction and Contact Speed matching this component's
     * own belt direction/speed, for a belt that both looks and behaves like it is moving.
     */
    class EXPORTED ProceduralConveyorLoopComponent : public GameObjectComponent, public Ogre::Plugin
    {
    public:
        typedef boost::shared_ptr<ProceduralConveyorLoopComponent> ProceduralConveyorLoopCompPtr;

    public:
        ProceduralConveyorLoopComponent();

        virtual ~ProceduralConveyorLoopComponent();

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
         * @see GameObjectComponent::clone
         */
        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

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

        virtual Ogre::String getClassName(void) const override
        {
            return "ProceduralConveyorLoopComponent";
        }

        virtual Ogre::String getParentClassName(void) const override
        {
            return "GameObjectComponent";
        }

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("ProceduralConveyorLoopComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "ProceduralConveyorLoopComponent";
        }

        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

        static Ogre::String getStaticInfoText(void)
        {
            return "Usage: A conveyor belt whose texture visibly scrolls around a closed stadium-shaped loop.\n\n"

                   "SIZE:\n"
                   "- 'Belt Length' and 'Depth' are the belt's outer dimensions in meters.\n"
                   "- 'Roller Radius' sets how rounded the two ends are - also the belt's overall height (2 x "
                   "radius).\n\n"

                   "ANIMATION:\n"
                   "- 'Belt Speed' is how fast the texture scrolls around the loop, in meters per second. "
                   "Negative reverses direction.\n"
                   "- 'Belt Repeat Count' is how many times the belt texture repeats around one full lap - kept "
                   "as a whole number on purpose, so the scroll never visibly pops once it has gone all the way "
                   "around.\n"
                   "- This only animates the TEXTURE, not physics - pair with a PhysicsMaterialComponent set to "
                   "Contact Behavior 'ConveyorPlayer'/'ConveyorObject' for objects to actually be pushed along.\n\n"

                   "MATERIAL:\n"
                   "- 'Belt Datablock' covers the scrolling belt surface; 'End Cap Datablock' covers the two "
                   "flat, non-scrolling faces at each end.\n"
                   "- 'Depth UV Tiling' scales the belt's texture across its depth, in meters per repeat.\n\n"

                   "COLLISION:\n"
                   "- Add a PhysicsArtifactComponent to the same game object and the belt is rebuilt as a "
                   "collision hull automatically whenever its geometry changes.\n\n"

                   "LUA API:\n"
                   "- getProceduralConveyorLoopComponent() on a GameObject returns this component.\n"
                   "- setBeltLength(l), setRollerRadius(r), setDepth(d), setBeltSpeed(s) set size and speed.\n"
                   "- setBeltDatablock(name), setEndCapDatablock(name) set the two materials.\n";
        }
        
        static std::optional<NOWA::GameObjectTypeDescriptor> getStaticTypeDescriptor()
        {
            NOWA::GameObjectTypeDescriptor desc;
            desc.type = eType::CUSTOM;
            desc.displayName = "Platform Conveyor";
            desc.meshToDisplay = "Node.mesh";
            desc.needsMeshItem = false;
            desc.enterMeshModifyMode = false;
            desc.autoComponents = {"ProceduralConveyorLoopComponent"};
            desc.guardWithPluginCheck = true;
            return desc;
        }

        static const Ogre::String AttrActivated(void)
        {
            return "Activated";
        }
        static const Ogre::String AttrBeltLength(void)
        {
            return "Belt Length";
        }
        static const Ogre::String AttrRollerRadius(void)
        {
            return "Roller Radius";
        }
        static const Ogre::String AttrDepth(void)
        {
            return "Depth";
        }
        static const Ogre::String AttrBeltSpeed(void)
        {
            return "Belt Speed";
        }
        static const Ogre::String AttrBeltRepeatCount(void)
        {
            return "Belt Repeat Count";
        }
        static const Ogre::String AttrDepthUVTiling(void)
        {
            return "Depth UV Tiling";
        }
        static const Ogre::String AttrBeltDatablock(void)
        {
            return "Belt Datablock";
        }
        static const Ogre::String AttrEndCapDatablock(void)
        {
            return "End Cap Datablock";
        }

    public:
        virtual void setActivated(bool activated) override;

        virtual bool isActivated(void) const override;

        void setBeltLength(Ogre::Real beltLength);

        Ogre::Real getBeltLength(void) const;

        void setRollerRadius(Ogre::Real rollerRadius);

        Ogre::Real getRollerRadius(void) const;

        void setDepth(Ogre::Real depth);

        Ogre::Real getDepth(void) const;

        void setBeltSpeed(Ogre::Real beltSpeed);

        Ogre::Real getBeltSpeed(void) const;

        /**
         * @brief Sets how many times the belt's texture repeats around one full lap of the loop.
         * @param[in] beltRepeatCount Whole number of repeats, minimum 1. Kept as an integer on
         *            purpose - see the class comment for why a fractional repeat count would
         *            make the scroll visibly pop once it wraps around.
         */
        void setBeltRepeatCount(int beltRepeatCount);

        int getBeltRepeatCount(void) const;

        void setDepthUVTiling(Ogre::Real depthUVTiling);

        Ogre::Real getDepthUVTiling(void) const;

        void setBeltDatablock(const Ogre::String& beltDatablock);

        Ogre::String getBeltDatablock(void) const;

        void setEndCapDatablock(const Ogre::String& endCapDatablock);

        Ogre::String getEndCapDatablock(void) const;
    private:
        // ── Mesh generation ──────────────────────────────────────────────────────────────
        void rebuildMesh(void);

        /**
         * @brief (Re)builds the stadium cross-section's perimeter points and their cumulative
         *        arc-length distances, and the loop's total perimeter length. Populates
         *        perimeterPoints/perimeterArcLengths/totalPerimeter; called once per
         *        rebuildMesh(), before the perimeter is extruded into actual geometry.
         */
        void buildPerimeter(void);

        /**
         * @brief Emits one quad onto the BELT submesh's own buffers, self-correcting its winding
         *        from the supplied outward normal - same approach as
         *        ProceduralBlockComponent::addBlockQuad. arcLength0/arcLength1 are cumulative
         *        arc-length values in meters, NOT yet scaled by BeltRepeatCount/totalPerimeter or
         *        offset by scrollOffset - see updateScrollingUVs() for where that happens every
         *        frame.
         */
        void addBeltQuad(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& v3, const Ogre::Vector3& normal, Ogre::Real arcLength0, Ogre::Real arcLength1, Ogre::Real v0Coord,
            Ogre::Real v1Coord);

        /**
         * @brief Emits one triangle onto the END CAP submesh's own buffers - a small,
         *        fixed-shape, non-scrolling face, so its UV is just the raw local X/Y position
         *        with no tiling or offset applied.
         */
        void addEndCapTriangle(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& normal, const Ogre::Vector2& uv0, const Ogre::Vector2& uv1, const Ogre::Vector2& uv2);

        /**
         * @brief Extrudes every perimeter edge through Depth as one belt quad each - the belt's
         *        own visible looping surface.
         */
        void addLoopSideQuads(Ogre::Real halfDepth);

        /**
         * @brief Closes the loop's two open Z ends with a fan-triangulated stadium-shaped cap
         *        each, onto the end cap submesh.
         */
        void addLoopEndCaps(Ogre::Real halfDepth);

        void createConveyorMesh(void);

        void createConveyorMeshInternal(const std::vector<float>& beltVerts, const std::vector<Ogre::uint32>& beltInds, size_t numBeltVerts, const std::vector<float>& endCapVerts, const std::vector<Ogre::uint32>& endCapInds,
            size_t numEndCapVerts);

        void destroyConveyorMesh(void);

        void updatePhysicsCollision(void);

        /**
         * @brief Recomputes the U texture coordinate of every belt vertex from its cached
         *        arc-length plus the current scrollOffset, and re-uploads the belt submesh's own
         *        dynamic vertex buffer - called from update() every frame the belt is moving.
         *        Never touches the end cap submesh's buffer at all.
         */
        void updateScrollingUVs(void);

    private:
        Ogre::String name;

        Variant* activated;
        Variant* beltLength;
        Variant* rollerRadius;
        Variant* depth;
        Variant* beltSpeed;
        Variant* beltRepeatCount;
        Variant* depthUVTiling;
        Variant* beltDatablock;
        Variant* endCapDatablock;

        // Interleaved layout: pos.xyz, normal.xyz, tangent.xyzw, uv.xy = 12 floats per vertex -
        // tangent included from the start, same reasoning as every other component in this
        // project's own history: a normal-mapped datablock otherwise fails outright.
        //
        // Unlike ProceduralBlockComponent/ProceduralThornComponent, this "vertices" buffer is
        // "beltVertices" is NOT a throwaway scratch array only used while building - it is kept
        // as a live member after the initial build, because updateScrollingUVs() edits its U
        // floats in place every frame and re-uploads the very same buffer. Every entry in it is
        // scrollable (this is the belt-only buffer now, unlike the single combined buffer this
        // component's first draft used), so there is no per-vertex "is this scrollable" sentinel
        // to check any more - beltVertexArcLength is valid for every single vertex here.
        std::vector<float> beltVertices;
        std::vector<Ogre::uint32> beltIndices;
        Ogre::uint32 currentBeltVertexIndex;
        std::vector<float> beltVertexArcLength;

        // End cap geometry is a completely separate, plain scratch buffer - built once per
        // rebuildMesh() and never touched again afterward, unlike beltVertices above.
        std::vector<float> endCapVertices;
        std::vector<Ogre::uint32> endCapIndices;
        Ogre::uint32 currentEndCapVertexIndex;

        // Cross-section perimeter cache, rebuilt by buildPerimeter() whenever the shape changes.
        std::vector<Ogre::Vector2> perimeterPoints;
        std::vector<Ogre::Real> perimeterArcLengths;
        Ogre::Real totalPerimeter;

        // Current animation offset in meters around the loop, wrapped against totalPerimeter
        // every frame in update().
        Ogre::Real scrollOffset;

        Ogre::Item* conveyorItem;
        Ogre::String conveyorMeshName;

        // Kept across frames so updateScrollingUVs() can re-upload into the SAME GPU buffer
        // every frame instead of recreating it - only rebuildMesh() (an actual shape change)
        // replaces this. The end cap submesh's own buffer needs no equivalent handle, since it
        // is never re-uploaded after creation.
        Ogre::VertexBufferPacked* dynamicVertexBuffer;

        PhysicsArtifactComponent* physicsArtifactComponent;
    };

}; // namespace end

#endif
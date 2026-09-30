/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#ifndef PROCEDURAL_FLOW_CURTAIN_COMPONENT_H
#define PROCEDURAL_FLOW_CURTAIN_COMPONENT_H

#include "OgrePlugin.h"
#include "gameobject/GameObjectComponent.h"

namespace NOWA
{
    /**
     * @class ProceduralFlowCurtainComponent
     * @brief A flat, vertical curtain of falling material (water, lava, slime, ...) whose texture
     *        visibly scrolls downward (or upward), built as a Rows x Cols grid so it is not a
     *        perfectly rigid single quad.
     *
     * NAMING: deliberately not "Waterfall" - the geometry and scrolling logic know nothing about
     * water. Swap 'Flow Datablock'/'Foam Datablock' for a lava, slime or honey texture and the
     * same component produces a lava curtain instead. "Flow" is the generic term; "Curtain"
     * describes the shape (a flat hanging sheet you can walk behind).
     *
     * GEOMETRY
     *
     * Two independent, stacked layers, each its own Rows x Cols grid / submesh / datablock:
     *   - Flow layer: the main falling surface. Always built when Activated.
     *   - Foam layer: an optional second, slightly-in-front layer (see 'Foam Z Offset') for
     *     spray/foam detail on top of the flow layer. Only built when 'Foam Enabled' is true.
     *
     * Both layers span the same local rectangle:
     *   x: -Width/2 .. +Width/2            (centred, so the node origin sits in the middle)
     *   y: 0 .. Height                     (origin at the BASE - place the node where the
     *                                        curtain meets the ground/pool, matching how you'd
     *                                        actually position a waterfall in a level)
     *   z: 0 (flow layer) / Foam Z Offset (foam layer)
     *
     * Deliberately NO collision is ever created for this component (unlike
     * ProceduralConveyorLoopComponent, which pairs with PhysicsArtifactComponent). The whole
     * point is that the player can walk through/behind it. If you want a splash sound or a wet
     * trigger zone, add a small separate PhysicsMaterialComponent trigger volume of your own -
     * that is a deliberate separation of concerns, not a missing feature here.
     *
     * RIPPLE (why Rows/Cols matter)
     *
     * A single flat quad reads as an obviously rigid card. Worse, a ripple that only varies
     * across the width (X) and is identical at every height looks like a rigid corrugated sheet
     * - literally the same curve extruded straight down - which is exactly what made earlier
     * versions of this component look like "an exactly downward-facing plane" despite having a
     * ripple at all. 'Ripple Amount' now offsets each grid vertex's Z position by a fixed
     * (time-independent, baked once at build time) combination of THREE things instead of one
     * plain sine wave:
     *   - a primary bulge across the width, whose phase itself slowly twists as height (Y)
     *     increases, so the curve is never the same at two different rows;
     *   - a smaller, higher-frequency secondary term layered on top of the primary one (classic
     *     multi-octave/turbulence trick), which is what actually reads as "chaos" instead of
     *     "one clean wave";
     *   - a per-instance random phase, seeded from the game object's id (see flowHashNoise() in
     *     the .cpp), so multiple placed curtains in the same level don't all ripple in obvious
     *     lockstep with each other.
     * On top of the Z ripple, whole ROWS are also swayed sideways along X by a slower, lower-
     * amplitude sine term, which breaks the silhouette out of being a perfect rectangle and is
     * a large part of what reads as "volume" rather than "flat card". The foam layer is built
     * with a different seed salt than the flow layer (see buildLayerGrid()'s layerSeedSalt
     * parameter), so the two layers never ripple/sway identically in lockstep, which by itself
     * adds a lot of apparent depth/parallax between them even though both are static.
     *
     * All of the above is still NOT animated per frame - only the scrolling texture animates:
     * an animated *mesh* ripple would need the same per-frame dynamic-vertex-buffer re-upload as
     * the scrolling itself, doubling the per-frame cost for a subtlety most players would never
     * consciously notice. If you do want that later, scrollLayer() is the one place to extend -
     * it already re-uploads every vertex float every frame, so adding a time-varying Z would not
     * need a second upload path, just a second write into the same vertex array before upload.
     *
     * 'Rows'/'Cols' set how many vertices are available to actually show all of the above curves
     * - too few and the twist/secondary-wave detail has nothing to bend around. Defaults were
     * raised (6x4 -> 10x10) specifically because the old low-resolution defaults were part of
     * why the curtain looked flat: there was barely enough geometry for even a single clean
     * wave, let alone a twisting, multi-octave one. Existing already-placed curtains keep
     * whatever Rows/Cols value is stored in their own saved XML - this only changes what a
     * freshly-created one starts with.
     *
     * SCROLLING - SIMPLER THAN ProceduralConveyorLoopComponent, ON PURPOSE
     *
     * Same underlying technique as ProceduralConveyorLoopComponent - PBS has no material-level
     * scrolling texture animation, so this rewrites the mesh's own V texture coordinate every
     * frame via a BT_DYNAMIC_DEFAULT vertex buffer, exactly like the belt does for its U.
     *
     * But this shape is OPEN, not a closed loop, and that removes the belt's entire reason for
     * needing an integer 'Belt Repeat Count' divided by the loop's total perimeter: nothing here
     * ever "wraps back to vertex 0" the way walking all the way around the belt's loop does, so
     * there is no seam-popping instant to avoid. Each vertex keeps a fixed base V coordinate
     * (its row's height fraction times 'Flow V Tiling', a plain repeats-per-meter value - no
     * perimeter math needed) for its entire lifetime, and every frame simply adds a scroll
     * offset in the SAME repeats-per-meter units. That offset is wrapped with fmod against 1.0
     * texture repeat purely to keep the float from growing without bound over a long play
     * session - since a periodic (tileable) texture already looks identical at every integer
     * repeat boundary, wrapping it can never produce a visible jump, unlike the belt's stricter
     * requirement.
     *
     * REQUIRES: the datablock's texture sampler must use WRAP addressing (not clamp) on the V
     * axis, or the scroll will smear the last row of texels instead of repeating. This is the
     * usual default for a tiling texture, but double-check it on whatever datablock you assign
     * to 'Flow Datablock'/'Foam Datablock'.
     *
     * TEXTURES
     *
     * Both textures used here (Flow/WaterA, Flow/FoamA by default) must tile seamlessly along V
     * (and ideally along U too, since 'Horizontal UV Tiling' can repeat them sideways). Plain
     * white RGB with the actual look carried entirely in the alpha channel works well and lets
     * the datablock/vertex colour tint it (e.g. blue-white for water, orange-red for lava)
     * without needing a second texture per material variant.
     *
     * PER-INSTANCE COLOUR/OPACITY
     *
     * 'Flow Colour Red/Green/Blue' + 'Flow Opacity' let each placed curtain have its own tint
     * without editing the shared JSON material (which would tint every other object using that
     * same 'Flow Datablock' name too). This works by CLONING the named flow datablock once per
     * game object (applyFlowColour()) and tinting the clone via
     * Ogre::HlmsUnlitDatablock::setUseColour()/setColour() - the exact same "useColour" flag
     * already seen set on flat-colour entries in this project's own Hlms JSON files. The clone
     * is destroyed and re-created whenever 'Flow Datablock' itself changes (a different base
     * material), and destroyed for good in onRemoveComponent()/destroyFlowMesh(). Exposed as
     * four separate Real attributes rather than one Vector3/Vector4, on purpose - this project's
     * XML property 'type' codes for Vector3/Vector4 were not available to verify against (unlike
     * int/real/string/bool, confirmed via ProceduralConveyorLoopComponent's own writeXML), so
     * this avoids guessing a schema that silently fails the same way the material lookup did.
     * The foam layer intentionally has no separate colour control - it stays a plain white/soft
     * overlay regardless of the flow tint, which is closer to how real foam/spray looks against
     * tinted water.
     */
    class EXPORTED ProceduralFlowCurtainComponent : public GameObjectComponent, public Ogre::Plugin
    {
    public:
        typedef boost::shared_ptr<ProceduralFlowCurtainComponent> ProceduralFlowCurtainCompPtr;

    public:
        ProceduralFlowCurtainComponent();

        virtual ~ProceduralFlowCurtainComponent();

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
            return "ProceduralFlowCurtainComponent";
        }

        virtual Ogre::String getParentClassName(void) const override
        {
            return "GameObjectComponent";
        }

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("ProceduralFlowCurtainComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "ProceduralFlowCurtainComponent";
        }

        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

        static Ogre::String getStaticInfoText(void)
        {
            return "Usage: A flat, vertical curtain of falling material (water, lava, slime, ...) built as a "
                   "Rows x Cols grid, with a downward/upward scrolling texture. No collision is ever created - "
                   "the player is meant to be able to walk through/behind it.\n\n"

                   "SIZE:\n"
                   "- 'Width' and 'Height' are the curtain's outer dimensions in meters, centred on X, based at "
                   "y=0 (place the node where the curtain meets the ground).\n"
                   "- 'Rows'/'Cols' subdivide the grid - higher values give 'Ripple Amount' more geometry to "
                   "curve and improve normal-mapped lighting over a tall surface.\n"
                   "- 'Ripple Amount' bends the grid with a fixed (non-animated) sine wave across the width, so "
                   "the curtain is not a perfectly flat card. Set to 0 for a flat sheet.\n\n"

                   "ANIMATION:\n"
                   "- 'Flow Speed'/'Foam Speed' are how fast each layer's texture scrolls, in meters per second. "
                   "Negative reverses direction.\n"
                   "- 'Flow V Tiling'/'Foam V Tiling' are how many times each layer's texture repeats per meter "
                   "of height. 'Horizontal UV Tiling' does the same across the width, for both layers.\n"
                   "- Unlike ProceduralConveyorLoopComponent, no integer repeat-count trick is needed here - "
                   "this is an open surface, not a closed loop, so an ordinary tileable texture never pops.\n\n"

                   "LAYERS:\n"
                   "- 'Flow Datablock' covers the main falling surface (always built while Activated).\n"
                   "- 'Foam Enabled' + 'Foam Datablock' add a second, optional layer offset by 'Foam Z Offset' "
                   "meters in front of the flow layer, for spray/foam detail.\n"
                   "- Both datablocks' texture samplers must use WRAP addressing on V, or scrolling will smear "
                   "instead of repeat.\n"
                   "- 'Flow Colour Red/Green/Blue' + 'Flow Opacity' tint this ONE instance only (via a per-object "
                   "cloned datablock) without touching the shared 'Flow Datablock' material or affecting any other "
                   "curtain using the same one.\n\n"

                   "LUA API:\n"
                   "- getProceduralFlowCurtainComponent() on a GameObject returns this component.\n"
                   "- setWidth(w), setHeight(h), setRows(r), setCols(c), setRippleAmount(a) set the shape.\n"
                   "- setFlowSpeed(s), setFoamSpeed(s) set scroll speed.\n"
                   "- setFlowDatablock(name), setFoamDatablock(name) set the two materials.\n";
        }

        static std::optional<NOWA::GameObjectTypeDescriptor> getStaticTypeDescriptor()
        {
            NOWA::GameObjectTypeDescriptor desc;
            desc.type = eType::CUSTOM;
            desc.displayName = "Flow Curtain";
            desc.meshToDisplay = "Node.mesh";
            desc.needsMeshItem = false;
            desc.enterMeshModifyMode = false;
            desc.autoComponents = {"ProceduralFlowCurtainComponent"};
            desc.guardWithPluginCheck = true;
            return desc;
        }

        static const Ogre::String AttrActivated(void)
        {
            return "Activated";
        }
        static const Ogre::String AttrWidth(void)
        {
            return "Width";
        }
        static const Ogre::String AttrHeight(void)
        {
            return "Height";
        }
        static const Ogre::String AttrRows(void)
        {
            return "Rows";
        }
        static const Ogre::String AttrCols(void)
        {
            return "Cols";
        }
        static const Ogre::String AttrRippleAmount(void)
        {
            return "Ripple Amount";
        }
        static const Ogre::String AttrHorizontalUVTiling(void)
        {
            return "Horizontal UV Tiling";
        }
        static const Ogre::String AttrFlowDatablock(void)
        {
            return "Flow Datablock";
        }
        static const Ogre::String AttrFlowSpeed(void)
        {
            return "Flow Speed";
        }
        static const Ogre::String AttrFlowVTiling(void)
        {
            return "Flow V Tiling";
        }
        static const Ogre::String AttrFoamEnabled(void)
        {
            return "Foam Enabled";
        }
        static const Ogre::String AttrFoamDatablock(void)
        {
            return "Foam Datablock";
        }
        static const Ogre::String AttrFoamSpeed(void)
        {
            return "Foam Speed";
        }
        static const Ogre::String AttrFoamVTiling(void)
        {
            return "Foam V Tiling";
        }
        static const Ogre::String AttrFoamZOffset(void)
        {
            return "Foam Z Offset";
        }
        static const Ogre::String AttrFlowColourRed(void)
        {
            return "Flow Colour Red";
        }
        static const Ogre::String AttrFlowColourGreen(void)
        {
            return "Flow Colour Green";
        }
        static const Ogre::String AttrFlowColourBlue(void)
        {
            return "Flow Colour Blue";
        }
        static const Ogre::String AttrFlowOpacity(void)
        {
            return "Flow Opacity";
        }

    public:
        virtual void setActivated(bool activated) override;

        virtual bool isActivated(void) const override;

        void setWidth(Ogre::Real width);

        Ogre::Real getWidth(void) const;

        void setHeight(Ogre::Real height);

        Ogre::Real getHeight(void) const;

        void setRows(int rows);

        int getRows(void) const;

        void setCols(int cols);

        int getCols(void) const;

        void setRippleAmount(Ogre::Real rippleAmount);

        Ogre::Real getRippleAmount(void) const;

        void setHorizontalUVTiling(Ogre::Real horizontalUVTiling);

        Ogre::Real getHorizontalUVTiling(void) const;

        void setFlowDatablock(const Ogre::String& flowDatablock);

        Ogre::String getFlowDatablock(void) const;

        void setFlowSpeed(Ogre::Real flowSpeed);

        Ogre::Real getFlowSpeed(void) const;

        void setFlowVTiling(Ogre::Real flowVTiling);

        Ogre::Real getFlowVTiling(void) const;

        void setFoamEnabled(bool foamEnabled);

        bool getFoamEnabled(void) const;

        void setFoamDatablock(const Ogre::String& foamDatablock);

        Ogre::String getFoamDatablock(void) const;

        void setFoamSpeed(Ogre::Real foamSpeed);

        Ogre::Real getFoamSpeed(void) const;

        void setFoamVTiling(Ogre::Real foamVTiling);

        Ogre::Real getFoamVTiling(void) const;

        void setFoamZOffset(Ogre::Real foamZOffset);

        Ogre::Real getFoamZOffset(void) const;

        void setFlowColourRed(Ogre::Real red);

        Ogre::Real getFlowColourRed(void) const;

        void setFlowColourGreen(Ogre::Real green);

        Ogre::Real getFlowColourGreen(void) const;

        void setFlowColourBlue(Ogre::Real blue);

        Ogre::Real getFlowColourBlue(void) const;

        void setFlowOpacity(Ogre::Real opacity);

        Ogre::Real getFlowOpacity(void) const;

    private:
        /**
         * @brief Per-layer scratch + live geometry state. One instance for the flow layer, one
         *        for the (optional) foam layer - identical structure, so the grid-building and
         *        per-frame scroll code (buildLayerGrid()/scrollLayer()) is written once and
         *        reused for both instead of duplicated the way
         *        ProceduralConveyorLoopComponent's belt/end-cap members had to be, since those
         *        two use genuinely different buffer types (one dynamic, one immutable) while
         *        both layers here are dynamic and behave identically.
         */
        struct FlowLayer
        {
            // Interleaved layout: pos.xyz, normal.xyz, tangent.xyzw, uv.xy = 12 floats per
            // vertex - tangent included from the start, same reasoning as every other
            // component in this project's history (a normal-mapped datablock otherwise fails
            // outright).
            std::vector<float> vertices;
            std::vector<Ogre::uint32> indices;
            Ogre::uint32 currentVertexIndex = 0;

            // Each vertex's fixed base V coordinate (row height fraction * that layer's own V
            // tiling), parallel to 'vertices'. scrollOffset is added to this every frame; never
            // touches U, position, normal or tangent.
            std::vector<float> vertexBaseV;

            // Current scroll offset, in texture-repeat units, wrapped against 1.0 every frame.
            Ogre::Real scrollOffset = 0.0f;

            // Kept across frames so the per-frame update re-uploads into the SAME GPU buffer
            // instead of recreating it - only rebuildMesh() (an actual shape change) replaces
            // this.
            Ogre::VertexBufferPacked* dynamicVertexBuffer = nullptr;

            void clear()
            {
                this->vertices.clear();
                this->indices.clear();
                this->vertexBaseV.clear();
                this->currentVertexIndex = 0;
                // scrollOffset and dynamicVertexBuffer intentionally survive a rebuild - a
                // resize/ripple change should not reset the scroll animation or leak the old
                // buffer pointer before createFlowMeshInternal() replaces it.
            }
        };

        // ── Mesh generation ──────────────────────────────────────────────────────────────
        void rebuildMesh(void);

        /**
         * @brief Emits one quad onto the given layer's own buffers, self-correcting its winding
         *        from the supplied outward-normal hint - same approach as
         *        ProceduralConveyorLoopComponent::addBeltQuad and ProceduralBlockComponent's
         *        addBlockQuad. Unlike those, the actual per-quad normal is computed from the
         *        (possibly ripple-displaced) positions themselves, not assumed constant, so
         *        lighting follows the ripple curve automatically; normalHint only decides
         *        winding.
         */
        void addLayerQuad(FlowLayer& layer, const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& v3, const Ogre::Vector3& normalHint, Ogre::Real u0, Ogre::Real u1, Ogre::Real baseV0, Ogre::Real baseV1);

        /**
         * @brief Builds a full Rows x Cols grid of quads into the given layer, at the given Z
         *        offset and V tiling. Used for both the flow layer (zOffset 0, layerSeedSalt 0)
         *        and the optional foam layer (zOffset = Foam Z Offset, layerSeedSalt non-zero).
         *
         * @param layerSeedSalt Mixed into the per-instance ripple/sway seed (see flowHashNoise()
         *        in the .cpp) before computing this layer's random phases, so the flow and foam
         *        layers get DIFFERENT phases from the same game object id instead of rippling in
         *        perfect lockstep with each other. Pass a different constant per layer; the
         *        actual value doesn't matter beyond "not equal to the other layer's salt".
         */
        void buildLayerGrid(FlowLayer& layer, Ogre::Real zOffset, Ogre::Real vTiling, unsigned int layerSeedSalt);

        void createFlowMesh(void);

        void createFlowMeshInternal(const std::vector<float>& flowVerts, const std::vector<Ogre::uint32>& flowInds, size_t numFlowVerts, const std::vector<float>& foamVerts, const std::vector<Ogre::uint32>& foamInds, size_t numFoamVerts);

        void destroyFlowMesh(void);

        /**
         * @brief Advances one layer's scrollOffset by speed * vTiling * dt (wrapped against one
         *        texture repeat), rewrites every one of its vertices' V float from
         *        vertexBaseV + scrollOffset, and non-blockingly re-uploads its dynamic vertex
         *        buffer. Called once per active layer from update().
         */
        void scrollLayer(FlowLayer& layer, Ogre::Real speed, Ogre::Real vTiling, Ogre::Real dt);

        void applyFlowColourInternal(void);

        /**
         * @brief Logic-thread entry point for applyFlowColourInternal() - every colour/opacity
         *        setter calls THIS, never applyFlowColourInternal() directly. Dispatches onto the
         *        render thread via enqueueAndWait(), exactly like createFlowMesh()/
         *        destroyFlowMesh() already do for their own Ogre work. Synchronous (Wait, not
         *        just enqueue) is deliberate here - this fires on rare, user-driven colour edits,
         *        not once per frame like scrollLayer()'s upload, so there is no per-frame cost to
         *        avoid, and Wait keeps the call trivially safe to chain from any setter without
         *        worrying about ordering against a subsequent rebuildMesh().
         */
        void applyFlowColour(void);

    private:
        Ogre::String name;

        Variant* activated;
        Variant* width;
        Variant* height;
        Variant* rows;
        Variant* cols;
        Variant* rippleAmount;
        Variant* horizontalUVTiling;
        Variant* flowDatablock;
        Variant* flowSpeed;
        Variant* flowVTiling;
        Variant* foamEnabled;
        Variant* foamDatablock;
        Variant* foamSpeed;
        Variant* foamVTiling;
        Variant* foamZOffset;
        Variant* flowColourRed;
        Variant* flowColourGreen;
        Variant* flowColourBlue;
        Variant* flowOpacity;

        FlowLayer flowLayer;
        FlowLayer foamLayer;

        Ogre::Item* flowItem;
        Ogre::String flowMeshName;

        // Name of the per-object cloned+tinted flow datablock, once applyFlowColour() has run at
        // least once - empty until then. Tracked so applyFlowColourInternal() can destroy the
        // previous clone once it is no longer linked to any renderable (e.g. after
        // setFlowDatablock() points at a different base material, or after a colour/opacity
        // change creates a fresh clone), and so destroyFlowMesh()/onRemoveComponent() can clean
        // it up instead of leaking an Hlms datablock every time this component is removed or
        // rebuilt.
        Ogre::String flowColourDatablockName;

        // Monotonically increasing, appended to every colour-clone name so each call to
        // applyFlowColourInternal() produces a genuinely NEW, uniquely-named clone rather than
        // re-using the previous call's name. Required for the destroy-after-relink ordering fix
        // (see applyFlowColourInternal()'s comment): the subitem must be pointed at the NEW clone
        // before the OLD one is destroyed, which is only possible if the two clones have
        // different names - Hlms::clone() would otherwise collide with the still-in-use old name.
        Ogre::uint32 flowColourCloneCounter;
    };

}; // namespace end

#endif
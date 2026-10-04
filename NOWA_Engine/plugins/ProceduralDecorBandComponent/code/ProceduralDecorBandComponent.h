/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#ifndef PROCEDURAL_DECOR_BAND_COMPONENT_H
#define PROCEDURAL_DECOR_BAND_COMPONENT_H

#include "OgrePlugin.h"
#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"

namespace NOWA
{
    /**
     * @class ProceduralDecorBandComponent
     * @brief Bands of procedurally generated rock silhouettes for dressing 2.5D levels -
     *        the foreground occluders a Metroidvania puts between the camera and the player,
     *        and the ridges it stacks behind him.
     *
     * The band is a plain rectangle in the GameObject's own XY plane, filled with a grid of
     * Rows x Columns rock clusters. Deliberately NO path and no mouse editing: a decoration
     * band is placed and sized like any other object, and everything about its shape comes
     * from the attributes. That also means nothing of it is serialized except those attributes
     * - the geometry is a pure function of them plus Seed, so a scene file never carries a
     * single vertex of it and two machines generate byte-identical bands.
     *
     * ROWS ARE DEPTH, NOT STACKING. Row 0 sits at the GameObject's own plane and every further
     * row is offset by Row Depth Spacing, scaled by Row Scale and shaded from Front Brightness
     * towards Back Brightness. That is what turns a flat picket fence into a ridge: the rows
     * overlap, and the brightness ramp does the rest. Value separation between depth layers is
     * what reads as depth - more objects at the same brightness only read as noise. Row Depth
     * Spacing may be negative, which brings the rows towards the camera instead of away from it.
     *
     * Grow Direction picks the axis the rock height grows along; the band's run is always local
     * X and the remaining axis carries the cluster thickness and the row offsets.
     *
     * Each row becomes its own submesh with its own cloned datablock, so the brightness ramp
     * costs one material per row and leaves the source datablock untouched - the same cloning
     * pattern ProceduralPipeComponent uses for its transparent near side.
     *
     * Intended use: one band in front of the play plane at 5-12% brightness as an occluder,
     * one or two behind it at 15-25%, and scene fog doing the rest.
     */
    class EXPORTED ProceduralDecorBandComponent : public GameObjectComponent, public Ogre::Plugin
    {
    public:
        typedef boost::shared_ptr<ProceduralDecorBandComponent> ProceduralDecorBandComponentPtr;

        /**
         * @brief Shape of a single rock cluster's outline. All four are the same extruded
         *        prism, they differ only in the profile function that gives the outline its
         *        height at each sample point.
         */
        enum class RockStyle
        {
            JAGGED = 0,    // Sharp broken peaks - the default cave look
            ROUNDED = 1,   // Weathered, smooth humps - sand, moss, old stone
            COLUMNAR = 2,  // Flat-topped steps of differing height - basalt, ruins, crystal
            STALAGMITE = 3 // One tall narrow spike per cell, for ceilings and cave floors
        };

        /**
         * @brief Which local axis the rock height grows along. The band itself always runs
         *        along local X; this picks where the rocks go FROM there, and the remaining
         *        axis automatically becomes the cluster thickness and the direction the depth
         *        rows are pushed along.
         */
        enum class GrowDirection
        {
            UP = 0,              // Grows along +Y - rocks standing on a floor, rows along Z
            DOWN = 1,            // Grows along -Y - stalactites hanging from a ceiling, rows along Z
            TOWARDS_CAMERA = 2,  // Grows along -Z, towards the viewer. Rows then stack along Y
            AWAY_FROM_CAMERA = 3 // Grows along +Z, away from the viewer. Rows then stack along Y
        };

    public:
        ProceduralDecorBandComponent();
        virtual ~ProceduralDecorBandComponent();

        /**
         * @see		Ogre::Plugin::install
         */
        virtual void install(const Ogre::NameValuePairList* options) override;

        /**
         * @see		Ogre::Plugin::initialise
         */
        virtual void initialise() override;

        /**
         * @see		Ogre::Plugin::shutdown
         */
        virtual void shutdown() override;

        /**
         * @see		Ogre::Plugin::uninstall
         */
        virtual void uninstall() override;

        /**
         * @see		Ogre::Plugin::getName
         */
        virtual const Ogre::String& getName() const override;

        /**
         * @see		Ogre::Plugin::getAbiCookie
         */
        virtual void getAbiCookie(Ogre::AbiCookie& outAbiCookie) override;

        /**
         * @see		GameObjectComponent::init
         */
        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

        /**
         * @see		GameObjectComponent::postInit
         */
        virtual bool postInit(void) override;

        /**
         * @see		GameObjectComponent::connect
         */
        virtual bool connect(void) override;

        /**
         * @see		GameObjectComponent::disconnect
         */
        virtual bool disconnect(void) override;

        /**
         * @see		GameObjectComponent::onRemoveComponent
         */
        virtual void onRemoveComponent(void);

        /**
         * @see		GameObjectComponent::clone
         */
        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

        /**
         * @see		GameObjectComponent::update
         */
        virtual void update(Ogre::Real dt, bool notSimulating = false) override;

        /**
         * @see		GameObjectComponent::actualizeValue
         */
        virtual void actualizeValue(Variant* attribute) override;

        /**
         * @see		GameObjectComponent::writeXML
         */
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("ProceduralDecorBandComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "ProceduralDecorBandComponent";
        }

        static bool canStaticAddComponent(GameObject* gameObject);

        static Ogre::String getStaticInfoText(void)
        {
            return "Usage: Fills a rectangular band with procedurally generated rock silhouettes, for dressing\n"
                   "2.5D levels - foreground occluders between camera and player, and ridges behind him.\n\n"
                   "LAYOUT:\n"
                   "- The band is a rectangle in this GameObject's own XY plane, centred on its node. Move,\n"
                   "  rotate and scale the object to place it; there is nothing to click or draw.\n"
                   "- 'Band Width' is its horizontal extent. 'Columns' is how many rock clusters fit across.\n"
                   "- 'Grow Direction' picks the axis the rock height grows along: Up (+Y) for rocks standing\n"
                   "  on a floor, Down (-Y) for stalactites on a ceiling, Towards Camera (-Z) or Away From\n"
                   "  Camera (+Z) for mats growing out of the play plane. The band always runs along local X.\n"
                   "- 'Rows' are DEPTH, not stacking. Row 0 sits on the object's own plane, every further row\n"
                   "  is pushed along the remaining axis by 'Row Depth Spacing', scaled by 'Row Scale' and\n"
                   "  shaded towards 'Back Brightness'. Overlapping rows at different brightness are what read\n"
                   "  as a ridge; more rocks at one brightness only read as noise.\n"
                   "- 'Row Depth Spacing' may be NEGATIVE. With Grow Direction Up, the camera looks along +Z,\n"
                   "  so a negative value brings the rows TOWARDS the camera - which is what a foreground\n"
                   "  occluder band in front of the player wants. Positive pushes them away, behind him.\n"
                   "  Row 0 always stays on the object's own plane, so with a negative spacing row 0 is the\n"
                   "  REARMOST row: swap Front and Back Brightness in that case.\n\n"
                   "SHAPE:\n"
                   "- 'Rock Style': Jagged (broken cave rock), Rounded (weathered humps), Columnar (flat\n"
                   "  topped basalt steps), Stalagmite (one narrow spike per cell).\n"
                   "- 'Rock Height' is the height of a row 0 cluster, before variation.\n"
                   "- 'Rock Depth' is how thick each cluster is. Keep it small for foreground bands:\n"
                   "  they are silhouettes, nobody sees their sides.\n"
                   "- 'Rock Width Scale' above 1 makes neighbouring clusters overlap, which is what removes\n"
                   "  the regular gaps that give a grid away. 1.35 is a good starting point. A cluster is\n"
                   "  never allowed to end up narrower than its cell plus the jitter, so this value cannot\n"
                   "  open a gap however it is combined with Width Jitter.\n"
                   "- 'Base Fill' is the fraction of Rock Height the outline never drops below. At 0 the\n"
                   "  Rounded and Stalagmite outlines fall to zero at their edges, so two overlapping\n"
                   "  clusters meet at zero height and a notch shows through anyway. 0.3-0.4 welds the band\n"
                   "  into a continuous ridge; use 0 for free standing spikes with real sky between them.\n"
                   "- 'Height Variation' and 'Width Jitter' break the grid up; without them a band reads as\n"
                   "  wallpaper however good the individual rock is.\n"
                   "- 'Density' below 1 drops random clusters, leaving gaps to see through.\n"
                   "- 'Profile Points' is the outline resolution per cluster. 8-16 is plenty for a silhouette.\n"
                   "- 'Row Scale' shrinks the back rows in HEIGHT and DEPTH only, never in width - scaling the\n"
                   "  width without scaling the cell spacing would open a gap in every row behind the first.\n\n"
                   "SHADING:\n"
                   "- 'Front Brightness' and 'Back Brightness' multiply the datablock's diffuse for the first\n"
                   "  and last row, with the rows in between interpolated. This is the whole point of the\n"
                   "  component: a foreground occluder wants roughly 0.1, a background ridge 0.3-0.5, while\n"
                   "  the play plane stays at full brightness. Scene fog then does the rest.\n"
                   "- The datablock is CLONED per row, so the original and everything else using it stay\n"
                   "  untouched.\n\n"
                   "DETERMINISM:\n"
                   "- 'Seed' picks the layout. The same seed always gives the same band, on every machine,\n"
                   "  so nothing of the geometry is ever written to the scene file - only these attributes.\n\n"
                   "LUA API:\n"
                   "- getProceduralDecorBandComponent() on a GameObject returns this component.\n"
                   "- setSeed(s), setDensity(d), setRockHeight(h), setFrontBrightness(b), setBackBrightness(b).\n"
                   "- setBrightness(front, back) changes only materials and needs no rebuild.\n"
                   "- regenerate() rebuilds the band after changing several values at once.\n";
        }

        static std::optional<NOWA::GameObjectTypeDescriptor> getStaticTypeDescriptor()
        {
            NOWA::GameObjectTypeDescriptor desc;
            desc.type = eType::ITEM;
            desc.displayName = "Decor Band";
            desc.meshToDisplay = "Node.mesh";
            desc.needsMeshItem = false;
            desc.enterMeshModifyMode = false;
            desc.autoComponents = {"ProceduralDecorBandComponent"};
            desc.guardWithPluginCheck = true;
            return desc;
        }

        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

        virtual Ogre::String getClassName(void) const override
        {
            return "ProceduralDecorBandComponent";
        }

        virtual Ogre::String getParentClassName(void) const override
        {
            return "GameObjectComponent";
        }

        // ── Attribute access ─────────────────────────────────────────────────────────

        void setActivated(bool activated);

        bool isActivated(void) const;

        void setBandWidth(Ogre::Real width);

        Ogre::Real getBandWidth(void) const;

        void setRows(int rows);

        int getRows(void) const;

        void setColumns(int columns);

        int getColumns(void) const;

        void setRowDepthSpacing(Ogre::Real spacing);

        Ogre::Real getRowDepthSpacing(void) const;

        void setRowScale(Ogre::Real scale);

        Ogre::Real getRowScale(void) const;

        /**
         * @brief Axis the rock height grows along: "Up (+Y)", "Down (-Y)", "Towards Camera (-Z)"
         *        or "Away From Camera (+Z)".
         */
        void setGrowDirection(const Ogre::String& direction);

        Ogre::String getGrowDirection(void) const;

        GrowDirection getGrowDirectionEnum(void) const;

        void setRockStyle(const Ogre::String& style);

        Ogre::String getRockStyle(void) const;

        RockStyle getRockStyleEnum(void) const;

        void setRockHeight(Ogre::Real height);

        Ogre::Real getRockHeight(void) const;

        void setRockDepth(Ogre::Real depth);

        Ogre::Real getRockDepth(void) const;

        void setRockWidthScale(Ogre::Real scale);

        Ogre::Real getRockWidthScale(void) const;

        /**
         * @brief Fraction of Rock Height the outline never drops below, 0 to 1. This is what
         *        keeps overlapping clusters welded together: an outline that falls to zero at
         *        its edges leaves a notch even where two clusters overlap.
         */
        void setBaseFill(Ogre::Real fill);

        Ogre::Real getBaseFill(void) const;

        void setHeightVariation(Ogre::Real variation);

        Ogre::Real getHeightVariation(void) const;

        void setWidthJitter(Ogre::Real jitter);

        Ogre::Real getWidthJitter(void) const;

        void setDensity(Ogre::Real density);

        Ogre::Real getDensity(void) const;

        void setProfilePoints(int points);

        int getProfilePoints(void) const;

        void setSeed(int seed);

        int getSeed(void) const;

        void setDatablockName(const Ogre::String& datablockName);

        Ogre::String getDatablockName(void) const;

        /**
         * @brief Multiplier on the datablock diffuse for the FIRST row. A foreground occluder
         *        wants something around 0.1 - it is meant to be a silhouette, not a rock the
         *        player can study.
         */
        void setFrontBrightness(Ogre::Real brightness);

        Ogre::Real getFrontBrightness(void) const;

        /**
         * @brief Multiplier for the LAST row; the rows between are interpolated.
         */
        void setBackBrightness(Ogre::Real brightness);

        Ogre::Real getBackBrightness(void) const;

        /**
         * @brief Both brightnesses at once. Touches materials only, never geometry, so it is
         *        cheap enough to drive per frame from a script - fading a foreground band out
         *        while the player stands behind it, for instance.
         */
        void setBrightness(Ogre::Real front, Ogre::Real back);

        void setUVTiling(const Ogre::Vector2& tiling);

        Ogre::Vector2 getUVTiling(void) const;

        void setCastShadows(bool castShadows);

        bool getCastShadows(void) const;

        /**
         * @brief Rebuilds the band. Every setter that changes geometry already does this, so
         *        it is only needed to collapse several changes made from Lua into one rebuild.
         */
        void regenerate(void);

    public:
        // Static attribute names
        static Ogre::String AttrActivated(void)
        {
            return "Activated";
        }
        static Ogre::String AttrBandWidth(void)
        {
            return "Band Width";
        }
        static Ogre::String AttrRows(void)
        {
            return "Rows";
        }
        static Ogre::String AttrColumns(void)
        {
            return "Columns";
        }
        static Ogre::String AttrRowDepthSpacing(void)
        {
            return "Row Depth Spacing";
        }
        static Ogre::String AttrRowScale(void)
        {
            return "Row Scale";
        }
        static Ogre::String AttrGrowDirection(void)
        {
            return "Grow Direction";
        }
        static Ogre::String AttrRockStyle(void)
        {
            return "Rock Style";
        }
        static Ogre::String AttrRockHeight(void)
        {
            return "Rock Height";
        }
        static Ogre::String AttrRockDepth(void)
        {
            return "Rock Depth";
        }
        static Ogre::String AttrRockWidthScale(void)
        {
            return "Rock Width Scale";
        }
        static Ogre::String AttrBaseFill(void)
        {
            return "Base Fill";
        }
        static Ogre::String AttrHeightVariation(void)
        {
            return "Height Variation";
        }
        static Ogre::String AttrWidthJitter(void)
        {
            return "Width Jitter";
        }
        static Ogre::String AttrDensity(void)
        {
            return "Density";
        }
        static Ogre::String AttrProfilePoints(void)
        {
            return "Profile Points";
        }
        static Ogre::String AttrSeed(void)
        {
            return "Seed";
        }
        static Ogre::String AttrDatablockName(void)
        {
            return "Datablock";
        }
        static Ogre::String AttrFrontBrightness(void)
        {
            return "Front Brightness";
        }
        static Ogre::String AttrBackBrightness(void)
        {
            return "Back Brightness";
        }
        static Ogre::String AttrUVTiling(void)
        {
            return "UV Tiling";
        }
        static Ogre::String AttrCastShadows(void)
        {
            return "Cast Shadows";
        }

    private:
        /**
         * @brief Fills the per-row vertex and index buffers from the current attributes. Pure
         *        CPU work, safe on the logic thread.
         */
        void buildGeometry(void);

        /**
         * @brief Resolves Grow Direction into the three local axes everything is built from:
         *        alongAxis is the band's run (always local X), growAxis the direction the rock
         *        height grows along, thickAxis the remaining one - which doubles as the cluster
         *        thickness and the direction the depth rows are pushed along. Deriving the row
         *        axis from the growth axis is what makes it impossible for the two to collide.
         */
        void resolveAxes(Ogre::Vector3& alongAxis, Ogre::Vector3& growAxis, Ogre::Vector3& thickAxis) const;

        /**
         * @brief Generates one rock cluster into the given row's buffers, as a prism: the
         *        silhouette profile extruded along thickAxis, closed front, back, top rim and
         *        ends. Every vertex goes through the one axis mapping, so no face can be left
         *        behind on an old axis when Grow Direction changes.
         *
         * @param rowOrigin   Origin of this row, already offset by Row Depth Spacing.
         * @param centreAlong Cluster centre along the band, measured from the band's centre.
         *                    Kept absolute rather than local so the UVs flow continuously
         *                    across neighbouring clusters instead of restarting per cluster.
         */
        void buildRockCluster(int row, const Ogre::Vector3& rowOrigin, const Ogre::Vector3& alongAxis, const Ogre::Vector3& growAxis, const Ogre::Vector3& thickAxis, Ogre::Real centreAlong, Ogre::Real width, Ogre::Real height, Ogre::Real depth,
            int columnIndex);

        /**
         * @brief The outline of one cluster as normalised heights in 0..1, one per profile
         *        point. This is the only place the four styles differ.
         */
        std::vector<Ogre::Real> buildRockProfile(int row, int columnIndex, int points) const;

        void addDecorQuad(int row, const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& v3, const Ogre::Vector3& normal, const Ogre::Vector2& uv0, const Ogre::Vector2& uv1, const Ogre::Vector2& uv2,
            const Ogre::Vector2& uv3);

        void createDecorMesh(void);

        void createDecorMeshInternal(const std::vector<std::vector<float>>& rowVerts, const std::vector<std::vector<Ogre::uint32>>& rowInds, const std::vector<Ogre::uint32>& rowVertexCounts);

        void destroyDecorMesh(void);

        /**
         * @brief Applies the brightness ramp by cloning the source datablock once per row and
         *        scaling the clone's diffuse. Setting it on the source would darken every other
         *        object sharing that material. Render thread only - it creates Hlms objects.
         */
        void applyDatablocks(void);

        /**
         * @brief Drops every cloned datablock, after pointing the submeshes back at the source.
         *        Destroying a datablock an Item still references is a crash, not a leak.
         *        Render thread only.
         */
        void destroyClonedDatablocks(void);

    private:
        Ogre::String name;

        // Attributes
        Variant* activated;
        Variant* bandWidth;
        Variant* rows;
        Variant* columns;
        Variant* rowDepthSpacing;
        Variant* rowScale;
        Variant* growDirection;
        Variant* rockStyle;
        Variant* rockHeight;
        Variant* rockDepth;
        Variant* rockWidthScale;
        Variant* baseFill;
        Variant* heightVariation;
        Variant* widthJitter;
        Variant* density;
        Variant* profilePoints;
        Variant* seed;
        Variant* datablockName;
        Variant* frontBrightness;
        Variant* backBrightness;
        Variant* uvTiling;
        Variant* castShadows;

        // One buffer set per row, so every row can become its own submesh with its own cloned
        // datablock - which is what the brightness ramp needs.
        std::vector<std::vector<float>> rowVertices;
        std::vector<std::vector<Ogre::uint32>> rowIndices;
        std::vector<Ogre::uint32> rowVertexCounts;

        Ogre::MeshPtr decorMesh;
        Ogre::Item* decorItem;

        // Names of the per-row datablock clones currently alive, and the source they were
        // cloned from. Empty when no clone exists.
        std::vector<Ogre::String> clonedDatablockNames;
        Ogre::String clonedSourceName;

        // Set by clone(); postInit does the single build. A clone has no init() to read XML in
        // and no scene-parsed event left to wait for.
        bool decorClonedNeedsRebuild;
    };

} // namespace NOWA

#endif // PROCEDURAL_DECOR_BAND_COMPONENT_H
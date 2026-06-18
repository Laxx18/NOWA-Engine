/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#ifndef PROCEDURAL_FOLIAGE_VOLUME_COMPONENT_H
#define PROCEDURAL_FOLIAGE_VOLUME_COMPONENT_H

#include "NOWAPrecompiled.h"
#include "OgrePlugin.h"
#include "gameobject/GameObjectComponent.h"
#include "gameobject/GameObjectController.h"
#include "main/Events.h"
#include <unordered_map>
#include <vector>

namespace Ogre
{
    class Terra;
    class Item;
}

namespace NOWA
{
    class PhysicsArtifactComponent;
    class WindComponent;

    /**
     * @struct FoliageRule
     * @brief Rule-based vegetation placement (professional system)
     *
     * Defines HOW and WHERE a specific vegetation type should be placed.
     * Multiple rules can coexist in one volume for complex biomes.
     */
    struct FoliageRule
    {
        // ========== Identity ==========
        Ogre::String name;     // "Oak_Forest", "Alpine_Grass"
        Ogre::String meshName; // "Oak01.mesh"
        Ogre::String resourceGroup = Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME; // "Objects"
        bool enabled;          // Can disable rule without deleting

        // ========== Density & Distribution ==========
        Ogre::Real density;            // Instances per m^2 (0.01 - 10.0)
        Ogre::uint32 seed;             // Seed for this rule (reproducible)
        Ogre::Real minDistanceToSame;  // Min spacing between same type (meters)
        Ogre::Real minDistanceToOther; // Min spacing to ANY other vegetation

        // ========== Scale Variation ==========
        Ogre::Vector2 scaleRange; // (min, max) e.g., (0.8, 1.4)
        bool uniformScale;        // true = X=Y=Z, false = independent axes

        // ========== Terrain Filters ==========
        Ogre::Vector2 heightRange; // (min, max) elevation in meters
        Ogre::Real maxSlope;       // Max slope angle in degrees (0-90)

        // ========== Terra Layer Filters ==========
        std::vector<int> terraLayerThresholds; // [255,200,0,0] = place on layers 1&2

        // ========== Rendering ==========
        Ogre::Real renderDistance; // Visibility distance (0 = infinite)
        Ogre::Real lodDistance;    // LOD transition distance (0 = auto)
        bool castShadows;          // Shadow casting (false for grass)
        Ogre::Real shadowDistance; // Shadow rendering distance

        // ========== Orientation ==========
        Ogre::Real alignToNormal;     // 0.0-1.0, how much to align to terrain normal
        bool randomYRotation;         // Random rotation around Y axis
        Ogre::Vector2 yRotationRange; // (min, max) degrees if not fully random

        // ========== Advanced ==========
        Ogre::Real cullDistance; // Distance to cull entire rule (optimization)
        bool useClusterLOD;      // Enable cluster-based LOD (future)

        Ogre::String categories; // e.g. "All" or "All-House" or "Ground"
        unsigned int categoriesId;
        // Bit(s) explicitly excluded via "-Name" tokens in categories, e.g.
        // "All-Obstacle" -> the Obstacle category bit alone. Computed the same
        // way as GameObjectPlaceComponent::parseExcludedCategories: walk every
        // "-Name" token and OR its generateCategoryId(name) result in. Used to
        // actively search FOR obstacles to reject a position, since
        // categoriesId itself (the "All minus Obstacle" mask) makes obstacles
        // invisible to a query using it directly as the query mask.
        unsigned int excludedCategoryId;
        Ogre::Real clearanceDistance;

        bool collisionEnabled;      // Enable physics collision
        Ogre::Real collisionRadius; // Trunk/stem radius in meters
        Ogre::Real collisionHeight; // Height of collision cylinder

        // ========== Procedural Grass ==========
        // When useProceduralGrass = true the meshName is ignored.
        // Geometry is generated on the CPU as cross-quad blades so no .mesh file is needed.
        // The blades use the HlmsWind datablock (HLMS_USER0) automatically, so they sway
        // whenever a WindComponent is present in the scene.
        bool useProceduralGrass;    // true = cross-quad blade generation instead of mesh
        Ogre::Real bladeWidth;      // Half-width of one grass blade in meters (default 0.15)
        Ogre::Real bladeHeight;     // Height of one grass blade in meters (default 0.5)
        Ogre::String grassMaterialName; // Wind HLMS datablock name (must exist in resources)

        // ========== Procedural Tree (per-branch leaf sway) ==========
        // When useProceduralTree = true, meshName is STILL used (unlike grass,
        // which generates geometry from scratch). Instead, this enables an
        // additional processing pass during createFoliageItems: any submesh
        // identified as "leaves" (see isLeavesSubMesh) gets its vertices
        // clustered into pseudo-branches at cell-build time, and a per-vertex
        // branchId+phase attribute is baked into the merged cell mesh so the
        // Wind HLMS can sway each branch independently and out of phase with
        // its neighbours, rather than the whole canopy moving as one rigid
        // unit. Trunk/bark submeshes are never affected -- they stay rigid.
        bool useProceduralTree;          // true = enable per-branch sway clustering for leaves submeshes
        int treeBranchClusterCount;      // Number of pseudo-branches to cluster leaf vertices into (default 8)
        Ogre::Real treeSwayStrength;     // Multiplier on branch sway displacement (default 1.0)

        FoliageRule() :
            enabled(true),
            density(1.0f),
            seed(0),
            minDistanceToSame(0.0f),
            minDistanceToOther(0.0f),
            scaleRange(Ogre::Vector2(0.8f, 1.2f)),
            uniformScale(true),
            heightRange(Ogre::Vector2(-FLT_MAX, FLT_MAX)),
            maxSlope(90.0f),
            terraLayerThresholds({255, 255, 255, 255}),
            renderDistance(0.0f),
            lodDistance(0.0f),
            castShadows(false),
            shadowDistance(0.0f),
            alignToNormal(1.0f),
            randomYRotation(true),
            yRotationRange(Ogre::Vector2(0.0f, 360.0f)),
            cullDistance(0.0f),
            useClusterLOD(false),
            categories("All"), // Default: grow everywhere
            categoriesId(GameObjectController::ALL_CATEGORIES_ID), // Default: all categories
            excludedCategoryId(0), // Default: nothing explicitly excluded
            clearanceDistance(0.0f), // 0 = disabled
            collisionEnabled(false),
            collisionRadius(0.3f), // 30cm default trunk
            collisionHeight(2.0f), // 2m default height
            useProceduralGrass(false),
            bladeWidth(0.15f),
            bladeHeight(0.5f),
            grassMaterialName("GrassMaterial"),
            useProceduralTree(false),
            treeBranchClusterCount(8),
            treeSwayStrength(1.0f)
        {
        }
    };

    /**
     * @struct VegetationInstance
     * @brief Single vegetation instance data (CPU-side)
     */
    struct VegetationInstance
    {
        Ogre::Vector3 position;
        Ogre::Quaternion orientation;
        Ogre::Vector3 scale;
        size_t ruleIndex; // Which rule created this instance

        VegetationInstance() : ruleIndex(0)
        {
        }
        VegetationInstance(const Ogre::Vector3& pos, const Ogre::Quaternion& rot, const Ogre::Vector3& scl, size_t rule) : position(pos), orientation(rot), scale(scl), ruleIndex(rule)
        {
        }
    };

    /**
     * @struct VegetationBatch
     * @brief Per-rule batch of instances
     */
    struct VegetationBatch
    {
        Ogre::String meshName;
        size_t ruleIndex;
        std::vector<VegetationInstance> instances;

        // Render thread only!
        std::vector<Ogre::Item*> items;
        std::vector<Ogre::SceneNode*> nodes;
        Ogre::HlmsDatablock* sharedDatablock;

        VegetationBatch() : ruleIndex(0), sharedDatablock(nullptr)
        {
        }
    };

    /**
     * @class ProceduralFoliageVolumeComponent
     * @brief Professional rule-based vegetation system
     *
     * FEATURES:
     * - Rule-based placement (height, slope, layers, spacing)
     * - Seed-based reproducibility
     * - Automatic LOD generation via MeshLodGenerator
     * - Ogre-Next Hlms batching (100k instances = ~10 draw calls)
     * - Thread-safe (position calc on main, Ogre on render thread)
     * - Biome-aware via Terra layer filtering
     *
     * USAGE:
     * 1. Set volume bounds (AABB)
     * 2. Add rules (FoliageRule)
     * 3. Call regenerate via executeAction or connect()
     *
     * LIFECYCLE:
     * - connect() -> Generates vegetation (simulation start)
     * - disconnect() -> Optionally clears (simulation stop)
     */
    class EXPORTED ProceduralFoliageVolumeComponent : public GameObjectComponent, public Ogre::Plugin
    {
    public:
        typedef boost::shared_ptr<ProceduralFoliageVolumeComponent> ProceduralFoliageVolumeComponentPtr;

    public:
        ProceduralFoliageVolumeComponent();
        virtual ~ProceduralFoliageVolumeComponent();

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
         * @see		GameObjectComponent::onCloned
         */
        virtual bool onCloned(void) override;

        /**
         * @see		GameObjectComponent::onRemoveComponent
         */
        virtual void onRemoveComponent(void);

        /**
         * @see		GameObjectComponent::onOtherComponentRemoved
         */
        virtual void onOtherComponentRemoved(unsigned int index) override;

        /**
         * @see		GameObjectComponent::onOtherComponentAdded
         */
        virtual void onOtherComponentAdded(unsigned int index) override;

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

         virtual Ogre::String getClassName(void) const override
        {
            return "ProceduralFoliageVolumeComponent";
        }

        virtual Ogre::String getParentClassName(void) const override
        {
            return "GameObjectComponent";
        }

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("ProceduralFoliageVolumeComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "ProceduralFoliageVolumeComponent";
        }

        static Ogre::String getStaticInfoText(void)
        {
            return "Usage: Generates a vegetation foliage with given rules. Add some rules. Best practise: Rule 0 start with trees, "
                   "Rule 2 other trees, Rule 3, go smaller and more dens: bushes and so on. Add a PhysicsArtifactComponent and enable rule collisions for trees for collision hulls.";
        }

        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController);
        static bool canStaticAddComponent(GameObject* gameObject);

        // ========== Volume Settings ==========
        void setVolumeBounds(const Ogre::Aabb& bounds);
        Ogre::Aabb getVolumeBounds(void) const;

        void setMasterSeed(Ogre::uint32 seed);
        Ogre::uint32 getMasterSeed(void) const;

        void setGridResolution(Ogre::Real resolution);
        Ogre::Real getGridResolution(void) const;

        // ========== Rule Management ==========
        void setRuleCount(unsigned int count);
        unsigned int getRuleCount(void) const;

        void setRuleName(unsigned int index, const Ogre::String& name);
        Ogre::String getRuleName(unsigned int index) const;

        void setRuleMeshName(unsigned int index, const Ogre::String& meshName);
        Ogre::String getRuleMeshName(unsigned int index) const;

        void setRuleDensity(unsigned int index, Ogre::Real density);
        Ogre::Real getRuleDensity(unsigned int index) const;

        void setRuleHeightRange(unsigned int index, const Ogre::Vector2& range);
        Ogre::Vector2 getRuleHeightRange(unsigned int index) const;

        void setRuleMaxSlope(unsigned int index, Ogre::Real maxSlope);
        Ogre::Real getRuleMaxSlope(unsigned int index) const;

        void setRuleTerraLayers(unsigned int index, const Ogre::String& layers);
        Ogre::String getRuleTerraLayers(unsigned int index) const;

        void setRuleScaleRange(unsigned int index, const Ogre::Vector2& range);
        Ogre::Vector2 getRuleScaleRange(unsigned int index) const;

        void setRuleMinSpacing(unsigned int index, Ogre::Real spacing);
        Ogre::Real getRuleMinSpacing(unsigned int index) const;

        void setRuleRenderDistance(unsigned int index, Ogre::Real distance);
        Ogre::Real getRuleRenderDistance(unsigned int index) const;

        void setRuleLodDistance(unsigned int index, Ogre::Real distance);
        Ogre::Real getRuleLodDistance(unsigned int index) const;

        void setRuleCastShadows(unsigned int index, bool castShadows);
        bool getRuleCastShadows(unsigned int index) const;

        void setRuleShadowDistance(unsigned int index, Ogre::Real shadowDistance);
        Ogre::Real getRuleShadowDistance(unsigned int index) const;

        void setRuleCategories(unsigned int index, const Ogre::String& categories);
        Ogre::String getRuleCategories(unsigned int index) const;

        void setRuleClearanceDistance(unsigned int index, Ogre::Real clearanceDistance);
        Ogre::Real getRuleClearanceDistance(unsigned int index) const;

        void setRuleCollisionEnabled(unsigned int index, bool enabled);
        bool getRuleCollisionEnabled(unsigned int index) const;
        
        void setRuleCollisionRadius(unsigned int index, Ogre::Real radius);
        Ogre::Real getRuleCollisionRadius(unsigned int index) const;

        void setRuleCollisionHeight(unsigned int index, Ogre::Real height);
        Ogre::Real getRuleCollisionHeight(unsigned int index) const;

        void setRuleUseProceduralGrass(unsigned int index, bool useGrass);
        bool getRuleUseProceduralGrass(unsigned int index) const;

        void setRuleBladeWidth(unsigned int index, Ogre::Real width);
        Ogre::Real getRuleBladeWidth(unsigned int index) const;

        void setRuleBladeHeight(unsigned int index, Ogre::Real height);
        Ogre::Real getRuleBladeHeight(unsigned int index) const;

        void setRuleGrassMaterialName(unsigned int index, const Ogre::String& materialName);
        Ogre::String getRuleGrassMaterialName(unsigned int index) const;

        void setRuleUseProceduralTree(unsigned int index, bool useTree);
        bool getRuleUseProceduralTree(unsigned int index) const;

        void setRuleTreeBranchClusterCount(unsigned int index, int clusterCount);
        int getRuleTreeBranchClusterCount(unsigned int index) const;

        void setRuleTreeSwayStrength(unsigned int index, Ogre::Real swayStrength);
        Ogre::Real getRuleTreeSwayStrength(unsigned int index) const;
    public:
        static const Ogre::String AttrRegenerate(void)
        {
            return "Regenerate";
        }
        static const Ogre::String AttrClear(void)
        {
            return "Clear";
        }
        static const Ogre::String AttrRandomizeSeed(void)
        {
            return "Randomize Seed";
        }
        static const Ogre::String AttrVolumeBounds(void)
        {
            return "Volume Bounds";
        }
        static const Ogre::String AttrMasterSeed(void)
        {
            return "Master Seed";
        }
        static const Ogre::String AttrGridResolution(void)
        {
            return "Grid Resolution";
        }
        static const Ogre::String AttrRuleCount(void)
        {
            return "Rule Count";
        }
        // Rule attributes (dynamic, based on rule count)
        static const Ogre::String AttrRuleName(void)
        {
            return "Rule Name ";
        }
        static const Ogre::String AttrRuleMeshName(void)
        {
            return "Rule Mesh ";
        }
        static const Ogre::String AttrRuleDensity(void)
        {
            return "Rule Density ";
        }
        static const Ogre::String AttrRuleHeightRange(void)
        {
            return "Rule Height Range ";
        }
        static const Ogre::String AttrRuleMaxSlope(void)
        {
            return "Rule Max Slope ";
        }
        static const Ogre::String AttrRuleTerraLayers(void)
        {
            return "Rule Terra Layers ";
        }
        static const Ogre::String AttrRuleScaleRange(void)
        {
            return "Rule Scale Range ";
        }
        static const Ogre::String AttrRuleMinSpacing(void)
        {
            return "Rule Min Spacing ";
        }
        static const Ogre::String AttrRuleRenderDistance(void)
        {
            return "Rule Render Distance ";
        }
        static const Ogre::String AttrRuleLodDistance(void)
        {
            return "Rule LOD Distance ";
        }
        static const Ogre::String AttrRuleCastShadows(void)
        {
            return "Rule Cast Shadows ";
        }
        static const Ogre::String AttrRuleShadowDistance(void)
        {
            return "Rule Shadow Distance ";
        }
        static const Ogre::String AttrRuleCategories(void)
        {
            return "Rule Categories ";
        }
        static const Ogre::String AttrRuleClearanceDistance(void)
        {
            return "Rule Clearance Distance ";
        }
        static const Ogre::String AttrRuleResourceGroup(void)
        {
            return "Rule Resource Group ";
        }
        static const Ogre::String AttrRuleCollisionEnabled(void)
        {
            return "Rule Collision Enabled ";
        }
        static const Ogre::String AttrRuleCollisionRadius(void)
        {
            return "Rule Collision Radius ";
        }
        static const Ogre::String AttrRuleCollisionHeight(void)
        {
            return "Rule Collision Height ";
        }
        static const Ogre::String AttrRuleUseProceduralGrass(void)
        {
            return "Rule Use Procedural Grass ";
        }
        static const Ogre::String AttrRuleBladeWidth(void)
        {
            return "Rule Blade Width ";
        }
        static const Ogre::String AttrRuleBladeHeight(void)
        {
            return "Rule Blade Height ";
        }
        static const Ogre::String AttrRuleGrassMaterialName(void)
        {
            return "Rule Grass Material ";
        }
        static const Ogre::String AttrRuleUseProceduralTree(void)
        {
            return "Rule Use Procedural Tree ";
        }
        static const Ogre::String AttrRuleTreeBranchClusterCount(void)
        {
            return "Rule Tree Branch Cluster Count ";
        }
        static const Ogre::String AttrRuleTreeSwayStrength(void)
        {
            return "Rule Tree Sway Strength ";
        }
    private:
        virtual bool executeAction(const Ogre::String& actionId, NOWA::Variant* attribute) override;
        /**
         * @brief Main generation function (runs on main thread)
         */
        void regenerateFoliage();

        /**
         * @brief Clear all vegetation (blocking, waits for render thread)
         */
        void clearFoliage();

        /**
         * @brief Calculate positions for all rules (parallel, main thread)
         */
        std::vector<VegetationBatch> calculateFoliagePositions();

        /**
         * @brief Check if position meets rule's terrain criteria
         */
        bool meetsTerrainCriteria(const Ogre::Vector3& position, const Ogre::Vector3& normal, const FoliageRule& rule, Ogre::Terra* terra);

        /**
         * @brief Check minimum spacing constraints
         */
        bool hasMinimumSpacing(const Ogre::Vector3& position, const FoliageRule& rule, const std::vector<VegetationInstance>& existingInstances);

        /**
         * @brief Calculate terrain normal at position
         */
        Ogre::Vector3 calculateTerrainNormal(const Ogre::Vector3& position, Ogre::Terra* terra);

        /**
         * @brief Dispatch to render thread
         */
        void createFoliageOnRenderThread(std::vector<VegetationBatch>&& batches);

        /**
         * @brief Create Ogre Items on render thread
         */
        void createFoliageItems(std::vector<VegetationBatch>& batches);

        void createFoliageCollision(void);

        /**
         * @brief Generate procedural cross-quad grass geometry for one rule batch.
         *        Called from createFoliageItems when rule.useProceduralGrass == true.
         *        Grass blades are grouped into 10m spatial cells (same as mesh foliage)
         *        so frustum culling and render distance work correctly.
         *        Blades use the Wind HLMS datablock so they sway when a WindComponent exists.
         */
        void createGrassItems(VegetationBatch& batch, const FoliageRule& rule);

        /**
         * @brief Generate LOD for mesh (render thread)
         */
        void generateLODForMesh(const Ogre::String& meshName, Ogre::Real lodDistance, const Ogre::String& resourceGroup);

        /**
         * @brief Destroy vegetation on render thread
         */
        void destroyFoliageOnRenderThread();

        /**
         * @brief Parse terra layer string "255,200,0,0"
         */
        std::vector<int> parseTerraLayers(const Ogre::String& layersStr);

        /**
         * @brief Check if position is within Terra bounds
         */
        bool isWithinTerraBounds(const Ogre::Vector3& position, Ogre::Terra* terra) const;

        bool isCategoryAllowed(const Ogre::Vector3& position, const FoliageRule& rule);

        /**
         * @brief Parses every "-Name" token out of a categories string (e.g.
         *        "All-Obstacle" -> "Obstacle") and ORs together
         *        generateCategoryId(name) for each one. Mirrors
         *        GameObjectPlaceComponent::parseExcludedCategories exactly.
         *        Used to get the bit(s) to actively search FOR when rejecting
         *        a placement position, since categoriesId itself (the
         *        "All minus excluded" mask) makes excluded objects invisible
         *        to a query that uses it directly as the query mask.
         */
        unsigned int parseExcludedCategoryId(const Ogre::String& categories);

        /**
         * @brief Determines whether a submesh should be treated as "leaves"
         *        for the purposes of useProceduralTree per-branch sway
         *        clustering. Combines the transparency signal (now including
         *        alpha_test/alpha_hash, not just real alpha blending) with
         *        datablock name pattern matching, since neither signal alone
         *        is reliable on every tree asset -- see implementation for
         *        the specific false-positive case this resolves.
         * @param[in] hasTransparency Whether the submesh's blendblock or
         *            alpha test/hashing settings indicate transparency.
         * @param[in] datablock The submesh's datablock (may be nullptr).
         * @return true if this submesh should get branch-sway treatment.
         */
        bool isLeavesSubMesh(bool hasTransparency, Ogre::HlmsDatablock* datablock);

        /**
         * @brief Clusters leaf-submesh vertex positions into pseudo-branches
         *        for per-branch wind sway (useProceduralTree). Pure spatial
         *        clustering -- no Blender re-export or pre-baked branch data
         *        required. Operates on LOCAL-space positions of a single
         *        source mesh submesh, so the result is identical for every
         *        instance of that mesh and only needs to be computed once
         *        per source mesh (not per cell, not per instance).
         *
         * Algorithm: simple greedy farthest-point seeding followed by one
         * nearest-centroid assignment pass (a lightweight single-iteration
         * k-means, sufficient for visually plausible branch grouping --
         * exact optimality does not matter for a wind sway effect).
         *
         * @param[in]  positions Local-space vertex positions of the leaves submesh.
         * @param[in]  clusterCount Number of pseudo-branches to create (rule.treeBranchClusterCount).
         * @param[out] outBranchIds Per-vertex branch index in [0, clusterCount).
         * @param[out] outBranchPivots Per-branch centroid position (local space), size == clusterCount.
         */
        void clusterLeafVerticesIntoBranches(
            const std::vector<Ogre::Vector3>& positions,
            int clusterCount,
            std::vector<int>& outBranchIds,
            std::vector<Ogre::Vector3>& outBranchPivots);

        /**
         * @brief Derives the expected "Swaying" Wind HLMS datablock name for
         *        a given original PBS datablock name, per the naming
         *        convention documented in SwayingTreeLeavesMaterials.material:
         *        the original full datablock name with the literal suffix
         *        "Swaying" appended, no other transformation. Example:
         *        "/dural/plants/trees/birch/texture/leaves_fall" becomes
         *        "/dural/plants/trees/birch/texture/leaves_fallSwaying".
         * @param[in] originalDatablockName The original PBS datablock's name.
         * @return The expected Swaying datablock name to look up.
         */
        Ogre::String getSwayingDatablockName(const Ogre::String& originalDatablockName) const;

        /**
         * @brief Looks up the Swaying Wind HLMS datablock matching a leaves
         *        submesh's original datablock, per getSwayingDatablockName.
         *        Requires a WindComponent to be present in the scene (same
         *        gating as procedural grass) -- there is no point swapping
         *        to a Wind datablock if nothing will drive the sway uniform.
         *        Returns nullptr (with a logged warning) if no WindComponent
         *        exists, or if no matching Swaying datablock was authored
         *        for this particular leaves texture -- per-branch sway is
         *        opt-in per tree texture, not automatic for every tree
         *        asset. The caller should keep the original datablock
         *        unchanged when this returns nullptr.
         * @param[in] originalDatablock The leaves submesh's original PBS datablock.
         * @return The matching Wind datablock, or nullptr if unavailable.
         */
        Ogre::HlmsDatablock* resolveSwayingLeavesDatablock(Ogre::HlmsDatablock* originalDatablock);

        std::vector<VegetationBatch> calculatePlanetFoliagePositions(GameObject* planetGo, Ogre::Real planetRadius, const Ogre::Vector3& planetCentre);

        bool meetsPlanetCriteria(Ogre::Real heightAboveSeaLevel, const Ogre::Vector3& hitNormal, const Ogre::Vector3& outwardDir, const FoliageRule& rule) const;

    private:
        /**
         * @brief One-shot handler for EventDataSceneParsed. Fires once the
         *        engine itself considers the scene fully valid and
         *        renderable for the
         *        engine-side counterpart). This is the actual trigger point
         *        for regenerating foliage loaded from a saved scene -- not
         *        lateInit() itself. Removes its own listener immediately so
         *        it never fires again.
         */
        void handleSceneParsed(NOWA::EventDataPtr eventData);

        void handleGeometryChanged(NOWA::EventDataPtr eventData);
    private:
        Ogre::String name;

        // Volume settings
        Variant* volumeBounds;      // AABB bounds
        Variant* masterSeed;        // Master seed for reproducibility
        Variant* gridResolution;    // Sample grid resolution (meters)

        // Actions
        Variant* regenerate;
        Variant* clear;
        Variant* randomizeSeed;


        // Rules
        Variant* ruleCount;
        std::vector<FoliageRule> rules;

        // Rule variant arrays (dynamic based on rule count)
        std::vector<Variant*> ruleNames;
        std::vector<Variant*> ruleMeshNames;
        std::vector<Variant*> ruleDensities;
        std::vector<Variant*> ruleHeightRanges;
        std::vector<Variant*> ruleMaxSlopes;
        std::vector<Variant*> ruleTerraLayers;
        std::vector<Variant*> ruleScaleRanges;
        std::vector<Variant*> ruleMinSpacings;
        std::vector<Variant*> ruleRenderDistances;
        std::vector<Variant*> ruleLodDistances;
        std::vector<Variant*> ruleCastShadows;
        std::vector<Variant*> ruleShadowDistances;
        std::vector<Variant*> ruleCategories;
        std::vector<Variant*> ruleClearanceDistances;
        std::vector<Variant*> ruleCollisionEnabled;
        std::vector<Variant*> ruleCollisionRadius;
        std::vector<Variant*> ruleCollisionHeight;

        // Procedural grass per-rule Variant arrays
        std::vector<Variant*> ruleUseProceduralGrass;
        std::vector<Variant*> ruleBladeWidths;
        std::vector<Variant*> ruleBladeHeights;
        std::vector<Variant*> ruleGrassMaterialNames;

        // Procedural tree per-rule Variant arrays
        std::vector<Variant*> ruleUseProceduralTree;
        std::vector<Variant*> ruleTreeBranchClusterCounts;
        std::vector<Variant*> ruleTreeSwayStrengths;

        // Runtime state
        std::vector<VegetationBatch> vegetationBatches; // Render thread only after creation!
        bool isDirty;

        bool foliageLoadedFromScene;

        // WindComponent pointer -- resolved once in postInit, used by grass sway logic.
        // Weak reference: if the WindComponent is removed from the scene the pointer
        // becomes stale; we re-resolve it at the next regenerate call.
        WindComponent* windComponent;

        // Spatial hash for spacing checks (optimization)
        std::unordered_map<int64_t, std::vector<VegetationInstance*>> spatialHash;
        Ogre::Real spatialHashCellSize;
        Ogre::RaySceneQuery* raySceneQuery;
        Ogre::SphereSceneQuery* sphereSceneQuery;
        PhysicsArtifactComponent* physicsArtifactComponent;
    };

}; // namespace end

#endif

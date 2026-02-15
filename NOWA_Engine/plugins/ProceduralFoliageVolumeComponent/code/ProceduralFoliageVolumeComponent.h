/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#ifndef PROCEDURAL_FOLIAGE_VOLUME_COMPONENT_H
#define PROCEDURAL_FOLIAGE_VOLUME_COMPONENT_H

#include "NOWAPrecompiled.h"
#include "OgrePlugin.h"
#include "gameobject/GameObjectComponent.h"
#include <unordered_map>
#include <vector>

namespace Ogre
{
    class Terra;
    class Item;
}

namespace NOWA
{
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
        Ogre::Real density;            // Instances per m² (0.01 - 10.0)
        Ogre::uint32 seed;             // Seed for this rule (reproducible)
        Ogre::Real minDistanceToSame;  // Min spacing between same type (meters)
        Ogre::Real minDistanceToOther; // Min spacing to ANY other vegetation

        // ========== Scale Variation ==========
        Ogre::Vector2 scaleRange; // (min, max) e.g., (0.8, 1.4)
        bool uniformScale;        // true = X=Y=Z, false = independent axes

        // ========== Terrain Filters ==========
        Ogre::Vector2 heightRange; // (min, max) elevation in meters
        Ogre::Real maxSlope;       // Max slope angle in degrees (0-90)
        Ogre::Vector2 slopeRange;  // (min, max) for specific slope ranges

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
            slopeRange(Ogre::Vector2(0.0f, 90.0f)),
            terraLayerThresholds({255, 255, 255, 255}),
            renderDistance(0.0f),
            lodDistance(0.0f),
            castShadows(false),
            shadowDistance(0.0f),
            alignToNormal(1.0f),
            randomYRotation(true),
            yRotationRange(Ogre::Vector2(0.0f, 360.0f)),
            cullDistance(0.0f),
            useClusterLOD(false)
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

        // ========== Generation Control ==========
        void setAutoGenerateOnConnect(bool autoGenerate);
        bool getAutoGenerateOnConnect(void) const;

        void setClearOnDisconnect(bool clearOnDisconnect);
        bool getClearOnDisconnect(void) const;

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
        static const Ogre::String AttrAutoGenerate(void)
        {
            return "Auto Generate On Connect";
        }
        static const Ogre::String AttrClearOnDisconnect(void)
        {
            return "Clear On Disconnect";
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

    protected:
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

        Ogre::String extractResourceGroupFromPath(const Ogre::String& path);
    protected:
        Ogre::String name;

        // Volume settings
        Variant* volumeBounds;      // AABB bounds
        Variant* masterSeed;        // Master seed for reproducibility
        Variant* gridResolution;    // Sample grid resolution (meters)
        Variant* autoGenerate;      // Auto-generate on connect()
        Variant* clearOnDisconnect; // Clear on disconnect()

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

        // Runtime state
        std::vector<VegetationBatch> vegetationBatches; // Render thread only after creation!
        bool isDirty;

        // Spatial hash for spacing checks (optimization)
        std::unordered_map<int64_t, std::vector<VegetationInstance*>> spatialHash;
        Ogre::Real spatialHashCellSize;
    };

}; // namespace end

#endif

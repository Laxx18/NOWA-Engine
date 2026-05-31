/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#ifndef PROCEDURAL_PLANET_COMPONENT_H
#define PROCEDURAL_PLANET_COMPONENT_H

#include "OgrePlugin.h"

#include "gameobject/GameObjectComponent.h"
#include "gameobject/PlanetTerraComponentBase.h"

#include <random>
#include <vector>

namespace NOWA
{
    /**
     * @brief Procedural planet surface generation component.
     *
     * Must be placed on the same GameObject as PlanetTerraComponent.
     * Clicking "Generate" runs the full noise + feature pipeline on the CPU,
     * then writes the result into the sibling PlanetTerraComponentBase via
     * setPlanetData(). The PlanetTerraComponent then uploads to GPU automatically.
     *
     * Pipeline order:
     *   1. Perlin FBM noise  -> base geoid heightfield in spherical UV space
     *   2. Island mask       -> optional radial falloff for ocean planets
     *   3. Canyon carving    -> long trench paths carved across the surface
     *   4. River carving     -> flow-accumulation drainage channels
     *   5. Road flattening   -> Catmull-Rom paths with cosine-falloff levelling
     *   6. Normals + tangents recomputed inside PlanetTerra
     *   7. GPU upload
     *
     * The generated heightData[] is stored in normalized world-space offsets
     * above/below the sphere radius (metres), matching PlanetTerra convention.
     */
    class EXPORTED ProceduralPlanetComponent : public GameObjectComponent, public Ogre::Plugin
    {
    public:
        // Attribute name constants
        static Ogre::String AttrHillAmplitude()
        {
            return "HillAmplitude";
        }
        static Ogre::String AttrHillFrequency()
        {
            return "HillFrequency";
        }
        static Ogre::String AttrOctaves()
        {
            return "Octaves";
        }
        static Ogre::String AttrPersistence()
        {
            return "Persistence";
        }
        static Ogre::String AttrLacunarity()
        {
            return "Lacunarity";
        }
        static Ogre::String AttrSeed()
        {
            return "Seed";
        }
        static Ogre::String AttrEnableIsland()
        {
            return "EnableIsland";
        }
        static Ogre::String AttrIslandFalloff()
        {
            return "IslandFalloff";
        }
        static Ogre::String AttrIslandSize()
        {
            return "IslandSize";
        }
        static Ogre::String AttrEnableRivers()
        {
            return "EnableRivers";
        }
        static Ogre::String AttrRiverCount()
        {
            return "RiverCount";
        }
        static Ogre::String AttrRiverWidth()
        {
            return "RiverWidth";
        }
        static Ogre::String AttrRiverDepth()
        {
            return "RiverDepth";
        }
        static Ogre::String AttrRiverMeandering()
        {
            return "RiverMeandering";
        }
        static Ogre::String AttrEnableCanyons()
        {
            return "EnableCanyons";
        }
        static Ogre::String AttrCanyonCount()
        {
            return "CanyonCount";
        }
        static Ogre::String AttrCanyonWidth()
        {
            return "CanyonWidth";
        }
        static Ogre::String AttrCanyonDepth()
        {
            return "CanyonDepth";
        }
        static Ogre::String AttrCanyonSteepness()
        {
            return "CanyonSteepness";
        }
        static Ogre::String AttrEnableRoads()
        {
            return "EnableRoads";
        }
        static Ogre::String AttrRoadCount()
        {
            return "RoadCount";
        }
        static Ogre::String AttrRoadWidth()
        {
            return "RoadWidth";
        }
        static Ogre::String AttrRoadDepth()
        {
            return "RoadDepth";
        }
        static Ogre::String AttrRoadSmoothness()
        {
            return "RoadSmoothness";
        }
        static Ogre::String AttrRoadCurviness()
        {
            return "RoadCurviness";
        }
        static Ogre::String AttrGenerate()
        {
            return "Generate";
        }
        static Ogre::String AttrLayerSandThresh()
        {
            return "LayerSandThresh";
        }
        static Ogre::String AttrLayerGrassThresh()
        {
            return "LayerGrassThresh";
        }
        static Ogre::String AttrLayerRockThresh()
        {
            return "LayerRockThresh";
        }
        static Ogre::String AttrLayerSlopeRock()
        {
            return "LayerSlopeRock";
        }

        static Ogre::String ActionGenerate()
        {
            return "ProceduralPlanetComponent.Generate";
        }

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("ProceduralPlanetComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "ProceduralPlanetComponent";
        }

    public:
        ProceduralPlanetComponent();
        virtual ~ProceduralPlanetComponent();

        virtual void install(const Ogre::NameValuePairList* options) override;
        virtual void initialise() override;
        virtual void shutdown() override;
        virtual void uninstall() override;
        virtual const Ogre::String& getName() const override;
        virtual void getAbiCookie(Ogre::AbiCookie& outAbiCookie) override;

        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;
        virtual bool postInit(void) override;
        virtual bool connect(void) override;
        virtual bool disconnect(void) override;
        virtual bool onCloned(void) override;
        virtual void onAddComponent(void) override;
        virtual void onRemoveComponent(void) override;
        virtual void onOtherComponentRemoved(unsigned int index) override;
        virtual void onOtherComponentAdded(unsigned int index) override;
        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;
        virtual void update(Ogre::Real dt, bool notSimulating) override;
        virtual void actualizeValue(Variant* attribute) override;
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        virtual Ogre::String getClassName(void) const override;
        virtual Ogre::String getParentClassName(void) const override;

        static std::optional<NOWA::GameObjectTypeDescriptor> getStaticTypeDescriptor();
        static bool canStaticAddComponent(GameObject* gameObject);

        /**
         * @brief Handles the Generate action button from the NOWA-Design property panel.
         */
        virtual bool executeAction(const Ogre::String& actionId, NOWA::Variant* attribute) override;

        // Attribute setters / getters

        /** @brief Hill height variation in world units (metres above/below sphere surface). */
        void setHillAmplitude(Ogre::Real amplitude);
        Ogre::Real getHillAmplitude(void) const;

        /** @brief Number of major terrain features across the sphere. */
        void setHillFrequency(Ogre::Real frequency);
        Ogre::Real getHillFrequency(void) const;

        /** @brief Number of FBM octaves (detail layers). */
        void setOctaves(Ogre::uint32 octaves);
        Ogre::uint32 getOctaves(void) const;

        /** @brief Amplitude decay per octave (0..1). Lower = smoother. */
        void setPersistence(Ogre::Real persistence);
        Ogre::Real getPersistence(void) const;

        /** @brief Frequency multiplier per octave. 2.0 is typical. */
        void setLacunarity(Ogre::Real lacunarity);
        Ogre::Real getLacunarity(void) const;

        /** @brief Random seed. Change to get a different planet layout. */
        void setSeed(Ogre::uint32 seed);
        Ogre::uint32 getSeed(void) const;

        void setEnableIsland(bool enable);
        bool getEnableIsland(void) const;

        /** @brief Controls how steeply terrain drops toward the ocean edge. */
        void setIslandFalloff(Ogre::Real falloff);
        Ogre::Real getIslandFalloff(void) const;

        /** @brief Fraction of the sphere that forms land (0..1). */
        void setIslandSize(Ogre::Real size);
        Ogre::Real getIslandSize(void) const;

        void setEnableRivers(bool enable);
        bool getEnableRivers(void) const;

        void setRiverCount(Ogre::uint32 count);
        Ogre::uint32 getRiverCount(void) const;

        /** @brief River channel width in world units. */
        void setRiverWidth(Ogre::Real width);
        Ogre::Real getRiverWidth(void) const;

        /** @brief River carve depth in world units. */
        void setRiverDepth(Ogre::Real depth);
        Ogre::Real getRiverDepth(void) const;

        /** @brief Meandering factor 0 = steepest descent, 1 = wandering. */
        void setRiverMeandering(Ogre::Real meandering);
        Ogre::Real getRiverMeandering(void) const;

        void setEnableCanyons(bool enable);
        bool getEnableCanyons(void) const;

        void setCanyonCount(Ogre::uint32 count);
        Ogre::uint32 getCanyonCount(void) const;

        /** @brief Canyon width scale in world units. */
        void setCanyonWidth(Ogre::Real width);
        Ogre::Real getCanyonWidth(void) const;

        /** @brief Canyon carve depth in world units. */
        void setCanyonDepth(Ogre::Real depth);
        Ogre::Real getCanyonDepth(void) const;

        /** @brief Canyon wall profile: lower = U-shape, higher = V-shape. */
        void setCanyonSteepness(Ogre::Real steepness);
        Ogre::Real getCanyonSteepness(void) const;

        void setEnableRoads(bool enable);
        bool getEnableRoads(void) const;

        void setRoadCount(Ogre::uint32 count);
        Ogre::uint32 getRoadCount(void) const;

        /** @brief Road width in world units. */
        void setRoadWidth(Ogre::Real width);
        Ogre::Real getRoadWidth(void) const;

        /** @brief How deep roads cut into the surface in world units. */
        void setRoadDepth(Ogre::Real depth);
        Ogre::Real getRoadDepth(void) const;

        /** @brief Cosine blend zone width multiplier (1.5..4.0). */
        void setRoadSmoothness(Ogre::Real smoothness);
        Ogre::Real getRoadSmoothness(void) const;

        /** @brief Road curvature 0 = straight, 1 = very curvy. */
        void setRoadCurviness(Ogre::Real curviness);
        Ogre::Real getRoadCurviness(void) const;

        /**
         * @brief Runs the full generation pipeline and applies the result to
         *        the sibling PlanetTerraComponentBase.
         */
        void generateProceduralPlanet(void);

        void setLayerSandThresh(Ogre::Real v);
        Ogre::Real getLayerSandThresh(void) const;
        void setLayerGrassThresh(Ogre::Real v);
        Ogre::Real getLayerGrassThresh(void) const;
        void setLayerRockThresh(Ogre::Real v);
        Ogre::Real getLayerRockThresh(void) const;
        void setLayerSopeRock(Ogre::Real v);
        Ogre::Real getLayerSlopeRock(void) const;

        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

        static Ogre::String getStaticInfoText(void)
        {
            return "Generates a procedural planet surface on a sibling PlanetTerraComponent. "
                   "Supports Perlin FBM hills, island masking, canyon carving, river drainage and road flattening. "
                   "Click Generate to apply. Parameters are preserved in the scene XML.";
        }

    private:
        // Internal structures (same concept as ProceduralTerrainCreationComponent)

        struct RoadPoint
        {
            Ogre::Vector2 uv;   // position in [0,1]x[0,1] UV space
            float targetHeight; // levelled height in world units
        };

        struct CanyonPath
        {
            std::vector<Ogre::Vector2> uvPoints;
            std::vector<float> widths; // world units
            std::vector<float> depths; // world units
        };

        struct RiverPath
        {
            std::vector<Ogre::Vector2> uvPoints;
            std::vector<float> flow;
        };

        // Noise core (identical to ProceduralTerrainCreationComponent)
        float fade(float t) const;
        float lerpF(float t, float a, float b) const;
        float grad(int hash, float x, float y) const;
        float noise2D(float x, float y, int seed) const;
        float noise3D(float x, float y, float z, int seed) const;
        float perlinFBM(float u, float v) const;

        // Catmull-Rom path helper
        Ogre::Vector2 evaluateCatmullRom(const std::vector<Ogre::Vector2>& points, float t) const;

        // Feature generators (all work in [0,1]x[0,1] UV space, heights in world units)
        void applyIslandMask(std::vector<float>& heights, int vertexCount, const std::vector<Ogre::Vector2>& uvCoords, float radius) const;

        void generateCanyons(std::vector<CanyonPath>& canyons, std::mt19937& rng) const;
        void applyCanyons(std::vector<float>& heights, const std::vector<CanyonPath>& canyons, int vertexCount, const std::vector<Ogre::Vector2>& uvCoords) const;

        std::vector<RiverPath> traceRivers(const std::vector<float>& heights, int vertexCount, const std::vector<Ogre::Vector2>& uvCoords, std::mt19937& rng) const;
        void carveRivers(std::vector<float>& heights, const std::vector<RiverPath>& rivers, int vertexCount, const std::vector<Ogre::Vector2>& uvCoords) const;

        std::vector<RoadPoint> generateRoadPath(const Ogre::Vector2& uvStart, const Ogre::Vector2& uvEnd, const std::vector<float>& heights, int vertexCount, const std::vector<Ogre::Vector2>& uvCoords, std::mt19937& rng) const;
        void applyRoads(std::vector<float>& heights, const std::vector<std::vector<RoadPoint>>& roads, int vertexCount, const std::vector<Ogre::Vector2>& uvCoords) const;

        // Finds the nearest vertex index to a given UV coordinate (simple linear scan;
        // fast enough for the vertex counts a planet would have).
        int findNearestVertex(const Ogre::Vector2& uv, const std::vector<Ogre::Vector2>& uvCoords, int vertexCount) const;

        // Finds the sibling PlanetTerraComponentBase; returns nullptr if absent.
        PlanetTerraComponentBase* findPlanetTerraBase(void) const;

        // Paints RGBA blend using bilinear grid interpolation + corrected slope.
        // amplitude is passed so depth clamping is available without a member.
        void autoRPaintLayers(const std::vector<float>& heights, const std::vector<Ogre::Vector2>& uvCoords, int vertexCount, float radius, float amplitude, uint32_t blendSize, unsigned char* blendOut) const;

    private:
        Ogre::String name;

        // Permutation table (copied from ProceduralTerrainCreationComponent)
        static const int permutation[256];

        // Attributes
        Variant* hillAmplitude = nullptr;
        Variant* hillFrequency = nullptr;
        Variant* octaves = nullptr;
        Variant* persistence = nullptr;
        Variant* lacunarity = nullptr;
        Variant* seed = nullptr;
        Variant* enableIsland = nullptr;
        Variant* islandFalloff = nullptr;
        Variant* islandSize = nullptr;
        Variant* enableRivers = nullptr;
        Variant* riverCount = nullptr;
        Variant* riverWidth = nullptr;
        Variant* riverDepth = nullptr;
        Variant* riverMeandering = nullptr;
        Variant* enableCanyons = nullptr;
        Variant* canyonCount = nullptr;
        Variant* canyonWidth = nullptr;
        Variant* canyonDepth = nullptr;
        Variant* canyonSteepness = nullptr;
        Variant* enableRoads = nullptr;
        Variant* roadCount = nullptr;
        Variant* roadWidth = nullptr;
        Variant* roadDepth = nullptr;
        Variant* roadSmoothness = nullptr;
        Variant* roadCurviness = nullptr;
        Variant* generate = nullptr;
        Variant* layerSandThresh = nullptr;
        Variant* layerGrassThresh = nullptr;
        Variant* layerRockThresh = nullptr;
        Variant* layerSlopeRock = nullptr;

        mutable float currentRadius;
        mutable float currentRadiusScale;
    };

    using ProceduralPlanetComponentPtr = boost::shared_ptr<ProceduralPlanetComponent>;

} // namespace NOWA

#endif
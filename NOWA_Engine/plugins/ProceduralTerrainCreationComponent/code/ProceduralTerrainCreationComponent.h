#ifndef PROCEDURAL_TERRAIN_CREATION_COMPONENT_H
#define PROCEDURAL_TERRAIN_CREATION_COMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"

namespace NOWA
{
	class Terra;
	class TerraComponent;

	/**
	  * @brief		Advanced Procedural Terrain Creation component for generating heightmaps for Terra.
	  *				This component generates procedural terrain using multi-octave Perlin noise
	  *				combined with sophisticated algorithms for roads, rivers, canyons, and more.
	  *
	  *				Features:
	  *				- Multi-octave Perlin noise for natural-looking terrain
	  *				- Configurable roads with smooth integration
	  *				- Hydraulic erosion-based river generation
	  *				- Canyon formation using targeted erosion
	  *				- Island generation with radial masking
	  *				- Particle-based erosion simulation
	  *				- Full parameter control via Variants
	  */
	class EXPORTED ProceduralTerrainCreationComponent : public GameObjectComponent, public Ogre::Plugin
	{
	public:
		typedef boost::shared_ptr<ProceduralTerrainCreationComponent> ProceduralTerrainCreationComponentPtr;
	public:

		ProceduralTerrainCreationComponent();

		virtual ~ProceduralTerrainCreationComponent();

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
		* @see		GameObjectComponent::getClassName
		*/
		virtual Ogre::String getClassName(void) const override;

		/**
		* @see		GameObjectComponent::getParentClassName
		*/
		virtual Ogre::String getParentClassName(void) const override;

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

		// Basic terrain parameters
		void setResolution(Ogre::uint32 resolution);
		Ogre::uint32 getResolution(void) const;
		void setBaseHeight(Ogre::Real baseHeight);
		Ogre::Real getBaseHeight(void) const;
		void setHillAmplitude(Ogre::Real hillAmplitude);
		Ogre::Real getHillAmplitude(void) const;
		void setHillFrequency(Ogre::Real hillFrequency);
		Ogre::Real getHillFrequency(void) const;
		void setOctaves(Ogre::uint32 octaves);
		Ogre::uint32 getOctaves(void) const;
		void setPersistence(Ogre::Real persistence);
		Ogre::Real getPersistence(void) const;
		void setLacunarity(Ogre::Real lacunarity);
		Ogre::Real getLacunarity(void) const;
		void setSeed(Ogre::uint32 seed);
		Ogre::uint32 getSeed(void) const;

		// Road parameters
		void setEnableRoads(bool enableRoads);
		bool getEnableRoads(void) const;
		void setRoadCount(Ogre::uint32 roadCount);
		Ogre::uint32 getRoadCount(void) const;
		void setRoadWidth(Ogre::Real roadWidth);
		Ogre::Real getRoadWidth(void) const;
		void setRoadDepth(Ogre::Real roadDepth);
		Ogre::Real getRoadDepth(void) const;
		void setRoadSmoothness(Ogre::Real roadSmoothness);
		Ogre::Real getRoadSmoothness(void) const;
		void setRoadCurviness(Ogre::Real roadCurviness);
		Ogre::Real getRoadCurviness(void) const;
		void setRoadsClosed(bool roadsClosed);
		bool getRoadsClosed(void) const;

		// River parameters
		void setEnableRivers(bool enableRivers);
		bool getEnableRivers(void) const;
		void setRiverCount(Ogre::uint32 riverCount);
		Ogre::uint32 getRiverCount(void) const;
		void setRiverWidth(Ogre::Real riverWidth);
		Ogre::Real getRiverWidth(void) const;
		void setRiverDepth(Ogre::Real riverDepth);
		Ogre::Real getRiverDepth(void) const;
		void setRiverMeandering(Ogre::Real riverMeandering);
		Ogre::Real getRiverMeandering(void) const;

		// Canyon parameters
		void setEnableCanyons(bool enableCanyons);
		bool getEnableCanyons(void) const;
		void setCanyonCount(Ogre::uint32 canyonCount);
		Ogre::uint32 getCanyonCount(void) const;
		void setCanyonWidth(Ogre::Real canyonWidth);
		Ogre::Real getCanyonWidth(void) const;
		void setCanyonDepth(Ogre::Real canyonDepth);
		Ogre::Real getCanyonDepth(void) const;
		void setCanyonSteepness(Ogre::Real canyonSteepness);
		Ogre::Real getCanyonSteepness(void) const;

		// Island parameters
		void setEnableIsland(bool enableIsland);
		bool getEnableIsland(void) const;
		void setIslandFalloff(Ogre::Real islandFalloff);
		Ogre::Real getIslandFalloff(void) const;
		void setIslandSize(Ogre::Real islandSize);
		Ogre::Real getIslandSize(void) const;

		// Erosion parameters
		void setEnableErosion(bool enableErosion);
		bool getEnableErosion(void) const;
		void setErosionIterations(Ogre::uint32 erosionIterations);
		Ogre::uint32 getErosionIterations(void) const;
		void setErosionStrength(Ogre::Real erosionStrength);
		Ogre::Real getErosionStrength(void) const;
		void setSedimentCapacity(Ogre::Real sedimentCapacity);
		Ogre::Real getSedimentCapacity(void) const;

		// Advanced river parameters
		void setRiverFlowThreshold(Ogre::Real riverFlowThreshold);
		Ogre::Real getRiverFlowThreshold(void) const;
		void setRiverWidthScale(Ogre::Real riverWidthScale);
		Ogre::Real getRiverWidthScale(void) const;
		void setRiverErosionFactor(Ogre::Real riverErosionFactor);
		Ogre::Real getRiverErosionFactor(void) const;
		void setMaxRiverDepth(Ogre::Real maxRiverDepth);
		Ogre::Real getMaxRiverDepth(void) const;

		// Advanced canyon parameters
		void setCanyonMinWidth(Ogre::Real canyonMinWidth);
		Ogre::Real getCanyonMinWidth(void) const;
		void setCanyonMaxWidth(Ogre::Real canyonMaxWidth);
		Ogre::Real getCanyonMaxWidth(void) const;

		void generateTerrain(void);

	public:
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("ProceduralTerrainCreationComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "ProceduralTerrainCreationComponent";
		}

		static bool canStaticAddComponent(GameObject* gameObject);

		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: Generates procedural heightmap terrain for Terra using multi-octave Perlin noise.\n\n"

				"Features:\n"
				"- Natural-looking hills and valleys\n"
				"- Procedural roads that follow terrain elevation\n"
				"- Drainage-based river networks\n"
				"- Canyon formation\n"
				"- Island generation\n"
				"- Hydraulic erosion simulation\n"
				"- Fully configurable parameters\n\n"

				"Basic Parameters:\n"
				"- Resolution: Heightmap size (512, 1024, 2048, etc.)\n"
				"- Base Height: Minimum terrain elevation\n"
				"- Hill Amplitude: Maximum height variation\n"
				"- Hill Frequency: Size of terrain features (lower = larger)\n"
				"- Octaves: Detail layers (more = more detail)\n"
				"- Persistence: Roughness (0.3-0.7 typical)\n"
				"- Lacunarity: Detail variation (2.0 typical)\n"
				"- Seed: Random seed for different terrain\n\n"

				"Road Parameters:\n"
				"- Enable Roads: Turn road generation on/off\n"
				"- Road Count: Number of roads to generate\n"
				"- Road Width: Width of roads\n"
				"- Road Depth: How deep roads cut into terrain\n"
				"- Road Smoothness: Blend factor (higher = smoother)\n"
				"- Road Curviness: How curvy roads are (0-1)\n"
				"- Roads Closed: Whether roads form loops\n\n"

				"River Parameters:\n"
				"- Enable Rivers: Turn river generation on/off\n"
				"- River Count: Number of major rivers\n"
				"- River Width: Width of rivers\n"
				"- River Depth: How deep rivers carve\n"
				"- River Meandering: Natural meandering amount\n\n"

				"Canyon Parameters:\n"
				"- Enable Canyons: Turn canyon generation on/off\n"
				"- Canyon Count: Number of canyons\n"
				"- Canyon Width: Width at the top\n"
				"- Canyon Depth: How deep canyons go\n"
				"- Canyon Steepness: Wall steepness\n\n"

				"Island Parameters:\n"
				"- Enable Island: Create ocean-surrounded terrain\n"
				"- Island Falloff: Edge drop-off steepness\n"
				"- Island Size: Relative island size (0-1)\n\n"

				"Erosion Parameters:\n"
				"- Enable Erosion: Apply hydraulic erosion\n"
				"- Erosion Iterations: Number of simulation steps\n"
				"- Erosion Strength: How aggressive erosion is\n"
				"- Sediment Capacity: Sediment carrying capacity\n\n"

				"Tips:\n"
				"- Start with defaults and adjust gradually\n"
				"- Higher resolution = slower generation but more detail\n"
				"- Toggle Activated to regenerate terrain\n"
				"- Features work best when combined\n"
				"- Use different seeds for different terrain layouts";
		}

		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

	public:
		static const Ogre::String AttrResolution(void)
		{
			return "Resolution";
		}
		static const Ogre::String AttrBaseHeight(void)
		{
			return "Base Height";
		}
		static const Ogre::String AttrHillAmplitude(void)
		{
			return "Hill Amplitude";
		}
		static const Ogre::String AttrHillFrequency(void)
		{
			return "Hill Frequency";
		}
		static const Ogre::String AttrOctaves(void)
		{
			return "Octaves";
		}
		static const Ogre::String AttrPersistence(void)
		{
			return "Persistence";
		}
		static const Ogre::String AttrLacunarity(void)
		{
			return "Lacunarity";
		}
		static const Ogre::String AttrSeed(void)
		{
			return "Seed";
		}

		static const Ogre::String AttrEnableRoads(void)
		{
			return "Enable Roads";
		}
		static const Ogre::String AttrRoadCount(void)
		{
			return "Road Count";
		}
		static const Ogre::String AttrRoadWidth(void)
		{
			return "Road Width";
		}
		static const Ogre::String AttrRoadDepth(void)
		{
			return "Road Depth";
		}
		static const Ogre::String AttrRoadSmoothness(void)
		{
			return "Road Smoothness";
		}
		static const Ogre::String AttrRoadCurviness(void)
		{
			return "Road Curviness";
		}
		static const Ogre::String AttrRoadsClosed(void)
		{
			return "Roads Closed";
		}

		static const Ogre::String AttrEnableRivers(void)
		{
			return "Enable Rivers";
		}
		static const Ogre::String AttrRiverCount(void)
		{
			return "River Count";
		}
		static const Ogre::String AttrRiverWidth(void)
		{
			return "River Width";
		}
		static const Ogre::String AttrRiverDepth(void)
		{
			return "River Depth";
		}
		static const Ogre::String AttrRiverMeandering(void)
		{
			return "River Meandering";
		}

		static const Ogre::String AttrEnableCanyons(void)
		{
			return "Enable Canyons";
		}
		static const Ogre::String AttrCanyonCount(void)
		{
			return "Canyon Count";
		}
		static const Ogre::String AttrCanyonWidth(void)
		{
			return "Canyon Width";
		}
		static const Ogre::String AttrCanyonDepth(void)
		{
			return "Canyon Depth";
		}
		static const Ogre::String AttrCanyonSteepness(void)
		{
			return "Canyon Steepness";
		}

		static const Ogre::String AttrEnableIsland(void)
		{
			return "Enable Island";
		}
		static const Ogre::String AttrIslandFalloff(void)
		{
			return "Island Falloff";
		}
		static const Ogre::String AttrIslandSize(void)
		{
			return "Island Size";
		}

		static const Ogre::String AttrEnableErosion(void)
		{
			return "Enable Erosion";
		}
		static const Ogre::String AttrErosionIterations(void)
		{
			return "Erosion Iterations";
		}
		static const Ogre::String AttrErosionStrength(void)
		{
			return "Erosion Strength";
		}
		static const Ogre::String AttrSedimentCapacity(void)
		{
			return "Sediment Capacity";
		}

		static const Ogre::String AttrRiverFlowThreshold(void)
		{
			return "River Flow Threshold";
		}

		static const Ogre::String AttrRiverWidthScale(void)
		{
			return "River Width Scale";
		}

		static const Ogre::String AttrRiverErosionFactor(void)
		{
			return "River Erosion Factor";
		}

		static const Ogre::String AttrMaxRiverDepth(void)
		{
			return "Max River Depth";
		}
		
		static const Ogre::String AttrCanyonMinWidth(void)
		{
			return "Canyon Min Width";
		}

		static const Ogre::String AttrCanyonMaxWidth(void)
		{
			return "Canyon Max Width";
		}

		static const Ogre::String AttrRegenerate(void)
		{
			return "Regenerate";
		}
	protected:
		virtual bool executeAction(const Ogre::String& actionId, NOWA::Variant* attribute) override;
	private:
		// Perlin noise generation helpers (PRESERVED FROM ORIGINAL)
		float fade(float t);
		float lerp(float t, float a, float b);
		float grad(int hash, float x, float y);
		float noise(float x, float y, int seed);
		float smoothNoise(float x, float y, int seed);
		float interpolate(float a, float b, float x);
		float perlinNoise(float x, float y);

		// Road generation helpers (PRESERVED FROM ORIGINAL)
		struct RoadPoint
		{
			Ogre::Vector2 position;
			float width;
			float targetHeight;
		};

		std::vector<RoadPoint> generateRoadPath(const Ogre::Vector2& start, const Ogre::Vector2& end, int numSegments);
		Ogre::Vector2 evaluateCatmullRom(const std::vector<Ogre::Vector2>& points, float t);
		void calculateRoadHeights(std::vector<std::vector<RoadPoint>>& roads, const std::vector<float>& baseTerrainHeights, Ogre::uint32 width, Ogre::uint32 height);
		void smoothRoadHeights(std::vector<RoadPoint>& road);
		float applyRoadCarving(float baseHeight, const Ogre::Vector2& position, const std::vector<std::vector<RoadPoint>>& roads, Ogre::uint32 width, Ogre::uint32 height);

		// River generation helpers
		struct DrainageNode
		{
			Ogre::Vector2 position;
			float flow;
			int downstreamIndex;
			float elevation;
		};

		void generateTensorField(std::vector<Ogre::Vector2>& tensorField, Ogre::uint32 width, Ogre::uint32 height);
		std::vector<ProceduralTerrainCreationComponent::DrainageNode> generateDrainageBasins(const std::vector<float>& heights, Ogre::uint32 width, Ogre::uint32 height);
		void carveRivers(std::vector<float>& heights, const std::vector<DrainageNode>& nodes, Ogre::uint32 width, Ogre::uint32 height);

		// Canyon generation helpers
		struct CanyonPath
		{
			std::vector<Ogre::Vector2> points;
			std::vector<float> widths;
			std::vector<float> depths;
		};

		void generateCanyons(std::vector<CanyonPath>& canyons, Ogre::uint32 width, Ogre::uint32 height);
		void applyCanyonsToTerrain(std::vector<float>& heights, const std::vector<CanyonPath>& canyons, Ogre::uint32 width, Ogre::uint32 height);

		// Island generation
		void applyIslandMask(std::vector<float>& heights, Ogre::uint32 width, Ogre::uint32 height);

		// Hydraulic erosion (particle-based)
		struct ErosionParticle
		{
			Ogre::Vector2 position;
			Ogre::Vector2 velocity;
			float sediment;
			float water;
		};

		void simulateHydraulicErosion(std::vector<float>& heights, Ogre::uint32 width, Ogre::uint32 height);
		void updateParticle(ErosionParticle& particle, std::vector<float>& heights, Ogre::uint32 width, Ogre::uint32 height);

		// NEW: Utility functions
		float getHeightBilinear(const std::vector<float>& heights, float x, float y, Ogre::uint32 width, Ogre::uint32 height);
		Ogre::Vector2 getGradient(const std::vector<float>& heights, int x, int y, Ogre::uint32 width, Ogre::uint32 height);
		float getDistanceToLine(const Ogre::Vector2& point, const Ogre::Vector2& lineStart, const Ogre::Vector2& lineEnd);

		// Terrain generation
		void generateProceduralTerrain(void);
		Ogre::Terra* findTerraComponent(void);

	private:
		Ogre::String name;

		// Terrain parameters
		Variant* resolution;
		Variant* baseHeight;
		Variant* hillAmplitude;
		Variant* hillFrequency;
		Variant* octaves;
		Variant* persistence;
		Variant* lacunarity;
		Variant* seed;

		// Road parameters
		Variant* enableRoads;
		Variant* roadCount;
		Variant* roadWidth;
		Variant* roadDepth;
		Variant* roadSmoothness;
		Variant* roadCurviness;

		// Additional road parameter
		Variant* roadsClosed;

		// River parameters
		Variant* enableRivers;
		Variant* riverCount;
		Variant* riverWidth;
		Variant* riverDepth;
		Variant* riverMeandering;

		// Canyon parameters
		Variant* enableCanyons;
		Variant* canyonCount;
		Variant* canyonWidth;
		Variant* canyonDepth;
		Variant* canyonSteepness;

		// Island parameters
		Variant* enableIsland;
		Variant* islandFalloff;
		Variant* islandSize;

		// Erosion parameters
		Variant* enableErosion;
		Variant* erosionIterations;
		Variant* erosionStrength;
		Variant* sedimentCapacity;

		Variant* riverFlowThreshold;
		Variant* riverWidthScale;
		Variant* riverErosionFactor;
		Variant* maxRiverDepth;

		Variant* canyonMinWidth;
		Variant* canyonMaxWidth;
		Variant* regenerate;

		// Runtime data (PRESERVED FROM ORIGINAL)
		bool terrainGenerated;
		float cachedPixelsPerMeter;
	};

}; // namespace end

#endif
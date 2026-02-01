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
	  * @brief		Procedural Terrain Creation component for generating heightmaps for Terra.
	  *				This component generates procedural terrain using multi-octave Perlin noise
	  *				with optional roads, hills, valleys, and other features.
	  *
	  *				Usage: Add this component to a GameObject that has a Terra terrain.
	  *				Configure the parameters and set Activated to true to generate the terrain.
	  *				Each time parameters change or Activated is toggled, the terrain regenerates.
	  *
	  *				Features:
	  *				- Multi-octave Perlin noise for natural-looking terrain
	  *				- Configurable roads with smooth integration into terrain
	  *				- Adjustable hills, valleys, and base height
	  *				- Road paths that follow terrain elevation
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

		/**
		 * @brief Sets whether the component is activated and should generate terrain.
		 * @param[in] activated Whether to activate and generate terrain
		 */
		void setActivated(bool activated);

		/**
		 * @brief Gets whether the component is activated.
		 * @return True if activated
		 */
		bool isActivated(void) const;

		/**
		 * @brief Sets the terrain resolution (width and height in pixels).
		 * @param[in] resolution Resolution (e.g., 512, 1024, 2048)
		 */
		void setResolution(Ogre::uint32 resolution);

		/**
		 * @brief Gets the terrain resolution.
		 * @return Resolution value
		 */
		Ogre::uint32 getResolution(void) const;

		/**
		 * @brief Sets the base height of the terrain (minimum elevation).
		 * @param[in] baseHeight Base height in world units
		 */
		void setBaseHeight(Ogre::Real baseHeight);

		/**
		 * @brief Gets the base height.
		 * @return Base height
		 */
		Ogre::Real getBaseHeight(void) const;

		/**
		 * @brief Sets the hill amplitude (max height variation from base).
		 * @param[in] hillAmplitude Amplitude in world units
		 */
		void setHillAmplitude(Ogre::Real hillAmplitude);

		/**
		 * @brief Gets the hill amplitude.
		 * @return Hill amplitude
		 */
		Ogre::Real getHillAmplitude(void) const;

		/**
		 * @brief Sets the hill frequency (controls terrain feature size).
		 *        Lower values = larger features, higher values = smaller features.
		 * @param[in] hillFrequency Frequency value (typically 0.001 to 0.1)
		 */
		void setHillFrequency(Ogre::Real hillFrequency);

		/**
		 * @brief Gets the hill frequency.
		 * @return Hill frequency
		 */
		Ogre::Real getHillFrequency(void) const;

		/**
		 * @brief Sets the number of noise octaves (detail layers).
		 *        More octaves = more detail but slower generation.
		 * @param[in] octaves Number of octaves (1-8 recommended)
		 */
		void setOctaves(Ogre::uint32 octaves);

		/**
		 * @brief Gets the number of octaves.
		 * @return Octave count
		 */
		Ogre::uint32 getOctaves(void) const;

		/**
		 * @brief Sets the persistence (amplitude decrease per octave).
		 *        Lower values = smoother terrain, higher values = rougher terrain.
		 * @param[in] persistence Persistence value (typically 0.3 to 0.7)
		 */
		void setPersistence(Ogre::Real persistence);

		/**
		 * @brief Gets the persistence.
		 * @return Persistence value
		 */
		Ogre::Real getPersistence(void) const;

		/**
		 * @brief Sets the lacunarity (frequency multiplier per octave).
		 *        Higher values = more variation in detail.
		 * @param[in] lacunarity Lacunarity value (typically 2.0)
		 */
		void setLacunarity(Ogre::Real lacunarity);

		/**
		 * @brief Gets the lacunarity.
		 * @return Lacunarity value
		 */
		Ogre::Real getLacunarity(void) const;

		/**
		 * @brief Sets the random seed for terrain generation.
		 *        Different seeds produce different terrain.
		 * @param[in] seed Seed value
		 */
		void setSeed(Ogre::uint32 seed);

		/**
		 * @brief Gets the seed.
		 * @return Seed value
		 */
		Ogre::uint32 getSeed(void) const;

		/**
		 * @brief Sets whether to enable procedural roads.
		 * @param[in] enableRoads True to generate roads
		 */
		void setEnableRoads(bool enableRoads);

		/**
		 * @brief Gets whether roads are enabled.
		 * @return True if roads enabled
		 */
		bool getEnableRoads(void) const;

		/**
		 * @brief Sets the number of roads to generate.
		 * @param[in] roadCount Number of roads (1-10)
		 */
		void setRoadCount(Ogre::uint32 roadCount);

		/**
		 * @brief Gets the road count.
		 * @return Number of roads
		 */
		Ogre::uint32 getRoadCount(void) const;

		/**
		 * @brief Sets the road width.
		 * @param[in] roadWidth Width in world units
		 */
		void setRoadWidth(Ogre::Real roadWidth);

		/**
		 * @brief Gets the road width.
		 * @return Road width
		 */
		Ogre::Real getRoadWidth(void) const;

		/**
		 * @brief Sets how deep roads cut into terrain.
		 * @param[in] roadDepth Depth in world units
		 */
		void setRoadDepth(Ogre::Real roadDepth);

		/**
		 * @brief Gets the road depth.
		 * @return Road depth
		 */
		Ogre::Real getRoadDepth(void) const;

		/**
		 * @brief Sets road smoothness multiplier (how smoothly roads blend into terrain).
		 * @param[in] roadSmoothness Smoothness multiplier (1.0-10.0)
		 */
		void setRoadSmoothness(Ogre::Real roadSmoothness);

		/**
		 * @brief Gets the road smoothness.
		 * @return Road smoothness
		 */
		Ogre::Real getRoadSmoothness(void) const;

		/**
		 * @brief Sets how curvy the roads are (0 = straight, 1 = very curvy).
		 * @param[in] roadCurviness Curviness factor (0.0-1.0)
		 */
		void setRoadCurviness(Ogre::Real roadCurviness);

		/**
		 * @brief Gets the road curviness.
		 * @return Road curviness
		 */
		Ogre::Real getRoadCurviness(void) const;

		/**
		 * @brief Manually triggers terrain generation.
		 */
		void generateTerrain(void);

	public:
		/**
		* @see		GameObjectComponent::getStaticClassId
		*/
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("ProceduralTerrainCreationComponent");
		}

		/**
		* @see		GameObjectComponent::getStaticClassName
		*/
		static Ogre::String getStaticClassName(void)
		{
			return "ProceduralTerrainCreationComponent";
		}

		/**
		* @see		GameObjectComponent::canStaticAddComponent
		*/
		static bool canStaticAddComponent(GameObject* gameObject);

		/**
		 * @see	GameObjectComponent::getStaticInfoText
		 */
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: Generates procedural heightmap terrain for Terra using multi-octave Perlin noise.\n\n"

				"Features:\n"
				"- Natural-looking hills and valleys\n"
				"- Procedural roads that follow terrain elevation\n"
				"- Fully configurable parameters\n"
				"- Real-time regeneration when parameters change\n\n"

				"Basic Parameters:\n"
				"- Resolution: Heightmap size (512, 1024, 2048, etc.)\n"
				"- Base Height: Minimum terrain elevation\n"
				"- Hill Amplitude: Maximum height variation\n"
				"- Hill Frequency: Size of terrain features (lower = larger)\n\n"

				"Advanced Parameters:\n"
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
				"- Road Curviness: How curvy roads are (0-1)\n\n"

				"Tips:\n"
				"- Start with defaults and adjust gradually\n"
				"- Higher resolution = slower generation but more detail\n"
				"- Toggle Activated to regenerate terrain\n"
				"- Roads automatically follow terrain elevation\n"
				"- Use different seeds for different terrain layouts";
		}

		/**
		 * @see	GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

	public:
		static const Ogre::String AttrActivated(void)
		{
			return "Activated";
		}
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

	private:
		// Perlin noise generation helpers
		float fade(float t);
		float lerp(float t, float a, float b);
		float grad(int hash, float x, float y);
		float noise(float x, float y, int seed);
		float smoothNoise(float x, float y, int seed);
		float interpolate(float a, float b, float x);
		float perlinNoise(float x, float y);

		// Road generation helpers
		struct RoadPoint
		{
			Ogre::Vector2 position;
			float width;
			float targetHeight;
		};

		std::vector<RoadPoint> generateRoadPath(const Ogre::Vector2& start, const Ogre::Vector2& end, int numSegments);
		Ogre::Vector2 evaluateCatmullRom(const std::vector<Ogre::Vector2>& points, float t);
		float applyRoadInfluence(float baseHeight, const Ogre::Vector2& position, const std::vector<std::vector<RoadPoint>>& roads);

		// Terrain generation
		void generateProceduralTerrain(void);

		Ogre::Terra* findTerraComponent(void);

	private:
		Ogre::String name;

		// Terrain parameters
		Variant* activated;
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

		// Runtime data
		bool terrainGenerated;
	};

}; // namespace end

#endif
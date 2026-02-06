#include "NOWAPrecompiled.h"
#include "ProceduralTerrainCreationComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"
#include "gameobject/TerraComponent.h"

#include "OgreAbiUtils.h"
#include <random>
#include <cmath>

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	ProceduralTerrainCreationComponent::ProceduralTerrainCreationComponent()
		: GameObjectComponent(),
		name("ProceduralTerrainCreationComponent"),
		resolution(new Variant(ProceduralTerrainCreationComponent::AttrResolution(), static_cast<Ogre::uint32>(1024), this->attributes)),
		// Base height: 0.5 = middle of terrain (Y=25m with your setup)
		baseHeight(new Variant(ProceduralTerrainCreationComponent::AttrBaseHeight(), 0.5f, this->attributes)),
		// Hill amplitude: 0.1 = only 10% variation = +-10m hills
		hillAmplitude(new Variant(ProceduralTerrainCreationComponent::AttrHillAmplitude(), 0.1f, this->attributes)),
		// Frequency: 3-4 features across terrain
		hillFrequency(new Variant(ProceduralTerrainCreationComponent::AttrHillFrequency(), 3.0f, this->attributes)),
		octaves(new Variant(ProceduralTerrainCreationComponent::AttrOctaves(), static_cast<Ogre::uint32>(4), this->attributes)),
		persistence(new Variant(ProceduralTerrainCreationComponent::AttrPersistence(), 0.5f, this->attributes)),
		lacunarity(new Variant(ProceduralTerrainCreationComponent::AttrLacunarity(), 2.0f, this->attributes)),
		seed(new Variant(ProceduralTerrainCreationComponent::AttrSeed(), static_cast<Ogre::uint32>(12345), this->attributes)),
		enableRoads(new Variant(ProceduralTerrainCreationComponent::AttrEnableRoads(), true, this->attributes)),
		roadCount(new Variant(ProceduralTerrainCreationComponent::AttrRoadCount(), static_cast<Ogre::uint32>(2), this->attributes)),
		roadWidth(new Variant(ProceduralTerrainCreationComponent::AttrRoadWidth(), 5.0f, this->attributes)),    // 5 meters
		roadDepth(new Variant(ProceduralTerrainCreationComponent::AttrRoadDepth(), 0.02f, this->attributes)),   // 2% of height range
		roadSmoothness(new Variant(ProceduralTerrainCreationComponent::AttrRoadSmoothness(), 2.0f, this->attributes)),
		roadCurviness(new Variant(ProceduralTerrainCreationComponent::AttrRoadCurviness(), 0.3f, this->attributes)),
		roadsClosed(new Variant(ProceduralTerrainCreationComponent::AttrRoadsClosed(), false, this->attributes)),

		// Add river parameters:
		enableRivers(new Variant(ProceduralTerrainCreationComponent::AttrEnableRivers(), false, this->attributes)),
		riverCount(new Variant(ProceduralTerrainCreationComponent::AttrRiverCount(), static_cast<Ogre::uint32>(2), this->attributes)),
		riverWidth(new Variant(ProceduralTerrainCreationComponent::AttrRiverWidth(), 20.0f, this->attributes)),
		riverDepth(new Variant(ProceduralTerrainCreationComponent::AttrRiverDepth(), 10.0f, this->attributes)),
		riverMeandering(new Variant(ProceduralTerrainCreationComponent::AttrRiverMeandering(), 0.5f, this->attributes)),

		// Add canyon parameters:
		enableCanyons(new Variant(ProceduralTerrainCreationComponent::AttrEnableCanyons(), false, this->attributes)),
		canyonCount(new Variant(ProceduralTerrainCreationComponent::AttrCanyonCount(), static_cast<Ogre::uint32>(1), this->attributes)),
		canyonWidth(new Variant(ProceduralTerrainCreationComponent::AttrCanyonWidth(), 50.0f, this->attributes)),
		canyonDepth(new Variant(ProceduralTerrainCreationComponent::AttrCanyonDepth(), 30.0f, this->attributes)),
		canyonSteepness(new Variant(ProceduralTerrainCreationComponent::AttrCanyonSteepness(), 0.7f, this->attributes)),

		// Add island parameters:
		enableIsland(new Variant(ProceduralTerrainCreationComponent::AttrEnableIsland(), false, this->attributes)),
		islandFalloff(new Variant(ProceduralTerrainCreationComponent::AttrIslandFalloff(), 2.0f, this->attributes)),
		islandSize(new Variant(ProceduralTerrainCreationComponent::AttrIslandSize(), 0.7f, this->attributes)),

		// Add erosion parameters:
		enableErosion(new Variant(ProceduralTerrainCreationComponent::AttrEnableErosion(), false, this->attributes)),
		erosionIterations(new Variant(ProceduralTerrainCreationComponent::AttrErosionIterations(), static_cast<Ogre::uint32>(50000), this->attributes)),
		erosionStrength(new Variant(ProceduralTerrainCreationComponent::AttrErosionStrength(), 0.5f, this->attributes)),
		sedimentCapacity(new Variant(ProceduralTerrainCreationComponent::AttrSedimentCapacity(), 4.0f, this->attributes)),

		riverFlowThreshold(new Variant(ProceduralTerrainCreationComponent::AttrRiverFlowThreshold(), 0.01f, this->attributes)),
		riverWidthScale(new Variant(ProceduralTerrainCreationComponent::AttrRiverWidthScale(), 1.0f, this->attributes)),
		riverErosionFactor(new Variant(ProceduralTerrainCreationComponent::AttrRiverErosionFactor(), 0.02f, this->attributes)),
		maxRiverDepth(new Variant(ProceduralTerrainCreationComponent::AttrMaxRiverDepth(), 10.0f, this->attributes)),
		canyonMinWidth(new Variant(ProceduralTerrainCreationComponent::AttrCanyonMinWidth(), 5.0f, this->attributes)),
		canyonMaxWidth(new Variant(ProceduralTerrainCreationComponent::AttrCanyonMaxWidth(), 20.0f, this->attributes)),
		regenerate(new Variant(ProceduralTerrainCreationComponent::AttrRegenerate(), "Generate", this->attributes)),
		terrainGenerated(false),
		cachedPixelsPerMeter(0.0f)
	{
		this->resolution->setDescription("Heightmap resolution (width and height). Common values: 512, 1024, 2048.");
		this->baseHeight->setDescription("Base terrain level [0-1]. 0.5 = center (Y=25m). Leave room above and below for hills/holes.");
		this->hillAmplitude->setDescription("Hill height variation [0-1]. 0.1 = gentle 10% variation, 0.3 = moderate hills, 0.5 = dramatic mountains.");
		this->hillFrequency->setDescription("Number of major terrain features across the map. 2 = few large hills, 4 = moderate, 8+ = many small features.");
		this->octaves->setDescription("Number of noise detail layers. More octaves = more detail (1-8 recommended).");
		this->persistence->setDescription("How much each octave contributes. Lower = smoother (0.3-0.7 typical).");
		this->lacunarity->setDescription("Frequency multiplier per octave. Higher = more variation (2.0 typical).");
		this->seed->setDescription("Random seed. Change to generate different terrain layouts.");
		this->enableRoads->setDescription("Enable procedural road generation.");
		this->roadCount->setDescription("Number of roads to generate (1-10).");
		this->roadWidth->setDescription("Road width in world units.");
		this->roadDepth->setDescription("How deep roads cut into the terrain.");
		this->roadSmoothness->setDescription("Blend smoothness (higher = smoother transition, 1-10 typical).");
		this->roadCurviness->setDescription("Road curvature amount (0 = straight, 1 = very curvy).");

		this->resolution->setConstraints(128u, 4096u);
		this->baseHeight->setConstraints(0.2f, 0.8f);      // Keep away from extremes
		this->hillAmplitude->setConstraints(0.02f, 0.4f);  // 2% to 40% variation
		this->hillFrequency->setConstraints(1.0f, 16.0f);	// 1-16 features per terrain
		this->octaves->setConstraints(1u, 8u);
		this->persistence->setConstraints(0.0f, 1.0f);
		this->lacunarity->setConstraints(1.0f, 10.0f);

		this->roadCount->setConstraints(1u, 10u);
		this->roadWidth->setConstraints(2.0f, 15.0f);      // 2-15 meters (realistic roads)
		this->roadDepth->setConstraints(0.005f, 0.1f);     // 0.5%-10% of height range
		this->roadSmoothness->setConstraints(1.5f, 4.0f);  // Blend zone multiplier
		this->roadCurviness->setConstraints(0.0f, 1.0f);

		this->roadsClosed->setDescription("If true, roads tend to form loops rather than purely point-to-point connections. Useful for islands / race tracks.");

		this->enableRivers->setDescription("Enable drainage-based river carving. Uses flow accumulation to place rivers in natural valleys.");
		this->riverCount->setConstraints(1u, 10u);
		this->riverCount->setDescription("High-level river complexity hint. Higher values typically increase major river coverage (used together with flow threshold).");
		this->riverWidth->setConstraints(0.1f, 200.0f);
		this->riverWidth->setDescription("High-level river width control (in heightmap pixels). Scales the internal width function.");
		this->riverDepth->setConstraints(0.0f, 200.0f);
		this->riverDepth->setDescription("High-level river depth control. Clamps / scales the internal erosion depth.");
		this->riverMeandering->setConstraints(0.0f, 1.0f);
		this->riverMeandering->setDescription("Controls how much the drainage descent is allowed to wander. 0 = strict steepest descent, 1 = more natural wandering.");

		this->riverFlowThreshold->setConstraints(1.0f, 5000.0f);
		this->riverFlowThreshold->setDescription("Minimum accumulated flow required before carving starts. Higher = fewer, larger rivers.");
		this->riverWidthScale->setConstraints(0.01f, 10.0f);
		this->riverWidthScale->setDescription("Fine tuning: width multiplier applied to log(flow). Works together with RiverWidth.");
		this->riverErosionFactor->setConstraints(0.0f, 1.0f);
		this->riverErosionFactor->setDescription("Fine tuning: how strongly flow translates into erosion depth. Works together with RiverDepth.");
		this->maxRiverDepth->setConstraints(0.0f, 200.0f);
		this->maxRiverDepth->setDescription("Fine tuning: absolute clamp for river erosion depth.");

		this->enableCanyons->setDescription("Enable canyon carving. Generates long edge-to-edge canyon paths and carves them with a V/U profile.");
		this->canyonCount->setConstraints(0u, 10u);
		this->canyonWidth->setConstraints(0.1f, 200.0f);
		this->canyonWidth->setDescription("High-level canyon width multiplier. Used as a global scale on min/max widths.");
		this->canyonDepth->setConstraints(0.0f, 500.0f);
		this->canyonSteepness->setConstraints(0.1f, 10.0f);
		this->canyonSteepness->setDescription("Controls canyon wall profile shape. Lower = more U-shaped, higher = sharper V-shaped walls.");
		this->canyonMinWidth->setConstraints(0.1f, 200.0f);
		this->canyonMinWidth->setDescription("Minimum canyon width in heightmap pixels.");
		this->canyonMaxWidth->setConstraints(0.1f, 400.0f);
		this->canyonMaxWidth->setDescription("Maximum canyon width in heightmap pixels (should be >= CanyonMinWidth).");

		this->enableIsland->setDescription("Enable island masking. Drops terrain toward edges to create ocean-surrounded landmass.");
		this->islandFalloff->setConstraints(0.1f, 10.0f);
		this->islandSize->setConstraints(0.1f, 1.0f);

		this->enableErosion->setDescription("Enable particle-based hydraulic erosion simulation. Expensive at high iteration counts.");
		this->erosionIterations->setConstraints(0u, 200000u);
		this->erosionStrength->setConstraints(0.0f, 5.0f);
		this->sedimentCapacity->setConstraints(0.0f, 50.0f);

		this->regenerate->setDescription("Generate/regenerate the terrain.");
		this->regenerate->addUserData(GameObject::AttrActionExec());
		this->regenerate->addUserData(GameObject::AttrActionExecId(), "ProceduralTerrainCreationComponent.Regenerate");

		/*
		// Terrain
		Resolution: 1024
		Base Height: 0
		Hill Amplitude: 30
		Hill Frequency: 0.003  (NOT 0.001 - that causes banding!)
		Octaves: 6
		Persistence: 0.5
		Seed: any number

		// Roads (NOW IN REAL METERS!)
		Enable Roads: true
		Road Count: 2-3
		Road Width: 4-6 meters    ← REALISTIC car road width!
		Road Depth: 1-2 meters    ← Slight depression
		Road Smoothness: 2.0
		Road Curviness: 0.4
		Roads Closed: false       (set true for race track)
		
		With your Terra at **center(0, 25, 0)** and **dimensions(100, 100, 100)**:

		### **Terrain at Sea Level (center height):**
		
		Base Height: 0        -> Terrain base at 25m (Terra's center)
		Hill Amplitude: 30    -> Hills go up to 55m
		

		### **Mountainous Terrain:**
		
		Base Height: 10       -> Base at 35m
		Hill Amplitude: 40    -> Peaks at 75m (Terra's max)
		

		### **Underwater Valleys:**
		
		Base Height: -20      -> Base at 5m
		Hill Amplitude: 25    -> Peaks at 30m
		

		### **Roads/Rivers/Canyons (in METERS):**
		
		Road Depth: 5-8 meters   -> Carves 5-8m into terrain
		River Depth: 8-12 meters -> Rivers carve deeper
		Canyon Depth: 25-40 meters -> Deep canyons
		*/
	}

	ProceduralTerrainCreationComponent::~ProceduralTerrainCreationComponent(void)
	{

	}

	void ProceduralTerrainCreationComponent::initialise()
	{

	}

	const Ogre::String& ProceduralTerrainCreationComponent::getName() const
	{
		return this->name;
	}

	void ProceduralTerrainCreationComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<ProceduralTerrainCreationComponent>(ProceduralTerrainCreationComponent::getStaticClassId(), ProceduralTerrainCreationComponent::getStaticClassName());
	}

	void ProceduralTerrainCreationComponent::shutdown()
	{
		// Do nothing here
	}

	void ProceduralTerrainCreationComponent::uninstall()
	{
		// Do nothing here
	}

	void ProceduralTerrainCreationComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool ProceduralTerrainCreationComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		bool success = GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Resolution")
		{
			this->resolution->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BaseHeight")
		{
			this->baseHeight->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "HillAmplitude")
		{
			this->hillAmplitude->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "HillFrequency")
		{
			this->hillFrequency->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Octaves")
		{
			this->octaves->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Persistence")
		{
			this->persistence->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Lacunarity")
		{
			this->lacunarity->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Seed")
		{
			this->seed->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "EnableRoads")
		{
			this->enableRoads->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RoadCount")
		{
			this->roadCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RoadWidth")
		{
			this->roadWidth->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RoadDepth")
		{
			this->roadDepth->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RoadSmoothness")
		{
			this->roadSmoothness->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RoadCurviness")
		{
			this->roadCurviness->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RoadsClosed")
		{
			this->roadsClosed->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		// River loading:
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "EnableRivers")
		{
			this->enableRivers->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RiverCount")
		{
			this->riverCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RiverWidth")
		{
			this->riverWidth->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RiverDepth")
		{
			this->riverDepth->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RiverMeandering")
		{
			this->riverMeandering->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "EnableCanyons")
		{
			this->enableCanyons->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CanyonCount")
		{
			this->canyonCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CanyonWidth")
		{
			this->canyonWidth->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CanyonDepth")
		{
			this->canyonDepth->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CanyonSteepness")
		{
			this->canyonSteepness->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "EnableIsland")
		{
			this->enableIsland->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "IslandFalloff")
		{
			this->islandFalloff->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "IslandSize")
		{
			this->islandSize->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "EnableErosion")
		{
			this->enableErosion->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ErosionIterations")
		{
			this->erosionIterations->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ErosionStrength")
		{
			this->erosionStrength->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SedimentCapacity")
		{
			this->sedimentCapacity->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RiverFlowThreshold")
		{
			this->riverFlowThreshold->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RiverWidthScale")
		{
			this->riverWidthScale->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RiverErosionFactor")
		{
			this->riverErosionFactor->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxRiverDepth")
		{
			this->maxRiverDepth->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CanyonMinWidth")
		{
			this->canyonMinWidth->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CanyonMaxWidth")
		{
			this->canyonMaxWidth->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		return success;
	}

	GameObjectCompPtr ProceduralTerrainCreationComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		return nullptr;
	}

	bool ProceduralTerrainCreationComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralTerrainCreationComponent] Init component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool ProceduralTerrainCreationComponent::connect(void)
	{
		GameObjectComponent::connect();

		return true;
	}

	bool ProceduralTerrainCreationComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();

		return true;
	}

	void ProceduralTerrainCreationComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();
	}

	void ProceduralTerrainCreationComponent::update(Ogre::Real dt, bool notSimulating)
	{
		// This component doesn't need updates - generation is triggered by parameter changes
	}

	void ProceduralTerrainCreationComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (ProceduralTerrainCreationComponent::AttrResolution() == attribute->getName())
		{
			this->setResolution(attribute->getUInt());
		}
		else if (ProceduralTerrainCreationComponent::AttrBaseHeight() == attribute->getName())
		{
			this->setBaseHeight(attribute->getReal());
		}
		else if (ProceduralTerrainCreationComponent::AttrHillAmplitude() == attribute->getName())
		{
			this->setHillAmplitude(attribute->getReal());
		}
		else if (ProceduralTerrainCreationComponent::AttrHillFrequency() == attribute->getName())
		{
			this->setHillFrequency(attribute->getReal());
		}
		else if (ProceduralTerrainCreationComponent::AttrOctaves() == attribute->getName())
		{
			this->setOctaves(attribute->getUInt());
		}
		else if (ProceduralTerrainCreationComponent::AttrPersistence() == attribute->getName())
		{
			this->setPersistence(attribute->getReal());
		}
		else if (ProceduralTerrainCreationComponent::AttrLacunarity() == attribute->getName())
		{
			this->setLacunarity(attribute->getReal());
		}
		else if (ProceduralTerrainCreationComponent::AttrSeed() == attribute->getName())
		{
			this->setSeed(attribute->getUInt());
		}
		else if (ProceduralTerrainCreationComponent::AttrEnableRoads() == attribute->getName())
		{
			this->setEnableRoads(attribute->getBool());
		}
		else if (ProceduralTerrainCreationComponent::AttrRoadCount() == attribute->getName())
		{
			this->setRoadCount(attribute->getUInt());
		}
		else if (ProceduralTerrainCreationComponent::AttrRoadWidth() == attribute->getName())
		{
			this->setRoadWidth(attribute->getReal());
		}
		else if (ProceduralTerrainCreationComponent::AttrRoadDepth() == attribute->getName())
		{
			this->setRoadDepth(attribute->getReal());
		}
		else if (ProceduralTerrainCreationComponent::AttrRoadSmoothness() == attribute->getName())
		{
			this->setRoadSmoothness(attribute->getReal());
		}
		else if (ProceduralTerrainCreationComponent::AttrRoadCurviness() == attribute->getName())
		{
			this->setRoadCurviness(attribute->getReal());
		}
		else if (ProceduralTerrainCreationComponent::AttrRoadsClosed() == attribute->getName())
		{
			this->setRoadsClosed(attribute->getBool());
		}
		else if (ProceduralTerrainCreationComponent::AttrEnableRivers() == attribute->getName())
		{
			this->setEnableRivers(attribute->getBool());
		}
		else if (ProceduralTerrainCreationComponent::AttrRiverCount() == attribute->getName())
		{
			this->setRiverCount(attribute->getUInt());
		}
		else if (ProceduralTerrainCreationComponent::AttrRiverWidth() == attribute->getName())
		{
			this->setRiverWidth(attribute->getReal());
		}
		else if (ProceduralTerrainCreationComponent::AttrRiverDepth() == attribute->getName())
		{
			this->setRiverDepth(attribute->getReal());
		}
		else if (ProceduralTerrainCreationComponent::AttrRiverMeandering() == attribute->getName())
		{
			this->setRiverMeandering(attribute->getReal());
		}
		else if (ProceduralTerrainCreationComponent::AttrEnableCanyons() == attribute->getName())
		{
			this->setEnableCanyons(attribute->getBool());
		}
		else if (ProceduralTerrainCreationComponent::AttrCanyonCount() == attribute->getName())
		{
			this->setCanyonCount(attribute->getUInt());
		}
		else if (ProceduralTerrainCreationComponent::AttrCanyonWidth() == attribute->getName())
		{
			this->setCanyonWidth(attribute->getReal());
		}
		else if (ProceduralTerrainCreationComponent::AttrCanyonDepth() == attribute->getName())
		{
			this->setCanyonDepth(attribute->getReal());
		}
		else if (ProceduralTerrainCreationComponent::AttrCanyonSteepness() == attribute->getName())
		{
			this->setCanyonSteepness(attribute->getReal());
		}
		else if (ProceduralTerrainCreationComponent::AttrEnableIsland() == attribute->getName())
		{
			this->setEnableIsland(attribute->getBool());
		}
		else if (ProceduralTerrainCreationComponent::AttrIslandFalloff() == attribute->getName())
		{
			this->setIslandFalloff(attribute->getReal());
		}
		else if (ProceduralTerrainCreationComponent::AttrIslandSize() == attribute->getName())
		{
			this->setIslandSize(attribute->getReal());
		}
		else if (ProceduralTerrainCreationComponent::AttrEnableErosion() == attribute->getName())
		{
			this->setEnableErosion(attribute->getBool());
		}
		else if (ProceduralTerrainCreationComponent::AttrErosionIterations() == attribute->getName())
		{
			this->setErosionIterations(attribute->getUInt());
		}
		else if (ProceduralTerrainCreationComponent::AttrErosionStrength() == attribute->getName())
		{
			this->setErosionStrength(attribute->getReal());
		}
		else if (ProceduralTerrainCreationComponent::AttrSedimentCapacity() == attribute->getName())
		{
			this->setSedimentCapacity(attribute->getReal());
		}
		else if (ProceduralTerrainCreationComponent::AttrRiverFlowThreshold() == attribute->getName())
		{
			this->setRiverFlowThreshold(attribute->getReal());
		}
		else if (ProceduralTerrainCreationComponent::AttrRiverWidthScale() == attribute->getName())
		{
			this->setRiverWidthScale(attribute->getReal());
		}
		else if (ProceduralTerrainCreationComponent::AttrRiverErosionFactor() == attribute->getName())
		{
			this->setRiverErosionFactor(attribute->getReal());
		}
		else if (ProceduralTerrainCreationComponent::AttrMaxRiverDepth() == attribute->getName())
		{
			this->setMaxRiverDepth(attribute->getReal());
		}
		else if (ProceduralTerrainCreationComponent::AttrCanyonMinWidth() == attribute->getName())
		{
			this->setCanyonMinWidth(attribute->getReal());
		}
		else if (ProceduralTerrainCreationComponent::AttrCanyonMaxWidth() == attribute->getName())
		{
			this->setCanyonMaxWidth(attribute->getReal());
		}

	}

	void ProceduralTerrainCreationComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		GameObjectComponent::writeXML(propertiesXML, doc);

		xml_node<>*  propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Resolution"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->resolution->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BaseHeight"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->baseHeight->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "HillAmplitude"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->hillAmplitude->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "HillFrequency"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->hillFrequency->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Octaves"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->octaves->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Persistence"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->persistence->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Lacunarity"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->lacunarity->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Seed"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->seed->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "EnableRoads"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->enableRoads->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RoadCount"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->roadCount->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RoadWidth"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->roadWidth->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RoadDepth"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->roadDepth->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RoadSmoothness"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->roadSmoothness->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RoadCurviness"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->roadCurviness->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RoadsClosed"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->roadsClosed->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "EnableRivers"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->enableRivers->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RiverCount"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->riverCount->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RiverFlowThreshold"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->riverFlowThreshold->getReal())));
		propertiesXML->append_node(propertyXML);

	}

	Ogre::String ProceduralTerrainCreationComponent::getClassName(void) const
	{
		return "ProceduralTerrainCreationComponent";
	}

	Ogre::String ProceduralTerrainCreationComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	//------------------------------------------------------------------------------------------------------
// Proper Perlin noise implementation based on Ken Perlin's improved algorithm
//------------------------------------------------------------------------------------------------------

// Permutation table for Perlin noise
	static const int permutation[512] = {
		151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
		8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
		35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
		134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
		55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208, 89,
		18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
		250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
		189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
		172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
		228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
		107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
		138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180,
		// Repeat the array to avoid overflow
		151,160,137,91,90,15,131,13,201,95,96,53,194,233,7,225,140,36,103,30,69,142,
		8,99,37,240,21,10,23,190,6,148,247,120,234,75,0,26,197,62,94,252,219,203,117,
		35,11,32,57,177,33,88,237,149,56,87,174,20,125,136,171,168,68,175,74,165,71,
		134,139,48,27,166,77,146,158,231,83,111,229,122,60,211,133,230,220,105,92,41,
		55,46,245,40,244,102,143,54,65,25,63,161,1,216,80,73,209,76,132,187,208, 89,
		18,169,200,196,135,130,116,188,159,86,164,100,109,198,173,186,3,64,52,217,226,
		250,124,123,5,202,38,147,118,126,255,82,85,212,207,206,59,227,47,16,58,17,182,
		189,28,42,223,183,170,213,119,248,152,2,44,154,163,70,221,153,101,155,167,43,
		172,9,129,22,39,253,19,98,108,110,79,113,224,232,178,185,112,104,218,246,97,
		228,251,34,242,193,238,210,144,12,191,179,162,241,81,51,145,235,249,14,239,
		107,49,192,214,31,181,199,106,157,184,84,204,176,115,121,50,45,127,4,150,254,
		138,236,205,93,222,114,67,29,24,72,243,141,128,195,78,66,215,61,156,180
	};

	bool ProceduralTerrainCreationComponent::executeAction(const Ogre::String& actionId, NOWA::Variant* attribute)
	{
		if ("ProceduralTerrainCreationComponent.Regenerate" == actionId)
		{
			this->generateProceduralTerrain();
			return true;
		}
		return false;
	}

	float ProceduralTerrainCreationComponent::fade(float t)
	{
		// Improved fade function: 6t^5 - 15t^4 + 10t^3
		return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
	}

	float ProceduralTerrainCreationComponent::lerp(float t, float a, float b)
	{
		return a + t * (b - a);
	}

	float ProceduralTerrainCreationComponent::grad(int hash, float x, float y)
	{
		// 2D Perlin gradients: 8 directions
		int h = hash & 7;                 // 0..7
		float u = (h < 4) ? x : y;
		float v = (h < 4) ? y : x;
		return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
	}

	float ProceduralTerrainCreationComponent::noise(float x, float y, int seed)
	{
		int xi = static_cast<int>(std::floor(x));
		int yi = static_cast<int>(std::floor(y));

		int X = xi & 255;
		int Y = yi & 255;

		float xf = x - static_cast<float>(xi);
		float yf = y - static_cast<float>(yi);

		float u = fade(xf);
		float v = fade(yf);

		// Seed affects the permutation chain (NOT x/y)
		int s = seed & 255;

		int A = permutation[(X + s) & 255] + Y;
		int B = permutation[(X + 1 + s) & 255] + Y;

		int aa = permutation[A & 255];
		int ab = permutation[(A + 1) & 255];
		int ba = permutation[B & 255];
		int bb = permutation[(B + 1) & 255];

		float x1 = lerp(u, grad(aa, xf, yf), grad(ba, xf - 1.0f, yf));
		float x2 = lerp(u, grad(ab, xf, yf - 1.0f), grad(bb, xf - 1.0f, yf - 1.0f));

		return lerp(v, x1, x2);
	}

	float ProceduralTerrainCreationComponent::smoothNoise(float x, float y, int seed)
	{
		// Not needed with proper Perlin noise
		return noise(x, y, seed);
	}

	float ProceduralTerrainCreationComponent::interpolate(float a, float b, float x)
	{
		// Cosine interpolation
		float ft = x * 3.1415927f;
		float f = (1.0f - std::cos(ft)) * 0.5f;
		return a * (1.0f - f) + b * f;
	}

	float ProceduralTerrainCreationComponent::perlinNoise(float x, float y)
	{
		Ogre::uint32 res = this->resolution->getUInt();
		float nx = x / static_cast<float>(res);
		float ny = y / static_cast<float>(res);

		float total = 0.0f;
		float maxValue = 0.0f;
		float amplitude = 1.0f;

		float frequency = this->hillFrequency->getReal();
		Ogre::uint32 octaveCount = this->octaves->getUInt();
		float persist = this->persistence->getReal();
		float lacun = this->lacunarity->getReal();
		int noiseSeed = static_cast<int>(this->seed->getUInt());

		for (Ogre::uint32 i = 0; i < octaveCount; ++i)
		{
			float sampleX = nx * frequency;
			float sampleY = ny * frequency;

			float noiseValue = noise(sampleX, sampleY, noiseSeed + static_cast<int>(i));

			total += noiseValue * amplitude;
			maxValue += amplitude;

			amplitude *= persist;
			frequency *= lacun;
		}

		// Normalize to [-1, 1] then to [0, 1]
		float normalized = (total / maxValue + 1.0f) * 0.5f;

		// Apply baseHeight and hillAmplitude
		// baseHeight = center point (0.5 = middle)
		// hillAmplitude = variation range (0.1 = ±10% from base)
		float base = this->baseHeight->getReal();
		float amp = this->hillAmplitude->getReal();

		// Center noise around base, scale by amplitude
		float result = base + (normalized - 0.5f) * amp * 2.0f;

		// Clamp to valid [0,1] range
		return Ogre::Math::Clamp(result, 0.0f, 1.0f);
	}

	//------------------------------------------------------------------------------------------------------
	// Road generation implementation with proper terrain carving
	//------------------------------------------------------------------------------------------------------

	std::vector<ProceduralTerrainCreationComponent::RoadPoint>
		ProceduralTerrainCreationComponent::generateRoadPath(
			const Ogre::Vector2& startPixels,
			const Ogre::Vector2& endPixels,
			int numSegments)
	{
		std::vector<RoadPoint> roadPoints;

		Ogre::uint32 width = this->resolution->getUInt();
		Ogre::uint32 height = this->resolution->getUInt();

		float curviness = this->roadCurviness->getReal();
		bool isClosed = this->roadsClosed->getBool();

		std::vector<Ogre::Vector2> controlPoints;
		std::mt19937 rng(this->seed->getUInt() + 789);

		if (isClosed)
		{
			// FIXED: Create a reasonably-sized closed circuit
			// Use 25-35% of terrain size, not 60%!
			Ogre::Vector2 center(width * 0.5f, height * 0.5f);
			float baseRadius = std::min(width, height) * 0.25f;  // 25% of terrain = reasonable track

			int numCorners = 8 + static_cast<int>(curviness * 4);

			std::uniform_real_distribution<float> radiusVar(0.85f, 1.15f);
			std::uniform_real_distribution<float> angleJitter(-0.1f, 0.1f);

			for (int i = 0; i < numCorners; ++i)
			{
				float baseAngle = (i * 2.0f * Ogre::Math::PI) / numCorners;
				float angle = baseAngle + angleJitter(rng) * curviness;
				float r = baseRadius * radiusVar(rng);

				Ogre::Vector2 point(
					center.x + std::cos(angle) * r,
					center.y + std::sin(angle) * r
				);

				// Keep within bounds
				float margin = width * 0.1f;
				point.x = Ogre::Math::Clamp(point.x, margin, width - margin);
				point.y = Ogre::Math::Clamp(point.y, margin, height - margin);

				controlPoints.push_back(point);
			}

			// Close the loop
			controlPoints.push_back(controlPoints[0]);
			controlPoints.push_back(controlPoints[1]);
			controlPoints.push_back(controlPoints[2]);
		}
		else
		{
			// Open road code stays the same...
			controlPoints.push_back(startPixels);

			int numWaypoints = 3 + static_cast<int>(curviness * 4);
			Ogre::Vector2 dir = (endPixels - startPixels);
			float roadLength = dir.length();
			dir.normalise();
			Ogre::Vector2 perp(-dir.y, dir.x);

			float maxOffset = roadLength * 0.15f * curviness;
			std::uniform_real_distribution<float> offsetDist(-maxOffset, maxOffset);

			for (int i = 1; i < numWaypoints; ++i)
			{
				float t = i / static_cast<float>(numWaypoints);
				Ogre::Vector2 basePoint = startPixels + (endPixels - startPixels) * t;
				controlPoints.push_back(basePoint + perp * offsetDist(rng));
			}

			controlPoints.push_back(endPixels);

			// Ghost points for Catmull-Rom
			Ogre::Vector2 preStart = controlPoints[0] + (controlPoints[0] - controlPoints[1]);
			Ogre::Vector2 postEnd = controlPoints.back() + (controlPoints.back() - controlPoints[controlPoints.size() - 2]);
			controlPoints.insert(controlPoints.begin(), preStart);
			controlPoints.push_back(postEnd);
		}

		// Generate smooth path
		for (int i = 0; i < numSegments; ++i)
		{
			float t = i / static_cast<float>(numSegments - 1);
			Ogre::Vector2 point = evaluateCatmullRom(controlPoints, t);

			RoadPoint rp;
			rp.position = point;
			rp.width = this->roadWidth->getReal();  // Store in meters, convert later
			rp.targetHeight = 0.0f;  // Will be set by calculateRoadHeights
			roadPoints.push_back(rp);
		}

		return roadPoints;
	}

	Ogre::Vector2 ProceduralTerrainCreationComponent::evaluateCatmullRom(
		const std::vector<Ogre::Vector2>& points, float t)
	{
		if (points.size() < 4)
			return points.size() > 0 ? points[0] : Ogre::Vector2::ZERO;

		int numSegments = points.size() - 3;
		float scaledT = t * numSegments;
		int segment = Ogre::Math::Clamp(static_cast<int>(scaledT), 0, numSegments - 1);
		float localT = scaledT - segment;

		// Clamp segment to valid range
		segment = std::min(segment, static_cast<int>(points.size()) - 4);

		Ogre::Vector2 p0 = points[segment];
		Ogre::Vector2 p1 = points[segment + 1];
		Ogre::Vector2 p2 = points[segment + 2];
		Ogre::Vector2 p3 = points[segment + 3];

		float t2 = localT * localT;
		float t3 = t2 * localT;

		return 0.5f * ((2.0f * p1) +
			(-p0 + p2) * localT +
			(2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
			(-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
	}

	// NEW: Calculate road height by sampling terrain along the road path
	void ProceduralTerrainCreationComponent::calculateRoadHeights(
		std::vector<std::vector<RoadPoint>>& roads,
		const std::vector<float>& baseTerrainHeights,
		Ogre::uint32 width,
		Ogre::uint32 height)
	{
		for (auto& road : roads)
		{
			for (auto& point : road)
			{
				// Sample the base terrain height at this road point
				int x = static_cast<int>(Ogre::Math::Clamp(point.position.x, 0.0f, static_cast<float>(width - 1)));
				int y = static_cast<int>(Ogre::Math::Clamp(point.position.y, 0.0f, static_cast<float>(height - 1)));
				int index = y * width + x;

				if (index >= 0 && index < static_cast<int>(baseTerrainHeights.size()))
				{
					point.targetHeight = baseTerrainHeights[index];
				}
			}

			// Smooth road heights to prevent steep slopes
			smoothRoadHeights(road);
		}
	}

	void ProceduralTerrainCreationComponent::smoothRoadHeights(std::vector<RoadPoint>& road)
	{
		if (road.size() < 3) return;

		// Multiple smoothing passes for gentler slopes
		int smoothPasses = 3;
		for (int pass = 0; pass < smoothPasses; ++pass)
		{
			std::vector<float> smoothedHeights(road.size());

			// First and last points stay the same
			smoothedHeights[0] = road[0].targetHeight;
			smoothedHeights[road.size() - 1] = road[road.size() - 1].targetHeight;

			// Average neighboring heights
			for (size_t i = 1; i < road.size() - 1; ++i)
			{
				smoothedHeights[i] = (road[i - 1].targetHeight + road[i].targetHeight + road[i + 1].targetHeight) / 3.0f;
			}

			// Apply smoothed heights
			for (size_t i = 0; i < road.size(); ++i)
			{
				road[i].targetHeight = smoothedHeights[i];
			}
		}
	}

	float ProceduralTerrainCreationComponent::applyRoadCarving(
		float baseHeight,
		const Ogre::Vector2& pixelPosition,
		const std::vector<std::vector<RoadPoint>>& roads,
		Ogre::uint32 width,
		Ogre::uint32 height)
	{
		if (roads.empty())
			return baseHeight;

		// Convert road width from METERS to PIXELS
		float terrainWidthMeters = 100.0f;  // Your Terra X dimension
		float pixelsPerMeter = static_cast<float>(width) / terrainWidthMeters;

		float roadWidthMeters = this->roadWidth->getReal();        // e.g., 5 meters
		float roadWidthPixels = roadWidthMeters * pixelsPerMeter;  // e.g., ~51 pixels
		float smoothness = this->roadSmoothness->getReal();        // Blend multiplier

		// roadDepth is now in normalized height units [0-1]
		// 0.05 = 5% of height range = noticeable cut
		float roadDepthNorm = this->roadDepth->getReal();

		// Find closest distance to any road segment
		float minDistToRoad = std::numeric_limits<float>::max();
		float closestRoadHeight = baseHeight;

		for (const auto& road : roads)
		{
			for (size_t i = 1; i < road.size(); ++i)
			{
				float dist = getDistanceToLine(pixelPosition, road[i - 1].position, road[i].position);
				if (dist < minDistToRoad)
				{
					minDistToRoad = dist;
					// Average height of the two road points
					closestRoadHeight = (road[i - 1].targetHeight + road[i].targetHeight) * 0.5f;
				}
			}
		}

		// Define zones (in pixels)
		float halfWidth = roadWidthPixels * 0.5f;
		float blendWidth = roadWidthPixels * smoothness;

		if (minDistToRoad > blendWidth)
		{
			// Outside road influence
			return baseHeight;
		}

		// Calculate blend factor
		float influence = 0.0f;
		if (minDistToRoad < halfWidth)
		{
			// Core road area - full effect
			influence = 1.0f;
		}
		else
		{
			// Blend zone - smooth transition
			float t = (minDistToRoad - halfWidth) / (blendWidth - halfWidth);
			influence = 1.0f - (t * t * (3.0f - 2.0f * t));  // Smoothstep
		}

		// Road surface: flatten toward the road's local height, then cut down slightly
		// This allows roads to follow terrain contours while being slightly depressed
		float roadSurface = closestRoadHeight - roadDepthNorm;

		// Blend between original terrain and road surface
		float result = baseHeight + (roadSurface - baseHeight) * influence;

		return result;
	}

	float ProceduralTerrainCreationComponent::getHeightBilinear(const std::vector<float>& heights, float x, float y, Ogre::uint32 width, Ogre::uint32 height)
	{
		// Clamp to valid range
		x = Ogre::Math::Clamp(x, 0.0f, (float)(width - 1));
		y = Ogre::Math::Clamp(y, 0.0f, (float)(height - 1));

		int x0 = (int)std::floor(x);
		int y0 = (int)std::floor(y);
		int x1 = std::min(x0 + 1, (int)width - 1);
		int y1 = std::min(y0 + 1, (int)height - 1);

		float fx = x - x0;
		float fy = y - y0;

		float h00 = heights[y0 * width + x0];
		float h10 = heights[y0 * width + x1];
		float h01 = heights[y1 * width + x0];
		float h11 = heights[y1 * width + x1];

		float h0 = h00 * (1 - fx) + h10 * fx;
		float h1 = h01 * (1 - fx) + h11 * fx;

		return h0 * (1 - fy) + h1 * fy;
	}

	Ogre::Vector2 ProceduralTerrainCreationComponent::getGradient(const std::vector<float>& heights, int x, int y, Ogre::uint32 width, Ogre::uint32 height)
	{
		// Clamp to valid range
		x = Ogre::Math::Clamp(x, 1, (int)width - 2);
		y = Ogre::Math::Clamp(y, 1, (int)height - 2);

		// Central difference
		float dx = heights[y * width + (x + 1)] - heights[y * width + (x - 1)];
		float dy = heights[(y + 1) * width + x] - heights[(y - 1) * width + x];

		return Ogre::Vector2(dx * 0.5f, dy * 0.5f);
	}

	float ProceduralTerrainCreationComponent::getDistanceToLine(const Ogre::Vector2& point, const Ogre::Vector2& lineStart, const Ogre::Vector2& lineEnd)
	{
		Ogre::Vector2 line = lineEnd - lineStart;
		float lineLengthSq = line.squaredLength();

		// Degenerate case: line is actually a point
		if (lineLengthSq < 0.0001f)
			return (point - lineStart).length();

		// Project point onto line segment
		// t represents position along line: 0 = start, 1 = end
		float t = ((point - lineStart).dotProduct(line)) / lineLengthSq;

		// Clamp to segment (this is CRITICAL - without this, distance goes to infinity)
		t = Ogre::Math::Clamp(t, 0.0f, 1.0f);

		// Find closest point on segment
		Ogre::Vector2 closestPoint = lineStart + line * t;

		return (point - closestPoint).length();
	}

	// Simple placeholder - can be left empty or return identity field
	void ProceduralTerrainCreationComponent::generateTensorField(std::vector<Ogre::Vector2>& tensorField, Ogre::uint32 width, Ogre::uint32 height)
	{
		tensorField.resize(width * height);

		// Initialize with default directions (can be used for road guidance later)
		for (Ogre::uint32 y = 0; y < height; ++y)
		{
			for (Ogre::uint32 x = 0; x < width; ++x)
			{
				int index = y * width + x;
				// Default: mostly horizontal flow with some variation
				tensorField[index] = Ogre::Vector2(1.0f, 0.0f);
			}
		}
	}

	std::vector<ProceduralTerrainCreationComponent::DrainageNode> ProceduralTerrainCreationComponent::generateDrainageBasins(const std::vector<float>& heights, Ogre::uint32 width, Ogre::uint32 height)
	{
		std::vector<DrainageNode> nodes;
		nodes.resize(width * height);

		// Initialize all nodes
		for (Ogre::uint32 y = 0; y < height; ++y)
		{
			for (Ogre::uint32 x = 0; x < width; ++x)
			{
				int index = y * width + x;
				nodes[index].position = Ogre::Vector2(static_cast<float>(x), static_cast<float>(y));
				nodes[index].elevation = heights[index];
				nodes[index].flow = 1.0f;
				nodes[index].downstreamIndex = -1;
			}
		}

		// Find downstream neighbor for each node (steepest descent)
		for (Ogre::uint32 y = 0; y < height; ++y)
		{
			for (Ogre::uint32 x = 0; x < width; ++x)
			{
				int index = y * width + x;
				float currentHeight = heights[index];
				float steepestSlope = 0.0f;
				int steepestNeighbor = -1;

				// Check all 8 neighbors
				for (int dy = -1; dy <= 1; ++dy)
				{
					for (int dx = -1; dx <= 1; ++dx)
					{
						if (dx == 0 && dy == 0)
							continue;

						int nx = static_cast<int>(x) + dx;
						int ny = static_cast<int>(y) + dy;

						if (nx < 0 || nx >= static_cast<int>(width) || ny < 0 || ny >= static_cast<int>(height))
							continue;

						int neighborIndex = ny * width + nx;
						float neighborHeight = heights[neighborIndex];
						float slope = currentHeight - neighborHeight;

						if (slope > steepestSlope)
						{
							steepestSlope = slope;
							steepestNeighbor = neighborIndex;
						}
					}
				}

				nodes[index].downstreamIndex = steepestNeighbor;
			}
		}

		// Accumulate flow from upstream to downstream
		std::vector<int> sortedIndices;
		sortedIndices.reserve(width * height);
		for (size_t i = 0; i < nodes.size(); ++i)
			sortedIndices.push_back(static_cast<int>(i));

		// Sort by elevation (high to low)
		std::sort(sortedIndices.begin(), sortedIndices.end(),
			[&nodes](int a, int b)
			{
				return nodes[a].elevation > nodes[b].elevation;
			});

		// Accumulate flow
		for (int idx : sortedIndices)
		{
			if (nodes[idx].downstreamIndex >= 0)
			{
				nodes[nodes[idx].downstreamIndex].flow += nodes[idx].flow;
			}
		}

		return nodes;
	}

	void ProceduralTerrainCreationComponent::carveRivers(std::vector<float>& heights, const std::vector<DrainageNode>& nodes, Ogre::uint32 width, Ogre::uint32 height)
	{
		// Find maximum flow for normalization
		float maxFlow = 0.0f;
		for (const auto& node : nodes)
		{
			if (node.flow > maxFlow)
				maxFlow = node.flow;
		}

		if (maxFlow < 0.0001f)
			return;

		float flowThreshold = this->riverFlowThreshold->getReal() * maxFlow;

		for (const auto& node : nodes)
		{
			// Only carve where flow is significant
			if (node.flow < flowThreshold)
				continue;

			// CRITICAL: Scale river width with flow (logarithmic for natural look)
			float normalizedFlow = node.flow / maxFlow;
			float riverWidthPixels = this->riverWidth->getReal() * std::sqrt(normalizedFlow);

			// CRITICAL: Scale depth with flow but cap it
			float erosionDepth = this->riverDepth->getReal() * std::pow(normalizedFlow, 0.5f);
			erosionDepth = std::min(erosionDepth, this->maxRiverDepth->getReal());

			int centerX = static_cast<int>(node.position.x);
			int centerY = static_cast<int>(node.position.y);

			// Apply river carving in a radius
			int radius = static_cast<int>(riverWidthPixels * 1.5f);

			for (int dy = -radius; dy <= radius; ++dy)
			{
				for (int dx = -radius; dx <= radius; ++dx)
				{
					int x = centerX + dx;
					int y = centerY + dy;

					if (x < 0 || x >= static_cast<int>(width) ||
						y < 0 || y >= static_cast<int>(height))
						continue;

					float distance = std::sqrt(static_cast<float>(dx * dx + dy * dy));

					if (distance > riverWidthPixels)
						continue;

					// U-shaped river profile (not V-shaped)
					float normalizedDist = distance / riverWidthPixels;
					float profile = 1.0f - normalizedDist;
					profile = std::pow(profile, 0.7f); // Gentle U-shape

					int index = y * width + x;
					float targetHeight = node.elevation - erosionDepth * profile;

					// CRITICAL: Use minimum to carve down
					heights[index] = std::min(heights[index], targetHeight);
				}
			}
		}

		// CRITICAL: Smooth river banks for natural look
		std::vector<float> smoothed = heights;
		int smoothRadius = 2;

		for (Ogre::uint32 y = smoothRadius; y < height - smoothRadius; ++y)
		{
			for (Ogre::uint32 x = smoothRadius; x < width - smoothRadius; ++x)
			{
				int index = y * width + x;

				// 3x3 box blur
				float sum = 0.0f;
				int count = 0;

				for (int dy = -smoothRadius; dy <= smoothRadius; ++dy)
				{
					for (int dx = -smoothRadius; dx <= smoothRadius; ++dx)
					{
						sum += heights[(y + dy) * width + (x + dx)];
						count++;
					}
				}

				// Only smooth areas near rivers (where height changed significantly)
				float avg = sum / count;
				if (std::abs(heights[index] - avg) > 0.5f)
				{
					smoothed[index] = heights[index] * 0.6f + avg * 0.4f;
				}
			}
		}

		heights = smoothed;
	}

	void ProceduralTerrainCreationComponent::generateCanyons(std::vector<CanyonPath>& canyons, Ogre::uint32 width, Ogre::uint32 height)
	{
		std::mt19937 rng(this->seed->getULong());
		std::uniform_real_distribution<float> dist(0.0f, 1.0f);
		std::uniform_real_distribution<float> angleDist(-1.0f, 1.0f);

		int numCanyons = this->canyonCount->getInt();
		canyons.clear();
		canyons.reserve(numCanyons);

		for (int i = 0; i < numCanyons; ++i)
		{
			CanyonPath canyon;

			// Random starting position along edges
			int edge = static_cast<int>(dist(rng) * 4);
			Ogre::Vector2 startPos;
			Ogre::Vector2 direction;

			switch (edge)
			{
			case 0: // Top edge
				startPos = Ogre::Vector2(dist(rng) * width, 0);
				direction = Ogre::Vector2(angleDist(rng) * 0.5f, 1.0f);
				break;
			case 1: // Right edge
				startPos = Ogre::Vector2(width - 1, dist(rng) * height);
				direction = Ogre::Vector2(-1.0f, angleDist(rng) * 0.5f);
				break;
			case 2: // Bottom edge
				startPos = Ogre::Vector2(dist(rng) * width, height - 1);
				direction = Ogre::Vector2(angleDist(rng) * 0.5f, -1.0f);
				break;
			default: // Left edge
				startPos = Ogre::Vector2(0, dist(rng) * height);
				direction = Ogre::Vector2(1.0f, angleDist(rng) * 0.5f);
				break;
			}

			direction.normalise();

			// Generate meandering canyon path
			Ogre::Vector2 currentPos = startPos;
			float widthScale = this->canyonWidth->getReal() / 15.0f;
			float currentWidth = (this->canyonMinWidth->getReal() + dist(rng) * (this->canyonMaxWidth->getReal() - this->canyonMinWidth->getReal())) * widthScale;
			currentWidth = Ogre::Math::Clamp(currentWidth, this->canyonMinWidth->getReal(), this->canyonMaxWidth->getReal());

			int maxSteps = static_cast<int>(std::max(width, height) * 1.5f);
			float stepSize = 2.0f;
			float angleChange = 0.0f;

			for (int step = 0; step < maxSteps; ++step)
			{
				if (currentPos.x < 0 || currentPos.x >= width || currentPos.y < 0 || currentPos.y >= height)
					break;

				canyon.points.push_back(currentPos);
				canyon.widths.push_back(currentWidth);
				canyon.depths.push_back(this->canyonDepth->getReal() * (0.8f + dist(rng) * 0.4f));

				// Add meandering using Perlin-like noise
				angleChange += (dist(rng) - 0.5f) * 0.3f;
				angleChange *= 0.95f; // Damping for smoother curves

				// Rotate direction
				float angle = std::atan2(direction.y, direction.x) + angleChange;
				direction = Ogre::Vector2(std::cos(angle), std::sin(angle));

				// Vary canyon width along path
				currentWidth += (dist(rng) - 0.5f) * 2.0f;
				currentWidth = Ogre::Math::Clamp(currentWidth, this->canyonMinWidth->getReal(), this->canyonMaxWidth->getReal());

				currentPos += direction * stepSize;
			}

			if (canyon.points.size() > 10) // Only add if canyon is long enough
				canyons.push_back(canyon);
		}
	}

	void ProceduralTerrainCreationComponent::applyCanyonsToTerrain(std::vector<float>& heights, const std::vector<CanyonPath>& canyons, Ogre::uint32 width, Ogre::uint32 height)
	{
		for (const auto& canyon : canyons)
		{
			// Process each canyon segment
			for (size_t i = 0; i < canyon.points.size(); ++i)
			{
				const Ogre::Vector2& point = canyon.points[i];
				float canyonWidth = canyon.widths[i];
				float canyonDepth = canyon.depths[i];

				int centerX = static_cast<int>(point.x);
				int centerY = static_cast<int>(point.y);

				int radius = static_cast<int>(canyonWidth * 0.5f);

				for (int dy = -radius; dy <= radius; ++dy)
				{
					for (int dx = -radius; dx <= radius; ++dx)
					{
						int x = centerX + dx;
						int y = centerY + dy;

						if (x < 0 || x >= static_cast<int>(width) ||
							y < 0 || y >= static_cast<int>(height))
							continue;

						float distance = std::sqrt(static_cast<float>(dx * dx + dy * dy));

						if (distance > canyonWidth * 0.5f)
							continue;

						int index = y * width + x;

						// CRITICAL: Gentle profile to prevent too-steep walls
						float normalizedDist = distance / (canyonWidth * 0.5f);

						// Use parabolic profile for gentle slopes
						// steepness < 1.0 = gentler, > 1.0 = steeper
						float steepness = Ogre::Math::Clamp(
							this->canyonSteepness->getReal(), 0.3f, 2.0f);

						float profile = 1.0f - std::pow(normalizedDist, steepness);
						profile = std::max(0.0f, profile);

						// CRITICAL: Don't directly subtract - blend for smoother result
						float erosion = canyonDepth * profile;
						float targetHeight = heights[index] - erosion;

						// Smooth transition
						heights[index] = heights[index] * 0.3f + targetHeight * 0.7f;
					}
				}
			}
		}

		// CRITICAL: Multi-pass smoothing to prevent sharp edges
		for (int pass = 0; pass < 3; ++pass)
		{
			std::vector<float> smoothed = heights;

			for (Ogre::uint32 y = 1; y < height - 1; ++y)
			{
				for (Ogre::uint32 x = 1; x < width - 1; ++x)
				{
					int index = y * width + x;

					// Weighted average with neighbors
					float center = heights[index] * 0.5f;
					float neighbors = (
						heights[(y - 1) * width + x] +
						heights[(y + 1) * width + x] +
						heights[y * width + (x - 1)] +
						heights[y * width + (x + 1)]
						) * 0.125f;

					smoothed[index] = center + neighbors;
				}
			}

			heights = smoothed;
		}
	}

	void ProceduralTerrainCreationComponent::simulateHydraulicErosion(
		std::vector<float>& heights, Ogre::uint32 width, Ogre::uint32 height)
	{
		Ogre::uint32 numParticles = this->erosionIterations->getUInt();
		float erosionStr = this->erosionStrength->getReal();
		float sedimentCap = this->sedimentCapacity->getReal();

		std::mt19937 rng(this->seed->getUInt() + 777);
		std::uniform_real_distribution<float> distX(0, width - 1);
		std::uniform_real_distribution<float> distY(0, height - 1);

		for (Ogre::uint32 i = 0; i < numParticles; ++i)
		{
			ErosionParticle particle;
			particle.position = Ogre::Vector2(distX(rng), distY(rng));
			particle.velocity = Ogre::Vector2::ZERO;
			particle.sediment = 0.0f;
			particle.water = 1.0f;

			// Simulate particle life
			for (int step = 0; step < 30 && particle.water > 0.01f; ++step)
			{
				updateParticle(particle, heights, width, height);

				// Move particle
				particle.position += particle.velocity;

				// Check bounds
				if (particle.position.x < 0 || particle.position.x >= width - 1 ||
					particle.position.y < 0 || particle.position.y >= height - 1)
				{
					break;
				}

				// Evaporate
				particle.water *= 0.95f;
			}

			// Deposit remaining sediment
			if (particle.sediment > 0 &&
				particle.position.x >= 0 && particle.position.x < width &&
				particle.position.y >= 0 && particle.position.y < height)
			{
				int x = (int)particle.position.x;
				int y = (int)particle.position.y;
				heights[y * width + x] += particle.sediment;
			}
		}
	}

	void ProceduralTerrainCreationComponent::updateParticle(ErosionParticle& particle, std::vector<float>& heights, Ogre::uint32 width, Ogre::uint32 height)
	{
		// Get current height
		float h = getHeightBilinear(heights, particle.position.x, particle.position.y, width, height);

		// Get gradient using bilinear sampling
		float hx = getHeightBilinear(heights, particle.position.x + 1.0f, particle.position.y, width, height);
		float hy = getHeightBilinear(heights, particle.position.x, particle.position.y + 1.0f, width, height);
		Ogre::Vector2 gradient(hx - h, hy - h);

		// CRITICAL: Scale gravity and friction for realistic behavior
		float gravity = 4.0f;  // Reduced for gentler erosion
		float inertia = 0.05f;  // How much particle resists direction change
		float minSlope = 0.01f; // Minimum slope to continue erosion

		// Update velocity using gradient
		Ogre::Vector2 newVelocity = particle.velocity * inertia + gradient * gravity * (1.0f - inertia);

		// Apply friction
		newVelocity *= 0.9f;

		particle.velocity = newVelocity;

		// Calculate erosion capacity based on speed and water
		float speed = particle.velocity.length();
		float capacity = std::max(0.0f, speed * particle.water * this->sedimentCapacity->getReal());

		// CRITICAL: Realistic erosion/deposition
		if (particle.sediment < capacity)
		{
			// Erode - but only if moving and on a slope
			if (speed > minSlope)
			{
				float amountToErode = this->erosionStrength->getReal() *
					(capacity - particle.sediment) *
					speed;

				particle.sediment += amountToErode;

				// Remove from heightmap with bilinear distribution
				int x = static_cast<int>(particle.position.x);
				int y = static_cast<int>(particle.position.y);

				if (x >= 0 && x < static_cast<int>(width) - 1 &&
					y >= 0 && y < static_cast<int>(height) - 1)
				{
					// Distribute erosion across 4 nearest pixels (bilinear)
					float fx = particle.position.x - x;
					float fy = particle.position.y - y;

					heights[y * width + x] -= amountToErode * (1 - fx) * (1 - fy);
					heights[y * width + (x + 1)] -= amountToErode * fx * (1 - fy);
					heights[(y + 1) * width + x] -= amountToErode * (1 - fx) * fy;
					heights[(y + 1) * width + (x + 1)] -= amountToErode * fx * fy;
				}
			}
		}
		else
		{
			// Deposit excess sediment
			float amountToDeposit = (particle.sediment - capacity) * 0.3f;
			particle.sediment -= amountToDeposit;

			int x = static_cast<int>(particle.position.x);
			int y = static_cast<int>(particle.position.y);

			if (x >= 0 && x < static_cast<int>(width) - 1 &&
				y >= 0 && y < static_cast<int>(height) - 1)
			{
				// Deposit with bilinear distribution
				float fx = particle.position.x - x;
				float fy = particle.position.y - y;

				heights[y * width + x] += amountToDeposit * (1 - fx) * (1 - fy);
				heights[y * width + (x + 1)] += amountToDeposit * fx * (1 - fy);
				heights[(y + 1) * width + x] += amountToDeposit * (1 - fx) * fy;
				heights[(y + 1) * width + (x + 1)] += amountToDeposit * fx * fy;
			}
		}
	}

	void ProceduralTerrainCreationComponent::applyIslandMask(std::vector<float>& heights, Ogre::uint32 width, Ogre::uint32 height)
	{
		float falloff = this->islandFalloff->getReal();
		float islandSize = this->islandSize->getReal();

		Ogre::Vector2 center(width * 0.5f, height * 0.5f);
		float maxRadius = std::min(width, height) * 0.5f * islandSize;

		for (Ogre::uint32 y = 0; y < height; ++y)
		{
			for (Ogre::uint32 x = 0; x < width; ++x)
			{
				Ogre::Vector2 pos(static_cast<float>(x), static_cast<float>(y));
				float dist = (pos - center).length();

				// Normalized distance (0 at center, 1 at island edge)
				float normalizedDist = dist / maxRadius;

				// CRITICAL: Use smooth gradient, not harsh cutoff
				float mask = 1.0f - std::pow(
					Ogre::Math::Clamp(normalizedDist, 0.0f, 1.0f),
					falloff
				);

				// Add coastal variation using noise
				float coastNoise = perlinNoise(x * 0.03f, y * 0.03f) * 0.1f;
				mask = Ogre::Math::Clamp(mask + coastNoise, 0.0f, 1.0f);

				// Apply mask - CRITICAL: Use multiplication, not addition
				int index = y * width + x;
				heights[index] *= mask;

				// Lower ocean floor around island
				if (mask < 0.3f)
				{
					float oceanDepth = this->baseHeight->getReal() * 0.5f;
					heights[index] = Ogre::Math::lerp(
						-oceanDepth,
						heights[index],
						mask / 0.3f
					);
				}
			}
		}
	}

	//------------------------------------------------------------------------------------------------------
	// Main terrain generation
	//------------------------------------------------------------------------------------------------------

	Ogre::Terra* ProceduralTerrainCreationComponent::findTerraComponent(void)
	{
		auto terraCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<TerraComponent>());
		if (nullptr != terraCompPtr)
		{
			return terraCompPtr->getTerra();
		}

		return nullptr;
	}

	void ProceduralTerrainCreationComponent::generateProceduralTerrain(void)
	{
		Ogre::Terra* terra = this->findTerraComponent();
		if (nullptr == terra)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralTerrainCreationComponent] Cannot find Terra component!");
			return;
		}

		Ogre::uint32 width = this->resolution->getUInt(); // e.g., 1024 pixels
		Ogre::uint32 height = this->resolution->getUInt(); 

		Ogre::Vector2 xzDimensions = terra->getXZDimensions(); // e.g., 100m x 100m
		float terraHeightRange = terra->getHeight();
		Ogre::Vector3 terraOrigin = terra->getTerrainOrigin();
     
		cachedPixelsPerMeter = static_cast<float>(width) / xzDimensions.x;  // e.g., 10.24 px/m

		// Terra height range: terraOrigin.y to (terraOrigin.y + terraHeightRange)
		// With your config: -25m to +75m
		float terraMinHeight = terraOrigin.y;
		float terraMaxHeight = terraOrigin.y + terraHeightRange;

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL,
			"[ProceduralTerrain] ========================================");
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL,
			"[ProceduralTerrain] Resolution: " + Ogre::StringConverter::toString(width));
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL,
			"[ProceduralTerrain] Terra XZ: " + Ogre::StringConverter::toString(xzDimensions.x) +
			"m x " + Ogre::StringConverter::toString(xzDimensions.y) + "m");
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL,
			"[ProceduralTerrain] Terra Height: " + Ogre::StringConverter::toString(terraMinHeight) +
			"m to " + Ogre::StringConverter::toString(terraMaxHeight) + "m");
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL,
			"[ProceduralTerrain] Pixels/meter: " +
			Ogre::StringConverter::toString(width / xzDimensions.x));

		// Generate base terrain
		std::vector<float> heightData(width * height);

		// Step 1: Generate raw Perlin noise (already outputs [0,1] with baseHeight/amplitude)
		for (Ogre::uint32 y = 0; y < height; ++y)
		{
			for (Ogre::uint32 x = 0; x < width; ++x)
			{
				heightData[y * width + x] = perlinNoise(
					static_cast<float>(x),
					static_cast<float>(y));
			}
		}

		// Step 2: Apply roads (modifies the noise, flattens locally)
		std::vector<std::vector<RoadPoint>> roads;

		if (this->enableRoads->getBool())
		{
			Ogre::uint32 numRoads = this->roadCount->getUInt();
			float roadWidthMeters = this->roadWidth->getReal();
			float pixelsPerMeter = width / xzDimensions.x;

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL,
				"[ProceduralTerrain] Generating " + Ogre::StringConverter::toString(numRoads) + " road(s)");
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL,
				"[ProceduralTerrain] Road: " + Ogre::StringConverter::toString(roadWidthMeters) +
				"m = " + Ogre::StringConverter::toString(roadWidthMeters * pixelsPerMeter) + " pixels");

			if (this->roadsClosed->getBool())
			{
				for (Ogre::uint32 i = 0; i < numRoads; ++i)
				{
					Ogre::Vector2 center(width * 0.5f, height * 0.5f);
					float radius = std::min(width, height) * 0.25f;

					Ogre::Vector2 start = center + Ogre::Vector2(radius, 0);
					Ogre::Vector2 end = center + Ogre::Vector2(0, radius);

					roads.push_back(generateRoadPath(start, end, 300));
				}
			}
			else
			{
				std::mt19937 rng(this->seed->getUInt() + 999);
				std::uniform_real_distribution<float> distX(width * 0.15f, width * 0.85f);
				std::uniform_real_distribution<float> distY(height * 0.15f, height * 0.85f);

				for (Ogre::uint32 i = 0; i < numRoads; ++i)
				{
					Ogre::Vector2 start(distX(rng), distY(rng));
					Ogre::Vector2 end(distX(rng), distY(rng));
					roads.push_back(generateRoadPath(start, end, 200));
				}
			}

			calculateRoadHeights(roads, heightData, width, height);

			// Carve roads
			int affected = 0;
			for (Ogre::uint32 y = 0; y < height; ++y)
			{
				for (Ogre::uint32 x = 0; x < width; ++x)
				{
					int idx = y * width + x;
					float orig = heightData[idx];

					Ogre::Vector2 pos(static_cast<float>(x), static_cast<float>(y));
					heightData[idx] = applyRoadCarving(orig, pos, roads, width, height);

					if (std::abs(heightData[idx] - orig) > 0.1f)
						affected++;
				}
			}

			float percent = (affected * 100.0f) / (width * height);
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL,
				"[ProceduralTerrain] Roads affected " + Ogre::StringConverter::toString(percent) + "% of terrain");
		}

		// Apply features (all your existing code stays the same!)
		if (this->enableIsland->getBool())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL,
				"[ProceduralTerrain] Applying island mask...");
			applyIslandMask(heightData, width, height);
		}

		if (this->enableCanyons->getBool())
		{
			std::vector<CanyonPath> canyons;
			generateCanyons(canyons, width, height);
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL,
				"[ProceduralTerrain] Carving " + Ogre::StringConverter::toString(canyons.size()) + " canyons...");
			applyCanyonsToTerrain(heightData, canyons, width, height);
		}

		if (this->enableErosion->getBool())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL,
				"[ProceduralTerrain] Simulating erosion...");
			simulateHydraulicErosion(heightData, width, height);
		}

		if (this->enableRivers->getBool())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL,
				"[ProceduralTerrain] Generating rivers...");
			std::vector<Ogre::Vector2> tensorField;
			generateTensorField(tensorField, width, height);
			std::vector<DrainageNode> nodes = generateDrainageBasins(heightData, width, height);
			carveRivers(heightData, nodes, width, height);
		}

		// Find height range
		float minH = std::numeric_limits<float>::max();
		float maxH = std::numeric_limits<float>::lowest();
		for (const auto& h : heightData)
		{
			minH = std::min(minH, h);
			maxH = std::max(maxH, h);
		}

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL,
			"[ProceduralTerrain] Generated heights: " + Ogre::StringConverter::toString(minH) +
			"m to " + Ogre::StringConverter::toString(maxH) + "m");

		// Convert to image - DON'T renormalize, just clamp!
		Ogre::Image2 image;
		image.createEmptyImage(width, height, 1, Ogre::TextureTypes::Type2D, Ogre::PFG_R16_UNORM, 1);
		Ogre::uint16* imageData = reinterpret_cast<Ogre::uint16*>(image.getData(0).data);

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL,
			"[ProceduralTerrain] Height range: " + Ogre::StringConverter::toString(minH) +
			" to " + Ogre::StringConverter::toString(maxH));

		// Direct conversion - perlinNoise already outputs [0,1]
		for (Ogre::uint32 i = 0; i < width * height; ++i)
		{
			float value = Ogre::Math::Clamp(heightData[i], 0.0f, 1.0f);
			imageData[i] = static_cast<Ogre::uint16>(value * 65535.0f);
		}

		terra->createHeightmapTexture(image);

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL,
			"[ProceduralTerrain] Complete!");
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL,
			"[ProceduralTerrain] ========================================");

		this->terrainGenerated = true;
	}

	void ProceduralTerrainCreationComponent::setResolution(Ogre::uint32 resolution)
	{
		// Clamp to valid power-of-2 values
		// Maximum is 4096 due to Terra's shadow mapper buffer limitation
		// The shadow mapper allocates 4096u * 16u elements, and needs (height << 4u) elements
		// So maximum height = 4096
		if (resolution < 128)
			resolution = 128;
		if (resolution > 4096)
			resolution = 4096;
		this->resolution->setValue(resolution);
	}

	Ogre::uint32 ProceduralTerrainCreationComponent::getResolution(void) const
	{
		return this->resolution->getUInt();
	}

	void ProceduralTerrainCreationComponent::setBaseHeight(Ogre::Real baseHeight)
	{
		this->baseHeight->setValue(baseHeight);
	}

	Ogre::Real ProceduralTerrainCreationComponent::getBaseHeight(void) const
	{
		return this->baseHeight->getReal();
	}

	void ProceduralTerrainCreationComponent::setHillAmplitude(Ogre::Real hillAmplitude)
	{
		if (hillAmplitude < 0.0f) hillAmplitude = 0.0f;
		this->hillAmplitude->setValue(hillAmplitude);
	}

	Ogre::Real ProceduralTerrainCreationComponent::getHillAmplitude(void) const
	{
		return this->hillAmplitude->getReal();
	}

	void ProceduralTerrainCreationComponent::setHillFrequency(Ogre::Real hillFrequency)
	{
		if (hillFrequency < 0.0001f) hillFrequency = 0.0001f;
		if (hillFrequency > 1.0f) hillFrequency = 1.0f;
		this->hillFrequency->setValue(hillFrequency);
	}

	Ogre::Real ProceduralTerrainCreationComponent::getHillFrequency(void) const
	{
		return this->hillFrequency->getReal();
	}

	void ProceduralTerrainCreationComponent::setOctaves(Ogre::uint32 octaves)
	{
		if (octaves < 1) octaves = 1;
		if (octaves > 8) octaves = 8;
		this->octaves->setValue(octaves);
	}

	Ogre::uint32 ProceduralTerrainCreationComponent::getOctaves(void) const
	{
		return this->octaves->getUInt();
	}

	void ProceduralTerrainCreationComponent::setPersistence(Ogre::Real persistence)
	{
		if (persistence < 0.0f) persistence = 0.0f;
		if (persistence > 1.0f) persistence = 1.0f;
		this->persistence->setValue(persistence);
	}

	Ogre::Real ProceduralTerrainCreationComponent::getPersistence(void) const
	{
		return this->persistence->getReal();
	}

	void ProceduralTerrainCreationComponent::setLacunarity(Ogre::Real lacunarity)
	{
		if (lacunarity < 1.0f) lacunarity = 1.0f;
		this->lacunarity->setValue(lacunarity);
	}

	Ogre::Real ProceduralTerrainCreationComponent::getLacunarity(void) const
	{
		return this->lacunarity->getReal();
	}

	void ProceduralTerrainCreationComponent::setSeed(Ogre::uint32 seed)
	{
		this->seed->setValue(seed);
	}

	Ogre::uint32 ProceduralTerrainCreationComponent::getSeed(void) const
	{
		return this->seed->getUInt();
	}

	void ProceduralTerrainCreationComponent::setEnableRoads(bool enableRoads)
	{
		this->enableRoads->setValue(enableRoads);
	}

	bool ProceduralTerrainCreationComponent::getEnableRoads(void) const
	{
		return this->enableRoads->getBool();
	}

	void ProceduralTerrainCreationComponent::setRoadCount(Ogre::uint32 roadCount)
	{
		if (roadCount < 1) roadCount = 1;
		if (roadCount > 10) roadCount = 10;
		this->roadCount->setValue(roadCount);
	}

	Ogre::uint32 ProceduralTerrainCreationComponent::getRoadCount(void) const
	{
		return this->roadCount->getUInt();
	}

	void ProceduralTerrainCreationComponent::setRoadWidth(Ogre::Real roadWidth)
	{
		if (roadWidth < 1.0f) roadWidth = 1.0f;
		this->roadWidth->setValue(roadWidth);
	}

	Ogre::Real ProceduralTerrainCreationComponent::getRoadWidth(void) const
	{
		return this->roadWidth->getReal();
	}

	void ProceduralTerrainCreationComponent::setRoadDepth(Ogre::Real roadDepth)
	{
		if (roadDepth < 0.0f) roadDepth = 0.0f;
		this->roadDepth->setValue(roadDepth);
	}

	Ogre::Real ProceduralTerrainCreationComponent::getRoadDepth(void) const
	{
		return this->roadDepth->getReal();
	}

	void ProceduralTerrainCreationComponent::setRoadSmoothness(Ogre::Real roadSmoothness)
	{
		if (roadSmoothness < 1.0f) roadSmoothness = 1.0f;
		this->roadSmoothness->setValue(roadSmoothness);
	}

	Ogre::Real ProceduralTerrainCreationComponent::getRoadSmoothness(void) const
	{
		return this->roadSmoothness->getReal();
	}

	void ProceduralTerrainCreationComponent::setRoadCurviness(Ogre::Real roadCurviness)
	{
		if (roadCurviness < 0.0f) roadCurviness = 0.0f;
		if (roadCurviness > 1.0f) roadCurviness = 1.0f;
		this->roadCurviness->setValue(roadCurviness);
	}

	Ogre::Real ProceduralTerrainCreationComponent::getRoadCurviness(void) const
	{
		return this->roadCurviness->getReal();
	}

	void ProceduralTerrainCreationComponent::setRoadsClosed(bool roadsClosed)
	{
		this->roadsClosed->setValue(roadsClosed);
	}

	bool ProceduralTerrainCreationComponent::getRoadsClosed(void) const
	{
		return this->roadsClosed->getBool();
	}

	void ProceduralTerrainCreationComponent::setEnableRivers(bool enableRivers)
	{
		this->enableRivers->setValue(enableRivers);
	}

	bool ProceduralTerrainCreationComponent::getEnableRivers(void) const
	{
		return this->enableRivers->getBool();
	}

	void ProceduralTerrainCreationComponent::setRiverCount(Ogre::uint32 riverCount)
	{
		if (riverCount < 1) riverCount = 1;
		if (riverCount > 10) riverCount = 10;
		this->riverCount->setValue(riverCount);
	}

	Ogre::uint32 ProceduralTerrainCreationComponent::getRiverCount(void) const
	{
		return this->riverCount->getUInt();
	}

	void ProceduralTerrainCreationComponent::setRiverWidth(Ogre::Real riverWidth)
	{
		if (riverWidth < 0.1f) riverWidth = 0.1f;
		this->riverWidth->setValue(riverWidth);
	}

	Ogre::Real ProceduralTerrainCreationComponent::getRiverWidth(void) const
	{
		return this->riverWidth->getReal();
	}

	void ProceduralTerrainCreationComponent::setRiverDepth(Ogre::Real riverDepth)
	{
		if (riverDepth < 0.0f) riverDepth = 0.0f;
		this->riverDepth->setValue(riverDepth);
	}

	Ogre::Real ProceduralTerrainCreationComponent::getRiverDepth(void) const
	{
		return this->riverDepth->getReal();
	}

	void ProceduralTerrainCreationComponent::setRiverMeandering(Ogre::Real riverMeandering)
	{
		riverMeandering = Ogre::Math::Clamp(riverMeandering, 0.0f, 1.0f);
		this->riverMeandering->setValue(riverMeandering);
	}

	Ogre::Real ProceduralTerrainCreationComponent::getRiverMeandering(void) const
	{
		return this->riverMeandering->getReal();
	}

	void ProceduralTerrainCreationComponent::setEnableCanyons(bool enableCanyons)
	{
		this->enableCanyons->setValue(enableCanyons);
	}

	bool ProceduralTerrainCreationComponent::getEnableCanyons(void) const
	{
		return this->enableCanyons->getBool();
	}

	void ProceduralTerrainCreationComponent::setCanyonCount(Ogre::uint32 canyonCount)
	{
		if (canyonCount > 10) canyonCount = 10;
		this->canyonCount->setValue(canyonCount);
	}

	Ogre::uint32 ProceduralTerrainCreationComponent::getCanyonCount(void) const
	{
		return this->canyonCount->getUInt();
	}

	void ProceduralTerrainCreationComponent::setCanyonWidth(Ogre::Real canyonWidth)
	{
		if (canyonWidth < 0.1f) canyonWidth = 0.1f;
		this->canyonWidth->setValue(canyonWidth);
	}

	Ogre::Real ProceduralTerrainCreationComponent::getCanyonWidth(void) const
	{
		return this->canyonWidth->getReal();
	}

	void ProceduralTerrainCreationComponent::setCanyonDepth(Ogre::Real canyonDepth)
	{
		if (canyonDepth < 0.0f) canyonDepth = 0.0f;
		this->canyonDepth->setValue(canyonDepth);
	}

	Ogre::Real ProceduralTerrainCreationComponent::getCanyonDepth(void) const
	{
		return this->canyonDepth->getReal();
	}

	void ProceduralTerrainCreationComponent::setCanyonSteepness(Ogre::Real canyonSteepness)
	{
		canyonSteepness = Ogre::Math::Clamp(canyonSteepness, 0.1f, 10.0f);
		this->canyonSteepness->setValue(canyonSteepness);
	}

	Ogre::Real ProceduralTerrainCreationComponent::getCanyonSteepness(void) const
	{
		return this->canyonSteepness->getReal();
	}

	void ProceduralTerrainCreationComponent::setEnableIsland(bool enableIsland)
	{
		this->enableIsland->setValue(enableIsland);
	}

	bool ProceduralTerrainCreationComponent::getEnableIsland(void) const
	{
		return this->enableIsland->getBool();
	}

	void ProceduralTerrainCreationComponent::setIslandFalloff(Ogre::Real islandFalloff)
	{
		if (islandFalloff < 0.1f) islandFalloff = 0.1f;
		this->islandFalloff->setValue(islandFalloff);
	}

	Ogre::Real ProceduralTerrainCreationComponent::getIslandFalloff(void) const
	{
		return this->islandFalloff->getReal();
	}

	void ProceduralTerrainCreationComponent::setIslandSize(Ogre::Real islandSize)
	{
		islandSize = Ogre::Math::Clamp(islandSize, 0.1f, 1.0f);
		this->islandSize->setValue(islandSize);
	}

	Ogre::Real ProceduralTerrainCreationComponent::getIslandSize(void) const
	{
		return this->islandSize->getReal();
	}

	void ProceduralTerrainCreationComponent::setEnableErosion(bool enableErosion)
	{
		this->enableErosion->setValue(enableErosion);
	}

	bool ProceduralTerrainCreationComponent::getEnableErosion(void) const
	{
		return this->enableErosion->getBool();
	}

	void ProceduralTerrainCreationComponent::setErosionIterations(Ogre::uint32 erosionIterations)
	{
		if (erosionIterations > 200000u)
			erosionIterations = 200000u;
		this->erosionIterations->setValue(erosionIterations);
	}

	Ogre::uint32 ProceduralTerrainCreationComponent::getErosionIterations(void) const
	{
		return this->erosionIterations->getUInt();
	}

	void ProceduralTerrainCreationComponent::setErosionStrength(Ogre::Real erosionStrength)
	{
		if (erosionStrength < 0.0f)
			erosionStrength = 0.0f;
		this->erosionStrength->setValue(erosionStrength);
	}

	Ogre::Real ProceduralTerrainCreationComponent::getErosionStrength(void) const
	{
		return this->erosionStrength->getReal();
	}

	void ProceduralTerrainCreationComponent::setSedimentCapacity(Ogre::Real sedimentCapacity)
	{
		if (sedimentCapacity < 0.0f)
			sedimentCapacity = 0.0f;
		this->sedimentCapacity->setValue(sedimentCapacity);
	}

	Ogre::Real ProceduralTerrainCreationComponent::getSedimentCapacity(void) const
	{
		return this->sedimentCapacity->getReal();
	}

	void ProceduralTerrainCreationComponent::setRiverFlowThreshold(Ogre::Real riverFlowThreshold)
	{
		if (riverFlowThreshold < 1.0f)
			riverFlowThreshold = 1.0f;
		this->riverFlowThreshold->setValue(riverFlowThreshold);
	}

	Ogre::Real ProceduralTerrainCreationComponent::getRiverFlowThreshold(void) const
	{
		return this->riverFlowThreshold->getReal();
	}

	void ProceduralTerrainCreationComponent::setRiverWidthScale(Ogre::Real riverWidthScale)
	{
		if (riverWidthScale < 0.01f)
			riverWidthScale = 0.01f;
		this->riverWidthScale->setValue(riverWidthScale);
	}

	Ogre::Real ProceduralTerrainCreationComponent::getRiverWidthScale(void) const
	{
		return this->riverWidthScale->getReal();
	}

	void ProceduralTerrainCreationComponent::setRiverErosionFactor(Ogre::Real riverErosionFactor)
	{
		if (riverErosionFactor < 0.0f)
			riverErosionFactor = 0.0f;
		this->riverErosionFactor->setValue(riverErosionFactor);
	}

	Ogre::Real ProceduralTerrainCreationComponent::getRiverErosionFactor(void) const
	{
		return this->riverErosionFactor->getReal();
	}

	void ProceduralTerrainCreationComponent::setMaxRiverDepth(Ogre::Real maxRiverDepth)
	{
		if (maxRiverDepth < 0.0f)
			maxRiverDepth = 0.0f;
		this->maxRiverDepth->setValue(maxRiverDepth);
	}

	Ogre::Real ProceduralTerrainCreationComponent::getMaxRiverDepth(void) const
	{
		return this->maxRiverDepth->getReal();
	}

	void ProceduralTerrainCreationComponent::setCanyonMinWidth(Ogre::Real canyonMinWidth)
	{
		if (canyonMinWidth < 0.1f)
			canyonMinWidth = 0.1f;
		this->canyonMinWidth->setValue(canyonMinWidth);
		if (this->canyonMaxWidth->getReal() < canyonMinWidth)
			this->canyonMaxWidth->setValue(canyonMinWidth);
	}

	Ogre::Real ProceduralTerrainCreationComponent::getCanyonMinWidth(void) const
	{
		return this->canyonMinWidth->getReal();
	}

	void ProceduralTerrainCreationComponent::setCanyonMaxWidth(Ogre::Real canyonMaxWidth)
	{
		if (canyonMaxWidth < 0.1f)
			canyonMaxWidth = 0.1f;

		this->canyonMaxWidth->setValue(canyonMaxWidth);
		if (this->canyonMaxWidth->getReal() < this->canyonMinWidth->getReal())
			this->canyonMinWidth->setValue(this->canyonMaxWidth->getReal());
	}

	Ogre::Real ProceduralTerrainCreationComponent::getCanyonMaxWidth(void) const
	{
		return this->canyonMaxWidth->getReal();
	}

	void ProceduralTerrainCreationComponent::generateTerrain(void)
	{
		this->generateProceduralTerrain();
	}

	//------------------------------------------------------------------------------------------------------
	// Lua registration
	//------------------------------------------------------------------------------------------------------

	ProceduralTerrainCreationComponent* getProceduralTerrainCreationComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<ProceduralTerrainCreationComponent>(gameObject->getComponentWithOccurrence<ProceduralTerrainCreationComponent>(occurrenceIndex)).get();
	}

	ProceduralTerrainCreationComponent* getProceduralTerrainCreationComponent(GameObject* gameObject)
	{
		return makeStrongPtr<ProceduralTerrainCreationComponent>(gameObject->getComponent<ProceduralTerrainCreationComponent>()).get();
	}

	ProceduralTerrainCreationComponent* getProceduralTerrainCreationComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<ProceduralTerrainCreationComponent>(gameObject->getComponentFromName<ProceduralTerrainCreationComponent>(name)).get();
	}

	void ProceduralTerrainCreationComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<ProceduralTerrainCreationComponent, GameObjectComponent>("ProceduralTerrainCreationComponent")
			.def("setResolution", &ProceduralTerrainCreationComponent::setResolution)
			.def("getResolution", &ProceduralTerrainCreationComponent::getResolution)
			.def("setBaseHeight", &ProceduralTerrainCreationComponent::setBaseHeight)
			.def("getBaseHeight", &ProceduralTerrainCreationComponent::getBaseHeight)
			.def("setHillAmplitude", &ProceduralTerrainCreationComponent::setHillAmplitude)
			.def("getHillAmplitude", &ProceduralTerrainCreationComponent::getHillAmplitude)
			.def("setHillFrequency", &ProceduralTerrainCreationComponent::setHillFrequency)
			.def("getHillFrequency", &ProceduralTerrainCreationComponent::getHillFrequency)
			.def("setOctaves", &ProceduralTerrainCreationComponent::setOctaves)
			.def("getOctaves", &ProceduralTerrainCreationComponent::getOctaves)
			.def("setPersistence", &ProceduralTerrainCreationComponent::setPersistence)
			.def("getPersistence", &ProceduralTerrainCreationComponent::getPersistence)
			.def("setLacunarity", &ProceduralTerrainCreationComponent::setLacunarity)
			.def("getLacunarity", &ProceduralTerrainCreationComponent::getLacunarity)
			.def("setSeed", &ProceduralTerrainCreationComponent::setSeed)
			.def("getSeed", &ProceduralTerrainCreationComponent::getSeed)
			.def("setEnableRoads", &ProceduralTerrainCreationComponent::setEnableRoads)
			.def("getEnableRoads", &ProceduralTerrainCreationComponent::getEnableRoads)
			.def("setRoadCount", &ProceduralTerrainCreationComponent::setRoadCount)
			.def("getRoadCount", &ProceduralTerrainCreationComponent::getRoadCount)
			.def("setRoadWidth", &ProceduralTerrainCreationComponent::setRoadWidth)
			.def("getRoadWidth", &ProceduralTerrainCreationComponent::getRoadWidth)
			.def("setRoadDepth", &ProceduralTerrainCreationComponent::setRoadDepth)
			.def("getRoadDepth", &ProceduralTerrainCreationComponent::getRoadDepth)
			.def("setRoadSmoothness", &ProceduralTerrainCreationComponent::setRoadSmoothness)
			.def("getRoadSmoothness", &ProceduralTerrainCreationComponent::getRoadSmoothness)
			.def("setRoadCurviness", &ProceduralTerrainCreationComponent::setRoadCurviness)
			.def("getRoadCurviness", &ProceduralTerrainCreationComponent::getRoadCurviness)
			.def("generateTerrain", &ProceduralTerrainCreationComponent::generateTerrain)
			.def("setRoadsClosed", &ProceduralTerrainCreationComponent::setRoadsClosed)
			.def("getRoadsClosed", &ProceduralTerrainCreationComponent::getRoadsClosed)

			.def("setEnableRivers", &ProceduralTerrainCreationComponent::setEnableRivers)
			.def("getEnableRivers", &ProceduralTerrainCreationComponent::getEnableRivers)
			.def("setRiverCount", &ProceduralTerrainCreationComponent::setRiverCount)
			.def("getRiverCount", &ProceduralTerrainCreationComponent::getRiverCount)
			.def("setRiverWidth", &ProceduralTerrainCreationComponent::setRiverWidth)
			.def("getRiverWidth", &ProceduralTerrainCreationComponent::getRiverWidth)
			.def("setRiverDepth", &ProceduralTerrainCreationComponent::setRiverDepth)
			.def("getRiverDepth", &ProceduralTerrainCreationComponent::getRiverDepth)
			.def("setRiverMeandering", &ProceduralTerrainCreationComponent::setRiverMeandering)
			.def("getRiverMeandering", &ProceduralTerrainCreationComponent::getRiverMeandering)
			.def("setRiverFlowThreshold", &ProceduralTerrainCreationComponent::setRiverFlowThreshold)
			.def("getRiverFlowThreshold", &ProceduralTerrainCreationComponent::getRiverFlowThreshold)
			.def("setRiverWidthScale", &ProceduralTerrainCreationComponent::setRiverWidthScale)
			.def("getRiverWidthScale", &ProceduralTerrainCreationComponent::getRiverWidthScale)
			.def("setRiverErosionFactor", &ProceduralTerrainCreationComponent::setRiverErosionFactor)
			.def("getRiverErosionFactor", &ProceduralTerrainCreationComponent::getRiverErosionFactor)
			.def("setMaxRiverDepth", &ProceduralTerrainCreationComponent::setMaxRiverDepth)
			.def("getMaxRiverDepth", &ProceduralTerrainCreationComponent::getMaxRiverDepth)

			.def("setEnableCanyons", &ProceduralTerrainCreationComponent::setEnableCanyons)
			.def("getEnableCanyons", &ProceduralTerrainCreationComponent::getEnableCanyons)
			.def("setCanyonCount", &ProceduralTerrainCreationComponent::setCanyonCount)
			.def("getCanyonCount", &ProceduralTerrainCreationComponent::getCanyonCount)
			.def("setCanyonWidth", &ProceduralTerrainCreationComponent::setCanyonWidth)
			.def("getCanyonWidth", &ProceduralTerrainCreationComponent::getCanyonWidth)
			.def("setCanyonDepth", &ProceduralTerrainCreationComponent::setCanyonDepth)
			.def("getCanyonDepth", &ProceduralTerrainCreationComponent::getCanyonDepth)
			.def("setCanyonSteepness", &ProceduralTerrainCreationComponent::setCanyonSteepness)
			.def("getCanyonSteepness", &ProceduralTerrainCreationComponent::getCanyonSteepness)
			.def("setCanyonMinWidth", &ProceduralTerrainCreationComponent::setCanyonMinWidth)
			.def("getCanyonMinWidth", &ProceduralTerrainCreationComponent::getCanyonMinWidth)
			.def("setCanyonMaxWidth", &ProceduralTerrainCreationComponent::setCanyonMaxWidth)
			.def("getCanyonMaxWidth", &ProceduralTerrainCreationComponent::getCanyonMaxWidth)

			.def("setEnableIsland", &ProceduralTerrainCreationComponent::setEnableIsland)
			.def("getEnableIsland", &ProceduralTerrainCreationComponent::getEnableIsland)
			.def("setIslandFalloff", &ProceduralTerrainCreationComponent::setIslandFalloff)
			.def("getIslandFalloff", &ProceduralTerrainCreationComponent::getIslandFalloff)
			.def("setIslandSize", &ProceduralTerrainCreationComponent::setIslandSize)
			.def("getIslandSize", &ProceduralTerrainCreationComponent::getIslandSize)

			.def("setEnableErosion", &ProceduralTerrainCreationComponent::setEnableErosion)
			.def("getEnableErosion", &ProceduralTerrainCreationComponent::getEnableErosion)
			.def("setErosionIterations", &ProceduralTerrainCreationComponent::setErosionIterations)
			.def("getErosionIterations", &ProceduralTerrainCreationComponent::getErosionIterations)
			.def("setErosionStrength", &ProceduralTerrainCreationComponent::setErosionStrength)
			.def("getErosionStrength", &ProceduralTerrainCreationComponent::getErosionStrength)
			.def("setSedimentCapacity", &ProceduralTerrainCreationComponent::setSedimentCapacity)
			.def("getSedimentCapacity", &ProceduralTerrainCreationComponent::getSedimentCapacity)
		];

		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "class inherits GameObjectComponent", ProceduralTerrainCreationComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "void setResolution(uint resolution)", "Sets heightmap resolution.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "uint getResolution()", "Gets resolution.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "void setBaseHeight(float height)", "Sets base height.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "float getBaseHeight()", "Gets base height.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "void setHillAmplitude(float amplitude)", "Sets hill amplitude.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "float getHillAmplitude()", "Gets hill amplitude.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "void setHillFrequency(float frequency)", "Sets hill frequency.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "float getHillFrequency()", "Gets hill frequency.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "void generateTerrain()", "Manually triggers terrain generation.");

		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "void setRoadsClosed(bool roadsClosed)", "If true, generated roads tend to form loops rather than only point-to-point connections.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "bool getRoadsClosed()", "Returns whether roads are generated as loops.");

		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "void setEnableRivers(bool enableRivers)", "Enables drainage-based river carving after the base terrain is generated.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "bool getEnableRivers()", "Returns whether river carving is enabled.");

		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "void setRiverCount(uint riverCount)", "High-level river complexity hint. Together with RiverFlowThreshold it influences how many major rivers appear.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "uint getRiverCount()", "Gets the river complexity hint.");

		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "void setRiverWidth(float riverWidth)", "High-level width control for rivers. Scales river carving width (in heightmap pixels).");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "float getRiverWidth()", "Gets the high-level river width control.");

		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "void setRiverDepth(float riverDepth)", "High-level depth control for rivers. Scales the amount of erosion applied to river beds.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "float getRiverDepth()", "Gets the high-level river depth control.");

		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "void setRiverMeandering(float riverMeandering)", "Controls how much river paths can wander. 0 = strict steepest descent, 1 = more natural meandering.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "float getRiverMeandering()", "Gets the river meandering amount.");

		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "void setRiverFlowThreshold(float riverFlowThreshold)", "Minimum flow accumulation before carving starts. Higher = fewer, larger rivers.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "float getRiverFlowThreshold()", "Gets the minimum flow threshold for river carving.");

		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "void setRiverWidthScale(float riverWidthScale)", "Fine tuning: multiplier applied to river width derived from flow. Works together with RiverWidth.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "float getRiverWidthScale()", "Gets the fine-tuning river width scale.");

		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "void setRiverErosionFactor(float riverErosionFactor)", "Fine tuning: converts flow into erosion depth. Works together with RiverDepth.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "float getRiverErosionFactor()", "Gets the flow-to-erosion factor used for rivers.");

		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "void setMaxRiverDepth(float maxRiverDepth)", "Fine tuning: absolute clamp for how deep river beds may be carved.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "float getMaxRiverDepth()", "Gets the absolute maximum depth for river carving.");

		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "void setEnableCanyons(bool enableCanyons)", "Enables canyon carving. Canyons are long paths carved with a wall profile.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "bool getEnableCanyons()", "Returns whether canyon carving is enabled.");

		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "void setCanyonCount(uint canyonCount)", "Number of canyon paths to generate. 0 disables canyon generation even if enabled.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "uint getCanyonCount()", "Gets the number of canyons to generate.");

		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "void setCanyonWidth(float canyonWidth)", "High-level canyon width multiplier that scales the min/max canyon widths.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "float getCanyonWidth()", "Gets the high-level canyon width multiplier.");

		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "void setCanyonDepth(float canyonDepth)", "How deep canyons are carved at their center line.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "float getCanyonDepth()", "Gets the canyon depth.");

		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "void setCanyonSteepness(float canyonSteepness)", "Controls canyon wall profile. Low = more U-shaped, high = sharper V-shaped walls.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "float getCanyonSteepness()", "Gets the canyon wall steepness.");

		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "void setCanyonMinWidth(float canyonMinWidth)", "Minimum canyon width in heightmap pixels (before CanyonWidth scaling).");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "float getCanyonMinWidth()", "Gets the minimum canyon width.");

		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "void setCanyonMaxWidth(float canyonMaxWidth)", "Maximum canyon width in heightmap pixels (before CanyonWidth scaling). Should be >= CanyonMinWidth.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "float getCanyonMaxWidth()", "Gets the maximum canyon width.");

		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "void setEnableIsland(bool enableIsland)", "Enables island masking. Heights fall off toward the edges to create an island-like shape.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "bool getEnableIsland()", "Returns whether island masking is enabled.");

		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "void setIslandFalloff(float islandFalloff)", "Controls how fast the terrain falls off toward edges. Higher = sharper coast drop.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "float getIslandFalloff()", "Gets island falloff strength.");

		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "void setIslandSize(float islandSize)", "Controls island radius as fraction of the heightmap. Lower = smaller island, more ocean around.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "float getIslandSize()", "Gets island size factor.");

		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "void setEnableErosion(bool enableErosion)", "Enables hydraulic erosion simulation. Expensive at high iteration counts.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "bool getEnableErosion()", "Returns whether erosion simulation is enabled.");

		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "void setErosionIterations(uint erosionIterations)", "Number of erosion iterations/particles. Higher = smoother/more realistic, but slower.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "uint getErosionIterations()", "Gets number of erosion iterations.");

		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "void setErosionStrength(float erosionStrength)", "Overall erosion strength. Higher values carve more aggressively per iteration.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "float getErosionStrength()", "Gets erosion strength.");

		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "void setSedimentCapacity(float sedimentCapacity)", "How much sediment a droplet can carry before it starts depositing. Higher = more transport, less deposition.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "float getSedimentCapacity()", "Gets sediment capacity.");


		gameObjectClass.def("getProceduralTerrainCreationComponentFromName", &getProceduralTerrainCreationComponentFromName);
		gameObjectClass.def("getProceduralTerrainCreationComponent", (ProceduralTerrainCreationComponent * (*)(GameObject*)) & getProceduralTerrainCreationComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ProceduralTerrainCreationComponent getProceduralTerrainCreationComponent()", "Gets the component.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ProceduralTerrainCreationComponent getProceduralTerrainCreationComponentFromName(String name)", "Gets the component by name.");

		gameObjectControllerClass.def("castProceduralTerrainCreationComponent", &GameObjectController::cast<ProceduralTerrainCreationComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "ProceduralTerrainCreationComponent castProceduralTerrainCreationComponent(ProceduralTerrainCreationComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool ProceduralTerrainCreationComponent::canStaticAddComponent(GameObject* gameObject)
	{
		auto terraCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<TerraComponent>());
		if (nullptr != terraCompPtr)
		{
			return terraCompPtr->getTerra() != nullptr;
		}

		return false;
	}

}; //namespace end
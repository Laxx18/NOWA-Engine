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
		activated(new Variant(ProceduralTerrainCreationComponent::AttrActivated(), false, this->attributes)),
		resolution(new Variant(ProceduralTerrainCreationComponent::AttrResolution(), static_cast<Ogre::uint32>(1024), this->attributes)),
		baseHeight(new Variant(ProceduralTerrainCreationComponent::AttrBaseHeight(), 10.0f, this->attributes)),
		hillAmplitude(new Variant(ProceduralTerrainCreationComponent::AttrHillAmplitude(), 45.0f, this->attributes)),
		hillFrequency(new Variant(ProceduralTerrainCreationComponent::AttrHillFrequency(), 0.001f, this->attributes)),
		octaves(new Variant(ProceduralTerrainCreationComponent::AttrOctaves(), static_cast<Ogre::uint32>(4), this->attributes)),
		persistence(new Variant(ProceduralTerrainCreationComponent::AttrPersistence(), 0.5f, this->attributes)),
		lacunarity(new Variant(ProceduralTerrainCreationComponent::AttrLacunarity(), 2.0f, this->attributes)),
		seed(new Variant(ProceduralTerrainCreationComponent::AttrSeed(), static_cast<Ogre::uint32>(12345), this->attributes)),
		enableRoads(new Variant(ProceduralTerrainCreationComponent::AttrEnableRoads(), true, this->attributes)),
		roadCount(new Variant(ProceduralTerrainCreationComponent::AttrRoadCount(), static_cast<Ogre::uint32>(2), this->attributes)),
		roadWidth(new Variant(ProceduralTerrainCreationComponent::AttrRoadWidth(), 8.0f, this->attributes)),
		roadDepth(new Variant(ProceduralTerrainCreationComponent::AttrRoadDepth(), 2.0f, this->attributes)),
		roadSmoothness(new Variant(ProceduralTerrainCreationComponent::AttrRoadSmoothness(), 4.0f, this->attributes)),
		roadCurviness(new Variant(ProceduralTerrainCreationComponent::AttrRoadCurviness(), 0.3f, this->attributes)),
		terrainGenerated(false)
	{
		this->activated->setDescription("Set to true to generate/regenerate the terrain. Automatically sets back to false after generation.");
		this->resolution->setDescription("Heightmap resolution (width and height). Common values: 512, 1024, 2048.");
		this->baseHeight->setDescription("Base terrain height (minimum elevation) in world units.");
		this->hillAmplitude->setDescription("Maximum height variation from base. Total max height = baseHeight + hillAmplitude.");
		this->hillFrequency->setDescription("Controls terrain feature size. Lower = larger features (0.001-0.1 typical).");
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

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
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

		if (ProceduralTerrainCreationComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (ProceduralTerrainCreationComponent::AttrResolution() == attribute->getName())
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
	}

	void ProceduralTerrainCreationComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		GameObjectComponent::writeXML(propertiesXML, doc);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
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
		// Convert lower 4 bits of hash into 12 gradient directions
		int h = hash & 15;
		float u = h < 8 ? x : y;
		float v = h < 4 ? y : (h == 12 || h == 14 ? x : 0);
		return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
	}

	float ProceduralTerrainCreationComponent::noise(float x, float y, int seed)
	{
		// Offset by seed to get different noise patterns
		x += seed * 13.7f;
		y += seed * 7.13f;

		// Find unit grid cell containing point
		int X = static_cast<int>(std::floor(x)) & 255;
		int Y = static_cast<int>(std::floor(y)) & 255;

		// Get relative xy coordinates of point within cell
		x -= std::floor(x);
		y -= std::floor(y);

		// Compute fade curves for x and y
		float u = fade(x);
		float v = fade(y);

		// Hash coordinates of the 4 square corners
		int A = permutation[X] + Y;
		int AA = permutation[A];
		int AB = permutation[A + 1];
		int B = permutation[X + 1] + Y;
		int BA = permutation[B];
		int BB = permutation[B + 1];

		// Add blended results from 4 corners of square
		float res = lerp(v,
			lerp(u, grad(permutation[AA], x, y),
				grad(permutation[BA], x - 1, y)),
			lerp(u, grad(permutation[AB], x, y - 1),
				grad(permutation[BB], x - 1, y - 1)));

		// Return value in range [-1, 1]
		return res;
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
		float total = 0.0f;
		float frequency = this->hillFrequency->getReal();
		float amplitude = 1.0f;
		float maxValue = 0.0f;
		Ogre::uint32 octaveCount = this->octaves->getUInt();
		float persist = this->persistence->getReal();
		float lacun = this->lacunarity->getReal();
		int noiseSeed = static_cast<int>(this->seed->getUInt());

		for (Ogre::uint32 i = 0; i < octaveCount; ++i)
		{
			float sampleX = x * frequency;
			float sampleY = y * frequency;

			float noiseValue = noise(sampleX, sampleY, noiseSeed + i);

			total += noiseValue * amplitude;
			maxValue += amplitude;

			amplitude *= persist;
			frequency *= lacun;
		}

		// Normalize to 0-1 range, then scale to desired height range
		float normalized = (total / maxValue + 1.0f) * 0.5f; // Convert from [-1,1] to [0,1]
		return this->baseHeight->getReal() + normalized * this->hillAmplitude->getReal();
	}

	//------------------------------------------------------------------------------------------------------
	// Road generation implementation
	//------------------------------------------------------------------------------------------------------

	std::vector<ProceduralTerrainCreationComponent::RoadPoint> ProceduralTerrainCreationComponent::generateRoadPath(const Ogre::Vector2& start, const Ogre::Vector2& end, int numSegments)
	{
		std::vector<RoadPoint> roadPoints;

		// Create control points for a curved road
		std::vector<Ogre::Vector2> controlPoints;
		controlPoints.push_back(start);

		// Add waypoints for curves based on curviness parameter
		float curviness = this->roadCurviness->getReal();
		int numWaypoints = static_cast<int>(3 + curviness * 3); // 3-6 waypoints depending on curviness

		std::mt19937 rng(this->seed->getUInt());
		std::uniform_real_distribution<float> dist(-50.0f * curviness, 50.0f * curviness);

		for (int i = 1; i < numWaypoints; ++i)
		{
			float t = i / static_cast<float>(numWaypoints);
			Ogre::Vector2 basePoint = start + (end - start) * t;

			// Add random offset perpendicular to the line
			Ogre::Vector2 dir = (end - start);
			dir.normalise();
			Ogre::Vector2 perpendicular(-dir.y, dir.x);

			float offset = dist(rng);
			controlPoints.push_back(basePoint + perpendicular * offset);
		}

		controlPoints.push_back(end);

		// Generate points along Catmull-Rom spline
		for (int i = 0; i < numSegments; ++i)
		{
			float t = i / static_cast<float>(numSegments - 1);
			Ogre::Vector2 point = evaluateCatmullRom(controlPoints, t);

			RoadPoint rp;
			rp.position = point;
			rp.width = this->roadWidth->getReal();
			rp.targetHeight = 0.0f; // Will be set based on terrain
			roadPoints.push_back(rp);
		}

		return roadPoints;
	}

	Ogre::Vector2 ProceduralTerrainCreationComponent::evaluateCatmullRom(const std::vector<Ogre::Vector2>& points, float t)
	{
		if (points.size() < 4)
			return points.size() > 0 ? points[0] : Ogre::Vector2::ZERO;

		int numSegments = points.size() - 3;
		float scaledT = t * numSegments;
		int segment = std::min(static_cast<int>(scaledT), numSegments - 1);
		float localT = scaledT - segment;

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

	float ProceduralTerrainCreationComponent::applyRoadInfluence(
		float baseHeight,
		const Ogre::Vector2& position,
		const std::vector<std::vector<RoadPoint>>& roads)
	{
		float minDist = std::numeric_limits<float>::max();
		float roadHeight = 0.0f;

		for (const auto& road : roads)
		{
			for (const auto& rp : road)
			{
				float dist = (position - rp.position).length();
				if (dist < minDist)
				{
					minDist = dist;
					roadHeight = rp.targetHeight;
				}
			}
		}

		// Smooth falloff based on distance from road
		float roadWidthVal = this->roadWidth->getReal();
		float smoothness = this->roadSmoothness->getReal();
		float depthVal = this->roadDepth->getReal();

		if (minDist < roadWidthVal * smoothness)
		{
			float influence = 1.0f - (minDist / (roadWidthVal * smoothness));
			influence = std::pow(influence, 2.0f); // Smooth curve

			// Blend between terrain height and road height (depressed)
			return baseHeight * (1.0f - influence) + (baseHeight - depthVal) * influence;
		}

		return baseHeight;
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
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralTerrainCreationComponent] Cannot find Terra component for terrain generation!");
			return;
		}

		NOWA::GraphicsModule::RenderCommand renderCommand = [this, terra]()
		{
			Ogre::uint32 width = this->resolution->getUInt();
			Ogre::uint32 height = this->resolution->getUInt();

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[ProceduralTerrainCreationComponent] Generating terrain " + Ogre::StringConverter::toString(width) + "x" + Ogre::StringConverter::toString(height));

			// Generate roads first
			std::vector<std::vector<RoadPoint>> roads;

			if (this->enableRoads->getBool())
			{
				Ogre::uint32 numRoads = this->roadCount->getUInt();
				std::mt19937 rng(this->seed->getUInt() + 999); // Different seed for road placement
				std::uniform_real_distribution<float> distX(width * 0.1f, width * 0.9f);
				std::uniform_real_distribution<float> distY(height * 0.1f, height * 0.9f);

				for (Ogre::uint32 i = 0; i < numRoads; ++i)
				{
					Ogre::Vector2 start(distX(rng), distY(rng));
					Ogre::Vector2 end(distX(rng), distY(rng));
					roads.push_back(generateRoadPath(start, end, 100));
				}
			}

			// Calculate road heights based on underlying terrain
			for (auto& road : roads)
			{
				for (auto& point : road)
				{
					point.targetHeight = perlinNoise(point.position.x, point.position.y);
				}
			}

			// Generate the heightmap data
			std::vector<float> heightData(width * height);

			for (Ogre::uint32 y = 0; y < height; ++y)
			{
				for (Ogre::uint32 x = 0; x < width; ++x)
				{
					// Base terrain from Perlin noise
					float terrainHeight = perlinNoise(static_cast<float>(x), static_cast<float>(y));

					// Apply road influences
					if (this->enableRoads->getBool())
					{
						Ogre::Vector2 pos(static_cast<float>(x), static_cast<float>(y));
						terrainHeight = applyRoadInfluence(terrainHeight, pos, roads);
					}

					heightData[y * width + x] = terrainHeight;
				}
			}

			// Now apply to Terra using Ogre::Image2
			Ogre::Image2 image;
			image.createEmptyImage(width, height, 1, Ogre::TextureTypes::Type2D, Ogre::PFG_R16_UNORM, 1);

			Ogre::uint16* imageData = reinterpret_cast<Ogre::uint16*>(image.getData(0).data);

			float maxHeight = this->baseHeight->getReal() + this->hillAmplitude->getReal();

			for (Ogre::uint32 i = 0; i < width * height; ++i)
			{
				// Normalize to 0-65535 range for R16_UNORM
				float normalized = Ogre::Math::Clamp(heightData[i] / maxHeight, 0.0f, 1.0f);
				imageData[i] = static_cast<Ogre::uint16>(normalized * 65535.0f);
			}

			// Apply to Terra
			terra->createHeightmapTexture(image);

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[ProceduralTerrainCreationComponent] Terrain generation complete!");

			this->terrainGenerated = true;

			// Auto-deactivate after generation
			this->activated->setValue(false);
		};
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralTerrainCreationComponent::generateProceduralTerrain");
	}

	void ProceduralTerrainCreationComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
		if (true == activated)
		{
			this->generateProceduralTerrain();
		}
	}

	bool ProceduralTerrainCreationComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void ProceduralTerrainCreationComponent::setResolution(Ogre::uint32 resolution)
	{
		// Clamp to valid power-of-2 values
		// Maximum is 4096 due to Terra's shadow mapper buffer limitation
		// The shadow mapper allocates 4096u * 16u elements, and needs (height << 4u) elements
		// So maximum height = 4096
		if (resolution < 128) resolution = 128;
		if (resolution > 4096) resolution = 4096;
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
			.def("setActivated", &ProceduralTerrainCreationComponent::setActivated)
			.def("isActivated", &ProceduralTerrainCreationComponent::isActivated)
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
		];

		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "class inherits GameObjectComponent", ProceduralTerrainCreationComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "void setActivated(bool activated)", "Sets whether to generate terrain.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "bool isActivated()", "Gets whether activated.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "void setResolution(uint resolution)", "Sets heightmap resolution.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "uint getResolution()", "Gets resolution.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "void setBaseHeight(float height)", "Sets base height.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "float getBaseHeight()", "Gets base height.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "void setHillAmplitude(float amplitude)", "Sets hill amplitude.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "float getHillAmplitude()", "Gets hill amplitude.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "void setHillFrequency(float frequency)", "Sets hill frequency.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "float getHillFrequency()", "Gets hill frequency.");
		LuaScriptApi::getInstance()->addClassToCollection("ProceduralTerrainCreationComponent", "void generateTerrain()", "Manually triggers terrain generation.");

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
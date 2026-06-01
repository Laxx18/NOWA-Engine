/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#include "NOWAPrecompiled.h"
#include "ProceduralPlanetComponent.h"
#include "gameobject/GameObjectFactory.h"
#include "main/AppStateManager.h"
#include "modules/LuaScriptApi.h"
#include "utilities/XMLConverter.h"

#include "OgreAbiUtils.h"
#include "OgreLogManager.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <random>

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    // =========================================================================
    // Permutation table (Ken Perlin's reference table, same as ProceduralTerrainCreationComponent)
    // =========================================================================

    const int ProceduralPlanetComponent::permutation[256] = {151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225, 140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23, 190, 6, 148, 247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219,
        203, 117, 35, 11, 32, 57, 177, 33, 88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175, 74, 165, 71, 134, 139, 48, 27, 166, 77, 146, 158, 231, 83, 111, 229, 122, 60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244, 102, 143, 54,
        65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169, 200, 196, 135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64, 52, 217, 226, 250, 124, 123, 5, 202, 38, 147, 118, 126, 255, 82, 85, 212, 207, 206, 59, 227, 47,
        16, 58, 17, 182, 189, 28, 42, 223, 183, 170, 213, 119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9, 129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104, 218, 246, 97, 228, 251, 34, 242, 193, 238,
        210, 144, 12, 191, 179, 162, 241, 81, 51, 145, 235, 249, 14, 239, 107, 49, 192, 214, 31, 181, 199, 106, 157, 184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254, 138, 236, 205, 93, 222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215,
        61, 156, 180};

    // =========================================================================
    // Constructor / Destructor
    // =========================================================================

    ProceduralPlanetComponent::ProceduralPlanetComponent() :
        GameObjectComponent(),
        name("ProceduralPlanetComponent"),
        hillAmplitude(new Variant(ProceduralPlanetComponent::AttrHillAmplitude(), 12.0f, this->attributes)),
        hillFrequency(new Variant(ProceduralPlanetComponent::AttrHillFrequency(), 2.0f, this->attributes)),
        octaves(new Variant(ProceduralPlanetComponent::AttrOctaves(), static_cast<Ogre::uint32>(6), this->attributes)),
        persistence(new Variant(ProceduralPlanetComponent::AttrPersistence(), 0.5f, this->attributes)),
        lacunarity(new Variant(ProceduralPlanetComponent::AttrLacunarity(), 2.0f, this->attributes)),
        seed(new Variant(ProceduralPlanetComponent::AttrSeed(), static_cast<Ogre::uint32>(12345), this->attributes)),
        enableIsland(new Variant(ProceduralPlanetComponent::AttrEnableIsland(), false, this->attributes)),
        islandFalloff(new Variant(ProceduralPlanetComponent::AttrIslandFalloff(), 2.0f, this->attributes)),
        islandSize(new Variant(ProceduralPlanetComponent::AttrIslandSize(), 0.7f, this->attributes)),
        enableRivers(new Variant(ProceduralPlanetComponent::AttrEnableRivers(), false, this->attributes)),
        riverCount(new Variant(ProceduralPlanetComponent::AttrRiverCount(), static_cast<Ogre::uint32>(3), this->attributes)),
        riverWidth(new Variant(ProceduralPlanetComponent::AttrRiverWidth(), 3.0f, this->attributes)),
        riverDepth(new Variant(ProceduralPlanetComponent::AttrRiverDepth(), 1.5f, this->attributes)),
        riverMeandering(new Variant(ProceduralPlanetComponent::AttrRiverMeandering(), 0.5f, this->attributes)),
        enableCanyons(new Variant(ProceduralPlanetComponent::AttrEnableCanyons(), false, this->attributes)),
        canyonCount(new Variant(ProceduralPlanetComponent::AttrCanyonCount(), static_cast<Ogre::uint32>(1), this->attributes)),
        canyonWidth(new Variant(ProceduralPlanetComponent::AttrCanyonWidth(), 8.0f, this->attributes)),
        canyonDepth(new Variant(ProceduralPlanetComponent::AttrCanyonDepth(), 3.0f, this->attributes)),
        canyonSteepness(new Variant(ProceduralPlanetComponent::AttrCanyonSteepness(), 0.7f, this->attributes)),
        enableRoads(new Variant(ProceduralPlanetComponent::AttrEnableRoads(), false, this->attributes)),
        roadCount(new Variant(ProceduralPlanetComponent::AttrRoadCount(), static_cast<Ogre::uint32>(2), this->attributes)),
        roadWidth(new Variant(ProceduralPlanetComponent::AttrRoadWidth(), 10.0f, this->attributes)),
        roadDepth(new Variant(ProceduralPlanetComponent::AttrRoadDepth(), 2.0f, this->attributes)),
        roadSmoothness(new Variant(ProceduralPlanetComponent::AttrRoadSmoothness(), 2.0f, this->attributes)),
        roadCurviness(new Variant(ProceduralPlanetComponent::AttrRoadCurviness(), 0.3f, this->attributes)),
        generate(new Variant(ProceduralPlanetComponent::AttrGenerate(), Ogre::String("Generate"), this->attributes)),
        layerSandThresh(new Variant(ProceduralPlanetComponent::AttrLayerSandThresh(), 0.2f, this->attributes)),
        layerGrassThresh(new Variant(ProceduralPlanetComponent::AttrLayerGrassThresh(), 0.5f, this->attributes)),
        layerRockThresh(new Variant(ProceduralPlanetComponent::AttrLayerRockThresh(), 0.75f, this->attributes)),
        layerSlopeRock(new Variant(ProceduralPlanetComponent::AttrLayerSlopeRock(), 0.45f, this->attributes)),
        currentRadius(50.0f),
        currentRadiusScale(1.0f)
    {
        this->hillAmplitude->setDescription("Hill height variation in world units (metres). "
                                            "This is the maximum displacement above/below the base sphere surface. "
                                            "30 = mountains 30 metres high.");
        this->hillFrequency->setDescription("Number of major terrain features across the sphere. "
                                            "2 = few large continents, 6 = many smaller highlands.");
        this->octaves->setDescription("FBM octave count. More octaves = more fine detail. 4-6 recommended.");
        this->persistence->setDescription("Amplitude decay per octave. Lower = smoother surface.");
        this->lacunarity->setDescription("Frequency multiplier per octave. 2.0 is standard.");
        this->seed->setDescription("Random seed. Change to generate a completely different planet.");
        this->enableIsland->setDescription("Applies a radial falloff mask so terrain drops toward the equator/poles, "
                                           "creating a concentrated landmass surrounded by lowlands.");
        this->islandFalloff->setDescription("How steeply terrain drops toward the island edge. "
                                            "Higher = sharper coastlines.");
        this->islandSize->setDescription("Fraction of the sphere covered by the raised landmass (0..1).");
        this->enableRivers->setDescription("Carves river channels using flow-accumulation from high to low areas.");
        this->riverCount->setDescription("Number of major river sources to trace.");
        this->riverWidth->setDescription("River channel width in world units.");
        this->riverDepth->setDescription("Maximum river carve depth in world units.");
        this->riverMeandering->setDescription("Meandering factor. 0 = steepest descent, 1 = wandering path.");
        this->enableCanyons->setDescription("Carves long canyon trenches across the surface.");
        this->canyonCount->setDescription("Number of canyon paths to generate.");
        this->canyonWidth->setDescription("Canyon width in world units.");
        this->canyonDepth->setDescription("Canyon carve depth in world units.");
        this->canyonSteepness->setDescription("Canyon wall profile. Low = U-shaped, high = sharp V walls.");
        this->enableRoads->setDescription("Flattens Catmull-Rom paths across the surface to simulate roads or riverbeds.");
        this->roadCount->setDescription("Number of road paths.");
        this->roadWidth->setDescription("Road width in world units.");
        this->roadDepth->setDescription("How deeply roads cut below the surrounding terrain (world units).");
        this->roadSmoothness->setDescription("Blend zone width as a multiplier of road half-width (1.5..4.0).");
        this->roadCurviness->setDescription("Road curvature. 0 = straight, 1 = very curvy.");

        this->hillAmplitude->setConstraints(1.0f, 500.0f);
        this->hillFrequency->setConstraints(1.0f, 16.0f);
        this->octaves->setConstraints(1u, 8u);
        this->persistence->setConstraints(0.1f, 0.9f);
        this->lacunarity->setConstraints(1.0f, 4.0f);
        this->islandFalloff->setConstraints(0.5f, 10.0f);
        this->islandSize->setConstraints(0.1f, 1.0f);
        this->riverCount->setConstraints(1u, 20u);
        this->riverWidth->setConstraints(1.0f, 200.0f);
        this->riverDepth->setConstraints(1.0f, 200.0f);
        this->riverMeandering->setConstraints(0.0f, 1.0f);
        this->canyonCount->setConstraints(0u, 10u);
        this->canyonWidth->setConstraints(5.0f, 500.0f);
        this->canyonDepth->setConstraints(5.0f, 500.0f);
        this->canyonSteepness->setConstraints(0.1f, 5.0f);
        this->roadCount->setConstraints(1u, 10u);
        this->roadWidth->setConstraints(2.0f, 100.0f);
        this->roadDepth->setConstraints(0.0f, 50.0f);
        this->roadSmoothness->setConstraints(1.5f, 4.0f);
        this->roadCurviness->setConstraints(0.0f, 1.0f);

        this->generate->setDescription("Runs the full generation pipeline and applies the result to the "
                                       "sibling PlanetTerraComponent. The planet must already exist.");
        this->generate->addUserData(GameObject::AttrActionExec());
        this->generate->addUserData(GameObject::AttrActionNeedRefresh());
        this->generate->addUserData(GameObject::AttrActionExecId(), ProceduralPlanetComponent::ActionGenerate());

        this->layerSandThresh->setDescription("Relative height fraction [0..1] below which sand (layer 2) is painted. "
                                              "0.2 = lowest 20% of terrain relief gets sand.");
        this->layerGrassThresh->setDescription("Relative height fraction [0..1] below which grass (layer 1) is painted.");
        this->layerRockThresh->setDescription("Relative height fraction [0..1] above which rock (layer 3) is painted on peaks. "
                                              "0.75 = top 25% of terrain relief gets rock.");
        this->layerSlopeRock->setDescription("Slope steepness [0..1] above which rock is forced. "
                                             "Slope is normalized by total terrain relief: 0.45 means a slope of "
                                             "45%% of the full terrain range per pixel triggers rock.");
        this->layerSandThresh->setConstraints(0.0f, 1.0f);
        this->layerGrassThresh->setConstraints(0.0f, 1.0f);
        this->layerRockThresh->setConstraints(0.0f, 1.0f);
        this->layerSlopeRock->setConstraints(0.0f, 1.0f);
    }

    ProceduralPlanetComponent::~ProceduralPlanetComponent()
    {
    }

    // =========================================================================
    // Ogre::Plugin
    // =========================================================================

    void ProceduralPlanetComponent::install(const Ogre::NameValuePairList* options)
    {
        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<ProceduralPlanetComponent>(ProceduralPlanetComponent::getStaticClassId(), ProceduralPlanetComponent::getStaticClassName());
    }

    void ProceduralPlanetComponent::initialise()
    {
    }

    void ProceduralPlanetComponent::shutdown()
    {
    }

    void ProceduralPlanetComponent::uninstall()
    {
    }

    void ProceduralPlanetComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    const Ogre::String& ProceduralPlanetComponent::getName() const
    {
        return this->name;
    }

    Ogre::String ProceduralPlanetComponent::getClassName() const
    {
        return "ProceduralPlanetComponent";
    }

    Ogre::String ProceduralPlanetComponent::getParentClassName() const
    {
        return "GameObjectComponent";
    }

    // =========================================================================
    // init
    // =========================================================================

    bool ProceduralPlanetComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        bool success = GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrHillAmplitude())
        {
            this->hillAmplitude->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrHillFrequency())
        {
            this->hillFrequency->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrOctaves())
        {
            this->octaves->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrPersistence())
        {
            this->persistence->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrLacunarity())
        {
            this->lacunarity->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrSeed())
        {
            this->seed->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrEnableIsland())
        {
            this->enableIsland->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrIslandFalloff())
        {
            this->islandFalloff->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrIslandSize())
        {
            this->islandSize->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrEnableRivers())
        {
            this->enableRivers->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrRiverCount())
        {
            this->riverCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrRiverWidth())
        {
            this->riverWidth->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrRiverDepth())
        {
            this->riverDepth->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrRiverMeandering())
        {
            this->riverMeandering->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrEnableCanyons())
        {
            this->enableCanyons->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrCanyonCount())
        {
            this->canyonCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrCanyonWidth())
        {
            this->canyonWidth->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrCanyonDepth())
        {
            this->canyonDepth->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrCanyonSteepness())
        {
            this->canyonSteepness->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrEnableRoads())
        {
            this->enableRoads->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrRoadCount())
        {
            this->roadCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrRoadWidth())
        {
            this->roadWidth->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrRoadDepth())
        {
            this->roadDepth->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrRoadSmoothness())
        {
            this->roadSmoothness->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrRoadCurviness())
        {
            this->roadCurviness->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrLayerSandThresh())
        {
            this->layerSandThresh->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrLayerGrassThresh())
        {
            this->layerGrassThresh->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrLayerRockThresh())
        {
            this->layerRockThresh->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrLayerSlopeRock())
        {
            this->layerSlopeRock->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        return success;
    }

    // =========================================================================
    // postInit / connect / disconnect / lifecycle
    // =========================================================================

    bool ProceduralPlanetComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPlanetComponent] Init for game object: " + this->gameObjectPtr->getName());
        return true;
    }

    bool ProceduralPlanetComponent::connect(void)
    {
        GameObjectComponent::connect();
        return true;
    }

    bool ProceduralPlanetComponent::disconnect(void)
    {
        GameObjectComponent::disconnect();
        return true;
    }

    bool ProceduralPlanetComponent::onCloned(void)
    {
        return true;
    }

    void ProceduralPlanetComponent::onAddComponent(void)
    {
    }

    void ProceduralPlanetComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();
    }

    void ProceduralPlanetComponent::onOtherComponentRemoved(unsigned int index)
    {
    }

    void ProceduralPlanetComponent::onOtherComponentAdded(unsigned int index)
    {
    }

    GameObjectCompPtr ProceduralPlanetComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        return nullptr;
    }

    void ProceduralPlanetComponent::update(Ogre::Real dt, bool notSimulating)
    {
    }

    bool ProceduralPlanetComponent::canStaticAddComponent(GameObject* gameObject)
    {
        auto thisCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<ProceduralPlanetComponent>());
        auto dependantCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<PlanetTerraComponentBase>());
        if (nullptr != dependantCompPtr && nullptr == thisCompPtr)
        {
            return true;
        }
        return false;
    }

    std::optional<NOWA::GameObjectTypeDescriptor> ProceduralPlanetComponent::getStaticTypeDescriptor()
    {
        return std::nullopt;
    }

    // =========================================================================
    // writeXML
    // =========================================================================

    void ProceduralPlanetComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        GameObjectComponent::writeXML(propertiesXML, doc);

        xml_node<>* propertyXML = nullptr;

        auto writeReal = [&](const Ogre::String& attrName, Ogre::Real value)
        {
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, attrName)));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, value)));
            propertiesXML->append_node(propertyXML);
        };

        auto writeUInt = [&](const Ogre::String& attrName, Ogre::uint32 value)
        {
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, attrName)));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, value)));
            propertiesXML->append_node(propertyXML);
        };

        auto writeBool = [&](const Ogre::String& attrName, bool value)
        {
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, attrName)));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, value)));
            propertiesXML->append_node(propertyXML);
        };

        writeReal(AttrHillAmplitude(), this->hillAmplitude->getReal());
        writeReal(AttrHillFrequency(), this->hillFrequency->getReal());
        writeUInt(AttrOctaves(), this->octaves->getUInt());
        writeReal(AttrPersistence(), this->persistence->getReal());
        writeReal(AttrLacunarity(), this->lacunarity->getReal());
        writeUInt(AttrSeed(), this->seed->getUInt());
        writeBool(AttrEnableIsland(), this->enableIsland->getBool());
        writeReal(AttrIslandFalloff(), this->islandFalloff->getReal());
        writeReal(AttrIslandSize(), this->islandSize->getReal());
        writeBool(AttrEnableRivers(), this->enableRivers->getBool());
        writeUInt(AttrRiverCount(), this->riverCount->getUInt());
        writeReal(AttrRiverWidth(), this->riverWidth->getReal());
        writeReal(AttrRiverDepth(), this->riverDepth->getReal());
        writeReal(AttrRiverMeandering(), this->riverMeandering->getReal());
        writeBool(AttrEnableCanyons(), this->enableCanyons->getBool());
        writeUInt(AttrCanyonCount(), this->canyonCount->getUInt());
        writeReal(AttrCanyonWidth(), this->canyonWidth->getReal());
        writeReal(AttrCanyonDepth(), this->canyonDepth->getReal());
        writeReal(AttrCanyonSteepness(), this->canyonSteepness->getReal());
        writeBool(AttrEnableRoads(), this->enableRoads->getBool());
        writeUInt(AttrRoadCount(), this->roadCount->getUInt());
        writeReal(AttrRoadWidth(), this->roadWidth->getReal());
        writeReal(AttrRoadDepth(), this->roadDepth->getReal());
        writeReal(AttrRoadSmoothness(), this->roadSmoothness->getReal());
        writeReal(AttrRoadCurviness(), this->roadCurviness->getReal());
        writeReal(AttrLayerSandThresh(), this->layerSandThresh->getReal());
        writeReal(AttrLayerGrassThresh(), this->layerGrassThresh->getReal());
        writeReal(AttrLayerRockThresh(), this->layerRockThresh->getReal());
        writeReal(AttrLayerSlopeRock(), this->layerSlopeRock->getReal());
    }

    // =========================================================================
    // actualizeValue
    // =========================================================================

    void ProceduralPlanetComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (AttrHillAmplitude() == attribute->getName())
        {
            this->setHillAmplitude(attribute->getReal());
        }
        else if (AttrHillFrequency() == attribute->getName())
        {
            this->setHillFrequency(attribute->getReal());
        }
        else if (AttrOctaves() == attribute->getName())
        {
            this->setOctaves(attribute->getUInt());
        }
        else if (AttrPersistence() == attribute->getName())
        {
            this->setPersistence(attribute->getReal());
        }
        else if (AttrLacunarity() == attribute->getName())
        {
            this->setLacunarity(attribute->getReal());
        }
        else if (AttrSeed() == attribute->getName())
        {
            this->setSeed(attribute->getUInt());
        }
        else if (AttrEnableIsland() == attribute->getName())
        {
            this->setEnableIsland(attribute->getBool());
        }
        else if (AttrIslandFalloff() == attribute->getName())
        {
            this->setIslandFalloff(attribute->getReal());
        }
        else if (AttrIslandSize() == attribute->getName())
        {
            this->setIslandSize(attribute->getReal());
        }
        else if (AttrEnableRivers() == attribute->getName())
        {
            this->setEnableRivers(attribute->getBool());
        }
        else if (AttrRiverCount() == attribute->getName())
        {
            this->setRiverCount(attribute->getUInt());
        }
        else if (AttrRiverWidth() == attribute->getName())
        {
            this->setRiverWidth(attribute->getReal());
        }
        else if (AttrRiverDepth() == attribute->getName())
        {
            this->setRiverDepth(attribute->getReal());
        }
        else if (AttrRiverMeandering() == attribute->getName())
        {
            this->setRiverMeandering(attribute->getReal());
        }
        else if (AttrEnableCanyons() == attribute->getName())
        {
            this->setEnableCanyons(attribute->getBool());
        }
        else if (AttrCanyonCount() == attribute->getName())
        {
            this->setCanyonCount(attribute->getUInt());
        }
        else if (AttrCanyonWidth() == attribute->getName())
        {
            this->setCanyonWidth(attribute->getReal());
        }
        else if (AttrCanyonDepth() == attribute->getName())
        {
            this->setCanyonDepth(attribute->getReal());
        }
        else if (AttrCanyonSteepness() == attribute->getName())
        {
            this->setCanyonSteepness(attribute->getReal());
        }
        else if (AttrEnableRoads() == attribute->getName())
        {
            this->setEnableRoads(attribute->getBool());
        }
        else if (AttrRoadCount() == attribute->getName())
        {
            this->setRoadCount(attribute->getUInt());
        }
        else if (AttrRoadWidth() == attribute->getName())
        {
            this->setRoadWidth(attribute->getReal());
        }
        else if (AttrRoadDepth() == attribute->getName())
        {
            this->setRoadDepth(attribute->getReal());
        }
        else if (AttrRoadSmoothness() == attribute->getName())
        {
            this->setRoadSmoothness(attribute->getReal());
        }
        else if (AttrRoadCurviness() == attribute->getName())
        {
            this->setRoadCurviness(attribute->getReal());
        }
        else if (AttrLayerSandThresh() == attribute->getName())
        {
            this->setLayerSandThresh(attribute->getReal());
        }
        else if (AttrLayerGrassThresh() == attribute->getName())
        {
            this->setLayerGrassThresh(attribute->getReal());
        }
        else if (AttrLayerRockThresh() == attribute->getName())
        {
            this->setLayerRockThresh(attribute->getReal());
        }
        else if (AttrLayerSlopeRock() == attribute->getName())
        {
            this->setLayerSopeRock(attribute->getReal());
        }
    }

    bool ProceduralPlanetComponent::executeAction(const Ogre::String& actionId, NOWA::Variant* attribute)
    {
        if (ActionGenerate() == actionId)
        {
            this->generateProceduralPlanet();
            return true;
        }
        return false;
    }

    // =========================================================================
    // Noise core (identical math to ProceduralTerrainCreationComponent)
    // =========================================================================

    float ProceduralPlanetComponent::fade(float t) const
    {
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    float ProceduralPlanetComponent::lerpF(float t, float a, float b) const
    {
        return a + t * (b - a);
    }

    float ProceduralPlanetComponent::grad(int hash, float x, float y) const
    {
        int h = hash & 7;
        float u = (h < 4) ? x : y;
        float v = (h < 4) ? y : x;
        return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
    }

    float ProceduralPlanetComponent::noise2D(float x, float y, int noiseSeed) const
    {
        int xi = static_cast<int>(std::floor(x));
        int yi = static_cast<int>(std::floor(y));
        int X = xi & 255;
        int Y = yi & 255;
        float xf = x - static_cast<float>(xi);
        float yf = y - static_cast<float>(yi);
        float u = this->fade(xf);
        float v = this->fade(yf);
        int s = noiseSeed & 255;
        int A = permutation[(X + s) & 255] + Y;
        int B = permutation[(X + 1 + s) & 255] + Y;
        int aa = permutation[A & 255];
        int ab = permutation[(A + 1) & 255];
        int ba = permutation[B & 255];
        int bb = permutation[(B + 1) & 255];
        float x1 = lerpF(u, this->grad(aa, xf, yf), this->grad(ba, xf - 1.0f, yf));
        float x2 = lerpF(u, this->grad(ab, xf, yf - 1.0f), this->grad(bb, xf - 1.0f, yf - 1.0f));
        return lerpF(v, x1, x2);
    }

    // 3D gradient noise - seamless on a sphere (no UV seam, no pole pinch).
    float ProceduralPlanetComponent::noise3D(float x, float y, float z, int noiseSeed) const
    {
        int xi = static_cast<int>(std::floor(x));
        int yi = static_cast<int>(std::floor(y));
        int zi = static_cast<int>(std::floor(z));
        int X = xi & 255;
        int Y = yi & 255;
        int Z = zi & 255;
        float xf = x - static_cast<float>(xi);
        float yf = y - static_cast<float>(yi);
        float zf = z - static_cast<float>(zi);
        float u = this->fade(xf);
        float v = this->fade(yf);
        float w = this->fade(zf);
        int s = noiseSeed & 255;
        auto g3 = [](int hash, float px, float py, float pz) -> float
        {
            int h = hash & 15;
            float ua = (h < 8) ? px : py;
            float ub = (h < 4) ? py : ((h == 12 || h == 14) ? px : pz);
            return ((h & 1) ? -ua : ua) + ((h & 2) ? -ub : ub);
        };
        int A = permutation[(X + s) & 255] + Y;
        int B = permutation[(X + 1 + s) & 255] + Y;
        int AA = permutation[A & 255] + Z;
        int AB = permutation[(A + 1) & 255] + Z;
        int BA = permutation[B & 255] + Z;
        int BB = permutation[(B + 1) & 255] + Z;
        float y1 = lerpF(v, lerpF(u, g3(permutation[AA & 255], xf, yf, zf), g3(permutation[BA & 255], xf - 1.f, yf, zf)), lerpF(u, g3(permutation[AB & 255], xf, yf - 1.f, zf), g3(permutation[BB & 255], xf - 1.f, yf - 1.f, zf)));
        float y2 = lerpF(v, lerpF(u, g3(permutation[(AA + 1) & 255], xf, yf, zf - 1.f), g3(permutation[(BA + 1) & 255], xf - 1.f, yf, zf - 1.f)),
            lerpF(u, g3(permutation[(AB + 1) & 255], xf, yf - 1.f, zf - 1.f), g3(permutation[(BB + 1) & 255], xf - 1.f, yf - 1.f, zf - 1.f)));
        return lerpF(w, y1, y2);
    }

    // perlinFBM: UV -> 3D sphere direction -> 3D noise. Completely seamless.
    float ProceduralPlanetComponent::perlinFBM(float u, float v) const
    {
        float phi = v * Ogre::Math::PI;
        float theta = u * Ogre::Math::TWO_PI;
        float sinPhi = std::sin(phi);
        float dx = sinPhi * std::cos(theta);
        float dy = std::cos(phi);
        float dz = sinPhi * std::sin(theta);
        float total = 0.0f;
        float maxValue = 0.0f;
        float amplitude = 1.0f;
        float frequency = this->hillFrequency->getReal();
        int noiseSeed = static_cast<int>(this->seed->getUInt());
        Ogre::uint32 octaveCount = this->octaves->getUInt();
        float persist = this->persistence->getReal();
        float lacun = this->lacunarity->getReal();
        for (Ogre::uint32 i = 0; i < octaveCount; ++i)
        {
            total += this->noise3D(dx * frequency, dy * frequency, dz * frequency, noiseSeed + static_cast<int>(i)) * amplitude;
            maxValue += amplitude;
            amplitude *= persist;
            frequency *= lacun;
        }
        return (total / maxValue + 1.0f) * 0.5f;
    }

    // =========================================================================
    // Catmull-Rom evaluation (identical to ProceduralTerrainCreationComponent)
    // =========================================================================

    Ogre::Vector2 ProceduralPlanetComponent::evaluateCatmullRom(const std::vector<Ogre::Vector2>& points, float t) const
    {
        if (points.size() < 4)
        {
            if (points.empty())
            {
                return Ogre::Vector2::ZERO;
            }
            return points[0];
        }

        int numSegments = static_cast<int>(points.size()) - 3;
        float scaledT = t * static_cast<float>(numSegments);
        int segment = Ogre::Math::Clamp(static_cast<int>(scaledT), 0, numSegments - 1);
        float localT = scaledT - static_cast<float>(segment);

        segment = std::min(segment, static_cast<int>(points.size()) - 4);

        Ogre::Vector2 p0 = points[segment];
        Ogre::Vector2 p1 = points[segment + 1];
        Ogre::Vector2 p2 = points[segment + 2];
        Ogre::Vector2 p3 = points[segment + 3];

        float t2 = localT * localT;
        float t3 = t2 * localT;

        return 0.5f * ((2.0f * p1) + (-p0 + p2) * localT + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 + (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
    }

    // =========================================================================
    // Nearest-vertex lookup in UV space
    // =========================================================================

    int ProceduralPlanetComponent::findNearestVertex(const Ogre::Vector2& uv, const std::vector<Ogre::Vector2>& uvCoords, int vertexCount) const
    {
        int best = 0;
        float bestDist = std::numeric_limits<float>::max();

        for (int i = 0; i < vertexCount; ++i)
        {
            float ddx = uvCoords[i].x - uv.x;
            if (ddx > 0.5f)
            {
                ddx -= 1.0f;
            }
            if (ddx < -0.5f)
            {
                ddx += 1.0f;
            }
            float ddy = uvCoords[i].y - uv.y;
            float d = ddx * ddx + ddy * ddy;
            if (d < bestDist)
            {
                bestDist = d;
                best = i;
            }
        }

        return best;
    }

    // =========================================================================
    // findPlanetTerraBase
    // =========================================================================

    PlanetTerraComponentBase* ProceduralPlanetComponent::findPlanetTerraBase(void) const
    {
        auto ptr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PlanetTerraComponentBase>());
        if (nullptr == ptr)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPlanetComponent] No PlanetTerraComponentBase found on '" + this->gameObjectPtr->getName() + "'. Add a PlanetTerraComponent first.");
            return nullptr;
        }
        return ptr.get();
    }

    // =========================================================================
    // Island mask
    // =========================================================================

    void ProceduralPlanetComponent::applyIslandMask(std::vector<float>& heights, int vertexCount, const std::vector<Ogre::Vector2>& uvCoords, float radius) const
    {
        // Island mask in UV space: full land at UV centre (0.5, 0.5), ocean at edges.
        const Ogre::Vector2 center(0.5f, 0.5f);
        float falloff = this->islandFalloff->getReal();
        float size = this->islandSize->getReal();
        float maxDist = 0.5f * size;

        for (int i = 0; i < vertexCount; ++i)
        {
            float dist = (uvCoords[i] - center).length();
            float normalizedDist = dist / (maxDist + 1e-6f);
            float mask = 1.0f - std::pow(Ogre::Math::Clamp(normalizedDist, 0.0f, 1.0f), falloff);

            // Coastal noise for organic-looking coastlines
            float coastNoise = (this->noise2D(uvCoords[i].x * 12.0f, uvCoords[i].y * 12.0f, static_cast<int>(this->seed->getUInt()) + 77) + 1.0f) * 0.5f;
            mask = Ogre::Math::Clamp(mask + coastNoise * 0.08f, 0.0f, 1.0f);

            // Ocean areas sink below zero
            if (mask < 0.3f)
            {
                float oceanDrop = -radius * 0.05f;
                heights[i] = lerpF(mask / 0.3f, oceanDrop, heights[i]);
            }
            else
            {
                heights[i] *= mask;
            }
        }
    }

    // =========================================================================
    // Canyon generation and carving
    // =========================================================================

    void ProceduralPlanetComponent::generateCanyons(std::vector<CanyonPath>& canyons, std::mt19937& rng) const
    {
        std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
        std::uniform_real_distribution<float> angle(-1.0f, 1.0f);

        int numCanyons = static_cast<int>(this->canyonCount->getUInt());
        canyons.clear();
        canyons.reserve(numCanyons);

        for (int i = 0; i < numCanyons; ++i)
        {
            CanyonPath canyon;

            // Start from a random edge of the UV square
            int edge = static_cast<int>(dist01(rng) * 4.0f);
            Ogre::Vector2 startUV;
            Ogre::Vector2 direction;

            if (edge == 0)
            {
                startUV = Ogre::Vector2(dist01(rng), 0.05f);
                direction = Ogre::Vector2(angle(rng) * 0.3f, 1.0f);
            }
            else if (edge == 1)
            {
                startUV = Ogre::Vector2(0.95f, dist01(rng));
                direction = Ogre::Vector2(-1.0f, angle(rng) * 0.3f);
            }
            else if (edge == 2)
            {
                startUV = Ogre::Vector2(dist01(rng), 0.95f);
                direction = Ogre::Vector2(angle(rng) * 0.3f, -1.0f);
            }
            else
            {
                startUV = Ogre::Vector2(0.05f, dist01(rng));
                direction = Ogre::Vector2(1.0f, angle(rng) * 0.3f);
            }

            direction.normalise();

            // Width and depth in world units converted to UV fraction later during carving
            float halfWidth = this->canyonWidth->getReal() * 0.5f;
            float depth = this->canyonDepth->getReal();

            Ogre::Vector2 currentUV = startUV;
            float angleChange = 0.0f;
            float stepSize = 0.005f; // UV step per iteration
            int maxSteps = static_cast<int>(1.4f / stepSize);

            for (int step = 0; step < maxSteps; ++step)
            {
                if (currentUV.x < 0.0f || currentUV.x > 1.0f || currentUV.y < 0.0f || currentUV.y > 1.0f)
                {
                    break;
                }

                canyon.uvPoints.push_back(currentUV);
                canyon.widths.push_back(halfWidth);
                canyon.depths.push_back(depth * (0.8f + dist01(rng) * 0.4f));

                // Gentle meandering
                angleChange += (dist01(rng) - 0.5f) * 0.2f;
                angleChange *= 0.9f;

                float ang = std::atan2(direction.y, direction.x) + angleChange;
                direction = Ogre::Vector2(std::cos(ang), std::sin(ang));

                currentUV += direction * stepSize;
            }

            if (canyon.uvPoints.size() > 10)
            {
                canyons.push_back(canyon);
            }
        }
    }

    void ProceduralPlanetComponent::applyCanyons(std::vector<float>& heights, const std::vector<CanyonPath>& canyons, int vertexCount, const std::vector<Ogre::Vector2>& uvCoords) const
    {
        float steepness = Ogre::Math::Clamp(this->canyonSteepness->getReal(), 0.1f, 5.0f);

        for (const auto& canyon : canyons)
        {
            for (size_t seg = 0; seg < canyon.uvPoints.size(); ++seg)
            {
                const Ogre::Vector2& cUV = canyon.uvPoints[seg];
                // width in UV = world_units / circumference; depth clamped to terrain amplitude
                float halfWidthUV = canyon.widths[seg] * this->currentRadiusScale / (2.0f * Ogre::Math::PI * this->currentRadius);
                float maxDepth = std::fabs(this->hillAmplitude->getReal() * this->currentRadiusScale) * 1.5f;
                float depth = Ogre::Math::Clamp(canyon.depths[seg] * this->currentRadiusScale, 0.0f, maxDepth);
                float halfWidthSq = halfWidthUV * halfWidthUV;

                for (int v = 0; v < vertexCount; ++v)
                {
                    float dx = uvCoords[v].x - cUV.x;
                    if (dx > 0.5f)
                    {
                        dx -= 1.0f;
                    }
                    if (dx < -0.5f)
                    {
                        dx += 1.0f;
                    }
                    float dy = uvCoords[v].y - cUV.y;
                    float distSq = dx * dx + dy * dy;

                    if (distSq >= halfWidthSq)
                    {
                        continue;
                    }

                    float normalizedDist = std::sqrt(distSq) / (halfWidthUV + 1e-6f);
                    float profile = 1.0f - std::pow(normalizedDist, steepness);
                    profile = std::max(0.0f, profile);

                    float erosion = depth * profile;
                    float targetH = heights[v] - erosion;

                    // Smooth blend
                    heights[v] = heights[v] * 0.3f + targetH * 0.7f;
                }
            }
        }

        // Two-pass smoothing to soften hard edges between vertices
        std::vector<float> smoothed = heights;
        for (int pass = 0; pass < 2; ++pass)
        {
            for (int v = 0; v < vertexCount; ++v)
            {
                // Simple average with UV neighbours (cheap approximation for sparse vertex sets)
                float sum = heights[v] * 4.0f;
                int count = 4;

                // Look at a small local neighbourhood by UV proximity
                const Ogre::Vector2& uv = uvCoords[v];
                for (int nb = 0; nb < vertexCount; ++nb)
                {
                    if (nb == v)
                    {
                        continue;
                    }
                    float d = uv.squaredDistance(uvCoords[nb]);
                    if (d < 0.0001f)
                    {
                        sum += heights[nb];
                        ++count;
                    }
                }

                smoothed[v] = sum / static_cast<float>(count);
            }
            heights = smoothed;
        }
    }

    // =========================================================================
    // River tracing and carving
    // =========================================================================

    std::vector<ProceduralPlanetComponent::RiverPath> ProceduralPlanetComponent::traceRivers(const std::vector<float>& heights, int vertexCount, const std::vector<Ogre::Vector2>& uvCoords, std::mt19937& rng) const
    {
        std::vector<RiverPath> rivers;
        std::uniform_real_distribution<float> dist01(0.0f, 1.0f);

        int numRivers = static_cast<int>(this->riverCount->getUInt());
        float meandering = this->riverMeandering->getReal();

        // Find high-altitude starting vertices to trace rivers downhill from
        // Sort vertices by height descending and pick the top ones spread across the UV space
        std::vector<std::pair<float, int>> heightIdx;
        heightIdx.reserve(vertexCount);
        for (int i = 0; i < vertexCount; ++i)
        {
            heightIdx.push_back(std::make_pair(heights[i], i));
        }
        std::sort(heightIdx.begin(), heightIdx.end(),
            [](const std::pair<float, int>& a, const std::pair<float, int>& b)
            {
                return a.first > b.first;
            });

        // Pick starting points distributed across different areas
        std::vector<int> startVertices;
        float minSpacingSq = 0.04f; // minimum UV spacing between river sources

        for (int candidate = 0; candidate < vertexCount && static_cast<int>(startVertices.size()) < numRivers; ++candidate)
        {
            int vi = heightIdx[candidate].second;
            bool tooClose = false;

            for (int sv : startVertices)
            {
                if (uvCoords[vi].squaredDistance(uvCoords[sv]) < minSpacingSq)
                {
                    tooClose = true;
                    break;
                }
            }

            if (!tooClose)
            {
                startVertices.push_back(vi);
            }
        }

        for (int startV : startVertices)
        {
            RiverPath river;
            river.uvPoints.push_back(uvCoords[startV]);
            river.flow.push_back(1.0f);

            int currentV = startV;
            int maxSteps = 500;
            float minDescend = 0.01f; // world units minimum drop per step to continue

            for (int step = 0; step < maxSteps; ++step)
            {
                const Ogre::Vector2& currentUV = uvCoords[currentV];

                // Find the lowest neighbour within a small UV radius
                float searchRadius = 0.03f;
                int bestNeighbour = -1;
                float bestHeight = heights[currentV] - minDescend;

                // Add some meandering jitter so rivers are not perfectly straight
                float jitterAngle = (dist01(rng) - 0.5f) * meandering * Ogre::Math::PI * 0.5f;

                for (int nb = 0; nb < vertexCount; ++nb)
                {
                    if (nb == currentV)
                    {
                        continue;
                    }

                    float dSq = currentUV.squaredDistance(uvCoords[nb]);
                    if (dSq > searchRadius * searchRadius)
                    {
                        continue;
                    }

                    // Apply a small random bias based on meandering
                    float h = heights[nb] + (dist01(rng) - 0.5f) * meandering * 0.5f;

                    if (h < bestHeight)
                    {
                        bestHeight = h;
                        bestNeighbour = nb;
                    }
                }

                if (bestNeighbour < 0)
                {
                    // No downhill neighbour found - river ends here
                    break;
                }

                float flowValue = river.flow.back() + 0.02f;
                river.uvPoints.push_back(uvCoords[bestNeighbour]);
                river.flow.push_back(flowValue);
                currentV = bestNeighbour;
            }

            if (river.uvPoints.size() > 5)
            {
                rivers.push_back(river);
            }
        }

        return rivers;
    }

    void ProceduralPlanetComponent::carveRivers(std::vector<float>& heights, const std::vector<RiverPath>& rivers, int vertexCount, const std::vector<Ogre::Vector2>& uvCoords) const
    {
        float maxDepth = this->riverDepth->getReal();
        float halfWidthUV = this->riverWidth->getReal() * this->currentRadiusScale / (2.0f * Ogre::Math::PI * this->currentRadius);
        float riverMaxDepth = std::fabs(this->hillAmplitude->getReal() * this->currentRadiusScale) * 1.5f;

        for (const auto& river : rivers)
        {
            for (size_t seg = 0; seg < river.uvPoints.size(); ++seg)
            {
                const Ogre::Vector2& rUV = river.uvPoints[seg];
                float flow = river.flow[seg];
                float widthScale = std::min(1.0f + std::log(flow + 1.0f) * 0.5f, 3.0f);
                float halfW = halfWidthUV * widthScale;
                float halfWSq = halfW * halfW;
                float depth = std::min(maxDepth * (0.3f + flow * 0.1f), maxDepth);

                for (int v = 0; v < vertexCount; ++v)
                {
                    float dx = uvCoords[v].x - rUV.x;
                    if (dx > 0.5f)
                    {
                        dx -= 1.0f;
                    }
                    if (dx < -0.5f)
                    {
                        dx += 1.0f;
                    }
                    float dy = uvCoords[v].y - rUV.y;
                    float distSq = dx * dx + dy * dy;

                    if (distSq >= halfWSq)
                    {
                        continue;
                    }

                    float nd = std::sqrt(distSq) / (halfW + 1e-6f);
                    float profile = 1.0f - nd * nd; // parabolic channel
                    profile = std::max(0.0f, profile);

                    float cut = depth * profile;
                    if (cut > 0.01f)
                    {
                        heights[v] -= cut;
                    }
                }
            }
        }
    }

    // =========================================================================
    // Road generation and flattening
    // =========================================================================

    std::vector<ProceduralPlanetComponent::RoadPoint> ProceduralPlanetComponent::generateRoadPath(const Ogre::Vector2& uvStart, const Ogre::Vector2& uvEnd, const std::vector<float>& heights, int vertexCount,
        const std::vector<Ogre::Vector2>& uvCoords, std::mt19937& rng) const
    {
        std::vector<RoadPoint> roadPoints;
        float curviness = this->roadCurviness->getReal();

        // Build control points with random lateral offsets (same strategy as ProceduralTerrainCreationComponent)
        std::vector<Ogre::Vector2> controlPoints;
        controlPoints.push_back(uvStart);

        int numWaypoints = 3 + static_cast<int>(curviness * 3);
        Ogre::Vector2 dir = uvEnd - uvStart;
        float roadLength = dir.length();
        dir.normalise();
        Ogre::Vector2 perp(-dir.y, dir.x);

        float maxOffset = roadLength * 0.15f * curviness;
        std::uniform_real_distribution<float> offsetDist(-maxOffset, maxOffset);

        for (int w = 1; w < numWaypoints; ++w)
        {
            float t = static_cast<float>(w) / static_cast<float>(numWaypoints);
            Ogre::Vector2 basePoint = uvStart + (uvEnd - uvStart) * t;
            controlPoints.push_back(basePoint + perp * offsetDist(rng));
        }

        controlPoints.push_back(uvEnd);

        // Ghost points for Catmull-Rom
        Ogre::Vector2 preStart = controlPoints[0] + (controlPoints[0] - controlPoints[1]);
        Ogre::Vector2 postEnd = controlPoints.back() + (controlPoints.back() - controlPoints[controlPoints.size() - 2]);
        controlPoints.insert(controlPoints.begin(), preStart);
        controlPoints.push_back(postEnd);

        int numSamples = 200;
        for (int s = 0; s < numSamples; ++s)
        {
            float t = static_cast<float>(s) / static_cast<float>(numSamples - 1);
            Ogre::Vector2 uv = this->evaluateCatmullRom(controlPoints, t);

            uv.x = Ogre::Math::Clamp(uv.x, 0.0f, 1.0f);
            uv.y = Ogre::Math::Clamp(uv.y, 0.0f, 1.0f);

            int nearest = this->findNearestVertex(uv, uvCoords, vertexCount);

            RoadPoint rp;
            rp.uv = uv;
            rp.targetHeight = heights[nearest];
            roadPoints.push_back(rp);
        }

        // Average heights along the road to create a graded surface
        float avgH = 0.0f;
        for (const auto& rp : roadPoints)
        {
            avgH += rp.targetHeight;
        }
        avgH /= static_cast<float>(roadPoints.size());

        // Apply the depth cut to the grade
        float depthCut = this->roadDepth->getReal();
        for (auto& rp : roadPoints)
        {
            rp.targetHeight = avgH - depthCut;
        }

        return roadPoints;
    }

    void ProceduralPlanetComponent::applyRoads(std::vector<float>& heights, const std::vector<std::vector<RoadPoint>>& roads, int vertexCount, const std::vector<Ogre::Vector2>& uvCoords) const
    {
        float halfWidthUV = this->roadWidth->getReal() * this->currentRadiusScale / (2.0f * Ogre::Math::PI * this->currentRadius);
        float smoothFactor = this->roadSmoothness->getReal();
        float blendWidthUV = halfWidthUV * smoothFactor;

        // Build influence and target maps
        std::vector<float> influenceMap(vertexCount, 0.0f);
        std::vector<float> targetMap(vertexCount, 0.0f);

        for (const auto& road : roads)
        {
            for (const auto& rp : road)
            {
                for (int v = 0; v < vertexCount; ++v)
                {
                    float dx = uvCoords[v].x - rp.uv.x;
                    if (dx > 0.5f)
                    {
                        dx -= 1.0f;
                    }
                    if (dx < -0.5f)
                    {
                        dx += 1.0f;
                    }
                    float dy = uvCoords[v].y - rp.uv.y;
                    float dist = std::sqrt(dx * dx + dy * dy);

                    if (dist > blendWidthUV)
                    {
                        continue;
                    }

                    float inf = 0.0f;
                    if (dist <= halfWidthUV)
                    {
                        inf = 1.0f;
                    }
                    else
                    {
                        float t = (dist - halfWidthUV) / (blendWidthUV - halfWidthUV + 1e-6f);
                        inf = 0.5f + 0.5f * std::cos(t * Ogre::Math::PI);
                    }

                    if (inf > influenceMap[v])
                    {
                        influenceMap[v] = inf;
                        targetMap[v] = rp.targetHeight;
                    }
                }
            }
        }

        // Blend terrain toward road grade
        for (int v = 0; v < vertexCount; ++v)
        {
            if (influenceMap[v] > 0.001f)
            {
                heights[v] = heights[v] + (targetMap[v] - heights[v]) * influenceMap[v];
            }
        }
    }

    // =========================================================================
    // Main generation entry point
    // =========================================================================

    void ProceduralPlanetComponent::generateProceduralPlanet(void)
    {
        PlanetTerraComponentBase* planetBase = this->findPlanetTerraBase();
        if (nullptr == planetBase)
        {
            return;
        }

        // Retrieve the current planet data blob to find out vertex count and radius.
        // Format: uint32 vertexCount, uint32 blendDataSize, float[vc] heights, uint8[bsz] blend.
        std::vector<unsigned char> currentData = planetBase->getPlanetData();
        if (currentData.size() < 8)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPlanetComponent] Planet data empty. Generate PlanetTerraComponent first.");
            return;
        }

        uint32_t vertexCount = 0;
        uint32_t blendSize = 0;
        memcpy(&vertexCount, &currentData[0], 4);
        memcpy(&blendSize, &currentData[4], 4);

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPlanetComponent] Generating planet surface for '" + this->gameObjectPtr->getName() + "' verts=" + Ogre::StringConverter::toString(vertexCount));

        int vc = static_cast<int>(vertexCount);
        float radius = planetBase->getRadius();
        this->currentRadius = radius;
        this->currentRadiusScale = radius / 50.0f;

        // Retrieve UV coordinates from the planet (needed for UV-space feature generation).
        const std::vector<Ogre::Vector2>& uvCoords = planetBase->getUvCoords();
        if (static_cast<int>(uvCoords.size()) < vc)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPlanetComponent] UV coord count mismatch. Cannot generate.");
            return;
        }

        // ---- Step 1: Base Perlin FBM noise ----
        // Heights are in world units (metres above/below the sphere surface).
        // Scale amplitude by radius so the same value gives same visual relief at any radius.
        // amplitude=5 -> +-5m on r=50, +-50m on r=500. Same 10% relief both times.
        std::vector<float> heights(vc, 0.0f);
        float amplitude = this->hillAmplitude->getReal() * this->currentRadiusScale;

        for (int i = 0; i < vc; ++i)
        {
            float n = this->perlinFBM(uvCoords[i].x, uvCoords[i].y);
            heights[i] = (n - 0.5f) * 2.0f * amplitude;
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPlanetComponent] Base noise generated.");

        // ---- Step 2: Island mask ----
        if (this->enableIsland->getBool())
        {
            this->applyIslandMask(heights, vc, uvCoords, radius);
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPlanetComponent] Island mask applied.");
        }

        std::mt19937 rng(this->seed->getUInt());

        // ---- Step 3: Canyon carving ----
        if (this->enableCanyons->getBool())
        {
            std::vector<CanyonPath> canyons;
            this->generateCanyons(canyons, rng);
            this->applyCanyons(heights, canyons, vc, uvCoords);
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPlanetComponent] " + Ogre::StringConverter::toString(static_cast<Ogre::uint32>(canyons.size())) + " canyon(s) carved.");
        }

        // ---- Step 4: River carving ----
        if (this->enableRivers->getBool())
        {
            std::vector<RiverPath> rivers = this->traceRivers(heights, vc, uvCoords, rng);
            this->carveRivers(heights, rivers, vc, uvCoords);
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPlanetComponent] " + Ogre::StringConverter::toString(static_cast<Ogre::uint32>(rivers.size())) + " river(s) carved.");
        }

        // ---- Step 5: Road flattening ----
        if (this->enableRoads->getBool())
        {
            std::vector<std::vector<RoadPoint>> roads;
            int numRoads = static_cast<int>(this->roadCount->getUInt());

            std::uniform_real_distribution<float> dist01(0.0f, 1.0f);
            for (int r = 0; r < numRoads; ++r)
            {
                // Start from one side of the UV space, end at the other
                int startEdge = r % 4;
                int endEdge = (startEdge + 2) % 4;

                auto pickEdgeUV = [&](int edge) -> Ogre::Vector2
                {
                    float margin = 0.1f;
                    float pos = margin + dist01(rng) * (1.0f - 2.0f * margin);
                    if (edge == 0)
                    {
                        return Ogre::Vector2(pos, margin);
                    }
                    else if (edge == 1)
                    {
                        return Ogre::Vector2(1.0f - margin, pos);
                    }
                    else if (edge == 2)
                    {
                        return Ogre::Vector2(pos, 1.0f - margin);
                    }
                    else
                    {
                        return Ogre::Vector2(margin, pos);
                    }
                };

                Ogre::Vector2 uvStart = pickEdgeUV(startEdge);
                Ogre::Vector2 uvEnd = pickEdgeUV(endEdge);
                roads.push_back(this->generateRoadPath(uvStart, uvEnd, heights, vc, uvCoords, rng));
            }

            this->applyRoads(heights, roads, vc, uvCoords);
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPlanetComponent] " + Ogre::StringConverter::toString(numRoads) + " road(s) flattened.");
        }

        // ---- Step 6: Seam sync ----
        // UV sphere has duplicate vertex columns at u=0 and u=1 (same world position).
        // Copy u<0.01 heights to matching u>0.99 vertices.
        for (int i = 0; i < vc; ++i)
        {
            if (uvCoords[i].x < 0.01f)
            {
                for (int j = 0; j < vc; ++j)
                {
                    if (uvCoords[j].x > 0.99f && std::fabs(uvCoords[i].y - uvCoords[j].y) < 0.001f)
                    {
                        heights[j] = heights[i];
                    }
                }
            }
        }

        // ---- Step 7: Pack blob ----
        std::vector<unsigned char> newData(currentData.size());
        memcpy(&newData[0], &vertexCount, 4);
        memcpy(&newData[4], &blendSize, 4);
        for (int i = 0; i < vc; ++i)
        {
            float h = heights[i];
            memcpy(&newData[8 + i * sizeof(float)], &h, sizeof(float));
        }

        // ---- Step 8: Auto-paint blend layers ----
        size_t blendOffset = 8 + static_cast<size_t>(vc) * sizeof(float);
        if (newData.size() >= blendOffset + blendSize && blendSize > 0)
        {
            this->autoRPaintLayers(heights, uvCoords, vc, radius, amplitude, blendSize, &newData[blendOffset]);
        }

        planetBase->setPlanetData(newData);

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPlanetComponent] Generation complete. " + Ogre::StringConverter::toString(vc) + " vertices updated.");

        // Notify navmesh/other systems that geometry changed
        boost::shared_ptr<NOWA::EventDataGeometryModified> evtGeo(new NOWA::EventDataGeometryModified());
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evtGeo);
    }

    // =========================================================================
    // Attribute setters / getters
    // =========================================================================

    void ProceduralPlanetComponent::setHillAmplitude(Ogre::Real v)
    {
        if (v < 1.0f)
        {
            v = 1.0f;
        }
        this->hillAmplitude->setValue(v);
    }
    Ogre::Real ProceduralPlanetComponent::getHillAmplitude(void) const
    {
        return this->hillAmplitude->getReal();
    }

    void ProceduralPlanetComponent::setHillFrequency(Ogre::Real v)
    {
        if (v < 0.1f)
        {
            v = 0.1f;
        }
        this->hillFrequency->setValue(v);
    }
    Ogre::Real ProceduralPlanetComponent::getHillFrequency(void) const
    {
        return this->hillFrequency->getReal();
    }

    void ProceduralPlanetComponent::setOctaves(Ogre::uint32 v)
    {
        if (v < 1)
        {
            v = 1;
        }
        if (v > 8)
        {
            v = 8;
        }
        this->octaves->setValue(v);
    }
    Ogre::uint32 ProceduralPlanetComponent::getOctaves(void) const
    {
        return this->octaves->getUInt();
    }

    void ProceduralPlanetComponent::setPersistence(Ogre::Real v)
    {
        if (v < 0.0f)
        {
            v = 0.0f;
        }
        if (v > 1.0f)
        {
            v = 1.0f;
        }
        this->persistence->setValue(v);
    }
    Ogre::Real ProceduralPlanetComponent::getPersistence(void) const
    {
        return this->persistence->getReal();
    }

    void ProceduralPlanetComponent::setLacunarity(Ogre::Real v)
    {
        if (v < 1.0f)
        {
            v = 1.0f;
        }
        this->lacunarity->setValue(v);
    }
    Ogre::Real ProceduralPlanetComponent::getLacunarity(void) const
    {
        return this->lacunarity->getReal();
    }

    void ProceduralPlanetComponent::setSeed(Ogre::uint32 v)
    {
        this->seed->setValue(v);
    }
    Ogre::uint32 ProceduralPlanetComponent::getSeed(void) const
    {
        return this->seed->getUInt();
    }

    void ProceduralPlanetComponent::setEnableIsland(bool v)
    {
        this->enableIsland->setValue(v);
    }
    bool ProceduralPlanetComponent::getEnableIsland(void) const
    {
        return this->enableIsland->getBool();
    }

    void ProceduralPlanetComponent::setIslandFalloff(Ogre::Real v)
    {
        this->islandFalloff->setValue(v);
    }
    Ogre::Real ProceduralPlanetComponent::getIslandFalloff(void) const
    {
        return this->islandFalloff->getReal();
    }

    void ProceduralPlanetComponent::setIslandSize(Ogre::Real v)
    {
        this->islandSize->setValue(v);
    }
    Ogre::Real ProceduralPlanetComponent::getIslandSize(void) const
    {
        return this->islandSize->getReal();
    }

    void ProceduralPlanetComponent::setEnableRivers(bool v)
    {
        this->enableRivers->setValue(v);
    }
    bool ProceduralPlanetComponent::getEnableRivers(void) const
    {
        return this->enableRivers->getBool();
    }

    void ProceduralPlanetComponent::setRiverCount(Ogre::uint32 v)
    {
        this->riverCount->setValue(v);
    }
    Ogre::uint32 ProceduralPlanetComponent::getRiverCount(void) const
    {
        return this->riverCount->getUInt();
    }

    void ProceduralPlanetComponent::setRiverWidth(Ogre::Real v)
    {
        this->riverWidth->setValue(v);
    }
    Ogre::Real ProceduralPlanetComponent::getRiverWidth(void) const
    {
        return this->riverWidth->getReal();
    }

    void ProceduralPlanetComponent::setRiverDepth(Ogre::Real v)
    {
        this->riverDepth->setValue(v);
    }
    Ogre::Real ProceduralPlanetComponent::getRiverDepth(void) const
    {
        return this->riverDepth->getReal();
    }

    void ProceduralPlanetComponent::setRiverMeandering(Ogre::Real v)
    {
        this->riverMeandering->setValue(v);
    }
    Ogre::Real ProceduralPlanetComponent::getRiverMeandering(void) const
    {
        return this->riverMeandering->getReal();
    }

    void ProceduralPlanetComponent::setEnableCanyons(bool v)
    {
        this->enableCanyons->setValue(v);
    }
    bool ProceduralPlanetComponent::getEnableCanyons(void) const
    {
        return this->enableCanyons->getBool();
    }

    void ProceduralPlanetComponent::setCanyonCount(Ogre::uint32 v)
    {
        this->canyonCount->setValue(v);
    }
    Ogre::uint32 ProceduralPlanetComponent::getCanyonCount(void) const
    {
        return this->canyonCount->getUInt();
    }

    void ProceduralPlanetComponent::setCanyonWidth(Ogre::Real v)
    {
        this->canyonWidth->setValue(v);
    }
    Ogre::Real ProceduralPlanetComponent::getCanyonWidth(void) const
    {
        return this->canyonWidth->getReal();
    }

    void ProceduralPlanetComponent::setCanyonDepth(Ogre::Real v)
    {
        this->canyonDepth->setValue(v);
    }
    Ogre::Real ProceduralPlanetComponent::getCanyonDepth(void) const
    {
        return this->canyonDepth->getReal();
    }

    void ProceduralPlanetComponent::setCanyonSteepness(Ogre::Real v)
    {
        this->canyonSteepness->setValue(v);
    }
    Ogre::Real ProceduralPlanetComponent::getCanyonSteepness(void) const
    {
        return this->canyonSteepness->getReal();
    }

    void ProceduralPlanetComponent::setEnableRoads(bool v)
    {
        this->enableRoads->setValue(v);
    }
    bool ProceduralPlanetComponent::getEnableRoads(void) const
    {
        return this->enableRoads->getBool();
    }

    void ProceduralPlanetComponent::setRoadCount(Ogre::uint32 v)
    {
        this->roadCount->setValue(v);
    }
    Ogre::uint32 ProceduralPlanetComponent::getRoadCount(void) const
    {
        return this->roadCount->getUInt();
    }

    void ProceduralPlanetComponent::setRoadWidth(Ogre::Real v)
    {
        this->roadWidth->setValue(v);
    }
    Ogre::Real ProceduralPlanetComponent::getRoadWidth(void) const
    {
        return this->roadWidth->getReal();
    }

    void ProceduralPlanetComponent::setRoadDepth(Ogre::Real v)
    {
        this->roadDepth->setValue(v);
    }
    Ogre::Real ProceduralPlanetComponent::getRoadDepth(void) const
    {
        return this->roadDepth->getReal();
    }

    void ProceduralPlanetComponent::setRoadSmoothness(Ogre::Real v)
    {
        this->roadSmoothness->setValue(v);
    }
    Ogre::Real ProceduralPlanetComponent::getRoadSmoothness(void) const
    {
        return this->roadSmoothness->getReal();
    }

    void ProceduralPlanetComponent::setRoadCurviness(Ogre::Real v)
    {
        this->roadCurviness->setValue(v);
    }
    Ogre::Real ProceduralPlanetComponent::getRoadCurviness(void) const
    {
        return this->roadCurviness->getReal();
    }

    void ProceduralPlanetComponent::setLayerSandThresh(Ogre::Real v)
    {
        this->layerSandThresh->setValue(Ogre::Math::Clamp(v, 0.0f, 1.0f));
    }
    Ogre::Real ProceduralPlanetComponent::getLayerSandThresh(void) const
    {
        return this->layerSandThresh->getReal();
    }

    void ProceduralPlanetComponent::setLayerGrassThresh(Ogre::Real v)
    {
        this->layerGrassThresh->setValue(Ogre::Math::Clamp(v, 0.0f, 1.0f));
    }
    Ogre::Real ProceduralPlanetComponent::getLayerGrassThresh(void) const
    {
        return this->layerGrassThresh->getReal();
    }

    void ProceduralPlanetComponent::setLayerRockThresh(Ogre::Real v)
    {
        this->layerRockThresh->setValue(Ogre::Math::Clamp(v, 0.0f, 1.0f));
    }
    Ogre::Real ProceduralPlanetComponent::getLayerRockThresh(void) const
    {
        return this->layerRockThresh->getReal();
    }

    void ProceduralPlanetComponent::setLayerSopeRock(Ogre::Real v)
    {
        this->layerSlopeRock->setValue(Ogre::Math::Clamp(v, 0.0f, 1.0f));
    }
    Ogre::Real ProceduralPlanetComponent::getLayerSlopeRock(void) const
    {
        return this->layerSlopeRock->getReal();
    }

    // =========================================================================
    // autoRPaintLayers
    //
    // Bilinear interpolation over the regular UV vertex grid -- no rectangular
    // Voronoi artifacts from nearest-neighbor lookup.
    //
    // Slope fix: slopeN = (height_diff_per_pixel) / range
    // where range = total terrain relief in world units.
    // This makes slopeRock=0.45 mean "a height change equal to 45% of the full
    // terrain range per blend-pixel triggers rock" -- independent of radius or
    // amplitude so the threshold works the same on any planet.
    // =========================================================================

#if 0
    void ProceduralPlanetComponent::autoRPaintLayers(const std::vector<float>& heights, const std::vector<Ogre::Vector2>& uvCoords, int vertexCount, float radius, float amplitude, uint32_t blendSize, unsigned char* blendOut) const
    {
        int bts = static_cast<int>(std::round(std::sqrt(static_cast<float>(blendSize) / 4.0f)));
        if (bts < 1 || vertexCount < 4)
        {
            return;
        }

        // Detect segH by counting the first row (all vertices with same v=0).
        int segH = 0;
        while (segH + 1 < vertexCount && std::fabs(uvCoords[segH + 1].y - uvCoords[0].y) < 1e-5f)
        {
            ++segH;
        }
        if (segH < 1)
        {
            return;
        }
        int colsPerRow = segH + 1;
        int segV = (vertexCount / colsPerRow) - 1;
        if (segV < 1)
        {
            return;
        }

        float minH = std::numeric_limits<float>::max();
        float maxH = -std::numeric_limits<float>::max();
        for (int i = 0; i < vertexCount; ++i)
        {
            if (heights[i] < minH)
            {
                minH = heights[i];
            }
            if (heights[i] > maxH)
            {
                maxH = heights[i];
            }
        }
        float range = maxH - minH;
        if (range < 1e-6f)
        {
            range = 1.0f;
        }

        float sandThresh = this->layerSandThresh->getReal();
        float grassThresh = this->layerGrassThresh->getReal();
        float rockThresh = this->layerRockThresh->getReal();
        float slopeCut = this->layerSlopeRock->getReal();

        const float invBts = 1.0f / static_cast<float>(bts);

        for (int py = 0; py < bts; ++py)
        {
            for (int px = 0; px < bts; ++px)
            {
                float u = (static_cast<float>(px) + 0.5f) * invBts;
                float v = (static_cast<float>(py) + 0.5f) * invBts;

                // Bilinear sample helper (inline lambda).
                auto bilerp = [&](float su, float sv) -> float
                {
                    float cf = Ogre::Math::Clamp(su, 0.0f, 1.0f) * static_cast<float>(segH);
                    float rf = Ogre::Math::Clamp(sv, 0.0f, 1.0f) * static_cast<float>(segV);
                    int c0 = static_cast<int>(cf);
                    int c1 = std::min(c0 + 1, segH);
                    int r0 = static_cast<int>(rf);
                    int r1 = std::min(r0 + 1, segV);
                    float fc = cf - static_cast<float>(c0);
                    float fr = rf - static_cast<float>(r0);
                    int i00 = r0 * colsPerRow + c0;
                    int i10 = r0 * colsPerRow + c1;
                    int i01 = r1 * colsPerRow + c0;
                    int i11 = r1 * colsPerRow + c1;
                    return lerpF(fr, lerpF(fc, heights[i00], heights[i10]), lerpF(fc, heights[i01], heights[i11]));
                };

                float h = bilerp(u, v);
                float hr = bilerp(u + invBts, v);
                float hu = bilerp(u, v + invBts);

                float hn = (h - minH) / range;

                // Slope = height change per pixel, normalised by total relief.
                // This is independent of radius and amplitude.
                float slope = std::sqrt((hr - h) * (hr - h) + (hu - h) * (hu - h));
                float slopeN = Ogre::Math::Clamp(slope / range, 0.0f, 1.0f);

                // Layer weights:
                // R = layer0 (dirt)   -- mid elevations, base fill
                // G = layer1 (grass)  -- low-mid, gentle slope
                // B = layer2 (sand)   -- lowest elevations
                // A = layer3 (rock)   -- high altitude OR steep slope
                float wRock = 0.0f;
                float wSand = 0.0f;
                float wGrass = 0.0f;

                if (slopeN >= slopeCut || hn >= rockThresh)
                {
                    float sb = Ogre::Math::Clamp((slopeN - slopeCut) / 0.2f, 0.0f, 1.0f);
                    float hb = Ogre::Math::Clamp((hn - rockThresh) / 0.15f, 0.0f, 1.0f);
                    wRock = std::max(sb, hb);
                }
                if (hn < sandThresh)
                {
                    wSand = Ogre::Math::Clamp(1.0f - hn / (sandThresh + 1e-6f), 0.0f, 1.0f);
                    wSand *= (1.0f - wRock);
                }
                if (hn >= sandThresh && hn < grassThresh && slopeN < slopeCut * 0.6f)
                {
                    float t = (hn - sandThresh) / (grassThresh - sandThresh + 1e-6f);
                    wGrass = Ogre::Math::Clamp(t * 1.4f, 0.0f, 1.0f);
                    wGrass *= (1.0f - wRock) * (1.0f - wSand);
                }
                float wDirt = Ogre::Math::Clamp(1.0f - wSand - wGrass - wRock, 0.0f, 1.0f);

                float total = wDirt + wGrass + wSand + wRock;
                if (total < 1e-6f)
                {
                    wDirt = 1.0f;
                    total = 1.0f;
                }

                int idx = (py * bts + px) * 4;
                blendOut[idx + 0] = static_cast<unsigned char>(Ogre::Math::Clamp((wDirt / total) * 255.0f, 0.0f, 255.0f));
                blendOut[idx + 1] = static_cast<unsigned char>(Ogre::Math::Clamp((wGrass / total) * 255.0f, 0.0f, 255.0f));
                blendOut[idx + 2] = static_cast<unsigned char>(Ogre::Math::Clamp((wSand / total) * 255.0f, 0.0f, 255.0f));
                blendOut[idx + 3] = static_cast<unsigned char>(Ogre::Math::Clamp((wRock / total) * 255.0f, 0.0f, 255.0f));
            }
        }
    }
#endif

    void ProceduralPlanetComponent::autoRPaintLayers(const std::vector<float>& heights, const std::vector<Ogre::Vector2>& uvCoords, int vertexCount, float radius, float amplitude, uint32_t blendSize, unsigned char* blendOut) const
    {
        int bts = static_cast<int>(std::round(std::sqrt(static_cast<float>(blendSize) / 4.0f)));
        if (bts < 1 || vertexCount < 4)
        {
            return;
        }

        // Detect segH by counting the first row (all vertices with same v=0).
        int segH = 0;
        while (segH + 1 < vertexCount && std::fabs(uvCoords[segH + 1].y - uvCoords[0].y) < 1e-5f)
        {
            ++segH;
        }
        if (segH < 1)
        {
            return;
        }
        int colsPerRow = segH + 1;
        int segV = (vertexCount / colsPerRow) - 1;
        if (segV < 1)
        {
            return;
        }

        float minH = std::numeric_limits<float>::max();
        float maxH = -std::numeric_limits<float>::max();
        for (int i = 0; i < vertexCount; ++i)
        {
            if (heights[i] < minH)
            {
                minH = heights[i];
            }
            if (heights[i] > maxH)
            {
                maxH = heights[i];
            }
        }
        float range = maxH - minH;
        if (range < 1e-6f)
        {
            range = 1.0f;
        }

        float sandThresh = this->layerSandThresh->getReal();
        float grassThresh = this->layerGrassThresh->getReal();
        float rockThresh = this->layerRockThresh->getReal();
        float slopeCut = this->layerSlopeRock->getReal();

        // Pixels whose sampled height is within this threshold of zero are
        // considered unmodified flat terrain — skip them so the base diffuse
        // (ground_dirt_gardenD) and any manual brush strokes show through.
        const float flatThreshold = amplitude * 0.05f;

        const float invBts = 1.0f / static_cast<float>(bts);

        for (int py = 0; py < bts; ++py)
        {
            for (int px = 0; px < bts; ++px)
            {
                float u = (static_cast<float>(px) + 0.5f) * invBts;
                float v = (static_cast<float>(py) + 0.5f) * invBts;

                // Bilinear sample helper (inline lambda).
                auto bilerp = [&](float su, float sv) -> float
                {
                    float cf = Ogre::Math::Clamp(su, 0.0f, 1.0f) * static_cast<float>(segH);
                    float rf = Ogre::Math::Clamp(sv, 0.0f, 1.0f) * static_cast<float>(segV);
                    int c0 = static_cast<int>(cf);
                    int c1 = std::min(c0 + 1, segH);
                    int r0 = static_cast<int>(rf);
                    int r1 = std::min(r0 + 1, segV);
                    float fc = cf - static_cast<float>(c0);
                    float fr = rf - static_cast<float>(r0);
                    int i00 = r0 * colsPerRow + c0;
                    int i10 = r0 * colsPerRow + c1;
                    int i01 = r1 * colsPerRow + c0;
                    int i11 = r1 * colsPerRow + c1;
                    return lerpF(fr, lerpF(fc, heights[i00], heights[i10]), lerpF(fc, heights[i01], heights[i11]));
                };

                float h = bilerp(u, v);
                float hr = bilerp(u + invBts, v);
                float hu = bilerp(u, v + invBts);

                // Skip unmodified flat pixels — preserve existing blend data so
                // base diffuse and manual brush strokes are not overwritten.
                if (std::fabs(h) < flatThreshold)
                {
                    continue;
                }

                float hn = (h - minH) / range;

                // Slope = height change per pixel, normalised by total relief.
                // Independent of radius and amplitude.
                float slope = std::sqrt((hr - h) * (hr - h) + (hu - h) * (hu - h));
                float slopeN = Ogre::Math::Clamp(slope / range, 0.0f, 1.0f);

                // Layer weights:
                // R = layer0 (dirt)   -- mid elevations, base fill
                // G = layer1 (grass)  -- low-mid, gentle slope
                // B = layer2 (sand)   -- lowest elevations
                // A = layer3 (rock)   -- high altitude OR steep slope
                float wRock = 0.0f;
                float wSand = 0.0f;
                float wGrass = 0.0f;

                if (slopeN >= slopeCut || hn >= rockThresh)
                {
                    float sb = Ogre::Math::Clamp((slopeN - slopeCut) / 0.2f, 0.0f, 1.0f);
                    float hb = Ogre::Math::Clamp((hn - rockThresh) / 0.15f, 0.0f, 1.0f);
                    wRock = std::max(sb, hb);
                }
                if (hn < sandThresh)
                {
                    wSand = Ogre::Math::Clamp(1.0f - hn / (sandThresh + 1e-6f), 0.0f, 1.0f);
                    wSand *= (1.0f - wRock);
                }
                if (hn >= sandThresh && hn < grassThresh && slopeN < slopeCut * 0.6f)
                {
                    float t = (hn - sandThresh) / (grassThresh - sandThresh + 1e-6f);
                    wGrass = Ogre::Math::Clamp(t * 1.4f, 0.0f, 1.0f);
                    wGrass *= (1.0f - wRock) * (1.0f - wSand);
                }
                float wDirt = Ogre::Math::Clamp(1.0f - wSand - wGrass - wRock, 0.0f, 1.0f);

                float total = wDirt + wGrass + wSand + wRock;
                if (total < 1e-6f)
                {
                    wDirt = 1.0f;
                    total = 1.0f;
                }

                int idx = (py * bts + px) * 4;
                blendOut[idx + 0] = static_cast<unsigned char>(Ogre::Math::Clamp((wDirt / total) * 255.0f, 0.0f, 255.0f));
                blendOut[idx + 1] = static_cast<unsigned char>(Ogre::Math::Clamp((wGrass / total) * 255.0f, 0.0f, 255.0f));
                blendOut[idx + 2] = static_cast<unsigned char>(Ogre::Math::Clamp((wSand / total) * 255.0f, 0.0f, 255.0f));
                blendOut[idx + 3] = static_cast<unsigned char>(Ogre::Math::Clamp((wRock / total) * 255.0f, 0.0f, 255.0f));
            }
        }
    }

    // =========================================================================
    // Lua API
    // =========================================================================

    ProceduralPlanetComponent* getProceduralPlanetComponent(GameObject* go)
    {
        return NOWA::makeStrongPtr(go->getComponent<ProceduralPlanetComponent>()).get();
    }

    ProceduralPlanetComponent* getProceduralPlanetComponentFromName(GameObject* go, const Ogre::String& name)
    {
        return NOWA::makeStrongPtr(go->getComponentFromName<ProceduralPlanetComponent>(name)).get();
    }

    void ProceduralPlanetComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
    {
        luabind::module(lua)[luabind::class_<ProceduralPlanetComponent, GameObjectComponent>("ProceduralPlanetComponent")
                .def("setHillAmplitude", &ProceduralPlanetComponent::setHillAmplitude)
                .def("getHillAmplitude", &ProceduralPlanetComponent::getHillAmplitude)
                .def("setHillFrequency", &ProceduralPlanetComponent::setHillFrequency)
                .def("getHillFrequency", &ProceduralPlanetComponent::getHillFrequency)
                .def("setOctaves", &ProceduralPlanetComponent::setOctaves)
                .def("getOctaves", &ProceduralPlanetComponent::getOctaves)
                .def("setPersistence", &ProceduralPlanetComponent::setPersistence)
                .def("getPersistence", &ProceduralPlanetComponent::getPersistence)
                .def("setLacunarity", &ProceduralPlanetComponent::setLacunarity)
                .def("getLacunarity", &ProceduralPlanetComponent::getLacunarity)
                .def("setSeed", &ProceduralPlanetComponent::setSeed)
                .def("getSeed", &ProceduralPlanetComponent::getSeed)
                .def("setEnableIsland", &ProceduralPlanetComponent::setEnableIsland)
                .def("getEnableIsland", &ProceduralPlanetComponent::getEnableIsland)
                .def("setIslandFalloff", &ProceduralPlanetComponent::setIslandFalloff)
                .def("getIslandFalloff", &ProceduralPlanetComponent::getIslandFalloff)
                .def("setIslandSize", &ProceduralPlanetComponent::setIslandSize)
                .def("getIslandSize", &ProceduralPlanetComponent::getIslandSize)
                .def("setEnableRivers", &ProceduralPlanetComponent::setEnableRivers)
                .def("getEnableRivers", &ProceduralPlanetComponent::getEnableRivers)
                .def("setRiverCount", &ProceduralPlanetComponent::setRiverCount)
                .def("getRiverCount", &ProceduralPlanetComponent::getRiverCount)
                .def("setRiverWidth", &ProceduralPlanetComponent::setRiverWidth)
                .def("getRiverWidth", &ProceduralPlanetComponent::getRiverWidth)
                .def("setRiverDepth", &ProceduralPlanetComponent::setRiverDepth)
                .def("getRiverDepth", &ProceduralPlanetComponent::getRiverDepth)
                .def("setRiverMeandering", &ProceduralPlanetComponent::setRiverMeandering)
                .def("getRiverMeandering", &ProceduralPlanetComponent::getRiverMeandering)
                .def("setEnableCanyons", &ProceduralPlanetComponent::setEnableCanyons)
                .def("getEnableCanyons", &ProceduralPlanetComponent::getEnableCanyons)
                .def("setCanyonCount", &ProceduralPlanetComponent::setCanyonCount)
                .def("getCanyonCount", &ProceduralPlanetComponent::getCanyonCount)
                .def("setCanyonWidth", &ProceduralPlanetComponent::setCanyonWidth)
                .def("getCanyonWidth", &ProceduralPlanetComponent::getCanyonWidth)
                .def("setCanyonDepth", &ProceduralPlanetComponent::setCanyonDepth)
                .def("getCanyonDepth", &ProceduralPlanetComponent::getCanyonDepth)
                .def("setCanyonSteepness", &ProceduralPlanetComponent::setCanyonSteepness)
                .def("getCanyonSteepness", &ProceduralPlanetComponent::getCanyonSteepness)
                .def("setEnableRoads", &ProceduralPlanetComponent::setEnableRoads)
                .def("getEnableRoads", &ProceduralPlanetComponent::getEnableRoads)
                .def("setRoadCount", &ProceduralPlanetComponent::setRoadCount)
                .def("getRoadCount", &ProceduralPlanetComponent::getRoadCount)
                .def("setRoadWidth", &ProceduralPlanetComponent::setRoadWidth)
                .def("getRoadWidth", &ProceduralPlanetComponent::getRoadWidth)
                .def("setRoadDepth", &ProceduralPlanetComponent::setRoadDepth)
                .def("getRoadDepth", &ProceduralPlanetComponent::getRoadDepth)
                .def("setRoadSmoothness", &ProceduralPlanetComponent::setRoadSmoothness)
                .def("getRoadSmoothness", &ProceduralPlanetComponent::getRoadSmoothness)
                .def("setRoadCurviness", &ProceduralPlanetComponent::setRoadCurviness)
                .def("getRoadCurviness", &ProceduralPlanetComponent::getRoadCurviness)
                .def("generateProceduralPlanet", &ProceduralPlanetComponent::generateProceduralPlanet)];

        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPlanetComponent", "class inherits GameObjectComponent", getStaticInfoText());

        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPlanetComponent", "void setHillAmplitude(float amplitude)", "Sets the hill height variation in world units (metres above/below sphere surface).");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPlanetComponent", "void setHillFrequency(float frequency)", "Sets the number of major terrain features across the sphere.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPlanetComponent", "void setSeed(uint seed)", "Sets the random seed. Change to generate a completely different planet.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPlanetComponent", "void setEnableIsland(bool enable)", "Enables radial island masking to create a landmass surrounded by lowlands.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPlanetComponent", "void setEnableRivers(bool enable)", "Enables flow-accumulation river carving.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPlanetComponent", "void setEnableCanyons(bool enable)", "Enables long canyon trench carving.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPlanetComponent", "void setEnableRoads(bool enable)", "Enables Catmull-Rom road path flattening.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPlanetComponent", "void generateProceduralPlanet()", "Runs the full generation pipeline and applies the result to the sibling PlanetTerraComponent.");

        gameObjectClass.def("getProceduralPlanetComponent", (ProceduralPlanetComponent * (*)(GameObject*)) & getProceduralPlanetComponent);
        gameObjectClass.def("getProceduralPlanetComponentFromName", &getProceduralPlanetComponentFromName);
        gameObjectControllerClass.def("castProceduralPlanetComponent", &GameObjectController::cast<ProceduralPlanetComponent>);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ProceduralPlanetComponent getProceduralPlanetComponent()", "Gets the ProceduralPlanetComponent from this GameObject.");
    }

} // namespace NOWA
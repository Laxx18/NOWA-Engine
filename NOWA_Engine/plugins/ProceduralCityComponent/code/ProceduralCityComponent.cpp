/*
Copyright (c) 2026 Lukas Kalinowski
GPL v3
*/

#include "NOWAPrecompiled.h"
#include "ProceduralCityComponent.h"
#include "../../ProceduralRoadComponent/code/ProceduralRoadComponent.h"
#include "gameobject/GameObjectFactory.h"
#include "gameobject/PhysicsArtifactComponent.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "modules/GraphicsModule.h"
#include "utilities/MathHelper.h"
#include "utilities/XMLConverter.h"

#include "OgreAbiUtils.h"
#include "OgreHlmsManager.h"
#include "OgreMeshManager2.h"
#include "OgreRoot.h"
#include "OgreSceneManager.h"
#include "OgreSubMesh2.h"
#include "Vao/OgreVaoManager.h"
#include "Vao/OgreVertexArrayObject.h"

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <future>
#include <random>

namespace
{
    // 64-bit FNV-1a-style hash helpers — ProceduralFoliageVolumeComponent pattern.
    // Reals quantised to 3 decimal places to avoid false stale-cache from float jitter.

    inline void cityHashCombine(uint64_t& seed, uint64_t value)
    {
        seed ^= value + 0x9E3779B97F4A7C15ULL + (seed << 6) + (seed >> 2);
    }
    inline uint64_t cityHashReal(Ogre::Real v)
    {
        return static_cast<uint64_t>(std::llround(static_cast<double>(v) * 1000.0));
    }
    inline uint64_t cityHashString(const Ogre::String& s)
    {
        std::hash<Ogre::String> h;
        return static_cast<uint64_t>(h(s));
    }

    // Sink depth: buildings are pushed this far below the terrain surface so
    // no gap appears at the base on sloped ground. Foliage does the same thing
    // with alignToNormal; we just apply a fixed downward offset instead.
    static constexpr Ogre::Real CITY_SINK_DEPTH = 0.35f;

    // Building geometry is split into NUM_BUILDING_VARIANTS batches, each getting a
    // different wall / roof / door datablock combination.  Each building is assigned to
    // a variant by (variantSeed % NUM_BUILDING_VARIANTS).
    // This gives genuine per-building visual variety without per-building draw calls —
    // every building in the same variant shares one merged BT_IMMUTABLE Item.
    static constexpr unsigned int NUM_BUILDING_VARIANTS = 4u;
    static constexpr unsigned int SLOTS_PER_VARIANT     = 6u;   // wall, roof, window, trim, door, garage
    static constexpr unsigned int INFRA_BATCH_OFFSET    = NUM_BUILDING_VARIANTS * SLOTS_PER_VARIANT;   // 24
    static constexpr unsigned int ROAD_SLOT             = INFRA_BATCH_OFFSET + 0u;   // 24
    static constexpr unsigned int CURB_SLOT             = INFRA_BATCH_OFFSET + 1u;   // 25
    static constexpr unsigned int TOTAL_CITY_BATCHES    = INFRA_BATCH_OFFSET + 2u;   // 26 (road + curb only)
}

namespace NOWA
{
    using namespace rapidxml;

    // =========================================================================
    // Plugin skeleton
    // =========================================================================

    void ProceduralCityComponent::install(const Ogre::NameValuePairList* options)
    {
        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<ProceduralCityComponent>(ProceduralCityComponent::getStaticClassId(), ProceduralCityComponent::getStaticClassName());
    }

    void ProceduralCityComponent::initialise()
    {
    }
    void ProceduralCityComponent::shutdown()
    {
    }
    void ProceduralCityComponent::uninstall()
    {
    }

    void ProceduralCityComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    const Ogre::String& ProceduralCityComponent::getName() const
    {
        return this->name;
    }

    Ogre::String ProceduralCityComponent::getClassName(void) const
    {
        return ProceduralCityComponent::getStaticClassName();
    }

    Ogre::String ProceduralCityComponent::getParentClassName(void) const
    {
        return "GameObjectComponent";
    }

    // =========================================================================
    // Constructor
    // =========================================================================

    ProceduralCityComponent::ProceduralCityComponent() :
        GameObjectComponent(),
        name("ProceduralCityComponent"),
        physicsArtifactComponent(nullptr),
        roadComponentId(0),
        roadConnectionAtStart(false),
        groundQuery(nullptr),
        isDirty(true),
        cityLoadedFromScene(false),
        cityOrigin(Ogre::Vector3::ZERO),
        activated(new Variant(AttrActivated(), true, this->attributes)),
        cityBoundsAttr(new Variant(AttrCityBounds(), Ogre::Vector4(-100.f, -100.f, 100.f, 100.f), this->attributes)),
        masterSeedAttr(new Variant(AttrMasterSeed(), 42u, this->attributes)),
        blockSizeAttr(new Variant(AttrBlockSize(), 40.f, this->attributes)),
        roadWidthAttr(new Variant(AttrRoadWidth(), 6.f, this->attributes)),
        roadVarianceAttr(new Variant(AttrRoadVariance(), 0.0f, this->attributes)),
        sidewalkWidthAttr(new Variant(AttrSidewalkWidth(), 2.f, this->attributes)),
        curbHeightAttr(new Variant(AttrCurbHeight(), 0.15f, this->attributes)),
        generateRoadsAttr(new Variant(AttrGenerateRoads(), true, this->attributes)),
        roadDatablockAttr(new Variant(AttrRoadDatablock(), Ogre::String("city_road_01"), this->attributes)),
        curbDatablockAttr(new Variant(AttrCurbDatablock(), Ogre::String("city_curb_01"), this->attributes)),
        roadComponentIdAttr(new Variant(AttrRoadComponentId(), Ogre::String("0"), this->attributes)),
        roadConnectionAtStartAttr(new Variant(AttrRoadConnectionAtStart(), false, this->attributes)),
        districtCountAttr(new Variant(AttrDistrictCount(), 1u, this->attributes)),
        doorDatablockAttr(new Variant(AttrDoorDatablock(), Ogre::String("city_door_01"), this->attributes)),
        garageDatablockAttr(new Variant(AttrGarageDatablock(), Ogre::String("city_garage_01"), this->attributes)),
        generateGarageAttr(new Variant(AttrGenerateGarage(), true, this->attributes)),
        generateBtn(new Variant(AttrGenerate(), Ogre::String("Generate Now"), this->attributes)),
        clearBtn(new Variant(AttrClear(), Ogre::String("Clear"), this->attributes))
    {
        this->generateBtn->addUserData(GameObject::AttrActionExec());
        this->generateBtn->addUserData(GameObject::AttrActionExecId(), ActionGenerate());
        this->clearBtn->addUserData(GameObject::AttrActionExec());
        this->clearBtn->addUserData(GameObject::AttrActionExecId(), ActionClear());

        this->cityBoundsAttr->setDescription("World-space XZ footprint (minX, minZ, maxX, maxZ). Centre the GameObject inside this area.");
        this->masterSeedAttr->setDescription("Master seed. Same seed + same parameters = identical city.");
        this->masterSeedAttr->setConstraints(0u, 999999u);

        this->roadVarianceAttr->setDescription("Road variance [0..0.5]: 0 = straight grid. Higher values add organic curves by routing each segment through a slightly offset midpoint. Each inner road is split into two sub-segments through that midpoint.");
        this->roadVarianceAttr->setConstraints(0.0f, 0.5f);
        this->blockSizeAttr->setDescription("Road-centre-to-road-centre block pitch in meters. Typical: 30-80m.");
        this->blockSizeAttr->setConstraints(10.f, 200.f);
        this->roadWidthAttr->setDescription("Drivable road half-width in meters. Default 6m.");
        this->roadWidthAttr->setConstraints(2.f, 20.f);
        this->sidewalkWidthAttr->setDescription("Setback between road edge and building face in meters.");
        this->sidewalkWidthAttr->setConstraints(0.f, 10.f);
        this->curbHeightAttr->setDescription("Kerb strip height in meters. 0 = no kerb.");
        this->curbHeightAttr->setConstraints(0.f, 1.f);
        this->generateRoadsAttr->setDescription("Generate internal road-strip geometry.");

        this->roadDatablockAttr->setDescription("PBS datablock for the road surface.");
        this->roadDatablockAttr->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");

        this->curbDatablockAttr->setDescription("PBS datablock for the kerb/curb strip.");
        this->curbDatablockAttr->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");


        this->roadComponentIdAttr->setDescription("ID of an existing ProceduralRoadComponent to connect to the city. '0' = none.");
        this->roadConnectionAtStartAttr->setDescription("Connect at road START (true) or END (false). Use false when road was drawn toward the city.");
        this->districtCountAttr->setDescription("Number of city districts. Each controls height range, density, and material variety.");
        this->districtCountAttr->addUserData(GameObject::AttrActionNeedRefresh());
        this->districtCountAttr->addUserData(GameObject::AttrActionSeparator());
        this->doorDatablockAttr->setDescription("PBS datablock for house doors. Shared by all buildings. Default placeholder: 'city_door_01'.");
        this->doorDatablockAttr->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");

        this->garageDatablockAttr->setDescription("PBS datablock for garage doors (placed on some residential buildings as an attached garage cube). Default: 'city_garage_01'.");
        this->garageDatablockAttr->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");

        this->generateGarageAttr->setDescription("If true, ~30% of Residential and Mixed buildings get an attached garage on their right face.  Disable to build a city without garages.");

        this->generateBtn->setDescription("Generate or regenerate the entire city. Cache is invalidated and rewritten.");
        this->clearBtn->setDescription("Clear all city geometry and delete the cache file.");

        this->setDistrictCount(1u);
    }

    ProceduralCityComponent::~ProceduralCityComponent()
    {
    }

    // =========================================================================
    // Lifecycle
    // =========================================================================

    bool ProceduralCityComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrActivated())
        {
            this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrCityBounds())
        {
            this->cityBoundsAttr->setValue(XMLConverter::getAttribVector4(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrMasterSeed())
        {
            this->masterSeedAttr->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data", 42u));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrBlockSize())
        {
            this->blockSizeAttr->setValue(XMLConverter::getAttribReal(propertyElement, "data", 40.f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrRoadWidth())
        {
            this->roadWidthAttr->setValue(XMLConverter::getAttribReal(propertyElement, "data", 6.f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrRoadVariance())
        {
            this->roadVarianceAttr->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrSidewalkWidth())
        {
            this->sidewalkWidthAttr->setValue(XMLConverter::getAttribReal(propertyElement, "data", 2.f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrCurbHeight())
        {
            this->curbHeightAttr->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.15f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrGenerateRoads())
        {
            this->generateRoadsAttr->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrRoadDatablock())
        {
            this->roadDatablockAttr->setValue(XMLConverter::getAttrib(propertyElement, "data", "city_road_01"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrCurbDatablock())
        {
            this->curbDatablockAttr->setValue(XMLConverter::getAttrib(propertyElement, "data", "city_curb_01"));
            propertyElement = propertyElement->next_sibling("property");
        }        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrRoadComponentId())
        {
            this->roadComponentIdAttr->setValue(XMLConverter::getAttrib(propertyElement, "data", "0"));
            this->roadComponentId = static_cast<unsigned long>(std::strtoul(this->roadComponentIdAttr->getString().c_str(), nullptr, 10));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrDoorDatablock())
        {
            this->doorDatablockAttr->setValue(XMLConverter::getAttrib(propertyElement, "data", "city_door_01"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrGarageDatablock())
        {
            this->garageDatablockAttr->setValue(XMLConverter::getAttrib(propertyElement, "data", "city_garage_01"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrGenerateGarage())
        {
            this->generateGarageAttr->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrRoadConnectionAtStart())
        {
            this->roadConnectionAtStartAttr->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
            this->roadConnectionAtStart = this->roadConnectionAtStartAttr->getBool();
            propertyElement = propertyElement->next_sibling("property");
        }

        unsigned int count = 1u;
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrDistrictCount())
        {
            count = XMLConverter::getAttribUnsignedInt(propertyElement, "data", 1u);
            this->districtCountAttr->setValue(count);
            propertyElement = propertyElement->next_sibling("property");
        }
        this->setDistrictCount(count);

        for (unsigned int i = 0; i < count; ++i)
        {
            Ogre::String idx = Ogre::StringConverter::toString(i);

            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrDistrictName() + idx)
            {
                this->setDistrictName(i, XMLConverter::getAttrib(propertyElement, "data", "District"));
                propertyElement = propertyElement->next_sibling("property");
            }
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrDistrictType() + idx)
            {
                this->setDistrictType(i, XMLConverter::getAttrib(propertyElement, "data", "Residential_Low"));
                propertyElement = propertyElement->next_sibling("property");
            }
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrDistrictMinHeight() + idx)
            {
                this->setDistrictMinHeight(i, XMLConverter::getAttribReal(propertyElement, "data", 4.f));
                propertyElement = propertyElement->next_sibling("property");
            }
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrDistrictMaxHeight() + idx)
            {
                this->setDistrictMaxHeight(i, XMLConverter::getAttribReal(propertyElement, "data", 9.f));
                propertyElement = propertyElement->next_sibling("property");
            }
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrDistrictMinFootprint() + idx)
            {
                this->setDistrictMinFootprint(i, XMLConverter::getAttribVector2(propertyElement, "data"));
                propertyElement = propertyElement->next_sibling("property");
            }
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrDistrictMaxFootprint() + idx)
            {
                this->setDistrictMaxFootprint(i, XMLConverter::getAttribVector2(propertyElement, "data"));
                propertyElement = propertyElement->next_sibling("property");
            }
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrDistrictDensity() + idx)
            {
                this->setDistrictDensity(i, XMLConverter::getAttribReal(propertyElement, "data", 0.85f));
                propertyElement = propertyElement->next_sibling("property");
            }
            for (unsigned int v = 0; v < 3u; ++v)
            {
                Ogre::String key = AttrDistrictRoofDatablock() + idx + "_" + Ogre::StringConverter::toString(v);
                if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == key)
                {
                    this->setDistrictRoofDatablock(i, v, XMLConverter::getAttrib(propertyElement, "data"));
                    propertyElement = propertyElement->next_sibling("property");
                }
            }
            for (unsigned int v = 0; v < 3u; ++v)
            {
                Ogre::String key = AttrDistrictWindowDatablock() + idx + "_" + Ogre::StringConverter::toString(v);
                if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == key)
                {
                    this->setDistrictWindowDatablock(i, v, XMLConverter::getAttrib(propertyElement, "data"));
                    propertyElement = propertyElement->next_sibling("property");
                }
            }
            for (unsigned int v = 0; v < 2u; ++v)
            {
                Ogre::String key = AttrDistrictTrimDatablock() + idx + "_" + Ogre::StringConverter::toString(v);
                if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == key)
                {
                    this->setDistrictTrimDatablock(i, v, XMLConverter::getAttrib(propertyElement, "data"));
                    propertyElement = propertyElement->next_sibling("property");
                }
            }
        }

        return true;
    }

    bool ProceduralCityComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralCityComponent] postInit for: " + this->gameObjectPtr->getName());

        AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &ProceduralCityComponent::handleComponentManuallyDeleted), EventDataDeleteComponent::getStaticEventType());

        this->groundQuery = this->gameObjectPtr->getSceneManager()->createRayQuery(Ogre::Ray(), GameObjectController::ALL_CATEGORIES_ID);
        this->groundQuery->setSortByDistance(true);
        this->groundPlane = Ogre::Plane(Ogre::Vector3::UNIT_Y, 0.0f);

        const auto& physicsArtifactCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsArtifactComponent>());
        if (physicsArtifactCompPtr)
        {
            this->physicsArtifactComponent = physicsArtifactCompPtr.get();
        }

        if (false == this->cityLoadedFromScene)
        {
            AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &ProceduralCityComponent::handleSceneParsed), EventDataSceneParsed::getStaticEventType());
        }

        return true;
    }

    bool ProceduralCityComponent::connect(void)
    {
        GameObjectComponent::connect();
        return true;
    }
    bool ProceduralCityComponent::disconnect(void)
    {
        GameObjectComponent::disconnect();
        return true;
    }
    GameObjectCompPtr ProceduralCityComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        return GameObjectCompPtr();
    }
    bool ProceduralCityComponent::onCloned(void)
    {
        return true;
    }
    void ProceduralCityComponent::onAddComponent(void)
    {
    }

    void ProceduralCityComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();

        AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ProceduralCityComponent::handleSceneParsed), EventDataSceneParsed::getStaticEventType());
        AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ProceduralCityComponent::handleComponentManuallyDeleted), EventDataDeleteComponent::getStaticEventType());

        this->clearCity(false);

        if (this->groundQuery)
        {
            auto* q = this->groundQuery;
            GraphicsModule::getInstance()->enqueueAndWait(
                [this, q]()
                {
                    this->gameObjectPtr->getSceneManager()->destroyQuery(q);
                },
                "ProceduralCityComponent::DestroyQuery");
            this->groundQuery = nullptr;
        }

        this->physicsArtifactComponent = nullptr;
    }

    void ProceduralCityComponent::onOtherComponentRemoved(unsigned int index)
    {
        if (nullptr != this->physicsArtifactComponent && index == this->physicsArtifactComponent->getIndex())
        {
            this->physicsArtifactComponent = nullptr;
        }
    }

    void ProceduralCityComponent::onOtherComponentAdded(unsigned int index)
    {
    }
    void ProceduralCityComponent::update(Ogre::Real dt, bool notSimulating)
    {
    }

    // =========================================================================
    // actualizeValue
    // =========================================================================

    void ProceduralCityComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (AttrActivated() == attribute->getName())
        {
            this->activated->setValue(attribute->getBool());
        }
        else if (AttrCityBounds() == attribute->getName())
        {
            this->setCityBounds(attribute->getVector4());
        }
        else if (AttrMasterSeed() == attribute->getName())
        {
            this->setMasterSeed(attribute->getUInt());
        }
        else if (AttrBlockSize() == attribute->getName())
        {
            this->setBlockSize(attribute->getReal());
        }
        else if (AttrRoadWidth() == attribute->getName())
        {
            this->setRoadWidth(attribute->getReal());
        }
        else if (AttrSidewalkWidth() == attribute->getName())
        {
            this->setSidewalkWidth(attribute->getReal());
        }
        else if (AttrCurbHeight() == attribute->getName())
        {
            this->setCurbHeight(attribute->getReal());
        }
        else if (AttrGenerateRoads() == attribute->getName())
        {
            this->setGenerateRoads(attribute->getBool());
        }
        else if (AttrRoadDatablock() == attribute->getName())
        {
            this->setRoadDatablock(attribute->getString());
        }
        else if (AttrCurbDatablock() == attribute->getName())
        {
            this->setCurbDatablock(attribute->getString());
        }
        else if (AttrDoorDatablock() == attribute->getName()) { this->setDoorDatablock(attribute->getString()); }
        else if (AttrGarageDatablock() == attribute->getName()) { this->setGarageDatablock(attribute->getString()); }
        else if (AttrGenerateGarage() == attribute->getName()) { this->generateGarageAttr->setValue(attribute->getBool()); }
        else if (AttrRoadComponentId() == attribute->getName())
        {
            this->setRoadComponentId(static_cast<unsigned long>(std::strtoul(attribute->getString().c_str(), nullptr, 10)));
        }
        else if (AttrRoadConnectionAtStart() == attribute->getName())
        {
            this->setRoadConnectionAtStart(attribute->getBool());
        }
        else if (AttrDistrictCount() == attribute->getName())
        {
            this->setDistrictCount(attribute->getUInt());
        }
        else
        {
            for (unsigned int i = 0; i < static_cast<unsigned int>(this->districts.size()); ++i)
            {
                Ogre::String idx = Ogre::StringConverter::toString(i);
                if (AttrDistrictName() + idx == attribute->getName())
                {
                    this->setDistrictName(i, attribute->getString());
                }
                else if (AttrDistrictType() + idx == attribute->getName())
                {
                    this->setDistrictType(i, attribute->getString());
                }
                else if (AttrDistrictMinHeight() + idx == attribute->getName())
                {
                    this->setDistrictMinHeight(i, attribute->getReal());
                }
                else if (AttrDistrictMaxHeight() + idx == attribute->getName())
                {
                    this->setDistrictMaxHeight(i, attribute->getReal());
                }
                else if (AttrDistrictMinFootprint() + idx == attribute->getName())
                {
                    this->setDistrictMinFootprint(i, attribute->getVector2());
                }
                else if (AttrDistrictMaxFootprint() + idx == attribute->getName())
                {
                    this->setDistrictMaxFootprint(i, attribute->getVector2());
                }
                else if (AttrDistrictDensity() + idx == attribute->getName())
                {
                    this->setDistrictDensity(i, attribute->getReal());
                }
                else
                {
                    for (unsigned int v = 0; v < 3u; ++v)
                    {
                        if (AttrDistrictRoofDatablock() + idx + "_" + Ogre::StringConverter::toString(v) == attribute->getName())
                        {
                            this->setDistrictRoofDatablock(i, v, attribute->getString());
                        }
                        if (AttrDistrictWindowDatablock() + idx + "_" + Ogre::StringConverter::toString(v) == attribute->getName())
                        {
                            this->setDistrictWindowDatablock(i, v, attribute->getString());
                        }
                    }
                    for (unsigned int v = 0; v < 2u; ++v)
                    {
                        if (AttrDistrictTrimDatablock() + idx + "_" + Ogre::StringConverter::toString(v) == attribute->getName())
                        {
                            this->setDistrictTrimDatablock(i, v, attribute->getString());
                        }
                    }
                }
            }
        }
    }

    // =========================================================================
    // writeXML
    // =========================================================================

    void ProceduralCityComponent::writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc)
    {
        GameObjectComponent::writeXML(propertiesXML, doc);

        rapidxml::xml_node<>* propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "CityBounds"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->cityBoundsAttr->getVector4())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "MasterSeed"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->masterSeedAttr->getUInt())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "BlockSize"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->blockSizeAttr->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "RoadWidth"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->roadWidthAttr->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "RoadVariance"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->roadVarianceAttr->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "SidewalkWidth"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->sidewalkWidthAttr->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "CurbHeight"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->curbHeightAttr->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "GenerateRoads"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->generateRoadsAttr->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "RoadDatablock"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->roadDatablockAttr->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "CurbDatablock"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->curbDatablockAttr->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "DoorDatablock"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->doorDatablockAttr->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "GarageDatablock"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->garageDatablockAttr->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "GenerateGarage"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->generateGarageAttr->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "RoadComponentId"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->roadComponentIdAttr->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "RoadConnectionAtStart"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->roadConnectionAtStartAttr->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "DistrictCount"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, static_cast<unsigned int>(this->districts.size()))));
        propertiesXML->append_node(propertyXML);

        for (unsigned int i = 0; i < static_cast<unsigned int>(this->districts.size()); ++i)
        {
            Ogre::String idx = Ogre::StringConverter::toString(i);

            propertyXML = doc.allocate_node(rapidxml::node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrDistrictName() + idx)));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->getDistrictName(i))));
            propertiesXML->append_node(propertyXML);

            propertyXML = doc.allocate_node(rapidxml::node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrDistrictType() + idx)));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->getDistrictType(i))));
            propertiesXML->append_node(propertyXML);

            propertyXML = doc.allocate_node(rapidxml::node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrDistrictMinHeight() + idx)));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->getDistrictMinHeight(i))));
            propertiesXML->append_node(propertyXML);

            propertyXML = doc.allocate_node(rapidxml::node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrDistrictMaxHeight() + idx)));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->getDistrictMaxHeight(i))));
            propertiesXML->append_node(propertyXML);

            propertyXML = doc.allocate_node(rapidxml::node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrDistrictMinFootprint() + idx)));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->getDistrictMinFootprint(i))));
            propertiesXML->append_node(propertyXML);

            propertyXML = doc.allocate_node(rapidxml::node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrDistrictMaxFootprint() + idx)));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->getDistrictMaxFootprint(i))));
            propertiesXML->append_node(propertyXML);

            propertyXML = doc.allocate_node(rapidxml::node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrDistrictDensity() + idx)));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->getDistrictDensity(i))));
            propertiesXML->append_node(propertyXML);

            propertyXML = doc.allocate_node(rapidxml::node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6"));

            for (unsigned int v = 0; v < 3u; ++v)
            {
                propertyXML = doc.allocate_node(rapidxml::node_element, "property");
                propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
                propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrDistrictRoofDatablock() + idx + "_" + Ogre::StringConverter::toString(v))));
                propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->getDistrictRoofDatablock(i, v))));
                propertiesXML->append_node(propertyXML);
            }
            for (unsigned int v = 0; v < 3u; ++v)
            {
                propertyXML = doc.allocate_node(rapidxml::node_element, "property");
                propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
                propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrDistrictWindowDatablock() + idx + "_" + Ogre::StringConverter::toString(v))));
                propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->getDistrictWindowDatablock(i, v))));
                propertiesXML->append_node(propertyXML);
            }
            for (unsigned int v = 0; v < 2u; ++v)
            {
                propertyXML = doc.allocate_node(rapidxml::node_element, "property");
                propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
                propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrDistrictTrimDatablock() + idx + "_" + Ogre::StringConverter::toString(v))));
                propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->getDistrictTrimDatablock(i, v))));
                propertiesXML->append_node(propertyXML);
            }
        }
    }

    bool ProceduralCityComponent::executeAction(const Ogre::String& actionId, NOWA::Variant* attribute)
    {
        if (actionId == ActionGenerate())
        {
            this->generateCity();
            return true;
        }
        if (actionId == ActionClear())
        {
            this->clearCity(true);
            return true;
        }
        return false;
    }

    bool ProceduralCityComponent::canStaticAddComponent(GameObject* gameObject)
    {
        return true;
    }

    std::optional<NOWA::GameObjectTypeDescriptor> ProceduralCityComponent::getStaticTypeDescriptor()
    {
        NOWA::GameObjectTypeDescriptor desc;
        desc.type = eType::CUSTOM;
        desc.displayName = "Procedural City";
        desc.meshToDisplay = "Node.mesh";
        desc.needsMeshItem = true;
        desc.forceZeroTransform = false;
        // ProceduralRoadComponent is auto-created alongside ProceduralCityComponent so the
        // city has a reliable, already-tested road generator built in — no custom road mesh
        // generation needed.  ProceduralCityComponent.buildCityRoadNetwork() feeds the grid
        // segments to it via addRoadSegmentLua().
        desc.autoComponents = {
            ProceduralCityComponent::getStaticClassName(),
            ProceduralRoadComponent::getStaticClassName()
        };
        return desc;
    }

    // =========================================================================
    // Generation entry points
    // =========================================================================

    void ProceduralCityComponent::generateCity(void)
    {
        if (!this->gameObjectPtr || this->districts.empty())
        {
            return;
        }
        this->clearCity(true);

        auto t0 = std::chrono::high_resolution_clock::now();
        std::vector<CityBatch> batches = this->generateCityLayout();
        if (batches.empty())
        {
            return;
        }

        this->saveCityDataToFile(batches);
        this->createCityOnRenderThread(std::move(batches));

        auto t1 = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        size_t total = 0;
        for (const auto& b : this->cityBatches)
        {
            total += b.instances.size();
        }
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralCityComponent] Generated " + Ogre::StringConverter::toString(total) + " buildings in " + Ogre::StringConverter::toString(ms * 0.001) + "s");
        this->isDirty = false;
    }

    void ProceduralCityComponent::clearCity(bool deleteCacheFile)
    {
        // Also clear the ProceduralRoadComponent's road network so "Clear" button
        // removes roads too (previously only buildings/walls were cleared).
        auto roadCompPtr = NOWA::makeStrongPtr(
            this->gameObjectPtr->getComponent<ProceduralRoadComponent>());
        if (nullptr != roadCompPtr)
        {
            // clearAllSegments() early-returns if already empty — safe to call unconditionally.
            roadCompPtr->clearAllSegments();
        }

        GraphicsModule::getInstance()->enqueueAndWait(
            [this]()
            {
                this->destroyCityOnRenderThread();
            },
            "ProceduralCityComponent::ClearCity");

        if (nullptr != this->physicsArtifactComponent && nullptr != this->physicsArtifactComponent->getBody())
        {
            this->physicsArtifactComponent->destroyBody();
        }
        if (deleteCacheFile)
        {
            this->deleteCityDataFile();
        }
    }

    void ProceduralCityComponent::loadOrGenerateCity(void)
    {
        if (!this->gameObjectPtr || this->districts.empty())
        {
            return;
        }

        const auto& compPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsArtifactComponent>());
        if (compPtr)
        {
            this->physicsArtifactComponent = compPtr.get();
            this->physicsArtifactComponent->getAttribute(PhysicsArtifactComponent::AttrSerialize())->setValue(false);
            this->physicsArtifactComponent->getAttribute(PhysicsArtifactComponent::AttrSerialize())->setVisible(false);
        }

        this->clearCity(false);

        auto t0 = std::chrono::high_resolution_clock::now();
        std::vector<CityBatch> batches;
        bool fromCache = this->loadCityDataFromFile(batches);

        if (false == fromCache)
        {
            batches = this->generateCityLayout();
            if (batches.empty())
            {
                return;
            }
            this->saveCityDataToFile(batches);
        }

        this->createCityOnRenderThread(std::move(batches));

        double ms = std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - t0).count();
        size_t total = 0;
        for (const auto& b : this->cityBatches)
        {
            total += b.instances.size();
        }
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralCityComponent] " + Ogre::String(fromCache ? "Loaded " : "Generated ") + Ogre::StringConverter::toString(total) + " buildings in " +
                                                                                Ogre::StringConverter::toString(ms * 0.001) + "s" + Ogre::String(fromCache ? " (cache)" : ""));
        this->isDirty = false;
    }

    // =========================================================================
    // generateCityLayout — logic thread
    // =========================================================================

    std::vector<CityBatch> ProceduralCityComponent::generateCityLayout(void)
    {
        const Ogre::Vector4 bounds = this->cityBoundsAttr->getVector4();
        const Ogre::Real minX = bounds.x;
        const Ogre::Real minZ = bounds.y;
        const Ogre::Real maxX = bounds.z;
        const Ogre::Real maxZ = bounds.w;
        const Ogre::Real blockSz = this->blockSizeAttr->getReal();
        const unsigned int mSeed = this->masterSeedAttr->getUInt();
        const Ogre::Real variance = this->roadVarianceAttr->getReal();

        // ---- Build the road grid (may be perturbed by variance) ----------------
        // The same grid is used for road generation AND block center/size calculation
        // so both always use a consistent coordinate system.
        //
        // Base grid: evenly spaced from minX to maxX in steps of blockSz.
        // Perturbed grid: inner lines (not outer boundaries) are shifted ±(variance × blockSz × 0.3)
        // using a deterministic per-line hash so each city always looks the same.
        // The perturbation creates blocks of varying widths and depths — visually very
        // different from a uniform grid even at variance=0.2.
        {
            this->cityGridX.clear();
            this->cityGridZ.clear();

            // X lines
            for (Ogre::Real gx = minX; gx <= maxX + blockSz * 0.01f; gx += blockSz)
            {
                this->cityGridX.push_back(gx);
            }
            // Z lines
            for (Ogre::Real gz = minZ; gz <= maxZ + blockSz * 0.01f; gz += blockSz)
            {
                this->cityGridZ.push_back(gz);
            }

            if (variance > 0.01f)
            {
                const Ogre::Real maxShift = variance * blockSz * 0.3f;

                // Perturb inner X lines (skip first and last — outer boundary stays fixed)
                for (size_t i = 1; i + 1 < this->cityGridX.size(); ++i)
                {
                    uint32_t h = (static_cast<uint32_t>(i) * 2654435761u) ^ mSeed;
                    h ^= h >> 16; h *= 0x45d9f3bu; h ^= h >> 16;
                    const Ogre::Real shift = (static_cast<Ogre::Real>(h & 0xFFFFu) / 65535.f * 2.f - 1.f) * maxShift;
                    this->cityGridX[i] = std::max(this->cityGridX[i-1] + blockSz * 0.35f,
                                         std::min(this->cityGridX[i+1] - blockSz * 0.35f,
                                                  this->cityGridX[i] + shift));
                }

                // Perturb inner Z lines
                for (size_t i = 1; i + 1 < this->cityGridZ.size(); ++i)
                {
                    uint32_t h = (static_cast<uint32_t>(i + 100u) * 2654435761u) ^ mSeed;
                    h ^= h >> 16; h *= 0x45d9f3bu; h ^= h >> 16;
                    const Ogre::Real shift = (static_cast<Ogre::Real>(h & 0xFFFFu) / 65535.f * 2.f - 1.f) * maxShift;
                    this->cityGridZ[i] = std::max(this->cityGridZ[i-1] + blockSz * 0.35f,
                                         std::min(this->cityGridZ[i+1] - blockSz * 0.35f,
                                                  this->cityGridZ[i] + shift));
                }
            }
        }

        // ---- Build blocks from the (possibly perturbed) grid -------------------
        // Each grid cell becomes one CityBlock whose centre and size derive from
        // the actual perturbed grid line positions — so road widths, building lots,
        // and road topology are always consistent with each other.
        std::vector<CityBlock> blocks;
        for (size_t xi = 0; xi + 1 < this->cityGridX.size(); ++xi)
        {
            for (size_t zi = 0; zi + 1 < this->cityGridZ.size(); ++zi)
            {
                const Ogre::Real cellW = this->cityGridX[xi + 1] - this->cityGridX[xi];
                const Ogre::Real cellD = this->cityGridZ[zi + 1] - this->cityGridZ[zi];

                CityBlock blk;
                blk.centre = Ogre::Vector3(
                    (this->cityGridX[xi] + this->cityGridX[xi + 1]) * 0.5f, 0.f,
                    (this->cityGridZ[zi] + this->cityGridZ[zi + 1]) * 0.5f);
                blk.size = Ogre::Vector2(cellW, cellD);
                blk.groundHeight = this->getGroundHeight(blk.centre);
                blk.centre.y = blk.groundHeight;
                blk.districtIdx = 0;

                blocks.push_back(blk);
            }
        }

        if (blocks.empty())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralCityComponent] No blocks produced — check CityBounds / BlockSize.");
            return {};
        }

        this->assignDistricts(blocks);

        {
            std::mt19937 rng(mSeed + 77777u);
            std::uniform_real_distribution<float> d01(0.f, 1.f);
            for (auto& blk : blocks)
            {
                if (blk.districtIdx < this->districts.size())
                {
                    // hasCourtyardWall removed
                }
            }
        }

        for (unsigned int bi = 0; bi < static_cast<unsigned int>(blocks.size()); ++bi)
        {
            this->subdivideBlock(blocks[bi], bi);
        }

        // =====================================================================
        // ROOT-CAUSE FIX for the road/building offset on clear+regenerate:
        //
        // ProceduralRoadComponent::createRoadMeshInternal() calls:
        //   gameObjectPtr->getSceneNode()->setPosition(roadOrigin)
        // which MOVES the shared city SceneNode to the first road grid endpoint.
        //
        // If cityOrigin is computed from the bounds centre (a different XYZ than
        // roadOrigin), buildings and roads end up in different coordinate spaces
        // and appear offset from each other every time the city is regenerated.
        //
        // THE FIX: build the road grid FIRST so the SceneNode is at roadOrigin,
        // then set cityOrigin = SceneNode position (= roadOrigin).
        // All subsequent building geometry is then generated in the same local
        // space as the road, so both always line up — even across regenerations.
        // =====================================================================
        if (this->generateRoadsAttr->getBool())
        {
            // Build road first (moves SceneNode to roadOrigin via ProceduralRoadComponent)
            this->buildCityRoadNetwork(blocks);
            // Now capture that exact position as cityOrigin
            this->cityOrigin = this->gameObjectPtr->getSceneNode()->_getDerivedPositionUpdated();
        }
        else
        {
            // Roads off: fall back to bounds centre as origin
            const Ogre::Real cx2 = (minX + maxX) * 0.5f;
            const Ogre::Real cz2 = (minZ + maxZ) * 0.5f;
            this->cityOrigin = Ogre::Vector3(cx2, this->getGroundHeight(Ogre::Vector3(cx2, 0.f, cz2)), cz2);
        }
        const Ogre::Vector3& localOrigin = this->cityOrigin;

        // 23 batches total:
        // Building variants 0-3: slots v*5+0..v*5+4 (wall, roof, window, trim, door)
        // Infrastructure: slot 20 = road, 21 = curb, 22 = courtyard wall
        std::vector<CityBatch> batches(TOTAL_CITY_BATCHES);
        for (unsigned int s = 0; s < TOTAL_CITY_BATCHES; ++s)
        {
            batches[s].materialSlot = s;
        }

        for (unsigned int bi = 0; bi < static_cast<unsigned int>(blocks.size()); ++bi)
        {
            const CityBlock& blk = blocks[bi];
            for (const auto& lot : blk.lots)
            {
                if (false == lot.occupied)
                {
                    continue;
                }
                if (lot.districtIdx >= this->districts.size())
                {
                    continue;
                }
                const CityDistrict& district = this->districts[lot.districtIdx];

                std::mt19937 rng(mSeed ^ (bi * 6271u) ^ (static_cast<unsigned int>(&lot - &blk.lots[0]) * 9337u));
                std::uniform_real_distribution<float> d01(0.f, 1.f);

                BuildingInstance building;
                building.position = Ogre::Vector3(lot.centre.x, 0.f, lot.centre.z);
                building.groundHeight = lot.groundHeight;
                // Lot half-width: actual distance from the BUILDING'S world position to the
                // NEAREST block boundary in either X or Z, minus the road inset.
                // Using the minimum of all 4 directions gives the most conservative bound
                // so garages never extend past the lot edge regardless of building position
                // or orientation within the block.
                {
                    const Ogre::Real blkInset = this->roadWidthAttr->getReal() * 0.5f + this->sidewalkWidthAttr->getReal();
                    const Ogre::Real dxNeg = (lot.centre.x - blk.centre.x) + blk.size.x * 0.5f - blkInset;
                    const Ogre::Real dxPos = (blk.centre.x + blk.size.x * 0.5f - blkInset) - lot.centre.x;
                    const Ogre::Real dzNeg = (lot.centre.z - blk.centre.z) + blk.size.y * 0.5f - blkInset;
                    const Ogre::Real dzPos = (blk.centre.z + blk.size.y * 0.5f - blkInset) - lot.centre.z;
                    building.lotHalfRight = std::max(0.f, std::min({dxNeg, dxPos, dzNeg, dzPos}));
                }
                building.archetypeIdx = lot.districtIdx;
                building.variantSeed = mSeed ^ (bi * 6271u) ^ (static_cast<unsigned int>(&lot - &blk.lots[0]) * 9337u);

                const Ogre::Real snapAngles[2] = {0.f, Ogre::Math::HALF_PI};
                Ogre::Real baseAngle = snapAngles[building.variantSeed % 2u];
                Ogre::Real jitter = (static_cast<float>(building.variantSeed % 1000u) / 1000.f * 0.35f) - 0.175f;
                building.orientation.FromAngleAxis(Ogre::Radian(baseAngle + jitter), Ogre::Vector3::UNIT_Y);

                Ogre::Real fw = Ogre::Math::lerp(district.minFootprint.x, district.maxFootprint.x, d01(rng));
                Ogre::Real fd = Ogre::Math::lerp(district.minFootprint.y, district.maxFootprint.y, d01(rng));
                fw = std::min(fw, lot.size.x * 0.85f);
                fd = std::min(fd, lot.size.y * 0.85f);
                Ogre::Real fh = Ogre::Math::lerp(district.minHeight, district.maxHeight, d01(rng));
                building.footprint = Ogre::Vector3(fw, fh, fd);

                const bool isHighRise = (district.type == "Tower" || district.type == "Industrial");
                building.roofType = isHighRise ? 0u : (building.variantSeed % 3u);
                building.roofPitch = isHighRise ? 0.f : (0.2f + d01(rng) * 0.55f);

                // Route building to variant batch group by variantSeed
                const unsigned int vi = building.variantSeed % NUM_BUILDING_VARIANTS;
                const unsigned int base = vi * SLOTS_PER_VARIANT;

                this->generateBuildingGeometry(building, district, localOrigin,
                    batches[base + 0].rawVertices, batches[base + 0].rawIndices, batches[base + 0].numVertices,
                    batches[base + 1].rawVertices, batches[base + 1].rawIndices, batches[base + 1].numVertices,
                    batches[base + 2].rawVertices, batches[base + 2].rawIndices, batches[base + 2].numVertices,
                    batches[base + 3].rawVertices, batches[base + 3].rawIndices, batches[base + 3].numVertices,
                    batches[base + 4].rawVertices, batches[base + 4].rawIndices, batches[base + 4].numVertices,
                    batches[base + 5].rawVertices, batches[base + 5].rawIndices, batches[base + 5].numVertices);

                batches[base + 0].instances.push_back(building);
            }
        }

        // Roads are built BEFORE buildings (see ROOT-CAUSE FIX above) so the
        // ProceduralRoadComponent SceneNode position is already established as cityOrigin.
        // Road/curb batch slots are left empty — ProceduralRoadComponent owns the road mesh.

        // Wall generation removed — designer places walls manually

        if (this->roadComponentId != 0)
        {
            this->connectExternalRoad(blocks);
        }

        return batches;
    }

    void ProceduralCityComponent::assignDistricts(std::vector<CityBlock>& blocks)
    {
        const unsigned int n = static_cast<unsigned int>(this->districts.size());
        if (0u == n)
        {
            return;
        }
        if (1u == n)
        {
            for (auto& b : blocks)
            {
                b.districtIdx = 0;
            }
            return;
        }

        const Ogre::Vector4 bounds = this->cityBoundsAttr->getVector4();
        std::mt19937 rng(this->masterSeedAttr->getUInt() + 55555u);
        std::uniform_real_distribution<float> dx(bounds.x, bounds.z);
        std::uniform_real_distribution<float> dz(bounds.y, bounds.w);

        std::vector<Ogre::Vector2> seeds(n);
        for (unsigned int i = 0; i < n; ++i)
        {
            seeds[i] = Ogre::Vector2(dx(rng), dz(rng));
        }

        for (auto& blk : blocks)
        {
            float best = std::numeric_limits<float>::max();
            unsigned int bestIdx = 0;
            for (unsigned int i = 0; i < n; ++i)
            {
                float dx2 = blk.centre.x - seeds[i].x;
                float dz2 = blk.centre.z - seeds[i].y;
                float d = dx2 * dx2 + dz2 * dz2;
                if (d < best)
                {
                    best = d;
                    bestIdx = i;
                }
            }
            blk.districtIdx = bestIdx;
        }
    }

    void ProceduralCityComponent::subdivideBlock(CityBlock& block, unsigned int blockIdx)
    {
        if (block.districtIdx >= this->districts.size())
        {
            return;
        }
        const CityDistrict& district = this->districts[block.districtIdx];

        const Ogre::Real inset = this->roadWidthAttr->getReal() * 0.5f + this->sidewalkWidthAttr->getReal();
        const Ogre::Real usableW = block.size.x - inset * 2.f;
        const Ogre::Real usableD = block.size.y - inset * 2.f;
        if (usableW <= 1.f || usableD <= 1.f)
        {
            return;
        }

        std::mt19937 rng(this->masterSeedAttr->getUInt() + blockIdx * 7919u);
        std::uniform_real_distribution<float> d01(0.f, 1.f);

        const Ogre::Real maxFP = std::max(district.maxFootprint.x, district.maxFootprint.y);
        const int cols = std::max(1, static_cast<int>(std::ceil(usableW / maxFP)));
        const int rows = std::max(1, static_cast<int>(std::ceil(usableD / maxFP)));
        const Ogre::Real lotW = usableW / static_cast<Ogre::Real>(cols);
        const Ogre::Real lotD = usableD / static_cast<Ogre::Real>(rows);
        const Ogre::Real baseX = block.centre.x - usableW * 0.5f;
        const Ogre::Real baseZ = block.centre.z - usableD * 0.5f;

        for (int row = 0; row < rows; ++row)
        {
            for (int col = 0; col < cols; ++col)
            {
                CityLot lot;
                lot.size = Ogre::Vector2(lotW, lotD);
                lot.centre = Ogre::Vector3(baseX + (col + 0.5f) * lotW, 0.f, baseZ + (row + 0.5f) * lotD);
                lot.groundHeight = this->getGroundHeight(lot.centre);
                lot.centre.y = lot.groundHeight;
                lot.districtIdx = block.districtIdx;
                lot.occupied = (d01(rng) < district.density);
                block.lots.push_back(lot);
            }
        }
    }

    // =========================================================================
    // pushQuad — core geometry helper
    // =========================================================================

    void ProceduralCityComponent::pushQuad(std::vector<float>& verts, std::vector<Ogre::uint32>& idx, size_t& vertCount, const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& v3, const Ogre::Vector3& normal,
        Ogre::Real uLen, Ogre::Real vLen)
    {
        // 8 floats/vertex: pos.xyz + normal.xyz + uv.xy
        // Winding: v0,v1,v2,v3 are CCW when viewed from the normal direction.
        // Normal is ALWAYS passed explicitly — never computed inside this function.
        const Ogre::uint32 base = static_cast<Ogre::uint32>(vertCount);

        auto pushV = [&](const Ogre::Vector3& p, Ogre::Real u, Ogre::Real v)
        {
            verts.push_back(p.x);
            verts.push_back(p.y);
            verts.push_back(p.z);
            verts.push_back(normal.x);
            verts.push_back(normal.y);
            verts.push_back(normal.z);
            verts.push_back(u);
            verts.push_back(v);
        };

        pushV(v0, 0.f, 0.f);
        pushV(v1, uLen, 0.f);
        pushV(v2, uLen, vLen);
        pushV(v3, 0.f, vLen);

        idx.push_back(base + 0u);
        idx.push_back(base + 1u);
        idx.push_back(base + 2u);
        idx.push_back(base + 0u);
        idx.push_back(base + 2u);
        idx.push_back(base + 3u);

        vertCount += 4;
    }

    // =========================================================================
    // generateBuildingGeometry
    // All normal-direction decisions are made at the call site; pushQuad never
    // recomputes them from the cross product.
    //
    // Building coordinate basis:
    //   wallDir = orientation * UNIT_Z  (building forward, runs along block edge)
    //   right   = wallDir.crossProduct(UNIT_Y)  — NEVER swap this order
    //   UNIT_Y  = vertical (world up)
    //
    // Face normals:
    //   front (-wallDir face): normal = -wallDir   (facing street at -Z side)
    //   back  (+wallDir face): normal = +wallDir
    //   left  (-right  face): normal = -right      (from wallDir.crossProduct(UNIT_Y))
    //   right (+right  face): normal = +right
    //   top / parapet:        normal = +UNIT_Y
    //
    // Y origin fix:
    //   building.position.y is stored as 0 (XZ only).
    //   building.groundHeight holds the terrain Y at that XZ position.
    //   base = Vector3(pos.x, groundHeight - SINK_DEPTH, pos.z)
    //   Earlier bug: lot.centre.y == groundHeight was stored in position.y
    //   AND base = position + (0, groundHeight, 0) → Y appeared twice.
    // =========================================================================

    void ProceduralCityComponent::generateBuildingGeometry(const BuildingInstance& building, const CityDistrict& district, const Ogre::Vector3& localOrigin, std::vector<float>& wallV, std::vector<Ogre::uint32>& wallI, size_t& wallN, std::vector<float>& roofV,
        std::vector<Ogre::uint32>& roofI, size_t& roofN, std::vector<float>& windowV, std::vector<Ogre::uint32>& windowI, size_t& windowN, std::vector<float>& trimV, std::vector<Ogre::uint32>& trimI, size_t& trimN,
        std::vector<float>& doorV, std::vector<Ogre::uint32>& doorI, size_t& doorN,
        std::vector<float>& garageV, std::vector<Ogre::uint32>& garageI, size_t& garageN)
    {
        const Ogre::Vector3 wallDir = building.orientation * Ogre::Vector3::UNIT_Z;
        // right = wallDir.crossProduct(UNIT_Y) — must NEVER be swapped
        const Ogre::Vector3 right = wallDir.crossProduct(Ogre::Vector3::UNIT_Y);

        const Ogre::Real hw = building.footprint.x * 0.5f; // half-width
        const Ogre::Real hd = building.footprint.z * 0.5f; // half-depth
        const Ogre::Real wh = building.footprint.y;        // wall height

        // Y fix: groundHeight applied once only.  CITY_SINK_DEPTH pulls the building
        // slightly below terrain surface to close any gap on sloped ground.
        // All coordinates are LOCAL — world position minus localOrigin.
        // Same origin-subtraction pattern as ProceduralRoadComponent.
        const Ogre::Vector3 base(
            building.position.x - localOrigin.x,
            building.groundHeight - CITY_SINK_DEPTH - localOrigin.y,
            building.position.z - localOrigin.z);

        // Eight corners (bottom, then top)
        const Ogre::Vector3 flb = base - right * hw - wallDir * hd;
        const Ogre::Vector3 frb = base + right * hw - wallDir * hd;
        const Ogre::Vector3 brb = base + right * hw + wallDir * hd;
        const Ogre::Vector3 blb = base - right * hw + wallDir * hd;
        const Ogre::Vector3 flt = flb + Ogre::Vector3(0.f, wh, 0.f);
        const Ogre::Vector3 frt = frb + Ogre::Vector3(0.f, wh, 0.f);
        const Ogre::Vector3 brt = brb + Ogre::Vector3(0.f, wh, 0.f);
        const Ogre::Vector3 blt = blb + Ogre::Vector3(0.f, wh, 0.f);

        const Ogre::Real wallTile = 3.f;
        const Ogre::Real fW = building.footprint.x; // face widths
        const Ogre::Real fD = building.footprint.z;

        const Ogre::Real trimH = std::min(1.5f, wh * 0.2f);

        // ---- Wall faces (above trim) — slot 0 --------------------------------
        // Front face: v0=flb+trim v1=frb+trim v2=frt v3=flt, normal = -wallDir
        this->pushQuad(wallV, wallI, wallN, flb + Ogre::Vector3(0.f, trimH, 0.f), frb + Ogre::Vector3(0.f, trimH, 0.f), frt, flt, -wallDir, fW / wallTile, (wh - trimH) / wallTile);

        // Back face: v0=brb+trim v1=blb+trim v2=blt v3=brt, normal = +wallDir
        this->pushQuad(wallV, wallI, wallN, brb + Ogre::Vector3(0.f, trimH, 0.f), blb + Ogre::Vector3(0.f, trimH, 0.f), blt, brt, wallDir, fW / wallTile, (wh - trimH) / wallTile);

        // Left face: v0=blb+trim v1=flb+trim v2=flt v3=blt, normal = -right
        // Direction v0->v1 = flb - blb = -wallDir (correct faceRight for windows)
        this->pushQuad(wallV, wallI, wallN, blb + Ogre::Vector3(0.f, trimH, 0.f), flb + Ogre::Vector3(0.f, trimH, 0.f), flt, blt, -right, fD / wallTile, (wh - trimH) / wallTile);

        // Right face: v0=frb+trim v1=brb+trim v2=brt v3=frt, normal = +right
        // Direction v0->v1 = brb - frb = +wallDir (correct faceRight for windows)
        this->pushQuad(wallV, wallI, wallN, frb + Ogre::Vector3(0.f, trimH, 0.f), brb + Ogre::Vector3(0.f, trimH, 0.f), brt, frt, right, fD / wallTile, (wh - trimH) / wallTile);

        // Parapet cap — normal = +UNIT_Y
        this->pushQuad(wallV, wallI, wallN, flt, frt, brt, blt, Ogre::Vector3::UNIT_Y, fW / wallTile, fD / wallTile);

        // ---- Trim strip — slot 3 (ground-level band) -------------------------
        this->pushQuad(trimV, trimI, trimN, flb, frb, frb + Ogre::Vector3(0.f, trimH, 0.f), flb + Ogre::Vector3(0.f, trimH, 0.f), -wallDir, fW / 1.5f, 1.f);
        this->pushQuad(trimV, trimI, trimN, brb, blb, blb + Ogre::Vector3(0.f, trimH, 0.f), brb + Ogre::Vector3(0.f, trimH, 0.f), wallDir, fW / 1.5f, 1.f);
        this->pushQuad(trimV, trimI, trimN, blb, flb, flb + Ogre::Vector3(0.f, trimH, 0.f), blb + Ogre::Vector3(0.f, trimH, 0.f), -right, fD / 1.5f, 1.f);
        this->pushQuad(trimV, trimI, trimN, frb, brb, brb + Ogre::Vector3(0.f, trimH, 0.f), frb + Ogre::Vector3(0.f, trimH, 0.f), right, fD / 1.5f, 1.f);

        // ---- Roof — slot 1 ---------------------------------------------------
        const Ogre::Real roofH = hw * 0.5f + building.roofPitch * hw;

        if (building.roofType == 0 || wh > 15.f)
        {
            // Flat: already capped by parapet
        }
        else if (building.roofType == 1)
        {
            // Gabled: ridge along wallDir
            const Ogre::Vector3 ridge = Ogre::Vector3(base.x, base.y + wh + roofH, base.z);
            const Ogre::Real ridgeHL = hd * 0.6f;
            const Ogre::Vector3 ridgeF = ridge - wallDir * ridgeHL;
            const Ogre::Vector3 ridgeB = ridge + wallDir * ridgeHL;

            Ogre::Vector3 sNL = (-right + Ogre::Vector3(0.f, 1.f, 0.f)).normalisedCopy();
            Ogre::Vector3 sNR = (right + Ogre::Vector3(0.f, 1.f, 0.f)).normalisedCopy();
            this->pushQuad(roofV, roofI, roofN, flt, ridgeF, ridgeB, blt, sNL, fD / wallTile, 1.f);
            this->pushQuad(roofV, roofI, roofN, ridgeF, frt, brt, ridgeB, sNR, fD / wallTile, 1.f);
            // Gable ends (triangle as degenerate quad — collapse v2==v3)
            this->pushQuad(roofV, roofI, roofN, flt, frt, ridgeF, ridgeF, -wallDir, fW / wallTile, 1.f);
            this->pushQuad(roofV, roofI, roofN, brt, blt, ridgeB, ridgeB, wallDir, fW / wallTile, 1.f);
        }
        else
        {
            // Hip: four slopes converging to a single apex
            const Ogre::Vector3 apex(base.x, base.y + wh + roofH, base.z);
            Ogre::Vector3 sNF = (-wallDir + Ogre::Vector3(0.f, 1.f, 0.f)).normalisedCopy();
            Ogre::Vector3 sNB = (wallDir + Ogre::Vector3(0.f, 1.f, 0.f)).normalisedCopy();
            Ogre::Vector3 sNL = (-right + Ogre::Vector3(0.f, 1.f, 0.f)).normalisedCopy();
            Ogre::Vector3 sNR = (right + Ogre::Vector3(0.f, 1.f, 0.f)).normalisedCopy();
            this->pushQuad(roofV, roofI, roofN, flt, frt, apex, apex, sNF, fW / wallTile, 1.f);
            this->pushQuad(roofV, roofI, roofN, brt, blt, apex, apex, sNB, fW / wallTile, 1.f);
            this->pushQuad(roofV, roofI, roofN, blt, flt, apex, apex, sNL, fD / wallTile, 1.f);
            this->pushQuad(roofV, roofI, roofN, frt, brt, apex, apex, sNR, fD / wallTile, 1.f);
        }

        // ---- Windows — slot 2 ------------------------------------------------
        // Windows placed as inset quads slightly in front of each wall face.
        // FIX: faceRight must match the v0->v1 direction of the corresponding wall
        // quad so windows are placed WITHIN the face, not outside it:
        //   front face v0->v1 = frb-flb = +right        → faceOriginBL=flb, faceRight=+right
        //   back  face v0->v1 = blb-brb = -right        → faceOriginBL=brb, faceRight=-right
        //   left  face v0->v1 = flb-blb = -wallDir      → faceOriginBL=blb, faceRight=-wallDir
        //   right face v0->v1 = brb-frb = +wallDir      → faceOriginBL=frb, faceRight=+wallDir
        const bool isIndustrial = (district.type == "Industrial");
        if (false == isIndustrial && wh > 4.f)
        {
            const Ogre::Real winW = 1.3f;
            const Ogre::Real winH = 1.8f;
            const Ogre::Real spacingX = 2.5f;
            const Ogre::Real spacingZ = 3.0f;
            const Ogre::Real winOff = 0.04f;
            // Window start Y must be ABOVE the door, not just above the trim band.
        // Otherwise windows overlap the door on the front face.
        // Use the same door height formula as the door slot so they always match.
        const Ogre::Real doorHInner = std::min(2.2f, wh * 0.45f);
        const Ogre::Real winStartY = std::max(trimH, doorHInner + 0.5f);  // 0.5m gap above door
        const Ogre::Real availH = wh - winStartY - 0.5f;

            auto addWindowRow = [&](const Ogre::Vector3& originBL, const Ogre::Vector3& faceRight, const Ogre::Vector3& faceNormal, Ogre::Real faceW, Ogre::Real faceH)
            {
                if (faceW < spacingX || faceH < spacingZ)
                {
                    return;
                }
                const int cols = std::max(1, static_cast<int>(faceW / spacingX));
                const int rows = std::max(1, static_cast<int>(faceH / spacingZ));
                const Ogre::Real stepX = faceW / static_cast<Ogre::Real>(cols);
                const Ogre::Real stepY = faceH / static_cast<Ogre::Real>(rows);
                const Ogre::Vector3 pushOut = faceNormal * winOff;

                for (int r = 0; r < rows; ++r)
                {
                    for (int c = 0; c < cols; ++c)
                    {
                        const Ogre::Real cx = (c + 0.5f) * stepX;
                        const Ogre::Real cy = winStartY + (r + 0.5f) * stepY;
                        const Ogre::Vector3 wc = originBL + faceRight * cx + Ogre::Vector3(0.f, cy, 0.f) + pushOut;
                        const Ogre::Vector3 wv0 = wc - faceRight * (winW * 0.5f) - Ogre::Vector3(0.f, winH * 0.5f, 0.f);
                        const Ogre::Vector3 wv1 = wc + faceRight * (winW * 0.5f) - Ogre::Vector3(0.f, winH * 0.5f, 0.f);
                        const Ogre::Vector3 wv2 = wc + faceRight * (winW * 0.5f) + Ogre::Vector3(0.f, winH * 0.5f, 0.f);
                        const Ogre::Vector3 wv3 = wc - faceRight * (winW * 0.5f) + Ogre::Vector3(0.f, winH * 0.5f, 0.f);
                        this->pushQuad(windowV, windowI, windowN, wv0, wv1, wv2, wv3, faceNormal, 1.f, 1.f);
                    }
                }
            };

            // Correct faceRight per face — matches v0->v1 direction of each wall pushQuad
            addWindowRow(flb, right, -wallDir, fW, availH);  // front
            addWindowRow(brb, -right, wallDir, fW, availH);  // back
            addWindowRow(blb, -wallDir, -right, fD, availH); // left  (FIX: was +wallDir)
            addWindowRow(frb, wallDir, right, fD, availH);   // right (FIX: was -wallDir)
        }

        // ---- Door — slot 4 ---------------------------------------------------
        // One door per building, centred on the front face (-wallDir side) at ground level.
        // FIX height: doorH was min(2.2, trimH*0.95) → for wh=4m that gives only 0.76m
        // (knee height!). Now: doorH = min(2.2, wh*0.45) so a 4m building gets 1.8m, 9m→2.2m.
        {
            const Ogre::Real doorH = std::min(2.2f, wh * 0.45f);    // always a reasonable door height
            const Ogre::Real doorW = std::min(1.2f, fW * 0.25f);
            const Ogre::Real doorOff = 0.05f;
            const Ogre::Vector3 pushOut = (-wallDir) * doorOff;

            // World XZ of the front face centre
            const Ogre::Real frontWorldX = building.position.x + (-wallDir.x) * hd;
            const Ogre::Real frontWorldZ = building.position.z + (-wallDir.z) * hd;
            const Ogre::Real frontTerrainY = this->getGroundHeight(
                Ogre::Vector3(frontWorldX, 0.f, frontWorldZ));
            // Door base in local space: never lower than the actual front-face terrain
            const Ogre::Real doorBaseLocal = std::max(
                base.y,
                frontTerrainY - CITY_SINK_DEPTH - localOrigin.y);

            const Ogre::Vector3 frontBaseCentre = Ogre::Vector3(
                base.x + (-wallDir.x) * hd,
                doorBaseLocal,
                base.z + (-wallDir.z) * hd);

            const Ogre::Vector3 dc = frontBaseCentre
                + right * 0.f                                        // centred
                + Ogre::Vector3(0.f, doorH * 0.5f, 0.f)            // mid-height
                + pushOut;

            const Ogre::Vector3 dv0 = dc - right*(doorW*0.5f) - Ogre::Vector3(0.f, doorH*0.5f, 0.f);
            const Ogre::Vector3 dv1 = dc + right*(doorW*0.5f) - Ogre::Vector3(0.f, doorH*0.5f, 0.f);
            const Ogre::Vector3 dv2 = dc + right*(doorW*0.5f) + Ogre::Vector3(0.f, doorH*0.5f, 0.f);
            const Ogre::Vector3 dv3 = dc - right*(doorW*0.5f) + Ogre::Vector3(0.f, doorH*0.5f, 0.f);
            this->pushQuad(doorV, doorI, doorN, dv0, dv1, dv2, dv3, -wallDir, 1.f, 1.f);
        }

        // ---- Garage — slot 5 (city_garage_01 datablock) -----------------------
        // Only on Residential buildings, 30% chance (seeded), not on very small lots.
        // The garage is a low rectangular box attached to the RIGHT face of the house,
        // extending outward by garageDept.  Walls use the wall slot (existing),
        // the outward-facing garage door quad uses this dedicated garage slot.
        // Winding/normal rules are identical to the house faces — explicit normals.
        // ---- Garage — slot 5 (city_garage_01 datablock) ----------------------
        // Garage is attached DIRECTLY to the RIGHT face of the house (no gap),
        // at the FRONT of the house so the door faces the street.
        //
        // Space check: garage extends gd meters to the right of the house right wall.
        // If gd > hw (building half-width) the garage would overflow the lot → skip.
        //
        // Orientation reference (for normal/winding derivation, all CCW from outside):
        //   wallDir = +Z (depth axis, front face at -wallDir*hd)
        //   right   = +X (width axis)
        //   Y       = +Y (up)
        //
        // Derived normals:
        //   Door (front, -wallDir):  e1=Y  e2=right → Y×right=Y×X=-Z = -wallDir ✓
        //   Left wall (-right):      e1=Y  e2=wallDir → Y×Z = -X = -right ✓
        //   Right wall (+right):     e1=Y  e2=wallDir → same as left but reverse order → +right ✓
        //   Back wall (+wallDir):    e1=right e2=Y → X×Y = Z = +wallDir ✓
        //   Roof (+Y):               e1=wallDir e2=right → Z×X = Y ✓
        const bool isGarageType  = (district.type == "Residential_Low" ||
                                     district.type == "Residential_Mid"  ||
                                     district.type == "Mixed");
        const bool generateGarage = this->generateGarageAttr->getBool();

        // Space check: garage extends gd meters to the right of the house right wall.
        // building.lotHalfRight is the lot's half-width in the right direction (from the
        // building's world position ≈ lot center to the road boundary).
        // Available space = lotHalfRight - hw (building already occupies hw from center).
        // Use 1m safety margin so the garage never touches the sidewalk edge.
        const Ogre::Real spaceRight = std::max(0.f, building.lotHalfRight - hw - 1.0f);
        const Ogre::Real gd         = std::min({4.0f, spaceRight, fW * 0.4f});

        const bool hasGarage = generateGarage && isGarageType &&
                               ((building.variantSeed % 10u) < 3u) &&
                               (gd >= 2.0f) && (fD > 5.f);
        if (hasGarage)
        {
            const Ogre::Real gw = std::min(4.5f, fD * 0.45f);   // garage width (wallDir, from front)
            const Ogre::Real gh = std::min(2.8f, wh * 0.4f);    // garage height (lower than house)

            // Corner positions in local space.
            // Garage is at the FRONT of the house (at -wallDir*hd), extending +wallDir*gw inward.
            // FL/FR = front (near street), BL/BR = back (away from street).
            // NL/NR subscript: N=Near house wall (right*hw), F=Far (right*(hw+gd)).
            const Ogre::Vector3 FL  = base + right * hw        + (-wallDir) * hd;
            const Ogre::Vector3 FR  = base + right * (hw + gd) + (-wallDir) * hd;
            const Ogre::Vector3 BL  = base + right * hw        + (-wallDir) * (hd - gw);
            const Ogre::Vector3 BR  = base + right * (hw + gd) + (-wallDir) * (hd - gw);
            const Ogre::Vector3 TFL = FL  + Ogre::Vector3(0.f, gh, 0.f);
            const Ogre::Vector3 TFR = FR  + Ogre::Vector3(0.f, gh, 0.f);
            const Ogre::Vector3 TBL = BL  + Ogre::Vector3(0.f, gh, 0.f);
            const Ogre::Vector3 TBR = BR  + Ogre::Vector3(0.f, gh, 0.f);

            // 1. Garage DOOR (visible from street = -wallDir side)
            // Cross(FR-FL, TFR-FL) = Cross(right*gd, right*gd+Y*gh) = gd*gh*(right×Y) = +wallDir = -(-wallDir) ✓
            this->pushQuad(garageV, garageI, garageN,
                FL, FR, TFR, TFL, -wallDir, gd / wallTile, gh / wallTile);

            // 2. Left wall (visible from -right side = house side)
            // Cross(FL-BL, TFL-BL) = Cross(-wallDir*gw, -wallDir*gw+Y*gh) = gw*gh*(−wallDir×Y)=gw*gh*right = -(-right) ✓
            this->pushQuad(wallV, wallI, wallN,
                BL, FL, TFL, TBL, -right, gw / wallTile, gh / wallTile);

            // 3. Right wall (visible from +right side = outer side)
            // Cross(BR-FR, TBR-FR) = Cross(+wallDir*gw, wallDir*gw+Y*gh) = gw*gh*(wallDir×Y) = -right = -(+right) ✓
            this->pushQuad(wallV, wallI, wallN,
                FR, BR, TBR, TFR, right, gw / wallTile, gh / wallTile);

            // 4. Back wall (visible from +wallDir side = away from street)
            // Cross(BL-BR, TBL-BR) = Cross(-right*gd, -right*gd+Y*gh) = gd*gh*(-right×Y) = -wallDir = -(+wallDir) ✓
            this->pushQuad(wallV, wallI, wallN,
                BR, BL, TBL, TBR, wallDir, gd / wallTile, gh / wallTile);

            // 5. Roof (visible from +Y side)
            // Cross(TFR-TFL, TBR-TFL) = Cross(right*gd, right*gd+wallDir*gw) = gd*gw*(right×wallDir) = -Y ✓
            this->pushQuad(wallV, wallI, wallN,
                TFL, TFR, TBR, TBL, Ogre::Vector3::UNIT_Y, gd / wallTile, gw / wallTile);
        }
    }

    // =========================================================================
    // buildCityRoadNetwork — drives ProceduralRoadComponent for city roads
    // =========================================================================

    void ProceduralCityComponent::buildCityRoadNetwork(const std::vector<CityBlock>& blocks)
    {
        auto roadCompPtr = NOWA::makeStrongPtr(
            this->gameObjectPtr->getComponent<ProceduralRoadComponent>());
        if (nullptr == roadCompPtr)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                "[ProceduralCityComponent] buildCityRoadNetwork: ProceduralRoadComponent not found "
                "on this GameObject. Check autoComponents in getStaticTypeDescriptor().");
            return;
        }
        ProceduralRoadComponent* roadComp = roadCompPtr.get();

        roadComp->setRoadWidth(this->roadWidthAttr->getReal());
        roadComp->setCenterDatablock(this->roadDatablockAttr->getString());
        roadComp->setEdgeDatablock(this->curbDatablockAttr->getString());

        // FIX 1: Set heightOffset to 0 before adding segments.
        // ProceduralRoadComponent::getGroundHeight() returns hitPoint.y + heightOffset.
        // Default heightOffset = 0.1f causes a staircase: each terrain subdivision
        // intermediate point is 0.1m above actual terrain, so the road surface is raised.
        //
        // CRITICAL: do NOT use setHeightOffset() here — it calls rebuildMesh() internally
        // (line 6560 of ProceduralRoadComponent.cpp), which would rebuild the COMPLETED
        // road with the restored non-zero offset AFTER endBatch(), re-introducing the bug.
        //
        // Instead, write the Variant value directly to bypass the rebuild trigger,
        // and leave heightOffset at 0 permanently for city roads (it is the correct value:
        // city roads should hug the terrain, not float 0.1m above it).
        const Ogre::Real savedHeightOffset = roadComp->getHeightOffset();
        if (savedHeightOffset != 0.0f)
        {
            // Write directly to the underlying Variant to avoid triggering rebuildMesh()
            // which setHeightOffset() would cause.  The value is restored the same way.
            roadComp->getAttribute(ProceduralRoadComponent::AttrHeightOffset())->setValue(0.0f);
        }

        // FIX 2: Always clear before re-generating.
        // clearAllSegments() early-outs if roadSegments is empty, so it is safe to call
        // unconditionally.  It also resets hasRoadOrigin so the next first segment
        // correctly establishes a new road origin.
        roadComp->clearAllSegments();

        // Use the PERTURBED grid computed in generateCityLayout (stored in cityGridX/cityGridZ).
        // If not yet computed (e.g., first call before layout), fall back to base grid.
        if (this->cityGridX.size() < 2 || this->cityGridZ.size() < 2)
        {
            const Ogre::Vector4 cityBnds2 = this->cityBoundsAttr->getVector4();
            const Ogre::Real blockSz2     = this->blockSizeAttr->getReal();
            this->cityGridX.clear();
            this->cityGridZ.clear();
            for (Ogre::Real gx = cityBnds2.x; gx <= cityBnds2.z + blockSz2 * 0.01f; gx += blockSz2)
                this->cityGridX.push_back(gx);
            for (Ogre::Real gz = cityBnds2.y; gz <= cityBnds2.w + blockSz2 * 0.01f; gz += blockSz2)
                this->cityGridZ.push_back(gz);
        }
        const std::vector<Ogre::Real>& gridX = this->cityGridX;
        const std::vector<Ogre::Real>& gridZ = this->cityGridZ;
        const Ogre::Real blockSz = this->blockSizeAttr->getReal();

        // Batch mode: defers rebuildMesh() until endBatch() so junction detection runs
        // ONCE with all segments present, AND no road mesh exists during getGroundHeight
        // calls — eliminating the staircase-height bug even without the heightOffset=0 fix.
        // Requires ProceduralRoadComponent_BatchMode_patch.cpp to be applied first.
        // ALL grid lines are generated (outer boundary included).
        //
        // WHY we must include outer boundary roads:
        // Junction detection (line 1489 of ProceduralRoadComponent.cpp) requires
        // distinct.size() >= 3 at a shared endpoint to create a junction patch.
        // Example: without the outer vertical strips, the point (-60, 0, -60) where
        // horizontal strip A ends and strip B starts has only 2 distinct segments
        // → no junction patch → raw road end-caps face each other → visible V-gap.
        // With outer vertical strips present, every grid crossing has 3+ segments
        // meeting: only the 4 outermost corners have 2 endpoints → end-caps there,
        // which is acceptable (road ends at city boundary).
        // ---- Terrain sample interval -----------------------------------------
        // Shorter interval = more terrain subdivision points per segment = better
        // gradient tracking on hills (default ~5m; 2m follows every bump cleanly).
        auto* terrainIntervalAttr = roadComp->getAttribute(ProceduralRoadComponent::AttrTerrainSampleInterval());
        Ogre::Real savedTerrainInterval = 5.0f;
        if (nullptr != terrainIntervalAttr)
        {
            savedTerrainInterval = terrainIntervalAttr->getReal();
            terrainIntervalAttr->setValue(2.0f);
        }

        const Ogre::Real variance = this->roadVarianceAttr->getReal();

        // Road variance: when > 0, add diagonal shortcut roads in addition to the grid.
        // Diagonals connect EXISTING grid junction points so the junction detection
        // (which requires 3+ segment endpoints at the same position) correctly creates
        // patches at every crossing.  Number of diagonals scales with variance:
        //   0.0 → none, 0.25 → 1, 0.5 → 2, 0.75 → 3, 1.0 → 4
        // The Catmull-Rom smooth-curve feature still applies if an inner grid segment
        // has its midpoint slightly offset (the midpoint approach from previous code).
        roadComp->beginBatch();

        // Skip the outermost boundary grid lines.
        const size_t zFirst = (gridZ.size() > 2) ? 1u : 0u;
        const size_t zLast  = (gridZ.size() > 2) ? gridZ.size() - 1u : gridZ.size();
        for (size_t zi = zFirst; zi < zLast; ++zi)
        {
            const Ogre::Real zVal = gridZ[zi];
            for (size_t xi = 0; xi + 1 < gridX.size(); ++xi)
            {
                roadComp->addRoadSegmentLua(
                    Ogre::Vector3(gridX[xi],     0.f, zVal),
                    Ogre::Vector3(gridX[xi + 1], 0.f, zVal));
            }
        }

        const size_t xFirst = (gridX.size() > 2) ? 1u : 0u;
        const size_t xLast  = (gridX.size() > 2) ? gridX.size() - 1u : gridX.size();
        for (size_t xi = xFirst; xi < xLast; ++xi)
        {
            const Ogre::Real xVal = gridX[xi];
            for (size_t zi = 0; zi + 1 < gridZ.size(); ++zi)
            {
                roadComp->addRoadSegmentLua(
                    Ogre::Vector3(xVal, 0.f, gridZ[zi]),
                    Ogre::Vector3(xVal, 0.f, gridZ[zi + 1]));
            }
        }

        // ---- Diagonal roads ---------------------------------------------------
        // Each diagonal connects two non-adjacent inner grid junction points so
        // junction detection sees 5+ arms at both endpoints → proper patches.
        // Key constraint: both dx AND dz must be non-zero (true diagonal).
        // If dx==0 the road is vertical — identical to an existing grid road.
        // If dz==0 the road is horizontal — identical to an existing grid road.
        if (variance > 0.01f && gridX.size() >= 4 && gridZ.size() >= 4)
        {
            const size_t numDiags = static_cast<size_t>(variance * 3.f) + 1u;
            uint32_t seed = this->masterSeedAttr->getUInt() ^ 0xC17Bu;

            // Build list of inner junction points
            std::vector<std::pair<size_t,size_t>> jpts;
            for (size_t zi = zFirst; zi < zLast; ++zi)
            {
                for (size_t xi = xFirst; xi < xLast; ++xi)
                {
                    jpts.push_back(std::make_pair(xi, zi));
                }
            }

            for (size_t di = 0; di < numDiags && jpts.size() >= 4; ++di)
            {
                // Find pair (ia, ib) that is a TRUE DIAGONAL (dx≠0 AND dz≠0) and long enough
                seed = seed * 1664525u + 1013904223u;
                const size_t ia = seed % jpts.size();
                seed = seed * 1664525u + 1013904223u;
                size_t ib = seed % jpts.size();

                unsigned int attempts = 0;
                while (attempts < 64)
                {
                    const int dx = static_cast<int>(jpts[ib].first)  - static_cast<int>(jpts[ia].first);
                    const int dz = static_cast<int>(jpts[ib].second) - static_cast<int>(jpts[ia].second);
                    // Must be diagonal (both non-zero) and span at least 2 grid cells in each direction
                    if (dx != 0 && dz != 0 && std::abs(dx) >= 2 && std::abs(dz) >= 2) { break; }
                    seed = seed * 1664525u + 1013904223u;
                    ib = seed % jpts.size();
                    ++attempts;
                }

                const Ogre::Real sx = gridX[jpts[ia].first];
                const Ogre::Real sz = gridZ[jpts[ia].second];
                const Ogre::Real ex = gridX[jpts[ib].first];
                const Ogre::Real ez = gridZ[jpts[ib].second];
                if (std::abs(ex - sx) > 0.1f && std::abs(ez - sz) > 0.1f)
                {
                    roadComp->addRoadSegmentLua(Ogre::Vector3(sx, 0.f, sz), Ogre::Vector3(ex, 0.f, ez));
                }
            }
        }

        roadComp->endBatch();

        // Restore terrain interval
        if (nullptr != terrainIntervalAttr)
        {
            terrainIntervalAttr->setValue(savedTerrainInterval);
        }

        // Restore the height offset via direct Variant write (same reason as above:
        // setHeightOffset() would trigger rebuildMesh() on the finished road, rebuilding
        // all terrain heights with the non-zero offset and re-introducing the staircase).
        if (savedHeightOffset != 0.0f)
        {
            roadComp->getAttribute(ProceduralRoadComponent::AttrHeightOffset())->setValue(savedHeightOffset);
        }

        const unsigned int numSegs = static_cast<unsigned int>(
            gridZ.size() * (gridX.size() > 0 ? gridX.size() - 1 : 0) +
            gridX.size() * (gridZ.size() > 0 ? gridZ.size() - 1 : 0));
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
            "[ProceduralCityComponent] Road grid: " + Ogre::StringConverter::toString(numSegs) +
            " segments. heightOffset=0 during generation (restored to " +
            Ogre::StringConverter::toString(savedHeightOffset) + "). "
            "All grid lines included: every mid-grid point has 3+ segments → junction patches, no gaps.");
    }

    // =========================================================================
    // generateRoadGeometry — KEPT FOR REFERENCE but no longer called.
    // Replaced by buildCityRoadNetwork() above.
    // FIX record: single midpoint raycast per 40m strip caused roads to clip through
    // houses and terrain; tessellation + ProceduralRoadComponent approach is better.
    // =========================================================================

    void ProceduralCityComponent::generateRoadGeometry(const std::vector<CityBlock>& blocks, const Ogre::Vector3& localOrigin, std::vector<float>& roadV, std::vector<Ogre::uint32>& roadI, size_t& roadN, std::vector<float>& curbV, std::vector<Ogre::uint32>& curbI, size_t& curbN)
    {
        if (blocks.empty())
        {
            return;
        }

        const Ogre::Real rw     = this->roadWidthAttr->getReal();
        const Ogre::Real curbH  = this->curbHeightAttr->getReal();
        const Ogre::Real curbW  = 0.4f;
        const Ogre::Real tile   = 4.f;

        // RoadWidth is interpreted as the TOTAL road width, so half-width = rw/2.
        // Using rw directly as half-width made roads 2× too wide (12m for rw=6)
        // and caused buildings' inset calculation (which uses rw*0.5 as half-road) to
        // disagree with the actual road geometry.
        const Ogre::Real halfRw = rw * 0.5f;

        // Use the shared perturbed grid (same grid used by buildCityRoadNetwork and block generation)
        const std::vector<Ogre::Real>& gridX = this->cityGridX;
        const std::vector<Ogre::Real>& gridZ = this->cityGridZ;
        const Ogre::Real blockSz = this->blockSizeAttr->getReal();
        if (gridX.size() < 2 || gridZ.size() < 2) { return; }

        const Ogre::Real ox = localOrigin.x;
        const Ogre::Real oy = localOrigin.y;
        const Ogre::Real oz = localOrigin.z;

        // Terrain-following step and lift (same as before)
        static constexpr Ogre::Real TERRAIN_STEP = 3.0f;
        // Road lift: clearly above terrain so the designer can sculpt Terra up to meet
        // the road surface using the heightmap brush. 0.5m is large enough to prevent
        // z-fighting at any camera distance while remaining visually manageable.
        static constexpr Ogre::Real ROAD_LIFT      = 0.5f;
        static constexpr Ogre::Real INTERSECT_EXTRA = 0.03f;  // intersection sits slightly higher to cover strip overlap
        // Both h0, h1 are already in LOCAL space (world - oy + LIFT).
        // Winding: BL→TL→TR→BR is CCW from +Y, giving a +Y-ish normal for flat and a slightly
        // tilted normal for sloped sub-quads.  Normal is computed from actual geometry, not
        // assumed to be UNIT_Y, so lighting is correct on slopes.
        auto emitRoadQuad = [&](Ogre::Real x0l, Ogre::Real x1l, Ogre::Real h0, Ogre::Real h1,
                                Ogre::Real z0l, Ogre::Real z1l, bool alongX, Ogre::Real subLen)
        {
            // BL, TL, TR, BR — all in local space
            // "B" = near edge (z0l or fixed), "T" = far edge (z1l or fixed)
            // "L" = start (x0l or fixed), "R" = end (x1l or fixed)
            Ogre::Vector3 BL, TL, TR, BR;
            if (alongX)
            {
                // road runs along X: h0 at x0l, h1 at x1l; z is fixed ±rw
                BL = Ogre::Vector3(x0l, h0, z0l);
                TL = Ogre::Vector3(x0l, h0, z1l);
                TR = Ogre::Vector3(x1l, h1, z1l);
                BR = Ogre::Vector3(x1l, h1, z0l);
            }
            else
            {
                // road runs along Z: h0 at z0l, h1 at z1l; x is fixed ±rw
                BL = Ogre::Vector3(x0l, h0, z0l);
                TL = Ogre::Vector3(x1l, h0, z0l);  // width edge at h0
                TR = Ogre::Vector3(x1l, h1, z1l);
                BR = Ogre::Vector3(x0l, h1, z1l);
            }

            // Compute actual normal from the (sloped) quad
            // First triangle: BL, TL, TR → normal = (TL-BL) × (TR-BL)
            const Ogre::Vector3 e1 = TL - BL;
            const Ogre::Vector3 e2 = TR - BL;
            Ogre::Vector3 n = e1.crossProduct(e2);
            if (n.y < 0.f) { n = -n; }  // always face upward (failsafe for degenerate cases)
            if (n.squaredLength() > 0.f) { n.normalise(); }
            else { n = Ogre::Vector3::UNIT_Y; }

            this->pushQuad(roadV, roadI, roadN, BL, TL, TR, BR, n, rw / tile, subLen / tile);
        };

        // Helper: emit one curb face (vertical strip at edge of road)
        auto emitCurb = [&](const Ogre::Vector3& cBL, const Ogre::Vector3& cBR,
                             const Ogre::Vector3& cTR, const Ogre::Vector3& cTL,
                             const Ogre::Vector3& cN, Ogre::Real uLen)
        {
            if (curbH <= 0.001f) { return; }
            this->pushQuad(curbV, curbI, curbN, cBL, cTL, cTR, cBR, cN, uLen / tile, 1.f);
        };

        // ---- Horizontal strips (road runs along X, at constant Z) ----
        for (Ogre::Real zVal : gridZ)
        {
            for (size_t xi = 0; xi + 1 < gridX.size(); ++xi)
            {
                const Ogre::Real x0 = gridX[xi];
                const Ogre::Real x1 = gridX[xi + 1];

                // Trim the strip so it stops halfRw short of each intersection quad.
                // Without trimming, horizontal and vertical strips overlap at every
                // crossing, producing a double layer of geometry that flickers.
                // The intersection quad fills the gap exactly.
                // No trimming — strips run full length (x0 to x1).
                // Intersections are raised slightly above the strip surface so they
                // always cover the small overlap zone cleanly, with no height discontinuity.
                const Ogre::Real segLen = x1 - x0;

                const int numPts = std::max(2, static_cast<int>(std::ceil(segLen / TERRAIN_STEP)) + 1);

                // Sample terrain heights at evenly-spaced points along the strip
                std::vector<Ogre::Real> heights(static_cast<size_t>(numPts));
                for (int pi = 0; pi < numPts; ++pi)
                {
                    const Ogre::Real px = x0 + static_cast<Ogre::Real>(pi) / static_cast<Ogre::Real>(numPts - 1) * segLen;
                    heights[static_cast<size_t>(pi)] = this->getGroundHeight(Ogre::Vector3(px, 0.f, zVal)) - oy + ROAD_LIFT;
                }
                // Single smooth pass: averages each interior sample with its neighbours.
                // Prevents visible steps from terrain micro-bumps under the road surface.
                if (numPts > 2)
                {
                    for (int pi = 1; pi < numPts - 1; ++pi)
                    {
                        heights[static_cast<size_t>(pi)] = (heights[static_cast<size_t>(pi - 1)] + heights[static_cast<size_t>(pi)] * 2.f + heights[static_cast<size_t>(pi + 1)]) * 0.25f;
                    }
                }

                for (int si = 0; si + 1 < numPts; ++si)
                {
                    const Ogre::Real subX0 = x0 + static_cast<Ogre::Real>(si)     / static_cast<Ogre::Real>(numPts - 1) * segLen;
                    const Ogre::Real subX1 = x0 + static_cast<Ogre::Real>(si + 1) / static_cast<Ogre::Real>(numPts - 1) * segLen;
                    const Ogre::Real h0 = heights[static_cast<size_t>(si)];
                    const Ogre::Real h1 = heights[static_cast<size_t>(si + 1)];
                    const Ogre::Real subLen = subX1 - subX0;

                    emitRoadQuad(subX0-ox, subX1-ox, h0, h1, zVal-oz - halfRw, zVal-oz + halfRw, true, subLen);

                    if (curbH > 0.001f)
                    {
                        // Near curb (-Z side, normal = -Z)
                        emitCurb(
                            Ogre::Vector3(subX0-ox, h0, zVal-oz - halfRw - curbW),
                            Ogre::Vector3(subX1-ox, h1, zVal-oz - halfRw - curbW),
                            Ogre::Vector3(subX1-ox, h1 + curbH, zVal-oz - halfRw),
                            Ogre::Vector3(subX0-ox, h0 + curbH, zVal-oz - halfRw),
                            Ogre::Vector3(0.f, 0.f, -1.f), subLen);
                        // Far curb (+Z side, normal = +Z)
                        emitCurb(
                            Ogre::Vector3(subX0-ox, h0, zVal-oz + halfRw),
                            Ogre::Vector3(subX1-ox, h1, zVal-oz + halfRw),
                            Ogre::Vector3(subX1-ox, h1 + curbH, zVal-oz + halfRw + curbW),
                            Ogre::Vector3(subX0-ox, h0 + curbH, zVal-oz + halfRw + curbW),
                            Ogre::Vector3(0.f, 0.f, 1.f), subLen);
                    }
                }
            }
        }

        // ---- Vertical strips (road runs along Z, at constant X) ----
        for (Ogre::Real xVal : gridX)
        {
            for (size_t zi = 0; zi + 1 < gridZ.size(); ++zi)
            {
                const Ogre::Real z0 = gridZ[zi];
                const Ogre::Real z1 = gridZ[zi + 1];

                // Same trimming as horizontal: stop halfRw short of each intersection
                // No trimming — same as horizontal (see above)
                const Ogre::Real segLen = z1 - z0;

                const int numPts = std::max(2, static_cast<int>(std::ceil(segLen / TERRAIN_STEP)) + 1);

                std::vector<Ogre::Real> heights(static_cast<size_t>(numPts));
                for (int pi = 0; pi < numPts; ++pi)
                {
                    const Ogre::Real pz = z0 + static_cast<Ogre::Real>(pi) / static_cast<Ogre::Real>(numPts - 1) * segLen;
                    heights[static_cast<size_t>(pi)] = this->getGroundHeight(Ogre::Vector3(xVal, 0.f, pz)) - oy + ROAD_LIFT;
                }
                // Same smoothing as horizontal strips
                if (numPts > 2)
                {
                    for (int pi = 1; pi < numPts - 1; ++pi)
                    {
                        heights[static_cast<size_t>(pi)] = (heights[static_cast<size_t>(pi - 1)] + heights[static_cast<size_t>(pi)] * 2.f + heights[static_cast<size_t>(pi + 1)]) * 0.25f;
                    }
                }

                for (int si = 0; si + 1 < numPts; ++si)
                {
                    const Ogre::Real subZ0 = z0 + static_cast<Ogre::Real>(si)     / static_cast<Ogre::Real>(numPts - 1) * segLen;
                    const Ogre::Real subZ1 = z0 + static_cast<Ogre::Real>(si + 1) / static_cast<Ogre::Real>(numPts - 1) * segLen;
                    const Ogre::Real h0 = heights[static_cast<size_t>(si)];
                    const Ogre::Real h1 = heights[static_cast<size_t>(si + 1)];
                    const Ogre::Real subLen = subZ1 - subZ0;

                    // For vertical road: BL/BR = left/right edge at z=subZ0, TL/TR at z=subZ1
                    // x-width: xVal-rw to xVal+rw
                    emitRoadQuad(xVal-ox - halfRw, xVal-ox + halfRw, h0, h1, subZ0-oz, subZ1-oz, false, subLen);
                }
            }
        }

        // ---- Intersection quads (square pad at each grid crossing) ----
        // Each corner uses its own terrain height, matching the adjacent strip endpoints.
        // INTERSECT_EXTRA lifts them just above the strip surface to cover any overlap
        // zone cleanly without causing visible height discontinuities.
        for (Ogre::Real xVal : gridX)
        {
            for (Ogre::Real zVal : gridZ)
            {
                const Ogre::Real gyBL = this->getGroundHeight(Ogre::Vector3(xVal, 0.f, zVal)) - oy + ROAD_LIFT + INTERSECT_EXTRA;
                const Ogre::Real gyBR = this->getGroundHeight(Ogre::Vector3(xVal, 0.f, zVal)) - oy + ROAD_LIFT + INTERSECT_EXTRA;
                const Ogre::Real gyTR = gyBR;
                const Ogre::Real gyTL = gyBL;
                // Intersection is a single point so all corners share the same sampled height.
                // The INTERSECT_EXTRA ensures it always covers the horizontal/vertical strip
                // overlap that exists directly beneath it.
                this->pushQuad(roadV, roadI, roadN,
                    Ogre::Vector3(xVal-ox - halfRw, gyBL, zVal-oz - halfRw),
                    Ogre::Vector3(xVal-ox - halfRw, gyTL, zVal-oz + halfRw),
                    Ogre::Vector3(xVal-ox + halfRw, gyTR, zVal-oz + halfRw),
                    Ogre::Vector3(xVal-ox + halfRw, gyBR, zVal-oz - halfRw),
                    Ogre::Vector3::UNIT_Y, rw / tile, rw / tile);
            }
        }
    }

    // =========================================================================
    // generateCourtyardWallGeometry — 5-face solid wall with thickness
    // Matches ProceduralWallComponent::generateSolidWall() geometry:
    //   1. Front face (outer,  normal =  n_outer)
    //   2. Back  face (inner,  normal = -n_outer)
    //   3. Top   cap  (UNIT_Y)
    //   4. Start cap  (normal = -dir)
    //   5. End   cap  (normal = +dir)
    //
    // Winding derivation (verified with cross-product for all 4 segments):
    //   Front:  s,  s+H, e+H, e        → n_outer
    //   Back:   eb, eb+H, sb+H, sb      → -n_outer
    //   Top:    e+H, eb+H, sb+H, s+H   → UNIT_Y
    //   Start:  sb+H, s+H, s, sb        → -dir
    //   End:    e+H, eb+H, eb, e        → +dir
    // Where sb = s - n_outer*thk,  eb = e - n_outer*thk (inner corners)
    // =========================================================================

    void ProceduralCityComponent::generateCourtyardWallGeometry(const std::vector<CityBlock>& blocks, const Ogre::Vector3& localOrigin, std::vector<float>& wallV, std::vector<Ogre::uint32>& wallI, size_t& wallN)
    {
        // Wall feature removed by designer request — function is a no-op stub.
        // The designer adds walls manually after city generation.
        (void)blocks; (void)localOrigin; (void)wallV; (void)wallI; (void)wallN;
    }

    // =========================================================================
    // connectExternalRoad
    // =========================================================================

    void ProceduralCityComponent::connectExternalRoad(const std::vector<CityBlock>& blocks)
    {
        ProceduralRoadComponent* roadComp = this->findRoadComponent();
        if (nullptr == roadComp)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralCityComponent] roadComponentId=" + Ogre::StringConverter::toString(this->roadComponentId) + " set but component not found.");
            return;
        }

        // getRoadConnectionPoint / getRoadApproachDirection must be added to
        // ProceduralRoadComponent — see the separate ProceduralRoadComponent patch
        // below. Both expose existing private members (roadOrigin / loadedRoadEndpoint)
        // with no new data.
        const Ogre::Vector3 roadEndpt = roadComp->getRoadConnectionPoint(this->roadConnectionAtStart);

        const Ogre::Vector4 bounds = this->cityBoundsAttr->getVector4();
        const float dxNeg = std::abs(roadEndpt.x - bounds.x);
        const float dxPos = std::abs(roadEndpt.x - bounds.z);
        const float dzNeg = std::abs(roadEndpt.z - bounds.y);
        const float dzPos = std::abs(roadEndpt.z - bounds.w);
        const float minDist = std::min({dxNeg, dxPos, dzNeg, dzPos});

        Ogre::Vector3 boundaryEntry;
        if (minDist == dxNeg)
        {
            boundaryEntry.x = bounds.x;
            boundaryEntry.z = Ogre::Math::Clamp(roadEndpt.z, bounds.y, bounds.w);
        }
        else if (minDist == dxPos)
        {
            boundaryEntry.x = bounds.z;
            boundaryEntry.z = Ogre::Math::Clamp(roadEndpt.z, bounds.y, bounds.w);
        }
        else if (minDist == dzNeg)
        {
            boundaryEntry.z = bounds.y;
            boundaryEntry.x = Ogre::Math::Clamp(roadEndpt.x, bounds.x, bounds.z);
        }
        else
        {
            boundaryEntry.z = bounds.w;
            boundaryEntry.x = Ogre::Math::Clamp(roadEndpt.x, bounds.x, bounds.z);
        }
        boundaryEntry.y = this->getGroundHeight(boundaryEntry);

        roadComp->setRoadWidth(this->roadWidthAttr->getReal());
        roadComp->addRoadSegmentLua(roadEndpt, boundaryEntry);

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralCityComponent] Connected road to city boundary at " + Ogre::StringConverter::toString(boundaryEntry));
    }

    // =========================================================================
    // createCityOnRenderThread
    // =========================================================================

    void ProceduralCityComponent::createCityOnRenderThread(std::vector<CityBatch>&& batches)
    {
        GraphicsModule::RenderCommand cmd = [this, batches = std::move(batches)]() mutable
        {
            Ogre::Root* root = Ogre::Root::getSingletonPtr();
            Ogre::VaoManager* vm = root->getRenderSystem()->getVaoManager();
            if (nullptr != vm)
            {
                vm->_update();
            }

            Ogre::SceneManager* sm = this->gameObjectPtr->getSceneManager();
            Ogre::HlmsManager* hlms = root->getHlmsManager();

            // ---- Helper: expand one batch's 8-float vertices to 12-float GPU layout,
            //             build VBO+IBO+VAO, and attach as a SubMesh.
            //             Returns a dummy single-vertex VAO for empty batches so the
            //             SubMesh count stays constant (matching ProceduralRoadComponent's
            //             "always create both submeshes even if one is empty" approach).
            auto buildSubMesh = [&](Ogre::MeshPtr& mesh, CityBatch& batch, Ogre::Vector3& minB, Ogre::Vector3& maxB)
            {
                Ogre::VertexElement2Vec elems;
                elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
                elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));
                elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT4, Ogre::VES_TANGENT));
                elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));

                Ogre::SubMesh* sub = mesh->createSubMesh();

                if (batch.rawVertices.empty() || 0u == batch.numVertices)
                {
                    // Dummy VAO — keeps SubMesh index stable even when a slot has
                    // no geometry (e.g. no courtyard walls, no curb).
                    float dummyV[12] = {};
                    Ogre::VertexBufferPackedVec dv;
                    dv.push_back(vm->createVertexBuffer(elems, 1u, Ogre::BT_IMMUTABLE, dummyV, false));
                    Ogre::VertexArrayObject* dVao = vm->createVertexArrayObject(dv, nullptr, Ogre::OT_TRIANGLE_LIST);
                    sub->mVao[Ogre::VpNormal].push_back(dVao);
                    sub->mVao[Ogre::VpShadow].push_back(dVao);
                    return;
                }

                const size_t numV      = batch.numVertices;
                const size_t srcStride = 8u;
                const size_t dstStride = 12u;
                float* dst = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(numV * dstStride * sizeof(float), Ogre::MEMCATEGORY_GEOMETRY));

                for (size_t vi = 0; vi < numV; ++vi)
                {
                    const size_t s = vi * srcStride;
                    const size_t d = vi * dstStride;
                    dst[d + 0] = batch.rawVertices[s + 0];
                    dst[d + 1] = batch.rawVertices[s + 1];
                    dst[d + 2] = batch.rawVertices[s + 2];
                    minB.makeFloor(Ogre::Vector3(dst[d + 0], dst[d + 1], dst[d + 2]));
                    maxB.makeCeil (Ogre::Vector3(dst[d + 0], dst[d + 1], dst[d + 2]));
                    dst[d + 3] = batch.rawVertices[s + 3];
                    dst[d + 4] = batch.rawVertices[s + 4];
                    dst[d + 5] = batch.rawVertices[s + 5];
                    Ogre::Vector3 n(dst[d + 3], dst[d + 4], dst[d + 5]);
                    Ogre::Vector3 t;
                    if (std::abs(n.y) < 0.9f)
                    {
                        t = Ogre::Vector3::UNIT_Y.crossProduct(n).normalisedCopy();
                    }
                    else
                    {
                        t = n.crossProduct(Ogre::Vector3::UNIT_X).normalisedCopy();
                    }
                    dst[d + 6]  = t.x; dst[d + 7]  = t.y; dst[d + 8]  = t.z; dst[d + 9]  = 1.f;
                    dst[d + 10] = batch.rawVertices[s + 6];
                    dst[d + 11] = batch.rawVertices[s + 7];
                }

                Ogre::VertexBufferPacked* vb = nullptr;
                try
                {
                    vb = vm->createVertexBuffer(elems, numV, Ogre::BT_IMMUTABLE, dst, true);
                }
                catch (Ogre::Exception& e)
                {
                    OGRE_FREE_SIMD(dst, Ogre::MEMCATEGORY_GEOMETRY);
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralCityComponent] VB fail slot " + Ogre::StringConverter::toString(batch.materialSlot) + ": " + e.getDescription());
                    return;
                }

                const size_t numIdx = batch.rawIndices.size();
                Ogre::uint32* idxData = reinterpret_cast<Ogre::uint32*>(OGRE_MALLOC_SIMD(numIdx * sizeof(Ogre::uint32), Ogre::MEMCATEGORY_GEOMETRY));
                memcpy(idxData, batch.rawIndices.data(), numIdx * sizeof(Ogre::uint32));

                Ogre::IndexBufferPacked* ib = nullptr;
                try
                {
                    ib = vm->createIndexBuffer(Ogre::IndexBufferPacked::IT_32BIT, numIdx, Ogre::BT_IMMUTABLE, idxData, true);
                }
                catch (Ogre::Exception& e)
                {
                    OGRE_FREE_SIMD(idxData, Ogre::MEMCATEGORY_GEOMETRY);
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralCityComponent] IB fail slot " + Ogre::StringConverter::toString(batch.materialSlot) + ": " + e.getDescription());
                    return;
                }

                Ogre::VertexBufferPackedVec vbVec;
                vbVec.push_back(vb);
                Ogre::VertexArrayObject* vao = vm->createVertexArrayObject(vbVec, ib, Ogre::OT_TRIANGLE_LIST);
                sub->mVao[Ogre::VpNormal].push_back(vao);
                sub->mVao[Ogre::VpShadow].push_back(vao);
            };

            auto makeItem = [&](Ogre::MeshPtr& mesh, const Ogre::String& meshName, Ogre::Vector3& minB, Ogre::Vector3& maxB, unsigned int firstSlot, unsigned int slotCount) -> Ogre::Item*
            {
                // Remove stale mesh
                {
                    Ogre::MeshManager& mm = Ogre::MeshManager::getSingleton();
                    Ogre::MeshPtr ex = mm.getByName(meshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
                    if (false == ex.isNull())
                    {
                        mm.remove(ex->getHandle());
                    }
                }
                mesh = Ogre::MeshManager::getSingleton().createManual(meshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

                minB = Ogre::Vector3( std::numeric_limits<float>::max());
                maxB = Ogre::Vector3(-std::numeric_limits<float>::max());

                for (unsigned int s = 0; s < slotCount; ++s)
                {
                    buildSubMesh(mesh, batches[firstSlot + s], minB, maxB);
                }

                if (minB.x > maxB.x)
                {
                    minB = Ogre::Vector3(-1.f);
                    maxB = Ogre::Vector3(1.f);
                }
                Ogre::Aabb aabb;
                aabb.setExtents(minB, maxB);
                mesh->_setBounds(aabb, false);
                mesh->_setBoundingSphereRadius(aabb.getRadius());

                return sm->createItem(mesh, Ogre::SCENE_STATIC);
            };

            const Ogre::String id = Ogre::StringConverter::toString(this->gameObjectPtr->getId());
            const unsigned int numDistricts = static_cast<unsigned int>(this->districts.size());
            const unsigned int d0 = (numDistricts > 0u) ? 0u : static_cast<unsigned int>(-1);

            // ---- 4 Building Variant Items (slots 0-19, 5 submeshes each) -------
            for (unsigned int vi = 0; vi < NUM_BUILDING_VARIANTS; ++vi)
            {
                const unsigned int batchBase = vi * SLOTS_PER_VARIANT;

                Ogre::MeshPtr bMesh;
                Ogre::Vector3 bMin, bMax;
                Ogre::Item* bItem = makeItem(bMesh, "CityBuildings_" + id + "_v" + Ogre::StringConverter::toString(vi), bMin, bMax, batchBase, SLOTS_PER_VARIANT);

                // Per-submesh datablocks: wall/roof/window/trim/door each with per-variant selection
                // Wall:   wallDatablocks[vi]           (4 variants map directly to 4 wall datablocks)
                // Roof:   roofDatablocks[vi % 3]       (cycle through 3 roof datablocks)
                // Window: windowDatablocks[vi % 3]
                // Trim:   trimDatablocks[vi % 2]
                // Door:   alternates city_door_01 / WoodenDoor
                const Ogre::String doorDb   = (vi % 2u == 0) ? this->doorDatablockAttr->getString() : Ogre::String("WoodenDoor");
                const Ogre::String garageDb = this->garageDatablockAttr->getString();

                auto applyBuildingDb = [&](unsigned int subIdx, const Ogre::String& dbName)
                {
                    if (dbName.empty()) { return; }
                    Ogre::HlmsDatablock* db = hlms->getDatablockNoDefault(dbName);
                    if (nullptr != db)
                    {
                        bItem->getSubItem(subIdx)->setDatablock(db);
                    }
                    else
                    {
                        static const char* names[] = {"wall","roof","window","trim","door"};
                        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                            "[ProceduralCityComponent] v" + Ogre::StringConverter::toString(vi)
                            + " " + Ogre::String(names[subIdx]) + ": '" + dbName + "' not found.");
                    }
                };

                if (d0 < numDistricts)
                {
                    applyBuildingDb(0, this->districts[d0].wallDatablocks[vi]);
                    applyBuildingDb(1, this->districts[d0].roofDatablocks[vi % 3]);
                    applyBuildingDb(2, this->districts[d0].windowDatablocks[vi % 3]);
                    applyBuildingDb(3, this->districts[d0].trimDatablocks[vi % 2]);
                }
                applyBuildingDb(4, doorDb);
                applyBuildingDb(5, garageDb);

                Ogre::SceneNode* bNode = sm->getRootSceneNode(Ogre::SCENE_STATIC)->createChildSceneNode(Ogre::SCENE_STATIC);
                bNode->setPosition(this->cityOrigin);
                bNode->setOrientation(Ogre::Quaternion::IDENTITY);
                bNode->setScale(Ogre::Vector3::UNIT_SCALE);
                bNode->attachObject(bItem);
                sm->notifyStaticAabbDirty(bItem);

                batches[batchBase].items.push_back(bItem);
                batches[batchBase].nodes.push_back(bNode);
            }

            // ---- Infrastructure Item: slots 24-25 (road, curb) ---
            Ogre::MeshPtr infraMesh;
            Ogre::Vector3 iMinB, iMaxB;
            Ogre::Item* infraItem = makeItem(infraMesh, "CityInfra_" + id, iMinB, iMaxB, INFRA_BATCH_OFFSET, 2u);

            auto applyInfraDb = [&](unsigned int subIdx, const Ogre::String& dbName)
            {
                if (dbName.empty()) { return; }
                Ogre::HlmsDatablock* db = hlms->getDatablockNoDefault(dbName);
                if (nullptr != db)
                {
                    infraItem->getSubItem(subIdx)->setDatablock(db);
                }
                else
                {
                    static const char* names[] = {"road","curb"};
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                        "[ProceduralCityComponent] infra " + Ogre::String(names[subIdx]) + ": '" + dbName + "' not found.");
                }
            };

            applyInfraDb(0, this->roadDatablockAttr->getString());
            applyInfraDb(1, this->curbDatablockAttr->getString());
            // slot 2 (courtyard wall) removed — walls feature removed

            Ogre::SceneNode* infraNode = sm->getRootSceneNode(Ogre::SCENE_STATIC)->createChildSceneNode(Ogre::SCENE_STATIC);
            infraNode->setPosition(this->cityOrigin);
            infraNode->setOrientation(Ogre::Quaternion::IDENTITY);
            infraNode->setScale(Ogre::Vector3::UNIT_SCALE);
            infraNode->attachObject(infraItem);
            sm->notifyStaticAabbDirty(infraItem);

            batches[INFRA_BATCH_OFFSET].items.push_back(infraItem);
            batches[INFRA_BATCH_OFFSET].nodes.push_back(infraNode);

            sm->notifyStaticDirty(sm->getRootSceneNode(Ogre::SCENE_STATIC));
            if (nullptr != vm)
            {
                vm->_update();
            }

            this->cityBatches = std::move(batches);

            if (nullptr != this->physicsArtifactComponent)
            {
                this->physicsArtifactComponent->reCreateCollision();
            }
        };

        GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "ProceduralCityComponent::Create");
    }

    // =========================================================================
    // destroyCityOnRenderThread
    // =========================================================================

    void ProceduralCityComponent::destroyCityOnRenderThread(void)
    {
        Ogre::SceneManager* sm = this->gameObjectPtr->getSceneManager();
        for (auto& batch : this->cityBatches)
        {
            for (size_t i = 0; i < batch.items.size(); ++i)
            {
                if (nullptr != batch.nodes[i])
                {
                    batch.nodes[i]->detachAllObjects();
                    GraphicsModule::getInstance()->removeTrackedNode(batch.nodes[i]);
                    sm->destroySceneNode(batch.nodes[i]);
                    batch.nodes[i] = nullptr;
                }
                if (nullptr != batch.items[i])
                {
                    Ogre::MeshPtr m = batch.items[i]->getMesh();
                    sm->destroyItem(batch.items[i]);
                    batch.items[i] = nullptr;
                    if (false == m.isNull())
                    {
                        const Ogre::String& mn = m->getName();
                        if (mn.find("CityBuildings_") != Ogre::String::npos || mn.find("CityInfra_") != Ogre::String::npos)
                        {
                            Ogre::MeshManager::getSingleton().remove(m->getHandle());
                        }
                    }
                }
            }
            batch.items.clear();
            batch.nodes.clear();
            batch.rawVertices.clear();
            batch.rawIndices.clear();
            batch.numVertices = 0;
            batch.instances.clear();
        }
        this->cityBatches.clear();
        sm->notifyStaticDirty(sm->getRootSceneNode(Ogre::SCENE_STATIC));
    }

    // =========================================================================
    // Cache file
    // =========================================================================

    Ogre::String ProceduralCityComponent::getCityDataFilePath(void) const
    {
        Ogre::String p = this->gameObjectPtr->getGlobal() ? Core::getSingletonPtr()->getCurrentProjectPath() : Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName();
        return p + "/City_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()) + ".citydata";
    }

    uint64_t ProceduralCityComponent::computeChecksum(void) const
    {
        uint64_t cs = 1469598103934665603ULL;
        cityHashCombine(cs, static_cast<uint64_t>(this->masterSeedAttr->getUInt()));
        cityHashCombine(cs, cityHashReal(this->blockSizeAttr->getReal()));
        cityHashCombine(cs, cityHashReal(this->roadWidthAttr->getReal()));
        cityHashCombine(cs, cityHashReal(this->sidewalkWidthAttr->getReal()));
        cityHashCombine(cs, cityHashReal(this->curbHeightAttr->getReal()));
        cityHashCombine(cs, static_cast<uint64_t>(this->generateRoadsAttr->getBool() ? 1u : 0u));
        cityHashCombine(cs, cityHashString(this->doorDatablockAttr->getString()));
        const Ogre::Vector4 b = this->cityBoundsAttr->getVector4();
        cityHashCombine(cs, cityHashReal(b.x));
        cityHashCombine(cs, cityHashReal(b.y));
        cityHashCombine(cs, cityHashReal(b.z));
        cityHashCombine(cs, cityHashReal(b.w));
        cityHashCombine(cs, static_cast<uint64_t>(this->districts.size()));
        for (const auto& d : this->districts)
        {
            cityHashCombine(cs, cityHashString(d.type));
            cityHashCombine(cs, cityHashReal(d.minHeight));
            cityHashCombine(cs, cityHashReal(d.maxHeight));
            cityHashCombine(cs, cityHashReal(d.minFootprint.x));
            cityHashCombine(cs, cityHashReal(d.minFootprint.y));
            cityHashCombine(cs, cityHashReal(d.maxFootprint.x));
            cityHashCombine(cs, cityHashReal(d.maxFootprint.y));
            cityHashCombine(cs, cityHashReal(d.density));
            for (int v = 0; v < 3; ++v)
            {
                cityHashCombine(cs, cityHashString(d.roofDatablocks[v]));
            }
            for (int v = 0; v < 3; ++v)
            {
                cityHashCombine(cs, cityHashString(d.windowDatablocks[v]));
            }
            for (int v = 0; v < 2; ++v)
            {
                cityHashCombine(cs, cityHashString(d.trimDatablocks[v]));
            }
        }
        return cs;
    }

    bool ProceduralCityComponent::saveCityDataToFile(const std::vector<CityBatch>& batches)
    {
        std::ofstream out(this->getCityDataFilePath().c_str(), std::ios::binary);
        if (false == out.is_open())
        {
            return false;
        }
        const char mg[4] = {'C', 'I', 'T', 'Y'};
        const uint32_t ver = CITY_CACHE_VERSION;
        const uint64_t cs = this->computeChecksum();
        const uint32_t bc = static_cast<uint32_t>(batches.size());
        out.write(mg, 4);
        out.write(reinterpret_cast<const char*>(&ver), sizeof(ver));
        out.write(reinterpret_cast<const char*>(&cs), sizeof(cs));
        out.write(reinterpret_cast<const char*>(&bc), sizeof(bc));
        for (const auto& batch : batches)
        {
            const uint32_t slot = batch.materialSlot;
            const uint64_t di = static_cast<uint64_t>(batch.districtIdx);
            const uint64_t ic = static_cast<uint64_t>(batch.instances.size());
            out.write(reinterpret_cast<const char*>(&slot), sizeof(slot));
            out.write(reinterpret_cast<const char*>(&di), sizeof(di));
            out.write(reinterpret_cast<const char*>(&ic), sizeof(ic));
            for (const auto& inst : batch.instances)
            {
                const float pos[3] = {inst.position.x, inst.position.y, inst.position.z};
                const float ori[4] = {inst.orientation.w, inst.orientation.x, inst.orientation.y, inst.orientation.z};
                const float fp[3] = {inst.footprint.x, inst.footprint.y, inst.footprint.z};
                const uint32_t arch = inst.archetypeIdx;
                const uint32_t vs = inst.variantSeed;
                const uint32_t rt = inst.roofType;
                const float rp = inst.roofPitch;
                const float gh = inst.groundHeight;
                out.write(reinterpret_cast<const char*>(pos), sizeof(pos));
                out.write(reinterpret_cast<const char*>(ori), sizeof(ori));
                out.write(reinterpret_cast<const char*>(fp), sizeof(fp));
                out.write(reinterpret_cast<const char*>(&arch), sizeof(arch));
                out.write(reinterpret_cast<const char*>(&vs), sizeof(vs));
                out.write(reinterpret_cast<const char*>(&rt), sizeof(rt));
                out.write(reinterpret_cast<const char*>(&rp), sizeof(rp));
                out.write(reinterpret_cast<const char*>(&gh), sizeof(gh));
            }
        }
        return true;
    }

    bool ProceduralCityComponent::loadCityDataFromFile(std::vector<CityBatch>& outBatches)
    {
        std::ifstream in(this->getCityDataFilePath().c_str(), std::ios::binary);
        if (false == in.is_open())
        {
            return false;
        }
        char mg[4] = {};
        in.read(mg, 4);
        if (false == in.good() || mg[0] != 'C' || mg[1] != 'I' || mg[2] != 'T' || mg[3] != 'Y')
        {
            return false;
        }
        uint32_t ver = 0;
        in.read(reinterpret_cast<char*>(&ver), sizeof(ver));
        if (false == in.good() || ver != CITY_CACHE_VERSION)
        {
            return false;
        }
        uint64_t storedCs = 0;
        in.read(reinterpret_cast<char*>(&storedCs), sizeof(storedCs));
        if (false == in.good() || storedCs != this->computeChecksum())
        {
            return false;
        }
        uint32_t bc = 0;
        in.read(reinterpret_cast<char*>(&bc), sizeof(bc));
        if (false == in.good())
        {
            return false;
        }

        std::vector<CityBatch> loaded;
        loaded.reserve(bc);
        for (uint32_t bi = 0; bi < bc; ++bi)
        {
            CityBatch batch;
            uint32_t slot = 0;
            uint64_t di = 0;
            uint64_t ic = 0;
            in.read(reinterpret_cast<char*>(&slot), sizeof(slot));
            in.read(reinterpret_cast<char*>(&di), sizeof(di));
            in.read(reinterpret_cast<char*>(&ic), sizeof(ic));
            if (false == in.good())
            {
                return false;
            }
            batch.materialSlot = slot;
            batch.districtIdx = static_cast<size_t>(di);
            batch.instances.reserve(static_cast<size_t>(ic));
            for (uint64_t ii = 0; ii < ic; ++ii)
            {
                float pos[3] = {}, ori[4] = {1.f, 0.f, 0.f, 0.f}, fp[3] = {};
                uint32_t arch = 0, vs = 0, rt = 0;
                float rp = 0.f, gh = 0.f;
                in.read(reinterpret_cast<char*>(pos), sizeof(pos));
                in.read(reinterpret_cast<char*>(ori), sizeof(ori));
                in.read(reinterpret_cast<char*>(fp), sizeof(fp));
                in.read(reinterpret_cast<char*>(&arch), sizeof(arch));
                in.read(reinterpret_cast<char*>(&vs), sizeof(vs));
                in.read(reinterpret_cast<char*>(&rt), sizeof(rt));
                in.read(reinterpret_cast<char*>(&rp), sizeof(rp));
                in.read(reinterpret_cast<char*>(&gh), sizeof(gh));
                if (false == in.good())
                {
                    return false;
                }
                BuildingInstance inst;
                inst.position = Ogre::Vector3(pos[0], pos[1], pos[2]);
                inst.orientation = Ogre::Quaternion(ori[0], ori[1], ori[2], ori[3]);
                inst.footprint = Ogre::Vector3(fp[0], fp[1], fp[2]);
                inst.archetypeIdx = arch;
                inst.variantSeed = vs;
                inst.roofType = rt;
                inst.roofPitch = rp;
                inst.groundHeight = gh;
                batch.instances.push_back(inst);
            }
            loaded.push_back(std::move(batch));
        }

        // Regenerate geometry from saved instances.
        // Slots 0..INFRA_BATCH_OFFSET-1 are building slots (4 variants × 5 slots each),
        // regenerated from BuildingInstance data.  Infra slots (road/curb/wall) have
        // no BuildingInstances and must be regenerated from scratch if changed.
        for (auto& batch : loaded)
        {
            if (batch.materialSlot >= INFRA_BATCH_OFFSET)
            {
                continue;
            }
            // Each variant batch only stores instances in its wall-slot (base+0).
            // The other 4 slots in the same variant group are regenerated by routing
            // the same building data to the right output buffers.
            for (const auto& inst : batch.instances)
            {
                if (inst.archetypeIdx >= this->districts.size())
                {
                    continue;
                }
                const CityDistrict& d = this->districts[inst.archetypeIdx];
                std::vector<float>        wV, rV, winV, tV, dV;
                std::vector<Ogre::uint32> wI, rI, winI, tI, dI;
                size_t wN = 0, rN = 0, winN = 0, tN = 0, dN = 0;
                std::vector<float> gV; std::vector<Ogre::uint32> gI; size_t gN = 0;
                this->generateBuildingGeometry(inst, d, this->cityOrigin, wV, wI, wN, rV, rI, rN, winV, winI, winN, tV, tI, tN, dV, dI, dN, gV, gI, gN);

                // Route to the correct variant batch group
                const unsigned int vi = inst.variantSeed % NUM_BUILDING_VARIANTS;
                const unsigned int batchBase = vi * SLOTS_PER_VARIANT;

                auto ap = [](std::vector<float>& dstV, std::vector<Ogre::uint32>& dstI, size_t& dstN,
                             const std::vector<float>& srcV, const std::vector<Ogre::uint32>& srcI, size_t srcN)
                {
                    const Ogre::uint32 base = static_cast<Ogre::uint32>(dstN);
                    dstV.insert(dstV.end(), srcV.begin(), srcV.end());
                    for (auto idx : srcI) { dstI.push_back(idx + base); }
                    dstN += srcN;
                };

                // Only accumulate if this batch is the wall-slot of its variant group
                // (the other 4 slots were not stored in the cache as separate batch entries
                // with instances — they need to be re-accumulated here from the wall-slot instances)
                if (batch.materialSlot == batchBase + 0) { ap(batch.rawVertices, batch.rawIndices, batch.numVertices, wV, wI, wN); }
                // Find the matching sibling batches in the loaded list and fill them too
                for (auto& sibling : loaded)
                {
                    if (sibling.materialSlot == batchBase + 1) { ap(sibling.rawVertices, sibling.rawIndices, sibling.numVertices, rV, rI, rN); }
                    else if (sibling.materialSlot == batchBase + 2) { ap(sibling.rawVertices, sibling.rawIndices, sibling.numVertices, winV, winI, winN); }
                    else if (sibling.materialSlot == batchBase + 3) { ap(sibling.rawVertices, sibling.rawIndices, sibling.numVertices, tV, tI, tN); }
                    else if (sibling.materialSlot == batchBase + 4) { ap(sibling.rawVertices, sibling.rawIndices, sibling.numVertices, dV, dI, dN); }
                    else if (sibling.materialSlot == batchBase + 5) { ap(sibling.rawVertices, sibling.rawIndices, sibling.numVertices, gV, gI, gN); }
                }
            }
        }
        outBatches = std::move(loaded);
        return true;
    }

    void ProceduralCityComponent::deleteCityDataFile(void)
    {
        std::remove(this->getCityDataFilePath().c_str());
    }

    // =========================================================================
    // getGroundHeight — ProceduralRoadComponent / FoliageVolumeComponent pattern
    // =========================================================================

    Ogre::Real ProceduralCityComponent::getGroundHeight(const Ogre::Vector3& position) const
    {
        if (nullptr == this->groundQuery)
        {
            return position.y;
        }
        Ogre::Ray ray(Ogre::Vector3(position.x, position.y + 1000.f, position.z), Ogre::Vector3::NEGATIVE_UNIT_Y);
        this->groundQuery->setRay(ray);

        std::vector<Ogre::MovableObject*> exclude;
        for (const auto& b : this->cityBatches)
        {
            for (auto* item : b.items)
            {
                if (nullptr != item) { exclude.push_back(item); }
            }
        }

        Ogre::Vector3 hp;
        Ogre::MovableObject* ho = nullptr;
        Ogre::Real hd = 0.f;
        Ogre::Vector3 hn;
        bool hit = MathHelper::getInstance()->getRaycastFromPoint(this->groundQuery,
            AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera(),
            hp, (size_t&)ho, hd, hn, &exclude);

        if (hit && nullptr != ho)
        {
            // Reject editor helper objects: Ogre::Light node meshes (LightDirectional.mesh etc.),
            // Ogre::Camera helpers, node.mesh gizmos. These are visible in the editor as indicator
            // objects but are NOT terrain — hitting them returns the gizmo altitude instead of the
            // actual terrain height, causing buildings/roads to appear at wrong Y positions.
            const Ogre::String& movType = ho->getMovableType();

            bool isEditorHelper = (movType == "Light" || movType == "Camera");
            if (false == isEditorHelper && movType == "Item")
            {
                Ogre::Item* hitItem = static_cast<Ogre::Item*>(ho);
                if (false == hitItem->getMesh().isNull())
                {
                    const Ogre::String& mName = hitItem->getMesh()->getName();
                    isEditorHelper =
                        mName.find("Light")  != Ogre::String::npos ||
                        mName.find("light")  != Ogre::String::npos ||
                        mName.find("Camera") != Ogre::String::npos ||
                        mName.find("camera") != Ogre::String::npos ||
                        mName.find("node.")  != Ogre::String::npos ||
                        mName.find("Node.")  != Ogre::String::npos ||
                        mName.find("arrow")  != Ogre::String::npos ||
                        mName.find("Arrow")  != Ogre::String::npos ||
                        mName.find("gizmo")  != Ogre::String::npos ||
                        mName.find("Gizmo")  != Ogre::String::npos;
                }
            }

            if (false == isEditorHelper)
            {
                // Sanity clamp: if the hit Y is more than 300m above or 100m below the city
                // origin, something is wrong (terrain spike, very hilly edge, or a missed
                // editor-helper exclusion).  Fall through to the ground-plane fallback.
                if (hp.y < this->cityOrigin.y - 100.f || hp.y > this->cityOrigin.y + 300.f)
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                        "[ProceduralCityComponent] getGroundHeight: hit Y=" +
                        Ogre::StringConverter::toString(hp.y) + " is implausible — rejected.");
                }
                else
                {
                    return hp.y;
                }
            }
            // else fall through to ground plane fallback below
        }

        auto res = ray.intersects(this->groundPlane);
        if (res.first && res.second > 0.f)
        {
            return position.y + 1000.f - res.second;
        }
        return position.y;
    }

    // =========================================================================
    // findRoadComponent / event handlers
    // =========================================================================

    ProceduralRoadComponent* ProceduralCityComponent::findRoadComponent(void) const
    {
        if (0 == this->roadComponentId)
        {
            return nullptr;
        }
        GameObjectPtr go = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->roadComponentId);
        if (!go)
        {
            return nullptr;
        }
        auto comp = NOWA::makeStrongPtr(go->getComponent<ProceduralRoadComponent>());
        return comp ? comp.get() : nullptr;
    }

    void ProceduralCityComponent::handleSceneParsed(NOWA::EventDataPtr eventData)
    {
        AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ProceduralCityComponent::handleSceneParsed), EventDataSceneParsed::getStaticEventType());

        this->loadOrGenerateCity();
    }

    void ProceduralCityComponent::handleComponentManuallyDeleted(NOWA::EventDataPtr eventData)
    {
        auto data = boost::static_pointer_cast<EventDataDeleteComponent>(eventData);
        if (data->getGameObjectId() == this->gameObjectPtr->getId())
        {
            this->deleteCityDataFile();
        }
    }

    // =========================================================================
    // setDistrictCount
    // =========================================================================

    void ProceduralCityComponent::setDistrictCount(unsigned int count)
    {
        this->districtCountAttr->setValue(count);
        const size_t old = this->districts.size();
        this->districts.resize(count);

        this->districtNameAttrs.resize(count, nullptr);
        this->districtTypeAttrs.resize(count, nullptr);
        this->districtMinHeightAttrs.resize(count, nullptr);
        this->districtMaxHeightAttrs.resize(count, nullptr);
        this->districtMinFootprintAttrs.resize(count, nullptr);
        this->districtMaxFootprintAttrs.resize(count, nullptr);
        this->districtDensityAttrs.resize(count, nullptr);
        this->districtCourtyardProbAttrs.resize(count, nullptr);
        this->districtWallDbAttrs.resize(count);
        this->districtRoofDbAttrs.resize(count);
        this->districtWindowDbAttrs.resize(count);
        this->districtTrimDbAttrs.resize(count);

        struct TD
        {
            Ogre::Real minH, maxH, minFP, maxFP;
        };
        static const TD td[] = {
            {4.f, 9.f, 6.f, 12.f},    // Residential_Low
            {8.f, 18.f, 8.f, 16.f},   // Residential_Mid
            {6.f, 15.f, 10.f, 20.f},  // Commercial
            {20.f, 60.f, 12.f, 24.f}, // Tower
            {5.f, 12.f, 15.f, 30.f},  // Industrial
            {4.f, 15.f, 8.f, 18.f}    // Mixed
        };
        static const char* typeNames[] = {"Residential_Low", "Residential_Mid", "Commercial", "Tower", "Industrial", "Mixed"};

        // Real datablock names from user's asset collection
        // Defaults: use full HLMS datablock names from the user's asset collection.
        // 4 wall variants map to the 4 wallDatablocks[] slots — each building gets
        // the wall for its variant group (variantSeed % 4).
        static const char* wallDef[4] = {
            "/dural/structures/moraf_stone_wall/texture",      // variant 0
            "/dural/structures/rezpa_wall/texture",            // variant 1
            "/dural/structures/ossyja_wall_cobblestone/texture",// variant 2
            "/dural/structures/moraf_timber_wall/texture"      // variant 3
        };
        static const char* roofDef[3] = {
            "city_roof_01",                                    // from generated PNG
            "/dural/structures/moraf_roof_bottom/texture",     // real asset
            "M_rooftiles_01"                                   // real asset
        };
        static const char* winDef[3] = {
            "city_window_01", "city_window_02", "city_window_01"
        };
        static const char* trimDef[2] = {
            "/dural/structures/ossyja_inner_walls/texture",
            "/dural/structures/rezpa_inner_walls/texture"
        };

        for (size_t i = old; i < count; ++i)
        {
            Ogre::String idx = Ogre::StringConverter::toString(i);
            const TD& t = td[i % 6];

            this->districts[i].minHeight = t.minH;
            this->districts[i].maxHeight = t.maxH;
            this->districts[i].minFootprint.x = this->districts[i].minFootprint.y = t.minFP;
            this->districts[i].maxFootprint.x = this->districts[i].maxFootprint.y = t.maxFP;
            this->districts[i].type = typeNames[i % 6];
            this->districts[i].name = Ogre::String("District_") + idx;

            auto mkS = [&](Variant*& ptr, const Ogre::String& name, const Ogre::String& val, const Ogre::String& desc = "")
            {
                if (nullptr == ptr)
                {
                    ptr = new Variant(name, val, this->attributes);
                    if (!desc.empty())
                    {
                        ptr->setDescription(desc);
                    }
                }
            };
            auto mkR = [&](Variant*& ptr, const Ogre::String& name, Ogre::Real val, Ogre::Real lo, Ogre::Real hi, const Ogre::String& desc = "")
            {
                if (nullptr == ptr)
                {
                    ptr = new Variant(name, val, this->attributes);
                    ptr->setConstraints(lo, hi);
                    if (!desc.empty())
                    {
                        ptr->setDescription(desc);
                    }
                }
            };
            auto mkV2 = [&](Variant*& ptr, const Ogre::String& name, const Ogre::Vector2& val)
            {
                if (nullptr == ptr)
                {
                    ptr = new Variant(name, val, this->attributes);
                }
            };

            mkS(this->districtNameAttrs[i], AttrDistrictName() + idx, this->districts[i].name, "Display name for district.");
            mkS(this->districtTypeAttrs[i], AttrDistrictType() + idx, Ogre::String(this->districts[i].type), "Residential_Low / Residential_Mid / Commercial / Tower / Industrial / Mixed");
            mkR(this->districtMinHeightAttrs[i], AttrDistrictMinHeight() + idx, t.minH, 1.f, 200.f, "Min building height (m).");
            mkR(this->districtMaxHeightAttrs[i], AttrDistrictMaxHeight() + idx, t.maxH, 1.f, 200.f, "Max building height (m). AAA variance: random per building.");
            mkV2(this->districtMinFootprintAttrs[i], AttrDistrictMinFootprint() + idx, Ogre::Vector2(t.minFP, t.minFP));
            mkV2(this->districtMaxFootprintAttrs[i], AttrDistrictMaxFootprint() + idx, Ogre::Vector2(t.maxFP, t.maxFP));
            mkR(this->districtDensityAttrs[i], AttrDistrictDensity() + idx, 0.85f, 0.f, 1.f, "Lot occupancy [0..1].");

            for (unsigned int v = 0; v < 3u; ++v)
            {
                if (nullptr == this->districtRoofDbAttrs[i][v])
                {
                    Ogre::String key = AttrDistrictRoofDatablock() + idx + "_" + Ogre::StringConverter::toString(v);
                    this->districtRoofDbAttrs[i][v] = new Variant(key, Ogre::String(roofDef[v]), this->attributes);
                    this->districtRoofDbAttrs[i][v]->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
                    this->districts[i].roofDatablocks[v] = roofDef[v];
                }
                if (nullptr == this->districtWindowDbAttrs[i][v])
                {
                    Ogre::String key = AttrDistrictWindowDatablock() + idx + "_" + Ogre::StringConverter::toString(v);
                    this->districtWindowDbAttrs[i][v] = new Variant(key, Ogre::String(winDef[v]), this->attributes);
                    this->districtWindowDbAttrs[i][v]->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
                    this->districts[i].windowDatablocks[v] = winDef[v];
                }
            }
            for (unsigned int v = 0; v < 2u; ++v)
            {
                if (nullptr == this->districtTrimDbAttrs[i][v])
                {
                    Ogre::String key = AttrDistrictTrimDatablock() + idx + "_" + Ogre::StringConverter::toString(v);
                    this->districtTrimDbAttrs[i][v] = new Variant(key, Ogre::String(trimDef[v]), this->attributes);
                    this->districtTrimDbAttrs[i][v]->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
                    this->districts[i].trimDatablocks[v] = trimDef[v];
                }
            }
        }
    }

    // =========================================================================
    // Setters / getters
    // =========================================================================

    void ProceduralCityComponent::setCityBounds(const Ogre::Vector4& v)
    {
        this->cityBoundsAttr->setValue(v);
    }
    Ogre::Vector4 ProceduralCityComponent::getCityBounds(void) const
    {
        return this->cityBoundsAttr->getVector4();
    }
    void ProceduralCityComponent::setMasterSeed(unsigned int v)
    {
        this->masterSeedAttr->setValue(v);
    }
    unsigned int ProceduralCityComponent::getMasterSeed(void) const
    {
        return this->masterSeedAttr->getUInt();
    }
    void ProceduralCityComponent::setBlockSize(Ogre::Real v)
    {
        this->blockSizeAttr->setValue(Ogre::Math::Clamp(v, 10.f, 200.f));
    }
    Ogre::Real ProceduralCityComponent::getBlockSize(void) const
    {
        return this->blockSizeAttr->getReal();
    }
    void ProceduralCityComponent::setRoadWidth(Ogre::Real v)
    {
        this->roadWidthAttr->setValue(Ogre::Math::Clamp(v, 2.f, 20.f));
    }
    Ogre::Real ProceduralCityComponent::getRoadWidth(void) const
    {
        return this->roadWidthAttr->getReal();
    }
    void ProceduralCityComponent::setSidewalkWidth(Ogre::Real v)
    {
        this->sidewalkWidthAttr->setValue(Ogre::Math::Clamp(v, 0.f, 10.f));
    }
    Ogre::Real ProceduralCityComponent::getSidewalkWidth(void) const
    {
        return this->sidewalkWidthAttr->getReal();
    }
    void ProceduralCityComponent::setCurbHeight(Ogre::Real v)
    {
        this->curbHeightAttr->setValue(Ogre::Math::Clamp(v, 0.f, 1.f));
    }
    Ogre::Real ProceduralCityComponent::getCurbHeight(void) const
    {
        return this->curbHeightAttr->getReal();
    }
    void ProceduralCityComponent::setGenerateRoads(bool v)
    {
        this->generateRoadsAttr->setValue(v);
    }
    bool ProceduralCityComponent::getGenerateRoads(void) const
    {
        return this->generateRoadsAttr->getBool();
    }
    void ProceduralCityComponent::setRoadDatablock(const Ogre::String& v)
    {
        this->roadDatablockAttr->setValue(v);
    }
    Ogre::String ProceduralCityComponent::getRoadDatablock(void) const
    {
        return this->roadDatablockAttr->getString();
    }
    void ProceduralCityComponent::setCurbDatablock(const Ogre::String& v)
    {
        this->curbDatablockAttr->setValue(v);
    }
    Ogre::String ProceduralCityComponent::getCurbDatablock(void) const
    {
        return this->curbDatablockAttr->getString();
    }

    void ProceduralCityComponent::setDoorDatablock(const Ogre::String& v)
    {
        this->doorDatablockAttr->setValue(v);
    }
    Ogre::String ProceduralCityComponent::getDoorDatablock(void) const
    {
        return this->doorDatablockAttr->getString();
    }

    void ProceduralCityComponent::setGarageDatablock(const Ogre::String& v)
    {
        this->garageDatablockAttr->setValue(v);
    }
    Ogre::String ProceduralCityComponent::getGarageDatablock(void) const
    {
        return this->garageDatablockAttr->getString();
    }

    void ProceduralCityComponent::setRoadComponentId(unsigned long id)
    {
        this->roadComponentId = id;
        this->roadComponentIdAttr->setValue(Ogre::StringConverter::toString(id));
    }
    unsigned long ProceduralCityComponent::getRoadComponentId(void) const
    {
        return this->roadComponentId;
    }

    void ProceduralCityComponent::setRoadConnectionAtStart(bool v)
    {
        this->roadConnectionAtStart = v;
        this->roadConnectionAtStartAttr->setValue(v);
    }
    bool ProceduralCityComponent::getRoadConnectionAtStart(void) const
    {
        return this->roadConnectionAtStart;
    }

    unsigned int ProceduralCityComponent::getDistrictCount(void) const
    {
        return this->districtCountAttr->getUInt();
    }

    void ProceduralCityComponent::setDistrictName(unsigned int i, const Ogre::String& v)
    {
        if (i < this->districts.size())
        {
            this->districts[i].name = v;
            if (this->districtNameAttrs[i])
            {
                this->districtNameAttrs[i]->setValue(v);
            }
        }
    }
    Ogre::String ProceduralCityComponent::getDistrictName(unsigned int i) const
    {
        return (i < this->districts.size()) ? this->districts[i].name : "";
    }

    void ProceduralCityComponent::setDistrictType(unsigned int i, const Ogre::String& v)
    {
        if (i < this->districts.size())
        {
            this->districts[i].type = v;
            if (this->districtTypeAttrs[i])
            {
                this->districtTypeAttrs[i]->setValue(v);
            }
        }
    }
    Ogre::String ProceduralCityComponent::getDistrictType(unsigned int i) const
    {
        return (i < this->districts.size()) ? this->districts[i].type : "";
    }

    void ProceduralCityComponent::setDistrictMinHeight(unsigned int i, Ogre::Real v)
    {
        if (i < this->districts.size())
        {
            this->districts[i].minHeight = v;
            if (this->districtMinHeightAttrs[i])
            {
                this->districtMinHeightAttrs[i]->setValue(v);
            }
        }
    }
    Ogre::Real ProceduralCityComponent::getDistrictMinHeight(unsigned int i) const
    {
        return (i < this->districts.size()) ? this->districts[i].minHeight : 4.f;
    }

    void ProceduralCityComponent::setDistrictMaxHeight(unsigned int i, Ogre::Real v)
    {
        if (i < this->districts.size())
        {
            this->districts[i].maxHeight = v;
            if (this->districtMaxHeightAttrs[i])
            {
                this->districtMaxHeightAttrs[i]->setValue(v);
            }
        }
    }
    Ogre::Real ProceduralCityComponent::getDistrictMaxHeight(unsigned int i) const
    {
        return (i < this->districts.size()) ? this->districts[i].maxHeight : 9.f;
    }

    void ProceduralCityComponent::setDistrictMinFootprint(unsigned int i, const Ogre::Vector2& v)
    {
        if (i < this->districts.size())
        {
            this->districts[i].minFootprint = v;
            if (this->districtMinFootprintAttrs[i])
            {
                this->districtMinFootprintAttrs[i]->setValue(v);
            }
        }
    }
    Ogre::Vector2 ProceduralCityComponent::getDistrictMinFootprint(unsigned int i) const
    {
        return (i < this->districts.size()) ? this->districts[i].minFootprint : Ogre::Vector2(6.f, 6.f);
    }

    void ProceduralCityComponent::setDistrictMaxFootprint(unsigned int i, const Ogre::Vector2& v)
    {
        if (i < this->districts.size())
        {
            this->districts[i].maxFootprint = v;
            if (this->districtMaxFootprintAttrs[i])
            {
                this->districtMaxFootprintAttrs[i]->setValue(v);
            }
        }
    }
    Ogre::Vector2 ProceduralCityComponent::getDistrictMaxFootprint(unsigned int i) const
    {
        return (i < this->districts.size()) ? this->districts[i].maxFootprint : Ogre::Vector2(12.f, 12.f);
    }

    void ProceduralCityComponent::setDistrictDensity(unsigned int i, Ogre::Real v)
    {
        if (i < this->districts.size())
        {
            this->districts[i].density = Ogre::Math::Clamp(v, 0.f, 1.f);
            if (this->districtDensityAttrs[i])
            {
                this->districtDensityAttrs[i]->setValue(this->districts[i].density);
            }
        }
    }
    Ogre::Real ProceduralCityComponent::getDistrictDensity(unsigned int i) const
    {
        return (i < this->districts.size()) ? this->districts[i].density : 0.85f;
    }



    void ProceduralCityComponent::setDistrictRoofDatablock(unsigned int di, unsigned int v, const Ogre::String& name)
    {
        if (di < this->districts.size() && v < 3u)
        {
            this->districts[di].roofDatablocks[v] = name;
            if (this->districtRoofDbAttrs[di][v])
            {
                this->districtRoofDbAttrs[di][v]->setValue(name);
            }
        }
    }
    Ogre::String ProceduralCityComponent::getDistrictRoofDatablock(unsigned int di, unsigned int v) const
    {
        return (di < this->districts.size() && v < 3u) ? this->districts[di].roofDatablocks[v] : "";
    }

    void ProceduralCityComponent::setDistrictWindowDatablock(unsigned int di, unsigned int v, const Ogre::String& name)
    {
        if (di < this->districts.size() && v < 3u)
        {
            this->districts[di].windowDatablocks[v] = name;
            if (this->districtWindowDbAttrs[di][v])
            {
                this->districtWindowDbAttrs[di][v]->setValue(name);
            }
        }
    }
    Ogre::String ProceduralCityComponent::getDistrictWindowDatablock(unsigned int di, unsigned int v) const
    {
        return (di < this->districts.size() && v < 3u) ? this->districts[di].windowDatablocks[v] : "";
    }

    void ProceduralCityComponent::setDistrictTrimDatablock(unsigned int di, unsigned int v, const Ogre::String& name)
    {
        if (di < this->districts.size() && v < 2u)
        {
            this->districts[di].trimDatablocks[v] = name;
            if (this->districtTrimDbAttrs[di][v])
            {
                this->districtTrimDbAttrs[di][v]->setValue(name);
            }
        }
    }
    Ogre::String ProceduralCityComponent::getDistrictTrimDatablock(unsigned int di, unsigned int v) const
    {
        return (di < this->districts.size() && v < 2u) ? this->districts[di].trimDatablocks[v] : "";
    }

} // namespace NOWA

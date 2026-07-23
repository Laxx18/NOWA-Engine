/*
Copyright (c) 2026 Lukas Kalinowski
GPL v3
*/

#include "NOWAPrecompiled.h"
#include "ProceduralCityComponent.h"
#include "../../ProceduralRoadComponent/code/ProceduralRoadComponent.h"
#include "CityFaceExtractor.h"
#include "CityLotSubdivider.h"
#include "CityRoadGraph.h"
#include "CityTensorField.h"
#include "RenderQueueEnums.h"
#include "editor/EditorManager.h"
#include "gameobject/GameObjectFactory.h"
#include "gameobject/PhysicsArtifactComponent.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "main/InputDeviceCore.h"
#include "modules/GraphicsModule.h"
#include "utilities/MathHelper.h"
#include "utilities/XMLConverter.h"
#include "utilities/Helper.h"

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
#include <functional>
#include <future>
#include <numeric>
#include <random>
#include <set>
#include <thread>

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
    static constexpr unsigned int SLOTS_PER_VARIANT = 6u;                                         // wall, roof, window, trim, door, garage
    static constexpr unsigned int INFRA_BATCH_OFFSET = NUM_BUILDING_VARIANTS * SLOTS_PER_VARIANT; // 24
    static constexpr unsigned int ROAD_SLOT = INFRA_BATCH_OFFSET + 0u;                            // 24
    static constexpr unsigned int CURB_SLOT = INFRA_BATCH_OFFSET + 1u;                            // 25
    static constexpr unsigned int TOTAL_CITY_BATCHES = INFRA_BATCH_OFFSET + 2u;                   // 26 (road + curb only)
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
        cityBoundsAttr(new Variant(AttrCityBounds(), Ogre::Vector4(-299.f, -299.f, 299.f, 299.f), this->attributes)),
        masterSeedAttr(new Variant(AttrMasterSeed(), 42u, this->attributes)),
        blockSizeAttr(new Variant(AttrBlockSize(), 40.f, this->attributes)),
        roadWidthAttr(new Variant(AttrRoadWidth(), 6.f, this->attributes)),
        roadVarianceAttr(new Variant(AttrRoadVariance(), 0.5f, this->attributes)),
        sidewalkWidthAttr(new Variant(AttrSidewalkWidth(), 2.f, this->attributes)),
        curbHeightAttr(new Variant(AttrCurbHeight(), 0.15f, this->attributes)),
        generateRoadsAttr(new Variant(AttrGenerateRoads(), true, this->attributes)),
        roadDatablockAttr(new Variant(AttrRoadDatablock(), Ogre::String("city_road_01"), this->attributes)),
        curbDatablockAttr(new Variant(AttrCurbDatablock(), Ogre::String("city_curb_01"), this->attributes)),
        roadComponentIdAttr(new Variant(AttrRoadComponentId(), Ogre::String("0"), this->attributes)),
        roadConnectionAtStartAttr(new Variant(AttrRoadConnectionAtStart(), false, this->attributes)),
        districtCountAttr(new Variant(AttrDistrictCount(), 9u, this->attributes)),
        doorDatablockAttr(new Variant(AttrDoorDatablock(), Ogre::String("city_door_01"), this->attributes)),
        garageDatablockAttr(new Variant(AttrGarageDatablock(), Ogre::String("city_garage_01"), this->attributes)),
        generateGarageAttr(new Variant(AttrGenerateGarage(), true, this->attributes)),
        gradientAlignmentAttr(new Variant(AttrGradientAlignment(), false, this->attributes)),
        generateBtn(new Variant(AttrGenerate(), Ogre::String("Generate Now"), this->attributes)),
        clearBtn(new Variant(AttrClear(), Ogre::String("Clear"), this->attributes)),
        generateBuildingsBtn(new Variant(AttrGenerateBuildings(), Ogre::String("Generate Buildings"), this->attributes)),
        editModeAttr(new Variant(AttrEditMode(), std::vector<Ogre::String>{"Object", "Segment"}, this->attributes)),
        selectedBuildingIdx(-1),
        isSelected(false),
        inputListenerRegistered(false),
        selOverlayObject(nullptr),
        selOverlayNode(nullptr)
    {
        this->generateBtn->addUserData(GameObject::AttrActionExec());
        this->generateBtn->addUserData(GameObject::AttrActionExecId(), ActionGenerate());
        this->clearBtn->addUserData(GameObject::AttrActionExec());
        this->clearBtn->addUserData(GameObject::AttrActionExecId(), ActionClear());
        this->generateBuildingsBtn->addUserData(GameObject::AttrActionExec());
        this->generateBuildingsBtn->addUserData(GameObject::AttrActionExecId(), ActionGenerateBuildings());

        this->cityBoundsAttr->setDescription("World-space XZ footprint (minX, minZ, maxX, maxZ). Centre the GameObject inside this area.");
        this->masterSeedAttr->setDescription("Master seed. Same seed + same parameters = identical city.");
        this->masterSeedAttr->setConstraints(0u, 999999u);

        this->roadVarianceAttr
            ->setDescription("Road variance [0..0.5]: 0 = straight grid. Higher values add organic curves by routing each segment through a slightly offset midpoint. Each inner road is split into two sub-segments through that midpoint.");
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

        this->gradientAlignmentAttr->setDescription("Tilt buildings so their local Y axis aligns with the planet surface normal.\n"
                                                    "Required when placing a city on a spherical planet to prevent buildings\n"
                                                    "leaning outward at city edges.  Has no effect on flat terrain (planet\n"
                                                    "centre = 0,0,0).  Performance: adds one quaternion multiply per building.");
        this->generateGarageAttr->setDescription("If true, ~30% of Residential and Mixed buildings get an attached garage on their right face.  Disable to build a city without garages.");

        this->generateBtn->setDescription("Generate or regenerate the entire city. Cache is invalidated and rewritten.");
        this->clearBtn->setDescription("Clear all city geometry and delete the cache file.");
        this->generateBuildingsBtn->setDescription("Regenerate ONLY building geometry — roads and their mesh are left completely unchanged.\n"
                                                   "Workflow: draw custom roads with ProceduralRoadComponent → press this button to fill the city.\n"
                                                   "cityOrigin is captured from the road component's current SceneNode so buildings align exactly with roads.");
        this->editModeAttr->setDescription("Object: standard camera/transform mode.\n"
                                           "Segment: left-click a building to select it (highlighted in log), press X to delete it.\n"
                                           "Ctrl+Z to undo a deletion. Use this to remove buildings placed on roads or in undesired spots.");
        this->editModeAttr->addUserData(GameObject::AttrActionNoUndo());

        this->setDistrictCount(9u);
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

        this->cityLoadedFromScene = true;

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
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrGradientAlignment())
        {
            this->gradientAlignmentAttr->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrRoadComponentId())
        {
            this->roadComponentIdAttr->setValue(XMLConverter::getAttrib(propertyElement, "data", "0"));
            this->roadComponentId = static_cast<unsigned long>(std::strtoul(this->roadComponentIdAttr->getString().c_str(), nullptr, 10));
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
            for (unsigned int v = 0; v < 6u; ++v)
            {
                Ogre::String key = AttrDistrictFaceDatablock() + idx + "_" + Ogre::StringConverter::toString(v);
                if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == key)
                {
                    this->setDistrictFaceDatablock(i, v, XMLConverter::getAttrib(propertyElement, "data"));
                    propertyElement = propertyElement->next_sibling("property");
                }
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
                if (v == 1)
                {
                    this->districtTrimDbAttrs[i][v]->addUserData(GameObject::AttrActionSeparator());
                }
            }
        }

        return true;
    }

    bool ProceduralCityComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralCityComponent] postInit for: " + this->gameObjectPtr->getName());

        AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &ProceduralCityComponent::handleComponentManuallyDeleted), EventDataDeleteComponent::getStaticEventType());
        AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &ProceduralCityComponent::handleGameObjectSelected), NOWA::EventDataGameObjectSelected::getStaticEventType());

        this->gameObjectPtr->changeCategory("City");

        this->groundQuery = this->gameObjectPtr->getSceneManager()->createRayQuery(Ogre::Ray(), AppStateManager::getSingletonPtr()->getGameObjectController()->generateCategoryId("All-Road"));
        this->groundQuery->setSortByDistance(true);
        this->groundPlane = Ogre::Plane(Ogre::Vector3::UNIT_Y, 0.0f);

        this->gameObjectPtr->setAttributePosition(Ogre::Vector3::ZERO);

        this->createSelectionOverlay();

        const auto& physicsArtifactCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsArtifactComponent>());
        if (physicsArtifactCompPtr)
        {
            this->physicsArtifactComponent = physicsArtifactCompPtr.get();
        }

        // ---- Create or reconnect the CityRoads child GameObject ---------------
        // Roads live on their own GO so each can carry an independent
        // PhysicsArtifactComponent (a GO may have only one Ogre::Item).
        // Pattern mirrors UniversumComponent::createPlanet().
        //
        // Reconnect path (scene loaded from disk):
        //   roadComponentId > 0 → the CityRoads GO was saved with the scene.
        //   findRoadComponent() will find it once the scene finishes parsing.
        //   We don't re-create here to avoid duplicates on load.
        //
        // First-time / cleared path (roadComponentId == 0):
        //   Create a new GO named "CityRoads_<ownerId>", add PRC to it,
        //   call postInit, then store its ID in roadComponentId so it's
        //   persisted on the next scene save.
        if (0ul == this->roadComponentId)
        {
            Ogre::SceneManager* sm = this->gameObjectPtr->getSceneManager();

            Ogre::String roadsName = "CityRoads_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
            AppStateManager::getSingletonPtr()->getGameObjectController()->getValidatedGameObjectName(roadsName);

            Ogre::SceneNode* roadsNode = sm->getRootSceneNode()->createChildSceneNode(Ogre::SCENE_DYNAMIC);
            roadsNode->setName(roadsName);
            roadsNode->setPosition(this->gameObjectPtr->getSceneNode()->_getDerivedPosition());

            GameObjectPtr roadsGo = GameObjectFactory::getInstance()->createGameObject(sm, roadsNode, nullptr, NOWA::ITEM, NOWA::makeUniqueID());

            if (nullptr != roadsGo)
            {
                auto prcRaw = GameObjectFactory::getInstance()->createComponentDeferred(roadsGo, ProceduralRoadComponent::getStaticClassName());

                if (nullptr != prcRaw)
                {
                    // Forward datablock settings from city attributes to the PRC
                    prcRaw->getAttribute(ProceduralRoadComponent::AttrCenterDatablock())->setValue(this->roadDatablockAttr->getString());
                    prcRaw->getAttribute(ProceduralRoadComponent::AttrEdgeDatablock())->setValue(this->curbDatablockAttr->getString());
                    prcRaw->postInit();
                }

                this->roadComponentId = roadsGo->getId();
                this->roadComponentIdAttr->setValue(Ogre::StringConverter::toString(this->roadComponentId));

                // Remove the old ProceduralRoadComponent from the city GO if one was
                // left there from before this refactor.  Roads now live exclusively on
                // CityRoads_N — the city GO must be free so PhysicsArtifactComponent
                // can apply to buildings only.
                auto oldPrc = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<ProceduralRoadComponent>());
                if (nullptr != oldPrc)
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralCityComponent] Removing legacy ProceduralRoadComponent "
                                                                                        "from city GO — roads have been migrated to CityRoads GO.");
                    this->gameObjectPtr->deleteComponent<ProceduralRoadComponent>(oldPrc->getIndex());
                }

                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                    "[ProceduralCityComponent] Created CityRoads GO '" + roadsName + "' (id=" + Ogre::StringConverter::toString(this->roadComponentId) + ") with ProceduralRoadComponent.");
            }
            else
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralCityComponent] WARNING: failed to create CityRoads GO.");
            }
        }
        else
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralCityComponent] CityRoads GO id=" + Ogre::StringConverter::toString(this->roadComponentId) + " will be reconnected after scene is parsed.");
        }

        if (true == this->cityLoadedFromScene)
        {
            // Loaded from a saved scene is NOT regenerated here.
            // postInit() runs once per GameObject during scene load, but other
            // GameObjects (e.g. obstacles like Wall_0, container_0) may not have
            // completed their own postInit() yet, so their category bits are not
            // guaranteed to be registered in the scene query structures. Querying
            // isCategoryAllowed() here can silently find no obstacles -- foliage
            // gets placed right through buildings.
            //
            // lateInit() (below) is called once, after EVERY GameObject in the
            // scene has finished postInit() -- see DotSceneImportModule::
            // postInitData(). That is the correct, safe point to regenerate
            this->cityLoadedFromScene = false;
            AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &ProceduralCityComponent::handleSceneParsed), EventDataSceneParsed::getStaticEventType());
        }

        return true;
    }

    void ProceduralCityComponent::handleSceneParsed(NOWA::EventDataPtr eventData)
    {
        this->loadOrGenerateCity();
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
        AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ProceduralCityComponent::handleGameObjectSelected), NOWA::EventDataGameObjectSelected::getStaticEventType());

        this->removeInputListener();
        this->destroySelectionOverlay();

        this->clearCity(false);

        // Destroy the CityRoads child GO we created in postInit.
        // Do this AFTER clearCity so the road component is still alive
        // when clearAllSegments / destroyRoadMesh are called during clearCity.
        if (this->roadComponentId != 0ul)
        {
            AppStateManager::getSingletonPtr()->getGameObjectController()->deleteGameObject(this->roadComponentId);
            this->roadComponentId = 0ul;
            this->roadComponentIdAttr->setValue(Ogre::String("0"));
        }

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

        if (AttrCityBounds() == attribute->getName())
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
        else if (AttrDoorDatablock() == attribute->getName())
        {
            this->setDoorDatablock(attribute->getString());
        }
        else if (AttrGarageDatablock() == attribute->getName())
        {
            this->setGarageDatablock(attribute->getString());
        }
        else if (AttrGenerateGarage() == attribute->getName())
        {
            this->generateGarageAttr->setValue(attribute->getBool());
        }
        else if (AttrGradientAlignment() == attribute->getName())
        {
            this->gradientAlignmentAttr->setValue(attribute->getBool());
        }
        else if (AttrEditMode() == attribute->getName())
        {
            this->editModeAttr->setListSelectedValue(attribute->getListSelectedValue());
            this->updateModificationState();
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralCityComponent] EditMode changed to: " + attribute->getListSelectedValue());
        }
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
                    for (unsigned int v = 0; v < 6u; ++v)
                    {
                        if (AttrDistrictFaceDatablock() + idx + "_" + Ogre::StringConverter::toString(v) == attribute->getName())
                        {
                            this->setDistrictFaceDatablock(i, v, attribute->getString());
                        }
                    }
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
        propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrCityBounds())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->cityBoundsAttr->getVector4())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrMasterSeed())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->masterSeedAttr->getUInt())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrBlockSize())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->blockSizeAttr->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrRoadWidth())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->roadWidthAttr->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrRoadVariance())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->roadVarianceAttr->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrSidewalkWidth())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->sidewalkWidthAttr->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrCurbHeight())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->curbHeightAttr->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrGenerateRoads())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->generateRoadsAttr->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrRoadDatablock())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->roadDatablockAttr->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrCurbDatablock())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->curbDatablockAttr->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrDoorDatablock())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->doorDatablockAttr->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrGarageDatablock())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->garageDatablockAttr->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrGenerateGarage())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->generateGarageAttr->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrGradientAlignment())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->gradientAlignmentAttr->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrRoadComponentId())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->roadComponentIdAttr->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrRoadConnectionAtStart())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->roadConnectionAtStartAttr->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(rapidxml::node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrDistrictCount())));
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

            for (unsigned int v = 0; v < 6u; ++v)
            {
                propertyXML = doc.allocate_node(rapidxml::node_element, "property");
                propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
                propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, AttrDistrictFaceDatablock() + idx + "_" + Ogre::StringConverter::toString(v))));
                propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->getDistrictFaceDatablock(i, v))));
                propertiesXML->append_node(propertyXML);
            }
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
        if (actionId == ActionGenerateBuildings())
        {
            this->generateBuildingsOnly();
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
        // ProceduralRoadComponent is NO LONGER auto-created here.
        // Roads live on a dedicated CityRoads_N child GameObject (created in postInit).
        // This keeps the city GO free for PhysicsArtifactComponent to use the
        // building geometry Item without road mesh interference.
        desc.autoComponents = {ProceduralCityComponent::getStaticClassName()};
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
        // Clear the CityRoads GO road network (not the city GO's PRC).
        ProceduralRoadComponent* roadCompForClear = this->findRoadComponent();
        if (nullptr != roadCompForClear)
        {
            roadCompForClear->clearAllSegments();
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
        }

        // Only destroy BUILDING batches here — NOT the road.
        // The road is owned and reloaded independently by ProceduralRoadComponent's
        // own handleSceneParsed. Touching it here races against that load.
        GraphicsModule::getInstance()->enqueueAndWait(
            [this]()
            {
                this->destroyCityOnRenderThread();
            },
            "ProceduralCityComponent::loadOrGenerateCity_DestroyBuildings");

        std::vector<CityBatch> batches;
        bool fromCache = this->loadCityDataFromFile(batches);

        if (true == fromCache)
        {
            // cityOrigin is NOT persisted in the batch cache file and is otherwise only
            // ever set inside generateCityLayout(). On a cache-hit load, generateCityLayout()
            // never runs, so cityOrigin would silently stay at its default ZERO — placing
            // the rebuilt city Item at world origin instead of the GameObject's actual
            // (correctly loaded) scene position. Derive it from the SceneNode instead,
            // same as generateBuildingsOnly() already does.
            this->cityOrigin = this->gameObjectPtr->getSceneNode()->_getDerivedPositionUpdated();
        }
        else
        {
            // We're about to regenerate the road network ourselves (generateCityLayout
            // calls buildCityRoadNetwork internally) — NOW it's safe/correct to clear
            // whatever the road component currently has, since we're replacing it.
            ProceduralRoadComponent* roadCompForClear = this->findRoadComponent();
            if (nullptr != roadCompForClear)
            {
                roadCompForClear->clearAllSegments();
            }

            batches = this->generateCityLayout(); // also calls buildCityRoadNetwork + syncs
            if (batches.empty())
            {
                return;
            }
            this->saveCityDataToFile(batches);
        }

        this->createCityOnRenderThread(std::move(batches));
        this->isDirty = false;
    }

    // =========================================================================
    // generateBuildingsOnly
    // =========================================================================
    void ProceduralCityComponent::generateBuildingsOnly(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralCityComponent] generateBuildingsOnly: rebuilding BUILDINGS only, "
                                                                            "ProceduralRoadComponent mesh is untouched.");

        // Destroy old building batches on the render thread.
        // ProceduralRoadComponent owns its own Ogre::Item — destroyCityOnRenderThread()
        // ONLY removes city batch items (buildings), NOT the road component's mesh. ✓
        GraphicsModule::getInstance()->enqueueAndWait(
            [this]()
            {
                this->destroyCityOnRenderThread();
            },
            "ProceduralCityComponent::GenerateBuildingsOnly");

        // Capture cityOrigin from the road component's current SceneNode position.
        // ProceduralRoadComponent::createRoadMeshInternal() called setPosition(roadOrigin)
        // when roads were built.  Using that same position keeps buildings in the same
        // local coordinate space as the road mesh so they always align correctly.
        this->cityOrigin = this->gameObjectPtr->getSceneNode()->_getDerivedPositionUpdated();

        // Keep organicRoadSegs from the previous organic generation.
        // When the designer uses "Generate Buildings", roads stay as-is and we want
        // building placement to respect those road positions.  If roads were built
        // organically, the stored segments are still valid exclusion zones.
        // Only clear them if no organic segments exist (grid-mode or first run).
        // organicRoadSegs is NOT cleared here — we intentionally preserve it.

        // Generate building layout (skip road build)
        std::vector<CityBatch> batches = this->generateCityLayout(true /*skipRoads*/);

        // Persist to cache and create GPU meshes
        this->saveCityDataToFile(batches);
        GraphicsModule::getInstance()->enqueueAndWait(
            [this, batches]() mutable
            {
                this->createCityOnRenderThread(std::move(batches));
            },
            "ProceduralCityComponent::CreateBuildingsOnly");

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralCityComponent] generateBuildingsOnly: done.");
    }

    // =========================================================================
    // generateCityLayout — logic thread
    // skipRoads: when true, skips buildCityRoadNetwork() and captures cityOrigin
    //            from the current SceneNode (road component's existing position).
    // =========================================================================

    std::vector<CityBatch> ProceduralCityComponent::generateCityLayout(bool skipRoads)
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
                    h ^= h >> 16;
                    h *= 0x45d9f3bu;
                    h ^= h >> 16;
                    const Ogre::Real shift = (static_cast<Ogre::Real>(h & 0xFFFFu) / 65535.f * 2.f - 1.f) * maxShift;
                    this->cityGridX[i] = std::max(this->cityGridX[i - 1] + blockSz * 0.35f, std::min(this->cityGridX[i + 1] - blockSz * 0.35f, this->cityGridX[i] + shift));
                }

                // Perturb inner Z lines
                for (size_t i = 1; i + 1 < this->cityGridZ.size(); ++i)
                {
                    uint32_t h = (static_cast<uint32_t>(i + 100u) * 2654435761u) ^ mSeed;
                    h ^= h >> 16;
                    h *= 0x45d9f3bu;
                    h ^= h >> 16;
                    const Ogre::Real shift = (static_cast<Ogre::Real>(h & 0xFFFFu) / 65535.f * 2.f - 1.f) * maxShift;
                    this->cityGridZ[i] = std::max(this->cityGridZ[i - 1] + blockSz * 0.35f, std::min(this->cityGridZ[i + 1] - blockSz * 0.35f, this->cityGridZ[i] + shift));
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
                blk.centre = Ogre::Vector3((this->cityGridX[xi] + this->cityGridX[xi + 1]) * 0.5f, 0.f, (this->cityGridZ[zi] + this->cityGridZ[zi + 1]) * 0.5f);
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
        // =====================================================================
        // Road build (skipped when called from generateBuildingsOnly)
        // =====================================================================
        if (false == skipRoads && this->generateRoadsAttr->getBool())
        {
            // Build road first (moves SceneNode to roadOrigin via ProceduralRoadComponent)
            this->buildCityRoadNetwork(blocks);
            // Now capture that exact position as cityOrigin
            this->cityOrigin = this->gameObjectPtr->getSceneNode()->_getDerivedPositionUpdated();
        }
        else if (skipRoads)
        {
            // Roads are untouched.  cityOrigin was already captured in generateBuildingsOnly()
            // from the road component's current SceneNode, so nothing to do here.
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralCityComponent] generateCityLayout: road build skipped (skipRoads=true).");
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

        // Buildings are placed on the perturbed grid.
        // Phase 2a: each building is oriented to face its nearest organic road segment.
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

                // ---- Road-proximity filter (Step 3) --------------------------------
                // Only place a building if the lot centre is close to at least one road
                // segment.  This creates the AAA-game pattern: buildings line streets
                // and open space exists between road corridors.
                // nearThreshold = how far from road centre a building can be (inward).
                // onRoadThreshold = road surface + sidewalk — lots ON the road are skipped.
                {
                    const Ogre::Real onRoadThresh = this->roadWidthAttr->getReal() * 0.5f + this->sidewalkWidthAttr->getReal();
                    const Ogre::Real nearThresh = onRoadThresh + this->blockSizeAttr->getReal() * 0.55f;
                    const Ogre::Vector2 lotXZ(lot.centre.x, lot.centre.z);

                    auto distToSeg = [](const Ogre::Vector2& p, const Ogre::Vector2& a, const Ogre::Vector2& b) -> Ogre::Real
                    {
                        const Ogre::Vector2 ab = b - a;
                        const Ogre::Real ab2 = ab.dotProduct(ab);
                        if (ab2 < 1e-6f)
                        {
                            return (p - a).length();
                        }
                        const Ogre::Real t = std::max(0.f, std::min(1.f, (p - a).dotProduct(ab) / ab2));
                        return (p - (a + t * ab)).length();
                    };

                    Ogre::Real minDist = std::numeric_limits<Ogre::Real>::max();

                    // Check organic artery segments
                    for (const auto& seg : this->organicRoadSegs)
                    {
                        const Ogre::Real d = distToSeg(lotXZ, seg.a, seg.b);
                        if (d < minDist)
                        {
                            minDist = d;
                        }
                        if (minDist < onRoadThresh)
                        {
                            break;
                        } // on road surface
                    }

                    // Check grid road lines (only when no organic roads were close enough)
                    if (minDist >= onRoadThresh)
                    {
                        for (const auto& gx : this->cityGridX)
                        {
                            const Ogre::Real d = std::abs(lotXZ.x - gx);
                            if (d < minDist)
                            {
                                minDist = d;
                            }
                        }
                        for (const auto& gz : this->cityGridZ)
                        {
                            const Ogre::Real d = std::abs(lotXZ.y - gz);
                            if (d < minDist)
                            {
                                minDist = d;
                            }
                        }
                    }

                    if (minDist < onRoadThresh)
                    {
                        continue;
                    } // on road surface — skip
                    if (minDist > nearThresh)
                    {
                        continue;
                    } // too far from any road — skip
                }

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

                // ---- Gradient alignment (planet surface tilt) --------------------
                // Compute gradientQ but do NOT bake it into building.orientation yet.
                // generateBuildingGeometry uses UNIT_Y for heights → correct face winding.
                // After generation we rotate all new vertices + normals + tangents
                // by gradientQ as a post-process.  This way:
                //   • winding is always correct (generated with standard UNIT_Y up)
                //   • roof, walls, stairs ALL tilt together with the building
                //   • road-facing direction (building.orientation) stays unaffected
                Ogre::Quaternion gradientQ = Ogre::Quaternion::IDENTITY;
                if (this->gradientAlignmentAttr->getBool())
                {
                    const Ogre::Vector3 worldPos(building.position.x, building.groundHeight, building.position.z);
                    const Ogre::Vector3 toSurface = worldPos;
                    // Guard: if groundHeight is much lower than localOrigin.y the terrain raycast
                    // missed (building outside Terra mesh bounds). Surface normal would be
                    // horizontal → 90° rotation → buildings become flat planes in space.
                    const bool terrainValid = (building.groundHeight > localOrigin.y * 0.3f) && (toSurface.squaredLength() > 1.f);
                    if (terrainValid)
                    {
                        const Ogre::Vector3 surfNorm = toSurface.normalisedCopy();
                        const Ogre::Vector3 axis = Ogre::Vector3::UNIT_Y.crossProduct(surfNorm);
                        const Ogre::Real dot = Ogre::Vector3::UNIT_Y.dotProduct(surfNorm);
                        if (axis.squaredLength() < 1e-6f)
                        {
                            gradientQ = (dot >= 0.f) ? Ogre::Quaternion::IDENTITY : Ogre::Quaternion(0.f, 1.f, 0.f, 0.f);
                        }
                        else
                        {
                            gradientQ.FromAngleAxis(Ogre::Math::ACos(Ogre::Math::Clamp(dot, -1.f, 1.f)), axis.normalisedCopy());
                        }
                        building.orientation = gradientQ * building.orientation;
                    }
                }

                Ogre::Real fw = Ogre::Math::lerp(district.minFootprint.x, district.maxFootprint.x, d01(rng));
                Ogre::Real fd = Ogre::Math::lerp(district.minFootprint.y, district.maxFootprint.y, d01(rng));
                fw = std::min(fw, lot.size.x * 0.85f);
                fd = std::min(fd, lot.size.y * 0.85f);
                Ogre::Real fh = Ogre::Math::lerp(district.minHeight, district.maxHeight, d01(rng));
                building.footprint = Ogre::Vector3(fw, fh, fd);

                const bool isHighRise = (district.type == "Tower" || district.type == "Industrial");
                building.roofType = isHighRise ? 0u : (building.variantSeed % 3u);
                building.roofPitch = isHighRise ? 0.f : (0.2f + d01(rng) * 0.55f);

                const unsigned int vi = building.variantSeed % NUM_BUILDING_VARIANTS;
                const unsigned int base = vi * SLOTS_PER_VARIANT;

                this->generateBuildingGeometry(building, district, localOrigin, batches[base + 0].rawVertices, batches[base + 0].rawIndices, batches[base + 0].numVertices, batches[base + 1].rawVertices, batches[base + 1].rawIndices,
                    batches[base + 1].numVertices, batches[base + 2].rawVertices, batches[base + 2].rawIndices, batches[base + 2].numVertices, batches[base + 3].rawVertices, batches[base + 3].rawIndices, batches[base + 3].numVertices,
                    batches[base + 4].rawVertices, batches[base + 4].rawIndices, batches[base + 4].numVertices, batches[base + 5].rawVertices, batches[base + 5].rawIndices, batches[base + 5].numVertices);

                // Store for Segment-mode selection/deletion
                this->storedBuildings.push_back(building);
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

    void ProceduralCityComponent::generateBuildingGeometry(const BuildingInstance& building, const CityDistrict& district, const Ogre::Vector3& localOrigin, std::vector<float>& wallV, std::vector<Ogre::uint32>& wallI, size_t& wallN,
        std::vector<float>& roofV, std::vector<Ogre::uint32>& roofI, size_t& roofN, std::vector<float>& windowV, std::vector<Ogre::uint32>& windowI, size_t& windowN, std::vector<float>& trimV, std::vector<Ogre::uint32>& trimI, size_t& trimN,
        std::vector<float>& doorV, std::vector<Ogre::uint32>& doorI, size_t& doorN, std::vector<float>& garageV, std::vector<Ogre::uint32>& garageI, size_t& garageN)
    {
        const Ogre::Vector3 wallDir = building.orientation * Ogre::Vector3::UNIT_Z;
        // up = orientation * UNIT_Y.
        // On flat terrain (orientation = yaw-only): up = UNIT_Y — no change.
        // With gradient alignment on a planet: up = planet surface normal at this building.
        // Using 'up' instead of hardcoded UNIT_Y makes all wall heights, roof, trim, stairs
        // and garage tilt correctly with the building relative to the planet surface.
        const Ogre::Vector3 up = building.orientation * Ogre::Vector3::UNIT_Y;
        const Ogre::Vector3 right = wallDir.crossProduct(up).normalisedCopy();

        const Ogre::Real hw = building.footprint.x * 0.5f; // half-width
        const Ogre::Real hd = building.footprint.z * 0.5f; // half-depth
        const Ogre::Real wh = building.footprint.y;        // wall height

        // Y fix: groundHeight applied once only.  CITY_SINK_DEPTH pulls the building
        // slightly below terrain surface to close any gap on sloped ground.
        // All coordinates are LOCAL — world position minus localOrigin.
        // Same origin-subtraction pattern as ProceduralRoadComponent.
        const Ogre::Vector3 base(building.position.x - localOrigin.x, building.groundHeight - CITY_SINK_DEPTH - localOrigin.y, building.position.z - localOrigin.z);

        // Eight corners (bottom, then top)
        const Ogre::Vector3 flb = base - right * hw - wallDir * hd;
        const Ogre::Vector3 frb = base + right * hw - wallDir * hd;
        const Ogre::Vector3 brb = base + right * hw + wallDir * hd;
        const Ogre::Vector3 blb = base - right * hw + wallDir * hd;
        const Ogre::Vector3 flt = flb + (up * wh);
        const Ogre::Vector3 frt = frb + (up * wh);
        const Ogre::Vector3 brt = brb + (up * wh);
        const Ogre::Vector3 blt = blb + (up * wh);

        const Ogre::Real wallTile = 3.f;
        const Ogre::Real fW = building.footprint.x; // face widths
        const Ogre::Real fD = building.footprint.z;

        const Ogre::Real trimH = std::min(1.5f, wh * 0.2f);

        // ---- Wall faces (above trim) — slot 0 --------------------------------
        // Front face: v0=flb+trim v1=frb+trim v2=frt v3=flt, normal = -wallDir
        this->pushQuad(wallV, wallI, wallN, flb + (up * trimH), frb + (up * trimH), frt, flt, -wallDir, fW / wallTile, (wh - trimH) / wallTile);

        // Back face: v0=brb+trim v1=blb+trim v2=blt v3=brt, normal = +wallDir
        this->pushQuad(wallV, wallI, wallN, brb + (up * trimH), blb + (up * trimH), blt, brt, wallDir, fW / wallTile, (wh - trimH) / wallTile);

        // Left face: v0=blb+trim v1=flb+trim v2=flt v3=blt, normal = -right
        // Direction v0->v1 = flb - blb = -wallDir (correct faceRight for windows)
        this->pushQuad(wallV, wallI, wallN, blb + (up * trimH), flb + (up * trimH), flt, blt, -right, fD / wallTile, (wh - trimH) / wallTile);

        // Right face: v0=frb+trim v1=brb+trim v2=brt v3=frt, normal = +right
        // Direction v0->v1 = brb - frb = +wallDir (correct faceRight for windows)
        this->pushQuad(wallV, wallI, wallN, frb + (up * trimH), brb + (up * trimH), brt, frt, right, fD / wallTile, (wh - trimH) / wallTile);

        // Parapet cap — normal = +UNIT_Y
        this->pushQuad(wallV, wallI, wallN, flt, frt, brt, blt, up, fW / wallTile, fD / wallTile);

        // ---- Trim strip — slot 3 (ground-level band) -------------------------
        this->pushQuad(trimV, trimI, trimN, flb, frb, frb + (up * trimH), flb + (up * trimH), -wallDir, fW / 1.5f, 1.f);
        this->pushQuad(trimV, trimI, trimN, brb, blb, blb + (up * trimH), brb + (up * trimH), wallDir, fW / 1.5f, 1.f);
        this->pushQuad(trimV, trimI, trimN, blb, flb, flb + (up * trimH), blb + (up * trimH), -right, fD / 1.5f, 1.f);
        this->pushQuad(trimV, trimI, trimN, frb, brb, brb + (up * trimH), frb + (up * trimH), right, fD / 1.5f, 1.f);

        // ---- Roof — slot 1 ---------------------------------------------------
        const Ogre::Real roofH = hw * 0.5f + building.roofPitch * hw;

        if (building.roofType == 0 || wh > 15.f)
        {
            // Flat: already capped by parapet
        }
        else if (building.roofType == 1)
        {
            // Gabled: ridge along wallDir
            // Ridge peak rises in building's 'up' direction (= surface normal on planet).
            // On flat terrain up=UNIT_Y so this is identical to the old formula.
            const Ogre::Vector3 ridge = base + up * (wh + roofH);
            const Ogre::Real ridgeHL = hd * 0.6f;
            const Ogre::Vector3 ridgeF = ridge - wallDir * ridgeHL;
            const Ogre::Vector3 ridgeB = ridge + wallDir * ridgeHL;

            Ogre::Vector3 sNL = (-right + (up * 1.f)).normalisedCopy();
            Ogre::Vector3 sNR = (right + (up * 1.f)).normalisedCopy();
            this->pushQuad(roofV, roofI, roofN, flt, ridgeF, ridgeB, blt, sNL, fD / wallTile, 1.f);
            this->pushQuad(roofV, roofI, roofN, ridgeF, frt, brt, ridgeB, sNR, fD / wallTile, 1.f);
            // Gable ends (triangle as degenerate quad — collapse v2==v3)
            this->pushQuad(roofV, roofI, roofN, flt, frt, ridgeF, ridgeF, -wallDir, fW / wallTile, 1.f);
            this->pushQuad(roofV, roofI, roofN, brt, blt, ridgeB, ridgeB, wallDir, fW / wallTile, 1.f);
        }
        else
        {
            // Hip: four slopes converging to a single apex
            // Apex rises in building's 'up' direction (= surface normal on planet).
            const Ogre::Vector3 apex = base + up * (wh + roofH);
            Ogre::Vector3 sNF = (-wallDir + (up * 1.f)).normalisedCopy();
            Ogre::Vector3 sNB = (wallDir + (up * 1.f)).normalisedCopy();
            Ogre::Vector3 sNL = (-right + (up * 1.f)).normalisedCopy();
            Ogre::Vector3 sNR = (right + (up * 1.f)).normalisedCopy();
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
            const Ogre::Real winStartY = std::max(trimH, doorHInner + 0.5f); // 0.5m gap above door
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
                        const Ogre::Vector3 wc = originBL + faceRight * cx + (up * cy) + pushOut;
                        const Ogre::Vector3 wv0 = wc - faceRight * (winW * 0.5f) - (up * winH * 0.5f);
                        const Ogre::Vector3 wv1 = wc + faceRight * (winW * 0.5f) - (up * winH * 0.5f);
                        const Ogre::Vector3 wv2 = wc + faceRight * (winW * 0.5f) + (up * winH * 0.5f);
                        const Ogre::Vector3 wv3 = wc - faceRight * (winW * 0.5f) + (up * winH * 0.5f);
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
            const Ogre::Real doorH = std::min(2.2f, wh * 0.45f); // always a reasonable door height
            const Ogre::Real doorW = std::min(1.2f, fW * 0.25f);
            const Ogre::Real doorOff = 0.05f;
            const Ogre::Vector3 pushOut = (-wallDir) * doorOff;

            // World XZ of the front face centre
            const Ogre::Real frontWorldX = building.position.x + (-wallDir.x) * hd;
            const Ogre::Real frontWorldZ = building.position.z + (-wallDir.z) * hd;
            const Ogre::Real frontTerrainY = this->getGroundHeight(Ogre::Vector3(frontWorldX, 0.f, frontWorldZ));
            // Door base in local space: never lower than the actual front-face terrain
            const Ogre::Real doorBaseLocal = std::max(base.y, frontTerrainY - CITY_SINK_DEPTH - localOrigin.y);

            const Ogre::Vector3 frontBaseCentre = Ogre::Vector3(base.x + (-wallDir.x) * hd, doorBaseLocal, base.z + (-wallDir.z) * hd);

            const Ogre::Vector3 dc = frontBaseCentre + right * 0.f // centred
                                     + (up * doorH * 0.5f)         // mid-height
                                     + pushOut;

            const Ogre::Vector3 dv0 = dc - right * (doorW * 0.5f) - (up * doorH * 0.5f);
            const Ogre::Vector3 dv1 = dc + right * (doorW * 0.5f) - (up * doorH * 0.5f);
            const Ogre::Vector3 dv2 = dc + right * (doorW * 0.5f) + (up * doorH * 0.5f);
            const Ogre::Vector3 dv3 = dc - right * (doorW * 0.5f) + (up * doorH * 0.5f);
            this->pushQuad(doorV, doorI, doorN, dv0, dv1, dv2, dv3, -wallDir, 1.f, 1.f);
        }

        // ---- Stairs — 3 steps in front of the door (wall geometry slot) ------
        // Steps run outward from the building front face in the -wallDir direction.
        // Step 0 = lowest/outermost, step 2 = highest/closest to door.
        // Each step has a TOP face (normal +Y) and a FRONT face (normal -wallDir).
        // Width = doorW + 0.6m, height per step = 0.15m, depth per step = 0.28m.
        // Winding convention: Cross(e1,e2) = -intended_normal (same as all other faces).
        //
        // Winding proof (reusing house-wall convention):
        //   TOP face  (+Y): FrontLeft,FrontRight,BackRight,BackLeft  → Cross = right×wallDir = -Y ✓
        //   FRONT face(-wallDir): BotLeft,BotRight,TopRight,TopLeft  → Cross = right×Y = +wallDir ✓
        {
            const Ogre::Real doorW = std::min(1.2f, fW * 0.25f);
            const Ogre::Real stairH = 0.15f;
            const Ogre::Real stairD = 0.28f;
            const int nSteps = 3;
            const Ogre::Real stairHW = doorW * 0.5f + 0.3f;

            // Front face centre at door-base height — same origin as the door block.
            const Ogre::Real frontWorldX2 = building.position.x + (-wallDir.x) * hd;
            const Ogre::Real frontWorldZ2 = building.position.z + (-wallDir.z) * hd;
            const Ogre::Real frontTerrainY2 = this->getGroundHeight(Ogre::Vector3(frontWorldX2, 0.f, frontWorldZ2));
            const Ogre::Real doorBaseLocal2 = std::max(base.y, frontTerrainY2 - CITY_SINK_DEPTH - localOrigin.y);

            // Use frontBase (= front-face centre at door-base Y) as stair origin.
            // FIX: was "baseXZ(base.x, 0, base.z)" which hardcoded Y=0 — wrong on planet.
            const Ogre::Vector3 frontBase(base.x + (-wallDir.x) * hd, doorBaseLocal2, base.z + (-wallDir.z) * hd);

            const Ogre::Vector3 rL = right * (-stairHW);
            const Ogre::Vector3 rR = right * stairHW;

            for (int s = 0; s < nSteps; ++s)
            {
                // FIX: use RELATIVE heights above frontBase, not absolute local-Y values.
                // Old code used absolute botY/topY (~= base.y + steps) multiplied by the
                // tilted 'up' vector — this displaced stairs wildly on a planet.
                const Ogre::Real botRelH = static_cast<Ogre::Real>(s) * stairH;
                const Ogre::Real topRelH = static_cast<Ogre::Real>(s + 1) * stairH;

                // Outward distance FROM frontBase (outer = further from building)
                const Ogre::Real outDist = static_cast<Ogre::Real>(nSteps - s) * stairD;
                const Ogre::Real inDist = static_cast<Ogre::Real>(nSteps - s - 1) * stairD;

                const Ogre::Vector3 cOut = (-wallDir) * outDist;
                const Ogre::Vector3 cIn = (-wallDir) * inDist;

                // ---- TOP face (normal = up) ----------------------------------------
                const Ogre::Vector3 topFL = frontBase + rL + cOut + up * topRelH;
                const Ogre::Vector3 topFR = frontBase + rR + cOut + up * topRelH;
                const Ogre::Vector3 topBR = frontBase + rR + cIn + up * topRelH;
                const Ogre::Vector3 topBL = frontBase + rL + cIn + up * topRelH;
                this->pushQuad(wallV, wallI, wallN, topFL, topFR, topBR, topBL, up, stairHW * 2.f / 0.5f, stairD / 0.5f);

                // ---- FRONT face (normal = -wallDir) -----------------------------------
                const Ogre::Vector3 frBL = frontBase + rL + cOut + up * botRelH;
                const Ogre::Vector3 frBR = frontBase + rR + cOut + up * botRelH;
                const Ogre::Vector3 frTR = frontBase + rR + cOut + up * topRelH;
                const Ogre::Vector3 frTL = frontBase + rL + cOut + up * topRelH;
                this->pushQuad(wallV, wallI, wallN, frBL, frBR, frTR, frTL, -wallDir, stairHW * 2.f / 0.5f, stairH / 0.25f);

                // ---- SIDE faces -------------------------------------------------------
                const Ogre::Real sideTopRelH = static_cast<Ogre::Real>(s + 1) * stairH;

                // Left side (normal = -right)
                const Ogre::Vector3 lBI = frontBase + rL + cIn + up * 0.f;
                const Ogre::Vector3 lBO = frontBase + rL + cOut + up * 0.f;
                const Ogre::Vector3 lTO = frontBase + rL + cOut + up * sideTopRelH;
                const Ogre::Vector3 lTI = frontBase + rL + cIn + up * sideTopRelH;
                this->pushQuad(wallV, wallI, wallN, lBI, lBO, lTO, lTI, -right, stairD / 0.5f, sideTopRelH / 0.5f);

                // Right side (normal = +right)
                const Ogre::Vector3 rBI = frontBase + rR + cIn + up * 0.f;
                const Ogre::Vector3 rBO = frontBase + rR + cOut + up * 0.f;
                const Ogre::Vector3 rTO = frontBase + rR + cOut + up * sideTopRelH;
                const Ogre::Vector3 rTI = frontBase + rR + cIn + up * sideTopRelH;
                this->pushQuad(wallV, wallI, wallN, rBO, rBI, rTI, rTO, right, stairD / 0.5f, sideTopRelH / 0.5f);
            }
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
        const bool isGarageType = (district.type == "Residential_Low" || district.type == "Residential_Mid" || district.type == "Mixed");
        const bool generateGarage = this->generateGarageAttr->getBool();

        // Space check: garage extends gd meters to the right of the house right wall.
        // building.lotHalfRight is the lot's half-width in the right direction (from the
        // building's world position ≈ lot center to the road boundary).
        // Available space = lotHalfRight - hw (building already occupies hw from center).
        // Use 1m safety margin so the garage never touches the sidewalk edge.
        const Ogre::Real spaceRight = std::max(0.f, building.lotHalfRight - hw - 1.0f);
        const Ogre::Real gd = std::min({4.0f, spaceRight, fW * 0.4f});

        const bool hasGarage = generateGarage && isGarageType && ((building.variantSeed % 10u) < 3u) && (gd >= 2.0f) && (fD > 5.f);
        if (hasGarage)
        {
            const Ogre::Real gw = std::min(4.5f, fD * 0.45f); // garage width (wallDir, from front)
            const Ogre::Real gh = std::min(2.8f, wh * 0.4f);  // garage height (lower than house)

            // Corner positions in local space.
            // Garage is at the FRONT of the house (at -wallDir*hd), extending +wallDir*gw inward.
            // FL/FR = front (near street), BL/BR = back (away from street).
            // NL/NR subscript: N=Near house wall (right*hw), F=Far (right*(hw+gd)).
            const Ogre::Vector3 FL = base + right * hw + (-wallDir) * hd;
            const Ogre::Vector3 FR = base + right * (hw + gd) + (-wallDir) * hd;
            const Ogre::Vector3 BL = base + right * hw + (-wallDir) * (hd - gw);
            const Ogre::Vector3 BR = base + right * (hw + gd) + (-wallDir) * (hd - gw);
            const Ogre::Vector3 TFL = FL + (up * gh);
            const Ogre::Vector3 TFR = FR + (up * gh);
            const Ogre::Vector3 TBL = BL + (up * gh);
            const Ogre::Vector3 TBR = BR + (up * gh);

            // 1. Garage DOOR (visible from street = -wallDir side)
            // Cross(FR-FL, TFR-FL) = Cross(right*gd, right*gd+Y*gh) = gd*gh*(right×Y) = +wallDir = -(-wallDir) ✓
            this->pushQuad(garageV, garageI, garageN, FL, FR, TFR, TFL, -wallDir, gd / wallTile, gh / wallTile);

            // 2. Left wall (visible from -right side = house side)
            // Cross(FL-BL, TFL-BL) = Cross(-wallDir*gw, -wallDir*gw+Y*gh) = gw*gh*(−wallDir×Y)=gw*gh*right = -(-right) ✓
            this->pushQuad(wallV, wallI, wallN, BL, FL, TFL, TBL, -right, gw / wallTile, gh / wallTile);

            // 3. Right wall (visible from +right side = outer side)
            // Cross(BR-FR, TBR-FR) = Cross(+wallDir*gw, wallDir*gw+Y*gh) = gw*gh*(wallDir×Y) = -right = -(+right) ✓
            this->pushQuad(wallV, wallI, wallN, FR, BR, TBR, TFR, right, gw / wallTile, gh / wallTile);

            // 4. Back wall (visible from +wallDir side = away from street)
            // Cross(BL-BR, TBL-BR) = Cross(-right*gd, -right*gd+Y*gh) = gd*gh*(-right×Y) = -wallDir = -(+wallDir) ✓
            this->pushQuad(wallV, wallI, wallN, BR, BL, TBL, TBR, wallDir, gd / wallTile, gh / wallTile);

            // 5. Roof (visible from +Y side)
            // Cross(TFR-TFL, TBR-TFL) = Cross(right*gd, right*gd+wallDir*gw) = gd*gw*(right×wallDir) = -Y ✓
            this->pushQuad(wallV, wallI, wallN, TFL, TFR, TBR, TBL, up, gd / wallTile, gw / wallTile);
        }
    }

    // =========================================================================
    // buildCityRoadNetwork — drives ProceduralRoadComponent for city roads
    // =========================================================================

    void ProceduralCityComponent::buildCityRoadNetwork(const std::vector<CityBlock>& blocks)
    {
        // Roads live on CityRoads_N GO (created in postInit) — NOT on city GO.
        // Always route through findRoadComponent() so we use the correct PRC.
        ProceduralRoadComponent* roadComp = this->findRoadComponent();
        if (nullptr == roadComp)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                "[ProceduralCityComponent] buildCityRoadNetwork: CityRoads GO (id=" + Ogre::StringConverter::toString(this->roadComponentId) + ") not found or has no ProceduralRoadComponent.");
            return;
        }

        roadComp->setRoadWidth(this->roadWidthAttr->getReal());
        roadComp->setCenterDatablock(this->roadDatablockAttr->getString());
        roadComp->setEdgeDatablock(this->curbDatablockAttr->getString());

        // heightOffset is intentionally NOT reset to 0 here.
        // ProceduralRoadComponent::getGroundHeight() excludes its own roadItem,
        // so each addRoadSegmentLua correctly samples terrain + heightOffset (0.1 by default)
        // without hitting the previously-built road mesh.  Resetting to 0 would move the
        // road flush to the terrain surface, causing z-fighting against the Terra mesh.

        // FIX 2: Always clear before re-generating.
        // clearAllSegments() early-outs if roadSegments is empty, so it is safe to call
        // unconditionally.  It also resets hasRoadOrigin so the next first segment
        // correctly establishes a new road origin.
        roadComp->clearAllSegments();
        this->organicRoadSegs.clear(); // reset road exclusion zones on each generation
        // Clear stored buildings on full regeneration so Segment mode starts fresh
        this->storedBuildings.clear();
        this->selectedBuildingIdx = -1;

        // Use the PERTURBED grid computed in generateCityLayout (stored in cityGridX/cityGridZ).
        // If not yet computed (e.g., first call before layout), fall back to base grid.
        if (this->cityGridX.size() < 2 || this->cityGridZ.size() < 2)
        {
            const Ogre::Vector4 cityBnds2 = this->cityBoundsAttr->getVector4();
            const Ogre::Real blockSz2 = this->blockSizeAttr->getReal();
            this->cityGridX.clear();
            this->cityGridZ.clear();
            for (Ogre::Real gx = cityBnds2.x; gx <= cityBnds2.z + blockSz2 * 0.01f; gx += blockSz2)
            {
                this->cityGridX.push_back(gx);
            }
            for (Ogre::Real gz = cityBnds2.y; gz <= cityBnds2.w + blockSz2 * 0.01f; gz += blockSz2)
            {
                this->cityGridZ.push_back(gz);
            }
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

        // ---- Mode selection: organic (variance > 0.1) vs. perturbed grid ------
        // Organic: CityRoadGraph grows roads via tensor-field agent steps.
        // Grid:    perturbed grid with optional diagonal shortcuts.
        //
        // NOTE: variance must be in computeChecksum() so the binary city cache is
        // invalidated when the user changes this value.  Without that, Generate Now
        // after changing variance hits the cache and NEVER reaches this code.
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
            "[ProceduralCityComponent] buildCityRoadNetwork: variance=" + Ogre::StringConverter::toString(variance) + (variance > 0.1f ? "  → ORGANIC road graph mode" : "  → GRID road mode"));

        // Collect ALL road segments (grid + diagonals) into one list,
        // then run CityFaceExtractor::buildPlanarGraph to split all segments
        // at their mutual crossing points.  Every crossing becomes a shared
        // endpoint → PRC creates proper junction patches everywhere.
        std::vector<std::pair<Ogre::Vector2, Ogre::Vector2>> allRoadSegs;

        // Skip the outermost boundary grid lines.
        const size_t zFirst = (gridZ.size() > 2) ? 1u : 0u;
        const size_t zLast = (gridZ.size() > 2) ? gridZ.size() - 1u : gridZ.size();
        for (size_t zi = zFirst; zi < zLast; ++zi)
        {
            const Ogre::Real zVal = gridZ[zi];
            for (size_t xi = 0; xi + 1 < gridX.size(); ++xi)
            {
                allRoadSegs.push_back({Ogre::Vector2(gridX[xi], zVal), Ogre::Vector2(gridX[xi + 1], zVal)});
            }
        }

        const size_t xFirst = (gridX.size() > 2) ? 1u : 0u;
        const size_t xLast = (gridX.size() > 2) ? gridX.size() - 1u : gridX.size();
        for (size_t xi = xFirst; xi < xLast; ++xi)
        {
            const Ogre::Real xVal = gridX[xi];
            for (size_t zi = 0; zi + 1 < gridZ.size(); ++zi)
            {
                allRoadSegs.push_back({Ogre::Vector2(xVal, gridZ[zi]), Ogre::Vector2(xVal, gridZ[zi + 1])});
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
            std::vector<std::pair<size_t, size_t>> jpts;
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
                    const int dx = static_cast<int>(jpts[ib].first) - static_cast<int>(jpts[ia].first);
                    const int dz = static_cast<int>(jpts[ib].second) - static_cast<int>(jpts[ia].second);
                    // Must be diagonal (both non-zero) and span at least 2 grid cells in each direction
                    if (dx != 0 && dz != 0 && std::abs(dx) >= 2 && std::abs(dz) >= 2)
                    {
                        break;
                    }
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
                    allRoadSegs.push_back({Ogre::Vector2(sx, sz), Ogre::Vector2(ex, ez)});
                }
            }
        }

        // Add diagonal arteries to the segment list
        if (variance > 0.1f)
        {
            this->buildOrganicRoadNetwork(allRoadSegs, variance);
        }

        // Preprocess: find all mutual crossings, insert split nodes.
        // Result: a proper planar graph where every crossing is a shared endpoint.
        {
            CityFaceExtractor planarizer;
            planarizer.buildPlanarGraph(allRoadSegs);
            const auto& pNodes = planarizer.getNodes();
            const auto& pHalfEdges = planarizer.getHalfEdges();

            this->organicRoadSegs.clear();
            this->organicRoadSegs.reserve(pHalfEdges.size() / 2u);

            // When GradientAlignment is on (planet mode), look up terrain Y per node
            // so PRC gets correct starting heights for AdaptToGround on a curved surface.
            // Cache per node to avoid redundant raycasts (edges share nodes).
            const bool usePlanetY = this->gradientAlignmentAttr->getBool();
            std::vector<Ogre::Real> nodeY(pNodes.size(), 0.f);
            if (usePlanetY)
            {
                for (size_t ni = 0; ni < pNodes.size(); ++ni)
                {
                    nodeY[ni] = this->getGroundHeight(
                        Ogre::Vector3(pNodes[ni].pos.x, 0.f, pNodes[ni].pos.y));
                }
            }

            roadComp->beginBatch();
            for (size_t hi = 0; hi < pHalfEdges.size(); hi += 2u)
            {
                const auto& he = pHalfEdges[hi];
                const Ogre::Vector2& a = pNodes[static_cast<size_t>(he.from)].pos;
                const Ogre::Vector2& b = pNodes[static_cast<size_t>(he.to)].pos;
                if ((b - a).squaredLength() < 0.01f)
                {
                    continue;
                }
                RoadSegXZ seg;
                seg.a = a;
                seg.b = b;
                this->organicRoadSegs.push_back(seg);
                const Ogre::Real ya = usePlanetY ? nodeY[static_cast<size_t>(he.from)] : 0.f;
                const Ogre::Real yb = usePlanetY ? nodeY[static_cast<size_t>(he.to)]   : 0.f;
                roadComp->addRoadSegmentLua(Ogre::Vector3(a.x, ya, a.y), Ogre::Vector3(b.x, yb, b.y));
            }
            roadComp->endBatch();

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralCityComponent] Planar graph: " + Ogre::StringConverter::toString(static_cast<unsigned int>(pNodes.size())) + " nodes, " +
                                                                                    Ogre::StringConverter::toString(static_cast<unsigned int>(pHalfEdges.size() / 2)) + " edges from " +
                                                                                    Ogre::StringConverter::toString(static_cast<unsigned int>(allRoadSegs.size())) + " raw segments.");
        }

        // Restore terrain interval
        if (nullptr != terrainIntervalAttr)
        {
            terrainIntervalAttr->setValue(savedTerrainInterval);
        }

        const unsigned int numSegs = static_cast<unsigned int>(gridZ.size() * (gridX.size() > 0 ? gridX.size() - 1 : 0) + gridX.size() * (gridZ.size() > 0 ? gridZ.size() - 1 : 0));
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralCityComponent] Road grid: " + Ogre::StringConverter::toString(numSegs) +
                                                                                " segments. heightOffset unchanged (PRC excludes own mesh in getGroundHeight, "
                                                                                "so terrain+heightOffset is sampled correctly without z-fighting).");
    }

    // =========================================================================
    // buildOrganicRoadNetwork
    // Adds diagonal arteries through the city.  Each artery is a straight line
    // between two boundary points, SPLIT at every grid-line crossing so PRC
    // gets proper shared endpoints at crossings → clean junction patches.
    // =========================================================================
    void ProceduralCityComponent::buildOrganicRoadNetwork(std::vector<std::pair<Ogre::Vector2, Ogre::Vector2>>& allRoadSegs, Ogre::Real variance)
    {
        const std::vector<Ogre::Real>& gX = this->cityGridX;
        const std::vector<Ogre::Real>& gZ = this->cityGridZ;
        if (gX.size() < 3u || gZ.size() < 3u)
        {
            return;
        }

        const unsigned int mSeed = this->masterSeedAttr->getUInt();
        std::mt19937 rng(mSeed ^ 0xB4D1A7C2u);

        const int nX = static_cast<int>(gX.size());
        const int nZ = static_cast<int>(gZ.size());
        // Cap at 3 arteries — more causes junction clusters that are hard to avoid.
        // Designer can remove specific roads manually anyway.
        const int numArts = 1 + static_cast<int>(variance * 2.0f); // 1–3

        std::uniform_int_distribution<int> dSide(0, 3);
        std::uniform_int_distribution<int> dX(1, nX - 2);
        std::uniform_int_distribution<int> dZ(1, nZ - 2);

        // Minimum separation between artery endpoints prevents arteries starting
        // near each other and converging into a junction cluster in the city interior.
        const Ogre::Real minSep = std::min(gX.back() - gX.front(), gZ.back() - gZ.front()) * 0.28f;
        std::vector<Ogre::Vector2> usedPts;

        // Diagonal arteries: straight lines between boundary points.
        // buildPlanarGraph will split them at every grid crossing later.
        int placed = 0;
        for (int attempt = 0; attempt < numArts * 6 && placed < numArts; ++attempt)
        {
            int sideA = dSide(rng);
            int sideB = (sideA + 2) % 4; // always pick the OPPOSITE side → long cross-city arteries

            auto boundaryPt = [&](int side) -> Ogre::Vector2
            {
                if (side == 0)
                {
                    return Ogre::Vector2(gX[static_cast<size_t>(dX(rng))], gZ.front());
                }
                if (side == 1)
                {
                    return Ogre::Vector2(gX[static_cast<size_t>(dX(rng))], gZ.back());
                }
                if (side == 2)
                {
                    return Ogre::Vector2(gX.front(), gZ[static_cast<size_t>(dZ(rng))]);
                }
                return Ogre::Vector2(gX.back(), gZ[static_cast<size_t>(dZ(rng))]);
            };

            const Ogre::Vector2 ptA = boundaryPt(sideA);
            const Ogre::Vector2 ptB = boundaryPt(sideB);
            if ((ptB - ptA).squaredLength() < 1.f)
            {
                continue;
            }

            // Reject if too close to an already-placed artery endpoint
            bool tooClose = false;
            for (const auto& p : usedPts)
            {
                if ((ptA - p).length() < minSep || (ptB - p).length() < minSep)
                {
                    tooClose = true;
                    break;
                }
            }
            if (tooClose)
            {
                continue;
            }

            usedPts.push_back(ptA);
            usedPts.push_back(ptB);
            allRoadSegs.push_back({ptA, ptB});
            ++placed;
        }
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

            // ----------------------------------------------------------------
            // Helper: expand one batch's 8-float vertices to 12-float GPU
            // layout (pos3 + normal3 + tangent4 + uv2), build VBO+IBO+VAO,
            // and attach as a SubMesh on the given mesh.
            // Empty batches get a 1-vertex dummy VAO so the SubMesh count
            // stays constant and submesh indices always match material slots.
            // ----------------------------------------------------------------
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
                    // Dummy VAO -- keeps SubMesh index stable.
                    float dummyV[12] = {};
                    Ogre::VertexBufferPackedVec dv;
                    dv.push_back(vm->createVertexBuffer(elems, 1u, Ogre::BT_IMMUTABLE, dummyV, false));
                    Ogre::VertexArrayObject* dVao = vm->createVertexArrayObject(dv, nullptr, Ogre::OT_TRIANGLE_LIST);
                    sub->mVao[Ogre::VpNormal].push_back(dVao);
                    sub->mVao[Ogre::VpShadow].push_back(dVao);
                    return;
                }

                const size_t numV = batch.numVertices;
                const size_t srcStride = 8u;
                const size_t dstStride = 12u;

                float* dst = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(numV * dstStride * sizeof(float), Ogre::MEMCATEGORY_GEOMETRY));

                for (size_t vi = 0; vi < numV; ++vi)
                {
                    const size_t s = vi * srcStride;
                    const size_t d = vi * dstStride;

                    // Position
                    dst[d + 0] = batch.rawVertices[s + 0];
                    dst[d + 1] = batch.rawVertices[s + 1];
                    dst[d + 2] = batch.rawVertices[s + 2];
                    minB.makeFloor(Ogre::Vector3(dst[d + 0], dst[d + 1], dst[d + 2]));
                    maxB.makeCeil(Ogre::Vector3(dst[d + 0], dst[d + 1], dst[d + 2]));

                    // Normal
                    dst[d + 3] = batch.rawVertices[s + 3];
                    dst[d + 4] = batch.rawVertices[s + 4];
                    dst[d + 5] = batch.rawVertices[s + 5];

                    // Tangent (computed from normal)
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
                    dst[d + 6] = t.x;
                    dst[d + 7] = t.y;
                    dst[d + 8] = t.z;
                    dst[d + 9] = 1.f;

                    // UV
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

            // ----------------------------------------------------------------
            // Build ONE unified mesh containing ALL geometry:
            //   SubMeshes 0..(NUM_BUILDING_VARIANTS*SLOTS_PER_VARIANT - 1)
            //       -> building geometry (4 variants x SLOTS_PER_VARIANT slots)
            //   SubMeshes N..N+1
            //       -> infrastructure (road slot, curb slot)
            //
            // Having a single Ogre::Item means PhysicsArtifactComponent
            // iterates ALL submeshes when building the TreeCollision hull,
            // so every building and road surface gets collision.
            //
            // Previously: 4 separate building Items + 1 infra Item, only
            // the first Item was passed to gameObjectPtr->init(), so only
            // 1/5th of the geometry had collision.
            // ----------------------------------------------------------------
            const Ogre::String id = Ogre::StringConverter::toString(this->gameObjectPtr->getId());
            const Ogre::String unifiedMeshName = "CityUnified_" + id;

            // Remove any stale mesh from a previous generation.
            {
                Ogre::MeshManager& mm = Ogre::MeshManager::getSingleton();
                Ogre::MeshPtr ex = mm.getByName(unifiedMeshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
                if (false == ex.isNull())
                {
                    mm.remove(ex->getHandle());
                }
            }

            Ogre::MeshPtr unifiedMesh = Ogre::MeshManager::getSingleton().createManual(unifiedMeshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, &NOWA::gDummyMeshLoader);

            Ogre::Vector3 uMinB(std::numeric_limits<float>::max());
            Ogre::Vector3 uMaxB(-std::numeric_limits<float>::max());

            // --- Building slots (variants x slots each) ---
            const unsigned int buildingSlotCount = NUM_BUILDING_VARIANTS * SLOTS_PER_VARIANT;
            for (unsigned int s = 0u; s < buildingSlotCount; ++s)
            {
                buildSubMesh(unifiedMesh, batches[s], uMinB, uMaxB);
            }

            // --- Infrastructure slots (road + curb) ---
            for (unsigned int s = 0u; s < 2u; ++s)
            {
                buildSubMesh(unifiedMesh, batches[INFRA_BATCH_OFFSET + s], uMinB, uMaxB);
            }

            if (uMinB.x > uMaxB.x)
            {
                uMinB = Ogre::Vector3(-1.f);
                uMaxB = Ogre::Vector3(1.f);
            }
            Ogre::Aabb uAabb;
            uAabb.setExtents(uMinB, uMaxB);
            unifiedMesh->_setBounds(uAabb, false);
            unifiedMesh->_setBoundingSphereRadius(uAabb.getRadius());

            Ogre::Item* unifiedItem = sm->createItem(unifiedMesh, Ogre::SCENE_STATIC);

            // ----------------------------------------------------------------
            // Apply datablocks -- submesh index follows the same slot order
            // as the buildSubMesh loop above.
            // ----------------------------------------------------------------
            const unsigned int numDistricts = static_cast<unsigned int>(this->districts.size());
            const unsigned int d0 = (numDistricts > 0u) ? 0u : static_cast<unsigned int>(-1);

            unsigned int subIdx = 0u;

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                "[ProceduralCityComponent] === Datablock assignment (subMesh -> slot -> datablock) ===");

            auto applyDb = [&](const Ogre::String& dbName, const char* slotLabel)
            {
                // Log every assignment so wrong datablocks can be traced
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                    "[ProceduralCityComponent]  subMesh " +
                    Ogre::StringConverter::toString(subIdx) +
                    "  slot=" + Ogre::String(slotLabel) +
                    "  datablock='" + dbName + "'");

                if (false == dbName.empty())
                {
                    Ogre::HlmsDatablock* db = hlms->getDatablockNoDefault(dbName);
                    if (nullptr != db)
                    {
                        unifiedItem->getSubItem(subIdx)->setDatablock(db);
                    }
                    else
                    {
                        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                            "[ProceduralCityComponent]  *** DATABLOCK NOT FOUND: '" + dbName + "' (subMesh " + Ogre::StringConverter::toString(subIdx) + " slot=" + Ogre::String(slotLabel) + ") ***");
                    }
                }
                ++subIdx;
            };

            for (unsigned int vi = 0u; vi < NUM_BUILDING_VARIANTS; ++vi)
            {
                const Ogre::String doorDb   = (vi % 2u == 0u) ? this->doorDatablockAttr->getString() : Ogre::String("WoodenDoor");
                const Ogre::String garageDb = this->garageDatablockAttr->getString();

                // Submesh order per variant: face(0), roof(1), window(2), trim(3), door(4), garage(5)
                // Slot 0 contains the building outer face geometry (formerly "wall").
                // wallDatablocks was removed from CityDistrict — use roofDatablocks[0] as
                // a solid PBR material for the outer faces until a dedicated facade attribute is added.
                if (d0 < numDistricts)
                {
                    // slot 0 — main building faces (from trimH to roofline). Randomly varied per variant.
                    // Uses roofDatablocks as facade material pool (solid PBR stone/brick textures).
                    // TODO: add dedicated DistrictFacadeDatablock attributes for full control.
                    applyDb(this->districts[d0].faceDatablocks[vi % 6],   "face");   // slot 0 — main faces
                    applyDb(this->districts[d0].roofDatablocks[vi % 3],   "roof");   // slot 1 — roof
                    applyDb(this->districts[d0].windowDatablocks[vi % 3], "window"); // slot 2 — windows
                    // slot 3 — trim: the thin ground-level plinth band (max 1.5m tall).
                    // Uses trimDatablocks which contain stone/cobblestone textures as accent.
                    applyDb(this->districts[d0].trimDatablocks[vi % 2],   "trim");   // slot 3 — base plinth
                }
                else
                {
                    subIdx += 4u;  // skip face + roof + window + trim
                }
                applyDb(doorDb,   "door");   // slot 4
                applyDb(garageDb, "garage"); // slot 5
            }

            // Infrastructure submeshes (road, curb)
            applyDb(this->roadDatablockAttr->getString(), "road");
            applyDb(this->curbDatablockAttr->getString(), "curb");

            // ----------------------------------------------------------------
            // Attach to scene
            // ----------------------------------------------------------------
            Ogre::SceneNode* cityNode = sm->getRootSceneNode(Ogre::SCENE_STATIC)->createChildSceneNode(Ogre::SCENE_STATIC);
            cityNode->setPosition(this->cityOrigin);
            cityNode->setOrientation(Ogre::Quaternion::IDENTITY);
            cityNode->setScale(Ogre::Vector3::UNIT_SCALE);
            cityNode->attachObject(unifiedItem);
            sm->notifyStaticAabbDirty(unifiedItem);
            sm->notifyStaticDirty(sm->getRootSceneNode(Ogre::SCENE_STATIC));

            if (nullptr != vm)
            {
                vm->_update();
            }

            // Store for lifecycle management (clear, regenerate, destroy).
            // Use slot 0 of the first batch as the canonical storage location.
            // All other batch slots are left with their geometry data intact
            // for any future per-batch queries but hold no Items/Nodes.
            this->cityBatches = std::move(batches);
            this->cityBatches[0].items.push_back(unifiedItem);
            this->cityBatches[0].nodes.push_back(cityNode);

            // ----------------------------------------------------------------
            // Register the unified Item so PhysicsArtifactComponent builds
            // its TreeCollision from the complete mesh -- ALL submeshes
            // (every building variant + road + curb) are included.
            //
            // Previously only the first building variant Item was registered,
            // so collision covered only ~1/5 of the actual geometry.
            // ----------------------------------------------------------------
            this->gameObjectPtr->init(unifiedItem);
            this->gameObjectPtr->setDoNotDestroyMovableObject(true);

            if (nullptr != this->physicsArtifactComponent)
            {
                this->physicsArtifactComponent->reCreateCollision();
            }

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralCityComponent] Created unified city mesh '" + unifiedMeshName + "' with " + Ogre::StringConverter::toString(unifiedMesh->getNumSubMeshes()) +
                                                                                   " submeshes. Bounds: min=" + Ogre::StringConverter::toString(uMinB) + " max=" + Ogre::StringConverter::toString(uMaxB));
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
                        if (mn.find("CityBuildings_") != Ogre::String::npos || mn.find("CityInfra_") != Ogre::String::npos || mn.find("CityCollision_") != Ogre::String::npos)
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
        // Important: if  this->gameObjectPtr->setDoNotDestroyMovableObject(true); is called, then movable object must be nulled!
        this->gameObjectPtr->nullMovableObject();
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
        // Gradient alignment changes the actual road/curb/intersection geometry (surface
        // tilt on a planet), so it must be in the checksum — otherwise toggling it hits the
        // cache and the road mesh is never rebuilt with the new tilt.
        cityHashCombine(cs, static_cast<uint64_t>(this->gradientAlignmentAttr->getBool() ? 1u : 0u));
        // CRITICAL: road variance must be in checksum so the cache is invalidated when the
        // designer changes it.  Without this, changing variance from 0→0.5 hits the cache
        // and never reaches buildCityRoadNetwork/buildOrganicRoadNetwork.
        cityHashCombine(cs, cityHashReal(this->roadVarianceAttr->getReal()));
        cityHashCombine(cs, static_cast<uint64_t>(this->generateGarageAttr->getBool() ? 1u : 0u));
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
            for (int v = 0; v < 6; ++v)
            {
                cityHashCombine(cs, cityHashString(d.faceDatablocks[v]));
            }
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
                std::vector<float> wV, rV, winV, tV, dV;
                std::vector<Ogre::uint32> wI, rI, winI, tI, dI;
                size_t wN = 0, rN = 0, winN = 0, tN = 0, dN = 0;
                std::vector<float> gV;
                std::vector<Ogre::uint32> gI;
                size_t gN = 0;
                this->generateBuildingGeometry(inst, d, this->cityOrigin, wV, wI, wN, rV, rI, rN, winV, winI, winN, tV, tI, tN, dV, dI, dN, gV, gI, gN);

                // Route to the correct variant batch group
                const unsigned int vi = inst.variantSeed % NUM_BUILDING_VARIANTS;
                const unsigned int batchBase = vi * SLOTS_PER_VARIANT;

                auto ap = [](std::vector<float>& dstV, std::vector<Ogre::uint32>& dstI, size_t& dstN, const std::vector<float>& srcV, const std::vector<Ogre::uint32>& srcI, size_t srcN)
                {
                    const Ogre::uint32 base = static_cast<Ogre::uint32>(dstN);
                    dstV.insert(dstV.end(), srcV.begin(), srcV.end());
                    for (auto idx : srcI)
                    {
                        dstI.push_back(idx + base);
                    }
                    dstN += srcN;
                };

                // Only accumulate if this batch is the wall-slot of its variant group
                // (the other 4 slots were not stored in the cache as separate batch entries
                // with instances — they need to be re-accumulated here from the wall-slot instances)
                if (batch.materialSlot == batchBase + 0)
                {
                    ap(batch.rawVertices, batch.rawIndices, batch.numVertices, wV, wI, wN);
                }
                // Find the matching sibling batches in the loaded list and fill them too
                for (auto& sibling : loaded)
                {
                    if (sibling.materialSlot == batchBase + 1)
                    {
                        ap(sibling.rawVertices, sibling.rawIndices, sibling.numVertices, rV, rI, rN);
                    }
                    else if (sibling.materialSlot == batchBase + 2)
                    {
                        ap(sibling.rawVertices, sibling.rawIndices, sibling.numVertices, winV, winI, winN);
                    }
                    else if (sibling.materialSlot == batchBase + 3)
                    {
                        ap(sibling.rawVertices, sibling.rawIndices, sibling.numVertices, tV, tI, tN);
                    }
                    else if (sibling.materialSlot == batchBase + 4)
                    {
                        ap(sibling.rawVertices, sibling.rawIndices, sibling.numVertices, dV, dI, dN);
                    }
                    else if (sibling.materialSlot == batchBase + 5)
                    {
                        ap(sibling.rawVertices, sibling.rawIndices, sibling.numVertices, gV, gI, gN);
                    }
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
        const Ogre::Real rayStartY = position.y + 1000.f; // hit must be below this

        std::vector<Ogre::MovableObject*> exclude;
        for (const auto& b : this->cityBatches)
        {
            for (auto* item : b.items)
            {
                if (nullptr != item)
                {
                    exclude.push_back(item);
                }
            }
        }

        Ogre::Vector3 hp;
        Ogre::MovableObject* ho = nullptr;
        Ogre::Real hd = 0.f;
        Ogre::Vector3 hn;
        bool hit = MathHelper::getInstance()->getRaycastFromPoint(this->groundQuery, AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera(), hp, (size_t&)ho, hd, hn, &exclude);

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
                    isEditorHelper = mName.find("Light") != Ogre::String::npos || mName.find("light") != Ogre::String::npos || mName.find("Camera") != Ogre::String::npos || mName.find("camera") != Ogre::String::npos ||
                                     mName.find("node.") != Ogre::String::npos || mName.find("Node.") != Ogre::String::npos || mName.find("arrow") != Ogre::String::npos || mName.find("Arrow") != Ogre::String::npos ||
                                     mName.find("gizmo") != Ogre::String::npos || mName.find("Gizmo") != Ogre::String::npos;
                }
            }

            if (false == isEditorHelper)
            {
                // Accept the hit if it is BELOW the ray start and within 2000m.
                // Old check: hp.y < cityOrigin.y - 100 || hp.y > cityOrigin.y + 300
                //   → fails for planet cities where cityOrigin.y=0 but surface is at Y=479.
                // New check: purely distance-based from the ray origin so it works for
                // flat worlds (Y≈0), elevated terrain, AND spherical planets at any radius.
                const Ogre::Real hitDist = rayStartY - hp.y; // positive = below ray start
                if (hitDist < 0.f || hitDist > 2000.f)
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
                        "[ProceduralCityComponent] getGroundHeight: hit Y=" + Ogre::StringConverter::toString(hp.y) + " dist=" + Ogre::StringConverter::toString(hitDist) + " rejected (above ray start or >2000m away).");
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

    void ProceduralCityComponent::handleComponentManuallyDeleted(NOWA::EventDataPtr eventData)
    {
        auto data = boost::static_pointer_cast<EventDataDeleteComponent>(eventData);
        if (data->getGameObjectId() == this->gameObjectPtr->getId())
        {
            this->deleteCityDataFile();
        }
    }

    // =========================================================================
    // Edit-mode event handlers (mirrors ProceduralRoadComponent pattern)
    // =========================================================================

    void ProceduralCityComponent::handleGameObjectSelected(NOWA::EventDataPtr eventData)
    {
        auto data = boost::static_pointer_cast<EventDataGameObjectSelected>(eventData);
        if (data->getGameObjectId() == this->gameObjectPtr->getId())
        {
            this->isSelected = data->getIsSelected();
        }
        else if (data->getIsSelected())
        {
            this->isSelected = false;
        }
        this->updateModificationState();
    }

    void ProceduralCityComponent::updateModificationState(void)
    {
        // Segment mode is active when: EditMode=Segment AND this object is selected.
        // We do NOT require EditorMeshModifyMode — that event fires every frame and
        // caused severe FPS drops + camera input corruption when used as a trigger.
        const bool segMode = (this->editModeAttr->getListSelectedValue() == "Segment");
        const bool shouldListen = segMode && this->isSelected && this->activated->getBool();

        // Only call addInputListener / removeInputListener when the state CHANGES.
        // Calling them every frame causes O(n) map operations in InputDeviceCore.
        if (shouldListen && false == this->inputListenerRegistered)
        {
            this->addInputListener();
            this->inputListenerRegistered = true;
        }
        else if (false == shouldListen && this->inputListenerRegistered)
        {
            this->removeInputListener();
            this->inputListenerRegistered = false;
            this->selectedBuildingIdx = -1;
            this->updateSelectionOverlay();
        }
    }

    void ProceduralCityComponent::addInputListener(void)
    {
        const Ogre::String name = ProceduralCityComponent::getStaticClassName() + "_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
        if (auto* core = InputDeviceCore::getSingletonPtr())
        {
            core->addKeyListener(this, name);
            core->addMouseListener(this, name);
        }
    }

    void ProceduralCityComponent::removeInputListener(void)
    {
        const Ogre::String name = ProceduralCityComponent::getStaticClassName() + "_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
        if (auto* core = InputDeviceCore::getSingletonPtr())
        {
            core->removeKeyListener(name);
            core->removeMouseListener(name);
        }
    }

    // =========================================================================
    // OIS::MouseListener
    // =========================================================================
    bool ProceduralCityComponent::mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
    {
        if (false == this->activated->getBool())
        {
            return true;
        }
        if (id != OIS::MB_Left)
        {
            return true;
        }
        if (nullptr != NOWA::GraphicsModule::getInstance()->getMyGUIFocusWidget())
        {
            return true;
        }
        if (this->editModeAttr->getListSelectedValue() != "Segment")
        {
            return true;
        }

        const int idx = this->findBuildingAtScreenPos(evt.state.X.abs, evt.state.Y.abs);
        this->selectedBuildingIdx = idx;
        this->updateSelectionOverlay();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
            "[ProceduralCityComponent] Segment click: building " + (idx >= 0 ? Ogre::StringConverter::toString(idx) + " selected — press X to delete" : "not found (clicked empty space)"));
        return false;
    }

    bool ProceduralCityComponent::mouseReleased(const OIS::MouseEvent& /*evt*/, OIS::MouseButtonID /*id*/)
    {
        return true;
    }

    bool ProceduralCityComponent::mouseMoved(const OIS::MouseEvent& /*evt*/)
    {
        return true;
    }

    // =========================================================================
    // OIS::KeyListener
    // =========================================================================
    bool ProceduralCityComponent::keyPressed(const OIS::KeyEvent& evt)
    {
        if (false == this->activated->getBool())
        {
            return true;
        }
        if (this->editModeAttr->getListSelectedValue() != "Segment")
        {
            return true;
        }

        if (evt.key == OIS::KC_X && this->selectedBuildingIdx >= 0)
        {
            this->deleteSelectedBuilding();
            return false;
        }

        return true;
    }

    bool ProceduralCityComponent::keyReleased(const OIS::KeyEvent& /*evt*/)
    {
        return true;
    }

    // =========================================================================
    // findBuildingAtScreenPos
    // Cast a ray and return the index of the nearest building whose XZ footprint
    // it passes through.  We do a simple cylinder (circle in XZ) test per building
    // which is fast and accurate enough for interactive selection.
    // =========================================================================
    int ProceduralCityComponent::findBuildingAtScreenPos(int screenXabs, int screenYabs)
    {
        Ogre::Camera* camera = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
        if (nullptr == camera)
        {
            return -1;
        }

        Ogre::Real nx = 0.f, ny = 0.f;
        MathHelper::getInstance()->mouseToViewPort(screenXabs, screenYabs, nx, ny, Core::getSingletonPtr()->getOgreRenderWindow());

        const Ogre::Ray ray = camera->getCameraToViewportRay(nx, ny);
        const Ogre::Vector3& ro = ray.getOrigin();
        const Ogre::Vector3& rd = ray.getDirection();

        int bestIdx = -1;
        float bestDist = std::numeric_limits<float>::max();

        for (size_t i = 0; i < this->storedBuildings.size(); ++i)
        {
            const BuildingInstance& b = this->storedBuildings[i];

            // World centre of building (mid-height)
            const Ogre::Vector3 centre(b.position.x, b.groundHeight + b.footprint.y * 0.5f, b.position.z);

            // Selection radius = max(footprint half-width, half-depth) + small margin
            const float radius = std::max(b.footprint.x, b.footprint.z) * 0.6f + 1.0f;

            // Ray-sphere test (approximate — spheres are simpler than OBBs for mouse pick)
            const Ogre::Vector3 oc = ro - centre;
            const float a = rd.dotProduct(rd);
            const float b2 = 2.f * oc.dotProduct(rd);
            const float c = oc.dotProduct(oc) - radius * radius;
            const float discriminant = b2 * b2 - 4.f * a * c;
            if (discriminant < 0.f)
            {
                continue;
            }

            const float dist = (-b2 - std::sqrt(discriminant)) / (2.f * a);
            if (dist < 0.01f || dist > bestDist)
            {
                continue;
            }
            bestDist = dist;
            bestIdx = static_cast<int>(i);
        }
        return bestIdx;
    }

    // =========================================================================
    // deleteSelectedBuilding — push undo snapshot, erase, rebuild batches
    // =========================================================================
    void ProceduralCityComponent::deleteSelectedBuilding(void)
    {
        if (this->selectedBuildingIdx < 0 || static_cast<size_t>(this->selectedBuildingIdx) >= this->storedBuildings.size())
        {
            return;
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralCityComponent] Deleted building " + Ogre::StringConverter::toString(this->selectedBuildingIdx) +
                                                                                " (pos=" + Ogre::StringConverter::toString(this->storedBuildings[static_cast<size_t>(this->selectedBuildingIdx)].position) + "). Ctrl+Z to undo.");

        this->storedBuildings.erase(this->storedBuildings.begin() + this->selectedBuildingIdx);
        this->selectedBuildingIdx = -1;
        this->updateSelectionOverlay();
        this->rebuildBatchesFromStoredBuildings();
    }

    // =========================================================================
    // rebuildBatchesFromStoredBuildings
    // Regenerates all batch geometry from this->storedBuildings and re-uploads
    // the GPU meshes.  Called after deletion or undo.
    // =========================================================================
    //void ProceduralCityComponent::rebuildBatchesFromStoredBuildings(void)
    //{
    //    std::vector<CityBatch> batches(TOTAL_CITY_BATCHES);
    //    for (unsigned int s = 0; s < TOTAL_CITY_BATCHES; ++s)
    //    {
    //        batches[s].materialSlot = s;
    //    }

    //    const Ogre::Vector3& localOrigin = this->cityOrigin;

    //    for (const auto& building : this->storedBuildings)
    //    {
    //        if (building.archetypeIdx >= this->districts.size())
    //        {
    //            continue;
    //        }
    //        const CityDistrict& district = this->districts[building.archetypeIdx];

    //        const unsigned int vi = building.variantSeed % NUM_BUILDING_VARIANTS;
    //        const unsigned int base = vi * SLOTS_PER_VARIANT;
    //        this->generateBuildingGeometry(building, district, localOrigin, batches[base + 0].rawVertices, batches[base + 0].rawIndices, batches[base + 0].numVertices, batches[base + 1].rawVertices, batches[base + 1].rawIndices,
    //            batches[base + 1].numVertices, batches[base + 2].rawVertices, batches[base + 2].rawIndices, batches[base + 2].numVertices, batches[base + 3].rawVertices, batches[base + 3].rawIndices, batches[base + 3].numVertices,
    //            batches[base + 4].rawVertices, batches[base + 4].rawIndices, batches[base + 4].numVertices, batches[base + 5].rawVertices, batches[base + 5].rawIndices, batches[base + 5].numVertices);
    //    }

    //    // Destroy old batches, recreate with new geometry
    //    // DIAGNOSTIC: two sequential enqueueAndWait here block logic thread.
    //    // ~N*0.1ms per building. For 200 buildings expect 200-600ms freeze on X press.
    //    auto tD = std::chrono::high_resolution_clock::now();
    //    GraphicsModule::getInstance()->enqueueAndWait(
    //        [this]()
    //        {
    //            this->destroyCityOnRenderThread();
    //        },
    //        "ProceduralCityComponent::RebuildFromStoredBuildings_Destroy");
    //    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
    //        "[ProceduralCityComponent] rebuildBatches Destroy " + Ogre::StringConverter::toString(std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - tD).count()) + "ms");

    //    auto tC = std::chrono::high_resolution_clock::now();
    //    GraphicsModule::getInstance()->enqueueAndWait(
    //        [this, batches]() mutable
    //        {
    //            this->createCityOnRenderThread(std::move(batches));
    //        },
    //        "ProceduralCityComponent::RebuildFromStoredBuildings_Create");
    //    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
    //        "[ProceduralCityComponent] rebuildBatches Create " + Ogre::StringConverter::toString(std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - tC).count()) + "ms");
    //}

    void ProceduralCityComponent::rebuildBatchesFromStoredBuildings(void)
    {
        std::vector<CityBatch> batches(TOTAL_CITY_BATCHES);
        for (unsigned int s = 0; s < TOTAL_CITY_BATCHES; ++s)
        {
            batches[s].materialSlot = s;
        }

        const Ogre::Vector3& localOrigin = this->cityOrigin;

        for (const auto& building : this->storedBuildings)
        {
            if (building.archetypeIdx >= this->districts.size())
            {
                continue;
            }
            const CityDistrict& district = this->districts[building.archetypeIdx];
            const unsigned int vi = building.variantSeed % NUM_BUILDING_VARIANTS;
            const unsigned int base = vi * SLOTS_PER_VARIANT;
            this->generateBuildingGeometry(building, district, localOrigin, batches[base + 0].rawVertices, batches[base + 0].rawIndices, batches[base + 0].numVertices, batches[base + 1].rawVertices, batches[base + 1].rawIndices,
                batches[base + 1].numVertices, batches[base + 2].rawVertices, batches[base + 2].rawIndices, batches[base + 2].numVertices, batches[base + 3].rawVertices, batches[base + 3].rawIndices, batches[base + 3].numVertices,
                batches[base + 4].rawVertices, batches[base + 4].rawIndices, batches[base + 4].numVertices, batches[base + 5].rawVertices, batches[base + 5].rawIndices, batches[base + 5].numVertices);
        }

        // Merge both render-thread operations into a SINGLE enqueueAndWait.
        // Previously: two sequential enqueueAndWait calls -- the logic thread
        // blocked twice, doubling the stall time.
        // Now: one round-trip to the render thread does destroy + create atomically.
        //
        // Also: skip reCreateCollision during interactive editing. Newton
        // TreeCollision rebuild on a large merged mesh takes 1-5 seconds and
        // is only needed for gameplay, not for the editor Segment delete workflow.
        // Collision is rebuilt once when the user explicitly leaves Segment mode.
        GraphicsModule::getInstance()->enqueueAndWait(
            [this, batches = std::move(batches)]() mutable
            {
                this->destroyCityOnRenderThread();
                // Pass skipCollision=true -- caller will rebuild when appropriate.
                this->createCityOnRenderThread(std::move(batches));
            },
            "ProceduralCityComponent::RebuildFromStoredBuildings");
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
        this->districtFaceDbAttrs.resize(count),
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
        // 6 face variants map to the 4 roofDatablocks[] slots — each building gets
        // the wall for its variant group (variantSeed % 4).
        static const char* faceDef[6] = {
            "/dural/structures/moraf_stone_wall/texture",        
            "/dural/structures/rezpa_wall/texture",              
            "/dural/structures/ossyja_wall_cobblestone/texture", 
            "/dural/structures/moraf_timber_wall/texture",
            "/dural/structures/ossyja_inner_walls/texture",
            "/dural/structures/rezpa_inner_walls/texture"
        };

        static const char* roofDef[3] = {
            "city_roof_01",                                // from generated PNG
            "/dural/structures/moraf_roof_bottom/texture", // real asset
            "M_rooftiles_01"                               // real asset
        };

        static const char* winDef[3] = {"city_window_01", "city_window_02", "city_window_01"};
        static const char* trimDef[2] = {"/dural/structures/ossyja_inner_walls/texture", "/dural/structures/rezpa_inner_walls/texture"};

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


            std::array<int, 6> faceIndices = {0, 1, 2, 3, 4, 5};
            std::shuffle(faceIndices.begin(), faceIndices.end(), std::mt19937(std::random_device{}()));

            for (unsigned int v = 0; v < 6u; ++v)
            {
                if (nullptr == this->districtFaceDbAttrs[i][v])
                {
                    Ogre::String key = AttrDistrictFaceDatablock() + idx + "_" + Ogre::StringConverter::toString(v);

                    const char* datablock = faceDef[faceIndices[v]];

                    this->districtFaceDbAttrs[i][v] = new Variant(key, Ogre::String(datablock), this->attributes);
                    this->districtFaceDbAttrs[i][v]->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
                    this->districtFaceDbAttrs[i][v]->setDescription("Building facade material variant " + Ogre::StringConverter::toString(v) + ".");
                    this->districts[i].faceDatablocks[v] = datablock;
                }
            }

            for (unsigned int v = 0; v < 3u; ++v)
            {
                if (nullptr == this->districtRoofDbAttrs[i][v])
                {
                    Ogre::String key = AttrDistrictRoofDatablock() + idx + "_" + Ogre::StringConverter::toString(v);
                    this->districtRoofDbAttrs[i][v] = new Variant(key, Ogre::String(roofDef[v]), this->attributes);
                    this->districtRoofDbAttrs[i][v]->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
                    this->districtRoofDbAttrs[i][v]->setDescription("Roof material variant " + Ogre::StringConverter::toString(v) + ".");

                    this->districts[i].roofDatablocks[v] = roofDef[v];
                }
                if (nullptr == this->districtWindowDbAttrs[i][v])
                {
                    Ogre::String key = AttrDistrictWindowDatablock() + idx + "_" + Ogre::StringConverter::toString(v);
                    this->districtWindowDbAttrs[i][v] = new Variant(key, Ogre::String(winDef[v]), this->attributes);
                    this->districtWindowDbAttrs[i][v]->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
                    this->districtWindowDbAttrs[i][v]->setDescription("Window material variant " + Ogre::StringConverter::toString(v) + ".");
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
                    this->districtTrimDbAttrs[i][v]->setDescription("Building face material variant " + Ogre::StringConverter::toString(v) + ". Used for lower building facade elements.");
                    this->districts[i].trimDatablocks[v] = trimDef[v];

                    if (v == 1)
                    {
                        this->districtTrimDbAttrs[i][v]->addUserData(GameObject::AttrActionSeparator());
                    }
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

    void ProceduralCityComponent::setDistrictFaceDatablock(unsigned int districtIdx, unsigned int variantIdx, const Ogre::String& name)
    {
        if (districtIdx < this->districts.size() && variantIdx < 6u)
        {
            this->districts[districtIdx].faceDatablocks[variantIdx] = name;
            if (this->districtFaceDbAttrs[districtIdx][variantIdx])
            {
                this->districtFaceDbAttrs[districtIdx][variantIdx]->setValue(name);
            }
        }
    }

    Ogre::String ProceduralCityComponent::getDistrictFaceDatablock(unsigned int districtIdx, unsigned int variantIdx) const
    {
        return (districtIdx < this->districts.size() && variantIdx < 6u) ? this->districts[districtIdx].faceDatablocks[variantIdx] : "";
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

    // =========================================================================
    // Selection overlay — wireframe box around the selected building
    // =========================================================================

    void ProceduralCityComponent::createSelectionOverlay(void)
    {
        GraphicsModule::getInstance()->enqueueAndWait(
            [this]()
            {
                this->selOverlayNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();
                this->selOverlayObject = this->gameObjectPtr->getSceneManager()->createManualObject();
                this->selOverlayObject->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);
                this->selOverlayObject->setName("CitySelOverlay_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()));
                this->selOverlayObject->setQueryFlags(0u);
                this->selOverlayObject->setCastShadows(false);
                this->selOverlayNode->attachObject(this->selOverlayObject);
                this->selOverlayNode->setVisible(false);
            },
            "ProceduralCityComponent::createSelectionOverlay");
    }

    void ProceduralCityComponent::destroySelectionOverlay(void)
    {
        GraphicsModule::getInstance()->enqueueAndWait(
            [this]()
            {
                if (nullptr == this->selOverlayNode)
                {
                    return;
                }
                this->selOverlayNode->detachAllObjects();
                if (this->selOverlayObject)
                {
                    this->gameObjectPtr->getSceneManager()->destroyManualObject(this->selOverlayObject);
                    this->selOverlayObject = nullptr;
                }
                this->selOverlayNode->getParentSceneNode()->removeAndDestroyChild(this->selOverlayNode);
                this->selOverlayNode = nullptr;
            },
            "ProceduralCityComponent::destroySelectionOverlay");
    }

    void ProceduralCityComponent::updateSelectionOverlay(void)
    {
        if (nullptr == this->selOverlayObject || nullptr == this->selOverlayNode)
        {
            return;
        }
        if (AppStateManager::getSingletonPtr()->getGameObjectController()->getIsDestroying())
        {
            return;
        }

        // Collect lines for the selected building's wireframe box
        // Each line = two 3D positions + one colour.  Format mirrors PRC's overlay.
        struct OverlayVert
        {
            Ogre::Vector3 pos;
            Ogre::ColourValue col;
        };
        std::vector<OverlayVert> lines;

        if (this->selectedBuildingIdx >= 0 && static_cast<size_t>(this->selectedBuildingIdx) < this->storedBuildings.size())
        {
            const BuildingInstance& b = this->storedBuildings[static_cast<size_t>(this->selectedBuildingIdx)];

            const Ogre::Real hw = b.footprint.x * 0.5f;
            const Ogre::Real hd = b.footprint.z * 0.5f;
            const Ogre::Real ht = b.footprint.y;
            const Ogre::Real y0 = b.groundHeight;
            const Ogre::Real y1 = y0 + ht;

            // Build oriented corners in local space then rotate by orientation
            const Ogre::Vector3 cx = b.orientation * Ogre::Vector3(hw, 0.f, 0.f);
            const Ogre::Vector3 cz = b.orientation * Ogre::Vector3(0.f, 0.f, hd);
            const Ogre::Vector3 ctr(b.position.x, 0.f, b.position.z);

            // 4 bottom corners, 4 top corners
            const Ogre::Vector3 b0 = ctr - cx - cz + Ogre::Vector3(0, y0, 0);
            const Ogre::Vector3 b1 = ctr + cx - cz + Ogre::Vector3(0, y0, 0);
            const Ogre::Vector3 b2 = ctr + cx + cz + Ogre::Vector3(0, y0, 0);
            const Ogre::Vector3 b3 = ctr - cx + cz + Ogre::Vector3(0, y0, 0);
            const Ogre::Vector3 t0 = b0 + Ogre::Vector3(0, ht, 0);
            const Ogre::Vector3 t1 = b1 + Ogre::Vector3(0, ht, 0);
            const Ogre::Vector3 t2 = b2 + Ogre::Vector3(0, ht, 0);
            const Ogre::Vector3 t3 = b3 + Ogre::Vector3(0, ht, 0);

            const Ogre::ColourValue col(1.f, 0.85f, 0.f, 1.f); // yellow selection
            auto addLine = [&](const Ogre::Vector3& a, const Ogre::Vector3& bv)
            {
                lines.push_back({a, col});
                lines.push_back({bv, col});
            };
            // Bottom quad
            addLine(b0, b1);
            addLine(b1, b2);
            addLine(b2, b3);
            addLine(b3, b0);
            // Top quad
            addLine(t0, t1);
            addLine(t1, t2);
            addLine(t2, t3);
            addLine(t3, t0);
            // Vertical edges
            addLine(b0, t0);
            addLine(b1, t1);
            addLine(b2, t2);
            addLine(b3, t3);
        }

        GraphicsModule::getInstance()->enqueue(
            [this, lines = std::move(lines)]()
            {
                if (nullptr == this->selOverlayObject)
                {
                    return;
                }
                this->selOverlayObject->clear();
                if (lines.empty())
                {
                    if (this->selOverlayNode)
                    {
                        this->selOverlayNode->setVisible(false);
                    }
                    return;
                }
                try
                {
                    this->selOverlayObject->begin("WhiteNoLightingBackground", Ogre::OT_LINE_LIST);
                    Ogre::uint32 idx2 = 0u;
                    for (const auto& v : lines)
                    {
                        this->selOverlayObject->position(v.pos);
                        this->selOverlayObject->colour(v.col);
                        this->selOverlayObject->index(idx2++);
                    }
                    this->selOverlayObject->end();
                }
                catch (Ogre::Exception& e)
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralCityComponent] selOverlay begin() failed: " + e.getDescription());
                }
                if (this->selOverlayNode)
                {
                    this->selOverlayNode->setVisible(true);
                }
            },
            "ProceduralCityComponent::selOverlay_draw");
    }
} // namespace NOWA
#include "NOWAPrecompiled.h"
#include "ProceduralFoliageVolumeComponent.h"
#include "../../PlanetTerraComponent/code/PlanetTerraComponent.h"
#include "gameobject/GameObjectFactory.h"
#include "gameobject/PhysicsArtifactComponent.h"
#include "gameobject/TerraComponent.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "main/EventManager.h"
#include "modules/GraphicsModule.h"
#include "modules/LuaScriptApi.h"
#include "utilities/XMLConverter.h"

#include "OgreBitwise.h"
#include "OgreConfigFile.h"
#include "OgreHlmsPbsPrerequisites.h"
#include "OgreLodConfig.h"
#include "OgreLodStrategyManager.h"
#include "OgreMesh2Serializer.h"
#include "OgreMeshLodGenerator.h"
#include "OgreMeshManager2.h"
#include "OgrePixelCountLodStrategy.h"
#include "OgreSubMesh2.h"
#include "Vao/OgreAsyncTicket.h"

#include "OgreAbiUtils.h"

#include <future>
#include <random>

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    ProceduralFoliageVolumeComponent::ProceduralFoliageVolumeComponent() :
        GameObjectComponent(),
        name("ProceduralFoliageVolumeComponent"),
        isDirty(true),
        foliageLoadedFromScene(false),
        spatialHashCellSize(10.0f),
        raySceneQuery(nullptr),
        sphereSceneQuery(nullptr),
        physicsArtifactComponent(nullptr),
        volumeBounds(new Variant(ProceduralFoliageVolumeComponent::AttrVolumeBounds(), Ogre::Vector4(-50.0f, -50.0f, 50.0f, 50.0f), this->attributes)), // 100x100m area
        masterSeed(new Variant(ProceduralFoliageVolumeComponent::AttrMasterSeed(), static_cast<unsigned int>(12345), this->attributes)),
        gridResolution(new Variant(ProceduralFoliageVolumeComponent::AttrGridResolution(), 2.0f, this->attributes)), // 1 meter sample resolution
        regenerate(new Variant(ProceduralFoliageVolumeComponent::AttrRegenerate(), "Regenerate", this->attributes)),
        clear(new Variant(ProceduralFoliageVolumeComponent::AttrClear(), "Clear", this->attributes)),
        randomizeSeed(new Variant(ProceduralFoliageVolumeComponent::AttrRandomizeSeed(), "Randomize Seed", this->attributes)),
        ruleCount(new Variant(ProceduralFoliageVolumeComponent::AttrRuleCount(), static_cast<unsigned int>(0), this->attributes))
    {
        // Setup action buttons
        this->regenerate->setDescription("Generate/regenerate vegetation using all rules.");
        this->regenerate->addUserData(GameObject::AttrActionExec());
        this->regenerate->addUserData(GameObject::AttrActionExecId(), "ProceduralFoliageVolumeComponent.Regenerate");

        this->clear->setDescription("Clear all vegetation.");
        this->clear->addUserData(GameObject::AttrActionExec());
        this->clear->addUserData(GameObject::AttrActionExecId(), "ProceduralFoliageVolumeComponent.Clear");

        this->randomizeSeed->setDescription("Randomize master seed and regenerate.");
        this->randomizeSeed->addUserData(GameObject::AttrActionExec());
        this->randomizeSeed->addUserData(GameObject::AttrActionExecId(), "ProceduralFoliageVolumeComponent.RandomizeSeed");

        // Setup descriptions
        this->volumeBounds->setDescription("AABB volume bounds (min.x, min.z, max.x, max.z) in world space. Match your terrain size.");
        this->masterSeed->setDescription("Master seed for reproducible placement. Same seed = same vegetation.");
        this->gridResolution->setDescription("Grid sample resolution in meters. Lower = denser sampling but slower. Typical: 0.5-2.0");
        this->ruleCount->addUserData(GameObject::AttrActionSeparator());
        this->ruleCount->setDescription("Number of foliage rules (different vegetation types). Start with 1-3.");
        this->ruleCount->addUserData(GameObject::AttrActionNeedRefresh());

        // Add constraints
        this->masterSeed->setConstraints(0u, UINT32_MAX);
        this->gridResolution->setConstraints(0.1f, 10.0f);
    }

    ProceduralFoliageVolumeComponent::~ProceduralFoliageVolumeComponent()
    {
        // Destruction handled in onRemoveComponent
    }

    void ProceduralFoliageVolumeComponent::install(const Ogre::NameValuePairList* options)
    {
        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<ProceduralFoliageVolumeComponent>(ProceduralFoliageVolumeComponent::getStaticClassId(), ProceduralFoliageVolumeComponent::getStaticClassName());
    }

    void ProceduralFoliageVolumeComponent::initialise()
    {
        // Called too early
    }

    void ProceduralFoliageVolumeComponent::shutdown()
    {
        // Called too late
    }

    void ProceduralFoliageVolumeComponent::uninstall()
    {
        // Called too late
    }

    void ProceduralFoliageVolumeComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    const Ogre::String& ProceduralFoliageVolumeComponent::getName() const
    {
        return this->name;
    }

    bool ProceduralFoliageVolumeComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        // Load volume settings (use attribute name helpers consistently)
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralFoliageVolumeComponent::AttrVolumeBounds())
        {
            this->volumeBounds->setValue(XMLConverter::getAttribVector4(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralFoliageVolumeComponent::AttrMasterSeed())
        {
            this->masterSeed->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralFoliageVolumeComponent::AttrGridResolution())
        {
            this->gridResolution->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralFoliageVolumeComponent::AttrRuleCount())
        {
            this->ruleCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        // Initialize rule arrays
        unsigned int count = this->ruleCount->getUInt();
        this->rules.resize(count);

        if (this->ruleNames.size() < count)
        {
            this->ruleNames.resize(count);
            this->ruleMeshNames.resize(count);
            this->ruleDensities.resize(count);
            this->ruleHeightRanges.resize(count);
            this->ruleMaxSlopes.resize(count);
            this->ruleTerraLayers.resize(count);
            this->ruleScaleRanges.resize(count);
            this->ruleMinSpacings.resize(count);
            this->ruleRenderDistances.resize(count);
            this->ruleLodDistances.resize(count);
            this->ruleCastShadows.resize(count);
            this->ruleShadowDistances.resize(count);
            this->ruleCategories.resize(count);
            this->ruleClearanceDistances.resize(count);
            this->ruleCollisionEnabled.resize(count);
            this->ruleCollisionRadius.resize(count);
            this->ruleCollisionHeight.resize(count);
        }

        this->masterSeed->setConstraints(0u, UINT32_MAX);
        this->gridResolution->setConstraints(0.1f, 10.0f);
        this->ruleCount->setConstraints(0u, 20u);

        // Load each rule's properties
        for (size_t i = 0; i < count; i++)
        {
            if (this->ruleDensities[i])
            {
                this->ruleDensities[i]->setConstraints(0.01f, 10.0f);
            }

            if (this->ruleMaxSlopes[i])
            {
                this->ruleMaxSlopes[i]->setConstraints(0.0f, 90.0f);
            }

            if (this->ruleMinSpacings[i])
            {
                this->ruleMinSpacings[i]->setConstraints(0.0f, 100.0f);
            }

            if (this->ruleRenderDistances[i])
            {
                this->ruleRenderDistances[i]->setConstraints(0.0f, 10000.0f);
            }

            if (this->ruleLodDistances[i])
            {
                this->ruleLodDistances[i]->setConstraints(0.0f, 1000.0f);
            }

            // Rule Name
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == (ProceduralFoliageVolumeComponent::AttrRuleName() + Ogre::StringConverter::toString(i)))
            {
                Ogre::String name = XMLConverter::getAttrib(propertyElement, "data");
                if (nullptr == this->ruleNames[i])
                {
                    this->ruleNames[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleName() + Ogre::StringConverter::toString(i), Ogre::String(), this->attributes);
                    this->ruleNames[i]->setDescription("Name for this vegetation rule.");
                }
                this->ruleNames[i]->setValue(name);
                this->rules[i].name = name;
                propertyElement = propertyElement->next_sibling("property");
            }

            // Rule Mesh Name
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == (ProceduralFoliageVolumeComponent::AttrRuleMeshName() + Ogre::StringConverter::toString(i)))
            {
                Ogre::String meshName = XMLConverter::getAttrib(propertyElement, "data");
                if (nullptr == this->ruleMeshNames[i])
                {
                    this->ruleMeshNames[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleMeshName() + Ogre::StringConverter::toString(i), Ogre::String(), this->attributes);
                    this->ruleMeshNames[i]->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
                    this->ruleMeshNames[i]->setDescription("Mesh for this vegetation type.");
                }
                this->ruleMeshNames[i]->setValue(meshName);
                this->rules[i].meshName = meshName;
                propertyElement = propertyElement->next_sibling("property");
            }

            // Rule Resource Group
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == (ProceduralFoliageVolumeComponent::AttrRuleResourceGroup() + Ogre::StringConverter::toString(i)))
            {
                Ogre::String resourceGroup = XMLConverter::getAttrib(propertyElement, "data");
                this->rules[i].resourceGroup = resourceGroup;
                propertyElement = propertyElement->next_sibling("property");
            }

            // Rule Density
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == (ProceduralFoliageVolumeComponent::AttrRuleDensity() + Ogre::StringConverter::toString(i)))
            {
                Ogre::Real density = XMLConverter::getAttribReal(propertyElement, "data");
                if (nullptr == this->ruleDensities[i])
                {
                    this->ruleDensities[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleDensity() + Ogre::StringConverter::toString(i), 1.0f, this->attributes);
                    this->ruleDensities[i]->setDescription("Instances per m * m (0.01 - 10.0).");
                }
                this->ruleDensities[i]->setValue(density);
                this->rules[i].density = density;
                propertyElement = propertyElement->next_sibling("property");
            }

            // Rule Height Range
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == (ProceduralFoliageVolumeComponent::AttrRuleHeightRange() + Ogre::StringConverter::toString(i)))
            {
                Ogre::Vector2 range = XMLConverter::getAttribVector2(propertyElement, "data");
                if (nullptr == this->ruleHeightRanges[i])
                {
                    this->ruleHeightRanges[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleHeightRange() + Ogre::StringConverter::toString(i), Ogre::Vector2(-FLT_MAX, FLT_MAX), this->attributes);
                    this->ruleHeightRanges[i]->setDescription("Min/max elevation in meters.");
                }
                this->ruleHeightRanges[i]->setValue(range);
                this->rules[i].heightRange = range;
                propertyElement = propertyElement->next_sibling("property");
            }

            // Rule Max Slope
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == (ProceduralFoliageVolumeComponent::AttrRuleMaxSlope() + Ogre::StringConverter::toString(i)))
            {
                Ogre::Real maxSlope = XMLConverter::getAttribReal(propertyElement, "data");
                if (nullptr == this->ruleMaxSlopes[i])
                {
                    this->ruleMaxSlopes[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleMaxSlope() + Ogre::StringConverter::toString(i), 90.0f, this->attributes);
                    this->ruleMaxSlopes[i]->setDescription("Maximum slope angle in degrees (0-90).");
                }
                this->ruleMaxSlopes[i]->setValue(maxSlope);
                this->rules[i].maxSlope = maxSlope;
                propertyElement = propertyElement->next_sibling("property");
            }

            // Rule Terra Layers
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == (ProceduralFoliageVolumeComponent::AttrRuleTerraLayers() + Ogre::StringConverter::toString(i)))
            {
                Ogre::String layers = XMLConverter::getAttrib(propertyElement, "data");
                if (nullptr == this->ruleTerraLayers[i])
                {
                    this->ruleTerraLayers[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleTerraLayers() + Ogre::StringConverter::toString(i), Ogre::String("255,255,255,255"), this->attributes);
                    this->ruleTerraLayers[i]->setDescription("Terra layer thresholds (e.g., 255,200,0,0).");
                }
                this->ruleTerraLayers[i]->setValue(layers);
                this->rules[i].terraLayerThresholds = this->parseTerraLayers(layers);
                propertyElement = propertyElement->next_sibling("property");
            }

            // Rule Scale Range
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == (ProceduralFoliageVolumeComponent::AttrRuleScaleRange() + Ogre::StringConverter::toString(i)))
            {
                Ogre::Vector2 range = XMLConverter::getAttribVector2(propertyElement, "data");
                if (nullptr == this->ruleScaleRanges[i])
                {
                    this->ruleScaleRanges[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleScaleRange() + Ogre::StringConverter::toString(i), Ogre::Vector2(0.8f, 1.2f), this->attributes);
                    this->ruleScaleRanges[i]->setDescription("Min/max scale multiplier.");
                }
                this->ruleScaleRanges[i]->setValue(range);
                this->rules[i].scaleRange = range;
                propertyElement = propertyElement->next_sibling("property");
            }

            // Rule Min Spacing
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == (ProceduralFoliageVolumeComponent::AttrRuleMinSpacing() + Ogre::StringConverter::toString(i)))
            {
                Ogre::Real spacing = XMLConverter::getAttribReal(propertyElement, "data");
                if (nullptr == this->ruleMinSpacings[i])
                {
                    this->ruleMinSpacings[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleMinSpacing() + Ogre::StringConverter::toString(i), 0.0f, this->attributes);
                    this->ruleMinSpacings[i]->setDescription("Minimum spacing between instances (meters).");
                }
                this->ruleMinSpacings[i]->setValue(spacing);
                this->rules[i].minDistanceToSame = spacing;
                propertyElement = propertyElement->next_sibling("property");
            }

            // Rule Render Distance
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == (ProceduralFoliageVolumeComponent::AttrRuleRenderDistance() + Ogre::StringConverter::toString(i)))
            {
                Ogre::Real distance = XMLConverter::getAttribReal(propertyElement, "data");
                if (nullptr == this->ruleRenderDistances[i])
                {
                    this->ruleRenderDistances[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleRenderDistance() + Ogre::StringConverter::toString(i), 0.0f, this->attributes);
                    this->ruleRenderDistances[i]->setDescription("Render distance in meters (0 = infinite).");
                }
                this->ruleRenderDistances[i]->setValue(distance);
                this->rules[i].renderDistance = distance;
                propertyElement = propertyElement->next_sibling("property");
            }

            // Rule LOD Distance
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == (ProceduralFoliageVolumeComponent::AttrRuleLodDistance() + Ogre::StringConverter::toString(i)))
            {
                Ogre::Real distance = XMLConverter::getAttribReal(propertyElement, "data");
                if (nullptr == this->ruleLodDistances[i])
                {
                    this->ruleLodDistances[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleLodDistance() + Ogre::StringConverter::toString(i), 0.0f, this->attributes);
                    this->ruleLodDistances[i]->setDescription("LOD transition distance (0 = auto-generate).");
                }
                this->ruleLodDistances[i]->setValue(distance);
                this->rules[i].lodDistance = distance;
                propertyElement = propertyElement->next_sibling("property");
            }

            // Rule Cast Shadows
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralFoliageVolumeComponent::AttrRuleCastShadows() + Ogre::StringConverter::toString(i))
            {
                bool enabled = XMLConverter::getAttribBool(propertyElement, "data");
                if (nullptr == this->ruleCastShadows[i])
                {
                    this->ruleCastShadows[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleCastShadows() + Ogre::StringConverter::toString(i), false, this->attributes);
                    this->ruleCastShadows[i]->setDescription("Enable shadow casts.");
                }
                this->ruleCastShadows[i]->setValue(enabled);
                this->rules[i].castShadows = enabled;
                propertyElement = propertyElement->next_sibling("property");
            }

            // Rule Shadow Distance
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == (ProceduralFoliageVolumeComponent::AttrRuleShadowDistance() + Ogre::StringConverter::toString(i)))
            {
                Ogre::Real distance = XMLConverter::getAttribReal(propertyElement, "data");
                if (nullptr == this->ruleShadowDistances[i])
                {
                    this->ruleShadowDistances[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleShadowDistance() + Ogre::StringConverter::toString(i), 0.0f, this->attributes);
                    this->ruleShadowDistances[i]->setDescription("The shadow distance in meters.");
                }
                this->ruleShadowDistances[i]->setValue(distance);
                this->rules[i].shadowDistance = distance;
                propertyElement = propertyElement->next_sibling("property");
            }

            // Rule Categories
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == (ProceduralFoliageVolumeComponent::AttrRuleCategories() + Ogre::StringConverter::toString(i)))
            {
                Ogre::String cats = XMLConverter::getAttrib(propertyElement, "data");
                if (nullptr == this->ruleCategories[i])
                {
                    this->ruleCategories[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleCategories() + Ogre::StringConverter::toString(i), Ogre::String("All"), this->attributes);
                    this->ruleCategories[i]->setDescription("Categories mask for raycasting. 'All' = grow everywhere. 'All-House' = everywhere except on Houses. 'Ground' = only on ground objects..");
                }
                this->ruleCategories[i]->setValue(cats);
                this->rules[i].categories = cats;
                this->rules[i].categoriesId = AppStateManager::getSingletonPtr()->getGameObjectController()->generateCategoryId(cats);
                propertyElement = propertyElement->next_sibling("property");
            }

            // Rule Clearance Distance
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == (ProceduralFoliageVolumeComponent::AttrRuleClearanceDistance() + Ogre::StringConverter::toString(i)))
            {
                Ogre::Real dist = XMLConverter::getAttribReal(propertyElement, "data");
                if (nullptr == this->ruleClearanceDistances[i])
                {
                    this->ruleClearanceDistances[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleClearanceDistance() + Ogre::StringConverter::toString(i), 0.0f, this->attributes);
                    this->ruleClearanceDistances[i]->setDescription("Clearance buffer in meters around excluded objects. 0 = disabled.");
                    this->ruleClearanceDistances[i]->setConstraints(0.0f, 50.0f);
                }
                this->ruleClearanceDistances[i]->setValue(dist);
                this->rules[i].clearanceDistance = dist;
                propertyElement = propertyElement->next_sibling("property");
            }

            // Rule Collision Enabled
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralFoliageVolumeComponent::AttrRuleCollisionEnabled() + Ogre::StringConverter::toString(i))
            {
                bool enabled = XMLConverter::getAttribBool(propertyElement, "data");
                if (nullptr == this->ruleCollisionEnabled[i])
                {
                    this->ruleCollisionEnabled[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleCollisionEnabled() + Ogre::StringConverter::toString(i), false, this->attributes);
                    this->ruleCollisionEnabled[i]->setDescription("Enable physics collision for this vegetation.");
                }
                this->ruleCollisionEnabled[i]->setValue(enabled);
                this->rules[i].collisionEnabled = enabled;
                propertyElement = propertyElement->next_sibling("property");
            }

            // Rule Collision Radius
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralFoliageVolumeComponent::AttrRuleCollisionRadius() + Ogre::StringConverter::toString(i))
            {
                Ogre::Real radius = XMLConverter::getAttribReal(propertyElement, "data");
                if (nullptr == this->ruleCollisionRadius[i])
                {
                    this->ruleCollisionRadius[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleCollisionRadius() + Ogre::StringConverter::toString(i), 0.3f, this->attributes);
                    this->ruleCollisionRadius[i]->setDescription("Collision cylinder radius in meters (trunk thickness).");
                    this->ruleCollisionRadius[i]->setConstraints(0.01f, 5.0f);
                }
                this->ruleCollisionRadius[i]->setValue(radius);
                this->rules[i].collisionRadius = radius;
                propertyElement = propertyElement->next_sibling("property");
            }

            // Rule Collision Height
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralFoliageVolumeComponent::AttrRuleCollisionHeight() + Ogre::StringConverter::toString(i))
            {
                Ogre::Real height = XMLConverter::getAttribReal(propertyElement, "data");
                if (nullptr == this->ruleCollisionHeight[i])
                {
                    this->ruleCollisionHeight[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleCollisionHeight() + Ogre::StringConverter::toString(i), 2.0f, this->attributes);
                    this->ruleCollisionHeight[i]->setDescription("Collision cylinder height in meters.");
                    this->ruleCollisionHeight[i]->setConstraints(0.1f, 20.0f);
                }
                this->ruleCollisionHeight[i]->setValue(height);
                this->ruleCollisionHeight[i]->addUserData(GameObject::AttrActionSeparator());
                this->rules[i].collisionHeight = height;
                propertyElement = propertyElement->next_sibling("property");
            }
        }

        this->foliageLoadedFromScene = true;

        return true;
    }

    bool ProceduralFoliageVolumeComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralFoliageVolumeComponent] Init component for game object: " + this->gameObjectPtr->getName());

        if (nullptr == this->raySceneQuery)
        {
            this->raySceneQuery = this->gameObjectPtr->getSceneManager()->createRayQuery(Ogre::Ray(), GameObjectController::ALL_CATEGORIES_ID);
            this->raySceneQuery->setSortByDistance(true);
        }

        if (nullptr == this->sphereSceneQuery)
        {
            this->sphereSceneQuery = this->gameObjectPtr->getSceneManager()->createSphereQuery(Ogre::Sphere(), GameObjectController::ALL_CATEGORIES_ID);
        }

        if (true == this->foliageLoadedFromScene)
        {
            this->regenerateFoliage();
        }

        return true;
    }

    bool ProceduralFoliageVolumeComponent::connect(void)
    {
        GameObjectComponent::connect();

        return true;
    }

    bool ProceduralFoliageVolumeComponent::disconnect(void)
    {
        GameObjectComponent::disconnect();
        return true;
    }

    GameObjectCompPtr ProceduralFoliageVolumeComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        // Not implemented
        return GameObjectCompPtr();
    }

    bool ProceduralFoliageVolumeComponent::onCloned(void)
    {
        return true;
    }

    void ProceduralFoliageVolumeComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();

        this->physicsArtifactComponent = nullptr;

        if (this->raySceneQuery)
        {
            auto* query = this->raySceneQuery;
            GraphicsModule::getInstance()->enqueueAndWait(
                [this, query]()
                {
                    this->gameObjectPtr->getSceneManager()->destroyQuery(query);
                },
                "ProceduralFoliageVolumeComponent::DestroyRayQuery");
            this->raySceneQuery = nullptr;
        }

        if (this->sphereSceneQuery)
        {
            auto* query = this->sphereSceneQuery;
            GraphicsModule::getInstance()->enqueueAndWait(
                [this, query]()
                {
                    this->gameObjectPtr->getSceneManager()->destroyQuery(query);
                },
                "ProceduralFoliageVolumeComponent::DestroySphereQuery");
            this->sphereSceneQuery = nullptr;
        }

        // Destroy all vegetation (blocking)
        this->clearFoliage();

        boost::shared_ptr<EventDataRefreshGui> eventDataRefreshGui(new EventDataRefreshGui());
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshGui);
    }

    void ProceduralFoliageVolumeComponent::onOtherComponentRemoved(unsigned int index)
    {
        if (nullptr != this->physicsArtifactComponent && index == this->physicsArtifactComponent->getIndex())
        {
            this->physicsArtifactComponent = nullptr;
        }
    }

    void ProceduralFoliageVolumeComponent::onOtherComponentAdded(unsigned int index)
    {
        // Not needed
    }

    void ProceduralFoliageVolumeComponent::update(Ogre::Real dt, bool notSimulating)
    {
        // Vegetation is static, no update needed
    }

    void ProceduralFoliageVolumeComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        // Volume settings
        if (ProceduralFoliageVolumeComponent::AttrVolumeBounds() == attribute->getName())
        {
            this->setVolumeBounds(Ogre::Aabb::newFromExtents(Ogre::Vector3(attribute->getVector4().x, -10000.0f, attribute->getVector4().y), Ogre::Vector3(attribute->getVector4().z, 10000.0f, attribute->getVector4().w)));
        }
        else if (ProceduralFoliageVolumeComponent::AttrMasterSeed() == attribute->getName())
        {
            this->setMasterSeed(attribute->getUInt());
        }
        else if (ProceduralFoliageVolumeComponent::AttrGridResolution() == attribute->getName())
        {
            this->setGridResolution(attribute->getReal());
        }
        else if (ProceduralFoliageVolumeComponent::AttrRuleCount() == attribute->getName())
        {
            this->setRuleCount(attribute->getUInt());
        }
        else
        {
            // Check each rule's properties
            for (unsigned int i = 0; i < static_cast<unsigned int>(this->rules.size()); i++)
            {
                if (ProceduralFoliageVolumeComponent::AttrRuleName() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setRuleName(i, attribute->getString());
                }
                else if (ProceduralFoliageVolumeComponent::AttrRuleMeshName() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setRuleMeshName(i, attribute->getString());
                }
                else if (ProceduralFoliageVolumeComponent::AttrRuleDensity() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setRuleDensity(i, attribute->getReal());
                }
                else if (ProceduralFoliageVolumeComponent::AttrRuleHeightRange() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setRuleHeightRange(i, attribute->getVector2());
                }
                else if (ProceduralFoliageVolumeComponent::AttrRuleMaxSlope() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setRuleMaxSlope(i, attribute->getReal());
                }
                else if (ProceduralFoliageVolumeComponent::AttrRuleTerraLayers() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setRuleTerraLayers(i, attribute->getString());
                }
                else if (ProceduralFoliageVolumeComponent::AttrRuleScaleRange() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setRuleScaleRange(i, attribute->getVector2());
                }
                else if (ProceduralFoliageVolumeComponent::AttrRuleMinSpacing() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setRuleMinSpacing(i, attribute->getReal());
                }
                else if (ProceduralFoliageVolumeComponent::AttrRuleRenderDistance() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setRuleRenderDistance(i, attribute->getReal());
                }
                else if (ProceduralFoliageVolumeComponent::AttrRuleLodDistance() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setRuleLodDistance(i, attribute->getReal());
                }
                else if (ProceduralFoliageVolumeComponent::AttrRuleCastShadows() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setRuleCastShadows(i, attribute->getBool());
                }
                else if (ProceduralFoliageVolumeComponent::AttrRuleShadowDistance() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setRuleShadowDistance(i, attribute->getReal());
                }
                else if (ProceduralFoliageVolumeComponent::AttrRuleCategories() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setRuleCategories(i, attribute->getString());
                }
                else if (ProceduralFoliageVolumeComponent::AttrRuleClearanceDistance() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setRuleClearanceDistance(i, attribute->getReal());
                }
                else if (ProceduralFoliageVolumeComponent::AttrRuleCollisionEnabled() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setRuleCollisionEnabled(i, attribute->getBool());
                }
                else if (ProceduralFoliageVolumeComponent::AttrRuleCollisionRadius() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setRuleCollisionRadius(i, attribute->getReal());
                }
                else if (ProceduralFoliageVolumeComponent::AttrRuleCollisionHeight() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setRuleCollisionHeight(i, attribute->getReal());
                }
            }
        }
    }

    void ProceduralFoliageVolumeComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        GameObjectComponent::writeXML(propertiesXML, doc);

        // Write volume settings
        xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "10")); // Vector4
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralFoliageVolumeComponent::AttrVolumeBounds())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->volumeBounds->getVector4())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2")); // Unsigned int
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralFoliageVolumeComponent::AttrMasterSeed())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->masterSeed->getUInt())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6")); // Real
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralFoliageVolumeComponent::AttrGridResolution())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->gridResolution->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2")); // Unsigned int
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralFoliageVolumeComponent::AttrRuleCount())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->ruleCount->getUInt())));
        propertiesXML->append_node(propertyXML);

        // Write each rule's properties
        for (size_t i = 0; i < this->rules.size(); i++)
        {
            // Rule Name
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "7")); // String
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralFoliageVolumeComponent::AttrRuleName() + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->ruleNames[i]->getString())));
            propertiesXML->append_node(propertyXML);

            // Rule Mesh Name
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "7")); // String
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralFoliageVolumeComponent::AttrRuleMeshName() + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->ruleMeshNames[i]->getString())));
            propertiesXML->append_node(propertyXML);

            // Rule Resource Group
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "7")); // String
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralFoliageVolumeComponent::AttrRuleResourceGroup() + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rules[i].resourceGroup)));
            propertiesXML->append_node(propertyXML);

            // Rule Density
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6")); // Real
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralFoliageVolumeComponent::AttrRuleDensity() + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->ruleDensities[i]->getReal())));
            propertiesXML->append_node(propertyXML);

            // Rule Height Range
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "8")); // Vector2
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralFoliageVolumeComponent::AttrRuleHeightRange() + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->ruleHeightRanges[i]->getVector2())));
            propertiesXML->append_node(propertyXML);

            // Rule Max Slope
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6")); // Real
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralFoliageVolumeComponent::AttrRuleMaxSlope() + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->ruleMaxSlopes[i]->getReal())));
            propertiesXML->append_node(propertyXML);

            // Rule Terra Layers
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "7")); // String
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralFoliageVolumeComponent::AttrRuleTerraLayers() + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->ruleTerraLayers[i]->getString())));
            propertiesXML->append_node(propertyXML);

            // Rule Scale Range
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "8")); // Vector2
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralFoliageVolumeComponent::AttrRuleScaleRange() + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->ruleScaleRanges[i]->getVector2())));
            propertiesXML->append_node(propertyXML);

            // Rule Min Spacing
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6")); // Real
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralFoliageVolumeComponent::AttrRuleMinSpacing() + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->ruleMinSpacings[i]->getReal())));
            propertiesXML->append_node(propertyXML);

            // Rule Render Distance
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6")); // Real
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralFoliageVolumeComponent::AttrRuleRenderDistance() + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->ruleRenderDistances[i]->getReal())));
            propertiesXML->append_node(propertyXML);

            // Rule LOD Distance
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6")); // Real
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralFoliageVolumeComponent::AttrRuleLodDistance() + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->ruleLodDistances[i]->getReal())));
            propertiesXML->append_node(propertyXML);

            // Rule Cast Shadows
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "12")); // Bool
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralFoliageVolumeComponent::AttrRuleCastShadows() + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->ruleCastShadows[i]->getBool())));
            propertiesXML->append_node(propertyXML);

            // Rule Shadow Distance
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6")); // Real
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralFoliageVolumeComponent::AttrRuleShadowDistance() + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->ruleShadowDistances[i]->getReal())));
            propertiesXML->append_node(propertyXML);

            // Rule Categories
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "7")); // String
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralFoliageVolumeComponent::AttrRuleCategories() + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->ruleCategories[i]->getString())));
            propertiesXML->append_node(propertyXML);

            // Rule Clearance Distance
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6")); // Real
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralFoliageVolumeComponent::AttrRuleClearanceDistance() + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->ruleClearanceDistances[i]->getReal())));
            propertiesXML->append_node(propertyXML);

            // Rule Collision Enabled
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "12")); // Bool
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralFoliageVolumeComponent::AttrRuleCollisionEnabled() + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->ruleCollisionEnabled[i]->getBool())));
            propertiesXML->append_node(propertyXML);

            // Rule Collision Radius
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6")); // Real
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralFoliageVolumeComponent::AttrRuleCollisionRadius() + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->ruleCollisionRadius[i]->getReal())));
            propertiesXML->append_node(propertyXML);

            // Rule Collision Height
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6")); // Real
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralFoliageVolumeComponent::AttrRuleCollisionHeight() + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->ruleCollisionHeight[i]->getReal())));
            propertiesXML->append_node(propertyXML);
        }
    }

    bool ProceduralFoliageVolumeComponent::executeAction(const Ogre::String& actionId, NOWA::Variant* attribute)
    {
        if (actionId == "ProceduralFoliageVolumeComponent.Regenerate")
        {
            this->regenerateFoliage();
            return true;
        }
        else if (actionId == "ProceduralFoliageVolumeComponent.Clear")
        {
            this->clearFoliage();
            return true;
        }
        else if (actionId == "ProceduralFoliageVolumeComponent.RandomizeSeed")
        {
            this->masterSeed->setValue(static_cast<unsigned int>(Ogre::Math::RangeRandom(0, UINT32_MAX)));
            this->regenerateFoliage();
            return true;
        }
        return false;
    }

    // ============================================================================
    // MAIN GENERATION FUNCTION
    // ============================================================================

    void ProceduralFoliageVolumeComponent::regenerateFoliage()
    {
        if (!this->gameObjectPtr)
        {
            return;
        }

        if (true == this->rules.empty())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralFoliageVolume] No rules defined, skipping generation.");
            return;
        }

        // Get PhysicsArtifactComponent if exists
        const auto& physicsArtifactCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsArtifactComponent>());
        if (physicsArtifactCompPtr)
        {
            this->physicsArtifactComponent = physicsArtifactCompPtr.get();
            this->physicsArtifactComponent->getAttribute(PhysicsArtifactComponent::AttrSerialize())->setValue(false);
            this->physicsArtifactComponent->getAttribute(PhysicsArtifactComponent::AttrSerialize())->setVisible(false);
        }

        // Clear existing
        this->clearFoliage();

        auto start = std::chrono::high_resolution_clock::now();

        // Calculate all positions (parallel, main thread)
        std::vector<VegetationBatch> batches = this->calculateFoliagePositions();

        if (true == batches.empty())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralFoliageVolume] No instances generated.");
            return;
        }

        // Send to render thread
        this->createFoliageOnRenderThread(std::move(batches));

        auto finish = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = finish - start;

        size_t totalInstances = 0;
        for (const auto& batch : this->vegetationBatches)
        {
            totalInstances += batch.items.size();
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralFoliageVolume] Generated " + Ogre::StringConverter::toString(totalInstances) + " instances from " + Ogre::StringConverter::toString(this->rules.size()) +
                                                                                " rules. Took: " + Ogre::StringConverter::toString(elapsed.count() * 0.001) + "s");

        this->isDirty = false;
    }

    // ============================================================================
    // POSITION CALCULATION (PARALLEL, MAIN THREAD)
    // ============================================================================

    std::vector<VegetationBatch> ProceduralFoliageVolumeComponent::calculateFoliagePositions()
    {
        // ====================================================================
        // PLANET MODE: check for PlanetTerraComponent on the same GO first.
        // ProceduralFoliageVolumeComponent is placed as a sibling component
        // under the same GameObject that has the PlanetTerraComponent.
        // If found, delegate entirely to the spherical ray-cast path and skip
        // the flat-terrain TerraComponent search below.
        // ====================================================================
        {
            auto planetTerraComp = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PlanetTerraComponent>());

            if (nullptr != planetTerraComp)
            {
                const Ogre::Real planetRadius = planetTerraComp->getRadius();

                if (planetRadius <= 0.0f)
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralFoliageVolume] PlanetTerraComponent found but radius is "
                                                                                        "zero or negative -- ensure radius is set correctly.");
                    return {};
                }

                const Ogre::Vector3 planetCentre = this->gameObjectPtr->getPosition();

                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralFoliageVolume] Planet mode: radius=" + Ogre::StringConverter::toString(planetRadius) + " centre=" + Ogre::StringConverter::toString(planetCentre));

                return this->calculatePlanetFoliagePositions(this->gameObjectPtr.get(), planetRadius, planetCentre);
            }
        }

        // ====================================================================
        // FLAT TERRAIN MODE (original code, unchanged)
        // ====================================================================

        // Get Terra
        Ogre::Terra* terra = nullptr;
        auto terraList = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsFromComponent(TerraComponent::getStaticClassName());
        if (false == terraList.empty())
        {
            auto terraCompPtr = NOWA::makeStrongPtr(terraList[0]->getComponent<TerraComponent>());
            if (terraCompPtr)
            {
                terra = terraCompPtr->getTerra();
            }
        }

        if (nullptr == terra)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralFoliageVolume] No Terra found!");
            return {};
        }

        Ogre::Vector4 boundsVec = this->volumeBounds->getVector4();
        Ogre::Vector2 minXZ(boundsVec.x, boundsVec.y);
        Ogre::Vector2 maxXZ(boundsVec.z, boundsVec.w);

        // ============================================================================
        // PHASE 1: PARALLEL - Terra height/slope/layer checks
        // ============================================================================
        std::vector<std::future<VegetationBatch>> futures;

        for (size_t ruleIdx = 0; ruleIdx < this->rules.size(); ruleIdx++)
        {
            const FoliageRule& rule = this->rules[ruleIdx];

            if (false == rule.enabled || true == rule.meshName.empty())
            {
                continue;
            }

            futures.emplace_back(std::async(std::launch::async,
                [=]() -> VegetationBatch
                {
                    VegetationBatch batch;
                    batch.meshName = rule.meshName;
                    batch.ruleIndex = ruleIdx;

                    std::mt19937 rng(this->masterSeed->getUInt() + rule.seed);
                    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);

                    const Ogre::Real resolution = this->gridResolution->getReal();
                    const Ogre::Real stepX = resolution / rule.density;
                    const Ogre::Real stepZ = resolution / rule.density;

                    for (Ogre::Real x = minXZ.x; x < maxXZ.x; x += stepX)
                    {
                        for (Ogre::Real z = minXZ.y; z < maxXZ.y; z += stepZ)
                        {
                            Ogre::Real jitterX = (dist01(rng) - 0.5f) * stepX * 0.8f;
                            Ogre::Real jitterZ = (dist01(rng) - 0.5f) * stepZ * 0.8f;

                            Ogre::Vector3 position(x + jitterX, 0.0f, z + jitterZ);

                            if (false == this->isWithinTerraBounds(position, terra))
                            {
                                continue;
                            }

                            if (false == terra->getHeightAt(position))
                            {
                                continue;
                            }

                            Ogre::Vector3 normal = this->calculateTerrainNormal(position, terra);

                            if (normal.y < 0.0f)
                            {
                                normal = -normal;
                            }

                            if (false == this->meetsTerrainCriteria(position, normal, rule, terra))
                            {
                                continue;
                            }

                            if (false == this->hasMinimumSpacing(position, rule, batch.instances))
                            {
                                continue;
                            }

                            Ogre::Real scaleRand = Ogre::Math::lerp(rule.scaleRange.x, rule.scaleRange.y, dist01(rng));
                            Ogre::Vector3 scale = rule.uniformScale ? Ogre::Vector3(scaleRand) : Ogre::Vector3(scaleRand, scaleRand, scaleRand);

                            Ogre::Quaternion orientation = Ogre::Quaternion::IDENTITY;
                            if (rule.alignToNormal > 0.0f)
                            {
                                Ogre::Vector3 up(Ogre::Math::lerp(Ogre::Vector3::UNIT_Y.x, normal.x, rule.alignToNormal), Ogre::Math::lerp(Ogre::Vector3::UNIT_Y.y, normal.y, rule.alignToNormal),
                                    Ogre::Math::lerp(Ogre::Vector3::UNIT_Y.z, normal.z, rule.alignToNormal));
                                up.normalise();
                                Ogre::Vector3 right = up.perpendicular();
                                Ogre::Vector3 forward = right.crossProduct(up);
                                orientation.FromAxes(right, up, forward);
                            }

                            if (rule.randomYRotation)
                            {
                                Ogre::Degree yRot(Ogre::Math::lerp(rule.yRotationRange.x, rule.yRotationRange.y, dist01(rng)));
                                Ogre::Quaternion yQuat;
                                yQuat.FromAngleAxis(yRot, Ogre::Vector3::UNIT_Y);
                                orientation = orientation * yQuat;
                            }

                            batch.instances.emplace_back(position, orientation, scale, ruleIdx);
                        }
                    }
                    return batch;
                }));
        }

        // Collect parallel results
        std::vector<VegetationBatch> batches;
        for (auto& future : futures)
        {
            VegetationBatch batch = future.get();
            if (false == batch.instances.empty())
            {
                batches.push_back(std::move(batch));
            }
        }

        // ============================================================================
        // PHASE 2: SEQUENTIAL - Category raycast filter
        // ============================================================================
        for (auto& batch : batches)
        {
            const FoliageRule& rule = this->rules[batch.ruleIndex];

            if (rule.categoriesId == GameObjectController::ALL_CATEGORIES_ID)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralFoliageVolume] Rule '" + rule.name + "' category=All, skipping raycast filter.");
                continue;
            }

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
                "[ProceduralFoliageVolume] Rule '" + rule.name + "' filtering " + Ogre::StringConverter::toString(batch.instances.size()) + " instances by category '" + rule.categories + "'...");

            std::vector<VegetationInstance> filtered;
            filtered.reserve(batch.instances.size());

            for (const auto& instance : batch.instances)
            {
                if (this->isCategoryAllowed(instance.position, rule))
                {
                    filtered.push_back(instance);
                }
            }

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
                "[ProceduralFoliageVolume] Rule '" + rule.name + "' kept " + Ogre::StringConverter::toString(filtered.size()) + " / " + Ogre::StringConverter::toString(batch.instances.size()) + " instances after category filter.");

            batch.instances = std::move(filtered);
        }

        batches.erase(std::remove_if(batches.begin(), batches.end(),
                          [](const VegetationBatch& b)
                          {
                              return b.instances.empty();
                          }),
            batches.end());

        return batches;
    }

    // ============================================================================
    // TERRAIN CRITERIA CHECKING
    // ============================================================================

    bool ProceduralFoliageVolumeComponent::meetsTerrainCriteria(const Ogre::Vector3& position, const Ogre::Vector3& normal, const FoliageRule& rule, Ogre::Terra* terra)
    {
        // Height check
        if (position.y < rule.heightRange.x || position.y > rule.heightRange.y)
        {
            return false;
        }

        // Slope check
        Ogre::Real slope = Ogre::Math::ACos(normal.dotProduct(Ogre::Vector3::UNIT_Y)).valueDegrees();

        if (slope > rule.maxSlope)
        {
            return false;
        }

        // Terra layer check
        if (terra && rule.terraLayerThresholds.size() == 4)
        {
            std::vector<int> layers = terra->getLayerAt(position);
            bool layerMatches = true;

            for (size_t i = 0; i < std::min(layers.size(), rule.terraLayerThresholds.size()); i++)
            {
                layerMatches &= (layers[i] <= rule.terraLayerThresholds[i]);
            }

            if (false == layerMatches)
            {
                return false;
            }
        }

        return true;
    }

    bool ProceduralFoliageVolumeComponent::hasMinimumSpacing(const Ogre::Vector3& position, const FoliageRule& rule, const std::vector<VegetationInstance>& existingInstances)
    {
        if (rule.minDistanceToSame <= 0.0f)
        {
            return true;
        }

        // Simple O(N) check - in production use spatial hash
        const Ogre::Real minDistSq = rule.minDistanceToSame * rule.minDistanceToSame;

        for (const auto& existing : existingInstances)
        {
            Ogre::Real distSq = position.squaredDistance(existing.position);
            if (distSq < minDistSq)
            {
                return false;
            }
        }

        return true;
    }

    Ogre::Vector3 ProceduralFoliageVolumeComponent::calculateTerrainNormal(const Ogre::Vector3& position, Ogre::Terra* terra)
    {
        // Sample neighboring heights
        const Ogre::Real offset = 0.5f;

        Ogre::Vector3 p0 = position;
        Ogre::Vector3 px = position + Ogre::Vector3(offset, 0, 0);
        Ogre::Vector3 pz = position + Ogre::Vector3(0, 0, offset);

        terra->getHeightAt(p0);
        terra->getHeightAt(px);
        terra->getHeightAt(pz);

        // Cross product to get normal
        Ogre::Vector3 tangentX = px - p0;
        Ogre::Vector3 tangentZ = pz - p0;
        Ogre::Vector3 normal = tangentX.crossProduct(tangentZ);
        normal.normalise();

        // Ensure normal points UP!
        if (normal.y < 0.0f)
        {
            normal = -normal; // Flip to point upward
        }

        return normal;
    }

    // ============================================================================
    // CREATE ITEMS ON RENDER THREAD
    // ============================================================================

    void ProceduralFoliageVolumeComponent::createFoliageOnRenderThread(std::vector<VegetationBatch>&& batches)
    {
        // Render thread: create items and nodes only
        GraphicsModule::getInstance()->enqueueAndWait(
            [this, batchesCopy = std::move(batches)]() mutable
            {
                this->createFoliageItems(batchesCopy);
            },
            "ProceduralFoliageVolume::CreateItems");

        // Physics (also main thread)
        this->createFoliageCollision();
    }

    void ProceduralFoliageVolumeComponent::createFoliageItems(std::vector<VegetationBatch>& batches)
    {
        // ============================================================================
        // CELL-BASED SPATIAL GROUPING
        //
        // Root cause of 12fps: fillBuffersForV2 is called every frame for every
        // visible Ogre::Item, computing a full concatenateAffine(viewMatrix, worldMat)
        // per item. With 19,374 bush Items that is ~300ms of CPU matrix math per frame
        // regardless of SCENE_STATIC, GPU instancing, or draw call count.
        //
        // Solution: divide the volume into NxN spatial cells. For each (rule x cell)
        // pair, pre-transform all instance vertices into world space on the CPU (once,
        // at generation time) and store them in one BT_IMMUTABLE merged Item.
        //
        // The merged Item has identity world matrix -> fillBuffersForV2 cost is O(1).
        // Vertices are already in world space so no per-frame transform is needed.
        // Each cell has a tight AABB -> frustum culling still works at cell granularity.
        //
        // With a 100x100m terrain and 10m cells: 100 cells x 4 rules = 400 Items max.
        // fillBuffersForV2 cost: 400 calls/frame instead of 28,045. ~70x improvement.
        // ============================================================================

        Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();
        Ogre::VaoManager* vaoManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getVaoManager();

        // Flush all pending immutable buffer uploads BEFORE reading any source mesh
        // vertex data. Without this, reading source mesh VAOs (e.g. acacia1.mesh,
        // which was just loaded as BT_IMMUTABLE) triggers a createAsyncTicket on an
        // unfinished upload -- one PERFORMANCE WARNING per rule batch.
        if (nullptr != vaoManager)
        {
            vaoManager->_update();
        }

        // Cell size in world units. Smaller = tighter culling but more Items.
        // 10m gives good balance for typical foliage scenes.
        const float cellSize = 10.0f;

        for (auto& batch : batches)
        {
            const FoliageRule& rule = this->rules[batch.ruleIndex];

            if (true == batch.instances.empty())
            {
                continue;
            }

            // Load source mesh to read vertex/index data from.
            Ogre::MeshPtr srcMesh;
            try
            {
                srcMesh = Ogre::MeshManager::getSingletonPtr()->load(batch.meshName, rule.resourceGroup);
            }
            catch (const Ogre::Exception& e)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralFoliageVolume] Failed to load mesh '" + batch.meshName + "': " + e.getDescription());
                continue;
            }

            if (srcMesh.isNull() || srcMesh->getNumSubMeshes() == 0u)
            {
                continue;
            }

            // ----------------------------------------------------------------
            // Read source mesh vertex/index data from the first submesh.
            // Using the readRequests / mapAsyncTickets pattern (MeshModifyComponent).
            // ----------------------------------------------------------------
            struct SrcData
            {
                std::vector<Ogre::Vector3> positions;
                std::vector<Ogre::Vector3> normals;
                std::vector<Ogre::Vector4> tangents;
                std::vector<Ogre::Vector2> uvs;
                std::vector<Ogre::uint32> indices;
                bool hasTangent;
                Ogre::HlmsDatablock* datablock;
                bool hasTransparency;
            };

            SrcData sd;
            sd.hasTangent = false;
            sd.datablock = nullptr;
            sd.hasTransparency = false;

            Ogre::SubMesh* sm0 = srcMesh->getSubMesh(0u);
            if (sm0->mVao[Ogre::VpNormal].empty() || sm0->mVao[Ogre::VpNormal][0]->getVertexBuffers().empty())
            {
                continue;
            }

            Ogre::VertexArrayObject* srcVao = sm0->mVao[Ogre::VpNormal][0];
            const size_t srcVC = srcVao->getVertexBuffers()[0]->getNumElements();
            const size_t srcIC = srcVao->getIndexBuffer() ? srcVao->getIndexBuffer()->getNumElements() : 0u;

            if (srcVC == 0u)
            {
                continue;
            }

            sd.positions.resize(srcVC);
            sd.normals.resize(srcVC, Ogre::Vector3::UNIT_Y);
            sd.tangents.resize(srcVC, Ogre::Vector4(1.0f, 0.0f, 0.0f, 1.0f));
            sd.uvs.resize(srcVC, Ogre::Vector2::ZERO);

            // Check for tangent element
            for (Ogre::VertexBufferPacked* vbp : srcVao->getVertexBuffers())
            {
                for (const Ogre::VertexElement2& e : vbp->getVertexElements())
                {
                    if (e.mSemantic == Ogre::VES_TANGENT)
                    {
                        sd.hasTangent = true;
                    }
                }
            }

            // Read vertex data
            {
                Ogre::VertexArrayObject::ReadRequestsVec requests;
                requests.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_POSITION));
                requests.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_NORMAL));
                requests.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_TEXTURE_COORDINATES));
                if (sd.hasTangent)
                {
                    requests.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_TANGENT));
                }

                srcVao->readRequests(requests);
                srcVao->mapAsyncTickets(requests);

                const bool isQTan = (requests[1].type == Ogre::VET_SHORT4_SNORM);

                for (size_t vi = 0u; vi < srcVC; ++vi)
                {
                    // Position
                    if (requests[0].type == Ogre::VET_HALF4)
                    {
                        const Ogre::uint16* p = reinterpret_cast<const Ogre::uint16*>(requests[0].data);
                        sd.positions[vi].x = Ogre::Bitwise::halfToFloat(p[0]);
                        sd.positions[vi].y = Ogre::Bitwise::halfToFloat(p[1]);
                        sd.positions[vi].z = Ogre::Bitwise::halfToFloat(p[2]);
                    }
                    else
                    {
                        const float* p = reinterpret_cast<const float*>(requests[0].data);
                        sd.positions[vi] = Ogre::Vector3(p[0], p[1], p[2]);
                    }

                    // Normal / QTangent
                    if (isQTan)
                    {
                        const Ogre::int16* q = reinterpret_cast<const Ogre::int16*>(requests[1].data);
                        Ogre::Quaternion qt;
                        qt.x = q[0] / 32767.0f;
                        qt.y = q[1] / 32767.0f;
                        qt.z = q[2] / 32767.0f;
                        qt.w = q[3] / 32767.0f;
                        const float refl = (qt.w < 0.0f) ? -1.0f : 1.0f;
                        sd.normals[vi] = qt.xAxis();
                        sd.tangents[vi] = Ogre::Vector4(qt.yAxis().x, qt.yAxis().y, qt.yAxis().z, refl);
                        sd.hasTangent = true;
                    }
                    else if (requests[1].type == Ogre::VET_HALF4)
                    {
                        const Ogre::uint16* n = reinterpret_cast<const Ogre::uint16*>(requests[1].data);
                        sd.normals[vi].x = Ogre::Bitwise::halfToFloat(n[0]);
                        sd.normals[vi].y = Ogre::Bitwise::halfToFloat(n[1]);
                        sd.normals[vi].z = Ogre::Bitwise::halfToFloat(n[2]);
                    }
                    else
                    {
                        const float* n = reinterpret_cast<const float*>(requests[1].data);
                        sd.normals[vi] = Ogre::Vector3(n[0], n[1], n[2]);
                    }

                    // UV
                    if (requests[2].type == Ogre::VET_HALF2)
                    {
                        const Ogre::uint16* uv = reinterpret_cast<const Ogre::uint16*>(requests[2].data);
                        sd.uvs[vi].x = Ogre::Bitwise::halfToFloat(uv[0]);
                        sd.uvs[vi].y = Ogre::Bitwise::halfToFloat(uv[1]);
                    }
                    else
                    {
                        const float* uv = reinterpret_cast<const float*>(requests[2].data);
                        sd.uvs[vi] = Ogre::Vector2(uv[0], uv[1]);
                    }

                    // Explicit tangent
                    if (!isQTan && sd.hasTangent && requests.size() >= 4u)
                    {
                        if (requests[3].type == Ogre::VET_FLOAT4)
                        {
                            const float* t = reinterpret_cast<const float*>(requests[3].data);
                            sd.tangents[vi] = Ogre::Vector4(t[0], t[1], t[2], t[3]);
                        }
                    }

                    // Advance pointers
                    requests[0].data += requests[0].vertexBuffer->getBytesPerElement();
                    requests[1].data += requests[1].vertexBuffer->getBytesPerElement();
                    requests[2].data += requests[2].vertexBuffer->getBytesPerElement();
                    if (!isQTan && sd.hasTangent && requests.size() >= 4u)
                    {
                        requests[3].data += requests[3].vertexBuffer->getBytesPerElement();
                    }
                }

                srcVao->unmapAsyncTickets(requests);
            }

            // Read index data
            if (srcIC > 0u && srcVao->getIndexBuffer())
            {
                Ogre::IndexBufferPacked* ibp = srcVao->getIndexBuffer();
                sd.indices.resize(srcIC);
                const void* shadow = ibp->getShadowCopy();
                if (nullptr != shadow)
                {
                    if (ibp->getIndexType() == Ogre::IndexBufferPacked::IT_32BIT)
                    {
                        const Ogre::uint32* s = reinterpret_cast<const Ogre::uint32*>(shadow);
                        for (size_t ii = 0u; ii < srcIC; ++ii)
                        {
                            sd.indices[ii] = s[ii];
                        }
                    }
                    else
                    {
                        const Ogre::uint16* s = reinterpret_cast<const Ogre::uint16*>(shadow);
                        for (size_t ii = 0u; ii < srcIC; ++ii)
                        {
                            sd.indices[ii] = static_cast<Ogre::uint32>(s[ii]);
                        }
                    }
                }
                else
                {
                    Ogre::AsyncTicketPtr ticket = ibp->readRequest(0u, srcIC);
                    const void* raw = ticket->map();
                    if (ibp->getIndexType() == Ogre::IndexBufferPacked::IT_32BIT)
                    {
                        const Ogre::uint32* s = reinterpret_cast<const Ogre::uint32*>(raw);
                        for (size_t ii = 0u; ii < srcIC; ++ii)
                        {
                            sd.indices[ii] = s[ii];
                        }
                    }
                    else
                    {
                        const Ogre::uint16* s = reinterpret_cast<const Ogre::uint16*>(raw);
                        for (size_t ii = 0u; ii < srcIC; ++ii)
                        {
                            sd.indices[ii] = static_cast<Ogre::uint32>(s[ii]);
                        }
                    }
                    ticket->unmap();
                }
            }

            // Get datablock from a proxy item
            {
                Ogre::Item* proxy = sceneManager->createItem(srcMesh, Ogre::SCENE_DYNAMIC);
                if (proxy->getNumSubItems() > 0u)
                {
                    sd.datablock = proxy->getSubItem(0u)->getDatablock();
                    if (nullptr != sd.datablock)
                    {
                        const Ogre::HlmsBlendblock* bb = sd.datablock->getBlendblock(false);
                        if (nullptr != bb && (bb->isAutoTransparent() || bb->isForcedTransparent()))
                        {
                            sd.hasTransparency = true;
                        }
                    }
                }
                sceneManager->destroyItem(proxy);
            }

            // ----------------------------------------------------------------
            // Group instances into spatial cells and build one merged Item per cell.
            // ----------------------------------------------------------------

            // Map from cell coordinate to list of instance indices in that cell.
            std::unordered_map<uint64_t, std::vector<size_t>> cellMap;
            cellMap.reserve(256u);

            for (size_t instIdx = 0u; instIdx < batch.instances.size(); ++instIdx)
            {
                const Ogre::Vector3& pos = batch.instances[instIdx].position;
                const int32_t cx = static_cast<int32_t>(std::floor(pos.x / cellSize));
                const int32_t cz = static_cast<int32_t>(std::floor(pos.z / cellSize));
                const uint64_t key = (static_cast<uint64_t>(static_cast<uint32_t>(cx)) << 32) | static_cast<uint32_t>(cz);
                cellMap[key].push_back(instIdx);
            }

            const Ogre::uint8 renderQueue = sd.hasTransparency ? NOWA::RENDER_QUEUE_V2_TRANSPARENT : NOWA::RENDER_QUEUE_V2_MESH;
            const size_t fpv = sd.hasTangent ? 12u : 8u;

            size_t cellIndex = 0u;
            for (auto& cellEntry : cellMap)
            {
                const std::vector<size_t>& cellInstances = cellEntry.second;
                const size_t cellInstCount = cellInstances.size();

                const size_t mergedVC = srcVC * cellInstCount;
                const size_t mergedIC = srcIC * cellInstCount;

                // Allocate merged vertex buffer
                float* vd = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(mergedVC * fpv * sizeof(float), Ogre::MEMCATEGORY_GEOMETRY));

                Ogre::Vector3 cellMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
                Ogre::Vector3 cellMax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());

                for (size_t ci = 0u; ci < cellInstCount; ++ci)
                {
                    const VegetationInstance& inst = batch.instances[cellInstances[ci]];

                    Ogre::Matrix3 rotMat;
                    inst.orientation.ToRotationMatrix(rotMat);

                    const Ogre::Vector3& sc = inst.scale;
                    const Ogre::Matrix3 srMat(rotMat[0][0] * sc.x, rotMat[0][1] * sc.y, rotMat[0][2] * sc.z, rotMat[1][0] * sc.x, rotMat[1][1] * sc.y, rotMat[1][2] * sc.z, rotMat[2][0] * sc.x, rotMat[2][1] * sc.y, rotMat[2][2] * sc.z);

                    const size_t vBase = ci * srcVC;
                    for (size_t vi = 0u; vi < srcVC; ++vi)
                    {
                        const Ogre::Vector3 wPos = inst.position + srMat * sd.positions[vi];
                        const Ogre::Vector3 wNrm = (rotMat * sd.normals[vi]).normalisedCopy();
                        const Ogre::Vector3 wTan = rotMat * Ogre::Vector3(sd.tangents[vi].x, sd.tangents[vi].y, sd.tangents[vi].z);

                        cellMin.makeFloor(wPos);
                        cellMax.makeCeil(wPos);

                        const size_t o = (vBase + vi) * fpv;
                        vd[o + 0] = wPos.x;
                        vd[o + 1] = wPos.y;
                        vd[o + 2] = wPos.z;
                        vd[o + 3] = wNrm.x;
                        vd[o + 4] = wNrm.y;
                        vd[o + 5] = wNrm.z;
                        if (sd.hasTangent)
                        {
                            vd[o + 6] = wTan.x;
                            vd[o + 7] = wTan.y;
                            vd[o + 8] = wTan.z;
                            vd[o + 9] = sd.tangents[vi].w;
                            vd[o + 10] = sd.uvs[vi].x;
                            vd[o + 11] = sd.uvs[vi].y;
                        }
                        else
                        {
                            vd[o + 6] = sd.uvs[vi].x;
                            vd[o + 7] = sd.uvs[vi].y;
                        }
                    }
                }

                // Merged index buffer
                Ogre::uint32* id = nullptr;
                if (mergedIC > 0u)
                {
                    id = reinterpret_cast<Ogre::uint32*>(OGRE_MALLOC_SIMD(mergedIC * sizeof(Ogre::uint32), Ogre::MEMCATEGORY_GEOMETRY));
                    for (size_t ci = 0u; ci < cellInstCount; ++ci)
                    {
                        const Ogre::uint32 voff = static_cast<Ogre::uint32>(ci * srcVC);
                        for (size_t ii = 0u; ii < srcIC; ++ii)
                        {
                            id[ci * srcIC + ii] = sd.indices[ii] + voff;
                        }
                    }
                }

                // Build VAO
                Ogre::VertexElement2Vec elems;
                elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
                elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));
                if (sd.hasTangent)
                {
                    elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT4, Ogre::VES_TANGENT));
                }
                elems.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));

                Ogre::VertexBufferPacked* vb = nullptr;
                try
                {
                    vb = vaoManager->createVertexBuffer(elems, mergedVC, Ogre::BT_IMMUTABLE, vd, true);
                }
                catch (const Ogre::Exception& e)
                {
                    OGRE_FREE_SIMD(vd, Ogre::MEMCATEGORY_GEOMETRY);
                    if (id)
                    {
                        OGRE_FREE_SIMD(id, Ogre::MEMCATEGORY_GEOMETRY);
                    }
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                        "[ProceduralFoliageVolume] createVertexBuffer failed for '" + rule.name + "' cell " + Ogre::StringConverter::toString(static_cast<unsigned int>(cellIndex)) + ": " + e.getDescription());
                    ++cellIndex;
                    continue;
                }

                Ogre::IndexBufferPacked* ib = nullptr;
                if (mergedIC > 0u && id)
                {
                    try
                    {
                        ib = vaoManager->createIndexBuffer(Ogre::IndexBufferPacked::IT_32BIT, mergedIC, Ogre::BT_IMMUTABLE, id, true);
                    }
                    catch (const Ogre::Exception& e)
                    {
                        OGRE_FREE_SIMD(id, Ogre::MEMCATEGORY_GEOMETRY);
                        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                            "[ProceduralFoliageVolume] createIndexBuffer failed for '" + rule.name + "' cell " + Ogre::StringConverter::toString(static_cast<unsigned int>(cellIndex)) + ": " + e.getDescription());
                        ++cellIndex;
                        continue;
                    }
                }

                Ogre::VertexBufferPackedVec vbVec;
                vbVec.push_back(vb);
                Ogre::VertexArrayObject* mergedVao = vaoManager->createVertexArrayObject(vbVec, ib, Ogre::OT_TRIANGLE_LIST);

                // Unique name per rule + cell
                const Ogre::String cellMeshName = "FoliageCell_" + rule.name + "_GO" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()) + "_B" + Ogre::StringConverter::toString(batch.ruleIndex) + "_C" +
                                                  Ogre::StringConverter::toString(static_cast<unsigned int>(cellIndex));

                {
                    Ogre::ResourcePtr existing = Ogre::MeshManager::getSingleton().getByName(cellMeshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
                    if (false == existing.isNull())
                    {
                        Ogre::MeshManager::getSingleton().remove(existing->getHandle());
                    }
                }

                Ogre::MeshPtr cellMesh = Ogre::MeshManager::getSingleton().createManual(cellMeshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
                cellMesh->_setVaoManager(vaoManager);

                Ogre::SubMesh* cellSM = cellMesh->createSubMesh();
                cellSM->mVao[Ogre::VpNormal].push_back(mergedVao);
                cellSM->mVao[Ogre::VpShadow].push_back(mergedVao);

                if (nullptr != sd.datablock && sd.datablock->getNameStr())
                {
                    cellSM->mMaterialName = *sd.datablock->getNameStr();
                }

                if (cellMin.x > cellMax.x)
                {
                    cellMin = cellMax = Ogre::Vector3::ZERO;
                }
                Ogre::Aabb cellAabb;
                cellAabb.setExtents(cellMin, cellMax);
                cellMesh->_setBounds(cellAabb, false);
                cellMesh->_setBoundingSphereRadius(cellAabb.getRadius());

                if (false == cellMesh->hasValidShadowMappingVaos())
                {
                    cellMesh->prepareForShadowMapping(true);
                }

                // One Item + one SceneNode at world origin per cell
                // (vertices already in world space, so identity transform is correct)
                Ogre::Item* cellItem = sceneManager->createItem(cellMesh, Ogre::SCENE_STATIC);
                cellItem->setName("FoliageCellItem_" + rule.name + "_C" + Ogre::StringConverter::toString(static_cast<unsigned int>(cellIndex)));

                if (nullptr != sd.datablock && cellItem->getNumSubItems() > 0u)
                {
                    cellItem->getSubItem(0u)->setDatablock(sd.datablock);
                }

                cellItem->setRenderQueueGroup(renderQueue);

                // Shadow casting is disabled for merged cell Items intentionally.
                //
                // Reason 1 - Correctness: each cell contains dozens of pre-transformed
                // instances merged into one mesh. The cell AABB covers a 10x10m area,
                // so Ogre submits the ENTIRE cell to the shadow pass even when only a
                // fraction of it would cast shadows onto visible geometry. This makes
                // the shadow pass render the same 10000+ triangles per cell that the
                // main pass does -- doubling the GPU cost for no visual benefit, since
                // merged-cell shadow silhouettes look identical to per-instance ones.
                //
                // Reason 2 - Performance: Colour Pass #7 (shadow atlas) cost 15ms in
                // profiling, almost entirely from shadow geometry. Disabling foliage
                // cell shadows brings this to ~2ms.
                //
                // If per-foliage-instance shadows are needed in the future, the right
                // approach is a dedicated shadow imposter pass or per-instance Items
                // with a short shadowRenderingDistance (e.g. 10m from camera).
                cellItem->setCastShadows(false);
                // setShadowRenderingDistance not needed since shadow casting is disabled.

                Ogre::SceneNode* cellNode = sceneManager->getRootSceneNode(Ogre::SCENE_STATIC)->createChildSceneNode(Ogre::SCENE_STATIC);
                cellNode->setPosition(Ogre::Vector3::ZERO);
                cellNode->setOrientation(Ogre::Quaternion::IDENTITY);
                cellNode->setScale(Ogre::Vector3::UNIT_SCALE);
                cellNode->attachObject(cellItem);

                batch.items.push_back(cellItem);
                batch.nodes.push_back(cellNode);

                ++cellIndex;
            }

            batch.sharedDatablock = sd.datablock;

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralFoliageVolume] Rule '" + rule.name + "': " + Ogre::StringConverter::toString(static_cast<unsigned int>(batch.instances.size())) + " instances -> " +
                                                                                    Ogre::StringConverter::toString(static_cast<unsigned int>(cellIndex)) + " cell Items" + " (cellSize=" + Ogre::StringConverter::toString(cellSize) + "m)" +
                                                                                    " RQ=" + Ogre::StringConverter::toString(static_cast<int>(renderQueue)));
        }

        this->vegetationBatches = std::move(batches);

        sceneManager->notifyStaticDirty(sceneManager->getRootSceneNode(Ogre::SCENE_STATIC));

        // Flush pending immutable buffer uploads so vaoName is valid for the first frame.
        Ogre::VaoManager* vaoManager2 = Ogre::Root::getSingletonPtr()->getRenderSystem()->getVaoManager();
        if (nullptr != vaoManager2)
        {
            vaoManager2->_update();
        }
    }

    void ProceduralFoliageVolumeComponent::createFoliageCollision()
    {
        if (nullptr == this->physicsArtifactComponent)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralFoliageVolume] No PhysicsArtifactComponent found, skipping collision creation");
            return;
        }

        // Check if any rule has collision enabled
        bool anyCollisionEnabled = false;
        for (const auto& rule : this->rules)
        {
            if (rule.collisionEnabled)
            {
                anyCollisionEnabled = true;
                break;
            }
        }

        if (!anyCollisionEnabled)
        {
            return;
        }

        auto start = std::chrono::high_resolution_clock::now();

        std::vector<OgreNewt::CollisionPtr> collisions;
        collisions.reserve(10000);

        OgreNewt::World* world = AppStateManager::getSingletonPtr()->getOgreNewtModule()->getOgreNewt();

        for (const auto& batch : this->vegetationBatches)
        {
            const FoliageRule& rule = this->rules[batch.ruleIndex];

            if (!rule.collisionEnabled)
            {
                continue;
            }

            // Rotate 90° around Z: maps Newton's X-axis -> world Y-axis (upright)
            Ogre::Quaternion uprightFix(Ogre::Degree(90), Ogre::Vector3::UNIT_Z);

            // The compound body will be placed at this position in the world
            Ogre::Vector3 bodyOrigin = this->gameObjectPtr->getPosition();

            for (const VegetationInstance& instance : batch.instances)
            {
                Ogre::Quaternion correctedOrientation = instance.orientation * uprightFix;

                // Convert world position -> body-local position
                Ogre::Vector3 localPosition = instance.position - bodyOrigin;

                // Then shift center along the tree's own up axis
                localPosition += instance.orientation * Ogre::Vector3(0.0f, rule.collisionHeight * 0.5f, 0.0f);

                OgreNewt::CollisionPtr cylinderCol = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::Cylinder(world, rule.collisionRadius, rule.collisionHeight, this->gameObjectPtr->getCategoryId(), correctedOrientation, localPosition));

                if (cylinderCol)
                {
                    collisions.push_back(cylinderCol);
                }
            }
        }

        if (collisions.empty())
        {
            return;
        }

        auto finish = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = finish - start;

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
            "[ProceduralFoliageVolume] Created " + Ogre::StringConverter::toString(collisions.size()) + " collision cylinders in " + Ogre::StringConverter::toString(elapsed.count() * 0.001) + "s");

        // Create compound body with serialization
        Ogre::String collisionName = "FoliageCompound_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());

        this->physicsArtifactComponent->createCompoundBody(collisions, collisionName);
    }

    void ProceduralFoliageVolumeComponent::generateLODForMesh(const Ogre::String& meshName, Ogre::Real lodDistance, const Ogre::String& resourceGroup)
    {
        // Check if the V2 mesh already has LOD before regenerating!
        Ogre::MeshPtr v2MeshCheck = Ogre::MeshManager::getSingletonPtr()->getByName(meshName, resourceGroup);

        if (v2MeshCheck && v2MeshCheck->getNumLodLevels() > 1)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralFoliageVolume] Mesh already has LOD, skipping: " + meshName);
            return; // Already has LOD, don't regenerate!
        }

        // Load V1 mesh
        Ogre::v1::MeshPtr v1Mesh = Ogre::v1::MeshManager::getSingletonPtr()->load(meshName, resourceGroup, Ogre::v1::HardwareBuffer::HBU_STATIC_WRITE_ONLY, Ogre::v1::HardwareBuffer::HBU_STATIC_WRITE_ONLY, true, true);

        if (!v1Mesh)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralFoliageVolume] Could not load V1 mesh for LOD: " + meshName);
            return;
        }

        // Generate LOD config
        Ogre::LodConfig lodConfig;
        Ogre::MeshLodGenerator lodGenerator;
        lodGenerator.getAutoconfig(v1Mesh, lodConfig);

        if (lodConfig.levels.size() <= 1)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralFoliageVolume] Mesh too simple for LOD: " + meshName);
            v1Mesh->unload();
            return;
        }

        lodConfig.strategy = Ogre::LodStrategyManager::getSingleton().getDefaultStrategy();

        // Space LOD levels evenly from lodDistance/N up to lodDistance.
        // Level 0 = highest detail, transitions at nearest distance.
        // Last level = lowest detail, transitions at lodDistance.
        const size_t numLevels = lodConfig.levels.size();
        for (size_t i = 0; i < numLevels; ++i)
        {
            // t goes from 1/N (first level) to 1.0 (last level)
            Ogre::Real t = static_cast<Ogre::Real>(i + 1) / static_cast<Ogre::Real>(numLevels);
            Ogre::Real dist = lodDistance * t;
            lodConfig.levels[i].distance = lodConfig.strategy->transformUserValue(dist);
        }

        // Generate LOD
        lodGenerator.generateLodLevels(lodConfig);

        // CRITICAL: Remove existing V2 mesh BEFORE re-importing!
        if (v2MeshCheck)
        {
            Ogre::MeshManager::getSingletonPtr()->remove(v2MeshCheck->getHandle());
        }

        // Import V1->V2 with LOD
        Ogre::MeshPtr v2Mesh = Ogre::MeshManager::getSingletonPtr()->createByImportingV1(meshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, v1Mesh.get(), true, true, true);

        v1Mesh->unload();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralFoliageVolume] Generated LOD for mesh: " + meshName + " (" + Ogre::StringConverter::toString(lodConfig.levels.size()) + " levels)");
    }

    // ============================================================================
    // CLEANUP
    // ============================================================================

    void ProceduralFoliageVolumeComponent::clearFoliage()
    {
        GraphicsModule::getInstance()->enqueueAndWait(
            [this]()
            {
                this->destroyFoliageOnRenderThread();
            },
            "ProceduralFoliageVolume::DestroyItems");

        if (this->physicsArtifactComponent && this->physicsArtifactComponent->getBody())
        {
            this->physicsArtifactComponent->destroyBody();
        }
    }

    void ProceduralFoliageVolumeComponent::destroyFoliageOnRenderThread()
    {
        //  RUNS ON RENDER THREAD!

        Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();

        for (auto& batch : this->vegetationBatches)
        {
            for (size_t i = 0; i < batch.items.size(); i++)
            {
                if (batch.nodes[i])
                {
                    batch.nodes[i]->detachAllObjects();
                    NOWA::GraphicsModule::getInstance()->removeTrackedNode(batch.nodes[i]);
                    sceneManager->destroySceneNode(batch.nodes[i]);
                    batch.nodes[i] = nullptr;
                }

                if (batch.items[i])
                {
                    // Remove the cell mesh from MeshManager before destroying the Item
                    // so subsequent Regenerate can create fresh meshes under the same names.
                    Ogre::MeshPtr meshToRemove = batch.items[i]->getMesh();
                    sceneManager->destroyItem(batch.items[i]);
                    batch.items[i] = nullptr;

                    if (false == meshToRemove.isNull())
                    {
                        const Ogre::String& mName = meshToRemove->getName();
                        if (mName.find("FoliageCell_") != Ogre::String::npos)
                        {
                            Ogre::MeshManager::getSingleton().remove(meshToRemove->getHandle());
                        }
                    }
                }
            }

            batch.items.clear();
            batch.nodes.clear();
        }

        this->vegetationBatches.clear();

        sceneManager->notifyStaticDirty(sceneManager->getRootSceneNode(Ogre::SCENE_STATIC));
    }

    // ============================================================================
    // SETTERS/GETTERS
    // ============================================================================

    void ProceduralFoliageVolumeComponent::setVolumeBounds(const Ogre::Aabb& bounds)
    {
        // Store as Vector4: (min.x, min.z, max.x, max.z)
        Ogre::Vector3 minCorner = bounds.getMinimum();
        Ogre::Vector3 maxCorner = bounds.getMaximum();
        this->volumeBounds->setValue(Ogre::Vector4(minCorner.x, minCorner.z, maxCorner.x, maxCorner.z));
    }

    Ogre::Aabb ProceduralFoliageVolumeComponent::getVolumeBounds(void) const
    {
        Ogre::Vector4 v = this->volumeBounds->getVector4();
        return Ogre::Aabb::newFromExtents(Ogre::Vector3(v.x, -10000.0f, v.y), Ogre::Vector3(v.z, 10000.0f, v.w));
    }

    void ProceduralFoliageVolumeComponent::setMasterSeed(Ogre::uint32 seed)
    {
        this->masterSeed->setValue(seed);
    }

    Ogre::uint32 ProceduralFoliageVolumeComponent::getMasterSeed(void) const
    {
        return this->masterSeed->getUInt();
    }

    void ProceduralFoliageVolumeComponent::setGridResolution(Ogre::Real resolution)
    {
        this->gridResolution->setValue(Ogre::Math::Clamp(resolution, 0.1f, 10.0f));
    }

    Ogre::Real ProceduralFoliageVolumeComponent::getGridResolution(void) const
    {
        return this->gridResolution->getReal();
    }

    void ProceduralFoliageVolumeComponent::setRuleCount(unsigned int count)
    {
        this->ruleCount->setValue(count);

        size_t oldSize = this->rules.size();

        if (count > oldSize)
        {
            // Expand arrays
            this->rules.resize(count);
            this->ruleNames.resize(count);
            this->ruleMeshNames.resize(count);
            this->ruleDensities.resize(count);
            this->ruleHeightRanges.resize(count);
            this->ruleMaxSlopes.resize(count);
            this->ruleTerraLayers.resize(count);
            this->ruleScaleRanges.resize(count);
            this->ruleMinSpacings.resize(count);
            this->ruleRenderDistances.resize(count);
            this->ruleLodDistances.resize(count);
            this->ruleCastShadows.resize(count);
            this->ruleShadowDistances.resize(count);
            this->ruleCategories.resize(count);
            this->ruleClearanceDistances.resize(count);
            this->ruleCollisionEnabled.resize(count);
            this->ruleCollisionRadius.resize(count);
            this->ruleCollisionHeight.resize(count);

            // Initialize new variants with SENSIBLE DEFAULTS
            for (size_t i = oldSize; i < count; i++)
            {
                // -----------------------------------------------------------------------
                // Density: trees need very sparse placement, ground cover needs denser.
                // Formula: stepX = resolution / density, so density = resolution / stepX.
                // With resolution=2: density=0.05 -> step=40m (very sparse, ~6 trees/100x100m)
                //                    density=0.10 -> step=20m (~25 shrubs/100x100m)
                //                    density=0.20 -> step=10m (~100 groundcover/100x100m)
                // The pattern scales by index: trees(0), shrubs(1), small shrubs(2), groundcover(3+)
                // -----------------------------------------------------------------------
                static const float densityByIndex[] = {0.05f, 0.10f, 1.0f, 2.0f, 3.0f};
                static const float minSpacingByIndex[] = {8.0f, 3.0f, 2.0f, 1.5f, 1.0f};
                static const float lodDistByIndex[] = {80.0f, 60.0f, 50.0f, 30.0f, 25.0f};
                static const float renderDistByIndex[] = {100.0f, 80.0f, 70.0f, 60.0f, 50.0f};
                static const float shadowDistByIndex[] = {60.0f, 40.0f, 30.0f, 0.0f, 0.0f};
                static const bool collisionByIndex[] = {true, false, false, false, false};
                static const bool castShadowsByIndex[] = {true, false, false, false, false};

                const size_t clampedIdx = std::min(i, static_cast<size_t>(4));

                const Ogre::Real defaultDensity = densityByIndex[clampedIdx];
                const Ogre::Real defaultMinSpacing = minSpacingByIndex[clampedIdx];
                const Ogre::Real defaultLodDist = lodDistByIndex[clampedIdx];
                const Ogre::Real defaultRenderDist = renderDistByIndex[clampedIdx];
                const Ogre::Real defaultShadowDist = shadowDistByIndex[clampedIdx];
                const bool defaultCollision = collisionByIndex[clampedIdx];
                const bool defaultCastShadows = castShadowsByIndex[clampedIdx];

                // Rule Name
                this->ruleNames[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleName() + Ogre::StringConverter::toString(i), Ogre::String("Vegetation_" + Ogre::StringConverter::toString(i)), this->attributes);
                this->ruleNames[i]->setDescription("Display name for this vegetation type.");

                // Rule Mesh Name
                this->ruleMeshNames[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleMeshName() + Ogre::StringConverter::toString(i), Ogre::String(), this->attributes);
                this->ruleMeshNames[i]->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
                this->ruleMeshNames[i]->setDescription("3D mesh file for this vegetation (.mesh).");

                // Rule Density
                this->ruleDensities[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleDensity() + Ogre::StringConverter::toString(i), defaultDensity, this->attributes);
                this->ruleDensities[i]->setDescription("Placement density. Trees: 0.05-0.10, Shrubs: 0.10-0.20, Ground cover: 0.20-0.50. "
                                                       "Step size = gridResolution / density, so lower = sparser.");
                this->ruleDensities[i]->setConstraints(0.01f, 10.0f);
                this->rules[i].density = defaultDensity;

                // Height Range — unlimited by default
                this->ruleHeightRanges[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleHeightRange() + Ogre::StringConverter::toString(i), Ogre::Vector2(-1000.0f, 1000.0f), this->attributes);
                this->ruleHeightRanges[i]->setDescription("Min/max elevation in meters. -1000/1000 = unlimited.");
                this->rules[i].heightRange = Ogre::Vector2(-1000.0f, 1000.0f);

                // Max Slope — permissive default (further narrowed by slopeRange below)
                this->ruleMaxSlopes[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleMaxSlope() + Ogre::StringConverter::toString(i), 45.0f, this->attributes);
                this->ruleMaxSlopes[i]->setDescription("Hard maximum slope angle in degrees. Acts as a fast rejection before slopeRange check.");
                this->ruleMaxSlopes[i]->setConstraints(0.0f, 90.0f);
                this->rules[i].maxSlope = 45.0f;

                // Terra Layers — all layers allowed
                this->ruleTerraLayers[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleTerraLayers() + Ogre::StringConverter::toString(i), Ogre::String("255,255,255,255"), this->attributes);
                this->ruleTerraLayers[i]->setDescription("Layer thresholds (0-255 per channel). 255,255,255,255 = all layers. "
                                                         "255,255,255,0 = layers 1-3 allowed, layer 4 blocked.");
                this->rules[i].terraLayerThresholds = this->parseTerraLayers("255,255,255,255");

                // Scale Range — slight variation around 1.0
                Ogre::Vector2 defaultScale = (i == 0) ? Ogre::Vector2(0.3f, 0.6f)  // trees: noticeable size variation
                                                      : Ogre::Vector2(0.2f, 0.7f); // bushes/ground cover: subtle variation
                this->ruleScaleRanges[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleScaleRange() + Ogre::StringConverter::toString(i), defaultScale, this->attributes);
                this->ruleScaleRanges[i]->setDescription("Min/max scale multiplier for size variation.");
                this->rules[i].scaleRange = defaultScale;

                // Min Spacing — prevents clustering within a rule
                this->ruleMinSpacings[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleMinSpacing() + Ogre::StringConverter::toString(i), defaultMinSpacing, this->attributes);
                this->ruleMinSpacings[i]->setDescription("Minimum distance between instances of this rule (meters). "
                                                         "Should be roughly 50-80% of the natural grid step (gridResolution / density).");
                this->ruleMinSpacings[i]->setConstraints(0.0f, 100.0f);
                this->rules[i].minDistanceToSame = defaultMinSpacing;

                // Render Distance
                this->ruleRenderDistances[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleRenderDistance() + Ogre::StringConverter::toString(i), defaultRenderDist, this->attributes);
                this->ruleRenderDistances[i]->setDescription("Visibility distance in meters. Trees: 400m, Bushes: 150-200m, Ground cover: 60-80m.");
                this->ruleRenderDistances[i]->setConstraints(0.0f, 10000.0f);
                this->rules[i].renderDistance = defaultRenderDist;

                // LOD Distance — enabled by default for all rules
                this->ruleLodDistances[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleLodDistance() + Ogre::StringConverter::toString(i), defaultLodDist, this->attributes);
                this->ruleLodDistances[i]->setDescription("Distance at which the lowest LOD is reached. 0 = no LOD (avoid for complex meshes). "
                                                          "Trees: 80m, Shrubs: 50-60m, Ground cover: 25-30m.");
                this->ruleLodDistances[i]->setConstraints(0.0f, 1000.0f);
                this->rules[i].lodDistance = defaultLodDist;

                // Cast Shadows - Should not be used for grass etc. only for trees
                this->ruleCastShadows[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleCastShadows() + Ogre::StringConverter::toString(i), defaultCastShadows, this->attributes);
                this->ruleCastShadows[i]->setDescription("Whether to cast shadows. Should not be used for grass etc. only for trees.");
                this->rules[i].castShadows = defaultCastShadows;

                // Shadow Distance
                this->ruleShadowDistances[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleShadowDistance() + Ogre::StringConverter::toString(i), defaultShadowDist, this->attributes);
                this->ruleShadowDistances[i]->setDescription("Distance til which shadows shall be rendered (if cast shadows is enabled).");
                this->ruleShadowDistances[i]->setConstraints(0.0f, 1000.0f);
                this->rules[i].shadowDistance = defaultShadowDist;

                // Categories
                this->ruleCategories[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleCategories() + Ogre::StringConverter::toString(i), Ogre::String("All"), this->attributes);
                this->ruleCategories[i]->setDescription("'All' = grow everywhere. 'All-Obstacle' = avoid objects in Obstacle category. "
                                                        "'Ground' = only on Ground category.");
                this->rules[i].categories = "All";
                this->rules[i].categoriesId = GameObjectController::ALL_CATEGORIES_ID;

                // Clearance Distance
                this->ruleClearanceDistances[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleClearanceDistance() + Ogre::StringConverter::toString(i), (i == 0) ? 5.0f : 1.0f, this->attributes); // trees need more clearance than bushes
                this->ruleClearanceDistances[i]->setDescription("Buffer zone around excluded objects (meters). Trees: 4-6m, Bushes: 1-2m, 0 = disabled.");
                this->ruleClearanceDistances[i]->setConstraints(0.0f, 50.0f);
                this->rules[i].clearanceDistance = (i == 0) ? 5.0f : 1.0f;

                // Collision Enabled — only for trees (index 0) by default
                this->ruleCollisionEnabled[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleCollisionEnabled() + Ogre::StringConverter::toString(i), defaultCollision, this->attributes);
                this->ruleCollisionEnabled[i]->setDescription("Physics cylinder collision. Enable only for trees/large obstacles. "
                                                              "Bushes and ground cover should have this off.");
                this->rules[i].collisionEnabled = defaultCollision;

                // Collision Radius
                Ogre::Real defaultColRadius = (i == 0) ? 0.3f : 0.15f;
                this->ruleCollisionRadius[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleCollisionRadius() + Ogre::StringConverter::toString(i), defaultColRadius, this->attributes);
                this->ruleCollisionRadius[i]->setDescription("Collision cylinder radius (trunk thickness). Trees: 0.2-0.5m, Bushes: 0.1-0.2m.");
                this->ruleCollisionRadius[i]->setConstraints(0.01f, 5.0f);
                this->rules[i].collisionRadius = defaultColRadius;

                // Collision Height
                Ogre::Real defaultColHeight = (i == 0) ? 4.0f : 1.0f;
                this->ruleCollisionHeight[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleCollisionHeight() + Ogre::StringConverter::toString(i), defaultColHeight, this->attributes);
                this->ruleCollisionHeight[i]->setDescription("Collision cylinder height. Trees: 3-5m (trunk height), Bushes: 0.5-1.5m.");
                this->ruleCollisionHeight[i]->setConstraints(0.1f, 20.0f);
                this->ruleCollisionHeight[i]->addUserData(GameObject::AttrActionSeparator());
                this->rules[i].collisionHeight = defaultColHeight;
            }
        }
        else if (count < oldSize)
        {
            // Shrink arrays
            this->eraseVariants(this->ruleNames, count);
            this->eraseVariants(this->ruleMeshNames, count);
            this->eraseVariants(this->ruleDensities, count);
            this->eraseVariants(this->ruleHeightRanges, count);
            this->eraseVariants(this->ruleMaxSlopes, count);
            this->eraseVariants(this->ruleTerraLayers, count);
            this->eraseVariants(this->ruleScaleRanges, count);
            this->eraseVariants(this->ruleMinSpacings, count);
            this->eraseVariants(this->ruleRenderDistances, count);
            this->eraseVariants(this->ruleLodDistances, count);
            this->eraseVariants(this->ruleCastShadows, count);
            this->eraseVariants(this->ruleShadowDistances, count);
            this->eraseVariants(this->ruleCategories, count);
            this->eraseVariants(this->ruleClearanceDistances, count);
            this->eraseVariants(this->ruleCollisionEnabled, count);
            this->eraseVariants(this->ruleCollisionRadius, count);
            this->eraseVariants(this->ruleCollisionHeight, count);

            this->rules.resize(count);
        }
    }

    unsigned int ProceduralFoliageVolumeComponent::getRuleCount(void) const
    {
        return this->ruleCount->getUInt();
    }

    void ProceduralFoliageVolumeComponent::setRuleName(unsigned int index, const Ogre::String& name)
    {
        if (index >= this->rules.size())
        {
            return;
        }
        this->ruleNames[index]->setValue(name);
        this->rules[index].name = name;
    }

    Ogre::String ProceduralFoliageVolumeComponent::getRuleName(unsigned int index) const
    {
        if (index >= this->rules.size())
        {
            return "";
        }
        return this->ruleNames[index]->getString();
    }

    void ProceduralFoliageVolumeComponent::setRuleMeshName(unsigned int index, const Ogre::String& meshName)
    {
        if (index >= this->rules.size())
        {
            return;
        }
        this->ruleMeshNames[index]->setValue(meshName);
        this->rules[index].meshName = meshName;

        // Extract resource group from saved folder path (if available) for faster load if resource group is known
        Ogre::String folderPath = this->ruleMeshNames[index]->getUserDataValue("PathToFolder");
        if (false == folderPath.empty())
        {
            this->rules[index].resourceGroup = Core::getSingletonPtr()->extractResourceGroupFromPath(folderPath);

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralFoliageVolume] Mesh '" + meshName + "' assigned to group: " + this->rules[index].resourceGroup);
        }
        else
        {
            // No path saved, use AUTODETECT
            this->rules[index].resourceGroup = Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME;
        }
    }

    Ogre::String ProceduralFoliageVolumeComponent::getRuleMeshName(unsigned int index) const
    {
        if (index >= this->rules.size())
        {
            return "";
        }
        return this->ruleMeshNames[index]->getString();
    }

    void ProceduralFoliageVolumeComponent::setRuleDensity(unsigned int index, Ogre::Real density)
    {
        if (index >= this->rules.size())
        {
            return;
        }
        density = Ogre::Math::Clamp(density, 0.01f, 10.0f);
        this->ruleDensities[index]->setValue(density);
        this->rules[index].density = density;
    }

    Ogre::Real ProceduralFoliageVolumeComponent::getRuleDensity(unsigned int index) const
    {
        if (index >= this->rules.size())
        {
            return 1.0f;
        }
        return this->ruleDensities[index]->getReal();
    }

    void ProceduralFoliageVolumeComponent::setRuleHeightRange(unsigned int index, const Ogre::Vector2& range)
    {
        if (index >= this->rules.size())
        {
            return;
        }
        this->ruleHeightRanges[index]->setValue(range);
        this->rules[index].heightRange = range;
    }

    Ogre::Vector2 ProceduralFoliageVolumeComponent::getRuleHeightRange(unsigned int index) const
    {
        if (index >= this->rules.size())
        {
            return Ogre::Vector2(-FLT_MAX, FLT_MAX);
        }
        return this->ruleHeightRanges[index]->getVector2();
    }

    void ProceduralFoliageVolumeComponent::setRuleMaxSlope(unsigned int index, Ogre::Real maxSlope)
    {
        if (index >= this->rules.size())
        {
            return;
        }
        maxSlope = Ogre::Math::Clamp(maxSlope, 0.0f, 90.0f);
        this->ruleMaxSlopes[index]->setValue(maxSlope);
        this->rules[index].maxSlope = maxSlope;
    }

    Ogre::Real ProceduralFoliageVolumeComponent::getRuleMaxSlope(unsigned int index) const
    {
        if (index >= this->rules.size())
        {
            return 90.0f;
        }
        return this->ruleMaxSlopes[index]->getReal();
    }

    void ProceduralFoliageVolumeComponent::setRuleTerraLayers(unsigned int index, const Ogre::String& layers)
    {
        if (index >= this->rules.size())
        {
            return;
        }
        this->ruleTerraLayers[index]->setValue(layers);
        this->rules[index].terraLayerThresholds = this->parseTerraLayers(layers);
    }

    Ogre::String ProceduralFoliageVolumeComponent::getRuleTerraLayers(unsigned int index) const
    {
        if (index >= this->rules.size())
        {
            return "255,255,255,255";
        }
        return this->ruleTerraLayers[index]->getString();
    }

    void ProceduralFoliageVolumeComponent::setRuleScaleRange(unsigned int index, const Ogre::Vector2& range)
    {
        if (index >= this->rules.size())
        {
            return;
        }
        this->ruleScaleRanges[index]->setValue(range);
        this->rules[index].scaleRange = range;
    }

    Ogre::Vector2 ProceduralFoliageVolumeComponent::getRuleScaleRange(unsigned int index) const
    {
        if (index >= this->rules.size())
        {
            return Ogre::Vector2(0.8f, 1.2f);
        }
        return this->ruleScaleRanges[index]->getVector2();
    }

    void ProceduralFoliageVolumeComponent::setRuleMinSpacing(unsigned int index, Ogre::Real spacing)
    {
        if (index >= this->rules.size())
        {
            return;
        }
        spacing = Ogre::Math::Clamp(spacing, 0.0f, 100.0f);
        this->ruleMinSpacings[index]->setValue(spacing);
        this->rules[index].minDistanceToSame = spacing;
    }

    Ogre::Real ProceduralFoliageVolumeComponent::getRuleMinSpacing(unsigned int index) const
    {
        if (index >= this->rules.size())
        {
            return 0.0f;
        }
        return this->ruleMinSpacings[index]->getReal();
    }

    void ProceduralFoliageVolumeComponent::setRuleRenderDistance(unsigned int index, Ogre::Real distance)
    {
        if (index >= this->rules.size())
        {
            return;
        }
        distance = Ogre::Math::Clamp(distance, 0.0f, 10000.0f);
        this->ruleRenderDistances[index]->setValue(distance);
        this->rules[index].renderDistance = distance;
    }

    Ogre::Real ProceduralFoliageVolumeComponent::getRuleRenderDistance(unsigned int index) const
    {
        if (index >= this->rules.size())
        {
            return 0.0f;
        }
        return this->ruleRenderDistances[index]->getReal();
    }

    void ProceduralFoliageVolumeComponent::setRuleLodDistance(unsigned int index, Ogre::Real distance)
    {
        if (index >= this->rules.size())
        {
            return;
        }
        distance = Ogre::Math::Clamp(distance, 0.0f, 1000.0f);
        this->ruleLodDistances[index]->setValue(distance);
        this->rules[index].lodDistance = distance;
    }

    Ogre::Real ProceduralFoliageVolumeComponent::getRuleLodDistance(unsigned int index) const
    {
        if (index >= this->rules.size())
        {
            return 0.0f;
        }
        return this->ruleLodDistances[index]->getReal();
    }

    void ProceduralFoliageVolumeComponent::setRuleCastShadows(unsigned int index, bool castShadows)
    {
        if (index >= this->rules.size())
        {
            return;
        }
        this->ruleCastShadows[index]->setValue(castShadows);
        this->rules[index].castShadows = castShadows;
    }

    bool ProceduralFoliageVolumeComponent::getRuleCastShadows(unsigned int index) const
    {
        if (index >= this->rules.size())
        {
            return 0.0f;
        }
        return this->ruleCastShadows[index]->getBool();
    }

    void ProceduralFoliageVolumeComponent::setRuleShadowDistance(unsigned int index, Ogre::Real shadowDistance)
    {
        if (index >= this->rules.size())
        {
            return;
        }
        shadowDistance = Ogre::Math::Clamp(shadowDistance, 0.0f, 1000.0f);
        this->ruleShadowDistances[index]->setValue(shadowDistance);
        this->rules[index].shadowDistance = shadowDistance;
    }

    Ogre::Real ProceduralFoliageVolumeComponent::getRuleShadowDistance(unsigned int index) const
    {
        if (index >= this->rules.size())
        {
            return 0.0f;
        }
        return this->ruleShadowDistances[index]->getReal();
    }

    void ProceduralFoliageVolumeComponent::setRuleCategories(unsigned int index, const Ogre::String& categories)
    {
        if (index >= this->rules.size())
        {
            return;
        }

        this->ruleCategories[index]->setValue(categories);
        this->rules[index].categories = categories;

        // Resolve category string -> bitmask ID (same as AreaOfInterestComponent)
        this->rules[index].categoriesId = AppStateManager::getSingletonPtr()->getGameObjectController()->generateCategoryId(categories);
    }

    Ogre::String ProceduralFoliageVolumeComponent::getRuleCategories(unsigned int index) const
    {
        if (index >= this->rules.size())
        {
            return "All";
        }
        return this->ruleCategories[index]->getString();
    }

    void ProceduralFoliageVolumeComponent::setRuleClearanceDistance(unsigned int index, Ogre::Real clearanceDistance)
    {
        if (index >= this->rules.size())
        {
            return;
        }
        clearanceDistance = Ogre::Math::Clamp(clearanceDistance, 0.0f, 50.0f);
        this->ruleClearanceDistances[index]->setValue(clearanceDistance);
        this->rules[index].clearanceDistance = clearanceDistance;
    }

    Ogre::Real ProceduralFoliageVolumeComponent::getRuleClearanceDistance(unsigned int index) const
    {
        if (index >= this->rules.size())
        {
            return 0.0f;
        }
        return this->ruleClearanceDistances[index]->getReal();
    }

    void ProceduralFoliageVolumeComponent::setRuleCollisionEnabled(unsigned int index, bool enabled)
    {
        if (index >= this->rules.size())
        {
            return;
        }
        this->ruleCollisionEnabled[index]->setValue(enabled);
        this->rules[index].collisionEnabled = enabled;
    }

    bool ProceduralFoliageVolumeComponent::getRuleCollisionEnabled(unsigned int index) const
    {
        if (index >= this->rules.size())
        {
            return 0.0f;
        }
        return this->ruleCollisionEnabled[index]->getBool();
    }

    void ProceduralFoliageVolumeComponent::setRuleCollisionRadius(unsigned int index, Ogre::Real radius)
    {
        if (index >= this->rules.size())
        {
            return;
        }
        radius = Ogre::Math::Clamp(radius, 0.01f, 5.0f);
        this->ruleCollisionRadius[index]->setValue(radius);
        this->rules[index].collisionRadius = radius;
    }

    Ogre::Real ProceduralFoliageVolumeComponent::getRuleCollisionRadius(unsigned int index) const
    {
        if (index >= this->rules.size())
        {
            return 0.0f;
        }
        return this->ruleCollisionRadius[index]->getReal();
    }

    void ProceduralFoliageVolumeComponent::setRuleCollisionHeight(unsigned int index, Ogre::Real height)
    {
        if (index >= this->rules.size())
        {
            return;
        }
        height = Ogre::Math::Clamp(height, 0.1f, 20.0f);
        this->ruleCollisionHeight[index]->setValue(height);
        this->rules[index].collisionHeight = height;
    }

    Ogre::Real ProceduralFoliageVolumeComponent::getRuleCollisionHeight(unsigned int index) const
    {
        if (index >= this->rules.size())
        {
            return 0.0f;
        }
        return this->ruleCollisionHeight[index]->getReal();
    }

    // Helper function
    std::vector<int> ProceduralFoliageVolumeComponent::parseTerraLayers(const Ogre::String& layersStr)
    {
        std::vector<int> layers;
        std::stringstream ss(layersStr.c_str());
        std::string token;

        while (std::getline(ss, token, ','))
        {
            int value = Ogre::StringConverter::parseInt(token);
            layers.push_back(Ogre::Math::Clamp(value, 0, 255));
        }

        // Ensure we have 4 values
        while (layers.size() < 4)
        {
            layers.push_back(255);
        }

        return layers;
    }

    bool ProceduralFoliageVolumeComponent::isWithinTerraBounds(const Ogre::Vector3& position, Ogre::Terra* terra) const
    {
        if (!terra)
        {
            return false;
        }

        // Get Terra's actual bounds using the API that EXISTS
        const Ogre::Vector3 terrainOrigin = terra->getTerrainOrigin(); // e.g., (-50, -25, -50)
        const Ogre::Vector2 xzDimensions = terra->getXZDimensions();   // e.g., (100, 100)

        // Calculate Terra's world-space bounds
        const Ogre::Real minX = terrainOrigin.x;
        const Ogre::Real maxX = terrainOrigin.x + xzDimensions.x;
        const Ogre::Real minZ = terrainOrigin.z;
        const Ogre::Real maxZ = terrainOrigin.z + xzDimensions.y;

        // Check bounds with margin to avoid edge cases
        const Ogre::Real margin = 1.0f;
        return (position.x >= minX + margin && position.x <= maxX - margin && position.z >= minZ + margin && position.z <= maxZ - margin);
    }

    bool ProceduralFoliageVolumeComponent::isCategoryAllowed(const Ogre::Vector3& position, const FoliageRule& rule)
    {
        if (!this->raySceneQuery)
        {
            return true;
        }

        // Pure "All" — no filtering needed
        if (rule.categoriesId == GameObjectController::ALL_CATEGORIES_ID)
        {
            return true;
        }

        const bool isExcludeMode = (rule.categories.find("All") != Ogre::String::npos && rule.categories.find('-') != Ogre::String::npos);

        unsigned int queryMask;
        bool allowIfHit;

        if (isExcludeMode)
        {
            // Cast against only the excluded bits — hit means blocked
            queryMask = ~rule.categoriesId; // e.g. "All-Obstacle" -> 0x00000002 = only Obstacle
            allowIfHit = false;

            // Sphere clearance check first (cheaper than raycast)
            if (rule.clearanceDistance > 0.0f && this->sphereSceneQuery)
            {
                this->sphereSceneQuery->setSphere(Ogre::Sphere(position, rule.clearanceDistance));
                this->sphereSceneQuery->setQueryMask(queryMask);
                Ogre::SceneQueryResult& result = this->sphereSceneQuery->execute();

                for (Ogre::MovableObject* mo : result.movables)
                {
                    // Cameras happen to share the "Default" category bit which equals
                    // the Obstacle bit in scenes where Default was registered first.
                    // Skip them -- a camera is never a real obstacle.
                    if (mo->getMovableType() == "Camera")
                    {
                        continue;
                    }
                    // Real obstacle found: reject this placement position.
                    return false;
                }
            }
        }
        else
        {
            // Include mode: cast against allowed bits — must hit to place
            queryMask = rule.categoriesId;
            allowIfHit = true;
        }

        this->raySceneQuery->setQueryMask(queryMask);
        this->raySceneQuery->setSortByDistance(true);
        this->raySceneQuery->setRay(Ogre::Ray(Ogre::Vector3(position.x, 10000.0f, position.z), Ogre::Vector3::NEGATIVE_UNIT_Y));

        Ogre::Vector3 hitPoint, hitNormal;
        Ogre::MovableObject* hitObject = nullptr;
        Ogre::Real closestDistance = 0.0f;
        std::vector<Ogre::MovableObject*> excludeList;

        Ogre::Camera* camera = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
        MathHelper::getInstance()->getRaycastFromPoint(this->raySceneQuery, camera, hitPoint, (size_t&)hitObject, closestDistance, hitNormal, &excludeList);

        return allowIfHit ? (hitObject != nullptr) : (hitObject == nullptr);
    }

    std::vector<VegetationBatch> ProceduralFoliageVolumeComponent::calculatePlanetFoliagePositions(GameObject* planetGo, Ogre::Real planetRadius, const Ogre::Vector3& planetCentre)
    {
        // -----------------------------------------------------------------------
        // NEW: use PlanetTerraComponent::collectSurfaceSamples() instead of
        // scene ray queries.  The old approach fired 3 rays per sample point
        // (primary + 2 neighbours for normal reconstruction) and iterated a
        // lat/lon grid that duplicated many sphere positions at the poles.
        // The new approach reads vertices[] / normals[] / baseDirs[] directly
        // from PlanetTerra's CPU arrays in a single O(V) pass -- no GPU round-
        // trips, no back-face normal ambiguity, no camera dependency.
        // For a 64x64 planet (4225 vertices) the whole scan is < 1 ms.
        // -----------------------------------------------------------------------

        auto terraComp = NOWA::makeStrongPtr(planetGo->getComponent<PlanetTerraComponent>());
        if (nullptr == terraComp)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralFoliageVolume] calculatePlanetFoliagePositions: "
                                                                                "no PlanetTerraComponent on '" +
                                                                                    planetGo->getName() + "'.");
            return {};
        }

        std::vector<VegetationBatch> batches;

        for (size_t ruleIdx = 0u; ruleIdx < this->rules.size(); ++ruleIdx)
        {
            const FoliageRule& rule = this->rules[ruleIdx];
            if (false == rule.enabled || true == rule.meshName.empty())
            {
                continue;
            }

            // Collect all surface samples that pass the slope gate up-front.
            // Height filtering is done per-rule below so we pass full range here
            // and let meetsPlanetCriteria handle heightRange.  slopeMaxDeg is the
            // strictest filter -- apply it here to avoid allocating samples that
            // will always be rejected.
            std::vector<PlanetTerra::SurfaceSample> samples;
            samples.reserve(terraComp->getVertexCount());

            Ogre::Vector3 worldOffset;
            terraComp->collectSurfaceSamples(rule.maxSlope, // slopeMaxDeg
                0.0f,                                       // heightMinLocal: no lower bound here
                std::numeric_limits<float>::max(),          // heightMaxLocal: no upper bound here
                samples, worldOffset);
            // worldOffset == planetGo->getPosition() (set by PlanetTerraComponent wrapper)

            if (samples.empty())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralFoliageVolume] Planet rule '" + rule.name + "': no samples passed slope filter (" + Ogre::StringConverter::toString(rule.maxSlope) + " deg).");
                continue;
            }

            // Density thinning: the vertex grid is fixed, so we use a combination
            // of rule.density and a deterministic per-vertex random to thin the set
            // down to the desired density.  This replaces the old angular-step loop
            // and preserves reproducibility via masterSeed + rule.seed.
            std::mt19937 rng(this->masterSeed->getUInt() + rule.seed);
            std::uniform_real_distribution<float> dist01(0.0f, 1.0f);

            // density in [0,1]: 1.0 = place on every qualifying vertex,
            // 0.5 = place on ~50% of qualifying vertices, etc.
            // The old code used angularStep = resolution / (radius * density) so
            // higher density -> smaller step -> more samples.  We mirror that by
            // treating density as the per-vertex acceptance probability.
            const float acceptProb = Ogre::Math::Clamp(static_cast<float>(rule.density), 0.0f, 1.0f);

            VegetationBatch batch;
            batch.meshName = rule.meshName;
            batch.ruleIndex = ruleIdx;

            for (const PlanetTerra::SurfaceSample& s : samples)
            {
                // ---- density thinning ----
                if (dist01(rng) > acceptProb)
                {
                    continue;
                }

                // ---- world-space hit point ----
                // localPos is in planet-local space (centre = origin).
                // worldOffset = planet GO world position.
                const Ogre::Vector3 hitPoint = worldOffset + s.localPos;

                // ---- outward radial at this vertex ----
                // baseDirs[i] is exactly s.normal for a flat sphere.  For a
                // displaced sphere, the "outward" radial is localPos.normalisedCopy().
                // We use s.normal (the actual mesh normal) for slope and orientation
                // and the radial for height-above-sea.
                const Ogre::Vector3 outward = s.localPos.normalisedCopy();

                // ---- height above sea level ----
                const Ogre::Real heightAboveSea = s.localPos.length() - planetRadius;

                // ---- criteria check (height + slope) ----
                // s.slopeDeg is already computed from normal vs. baseDirs, so pass
                // s.normal and outward to meetsPlanetCriteria unchanged.
                if (false == this->meetsPlanetCriteria(heightAboveSea, s.normal, outward, rule))
                {
                    continue;
                }

                // ---- spacing check ----
                if (false == this->hasMinimumSpacing(hitPoint, rule, batch.instances))
                {
                    continue;
                }

                // ---- orientation ----
                // "Up" on a planet = outward radial, optionally lerped toward the
                // actual mesh normal by alignToNormal (same semantic as flat terrain).
                Ogre::Vector3 upAxis;
                if (rule.alignToNormal > 0.0f)
                {
                    upAxis = Ogre::Vector3(Ogre::Math::lerp(outward.x, s.normal.x, rule.alignToNormal), Ogre::Math::lerp(outward.y, s.normal.y, rule.alignToNormal), Ogre::Math::lerp(outward.z, s.normal.z, rule.alignToNormal));
                    upAxis.normalise();
                }
                else
                {
                    upAxis = outward;
                }

                // Stable tangent frame -- avoid UNIT_Y degeneracy near poles.
                Ogre::Vector3 worldRef = (std::abs(upAxis.dotProduct(Ogre::Vector3::UNIT_Y)) < 0.99f) ? Ogre::Vector3::UNIT_Y : Ogre::Vector3::UNIT_X;

                Ogre::Vector3 right = worldRef.crossProduct(upAxis);
                right.normalise();
                Ogre::Vector3 forward = upAxis.crossProduct(right);
                forward.normalise();

                Ogre::Quaternion orientation;
                orientation.FromAxes(right, upAxis, forward);

                // Random rotation around local up.
                if (true == rule.randomYRotation)
                {
                    Ogre::Degree yRot(Ogre::Math::lerp(rule.yRotationRange.x, rule.yRotationRange.y, dist01(rng)));
                    Ogre::Quaternion yQuat;
                    yQuat.FromAngleAxis(yRot, upAxis);
                    orientation = orientation * yQuat;
                }

                // ---- scale ----
                Ogre::Real scaleRand = Ogre::Math::lerp(rule.scaleRange.x, rule.scaleRange.y, dist01(rng));
                Ogre::Vector3 scale = rule.uniformScale ? Ogre::Vector3(scaleRand) : Ogre::Vector3(scaleRand, scaleRand, scaleRand);

                batch.instances.emplace_back(hitPoint, orientation, scale, ruleIdx);
            }

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralFoliageVolume] Planet rule '" + rule.name + "' placed " + Ogre::StringConverter::toString(static_cast<unsigned int>(batch.instances.size())) +
                                                                                   " instances (from " + Ogre::StringConverter::toString(static_cast<unsigned int>(samples.size())) + " surface samples).");

            if (false == batch.instances.empty())
            {
                batches.push_back(std::move(batch));
            }
        }

        // -----------------------------------------------------------------------
        // CATEGORY FILTER PASS  (unchanged from original)
        // isCategoryAllowed uses a downward ray -- on a planet surface the
        // instance is already ON the surface so this pass mainly rejects
        // instances that landed inside excluded zones (e.g. water volumes).
        // -----------------------------------------------------------------------
        for (auto& batch : batches)
        {
            const FoliageRule& rule = this->rules[batch.ruleIndex];
            if (rule.categoriesId == GameObjectController::ALL_CATEGORIES_ID)
            {
                continue;
            }

            std::vector<VegetationInstance> filtered;
            filtered.reserve(batch.instances.size());
            for (const auto& instance : batch.instances)
            {
                if (true == this->isCategoryAllowed(instance.position, rule))
                {
                    filtered.push_back(instance);
                }
            }
            batch.instances = std::move(filtered);
        }

        batches.erase(std::remove_if(batches.begin(), batches.end(),
                          [](const VegetationBatch& b)
                          {
                              return b.instances.empty();
                          }),
            batches.end());

        return batches;
    }

    bool ProceduralFoliageVolumeComponent::meetsPlanetCriteria(Ogre::Real heightAboveSeaLevel, const Ogre::Vector3& hitNormal, const Ogre::Vector3& outwardDir, const FoliageRule& rule) const
    {
        // ---- Height above sea level check ----
        // rule.heightRange.x = minimum height above sea level (negative = underwater)
        // rule.heightRange.y = maximum height above sea level
        // Reuses the same FoliageRule field as flat terrain -- just measured radially.
        if (heightAboveSeaLevel < rule.heightRange.x || heightAboveSeaLevel > rule.heightRange.y)
        {
            return false;
        }

        // ---- Slope check ----
        // On flat terrain: slope = angle between surface normal and world UP (UNIT_Y).
        // On a planet:     slope = angle between surface normal and outward radial.
        // outwardDir is the "local up" at this point on the sphere -- the equivalent
        // of UNIT_Y for a curved surface.
        // dot == 1.0  -> perfectly flat (normal parallel to radial) -> slope = 0 deg
        // dot == 0.0  -> vertical cliff -> slope = 90 deg
        // dot < 0.0   -> overhanging surface -> slope > 90 deg (always rejected)
        const Ogre::Real dot = Ogre::Math::Clamp(hitNormal.dotProduct(outwardDir), -1.0f, 1.0f);
        const Ogre::Real slope = Ogre::Math::ACos(dot).valueDegrees();

        if (slope > rule.maxSlope)
        {
            return false;
        }

        // Terra layer check is intentionally omitted for planets.
        // Planets have no blend map. Use heightRange to restrict placement to
        // specific altitude bands (e.g. lowland forests, mountain snow, etc.).
        // If per-biome control is needed in the future, sample PlanetTerraComponent
        // colour/weight at the hit point and add a rule.planetLayerThreshold field.

        return true;
    }

    void ProceduralFoliageVolumeComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
    {
        // Lua bindings - can be filled in later if needed
    }

    bool ProceduralFoliageVolumeComponent::canStaticAddComponent(GameObject* gameObject)
    {
        auto terraCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<TerraComponent>());
        auto planetTerraCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<PlanetTerraComponent>());
        if (nullptr != terraCompPtr || nullptr != planetTerraCompPtr)
        {
            return true;
        }

        return false;
    }

}; // namespace end
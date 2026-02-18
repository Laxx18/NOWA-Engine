#include "NOWAPrecompiled.h"
#include "ProceduralFoliageVolumeComponent.h"
#include "gameobject/GameObjectFactory.h"
#include "gameobject/TerraComponent.h"
#include "main/AppStateManager.h"
#include "main/EventManager.h"
#include "modules/GraphicsModule.h"
#include "modules/LuaScriptApi.h"
#include "utilities/XMLConverter.h"

#include "OgreMeshManager2.h"
#include "OgreConfigFile.h"
#include "OgreLodConfig.h"
#include "OgreLodStrategyManager.h"
#include "OgreMeshLodGenerator.h"
#include "OgreMesh2Serializer.h"
#include "OgrePixelCountLodStrategy.h"
#include "OgreHlmsPbsPrerequisites.h"

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
        gridResolution(new Variant(ProceduralFoliageVolumeComponent::AttrGridResolution(), 1.0f, this->attributes)), // 1 meter sample resolution
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
            GraphicsModule::getInstance()->enqueueAndWait([this, query]()
                {
                    this->gameObjectPtr->getSceneManager()->destroyQuery(query);
                }, "ProceduralFoliageVolumeComponent::DestroyRayQuery");
            this->raySceneQuery = nullptr;
        }

        if (this->sphereSceneQuery)
        {
            auto* query = this->sphereSceneQuery;
            GraphicsModule::getInstance()->enqueueAndWait(
                [this, query]()
                {
                    this->gameObjectPtr->getSceneManager()->destroyQuery(query);
                }, "ProceduralFoliageVolumeComponent::DestroySphereQuery");
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

            futures.emplace_back(std::async(std::launch::async, [=]() -> VegetationBatch
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

                            // Terra bounds check
                            if (false == this->isWithinTerraBounds(position, terra))
                            {
                                continue;
                            }

                            // Terra height
                            if (false == terra->getHeightAt(position))
                            {
                                continue;
                            }

                            // Normal
                            Ogre::Vector3 normal = this->calculateTerrainNormal(position, terra);

                            // Ensure normal points up
                            if (normal.y < 0.0f)
                            {
                                normal = -normal;
                            }

                            // Height/slope/layer checks (Terra only - thread safe!)
                            if (false == this->meetsTerrainCriteria(position, normal, rule, terra))
                            {
                                continue;
                            }

                            // Spacing check
                            if (false == this->hasMinimumSpacing(position, rule, batch.instances))
                            {
                                continue;
                            }

                            // Build instance
                            Ogre::Real scaleRand = Ogre::Math::lerp(rule.scaleRange.x, rule.scaleRange.y, dist01(rng));
                            Ogre::Vector3 scale = rule.uniformScale ? Ogre::Vector3(scaleRand) : Ogre::Vector3(scaleRand, scaleRand, scaleRand);

                            // Orientation
                            Ogre::Quaternion orientation = Ogre::Quaternion::IDENTITY;
                            if (rule.alignToNormal > 0.0f)
                            {
                                // Align to terrain normal
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
        // PHASE 2: SEQUENTIAL - Category raycast filter (NOT thread safe!)
        // Only runs if any rule has non-"All" category -> often zero cost!
        // ============================================================================
        for (auto& batch : batches)
        {
            const FoliageRule& rule = this->rules[batch.ruleIndex];

            // Skip raycast entirely if category is "All" (most common case!)
            if (rule.categoriesId == GameObjectController::ALL_CATEGORIES_ID)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralFoliageVolume] Rule '" + rule.name + "' category=All, skipping raycast filter.");
                continue;
            }

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
                "[ProceduralFoliageVolume] Rule '" + rule.name + "' filtering " + Ogre::StringConverter::toString(batch.instances.size()) + " instances by category '" + rule.categories + "'...");

            // Filter instances by category (sequential, single raySceneQuery)
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

        // Remove empty batches
        batches.erase(std::remove_if(batches.begin(), batches.end(), [](const VegetationBatch& b)
            {
                return b.instances.empty();
            }), batches.end());

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

        if (slope < rule.slopeRange.x || slope > rule.slopeRange.y)
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
        // Queue to render thread
        GraphicsModule::getInstance()->enqueue([this, batchesCopy = std::move(batches)]() mutable
            {
                this->createFoliageItems(batchesCopy);
            }, "ProceduralFoliageVolume::CreateItems");

        // Now create physics collision (on main thread, after Items exist)
        this->createFoliageCollision();
    }

    void ProceduralFoliageVolumeComponent::createFoliageItems(std::vector<VegetationBatch>& batches)
    {
        Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();

        for (auto& batch : batches)
        {
            const FoliageRule& rule = this->rules[batch.ruleIndex];

            if (true == batch.instances.empty())
            {
                continue;
            }

            // Generate LOD
            if (rule.lodDistance > 0.0f)
            {
                this->generateLODForMesh(batch.meshName, rule.lodDistance, rule.resourceGroup);
            }

            // Load mesh
            Ogre::MeshPtr meshPtr;
            try
            {
                meshPtr = Ogre::MeshManager::getSingletonPtr()->load(batch.meshName, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
            }
            catch (const Ogre::Exception& e)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralFoliageVolume] Failed to load mesh: " + e.getDescription());
                continue;
            }

            if (!meshPtr)
            {
                continue;
            }

            // ============================================================================
            // CREATE FIRST ITEM
            // ============================================================================
            Ogre::Item* firstItem = sceneManager->createItem(meshPtr, Ogre::SCENE_STATIC);

            if (firstItem->getNumSubItems() == 0)
            {
                sceneManager->destroyItem(firstItem);
                continue;
            }

            // CHECK TRANSPARENCY THE CORRECT WAY - via HlmsBlendblock!
            bool hasTransparency = false;

            // Store all submesh datablocks for sharing
            std::vector<Ogre::HlmsDatablock*> submeshDatablocks;

            for (size_t subIdx = 0; subIdx < firstItem->getNumSubItems(); subIdx++)
            {
                Ogre::SubItem* subItem = firstItem->getSubItem(subIdx);
                Ogre::HlmsDatablock* datablock = subItem->getDatablock();
                submeshDatablocks.push_back(datablock);

                // Get the blendblock from the datablock
                const Ogre::HlmsBlendblock* blendblock = datablock->getBlendblock(false); // false = regular pass

                if (blendblock)
                {
                    // Check if this blendblock is marked as transparent
                    if (blendblock->isAutoTransparent() || blendblock->isForcedTransparent())
                    {
                        hasTransparency = true;

                        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralFoliageVolume] Submesh " + Ogre::StringConverter::toString(subIdx) + " of '" + batch.meshName + "' has transparent blendblock");
                    }
                }
            }

            // SET CORRECT RENDER QUEUE
            Ogre::uint8 renderQueue;
            if (hasTransparency)
            {
                renderQueue = NOWA::RENDER_QUEUE_V2_TRANSPARENT; // 205 (RQ 200-224)
            }
            else
            {
                renderQueue = NOWA::RENDER_QUEUE_V2_MESH; // 10 (RQ 0-99)
            }

            firstItem->setRenderQueueGroup(renderQueue);

            // Configure first Item
            firstItem->setCastShadows(rule.castShadows);
            if (rule.renderDistance > 0.0f)
            {
                firstItem->setRenderingDistance(rule.renderDistance);
            }
            if (rule.shadowDistance > 0.0f)
            {
                firstItem->setShadowRenderingDistance(rule.shadowDistance);
            }

            // Create scene node
            Ogre::SceneNode* firstNode = sceneManager->getRootSceneNode(Ogre::SCENE_STATIC)->createChildSceneNode(Ogre::SCENE_STATIC);

            const VegetationInstance& firstInstance = batch.instances[0];
            firstNode->setPosition(firstInstance.position);
            firstNode->setOrientation(firstInstance.orientation);
            firstNode->setScale(firstInstance.scale);
            firstNode->attachObject(firstItem);

            batch.items.push_back(firstItem);
            batch.nodes.push_back(firstNode);
            batch.sharedDatablock = submeshDatablocks[0]; // Store primary datablock

            // ============================================================================
            // CREATE REMAINING ITEMS - Share ALL submesh datablocks!
            // ============================================================================
            for (size_t i = 1; i < batch.instances.size(); i++)
            {
                const VegetationInstance& instance = batch.instances[i];

                Ogre::Item* item = sceneManager->createItem(meshPtr, Ogre::SCENE_STATIC);

                // SHARE DATABLOCKS FOR ALL SUBMESHES (for batching!)
                for (size_t subIdx = 0; subIdx < item->getNumSubItems() && subIdx < submeshDatablocks.size(); subIdx++)
                {
                    item->getSubItem(subIdx)->setDatablock(submeshDatablocks[subIdx]);
                }

                // Set same render queue
                item->setRenderQueueGroup(renderQueue);

                // Configure
                item->setCastShadows(rule.castShadows);
                if (rule.renderDistance > 0.0f)
                {
                    item->setRenderingDistance(rule.renderDistance);
                }
                if (rule.shadowDistance > 0.0f)
                {
                    item->setShadowRenderingDistance(rule.shadowDistance);
                }

                // Create scene node
                Ogre::SceneNode* node = sceneManager->getRootSceneNode(Ogre::SCENE_STATIC)->createChildSceneNode(Ogre::SCENE_STATIC);

                node->setPosition(instance.position);
                node->setOrientation(instance.orientation);
                node->setScale(instance.scale);
                node->attachObject(item);

                batch.items.push_back(item);
                batch.nodes.push_back(node);
            }

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralFoliageVolume] Created " + Ogre::StringConverter::toString(batch.items.size()) + " instances in RQ " + Ogre::StringConverter::toString((int)renderQueue) +
                                                                                    " (" + (hasTransparency ? "TRANSPARENT" : "OPAQUE") + ") for: " + rule.name);
        }

        this->vegetationBatches = std::move(batches);
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

            // Rotate cylinder 90° around Z to make it stand upright (X-axis -> Y-axis)
            Ogre::Quaternion uprightFix(Ogre::Degree(90), Ogre::Vector3::UNIT_Z);

            for (const VegetationInstance& instance : batch.instances)
            {
                Ogre::Quaternion correctedOrientation = instance.orientation * uprightFix;

                // Shift cylinder up so its base sits at the instance position, not its center
                Ogre::Vector3 correctedPosition = instance.position + Ogre::Vector3(0.0f, rule.collisionHeight * 0.5f, 0.0f);

                OgreNewt::CollisionPtr cylinderCol =
                    OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::Cylinder(world, rule.collisionRadius, rule.collisionHeight, this->gameObjectPtr->getCategoryId(), correctedOrientation, correctedPosition));

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

        // Set distances
        Ogre::Real factor[10] = {0.0f};
        for (size_t i = 0; i < lodConfig.levels.size() - 1 && i < 10; ++i)
        {
            factor[i] = lodConfig.levels[i].distance / lodConfig.levels[i + 1].distance;
        }

        for (size_t i = 0; i < lodConfig.levels.size(); ++i)
        {
            Ogre::Real dist = (i < lodConfig.levels.size() - 1) ? lodDistance * factor[i] : lodDistance;
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
            }, "ProceduralFoliageVolume::DestroyItems");

        // Clear physics collision first
        if (this->physicsArtifactComponent && this->physicsArtifactComponent->getBody())
        {
            // This will destroy the compound body
            this->physicsArtifactComponent->reCreateCollision(true);
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
                    sceneManager->destroySceneNode(batch.nodes[i]);
                }

                if (batch.items[i])
                {
                    sceneManager->destroyItem(batch.items[i]);
                }
            }

            batch.items.clear();
            batch.nodes.clear();
        }

        this->vegetationBatches.clear();
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
            this->ruleCategories.resize(count);
            this->ruleClearanceDistances.resize(count);
            this->ruleCollisionEnabled.resize(count);
            this->ruleCollisionRadius.resize(count);
            this->ruleCollisionHeight.resize(count);

            // Initialize new variants with SENSIBLE DEFAULTS
            for (size_t i = oldSize; i < count; i++)
            {
                // Rule Name - UNIQUE
                this->ruleNames[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleName() + Ogre::StringConverter::toString(i), Ogre::String("Vegetation_" + Ogre::StringConverter::toString(i)), this->attributes);
                this->ruleNames[i]->setDescription("Display name for this vegetation type.");

                // Rule Mesh Name
                this->ruleMeshNames[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleMeshName() + Ogre::StringConverter::toString(i), Ogre::String(), this->attributes);
                this->ruleMeshNames[i]->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
                this->ruleMeshNames[i]->setDescription("3D mesh file for this vegetation (.mesh).");

                // Rule Density - VARY BY INDEX for visual distinction
                Ogre::Real densityVariation = 0.3f + (i * 0.2f); // 0.3, 0.5, 0.7, 0.9, ...
                this->ruleDensities[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleDensity() + Ogre::StringConverter::toString(i), densityVariation, this->attributes);
                this->ruleDensities[i]->setDescription("Instances per m * m. Trees: 0.1-0.5, Bushes: 0.5-2.0, Grass: 2.0-10.0");
                this->ruleDensities[i]->setConstraints(0.01f, 10.0f);

                // Height Range - UNLIMITED (ensure visibility!)
                this->ruleHeightRanges[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleHeightRange() + Ogre::StringConverter::toString(i), Ogre::Vector2(-1000.0f, 1000.0f), // <- Unlimited!
                    this->attributes);
                this->ruleHeightRanges[i]->setDescription("Min/max elevation. Use -1000 to 1000 for unlimited.");

                // Max Slope - PERMISSIVE (ensure visibility!)
                this->ruleMaxSlopes[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleMaxSlope() + Ogre::StringConverter::toString(i),
                    85.0f, // <- Almost any slope! (very permissive)
                    this->attributes);
                this->ruleMaxSlopes[i]->setDescription("Max slope angle. Use 85+ for nearly any terrain.");
                this->ruleMaxSlopes[i]->setConstraints(0.0f, 90.0f);

                // Terra Layers - ALL ALLOWED (ensure visibility!)
                this->ruleTerraLayers[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleTerraLayers() + Ogre::StringConverter::toString(i),
                    Ogre::String("255,255,255,255"), // All layers
                    this->attributes);
                this->ruleTerraLayers[i]->setDescription("Layer thresholds. 255,255,255,255 = all layers allowed. Or 255,255,255,0 -> Layer 1-3 allowed and layer 4 not.");

                // Scale Range
                this->ruleScaleRanges[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleScaleRange() + Ogre::StringConverter::toString(i), Ogre::Vector2(0.4f, 0.6f), this->attributes);
                this->ruleScaleRanges[i]->setDescription("Min/max scale for size variation.");

                // Min Spacing - NO SPACING by default (ensure visibility!)
                this->ruleMinSpacings[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleMinSpacing() + Ogre::StringConverter::toString(i),
                    0.0f, // No spacing constraint
                    this->attributes);
                this->ruleMinSpacings[i]->setDescription("Min spacing between instances. 0 = no constraint.");
                this->ruleMinSpacings[i]->setConstraints(0.0f, 100.0f);

                // Render Distance - GOOD DEFAULT
                this->ruleRenderDistances[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleRenderDistance() + Ogre::StringConverter::toString(i),
                    300.0f, // 300m visibility
                    this->attributes);
                this->ruleRenderDistances[i]->setDescription("Visibility distance. 0 = infinite.");
                this->ruleRenderDistances[i]->setConstraints(0.0f, 10000.0f);

                // LOD Distance - DISABLED by default
                this->ruleLodDistances[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleLodDistance() + Ogre::StringConverter::toString(i),
                    0.0f, // No LOD by default
                    this->attributes);
                this->ruleLodDistances[i]->setDescription("LOD distance. 0 = no LOD (simpler).");
                this->ruleLodDistances[i]->setConstraints(0.0f, 1000.0f);

                // Rule Categories
                this->ruleCategories[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleCategories() + Ogre::StringConverter::toString(i), Ogre::String("All"), this->attributes);
                this->ruleCategories[i]->setDescription("Categories mask for raycasting. 'All' = grow everywhere. 'All-House' = everywhere except on Houses. "
                                                        "'Ground' = only on Ground category objects. Combine multiple with dash: 'Ground-Terrain'.");
                this->rules[i].categories = "All";
                this->rules[i].categoriesId = GameObjectController::ALL_CATEGORIES_ID;

                this->ruleClearanceDistances[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleClearanceDistance() + Ogre::StringConverter::toString(i), 0.0f, this->attributes);
                this->ruleClearanceDistances[i]->setDescription("Clearance buffer in meters around excluded objects (e.g. houses). 0 = disabled. Set to 2-5m to prevent branches clipping into buildings.");
                this->ruleClearanceDistances[i]->setConstraints(0.0f, 50.0f);
                this->rules[i].clearanceDistance = 0.0f;

                // Rule Collision Enabled
                this->ruleCollisionEnabled[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleCollisionEnabled() + Ogre::StringConverter::toString(i), false, this->attributes);
                this->ruleCollisionEnabled[i]->setDescription("Enable physics collision for this vegetation. "
                                                              "All instances combined into one compound body for performance.");
                this->rules[i].collisionEnabled = false;

                // Rule Collision Radius
                this->ruleCollisionRadius[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleCollisionRadius() + Ogre::StringConverter::toString(i), 0.3f, this->attributes);
                this->ruleCollisionRadius[i]->setDescription("Collision cylinder radius in meters (trunk/stem thickness). Trees: 0.2-0.5m, Bushes: 0.1-0.3m");
                this->ruleCollisionRadius[i]->setConstraints(0.01f, 5.0f);
                this->rules[i].collisionRadius = 0.3f;

                // Rule Collision Height
                this->ruleCollisionHeight[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleCollisionHeight() + Ogre::StringConverter::toString(i), 2.0f, this->attributes);
                this->ruleCollisionHeight[i]->setDescription("Collision cylinder height in meters. Should match approximate trunk height. Trees: 2-5m, Bushes: 0.5-1.5m");
                this->ruleCollisionHeight[i]->setConstraints(0.1f, 20.0f);
                this->rules[i].collisionHeight = 2.0f;
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
            this->rules[index].resourceGroup = this->extractResourceGroupFromPath(folderPath);

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

    Ogre::String ProceduralFoliageVolumeComponent::extractResourceGroupFromPath(const Ogre::String& path)
    {
        // Path: "../../media/models/Plants"
        // Extract: "Plants"

        size_t lastSlash = path.find_last_of("/\\");
        if (lastSlash != Ogre::String::npos)
        {
            Ogre::String groupName = path.substr(lastSlash + 1);

            // Verify group exists
            if (Ogre::ResourceGroupManager::getSingleton().resourceGroupExists(groupName))
            {
                return groupName;
            }
        }

        // Fallback to AUTODETECT if extraction fails
        return Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME;
    }

    bool ProceduralFoliageVolumeComponent::isCategoryAllowed(const Ogre::Vector3& position, const FoliageRule& rule)
    {
        // Fast path
        if (rule.categoriesId == GameObjectController::ALL_CATEGORIES_ID)
        {
            return true;
        }

        if (!this->raySceneQuery)
        {
            return true;
        }

        bool isExcludeMode = (rule.categories.find("All") != Ogre::String::npos && rule.categories.find('-') != Ogre::String::npos);

        unsigned int queryMask;
        if (isExcludeMode)
        {
            size_t dashPos = rule.categories.find('-');
            Ogre::String excludedCategories = rule.categories.substr(dashPos + 1);
            queryMask = AppStateManager::getSingletonPtr()->getGameObjectController()->generateCategoryId(excludedCategories);
        }
        else
        {
            queryMask = rule.categoriesId;
        }

        if (queryMask == 0)
        {
            return true;
        }

        // ── Raycast (direct hit check) ──────────────────────────────────────
        this->raySceneQuery->setQueryMask(queryMask);
        this->raySceneQuery->setSortByDistance(true);

        Ogre::Vector3 rayOrigin = Ogre::Vector3(position.x, 10000.0f, position.z);
        Ogre::Ray ray(rayOrigin, Ogre::Vector3::NEGATIVE_UNIT_Y);
        this->raySceneQuery->setRay(ray);

        Ogre::Vector3 hitPoint = Ogre::Vector3::ZERO;
        Ogre::Vector3 hitNormal = Ogre::Vector3::ZERO;
        Ogre::MovableObject* hitMovableObject = nullptr;
        Ogre::Real closestDistance = 0.0f;
        std::vector<Ogre::MovableObject*> excludeList;

        Ogre::Camera* camera = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();

        MathHelper::getInstance()->getRaycastFromPoint(this->raySceneQuery, camera, hitPoint, (size_t&)hitMovableObject, closestDistance, hitNormal, &excludeList);

        if (true == isExcludeMode)
        {
            // Direct hit -> always blocked
            if (hitMovableObject != nullptr)
            {
                return false;
            }

            // ── Clearance sphere check ──────────────────────────────────────
            // Even if no direct hit above, check if excluded object is nearby
            if (rule.clearanceDistance > 0.0f && this->sphereSceneQuery)
            {
                Ogre::Sphere sphere(position, rule.clearanceDistance);
                this->sphereSceneQuery->setSphere(sphere);
                this->sphereSceneQuery->setQueryMask(queryMask);

                Ogre::SceneQueryResultMovableList& results = this->sphereSceneQuery->execute().movables;

                if (false == results.empty())
                {
                    // An excluded object (e.g. house) is within clearanceDistance!
                    return false;
                }
            }

            return true; // No direct hit, no nearby excluded objects
        }
        else
        {
            // INCLUDE mode: must hit the target category
            return (hitMovableObject != nullptr);
        }
    }

    void ProceduralFoliageVolumeComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
    {
        // Lua bindings - can be filled in later if needed
    }

    bool ProceduralFoliageVolumeComponent::canStaticAddComponent(GameObject* gameObject)
    {
        auto terraCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<TerraComponent>());
        if (nullptr != terraCompPtr)
        {
            return terraCompPtr->getTerra() != nullptr;
        }

        return false;
    }

}; // namespace end
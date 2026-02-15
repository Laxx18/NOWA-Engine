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
        spatialHashCellSize(10.0f),
        // GOOD DEFAULTS: Will actually work out of the box!
        volumeBounds(new Variant(ProceduralFoliageVolumeComponent::AttrVolumeBounds(), Ogre::Vector4(-50.0f, -50.0f, 50.0f, 50.0f), this->attributes)), // 100x100m area
        masterSeed(new Variant(ProceduralFoliageVolumeComponent::AttrMasterSeed(), static_cast<unsigned int>(12345), this->attributes)),
        gridResolution(new Variant(ProceduralFoliageVolumeComponent::AttrGridResolution(), 1.0f, this->attributes)), // 1 meter sample resolution
        autoGenerate(new Variant(ProceduralFoliageVolumeComponent::AttrAutoGenerate(), true, this->attributes)),
        clearOnDisconnect(new Variant(ProceduralFoliageVolumeComponent::AttrClearOnDisconnect(), false, this->attributes)),
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
        this->autoGenerate->setDescription("Automatically generate vegetation when simulation starts (connect).");
        this->clearOnDisconnect->setDescription("Clear vegetation when simulation stops (disconnect). Useful for iteration.");
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

    // ============================================================================
    // INIT / XML SERIALIZATION
    // ============================================================================

    bool ProceduralFoliageVolumeComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        // Load volume settings
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "VolumeBounds")
        {
            this->volumeBounds->setValue(XMLConverter::getAttribVector4(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MasterSeed")
        {
            this->masterSeed->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "GridResolution")
        {
            this->gridResolution->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AutoGenerate")
        {
            this->autoGenerate->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ClearOnDisconnect")
        {
            this->clearOnDisconnect->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RuleCount")
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
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RuleName" + Ogre::StringConverter::toString(i))
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
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RuleMeshName" + Ogre::StringConverter::toString(i))
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

            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RuleResourceGroup" + Ogre::StringConverter::toString(i))
            {
                Ogre::String resourceGroup = XMLConverter::getAttrib(propertyElement, "data");
                this->rules[i].resourceGroup = resourceGroup;
                propertyElement = propertyElement->next_sibling("property");
            }

            // Rule Density
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RuleDensity" + Ogre::StringConverter::toString(i))
            {
                Ogre::Real density = XMLConverter::getAttribReal(propertyElement, "data");
                if (nullptr == this->ruleDensities[i])
                {
                    this->ruleDensities[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleDensity() + Ogre::StringConverter::toString(i), 1.0f, this->attributes);
                    this->ruleDensities[i]->setDescription("Instances per m² (0.01 - 10.0).");
                }
                this->ruleDensities[i]->setValue(density);
                this->rules[i].density = density;
                propertyElement = propertyElement->next_sibling("property");
            }

            // Rule Height Range
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RuleHeightRange" + Ogre::StringConverter::toString(i))
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
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RuleMaxSlope" + Ogre::StringConverter::toString(i))
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
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RuleTerraLayers" + Ogre::StringConverter::toString(i))
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
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RuleScaleRange" + Ogre::StringConverter::toString(i))
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
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RuleMinSpacing" + Ogre::StringConverter::toString(i))
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
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RuleRenderDistance" + Ogre::StringConverter::toString(i))
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
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RuleLodDistance" + Ogre::StringConverter::toString(i))
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
        }

        return true;
    }

    bool ProceduralFoliageVolumeComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralFoliageVolumeComponent] Init component for game object: " + this->gameObjectPtr->getName());

        return true;
    }

    bool ProceduralFoliageVolumeComponent::connect(void)
    {
        GameObjectComponent::connect();

        if (this->autoGenerate->getBool())
        {
            this->regenerateFoliage();
        }

        return true;
    }

    bool ProceduralFoliageVolumeComponent::disconnect(void)
    {
        if (this->clearOnDisconnect->getBool())
        {
            this->clearFoliage();
        }

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

        // Destroy all vegetation (blocking)
        this->clearFoliage();

        boost::shared_ptr<EventDataRefreshGui> eventDataRefreshGui(new EventDataRefreshGui());
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshGui);
    }

    void ProceduralFoliageVolumeComponent::onOtherComponentRemoved(unsigned int index)
    {
        // Not needed
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
        else if (ProceduralFoliageVolumeComponent::AttrAutoGenerate() == attribute->getName())
        {
            this->setAutoGenerateOnConnect(attribute->getBool());
        }
        else if (ProceduralFoliageVolumeComponent::AttrClearOnDisconnect() == attribute->getName())
        {
            this->setClearOnDisconnect(attribute->getBool());
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
            }
        }
    }

    void ProceduralFoliageVolumeComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        GameObjectComponent::writeXML(propertiesXML, doc);

        // Write volume settings
        xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "10")); // Vector4
        propertyXML->append_attribute(doc.allocate_attribute("name", "VolumeBounds"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->volumeBounds->getVector4())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2")); // Unsigned int
        propertyXML->append_attribute(doc.allocate_attribute("name", "MasterSeed"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->masterSeed->getUInt())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6")); // Real
        propertyXML->append_attribute(doc.allocate_attribute("name", "GridResolution"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->gridResolution->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12")); // Bool
        propertyXML->append_attribute(doc.allocate_attribute("name", "AutoGenerate"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->autoGenerate->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12")); // Bool
        propertyXML->append_attribute(doc.allocate_attribute("name", "ClearOnDisconnect"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->clearOnDisconnect->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2")); // Unsigned int
        propertyXML->append_attribute(doc.allocate_attribute("name", "RuleCount"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->ruleCount->getUInt())));
        propertiesXML->append_node(propertyXML);

        // Write each rule's properties
        for (size_t i = 0; i < this->rules.size(); i++)
        {
            // Rule Name
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "7")); // String
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "RuleName" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->ruleNames[i]->getString())));
            propertiesXML->append_node(propertyXML);

            // Rule Mesh Name
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "RuleMeshName" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->ruleMeshNames[i]->getString())));
            propertiesXML->append_node(propertyXML);

            // Rule ResourceGroup
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "RuleResourceGroup" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rules[i].resourceGroup)));
            propertiesXML->append_node(propertyXML);

            // Rule Density
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "RuleDensity" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->ruleDensities[i]->getReal())));
            propertiesXML->append_node(propertyXML);

            // Rule Height Range
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "8")); // Vector2
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "RuleHeightRange" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->ruleHeightRanges[i]->getVector2())));
            propertiesXML->append_node(propertyXML);

            // Rule Max Slope
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "RuleMaxSlope" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->ruleMaxSlopes[i]->getReal())));
            propertiesXML->append_node(propertyXML);

            // Rule Terra Layers
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "RuleTerraLayers" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->ruleTerraLayers[i]->getString())));
            propertiesXML->append_node(propertyXML);

            // Rule Scale Range
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "RuleScaleRange" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->ruleScaleRanges[i]->getVector2())));
            propertiesXML->append_node(propertyXML);

            // Rule Min Spacing
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "RuleMinSpacing" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->ruleMinSpacings[i]->getReal())));
            propertiesXML->append_node(propertyXML);

            // Rule Render Distance
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "RuleRenderDistance" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->ruleRenderDistances[i]->getReal())));
            propertiesXML->append_node(propertyXML);

            // Rule LOD Distance
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "RuleLodDistance" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->ruleLodDistances[i]->getReal())));
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

        if (this->rules.empty())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralFoliageVolume] No rules defined, skipping generation.");
            return;
        }

        // Clear existing
        this->clearFoliage();

        auto start = std::chrono::high_resolution_clock::now();

        // Calculate all positions (parallel, main thread)
        std::vector<VegetationBatch> batches = this->calculateFoliagePositions();

        if (batches.empty())
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
        // Get Terra for height sampling
        Ogre::Terra* terra = nullptr;
        auto terraList = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsFromComponent(TerraComponent::getStaticClassName());
        if (!terraList.empty())
        {
            auto terraCompPtr = NOWA::makeStrongPtr(terraList[0]->getComponent<TerraComponent>());
            if (terraCompPtr)
            {
                terra = terraCompPtr->getTerra();
            }
        }

        if (!terra)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralFoliageVolume] No Terra found! Cannot generate vegetation.");
            return {};
        }

        // Get volume bounds
        Ogre::Vector4 boundsVec = this->volumeBounds->getVector4();
        Ogre::Vector2 minXZ(boundsVec.x, boundsVec.y);
        Ogre::Vector2 maxXZ(boundsVec.z, boundsVec.w);

        // Parallel generation: one future per rule
        std::vector<std::future<VegetationBatch>> futures;

        for (size_t ruleIdx = 0; ruleIdx < this->rules.size(); ruleIdx++)
        {
            const FoliageRule& rule = this->rules[ruleIdx];

            if (!rule.enabled || rule.meshName.empty())
            {
                continue;
            }

            // Launch async task for this rule
            futures.emplace_back(std::async(std::launch::async,
                [=]() -> VegetationBatch
                {
                    VegetationBatch batch;
                    batch.meshName = rule.meshName;
                    batch.ruleIndex = ruleIdx;

                    // Seeded RNG for reproducibility
                    std::mt19937 rng(this->masterSeed->getUInt() + rule.seed);
                    std::uniform_real_distribution<float> dist01(0.0f, 1.0f);

                    const Ogre::Real resolution = this->gridResolution->getReal();
                    const Ogre::Real stepX = resolution / rule.density;
                    const Ogre::Real stepZ = resolution / rule.density;

                    // Grid sampling
                    for (Ogre::Real x = minXZ.x; x < maxXZ.x; x += stepX)
                    {
                        for (Ogre::Real z = minXZ.y; z < maxXZ.y; z += stepZ)
                        {
                            // Add jitter for natural look
                            Ogre::Real jitterX = (dist01(rng) - 0.5f) * stepX * 0.8f;
                            Ogre::Real jitterZ = (dist01(rng) - 0.5f) * stepZ * 0.8f;

                            Ogre::Vector3 position(x + jitterX, 0.0f, z + jitterZ);

                            // CHECK TERRA BOUNDS BEFORE SAMPLING!
                            if (!this->isWithinTerraBounds(position, terra))
                            {
                                continue;
                            }

                            // Get height from Terra (now safe!)
                            if (!terra->getHeightAt(position))
                            {
                                continue;
                            }

                            // Calculate normal
                            Ogre::Vector3 normal = this->calculateTerrainNormal(position, terra);

                            // Check if meets rule criteria
                            if (!this->meetsTerrainCriteria(position, normal, rule, terra))
                            {
                                continue;
                            }

                            // Check spacing
                            if (!this->hasMinimumSpacing(position, rule, batch.instances))
                            {
                                continue;
                            }

                            // Create instance
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

                            // Y rotation
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

        // Collect results
        std::vector<VegetationBatch> batches;
        for (auto& future : futures)
        {
            VegetationBatch batch = future.get();
            if (!batch.instances.empty())
            {
                batches.push_back(std::move(batch));
            }
        }

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

        // Slope check (angle from vertical)
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

            for (size_t i = 0; i < std::min(layers.size(), rule.terraLayerThresholds.size()); i++)
            {
                if (layers[i] > rule.terraLayerThresholds[i])
                {
                    return false;
                }
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
        GraphicsModule::getInstance()->enqueue(
            [this, batchesCopy = std::move(batches)]() mutable
            {
                this->createFoliageItems(batchesCopy);
            },
            "ProceduralFoliageVolume::CreateItems");
    }

    void ProceduralFoliageVolumeComponent::createFoliageItems(std::vector<VegetationBatch>& batches)
    {
        Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();

        for (auto& batch : batches)
        {
            const FoliageRule& rule = this->rules[batch.ruleIndex];

            if (batch.instances.empty())
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

        // Import V1→V2 with LOD
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
                this->ruleDensities[i]->setDescription("Instances per m². Trees: 0.1-0.5, Bushes: 0.5-2.0, Grass: 2.0-10.0");
                this->ruleDensities[i]->setConstraints(0.01f, 10.0f);

                // Height Range - UNLIMITED (ensure visibility!)
                this->ruleHeightRanges[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleHeightRange() + Ogre::StringConverter::toString(i), Ogre::Vector2(-1000.0f, 1000.0f), // ← Unlimited!
                    this->attributes);
                this->ruleHeightRanges[i]->setDescription("Min/max elevation. Use -1000 to 1000 for unlimited.");

                // Max Slope - PERMISSIVE (ensure visibility!)
                this->ruleMaxSlopes[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleMaxSlope() + Ogre::StringConverter::toString(i),
                    85.0f, // ← Almost any slope! (very permissive)
                    this->attributes);
                this->ruleMaxSlopes[i]->setDescription("Max slope angle. Use 85+ for nearly any terrain.");
                this->ruleMaxSlopes[i]->setConstraints(0.0f, 90.0f);

                // Terra Layers - ALL ALLOWED (ensure visibility!)
                this->ruleTerraLayers[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleTerraLayers() + Ogre::StringConverter::toString(i),
                    Ogre::String("255,255,255,255"), // All layers
                    this->attributes);
                this->ruleTerraLayers[i]->setDescription("Layer thresholds. 255,255,255,255 = all layers allowed.");

                // Scale Range
                this->ruleScaleRanges[i] = new Variant(ProceduralFoliageVolumeComponent::AttrRuleScaleRange() + Ogre::StringConverter::toString(i), Ogre::Vector2(0.8f, 1.2f), this->attributes);
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

        // Extract resource group from saved folder path (if available)
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

    void ProceduralFoliageVolumeComponent::setAutoGenerateOnConnect(bool autoGenerate)
    {
        this->autoGenerate->setValue(autoGenerate);
    }

    bool ProceduralFoliageVolumeComponent::getAutoGenerateOnConnect(void) const
    {
        return this->autoGenerate->getBool();
    }

    void ProceduralFoliageVolumeComponent::setClearOnDisconnect(bool clearOnDisconnect)
    {
        this->clearOnDisconnect->setValue(clearOnDisconnect);
    }

    bool ProceduralFoliageVolumeComponent::getClearOnDisconnect(void) const
    {
        return this->clearOnDisconnect->getBool();
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

    // ============================================================================
    // TERRA BOUNDS CHECKING - Add to .cpp file
    // ============================================================================

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

    void ProceduralFoliageVolumeComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
    {
        // Lua bindings - can be filled in later if needed
    }

    bool ProceduralFoliageVolumeComponent::canStaticAddComponent(GameObject* gameObject)
    {
        // Can only be added to main game object (world-level component)
        return (gameObject->getId() == GameObjectController::MAIN_GAMEOBJECT_ID);
    }

}; // namespace end

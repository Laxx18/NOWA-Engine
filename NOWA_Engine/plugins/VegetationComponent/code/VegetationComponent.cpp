#include "NOWAPrecompiled.h"
#include "VegetationComponent.h"
#include "gameobject/GameObjectFactory.h"
#include "gameobject/TerraComponent.h"
#include "main/AppStateManager.h"
#include "main/EventManager.h"
#include "modules/GraphicsModule.h" // Your render thread queue
#include "modules/LuaScriptApi.h"
#include "utilities/XMLConverter.h"

#include <atomic>
#include <future>
#include <regex>

#include "OgreAbiUtils.h"

namespace
{
    std::vector<Ogre::String> split(const Ogre::String& str)
    {
        std::vector<Ogre::String> values;
        std::regex rgx("\\s*,\\s*");
        std::sregex_token_iterator iter(str.begin(), str.end(), rgx, -1);
        std::sregex_token_iterator end;

        for (; iter != end; ++iter)
        {
            values.emplace_back(*iter);
        }

        return values;
    }
}

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    VegetationComponent::VegetationComponent() :
        GameObjectComponent(),
        name("VegetationComponent"),
        minimumBounds(Ogre::Vector3::ZERO),
        maximumBounds(Ogre::Vector3::ZERO),
        raySceneQuery(nullptr),
        regenerate(new Variant(VegetationComponent::AttrRegenerate(), "Generate", this->attributes)),
        targetGameObjectId(new Variant(VegetationComponent::AttrTargetGameObjectId(), static_cast<unsigned long>(0), this->attributes)),
        density(new Variant(VegetationComponent::AttrDensity(), 1.0f, this->attributes)),
        positionXZ(new Variant(VegetationComponent::AttrPositionXZ(), Ogre::Vector2::ZERO, this->attributes)),
        scale(new Variant(VegetationComponent::AttrScale(), 1.0f, this->attributes)),
        autoOrientation(new Variant(VegetationComponent::AttrAutoOrientation(), false, this->attributes)),
        categories(new Variant(VegetationComponent::AttrCategories(), Ogre::String("All"), this->attributes)),
        maxHeight(new Variant(VegetationComponent::AttrMaxHeight(), 0.0f, this->attributes)),
        renderDistance(new Variant(VegetationComponent::AttrRenderDistance(), 30.0f, this->attributes)),
        terraLayers(new Variant(VegetationComponent::AttrTerraLayers(), Ogre::String("255,255,255,255"), this->attributes)),
        vegetationTypesCount(new Variant(VegetationComponent::AttrVegetationTypesCount(), static_cast<unsigned int>(0), this->attributes)),
        clusterSize(new Variant(VegetationComponent::AttrClusterSize(), 64.0f, this->attributes))
    {
        this->regenerate->setDescription("Generate/regenerate the vegetation.");
        this->regenerate->addUserData(GameObject::AttrActionExec());
        this->regenerate->addUserData(GameObject::AttrActionExecId(), "VegetationComponent.Regenerate");

        this->targetGameObjectId->setDescription("Target game object for planting vegetation (0 = whole world).");
        this->positionXZ->setDescription("X-Z position offset for vegetation area.");
        this->scale->setDescription("Scale of vegetation area (1.0 = whole target area).");
        this->autoOrientation->setDescription("Auto-orient vegetation to terrain slope.");
        this->maxHeight->setDescription("Maximum height for placement (0 = no limit).");
        this->renderDistance->setDescription("Render distance in meters (0 = no limit).");
        this->terraLayers->setDescription("Terra layer thresholds (e.g., 255,255,0,255).");
        this->vegetationTypesCount->setDescription("Number of vegetation mesh types.");
        this->vegetationTypesCount->addUserData(GameObject::AttrActionNeedRefresh());

        this->clusterSize->setDescription("Cluster size for spatial grouping (meters). Smaller = more precise culling but more overhead.");
    }

    VegetationComponent::~VegetationComponent(void)
    {
        // Destruction handled in onRemoveComponent
    }

    void VegetationComponent::initialise()
    {
    }

    const Ogre::String& VegetationComponent::getName() const
    {
        return this->name;
    }

    void VegetationComponent::install(const Ogre::NameValuePairList* options)
    {
        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<VegetationComponent>(VegetationComponent::getStaticClassId(), VegetationComponent::getStaticClassName());
    }

    void VegetationComponent::shutdown()
    {
        // Called too late
    }

    void VegetationComponent::uninstall()
    {
        // Called too late
    }

    void VegetationComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    bool VegetationComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetGameObjectId")
        {
            this->targetGameObjectId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data", false));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Density")
        {
            this->density->setValue(XMLConverter::getAttribReal(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PositionXZ")
        {
            this->positionXZ->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Scale")
        {
            this->scale->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AutoOrientation")
        {
            this->autoOrientation->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Categories")
        {
            this->categories->setValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxHeight")
        {
            this->maxHeight->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RenderDistance")
        {
            this->renderDistance->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TerraLayers")
        {
            this->terraLayers->setValue(XMLConverter::getAttrib(propertyElement, "data"));
            checkAndSetTerraLayers(this->terraLayers->getString());
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "VegetationTypesCount")
        {
            this->vegetationTypesCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ClusterSize")
        {
            this->clusterSize->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        // Initialize vegetation mesh name variants
        if (this->vegetationMeshNames.size() < this->vegetationTypesCount->getUInt())
        {
            this->vegetationMeshNames.resize(this->vegetationTypesCount->getUInt());
        }

        for (size_t i = 0; i < this->vegetationMeshNames.size(); i++)
        {
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "VegetationMeshName" + Ogre::StringConverter::toString(i))
            {
                Ogre::String meshName = XMLConverter::getAttrib(propertyElement, "data");
                if (nullptr == this->vegetationMeshNames[i])
                {
                    this->vegetationMeshNames[i] = new Variant(VegetationComponent::AttrVegetationMeshName() + Ogre::StringConverter::toString(i), std::vector<Ogre::String>(), this->attributes);
                    this->vegetationMeshNames[i]->setValue(meshName);
                    this->vegetationMeshNames[i]->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
                    this->vegetationMeshNames[i]->setDescription("Mesh for this vegetation type (Hlms auto-batches Items).");
                }
                else
                {
                    this->vegetationMeshNames[i]->setValue(meshName);
                }
                propertyElement = propertyElement->next_sibling("property");
            }
        }

        return true;
    }

    GameObjectCompPtr VegetationComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        // Not implemented
        return GameObjectCompPtr();
    }

    bool VegetationComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[VegetationComponent] Init component for game object: " + this->gameObjectPtr->getName());

        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &VegetationComponent::handleUpdateBounds), EventDataBoundsUpdated::getStaticEventType());
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &VegetationComponent::handleSceneParsed), NOWA::EventDataSceneParsed::getStaticEventType());

        if (nullptr == this->raySceneQuery)
        {
            this->raySceneQuery = this->gameObjectPtr->getSceneManager()->createRayQuery(Ogre::Ray(), GameObjectController::ALL_CATEGORIES_ID);
            this->raySceneQuery->setSortByDistance(true);
        }

        return true;
    }

    bool VegetationComponent::connect(void)
    {
        GameObjectComponent::connect();
        return true;
    }

    bool VegetationComponent::disconnect(void)
    {
        GameObjectComponent::disconnect();
        return true;
    }

    bool VegetationComponent::onCloned(void)
    {
        return true;
    }

    void VegetationComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();

        // Destroy vegetation on render thread (blocking to ensure cleanup completes)
        this->destroyVegetation();

        if (this->raySceneQuery)
        {
            // Destroy query on render thread
            auto* query = this->raySceneQuery;
            GraphicsModule::getInstance()->enqueueAndWait(
                [this, query]()
                {
                    this->gameObjectPtr->getSceneManager()->destroyQuery(query);
                },
                "VegetationComponent::DestroyRayQuery");
            this->raySceneQuery = nullptr;
        }

        boost::shared_ptr<EventDataRefreshGui> eventDataRefreshGui(new EventDataRefreshGui());
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshGui);

        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &VegetationComponent::handleSceneParsed), NOWA::EventDataSceneParsed::getStaticEventType());
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &VegetationComponent::handleUpdateBounds), EventDataBoundsUpdated::getStaticEventType());
    }

    void VegetationComponent::onOtherComponentRemoved(unsigned int index)
    {
        // Not needed
    }

    void VegetationComponent::onOtherComponentAdded(unsigned int index)
    {
        // Not needed
    }

    void VegetationComponent::update(Ogre::Real dt, bool notSimulating)
    {
        // Vegetation is static, no update needed
    }

    void VegetationComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (VegetationComponent::AttrTargetGameObjectId() == attribute->getName())
        {
            this->setTargetGameObjectId(attribute->getULong());
        }
        else if (VegetationComponent::AttrDensity() == attribute->getName())
        {
            this->setDensity(attribute->getReal());
        }
        else if (VegetationComponent::AttrPositionXZ() == attribute->getName())
        {
            this->setPositionXZ(attribute->getVector2());
        }
        else if (VegetationComponent::AttrScale() == attribute->getName())
        {
            this->setScale(attribute->getReal());
        }
        else if (VegetationComponent::AttrAutoOrientation() == attribute->getName())
        {
            this->setAutoOrientation(attribute->getBool());
        }
        else if (VegetationComponent::AttrCategories() == attribute->getName())
        {
            this->setCategories(attribute->getString());
        }
        else if (VegetationComponent::AttrMaxHeight() == attribute->getName())
        {
            this->setMaxHeight(attribute->getReal());
        }
        else if (VegetationComponent::AttrRenderDistance() == attribute->getName())
        {
            this->setRenderDistance(attribute->getReal());
        }
        else if (VegetationComponent::AttrTerraLayers() == attribute->getName())
        {
            this->setTerraLayers(attribute->getString());
        }
        else if (VegetationComponent::AttrVegetationTypesCount() == attribute->getName())
        {
            this->setVegetationTypesCount(attribute->getUInt());
        }
        else if (VegetationComponent::AttrClusterSize() == attribute->getName())
        {
            this->setClusterSize(attribute->getReal());
        }
        else
        {
            for (unsigned int i = 0; i < static_cast<unsigned int>(this->vegetationMeshNames.size()); i++)
            {
                if (VegetationComponent::AttrVegetationMeshName() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setVegetationMeshName(i, attribute->getString());
                }
            }
        }
    }

    void VegetationComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        GameObjectComponent::writeXML(propertiesXML, doc);

        xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "TargetGameObjectId"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->targetGameObjectId->getULong())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Density"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->density->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "PositionXZ"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->positionXZ->getVector2())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Scale"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->scale->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "AutoOrientation"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->autoOrientation->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Categories"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->categories->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "MaxHeight"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxHeight->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "RenderDistance"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->renderDistance->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "TerraLayers"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->terraLayers->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "VegetationTypesCount"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->vegetationTypesCount->getUInt())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "ClusterSize"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->clusterSize->getReal())));
        propertiesXML->append_node(propertyXML);

        for (size_t i = 0; i < this->vegetationMeshNames.size(); i++)
        {
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "VegetationMeshName" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->vegetationMeshNames[i]->getString())));
            propertiesXML->append_node(propertyXML);
        }
    }

    // ============================================================================
    // SETTERS/GETTERS
    // ============================================================================

    void VegetationComponent::setTargetGameObjectId(unsigned long targetGameObjectId)
    {
        this->targetGameObjectId->setValue(targetGameObjectId);
        this->regenerateVegetation();
    }

    unsigned long VegetationComponent::getTargetGameObjectId(void) const
    {
        return this->targetGameObjectId->getULong();
    }

    void VegetationComponent::setDensity(Ogre::Real density)
    {
        density = Ogre::Math::Clamp(density, 0.1f, 5.0f);
        this->density->setValue(density);
        this->regenerateVegetation();
    }

    Ogre::Real VegetationComponent::getDensity(void) const
    {
        return this->density->getReal();
    }

    void VegetationComponent::setPositionXZ(const Ogre::Vector2& positionXZ)
    {
        this->positionXZ->setValue(positionXZ);
        this->regenerateVegetation();
    }

    Ogre::Vector2 VegetationComponent::getPositionXZ(void) const
    {
        return this->positionXZ->getVector2();
    }

    void VegetationComponent::setScale(Ogre::Real scale)
    {
        scale = Ogre::Math::Clamp(scale, 0.1f, 100.0f);
        this->scale->setValue(scale);
        this->regenerateVegetation();
    }

    Ogre::Real VegetationComponent::getScale(void) const
    {
        return this->scale->getReal();
    }

    void VegetationComponent::setAutoOrientation(bool autoOrientation)
    {
        this->autoOrientation->setValue(autoOrientation);
        this->regenerateVegetation();
    }

    bool VegetationComponent::getAutoOrientation(void) const
    {
        return this->autoOrientation->getBool();
    }

    void VegetationComponent::setCategories(const Ogre::String& categories)
    {
        this->categories->setValue(categories);
        this->regenerateVegetation();
    }

    Ogre::String VegetationComponent::getCategories(void) const
    {
        return this->categories->getString();
    }

    void VegetationComponent::setMaxHeight(Ogre::Real maxHeight)
    {
        maxHeight = Ogre::Math::Clamp(maxHeight, 0.0f, 10000.0f);
        this->maxHeight->setValue(maxHeight);
        this->regenerateVegetation();
    }

    Ogre::Real VegetationComponent::getMaxHeight(void) const
    {
        return this->maxHeight->getReal();
    }

    void VegetationComponent::setRenderDistance(Ogre::Real renderDistance)
    {
        renderDistance = Ogre::Math::Clamp(renderDistance, 0.0f, 10000.0f);
        this->renderDistance->setValue(renderDistance);
        this->regenerateVegetation();
    }

    Ogre::Real VegetationComponent::getRenderDistance(void) const
    {
        return this->renderDistance->getReal();
    }

    void VegetationComponent::checkAndSetTerraLayers(const Ogre::String& terraLayers)
    {
        this->terraLayerList.clear();
        auto strLayers = split(terraLayers);

        if (strLayers.empty())
        {
            this->terraLayerList = {255, 255, 255, 255};
            return;
        }

        for (size_t i = 0; i < strLayers.size() && i < 4; i++)
        {
            int value = Ogre::StringConverter::parseInt(strLayers[i]);
            this->terraLayerList.emplace_back(Ogre::Math::Clamp(value, 0, 255));
        }

        while (this->terraLayerList.size() < 4)
        {
            this->terraLayerList.emplace_back(255);
        }
    }

    void VegetationComponent::setTerraLayers(const Ogre::String& terraLayers)
    {
        this->checkAndSetTerraLayers(terraLayers);
        this->terraLayers->setValue(terraLayers);
        this->regenerateVegetation();
    }

    Ogre::String VegetationComponent::getTerraLayers(void) const
    {
        return this->terraLayers->getString();
    }

    void VegetationComponent::setVegetationTypesCount(unsigned int vegetationTypesCount)
    {
        this->vegetationTypesCount->setValue(vegetationTypesCount);

        size_t oldSize = this->vegetationMeshNames.size();

        if (vegetationTypesCount > oldSize)
        {
            this->vegetationMeshNames.resize(vegetationTypesCount);

            for (size_t i = oldSize; i < vegetationTypesCount; i++)
            {
                this->vegetationMeshNames[i] = new Variant(VegetationComponent::AttrVegetationMeshName() + Ogre::StringConverter::toString(i), Ogre::String(), this->attributes);
                this->vegetationMeshNames[i]->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
                this->vegetationMeshNames[i]->setDescription("Mesh for vegetation (Hlms auto-batches).");
            }
        }
        else if (vegetationTypesCount < oldSize)
        {
            this->eraseVariants(this->vegetationMeshNames, vegetationTypesCount);
        }

        this->regenerateVegetation();
    }

    unsigned int VegetationComponent::getVegetationTypesCount(void) const
    {
        return this->vegetationTypesCount->getUInt();
    }

    void VegetationComponent::setVegetationMeshName(unsigned int index, const Ogre::String& vegetationMeshName)
    {
        if (index >= this->vegetationMeshNames.size())
        {
            return;
        }

        this->vegetationMeshNames[index]->setValue(vegetationMeshName);
        this->regenerateVegetation();
    }

    Ogre::String VegetationComponent::getVegetationMeshName(unsigned int index) const
    {
        if (index >= this->vegetationMeshNames.size())
        {
            return "";
        }
        return this->vegetationMeshNames[index]->getString();
    }

    void VegetationComponent::setClusterSize(Ogre::Real clusterSize)
    {
        clusterSize = Ogre::Math::Clamp(clusterSize, 16.0f, 256.0f);
        this->clusterSize->setValue(clusterSize);
        this->regenerateVegetation();
    }

    Ogre::Real VegetationComponent::getClusterSize(void) const
    {
        return this->clusterSize->getReal();
    }

    bool VegetationComponent::executeAction(const Ogre::String& actionId, NOWA::Variant* attribute)
    {
        if ("VegetationComponent.Regenerate" == actionId)
        {
            this->destroyVegetation();
            this->regenerateVegetation();
            return true;
        }
        return false;
    }

    // ============================================================================
    // CORE VEGETATION GENERATION (MAIN THREAD)
    // ============================================================================

    void VegetationComponent::regenerateVegetation()
    {
        if (!this->gameObjectPtr)
        {
            return;
        }

        if (GameObject::AttrCustomDataSkipCreation() == this->gameObjectPtr->getCustomDataString())
        {
            return;
        }

        // Destroy existing first
        this->destroyVegetation();

        if (this->vegetationTypesCount->getUInt() == 0)
        {
            return;
        }

        // Validate meshes
        bool hasValidMeshes = false;
        for (size_t i = 0; i < this->vegetationTypesCount->getUInt(); i++)
        {
            if (i < this->vegetationMeshNames.size() && !this->vegetationMeshNames[i]->getString().empty())
            {
                hasValidMeshes = true;
                break;
            }
        }

        if (!hasValidMeshes)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[VegetationComponent] No valid vegetation meshes defined.");
            return;
        }

        auto start = std::chrono::high_resolution_clock::now();

        // =================================================================
        // STEP 1: Calculate positions on MAIN THREAD (parallel, thread-safe)
        // =================================================================
        std::vector<VegetationComponent::VegetationBatch> batches = this->calculateVegetationPositions();

        if (batches.empty())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[VegetationComponent] No vegetation instances generated.");
            return;
        }

        // =================================================================
        // STEP 2: Send to RENDER THREAD for Ogre object creation
        // =================================================================
        this->createVegetationOnRenderThread(std::move(batches));

        auto finish = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = finish - start;

        size_t totalInstances = 0;
        for (const auto& batch : this->vegetationBatches)
        {
            totalInstances += batch.items.size();
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(this->bShowDebugData ? Ogre::LML_CRITICAL : Ogre::LML_TRIVIAL,
            "[VegetationComponent] Generated " + Ogre::StringConverter::toString(totalInstances) + " instances in " + Ogre::StringConverter::toString(this->vegetationBatches.size()) +
                " batches. Took: " + Ogre::StringConverter::toString(elapsed.count() * 0.001) + "s");
    }

    std::vector<VegetationComponent::VegetationBatch> VegetationComponent::calculateVegetationPositions()
    {
        // [SAME AS BEFORE - This part is correct and thread-safe]
        // Just remove the mesh loading part - we'll do that on render thread

        unsigned int categoryId = AppStateManager::getSingletonPtr()->getGameObjectController()->generateCategoryId(this->categories->getString());
        this->raySceneQuery->setQueryMask(categoryId);

        // Get terrain
        Ogre::Terra* terra = nullptr;
        GameObjectPtr targetGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->targetGameObjectId->getULong());

        if (targetGameObjectPtr)
        {
            auto terraCompPtr = NOWA::makeStrongPtr(targetGameObjectPtr->getComponent<TerraComponent>());
            if (terraCompPtr)
            {
                terra = terraCompPtr->getTerra();
            }
        }

        if (!terra)
        {
            auto terraList = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsFromComponent(TerraComponent::getStaticClassName());
            if (!terraList.empty())
            {
                auto terraCompPtr = NOWA::makeStrongPtr(terraList[0]->getComponent<TerraComponent>());
                if (terraCompPtr)
                {
                    terra = terraCompPtr->getTerra();
                }
            }
        }

        // Calculate bounds
        if (targetGameObjectPtr)
        {
            this->minimumBounds = targetGameObjectPtr->getMovableObject()->getWorldAabb().getMinimum();
            this->maximumBounds = targetGameObjectPtr->getMovableObject()->getWorldAabb().getMaximum();
        }

        // Initialize batches
        std::vector<VegetationComponent::VegetationBatch> batches;
        for (size_t i = 0; i < this->vegetationTypesCount->getUInt(); i++)
        {
            if (i < this->vegetationMeshNames.size() && !this->vegetationMeshNames[i]->getString().empty())
            {
                batches.emplace_back(this->vegetationMeshNames[i]->getString());
            }
        }

        if (batches.empty())
        {
            return batches;
        }

        // Grid parameters
        const Ogre::Real scaleVal = this->scale->getReal();
        const Ogre::Vector2 posXZ = this->positionXZ->getVector2();
        const Ogre::Real step = 1.0f / this->density->getReal();

        const Ogre::Real startX = this->minimumBounds.x * scaleVal + posXZ.x;
        const Ogre::Real endX = this->maximumBounds.x * scaleVal + posXZ.x;
        const Ogre::Real startZ = this->minimumBounds.z * scaleVal + posXZ.y;
        const Ogre::Real endZ = this->maximumBounds.z * scaleVal + posXZ.y;

        Ogre::Camera* camera = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
        std::atomic<size_t> totalGenerated{0};

        // Parallel position calculation
        const size_t numThreads = std::max<size_t>(1, Ogre::PlatformInformation::getNumLogicalCores());
        std::vector<std::future<std::vector<VegetationComponent::VegetationBatch>>> futures;

        for (size_t threadIdx = 0; threadIdx < numThreads; ++threadIdx)
        {
            futures.emplace_back(std::async(std::launch::async,
                [=, &totalGenerated]() -> std::vector<VegetationComponent::VegetationBatch>
                {
                    std::vector<VegetationComponent::VegetationBatch> localBatches;
                    for (const auto& batch : batches)
                    {
                        localBatches.emplace_back(batch.meshName);
                    }

                    const Ogre::Real threadStartX = startX + (threadIdx * (endX - startX) / numThreads);
                    const Ogre::Real threadEndX = startX + ((threadIdx + 1) * (endX - startX) / numThreads);

                    for (Ogre::Real x = threadStartX; x < threadEndX; x += step)
                    {
                        for (Ogre::Real z = startZ; z < endZ; z += step)
                        {
                            Ogre::Real randX = Ogre::Math::RangeRandom(-step * 0.4f, step * 0.4f);
                            Ogre::Real randZ = Ogre::Math::RangeRandom(-step * 0.4f, step * 0.4f);
                            Ogre::Vector3 objPosition(x + randX, 10000.0f, z + randZ);

                            Ogre::Ray hitRay(objPosition, Ogre::Vector3::NEGATIVE_UNIT_Y);
                            this->raySceneQuery->setRay(hitRay);

                            Ogre::Vector3 hitPoint = Ogre::Vector3::ZERO;
                            Ogre::Vector3 normal = Ogre::Vector3::ZERO;
                            Ogre::MovableObject* hitMovableObject = nullptr;
                            Ogre::Real closestDistance = 0.0f;

                            std::vector<Ogre::MovableObject*> excludeList;
                            MathHelper::getInstance()->getRaycastFromPoint(this->raySceneQuery, camera, hitPoint, (size_t&)hitMovableObject, closestDistance, normal, &excludeList);

                            if (!hitMovableObject)
                            {
                                continue;
                            }

                            objPosition.y = hitPoint.y;

                            if (this->maxHeight->getReal() > 0.0f && hitPoint.y > this->maxHeight->getReal())
                            {
                                continue;
                            }

                            if (terra)
                            {
                                std::vector<int> layers = terra->getLayerAt(objPosition);
                                bool layerMatches = true;

                                for (size_t i = 0; i < std::min(layers.size(), this->terraLayerList.size()); i++)
                                {
                                    layerMatches &= (layers[i] <= this->terraLayerList[i]);
                                }

                                if (!layerMatches)
                                {
                                    continue;
                                }
                            }

                            int rndBatchIdx = Ogre::Math::RangeRandom(0, static_cast<int>(localBatches.size()) - 1);

                            Ogre::Real scaleRand = Ogre::Math::RangeRandom(0.5f, 1.2f);
                            Ogre::Vector3 scaleVec(scaleRand, scaleRand, scaleRand);

                            Ogre::Quaternion orientation = Ogre::Quaternion::IDENTITY;

                            if (this->autoOrientation->getBool())
                            {
                                Ogre::Vector3 right = normal.perpendicular();
                                Ogre::Vector3 forward = right.crossProduct(normal);
                                orientation.FromAxes(right, normal, forward);
                            }

                            Ogre::Degree randAngle(Ogre::Math::RangeRandom(0.0f, 360.0f));
                            Ogre::Quaternion randRot;
                            randRot.FromAngleAxis(randAngle, Ogre::Vector3::UNIT_Y);
                            orientation = orientation * randRot;

                            localBatches[rndBatchIdx].instances.emplace_back(objPosition, orientation, scaleVec);
                            totalGenerated++;
                        }
                    }

                    return localBatches;
                }));
        }

        // Merge results
        for (auto& future : futures)
        {
            std::vector<VegetationComponent::VegetationBatch> threadBatches = future.get();

            for (size_t i = 0; i < batches.size(); i++)
            {
                batches[i].instances.insert(batches[i].instances.end(), threadBatches[i].instances.begin(), threadBatches[i].instances.end());
            }
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[VegetationComponent] Calculated " + Ogre::StringConverter::toString(totalGenerated) + " positions.");

        return batches;
    }

    // ============================================================================
    // CREATE OGRE-NEXT ITEMS ON RENDER THREAD
    // ============================================================================

    void VegetationComponent::createVegetationOnRenderThread(std::vector<VegetationComponent::VegetationBatch>&& batches)
    {
        // Queue to render thread using your GraphicsModule
        // Capture batches by value (move) to ensure lifetime
        GraphicsModule::getInstance()->enqueue(
            [this, batchesCopy = std::move(batches)]() mutable
            {
                this->createVegetationItems(batchesCopy);
            },
            "VegetationComponent::CreateItems");
    }

    void VegetationComponent::createVegetationItems(std::vector<VegetationComponent::VegetationBatch>& batches)
    {
        //  THIS MUST RUN ON RENDER THREAD!

        Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();
        unsigned int queryFlags = AppStateManager::getSingletonPtr()->getGameObjectController()->generateCategoryId(this->gameObjectPtr->getCategory());

        GameObjectPtr targetGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->targetGameObjectId->getULong());

        // Get or create shared datablock for batching
        Ogre::Hlms* hlmsPbs = Ogre::Root::getSingletonPtr()->getHlmsManager()->getHlms(Ogre::HLMS_PBS);
        if (!hlmsPbs)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[VegetationComponent] No PBS Hlms available!");
            return;
        }

        for (auto& batch : batches)
        {
            if (batch.instances.empty())
            {
                continue;
            }

            // Load V2 mesh
            Ogre::MeshPtr meshPtr;
            try
            {
                // In Ogre-Next, you directly load V2 meshes
                meshPtr = Ogre::MeshManager::getSingletonPtr()->load(batch.meshName, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
            }
            catch (const Ogre::Exception& e)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[VegetationComponent] Failed to load mesh '" + batch.meshName + "': " + e.getDescription());
                continue;
            }

            // Create ONE shared datablock for all instances of this mesh
            // Hlms will automatically batch Items that share the same datablock
            Ogre::String datablockName = "VegetationDatablock_" + batch.meshName + "_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());

            Ogre::HlmsDatablock* datablock = hlmsPbs->getDatablock(datablockName);
            if (!datablock)
            {
                datablock = hlmsPbs->createDatablock(datablockName, datablockName, Ogre::HlmsMacroblock(), Ogre::HlmsBlendblock(), Ogre::HlmsParamVec());
            }

            // =================================================================
            // CREATE ITEMS - Ogre-Next auto-batches them via Hlms!
            // =================================================================
            for (const auto& instance : batch.instances)
            {
                // Create V2 Item (not Entity!)
                Ogre::Item* item = sceneManager->createItem(meshPtr, Ogre::SCENE_STATIC);

                if (!item)
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[VegetationComponent] Failed to create Item");
                    continue;
                }

                // Set the shared datablock - THIS IS KEY FOR BATCHING!
                item->setDatablock(datablock);

                item->setQueryFlags(queryFlags);
                item->setCastShadows(false); // Performance

                if (this->renderDistance->getReal() > 0.0f)
                {
                    item->setRenderingDistance(this->renderDistance->getReal());
                }

                // Create scene node
                Ogre::SceneNode* node = nullptr;
                if (targetGameObjectPtr)
                {
                    node = targetGameObjectPtr->getSceneNode()->createChildSceneNode(Ogre::SCENE_STATIC);
                }
                else
                {
                    node = sceneManager->getRootSceneNode(Ogre::SCENE_STATIC)->createChildSceneNode(Ogre::SCENE_STATIC);
                }

                node->setPosition(instance.position);
                node->setOrientation(instance.orientation);
                node->setScale(instance.scale);
                node->attachObject(item);

                batch.items.push_back(item);
                batch.nodes.push_back(node);
            }

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[VegetationComponent] Created " + Ogre::StringConverter::toString(batch.items.size()) + " Items for mesh: " + batch.meshName + " (Hlms will auto-batch them)");
        }

        // Store batches for cleanup
        this->vegetationBatches = std::move(batches);
    }

    void VegetationComponent::destroyVegetation()
    {
        // Queue destruction to render thread (blocking to ensure cleanup)
        GraphicsModule::getInstance()->enqueueAndWait(
            [this]()
            {
                this->destroyVegetationOnRenderThread();
            },
            "VegetationComponent::DestroyItems");
    }

    void VegetationComponent::destroyVegetationOnRenderThread()
    {
        //  THIS MUST RUN ON RENDER THREAD!

        Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();

        for (auto& batch : this->vegetationBatches)
        {
            // Destroy all items and nodes
            for (size_t i = 0; i < batch.items.size(); i++)
            {
                Ogre::Item* item = batch.items[i];
                Ogre::SceneNode* node = batch.nodes[i];

                if (node)
                {
                    node->detachAllObjects();
                    sceneManager->destroySceneNode(node);
                }

                if (item)
                {
                    sceneManager->destroyItem(item);
                }
            }

            batch.items.clear();
            batch.nodes.clear();
        }

        this->vegetationBatches.clear();
    }

    // ============================================================================
    // EVENT HANDLERS
    // ============================================================================

    void VegetationComponent::handleUpdateBounds(NOWA::EventDataPtr eventData)
    {
        boost::shared_ptr<NOWA::EventDataBoundsUpdated> castEventData = boost::static_pointer_cast<EventDataBoundsUpdated>(eventData);

        this->minimumBounds = castEventData->getCalculatedBounds().first;
        this->maximumBounds = castEventData->getCalculatedBounds().second;
    }

    void VegetationComponent::handleSceneParsed(NOWA::EventDataPtr eventData)
    {
        this->minimumBounds = Core::getSingletonPtr()->getCurrentSceneBoundLeftNear();
        this->maximumBounds = Core::getSingletonPtr()->getCurrentSceneBoundRightFar();
        this->regenerateVegetation();
    }

    void VegetationComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
    {
        // Lua bindings
    }

    bool VegetationComponent::canStaticAddComponent(GameObject* gameObject)
    {
        return (gameObject->getId() == GameObjectController::MAIN_GAMEOBJECT_ID);
    }

}; // namespace end

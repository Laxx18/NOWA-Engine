#include "NOWAPrecompiled.h"
#include "PlanetTerraComponent.h"
#include "gameobject/GameObjectController.h"
#include "gameobject/GameObjectFactory.h"
#include "gameobject/DatablockPbsComponent.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "main/InputDeviceCore.h"
#include "modules/LuaScriptApi.h"
#include "utilities/MathHelper.h"
#include "utilities/XMLConverter.h"
#include "editor/EditorManager.h"

#include "OgreAbiUtils.h"
#include "OgreImage2.h"
#include "OgreLogManager.h"
#include "OgreMesh2Serializer.h"

#include <filesystem>
#include <fstream>
#include <system_error>

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    // =========================================================================
    //  Constructor / Destructor
    // =========================================================================

    PlanetTerraComponent::PlanetTerraComponent()
        : PlanetTerraComponentBase(),
        name("PlanetTerraComponent"),
        hasModifiedData(false)
    {
        this->activated = new Variant(PlanetTerraComponent::AttrActivated(), true, this->attributes);
        this->radius = new Variant(PlanetTerraComponent::AttrRadius(), 50.0f, this->attributes);
        this->segmentsH = new Variant(PlanetTerraComponent::AttrSegmentsH(), static_cast<unsigned int>(64), this->attributes);
        this->segmentsV = new Variant(PlanetTerraComponent::AttrSegmentsV(), static_cast<unsigned int>(64), this->attributes);
        this->blendTexSize = new Variant(PlanetTerraComponent::AttrBlendTexSize(), static_cast<unsigned int>(1024), this->attributes);
        this->datablock = new Variant(PlanetTerraComponent::AttrDatablock(), PlanetTerraComponent::DefaultMaterialName(), this->attributes);
        this->editMode = new Variant(PlanetTerraComponent::AttrEditMode(), std::vector<Ogre::String>{"Sculpt", "Paint"}, this->attributes);
        this->brushMode = new Variant(PlanetTerraComponent::AttrBrushMode(), std::vector<Ogre::String>{"Pull", "Push", "Smooth", "Flatten", "Inflate"}, this->attributes);
        this->paintLayer = new Variant(PlanetTerraComponent::AttrPaintLayer(), std::vector<Ogre::String>{"0", "1", "2", "3"}, this->attributes);
        // Default to layer 1: layer 0 is the base texture (always active via blend R=1).
        // User paints layer 1 (G channel) onto the base.
        this->paintLayer->setListSelectedValue("1");
        this->brushName = new Variant(PlanetTerraComponent::AttrBrushName(), this->attributes);
        this->brushSize = new Variant(PlanetTerraComponent::AttrBrushSize(), 30.0f, this->attributes);
        this->brushIntensity = new Variant(PlanetTerraComponent::AttrBrushIntensity(), 0.5f, this->attributes);
        this->brushFalloff = new Variant(PlanetTerraComponent::AttrBrushFalloff(), 2.0f, this->attributes);
        this->bakeMesh = new Variant(PlanetTerraComponent::AttrBakeMesh(), Ogre::String("Bake Mesh"), this->attributes);
        this->activated->setDescription("Enable / disable planet edit mode.");
        this->radius->setDescription("Planet radius in world units.");
        this->segmentsH->setDescription("Horizontal (longitude) tessellation. Higher = more detail. Must be >= 4.");
        this->segmentsV->setDescription("Vertical (latitude) tessellation. Higher = more detail. Must be >= 4.");
        this->blendTexSize->setDescription("Blend-weight texture resolution (square, RGBA8).\n"
                                     "R = detail layer 0 weight, G = layer 1, B = layer 2, A = layer 3.\n"
                                     "Wired automatically to PBSM_DETAIL_WEIGHT on the sibling DatablockPbsComponent.\n"
                                     "DatablockPbsComponent starts with PlanetTerraDefaultMaterial (4 terrain layers). "
                                     "Change detail0..3 textures there to customize terrain appearance.");
        this->editMode->setDescription("Sculpt: move vertices with the brush. Paint: paint terrain layer blend weights.");
        this->brushMode->setDescription("Pull=raise, Push=lower, Smooth=average, Flatten=level, Inflate=puff outward.");
        this->paintLayer->setDescription("Which blend texture channel the paint brush writes to:\n"
                                   "  0 = R → detail0 (base terrain, active everywhere by default)\n"
                                   "  1 = G → detail1\n"
                                   "  2 = B → detail2\n"
                                   "  3 = A → detail3");
        this->brushName->setDescription("Brush mask image. White = full strength, black = no effect.");
        this->brushSize->setDescription("Brush radius:\n  Sculpt mode: world units.\n  Paint mode: blend-texture pixels.");
        this->brushIntensity->setDescription("Brush strength per stroke application (0..1).");
        this->brushFalloff->setDescription("Falloff exponent. Higher = sharper edge.");
        this->bakeMesh->setDescription("Exports the current planet to a static .mesh file in the scene folder "
                                 "and generates LOD levels. Baking does NOT remove this component.");
        this->bakeMesh->addUserData(GameObject::AttrActionExec());
        this->bakeMesh->addUserData(GameObject::AttrActionNeedRefresh());
        this->bakeMesh->addUserData(GameObject::AttrActionExecId(), PlanetTerraComponent::ActionBakeMesh());

        this->detailUVScale = new Variant(PlanetTerraComponent::AttrDetailUVScale(), 128.0f, this->attributes);
        this->detailUVScale->setDescription("UV tiling scale for all detail texture layers (diffuse + normal maps). "
                                      "Higher = more tiling = smaller apparent texture on the sphere. "
                                      "Formula: sphere_circumference / desired_tile_size_in_metres. "
                                      "Radius 50 (circumference ~314m): 128 = ~2.5m per tile, 64 = ~5m per tile.");

        this->baseUVScale = new Variant(PlanetTerraComponent::AttrBaseUVScale(), 128.0f, this->attributes);
        this->baseUVScale->setDescription("UV tiling scale for the main diffuse and normal texture (UV set 1). "
                                    "1 = tiles once across the whole sphere (good from-orbit look). "
                                    "Set 64 for a radius-50 planet to tile every ~5m for close-up base terrain. "
                                    "Changing this triggers a mesh rebuild.");

        // ── Detail NM textures ────────────────────────────────────────────────
        // These are NOT loaded via the material JSON (doing so causes @piece blocks
        // to be emitted literally as HLSL code → shader crash). They are set here
        // as configurable Variants and applied programmatically in wireBlend.
        this->detail0NMTextureName = new Variant(PlanetTerraComponent::AttrDetail0NMTextureName(), Ogre::String("adesert_cracks_n.dds"), this->attributes);
        this->detail0NMTextureName->setDescription("BC5_SNORM DDS normal map for detail layer 0.");
        this->addAttributeFilePathData(detail0NMTextureName);

        this->detail1NMTextureName = new Variant(PlanetTerraComponent::AttrDetail1NMTextureName(), Ogre::String("grass_green_n.dds"), this->attributes);
        this->detail1NMTextureName->setDescription("BC5_SNORM DDS normal map for detail layer 1.");
        this->addAttributeFilePathData(detail1NMTextureName);

        this->detail2NMTextureName = new Variant(PlanetTerraComponent::AttrDetail2NMTextureName(), Ogre::String("island_sand_n.dds"), this->attributes);
        this->detail2NMTextureName->setDescription("BC5_SNORM DDS normal map for detail layer 2.");
        this->addAttributeFilePathData(detail2NMTextureName);

        this->detail3NMTextureName = new Variant(PlanetTerraComponent::AttrDetail3NMTextureName(), Ogre::String("mntn_black_n.dds"), this->attributes);
        this->detail3NMTextureName->setDescription("BC5_SNORM DDS normal map for detail layer 3.");
        this->addAttributeFilePathData(detail3NMTextureName);

        // ── Constraints ───────────────────────────────────────────────────────
        this->radius->setConstraints(10.0f, 50000.0f);
        this->segmentsH->setConstraints(4u, 512u);
        this->segmentsV->setConstraints(4u, 512u);
        this->blendTexSize->setConstraints(0u, 4096u);
        this->detailUVScale->setConstraints(1.0f, 1024.0f);
        this->baseUVScale->setConstraints(0.1f, 512.0f);
        this->brushSize->setConstraints(1.0f, 2000.0f);
        this->brushIntensity->setConstraints(0.001f, 1.0f);
        this->brushFalloff->setConstraints(0.1f, 10.0f);
    }

    PlanetTerraComponent::~PlanetTerraComponent()
    {
    }

    void PlanetTerraComponent::install(const Ogre::NameValuePairList* options)
    {
        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<PlanetTerraComponent>(PlanetTerraComponent::getStaticClassId(), PlanetTerraComponent::getStaticClassName());
    }

    void PlanetTerraComponent::initialise()
    {
    }
    void PlanetTerraComponent::shutdown()
    {
    }
    void PlanetTerraComponent::uninstall()
    {
    }

    void PlanetTerraComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    const Ogre::String& PlanetTerraComponent::getName() const
    {
        return this->name;
    }

    Ogre::String PlanetTerraComponent::getClassName() const
    {
        return "PlanetTerraComponent";
    }

    Ogre::String PlanetTerraComponent::getParentClassName() const
    {
        return "GameObjectComponent";
    }

    bool PlanetTerraComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrActivated())
        {
            activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrRadius())
        {
            radius->setValue(XMLConverter::getAttribReal(propertyElement, "data", 500.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrSegmentsH())
        {
            segmentsH->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data", 64u));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrSegmentsV())
        {
            segmentsV->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data", 64u));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrBlendTexSize())
        {
            blendTexSize->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data", 1024u));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrBrushName())
        {
            brushName->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrBrushSize())
        {
            brushSize->setValue(XMLConverter::getAttribReal(propertyElement, "data", 30.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrBrushIntensity())
        {
            brushIntensity->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.5f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrBrushFalloff())
        {
            brushFalloff->setValue(XMLConverter::getAttribReal(propertyElement, "data", 2.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrDetailUVScale())
        {
            detailUVScale->setValue(XMLConverter::getAttribReal(propertyElement, "data", 128.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AttrBaseUVScale())
        {
            baseUVScale->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
            propertyElement = propertyElement->next_sibling("property");
        }

        this->planetLoadedFromFile = true;
        return true;
    }

    bool PlanetTerraComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlanetTerraComponent] Init for game object: " + this->gameObjectPtr->getName());

        {
            Ogre::StringVectorPtr brushNames = Ogre::ResourceGroupManager::getSingleton().findResourceNames("Brushes", "*.png");
            std::vector<Ogre::String> brushList;
            if (brushNames.get())
            {
                for (const auto& f : *brushNames)
                {
                    brushList.push_back(f);
                }
            }
            if (!brushList.empty())
            {
                brushName->setValue(brushList);
                brushName->setListSelectedValue(brushList[0]);
            }
            brushName->addUserData(GameObject::AttrActionImage());
        }

        // Scene-level raycast query for mouse picking
        rayQuery = this->gameObjectPtr->getSceneManager()->createRayQuery(Ogre::Ray(), GameObjectController::ALL_CATEGORIES_ID);
        rayQuery->setSortByDistance(true);

        // Subscribe to editor events
        AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &PlanetTerraComponent::handleNewComponent), EventDataNewComponent::getStaticEventType());
        AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &PlanetTerraComponent::handleMeshModifyMode), EventDataEditorMode::getStaticEventType());
        AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &PlanetTerraComponent::handleGameObjectSelected), EventDataGameObjectSelected::getStaticEventType());
        AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &PlanetTerraComponent::handleComponentManuallyDeleted), EventDataDeleteComponent::getStaticEventType());

        // Load brush image if one was saved
        this->loadBrushImage(this->brushName->getListSelectedValue());

        // Attempt to restore sculpt/paint data from the binary sidecar file
        if (this->planetLoadedFromFile)
        {
            this->loadPlanetDataFromFile();
        }

        this->postInitDone = true;
        this->createPlanet();

        this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
        this->gameObjectPtr->getAttribute(GameObject::AttrCastShadows())->setVisible(false);
        this->gameObjectPtr->getAttribute(GameObject::AttrClampY())->setVisible(false);

        // Subscribe to EventDataNewComponent so we are notified the moment
        // DatablockPbsComponent finishes its own postInit(). At that point all
        // Ogre state is valid and we can safely call wireBlendTextureToPbsDatablock()
        // from the main thread via enqueueAndWait. This avoids the render-thread
        // deadlock that occurred when calling createComponent from a tracked closure.
        // The listener unsubscribes itself after the first matching event.
        AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &PlanetTerraComponent::handleNewComponent), EventDataNewComponent::getStaticEventType());

        return true;
    }

    bool PlanetTerraComponent::connect(void)
    {
        this->wireBlendTextureToPbsDatablock();

        return true;
    }

    bool PlanetTerraComponent::disconnect(void)
    {
        removeInputListener();
        return true;
    }

    bool PlanetTerraComponent::onCloned(void)
    {
        return true;
    }
    void PlanetTerraComponent::onAddComponent(void)
    {
    }

    void PlanetTerraComponent::onRemoveComponent(void)
    {
        this->removeInputListener();

        // Clean up all event subscriptions
        AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &PlanetTerraComponent::handleNewComponent), EventDataNewComponent::getStaticEventType());
        AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &PlanetTerraComponent::handleMeshModifyMode), EventDataEditorMode::getStaticEventType());
        AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &PlanetTerraComponent::handleGameObjectSelected), EventDataGameObjectSelected::getStaticEventType());
        AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &PlanetTerraComponent::handleComponentManuallyDeleted), EventDataDeleteComponent::getStaticEventType());

        if (this->rayQuery)
        {
            this->gameObjectPtr->getSceneManager()->destroyQuery(this->rayQuery);
            this->rayQuery = nullptr;
        }

        this->destroyPlanet();
    }

    void PlanetTerraComponent::onOtherComponentRemoved(unsigned int)
    {
    }
    void PlanetTerraComponent::onOtherComponentAdded(unsigned int)
    {
    }

    // =========================================================================
    //  clone
    // =========================================================================

    GameObjectCompPtr PlanetTerraComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        PlanetTerraComponentPtr cloned(boost::make_shared<PlanetTerraComponent>());
        cloned->setRadius(radius->getReal());
        cloned->setSegmentsH(segmentsH->getUInt());
        cloned->setSegmentsV(segmentsV->getUInt());
        cloned->setBlendTexSize(blendTexSize->getUInt());
        cloned->setBrushSize(brushSize->getReal());
        cloned->setBrushIntensity(brushIntensity->getReal());
        cloned->setBrushFalloff(brushFalloff->getReal());

        clonedGameObjectPtr->addComponent(cloned);
        cloned->setOwner(clonedGameObjectPtr);

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(cloned));
        return cloned;
    }

    void PlanetTerraComponent::update(Ogre::Real dt, bool notSimulating)
    {
        // PlanetTerra has no per-frame GPU update (unlike Terra's shadow mapper).
    }

    bool PlanetTerraComponent::canStaticAddComponent(GameObject* gameObject)
    {
        auto thisCompPtr = NOWA::makeStrongPtr(gameObject->getComponent<PlanetTerraComponent>());
        if (nullptr == thisCompPtr)
        {
            return true;
        }
        return false;
    }

    std::optional<NOWA::GameObjectTypeDescriptor> PlanetTerraComponent::getStaticTypeDescriptor()
    {
        NOWA::GameObjectTypeDescriptor desc;
        desc.type = eType::CUSTOM;
        desc.displayName = "Planet Terra";
        desc.meshToDisplay = "Node.mesh";
        desc.needsMeshItem = false;
        desc.forceZeroTransform = false;
        desc.autoComponents = {PlanetTerraComponent::getStaticClassName(), DatablockPbsComponent::getStaticClassName()};
        return desc;
    }

    void PlanetTerraComponent::writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc)
    {
        GameObjectComponent::writeXML(propertiesXML, doc);

        auto addProp = [&](const Ogre::String& propName, const Ogre::String& typeStr, const Ogre::String& valueStr)
        {
            xml_node<>* node = doc.allocate_node(node_element, "property");
            node->append_attribute(doc.allocate_attribute("type", doc.allocate_string(typeStr.c_str())));
            node->append_attribute(doc.allocate_attribute("name", doc.allocate_string(propName.c_str())));
            node->append_attribute(doc.allocate_attribute("data", doc.allocate_string(valueStr.c_str())));
            propertiesXML->append_node(node);
        };

        addProp(AttrActivated(), "12", XMLConverter::ConvertString(doc, activated->getBool()));
        addProp(AttrRadius(), "6", XMLConverter::ConvertString(doc, radius->getReal()));
        addProp(AttrSegmentsH(), "2", XMLConverter::ConvertString(doc, segmentsH->getUInt()));
        addProp(AttrSegmentsV(), "2", XMLConverter::ConvertString(doc, segmentsV->getUInt()));
        addProp(AttrBlendTexSize(), "2", XMLConverter::ConvertString(doc, blendTexSize->getUInt()));
        addProp(AttrBrushName(), "7", brushName->getListSelectedValue());
        addProp(AttrBrushSize(), "6", XMLConverter::ConvertString(doc, brushSize->getReal()));
        addProp(AttrBrushIntensity(), "6", XMLConverter::ConvertString(doc, brushIntensity->getReal()));
        addProp(AttrBrushFalloff(), "6", XMLConverter::ConvertString(doc, brushFalloff->getReal()));
        addProp(AttrDetailUVScale(), "6", XMLConverter::ConvertString(doc, detailUVScale->getReal()));
        addProp(AttrBaseUVScale(), "6", XMLConverter::ConvertString(doc, baseUVScale->getReal()));

        // Save the heavy geometry data to the binary sidecar
        this->savePlanetDataToFile();

        // Save blend weight data as PNG from the CPU blendData array.
        // The CPU array is always authoritative - all paint strokes write there first.
        if (nullptr != this->planet)
        {
            const Ogre::String blendFilePath = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "/" + this->planet->getBlendTextureName() + ".png";
            this->planet->saveBlendDataToFile(blendFilePath);
        }
    }

    void PlanetTerraComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (AttrActivated() == attribute->getName())
        {
            setActivated(attribute->getBool());
        }
        else if (AttrRadius() == attribute->getName())
        {
            setRadius(attribute->getReal());
        }
        else if (AttrSegmentsH() == attribute->getName())
        {
            setSegmentsH(attribute->getUInt());
        }
        else if (AttrSegmentsV() == attribute->getName())
        {
            setSegmentsV(attribute->getUInt());
        }
        else if (AttrBlendTexSize() == attribute->getName())
        {
            setBlendTexSize(attribute->getUInt());
        }
        else if (AttrEditMode() == attribute->getName())
        {
            setEditMode(attribute->getListSelectedValue());
        }
        else if (AttrBrushMode() == attribute->getName())
        {
            setBrushMode(attribute->getListSelectedValue());
        }
        else if (AttrPaintLayer() == attribute->getName())
        {
            setPaintLayer(attribute->getListSelectedValue());
        }
        else if (AttrBrushName() == attribute->getName())
        {
            setBrushName(attribute->getListSelectedValue());
        }
        else if (AttrBrushSize() == attribute->getName())
        {
            brushSize->setValue(attribute->getReal());
        }
        else if (AttrBrushIntensity() == attribute->getName())
        {
            brushIntensity->setValue(attribute->getReal());
        }
        else if (AttrBrushFalloff() == attribute->getName())
        {
            brushFalloff->setValue(attribute->getReal());
        }
        else if (AttrDetailUVScale() == attribute->getName())
        {
            this->setDetailUVScale(attribute->getReal());
        }
        else if (AttrBaseUVScale() == attribute->getName())
        {
            this->setBaseUVScale(attribute->getReal());
        }
    }

    bool PlanetTerraComponent::executeAction(const Ogre::String& actionId, NOWA::Variant* attribute)
    {
        if (ActionBakeMesh() == actionId)
        {
            return this->bakeMeshToFile();
        }
        return true;
    }

    // =========================================================================
    //  Planet lifecycle
    // =========================================================================

    void PlanetTerraComponent::createPlanet(void)
    {
        if (this->planet || !this->postInitDone)
        {
            return;
        }

        this->planet = new PlanetTerra(this->gameObjectPtr->getName(), this->gameObjectPtr->getSceneManager());

        // Apply baseUVScale before create() so UV1 is baked correctly into vertex data
        this->planet->setBaseUVScale(this->baseUVScale->getReal());

        NOWA::GraphicsModule::RenderCommand cmd = [this]()
        {
            const float r = this->radius->getReal();
            const int segH = static_cast<int>(this->segmentsH->getUInt());
            const int segV = static_cast<int>(this->segmentsV->getUInt());
            const int bts = static_cast<int>(this->blendTexSize->getUInt());

            const float lodDist = static_cast<float>(this->gameObjectPtr->getLodDistance());
            const float renderDist = static_cast<float>(this->gameObjectPtr->getRenderDistance());
            const float effectiveLodDist = (lodDist > 0.0f) ? lodDist : (renderDist > 0.0f) ? renderDist : this->radius->getReal() * 4.0f;

            // useBlendWeight: true if 0u != this->blendTexSize->getUInt()
            if (!this->planet->create(r, segH, segV, bts, this->gameObjectPtr->getSceneNode(), PlanetTerraComponent::DefaultMaterialName(), effectiveLodDist, 0u != this->blendTexSize->getUInt()))
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PlanetTerraComponent] Planet creation failed for: " + this->gameObjectPtr->getName());
                return;
            }

            // internally weight texture is set
            this->planet->setDatablockByName(PlanetTerraComponent::DefaultMaterialName());

            // Prevent the planet Item from being caught by the editor's rubber-band
            // selection box. The planet is picked only via our explicit ray cast in
            // mousePressed, not through the generic editor selection mechanism.
            // this->planet->getItem()->setQueryFlags(0u);

            // Restore sculpt / paint data if loaded from file
            if (false == this->loadedHeightData.empty())
            {
                this->planet->restoreHeightData(this->loadedHeightData);
                this->planet->restoreBlendData(this->loadedBlendData.empty() ? std::vector<uint8_t>() : this->loadedBlendData);
                this->loadedHeightData.clear();
                this->loadedBlendData.clear();

                this->planet->recalculateNormals();
                this->planet->recalculateTangents();
                this->planet->uploadVertexData();
                this->planet->uploadBlendData();
            }

            this->gameObjectPtr->setDoNotDestroyMovableObject(true);
            this->gameObjectPtr->init(this->planet->getItem());
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "PlanetTerraComponent::createPlanet");
    }

    void PlanetTerraComponent::destroyPlanet(void)
    {
        if (nullptr == this->planet)
        {
            return;
        }

        auto* p = this->planet;
        planet = nullptr;

        NOWA::GraphicsModule::RenderCommand cmd = [p, this]()
        {
            p->destroy();
            delete p;

            this->gameObjectPtr->nullMovableObject();
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "PlanetTerraComponent::destroyPlanet");
    }

    // =========================================================================
    //  Input listener registration (render thread required by NOWA InputDeviceCore)
    // =========================================================================

    void PlanetTerraComponent::addInputListener(void)
    {
        const Ogre::String id = getStaticClassName() + "_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
        NOWA::GraphicsModule::RenderCommand cmd = [this, id]()
        {
            if (auto* core = InputDeviceCore::getSingletonPtr())
            {
                core->addKeyListener(this, id);
                core->addMouseListener(this, id);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "PlanetTerraComponent::addInputListener");
    }

    void PlanetTerraComponent::removeInputListener(void)
    {
        const Ogre::String id = getStaticClassName() + "_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
        NOWA::GraphicsModule::RenderCommand cmd = [id]()
        {
            if (auto* core = InputDeviceCore::getSingletonPtr())
            {
                core->removeKeyListener(id);
                core->removeMouseListener(id);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "PlanetTerraComponent::removeInputListener");
    }

    // =========================================================================
    //  OIS key handlers
    // =========================================================================

    bool PlanetTerraComponent::keyPressed(const OIS::KeyEvent& evt)
    {
        if (!activated->getBool())
        {
            return true;
        }
        if (evt.key == OIS::KC_LSHIFT || evt.key == OIS::KC_RSHIFT)
        {
            isShiftPressed = true;
        }
        return true;
    }

    bool PlanetTerraComponent::keyReleased(const OIS::KeyEvent& evt)
    {
        if (evt.key == OIS::KC_LSHIFT || evt.key == OIS::KC_RSHIFT)
        {
            isShiftPressed = false;
        }
        return true;
    }

    // =========================================================================
    //  OIS mouse handlers
    // =========================================================================

    bool PlanetTerraComponent::mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
    {
        if (!activated->getBool() || !isEditorMeshModifyMode || !isSelected)
        {
            return true;
        }
        if (id != OIS::MB_Left)
        {
            return true;
        }
        if (NOWA::GraphicsModule::getInstance()->getMyGUIFocusWidget())
        {
            return true;
        }
        if (!this->planet)
        {
            return true;
        }

        Ogre::Camera* cam = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
        if (!cam)
        {
            return true;
        }

        // Build the ray from screen coordinates
        const float screenX = static_cast<float>(evt.state.X.abs) / static_cast<float>(evt.state.width);
        const float screenY = static_cast<float>(evt.state.Y.abs) / static_cast<float>(evt.state.height);
        const Ogre::Ray worldRay = cam->getCameraToViewportRay(screenX, screenY);

        // Direct triangle raycast against the planet mesh CPU data.
        // The planet item has queryFlags=0 to avoid editor selection rectangle —
        // so we cannot use the scene query system here.
        Ogre::Vector3 hitPos = Ogre::Vector3::ZERO;
        Ogre::Vector3 hitNorm = Ogre::Vector3::UNIT_Y;
        bool hit = false;
        float bestDist = std::numeric_limits<float>::max();

        const Ogre::Matrix4 nodeInv = this->gameObjectPtr->getSceneNode()->_getFullTransform().inverse();
        Ogre::Matrix3 nodeInv3;
        nodeInv.extract3x3Matrix(nodeInv3);

        const Ogre::Ray localRay(nodeInv * worldRay.getOrigin(), (nodeInv3 * worldRay.getDirection()).normalisedCopy());

        const std::vector<Ogre::Vector3>& verts = this->planet->getVertices();
        const std::vector<Ogre::uint32>& idxs = this->planet->getIndices();
        const size_t idxCount = this->planet->getIndexCount();

        for (size_t i = 0u; i < idxCount; i += 3u)
        {
            const Ogre::Vector3& v0 = verts[idxs[i]];
            const Ogre::Vector3& v1 = verts[idxs[i + 1]];
            const Ogre::Vector3& v2 = verts[idxs[i + 2]];

            std::pair<bool, Ogre::Real> r = Ogre::Math::intersects(localRay, v0, v1, v2, true, false);
            if (r.first && r.second < bestDist)
            {
                bestDist = r.second;
                hitPos = this->gameObjectPtr->getSceneNode()->_getFullTransform() * localRay.getPoint(r.second);
                Ogre::Vector3 fn = (v1 - v0).crossProduct(v2 - v0).normalisedCopy();
                hitNorm = (nodeInv3.Transpose() * fn).normalisedCopy();
                hit = true;
            }
        }

        if (hit)
        {
            this->isPressing = true;
            this->brushStrokeActive = false;
            this->brushStrokePreEditData = this->getPlanetData();

            if (this->editMode->getListSelectedValue() == "Sculpt")
            {
                this->doApplySculptBrush(hitPos, hitNorm);
            }
            else
            {
                // Compute equirectangular UV matching the sphere vertex UV generation exactly.
                // Sphere vertex UV: u = h/segH = theta/(2PI) where theta goes 0..2PI.
                // atan2 gives angle in (-PI, PI] so we wrap negatives to get 0..1.
                const Ogre::Vector3 localHit = nodeInv * hitPos;
                const Ogre::Vector3 dir = localHit.normalisedCopy();

                float u = std::atan2(dir.z, dir.x) / (2.0f * Ogre::Math::PI);
                if (u < 0.0f)
                {
                    u += 1.0f;
                }
                // v: phi = acos(dir.y), v = phi/PI. Same as 0.5 - asin(dir.y)/PI.
                const float v = 0.5f - std::asin(Ogre::Math::Clamp(dir.y, -1.0f, 1.0f)) / Ogre::Math::PI;

                this->doApplyPaintBrush(Ogre::Vector2(u, v));
            }
        }
        else
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlanetTerraComponent] mousePressed: no triangle hit on planet '" + this->gameObjectPtr->getName() + "'.");
        }

        return false;
    }

    bool PlanetTerraComponent::mouseMoved(const OIS::MouseEvent& evt)
    {
        if (!this->activated->getBool() || !this->isEditorMeshModifyMode || !this->isSelected)
        {
            return true;
        }
        if (!this->isPressing || !this->planet)
        {
            return true;
        }
        if (NOWA::GraphicsModule::getInstance()->getMyGUIFocusWidget())
        {
            return true;
        }

        Ogre::Camera* cam = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
        if (!cam)
        {
            return true;
        }

        const float screenX = static_cast<float>(evt.state.X.abs) / static_cast<float>(evt.state.width);
        const float screenY = static_cast<float>(evt.state.Y.abs) / static_cast<float>(evt.state.height);
        const Ogre::Ray worldRay = cam->getCameraToViewportRay(screenX, screenY);

        const Ogre::Matrix4 nodeInv = this->gameObjectPtr->getSceneNode()->_getFullTransform().inverse();
        Ogre::Matrix3 nodeInv3;
        nodeInv.extract3x3Matrix(nodeInv3);

        const Ogre::Ray localRay(nodeInv * worldRay.getOrigin(), (nodeInv3 * worldRay.getDirection()).normalisedCopy());

        const std::vector<Ogre::Vector3>& verts = this->planet->getVertices();
        const std::vector<Ogre::uint32>& idxs = this->planet->getIndices();
        const size_t idxCount = this->planet->getIndexCount();

        Ogre::Vector3 hitPos = Ogre::Vector3::ZERO;
        Ogre::Vector3 hitNorm = Ogre::Vector3::UNIT_Y;
        float bestDist = std::numeric_limits<float>::max();
        bool hit = false;

        for (size_t i = 0u; i < idxCount; i += 3u)
        {
            std::pair<bool, Ogre::Real> r = Ogre::Math::intersects(localRay, verts[idxs[i]], verts[idxs[i + 1]], verts[idxs[i + 2]], true, false);
            if (r.first && r.second < bestDist)
            {
                bestDist = r.second;
                hitPos = this->gameObjectPtr->getSceneNode()->_getFullTransform() * localRay.getPoint(r.second);
                Ogre::Vector3 fn = (verts[idxs[i + 1]] - verts[idxs[i]]).crossProduct(verts[idxs[i + 2]] - verts[idxs[i]]).normalisedCopy();
                hitNorm = (nodeInv3.Transpose() * fn).normalisedCopy();
                hit = true;
            }
        }

        if (hit)
        {
            if (this->editMode->getListSelectedValue() == "Sculpt")
            {
                this->doApplySculptBrush(hitPos, hitNorm);
            }
            else
            {
                const Ogre::Vector3 localHit = nodeInv * hitPos;
                const Ogre::Vector3 dir = localHit.normalisedCopy();

                float u = std::atan2(dir.z, dir.x) / (2.0f * Ogre::Math::PI);
                if (u < 0.0f)
                {
                    u += 1.0f;
                }
                const float v = 0.5f - std::asin(Ogre::Math::Clamp(dir.y, -1.0f, 1.0f)) / Ogre::Math::PI;

                this->doApplyPaintBrush(Ogre::Vector2(u, v));
            }
        }

        return false;
    }

    bool PlanetTerraComponent::mouseReleased(const OIS::MouseEvent&, OIS::MouseButtonID id)
    {
        if (id != OIS::MB_Left)
        {
            return true;
        }

        if (this->isPressing && this->brushStrokeActive)
        {
            this->fireUndoEvent(this->brushStrokePreEditData);
        }

        this->isPressing = false;
        this->brushStrokeActive = false;
        return false;
    }

    void PlanetTerraComponent::doApplySculptBrush(const Ogre::Vector3& worldHitPos, const Ogre::Vector3& /*worldHitNorm*/)
    {
        // Transform hit to planet local space
        const Ogre::Matrix4 inv = this->gameObjectPtr->getSceneNode()->_getFullTransform().inverse();
        const Ogre::Vector3 localHit = inv * worldHitPos;

        // invert = Shift held (dig instead of raise for Pull mode, etc.)
        const bool invert = isShiftPressed;

        // CPU sculpt (main thread)
        planet->applyBrush(localHit, invert, getBrushModeEnum(), brushData, brushImageW, brushImageH, brushSize->getReal(), brushIntensity->getReal(), brushFalloff->getReal());

        planet->recalculateNormals();
        planet->recalculateTangents();

        // Upload to GPU (render thread)
        NOWA::GraphicsModule::RenderCommand cmd = [this]()
        {
            planet->uploadVertexData();
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "PlanetTerraComponent::sculptUpload");

        this->brushStrokeActive = true;
        this->hasModifiedData = true;
    }

    void PlanetTerraComponent::doApplyPaintBrush(const Ogre::Vector2& hitUV)
    {
        const bool invert = this->isShiftPressed;

        /*Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlanetTerraComponent] Paint UV=(" + Ogre::StringConverter::toString(hitUV.x) + ", " + Ogre::StringConverter::toString(hitUV.y) +
                                                                               ") layer=" + Ogre::StringConverter::toString(this->getPaintLayerId()) + " brushSize=" + Ogre::StringConverter::toString(this->brushSize->getReal()) +
                                                                               " blendTexSize=" + Ogre::StringConverter::toString(this->blendTexSize->getUInt()) +
                                                                               " blendDataSize=" + Ogre::StringConverter::toString(static_cast<unsigned int>(this->planet->getBlendDataCopy().size())));*/

        this->planet->applyPaintBrush(hitUV, invert, this->getPaintLayerId(), this->brushData, this->brushImageW, this->brushImageH, this->brushSize->getReal(), this->brushIntensity->getReal(), this->brushFalloff->getReal());

        NOWA::GraphicsModule::RenderCommand cmd = [this]()
        {
            this->planet->uploadBlendData();
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "PlanetTerraComponent::paintUpload");

        this->brushStrokeActive = true;
        this->hasModifiedData = true;
    }

    void PlanetTerraComponent::updateModificationState(void)
    {
        const bool shouldBeActive = activated->getBool() && isEditorMeshModifyMode && isSelected;

        if (shouldBeActive)
        {
            addInputListener();
        }
        else
        {
            removeInputListener();
            isPressing = false;
            brushStrokeActive = false;
        }
    }

    bool PlanetTerraComponent::bakeMeshToFile(void)
    {
        if (!planet || !planet->getMesh())
        {
            return false;
        }

        const Ogre::String meshFileName = this->gameObjectPtr->getName() + "_Planet.mesh";
        const Ogre::String filePath = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "/" + meshFileName;

        bool success = false;
        NOWA::GraphicsModule::RenderCommand cmd = [this, filePath, &success]()
        {
            try
            {
                Ogre::MeshSerializer serializer(Ogre::Root::getSingletonPtr()->getRenderSystem()->getVaoManager());
                serializer.exportMesh(planet->getMesh().get(), filePath, Ogre::MESH_VERSION_LATEST, Ogre::Serializer::ENDIAN_NATIVE);
                success = true;
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlanetTerraComponent] Baked mesh to: " + filePath);
            }
            catch (Ogre::Exception& e)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PlanetTerraComponent] Bake failed: " + e.getDescription());
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "PlanetTerraComponent::bakeMesh");

        if (success)
        {
            // Refresh mesh resource list so it shows up in the editor immediately
            boost::shared_ptr<EventDataRefreshMeshResources> evt(new EventDataRefreshMeshResources());
            AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evt);
        }

        return success;
    }

    void PlanetTerraComponent::wireBlendTextureToPbsDatablockInternal(void)
    {
        if (!this->planet || !this->planet->getBlendTex())
        {
            return;
        }

        auto datablockComp = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<DatablockPbsComponent>());
        if (!datablockComp)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PlanetTerraComponent] wireBlend: no DatablockPbsComponent on '" + this->gameObjectPtr->getName() + "'.");
            return;
        }

        if (0u != this->blendTexSize->getUInt())
        {
            // Wire the runtime blend texture into the cloned datablock
            datablockComp->setTextureDirectly(Ogre::PBSM_DETAIL_WEIGHT, this->planet->getBlendTex());
        }

        // Get the cloned datablock from DatablockPbsComponent — NOT from
        // planet->getItem(), which may still point to the original template
        // assigned by setDatablockByName() inside create().
        Ogre::HlmsPbsDatablock* pbsDb = datablockComp->getDataBlock();
        if (!pbsDb)
        {
            return;
        }

        // Ensure the planet item actually uses this cloned datablock
        this->planet->getItem()->getSubItem(0)->setDatablock(pbsDb);

        pbsDb->setTextureUvSource(Ogre::PBSM_DIFFUSE, 1u);
        pbsDb->setTextureUvSource(Ogre::PBSM_NORMAL, 1u);

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlanetTerraComponent] Blend texture wired + UV1 assigned for '" + this->gameObjectPtr->getName() + "'.");
    }

    void PlanetTerraComponent::wireBlendTextureToPbsDatablock(void)
    {
        if (!this->planet)
        {
            return;
        }

        NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
        {
            this->wireBlendTextureToPbsDatablockInternal();
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PlanetTerraComponent::wireBlendTextureToPbsDatablock");
    }

    Ogre::String PlanetTerraComponent::getPlanetDataFilePath() const
    {
        return Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "/" + this->gameObjectPtr->getName() + "_PlanetTerra_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()) + ".ptd";
    }

    void PlanetTerraComponent::savePlanetDataToFile()
    {
        if (!this->planet)
        {
            return;
        }

         // Only persist data that was actually modified or previously loaded.
        // Fresh flat procedural planets (never sculpted/painted) produce no useful
        // sidecar -- skipping keeps the scene folder clean, especially when
        // UniversumComponent generates dozens of planets.
        /*if (false == this->hasModifiedData)
        {
            return;
        }*/

        const std::vector<float>& heights = this->planet->getHeightDataCopy();
        const std::vector<uint8_t>& blend = this->planet->getBlendDataCopy();

        if (heights.empty())
        {
            return;
        }

        std::ofstream out(this->getPlanetDataFilePath(), std::ios::binary | std::ios::trunc);
        if (!out.good())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PlanetTerraComponent] Cannot write planet data to: " + this->getPlanetDataFilePath());
            return;
        }

        // Header
        const char magic[4] = {'P', 'L', 'N', 'T'};
        const uint32_t version = 1u;
        const uint32_t vc = static_cast<uint32_t>(heights.size());
        const uint32_t bts = static_cast<uint32_t>(planet->getBlendTexSize());

        out.write(magic, 4);
        out.write(reinterpret_cast<const char*>(&version), 4);
        out.write(reinterpret_cast<const char*>(&vc), 4);
        out.write(reinterpret_cast<const char*>(&bts), 4);
        out.write(reinterpret_cast<const char*>(heights.data()), vc * sizeof(float));
        out.write(reinterpret_cast<const char*>(blend.data()), blend.size());

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlanetTerraComponent] Saved planet data (" + Ogre::StringConverter::toString(vc) + " verts) to: " + getPlanetDataFilePath());
    }

    bool PlanetTerraComponent::loadPlanetDataFromFile()
    {
        const Ogre::String path = getPlanetDataFilePath();
        std::ifstream in(path, std::ios::binary);
        if (!in.good())
        {
            return false;
        }

        char magic[4];
        in.read(magic, 4);
        if (magic[0] != 'P' || magic[1] != 'L' || magic[2] != 'N' || magic[3] != 'T')
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PlanetTerraComponent] Bad magic in: " + path);
            return false;
        }

        uint32_t version = 0u, vc = 0u, bts = 0u;
        in.read(reinterpret_cast<char*>(&version), 4);
        in.read(reinterpret_cast<char*>(&vc), 4);
        in.read(reinterpret_cast<char*>(&bts), 4);

        loadedHeightData.resize(vc);
        in.read(reinterpret_cast<char*>(loadedHeightData.data()), vc * sizeof(float));

        const size_t blendBytes = static_cast<size_t>(bts) * bts * 4u;
        loadedBlendData.resize(blendBytes);
        in.read(reinterpret_cast<char*>(loadedBlendData.data()), blendBytes);

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlanetTerraComponent] Loaded planet data (" + Ogre::StringConverter::toString(vc) + " verts) from: " + path);

        const bool success = in.good() || in.eof();
        if (true == success)
        {
            // Mark dirty so the next scene save re-persists the loaded data.
            this->hasModifiedData = true;
        }
        return success;
    }

    void PlanetTerraComponent::deletePlanetDataFile()
    {
        const std::filesystem::path p(getPlanetDataFilePath());
        std::error_code ec;
        std::filesystem::remove(p, ec);
        if (!ec)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlanetTerraComponent] Deleted planet data file: " + getPlanetDataFilePath());
        }
    }

    std::vector<unsigned char> PlanetTerraComponent::getPlanetData() const
    {
        if (!planet)
        {
            return {};
        }

        const std::vector<float>& heights = planet->getHeightDataCopy();
        const std::vector<uint8_t>& blend = planet->getBlendDataCopy();

        const uint32_t vc = static_cast<uint32_t>(heights.size());
        const uint32_t bsz = static_cast<uint32_t>(blend.size());

        std::vector<unsigned char> buf;
        buf.resize(8u + vc * sizeof(float) + bsz);

        size_t off = 0u;
        memcpy(&buf[off], &vc, 4);
        off += 4u;
        memcpy(&buf[off], &bsz, 4);
        off += 4u;
        memcpy(&buf[off], heights.data(), vc * sizeof(float));
        off += vc * sizeof(float);
        memcpy(&buf[off], blend.data(), bsz);

        return buf;
    }

    void PlanetTerraComponent::setPlanetData(const std::vector<unsigned char>& data)
    {
        if (!this->planet || data.size() < 8u)
        {
            return;
        }

        uint32_t vc = 0u, bsz = 0u;
        size_t off = 0u;
        memcpy(&vc, &data[off], 4);
        off += 4u;
        memcpy(&bsz, &data[off], 4);
        off += 4u;

        const size_t expectedSize = 8u + vc * sizeof(float) + bsz;
        if (data.size() < expectedSize)
        {
            return;
        }

        std::vector<float> heights(vc);
        std::vector<uint8_t> blend(bsz);
        memcpy(heights.data(), &data[off], vc * sizeof(float));
        off += vc * sizeof(float);
        memcpy(blend.data(), &data[off], bsz);

        this->planet->restoreHeightData(heights);
        this->planet->restoreBlendData(blend);

        NOWA::GraphicsModule::RenderCommand cmd = [this]()
        {
            this->planet->uploadVertexData();
            this->planet->uploadBlendData();
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "PlanetTerraComponent::setPlanetData");
    }

    void PlanetTerraComponent::setDynamic(bool dynamic)
    {
        if (nullptr != this->planet)
        {
            this->planet->setDynamic(dynamic);
        }
    }

    Ogre::Real PlanetTerraComponent::getComputedMaxRadius() const
    {
        if (nullptr != this->planet)
        {
            return this->planet->getComputedMaxRadius();
        }
        return this->radius->getReal(); // fallback to base radius if terra not yet built
    }

    bool PlanetTerraComponent::findFlatLandingVertex(const Ogre::Vector3& outwardDirWorld, float searchHalfAngleDeg, Ogre::Vector3& outWorldPos, Ogre::Vector3& outWorldNormal) const
    {
        if (nullptr == this->planet)
        {
            return false;
        }
        Ogre::Vector3 localPos, localNormal;
        if (false == this->planet->findFlatLandingVertex(outwardDirWorld, searchHalfAngleDeg, localPos, localNormal))
        {
            return false;
        }
        // PlanetTerra vertices are in local space (planet centre = origin, no rotation applied).
        // The planet GO's scene node has world position but negligible rotation for landing purposes.
        // Add the GO world position to convert to world space.
        const Ogre::Vector3 planetWorldPos = this->gameObjectPtr->getPosition();
        outWorldPos = planetWorldPos + localPos;
        outWorldNormal = localNormal; // direction is rotation-invariant for a sphere
        return true;
    }

    bool PlanetTerraComponent::collectSurfaceSamples(float slopeMaxDeg, float heightMinLocal, float heightMaxLocal, std::vector<PlanetTerra::SurfaceSample>& outSamples, Ogre::Vector3& outWorldOffset) const
    {
        if (nullptr == this->planet)
        {
            return false;
        }
        outWorldOffset = this->gameObjectPtr->getPosition();
        this->planet->collectSurfaceSamples(slopeMaxDeg, heightMinLocal, heightMaxLocal, outSamples);
        return true;
    }

    bool PlanetTerraComponent::collectSurfaceSamplesInCone(const Ogre::Vector3& capDirWorld, float capHalfAngleDeg, float slopeMaxDeg, float heightMinLocal, float heightMaxLocal, std::vector<PlanetTerra::SurfaceSample>& outSamples,
        Ogre::Vector3& outWorldOffset) const
    {
        if (nullptr == this->planet)
        {
            return false;
        }
        outWorldOffset = this->gameObjectPtr->getPosition();
        // capDir is already world-space and equals local-space because the planet has
        // no rotation baked in (only position differs from local to world).
        this->planet->collectSurfaceSamplesInCone(capDirWorld, capHalfAngleDeg, slopeMaxDeg, heightMinLocal, heightMaxLocal, outSamples);
        return true;
    }

    size_t PlanetTerraComponent::getVertexCount(void) const
    {
        if (nullptr == this->planet)
        {
            return 0;
        }
        return this->planet->getVertexCount();
    }

    void PlanetTerraComponent::fireUndoEvent(const std::vector<unsigned char>& oldData)
    {
        const std::vector<unsigned char> newData = this->getPlanetData();

        // Queue the full undo transaction. The EditorManager is the sole listener
        // for EventDataPlanetTerraModifyEnd — it stores old/new on its undo stack
        // and calls setPlanetData() directly when Ctrl+Z / Ctrl+Y is invoked.
        // We do NOT subscribe to this event ourselves; no ignore-flag is needed.
        boost::shared_ptr<NOWA::EventDataCommandTransactionBegin> evtBegin(new NOWA::EventDataCommandTransactionBegin("Planet Terra Edit"));
        AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evtBegin);

        boost::shared_ptr<EventDataPlanetTerraModifyEnd> evtMod(new EventDataPlanetTerraModifyEnd(oldData, newData, this->gameObjectPtr->getId()));
        AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evtMod);

        boost::shared_ptr<NOWA::EventDataCommandTransactionEnd> evtEnd(new NOWA::EventDataCommandTransactionEnd());
        AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evtEnd);
    }

    void PlanetTerraComponent::loadBrushImage(const Ogre::String& name)
    {
        brushData.clear();
        brushImageW = 0;
        brushImageH = 0;

        if (name.empty() || name == "Default (Smooth)")
        {
            return;
        }

        try
        {
            Ogre::Image2 img;
            img.load(name, Ogre::ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME);
            brushImageW = static_cast<int>(img.getWidth());
            brushImageH = static_cast<int>(img.getHeight());
            brushData.resize(static_cast<size_t>(brushImageW) * brushImageH, 0.0f);
            for (Ogre::uint32 y = 0u; y < static_cast<Ogre::uint32>(brushImageH); ++y)
            {
                for (Ogre::uint32 x = 0u; x < static_cast<Ogre::uint32>(brushImageW); ++x)
                {
                    brushData[static_cast<size_t>(y) * brushImageW + x] = img.getColourAt(x, y, 0u).r;
                }
            }
        }
        catch (Ogre::Exception& e)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PlanetTerraComponent] Brush load failed '" + name + "': " + e.getDescription());
        }
    }

    PlanetTerra::BrushMode PlanetTerraComponent::getBrushModeEnum() const
    {
        const Ogre::String& m = brushMode->getListSelectedValue();
        if (m == "Push")
        {
            return PlanetTerra::BrushMode::PUSH;
        }
        if (m == "Smooth")
        {
            return PlanetTerra::BrushMode::SMOOTH;
        }
        if (m == "Flatten")
        {
            return PlanetTerra::BrushMode::FLATTEN;
        }
        if (m == "Inflate")
        {
            return PlanetTerra::BrushMode::INFLATE;
        }
        return PlanetTerra::BrushMode::PULL; // default
    }

    void PlanetTerraComponent::handleMeshModifyMode(NOWA::EventDataPtr eventData)
    {
        auto castData = boost::static_pointer_cast<EventDataEditorMode>(eventData);
        isEditorMeshModifyMode = (castData->getManipulationMode() == EditorManager::EDITOR_MESH_MODIFY_MODE);
        updateModificationState();
    }

    void PlanetTerraComponent::handleGameObjectSelected(NOWA::EventDataPtr eventData)
    {
        auto castData = boost::static_pointer_cast<EventDataGameObjectSelected>(eventData);

        if (castData->getGameObjectId() == this->gameObjectPtr->getId())
        {
            this->isSelected = castData->getIsSelected();
        }
        else if (castData->getIsSelected())
        {
            // Another object was selected, deselect this one
            this->isSelected = false;
        }

        // Do NOT queue EventDataEditorMode here. Forcing EDITOR_MESH_MODIFY_MODE on
        // selection hijacks the editor mouse handling and causes selection rectangles
        // to appear. The user switches edit mode via the editor toolbar.
        this->updateModificationState();
    }

    void PlanetTerraComponent::handleNewComponent(NOWA::EventDataPtr eventData)
    {
        auto castData = boost::static_pointer_cast<EventDataNewComponent>(eventData);

        // Only react when DatablockPbsComponent on THIS game object finishes postInit.
        if (castData->getGameObjectId() != this->gameObjectPtr->getId())
        {
            return;
        }
        if (castData->getComponentName() != DatablockPbsComponent::getStaticClassName())
        {
            return;
        }

        this->wireBlendTextureToPbsDatablock();

        // Apply the stored UV scale to all detail layers immediately after wiring.
        // This ensures the scale is correct on both fresh placement and scene reload.
        this->setDetailUVScale(this->detailUVScale->getReal());

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PlanetTerraComponent] handleNewComponent: DatablockPbsComponent ready, blend texture wired for '" + this->gameObjectPtr->getName() + "'.");
    }

    void PlanetTerraComponent::handleComponentManuallyDeleted(NOWA::EventDataPtr eventData)
    {
        auto castData = boost::static_pointer_cast<EventDataDeleteComponent>(eventData);
        if (castData->getGameObjectId() == this->gameObjectPtr->getId() && castData->getComponentName() == this->getClassName())
        {
            deletePlanetDataFile();
        }
    }

    void PlanetTerraComponent::setActivated(bool v)
    {
        activated->setValue(v);
        updateModificationState();
    }

    bool PlanetTerraComponent::isActivated() const
    {
        return activated->getBool();
    }

    void PlanetTerraComponent::setRadius(Ogre::Real radius)
    {
        this->radius->setValue(radius);
        destroyPlanet();
        createPlanet();
        // Re-wire the new blend texture and NM textures after rebuild.
        // Without this call, the recreated planet uses the original material's
        // static weight map (PlanetTerraBlendDefault.png) which may have A=255
        // causing mntn_black to render at full weight everywhere.
        this->wireBlendTextureToPbsDatablock();
    }

    Ogre::Real PlanetTerraComponent::getRadius() const
    {
        return radius->getReal();
    }

    const std::vector<Ogre::Vector2>& PlanetTerraComponent::getUvCoords(void) const
    {
        if (nullptr != this->planet)
        {
            return this->planet->getUvCoords();
        }

        // The warning is correct — returning a reference to a temporary that gets destroyed immediately.Fix it with a static empty vector
        // static means it lives for the duration of the program, so the reference is always valid.
        static const std::vector<Ogre::Vector2> empty;
        return empty;
    }

    void PlanetTerraComponent::setSegmentsH(unsigned int s)
    {
        this->segmentsH->setValue(s);
        this->destroyPlanet();
        this->createPlanet();
        // Re-wire the new blendWeightTex after rebuild since a new TextureGpu was created.
        this->wireBlendTextureToPbsDatablock();
    }

    unsigned int PlanetTerraComponent::getSegmentsH() const
    {
        return this->segmentsH->getUInt();
    }

    void PlanetTerraComponent::setSegmentsV(unsigned int s)
    {
        this->segmentsV->setValue(s);
        this->destroyPlanet();
        this->createPlanet();
        this->wireBlendTextureToPbsDatablock();
    }

    unsigned int PlanetTerraComponent::getSegmentsV() const
    {
        return this->segmentsV->getUInt();
    }

    void PlanetTerraComponent::setBlendTexSize(unsigned int s)
    {
        this->blendTexSize->setValue(s);
        this->destroyPlanet();
        this->createPlanet();
        this->wireBlendTextureToPbsDatablock();
    }

    unsigned int PlanetTerraComponent::getBlendTexSize() const
    {
        return this->blendTexSize->getUInt();
    }

    void PlanetTerraComponent::setEditMode(const Ogre::String& m)
    {
        editMode->setListSelectedValue(m);
    }

    Ogre::String PlanetTerraComponent::getEditMode() const
    {
        return editMode->getListSelectedValue();
    }

    void PlanetTerraComponent::setBrushMode(const Ogre::String& m)
    {
        brushMode->setListSelectedValue(m);
    }

    Ogre::String PlanetTerraComponent::getBrushMode() const
    {
        return brushMode->getListSelectedValue();
    }

    void PlanetTerraComponent::setPaintLayer(const Ogre::String& l)
    {
        paintLayer->setListSelectedValue(l);
    }

    Ogre::String PlanetTerraComponent::getPaintLayer() const
    {
        return paintLayer->getListSelectedValue();
    }

    int PlanetTerraComponent::getPaintLayerId() const
    {
        return Ogre::StringConverter::parseInt(paintLayer->getListSelectedValue());
    }

    void PlanetTerraComponent::setBrushName(const Ogre::String& name)
    {
        brushName->setListSelectedValue(name);
        loadBrushImage(name);
    }

    Ogre::String PlanetTerraComponent::getBrushName() const
    {
        return brushName->getListSelectedValue();
    }

    void PlanetTerraComponent::setBrushSize(float s)
    {
        brushSize->setValue(s);
    }

    float PlanetTerraComponent::getBrushSize() const
    {
        return brushSize->getReal();
    }

    void PlanetTerraComponent::setBrushIntensity(float v)
    {
        brushIntensity->setValue(v);
    }

    float PlanetTerraComponent::getBrushIntensity() const
    {
        return this->brushIntensity->getReal();
    }

    void PlanetTerraComponent::setBaseUVScale(float scale)
    {
        if (scale < 0.1f)
        {
            scale = 0.1f;
        }

        this->baseUVScale->setValue(scale);

        if (!this->planet)
        {
            return;
        }

        this->planet->setBaseUVScale(scale);

        NOWA::GraphicsModule::RenderCommand cmd = [this]()
        {
            // Null out BEFORE destroy so GUI sees nullptr not dangling ptr
            this->gameObjectPtr->nullMovableObject();

            this->planet->rebuildDynamicBuffers();

            // Re-wire blend texture + UV sources since Item was recreated.
            // Do NOT call init() here — it would displace the ocean item
            // from the scene node and break PlanetOcean::destroy().
            // rebuildDynamicBuffers already attaches the new item to the node.
            // We only need movableObject updated — use assignMesh which does
            // exactly that without detaching other attached objects.
            this->gameObjectPtr->assignMesh(this->planet->getItem());

            this->wireBlendTextureToPbsDatablockInternal();
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "PlanetTerraComponent::setBaseUVScale");
    }

    float PlanetTerraComponent::getBaseUVScale(void) const
    {
        return this->baseUVScale->getReal();
    }

    void PlanetTerraComponent::setDetailUVScale(float scale)
    {
        if (scale < 1.0f)
        {
            scale = 1.0f;
        }
        this->detailUVScale->setValue(scale);

        NOWA::GraphicsModule::RenderCommand cmd = [this, scale]()
        {
            if (!this->planet || !this->planet->getItem())
            {
                return;
            }

            Ogre::HlmsPbsDatablock* pbsDb = dynamic_cast<Ogre::HlmsPbsDatablock*>(this->planet->getItem()->getSubItem(0)->getDatablock());
            if (!pbsDb)
            {
                return;
            }

            // Detail layers 0-3
            for (Ogre::uint8 layer = 0u; layer < 4u; ++layer)
            {
                pbsDb->setDetailMapOffsetScale(layer, Ogre::Vector4(0.0f, 0.0f, scale, scale));
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "PlanetTerraComponent::setDetailUVScale");
    }

    float PlanetTerraComponent::getDetailUVScale(void) const
    {
        return this->detailUVScale->getReal();
    }

    void PlanetTerraComponent::setBrushFalloff(float v)
    {
        this->brushFalloff->setValue(v);
    }

    float PlanetTerraComponent::getBrushFalloff() const
    {
        return this->brushFalloff->getReal();
    }

    // =========================================================================
    //  Lua API
    // =========================================================================

    PlanetTerraComponent* getPlanetTerraComponent(GameObject* go)
    {
        return NOWA::makeStrongPtr(go->getComponent<PlanetTerraComponent>()).get();
    }

    PlanetTerraComponent* getPlanetTerraComponentFromName(GameObject* go, const Ogre::String& name)
    {
        return NOWA::makeStrongPtr(go->getComponentFromName<PlanetTerraComponent>(name)).get();
    }

    void PlanetTerraComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
    {
        luabind::module(lua)[luabind::class_<PlanetTerraComponent, GameObjectComponent>("PlanetTerraComponent")
                .def("setActivated", &PlanetTerraComponent::setActivated)
                .def("isActivated", &PlanetTerraComponent::isActivated)
                .def("setRadius", &PlanetTerraComponent::setRadius)
                .def("getRadius", &PlanetTerraComponent::getRadius)
                .def("setSegmentsH", &PlanetTerraComponent::setSegmentsH)
                .def("getSegmentsH", &PlanetTerraComponent::getSegmentsH)
                .def("setSegmentsV", &PlanetTerraComponent::setSegmentsV)
                .def("getSegmentsV", &PlanetTerraComponent::getSegmentsV)
                .def("setEditMode", &PlanetTerraComponent::setEditMode)
                .def("getEditMode", &PlanetTerraComponent::getEditMode)
                .def("setBrushMode", &PlanetTerraComponent::setBrushMode)
                .def("getBrushMode", &PlanetTerraComponent::getBrushMode)
                .def("setPaintLayer", &PlanetTerraComponent::setPaintLayer)
                .def("getPaintLayer", &PlanetTerraComponent::getPaintLayer)
                .def("setBrushName", &PlanetTerraComponent::setBrushName)
                .def("getBrushName", &PlanetTerraComponent::getBrushName)
                .def("setBrushSize", &PlanetTerraComponent::setBrushSize)
                .def("getBrushSize", &PlanetTerraComponent::getBrushSize)
                .def("setBrushIntensity", &PlanetTerraComponent::setBrushIntensity)
                .def("getBrushIntensity", &PlanetTerraComponent::getBrushIntensity)
                .def("setBrushFalloff", &PlanetTerraComponent::setBrushFalloff)
                .def("getBrushFalloff", &PlanetTerraComponent::getBrushFalloff)
                .def("bakeMeshToFile", &PlanetTerraComponent::bakeMeshToFile)];

        LuaScriptApi::getInstance()->addClassToCollection("PlanetTerraComponent", "class inherits GameObjectComponent", getStaticInfoText());

        LuaScriptApi::getInstance()->addClassToCollection("PlanetTerraComponent", "void setActivated(bool activated)", "Enables or disables the sculpt/paint edit mode.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetTerraComponent", "bool isActivated()", "Returns true if the edit mode is currently active.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetTerraComponent", "void setRadius(float radius)", "Sets the planet radius in world units and fully recreates the mesh. Existing sculpt data is discarded.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetTerraComponent", "float getRadius()", "Returns the current planet radius in world units.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetTerraComponent", "void setSegmentsH(uint segs)", "Sets the horizontal (longitude) tessellation and recreates the mesh. Minimum 4.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetTerraComponent", "uint getSegmentsH()", "Returns the current horizontal tessellation segment count.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetTerraComponent", "void setSegmentsV(uint segs)", "Sets the vertical (latitude) tessellation and recreates the mesh. Minimum 4.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetTerraComponent", "uint getSegmentsV()", "Returns the current vertical tessellation segment count.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetTerraComponent", "void setEditMode(string mode)", "Sets the active edit mode. Valid values: 'Sculpt', 'Paint'.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetTerraComponent", "string getEditMode()", "Returns the active edit mode: 'Sculpt' or 'Paint'.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetTerraComponent", "void setBrushMode(string mode)", "Sets the sculpt brush mode. Valid values: 'Pull', 'Push', 'Smooth', 'Flatten', 'Inflate'.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetTerraComponent", "string getBrushMode()", "Returns the current sculpt brush mode.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetTerraComponent", "void setPaintLayer(string layer)", "Sets the active paint layer. Valid values: '0', '1', '2', '3' (maps to RGBA channels of the blend texture).");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetTerraComponent", "string getPaintLayer()", "Returns the active paint layer as a string '0'..'3'.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetTerraComponent", "void setBrushName(string name)", "Loads the named brush image from the Brushes resource group as the active brush mask.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetTerraComponent", "string getBrushName()", "Returns the filename of the currently active brush image.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetTerraComponent", "void setBrushSize(float size)", "Sets the brush radius. In Sculpt mode: world units. In Paint mode: blend-texture pixels.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetTerraComponent", "float getBrushSize()", "Returns the current brush radius.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetTerraComponent", "void setBrushIntensity(float intensity)", "Sets the brush strength per application step. Range 0..1.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetTerraComponent", "float getBrushIntensity()", "Returns the current brush intensity.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetTerraComponent", "void setBrushFalloff(float falloff)", "Sets the brush edge falloff exponent. Higher values produce a sharper edge.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetTerraComponent", "float getBrushFalloff()", "Returns the current brush falloff exponent.");
        LuaScriptApi::getInstance()->addClassToCollection("PlanetTerraComponent", "bool bakeMeshToFile()", "Exports the current planet mesh to '<goName>_Planet.mesh' in the scene folder. Returns true on success.");

        gameObjectClass.def("getPlanetTerraComponent", (PlanetTerraComponent * (*)(GameObject*)) & getPlanetTerraComponent);
        gameObjectClass.def("getPlanetTerraComponentFromName", &getPlanetTerraComponentFromName);
        gameObjectControllerClass.def("castPlanetTerraComponent", &GameObjectController::cast<PlanetTerraComponent>);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PlanetTerraComponent getPlanetTerraComponent()", "Gets the PlanetTerraComponent from this GameObject.");
    }

} // namespace NOWA
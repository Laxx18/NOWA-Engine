/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#include "NOWAPrecompiled.h"
#include "ProceduralPlatformComponent.h"
#include "editor/EditorManager.h"
#include "gameobject/GameObjectController.h"
#include "gameobject/GameObjectFactory.h"
#include "gameobject/GameObjectTitleComponent.h"
#include "gameobject/NodeComponent.h"
#include "gameobject/PhysicsArtifactComponent.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "main/InputDeviceCore.h"
#include "modules/GraphicsModule.h"
#include "modules/LuaScriptApi.h"
#include "utilities/Helper.h"
#include "utilities/MathHelper.h"
#include "utilities/XMLConverter.h"

#include "RenderQueueEnums.h"

#include "OgreHlmsManager.h"
#include "OgreHlmsPbs.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgreItem.h"
#include "OgreMesh2.h"
#include "OgreMesh2Serializer.h"
#include "OgreMeshManager2.h"
#include "OgreSubMesh2.h"
#include "Vao/OgreVaoManager.h"
#include "Vao/OgreVertexArrayObject.h"

#include "OgreAbiUtils.h"

#include <filesystem>
#include <fstream>
#include <system_error>

// =============================================================================
// STATUS: complete. Every method declared in ProceduralPlatformComponent.h is defined in
// this file (constructor, Plugin boilerplate, XML round-trip, input handling, placement
// lifecycle, rebuildMesh + curve/smoothing helpers, style generators, junction fill, mesh
// creation/destruction/preview, save/load + undo/redo, segment mode, snapping, cross-network
// merging, batch API, event handlers, Lua API).
//
// Known deliberate scope differences from ProceduralRoadComponent (all flagged inline where
// they occur too):
//   - No closed-loop chain support (no isClosed/generateSeamQuad equivalent).
//   - No computeMiterData equivalent - Depth is orthogonal to the whole path plane and
//     uniform everywhere, so there is no per-point offset direction to precompute.
//   - Wood/Stone/Ice are structurally identical plain boxes (generatePlatformBox with
//     topBevel=0, rimHeight=0) - only Grass (chamfer bevel) and Metal (raised rim) get
//     distinct geometry; the rest rely on their assigned datablock texture for visual
//     distinction, same as most simple block-style platformers.
//   - Event type 0xF1A7F04D for EventDataPlatformModifyEnd and the "Platform" category name
//     are both still unconfirmed against your project's own registries - grep your
//     Events.h/category constants before relying on either.
// =============================================================================

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    ProceduralPlatformComponent::ProceduralPlatformComponent() :
        PlatformComponentBase(),
        name("ProceduralPlatformComponent"),
        activated(new Variant(ProceduralPlatformComponent::AttrActivated(), true, this->attributes)),
        platformDepth(new Variant(ProceduralPlatformComponent::AttrPlatformDepth(), 2.0f, this->attributes)),
        platformHeight(new Variant(ProceduralPlatformComponent::AttrPlatformHeight(), 1.0f, this->attributes)),
        platformStyle(new Variant(ProceduralPlatformComponent::AttrPlatformStyle(), {"Grass", "Wood", "Stone", "Ice", "Metal"}, this->attributes)),
        snapToGrid(new Variant(ProceduralPlatformComponent::AttrSnapToGrid(), false, this->attributes)),
        gridSize(new Variant(ProceduralPlatformComponent::AttrGridSize(), 1.0f, this->attributes)),
        smoothingFactor(new Variant(ProceduralPlatformComponent::AttrSmoothingFactor(), 0.5f, this->attributes)),
        curveSubdivisions(new Variant(ProceduralPlatformComponent::AttrCurveSubdivisions(), 10, this->attributes)),
        surfaceDatablock(new Variant(ProceduralPlatformComponent::AttrSurfaceDatablock(), "grass_clean", this->attributes)),
        groundDatablock(new Variant(ProceduralPlatformComponent::AttrGroundDatablock(), "rockClif_D", this->attributes)),
        surfaceUVTiling(new Variant(ProceduralPlatformComponent::AttrSurfaceUVTiling(), Ogre::Vector2(1.0f, 1.0f), this->attributes)),
        groundUVTiling(new Variant(ProceduralPlatformComponent::AttrGroundUVTiling(), Ogre::Vector2(1.0f, 1.0f), this->attributes)),
        editMode(new Variant(ProceduralPlatformComponent::AttrEditMode(), std::vector<Ogre::String>{"Object", "Segment"}, this->attributes)),
        convertToMesh(new Variant(ProceduralPlatformComponent::AttrConvertToMesh(), Ogre::String("Convert to Mesh"), this->attributes)),
        buildState(BuildState::IDLE),
        isEditorMeshModifyMode(false),
        isSelected(false),
        currentSurfaceVertexIndex(0),
        currentGroundVertexIndex(0),
        currentJunctionVertexIndex(0),
        platformItem(nullptr),
        previewItem(nullptr),
        previewNode(nullptr),
        isShiftPressed(true),
        isCtrlPressed(false),
        hasPlatformOrigin(false),
        platformFrame(Ogre::Quaternion::IDENTITY),
        otherPlatformQuery(nullptr),
        pendingCrossNetworkSnap(false),
        pendingMergeOtherPlatform(nullptr),
        cachedNumSurfaceVertices(0),
        cachedNumGroundVertices(0),
        cachedNumJunctionVertices(0),
        originPositionSet(false),
        hasLoadedPlatformEndpoint(false),
        loadedPlatformEndpointHeight(0.0f),
        selectedSegmentIndex(-1),
        segOverlayNode(nullptr),
        segOverlayObject(nullptr),
        isExtendingFromSegment(false),
        isSnapToOwnPlatform(false),
        snapToPlatformSegmentIdx(-1),
        snapRadius(0.0f),
        bBatchMode(false),
        platformLoadedFromScene(false),
        physicsArtifactComponent(nullptr)
    {
        this->platformStyle->setDescription("Style of the platform to generate.");
        this->platformDepth->setDescription("Z-thickness of the platform slab - how far it extends front-to-back on the fixed depth plane (meters). "
                                            "Plays the same cross-section role roadWidth plays for a road, just on the depth axis instead of "
                                            "sideways, since the drawn path already IS the platform's walkable shape.");
        this->platformHeight->setDescription("How far the solid platform body extends downward from the top walkable surface (meters).");
        this->smoothingFactor->setDescription("Amount of height smoothing between connected segments (0-1, higher = smoother gradients).");
        this->curveSubdivisions->setDescription("Number of segments for curved platform paths (higher = smoother).");

        this->surfaceDatablock->setDescription("The top walkable surface datablock to set (e.g. grass, wood plank top).");
        this->surfaceDatablock->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
        this->groundDatablock->setDescription("PBS datablock for the platform body (sides/bottom, e.g. striped dirt, wood grain). "
                                              "Falls back to Surface Datablock when left empty. Also used for junction fan patches "
                                              "(where 3+ segments meet) - there is no separate junction property.");
        this->groundDatablock->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");

        this->editMode->setDescription("Object: click-drag to build platforms.\n"
                                       "Segment: LMB to select a segment, X to delete it, E to extend.");
        this->editMode->addUserData(GameObject::AttrActionNoUndo());

        this->convertToMesh->setDescription("Converts this procedural platform to a static mesh file. "
                                            "This is a ONE-WAY operation! After conversion:\n"
                                            "- The platform mesh is exported to the Procedural resource folder\n"
                                            "- This ProceduralPlatformComponent is removed\n"
                                            "- The GameObject will use the static mesh file\n"
                                            "- You can no longer edit the platform procedurally\n"
                                            "- The mesh will benefit from Ogre's graphics caching (no FPS drops)\n\n"
                                            "Use this when you're finished designing the platform and want optimal performance.");
        this->convertToMesh->addUserData(GameObject::AttrActionExec());
        this->convertToMesh->addUserData(GameObject::AttrActionNeedRefresh());
        this->convertToMesh->addUserData(GameObject::AttrActionExecId(), "ProceduralPlatformComponent.ConvertToMesh");
    }

    ProceduralPlatformComponent::~ProceduralPlatformComponent()
    {
    }

    void ProceduralPlatformComponent::install(const Ogre::NameValuePairList* options)
    {
        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<ProceduralPlatformComponent>(ProceduralPlatformComponent::getStaticClassId(), ProceduralPlatformComponent::getStaticClassName());
    }

    void ProceduralPlatformComponent::initialise()
    {
    }

    void ProceduralPlatformComponent::shutdown()
    {
    }

    void ProceduralPlatformComponent::uninstall()
    {
    }

    const Ogre::String& ProceduralPlatformComponent::getName() const
    {
        return this->name;
    }

    void ProceduralPlatformComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    bool ProceduralPlatformComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        this->platformLoadedFromScene = true;

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPlatformComponent::AttrActivated())
        {
            this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPlatformComponent::AttrPlatformDepth())
        {
            this->platformDepth->setValue(XMLConverter::getAttribReal(propertyElement, "data", 2.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPlatformComponent::AttrPlatformHeight())
        {
            this->platformHeight->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPlatformComponent::AttrPlatformStyle())
        {
            this->platformStyle->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPlatformComponent::AttrSnapToGrid())
        {
            this->snapToGrid->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPlatformComponent::AttrGridSize())
        {
            this->gridSize->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPlatformComponent::AttrSmoothingFactor())
        {
            this->smoothingFactor->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.5f));
            propertyElement = propertyElement->next_sibling("property");
        }
        // Backward compatibility: scenes saved before Max Gradient and Max Turn Angle were
        // removed still carry both properties, in this position, in their XML. init() walks
        // the property list strictly in order, so an unconsumed property would leave the
        // reader stuck on it and silently drop EVERY attribute after it. Skip over them
        // without reading their values. Note both are gone for good, not merely defaulted -
        // see smoothHeightTransitions and updatePlatformPreview for why neither has a
        // meaning any more.
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Max Gradient")
        {
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Max Turn Angle")
        {
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPlatformComponent::AttrCurveSubdivisions())
        {
            this->curveSubdivisions->setValue(XMLConverter::getAttribInt(propertyElement, "data", 10));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPlatformComponent::AttrSurfaceDatablock())
        {
            this->surfaceDatablock->setValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPlatformComponent::AttrGroundDatablock())
        {
            this->groundDatablock->setValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        // Backward compatibility: scenes saved before Junction Datablock was removed (it now
        // always reuses Ground Datablock) still have this property in their XML - skip over
        // it without reading its value, so older saves keep loading correctly instead of
        // getting the reader stuck here and silently failing every property after it.
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Junction Datablock")
        {
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPlatformComponent::AttrSurfaceUVTiling())
        {
            this->surfaceUVTiling->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPlatformComponent::AttrGroundUVTiling())
        {
            this->groundUVTiling->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        return true;
    }

    GameObjectCompPtr ProceduralPlatformComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        // NOTE: mirrors ProceduralRoadComponent::clone() exactly - cloning is not yet
        // supported, for the same reason: the procedural geometry cache is keyed by this
        // GameObject's own id (getPlatformDataFilePath() -> "Platform_<id>.platformdata"),
        // so a naive clone would either collide with or silently miss the source object's
        // saved geometry. Left unimplemented rather than guessed at.
        return nullptr;
    }

    bool ProceduralPlatformComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPlatformComponent] Init platform component for game object: " + this->gameObjectPtr->getName());

        AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &ProceduralPlatformComponent::handleMeshModifyMode), NOWA::EventDataEditorMode::getStaticEventType());
        AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &ProceduralPlatformComponent::handleGameObjectSelected), NOWA::EventDataGameObjectSelected::getStaticEventType());
        AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &ProceduralPlatformComponent::handleComponentManuallyDeleted), EventDataDeleteComponent::getStaticEventType());

        this->gameObjectPtr->changeCategory("Platform");

        // Separate query, ALL_CATEGORIES_ID, so it can see ANY other GameObject's platform
        // item regardless of category - deliberately independent of any obstacle/ground
        // category scoping. Used only by findOtherPlatformNearby().
        this->otherPlatformQuery = this->gameObjectPtr->getSceneManager()->createRayQuery(Ogre::Ray(), GameObjectController::ALL_CATEGORIES_ID);
        this->otherPlatformQuery->setSortByDistance(true);

        // Capture the fixed depth plane from the GameObject's OWN current transform.
        // Unlike ProceduralRoadComponent::roadFrame (derived from a terrain-hit normal, so
        // it can only be known once the user's first click lands somewhere), this doesn't
        // need to wait for anything: "fixed -Z-axis" means the depth layer is already
        // decided by wherever this (empty) GameObject was placed - via the normal gizmo -
        // before this component was added. Works identically whether the GameObject was
        // just created (fresh build) or is being restored from a saved scene (the saved
        // node transform already encodes the intended plane).
        this->platformFrame = this->gameObjectPtr->getSceneNode()->_getDerivedOrientationUpdated();
        this->platformPlaneAnchor = this->gameObjectPtr->getSceneNode()->_getDerivedPositionUpdated();

        // Create preview scene node
        this->previewNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();

        // Start with add-platform mode by default
        this->isShiftPressed = true;

        if (true == this->platformLoadedFromScene)
        {
            // Loaded from a saved scene is NOT regenerated here - deferred to
            // handleSceneParsed(), which fires once every GameObject in the scene has
            // finished its own postInit(). See ProceduralRoadComponent::postInit for the
            // detailed rationale (category/query safety during scene load); this component
            // has no such terrain/category dependency itself, but the deferral is kept for
            // consistency and because it costs nothing to wait one extra event.
            this->platformLoadedFromScene = false;

            AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &ProceduralPlatformComponent::handleSceneParsed), EventDataSceneParsed::getStaticEventType());
        }

        this->createSegmentOverlay();

        // Adjust snap radius here
        this->snapRadius = this->platformDepth->getReal() * 0.4f;

        return true;
    }

    void ProceduralPlatformComponent::handleSceneParsed(NOWA::EventDataPtr eventData)
    {
        AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ProceduralPlatformComponent::handleSceneParsed), EventDataSceneParsed::getStaticEventType());

        // Load platform data from file
        if (true == this->loadPlatformDataFromFile())
        {
            // Get PhysicsArtifactComponent if exists
            const auto& physicsArtifactCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsArtifactComponent>());
            if (physicsArtifactCompPtr)
            {
                this->physicsArtifactComponent = physicsArtifactCompPtr.get();
                if (nullptr != this->physicsArtifactComponent)
                {
                    this->physicsArtifactComponent->reCreateCollision();
                }
            }

            if (false == this->platformSegments.empty())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPlatformComponent] Successfully loaded and rebuilt platform with " + Ogre::StringConverter::toString(this->platformSegments.size()) + " segments");
            }
        }
    }

    bool ProceduralPlatformComponent::connect(void)
    {
        if (this->segOverlayNode)
        {
            NOWA::GraphicsModule::RenderCommand cmd = [this]()
            {
                if (this->segOverlayNode)
                {
                    this->segOverlayNode->setVisible(false);
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueue(std::move(cmd), "ProceduralPlatformComponent::connect::hideSegOverlay");
        }

        return true;
    }

    bool ProceduralPlatformComponent::disconnect(void)
    {
        this->destroyPreviewMesh();
        this->buildState = BuildState::IDLE;

        return true;
    }

    bool ProceduralPlatformComponent::onCloned(void)
    {
        return true;
    }

    void ProceduralPlatformComponent::onAddComponent(void)
    {
        boost::shared_ptr<EventDataEditorMode> eventDataEditorMode(new EventDataEditorMode(EditorManager::EDITOR_MESH_MODIFY_MODE));
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataEditorMode);

        // Causes correct platform selection, if new platform game object is added so that
        // platforms can be merged together
        boost::shared_ptr<NOWA::EventDataGameObjectSelected> eventDataGameObjectSelected(new NOWA::EventDataGameObjectSelected(this->gameObjectPtr->getId(), true, false));
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataGameObjectSelected);

        this->isSelected = true;
        this->addInputListener();
    }

    void ProceduralPlatformComponent::onRemoveComponent(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPlatformComponent] Destructor platform component for game object: " + this->gameObjectPtr->getName());

        this->pendingMergeOtherPlatform = nullptr;
        this->physicsArtifactComponent = nullptr;

        AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ProceduralPlatformComponent::handleMeshModifyMode), NOWA::EventDataEditorMode::getStaticEventType());
        AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ProceduralPlatformComponent::handleGameObjectSelected), NOWA::EventDataGameObjectSelected::getStaticEventType());
        AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ProceduralPlatformComponent::handleComponentManuallyDeleted), EventDataDeleteComponent::getStaticEventType());
        AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ProceduralPlatformComponent::handleSceneParsed), EventDataSceneParsed::getStaticEventType());

        if (nullptr != this->otherPlatformQuery)
        {
            this->gameObjectPtr->getSceneManager()->destroyQuery(this->otherPlatformQuery);
            this->otherPlatformQuery = nullptr;
        }

        InputDeviceCore::getSingletonPtr()->removeKeyListener(ProceduralPlatformComponent::getStaticClassName() + "_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()));
        InputDeviceCore::getSingletonPtr()->removeMouseListener(ProceduralPlatformComponent::getStaticClassName() + "_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()));

        this->destroyPlatformMesh();
        this->destroyPreviewMesh();
        this->destroySegmentOverlay();

        if (nullptr != this->previewNode)
        {
            NOWA::GraphicsModule::getInstance()->removeTrackedNode(this->previewNode);
            this->gameObjectPtr->getSceneManager()->destroySceneNode(this->previewNode);
            this->previewNode = nullptr;
        }

        GameObjectComponent::onRemoveComponent();
    }

    void ProceduralPlatformComponent::onOtherComponentRemoved(unsigned int index)
    {
        if (nullptr != this->physicsArtifactComponent && index == this->physicsArtifactComponent->getIndex())
        {
            this->physicsArtifactComponent = nullptr;
        }
    }

    void ProceduralPlatformComponent::onOtherComponentAdded(unsigned int index)
    {
    }

    void ProceduralPlatformComponent::update(Ogre::Real dt, bool notSimulating)
    {
    }

    void ProceduralPlatformComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (ProceduralPlatformComponent::AttrActivated() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
        }
        else if (ProceduralPlatformComponent::AttrPlatformDepth() == attribute->getName())
        {
            this->setPlatformDepth(attribute->getReal());
        }
        else if (ProceduralPlatformComponent::AttrPlatformHeight() == attribute->getName())
        {
            this->setPlatformHeight(attribute->getReal());
        }
        else if (ProceduralPlatformComponent::AttrPlatformStyle() == attribute->getName())
        {
            this->setPlatformStyle(attribute->getListSelectedValue());
        }
        else if (ProceduralPlatformComponent::AttrSnapToGrid() == attribute->getName())
        {
            this->setSnapToGrid(attribute->getBool());
        }
        else if (ProceduralPlatformComponent::AttrGridSize() == attribute->getName())
        {
            this->setGridSize(attribute->getReal());
        }
        else if (ProceduralPlatformComponent::AttrSmoothingFactor() == attribute->getName())
        {
            this->setSmoothingFactor(attribute->getReal());
        }
        else if (ProceduralPlatformComponent::AttrCurveSubdivisions() == attribute->getName())
        {
            this->setCurveSubdivisions(attribute->getInt());
        }
        else if (ProceduralPlatformComponent::AttrSurfaceDatablock() == attribute->getName())
        {
            this->setSurfaceDatablock(attribute->getString());
        }
        else if (ProceduralPlatformComponent::AttrGroundDatablock() == attribute->getName())
        {
            this->setGroundDatablock(attribute->getString());
        }
        else if (ProceduralPlatformComponent::AttrSurfaceUVTiling() == attribute->getName())
        {
            this->setSurfaceUVTiling(attribute->getVector2());
        }
        else if (ProceduralPlatformComponent::AttrGroundUVTiling() == attribute->getName())
        {
            this->setGroundUVTiling(attribute->getVector2());
        }
        else if (ProceduralPlatformComponent::AttrEditMode() == attribute->getName())
        {
            this->setEditMode(attribute->getListSelectedValue());
        }
    }

    void ProceduralPlatformComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        GameObjectComponent::writeXML(propertiesXML, doc);

        xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPlatformComponent::AttrActivated().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPlatformComponent::AttrPlatformDepth().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->platformDepth->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPlatformComponent::AttrPlatformHeight().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->platformHeight->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPlatformComponent::AttrPlatformStyle().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->platformStyle->getListSelectedValue())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPlatformComponent::AttrSnapToGrid().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->snapToGrid->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPlatformComponent::AttrGridSize().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->gridSize->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPlatformComponent::AttrSmoothingFactor().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->smoothingFactor->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPlatformComponent::AttrCurveSubdivisions().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->curveSubdivisions->getInt())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPlatformComponent::AttrSurfaceDatablock().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->surfaceDatablock->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPlatformComponent::AttrGroundDatablock().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->groundDatablock->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPlatformComponent::AttrSurfaceUVTiling().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->surfaceUVTiling->getVector2())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPlatformComponent::AttrGroundUVTiling().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->groundUVTiling->getVector2())));
        propertiesXML->append_node(propertyXML);

        this->savePlatformDataToFile();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////

    bool ProceduralPlatformComponent::canStaticAddComponent(GameObject* gameObject)
    {
        return false;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Input Handling
    ///////////////////////////////////////////////////////////////////////////////////////////////

    // ------------------------------------------------------------------------
    // Convert WORLD raycast hits to PLATFORM-LOCAL right after the raycast.
    // Everything downstream (snapToGridFunc, detectSnapToOwnPlatform,
    // findNearestSegmentWithinRadius, startPlatformPlacement, updatePlatformPreview,
    // confirmPlatform, loadedPlatformEndpoint) then runs uniformly in LOCAL space, exactly
    // like ProceduralRoadComponent. platformFrame is ALREADY known by the time any of this
    // runs (captured in postInit - see the comment there), so there is no first-click
    // frame-derivation step here the way ProceduralRoadComponent needs for roadFrame.
    // ------------------------------------------------------------------------
    bool ProceduralPlatformComponent::mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
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

        Ogre::Camera* camera = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
        if (nullptr == camera)
        {
            return true;
        }

        Ogre::Real screenX = 0.0f;
        Ogre::Real screenY = 0.0f;
        MathHelper::getInstance()->mouseToViewPort(evt.state.X.abs, evt.state.Y.abs, screenX, screenY, Core::getSingletonPtr()->getOgreRenderWindow());

        // ── SEGMENT MODE ──────────────────────────────────────────────────────
        if (this->getEditModeEnum() == EditMode::SEGMENT)
        {
            if (this->isExtendingFromSegment && this->buildState == BuildState::DRAGGING)
            {
                this->confirmPlatform();
                return false;
            }

            Ogre::Vector3 hitPos = Ogre::Vector3::ZERO;
            const bool hit = this->raycastFixedPlane(screenX, screenY, hitPos);

            if (hit)
            {
                hitPos = this->platformFrame.Inverse() * hitPos;

                const Ogre::Real radius = this->platformDepth->getReal() * 1.5f;
                this->selectedSegmentIndex = this->findNearestSegmentWithinRadius(hitPos, radius);
            }

            this->scheduleSegmentOverlayUpdate();
            return false;
        }

        // ── OBJECT MODE ───────────────────────────────────────────────────────
        // NOTE: unlike ProceduralRoadComponent, there is no "did this click land on my own
        // already-built item" scene-raycast guard here (Object-mode positioning is pure
        // plane math, not a scene raycast, so there is no hitMovableObject to compare
        // against). Practical effect: clicking on top of an already-built platform while
        // idle in Object mode starts a new drag from that point rather than bubbling to
        // the selection system - use Segment mode (which IS selection-first) to pick an
        // existing platform instead. Flagging this as a deliberate scope trade-off, not an
        // oversight.
        Ogre::Vector3 hitPosition;
        if (this->raycastFixedPlane(screenX, screenY, hitPosition))
        {
            // Cross-network check for a DIFFERENT platform's path. On a hit, remember the
            // endpoint AND (via dynamic_cast) the concrete component - confirmPlatform()
            // will merge its segments into this platform and delete it once the current
            // segment is confirmed. Mirrors ProceduralRoadComponent's cross-network snap
            // exactly.
            Ogre::Vector3 otherPlatformSnapWorld;
            PlatformComponentBase* otherPlatform = this->findOtherPlatformNearby(hitPosition, this->snapRadius, otherPlatformSnapWorld);
            if (nullptr != otherPlatform)
            {
                hitPosition = otherPlatformSnapWorld;
                this->pendingCrossNetworkSnap = true;
                this->pendingMergeOtherPlatform = dynamic_cast<ProceduralPlatformComponent*>(otherPlatform);
            }
            else
            {
                this->pendingCrossNetworkSnap = false;
                this->pendingMergeOtherPlatform = nullptr;
            }

            hitPosition = this->platformFrame.Inverse() * hitPosition;

            if (this->snapToGrid->getBool() && nullptr == otherPlatform)
            {
                hitPosition = this->snapToGridFunc(hitPosition);
            }

            if (this->buildState == BuildState::IDLE)
            {
                if (this->isShiftPressed && this->hasLoadedPlatformEndpoint && false == this->platformSegments.empty())
                {
                    PlatformControlPoint startPoint;
                    startPoint.position = this->loadedPlatformEndpoint;
                    startPoint.position.y = 0.0f;
                    startPoint.rawHeight = this->loadedPlatformEndpointHeight;
                    startPoint.smoothedHeight = startPoint.rawHeight;
                    startPoint.distFromStart = 0.0f;

                    this->currentSegment.controlPoints.clear();
                    this->currentSegment.controlPoints.push_back(startPoint);
                    this->currentSegment.isCurved = false;
                    this->currentSegment.curvature = 0.0f;

                    this->buildState = BuildState::DRAGGING;
                    this->lastValidPosition = startPoint.position;
                }
                else
                {
                    this->startPlatformPlacement(hitPosition);
                    this->hasLoadedPlatformEndpoint = false;
                }
            }
            else if (this->buildState == BuildState::DRAGGING)
            {
                this->confirmPlatform();
            }

            return false;
        }

        return false;
    }

    bool ProceduralPlatformComponent::mouseMoved(const OIS::MouseEvent& evt)
    {
        if (false == this->activated->getBool())
        {
            return true;
        }

        const bool wantPreview = (this->buildState == BuildState::DRAGGING) && (this->getEditModeEnum() == EditMode::OBJECT || this->isExtendingFromSegment);

        if (false == wantPreview)
        {
            return true;
        }

        Ogre::Real screenX = 0.0f;
        Ogre::Real screenY = 0.0f;
        MathHelper::getInstance()->mouseToViewPort(evt.state.X.abs, evt.state.Y.abs, screenX, screenY, Core::getSingletonPtr()->getOgreRenderWindow());

        Ogre::Vector3 hitPosition;
        if (this->raycastFixedPlane(screenX, screenY, hitPosition))
        {
            Ogre::Vector3 otherPlatformSnapWorld;
            PlatformComponentBase* otherPlatform = this->findOtherPlatformNearby(hitPosition, this->snapRadius, otherPlatformSnapWorld);
            bool snappedToOtherPlatform = false;
            if (nullptr != otherPlatform)
            {
                hitPosition = otherPlatformSnapWorld;
                snappedToOtherPlatform = true;
            }

            hitPosition = this->platformFrame.Inverse() * hitPosition;

            if (true == this->snapToGrid->getBool() && false == snappedToOtherPlatform)
            {
                hitPosition = this->snapToGridFunc(hitPosition);
            }

            Ogre::Vector3 previewPos = hitPosition;

            if (false == snappedToOtherPlatform)
            {
                const Ogre::Real sr = this->platformDepth->getReal() * 0.6f;
                this->detectSnapToOwnPlatform(hitPosition, sr);
                previewPos = this->isSnapToOwnPlatform ? this->snapToPlatformPoint : hitPosition;
                this->pendingCrossNetworkSnap = false;
                this->pendingMergeOtherPlatform = nullptr;
            }
            else
            {
                // Reuse the existing snap-indicator state for the cross-network case too -
                // no separate indicator state needed.
                this->isSnapToOwnPlatform = true;
                this->snapToPlatformPoint = hitPosition;
                previewPos = hitPosition;

                this->pendingCrossNetworkSnap = true;
                this->pendingMergeOtherPlatform = dynamic_cast<ProceduralPlatformComponent*>(otherPlatform);
            }

            this->updatePlatformPreview(previewPos);
            this->scheduleSnapIndicatorUpdate();
        }

        return true;
    }

    bool ProceduralPlatformComponent::mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
    {
        if (false == this->activated->getBool())
        {
            return true; // not handled -> bubble
        }

        if (id == OIS::MB_Right)
        {
            // Cancel current platform segment in progress
            this->cancelPlatform();
            // Cancel both normal drag and extend-from-segment drag
            this->isExtendingFromSegment = false;
            if (this->getEditModeEnum() != EditMode::SEGMENT)
            {
                // Remove listeners - user wants to stop building
                this->removeInputListener();
            }

            return false;
        }

        return true; // not handled -> bubble
    }

    bool ProceduralPlatformComponent::keyPressed(const OIS::KeyEvent& evt)
    {
        if (false == this->activated->getBool())
        {
            return true;
        }

        if (evt.key == OIS::KC_LSHIFT || evt.key == OIS::KC_RSHIFT)
        {
            this->isShiftPressed = true;
            return false;
        }
        else if (evt.key == OIS::KC_LCONTROL || evt.key == OIS::KC_RCONTROL)
        {
            this->isCtrlPressed = true;
            return false;
        }

        // ── SEGMENT MODE key handling ─────────────────────────────────────────
        if (this->getEditModeEnum() == EditMode::SEGMENT)
        {
            if (evt.key == OIS::KC_X && this->selectedSegmentIndex >= 0)
            {
                this->deleteSelectedSegment();
                return false;
            }

            if (evt.key == OIS::KC_E && this->selectedSegmentIndex >= 0)
            {
                const PlatformSegment& sel = this->platformSegments[this->selectedSegmentIndex];
                const PlatformControlPoint& tail = sel.controlPoints.back();

                PlatformControlPoint startPoint;
                startPoint.position = tail.position;
                startPoint.position.y = 0.0f;
                startPoint.rawHeight = tail.smoothedHeight;
                startPoint.smoothedHeight = tail.smoothedHeight;
                startPoint.distFromStart = 0.0f;

                this->currentSegment.controlPoints.clear();
                this->currentSegment.controlPoints.push_back(startPoint);
                this->currentSegment.isCurved = false;
                this->currentSegment.curvature = 0.0f;

                this->buildState = BuildState::DRAGGING;
                this->lastValidPosition = startPoint.position;
                this->isShiftPressed = true;         // auto-chain on confirm
                this->isExtendingFromSegment = true; // allow preview in segment mode

                return false;
            }

            if (evt.key == OIS::KC_ESCAPE)
            {
                if (this->isExtendingFromSegment)
                {
                    this->isExtendingFromSegment = false;
                    this->buildState = BuildState::IDLE;
                    this->destroyPreviewMesh();
                }
                else
                {
                    this->selectedSegmentIndex = -1;
                    this->scheduleSegmentOverlayUpdate();
                }
                return false;
            }

            return true; // all other keys bubble in segment mode
        }

        // ── OBJECT MODE key handling ──────────────────────────────────────────
        if (evt.key == OIS::KC_Z && this->isCtrlPressed)
        {
            this->removeLastSegment();
            return false;
        }
        else if (evt.key == OIS::KC_ESCAPE)
        {
            this->cancelPlatform();
            this->removeInputListener();
            return false;
        }

        return true;
    }

    bool ProceduralPlatformComponent::keyReleased(const OIS::KeyEvent& evt)
    {
        if (false == this->activated->getBool())
        {
            return true;
        }

        if (evt.key == OIS::KC_LSHIFT || evt.key == OIS::KC_RSHIFT)
        {
            this->isShiftPressed = false;
        }
        else if (evt.key == OIS::KC_LCONTROL || evt.key == OIS::KC_RCONTROL)
        {
            this->isCtrlPressed = false;
        }

        return false; // handled -> do not bubble
    }

    bool ProceduralPlatformComponent::executeAction(const Ogre::String& actionId, NOWA::Variant* attribute)
    {
        if ("ProceduralPlatformComponent.ConvertToMesh" == actionId)
        {
            return this->convertToMeshApply();
        }
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////

    bool ProceduralPlatformComponent::raycastFixedPlane(Ogre::Real screenX, Ogre::Real screenY, Ogre::Vector3& hitPosition)
    {
        Ogre::Camera* camera = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
        if (nullptr == camera)
        {
            return false;
        }

        Ogre::Ray ray = camera->getCameraToViewportRay(screenX, screenY);

        // Plane through platformPlaneAnchor with normal = platformFrame * UNIT_Z. Using the
        // (normal, d) constructor confirmed by ProceduralRoadComponent's own fallback plane
        // usage (Ogre::Plane(normal, constant) where normal.dotProduct(p) == constant for
        // any point p on the plane), rather than assuming a (normal, point) overload exists.
        const Ogre::Vector3 planeNormal = this->platformFrame * Ogre::Vector3::UNIT_Z;
        const Ogre::Real planeD = planeNormal.dotProduct(this->platformPlaneAnchor);
        Ogre::Plane workingPlane(planeNormal, planeD);

        std::pair<bool, Ogre::Real> result = ray.intersects(workingPlane);
        if (true == result.first && result.second > 0.0f)
        {
            hitPosition = ray.getPoint(result.second);
            return true;
        }

        return false;
    }

    void ProceduralPlatformComponent::startPlatformPlacement(const Ogre::Vector3& worldPosition)
    {
        Ogre::Vector3 startPos = this->snapToGrid->getBool() ? this->snapToGridFunc(worldPosition) : worldPosition;

        PlatformControlPoint startPoint;
        startPoint.position = startPos;
        startPoint.position.y = 0.0f; // horizontal only, height is separate
        // Unlike ProceduralRoadComponent::getGroundHeight (a SECOND raycast, needed
        // because the first click's hit might have landed on an arbitrary obstacle rather
        // than the ground), the fixed-plane hit already IS the intended point - no second
        // raycast needed, its local Y component is directly the platform's height here.
        startPoint.rawHeight = startPos.y;
        startPoint.smoothedHeight = startPoint.rawHeight;
        startPoint.distFromStart = 0.0f;

        this->currentSegment.controlPoints.clear();
        this->currentSegment.controlPoints.push_back(startPoint);
        this->currentSegment.isCurved = false;
        this->currentSegment.curvature = 0.0f;

        if (false == this->hasPlatformOrigin)
        {
            this->platformOrigin = startPoint.position;
            this->platformOrigin.y = startPoint.rawHeight;
            this->hasPlatformOrigin = true;
        }

        this->buildState = BuildState::DRAGGING;
        this->lastValidPosition = startPoint.position;
    }

    void ProceduralPlatformComponent::updatePlatformPreview(const Ogre::Vector3& worldPosition)
    {
        if (true == this->currentSegment.controlPoints.empty())
        {
            return;
        }

        // If isSnapToOwnPlatform is already true (set by either detectSnapToOwnPlatform for
        // an own-network snap, or by mouseMoved for a cross-network snap), worldPosition IS
        // ALREADY the resolved, exact snap point - re-applying grid snapping here would
        // silently nudge it off that point again.
        Ogre::Vector3 currentPos = (this->snapToGrid->getBool() && false == this->isSnapToOwnPlatform) ? this->snapToGridFunc(worldPosition) : worldPosition;

        // Constrain to axis if ctrl is held: lock either the horizontal position or the
        // height, whichever moved less - mirrors ProceduralRoadComponent's X/Z axis
        // constraint, just on this component's two in-plane axes (horizontal, height)
        // instead of Road's two ground-plane axes (X, Z).
        if (true == this->isCtrlPressed)
        {
            Ogre::Vector3 delta = currentPos - this->currentSegment.controlPoints.front().position;
            Ogre::Real deltaHeight = std::abs(currentPos.y - this->currentSegment.controlPoints.front().rawHeight);
            if (std::abs(delta.x) > deltaHeight)
            {
                currentPos.y = this->currentSegment.controlPoints.front().rawHeight;
            }
            else
            {
                currentPos.x = this->currentSegment.controlPoints.front().position.x;
            }
        }

        // REMOVED: the Max Turn Angle drag clamp.
        //
        // It used to rotate the dragged endpoint back onto an allowed cone around the
        // previous segment's direction whenever the turn exceeded Max Turn Angle. That is
        // exactly the "strange offset once some angle is exceeded" seen while drawing a
        // loop: the preview endpoint silently stops following the mouse and swings to the
        // cone boundary instead, so each segment of the loop lands somewhere the user did
        // not point at.
        //
        // Its original justification was that a sharp reversal would self-intersect the
        // fixed-depth extrusion, so it had to be made undrawable. That is no longer true:
        // generatePlatformBox miters along the path normal now, and rebuildMesh classifies
        // each waypoint by its real turn angle - a bend is swept as one continuous run, and
        // only a near-180 degree hairpin is cut into two capped runs. The geometry copes
        // with any turn the user can draw, so nothing needs to be forbidden at drag time.

        PlatformControlPoint endPoint;
        endPoint.position = currentPos;
        endPoint.position.y = 0.0f;
        endPoint.rawHeight = currentPos.y;
        endPoint.smoothedHeight = endPoint.rawHeight;
        // NOTE: mirrors ProceduralRoadComponent's own preview distFromStart exactly -
        // front().position always has y=0 (placeholder) while currentPos still carries its
        // real height at this point, so this is a slightly approximate PREVIEW distance.
        // rebuildMesh() recomputes the real accumulated distance properly once confirmed
        // (see the accumDist loop there) - this value is only ever used for the live preview
        // mesh's UVs before confirm.
        endPoint.distFromStart = this->currentSegment.controlPoints.front().position.distance(currentPos);

        // Update or add the end point
        if (this->currentSegment.controlPoints.size() == 1)
        {
            this->currentSegment.controlPoints.push_back(endPoint);
        }
        else
        {
            this->currentSegment.controlPoints.back() = endPoint;
        }

        this->lastValidPosition = currentPos;

        // Create visual preview
        this->updatePreviewMesh();
    }

    void ProceduralPlatformComponent::confirmPlatform(void)
    {
        if (this->buildState != BuildState::DRAGGING)
        {
            return;
        }

        if (this->currentSegment.controlPoints.size() < 2)
        {
            return;
        }

        Ogre::Real length = this->currentSegment.controlPoints.front().position.distance(this->currentSegment.controlPoints.back().position);
        if (length < 0.05f && std::abs(this->currentSegment.controlPoints.front().rawHeight - this->currentSegment.controlPoints.back().rawHeight) < 0.05f)
        {
            this->cancelPlatform();
            return;
        }

        // ── Snap-to-platform: just override the endpoint position ─────────────
        const bool wasSnapping = this->isSnapToOwnPlatform;

        if (wasSnapping)
        {
            PlatformControlPoint& endCP = this->currentSegment.controlPoints.back();
            endCP.position = this->snapToPlatformPoint;
            endCP.position.y = 0.0f;
            endCP.rawHeight = this->snapToPlatformPoint.y;
            endCP.smoothedHeight = this->snapToPlatformPoint.y;

            this->isSnapToOwnPlatform = false;
            this->snapToPlatformSegmentIdx = -1;
            this->pendingCrossNetworkSnap = false;
        }

        // Closing a loop always terminates - never keep chaining
        bool shouldChain = this->isShiftPressed && !wasSnapping;

        boost::shared_ptr<NOWA::EventDataCommandTransactionBegin> eventDataUndoBegin(new NOWA::EventDataCommandTransactionBegin("Add Platform Segment"));
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataUndoBegin);

        std::vector<unsigned char> oldData = this->getPlatformData();

        this->smoothHeightTransitions(this->currentSegment.controlPoints);

        PlatformControlPoint exactEndpoint = this->currentSegment.controlPoints.back();

        this->platformSegments.push_back(this->currentSegment);

        this->destroyPreviewMesh();
        if (this->previewNode)
        {
            this->previewNode->setPosition(Ogre::Vector3::ZERO);
        }

        this->rebuildMesh();

        std::vector<unsigned char> newData = this->getPlatformData();

        boost::shared_ptr<EventDataPlatformModifyEnd> eventDataPlatformModifyEnd(new EventDataPlatformModifyEnd(oldData, newData, this->gameObjectPtr->getId()));
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataPlatformModifyEnd);

        boost::shared_ptr<NOWA::EventDataCommandTransactionEnd> eventDataUndoEnd(new NOWA::EventDataCommandTransactionEnd());
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataUndoEnd);

        this->updateContinuationPoint();

        if (shouldChain)
        {
            PlatformControlPoint startPoint;
            startPoint.position = exactEndpoint.position;
            startPoint.position.y = 0.0f;
            startPoint.rawHeight = exactEndpoint.smoothedHeight;
            startPoint.smoothedHeight = exactEndpoint.smoothedHeight;
            startPoint.distFromStart = 0.0f;

            this->currentSegment.controlPoints.clear();
            this->currentSegment.controlPoints.push_back(startPoint);
            this->currentSegment.isCurved = false;
            this->currentSegment.curvature = 0.0f;

            this->buildState = BuildState::DRAGGING;
            this->lastValidPosition = startPoint.position;
        }
        else
        {
            this->buildState = BuildState::IDLE;
            this->currentSegment.controlPoints.clear();
            this->isExtendingFromSegment = false;
            this->removeInputListener();
        }

        // >>> MERGE: if this confirm closed a cross-network snap, absorb the other
        // platform's segments into this one and remove its GameObject. Placed at the very
        // end, after our own segment is fully committed (pushed, meshed, undo-recorded), so
        // the merge only ever touches an already-consistent state. Mirrors
        // ProceduralRoadComponent::confirmRoad exactly.
        if (nullptr != this->pendingMergeOtherPlatform)
        {
            ProceduralPlatformComponent* otherPlatform = this->pendingMergeOtherPlatform;
            this->pendingMergeOtherPlatform = nullptr;
            this->mergeOtherPlatformIntoThis(otherPlatform);
        }
    }

    void ProceduralPlatformComponent::updateContinuationPoint(void)
    {
        if (!this->platformSegments.empty())
        {
            const PlatformSegment& lastSegment = this->platformSegments.back();
            if (!lastSegment.controlPoints.empty())
            {
                const PlatformControlPoint& lastCP = lastSegment.controlPoints.back();

                this->loadedPlatformEndpoint = lastCP.position;
                this->loadedPlatformEndpointHeight = lastCP.smoothedHeight;
                this->hasLoadedPlatformEndpoint = true;
            }
        }
        else
        {
            this->hasLoadedPlatformEndpoint = false;
        }
    }

    void ProceduralPlatformComponent::cancelPlatform(void)
    {
        this->destroyPreviewMesh();
        this->buildState = BuildState::IDLE;
        this->currentSegment.controlPoints.clear();

        // Reset modifier key flags when canceling
        this->isShiftPressed = false;
        this->isCtrlPressed = false;
    }

    void ProceduralPlatformComponent::removeLastSegment(void)
    {
        if (true == this->platformSegments.empty())
        {
            return;
        }

        std::vector<unsigned char> oldData = this->getPlatformData();

        this->platformSegments.pop_back();

        if (true == this->platformSegments.empty())
        {
            this->destroyPlatformMesh();
            this->hasPlatformOrigin = false;
            this->hasLoadedPlatformEndpoint = false;
        }
        else
        {
            this->rebuildMesh();
            this->updateContinuationPoint();
        }

        std::vector<unsigned char> newData = this->getPlatformData();

        boost::shared_ptr<EventDataPlatformModifyEnd> eventDataPlatformModifyEnd(new EventDataPlatformModifyEnd(oldData, newData, this->gameObjectPtr->getId()));
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataPlatformModifyEnd);
    }

    void ProceduralPlatformComponent::clearAllSegments(void)
    {
        if (true == this->platformSegments.empty())
        {
            return;
        }

        std::vector<unsigned char> oldData = this->getPlatformData();

        this->platformSegments.clear();
        this->destroyPlatformMesh();
        this->hasPlatformOrigin = false;
        this->hasLoadedPlatformEndpoint = false;
        this->bBatchMode = false;
        this->originPositionSet = false;

        if (false == AppStateManager::getSingletonPtr()->getGameObjectController()->getIsDestroying())
        {
            std::vector<unsigned char> newData; // Empty = cleared platform

            boost::shared_ptr<EventDataPlatformModifyEnd> eventDataPlatformModifyEnd(new EventDataPlatformModifyEnd(oldData, newData, this->gameObjectPtr->getId()));
            NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataPlatformModifyEnd);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Mesh Generation
    ///////////////////////////////////////////////////////////////////////////////////////////////

    namespace
    {
        // Per-run depth override for generatePlatformBox. Not a class member - deliberately
        // file-local so no header change is needed. rebuildMesh sets this to a slightly
        // smaller-than-platformDepth value for every monotonic run after the first one at a
        // direction-reversal split (see the loop below), then resets it to the -1 sentinel
        // (meaning "use this->platformDepth as normal") right after that one generatePlatformSegment
        // call, so every other call to generatePlatformBox anywhere else is completely
        // unaffected. See the comment at its use site in rebuildMesh for why: however
        // precisely the split point is located, two runs meeting at an exact corner still
        // produce end caps that sit in the exact same plane there and can z-fight/overlap by
        // even a fraction of a millimetre of leftover geometry on either side. A tiny,
        // deliberate depth taper on the later run means its whole cross-section (and end cap)
        // is nested strictly inside the earlier run's at that shared corner instead of being
        // coincident with it, so there is no plane left to fight over.
        thread_local Ogre::Real g_platformRunDepthOverride = -1.0f;

        // Platform paths live in exactly TWO axes: horizontal (x) and height - unlike
        // ProceduralRoadComponent, where XZ (ground plane) is one independent quantity and Y
        // (terrain height) is a SEPARATE, independently-raycast quantity that Road
        // deliberately excludes from its own arc-length/distance math (position.y is always
        // zeroed before any position.distance() call there). For platform, height is NOT
        // independent - it is exactly as much a part of the path as the horizontal axis (a
        // steep ramp, or a near-vertical ladder-like chain, has real path LENGTH coming
        // almost entirely from height), so every arc-length/gradient calculation in this file
        // combines x AND height together via this helper, rather than reusing
        // Ogre::Vector3::distance() the way the reference does (which would silently drop the
        // height term here, since position.y/z stay at 0 as placeholders - see
        // PlatformControlPoint's comment in the header).
        inline Ogre::Real pathDistance2D(Ogre::Real x0, Ogre::Real h0, Ogre::Real x1, Ogre::Real h1)
        {
            const Ogre::Real dx = x1 - x0;
            const Ogre::Real dh = h1 - h0;
            return std::sqrt(dx * dx + dh * dh);
        }
    }

    void ProceduralPlatformComponent::rebuildMesh(void)
    {
        this->surfaceVertices.clear();
        this->surfaceIndices.clear();
        this->currentSurfaceVertexIndex = 0;

        this->groundVertices.clear();
        this->groundIndices.clear();
        this->currentGroundVertexIndex = 0;

        this->junctionVertices.clear();
        this->junctionIndices.clear();
        this->currentJunctionVertexIndex = 0;

        if (this->platformSegments.empty())
        {
            return;
        }

        Ogre::Vector3 originToUse = this->platformOrigin;
        struct QKey
        {
            int ix, ih;
            bool operator<(const QKey& o) const
            {
                return ix < o.ix || (ix == o.ix && ih < o.ih);
            }
        };
        auto quantise = [](float v) -> int
        {
            return static_cast<int>(std::round(v * 10.0f));
        };

        // ── Junction detection ─────────────────────────────────────────────────
        std::map<QKey, std::vector<size_t>> endpointMap;
        for (size_t si = 0; si < this->platformSegments.size(); ++si)
        {
            const PlatformSegment& seg = this->platformSegments[si];
            if (seg.controlPoints.size() < 2)
            {
                continue;
            }
            for (const PlatformControlPoint* cp : {&seg.controlPoints.front(), &seg.controlPoints.back()})
            {
                QKey k{quantise(cp->position.x), quantise(cp->rawHeight)};
                endpointMap[k].push_back(si);
            }
        }

        std::vector<JunctionPoint> junctions;
        std::set<QKey> seenJunctions;

        for (auto& [key, segs] : endpointMap)
        {
            std::set<size_t> distinct(segs.begin(), segs.end());
            if (distinct.size() < 3)
            {
                continue;
            }
            if (seenJunctions.count(key))
            {
                continue;
            }
            seenJunctions.insert(key);

            JunctionPoint jp;
            bool found = false;
            for (size_t si2 : distinct)
            {
                if (found)
                {
                    break;
                }
                for (const auto& cp : this->platformSegments[si2].controlPoints)
                {
                    if (quantise(cp.position.x) == key.ix && quantise(cp.rawHeight) == key.ih)
                    {
                        jp.worldPos = Ogre::Vector3(cp.position.x, cp.smoothedHeight, 0.0f);
                        found = true;
                        break;
                    }
                }
            }

            jp.segIndices.assign(distinct.begin(), distinct.end());

            for (size_t si2 : distinct)
            {
                const PlatformSegment& seg2 = this->platformSegments[si2];
                bool isStart = (quantise(seg2.controlPoints.front().position.x) == key.ix && quantise(seg2.controlPoints.front().rawHeight) == key.ih);
                const PlatformControlPoint& otherCp = isStart ? seg2.controlPoints.back() : seg2.controlPoints.front();
                Ogre::Vector3 dir(otherCp.position.x - jp.worldPos.x, otherCp.smoothedHeight - jp.worldPos.y, 0.0f);
                if (dir.squaredLength() > 1e-6f)
                {
                    dir.normalise();
                }
                // Kept purely so the [JUNCTION-DEBUG] log can report which way each arm
                // leaves the junction. The angle-driven armTrimDists that used to be
                // computed here are gone with the trimming itself - arms are no longer cut
                // back at a junction at all (see generateJunctionPatch).
                jp.armDirs.push_back(dir);
            }

            junctions.push_back(jp);
        }

        std::map<QKey, size_t> junctionByKey;
        for (size_t ji = 0; ji < junctions.size(); ++ji)
        {
            QKey k{quantise(junctions[ji].worldPos.x), quantise(junctions[ji].worldPos.y)};
            junctionByKey[k] = ji;
        }

        // Records the trimmed-boundary point for one arm, as (x, height, 0) - one point per
        // arm, unlike ProceduralRoadComponent's two (left/right width-offset corners) per
        // arm - see the header comment on JunctionPoint::patchCorners for why. depthAtArm is
        // whatever depth THIS arm's own connecting geometry actually uses there (see
        // JunctionPoint::armDepths) - lets the junction fan match each arm's real footprint
        // instead of assuming one uniform depth for the whole fan.
        auto storePatchCorner = [&](size_t ji, Ogre::Real x, Ogre::Real height, Ogre::Real depthAtArm)
        {
            junctions[ji].patchCorners.push_back(Ogre::Vector3(x, height, 0.0f));
            junctions[ji].armDepths.push_back(depthAtArm);
        };

        // ── Chain building (undirected graph walk, handles 2-way snap joins) ────
        // NOTE: unlike ProceduralRoadComponent, closed-loop chains are not supported here
        // (see the class doc comment and header) - no closedLoop tracking needed.
        std::map<QKey, std::vector<std::pair<size_t, bool>>> pointSegs; // bool = touches via FRONT
        for (size_t si = 0; si < this->platformSegments.size(); ++si)
        {
            const PlatformSegment& seg = this->platformSegments[si];
            if (seg.controlPoints.size() < 2)
            {
                continue;
            }
            QKey kf{quantise(seg.controlPoints.front().position.x), quantise(seg.controlPoints.front().rawHeight)};
            QKey kb{quantise(seg.controlPoints.back().position.x), quantise(seg.controlPoints.back().rawHeight)};
            pointSegs[kf].push_back({si, true});
            pointSegs[kb].push_back({si, false});
        }

        auto distinctCountAt = [&](const QKey& k) -> size_t
        {
            auto it = pointSegs.find(k);
            if (it == pointSegs.end())
            {
                return 0;
            }
            std::set<size_t> distinctSegs;
            for (const auto& e : it->second)
            {
                distinctSegs.insert(e.first);
            }
            return distinctSegs.size();
        };

        std::vector<bool> processed(this->platformSegments.size(), false);

        // Per-junction arm counter, used to nest each arm's depth slightly (see the
        // g_platformRunDepthOverride assignment further down). The first arm registered at a
        // junction keeps the full platform depth, every following one is inset by 1 cm, so no
        // two arms of the same junction share the z = +/- depth/2 planes their front and back
        // faces live on. Without this, two arms whose footprints overlap in XY - which is now
        // the normal case, since arms are no longer trimmed back - would z-fight on exactly
        // those two planes.
        std::map<size_t, int> junctionArmCounter;

        for (size_t i = 0; i < this->platformSegments.size(); ++i)
        {
            if (processed[i])
            {
                continue;
            }

            std::vector<std::pair<size_t, bool>> chainIndices;
            chainIndices.push_back({i, false});
            processed[i] = true;

            // ── Extend forward from the chain's tail ───────────────────────────
            {
                bool extending = true;
                while (extending)
                {
                    extending = false;

                    const PlatformSegment& tailSeg = this->platformSegments[chainIndices.back().first];
                    const bool tailReversed = chainIndices.back().second;
                    const PlatformControlPoint& tailCp = tailReversed ? tailSeg.controlPoints.front() : tailSeg.controlPoints.back();
                    QKey tailKey{quantise(tailCp.position.x), quantise(tailCp.rawHeight)};

                    if (distinctCountAt(tailKey) >= 3)
                    {
                        break;
                    }

                    auto it = pointSegs.find(tailKey);
                    if (it == pointSegs.end())
                    {
                        break;
                    }

                    for (const auto& entry : it->second)
                    {
                        const size_t nextIdx = entry.first;
                        const bool nextTouchesViaFront = entry.second;

                        if (processed[nextIdx])
                        {
                            continue;
                        }

                        const bool nextReversed = !nextTouchesViaFront;
                        chainIndices.push_back({nextIdx, nextReversed});
                        processed[nextIdx] = true;
                        extending = true;
                        break;
                    }
                }
            }

            // ── Extend backward from the chain's head ───────────────────────────
            {
                bool extending = true;
                while (extending)
                {
                    extending = false;

                    const PlatformSegment& headSeg = this->platformSegments[chainIndices.front().first];
                    const bool headReversed = chainIndices.front().second;
                    const PlatformControlPoint& headCp = headReversed ? headSeg.controlPoints.back() : headSeg.controlPoints.front();
                    QKey headKey{quantise(headCp.position.x), quantise(headCp.rawHeight)};

                    if (distinctCountAt(headKey) >= 3)
                    {
                        break;
                    }

                    auto it = pointSegs.find(headKey);
                    if (it == pointSegs.end())
                    {
                        break;
                    }

                    for (const auto& entry : it->second)
                    {
                        const size_t nextIdx = entry.first;
                        const bool nextTouchesViaFront = entry.second;

                        if (processed[nextIdx])
                        {
                            continue;
                        }

                        const bool nextReversed = nextTouchesViaFront;
                        chainIndices.insert(chainIndices.begin(), {nextIdx, nextReversed});
                        processed[nextIdx] = true;
                        extending = true;
                        break;
                    }
                }
            }

            // ── Collect waypoints (respecting per-segment reversal) ────────────
            std::vector<PlatformControlPoint> chainWaypoints;
            for (size_t ci = 0; ci < chainIndices.size(); ++ci)
            {
                const PlatformSegment& seg = this->platformSegments[chainIndices[ci].first];
                const bool reversed = chainIndices[ci].second;

                if (false == reversed)
                {
                    for (size_t pi = 0; pi < seg.controlPoints.size(); ++pi)
                    {
                        const PlatformControlPoint& cp = seg.controlPoints[pi];
                        if (false == chainWaypoints.empty() && pathDistance2D(chainWaypoints.back().position.x, chainWaypoints.back().rawHeight, cp.position.x, cp.rawHeight) < 0.01f)
                        {
                            continue;
                        }
                        chainWaypoints.push_back(cp);
                    }
                }
                else
                {
                    for (size_t pi = seg.controlPoints.size(); pi-- > 0;)
                    {
                        const PlatformControlPoint& cp = seg.controlPoints[pi];
                        if (false == chainWaypoints.empty() && pathDistance2D(chainWaypoints.back().position.x, chainWaypoints.back().rawHeight, cp.position.x, cp.rawHeight) < 0.01f)
                        {
                            continue;
                        }
                        chainWaypoints.push_back(cp);
                    }
                }
            }

            if (chainWaypoints.size() < 2)
            {
                continue;
            }

            // ── Classify every interior waypoint by its TURN ANGLE ───────────────
            // REPLACES the old X-sign-flip reversal detection. That test was the last
            // leftover of the road model, where the path is a height function over a 1D
            // horizontal axis: "the path reversed" was taken to mean "dx changed sign".
            // Now that generatePlatformBox extrudes along the path normal (see the miter
            // rework there), the path is genuinely 2D and dx tells us nothing useful - a
            // vertical wall has dx = 0 throughout, so noise decided whether it counted as a
            // reversal, and a real 90 degree corner between a vertical and a horizontal leg
            // registered as no turn at all.
            //
            // The angle between the incoming and outgoing tangent is the correct measure,
            // and it splits into three cases:
            //   SMOOTH  (turn <  cornerTurnThresholdDeg) - Catmull-Rom curves through it.
            //   CORNER  (turn >= cornerTurnThresholdDeg) - kept as an exact vertex. The
            //           curve is cut here so it does not round the corner off, but the path
            //           stays CONTINUOUS: the mitered extrusion joins the two legs by
            //           itself, which is exactly what an L-shaped wall-into-ceiling needs.
            //   REVERSAL(turn >= reversalTurnThresholdDeg) - a genuine hairpin. The miter
            //           cannot resolve a near-180 degree fold, so the chain is cut into two
            //           separately meshed runs here, each with its own end cap and a 1 cm
            //           depth nesting offset (unchanged from before).
            //
            // TUNING: cornerTurnThresholdDeg decides what still counts as "a curve the user
            // drew" versus "a corner the user drew". It has to sit above the per-waypoint
            // turn of a deliberately smooth arc (a hand-placed loop turns roughly 30-45
            // degrees per waypoint) and below a structural corner (90 degrees). 60 is the
            // midpoint of that gap.
            const Ogre::Real cornerTurnThresholdDeg = 60.0f;
            const Ogre::Real reversalTurnThresholdDeg = 160.0f;

            std::vector<size_t> cornerWaypoints;   // indices into chainWaypoints - curve is cut, path stays continuous
            std::vector<size_t> reversalWaypoints; // indices into chainWaypoints - mesh is cut into separate runs
            for (size_t wi = 1; wi + 1 < chainWaypoints.size(); ++wi)
            {
                Ogre::Vector2 tangentIn(chainWaypoints[wi].position.x - chainWaypoints[wi - 1].position.x, chainWaypoints[wi].smoothedHeight - chainWaypoints[wi - 1].smoothedHeight);
                Ogre::Vector2 tangentOut(chainWaypoints[wi + 1].position.x - chainWaypoints[wi].position.x, chainWaypoints[wi + 1].smoothedHeight - chainWaypoints[wi].smoothedHeight);

                if (tangentIn.squaredLength() < 1e-8f || tangentOut.squaredLength() < 1e-8f)
                {
                    continue;
                }

                tangentIn.normalise();
                tangentOut.normalise();

                const Ogre::Real cosTurn = Ogre::Math::Clamp(tangentIn.dotProduct(tangentOut), -1.0f, 1.0f);
                const Ogre::Real turnDeg = Ogre::Math::RadiansToDegrees(std::acos(cosTurn));

                if (turnDeg >= reversalTurnThresholdDeg)
                {
                    reversalWaypoints.push_back(wi);
                    cornerWaypoints.push_back(wi);
                }
                else if (turnDeg >= cornerTurnThresholdDeg)
                {
                    cornerWaypoints.push_back(wi);
                }
            }

            // ── Build final path, one smooth group at a time ─────────────────────
            // A "group" is a maximal stretch of waypoints containing no corner in its
            // interior. Curving, uniform resampling and height smoothing all run PER GROUP,
            // so a corner waypoint is always the first or last point of its group - and all
            // three of those steps leave first and last points exactly where they are. That
            // is what keeps an authored corner sharp: previously the whole chain went
            // through one Catmull-Rom pass, which threaded a smooth curve through the corner
            // and turned a vertical-plus-horizontal L into a single banana-shaped bend.
            //
            // The groups are concatenated into ONE finalPath (the shared corner point is not
            // duplicated), so the result is still a single continuous path. Only a REVERSAL
            // additionally cuts the mesh, and its index in finalPath is recorded directly
            // here - which replaces the old two-pass "find the authored corner's nearest
            // dense sample, then refine to the true local X extremum" search entirely. That
            // search existed only because the split points were expressed as world X values;
            // knowing the index at construction time is both exact and far simpler.
            std::vector<PlatformControlPoint> finalPath;
            std::vector<size_t> reversalPathIndices;

            {
                std::vector<size_t> groupBounds;
                groupBounds.push_back(0);
                for (size_t ci : cornerWaypoints)
                {
                    groupBounds.push_back(ci);
                }
                groupBounds.push_back(chainWaypoints.size() - 1);

                const int subdivisions = this->curveSubdivisions->getInt();

                for (size_t gi = 0; gi + 1 < groupBounds.size(); ++gi)
                {
                    const size_t lo = groupBounds[gi];
                    const size_t hi = groupBounds[gi + 1];
                    if (hi <= lo)
                    {
                        continue;
                    }

                    std::vector<PlatformControlPoint> groupWaypoints(chainWaypoints.begin() + lo, chainWaypoints.begin() + hi + 1);

                    std::vector<PlatformControlPoint> groupPath;
                    if (groupWaypoints.size() >= 3)
                    {
                        std::vector<PlatformControlPoint> densePath;
                        densePath.reserve((groupWaypoints.size() - 1) * static_cast<size_t>(subdivisions) + 1);

                        for (size_t pi = 0; pi + 1 < groupWaypoints.size(); ++pi)
                        {
                            for (int j = 0; j < subdivisions; ++j)
                            {
                                Ogre::Real t = static_cast<Ogre::Real>(j) / subdivisions;
                                Ogre::Real globalT = static_cast<Ogre::Real>(pi) + t;

                                // Interpolates x AND height TOGETHER - see evaluateCatmullRom's
                                // own comment for why there is no independent oracle to consult
                                // afterward the way ProceduralRoadComponent consults terrain.
                                Ogre::Vector2 xh = this->evaluateCatmullRom(groupWaypoints, globalT);

                                PlatformControlPoint cp;
                                cp.position = Ogre::Vector3(xh.x, 0.0f, 0.0f);
                                cp.rawHeight = xh.y;
                                if (0 == pi && 0 == j)
                                {
                                    cp.rawHeight = groupWaypoints.front().smoothedHeight;
                                }
                                cp.smoothedHeight = cp.rawHeight;
                                cp.distFromStart = 0.0f;
                                densePath.push_back(cp);
                            }
                        }
                        PlatformControlPoint lastCp = groupWaypoints.back();
                        lastCp.position.y = 0.0f;
                        densePath.push_back(lastCp);

                        // Reparametrize to uniform arc-length spacing - fixes the same
                        // texture-stretching-on-curves issue ProceduralRoadComponent's
                        // resamplePathUniformly was written for. BUGFIX: the step must scale
                        // with curveSubdivisions, not be a hardcoded meter value - a fixed step
                        // (e.g. 1m) silently caps the final mesh resolution at ~1 vertex/meter
                        // regardless of how high Curve Subdivisions is set, which is exactly the
                        // faceted/angular look on tight curves even at Curve Subdivisions=50.
                        Ogre::Real totalWaypointLength = 0.0f;
                        for (size_t pi = 0; pi + 1 < groupWaypoints.size(); ++pi)
                        {
                            totalWaypointLength += pathDistance2D(groupWaypoints[pi].position.x, groupWaypoints[pi].rawHeight, groupWaypoints[pi + 1].position.x, groupWaypoints[pi + 1].rawHeight);
                        }
                        const int totalSamples = std::max(1, static_cast<int>(groupWaypoints.size() - 1) * subdivisions);
                        const Ogre::Real resampleStep = std::max(0.02f, totalWaypointLength / static_cast<Ogre::Real>(totalSamples));
                        groupPath = this->resamplePathUniformly(densePath, resampleStep);
                    }
                    else
                    {
                        groupPath = this->subdivideWithHeightInterpolation(groupWaypoints);
                    }

                    // ── Height smoothing (per group - corners are its pinned endpoints) ──
                    this->smoothHeightTransitions(groupPath);

                    if (true == finalPath.empty())
                    {
                        finalPath = groupPath;
                    }
                    else
                    {
                        // The shared corner point is already the last entry of finalPath.
                        // Record it as a mesh cut only if this boundary was classified as a
                        // genuine reversal, not merely a corner.
                        const size_t joinIndex = finalPath.size() - 1;
                        bool isReversal = false;
                        for (size_t rw : reversalWaypoints)
                        {
                            if (rw == lo)
                            {
                                isReversal = true;
                                break;
                            }
                        }
                        if (true == isReversal)
                        {
                            reversalPathIndices.push_back(joinIndex);
                        }
                        finalPath.insert(finalPath.end(), groupPath.begin() + 1, groupPath.end());
                    }
                }
            }

            if (finalPath.size() < 2)
            {
                continue;
            }

            // ── UV distance ─────────────────────────────────────────────────────
            Ogre::Real accumDist = 0.0f;
            for (size_t pi = 0; pi < finalPath.size(); ++pi)
            {
                if (pi > 0)
                {
                    accumDist += pathDistance2D(finalPath[pi - 1].position.x, finalPath[pi - 1].smoothedHeight, finalPath[pi].position.x, finalPath[pi].smoothedHeight);
                }
                finalPath[pi].distFromStart = accumDist;
            }

            // ── Junction membership of this chain's two ends ─────────────────────
            // Only WHETHER each end sits on a junction (and which one) matters now. The
            // trim-distance machinery that used to live here - a per-arm distance derived
            // from the angular gap to the neighbouring arms, then capped against the chain's
            // own length so the two ends could not trim past each other - is gone with the
            // trimming itself. Arms run all the way into the junction and simply overlap
            // (see generateJunctionPatch for why that is correct here).
            bool hasFrontJunction = false;
            size_t frontJi = 0;

            bool hasBackJunction = false;
            size_t backJi = 0;

            {
                QKey frontKey{quantise(chainWaypoints.front().position.x), quantise(chainWaypoints.front().rawHeight)};
                auto it = junctionByKey.find(frontKey);
                if (it != junctionByKey.end())
                {
                    hasFrontJunction = true;
                    frontJi = it->second;
                }
            }
            {
                QKey backKey{quantise(chainWaypoints.back().position.x), quantise(chainWaypoints.back().rawHeight)};
                auto it = junctionByKey.find(backKey);
                if (it != junctionByKey.end())
                {
                    hasBackJunction = true;
                    backJi = it->second;
                }
            }

            const Ogre::Real junctionOvershoot = std::min(0.05f, this->platformHeight->getReal() * 0.1f);

            int nestLevel = 0;
            if (true == hasFrontJunction)
            {
                nestLevel = std::max(nestLevel, junctionArmCounter[frontJi]++);
            }
            if (true == hasBackJunction)
            {
                nestLevel = std::max(nestLevel, junctionArmCounter[backJi]++);
            }

            // ── Snap front endpoint onto the junction and overshoot slightly ─────
            if (true == hasFrontJunction)
            {
                const size_t ji = frontJi;
                const Ogre::Real jX = junctions[ji].worldPos.x;
                const Ogre::Real jH = junctions[ji].worldPos.y;

                if (finalPath.size() >= 2)
                {
                    // Snap exactly onto the junction centre first. Endpoint detection
                    // quantises to 0.1, so two arms of the same junction can otherwise sit
                    // up to that far apart and leave a hairline crack between their slabs.
                    PlatformControlPoint bp = finalPath.front();
                    bp.position.x = jX;
                    bp.smoothedHeight = jH;
                    bp.rawHeight = jH;
                    bp.distFromStart = 0.0f;
                    finalPath.front() = bp;

                    // Then push the endpoint a little PAST the centre, along this arm's own
                    // outward direction. That direction always points into the interior of the
                    // union of all arms, so the extra sliver is guaranteed to be buried inside
                    // solid geometry. This is what keeps this arm's end cap from landing
                    // coplanar with another arm's end cap on the shared junction plane - the
                    // one pair the depth nesting cannot separate, since nesting offsets Z
                    // while the caps are X planes.
                    Ogre::Vector3 outward(finalPath.front().position.x - finalPath[1].position.x, finalPath.front().smoothedHeight - finalPath[1].smoothedHeight, 0.0f);
                    if (outward.squaredLength() > 1e-8f)
                    {
                        outward.normalise();
                        finalPath.front().position.x += outward.x * junctionOvershoot;
                        finalPath.front().smoothedHeight += outward.y * junctionOvershoot;
                        finalPath.front().rawHeight = finalPath.front().smoothedHeight;
                    }

                    storePatchCorner(ji, finalPath.front().position.x, finalPath.front().smoothedHeight, this->platformDepth->getReal());
                }
            }

            // ── Snap back endpoint onto the junction and overshoot slightly ──────
            if (true == hasBackJunction)
            {
                const size_t ji = backJi;
                const Ogre::Real jX = junctions[ji].worldPos.x;
                const Ogre::Real jH = junctions[ji].worldPos.y;

                if (finalPath.size() >= 2)
                {
                    PlatformControlPoint bp = finalPath.back();
                    bp.position.x = jX;
                    bp.smoothedHeight = jH;
                    bp.rawHeight = jH;
                    finalPath.back() = bp;

                    const size_t lastIdx = finalPath.size() - 1;
                    Ogre::Vector3 outward(finalPath[lastIdx].position.x - finalPath[lastIdx - 1].position.x, finalPath[lastIdx].smoothedHeight - finalPath[lastIdx - 1].smoothedHeight, 0.0f);
                    if (outward.squaredLength() > 1e-8f)
                    {
                        outward.normalise();
                        finalPath[lastIdx].position.x += outward.x * junctionOvershoot;
                        finalPath[lastIdx].smoothedHeight += outward.y * junctionOvershoot;
                        finalPath[lastIdx].rawHeight = finalPath[lastIdx].smoothedHeight;
                    }

                    // Recorded for the [JUNCTION-DEBUG] arm list only - no junction geometry
                    // is built from these corners any more.
                    storePatchCorner(ji, finalPath.back().position.x, finalPath.back().smoothedHeight, this->platformDepth->getReal());
                }
            }

            // Re-accumulate UV after trimming
            accumDist = 0.0f;
            for (size_t pi = 0; pi < finalPath.size(); ++pi)
            {
                if (pi > 0)
                {
                    accumDist += pathDistance2D(finalPath[pi - 1].position.x, finalPath[pi - 1].smoothedHeight, finalPath[pi].position.x, finalPath[pi].smoothedHeight);
                }
                finalPath[pi].distFromStart = accumDist;
            }

            if (finalPath.size() < 2)
            {
                continue;
            }

            // ── Transform to local (mesh) space ─────────────────────────────────
            std::vector<PlatformControlPoint> localPath;
            localPath.reserve(finalPath.size());
            for (const auto& cp : finalPath)
            {
                PlatformControlPoint lp;
                lp.position = Ogre::Vector3(cp.position.x - originToUse.x, 0.0f, 0.0f);
                lp.rawHeight = cp.rawHeight - originToUse.y;
                lp.smoothedHeight = cp.smoothedHeight - originToUse.y;
                lp.distFromStart = cp.distFromStart;
                localPath.push_back(lp);
            }

            // ── Generate platform geometry for this chain ───────────────────────
            // The chain is meshed as ONE piece unless it contains a genuine hairpin
            // reversal. reversalPathIndices was recorded while the path was built (see the
            // turn-angle classification above), so the cut points are exact indices into
            // this path - no searching needed.
            //
            // REPLACES: the old "cut into runs monotonic in local X" machinery, plus its two
            // helper passes (nearest-sample anchor per authored corner, then refinement to
            // the true local X extremum within a window). All of that existed to compensate
            // for a straight-down extrusion, which folds through itself the moment X-progress
            // reverses regardless of how gentle the actual turn is. The mitered extrusion in
            // generatePlatformBox has no such constraint: it follows the path normal, so an
            // ordinary bend of any steepness - including a vertical wall, where X does not
            // progress at all - sweeps cleanly as one continuous run. Only a near-180 degree
            // fold is genuinely unresolvable by a miter, and that is precisely what the
            // reversal threshold now selects.
            //
            // Every cut point is duplicated into both neighbouring runs and gets a real end
            // cap on each side, so the two runs meet as a closed corner joint instead of an
            // open fold.
            std::vector<std::vector<PlatformControlPoint>> meshRuns;
            {
                size_t runStart = 0;
                for (size_t si : reversalPathIndices)
                {
                    if (si > runStart && si + 1 < localPath.size())
                    {
                        meshRuns.emplace_back(localPath.begin() + runStart, localPath.begin() + si + 1);
                        runStart = si;
                    }
                }
                meshRuns.emplace_back(localPath.begin() + runStart, localPath.end());
            }

            for (size_t ri = 0; ri < meshRuns.size(); ++ri)
            {
                if (meshRuns[ri].size() < 2)
                {
                    continue;
                }

                PlatformSegment localSegment;
                localSegment.controlPoints = meshRuns[ri];
                localSegment.isCurved = false;
                localSegment.curvature = 0.0f;
                // BUGFIX: both neighbours of an internal split joint used to get a real cap
                // (skipFrontCap/skipBackCap = false on both sides). The cap is always built as
                // a flat slice at constant local X (see addEndCap's toVec3 above - only the
                // shading normal is tilted, not the plane itself), and a split joint's two
                // runs share that exact X - so the two caps were perfectly coincident,
                // z-fighting against each other every frame (the flicker in the two
                // screenshots). Only the run ENDING at a joint draws the cap now; the run
                // STARTING there skips its front cap, since the previous run's back cap
                // already closes that seam.
                //
                // ARCHITECTURE CHANGE: a junction end now gets a REAL cap. Previously it was
                // skipped because the junction patch closed that opening; with no patch left,
                // skipping would leave the slab's hollow interior open at the junction. The
                // cap ends up buried inside the other arms' solid volume thanks to the
                // overshoot applied above, so it is never actually visible.
                localSegment.skipBackCap = false;
                localSegment.skipFrontCap = true;
                if (0 == ri)
                {
                    localSegment.skipFrontCap = false;
                }

                // BUGFIX 4: even with the refined split point (BUGFIX 3 above), a residual
                // sliver of overlap can still show up right at a reversal joint - floating
                // point means "the true turning sample" and "where the two runs visually
                // meet" are only ever equal to within some epsilon, not exactly. Nudging every
                // run after the first slightly narrower in depth means its whole
                // cross-section - including its end cap - sits strictly NESTED inside the
                // previous run's footprint at the shared corner instead of flush with it, so
                // there's no shared plane left to z-fight over even if the split location is
                // imperfect. 1cm per run is small enough not to read as a visible step on the
                // sides, but enough to be well clear of float noise. g_platformRunDepthOverride
                // is a file-local (not a class member - see its declaration up top), reset to
                // the "no override" sentinel right after this one call so nothing else
                // (including the very next run, or a junction patch) is affected.
                //
                // ARCHITECTURE CHANGE: nestLevel (computed per junction further up) is added
                // to the run index here, so the exact same nesting now also separates the
                // arms of a junction from each other. Since arms are no longer trimmed back,
                // two arms meeting at a junction genuinely overlap in XY, and their front and
                // back faces would otherwise be coplanar at z = +/- depth/2 and z-fight. 1 cm
                // per level is far too small to read as a step on the sides, but well clear
                // of float noise.
                g_platformRunDepthOverride = std::max(0.05f, this->platformDepth->getReal() - 0.01f * (static_cast<Ogre::Real>(nestLevel) + static_cast<Ogre::Real>(ri)));
                this->generatePlatformSegment(localSegment);
                g_platformRunDepthOverride = -1.0f;
            }
        }

        // ── Junction patches ────────────────────────────────────────────────────
        // ARCHITECTURE CHANGE: this no longer builds anything - generateJunctionPatch is
        // logging only now. The arms above are untrimmed and overlap each other at every
        // junction, and their union is already the correct solid. The call is kept solely so
        // the [JUNCTION-DEBUG] arm list still gets emitted once per rebuild.
        // The ARM/JUNCTION-WEDGE labelling the diagnostics used to do is gone with it: every
        // triangle now comes from regular per-chain box generation, so there is nothing left
        // to distinguish.
        for (const JunctionPoint& jp : junctions)
        {
            this->generateJunctionPatch(jp, originToUse);
        }

        if (this->currentSurfaceVertexIndex > 0 || this->currentGroundVertexIndex > 0 || this->currentJunctionVertexIndex > 0)
        {
            this->createPlatformMesh();
        }
    }

    Ogre::Vector2 ProceduralPlatformComponent::evaluateCatmullRom(const std::vector<PlatformControlPoint>& points, Ogre::Real t)
    {
        int segment = static_cast<int>(t);
        Ogre::Real localT = t - segment;

        int p0 = Ogre::Math::Clamp(segment - 1, 0, static_cast<int>(points.size()) - 1);
        int p1 = Ogre::Math::Clamp(segment, 0, static_cast<int>(points.size()) - 1);
        int p2 = Ogre::Math::Clamp(segment + 1, 0, static_cast<int>(points.size()) - 1);
        int p3 = Ogre::Math::Clamp(segment + 2, 0, static_cast<int>(points.size()) - 1);

        // Unlike ProceduralRoadComponent (which interpolates points[i].position directly,
        // since both its active axes - X,Z - live there), this pulls x from position and
        // height from rawHeight (the pre-smoothing value, matching which field
        // ProceduralRoadComponent's own position.z would have held at this same pipeline
        // stage) into one combined (x, height) pair per control point, so a single
        // Catmull-Rom evaluation produces both axes together.
        Ogre::Vector2 v0(points[p0].position.x, points[p0].rawHeight);
        Ogre::Vector2 v1(points[p1].position.x, points[p1].rawHeight);
        Ogre::Vector2 v2(points[p2].position.x, points[p2].rawHeight);
        Ogre::Vector2 v3(points[p3].position.x, points[p3].rawHeight);

        // BUGFIX: this used to be a plain uniform Catmull-Rom (fixed cubic Hermite blend,
        // tangents estimated from the chord between neighbours regardless of how sharp the
        // turn there is). At a near-reversal waypoint - left-to-right then immediately
        // right-to-left, the user's reported case - that tangent estimate overshoots
        // wildly and the curve loops out away from the corner instead of tracking it: the
        // mouth of that loop is the hole in the mesh, and the far side of the loop is the
        // overlapping triangles. This is the textbook failure mode of UNIFORM
        // parametrization. Centripetal parametrization (Barry-Goldman construction,
        // alpha=0.5, knot spacing ~ sqrt(chord length)) is the standard fix and is proven
        // to never loop or self-intersect within a segment for any control point
        // configuration, including hairpins - so no separate sharp-turn special case is
        // needed, this replaces the formula unconditionally. Same external contract as
        // before (t = segment index + local fraction), so both call sites (the dense-path
        // loop in rebuildMesh and generateCurvePoints) need no changes.
        auto knotDelta = [](const Ogre::Vector2& a, const Ogre::Vector2& b) -> Ogre::Real
        {
            // alpha = 0.5; floored so two coincident control points (start/end boundary
            // duplication via the Clamp() calls above, or an accidental zero-length
            // waypoint gap) never divide by zero below.
            return std::max(1e-4f, std::sqrt(a.distance(b)));
        };

        const Ogre::Real k0 = 0.0f;
        const Ogre::Real k1 = k0 + knotDelta(v0, v1);
        const Ogre::Real k2 = k1 + knotDelta(v1, v2);
        const Ogre::Real k3 = k2 + knotDelta(v2, v3);

        const Ogre::Real kt = k1 + Ogre::Math::Clamp(localT, 0.0f, 1.0f) * (k2 - k1);

        const Ogre::Vector2 A1 = v0 * ((k1 - kt) / (k1 - k0)) + v1 * ((kt - k0) / (k1 - k0));
        const Ogre::Vector2 A2 = v1 * ((k2 - kt) / (k2 - k1)) + v2 * ((kt - k1) / (k2 - k1));
        const Ogre::Vector2 A3 = v2 * ((k3 - kt) / (k3 - k2)) + v3 * ((kt - k2) / (k3 - k2));

        const Ogre::Vector2 B1 = A1 * ((k2 - kt) / (k2 - k0)) + A2 * ((kt - k0) / (k2 - k0));
        const Ogre::Vector2 B2 = A2 * ((k3 - kt) / (k3 - k1)) + A3 * ((kt - k1) / (k3 - k1));

        return B1 * ((k2 - kt) / (k2 - k1)) + B2 * ((kt - k1) / (k2 - k1));
    }

    std::vector<Ogre::Vector2> ProceduralPlatformComponent::generateCurvePoints(const std::vector<PlatformControlPoint>& controlPoints, int subdivisions)
    {
        std::vector<Ogre::Vector2> result;

        if (controlPoints.size() < 2)
        {
            return result;
        }

        if (controlPoints.size() == 2)
        {
            for (int i = 0; i <= subdivisions; ++i)
            {
                Ogre::Real t = static_cast<Ogre::Real>(i) / subdivisions;
                result.push_back(Ogre::Vector2(controlPoints[0].position.x, controlPoints[0].rawHeight) * (1.0f - t) + Ogre::Vector2(controlPoints[1].position.x, controlPoints[1].rawHeight) * t);
            }
            return result;
        }

        for (size_t i = 0; i < controlPoints.size() - 1; ++i)
        {
            for (int j = 0; j < subdivisions; ++j)
            {
                Ogre::Real t = static_cast<Ogre::Real>(j) / subdivisions;
                result.push_back(this->evaluateCatmullRom(controlPoints, i + t));
            }
        }

        result.push_back(Ogre::Vector2(controlPoints.back().position.x, controlPoints.back().rawHeight));
        return result;
    }

    std::vector<ProceduralPlatformComponent::PlatformControlPoint> ProceduralPlatformComponent::subdivideWithHeightInterpolation(const std::vector<PlatformControlPoint>& points)
    {
        if (points.size() < 2)
        {
            return points;
        }

        std::vector<PlatformControlPoint> result;
        const Ogre::Real interval = 1.0f;

        for (size_t i = 0; i < points.size() - 1; ++i)
        {
            const PlatformControlPoint& p0 = points[i];
            const PlatformControlPoint& p1 = points[i + 1];

            const Ogre::Real segLength = pathDistance2D(p0.position.x, p0.rawHeight, p1.position.x, p1.rawHeight);
            int numSamples = std::max(2, static_cast<int>(segLength / interval) + 1);

            for (int j = 0; j < numSamples; ++j)
            {
                // BUGFIX: this used to be gated by "i < points.size() - 2", i.e. it only
                // skipped the duplicate on segments that are NOT the last one. For a chain
                // with exactly 2 raw points (the single-segment case - the only case that
                // ever reaches this function, see the size()<3 branch in rebuildMesh), the
                // loop only ever runs i=0, and "0 < points.size() - 2" is "0 < 0" = false,
                // so the last sample (t=1, identical to p1) was NOT skipped here - and then
                // points.back() (the same p1) was pushed again right below, producing two
                // near-duplicate points at the very end. That degenerate near-zero-length
                // tail segment survived height smoothing with a tiny float offset and showed
                // up in generatePlatformBox as a thin extra wedge sticking out past the real
                // end face. Skipping the last sample unconditionally on every segment is
                // correct in general: the trailing points.back() push below always supplies
                // the true final point exactly once, and for multi-segment chains the next
                // segment's own j=0 sample already supplies the shared boundary point.
                if (j == numSamples - 1)
                {
                    continue; // Skip duplicate endpoint - always, not just for non-last segments
                }

                Ogre::Real t = static_cast<Ogre::Real>(j) / (numSamples - 1);

                PlatformControlPoint cp;
                cp.position = Ogre::Vector3(p0.position.x * (1.0f - t) + p1.position.x * t, 0.0f, 0.0f);
                // A straight line between exactly two known points has no independent
                // "terrain" to resample - unlike ProceduralRoadComponent's version, plain
                // linear interpolation IS the correct height here (and naturally reproduces
                // the exact endpoint heights at t=0/t=1, so no special-casing is needed).
                cp.rawHeight = Ogre::Math::lerp(p0.smoothedHeight, p1.smoothedHeight, t);
                cp.smoothedHeight = cp.rawHeight;
                cp.distFromStart = 0.0f;
                result.push_back(cp);
            }
        }

        result.push_back(points.back());
        return result;
    }

    std::vector<ProceduralPlatformComponent::PlatformControlPoint> ProceduralPlatformComponent::resamplePathUniformly(const std::vector<PlatformControlPoint>& densePath, Ogre::Real stepMeters)
    {
        if (densePath.size() < 3)
        {
            return densePath;
        }

        std::vector<Ogre::Real> cumDist(densePath.size(), 0.0f);
        for (size_t i = 1; i < densePath.size(); ++i)
        {
            cumDist[i] = cumDist[i - 1] + pathDistance2D(densePath[i - 1].position.x, densePath[i - 1].rawHeight, densePath[i].position.x, densePath[i].rawHeight);
        }
        const Ogre::Real totalLength = cumDist.back();

        const Ogre::Real step = std::max(0.1f, stepMeters);
        const int numSteps = std::max(1, static_cast<int>(std::round(totalLength / step)));

        std::vector<PlatformControlPoint> resampled;
        resampled.reserve(static_cast<size_t>(numSteps) + 1);

        size_t searchStart = 0;

        for (int s = 0; s <= numSteps; ++s)
        {
            Ogre::Real targetDist = (s == 0) ? 0.0f : (s == numSteps) ? totalLength : (static_cast<Ogre::Real>(s) / static_cast<Ogre::Real>(numSteps)) * totalLength;

            while (searchStart + 1 < cumDist.size() && cumDist[searchStart + 1] < targetDist)
            {
                ++searchStart;
            }

            const size_t i0 = searchStart;
            const size_t i1 = std::min(i0 + 1, densePath.size() - 1);
            const Ogre::Real segLen = cumDist[i1] - cumDist[i0];
            const Ogre::Real t = (segLen > 1e-6f) ? Ogre::Math::Clamp((targetDist - cumDist[i0]) / segLen, 0.0f, 1.0f) : 0.0f;

            PlatformControlPoint cp;
            cp.position = Ogre::Vector3(densePath[i0].position.x * (1.0f - t) + densePath[i1].position.x * t, 0.0f, 0.0f);
            cp.rawHeight = densePath[i0].rawHeight * (1.0f - t) + densePath[i1].rawHeight * t;
            cp.smoothedHeight = densePath[i0].smoothedHeight * (1.0f - t) + densePath[i1].smoothedHeight * t;
            cp.distFromStart = 0.0f; // recomputed by the caller afterward, as before
            resampled.push_back(cp);
        }

        return resampled;
    }

    ///////////////////////////////////////////////////////////////////////////
    // smoothHeightTransitions - 5-tap Gaussian passes over the interior points, with the
    // first and last point pinned. Called once per smooth GROUP by rebuildMesh, never over
    // a whole chain, so its two pinned points are always either a chain end or an authored
    // corner.
    ///////////////////////////////////////////////////////////////////////////

    void ProceduralPlatformComponent::smoothHeightTransitions(std::vector<PlatformControlPoint>& points)
    {
        if (points.size() < 3)
        {
            return;
        }

        // ── REMOVED: the whole Max Gradient machinery ────────────────────────────────────
        // This function was a direct port of ProceduralRoadComponent's version and carried
        // that component's core assumption with it: that height is a FUNCTION of horizontal
        // distance, so a slope can be measured as heightDiff / horizontalDist and clamped.
        // A road can never be vertical; a platform can, and must - that is what a
        // Metroidvania wall is. For a vertical stretch horizontalDist goes to zero, so the
        // clamp demanded a height change of ~0 and flattened exactly the geometry the user
        // had deliberately drawn. Two symptoms came from it, both reported: a vertical leg
        // followed by a horizontal one collapsed into a single diagonal beam on confirm
        // (the "bridge mode" chord fallback re-lerped every interior height along CUMULATIVE
        // HORIZONTAL distance, and the corner's was 0), and every following drag preview
        // appeared offset from the chain end, because the bidirectional clamp loops moved
        // points.front() and points.back() as well as interior points.
        //
        // An intermediate fix kept the clamp but floored the allowed height difference at
        // whatever the path already had on entry, so it could never make a pair shallower
        // than the user drew it. That made the clamp a no-op in practice, and this is why it
        // is now gone entirely rather than merely defanged: the smoothing below is a
        // weighted average of neighbouring heights, and an average always lands inside the
        // convex hull of its inputs - it cannot produce a slope steeper than the steepest
        // one already present. There was never anything left for a gradient clamp to catch.
        //
        // The Max Gradient attribute is removed along with it. It no longer had a meaning
        // that could be stated honestly in a tooltip, and a slider reading 45 while the user
        // builds a 90 degree wall is worse than no slider.
        const Ogre::Real smoothing = this->smoothingFactor->getReal();

        const Ogre::Real startHeight = points.front().smoothedHeight;
        const Ogre::Real endHeight = points.back().smoothedHeight;

        const int smoothPasses = static_cast<int>(smoothing * 8.0f) + 1;

        for (int pass = 0; pass < smoothPasses; ++pass)
        {
            std::vector<Ogre::Real> newHeights(points.size());

            for (size_t i = 0; i < points.size(); ++i)
            {
                if (i == 0 || i == points.size() - 1)
                {
                    newHeights[i] = points[i].rawHeight;
                    continue;
                }

                const Ogre::Real weights[5] = {0.1f, 0.2f, 0.4f, 0.2f, 0.1f};
                Ogre::Real weightSum = 0.0f;
                Ogre::Real heightSum = 0.0f;

                for (int k = -2; k <= 2; ++k)
                {
                    int idx = Ogre::Math::Clamp(static_cast<int>(i) + k, 0, static_cast<int>(points.size()) - 1);
                    Ogre::Real w = weights[k + 2];
                    heightSum += points[idx].smoothedHeight * w;
                    weightSum += w;
                }

                newHeights[i] = Ogre::Math::lerp(points[i].rawHeight, heightSum / weightSum, smoothing);
            }

            for (size_t i = 1; i < points.size() - 1; ++i)
            {
                points[i].smoothedHeight = newHeights[i];
            }
        }

        // The first and last point are pinned. rebuildMesh calls this once per smooth GROUP
        // (a stretch of the chain with no authored corner inside it), so these two are always
        // either a chain end or an authored corner - both of which have to stay exactly where
        // the user put them. That is what keeps a drag preview starting flush with the built
        // chain instead of offset from it, and what keeps a 90 degree corner from being
        // rounded away.
        points.front().smoothedHeight = startHeight;
        points.back().smoothedHeight = endHeight;
    }

    Ogre::Vector3 ProceduralPlatformComponent::snapToGridFunc(const Ogre::Vector3& position)
    {
        if (false == this->snapToGrid->getBool())
        {
            return position;
        }

        Ogre::Real gridSz = this->gridSize->getReal();

        // Unlike ProceduralRoadComponent (which never snaps Y, since it comes from terrain,
        // not from the user), platform snaps BOTH axes - horizontal AND height - since both
        // are directly placed by the user and there is no terrain to be inconsistent with.
        return Ogre::Vector3(Ogre::Math::Floor(position.x / gridSz + 0.5f) * gridSz, Ogre::Math::Floor(position.y / gridSz + 0.5f) * gridSz, position.z);
    }

    ProceduralPlatformComponent::PlatformStyle ProceduralPlatformComponent::getPlatformStyleEnum(void) const
    {
        Ogre::String styleStr = this->platformStyle->getListSelectedValue();

        if (styleStr == "Grass")
        {
            return PlatformStyle::GRASS;
        }
        else if (styleStr == "Wood")
        {
            return PlatformStyle::WOOD;
        }
        else if (styleStr == "Stone")
        {
            return PlatformStyle::STONE;
        }
        else if (styleStr == "Ice")
        {
            return PlatformStyle::ICE;
        }
        else if (styleStr == "Metal")
        {
            return PlatformStyle::METAL;
        }

        return PlatformStyle::GRASS;
    }

    void ProceduralPlatformComponent::generatePlatformSegment(const PlatformSegment& segment)
    {
        if (segment.controlPoints.size() < 2)
        {
            return;
        }

        if (segment.isCurved)
        {
            this->generateCurvedPlatform(segment.controlPoints, segment.curvature, segment.skipFrontCap, segment.skipBackCap);
        }
        else
        {
            this->generateStraightPlatform(segment.controlPoints, segment.skipFrontCap, segment.skipBackCap);
        }
    }

    void ProceduralPlatformComponent::generateStraightPlatform(const std::vector<PlatformControlPoint>& points, bool skipFrontCap, bool skipBackCap)
    {
        if (points.size() < 2)
        {
            return;
        }

        // NOTE: unlike ProceduralRoadComponent's generateStraightRoad, no computeMiterData
        // call - see the class doc comment: depth is orthogonal to the whole path plane and
        // uniform everywhere, so there is no per-point miter/offset direction to precompute.
        // Each style generator works directly off the (x, height) points plus the
        // platformDepth/platformHeight Variants.
        switch (this->getPlatformStyleEnum())
        {
        case PlatformStyle::GRASS:
            this->generateGrassPlatform(points, skipFrontCap, skipBackCap);
            break;
        case PlatformStyle::WOOD:
            this->generateWoodPlatform(points, skipFrontCap, skipBackCap);
            break;
        case PlatformStyle::STONE:
            this->generateStonePlatform(points, skipFrontCap, skipBackCap);
            break;
        case PlatformStyle::ICE:
            this->generateIcePlatform(points, skipFrontCap, skipBackCap);
            break;
        case PlatformStyle::METAL:
            this->generateMetalPlatform(points, skipFrontCap, skipBackCap);
            break;
        default:
            this->generateGrassPlatform(points, skipFrontCap, skipBackCap);
            break;
        }
    }

    void ProceduralPlatformComponent::generateCurvedPlatform(const std::vector<PlatformControlPoint>& points, Ogre::Real curvature, bool skipFrontCap, bool skipBackCap)
    {
        // Secondary path for an explicitly-curved single segment (e.g. a scripted/Lua call
        // setting isCurved+curvature directly) - the interactive multi-point chain path in
        // rebuildMesh always produces its own Catmull-Rom fit regardless of this flag, so
        // this is rarely exercised in normal editor use. Kept for parity with
        // ProceduralRoadComponent::generateCurvedRoad.
        if (points.size() < 2)
        {
            return;
        }

        const int subdivisions = this->curveSubdivisions->getInt();
        std::vector<Ogre::Vector2> curvePoints = this->generateCurvePoints(points, subdivisions);

        if (curvePoints.size() < 2)
        {
            return;
        }

        std::vector<PlatformControlPoint> expanded;
        expanded.reserve(curvePoints.size());
        for (const auto& xh : curvePoints)
        {
            PlatformControlPoint cp;
            cp.position = Ogre::Vector3(xh.x, 0.0f, 0.0f);
            cp.rawHeight = xh.y;
            cp.smoothedHeight = xh.y;
            expanded.push_back(cp);
        }

        Ogre::Real accumDist = 0.0f;
        for (size_t pi = 0; pi < expanded.size(); ++pi)
        {
            if (pi > 0)
            {
                accumDist += pathDistance2D(expanded[pi - 1].position.x, expanded[pi - 1].smoothedHeight, expanded[pi].position.x, expanded[pi].smoothedHeight);
            }
            expanded[pi].distFromStart = accumDist;
        }

        this->generateStraightPlatform(expanded, skipFrontCap, skipBackCap);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Style Generators
    ///////////////////////////////////////////////////////////////////////////////////////////////

    void ProceduralPlatformComponent::addPlatformQuad(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& v3, const Ogre::Vector3& normal, Ogre::Real u0, Ogre::Real u1, Ogre::Real v0Val, Ogre::Real v1Val,
        PlatformMeshBuffer targetBuffer)
    {
        std::vector<float>& verts = (targetBuffer == PlatformMeshBuffer::SURFACE) ? this->surfaceVertices : (targetBuffer == PlatformMeshBuffer::GROUND) ? this->groundVertices : this->junctionVertices;
        std::vector<Ogre::uint32>& inds = (targetBuffer == PlatformMeshBuffer::SURFACE) ? this->surfaceIndices : (targetBuffer == PlatformMeshBuffer::GROUND) ? this->groundIndices : this->junctionIndices;
        Ogre::uint32& currentIdx = (targetBuffer == PlatformMeshBuffer::SURFACE) ? this->currentSurfaceVertexIndex : (targetBuffer == PlatformMeshBuffer::GROUND) ? this->currentGroundVertexIndex : this->currentJunctionVertexIndex;

        Ogre::Vector3 edge1 = v1 - v0;
        Ogre::Vector3 edge2 = v2 - v0;
        Ogre::Vector3 triNormal = edge1.crossProduct(edge2);

        // BUGFIX: the old check compared the cross product's squared length against a fixed
        // absolute 0.0001f. That works for "normal sized" quads, but at fine curve
        // subdivision (short segments => short edge1) the cross product shrinks right along
        // with the edge length, and can drop below a FIXED threshold even for a perfectly
        // valid, non-degenerate quad. When that happens this function silently stops
        // correcting the winding at all (falls into the "else" branch below) - exactly at
        // the densely-subdivided spots, i.e. exactly on curves. Compare against a threshold
        // that scales with the edge lengths instead, so "is this quad degenerate" and "are
        // these edges short" stay two separate questions.
        const Ogre::Real edgeScale = std::max(edge1.squaredLength(), edge2.squaredLength());
        const Ogre::Real degenerateThreshold = std::max(1e-10f, edgeScale * 1e-6f);

        bool flipWinding = false;
        if (triNormal.squaredLength() > degenerateThreshold)
        {
            triNormal.normalise();
            if (triNormal.dotProduct(normal) < 0.0f)
            {
                flipWinding = true;
                triNormal = -triNormal;
            }
        }
        else
        {
            triNormal = normal;
        }

        auto addVertex = [&](const Ogre::Vector3& pos, Ogre::Real u, Ogre::Real v)
        {
            verts.push_back(pos.x);
            verts.push_back(pos.y);
            verts.push_back(pos.z);
            verts.push_back(triNormal.x);
            verts.push_back(triNormal.y);
            verts.push_back(triNormal.z);
            verts.push_back(u);
            verts.push_back(v);
        };

        Ogre::uint32 baseIdx = currentIdx;
        addVertex(v0, u0, v0Val);
        addVertex(v1, u1, v0Val);
        addVertex(v2, u1, v1Val);
        addVertex(v3, u0, v1Val);

        if (false == flipWinding)
        {
            inds.push_back(baseIdx + 0);
            inds.push_back(baseIdx + 1);
            inds.push_back(baseIdx + 2);

            inds.push_back(baseIdx + 0);
            inds.push_back(baseIdx + 2);
            inds.push_back(baseIdx + 3);
        }
        else
        {
            inds.push_back(baseIdx + 0);
            inds.push_back(baseIdx + 2);
            inds.push_back(baseIdx + 1);

            inds.push_back(baseIdx + 0);
            inds.push_back(baseIdx + 3);
            inds.push_back(baseIdx + 2);
        }

        currentIdx += 4;
    }

    void ProceduralPlatformComponent::generatePlatformBox(const std::vector<PlatformControlPoint>& points, Ogre::Real topBevel, Ogre::Real rimHeight, bool skipFrontCap, bool skipBackCap)
    {
        if (points.size() < 2)
        {
            return;
        }

        // BUGFIX: reads g_platformRunDepthOverride first - see its declaration up in the
        // anonymous namespace near pathDistance2D for why. -1 (the sentinel) means "no
        // override", i.e. every normal call behaves exactly as before.
        const Ogre::Real depth = (g_platformRunDepthOverride > 0.0f) ? g_platformRunDepthOverride : this->platformDepth->getReal();
        const Ogre::Real height = this->platformHeight->getReal();
        const Ogre::Real zFront = -depth * 0.5f;
        const Ogre::Real zBack = depth * 0.5f;

        const Ogre::Vector2 surfUV = this->surfaceUVTiling->getVector2();
        const Ogre::Vector2 groundUV = this->groundUVTiling->getVector2();

        const Ogre::Real bevel = Ogre::Math::Clamp(topBevel, 0.0f, std::min(depth, height) * 0.45f);
        const Ogre::Real rim = Ogre::Math::Clamp(rimHeight, 0.0f, height * 0.3f);
        const Ogre::Real rimBand = std::min(depth * 0.15f, 0.3f);

        const Ogre::Real topZFront = zFront + bevel;
        const Ogre::Real topZBack = zBack - bevel;

        // ── BUGFIX: mitered (perpendicular-to-path) extrusion instead of straight down ────
        // The slab used to be extruded straight down (-Y) by platformHeight: the bottom of a
        // point at (x, h) was simply (x, h - height). That is exact for a horizontal platform,
        // but the PERPENDICULAR thickness it produces is height * |upNormal.y| - so it shrinks
        // as the path steepens and collapses to ZERO for a vertical segment. Top and bottom
        // face then land on the same plane x = const and the whole segment renders as a
        // paper-thin blade with no thickness at all (the "completely flat" vertical pull the
        // user reported, and the same in the drag preview, which goes through this function
        // via generateStraightPlatform).
        //
        // The slab is now offset along the path NORMAL instead, so its thickness is
        // platformHeight measured perpendicular to the path at every angle - a vertical pull
        // becomes a proper wall of thickness platformHeight in X. For a horizontal platform
        // the normal IS -Y, so that case is bit-for-bit what it was before.
        //
        // NOTE on the older REVERTED note that used to sit here: a mitered extrusion was tried
        // once before and reverted, because generateJunctionPatch's bottom fan still assumed
        // straight-down extrusion and an arm's mitered bottom edge no longer lined up with the
        // junction's own bottom geometry. That reason is gone - junctions build no geometry at
        // all any more (see generateJunctionPatch), arms simply interpenetrate, so there is
        // nothing left for the bottom edge to line up WITH.
        //
        // Two passes: per-segment normals first (with the existing orientation-continuity
        // rules), then per-VERTEX normals as the average of the two adjacent segment normals.
        // Using a shared per-vertex normal is what keeps the bottom edges of two neighbouring
        // segments meeting exactly at their common point instead of tearing open at every
        // bend. The 1/cos(halfAngle) miter extension keeps the thickness uniform through a
        // bend; it is clamped so a sharp corner cannot blow the bottom edge out to infinity
        // (genuine reversals never reach here anyway - rebuildMesh already splits the chain
        // into monotonic runs before calling this).
        const size_t numPoints = points.size();
        const size_t numSegments = numPoints - 1;

        std::vector<Ogre::Vector2> segmentNormals(numSegments, Ogre::Vector2(0.0f, 1.0f));
        {
            Ogre::Vector2 previousUpNormal(0.0f, 0.0f);
            for (size_t i = 0; i < numSegments; ++i)
            {
                const Ogre::Real dx = points[i + 1].position.x - points[i].position.x;
                const Ogre::Real dh = points[i + 1].smoothedHeight - points[i].smoothedHeight;

                Ogre::Vector2 upNormal(-dh, dx); // raw 90-degree rotation of the tangent
                if (upNormal.squaredLength() > 1e-8f)
                {
                    upNormal.normalise();

                    const Ogre::Real confidence = std::abs(upNormal.y); // how meaningful "prefer true up" is here
                    if (confidence > 0.3f)
                    {
                        if (upNormal.y < 0.0f)
                        {
                            upNormal = -upNormal;
                        }
                    }
                    else if (previousUpNormal.squaredLength() > 1e-8f)
                    {
                        // Near-vertical segment: true up is an unreliable tie-breaker here, so
                        // stay smooth with whatever the path was already doing instead.
                        if (upNormal.dotProduct(previousUpNormal) < 0.0f)
                        {
                            upNormal = -upNormal;
                        }
                    }
                    else if (upNormal.x < 0.0f)
                    {
                        // First segment in the chain, also near-vertical: no history yet either.
                        // True up is useless as a tie-breaker at this angle, so pick a stable
                        // starting side by X instead - a wall pulled straight up then always
                        // gets its thickness on the same side rather than flipping between
                        // rebuilds depending on float noise in a near-zero .y.
                        upNormal = -upNormal;
                    }
                }
                else
                {
                    // Degenerate zero-length segment.
                    upNormal = Ogre::Vector2(0.0f, 1.0f);
                    if (previousUpNormal.squaredLength() > 1e-8f)
                    {
                        upNormal = previousUpNormal;
                    }
                }

                segmentNormals[i] = upNormal;
                previousUpNormal = upNormal;
            }
        }

        // Per-vertex geometry: the top point as authored, plus the down direction, the mitered
        // bottom point, the chamfer point and the raised-rim point derived from it. Everything
        // in the sweep below is expressed through these, so no piece of the cross-section can
        // disagree with another about where "down" is.
        std::vector<Ogre::Vector2> topPoints(numPoints);
        std::vector<Ogre::Vector2> downDirs(numPoints);
        std::vector<Ogre::Vector2> bottomPoints(numPoints);
        std::vector<Ogre::Vector2> bevelPoints(numPoints);
        std::vector<Ogre::Vector2> rimPoints(numPoints);
        for (size_t i = 0; i < numPoints; ++i)
        {
            Ogre::Vector2 vertexNormal;
            if (0 == i)
            {
                vertexNormal = segmentNormals.front();
            }
            else if (i + 1 == numPoints)
            {
                vertexNormal = segmentNormals.back();
            }
            else
            {
                vertexNormal = segmentNormals[i - 1] + segmentNormals[i];
                if (vertexNormal.squaredLength() > 1e-8f)
                {
                    vertexNormal.normalise();
                }
                else
                {
                    vertexNormal = segmentNormals[i];
                }
            }

            size_t adjacentSegment = i;
            if (i + 1 == numPoints)
            {
                adjacentSegment = numSegments - 1;
            }
            const Ogre::Real cosHalfAngle = std::max(0.35f, vertexNormal.dotProduct(segmentNormals[adjacentSegment]));
            const Ogre::Real thickness = height / cosHalfAngle;

            topPoints[i] = Ogre::Vector2(points[i].position.x, points[i].smoothedHeight);
            downDirs[i] = -vertexNormal;
            bottomPoints[i] = topPoints[i] + downDirs[i] * thickness;
            bevelPoints[i] = topPoints[i] + downDirs[i] * bevel;
            rimPoints[i] = topPoints[i] - downDirs[i] * rim;
        }

        auto toWorld = [](const Ogre::Vector2& xy, Ogre::Real z)
        {
            return Ogre::Vector3(xy.x, xy.y, z);
        };

        for (size_t i = 0; i + 1 < numPoints; ++i)
        {
            const Ogre::Real u0s = points[i].distFromStart * surfUV.x;
            const Ogre::Real u1s = points[i + 1].distFromStart * surfUV.x;
            const Ogre::Real u0g = points[i].distFromStart * groundUV.x;
            const Ogre::Real u1g = points[i + 1].distFromStart * groundUV.x;

            const Ogre::Vector3 upNormal(segmentNormals[i].x, segmentNormals[i].y, 0.0f);

            if (rim > 0.0f && depth > rimBand * 2.5f)
            {
                // ── 3-band top: raised rim strip along each Z edge, flat middle (Metal) ──
                const Ogre::Real midZFront = topZFront + rimBand;
                const Ogre::Real midZBack = topZBack - rimBand;

                this->addPlatformQuad(toWorld(topPoints[i], midZFront), toWorld(topPoints[i + 1], midZFront), toWorld(topPoints[i + 1], midZBack), toWorld(topPoints[i], midZBack), upNormal, u0s, u1s, 0.0f, surfUV.y, PlatformMeshBuffer::SURFACE);

                this->addPlatformQuad(toWorld(topPoints[i], topZFront), toWorld(topPoints[i + 1], topZFront), toWorld(rimPoints[i + 1], topZFront), toWorld(rimPoints[i], topZFront), Ogre::Vector3(0.0f, 0.0f, -1.0f), u0s, u1s, 0.0f, 1.0f,
                    PlatformMeshBuffer::SURFACE);
                this->addPlatformQuad(toWorld(rimPoints[i], topZFront), toWorld(rimPoints[i + 1], topZFront), toWorld(rimPoints[i + 1], midZFront), toWorld(rimPoints[i], midZFront), upNormal, u0s, u1s, 0.0f, 1.0f, PlatformMeshBuffer::SURFACE);
                this->addPlatformQuad(toWorld(rimPoints[i + 1], midZFront), toWorld(rimPoints[i], midZFront), toWorld(topPoints[i], midZFront), toWorld(topPoints[i + 1], midZFront), Ogre::Vector3(0.0f, 0.0f, 1.0f), u0s, u1s, 0.0f, 1.0f,
                    PlatformMeshBuffer::SURFACE);

                this->addPlatformQuad(toWorld(topPoints[i + 1], topZBack), toWorld(topPoints[i], topZBack), toWorld(rimPoints[i], topZBack), toWorld(rimPoints[i + 1], topZBack), Ogre::Vector3(0.0f, 0.0f, 1.0f), u0s, u1s, 0.0f, 1.0f,
                    PlatformMeshBuffer::SURFACE);
                this->addPlatformQuad(toWorld(rimPoints[i + 1], topZBack), toWorld(rimPoints[i], topZBack), toWorld(rimPoints[i], midZBack), toWorld(rimPoints[i + 1], midZBack), upNormal, u0s, u1s, 0.0f, 1.0f, PlatformMeshBuffer::SURFACE);
                this->addPlatformQuad(toWorld(rimPoints[i], midZBack), toWorld(rimPoints[i + 1], midZBack), toWorld(topPoints[i + 1], midZBack), toWorld(topPoints[i], midZBack), Ogre::Vector3(0.0f, 0.0f, -1.0f), u0s, u1s, 0.0f, 1.0f,
                    PlatformMeshBuffer::SURFACE);
            }
            else
            {
                // ── Plain flat top surface ───────────────────────────────────────────
                this->addPlatformQuad(toWorld(topPoints[i], topZFront), toWorld(topPoints[i + 1], topZFront), toWorld(topPoints[i + 1], topZBack), toWorld(topPoints[i], topZBack), upNormal, u0s, u1s, 0.0f, surfUV.y, PlatformMeshBuffer::SURFACE);
            }

            // ── Bottom surface ───────────────────────────────────────────────────────
            this->addPlatformQuad(toWorld(bottomPoints[i], zBack), toWorld(bottomPoints[i + 1], zBack), toWorld(bottomPoints[i + 1], zFront), toWorld(bottomPoints[i], zFront), -upNormal, u0g, u1g, 0.0f, groundUV.y, PlatformMeshBuffer::GROUND);

            // ── Front face (z = zFront) ───────────────────────────────────────────────
            this->addPlatformQuad(toWorld(bevelPoints[i], zFront), toWorld(bottomPoints[i], zFront), toWorld(bottomPoints[i + 1], zFront), toWorld(bevelPoints[i + 1], zFront), Ogre::Vector3(0.0f, 0.0f, -1.0f), u0g, u1g, 0.0f, groundUV.y,
                PlatformMeshBuffer::GROUND);

            // ── Back face (z = zBack) ─────────────────────────────────────────────────
            this->addPlatformQuad(toWorld(bevelPoints[i + 1], zBack), toWorld(bottomPoints[i + 1], zBack), toWorld(bottomPoints[i], zBack), toWorld(bevelPoints[i], zBack), Ogre::Vector3(0.0f, 0.0f, 1.0f), u0g, u1g, 0.0f, groundUV.y,
                PlatformMeshBuffer::GROUND);

            // ── Chamfer bevel (Grass) ─────────────────────────────────────────────────
            if (bevel > 0.0f)
            {
                Ogre::Vector3 bevelFrontNormal = upNormal + Ogre::Vector3(0.0f, 0.0f, -1.0f);
                if (bevelFrontNormal.squaredLength() > 1e-8f)
                {
                    bevelFrontNormal.normalise();
                }
                else
                {
                    bevelFrontNormal = Ogre::Vector3(0.0f, 0.0f, -1.0f);
                }

                Ogre::Vector3 bevelBackNormal = upNormal + Ogre::Vector3(0.0f, 0.0f, 1.0f);
                if (bevelBackNormal.squaredLength() > 1e-8f)
                {
                    bevelBackNormal.normalise();
                }
                else
                {
                    bevelBackNormal = Ogre::Vector3(0.0f, 0.0f, 1.0f);
                }

                this->addPlatformQuad(toWorld(topPoints[i], topZFront), toWorld(topPoints[i + 1], topZFront), toWorld(bevelPoints[i + 1], zFront), toWorld(bevelPoints[i], zFront), bevelFrontNormal, u0s, u1s, 0.0f, 1.0f, PlatformMeshBuffer::SURFACE);

                this->addPlatformQuad(toWorld(topPoints[i + 1], topZBack), toWorld(topPoints[i], topZBack), toWorld(bevelPoints[i], zBack), toWorld(bevelPoints[i + 1], zBack), bevelBackNormal, u0s, u1s, 0.0f, 1.0f, PlatformMeshBuffer::SURFACE);
            }
        }

        // ── End caps ─────────────────────────────────────────────────────────
        // Drawn at every end that is not chained to a following run. Since the junction
        // architecture change (see generateJunctionPatch) this now also includes ends that
        // connect to a junction: nothing fills that opening any more, so the cap has to. The
        // overshoot applied in rebuildMesh buries such a cap inside the neighbouring arms'
        // solid volume, so it is never actually visible there.
        //
        // BUGFIX: this used to close the end with one flat rectangle spanning the full depth
        // at the un-beveled height (h), regardless of style. That is correct for the plain-box
        // styles (Wood/Stone/Ice), but for a beveled style (Grass) the real top surface chamfers
        // down to (h - bevel) at zFront/zBack - the flat rectangle stood taller than that
        // chamfer at both edges, so a thin ridge of the cap poked up past the real chamfered
        // corners right where the cap meets the top surface (and the analogous mismatch exists
        // for Metal's raised rim). Now the cap is built from the SAME cross-section outline the
        // sweep above uses at any x - plain rectangle / bevel hexagon / rim profile - just closed
        // off with a triangle fan instead of extruded, so it always sits flush with the real top
        // surface whichever style is active.
        //
        // BUGFIX (miter): the profile is no longer expressed in absolute Y. Each entry is now
        // (z, offsetAlongDown) - a distance measured from the top point along that end's own
        // down direction - so the cap follows the mitered cross-section instead of a flat slice
        // at constant X. For a horizontal platform down is -Y and this is identical to before;
        // for a vertical pull the cap correctly becomes a horizontal lid rather than collapsing
        // into the wall's own plane.
        auto addEndCap = [this, depth, bevel, rim, rimBand, zFront, zBack, groundUV](const Ogre::Vector2& capTop, const Ogre::Vector2& capDown, Ogre::Real capThickness, const Ogre::Vector3& outDir)
        {
            std::vector<Ogre::Vector2> profile;
            if (rim > 0.0f && depth > rimBand * 2.5f)
            {
                const Ogre::Real topZFront = zFront + bevel;
                const Ogre::Real topZBack = zBack - bevel;
                const Ogre::Real midZFront = topZFront + rimBand;
                const Ogre::Real midZBack = topZBack - rimBand;

                profile = {Ogre::Vector2(zFront, capThickness), Ogre::Vector2(zFront, bevel), Ogre::Vector2(topZFront, 0.0f), Ogre::Vector2(topZFront, -rim), Ogre::Vector2(midZFront, -rim), Ogre::Vector2(midZFront, 0.0f),
                    Ogre::Vector2(midZBack, 0.0f), Ogre::Vector2(midZBack, -rim), Ogre::Vector2(topZBack, -rim), Ogre::Vector2(topZBack, 0.0f), Ogre::Vector2(zBack, bevel), Ogre::Vector2(zBack, capThickness)};
            }
            else if (bevel > 0.0f)
            {
                const Ogre::Real topZFront = zFront + bevel;
                const Ogre::Real topZBack = zBack - bevel;

                profile = {Ogre::Vector2(zFront, capThickness), Ogre::Vector2(zFront, bevel), Ogre::Vector2(topZFront, 0.0f), Ogre::Vector2(topZBack, 0.0f), Ogre::Vector2(zBack, bevel), Ogre::Vector2(zBack, capThickness)};
            }
            else
            {
                profile = {Ogre::Vector2(zFront, capThickness), Ogre::Vector2(zFront, 0.0f), Ogre::Vector2(zBack, 0.0f), Ogre::Vector2(zBack, capThickness)};
            }

            // Fan-triangulate from the first perimeter point. addPlatformQuad recomputes each
            // triangle's own winding from the outDir normal passed in, so the perimeter walk
            // direction doesn't matter here - only that consecutive profile entries are
            // boundary neighbours.
            Ogre::Real perim = 0.0f;
            std::vector<Ogre::Real> param(profile.size(), 0.0f);
            for (size_t i = 1; i < profile.size(); ++i)
            {
                perim += (profile[i] - profile[i - 1]).length();
                param[i] = perim;
            }

            auto toVec3 = [capTop, capDown](const Ogre::Vector2& zd)
            {
                return Ogre::Vector3(capTop.x + capDown.x * zd.y, capTop.y + capDown.y * zd.y, zd.x);
            };

            for (size_t i = 1; i + 1 < profile.size(); ++i)
            {
                this->addPlatformQuad(toVec3(profile[0]), toVec3(profile[i]), toVec3(profile[i + 1]), toVec3(profile[i + 1]), outDir, param[0] * groundUV.x, param[i + 1] * groundUV.x, 0.0f, groundUV.y, PlatformMeshBuffer::GROUND);
            }
        };

        if (false == skipFrontCap)
        {
            const PlatformControlPoint& p0 = points.front();
            const PlatformControlPoint& p1 = points[1];
            Ogre::Vector3 outDir(p0.position.x - p1.position.x, p0.smoothedHeight - p1.smoothedHeight, 0.0f);
            if (outDir.squaredLength() > 1e-8f)
            {
                outDir.normalise();
            }
            else
            {
                outDir = Ogre::Vector3(-1.0f, 0.0f, 0.0f);
            }

            addEndCap(topPoints.front(), downDirs.front(), (bottomPoints.front() - topPoints.front()).length(), outDir);
        }
        if (false == skipBackCap)
        {
            const size_t n = numPoints;
            const PlatformControlPoint& p0 = points[n - 1];
            const PlatformControlPoint& p1 = points[n - 2];
            Ogre::Vector3 outDir(p0.position.x - p1.position.x, p0.smoothedHeight - p1.smoothedHeight, 0.0f);
            if (outDir.squaredLength() > 1e-8f)
            {
                outDir.normalise();
            }
            else
            {
                outDir = Ogre::Vector3(1.0f, 0.0f, 0.0f);
            }

            addEndCap(topPoints.back(), downDirs.back(), (bottomPoints.back() - topPoints.back()).length(), outDir);
        }
    }

    void ProceduralPlatformComponent::generateGrassPlatform(const std::vector<PlatformControlPoint>& points, bool skipFrontCap, bool skipBackCap)
    {
        // Rounded grass-over-dirt look (see the reference moodboard) via a top chamfer.
        this->generatePlatformBox(points, std::min(0.15f, this->platformDepth->getReal() * 0.2f), 0.0f, skipFrontCap, skipBackCap);
    }

    void ProceduralPlatformComponent::generateWoodPlatform(const std::vector<PlatformControlPoint>& points, bool skipFrontCap, bool skipBackCap)
    {
        this->generatePlatformBox(points, 0.0f, 0.0f, skipFrontCap, skipBackCap);
    }

    void ProceduralPlatformComponent::generateStonePlatform(const std::vector<PlatformControlPoint>& points, bool skipFrontCap, bool skipBackCap)
    {
        this->generatePlatformBox(points, 0.0f, 0.0f, skipFrontCap, skipBackCap);
    }

    void ProceduralPlatformComponent::generateIcePlatform(const std::vector<PlatformControlPoint>& points, bool skipFrontCap, bool skipBackCap)
    {
        this->generatePlatformBox(points, 0.0f, 0.0f, skipFrontCap, skipBackCap);
    }

    void ProceduralPlatformComponent::generateMetalPlatform(const std::vector<PlatformControlPoint>& points, bool skipFrontCap, bool skipBackCap)
    {
        // Thin raised rim/frame around the top perimeter.
        this->generatePlatformBox(points, 0.0f, std::min(0.05f, this->platformHeight->getReal() * 0.15f), skipFrontCap, skipBackCap);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Junctions
    //
    // ARCHITECTURE CHANGE: a junction generates NO geometry at all any more.
    //
    // Every previous attempt built something to fill the space between the arms: a fan through
    // a shared centre, ear-clipped arm corners, an ear-clipped combined top+bottom ring,
    // separate top/bottom caps plus a per-arm connector, N boundary-edge bands with no cap, and
    // most recently a simple hub box. All of them carried over the assumption that holds for
    // ProceduralRoadComponent - that arms are flat ribbons whose overlapping top surfaces are
    // directly visible and therefore have to be trimmed away and replaced by a patch.
    //
    // A platform arm is not a ribbon. It is a vertical slab in the XY plane, and every arm
    // occupies the exact same Z range [-depth/2, +depth/2]. Two slabs crossing in XY simply
    // share solid volume. Take a horizontal arm along y = 0 covering y in [-h, 0] and a 45
    // degree arm rising from the same point covering y in [x-h, x]: their union at x > 0 is
    // y in [-h, x] - continuous, no gap, and the wedge between the two arms correctly empty.
    // That union IS the shape the user asked for. There is nothing for a patch to fill, and
    // the trimming that made a patch seem necessary was itself what blew the hub box up: the
    // angle-driven armTrimDists (clamped up to platformHeight * 6) can cut a steep arm back
    // several metres, whose corner then sits metres above the horizontal arms, which forced
    // the hub to span that whole height range.
    //
    // The two artifacts that DO remain from letting the arms interpenetrate are both handled
    // in rebuildMesh, without any polygon math:
    //   - Coplanar front/back faces of two overlapping arms. Solved by per-junction depth
    //     nesting through g_platformRunDepthOverride (nestLevel), the same 1 cm trick that
    //     already separates consecutive monotonic runs at a reversal corner.
    //   - Coplanar end caps at the shared junction plane. Depth nesting cannot help there,
    //     since it offsets Z while the caps are X planes. Solved instead by pushing each arm's
    //     endpoint slightly PAST the junction centre (junctionOvershoot): the overshoot
    //     direction always points into the interior of the union, so each cap ends up buried
    //     inside the other arms' solid volume and on its own distinct plane.
    //
    // Known limitation: nestLevel takes the max over a chain's two junctions, so a chain that
    // is arm 0 of one junction and arm 2 of another takes level 2 and could in principle
    // collide with another level-2 arm at the first junction. The consequence is a small
    // z-fighting triangle at that one junction, not broken geometry. The proper fix would be a
    // pre-pass greedy colouring over the chain adjacency graph; deliberately not built until
    // the case is actually observed.
    //
    // The arm-list logging below is kept: it is still the fastest way to see what actually
    // converges at a junction when something looks wrong.
    ///////////////////////////////////////////////////////////////////////////////////////////////

    void ProceduralPlatformComponent::generateJunctionPatch(const JunctionPoint& jp, const Ogre::Vector3& origin)
    {
        const size_t numArms = jp.patchCorners.size();
        if (numArms < 2)
        {
            return;
        }

        // [JUNCTION-DEBUG] arm list - positions are relative to the mesh origin, matching what
        // [VERTEXDUMP] prints, so a suspicious arm can be cross-referenced directly against the
        // vertices actually generated for it.
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JUNCTION-DEBUG] junction at world(" + Ogre::StringConverter::toString(jp.worldPos.x) + "," + Ogre::StringConverter::toString(jp.worldPos.y) +
                                                                                ") numArms=" + Ogre::StringConverter::toString(static_cast<int>(numArms)) + " segIndices=" + Ogre::StringConverter::toString(static_cast<int>(jp.segIndices.size())));

        for (size_t ci = 0; ci < numArms; ++ci)
        {
            const Ogre::Real x = jp.patchCorners[ci].x - origin.x;
            const Ogre::Real h = jp.patchCorners[ci].y - origin.y;

            Ogre::Real armDepth = this->platformDepth->getReal();
            if (ci < jp.armDepths.size())
            {
                armDepth = jp.armDepths[ci];
            }

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                "[JUNCTION-DEBUG]   arm[" + Ogre::StringConverter::toString(static_cast<int>(ci)) + "] x=" + Ogre::StringConverter::toString(x) + " h=" + Ogre::StringConverter::toString(h) + " depth=" + Ogre::StringConverter::toString(armDepth));
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Mesh Creation
    ///////////////////////////////////////////////////////////////////////////////////////////////

    void ProceduralPlatformComponent::createPlatformMesh(void)
    {
        this->destroyPlatformMesh();

        if (this->currentSurfaceVertexIndex == 0 && this->currentGroundVertexIndex == 0 && this->currentJunctionVertexIndex == 0)
        {
            return;
        }

        const auto& physicsArtifactCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsArtifactComponent>());
        if (physicsArtifactCompPtr)
        {
            this->physicsArtifactComponent = physicsArtifactCompPtr.get();
        }

        this->cachedSurfaceVertices = this->surfaceVertices;
        this->cachedSurfaceIndices = this->surfaceIndices;
        this->cachedNumSurfaceVertices = this->currentSurfaceVertexIndex;

        this->cachedGroundVertices = this->groundVertices;
        this->cachedGroundIndices = this->groundIndices;
        this->cachedNumGroundVertices = this->currentGroundVertexIndex;

        this->cachedJunctionVertices = this->junctionVertices;
        this->cachedJunctionIndices = this->junctionIndices;
        this->cachedNumJunctionVertices = this->currentJunctionVertexIndex;

        this->cachedPlatformOrigin = this->platformOrigin;

        this->logSuspectedZFightingPairs();
        this->logSuspectedCoplanarOverlaps();
        this->logAllVertexPositions();

        std::vector<float> surfaceVerticesCopy = this->surfaceVertices;
        std::vector<Ogre::uint32> surfaceIndicesCopy = this->surfaceIndices;
        size_t numSurfaceVertices = this->currentSurfaceVertexIndex;

        std::vector<float> groundVerticesCopy = this->groundVertices;
        std::vector<Ogre::uint32> groundIndicesCopy = this->groundIndices;
        size_t numGroundVertices = this->currentGroundVertexIndex;

        std::vector<float> junctionVerticesCopy = this->junctionVertices;
        std::vector<Ogre::uint32> junctionIndicesCopy = this->junctionIndices;
        size_t numJunctionVertices = this->currentJunctionVertexIndex;

        Ogre::Vector3 platformOriginCopy = this->platformOrigin;

        GraphicsModule::RenderCommand renderCommand =
            [this, surfaceVerticesCopy, surfaceIndicesCopy, numSurfaceVertices, groundVerticesCopy, groundIndicesCopy, numGroundVertices, junctionVerticesCopy, junctionIndicesCopy, numJunctionVertices, platformOriginCopy]()
        {
            this->createPlatformMeshInternal(surfaceVerticesCopy, surfaceIndicesCopy, numSurfaceVertices, groundVerticesCopy, groundIndicesCopy, numGroundVertices, junctionVerticesCopy, junctionIndicesCopy, numJunctionVertices, platformOriginCopy);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralPlatformComponent::createPlatformMesh");

        this->surfaceVertices.clear();
        this->surfaceIndices.clear();
        this->groundVertices.clear();
        this->groundIndices.clear();
        this->junctionVertices.clear();
        this->junctionIndices.clear();
    }

    void ProceduralPlatformComponent::logSuspectedZFightingPairs(void)
    {
        struct TriInfo
        {
            Ogre::Vector3 centroid;
            Ogre::Vector3 normal;
            Ogre::Vector3 v0, v1, v2;
            const char* buffer;
            size_t triIndex;
        };
        std::vector<TriInfo> tris;

        const size_t floatsPerVert = 8; // pos.xyz, normal.xyz, uv.xy - matches every addPlatformQuad push

        auto extract = [&](const std::vector<float>& verts, const std::vector<Ogre::uint32>& inds, const char* bufferName)
        {
            auto getPos = [&](Ogre::uint32 idx) -> Ogre::Vector3
            {
                const size_t base = static_cast<size_t>(idx) * floatsPerVert;
                if (base + 2 >= verts.size())
                {
                    return Ogre::Vector3::ZERO;
                }
                return Ogre::Vector3(verts[base + 0], verts[base + 1], verts[base + 2]);
            };
            for (size_t i = 0; i + 2 < inds.size(); i += 3)
            {
                const Ogre::Vector3 v0 = getPos(inds[i]);
                const Ogre::Vector3 v1 = getPos(inds[i + 1]);
                const Ogre::Vector3 v2 = getPos(inds[i + 2]);
                const Ogre::Vector3 centroid = (v0 + v1 + v2) / 3.0f;
                Ogre::Vector3 normal = (v1 - v0).crossProduct(v2 - v0);
                if (normal.squaredLength() > 1e-10f)
                {
                    normal.normalise();
                }
                tris.push_back({centroid, normal, v0, v1, v2, bufferName, i / 3});
            }
        };

        extract(this->surfaceVertices, this->surfaceIndices, "SURFACE");
        extract(this->groundVertices, this->groundIndices, "GROUND");
        extract(this->junctionVertices, this->junctionIndices, "JUNCTION");

        // 2cm centroid tolerance (generous enough to catch near-but-not-exactly coincident
        // triangles - the kind that flickers depending on viewing angle/float rounding -
        // without also matching unrelated nearby geometry) and ~11 degrees of normal
        // parallelism (abs() so opposite-facing coincident pairs - e.g. a front face and a
        // back face sitting at the same spot - count too, since those z-fight just as badly).
        // Widened after an initial pass at 2cm/0.98 found nothing despite a clear visual
        // z-fighting pattern reported by the user - casting a wider net to catch pairs that
        // are close but not exactly coincident (which can still flicker depending on
        // viewing angle and depth-buffer precision at a distance).
        const Ogre::Real centroidEpsilon = 0.15f;
        const Ogre::Real normalDotThreshold = 0.85f;

        int foundCount = 0;
        for (size_t a = 0; a < tris.size(); ++a)
        {
            for (size_t b = a + 1; b < tris.size(); ++b)
            {
                if (tris[a].buffer == tris[b].buffer && tris[a].triIndex == tris[b].triIndex)
                {
                    continue;
                }
                const Ogre::Real centroidDist = tris[a].centroid.distance(tris[b].centroid);
                if (centroidDist > centroidEpsilon)
                {
                    continue;
                }
                const Ogre::Real normalDot = std::abs(tris[a].normal.dotProduct(tris[b].normal));
                if (normalDot < normalDotThreshold)
                {
                    continue;
                }

                ++foundCount;
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, Ogre::String("[ZFIGHT-DEBUG] suspected pair: ") + tris[a].buffer + "#" + Ogre::StringConverter::toString((int)tris[a].triIndex) + " vs " + tris[b].buffer + "#" +
                                                                                        Ogre::StringConverter::toString((int)tris[b].triIndex) + " centroidDist=" + Ogre::StringConverter::toString(centroidDist) +
                                                                                        " normalDot=" + Ogre::StringConverter::toString(normalDot));
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                    Ogre::String("[ZFIGHT-DEBUG]   A(") + tris[a].buffer + "): (" + Ogre::StringConverter::toString(tris[a].v0.x) + "," + Ogre::StringConverter::toString(tris[a].v0.y) + "," + Ogre::StringConverter::toString(tris[a].v0.z) + ") (" +
                        Ogre::StringConverter::toString(tris[a].v1.x) + "," + Ogre::StringConverter::toString(tris[a].v1.y) + "," + Ogre::StringConverter::toString(tris[a].v1.z) + ") (" + Ogre::StringConverter::toString(tris[a].v2.x) + "," +
                        Ogre::StringConverter::toString(tris[a].v2.y) + "," + Ogre::StringConverter::toString(tris[a].v2.z) + ") normal=(" + Ogre::StringConverter::toString(tris[a].normal.x) + "," + Ogre::StringConverter::toString(tris[a].normal.y) +
                        "," + Ogre::StringConverter::toString(tris[a].normal.z) + ")");
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                    Ogre::String("[ZFIGHT-DEBUG]   B(") + tris[b].buffer + "): (" + Ogre::StringConverter::toString(tris[b].v0.x) + "," + Ogre::StringConverter::toString(tris[b].v0.y) + "," + Ogre::StringConverter::toString(tris[b].v0.z) + ") (" +
                        Ogre::StringConverter::toString(tris[b].v1.x) + "," + Ogre::StringConverter::toString(tris[b].v1.y) + "," + Ogre::StringConverter::toString(tris[b].v1.z) + ") (" + Ogre::StringConverter::toString(tris[b].v2.x) + "," +
                        Ogre::StringConverter::toString(tris[b].v2.y) + "," + Ogre::StringConverter::toString(tris[b].v2.z) + ") normal=(" + Ogre::StringConverter::toString(tris[b].normal.x) + "," + Ogre::StringConverter::toString(tris[b].normal.y) +
                        "," + Ogre::StringConverter::toString(tris[b].normal.z) + ")");
            }
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
            "[ZFIGHT-DEBUG] scan complete: " + Ogre::StringConverter::toString((int)tris.size()) + " triangles checked, " + Ogre::StringConverter::toString(foundCount) + " suspected z-fighting pairs found");
    }

    void ProceduralPlatformComponent::logSuspectedCoplanarOverlaps(void)
    {
        struct TriInfo
        {
            Ogre::Vector3 v0, v1, v2;
            Ogre::Vector3 normal;
            const char* buffer;
            size_t triIndex;
        };
        std::vector<TriInfo> tris;

        const size_t floatsPerVert = 8;

        auto extract = [&](const std::vector<float>& verts, const std::vector<Ogre::uint32>& inds, const char* bufferName)
        {
            auto getPos = [&](Ogre::uint32 idx) -> Ogre::Vector3
            {
                const size_t base = static_cast<size_t>(idx) * floatsPerVert;
                if (base + 2 >= verts.size())
                {
                    return Ogre::Vector3::ZERO;
                }
                return Ogre::Vector3(verts[base + 0], verts[base + 1], verts[base + 2]);
            };
            for (size_t i = 0; i + 2 < inds.size(); i += 3)
            {
                const Ogre::Vector3 v0 = getPos(inds[i]);
                const Ogre::Vector3 v1 = getPos(inds[i + 1]);
                const Ogre::Vector3 v2 = getPos(inds[i + 2]);
                Ogre::Vector3 normal = (v1 - v0).crossProduct(v2 - v0);
                if (normal.squaredLength() > 1e-10f)
                {
                    normal.normalise();
                }
                tris.push_back({v0, v1, v2, normal, bufferName, i / 3});
            }
        };

        extract(this->surfaceVertices, this->surfaceIndices, "SURFACE");
        extract(this->groundVertices, this->groundIndices, "GROUND");
        extract(this->junctionVertices, this->junctionIndices, "JUNCTION");

        // Barycentric point-in-triangle test, done in the triangle's own plane.
        auto pointInTriangle2D = [](const Ogre::Vector3& p, const Ogre::Vector3& a, const Ogre::Vector3& b, const Ogre::Vector3& c) -> bool
        {
            const Ogre::Vector3 v0v = c - a;
            const Ogre::Vector3 v1v = b - a;
            const Ogre::Vector3 v2v = p - a;
            const Ogre::Real dot00 = v0v.dotProduct(v0v);
            const Ogre::Real dot01 = v0v.dotProduct(v1v);
            const Ogre::Real dot02 = v0v.dotProduct(v2v);
            const Ogre::Real dot11 = v1v.dotProduct(v1v);
            const Ogre::Real dot12 = v1v.dotProduct(v2v);
            const Ogre::Real denom = dot00 * dot11 - dot01 * dot01;
            if (std::abs(denom) < 1e-12f)
            {
                return false;
            }
            const Ogre::Real invDenom = 1.0f / denom;
            const Ogre::Real u = (dot11 * dot02 - dot01 * dot12) * invDenom;
            const Ogre::Real v = (dot00 * dot12 - dot01 * dot02) * invDenom;
            return (u >= 0.05f) && (v >= 0.05f) && (u + v <= 0.95f);
        };

        const Ogre::Real planeEpsilon = 0.01f; // 1cm - how far off-plane a vertex can be and still count as "coplanar"
        int foundCount = 0;

        auto sharedVertexCount = [](const TriInfo& t1, const TriInfo& t2) -> int
        {
            const Ogre::Vector3 aVerts[3] = {t1.v0, t1.v1, t1.v2};
            const Ogre::Vector3 bVerts[3] = {t2.v0, t2.v1, t2.v2};
            int shared = 0;
            for (int i = 0; i < 3; ++i)
            {
                for (int j = 0; j < 3; ++j)
                {
                    if (aVerts[i].squaredDistance(bVerts[j]) < 1e-6f)
                    {
                        ++shared;
                        break;
                    }
                }
            }
            return shared;
        };

        for (size_t a = 0; a < tris.size(); ++a)
        {
            for (size_t b = a + 1; b < tris.size(); ++b)
            {
                if (tris[a].buffer == tris[b].buffer && tris[a].triIndex == tris[b].triIndex)
                {
                    continue;
                }
                const Ogre::Real normalDot = std::abs(tris[a].normal.dotProduct(tris[b].normal));
                if (normalDot < 0.98f)
                {
                    continue; // not near-parallel, can't be a meaningful coplanar overlap
                }

                // Coplanarity: every vertex of B must sit close to A's plane, and vice versa.
                const Ogre::Real dB0 = std::abs((tris[b].v0 - tris[a].v0).dotProduct(tris[a].normal));
                const Ogre::Real dB1 = std::abs((tris[b].v1 - tris[a].v0).dotProduct(tris[a].normal));
                const Ogre::Real dB2 = std::abs((tris[b].v2 - tris[a].v0).dotProduct(tris[a].normal));
                if (dB0 > planeEpsilon || dB1 > planeEpsilon || dB2 > planeEpsilon)
                {
                    continue;
                }

                // BUGFIX: sharing 2+ vertices is completely normal for adjacent coplanar
                // quads (e.g. a flat run's bottom face is many quads in the same plane,
                // each pair of neighbors sharing an edge) - that is not an overlap, it's
                // ordinary mesh connectivity. Only triangles that DON'T already share most
                // of their boundary are worth checking for genuine area overlap.
                if (sharedVertexCount(tris[a], tris[b]) >= 2)
                {
                    continue;
                }

                // Coplanar, near-parallel, not just edge-adjacent - now check for GENUINE
                // area overlap using only the centroid test (vertex-in-triangle tests are
                // inherently boundary-prone for adjacent shapes and were the main source of
                // false positives at the previous, looser tolerance).
                const Ogre::Vector3 centroidA = (tris[a].v0 + tris[a].v1 + tris[a].v2) / 3.0f;
                const Ogre::Vector3 centroidB = (tris[b].v0 + tris[b].v1 + tris[b].v2) / 3.0f;

                bool overlaps = pointInTriangle2D(centroidB, tris[a].v0, tris[a].v1, tris[a].v2) || pointInTriangle2D(centroidA, tris[b].v0, tris[b].v1, tris[b].v2);

                if (false == overlaps)
                {
                    continue;
                }

                ++foundCount;
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, Ogre::String("[COPLANAR-DEBUG] suspected area overlap: ") + tris[a].buffer + "#" + Ogre::StringConverter::toString((int)tris[a].triIndex) + " vs " + tris[b].buffer +
                                                                                        "#" + Ogre::StringConverter::toString((int)tris[b].triIndex) + " normalDot=" + Ogre::StringConverter::toString(normalDot));
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, Ogre::String("[COPLANAR-DEBUG]   A(") + tris[a].buffer + "): (" + Ogre::StringConverter::toString(tris[a].v0.x) + "," +
                                                                                        Ogre::StringConverter::toString(tris[a].v0.y) + "," + Ogre::StringConverter::toString(tris[a].v0.z) + ") (" + Ogre::StringConverter::toString(tris[a].v1.x) +
                                                                                        "," + Ogre::StringConverter::toString(tris[a].v1.y) + "," + Ogre::StringConverter::toString(tris[a].v1.z) + ") (" +
                                                                                        Ogre::StringConverter::toString(tris[a].v2.x) + "," + Ogre::StringConverter::toString(tris[a].v2.y) + "," + Ogre::StringConverter::toString(tris[a].v2.z) + ")");
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, Ogre::String("[COPLANAR-DEBUG]   B(") + tris[b].buffer + "): (" + Ogre::StringConverter::toString(tris[b].v0.x) + "," +
                                                                                        Ogre::StringConverter::toString(tris[b].v0.y) + "," + Ogre::StringConverter::toString(tris[b].v0.z) + ") (" + Ogre::StringConverter::toString(tris[b].v1.x) +
                                                                                        "," + Ogre::StringConverter::toString(tris[b].v1.y) + "," + Ogre::StringConverter::toString(tris[b].v1.z) + ") (" +
                                                                                        Ogre::StringConverter::toString(tris[b].v2.x) + "," + Ogre::StringConverter::toString(tris[b].v2.y) + "," + Ogre::StringConverter::toString(tris[b].v2.z) + ")");
            }
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
            "[COPLANAR-DEBUG] scan complete: " + Ogre::StringConverter::toString((int)tris.size()) + " triangles checked, " + Ogre::StringConverter::toString(foundCount) + " suspected coplanar area overlaps found");
    }

    void ProceduralPlatformComponent::logAllVertexPositions(void)
    {
        const size_t floatsPerVert = 8; // pos.xyz, normal.xyz, uv.xy

        auto dumpBuffer = [&](const std::vector<float>& verts, const std::vector<Ogre::uint32>& inds, const char* bufferName)
        {
            const size_t numVerts = verts.size() / floatsPerVert;
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                Ogre::String("[VERTEXDUMP] ") + bufferName + ": " + Ogre::StringConverter::toString((int)numVerts) + " vertices, " + Ogre::StringConverter::toString((int)(inds.size() / 3)) + " triangles");

            for (size_t v = 0; v < numVerts; ++v)
            {
                const size_t base = v * floatsPerVert;
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                    Ogre::String("[VERTEXDUMP]   ") + bufferName + "[" + Ogre::StringConverter::toString((int)v) + "] pos=(" + Ogre::StringConverter::toString(verts[base + 0]) + "," + Ogre::StringConverter::toString(verts[base + 1]) + "," +
                        Ogre::StringConverter::toString(verts[base + 2]) + ") normal=(" + Ogre::StringConverter::toString(verts[base + 3]) + "," + Ogre::StringConverter::toString(verts[base + 4]) + "," +
                        Ogre::StringConverter::toString(verts[base + 5]) + ") uv=(" + Ogre::StringConverter::toString(verts[base + 6]) + "," + Ogre::StringConverter::toString(verts[base + 7]) + ")");
            }

            for (size_t i = 0; i + 2 < inds.size(); i += 3)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, Ogre::String("[VERTEXDUMP]   ") + bufferName + " tri#" + Ogre::StringConverter::toString((int)(i / 3)) + " uses indices (" +
                                                                                        Ogre::StringConverter::toString((int)inds[i]) + "," + Ogre::StringConverter::toString((int)inds[i + 1]) + "," +
                                                                                        Ogre::StringConverter::toString((int)inds[i + 2]) + ")");
            }
        };

        dumpBuffer(this->surfaceVertices, this->surfaceIndices, "SURFACE");
        dumpBuffer(this->groundVertices, this->groundIndices, "GROUND");
        dumpBuffer(this->junctionVertices, this->junctionIndices, "JUNCTION");
    }

    void ProceduralPlatformComponent::createPlatformMeshInternal(const std::vector<float>& surfaceVerts, const std::vector<Ogre::uint32>& surfaceInds, size_t numSurfaceVerts, const std::vector<float>& groundVerts,
        const std::vector<Ogre::uint32>& groundInds, size_t numGroundVerts, const std::vector<float>& junctionVerts, const std::vector<Ogre::uint32>& junctionInds, size_t numJunctionVerts, const Ogre::Vector3& origin)
    {
        Ogre::Root* root = Ogre::Root::getSingletonPtr();
        Ogre::RenderSystem* renderSystem = root->getRenderSystem();
        Ogre::VaoManager* vaoManager = renderSystem->getVaoManager();

        Ogre::String meshName = this->gameObjectPtr->getName() + "_Platform_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
        const Ogre::String groupName = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

        {
            Ogre::MeshManager& meshMgr = Ogre::MeshManager::getSingleton();
            Ogre::MeshPtr existing = meshMgr.getByName(meshName, groupName);
            if (false == existing.isNull())
            {
                meshMgr.remove(existing->getHandle());
            }
        }

        this->platformMesh = Ogre::MeshManager::getSingleton().createManual(meshName, groupName, &NOWA::gDummyMeshLoader);

        Ogre::VertexElement2Vec vertexElements;
        vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
        vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));
        vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT4, Ogre::VES_TANGENT));
        vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));

        const size_t srcFloatsPerVertex = 8;
        const size_t dstFloatsPerVertex = 12;

        Ogre::Vector3 minBounds(std::numeric_limits<float>::max());
        Ogre::Vector3 maxBounds(std::numeric_limits<float>::lowest());

        // Same VAO-building pattern as ProceduralRoadComponent::createRoadMeshInternal's
        // buildSubMesh lambda: 8->12 float expansion (adds tangent), empty-dummy-VAO
        // fallback, bounds tracking - written once, used for all three submeshes.
        auto buildSubMesh = [&](const std::vector<float>& verts, const std::vector<Ogre::uint32>& inds, size_t numVerts, const char* emptyLogLabel) -> Ogre::SubMesh*
        {
            Ogre::SubMesh* subMesh = this->platformMesh->createSubMesh();

            if (numVerts > 0)
            {
                const size_t vertexDataSize = numVerts * dstFloatsPerVertex * sizeof(float);
                float* vertexData = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(vertexDataSize, Ogre::MEMCATEGORY_GEOMETRY));

                for (size_t i = 0; i < numVerts; ++i)
                {
                    size_t srcOffset = i * srcFloatsPerVertex;
                    size_t dstOffset = i * dstFloatsPerVertex;

                    vertexData[dstOffset + 0] = verts[srcOffset + 0];
                    vertexData[dstOffset + 1] = verts[srcOffset + 1];
                    vertexData[dstOffset + 2] = verts[srcOffset + 2];

                    Ogre::Vector3 pos(verts[srcOffset + 0], verts[srcOffset + 1], verts[srcOffset + 2]);
                    minBounds.makeFloor(pos);
                    maxBounds.makeCeil(pos);

                    Ogre::Vector3 normal(verts[srcOffset + 3], verts[srcOffset + 4], verts[srcOffset + 5]);
                    vertexData[dstOffset + 3] = normal.x;
                    vertexData[dstOffset + 4] = normal.y;
                    vertexData[dstOffset + 5] = normal.z;

                    Ogre::Vector3 tangent;
                    if (std::abs(normal.y) < 0.9f)
                    {
                        tangent = Ogre::Vector3::UNIT_Y.crossProduct(normal);
                    }
                    else
                    {
                        tangent = normal.crossProduct(Ogre::Vector3::UNIT_X);
                    }
                    tangent.normalise();

                    vertexData[dstOffset + 6] = tangent.x;
                    vertexData[dstOffset + 7] = tangent.y;
                    vertexData[dstOffset + 8] = tangent.z;
                    vertexData[dstOffset + 9] = 1.0f;

                    vertexData[dstOffset + 10] = verts[srcOffset + 6];
                    vertexData[dstOffset + 11] = verts[srcOffset + 7];
                }

                Ogre::VertexBufferPacked* vertexBuffer = nullptr;
                try
                {
                    vertexBuffer = vaoManager->createVertexBuffer(vertexElements, numVerts, Ogre::BT_IMMUTABLE, vertexData, true);
                }
                catch (Ogre::Exception& e)
                {
                    OGRE_FREE_SIMD(vertexData, Ogre::MEMCATEGORY_GEOMETRY);
                    throw e;
                }

                const size_t indexDataSize = inds.size() * sizeof(Ogre::uint32);
                Ogre::uint32* indexData = reinterpret_cast<Ogre::uint32*>(OGRE_MALLOC_SIMD(indexDataSize, Ogre::MEMCATEGORY_GEOMETRY));
                memcpy(indexData, inds.data(), indexDataSize);

                Ogre::IndexBufferPacked* indexBuffer = nullptr;
                try
                {
                    indexBuffer = vaoManager->createIndexBuffer(Ogre::IndexBufferPacked::IT_32BIT, inds.size(), Ogre::BT_IMMUTABLE, indexData, true);
                }
                catch (Ogre::Exception& e)
                {
                    OGRE_FREE_SIMD(indexData, Ogre::MEMCATEGORY_GEOMETRY);
                    throw e;
                }

                Ogre::VertexBufferPackedVec vertexBuffers;
                vertexBuffers.push_back(vertexBuffer);

                Ogre::VertexArrayObject* vao = vaoManager->createVertexArrayObject(vertexBuffers, indexBuffer, Ogre::OT_TRIANGLE_LIST);

                subMesh->mVao[Ogre::VpNormal].push_back(vao);
                subMesh->mVao[Ogre::VpShadow].push_back(vao);
            }
            else
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, Ogre::String("[ProceduralPlatformComponent] Creating empty ") + emptyLogLabel + " submesh");

                const size_t dummyVertexDataSize = 1 * dstFloatsPerVertex * sizeof(float);
                float* dummyVertexData = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(dummyVertexDataSize, Ogre::MEMCATEGORY_GEOMETRY));

                dummyVertexData[0] = 0.0f;
                dummyVertexData[1] = 0.0f;
                dummyVertexData[2] = 0.0f;

                dummyVertexData[3] = 0.0f;
                dummyVertexData[4] = 1.0f;
                dummyVertexData[5] = 0.0f;

                dummyVertexData[6] = 1.0f;
                dummyVertexData[7] = 0.0f;
                dummyVertexData[8] = 0.0f;
                dummyVertexData[9] = 1.0f;

                dummyVertexData[10] = 0.0f;
                dummyVertexData[11] = 0.0f;

                Ogre::VertexBufferPacked* dummyVertexBuffer = vaoManager->createVertexBuffer(vertexElements, 1, Ogre::BT_IMMUTABLE, dummyVertexData, true);

                Ogre::uint32* dummyIndexData = reinterpret_cast<Ogre::uint32*>(OGRE_MALLOC_SIMD(sizeof(Ogre::uint32), Ogre::MEMCATEGORY_GEOMETRY));
                dummyIndexData[0] = 0;

                Ogre::IndexBufferPacked* dummyIndexBuffer = vaoManager->createIndexBuffer(Ogre::IndexBufferPacked::IT_32BIT, 0, Ogre::BT_IMMUTABLE, dummyIndexData, true);

                Ogre::VertexBufferPackedVec dummyVertexBuffers;
                dummyVertexBuffers.push_back(dummyVertexBuffer);

                Ogre::VertexArrayObject* dummyVao = vaoManager->createVertexArrayObject(dummyVertexBuffers, dummyIndexBuffer, Ogre::OT_TRIANGLE_LIST);

                subMesh->mVao[Ogre::VpNormal].push_back(dummyVao);
                subMesh->mVao[Ogre::VpShadow].push_back(dummyVao);
            }

            return subMesh;
        };

        // ==================== SUBMESH 0: SURFACE ====================
        buildSubMesh(surfaceVerts, surfaceInds, numSurfaceVerts, "surface");

        // ==================== SUBMESH 1: GROUND =====================
        buildSubMesh(groundVerts, groundInds, numGroundVerts, "ground");

        // ==================== SUBMESH 2: JUNCTION ====================
        buildSubMesh(junctionVerts, junctionInds, numJunctionVerts, "junction");

        if (minBounds.x == std::numeric_limits<float>::max())
        {
            minBounds = Ogre::Vector3(-1, -1, -1);
            maxBounds = Ogre::Vector3(1, 1, 1);
        }

        Ogre::Aabb bounds;
        bounds.setExtents(minBounds, maxBounds);
        this->platformMesh->_setBounds(bounds, false);
        this->platformMesh->_setBoundingSphereRadius(bounds.getRadius());

        this->platformItem = this->gameObjectPtr->getSceneManager()->createItem(this->platformMesh, this->gameObjectPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC);

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPlatformComponent] Created platform item with " + Ogre::StringConverter::toString(this->platformMesh->getNumSubMeshes()) + " submeshes");

        // Submesh 0: Surface datablock
        Ogre::String surfaceDbName = this->surfaceDatablock->getString();
        if (false == surfaceDbName.empty())
        {
            Ogre::HlmsDatablock* surfaceDb = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(surfaceDbName);
            if (nullptr != surfaceDb)
            {
                this->platformItem->getSubItem(0)->setDatablock(surfaceDb);
            }
        }

        // Submesh 1: Ground datablock, falls back to Surface if left empty
        Ogre::String groundDbName = this->groundDatablock->getString();
        if (false == groundDbName.empty())
        {
            Ogre::HlmsDatablock* groundDb = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(groundDbName);
            if (nullptr != groundDb)
            {
                this->platformItem->getSubItem(1)->setDatablock(groundDb);
            }
        }
        else if (false == surfaceDbName.empty())
        {
            Ogre::HlmsDatablock* surfaceDb = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(surfaceDbName);
            if (nullptr != surfaceDb)
            {
                this->platformItem->getSubItem(1)->setDatablock(surfaceDb);
            }
        }

        // Submesh 2: Junction fill - no separate property, always mirrors Ground Datablock
        // (falling back to Surface if Ground is also empty).
        if (false == groundDbName.empty())
        {
            Ogre::HlmsDatablock* groundDb = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(groundDbName);
            if (nullptr != groundDb)
            {
                this->platformItem->getSubItem(2)->setDatablock(groundDb);
            }
        }
        else if (false == surfaceDbName.empty())
        {
            Ogre::HlmsDatablock* surfaceDb = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(surfaceDbName);
            if (nullptr != surfaceDb)
            {
                this->platformItem->getSubItem(2)->setDatablock(surfaceDb);
            }
        }

        if (false == this->originPositionSet)
        {
            this->originPositionSet = true;
            this->gameObjectPtr->getSceneNode()->setPosition(this->platformFrame * origin);
            this->gameObjectPtr->getSceneNode()->setOrientation(this->platformFrame);
        }

        this->gameObjectPtr->getSceneNode()->attachObject(this->platformItem);
        this->gameObjectPtr->setDoNotDestroyMovableObject(true);
        this->gameObjectPtr->init(this->platformItem);

        if (nullptr != this->physicsArtifactComponent)
        {
            this->physicsArtifactComponent->reCreateCollision();
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPlatformComponent] Platform mesh created with " + Ogre::StringConverter::toString(numSurfaceVerts) + " surface vertices, " +
                                                                               Ogre::StringConverter::toString(numGroundVerts) + " ground vertices, " + Ogre::StringConverter::toString(numJunctionVerts) + " junction vertices, attached to scene");
    }

    void ProceduralPlatformComponent::destroyPlatformMesh(void)
    {
        if (nullptr == this->platformItem && nullptr == this->platformMesh)
        {
            return;
        }

        GraphicsModule::RenderCommand renderCommand = [this]()
        {
            if (nullptr != this->platformItem)
            {
                if (this->platformItem->getParentSceneNode())
                {
                    this->platformItem->getParentSceneNode()->detachObject(this->platformItem);
                }
                this->gameObjectPtr->getSceneManager()->destroyItem(this->platformItem);
                this->platformItem = nullptr;
                this->gameObjectPtr->nullMovableObject();
            }

            if (this->platformMesh)
            {
                Ogre::MeshManager::getSingleton().remove(this->platformMesh->getHandle());
                this->platformMesh.reset();
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralPlatformComponent::destroyPlatformMesh");
    }

    void ProceduralPlatformComponent::destroyPreviewMesh(void)
    {
        if (nullptr == this->previewItem && nullptr == this->previewMesh)
        {
            return;
        }

        GraphicsModule::RenderCommand renderCommand = [this]()
        {
            if (nullptr != this->previewItem)
            {
                if (this->previewItem->getParentSceneNode())
                {
                    this->previewItem->getParentSceneNode()->detachObject(this->previewItem);
                }
                this->gameObjectPtr->getSceneManager()->destroyItem(this->previewItem);
                this->previewItem = nullptr;
            }

            if (nullptr != this->previewMesh)
            {
                Ogre::MeshManager::getSingleton().remove(this->previewMesh->getHandle());
                this->previewMesh.reset();
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralPlatformComponent::destroyPreviewMesh");
    }

    void ProceduralPlatformComponent::updatePreviewMesh(void)
    {
        if (this->currentSegment.controlPoints.size() < 2)
        {
            return;
        }

        if (nullptr == this->previewNode)
        {
            this->previewNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();
        }

        this->surfaceVertices.clear();
        this->surfaceIndices.clear();
        this->currentSurfaceVertexIndex = 0;

        this->groundVertices.clear();
        this->groundIndices.clear();
        this->currentGroundVertexIndex = 0;

        // Generate the current segment in LOCAL space, relative to its own start point -
        // mirrors ProceduralRoadComponent::updatePreviewMesh exactly, just with rawHeight
        // instead of a separate groundHeight, and no isCurved handling needed for a 2-point
        // preview segment.
        const Ogre::Real startX = this->currentSegment.controlPoints.front().position.x;
        const Ogre::Real startHeight = this->currentSegment.controlPoints.front().smoothedHeight;

        std::vector<PlatformControlPoint> localPoints;
        Ogre::Real accumDist = 0.0f;
        for (size_t i = 0; i < this->currentSegment.controlPoints.size(); ++i)
        {
            const PlatformControlPoint& cp = this->currentSegment.controlPoints[i];
            PlatformControlPoint localPoint;
            localPoint.position = Ogre::Vector3(cp.position.x - startX, 0.0f, 0.0f);
            localPoint.rawHeight = cp.smoothedHeight - startHeight;
            localPoint.smoothedHeight = localPoint.rawHeight;
            if (i > 0)
            {
                accumDist += pathDistance2D(localPoints.back().position.x, localPoints.back().smoothedHeight, localPoint.position.x, localPoint.smoothedHeight);
            }
            localPoint.distFromStart = accumDist;
            localPoints.push_back(localPoint);
        }

        this->generateStraightPlatform(localPoints);

        if (this->surfaceVertices.empty() && this->groundVertices.empty())
        {
            return;
        }

        std::vector<float> combinedVertices = this->surfaceVertices;
        std::vector<Ogre::uint32> combinedIndices = this->surfaceIndices;

        size_t vertexOffset = this->currentSurfaceVertexIndex;
        combinedVertices.insert(combinedVertices.end(), this->groundVertices.begin(), this->groundVertices.end());
        for (const auto& idx : this->groundIndices)
        {
            combinedIndices.push_back(idx + static_cast<Ogre::uint32>(vertexOffset));
        }

        size_t totalVertices = this->currentSurfaceVertexIndex + this->currentGroundVertexIndex;

        // BUGFIX: previewPosition.z must be the segment's ACTUAL local Z (the fixed
        // plane's constant - see raycastFixedPlane's comment: in local space the plane is
        // Z=constant, and that constant is generally NONZERO, whatever
        // platformPlaneAnchor projects to). Hardcoding 0 here put the live preview at the
        // wrong depth whenever that constant wasn't 0 - the confirmed mesh (positioned via
        // platformFrame * platformOrigin, which DOES carry the real constant) was correct,
        // so only the in-progress drag preview showed a visible Z-offset that "snapped"
        // into alignment the moment confirmPlatform() ran rebuildMesh().
        Ogre::Vector3 previewPosition(startX, startHeight, this->currentSegment.controlPoints.front().position.z);

        GraphicsModule::RenderCommand renderCommand = [this, combinedVertices, combinedIndices, totalVertices, previewPosition]()
        {
            if (nullptr != this->previewItem)
            {
                if (this->previewItem->getParentSceneNode())
                {
                    this->previewItem->getParentSceneNode()->detachObject(this->previewItem);
                }
                this->gameObjectPtr->getSceneManager()->destroyItem(this->previewItem);
                this->previewItem = nullptr;
            }

            if (nullptr != this->previewMesh)
            {
                Ogre::MeshManager::getSingleton().remove(this->previewMesh->getHandle());
                this->previewMesh.reset();
            }

            Ogre::Root* root = Ogre::Root::getSingletonPtr();
            Ogre::VaoManager* vaoManager = root->getRenderSystem()->getVaoManager();

            Ogre::String meshName = "PlatformPreview_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
            const Ogre::String groupName = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

            this->previewMesh = Ogre::MeshManager::getSingleton().createManual(meshName, groupName, &NOWA::gDummyMeshLoader);
            Ogre::SubMesh* subMesh = this->previewMesh->createSubMesh();

            Ogre::VertexElement2Vec vertexElements;
            vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
            vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));
            vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT4, Ogre::VES_TANGENT));
            vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));

            const size_t srcFloatsPerVertex = 8;
            const size_t dstFloatsPerVertex = 12;
            const size_t vertexDataSize = totalVertices * dstFloatsPerVertex * sizeof(float);
            float* vertexData = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(vertexDataSize, Ogre::MEMCATEGORY_GEOMETRY));

            for (size_t i = 0; i < totalVertices; ++i)
            {
                size_t srcOffset = i * srcFloatsPerVertex;
                size_t dstOffset = i * dstFloatsPerVertex;

                vertexData[dstOffset + 0] = combinedVertices[srcOffset + 0];
                vertexData[dstOffset + 1] = combinedVertices[srcOffset + 1];
                vertexData[dstOffset + 2] = combinedVertices[srcOffset + 2];

                Ogre::Vector3 normal(combinedVertices[srcOffset + 3], combinedVertices[srcOffset + 4], combinedVertices[srcOffset + 5]);
                vertexData[dstOffset + 3] = normal.x;
                vertexData[dstOffset + 4] = normal.y;
                vertexData[dstOffset + 5] = normal.z;

                Ogre::Vector3 tangent;
                if (std::abs(normal.y) < 0.9f)
                {
                    tangent = Ogre::Vector3::UNIT_Y.crossProduct(normal);
                }
                else
                {
                    tangent = normal.crossProduct(Ogre::Vector3::UNIT_X);
                }
                tangent.normalise();

                vertexData[dstOffset + 6] = tangent.x;
                vertexData[dstOffset + 7] = tangent.y;
                vertexData[dstOffset + 8] = tangent.z;
                vertexData[dstOffset + 9] = 1.0f;

                vertexData[dstOffset + 10] = combinedVertices[srcOffset + 6];
                vertexData[dstOffset + 11] = combinedVertices[srcOffset + 7];
            }

            Ogre::VertexBufferPacked* vertexBuffer = vaoManager->createVertexBuffer(vertexElements, totalVertices, Ogre::BT_IMMUTABLE, vertexData, true);

            const size_t indexDataSize = combinedIndices.size() * sizeof(Ogre::uint32);
            Ogre::uint32* indexData = reinterpret_cast<Ogre::uint32*>(OGRE_MALLOC_SIMD(indexDataSize, Ogre::MEMCATEGORY_GEOMETRY));
            memcpy(indexData, combinedIndices.data(), indexDataSize);

            Ogre::IndexBufferPacked* indexBuffer = vaoManager->createIndexBuffer(Ogre::IndexBufferPacked::IT_32BIT, combinedIndices.size(), Ogre::BT_IMMUTABLE, indexData, true);

            Ogre::VertexBufferPackedVec vertexBuffers;
            vertexBuffers.push_back(vertexBuffer);

            Ogre::VertexArrayObject* vao = vaoManager->createVertexArrayObject(vertexBuffers, indexBuffer, Ogre::OT_TRIANGLE_LIST);

            subMesh->mVao[Ogre::VpNormal].push_back(vao);
            subMesh->mVao[Ogre::VpShadow].push_back(vao);

            Ogre::Vector3 minBounds(std::numeric_limits<float>::max());
            Ogre::Vector3 maxBounds(std::numeric_limits<float>::lowest());

            for (size_t i = 0; i < totalVertices; ++i)
            {
                size_t offset = i * srcFloatsPerVertex;
                Ogre::Vector3 pos(combinedVertices[offset + 0], combinedVertices[offset + 1], combinedVertices[offset + 2]);
                minBounds.makeFloor(pos);
                maxBounds.makeCeil(pos);
            }

            Ogre::Aabb bounds;
            bounds.setExtents(minBounds, maxBounds);
            this->previewMesh->_setBounds(bounds, false);
            this->previewMesh->_setBoundingSphereRadius(bounds.getRadius());

            this->previewItem = this->gameObjectPtr->getSceneManager()->createItem(this->previewMesh, Ogre::SCENE_DYNAMIC);

            this->previewNode->setPosition(this->platformFrame * previewPosition);
            this->previewNode->setOrientation(this->platformFrame);
            this->previewNode->attachObject(this->previewItem);

            Ogre::String dbName = this->surfaceDatablock->getString();
            if (false == dbName.empty())
            {
                Ogre::HlmsDatablock* db = Ogre::Root::getSingletonPtr()->getHlmsManager()->getDatablockNoDefault(dbName);
                if (nullptr != db)
                {
                    this->previewItem->getSubItem(0)->setDatablock(db);
                }
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralPlatformComponent::updatePreviewMesh");
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Attribute Access
    ///////////////////////////////////////////////////////////////////////////////////////////////

    void ProceduralPlatformComponent::setActivated(bool activated)
    {
        this->activated->setValue(activated);
        this->updateModificationState();
    }

    bool ProceduralPlatformComponent::isActivated(void) const
    {
        return this->activated->getBool();
    }

    void ProceduralPlatformComponent::setPlatformDepth(Ogre::Real depth)
    {
        this->platformDepth->setValue(std::max(0.05f, depth));
        this->snapRadius = this->platformDepth->getReal() * 0.4f;
        if (false == this->platformSegments.empty())
        {
            this->rebuildMesh();
        }
    }

    Ogre::Real ProceduralPlatformComponent::getPlatformDepth(void) const
    {
        return this->platformDepth->getReal();
    }

    void ProceduralPlatformComponent::setPlatformHeight(Ogre::Real height)
    {
        this->platformHeight->setValue(std::max(0.05f, height));
        if (false == this->platformSegments.empty())
        {
            this->rebuildMesh();
        }
    }

    Ogre::Real ProceduralPlatformComponent::getPlatformHeight(void) const
    {
        return this->platformHeight->getReal();
    }

    void ProceduralPlatformComponent::setPlatformStyle(const Ogre::String& style)
    {
        this->platformStyle->setListSelectedValue(style);
        if (false == this->platformSegments.empty())
        {
            this->rebuildMesh();
        }
    }

    Ogre::String ProceduralPlatformComponent::getPlatformStyle(void) const
    {
        return this->platformStyle->getListSelectedValue();
    }

    void ProceduralPlatformComponent::setSnapToGrid(bool snap)
    {
        this->snapToGrid->setValue(snap);
    }

    bool ProceduralPlatformComponent::getSnapToGrid(void) const
    {
        return this->snapToGrid->getBool();
    }

    void ProceduralPlatformComponent::setGridSize(Ogre::Real size)
    {
        this->gridSize->setValue(std::max(0.01f, size));
    }

    Ogre::Real ProceduralPlatformComponent::getGridSize(void) const
    {
        return this->gridSize->getReal();
    }

    void ProceduralPlatformComponent::setSmoothingFactor(Ogre::Real factor)
    {
        this->smoothingFactor->setValue(Ogre::Math::Clamp(factor, 0.0f, 1.0f));
        if (false == this->platformSegments.empty())
        {
            this->rebuildMesh();
        }
    }

    Ogre::Real ProceduralPlatformComponent::getSmoothingFactor(void) const
    {
        return this->smoothingFactor->getReal();
    }

    void ProceduralPlatformComponent::setCurveSubdivisions(int subdivisions)
    {
        this->curveSubdivisions->setValue(std::max(1, subdivisions));
        if (false == this->platformSegments.empty())
        {
            this->rebuildMesh();
        }
    }

    int ProceduralPlatformComponent::getCurveSubdivisions(void) const
    {
        return this->curveSubdivisions->getInt();
    }

    void ProceduralPlatformComponent::setSurfaceDatablock(const Ogre::String& datablock)
    {
        this->surfaceDatablock->setValue(datablock);

        if (nullptr != this->platformItem)
        {
            GraphicsModule::RenderCommand renderCommand = [this, datablock]()
            {
                if (false == datablock.empty() && this->platformItem->getNumSubItems() >= 1)
                {
                    Ogre::HlmsDatablock* db = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(datablock);
                    if (nullptr != db)
                    {
                        this->platformItem->getSubItem(0)->setDatablock(db);
                    }
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "ProceduralPlatformComponent::setSurfaceDatablock");
        }
    }

    Ogre::String ProceduralPlatformComponent::getSurfaceDatablock(void) const
    {
        return this->surfaceDatablock->getString();
    }

    void ProceduralPlatformComponent::setGroundDatablock(const Ogre::String& datablock)
    {
        this->groundDatablock->setValue(datablock);

        // Submesh 1 (ground) AND submesh 2 (junction fill) both mirror this - there is no
        // separate Junction Datablock property, junction geometry always reuses Ground.
        if (nullptr != this->platformItem && this->platformItem->getNumSubItems() >= 2)
        {
            const Ogre::String dbToUse = datablock.empty() ? this->surfaceDatablock->getString() : datablock;
            const bool hasJunctionSubmesh = this->platformItem->getNumSubItems() >= 3;
            GraphicsModule::RenderCommand renderCommand = [this, dbToUse, hasJunctionSubmesh]()
            {
                if (false == dbToUse.empty())
                {
                    Ogre::HlmsDatablock* db = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(dbToUse);
                    if (nullptr != db)
                    {
                        this->platformItem->getSubItem(1)->setDatablock(db);
                        if (true == hasJunctionSubmesh)
                        {
                            this->platformItem->getSubItem(2)->setDatablock(db);
                        }
                    }
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "ProceduralPlatformComponent::setGroundDatablock");
        }
    }

    Ogre::String ProceduralPlatformComponent::getGroundDatablock(void) const
    {
        return this->groundDatablock->getString();
    }

    void ProceduralPlatformComponent::setSurfaceUVTiling(const Ogre::Vector2& tiling)
    {
        this->surfaceUVTiling->setValue(tiling);
        if (false == this->platformSegments.empty())
        {
            this->rebuildMesh();
        }
    }

    Ogre::Vector2 ProceduralPlatformComponent::getSurfaceUVTiling(void) const
    {
        return this->surfaceUVTiling->getVector2();
    }

    void ProceduralPlatformComponent::setGroundUVTiling(const Ogre::Vector2& tiling)
    {
        this->groundUVTiling->setValue(tiling);
        if (false == this->platformSegments.empty())
        {
            this->rebuildMesh();
        }
    }

    Ogre::Vector2 ProceduralPlatformComponent::getGroundUVTiling(void) const
    {
        return this->groundUVTiling->getVector2();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Convert To Mesh / Export
    ///////////////////////////////////////////////////////////////////////////////////////////////

    bool ProceduralPlatformComponent::convertToMeshApply(void)
    {
        if (this->platformSegments.empty())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPlatformComponent] Cannot convert to mesh: No platform segments exist! Create a platform first before converting.");
            return false;
        }

        if (nullptr == this->platformMesh || nullptr == this->platformItem)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPlatformComponent] Cannot convert to mesh: Platform mesh not generated! The platform may be in an invalid state.");
            return false;
        }

        if (this->platformMesh->getNumSubMeshes() == 0)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPlatformComponent] Cannot convert to mesh: Mesh has no submeshes!");
            return false;
        }

        Ogre::String meshName = "Platform_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()) + ".mesh";

        if (!Ogre::StringUtil::endsWith(meshName, ".mesh", true))
        {
            meshName += ".mesh";
        }

        auto filePathNames = Core::getSingletonPtr()->getSectionPath("Procedural");

        if (true == filePathNames.empty())
        {
            return false;
        }
        Ogre::String fullPath = filePathNames[0] + "/" + meshName;

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPlatformComponent] Converting procedural platform to static mesh: " + meshName);

        if (!this->exportMesh(fullPath))
        {
            return false;
        }

        Ogre::String capturedMeshName = meshName;
        GameObjectPtr capturedGameObjectPtr = this->gameObjectPtr;
        unsigned int capturedComponentIndex = this->getIndex();

        Ogre::String surfaceDbName = this->surfaceDatablock->getString();
        Ogre::String groundDbName = this->groundDatablock->getString();

        Ogre::Vector3 currentPosition = this->gameObjectPtr->getPosition();
        Ogre::Quaternion currentOrientation = this->gameObjectPtr->getOrientation();
        Ogre::Vector3 currentScale = this->gameObjectPtr->getScale();

        NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.5f));

        auto conversionFunction = [this, capturedMeshName, capturedGameObjectPtr, capturedComponentIndex, surfaceDbName, groundDbName, currentPosition, currentOrientation, currentScale]()
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPlatformComponent] Loading converted mesh: " + capturedMeshName);

            Ogre::MeshPtr loadedMesh;
            try
            {
                loadedMesh = Ogre::MeshManager::getSingleton().load(capturedMeshName, "Procedural");
            }
            catch (Ogre::Exception& e)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPlatformComponent] Failed to load exported mesh: " + e.getFullDescription());
                return;
            }

            if (loadedMesh.isNull())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPlatformComponent] Loaded mesh is null!");
                return;
            }

            Ogre::Item* newItem = nullptr;

            NOWA::GraphicsModule::RenderCommand renderCommand = [this, capturedGameObjectPtr, loadedMesh, surfaceDbName, groundDbName, &newItem]()
            {
                newItem = capturedGameObjectPtr->getSceneManager()->createItem(loadedMesh, capturedGameObjectPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC);

                if (newItem->getNumSubItems() >= 1 && !surfaceDbName.empty())
                {
                    Ogre::HlmsDatablock* surfaceDb = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(surfaceDbName);
                    if (nullptr != surfaceDb)
                    {
                        newItem->getSubItem(0)->setDatablock(surfaceDb);
                    }
                }

                // Submesh 1 (ground) and submesh 2 (junction fill, if present) both mirror
                // the same datablock - there is no separate junction property.
                Ogre::String groundOrFallback = groundDbName.empty() ? surfaceDbName : groundDbName;
                if (!groundOrFallback.empty())
                {
                    Ogre::HlmsDatablock* groundDb = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(groundOrFallback);
                    if (nullptr != groundDb)
                    {
                        if (newItem->getNumSubItems() >= 2)
                        {
                            newItem->getSubItem(1)->setDatablock(groundDb);
                        }
                        if (newItem->getNumSubItems() >= 3)
                        {
                            newItem->getSubItem(2)->setDatablock(groundDb);
                        }
                    }
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralPlatformComponent::convertToMesh_createItem");

            if (nullptr == newItem)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPlatformComponent] Failed to create Item from mesh!");
                return;
            }

            this->destroyPreviewMesh();
            this->destroyPlatformMesh();

            if (false == capturedGameObjectPtr->assignMesh(newItem))
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPlatformComponent] Failed to assign mesh to GameObject!");
                return;
            }

            capturedGameObjectPtr->getSceneNode()->setPosition(currentPosition);
            capturedGameObjectPtr->getSceneNode()->setOrientation(currentOrientation);
            capturedGameObjectPtr->getSceneNode()->setScale(currentScale);

            boost::shared_ptr<EventDataDeleteComponent> eventDataDeleteComponent(new EventDataDeleteComponent(capturedGameObjectPtr->getId(), "ProceduralPlatformComponent", capturedComponentIndex));
            NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataDeleteComponent);

            capturedGameObjectPtr->deleteComponentByIndex(capturedComponentIndex);

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPlatformComponent] Conversion complete: static mesh file " + capturedMeshName + ", SAVE YOUR SCENE to persist this change!");
        };

        NOWA::ProcessPtr closureProcess(new NOWA::ClosureProcess(conversionFunction));
        delayProcess->attachChild(closureProcess);
        NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);

        boost::shared_ptr<EventDataRefreshGui> eventDataRefreshGui(new EventDataRefreshGui());
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshGui);

        return true;
    }

    bool ProceduralPlatformComponent::exportMesh(const Ogre::String& filename)
    {
        if (nullptr == this->platformMesh)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPlatformComponent] No mesh to export!");
            return false;
        }

        try
        {
            Ogre::MeshSerializer serializer(Ogre::Root::getSingletonPtr()->getRenderSystem()->getVaoManager());
            serializer.exportMesh(this->platformMesh.get(), filename);

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPlatformComponent] Exported mesh to: " + filename);
            return true;
        }
        catch (Ogre::Exception& e)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPlatformComponent] Export failed: " + e.getFullDescription());
            return false;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Save/Load
    //
    // Binary layout is a direct port of ProceduralRoadComponent's .roaddata format: same
    // header shape (magic, version, origin xyz, counts, posSet byte), same per-segment
    // layout (isCurved, curvature, numCPs, then 5 floats per control point = 20 bytes -
    // position.xyz + rawHeight + smoothedHeight, mirroring position.xyz + groundHeight +
    // smoothedHeight), same trailing surface/ground/junction vertex+index blocks. Only the
    // magic constant, file suffix, and field names differ.
    ///////////////////////////////////////////////////////////////////////////////////////////////

    Ogre::String ProceduralPlatformComponent::getPlatformDataFilePath(void) const
    {
        Ogre::String projectFilePath;

        if (false == this->gameObjectPtr->getGlobal())
        {
            projectFilePath = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName();
        }
        else
        {
            projectFilePath = Core::getSingletonPtr()->getCurrentProjectPath();
        }

        Ogre::String filename = "Platform_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()) + ".platformdata";

        return projectFilePath + "/" + filename;
    }

    bool ProceduralPlatformComponent::savePlatformDataToFile(void)
    {
        if (this->platformSegments.empty() && this->cachedSurfaceVertices.empty())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPlatformComponent] savePlatformDataToFile: nothing to save, deleting file");
            this->deletePlatformDataFile();
            return true;
        }

        Ogre::String filePath = this->getPlatformDataFilePath();

        try
        {
            uint32_t numSegments = static_cast<uint32_t>(this->platformSegments.size());
            uint32_t numSurfaceVerts = static_cast<uint32_t>(this->cachedNumSurfaceVertices);
            uint32_t numSurfaceIdx = static_cast<uint32_t>(this->cachedSurfaceIndices.size());
            uint32_t numGroundVerts = static_cast<uint32_t>(this->cachedNumGroundVertices);
            uint32_t numGroundIdx = static_cast<uint32_t>(this->cachedGroundIndices.size());
            uint32_t numJunctionVerts = static_cast<uint32_t>(this->cachedNumJunctionVertices);
            uint32_t numJunctionIdx = static_cast<uint32_t>(this->cachedJunctionIndices.size());

            const size_t floatsPerVertex = 8;

            size_t segmentDataSize = 0;
            for (const auto& seg : this->platformSegments)
            {
                segmentDataSize += 1;
                segmentDataSize += 4;
                segmentDataSize += 4;
                segmentDataSize += seg.controlPoints.size() * 20;
            }

            size_t headerSize = 49;
            size_t surfaceVertBytes = numSurfaceVerts * floatsPerVertex * sizeof(float);
            size_t surfaceIdxBytes = numSurfaceIdx * sizeof(uint32_t);
            size_t groundVertBytes = numGroundVerts * floatsPerVertex * sizeof(float);
            size_t groundIdxBytes = numGroundIdx * sizeof(uint32_t);
            size_t junctionVertBytes = numJunctionVerts * floatsPerVertex * sizeof(float);
            size_t junctionIdxBytes = numJunctionIdx * sizeof(uint32_t);

            size_t totalSize = headerSize + segmentDataSize + surfaceVertBytes + surfaceIdxBytes + groundVertBytes + groundIdxBytes + junctionVertBytes + junctionIdxBytes;

            std::vector<unsigned char> buffer(totalSize);
            size_t off = 0;

            uint32_t magic = PLATFORMDATA_MAGIC;
            uint32_t version = PLATFORMDATA_VERSION;
            memcpy(&buffer[off], &magic, 4);
            off += 4;
            memcpy(&buffer[off], &version, 4);
            off += 4;
            memcpy(&buffer[off], &this->cachedPlatformOrigin.x, 4);
            off += 4;
            memcpy(&buffer[off], &this->cachedPlatformOrigin.y, 4);
            off += 4;
            memcpy(&buffer[off], &this->cachedPlatformOrigin.z, 4);
            off += 4;
            memcpy(&buffer[off], &numSegments, 4);
            off += 4;
            memcpy(&buffer[off], &numSurfaceVerts, 4);
            off += 4;
            memcpy(&buffer[off], &numSurfaceIdx, 4);
            off += 4;
            memcpy(&buffer[off], &numGroundVerts, 4);
            off += 4;
            memcpy(&buffer[off], &numGroundIdx, 4);
            off += 4;
            memcpy(&buffer[off], &numJunctionVerts, 4);
            off += 4;
            memcpy(&buffer[off], &numJunctionIdx, 4);
            off += 4;
            uint8_t posSet = this->originPositionSet ? 1 : 0;
            buffer[off++] = posSet;

            for (const auto& seg : this->platformSegments)
            {
                uint8_t curved = seg.isCurved ? 1 : 0;
                buffer[off++] = curved;

                memcpy(&buffer[off], &seg.curvature, 4);
                off += 4;

                uint32_t numCPs = static_cast<uint32_t>(seg.controlPoints.size());
                memcpy(&buffer[off], &numCPs, 4);
                off += 4;

                for (const auto& cp : seg.controlPoints)
                {
                    memcpy(&buffer[off], &cp.position.x, 4);
                    off += 4;
                    memcpy(&buffer[off], &cp.position.y, 4);
                    off += 4;
                    memcpy(&buffer[off], &cp.position.z, 4);
                    off += 4;
                    memcpy(&buffer[off], &cp.rawHeight, 4);
                    off += 4;
                    memcpy(&buffer[off], &cp.smoothedHeight, 4);
                    off += 4;
                }
            }

            if (surfaceVertBytes > 0)
            {
                memcpy(&buffer[off], this->cachedSurfaceVertices.data(), surfaceVertBytes);
            }
            off += surfaceVertBytes;

            if (surfaceIdxBytes > 0)
            {
                memcpy(&buffer[off], this->cachedSurfaceIndices.data(), surfaceIdxBytes);
            }
            off += surfaceIdxBytes;

            if (groundVertBytes > 0)
            {
                memcpy(&buffer[off], this->cachedGroundVertices.data(), groundVertBytes);
            }
            off += groundVertBytes;

            if (groundIdxBytes > 0)
            {
                memcpy(&buffer[off], this->cachedGroundIndices.data(), groundIdxBytes);
            }
            off += groundIdxBytes;

            if (junctionVertBytes > 0)
            {
                memcpy(&buffer[off], this->cachedJunctionVertices.data(), junctionVertBytes);
            }
            off += junctionVertBytes;

            if (junctionIdxBytes > 0)
            {
                memcpy(&buffer[off], this->cachedJunctionIndices.data(), junctionIdxBytes);
            }
            off += junctionIdxBytes;

            std::ofstream outFile(filePath.c_str(), std::ios::binary);
            if (false == outFile.is_open())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPlatformComponent] Cannot open for writing: " + filePath);
                return false;
            }

            outFile.write(reinterpret_cast<const char*>(buffer.data()), totalSize);
            outFile.close();

            if (true == outFile.fail())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPlatformComponent] Write failed: " + filePath);
                return false;
            }

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPlatformComponent] Saved to: " + filePath + " (" + Ogre::StringConverter::toString(totalSize) + " bytes)");
            return true;
        }
        catch (const std::exception& e)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPlatformComponent] Exception saving: " + Ogre::String(e.what()));
            return false;
        }
    }

    bool ProceduralPlatformComponent::loadPlatformDataFromFile(void)
    {
        Ogre::String filePath = this->getPlatformDataFilePath();

        std::ifstream inFile(filePath.c_str(), std::ios::binary);
        if (false == inFile.is_open())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPlatformComponent] No platform data file (new platform): " + filePath);
            return true;
        }

        try
        {
            inFile.seekg(0, std::ios::end);
            size_t fileSize = static_cast<size_t>(inFile.tellg());
            inFile.seekg(0, std::ios::beg);

            if (fileSize < 49)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPlatformComponent] File too small: " + filePath);
                inFile.close();
                return false;
            }

            std::vector<unsigned char> buffer(fileSize);
            inFile.read(reinterpret_cast<char*>(buffer.data()), fileSize);
            inFile.close();

            if (true == inFile.fail())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPlatformComponent] Read failed: " + filePath);
                return false;
            }

            size_t off = 0;

            uint32_t magic, version;
            memcpy(&magic, &buffer[off], 4);
            off += 4;
            memcpy(&version, &buffer[off], 4);
            off += 4;

            if (magic != PLATFORMDATA_MAGIC)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPlatformComponent] Bad magic in: " + filePath);
                return false;
            }
            if (version != PLATFORMDATA_VERSION)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                    "[ProceduralPlatformComponent] Unsupported version " + Ogre::StringConverter::toString(version) + " in: " + filePath + " (expected " + Ogre::StringConverter::toString(PLATFORMDATA_VERSION) + ")");
                return false;
            }

            Ogre::Vector3 origin;
            memcpy(&origin.x, &buffer[off], 4);
            off += 4;
            memcpy(&origin.y, &buffer[off], 4);
            off += 4;
            memcpy(&origin.z, &buffer[off], 4);
            off += 4;

            uint32_t numSegments, numSurfaceVerts, numSurfaceIdx, numGroundVerts, numGroundIdx;
            memcpy(&numSegments, &buffer[off], 4);
            off += 4;
            memcpy(&numSurfaceVerts, &buffer[off], 4);
            off += 4;
            memcpy(&numSurfaceIdx, &buffer[off], 4);
            off += 4;
            memcpy(&numGroundVerts, &buffer[off], 4);
            off += 4;
            memcpy(&numGroundIdx, &buffer[off], 4);
            off += 4;
            uint32_t numJunctionVerts, numJunctionIdx;
            memcpy(&numJunctionVerts, &buffer[off], 4);
            off += 4;
            memcpy(&numJunctionIdx, &buffer[off], 4);
            off += 4;

            uint8_t posSet = buffer[off++];
            this->originPositionSet = (posSet != 0);

            this->platformSegments.clear();
            for (uint32_t i = 0; i < numSegments; ++i)
            {
                if (off >= fileSize)
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPlatformComponent] Unexpected end of file reading segment " + Ogre::StringConverter::toString(i));
                    return false;
                }

                PlatformSegment seg;
                seg.isCurved = (buffer[off++] != 0);

                memcpy(&seg.curvature, &buffer[off], 4);
                off += 4;

                uint32_t numCPs;
                memcpy(&numCPs, &buffer[off], 4);
                off += 4;

                for (uint32_t j = 0; j < numCPs; ++j)
                {
                    PlatformControlPoint cp;
                    memcpy(&cp.position.x, &buffer[off], 4);
                    off += 4;
                    memcpy(&cp.position.y, &buffer[off], 4);
                    off += 4;
                    memcpy(&cp.position.z, &buffer[off], 4);
                    off += 4;
                    memcpy(&cp.rawHeight, &buffer[off], 4);
                    off += 4;
                    memcpy(&cp.smoothedHeight, &buffer[off], 4);
                    off += 4;

                    cp.distFromStart = 0.0f;
                    seg.controlPoints.push_back(cp);
                }

                this->platformSegments.push_back(seg);
            }

            this->cachedPlatformOrigin = origin;
            this->platformOrigin = origin;
            this->hasPlatformOrigin = true;

            const size_t floatsPerVertex = 8;
            size_t surfaceVertBytes = numSurfaceVerts * floatsPerVertex * sizeof(float);
            size_t surfaceIdxBytes = numSurfaceIdx * sizeof(uint32_t);
            size_t groundVertBytes = numGroundVerts * floatsPerVertex * sizeof(float);
            size_t groundIdxBytes = numGroundIdx * sizeof(uint32_t);
            size_t junctionVertBytes = numJunctionVerts * floatsPerVertex * sizeof(float);
            size_t junctionIdxBytes = numJunctionIdx * sizeof(uint32_t);

            size_t expectedRemaining = surfaceVertBytes + surfaceIdxBytes + groundVertBytes + groundIdxBytes + junctionVertBytes + junctionIdxBytes;
            if (off + expectedRemaining > fileSize)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                    "[ProceduralPlatformComponent] File size mismatch: need " + Ogre::StringConverter::toString(off + expectedRemaining) + " bytes, file has " + Ogre::StringConverter::toString(fileSize));
                return false;
            }

            this->cachedSurfaceVertices.resize(numSurfaceVerts * floatsPerVertex);
            this->cachedSurfaceIndices.resize(numSurfaceIdx);
            this->cachedGroundVertices.resize(numGroundVerts * floatsPerVertex);
            this->cachedGroundIndices.resize(numGroundIdx);
            this->cachedJunctionVertices.resize(numJunctionVerts * floatsPerVertex);
            this->cachedJunctionIndices.resize(numJunctionIdx);

            if (surfaceVertBytes > 0)
            {
                memcpy(this->cachedSurfaceVertices.data(), &buffer[off], surfaceVertBytes);
            }
            off += surfaceVertBytes;

            if (surfaceIdxBytes > 0)
            {
                memcpy(this->cachedSurfaceIndices.data(), &buffer[off], surfaceIdxBytes);
            }
            off += surfaceIdxBytes;

            if (groundVertBytes > 0)
            {
                memcpy(this->cachedGroundVertices.data(), &buffer[off], groundVertBytes);
            }
            off += groundVertBytes;

            if (groundIdxBytes > 0)
            {
                memcpy(this->cachedGroundIndices.data(), &buffer[off], groundIdxBytes);
            }
            off += groundIdxBytes;

            if (junctionVertBytes > 0)
            {
                memcpy(this->cachedJunctionVertices.data(), &buffer[off], junctionVertBytes);
            }
            off += junctionVertBytes;

            if (junctionIdxBytes > 0)
            {
                memcpy(this->cachedJunctionIndices.data(), &buffer[off], junctionIdxBytes);
            }
            off += junctionIdxBytes;

            this->cachedNumSurfaceVertices = numSurfaceVerts;
            this->cachedNumGroundVertices = numGroundVerts;
            this->cachedNumJunctionVertices = numJunctionVerts;

            std::vector<float> sv = this->cachedSurfaceVertices;
            std::vector<Ogre::uint32> si = this->cachedSurfaceIndices;
            std::vector<float> gv = this->cachedGroundVertices;
            std::vector<Ogre::uint32> gi = this->cachedGroundIndices;
            std::vector<float> jv = this->cachedJunctionVertices;
            std::vector<Ogre::uint32> ji = this->cachedJunctionIndices;
            size_t nsv = this->cachedNumSurfaceVertices;
            size_t ngv = this->cachedNumGroundVertices;
            size_t njv = this->cachedNumJunctionVertices;

            GraphicsModule::RenderCommand renderCommand = [this, sv, si, nsv, gv, gi, ngv, jv, ji, njv, origin]()
            {
                this->createPlatformMeshInternal(sv, si, nsv, gv, gi, ngv, jv, ji, njv, origin);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralPlatformComponent::loadPlatformDataFromFile");

            this->updateContinuationPoint();

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPlatformComponent] Load complete: " + filePath);
            return true;
        }
        catch (const std::exception& e)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPlatformComponent] Exception loading: " + Ogre::String(e.what()));
            return false;
        }
    }

    std::vector<unsigned char> ProceduralPlatformComponent::getPlatformData(void) const
    {
        std::vector<unsigned char> result;

        if (this->platformSegments.empty() && this->cachedSurfaceVertices.empty())
        {
            return result;
        }

        uint32_t numSegments = static_cast<uint32_t>(this->platformSegments.size());
        uint32_t numSurfaceVerts = static_cast<uint32_t>(this->cachedNumSurfaceVertices);
        uint32_t numSurfaceIdx = static_cast<uint32_t>(this->cachedSurfaceIndices.size());
        uint32_t numGroundVerts = static_cast<uint32_t>(this->cachedNumGroundVertices);
        uint32_t numGroundIdx = static_cast<uint32_t>(this->cachedGroundIndices.size());
        uint32_t numJunctionVerts = static_cast<uint32_t>(this->cachedNumJunctionVertices);
        uint32_t numJunctionIdx = static_cast<uint32_t>(this->cachedJunctionIndices.size());

        const size_t floatsPerVertex = 8;

        size_t segmentDataSize = 0;
        for (const auto& seg : this->platformSegments)
        {
            segmentDataSize += 1;
            segmentDataSize += 4;
            segmentDataSize += 4;
            segmentDataSize += seg.controlPoints.size() * 20;
        }

        size_t headerSize = 49;
        size_t surfaceVertBytes = numSurfaceVerts * floatsPerVertex * sizeof(float);
        size_t surfaceIdxBytes = numSurfaceIdx * sizeof(uint32_t);
        size_t groundVertBytes = numGroundVerts * floatsPerVertex * sizeof(float);
        size_t groundIdxBytes = numGroundIdx * sizeof(uint32_t);
        size_t junctionVertBytes = numJunctionVerts * floatsPerVertex * sizeof(float);
        size_t junctionIdxBytes = numJunctionIdx * sizeof(uint32_t);

        size_t totalSize = headerSize + segmentDataSize + surfaceVertBytes + surfaceIdxBytes + groundVertBytes + groundIdxBytes + junctionVertBytes + junctionIdxBytes;

        result.resize(totalSize);
        size_t off = 0;

        uint32_t magic = PLATFORMDATA_MAGIC;
        uint32_t version = PLATFORMDATA_VERSION;
        memcpy(&result[off], &magic, 4);
        off += 4;
        memcpy(&result[off], &version, 4);
        off += 4;
        memcpy(&result[off], &this->cachedPlatformOrigin.x, 4);
        off += 4;
        memcpy(&result[off], &this->cachedPlatformOrigin.y, 4);
        off += 4;
        memcpy(&result[off], &this->cachedPlatformOrigin.z, 4);
        off += 4;
        memcpy(&result[off], &numSegments, 4);
        off += 4;
        memcpy(&result[off], &numSurfaceVerts, 4);
        off += 4;
        memcpy(&result[off], &numSurfaceIdx, 4);
        off += 4;
        memcpy(&result[off], &numGroundVerts, 4);
        off += 4;
        memcpy(&result[off], &numGroundIdx, 4);
        off += 4;
        memcpy(&result[off], &numJunctionVerts, 4);
        off += 4;
        memcpy(&result[off], &numJunctionIdx, 4);
        off += 4;

        uint8_t posSet = this->originPositionSet ? 1 : 0;
        result[off++] = posSet;

        for (const auto& seg : this->platformSegments)
        {
            uint8_t curved = seg.isCurved ? 1 : 0;
            result[off++] = curved;

            memcpy(&result[off], &seg.curvature, 4);
            off += 4;

            uint32_t numCPs = static_cast<uint32_t>(seg.controlPoints.size());
            memcpy(&result[off], &numCPs, 4);
            off += 4;

            for (const auto& cp : seg.controlPoints)
            {
                memcpy(&result[off], &cp.position.x, 4);
                off += 4;
                memcpy(&result[off], &cp.position.y, 4);
                off += 4;
                memcpy(&result[off], &cp.position.z, 4);
                off += 4;
                memcpy(&result[off], &cp.rawHeight, 4);
                off += 4;
                memcpy(&result[off], &cp.smoothedHeight, 4);
                off += 4;
            }
        }

        if (surfaceVertBytes > 0)
        {
            memcpy(&result[off], this->cachedSurfaceVertices.data(), surfaceVertBytes);
        }
        off += surfaceVertBytes;

        if (surfaceIdxBytes > 0)
        {
            memcpy(&result[off], this->cachedSurfaceIndices.data(), surfaceIdxBytes);
        }
        off += surfaceIdxBytes;

        if (groundVertBytes > 0)
        {
            memcpy(&result[off], this->cachedGroundVertices.data(), groundVertBytes);
        }
        off += groundVertBytes;

        if (groundIdxBytes > 0)
        {
            memcpy(&result[off], this->cachedGroundIndices.data(), groundIdxBytes);
        }
        off += groundIdxBytes;

        if (junctionVertBytes > 0)
        {
            memcpy(&result[off], this->cachedJunctionVertices.data(), junctionVertBytes);
        }
        off += junctionVertBytes;

        if (junctionIdxBytes > 0)
        {
            memcpy(&result[off], this->cachedJunctionIndices.data(), junctionIdxBytes);
        }
        off += junctionIdxBytes;

        return result;
    }

    void ProceduralPlatformComponent::setPlatformData(const std::vector<unsigned char>& data)
    {
        this->destroyPlatformMesh();

        if (data.empty())
        {
            this->platformSegments.clear();
            this->cachedSurfaceVertices.clear();
            this->cachedSurfaceIndices.clear();
            this->cachedGroundVertices.clear();
            this->cachedGroundIndices.clear();
            this->cachedJunctionVertices.clear();
            this->cachedJunctionIndices.clear();
            this->cachedNumSurfaceVertices = 0;
            this->cachedNumGroundVertices = 0;
            this->cachedNumJunctionVertices = 0;
            this->hasPlatformOrigin = false;
            this->updateContinuationPoint();
            return;
        }

        if (data.size() < 49)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPlatformComponent] setPlatformData: buffer too small");
            return;
        }

        size_t off = 0;

        uint32_t magic, version;
        memcpy(&magic, &data[off], 4);
        off += 4;
        memcpy(&version, &data[off], 4);
        off += 4;

        if (magic != PLATFORMDATA_MAGIC || version != PLATFORMDATA_VERSION)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPlatformComponent] setPlatformData: invalid magic/version");
            return;
        }

        Ogre::Vector3 origin;
        memcpy(&origin.x, &data[off], 4);
        off += 4;
        memcpy(&origin.y, &data[off], 4);
        off += 4;
        memcpy(&origin.z, &data[off], 4);
        off += 4;

        uint32_t numSegments, numSurfaceVerts, numSurfaceIdx, numGroundVerts, numGroundIdx;
        memcpy(&numSegments, &data[off], 4);
        off += 4;
        memcpy(&numSurfaceVerts, &data[off], 4);
        off += 4;
        memcpy(&numSurfaceIdx, &data[off], 4);
        off += 4;
        memcpy(&numGroundVerts, &data[off], 4);
        off += 4;
        memcpy(&numGroundIdx, &data[off], 4);
        off += 4;
        uint32_t numJunctionVerts, numJunctionIdx;
        memcpy(&numJunctionVerts, &data[off], 4);
        off += 4;
        memcpy(&numJunctionIdx, &data[off], 4);
        off += 4;

        uint8_t posSet = data[off++];
        this->originPositionSet = (posSet != 0);

        this->platformSegments.clear();
        for (uint32_t i = 0; i < numSegments; ++i)
        {
            if (off >= data.size())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPlatformComponent] setPlatformData: unexpected end of buffer at segment " + Ogre::StringConverter::toString(i));
                return;
            }

            PlatformSegment seg;
            seg.isCurved = (data[off++] != 0);

            memcpy(&seg.curvature, &data[off], 4);
            off += 4;

            uint32_t numCPs;
            memcpy(&numCPs, &data[off], 4);
            off += 4;

            for (uint32_t j = 0; j < numCPs; ++j)
            {
                PlatformControlPoint cp;
                memcpy(&cp.position.x, &data[off], 4);
                off += 4;
                memcpy(&cp.position.y, &data[off], 4);
                off += 4;
                memcpy(&cp.position.z, &data[off], 4);
                off += 4;
                memcpy(&cp.rawHeight, &data[off], 4);
                off += 4;
                memcpy(&cp.smoothedHeight, &data[off], 4);
                off += 4;
                cp.distFromStart = 0.0f;
                seg.controlPoints.push_back(cp);
            }

            this->platformSegments.push_back(seg);
        }

        this->cachedPlatformOrigin = origin;
        this->platformOrigin = origin;
        this->hasPlatformOrigin = (numSegments > 0);

        const size_t floatsPerVertex = 8;
        size_t surfaceVertBytes = numSurfaceVerts * floatsPerVertex * sizeof(float);
        size_t surfaceIdxBytes = numSurfaceIdx * sizeof(uint32_t);
        size_t groundVertBytes = numGroundVerts * floatsPerVertex * sizeof(float);
        size_t groundIdxBytes = numGroundIdx * sizeof(uint32_t);
        size_t junctionVertBytes = numJunctionVerts * floatsPerVertex * sizeof(float);
        size_t junctionIdxBytes = numJunctionIdx * sizeof(uint32_t);

        if (off + surfaceVertBytes + surfaceIdxBytes + groundVertBytes + groundIdxBytes + junctionVertBytes + junctionIdxBytes > data.size())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPlatformComponent] setPlatformData: buffer too small for vertex data");
            return;
        }

        this->cachedSurfaceVertices.resize(numSurfaceVerts * floatsPerVertex);
        this->cachedSurfaceIndices.resize(numSurfaceIdx);
        this->cachedGroundVertices.resize(numGroundVerts * floatsPerVertex);
        this->cachedGroundIndices.resize(numGroundIdx);
        this->cachedJunctionVertices.resize(numJunctionVerts * floatsPerVertex);
        this->cachedJunctionIndices.resize(numJunctionIdx);

        if (surfaceVertBytes > 0)
        {
            memcpy(this->cachedSurfaceVertices.data(), &data[off], surfaceVertBytes);
        }
        off += surfaceVertBytes;

        if (surfaceIdxBytes > 0)
        {
            memcpy(this->cachedSurfaceIndices.data(), &data[off], surfaceIdxBytes);
        }
        off += surfaceIdxBytes;

        if (groundVertBytes > 0)
        {
            memcpy(this->cachedGroundVertices.data(), &data[off], groundVertBytes);
        }
        off += groundVertBytes;

        if (groundIdxBytes > 0)
        {
            memcpy(this->cachedGroundIndices.data(), &data[off], groundIdxBytes);
        }
        off += groundIdxBytes;

        if (junctionVertBytes > 0)
        {
            memcpy(this->cachedJunctionVertices.data(), &data[off], junctionVertBytes);
        }
        off += junctionVertBytes;

        if (junctionIdxBytes > 0)
        {
            memcpy(this->cachedJunctionIndices.data(), &data[off], junctionIdxBytes);
        }
        off += junctionIdxBytes;

        this->cachedNumSurfaceVertices = numSurfaceVerts;
        this->cachedNumGroundVertices = numGroundVerts;
        this->cachedNumJunctionVertices = numJunctionVerts;

        if (numSurfaceVerts > 0 || numGroundVerts > 0 || numJunctionVerts > 0)
        {
            std::vector<float> sv = this->cachedSurfaceVertices;
            std::vector<Ogre::uint32> si = this->cachedSurfaceIndices;
            std::vector<float> gv = this->cachedGroundVertices;
            std::vector<Ogre::uint32> gi = this->cachedGroundIndices;
            std::vector<float> jv = this->cachedJunctionVertices;
            std::vector<Ogre::uint32> ji = this->cachedJunctionIndices;
            size_t nsv = this->cachedNumSurfaceVertices;
            size_t ngv = this->cachedNumGroundVertices;
            size_t njv = this->cachedNumJunctionVertices;

            GraphicsModule::RenderCommand renderCommand = [this, sv, si, nsv, gv, gi, ngv, jv, ji, njv, origin]()
            {
                this->createPlatformMeshInternal(sv, si, nsv, gv, gi, ngv, jv, ji, njv, origin);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralPlatformComponent::setPlatformData");
        }

        this->updateContinuationPoint();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPlatformComponent] setPlatformData: restored " + Ogre::StringConverter::toString(numSegments) + " segments, " + Ogre::StringConverter::toString(numSurfaceVerts) +
                                                                               " surface verts, " + Ogre::StringConverter::toString(numGroundVerts) + " ground verts, " + Ogre::StringConverter::toString(numJunctionVerts) + " junction verts");
    }

    void ProceduralPlatformComponent::deletePlatformDataFile(void)
    {
        std::filesystem::path relativePath(this->getPlatformDataFilePath());
        std::filesystem::path absolutePath = std::filesystem::absolute(relativePath);

        if (false == std::filesystem::exists(absolutePath))
        {
            return;
        }

        std::error_code ec;
        bool removed = std::filesystem::remove(absolutePath, ec);

        if (ec)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPlatformComponent] Delete failed: " + ec.message());
            return;
        }

        if (false == removed)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPlatformComponent] Remove returned false.");
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Segment Mode
    ///////////////////////////////////////////////////////////////////////////////////////////////

    void ProceduralPlatformComponent::setEditMode(const Ogre::String& editModeStr)
    {
        this->editMode->setListSelectedValue(editModeStr);
        this->selectedSegmentIndex = -1;

        if (this->getEditModeEnum() == EditMode::SEGMENT && this->buildState == BuildState::DRAGGING)
        {
            this->cancelPlatform();
        }

        if (this->getEditModeEnum() == EditMode::SEGMENT)
        {
            // One-shot resync against the GameObject's actual current transform, exactly
            // once per Segment-mode entry - mirrors ProceduralRoadComponent's identical
            // resync (covers the case where the user moved/rotated the GameObject node
            // externally after building the platform, e.g. via the normal move/rotate
            // gizmo). No ground-height cache to invalidate afterward - platform has no
            // terrain dependency at all (see the class doc comment).
            if (true == this->hasPlatformOrigin && nullptr != this->gameObjectPtr)
            {
                Ogre::SceneNode* node = this->gameObjectPtr->getSceneNode();
                if (nullptr != node)
                {
                    const Ogre::Vector3 liveWorldPos = node->_getDerivedPositionUpdated();
                    const Ogre::Quaternion liveOrientation = node->_getDerivedOrientationUpdated();

                    const Ogre::Vector3 expectedWorldPos = this->platformFrame * this->platformOrigin;
                    const bool positionChanged = false == MathHelper::getInstance()->vector3Equals(liveWorldPos, expectedWorldPos, 0.01f);
                    const bool orientationChanged = false == liveOrientation.equals(this->platformFrame, Ogre::Radian(0.001f));

                    if (true == positionChanged || true == orientationChanged)
                    {
                        const Ogre::Quaternion newFrame = liveOrientation;
                        const Ogre::Vector3 newPlatformOrigin = newFrame.Inverse() * liveWorldPos;
                        const Ogre::Vector3 delta = newPlatformOrigin - this->platformOrigin;

                        for (PlatformSegment& seg : this->platformSegments)
                        {
                            for (PlatformControlPoint& cp : seg.controlPoints)
                            {
                                cp.position.x += delta.x;
                                cp.rawHeight += delta.y;
                                cp.smoothedHeight += delta.y;
                            }
                        }
                        this->platformOrigin = newPlatformOrigin;
                        this->platformFrame = newFrame;
                        this->platformPlaneAnchor = liveWorldPos;

                        if (true == this->hasLoadedPlatformEndpoint)
                        {
                            this->loadedPlatformEndpoint.x += delta.x;
                            this->loadedPlatformEndpointHeight += delta.y;
                        }

                        this->rebuildMesh();
                    }
                }
            }

            boost::shared_ptr<EventDataEditorMode> eventDataEditorMode(new EventDataEditorMode(NOWA::EditorManager::EDITOR_MESH_MODIFY_MODE));
            NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataEditorMode);
        }
        else
        {
            boost::shared_ptr<EventDataEditorMode> eventDataEditorMode(new EventDataEditorMode(NOWA::EditorManager::EDITOR_SELECT_MODE));
            NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataEditorMode);
        }
    }

    ProceduralPlatformComponent::EditMode ProceduralPlatformComponent::getEditModeEnum(void) const
    {
        return (this->editMode->getListSelectedValue() == "Segment") ? EditMode::SEGMENT : EditMode::OBJECT;
    }

    void ProceduralPlatformComponent::deleteSelectedSegment(void)
    {
        if (this->selectedSegmentIndex < 0 || this->selectedSegmentIndex >= static_cast<int>(this->platformSegments.size()))
        {
            return;
        }

        std::vector<unsigned char> oldData = this->getPlatformData();

        this->platformSegments.erase(this->platformSegments.begin() + this->selectedSegmentIndex);
        this->selectedSegmentIndex = -1;

        if (this->platformSegments.empty())
        {
            this->destroyPlatformMesh();
            this->hasPlatformOrigin = false;
            this->hasLoadedPlatformEndpoint = false;
        }
        else
        {
            this->rebuildMesh();
            this->updateContinuationPoint();
        }

        this->scheduleSegmentOverlayUpdate();

        std::vector<unsigned char> newData = this->getPlatformData();

        boost::shared_ptr<EventDataCommandTransactionBegin> evtBegin(new EventDataCommandTransactionBegin("Delete Platform Segment"));
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evtBegin);

        boost::shared_ptr<EventDataPlatformModifyEnd> evtMod(new EventDataPlatformModifyEnd(oldData, newData, this->gameObjectPtr->getId()));
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evtMod);

        boost::shared_ptr<EventDataCommandTransactionEnd> evtEnd(new EventDataCommandTransactionEnd());
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evtEnd);
    }

    int ProceduralPlatformComponent::findNearestSegmentWithinRadius(const Ogre::Vector3& worldPos, Ogre::Real radius) const
    {
        if (this->platformSegments.empty())
        {
            return -1;
        }

        int bestSeg = -1;
        float bestDist = radius; // only accept hits within radius

        for (size_t si = 0; si < this->platformSegments.size(); ++si)
        {
            const PlatformSegment& seg = this->platformSegments[si];
            for (size_t pi = 1; pi < seg.controlPoints.size(); ++pi)
            {
                Ogre::Vector2 a(seg.controlPoints[pi - 1].position.x, seg.controlPoints[pi - 1].smoothedHeight);
                Ogre::Vector2 b(seg.controlPoints[pi].position.x, seg.controlPoints[pi].smoothedHeight);
                Ogre::Vector2 p(worldPos.x, worldPos.y);

                Ogre::Vector2 ab = b - a;
                float abLen2 = ab.dotProduct(ab);
                float t = (abLen2 > 1e-6f) ? Ogre::Math::Clamp((p - a).dotProduct(ab) / abLen2, 0.0f, 1.0f) : 0.0f;
                float dist = (a + ab * t - p).length();

                if (dist < bestDist)
                {
                    bestDist = dist;
                    bestSeg = static_cast<int>(si);
                }
            }
        }
        return bestSeg;
    }

    void ProceduralPlatformComponent::createSegmentOverlay(void)
    {
        NOWA::GraphicsModule::RenderCommand cmd = [this]()
        {
            this->segOverlayNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();

            this->segOverlayObject = this->gameObjectPtr->getSceneManager()->createManualObject();
            this->segOverlayObject->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);
            this->segOverlayObject->setName("PlatformSegOverlay_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()));
            this->segOverlayObject->setQueryFlags(0u);
            this->segOverlayObject->setCastShadows(false);
            this->segOverlayNode->attachObject(this->segOverlayObject);
            this->segOverlayNode->setVisible(false);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "ProceduralPlatformComponent::createSegmentOverlay");
    }

    void ProceduralPlatformComponent::destroySegmentOverlay(void)
    {
        NOWA::GraphicsModule::RenderCommand cmd = [this]()
        {
            if (!this->segOverlayNode)
            {
                return;
            }
            this->segOverlayNode->detachAllObjects();
            if (this->segOverlayObject)
            {
                this->gameObjectPtr->getSceneManager()->destroyManualObject(this->segOverlayObject);
                this->segOverlayObject = nullptr;
            }
            this->segOverlayNode->getParentSceneNode()->removeAndDestroyChild(this->segOverlayNode);
            this->segOverlayNode = nullptr;
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "ProceduralPlatformComponent::destroySegmentOverlay");
    }

    void ProceduralPlatformComponent::scheduleSegmentOverlayUpdate(void)
    {
        if (true == AppStateManager::getSingletonPtr()->getGameObjectController()->getIsDestroying())
        {
            return;
        }

        if (nullptr == this->segOverlayObject || nullptr == this->gameObjectPtr)
        {
            return;
        }

        const bool segmentMode = (this->getEditModeEnum() == EditMode::SEGMENT);

        if (false == segmentMode || true == this->platformSegments.empty())
        {
            NOWA::GraphicsModule::RenderCommand hideCmd = [this]()
            {
                if (this->segOverlayObject)
                {
                    this->segOverlayObject->clear();
                }
                if (this->segOverlayNode)
                {
                    this->segOverlayNode->setVisible(false);
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueue(std::move(hideCmd), "ProceduralPlatformComponent::segOverlay_hide");
            return;
        }

        // ── Build line geometry entirely on the main thread ─────────────────────
        // Heights come from stored smoothedHeight - exactly what the platform mesh was
        // built at, so the overlay always tracks the platform surface, no raycasting
        // needed. NOTE: unlike ProceduralRoadComponent's overlay, there are no left/right
        // "edge strip" lines - platform's path has no in-plane width to draw strips at (see
        // the class doc comment) - only the centerline and endpoint crosses are drawn.
        struct LV
        {
            Ogre::Vector3 pos;
            Ogre::ColourValue col;
        };
        std::vector<LV> lines;
        lines.reserve(this->platformSegments.size() * 16);

        const Ogre::Real pushZ = 0.05f; // slightly toward camera to avoid z-fighting with the platform surface
        const Ogre::Real crossR = std::max(0.15f, this->platformDepth->getReal() * 0.2f);

        const Ogre::ColourValue cGrey(0.35f, 0.35f, 0.35f, 1.0f);
        const Ogre::ColourValue cSelected(1.00f, 0.75f, 0.00f, 1.0f);
        const Ogre::ColourValue cEndpt(1.00f, 0.75f, 0.00f, 1.0f);

        auto addLine = [&](const Ogre::Vector3& a, const Ogre::Vector3& b, const Ogre::ColourValue& c)
        {
            lines.push_back({a, c});
            lines.push_back({b, c});
        };

        for (int si = 0; si < static_cast<int>(this->platformSegments.size()); ++si)
        {
            const PlatformSegment& seg = this->platformSegments[si];
            if (seg.controlPoints.size() < 2)
            {
                continue;
            }

            const PlatformControlPoint& cp0 = seg.controlPoints.front();
            const PlatformControlPoint& cp1 = seg.controlPoints.back();
            const bool selected = (si == this->selectedSegmentIndex);
            const Ogre::ColourValue& lineCol = selected ? cSelected : cGrey;

            const float segLen = std::abs(cp1.position.x - cp0.position.x);
            const int N = std::max(4, static_cast<int>(segLen / 2.0f) + 1);

            const float h0 = cp0.smoothedHeight;
            const float h1 = cp1.smoothedHeight;

            std::vector<Ogre::Vector3> path;
            path.reserve(N + 1);
            for (int k = 0; k <= N; ++k)
            {
                const float t = static_cast<float>(k) / static_cast<float>(N);
                // BUGFIX: cp0.position.z + pushZ, not pushZ alone - the overlay node carries
                // only orientation (no position offset, same as ProceduralRoadComponent's),
                // so these vertices must be full frame-local coordinates including the
                // fixed plane's real Z constant, or the line floats at the wrong depth.
                path.push_back(Ogre::Vector3(cp0.position.x + (cp1.position.x - cp0.position.x) * t, h0 + (h1 - h0) * t, cp0.position.z + pushZ));
            }

            for (int k = 0; k < static_cast<int>(path.size()) - 1; ++k)
            {
                addLine(path[k], path[k + 1], lineCol);
            }

            if (selected)
            {
                for (const Ogre::Vector3& ep : {path.front(), path.back()})
                {
                    addLine(ep + Ogre::Vector3(-crossR, 0, 0), ep + Ogre::Vector3(crossR, 0, 0), cEndpt);
                    addLine(ep + Ogre::Vector3(0, -crossR, 0), ep + Ogre::Vector3(0, crossR, 0), cEndpt);
                }
            }
        }

        NOWA::GraphicsModule::RenderCommand drawCmd = [this, lines = std::move(lines)]()
        {
            if (nullptr == this->segOverlayObject)
            {
                return;
            }
            this->segOverlayObject->clear();

            if (true == lines.empty())
            {
                if (this->segOverlayNode)
                {
                    this->segOverlayNode->setVisible(false);
                }
                return;
            }

            try
            {
                this->segOverlayObject->begin("WhiteNoLightingBackground", Ogre::OT_LINE_LIST);
                Ogre::uint32 idx = 0;
                for (const auto& v : lines)
                {
                    this->segOverlayObject->position(v.pos);
                    this->segOverlayObject->colour(v.col);
                    this->segOverlayObject->index(idx++);
                }
                this->segOverlayObject->end();
            }
            catch (Ogre::Exception& e)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPlatformComponent] Overlay begin() FAILED: " + e.getDescription());
            }

            if (this->segOverlayNode)
            {
                this->segOverlayNode->setOrientation(this->platformFrame);
                this->segOverlayNode->setVisible(true);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(drawCmd), "ProceduralPlatformComponent::segOverlay_draw");
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Snapping
    ///////////////////////////////////////////////////////////////////////////////////////////////

    bool ProceduralPlatformComponent::detectSnapToOwnPlatform(const Ogre::Vector3& worldPos, Ogre::Real radius)
    {
        if (this->platformSegments.empty())
        {
            return false;
        }

        float bestDist = radius;
        int bestSeg = -1;
        Ogre::Vector3 bestPt;

        for (int si = 0; si < static_cast<int>(this->platformSegments.size()); ++si)
        {
            const PlatformSegment& seg = this->platformSegments[si];
            if (seg.controlPoints.size() < 2)
            {
                continue;
            }

            {
                const Ogre::Vector2 ep(seg.controlPoints.front().position.x, seg.controlPoints.front().smoothedHeight);
                const Ogre::Vector2 p(worldPos.x, worldPos.y);
                float dist = ep.distance(p);
                if (dist < bestDist)
                {
                    bestDist = dist;
                    bestSeg = si;
                    // BUGFIX: keep the stored point's real z (the fixed plane's constant),
                    // not a hardcoded 0 - this feeds updatePlatformPreview/confirmPlatform,
                    // and ultimately the snap-indicator circle, which need the true depth.
                    bestPt = Ogre::Vector3(seg.controlPoints.front().position.x, seg.controlPoints.front().smoothedHeight, seg.controlPoints.front().position.z);
                }
            }

            {
                const Ogre::Vector2 ep(seg.controlPoints.back().position.x, seg.controlPoints.back().smoothedHeight);
                const Ogre::Vector2 p(worldPos.x, worldPos.y);
                float dist = ep.distance(p);
                if (dist < bestDist)
                {
                    bestDist = dist;
                    bestSeg = si;
                    bestPt = Ogre::Vector3(seg.controlPoints.back().position.x, seg.controlPoints.back().smoothedHeight, seg.controlPoints.back().position.z);
                }
            }
        }

        this->isSnapToOwnPlatform = (bestSeg >= 0);
        this->snapToPlatformSegmentIdx = bestSeg;
        this->snapToPlatformPoint = bestPt;
        return this->isSnapToOwnPlatform;
    }

    void ProceduralPlatformComponent::scheduleSnapIndicatorUpdate(void)
    {
        if (nullptr == this->segOverlayObject)
        {
            return;
        }

        if (!this->isSnapToOwnPlatform)
        {
            // Only clear if we're not already drawing the segment-mode overlay
            if (this->getEditModeEnum() != EditMode::SEGMENT)
            {
                NOWA::GraphicsModule::RenderCommand cmd = [this]()
                {
                    if (this->segOverlayObject)
                    {
                        this->segOverlayObject->clear();
                    }
                    if (this->segOverlayNode)
                    {
                        this->segOverlayNode->setVisible(false);
                    }
                };
                NOWA::GraphicsModule::getInstance()->enqueue(std::move(cmd), "snapIndicator_hide");
            }
            return;
        }

        // Circle drawn in the (x, height) plane at a small Z push toward the camera -
        // unlike ProceduralRoadComponent's ground-plane circle (pushed up in Y), platform's
        // "plane" already IS x/height, so the push is along Z instead.
        const Ogre::Vector3 centre = this->snapToPlatformPoint;
        const Ogre::Real r = this->platformDepth->getReal() * 0.6f;
        const int segs = 16;
        const Ogre::Real pushZ = 0.1f;

        struct LV
        {
            Ogre::Vector3 pos;
            Ogre::ColourValue col;
        };
        std::vector<LV> lines;
        lines.reserve(segs * 2);

        const Ogre::ColourValue snapCol(0.0f, 1.0f, 0.5f, 1.0f);

        for (int k = 0; k < segs; ++k)
        {
            const float a0 = Ogre::Math::TWO_PI * k / segs;
            const float a1 = Ogre::Math::TWO_PI * (k + 1) / segs;
            // BUGFIX: centre.z + pushZ, not pushZ alone - centre.z is now correct since
            // detectSnapToOwnPlatform preserves the real z (see its own bugfix comment).
            lines.push_back({Ogre::Vector3(centre.x + r * std::cos(a0), centre.y + r * std::sin(a0), centre.z + pushZ), snapCol});
            lines.push_back({Ogre::Vector3(centre.x + r * std::cos(a1), centre.y + r * std::sin(a1), centre.z + pushZ), snapCol});
        }

        NOWA::GraphicsModule::RenderCommand cmd = [this, lines = std::move(lines)]()
        {
            if (!this->segOverlayObject)
            {
                return;
            }
            this->segOverlayObject->clear();
            try
            {
                this->segOverlayObject->begin("WhiteNoLightingBackground", Ogre::OT_LINE_LIST);
                Ogre::uint32 idx = 0;
                for (const auto& v : lines)
                {
                    this->segOverlayObject->position(v.pos);
                    this->segOverlayObject->colour(v.col);
                    this->segOverlayObject->index(idx++);
                }
                this->segOverlayObject->end();
            }
            catch (Ogre::Exception&)
            {
            }
            if (this->segOverlayNode)
            {
                this->segOverlayNode->setOrientation(this->platformFrame);
                this->segOverlayNode->setVisible(true);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(cmd), "snapIndicator_draw");
    }

    void ProceduralPlatformComponent::splitSegmentAtPoint(int segIdx, float t, const Ogre::Vector3& splitWorldPos)
    {
        if (segIdx < 0 || segIdx >= static_cast<int>(this->platformSegments.size()))
        {
            return;
        }

        const PlatformSegment& original = this->platformSegments[segIdx];
        if (original.controlPoints.size() < 2)
        {
            return;
        }

        const Ogre::Vector3 localSplit = this->platformFrame.Inverse() * splitWorldPos;

        const size_t n = original.controlPoints.size();
        const float totalSegs = static_cast<float>(n - 1);
        const float scaledT = t * totalSegs;
        const size_t segPart = static_cast<size_t>(std::min(static_cast<float>(n - 2), std::floor(scaledT)));
        const float localT = scaledT - static_cast<float>(segPart);

        const PlatformControlPoint& cpA = original.controlPoints[segPart];
        const PlatformControlPoint& cpB = original.controlPoints[segPart + 1];

        PlatformControlPoint splitCP;
        // BUGFIX: keep localSplit.z (the fixed plane's real, generally-nonzero local Z),
        // not a hardcoded 0 - this control point is stored long-term in platformSegments,
        // unlike rebuildMesh's temporary local buffers where z is genuinely unused.
        splitCP.position = Ogre::Vector3(localSplit.x, 0.0f, localSplit.z);
        splitCP.rawHeight = cpA.rawHeight + (cpB.rawHeight - cpA.rawHeight) * localT;
        splitCP.smoothedHeight = cpA.smoothedHeight + (cpB.smoothedHeight - cpA.smoothedHeight) * localT;
        splitCP.distFromStart = 0.0f;

        PlatformSegment seg1;
        seg1.isCurved = original.isCurved;
        seg1.curvature = original.curvature;
        for (size_t i = 0; i <= segPart; ++i)
        {
            seg1.controlPoints.push_back(original.controlPoints[i]);
        }
        seg1.controlPoints.push_back(splitCP);

        PlatformSegment seg2;
        seg2.isCurved = original.isCurved;
        seg2.curvature = original.curvature;
        seg2.controlPoints.push_back(splitCP);
        for (size_t i = segPart + 1; i < n; ++i)
        {
            seg2.controlPoints.push_back(original.controlPoints[i]);
        }

        this->platformSegments.erase(this->platformSegments.begin() + segIdx);
        this->platformSegments.insert(this->platformSegments.begin() + segIdx, seg2);
        this->platformSegments.insert(this->platformSegments.begin() + segIdx, seg1);

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPlatformComponent] Split segment " + Ogre::StringConverter::toString(segIdx) + " at t=" + Ogre::StringConverter::toString(t));
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Cross-Network Merging
    ///////////////////////////////////////////////////////////////////////////////////////////////

    PlatformComponentBase* ProceduralPlatformComponent::findOtherPlatformNearby(const Ogre::Vector3& worldPos, Ogre::Real maxRadius, Ogre::Vector3& outSnapPoint) const
    {
        if (nullptr == this->otherPlatformQuery)
        {
            return nullptr;
        }

        // Search ALONG the depth axis (platformFrame * UNIT_Z), not vertically - unlike
        // ProceduralRoadComponent, whose other-road candidates are found via a vertical
        // probe (roads sit on/near the ground, so straight down always makes sense). A
        // platform's neighbors can be positioned anywhere in 3D, but they always share the
        // same (x, height) plane concept this component uses - probing along Z from far in
        // front finds whatever box occupies that (x, height) column at any depth.
        const Ogre::Vector3 probeAxis = this->platformFrame * Ogre::Vector3::UNIT_Z;
        const Ogre::Vector3 rayOrigin = worldPos + probeAxis * 1000.0f;
        Ogre::Ray ray(rayOrigin, -probeAxis);

        this->otherPlatformQuery->setRay(ray);
        this->otherPlatformQuery->setSortByDistance(true);
        Ogre::RaySceneQueryResult& result = this->otherPlatformQuery->execute();

        for (const auto& entry : result)
        {
            if (nullptr == entry.movable)
            {
                continue;
            }
            if (entry.movable == this->platformItem || entry.movable == this->previewItem)
            {
                continue;
            }

            const Ogre::Any& userAny = entry.movable->getUserObjectBindings().getUserAny();
            if (true == userAny.isEmpty())
            {
                continue;
            }

            GameObject* otherGameObject = nullptr;
            try
            {
                otherGameObject = Ogre::any_cast<GameObject*>(userAny);
            }
            catch (Ogre::Exception&)
            {
                continue;
            }

            if (nullptr == otherGameObject || otherGameObject->getId() == this->gameObjectPtr->getId())
            {
                continue;
            }

            const auto& otherPlatformCompPtr = NOWA::makeStrongPtr(otherGameObject->getComponent<PlatformComponentBase>());
            if (!otherPlatformCompPtr)
            {
                continue;
            }

            Ogre::Vector3 candidate;
            if (true == otherPlatformCompPtr->getNearestPointOnPlatform(worldPos, maxRadius, candidate))
            {
                outSnapPoint = candidate;
                return otherPlatformCompPtr.get();
            }
        }

        return nullptr;
    }

    void ProceduralPlatformComponent::mergeOtherPlatformIntoThis(ProceduralPlatformComponent* otherPlatform)
    {
        if (nullptr == otherPlatform || otherPlatform == this || nullptr == otherPlatform->gameObjectPtr)
        {
            return;
        }

        std::vector<std::pair<Ogre::Vector3, Ogre::Vector3>> worldPairs;
        for (const PlatformSegment& seg : otherPlatform->platformSegments)
        {
            for (size_t i = 0; i + 1 < seg.controlPoints.size(); ++i)
            {
                const PlatformControlPoint& cpA = seg.controlPoints[i];
                const PlatformControlPoint& cpB = seg.controlPoints[i + 1];

                // cpA/cpB.position are LOCAL to otherPlatform's OWN frame (height stored
                // separately in smoothedHeight, mirroring ProceduralRoadComponent's
                // position.y=0-placeholder convention) - reconstruct the full local point
                // and transform through otherPlatform's own platformFrame to get world
                // space, exactly like getPlatformConnectionPoint does for a single endpoint.
                // BUGFIX: keep cpA/cpB.position.z (otherPlatform's OWN plane constant) - it
                // can be entirely different from this component's own constant, since they
                // are different GameObjects/planes. Hardcoding 0 here silently teleported
                // the merged geometry to the wrong depth.
                const Ogre::Vector3 worldA = otherPlatform->platformFrame * Ogre::Vector3(cpA.position.x, cpA.smoothedHeight, cpA.position.z);
                const Ogre::Vector3 worldB = otherPlatform->platformFrame * Ogre::Vector3(cpB.position.x, cpB.smoothedHeight, cpB.position.z);
                worldPairs.push_back({worldA, worldB});
            }
        }

        if (true == worldPairs.empty())
        {
            return;
        }

        const unsigned long otherGameObjectId = otherPlatform->gameObjectPtr->getId();

        // Batch all transferred segments into ONE rebuildMesh() at the end, instead of one
        // rebuild per addPlatformSegment() call.
        this->beginBatch();
        for (const auto& pr : worldPairs)
        {
            this->addPlatformSegment(pr.first, pr.second);
        }
        this->endBatch();

        // otherPlatform and everything it owns is gone after this call - nothing above may
        // reference it again.
        AppStateManager::getSingletonPtr()->getGameObjectController()->deleteGameObject(otherGameObjectId);

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
            "[ProceduralPlatformComponent] Merged " + Ogre::StringConverter::toString(worldPairs.size()) + " segment(s) from GameObject id=" + Ogre::StringConverter::toString(otherGameObjectId) + " into this platform and removed it.");
    }

    bool ProceduralPlatformComponent::getNearestPointOnPlatform(const Ogre::Vector3& worldPos, Ogre::Real maxRadius, Ogre::Vector3& outPoint) const
    {
        if (true == this->platformSegments.empty())
        {
            return false;
        }

        const Ogre::Vector3 localPos = this->platformFrame.Inverse() * worldPos;
        const Ogre::Vector2 p(localPos.x, localPos.y);

        Ogre::Real bestDist = maxRadius;
        int bestSeg = -1;
        bool bestIsFront = true;

        for (int si = 0; si < static_cast<int>(this->platformSegments.size()); ++si)
        {
            const PlatformSegment& seg = this->platformSegments[si];
            if (seg.controlPoints.size() < 2)
            {
                continue;
            }

            {
                const Ogre::Vector2 ep(seg.controlPoints.front().position.x, seg.controlPoints.front().rawHeight);
                const Ogre::Real dist = ep.distance(p);
                if (dist < bestDist)
                {
                    bestDist = dist;
                    bestSeg = si;
                    bestIsFront = true;
                }
            }

            {
                const Ogre::Vector2 ep(seg.controlPoints.back().position.x, seg.controlPoints.back().rawHeight);
                const Ogre::Real dist = ep.distance(p);
                if (dist < bestDist)
                {
                    bestDist = dist;
                    bestSeg = si;
                    bestIsFront = false;
                }
            }
        }

        if (bestSeg < 0)
        {
            return false;
        }

        const PlatformSegment& matchedSeg = this->platformSegments[static_cast<size_t>(bestSeg)];
        Ogre::Vector3 bestLocal;

        // BUGFIX: keep the matched point's real position.z instead of hardcoding 0 - this
        // return value is transformed straight through platformFrame into a world point and
        // used both as the cross-network snap target and as mergeOtherPlatformIntoThis's
        // source geometry, so the wrong z here silently misplaces both.
        if (true == bestIsFront)
        {
            bestLocal = Ogre::Vector3(matchedSeg.controlPoints.front().position.x, matchedSeg.controlPoints.front().smoothedHeight, matchedSeg.controlPoints.front().position.z);
        }
        else
        {
            bestLocal = Ogre::Vector3(matchedSeg.controlPoints.back().position.x, matchedSeg.controlPoints.back().smoothedHeight, matchedSeg.controlPoints.back().position.z);
        }

        outPoint = this->platformFrame * bestLocal;
        return true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Segment / Connection API
    ///////////////////////////////////////////////////////////////////////////////////////////////

    void ProceduralPlatformComponent::addPlatformSegment(const Ogre::Vector3& start, const Ogre::Vector3& end)
    {
        // NOTE: unlike ProceduralRoadComponent::addRoadSegment, there is no frame-
        // bootstrapping block here - platformFrame is already captured in postInit() (see
        // the header comment there), so a scripted/Lua caller never needs the first-call
        // fallback a frame-less Road requires.
        const Ogre::Quaternion invFrame = this->platformFrame.Inverse();
        const Ogre::Vector3 localStart = invFrame * start;
        const Ogre::Vector3 localEnd = invFrame * end;

        PlatformSegment seg;
        seg.isCurved = false;
        seg.curvature = 0.0f;

        // BUGFIX: keep localStart.z/localEnd.z (the caller-provided world point's real local
        // depth) instead of hardcoding 0 - this point is stored long-term and later read
        // back out for world reconstruction (getPlatformConnectionPoint, cross-network
        // merge), so discarding it here silently moved the platform to Z=0 regardless of
        // where the caller actually meant it to sit.
        PlatformControlPoint cpStart;
        cpStart.position = Ogre::Vector3(localStart.x, 0.0f, localStart.z);
        cpStart.rawHeight = localStart.y;
        cpStart.smoothedHeight = cpStart.rawHeight;
        cpStart.distFromStart = 0.0f;

        PlatformControlPoint cpEnd;
        cpEnd.position = Ogre::Vector3(localEnd.x, 0.0f, localEnd.z);
        cpEnd.rawHeight = localEnd.y;
        cpEnd.smoothedHeight = cpEnd.rawHeight;
        cpEnd.distFromStart = pathDistance2D(localStart.x, localStart.y, localEnd.x, localEnd.y);

        if (false == this->hasPlatformOrigin)
        {
            this->platformOrigin = Ogre::Vector3(localStart.x, cpStart.rawHeight, localStart.z);
            this->hasPlatformOrigin = true;
        }

        seg.controlPoints.push_back(cpStart);
        seg.controlPoints.push_back(cpEnd);
        this->platformSegments.push_back(seg);
        if (false == this->bBatchMode)
        {
            this->rebuildMesh();
        }
        this->updateContinuationPoint();
    }

    int ProceduralPlatformComponent::getSegmentCount(void) const
    {
        return static_cast<int>(this->platformSegments.size());
    }

    Ogre::Vector3 ProceduralPlatformComponent::getPlatformConnectionPoint(bool atStart) const
    {
        if (this->platformSegments.empty())
        {
            return this->gameObjectPtr->getSceneNode()->_getDerivedPositionUpdated();
        }

        if (atStart)
        {
            const PlatformControlPoint& cp = this->platformSegments.front().controlPoints.front();
            // BUGFIX: cp.position.z (the real, stored plane constant), not a hardcoded 0 -
            // this is an external API (e.g. for connecting another procedural component to
            // this platform's end), so it must return the true world point.
            return this->platformFrame * Ogre::Vector3(cp.position.x, cp.smoothedHeight, cp.position.z);
        }
        else
        {
            // loadedPlatformEndpoint already carries the real z (copied straight from a
            // stored control point in updateContinuationPoint), so just use it directly.
            return this->platformFrame * Ogre::Vector3(this->loadedPlatformEndpoint.x, this->loadedPlatformEndpointHeight, this->loadedPlatformEndpoint.z);
        }
    }

    Ogre::Vector3 ProceduralPlatformComponent::getPlatformApproachDirection(bool atStart) const
    {
        if (this->platformSegments.empty())
        {
            return Ogre::Vector3::UNIT_X;
        }

        Ogre::Vector3 dir;
        if (atStart)
        {
            const auto& cps = this->platformSegments.front().controlPoints;
            if (cps.size() >= 2)
            {
                dir = Ogre::Vector3(cps[0].position.x - cps[1].position.x, cps[0].smoothedHeight - cps[1].smoothedHeight, 0.0f);
            }
            else
            {
                dir = -Ogre::Vector3::UNIT_X;
            }
        }
        else
        {
            const auto& cps = this->platformSegments.back().controlPoints;
            const size_t n = cps.size();
            if (n >= 2)
            {
                dir = Ogre::Vector3(cps[n - 1].position.x - cps[n - 2].position.x, cps[n - 1].smoothedHeight - cps[n - 2].smoothedHeight, 0.0f);
            }
            else
            {
                dir = Ogre::Vector3::UNIT_X;
            }
        }

        if (dir.isZeroLength())
        {
            return Ogre::Vector3::UNIT_X;
        }

        dir.normalise();
        return this->platformFrame * dir;
    }

    void ProceduralPlatformComponent::addPlatformSegmentBatch(const Ogre::Vector3& start, const Ogre::Vector3& end)
    {
        // Identical to addPlatformSegment() but WITHOUT the rebuildMesh() call. After adding
        // all segments, call finalizeBatch() once to build the mesh.
        const Ogre::Quaternion invFrame = this->platformFrame.Inverse();
        const Ogre::Vector3 localStart = invFrame * start;
        const Ogre::Vector3 localEnd = invFrame * end;

        if (!this->originPositionSet)
        {
            this->platformOrigin = Ogre::Vector3(localStart.x, 0.0f, localStart.z);
            this->originPositionSet = true;
        }

        PlatformSegment seg;
        seg.isCurved = false;
        seg.curvature = 0.0f;

        // BUGFIX: same as addPlatformSegment - keep localStart.z/localEnd.z instead of
        // hardcoding 0.
        PlatformControlPoint cp0;
        cp0.position = Ogre::Vector3(localStart.x, 0.0f, localStart.z);
        cp0.rawHeight = localStart.y;
        cp0.smoothedHeight = cp0.rawHeight;
        seg.controlPoints.push_back(cp0);

        PlatformControlPoint cp1;
        cp1.position = Ogre::Vector3(localEnd.x, 0.0f, localEnd.z);
        cp1.rawHeight = localEnd.y;
        cp1.smoothedHeight = cp1.rawHeight;
        seg.controlPoints.push_back(cp1);

        this->platformSegments.push_back(std::move(seg));
        this->updateContinuationPoint();
        // NOTE: rebuildMesh() intentionally NOT called here
    }

    void ProceduralPlatformComponent::finalizeBatch(void)
    {
        this->rebuildMesh();
    }

    void ProceduralPlatformComponent::beginBatch(void)
    {
        this->bBatchMode = true;
    }

    void ProceduralPlatformComponent::endBatch(void)
    {
        this->bBatchMode = false;
        this->rebuildMesh();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Event Handlers
    ///////////////////////////////////////////////////////////////////////////////////////////////

    void ProceduralPlatformComponent::handleMeshModifyMode(NOWA::EventDataPtr eventData)
    {
        auto castEventData = boost::static_pointer_cast<EventDataEditorMode>(eventData);

        this->isEditorMeshModifyMode = (castEventData->getManipulationMode() == EditorManager::EDITOR_MESH_MODIFY_MODE);

        this->updateModificationState();
    }

    void ProceduralPlatformComponent::handleGameObjectSelected(NOWA::EventDataPtr eventData)
    {
        auto castEventData = boost::static_pointer_cast<EventDataGameObjectSelected>(eventData);

        if (castEventData->getGameObjectId() == this->gameObjectPtr->getId())
        {
            this->isSelected = castEventData->getIsSelected();
            if (false == this->isSelected)
            {
                this->setEditMode("Object");
                return;
            }
        }
        else if (castEventData->getIsSelected())
        {
            this->isSelected = false;
        }

        if (false == castEventData->getIsPartOfMultiSelection())
        {
            const bool segmentMode = (this->getEditModeEnum() == EditMode::SEGMENT);
            if (true == segmentMode)
            {
                // Go directly to mesh modify mode when switching to Segment edit mode, so the user can immediately select segments
                boost::shared_ptr<EventDataEditorMode> eventDataEditorMode(new EventDataEditorMode(NOWA::EditorManager::EDITOR_MESH_MODIFY_MODE));
                NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataEditorMode);
            }
        }

        this->updateModificationState();
    }

    void ProceduralPlatformComponent::handleComponentManuallyDeleted(NOWA::EventDataPtr eventData)
    {
        boost::shared_ptr<EventDataDeleteComponent> castEventData = boost::static_pointer_cast<EventDataDeleteComponent>(eventData);
        if (this->gameObjectPtr->getId() == castEventData->getGameObjectId())
        {
            if (this->getClassName() == castEventData->getComponentName())
            {
                this->deletePlatformDataFile();
            }
        }
    }

    void ProceduralPlatformComponent::addInputListener(void)
    {
        const Ogre::String listenerName = ProceduralPlatformComponent::getStaticClassName() + "_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
        if (auto* core = InputDeviceCore::getSingletonPtr())
        {
            core->addKeyListener(this, listenerName);
            core->addMouseListener(this, listenerName);
        }
    }

    void ProceduralPlatformComponent::removeInputListener(void)
    {
        const Ogre::String listenerName = ProceduralPlatformComponent::getStaticClassName() + "_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
        if (auto* core = InputDeviceCore::getSingletonPtr())
        {
            core->removeKeyListener(listenerName);
            core->removeMouseListener(listenerName);
        }
    }

    void ProceduralPlatformComponent::updateModificationState(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPlatformComponent] updateModificationState: activated=" + Ogre::StringConverter::toString(this->activated->getBool()) +
                                                                               " meshModifyMode=" + Ogre::StringConverter::toString(this->isEditorMeshModifyMode) + " selected=" + Ogre::StringConverter::toString(this->isSelected) +
                                                                               " editMode=" + this->editMode->getListSelectedValue());

        const bool shouldBeActive = this->activated->getBool() && this->isEditorMeshModifyMode && this->isSelected;

        if (shouldBeActive)
        {
            this->addInputListener();

            if (false == this->platformSegments.empty())
            {
                this->updateContinuationPoint();
            }

            // Refresh overlay whenever we become active - covers the case where the user
            // set Segment mode first, THEN clicked into mesh-modify mode.
            this->scheduleSegmentOverlayUpdate();
        }
        else
        {
            this->removeInputListener();

            if (this->buildState == BuildState::DRAGGING)
            {
                this->cancelPlatform();
            }

            this->selectedSegmentIndex = -1;
            this->scheduleSegmentOverlayUpdate();
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    //  Lua API
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ProceduralPlatformComponent* getProceduralPlatformComponent(GameObject* go)
    {
        return NOWA::makeStrongPtr(go->getComponent<ProceduralPlatformComponent>()).get();
    }

    ProceduralPlatformComponent* getProceduralPlatformComponentFromName(GameObject* go, const Ogre::String& name)
    {
        return NOWA::makeStrongPtr(go->getComponentFromName<ProceduralPlatformComponent>(name)).get();
    }

    void ProceduralPlatformComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
    {
        luabind::module(lua)[luabind::class_<ProceduralPlatformComponent, GameObjectComponent>("ProceduralPlatformComponent")

                // ── Activation ────────────────────────────────────────────────
                .def("setActivated", &ProceduralPlatformComponent::setActivated)
                .def("isActivated", &ProceduralPlatformComponent::isActivated)

                // ── Platform dimensions ──────────────────────────────────────
                .def("setPlatformDepth", &ProceduralPlatformComponent::setPlatformDepth)
                .def("getPlatformDepth", &ProceduralPlatformComponent::getPlatformDepth)
                .def("setPlatformHeight", &ProceduralPlatformComponent::setPlatformHeight)
                .def("getPlatformHeight", &ProceduralPlatformComponent::getPlatformHeight)

                // ── Platform style ────────────────────────────────────────────
                .def("setPlatformStyle", &ProceduralPlatformComponent::setPlatformStyle)
                .def("getPlatformStyle", &ProceduralPlatformComponent::getPlatformStyle)

                // ── Height smoothing ──────────────────────────────────────────
                .def("setSmoothingFactor", &ProceduralPlatformComponent::setSmoothingFactor)
                .def("getSmoothingFactor", &ProceduralPlatformComponent::getSmoothingFactor)

                // ── Grid / snap ───────────────────────────────────────────────
                .def("setSnapToGrid", &ProceduralPlatformComponent::setSnapToGrid)
                .def("getSnapToGrid", &ProceduralPlatformComponent::getSnapToGrid)
                .def("setGridSize", &ProceduralPlatformComponent::setGridSize)
                .def("getGridSize", &ProceduralPlatformComponent::getGridSize)

                // ── Datablocks ────────────────────────────────────────────────
                .def("setSurfaceDatablock", &ProceduralPlatformComponent::setSurfaceDatablock)
                .def("getSurfaceDatablock", &ProceduralPlatformComponent::getSurfaceDatablock)
                .def("setGroundDatablock", &ProceduralPlatformComponent::setGroundDatablock)
                .def("getGroundDatablock", &ProceduralPlatformComponent::getGroundDatablock)

                // ── UV tiling ─────────────────────────────────────────────────
                .def("setSurfaceUVTiling", &ProceduralPlatformComponent::setSurfaceUVTiling)
                .def("getSurfaceUVTiling", &ProceduralPlatformComponent::getSurfaceUVTiling)
                .def("setGroundUVTiling", &ProceduralPlatformComponent::setGroundUVTiling)
                .def("getGroundUVTiling", &ProceduralPlatformComponent::getGroundUVTiling)

                // ── Curve quality ─────────────────────────────────────────────
                .def("setCurveSubdivisions", &ProceduralPlatformComponent::setCurveSubdivisions)
                .def("getCurveSubdivisions", &ProceduralPlatformComponent::getCurveSubdivisions)

                // ── Segment management ────────────────────────────────────────
                .def("getSegmentCount", &ProceduralPlatformComponent::getSegmentCount)
                .def("addPlatformSegment", &ProceduralPlatformComponent::addPlatformSegment)];

        // ── LuaScriptApi documentation ─────────────────────────────────────────
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPlatformComponent", "class inherits GameObjectComponent", ProceduralPlatformComponent::getStaticInfoText());
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPlatformComponent", "void setActivated(bool activated)", "Activates or deactivates the platform component.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPlatformComponent", "void setPlatformDepth(float depth)", "Sets the platform's Z-thickness (fixed depth axis) in world units.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPlatformComponent", "void setPlatformHeight(float height)", "Sets how far the platform body extends downward from its top surface.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPlatformComponent", "void setPlatformStyle(string style)", "Sets platform style. Values: 'Grass', 'Wood', 'Stone', 'Ice', 'Metal'.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPlatformComponent", "void setSurfaceDatablock(string name)", "Sets the PBS datablock for the platform's top walkable surface.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPlatformComponent", "void setGroundDatablock(string name)", "Sets the PBS datablock for the platform body (sides/bottom); junction fan patches always reuse it too.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPlatformComponent", "int getSegmentCount()", "Returns the number of platform segments currently placed.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPlatformComponent", "void addPlatformSegment(Vector3 start, Vector3 end)", "Adds a single platform segment from start to end world position and rebuilds.");

        // ── Register on GameObject and GameObjectController ────────────────────
        gameObjectClass.def("getProceduralPlatformComponent", (ProceduralPlatformComponent * (*)(GameObject*)) & getProceduralPlatformComponent);
        gameObjectClass.def("getProceduralPlatformComponentFromName", &getProceduralPlatformComponentFromName);
        gameObjectControllerClass.def("castProceduralPlatformComponent", &GameObjectController::cast<ProceduralPlatformComponent>);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ProceduralPlatformComponent getProceduralPlatformComponent()", "Gets the ProceduralPlatformComponent from this GameObject.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ProceduralPlatformComponent getProceduralPlatformComponentFromName(string name)", "Gets a named ProceduralPlatformComponent from this GameObject.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "ProceduralPlatformComponent castProceduralPlatformComponent(ProceduralPlatformComponent other)", "Casts for Lua auto-completion support.");
    }

} // namespace NOWA
/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#include "NOWAPrecompiled.h"
#include "ProceduralPlatformBoundaryComponent.h"
#include "editor/EditorManager.h"
#include "gameobject/GameObjectController.h"
#include "gameobject/GameObjectFactory.h"
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
#include "OgreMeshManager2.h"
#include "OgreSubMesh2.h"
#include "Vao/OgreVaoManager.h"
#include "Vao/OgreVertexArrayObject.h"

#include "OgreAbiUtils.h"

#include <sstream>

// =============================================================================
// ProceduralPlatformBoundaryComponent
//
// The simplified, non-interactive sibling of ProceduralPlatformComponent. See the class comment
// in the header for why it is a separate component rather than a mode of that one.
//
// Design decisions worth stating up front, because they explain most of what follows:
//
//   - The boundary is regenerated WHOLE on every change. There is no incremental update path and
//     no cached vertex buffer in a data file. The geometry is a pure function of five numbers and
//     a set of removed cell indices, and regenerating a 100 x 20 boundary is a few thousand
//     vertices - far below the point where caching would pay for the ways it could go stale.
//     This is the opposite choice from ProceduralPlatformComponent, which caches its mesh in a
//     .platformdata file, and deliberately so: there, the geometry depends on a hand-drawn path
//     that cannot be recomputed from a handful of properties.
//
//   - Removed cells are stored as a plain string attribute in the scene XML, not in a binary
//     side-car file. A level has a handful of doorways, so the whole state is a short list of
//     integers - readable in the scene file, diffable, and with nothing to keep in sync.
//
//   - A run of consecutive kept cells is emitted as ONE box, not as one box per cell. Besides
//     the obvious triangle saving, this is what makes a three-meter doorway read as a single
//     opening: without merging, every cell boundary would leave two coincident interior faces
//     z-fighting against each other along the whole boundary.
// =============================================================================

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    ProceduralPlatformBoundaryComponent::ProceduralPlatformBoundaryComponent() :
        PlatformComponentBase(),
        name("ProceduralPlatformBoundaryComponent"),
        activated(new Variant(ProceduralPlatformBoundaryComponent::AttrActivated(), true, this->attributes)),
        boundaryWidth(new Variant(ProceduralPlatformBoundaryComponent::AttrBoundaryWidth(), 100.0f, this->attributes)),
        boundaryHeight(new Variant(ProceduralPlatformBoundaryComponent::AttrBoundaryHeight(), 20.0f, this->attributes)),
        boundaryDepth(new Variant(ProceduralPlatformBoundaryComponent::AttrBoundaryDepth(), 10.0f, this->attributes)),
        wallThickness(new Variant(ProceduralPlatformBoundaryComponent::AttrWallThickness(), 1.0f, this->attributes)),
        surfaceDatablock(new Variant(ProceduralPlatformBoundaryComponent::AttrSurfaceDatablock(), Ogre::String("grass_clean"), this->attributes)),
        groundDatablock(new Variant(ProceduralPlatformBoundaryComponent::AttrGroundDatablock(), Ogre::String("rockClif_D"), this->attributes)),
        surfaceUVTiling(new Variant(ProceduralPlatformBoundaryComponent::AttrSurfaceUVTiling(), Ogre::Vector2(1.0f, 5.0f), this->attributes)),
        groundUVTiling(new Variant(ProceduralPlatformBoundaryComponent::AttrGroundUVTiling(), Ogre::Vector2(1.0f, 1.0f), this->attributes)),
        editMode(new Variant(ProceduralPlatformBoundaryComponent::AttrEditMode(), std::vector<Ogre::String>{"Object", "Segment"}, this->attributes)),
        currentSurfaceVertexIndex(0),
        currentGroundVertexIndex(0),
        boundaryItem(nullptr),
        hasSelectedCell(false),
        segOverlayNode(nullptr),
        segOverlayObject(nullptr),
        isEditorMeshModifyMode(false),
        isSelected(false),
        isShiftKeyDown(false),
        physicsArtifactComponent(nullptr)
    {
        this->activated->setDescription("Activates the boundary. When deactivated the mesh is removed.");

        this->boundaryWidth->setDescription("Level length in meters, along the direction the player runs. This is an exact outer "
                                            "dimension - a value of 100 means the level really is 100 meters wide, which is what "
                                            "chaining levels and drawing a minimap depend on.");
        this->boundaryWidth->setConstraints(1.0f, 10000.0f);

        this->boundaryHeight->setDescription("Level height in meters, from the outside of the floor to the outside of the ceiling.");
        this->boundaryHeight->setConstraints(1.0f, 10000.0f);

        this->boundaryDepth->setDescription("Level depth in meters, along the Z axis. The play plane sits in the middle, so the "
                                            "boundary spans -Depth/2 .. +Depth/2 in local Z.");
        this->boundaryDepth->setConstraints(0.1f, 1000.0f);

        this->wallThickness->setDescription("How solid the floor, ceiling and walls are, in meters. This eats INTO the level from "
                                            "the outside, so it never changes the outer dimensions above.");
        this->wallThickness->setConstraints(0.05f, 100.0f);

        this->surfaceDatablock->setDescription("Datablock for the faces pointing into the level - what the player sees and walks on.");
        this->groundDatablock->setDescription("Datablock for the outer shell and for the cut faces of a doorway.");

        this->surfaceUVTiling->setDescription("Texture tiling of the inner faces, in meters per texture repeat.");
        this->groundUVTiling->setDescription("Texture tiling of the outer shell, in meters per texture repeat.");

        this->editMode->setDescription("'Object' leaves the boundary alone. 'Segment' lets you click one-meter cells and cut "
                                       "doorways with X (SHIFT+X restores).");
        this->editMode->addUserData(GameObject::AttrActionNoUndo());

        this->boundaryMeshName = "";
    }

    ProceduralPlatformBoundaryComponent::~ProceduralPlatformBoundaryComponent()
    {
    }

    const Ogre::String& ProceduralPlatformBoundaryComponent::getName() const
    {
        return this->name;
    }

    void ProceduralPlatformBoundaryComponent::install(const Ogre::NameValuePairList* options)
    {
        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<ProceduralPlatformBoundaryComponent>(ProceduralPlatformBoundaryComponent::getStaticClassId(), ProceduralPlatformBoundaryComponent::getStaticClassName());
    }

    void ProceduralPlatformBoundaryComponent::shutdown()
    {
    }

    void ProceduralPlatformBoundaryComponent::uninstall()
    {
    }

    void ProceduralPlatformBoundaryComponent::initialise()
    {
    }

    void ProceduralPlatformBoundaryComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    bool ProceduralPlatformBoundaryComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPlatformBoundaryComponent::AttrActivated())
        {
            this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPlatformBoundaryComponent::AttrBoundaryWidth())
        {
            this->boundaryWidth->setValue(Ogre::Math::Clamp(XMLConverter::getAttribReal(propertyElement, "data", 100.0f), 1.0f, 10000.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPlatformBoundaryComponent::AttrBoundaryHeight())
        {
            this->boundaryHeight->setValue(Ogre::Math::Clamp(XMLConverter::getAttribReal(propertyElement, "data", 20.0f), 1.0f, 10000.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPlatformBoundaryComponent::AttrBoundaryDepth())
        {
            this->boundaryDepth->setValue(Ogre::Math::Clamp(XMLConverter::getAttribReal(propertyElement, "data", 10.0f), 0.1f, 1000.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPlatformBoundaryComponent::AttrWallThickness())
        {
            this->wallThickness->setValue(Ogre::Math::Clamp(XMLConverter::getAttribReal(propertyElement, "data", 1.0f), 0.05f, 100.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPlatformBoundaryComponent::AttrSurfaceDatablock())
        {
            this->surfaceDatablock->setValue(XMLConverter::getAttrib(propertyElement, "data", "grass_clean"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPlatformBoundaryComponent::AttrGroundDatablock())
        {
            this->groundDatablock->setValue(XMLConverter::getAttrib(propertyElement, "data", "rockClif_D"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPlatformBoundaryComponent::AttrSurfaceUVTiling())
        {
            this->surfaceUVTiling->setValue(XMLConverter::getAttribVector2(propertyElement, "data", Ogre::Vector2(1.0f, 1.0f)));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPlatformBoundaryComponent::AttrGroundUVTiling())
        {
            this->groundUVTiling->setValue(XMLConverter::getAttribVector2(propertyElement, "data", Ogre::Vector2(1.0f, 1.0f)));
            propertyElement = propertyElement->next_sibling("property");
        }

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPlatformBoundaryComponent::AttrRemovedCells())
        {
            // Stored as plain text rather than in a binary side-car file: a level has a handful of
            // doorways, so the entire state is a short list of integers. Keeping it in the scene
            // XML means it is readable, diffable, and there is no second file to keep in sync.
            this->deserializeRemovedCells(XMLConverter::getAttrib(propertyElement, "data", ""));
            propertyElement = propertyElement->next_sibling("property");
        }

        return true;
    }

    GameObjectCompPtr ProceduralPlatformBoundaryComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        ProceduralPlatformBoundaryCompPtr clonedCompPtr(boost::make_shared<ProceduralPlatformBoundaryComponent>());

        clonedCompPtr->setBoundaryWidth(this->boundaryWidth->getReal());
        clonedCompPtr->setBoundaryHeight(this->boundaryHeight->getReal());
        clonedCompPtr->setBoundaryDepth(this->boundaryDepth->getReal());
        clonedCompPtr->setWallThickness(this->wallThickness->getReal());
        clonedCompPtr->setSurfaceDatablock(this->surfaceDatablock->getString());
        clonedCompPtr->setGroundDatablock(this->groundDatablock->getString());
        clonedCompPtr->setSurfaceUVTiling(this->surfaceUVTiling->getVector2());
        clonedCompPtr->setGroundUVTiling(this->groundUVTiling->getVector2());
        clonedCompPtr->deserializeRemovedCells(this->serializeRemovedCells());

        clonedGameObjectPtr->addComponent(clonedCompPtr);
        clonedCompPtr->setOwner(clonedGameObjectPtr);

        clonedCompPtr->setActivated(this->activated->getBool());

        return clonedCompPtr;
    }

    bool ProceduralPlatformBoundaryComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPlatformBoundaryComponent] Init boundary component for game object: " + this->gameObjectPtr->getName());

        this->boundaryMeshName = "ProceduralBoundaryMesh_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());

        this->createSegmentOverlay();

        AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &ProceduralPlatformBoundaryComponent::handleMeshModifyMode), EventDataEditorMode::getStaticEventType());
        AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &ProceduralPlatformBoundaryComponent::handleGameObjectSelected), EventDataGameObjectSelected::getStaticEventType());

        if (true == this->activated->getBool())
        {
            this->rebuildMesh();
        }

        return true;
    }

    bool ProceduralPlatformBoundaryComponent::connect(void)
    {
        GameObjectComponent::connect();

        // Leaving segment mode on simulation start: doorways are a design-time concept, and a
        // selection outline hanging in the running game would be a bug, not a feature.
        this->removeInputListener();
        this->hasSelectedCell = false;
        this->scheduleSegmentOverlayUpdate();

        return true;
    }

    bool ProceduralPlatformBoundaryComponent::disconnect(void)
    {
        GameObjectComponent::disconnect();
        this->updateModificationState();
        return true;
    }

    void ProceduralPlatformBoundaryComponent::onAddComponent(void)
    {
        // BUGFIX: this override did not exist. Without it a freshly added boundary is not selected
        // and nobody has claimed edit focus, so isSelected stays false, updateModificationState
        // never attaches the input listener, and Segment mode does nothing at all until the object
        // happens to be selected some other way.
        //
        // Both events mirror ProceduralPlatformComponent::onAddComponent: the claiming form of
        // EventDataEditorMode takes editing on this game object, and EventDataGameObjectSelected
        // puts the editor's selection on it so the designer can start cutting doorways right away.
        boost::shared_ptr<EventDataEditorMode> eventDataEditorMode(new EventDataEditorMode(EditorManager::EDITOR_MESH_MODIFY_MODE, this->gameObjectPtr->getId(), ProceduralPlatformBoundaryComponent::getStaticClassName()));
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataEditorMode);

        boost::shared_ptr<NOWA::EventDataGameObjectSelected> eventDataGameObjectSelected(new NOWA::EventDataGameObjectSelected(this->gameObjectPtr->getId(), true, false));
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataGameObjectSelected);

        this->isSelected = true;
        this->updateModificationState();
    }

    void ProceduralPlatformBoundaryComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();

        this->physicsArtifactComponent = nullptr;

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPlatformBoundaryComponent] Remove boundary component for game object: " + this->gameObjectPtr->getName());

        AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ProceduralPlatformBoundaryComponent::handleMeshModifyMode), EventDataEditorMode::getStaticEventType());
        AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ProceduralPlatformBoundaryComponent::handleGameObjectSelected), EventDataGameObjectSelected::getStaticEventType());

        this->removeInputListener();
        this->destroySegmentOverlay();
        this->destroyBoundaryMesh();
    }

    void ProceduralPlatformBoundaryComponent::onOtherComponentRemoved(unsigned int index)
    {
        if (nullptr != this->physicsArtifactComponent && index == this->physicsArtifactComponent->getIndex())
        {
            this->physicsArtifactComponent = nullptr;
        }
    }

    void ProceduralPlatformBoundaryComponent::update(Ogre::Real dt, bool notSimulating)
    {
        // Nothing to do per frame: the boundary is static geometry and every change is
        // event-driven. Kept as an explicit empty override so it is clear this is intentional
        // rather than forgotten.
    }

    void ProceduralPlatformBoundaryComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (ProceduralPlatformBoundaryComponent::AttrActivated() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
        }
        else if (ProceduralPlatformBoundaryComponent::AttrBoundaryWidth() == attribute->getName())
        {
            this->setBoundaryWidth(attribute->getReal());
        }
        else if (ProceduralPlatformBoundaryComponent::AttrBoundaryHeight() == attribute->getName())
        {
            this->setBoundaryHeight(attribute->getReal());
        }
        else if (ProceduralPlatformBoundaryComponent::AttrBoundaryDepth() == attribute->getName())
        {
            this->setBoundaryDepth(attribute->getReal());
        }
        else if (ProceduralPlatformBoundaryComponent::AttrWallThickness() == attribute->getName())
        {
            this->setWallThickness(attribute->getReal());
        }
        else if (ProceduralPlatformBoundaryComponent::AttrSurfaceDatablock() == attribute->getName())
        {
            this->setSurfaceDatablock(attribute->getString());
        }
        else if (ProceduralPlatformBoundaryComponent::AttrGroundDatablock() == attribute->getName())
        {
            this->setGroundDatablock(attribute->getString());
        }
        else if (ProceduralPlatformBoundaryComponent::AttrSurfaceUVTiling() == attribute->getName())
        {
            this->setSurfaceUVTiling(attribute->getVector2());
        }
        else if (ProceduralPlatformBoundaryComponent::AttrGroundUVTiling() == attribute->getName())
        {
            this->setGroundUVTiling(attribute->getVector2());
        }
        else if (ProceduralPlatformBoundaryComponent::AttrEditMode() == attribute->getName())
        {
            this->setEditMode(attribute->getListSelectedValue());
        }
    }

    void ProceduralPlatformBoundaryComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        // 2 = int, 6 = real, 7 = string, 8 = vector2, 9 = vector3, 12 = bool
        GameObjectComponent::writeXML(propertiesXML, doc);

        xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPlatformBoundaryComponent::AttrActivated().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPlatformBoundaryComponent::AttrBoundaryWidth().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->boundaryWidth->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPlatformBoundaryComponent::AttrBoundaryHeight().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->boundaryHeight->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPlatformBoundaryComponent::AttrBoundaryDepth().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->boundaryDepth->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPlatformBoundaryComponent::AttrWallThickness().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wallThickness->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPlatformBoundaryComponent::AttrSurfaceDatablock().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->surfaceDatablock->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPlatformBoundaryComponent::AttrGroundDatablock().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->groundDatablock->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPlatformBoundaryComponent::AttrSurfaceUVTiling().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->surfaceUVTiling->getVector2())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPlatformBoundaryComponent::AttrGroundUVTiling().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->groundUVTiling->getVector2())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPlatformBoundaryComponent::AttrRemovedCells().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->serializeRemovedCells())));
        propertiesXML->append_node(propertyXML);
    }

    // =========================================================================================
    // Removed-cell bookkeeping
    // =========================================================================================

    Ogre::String ProceduralPlatformBoundaryComponent::serializeRemovedCells(void) const
    {
        // Format: "side:index side:index ...". Space separated so it survives an XML attribute
        // without escaping, and readable enough that a doorway can be checked or hand-edited in
        // the scene file.
        Ogre::StringStream stream;
        bool first = true;

        for (const BoundaryCell& cell : this->removedCells)
        {
            if (false == first)
            {
                stream << " ";
            }
            stream << static_cast<int>(cell.side) << ":" << cell.index;
            first = false;
        }

        return stream.str();
    }

    void ProceduralPlatformBoundaryComponent::deserializeRemovedCells(const Ogre::String& data)
    {
        this->removedCells.clear();

        if (true == data.empty())
        {
            return;
        }

        std::istringstream stream(data);
        Ogre::String token;

        while (stream >> token)
        {
            const size_t colonPos = token.find(':');
            if (Ogre::String::npos == colonPos)
            {
                continue;
            }

            const int sideValue = Ogre::StringConverter::parseInt(token.substr(0, colonPos), 0);
            const int index = Ogre::StringConverter::parseInt(token.substr(colonPos + 1), -1);

            if (sideValue < 0 || sideValue > 3 || index < 0)
            {
                continue;
            }

            BoundaryCell cell;
            cell.side = static_cast<BoundarySide>(sideValue);
            cell.index = index;
            this->removedCells.insert(cell);
        }
    }

    // =========================================================================================
    // PlatformComponentBase data interface (undo/redo)
    // =========================================================================================

    std::vector<unsigned char> ProceduralPlatformBoundaryComponent::getPlatformData(void) const
    {
        // Captures every value rebuildMesh() reads, plus the removed-cell set - nothing more.
        // See the header comment on this function for why no mesh data is included: the geometry
        // is cheap to regenerate, so there is nothing worth caching.
        const Ogre::String surfaceDb = this->surfaceDatablock->getString();
        const Ogre::String groundDb = this->groundDatablock->getString();
        const bool isSegmentMode = (EditMode::SEGMENT == this->getEditModeEnum());

        const uint32_t surfaceDbLen = static_cast<uint32_t>(surfaceDb.size());
        const uint32_t groundDbLen = static_cast<uint32_t>(groundDb.size());
        const uint32_t numRemovedCells = static_cast<uint32_t>(this->removedCells.size());

        const size_t totalSize = 4 + 4                                             // magic, version
                                 + 4 * 4                                           // width, height, depth, thickness
                                 + 1 + 1                                           // activated, edit mode
                                 + 4 + surfaceDbLen                                // surface datablock name
                                 + 4 + groundDbLen                                 // ground datablock name
                                 + 4 * 4                                           // surface + ground UV tiling
                                 + 4                                               // removed cell count
                                 + static_cast<size_t>(numRemovedCells) * (1 + 4); // side + index per cell

        std::vector<unsigned char> result(totalSize);
        size_t off = 0;

        const uint32_t magic = ProceduralPlatformBoundaryComponent::BOUNDARYDATA_MAGIC;
        const uint32_t version = ProceduralPlatformBoundaryComponent::BOUNDARYDATA_VERSION;
        memcpy(&result[off], &magic, 4);
        off += 4;
        memcpy(&result[off], &version, 4);
        off += 4;

        const float width = this->boundaryWidth->getReal();
        const float height = this->boundaryHeight->getReal();
        const float depth = this->boundaryDepth->getReal();
        const float thickness = this->wallThickness->getReal();
        memcpy(&result[off], &width, 4);
        off += 4;
        memcpy(&result[off], &height, 4);
        off += 4;
        memcpy(&result[off], &depth, 4);
        off += 4;
        memcpy(&result[off], &thickness, 4);
        off += 4;

        result[off++] = this->activated->getBool() ? 1 : 0;
        result[off++] = isSegmentMode ? 1 : 0;

        memcpy(&result[off], &surfaceDbLen, 4);
        off += 4;
        if (surfaceDbLen > 0)
        {
            memcpy(&result[off], surfaceDb.data(), surfaceDbLen);
        }
        off += surfaceDbLen;

        memcpy(&result[off], &groundDbLen, 4);
        off += 4;
        if (groundDbLen > 0)
        {
            memcpy(&result[off], groundDb.data(), groundDbLen);
        }
        off += groundDbLen;

        const Ogre::Vector2 surfUV = this->surfaceUVTiling->getVector2();
        const Ogre::Vector2 groundUV = this->groundUVTiling->getVector2();
        memcpy(&result[off], &surfUV.x, 4);
        off += 4;
        memcpy(&result[off], &surfUV.y, 4);
        off += 4;
        memcpy(&result[off], &groundUV.x, 4);
        off += 4;
        memcpy(&result[off], &groundUV.y, 4);
        off += 4;

        memcpy(&result[off], &numRemovedCells, 4);
        off += 4;

        for (const BoundaryCell& cell : this->removedCells)
        {
            result[off++] = static_cast<uint8_t>(cell.side);
            const int32_t index = static_cast<int32_t>(cell.index);
            memcpy(&result[off], &index, 4);
            off += 4;
        }

        return result;
    }

    void ProceduralPlatformBoundaryComponent::setPlatformData(const std::vector<unsigned char>& data)
    {
        if (true == data.empty())
        {
            // Mirrors ProceduralPlatformComponent::setPlatformData: empty is the legitimate
            // "nothing to restore" state a freshly created component starts in, not an error.
            this->removedCells.clear();
            this->rebuildMesh();
            this->scheduleSegmentOverlayUpdate();
            return;
        }

        // Only the fixed-size portion of the header can be checked up front. The datablock names
        // and the removed-cell list are variable length and come FROM the buffer, so each of them
        // is bounds-checked individually as it is read, the same layered approach
        // ProceduralPlatformComponent::setPlatformData uses for its own variable-length sections.
        const size_t fixedHeaderSize = 4 + 4 + 4 * 4 + 1 + 1 + 4 + 4 + 4 * 4 + 4;
        if (data.size() < fixedHeaderSize)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPlatformBoundaryComponent] setPlatformData: buffer too small");
            return;
        }

        size_t off = 0;

        uint32_t magic = 0;
        uint32_t version = 0;
        memcpy(&magic, &data[off], 4);
        off += 4;
        memcpy(&version, &data[off], 4);
        off += 4;

        if (magic != ProceduralPlatformBoundaryComponent::BOUNDARYDATA_MAGIC || version != ProceduralPlatformBoundaryComponent::BOUNDARYDATA_VERSION)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPlatformBoundaryComponent] setPlatformData: invalid magic/version");
            return;
        }

        float width = 0.0f;
        float height = 0.0f;
        float depth = 0.0f;
        float thickness = 0.0f;
        memcpy(&width, &data[off], 4);
        off += 4;
        memcpy(&height, &data[off], 4);
        off += 4;
        memcpy(&depth, &data[off], 4);
        off += 4;
        memcpy(&thickness, &data[off], 4);
        off += 4;

        const uint8_t activatedByte = data[off++];
        const uint8_t editModeByte = data[off++];

        uint32_t surfaceDbLen = 0;
        memcpy(&surfaceDbLen, &data[off], 4);
        off += 4;
        if (off + surfaceDbLen > data.size())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPlatformBoundaryComponent] setPlatformData: buffer too small for surface datablock name");
            return;
        }
        const Ogre::String surfaceDb(reinterpret_cast<const char*>(&data[off]), surfaceDbLen);
        off += surfaceDbLen;

        if (off + 4 > data.size())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPlatformBoundaryComponent] setPlatformData: buffer too small for ground datablock length");
            return;
        }
        uint32_t groundDbLen = 0;
        memcpy(&groundDbLen, &data[off], 4);
        off += 4;
        if (off + groundDbLen > data.size())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPlatformBoundaryComponent] setPlatformData: buffer too small for ground datablock name");
            return;
        }
        const Ogre::String groundDb(reinterpret_cast<const char*>(&data[off]), groundDbLen);
        off += groundDbLen;

        if (off + 4 * 4 + 4 > data.size())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPlatformBoundaryComponent] setPlatformData: buffer too small for UV tiling / cell count");
            return;
        }

        Ogre::Vector2 surfUV;
        Ogre::Vector2 groundUV;
        memcpy(&surfUV.x, &data[off], 4);
        off += 4;
        memcpy(&surfUV.y, &data[off], 4);
        off += 4;
        memcpy(&groundUV.x, &data[off], 4);
        off += 4;
        memcpy(&groundUV.y, &data[off], 4);
        off += 4;

        uint32_t numRemovedCells = 0;
        memcpy(&numRemovedCells, &data[off], 4);
        off += 4;

        std::set<BoundaryCell> restoredCells;
        for (uint32_t i = 0; i < numRemovedCells; ++i)
        {
            if (off + 1 + 4 > data.size())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPlatformBoundaryComponent] setPlatformData: unexpected end of buffer at removed cell " + Ogre::StringConverter::toString(i));
                break;
            }

            const uint8_t sideByte = data[off++];
            int32_t index = 0;
            memcpy(&index, &data[off], 4);
            off += 4;

            if (sideByte > 3 || index < 0)
            {
                continue;
            }

            BoundaryCell cell;
            cell.side = static_cast<BoundarySide>(sideByte);
            cell.index = static_cast<int>(index);
            restoredCells.insert(cell);
        }

        // Assigned directly to the Variants rather than through the public setBoundaryWidth() /
        // setEditMode() / etc. setters. Those setters each trigger their own rebuildMesh() (and
        // setEditMode additionally claims edit focus) - going through them here would mean up to
        // six redundant rebuilds and a focus steal on every single undo step. One rebuild at the
        // end, after every value is in place, is the entire reason this is its own code path
        // rather than a sequence of setter calls.
        this->boundaryWidth->setValue(width);
        this->boundaryHeight->setValue(height);
        this->boundaryDepth->setValue(depth);
        this->wallThickness->setValue(thickness);
        this->activated->setValue(activatedByte != 0);
        this->editMode->setListSelectedValue(editModeByte != 0 ? "Segment" : "Object");
        this->surfaceDatablock->setValue(surfaceDb);
        this->groundDatablock->setValue(groundDb);
        this->surfaceUVTiling->setValue(surfUV);
        this->groundUVTiling->setValue(groundUV);
        this->removedCells = restoredCells;

        if (true == this->activated->getBool())
        {
            this->rebuildMesh();
        }
        else
        {
            this->destroyBoundaryMesh();
        }

        this->scheduleSegmentOverlayUpdate();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPlatformBoundaryComponent] setPlatformData: restored " + Ogre::StringConverter::toString(width) + " x " + Ogre::StringConverter::toString(height) + " x " +
                                                                               Ogre::StringConverter::toString(depth) + ", " + Ogre::StringConverter::toString(numRemovedCells) + " cell(s) removed.");
    }

    bool ProceduralPlatformBoundaryComponent::getNearestPointOnPlatform(const Ogre::Vector3& worldPos, Ogre::Real maxRadius, Ogre::Vector3& outPoint) const
    {
        return false;
    }

    int ProceduralPlatformBoundaryComponent::getCellCount(BoundarySide side) const
    {
        // Floor and ceiling run the full width; the side walls fill only the gap between them, so
        // the four sides meet at the corners without overlapping. Counting in whole meters is what
        // makes a doorway snap to the grid by construction rather than by rounding a mouse
        // position after the fact.
        const Ogre::Real thickness = this->wallThickness->getReal();

        if (BoundarySide::FLOOR == side || BoundarySide::CEILING == side)
        {
            return std::max(1, static_cast<int>(std::floor(this->boundaryWidth->getReal() + 0.5f)));
        }

        const Ogre::Real innerHeight = this->boundaryHeight->getReal() - 2.0f * thickness;
        return std::max(1, static_cast<int>(std::floor(innerHeight + 0.5f)));
    }

    bool ProceduralPlatformBoundaryComponent::isCellRemoved(BoundarySide side, int index) const
    {
        BoundaryCell cell;
        cell.side = side;
        cell.index = index;
        return this->removedCells.find(cell) != this->removedCells.end();
    }

    unsigned int ProceduralPlatformBoundaryComponent::getRemovedCellCount(void) const
    {
        return static_cast<unsigned int>(this->removedCells.size());
    }

    void ProceduralPlatformBoundaryComponent::applyCellChange(const std::set<BoundaryCell>& newRemovedCells, const Ogre::String& transactionName)
    {
        if (newRemovedCells == this->removedCells)
        {
            return;
        }

        // BUGFIX: this used to snapshot serializeRemovedCells() - the plain "side:index ..." text
        // also used for the scene XML - as the transaction's old/new bytes. That round-trips fine
        // through the XML loader, but it is NOT the format setPlatformData understands, and
        // setPlatformData is what the generic undo/redo mechanism calls through a
        // PlatformComponentBase pointer. An undo step built that way would have handed
        // setPlatformData a buffer it would reject on the first magic/version check - silently
        // doing nothing, since setPlatformData logs and returns rather than crashing.
        //
        // getPlatformData()/setPlatformData() are the only pair guaranteed to agree on the format,
        // so they are the only pair used for the transaction bytes here.
        std::vector<unsigned char> oldData = this->getPlatformData();

        this->removedCells = newRemovedCells;

        this->rebuildMesh();
        this->scheduleSegmentOverlayUpdate();

        std::vector<unsigned char> newData = this->getPlatformData();

        boost::shared_ptr<EventDataCommandTransactionBegin> evtBegin(new EventDataCommandTransactionBegin(transactionName));
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evtBegin);

        boost::shared_ptr<EventDataBoundaryModifyEnd> evtMod(new EventDataBoundaryModifyEnd(oldData, newData, this->gameObjectPtr->getId()));
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evtMod);

        boost::shared_ptr<EventDataCommandTransactionEnd> evtEnd(new EventDataCommandTransactionEnd());
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evtEnd);
    }

    void ProceduralPlatformBoundaryComponent::removeCell(int side, int index)
    {
        if (side < 0 || side > 3)
        {
            return;
        }

        BoundaryCell cell;
        cell.side = static_cast<BoundarySide>(side);
        cell.index = index;

        if (cell.index < 0 || cell.index >= this->getCellCount(cell.side))
        {
            return;
        }

        std::set<BoundaryCell> newCells = this->removedCells;
        newCells.insert(cell);

        this->applyCellChange(newCells, "Cut Boundary Doorway");
    }

    void ProceduralPlatformBoundaryComponent::restoreCell(int side, int index)
    {
        if (side < 0 || side > 3)
        {
            return;
        }

        BoundaryCell cell;
        cell.side = static_cast<BoundarySide>(side);
        cell.index = index;

        std::set<BoundaryCell> newCells = this->removedCells;
        newCells.erase(cell);

        this->applyCellChange(newCells, "Close Boundary Doorway");
    }

    void ProceduralPlatformBoundaryComponent::clearAllCells(void)
    {
        std::set<BoundaryCell> newCells;
        this->applyCellChange(newCells, "Close All Boundary Doorways");
    }

    // =========================================================================================
    // Mesh generation
    // =========================================================================================

    void ProceduralPlatformBoundaryComponent::addBoundaryQuad(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& v3, const Ogre::Vector3& normal, Ogre::Real u0, Ogre::Real u1, Ogre::Real v0Coord,
        Ogre::Real v1Coord, BoundaryMeshBuffer targetBuffer)
    {
        std::vector<float>& verts = (BoundaryMeshBuffer::SURFACE == targetBuffer) ? this->surfaceVertices : this->groundVertices;
        std::vector<Ogre::uint32>& inds = (BoundaryMeshBuffer::SURFACE == targetBuffer) ? this->surfaceIndices : this->groundIndices;
        Ogre::uint32& currentIdx = (BoundaryMeshBuffer::SURFACE == targetBuffer) ? this->currentSurfaceVertexIndex : this->currentGroundVertexIndex;

        // Winding is derived from the normal rather than trusted from the caller. addBoundaryBox
        // emits six faces whose corner order follows the box's own axes, so half of them would
        // otherwise come out inside-out; deciding here means every call site can stay simple.
        const Ogre::Vector3 edge1 = v1 - v0;
        const Ogre::Vector3 edge2 = v3 - v0;
        const bool flip = edge1.crossProduct(edge2).dotProduct(normal) < 0.0f;

        const Ogre::Vector3 positions[4] = {v0, v1, v2, v3};
        const Ogre::Real uvs[4][2] = {{u0, v0Coord}, {u1, v0Coord}, {u1, v1Coord}, {u0, v1Coord}};

        for (int i = 0; i < 4; ++i)
        {
            verts.push_back(positions[i].x);
            verts.push_back(positions[i].y);
            verts.push_back(positions[i].z);
            verts.push_back(normal.x);
            verts.push_back(normal.y);
            verts.push_back(normal.z);
            verts.push_back(static_cast<float>(uvs[i][0]));
            verts.push_back(static_cast<float>(uvs[i][1]));
        }

        if (false == flip)
        {
            inds.push_back(currentIdx + 0);
            inds.push_back(currentIdx + 1);
            inds.push_back(currentIdx + 2);
            inds.push_back(currentIdx + 0);
            inds.push_back(currentIdx + 2);
            inds.push_back(currentIdx + 3);
        }
        else
        {
            inds.push_back(currentIdx + 0);
            inds.push_back(currentIdx + 2);
            inds.push_back(currentIdx + 1);
            inds.push_back(currentIdx + 0);
            inds.push_back(currentIdx + 3);
            inds.push_back(currentIdx + 2);
        }

        currentIdx += 4;
    }

    void ProceduralPlatformBoundaryComponent::addBoundaryBox(const Ogre::Vector3& minCorner, const Ogre::Vector3& maxCorner, const Ogre::Vector3& innerNormal)
    {
        // A run of consecutive kept cells is always exactly an axis-aligned box, which is why the
        // entire boundary can be built from this one primitive.
        //
        // The face whose outward normal matches innerNormal is the one pointing into the level -
        // what the player sees and walks on - so it goes to SURFACE. Everything else, including
        // the two faces that form the sides of a doorway, goes to GROUND. That is the same
        // Surface/Ground split ProceduralPlatformComponent uses, and it means a doorway's cut
        // edges automatically show rock rather than grass without any special case.
        const Ogre::Vector2 surfUV = this->surfaceUVTiling->getVector2();
        const Ogre::Vector2 groundUV = this->groundUVTiling->getVector2();

        struct Face
        {
            Ogre::Vector3 corners[4];
            Ogre::Vector3 normal;
            Ogre::Real uSpan;
            Ogre::Real vSpan;
        };

        const Ogre::Vector3& lo = minCorner;
        const Ogre::Vector3& hi = maxCorner;

        const Ogre::Real spanX = hi.x - lo.x;
        const Ogre::Real spanY = hi.y - lo.y;
        const Ogre::Real spanZ = hi.z - lo.z;

        const Face faces[6] = {// -X and +X: UVs run along Z and Y
            {{Ogre::Vector3(lo.x, lo.y, lo.z), Ogre::Vector3(lo.x, lo.y, hi.z), Ogre::Vector3(lo.x, hi.y, hi.z), Ogre::Vector3(lo.x, hi.y, lo.z)}, Ogre::Vector3(-1.0f, 0.0f, 0.0f), spanZ, spanY},
            {{Ogre::Vector3(hi.x, lo.y, lo.z), Ogre::Vector3(hi.x, lo.y, hi.z), Ogre::Vector3(hi.x, hi.y, hi.z), Ogre::Vector3(hi.x, hi.y, lo.z)}, Ogre::Vector3(1.0f, 0.0f, 0.0f), spanZ, spanY},
            // -Y and +Y: UVs run along X and Z
            {{Ogre::Vector3(lo.x, lo.y, lo.z), Ogre::Vector3(hi.x, lo.y, lo.z), Ogre::Vector3(hi.x, lo.y, hi.z), Ogre::Vector3(lo.x, lo.y, hi.z)}, Ogre::Vector3(0.0f, -1.0f, 0.0f), spanX, spanZ},
            {{Ogre::Vector3(lo.x, hi.y, lo.z), Ogre::Vector3(hi.x, hi.y, lo.z), Ogre::Vector3(hi.x, hi.y, hi.z), Ogre::Vector3(lo.x, hi.y, hi.z)}, Ogre::Vector3(0.0f, 1.0f, 0.0f), spanX, spanZ},
            // -Z and +Z: UVs run along X and Y
            {{Ogre::Vector3(lo.x, lo.y, lo.z), Ogre::Vector3(hi.x, lo.y, lo.z), Ogre::Vector3(hi.x, hi.y, lo.z), Ogre::Vector3(lo.x, hi.y, lo.z)}, Ogre::Vector3(0.0f, 0.0f, -1.0f), spanX, spanY},
            {{Ogre::Vector3(lo.x, lo.y, hi.z), Ogre::Vector3(hi.x, lo.y, hi.z), Ogre::Vector3(hi.x, hi.y, hi.z), Ogre::Vector3(lo.x, hi.y, hi.z)}, Ogre::Vector3(0.0f, 0.0f, 1.0f), spanX, spanY}};

        for (const Face& face : faces)
        {
            const bool isInnerFace = face.normal.dotProduct(innerNormal) > 0.5f;

            BoundaryMeshBuffer buffer = BoundaryMeshBuffer::GROUND;
            Ogre::Vector2 tiling = groundUV;
            if (true == isInnerFace)
            {
                buffer = BoundaryMeshBuffer::SURFACE;
                tiling = surfUV;
            }

            // UVs are derived from the face's real size in meters, so the texture density stays
            // constant whether a wall run is one meter long or ninety.
            this->addBoundaryQuad(face.corners[0], face.corners[1], face.corners[2], face.corners[3], face.normal, 0.0f, face.uSpan * tiling.x, 0.0f, face.vSpan * tiling.y, buffer);
        }
    }

    void ProceduralPlatformBoundaryComponent::rebuildMesh(void)
    {
        this->surfaceVertices.clear();
        this->surfaceIndices.clear();
        this->currentSurfaceVertexIndex = 0;

        this->groundVertices.clear();
        this->groundIndices.clear();
        this->currentGroundVertexIndex = 0;

        const Ogre::Real width = this->boundaryWidth->getReal();
        const Ogre::Real height = this->boundaryHeight->getReal();
        const Ogre::Real depth = this->boundaryDepth->getReal();
        Ogre::Real thickness = this->wallThickness->getReal();

        // Two walls have to fit inside the height with something left over, otherwise floor and
        // ceiling would overlap and the side walls would have negative length. Clamping here
        // rather than rejecting the value keeps a half-typed number in the editor from producing
        // broken geometry.
        thickness = std::min(thickness, std::min(width, height) * 0.4f);

        const Ogre::Real zFront = -depth * 0.5f;
        const Ogre::Real zBack = depth * 0.5f;

        // Emit one box per RUN of consecutive kept cells rather than one box per cell. Besides the
        // triangle saving, this is what makes a three-meter doorway read as a single opening:
        // without merging, every cell boundary would leave two coincident interior faces
        // z-fighting along the entire length of the level.
        auto emitRuns = [this](BoundarySide side, Ogre::Real cellSize, Ogre::Real spanStart, const Ogre::Vector3& innerNormal, std::function<void(Ogre::Real, Ogre::Real)> emitBox)
        {
            const int cellCount = this->getCellCount(side);

            int runStart = -1;
            for (int i = 0; i <= cellCount; ++i)
            {
                const bool kept = (i < cellCount) && (false == this->isCellRemoved(side, i));

                if (true == kept && runStart < 0)
                {
                    runStart = i;
                }
                else if (false == kept && runStart >= 0)
                {
                    emitBox(spanStart + static_cast<Ogre::Real>(runStart) * cellSize, spanStart + static_cast<Ogre::Real>(i) * cellSize);
                    runStart = -1;
                }
            }
        };

        // The cell grid is whole meters, but the boundary's dimensions need not be. The last cell
        // therefore absorbs the remainder, so the outer size stays exactly what was typed in -
        // which is the entire point of this component.
        const int floorCells = this->getCellCount(BoundarySide::FLOOR);
        const Ogre::Real floorCellSize = width / static_cast<Ogre::Real>(floorCells);

        const Ogre::Real innerBottom = thickness;
        const Ogre::Real innerTop = height - thickness;
        const int wallCells = this->getCellCount(BoundarySide::LEFT);
        const Ogre::Real wallCellSize = (innerTop - innerBottom) / static_cast<Ogre::Real>(wallCells);

        // ── Floor: inner face points up ──────────────────────────────────────────
        emitRuns(BoundarySide::FLOOR, floorCellSize, 0.0f, Ogre::Vector3::UNIT_Y,
            [this, thickness, zFront, zBack](Ogre::Real from, Ogre::Real to)
            {
                this->addBoundaryBox(Ogre::Vector3(from, 0.0f, zFront), Ogre::Vector3(to, thickness, zBack), Ogre::Vector3::UNIT_Y);
            });

        // ── Ceiling: inner face points down ──────────────────────────────────────
        emitRuns(BoundarySide::CEILING, floorCellSize, 0.0f, Ogre::Vector3::NEGATIVE_UNIT_Y,
            [this, height, thickness, zFront, zBack](Ogre::Real from, Ogre::Real to)
            {
                this->addBoundaryBox(Ogre::Vector3(from, height - thickness, zFront), Ogre::Vector3(to, height, zBack), Ogre::Vector3::NEGATIVE_UNIT_Y);
            });

        // ── Left wall: inner face points towards +X ──────────────────────────────
        emitRuns(BoundarySide::LEFT, wallCellSize, innerBottom, Ogre::Vector3::UNIT_X,
            [this, thickness, zFront, zBack](Ogre::Real from, Ogre::Real to)
            {
                this->addBoundaryBox(Ogre::Vector3(0.0f, from, zFront), Ogre::Vector3(thickness, to, zBack), Ogre::Vector3::UNIT_X);
            });

        // ── Right wall: inner face points towards -X ─────────────────────────────
        emitRuns(BoundarySide::RIGHT, wallCellSize, innerBottom, Ogre::Vector3::NEGATIVE_UNIT_X,
            [this, width, thickness, zFront, zBack](Ogre::Real from, Ogre::Real to)
            {
                this->addBoundaryBox(Ogre::Vector3(width - thickness, from, zFront), Ogre::Vector3(width, to, zBack), Ogre::Vector3::NEGATIVE_UNIT_X);
            });

        this->createBoundaryMesh();
    }

    void ProceduralPlatformBoundaryComponent::createBoundaryMesh(void)
    {
        if (0 == this->currentSurfaceVertexIndex && 0 == this->currentGroundVertexIndex)
        {
            this->destroyBoundaryMesh();
            return;
        }

        std::vector<float> surfaceVerticesCopy = this->surfaceVertices;
        std::vector<Ogre::uint32> surfaceIndicesCopy = this->surfaceIndices;
        const size_t numSurfaceVertices = this->currentSurfaceVertexIndex;

        std::vector<float> groundVerticesCopy = this->groundVertices;
        std::vector<Ogre::uint32> groundIndicesCopy = this->groundIndices;
        const size_t numGroundVertices = this->currentGroundVertexIndex;

        GraphicsModule::RenderCommand renderCommand = [this, surfaceVerticesCopy, surfaceIndicesCopy, numSurfaceVertices, groundVerticesCopy, groundIndicesCopy, numGroundVertices]()
        {
            this->createBoundaryMeshInternal(surfaceVerticesCopy, surfaceIndicesCopy, numSurfaceVertices, groundVerticesCopy, groundIndicesCopy, numGroundVertices);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralPlatformBoundaryComponent::createBoundaryMesh");

        this->updatePhysicsCollision();

        // The boundary's own extents just changed - a dimension, a doorway, an undo/redo. See
        // notifySceneBoundsDirty for why this does not compute the scene bounds itself.
        this->notifySceneBoundsDirty();
    }

    void ProceduralPlatformBoundaryComponent::createBoundaryMeshInternal(const std::vector<float>& surfaceVerts, const std::vector<Ogre::uint32>& surfaceInds, size_t numSurfaceVerts, const std::vector<float>& groundVerts,
        const std::vector<Ogre::uint32>& groundInds, size_t numGroundVerts)
    {
        //  RUNS ON RENDER THREAD!
        Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();
        Ogre::VaoManager* vaoManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getVaoManager();

        // Destroy first, then rebuild. The boundary is regenerated whole on every change (see the
        // file header), so there is never a partial-update path to get wrong.
        if (nullptr != this->boundaryItem)
        {
            this->gameObjectPtr->getSceneNode()->detachObject(this->boundaryItem);
            sceneManager->destroyItem(this->boundaryItem);
            this->boundaryItem = nullptr;

            // Counterpart to gameObjectPtr->init(item) further down: the game object still points
            // at the item about to be freed, and the selection ray would read released memory in
            // the window before the new item replaces it.
            this->gameObjectPtr->nullMovableObject();
        }

        {
            Ogre::ResourcePtr existing = Ogre::MeshManager::getSingleton().getByName(this->boundaryMeshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
            if (false == existing.isNull())
            {
                Ogre::MeshManager::getSingleton().remove(existing->getHandle());
            }
        }

        Ogre::MeshPtr mesh = Ogre::MeshManager::getSingleton().createManual(this->boundaryMeshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, &NOWA::gDummyMeshLoader);
        mesh->_setVaoManager(vaoManager);

        Ogre::Aabb bounds;
        Ogre::Vector3 aabbMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
        Ogre::Vector3 aabbMax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());

        // BUGFIX: the vertex declaration used to be position + normal + uv only, with the 8 floats
        // per vertex copied straight through. Any PBS datablock carrying a normal map then failed
        // with "Renderable can't use normal maps but datablock wants normal maps", Ogre fell back
        // to the default datablock, and the boundary rendered untextured - exactly what the log
        // showed for both grass_clean and rockClif_D.
        //
        // A normal map is sampled in tangent space, so the renderable has to supply VES_TANGENT.
        // The geometry buffers still hold 8 floats per vertex (pos, normal, uv); the tangent is
        // derived here while expanding them to 12, which is the same 8 -> 12 expansion
        // ProceduralPlatformComponent does in its own buildSubMesh - and the reason that component
        // never hit this.
        Ogre::VertexElement2Vec elements;
        elements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
        elements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));
        elements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT4, Ogre::VES_TANGENT));
        elements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));

        const size_t srcFloatsPerVertex = 8u;
        const size_t dstFloatsPerVertex = 12u;

        auto buildSubMesh = [&](const std::vector<float>& verts, const std::vector<Ogre::uint32>& inds, size_t vertexCount, const char* label)
        {
            // Only created when it actually has geometry - an empty submesh would need a dummy VAO
            // with zero indices, which is exactly the trap that broke MeshModifyComponent on the
            // platform component's dead junction buffer.
            if (0u == vertexCount || true == inds.empty())
            {
                return;
            }

            float* vertexData = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(vertexCount * dstFloatsPerVertex * sizeof(float), Ogre::MEMCATEGORY_GEOMETRY));

            for (size_t vi = 0u; vi < vertexCount; ++vi)
            {
                const size_t srcOffset = vi * srcFloatsPerVertex;
                const size_t dstOffset = vi * dstFloatsPerVertex;

                const Ogre::Vector3 position(verts[srcOffset + 0], verts[srcOffset + 1], verts[srcOffset + 2]);
                aabbMin.makeFloor(position);
                aabbMax.makeCeil(position);

                vertexData[dstOffset + 0] = position.x;
                vertexData[dstOffset + 1] = position.y;
                vertexData[dstOffset + 2] = position.z;

                const Ogre::Vector3 normal(verts[srcOffset + 3], verts[srcOffset + 4], verts[srcOffset + 5]);
                vertexData[dstOffset + 3] = normal.x;
                vertexData[dstOffset + 4] = normal.y;
                vertexData[dstOffset + 5] = normal.z;

                // Same tangent construction as ProceduralPlatformComponent: cross the normal with
                // world up, and fall back to world X for faces that are themselves nearly
                // horizontal, where world up would be parallel to the normal and the cross product
                // would collapse to zero. Every boundary face is axis aligned, so this yields a
                // clean tangent for all six directions of every box.
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
                vertexData[dstOffset + 9] = 1.0f; // handedness

                vertexData[dstOffset + 10] = verts[srcOffset + 6];
                vertexData[dstOffset + 11] = verts[srcOffset + 7];
            }

            Ogre::uint32* indexData = reinterpret_cast<Ogre::uint32*>(OGRE_MALLOC_SIMD(inds.size() * sizeof(Ogre::uint32), Ogre::MEMCATEGORY_GEOMETRY));
            memcpy(indexData, inds.data(), inds.size() * sizeof(Ogre::uint32));

            Ogre::VertexBufferPacked* vertexBuffer = nullptr;
            Ogre::IndexBufferPacked* indexBuffer = nullptr;

            try
            {
                vertexBuffer = vaoManager->createVertexBuffer(elements, vertexCount, Ogre::BT_IMMUTABLE, vertexData, true);
                indexBuffer = vaoManager->createIndexBuffer(Ogre::IndexBufferPacked::IT_32BIT, inds.size(), Ogre::BT_IMMUTABLE, indexData, true);
            }
            catch (const Ogre::Exception& e)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, Ogre::String("[ProceduralPlatformBoundaryComponent] Failed to create ") + label + " buffers: " + e.getDescription());
                return;
            }

            Ogre::VertexBufferPackedVec vertexBuffers;
            vertexBuffers.push_back(vertexBuffer);

            Ogre::VertexArrayObject* vao = vaoManager->createVertexArrayObject(vertexBuffers, indexBuffer, Ogre::OT_TRIANGLE_LIST);

            Ogre::SubMesh* subMesh = mesh->createSubMesh();
            subMesh->mVao[Ogre::VpNormal].push_back(vao);
            subMesh->mVao[Ogre::VpShadow].push_back(vao);
        };

        buildSubMesh(surfaceVerts, surfaceInds, numSurfaceVerts, "surface");
        buildSubMesh(groundVerts, groundInds, numGroundVerts, "ground");

        if (aabbMin.x > aabbMax.x)
        {
            aabbMin = aabbMax = Ogre::Vector3::ZERO;
        }

        bounds.setExtents(aabbMin, aabbMax);
        mesh->_setBounds(bounds, false);
        mesh->_setBoundingSphereRadius(bounds.getRadius());

        if (false == mesh->hasValidShadowMappingVaos())
        {
            mesh->prepareForShadowMapping(true);
        }

        this->boundaryItem = sceneManager->createItem(mesh, this->gameObjectPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC);
        this->boundaryItem->setName("ProceduralBoundaryItem_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()));
        this->boundaryItem->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);
        this->boundaryItem->setQueryFlags(this->gameObjectPtr->getCategoryId());
        this->boundaryItem->setCastShadows(true);

        const Ogre::String surfaceDbName = this->surfaceDatablock->getString();
        const Ogre::String groundDbName = this->groundDatablock->getString();

        // Submesh 0 is SURFACE and submesh 1 is GROUND, but only when both actually exist. A
        // boundary whose inner faces were all cut away would have just one - so the datablocks are
        // assigned by counting what was created rather than by assuming a fixed layout.
        unsigned int subItemIndex = 0u;
        if (0u != numSurfaceVerts && false == surfaceInds.empty())
        {
            if (false == surfaceDbName.empty() && subItemIndex < this->boundaryItem->getNumSubItems())
            {
                Ogre::HlmsDatablock* db = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(surfaceDbName);
                if (nullptr != db)
                {
                    this->boundaryItem->getSubItem(subItemIndex)->setDatablock(db);
                }
            }
            ++subItemIndex;
        }
        if (0u != numGroundVerts && false == groundInds.empty())
        {
            if (false == groundDbName.empty() && subItemIndex < this->boundaryItem->getNumSubItems())
            {
                Ogre::HlmsDatablock* db = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(groundDbName);
                if (nullptr != db)
                {
                    this->boundaryItem->getSubItem(subItemIndex)->setDatablock(db);
                }
            }
        }

        this->gameObjectPtr->getSceneNode()->attachObject(this->boundaryItem);

        // setDoNotDestroyMovableObject(true) goes with it: this component owns the item and
        // destroys it itself, so the GameObject must not also try to. Both calls, in this order,
        // are exactly what ProceduralPlatformComponent does after attaching its platform item.
        this->gameObjectPtr->setDoNotDestroyMovableObject(true);
        this->gameObjectPtr->init(this->boundaryItem);

        if (false == this->gameObjectPtr->isDynamic())
        {
            sceneManager->notifyStaticAabbDirty(this->boundaryItem);
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPlatformBoundaryComponent] Boundary mesh created: " + Ogre::StringConverter::toString(static_cast<unsigned int>(numSurfaceVerts)) + " surface vertices, " +
                                                                               Ogre::StringConverter::toString(static_cast<unsigned int>(numGroundVerts)) + " ground vertices, " +
                                                                               Ogre::StringConverter::toString(static_cast<unsigned int>(this->removedCells.size())) + " cells removed.");
    }

    void ProceduralPlatformBoundaryComponent::destroyBoundaryMesh(void)
    {
        // Captured before the render command runs, so the notify below fires only when there was
        // actually something removed - not on a no-op call where the item was already gone.
        const bool hadItem = (nullptr != this->boundaryItem);

        GraphicsModule::RenderCommand renderCommand = [this]()
        {
            if (nullptr == this->boundaryItem)
            {
                return;
            }

            Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();

            if (nullptr != this->gameObjectPtr->getSceneNode())
            {
                this->gameObjectPtr->getSceneNode()->detachObject(this->boundaryItem);
            }
            sceneManager->destroyItem(this->boundaryItem);
            this->boundaryItem = nullptr;
            this->gameObjectPtr->nullMovableObject();

            Ogre::ResourcePtr existing = Ogre::MeshManager::getSingleton().getByName(this->boundaryMeshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
            if (false == existing.isNull())
            {
                Ogre::MeshManager::getSingleton().remove(existing->getHandle());
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralPlatformBoundaryComponent::destroyBoundaryMesh");

        if (true == hadItem)
        {
            // The boundary just disappeared (deactivated, emptied by dimensions, or the component
            // is being removed) - the scene's overall extents may have shrunk.
            this->notifySceneBoundsDirty();
        }
    }

    void ProceduralPlatformBoundaryComponent::notifySceneBoundsDirty(void) const
    {
        // Guarded the same way scheduleSegmentOverlayUpdate already is elsewhere in this file: a
        // scene teardown destroys game objects one at a time, and each one's destruction would
        // otherwise fire a bounds update for a game object that is itself about to disappear.
        if (true == AppStateManager::getSingletonPtr()->getGameObjectController()->getIsDestroying())
        {
            return;
        }

        // BUGFIX (design correction): this used to fire a lightweight EventDataRecalculateSceneBounds
        // "please rescan the whole scene" request, on the assumption that only a full scan over every
        // game object could know the scene's overall extents. That is true in general, but not for
        // THIS component: a level boundary's whole purpose is to define what the level's extents ARE.
        // There is nothing to scan for - the boundary's own world-space box already is the answer,
        // authoritatively, which is exactly what the caller wants EventDataBoundsUpdated to carry.
        //
        // So this now fires that SAME event FollowCamera2D already listens for
        // (FollowCamera2D::handleUpdateBounds), computed directly from this component's own
        // dimensions and transform - the identical event, queued and followed by
        // Core::setCurrentSceneBounds(...), that a full scene scan would produce at load time. No new
        // event type, no round trip through code this component does not have.
        const Ogre::Real width = this->boundaryWidth->getReal();
        const Ogre::Real height = this->boundaryHeight->getReal();
        const Ogre::Real depth = this->boundaryDepth->getReal();

        // Local-space box, exactly the outer extents rebuildMesh() builds: x in [0, width], y in
        // [0, height], z in [-depth/2, +depth/2].
        const Ogre::Vector3 localMin(0.0f, 0.0f, -depth * 0.5f);
        const Ogre::Vector3 localMax(width, height, depth * 0.5f);

        const Ogre::Vector3 nodePosition = this->gameObjectPtr->getSceneNode()->_getDerivedPositionUpdated();
        const Ogre::Quaternion nodeOrientation = this->gameObjectPtr->getSceneNode()->_getDerivedOrientationUpdated();
        const Ogre::Vector3 nodeScale = this->gameObjectPtr->getSceneNode()->_getDerivedScaleUpdated();

        // All 8 corners of the local box are transformed rather than just min/max, because a rotated
        // boundary's world-space AABB is not simply the transform of its two local corners - the
        // other six corners can each individually end up on the true min/max face once rotation is
        // involved. A boundary is usually axis aligned, but nothing here assumes that.
        Ogre::Vector3 worldMin(std::numeric_limits<Ogre::Real>::max(), std::numeric_limits<Ogre::Real>::max(), std::numeric_limits<Ogre::Real>::max());
        Ogre::Vector3 worldMax(-std::numeric_limits<Ogre::Real>::max(), -std::numeric_limits<Ogre::Real>::max(), -std::numeric_limits<Ogre::Real>::max());

        for (int cx = 0; cx < 2; ++cx)
        {
            for (int cy = 0; cy < 2; ++cy)
            {
                for (int cz = 0; cz < 2; ++cz)
                {
                    const Ogre::Vector3 localCorner(0 == cx ? localMin.x : localMax.x, 0 == cy ? localMin.y : localMax.y, 0 == cz ? localMin.z : localMax.z);
                    const Ogre::Vector3 worldCorner = nodePosition + nodeOrientation * (localCorner * nodeScale);
                    worldMin.makeFloor(worldCorner);
                    worldMax.makeCeil(worldCorner);
                }
            }
        }

        boost::shared_ptr<EventDataBoundsUpdated> eventDataBoundsUpdated(boost::make_shared<EventDataBoundsUpdated>(worldMin, worldMax));
        AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataBoundsUpdated);
    }

    void ProceduralPlatformBoundaryComponent::updatePhysicsCollision(void)
    {
        if (nullptr == this->boundaryItem)
        {
            return;
        }

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
    }

    // =========================================================================================
    // Cell picking
    // =========================================================================================

    bool ProceduralPlatformBoundaryComponent::raycastBoundaryPlane(Ogre::Real screenX, Ogre::Real screenY, const Ogre::Vector3& planeNormal, Ogre::Real planeOffset, Ogre::Vector3& hitPosition) const
    {
        Ogre::Camera* camera = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
        if (nullptr == camera)
        {
            return false;
        }

        const Ogre::Ray ray = camera->getCameraToViewportRay(screenX, screenY);

        // The plane is expressed in the game object's LOCAL space, so the ray is transformed into
        // that space rather than the plane out of it. That keeps a rotated or moved boundary
        // working without a single extra case: the cell grid only ever exists in local meters.
        const Ogre::Vector3 localOrigin = this->gameObjectPtr->getSceneNode()->convertWorldToLocalPosition(ray.getOrigin());
        const Ogre::Vector3 localDirection = this->gameObjectPtr->getSceneNode()->convertWorldToLocalDirection(ray.getDirection(), false);

        const Ogre::Ray localRay(localOrigin, localDirection);
        const Ogre::Plane plane(planeNormal, planeOffset);

        const std::pair<bool, Ogre::Real> result = localRay.intersects(plane);
        if (false == result.first)
        {
            return false;
        }

        hitPosition = localRay.getPoint(result.second);
        return true;
    }

    bool ProceduralPlatformBoundaryComponent::pickCell(Ogre::Real screenX, Ogre::Real screenY, BoundaryCell& outCell) const
    {
        const Ogre::Real width = this->boundaryWidth->getReal();
        const Ogre::Real height = this->boundaryHeight->getReal();
        Ogre::Real thickness = std::min(this->wallThickness->getReal(), std::min(width, height) * 0.4f);

        const Ogre::Real innerBottom = thickness;
        const Ogre::Real innerTop = height - thickness;

        // Every side is tested against its own INNER plane - the face the designer is actually
        // looking at - and the nearest valid hit wins. Testing the inner faces rather than the
        // outer shell is what makes clicking do the obvious thing when the camera is inside the
        // level, which is where it always is while building one.
        struct SideTest
        {
            BoundarySide side;
            Ogre::Vector3 normal;
            Ogre::Real offset;
        };

        const SideTest tests[4] = {{BoundarySide::FLOOR, Ogre::Vector3::UNIT_Y, thickness}, {BoundarySide::CEILING, Ogre::Vector3::UNIT_Y, innerTop}, {BoundarySide::LEFT, Ogre::Vector3::UNIT_X, thickness},
            {BoundarySide::RIGHT, Ogre::Vector3::UNIT_X, width - thickness}};

        Ogre::Camera* camera = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
        if (nullptr == camera)
        {
            return false;
        }

        const Ogre::Vector3 localCameraPosition = this->gameObjectPtr->getSceneNode()->convertWorldToLocalPosition(camera->getDerivedPosition());

        bool found = false;
        Ogre::Real bestDistanceSq = std::numeric_limits<Ogre::Real>::max();

        for (const SideTest& test : tests)
        {
            Ogre::Vector3 hit;
            if (false == this->raycastBoundaryPlane(screenX, screenY, test.normal, test.offset, hit))
            {
                continue;
            }

            // Reject hits outside the side's own extent, so clicking past the end of a wall does
            // not silently select its last cell.
            int cellIndex = -1;

            if (BoundarySide::FLOOR == test.side || BoundarySide::CEILING == test.side)
            {
                if (hit.x < 0.0f || hit.x > width)
                {
                    continue;
                }
                const int cellCount = this->getCellCount(test.side);
                const Ogre::Real cellSize = width / static_cast<Ogre::Real>(cellCount);
                cellIndex = static_cast<int>(std::floor(hit.x / cellSize));
                cellIndex = Ogre::Math::Clamp(cellIndex, 0, cellCount - 1);
            }
            else
            {
                if (hit.y < innerBottom || hit.y > innerTop)
                {
                    continue;
                }
                const int cellCount = this->getCellCount(test.side);
                const Ogre::Real cellSize = (innerTop - innerBottom) / static_cast<Ogre::Real>(cellCount);
                cellIndex = static_cast<int>(std::floor((hit.y - innerBottom) / cellSize));
                cellIndex = Ogre::Math::Clamp(cellIndex, 0, cellCount - 1);
            }

            const Ogre::Real distanceSq = hit.squaredDistance(localCameraPosition);
            if (distanceSq < bestDistanceSq)
            {
                bestDistanceSq = distanceSq;
                outCell.side = test.side;
                outCell.index = cellIndex;
                found = true;
            }
        }

        return found;
    }

    void ProceduralPlatformBoundaryComponent::getCellOutline(const BoundaryCell& cell, std::vector<Ogre::Vector3>& outCorners) const
    {
        outCorners.clear();

        const Ogre::Real width = this->boundaryWidth->getReal();
        const Ogre::Real height = this->boundaryHeight->getReal();
        const Ogre::Real depth = this->boundaryDepth->getReal();
        const Ogre::Real thickness = std::min(this->wallThickness->getReal(), std::min(width, height) * 0.4f);

        // Nudged a little off the surface towards the level's interior, so the outline never
        // z-fights with the face it is outlining.
        const Ogre::Real push = 0.02f;

        const Ogre::Real zFront = -depth * 0.5f + push;
        const Ogre::Real zBack = depth * 0.5f - push;

        const Ogre::Real innerBottom = thickness;
        const Ogre::Real innerTop = height - thickness;

        if (BoundarySide::FLOOR == cell.side || BoundarySide::CEILING == cell.side)
        {
            const int cellCount = this->getCellCount(cell.side);
            const Ogre::Real cellSize = width / static_cast<Ogre::Real>(cellCount);
            const Ogre::Real from = static_cast<Ogre::Real>(cell.index) * cellSize;
            const Ogre::Real to = from + cellSize;

            Ogre::Real y = thickness + push;
            if (BoundarySide::CEILING == cell.side)
            {
                y = innerTop - push;
            }

            outCorners.push_back(Ogre::Vector3(from, y, zFront));
            outCorners.push_back(Ogre::Vector3(to, y, zFront));
            outCorners.push_back(Ogre::Vector3(to, y, zBack));
            outCorners.push_back(Ogre::Vector3(from, y, zBack));
        }
        else
        {
            const int cellCount = this->getCellCount(cell.side);
            const Ogre::Real cellSize = (innerTop - innerBottom) / static_cast<Ogre::Real>(cellCount);
            const Ogre::Real from = innerBottom + static_cast<Ogre::Real>(cell.index) * cellSize;
            const Ogre::Real to = from + cellSize;

            Ogre::Real x = thickness + push;
            if (BoundarySide::RIGHT == cell.side)
            {
                x = width - thickness - push;
            }

            outCorners.push_back(Ogre::Vector3(x, from, zFront));
            outCorners.push_back(Ogre::Vector3(x, to, zFront));
            outCorners.push_back(Ogre::Vector3(x, to, zBack));
            outCorners.push_back(Ogre::Vector3(x, from, zBack));
        }
    }

    // =========================================================================================
    // Selection overlay
    // =========================================================================================

    void ProceduralPlatformBoundaryComponent::createSegmentOverlay(void)
    {
        NOWA::GraphicsModule::RenderCommand cmd = [this]()
        {
            this->segOverlayNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();

            this->segOverlayObject = this->gameObjectPtr->getSceneManager()->createManualObject();
            this->segOverlayObject->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);
            this->segOverlayObject->setName("BoundarySegOverlay_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()));
            this->segOverlayObject->setQueryFlags(0u);
            this->segOverlayObject->setCastShadows(false);
            this->segOverlayNode->attachObject(this->segOverlayObject);
            this->segOverlayNode->setVisible(false);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "ProceduralPlatformBoundaryComponent::createSegmentOverlay");
    }

    void ProceduralPlatformBoundaryComponent::destroySegmentOverlay(void)
    {
        NOWA::GraphicsModule::RenderCommand cmd = [this]()
        {
            if (nullptr == this->segOverlayNode)
            {
                return;
            }
            this->segOverlayNode->detachAllObjects();
            if (nullptr != this->segOverlayObject)
            {
                this->gameObjectPtr->getSceneManager()->destroyManualObject(this->segOverlayObject);
                this->segOverlayObject = nullptr;
            }
            this->segOverlayNode->getParentSceneNode()->removeAndDestroyChild(this->segOverlayNode);
            this->segOverlayNode = nullptr;
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "ProceduralPlatformBoundaryComponent::destroySegmentOverlay");
    }

    void ProceduralPlatformBoundaryComponent::scheduleSegmentOverlayUpdate(void)
    {
        if (true == AppStateManager::getSingletonPtr()->getGameObjectController()->getIsDestroying())
        {
            return;
        }

        if (nullptr == this->segOverlayObject || nullptr == this->gameObjectPtr)
        {
            return;
        }

        const bool segmentMode = (EditMode::SEGMENT == this->getEditModeEnum());

        // BUGFIX: the overlay used to require a SELECTED cell, so with nothing selected yet -
        // which is the state Segment mode always starts in - absolutely nothing was drawn. There
        // was then no way to tell a working Segment mode from a broken one, and no indication of
        // where the one-meter cells actually sit before clicking.
        //
        // Now the whole cell grid is drawn as soon as Segment mode is active. That doubles as the
        // feedback that the mode is live at all.
        if (false == segmentMode)
        {
            NOWA::GraphicsModule::RenderCommand hideCmd = [this]()
            {
                if (nullptr != this->segOverlayObject)
                {
                    this->segOverlayObject->clear();
                }
                if (nullptr != this->segOverlayNode)
                {
                    this->segOverlayNode->setVisible(false);
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueue(std::move(hideCmd), "ProceduralPlatformBoundaryComponent::segOverlay_hide");
            return;
        }

        struct LV
        {
            Ogre::Vector3 pos;
            Ogre::ColourValue col;
        };
        std::vector<LV> lines;

        // Colour carries the state, so the designer can see at a glance what a key press will do:
        // dim grey for an ordinary solid cell, green for an existing doorway, amber for whatever
        // is selected right now.
        const Ogre::ColourValue colourGrid(0.35f, 0.35f, 0.35f, 1.0f);
        const Ogre::ColourValue colourRemoved(0.20f, 0.90f, 0.30f, 1.0f);
        const Ogre::ColourValue colourSelected(1.00f, 0.75f, 0.00f, 1.0f);

        auto addCellOutline = [this, &lines](const BoundaryCell& cell, const Ogre::ColourValue& colour, bool withDiagonals)
        {
            std::vector<Ogre::Vector3> corners;
            this->getCellOutline(cell, corners);

            for (size_t i = 0u; i < corners.size(); ++i)
            {
                const Ogre::Vector3& a = corners[i];
                const Ogre::Vector3& b = corners[(i + 1u) % corners.size()];
                lines.push_back({a, colour});
                lines.push_back({b, colour});
            }

            // Diagonals make a cell read as filled rather than as a floating rectangle when seen
            // edge-on - which is most of the time on a floor or ceiling cell.
            if (true == withDiagonals && 4u == corners.size())
            {
                lines.push_back({corners[0], colour});
                lines.push_back({corners[2], colour});
                lines.push_back({corners[1], colour});
                lines.push_back({corners[3], colour});
            }
        };

        const BoundarySide allSides[4] = {BoundarySide::FLOOR, BoundarySide::CEILING, BoundarySide::LEFT, BoundarySide::RIGHT};

        for (const BoundarySide side : allSides)
        {
            const int cellCount = this->getCellCount(side);
            for (int index = 0; index < cellCount; ++index)
            {
                BoundaryCell cell;
                cell.side = side;
                cell.index = index;

                if (true == this->hasSelectedCell && cell == this->selectedCell)
                {
                    // Drawn last instead, so the selection is never overdrawn by the grid.
                    continue;
                }

                if (true == this->isCellRemoved(side, index))
                {
                    addCellOutline(cell, colourRemoved, true);
                }
                else
                {
                    addCellOutline(cell, colourGrid, false);
                }
            }
        }

        if (true == this->hasSelectedCell)
        {
            addCellOutline(this->selectedCell, colourSelected, true);
        }

        const Ogre::Vector3 nodePosition = this->gameObjectPtr->getSceneNode()->_getDerivedPositionUpdated();
        const Ogre::Quaternion nodeOrientation = this->gameObjectPtr->getSceneNode()->_getDerivedOrientationUpdated();
        const Ogre::Vector3 nodeScale = this->gameObjectPtr->getSceneNode()->_getDerivedScaleUpdated();

        NOWA::GraphicsModule::RenderCommand drawCmd = [this, lines = std::move(lines), nodePosition, nodeOrientation, nodeScale]()
        {
            if (nullptr == this->segOverlayObject)
            {
                return;
            }
            this->segOverlayObject->clear();

            if (true == lines.empty())
            {
                if (nullptr != this->segOverlayNode)
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
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPlatformBoundaryComponent] Overlay begin() FAILED: " + e.getDescription());
            }

            if (nullptr != this->segOverlayNode)
            {
                // The overlay node hangs off the scene root rather than the game object, so it has
                // to be given the object's full transform: the outline is built in local meters,
                // exactly like the boundary itself.
                this->segOverlayNode->setPosition(nodePosition);
                this->segOverlayNode->setOrientation(nodeOrientation);
                this->segOverlayNode->setScale(nodeScale);
                this->segOverlayNode->setVisible(true);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(drawCmd), "ProceduralPlatformBoundaryComponent::segOverlay_draw");
    }

    // =========================================================================================
    // Editor state and input
    // =========================================================================================

    ProceduralPlatformBoundaryComponent::EditMode ProceduralPlatformBoundaryComponent::getEditModeEnum(void) const
    {
        if ("Segment" == this->editMode->getListSelectedValue())
        {
            return EditMode::SEGMENT;
        }
        return EditMode::OBJECT;
    }

    void ProceduralPlatformBoundaryComponent::addInputListener(void)
    {
        const Ogre::String listenerName = ProceduralPlatformBoundaryComponent::getStaticClassName() + "_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
        if (auto* core = InputDeviceCore::getSingletonPtr())
        {
            core->addKeyListener(this, listenerName);
            core->addMouseListener(this, listenerName);
        }
    }

    void ProceduralPlatformBoundaryComponent::removeInputListener(void)
    {
        const Ogre::String listenerName = ProceduralPlatformBoundaryComponent::getStaticClassName() + "_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
        if (auto* core = InputDeviceCore::getSingletonPtr())
        {
            core->removeKeyListener(listenerName);
            core->removeMouseListener(listenerName);
        }
    }

    void ProceduralPlatformBoundaryComponent::claimEditFocus(void)
    {
        // Same mechanism as ProceduralPlatformComponent::claimEditFocus - one event type, one
        // handler, one truth. Several editing components can sit on the same game object, and
        // whichever one's modify-setter the designer touches last owns the mouse.
        unsigned short manipulationMode = NOWA::EditorManager::EDITOR_SELECT_MODE;
        if (EditMode::SEGMENT == this->getEditModeEnum())
        {
            manipulationMode = NOWA::EditorManager::EDITOR_MESH_MODIFY_MODE;
        }

        boost::shared_ptr<EventDataEditorMode> eventDataEditorMode(new EventDataEditorMode(manipulationMode, this->gameObjectPtr->getId(), ProceduralPlatformBoundaryComponent::getStaticClassName()));
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataEditorMode);
    }

    bool ProceduralPlatformBoundaryComponent::isEditFocusOwner(void) const
    {
        // Empty means nobody has claimed editing on this game object, which is the normal case for
        // an object carrying a single editing component. Defaulting to "yes" there is what keeps a
        // lone boundary component working; the rule only bites once someone actually claims.
        if (true == this->editFocusOwner.empty())
        {
            return true;
        }
        return this->editFocusOwner == ProceduralPlatformBoundaryComponent::getStaticClassName();
    }

    void ProceduralPlatformBoundaryComponent::handleMeshModifyMode(NOWA::EventDataPtr eventData)
    {
        auto castEventData = boost::static_pointer_cast<EventDataEditorMode>(eventData);

        this->isEditorMeshModifyMode = (castEventData->getManipulationMode() == EditorManager::EDITOR_MESH_MODIFY_MODE);

        // Only a CLAIMING event carries an owner. Events built with the plain single-argument
        // constructor - EditorManager's own mode switches among them - must leave the recorded
        // owner alone, or every ordinary mode change would silently reassign editing.
        if (true == castEventData->hasEditFocusClaim() && castEventData->getGameObjectId() == this->gameObjectPtr->getId())
        {
            this->editFocusOwner = castEventData->getComponentClassName();
        }

        this->updateModificationState();
    }

    void ProceduralPlatformBoundaryComponent::handleGameObjectSelected(NOWA::EventDataPtr eventData)
    {
        auto castEventData = boost::static_pointer_cast<EventDataGameObjectSelected>(eventData);

        if (castEventData->getGameObjectId() == this->gameObjectPtr->getId())
        {
            this->isSelected = castEventData->getIsSelected();
            if (false == this->isSelected)
            {
                // Deselecting leaves Segment mode as well. A doorway cannot be cut on an object
                // that is not selected, so staying in Segment mode would only show a mode the
                // designer cannot act on.
                this->setEditMode("Object");
                return;
            }
        }
        else if (true == castEventData->getIsSelected())
        {
            // Another object was selected, so this one no longer is.
            this->isSelected = false;
        }

        // BUGFIX: selecting the object while Segment mode was already set did not put the EDITOR
        // into mesh modify mode. isEditorMeshModifyMode therefore stayed false and
        // updateModificationState refused to attach the input listener - Segment mode looked
        // switched on in the properties while nothing responded to clicks. Same nudge
        // ProceduralPlatformComponent gives in its own handler.
        if (false == castEventData->getIsPartOfMultiSelection())
        {
            if (EditMode::SEGMENT == this->getEditModeEnum())
            {
                boost::shared_ptr<EventDataEditorMode> eventDataEditorMode(new EventDataEditorMode(NOWA::EditorManager::EDITOR_MESH_MODIFY_MODE, this->gameObjectPtr->getId(), ProceduralPlatformBoundaryComponent::getStaticClassName()));
                NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataEditorMode);
            }
        }

        this->updateModificationState();
    }

    void ProceduralPlatformBoundaryComponent::updateModificationState(void)
    {
        // Same diagnostic line ProceduralPlatformComponent has, and for the same reason: when
        // Segment mode "does nothing", exactly one of these four flags is false, and this line
        // says which one without any guessing.
        /*Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPlatformBoundaryComponent] updateModificationState: activated=" + Ogre::StringConverter::toString(this->activated->getBool()) +
                                                                               " meshModifyMode=" + Ogre::StringConverter::toString(this->isEditorMeshModifyMode) + " selected=" + Ogre::StringConverter::toString(this->isSelected) +
                                                                               " focusOwner=" + Ogre::StringConverter::toString(this->isEditFocusOwner()) + " editMode=" + this->editMode->getListSelectedValue());*/

        // BUGFIX: this used to require EditMode::SEGMENT as a fifth condition. That looked
        // harmless - there is nothing to do in Object mode anyway - but it made the component
        // deaf at exactly the wrong moment. claimEditFocus() QUEUES its EventDataEditorMode, so
        // when the designer switches to Segment the sequence is: editMode becomes Segment ->
        // updateModificationState runs while isEditorMeshModifyMode is still false -> no listener
        // -> and the only thing that would re-run this is handleMeshModifyMode, which fires a
        // frame later. If anything in between reset the state, the listener was never attached at
        // all, and no click ever reached pickCell.
        //
        // ProceduralPlatformComponent gates on four conditions, not five, and lets its input
        // handlers decide what to do per edit mode. mousePressed and keyPressed here already do
        // exactly that - both return early unless Segment mode is active - so dropping the
        // condition costs nothing and removes the ordering trap.
        const bool shouldBeActive = this->activated->getBool() && this->isEditorMeshModifyMode && this->isSelected && this->isEditFocusOwner();

        if (true == shouldBeActive)
        {
            this->addInputListener();

            // Refresh the overlay whenever the component becomes active - covers the case where
            // the designer set Segment mode FIRST and only then clicked into mesh modify mode.
            this->scheduleSegmentOverlayUpdate();
        }
        else
        {
            this->removeInputListener();

            // Drop the selection along with the input. Leaving a highlighted cell behind after the
            // component stops listening would show a selection that no key can act on.
            if (true == this->hasSelectedCell)
            {
                this->hasSelectedCell = false;
            }
            this->scheduleSegmentOverlayUpdate();
        }
    }

    bool ProceduralPlatformBoundaryComponent::mouseMoved(const OIS::MouseEvent& evt)
    {
        return true;
    }

    bool ProceduralPlatformBoundaryComponent::mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
    {
        if (false == this->activated->getBool() || EditMode::SEGMENT != this->getEditModeEnum())
        {
            return true;
        }

        if (OIS::MB_Left != id)
        {
            return true;
        }

        if (nullptr != NOWA::GraphicsModule::getInstance()->getMyGUIFocusWidget())
        {
            return true;
        }

        Ogre::Real screenX = 0.0f;
        Ogre::Real screenY = 0.0f;
        MathHelper::getInstance()->mouseToViewPort(evt.state.X.abs, evt.state.Y.abs, screenX, screenY, Core::getSingletonPtr()->getOgreRenderWindow());

        BoundaryCell cell;
        if (true == this->pickCell(screenX, screenY, cell))
        {
            this->selectedCell = cell;
            this->hasSelectedCell = true;
            this->scheduleSegmentOverlayUpdate();
            return false;
        }

        // Clicking past the boundary clears the selection, which is the least surprising way to
        // deselect. Logged because "click does nothing" and "click missed the boundary" look
        // identical on screen but have completely different causes.
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
            "[ProceduralPlatformBoundaryComponent] Click at " + Ogre::StringConverter::toString(screenX) + ", " + Ogre::StringConverter::toString(screenY) + " did not hit any boundary cell.");

        if (true == this->hasSelectedCell)
        {
            this->hasSelectedCell = false;
            this->scheduleSegmentOverlayUpdate();
        }

        return true;
    }

    bool ProceduralPlatformBoundaryComponent::mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
    {
        return true;
    }

    bool ProceduralPlatformBoundaryComponent::keyPressed(const OIS::KeyEvent& evt)
    {
        if (OIS::KC_LSHIFT == evt.key || OIS::KC_RSHIFT == evt.key)
        {
            this->isShiftKeyDown = true;
            return true;
        }

        if (false == this->activated->getBool() || EditMode::SEGMENT != this->getEditModeEnum())
        {
            return true;
        }

        if (OIS::KC_X == evt.key && true == this->hasSelectedCell)
        {
            // X cuts, SHIFT+X closes. One key for both directions, because a doorway is edited by
            // looking at it and toggling, not by remembering two separate bindings.
            if (true == this->isShiftKeyDown)
            {
                this->restoreCell(static_cast<int>(this->selectedCell.side), this->selectedCell.index);
            }
            else
            {
                this->removeCell(static_cast<int>(this->selectedCell.side), this->selectedCell.index);
            }
            return false;
        }

        if (OIS::KC_ESCAPE == evt.key && true == this->hasSelectedCell)
        {
            this->hasSelectedCell = false;
            this->scheduleSegmentOverlayUpdate();
            return false;
        }

        return true;
    }

    bool ProceduralPlatformBoundaryComponent::keyReleased(const OIS::KeyEvent& evt)
    {
        if (OIS::KC_LSHIFT == evt.key || OIS::KC_RSHIFT == evt.key)
        {
            this->isShiftKeyDown = false;
        }
        return true;
    }

    // =========================================================================================
    // Setters and getters
    // =========================================================================================

    void ProceduralPlatformBoundaryComponent::setActivated(bool activated)
    {
        this->activated->setValue(activated);

        if (true == activated)
        {
            this->rebuildMesh();
        }
        else
        {
            this->destroyBoundaryMesh();
        }

        this->updateModificationState();
    }

    bool ProceduralPlatformBoundaryComponent::isActivated(void) const
    {
        return this->activated->getBool();
    }

    void ProceduralPlatformBoundaryComponent::setBoundaryWidth(Ogre::Real width)
    {
        this->boundaryWidth->setValue(Ogre::Math::Clamp(width, 1.0f, 10000.0f));

        // Changing a dimension throws the whole boundary away and regenerates it, by design.
        // Removed cells are kept: a doorway is remembered as "floor, cell 12", so it survives the
        // level being made taller or deeper. Cells that fall outside the new size are simply never
        // emitted - they come back if the size is increased again, which is far friendlier than
        // silently discarding them on a mistyped number.
        this->rebuildMesh();
        this->scheduleSegmentOverlayUpdate();
    }

    Ogre::Real ProceduralPlatformBoundaryComponent::getBoundaryWidth(void) const
    {
        return this->boundaryWidth->getReal();
    }

    void ProceduralPlatformBoundaryComponent::setBoundaryHeight(Ogre::Real height)
    {
        this->boundaryHeight->setValue(Ogre::Math::Clamp(height, 1.0f, 10000.0f));
        this->rebuildMesh();
        this->scheduleSegmentOverlayUpdate();
    }

    Ogre::Real ProceduralPlatformBoundaryComponent::getBoundaryHeight(void) const
    {
        return this->boundaryHeight->getReal();
    }

    void ProceduralPlatformBoundaryComponent::setBoundaryDepth(Ogre::Real depth)
    {
        this->boundaryDepth->setValue(Ogre::Math::Clamp(depth, 0.1f, 1000.0f));
        this->rebuildMesh();
        this->scheduleSegmentOverlayUpdate();
    }

    Ogre::Real ProceduralPlatformBoundaryComponent::getBoundaryDepth(void) const
    {
        return this->boundaryDepth->getReal();
    }

    void ProceduralPlatformBoundaryComponent::setWallThickness(Ogre::Real thickness)
    {
        this->wallThickness->setValue(Ogre::Math::Clamp(thickness, 0.05f, 100.0f));
        this->rebuildMesh();
        this->scheduleSegmentOverlayUpdate();
    }

    Ogre::Real ProceduralPlatformBoundaryComponent::getWallThickness(void) const
    {
        return this->wallThickness->getReal();
    }

    void ProceduralPlatformBoundaryComponent::setSurfaceDatablock(const Ogre::String& datablock)
    {
        this->surfaceDatablock->setValue(datablock);
        this->rebuildMesh();
    }

    Ogre::String ProceduralPlatformBoundaryComponent::getSurfaceDatablock(void) const
    {
        return this->surfaceDatablock->getString();
    }

    void ProceduralPlatformBoundaryComponent::setGroundDatablock(const Ogre::String& datablock)
    {
        this->groundDatablock->setValue(datablock);
        this->rebuildMesh();
    }

    Ogre::String ProceduralPlatformBoundaryComponent::getGroundDatablock(void) const
    {
        return this->groundDatablock->getString();
    }

    void ProceduralPlatformBoundaryComponent::setSurfaceUVTiling(const Ogre::Vector2& tiling)
    {
        this->surfaceUVTiling->setValue(tiling);
        this->rebuildMesh();
    }

    Ogre::Vector2 ProceduralPlatformBoundaryComponent::getSurfaceUVTiling(void) const
    {
        return this->surfaceUVTiling->getVector2();
    }

    void ProceduralPlatformBoundaryComponent::setGroundUVTiling(const Ogre::Vector2& tiling)
    {
        this->groundUVTiling->setValue(tiling);
        this->rebuildMesh();
    }

    Ogre::Vector2 ProceduralPlatformBoundaryComponent::getGroundUVTiling(void) const
    {
        return this->groundUVTiling->getVector2();
    }

    void ProceduralPlatformBoundaryComponent::setEditMode(const Ogre::String& editMode)
    {
        this->editMode->setListSelectedValue(editMode);

        this->hasSelectedCell = false;

        // This component's modify-setter, so this is where it takes editing back from whichever
        // sibling had it - and, just as importantly, where the EDITOR is pushed into mesh modify
        // mode so segments can be picked straight away. claimEditFocus sends exactly that event,
        // with EDITOR_MESH_MODIFY_MODE when Segment is selected and EDITOR_SELECT_MODE otherwise.
        this->claimEditFocus();

        // BUGFIX: isEditorMeshModifyMode is set from the queued event above, which does not arrive
        // until the next event pump. Waiting for it means the listener is attached a frame late at
        // best, and never at all if the editor was ALREADY in mesh modify mode - in that case no
        // new mode event is broadcast, so handleMeshModifyMode never fires and the flag would stay
        // false forever. Setting it here from what was just requested makes the switch take effect
        // immediately; the event that follows simply confirms the same value.
        if (EditMode::SEGMENT == this->getEditModeEnum())
        {
            this->isEditorMeshModifyMode = true;
        }

        this->updateModificationState();
    }

    Ogre::String ProceduralPlatformBoundaryComponent::getEditMode(void) const
    {
        return this->editMode->getListSelectedValue();
    }

    // =========================================================================================
    // Lua API
    // =========================================================================================

    ProceduralPlatformBoundaryComponent* getProceduralPlatformBoundaryComponent(GameObject* gameObject, unsigned int occurrenceIndex)
    {
        return makeStrongPtr<ProceduralPlatformBoundaryComponent>(gameObject->getComponentWithOccurrence<ProceduralPlatformBoundaryComponent>(occurrenceIndex)).get();
    }

    ProceduralPlatformBoundaryComponent* getProceduralPlatformBoundaryComponent(GameObject* gameObject)
    {
        return makeStrongPtr<ProceduralPlatformBoundaryComponent>(gameObject->getComponent<ProceduralPlatformBoundaryComponent>()).get();
    }

    ProceduralPlatformBoundaryComponent* getProceduralPlatformBoundaryComponentFromName(GameObject* gameObject, const Ogre::String& name)
    {
        return makeStrongPtr<ProceduralPlatformBoundaryComponent>(gameObject->getComponentFromName<ProceduralPlatformBoundaryComponent>(name)).get();
    }

    void ProceduralPlatformBoundaryComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
    {
        module(lua)
        [
            class_<ProceduralPlatformBoundaryComponent, GameObjectComponent>("ProceduralPlatformBoundaryComponent")
            .def("setActivated", &ProceduralPlatformBoundaryComponent::setActivated)
            .def("isActivated", &ProceduralPlatformBoundaryComponent::isActivated)
            .def("setBoundaryWidth", &ProceduralPlatformBoundaryComponent::setBoundaryWidth)
            .def("getBoundaryWidth", &ProceduralPlatformBoundaryComponent::getBoundaryWidth)
            .def("setBoundaryHeight", &ProceduralPlatformBoundaryComponent::setBoundaryHeight)
            .def("getBoundaryHeight", &ProceduralPlatformBoundaryComponent::getBoundaryHeight)
            .def("setBoundaryDepth", &ProceduralPlatformBoundaryComponent::setBoundaryDepth)
            .def("getBoundaryDepth", &ProceduralPlatformBoundaryComponent::getBoundaryDepth)
            .def("setWallThickness", &ProceduralPlatformBoundaryComponent::setWallThickness)
            .def("getWallThickness", &ProceduralPlatformBoundaryComponent::getWallThickness)
            .def("setSurfaceDatablock", &ProceduralPlatformBoundaryComponent::setSurfaceDatablock)
            .def("getSurfaceDatablock", &ProceduralPlatformBoundaryComponent::getSurfaceDatablock)
            .def("setGroundDatablock", &ProceduralPlatformBoundaryComponent::setGroundDatablock)
            .def("getGroundDatablock", &ProceduralPlatformBoundaryComponent::getGroundDatablock)
            .def("removeCell", &ProceduralPlatformBoundaryComponent::removeCell)
            .def("restoreCell", &ProceduralPlatformBoundaryComponent::restoreCell)
            .def("clearAllCells", &ProceduralPlatformBoundaryComponent::clearAllCells)
            .def("getRemovedCellCount", &ProceduralPlatformBoundaryComponent::getRemovedCellCount)
        ];

        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPlatformBoundaryComponent", "class inherits GameObjectComponent", ProceduralPlatformBoundaryComponent::getStaticInfoText());

        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPlatformBoundaryComponent", "void setBoundaryWidth(float width)", "Sets the level length in meters. Regenerates the whole boundary.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPlatformBoundaryComponent", "float getBoundaryWidth()", "Gets the level length in meters.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPlatformBoundaryComponent", "void setBoundaryHeight(float height)", "Sets the level height in meters. Regenerates the whole boundary.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPlatformBoundaryComponent", "float getBoundaryHeight()", "Gets the level height in meters.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPlatformBoundaryComponent", "void setBoundaryDepth(float depth)", "Sets the level depth in meters. Regenerates the whole boundary.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPlatformBoundaryComponent", "float getBoundaryDepth()", "Gets the level depth in meters.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPlatformBoundaryComponent", "void setWallThickness(float thickness)", "Sets how solid floor, ceiling and walls are, in meters.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPlatformBoundaryComponent", "float getWallThickness()", "Gets the wall thickness in meters.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPlatformBoundaryComponent", "void setSurfaceDatablock(String datablock)", "Sets the datablock for the faces pointing into the level.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPlatformBoundaryComponent", "String getSurfaceDatablock()", "Gets the inner faces datablock.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPlatformBoundaryComponent", "void setGroundDatablock(String datablock)", "Sets the datablock for the outer shell and doorway cut faces.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPlatformBoundaryComponent", "String getGroundDatablock()", "Gets the outer shell datablock.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPlatformBoundaryComponent", "void removeCell(int side, int index)",
            "Cuts a one-meter doorway cell. side: 0 = floor, 1 = ceiling, 2 = left wall, 3 = right wall. index counts in meters from the origin. "
            "Example: boundaryComp:removeCell(3, 5) -- opening in the right wall, 5 meters up");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPlatformBoundaryComponent", "void restoreCell(int side, int index)", "Closes a previously cut doorway cell again.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPlatformBoundaryComponent", "void clearAllCells()", "Closes every doorway, making the boundary solid again.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPlatformBoundaryComponent", "unsigned int getRemovedCellCount()", "Gets how many cells are currently cut away.");

        gameObjectClass.def("getProceduralPlatformBoundaryComponentFromName", &getProceduralPlatformBoundaryComponentFromName);
        gameObjectClass.def("getProceduralPlatformBoundaryComponent", (ProceduralPlatformBoundaryComponent * (*)(GameObject*)) & getProceduralPlatformBoundaryComponent);
        gameObjectClass.def("getProceduralPlatformBoundaryComponent2", (ProceduralPlatformBoundaryComponent * (*)(GameObject*, unsigned int)) & getProceduralPlatformBoundaryComponent);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ProceduralPlatformBoundaryComponent getProceduralPlatformBoundaryComponent()", "Gets the component. Use this if the game object has this component only once.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ProceduralPlatformBoundaryComponent getProceduralPlatformBoundaryComponent2(unsigned int occurrenceIndex)",
            "Gets the component by the given occurrence index, since a game object may have this component several times.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ProceduralPlatformBoundaryComponent getProceduralPlatformBoundaryComponentFromName(String name)", "Gets the component by its custom name.");

        gameObjectControllerClass.def("castProceduralPlatformBoundaryComponent", &GameObjectController::cast<ProceduralPlatformBoundaryComponent>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "ProceduralPlatformBoundaryComponent castProceduralPlatformBoundaryComponent(ProceduralPlatformBoundaryComponent other)",
            "Casts an incoming type from function for lua auto completion.");
    }

}; // namespace end
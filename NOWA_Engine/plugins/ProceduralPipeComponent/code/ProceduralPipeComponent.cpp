/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#include "NOWAPrecompiled.h"
#include "ProceduralPipeComponent.h"
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

// =============================================================================
// ProceduralPipeComponent - a tube swept along the same 2.5D path model
// ProceduralPlatformComponent uses. Read that component first if this one is unfamiliar:
// everything about the path, the editor interaction and the persistence is deliberately the
// same, down to the byte layout of the "Path Data" property, so the two stay comparable.
//
// What is actually new here:
//   - generatePipeRings: a ring sweep with mitered in-plane normals, an optional inner shell
//     for wall thickness, and closing rings at the chain ends.
//   - The NEAR/FAR submesh split by radial angle, which is what lets the arc between camera
//     and player be shaded separately, faded, or dropped entirely.
//   - pathSamples + the Lua path query API, so a script can drive or track the player along
//     the tube.
//
// Deliberately NOT carried over from the platform component:
//   - Junctions. Three tubes meeting needs a real hub patch on a round cross-section; arms
//     that simply interpenetrate look fine for a slab and wrong for a pipe.
//   - Grass, trees, styles, convert-to-mesh, cross-network merging of two pipe GameObjects.
//   - Hairpin run splitting. The ring sweep handles any turn the miter can, and a true 180
//     degree reversal is a modelling mistake in a pipe rather than something to support.
// =============================================================================

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    ProceduralPipeComponent::ProceduralPipeComponent() :
        PlatformComponentBase(),
        name("ProceduralPipeComponent"),
        activated(new Variant(ProceduralPipeComponent::AttrActivated(), true, this->attributes)),
        pipeRadius(new Variant(ProceduralPipeComponent::AttrPipeRadius(), 2.0f, this->attributes)),
        wallThickness(new Variant(ProceduralPipeComponent::AttrWallThickness(), 0.25f, this->attributes)),
        radialSegments(new Variant(ProceduralPipeComponent::AttrRadialSegments(), 16, this->attributes)),
        nearSideMode(new Variant(ProceduralPipeComponent::AttrNearSideMode(), std::vector<Ogre::String>{"Solid", "Transparent", "Hidden"}, this->attributes)),
        nearSideArc(new Variant(ProceduralPipeComponent::AttrNearSideArc(), 140.0f, this->attributes)),
        junctionHubScale(new Variant(ProceduralPipeComponent::AttrJunctionHubScale(), 1.15f, this->attributes)),
        nearSideAlpha(new Variant(ProceduralPipeComponent::AttrNearSideAlpha(), 0.5f, this->attributes)),
        invertNearSide(new Variant(ProceduralPipeComponent::AttrInvertNearSide(), false, this->attributes)),
        snapToGrid(new Variant(ProceduralPipeComponent::AttrSnapToGrid(), false, this->attributes)),
        gridSize(new Variant(ProceduralPipeComponent::AttrGridSize(), 1.0f, this->attributes)),
        smoothingFactor(new Variant(ProceduralPipeComponent::AttrSmoothingFactor(), 0.5f, this->attributes)),
        curveSubdivisions(new Variant(ProceduralPipeComponent::AttrCurveSubdivisions(), 10, this->attributes)),
        pipeDatablock(new Variant(ProceduralPipeComponent::AttrPipeDatablock(), Ogre::String("city_roof_01"), this->attributes)),
        nearDatablock(new Variant(ProceduralPipeComponent::AttrNearDatablock(), Ogre::String("city_roof_01"), this->attributes)),
        pipeUVTiling(new Variant(ProceduralPipeComponent::AttrPipeUVTiling(), Ogre::Vector2(0.01f, 0.01f), this->attributes)),
        editMode(new Variant(ProceduralPipeComponent::AttrEditMode(), std::vector<Ogre::String>{"Object", "Segment"}, this->attributes)),
        buildState(BuildState::IDLE),
        isEditorMeshModifyMode(false),
        isSelected(false),
        currentFarVertexIndex(0),
        currentNearVertexIndex(0),
        pipeItem(nullptr),
        previewItem(nullptr),
        previewNode(nullptr),
        isShiftPressed(true),
        isShiftKeyDown(false),
        isCtrlPressed(false),
        hasPipeOrigin(false),
        pipeFrame(Ogre::Quaternion::IDENTITY),
        cachedNumFarVertices(0),
        cachedNumNearVertices(0),
        originPositionSet(false),
        hasLoadedPipeEndpoint(false),
        loadedPipeEndpointHeight(0.0f),
        selectedSegmentIndex(-1),
        segOverlayNode(nullptr),
        segOverlayObject(nullptr),
        isExtendingFromSegment(false),
        isSnapToOwnPipe(false),
        snapToPipeSegmentIdx(-1),
        snapRadius(0.0f),
        pipeLoadedFromScene(false),
        pipeClonedNeedsRebuild(false),
        physicsArtifactComponent(nullptr)
    {
        this->pipeRadius->setDescription("Outer radius of the tube in meters. Also the step size for the U / SHIFT+U depth nudge.");
        this->pipeRadius->setConstraints(0.05f, 200.0f);

        this->wallThickness->setDescription("Thickness of the pipe wall in meters. Greater than 0 builds a second, inward-facing shell plus "
                                            "closing rings at both ends: the inside is then correctly lit without two-sided tricks, the pipe "
                                            "ends look solid instead of paper-thin, and the physics collision gets a real inner surface for "
                                            "the player to run on. 0 builds a single shell - cheaper, but the interior is backfacing.");
        this->wallThickness->setConstraints(0.0f, 50.0f);

        this->radialSegments->setDescription("Vertices per ring. 8 is faceted, 16 is a good default, 32 is smooth and four times the geometry.");
        this->radialSegments->setConstraints(3, 128);

        this->nearSideMode->setDescription("How the arc between camera and player is treated.\n"
                                           "Solid: closed tube, the player is hidden inside.\n"
                                           "Transparent: that arc uses 'Near Datablock' - typically an alpha-blended copy of the pipe "
                                           "material - so the player shows through while the tube still reads as a tube. Switching between "
                                           "Solid and Transparent is a pure datablock swap and needs no rebuild.\n"
                                           "Hidden: that arc is not built at all and the cut edges get rim strips, giving an open trough. "
                                           "This one changes the geometry, so it rebuilds.");

        this->nearSideArc->setDescription("Width of the near arc in degrees, measured around the camera direction (+Z in the pipe's own "
                                          "frame). 180 is exactly half the tube; 0 disables the split entirely and everything becomes far "
                                          "side. Values above ~200 start eating the walls the player runs on.");
        this->nearSideArc->setConstraints(0.0f, 340.0f);

        this->junctionHubScale->setDescription("Radius of the sphere that fills a junction where three or more arms meet, as a multiple of "
                                               "Pipe Radius. Keep it LOW for a pipe the player runs through: the hub chamber is that much "
                                               "wider than the bore, and the difference is a dip in the floor he has to climb out of at every "
                                               "junction. 1.15 gives about a fifth of the pipe radius and is barely noticeable; 1.3 already looks "
                                               "like a proper bulged fitting but digs a real pothole. Each arm is "
                                               "trimmed back so its mouth sits inside the sphere, and the sphere gets a hole where the arm enters "
                                               "- so the player can pass straight through the junction in any direction.");
        this->junctionHubScale->setConstraints(1.05f, 3.0f);

        this->nearSideAlpha->setDescription("Alpha the near arc is rendered with in Transparent mode. The component clones the datablock and "
                                            "sets the transparency on the COPY, so the body - and everything else using the same material - "
                                            "stays opaque. 1.0 means 'use the datablock exactly as authored' and touches nothing, which is how "
                                            "to plug in a hand-made transparent material instead.");
        this->nearSideAlpha->setConstraints(0.0f, 1.0f);

        this->invertNearSide->setDescription("Which side of the tube counts as the near one. Off means local -Z, which is the camera side for a "
                                             "GameObject with an unrotated frame. Turn it on when a pipe's object is rotated 180 degrees and the "
                                             "transparent or cut-away arc ends up at the back.");

        this->smoothingFactor->setDescription("Amount of height smoothing between connected segments (0-1, higher = smoother gradients).");
        this->curveSubdivisions->setDescription("Number of interpolated points per segment along the path (higher = smoother bends).");

        this->pipeDatablock->setDescription("PBS datablock for the pipe body - everything except the near arc.");
        this->pipeDatablock->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");

        this->nearDatablock->setDescription("PBS datablock for the near arc, used when Near Side Mode is Transparent. Make it a copy of the "
                                            "pipe datablock with transparency enabled (HlmsPbsDatablock::setTransparency). Left empty, the "
                                            "near arc falls back to the pipe datablock, which makes Transparent look identical to Solid.");
        this->nearDatablock->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");

        this->pipeUVTiling->setDescription("UV tiling: x repeats around the circumference, y repeats along the length.");

        this->editMode->setDescription("Object: click-drag to build pipes.\n"
                                       "Segment: LMB to select a segment, X to delete it, E to extend, U / SHIFT+U to nudge its depth.");
        this->editMode->addUserData(GameObject::AttrActionNoUndo());
    }

    ProceduralPipeComponent::~ProceduralPipeComponent()
    {
    }

    void ProceduralPipeComponent::install(const Ogre::NameValuePairList* options)
    {
        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<ProceduralPipeComponent>(ProceduralPipeComponent::getStaticClassId(), ProceduralPipeComponent::getStaticClassName());
    }

    void ProceduralPipeComponent::initialise()
    {
    }

    void ProceduralPipeComponent::shutdown()
    {
    }

    void ProceduralPipeComponent::uninstall()
    {
    }

    const Ogre::String& ProceduralPipeComponent::getName() const
    {
        return this->name;
    }

    void ProceduralPipeComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    bool ProceduralPipeComponent::canStaticAddComponent(GameObject* gameObject)
    {
        return false;
    }

    bool ProceduralPipeComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        this->pipeLoadedFromScene = true;

        // Strictly sequential reader: each property is guarded on its own so a scene saved
        // before a given attribute existed simply skips it instead of desynchronising
        // everything after it.
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPipeComponent::AttrActivated())
        {
            this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPipeComponent::AttrPipeRadius())
        {
            this->pipeRadius->setValue(XMLConverter::getAttribReal(propertyElement, "data", 2.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPipeComponent::AttrWallThickness())
        {
            this->wallThickness->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.25f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPipeComponent::AttrRadialSegments())
        {
            this->radialSegments->setValue(XMLConverter::getAttribInt(propertyElement, "data", 16));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPipeComponent::AttrNearSideMode())
        {
            this->nearSideMode->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPipeComponent::AttrNearSideArc())
        {
            this->nearSideArc->setValue(XMLConverter::getAttribReal(propertyElement, "data", 140.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPipeComponent::AttrJunctionHubScale())
        {
            this->junctionHubScale->setValue(Ogre::Math::Clamp(XMLConverter::getAttribReal(propertyElement, "data", 1.15f), 1.05f, 3.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPipeComponent::AttrNearSideAlpha())
        {
            this->nearSideAlpha->setValue(Ogre::Math::Clamp(XMLConverter::getAttribReal(propertyElement, "data", 0.5f), 0.0f, 1.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPipeComponent::AttrInvertNearSide())
        {
            this->invertNearSide->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPipeComponent::AttrSnapToGrid())
        {
            this->snapToGrid->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPipeComponent::AttrGridSize())
        {
            this->gridSize->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPipeComponent::AttrSmoothingFactor())
        {
            this->smoothingFactor->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.5f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPipeComponent::AttrCurveSubdivisions())
        {
            this->curveSubdivisions->setValue(XMLConverter::getAttribInt(propertyElement, "data", 10));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPipeComponent::AttrPipeDatablock())
        {
            this->pipeDatablock->setValue(XMLConverter::getAttrib(propertyElement, "data", ""));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPipeComponent::AttrNearDatablock())
        {
            this->nearDatablock->setValue(XMLConverter::getAttrib(propertyElement, "data", ""));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPipeComponent::AttrPipeUVTiling())
        {
            this->pipeUVTiling->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        // The path, always last. Decoded here into plain CPU-side data only - init() runs
        // before postInit, so there is no scene node and no frame yet; the mesh is swept in
        // handleSceneParsed.
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralPipeComponent::AttrPathData())
        {
            const Ogre::String encodedPathData = XMLConverter::getAttrib(propertyElement, "data");
            propertyElement = propertyElement->next_sibling("property");

            if (false == encodedPathData.empty())
            {
                this->deserializePathData(encodedPathData);
            }
        }

        return true;
    }

    void ProceduralPipeComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        GameObjectComponent::writeXML(propertiesXML, doc);

        xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPipeComponent::AttrActivated().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPipeComponent::AttrPipeRadius().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->pipeRadius->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPipeComponent::AttrWallThickness().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wallThickness->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPipeComponent::AttrRadialSegments().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->radialSegments->getInt())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPipeComponent::AttrNearSideMode().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->nearSideMode->getListSelectedValue())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPipeComponent::AttrNearSideArc().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->nearSideArc->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPipeComponent::AttrJunctionHubScale().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->junctionHubScale->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPipeComponent::AttrNearSideAlpha().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->nearSideAlpha->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPipeComponent::AttrInvertNearSide().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->invertNearSide->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPipeComponent::AttrSnapToGrid().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->snapToGrid->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPipeComponent::AttrGridSize().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->gridSize->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPipeComponent::AttrSmoothingFactor().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->smoothingFactor->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPipeComponent::AttrCurveSubdivisions().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->curveSubdivisions->getInt())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPipeComponent::AttrPipeDatablock().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->pipeDatablock->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPipeComponent::AttrNearDatablock().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->nearDatablock->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPipeComponent::AttrPipeUVTiling().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->pipeUVTiling->getVector2())));
        propertiesXML->append_node(propertyXML);

        // Written unconditionally, even when empty, so the property order init() walks is the
        // same for every saved pipe. allocate_string because rapidxml does not copy and the
        // serializePathData() result is a temporary.
        const Ogre::String encodedPathData = this->serializePathData();

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralPipeComponent::AttrPathData().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", doc.allocate_string(encodedPathData.c_str())));
        propertiesXML->append_node(propertyXML);
    }

    GameObjectCompPtr ProceduralPipeComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        ProceduralPipeComponentPtr clonedCompPtr(boost::make_shared<ProceduralPipeComponent>());

        clonedCompPtr->setOwner(clonedGameObjectPtr);

        // Attribute setters FIRST, while the clone's path is still empty - every
        // rebuild-triggering setter guards on "path not empty", so none of them builds here.
        // That is the point: at this moment the clone has no platformFrame, no plane anchor,
        // no preview node and a GameObject that has not been initialised, and
        // createPipeMeshInternal ends with gameObjectPtr->init(item), which the GameObject's
        // own initialisation would then throw away again. The path goes in afterwards and the
        // one rebuild happens in postInit.
        clonedCompPtr->setPipeRadius(this->pipeRadius->getReal());
        clonedCompPtr->setWallThickness(this->wallThickness->getReal());
        clonedCompPtr->setRadialSegments(this->radialSegments->getInt());
        clonedCompPtr->setNearSideMode(this->nearSideMode->getListSelectedValue());
        clonedCompPtr->setNearSideArc(this->nearSideArc->getReal());
        clonedCompPtr->setJunctionHubScale(this->junctionHubScale->getReal());
        clonedCompPtr->setNearSideAlpha(this->nearSideAlpha->getReal());
        clonedCompPtr->setInvertNearSide(this->invertNearSide->getBool());
        clonedCompPtr->setSnapToGrid(this->snapToGrid->getBool());
        clonedCompPtr->setGridSize(this->gridSize->getReal());
        clonedCompPtr->setSmoothingFactor(this->smoothingFactor->getReal());
        clonedCompPtr->setCurveSubdivisions(this->curveSubdivisions->getInt());
        clonedCompPtr->setPipeDatablock(this->pipeDatablock->getString());
        clonedCompPtr->setNearDatablock(this->nearDatablock->getString());
        clonedCompPtr->setPipeUVTiling(this->pipeUVTiling->getVector2());

        // Edit Mode is transient editor state, not data - a clone starts in Object mode.

        clonedCompPtr->setActivated(this->activated->getBool());

        clonedCompPtr->pipeSegments = this->pipeSegments;

        // The origin travels with the path: rebuildMesh subtracts it from every control point
        // to get mesh-local space, so leaving it at zero would put the clone's vertices in
        // absolute path coordinates while its node sits elsewhere.
        clonedCompPtr->pipeOrigin = this->pipeOrigin;
        clonedCompPtr->cachedPipeOrigin = this->pipeOrigin;
        clonedCompPtr->hasPipeOrigin = this->hasPipeOrigin;

        // Pre-set so createPipeMeshInternal does not snap the clone's node onto the source's
        // origin - where the clone sits is the editor's decision, and postInit re-anchors the
        // path onto it instead.
        clonedCompPtr->originPositionSet = true;

        clonedCompPtr->pipeClonedNeedsRebuild = (false == this->pipeSegments.empty());

        clonedGameObjectPtr->addComponent(clonedCompPtr);

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));

        return clonedCompPtr;
    }

    bool ProceduralPipeComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPipeComponent] Init pipe component for game object: " + this->gameObjectPtr->getName());

        AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &ProceduralPipeComponent::handleMeshModifyMode), NOWA::EventDataEditorMode::getStaticEventType());
        AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &ProceduralPipeComponent::handleGameObjectSelected), NOWA::EventDataGameObjectSelected::getStaticEventType());
        AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &ProceduralPipeComponent::handleComponentManuallyDeleted), EventDataDeleteComponent::getStaticEventType());

        this->gameObjectPtr->changeCategory("Pipe");

        // The fixed depth plane comes from the GameObject's OWN transform: "fixed -Z axis"
        // means the depth layer was already decided by wherever the (empty) GameObject was
        // placed before this component was added. Works the same for a fresh object and for
        // one restored from a saved scene.
        this->pipeFrame = this->gameObjectPtr->getSceneNode()->_getDerivedOrientationUpdated();
        this->pipePlaneAnchor = this->gameObjectPtr->getSceneNode()->_getDerivedPositionUpdated();

        this->previewNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();

        this->isShiftPressed = true;

        if (true == this->pipeLoadedFromScene)
        {
            this->pipeLoadedFromScene = false;

            AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &ProceduralPipeComponent::handleSceneParsed), EventDataSceneParsed::getStaticEventType());
        }

        this->createSegmentOverlay();

        this->snapRadius = std::max(1.0f, this->pipeRadius->getReal());

        // A clone has neither an init() nor a scene-parsed event to hang its first build on -
        // it is created while the scene is already running. Everything it needs exists here.
        if (true == this->pipeClonedNeedsRebuild)
        {
            this->pipeClonedNeedsRebuild = false;

            if (false == this->pipeSegments.empty())
            {
                // Re-anchor the copied path onto the CLONE's node. Control points are stored in
                // absolute path space, so a clone dropped elsewhere would render correctly (the
                // node transform moves the finished mesh) but be uneditable: the first extension
                // would come from a mouse ray in the clone's position while every existing point
                // still describes the source's. Shifting points AND origin by the same delta
                // leaves every local vertex identical and puts the path in the clone's space.
                const Ogre::Vector3 sourceNodePosition = this->pipeFrame * this->pipeOrigin;
                const Ogre::Vector3 delta = this->pipePlaneAnchor - sourceNodePosition;

                if (false == delta.positionEquals(Ogre::Vector3::ZERO, 0.0001f))
                {
                    for (PipeSegment& seg : this->pipeSegments)
                    {
                        for (PipeControlPoint& cp : seg.controlPoints)
                        {
                            // x/z live in position, the height lives in raw/smoothedHeight -
                            // position.y is always 0 here.
                            cp.position.x += delta.x;
                            cp.position.z += delta.z;
                            cp.rawHeight += delta.y;
                            cp.smoothedHeight += delta.y;
                            cp.renderZ = cp.position.z;
                        }
                    }

                    this->pipeOrigin += delta;
                    this->cachedPipeOrigin = this->pipeOrigin;
                }

                this->rebuildMesh();
                this->updateContinuationPoint();
            }
        }

        return true;
    }

    void ProceduralPipeComponent::handleSceneParsed(NOWA::EventDataPtr eventData)
    {
        AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ProceduralPipeComponent::handleSceneParsed), EventDataSceneParsed::getStaticEventType());

        // The path was decoded in init(); what needs a live scene is the sweep. Always a full
        // rebuild, never a cached-geometry restore: the cache cannot be newer than the
        // attributes, and rebuilding is the one path guaranteed to agree with them.
        if (true == this->pipeSegments.empty())
        {
            return;
        }

        this->rebuildMesh();
        this->updateContinuationPoint();

        const auto& physicsArtifactCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsArtifactComponent>());
        if (physicsArtifactCompPtr)
        {
            this->physicsArtifactComponent = physicsArtifactCompPtr.get();
            if (nullptr != this->physicsArtifactComponent)
            {
                this->physicsArtifactComponent->reCreateCollision();
            }
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPipeComponent] Rebuilt pipe from scene path data with " + Ogre::StringConverter::toString(this->pipeSegments.size()) + " segments");
    }

    bool ProceduralPipeComponent::connect(void)
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
            NOWA::GraphicsModule::getInstance()->enqueue(std::move(cmd), "ProceduralPipeComponent::connect::hideSegOverlay");
        }

        return true;
    }

    bool ProceduralPipeComponent::disconnect(void)
    {
        this->destroyPreviewMesh();
        this->buildState = BuildState::IDLE;

        return true;
    }

    bool ProceduralPipeComponent::onCloned(void)
    {
        return true;
    }

    void ProceduralPipeComponent::onAddComponent(void)
    {
        boost::shared_ptr<EventDataEditorMode> eventDataEditorMode(new EventDataEditorMode(EditorManager::EDITOR_MESH_MODIFY_MODE, this->gameObjectPtr->getId(), ProceduralPipeComponent::getStaticClassName()));
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataEditorMode);

        boost::shared_ptr<NOWA::EventDataGameObjectSelected> eventDataGameObjectSelected(new NOWA::EventDataGameObjectSelected(this->gameObjectPtr->getId(), true, false));
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataGameObjectSelected);

        this->isSelected = true;
        this->addInputListener();
    }

    void ProceduralPipeComponent::onRemoveComponent(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPipeComponent] Removing pipe component for game object: " + this->gameObjectPtr->getName());

        this->physicsArtifactComponent = nullptr;

        AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ProceduralPipeComponent::handleMeshModifyMode), NOWA::EventDataEditorMode::getStaticEventType());
        AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ProceduralPipeComponent::handleGameObjectSelected), NOWA::EventDataGameObjectSelected::getStaticEventType());
        AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ProceduralPipeComponent::handleComponentManuallyDeleted), EventDataDeleteComponent::getStaticEventType());
        AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ProceduralPipeComponent::handleSceneParsed), EventDataSceneParsed::getStaticEventType());

        this->removeInputListener();

        this->destroyPipeMesh();
        this->destroyPreviewMesh();
        this->destroySegmentOverlay();

        // After destroyPipeMesh, so no Item can still reference the clone by the time it goes.
        {
            GraphicsModule::RenderCommand renderCommand = [this]()
            {
                this->destroyClonedNearDatablock();
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralPipeComponent::onRemoveComponent::destroyClonedNearDatablock");
        }

        if (nullptr != this->previewNode)
        {
            NOWA::GraphicsModule::getInstance()->removeTrackedNode(this->previewNode);
            this->gameObjectPtr->getSceneManager()->destroySceneNode(this->previewNode);
            this->previewNode = nullptr;
        }

        GameObjectComponent::onRemoveComponent();
    }

    void ProceduralPipeComponent::onOtherComponentRemoved(unsigned int index)
    {
        if (nullptr != this->physicsArtifactComponent && index == this->physicsArtifactComponent->getIndex())
        {
            this->physicsArtifactComponent = nullptr;
        }
    }

    void ProceduralPipeComponent::update(Ogre::Real dt, bool notSimulating)
    {
    }

    void ProceduralPipeComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (ProceduralPipeComponent::AttrActivated() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
        }
        else if (ProceduralPipeComponent::AttrPipeRadius() == attribute->getName())
        {
            this->setPipeRadius(attribute->getReal());
        }
        else if (ProceduralPipeComponent::AttrWallThickness() == attribute->getName())
        {
            this->setWallThickness(attribute->getReal());
        }
        else if (ProceduralPipeComponent::AttrRadialSegments() == attribute->getName())
        {
            this->setRadialSegments(attribute->getInt());
        }
        else if (ProceduralPipeComponent::AttrNearSideMode() == attribute->getName())
        {
            this->setNearSideMode(attribute->getListSelectedValue());
        }
        else if (ProceduralPipeComponent::AttrNearSideArc() == attribute->getName())
        {
            this->setNearSideArc(attribute->getReal());
        }
        else if (ProceduralPipeComponent::AttrJunctionHubScale() == attribute->getName())
        {
            this->setJunctionHubScale(attribute->getReal());
        }
        else if (ProceduralPipeComponent::AttrNearSideAlpha() == attribute->getName())
        {
            this->setNearSideAlpha(attribute->getReal());
        }
        else if (ProceduralPipeComponent::AttrInvertNearSide() == attribute->getName())
        {
            this->setInvertNearSide(attribute->getBool());
        }
        else if (ProceduralPipeComponent::AttrSnapToGrid() == attribute->getName())
        {
            this->setSnapToGrid(attribute->getBool());
        }
        else if (ProceduralPipeComponent::AttrGridSize() == attribute->getName())
        {
            this->setGridSize(attribute->getReal());
        }
        else if (ProceduralPipeComponent::AttrSmoothingFactor() == attribute->getName())
        {
            this->setSmoothingFactor(attribute->getReal());
        }
        else if (ProceduralPipeComponent::AttrCurveSubdivisions() == attribute->getName())
        {
            this->setCurveSubdivisions(attribute->getInt());
        }
        else if (ProceduralPipeComponent::AttrPipeDatablock() == attribute->getName())
        {
            this->setPipeDatablock(attribute->getString());
        }
        else if (ProceduralPipeComponent::AttrNearDatablock() == attribute->getName())
        {
            this->setNearDatablock(attribute->getString());
        }
        else if (ProceduralPipeComponent::AttrPipeUVTiling() == attribute->getName())
        {
            this->setPipeUVTiling(attribute->getVector2());
        }
        else if (ProceduralPipeComponent::AttrEditMode() == attribute->getName())
        {
            this->setEditMode(attribute->getListSelectedValue());
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Input Handling
    //
    // Raycast hits are converted from WORLD to PIPE-LOCAL immediately, so everything
    // downstream (grid snapping, endpoint snapping, segment picking, the placement lifecycle)
    // runs uniformly in local space. pipeFrame is already known here - postInit captured it.
    ///////////////////////////////////////////////////////////////////////////////////////////////

    bool ProceduralPipeComponent::mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
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
                this->confirmPipe();
                return false;
            }

            // Pick radius stays independent of the pipe radius: the overlay and the hit test
            // must describe the same small editing area, whatever size the tube is.
            const Ogre::Real radius = 1.0f;
            this->selectedSegmentIndex = this->findNearestSegmentOnScreen(screenX, screenY, radius);

            this->scheduleSegmentOverlayUpdate();
            return false;
        }

        // ── OBJECT MODE ───────────────────────────────────────────────────────
        Ogre::Vector3 hitPosition;
        if (this->raycastFixedPlane(screenX, screenY, hitPosition))
        {
            hitPosition = this->pipeFrame.Inverse() * hitPosition;

            if (this->snapToGrid->getBool())
            {
                hitPosition = this->snapToGridFunc(hitPosition);
            }

            if (this->buildState == BuildState::IDLE)
            {
                // Snap the START of a new segment onto an existing endpoint.
                //
                // This is what makes a junction possible at all. Segment ends are matched on a
                // 5 cm grid to decide who meets whom, and a hand-placed click is never that
                // accurate - so without this, branching off an existing pipe produces a segment
                // that merely LOOKS attached. It is never recognised as a third arm, nothing is
                // trimmed, no hub is built, and the branch simply drives its wall through the
                // bore of the pipe it was supposed to join.
                //
                // Only the start is handled here; the end already snaps while dragging.
                if (true == this->detectSnapToOwnPipe(hitPosition, 1.0f))
                {
                    PipeControlPoint startPoint;
                    startPoint.position = Ogre::Vector3(this->snapToPipePoint.x, 0.0f, this->snapToPipePoint.z);
                    startPoint.renderZ = this->snapToPipePoint.z;
                    startPoint.rawHeight = this->snapToPipePoint.y;
                    startPoint.smoothedHeight = startPoint.rawHeight;
                    startPoint.distFromStart = 0.0f;

                    this->currentSegment.controlPoints.clear();
                    this->currentSegment.controlPoints.push_back(startPoint);
                    this->currentSegment.isCurved = false;
                    this->currentSegment.curvature = 0.0f;

                    this->buildState = BuildState::DRAGGING;
                    this->lastValidPosition = startPoint.position;

                    // Cleared straight away: confirmPipe reads these to decide whether the
                    // segment's END closed onto something, and a start snap must not be
                    // mistaken for that - it would drag the far end back onto this point.
                    this->isSnapToOwnPipe = false;
                    this->snapToPipeSegmentIdx = -1;

                    this->hasLoadedPipeEndpoint = false;
                    return false;
                }

                if (this->isShiftPressed && this->hasLoadedPipeEndpoint && false == this->pipeSegments.empty())
                {
                    PipeControlPoint startPoint;
                    startPoint.position = this->loadedPipeEndpoint;
                    startPoint.position.y = 0.0f;
                    startPoint.rawHeight = this->loadedPipeEndpointHeight;
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
                    this->startPipePlacement(hitPosition);
                    this->hasLoadedPipeEndpoint = false;
                }
            }
            else if (this->buildState == BuildState::DRAGGING)
            {
                this->confirmPipe();
            }

            return false;
        }

        return false;
    }

    bool ProceduralPipeComponent::mouseMoved(const OIS::MouseEvent& evt)
    {
        if (false == this->activated->getBool())
        {
            return true;
        }

        const bool wantPreview = (this->buildState == BuildState::DRAGGING) && (this->getEditModeEnum() == EditMode::OBJECT || this->isExtendingFromSegment);

        // Hovering in Object mode with nothing in progress: still run the snap detection, so
        // the green circle appears BEFORE the click. Without it, branching is guesswork - the
        // user finds out whether the two pipes actually share an endpoint only after the
        // junction fails to appear.
        const bool wantHoverSnap = (this->buildState == BuildState::IDLE) && (this->getEditModeEnum() == EditMode::OBJECT) && (false == this->pipeSegments.empty());

        if (false == wantPreview && false == wantHoverSnap)
        {
            return true;
        }

        Ogre::Real screenX = 0.0f;
        Ogre::Real screenY = 0.0f;
        MathHelper::getInstance()->mouseToViewPort(evt.state.X.abs, evt.state.Y.abs, screenX, screenY, Core::getSingletonPtr()->getOgreRenderWindow());

        if (true == wantHoverSnap)
        {
            Ogre::Vector3 hoverPosition;
            if (true == this->raycastFixedPlane(screenX, screenY, hoverPosition))
            {
                hoverPosition = this->pipeFrame.Inverse() * hoverPosition;
                this->detectSnapToOwnPipe(hoverPosition, 1.0f);
                this->scheduleSnapIndicatorUpdate();
            }
            return true;
        }

        Ogre::Vector3 hitPosition;
        if (this->raycastFixedPlane(screenX, screenY, hitPosition))
        {
            hitPosition = this->pipeFrame.Inverse() * hitPosition;

            if (true == this->snapToGrid->getBool())
            {
                hitPosition = this->snapToGridFunc(hitPosition);
            }

            // Fixed snap radius for the same reason the pick radius is fixed - it is an editor
            // affordance, not a function of how fat the tube happens to be.
            const Ogre::Real sr = 1.0f;
            this->detectSnapToOwnPipe(hitPosition, sr);

            const Ogre::Vector3 previewPos = this->isSnapToOwnPipe ? this->snapToPipePoint : hitPosition;

            this->updatePipePreview(previewPos);
            this->scheduleSnapIndicatorUpdate();
        }

        return true;
    }

    bool ProceduralPipeComponent::mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
    {
        if (false == this->activated->getBool())
        {
            return true;
        }

        if (id == OIS::MB_Right)
        {
            this->cancelPipe();
            this->isExtendingFromSegment = false;
            if (this->getEditModeEnum() != EditMode::SEGMENT)
            {
                this->removeInputListener();
            }

            return false;
        }

        return true;
    }

    bool ProceduralPipeComponent::keyPressed(const OIS::KeyEvent& evt)
    {
        if (false == this->activated->getBool())
        {
            return true;
        }

        if (evt.key == OIS::KC_LSHIFT || evt.key == OIS::KC_RSHIFT)
        {
            this->isShiftPressed = true;
            this->isShiftKeyDown = true;
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
                const PipeSegment& sel = this->pipeSegments[this->selectedSegmentIndex];
                const PipeControlPoint& tail = sel.controlPoints.back();

                PipeControlPoint startPoint;
                startPoint.position = tail.position;
                startPoint.position.y = 0.0f;
                // Start at the depth the tail is actually DRAWN at. For a chain end the
                // authored and drawn values agree, but further in the ramp has carried the
                // tube away from its authored offset and the extension would begin in thin air.
                startPoint.position.z = tail.renderZ;
                startPoint.renderZ = tail.renderZ;
                startPoint.rawHeight = tail.smoothedHeight;
                startPoint.smoothedHeight = tail.smoothedHeight;
                startPoint.distFromStart = 0.0f;

                this->currentSegment.controlPoints.clear();
                this->currentSegment.controlPoints.push_back(startPoint);
                this->currentSegment.isCurved = false;
                this->currentSegment.curvature = 0.0f;

                this->buildState = BuildState::DRAGGING;
                this->lastValidPosition = startPoint.position;
                this->isShiftPressed = true;
                this->isExtendingFromSegment = true;

                return false;
            }

            // ── Depth nudge: U / SHIFT+U ─────────────────────────────────────
            // Shifts the selected segment one Pipe Radius along the depth axis, so two runs
            // that cross pass in front of / behind each other. The offset is stored per
            // control point in position.z, and rebuildMesh ramps linearly between a CHAIN'S
            // TWO END values - so nudging an end segment tilts the whole chain into a gentle
            // helix, and nudging a middle segment does nothing visible. isShiftKeyDown, not
            // isShiftPressed: the latter is the auto-chain flag and does not track the key.
            if (evt.key == OIS::KC_U && this->selectedSegmentIndex >= 0)
            {
                Ogre::Real step = this->pipeRadius->getReal();
                if (true == this->isShiftKeyDown)
                {
                    step = -step;
                }

                std::vector<unsigned char> oldData = this->getPlatformData();

                PipeSegment& sel = this->pipeSegments[this->selectedSegmentIndex];
                for (PipeControlPoint& cp : sel.controlPoints)
                {
                    cp.position.z += step;
                }

                this->rebuildMesh();
                this->scheduleSegmentOverlayUpdate();

                std::vector<unsigned char> newData = this->getPlatformData();

                boost::shared_ptr<EventDataCommandTransactionBegin> evtBegin(new EventDataCommandTransactionBegin("Move Pipe Segment Depth"));
                NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evtBegin);

                boost::shared_ptr<EventDataPlatformModifyEnd> evtMod(new EventDataPlatformModifyEnd(oldData, newData, this->gameObjectPtr->getId()));
                NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evtMod);

                boost::shared_ptr<EventDataCommandTransactionEnd> evtEnd(new EventDataCommandTransactionEnd());
                NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evtEnd);

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

            return true;
        }

        // ── OBJECT MODE key handling ──────────────────────────────────────────
        if (evt.key == OIS::KC_Z && this->isCtrlPressed)
        {
            this->removeLastSegment();
            return false;
        }
        else if (evt.key == OIS::KC_ESCAPE)
        {
            this->cancelPipe();
            this->removeInputListener();
            return false;
        }

        return true;
    }

    bool ProceduralPipeComponent::keyReleased(const OIS::KeyEvent& evt)
    {
        if (false == this->activated->getBool())
        {
            return true;
        }

        if (evt.key == OIS::KC_LSHIFT || evt.key == OIS::KC_RSHIFT)
        {
            this->isShiftPressed = false;
            this->isShiftKeyDown = false;
        }
        else if (evt.key == OIS::KC_LCONTROL || evt.key == OIS::KC_RCONTROL)
        {
            this->isCtrlPressed = false;
        }

        return false;
    }

    bool ProceduralPipeComponent::raycastFixedPlane(Ogre::Real screenX, Ogre::Real screenY, Ogre::Vector3& hitPosition, Ogre::Real localZOffset)
    {
        Ogre::Camera* camera = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
        if (nullptr == camera)
        {
            return false;
        }

        Ogre::Ray ray = camera->getCameraToViewportRay(screenX, screenY);

        // Plane through pipePlaneAnchor with normal = pipeFrame * UNIT_Z. pipeFrame is a
        // quaternion, so that normal is already unit length and localZOffset can be added
        // straight onto the plane constant to slide the plane along itself.
        const Ogre::Vector3 planeNormal = this->pipeFrame * Ogre::Vector3::UNIT_Z;
        const Ogre::Real planeD = planeNormal.dotProduct(this->pipePlaneAnchor) + localZOffset;
        Ogre::Plane workingPlane(planeNormal, planeD);

        std::pair<bool, Ogre::Real> result = ray.intersects(workingPlane);
        if (true == result.first && result.second > 0.0f)
        {
            hitPosition = ray.getPoint(result.second);
            return true;
        }

        return false;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Placement lifecycle
    ///////////////////////////////////////////////////////////////////////////////////////////////

    void ProceduralPipeComponent::startPipePlacement(const Ogre::Vector3& worldPosition)
    {
        Ogre::Vector3 startPos = this->snapToGrid->getBool() ? this->snapToGridFunc(worldPosition) : worldPosition;

        // A freshly drawn pipe starts on the plane the GameObject itself defines. Inheriting
        // the raycast hit's z instead would make a later switch to Segment mode restore some
        // older depth - the same trap the platform component documents.
        startPos.z = (this->pipeFrame.Inverse() * this->pipePlaneAnchor).z;

        PipeControlPoint startPoint;
        startPoint.position = startPos;
        startPoint.position.y = 0.0f;
        startPoint.rawHeight = startPos.y;
        startPoint.smoothedHeight = startPoint.rawHeight;
        startPoint.distFromStart = 0.0f;
        startPoint.renderZ = startPos.z;

        this->currentSegment.controlPoints.clear();
        this->currentSegment.controlPoints.push_back(startPoint);
        this->currentSegment.isCurved = false;
        this->currentSegment.curvature = 0.0f;

        if (false == this->hasPipeOrigin)
        {
            this->pipeOrigin = startPoint.position;
            this->pipeOrigin.y = startPoint.rawHeight;
            this->hasPipeOrigin = true;
        }

        this->buildState = BuildState::DRAGGING;
        this->lastValidPosition = startPoint.position;
    }

    void ProceduralPipeComponent::updatePipePreview(const Ogre::Vector3& worldPosition)
    {
        if (true == this->currentSegment.controlPoints.empty())
        {
            return;
        }

        // When snapping is active the incoming position IS the resolved snap point - grid
        // snapping it again would nudge it straight back off.
        Ogre::Vector3 currentPos = (this->snapToGrid->getBool() && false == this->isSnapToOwnPipe) ? this->snapToGridFunc(worldPosition) : worldPosition;

        if (true == this->isCtrlPressed)
        {
            const Ogre::Vector3 delta = currentPos - this->currentSegment.controlPoints.front().position;
            const Ogre::Real deltaHeight = std::abs(currentPos.y - this->currentSegment.controlPoints.front().rawHeight);
            if (std::abs(delta.x) > deltaHeight)
            {
                currentPos.y = this->currentSegment.controlPoints.front().rawHeight;
            }
            else
            {
                currentPos.x = this->currentSegment.controlPoints.front().position.x;
            }
        }

        PipeControlPoint endPoint;
        endPoint.position = currentPos;
        endPoint.position.y = 0.0f;
        // Inherit the depth from the point this drag started at rather than from the base
        // plane the raycast returned, so an extension from a nudged run does not slope back.
        endPoint.position.z = this->currentSegment.controlPoints.front().position.z;
        endPoint.renderZ = endPoint.position.z;
        endPoint.rawHeight = currentPos.y;
        endPoint.smoothedHeight = endPoint.rawHeight;
        endPoint.distFromStart = this->currentSegment.controlPoints.front().position.distance(currentPos);

        if (this->currentSegment.controlPoints.size() == 1)
        {
            this->currentSegment.controlPoints.push_back(endPoint);
        }
        else
        {
            this->currentSegment.controlPoints.back() = endPoint;
        }

        this->lastValidPosition = currentPos;

        this->updatePreviewMesh();
    }

    void ProceduralPipeComponent::confirmPipe(void)
    {
        if (this->buildState != BuildState::DRAGGING)
        {
            return;
        }

        if (this->currentSegment.controlPoints.size() < 2)
        {
            return;
        }

        const Ogre::Real length = this->currentSegment.controlPoints.front().position.distance(this->currentSegment.controlPoints.back().position);
        if (length < 0.05f && std::abs(this->currentSegment.controlPoints.front().rawHeight - this->currentSegment.controlPoints.back().rawHeight) < 0.05f)
        {
            this->cancelPipe();
            return;
        }

        const bool wasSnapping = this->isSnapToOwnPipe;

        if (wasSnapping)
        {
            PipeControlPoint& endCP = this->currentSegment.controlPoints.back();
            endCP.position = this->snapToPipePoint;
            endCP.position.y = 0.0f;
            endCP.rawHeight = this->snapToPipePoint.y;
            endCP.smoothedHeight = this->snapToPipePoint.y;

            this->isSnapToOwnPipe = false;
            this->snapToPipeSegmentIdx = -1;
        }

        // Closing onto an existing endpoint always terminates - never keep chaining.
        const bool shouldChain = this->isShiftPressed && false == wasSnapping;

        boost::shared_ptr<NOWA::EventDataCommandTransactionBegin> eventDataUndoBegin(new NOWA::EventDataCommandTransactionBegin("Add Pipe Segment"));
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataUndoBegin);

        std::vector<unsigned char> oldData = this->getPlatformData();

        this->smoothHeightTransitions(this->currentSegment.controlPoints);

        const PipeControlPoint exactEndpoint = this->currentSegment.controlPoints.back();

        this->pipeSegments.push_back(this->currentSegment);

        this->destroyPreviewMesh();
        if (this->previewNode)
        {
            this->previewNode->setPosition(Ogre::Vector3::ZERO);
        }

        this->rebuildMesh();
        this->updateContinuationPoint();
        this->scheduleSegmentOverlayUpdate();

        std::vector<unsigned char> newData = this->getPlatformData();

        boost::shared_ptr<EventDataPlatformModifyEnd> eventDataPipeModifyEnd(new EventDataPlatformModifyEnd(oldData, newData, this->gameObjectPtr->getId()));
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataPipeModifyEnd);

        boost::shared_ptr<NOWA::EventDataCommandTransactionEnd> eventDataUndoEnd(new NOWA::EventDataCommandTransactionEnd());
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataUndoEnd);

        if (true == shouldChain)
        {
            PipeControlPoint startPoint;
            startPoint.position = exactEndpoint.position;
            startPoint.position.y = 0.0f;
            startPoint.rawHeight = exactEndpoint.smoothedHeight;
            startPoint.smoothedHeight = exactEndpoint.smoothedHeight;
            startPoint.renderZ = exactEndpoint.position.z;
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
    }

    void ProceduralPipeComponent::updateContinuationPoint(void)
    {
        if (false == this->pipeSegments.empty())
        {
            const PipeSegment& lastSegment = this->pipeSegments.back();
            if (false == lastSegment.controlPoints.empty())
            {
                const PipeControlPoint& lastCP = lastSegment.controlPoints.back();

                this->loadedPipeEndpoint = lastCP.position;
                this->loadedPipeEndpointHeight = lastCP.smoothedHeight;
                this->hasLoadedPipeEndpoint = true;
            }
        }
        else
        {
            this->hasLoadedPipeEndpoint = false;
        }
    }

    void ProceduralPipeComponent::cancelPipe(void)
    {
        this->destroyPreviewMesh();
        this->buildState = BuildState::IDLE;
        this->currentSegment.controlPoints.clear();

        this->isShiftPressed = false;
        this->isCtrlPressed = false;
    }

    void ProceduralPipeComponent::removeLastSegment(void)
    {
        if (true == this->pipeSegments.empty())
        {
            return;
        }

        std::vector<unsigned char> oldData = this->getPlatformData();

        this->pipeSegments.pop_back();

        if (true == this->pipeSegments.empty())
        {
            this->destroyPipeMesh();
            this->pathSamples.clear();
            this->hasPipeOrigin = false;
            this->hasLoadedPipeEndpoint = false;
        }
        else
        {
            this->rebuildMesh();
            this->updateContinuationPoint();
        }

        this->scheduleSegmentOverlayUpdate();

        std::vector<unsigned char> newData = this->getPlatformData();

        boost::shared_ptr<EventDataPlatformModifyEnd> eventDataPipeModifyEnd(new EventDataPlatformModifyEnd(oldData, newData, this->gameObjectPtr->getId()));
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataPipeModifyEnd);
    }

    void ProceduralPipeComponent::clearAllSegments(void)
    {
        if (true == this->pipeSegments.empty())
        {
            return;
        }

        std::vector<unsigned char> oldData = this->getPlatformData();

        this->pipeSegments.clear();
        this->pathSamples.clear();
        this->destroyPipeMesh();
        this->hasPipeOrigin = false;
        this->hasLoadedPipeEndpoint = false;
        this->originPositionSet = false;

        if (false == AppStateManager::getSingletonPtr()->getGameObjectController()->getIsDestroying())
        {
            std::vector<unsigned char> newData;

            boost::shared_ptr<EventDataPlatformModifyEnd> eventDataPipeModifyEnd(new EventDataPlatformModifyEnd(oldData, newData, this->gameObjectPtr->getId()));
            NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataPipeModifyEnd);
        }
    }

    void ProceduralPipeComponent::addPipeSegment(const Ogre::Vector3& start, const Ogre::Vector3& end)
    {
        PipeSegment segment;

        PipeControlPoint startPoint;
        startPoint.position = Ogre::Vector3(start.x, 0.0f, start.z);
        startPoint.rawHeight = start.y;
        startPoint.smoothedHeight = start.y;
        startPoint.renderZ = start.z;
        startPoint.distFromStart = 0.0f;

        PipeControlPoint endPoint;
        endPoint.position = Ogre::Vector3(end.x, 0.0f, start.z);
        endPoint.rawHeight = end.y;
        endPoint.smoothedHeight = end.y;
        endPoint.renderZ = start.z;
        endPoint.distFromStart = Ogre::Vector2(end.x - start.x, end.y - start.y).length();

        segment.controlPoints.push_back(startPoint);
        segment.controlPoints.push_back(endPoint);

        if (false == this->hasPipeOrigin)
        {
            this->pipeOrigin = Ogre::Vector3(start.x, start.y, start.z);
            this->hasPipeOrigin = true;
        }

        this->pipeSegments.push_back(segment);

        this->rebuildMesh();
        this->updateContinuationPoint();
    }

    int ProceduralPipeComponent::getSegmentCount(void) const
    {
        return static_cast<int>(this->pipeSegments.size());
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Path math
    //
    // All of it works on (x, height): those two axes ARE the path. Depth is a third, separate
    // quantity that is ramped per chain, not part of the curve - which is what makes a 2.5D
    // sweep so much cheaper than a general 3D one.
    ///////////////////////////////////////////////////////////////////////////////////////////////

    namespace
    {
        inline Ogre::Real pipePathDistance2D(Ogre::Real x0, Ogre::Real h0, Ogre::Real x1, Ogre::Real h1)
        {
            const Ogre::Real dx = x1 - x0;
            const Ogre::Real dh = h1 - h0;
            return std::sqrt(dx * dx + dh * dh);
        }
    }

    Ogre::Vector2 ProceduralPipeComponent::evaluateCatmullRom(const std::vector<PipeControlPoint>& points, Ogre::Real t)
    {
        if (points.size() < 2)
        {
            if (true == points.empty())
            {
                return Ogre::Vector2::ZERO;
            }
            return Ogre::Vector2(points[0].position.x, points[0].smoothedHeight);
        }

        const int numPoints = static_cast<int>(points.size());
        int i = static_cast<int>(std::floor(t));
        Ogre::Real localT = t - static_cast<Ogre::Real>(i);

        i = Ogre::Math::Clamp(i, 0, numPoints - 2);
        localT = Ogre::Math::Clamp(localT, 0.0f, 1.0f);

        // Endpoints are duplicated rather than extrapolated, so a chain does not bulge outward
        // at its first and last span.
        const int i0 = std::max(0, i - 1);
        const int i1 = i;
        const int i2 = std::min(numPoints - 1, i + 1);
        const int i3 = std::min(numPoints - 1, i + 2);

        const Ogre::Vector2 p0(points[i0].position.x, points[i0].smoothedHeight);
        const Ogre::Vector2 p1(points[i1].position.x, points[i1].smoothedHeight);
        const Ogre::Vector2 p2(points[i2].position.x, points[i2].smoothedHeight);
        const Ogre::Vector2 p3(points[i3].position.x, points[i3].smoothedHeight);

        const Ogre::Real t2 = localT * localT;
        const Ogre::Real t3 = t2 * localT;

        // x and height are interpolated TOGETHER by the same spline. Interpolating them
        // separately (as one would for a road, where height comes from a terrain raycast)
        // would give a curve whose height no longer belongs to its own x.
        return 0.5f * ((2.0f * p1) + (-p0 + p2) * localT + (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 + (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
    }

    std::vector<ProceduralPipeComponent::PipeControlPoint> ProceduralPipeComponent::subdivideWithHeightInterpolation(const std::vector<PipeControlPoint>& points)
    {
        std::vector<PipeControlPoint> result;

        if (points.size() < 2)
        {
            return points;
        }

        const int subdivisions = std::max(1, this->curveSubdivisions->getInt());
        const bool useSpline = (points.size() >= 3);

        for (size_t i = 0; i + 1 < points.size(); ++i)
        {
            const PipeControlPoint& a = points[i];
            const PipeControlPoint& b = points[i + 1];

            for (int s = 0; s < subdivisions; ++s)
            {
                const Ogre::Real localT = static_cast<Ogre::Real>(s) / static_cast<Ogre::Real>(subdivisions);

                PipeControlPoint p;

                if (true == useSpline)
                {
                    const Ogre::Vector2 xh = this->evaluateCatmullRom(points, static_cast<Ogre::Real>(i) + localT);
                    p.position.x = xh.x;
                    p.rawHeight = xh.y;
                }
                else
                {
                    p.position.x = a.position.x + (b.position.x - a.position.x) * localT;
                    p.rawHeight = a.smoothedHeight + (b.smoothedHeight - a.smoothedHeight) * localT;
                }

                p.position.y = 0.0f;
                // renderZ (the RAMPED depth, not the authored one) is what the tube is swept
                // at, so it has to be carried through the subdivision - linearly, because the
                // ramp is linear in arc length by construction.
                p.position.z = a.position.z + (b.position.z - a.position.z) * localT;
                p.renderZ = a.renderZ + (b.renderZ - a.renderZ) * localT;
                p.smoothedHeight = p.rawHeight;

                result.push_back(p);
            }
        }

        result.push_back(points.back());

        // Accumulated distance, recomputed here rather than carried over: the subdivided path
        // is longer than the polyline it came from, and every UV and every path sample reads
        // this value.
        Ogre::Real accumulated = 0.0f;
        for (size_t i = 0; i < result.size(); ++i)
        {
            if (i > 0)
            {
                accumulated += pipePathDistance2D(result[i - 1].position.x, result[i - 1].smoothedHeight, result[i].position.x, result[i].smoothedHeight);
            }
            result[i].distFromStart = accumulated;
        }

        return result;
    }

    std::vector<ProceduralPipeComponent::PipeControlPoint> ProceduralPipeComponent::resamplePathUniformly(const std::vector<PipeControlPoint>& densePath, Ogre::Real stepMeters)
    {
        if (densePath.size() < 2 || stepMeters <= 0.0f)
        {
            return densePath;
        }

        const Ogre::Real totalLength = densePath.back().distFromStart;
        if (totalLength <= stepMeters)
        {
            return densePath;
        }

        // Even ring spacing matters more for a tube than for a slab: unevenly spaced rings
        // show up directly as stretched texture bands around the circumference, where a slab
        // only has two long edges to betray it.
        const int numSteps = std::max(1, static_cast<int>(std::ceil(totalLength / stepMeters)));

        std::vector<PipeControlPoint> result;
        result.reserve(numSteps + 1);

        size_t searchIdx = 0;

        for (int s = 0; s <= numSteps; ++s)
        {
            const Ogre::Real target = totalLength * static_cast<Ogre::Real>(s) / static_cast<Ogre::Real>(numSteps);

            while (searchIdx + 2 < densePath.size() && densePath[searchIdx + 1].distFromStart < target)
            {
                ++searchIdx;
            }

            const PipeControlPoint& a = densePath[searchIdx];
            const PipeControlPoint& b = densePath[searchIdx + 1];

            const Ogre::Real span = b.distFromStart - a.distFromStart;
            const Ogre::Real localT = (span > 1e-6f) ? Ogre::Math::Clamp((target - a.distFromStart) / span, 0.0f, 1.0f) : 0.0f;

            PipeControlPoint p;
            p.position.x = a.position.x + (b.position.x - a.position.x) * localT;
            p.position.y = 0.0f;
            p.position.z = a.position.z + (b.position.z - a.position.z) * localT;
            p.renderZ = a.renderZ + (b.renderZ - a.renderZ) * localT;
            p.rawHeight = a.smoothedHeight + (b.smoothedHeight - a.smoothedHeight) * localT;
            p.smoothedHeight = p.rawHeight;
            p.distFromStart = target;

            result.push_back(p);
        }

        return result;
    }

    void ProceduralPipeComponent::smoothHeightTransitions(std::vector<PipeControlPoint>& points)
    {
        if (points.size() < 3)
        {
            // Two points are a straight run - there is nothing between them to smooth, and
            // smoothedHeight must still be initialised or the sweep reads zeros.
            for (PipeControlPoint& p : points)
            {
                p.smoothedHeight = p.rawHeight;
            }
            return;
        }

        const Ogre::Real factor = Ogre::Math::Clamp(this->smoothingFactor->getReal(), 0.0f, 1.0f);

        for (PipeControlPoint& p : points)
        {
            p.smoothedHeight = p.rawHeight;
        }

        if (factor <= 0.0f)
        {
            return;
        }

        // The chain ENDS keep their authored height: they are where this chain meets another
        // one or where the player enters, and moving them would open a step exactly where it
        // is most visible.
        std::vector<Ogre::Real> smoothed(points.size());
        smoothed.front() = points.front().rawHeight;
        smoothed.back() = points.back().rawHeight;

        for (size_t i = 1; i + 1 < points.size(); ++i)
        {
            const Ogre::Real neighbourAverage = 0.5f * (points[i - 1].rawHeight + points[i + 1].rawHeight);
            smoothed[i] = points[i].rawHeight + (neighbourAverage - points[i].rawHeight) * factor;
        }

        for (size_t i = 0; i < points.size(); ++i)
        {
            points[i].smoothedHeight = smoothed[i];
        }
    }

    ProceduralPipeComponent::EndpointKey ProceduralPipeComponent::makeEndpointKey(const PipeControlPoint& cp)
    {
        EndpointKey key;
        key.x = static_cast<int>(std::round(cp.position.x * 20.0f));
        key.h = static_cast<int>(std::round(cp.smoothedHeight * 20.0f));
        return key;
    }

    std::map<ProceduralPipeComponent::EndpointKey, std::vector<std::pair<size_t, int>>> ProceduralPipeComponent::buildEndpointMap(void) const
    {
        std::map<EndpointKey, std::vector<std::pair<size_t, int>>> endpointMap;

        for (size_t si = 0; si < this->pipeSegments.size(); ++si)
            {
            if (this->pipeSegments[si].controlPoints.size() < 2)
            {
                continue;
            }
            endpointMap[makeEndpointKey(this->pipeSegments[si].controlPoints.front())].push_back({si, 0});
            endpointMap[makeEndpointKey(this->pipeSegments[si].controlPoints.back())].push_back({si, 1});
        }

        return endpointMap;
    }

    std::vector<std::vector<std::pair<size_t, bool>>> ProceduralPipeComponent::buildChains(void) const
    {
        std::vector<std::vector<std::pair<size_t, bool>>> chains;

        const size_t numSegments = this->pipeSegments.size();
        if (0 == numSegments)
        {
            return chains;
        }

        const std::map<EndpointKey, std::vector<std::pair<size_t, int>>> endpointMap = this->buildEndpointMap();

        std::vector<bool> visited(numSegments, false);

        // A chain STOPS at a junction rather than picking an arbitrary branch to continue
        // into. That is the whole difference to a plain polyline walker: which two of three
        // arms get swept as one continuous tube would otherwise be decided by segment order,
        // and the third arm would end in mid air. Stopping here leaves every arm as its own
        // chain, each with a clean mouth the hub can be fitted to.
        auto degreeOf = [&endpointMap](const EndpointKey& key) -> size_t
        {
            const auto it = endpointMap.find(key);
            return (it == endpointMap.end()) ? 0 : it->second.size();
        };

        auto walkFrom = [&](size_t startSegment, bool enterAtFront)
        {
            std::vector<std::pair<size_t, bool>> chain;

            size_t current = startSegment;
            bool atFront = enterAtFront;

            while (true)
            {
                visited[current] = true;

                // Entered at the back means the segment is traversed backwards, so the sweep
                // reads its control points in reverse.
                chain.push_back({current, false == atFront});

                const PipeControlPoint& exitPoint = atFront ? this->pipeSegments[current].controlPoints.back() : this->pipeSegments[current].controlPoints.front();
                const EndpointKey exitKey = makeEndpointKey(exitPoint);

                if (degreeOf(exitKey) > 2)
                {
                    break;
                }

                const auto it = endpointMap.find(exitKey);
                if (it == endpointMap.end())
                {
                    break;
                }

                bool advanced = false;
                for (const auto& incidence : it->second)
                {
                    if (incidence.first == current || true == visited[incidence.first])
                    {
                        continue;
                    }

                    current = incidence.first;
                    atFront = (0 == incidence.second);
                    advanced = true;
                    break;
                }

                if (false == advanced)
                {
                    break;
                }
            }

            chains.push_back(chain);
        };

        // Real ends first - an open end (degree 1) or a junction (degree 3+). Starting in the
        // middle of a chain would split it in two.
        for (size_t si = 0; si < numSegments; ++si)
        {
            if (true == visited[si] || this->pipeSegments[si].controlPoints.size() < 2)
            {
                continue;
            }

            const size_t frontDegree = degreeOf(makeEndpointKey(this->pipeSegments[si].controlPoints.front()));
            const size_t backDegree = degreeOf(makeEndpointKey(this->pipeSegments[si].controlPoints.back()));

            if (1 == frontDegree || frontDegree > 2)
            {
                walkFrom(si, true);
            }
            else if (1 == backDegree || backDegree > 2)
            {
                walkFrom(si, false);
            }
        }

        // Whatever is left is a closed loop - no end anywhere. Started arbitrarily, since a
        // loop has no natural beginning.
        for (size_t si = 0; si < numSegments; ++si)
        {
            if (true == visited[si] || this->pipeSegments[si].controlPoints.size() < 2)
            {
                continue;
            }
            walkFrom(si, true);
        }

        return chains;
    }

    std::vector<ProceduralPipeComponent::PipeControlPoint> ProceduralPipeComponent::trimPathEnds(const std::vector<PipeControlPoint>& points, Ogre::Real trimFront, Ogre::Real trimBack) const
    {
        if (points.size() < 2)
        {
            return points;
        }

        const Ogre::Real total = points.back().distFromStart;

        // Refuse to trim a chain that is shorter than the two bites taken out of it - a
        // segment drawn shorter than the hub radius would otherwise vanish completely and take
        // its arm direction with it. Better a slightly poking arm than a missing one.
        if (total <= (trimFront + trimBack) * 1.1f)
        {
            return points;
        }

        const Ogre::Real from = trimFront;
        const Ogre::Real to = total - trimBack;

        auto sampleAt = [&points](Ogre::Real target) -> PipeControlPoint
        {
            for (size_t i = 1; i < points.size(); ++i)
            {
                if (points[i].distFromStart >= target)
                {
                    const PipeControlPoint& a = points[i - 1];
                    const PipeControlPoint& b = points[i];

                    const Ogre::Real span = b.distFromStart - a.distFromStart;
                    const Ogre::Real t = (span > 1e-6f) ? Ogre::Math::Clamp((target - a.distFromStart) / span, 0.0f, 1.0f) : 0.0f;

                    PipeControlPoint p;
                    p.position.x = a.position.x + (b.position.x - a.position.x) * t;
                    p.position.y = 0.0f;
                    p.position.z = a.position.z + (b.position.z - a.position.z) * t;
                    p.renderZ = a.renderZ + (b.renderZ - a.renderZ) * t;
                    p.rawHeight = a.smoothedHeight + (b.smoothedHeight - a.smoothedHeight) * t;
                    p.smoothedHeight = p.rawHeight;
                    p.distFromStart = target;
                    return p;
                }
            }
            return points.back();
        };

        std::vector<PipeControlPoint> result;
        result.reserve(points.size());

        result.push_back(sampleAt(from));

        for (const PipeControlPoint& p : points)
        {
            if (p.distFromStart > from + 1e-4f && p.distFromStart < to - 1e-4f)
            {
                result.push_back(p);
            }
        }

        result.push_back(sampleAt(to));

        // distFromStart is re-based on the trimmed path: it drives the UVs and the Lua path
        // samples, and both want to start at 0 for whatever actually got built.
        const Ogre::Real base = result.front().distFromStart;
        for (PipeControlPoint& p : result)
        {
            p.distFromStart -= base;
        }

        return result;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Mesh generation
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ProceduralPipeComponent::PipeMeshBuffer ProceduralPipeComponent::bufferForDirection(const Ogre::Vector3& localDirection) const
    {
        const Ogre::Real arcDegrees = this->nearSideArc->getReal();
        if (arcDegrees <= 0.0f)
        {
            return PipeMeshBuffer::FAR_SIDE;
        }

        // The camera side is local -Z: a 2.5D camera sits in FRONT of the depth plane looking
        // along -Z into the level, so the surface it faces is the one whose outward normal
        // points back at it. invertNearSide flips the axis for a GameObject whose frame is
        // rotated 180 degrees.
        //
        // Both the ring sweep and the junction hub classify through this one function, so a
        // sphere quad lands on the same side of the cut as the tube quad it meets - otherwise
        // the cut-away would jump sides at every junction.
        const Ogre::Real halfArc = Ogre::Degree(arcDegrees * 0.5f).valueRadians();

        Ogre::Vector3 dir = localDirection;
        if (dir.squaredLength() < 1e-12f)
        {
            return PipeMeshBuffer::FAR_SIDE;
        }
        dir.normalise();

        const Ogre::Real towardsCamera = this->invertNearSide->getBool() ? dir.z : -dir.z;

        return (towardsCamera >= std::cos(halfArc)) ? PipeMeshBuffer::NEAR_SIDE : PipeMeshBuffer::FAR_SIDE;
    }

    ProceduralPipeComponent::PipeMeshBuffer ProceduralPipeComponent::bufferForAngle(Ogre::Real theta) const
    {
        // theta is measured from the ring's +depth axis, which for a 2.5D path IS local +Z up
        // to the Gram-Schmidt correction a depth ramp introduces - so cos(theta) is the z
        // component of the ring direction, and the classification stays identical to the hub's.
        return this->bufferForDirection(Ogre::Vector3(0.0f, 0.0f, std::cos(theta)));
    }

    void ProceduralPipeComponent::addPipeQuad(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& v3, const Ogre::Vector3& normal, Ogre::Real u0, Ogre::Real u1, Ogre::Real v0Val,
        Ogre::Real v1Val, PipeMeshBuffer targetBuffer)
    {
        std::vector<float>& verts = (targetBuffer == PipeMeshBuffer::NEAR_SIDE) ? this->nearVertices : this->farVertices;
        std::vector<Ogre::uint32>& inds = (targetBuffer == PipeMeshBuffer::NEAR_SIDE) ? this->nearIndices : this->farIndices;
        Ogre::uint32& currentIdx = (targetBuffer == PipeMeshBuffer::NEAR_SIDE) ? this->currentNearVertexIndex : this->currentFarVertexIndex;

        const Ogre::Vector3 edge1 = v1 - v0;
        const Ogre::Vector3 edge2 = v2 - v0;
        Ogre::Vector3 triNormal = edge1.crossProduct(edge2);

        // Degeneracy threshold scales with the edge lengths. A fixed absolute epsilon breaks
        // exactly where rings are densest (tight bends, high radial counts), which is the last
        // place where silently giving up on winding correction is acceptable.
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

        const Ogre::uint32 baseIdx = currentIdx;
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

    void ProceduralPipeComponent::generatePipeRings(const std::vector<PipeControlPoint>& points, bool capFront, bool capBack)
    {
        const size_t numRings = points.size();
        if (numRings < 2)
        {
            return;
        }

        const int radialCount = std::max(3, this->radialSegments->getInt());
        const Ogre::Real outerRadius = std::max(0.01f, this->pipeRadius->getReal());
        const Ogre::Real thickness = std::max(0.0f, this->wallThickness->getReal());
        const Ogre::Real innerRadius = outerRadius - thickness;
        const bool hasInnerShell = (thickness > 0.0f && innerRadius > 0.01f);
        const bool hideNear = (this->getNearSideModeEnum() == NearSideMode::HIDDEN && this->nearSideArc->getReal() > 0.0f);

        const Ogre::Vector2 tiling = this->pipeUVTiling->getVector2();

        // ── Per-ring frames ──────────────────────────────────────────────────────────
        // depthAxis is the component's fixed Z, Gram-Schmidt'd against the local tangent so it
        // stays perpendicular even where a depth ramp tilts the path out of the plane.
        // planeNormal completes the frame and is the axis the miter stretches: a bend happens
        // in the tangent/normal plane and leaves the depth axis untouched, which is the whole
        // reason a 2.5D tube needs no parallel-transport frame.
        std::vector<Ogre::Vector3> centres(numRings);
        std::vector<Ogre::Vector3> tangents(numRings);
        std::vector<Ogre::Vector3> depthAxes(numRings);
        std::vector<Ogre::Vector3> planeNormals(numRings);
        std::vector<Ogre::Real> miterScales(numRings, 1.0f);

        for (size_t i = 0; i < numRings; ++i)
        {
            centres[i] = Ogre::Vector3(points[i].position.x, points[i].smoothedHeight, points[i].renderZ);
        }

        for (size_t i = 0; i < numRings; ++i)
        {
            Ogre::Vector3 tangent;
            if (0 == i)
            {
                tangent = centres[1] - centres[0];
            }
            else if (i == numRings - 1)
            {
                tangent = centres[numRings - 1] - centres[numRings - 2];
            }
            else
            {
                tangent = centres[i + 1] - centres[i - 1];
            }

            if (tangent.squaredLength() < 1e-12f)
            {
                tangent = Ogre::Vector3::UNIT_X;
            }
            tangent.normalise();
            tangents[i] = tangent;

            Ogre::Vector3 depthAxis = Ogre::Vector3::UNIT_Z - tangent * tangent.dotProduct(Ogre::Vector3::UNIT_Z);
            if (depthAxis.squaredLength() < 1e-8f)
            {
                // Only reachable if the path ran straight along the depth axis, which the 2.5D
                // editor cannot produce - but a Lua caller could.
                depthAxis = Ogre::Vector3::UNIT_Y - tangent * tangent.dotProduct(Ogre::Vector3::UNIT_Y);
            }
            depthAxis.normalise();
            depthAxes[i] = depthAxis;

            planeNormals[i] = depthAxis.crossProduct(tangent).normalisedCopy();

            if (i > 0 && i < numRings - 1)
            {
                Ogre::Vector3 incoming = centres[i] - centres[i - 1];
                if (incoming.squaredLength() > 1e-12f)
                {
                    incoming.normalise();
                    // cos of HALF the turn, because tangent is already the bisector. Clamped so
                    // a near-hairpin stretches by at most 4x instead of exploding to infinity.
                    const Ogre::Real cosHalf = std::max(0.25f, incoming.dotProduct(tangent));
                    miterScales[i] = 1.0f / cosHalf;
                }
            }
        }

        auto ringPoint = [&](size_t ring, Ogre::Real theta, Ogre::Real radius) -> Ogre::Vector3
        {
            // Only the in-plane component is mitered - the depth component of the ring is
            // unaffected by a bend that happens within the plane.
            return centres[ring] + depthAxes[ring] * (radius * std::cos(theta)) + planeNormals[ring] * (radius * std::sin(theta) * miterScales[ring]);
        };

        auto ringNormal = [&](size_t ring, Ogre::Real theta) -> Ogre::Vector3
        {
            // Un-mitered direction: the miter is a positional correction, the surface still
            // faces the way an unstretched circle would.
            return (depthAxes[ring] * std::cos(theta) + planeNormals[ring] * std::sin(theta)).normalisedCopy();
        };

        auto thetaAt = [radialCount](int j) -> Ogre::Real
        {
            return Ogre::Math::TWO_PI * static_cast<Ogre::Real>(j) / static_cast<Ogre::Real>(radialCount);
        };

        // u runs around the circumference, v along the length in meters.
        auto uAt = [&](int j) -> Ogre::Real
        {
            return (static_cast<Ogre::Real>(j) / static_cast<Ogre::Real>(radialCount)) * tiling.x;
        };

        // ── Shells ───────────────────────────────────────────────────────────────────
        for (size_t i = 0; i + 1 < numRings; ++i)
        {
            const Ogre::Real vA = points[i].distFromStart * tiling.y;
            const Ogre::Real vB = points[i + 1].distFromStart * tiling.y;

            for (int j = 0; j < radialCount; ++j)
            {
                const Ogre::Real theta0 = thetaAt(j);
                const Ogre::Real theta1 = thetaAt(j + 1);
                const Ogre::Real thetaMid = 0.5f * (theta0 + theta1);

                const PipeMeshBuffer buffer = this->bufferForAngle(thetaMid);

                if (true == hideNear && PipeMeshBuffer::NEAR_SIDE == buffer)
                {
                    continue;
                }

                // Outer shell, normals pointing away from the centerline.
                {
                    const Ogre::Vector3 p0 = ringPoint(i, theta0, outerRadius);
                    const Ogre::Vector3 p1 = ringPoint(i, theta1, outerRadius);
                    const Ogre::Vector3 p2 = ringPoint(i + 1, theta1, outerRadius);
                    const Ogre::Vector3 p3 = ringPoint(i + 1, theta0, outerRadius);

                    const Ogre::Vector3 normal = (ringNormal(i, thetaMid) + ringNormal(i + 1, thetaMid)).normalisedCopy();

                    this->addPipeQuad(p0, p1, p2, p3, normal, uAt(j), uAt(j + 1), vA, vB, buffer);
                }

                // Inner shell, normals pointing INWARD - this is the surface the player sees
                // and runs on, and the reason a thickness of 0 leaves the inside backfacing.
                if (true == hasInnerShell)
                {
                    const Ogre::Vector3 p0 = ringPoint(i, theta0, innerRadius);
                    const Ogre::Vector3 p1 = ringPoint(i, theta1, innerRadius);
                    const Ogre::Vector3 p2 = ringPoint(i + 1, theta1, innerRadius);
                    const Ogre::Vector3 p3 = ringPoint(i + 1, theta0, innerRadius);

                    const Ogre::Vector3 normal = -(ringNormal(i, thetaMid) + ringNormal(i + 1, thetaMid)).normalisedCopy();

                    this->addPipeQuad(p0, p1, p2, p3, normal, uAt(j), uAt(j + 1), vA, vB, buffer);
                }
            }
        }

        // ── End rings ────────────────────────────────────────────────────────────────
        // Only meaningful with a wall: they close the gap between the outer and inner shell so
        // the pipe mouth looks like a pipe mouth instead of a paper edge. A single-shell tube
        // is left open on purpose - a disc there would block the player from entering.
        if (true == hasInnerShell)
        {
            auto buildEndRing = [&](size_t ring, const Ogre::Vector3& outwardNormal)
            {
                const Ogre::Real vRing = points[ring].distFromStart * tiling.y;

                for (int j = 0; j < radialCount; ++j)
                {
                    const Ogre::Real theta0 = thetaAt(j);
                    const Ogre::Real theta1 = thetaAt(j + 1);
                    const Ogre::Real thetaMid = 0.5f * (theta0 + theta1);

                    const PipeMeshBuffer buffer = this->bufferForAngle(thetaMid);

                    if (true == hideNear && PipeMeshBuffer::NEAR_SIDE == buffer)
                    {
                        continue;
                    }

                    const Ogre::Vector3 p0 = ringPoint(ring, theta0, outerRadius);
                    const Ogre::Vector3 p1 = ringPoint(ring, theta1, outerRadius);
                    const Ogre::Vector3 p2 = ringPoint(ring, theta1, innerRadius);
                    const Ogre::Vector3 p3 = ringPoint(ring, theta0, innerRadius);

                    this->addPipeQuad(p0, p1, p2, p3, outwardNormal, uAt(j), uAt(j + 1), vRing, vRing + thickness * tiling.y, buffer);
                }
            };

            if (true == capFront)
            {
                buildEndRing(0, -tangents[0]);
            }
            if (true == capBack)
            {
                buildEndRing(numRings - 1, tangents[numRings - 1]);
            }
        }

        // ── Rim strips along the cut edges (Hidden mode only) ────────────────────────
        // Without these the removed arc leaves the wall's cross-section open along the whole
        // length of the tube, and the trough looks like folded paper from any angle that can
        // see the cut. One strip per boundary, running the full length.
        if (true == hideNear && true == hasInnerShell)
        {
            for (int j = 0; j < radialCount; ++j)
            {
                const int jNext = (j + 1) % radialCount;

                const PipeMeshBuffer classThis = this->bufferForAngle(0.5f * (thetaAt(j) + thetaAt(j + 1)));
                const PipeMeshBuffer classNext = this->bufferForAngle(0.5f * (thetaAt(jNext) + thetaAt(jNext + 1)));

                if (classThis == classNext)
                {
                    continue;
                }

                // The boundary sits at the shared angle between the two quads. The rim has to
                // face INTO the removed arc, which is whichever of the two neighbours was the
                // near one.
                const Ogre::Real thetaBoundary = thetaAt(j + 1);
                const bool removedSideIsLower = (PipeMeshBuffer::NEAR_SIDE == classThis);

                for (size_t i = 0; i + 1 < numRings; ++i)
                {
                    const Ogre::Real vA = points[i].distFromStart * tiling.y;
                    const Ogre::Real vB = points[i + 1].distFromStart * tiling.y;

                    const Ogre::Vector3 o0 = ringPoint(i, thetaBoundary, outerRadius);
                    const Ogre::Vector3 i0 = ringPoint(i, thetaBoundary, innerRadius);
                    const Ogre::Vector3 i1 = ringPoint(i + 1, thetaBoundary, innerRadius);
                    const Ogre::Vector3 o1 = ringPoint(i + 1, thetaBoundary, outerRadius);

                    // Tangential direction within the ring - the derivative of the ring
                    // direction with respect to theta, which is exactly the way the rim faces.
                    Ogre::Vector3 rimNormal = (-depthAxes[i] * std::sin(thetaBoundary) + planeNormals[i] * std::cos(thetaBoundary)).normalisedCopy();
                    if (true == removedSideIsLower)
                    {
                        rimNormal = -rimNormal;
                    }

                    this->addPipeQuad(o0, i0, i1, o1, rimNormal, 0.0f, thickness * tiling.x, vA, vB, PipeMeshBuffer::FAR_SIDE);
                }
            }
        }

        // ── Path samples for the Lua API ─────────────────────────────────────────────
        // Appended, not replaced: with several chains the samples of all of them end up in one
        // list, walked in chain order. getDistanceOnPipe projects onto whichever is nearest, so
        // a script tracking the player still gets a sensible answer; only a pipe built as
        // several disjoint runs has a distance axis that jumps at the seams.
        const Ogre::Real baseDistance = this->pathSamples.empty() ? 0.0f : this->pathSamples.back().distance;

        for (size_t i = 0; i < numRings; ++i)
        {
            PipePathSample sample;
            sample.position = centres[i];
            sample.direction = tangents[i];
            sample.distance = baseDistance + points[i].distFromStart;
            this->pathSamples.push_back(sample);
        }
    }

    void ProceduralPipeComponent::generateJunctionHub(const PipeJunction& junction)
    {
        const Ogre::Real pipeR = std::max(0.01f, this->pipeRadius->getReal());
        const Ogre::Real hubR = pipeR * std::max(1.05f, this->junctionHubScale->getReal());
        const Ogre::Real thickness = std::max(0.0f, this->wallThickness->getReal());

        const Ogre::Real innerPipeR = pipeR - thickness;
        const Ogre::Real innerHubR = hubR - thickness;
        const bool hasInnerShell = (thickness > 0.0f && innerPipeR > 0.01f && innerHubR > 0.01f);

        const bool hideNear = (this->getNearSideModeEnum() == NearSideMode::HIDDEN && this->nearSideArc->getReal() > 0.0f);

        const Ogre::Vector2 tiling = this->pipeUVTiling->getVector2();

        // Deliberately finer than the tube's rings. A hole is punched by dropping whole cells,
        // so the boundary is quantised to this grid - at the tube's own resolution the notch
        // where an arm enters would be several degrees wide and plainly visible. A junction
        // sphere is a few hundred triangles either way.
        const int lonSegments = std::max(24, this->radialSegments->getInt());
        const int latSegments = std::max(12, this->radialSegments->getInt() / 2);

        // Angular radius of the hole an arm punches into a shell. An arm of radius r whose axis
        // passes through the centre of a sphere of radius R cuts a circle at asin(r/R) - so the
        // two shells need DIFFERENT hole sizes: asin(r/R) for the outer pair and
        // asin((r-t)/(R-t)) for the inner one. Using one angle for both leaves a ring-shaped gap
        // between the inner tube and the inner sphere, which reads as a see-through slot from
        // inside the pipe, exactly where the player is standing.
        const Ogre::Real margin = Ogre::Degree(3.0f).valueRadians();
        const Ogre::Real outerHoleAngle = std::asin(std::min(0.995f, pipeR / hubR)) + margin;
        const Ogre::Real innerHoleAngle = hasInnerShell ? (std::asin(std::min(0.995f, innerPipeR / innerHubR)) + margin) : outerHoleAngle;

        auto directionAt = [lonSegments, latSegments](int lat, int lon) -> Ogre::Vector3
        {
            const Ogre::Real phi = Ogre::Math::PI * static_cast<Ogre::Real>(lat) / static_cast<Ogre::Real>(latSegments);
            const Ogre::Real lambda = Ogre::Math::TWO_PI * static_cast<Ogre::Real>(lon) / static_cast<Ogre::Real>(lonSegments);

            return Ogre::Vector3(std::sin(phi) * std::cos(lambda), std::cos(phi), std::sin(phi) * std::sin(lambda));
        };

        auto quadDirection = [&directionAt](int lat, int lon) -> Ogre::Vector3
        {
            // Centre direction of the quad spanned by (lat, lon) .. (lat+1, lon+1). Averaging
            // the four corners and renormalising is enough here - the cells are small and only
            // ever used for classification, never for a position.
            Ogre::Vector3 sum = directionAt(lat, lon) + directionAt(lat, lon + 1) + directionAt(lat + 1, lon + 1) + directionAt(lat + 1, lon);
            if (sum.squaredLength() < 1e-9f)
            {
                return directionAt(lat, lon);
            }
            return sum.normalisedCopy();
        };

        auto insideAnyArm = [&junction](const Ogre::Vector3& dir, Ogre::Real holeAngle) -> bool
        {
            const Ogre::Real cosLimit = std::cos(holeAngle);
            for (const Ogre::Vector3& armDir : junction.armDirections)
            {
                if (dir.dotProduct(armDir) >= cosLimit)
                {
                    return true;
                }
            }
            return false;
        };

        // Per-cell bookkeeping, so the rim strips further down can tell WHY a neighbouring cell
        // is missing: a hole is covered by the arm that made it and needs no rim, a near-side
        // cut is an open wall cross-section and does.
        const size_t cellCount = static_cast<size_t>(latSegments) * static_cast<size_t>(lonSegments);
        std::vector<bool> droppedByHole(cellCount, false);
        std::vector<bool> droppedByNearCut(cellCount, false);

        for (int lat = 0; lat < latSegments; ++lat)
        {
            for (int lon = 0; lon < lonSegments; ++lon)
            {
                const size_t cell = static_cast<size_t>(lat) * static_cast<size_t>(lonSegments) + static_cast<size_t>(lon);
                const Ogre::Vector3 dir = quadDirection(lat, lon);

                droppedByHole[cell] = insideAnyArm(dir, outerHoleAngle);
                droppedByNearCut[cell] = (true == hideNear && PipeMeshBuffer::NEAR_SIDE == this->bufferForDirection(dir));
            }
        }

        auto buildShell = [&](Ogre::Real radius, Ogre::Real holeAngle, bool outward)
        {
            for (int lat = 0; lat < latSegments; ++lat)
            {
                for (int lon = 0; lon < lonSegments; ++lon)
                {
                    const Ogre::Vector3 dir = quadDirection(lat, lon);

                    if (true == insideAnyArm(dir, holeAngle))
                    {
                        continue;
                    }

                    const PipeMeshBuffer buffer = this->bufferForDirection(dir);

                    if (true == hideNear && PipeMeshBuffer::NEAR_SIDE == buffer)
                    {
                        continue;
                    }

                    const Ogre::Vector3 d00 = directionAt(lat, lon);
                    const Ogre::Vector3 d01 = directionAt(lat, lon + 1);
                    const Ogre::Vector3 d11 = directionAt(lat + 1, lon + 1);
                    const Ogre::Vector3 d10 = directionAt(lat + 1, lon);

                    const Ogre::Vector3 p0 = junction.centre + d00 * radius;
                    const Ogre::Vector3 p1 = junction.centre + d01 * radius;
                    const Ogre::Vector3 p2 = junction.centre + d11 * radius;
                    const Ogre::Vector3 p3 = junction.centre + d10 * radius;

                    const Ogre::Vector3 normal = outward ? dir : -dir;

                    const Ogre::Real u0 = (static_cast<Ogre::Real>(lon) / static_cast<Ogre::Real>(lonSegments)) * tiling.x;
                    const Ogre::Real u1 = (static_cast<Ogre::Real>(lon + 1) / static_cast<Ogre::Real>(lonSegments)) * tiling.x;
                    const Ogre::Real v0 = (static_cast<Ogre::Real>(lat) / static_cast<Ogre::Real>(latSegments)) * Ogre::Math::PI * radius * tiling.y;
                    const Ogre::Real v1 = (static_cast<Ogre::Real>(lat + 1) / static_cast<Ogre::Real>(latSegments)) * Ogre::Math::PI * radius * tiling.y;

                    // The two polar rows collapse to a point on one side, so one of the two
                    // triangles is zero-area. Harmless - Ogre skips it - and cheaper than a
                    // separate triangle-fan path for two rows out of latSegments.
                    this->addPipeQuad(p0, p1, p2, p3, normal, u0, u1, v0, v1, buffer);
                }
            }
        };

        buildShell(hubR, outerHoleAngle, true);

        if (true == hasInnerShell)
        {
            buildShell(innerHubR, innerHoleAngle, false);
        }

        // ── Rim strips ───────────────────────────────────────────────────────────────
        // Wherever a cell was dropped, the shell's wall cross-section is left open, and these
        // close it. Two reasons a cell can be missing, and BOTH need a rim:
        //
        //   - the near-side cut, same as on the tube: without a rim the hub looks like an
        //     eggshell from any angle that can see the edge.
        //   - an arm hole. The hole boundary is quantised to the grid above, so the inner and
        //     outer shells never cut at exactly the same angle - and the outer shell's hole is
        //     wider than the inner one's by construction (asin(r/R) grows as both shrink by the
        //     wall thickness). Without a rim that mismatch is a slot you can see through from
        //     inside the pipe. With one, the hub is a closed solid shell with tubular holes and
        //     any remaining mismatch is just interpenetration with the arm, which is invisible.
        if (true == hasInnerShell)
        {
            auto cellIndex = [lonSegments](int lat, int lon) -> size_t
            {
                return static_cast<size_t>(lat) * static_cast<size_t>(lonSegments) + static_cast<size_t>(lon);
            };

            auto isDropped = [&](int lat, int lon, bool& outExists) -> bool
            {
                outExists = (lat >= 0 && lat < latSegments);
                if (false == outExists)
                {
                    // Off the top or bottom of the sphere: the cells there collapse into the
                    // pole, so there is no edge to close.
                    return false;
                }
                const int wrappedLon = ((lon % lonSegments) + lonSegments) % lonSegments;
                const size_t cell = cellIndex(lat, wrappedLon);
                return (true == droppedByHole[cell] || true == droppedByNearCut[cell]);
            };

            auto isKept = [&](int lat, int lon) -> bool
            {
                bool exists = false;
                const bool dropped = isDropped(lat, lon, exists);
                return (true == exists && false == dropped);
            };

            auto needsRimTowards = [&](int lat, int lon) -> bool
            {
                bool exists = false;
                const bool dropped = isDropped(lat, lon, exists);
                return (true == exists && true == dropped);
            };

            auto addRim = [&](const Ogre::Vector3& dirA, const Ogre::Vector3& dirB, const Ogre::Vector3& towardsRemoved)
            {
                const Ogre::Vector3 o0 = junction.centre + dirA * hubR;
                const Ogre::Vector3 o1 = junction.centre + dirB * hubR;
                const Ogre::Vector3 i1 = junction.centre + dirB * innerHubR;
                const Ogre::Vector3 i0 = junction.centre + dirA * innerHubR;

                this->addPipeQuad(o0, o1, i1, i0, towardsRemoved, 0.0f, thickness * tiling.x, 0.0f, thickness * tiling.y, PipeMeshBuffer::FAR_SIDE);
            };

            for (int lat = 0; lat < latSegments; ++lat)
            {
                for (int lon = 0; lon < lonSegments; ++lon)
                {
                    if (false == isKept(lat, lon))
                    {
                        continue;
                    }

                    const Ogre::Vector3 own = quadDirection(lat, lon);

                    // Next longitude
                    if (true == needsRimTowards(lat, lon + 1))
                    {
                        addRim(directionAt(lat, lon + 1), directionAt(lat + 1, lon + 1), (quadDirection(lat, (lon + 1) % lonSegments) - own).normalisedCopy());
                    }
                    // Previous longitude
                    if (true == needsRimTowards(lat, lon - 1))
                    {
                        addRim(directionAt(lat, lon), directionAt(lat + 1, lon), (quadDirection(lat, (lon - 1 + lonSegments) % lonSegments) - own).normalisedCopy());
                    }
                    // Next latitude
                    if (true == needsRimTowards(lat + 1, lon))
                    {
                        addRim(directionAt(lat + 1, lon), directionAt(lat + 1, lon + 1), (quadDirection(lat + 1, lon) - own).normalisedCopy());
                    }
                    // Previous latitude
                    if (true == needsRimTowards(lat - 1, lon))
                    {
                        addRim(directionAt(lat, lon), directionAt(lat, lon + 1), (quadDirection(lat - 1, lon) - own).normalisedCopy());
                    }
                }
            }
        }
    }

    void ProceduralPipeComponent::rebuildMesh(void)
    {
        this->farVertices.clear();
        this->farIndices.clear();
        this->currentFarVertexIndex = 0;

        this->nearVertices.clear();
        this->nearIndices.clear();
        this->currentNearVertexIndex = 0;

        this->pathSamples.clear();

        if (true == this->pipeSegments.empty())
        {
            return;
        }

        // Every point's DRAWN depth defaults to its authored one, so nothing is ever left at a
        // meaningless 0 - segment picking and the overlay both read renderZ.
        for (PipeSegment& seg : this->pipeSegments)
        {
            for (PipeControlPoint& cp : seg.controlPoints)
            {
                cp.renderZ = cp.position.z;
            }
        }

        const Ogre::Vector3 origin = this->pipeOrigin;

        const std::map<EndpointKey, std::vector<std::pair<size_t, int>>> endpointMap = this->buildEndpointMap();

        // Three or more arms at one endpoint is a junction. Two is an ordinary joint inside a
        // chain and one is a free end; neither needs anything built.
        std::map<EndpointKey, PipeJunction> junctions;
        for (const auto& entry : endpointMap)
        {
            if (entry.second.size() > 2)
            {
                junctions[entry.first] = PipeJunction();
            }
        }

        // One line per rebuild, not per frame. It exists because a junction that is never
        // DETECTED fails completely silently: the arms simply run through each other and look
        // like a modelling mistake rather than a missing feature. If this says 0 junctions
        // while there visibly is a branch, the branch does not share an endpoint with the pipe
        // it meets - start the branch on the green snap circle.
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
            "[ProceduralPipeComponent] Rebuild: " + Ogre::StringConverter::toString(this->pipeSegments.size()) + " segments, " + Ogre::StringConverter::toString(endpointMap.size()) + " distinct endpoints, " +
                Ogre::StringConverter::toString(junctions.size()) + " junctions (3+ arms)");

        // How much of each arm the hub swallows. Derived, not exposed, and the exact value
        // matters more than it looks:
        //
        // An arm of radius r meeting a sphere of radius R crosses its surface at
        // sqrt(R^2 - r^2) from the centre. Doing the same for the two INNER radii gives a
        // second, smaller crossing distance - and the window between the two is exactly where
        // the arm's mouth has to sit, because that is the band the hub's own wall occupies.
        // Land inside it and the arm's open cross-section is buried in the hub wall, invisible
        // from the bore and from outside alike.
        //
        // Trimming SHORTER than that window (which an innocent-looking 0.9 factor does) pulls
        // the mouth into the hub's hollow chamber, where it is not covered by anything - and
        // with three arms meeting, one arm's mouth ring then reaches straight across the
        // NEXT arm's bore. That is a wall the player runs into, in the middle of the junction.
        const Ogre::Real pipeR = std::max(0.01f, this->pipeRadius->getReal());
        const Ogre::Real hubR = pipeR * std::max(1.05f, this->junctionHubScale->getReal());
        const Ogre::Real wallForTrim = std::min(std::max(0.0f, this->wallThickness->getReal()), pipeR * 0.9f);

        const Ogre::Real outerCrossing = std::sqrt(std::max(0.0f, hubR * hubR - pipeR * pipeR));
        const Ogre::Real innerCrossing = std::sqrt(std::max(0.0f, (hubR - wallForTrim) * (hubR - wallForTrim) - (pipeR - wallForTrim) * (pipeR - wallForTrim)));

        // The midpoint of the window. It is always a valid window: subtracting the same wall
        // thickness from both radii shrinks R^2 - r^2 by 2*t*(R - r), which is positive.
        const Ogre::Real trimDistance = 0.5f * (innerCrossing + outerCrossing);

        // Collected per junction while the arms are swept, then averaged: a depth nudge can make
        // two arms reach the same (x, height) at different z, and the sphere has to sit
        // somewhere sensible between them rather than on top of one arm.
        std::map<EndpointKey, std::vector<Ogre::Vector3>> junctionArmEndpoints;

        const std::vector<std::vector<std::pair<size_t, bool>>> chains = this->buildChains();

        for (const auto& chain : chains)
        {
            // Gather the chain's points as POINTERS into the stored segments, so the depth ramp
            // computed below can be written back where the overlay and the picker read it.
            std::vector<PipeControlPoint*> chainRefs;

            for (const auto& entry : chain)
            {
                PipeSegment& seg = this->pipeSegments[entry.first];
                const bool reversed = entry.second;

                const size_t count = seg.controlPoints.size();
                for (size_t k = 0; k < count; ++k)
                {
                    PipeControlPoint* cp = &seg.controlPoints[reversed ? (count - 1 - k) : k];

                    // The joint point is shared by both segments - keep one copy, or the sweep
                    // gets a zero-length span and a degenerate ring right at every seam.
                    if (0 == k && false == chainRefs.empty())
                    {
                        continue;
                    }
                    chainRefs.push_back(cp);
                }
            }

            if (chainRefs.size() < 2)
            {
                continue;
            }

            const EndpointKey frontKey = makeEndpointKey(*chainRefs.front());
            const EndpointKey backKey = makeEndpointKey(*chainRefs.back());

            const bool frontIsJunction = (junctions.find(frontKey) != junctions.end());
            const bool backIsJunction = (junctions.find(backKey) != junctions.end());

            // ── Depth ramp ───────────────────────────────────────────────────────────
            // Only the chain's two END offsets are read; everything between is interpolated by
            // arc length. That is what turns a nudged loop into a gentle helix instead of a
            // chain with a step in it - and it is why nudging a MIDDLE segment does nothing.
            {
                std::vector<Ogre::Real> accumulated(chainRefs.size(), 0.0f);
                for (size_t i = 1; i < chainRefs.size(); ++i)
                {
                    accumulated[i] = accumulated[i - 1] + pipePathDistance2D(chainRefs[i - 1]->position.x, chainRefs[i - 1]->smoothedHeight, chainRefs[i]->position.x, chainRefs[i]->smoothedHeight);
                }

                const Ogre::Real total = accumulated.back();
                const Ogre::Real zStart = chainRefs.front()->position.z;
                const Ogre::Real zEnd = chainRefs.back()->position.z;

                for (size_t i = 0; i < chainRefs.size(); ++i)
                {
                    const Ogre::Real t = (total > 1e-6f) ? (accumulated[i] / total) : 0.0f;
                    chainRefs[i]->renderZ = zStart + (zEnd - zStart) * t;
                }
            }

            // ── Local-space copy ─────────────────────────────────────────────────────
            std::vector<PipeControlPoint> localPoints;
            localPoints.reserve(chainRefs.size());

            for (const PipeControlPoint* cp : chainRefs)
            {
                PipeControlPoint local;
                local.position = Ogre::Vector3(cp->position.x - origin.x, 0.0f, cp->renderZ - origin.z);
                local.renderZ = cp->renderZ - origin.z;
                local.rawHeight = cp->smoothedHeight - origin.y;
                local.smoothedHeight = local.rawHeight;
                localPoints.push_back(local);
            }

            this->smoothHeightTransitions(localPoints);

            std::vector<PipeControlPoint> densePath = this->subdivideWithHeightInterpolation(localPoints);

            // Ring spacing scales with the radius: a fat pipe needs fewer rings per meter to
            // look round than a thin one does.
            const Ogre::Real step = std::max(0.2f, pipeR * 0.5f);
            densePath = this->resamplePathUniformly(densePath, step);

            if (densePath.size() < 2)
            {
                continue;
            }

            // The untrimmed ends ARE the junction positions, seen from this arm.
            const Ogre::Vector3 frontEndpoint(densePath.front().position.x, densePath.front().smoothedHeight, densePath.front().renderZ);
            const Ogre::Vector3 backEndpoint(densePath.back().position.x, densePath.back().smoothedHeight, densePath.back().renderZ);

            if (true == frontIsJunction)
            {
                junctionArmEndpoints[frontKey].push_back(frontEndpoint);
            }
            if (true == backIsJunction)
            {
                junctionArmEndpoints[backKey].push_back(backEndpoint);
            }

            const std::vector<PipeControlPoint> trimmedPath = this->trimPathEnds(densePath, frontIsJunction ? trimDistance : 0.0f, backIsJunction ? trimDistance : 0.0f);

            if (trimmedPath.size() < 2)
            {
                continue;
            }

            // Arm directions point AWAY from the junction, taken from the trimmed path so they
            // describe where the tube actually goes rather than where it was authored.
            if (true == frontIsJunction)
            {
                const Ogre::Vector3 a(trimmedPath[0].position.x, trimmedPath[0].smoothedHeight, trimmedPath[0].renderZ);
                const Ogre::Vector3 b(trimmedPath[1].position.x, trimmedPath[1].smoothedHeight, trimmedPath[1].renderZ);
                const Ogre::Vector3 dir = b - a;
                if (dir.squaredLength() > 1e-9f)
                {
                    junctions[frontKey].armDirections.push_back(dir.normalisedCopy());
                }
            }
            if (true == backIsJunction)
            {
                const Ogre::Vector3 a(trimmedPath[trimmedPath.size() - 1].position.x, trimmedPath[trimmedPath.size() - 1].smoothedHeight, trimmedPath[trimmedPath.size() - 1].renderZ);
                const Ogre::Vector3 b(trimmedPath[trimmedPath.size() - 2].position.x, trimmedPath[trimmedPath.size() - 2].smoothedHeight, trimmedPath[trimmedPath.size() - 2].renderZ);
                const Ogre::Vector3 dir = a - b;
                if (dir.squaredLength() > 1e-9f)
                {
                    junctions[backKey].armDirections.push_back(dir.normalisedCopy());
                }
            }

            // NO cap at a junction end. The cap is an annulus rather than a disc, so it does
            // not block its OWN bore - but it is a full ring standing across the tube axis, and
            // at a junction the next arm's bore passes right through where that ring sits. The
            // player then walks into a ring-shaped wall inside the hub.
            //
            // Leaving it off is safe precisely because of the trim distance above: the arm's
            // open cross-section ends up inside the hub's wall band, where the hub's own two
            // shells close it from both sides.
            this->generatePipeRings(trimmedPath, false == frontIsJunction, false == backIsJunction);

            if (true == frontIsJunction || true == backIsJunction)
            {
                // One line per arm per rebuild - enough to check a junction actually came out
                // the way the numbers say, without drowning the log.
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
                    "[ProceduralPipeComponent] Junction arm: trim=" + Ogre::StringConverter::toString(trimDistance) + " mouth inside hub wall band [" + Ogre::StringConverter::toString(hubR - wallForTrim) + ", " +
                        Ogre::StringConverter::toString(hubR) + "] at " + Ogre::StringConverter::toString(std::sqrt(trimDistance * trimDistance + (pipeR - wallForTrim) * (pipeR - wallForTrim))) + ".." +
                        Ogre::StringConverter::toString(std::sqrt(trimDistance * trimDistance + pipeR * pipeR)));
            }
        }

        for (auto& entry : junctions)
        {
            PipeJunction& junction = entry.second;

            const auto endpointsIt = junctionArmEndpoints.find(entry.first);
            if (endpointsIt == junctionArmEndpoints.end() || endpointsIt->second.empty())
            {
                continue;
            }

            // Fewer than three arms actually arrived - one of them was too short to survive
            // trimming, or two arms of the junction belong to the same chain. A sphere there
            // would be a bulge for no reason.
            if (junction.armDirections.size() < 3)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL,
                    "[ProceduralPipeComponent] Junction detected but SKIPPED: only " + Ogre::StringConverter::toString(junction.armDirections.size()) +
                        " arms reached it. Usually one of the arms is shorter than the hub is wide, so it could not be trimmed back.");
                continue;
            }

            Ogre::Vector3 sum = Ogre::Vector3::ZERO;
            for (const Ogre::Vector3& endpoint : endpointsIt->second)
            {
                sum += endpoint;
            }
            junction.centre = sum / static_cast<Ogre::Real>(endpointsIt->second.size());

            this->generateJunctionHub(junction);

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
                "[ProceduralPipeComponent] Junction hub built: " + Ogre::StringConverter::toString(junction.armDirections.size()) + " arms, centre " + Ogre::StringConverter::toString(junction.centre) + ", hub radius " +
                    Ogre::StringConverter::toString(hubR) + " (bore stays " + Ogre::StringConverter::toString(pipeR - wallForTrim) + ", hub chamber " + Ogre::StringConverter::toString(hubR - wallForTrim) +
                    " - the difference is how deep the floor dips at this junction)");
        }

        this->createPipeMesh();
    }

    void ProceduralPipeComponent::createPipeMesh(void)
    {
        this->destroyPipeMesh();

        if (0 == this->currentFarVertexIndex && 0 == this->currentNearVertexIndex)
        {
            return;
        }

        const auto& physicsArtifactCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsArtifactComponent>());
        if (physicsArtifactCompPtr)
        {
            this->physicsArtifactComponent = physicsArtifactCompPtr.get();
        }

        this->cachedFarVertices = this->farVertices;
        this->cachedFarIndices = this->farIndices;
        this->cachedNumFarVertices = this->currentFarVertexIndex;

        this->cachedNearVertices = this->nearVertices;
        this->cachedNearIndices = this->nearIndices;
        this->cachedNumNearVertices = this->currentNearVertexIndex;

        this->cachedPipeOrigin = this->pipeOrigin;

        const std::vector<float> farVerticesCopy = this->farVertices;
        const std::vector<Ogre::uint32> farIndicesCopy = this->farIndices;
        const size_t numFarVertices = this->currentFarVertexIndex;

        const std::vector<float> nearVerticesCopy = this->nearVertices;
        const std::vector<Ogre::uint32> nearIndicesCopy = this->nearIndices;
        const size_t numNearVertices = this->currentNearVertexIndex;

        const Ogre::Vector3 pipeOriginCopy = this->pipeOrigin;

        GraphicsModule::RenderCommand renderCommand = [this, farVerticesCopy, farIndicesCopy, numFarVertices, nearVerticesCopy, nearIndicesCopy, numNearVertices, pipeOriginCopy]()
        {
            this->createPipeMeshInternal(farVerticesCopy, farIndicesCopy, numFarVertices, nearVerticesCopy, nearIndicesCopy, numNearVertices, pipeOriginCopy);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralPipeComponent::createPipeMesh");

        this->farVertices.clear();
        this->farIndices.clear();
        this->nearVertices.clear();
        this->nearIndices.clear();
    }

    void ProceduralPipeComponent::createPipeMeshInternal(const std::vector<float>& farVerts, const std::vector<Ogre::uint32>& farInds, size_t numFarVerts, const std::vector<float>& nearVerts, const std::vector<Ogre::uint32>& nearInds,
        size_t numNearVerts, const Ogre::Vector3& origin)
    {
        Ogre::Root* root = Ogre::Root::getSingletonPtr();
        Ogre::RenderSystem* renderSystem = root->getRenderSystem();
        Ogre::VaoManager* vaoManager = renderSystem->getVaoManager();

        const Ogre::String meshName = this->gameObjectPtr->getName() + "_Pipe_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
        const Ogre::String groupName = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

        {
            Ogre::MeshManager& meshMgr = Ogre::MeshManager::getSingleton();
            Ogre::MeshPtr existing = meshMgr.getByName(meshName, groupName);
            if (false == existing.isNull())
            {
                meshMgr.remove(existing->getHandle());
            }
        }

        this->pipeMesh = Ogre::MeshManager::getSingleton().createManual(meshName, groupName, &NOWA::gDummyMeshLoader);

        Ogre::VertexElement2Vec vertexElements;
        vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
        vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));
        vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT4, Ogre::VES_TANGENT));
        vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));

        const size_t srcFloatsPerVertex = 8;
        const size_t dstFloatsPerVertex = 12;

        Ogre::Vector3 minBounds(std::numeric_limits<float>::max());
        Ogre::Vector3 maxBounds(std::numeric_limits<float>::lowest());

        // A submesh is only created when it actually has geometry. An empty submesh needs a
        // dummy VAO with zero indices to keep Ogre from crashing, and anything that later walks
        // the submeshes and rebuilds them (MeshModifyComponent, for one) dies on that zero-byte
        // buffer - which is exactly the trap the platform component documents at length. With
        // Near Side Mode = Hidden there is genuinely nothing in the near buffer, so this is not
        // a hypothetical case here.
        auto buildSubMesh = [&](const std::vector<float>& verts, const std::vector<Ogre::uint32>& inds, size_t numVerts) -> bool
        {
            if (0 == numVerts || true == inds.empty())
            {
                return false;
            }

            Ogre::SubMesh* subMesh = this->pipeMesh->createSubMesh();

            const size_t vertexDataSize = numVerts * dstFloatsPerVertex * sizeof(float);
            float* vertexData = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(vertexDataSize, Ogre::MEMCATEGORY_GEOMETRY));

            for (size_t i = 0; i < numVerts; ++i)
            {
                const size_t srcOffset = i * srcFloatsPerVertex;
                const size_t dstOffset = i * dstFloatsPerVertex;

                vertexData[dstOffset + 0] = verts[srcOffset + 0];
                vertexData[dstOffset + 1] = verts[srcOffset + 1];
                vertexData[dstOffset + 2] = verts[srcOffset + 2];

                const Ogre::Vector3 pos(verts[srcOffset + 0], verts[srcOffset + 1], verts[srcOffset + 2]);
                minBounds.makeFloor(pos);
                maxBounds.makeCeil(pos);

                const Ogre::Vector3 normal(verts[srcOffset + 3], verts[srcOffset + 4], verts[srcOffset + 5]);
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
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPipeComponent] Vertex buffer creation failed: " + e.getDescription());
                return false;
            }

            const size_t indexDataSize = inds.size() * sizeof(Ogre::uint32);
            Ogre::uint32* indexData = reinterpret_cast<Ogre::uint32*>(OGRE_MALLOC_SIMD(indexDataSize, Ogre::MEMCATEGORY_GEOMETRY));
            memcpy(indexData, inds.data(), indexDataSize);

            Ogre::IndexBufferPacked* indexBuffer = vaoManager->createIndexBuffer(Ogre::IndexBufferPacked::IT_32BIT, inds.size(), Ogre::BT_IMMUTABLE, indexData, true);

            Ogre::VertexBufferPackedVec vertexBuffers;
            vertexBuffers.push_back(vertexBuffer);

            Ogre::VertexArrayObject* vao = vaoManager->createVertexArrayObject(vertexBuffers, indexBuffer, Ogre::OT_TRIANGLE_LIST);

            subMesh->mVao[Ogre::VpNormal].push_back(vao);
            subMesh->mVao[Ogre::VpShadow].push_back(vao);

            return true;
        };

        // Submesh 0 is the pipe body, submesh 1 the near arc - in that order whenever both
        // exist, which applyDatablocks relies on.
        const bool hasFar = buildSubMesh(farVerts, farInds, numFarVerts);
        const bool hasNear = buildSubMesh(nearVerts, nearInds, numNearVerts);

        if (false == hasFar && false == hasNear)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPipeComponent] Nothing to build - no geometry in either buffer");
            return;
        }

        if (minBounds.x == std::numeric_limits<float>::max())
        {
            minBounds = Ogre::Vector3(-1, -1, -1);
            maxBounds = Ogre::Vector3(1, 1, 1);
        }

        Ogre::Aabb bounds;
        bounds.setExtents(minBounds, maxBounds);
        this->pipeMesh->_setBounds(bounds, false);
        this->pipeMesh->_setBoundingSphereRadius(bounds.getRadius());

        this->pipeItem = this->gameObjectPtr->getSceneManager()->createItem(this->pipeMesh, this->gameObjectPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC);

        this->applyDatablocks();

        if (false == this->originPositionSet)
        {
            this->originPositionSet = true;
            this->gameObjectPtr->getSceneNode()->setPosition(this->pipeFrame * origin);
            this->gameObjectPtr->getSceneNode()->setOrientation(this->pipeFrame);
        }

        this->gameObjectPtr->getSceneNode()->attachObject(this->pipeItem);
        this->gameObjectPtr->setDoNotDestroyMovableObject(true);
        this->gameObjectPtr->init(this->pipeItem);

        if (nullptr != this->physicsArtifactComponent)
        {
            this->physicsArtifactComponent->reCreateCollision();
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
            "[ProceduralPipeComponent] Pipe mesh created with " + Ogre::StringConverter::toString(numFarVerts) + " body vertices and " + Ogre::StringConverter::toString(numNearVerts) + " near-side vertices");
    }

    void ProceduralPipeComponent::applyDatablocks(void)
    {
        if (nullptr == this->pipeItem)
        {
            return;
        }

        Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingleton().getHlmsManager();

        const Ogre::String bodyName = this->pipeDatablock->getString();
        Ogre::HlmsDatablock* bodyDatablock = (false == bodyName.empty()) ? hlmsManager->getDatablockNoDefault(bodyName) : nullptr;

        Ogre::HlmsDatablock* nearSideDatablock = this->resolveNearSideDatablock();

        const size_t numSubItems = this->pipeItem->getNumSubItems();

        if (numSubItems >= 1 && nullptr != bodyDatablock)
        {
            this->pipeItem->getSubItem(0)->setDatablock(bodyDatablock);
        }
        // Submesh 1 only exists when the near buffer had geometry, i.e. not in Hidden mode.
        if (numSubItems >= 2 && nullptr != nearSideDatablock)
        {
            this->pipeItem->getSubItem(1)->setDatablock(nearSideDatablock);
        }
    }

    Ogre::HlmsDatablock* ProceduralPipeComponent::resolveNearSideDatablock(void)
    {
        Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingleton().getHlmsManager();

        const Ogre::String bodyName = this->pipeDatablock->getString();
        const Ogre::String nearName = this->nearDatablock->getString();

        Ogre::HlmsDatablock* bodyDatablock = (false == bodyName.empty()) ? hlmsManager->getDatablockNoDefault(bodyName) : nullptr;
        Ogre::HlmsDatablock* explicitNear = (false == nearName.empty()) ? hlmsManager->getDatablockNoDefault(nearName) : nullptr;

        if (false == nearName.empty() && nullptr == explicitNear)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[ProceduralPipeComponent] Near datablock '" + nearName + "' not found - falling back to the body datablock");
        }

        // Anything other than Transparent: no clone should be alive, and the near arc simply
        // uses whatever material it was given. That is what makes Solid <-> Transparent a
        // material swap rather than a rebuild.
        if (NearSideMode::FADE != this->getNearSideModeEnum())
        {
            this->destroyClonedNearDatablock();
            return (nullptr != explicitNear) ? explicitNear : bodyDatablock;
        }

        Ogre::HlmsDatablock* source = (nullptr != explicitNear) ? explicitNear : bodyDatablock;
        if (nullptr == source)
        {
            this->destroyClonedNearDatablock();
            return nullptr;
        }

        const Ogre::Real alpha = Ogre::Math::Clamp(this->nearSideAlpha->getReal(), 0.0f, 1.0f);

        // Alpha 1 is the explicit opt-out: use the assigned datablock exactly as it was
        // authored. That is the path for a hand-made transparent material that already has its
        // blendblock, fresnel and alpha test set up the way its author wants.
        if (alpha >= 1.0f)
        {
            this->destroyClonedNearDatablock();
            return source;
        }

        Ogre::HlmsPbsDatablock* sourcePbs = dynamic_cast<Ogre::HlmsPbsDatablock*>(source);
        if (nullptr == sourcePbs)
        {
            // Unlit, Wind or any other Hlms: no setTransparency to call, so the arc stays as it
            // is rather than being silently left looking like Solid with no explanation.
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL,
                "[ProceduralPipeComponent] Near side datablock is not a PBS datablock - transparency cannot be applied, the near arc stays opaque");
            this->destroyClonedNearDatablock();
            return source;
        }

        const Ogre::String sourceName = *sourcePbs->getNameStr();

        // The clone is what carries the transparency. Setting it on the source would fade the
        // pipe body too whenever both share a material - which is the normal case, since Near
        // Datablock is usually left empty - and would leak out to every other object in the
        // scene using that same datablock.
        if (false == this->clonedNearDatablockName.empty() && this->clonedNearSourceName != sourceName)
        {
            // The source changed under us, so the old clone carries the wrong textures.
            this->destroyClonedNearDatablock();
        }

        Ogre::HlmsPbsDatablock* clonePbs = nullptr;

        if (false == this->clonedNearDatablockName.empty())
        {
            clonePbs = dynamic_cast<Ogre::HlmsPbsDatablock*>(hlmsManager->getDatablockNoDefault(this->clonedNearDatablockName));
            if (nullptr == clonePbs)
            {
                // Someone destroyed it behind our back - forget the name and make a new one.
                this->clonedNearDatablockName.clear();
                this->clonedNearSourceName.clear();
            }
        }

        if (nullptr == clonePbs)
        {
            const Ogre::String cloneName = "ProceduralPipeNear_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());

            // A clone may already exist from an earlier session of this very component (the
            // name is derived from the GameObject id, so it survives a rebuild).
            Ogre::HlmsDatablock* existing = hlmsManager->getDatablockNoDefault(cloneName);
            if (nullptr != existing)
            {
                clonePbs = dynamic_cast<Ogre::HlmsPbsDatablock*>(existing);
            }

            if (nullptr == clonePbs)
            {
                try
                {
                    clonePbs = dynamic_cast<Ogre::HlmsPbsDatablock*>(sourcePbs->clone(cloneName));
                }
                catch (Ogre::Exception& e)
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPipeComponent] Could not clone near side datablock: " + e.getDescription());
                    return source;
                }
            }

            if (nullptr == clonePbs)
            {
                return source;
            }

            this->clonedNearDatablockName = cloneName;
            this->clonedNearSourceName = sourceName;
        }

        // useAlphaFromTextures stays false: the alpha wanted here is the one from this
        // attribute, and a diffuse texture with an opaque alpha channel would otherwise
        // override it and leave the arc looking untouched.
        clonePbs->setTransparency(alpha, Ogre::HlmsPbsDatablock::Transparent, false);

        return clonePbs;
    }

    void ProceduralPipeComponent::destroyClonedNearDatablock(void)
    {
        if (true == this->clonedNearDatablockName.empty())
        {
            return;
        }

        Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingleton().getHlmsManager();
        Ogre::HlmsDatablock* clone = hlmsManager->getDatablockNoDefault(this->clonedNearDatablockName);

        // Point the near submesh somewhere else FIRST. Destroying a datablock an Item still
        // references is a crash, not a leak.
        if (nullptr != this->pipeItem && this->pipeItem->getNumSubItems() >= 2)
        {
            const Ogre::String bodyName = this->pipeDatablock->getString();
            Ogre::HlmsDatablock* bodyDatablock = (false == bodyName.empty()) ? hlmsManager->getDatablockNoDefault(bodyName) : nullptr;

            if (nullptr != bodyDatablock)
            {
                this->pipeItem->getSubItem(1)->setDatablock(bodyDatablock);
            }
        }

        if (nullptr != clone && nullptr != clone->getCreator())
        {
            try
            {
                clone->getCreator()->destroyDatablock(this->clonedNearDatablockName);
            }
            catch (Ogre::Exception& e)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPipeComponent] Could not destroy cloned near datablock: " + e.getDescription());
            }
        }

        this->clonedNearDatablockName.clear();
        this->clonedNearSourceName.clear();
    }

    void ProceduralPipeComponent::destroyPipeMesh(void)
    {
        if (nullptr == this->pipeItem && nullptr == this->pipeMesh)
        {
            return;
        }

        GraphicsModule::RenderCommand renderCommand = [this]()
        {
            if (nullptr != this->pipeItem)
            {
                if (this->pipeItem->getParentSceneNode())
                {
                    this->pipeItem->getParentSceneNode()->detachObject(this->pipeItem);
                }
                this->gameObjectPtr->getSceneManager()->destroyItem(this->pipeItem);
                this->pipeItem = nullptr;
                this->gameObjectPtr->nullMovableObject();
            }

            if (this->pipeMesh)
            {
                Ogre::MeshManager::getSingleton().remove(this->pipeMesh->getHandle());
                this->pipeMesh.reset();
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralPipeComponent::destroyPipeMesh");
    }

    void ProceduralPipeComponent::destroyPreviewMesh(void)
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
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralPipeComponent::destroyPreviewMesh");
    }

    void ProceduralPipeComponent::updatePreviewMesh(void)
    {
        if (this->currentSegment.controlPoints.size() < 2)
        {
            return;
        }

        if (nullptr == this->previewNode)
        {
            this->previewNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();
        }

        // The preview is swept by the SAME code as the real thing, into the same two buffers,
        // so what the user drags is what they get - including the near-side cut. The buffers
        // are then merged into one submesh, since a preview needs one material, not two.
        this->farVertices.clear();
        this->farIndices.clear();
        this->currentFarVertexIndex = 0;

        this->nearVertices.clear();
        this->nearIndices.clear();
        this->currentNearVertexIndex = 0;

        const std::vector<PipePathSample> savedSamples = this->pathSamples;

        const Ogre::Real startX = this->currentSegment.controlPoints.front().position.x;
        const Ogre::Real startHeight = this->currentSegment.controlPoints.front().smoothedHeight;
        const Ogre::Real startZ = this->currentSegment.controlPoints.front().position.z;

        std::vector<PipeControlPoint> localPoints;
        Ogre::Real accumDist = 0.0f;
        for (size_t i = 0; i < this->currentSegment.controlPoints.size(); ++i)
        {
            const PipeControlPoint& cp = this->currentSegment.controlPoints[i];

            PipeControlPoint localPoint;
            localPoint.position = Ogre::Vector3(cp.position.x - startX, 0.0f, cp.position.z - startZ);
            localPoint.renderZ = cp.position.z - startZ;
            localPoint.rawHeight = cp.smoothedHeight - startHeight;
            localPoint.smoothedHeight = localPoint.rawHeight;

            if (i > 0)
            {
                accumDist += pipePathDistance2D(localPoints.back().position.x, localPoints.back().smoothedHeight, localPoint.position.x, localPoint.smoothedHeight);
            }
            localPoint.distFromStart = accumDist;

            localPoints.push_back(localPoint);
        }

        const Ogre::Real step = std::max(0.2f, this->pipeRadius->getReal() * 0.5f);
        std::vector<PipeControlPoint> densePath = this->resamplePathUniformly(this->subdivideWithHeightInterpolation(localPoints), step);

        this->generatePipeRings(densePath, true, true);

        // The preview must not leave its samples behind in the real path - a drag would
        // otherwise keep appending to the query path and getPipeLength would grow with every
        // mouse move.
        this->pathSamples = savedSamples;

        if (this->farVertices.empty() && this->nearVertices.empty())
        {
            return;
        }

        std::vector<float> combinedVertices = this->farVertices;
        std::vector<Ogre::uint32> combinedIndices = this->farIndices;

        const size_t vertexOffset = this->currentFarVertexIndex;
        combinedVertices.insert(combinedVertices.end(), this->nearVertices.begin(), this->nearVertices.end());
        for (const auto& idx : this->nearIndices)
        {
            combinedIndices.push_back(idx + static_cast<Ogre::uint32>(vertexOffset));
        }

        const size_t totalVertices = this->currentFarVertexIndex + this->currentNearVertexIndex;

        const Ogre::Vector3 previewPosition(startX, startHeight, startZ);

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

            const Ogre::String meshName = "PipePreview_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
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
                const size_t srcOffset = i * srcFloatsPerVertex;
                const size_t dstOffset = i * dstFloatsPerVertex;

                vertexData[dstOffset + 0] = combinedVertices[srcOffset + 0];
                vertexData[dstOffset + 1] = combinedVertices[srcOffset + 1];
                vertexData[dstOffset + 2] = combinedVertices[srcOffset + 2];

                const Ogre::Vector3 normal(combinedVertices[srcOffset + 3], combinedVertices[srcOffset + 4], combinedVertices[srcOffset + 5]);
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
                const size_t offset = i * srcFloatsPerVertex;
                const Ogre::Vector3 pos(combinedVertices[offset + 0], combinedVertices[offset + 1], combinedVertices[offset + 2]);
                minBounds.makeFloor(pos);
                maxBounds.makeCeil(pos);
            }

            Ogre::Aabb bounds;
            bounds.setExtents(minBounds, maxBounds);
            this->previewMesh->_setBounds(bounds, false);
            this->previewMesh->_setBoundingSphereRadius(bounds.getRadius());

            this->previewItem = this->gameObjectPtr->getSceneManager()->createItem(this->previewMesh, Ogre::SCENE_DYNAMIC);

            this->previewNode->setPosition(this->pipeFrame * previewPosition);
            this->previewNode->setOrientation(this->pipeFrame);
            this->previewNode->attachObject(this->previewItem);

            const Ogre::String dbName = this->pipeDatablock->getString();
            if (false == dbName.empty())
            {
                Ogre::HlmsDatablock* db = Ogre::Root::getSingletonPtr()->getHlmsManager()->getDatablockNoDefault(dbName);
                if (nullptr != db)
                {
                    this->previewItem->getSubItem(0)->setDatablock(db);
                }
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralPipeComponent::updatePreviewMesh");
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Attribute access
    ///////////////////////////////////////////////////////////////////////////////////////////////

    void ProceduralPipeComponent::setActivated(bool activated)
    {
        this->activated->setValue(activated);
        this->updateModificationState();
    }

    bool ProceduralPipeComponent::isActivated(void) const
    {
        return this->activated->getBool();
    }

    void ProceduralPipeComponent::setPipeRadius(Ogre::Real radius)
    {
        this->pipeRadius->setValue(std::max(0.05f, radius));
        this->snapRadius = std::max(1.0f, this->pipeRadius->getReal());

        if (false == this->pipeSegments.empty())
        {
            this->rebuildMesh();
        }
    }

    Ogre::Real ProceduralPipeComponent::getPipeRadius(void) const
    {
        return this->pipeRadius->getReal();
    }

    void ProceduralPipeComponent::setWallThickness(Ogre::Real thickness)
    {
        // Clamped below the radius: a wall as thick as the pipe leaves no bore for the player,
        // and the inner shell would turn inside out.
        const Ogre::Real maxThickness = this->pipeRadius->getReal() * 0.9f;
        this->wallThickness->setValue(Ogre::Math::Clamp(thickness, 0.0f, maxThickness));

        if (false == this->pipeSegments.empty())
        {
            this->rebuildMesh();
        }
    }

    Ogre::Real ProceduralPipeComponent::getWallThickness(void) const
    {
        return this->wallThickness->getReal();
    }

    void ProceduralPipeComponent::setRadialSegments(int segments)
    {
        this->radialSegments->setValue(Ogre::Math::Clamp(segments, 3, 128));

        if (false == this->pipeSegments.empty())
        {
            this->rebuildMesh();
        }
    }

    int ProceduralPipeComponent::getRadialSegments(void) const
    {
        return this->radialSegments->getInt();
    }

    void ProceduralPipeComponent::setNearSideMode(const Ogre::String& mode)
    {
        const NearSideMode previousMode = this->getNearSideModeEnum();

        this->nearSideMode->setListSelectedValue(mode);

        const NearSideMode newMode = this->getNearSideModeEnum();

        if (previousMode == newMode)
        {
            return;
        }

        if (true == this->pipeSegments.empty())
        {
            return;
        }

        // Hidden is the only mode that changes which triangles exist, so it is the only one
        // that costs a rebuild. Solid <-> Transparent is a material swap on an existing
        // submesh - safe to call every frame from a script if someone wants to fade the tube
        // in and out as the player enters it.
        const bool involvesHidden = (NearSideMode::HIDDEN == previousMode || NearSideMode::HIDDEN == newMode);

        if (true == involvesHidden)
        {
            this->rebuildMesh();
        }
        else
        {
            GraphicsModule::RenderCommand renderCommand = [this]()
            {
                this->applyDatablocks();
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralPipeComponent::setNearSideMode");
        }
    }

    Ogre::String ProceduralPipeComponent::getNearSideMode(void) const
    {
        return this->nearSideMode->getListSelectedValue();
    }

    ProceduralPipeComponent::NearSideMode ProceduralPipeComponent::getNearSideModeEnum(void) const
    {
        const Ogre::String value = this->nearSideMode->getListSelectedValue();

        if ("Transparent" == value)
        {
            return NearSideMode::FADE;
        }
        if ("Hidden" == value)
        {
            return NearSideMode::HIDDEN;
        }
        return NearSideMode::SOLID;
    }

    void ProceduralPipeComponent::setNearSideArc(Ogre::Real degrees)
    {
        this->nearSideArc->setValue(Ogre::Math::Clamp(degrees, 0.0f, 340.0f));

        // The arc decides which quad lands in which buffer, so it is a geometry change in
        // every mode, not just Hidden.
        if (false == this->pipeSegments.empty())
        {
            this->rebuildMesh();
        }
    }

    Ogre::Real ProceduralPipeComponent::getNearSideArc(void) const
    {
        return this->nearSideArc->getReal();
    }

    void ProceduralPipeComponent::setJunctionHubScale(Ogre::Real scale)
    {
        this->junctionHubScale->setValue(Ogre::Math::Clamp(scale, 1.05f, 3.0f));

        // Changes both the sphere AND how far each arm is trimmed back, so this is geometry.
        if (false == this->pipeSegments.empty())
        {
            this->rebuildMesh();
        }
    }

    Ogre::Real ProceduralPipeComponent::getJunctionHubScale(void) const
    {
        return this->junctionHubScale->getReal();
    }

    void ProceduralPipeComponent::setNearSideAlpha(Ogre::Real alpha)
    {
        this->nearSideAlpha->setValue(Ogre::Math::Clamp(alpha, 0.0f, 1.0f));

        // Pure material change - no geometry involved, so this is safe to drive per frame from
        // a script that fades the tube as the player enters it.
        GraphicsModule::RenderCommand renderCommand = [this]()
        {
            this->applyDatablocks();
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralPipeComponent::setNearSideAlpha");
    }

    Ogre::Real ProceduralPipeComponent::getNearSideAlpha(void) const
    {
        return this->nearSideAlpha->getReal();
    }

    void ProceduralPipeComponent::setInvertNearSide(bool invert)
    {
        this->invertNearSide->setValue(invert);

        // Which quad lands in which buffer changes, so this is geometry, not material.
        if (false == this->pipeSegments.empty())
        {
            this->rebuildMesh();
        }
    }

    bool ProceduralPipeComponent::getInvertNearSide(void) const
    {
        return this->invertNearSide->getBool();
    }

    void ProceduralPipeComponent::setSnapToGrid(bool snap)
    {
        this->snapToGrid->setValue(snap);
    }

    bool ProceduralPipeComponent::getSnapToGrid(void) const
    {
        return this->snapToGrid->getBool();
    }

    void ProceduralPipeComponent::setGridSize(Ogre::Real size)
    {
        this->gridSize->setValue(std::max(0.01f, size));
    }

    Ogre::Real ProceduralPipeComponent::getGridSize(void) const
    {
        return this->gridSize->getReal();
    }

    void ProceduralPipeComponent::setSmoothingFactor(Ogre::Real factor)
    {
        this->smoothingFactor->setValue(Ogre::Math::Clamp(factor, 0.0f, 1.0f));

        if (false == this->pipeSegments.empty())
        {
            this->rebuildMesh();
        }
    }

    Ogre::Real ProceduralPipeComponent::getSmoothingFactor(void) const
    {
        return this->smoothingFactor->getReal();
    }

    void ProceduralPipeComponent::setCurveSubdivisions(int subdivisions)
    {
        this->curveSubdivisions->setValue(Ogre::Math::Clamp(subdivisions, 1, 100));

        if (false == this->pipeSegments.empty())
        {
            this->rebuildMesh();
        }
    }

    int ProceduralPipeComponent::getCurveSubdivisions(void) const
    {
        return this->curveSubdivisions->getInt();
    }

    void ProceduralPipeComponent::setPipeDatablock(const Ogre::String& datablock)
    {
        this->pipeDatablock->setValue(datablock);

        GraphicsModule::RenderCommand renderCommand = [this]()
        {
            this->applyDatablocks();
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralPipeComponent::setPipeDatablock");
    }

    Ogre::String ProceduralPipeComponent::getPipeDatablock(void) const
    {
        return this->pipeDatablock->getString();
    }

    void ProceduralPipeComponent::setNearDatablock(const Ogre::String& datablock)
    {
        this->nearDatablock->setValue(datablock);

        GraphicsModule::RenderCommand renderCommand = [this]()
        {
            this->applyDatablocks();
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralPipeComponent::setNearDatablock");
    }

    Ogre::String ProceduralPipeComponent::getNearDatablock(void) const
    {
        return this->nearDatablock->getString();
    }

    void ProceduralPipeComponent::setPipeUVTiling(const Ogre::Vector2& tiling)
    {
        this->pipeUVTiling->setValue(tiling);

        if (false == this->pipeSegments.empty())
        {
            this->rebuildMesh();
        }
    }

    Ogre::Vector2 ProceduralPipeComponent::getPipeUVTiling(void) const
    {
        return this->pipeUVTiling->getVector2();
    }

    Ogre::Vector3 ProceduralPipeComponent::snapToGridFunc(const Ogre::Vector3& position)
    {
        const Ogre::Real grid = std::max(0.01f, this->gridSize->getReal());

        Ogre::Vector3 snapped = position;
        snapped.x = std::round(position.x / grid) * grid;
        snapped.y = std::round(position.y / grid) * grid;
        // z is deliberately untouched: the depth lane is chosen by the GameObject's plane and
        // by the U nudge, not by the grid.

        return snapped;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Segment mode
    ///////////////////////////////////////////////////////////////////////////////////////////////

    void ProceduralPipeComponent::setEditMode(const Ogre::String& editModeStr)
    {
        this->editMode->setListSelectedValue(editModeStr);
        this->selectedSegmentIndex = -1;

        if (this->getEditModeEnum() == EditMode::SEGMENT && this->buildState == BuildState::DRAGGING)
        {
            this->cancelPipe();
        }

        if (this->getEditModeEnum() == EditMode::SEGMENT)
        {
            // One-shot resync against the GameObject's actual transform, once per Segment-mode
            // entry: the user may have moved or rotated the object with the normal gizmo after
            // building the pipe, and every pick and every overlay line below assumes the stored
            // frame still matches.
            if (true == this->hasPipeOrigin && nullptr != this->gameObjectPtr)
            {
                Ogre::SceneNode* node = this->gameObjectPtr->getSceneNode();
                if (nullptr != node)
                {
                    const Ogre::Vector3 liveWorldPos = node->_getDerivedPositionUpdated();
                    const Ogre::Quaternion liveOrientation = node->_getDerivedOrientationUpdated();

                    const Ogre::Vector3 expectedWorldPos = this->pipeFrame * this->pipeOrigin;
                    const bool positionChanged = false == MathHelper::getInstance()->vector3Equals(liveWorldPos, expectedWorldPos, 0.01f);
                    const bool orientationChanged = false == liveOrientation.equals(this->pipeFrame, Ogre::Radian(0.001f));

                    if (true == positionChanged || true == orientationChanged)
                    {
                        const Ogre::Quaternion newFrame = liveOrientation;
                        const Ogre::Vector3 newPipeOrigin = newFrame.Inverse() * liveWorldPos;
                        const Ogre::Vector3 delta = newPipeOrigin - this->pipeOrigin;

                        for (PipeSegment& seg : this->pipeSegments)
                        {
                            for (PipeControlPoint& cp : seg.controlPoints)
                            {
                                cp.position.x += delta.x;
                                cp.rawHeight += delta.y;
                                cp.smoothedHeight += delta.y;
                            }
                        }

                        this->pipeOrigin = newPipeOrigin;
                        this->pipeFrame = newFrame;
                        this->pipePlaneAnchor = liveWorldPos;

                        if (true == this->hasLoadedPipeEndpoint)
                        {
                            this->loadedPipeEndpoint.x += delta.x;
                            this->loadedPipeEndpointHeight += delta.y;
                        }

                        this->rebuildMesh();
                    }
                }
            }
        }

        this->claimEditFocus();
    }

    ProceduralPipeComponent::EditMode ProceduralPipeComponent::getEditModeEnum(void) const
    {
        return (this->editMode->getListSelectedValue() == "Segment") ? EditMode::SEGMENT : EditMode::OBJECT;
    }

    void ProceduralPipeComponent::deleteSelectedSegment(void)
    {
        if (this->selectedSegmentIndex < 0 || this->selectedSegmentIndex >= static_cast<int>(this->pipeSegments.size()))
        {
            return;
        }

        std::vector<unsigned char> oldData = this->getPlatformData();

        this->pipeSegments.erase(this->pipeSegments.begin() + this->selectedSegmentIndex);
        this->selectedSegmentIndex = -1;

        if (this->pipeSegments.empty())
        {
            this->destroyPipeMesh();
            this->pathSamples.clear();
            this->hasPipeOrigin = false;
            this->hasLoadedPipeEndpoint = false;
        }
        else
        {
            this->rebuildMesh();
            this->updateContinuationPoint();
        }

        this->scheduleSegmentOverlayUpdate();

        std::vector<unsigned char> newData = this->getPlatformData();

        boost::shared_ptr<EventDataCommandTransactionBegin> evtBegin(new EventDataCommandTransactionBegin("Delete Pipe Segment"));
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evtBegin);

        boost::shared_ptr<EventDataPlatformModifyEnd> evtMod(new EventDataPlatformModifyEnd(oldData, newData, this->gameObjectPtr->getId()));
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evtMod);

        boost::shared_ptr<EventDataCommandTransactionEnd> evtEnd(new EventDataCommandTransactionEnd());
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evtEnd);
    }

    int ProceduralPipeComponent::findNearestSegmentOnScreen(Ogre::Real screenX, Ogre::Real screenY, Ogre::Real radius)
    {
        if (this->pipeSegments.empty())
        {
            return -1;
        }

        int bestSeg = -1;
        Ogre::Real bestDist = radius;

        // renderZ is an ABSOLUTE local Z (it carries the working plane's own constant), while
        // raycastFixedPlane's localZOffset is a DELTA on top of that plane - so the base
        // plane's own z has to be subtracted or every pick plane lands twice as far out.
        const Ogre::Real baseLocalZ = (this->pipeFrame.Inverse() * this->pipePlaneAnchor).z;

        for (size_t si = 0; si < this->pipeSegments.size(); ++si)
        {
            const PipeSegment& seg = this->pipeSegments[si];
            if (seg.controlPoints.size() < 2)
            {
                continue;
            }

            // One pick plane per segment, at the depth that segment is actually drawn at.
            // Sharing one plane across a ramped chain is pure parallax and makes the far end
            // of a nudged run unselectable.
            Ogre::Real segZ = 0.0f;
            for (const PipeControlPoint& cp : seg.controlPoints)
            {
                segZ += cp.renderZ;
            }
            segZ /= static_cast<Ogre::Real>(seg.controlPoints.size());

            Ogre::Vector3 hitPos = Ogre::Vector3::ZERO;
            if (false == this->raycastFixedPlane(screenX, screenY, hitPos, segZ - baseLocalZ))
            {
                continue;
            }
            hitPos = this->pipeFrame.Inverse() * hitPos;

            for (size_t pi = 1; pi < seg.controlPoints.size(); ++pi)
            {
                const Ogre::Vector2 a(seg.controlPoints[pi - 1].position.x, seg.controlPoints[pi - 1].smoothedHeight);
                const Ogre::Vector2 b(seg.controlPoints[pi].position.x, seg.controlPoints[pi].smoothedHeight);
                const Ogre::Vector2 p(hitPos.x, hitPos.y);

                const Ogre::Vector2 ab = b - a;
                const Ogre::Real abLen2 = ab.dotProduct(ab);
                Ogre::Real t = 0.0f;
                if (abLen2 > 1e-6f)
                {
                    t = Ogre::Math::Clamp((p - a).dotProduct(ab) / abLen2, 0.0f, 1.0f);
                }
                const Ogre::Real dist = (a + ab * t - p).length();

                if (dist < bestDist)
                {
                    bestDist = dist;
                    bestSeg = static_cast<int>(si);
                }
            }
        }

        return bestSeg;
    }

    void ProceduralPipeComponent::createSegmentOverlay(void)
    {
        NOWA::GraphicsModule::RenderCommand cmd = [this]()
        {
            this->segOverlayNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();

            this->segOverlayObject = this->gameObjectPtr->getSceneManager()->createManualObject();
            this->segOverlayObject->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);
            this->segOverlayObject->setName("PipeSegOverlay_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()));
            this->segOverlayObject->setQueryFlags(0u);
            this->segOverlayObject->setCastShadows(false);
            this->segOverlayNode->attachObject(this->segOverlayObject);
            this->segOverlayNode->setVisible(false);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "ProceduralPipeComponent::createSegmentOverlay");
    }

    void ProceduralPipeComponent::destroySegmentOverlay(void)
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
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "ProceduralPipeComponent::destroySegmentOverlay");
    }

    void ProceduralPipeComponent::scheduleSegmentOverlayUpdate(void)
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

        if (false == segmentMode || true == this->pipeSegments.empty())
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
            NOWA::GraphicsModule::getInstance()->enqueue(std::move(hideCmd), "ProceduralPipeComponent::segOverlay_hide");
            return;
        }

        // Line geometry is built here on the main thread and handed over as plain data - the
        // render command below only pushes it into the ManualObject.
        struct LV
        {
            Ogre::Vector3 pos;
            Ogre::ColourValue col;
        };
        std::vector<LV> lines;
        lines.reserve(this->pipeSegments.size() * 16);

        // Pushed toward the camera by the pipe's own radius, so the centerline is drawn in
        // front of the tube instead of buried inside it - the one place where the overlay for
        // a round cross-section has to differ from the platform's flat one.
        const Ogre::Real pushZ = this->pipeRadius->getReal() + 0.05f;
        const Ogre::Real crossR = std::max(0.15f, this->pipeRadius->getReal() * 0.5f);

        const Ogre::ColourValue cGrey(0.35f, 0.35f, 0.35f, 1.0f);
        const Ogre::ColourValue cSelected(1.00f, 0.75f, 0.00f, 1.0f);
        const Ogre::ColourValue cEndpt(1.00f, 0.75f, 0.00f, 1.0f);

        auto addLine = [&](const Ogre::Vector3& a, const Ogre::Vector3& b, const Ogre::ColourValue& c)
        {
            lines.push_back({a, c});
            lines.push_back({b, c});
        };

        for (int si = 0; si < static_cast<int>(this->pipeSegments.size()); ++si)
        {
            const PipeSegment& seg = this->pipeSegments[si];
            if (seg.controlPoints.size() < 2)
            {
                continue;
            }

            const PipeControlPoint& cp0 = seg.controlPoints.front();
            const PipeControlPoint& cp1 = seg.controlPoints.back();
            const bool selected = (si == this->selectedSegmentIndex);
            const Ogre::ColourValue& lineCol = selected ? cSelected : cGrey;

            const float segLen = std::abs(cp1.position.x - cp0.position.x);
            const int N = std::max(4, static_cast<int>(segLen / 2.0f) + 1);

            std::vector<Ogre::Vector3> path;
            path.reserve(N + 1);

            for (int k = 0; k <= N; ++k)
            {
                const float t = static_cast<float>(k) / static_cast<float>(N);
                // renderZ, not position.z: the overlay has to sit where the MESH is, and a
                // ramped chain has carried the tube away from its authored depth everywhere
                // except at the two ends.
                const float z = cp0.renderZ + (cp1.renderZ - cp0.renderZ) * t;
                path.push_back(Ogre::Vector3(cp0.position.x + (cp1.position.x - cp0.position.x) * t, cp0.smoothedHeight + (cp1.smoothedHeight - cp0.smoothedHeight) * t, z + pushZ));
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
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPipeComponent] Overlay begin() FAILED: " + e.getDescription());
            }

            if (this->segOverlayNode)
            {
                this->segOverlayNode->setOrientation(this->pipeFrame);
                this->segOverlayNode->setVisible(true);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(drawCmd), "ProceduralPipeComponent::segOverlay_draw");
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Snapping
    ///////////////////////////////////////////////////////////////////////////////////////////////

    bool ProceduralPipeComponent::detectSnapToOwnPipe(const Ogre::Vector3& worldPos, Ogre::Real radius)
    {
        if (this->pipeSegments.empty())
        {
            return false;
        }

        float bestDist = radius;
        int bestSeg = -1;
        Ogre::Vector3 bestPt = Ogre::Vector3::ZERO;

        for (int si = 0; si < static_cast<int>(this->pipeSegments.size()); ++si)
        {
            const PipeSegment& seg = this->pipeSegments[si];
            if (seg.controlPoints.size() < 2)
            {
                continue;
            }

            // Only the two ENDPOINTS snap, not the whole polyline: connecting to a pipe's mouth
            // is the meaningful operation, dropping a tube into the middle of another one is
            // not (that is what a junction would be, and there is none here).
            for (int end = 0; end < 2; ++end)
            {
                const PipeControlPoint& cp = (0 == end) ? seg.controlPoints.front() : seg.controlPoints.back();

                const Ogre::Vector2 ep(cp.position.x, cp.smoothedHeight);
                const Ogre::Vector2 p(worldPos.x, worldPos.y);

                const float dist = ep.distance(p);
                if (dist < bestDist)
                {
                    bestDist = dist;
                    bestSeg = si;
                    // Keep the stored point's real z - the snap indicator and the confirmed
                    // endpoint both need the true depth, not a hardcoded 0.
                    bestPt = Ogre::Vector3(cp.position.x, cp.smoothedHeight, cp.position.z);
                }
            }
        }

        this->isSnapToOwnPipe = (bestSeg >= 0);
        this->snapToPipeSegmentIdx = bestSeg;
        this->snapToPipePoint = bestPt;

        return this->isSnapToOwnPipe;
    }

    void ProceduralPipeComponent::scheduleSnapIndicatorUpdate(void)
    {
        if (nullptr == this->segOverlayObject)
        {
            return;
        }

        if (false == this->isSnapToOwnPipe)
        {
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
                NOWA::GraphicsModule::getInstance()->enqueue(std::move(cmd), "ProceduralPipeComponent::snapIndicator_hide");
            }
            return;
        }

        const Ogre::Vector3 centre = this->snapToPipePoint;
        const Ogre::Real r = std::max(1.0f, this->pipeRadius->getReal());
        const int segs = 16;
        const Ogre::Real pushZ = this->pipeRadius->getReal() + 0.1f;

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
                this->segOverlayNode->setOrientation(this->pipeFrame);
                this->segOverlayNode->setVisible(true);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(cmd), "ProceduralPipeComponent::snapIndicator_draw");
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Path persistence
    //
    // The path lives inside the scene XML as a base64 "Path Data" property - there is no side
    // car file, for the same reasons ProceduralPlatformComponent gave up on its own: a level
    // with thirty pipes would mean thirty binary files to keep in sync with the scene by hand.
    //
    // The byte layout is deliberately IDENTICAL to the platform's, so a path can be moved
    // between the two components by copying one XML attribute.
    ///////////////////////////////////////////////////////////////////////////////////////////////

    namespace
    {
        const char* const pipeBase64Alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

        Ogre::String encodePipeBase64(const std::vector<unsigned char>& data)
        {
            Ogre::String result;
            result.reserve(((data.size() + 2) / 3) * 4);

            size_t i = 0;
            while (i + 2 < data.size())
            {
                const uint32_t triple = (static_cast<uint32_t>(data[i]) << 16) | (static_cast<uint32_t>(data[i + 1]) << 8) | static_cast<uint32_t>(data[i + 2]);

                result += pipeBase64Alphabet[(triple >> 18) & 0x3F];
                result += pipeBase64Alphabet[(triple >> 12) & 0x3F];
                result += pipeBase64Alphabet[(triple >> 6) & 0x3F];
                result += pipeBase64Alphabet[triple & 0x3F];

                i += 3;
            }

            const size_t remaining = data.size() - i;

            if (1 == remaining)
            {
                const uint32_t triple = static_cast<uint32_t>(data[i]) << 16;

                result += pipeBase64Alphabet[(triple >> 18) & 0x3F];
                result += pipeBase64Alphabet[(triple >> 12) & 0x3F];
                result += "==";
            }
            else if (2 == remaining)
            {
                const uint32_t triple = (static_cast<uint32_t>(data[i]) << 16) | (static_cast<uint32_t>(data[i + 1]) << 8);

                result += pipeBase64Alphabet[(triple >> 18) & 0x3F];
                result += pipeBase64Alphabet[(triple >> 12) & 0x3F];
                result += pipeBase64Alphabet[(triple >> 6) & 0x3F];
                result += '=';
            }

            return result;
        }

        bool decodePipeBase64(const Ogre::String& encoded, std::vector<unsigned char>& outData)
        {
            outData.clear();
            outData.reserve((encoded.size() / 4) * 3);

            uint32_t accumulator = 0;
            int bitsCollected = 0;

            for (size_t i = 0; i < encoded.size(); ++i)
            {
                const char c = encoded[i];

                // Whitespace is tolerated - an XML editor may have wrapped the attribute.
                if (' ' == c || '\t' == c || '\r' == c || '\n' == c)
                {
                    continue;
                }
                if ('=' == c)
                {
                    break;
                }

                int value = -1;

                if (c >= 'A' && c <= 'Z')
                {
                    value = c - 'A';
                }
                else if (c >= 'a' && c <= 'z')
                {
                    value = c - 'a' + 26;
                }
                else if (c >= '0' && c <= '9')
                {
                    value = c - '0' + 52;
                }
                else if ('+' == c)
                {
                    value = 62;
                }
                else if ('/' == c)
                {
                    value = 63;
                }
                else
                {
                    return false;
                }

                accumulator = (accumulator << 6) | static_cast<uint32_t>(value);
                bitsCollected += 6;

                if (bitsCollected >= 8)
                {
                    bitsCollected -= 8;
                    outData.push_back(static_cast<unsigned char>((accumulator >> bitsCollected) & 0xFF));
                }
            }

            return true;
        }
    } // namespace

    Ogre::String ProceduralPipeComponent::serializePathData(void) const
    {
        if (true == this->pipeSegments.empty())
        {
            return "";
        }

        // Little endian throughout:
        //   uint32 version | uint32 numSegments | float origin.x/.y/.z
        //   per segment: uint8 isCurved | float curvature | uint32 numControlPoints
        //     per control point: float position.x/.y/.z, rawHeight, smoothedHeight
        //
        // renderZ and distFromStart are NOT written - both are recomputed by every rebuild,
        // and storing a derived value is only a way for the file to start disagreeing with the
        // geometry it describes.
        size_t totalSize = 4 + 4 + 12;

        for (const auto& seg : this->pipeSegments)
        {
            totalSize += 1 + 4 + 4;
            totalSize += seg.controlPoints.size() * 20;
        }

        std::vector<unsigned char> buffer(totalSize);
        size_t off = 0;

        uint32_t version = PATHDATA_VERSION;
        uint32_t numSegments = static_cast<uint32_t>(this->pipeSegments.size());

        memcpy(&buffer[off], &version, 4);
        off += 4;
        memcpy(&buffer[off], &numSegments, 4);
        off += 4;

        const Ogre::Vector3 origin = (true == this->hasPipeOrigin) ? this->pipeOrigin : this->cachedPipeOrigin;

        memcpy(&buffer[off], &origin.x, 4);
        off += 4;
        memcpy(&buffer[off], &origin.y, 4);
        off += 4;
        memcpy(&buffer[off], &origin.z, 4);
        off += 4;

        for (const auto& seg : this->pipeSegments)
        {
            const uint8_t curved = seg.isCurved ? 1 : 0;
            buffer[off++] = curved;

            memcpy(&buffer[off], &seg.curvature, 4);
            off += 4;

            const uint32_t numCPs = static_cast<uint32_t>(seg.controlPoints.size());
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

        return encodePipeBase64(buffer);
    }

    bool ProceduralPipeComponent::deserializePathData(const Ogre::String& encodedData)
    {
        std::vector<unsigned char> buffer;

        if (false == decodePipeBase64(encodedData, buffer))
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPipeComponent] Path Data is not valid base64");
            return false;
        }

        if (buffer.size() < 8)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPipeComponent] Path Data too small (" + Ogre::StringConverter::toString(buffer.size()) + " bytes)");
            return false;
        }

        size_t off = 0;
        bool ok = true;

        // Every read goes through this, so a truncated or hand-edited property cannot walk off
        // the end of the buffer.
        auto readBytes = [&buffer, &off, &ok](void* destination, size_t byteCount)
        {
            if (false == ok || off + byteCount > buffer.size())
            {
                ok = false;
                return;
            }

            memcpy(destination, &buffer[off], byteCount);
            off += byteCount;
        };

        uint32_t version = 0;
        uint32_t numSegments = 0;

        readBytes(&version, 4);
        readBytes(&numSegments, 4);

        if (false == ok || (version != PATHDATA_VERSION && version != 1u))
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPipeComponent] Unsupported Path Data version " + Ogre::StringConverter::toString(version));
            return false;
        }

        const bool hasStoredOrigin = (version >= 2);

        Ogre::Vector3 origin = Ogre::Vector3::ZERO;
        if (true == hasStoredOrigin)
        {
            readBytes(&origin.x, 4);
            readBytes(&origin.y, 4);
            readBytes(&origin.z, 4);
        }

        // Parsed into a local list and only committed once the whole blob turned out readable:
        // a half-restored path would look like a shortened pipe, and the next save would write
        // that truncation back over the intact data.
        std::vector<PipeSegment> parsedSegments;
        parsedSegments.reserve(numSegments);

        for (uint32_t i = 0; i < numSegments; ++i)
        {
            PipeSegment seg;

            uint8_t curved = 0;
            readBytes(&curved, 1);
            seg.isCurved = (0 != curved);

            readBytes(&seg.curvature, 4);

            uint32_t numCPs = 0;
            readBytes(&numCPs, 4);

            if (false == ok || off + static_cast<size_t>(numCPs) * 20 > buffer.size())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPipeComponent] Path Data truncated at segment " + Ogre::StringConverter::toString(i));
                return false;
            }

            seg.controlPoints.reserve(numCPs);

            for (uint32_t j = 0; j < numCPs; ++j)
            {
                PipeControlPoint cp;

                readBytes(&cp.position.x, 4);
                readBytes(&cp.position.y, 4);
                readBytes(&cp.position.z, 4);
                readBytes(&cp.rawHeight, 4);
                readBytes(&cp.smoothedHeight, 4);

                cp.renderZ = cp.position.z;
                cp.distFromStart = 0.0f;

                seg.controlPoints.push_back(cp);
            }

            parsedSegments.push_back(seg);
        }

        if (false == ok)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPipeComponent] Unexpected end of Path Data");
            return false;
        }

        this->pipeSegments = parsedSegments;

        if (false == hasStoredOrigin && false == this->pipeSegments.empty() && false == this->pipeSegments.front().controlPoints.empty())
        {
            const PipeControlPoint& firstControlPoint = this->pipeSegments.front().controlPoints.front();
            origin = Ogre::Vector3(firstControlPoint.position.x, firstControlPoint.rawHeight, firstControlPoint.position.z);
        }

        this->pipeOrigin = origin;
        this->cachedPipeOrigin = origin;
        this->hasPipeOrigin = (false == this->pipeSegments.empty());

        // A path that has segments was meshed at least once, so the GameObject's node
        // transform is already in the scene XML and IS the authoritative one - the mesh must
        // not snap the node back onto the stored origin.
        this->originPositionSet = (false == this->pipeSegments.empty());

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPipeComponent] Restored path with " + Ogre::StringConverter::toString(this->pipeSegments.size()) + " segments from scene data");

        return true;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Undo / redo blob
    //
    // A different format from the scene property above, and deliberately so: this one also
    // carries the finished vertex and index buffers. It never leaves the process, an undo step
    // lands in a live component whose attributes cannot have changed underneath it, so the
    // cached geometry is guaranteed to match and restoring it beats re-sweeping.
    ///////////////////////////////////////////////////////////////////////////////////////////////

    std::vector<unsigned char> ProceduralPipeComponent::getPlatformData(void) const
    {
        std::vector<unsigned char> result;

        if (this->pipeSegments.empty() && 0 == this->cachedNumFarVertices && 0 == this->cachedNumNearVertices)
        {
            return result;
        }

        // magic(4) version(4) origin(12) numSegments(4) farVerts(4) farInds(4) nearVerts(4)
        // nearInds(4) posSet(1) = 41 bytes
        const size_t headerSize = 41;

        size_t totalSize = headerSize;

        for (const auto& seg : this->pipeSegments)
        {
            totalSize += 1 + 4 + 4;
            totalSize += seg.controlPoints.size() * 20;
        }

        totalSize += this->cachedFarVertices.size() * sizeof(float);
        totalSize += this->cachedFarIndices.size() * sizeof(Ogre::uint32);
        totalSize += this->cachedNearVertices.size() * sizeof(float);
        totalSize += this->cachedNearIndices.size() * sizeof(Ogre::uint32);

        result.resize(totalSize);
        size_t off = 0;

        uint32_t magic = PIPEDATA_MAGIC;
        uint32_t version = PIPEDATA_VERSION;
        uint32_t numSegments = static_cast<uint32_t>(this->pipeSegments.size());
        uint32_t numFarVerts = static_cast<uint32_t>(this->cachedNumFarVertices);
        uint32_t numFarInds = static_cast<uint32_t>(this->cachedFarIndices.size());
        uint32_t numNearVerts = static_cast<uint32_t>(this->cachedNumNearVertices);
        uint32_t numNearInds = static_cast<uint32_t>(this->cachedNearIndices.size());
        uint8_t posSet = this->originPositionSet ? 1 : 0;

        memcpy(&result[off], &magic, 4);
        off += 4;
        memcpy(&result[off], &version, 4);
        off += 4;

        const Ogre::Vector3 origin = this->cachedPipeOrigin;
        memcpy(&result[off], &origin.x, 4);
        off += 4;
        memcpy(&result[off], &origin.y, 4);
        off += 4;
        memcpy(&result[off], &origin.z, 4);
        off += 4;

        memcpy(&result[off], &numSegments, 4);
        off += 4;
        memcpy(&result[off], &numFarVerts, 4);
        off += 4;
        memcpy(&result[off], &numFarInds, 4);
        off += 4;
        memcpy(&result[off], &numNearVerts, 4);
        off += 4;
        memcpy(&result[off], &numNearInds, 4);
        off += 4;

        result[off++] = posSet;

        for (const auto& seg : this->pipeSegments)
        {
            result[off++] = seg.isCurved ? 1 : 0;

            memcpy(&result[off], &seg.curvature, 4);
            off += 4;

            const uint32_t numCPs = static_cast<uint32_t>(seg.controlPoints.size());
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

        if (false == this->cachedFarVertices.empty())
        {
            memcpy(&result[off], this->cachedFarVertices.data(), this->cachedFarVertices.size() * sizeof(float));
            off += this->cachedFarVertices.size() * sizeof(float);
        }
        if (false == this->cachedFarIndices.empty())
        {
            memcpy(&result[off], this->cachedFarIndices.data(), this->cachedFarIndices.size() * sizeof(Ogre::uint32));
            off += this->cachedFarIndices.size() * sizeof(Ogre::uint32);
        }
        if (false == this->cachedNearVertices.empty())
        {
            memcpy(&result[off], this->cachedNearVertices.data(), this->cachedNearVertices.size() * sizeof(float));
            off += this->cachedNearVertices.size() * sizeof(float);
        }
        if (false == this->cachedNearIndices.empty())
        {
            memcpy(&result[off], this->cachedNearIndices.data(), this->cachedNearIndices.size() * sizeof(Ogre::uint32));
            off += this->cachedNearIndices.size() * sizeof(Ogre::uint32);
        }

        return result;
    }

    void ProceduralPipeComponent::setPlatformData(const std::vector<unsigned char>& data)
    {
        // An empty blob is a legitimate undo target: it is what "the pipe did not exist yet"
        // looks like.
        if (true == data.empty())
        {
            this->pipeSegments.clear();
            this->pathSamples.clear();
            this->destroyPipeMesh();
            this->hasPipeOrigin = false;
            this->hasLoadedPipeEndpoint = false;
            this->selectedSegmentIndex = -1;
            this->scheduleSegmentOverlayUpdate();
            return;
        }

        if (data.size() < 41)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPipeComponent] Undo blob too small");
            return;
        }

        size_t off = 0;
        bool ok = true;

        auto readBytes = [&data, &off, &ok](void* destination, size_t byteCount)
        {
            if (false == ok || off + byteCount > data.size())
            {
                ok = false;
                return;
            }
            memcpy(destination, &data[off], byteCount);
            off += byteCount;
        };

        uint32_t magic = 0;
        uint32_t version = 0;
        readBytes(&magic, 4);
        readBytes(&version, 4);

        if (false == ok || magic != PIPEDATA_MAGIC || version != PIPEDATA_VERSION)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPipeComponent] Undo blob has wrong magic or version");
            return;
        }

        Ogre::Vector3 origin = Ogre::Vector3::ZERO;
        readBytes(&origin.x, 4);
        readBytes(&origin.y, 4);
        readBytes(&origin.z, 4);

        uint32_t numSegments = 0;
        uint32_t numFarVerts = 0;
        uint32_t numFarInds = 0;
        uint32_t numNearVerts = 0;
        uint32_t numNearInds = 0;
        uint8_t posSet = 0;

        readBytes(&numSegments, 4);
        readBytes(&numFarVerts, 4);
        readBytes(&numFarInds, 4);
        readBytes(&numNearVerts, 4);
        readBytes(&numNearInds, 4);
        readBytes(&posSet, 1);

        if (false == ok)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPipeComponent] Undo blob header truncated");
            return;
        }

        std::vector<PipeSegment> parsedSegments;
        parsedSegments.reserve(numSegments);

        for (uint32_t i = 0; i < numSegments; ++i)
        {
            PipeSegment seg;

            uint8_t curved = 0;
            readBytes(&curved, 1);
            seg.isCurved = (0 != curved);

            readBytes(&seg.curvature, 4);

            uint32_t numCPs = 0;
            readBytes(&numCPs, 4);

            if (false == ok || off + static_cast<size_t>(numCPs) * 20 > data.size())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPipeComponent] Undo blob truncated at segment " + Ogre::StringConverter::toString(i));
                return;
            }

            seg.controlPoints.reserve(numCPs);

            for (uint32_t j = 0; j < numCPs; ++j)
            {
                PipeControlPoint cp;

                readBytes(&cp.position.x, 4);
                readBytes(&cp.position.y, 4);
                readBytes(&cp.position.z, 4);
                readBytes(&cp.rawHeight, 4);
                readBytes(&cp.smoothedHeight, 4);

                cp.renderZ = cp.position.z;
                cp.distFromStart = 0.0f;

                seg.controlPoints.push_back(cp);
            }

            parsedSegments.push_back(seg);
        }

        std::vector<float> farVerts(numFarVerts * 8);
        std::vector<Ogre::uint32> farInds(numFarInds);
        std::vector<float> nearVerts(numNearVerts * 8);
        std::vector<Ogre::uint32> nearInds(numNearInds);

        if (false == farVerts.empty())
        {
            readBytes(farVerts.data(), farVerts.size() * sizeof(float));
        }
        if (false == farInds.empty())
        {
            readBytes(farInds.data(), farInds.size() * sizeof(Ogre::uint32));
        }
        if (false == nearVerts.empty())
        {
            readBytes(nearVerts.data(), nearVerts.size() * sizeof(float));
        }
        if (false == nearInds.empty())
        {
            readBytes(nearInds.data(), nearInds.size() * sizeof(Ogre::uint32));
        }

        if (false == ok)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralPipeComponent] Undo blob truncated in geometry block");
            return;
        }

        this->pipeSegments = parsedSegments;
        this->pipeOrigin = origin;
        this->cachedPipeOrigin = origin;
        this->hasPipeOrigin = (false == this->pipeSegments.empty());
        this->originPositionSet = (0 != posSet);
        this->selectedSegmentIndex = -1;

        this->cachedFarVertices = farVerts;
        this->cachedFarIndices = farInds;
        this->cachedNumFarVertices = numFarVerts;
        this->cachedNearVertices = nearVerts;
        this->cachedNearIndices = nearInds;
        this->cachedNumNearVertices = numNearVerts;

        this->destroyPipeMesh();

        if (0 == numFarVerts && 0 == numNearVerts)
        {
            this->pathSamples.clear();
            this->updateContinuationPoint();
            this->scheduleSegmentOverlayUpdate();
            return;
        }

        GraphicsModule::RenderCommand renderCommand = [this, farVerts, farInds, numFarVerts, nearVerts, nearInds, numNearVerts, origin]()
        {
            this->createPipeMeshInternal(farVerts, farInds, numFarVerts, nearVerts, nearInds, numNearVerts, origin);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralPipeComponent::setPlatformData");

        // The geometry came back from the cache, but pathSamples did not - they are not part
        // of the blob, and the Lua path API would silently keep answering with the pre-undo
        // path. Cheapest honest fix: one sweep with the mesh thrown away afterwards would be
        // wasteful, so the samples are rebuilt from the restored path directly by a full
        // rebuild ONLY when something actually queries them. Until then they are cleared, and
        // getPipeLength reports 0 rather than a stale number.
        this->pathSamples.clear();

        this->updateContinuationPoint();
        this->scheduleSegmentOverlayUpdate();
    }

    bool ProceduralPipeComponent::getNearestPointOnPlatform(const Ogre::Vector3& worldPos, Ogre::Real maxRadius, Ogre::Vector3& outPoint) const
    {
        if (true == this->pipeSegments.empty())
        {
            return false;
        }

        Ogre::Real bestDistSquared = maxRadius * maxRadius;
        bool found = false;

        const Ogre::Vector3 localQuery = this->pipeFrame.Inverse() * worldPos;

        for (const PipeSegment& seg : this->pipeSegments)
        {
            for (const PipeControlPoint& cp : seg.controlPoints)
            {
                const Ogre::Vector3 candidate(cp.position.x, cp.smoothedHeight, cp.position.z);
                const Ogre::Real distSquared = candidate.squaredDistance(localQuery);

                if (distSquared < bestDistSquared)
                {
                    bestDistSquared = distSquared;
                    outPoint = this->pipeFrame * candidate;
                    found = true;
                }
            }
        }

        return found;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Path query API
    //
    // pathSamples are mesh-local; everything here returns WORLD space, read off the live scene
    // node rather than off pipeFrame/pipeOrigin, so a pipe that was moved with the gizmo after
    // it was built still answers correctly.
    ///////////////////////////////////////////////////////////////////////////////////////////////

    Ogre::Real ProceduralPipeComponent::getPipeLength(void) const
    {
        if (this->pathSamples.empty())
        {
            return 0.0f;
        }
        return this->pathSamples.back().distance;
    }

    Ogre::Vector3 ProceduralPipeComponent::getPointAtDistance(Ogre::Real distance) const
    {
        if (this->pathSamples.empty())
        {
            return Ogre::Vector3::ZERO;
        }

        Ogre::Vector3 localPoint = this->pathSamples.back().position;

        if (distance <= this->pathSamples.front().distance)
        {
            localPoint = this->pathSamples.front().position;
        }
        else
        {
            for (size_t i = 1; i < this->pathSamples.size(); ++i)
            {
                if (this->pathSamples[i].distance >= distance)
                {
                    const PipePathSample& a = this->pathSamples[i - 1];
                    const PipePathSample& b = this->pathSamples[i];

                    const Ogre::Real span = b.distance - a.distance;
                    const Ogre::Real t = (span > 1e-6f) ? ((distance - a.distance) / span) : 0.0f;

                    localPoint = a.position + (b.position - a.position) * t;
                    break;
                }
            }
        }

        Ogre::SceneNode* node = (nullptr != this->gameObjectPtr) ? this->gameObjectPtr->getSceneNode() : nullptr;
        if (nullptr != node)
        {
            return node->_getDerivedOrientationUpdated() * localPoint + node->_getDerivedPositionUpdated();
        }

        return this->pipeFrame * (localPoint + this->pipeOrigin);
    }

    Ogre::Vector3 ProceduralPipeComponent::getDirectionAtDistance(Ogre::Real distance) const
    {
        if (this->pathSamples.empty())
        {
            return Ogre::Vector3::UNIT_X;
        }

        Ogre::Vector3 localDirection = this->pathSamples.back().direction;

        if (distance <= this->pathSamples.front().distance)
        {
            localDirection = this->pathSamples.front().direction;
        }
        else
        {
            for (size_t i = 1; i < this->pathSamples.size(); ++i)
            {
                if (this->pathSamples[i].distance >= distance)
                {
                    const PipePathSample& a = this->pathSamples[i - 1];
                    const PipePathSample& b = this->pathSamples[i];

                    const Ogre::Real span = b.distance - a.distance;
                    const Ogre::Real t = (span > 1e-6f) ? ((distance - a.distance) / span) : 0.0f;

                    localDirection = (a.direction + (b.direction - a.direction) * t).normalisedCopy();
                    break;
                }
            }
        }

        Ogre::SceneNode* node = (nullptr != this->gameObjectPtr) ? this->gameObjectPtr->getSceneNode() : nullptr;
        if (nullptr != node)
        {
            return node->_getDerivedOrientationUpdated() * localDirection;
        }

        return this->pipeFrame * localDirection;
    }

    Ogre::Vector3 ProceduralPipeComponent::getPointAt(Ogre::Real t) const
    {
        return this->getPointAtDistance(Ogre::Math::Clamp(t, 0.0f, 1.0f) * this->getPipeLength());
    }

    Ogre::Vector3 ProceduralPipeComponent::getDirectionAt(Ogre::Real t) const
    {
        return this->getDirectionAtDistance(Ogre::Math::Clamp(t, 0.0f, 1.0f) * this->getPipeLength());
    }

    Ogre::Real ProceduralPipeComponent::getDistanceOnPipe(const Ogre::Vector3& worldPosition) const
    {
        if (this->pathSamples.size() < 2)
        {
            return 0.0f;
        }

        Ogre::Vector3 localQuery = worldPosition;

        Ogre::SceneNode* node = (nullptr != this->gameObjectPtr) ? this->gameObjectPtr->getSceneNode() : nullptr;
        if (nullptr != node)
        {
            localQuery = node->_getDerivedOrientationUpdated().Inverse() * (worldPosition - node->_getDerivedPositionUpdated());
        }
        else
        {
            localQuery = (this->pipeFrame.Inverse() * worldPosition) - this->pipeOrigin;
        }

        Ogre::Real bestDistanceSquared = std::numeric_limits<Ogre::Real>::max();
        Ogre::Real bestAlong = 0.0f;

        // Projected onto every span rather than snapped to the nearest sample: a pipe with
        // metre-spaced rings would otherwise quantise the player's position to metres, which is
        // very visible if a script drives a camera off it.
        for (size_t i = 1; i < this->pathSamples.size(); ++i)
        {
            const PipePathSample& a = this->pathSamples[i - 1];
            const PipePathSample& b = this->pathSamples[i];

            const Ogre::Vector3 ab = b.position - a.position;
            const Ogre::Real abLen2 = ab.squaredLength();

            Ogre::Real t = 0.0f;
            if (abLen2 > 1e-8f)
            {
                t = Ogre::Math::Clamp((localQuery - a.position).dotProduct(ab) / abLen2, 0.0f, 1.0f);
            }

            const Ogre::Vector3 projected = a.position + ab * t;
            const Ogre::Real distanceSquared = projected.squaredDistance(localQuery);

            if (distanceSquared < bestDistanceSquared)
            {
                bestDistanceSquared = distanceSquared;
                bestAlong = a.distance + (b.distance - a.distance) * t;
            }
        }

        return bestAlong;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Editing focus and events
    ///////////////////////////////////////////////////////////////////////////////////////////////

    void ProceduralPipeComponent::claimEditFocus(void)
    {
        // Announced through the editor-mode event every editing component already listens to,
        // rather than a second event type: with several procedural components on one object,
        // two parallel listeners would fire in an order nobody controls and leave two pieces of
        // state that can disagree about who is editing.
        unsigned short manipulationMode = NOWA::EditorManager::EDITOR_SELECT_MODE;
        if (this->getEditModeEnum() == EditMode::SEGMENT)
        {
            manipulationMode = NOWA::EditorManager::EDITOR_MESH_MODIFY_MODE;
        }

        boost::shared_ptr<EventDataEditorMode> eventDataEditorMode(new EventDataEditorMode(manipulationMode, this->gameObjectPtr->getId(), ProceduralPipeComponent::getStaticClassName()));
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataEditorMode);
    }

    bool ProceduralPipeComponent::isEditFocusOwner(void) const
    {
        // Empty means nobody claimed editing on this GameObject, which is the normal case for
        // an object carrying a single editing component - defaulting to "yes" there is what
        // keeps a lone pipe working. The rule only bites once someone actually claims.
        if (true == this->editFocusOwner.empty())
        {
            return true;
        }
        return this->editFocusOwner == ProceduralPipeComponent::getStaticClassName();
    }

    void ProceduralPipeComponent::handleMeshModifyMode(NOWA::EventDataPtr eventData)
    {
        auto castEventData = boost::static_pointer_cast<EventDataEditorMode>(eventData);

        this->isEditorMeshModifyMode = (castEventData->getManipulationMode() == EditorManager::EDITOR_MESH_MODIFY_MODE);

        if (true == castEventData->hasEditFocusClaim() && castEventData->getGameObjectId() == this->gameObjectPtr->getId())
        {
            this->editFocusOwner = castEventData->getComponentClassName();
        }

        this->updateModificationState();
    }

    void ProceduralPipeComponent::handleGameObjectSelected(NOWA::EventDataPtr eventData)
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
            if (this->getEditModeEnum() == EditMode::SEGMENT)
            {
                boost::shared_ptr<EventDataEditorMode> eventDataEditorMode(new EventDataEditorMode(NOWA::EditorManager::EDITOR_MESH_MODIFY_MODE));
                NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataEditorMode);
            }
        }

        this->updateModificationState();
    }

    void ProceduralPipeComponent::handleComponentManuallyDeleted(NOWA::EventDataPtr eventData)
    {
        boost::shared_ptr<EventDataDeleteComponent> castEventData = boost::static_pointer_cast<EventDataDeleteComponent>(eventData);
        if (this->gameObjectPtr->getId() == castEventData->getGameObjectId())
        {
            if (this->getClassName() == castEventData->getComponentName())
            {
                // Nothing to clean up on disk: deleting the component removes its whole
                // property block from the scene XML, and the path goes with it.
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralPipeComponent] Component manually deleted for game object: " + this->gameObjectPtr->getName());
            }
        }
    }

    void ProceduralPipeComponent::addInputListener(void)
    {
        const Ogre::String listenerName = ProceduralPipeComponent::getStaticClassName() + "_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
        if (auto* core = InputDeviceCore::getSingletonPtr())
        {
            core->addKeyListener(this, listenerName);
            core->addMouseListener(this, listenerName);
        }
    }

    void ProceduralPipeComponent::removeInputListener(void)
    {
        const Ogre::String listenerName = ProceduralPipeComponent::getStaticClassName() + "_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
        if (auto* core = InputDeviceCore::getSingletonPtr())
        {
            core->removeKeyListener(listenerName);
            core->removeMouseListener(listenerName);
        }
    }

    void ProceduralPipeComponent::updateModificationState(void)
    {
        // isEditFocusOwner is the fourth condition and it is what lets a SIBLING component on
        // the same GameObject take over editing. Not owning it drops the input listener
        // entirely, so the click reaches whoever does - and routing through here rather than
        // just removing the listener also cancels an in-progress drag and hides the overlay,
        // so nothing is left on screen over geometry another component now owns.
        const bool shouldBeActive = this->activated->getBool() && this->isEditorMeshModifyMode && this->isSelected && this->isEditFocusOwner();

        if (shouldBeActive)
        {
            this->addInputListener();

            if (false == this->pipeSegments.empty())
            {
                this->updateContinuationPoint();
            }

            this->scheduleSegmentOverlayUpdate();
        }
        else
        {
            this->removeInputListener();

            if (this->buildState == BuildState::DRAGGING)
            {
                this->cancelPipe();
            }

            this->selectedSegmentIndex = -1;
            this->scheduleSegmentOverlayUpdate();
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    //  Lua API
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ProceduralPipeComponent* getProceduralPipeComponent(GameObject* go)
    {
        return NOWA::makeStrongPtr(go->getComponent<ProceduralPipeComponent>()).get();
    }

    ProceduralPipeComponent* getProceduralPipeComponentFromName(GameObject* go, const Ogre::String& name)
    {
        return NOWA::makeStrongPtr(go->getComponentFromName<ProceduralPipeComponent>(name)).get();
    }

    void ProceduralPipeComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
    {
        luabind::module(lua)[luabind::class_<ProceduralPipeComponent, GameObjectComponent>("ProceduralPipeComponent")

                .def("setActivated", &ProceduralPipeComponent::setActivated)
                .def("isActivated", &ProceduralPipeComponent::isActivated)

                // ── Geometry ──────────────────────────────────────────────────
                .def("setPipeRadius", &ProceduralPipeComponent::setPipeRadius)
                .def("getPipeRadius", &ProceduralPipeComponent::getPipeRadius)
                .def("setWallThickness", &ProceduralPipeComponent::setWallThickness)
                .def("getWallThickness", &ProceduralPipeComponent::getWallThickness)
                .def("setRadialSegments", &ProceduralPipeComponent::setRadialSegments)
                .def("getRadialSegments", &ProceduralPipeComponent::getRadialSegments)
                .def("setCurveSubdivisions", &ProceduralPipeComponent::setCurveSubdivisions)
                .def("getCurveSubdivisions", &ProceduralPipeComponent::getCurveSubdivisions)
                .def("setSmoothingFactor", &ProceduralPipeComponent::setSmoothingFactor)
                .def("getSmoothingFactor", &ProceduralPipeComponent::getSmoothingFactor)

                // ── Near side ─────────────────────────────────────────────────
                .def("setNearSideMode", &ProceduralPipeComponent::setNearSideMode)
                .def("getNearSideMode", &ProceduralPipeComponent::getNearSideMode)
                .def("setNearSideArc", &ProceduralPipeComponent::setNearSideArc)
                .def("getNearSideArc", &ProceduralPipeComponent::getNearSideArc)
                .def("setJunctionHubScale", &ProceduralPipeComponent::setJunctionHubScale)
                .def("getJunctionHubScale", &ProceduralPipeComponent::getJunctionHubScale)
                .def("setNearSideAlpha", &ProceduralPipeComponent::setNearSideAlpha)
                .def("getNearSideAlpha", &ProceduralPipeComponent::getNearSideAlpha)
                .def("setInvertNearSide", &ProceduralPipeComponent::setInvertNearSide)
                .def("getInvertNearSide", &ProceduralPipeComponent::getInvertNearSide)

                // ── Datablocks / UV ───────────────────────────────────────────
                .def("setPipeDatablock", &ProceduralPipeComponent::setPipeDatablock)
                .def("getPipeDatablock", &ProceduralPipeComponent::getPipeDatablock)
                .def("setNearDatablock", &ProceduralPipeComponent::setNearDatablock)
                .def("getNearDatablock", &ProceduralPipeComponent::getNearDatablock)
                .def("setPipeUVTiling", &ProceduralPipeComponent::setPipeUVTiling)
                .def("getPipeUVTiling", &ProceduralPipeComponent::getPipeUVTiling)

                // ── Grid / snap ───────────────────────────────────────────────
                .def("setSnapToGrid", &ProceduralPipeComponent::setSnapToGrid)
                .def("getSnapToGrid", &ProceduralPipeComponent::getSnapToGrid)
                .def("setGridSize", &ProceduralPipeComponent::setGridSize)
                .def("getGridSize", &ProceduralPipeComponent::getGridSize)

                // ── Segments ──────────────────────────────────────────────────
                .def("getSegmentCount", &ProceduralPipeComponent::getSegmentCount)
                .def("addPipeSegment", &ProceduralPipeComponent::addPipeSegment)

                // ── Path queries ──────────────────────────────────────────────
                .def("getPipeLength", &ProceduralPipeComponent::getPipeLength)
                .def("getPointAtDistance", &ProceduralPipeComponent::getPointAtDistance)
                .def("getDirectionAtDistance", &ProceduralPipeComponent::getDirectionAtDistance)
                .def("getPointAt", &ProceduralPipeComponent::getPointAt)
                .def("getDirectionAt", &ProceduralPipeComponent::getDirectionAt)
                .def("getDistanceOnPipe", &ProceduralPipeComponent::getDistanceOnPipe)];

        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPipeComponent", "class inherits GameObjectComponent", ProceduralPipeComponent::getStaticInfoText());
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPipeComponent", "void setActivated(bool activated)", "Activates or deactivates the pipe component.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPipeComponent", "void setPipeRadius(float radius)", "Sets the outer radius of the tube in meters and rebuilds.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPipeComponent", "void setWallThickness(float thickness)", "Sets the wall thickness. Greater than 0 adds an inner shell and closing end rings.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPipeComponent", "void setRadialSegments(int segments)", "Sets how many vertices each ring has (3-128).");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPipeComponent", "void setNearSideMode(string mode)",
            "Sets how the arc facing the camera is treated: 'Solid', 'Transparent' or 'Hidden'. Solid<->Transparent is a datablock swap and needs no rebuild; anything involving Hidden rebuilds the mesh.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPipeComponent", "string getNearSideMode()", "Returns the current near side mode.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPipeComponent", "void setNearSideArc(float degrees)", "Sets how wide the near arc is, in degrees around the camera direction. 0 disables the split.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPipeComponent", "void setJunctionHubScale(float scale)",
            "Sets the radius of the sphere filling a junction, as a multiple of Pipe Radius (1.05-3.0). Also decides how far each arm is trimmed back into it, so this rebuilds the mesh.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPipeComponent", "float getJunctionHubScale()", "Returns the junction hub scale.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPipeComponent", "void setNearSideAlpha(float alpha)",
            "Sets the alpha of the near arc in Transparent mode (0-1). The transparency is applied to a CLONE of the datablock, so the pipe body and every other object sharing that material stay opaque. 1.0 leaves the datablock untouched.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPipeComponent", "float getNearSideAlpha()", "Returns the near arc alpha.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPipeComponent", "void setInvertNearSide(bool invert)",
            "Flips which side counts as the near one. Off means local -Z (the camera side for an unrotated object); turn it on for a pipe whose GameObject is rotated 180 degrees.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPipeComponent", "float getPipeLength()", "Returns the total swept centerline length in meters.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPipeComponent", "Vector3 getPointAtDistance(float distance)", "Returns the world position d meters along the pipe.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPipeComponent", "Vector3 getDirectionAtDistance(float distance)", "Returns the world tangent d meters along the pipe.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPipeComponent", "Vector3 getPointAt(float t)", "Returns the world position at t in 0..1 along the pipe.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPipeComponent", "Vector3 getDirectionAt(float t)", "Returns the world tangent at t in 0..1 along the pipe.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPipeComponent", "float getDistanceOnPipe(Vector3 worldPosition)",
            "Projects a world position onto the centerline and returns how far along the pipe it lies - feed it the player's position to track where he is inside the tube.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPipeComponent", "int getSegmentCount()", "Returns the number of pipe segments currently placed.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralPipeComponent", "void addPipeSegment(Vector3 start, Vector3 end)", "Adds a single pipe segment between two local positions and rebuilds.");

        gameObjectClass.def("getProceduralPipeComponent", (ProceduralPipeComponent * (*)(GameObject*)) & getProceduralPipeComponent);
        gameObjectClass.def("getProceduralPipeComponentFromName", &getProceduralPipeComponentFromName);
        gameObjectControllerClass.def("castProceduralPipeComponent", &GameObjectController::cast<ProceduralPipeComponent>);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ProceduralPipeComponent getProceduralPipeComponent()", "Gets the ProceduralPipeComponent from this GameObject.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ProceduralPipeComponent getProceduralPipeComponentFromName(string name)", "Gets a named ProceduralPipeComponent from this GameObject.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "ProceduralPipeComponent castProceduralPipeComponent(ProceduralPipeComponent other)", "Casts for Lua auto-completion support.");
    }

} // namespace NOWA
/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#include "NOWAPrecompiled.h"
#include "ProceduralStairsComponent.h"
#include "editor/EditorManager.h"
#include "gameObject/GameObjectController.h"
#include "gameobject/PhysicsArtifactComponent.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "main/InputDeviceCore.h"
#include "utilities/MathHelper.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"

#include "OgreAbiUtils.h"
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

#include <cmath>
#include <limits>

namespace NOWA
{
    using namespace rapidxml;

    // =========================================================================
    //  Constructor
    // =========================================================================

    ProceduralStairsComponent::ProceduralStairsComponent() :
        GeometricComponentBase(),
        name("ProceduralStairsComponent"),
        activated(nullptr),
        stairShape(nullptr),
        stepCount(nullptr),
        stepHeight(nullptr),
        stepDepth(nullptr),
        stepWidth(nullptr),
        stepNosing(nullptr),
        openRiser(nullptr),
        stringerStyle(nullptr),
        bottomStyle(nullptr),
        innerRadius(nullptr),
        outerRadius(nullptr),
        arcAngle(nullptr),
        rotationDir(nullptr),
        centrePole(nullptr),
        pivotPosition(nullptr),
        uvMode(nullptr),
        uvTiling(nullptr),
        treadDatablock(nullptr),
        riserDatablock(nullptr),
        stringerDatablock(nullptr),
        convertToMesh(nullptr),
        treadVertexBase(0u),
        riserVertexBase(0u),
        stringerVertexBase(0u),
        stairsMesh(nullptr),
        stairsItem(nullptr),
        previewMesh(nullptr),
        previewItem(nullptr),
        previewNode(nullptr),
        physicsArtifactComponent(nullptr)
    {
        this->activated = new Variant(ProceduralStairsComponent::AttrActivated(), true, this->attributes);

        // ── Shape ─────────────────────────────────────────────────────────────
        std::vector<Ogre::String> shapes;
        shapes.push_back("Linear");
        shapes.push_back("Curved");
        this->stairShape = new Variant(ProceduralStairsComponent::AttrStairShape(), shapes, this->attributes);

        // ── Step geometry ─────────────────────────────────────────────────────
        this->stepCount = new Variant(ProceduralStairsComponent::AttrStepCount(), 12, this->attributes);
        this->stepHeight = new Variant(ProceduralStairsComponent::AttrStepHeight(), 0.2f, this->attributes);
        this->stepDepth = new Variant(ProceduralStairsComponent::AttrStepDepth(), 0.3f, this->attributes);
        this->stepWidth = new Variant(ProceduralStairsComponent::AttrStepWidth(), 1.2f, this->attributes);
        this->stepNosing = new Variant(ProceduralStairsComponent::AttrStepNosing(), 0.0f, this->attributes);
        this->openRiser = new Variant(ProceduralStairsComponent::AttrOpenRiser(), false, this->attributes);

        // ── Structural ────────────────────────────────────────────────────────
        std::vector<Ogre::String> stringers;
        stringers.push_back("None");
        stringers.push_back("Closed");
        stringers.push_back("Open");
        this->stringerStyle = new Variant(ProceduralStairsComponent::AttrStringerStyle(), stringers, this->attributes);
        this->stringerStyle->setListSelectedValue("Open");

        std::vector<Ogre::String> bottoms;
        bottoms.push_back("None");
        bottoms.push_back("Sloped");
        bottoms.push_back("Stepped");
        this->bottomStyle = new Variant(ProceduralStairsComponent::AttrBottomStyle(), bottoms, this->attributes);

        // ── Curved ───────────────────────────────────────────────────
        this->innerRadius = new Variant(ProceduralStairsComponent::AttrInnerRadius(), 0.3f, this->attributes);
        this->outerRadius = new Variant(ProceduralStairsComponent::AttrOuterRadius(), 1.5f, this->attributes);
        this->arcAngle = new Variant(ProceduralStairsComponent::AttrArcAngle(), 180.0f, this->attributes);

        std::vector<Ogre::String> dirs;
        dirs.push_back("Counter-Clockwise");
        dirs.push_back("Clockwise");
        this->rotationDir = new Variant(ProceduralStairsComponent::AttrRotationDir(), dirs, this->attributes);

        this->centrePole = new Variant(ProceduralStairsComponent::AttrCentrePole(), false, this->attributes);

        // ── Pivot ─────────────────────────────────────────────────────────────
        std::vector<Ogre::String> pivots;
        pivots.push_back("Bottom-Front");
        pivots.push_back("Bottom-Centre");
        pivots.push_back("Bottom-Back");
        this->pivotPosition = new Variant(ProceduralStairsComponent::AttrPivotPosition(), pivots, this->attributes);

        this->rampCollider = new Variant(ProceduralStairsComponent::AttrRampCollider(), false, this->attributes);

        // ── UV ───────────────────────────────────────────────────────────────
        std::vector<Ogre::String> uvModes;
        uvModes.push_back("Box");
        uvModes.push_back("Continuous");
        this->uvMode = new Variant(ProceduralStairsComponent::AttrUVMode(), uvModes, this->attributes);
        this->uvTiling = new Variant(ProceduralStairsComponent::AttrUVTiling(), Ogre::Vector2(1.0f, 1.0f), this->attributes);

        // ── Materials ─────────────────────────────────────────────────────────
        this->treadDatablock = new Variant(ProceduralStairsComponent::AttrTreadDatablock(), Ogre::String("proceduralStairs1"), this->attributes);
        this->riserDatablock = new Variant(ProceduralStairsComponent::AttrRiserDatablock(), Ogre::String("proceduralStairs1"), this->attributes);
        this->stringerDatablock = new Variant(ProceduralStairsComponent::AttrStringerDatablock(), Ogre::String("proceduralStairs1"), this->attributes);

        // ── Convert to mesh action ────────────────────────────────────────────
        this->convertToMesh = new Variant(ProceduralStairsComponent::AttrConvertToMesh(), Ogre::String("Convert to Mesh"), this->attributes);

        // ── Descriptions ──────────────────────────────────────────────────────
        this->activated->setDescription("Activate / deactivate the stair placement mode.");
        this->stairShape->setDescription("Linear: straight flight. "
                                         "Curved: arc flight using Inner/Outer Radius and Arc Angle. "
                                         "Spiral: like Curved but allows Arc Angle > 360 degrees.");
        this->stepCount->setDescription("Number of steps in the flight (min 1).");
        this->stepWidth->setDescription("Lateral width of the stair flight in world units. "
                                        "Linear only — for Curved use Inner/Outer Radius.");
        this->stepDepth->setDescription("Horizontal run per step in world units. "
                                        "Linear only — for Curved the depth is set by arc geometry.");
        this->stepNosing->setDescription("Tread overhang beyond the riser below. "
                                         "Linear: world units offset. "
                                         "Curved: converted to angular extension at the mid radius.");
        this->stringerStyle->setDescription("Side panel style: None / Closed / Open. "
                                            "Linear only — curved stairs have no straight side panels.");
        this->bottomStyle->setDescription("Underside geometry: None / Sloped / Stepped. "
                                          "Linear only — curved underside is handled per-step.");
        this->pivotPosition->setDescription("Scene node origin placement. "
                                            "Linear: Bottom-Front/Centre/Back shifts along the run direction. "
                                            "Curved: Bottom-Front = entry angle faces +X (default). "
                                            "        Bottom-Back  = exit angle faces +X (stair reversed).");
        this->rampCollider->setDescription("When enabled, generates an invisible diagonal ramp face over the stair flight. "
                                           "This lets physics-driven characters walk up stairs smoothly without "
                                           "stairstep jitter — the collider ramp is fully transparent (opacity 0). "
                                           "Works for both Linear and Curved stairs. "
                                           "The ramp uses an auto-created PBS datablock named 'StairsRamp_<id>'. "
                                           "Combine with a PhysicsArtifactComponent for best results.");
        this->innerRadius->setDescription("Inner void radius in world units (Curved / Spiral only).");
        this->outerRadius->setDescription("Outer step edge radius in world units (Curved / Spiral only).");
        this->arcAngle->setDescription("Total rotation of the stair flight in degrees. "
                                       "180 = half-turn, 360 = full spiral, any value supported.");
        this->rotationDir->setDescription("Direction the stair turns when ascending (Curved / Spiral only).");
        this->centrePole->setDescription("Generate a solid cylinder at the inner radius as a structural pole "
                                         "(Curved / Spiral only).");
        
        this->uvMode->setDescription("Box        – each face projected from its dominant axis (best for untiled). "
                                     "Continuous – UV flows along the stair flight direction (best for tiling).");
        this->uvTiling->setDescription("UV tiling multiplier applied to all faces (x = U, y = V).");
        this->treadDatablock->setDescription("PBS datablock applied to the top (horizontal tread) faces.");
        this->riserDatablock->setDescription("PBS datablock applied to the front (vertical riser) faces. "
                                             "Ignored when Open Riser = true.");
        this->stringerDatablock->setDescription("PBS datablock applied to the stringer side panels. "
                                                "Ignored when Stringer Style = None.");
        this->convertToMesh->setDescription("Bakes the current stair geometry to a static .mesh file and replaces "
                                            "this component with a plain MeshComponent. "
                                            "THIS IS A ONE-WAY, IRREVERSIBLE OPERATION.");

        this->convertToMesh->addUserData(GameObject::AttrActionExec());
        this->convertToMesh->addUserData(GameObject::AttrActionNeedRefresh());
        this->convertToMesh->addUserData(GameObject::AttrActionExecId(), "ProceduralStairsComponent.ConvertToMesh");

        this->stairShape->addUserData(GameObject::AttrActionNeedRefresh());
    }

    ProceduralStairsComponent::~ProceduralStairsComponent()
    {
    }

    // =========================================================================
    //  Ogre::Plugin
    // =========================================================================

    void ProceduralStairsComponent::install(const Ogre::NameValuePairList* options)
    {
        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<ProceduralStairsComponent>(ProceduralStairsComponent::getStaticClassId(), ProceduralStairsComponent::getStaticClassName());
    }

    void ProceduralStairsComponent::initialise()
    {
    }
    void ProceduralStairsComponent::shutdown()
    {
    }
    void ProceduralStairsComponent::uninstall()
    {
    }

    const Ogre::String& ProceduralStairsComponent::getName() const
    {
        return this->name;
    }

    void ProceduralStairsComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    // =========================================================================
    //  init
    // =========================================================================

    bool ProceduralStairsComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralStairsComponent::AttrActivated())
        {
            this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralStairsComponent::AttrStairShape())
        {
            this->stairShape->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "Linear"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralStairsComponent::AttrStepCount())
        {
            this->stepCount->setValue(XMLConverter::getAttribInt(propertyElement, "data", 12));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralStairsComponent::AttrStepHeight())
        {
            this->stepHeight->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.2f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralStairsComponent::AttrStepDepth())
        {
            this->stepDepth->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.3f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralStairsComponent::AttrStepWidth())
        {
            this->stepWidth->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.2f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralStairsComponent::AttrStepNosing())
        {
            this->stepNosing->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.02f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralStairsComponent::AttrOpenRiser())
        {
            this->openRiser->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralStairsComponent::AttrStringerStyle())
        {
            this->stringerStyle->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "None"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralStairsComponent::AttrBottomStyle())
        {
            this->bottomStyle->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "None"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralStairsComponent::AttrInnerRadius())
        {
            this->innerRadius->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.3f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralStairsComponent::AttrOuterRadius())
        {
            this->outerRadius->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.5f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralStairsComponent::AttrArcAngle())
        {
            this->arcAngle->setValue(XMLConverter::getAttribReal(propertyElement, "data", 180.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralStairsComponent::AttrRotationDir())
        {
            this->rotationDir->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "Counter-Clockwise"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralStairsComponent::AttrCentrePole())
        {
            this->centrePole->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralStairsComponent::AttrPivotPosition())
        {
            this->pivotPosition->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "Bottom-Front"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralStairsComponent::AttrRampCollider())
        {
            this->rampCollider->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralStairsComponent::AttrUVMode())
        {
            this->uvMode->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "Box"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralStairsComponent::AttrUVTiling())
        {
            this->uvTiling->setValue(XMLConverter::getAttribVector2(propertyElement, "data", Ogre::Vector2(1.0f, 1.0f)));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralStairsComponent::AttrTreadDatablock())
        {
            this->treadDatablock->setValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralStairsComponent::AttrRiserDatablock())
        {
            this->riserDatablock->setValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralStairsComponent::AttrStringerDatablock())
        {
            this->stringerDatablock->setValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        return true;
    }

    // =========================================================================
    //  postInit
    // =========================================================================

    bool ProceduralStairsComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralStairsComponent] Init component for game object: " + this->gameObjectPtr->getName());

        // Create preview scene node on render thread
        NOWA::GraphicsModule::RenderCommand makeNode = [this]()
        {
            this->previewNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();
            this->previewNode->setVisible(false);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(makeNode), "ProceduralStairsComponent::postInit_previewNode");

        this->physicsArtifactComponent = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsArtifactComponent>()).get();

        this->rebuildMesh();

        return true;
    }

    bool ProceduralStairsComponent::connect(void)
    {
        return true;
    }

    bool ProceduralStairsComponent::disconnect(void)
    {
        this->destroyPreviewMesh();
        return true;
    }

    bool ProceduralStairsComponent::onCloned(void)
    {
        return true;
    }

    void ProceduralStairsComponent::onAddComponent(void)
    {
        
    }

    void ProceduralStairsComponent::onRemoveComponent(void)
    {
        this->destroyPreviewMesh();
        this->destroyStairsMesh();

        NOWA::GraphicsModule::RenderCommand destroyNode = [this]()
        {
            if (nullptr != this->previewNode)
            {
                NOWA::GraphicsModule::getInstance()->removeTrackedNode(this->previewNode);
                this->gameObjectPtr->getSceneManager()->destroySceneNode(this->previewNode);
                this->previewNode = nullptr;
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(destroyNode), "ProceduralStairsComponent::onRemoveComponent_node");

        this->physicsArtifactComponent = nullptr;
    }

    void ProceduralStairsComponent::onOtherComponentRemoved(unsigned int index)
    {
        if (nullptr != this->physicsArtifactComponent && index == this->physicsArtifactComponent->getIndex())
        {
            this->physicsArtifactComponent = nullptr;
        }
    }

    void ProceduralStairsComponent::onOtherComponentAdded(unsigned int index)
    {
    }

    void ProceduralStairsComponent::update(Ogre::Real dt, bool notSimulating)
    {
    }

    bool ProceduralStairsComponent::canStaticAddComponent(GameObject* gameObject)
    {
        return false;
    }

    // =========================================================================
    //  clone
    // =========================================================================

    GameObjectCompPtr ProceduralStairsComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        ProceduralStairsComponentPtr cloned(boost::make_shared<ProceduralStairsComponent>());

        cloned->setActivated(this->activated->getBool());
        cloned->setStairShape(this->stairShape->getListSelectedValue());
        cloned->setStepCount(this->stepCount->getInt());
        cloned->setStepHeight(this->stepHeight->getReal());
        cloned->setStepDepth(this->stepDepth->getReal());
        cloned->setStepWidth(this->stepWidth->getReal());
        cloned->setStepNosing(this->stepNosing->getReal());
        cloned->setOpenRiser(this->openRiser->getBool());
        cloned->setStringerStyle(this->stringerStyle->getListSelectedValue());
        cloned->setBottomStyle(this->bottomStyle->getListSelectedValue());
        cloned->setInnerRadius(this->innerRadius->getReal());
        cloned->setOuterRadius(this->outerRadius->getReal());
        cloned->setArcAngle(this->arcAngle->getReal());
        cloned->setRotationDir(this->rotationDir->getListSelectedValue());
        cloned->setCentrePole(this->centrePole->getBool());
        cloned->setPivotPosition(this->pivotPosition->getListSelectedValue());
        cloned->setRampCollider(this->rampCollider->getBool());
        cloned->setUVMode(this->uvMode->getListSelectedValue());
        cloned->setUVTiling(this->uvTiling->getVector2());
        cloned->setTreadDatablock(this->treadDatablock->getString());
        cloned->setRiserDatablock(this->riserDatablock->getString());
        cloned->setStringerDatablock(this->stringerDatablock->getString());

        clonedGameObjectPtr->addComponent(cloned);
        cloned->setOwner(clonedGameObjectPtr);

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(cloned));
        return cloned;
    }

    // =========================================================================
    //  actualizeValue
    // =========================================================================

    void ProceduralStairsComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (ProceduralStairsComponent::AttrActivated() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
        }
        else if (ProceduralStairsComponent::AttrStairShape() == attribute->getName())
        {
            this->setStairShape(attribute->getListSelectedValue());
        }
        else if (ProceduralStairsComponent::AttrStepCount() == attribute->getName())
        {
            this->setStepCount(attribute->getInt());
        }
        else if (ProceduralStairsComponent::AttrStepHeight() == attribute->getName())
        {
            this->setStepHeight(attribute->getReal());
        }
        else if (ProceduralStairsComponent::AttrStepDepth() == attribute->getName())
        {
            this->setStepDepth(attribute->getReal());
        }
        else if (ProceduralStairsComponent::AttrStepWidth() == attribute->getName())
        {
            this->setStepWidth(attribute->getReal());
        }
        else if (ProceduralStairsComponent::AttrStepNosing() == attribute->getName())
        {
            this->setStepNosing(attribute->getReal());
        }
        else if (ProceduralStairsComponent::AttrOpenRiser() == attribute->getName())
        {
            this->setOpenRiser(attribute->getBool());
        }
        else if (ProceduralStairsComponent::AttrStringerStyle() == attribute->getName())
        {
            this->setStringerStyle(attribute->getListSelectedValue());
        }
        else if (ProceduralStairsComponent::AttrBottomStyle() == attribute->getName())
        {
            this->setBottomStyle(attribute->getListSelectedValue());
        }
        else if (ProceduralStairsComponent::AttrInnerRadius() == attribute->getName())
        {
            this->setInnerRadius(attribute->getReal());
        }
        else if (ProceduralStairsComponent::AttrOuterRadius() == attribute->getName())
        {
            this->setOuterRadius(attribute->getReal());
        }
        else if (ProceduralStairsComponent::AttrArcAngle() == attribute->getName())
        {
            this->setArcAngle(attribute->getReal());
        }
        else if (ProceduralStairsComponent::AttrRotationDir() == attribute->getName())
        {
            this->setRotationDir(attribute->getListSelectedValue());
        }
        else if (ProceduralStairsComponent::AttrCentrePole() == attribute->getName())
        {
            this->setCentrePole(attribute->getBool());
        }
        else if (ProceduralStairsComponent::AttrPivotPosition() == attribute->getName())
        {
            this->setPivotPosition(attribute->getListSelectedValue());
        }
        else if (ProceduralStairsComponent::AttrRampCollider() == attribute->getName())
        {
            this->setRampCollider(attribute->getBool());
        }
        else if (ProceduralStairsComponent::AttrUVMode() == attribute->getName())
        {
            this->setUVMode(attribute->getListSelectedValue());
        }
        else if (ProceduralStairsComponent::AttrUVTiling() == attribute->getName())
        {
            this->setUVTiling(attribute->getVector2());
        }
        else if (ProceduralStairsComponent::AttrTreadDatablock() == attribute->getName())
        {
            this->setTreadDatablock(attribute->getString());
        }
        else if (ProceduralStairsComponent::AttrRiserDatablock() == attribute->getName())
        {
            this->setRiserDatablock(attribute->getString());
        }
        else if (ProceduralStairsComponent::AttrStringerDatablock() == attribute->getName())
        {
            this->setStringerDatablock(attribute->getString());
        }
    }

    // =========================================================================
    //  writeXML
    // =========================================================================

    void ProceduralStairsComponent::writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc)
    {
        GameObjectComponent::writeXML(propertiesXML, doc);

        // Helper lambdas matching the wall component style
        auto appendBool = [&](const Ogre::String& n, bool v)
        {
            xml_node<>* node = doc.allocate_node(node_element, "property");
            node->append_attribute(doc.allocate_attribute("type", "12"));
            node->append_attribute(doc.allocate_attribute("name", doc.allocate_string(n.c_str())));
            node->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, v)));
            propertiesXML->append_node(node);
        };
        auto appendInt = [&](const Ogre::String& n, int v)
        {
            xml_node<>* node = doc.allocate_node(node_element, "property");
            node->append_attribute(doc.allocate_attribute("type", "2"));
            node->append_attribute(doc.allocate_attribute("name", doc.allocate_string(n.c_str())));
            node->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, v)));
            propertiesXML->append_node(node);
        };
        auto appendReal = [&](const Ogre::String& n, Ogre::Real v)
        {
            xml_node<>* node = doc.allocate_node(node_element, "property");
            node->append_attribute(doc.allocate_attribute("type", "6"));
            node->append_attribute(doc.allocate_attribute("name", doc.allocate_string(n.c_str())));
            node->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, v)));
            propertiesXML->append_node(node);
        };
        auto appendStr = [&](const Ogre::String& n, const Ogre::String& v)
        {
            xml_node<>* node = doc.allocate_node(node_element, "property");
            node->append_attribute(doc.allocate_attribute("type", "7"));
            node->append_attribute(doc.allocate_attribute("name", doc.allocate_string(n.c_str())));
            node->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, v)));
            propertiesXML->append_node(node);
        };
        auto appendV2 = [&](const Ogre::String& n, const Ogre::Vector2& v)
        {
            xml_node<>* node = doc.allocate_node(node_element, "property");
            node->append_attribute(doc.allocate_attribute("type", "8"));
            node->append_attribute(doc.allocate_attribute("name", doc.allocate_string(n.c_str())));
            node->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, v)));
            propertiesXML->append_node(node);
        };

        appendBool(AttrActivated(), this->activated->getBool());
        appendStr(AttrStairShape(), this->stairShape->getListSelectedValue());
        appendInt(AttrStepCount(), this->stepCount->getInt());
        appendReal(AttrStepHeight(), this->stepHeight->getReal());
        appendReal(AttrStepDepth(), this->stepDepth->getReal());
        appendReal(AttrStepWidth(), this->stepWidth->getReal());
        appendReal(AttrStepNosing(), this->stepNosing->getReal());
        appendBool(AttrOpenRiser(), this->openRiser->getBool());
        appendStr(AttrStringerStyle(), this->stringerStyle->getListSelectedValue());
        appendStr(AttrBottomStyle(), this->bottomStyle->getListSelectedValue());
        appendReal(AttrInnerRadius(), this->innerRadius->getReal());
        appendReal(AttrOuterRadius(), this->outerRadius->getReal());
        appendReal(AttrArcAngle(), this->arcAngle->getReal());
        appendStr(AttrRotationDir(), this->rotationDir->getListSelectedValue());
        appendBool(AttrCentrePole(), this->centrePole->getBool());
        appendStr(AttrPivotPosition(), this->pivotPosition->getListSelectedValue());
        appendBool(AttrRampCollider(), this->rampCollider->getBool());
        appendStr(AttrUVMode(), this->uvMode->getListSelectedValue());
        appendV2(AttrUVTiling(), this->uvTiling->getVector2());
        appendStr(AttrTreadDatablock(), this->treadDatablock->getString());
        appendStr(AttrRiserDatablock(), this->riserDatablock->getString());
        appendStr(AttrStringerDatablock(), this->stringerDatablock->getString());
    }

    // =========================================================================
    //  executeAction
    // =========================================================================

    bool ProceduralStairsComponent::executeAction(const Ogre::String& actionId, NOWA::Variant* attribute)
    {
        if ("ProceduralStairsComponent.ConvertToMesh" == actionId)
        {
            return this->convertToMeshApply();
        }
        return true;
    }

    // =========================================================================
    //  Low-level vertex / index helpers
    // =========================================================================

    // ── Tread (submesh 0) ─────────────────────────────────────────────────────

    void ProceduralStairsComponent::tv(Ogre::Real px, Ogre::Real py, Ogre::Real pz, Ogre::Real nx, Ogre::Real ny, Ogre::Real nz, Ogre::Real u, Ogre::Real v)
    {
        const Ogre::Vector2 t = this->uvTiling->getVector2();
        this->treadVertices.push_back(px);
        this->treadVertices.push_back(py);
        this->treadVertices.push_back(pz);
        this->treadVertices.push_back(nx);
        this->treadVertices.push_back(ny);
        this->treadVertices.push_back(nz);
        this->treadVertices.push_back(u * t.x);
        this->treadVertices.push_back(v * t.y);
    }

    void ProceduralStairsComponent::tt(Ogre::uint32 i0, Ogre::uint32 i1, Ogre::uint32 i2)
    {
        this->treadIndices.push_back(this->treadVertexBase + i0);
        this->treadIndices.push_back(this->treadVertexBase + i1);
        this->treadIndices.push_back(this->treadVertexBase + i2);
    }

    void ProceduralStairsComponent::tq(Ogre::uint32 i0, Ogre::uint32 i1, Ogre::uint32 i2, Ogre::uint32 i3)
    {
        this->tt(i0, i1, i2);
        this->tt(i0, i2, i3);
    }

    // ── Riser (submesh 1) ─────────────────────────────────────────────────────

    void ProceduralStairsComponent::rv(Ogre::Real px, Ogre::Real py, Ogre::Real pz, Ogre::Real nx, Ogre::Real ny, Ogre::Real nz, Ogre::Real u, Ogre::Real v)
    {
        const Ogre::Vector2 t = this->uvTiling->getVector2();
        this->riserVertices.push_back(px);
        this->riserVertices.push_back(py);
        this->riserVertices.push_back(pz);
        this->riserVertices.push_back(nx);
        this->riserVertices.push_back(ny);
        this->riserVertices.push_back(nz);
        this->riserVertices.push_back(u * t.x);
        this->riserVertices.push_back(v * t.y);
    }

    void ProceduralStairsComponent::rt(Ogre::uint32 i0, Ogre::uint32 i1, Ogre::uint32 i2)
    {
        this->riserIndices.push_back(this->riserVertexBase + i0);
        this->riserIndices.push_back(this->riserVertexBase + i1);
        this->riserIndices.push_back(this->riserVertexBase + i2);
    }

    void ProceduralStairsComponent::rq(Ogre::uint32 i0, Ogre::uint32 i1, Ogre::uint32 i2, Ogre::uint32 i3)
    {
        this->rt(i0, i1, i2);
        this->rt(i0, i2, i3);
    }

    // ── Stringer (submesh 2) ──────────────────────────────────────────────────

    void ProceduralStairsComponent::sv(Ogre::Real px, Ogre::Real py, Ogre::Real pz, Ogre::Real nx, Ogre::Real ny, Ogre::Real nz, Ogre::Real u, Ogre::Real v)
    {
        const Ogre::Vector2 t = this->uvTiling->getVector2();
        this->stringerVertices.push_back(px);
        this->stringerVertices.push_back(py);
        this->stringerVertices.push_back(pz);
        this->stringerVertices.push_back(nx);
        this->stringerVertices.push_back(ny);
        this->stringerVertices.push_back(nz);
        this->stringerVertices.push_back(u * t.x);
        this->stringerVertices.push_back(v * t.y);
    }

    void ProceduralStairsComponent::st(Ogre::uint32 i0, Ogre::uint32 i1, Ogre::uint32 i2)
    {
        this->stringerIndices.push_back(this->stringerVertexBase + i0);
        this->stringerIndices.push_back(this->stringerVertexBase + i1);
        this->stringerIndices.push_back(this->stringerVertexBase + i2);
    }

    void ProceduralStairsComponent::sq(Ogre::uint32 i0, Ogre::uint32 i1, Ogre::uint32 i2, Ogre::uint32 i3)
    {
        this->st(i0, i1, i2);
        this->st(i0, i2, i3);
    }

    void ProceduralStairsComponent::pv(Ogre::Real px, Ogre::Real py, Ogre::Real pz, Ogre::Real nx, Ogre::Real ny, Ogre::Real nz, Ogre::Real u, Ogre::Real v)
    {
        this->rampVertices.push_back(px);
        this->rampVertices.push_back(py);
        this->rampVertices.push_back(pz);
        this->rampVertices.push_back(nx);
        this->rampVertices.push_back(ny);
        this->rampVertices.push_back(nz);
        this->rampVertices.push_back(u);
        this->rampVertices.push_back(v);
    }

    void ProceduralStairsComponent::pt(Ogre::uint32 i0, Ogre::uint32 i1, Ogre::uint32 i2)
    {
        this->rampIndices.push_back(this->rampVertexBase + i0);
        this->rampIndices.push_back(this->rampVertexBase + i1);
        this->rampIndices.push_back(this->rampVertexBase + i2);
    }

    void ProceduralStairsComponent::pq(Ogre::uint32 i0, Ogre::uint32 i1, Ogre::uint32 i2, Ogre::uint32 i3)
    {
        this->pt(i0, i1, i2);
        this->pt(i0, i2, i3);
    }

    // =========================================================================
    //  Enum helpers
    // =========================================================================

    ProceduralStairsComponent::StairShape ProceduralStairsComponent::getStairShapeEnum(void) const
    {
        const Ogre::String& s = this->stairShape->getListSelectedValue();
        if (s == "Curved")
        {
            return StairShape::CURVED;
        }
        return StairShape::LINEAR;
    }

    ProceduralStairsComponent::StringerStyle ProceduralStairsComponent::getStringerStyleEnum(void) const
    {
        const Ogre::String& s = this->stringerStyle->getListSelectedValue();
        if (s == "Closed")
        {
            return StringerStyle::CLOSED;
        }
        if (s == "Open")
        {
            return StringerStyle::OPEN;
        }
        return StringerStyle::NONE;
    }

    ProceduralStairsComponent::BottomStyle ProceduralStairsComponent::getBottomStyleEnum(void) const
    {
        const Ogre::String& s = this->bottomStyle->getListSelectedValue();
        if (s == "Sloped")
        {
            return BottomStyle::SLOPED;
        }
        if (s == "Stepped")
        {
            return BottomStyle::STEPPED;
        }
        return BottomStyle::NONE;
    }

    ProceduralStairsComponent::PivotPosition ProceduralStairsComponent::getPivotPositionEnum(void) const
    {
        const Ogre::String& s = this->pivotPosition->getListSelectedValue();
        if (s == "Bottom-Centre")
        {
            return PivotPosition::BOTTOM_CENTRE;
        }
        if (s == "Bottom-Back")
        {
            return PivotPosition::BOTTOM_BACK;
        }
        return PivotPosition::BOTTOM_FRONT;
    }

    ProceduralStairsComponent::UVMode ProceduralStairsComponent::getUVModeEnum(void) const
    {
        const Ogre::String& s = this->uvMode->getListSelectedValue();
        if (s == "Continuous")
        {
            return UVMode::CONTINUOUS;
        }
        return UVMode::BOX;
    }

    // =========================================================================
    //  Pivot offset
    // =========================================================================

    void ProceduralStairsComponent::computePivotOffset(Ogre::Real& outOffsetX, Ogre::Real& outOffsetZ) const
    {
        const int n = std::max(1, this->stepCount->getInt());
        const float depth = this->stepDepth->getReal();
        const float totalDepth = n * depth;

        outOffsetX = 0.0f;

        switch (this->getPivotPositionEnum())
        {
        case PivotPosition::BOTTOM_FRONT:
            outOffsetZ = 0.0f;
            break;
        case PivotPosition::BOTTOM_CENTRE:
            outOffsetZ = totalDepth * 0.5f;
            break;
        case PivotPosition::BOTTOM_BACK:
            outOffsetZ = totalDepth;
            break;
        }
    }

    // =========================================================================
    //  buildGeometry  –  dispatch to shape generator
    // =========================================================================

    void ProceduralStairsComponent::buildGeometry(void)
    {
        this->treadVertices.clear();
        this->treadIndices.clear();
        this->treadVertexBase = 0u;

        this->riserVertices.clear();
        this->riserIndices.clear();
        this->riserVertexBase = 0u;

        this->stringerVertices.clear();
        this->stringerIndices.clear();
        this->stringerVertexBase = 0u;

        // ── Ramp buffer ───────────────────────────────────────────────────────────
        this->rampVertices.clear();
        this->rampIndices.clear();
        this->rampVertexBase = 0u;

        switch (this->getStairShapeEnum())
        {
        case StairShape::LINEAR:
            this->generateLinearStairs();
            if (this->rampCollider->getBool())
            {
                this->generateLinearRamp(/* pivotOffsetX= */ 0.0f, /* pivotOffsetZ= */ 0.0f);
                // Recompute pivot — generateLinearStairs already called computePivotOffset
                // internally, but ramp needs it independently.
                Ogre::Real pox = 0.0f, poz = 0.0f;
                this->computePivotOffset(pox, poz);
                this->rampVertices.clear();
                this->rampIndices.clear();
                this->rampVertexBase = 0u;
                this->generateLinearRamp(pox, poz);
            }
            break;
        case StairShape::CURVED:
            this->generateCurvedStairs();
            if (this->rampCollider->getBool())
            {
                this->generateCurvedRamp();
            }
            break;
        }
    }

    void ProceduralStairsComponent::generateLinearRamp(Ogre::Real pivotOffsetX, Ogre::Real pivotOffsetZ)
    {
        const int n = std::max(1, this->stepCount->getInt());
        const float sh = this->stepHeight->getReal();
        const float sd = this->stepDepth->getReal();
        const float sw = this->stepWidth->getReal();

        const float xL = -sw * 0.5f - pivotOffsetX;
        const float xR = sw * 0.5f - pivotOffsetX;

        // ── Ramp bottom: nose of step 0 ───────────────────────────────────────────
        // Step 0 nose = front edge of tread at tread height = (z0, sh).
        // Starting at y=0 would bury the ramp inside the steps by exactly sh.
        const float rampZ0 = -pivotOffsetZ; // z of step 0 riser
        const float rampY0 = sh;            // y of step 0 nose

        // ── Ramp top: nose of last step ───────────────────────────────────────────
        // Step (n-1) nose = (z0 + (n-1)*sd, n*sh).
        // NOT z1 = z0+n*sd (back of last step) — that would overshoot and dip.
        const float rampZ1 = rampZ0 + (n - 1) * sd;      // z of step (n-1) nose
        const float rampY1 = static_cast<float>(n) * sh; // = totalH

        // ── Verify: ramp passes through all step noses ────────────────────────────
        // At step i nose: z=rampZ0+i*sd, y_ramp = sh + i*(rampY1-sh)/((n-1)*sd) * sd
        //   = sh + i*(n-1)*sh/(n-1) = sh + i*sh = (i+1)*sh = nose height ✓

        // ── Normal: perpendicular to slope and width ───────────────────────────────
        // Slope vector S = (0, rampY1-rampY0, rampZ1-rampZ0) = (0, (n-1)*sh, (n-1)*sd)
        //   simplified: (0, sh, sd).
        // Width vector W = (1, 0, 0).
        // N = S × W = (sh*0-sd*0, sd*1-0*0, 0*0-sh*1) ... let us do it properly:
        // N = W × S to get outward-upward normal:
        //   W × S = (1,0,0) × (0,sh,sd) = (0*sd-0*sh, 0*0-1*sd, 1*sh-0*0) = (0,-sd,sh)
        // Normalise (0,-sd,sh): points upward-forward (positive y, positive z component)
        // which is the outward normal of a ramp facing the approaching person ✓
        const float slopeLen = std::sqrt(sd * sd + sh * sh);
        const float nx = 0.0f;
        const float ny = (slopeLen > 1e-6f) ? sh / slopeLen : 1.0f;
        const float nz = (slopeLen > 1e-6f) ? -sd / slopeLen : 0.0f;
        // Wait: (0,-sd,sh) normalised: ny=-sd/len, nz=sh/len. Let me re-examine sign.
        // The ramp faces UPward and toward the approaching person (lower z).
        // Approaching person is at z < rampZ0. Outward = toward lower z = -Z direction.
        // So nz should be negative. N = (0, sh, -sd)/len:
        //   ny = sh/len > 0 (upward) ✓
        //   nz = -sd/len < 0 (toward -Z = toward approaching person) ✓
        // Recalculate: slope direction is +Z and +Y. Normal to slope pointing upward/outward:
        //   perpendicular to (0,sh,sd) in YZ plane = (sd,-sh) rotated 90° CCW = (sd, -sh)?
        //   No: rotate (sh,sd) by 90° CCW in YZ = (-sd, sh). So N.y=-sd, N.z=sh → wrong sign.
        //   Rotate 90° CW in YZ: (sd, -sh). So N.y=sd, N.z=-sh. Normalise:
        const float nxFinal = 0.0f;
        const float nyFinal = (slopeLen > 1e-6f) ? sd / slopeLen : 1.0f;
        const float nzFinal = (slopeLen > 1e-6f) ? -sh / slopeLen : 0.0f;
        // Verify: dot with slope (0,sh,sd) = sd*sh + (-sh)*sd = 0 ✓ (perpendicular)
        //         ny=sd/len > 0 (points up) ✓
        //         nz=-sh/len < 0 (points toward approaching person at -Z) ✓

        this->rampVertexBase = static_cast<Ogre::uint32>(this->rampVertices.size() / 8u);

        // pq(0,3,2,1): CCW from above-front:
        // (v3-v0)×(v2-v0) = (0, rampY1-rampY0, rampZ1-rampZ0) × (sw,0,0)
        //   = (0*(0) - 0*(0), 0*sw - (rampZ1-rampZ0)*0...
        // Use the component formula:
        // v3-v0 = (0, rampY1-rampY0, 0)  [same z, different y]  — wait no:
        // v0=(xL,rampY0,rampZ0), v1=(xR,rampY0,rampZ0), v2=(xR,rampY1,rampZ1), v3=(xL,rampY1,rampZ1)
        // pq(0,3,2,1) → st(0,3,2):
        //   v3-v0 = (0, rampY1-rampY0, rampZ1-rampZ0) = (0,(n-1)*sh,(n-1)*sd)
        //   v2-v0 = (sw,(n-1)*sh,(n-1)*sd)
        //   cross = ((n-1)*sh*(n-1)*sd-(n-1)*sd*(n-1)*sh,  (n-1)*sd*sw-0,  0-(n-1)*sh*sw)
        //         = (0, (n-1)*sd*sw, -(n-1)*sh*sw)
        //         ∝ (0, sd, -sh) → ny>0, nz<0 ✓ outward-upward ✓
        pv(xL, rampY0, rampZ0, nxFinal, nyFinal, nzFinal, 0.0f, 0.0f); // [0] bottom-front-left
        pv(xR, rampY0, rampZ0, nxFinal, nyFinal, nzFinal, 1.0f, 0.0f); // [1] bottom-front-right
        pv(xR, rampY1, rampZ1, nxFinal, nyFinal, nzFinal, 1.0f, 1.0f); // [2] top-back-right
        pv(xL, rampY1, rampZ1, nxFinal, nyFinal, nzFinal, 0.0f, 1.0f); // [3] top-back-left

        pq(0, 3, 2, 1); // N=(0,+sd,-sh)/len → upward-outward ✓

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralStairsComponent] Linear ramp: "
                                                                           "bottom=(" + Ogre::StringConverter::toString(rampZ0) + ", " + Ogre::StringConverter::toString(rampY0) +
                                                                               ") top=(" + Ogre::StringConverter::toString(rampZ1) + ", " + Ogre::StringConverter::toString(rampY1) +
                                                                               ") slope=" + Ogre::StringConverter::toString(sh / sd));
    }

    void ProceduralStairsComponent::generateCurvedRamp(void)
    {
        const int n = std::max(1, this->stepCount->getInt());
        const float sh = this->stepHeight->getReal();
        const float rInner = std::max(0.01f, this->innerRadius->getReal());
        const float rOuter = std::max(rInner + 0.01f, this->outerRadius->getReal());
        const float totalArcDeg = this->arcAngle->getReal();
        const float stepArcDeg = totalArcDeg / n;
        const bool cw = (this->rotationDir->getListSelectedValue() == "Clockwise");
        const float dirMul = cw ? -1.0f : 1.0f;

        float startAngleDeg = 0.0f;
        switch (this->getPivotPositionEnum())
        {
        case PivotPosition::BOTTOM_FRONT:
            startAngleDeg = 0.0f;
            break;
        case PivotPosition::BOTTOM_CENTRE:
            startAngleDeg = -totalArcDeg * 0.5f;
            break;
        case PivotPosition::BOTTOM_BACK:
            startAngleDeg = -totalArcDeg;
            break;
        }

        // ── Ramp geometry: n-1 sectors connecting consecutive step noses ──────────
        //
        // Nose of step i:
        //   angle = aStart_i   (the ENTRY radial edge of step i)
        //   y     = (i+1)*sh   (the TREAD TOP height of step i)
        //
        // Sector j connects nose j → nose j+1:
        //   angle range : aStart_j  →  aEnd_j  (= aStart_{j+1})
        //   y at entry  : (j+1)*sh             (nose of step j)
        //   y at exit   : (j+2)*sh             (nose of step j+1)
        //
        // This places the ramp surface exactly on the front-top corners of each step.
        // Previous version used y=i*sh (step BOTTOM) → ramp was buried inside steps.

        for (int j = 0; j < n - 1; ++j)
        {
            const float yBottom = static_cast<float>(j + 1) * sh; // nose of step j
            const float yTop = static_cast<float>(j + 2) * sh;    // nose of step j+1

            const float aS = Ogre::Math::DegreesToRadians(startAngleDeg + j * stepArcDeg) * dirMul;
            const float aE = Ogre::Math::DegreesToRadians(startAngleDeg + (j + 1) * stepArcDeg) * dirMul;

            const float csS = std::cos(aS), snS = std::sin(aS);
            const float csE = std::cos(aE), snE = std::sin(aE);

            // pq(0,3,2,1) CCW:
            //   v3-v0 = (0, yTop-yBottom, 0) = (0, sh, 0)
            //   v2-v0 ≈ (rOuter*(csE-csS), sh, rOuter*(snE-snS))
            //   cross Y = sh*rOuter*(csE-csS)... well, the face curves but N≈(0,+1,0) ✓
            this->rampVertexBase = static_cast<Ogre::uint32>(this->rampVertices.size() / 8u);

            pv(rInner * csS, yBottom, rInner * snS, 0, 1, 0, 0.0f, 0.0f); // [0] inner start (nose j)
            pv(rOuter * csS, yBottom, rOuter * snS, 0, 1, 0, 1.0f, 0.0f); // [1] outer start (nose j)
            pv(rOuter * csE, yTop, rOuter * snE, 0, 1, 0, 1.0f, 1.0f);    // [2] outer end   (nose j+1)
            pv(rInner * csE, yTop, rInner * snE, 0, 1, 0, 0.0f, 1.0f);    // [3] inner end   (nose j+1)

            if (!cw)
            {
                pq(0, 3, 2, 1);
            }
            else
            {
                pq(0, 1, 2, 3);
            }
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
            "[ProceduralStairsComponent] Curved ramp: " + Ogre::StringConverter::toString(n - 1) + " sectors  y: " + Ogre::StringConverter::toString(sh) + " → " + Ogre::StringConverter::toString(static_cast<float>(n) * sh));
    }

    // =========================================================================
    //  generateLinearStep  –  one step box at position stepIndex
    // =========================================================================
    void ProceduralStairsComponent::generateLinearStep(int stepIndex, Ogre::Real pivotOffsetX, Ogre::Real pivotOffsetZ)
    {
        const float sh = this->stepHeight->getReal();
        const float sd = this->stepDepth->getReal();
        const float sw = this->stepWidth->getReal();
        const float nosing = this->stepNosing->getReal();

        const float y0 = stepIndex * sh;
        const float y1 = y0 + sh;

        // z0 = riser position; riser faces −Z (toward approaching person).
        // z1 = back of tread (away from approaching person, toward wall).
        const float z0 = stepIndex * sd - pivotOffsetZ;
        const float z1 = z0 + sd;

        // FIX: nosing extends toward approaching person = toward LOWER z (−Z direction).
        // Previous code used z1n = z1+nosing → extended AWAY from person = WRONG.
        // Nosing only overhangs when there is a step below (stepIndex > 0).
        const float z0n = (stepIndex > 0 && nosing > 0.001f) ? z0 - nosing : z0;

        const float xL = -sw * 0.5f - pivotOffsetX;
        const float xR = sw * 0.5f - pivotOffsetX;

        // ── Tread top (N=(0,+1,0)) ────────────────────────────────────────────────
        // Tread spans from z0n (nosing edge, closer to person) to z1 (back of tread).
        // Cross check tq(0,3,2,1): (v3-v0)×(v2-v0) = (0,0,z1-z0n)×(sw,0,z1-z0n)
        //   = (0,(z1-z0n)*sw,0) → N=(0,+1,0)  
        this->treadVertexBase = static_cast<Ogre::uint32>(this->treadVertices.size() / 8u);
        tv(xL, y1, z0n, 0, 1, 0, 0.0f, 0.0f);
        tv(xR, y1, z0n, 0, 1, 0, 1.0f, 0.0f);
        tv(xR, y1, z1, 0, 1, 0, 1.0f, 1.0f);
        tv(xL, y1, z1, 0, 1, 0, 0.0f, 1.0f);
        tq(0, 3, 2, 1);

        // ── Riser (N=(0,0,−1)) ────────────────────────────────────────────────────
        // Riser at z=z0 faces the approaching person (−Z direction).
        // Cross check rq(0,3,2,1): (v3-v0)×(v2-v0) = (0,sh,0)×(sw,sh,0) = (0,0,-sh*sw)  
        if (!this->openRiser->getBool())
        {
            this->riserVertexBase = static_cast<Ogre::uint32>(this->riserVertices.size() / 8u);
            rv(xL, y0, z0, 0, 0, -1, 0.0f, 0.0f);
            rv(xR, y0, z0, 0, 0, -1, 1.0f, 0.0f);
            rv(xR, y1, z0, 0, 0, -1, 1.0f, 1.0f);
            rv(xL, y1, z0, 0, 0, -1, 0.0f, 1.0f);
            rq(0, 3, 2, 1);
        }

        // NOTE: No nosing underside cap.
        // The underside of the overhang (y=y1, z=[z0n,z0]) would z-fight with the tread
        // top at exactly the same coordinates. The riser of this step (at z=z0) and
        // the side caps close the geometry sufficiently for all practical game views.
    }

    // =========================================================================
    //  generateLinearStairs
    // =========================================================================

    void ProceduralStairsComponent::generateLinearStairs(void)
    {
        const int n = std::max(1, this->stepCount->getInt());
        const float sh = this->stepHeight->getReal();
        const float sd = this->stepDepth->getReal();
        const float sw = this->stepWidth->getReal();
        const float nosing = std::max(0.0f, std::min(this->stepNosing->getReal(), sd - 0.001f));
        const float totalH = n * sh;
        const float totalD = n * sd;

        Ogre::Real pivotOffsetX = 0.0f;
        Ogre::Real pivotOffsetZ = 0.0f;
        this->computePivotOffset(pivotOffsetX, pivotOffsetZ);

        const float xL = -sw * 0.5f - pivotOffsetX;
        const float xR = sw * 0.5f - pivotOffsetX;
        const float z0 = -pivotOffsetZ;
        const float z1 = totalD - pivotOffsetZ;

        // ── Tread tops and risers ─────────────────────────────────────────────────
        for (int i = 0; i < n; ++i)
        {
            this->generateLinearStep(i, pivotOffsetX, pivotOffsetZ);
        }

        // ── Bottom face at y=0 (N=(0,-1,0)) ──────────────────────────────────────
        // sq(0,1,2,3): (sw,0,0)×(sw,0,totalD) = (0,-sw*totalD,0) → N=(0,-1,0)  
        this->stringerVertexBase = static_cast<Ogre::uint32>(this->stringerVertices.size() / 8u);
        sv(xL, 0.0f, z0, 0, -1, 0, 0.0f, 0.0f);
        sv(xR, 0.0f, z0, 0, -1, 0, 1.0f, 0.0f);
        sv(xR, 0.0f, z1, 0, -1, 0, 1.0f, 1.0f);
        sv(xL, 0.0f, z1, 0, -1, 0, 0.0f, 1.0f);
        sq(0, 1, 2, 3);

        // ── Back face at z=z1 (N=(0,0,+1)) ───────────────────────────────────────
        // sq(0,1,2,3): (sw,0,0)×(sw,totalH,0) = (0,0,sw*totalH) → N=(0,0,+1)  
        this->stringerVertexBase = static_cast<Ogre::uint32>(this->stringerVertices.size() / 8u);
        sv(xL, 0.0f, z1, 0, 0, +1, 0.0f, 0.0f);
        sv(xR, 0.0f, z1, 0, 0, +1, 1.0f, 0.0f);
        sv(xR, totalH, z1, 0, 0, +1, 1.0f, 1.0f);
        sv(xL, totalH, z1, 0, 0, +1, 0.0f, 1.0f);
        sq(0, 1, 2, 3);
        sq(0, 3, 2, 1);

        // ── Stringer / side panels ────────────────────────────────────────────────
        // None:   no side geometry (open / floating stair look)
        // Closed: one solid rectangular board per side (stringer buffer)
        // Open:   per-step notched profile (stringer buffer, different material)
        switch (this->getStringerStyleEnum())
        {
        case StringerStyle::CLOSED:
            this->generateClosedStringer(pivotOffsetX, pivotOffsetZ);
            break;
        case StringerStyle::OPEN:
            this->generateOpenStringer(pivotOffsetX, pivotOffsetZ);
            break;
        default:
            break;
        }

        // ── Optional bottom style ─────────────────────────────────────────────────
        switch (this->getBottomStyleEnum())
        {
        case BottomStyle::SLOPED:
            this->generateSlopedBottom(pivotOffsetX, pivotOffsetZ);
            break;
        case BottomStyle::STEPPED:
            this->generateSteppedBottom(pivotOffsetX, pivotOffsetZ);
            break;
        default:
            break;
        }
    }

    void ProceduralStairsComponent::generateClosedStringer(Ogre::Real pivotOffsetX, Ogre::Real pivotOffsetZ)
    {
        // One solid rectangular board per side.
        // Spans the full stair profile: front-to-back, bottom-to-top.
        // Vertex layout  [0]=bot-front  [1]=bot-back  [2]=top-back  [3]=top-front

        const int n = std::max(1, this->stepCount->getInt());
        const float sh = this->stepHeight->getReal();
        const float sd = this->stepDepth->getReal();
        const float sw = this->stepWidth->getReal();
        const float totalH = n * sh;
        const float totalD = n * sd;

        for (int side = 0; side < 2; ++side)
        {
            const float xS = (side == 0) ? -sw * 0.5f - pivotOffsetX : sw * 0.5f - pivotOffsetX;
            const float nxS = (side == 0) ? -1.0f : 1.0f;
            const float z0 = -pivotOffsetZ;
            const float z1 = totalD - pivotOffsetZ;

            this->stringerVertexBase = static_cast<Ogre::uint32>(this->stringerVertices.size() / 8u);

            sv(xS, 0.0f, z0, nxS, 0, 0, 0.0f, 0.0f);   // [0] bot-front
            sv(xS, 0.0f, z1, nxS, 0, 0, 1.0f, 0.0f);   // [1] bot-back
            sv(xS, totalH, z1, nxS, 0, 0, 1.0f, 1.0f); // [2] top-back
            sv(xS, totalH, z0, nxS, 0, 0, 0.0f, 1.0f); // [3] top-front

            // Left:  sq(0,1,2,3): st(0,1,2): (0,0,totalD)×(0,totalH,totalD)
            //        = (-totalH*totalD,0,0) → N=(-1,0,0)  
            // Right: sq(0,3,2,1): st(0,3,2): (0,totalH,0)×(0,totalH,totalD)
            //        = (totalH*totalD,0,0) → N=(+1,0,0)  
            if (side == 0)
            {
                sq(0, 1, 2, 3);
                sq(0, 3, 2, 1);
            }
            else
            {
                sq(0, 3, 2, 1);
                sq(0, 1, 2, 3);
            }
        }
    }

    void ProceduralStairsComponent::generateOpenStringer(Ogre::Real pivotOffsetX, Ogre::Real pivotOffsetZ)
    {
        // Per-step notched profile written to the STRINGER buffer.
        // Visually identical shape to the stair side profile, but rendered
        // with the stringer datablock/material instead of the tread datablock.
        // This lets designers use a different wood/metal texture on the stringer
        // while keeping the step-notch silhouette visible.

        const int n = std::max(1, this->stepCount->getInt());
        const float sh = this->stepHeight->getReal();
        const float sd = this->stepDepth->getReal();
        const float sw = this->stepWidth->getReal();
        const float nosing = std::max(0.0f, std::min(this->stepNosing->getReal(), sd - 0.001f));

        for (int side = 0; side < 2; ++side)
        {
            const float xS = (side == 0) ? -sw * 0.5f - pivotOffsetX : sw * 0.5f - pivotOffsetX;
            const float nxS = (side == 0) ? -1.0f : 1.0f;

            for (int i = 0; i < n; ++i)
            {
                const float y0s = i * sh;
                const float y1s = y0s + sh;
                const float z0s = i * sd - pivotOffsetZ;
                const float z1s = z0s + sd;
                // Nosing only applies from step 1 onward
                const float z0sn = (i > 0 && nosing > 0.001f) ? z0s - nosing : z0s;

                // ── Tread-level side band: y=[y0s,y1s], z=[z0sn,z1s] ─────────────
                // Left:  sq(0,3,2,1): st(0,3,2): (0,0,z1s-z0sn)×(0,sh,z1s-z0sn)
                //        = (-sh*(z1s-z0sn),0,0) → N=(-1,0,0)  
                // Right: sq(0,1,2,3): same → N=(+1,0,0)  
                this->stringerVertexBase = static_cast<Ogre::uint32>(this->stringerVertices.size() / 8u);
                sv(xS, y0s, z0sn, nxS, 0, 0, 0.0f, 0.0f);
                sv(xS, y1s, z0sn, nxS, 0, 0, 0.0f, 1.0f);
                sv(xS, y1s, z1s, nxS, 0, 0, 1.0f, 1.0f);
                sv(xS, y0s, z1s, nxS, 0, 0, 1.0f, 0.0f);
                if (side == 0)
                {
                    sq(0, 3, 2, 1);
                }
                else
                {
                    sq(0, 1, 2, 3);
                }

                // ── Filler below this step: y=[0,y0s], z=[z0s,z1s] ──────────────
                // Fills the "triangle" of the stair profile below each step column.
                // Uses z0s (not z0sn) — filler is flush against the riser above it.
                if (i > 0)
                {
                    this->stringerVertexBase = static_cast<Ogre::uint32>(this->stringerVertices.size() / 8u);
                    sv(xS, 0.0f, z0s, nxS, 0, 0, 0.0f, 0.0f);
                    sv(xS, y0s, z0s, nxS, 0, 0, 0.0f, 1.0f);
                    sv(xS, y0s, z1s, nxS, 0, 0, 1.0f, 1.0f);
                    sv(xS, 0.0f, z1s, nxS, 0, 0, 1.0f, 0.0f);
                    if (side == 0)
                    {
                        sq(0, 3, 2, 1);
                    }
                    else
                    {
                        sq(0, 1, 2, 3);
                    }
                }
            }
        }
    }

    // =========================================================================
    //  generateSlopedBottom  –  single diagonal soffit
    // =========================================================================

    void ProceduralStairsComponent::generateSlopedBottom(Ogre::Real pivotOffsetX, Ogre::Real pivotOffsetZ)
    {
        const int n = std::max(1, this->stepCount->getInt());
        const float sh = this->stepHeight->getReal();
        const float sd = this->stepDepth->getReal();
        const float sw = this->stepWidth->getReal();

        const float totalH = n * sh;
        const float totalD = n * sd;

        const float xL = -sw * 0.5f - pivotOffsetX;
        const float xR = sw * 0.5f - pivotOffsetX;
        const float z0 = -pivotOffsetZ;
        const float z1 = totalD - pivotOffsetZ;

        // Slope normal = cross( (xR-xL, 0, 0), (0, totalH, totalD) ) normalised
        // = (0, -totalD, totalH) normalised — pointing downward/outward
        Ogre::Vector3 slopeN = Ogre::Vector3(0.0f, -totalD, totalH);
        slopeN.normalise();

        this->stringerVertexBase = static_cast<Ogre::uint32>(this->stringerVertices.size() / 8u);

        sv(xL, 0.0f, z0, slopeN.x, slopeN.y, slopeN.z, 0.0f, 0.0f);
        sv(xR, 0.0f, z0, slopeN.x, slopeN.y, slopeN.z, 1.0f, 0.0f);
        sv(xR, totalH, z1, slopeN.x, slopeN.y, slopeN.z, 1.0f, 1.0f);
        sv(xL, totalH, z1, slopeN.x, slopeN.y, slopeN.z, 0.0f, 1.0f);

        sq(0, 1, 2, 3); // CCW -> slopeN outward 
    }

    // =========================================================================
    //  generateSteppedBottom  –  mirrors the step geometry underneath
    // =========================================================================

    void ProceduralStairsComponent::generateSteppedBottom(Ogre::Real pivotOffsetX, Ogre::Real pivotOffsetZ)
    {
        const int n = std::max(1, this->stepCount->getInt());
        const float sh = this->stepHeight->getReal();
        const float sd = this->stepDepth->getReal();
        const float sw = this->stepWidth->getReal();

        const float xL = -sw * 0.5f - pivotOffsetX;
        const float xR = sw * 0.5f - pivotOffsetX;

        for (int i = 0; i < n; ++i)
        {
            const float y0 = i * sh;
            const float z0 = i * sd - pivotOffsetZ;
            const float z1 = z0 + sd;

            // Horizontal underside strip at this step's base level (N=(0,-1,0))
            this->stringerVertexBase = static_cast<Ogre::uint32>(this->stringerVertices.size() / 8u);
            sv(xL, y0, z0, 0, -1, 0, 0.0f, 0.0f);
            sv(xR, y0, z0, 0, -1, 0, 1.0f, 0.0f);
            sv(xR, y0, z1, 0, -1, 0, 1.0f, 1.0f);
            sv(xL, y0, z1, 0, -1, 0, 0.0f, 1.0f);
            sq(0, 1, 2, 3);

            // Vertical back face under each tread (N=(0,0,+1))
            if (i > 0)
            {
                const float yPrev = (i - 1) * sh;
                this->stringerVertexBase = static_cast<Ogre::uint32>(this->stringerVertices.size() / 8u);
                sv(xL, yPrev, z0, 0, 0, 1, 0.0f, 0.0f);
                sv(xR, yPrev, z0, 0, 0, 1, 1.0f, 0.0f);
                sv(xR, y0, z0, 0, 0, 1, 1.0f, 1.0f);
                sv(xL, y0, z0, 0, 0, 1, 0.0f, 1.0f);
                sq(0, 1, 2, 3);
            }
        }
    }

    // =========================================================================
    //  generateCurvedStairs
    // =========================================================================

    void ProceduralStairsComponent::generateCurvedStairs(void)
    {
        const int n = std::max(1, this->stepCount->getInt());
        const float sh = this->stepHeight->getReal();
        const float rInner = std::max(0.01f, this->innerRadius->getReal());
        const float rOuter = std::max(rInner + 0.01f, this->outerRadius->getReal());
        const float totalArcDeg = this->arcAngle->getReal();
        const float stepArcDeg = totalArcDeg / n;
        const bool cw = (this->rotationDir->getListSelectedValue() == "Clockwise");
        const float dirMul = cw ? -1.0f : 1.0f;

        float startAngleDeg = 0.0f;
        switch (this->getPivotPositionEnum())
        {
        case PivotPosition::BOTTOM_FRONT:
            startAngleDeg = 0.0f;
            break;
        case PivotPosition::BOTTOM_CENTRE:
            startAngleDeg = -totalArcDeg * 0.5f;
            break;
        case PivotPosition::BOTTOM_BACK:
            startAngleDeg = -totalArcDeg;
            break;
        }

        const float midR = (rInner + rOuter) * 0.5f;
        float nosingDeg = 0.0f;
        if (this->stepNosing->getReal() > 0.001f && midR > 0.001f)
        {
            nosingDeg = Ogre::Math::RadiansToDegrees(this->stepNosing->getReal() / midR);
            nosingDeg = std::min(nosingDeg, stepArcDeg - 0.01f);
        }

        for (int i = 0; i < n; ++i)
        {
            const float y0 = i * sh;
            const float y1 = y0 + sh;

            const float aStartDeg = startAngleDeg + i * stepArcDeg;
            const float aEndDeg = startAngleDeg + (i + 1) * stepArcDeg;
            const float aMidDeg = (aStartDeg + aEndDeg) * 0.5f;
            const float aTFrontDeg = (i > 0 && nosingDeg > 0.001f) ? aStartDeg - nosingDeg : aStartDeg;

            const float aStart = Ogre::Math::DegreesToRadians(aStartDeg) * dirMul;
            const float aEnd = Ogre::Math::DegreesToRadians(aEndDeg) * dirMul;
            const float aMid = Ogre::Math::DegreesToRadians(aMidDeg) * dirMul;
            const float aTFront = Ogre::Math::DegreesToRadians(aTFrontDeg) * dirMul;

            const float csS = std::cos(aStart), snS = std::sin(aStart);
            const float csE = std::cos(aEnd), snE = std::sin(aEnd);
            const float csM = std::cos(aMid), snM = std::sin(aMid);
            const float csF = std::cos(aTFront), snF = std::sin(aTFront);

            // Precomputed corner positions
            const float iSx = rInner * csS, iSz = rInner * snS;
            const float oSx = rOuter * csS, oSz = rOuter * snS;
            const float iEx = rInner * csE, iEz = rInner * snE;
            const float oEx = rOuter * csE, oEz = rOuter * snE;
            const float iFx = rInner * csF, iFz = rInner * snF;
            const float oFx = rOuter * csF, oFz = rOuter * snF;

            // ── Face 1: Tread top (N=(0,+1,0)) ───────────────────────────────────
            // CCW tq(0,3,2,1): 2D cross in XZ plane = +Y  
            this->treadVertexBase = static_cast<Ogre::uint32>(this->treadVertices.size() / 8u);
            tv(iFx, y1, iFz, 0, 1, 0, 0.0f, 0.0f); // [0] nosing-inner
            tv(oFx, y1, oFz, 0, 1, 0, 1.0f, 0.0f); // [1] nosing-outer
            tv(oEx, y1, oEz, 0, 1, 0, 1.0f, 1.0f); // [2] end-outer
            tv(iEx, y1, iEz, 0, 1, 0, 0.0f, 1.0f); // [3] end-inner
            if (!cw)
            {
                tq(0, 3, 2, 1);
            }
            else
            {
                tq(0, 1, 2, 3);
            }

            // ── Face 2: Riser at aStart (against travel) ──────────────────────────
            // N = (snS*dirMul, 0, -csS*dirMul). CCW rq(0,3,2,1)  
            if (!this->openRiser->getBool())
            {
                const float rNx = snS * dirMul;
                const float rNz = -csS * dirMul;
                this->riserVertexBase = static_cast<Ogre::uint32>(this->riserVertices.size() / 8u);
                rv(iSx, y0, iSz, rNx, 0, rNz, 0.0f, 0.0f); // [0] inner-bot
                rv(oSx, y0, oSz, rNx, 0, rNz, 1.0f, 0.0f); // [1] outer-bot
                rv(oSx, y1, oSz, rNx, 0, rNz, 1.0f, 1.0f); // [2] outer-top
                rv(iSx, y1, iSz, rNx, 0, rNz, 0.0f, 1.0f); // [3] inner-top
                if (!cw)
                {
                    rq(0, 3, 2, 1);
                }
                else
                {
                    rq(0, 1, 2, 3);
                }
            }

            // ── Face 3: Inner arc (N = outward from inner cylinder) ───────────────
            // N = (+csM,0,+snM): visible from stair body (rInner < r < rOuter).
            // Proof CCW sq(0,3,2,1): (v3-v0)×(v2-v0) = sh*dr*(snE-snS, 0, -(csE-csS))
            //   at step0 CCW 30°: (0.5sh*rInner, 0, 0.134sh*rInner) → N∝(+csM,+snM)  
            this->stringerVertexBase = static_cast<Ogre::uint32>(this->stringerVertices.size() / 8u);
            sv(iSx, y0, iSz, csM, 0, snM, 0.0f, 0.0f); // [0] start-bot
            sv(iEx, y0, iEz, csM, 0, snM, 1.0f, 0.0f); // [1] end-bot
            sv(iEx, y1, iEz, csM, 0, snM, 1.0f, 1.0f); // [2] end-top
            sv(iSx, y1, iSz, csM, 0, snM, 0.0f, 1.0f); // [3] start-top
            if (!cw)
            {
                sq(0, 3, 2, 1);
            }
            else
            {
                sq(0, 1, 2, 3);
            }

            // ── Face 4: Outer arc (N = outward from outer surface) ────────────────
            // N = (+csM,0,+snM): visible from outside (r > rOuter).
            // Same winding as inner — both arcs curve in the same angular direction.
            this->stringerVertexBase = static_cast<Ogre::uint32>(this->stringerVertices.size() / 8u);
            sv(oSx, y0, oSz, csM, 0, snM, 0.0f, 0.0f);
            sv(oEx, y0, oEz, csM, 0, snM, 1.0f, 0.0f);
            sv(oEx, y1, oEz, csM, 0, snM, 1.0f, 1.0f);
            sv(oSx, y1, oSz, csM, 0, snM, 0.0f, 1.0f);
            if (!cw)
            {
                sq(0, 3, 2, 1);
            }
            else
            {
                sq(0, 1, 2, 3);
            }

            // ── Face 5: Step bottom (N=(0,-1,0)) ─────────────────────────────────
            // Only emit when bottom style is Stepped (one wedge bottom per step)
            // or when this is step 0 and bottom style is Sloped/Stepped
            // (step 0 sits at y=0 so it doubles as the base).
            // Proof CCW tq(0,3,2,1): st(0,3,2):
            //   v3-v0 = rInner*(cos30-1, 0, sin30),  v2-v0 = (rOuter*cos30-rInner, 0, rOuter*sin30)
            //   Y = rInner*(cos30-1)*rOuter*sin30 - rInner*sin30*(rOuter*cos30-rInner)
            //     = rInner*sin30*(rInner-rOuter) < 0  →  N=(0,-1,0)  
            // FIX: was tq(0,1,2,3) for CCW → N=(0,+1,0) WRONG (step bottoms lit from below)
            {
                const bool addBottom = (this->getBottomStyleEnum() == BottomStyle::STEPPED) || (this->getBottomStyleEnum() == BottomStyle::SLOPED && i == 0);

                if (addBottom)
                {
                    this->treadVertexBase = static_cast<Ogre::uint32>(this->treadVertices.size() / 8u);
                    tv(iSx, y0, iSz, 0, -1, 0, 0.0f, 0.0f);
                    tv(oSx, y0, oSz, 0, -1, 0, 1.0f, 0.0f);
                    tv(oEx, y0, oEz, 0, -1, 0, 1.0f, 1.0f);
                    tv(iEx, y0, iEz, 0, -1, 0, 0.0f, 1.0f);
                    // FIX: swap winding for correct N=(0,-1,0)
                    if (!cw)
                    {
                        tq(0, 3, 2, 1);
                    }
                    else
                    {
                        tq(0, 1, 2, 3);
                    }
                }
            }

            // ── Face 6: End face at aEnd (forward travel direction) ───────────────
            // THIS WAS THE MISSING FACE. Each step is a wedge with 6 sides.
            // Without this face the wedge was open at aEnd — visible as dark holes
            // between steps when viewed from outside the stair.
            // N = (-snE*dirMul, 0, csE*dirMul).
            // CCW rq(0,1,2,3): (v1-v0)×(v2-v0) = dr*sh*(-snE,0,csE)  
            // Only intermediate steps — last step gets Face 8 (exit cap) instead.
            if (i < n - 1)
            {
                const float eNx = -snE * dirMul;
                const float eNz = csE * dirMul;
                this->riserVertexBase = static_cast<Ogre::uint32>(this->riserVertices.size() / 8u);
                rv(iEx, y0, iEz, eNx, 0, eNz, 0.0f, 0.0f); // [0] inner-bot
                rv(oEx, y0, oEz, eNx, 0, eNz, 1.0f, 0.0f); // [1] outer-bot
                rv(oEx, y1, oEz, eNx, 0, eNz, 1.0f, 1.0f); // [2] outer-top
                rv(iEx, y1, iEz, eNx, 0, eNz, 0.0f, 1.0f); // [3] inner-top
                if (!cw)
                {
                    rq(0, 1, 2, 3);
                }
                else
                {
                    rq(0, 3, 2, 1);
                }
            }
        }

        // ── Bottom annular cap at y=0 (N=(0,-1,0)) ───────────────────────────────
        // None:    no cap
        // Sloped:  full annular disk at y=0 (flat base, one sector per step)
        // Stepped: fill sectors i=1..n-1 at y=0 only (sector 0 covered by Face 5 step0)
        //
        // Proof CCW tq(0,3,2,1): same analysis as Face 5 → N=(0,-1,0)  
        // FIX: was tq(0,1,2,3) for CCW → N=(0,+1,0) WRONG (visible as dark disk from below)
        if (this->getBottomStyleEnum() != BottomStyle::NONE)
        {
            // Sloped: emit all sectors including 0 (no per-step Face 5 except step0).
            // Stepped: emit sectors 1..n-1 (sector 0 already covered by Face 5 of step 0).
            const int startSector = (this->getBottomStyleEnum() == BottomStyle::SLOPED) ? 0 : 1;

            for (int i = startSector; i < n; ++i)
            {
                const float aS = Ogre::Math::DegreesToRadians(startAngleDeg + i * stepArcDeg) * dirMul;
                const float aE = Ogre::Math::DegreesToRadians(startAngleDeg + (i + 1) * stepArcDeg) * dirMul;
                const float csS2 = std::cos(aS), snS2 = std::sin(aS);
                const float csE2 = std::cos(aE), snE2 = std::sin(aE);

                this->treadVertexBase = static_cast<Ogre::uint32>(this->treadVertices.size() / 8u);
                tv(rInner * csS2, 0.0f, rInner * snS2, 0, -1, 0, 0.0f, 0.0f);
                tv(rOuter * csS2, 0.0f, rOuter * snS2, 0, -1, 0, 1.0f, 0.0f);
                tv(rOuter * csE2, 0.0f, rOuter * snE2, 0, -1, 0, 1.0f, 1.0f);
                tv(rInner * csE2, 0.0f, rInner * snE2, 0, -1, 0, 0.0f, 1.0f);
                if (!cw)
                {
                    tq(0, 3, 2, 1);
                }
                else
                {
                    tq(0, 1, 2, 3);
                }
            }
        }

        // ── Centre pole ───────────────────────────────────────────────────────────
        if (this->centrePole->getBool())
        {
            this->generateCentrePole();
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralStairsComponent] Curved stair built: " + Ogre::StringConverter::toString(n) + " steps  arc=" + Ogre::StringConverter::toString(totalArcDeg) + "°  " +
                                                                               (cw ? "CW" : "CCW") + "  rInner=" + Ogre::StringConverter::toString(rInner) + "  rOuter=" + Ogre::StringConverter::toString(rOuter));
    }

    // =========================================================================
    //  generateCentrePole  –  simple cylinder at inner radius
    // =========================================================================

    void ProceduralStairsComponent::generateCentrePole(void)
    {
        const float r = this->innerRadius->getReal();
        const float h = this->stepCount->getInt() * this->stepHeight->getReal();
        const int slices = 12;

        for (int sl = 0; sl < slices; ++sl)
        {
            const float t0 = sl * Ogre::Math::TWO_PI / slices;
            const float t1 = (sl + 1) * Ogre::Math::TWO_PI / slices;
            const float c0 = std::cos(t0), s0 = std::sin(t0);
            const float c1 = std::cos(t1), s1 = std::sin(t1);
            const float u0 = static_cast<float>(sl) / slices;
            const float u1 = static_cast<float>(sl + 1) / slices;

            // ── Lateral strip ─────────────────────────────────────────────────────
            this->stringerVertexBase = static_cast<Ogre::uint32>(this->stringerVertices.size() / 8u);
            sv(r * c0, 0, r * s0, c0, 0, s0, u0, 0.0f); // [0] bot-start
            sv(r * c1, 0, r * s1, c1, 0, s1, u1, 0.0f); // [1] bot-end
            sv(r * c1, h, r * s1, c1, 0, s1, u1, 1.0f); // [2] top-end
            sv(r * c0, h, r * s0, c0, 0, s0, u0, 1.0f); // [3] top-start

            // FIX: was sq(0,1,2,3) → st(0,1,2): (rΔc,0,rΔs)×(rΔc,h,rΔs) → INWARD.
            // sq(0,3,2,1) → st(0,3,2): (v3-v0)×(v2-v0)=(0,h,0)×(rΔc,h,rΔs)
            //   =(h*rΔs-0*h, 0*rΔc-0*rΔs, 0*h-h*rΔc)=(h*rΔs,0,-h*rΔc)
            //   ∝ (sin(dθ),0,-Δc) = (sin,0,1-cos) which is outward  
            sq(0, 3, 2, 1);

            // ── Top cap ───────────────────────────────────────────────────────────
            this->stringerVertexBase = static_cast<Ogre::uint32>(this->stringerVertices.size() / 8u);
            sv(0, h, 0, 0, 1, 0, 0.5f, 0.5f);
            sv(r * c0, h, r * s0, 0, 1, 0, 0.5f + c0 * 0.5f, 0.5f + s0 * 0.5f);
            sv(r * c1, h, r * s1, 0, 1, 0, 0.5f + c1 * 0.5f, 0.5f + s1 * 0.5f);
            // st(0,2,1): (v2-v0)×(v1-v0)=(rcos1,0,rsin1)×(rcos0,0,0)
            //   =(0*0-rsin1*0, rsin1*rcos0-rcos1*0, rcos1*0-0*rcos0)
            //   =(0,rsin1*rcos0,0) → for sl=0 sin1>0 → N=(0,+1,0)  
            st(0, 2, 1);
        }
    }

    // =========================================================================
    //  uploadSubMesh  –  8-float -> 12-float GPU upload
    // =========================================================================

    void ProceduralStairsComponent::uploadSubMesh(Ogre::SubMesh* subMesh, const std::vector<float>& srcVerts, const std::vector<Ogre::uint32>& srcIdx, size_t numVerts, Ogre::VaoManager* vaoManager, Ogre::Vector3& inOutMin, Ogre::Vector3& inOutMax)
    {
        const size_t srcStride = 8u;
        const size_t dstStride = 12u;
        const size_t dstBytes = numVerts * dstStride * sizeof(float);

        Ogre::VertexElement2Vec elements;
        elements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
        elements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));
        elements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT4, Ogre::VES_TANGENT));
        elements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));

        float* dst = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(dstBytes, Ogre::MEMCATEGORY_GEOMETRY));

        for (size_t i = 0; i < numVerts; ++i)
        {
            const size_t s = i * srcStride;
            const size_t d = i * dstStride;

            dst[d + 0] = srcVerts[s + 0];
            dst[d + 1] = srcVerts[s + 1];
            dst[d + 2] = srcVerts[s + 2];

            Ogre::Vector3 pos(dst[d + 0], dst[d + 1], dst[d + 2]);
            inOutMin.makeFloor(pos);
            inOutMax.makeCeil(pos);

            Ogre::Vector3 n(srcVerts[s + 3], srcVerts[s + 4], srcVerts[s + 5]);
            dst[d + 3] = n.x;
            dst[d + 4] = n.y;
            dst[d + 5] = n.z;

            // Tangent derived from normal
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
            dst[d + 9] = 1.0f;

            dst[d + 10] = srcVerts[s + 6];
            dst[d + 11] = srcVerts[s + 7];
        }

        Ogre::VertexBufferPacked* vb = vaoManager->createVertexBuffer(elements, numVerts, Ogre::BT_IMMUTABLE, dst, true);

        const size_t idxBytes = srcIdx.size() * sizeof(Ogre::uint32);
        Ogre::uint32* idxData = reinterpret_cast<Ogre::uint32*>(OGRE_MALLOC_SIMD(idxBytes, Ogre::MEMCATEGORY_GEOMETRY));
        memcpy(idxData, srcIdx.data(), idxBytes);

        Ogre::IndexBufferPacked* ib = vaoManager->createIndexBuffer(Ogre::IndexBufferPacked::IT_32BIT, srcIdx.size(), Ogre::BT_IMMUTABLE, idxData, true);

        Ogre::VertexBufferPackedVec vbVec;
        vbVec.push_back(vb);
        Ogre::VertexArrayObject* vao = vaoManager->createVertexArrayObject(vbVec, ib, Ogre::OT_TRIANGLE_LIST);

        subMesh->mVao[Ogre::VpNormal].push_back(vao);
        subMesh->mVao[Ogre::VpShadow].push_back(vao);
    }

    // =========================================================================
    //  createStairsMesh
    // =========================================================================

    void ProceduralStairsComponent::createStairsMesh(void)
    {
        this->buildGeometry();

        const std::vector<float> tVerts = this->treadVertices;
        const std::vector<Ogre::uint32> tIdx = this->treadIndices;
        const size_t tNV = this->treadVertices.size() / 8u;

        const std::vector<float> rVerts = this->riserVertices;
        const std::vector<Ogre::uint32> rIdx = this->riserIndices;
        const size_t rNV = this->riserVertices.size() / 8u;

        const std::vector<float> sVerts = this->stringerVertices;
        const std::vector<Ogre::uint32> sIdx = this->stringerIndices;
        const size_t sNV = this->stringerVertices.size() / 8u;

        const std::vector<float> pVerts = this->rampVertices;
        const std::vector<Ogre::uint32> pIdx = this->rampIndices;
        const size_t pNV = this->rampVertices.size() / 8u;

        NOWA::GraphicsModule::RenderCommand cmd = [this, tVerts, tIdx, tNV, rVerts, rIdx, rNV, sVerts, sIdx, sNV, pVerts, pIdx, pNV]()
        {
            this->createStairsMeshInternal(tVerts, tIdx, tNV, rVerts, rIdx, rNV, sVerts, sIdx, sNV, pVerts, pIdx, pNV);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "ProceduralStairsComponent::createStairsMesh");
    }

    void ProceduralStairsComponent::createStairsMeshInternal(const std::vector<float>& treadVerts, const std::vector<Ogre::uint32>& treadIdx, size_t numTreadVerts, const std::vector<float>& riserVerts, const std::vector<Ogre::uint32>& riserIdx,
        size_t numRiserVerts, const std::vector<float>& stringerVerts, const std::vector<Ogre::uint32>& stringerIdx, size_t numStringerVerts, const std::vector<float>& rampVerts, const std::vector<Ogre::uint32>& rampIdx, size_t numRampVerts)
    {
        if (numTreadVerts == 0)
        {
            return;
        }

        Ogre::Root* root = Ogre::Root::getSingletonPtr();
        Ogre::VaoManager* vaoManager = root->getRenderSystem()->getVaoManager();
        const Ogre::String groupName = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

        const Ogre::String meshName = this->gameObjectPtr->getName() + "_Stairs_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());

        // Remove stale mesh
        {
            Ogre::MeshPtr ex = Ogre::MeshManager::getSingleton().getByName(meshName, groupName);
            if (false == ex.isNull())
            {
                Ogre::MeshManager::getSingleton().remove(ex->getHandle());
            }
        }

        this->stairsMesh = Ogre::MeshManager::getSingleton().createManual(meshName, groupName);

        Ogre::Vector3 minBB(std::numeric_limits<float>::max());
        Ogre::Vector3 maxBB(-std::numeric_limits<float>::max());

        // ── Submesh 0: Tread ──────────────────────────────────────────────────────
        {
            Ogre::SubMesh* sm = this->stairsMesh->createSubMesh();
            this->uploadSubMesh(sm, treadVerts, treadIdx, numTreadVerts, vaoManager, minBB, maxBB);
        }

        // ── Submesh 1: Riser ──────────────────────────────────────────────────────
        if (numRiserVerts > 0 && !riserIdx.empty())
        {
            Ogre::SubMesh* sm = this->stairsMesh->createSubMesh();
            this->uploadSubMesh(sm, riserVerts, riserIdx, numRiserVerts, vaoManager, minBB, maxBB);
        }

        // ── Submesh 2: Stringer / soffit ──────────────────────────────────────────
        if (numStringerVerts > 0 && !stringerIdx.empty())
        {
            Ogre::SubMesh* sm = this->stairsMesh->createSubMesh();
            this->uploadSubMesh(sm, stringerVerts, stringerIdx, numStringerVerts, vaoManager, minBB, maxBB);
        }

        // ── Submesh 3: Ramp collider (invisible physics helper) ───────────────────
        if (numRampVerts > 0 && !rampIdx.empty())
        {
            Ogre::SubMesh* sm = this->stairsMesh->createSubMesh();
            this->uploadSubMesh(sm, rampVerts, rampIdx, numRampVerts, vaoManager, minBB, maxBB);
        }

        // ── Bounds ────────────────────────────────────────────────────────────────
        if (minBB.x > maxBB.x)
        {
            minBB = Ogre::Vector3(-1.0f);
            maxBB = Ogre::Vector3(1.0f);
        }
        Ogre::Aabb aabb;
        aabb.setExtents(minBB, maxBB);
        this->stairsMesh->_setBounds(aabb, false);
        this->stairsMesh->_setBoundingSphereRadius(aabb.getRadius());

        // ── Create Item ───────────────────────────────────────────────────────────
        this->stairsItem = this->gameObjectPtr->getSceneManager()->createItem(this->stairsMesh, this->gameObjectPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC);

        this->applyDatablocks(this->stairsItem);

        this->gameObjectPtr->getSceneNode()->attachObject(this->stairsItem);
        this->gameObjectPtr->setDoNotDestroyMovableObject(true);
        this->gameObjectPtr->init(this->stairsItem);

        if (nullptr != this->physicsArtifactComponent)
        {
            this->physicsArtifactComponent->reCreateCollision();
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralStairsComponent] Mesh created: " + Ogre::StringConverter::toString(this->stairsMesh->getNumSubMeshes()) + " submeshes for: " + this->gameObjectPtr->getName());
    }

    // =========================================================================
    //  applyDatablocks
    // =========================================================================

    void ProceduralStairsComponent::applyDatablocks(Ogre::Item* item)
    {
        if (nullptr == item)
        {
            return;
        }

        Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingleton().getHlmsManager();

        // ── Submesh 0: Tread ──────────────────────────────────────────────────────
        unsigned int subIdx = 0;
        if (subIdx < item->getNumSubItems() && false == this->treadDatablock->getString().empty())
        {
            Ogre::HlmsDatablock* db = hlmsManager->getDatablockNoDefault(this->treadDatablock->getString());
            if (nullptr != db)
            {
                item->getSubItem(subIdx)->setDatablock(db);
            }
        }
        ++subIdx;

        // ── Submesh 1: Riser ──────────────────────────────────────────────────────
        if (subIdx < item->getNumSubItems() && false == this->riserDatablock->getString().empty())
        {
            Ogre::HlmsDatablock* db = hlmsManager->getDatablockNoDefault(this->riserDatablock->getString());
            if (nullptr != db)
            {
                item->getSubItem(subIdx)->setDatablock(db);
            }
        }
        ++subIdx;

        // ── Submesh 2: Stringer / soffit / centre pole ────────────────────────────
        if (subIdx < item->getNumSubItems() && false == this->stringerDatablock->getString().empty())
        {
            Ogre::HlmsDatablock* db = hlmsManager->getDatablockNoDefault(this->stringerDatablock->getString());
            if (nullptr != db)
            {
                item->getSubItem(subIdx)->setDatablock(db);
            }
        }
        ++subIdx;

        // ── Submesh 3: Ramp collider (auto-created fully transparent datablock) ────
        // Only present when Ramp Collider = true AND the ramp submesh was generated.
        if (subIdx < item->getNumSubItems() && this->rampCollider->getBool())
        {
            // Unique name per GameObject so multiple stair instances never share
            // the same datablock object (each may be destroyed independently).
            const Ogre::String rampDbName = "StairsRamp_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());

            // Get the PBS HLMS — the ramp datablock must be a PBS datablock so that
            // setTransparency() is available (it is a PBS-specific method).
            Ogre::HlmsPbs* hlmsPbs = static_cast<Ogre::HlmsPbs*>(hlmsManager->getHlms(Ogre::HLMS_PBS));

            // Try to reuse an existing datablock from a previous rebuildMesh call.
            Ogre::HlmsPbsDatablock* rampDb = static_cast<Ogre::HlmsPbsDatablock*>(hlmsManager->getDatablockNoDefault(rampDbName));

            if (nullptr == rampDb)
            {
                // First time: create the datablock with default macro/blend blocks.
                // The blend block will be overridden by setTransparency() below.
                rampDb = static_cast<Ogre::HlmsPbsDatablock*>(hlmsPbs->createDatablock(rampDbName, // internal name (IdString)
                    rampDbName,                                                                    // human-readable name
                    Ogre::HlmsMacroblock(),                                                        // default: depth write on, no cull
                    Ogre::HlmsBlendblock(),                                                        // default: opaque — overridden below
                    Ogre::HlmsParamVec()));                                                        // no extra params

                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralStairsComponent] Created transparent ramp datablock: " + rampDbName);
            }

            // transparency = 1.0  → fully transparent (alpha = 0, invisible).
            // Ogre::HlmsPbsDatablock::Transparent → alpha-blend mode (not fade/none).
            // useAlphaFromTextures = false → ignore any texture alpha; use the
            //   transparency value alone so the face is always invisible regardless
            //   of which texture (if any) the user later assigns.
            // TODO: Comment out to see it. And its a nice effect, if it would be always visible!
            rampDb->setTransparency(0.0f, Ogre::HlmsPbsDatablock::Transparent, false /*useAlphaFromTextures*/);

            // Apply to the ramp submesh subitem.
            // subIdx is 3 here (tread=0, riser=1, stringer=2, ramp=3).
            item->getSubItem(subIdx)->setDatablock(rampDb);

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralStairsComponent] Applied ramp datablock '" + rampDbName + "' to subItem " + Ogre::StringConverter::toString(subIdx));
        }
    }

    // =========================================================================
    //  destroyStairsMesh
    // =========================================================================

    void ProceduralStairsComponent::destroyStairsMesh(void)
    {
        if (nullptr == this->stairsItem && this->stairsMesh.isNull())
        {
            return;
        }

        NOWA::GraphicsModule::RenderCommand cmd = [this]()
        {
            if (nullptr != this->stairsItem)
            {
                if (this->stairsItem->getParentSceneNode())
                {
                    this->stairsItem->getParentSceneNode()->detachObject(this->stairsItem);
                }
                this->gameObjectPtr->getSceneManager()->destroyItem(this->stairsItem);
                this->stairsItem = nullptr;
                this->gameObjectPtr->nullMovableObject();
            }
            if (false == this->stairsMesh.isNull())
            {
                Ogre::MeshManager::getSingleton().remove(this->stairsMesh->getHandle());
                this->stairsMesh.reset();
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "ProceduralStairsComponent::destroyStairsMesh");
    }

    // =========================================================================
    //  rebuildMesh
    // =========================================================================

    void ProceduralStairsComponent::rebuildMesh(void)
    {
        this->destroyStairsMesh();
        this->createStairsMesh();
    }

    // =========================================================================
    //  Preview mesh  (same lazy pattern as ProceduralWallComponent)
    // =========================================================================

    void ProceduralStairsComponent::createPreviewMesh(void)
    {
        this->buildGeometry();

        if (this->treadVertices.empty())
        {
            return;
        }

        // Merge all 3 buffers into one flat list for the preview
        std::vector<float> allVerts;
        std::vector<Ogre::uint32> allIdx;

        auto appendBuffer = [&](const std::vector<float>& verts, const std::vector<Ogre::uint32>& idxs)
        {
            const Ogre::uint32 offset = static_cast<Ogre::uint32>(allVerts.size() / 8u);
            allVerts.insert(allVerts.end(), verts.begin(), verts.end());
            for (Ogre::uint32 i : idxs)
            {
                allIdx.push_back(i + offset);
            }
        };

        appendBuffer(this->treadVertices, this->treadIndices);
        appendBuffer(this->riserVertices, this->riserIndices);
        appendBuffer(this->stringerVertices, this->stringerIndices);

        const size_t numVerts = allVerts.size() / 8u;
        const Ogre::String mn = this->gameObjectPtr->getName() + "_StairsPrev_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());

        std::vector<float> vertsCopy = allVerts;
        std::vector<Ogre::uint32> idxsCopy = allIdx;

        NOWA::GraphicsModule::RenderCommand cmd = [this, vertsCopy, idxsCopy, numVerts, mn]()
        {
            if (nullptr == this->previewNode)
            {
                this->previewNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();
                this->previewNode->setVisible(false);
            }

            // Destroy previous preview
            if (nullptr != this->previewItem)
            {
                if (this->previewItem->getParentSceneNode())
                {
                    this->previewItem->getParentSceneNode()->detachObject(this->previewItem);
                }
                this->gameObjectPtr->getSceneManager()->destroyItem(this->previewItem);
                this->previewItem = nullptr;
            }
            if (false == this->previewMesh.isNull())
            {
                Ogre::MeshManager::getSingleton().remove(this->previewMesh->getHandle());
                this->previewMesh.reset();
            }

            // Remove stale by name
            {
                const Ogre::String grp = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;
                Ogre::MeshPtr ex = Ogre::MeshManager::getSingleton().getByName(mn, grp);
                if (false == ex.isNull())
                {
                    Ogre::MeshManager::getSingleton().remove(ex->getHandle());
                }
            }

            Ogre::Root* root = Ogre::Root::getSingletonPtr();
            Ogre::VaoManager* vm = root->getRenderSystem()->getVaoManager();
            const Ogre::String grp = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

            this->previewMesh = Ogre::MeshManager::getSingleton().createManual(mn, grp);

            Ogre::Vector3 minBB(std::numeric_limits<float>::max());
            Ogre::Vector3 maxBB(-std::numeric_limits<float>::max());

            Ogre::SubMesh* sm = this->previewMesh->createSubMesh();
            this->uploadSubMesh(sm, vertsCopy, idxsCopy, numVerts, vm, minBB, maxBB);

            if (minBB.x > maxBB.x)
            {
                minBB = Ogre::Vector3(-1.0f);
                maxBB = Ogre::Vector3(1.0f);
            }
            Ogre::Aabb aabb;
            aabb.setExtents(minBB, maxBB);
            this->previewMesh->_setBounds(aabb, false);
            this->previewMesh->_setBoundingSphereRadius(aabb.getRadius());

            this->previewItem = this->gameObjectPtr->getSceneManager()->createItem(this->previewMesh, Ogre::SCENE_DYNAMIC);

            Ogre::HlmsDatablock* previewDb = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault("BaseWhiteNoLighting");
            if (nullptr != previewDb)
            {
                this->previewItem->getSubItem(0)->setDatablock(previewDb);
            }

            this->previewItem->setQueryFlags(0u);
            this->previewItem->setCastShadows(false);
            this->previewNode->attachObject(this->previewItem);
            this->previewNode->setVisible(true);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "ProceduralStairsComponent::createPreviewMesh");
    }

    void ProceduralStairsComponent::destroyPreviewMesh(void)
    {
        if (nullptr == this->previewItem && this->previewMesh.isNull())
        {
            return;
        }
        NOWA::GraphicsModule::RenderCommand cmd = [this]()
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
            if (false == this->previewMesh.isNull())
            {
                Ogre::MeshManager::getSingleton().remove(this->previewMesh->getHandle());
                this->previewMesh.reset();
            }
            if (nullptr != this->previewNode)
            {
                this->previewNode->setVisible(false);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "ProceduralStairsComponent::destroyPreviewMesh");
    }

    // =========================================================================
    //  Convert to mesh  (same pattern as ProceduralWallComponent)
    // =========================================================================

    bool ProceduralStairsComponent::convertToMeshApply(void)
    {
        if (this->stairsMesh.isNull() || nullptr == this->stairsItem)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralStairsComponent] convertToMeshApply: no mesh to export!");
            return false;
        }

        const Ogre::String meshName = "Stairs_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()) + ".mesh";

        auto filePathNames = Core::getSingletonPtr()->getSectionPath("Procedural");
        if (filePathNames.empty())
        {
            return false;
        }
        const Ogre::String fullPath = filePathNames[0] + "/" + meshName;

        if (!this->exportMesh(fullPath))
        {
            return false;
        }

        const Ogre::String capturedMeshName = meshName;
        GameObjectPtr capturedGOPtr = this->gameObjectPtr;
        const unsigned int capturedCompIndex = this->getIndex();
        const Ogre::String capturedTreadDb = this->treadDatablock->getString();
        const Ogre::String capturedRiserDb = this->riserDatablock->getString();
        const Ogre::String capturedStringerDb = this->stringerDatablock->getString();
        const Ogre::Vector3 currentPosition = this->gameObjectPtr->getPosition();
        const Ogre::Quaternion currentOrient = this->gameObjectPtr->getOrientation();
        const Ogre::Vector3 currentScale = this->gameObjectPtr->getScale();

        NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.5f));

        auto conversionFunction = [this, capturedMeshName, capturedGOPtr, capturedCompIndex, capturedTreadDb, capturedRiserDb, capturedStringerDb, currentPosition, currentOrient, currentScale]()
        {
            Ogre::MeshPtr loadedMesh;
            try
            {
                loadedMesh = Ogre::MeshManager::getSingleton().load(capturedMeshName, "Procedural");
            }
            catch (Ogre::Exception& e)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralStairsComponent] Failed to load exported mesh: " + e.getFullDescription());
                return;
            }

            if (loadedMesh.isNull())
            {
                return;
            }

            Ogre::Item* newItem = nullptr;

            NOWA::GraphicsModule::RenderCommand renderCmd = [capturedGOPtr, loadedMesh, capturedTreadDb, capturedRiserDb, capturedStringerDb, &newItem]()
            {
                newItem = capturedGOPtr->getSceneManager()->createItem(loadedMesh, capturedGOPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC);

                auto applyDb = [&](unsigned int idx, const Ogre::String& dbName)
                {
                    if (idx < newItem->getNumSubItems() && !dbName.empty())
                    {
                        Ogre::HlmsDatablock* db = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(dbName);
                        if (nullptr != db)
                        {
                            newItem->getSubItem(idx)->setDatablock(db);
                        }
                    }
                };
                applyDb(0, capturedTreadDb);
                applyDb(1, capturedRiserDb);
                applyDb(2, capturedStringerDb);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCmd), "ProceduralStairsComponent::convertToMesh_createItem");

            if (nullptr == newItem)
            {
                return;
            }

            this->destroyPreviewMesh();
            this->destroyStairsMesh();

            if (!capturedGOPtr->assignMesh(newItem))
            {
                return;
            }

            capturedGOPtr->getSceneNode()->setPosition(currentPosition);
            capturedGOPtr->getSceneNode()->setOrientation(currentOrient);
            capturedGOPtr->getSceneNode()->setScale(currentScale);

            boost::shared_ptr<EventDataDeleteComponent> evtDel(new EventDataDeleteComponent(capturedGOPtr->getId(), "ProceduralStairsComponent", capturedCompIndex));
            NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evtDel);

            capturedGOPtr->deleteComponentByIndex(capturedCompIndex);

            boost::shared_ptr<EventDataRefreshGui> evtGui(new EventDataRefreshGui());
            NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evtGui);

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralStairsComponent] CONVERSION COMPLETE: " + capturedMeshName + " — SAVE YOUR SCENE to persist this change.");
        };

        NOWA::ProcessPtr closureProcess(new NOWA::ClosureProcess(conversionFunction));
        delayProcess->attachChild(closureProcess);
        NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);

        boost::shared_ptr<EventDataRefreshGui> evtGui(new EventDataRefreshGui());
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evtGui);

        return true;
    }

    bool ProceduralStairsComponent::exportMesh(const Ogre::String& filename)
    {
        if (this->stairsMesh.isNull())
        {
            return false;
        }
        try
        {
            Ogre::MeshSerializer serializer(Ogre::Root::getSingletonPtr()->getRenderSystem()->getVaoManager());
            serializer.exportMesh(this->stairsMesh.get(), filename);
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralStairsComponent] Exported mesh to: " + filename);
            return true;
        }
        catch (Ogre::Exception& e)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralStairsComponent] exportMesh failed: " + e.getDescription());
            return false;
        }
    }

    // =========================================================================
    //  getConvexParts  –  GeometricComponentBase physics integration
    // =========================================================================

    std::vector<GeometricComponentBase::ConvexPart> ProceduralStairsComponent::getConvexParts(void) const
    {
        std::vector<GeometricComponentBase::ConvexPart> parts;

        const int n = std::max(1, this->stepCount->getInt());
        const float sh = this->stepHeight->getReal();
        const float sd = this->stepDepth->getReal();

        Ogre::Real pivotOffsetX = 0.0f;
        Ogre::Real pivotOffsetZ = 0.0f;
        this->computePivotOffset(pivotOffsetX, pivotOffsetZ);

        switch (this->getStairShapeEnum())
        {
        // ── Linear: one Box primitive per step ──────────────────────────────
        case StairShape::LINEAR:
        {
            const float sw = this->stepWidth->getReal();
            for (int i = 0; i < n; ++i)
            {
                GeometricComponentBase::ConvexPart p;
                p.type = "Box";
                p.size = Ogre::Vector3(sw, sh, sd);

                // Centre of this step's box
                p.position = Ogre::Vector3(-pivotOffsetX, i * sh + sh * 0.5f, i * sd + sd * 0.5f - pivotOffsetZ);

                parts.push_back(p);
            }
            break;
        }
        // ── Curved / Spiral: one ConvexHull per step (wedge shape) ──────────
        case StairShape::CURVED:
        {
            const float rInner = this->innerRadius->getReal();
            const float rOuter = this->outerRadius->getReal();
            const float totalArc = this->arcAngle->getReal();
            const float stepArc = totalArc / n;
            const bool cw = (this->rotationDir->getListSelectedValue() == "Clockwise");
            const float dirMul = cw ? -1.0f : 1.0f;

            for (int i = 0; i < n; ++i)
            {
                const float y0 = i * sh;
                const float y1 = y0 + sh;
                const float aStart = Ogre::Math::DegreesToRadians(i * stepArc) * dirMul;
                const float aEnd = Ogre::Math::DegreesToRadians((i + 1) * stepArc) * dirMul;
                const float csS = std::cos(aStart), snS = std::sin(aStart);
                const float csE = std::cos(aEnd), snE = std::sin(aEnd);

                GeometricComponentBase::ConvexPart p;
                p.type = "ConvexHull";
                p.vertices = {Ogre::Vector3(rInner * csS, y0, rInner * snS), Ogre::Vector3(rOuter * csS, y0, rOuter * snS), Ogre::Vector3(rInner * csE, y0, rInner * snE), Ogre::Vector3(rOuter * csE, y0, rOuter * snE),
                    Ogre::Vector3(rInner * csS, y1, rInner * snS), Ogre::Vector3(rOuter * csS, y1, rOuter * snS), Ogre::Vector3(rInner * csE, y1, rInner * snE), Ogre::Vector3(rOuter * csE, y1, rOuter * snE)};
                parts.push_back(p);
            }
            break;
        }
        }

        return parts;
    }

    // =========================================================================
    //  Attribute setters / getters
    // =========================================================================

    void ProceduralStairsComponent::setActivated(bool activated)
    {
        this->activated->setValue(activated);
    }

    bool ProceduralStairsComponent::isActivated(void) const
    {
        return this->activated->getBool();
    }

    void ProceduralStairsComponent::setStairShape(const Ogre::String& shape)
    {
        this->stairShape->setListSelectedValue(shape);
        this->rebuildMesh();
    }
    Ogre::String ProceduralStairsComponent::getStairShape(void) const
    {
        return this->stairShape->getListSelectedValue();
    }

    void ProceduralStairsComponent::setStepCount(int count)
    {
        this->stepCount->setValue(std::max(1, count));
        this->rebuildMesh();
    }
    int ProceduralStairsComponent::getStepCount(void) const
    {
        return this->stepCount->getInt();
    }

    void ProceduralStairsComponent::setStepHeight(Ogre::Real height)
    {
        this->stepHeight->setValue(std::max(0.01f, static_cast<float>(height)));
        this->rebuildMesh();
    }
    Ogre::Real ProceduralStairsComponent::getStepHeight(void) const
    {
        return this->stepHeight->getReal();
    }

    void ProceduralStairsComponent::setStepDepth(Ogre::Real depth)
    {
        this->stepDepth->setValue(std::max(0.01f, static_cast<float>(depth)));
        this->rebuildMesh();
    }
    Ogre::Real ProceduralStairsComponent::getStepDepth(void) const
    {
        return this->stepDepth->getReal();
    }

    void ProceduralStairsComponent::setStepWidth(Ogre::Real width)
    {
        this->stepWidth->setValue(std::max(0.01f, static_cast<float>(width)));
        this->rebuildMesh();
    }
    Ogre::Real ProceduralStairsComponent::getStepWidth(void) const
    {
        return this->stepWidth->getReal();
    }

    void ProceduralStairsComponent::setStepNosing(Ogre::Real nosing)
    {
        this->stepNosing->setValue(std::max(0.0f, static_cast<float>(nosing)));
        this->rebuildMesh();
    }
    Ogre::Real ProceduralStairsComponent::getStepNosing(void) const
    {
        return this->stepNosing->getReal();
    }

    void ProceduralStairsComponent::setOpenRiser(bool open)
    {
        this->openRiser->setValue(open);
        this->rebuildMesh();
    }
    bool ProceduralStairsComponent::getOpenRiser(void) const
    {
        return this->openRiser->getBool();
    }

    void ProceduralStairsComponent::setStringerStyle(const Ogre::String& style)
    {
        this->stringerStyle->setListSelectedValue(style);
        this->rebuildMesh();
    }
    Ogre::String ProceduralStairsComponent::getStringerStyle(void) const
    {
        return this->stringerStyle->getListSelectedValue();
    }

    void ProceduralStairsComponent::setBottomStyle(const Ogre::String& style)
    {
        this->bottomStyle->setListSelectedValue(style);
        this->rebuildMesh();
    }
    Ogre::String ProceduralStairsComponent::getBottomStyle(void) const
    {
        return this->bottomStyle->getListSelectedValue();
    }

    void ProceduralStairsComponent::setInnerRadius(Ogre::Real radius)
    {
        this->innerRadius->setValue(std::max(0.01f, static_cast<float>(radius)));
        this->rebuildMesh();
    }
    Ogre::Real ProceduralStairsComponent::getInnerRadius(void) const
    {
        return this->innerRadius->getReal();
    }

    void ProceduralStairsComponent::setOuterRadius(Ogre::Real radius)
    {
        this->outerRadius->setValue(std::max(0.01f, static_cast<float>(radius)));
        this->rebuildMesh();
    }
    Ogre::Real ProceduralStairsComponent::getOuterRadius(void) const
    {
        return this->outerRadius->getReal();
    }

    void ProceduralStairsComponent::setArcAngle(Ogre::Real degrees)
    {
        this->arcAngle->setValue(std::max(1.0f, static_cast<float>(degrees)));
        this->rebuildMesh();
    }
    Ogre::Real ProceduralStairsComponent::getArcAngle(void) const
    {
        return this->arcAngle->getReal();
    }

    void ProceduralStairsComponent::setRotationDir(const Ogre::String& dir)
    {
        this->rotationDir->setListSelectedValue(dir);
        this->rebuildMesh();
    }
    Ogre::String ProceduralStairsComponent::getRotationDir(void) const
    {
        return this->rotationDir->getListSelectedValue();
    }

    void ProceduralStairsComponent::setCentrePole(bool enable)
    {
        this->centrePole->setValue(enable);
        this->rebuildMesh();
    }
    bool ProceduralStairsComponent::getCentrePole(void) const
    {
        return this->centrePole->getBool();
    }

    void ProceduralStairsComponent::setPivotPosition(const Ogre::String& pivot)
    {
        this->pivotPosition->setListSelectedValue(pivot);
        this->rebuildMesh();
    }
    Ogre::String ProceduralStairsComponent::getPivotPosition(void) const
    {
        return this->pivotPosition->getListSelectedValue();
    }

    void ProceduralStairsComponent::setRampCollider(bool enabled)
    {
        this->rampCollider->setValue(enabled);
        this->rebuildMesh();
    }

    bool ProceduralStairsComponent::getRampCollider(void) const
    {
        return this->rampCollider->getBool();
    }

    void ProceduralStairsComponent::setUVMode(const Ogre::String& mode)
    {
        this->uvMode->setListSelectedValue(mode);
        this->rebuildMesh();
    }
    Ogre::String ProceduralStairsComponent::getUVMode(void) const
    {
        return this->uvMode->getListSelectedValue();
    }

    void ProceduralStairsComponent::setUVTiling(const Ogre::Vector2& tiling)
    {
        this->uvTiling->setValue(tiling);
        this->rebuildMesh();
    }
    Ogre::Vector2 ProceduralStairsComponent::getUVTiling(void) const
    {
        return this->uvTiling->getVector2();
    }

    void ProceduralStairsComponent::setTreadDatablock(const Ogre::String& name)
    {
        this->treadDatablock->setValue(name);
        if (nullptr != this->stairsItem)
        {
            NOWA::GraphicsModule::RenderCommand cmd = [this, name]()
            {
                if (nullptr == this->stairsItem)
                {
                    return;
                }
                Ogre::HlmsDatablock* db = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(name);
                if (nullptr != db && this->stairsItem->getNumSubItems() > 0)
                {
                    this->stairsItem->getSubItem(0)->setDatablock(db);
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "ProceduralStairsComponent::setTreadDatablock");
        }
    }
    Ogre::String ProceduralStairsComponent::getTreadDatablock(void) const
    {
        return this->treadDatablock->getString();
    }

    void ProceduralStairsComponent::setRiserDatablock(const Ogre::String& name)
    {
        this->riserDatablock->setValue(name);
        if (nullptr != this->stairsItem)
        {
            NOWA::GraphicsModule::RenderCommand cmd = [this, name]()
            {
                if (nullptr == this->stairsItem)
                {
                    return;
                }
                Ogre::HlmsDatablock* db = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(name);
                if (nullptr != db && this->stairsItem->getNumSubItems() > 1)
                {
                    this->stairsItem->getSubItem(1)->setDatablock(db);
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "ProceduralStairsComponent::setRiserDatablock");
        }
    }
    Ogre::String ProceduralStairsComponent::getRiserDatablock(void) const
    {
        return this->riserDatablock->getString();
    }

    void ProceduralStairsComponent::setStringerDatablock(const Ogre::String& name)
    {
        this->stringerDatablock->setValue(name);
        if (nullptr != this->stairsItem)
        {
            NOWA::GraphicsModule::RenderCommand cmd = [this, name]()
            {
                if (nullptr == this->stairsItem)
                {
                    return;
                }
                Ogre::HlmsDatablock* db = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(name);
                if (nullptr != db && this->stairsItem->getNumSubItems() > 2)
                {
                    this->stairsItem->getSubItem(2)->setDatablock(db);
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "ProceduralStairsComponent::setStringerDatablock");
        }
    }
    Ogre::String ProceduralStairsComponent::getStringerDatablock(void) const
    {
        return this->stringerDatablock->getString();
    }

    // =========================================================================
    //  Lua API
    // =========================================================================

    ProceduralStairsComponent* getProceduralStairsComponent(GameObject* go)
    {
        return NOWA::makeStrongPtr(go->getComponent<ProceduralStairsComponent>()).get();
    }

    ProceduralStairsComponent* getProceduralStairsComponentFromName(GameObject* go, const Ogre::String& name)
    {
        return NOWA::makeStrongPtr(go->getComponentFromName<ProceduralStairsComponent>(name)).get();
    }

    void ProceduralStairsComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
    {
        luabind::module(lua)
        [
            luabind::class_<ProceduralStairsComponent, GameObjectComponent>("ProceduralStairsComponent")
            .def("setActivated", &ProceduralStairsComponent::setActivated)
            .def("isActivated", &ProceduralStairsComponent::isActivated)
            .def("setStairShape", &ProceduralStairsComponent::setStairShape)
            .def("getStairShape", &ProceduralStairsComponent::getStairShape)
            .def("setStepCount", &ProceduralStairsComponent::setStepCount)
            .def("getStepCount", &ProceduralStairsComponent::getStepCount)
            .def("setStepHeight", &ProceduralStairsComponent::setStepHeight)
            .def("getStepHeight", &ProceduralStairsComponent::getStepHeight)
            .def("setStepDepth", &ProceduralStairsComponent::setStepDepth)
            .def("getStepDepth", &ProceduralStairsComponent::getStepDepth)
            .def("setStepWidth", &ProceduralStairsComponent::setStepWidth)
            .def("getStepWidth", &ProceduralStairsComponent::getStepWidth)
            .def("setStepNosing", &ProceduralStairsComponent::setStepNosing)
            .def("getStepNosing", &ProceduralStairsComponent::getStepNosing)
            .def("setOpenRiser", &ProceduralStairsComponent::setOpenRiser)
            .def("getOpenRiser", &ProceduralStairsComponent::getOpenRiser)
            .def("setStringerStyle", &ProceduralStairsComponent::setStringerStyle)
            .def("getStringerStyle", &ProceduralStairsComponent::getStringerStyle)
            .def("setBottomStyle", &ProceduralStairsComponent::setBottomStyle)
            .def("getBottomStyle", &ProceduralStairsComponent::getBottomStyle)
            .def("setInnerRadius", &ProceduralStairsComponent::setInnerRadius)
            .def("getInnerRadius", &ProceduralStairsComponent::getInnerRadius)
            .def("setOuterRadius", &ProceduralStairsComponent::setOuterRadius)
            .def("getOuterRadius", &ProceduralStairsComponent::getOuterRadius)
            .def("setArcAngle", &ProceduralStairsComponent::setArcAngle)
            .def("getArcAngle", &ProceduralStairsComponent::getArcAngle)
            .def("setRotationDir", &ProceduralStairsComponent::setRotationDir)
            .def("getRotationDir", &ProceduralStairsComponent::getRotationDir)
            .def("setCentrePole", &ProceduralStairsComponent::setCentrePole)
            .def("getCentrePole", &ProceduralStairsComponent::getCentrePole)
            .def("setPivotPosition", &ProceduralStairsComponent::setPivotPosition)
            .def("getPivotPosition", &ProceduralStairsComponent::getPivotPosition)
            .def("setRampCollider", &ProceduralStairsComponent::setRampCollider)
            .def("getRampCollider", &ProceduralStairsComponent::getRampCollider)
            .def("setUVMode", &ProceduralStairsComponent::setUVMode)
            .def("getUVMode", &ProceduralStairsComponent::getUVMode)
            .def("setUVTiling", &ProceduralStairsComponent::setUVTiling)
            .def("getUVTiling", &ProceduralStairsComponent::getUVTiling)
            .def("setTreadDatablock", &ProceduralStairsComponent::setTreadDatablock)
            .def("getTreadDatablock", &ProceduralStairsComponent::getTreadDatablock)
            .def("setRiserDatablock", &ProceduralStairsComponent::setRiserDatablock)
            .def("getRiserDatablock", &ProceduralStairsComponent::getRiserDatablock)
            .def("setStringerDatablock", &ProceduralStairsComponent::setStringerDatablock)
            .def("getStringerDatablock", &ProceduralStairsComponent::getStringerDatablock)
            .def("rebuildMesh", &ProceduralStairsComponent::rebuildMesh)
        ];

        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "class inherits GameObjectComponent", ProceduralStairsComponent::getStaticInfoText());

        // ── Activation ────────────────────────────────────────────────────────────
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "void setActivated(bool activated)",
            "Activates or deactivates the stair placement mode. "
            "When false, mouse input for placement is ignored.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "bool isActivated()", "Returns true if the stair placement mode is active.");

        // ── Shape ─────────────────────────────────────────────────────────────────
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "void setStairShape(string shape)",
            "Sets the stair shape and rebuilds the mesh. "
            "Valid values: 'Linear', 'Curved'. "
            "Curved supports Arc Angle > 360 degrees for full spirals.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "string getStairShape()", "Returns the currently selected stair shape name: 'Linear' or 'Curved'.");

        // ── Step count ────────────────────────────────────────────────────────────
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "void setStepCount(int n)",
            "Sets the number of steps in the flight (minimum 1). Rebuilds the mesh. "
            "Total stair height = StepCount × StepHeight. "
            "Total stair depth  = StepCount × StepDepth (Linear only).");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "int getStepCount()", "Returns the current number of steps.");

        // ── Step height ───────────────────────────────────────────────────────────
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "void setStepHeight(float h)",
            "Sets the vertical rise per step in world units. Rebuilds the mesh. "
            "Minimum clamped to 0.01. Standard architectural value: 0.15 – 0.20 m.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "float getStepHeight()", "Returns the vertical rise per step in world units.");

        // ── Step depth ────────────────────────────────────────────────────────────
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "void setStepDepth(float d)",
            "Sets the horizontal run (tread depth) per step in world units. Rebuilds the mesh. "
            "Linear only — for Curved the arc geometry determines effective depth. "
            "Minimum clamped to 0.01. Standard architectural value: 0.25 – 0.30 m.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "float getStepDepth()", "Returns the horizontal run per step in world units.");

        // ── Step width ────────────────────────────────────────────────────────────
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "void setStepWidth(float w)",
            "Sets the lateral width of the stair flight in world units. Rebuilds the mesh. "
            "Linear only — for Curved use setInnerRadius() and setOuterRadius() instead. "
            "Minimum clamped to 0.01.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "float getStepWidth()", "Returns the lateral width of the stair flight in world units.");

        // ── Nosing ────────────────────────────────────────────────────────────────
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "void setStepNosing(float n)",
            "Sets the tread overhang beyond the riser in world units. Rebuilds the mesh. "
            "0 = flush (no overhang). Automatically clamped to less than StepDepth. "
            "For Curved stairs the value is converted to an angular extension at the mid-radius. "
            "Standard architectural value: 0.02 – 0.04 m.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "float getStepNosing()", "Returns the tread nosing overhang in world units.");

        // ── Open riser ────────────────────────────────────────────────────────────
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "void setOpenRiser(bool open)",
            "When true, omits the front vertical riser face giving a floating/open stair look. "
            "Rebuilds the mesh. The Riser Datablock is ignored when this is true.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "bool getOpenRiser()", "Returns true if the vertical riser faces are omitted.");

        // ── Stringer style ────────────────────────────────────────────────────────
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "void setStringerStyle(string style)",
            "Sets the side panel style. Rebuilds the mesh. Valid values: "
            "'None'   – no side panels; steps appear to float with open sides. "
            "'Closed' – one solid rectangular board per side spanning the full profile. "
            "           Uses the Stringer Datablock material. "
            "'Open'   – per-step notched profile on each side, same silhouette as the "
            "           stair profile but rendered with the Stringer Datablock material. "
            "           Useful for wooden open-riser stairs with a visible stringer board.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "string getStringerStyle()", "Returns the current stringer style: 'None', 'Closed', or 'Open'.");

        // ── Bottom style ──────────────────────────────────────────────────────────
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "void setBottomStyle(string style)",
            "Sets the underside geometry style. Rebuilds the mesh. Valid values: "
            "'None'    – hollow / open underside. No geometry below the steps. "
            "'Sloped'  – one flat diagonal soffit panel under the entire Linear flight. "
            "            For Curved: a flat annular disk at y=0 closes the base. "
            "'Stepped' – the underside mirrors the step geometry (fully closed solid). "
            "            Recommended when the stair is visible from below.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "string getBottomStyle()", "Returns the current bottom style: 'None', 'Sloped', or 'Stepped'.");

        // ── Curved: inner radius ──────────────────────────────────────────────────
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "void setInnerRadius(float r)",
            "Sets the inner void radius for Curved stairs in world units. Rebuilds the mesh. "
            "This is the radius of the central column / void at the centre of the spiral. "
            "Minimum clamped to 0.01. Must be less than Outer Radius.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "float getInnerRadius()", "Returns the inner void radius for Curved stairs in world units.");

        // ── Curved: outer radius ──────────────────────────────────────────────────
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "void setOuterRadius(float r)",
            "Sets the outer edge radius for Curved stairs in world units. Rebuilds the mesh. "
            "The difference (OuterRadius - InnerRadius) is the effective step width. "
            "Automatically clamped to be at least 0.01 greater than Inner Radius.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "float getOuterRadius()", "Returns the outer edge radius for Curved stairs in world units.");

        // ── Curved: arc angle ─────────────────────────────────────────────────────
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "void setArcAngle(float deg)",
            "Sets the total rotation of the stair flight in degrees. Rebuilds the mesh. "
            "90  = quarter turn.  180 = half turn.  270 = three-quarter turn. "
            "360 = full spiral.   Any value > 360 creates a multi-revolution spiral. "
            "Minimum clamped to 1 degree.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "float getArcAngle()", "Returns the total arc angle of the stair flight in degrees.");

        // ── Curved: rotation direction ────────────────────────────────────────────
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "void setRotationDir(string dir)",
            "Sets the direction the stair turns when ascending. Rebuilds the mesh. "
            "Valid values: 'Counter-Clockwise', 'Clockwise'. "
            "Curved only — has no effect on Linear stairs.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "string getRotationDir()", "Returns the rotation direction: 'Counter-Clockwise' or 'Clockwise'.");

        // ── Curved: centre pole ───────────────────────────────────────────────────
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "void setCentrePole(bool enabled)",
            "When true, generates a solid cylinder at the inner radius as a structural pole. "
            "Curved only — has no effect on Linear stairs. Rebuilds the mesh. "
            "The pole uses the Stringer Datablock material.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "bool getCentrePole()", "Returns true if the centre pole cylinder is enabled.");

        // ── Pivot position ────────────────────────────────────────────────────────
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "void setPivotPosition(string pivot)",
            "Sets where the scene node origin sits relative to the stair geometry. "
            "Rebuilds the mesh. Valid values: "
            "'Bottom-Front'  – origin at the base of the first step (default). "
            "                  Place the GO at ground level at the foot of the stair. "
            "'Bottom-Centre' – origin at the horizontal midpoint of the flight. "
            "                  Useful for symmetric placement around a centre point. "
            "'Bottom-Back'   – origin at the base of the top step. "
            "                  Place the GO where the stair meets the upper floor.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "string getPivotPosition()", "Returns the pivot position: 'Bottom-Front', 'Bottom-Centre', or 'Bottom-Back'.");

        // ── Ramp collider ─────────────────────────────────────────────────────────
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "void setRampCollider(bool enabled)",
            "When true, generates an invisible diagonal ramp face over the stair flight. "
            "This lets physics-driven characters walk up stairs smoothly without "
            "stairstep jitter — the ramp face is fully transparent (PBS opacity = 0). "
            "Works for both Linear (single diagonal quad) and Curved (helical sectors). "
            "The auto-created datablock is named 'StairsRamp_<gameObjectId>'. "
            "Combine with a PhysicsArtifactComponent for collision detection. Rebuilds the mesh.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "bool getRampCollider()", "Returns true if the invisible diagonal ramp collider submesh is active.");

        // ── UV mode ───────────────────────────────────────────────────────────────
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "void setUVMode(string mode)",
            "Sets the UV projection mode applied to all faces. Rebuilds the mesh. "
            "Valid values: "
            "'Box'        – each face projected from its dominant axis (best for untiled textures). "
            "'Continuous' – UVs flow along the stair flight direction (best for tiling textures).");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "string getUVMode()", "Returns the current UV mode: 'Box' or 'Continuous'.");

        // ── UV tiling ─────────────────────────────────────────────────────────────
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "void setUVTiling(Vector2 tiling)",
            "Sets the UV tiling multiplier applied to all faces. Rebuilds the mesh. "
            "x = U repeat, y = V repeat. Default is (1, 1) = no tiling. "
            "Increase to tile a texture across multiple steps, e.g. (4, 2).");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "Vector2 getUVTiling()", "Returns the current UV tiling multiplier as a Vector2.");

        // ── Tread datablock ───────────────────────────────────────────────────────
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "void setTreadDatablock(string name)",
            "Assigns a PBS datablock by name to the top (horizontal tread) faces. "
            "This is a live swap — does NOT trigger a full mesh rebuild. "
            "The datablock must already be registered with the HlmsManager.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "string getTreadDatablock()", "Returns the PBS datablock name currently applied to the tread faces.");

        // ── Riser datablock ───────────────────────────────────────────────────────
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "void setRiserDatablock(string name)",
            "Assigns a PBS datablock by name to the front (vertical riser) faces. "
            "This is a live swap — does NOT trigger a full mesh rebuild. "
            "Ignored when Open Riser = true (no riser faces are generated).");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "string getRiserDatablock()", "Returns the PBS datablock name currently applied to the riser faces.");

        // ── Stringer datablock ────────────────────────────────────────────────────
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "void setStringerDatablock(string name)",
            "Assigns a PBS datablock by name to the stringer side panels, soffit, and centre pole. "
            "This is a live swap — does NOT trigger a full mesh rebuild. "
            "Ignored when Stringer Style = None and Bottom Style = None and Centre Pole = false.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "string getStringerDatablock()", "Returns the PBS datablock name currently applied to stringer/soffit/pole faces.");

        // ── Rebuild ───────────────────────────────────────────────────────────────
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralStairsComponent", "void rebuildMesh()",
            "Forces a complete geometry rebuild from all current parameter values. "
            "Call this once after making several parameter changes via Lua to avoid "
            "triggering a redundant rebuild after every individual setter call. "
            "Example: "
            "  local s = go:getProceduralStairsComponent() "
            "  s:setStepCount(16)    -- no rebuild yet if called internally "
            "  s:setStepHeight(0.18) "
            "  s:setStepDepth(0.28) "
            "  s:rebuildMesh()        -- single rebuild covering all changes");

        // ── GameObject / GameObjectController registration ─────────────────────────
        gameObjectClass.def("getProceduralStairsComponent", (ProceduralStairsComponent * (*)(GameObject*)) & getProceduralStairsComponent);
        gameObjectClass.def("getProceduralStairsComponentFromName", &getProceduralStairsComponentFromName);
        gameObjectControllerClass.def("castProceduralStairsComponent", &GameObjectController::cast<ProceduralStairsComponent>);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ProceduralStairsComponent getProceduralStairsComponent()",
            "Gets the first ProceduralStairsComponent attached to this GameObject. "
            "Returns nil if no such component exists.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ProceduralStairsComponent getProceduralStairsComponentFromName(string name)",
            "Gets a named ProceduralStairsComponent from this GameObject. "
            "Use this when the GameObject has more than one stair component.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "ProceduralStairsComponent castProceduralStairsComponent(ProceduralStairsComponent other)",
            "Casts a GameObjectComponent to ProceduralStairsComponent for Lua auto-completion support.");
    }

} // namespace NOWA

/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#include "NOWAPrecompiled.h"
#include "ProceduralConveyorLoopComponent.h"
#include "gameobject/GameObjectController.h"
#include "gameobject/GameObjectFactory.h"
#include "gameobject/PhysicsArtifactComponent.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
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
// ProceduralConveyorLoopComponent
//
// See the class comment in the header for the full geometry/material/animation reasoning. Two
// points worth repeating here because they carry over lessons from this project's own history:
//
//   - Every vertex carries a tangent from the start (ProceduralThornComponent's first draft did
//     not, and failed to apply a normal-mapped datablock at all).
//   - Face winding is derived from the intended outward normal via the same self-correcting
//     cross-product check ProceduralBlockComponent::addBlockQuad uses, rather than worked out by
//     hand per face (ProceduralThornComponent's cones needed several rounds of fixing precisely
//     because their winding was chosen manually).
//
// BUGFIX (design correction, this component's second draft): the first draft used ONE combined
// datablock and vertex buffer for both the scrolling belt surface and the two static end caps,
// and scaled the belt's U coordinate by a free "meters per repeat" tiling value. Both were
// wrong: a single datablock could not give the belt surface and the end caps different
// materials, and a free tiling value almost never divides the loop's own total perimeter into a
// whole number of repeats - so the instant scrollOffset wrapped back to 0 after one full lap,
// every vertex's U jumped by a FRACTIONAL repeat instead of a whole one, which is exactly the
// visible pop reported in testing. Fixed by splitting into two submeshes/buffers (see the class
// comment) and by deriving the belt's U scale from an INTEGER "Belt Repeat Count" divided by the
// loop's own total perimeter, which makes one full lap of scrollOffset ALWAYS exactly
// BeltRepeatCount whole repeats, by construction - wrapping it can then never produce anything
// but an invisible, whole-repeat jump.
// =============================================================================

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    namespace
    {
        // Subdivisions per semicircular end. 16 reads as properly round at the modest radii a
        // conveyor belt actually uses, without generating an excessive vertex count for a shape
        // there will only ever be one or two of in a level.
        const int CONVEYOR_ARC_SEGMENTS = 16;
    }

    ProceduralConveyorLoopComponent::ProceduralConveyorLoopComponent() :
        GameObjectComponent(),
        name("ProceduralConveyorLoopComponent"),
        activated(new Variant(ProceduralConveyorLoopComponent::AttrActivated(), true, this->attributes)),
        beltLength(new Variant(ProceduralConveyorLoopComponent::AttrBeltLength(), 4.0f, this->attributes)),
        rollerRadius(new Variant(ProceduralConveyorLoopComponent::AttrRollerRadius(), 0.5f, this->attributes)),
        depth(new Variant(ProceduralConveyorLoopComponent::AttrDepth(), 1.0f, this->attributes)),
        beltSpeed(new Variant(ProceduralConveyorLoopComponent::AttrBeltSpeed(), 2.0f, this->attributes)),
        beltRepeatCount(new Variant(ProceduralConveyorLoopComponent::AttrBeltRepeatCount(), 8, this->attributes)),
        depthUVTiling(new Variant(ProceduralConveyorLoopComponent::AttrDepthUVTiling(), Ogre::Real(1.0f), this->attributes)),
        beltDatablock(new Variant(ProceduralConveyorLoopComponent::AttrBeltDatablock(), Ogre::String("Brick1_4"), this->attributes)),
        endCapDatablock(new Variant(ProceduralConveyorLoopComponent::AttrEndCapDatablock(), Ogre::String("M_Brick"), this->attributes)), // looping_part2
        currentBeltVertexIndex(0),
        currentEndCapVertexIndex(0),
        totalPerimeter(0.0f),
        scrollOffset(0.0f),
        conveyorItem(nullptr),
        dynamicVertexBuffer(nullptr),
        physicsArtifactComponent(nullptr)
    {
        this->activated->setDescription("Activates the belt. When deactivated the mesh is removed.");

        this->beltLength->setDescription("Total length of the belt in meters, along the direction of travel.");
        this->beltLength->setConstraints(0.2f, 1000.0f);

        this->rollerRadius->setDescription("Radius of the two end rollers in meters - also sets the belt's overall "
                                           "height (2 x radius) and how rounded its ends are.");
        this->rollerRadius->setConstraints(0.05f, 100.0f);

        this->depth->setDescription("Depth of the belt in meters - spans -Depth/2 .. +Depth/2 in local Z.");
        this->depth->setConstraints(0.05f, 1000.0f);

        this->beltSpeed->setDescription("How fast the texture scrolls around the belt's loop, in meters per second. "
                                        "Negative reverses the direction. This only animates the texture - pair with "
                                        "a PhysicsMaterialComponent (Contact Behavior 'ConveyorPlayer'/'ConveyorObject') "
                                        "for objects to actually be pushed along.");

        this->beltRepeatCount->setDescription("How many times the belt's own texture repeats around one full lap of "
                                              "the loop. Kept as a whole number on purpose - a fractional repeat "
                                              "count would make the scroll visibly pop once it wraps around, since "
                                              "wrapping would then no longer land on an exact texture-repeat "
                                              "boundary.");
        this->beltRepeatCount->setConstraints(1, 1000);

        this->depthUVTiling->setDescription("Texture tiling for the belt across its depth (the non-looping axis), in "
                                            "meters per repeat.");

        this->beltDatablock->setDescription("The datablock covering the belt's own scrolling surface.");
        this->endCapDatablock->setDescription("The datablock covering the two flat, non-scrolling faces at each end.");

        this->conveyorMeshName = "";
    }

    ProceduralConveyorLoopComponent::~ProceduralConveyorLoopComponent()
    {
    }

    const Ogre::String& ProceduralConveyorLoopComponent::getName() const
    {
        return this->name;
    }

    void ProceduralConveyorLoopComponent::install(const Ogre::NameValuePairList* options)
    {
        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<ProceduralConveyorLoopComponent>(ProceduralConveyorLoopComponent::getStaticClassId(), ProceduralConveyorLoopComponent::getStaticClassName());
    }

    void ProceduralConveyorLoopComponent::shutdown()
    {
    }

    void ProceduralConveyorLoopComponent::uninstall()
    {
    }

    void ProceduralConveyorLoopComponent::initialise()
    {
    }

    void ProceduralConveyorLoopComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    bool ProceduralConveyorLoopComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralConveyorLoopComponent::AttrActivated())
        {
            this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralConveyorLoopComponent::AttrBeltLength())
        {
            this->beltLength->setValue(Ogre::Math::Clamp(XMLConverter::getAttribReal(propertyElement, "data", 4.0f), 0.2f, 1000.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralConveyorLoopComponent::AttrRollerRadius())
        {
            this->rollerRadius->setValue(Ogre::Math::Clamp(XMLConverter::getAttribReal(propertyElement, "data", 0.5f), 0.05f, 100.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralConveyorLoopComponent::AttrDepth())
        {
            this->depth->setValue(Ogre::Math::Clamp(XMLConverter::getAttribReal(propertyElement, "data", 2.0f), 0.05f, 1000.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralConveyorLoopComponent::AttrBeltSpeed())
        {
            this->beltSpeed->setValue(XMLConverter::getAttribReal(propertyElement, "data", 2.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralConveyorLoopComponent::AttrBeltRepeatCount())
        {
            this->beltRepeatCount->setValue(std::max(1, XMLConverter::getAttribInt(propertyElement, "data", 8)));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralConveyorLoopComponent::AttrDepthUVTiling())
        {
            this->depthUVTiling->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralConveyorLoopComponent::AttrBeltDatablock())
        {
            this->beltDatablock->setValue(XMLConverter::getAttrib(propertyElement, "data", "rockClif_D"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralConveyorLoopComponent::AttrEndCapDatablock())
        {
            this->endCapDatablock->setValue(XMLConverter::getAttrib(propertyElement, "data", "rockClif_D"));
            propertyElement = propertyElement->next_sibling("property");
        }

        return true;
    }

    GameObjectCompPtr ProceduralConveyorLoopComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        ProceduralConveyorLoopCompPtr clonedCompPtr(boost::make_shared<ProceduralConveyorLoopComponent>());

        // setOwner() must come before any setter below - every one of them calls rebuildMesh()
        // internally, which reaches through this->gameObjectPtr for the scene manager and scene
        // node. Confirmed the hard way earlier this session on three sibling components whose
        // clone() called setters before the owner was set.
        clonedCompPtr->setOwner(clonedGameObjectPtr);

        clonedCompPtr->setBeltLength(this->beltLength->getReal());
        clonedCompPtr->setRollerRadius(this->rollerRadius->getReal());
        clonedCompPtr->setDepth(this->depth->getReal());
        clonedCompPtr->setBeltSpeed(this->beltSpeed->getReal());
        clonedCompPtr->setBeltRepeatCount(this->beltRepeatCount->getInt());
        clonedCompPtr->setDepthUVTiling(this->depthUVTiling->getReal());
        clonedCompPtr->setBeltDatablock(this->beltDatablock->getString());
        clonedCompPtr->setEndCapDatablock(this->endCapDatablock->getString());

        clonedCompPtr->setActivated(this->activated->getBool());

        clonedGameObjectPtr->addComponent(clonedCompPtr);

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));

        return clonedCompPtr;
    }

    bool ProceduralConveyorLoopComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralConveyorLoopComponent] Init conveyor loop component for game object: " + this->gameObjectPtr->getName());

        this->conveyorMeshName = "ProceduralConveyorLoopMesh_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());

        if (true == this->activated->getBool())
        {
            this->rebuildMesh();
        }

        return true;
    }

    void ProceduralConveyorLoopComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralConveyorLoopComponent] Remove conveyor loop component for game object: " + this->gameObjectPtr->getName());

        this->destroyConveyorMesh();
    }

    void ProceduralConveyorLoopComponent::update(Ogre::Real dt, bool notSimulating)
    {
        if (false == this->activated->getBool() || nullptr == this->conveyorItem)
        {
            return;
        }

        const Ogre::Real speed = this->beltSpeed->getReal();
        if (0.0f == speed || this->totalPerimeter < 0.0001f)
        {
            return;
        }

        // Advances the loop position and wraps it against the loop's own total perimeter -
        // a remainder wrap rather than a hard reset to 0, so the visible scroll never jumps
        // (given Belt Repeat Count's own integer constraint - see the class comment), and never
        // grows without bound over a long play session either.
        this->scrollOffset = std::fmod(this->scrollOffset + speed * dt, this->totalPerimeter);
        if (this->scrollOffset < 0.0f)
        {
            this->scrollOffset += this->totalPerimeter;
        }

        this->updateScrollingUVs();
    }

    void ProceduralConveyorLoopComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (ProceduralConveyorLoopComponent::AttrActivated() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
        }
        else if (ProceduralConveyorLoopComponent::AttrBeltLength() == attribute->getName())
        {
            this->setBeltLength(attribute->getReal());
        }
        else if (ProceduralConveyorLoopComponent::AttrRollerRadius() == attribute->getName())
        {
            this->setRollerRadius(attribute->getReal());
        }
        else if (ProceduralConveyorLoopComponent::AttrDepth() == attribute->getName())
        {
            this->setDepth(attribute->getReal());
        }
        else if (ProceduralConveyorLoopComponent::AttrBeltSpeed() == attribute->getName())
        {
            this->setBeltSpeed(attribute->getReal());
        }
        else if (ProceduralConveyorLoopComponent::AttrBeltRepeatCount() == attribute->getName())
        {
            this->setBeltRepeatCount(attribute->getInt());
        }
        else if (ProceduralConveyorLoopComponent::AttrDepthUVTiling() == attribute->getName())
        {
            this->setDepthUVTiling(attribute->getReal());
        }
        else if (ProceduralConveyorLoopComponent::AttrBeltDatablock() == attribute->getName())
        {
            this->setBeltDatablock(attribute->getString());
        }
        else if (ProceduralConveyorLoopComponent::AttrEndCapDatablock() == attribute->getName())
        {
            this->setEndCapDatablock(attribute->getString());
        }
    }

    void ProceduralConveyorLoopComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        // 2 = int, 6 = real, 7 = string, 12 = bool
        GameObjectComponent::writeXML(propertiesXML, doc);

        xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralConveyorLoopComponent::AttrActivated().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralConveyorLoopComponent::AttrBeltLength().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->beltLength->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralConveyorLoopComponent::AttrRollerRadius().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rollerRadius->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralConveyorLoopComponent::AttrDepth().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->depth->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralConveyorLoopComponent::AttrBeltSpeed().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->beltSpeed->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralConveyorLoopComponent::AttrBeltRepeatCount().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->beltRepeatCount->getInt())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralConveyorLoopComponent::AttrDepthUVTiling().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->depthUVTiling->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralConveyorLoopComponent::AttrBeltDatablock().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->beltDatablock->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralConveyorLoopComponent::AttrEndCapDatablock().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->endCapDatablock->getString())));
        propertiesXML->append_node(propertyXML);
    }

    // =========================================================================================
    // Mesh generation
    // =========================================================================================

    void ProceduralConveyorLoopComponent::addEndCapTriangle(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& normal, const Ogre::Vector2& uv0, const Ogre::Vector2& uv1,
        const Ogre::Vector2& uv2)
    {
        // Same self-correcting winding as addBeltQuad below - computed from the actual triangle
        // geometry and the desired outward normal, never picked by hand.
        const Ogre::Vector3 edge1 = v1 - v0;
        const Ogre::Vector3 edge2 = v2 - v0;
        const bool flip = edge1.crossProduct(edge2).dotProduct(normal) < 0.0f;

        Ogre::Vector3 tangent = edge1;
        if (tangent.squaredLength() < 0.0001f)
        {
            tangent = edge2;
        }
        tangent.normalise();

        const Ogre::Vector3 positions[3] = {v0, v1, v2};
        const Ogre::Vector2 uvs[3] = {uv0, uv1, uv2};

        for (int i = 0; i < 3; ++i)
        {
            this->endCapVertices.push_back(positions[i].x);
            this->endCapVertices.push_back(positions[i].y);
            this->endCapVertices.push_back(positions[i].z);
            this->endCapVertices.push_back(normal.x);
            this->endCapVertices.push_back(normal.y);
            this->endCapVertices.push_back(normal.z);
            this->endCapVertices.push_back(tangent.x);
            this->endCapVertices.push_back(tangent.y);
            this->endCapVertices.push_back(tangent.z);
            this->endCapVertices.push_back(1.0f); // handedness
            this->endCapVertices.push_back(uvs[i].x);
            this->endCapVertices.push_back(uvs[i].y);
        }

        if (false == flip)
        {
            this->endCapIndices.push_back(this->currentEndCapVertexIndex + 0);
            this->endCapIndices.push_back(this->currentEndCapVertexIndex + 1);
            this->endCapIndices.push_back(this->currentEndCapVertexIndex + 2);
        }
        else
        {
            this->endCapIndices.push_back(this->currentEndCapVertexIndex + 0);
            this->endCapIndices.push_back(this->currentEndCapVertexIndex + 2);
            this->endCapIndices.push_back(this->currentEndCapVertexIndex + 1);
        }

        this->currentEndCapVertexIndex += 3;
    }

    void ProceduralConveyorLoopComponent::addBeltQuad(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& v3, const Ogre::Vector3& normal, Ogre::Real arcLength0, Ogre::Real arcLength1,
        Ogre::Real v0Coord, Ogre::Real v1Coord)
    {
        // Winding derived from the normal rather than worked out by hand per face - same
        // self-correcting approach as ProceduralBlockComponent::addBlockQuad, deliberately
        // reused rather than re-derived: addLoopSideQuads emits one of these per perimeter edge
        // walking all the way around a closed loop, so getting even one winding wrong by hand
        // would be easy to miss and hard to spot visually until lit from the wrong angle.
        const Ogre::Vector3 edge1 = v1 - v0;
        const Ogre::Vector3 edge2 = v3 - v0;
        const bool flip = edge1.crossProduct(edge2).dotProduct(normal) < 0.0f;

        Ogre::Vector3 tangent = edge1;
        if (tangent.squaredLength() < 0.0001f)
        {
            tangent = edge2;
        }
        tangent.normalise();

        // arcLength0/arcLength1 arrive here as raw meters of arc-length - the U scale
        // (BeltRepeatCount / totalPerimeter) and the current scroll offset are applied right
        // here, once, at build time; updateScrollingUVs() re-does exactly this same computation
        // every frame using the cached arc-length stored below, without touching anything else
        // about the vertex. See the class comment for why this scale is derived from an INTEGER
        // repeat count rather than a free tiling value.
        const Ogre::Real uScale = (this->totalPerimeter > 0.0001f) ? static_cast<Ogre::Real>(this->beltRepeatCount->getInt()) / this->totalPerimeter : 0.0f;
        const Ogre::Real uv0Value = (arcLength0 + this->scrollOffset) * uScale;
        const Ogre::Real uv1Value = (arcLength1 + this->scrollOffset) * uScale;

        const Ogre::Vector3 positions[4] = {v0, v1, v2, v3};
        const Ogre::Real us[4] = {uv0Value, uv1Value, uv1Value, uv0Value};
        const Ogre::Real vs[4] = {v0Coord, v0Coord, v1Coord, v1Coord};
        const Ogre::Real arcLens[4] = {arcLength0, arcLength1, arcLength1, arcLength0};

        for (int i = 0; i < 4; ++i)
        {
            this->beltVertices.push_back(positions[i].x);
            this->beltVertices.push_back(positions[i].y);
            this->beltVertices.push_back(positions[i].z);
            this->beltVertices.push_back(normal.x);
            this->beltVertices.push_back(normal.y);
            this->beltVertices.push_back(normal.z);
            this->beltVertices.push_back(tangent.x);
            this->beltVertices.push_back(tangent.y);
            this->beltVertices.push_back(tangent.z);
            this->beltVertices.push_back(1.0f); // handedness
            this->beltVertices.push_back(static_cast<float>(us[i]));
            this->beltVertices.push_back(static_cast<float>(vs[i]));

            this->beltVertexArcLength.push_back(static_cast<float>(arcLens[i]));
        }

        if (false == flip)
        {
            this->beltIndices.push_back(this->currentBeltVertexIndex + 0);
            this->beltIndices.push_back(this->currentBeltVertexIndex + 1);
            this->beltIndices.push_back(this->currentBeltVertexIndex + 2);
            this->beltIndices.push_back(this->currentBeltVertexIndex + 0);
            this->beltIndices.push_back(this->currentBeltVertexIndex + 2);
            this->beltIndices.push_back(this->currentBeltVertexIndex + 3);
        }
        else
        {
            this->beltIndices.push_back(this->currentBeltVertexIndex + 0);
            this->beltIndices.push_back(this->currentBeltVertexIndex + 2);
            this->beltIndices.push_back(this->currentBeltVertexIndex + 1);
            this->beltIndices.push_back(this->currentBeltVertexIndex + 0);
            this->beltIndices.push_back(this->currentBeltVertexIndex + 3);
            this->beltIndices.push_back(this->currentBeltVertexIndex + 2);
        }

        this->currentBeltVertexIndex += 4;
    }

    void ProceduralConveyorLoopComponent::buildPerimeter(void)
    {
        this->perimeterPoints.clear();
        this->perimeterArcLengths.clear();

        const Ogre::Real length = this->beltLength->getReal();
        // A stadium needs at least 2*radius of straight length between the two roller centres -
        // clamp rather than let the two rollers overlap into an invalid shape.
        const Ogre::Real radius = std::min(this->rollerRadius->getReal(), length * 0.5f);

        Ogre::Real cumulative = 0.0f;
        Ogre::Vector2 previous;
        bool havePrevious = false;

        auto addPoint = [&](const Ogre::Vector2& p)
        {
            if (true == havePrevious)
            {
                cumulative += (p - previous).length();
            }
            this->perimeterPoints.push_back(p);
            this->perimeterArcLengths.push_back(cumulative);
            previous = p;
            havePrevious = true;
        };

        // Walking counter-clockwise, starting at the bottom of the left roller - the exact same
        // winding convention (and the same (dx,dy) -> (dy,-dx) outward-normal identity, applied
        // per edge in addLoopSideQuads) already verified against known faces in
        // ProceduralBlockComponent::addBlockChamferedBox.
        addPoint(Ogre::Vector2(radius, 0.0f));
        addPoint(Ogre::Vector2(length - radius, 0.0f));

        // Right roller: semicircle from -90 degrees to +90 degrees around its centre.
        for (int i = 1; i <= CONVEYOR_ARC_SEGMENTS; ++i)
        {
            const Ogre::Real t = static_cast<Ogre::Real>(i) / static_cast<Ogre::Real>(CONVEYOR_ARC_SEGMENTS);
            const Ogre::Radian angle(-Ogre::Math::HALF_PI + t * Ogre::Math::PI);
            addPoint(Ogre::Vector2(length - radius + radius * Ogre::Math::Cos(angle), radius + radius * Ogre::Math::Sin(angle)));
        }

        addPoint(Ogre::Vector2(radius, 2.0f * radius));

        // Left roller: semicircle continuing from +90 degrees to +270 degrees around its centre.
        for (int i = 1; i <= CONVEYOR_ARC_SEGMENTS; ++i)
        {
            const Ogre::Real t = static_cast<Ogre::Real>(i) / static_cast<Ogre::Real>(CONVEYOR_ARC_SEGMENTS);
            const Ogre::Radian angle(Ogre::Math::HALF_PI + t * Ogre::Math::PI);
            addPoint(Ogre::Vector2(radius + radius * Ogre::Math::Cos(angle), radius + radius * Ogre::Math::Sin(angle)));
        }

        // Closes back to (approximately) perimeterPoints[0] - not re-added as a duplicate point;
        // instead the closing EDGE (last point back to point 0) is what addLoopSideQuads emits
        // to actually close the loop, and its length is what completes totalPerimeter here.
        this->totalPerimeter = cumulative + (this->perimeterPoints.front() - this->perimeterPoints.back()).length();
    }

    void ProceduralConveyorLoopComponent::addLoopSideQuads(Ogre::Real halfDepth)
    {
        const size_t pointCount = this->perimeterPoints.size();
        const Ogre::Real depthTiling = this->depthUVTiling->getReal();
        const Ogre::Real depthSpan = 2.0f * halfDepth;

        for (size_t i = 0; i < pointCount; ++i)
        {
            const Ogre::Vector2& p0 = this->perimeterPoints[i];
            const Ogre::Vector2& p1 = this->perimeterPoints[(i + 1) % pointCount];

            const Ogre::Real arcLen0 = this->perimeterArcLengths[i];
            // The final edge wraps back to point 0, whose OWN cached arc-length is 0 again -
            // using totalPerimeter here instead keeps this edge's arc-length continuously
            // increasing rather than snapping backward, which is what actually makes the seam
            // invisible while the texture is scrolling.
            const Ogre::Real arcLen1 = (i + 1 < pointCount) ? this->perimeterArcLengths[i + 1] : this->totalPerimeter;

            const Ogre::Real dx = p1.x - p0.x;
            const Ogre::Real dy = p1.y - p0.y;
            const Ogre::Real edgeLength = std::sqrt(dx * dx + dy * dy);
            if (edgeLength < 0.0001f)
            {
                continue;
            }

            // Outward normal via the same CCW-polygon rotation identity already verified in
            // ProceduralBlockComponent::addBlockChamferedBox: (dx, dy) -> (dy, -dx).
            const Ogre::Vector3 normal(dy / edgeLength, -dx / edgeLength, 0.0f);

            const Ogre::Vector3 v0(p0.x, p0.y, -halfDepth);
            const Ogre::Vector3 v1(p1.x, p1.y, -halfDepth);
            const Ogre::Vector3 v2(p1.x, p1.y, halfDepth);
            const Ogre::Vector3 v3(p0.x, p0.y, halfDepth);

            this->addBeltQuad(v0, v1, v2, v3, normal, arcLen0, arcLen1, 0.0f, depthSpan * depthTiling);
        }
    }

    void ProceduralConveyorLoopComponent::addLoopEndCaps(Ogre::Real halfDepth)
    {
        const size_t pointCount = this->perimeterPoints.size();
        if (pointCount < 3)
        {
            return;
        }

        for (int side = 0; side < 2; ++side)
        {
            const Ogre::Real z = (0 == side) ? -halfDepth : halfDepth;
            const Ogre::Vector3 normal(0.0f, 0.0f, (0 == side) ? -1.0f : 1.0f);

            for (size_t i = 1; i + 1 < pointCount; ++i)
            {
                const Ogre::Vector3 v0(this->perimeterPoints[0].x, this->perimeterPoints[0].y, z);
                const Ogre::Vector3 v1(this->perimeterPoints[i].x, this->perimeterPoints[i].y, z);
                const Ogre::Vector3 v2(this->perimeterPoints[i + 1].x, this->perimeterPoints[i + 1].y, z);

                // End caps are a small fixed-shape face, not part of the belt's own visible loop
                // surface - their UV is just the raw local X/Y position, never scrolled.
                this->addEndCapTriangle(v0, v1, v2, normal, Ogre::Vector2(this->perimeterPoints[0].x, this->perimeterPoints[0].y), Ogre::Vector2(this->perimeterPoints[i].x, this->perimeterPoints[i].y),
                    Ogre::Vector2(this->perimeterPoints[i + 1].x, this->perimeterPoints[i + 1].y));
            }
        }
    }

    void ProceduralConveyorLoopComponent::rebuildMesh(void)
    {
        this->beltVertices.clear();
        this->beltIndices.clear();
        this->beltVertexArcLength.clear();
        this->currentBeltVertexIndex = 0;

        this->endCapVertices.clear();
        this->endCapIndices.clear();
        this->currentEndCapVertexIndex = 0;

        // totalPerimeter must be known BEFORE addLoopSideQuads runs, since addBeltQuad derives
        // its U scale (BeltRepeatCount / totalPerimeter) from it.
        this->buildPerimeter();

        const Ogre::Real halfDepth = this->depth->getReal() * 0.5f;

        this->addLoopSideQuads(halfDepth);
        this->addLoopEndCaps(halfDepth);

        this->createConveyorMesh();
    }

    void ProceduralConveyorLoopComponent::createConveyorMesh(void)
    {
        if (0 == this->currentBeltVertexIndex)
        {
            this->destroyConveyorMesh();
            return;
        }

        std::vector<float> beltVerticesCopy = this->beltVertices;
        std::vector<Ogre::uint32> beltIndicesCopy = this->beltIndices;
        const size_t numBeltVertices = this->currentBeltVertexIndex;

        std::vector<float> endCapVerticesCopy = this->endCapVertices;
        std::vector<Ogre::uint32> endCapIndicesCopy = this->endCapIndices;
        const size_t numEndCapVertices = this->currentEndCapVertexIndex;

        GraphicsModule::RenderCommand renderCommand = [this, beltVerticesCopy, beltIndicesCopy, numBeltVertices, endCapVerticesCopy, endCapIndicesCopy, numEndCapVertices]()
        { this->createConveyorMeshInternal(beltVerticesCopy, beltIndicesCopy, numBeltVertices, endCapVerticesCopy, endCapIndicesCopy, numEndCapVertices); };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralConveyorLoopComponent::createConveyorMesh");

        this->updatePhysicsCollision();
    }

    void ProceduralConveyorLoopComponent::createConveyorMeshInternal(const std::vector<float>& beltVerts, const std::vector<Ogre::uint32>& beltInds, size_t numBeltVerts, const std::vector<float>& endCapVerts,
        const std::vector<Ogre::uint32>& endCapInds, size_t numEndCapVerts)
    {
        //  RUNS ON RENDER THREAD!
        Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();
        Ogre::VaoManager* vaoManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getVaoManager();

        if (nullptr != this->conveyorItem)
        {
            this->gameObjectPtr->getSceneNode()->detachObject(this->conveyorItem);
            sceneManager->destroyItem(this->conveyorItem);
            this->conveyorItem = nullptr;
            this->gameObjectPtr->nullMovableObject();
        }
        // The item owned the mesh's VAOs, which owned the previous dynamicVertexBuffer - once
        // the item above is gone, that pointer is dangling and must not be reused.
        this->dynamicVertexBuffer = nullptr;

        {
            Ogre::ResourcePtr existing = Ogre::MeshManager::getSingleton().getByName(this->conveyorMeshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
            if (false == existing.isNull())
            {
                Ogre::MeshManager::getSingleton().remove(existing->getHandle());
            }
        }

        Ogre::MeshPtr mesh = Ogre::MeshManager::getSingleton().createManual(this->conveyorMeshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, &NOWA::gDummyMeshLoader);
        mesh->_setVaoManager(vaoManager);

        const size_t floatsPerVertex = 12u;

        Ogre::VertexElement2Vec elements;
        elements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
        elements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));
        elements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT4, Ogre::VES_TANGENT));
        elements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));

        Ogre::Vector3 aabbMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
        Ogre::Vector3 aabbMax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());

        // -- Belt submesh: BT_DYNAMIC_DEFAULT, re-uploaded every frame by updateScrollingUVs() --
        {
            float* vertexData = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(numBeltVerts * floatsPerVertex * sizeof(float), Ogre::MEMCATEGORY_GEOMETRY));
            memcpy(vertexData, beltVerts.data(), numBeltVerts * floatsPerVertex * sizeof(float));

            Ogre::uint32* indexData = reinterpret_cast<Ogre::uint32*>(OGRE_MALLOC_SIMD(beltInds.size() * sizeof(Ogre::uint32), Ogre::MEMCATEGORY_GEOMETRY));
            memcpy(indexData, beltInds.data(), beltInds.size() * sizeof(Ogre::uint32));

            for (size_t vi = 0u; vi < numBeltVerts; ++vi)
        {
                const Ogre::Vector3 position(beltVerts[vi * floatsPerVertex + 0], beltVerts[vi * floatsPerVertex + 1], beltVerts[vi * floatsPerVertex + 2]);
            aabbMin.makeFloor(position);
            aabbMax.makeCeil(position);
        }

            Ogre::VertexBufferPacked* vertexBuffer = nullptr;
            Ogre::IndexBufferPacked* indexBuffer = nullptr;

            try
            {
                // BT_DYNAMIC_DEFAULT, not BT_IMMUTABLE - this is the one thing that differs from
                // every sibling component's mesh creation, and it is the whole point of this
                // component: updateScrollingUVs() re-uploads into this exact buffer every frame
                // the belt is moving, which BT_IMMUTABLE would not allow. "false" for the
                // keepAsShadow parameter matches MeshModifyComponent's own dynamic vertex buffer
                // creation.
                vertexBuffer = vaoManager->createVertexBuffer(elements, numBeltVerts, Ogre::BT_DYNAMIC_DEFAULT, vertexData, false);
                indexBuffer = vaoManager->createIndexBuffer(Ogre::IndexBufferPacked::IT_32BIT, beltInds.size(), Ogre::BT_IMMUTABLE, indexData, true);
            }
            catch (const Ogre::Exception& e)
        {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralConveyorLoopComponent] Failed to create belt buffers: " + e.getDescription());
                return;
            }

            Ogre::VertexBufferPackedVec vertexBuffers;
            vertexBuffers.push_back(vertexBuffer);
            Ogre::VertexArrayObject* vao = vaoManager->createVertexArrayObject(vertexBuffers, indexBuffer, Ogre::OT_TRIANGLE_LIST);

            Ogre::SubMesh* beltSubMesh = mesh->createSubMesh();
            beltSubMesh->mVao[Ogre::VpNormal].push_back(vao);
            beltSubMesh->mVao[Ogre::VpShadow].push_back(vao);

            // Kept for updateScrollingUVs() to re-upload into every frame going forward - this
            // is the one member no sibling component needs, because none of them ever change
            // after the initial build.
            this->dynamicVertexBuffer = vertexBuffer;
        }

        // -- End cap submesh: plain BT_IMMUTABLE, built once, never touched again --
        if (numEndCapVerts > 0u)
        {
            float* vertexData = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(numEndCapVerts * floatsPerVertex * sizeof(float), Ogre::MEMCATEGORY_GEOMETRY));
            memcpy(vertexData, endCapVerts.data(), numEndCapVerts * floatsPerVertex * sizeof(float));

            Ogre::uint32* indexData = reinterpret_cast<Ogre::uint32*>(OGRE_MALLOC_SIMD(endCapInds.size() * sizeof(Ogre::uint32), Ogre::MEMCATEGORY_GEOMETRY));
            memcpy(indexData, endCapInds.data(), endCapInds.size() * sizeof(Ogre::uint32));

            for (size_t vi = 0u; vi < numEndCapVerts; ++vi)
            {
                const Ogre::Vector3 position(endCapVerts[vi * floatsPerVertex + 0], endCapVerts[vi * floatsPerVertex + 1], endCapVerts[vi * floatsPerVertex + 2]);
                aabbMin.makeFloor(position);
                aabbMax.makeCeil(position);
            }

        Ogre::VertexBufferPacked* vertexBuffer = nullptr;
        Ogre::IndexBufferPacked* indexBuffer = nullptr;

        try
        {
                vertexBuffer = vaoManager->createVertexBuffer(elements, numEndCapVerts, Ogre::BT_IMMUTABLE, vertexData, true);
                indexBuffer = vaoManager->createIndexBuffer(Ogre::IndexBufferPacked::IT_32BIT, endCapInds.size(), Ogre::BT_IMMUTABLE, indexData, true);
        }
        catch (const Ogre::Exception& e)
        {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralConveyorLoopComponent] Failed to create end cap buffers: " + e.getDescription());
            return;
        }

        Ogre::VertexBufferPackedVec vertexBuffers;
        vertexBuffers.push_back(vertexBuffer);
        Ogre::VertexArrayObject* vao = vaoManager->createVertexArrayObject(vertexBuffers, indexBuffer, Ogre::OT_TRIANGLE_LIST);

            Ogre::SubMesh* endCapSubMesh = mesh->createSubMesh();
            endCapSubMesh->mVao[Ogre::VpNormal].push_back(vao);
            endCapSubMesh->mVao[Ogre::VpShadow].push_back(vao);
        }

        if (aabbMin.x > aabbMax.x)
        {
            aabbMin = aabbMax = Ogre::Vector3::ZERO;
        }

        Ogre::Aabb bounds;
        bounds.setExtents(aabbMin, aabbMax);
        mesh->_setBounds(bounds, false);
        mesh->_setBoundingSphereRadius(bounds.getRadius());

        if (false == mesh->hasValidShadowMappingVaos())
        {
            mesh->prepareForShadowMapping(true);
        }

        this->conveyorItem = sceneManager->createItem(mesh, this->gameObjectPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC);
        this->conveyorItem->setName("ProceduralConveyorLoopItem_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()));
        this->conveyorItem->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);
        this->conveyorItem->setQueryFlags(this->gameObjectPtr->getCategoryId());
        this->conveyorItem->setCastShadows(true);

        // Submesh 0 = belt (scrolling), submesh 1 = end caps (static) - matching the exact order
        // createSubMesh() was called above.
        if (this->conveyorItem->getNumSubItems() > 0u)
        {
            const Ogre::String beltDbName = this->beltDatablock->getString();
            if (false == beltDbName.empty())
        {
                Ogre::HlmsDatablock* db = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(beltDbName);
            if (nullptr != db)
            {
                this->conveyorItem->getSubItem(0u)->setDatablock(db);
            }
        }
        }
        if (this->conveyorItem->getNumSubItems() > 1u)
        {
            const Ogre::String endCapDbName = this->endCapDatablock->getString();
            if (false == endCapDbName.empty())
            {
                Ogre::HlmsDatablock* db = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(endCapDbName);
                if (nullptr != db)
                {
                    this->conveyorItem->getSubItem(1u)->setDatablock(db);
                }
            }
        }

        this->gameObjectPtr->getSceneNode()->attachObject(this->conveyorItem);

        // Registers this item as the game object's own movable object - without this the object
        // cannot be selected in the editor, because its bounding box never gets picked up. Same
        // fix every procedural mesh component in this project has needed.
        this->gameObjectPtr->setDoNotDestroyMovableObject(true);
        this->gameObjectPtr->init(this->conveyorItem);

        if (false == this->gameObjectPtr->isDynamic())
        {
            sceneManager->notifyStaticAabbDirty(this->conveyorItem);
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
            "[ProceduralConveyorLoopComponent] Conveyor loop mesh created: " + Ogre::StringConverter::toString(static_cast<unsigned int>(numBeltVerts)) + " belt vertices, " +
                Ogre::StringConverter::toString(static_cast<unsigned int>(numEndCapVerts)) + " end cap vertices.");
    }

    void ProceduralConveyorLoopComponent::destroyConveyorMesh(void)
    {
        GraphicsModule::RenderCommand renderCommand = [this]()
        {
            if (nullptr == this->conveyorItem)
            {
                return;
            }

            Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();

            if (nullptr != this->gameObjectPtr->getSceneNode())
            {
                this->gameObjectPtr->getSceneNode()->detachObject(this->conveyorItem);
            }
            sceneManager->destroyItem(this->conveyorItem);
            this->conveyorItem = nullptr;
            this->dynamicVertexBuffer = nullptr;
            this->gameObjectPtr->nullMovableObject();

            Ogre::ResourcePtr existing = Ogre::MeshManager::getSingleton().getByName(this->conveyorMeshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
            if (false == existing.isNull())
            {
                Ogre::MeshManager::getSingleton().remove(existing->getHandle());
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralConveyorLoopComponent::destroyConveyorMesh");
    }

    void ProceduralConveyorLoopComponent::updateScrollingUVs(void)
    {
        if (true == this->beltVertices.empty() || nullptr == this->dynamicVertexBuffer)
        {
            return;
        }

        const Ogre::Real uScale = (this->totalPerimeter > 0.0001f) ? static_cast<Ogre::Real>(this->beltRepeatCount->getInt()) / this->totalPerimeter : 0.0f;
        const size_t floatsPerVertex = 12u;
        const size_t uOffsetWithinVertex = 10u;

        // Rewrites ONLY the U float of every belt vertex, in this->beltVertices (kept alive as a
        // member for exactly this purpose - see the header). Position, normal, tangent and V are
        // left completely untouched. The end cap submesh's buffer is never touched here at all -
        // it has its own, separate, immutable buffer that was only ever written once.
        for (size_t i = 0; i < this->beltVertexArcLength.size(); ++i)
            {
            const float arcLen = this->beltVertexArcLength[i];
            this->beltVertices[i * floatsPerVertex + uOffsetWithinVertex] = static_cast<float>((arcLen + this->scrollOffset) * uScale);
        }

        // Non-blocking enqueue, not enqueueAndWait: this runs every single frame the belt is
        // moving, and must not stall the logic thread waiting on the render thread the way the
        // one-off createConveyorMesh()/destroyConveyorMesh() calls are allowed to.
        std::vector<float> verticesCopy = this->beltVertices;
        const size_t vertexCount = this->currentBeltVertexIndex;

        GraphicsModule::RenderCommand renderCommand = [this, verticesCopy, vertexCount]()
        {
            if (nullptr != this->dynamicVertexBuffer)
            {
                this->dynamicVertexBuffer->upload(verticesCopy.data(), 0, vertexCount);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "ProceduralConveyorLoopComponent::updateScrollingUVs");
    }

    void ProceduralConveyorLoopComponent::updatePhysicsCollision(void)
    {
        if (nullptr == this->conveyorItem)
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
    // Setters and getters
    // =========================================================================================

    void ProceduralConveyorLoopComponent::setActivated(bool activated)
    {
        this->activated->setValue(activated);

        if (true == activated)
        {
            this->rebuildMesh();
        }
        else
        {
            this->destroyConveyorMesh();
        }
    }

    bool ProceduralConveyorLoopComponent::isActivated(void) const
    {
        return this->activated->getBool();
    }

    void ProceduralConveyorLoopComponent::setBeltLength(Ogre::Real beltLength)
    {
        this->beltLength->setValue(Ogre::Math::Clamp(beltLength, 0.2f, 1000.0f));
        this->rebuildMesh();
    }

    Ogre::Real ProceduralConveyorLoopComponent::getBeltLength(void) const
    {
        return this->beltLength->getReal();
    }

    void ProceduralConveyorLoopComponent::setRollerRadius(Ogre::Real rollerRadius)
    {
        this->rollerRadius->setValue(Ogre::Math::Clamp(rollerRadius, 0.05f, 100.0f));
        this->rebuildMesh();
    }

    Ogre::Real ProceduralConveyorLoopComponent::getRollerRadius(void) const
    {
        return this->rollerRadius->getReal();
    }

    void ProceduralConveyorLoopComponent::setDepth(Ogre::Real depth)
    {
        this->depth->setValue(Ogre::Math::Clamp(depth, 0.05f, 1000.0f));
        this->rebuildMesh();
    }

    Ogre::Real ProceduralConveyorLoopComponent::getDepth(void) const
    {
        return this->depth->getReal();
    }

    void ProceduralConveyorLoopComponent::setBeltSpeed(Ogre::Real beltSpeed)
    {
        this->beltSpeed->setValue(beltSpeed);
    }

    Ogre::Real ProceduralConveyorLoopComponent::getBeltSpeed(void) const
    {
        return this->beltSpeed->getReal();
    }

    void ProceduralConveyorLoopComponent::setBeltRepeatCount(int beltRepeatCount)
    {
        this->beltRepeatCount->setValue(std::max(1, beltRepeatCount));
        this->rebuildMesh();
    }

    int ProceduralConveyorLoopComponent::getBeltRepeatCount(void) const
    {
        return this->beltRepeatCount->getInt();
    }

    void ProceduralConveyorLoopComponent::setDepthUVTiling(Ogre::Real depthUVTiling)
    {
        this->depthUVTiling->setValue(depthUVTiling);
        this->rebuildMesh();
    }

    Ogre::Real ProceduralConveyorLoopComponent::getDepthUVTiling(void) const
    {
        return this->depthUVTiling->getReal();
    }

    void ProceduralConveyorLoopComponent::setBeltDatablock(const Ogre::String& beltDatablock)
    {
        this->beltDatablock->setValue(beltDatablock);
        this->rebuildMesh();
    }

    Ogre::String ProceduralConveyorLoopComponent::getBeltDatablock(void) const
    {
        return this->beltDatablock->getString();
    }

    void ProceduralConveyorLoopComponent::setEndCapDatablock(const Ogre::String& endCapDatablock)
    {
        this->endCapDatablock->setValue(endCapDatablock);
        this->rebuildMesh();
    }

    Ogre::String ProceduralConveyorLoopComponent::getEndCapDatablock(void) const
    {
        return this->endCapDatablock->getString();
    }

    // =========================================================================================
    // Lua API
    // =========================================================================================

    ProceduralConveyorLoopComponent* getProceduralConveyorLoopComponent(GameObject* gameObject, unsigned int occurrenceIndex)
    {
        return makeStrongPtr<ProceduralConveyorLoopComponent>(gameObject->getComponentWithOccurrence<ProceduralConveyorLoopComponent>(occurrenceIndex)).get();
    }

    ProceduralConveyorLoopComponent* getProceduralConveyorLoopComponent(GameObject* gameObject)
    {
        return makeStrongPtr<ProceduralConveyorLoopComponent>(gameObject->getComponent<ProceduralConveyorLoopComponent>()).get();
    }

    ProceduralConveyorLoopComponent* getProceduralConveyorLoopComponentFromName(GameObject* gameObject, const Ogre::String& name)
    {
        return makeStrongPtr<ProceduralConveyorLoopComponent>(gameObject->getComponentFromName<ProceduralConveyorLoopComponent>(name)).get();
    }

    void ProceduralConveyorLoopComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
    {
        module(lua)[class_<ProceduralConveyorLoopComponent, GameObjectComponent>("ProceduralConveyorLoopComponent")
                .def("setActivated", &ProceduralConveyorLoopComponent::setActivated)
                .def("isActivated", &ProceduralConveyorLoopComponent::isActivated)
                .def("setBeltLength", &ProceduralConveyorLoopComponent::setBeltLength)
                .def("getBeltLength", &ProceduralConveyorLoopComponent::getBeltLength)
                .def("setRollerRadius", &ProceduralConveyorLoopComponent::setRollerRadius)
                .def("getRollerRadius", &ProceduralConveyorLoopComponent::getRollerRadius)
                .def("setDepth", &ProceduralConveyorLoopComponent::setDepth)
                .def("getDepth", &ProceduralConveyorLoopComponent::getDepth)
                .def("setBeltSpeed", &ProceduralConveyorLoopComponent::setBeltSpeed)
                .def("getBeltSpeed", &ProceduralConveyorLoopComponent::getBeltSpeed)
                        .def("setBeltRepeatCount", &ProceduralConveyorLoopComponent::setBeltRepeatCount)
                        .def("getBeltRepeatCount", &ProceduralConveyorLoopComponent::getBeltRepeatCount)
                        .def("setDepthUVTiling", &ProceduralConveyorLoopComponent::setDepthUVTiling)
                        .def("getDepthUVTiling", &ProceduralConveyorLoopComponent::getDepthUVTiling)
                        .def("setBeltDatablock", &ProceduralConveyorLoopComponent::setBeltDatablock)
                        .def("getBeltDatablock", &ProceduralConveyorLoopComponent::getBeltDatablock)
                        .def("setEndCapDatablock", &ProceduralConveyorLoopComponent::setEndCapDatablock)
                        .def("getEndCapDatablock", &ProceduralConveyorLoopComponent::getEndCapDatablock)];

        LuaScriptApi::getInstance()->addClassToCollection("ProceduralConveyorLoopComponent", "class inherits GameObjectComponent", ProceduralConveyorLoopComponent::getStaticInfoText());

        LuaScriptApi::getInstance()->addClassToCollection("ProceduralConveyorLoopComponent", "void setBeltLength(float length)", "Sets the belt's length in meters. Regenerates the whole belt.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralConveyorLoopComponent", "float getBeltLength()", "Gets the belt's length in meters.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralConveyorLoopComponent", "void setRollerRadius(float radius)", "Sets the roller radius in meters. Regenerates the whole belt.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralConveyorLoopComponent", "float getRollerRadius()", "Gets the roller radius in meters.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralConveyorLoopComponent", "void setDepth(float depth)", "Sets the belt's depth in meters. Regenerates the whole belt.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralConveyorLoopComponent", "float getDepth()", "Gets the belt's depth in meters.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralConveyorLoopComponent", "void setBeltSpeed(float speed)", "Sets how fast the texture scrolls around the loop, in meters per second. Negative reverses direction.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralConveyorLoopComponent", "float getBeltSpeed()", "Gets the belt's scroll speed in meters per second.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralConveyorLoopComponent", "void setBeltRepeatCount(int count)",
            "Sets how many times the belt's texture repeats around one full lap - a whole number, so the scroll never pops once it wraps. Regenerates the whole belt.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralConveyorLoopComponent", "int getBeltRepeatCount()", "Gets the belt's texture repeat count.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralConveyorLoopComponent", "void setDepthUVTiling(float tiling)", "Sets the belt's texture tiling across its depth, in meters per repeat. Regenerates the whole belt.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralConveyorLoopComponent", "float getDepthUVTiling()", "Gets the belt's depth texture tiling.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralConveyorLoopComponent", "void setBeltDatablock(String datablock)", "Sets the datablock covering the belt's own scrolling surface.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralConveyorLoopComponent", "String getBeltDatablock()", "Gets the belt surface's datablock.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralConveyorLoopComponent", "void setEndCapDatablock(String datablock)", "Sets the datablock covering the two flat, non-scrolling end faces.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralConveyorLoopComponent", "String getEndCapDatablock()", "Gets the end caps' datablock.");

        gameObjectClass.def("getProceduralConveyorLoopComponentFromName", &getProceduralConveyorLoopComponentFromName);
        gameObjectClass.def("getProceduralConveyorLoopComponent", (ProceduralConveyorLoopComponent * (*)(GameObject*)) & getProceduralConveyorLoopComponent);
        gameObjectClass.def("getProceduralConveyorLoopComponent2", (ProceduralConveyorLoopComponent * (*)(GameObject*, unsigned int)) & getProceduralConveyorLoopComponent);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ProceduralConveyorLoopComponent getProceduralConveyorLoopComponent()", "Gets the component. Use this if the game object has this component only once.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ProceduralConveyorLoopComponent getProceduralConveyorLoopComponent2(unsigned int occurrenceIndex)",
            "Gets the component by the given occurrence index, since a game object may have this component several times.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ProceduralConveyorLoopComponent getProceduralConveyorLoopComponentFromName(String name)", "Gets the component by its custom name.");

        gameObjectControllerClass.def("castProceduralConveyorLoopComponent", &GameObjectController::cast<ProceduralConveyorLoopComponent>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "ProceduralConveyorLoopComponent castProceduralConveyorLoopComponent(ProceduralConveyorLoopComponent other)", "Casts an incoming type from function for lua auto completion.");
    }

}; // namespace end
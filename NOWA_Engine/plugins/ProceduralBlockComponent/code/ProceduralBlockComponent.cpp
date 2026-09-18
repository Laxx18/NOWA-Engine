/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#include "NOWAPrecompiled.h"
#include "ProceduralBlockComponent.h"
#include "gameobject/GameObjectController.h"
#include "gameobject/GameObjectFactory.h"
#include "gameobject/PhysicsArtifactComponent.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "modules/GraphicsModule.h"
#include "modules/LuaScriptApi.h"
#include "utilities/XMLConverter.h"
#include "utilities/Helper.h"
#include "utilities/MathHelper.h"

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

// =============================================================================
// ProceduralBlockComponent
//
// The simplest of the procedural mesh components - see the header for the full reasoning. Two
// points worth repeating here because they were both hard-won lessons from its two siblings:
//
//   - Every vertex carries a tangent from the start (ProceduralThornComponent's first draft did
//     not, and failed to apply a normal-mapped datablock at all).
//   - Face winding is derived from the intended outward normal via the same self-correcting
//     cross-product check ProceduralPlatformBoundaryComponent::addBoundaryQuad uses, rather than
//     worked out by hand per face (ProceduralThornComponent's cones needed several rounds of
//     fixing precisely because their winding was chosen manually).
// =============================================================================

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    ProceduralBlockComponent::ProceduralBlockComponent() :
        GameObjectComponent(),
        name("ProceduralBlockComponent"),
        activated(new Variant(ProceduralBlockComponent::AttrActivated(), true, this->attributes)),
        columns(new Variant(ProceduralBlockComponent::AttrColumns(), 5, this->attributes)),
        rows(new Variant(ProceduralBlockComponent::AttrRows(), 1, this->attributes)),
        depth(new Variant(ProceduralBlockComponent::AttrDepth(), 10.0f, this->attributes)),
        datablock(new Variant(ProceduralBlockComponent::AttrDatablock(), Ogre::String("rockClif_D"), this->attributes)),
        uvTiling(new Variant(ProceduralBlockComponent::AttrUVTiling(), Ogre::Vector2(1.0f, 5.0f), this->attributes)),
        useGradient(new Variant(ProceduralBlockComponent::AttrUseGradient(), false, this->attributes)),
        useBevel(new Variant(ProceduralBlockComponent::AttrUseBevel(), false, this->attributes)),
        bevelSize(new Variant(ProceduralBlockComponent::AttrBevelSize(), 0.1f, this->attributes)),
        currentVertexIndex(0),
        blockItem(nullptr),
        physicsArtifactComponent(nullptr)
    {
        this->activated->setDescription("Activates the block. When deactivated the mesh is removed.");

        this->columns->setDescription("Length of the block in whole meters, along the direction of travel. 1 column = 1 meter, "
                                      "so several blocks placed side by side line up on exact meter boundaries.");
        this->columns->setConstraints(1, 10000);

        this->rows->setDescription("Height of the block in whole meters. 1 row = 1 meter.");
        this->rows->setConstraints(1, 10000);

        this->depth->setDescription("Depth of the block in meters - a plain value, not row/column based. The block spans "
                                    "-Depth/2 .. +Depth/2 in local Z.");
        this->depth->setConstraints(0.05f, 1000.0f);

        this->datablock->setDescription("The single datablock used for the whole block.");

        this->uvTiling->setDescription("Texture tiling, in meters per texture repeat.");

        this->useGradient->setDescription("If true, builds the block as a sloped ramp instead of a straight box. The slope is not a "
                                          "separate setting - it is exactly Height / Length (Rows / Columns), rising from the ground "
                                          "at x = 0 to the full Height at x = Length. Takes priority over Use Bevel.");

        this->useBevel->setDescription("If true, chamfers the block's top-front and top-back edges instead of leaving them sharp. "
                                       "Has no effect while Use Gradient is also true.");

        this->bevelSize->setDescription("How far the chamfer cuts into the block along both the height and length axes, in meters. "
                                        "Clamped at render time to stay well short of overlapping the opposite chamfer or removing "
                                        "the block's own walls entirely.");
        this->bevelSize->setConstraints(0.01f, 100.0f);

        this->blockMeshName = "";
    }

    ProceduralBlockComponent::~ProceduralBlockComponent()
    {
    }

    const Ogre::String& ProceduralBlockComponent::getName() const
    {
        return this->name;
    }

    void ProceduralBlockComponent::install(const Ogre::NameValuePairList* options)
    {
        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<ProceduralBlockComponent>(ProceduralBlockComponent::getStaticClassId(), ProceduralBlockComponent::getStaticClassName());
    }

    void ProceduralBlockComponent::shutdown()
    {
    }

    void ProceduralBlockComponent::uninstall()
    {
    }

    void ProceduralBlockComponent::initialise()
    {
    }

    void ProceduralBlockComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    bool ProceduralBlockComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralBlockComponent::AttrActivated())
        {
            this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralBlockComponent::AttrColumns())
        {
            this->columns->setValue(Ogre::Math::Clamp(XMLConverter::getAttribInt(propertyElement, "data", 5), 1, 10000));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralBlockComponent::AttrRows())
        {
            this->rows->setValue(Ogre::Math::Clamp(XMLConverter::getAttribInt(propertyElement, "data", 1), 1, 10000));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralBlockComponent::AttrDepth())
        {
            this->depth->setValue(Ogre::Math::Clamp(XMLConverter::getAttribReal(propertyElement, "data", 2.0f), 0.05f, 1000.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralBlockComponent::AttrDatablock())
        {
            this->datablock->setValue(XMLConverter::getAttrib(propertyElement, "data", "rockClif_D"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralBlockComponent::AttrUVTiling())
        {
            this->uvTiling->setValue(XMLConverter::getAttribVector2(propertyElement, "data", Ogre::Vector2(1.0f, 1.0f)));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralBlockComponent::AttrUseGradient())
        {
            this->useGradient->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralBlockComponent::AttrUseBevel())
        {
            this->useBevel->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralBlockComponent::AttrBevelSize())
        {
            this->bevelSize->setValue(Ogre::Math::Clamp(XMLConverter::getAttribReal(propertyElement, "data", 0.1f), 0.01f, 100.0f));
            propertyElement = propertyElement->next_sibling("property");
        }

        return true;
    }

    GameObjectCompPtr ProceduralBlockComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        ProceduralBlockCompPtr clonedCompPtr(boost::make_shared<ProceduralBlockComponent>());

        clonedCompPtr->setColumns(this->columns->getInt());
        clonedCompPtr->setRows(this->rows->getInt());
        clonedCompPtr->setDepth(this->depth->getReal());
        clonedCompPtr->setDatablock(this->datablock->getString());
        clonedCompPtr->setUVTiling(this->uvTiling->getVector2());
        clonedCompPtr->setUseGradient(this->useGradient->getBool());
        clonedCompPtr->setUseBevel(this->useBevel->getBool());
        clonedCompPtr->setBevelSize(this->bevelSize->getReal());
        clonedGameObjectPtr->addComponent(clonedCompPtr);
        clonedCompPtr->setOwner(clonedGameObjectPtr);

        clonedCompPtr->setActivated(this->activated->getBool());

        return clonedCompPtr;
    }

    bool ProceduralBlockComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralBlockComponent] Init block component for game object: " + this->gameObjectPtr->getName());

        this->blockMeshName = "ProceduralBlockMesh_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());

        if (true == this->activated->getBool())
        {
            this->rebuildMesh();
        }

        return true;
    }

    void ProceduralBlockComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralBlockComponent] Remove block component for game object: " + this->gameObjectPtr->getName());

        this->destroyBlockMesh();
    }

    void ProceduralBlockComponent::update(Ogre::Real dt, bool notSimulating)
    {
        // Nothing to do per frame: the block is static geometry and every change is
        // property-driven. Kept as an explicit empty override so it is clear this is
        // intentional rather than forgotten.
    }

    void ProceduralBlockComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (ProceduralBlockComponent::AttrActivated() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
        }
        else if (ProceduralBlockComponent::AttrColumns() == attribute->getName())
        {
            this->setColumns(attribute->getInt());
        }
        else if (ProceduralBlockComponent::AttrRows() == attribute->getName())
        {
            this->setRows(attribute->getInt());
        }
        else if (ProceduralBlockComponent::AttrDepth() == attribute->getName())
        {
            this->setDepth(attribute->getReal());
        }
        else if (ProceduralBlockComponent::AttrDatablock() == attribute->getName())
        {
            this->setDatablock(attribute->getString());
        }
        else if (ProceduralBlockComponent::AttrUVTiling() == attribute->getName())
        {
            this->setUVTiling(attribute->getVector2());
        }
        else if (ProceduralBlockComponent::AttrUseGradient() == attribute->getName())
        {
            this->setUseGradient(attribute->getBool());
        }
        else if (ProceduralBlockComponent::AttrUseBevel() == attribute->getName())
        {
            this->setUseBevel(attribute->getBool());
        }
        else if (ProceduralBlockComponent::AttrBevelSize() == attribute->getName())
        {
            this->setBevelSize(attribute->getReal());
        }
    }

    void ProceduralBlockComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        // 2 = int, 6 = real, 7 = string, 8 = vector2, 12 = bool
        GameObjectComponent::writeXML(propertiesXML, doc);

        xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralBlockComponent::AttrActivated().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralBlockComponent::AttrColumns().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->columns->getInt())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralBlockComponent::AttrRows().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rows->getInt())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralBlockComponent::AttrDepth().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->depth->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralBlockComponent::AttrDatablock().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->datablock->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralBlockComponent::AttrUVTiling().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->uvTiling->getVector2())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralBlockComponent::AttrUseGradient().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useGradient->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralBlockComponent::AttrUseBevel().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useBevel->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralBlockComponent::AttrBevelSize().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->bevelSize->getReal())));
        propertiesXML->append_node(propertyXML);
    }

    // =========================================================================================
    // Mesh generation
    // =========================================================================================

    void ProceduralBlockComponent::addBlockQuad(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& v3, const Ogre::Vector3& normal, Ogre::Real u0, Ogre::Real u1, Ogre::Real v0Coord, Ogre::Real v1Coord)
    {
        // Winding is derived from the normal rather than worked out by hand per face - the same
        // self-correcting approach as ProceduralPlatformBoundaryComponent::addBoundaryQuad, and
        // deliberately reused rather than re-derived: addBlockBox emits six faces whose corner
        // order follows the box's own axes, so half of them would otherwise come out inside-out,
        // exactly the class of bug that needed several rounds of fixing on
        // ProceduralThornComponent's cones.
        const Ogre::Vector3 edge1 = v1 - v0;
        const Ogre::Vector3 edge2 = v3 - v0;
        const bool flip = edge1.crossProduct(edge2).dotProduct(normal) < 0.0f;

        // Tangent = one of the face's own edges, which is always exactly in the face's plane -
        // sufficient for correct normal mapping on a plain axis-aligned box face.
        Ogre::Vector3 tangent = edge1;
        if (tangent.squaredLength() < 0.0001f)
        {
            tangent = edge2;
        }
        tangent.normalise();

        const Ogre::Vector3 positions[4] = {v0, v1, v2, v3};
        const Ogre::Real uvs[4][2] = {{u0, v0Coord}, {u1, v0Coord}, {u1, v1Coord}, {u0, v1Coord}};

        for (int i = 0; i < 4; ++i)
        {
            this->vertices.push_back(positions[i].x);
            this->vertices.push_back(positions[i].y);
            this->vertices.push_back(positions[i].z);
            this->vertices.push_back(normal.x);
            this->vertices.push_back(normal.y);
            this->vertices.push_back(normal.z);
            this->vertices.push_back(tangent.x);
            this->vertices.push_back(tangent.y);
            this->vertices.push_back(tangent.z);
            this->vertices.push_back(1.0f); // handedness
            this->vertices.push_back(static_cast<float>(uvs[i][0]));
            this->vertices.push_back(static_cast<float>(uvs[i][1]));
        }

        if (false == flip)
        {
            this->indices.push_back(this->currentVertexIndex + 0);
            this->indices.push_back(this->currentVertexIndex + 1);
            this->indices.push_back(this->currentVertexIndex + 2);
            this->indices.push_back(this->currentVertexIndex + 0);
            this->indices.push_back(this->currentVertexIndex + 2);
            this->indices.push_back(this->currentVertexIndex + 3);
        }
        else
        {
            this->indices.push_back(this->currentVertexIndex + 0);
            this->indices.push_back(this->currentVertexIndex + 2);
            this->indices.push_back(this->currentVertexIndex + 1);
            this->indices.push_back(this->currentVertexIndex + 0);
            this->indices.push_back(this->currentVertexIndex + 3);
            this->indices.push_back(this->currentVertexIndex + 2);
        }

        this->currentVertexIndex += 4;
    }

    void ProceduralBlockComponent::addBlockBox(const Ogre::Vector3& minCorner, const Ogre::Vector3& maxCorner)
    {
        const Ogre::Vector2 tiling = this->uvTiling->getVector2();

        const Ogre::Vector3& lo = minCorner;
        const Ogre::Vector3& hi = maxCorner;

        const Ogre::Real spanX = hi.x - lo.x;
        const Ogre::Real spanY = hi.y - lo.y;
        const Ogre::Real spanZ = hi.z - lo.z;

        struct Face
        {
            Ogre::Vector3 corners[4];
            Ogre::Vector3 normal;
            Ogre::Real uSpan;
            Ogre::Real vSpan;
        };

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
            // UVs are derived from the face's real size in meters, so texture density stays
            // constant regardless of the block's overall dimensions.
            this->addBlockQuad(face.corners[0], face.corners[1], face.corners[2], face.corners[3], face.normal, 0.0f, face.uSpan * tiling.x, 0.0f, face.vSpan * tiling.y);
        }
    }

    void ProceduralBlockComponent::addBlockTriangle(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& normal, const Ogre::Vector2& uv0, const Ogre::Vector2& uv1, const Ogre::Vector2& uv2)
    {
        // Same self-correcting winding as addBlockQuad, for the same reason: computed from the
        // actual triangle geometry and the desired outward normal, never picked by hand.
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
            this->vertices.push_back(positions[i].x);
            this->vertices.push_back(positions[i].y);
            this->vertices.push_back(positions[i].z);
            this->vertices.push_back(normal.x);
            this->vertices.push_back(normal.y);
            this->vertices.push_back(normal.z);
            this->vertices.push_back(tangent.x);
            this->vertices.push_back(tangent.y);
            this->vertices.push_back(tangent.z);
            this->vertices.push_back(1.0f); // handedness
            this->vertices.push_back(uvs[i].x);
            this->vertices.push_back(uvs[i].y);
        }

        if (false == flip)
        {
            this->indices.push_back(this->currentVertexIndex + 0);
            this->indices.push_back(this->currentVertexIndex + 1);
            this->indices.push_back(this->currentVertexIndex + 2);
        }
        else
        {
            this->indices.push_back(this->currentVertexIndex + 0);
            this->indices.push_back(this->currentVertexIndex + 2);
            this->indices.push_back(this->currentVertexIndex + 1);
        }

        this->currentVertexIndex += 3;
    }

    void ProceduralBlockComponent::addBlockChamferedBox(Ogre::Real length, Ogre::Real height, Ogre::Real halfDepth, Ogre::Real bevel)
    {
        const Ogre::Vector2 tiling = this->uvTiling->getVector2();
        const Ogre::Real depthSpan = 2.0f * halfDepth;

        // Hexagonal cross-section in the X-Y plane, wound counter-clockwise: bottom-front,
        // bottom-back, back-wall-top, back-chamfer-top, front-chamfer-top, front-wall-top - the
        // same six-point profile ProceduralPlatformComponent's own "Grass" style chamfer already
        // uses (there varying per path point; here a single constant cross-section extruded
        // through Depth, since a block does not sweep along a path).
        const Ogre::Vector2 hexPoints[6] = {Ogre::Vector2(0.0f, 0.0f), Ogre::Vector2(length, 0.0f), Ogre::Vector2(length, height - bevel), Ogre::Vector2(length - bevel, height), Ogre::Vector2(bevel, height),
            Ogre::Vector2(0.0f, height - bevel)};

        // For a CCW-wound convex polygon, rotating an edge's own direction (dx, dy) to (dy, -dx)
        // gives that edge's OUTWARD normal - verified by hand against four of these six edges
        // before relying on it here: the bottom edge (dx=length, dy=0) rotates to (0, -length),
        // i.e. straight down, matching a box's known bottom-face normal; the back wall
        // (dx=0, dy=height-bevel) rotates to (height-bevel, 0), i.e. +X, matching the box's
        // known back-face normal; the back chamfer and front chamfer both rotate to the
        // up-and-outward diagonal each one is expected to face. Generalising a verified
        // identity like this, rather than asserting each of the six normals by hand, is
        // deliberately different from how ProceduralThornComponent's cone normals were first
        // written - which needed two rounds of fixing precisely because they were asserted
        // rather than derived this way.
        Ogre::Real perimeterSoFar = 0.0f;
        for (int i = 0; i < 6; ++i)
        {
            const Ogre::Vector2& p0 = hexPoints[i];
            const Ogre::Vector2& p1 = hexPoints[(i + 1) % 6];

            const Ogre::Real dx = p1.x - p0.x;
            const Ogre::Real dy = p1.y - p0.y;
            const Ogre::Real edgeLength = std::sqrt(dx * dx + dy * dy);
            if (edgeLength < 0.0001f)
            {
                // Degenerate edge: happens if bevel is clamped right up against half the height,
                // making the flat top exactly zero-width. Nothing to build for it.
                continue;
            }

            const Ogre::Vector3 normal(dy / edgeLength, -dx / edgeLength, 0.0f);

            const Ogre::Vector3 v0(p0.x, p0.y, -halfDepth);
            const Ogre::Vector3 v1(p1.x, p1.y, -halfDepth);
            const Ogre::Vector3 v2(p1.x, p1.y, halfDepth);
            const Ogre::Vector3 v3(p0.x, p0.y, halfDepth);

            this->addBlockQuad(v0, v1, v2, v3, normal, perimeterSoFar * tiling.x, (perimeterSoFar + edgeLength) * tiling.x, 0.0f, depthSpan * tiling.y);
            perimeterSoFar += edgeLength;
        }

        // Two hexagonal end caps (front z = -halfDepth, back z = +halfDepth), fan-triangulated
        // from hexPoints[0] - matching how ProceduralPlatformBoundaryComponent's own end caps
        // triangulate an arbitrary profile.
        for (int side = 0; side < 2; ++side)
        {
            const Ogre::Real z = (0 == side) ? -halfDepth : halfDepth;
            const Ogre::Vector3 normal(0.0f, 0.0f, (0 == side) ? -1.0f : 1.0f);

            for (int i = 1; i + 1 < 6; ++i)
            {
                const Ogre::Vector3 v0(hexPoints[0].x, hexPoints[0].y, z);
                const Ogre::Vector3 v1(hexPoints[i].x, hexPoints[i].y, z);
                const Ogre::Vector3 v2(hexPoints[i + 1].x, hexPoints[i + 1].y, z);

                this->addBlockTriangle(v0, v1, v2, normal, Ogre::Vector2(hexPoints[0].x * tiling.x, hexPoints[0].y * tiling.y), Ogre::Vector2(hexPoints[i].x * tiling.x, hexPoints[i].y * tiling.y),
                    Ogre::Vector2(hexPoints[i + 1].x * tiling.x, hexPoints[i + 1].y * tiling.y));
            }
        }
    }

    void ProceduralBlockComponent::addBlockWedge(Ogre::Real length, Ogre::Real height, Ogre::Real halfDepth)
    {
        const Ogre::Vector2 tiling = this->uvTiling->getVector2();
        const Ogre::Real depthSpan = 2.0f * halfDepth;
        const Ogre::Real slopeLength = std::sqrt(length * length + height * height);

        // Bottom (flat on the ground, full footprint) - normal straight down, same convention
        // as the plain box's own bottom face.
        this->addBlockQuad(Ogre::Vector3(0.0f, 0.0f, -halfDepth), Ogre::Vector3(length, 0.0f, -halfDepth), Ogre::Vector3(length, 0.0f, halfDepth), Ogre::Vector3(0.0f, 0.0f, halfDepth), Ogre::Vector3(0.0f, -1.0f, 0.0f), 0.0f,
            length * tiling.x, 0.0f, depthSpan * tiling.y);

        // Back wall (vertical, at x = length) - normal +X, same convention as the plain box's
        // own back face.
        this->addBlockQuad(Ogre::Vector3(length, 0.0f, -halfDepth), Ogre::Vector3(length, 0.0f, halfDepth), Ogre::Vector3(length, height, halfDepth), Ogre::Vector3(length, height, -halfDepth), Ogre::Vector3(1.0f, 0.0f, 0.0f),
            0.0f, depthSpan * tiling.x, 0.0f, height * tiling.y);

        // Slope (the walkable ramp surface, from (0,0) up to (length,height)). addBlockQuad's
        // own self-correcting winding means the CORNER ORDER passed in here doesn't need to be
        // hand-verified - only the NORMAL's sign does. Outward means "away from the wedge's
        // solid interior", which occupies x >= 0 below-and-right of the slope line; moving from
        // any point on the slope in the direction (-height, length) heads toward negative x,
        // away from that interior - confirmed directly, not derived from a general rotation
        // rule this time, since the slope's own winding direction differs from the chamfer
        // hexagon's.
        Ogre::Vector3 slopeNormal(-height, length, 0.0f);
        slopeNormal.normalise();
        this->addBlockQuad(Ogre::Vector3(0.0f, 0.0f, -halfDepth), Ogre::Vector3(length, height, -halfDepth), Ogre::Vector3(length, height, halfDepth), Ogre::Vector3(0.0f, 0.0f, halfDepth), slopeNormal, 0.0f,
            slopeLength * tiling.x, 0.0f, depthSpan * tiling.y);

        // Two triangular side caps, closing the wedge's depth ends - same outward-along-Z
        // convention as every other component's end caps in this project.
        this->addBlockTriangle(Ogre::Vector3(0.0f, 0.0f, -halfDepth), Ogre::Vector3(length, 0.0f, -halfDepth), Ogre::Vector3(length, height, -halfDepth), Ogre::Vector3(0.0f, 0.0f, -1.0f), Ogre::Vector2(0.0f, 0.0f),
            Ogre::Vector2(length * tiling.x, 0.0f), Ogre::Vector2(length * tiling.x, height * tiling.y));

        this->addBlockTriangle(Ogre::Vector3(length, 0.0f, halfDepth), Ogre::Vector3(0.0f, 0.0f, halfDepth), Ogre::Vector3(length, height, halfDepth), Ogre::Vector3(0.0f, 0.0f, 1.0f), Ogre::Vector2(length * tiling.x, 0.0f),
            Ogre::Vector2(0.0f, 0.0f), Ogre::Vector2(length * tiling.x, height * tiling.y));
    }

    void ProceduralBlockComponent::rebuildMesh(void)
    {
        this->vertices.clear();
        this->indices.clear();
        this->currentVertexIndex = 0;

        const Ogre::Real length = this->getLength();
        const Ogre::Real height = this->getHeight();
        const Ogre::Real halfDepth = this->depth->getReal() * 0.5f;

        // Gradient takes priority over bevel, per the header's own documented precedence: a
        // ramp's own sloped top edge is not additionally chamfered by this component - combining
        // the two would need real 3D edge chamfering rather than the flat 2D cross-section
        // extrusion both addBlockChamferedBox and addBlockWedge rely on, and is deliberately not
        // attempted here.
        if (true == this->useGradient->getBool())
        {
            this->addBlockWedge(length, height, halfDepth);
        }
        else if (true == this->useBevel->getBool())
        {
            // Clamped against the CURRENT length/height here, not only at the setter - Columns,
            // Rows and Bevel Size can each change independently, and a bevel that was valid for
            // one size might not be for another.
            const Ogre::Real bevel = Ogre::Math::Clamp(this->bevelSize->getReal(), 0.0f, std::min(length, height) * 0.45f);
            if (bevel > 0.0001f)
            {
                this->addBlockChamferedBox(length, height, halfDepth, bevel);
            }
            else
            {
                this->addBlockBox(Ogre::Vector3(0.0f, 0.0f, -halfDepth), Ogre::Vector3(length, height, halfDepth));
            }
        }
        else
        {
        this->addBlockBox(Ogre::Vector3(0.0f, 0.0f, -halfDepth), Ogre::Vector3(length, height, halfDepth));
        }

        this->createBlockMesh();
    }

    void ProceduralBlockComponent::createBlockMesh(void)
    {
        if (0 == this->currentVertexIndex)
        {
            this->destroyBlockMesh();
            return;
        }

        std::vector<float> verticesCopy = this->vertices;
        std::vector<Ogre::uint32> indicesCopy = this->indices;
        const size_t numVertices = this->currentVertexIndex;

        GraphicsModule::RenderCommand renderCommand = [this, verticesCopy, indicesCopy, numVertices]()
        {
            this->createBlockMeshInternal(verticesCopy, indicesCopy, numVertices);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralBlockComponent::createBlockMesh");

        this->updatePhysicsCollision();
    }

    void ProceduralBlockComponent::createBlockMeshInternal(const std::vector<float>& verts, const std::vector<Ogre::uint32>& inds, size_t numVerts)
    {
        //  RUNS ON RENDER THREAD!
        Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();
        Ogre::VaoManager* vaoManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getVaoManager();

        if (nullptr != this->blockItem)
        {
            this->gameObjectPtr->getSceneNode()->detachObject(this->blockItem);
            sceneManager->destroyItem(this->blockItem);
            this->blockItem = nullptr;
            this->gameObjectPtr->nullMovableObject();
        }

        {
            Ogre::ResourcePtr existing = Ogre::MeshManager::getSingleton().getByName(this->blockMeshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
            if (false == existing.isNull())
            {
                Ogre::MeshManager::getSingleton().remove(existing->getHandle());
            }
        }

        Ogre::MeshPtr mesh = Ogre::MeshManager::getSingleton().createManual(this->blockMeshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, &NOWA::gDummyMeshLoader);
        mesh->_setVaoManager(vaoManager);

        const size_t floatsPerVertex = 12u;

        float* vertexData = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(numVerts * floatsPerVertex * sizeof(float), Ogre::MEMCATEGORY_GEOMETRY));
        memcpy(vertexData, verts.data(), numVerts * floatsPerVertex * sizeof(float));

        Ogre::uint32* indexData = reinterpret_cast<Ogre::uint32*>(OGRE_MALLOC_SIMD(inds.size() * sizeof(Ogre::uint32), Ogre::MEMCATEGORY_GEOMETRY));
        memcpy(indexData, inds.data(), inds.size() * sizeof(Ogre::uint32));

        Ogre::Vector3 aabbMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
        Ogre::Vector3 aabbMax(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());
        for (size_t vi = 0u; vi < numVerts; ++vi)
        {
            const Ogre::Vector3 position(verts[vi * floatsPerVertex + 0], verts[vi * floatsPerVertex + 1], verts[vi * floatsPerVertex + 2]);
            aabbMin.makeFloor(position);
            aabbMax.makeCeil(position);
        }
        if (aabbMin.x > aabbMax.x)
        {
            aabbMin = aabbMax = Ogre::Vector3::ZERO;
        }

        Ogre::VertexElement2Vec elements;
        elements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
        elements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));
        elements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT4, Ogre::VES_TANGENT));
        elements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));

        Ogre::VertexBufferPacked* vertexBuffer = nullptr;
        Ogre::IndexBufferPacked* indexBuffer = nullptr;

        try
        {
            vertexBuffer = vaoManager->createVertexBuffer(elements, numVerts, Ogre::BT_IMMUTABLE, vertexData, true);
            indexBuffer = vaoManager->createIndexBuffer(Ogre::IndexBufferPacked::IT_32BIT, inds.size(), Ogre::BT_IMMUTABLE, indexData, true);
        }
        catch (const Ogre::Exception& e)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralBlockComponent] Failed to create buffers: " + e.getDescription());
            return;
        }

        Ogre::VertexBufferPackedVec vertexBuffers;
        vertexBuffers.push_back(vertexBuffer);
        Ogre::VertexArrayObject* vao = vaoManager->createVertexArrayObject(vertexBuffers, indexBuffer, Ogre::OT_TRIANGLE_LIST);

        Ogre::SubMesh* subMesh = mesh->createSubMesh();
        subMesh->mVao[Ogre::VpNormal].push_back(vao);
        subMesh->mVao[Ogre::VpShadow].push_back(vao);

        Ogre::Aabb bounds;
        bounds.setExtents(aabbMin, aabbMax);
        mesh->_setBounds(bounds, false);
        mesh->_setBoundingSphereRadius(bounds.getRadius());

        if (false == mesh->hasValidShadowMappingVaos())
        {
            mesh->prepareForShadowMapping(true);
        }

        this->blockItem = sceneManager->createItem(mesh, this->gameObjectPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC);
        this->blockItem->setName("ProceduralBlockItem_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()));
        this->blockItem->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);
        this->blockItem->setQueryFlags(this->gameObjectPtr->getCategoryId());
        this->blockItem->setCastShadows(true);

        const Ogre::String dbName = this->datablock->getString();
        if (false == dbName.empty() && this->blockItem->getNumSubItems() > 0u)
        {
            Ogre::HlmsDatablock* db = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(dbName);
            if (nullptr != db)
            {
                this->blockItem->getSubItem(0u)->setDatablock(db);
            }
        }

        this->gameObjectPtr->getSceneNode()->attachObject(this->blockItem);

        // Registers this item as the game object's own movable object - without this the object
        // cannot be selected in the editor, because its bounding box never gets picked up. Same
        // fix ProceduralPlatformBoundaryComponent and ProceduralThornComponent both needed.
        this->gameObjectPtr->setDoNotDestroyMovableObject(true);
        this->gameObjectPtr->init(this->blockItem);

        if (false == this->gameObjectPtr->isDynamic())
        {
            sceneManager->notifyStaticAabbDirty(this->blockItem);
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralBlockComponent] Block mesh created: " + Ogre::StringConverter::toString(static_cast<unsigned int>(numVerts)) + " vertices.");
    }

    void ProceduralBlockComponent::destroyBlockMesh(void)
    {
        GraphicsModule::RenderCommand renderCommand = [this]()
        {
            if (nullptr == this->blockItem)
            {
                return;
            }

            Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();

            if (nullptr != this->gameObjectPtr->getSceneNode())
            {
                this->gameObjectPtr->getSceneNode()->detachObject(this->blockItem);
            }
            sceneManager->destroyItem(this->blockItem);
            this->blockItem = nullptr;
            this->gameObjectPtr->nullMovableObject();

            Ogre::ResourcePtr existing = Ogre::MeshManager::getSingleton().getByName(this->blockMeshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
            if (false == existing.isNull())
            {
                Ogre::MeshManager::getSingleton().remove(existing->getHandle());
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralBlockComponent::destroyBlockMesh");
    }

    void ProceduralBlockComponent::updatePhysicsCollision(void)
    {
        if (nullptr == this->blockItem)
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

    void ProceduralBlockComponent::setActivated(bool activated)
    {
        this->activated->setValue(activated);

        if (true == activated)
        {
            this->rebuildMesh();
        }
        else
        {
            this->destroyBlockMesh();
        }
    }

    bool ProceduralBlockComponent::isActivated(void) const
    {
        return this->activated->getBool();
    }

    void ProceduralBlockComponent::setColumns(int columns)
    {
        this->columns->setValue(Ogre::Math::Clamp(columns, 1, 10000));
        this->rebuildMesh();
    }

    int ProceduralBlockComponent::getColumns(void) const
    {
        return this->columns->getInt();
    }

    void ProceduralBlockComponent::setRows(int rows)
    {
        this->rows->setValue(Ogre::Math::Clamp(rows, 1, 10000));
        this->rebuildMesh();
    }

    int ProceduralBlockComponent::getRows(void) const
    {
        return this->rows->getInt();
    }

    void ProceduralBlockComponent::setDepth(Ogre::Real depth)
    {
        this->depth->setValue(Ogre::Math::Clamp(depth, 0.05f, 1000.0f));
        this->rebuildMesh();
    }

    Ogre::Real ProceduralBlockComponent::getDepth(void) const
    {
        return this->depth->getReal();
    }

    Ogre::Real ProceduralBlockComponent::getLength(void) const
    {
        return static_cast<Ogre::Real>(this->columns->getInt()) * 1.0f;
    }

    Ogre::Real ProceduralBlockComponent::getHeight(void) const
    {
        return static_cast<Ogre::Real>(this->rows->getInt()) * 1.0f;
    }

    void ProceduralBlockComponent::setDatablock(const Ogre::String& datablock)
    {
        this->datablock->setValue(datablock);
        this->rebuildMesh();
    }

    Ogre::String ProceduralBlockComponent::getDatablock(void) const
    {
        return this->datablock->getString();
    }

    void ProceduralBlockComponent::setUVTiling(const Ogre::Vector2& tiling)
    {
        this->uvTiling->setValue(tiling);
        this->rebuildMesh();
    }

    Ogre::Vector2 ProceduralBlockComponent::getUVTiling(void) const
    {
        return this->uvTiling->getVector2();
    }

    void ProceduralBlockComponent::setUseGradient(bool useGradient)
    {
        this->useGradient->setValue(useGradient);
        this->rebuildMesh();
    }

    bool ProceduralBlockComponent::getUseGradient(void) const
    {
        return this->useGradient->getBool();
    }

    void ProceduralBlockComponent::setUseBevel(bool useBevel)
    {
        this->useBevel->setValue(useBevel);
        this->rebuildMesh();
    }

    bool ProceduralBlockComponent::getUseBevel(void) const
    {
        return this->useBevel->getBool();
    }

    void ProceduralBlockComponent::setBevelSize(Ogre::Real bevelSize)
    {
        this->bevelSize->setValue(Ogre::Math::Clamp(bevelSize, 0.01f, 100.0f));
        this->rebuildMesh();
    }

    Ogre::Real ProceduralBlockComponent::getBevelSize(void) const
    {
        return this->bevelSize->getReal();
    }

    // =========================================================================================
    // Lua API
    // =========================================================================================

    ProceduralBlockComponent* getProceduralBlockComponent(GameObject* gameObject, unsigned int occurrenceIndex)
    {
        return makeStrongPtr<ProceduralBlockComponent>(gameObject->getComponentWithOccurrence<ProceduralBlockComponent>(occurrenceIndex)).get();
    }

    ProceduralBlockComponent* getProceduralBlockComponent(GameObject* gameObject)
    {
        return makeStrongPtr<ProceduralBlockComponent>(gameObject->getComponent<ProceduralBlockComponent>()).get();
    }

    ProceduralBlockComponent* getProceduralBlockComponentFromName(GameObject* gameObject, const Ogre::String& name)
    {
        return makeStrongPtr<ProceduralBlockComponent>(gameObject->getComponentFromName<ProceduralBlockComponent>(name)).get();
    }

    void ProceduralBlockComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
    {
        module(lua)[class_<ProceduralBlockComponent, GameObjectComponent>("ProceduralBlockComponent")
                .def("setActivated", &ProceduralBlockComponent::setActivated)
                .def("isActivated", &ProceduralBlockComponent::isActivated)
                .def("setColumns", &ProceduralBlockComponent::setColumns)
                .def("getColumns", &ProceduralBlockComponent::getColumns)
                .def("setRows", &ProceduralBlockComponent::setRows)
                .def("getRows", &ProceduralBlockComponent::getRows)
                .def("setDepth", &ProceduralBlockComponent::setDepth)
                .def("getDepth", &ProceduralBlockComponent::getDepth)
                .def("getLength", &ProceduralBlockComponent::getLength)
                .def("getHeight", &ProceduralBlockComponent::getHeight)
                .def("setDatablock", &ProceduralBlockComponent::setDatablock)
                        .def("getDatablock", &ProceduralBlockComponent::getDatablock)
                        .def("setUseGradient", &ProceduralBlockComponent::setUseGradient)
                        .def("getUseGradient", &ProceduralBlockComponent::getUseGradient)
                        .def("setUseBevel", &ProceduralBlockComponent::setUseBevel)
                        .def("getUseBevel", &ProceduralBlockComponent::getUseBevel)
                        .def("setBevelSize", &ProceduralBlockComponent::setBevelSize)
                        .def("getBevelSize", &ProceduralBlockComponent::getBevelSize)];

        LuaScriptApi::getInstance()->addClassToCollection("ProceduralBlockComponent", "class inherits GameObjectComponent", ProceduralBlockComponent::getStaticInfoText());

        LuaScriptApi::getInstance()->addClassToCollection("ProceduralBlockComponent", "void setColumns(int columns)", "Sets the block's length in whole meters (1 column = 1 meter). Regenerates the whole block.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralBlockComponent", "int getColumns()", "Gets the block's length in columns.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralBlockComponent", "void setRows(int rows)", "Sets the block's height in whole meters (1 row = 1 meter). Regenerates the whole block.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralBlockComponent", "int getRows()", "Gets the block's height in rows.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralBlockComponent", "void setDepth(float depth)", "Sets the block's depth in meters. Regenerates the whole block.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralBlockComponent", "float getDepth()", "Gets the block's depth in meters.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralBlockComponent", "float getLength()", "Gets the block's actual length in meters (columns * 1.0).");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralBlockComponent", "float getHeight()", "Gets the block's actual height in meters (rows * 1.0).");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralBlockComponent", "void setDatablock(String datablock)", "Sets the single datablock used for the whole block.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralBlockComponent", "String getDatablock()", "Gets the block's datablock.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralBlockComponent", "void setUseGradient(bool useGradient)",
            "Sets whether the block is a sloped ramp instead of a straight box. The slope is Height / Length, not a separate setting. Takes priority over Use Bevel. Regenerates the whole block.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralBlockComponent", "bool getUseGradient()", "Gets whether the block is built as a ramp.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralBlockComponent", "void setUseBevel(bool useBevel)",
            "Sets whether the block's top-front and top-back edges are chamfered. Has no effect while Use Gradient is true. Regenerates the whole block.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralBlockComponent", "bool getUseBevel()", "Gets whether the block's top edges are chamfered.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralBlockComponent", "void setBevelSize(float bevelSize)", "Sets how far the chamfer cuts in, in meters. Regenerates the whole block.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralBlockComponent", "float getBevelSize()", "Gets the chamfer size in meters.");

        gameObjectClass.def("getProceduralBlockComponentFromName", &getProceduralBlockComponentFromName);
        gameObjectClass.def("getProceduralBlockComponent", (ProceduralBlockComponent * (*)(GameObject*)) & getProceduralBlockComponent);
        gameObjectClass.def("getProceduralBlockComponent2", (ProceduralBlockComponent * (*)(GameObject*, unsigned int)) & getProceduralBlockComponent);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ProceduralBlockComponent getProceduralBlockComponent()", "Gets the component. Use this if the game object has this component only once.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ProceduralBlockComponent getProceduralBlockComponent2(unsigned int occurrenceIndex)",
            "Gets the component by the given occurrence index, since a game object may have this component several times.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ProceduralBlockComponent getProceduralBlockComponentFromName(String name)", "Gets the component by its custom name.");

        gameObjectControllerClass.def("castProceduralBlockComponent", &GameObjectController::cast<ProceduralBlockComponent>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "ProceduralBlockComponent castProceduralBlockComponent(ProceduralBlockComponent other)", "Casts an incoming type from function for lua auto completion.");
    }

}; // namespace end
/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#include "NOWAPrecompiled.h"
#include "ProceduralThornComponent.h"
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
// ProceduralThornComponent
//
// The simplified, non-interactive sibling of ProceduralPlatformBoundaryComponent. See the class
// comment in the header for the geometry and for what this component deliberately does NOT do
// (apply damage on contact - see the accompanying reply for why).
//
// Design decisions worth stating up front, mirroring the reasoning already established for
// ProceduralPlatformBoundaryComponent:
//
//   - The strip is regenerated WHOLE on every change, from five numbers - no cached vertex
//     buffer, no undo/redo. A spike strip is a few dozen triangles at most; there is nothing
//     here worth caching against the ways a cache could go stale.
//
//   - Every vertex carries a tangent from the very first version of this file. The boundary
//     component's FIRST draft did not, and failed with "Renderable can't use normal maps but
//     datablock wants normal maps" the moment a normal-mapped datablock was assigned - see that
//     component's own history. Building it in from the start here avoids repeating that bug.
// =============================================================================

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    ProceduralThornComponent::ProceduralThornComponent() :
        GameObjectComponent(),
        name("ProceduralThornComponent"),
        activated(new Variant(ProceduralThornComponent::AttrActivated(), true, this->attributes)),
        thornLength(new Variant(ProceduralThornComponent::AttrThornLength(), 10.0f, this->attributes)),
        thornWidth(new Variant(ProceduralThornComponent::AttrThornWidth(), 10.0f, this->attributes)),
        thornHeight(new Variant(ProceduralThornComponent::AttrThornHeight(), 1.0f, this->attributes)),
        thornBaseSize(new Variant(ProceduralThornComponent::AttrThornBaseSize(), 0.5f, this->attributes)),
        datablock(new Variant(ProceduralThornComponent::AttrDatablock(), Ogre::String("SOLID/TEX/thorn.png"), this->attributes)),
        uvTiling(new Variant(ProceduralThornComponent::AttrUVTiling(), Ogre::Vector2(1.0f, 5.0f), this->attributes)),
        currentVertexIndex(0),
        thornItem(nullptr),
        physicsArtifactComponent(nullptr)
    {
        this->activated->setDescription("Activates the spike strip. When deactivated the mesh is removed.");

        this->thornLength->setDescription("Total length of the spike strip in meters, along the direction of travel. This is an "
                                          "exact outer dimension, like the level boundary's own Width.");
        this->thornLength->setConstraints(0.1f, 1000.0f);

        this->thornWidth->setDescription("Depth of the strip in meters, along the fixed Z axis - centred, so it spans "
                                         "-Width/2 .. +Width/2 in local Z. Set this to span the full walkable depth so "
                                         "there is no gap to sneak through sideways.");
        this->thornWidth->setConstraints(0.1f, 1000.0f);

        this->thornHeight->setDescription("Height of each spike in meters, from its base to its apex.");
        this->thornHeight->setConstraints(0.05f, 100.0f);

        this->thornBaseSize->setDescription("Each cone's base diameter in meters, e.g. 0.5 means a 0.5 x 0.5m footprint. Also the "
                                            "grid spacing in both Length and Width directions, so cones sit base-to-base with no "
                                            "gaps. The actual grid counts are recomputed so the cones fill Thorn Length and Thorn "
                                            "Width exactly.");
        this->thornBaseSize->setConstraints(0.05f, 50.0f);

        this->datablock->setDescription("The single datablock used for the whole spike strip.");

        this->uvTiling->setDescription("Texture tiling, in meters per texture repeat.");

        this->thornMeshName = "";
    }

    ProceduralThornComponent::~ProceduralThornComponent()
    {
    }

    const Ogre::String& ProceduralThornComponent::getName() const
    {
        return this->name;
    }

    void ProceduralThornComponent::install(const Ogre::NameValuePairList* options)
    {
        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<ProceduralThornComponent>(ProceduralThornComponent::getStaticClassId(), ProceduralThornComponent::getStaticClassName());
    }

    void ProceduralThornComponent::shutdown()
    {
    }

    void ProceduralThornComponent::uninstall()
    {
    }

    void ProceduralThornComponent::initialise()
    {
    }

    void ProceduralThornComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    bool ProceduralThornComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralThornComponent::AttrActivated())
        {
            this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralThornComponent::AttrThornLength())
        {
            this->thornLength->setValue(Ogre::Math::Clamp(XMLConverter::getAttribReal(propertyElement, "data", 5.0f), 0.1f, 1000.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralThornComponent::AttrThornWidth())
        {
            this->thornWidth->setValue(Ogre::Math::Clamp(XMLConverter::getAttribReal(propertyElement, "data", 2.0f), 0.1f, 1000.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralThornComponent::AttrThornHeight())
        {
            this->thornHeight->setValue(Ogre::Math::Clamp(XMLConverter::getAttribReal(propertyElement, "data", 1.0f), 0.05f, 100.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralThornComponent::AttrThornBaseSize())
        {
            this->thornBaseSize->setValue(Ogre::Math::Clamp(XMLConverter::getAttribReal(propertyElement, "data", 0.5f), 0.05f, 50.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralThornComponent::AttrDatablock())
        {
            this->datablock->setValue(XMLConverter::getAttrib(propertyElement, "data", "rockClif_D"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralThornComponent::AttrUVTiling())
        {
            this->uvTiling->setValue(XMLConverter::getAttribVector2(propertyElement, "data", Ogre::Vector2(1.0f, 1.0f)));
            propertyElement = propertyElement->next_sibling("property");
        }

        return true;
    }

    GameObjectCompPtr ProceduralThornComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        ProceduralThornCompPtr clonedCompPtr(boost::make_shared<ProceduralThornComponent>());

        clonedCompPtr->setOwner(clonedGameObjectPtr);

        clonedCompPtr->setThornLength(this->thornLength->getReal());
        clonedCompPtr->setThornWidth(this->thornWidth->getReal());
        clonedCompPtr->setThornHeight(this->thornHeight->getReal());
        clonedCompPtr->setThornBaseSize(this->thornBaseSize->getReal());
        clonedCompPtr->setDatablock(this->datablock->getString());
        clonedCompPtr->setUVTiling(this->uvTiling->getVector2());
        clonedCompPtr->setActivated(this->activated->getBool());

        clonedGameObjectPtr->addComponent(clonedCompPtr);

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));

        return clonedCompPtr;
    }

    bool ProceduralThornComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralThornComponent] Init thorn component for game object: " + this->gameObjectPtr->getName());

        this->thornMeshName = "ProceduralThornMesh_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());

        if (true == this->activated->getBool())
        {
            this->rebuildMesh();
        }

        return true;
    }

    void ProceduralThornComponent::onAddComponent(void)
    {
        // No edit focus / selection event to claim here, unlike the boundary and platform
        // components - there is nothing interactive on this component for another component to
        // fight over, so there is nothing to announce.
    }

    void ProceduralThornComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralThornComponent] Remove thorn component for game object: " + this->gameObjectPtr->getName());

        this->destroyThornMesh();
    }

    void ProceduralThornComponent::update(Ogre::Real dt, bool notSimulating)
    {
        // Nothing to do per frame: the strip is static geometry and every change is
        // property-driven. Kept as an explicit empty override so it is clear this is
        // intentional rather than forgotten.
    }

    void ProceduralThornComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (ProceduralThornComponent::AttrActivated() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
        }
        else if (ProceduralThornComponent::AttrThornLength() == attribute->getName())
        {
            this->setThornLength(attribute->getReal());
        }
        else if (ProceduralThornComponent::AttrThornWidth() == attribute->getName())
        {
            this->setThornWidth(attribute->getReal());
        }
        else if (ProceduralThornComponent::AttrThornHeight() == attribute->getName())
        {
            this->setThornHeight(attribute->getReal());
        }
        else if (ProceduralThornComponent::AttrThornBaseSize() == attribute->getName())
        {
            this->setThornBaseSize(attribute->getReal());
        }
        else if (ProceduralThornComponent::AttrDatablock() == attribute->getName())
        {
            this->setDatablock(attribute->getString());
        }
        else if (ProceduralThornComponent::AttrUVTiling() == attribute->getName())
        {
            this->setUVTiling(attribute->getVector2());
        }
    }

    void ProceduralThornComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        // 6 = real, 7 = string, 8 = vector2, 12 = bool
        GameObjectComponent::writeXML(propertiesXML, doc);

        xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralThornComponent::AttrActivated().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralThornComponent::AttrThornLength().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->thornLength->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralThornComponent::AttrThornWidth().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->thornWidth->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralThornComponent::AttrThornHeight().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->thornHeight->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralThornComponent::AttrThornBaseSize().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->thornBaseSize->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralThornComponent::AttrDatablock().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->datablock->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
        propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(ProceduralThornComponent::AttrUVTiling().c_str())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->uvTiling->getVector2())));
        propertiesXML->append_node(propertyXML);
    }

    // =========================================================================================
    // Mesh generation
    // =========================================================================================

    void ProceduralThornComponent::addThornTriangle(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& normal0, const Ogre::Vector3& normal1, const Ogre::Vector3& normal2,
        const Ogre::Vector3& tangent, const Ogre::Vector2& uv0, const Ogre::Vector2& uv1, const Ogre::Vector2& uv2)
    {
        // BUGFIX (design correction): this used to take ONE shared normal for the whole triangle,
        // fine for the old flat-shaded roof panels but wrong for a cone, whose lateral surface is
        // meant to look smoothly rounded rather than faceted - each of the three corners now
        // carries its OWN normal (see addCone for how each is derived), and Ogre interpolates
        // between them across the triangle exactly like it would for an authored round mesh.
        const Ogre::Vector3 positions[3] = {v0, v1, v2};
        const Ogre::Vector3 normals[3] = {normal0, normal1, normal2};
        const Ogre::Vector2 uvs[3] = {uv0, uv1, uv2};

        for (int i = 0; i < 3; ++i)
        {
            this->vertices.push_back(positions[i].x);
            this->vertices.push_back(positions[i].y);
            this->vertices.push_back(positions[i].z);
            this->vertices.push_back(normals[i].x);
            this->vertices.push_back(normals[i].y);
            this->vertices.push_back(normals[i].z);
            this->vertices.push_back(tangent.x);
            this->vertices.push_back(tangent.y);
            this->vertices.push_back(tangent.z);
            this->vertices.push_back(1.0f); // handedness
            this->vertices.push_back(uvs[i].x);
            this->vertices.push_back(uvs[i].y);
        }

        this->indices.push_back(this->currentVertexIndex + 0);
        this->indices.push_back(this->currentVertexIndex + 1);
        this->indices.push_back(this->currentVertexIndex + 2);

        this->currentVertexIndex += 3;
    }

    void ProceduralThornComponent::addThornQuad(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& v3, const Ogre::Vector3& normal, Ogre::Real u0, Ogre::Real u1, Ogre::Real v0Coord,
        Ogre::Real v1Coord)
    {
        // Winding derived from the normal rather than worked out by hand per face - same
        // self-correcting approach as ProceduralBlockComponent::addBlockQuad, used here
        // deliberately instead of repeating addCone's original approach (asserting the winding
        // order directly), which needed two rounds of fixing before its normals actually pointed
        // outward.
        const Ogre::Vector3 edge1 = v1 - v0;
        const Ogre::Vector3 edge2 = v3 - v0;
        const bool flip = edge1.crossProduct(edge2).dotProduct(normal) < 0.0f;

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

    namespace
    {
        // Triangles per cone. 10 reads as a proper round, pointed cone at the small sizes these
        // spikes are built at (around half a meter), without generating an excessive triangle
        // count once many cones tile a whole field - a 5 x 2m field at the default 0.5m base size
        // is a 10 x 4 grid, 400 triangles total at this segment count.
        const int THORN_CONE_SEGMENTS = 10;
    }

    void ProceduralThornComponent::addCone(Ogre::Real centerX, Ogre::Real centerZ, Ogre::Real height, Ogre::Real radius)
    {
        // A right circular cone is a "ruled surface": every straight line from a base point to
        // the apex (a generator) has a CONSTANT surface normal along its whole length. For a
        // generator at angle theta, that normal works out to
        //     normalize(height * cos(theta), radius, height * sin(theta))
        // - derived directly from the cross product of the generator direction and the base
        // circle's own tangent direction at theta, which is also exactly the vector this UVs the
        // surface with as its "around the cone" tangent below. Two BASE vertices at adjacent
        // angles therefore already have the correct, DIFFERENT normals for a smoothly rounded
        // surface; the shared APEX point does not have a single correct normal (it is a singular
        // point of the cone), so each wedge's own copy of it uses the MIDPOINT angle's normal -
        // splitting the difference between its two neighbouring generators, which is what makes
        // adjacent wedges blend into a round-looking point rather than a faceted star.
        const Ogre::Vector2 uvTile = this->uvTiling->getVector2();
        const Ogre::Real circumference = Ogre::Math::TWO_PI * radius;
        const Ogre::Real slantLength = std::sqrt(radius * radius + height * height);
        const Ogre::Real vApex = slantLength * uvTile.y;

        for (int i = 0; i < THORN_CONE_SEGMENTS; ++i)
        {
            const Ogre::Real theta0 = (Ogre::Math::TWO_PI * static_cast<Ogre::Real>(i)) / static_cast<Ogre::Real>(THORN_CONE_SEGMENTS);
            const Ogre::Real theta1 = (Ogre::Math::TWO_PI * static_cast<Ogre::Real>(i + 1)) / static_cast<Ogre::Real>(THORN_CONE_SEGMENTS);
            const Ogre::Real thetaMid = (theta0 + theta1) * 0.5f;

            const Ogre::Vector3 base0(centerX + radius * std::cos(theta0), 0.0f, centerZ + radius * std::sin(theta0));
            const Ogre::Vector3 base1(centerX + radius * std::cos(theta1), 0.0f, centerZ + radius * std::sin(theta1));
            const Ogre::Vector3 apex(centerX, height, centerZ);

            Ogre::Vector3 normal0(height * std::cos(theta0), radius, height * std::sin(theta0));
            normal0.normalise();
            Ogre::Vector3 normal1(height * std::cos(theta1), radius, height * std::sin(theta1));
            normal1.normalise();
            Ogre::Vector3 normalMid(height * std::cos(thetaMid), radius, height * std::sin(thetaMid));
            normalMid.normalise();

            // Tangent = the base circle's own tangent direction at the wedge's midpoint angle -
            // the "around the cone" texture axis, matching how the U coordinate below is laid
            // out.
            Ogre::Vector3 tangent(-std::sin(thetaMid), 0.0f, std::cos(thetaMid));

            const Ogre::Real u0 = (circumference * static_cast<Ogre::Real>(i)) / static_cast<Ogre::Real>(THORN_CONE_SEGMENTS) * uvTile.x;
            const Ogre::Real u1 = (circumference * static_cast<Ogre::Real>(i + 1)) / static_cast<Ogre::Real>(THORN_CONE_SEGMENTS) * uvTile.x;
            const Ogre::Real uMid = (u0 + u1) * 0.5f;

            // WINDING FIX: base0/base1 (and their matching normal/uv) swapped compared to before.
            // The old order (base0, base1, apex) produced a cross(edge1, edge2) that pointed
            // INWARD instead of outward, which under Ogre's default CULL_CLOCKWISE mode makes
            // this triangle a back face - it got culled from outside and was only visible from
            // inside the cone. Swapping two of the three vertices reverses the winding without
            // touching a single normal.
            this->addThornTriangle(base1, base0, apex, normal1, normal0, normalMid, tangent, Ogre::Vector2(u1, 0.0f), Ogre::Vector2(u0, 0.0f), Ogre::Vector2(uMid, vApex));
        }
    }

    void ProceduralThornComponent::addBasePlane(Ogre::Real length, Ogre::Real halfWidth)
    {
        // A single flat quad at y = 0, spanning the whole grid's footprint - the surface the
        // spikes visually sit on. Same datablock and UV Tiling as the cones (this->datablock,
        // this->uvTiling - nothing new added for it), so it becomes part of the SAME submesh
        // rather than a second material.
        const Ogre::Vector2 uvTile = this->uvTiling->getVector2();
        const Ogre::Vector3 normal(0.0f, 1.0f, 0.0f);

        this->addThornQuad(Ogre::Vector3(0.0f, 0.0f, -halfWidth), Ogre::Vector3(length, 0.0f, -halfWidth), Ogre::Vector3(length, 0.0f, halfWidth), Ogre::Vector3(0.0f, 0.0f, halfWidth), normal, 0.0f, length * uvTile.x, 0.0f,
            (2.0f * halfWidth) * uvTile.y);
    }

    void ProceduralThornComponent::rebuildMesh(void)
    {
        this->vertices.clear();
        this->indices.clear();
        this->currentVertexIndex = 0;

        const Ogre::Real length = this->thornLength->getReal();
        const Ogre::Real width = this->thornWidth->getReal();
        const Ogre::Real height = this->thornHeight->getReal();
        const Ogre::Real baseSize = this->thornBaseSize->getReal();

        // Same "last cell absorbs the remainder" approach as
        // ProceduralPlatformBoundaryComponent::getCellCount(), applied independently on BOTH grid
        // axes: Base Size decides roughly how many cones fit, then the actual spacing on each
        // axis is recomputed so the cones fill the stated Length and Width exactly, rather than
        // falling short by a partial cone.
        const int countX = std::max(1, static_cast<int>(std::floor(length / baseSize + 0.5f)));
        const int countZ = std::max(1, static_cast<int>(std::floor(width / baseSize + 0.5f)));
        const Ogre::Real spacingX = length / static_cast<Ogre::Real>(countX);
        const Ogre::Real spacingZ = width / static_cast<Ogre::Real>(countZ);

        // Cones sit base-to-base: radius is half of whichever spacing is smaller, so a cone never
        // overlaps its neighbour on the tighter axis even when Length and Width do not divide
        // into perfectly square cells.
        const Ogre::Real radius = std::min(spacingX, spacingZ) * 0.5f;

        // Base plane FIRST, spikes after - purely for readability (build order has no effect on
        // the result, since both go into the same single submesh with the same datablock).
        this->addBasePlane(length, width * 0.5f);

        for (int ix = 0; ix < countX; ++ix)
        {
            const Ogre::Real centerX = (static_cast<Ogre::Real>(ix) + 0.5f) * spacingX;
            for (int iz = 0; iz < countZ; ++iz)
            {
                const Ogre::Real centerZ = -width * 0.5f + (static_cast<Ogre::Real>(iz) + 0.5f) * spacingZ;
                this->addCone(centerX, centerZ, height, radius);
            }
        }

        this->createThornMesh();
    }

    void ProceduralThornComponent::createThornMesh(void)
    {
        if (0 == this->currentVertexIndex)
        {
            this->destroyThornMesh();
            return;
        }

        std::vector<float> verticesCopy = this->vertices;
        std::vector<Ogre::uint32> indicesCopy = this->indices;
        const size_t numVertices = this->currentVertexIndex;

        GraphicsModule::RenderCommand renderCommand = [this, verticesCopy, indicesCopy, numVertices]() { this->createThornMeshInternal(verticesCopy, indicesCopy, numVertices); };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralThornComponent::createThornMesh");

        this->updatePhysicsCollision();
    }

    void ProceduralThornComponent::createThornMeshInternal(const std::vector<float>& verts, const std::vector<Ogre::uint32>& inds, size_t numVerts)
    {
        //  RUNS ON RENDER THREAD!
        Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();
        Ogre::VaoManager* vaoManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getVaoManager();

        if (nullptr != this->thornItem)
        {
            this->gameObjectPtr->getSceneNode()->detachObject(this->thornItem);
            sceneManager->destroyItem(this->thornItem);
            this->thornItem = nullptr;
            this->gameObjectPtr->nullMovableObject();
        }

        {
            Ogre::ResourcePtr existing = Ogre::MeshManager::getSingleton().getByName(this->thornMeshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
            if (false == existing.isNull())
            {
                Ogre::MeshManager::getSingleton().remove(existing->getHandle());
            }
        }

        Ogre::MeshPtr mesh = Ogre::MeshManager::getSingleton().createManual(this->thornMeshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, &NOWA::gDummyMeshLoader);
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
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralThornComponent] Failed to create buffers: " + e.getDescription());
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

        this->thornItem = sceneManager->createItem(mesh, this->gameObjectPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC);
        this->thornItem->setName(this->gameObjectPtr->getName());
        this->thornItem->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);
        this->thornItem->setQueryFlags(this->gameObjectPtr->getCategoryId());
        this->thornItem->setCastShadows(true);

        const Ogre::String dbName = this->datablock->getString();
        if (false == dbName.empty() && this->thornItem->getNumSubItems() > 0u)
        {
            Ogre::HlmsDatablock* db = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(dbName);
            if (nullptr != db)
            {
                this->thornItem->getSubItem(0u)->setDatablock(db);
            }
        }

        this->gameObjectPtr->getSceneNode()->attachObject(this->thornItem);

        // Registers this item as the game object's own movable object, exactly like
        // ProceduralPlatformBoundaryComponent does - without this the object cannot be selected
        // in the editor, because its bounding box never gets picked up.
        this->gameObjectPtr->setDoNotDestroyMovableObject(true);
        this->gameObjectPtr->init(this->thornItem);

        if (false == this->gameObjectPtr->isDynamic())
        {
            sceneManager->notifyStaticAabbDirty(this->thornItem);
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralThornComponent] Thorn mesh created: " + Ogre::StringConverter::toString(static_cast<unsigned int>(numVerts)) + " vertices.");
    }

    void ProceduralThornComponent::destroyThornMesh(void)
    {
        GraphicsModule::RenderCommand renderCommand = [this]()
        {
            if (nullptr == this->thornItem)
            {
                return;
            }

            Ogre::SceneManager* sceneManager = this->gameObjectPtr->getSceneManager();

            if (nullptr != this->gameObjectPtr->getSceneNode())
            {
                this->gameObjectPtr->getSceneNode()->detachObject(this->thornItem);
            }
            sceneManager->destroyItem(this->thornItem);
            this->thornItem = nullptr;
            this->gameObjectPtr->nullMovableObject();

            Ogre::ResourcePtr existing = Ogre::MeshManager::getSingleton().getByName(this->thornMeshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
            if (false == existing.isNull())
            {
                Ogre::MeshManager::getSingleton().remove(existing->getHandle());
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralThornComponent::destroyThornMesh");
    }

    void ProceduralThornComponent::updatePhysicsCollision(void)
    {
        if (nullptr == this->thornItem)
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

    void ProceduralThornComponent::setActivated(bool activated)
    {
        this->activated->setValue(activated);

        if (true == activated)
        {
            this->rebuildMesh();
        }
        else
        {
            this->destroyThornMesh();
        }
    }

    bool ProceduralThornComponent::isActivated(void) const
    {
        return this->activated->getBool();
    }

    void ProceduralThornComponent::setThornLength(Ogre::Real length)
    {
        this->thornLength->setValue(Ogre::Math::Clamp(length, 0.1f, 1000.0f));
        this->rebuildMesh();
    }

    Ogre::Real ProceduralThornComponent::getThornLength(void) const
    {
        return this->thornLength->getReal();
    }

    void ProceduralThornComponent::setThornWidth(Ogre::Real width)
    {
        this->thornWidth->setValue(Ogre::Math::Clamp(width, 0.1f, 1000.0f));
        this->rebuildMesh();
    }

    Ogre::Real ProceduralThornComponent::getThornWidth(void) const
    {
        return this->thornWidth->getReal();
    }

    void ProceduralThornComponent::setThornHeight(Ogre::Real height)
    {
        this->thornHeight->setValue(Ogre::Math::Clamp(height, 0.05f, 100.0f));
        this->rebuildMesh();
    }

    Ogre::Real ProceduralThornComponent::getThornHeight(void) const
    {
        return this->thornHeight->getReal();
    }

    void ProceduralThornComponent::setThornBaseSize(Ogre::Real baseSize)
    {
        this->thornBaseSize->setValue(Ogre::Math::Clamp(baseSize, 0.05f, 50.0f));
        this->rebuildMesh();
    }

    Ogre::Real ProceduralThornComponent::getThornBaseSize(void) const
    {
        return this->thornBaseSize->getReal();
    }

    void ProceduralThornComponent::setDatablock(const Ogre::String& datablock)
    {
        this->datablock->setValue(datablock);
        this->rebuildMesh();
    }

    Ogre::String ProceduralThornComponent::getDatablock(void) const
    {
        return this->datablock->getString();
    }

    void ProceduralThornComponent::setUVTiling(const Ogre::Vector2& tiling)
    {
        this->uvTiling->setValue(tiling);
        this->rebuildMesh();
    }

    Ogre::Vector2 ProceduralThornComponent::getUVTiling(void) const
    {
        return this->uvTiling->getVector2();
    }

    // =========================================================================================
    // Lua API
    // =========================================================================================

    ProceduralThornComponent* getProceduralThornComponent(GameObject* gameObject, unsigned int occurrenceIndex)
    {
        return makeStrongPtr<ProceduralThornComponent>(gameObject->getComponentWithOccurrence<ProceduralThornComponent>(occurrenceIndex)).get();
    }

    ProceduralThornComponent* getProceduralThornComponent(GameObject* gameObject)
    {
        return makeStrongPtr<ProceduralThornComponent>(gameObject->getComponent<ProceduralThornComponent>()).get();
    }

    ProceduralThornComponent* getProceduralThornComponentFromName(GameObject* gameObject, const Ogre::String& name)
    {
        return makeStrongPtr<ProceduralThornComponent>(gameObject->getComponentFromName<ProceduralThornComponent>(name)).get();
    }

    void ProceduralThornComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
    {
        module(lua)[class_<ProceduralThornComponent, GameObjectComponent>("ProceduralThornComponent")
                .def("setActivated", &ProceduralThornComponent::setActivated)
                .def("isActivated", &ProceduralThornComponent::isActivated)
                .def("setThornLength", &ProceduralThornComponent::setThornLength)
                .def("getThornLength", &ProceduralThornComponent::getThornLength)
                .def("setThornWidth", &ProceduralThornComponent::setThornWidth)
                .def("getThornWidth", &ProceduralThornComponent::getThornWidth)
                .def("setThornHeight", &ProceduralThornComponent::setThornHeight)
                .def("getThornHeight", &ProceduralThornComponent::getThornHeight)
                        .def("setThornBaseSize", &ProceduralThornComponent::setThornBaseSize)
                        .def("getThornBaseSize", &ProceduralThornComponent::getThornBaseSize)
                .def("setDatablock", &ProceduralThornComponent::setDatablock)
                .def("getDatablock", &ProceduralThornComponent::getDatablock)];

        LuaScriptApi::getInstance()->addClassToCollection("ProceduralThornComponent", "class inherits GameObjectComponent", ProceduralThornComponent::getStaticInfoText());

        LuaScriptApi::getInstance()->addClassToCollection("ProceduralThornComponent", "void setThornLength(float length)", "Sets the spike strip's length in meters. Regenerates the whole strip.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralThornComponent", "float getThornLength()", "Gets the spike strip's length in meters.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralThornComponent", "void setThornWidth(float width)", "Sets the spike strip's depth in meters. Regenerates the whole strip.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralThornComponent", "float getThornWidth()", "Gets the spike strip's depth in meters.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralThornComponent", "void setThornHeight(float height)", "Sets each spike's height in meters. Regenerates the whole strip.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralThornComponent", "float getThornHeight()", "Gets each spike's height in meters.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralThornComponent", "void setThornBaseSize(float baseSize)", "Sets each cone's base diameter in meters, also used as the grid spacing. Regenerates the whole field.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralThornComponent", "float getThornBaseSize()", "Gets each cone's base diameter in meters.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralThornComponent", "void setDatablock(String datablock)", "Sets the single datablock used for the whole strip.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralThornComponent", "String getDatablock()", "Gets the strip's datablock.");

        gameObjectClass.def("getProceduralThornComponentFromName", &getProceduralThornComponentFromName);
        gameObjectClass.def("getProceduralThornComponent", (ProceduralThornComponent * (*)(GameObject*)) & getProceduralThornComponent);
        gameObjectClass.def("getProceduralThornComponent2", (ProceduralThornComponent * (*)(GameObject*, unsigned int)) & getProceduralThornComponent);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ProceduralThornComponent getProceduralThornComponent()", "Gets the component. Use this if the game object has this component only once.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ProceduralThornComponent getProceduralThornComponent2(unsigned int occurrenceIndex)",
            "Gets the component by the given occurrence index, since a game object may have this component several times.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ProceduralThornComponent getProceduralThornComponentFromName(String name)", "Gets the component by its custom name.");

        gameObjectControllerClass.def("castProceduralThornComponent", &GameObjectController::cast<ProceduralThornComponent>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "ProceduralThornComponent castProceduralThornComponent(ProceduralThornComponent other)", "Casts an incoming type from function for lua auto completion.");
    }

}; // namespace end
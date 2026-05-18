#include "NOWAPrecompiled.h"
#include "ProceduralGeometryComponent.h"
#include "editor/EditorManager.h"
#include "gameObject/GameObjectController.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "main/InputDeviceCore.h"
#include "utilities/MathHelper.h"
#include "utilities/XMLConverter.h"

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
#include <filesystem>
#include <system_error>

#include "OgreAbiUtils.h"

namespace NOWA
{
    using namespace rapidxml;

    // =========================================================================
    //  Constants
    // =========================================================================

    static const Ogre::String PREVIEW_DATABLOCK = "BaseWhiteNoLighting";
    static const float PREVIEW_ALPHA = 0.45f; // ghost transparency (PBS alpha-blend)

    // =========================================================================
    //  Constructor / Destructor
    // =========================================================================

    ProceduralGeometryComponent::ProceduralGeometryComponent()
        : GeometricComponentBase(),
        name("ProceduralGeometryComponent"),
        activated(nullptr),
        shape(nullptr),
        size(nullptr),
        segmentsH(nullptr),
        segmentsV(nullptr),
        flipNormals(nullptr),
        datablock(nullptr),
        uvTiling(nullptr),
        convertToMesh(nullptr),
        vertBase(0u),
        geomMesh(nullptr),
        geomItem(nullptr),
        previewMesh(nullptr),
        previewItem(nullptr),
        previewNode(nullptr),
        physicsArtifactComponent(nullptr)
    {
        // ── Activation ────────────────────────────────────────────────────────
        activated = new Variant(ProceduralGeometryComponent::AttrActivated(), true, this->attributes);

        // ── Shape list ────────────────────────────────────────────────────────
        std::vector<Ogre::String> shapes;
        shapes.push_back("Box");
        shapes.push_back("Pyramid");
        shapes.push_back("Sphere");
        shapes.push_back("Cylinder");
        shapes.push_back("Cone");
        shapes.push_back("Capsule");
        shapes.push_back("Torus");
        shapes.push_back("Plane");
        shapes.push_back("Prism");
        shapes.push_back("Disc");
        shapes.push_back("Wedge");
        shape = new Variant(ProceduralGeometryComponent::AttrShape(), shapes, this->attributes);

        // ── Dimensions ────────────────────────────────────────────────────────
        size = new Variant(ProceduralGeometryComponent::AttrSize(), Ogre::Vector3(1.0f, 1.0f, 1.0f), this->attributes);

        // ── Tessellation ──────────────────────────────────────────────────────
        segmentsH = new Variant(ProceduralGeometryComponent::AttrSegmentsH(), 16.0f, this->attributes);
        segmentsV = new Variant(ProceduralGeometryComponent::AttrSegmentsV(), 8.0f, this->attributes);

        // ── Normal flip ───────────────────────────────────────────────────────
        flipNormals = new Variant(ProceduralGeometryComponent::AttrFlipNormals(), false, this->attributes);

        // ── Material ──────────────────────────────────────────────────────────
        datablock = new Variant(ProceduralGeometryComponent::AttrDatablock(), Ogre::String("proceduralGeometry1"), this->attributes);

        // ── UV tiling ─────────────────────────────────────────────────────────
        uvTiling = new Variant(ProceduralGeometryComponent::AttrUVTiling(), Ogre::Vector2(1.0f, 1.0f), this->attributes);

        // ── Convert-to-mesh action ────────────────────────────────────────────
        convertToMesh = new Variant(ProceduralGeometryComponent::AttrConvertToMesh(), Ogre::String("Convert to Mesh"), this->attributes);

        // ── Descriptions ──────────────────────────────────────────────────────
        activated->setDescription("Activate / deactivate geometry placement mode.");
        shape->setDescription("Primitive shape to generate. "
                                "Box      : Size = width x height x depth "
                                "Pyramid  : Size = baseW x height x baseD "
                                "Sphere   : Size.x = radius (Y/Z ignored) "
                                "Cylinder : Size.x = radius, Size.y = height "
                                "Cone     : Size.x = radius, Size.y = height "
                                "Capsule  : Size.x = radius, Size.y = body height (caps add 2xradius) "
                                "Torus    : Size.x = major radius, Size.z = tube radius "
                                "Plane    : Size.x = width, Size.z = depth "
                                "Prism    : Size = baseW x height x baseD (triangular cross-section) "
                                "Disc     : Size.x = outer radius, Size.z = inner radius (0 = solid) "
                                "Wedge    : Size = width x height x depth (inclined ramp)");
        size->setDescription("Shape dimensions.  Interpretation depends on the selected Shape (see Shape tooltip).");
        segmentsH->setDescription("Horizontal (angular / width) tessellation.  Min = 3 for round shapes.");
        segmentsV->setDescription("Vertical tessellation.  Used by Sphere, Plane, Capsule.  Min = 2.");
        flipNormals->setDescription("Invert face winding.  Useful for inside-out geometry (e.g. a skybox cube).");
        datablock->setDescription("PBS datablock name used for this geometry.");
        uvTiling->setDescription("UV tiling multiplier applied to all faces  (x = U repeat, y = V repeat).");

        convertToMesh->setDescription("Bakes this procedural geometry to a static .mesh file and replaces the component "
                                        "with a plain MeshComponent for optimal runtime performance. "
                                        "THIS IS A ONE-WAY, IRREVERSIBLE OPERATION.");
        convertToMesh->addUserData(GameObject::AttrActionExec());
        convertToMesh->addUserData(GameObject::AttrActionNeedRefresh());
        convertToMesh->addUserData(GameObject::AttrActionExecId(), "ProceduralGeometryComponent.ConvertToMesh");

        // The shape list triggers an immediate mesh rebuild
        shape->addUserData(GameObject::AttrActionNeedRefresh());
    }

    ProceduralGeometryComponent::~ProceduralGeometryComponent()
    {
    }

    // =========================================================================
    //  Ogre::Plugin interface
    // =========================================================================

    void ProceduralGeometryComponent::install(const Ogre::NameValuePairList* options)
    {
        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<ProceduralGeometryComponent>(ProceduralGeometryComponent::getStaticClassId(), ProceduralGeometryComponent::getStaticClassName());
    }

    void ProceduralGeometryComponent::initialise()
    {
    }
    void ProceduralGeometryComponent::shutdown()
    {
    }
    void ProceduralGeometryComponent::uninstall()
    {
    }

    const Ogre::String& ProceduralGeometryComponent::getName() const
    {
        return name;
    }

    void ProceduralGeometryComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    // =========================================================================
    //  init  – read from XML
    // =========================================================================

    bool ProceduralGeometryComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralGeometryComponent::AttrActivated())
        {
            activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralGeometryComponent::AttrShape())
        {
            shape->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralGeometryComponent::AttrSize())
        {
            size->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(1.0f)));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralGeometryComponent::AttrSegmentsH())
        {
            segmentsH->setValue(XMLConverter::getAttribReal(propertyElement, "data", 16.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralGeometryComponent::AttrSegmentsV())
        {
            segmentsV->setValue(XMLConverter::getAttribReal(propertyElement, "data", 8.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralGeometryComponent::AttrFlipNormals())
        {
            flipNormals->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralGeometryComponent::AttrDatablock())
        {
            datablock->setValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralGeometryComponent::AttrUVTiling())
        {
            uvTiling->setValue(XMLConverter::getAttribVector2(propertyElement, "data", Ogre::Vector2(1.0f)));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralGeometryComponent::AttrConvertToMesh())
        {
            // Skip action attribute - it stores only the button label
            propertyElement = propertyElement->next_sibling("property");
        }

        return true;
    }

    // =========================================================================
    //  postInit  – subscribe events, create scene objects, rebuild mesh
    // =========================================================================

    bool ProceduralGeometryComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralGeometryComponent] Init component for game object: " + this->gameObjectPtr->getName());

        // ── Preview scene node (always exists; item attached on demand) ────────
        NOWA::GraphicsModule::RenderCommand makeNode = [this]()
        {
            previewNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();
            previewNode->setVisible(false);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(makeNode), "ProceduralGeometryComponent::postInit_previewNode");

        // ── Build geometry from loaded (or default) parameters ────────────────
        this->rebuildMesh();

        return true;
    }

    bool ProceduralGeometryComponent::connect(void)
    {
        return true;
    }

    bool ProceduralGeometryComponent::disconnect(void)
    {
        this->destroyPreviewMesh();
        return true;
    }

    bool ProceduralGeometryComponent::onCloned(void)
    {
        return true;
    }

    // ── When the component is added in the editor → enter placement mode ─────
    void ProceduralGeometryComponent::onAddComponent(void)
    {

    }

    void ProceduralGeometryComponent::onRemoveComponent(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralGeometryComponent] onRemoveComponent");

        this->destroyPreviewMesh();
        this->destroyGeometryMesh();

        NOWA::GraphicsModule::RenderCommand destroyNode = [this]()
        {
            if (previewNode)
            {
                this->gameObjectPtr->getSceneManager()->destroySceneNode(previewNode);
                previewNode = nullptr;
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(destroyNode), "ProceduralGeometryComponent::onRemoveComponent_node");

        physicsArtifactComponent = nullptr;
    }

    void ProceduralGeometryComponent::onOtherComponentRemoved(unsigned int index)
    {
        if (nullptr != physicsArtifactComponent && index == physicsArtifactComponent->getIndex())
        {
            physicsArtifactComponent = nullptr;
        }
    }

    void ProceduralGeometryComponent::onOtherComponentAdded(unsigned int index)
    {
    }

    // =========================================================================
    //  clone
    // =========================================================================

    GameObjectCompPtr ProceduralGeometryComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        ProceduralGeometryComponentPtr cloned(boost::make_shared<ProceduralGeometryComponent>());

        cloned->setActivated(activated->getBool());
        cloned->setShape(shape->getListSelectedValue());
        cloned->setSize(size->getVector3());
        cloned->setSegmentsH(segmentsH->getReal());
        cloned->setSegmentsV(segmentsV->getReal());
        cloned->setFlipNormals(flipNormals->getBool());
        cloned->setDatablock(datablock->getString());
        cloned->setUVTiling(uvTiling->getVector2());

        clonedGameObjectPtr->addComponent(cloned);
        cloned->setOwner(clonedGameObjectPtr);

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(cloned));
        return cloned;
    }

    // =========================================================================
    //  update  – nothing to do per frame; mesh is rebuilt on demand
    // =========================================================================

    void ProceduralGeometryComponent::update(Ogre::Real dt, bool notSimulating)
    {
    }

    // =========================================================================
    //  actualizeValue  – property panel callback → rebuild mesh on changes
    // =========================================================================

    void ProceduralGeometryComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (ProceduralGeometryComponent::AttrActivated() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
        }
        else if (ProceduralGeometryComponent::AttrShape() == attribute->getName())
        {
            this->setShape(attribute->getListSelectedValue());
        }
        else if (ProceduralGeometryComponent::AttrSize() == attribute->getName())
        {
            this->setSize(attribute->getVector3());
        }
        else if (ProceduralGeometryComponent::AttrSegmentsH() == attribute->getName())
        {
            this->setSegmentsH(attribute->getReal());
        }
        else if (ProceduralGeometryComponent::AttrSegmentsV() == attribute->getName())
        {
            this->setSegmentsV(attribute->getReal());
        }
        else if (ProceduralGeometryComponent::AttrFlipNormals() == attribute->getName())
        {
            this->setFlipNormals(attribute->getBool());
        }
        else if (ProceduralGeometryComponent::AttrDatablock() == attribute->getName())
        {
            this->setDatablock(attribute->getString());
        }
        else if (ProceduralGeometryComponent::AttrUVTiling() == attribute->getName())
        {
            this->setUVTiling(attribute->getVector2());
        }
    }

    // =========================================================================
    //  writeXML
    // =========================================================================

    void ProceduralGeometryComponent::writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc)
    {
        GameObjectComponent::writeXML(propertiesXML, doc);

        auto appendBool = [&](const Ogre::String& attrName, bool val)
        {
            xml_node<>* node = doc.allocate_node(node_element, "property");
            node->append_attribute(doc.allocate_attribute("type", "12"));
            node->append_attribute(doc.allocate_attribute("name", doc.allocate_string(attrName.c_str())));
            node->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, val)));
            propertiesXML->append_node(node);
        };
        auto appendReal = [&](const Ogre::String& attrName, Ogre::Real val)
        {
            xml_node<>* node = doc.allocate_node(node_element, "property");
            node->append_attribute(doc.allocate_attribute("type", "6"));
            node->append_attribute(doc.allocate_attribute("name", doc.allocate_string(attrName.c_str())));
            node->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, val)));
            propertiesXML->append_node(node);
        };
        auto appendStr = [&](const Ogre::String& attrName, const Ogre::String& val)
        {
            xml_node<>* node = doc.allocate_node(node_element, "property");
            node->append_attribute(doc.allocate_attribute("type", "7"));
            node->append_attribute(doc.allocate_attribute("name", doc.allocate_string(attrName.c_str())));
            node->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, val)));
            propertiesXML->append_node(node);
        };
        auto appendV2 = [&](const Ogre::String& attrName, const Ogre::Vector2& val)
        {
            xml_node<>* node = doc.allocate_node(node_element, "property");
            node->append_attribute(doc.allocate_attribute("type", "8"));
            node->append_attribute(doc.allocate_attribute("name", doc.allocate_string(attrName.c_str())));
            node->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, val)));
            propertiesXML->append_node(node);
        };
        auto appendV3 = [&](const Ogre::String& attrName, const Ogre::Vector3& val)
        {
            xml_node<>* node = doc.allocate_node(node_element, "property");
            node->append_attribute(doc.allocate_attribute("type", "9"));
            node->append_attribute(doc.allocate_attribute("name", doc.allocate_string(attrName.c_str())));
            node->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, val)));
            propertiesXML->append_node(node);
        };

        appendBool(AttrActivated(), activated->getBool());
        appendStr(AttrShape(), shape->getListSelectedValue());
        appendV3(AttrSize(), size->getVector3());
        appendReal(AttrSegmentsH(), segmentsH->getReal());
        appendReal(AttrSegmentsV(), segmentsV->getReal());
        appendBool(AttrFlipNormals(), flipNormals->getBool());
        appendStr(AttrDatablock(), datablock->getString());
        appendV2(AttrUVTiling(), uvTiling->getVector2());
        // convertToMesh is an action button - no data to persist
    }

    std::vector<GeometricComponentBase::ConvexPart> ProceduralGeometryComponent::getConvexParts(void) const
    {
        using CP = GeometricComponentBase::ConvexPart;
        std::vector<CP> parts;

        const Ogre::Vector3 sz = size->getVector3();

        switch (this->getShapeEnum())
        {
        // ── Primitives that map 1-to-1 onto OgreNewt collision types ─────────────
        case GeometryShape::BOX:
        {
            CP p;
            p.type = "Box";
            p.size = sz; // width x height x depth  (direct match)
            parts.push_back(p);
            break;
        }
        case GeometryShape::SPHERE:
        {
            CP p;
            p.type = "Ellipsoid";
            p.size = Ogre::Vector3(sz.x, sz.x, sz.x); // uniform radius
            parts.push_back(p);
            break;
        }
        case GeometryShape::CYLINDER:
        {
            CP p;
            p.type = "Cylinder";
            p.size = Ogre::Vector3(sz.x, sz.y, sz.x); // radius x height x radius
            parts.push_back(p);
            break;
        }
        case GeometryShape::CONE:
        {
            CP p;
            p.type = "Cone";
            p.size = Ogre::Vector3(sz.x, sz.y, sz.x); // radius x height x radius
            parts.push_back(p);
            break;
        }
        case GeometryShape::CAPSULE:
        {
            CP p;
            p.type = "Capsule";
            p.size = Ogre::Vector3(sz.x, sz.y, sz.x); // radius x bodyHeight x radius
            parts.push_back(p);
            break;
        }
        case GeometryShape::PLANE:
        {
            // Represent as a very thin box; 0.05 avoids degenerate Newton collisions
            CP p;
            p.type = "Box";
            p.size = Ogre::Vector3(sz.x, 0.05f, sz.z);
            parts.push_back(p);
            break;
        }
        case GeometryShape::DISC:
        {
            // Disc → thin cylinder; sz.y is the user-set thickness (min-clamped)
            CP p;
            p.type = "Cylinder";
            p.size = Ogre::Vector3(sz.x, std::max(0.05f, sz.y), sz.x);
            parts.push_back(p);
            break;
        }

        // ── Shapes that need a ConvexHull (no matching OgreNewt primitive) ────────
        case GeometryShape::PYRAMID:
        {
            // 4 base corners + 1 apex
            const float hx = sz.x * 0.5f;
            const float hy = sz.y;
            const float hz = sz.z * 0.5f;

            CP p;
            p.type = "ConvexHull";
            p.vertices = {{-hx, 0.0f, -hz}, {hx, 0.0f, -hz}, {hx, 0.0f, hz}, {-hx, 0.0f, hz}, {0.0f, hy, 0.0f}};
            parts.push_back(p);
            break;
        }
        case GeometryShape::PRISM:
        {
            // Isosceles triangular prism: 3 bottom + 3 top
            const float hx = sz.x * 0.5f;
            const float hy = sz.y;
            const float hz = sz.z * 0.5f;

            CP p;
            p.type = "ConvexHull";
            p.vertices = {
                {-hx, 0.0f, -hz}, {hx, 0.0f, -hz}, {0.0f, 0.0f, hz}, // bottom triangle
                {-hx, hy, -hz}, {hx, hy, -hz}, {0.0f, hy, hz}        // top triangle
            };
            parts.push_back(p);
            break;
        }
        case GeometryShape::WEDGE:
        {
            // 4 bottom verts + 2 top-back verts → inclined ramp
            const float hx = sz.x * 0.5f;
            const float hy = sz.y;
            const float hz = sz.z * 0.5f;

            CP p;
            p.type = "ConvexHull";
            p.vertices = {
                {-hx, 0.0f, hz}, {hx, 0.0f, hz},   // front-bottom
                {hx, 0.0f, -hz}, {-hx, 0.0f, -hz}, // back-bottom
                {-hx, hy, -hz}, {hx, hy, -hz}      // back-top  (sloped face)
            };
            parts.push_back(p);
            break;
        }

        // ── Torus: no single OgreNewt primitive; approximate with N cylinders ─────
        case GeometryShape::TORUS:
        {
            const float majorR = sz.x; // ring centre radius
            const float tubeR = sz.z;  // tube cross-section radius
            const int N = 8;           // number of cylinder segments

            // Arc length of one segment + a little overlap so no gaps in the physics
            const float segArcH = Ogre::Math::TWO_PI * majorR / N + tubeR;

            for (int i = 0; i < N; ++i)
            {
                const float angle = i * Ogre::Math::TWO_PI / N;
                const float c = std::cos(angle);
                const float s = std::sin(angle);

                CP p;
                p.type = "Cylinder";
                p.size = Ogre::Vector3(tubeR, segArcH, tubeR);
                p.position = Ogre::Vector3(majorR * c, 0.0f, majorR * s);
                // Rotate so the cylinder's height axis is tangent to the ring
                p.orientation = Ogre::Quaternion(Ogre::Radian(angle + Ogre::Math::HALF_PI), Ogre::Vector3::UNIT_Y);
                parts.push_back(p);
            }
            break;
        }

        default:
        {
            // Safe fallback: bounding box
            CP p;
            p.type = "Box";
            p.size = sz;
            parts.push_back(p);
            break;
        }
        }

        return parts;
    }

    bool ProceduralGeometryComponent::hasConvexParts(void) const
    {
        return true;
    }

    bool ProceduralGeometryComponent::canStaticAddComponent(GameObject* gameObject)
    {
        // Only addable through the editor (plugin-based component)
        return false;
    }

    // =========================================================================
    //  executeAction  – handles button actions from the property panel
    // =========================================================================

    bool ProceduralGeometryComponent::executeAction(const Ogre::String& actionId, NOWA::Variant* attribute)
    {
        if ("ProceduralGeometryComponent.ConvertToMesh" == actionId)
        {
            return this->convertToMeshApply();
        }
        return true;
    }

    // =========================================================================
    //  Geometry build pipeline
    // =========================================================================

    void ProceduralGeometryComponent::rebuildMesh(void)
    {
        this->destroyGeometryMesh();
        this->createGeometryMesh();
    }

    void ProceduralGeometryComponent::buildGeometry(void)
    {
        vertices.clear();
        indices.clear();
        this->vertBase = 0u;

        switch (this->getShapeEnum())
        {
        case GeometryShape::BOX:
            this->generateBox();
            break;
        case GeometryShape::PYRAMID:
            this->generatePyramid();
            break;
        case GeometryShape::SPHERE:
            this->generateSphere();
            break;
        case GeometryShape::CYLINDER:
            this->generateCylinder();
            break;
        case GeometryShape::CONE:
            this->generateCone();
            break;
        case GeometryShape::CAPSULE:
            this->generateCapsule();
            break;
        case GeometryShape::TORUS:
            this->generateTorus();
            break;
        case GeometryShape::PLANE:
            this->generatePlane();
            break;
        case GeometryShape::PRISM:
            this->generatePrism();
            break;
        case GeometryShape::DISC:
            this->generateDisc();
            break;
        case GeometryShape::WEDGE:
            this->generateWedge();
            break;
        default:
            this->generateBox();
            break;
        }
    }

    void ProceduralGeometryComponent::createGeometryMesh(void)
    {
        this->buildGeometry();

        if (vertices.empty() || indices.empty())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralGeometryComponent] buildGeometry produced no geometry!");
            return;
        }

        // Take a snapshot for the render-thread command capture
        std::vector<float> vertsCopy = vertices;
        std::vector<Ogre::uint32> idxsCopy = indices;
        const size_t numVerts = vertices.size() / 8u;
        const Ogre::String meshName = this->gameObjectPtr->getName() + "_Geom_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());

        NOWA::GraphicsModule::RenderCommand renderCmd = [this, vertsCopy, idxsCopy, numVerts, meshName]()
        {
            this->createGeometryMeshInternal(vertsCopy, idxsCopy, numVerts, meshName);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCmd), "ProceduralGeometryComponent::createGeometryMesh");
    }

    // ── Runs on render thread ──────────────────────────────────────────────────
    void ProceduralGeometryComponent::createGeometryMeshInternal(const std::vector<float>& srcVerts, const std::vector<Ogre::uint32>& srcIdx, size_t numVerts, const Ogre::String& meshName)
    {
        Ogre::Root* root = Ogre::Root::getSingletonPtr();
        Ogre::RenderSystem* renderSystem = root->getRenderSystem();
        Ogre::VaoManager* vaoManager = renderSystem->getVaoManager();
        const Ogre::String groupName = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

        // ── Remove stale mesh with the same name ──────────────────────────────
        {
            Ogre::MeshManager& mm = Ogre::MeshManager::getSingleton();
            Ogre::MeshPtr existing = mm.getByName(meshName, groupName);
            if (false == existing.isNull())
            {
                mm.remove(existing->getHandle());
            }
        }

        geomMesh = Ogre::MeshManager::getSingleton().createManual(meshName, groupName);

        // ── Vertex element declaration: pos3 + normal3 + tangent4 + uv2 ───────
        Ogre::VertexElement2Vec vertexElements;
        vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
        vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));
        vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT4, Ogre::VES_TANGENT));
        vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));

        // srcVerts: 8 floats/vertex (pos3 + normal3 + uv2)
        // dstVerts: 12 floats/vertex (pos3 + normal3 + tangent4 + uv2)
        const size_t srcStride = 8u;
        const size_t dstStride = 12u;

        Ogre::Vector3 minBounds(std::numeric_limits<float>::max());
        Ogre::Vector3 maxBounds(-std::numeric_limits<float>::max());

        // ── Build 12-float vertex array with normal-derived tangents ──────────
        const size_t dstBytes = numVerts * dstStride * sizeof(float);
        float* dstVerts = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(dstBytes, Ogre::MEMCATEGORY_GEOMETRY));

        for (size_t i = 0; i < numVerts; ++i)
        {
            const size_t src = i * srcStride;
            const size_t dst = i * dstStride;

            // Position
            dstVerts[dst + 0] = srcVerts[src + 0];
            dstVerts[dst + 1] = srcVerts[src + 1];
            dstVerts[dst + 2] = srcVerts[src + 2];

            Ogre::Vector3 pos(srcVerts[src + 0], srcVerts[src + 1], srcVerts[src + 2]);
            minBounds.makeFloor(pos);
            maxBounds.makeCeil(pos);

            // Normal
            Ogre::Vector3 n(srcVerts[src + 3], srcVerts[src + 4], srcVerts[src + 5]);
            dstVerts[dst + 3] = n.x;
            dstVerts[dst + 4] = n.y;
            dstVerts[dst + 5] = n.z;

            // Tangent – derived from normal (axis-aligned reference; good for PBS)
            Ogre::Vector3 t;
            if (std::abs(n.y) < 0.9f)
            {
                t = Ogre::Vector3::UNIT_Y.crossProduct(n).normalisedCopy();
            }
            else
            {
                t = n.crossProduct(Ogre::Vector3::UNIT_X).normalisedCopy();
            }

            dstVerts[dst + 6] = t.x;
            dstVerts[dst + 7] = t.y;
            dstVerts[dst + 8] = t.z;
            dstVerts[dst + 9] = 1.0f; // handedness

            // UV
            dstVerts[dst + 10] = srcVerts[src + 6];
            dstVerts[dst + 11] = srcVerts[src + 7];
        }

        // ── Vertex buffer ─────────────────────────────────────────────────────
        Ogre::VertexBufferPacked* vb = nullptr;
        try
        {
            vb = vaoManager->createVertexBuffer(vertexElements, numVerts, Ogre::BT_IMMUTABLE, dstVerts, true /*keepAsShadow*/);
        }
        catch (Ogre::Exception& e)
        {
            OGRE_FREE_SIMD(dstVerts, Ogre::MEMCATEGORY_GEOMETRY);
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralGeometryComponent] createVertexBuffer failed: " + e.getDescription());
            return;
        }

        // ── Index buffer ──────────────────────────────────────────────────────
        const size_t idxBytes = srcIdx.size() * sizeof(Ogre::uint32);
        Ogre::uint32* idxData = reinterpret_cast<Ogre::uint32*>(OGRE_MALLOC_SIMD(idxBytes, Ogre::MEMCATEGORY_GEOMETRY));
        memcpy(idxData, srcIdx.data(), idxBytes);

        Ogre::IndexBufferPacked* ib = nullptr;
        try
        {
            ib = vaoManager->createIndexBuffer(Ogre::IndexBufferPacked::IT_32BIT, srcIdx.size(), Ogre::BT_IMMUTABLE, idxData, true);
        }
        catch (Ogre::Exception& e)
        {
            OGRE_FREE_SIMD(idxData, Ogre::MEMCATEGORY_GEOMETRY);
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralGeometryComponent] createIndexBuffer failed: " + e.getDescription());
            return;
        }

        // ── VAO → SubMesh ─────────────────────────────────────────────────────
        Ogre::VertexBufferPackedVec vbVec;
        vbVec.push_back(vb);

        Ogre::VertexArrayObject* vao = vaoManager->createVertexArrayObject(vbVec, ib, Ogre::OT_TRIANGLE_LIST);

        Ogre::SubMesh* subMesh = geomMesh->createSubMesh();
        subMesh->mVao[Ogre::VpNormal].push_back(vao);
        subMesh->mVao[Ogre::VpShadow].push_back(vao);

        // ── Bounds ────────────────────────────────────────────────────────────
        if (minBounds.x == std::numeric_limits<float>::max())
        {
            minBounds = Ogre::Vector3(-1.0f);
            maxBounds = Ogre::Vector3(1.0f);
        }
        Ogre::Aabb aabb;
        aabb.setExtents(minBounds, maxBounds);
        geomMesh->_setBounds(aabb, false);
        geomMesh->_setBoundingSphereRadius(aabb.getRadius());

        // ── Create Item and attach to scene ───────────────────────────────────
        geomItem = this->gameObjectPtr->getSceneManager()->createItem(geomMesh, this->gameObjectPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC);

        // Apply datablock
        const Ogre::String& dbName = datablock->getString();
        if (false == dbName.empty())
        {
            Ogre::HlmsDatablock* db = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(dbName);
            if (nullptr != db)
            {
                geomItem->getSubItem(0)->setDatablock(db);
            }
        }

        this->gameObjectPtr->getSceneNode()->attachObject(geomItem);
        this->gameObjectPtr->setDoNotDestroyMovableObject(true);
        this->gameObjectPtr->init(geomItem);

        if (nullptr != physicsArtifactComponent)
        {
            physicsArtifactComponent->reCreateCollision();
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
            "[ProceduralGeometryComponent] Created '" + shape->getListSelectedValue() + "' mesh: " + Ogre::StringConverter::toString(numVerts) + " vertices, " + Ogre::StringConverter::toString(srcIdx.size() / 3u) + " triangles");
    }

    void ProceduralGeometryComponent::destroyGeometryMesh(void)
    {
        if (nullptr == geomItem && geomMesh.isNull())
        {
            return;
        }

        NOWA::GraphicsModule::RenderCommand cmd = [this]()
        {
            if (nullptr != geomItem)
            {
                if (geomItem->getParentSceneNode())
                {
                    geomItem->getParentSceneNode()->detachObject(geomItem);
                }
                this->gameObjectPtr->getSceneManager()->destroyItem(geomItem);
                geomItem = nullptr;
                this->gameObjectPtr->nullMovableObject();
            }
            if (false == geomMesh.isNull())
            {
                Ogre::MeshManager::getSingleton().remove(geomMesh->getHandle());
                geomMesh.reset();
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "ProceduralGeometryComponent::destroyGeometryMesh");
    }

    // =========================================================================
    //  Preview mesh
    // =========================================================================

    void ProceduralGeometryComponent::createPreviewMesh(void)
    {
        // ── Step 1: Build geometry into CPU buffers (main thread, safe) ───────────
        this->buildGeometry();

        if (vertices.empty() || indices.empty())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralGeometryComponent] createPreviewMesh: buildGeometry() produced "
                                                                                "no geometry — cannot create preview.");
            return;
        }

        // ── Step 2: Take value-copies for the render-thread lambda capture ────────
        // Never capture m_vertices / m_indices by reference — they may be cleared
        // before the render thread consumes them.
        const std::vector<float> vertsCopy = vertices;
        const std::vector<Ogre::uint32> idxsCopy = indices;
        const size_t numVerts = vertices.size() / 8u; // 8 floats/vertex

        const Ogre::String meshName = this->gameObjectPtr->getName() + "_GeomPrev_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());

        // ── Step 3: Everything that touches Ogre-Next goes on the render thread ───
        NOWA::GraphicsModule::RenderCommand cmd = [this, vertsCopy, idxsCopy, numVerts, meshName]()
        {
            // ── 3a: Ensure the preview scene node exists ──────────────────────────
            // previewNode is created in postInit(), but createPreviewMesh() is
            // called from mouseMoved() which is guaranteed post-postInit().
            // This guard handles any future edge-case where call order changes.
            if (nullptr == previewNode)
            {
                previewNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();
                previewNode->setVisible(false);

                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralGeometryComponent] createPreviewMesh: "
                                                                                   "m_previewNode created defensively on render thread.");
            }

            // ── 3b: Destroy any stale preview mesh/item from a previous call ──────
            if (nullptr != previewItem)
            {
                if (previewItem->getParentSceneNode())
                {
                    previewItem->getParentSceneNode()->detachObject(previewItem);
                }
                this->gameObjectPtr->getSceneManager()->destroyItem(previewItem);
                previewItem = nullptr;
            }
            if (false == previewMesh.isNull())
            {
                Ogre::MeshManager::getSingleton().remove(previewMesh->getHandle());
                previewMesh.reset();
            }

            // ── 3c: Remove any stale mesh registered under the same name ──────────
            {
                Ogre::MeshManager& mm = Ogre::MeshManager::getSingleton();
                const Ogre::String grp = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;
                Ogre::MeshPtr existing = mm.getByName(meshName, grp);
                if (false == existing.isNull())
                {
                    mm.remove(existing->getHandle());
                }
            }

            // ── 3d: Get the Ogre-Next plumbing ────────────────────────────────────
            Ogre::Root* root = Ogre::Root::getSingletonPtr();
            Ogre::RenderSystem* renderSys = root->getRenderSystem();
            Ogre::VaoManager* vaoManager = renderSys->getVaoManager();
            const Ogre::String groupName = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

            // ── 3e: Create the blank managed mesh ─────────────────────────────────
            previewMesh = Ogre::MeshManager::getSingleton().createManual(meshName, groupName);

            // ── 3f: Declare the vertex layout ─────────────────────────────────────
            // pos(3) + normal(3) + tangent(4) + uv(2) = 12 floats per vertex
            Ogre::VertexElement2Vec vertexElements;
            vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
            vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));
            vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT4, Ogre::VES_TANGENT));
            vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));

            // srcVerts layout: pos(3) + normal(3) + uv(2) = 8 floats
            // dstVerts layout: pos(3) + normal(3) + tangent(4) + uv(2) = 12 floats
            const size_t srcStride = 8u;
            const size_t dstStride = 12u;

            // ── 3g: Allocate SIMD-aligned destination vertex buffer ───────────────
            const size_t dstBytes = numVerts * dstStride * sizeof(float);
            float* dstData = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(dstBytes, Ogre::MEMCATEGORY_GEOMETRY));

            // ── 3h: Convert 8-float → 12-float; compute tangent from normal ───────
            Ogre::Vector3 minBounds(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
            Ogre::Vector3 maxBounds(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());

            for (size_t i = 0; i < numVerts; ++i)
            {
                const size_t src = i * srcStride;
                const size_t dst = i * dstStride;

                // Position
                dstData[dst + 0] = vertsCopy[src + 0];
                dstData[dst + 1] = vertsCopy[src + 1];
                dstData[dst + 2] = vertsCopy[src + 2];

                // Track bounds for _setBounds()
                Ogre::Vector3 pos(dstData[dst + 0], dstData[dst + 1], dstData[dst + 2]);
                minBounds.makeFloor(pos);
                maxBounds.makeCeil(pos);

                // Normal
                Ogre::Vector3 n(vertsCopy[src + 3], vertsCopy[src + 4], vertsCopy[src + 5]);
                dstData[dst + 3] = n.x;
                dstData[dst + 4] = n.y;
                dstData[dst + 5] = n.z;

                // Tangent — derived from normal (axis-aligned reference vector)
                // This matches the same approach used in createGeometryMeshInternal().
                Ogre::Vector3 t;
                if (std::abs(n.y) < 0.9f)
                {
                    t = Ogre::Vector3::UNIT_Y.crossProduct(n).normalisedCopy();
                }
                else
                {
                    t = n.crossProduct(Ogre::Vector3::UNIT_X).normalisedCopy();
                }

                dstData[dst + 6] = t.x;
                dstData[dst + 7] = t.y;
                dstData[dst + 8] = t.z;
                dstData[dst + 9] = 1.0f; // handedness

                // UV
                dstData[dst + 10] = vertsCopy[src + 6];
                dstData[dst + 11] = vertsCopy[src + 7];
            }

            // ── 3i: Upload vertex buffer to GPU ───────────────────────────────────
            Ogre::VertexBufferPacked* vertexBuffer = nullptr;
            try
            {
                vertexBuffer = vaoManager->createVertexBuffer(vertexElements, numVerts, Ogre::BT_IMMUTABLE, dstData, true /*keepAsShadow — vaoManager owns dstData now*/);
            }
            catch (Ogre::Exception& e)
            {
                OGRE_FREE_SIMD(dstData, Ogre::MEMCATEGORY_GEOMETRY);
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralGeometryComponent] createPreviewMesh: "
                                                                                    "createVertexBuffer failed: " +
                                                                                        e.getDescription());
                return;
            }

            // ── 3j: Allocate SIMD-aligned index buffer ────────────────────────────
            const size_t idxBytes = idxsCopy.size() * sizeof(Ogre::uint32);
            Ogre::uint32* idxData = reinterpret_cast<Ogre::uint32*>(OGRE_MALLOC_SIMD(idxBytes, Ogre::MEMCATEGORY_GEOMETRY));
            memcpy(idxData, idxsCopy.data(), idxBytes);

            // ── 3k: Upload index buffer to GPU ────────────────────────────────────
            Ogre::IndexBufferPacked* indexBuffer = nullptr;
            try
            {
                indexBuffer = vaoManager->createIndexBuffer(Ogre::IndexBufferPacked::IT_32BIT, idxsCopy.size(), Ogre::BT_IMMUTABLE, idxData, true /*keepAsShadow*/);
            }
            catch (Ogre::Exception& e)
            {
                OGRE_FREE_SIMD(idxData, Ogre::MEMCATEGORY_GEOMETRY);
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralGeometryComponent] createPreviewMesh: "
                                                                                    "createIndexBuffer failed: " +
                                                                                        e.getDescription());
                return;
            }

            // ── 3l: Create VAO and wire it to the submesh ─────────────────────────
            Ogre::VertexBufferPackedVec vbVec;
            vbVec.push_back(vertexBuffer);

            Ogre::VertexArrayObject* vao = vaoManager->createVertexArrayObject(vbVec, indexBuffer, Ogre::OT_TRIANGLE_LIST);

            Ogre::SubMesh* subMesh = previewMesh->createSubMesh();
            subMesh->mVao[Ogre::VpNormal].push_back(vao);
            subMesh->mVao[Ogre::VpShadow].push_back(vao);

            // ── 3m: Set mesh bounds ───────────────────────────────────────────────
            // Guard against degenerate geometry producing inverted bounds
            if (minBounds.x > maxBounds.x)
            {
                minBounds = Ogre::Vector3(-1.0f, -1.0f, -1.0f);
                maxBounds = Ogre::Vector3(1.0f, 1.0f, 1.0f);
            }

            Ogre::Aabb aabb;
            aabb.setExtents(minBounds, maxBounds);
            previewMesh->_setBounds(aabb, false);
            previewMesh->_setBoundingSphereRadius(aabb.getRadius());

            // ── 3n: Create Item (always SCENE_DYNAMIC — preview is transient) ─────
            previewItem = this->gameObjectPtr->getSceneManager()->createItem(previewMesh, Ogre::SCENE_DYNAMIC);

            // ── 3o: Apply the preview datablock ("BaseWhiteNoLighting") ──────────
            // This datablock is always registered by Ogre-Next at startup.
            // It renders as solid white with no lighting — clearly distinguishable
            // from the real mesh, and requires no PBS setup.
            Ogre::HlmsDatablock* previewDb = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault("BaseWhiteNoLighting");
            if (nullptr != previewDb)
            {
                previewItem->getSubItem(0)->setDatablock(previewDb);
            }
            // If the project has a dedicated "proceduralPreview" PBS datablock
            // (e.g. semi-transparent blue), prefer that instead:
            //
            //   Ogre::HlmsDatablock* customDb =
            //       Ogre::Root::getSingleton().getHlmsManager()
            //           ->getDatablockNoDefault("proceduralPreview");
            //   if (nullptr != customDb)
            //       previewItem->getSubItem(0)->setDatablock(customDb);

            // ── 3p: Configure Item so it is invisible to raycasts ─────────────────
            // queryFlags = 0 means PhysicsActiveCompoundComponent, editor raycasts,
            // etc. will never accidentally hit the ghost preview.
            previewItem->setQueryFlags(0u);
            previewItem->setCastShadows(false);

            // ── 3q: Attach to the preview scene node and show it ──────────────────
            previewNode->attachObject(previewItem);
            previewNode->setVisible(true);

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralGeometryComponent] createPreviewMesh: created '" + shape->getListSelectedValue() + "' preview — " + Ogre::StringConverter::toString(numVerts) + " verts, " +
                                                                                   Ogre::StringConverter::toString(idxsCopy.size() / 3u) + " tris.");
        };

        // ── Step 4: Submit to render thread and wait (we need previewItem valid) ─
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "ProceduralGeometryComponent::createPreviewMesh");
    }

    void ProceduralGeometryComponent::updatePreviewPosition(const Ogre::Vector3& worldPos)
    {
        if (nullptr == previewNode)
        {
            return;
        }
        NOWA::GraphicsModule::RenderCommand cmd = [this, worldPos]()
        {
            if (previewNode)
            {
                previewNode->setPosition(worldPos);
                previewNode->setVisible(true);
            }
        };
        // Non-blocking – preview can lag one frame
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(cmd), "ProceduralGeometryComponent::updatePreviewPosition");
    }

    void ProceduralGeometryComponent::destroyPreviewMesh(void)
    {
        if (nullptr == previewItem && previewMesh.isNull())
        {
            return;
        }
        NOWA::GraphicsModule::RenderCommand cmd = [this]()
        {
            if (nullptr != previewItem)
            {
                if (previewItem->getParentSceneNode())
                {
                    previewItem->getParentSceneNode()->detachObject(previewItem);
                }
                this->gameObjectPtr->getSceneManager()->destroyItem(previewItem);
                previewItem = nullptr;
            }
            if (false == previewMesh.isNull())
            {
                Ogre::MeshManager::getSingleton().remove(previewMesh->getHandle());
                previewMesh.reset();
            }
            if (previewNode)
            {
                previewNode->setVisible(false);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "ProceduralGeometryComponent::destroyPreviewMesh");
    }

    // =========================================================================
    //  Low-level vertex/index helpers
    // =========================================================================

    void ProceduralGeometryComponent::gv(Ogre::Real px, Ogre::Real py, Ogre::Real pz, Ogre::Real nx, Ogre::Real ny, Ogre::Real nz, Ogre::Real u, Ogre::Real v)
    {
        const Ogre::Vector2 uvT = uvTiling->getVector2();
        vertices.push_back(px);
        vertices.push_back(py);
        vertices.push_back(pz);
        vertices.push_back(nx);
        vertices.push_back(ny);
        vertices.push_back(nz);
        vertices.push_back(u * uvT.x);
        vertices.push_back(v * uvT.y);
    }

    void ProceduralGeometryComponent::gt(Ogre::uint32 i0, Ogre::uint32 i1, Ogre::uint32 i2)
    {
        if (flipNormals->getBool())
        {
            // Reverse winding to flip normals
            indices.push_back(this->vertBase + i0);
            indices.push_back(this->vertBase + i2);
            indices.push_back(this->vertBase + i1);
        }
        else
        {
            indices.push_back(this->vertBase + i0);
            indices.push_back(this->vertBase + i1);
            indices.push_back(this->vertBase + i2);
        }
    }

    void ProceduralGeometryComponent::gq(Ogre::uint32 i0, Ogre::uint32 i1, Ogre::uint32 i2, Ogre::uint32 i3)
    {
        this->gt(i0, i1, i2);
        this->gt(i0, i2, i3);
    }

    // =========================================================================
    //  Shape generators
    // =========================================================================

    // ── Box ──────────────────────────────────────────────────────────────────────
    // size = width x height x depth
    void ProceduralGeometryComponent::generateBox(void)
    {
        const Ogre::Vector3 sz = this->size->getVector3();
        const float hx = sz.x * 0.5f;
        const float hy = sz.y * 0.5f;
        const float hz = sz.z * 0.5f;

        // Helper: emit one quad face.
        // v0..v3 must be in CCW order when viewed from the outward-normal direction.
        // Verified: for each face, (v1-v0)x(v2-v0) == outward normal.
        auto face = [&](float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2, float z2, float x3, float y3, float z3, float nx, float ny, float nz)
        {
            this->vertBase = static_cast<Ogre::uint32>(this->vertices.size() / 8u);
            gv(x0, y0, z0, nx, ny, nz, 0.0f, 0.0f);
            gv(x1, y1, z1, nx, ny, nz, 1.0f, 0.0f);
            gv(x2, y2, z2, nx, ny, nz, 1.0f, 1.0f);
            gv(x3, y3, z3, nx, ny, nz, 0.0f, 1.0f);
            gq(0, 1, 2, 3);
        };

        // +Y top  – CCW from above: left-back → left-front → right-front → right-back
        // Cross check: (v1-v0)x(v2-v0) = (0,0,2hz)x(2hx,0,2hz) = (0,+4hxhz,0) ✓
        face(-hx, +hy, -hz, -hx, +hy, +hz, hx, +hy, +hz, hx, +hy, -hz, 0, +1, 0);

        // -Y bottom – CCW from below: left-front → left-back → right-back → right-front
        // Cross check: (v1-v0)x(v2-v0) = (0,0,-2hz)x(2hx,0,-2hz) = (0,-4hxhz,0) ✓
        face(-hx, -hy, +hz, -hx, -hy, -hz, hx, -hy, -hz, hx, -hy, +hz, 0, -1, 0);

        // +Z front – unchanged (was already correct)
        face(-hx, -hy, +hz, hx, -hy, +hz, hx, +hy, +hz, -hx, +hy, +hz, 0, 0, +1);

        // -Z back  – unchanged (was already correct)
        face(hx, -hy, -hz, -hx, -hy, -hz, -hx, +hy, -hz, hx, +hy, -hz, 0, 0, -1);

        // +X right – unchanged (was already correct)
        face(hx, -hy, +hz, hx, -hy, -hz, hx, +hy, -hz, hx, +hy, +hz, +1, 0, 0);

        // -X left  – unchanged (was already correct)
        face(-hx, -hy, -hz, -hx, -hy, +hz, -hx, +hy, +hz, -hx, +hy, -hz, -1, 0, 0);
    }

    // ── Pyramid ───────────────────────────────────────────────────────────────
    //   size = baseWidth x height x baseDepth
    void ProceduralGeometryComponent::generatePyramid(void)
    {
        const Ogre::Vector3 sz = this->size->getVector3();
        const float hx = sz.x * 0.5f;
        const float hy = sz.y;
        const float hz = sz.z * 0.5f;

        // Base corners (CCW from below)
        const float ax = -hx, az = hz;  // front-left
        const float bx = hx, bz = hz;   // front-right
        const float cx = hx, cz = -hz;  // back-right
        const float dx = -hx, dz = -hz; // back-left
        const float apexX = 0.0f, apexY = hy, apexZ = 0.0f;

        // Helper: emit one triangular face
        auto triface = [&](float x0, float y0, float z0, float x1, float y1, float z1, float x2, float y2, float z2, float u0, float v0, float u1, float v1, float u2, float v2)
        {
            // Compute face normal
            Ogre::Vector3 e1(x1 - x0, y1 - y0, z1 - z0);
            Ogre::Vector3 e2(x2 - x0, y2 - y0, z2 - z0);
            Ogre::Vector3 n = e1.crossProduct(e2).normalisedCopy();

            this->vertBase = static_cast<Ogre::uint32>(this->vertices.size() / 8u);
            gv(x0, y0, z0, n.x, n.y, n.z, u0, v0);
            gv(x1, y1, z1, n.x, n.y, n.z, u1, v1);
            gv(x2, y2, z2, n.x, n.y, n.z, u2, v2);
            gt(0, 1, 2);
        };

        // Front face: A,B, apex
        triface(ax, 0, az, bx, 0, bz, apexX, apexY, apexZ, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 1.0f);
        // Right face: B,C, apex
        triface(bx, 0, bz, cx, 0, cz, apexX, apexY, apexZ, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 1.0f);
        // Back face: C,D, apex
        triface(cx, 0, cz, dx, 0, dz, apexX, apexY, apexZ, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 1.0f);
        // Left face: D,A, apex
        triface(dx, 0, dz, ax, 0, az, apexX, apexY, apexZ, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 1.0f);

        // Base face (facing -Y): A,D,C,B
        this->vertBase = static_cast<Ogre::uint32>(this->vertices.size() / 8u);
        gv(ax, 0, az, 0, -1, 0, 0.0f, 1.0f);
        gv(dx, 0, dz, 0, -1, 0, 0.0f, 0.0f);
        gv(cx, 0, cz, 0, -1, 0, 1.0f, 0.0f);
        gv(bx, 0, bz, 0, -1, 0, 1.0f, 1.0f);
        gq(0, 1, 2, 3);
    }

    // ── Sphere (UV) ───────────────────────────────────────────────────────────────
    // No changes needed – was already correct.
    void ProceduralGeometryComponent::generateSphere(void)
    {
        const float r = this->size->getVector3().x;
        const int slices = std::max(3, static_cast<int>(this->segmentsH->getReal()));
        const int stacks = std::max(2, static_cast<int>(this->segmentsV->getReal()));

        this->vertBase = static_cast<Ogre::uint32>(this->vertices.size() / 8u);

        for (int stack = 0; stack <= stacks; ++stack)
        {
            const float phi = -Ogre::Math::HALF_PI + stack * Ogre::Math::PI / stacks;
            const float cosP = std::cos(phi), sinP = std::sin(phi);
            const float v = 1.0f - static_cast<float>(stack) / stacks;

            for (int sl = 0; sl <= slices; ++sl)
            {
                const float theta = sl * Ogre::Math::TWO_PI / slices;
                const float cosT = std::cos(theta), sinT = std::sin(theta);
                const float nx = cosP * cosT, ny = sinP, nz = cosP * sinT;
                gv(r * nx, r * ny, r * nz, nx, ny, nz, static_cast<float>(sl) / slices, v);
            }
        }

        const int rowLen = slices + 1;
        for (int stack = 0; stack < stacks; ++stack)
        {
            for (int sl = 0; sl < slices; ++sl)
            {
                const Ogre::uint32 a = stack * rowLen + sl;
                const Ogre::uint32 b = a + 1;
                const Ogre::uint32 c = (stack + 1) * rowLen + sl;
                const Ogre::uint32 d = c + 1;
                // gt(a,c,b): cross check at equator = (0,+,0) ✓
                gt(a, c, b);
                gt(b, c, d);
            }
        }
    }

    // ── Cylinder ─────────────────────────────────────────────────────────────────
    // size.x = radius,  size.y = height
    void ProceduralGeometryComponent::generateCylinder(void)
    {
        const float r = this->size->getVector3().x;
        const float h = this->size->getVector3().y;
        const int slices = std::max(3, static_cast<int>(this->segmentsH->getReal()));

        // ── Lateral surface ───────────────────────────────────────────────────────
        this->vertBase = static_cast<Ogre::uint32>(this->vertices.size() / 8u);

        for (int sl = 0; sl <= slices; ++sl)
        {
            const float theta = sl * Ogre::Math::TWO_PI / slices;
            const float c = std::cos(theta), s = std::sin(theta);
            const float u = static_cast<float>(sl) / slices;
            gv(r * c, 0.0f, r * s, c, 0, s, u, 1.0f); // bottom  [2*sl]
            gv(r * c, h, r * s, c, 0, s, u, 0.0f);    // top     [2*sl+1]
        }

        for (int sl = 0; sl < slices; ++sl)
        {
            const Ogre::uint32 b = sl * 2u;
            // FIX: was gq(b, b+2, b+3, b+1) → CW from outside.
            // gq(b, b+1, b+3, b+2): cross at sl=0 → (0,h,0)x(0,0,rΔθ) = (hrΔθ,0,0) → N=(+1,0,0) ✓
            gq(b, b + 1, b + 3, b + 2);
        }

        // ── Top cap (+Y) ──────────────────────────────────────────────────────────
        this->vertBase = static_cast<Ogre::uint32>(this->vertices.size() / 8u);
        gv(0.0f, h, 0.0f, 0, 1, 0, 0.5f, 0.5f); // centre [0]
        for (int sl = 0; sl <= slices; ++sl)
        {
            const float theta = sl * Ogre::Math::TWO_PI / slices;
            const float c = std::cos(theta), s = std::sin(theta);
            gv(r * c, h, r * s, 0, 1, 0, 0.5f + c * 0.5f, 0.5f + s * 0.5f);
        }
        for (int sl = 0; sl < slices; ++sl)
        {
            // gt(0, sl+2, sl+1): cross = ring[1]xring[0] → (0,+,0) ✓
            gt(0, sl + 2, sl + 1);
        }

        // ── Bottom cap (-Y) ───────────────────────────────────────────────────────
        this->vertBase = static_cast<Ogre::uint32>(this->vertices.size() / 8u);
        gv(0.0f, 0.0f, 0.0f, 0, -1, 0, 0.5f, 0.5f); // centre [0]
        for (int sl = 0; sl <= slices; ++sl)
        {
            const float theta = sl * Ogre::Math::TWO_PI / slices;
            const float c = std::cos(theta), s = std::sin(theta);
            gv(r * c, 0.0f, r * s, 0, -1, 0, 0.5f + c * 0.5f, 0.5f - s * 0.5f);
        }
        for (int sl = 0; sl < slices; ++sl)
        {
            // gt(0, sl+1, sl+2): cross = ring[0]xring[1] → (0,-,0) ✓
            gt(0, sl + 1, sl + 2);
        }
    }

    // ── Cone ─────────────────────────────────────────────────────────────────────
    // size.x = base radius,  size.y = height
    void ProceduralGeometryComponent::generateCone(void)
    {
        const float r = this->size->getVector3().x;
        const float h = this->size->getVector3().y;
        const int slices = std::max(3, static_cast<int>(this->segmentsH->getReal()));

        const float slant = std::sqrt(r * r + h * h);
        const float ny_n = (slant > 1e-6f) ? r / slant : 0.0f;
        const float nxz_s = (slant > 1e-6f) ? h / slant : 1.0f;

        // ── Lateral surface: one triangle per slice ───────────────────────────────
        for (int sl = 0; sl < slices; ++sl)
        {
            const float t0 = sl * Ogre::Math::TWO_PI / slices;
            const float t1 = (sl + 1) * Ogre::Math::TWO_PI / slices;
            const float c0 = std::cos(t0), s0 = std::sin(t0);
            const float c1 = std::cos(t1), s1 = std::sin(t1);
            const float cmid = std::cos((t0 + t1) * 0.5f);
            const float smid = std::sin((t0 + t1) * 0.5f);

            this->vertBase = static_cast<Ogre::uint32>(this->vertices.size() / 8u);

            gv(0, h, 0, cmid * nxz_s, ny_n, smid * nxz_s, (sl + 0.5f) / slices, 0.0f);                      // apex   [0]
            gv(r * c0, 0, r * s0, c0 * nxz_s, ny_n, s0 * nxz_s, static_cast<float>(sl) / slices, 1.0f);     // [1]
            gv(r * c1, 0, r * s1, c1 * nxz_s, ny_n, s1 * nxz_s, static_cast<float>(sl + 1) / slices, 1.0f); // [2]

            // FIX: was gt(0,1,2) → CW; cross at sl=0 of (base[0]-apex)x(base[1]-apex) pointed inward.
            // gt(0,2,1): (base[1]-apex)x(base[0]-apex) → outward ✓
            gt(0, 2, 1);
        }

        // ── Base cap (-Y) ─────────────────────────────────────────────────────────
        this->vertBase = static_cast<Ogre::uint32>(this->vertices.size() / 8u);
        gv(0, 0, 0, 0, -1, 0, 0.5f, 0.5f);
        for (int sl = 0; sl <= slices; ++sl)
        {
            const float t = sl * Ogre::Math::TWO_PI / slices;
            const float c = std::cos(t), s = std::sin(t);
            gv(r * c, 0, r * s, 0, -1, 0, 0.5f + c * 0.5f, 0.5f - s * 0.5f);
        }
        for (int sl = 0; sl < slices; ++sl)
        {
            gt(0, sl + 1, sl + 2); // cross → (0,-,0) ✓
        }
    }

    // ── Capsule ───────────────────────────────────────────────────────────────────
    // size.x = radius,  size.y = body cylinder height
    void ProceduralGeometryComponent::generateCapsule(void)
    {
        const float r = this->size->getVector3().x;
        const float bodyH = this->size->getVector3().y;
        const int slices = std::max(3, static_cast<int>(this->segmentsH->getReal()));
        const int hStacks = std::max(2, static_cast<int>(this->segmentsV->getReal()) / 2);

        const float topY = bodyH * 0.5f;
        const float botY = -bodyH * 0.5f;
        const int rowLen = slices + 1;

        // ── Top hemisphere ────────────────────────────────────────────────────────
        this->vertBase = static_cast<Ogre::uint32>(this->vertices.size() / 8u);

        for (int st = 0; st <= hStacks; ++st)
        {
            const float phi = static_cast<float>(st) * Ogre::Math::HALF_PI / hStacks;
            const float cosP = std::cos(phi), sinP = std::sin(phi);
            const float yOff = topY + r * sinP;
            const float v = 0.5f - static_cast<float>(st) / (hStacks * 2);

            for (int sl = 0; sl <= slices; ++sl)
            {
                const float theta = sl * Ogre::Math::TWO_PI / slices;
                const float c = std::cos(theta), s = std::sin(theta);
                gv(r * cosP * c, yOff, r * cosP * s, cosP * c, sinP, cosP * s, static_cast<float>(sl) / slices, v);
            }
        }

        for (int st = 0; st < hStacks; ++st)
        {
            for (int sl = 0; sl < slices; ++sl)
            {
                const Ogre::uint32 a = st * rowLen + sl;
                const Ogre::uint32 b = a + 1;
                const Ogre::uint32 c = (st + 1) * rowLen + sl;
                const Ogre::uint32 d = c + 1;
                gt(a, c, b); // same as sphere – cross → outward ✓
                gt(b, c, d);
            }
        }

        // ── Body cylinder (no caps) ────────────────────────────────────────────────
        this->vertBase = static_cast<Ogre::uint32>(this->vertices.size() / 8u);

        for (int sl = 0; sl <= slices; ++sl)
        {
            const float theta = sl * Ogre::Math::TWO_PI / slices;
            const float c = std::cos(theta), s = std::sin(theta);
            const float u = static_cast<float>(sl) / slices;
            gv(r * c, botY, r * s, c, 0, s, u, 0.75f); // bottom [2*sl]
            gv(r * c, topY, r * s, c, 0, s, u, 0.5f);  // top    [2*sl+1]
        }

        for (int sl = 0; sl < slices; ++sl)
        {
            const Ogre::uint32 b = sl * 2u;
            // FIX: was gq(b, b+2, b+3, b+1) → CW from outside.
            gq(b, b + 1, b + 3, b + 2); // CCW from outside ✓
        }

        // ── Bottom hemisphere ─────────────────────────────────────────────────────
        this->vertBase = static_cast<Ogre::uint32>(this->vertices.size() / 8u);

        for (int st = 0; st <= hStacks; ++st)
        {
            const float phi = static_cast<float>(st) * Ogre::Math::HALF_PI / hStacks;
            const float cosP = std::cos(phi), sinP = std::sin(phi);
            const float yOff = botY - r * sinP;
            const float v = 0.75f + static_cast<float>(st) / (hStacks * 2);

            for (int sl = 0; sl <= slices; ++sl)
            {
                const float theta = sl * Ogre::Math::TWO_PI / slices;
                const float c = std::cos(theta), s = std::sin(theta);
                gv(r * cosP * c, yOff, r * cosP * s, cosP * c, -sinP, cosP * s, static_cast<float>(sl) / slices, v);
            }
        }

        for (int st = 0; st < hStacks; ++st)
        {
            for (int sl = 0; sl < slices; ++sl)
            {
                const Ogre::uint32 a = st * rowLen + sl;
                const Ogre::uint32 b = a + 1, c = (st + 1) * rowLen + sl, d = c + 1;
                // Reversed vs top hemisphere because y-descent inverts the swept direction.
                gt(a, b, c);
                gt(b, d, c);
            }
        }
    }

    // ── Torus ─────────────────────────────────────────────────────────────────────
    // size.x = major radius,  size.z = tube radius
    void ProceduralGeometryComponent::generateTorus(void)
    {
        const float R = this->size->getVector3().x;
        const float r = this->size->getVector3().z;
        const int slices = std::max(3, static_cast<int>(this->segmentsH->getReal()));
        const int rings = std::max(3, static_cast<int>(this->segmentsV->getReal()));

        this->vertBase = static_cast<Ogre::uint32>(this->vertices.size() / 8u);

        for (int sl = 0; sl <= slices; ++sl)
        {
            const float theta = sl * Ogre::Math::TWO_PI / slices;
            const float cosT = std::cos(theta), sinT = std::sin(theta);
            const float u = static_cast<float>(sl) / slices;

            for (int ri = 0; ri <= rings; ++ri)
            {
                const float phi = ri * Ogre::Math::TWO_PI / rings;
                const float cosP = std::cos(phi), sinP = std::sin(phi);

                const float x = (R + r * cosP) * cosT;
                const float y = r * sinP;
                const float z = (R + r * cosP) * sinT;

                gv(x, y, z, cosP * cosT, sinP, cosP * sinT, u, static_cast<float>(ri) / rings);
            }
        }

        const int rowLen = rings + 1;
        for (int sl = 0; sl < slices; ++sl)
        {
            for (int ri = 0; ri < rings; ++ri)
            {
                const Ogre::uint32 a = sl * rowLen + ri;       // (sl,   ri)
                const Ogre::uint32 b = a + 1;                  // (sl,   ri+1)
                const Ogre::uint32 c = (sl + 1) * rowLen + ri; // (sl+1, ri)
                const Ogre::uint32 d = c + 1;                  // (sl+1, ri+1)

                // FIX: was gt(a,c,b) → (c-a)x(b-a) = (0,0,RΔθ)x(0,rΔφ,0) = (-RrΔθΔφ,0,0) → INWARD.
                // gt(a,b,c): (b-a)x(c-a) = (0,rΔφ,0)x(0,0,RΔθ) = (+RrΔθΔφ,0,0) → OUTWARD ✓
                gt(a, b, c);
                gt(b, d, c);
            }
        }
    }

    // ── Plane ─────────────────────────────────────────────────────────────────────
    // size.x = width,  size.z = depth.  Double-sided: top face N=(0,+1,0), bottom N=(0,-1,0).
    void ProceduralGeometryComponent::generatePlane(void)
    {
        const float sx = this->size->getVector3().x;
        const float sz = this->size->getVector3().z;
        const int cols = std::max(1, static_cast<int>(this->segmentsH->getReal()));
        const int rows = std::max(1, static_cast<int>(this->segmentsV->getReal()));

        const int rowLen = cols + 1;

        auto emitGrid = [&](float ny, bool reverseWinding)
        {
            this->vertBase = static_cast<Ogre::uint32>(this->vertices.size() / 8u);

            for (int row = 0; row <= rows; ++row)
            {
                for (int col = 0; col <= cols; ++col)
                {
                    const float u = static_cast<float>(col) / cols;
                    const float v = static_cast<float>(row) / rows;
                    gv((u - 0.5f) * sx, 0.0f, (v - 0.5f) * sz, 0, ny, 0, u, v);
                }
            }

            for (int row = 0; row < rows; ++row)
            {
                for (int col = 0; col < cols; ++col)
                {
                    const Ogre::uint32 a = row * rowLen + col;
                    const Ogre::uint32 b = a + 1;
                    const Ogre::uint32 c = (row + 1) * rowLen + col;
                    const Ogre::uint32 d = c + 1;

                    if (!reverseWinding)
                    {
                        // Top face: gt(a,c,b) → cross=(0,0,Δz)x(Δx,0,0)=(0,+ΔxΔz,0) ✓
                        gt(a, c, b);
                        gt(b, c, d);
                    }
                    else
                    {
                        // Bottom face: reversed winding → N=(0,-1,0)
                        gt(a, b, c);
                        gt(b, d, c);
                    }
                }
            }
        };

        emitGrid(+1.0f, false); // top face
        emitGrid(-1.0f, true);  // bottom face
    }

    // ── Prism (triangular) ────────────────────────────────────────────────────────
    // size = baseWidth x height x baseDepth
    void ProceduralGeometryComponent::generatePrism(void)
    {
        const Ogre::Vector3 sz = this->size->getVector3();
        const float hx = sz.x * 0.5f;
        const float hy = sz.y;
        const float hz = sz.z * 0.5f;

        // Triangle base in XZ: A=(-hx,?,-hz)  B=(+hx,?,-hz)  C=(0,?,+hz)

        // Outward normal of a vertical side panel given its two XZ base points
        auto sideNormal = [](float ax, float az, float bx, float bz) -> Ogre::Vector3
        {
            // edge = (bx-ax, 0, bz-az);  normal = Y_UP x edge, then normalise
            Ogre::Vector3 edge(bx - ax, 0.0f, bz - az);
            return Ogre::Vector3::UNIT_Y.crossProduct(edge).normalisedCopy();
        };

        // ── Bottom cap (Y=0, N=(0,-1,0)) ──────────────────────────────────────────
        // Vertices: A, B, C in CW order when viewed from above = CCW from below ✓
        this->vertBase = static_cast<Ogre::uint32>(this->vertices.size() / 8u);
        gv(-hx, 0.0f, -hz, 0, -1, 0, 0.0f, 0.0f); // A [0]
        gv(hx, 0.0f, -hz, 0, -1, 0, 1.0f, 0.0f);  // B [1]
        gv(0, 0.0f, hz, 0, -1, 0, 0.5f, 1.0f);    // C [2]
        // FIX: was gt(0,2,1) → cross=(C-A)x(B-A)=(hx,0,2hz)x(2hx,0,0)=(0,+,0) WRONG (+Y not -Y).
        // gt(0,1,2): cross=(B-A)x(C-A)=(2hx,0,0)x(hx,0,2hz)=(0,-4hxhz,0) → N=(0,-1,0) ✓
        gt(0, 1, 2);

        // ── Top cap (Y=hy, N=(0,+1,0)) ────────────────────────────────────────────
        this->vertBase = static_cast<Ogre::uint32>(this->vertices.size() / 8u);
        gv(-hx, hy, -hz, 0, +1, 0, 0.0f, 0.0f); // A' [0]
        gv(hx, hy, -hz, 0, +1, 0, 1.0f, 0.0f);  // B' [1]
        gv(0, hy, hz, 0, +1, 0, 0.5f, 1.0f);    // C' [2]
        // FIX: was gt(0,1,2) → N=(0,-1,0). Swap to gt(0,2,1) → N=(0,+1,0) ✓
        gt(0, 2, 1);

        // ── Back face (A-B-B'-A', N ≈ (0,0,-1)) ─────────────────────────────────
        {
            const Ogre::Vector3 n = sideNormal(-hx, -hz, +hx, -hz);
            this->vertBase = static_cast<Ogre::uint32>(this->vertices.size() / 8u);
            gv(-hx, 0.0f, -hz, n.x, n.y, n.z, 0.0f, 0.0f);
            gv(hx, 0.0f, -hz, n.x, n.y, n.z, 1.0f, 0.0f);
            gv(hx, hy, -hz, n.x, n.y, n.z, 1.0f, 1.0f);
            gv(-hx, hy, -hz, n.x, n.y, n.z, 0.0f, 1.0f);
            // FIX: was gq(0,1,2,3) → cross=(2hx,0,0)x(2hx,hy,0)=(0,0,+2hxhy) → N=(0,0,+1) WRONG.
            // gq(0,3,2,1): cross=(v3-v0)x(v2-v0)=(0,hy,0)x(2hx,hy,0)=(0,0,-2hxhy) → N=(0,0,-1) ✓
            gq(0, 3, 2, 1);
        }

        // ── Right face (B-C-C'-B', outward to +X/+Z) ─────────────────────────────
        {
            const Ogre::Vector3 n = sideNormal(+hx, -hz, 0.0f, +hz);
            this->vertBase = static_cast<Ogre::uint32>(this->vertices.size() / 8u);
            gv(hx, 0.0f, -hz, n.x, n.y, n.z, 0.0f, 0.0f);
            gv(0, 0.0f, hz, n.x, n.y, n.z, 1.0f, 0.0f);
            gv(0, hy, hz, n.x, n.y, n.z, 1.0f, 1.0f);
            gv(hx, hy, -hz, n.x, n.y, n.z, 0.0f, 1.0f);
            // FIX: was gq(0,1,2,3) → INWARD.
            // gq(0,3,2,1): cross=(v3-v0)x(v2-v0)=(0,hy,0)x(-hx,hy,2hz)=(2hzhy,0,hxhy) → N∝(2hz,0,hx) ✓
            gq(0, 3, 2, 1);
        }

        // ── Left face (C-A-A'-C', outward to -X/+Z) ──────────────────────────────
        {
            const Ogre::Vector3 n = sideNormal(0.0f, +hz, -hx, -hz);
            this->vertBase = static_cast<Ogre::uint32>(this->vertices.size() / 8u);
            gv(0, 0.0f, hz, n.x, n.y, n.z, 0.0f, 0.0f);
            gv(-hx, 0.0f, -hz, n.x, n.y, n.z, 1.0f, 0.0f);
            gv(-hx, hy, -hz, n.x, n.y, n.z, 1.0f, 1.0f);
            gv(0, hy, hz, n.x, n.y, n.z, 0.0f, 1.0f);
            // FIX: was gq(0,1,2,3) → INWARD.
            // gq(0,3,2,1): cross=(v3-v0)x(v2-v0)=(0,hy,0)x(-hx,hy,-2hz)=(-2hzhy,0,hxhy)→N∝(-2hz,0,hx) ✓
            gq(0, 3, 2, 1);
        }
    }

    // ── Disc / Annulus ────────────────────────────────────────────────────────────
    // size.x = outer radius,  size.z = inner radius (0 = solid disc)
    void ProceduralGeometryComponent::generateDisc(void)
    {
        const float outerR = this->size->getVector3().x;
        const float innerR = std::max(0.0f, this->size->getVector3().z);
        const int slices = std::max(3, static_cast<int>(this->segmentsH->getReal()));

        // Only treat as annulus if inner is meaningfully smaller than outer.
        // This fixes the default-size (1,1,1) case where innerR==outerR → zero-width ring.
        const bool isRing = (innerR > 1e-4f) && (innerR < outerR - 1e-4f);

        if (!isRing)
        {
            // ── Solid disc: triangle fan from centre ──────────────────────────────
            this->vertBase = static_cast<Ogre::uint32>(this->vertices.size() / 8u);
            gv(0, 0, 0, 0, 1, 0, 0.5f, 0.5f); // centre [0]

            for (int sl = 0; sl <= slices; ++sl)
            {
                const float t = sl * Ogre::Math::TWO_PI / slices;
                const float c = std::cos(t), s = std::sin(t);
                gv(outerR * c, 0, outerR * s, 0, 1, 0, 0.5f + c * 0.5f, 0.5f + s * 0.5f);
            }

            for (int sl = 0; sl < slices; ++sl)
            {
                // FIX: was gt(0,sl+1,sl+2) → cross=(ring[0])x(ring[1]) → (0,-r²sinΔ,0) → N=(0,-1,0) WRONG.
                // gt(0,sl+2,sl+1): cross=(ring[1])x(ring[0]) → (0,+r²sinΔ,0) → N=(0,+1,0) ✓
                gt(0, sl + 2, sl + 1);
            }
        }
        else
        {
            // ── Annulus: two concentric rings connected by quads ──────────────────
            this->vertBase = static_cast<Ogre::uint32>(this->vertices.size() / 8u);

            for (int sl = 0; sl <= slices; ++sl)
            {
                const float t = sl * Ogre::Math::TWO_PI / slices;
                const float c = std::cos(t), s = std::sin(t);
                const float u = static_cast<float>(sl) / slices;
                gv(innerR * c, 0, innerR * s, 0, 1, 0, u, 0.0f); // inner [2*sl]
                gv(outerR * c, 0, outerR * s, 0, 1, 0, u, 1.0f); // outer [2*sl+1]
            }

            for (int sl = 0; sl < slices; ++sl)
            {
                const Ogre::uint32 b = sl * 2u;
                // gq(b,b+2,b+3,b+1): cross verification → N=(0,+1,0) ✓ (was already correct)
                gq(b, b + 2, b + 3, b + 1);
            }
        }
    }

    // ── Wedge (ramp) ──────────────────────────────────────────────────────────────
    // size = width x height x depth
    void ProceduralGeometryComponent::generateWedge(void)
    {
        const Ogre::Vector3 sz = this->size->getVector3();
        const float hx = sz.x * 0.5f;
        const float hy = sz.y;
        const float hz = sz.z * 0.5f;

        // Slope normal: edge direction along slope = (0, -hy, sz.z).
        // slopeN = edge x UNIT_X, normalized = (0, sz.z, hy) / |…|
        const Ogre::Vector3 slopeEdge(0.0f, -hy, sz.z);
        const Ogre::Vector3 slopeN = slopeEdge.crossProduct(Ogre::Vector3::UNIT_X).normalisedCopy();

        // ── Bottom face (N=(0,-1,0)) ─────────────────────────────────────────────
        this->vertBase = static_cast<Ogre::uint32>(this->vertices.size() / 8u);
        gv(-hx, 0.0f, hz, 0, -1, 0, 0.0f, 0.0f);
        gv(hx, 0.0f, hz, 0, -1, 0, 1.0f, 0.0f);
        gv(hx, 0.0f, -hz, 0, -1, 0, 1.0f, 1.0f);
        gv(-hx, 0.0f, -hz, 0, -1, 0, 0.0f, 1.0f);
        // FIX: was gq(0,1,2,3) → cross=(2hx,0,0)x(2hx,0,-2hz)=(0,+4hxhz,0) → N=(0,+1,0) WRONG.
        // gq(0,3,2,1): cross=(v3-v0)x(v2-v0)=(0,0,-2hz)x(2hx,0,-2hz)=(0,-4hxhz,0) → N=(0,-1,0) ✓
        gq(0, 3, 2, 1);

        // ── Back face (N=(0,0,-1)) ────────────────────────────────────────────────
        this->vertBase = static_cast<Ogre::uint32>(this->vertices.size() / 8u);
        gv(hx, 0.0f, -hz, 0, 0, -1, 0.0f, 0.0f);
        gv(-hx, 0.0f, -hz, 0, 0, -1, 1.0f, 0.0f);
        gv(-hx, hy, -hz, 0, 0, -1, 1.0f, 1.0f);
        gv(hx, hy, -hz, 0, 0, -1, 0.0f, 1.0f);
        // gq(0,1,2,3): cross=(-2hx,0,0)x(-2hx,hy,0)=(0,0,-2hxhy) → N=(0,0,-1) ✓  (was already correct)
        gq(0, 1, 2, 3);

        // ── Sloped top face ────────────────────────────────────────────────────────
        this->vertBase = static_cast<Ogre::uint32>(this->vertices.size() / 8u);
        gv(-hx, hy, -hz, slopeN.x, slopeN.y, slopeN.z, 0.0f, 0.0f);
        gv(hx, hy, -hz, slopeN.x, slopeN.y, slopeN.z, 1.0f, 0.0f);
        gv(hx, 0.0f, hz, slopeN.x, slopeN.y, slopeN.z, 1.0f, 1.0f);
        gv(-hx, 0.0f, hz, slopeN.x, slopeN.y, slopeN.z, 0.0f, 1.0f);
        // FIX: was gq(0,1,2,3) → cross=(2hx,0,0)x(2hx,-hy,2hz)=(0,-,0) and -(0,0,hy) → INWARD.
        // gq(0,3,2,1): cross=(v3-v0)x(v2-v0)=(0,-hy,2hz)x(2hx,-hy,2hz)=(0,+4hxhz,+2hxhy) → OUTWARD ✓
        gq(0, 3, 2, 1);

        // ── Left triangle (N=(-1,0,0)) ────────────────────────────────────────────
        this->vertBase = static_cast<Ogre::uint32>(this->vertices.size() / 8u);
        gv(-hx, 0.0f, hz, -1, 0, 0, 0.0f, 0.0f);
        gv(-hx, 0.0f, -hz, -1, 0, 0, 1.0f, 0.0f);
        gv(-hx, hy, -hz, -1, 0, 0, 1.0f, 1.0f);
        // FIX: was gt(0,1,2) → cross=(0,0,-2hz)x(0,hy,-2hz)=(+2hzhy,0,0) → N=(+1,0,0) WRONG.
        // gt(0,2,1): cross=(v2-v0)x(v1-v0)=(0,hy,-2hz)x(0,0,-2hz)=(-2hzhy,0,0) → N=(-1,0,0) ✓
        gt(0, 2, 1);

        // ── Right triangle (N=(+1,0,0)) ───────────────────────────────────────────
        this->vertBase = static_cast<Ogre::uint32>(this->vertices.size() / 8u);
        gv(hx, 0.0f, -hz, +1, 0, 0, 0.0f, 0.0f);
        gv(hx, 0.0f, hz, +1, 0, 0, 1.0f, 0.0f);
        gv(hx, hy, -hz, +1, 0, 0, 0.5f, 1.0f);
        // FIX: was gt(0,1,2) → cross=(0,0,2hz)x(0,hy,0)=(-2hzhy,0,0) → N=(-1,0,0) WRONG.
        // gt(0,2,1): cross=(v2-v0)x(v1-v0)=(0,hy,0)x(0,0,2hz)=(2hzhy,0,0) → N=(+1,0,0) ✓
        gt(0, 2, 1);
    }

    // =========================================================================
    //  Enum helper
    // =========================================================================

    ProceduralGeometryComponent::GeometryShape ProceduralGeometryComponent::getShapeEnum(void) const
    {
        const Ogre::String& sel = shape->getListSelectedValue();
        if (sel == "Box")
        {
            return GeometryShape::BOX;
        }
        if (sel == "Pyramid")
        {
            return GeometryShape::PYRAMID;
        }
        if (sel == "Sphere")
        {
            return GeometryShape::SPHERE;
        }
        if (sel == "Cylinder")
        {
            return GeometryShape::CYLINDER;
        }
        if (sel == "Cone")
        {
            return GeometryShape::CONE;
        }
        if (sel == "Capsule")
        {
            return GeometryShape::CAPSULE;
        }
        if (sel == "Torus")
        {
            return GeometryShape::TORUS;
        }
        if (sel == "Plane")
        {
            return GeometryShape::PLANE;
        }
        if (sel == "Prism")
        {
            return GeometryShape::PRISM;
        }
        if (sel == "Disc")
        {
            return GeometryShape::DISC;
        }
        if (sel == "Wedge")
        {
            return GeometryShape::WEDGE;
        }
        return GeometryShape::BOX;
    }

    // =========================================================================
    //  Convert to static mesh
    // =========================================================================

    bool ProceduralGeometryComponent::convertToMeshApply(void)
    {
        if (geomMesh.isNull() || nullptr == geomItem)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralGeometryComponent] convertToMeshApply: no geometry mesh to export.");
            return false;
        }

        // ── 1. Build filename and resolve the "Procedural" resource folder ────────
        Ogre::String meshName = shape->getListSelectedValue() + "_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()) + ".mesh";

        if (!Ogre::StringUtil::endsWith(meshName, ".mesh", true))
        {
            meshName += ".mesh";
        }

        auto filePathNames = Core::getSingletonPtr()->getSectionPath("Procedural");
        if (filePathNames.empty())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralGeometryComponent] 'Procedural' resource section not found.");
            return false;
        }

        const Ogre::String fullPath = filePathNames[0] + "/" + meshName;

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralGeometryComponent] Converting procedural geometry to static mesh: " + meshName);

        // ── 2. Export the mesh now (on the calling thread) ────────────────────────
        if (false == this->exportMesh(fullPath))
        {
            return false;
        }

        // ── 3. Capture everything needed inside the deferred closure ──────────────
        const Ogre::String capturedMeshName = meshName;
        GameObjectPtr capturedGOPtr = this->gameObjectPtr;
        const unsigned int capturedCompIndex = this->getIndex();
        const Ogre::String capturedDatablockName = datablock->getString();

        // Capture full transform so we can restore it after assignMesh()
        const Ogre::Vector3 currentPosition = this->gameObjectPtr->getPosition();
        const Ogre::Quaternion currentOrientation = this->gameObjectPtr->getOrientation();
        const Ogre::Vector3 currentScale = this->gameObjectPtr->getScale();

        // ── 4. Schedule the swap with a short delay (mesh file needs to flush) ────
        NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.5f));

        auto conversionFunction = [this, capturedMeshName, capturedGOPtr, capturedCompIndex, capturedDatablockName, currentPosition, currentOrientation, currentScale]()
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralGeometryComponent] Loading converted mesh: " + capturedMeshName);

            // ── Load the exported .mesh from the "Procedural" resource group ──────
            Ogre::MeshPtr loadedMesh;
            try
            {
                loadedMesh = Ogre::MeshManager::getSingleton().load(capturedMeshName, "Procedural");
            }
            catch (Ogre::Exception& e)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralGeometryComponent] Failed to load exported mesh: " + e.getFullDescription());
                return;
            }

            if (loadedMesh.isNull())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralGeometryComponent] Loaded mesh is null!");
                return;
            }

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralGeometryComponent] Mesh loaded: " + Ogre::StringConverter::toString(loadedMesh->getNumSubMeshes()) + " submesh(es)");

            // ── Create a new Item from the loaded mesh on the render thread ───────
            Ogre::Item* newItem = nullptr;

            NOWA::GraphicsModule::RenderCommand renderCommand = [this, capturedGOPtr, loadedMesh, capturedDatablockName, &newItem]()
            {
                newItem = capturedGOPtr->getSceneManager()->createItem(loadedMesh, capturedGOPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC);

                // Re-apply the single geometry datablock to submesh 0
                if (newItem->getNumSubItems() >= 1 && !capturedDatablockName.empty())
                {
                    Ogre::HlmsDatablock* db = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(capturedDatablockName);
                    if (nullptr != db)
                    {
                        newItem->getSubItem(0)->setDatablock(db);
                        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralGeometryComponent] Applied datablock: " + capturedDatablockName);
                    }
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralGeometryComponent::convertToMesh_createItem");

            if (nullptr == newItem)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralGeometryComponent] Failed to create Item from exported mesh!");
                return;
            }

            // ── Tear down the procedural mesh before handing control to the GO ────
            this->destroyPreviewMesh();
            this->destroyGeometryMesh();

            // ── Swap: assign the new static Item to the GameObject ────────────────
            if (false == capturedGOPtr->assignMesh(newItem))
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralGeometryComponent] assignMesh() failed!");
                return;
            }

            // ── Restore transform (assignMesh may reset it) ───────────────────────
            capturedGOPtr->getSceneNode()->setPosition(currentPosition);
            capturedGOPtr->getSceneNode()->setOrientation(currentOrientation);
            capturedGOPtr->getSceneNode()->setScale(currentScale);

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralGeometryComponent] Transform restored: pos=" + Ogre::StringConverter::toString(currentPosition));

            // ── Fire the component-deleted event so the editor updates its UI ─────
            boost::shared_ptr<EventDataDeleteComponent> evtDel(new EventDataDeleteComponent(capturedGOPtr->getId(), "ProceduralGeometryComponent", capturedCompIndex));
            NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evtDel);

            // ── Actually remove the ProceduralGeometryComponent from the GO ───────
            capturedGOPtr->deleteComponentByIndex(capturedCompIndex);

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralGeometryComponent] ========================================");
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralGeometryComponent] CONVERSION COMPLETE!");
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralGeometryComponent] - ProceduralGeometryComponent removed");
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralGeometryComponent] - Static mesh: " + capturedMeshName);
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralGeometryComponent] IMPORTANT: SAVE YOUR SCENE to persist this change!");
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralGeometryComponent] ========================================");
        };

        NOWA::ProcessPtr closureProcess(new NOWA::ClosureProcess(conversionFunction));
        delayProcess->attachChild(closureProcess);
        NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralGeometryComponent] Mesh exported. Conversion scheduled in 0.5 s...");

        // Refresh the property panel immediately (button press feedback)
        boost::shared_ptr<EventDataRefreshGui> evtGui(new EventDataRefreshGui());
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evtGui);

        return true;
    }

    bool ProceduralGeometryComponent::exportMesh(const Ogre::String& filename)
    {
        if (geomMesh.isNull())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralGeometryComponent] exportMesh: no mesh!");
            return false;
        }
        try
        {
            Ogre::MeshSerializer serializer(Ogre::Root::getSingletonPtr()->getRenderSystem()->getVaoManager());
            serializer.exportMesh(geomMesh.get(), filename);
            return true;
        }
        catch (Ogre::Exception& e)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralGeometryComponent] exportMesh failed: " + e.getDescription());
            return false;
        }
    }

    // =========================================================================
    //  Attribute setters / getters
    // =========================================================================

    void ProceduralGeometryComponent::setActivated(bool activated)
    {
        this->activated->setValue(activated);
    }

    bool ProceduralGeometryComponent::isActivated(void) const
    {
        return activated->getBool();
    }

    void ProceduralGeometryComponent::setShape(const Ogre::String& shapeName)
    {
        this->shape->setListSelectedValue(shapeName);
        this->rebuildMesh();
    }

    Ogre::String ProceduralGeometryComponent::getShape(void) const
    {
        return this->shape->getListSelectedValue();
    }

    void ProceduralGeometryComponent::setSize(const Ogre::Vector3& size)
    {
        this->size->setValue(size);
        this->rebuildMesh();
    }

    Ogre::Vector3 ProceduralGeometryComponent::getSize(void) const
    {
        return this->size->getVector3();
    }

    void ProceduralGeometryComponent::setSegmentsH(Ogre::Real segments)
    {
        this->segmentsH->setValue(std::max(3.0f, segments));
        this->rebuildMesh();
    }

    Ogre::Real ProceduralGeometryComponent::getSegmentsH(void) const
    {
        return this->segmentsH->getReal();
    }

    void ProceduralGeometryComponent::setSegmentsV(Ogre::Real segments)
    {
        this->segmentsV->setValue(std::max(2.0f, segments));
        this->rebuildMesh();
    }

    Ogre::Real ProceduralGeometryComponent::getSegmentsV(void) const
    {
        return this->segmentsV->getReal();
    }

    void ProceduralGeometryComponent::setFlipNormals(bool flip)
    {
        this->flipNormals->setValue(flip);
        this->rebuildMesh();
    }

    bool ProceduralGeometryComponent::getFlipNormals(void) const
    {
        return this->flipNormals->getBool();
    }

    void ProceduralGeometryComponent::setDatablock(const Ogre::String& datablockName)
    {
        this->datablock->setValue(datablockName);

        // Live-swap the datablock without a full mesh rebuild
        if (nullptr != this->geomItem)
        {
            NOWA::GraphicsModule::RenderCommand cmd = [this, datablockName]()
            {
                if (nullptr == this->geomItem)
                {
                    return;
                }

                Ogre::HlmsDatablock* db = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(datablockName);
                if (nullptr != db)
                {
                    this->geomItem->getSubItem(0)->setDatablock(db);
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "ProceduralGeometryComponent::setDatablock");
        }
    }

    Ogre::String ProceduralGeometryComponent::getDatablock(void) const
    {
        return this->datablock->getString();
    }

    void ProceduralGeometryComponent::setUVTiling(const Ogre::Vector2& tiling)
    {
        this->uvTiling->setValue(tiling);
        this->rebuildMesh();
    }

    Ogre::Vector2 ProceduralGeometryComponent::getUVTiling(void) const
    {
        return this->uvTiling->getVector2();
    }

    // =========================================================================
    //  Lua API
    // =========================================================================

    ProceduralGeometryComponent* getProceduralGeometryComponent(GameObject* go)
    {
        return NOWA::makeStrongPtr(go->getComponent<ProceduralGeometryComponent>()).get();
    }

    ProceduralGeometryComponent* getProceduralGeometryComponentFromName(GameObject* go, const Ogre::String& name)
    {
        return NOWA::makeStrongPtr(go->getComponentFromName<ProceduralGeometryComponent>(name)).get();
    }

    void ProceduralGeometryComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
    {
        luabind::module(lua)[luabind::class_<ProceduralGeometryComponent, GameObjectComponent>("ProceduralGeometryComponent")

                // ── Activation ──────────────────────────────────────────────────
                .def("setActivated", &ProceduralGeometryComponent::setActivated)
                .def("isActivated", &ProceduralGeometryComponent::isActivated)

                // ── Shape ────────────────────────────────────────────────────────
                .def("setShape", &ProceduralGeometryComponent::setShape)
                .def("getShape", &ProceduralGeometryComponent::getShape)

                // ── Dimensions ───────────────────────────────────────────────────
                .def("setSize", &ProceduralGeometryComponent::setSize)
                .def("getSize", &ProceduralGeometryComponent::getSize)

                // ── Tessellation ─────────────────────────────────────────────────
                .def("setSegmentsH", &ProceduralGeometryComponent::setSegmentsH)
                .def("getSegmentsH", &ProceduralGeometryComponent::getSegmentsH)
                .def("setSegmentsV", &ProceduralGeometryComponent::setSegmentsV)
                .def("getSegmentsV", &ProceduralGeometryComponent::getSegmentsV)

                // ── Normal orientation ───────────────────────────────────────────
                .def("setFlipNormals", &ProceduralGeometryComponent::setFlipNormals)
                .def("getFlipNormals", &ProceduralGeometryComponent::getFlipNormals)

                // ── Material ─────────────────────────────────────────────────────
                .def("setDatablock", &ProceduralGeometryComponent::setDatablock)
                .def("getDatablock", &ProceduralGeometryComponent::getDatablock)

                // ── UV ───────────────────────────────────────────────────────────
                .def("setUVTiling", &ProceduralGeometryComponent::setUVTiling)
                .def("getUVTiling", &ProceduralGeometryComponent::getUVTiling)

                // ── Manual rebuild ────────────────────────────────────────────────
                .def("rebuildMesh", &ProceduralGeometryComponent::rebuildMesh)];

        // ── LuaScriptApi documentation ────────────────────────────────────────
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralGeometryComponent", "class inherits GameObjectComponent", ProceduralGeometryComponent::getStaticInfoText());

        LuaScriptApi::getInstance()->addClassToCollection("ProceduralGeometryComponent", "void setActivated(bool activated)", "Activates or deactivates the geometry placement mode.");

        LuaScriptApi::getInstance()->addClassToCollection("ProceduralGeometryComponent", "void setShape(string shape)",
            "Sets the primitive shape. Valid values: 'Box', 'Pyramid', 'Sphere', 'Cylinder', "
            "'Cone', 'Capsule', 'Torus', 'Plane', 'Prism', 'Disc', 'Wedge'. Triggers a mesh rebuild.");

        LuaScriptApi::getInstance()->addClassToCollection("ProceduralGeometryComponent", "string getShape()", "Returns the currently selected shape name.");

        LuaScriptApi::getInstance()->addClassToCollection("ProceduralGeometryComponent", "void setSize(Vector3 size)",
            "Sets shape dimensions. Interpretation varies per shape: "
            "Box=widthXheightXdepth, Sphere=radius(x), Cylinder=radius(x)/height(y), "
            "Torus=majorR(x)/tubeR(z), Plane=width(x)/depth(z), Disc=outerR(x)/innerR(z). "
            "Always triggers a mesh rebuild.");

        LuaScriptApi::getInstance()->addClassToCollection("ProceduralGeometryComponent", "Vector3 getSize()", "Returns the current size Vector3.");

        LuaScriptApi::getInstance()->addClassToCollection("ProceduralGeometryComponent", "void setSegmentsH(float n)",
            "Sets the horizontal (angular/width) tessellation count (min 3). "
            "Affects all round shapes and the Plane. Triggers a mesh rebuild.");

        LuaScriptApi::getInstance()->addClassToCollection("ProceduralGeometryComponent", "void setSegmentsV(float n)",
            "Sets the vertical tessellation count (min 2). "
            "Affects Sphere, Capsule, Plane and the Torus tube. Triggers a mesh rebuild.");

        LuaScriptApi::getInstance()->addClassToCollection("ProceduralGeometryComponent", "void setFlipNormals(bool flip)",
            "If true, reverses face winding so normals point inward. "
            "Use for skybox cubes, inside-out rooms, etc. Triggers a mesh rebuild.");

        LuaScriptApi::getInstance()->addClassToCollection("ProceduralGeometryComponent", "void setDatablock(string name)",
            "Assigns a PBS datablock material by name. "
            "This is a live swap and does NOT trigger a full mesh rebuild.");

        LuaScriptApi::getInstance()->addClassToCollection("ProceduralGeometryComponent", "void setUVTiling(Vector2 tiling)",
            "Sets the UV tiling multiplier for all faces (x=U repeat, y=V repeat). "
            "Triggers a mesh rebuild.");

        LuaScriptApi::getInstance()->addClassToCollection("ProceduralGeometryComponent", "void rebuildMesh()",
            "Forces a complete geometry rebuild from current parameter values. "
            "Call this after making several changes via Lua to rebuild only once.");

        // ── Register on GameObject and GameObjectController ────────────────────
        gameObjectClass.def("getProceduralGeometryComponent", (ProceduralGeometryComponent * (*)(GameObject*)) & getProceduralGeometryComponent);
        gameObjectClass.def("getProceduralGeometryComponentFromName", &getProceduralGeometryComponentFromName);
        gameObjectControllerClass.def("castProceduralGeometryComponent", &GameObjectController::cast<ProceduralGeometryComponent>);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ProceduralGeometryComponent getProceduralGeometryComponent()", "Gets the first ProceduralGeometryComponent from this GameObject.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ProceduralGeometryComponent getProceduralGeometryComponentFromName(string name)", "Gets a named ProceduralGeometryComponent from this GameObject.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "ProceduralGeometryComponent castProceduralGeometryComponent(ProceduralGeometryComponent other)", "Casts for Lua auto-completion support.");
    }

} // namespace NOWA

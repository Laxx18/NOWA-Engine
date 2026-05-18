/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#ifndef PROCEDURAL_GEOMETRY_COMPONENT_H
#define PROCEDURAL_GEOMETRY_COMPONENT_H

#include "OgrePlugin.h"
#include "gameobject/GeometricComponentBase.h"

namespace NOWA
{
    /**
     * @class ProceduralGeometryComponent
     * @brief Click-to-place procedural geometry primitive component.
     *
     * Supported shapes (11):
     *   Box      – cuboid, the fundamental building block
     *   Pyramid  – rectangular base, apex at the top
     *   Sphere   – UV sphere with configurable slices/stacks
     *   Cylinder – with capped ends and configurable slices
     *   Cone     – circular base, apex at the top
     *   Capsule  – cylinder with hemispherical caps (great for characters)
     *   Torus    – donut / ring, configurable tube radius
     *   Plane    – subdivided flat surface, ideal for custom terrain patches
     *   Prism    – triangular cross-section extruded along Y (ramps, roofs)
     *   Disc     – flat circle; set size.z > 0 for a ring / annulus
     *   Wedge    – inclined ramp / wedge (slopes, stairs, architectural bevels)
     *
     * Workflow (NOWA-Design):
     *   1. Add the component to a GameObject.  The editor enters "Geometry Modify
     *      Mode" and a ghost preview mesh follows the mouse cursor.
     *   2. Left-click on terrain / any surface to confirm placement (the
     *      GameObject snaps to the hit position).
     *   3. After placement, every property change (shape, size, segments …)
     *      immediately rebuilds the mesh with full undo/redo support.
     *   4. Press ESC or right-click to cancel placement without moving the GO.
     *   5. "Convert To Mesh" bakes the geometry to a static .mesh file and
     *      replaces this component with a plain MeshComponent (one-way, permanent).
     *
     * Render thread safety:
     *   All Ogre-Next calls go through GraphicsModule::enqueueAndWait().
     *   Input listener registration is likewise render-thread-safe.
     *
     * Vertex format:  pos(3) + normal(3) + tangent(4) + uv(2)  =  12 floats
     */
    class EXPORTED ProceduralGeometryComponent : public GeometricComponentBase, public Ogre::Plugin
    {
    public:
        typedef boost::shared_ptr<ProceduralGeometryComponent> ProceduralGeometryComponentPtr;

        // ── Shape enum ────────────────────────────────────────────────────────
        enum class GeometryShape
        {
            BOX = 0,      ///< Cuboid  (size = width × height × depth)
            PYRAMID = 1,  ///< Rectangular-base pyramid  (size = baseW × height × baseD)
            SPHERE = 2,   ///< UV sphere  (size.x = radius)
            CYLINDER = 3, ///< Cylinder with caps  (size.x = radius, size.y = height)
            CONE = 4,     ///< Cone with base cap  (size.x = radius, size.y = height)
            CAPSULE = 5,  ///< Cylinder + hemi-caps  (size.x = radius, size.y = body height)
            TORUS = 6,    ///< Donut  (size.x = major radius, size.z = tube radius)
            PLANE = 7,    ///< Subdivided flat quad  (size.x = width, size.z = depth)
            PRISM = 8,    ///< Triangular prism  (size = baseW × height × baseD)
            DISC = 9,     ///< Flat circle / ring  (size.x = outer R, size.z = inner R ≥ 0)
            WEDGE = 10    ///< Inclined ramp  (size = width × height × depth)
        };

        // ── Build-state machine ───────────────────────────────────────────────
        enum class BuildState
        {
            IDLE = 0,   ///< Geometry exists; react only to property changes
            PLACING = 1 ///< Preview ghost follows cursor; waiting for LMB confirm
        };

    public:
        ProceduralGeometryComponent();
        virtual ~ProceduralGeometryComponent();

        // ── Ogre::Plugin interface ────────────────────────────────────────────
        virtual void install(const Ogre::NameValuePairList* options) override;
        virtual void initialise() override;
        virtual void shutdown() override;
        virtual void uninstall() override;
        virtual const Ogre::String& getName() const override;
        virtual void getAbiCookie(Ogre::AbiCookie& outAbiCookie) override;

        // ── GameObjectComponent interface ─────────────────────────────────────
        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;
        virtual bool postInit(void) override;
        virtual bool connect(void) override;
        virtual bool disconnect(void) override;
        virtual bool onCloned(void) override;
        virtual void onAddComponent(void) override;
        virtual void onRemoveComponent(void) override;
        virtual void onOtherComponentRemoved(unsigned int index) override;
        virtual void onOtherComponentAdded(unsigned int index) override;
        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;
        virtual void update(Ogre::Real dt, bool notSimulating = false) override;
        virtual void actualizeValue(Variant* attribute) override;
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        /**
         * @see GeometricComponentBase::getConvexParts
         *
         * Per-shape mappings:
         *   Box      → 1 Box primitive
         *   Pyramid  → 1 ConvexHull  (5 verts: 4 base corners + apex)
         *   Sphere   → 1 Ellipsoid
         *   Cylinder → 1 Cylinder
         *   Cone     → 1 Cone
         *   Capsule  → 1 Capsule
         *   Torus    → 8 Cylinder parts arranged around the major ring
         *   Plane    → 1 Box  (0.05 unit thick)
         *   Prism    → 1 ConvexHull  (6 verts)
         *   Disc     → 1 Cylinder  (thin)
         *   Wedge    → 1 ConvexHull  (6 verts)
         */
        virtual std::vector<GeometricComponentBase::ConvexPart> getConvexParts(void) const override;

        virtual bool hasConvexParts(void) const override;

        // ── Static registration helpers ───────────────────────────────────────
        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("ProceduralGeometryComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "ProceduralGeometryComponent";
        }

        static bool canStaticAddComponent(GameObject* gameObject);

        static Ogre::String getStaticInfoText(void)
        {
            return "Usage: Creates a procedural geometry primitive directly in the viewport.\n\n"
                   "PLACEMENT:\n"
                   "- When the component is added, a ghost preview mesh follows the mouse cursor.\n"
                   "- Left-click on any surface to confirm placement (snaps the GameObject there).\n"
                   "- Right-click or press ESC to cancel without moving the GameObject.\n\n"
                   "SHAPES:\n"
                   "- Box      : standard cuboid.  Size = width × height × depth.\n"
                   "- Pyramid  : rectangular base, apex at top.  Size = baseW × height × baseD.\n"
                   "- Sphere   : UV sphere.  Size.x = radius.  Use Segments H/V for smoothness.\n"
                   "- Cylinder : capped cylinder.  Size.x = radius, Size.y = height.\n"
                   "- Cone     : cone with base cap.  Size.x = radius, Size.y = height.\n"
                   "- Capsule  : cylinder + hemispherical caps.  Size.x = radius, Size.y = body height.\n"
                   "- Torus    : donut / ring.  Size.x = major radius, Size.z = tube radius.\n"
                   "- Plane    : subdivided flat quad.  Size.x = width, Size.z = depth.\n"
                   "- Prism    : triangular prism (isosceles triangle extruded).  Size = baseW × height × baseD.\n"
                   "- Disc     : flat circle.  Size.x = outer radius.  Set Size.z > 0 for a ring/annulus.\n"
                   "- Wedge    : inclined ramp.  Size = width × height × depth.\n\n"
                   "EDITING:\n"
                   "- Every property change instantly rebuilds the mesh; full undo/redo is supported.\n"
                   "- 'Flip Normals' inverts face winding (useful for inside-out geometry, skyboxes).\n"
                   "- Segments H controls horizontal/angular resolution.  Segments V controls vertical.\n"
                   "- UV Tiling scales texture coordinates across all faces.\n\n"
                   "CONVERT TO MESH:\n"
                   "- Bakes the current shape to a static .mesh file and replaces this component\n"
                   "  with a plain MeshComponent for optimal runtime performance.\n"
                   "- This operation is permanent and cannot be undone procedurally.\n\n"
                   "LUA API:\n"
                   "- getProceduralGeometryComponent() on a GameObject returns this component.\n"
                   "- setShape(name) sets the primitive: 'Box', 'Sphere', 'Cylinder', etc.\n"
                   "- setSize(Vector3) sets shape dimensions (interpretation is per-shape).\n"
                   "- setSegmentsH(n) / setSegmentsV(n) control mesh tessellation.\n"
                   "- setDatablock(name) assigns a PBS datablock material.\n"
                   "- setUVTiling(Vector2) controls texture repeat.\n"
                   "- setFlipNormals(bool) inverts face normals.\n"
                   "- rebuildMesh() forces a full geometry rebuild.";
        }

        static std::optional<NOWA::GameObjectTypeDescriptor> getStaticTypeDescriptor()
        {
            NOWA::GameObjectTypeDescriptor desc;
            desc.type = eType::CUSTOM;
            desc.displayName = "Geometry";
            desc.meshToDisplay = "Node.mesh";
            desc.needsMeshItem = false;
            desc.enterMeshModifyMode = false;
            desc.autoComponents = {"ProceduralGeometryComponent"};
            desc.guardWithPluginCheck = true;
            return desc;
        }

        virtual Ogre::String getClassName(void) const override
        {
            return "ProceduralGeometryComponent";
        }
        virtual Ogre::String getParentClassName(void) const override
        {
            return "GameObjectComponent";
        }
        virtual Ogre::String getParentParentClassName(void) const override
        {
            return "GameObjectComponent";
        }

        // ── Static Lua API registration ───────────────────────────────────────
        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

        // ── Mesh operations ───────────────────────────────────────────────────
        void rebuildMesh(void);

        // ── Attribute setters / getters ───────────────────────────────────────
        void setActivated(bool activated);
        bool isActivated(void) const;

        void setShape(const Ogre::String& shapeName);
        Ogre::String getShape(void) const;

        void setSize(const Ogre::Vector3& size);
        Ogre::Vector3 getSize(void) const;

        void setSegmentsH(Ogre::Real segments);
        Ogre::Real getSegmentsH(void) const;

        void setSegmentsV(Ogre::Real segments);
        Ogre::Real getSegmentsV(void) const;

        void setFlipNormals(bool flip);
        bool getFlipNormals(void) const;

        void setDatablock(const Ogre::String& datablockName);
        Ogre::String getDatablock(void) const;

        void setUVTiling(const Ogre::Vector2& tiling);
        Ogre::Vector2 getUVTiling(void) const;

    public:
        // ── Static attribute name strings ─────────────────────────────────────
        static Ogre::String AttrActivated(void)
        {
            return "Activated";
        }
        static Ogre::String AttrShape(void)
        {
            return "Shape";
        }
        static Ogre::String AttrSize(void)
        {
            return "Size";
        }
        static Ogre::String AttrSegmentsH(void)
        {
            return "Segments H";
        }
        static Ogre::String AttrSegmentsV(void)
        {
            return "Segments V";
        }
        static Ogre::String AttrFlipNormals(void)
        {
            return "Flip Normals";
        }
        static Ogre::String AttrDatablock(void)
        {
            return "Datablock";
        }
        static Ogre::String AttrUVTiling(void)
        {
            return "UV Tiling";
        }
        static Ogre::String AttrConvertToMesh(void)
        {
            return "Convert To Mesh";
        }

    protected:
        virtual bool executeAction(const Ogre::String& actionId, NOWA::Variant* attribute) override;

    private:
        // ── High-level mesh pipeline ──────────────────────────────────────────
        void buildGeometry(void); // populate vertices / indices on CPU
        void createGeometryMesh(void);
        void createGeometryMeshInternal(const std::vector<float>& vertData, const std::vector<Ogre::uint32>& idxData, size_t numVerts, const Ogre::String& meshName);
        void destroyGeometryMesh(void);

        // ── Preview mesh (ghost that follows cursor during PLACING) ───────────
        void createPreviewMesh(void);
        void updatePreviewPosition(const Ogre::Vector3& worldPos);
        void destroyPreviewMesh(void);

        // ── Per-shape geometry generators ─────────────────────────────────────
        void generateBox(void);
        void generatePyramid(void);
        void generateSphere(void);
        void generateCylinder(void);
        void generateCone(void);
        void generateCapsule(void);
        void generateTorus(void);
        void generatePlane(void);
        void generatePrism(void);
        void generateDisc(void);
        void generateWedge(void);

        // ── Low-level vertex/index helpers ────────────────────────────────────
        /**
         * @brief Append one vertex (8 raw floats: pos3 normal3 uv2).
         *        Tangents are computed later in createGeometryMeshInternal.
         */
        void gv(Ogre::Real px, Ogre::Real py, Ogre::Real pz, Ogre::Real nx, Ogre::Real ny, Ogre::Real nz, Ogre::Real u, Ogre::Real v);

        /**
         * @brief Append one triangle (indices relative to m_vertBase).
         *        Winding is CCW (right-hand-rule outward normals).
         *        If flipNormals is set, winding is reversed automatically.
         */
        void gt(Ogre::uint32 i0, Ogre::uint32 i1, Ogre::uint32 i2);

        /// Append a quad as two triangles (i0,i1,i2) + (i0,i2,i3).
        void gq(Ogre::uint32 i0, Ogre::uint32 i1, Ogre::uint32 i2, Ogre::uint32 i3);

        // ── Enum helpers ──────────────────────────────────────────────────────
        GeometryShape getShapeEnum(void) const;

        // ── Mesh export / bake ────────────────────────────────────────────────
        bool convertToMeshApply(void);
        bool exportMesh(const Ogre::String& filename);

    private:
        Ogre::String name;

        // ── Attributes (Variant*) ─────────────────────────────────────────────
        Variant* activated;
        Variant* shape;
        Variant* size;
        Variant* segmentsH;
        Variant* segmentsV;
        Variant* flipNormals;
        Variant* datablock;
        Variant* uvTiling;
        Variant* convertToMesh;

        // ── CPU-side geometry buffers ─────────────────────────────────────────
        std::vector<float> vertices; ///< 8 floats/vertex: pos3 + normal3 + uv2
        std::vector<Ogre::uint32> indices;
        Ogre::uint32 vertBase; ///< vertex-base for current sub-generator call

        // ── Ogre-Next scene objects ───────────────────────────────────────────
        Ogre::MeshPtr geomMesh;
        Ogre::Item* geomItem;

        Ogre::MeshPtr previewMesh;
        Ogre::Item* previewItem;
        Ogre::SceneNode* previewNode;

        // ── Physics (optional) ────────────────────────────────────────────────
        PhysicsArtifactComponent* physicsArtifactComponent;
    };

} // namespace NOWA

#endif // PROCEDURAL_GEOMETRY_COMPONENT_H

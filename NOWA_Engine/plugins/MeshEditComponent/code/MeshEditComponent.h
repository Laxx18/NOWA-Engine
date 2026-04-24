/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#ifndef MESHEDITCOMPONENT_H
#define MESHEDITCOMPONENT_H

#include "OgrePlugin.h"
#include "gameobject/MeshEditComponentBase.h"
#include "main/Events.h"
#include "editor/ScreenRectSelector.h"

#include <set>
#include <unordered_map>
#include <unordered_set>

namespace Ogre
{
    class VertexBufferPacked;
    class IndexBufferPacked;
    class ManualObject;
}

namespace NOWA
{
    /**
     * @brief   Blender-inspired offline mesh editing component for NOWA-Engine.
     *
     *          Workflow:
     *           1. Add component  -> original Ogre::Item is REPLACED by an editable clone
     *              (the original .mesh resource is never touched).
     *           2. Select a mode (Vertex / Edge / Face) and edit interactively.
     *           3. Press "Apply Mesh" -> exports the result to <OutputFileName>.mesh.
     *           4. Remove component -> original mesh item is restored on the GameObject.
     *
     *          Editing is only active when:
     *            - the component is Activated
     *            - the editor is in EDITOR_MESH_MODIFY_MODE
     *            - this GameObject is selected
     *
     *          Mouse (active in non-OBJECT mode):
     *           LMB              : select topology (Shift=add, Ctrl=remove)
     *           LMB drag (VERTEX): brush sculpt
     *           RMB (while grab) : cancel grab
     *
     *          Keyboard (active in non-OBJECT mode):
     *           G      : Grab — move selection along camera plane. LMB/Enter=confirm, RMB/Esc=cancel.
     *           X      : Delete selected topology.
     *           E      : Extrude selected faces (Face mode).
     *           I      : Subdivide selected faces (Face mode) / selected edges or vertices.
     *           F      : Flip normals of entire mesh.
     *           B      : Toggle sculpt-brush mode (Vertex mode only).
     *           A      : Toggle select-all / deselect-all (Ctrl+A).
     *           Ctrl+I : Subdivide ALL faces.
     *           Ctrl+N : Recalculate normals (smooth, area-weighted).
     *           Ctrl+Z : Undo (up to 16 levels).
     *           Esc    : Cancel grab OR deselect-all.
     */
    class EXPORTED MeshEditComponent : public MeshEditComponentBase, public Ogre::Plugin, public OIS::MouseListener, public OIS::KeyListener
    {
    public:
        typedef boost::shared_ptr<MeshEditComponent> MeshEditComponentPtr;

        // ── Enums ──────────────────────────────────────────────────────────────
        enum class EditMode
        {
            OBJECT,
            VERTEX,
            EDGE,
            FACE
        };
        enum class ActiveTool
        {
            SELECT,
            GRAB
        };
        enum class BrushMode
        {
            PUSH,
            PULL,
            SMOOTH,
            FLATTEN,
            PINCH,
            INFLATE
        };

        // ── Internal structures ────────────────────────────────────────────────
        struct VertexElementInfo
        {
            bool hasPosition = false;
            bool hasNormal = false;
            bool hasTangent = false;
            bool hasQTangent = false;
            bool hasUV = false;
            Ogre::VertexElementType positionType = Ogre::VET_FLOAT3;
            Ogre::VertexElementType normalType = Ogre::VET_FLOAT3;
            Ogre::VertexElementType tangentType = Ogre::VET_FLOAT4;
            Ogre::VertexElementType uvType = Ogre::VET_FLOAT2;
        };

        struct SubMeshInfo
        {
            size_t vertexOffset = 0;
            size_t vertexCount = 0;
            size_t indexOffset = 0;
            size_t indexCount = 0;
            bool hasTangent = false;
            size_t floatsPerVertex = 8;

            Ogre::VertexBufferPacked* dynamicVertexBuffer = nullptr;
            Ogre::IndexBufferPacked* dynamicIndexBuffer = nullptr;
        };

        /// Canonical undirected edge key: a <= b always
        struct EdgeKey
        {
            size_t a, b;
            EdgeKey(size_t x, size_t y) : a(x < y ? x : y), b(x < y ? y : x)
            {
            }
            bool operator<(const EdgeKey& o) const
            {
                return a < o.a || (a == o.a && b < o.b);
            }
            bool operator==(const EdgeKey& o) const
            {
                return a == o.a && b == o.b;
            }
        };

    public:
        MeshEditComponent();
        virtual ~MeshEditComponent();

        // ── Ogre::Plugin ──────────────────────────────────────────────────────
        virtual void install(const Ogre::NameValuePairList* options) override;
        virtual void initialise() override
        {
        }
        virtual void shutdown() override
        {
        }
        virtual void uninstall() override
        {
        }
        virtual const Ogre::String& getName() const override;
        virtual void getAbiCookie(Ogre::AbiCookie& outAbiCookie) override;

        // ── GameObjectComponent ───────────────────────────────────────────────
        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;
        virtual bool postInit(void) override;
        virtual bool connect(void) override;
        virtual bool disconnect(void) override;
        virtual bool onCloned(void) override;
        virtual void onAddComponent(void) override;
        virtual void onRemoveComponent(void) override;
        virtual void onOtherComponentRemoved(unsigned int index) override;
        virtual void onOtherComponentAdded(unsigned int index) override;
        virtual Ogre::String getClassName(void) const override;
        virtual Ogre::String getParentClassName(void) const override;
        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;
        virtual void update(Ogre::Real dt, bool notSimulating = false) override;
        virtual void actualizeValue(Variant* attribute) override;
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;
        void writeMesh();
        virtual bool executeAction(const Ogre::String& actionId, NOWA::Variant* attribute) override;
        virtual void setActivated(bool activated) override;
        virtual bool isActivated(void) const override;

        // ── Edit mode ─────────────────────────────────────────────────────────
        void setEditMode(const Ogre::String& mode);
        Ogre::String getEditModeString(void) const;
        EditMode getEditMode(void) const;

        // ── Selection ─────────────────────────────────────────────────────────
        void selectAll(void);
        void deselectAll(void);
        void selectVertex(size_t idx, bool addToSelection = false);
        void selectEdge(size_t a, size_t b, bool addToSelection = false);
        void selectFace(size_t faceIdx, bool addToSelection = false);
        const std::set<size_t>& getSelectedVertices(void) const;
        const std::set<EdgeKey>& getSelectedEdges(void) const;
        const std::set<size_t>& getSelectedFaces(void) const;

        // ── Destructive operations ────────────────────────────────────────────
        void deleteSelected(void);
        void deleteSelectedVertices(void);
        void deleteSelectedEdges(void);
        void deleteSelectedFaces(void);

        // ── Transform ────────────────────────────────────────────────────────
        void moveSelected(const Ogre::Vector3& localDelta);

        // ── Extra operations ──────────────────────────────────────────────────
        void mergeByDistance(Ogre::Real threshold = 0.001f);
        void flipNormals(void);
        void recalculateNormals(void);
        void subdivideSelectedFaces(void);
        void extrudeSelectedFaces(Ogre::Real amount = 0.1f);

        /// Subdivides every face in the mesh uniformly (no selection required).
        void subdivideAll(void);

        /**
         * @brief Mirrors the mesh across the chosen axis and welds the seam.
         * @param axis One of "+X", "-X", "+Y", "-Y", "+Z", "-Z".
         *             The sign indicates which half of the mesh is kept as the source;
         *             the reflected copy is appended and the shared edge is welded.
         */
        void mirrorMesh(const Ogre::String& axis);

        /**
         * @brief Bakes the scene-node's current world scale into vertex positions,
         *        then resets the node scale to (1,1,1).
         *        Useful before export or physics setup when an accidental scale exists.
         */
        void applyScale(void);

        // ── Undo (EditorManager integration) ─────────────────────────────────
        /// Serialise current CPU mesh state to a flat byte buffer.
        std::vector<unsigned char> getMeshData(void) const;
        /// Restore CPU mesh state from a previously serialised buffer, then rebuild GPU buffers.
        void setMeshData(const std::vector<unsigned char>& data);
        /// Fire EventDataMeshEditModifyEnd so EditorManager pushes one undo entry.
        /// @param oldData  Snapshot captured BEFORE the modification.
        void fireUndoEvent(const std::vector<unsigned char>& oldData);

        // ── Brush API (mirrors MeshModifyComponent) ───────────────────────────
        void setBrushName(const Ogre::String& brushName);
        Ogre::String getBrushName(void) const;
        void setBrushSize(Ogre::Real v);
        Ogre::Real getBrushSize(void) const;
        void setBrushIntensity(Ogre::Real v);
        Ogre::Real getBrushIntensity(void) const;
        void setBrushFalloff(Ogre::Real v);
        Ogre::Real getBrushFalloff(void) const;
        void setBrushMode(const Ogre::String& mode);
        Ogre::String getBrushModeString(void) const;
        BrushMode getBrushMode(void) const;

        void setProportionalRadius(Ogre::Real radius);
        Ogre::Real getProportionalRadius(void) const;
        bool getIsProportionalEditing(void) const;

        void setBevelAmount(Ogre::Real amount);
        Ogre::Real getBevelAmount(void) const;

        void setLoopCutFraction(Ogre::Real fraction);
        Ogre::Real getLoopCutFraction(void) const;

        // ── Extrude parameter ─────────────────────────────────────────────────
        void setExtrudeAmount(Ogre::Real amount);
        Ogre::Real getExtrudeAmount(void) const;

        // ── Mirror axis parameter ─────────────────────────────────────────────
        void setMirrorAxis(const Ogre::String& axis);
        Ogre::String getMirrorAxis(void) const;

        void setOriginAlignment(const Ogre::String& preset);
        Ogre::String getOriginAlignment(void) const;

        void setFlipDirection(const Ogre::String& axis);
        Ogre::String getFlipDirection(void) const;

        // Places a new vertex at the given world position and selects it.
        // Called from mousePressed when addVertexMode is active.
        void addVertexAtPosition(const Ogre::Vector3& localPos);

        // Creates triangles from exactly 3 or 4 currently selected vertices.
        // 3 selected → 1 triangle.
        // 4 selected → 2 triangles (split on the shorter diagonal for best shape).
        // Returns false if fewer than 3 or more than 4 vertices are selected.
        bool buildFaceFromSelection(void);

        // ── Export / Apply ────────────────────────────────────────────────────
        bool exportMesh(const Ogre::String& fileNameOverride = "");
        bool applyMesh(void); ///< Called by executeAction
        void setOutputFileName(const Ogre::String& name);
        Ogre::String getOutputFileName(void) const;

        // ── Info ──────────────────────────────────────────────────────────────
        size_t getVertexCount(void) const;
        size_t getIndexCount(void) const;
        size_t getTriangleCount(void) const;

    public:
        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("MeshEditComponent");
        }
        static Ogre::String getStaticClassName(void)
        {
            return "MeshEditComponent";
        }
        static bool canStaticAddComponent(GameObject* gameObject);
        static Ogre::String getStaticInfoText(void)
        {
            return "== Setup ==\n"
                   "1. Add MeshEditComponent to a mesh GameObject.\n"
                   "2. Click the GameObject to activate listeners.\n"
                   "3. Set Edit Mode dropdown to Vertex / Edge / Face.\n"
                   "\n"
                   "== Edit Modes ==\n"
                   "Object : Gizmo-only, overlay hidden.\n"
                   "Vertex : Select vertices. Grab, Scale, Brush, Rect-select.\n"
                   "Edge   : Select edges.   Grab, Bevel, Dissolve, Loop Cut.\n"
                   "Face   : Select faces.   Grab, Extrude, Subdivide, Fill.\n"
                   "\n"
                   "== Selection ==\n"
                   "LMB              - Select element.\n"
                   "Shift+LMB        - Add to selection.\n"
                   "Ctrl+LMB         - Remove from selection.\n"
                   "LMB drag (empty) - Rectangle select.\n"
                   "Shift+LMB drag   - Add rect to selection.\n"
                   "Ctrl+LMB drag    - Remove rect from selection.\n"
                   "L (hover)        - Select linked (flood fill).\n"
                   "Shift+L          - Add linked to selection.\n"
                   "Ctrl+A           - Select all / Deselect all.\n"
                   "\n"
                   "== Grab (G) ==\n"
                   "G        - Grab: move freely in camera plane.\n"
                   "  X/Y/Z  - Lock to world axis (press again: free).\n"
                   "           Note: Z key = Y axis (up), Y key = Z axis.\n"
                   "  S      - Scale mode (drag left/right).\n"
                   "  Enter  - Confirm (pushes undo record).\n"
                   "  Esc    - Cancel (restores original positions).\n"
                   "O        - Toggle Proportional Editing.\n"
                   "\n"
                   "== Sculpt Brush (Vertex mode) ==\n"
                   "B            - Toggle brush on/off.\n"
                   "LMB drag     - Paint stroke (one undo per stroke).\n"
                   "Ctrl+LMB     - Invert Push/Pull.\n"
                   "Modes: Push Pull Smooth Flatten Pinch Inflate.\n"
                   "\n"
                   "== Keys ==\n"
                   "X       - Delete selected.\n"
                   "Ctrl+D  - Dissolve selected.\n"
                   "M       - Merge to centroid.\n"
                   "E       - Extrude faces (Face mode).\n"
                   "I       - Subdivide selected faces.\n"
                   "Ctrl+I  - Subdivide all.\n"
                   "F       - Flip normals.\n"
                   "Ctrl+N  - Recalculate normals.\n"
                   "Ctrl+B  - Bevel edges.\n"
                   "Ctrl+R  - Loop cut.\n"
                   "Ctrl+F  - Fill border loop.\n"
                   "Ctrl+Z  - Undo (16 levels).\n"
                   "\n"
                   "== Buttons ==\n"
                   "Weld Vertices / Flip Normals / Recalculate\n"
                   "Subdivide Sel. / Subdivide All\n"
                   "Extrude Sel. / Mirror Mesh / Apply Scale\n"
                   "Merge Sel. / Dissolve / Fill / Bevel / Loop Cut\n"
                   "Apply Mesh - export to .mesh file.\n"
                   "Cancel Edit - restore original mesh.";
        }
        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

    public:
        static const Ogre::String AttrActivated(void)
        {
            return "Activated";
        }
        static const Ogre::String AttrEditMode(void)
        {
            return "Edit Mode";
        }
        static const Ogre::String AttrShowWireframe(void)
        {
            return "Show Wireframe";
        }
        static const Ogre::String AttrXRayOverlay(void)
        {
            return "X-Ray Overlay";
        }
        static const Ogre::String AttrVertexMarkerSize(void)
        {
            return "Vertex Marker Size";
        }
        static const Ogre::String AttrOutputFileName(void)
        {
            return "Output File Name";
        }
        static const Ogre::String AttrApplyMesh(void)
        {
            return "Apply Mesh";
        }
        static const Ogre::String AttrBrushName(void)
        {
            return "Brush Name";
        }
        static const Ogre::String AttrBrushSize(void)
        {
            return "Brush Size";
        }
        static const Ogre::String AttrBrushIntensity(void)
        {
            return "Brush Intensity";
        }
        static const Ogre::String AttrBrushFalloff(void)
        {
            return "Brush Falloff";
        }
        static const Ogre::String AttrBrushMode(void)
        {
            return "Brush Mode";
        }
        static const Ogre::String ActionApplyMesh(void)
        {
            return "MeshEditComponent.ApplyMesh";
        }
        static const Ogre::String AttrCancelEdit(void)
        {
            return "Cancel Edit";
        }
        static const Ogre::String ActionCancelEdit(void)
        {
            return "MeshEditComponent.CancelEdit";
        }
        static const Ogre::String AttrWeldVertices(void)
        {
            return "Weld Vertices";
        }
        static const Ogre::String ActionWeldVertices(void)
        {
            return "MeshEditComponent.WeldVertices";
        }

        // ── Normals ───────────────────────────────────────────────────────────
        static const Ogre::String AttrFlipNormals(void)
        {
            return "Flip Normals";
        }
        static const Ogre::String ActionFlipNormals(void)
        {
            return "MeshEditComponent.FlipNormals";
        }
        static const Ogre::String AttrRecalcNormals(void)
        {
            return "Recalculate Normals";
        }
        static const Ogre::String ActionRecalcNormals(void)
        {
            return "MeshEditComponent.RecalcNormals";
        }

        // ── Subdivide ─────────────────────────────────────────────────────────
        static const Ogre::String AttrSubdivideFaces(void)
        {
            return "Subdivide Selected";
        }
        static const Ogre::String ActionSubdivideFaces(void)
        {
            return "MeshEditComponent.SubdivideFaces";
        }
        static const Ogre::String AttrSubdivideAll(void)
        {
            return "Subdivide All";
        }
        static const Ogre::String ActionSubdivideAll(void)
        {
            return "MeshEditComponent.SubdivideAll";
        }

        // ── Extrude ───────────────────────────────────────────────────────────
        static const Ogre::String AttrExtrudeAmount(void)
        {
            return "Extrude Amount";
        }
        static const Ogre::String AttrExtrudeFaces(void)
        {
            return "Extrude Selected";
        }
        static const Ogre::String ActionExtrudeFaces(void)
        {
            return "MeshEditComponent.ExtrudeFaces";
        }

        // ── Mirror ────────────────────────────────────────────────────────────
        static const Ogre::String AttrMirrorAxis(void)
        {
            return "Mirror Axis";
        }
        static const Ogre::String AttrMirrorMesh(void)
        {
            return "Mirror Mesh";
        }
        static const Ogre::String ActionMirrorMesh(void)
        {
            return "MeshEditComponent.MirrorMesh";
        }

        // ── Apply Scale ───────────────────────────────────────────────────────
        static const Ogre::String AttrApplyScale(void)
        {
            return "Apply Scale";
        }
        static const Ogre::String ActionApplyScale(void)
        {
            return "MeshEditComponent.ApplyScale";
        }
        static Ogre::String AttrProportionalRadius()
        {
            return "Proportional Radius";
        }
        static Ogre::String AttrBevelAmount()
        {
            return "Bevel Amount";
        }
        static Ogre::String AttrLoopCutFraction()
        {
            return "Loop Cut Fraction";
        }
        static Ogre::String AttrMergeSelected()
        {
            return "Merge Selected";
        }
        static Ogre::String AttrDissolveSelected()
        {
            return "Dissolve";
        }
        static Ogre::String AttrFillSelected()
        {
            return "Fill";
        }
        static Ogre::String AttrBevel()
        {
            return "Bevel";
        }
        static Ogre::String AttrLoopCut()
        {
            return "Loop Cut";
        }

        static Ogre::String ActionMergeSelected()
        {
            return "MergeSelected";
        }
        static Ogre::String ActionDissolveSelected()
        {
            return "DissolveSelected";
        }
        static Ogre::String ActionFillSelected()
        {
            return "FillSelected";
        }
        static Ogre::String ActionBevel()
        {
            return "Bevel";
        }
        static Ogre::String ActionLoopCut()
        {
            return "LoopCut";
        }
        static Ogre::String AttrAddVertex()
        {
            return "Add Vertex Mode";
        }
        static Ogre::String ActionBuildFace()
        {
            return "BuildFace";
        }
        static Ogre::String AttrBuildFace()
        {
            return "Build Face";
        }

        static Ogre::String AttrOriginAlignment()
        {
            return "Origin Alignment";
        }
        static Ogre::String AttrFlipDirection()
        {
            return "Flip Direction";
        }
        static Ogre::String ActionApplyOriginAlignment()
        {
            return "ApplyOriginAlignment";
        }
        static Ogre::String ActionApplyFlipDirection()
        {
            return "ApplyFlipDirection";
        }
        static Ogre::String AttrShowOrigin()
        {
            return "Show Origin";
        }

    protected:
        // OIS::MouseListener
        virtual bool mouseMoved(const OIS::MouseEvent& evt) override;
        virtual bool mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;
        virtual bool mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;
        // OIS::KeyListener
        virtual bool keyPressed(const OIS::KeyEvent& evt) override;
        virtual bool keyReleased(const OIS::KeyEvent& evt) override;

    private:
        struct OverlayVertex
        {
            Ogre::Vector3     pos;
            Ogre::ColourValue color;
        };

        // ── Mesh preparation ──────────────────────────────────────────────────
        bool prepareEditableMesh(void);
        bool extractMeshData(void);
        bool createDynamicBuffers(void);
        bool rebuildDynamicBuffers(void);
        void destroyEditableMesh(void);
        void uploadVertexData(void);
        bool detectVertexFormat(Ogre::VertexArrayObject* vao);

        // ── Normals / tangents ────────────────────────────────────────────────
        void recalculateNormals_internal(void);
        void recalculateTangents(void);
        void buildVertexAdjacency(void);

        // =============================================================================
        //  buildPositionGroups
        //
        //  Builds vertexGroup[i] -> group index and positionGroups[g] -> {indices}.
        //  Must be called whenever vertex positions or topology change, i.e. at the
        //  end of buildVertexAdjacency().
        //
        //  Complexity: O(n^2) — fine for mesh editing meshes (typically < 50k verts).
        //  For very dense sculpts consider a spatial hash; for now O(n^2) is practical.
        // =============================================================================
        void buildPositionGroups(void);

        // ── Overlay ───────────────────────────────────────────────────────────
        void createOverlay(void);
        void destroyOverlay(void);
        void scheduleOverlayUpdate(void);

        // ── Brush ─────────────────────────────────────────────────────────────
        Ogre::Real calculateBrushInfluence(Ogre::Real distance, Ogre::Real radius) const;
        void applyBrush(const Ogre::Vector3& brushCenterLocal, bool invertEffect);

        // ── Picking ───────────────────────────────────────────────────────────
        bool pickVertex(Ogre::Real sx, Ogre::Real sy, size_t& outIdx);
        bool pickEdge(Ogre::Real sx, Ogre::Real sy, EdgeKey& outEdge);
        bool pickFace(Ogre::Real sx, Ogre::Real sy, size_t& outFaceIdx);
        bool raycastMesh(Ogre::Real sx, Ogre::Real sy, Ogre::Vector3& hitPos, Ogre::Vector3& hitNorm, size_t& hitTri);

        // ── Grab tool ─────────────────────────────────────────────────────────
        void beginGrab(void);
        void confirmGrab(void);
        void cancelGrab(void);
        void applyGrabMouseDelta(int dx, int dy);
        void applyGrabScale(int dx);
        bool findBorderLoop(std::vector<size_t>& outLoop);

        // ── Coordinate helpers ────────────────────────────────────────────────
        Ogre::Vector3 worldToLocal(const Ogre::Vector3& worldPos) const;
        Ogre::Vector3 localToWorld(const Ogre::Vector3& localPos) const;

        // ── Input / state management (ProceduralRoadComponent pattern) ────────
        void addInputListener(void);
        void removeInputListener(void);
        void updateModificationState(void);
        void cancelEdit(void);

        // ── Event callbacks ───────────────────────────────────────────────────
        void handleMeshModifyMode(NOWA::EventDataPtr eventData);
        void handleGameObjectSelected(NOWA::EventDataPtr eventData);

        // ── Helpers ───────────────────────────────────────────────────────────
        Ogre::String buildDefaultOutputName(void) const;

        /// Algorithm-only merge — no undo event fired. Used internally by
        /// mirrorMesh() and the public mergeByDistance() which wraps it.
        void mergeByDistance_noUndo(Ogre::Real threshold);

        void mergeSelected(void);
        void dissolveSelected(void);
        void dissolveSelectedVertices(void);
        void dissolveSelectedEdges(void);
        void fillSelected(void);
        void bevel(Ogre::Real amount);
        void loopCut(Ogre::Real fraction);

        void selectLinked(Ogre::Real sx, Ogre::Real sy, bool add);
        // helpers used by concrete IScreenRectSelectable adapters:
        Ogre::Matrix4 getScreenMatrix() const;
        bool projectVertexToScreen(size_t idx, float& nx, float& ny) const;

        // Applies the currently selected origin alignment preset to all vertices.
        // Translates all vertices so the chosen bounding-box point becomes (0,0,0).
        void applyOriginAlignment(void);

        // Flips (mirrors) all vertices across the chosen axis.
        // Also mirrors normals and fixes triangle winding to keep faces front-facing.
        void applyFlipDirection(void);

    private:
        Ogre::String componentName;

        // Original mesh is remembered only as a name — the item is destroyed on swap.
        Ogre::String originalMeshName;
        Ogre::Item* editableItem;
        Ogre::MeshPtr editableMesh;

        std::vector<SubMeshInfo> subMeshInfoList;
        Ogre::VertexBufferPacked* dynamicVertexBuffer;
        Ogre::IndexBufferPacked* dynamicIndexBuffer;

        // CPU data
        std::vector<Ogre::Vector3> vertices;
        std::vector<Ogre::Vector3> normals;
        std::vector<Ogre::Vector4> tangents;
        std::vector<Ogre::Vector2> uvCoordinates;
        std::vector<Ogre::uint32> indices;
        std::vector<std::vector<size_t>> vertexNeighbors;
        unsigned int meshRebuildCounter;

        VertexElementInfo vertexFormat;
        bool isIndices32;
        size_t vertexCount;
        size_t indexCount;

        // Selection
        std::set<size_t> selectedVertices;
        std::set<EdgeKey> selectedEdges;
        std::set<size_t> selectedFaces;

        // Grab tool state
        ActiveTool activeTool;
        bool isGrabbing;
        enum class GrabAxis
        {
            FREE,
            X,
            Y,
            Z
        };
        GrabAxis grabAxis;
        Ogre::Vector3 grabAccumDelta;
        std::vector<Ogre::Vector3> grabSavedPositions;
        std::vector<unsigned char> grabPreEditData; ///< full mesh snapshot taken at beginGrab for undo

        // Brush
        std::vector<Ogre::Real> brushData;
        size_t brushImageWidth;
        size_t brushImageHeight;
        bool isBrushArmed;
        bool isScaling;                             ///< true while scale mode active inside grab
        int grabScaleMouseAccum;                    ///< total mouse-X pixels since scale started

        // Brush stroke undo
        Ogre::Vector3 brushLastHitPos;                     ///< last hit position for distance throttle
        std::vector<unsigned char> brushStrokePreEditData; ///< snapshot taken at stroke start

        Ogre::Vector3 grabCentroid;                 ///< selection centroid computed at beginGrab

        // Proportional editing
        bool isProportionalEditing; ///< O-key toggle

        // Overlay (render thread owned, updated via tracked closure)
        Ogre::SceneNode* overlayNode;
        Ogre::ManualObject* overlayObject;

        // Mouse state
        bool isPressing;
        bool isShiftPressed;
        bool isCtrlPressed;

        // Activation state (mirrors ProceduralRoadComponent)
        bool isEditorMeshModifyMode; ///< set by handleMeshModifyMode
        bool isSelected;             ///< set by handleGameObjectSelected
        bool isSimulating;           ///< true between connect/disconnect
        bool overlayClosureRemoved;

        std::vector<int> vertexGroup;
        std::vector<std::vector<size_t>> positionGroups;

        ScreenRectSelector rectSelector;
        bool  isRectSelecting;
        float rectDragThreshold;   // px movement before rect starts (default 4)
        float rectPressX, rectPressY; // where LMB went down

        // Attributes
        Variant* activated;
        Variant* editMode;
        Variant* showWireframe;
        Variant* xRayOverlay;
        Variant* vertexMarkerSize;
        Variant* outputFileName;
        Variant* weldButton;
        // ── Normals ───────────────────────────────────────────────────────────
        Variant* flipNormalsButton;
        Variant* recalcNormalsButton;
        // ── Subdivide ─────────────────────────────────────────────────────────
        Variant* subdivideFacesButton;
        Variant* subdivideAllButton;
        // ── Extrude ───────────────────────────────────────────────────────────
        Variant* extrudeAmount;
        Variant* extrudeFacesButton;
        // ── Mirror ────────────────────────────────────────────────────────────
        Variant* mirrorAxis;
        Variant* mirrorMeshButton;
        // ── Apply Scale ───────────────────────────────────────────────────────
        Variant* applyScaleButton;

        Variant* proportionalRadius;
        Variant* bevelAmount;
        Variant* loopCutFraction;
        Variant* mergeSelectedButton;
        Variant* dissolveButton;
        Variant* fillButton;
        Variant* bevelButton;
        Variant* loopCutButton;

        // ── Brush ─────────────────────────────────────────────────────────────
        Variant* brushName;
        Variant* brushSize;
        Variant* brushIntensity;
        Variant* brushFalloff;
        Variant* brushMode;

        Variant* buildFaceButton; // "Build Face" action button

        Variant* originAlignment;     // ComboBox: 27 presets (Y/X/Z combined)
        Variant* originAlignButton;   // Button: Apply Origin
        Variant* flipDirection;       // ComboBox: X / Y / Z
        Variant* flipDirectionButton; // Button: Apply Flip
        Variant* showOrigin;

        // ── Export / misc ─────────────────────────────────────────────────────
        Variant* applyMeshButton;
        Variant* cancelEditButton;

        // Undo snapshots for multi-tick operations
        std::vector<unsigned char> grabUndoData;  ///< captured at beginGrab
        std::vector<unsigned char> brushUndoData; ///< captured at brush stroke start
    };

}; // namespace NOWA

#endif // MESHEDITCOMPONENT_H
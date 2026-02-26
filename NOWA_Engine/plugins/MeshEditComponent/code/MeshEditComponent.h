/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#ifndef MESHEDITCOMPONENT_H
#define MESHEDITCOMPONENT_H

#include "OgrePlugin.h"
#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"

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
     *           1. Add component  → original Ogre::Item is REPLACED by an editable clone
     *              (the original .mesh resource is never touched).
     *           2. Select a mode (Vertex / Edge / Face) and edit interactively.
     *           3. Press "Apply Mesh" → exports the result to <OutputFileName>.mesh.
     *           4. Remove component → original mesh item is restored on the GameObject.
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
     *           A      : Toggle select-all / deselect-all.
     *           Ctrl+Z : Undo (up to 16 levels).
     *           Esc    : Cancel grab OR deselect-all.
     */
    class EXPORTED MeshEditComponent : public GameObjectComponent, public Ogre::Plugin, public OIS::MouseListener, public OIS::KeyListener
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

        struct MeshSnapshot
        {
            std::vector<Ogre::Vector3> vertices;
            std::vector<Ogre::Vector3> normals;
            std::vector<Ogre::Vector4> tangents;
            std::vector<Ogre::Vector2> uvCoordinates;
            std::vector<Ogre::uint32> indices;
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

        // ── Undo ──────────────────────────────────────────────────────────────
        void undo(void);
        bool canUndo(void) const;
        void pushUndoSnapshot(void);

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
            return "Blender-style offline mesh editor. "
                   "Requires Mesh Modify Mode and a selected GameObject. "
                   "Modes: Object / Vertex / Edge / Face. "
                   "G = Grab (move along camera plane). "
                   "During Grab: X/Y/Z = lock to world axis (press same axis twice to return to free movement). "
                   "Enter or LMB = Confirm. Esc or RMB = Cancel (restores original positions). "
                   "X = Delete selection. "
                   "A = Toggle Select All / Deselect All. "
                   "Ctrl+Z = Undo (up to 16 levels, snapshot before destructive ops). "
                   "Press 'Apply Mesh' to save edited mesh as a new .mesh asset.";
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

    protected:
        // OIS::MouseListener
        virtual bool mouseMoved(const OIS::MouseEvent& evt) override;
        virtual bool mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;
        virtual bool mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;
        // OIS::KeyListener
        virtual bool keyPressed(const OIS::KeyEvent& evt) override;
        virtual bool keyReleased(const OIS::KeyEvent& evt) override;

    private:
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

        VertexElementInfo vertexFormat;
        bool isIndices32;
        size_t vertexCount;
        size_t indexCount;

        // Selection
        std::set<size_t> selectedVertices;
        std::set<EdgeKey> selectedEdges;
        std::set<size_t> selectedFaces;

        // Undo
        static constexpr size_t MAX_UNDO_LEVELS = 16;
        std::vector<MeshSnapshot> undoStack;

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

        // Brush
        std::vector<Ogre::Real> brushData;
        size_t brushImageWidth;
        size_t brushImageHeight;
        bool isBrushArmed;

        // Overlay (render thread owned, updated via tracked closure)
        Ogre::SceneNode* overlayNode;
        Ogre::ManualObject* overlayObject;
        std::atomic<bool> overlayDirty;

        // Mouse state
        bool isPressing;
        bool isShiftPressed;
        bool isCtrlPressed;

        // Activation state (mirrors ProceduralRoadComponent)
        bool isEditorMeshModifyMode; ///< set by handleMeshModifyMode
        bool isSelected;             ///< set by handleGameObjectSelected
        bool isSimulating;           ///< true between connect/disconnect

        // Attributes
        Variant* activated;
        Variant* editMode;
        Variant* showWireframe;
        Variant* xRayOverlay;
        Variant* vertexMarkerSize;
        Variant* outputFileName;
        Variant* applyMeshButton;
        Variant* brushName;
        Variant* brushSize;
        Variant* brushIntensity;
        Variant* brushFalloff;
        Variant* brushMode;
        Variant* cancelEditButton;
    };

}; // namespace NOWA

#endif // MESHEDITCOMPONENT_H
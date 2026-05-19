/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#ifndef PROCEDURAL_STAIRS_COMPONENT_H
#define PROCEDURAL_STAIRS_COMPONENT_H

#include "OgrePlugin.h"
#include "gameobject/GeometricComponentBase.h"

namespace NOWA
{
    class PhysicsArtifactComponent;

    /**
     * @class ProceduralStairsComponent
     * @brief Procedural stair generator supporting Linear and Curved/Spiral flights.
     *
     * Geometry parameters:
     *   Step Count       – number of steps in the flight
     *   Step Height      – vertical rise per step (world units)
     *   Step Depth       – horizontal run per step / tread depth (world units)
     *   Step Width       – lateral extent of the stair (world units)
     *   Step Nosing      – tread overhang beyond the riser below (0 = flush)
     *   Open Riser       – omit the vertical riser face (floating stair look)
     *   Stringer Style   – None / Closed / Open  (side support panels)
     *   Bottom Style     – None / Sloped / Stepped  (underside geometry)
     *
     * Curved / Spiral additional parameters (active when Shape != Linear):
     *   Inner Radius     – radius of the central void / pole
     *   Outer Radius     – replaces Step Width for curved shapes
     *   Arc Angle        – total rotation in degrees (e.g. 180 = half-turn, 360 = full spiral)
     *   Rotation Dir     – Clockwise / Counter-Clockwise
     *   Centre Pole      – generate a solid cylinder at the inner radius
     *
     * Materials (separate PBS datablock per surface region):
     *   Tread Datablock     – top horizontal face of each step
     *   Riser Datablock     – front vertical face of each step
     *   Stringer Datablock  – side panel (ignored when Stringer = None)
     *
     * UV:
     *   UV Mode    – Box (per-face axis-aligned) / Continuous (flows along flight)
     *   UV Tiling  – X / Y tiling multiplier
     *
     * Pivot:
     *   Pivot Position  – Bottom-Front / Bottom-Centre / Bottom-Back
     *
     * Mesh export:
     *   Convert To Mesh – bakes to .mesh + replaces component with MeshComponent (one-way)
     *
     * Physics integration:
     *   getConvexParts() returns one Box per step (Linear) or one ConvexHull per step
     *   (Curved), letting PhysicsActiveCompoundComponent build exact collision without
     *   any mesh extraction step.
     */
    class EXPORTED ProceduralStairsComponent : public GeometricComponentBase, public Ogre::Plugin
    {
    public:
        typedef boost::shared_ptr<ProceduralStairsComponent> ProceduralStairsComponentPtr;

        // ── Shape enum ────────────────────────────────────────────────────────
        enum class StairShape
        {
            LINEAR = 0,
            CURVED = 1
        };

        // ── Stringer style ────────────────────────────────────────────────────
        enum class StringerStyle
        {
            NONE = 0,
            CLOSED = 1,
            OPEN = 2
        };

        // ── Underside / soffit style ──────────────────────────────────────────
        enum class BottomStyle
        {
            NONE = 0,
            SLOPED = 1,
            STEPPED = 2
        };

        // ── Pivot anchor ──────────────────────────────────────────────────────
        enum class PivotPosition
        {
            BOTTOM_FRONT = 0,
            BOTTOM_CENTRE = 1,
            BOTTOM_BACK = 2
        };

        // ── UV projection mode ────────────────────────────────────────────────
        enum class UVMode
        {
            BOX = 0,
            CONTINUOUS = 1
        };

    public:
        ProceduralStairsComponent();
        virtual ~ProceduralStairsComponent();

        // ── Ogre::Plugin ──────────────────────────────────────────────────────
        virtual void install(const Ogre::NameValuePairList* options) override;
        virtual void initialise() override;
        virtual void shutdown() override;
        virtual void uninstall() override;
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
        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;
        virtual void update(Ogre::Real dt, bool notSimulating = false) override;
        virtual void actualizeValue(Variant* attribute) override;
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        // ── Static registration ───────────────────────────────────────────────
        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("ProceduralStairsComponent");
        }
        static Ogre::String getStaticClassName(void)
        {
            return "ProceduralStairsComponent";
        }
        static bool canStaticAddComponent(GameObject* gameObject);

        static Ogre::String getStaticInfoText(void)
        {
            return "Usage: Creates procedural stair geometry directly in the viewport.\n\n"
                   "SHAPES:\n"
                   "  Linear  – straight flight.  Step Width sets the lateral size.\n"
                   "  Curved  – steps radiate from a centre point over a configurable arc.\n"
                   "  Spiral  – like Curved but Arc Angle can exceed 360 degrees.\n\n"
                   "STEP PARAMETERS:\n"
                   "  Step Count   – number of steps (min 1).\n"
                   "  Step Height  – vertical rise per step in world units.\n"
                   "  Step Depth   – horizontal run (tread depth) per step.\n"
                   "  Step Width   – lateral width (Linear) or use Inner/Outer Radius (Curved).\n"
                   "  Step Nosing  – tread overhang beyond the riser (0 = flush).\n"
                   "  Open Riser   – omit the front vertical face for a floating look.\n\n"
                   "STRUCTURAL:\n"
                   "  Stringer Style – None: no sides.  Closed: solid diagonal panel.\n"
                   "                   Open: notched panel cut under each tread.\n"
                   "  Bottom Style   – None: hollow underside.  Sloped: one diagonal soffit.\n"
                   "                   Stepped: mirrors the step geometry underneath.\n\n"
                   "CURVED / SPIRAL:\n"
                   "  Inner Radius  – radius of the central pole / void.\n"
                   "  Outer Radius  – radius of the outer step edge.\n"
                   "  Arc Angle     – total rotation in degrees (180=half-turn, 360=full spiral).\n"
                   "  Rotation Dir  – Clockwise or Counter-Clockwise.\n"
                   "  Centre Pole   – generate a solid cylinder at the inner radius.\n\n"
                   "MATERIALS:\n"
                   "  Tread / Riser / Stringer datablocks are separate PBS material slots.\n\n"
                   "UV:\n"
                   "  Box        – each face projected from its dominant axis.\n"
                   "  Continuous – UVs flow along the stair flight direction.\n\n"
                   "PIVOT:\n"
                   "  Bottom-Front  – origin at the base of the first step.\n"
                   "  Bottom-Centre – origin at the horizontal midpoint of the flight.\n"
                   "  Bottom-Back   – origin at the base of the top step.\n\n"
                   "PHYSICS:\n"
                   "  When combined with PhysicsActiveCompoundComponent, each step produces\n"
                   "  an exact Box (Linear) or ConvexHull (Curved) collision primitive.\n\n"
                   "CONVERT TO MESH:\n"
                   "  Bakes to a static .mesh file and replaces this component with a plain\n"
                   "  MeshComponent.  This is a one-way irreversible operation.\n\n"
                   "LUA API:\n"
                   "  getProceduralStairsComponent() on a GameObject returns this component.\n"
                   "  setStairShape(name) – 'Linear', 'Curved', 'Spiral'.\n"
                   "  setStepCount(n) / setStepHeight(h) / setStepDepth(d) / setStepWidth(w).\n"
                   "  setStepNosing(n) / setOpenRiser(bool) / setStringerStyle(name).\n"
                   "  setBottomStyle(name) / setPivotPosition(name).\n"
                   "  setInnerRadius(r) / setOuterRadius(r) / setArcAngle(deg).\n"
                   "  setTreadDatablock(name) / setRiserDatablock(name) / setStringerDatablock(name).\n"
                   "  setUVMode(name) / setUVTiling(Vector2) / rebuildMesh().";
        }

        static std::optional<NOWA::GameObjectTypeDescriptor> getStaticTypeDescriptor()
        {
            NOWA::GameObjectTypeDescriptor desc;
            desc.type = eType::CUSTOM;
            desc.displayName = "Stairs";
            desc.meshToDisplay = "Node.mesh";
            desc.needsMeshItem = false;
            desc.enterMeshModifyMode = false;
            desc.autoComponents = {"ProceduralStairsComponent"};
            desc.guardWithPluginCheck = true;
            return desc;
        }

        virtual Ogre::String getClassName(void) const override
        {
            return "ProceduralStairsComponent";
        }
        virtual Ogre::String getParentClassName(void) const override
        {
            return "GeometricComponentBase";
        }
        virtual Ogre::String getParentParentClassName(void) const override
        {
            return "GameObjectComponent";
        }

        // ── Lua API registration ──────────────────────────────────────────────
        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

        // ── GeometricComponentBase interface ──────────────────────────────────
        virtual std::vector<GeometricComponentBase::ConvexPart> getConvexParts(void) const override;
        virtual bool hasConvexParts(void) const override
        {
            return true;
        }

        // ── Mesh rebuild ──────────────────────────────────────────────────────
        void rebuildMesh(void);

        // ── Attribute setters / getters ───────────────────────────────────────
        virtual void setActivated(bool activated) override;
        bool isActivated(void) const;

        void setStairShape(const Ogre::String& shape);
        Ogre::String getStairShape(void) const;

        void setStepCount(int count);
        int getStepCount(void) const;

        void setStepHeight(Ogre::Real height);
        Ogre::Real getStepHeight(void) const;

        void setStepDepth(Ogre::Real depth);
        Ogre::Real getStepDepth(void) const;

        void setStepWidth(Ogre::Real width);
        Ogre::Real getStepWidth(void) const;

        void setStepNosing(Ogre::Real nosing);
        Ogre::Real getStepNosing(void) const;

        void setOpenRiser(bool open);
        bool getOpenRiser(void) const;

        void setStringerStyle(const Ogre::String& style);
        Ogre::String getStringerStyle(void) const;

        void setBottomStyle(const Ogre::String& style);
        Ogre::String getBottomStyle(void) const;

        void setInnerRadius(Ogre::Real radius);
        Ogre::Real getInnerRadius(void) const;

        void setOuterRadius(Ogre::Real radius);
        Ogre::Real getOuterRadius(void) const;

        void setArcAngle(Ogre::Real degrees);
        Ogre::Real getArcAngle(void) const;

        void setRotationDir(const Ogre::String& dir);
        Ogre::String getRotationDir(void) const;

        void setCentrePole(bool enable);
        bool getCentrePole(void) const;

        void setPivotPosition(const Ogre::String& pivot);
        Ogre::String getPivotPosition(void) const;

        void setRampCollider(bool enabled);
        bool getRampCollider(void) const;

        void setUVMode(const Ogre::String& mode);
        Ogre::String getUVMode(void) const;

        void setUVTiling(const Ogre::Vector2& tiling);
        Ogre::Vector2 getUVTiling(void) const;

        void setTreadDatablock(const Ogre::String& name);
        Ogre::String getTreadDatablock(void) const;

        void setRiserDatablock(const Ogre::String& name);
        Ogre::String getRiserDatablock(void) const;

        void setStringerDatablock(const Ogre::String& name);
        Ogre::String getStringerDatablock(void) const;

    public:
        // ── Attribute name strings ────────────────────────────────────────────
        static Ogre::String AttrActivated(void)
        {
            return "Activated";
        }
        static Ogre::String AttrStairShape(void)
        {
            return "Stair Shape";
        }
        static Ogre::String AttrStepCount(void)
        {
            return "Step Count";
        }
        static Ogre::String AttrStepHeight(void)
        {
            return "Step Height";
        }
        static Ogre::String AttrStepDepth(void)
        {
            return "Step Depth";
        }
        static Ogre::String AttrStepWidth(void)
        {
            return "Step Width";
        }
        static Ogre::String AttrStepNosing(void)
        {
            return "Step Nosing";
        }
        static Ogre::String AttrOpenRiser(void)
        {
            return "Open Riser";
        }
        static Ogre::String AttrStringerStyle(void)
        {
            return "Stringer Style";
        }
        static Ogre::String AttrBottomStyle(void)
        {
            return "Bottom Style";
        }
        static Ogre::String AttrInnerRadius(void)
        {
            return "Inner Radius";
        }
        static Ogre::String AttrOuterRadius(void)
        {
            return "Outer Radius";
        }
        static Ogre::String AttrArcAngle(void)
        {
            return "Arc Angle";
        }
        static Ogre::String AttrRotationDir(void)
        {
            return "Rotation Dir";
        }
        static Ogre::String AttrCentrePole(void)
        {
            return "Centre Pole";
        }
        static Ogre::String AttrPivotPosition(void)
        {
            return "Pivot Position";
        }
        static Ogre::String AttrRampCollider(void)
        {
            return "Ramp Collider";
        }
        static Ogre::String AttrUVMode(void)
        {
            return "UV Mode";
        }
        static Ogre::String AttrUVTiling(void)
        {
            return "UV Tiling";
        }
        static Ogre::String AttrTreadDatablock(void)
        {
            return "Tread Datablock";
        }
        static Ogre::String AttrRiserDatablock(void)
        {
            return "Riser Datablock";
        }
        static Ogre::String AttrStringerDatablock(void)
        {
            return "Stringer Datablock";
        }
        static Ogre::String AttrConvertToMesh(void)
        {
            return "Convert To Mesh";
        }

    protected:
        virtual bool executeAction(const Ogre::String& actionId, NOWA::Variant* attribute) override;

    private:
        // ── Mesh pipeline ─────────────────────────────────────────────────────
        void buildGeometry(void);
        void createStairsMesh(void);
        void createStairsMeshInternal(const std::vector<float>& treadVerts, const std::vector<Ogre::uint32>& treadIdx, size_t numTreadVerts, const std::vector<float>& riserVerts, const std::vector<Ogre::uint32>& riserIdx, size_t numRiserVerts,
            const std::vector<float>& stringerVerts, const std::vector<Ogre::uint32>& stringerIdx, size_t numStringerVerts, const std::vector<float>& rampVerts, const std::vector<Ogre::uint32>& rampIdx, size_t numRampVerts);
        void destroyStairsMesh(void);

        // ── Preview mesh ──────────────────────────────────────────────────────
        void createPreviewMesh(void);
        void destroyPreviewMesh(void);

        // ── Per-shape generators ──────────────────────────────────────────────
        void generateLinearStairs(void);
        void generateCurvedStairs(void);

        // ── Per-feature generators ────────────────────────────────────────────
        void generateLinearStep(int stepIndex, Ogre::Real pivotOffsetX, Ogre::Real pivotOffsetZ);
        void generateClosedStringer(Ogre::Real pivotOffsetX, Ogre::Real pivotOffsetZ);
        void generateOpenStringer(Ogre::Real pivotOffsetX, Ogre::Real pivotOffsetZ);
        void generateSlopedBottom(Ogre::Real pivotOffsetX, Ogre::Real pivotOffsetZ);
        void generateSteppedBottom(Ogre::Real pivotOffsetX, Ogre::Real pivotOffsetZ);
        void generateCentrePole(void);

        // ── Low-level vertex / quad helpers ───────────────────────────────────
        // Tread buffer (submesh 0)
        void tv(Ogre::Real px, Ogre::Real py, Ogre::Real pz, Ogre::Real nx, Ogre::Real ny, Ogre::Real nz, Ogre::Real u, Ogre::Real v);
        void tt(Ogre::uint32 i0, Ogre::uint32 i1, Ogre::uint32 i2);
        void tq(Ogre::uint32 i0, Ogre::uint32 i1, Ogre::uint32 i2, Ogre::uint32 i3);

        // Riser buffer (submesh 1)
        void rv(Ogre::Real px, Ogre::Real py, Ogre::Real pz, Ogre::Real nx, Ogre::Real ny, Ogre::Real nz, Ogre::Real u, Ogre::Real v);
        void rt(Ogre::uint32 i0, Ogre::uint32 i1, Ogre::uint32 i2);
        void rq(Ogre::uint32 i0, Ogre::uint32 i1, Ogre::uint32 i2, Ogre::uint32 i3);

        // Stringer buffer (submesh 2)
        void sv(Ogre::Real px, Ogre::Real py, Ogre::Real pz, Ogre::Real nx, Ogre::Real ny, Ogre::Real nz, Ogre::Real u, Ogre::Real v);
        void st(Ogre::uint32 i0, Ogre::uint32 i1, Ogre::uint32 i2);
        void sq(Ogre::uint32 i0, Ogre::uint32 i1, Ogre::uint32 i2, Ogre::uint32 i3);

        void pv(Ogre::Real px, Ogre::Real py, Ogre::Real pz, Ogre::Real nx, Ogre::Real ny, Ogre::Real nz, Ogre::Real u, Ogre::Real v);
        void pt(Ogre::uint32 i0, Ogre::uint32 i1, Ogre::uint32 i2);
        void pq(Ogre::uint32 i0, Ogre::uint32 i1, Ogre::uint32 i2, Ogre::uint32 i3);
        void generateLinearRamp(Ogre::Real pivotOffsetX, Ogre::Real pivotOffsetZ);
        void generateCurvedRamp(void);

        // ── Enum helpers ──────────────────────────────────────────────────────
        StairShape getStairShapeEnum(void) const;
        StringerStyle getStringerStyleEnum(void) const;
        BottomStyle getBottomStyleEnum(void) const;
        PivotPosition getPivotPositionEnum(void) const;
        UVMode getUVModeEnum(void) const;

        // ── Pivot offset ──────────────────────────────────────────────────────
        /**
         * @brief Returns the X and Z offset to subtract from all vertices
         *        so that the scene node origin sits at the chosen pivot position.
         */
        void computePivotOffset(Ogre::Real& outOffsetX, Ogre::Real& outOffsetZ) const;

        // ── GPU buffer upload helper ──────────────────────────────────────────
        /**
         * @brief Converts an 8-float/vertex (pos3+normal3+uv2) buffer into a
         *        12-float/vertex GPU buffer (pos3+normal3+tangent4+uv2),
         *        creates a VAO and wires it to subMesh.
         */
        void uploadSubMesh(Ogre::SubMesh* subMesh, const std::vector<float>& srcVerts, const std::vector<Ogre::uint32>& srcIdx, size_t numVerts, Ogre::VaoManager* vaoManager, Ogre::Vector3& inOutMin, Ogre::Vector3& inOutMax);

        // ── Mesh export ───────────────────────────────────────────────────────
        bool convertToMeshApply(void);
        bool exportMesh(const Ogre::String& filename);

        // ── Datablock application ─────────────────────────────────────────────
        void applyDatablocks(Ogre::Item* item);

    private:
        Ogre::String name;

        // ── Variants (editor attributes) ──────────────────────────────────────
        Variant* activated;
        Variant* stairShape;
        Variant* stepCount;
        Variant* stepHeight;
        Variant* stepDepth;
        Variant* stepWidth;
        Variant* stepNosing;
        Variant* openRiser;
        Variant* stringerStyle;
        Variant* bottomStyle;
        Variant* innerRadius;
        Variant* outerRadius;
        Variant* arcAngle;
        Variant* rotationDir;
        Variant* centrePole;
        Variant* pivotPosition;
        Variant* rampCollider;
        Variant* uvMode;
        Variant* uvTiling;
        Variant* treadDatablock;
        Variant* riserDatablock;
        Variant* stringerDatablock;
        Variant* convertToMesh;

        // ── CPU geometry buffers (3 submeshes) ────────────────────────────────
        // Each stores 8 floats/vertex: pos(3) + normal(3) + uv(2)
        std::vector<float> treadVertices; // submesh 0 – tread top faces
        std::vector<Ogre::uint32> treadIndices;
        Ogre::uint32 treadVertexBase;

        std::vector<float> riserVertices; // submesh 1 – riser front faces
        std::vector<Ogre::uint32> riserIndices;
        Ogre::uint32 riserVertexBase;

        std::vector<float> stringerVertices; // submesh 2 – stringer sides + soffit
        std::vector<Ogre::uint32> stringerIndices;
        Ogre::uint32 stringerVertexBase;

        // ── 4th CPU geometry buffer ───────────────────────────────────────────────────
        std::vector<float> rampVertices; // submesh 3 – invisible physics ramp
        std::vector<Ogre::uint32> rampIndices;
        Ogre::uint32 rampVertexBase;

        // ── Ogre-Next scene objects ───────────────────────────────────────────
        Ogre::MeshPtr stairsMesh;
        Ogre::Item* stairsItem;

        Ogre::MeshPtr previewMesh;
        Ogre::Item* previewItem;
        Ogre::SceneNode* previewNode;

        // ── Physics ───────────────────────────────────────────────────────────
        PhysicsArtifactComponent* physicsArtifactComponent;
    };

} // namespace NOWA

#endif // PROCEDURAL_STAIRS_COMPONENT_H

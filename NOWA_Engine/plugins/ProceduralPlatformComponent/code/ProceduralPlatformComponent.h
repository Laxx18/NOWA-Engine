/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#ifndef PROCEDURAL_PLATFORM_COMPONENT_H
#define PROCEDURAL_PLATFORM_COMPONENT_H

#include "OgrePlugin.h"
#include "gameobject/PlatformComponentBase.h"
#include "main/Events.h"

namespace NOWA
{
    class PhysicsArtifactComponent;

    /**
     * @class ProceduralPlatformComponent
     * @brief Interactive platform building component for 2.5D Metroidvania levels -
     *        click and drag to create platform chains on a fixed depth plane.
     *
     * Features:
     * - Click and drag to draw platforms in real-time, on a fixed local Z plane
     * - Support for straight and curved (Catmull-Rom) platform paths
     * - Dual material system (surface top + ground body), plus a separate junction datablock
     * - Interpolated height/gradient smoothing between connected segments
     * - Configurable platform depth (Z thickness) and height (downward slab thickness)
     * - Support for different platform styles (grass, wood, stone, ice, metal)
     * - Platform junctions where 3+ segments meet, so chains can branch/combine
     * - Export generated mesh to file
     * - Undo/Redo support for platform segments
     * - Segment mode to select, extend and remove individual segments
     * - Snap-to-existing-endpoint indicator while dragging, both within this component's own
     *   network (junction fan patches) and across to a SEPARATE ProceduralPlatformComponent
     *   GameObject, which gets merged into this one and removed - mirrors
     *   ProceduralRoadComponent's findOtherRoadNearby/mergeOtherRoadIntoThis exactly.
     *
     * Derives from PlatformComponentBase (not GameObjectComponent directly) so that other
     * code - and other ProceduralPlatformComponent instances performing a cross-network
     * merge - can reference a platform component through its data-level interface
     * (setPlatformData/getPlatformData/getNearestPointOnPlatform) without linking against
     * this plugin, exactly mirroring RoadComponentBase's role for roads.
     */
    class EXPORTED ProceduralPlatformComponent : public PlatformComponentBase, public Ogre::Plugin, public OIS::MouseListener, public OIS::KeyListener
    {
    public:
        typedef boost::shared_ptr<ProceduralPlatformComponent> ProceduralPlatformComponentPtr;

        enum class PlatformStyle
        {
            GRASS = 0, // Grass top, striped dirt/earth body (chunky block look)
            WOOD = 1,  // Bamboo/wood plank top and body
            STONE = 2, // Carved stone block
            ICE = 3,   // Slippery ice block
            METAL = 4  // Riveted metal platform
        };

        enum class BuildState
        {
            IDLE = 0,
            DRAGGING,
            CONFIRMING
        };

        enum class EditMode
        {
            OBJECT,
            SEGMENT
        };

        // Two structural buffers (surface = top walkable face, ground = body/side/bottom
        // faces), plus a third for junction fan patches - mirrors ProceduralRoadComponent's
        // CENTER/EDGE/JUNCTION split, renamed to match this component's own attribute names.
        enum class PlatformMeshBuffer
        {
            SURFACE,
            GROUND,
            JUNCTION
        };

        struct PlatformControlPoint
        {
            Ogre::Vector3 position;      // x = horizontal position along the platform plane, y = 0 (placeholder, real height kept separately below), z = 0 (placeholder, depth is a uniform extrusion applied at mesh-gen time, not stored per point)
            Ogre::Real rawHeight = 0.0f; // Vertical position exactly as placed by the user drag (no raycast involved - there is no terrain to sample)
            Ogre::Real smoothedHeight = 0.0f; // Vertical position after gradient smoothing between segments
            Ogre::Real distFromStart = 0.0f;  // Accumulated distance from chain start (for UV)
        };

        struct PlatformSegment
        {
            std::vector<PlatformControlPoint> controlPoints;
            bool isCurved;
            Ogre::Real curvature; // 0.0 = straight, 1.0 = maximum curve
            // Set by rebuildMesh when this chain's front/back end is trimmed against a
            // junction - generatePlatformBox skips the end-cap quad there, since
            // generateJunctionPatch already fills that exact spot; drawing both caused
            // overlapping/z-fighting geometry right at the junction. Default false so
            // direct callers (e.g. the live preview segment) keep both caps as before.
            bool skipFrontCap = false;
            bool skipBackCap = false;
        };

        struct JunctionPoint
        {
            Ogre::Vector3 worldPos;
            std::vector<size_t> segIndices;
            std::vector<Ogre::Vector3> armDirs;   // Per-arm outward direction, in the platform plane (x/height), z always 0
            std::vector<Ogre::Real> armTrimDists; // Per-arm, computed from adjacent angles - how far back each arm gets trimmed to make room for the fan patch
            // Per-arm trimmed boundary point (x, height), z always 0 - ONE point per arm,
            // unlike ProceduralRoadComponent's patchCorners/patchCornersInner PAIR per arm.
            // Road needs a pair because road width creates an inner/outer ring to bridge;
            // platform's path has no in-plane width (only Depth, which is orthogonal to this
            // whole plane and uniform everywhere), so there is only one ring here. The fan
            // fill is a solid wedge: 2D fan triangle (center + two adjacent ring points),
            // duplicated at the front/back depth faces, plus a perimeter quad between them -
            // see generateJunctionPatch.
            std::vector<Ogre::Vector3> patchCorners;
        };

    public:
        ProceduralPlatformComponent();
        virtual ~ProceduralPlatformComponent();

        /**
         * @see		Ogre::Plugin::install
         */
        virtual void install(const Ogre::NameValuePairList* options) override;

        /**
         * @see		Ogre::Plugin::initialise
         */
        virtual void initialise() override;

        /**
         * @see		Ogre::Plugin::shutdown
         */
        virtual void shutdown() override;

        /**
         * @see		Ogre::Plugin::uninstall
         */
        virtual void uninstall() override;

        /**
         * @see		Ogre::Plugin::getName
         */
        virtual const Ogre::String& getName() const override;

        /**
         * @see		Ogre::Plugin::getAbiCookie
         */
        virtual void getAbiCookie(Ogre::AbiCookie& outAbiCookie) override;

        /**
         * @see		GameObjectComponent::init
         */
        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

        /**
         * @see		GameObjectComponent::postInit
         */
        virtual bool postInit(void) override;

        /**
         * @see		GameObjectComponent::connect
         */
        virtual bool connect(void) override;

        /**
         * @see		GameObjectComponent::disconnect
         */
        virtual bool disconnect(void) override;

        /**
         * @see		GameObjectComponent::onCloned
         */
        virtual bool onCloned(void) override;

        /**
         * @see		GameObjectComponent::onAddComponent
         */
        virtual void onAddComponent(void) override;

        /**
         * @see		GameObjectComponent::onRemoveComponent
         */
        virtual void onRemoveComponent(void);

        /**
         * @see		GameObjectComponent::onOtherComponentRemoved
         */
        virtual void onOtherComponentRemoved(unsigned int index) override;

        /**
         * @see		GameObjectComponent::onOtherComponentAdded
         */
        virtual void onOtherComponentAdded(unsigned int index) override;

        /**
         * @see		GameObjectComponent::clone
         */
        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

        /**
         * @see		GameObjectComponent::update
         */
        virtual void update(Ogre::Real dt, bool notSimulating = false) override;

        /**
         * @see		GameObjectComponent::actualizeValue
         */
        virtual void actualizeValue(Variant* attribute) override;

        /**
         * @see		GameObjectComponent::writeXML
         */
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("ProceduralPlatformComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "ProceduralPlatformComponent";
        }

        static bool canStaticAddComponent(GameObject* gameObject);

        static Ogre::String getStaticInfoText(void)
        {
            return "Usage: Creates procedural 2.5D platform chains on a fixed depth plane, for Metroidvania-style levels.\n\n"
                   "PLATFORM BUILDING (Object Mode):\n"
                   "- Left-click anywhere to start a new platform segment. The first click of the whole\n"
                   "  chain fixes the local Z (depth) plane every further segment will be drawn on.\n"
                   "- Move the mouse to preview the segment, then left-click again to confirm it.\n"
                   "- Hold SHIFT while confirming to automatically chain the next segment from the endpoint.\n"
                   "- Hold CTRL to constrain the segment direction to horizontal or vertical.\n"
                   "- Right-click or press ESC to cancel the current segment.\n"
                   "- Press CTRL+Z to undo the last confirmed segment.\n\n"
                   "SEGMENT MODE:\n"
                   "- Set the 'Edit Mode' property to 'Segment' to enter segment editing.\n"
                   "- Left-click near any platform segment to select it. The selected segment is highlighted.\n"
                   "- Press X to delete the selected segment. The remaining platform rebuilds automatically.\n"
                   "- Press E to extend a new segment from the tail endpoint of the selected segment.\n"
                   "  Drag the mouse to preview the extension, then left-click to confirm.\n"
                   "  Press ESC to cancel the extension.\n"
                   "- Press ESC (without extending) to deselect the current segment.\n"
                   "- When building or extending near an existing segment endpoint, a green snap circle\n"
                   "  appears. Release at that point to snap exactly to the endpoint, closing the chain\n"
                   "  or connecting to an existing junction.\n\n"
                   "JUNCTIONS:\n"
                   "- When three or more segments share an endpoint, a junction patch is generated\n"
                   "  automatically to fill the gap between the converging platform arms - this is how\n"
                   "  segments combine into more complex, branching platform shapes.\n"
                   "- The junction patch uses its own datablock (Junction Datablock), separate from\n"
                   "  Surface/Ground, since a fan patch has no single consistent surface direction.\n\n"
                   "PLATFORM STYLES:\n"
                   "- Grass: grass surface top, striped dirt/earth body.\n"
                   "- Wood: bamboo/wood plank surface and body.\n"
                   "- Stone: carved stone block.\n"
                   "- Ice: slippery ice block.\n"
                   "- Metal: riveted metal platform.\n\n"
                   "DEPTH / HEIGHT:\n"
                   "- 'Platform Depth' is the Z-thickness of the block (how far it extends front-to-back\n"
                   "  on the fixed depth plane) - the same role roadWidth plays for roads, just on the\n"
                   "  third axis instead of sideways, since the path itself already IS the width.\n"
                   "- 'Platform Height' is how far the solid slab extends downward from the top surface.\n"
                   "- 'Smoothing Factor' and 'Max Gradient' control how height changes between connected\n"
                   "  segments are blended, so chained platforms don't have a jarring vertical snap.\n"
                   "- 'Curve Subdivisions' controls how many interpolated points are used per segment.\n\n"
                   "CONVERT TO MESH:\n"
                   "- 'Convert To Mesh' exports the current platform geometry as a static .mesh file\n"
                   "  and replaces this component with a standard mesh item for optimal performance.\n"
                   "- This operation is permanent and cannot be undone procedurally.\n\n"
                   "LUA API:\n"
                   "- getProceduralPlatformComponent() on a GameObject returns this component.\n"
                   "- addPlatformSegment(start, end) adds a segment between two local positions.\n"
                   "- getSegmentCount() returns the current number of segments.\n"
                   "- setPlatformDepth(d), setPlatformHeight(h) adjust geometry dimensions.\n"
                   "- setPlatformStyle(s) sets the style string: Grass, Wood, Stone, Ice, Metal.\n";
        }

        static std::optional<NOWA::GameObjectTypeDescriptor> getStaticTypeDescriptor()
        {
            NOWA::GameObjectTypeDescriptor desc;
            desc.type = eType::CUSTOM;
            desc.displayName = "Platform";
            desc.meshToDisplay = "Node.mesh";
            desc.needsMeshItem = false;
            desc.enterMeshModifyMode = true;
            desc.autoComponents = {"ProceduralPlatformComponent"};
            desc.guardWithPluginCheck = true;
            return desc;
        }

        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

        virtual Ogre::String getClassName(void) const override
        {
            return "ProceduralPlatformComponent";
        }

        virtual Ogre::String getParentClassName(void) const override
        {
            return "PlatformComponentBase";
        }

        virtual Ogre::String getParentParentClassName(void) const override
        {
            return "GameObjectComponent";
        }

        // Platform building API
        void startPlatformPlacement(const Ogre::Vector3& worldPosition);

        void updatePlatformPreview(const Ogre::Vector3& worldPosition);

        void confirmPlatform(void);

        void updateContinuationPoint(void);

        void cancelPlatform(void);

        void removeLastSegment(void);

        void clearAllSegments(void);

        // Mesh operations
        void rebuildMesh(void);

        /**
         * @brief Intersects the mouse ray with the fixed local depth plane. There is no
         *        terrain to raycast against - the plane itself IS the ground truth for
         *        cursor positioning (equivalent to ProceduralRoadComponent's raycastGround
         *        FALLBACK branch, promoted here to the only branch for THIS purpose).
         *        Cross-network lookup of an existing, separate platform GameObject still
         *        uses a scene query - see findOtherPlatformNearby() / otherPlatformQuery.
         */
        bool raycastFixedPlane(Ogre::Real screenX, Ogre::Real screenY, Ogre::Vector3& hitPosition);

        // Attribute access
        void setActivated(bool activated);

        bool isActivated(void) const;

        void setPlatformDepth(Ogre::Real depth);

        Ogre::Real getPlatformDepth(void) const;

        void setPlatformHeight(Ogre::Real height);

        Ogre::Real getPlatformHeight(void) const;

        void setPlatformStyle(const Ogre::String& style);

        Ogre::String getPlatformStyle(void) const;

        void setSnapToGrid(bool snap);

        bool getSnapToGrid(void) const;

        void setGridSize(Ogre::Real size);

        Ogre::Real getGridSize(void) const;

        void setSmoothingFactor(Ogre::Real factor);

        Ogre::Real getSmoothingFactor(void) const;

        void setMaxGradient(Ogre::Real gradient);

        Ogre::Real getMaxGradient(void) const;

        void setMaxTurnAngle(Ogre::Real angleDeg);

        Ogre::Real getMaxTurnAngle(void) const;

        void setCurveSubdivisions(int subdivisions);

        int getCurveSubdivisions(void) const;

        void setSurfaceDatablock(const Ogre::String& datablock);

        Ogre::String getSurfaceDatablock(void) const;

        void setGroundDatablock(const Ogre::String& datablock);

        Ogre::String getGroundDatablock(void) const;

        void setSurfaceUVTiling(const Ogre::Vector2& tiling);

        Ogre::Vector2 getSurfaceUVTiling(void) const;

        void setGroundUVTiling(const Ogre::Vector2& tiling);

        Ogre::Vector2 getGroundUVTiling(void) const;

        void setEditMode(const Ogre::String& editMode);

        EditMode getEditModeEnum(void) const;

        int findNearestSegmentWithinRadius(const Ogre::Vector3& worldPos, Ogre::Real radius) const;

        void deleteSelectedSegment(void);

        void createSegmentOverlay(void);

        void destroySegmentOverlay(void);

        void scheduleSegmentOverlayUpdate(void);

        virtual void setPlatformData(const std::vector<unsigned char>& data) override;

        virtual std::vector<unsigned char> getPlatformData(void) const override;

        virtual bool getNearestPointOnPlatform(const Ogre::Vector3& worldPos, Ogre::Real maxRadius, Ogre::Vector3& outPoint) const override;

        void addPlatformSegment(const Ogre::Vector3& start, const Ogre::Vector3& end);

        int getSegmentCount(void) const;

        Ogre::Vector3 getPlatformConnectionPoint(bool atStart) const;

        Ogre::Vector3 getPlatformApproachDirection(bool atStart) const;

        void addPlatformSegmentBatch(const Ogre::Vector3& start, const Ogre::Vector3& end);

        void finalizeBatch(void);

        void beginBatch(void);

        void endBatch(void);

    public:
        // Static attribute names
        static Ogre::String AttrActivated(void)
        {
            return "Activated";
        }
        static Ogre::String AttrPlatformDepth(void)
        {
            return "Platform Depth";
        }
        static Ogre::String AttrPlatformHeight(void)
        {
            return "Platform Height";
        }
        static Ogre::String AttrPlatformStyle(void)
        {
            return "Platform Style";
        }
        static Ogre::String AttrSnapToGrid(void)
        {
            return "Snap To Grid";
        }
        static Ogre::String AttrGridSize(void)
        {
            return "Grid Size";
        }
        static Ogre::String AttrSmoothingFactor(void)
        {
            return "Smoothing Factor";
        }
        static Ogre::String AttrMaxGradient(void)
        {
            return "Max Gradient";
        }
        static Ogre::String AttrMaxTurnAngle(void)
        {
            return "Max Turn Angle";
        }
        static Ogre::String AttrCurveSubdivisions(void)
        {
            return "Curve Subdivisions";
        }
        static Ogre::String AttrSurfaceDatablock(void)
        {
            return "Surface Datablock";
        }
        static Ogre::String AttrGroundDatablock(void)
        {
            return "Ground Datablock";
        }
        static Ogre::String AttrSurfaceUVTiling(void)
        {
            return "Surface UV Tiling";
        }
        static Ogre::String AttrGroundUVTiling(void)
        {
            return "Ground UV Tiling";
        }
        static Ogre::String AttrEditMode()
        {
            return "Edit Mode";
        }
        static const Ogre::String AttrConvertToMesh(void)
        {
            return "Convert To Mesh";
        }

    protected:
        // Mouse handling for interactive platform building
        virtual bool mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;

        virtual bool mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;

        virtual bool mouseMoved(const OIS::MouseEvent& evt) override;

        virtual bool keyPressed(const OIS::KeyEvent& evt) override;

        virtual bool keyReleased(const OIS::KeyEvent& evt) override;

        virtual bool executeAction(const Ogre::String& actionId, NOWA::Variant* attribute) override;

    private:
        void createPlatformMesh(void);

        void createPlatformMeshInternal(const std::vector<float>& surfaceVerts, const std::vector<Ogre::uint32>& surfaceInds, size_t numSurfaceVerts, const std::vector<float>& groundVerts, const std::vector<Ogre::uint32>& groundInds,
            size_t numGroundVerts, const std::vector<float>& junctionVerts, const std::vector<Ogre::uint32>& junctionInds, size_t numJunctionVerts, const Ogre::Vector3& origin);

        void destroyPlatformMesh(void);

        void destroyPreviewMesh(void);

        void updatePreviewMesh(void);

        // Geometry generation helpers
        void generatePlatformSegment(const PlatformSegment& segment);

        void generateStraightPlatform(const std::vector<PlatformControlPoint>& points, bool skipFrontCap = false, bool skipBackCap = false);

        void generateCurvedPlatform(const std::vector<PlatformControlPoint>& points, Ogre::Real curvature, bool skipFrontCap = false, bool skipBackCap = false);

        // Spline interpolation for curves
        // Returns (x, height) - unlike ProceduralRoadComponent's Vector3 (x,z world-plane
        // position), platform's curve lives entirely in these two axes; there is no third
        // component worth carrying since depth is a uniform extrusion applied later, not
        // part of the path itself.
        Ogre::Vector2 evaluateCatmullRom(const std::vector<PlatformControlPoint>& points, Ogre::Real t);

        Ogre::Real evaluateCatmullRomHeight(const std::vector<PlatformControlPoint>& points, Ogre::Real t);

        std::vector<Ogre::Vector2> generateCurvePoints(const std::vector<PlatformControlPoint>& controlPoints, int subdivisions);

        std::vector<PlatformControlPoint> subdivideWithHeightInterpolation(const std::vector<PlatformControlPoint>& points);

        std::vector<PlatformControlPoint> resamplePathUniformly(const std::vector<PlatformControlPoint>& densePath, Ogre::Real stepMeters);

        // Height smoothing for realistic gradients between connected segments
        void smoothHeightTransitions(std::vector<PlatformControlPoint>& points);

        Ogre::Real calculateSmoothedHeight(const std::vector<PlatformControlPoint>& points, int index);

        void addPlatformQuad(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& v3, const Ogre::Vector3& normal, Ogre::Real u0, Ogre::Real u1, Ogre::Real v0Val, Ogre::Real v1Val,
            PlatformMeshBuffer targetBuffer);

        void addJunctionTriangle(const Ogre::Vector3& v0, const Ogre::Vector2& uv0, const Ogre::Vector3& v1, const Ogre::Vector2& uv1, const Ogre::Vector3& v2, const Ogre::Vector2& uv2, const Ogre::Vector3& normalHint,
            PlatformMeshBuffer targetBuffer);

        Ogre::Vector3 snapToGridFunc(const Ogre::Vector3& position);

        PlatformStyle getPlatformStyleEnum(void) const;

        // Style-specific geometry generators
        /**
         * @brief Shared box-extrusion core all 5 style generators build on: top surface
         *        (SURFACE buffer) + bottom/front/back/end-caps (GROUND buffer). topBevel
         *        adds a 45-degree chamfer strip along the top front/back edges (used by
         *        Grass, for the rounded grass-over-dirt look); rimHeight adds a thin raised
         *        lip around the top perimeter instead (used by Metal). Both default to 0 for
         *        the plain sharp-edged box the remaining styles use - those styles rely on
         *        their assigned datablock texture for visual distinction rather than a
         *        structurally different mesh, same as most simple block-style platformers.
         */
        void generatePlatformBox(const std::vector<PlatformControlPoint>& points, Ogre::Real topBevel, Ogre::Real rimHeight, bool skipFrontCap = false, bool skipBackCap = false);

        void generateGrassPlatform(const std::vector<PlatformControlPoint>& points, bool skipFrontCap = false, bool skipBackCap = false);

        void generateWoodPlatform(const std::vector<PlatformControlPoint>& points, bool skipFrontCap = false, bool skipBackCap = false);

        void generateStonePlatform(const std::vector<PlatformControlPoint>& points, bool skipFrontCap = false, bool skipBackCap = false);

        void generateIcePlatform(const std::vector<PlatformControlPoint>& points, bool skipFrontCap = false, bool skipBackCap = false);

        void generateMetalPlatform(const std::vector<PlatformControlPoint>& points, bool skipFrontCap = false, bool skipBackCap = false);

        void generateJunctionPatch(const JunctionPoint& jp, const Ogre::Vector3& origin);

        int findNearestSegment(const Ogre::Vector3& worldPos) const;

        /**
         * @brief Cross-network lookup - is there a SEPARATE ProceduralPlatformComponent
         *        GameObject whose path passes near worldPos? Mirrors
         *        ProceduralRoadComponent::findOtherRoadNearby exactly: raycasts along the
         *        fixed plane's normal (this component's depth axis) via otherPlatformQuery
         *        (ALL_CATEGORIES_ID) to find a candidate GameObject, then confirms/refines
         *        via that GameObject's PlatformComponentBase::getNearestPointOnPlatform.
         */
        PlatformComponentBase* findOtherPlatformNearby(const Ogre::Vector3& worldPos, Ogre::Real maxRadius, Ogre::Vector3& outSnapPoint) const;

        /**
         * @brief Absorbs otherPlatform's segments into this component (as new segments,
         *        transformed through this component's own platformFrame/platformOrigin),
         *        then deletes otherPlatform's GameObject. Mirrors
         *        ProceduralRoadComponent::mergeOtherRoadIntoThis exactly.
         */
        void mergeOtherPlatformIntoThis(ProceduralPlatformComponent* otherPlatform);

        bool detectSnapToOwnPlatform(const Ogre::Vector3& worldPos, Ogre::Real radius);

        void splitSegmentAtPoint(int segIdx, float t, const Ogre::Vector3& splitWorldPos);

        void scheduleSnapIndicatorUpdate(void);

        // Save/Load functionality
        Ogre::String getPlatformDataFilePath(void) const;

        bool savePlatformDataToFile(void);

        bool loadPlatformDataFromFile(void);

        void deletePlatformDataFile(void);

        /**
         * @brief Exports the platform mesh and converts the GameObject to use the static mesh file.
         *        This is a one-way operation - the procedural data will be removed.
         * @return True if conversion succeeded
         */
        bool convertToMeshApply(void);

        /**
         * @brief Exports the current platform mesh to a file.
         * @param[in] filename Full path to export location
         * @return True if export succeeded
         */
        bool exportMesh(const Ogre::String& filename);

        void handleMeshModifyMode(NOWA::EventDataPtr eventData);

        void handleGameObjectSelected(NOWA::EventDataPtr eventData);

        void handleComponentManuallyDeleted(NOWA::EventDataPtr eventData);

        /**
         * @brief Deferred load after all postInits have run.
         */
        void handleSceneParsed(NOWA::EventDataPtr eventData);

        void addInputListener(void);

        void removeInputListener(void);

        void updateModificationState(void);

    private:
        static const uint32_t PLATFORMDATA_MAGIC = 0x504C4154; // "PLAT" in hex
        static const uint32_t PLATFORMDATA_VERSION = 1;

    private:
        Ogre::String name;

        // Attributes
        Variant* activated;
        Variant* platformDepth;
        Variant* platformHeight;
        Variant* platformStyle;
        Variant* snapToGrid;
        Variant* gridSize;
        Variant* smoothingFactor;
        Variant* maxGradient;
        Variant* maxTurnAngle;
        Variant* curveSubdivisions;
        Variant* surfaceDatablock;
        Variant* groundDatablock;
        Variant* surfaceUVTiling;
        Variant* groundUVTiling;
        Variant* editMode;
        Variant* convertToMesh;

        // Platform segments
        std::vector<PlatformSegment> platformSegments;
        PlatformSegment currentSegment;
        BuildState buildState;
        bool isEditorMeshModifyMode;
        bool isSelected;

        // Mesh data - separated for surface (top) and ground (body/sides/bottom)
        std::vector<float> surfaceVertices;
        std::vector<Ogre::uint32> surfaceIndices;
        Ogre::uint32 currentSurfaceVertexIndex;

        std::vector<float> groundVertices;
        std::vector<Ogre::uint32> groundIndices;
        Ogre::uint32 currentGroundVertexIndex;

        std::vector<float> junctionVertices;
        std::vector<Ogre::uint32> junctionIndices;
        Ogre::uint32 currentJunctionVertexIndex;

        // Ogre objects
        Ogre::MeshPtr platformMesh;
        Ogre::Item* platformItem;
        Ogre::MeshPtr previewMesh;
        Ogre::Item* previewItem;
        Ogre::SceneNode* previewNode;

        // Input state
        bool isShiftPressed;
        bool isCtrlPressed;
        Ogre::Vector3 lastValidPosition;

        // For continuous platform building
        bool continuousMode;
        Ogre::Vector3 platformOrigin;
        bool hasPlatformOrigin;

        // Set whenever a new drag continues from an existing endpoint (Shift-click chain
        // continuation, or Segment mode's 'E' extend) - the direction of the last edge of
        // whichever segment is being continued, in local (x, height) space, z always 0. Not
        // set for a genuinely fresh drag (startPlatformPlacement), since there is nothing to
        // stay consistent with there. updatePlatformPreview uses this to cap how sharply the
        // new drag can turn relative to it (maxTurnAngle), preventing the kind of tight
        // reversal that makes the fixed-depth extrusion self-intersect - see the class
        // design notes on that limitation.
        bool hasIncomingDirection;
        Ogre::Vector3 incomingDirection;

        // Mirrors ProceduralRoadComponent's roadFrame/roadOrigin pair exactly, so
        // local<->world conversion (getPlatformConnectionPoint, getNearestPointOnPlatform,
        // mergeOtherPlatformIntoThis, ...) is the same rotate-by-frame pattern throughout
        // the file. Unlike roadFrame (derived from a terrain-hit normal / planet surface),
        // platformFrame is simply read from the GameObject's own node orientation once in
        // postInit() - there is no bootstrapping wait for a first click, since "fixed
        // -Z-axis" means the depth layer is already decided by where the (empty) GameObject
        // was placed before this component was added.
        Ogre::Quaternion platformFrame;
        // World-space point the fixed depth plane passes through - the GameObject's own
        // world position at postInit time. Combined with platformFrame * Ogre::Vector3::UNIT_Z
        // (the plane's normal) this fully defines the plane raycastFixedPlane() intersects.
        Ogre::Vector3 platformPlaneAnchor;

        // Separate RaySceneQuery, ALL_CATEGORIES_ID, so it can see ANY other GameObject's
        // platform item regardless of category - used only by findOtherPlatformNearby().
        // Mirrors ProceduralRoadComponent's otherRoadQuery exactly.
        Ogre::RaySceneQuery* otherPlatformQuery;
        bool pendingCrossNetworkSnap;
        ProceduralPlatformComponent* pendingMergeOtherPlatform;

        // Cached data for save/load
        std::vector<float> cachedSurfaceVertices;
        std::vector<Ogre::uint32> cachedSurfaceIndices;
        size_t cachedNumSurfaceVertices;

        std::vector<float> cachedGroundVertices;
        std::vector<Ogre::uint32> cachedGroundIndices;
        size_t cachedNumGroundVertices;

        std::vector<float> cachedJunctionVertices;
        std::vector<Ogre::uint32> cachedJunctionIndices;
        size_t cachedNumJunctionVertices;

        Ogre::Vector3 cachedPlatformOrigin;
        bool originPositionSet;

        bool hasLoadedPlatformEndpoint;
        Ogre::Vector3 loadedPlatformEndpoint;
        Ogre::Real loadedPlatformEndpointHeight;

        // Segment selection state
        int selectedSegmentIndex; // -1 = nothing selected
        bool isSegmentDragging;   // true while extending from selected endpoint

        Ogre::SceneNode* segOverlayNode;
        Ogre::ManualObject* segOverlayObject;
        bool isExtendingFromSegment;

        // Own-network endpoint snapping (see item: "snapping system/overlay like in roads")
        bool isSnapToOwnPlatform;
        Ogre::Vector3 snapToPlatformPoint;
        int snapToPlatformSegmentIdx;
        float snapToPlatformT;
        Ogre::Real snapRadius; // = platformDepth * 1.5f, set in postInit

        bool bBatchMode;
        bool platformLoadedFromScene;

        PhysicsArtifactComponent* physicsArtifactComponent;
    };

} // namespace NOWA

#endif // PROCEDURAL_PLATFORM_COMPONENT_H
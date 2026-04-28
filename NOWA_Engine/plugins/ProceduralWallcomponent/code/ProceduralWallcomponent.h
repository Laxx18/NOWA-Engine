/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#ifndef PROCEDURAL_WALL_COMPONENT_H
#define PROCEDURAL_WALL_COMPONENT_H

#include "OgrePlugin.h"
#include "gameobject/WallComponentBase.h"

namespace NOWA
{
    /**
     * @class ProceduralWallComponent
     * @brief Interactive wall building component - click and drag to create walls dynamically
     *
     * Features:
     * - Click and drag to draw walls in real-time
     * - Optional corner pillars at start/end points
     * - Terrain/ground adaptation via raycasting
     * - Configurable wall dimensions and materials
     * - Support for different wall styles (solid, fence, etc.)
     * - Export generated mesh to file
     * - Undo/Redo support for wall segments
     */
    class EXPORTED ProceduralWallComponent : public WallComponentBase, public Ogre::Plugin, public OIS::MouseListener, public OIS::KeyListener
    {
    public:
        typedef boost::shared_ptr<ProceduralWallComponent> ProceduralWallComponentPtr;

        enum class WallStyle
        {
            SOLID = 0,      // Solid rectangular wall
            FENCE = 1,      // Fence with posts and rails
            BATTLEMENT = 2, // Castle-style with crenellations
            ARCH = 3        // Wall with arch openings
        };

        enum class BuildState
        {
            IDLE = 0,
            DRAGGING,
            CONFIRMING
        };

        enum class EditMode
        {
            OBJECT = 0,
            SEGMENT = 1
        };

        struct WallSegment
        {
            Ogre::Vector3 startPoint;
            Ogre::Vector3 endPoint;
            Ogre::Real groundHeightStart;
            Ogre::Real groundHeightEnd;
            bool hasStartPillar;
            bool hasEndPillar;
        };

    public:
        ProceduralWallComponent();
        virtual ~ProceduralWallComponent();

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
            return NOWA::getIdFromName("ProceduralWallComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "ProceduralWallComponent";
        }

        static bool canStaticAddComponent(GameObject* gameObject);

        static Ogre::String getStaticInfoText(void)
        {
            return "Usage: Creates procedural walls with terrain-following geometry and multiple styles.\n\n"
                   "WALL BUILDING (Object Mode):\n"
                   "- Left-click on terrain to place the start point of a new wall segment.\n"
                   "- Move the mouse to preview the segment direction and length.\n"
                   "- Left-click again to confirm the segment and add it to the wall.\n"
                   "- Hold SHIFT while confirming to automatically chain the next segment\n"
                   "  from the endpoint of the one just placed.\n"
                   "- Hold CTRL to constrain the segment to the X or Z axis only.\n"
                   "- Right-click or press ESC to cancel the current segment in progress.\n"
                   "- Press CTRL+Z to remove the last confirmed segment.\n\n"
                   "SEGMENT MODE:\n"
                   "- Set the 'Edit Mode' property to 'Segment' to enter segment editing.\n"
                   "- Left-click near any wall segment to select it.\n"
                   "  The selected segment is highlighted in orange with endpoint crosses.\n"
                   "- Press X to delete the selected segment.\n"
                   "  The remaining wall geometry rebuilds automatically.\n"
                   "- Press E to extend a new segment from the tail endpoint of the selected segment.\n"
                   "  Drag the mouse to preview the extension, then left-click to confirm.\n"
                   "  Press ESC to cancel the extension without placing a segment.\n"
                   "- Press ESC (without extending) to deselect the current segment.\n"
                   "- When dragging a segment near an existing endpoint, a green snap circle appears.\n"
                   "  Clicking at that point snaps the endpoint exactly, closing a loop or\n"
                   "  connecting to an existing wall endpoint.\n\n"
                   "WALL STYLES:\n"
                   "- Solid: a continuous flat-faced wall with full height and thickness.\n"
                   "- Fence: open post-and-rail fence with configurable post spacing and rails.\n"
                   "- Battlement: solid wall topped with alternating merlons and gaps.\n"
                   "  Battlement width and height are configurable.\n"
                   "- Arch: wall panels with rounded arched openings between pillar posts.\n\n"
                   "TERRAIN FOLLOWING:\n"
                   "- When 'Adapt To Ground' is enabled, the base of each wall segment\n"
                   "  is sampled from the terrain height at its start and end endpoints.\n"
                   "- The wall top follows this ground slope so the wall always sits flush\n"
                   "  on the terrain surface regardless of incline.\n\n"
                   "PILLARS:\n"
                   "- When 'Create Pillars' is enabled, a square pillar is generated\n"
                   "  at the start and end of each segment.\n"
                   "- Adjacent connected segments share a single pillar at their join point.\n"
                   "- Pillar size controls the width and depth of each pillar.\n"
                   "- Pillars use a separate datablock from the wall surface, allowing\n"
                   "  different materials for wall face and pillar.\n\n"
                   "CONVERT TO MESH:\n"
                   "- 'Convert To Mesh' exports the current wall geometry as a static .mesh file\n"
                   "  and replaces this component with a standard mesh item for optimal performance.\n"
                   "- This operation is permanent and cannot be undone procedurally.\n\n"
                   "LUA API:\n"
                   "- getProceduralWallComponent() on a GameObject returns this component.\n"
                   "- addWallSegment(start, end) adds a segment between two world positions.\n"
                   "- removeLastSegment() removes the most recently added segment.\n"
                   "- getSegmentCount() returns the current number of wall segments.\n"
                   "- setWallHeight(h), setWallThickness(t) adjust geometry dimensions.\n"
                   "- setWallStyle(s) sets the style string: Solid, Fence, Battlement, Arch.\n"
                   "- setWallDatablock(name), setPillarDatablock(name) assign materials.\n"
                   "- setCreatePillars(bool) enables or disables pillar generation.\n"
                   "- setAdaptToGround(bool) enables or disables terrain height following.\n"
                   "- setFencePostSpacing(f) sets post spacing for Fence style.\n"
                   "- setBattlementWidth(f), setBattlementHeight(f) configure Battlement style.\n"
                   "- setUVTiling(Vector2) controls texture repeat on wall faces.";
        }

        virtual Ogre::String getClassName(void) const override
        {
            return "ProceduralWallComponent";
        }

        virtual Ogre::String getParentClassName(void) const override
        {
            return "WallComponentBase";
        }

        virtual Ogre::String getParentParentClassName(void) const override
        {
            return "GameObjectComponent";
        }

        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

        // Wall building API
        void startWallPlacement(const Ogre::Vector3& worldPosition);

        void updateWallPreview(const Ogre::Vector3& worldPosition);

        void confirmWall(void);

        void updateContinuationPoint(void);

        void cancelWall(void);

        // Wall segment management
        void addWallSegment(const Ogre::Vector3& start, const Ogre::Vector3& end);

        void removeLastSegment(void);

        void clearAllSegments(void);

        // Mesh operations
        void rebuildMesh(void);

        // Ground adaptation
        Ogre::Real getGroundHeight(const Ogre::Vector3& position);

        bool raycastGround(Ogre::Real screenX, Ogre::Real screenY, Ogre::Vector3& hitPosition);

        // Attribute access
        void setActivated(bool activated);

        bool isActivated(void) const;

        void setWallHeight(Ogre::Real height);

        Ogre::Real getWallHeight(void) const;

        void setWallThickness(Ogre::Real thickness);

        Ogre::Real getWallThickness(void) const;

        void setWallStyle(const Ogre::String& style);

        Ogre::String getWallStyle(void) const;

        void setSnapToGrid(bool snap);

        bool getSnapToGrid(void) const;

        void setGridSize(Ogre::Real size);

        Ogre::Real getGridSize(void) const;

        void setAdaptToGround(bool adapt);

        bool getAdaptToGround(void) const;

        void setCreatePillars(bool create);

        bool getCreatePillars(void) const;

        void setPillarSize(Ogre::Real size);

        Ogre::Real getPillarSize(void) const;

        void setWallDatablock(const Ogre::String& datablock);

        Ogre::String getWallDatablock(void) const;

        void setPillarDatablock(const Ogre::String& datablock);

        Ogre::String getPillarDatablock(void) const;

        void setUVTiling(const Ogre::Vector2& tiling);
        Ogre::Vector2 getUVTiling(void) const;

        void setFencePostSpacing(Ogre::Real spacing);

        void setBattlementWidth(Ogre::Real width);

        void setBattlementHeight(Ogre::Real height);

        int getSegmentCountLua(void) const;

    public:
        // Static attribute names
        static Ogre::String AttrActivated(void)
        {
            return "Activated";
        }
        static Ogre::String AttrWallHeight(void)
        {
            return "Wall Height";
        }
        static Ogre::String AttrWallThickness(void)
        {
            return "Wall Thickness";
        }
        static Ogre::String AttrWallStyle(void)
        {
            return "Wall Style";
        }
        static Ogre::String AttrSnapToGrid(void)
        {
            return "Snap To Grid";
        }
        static Ogre::String AttrGridSize(void)
        {
            return "Grid Size";
        }
        static Ogre::String AttrAdaptToGround(void)
        {
            return "Adapt To Ground";
        }
        static Ogre::String AttrCreatePillars(void)
        {
            return "Create Pillars";
        }
        static Ogre::String AttrPillarSize(void)
        {
            return "Pillar Size";
        }
        static Ogre::String AttrWallDatablock(void)
        {
            return "Wall Datablock";
        }
        static Ogre::String AttrPillarDatablock(void)
        {
            return "Pillar Datablock";
        }
        static Ogre::String AttrUVTiling(void)
        {
            return "UV Tiling";
        }
        static Ogre::String AttrFencePostSpacing(void)
        {
            return "Fence Post Spacing";
        }
        static Ogre::String AttrBattlementWidth(void)
        {
            return "Battlement Width";
        }
        static Ogre::String AttrBattlementHeight(void)
        {
            return "Battlement Height";
        }
        static Ogre::String AttrEditMode(void)
        {
            return "Edit Mode";
        }
        static Ogre::String ActionExtendFromSegment(void)
        {
            return "ExtendFromSegment";
        }
        static Ogre::String AttrConvertToMesh(void)
        {
            return "Convert To Mesh";
        }
    protected:
        // Mouse handling for interactive wall building
        virtual bool mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;

        virtual bool mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;

        virtual bool mouseMoved(const OIS::MouseEvent& evt) override;

        virtual bool keyPressed(const OIS::KeyEvent& evt) override;

        virtual bool keyReleased(const OIS::KeyEvent& evt) override;

        virtual bool executeAction(const Ogre::String& actionId, NOWA::Variant* attribute) override;
    private:
        void createWallMesh(void);
        void createWallMeshInternal(const std::vector<float>& wallVertices, const std::vector<Ogre::uint32>& wallIndices, size_t numWallVertices, const std::vector<float>& pillarVertices,
                                    const std::vector<Ogre::uint32>& pillarIndices, size_t numPillarVertices, const Ogre::Vector3& wallOrigin);
        void destroyWallMesh(void);
        void destroyPreviewMesh(void);
        void updatePreviewMesh(void);

        // Geometry generation helpers
        void generateSolidWall(const WallSegment& segment);
        void generateFenceWall(const WallSegment& segment);
        void generateBattlementWall(const WallSegment& segment);
        void generateArchWall(const WallSegment& segment);
        void generatePillar(const Ogre::Vector3& position, Ogre::Real groundHeightOffset);

        void generateSolidWallWithSubdivision(const WallSegment& segment, int subdivisions);

        void addBox(Ogre::Real minX, Ogre::Real minY, Ogre::Real minZ, Ogre::Real maxX, Ogre::Real maxY, Ogre::Real maxZ, Ogre::Real uTile, Ogre::Real vTile);
        void addQuad(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& v3, const Ogre::Vector3& normal, Ogre::Real uTile, Ogre::Real vTile);

        void addPillarQuad(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& v3, const Ogre::Vector3& normal, Ogre::Real uTile, Ogre::Real vTile);

        Ogre::Vector3 snapToGridFunc(const Ogre::Vector3& position);
        WallStyle getWallStyleEnum(void) const;

        Ogre::String getWallDataFilePath(void) const;

        bool saveWallDataToFile(void);

        bool loadWallDataFromFile(void);

        void deleteWallDataFile(void);

        void handleMeshModifyMode(NOWA::EventDataPtr eventData);

        void handleGameObjectSelected(NOWA::EventDataPtr eventData);

        void handleComponentManuallyDeleted(NOWA::EventDataPtr eventData);

        void addInputListener(void);

        void removeInputListener(void);

        void updateModificationState(void);

        std::vector<unsigned char> getWallData(void) const;

        void setWallData(const std::vector<unsigned char>& data);

        /**
         * @brief Bakes the procedural dungeon to a static .mesh file and replaces
         *        this component with a plain MeshComponent referencing that file.
         *        This is a one-way, irreversible operation.
         */
        bool convertToMeshApply(void);

        /**
         * @brief Exports the current dungeon mesh to @p filename on disk.
         */
        bool exportMesh(const Ogre::String& filename);

        EditMode getEditModeEnum(void) const;

        int findNearestSegmentWithinRadius(const Ogre::Vector3& worldPos, Ogre::Real radius) const;

        void deleteSelectedSegment(void);

        void createSegmentOverlay(void);

        void destroySegmentOverlay(void);

        void scheduleSegmentOverlayUpdate(void);

        bool detectSnapToWall(const Ogre::Vector3& worldPos, Ogre::Real radius);

        void scheduleSnapIndicatorUpdate(void);

    private:
        static const uint32_t WALLDATA_MAGIC = 0x57414C4C; // "WALL" in hex
        static const uint32_t WALLDATA_VERSION = 1;

    private:
        Ogre::String name;
        // Attributes
        Variant* activated;
        Variant* wallHeight;
        Variant* wallThickness;
        Variant* wallStyle;
        Variant* snapToGrid;
        Variant* gridSize;
        Variant* adaptToGround;
        Variant* createPillars;
        Variant* pillarSize;
        Variant* wallDatablock;
        Variant* pillarDatablock;
        Variant* uvTiling;
        Variant* fencePostSpacing;
        Variant* battlementWidth;
        Variant* battlementHeight;
        Variant* editMode;
        Variant* convertToMesh;

        // Wall segments
        std::vector<WallSegment> wallSegments;
        WallSegment currentSegment;
        BuildState buildState;
        bool isEditorMeshModifyMode;
        bool isSelected;

        // Mesh data
        std::vector<float> vertices;
        std::vector<Ogre::uint32> indices;
        Ogre::uint32 currentVertexIndex;

        std::vector<float> pillarVertices;
        std::vector<Ogre::uint32> pillarIndices;
        Ogre::uint32 currentPillarVertexIndex;

        // Ogre objects
        Ogre::MeshPtr wallMesh;
        Ogre::Item* wallItem;
        Ogre::MeshPtr previewMesh;
        Ogre::Item* previewItem;
        Ogre::SceneNode* previewNode;

        // Input state
        bool isShiftPressed;
        bool isCtrlPressed;
        Ogre::Vector3 lastValidPosition;

        // For continuous wall building (shift-click)
        bool continuousMode;
        Ogre::Vector3 wallOrigin;
        bool hasWallOrigin;
        Ogre::Plane groundPlane;
        Ogre::RaySceneQuery* groundQuery;

        std::vector<float> cachedWallVertices;
        std::vector<Ogre::uint32> cachedWallIndices;
        size_t cachedNumWallVertices;

        std::vector<float> cachedPillarVertices;
        std::vector<Ogre::uint32> cachedPillarIndices;
        size_t cachedNumPillarVertices;

        Ogre::Vector3 cachedWallOrigin;
        bool originPositionSet;

        bool hasLoadedWallEndpoint;
        Ogre::Vector3 loadedWallEndpoint;    // XZ position
        Ogre::Real loadedWallEndpointHeight; // World-space height

        int selectedSegmentIndex;
        bool isExtendingFromSegment;
        bool isSnapToWall;
        Ogre::Vector3 snapToWallPoint;
        int snapToWallSegmentIdx;
        float snapToWallT;
        Ogre::SceneNode* segOverlayNode;
        Ogre::ManualObject* segOverlayObject;
        Ogre::SceneNode* snapOverlayNode;
        Ogre::ManualObject* snapOverlayObject;

        PhysicsArtifactComponent* physicsArtifactComponent;
    };

} // namespace NOWA

#endif // PROCEDURAL_WALL_COMPONENT_H
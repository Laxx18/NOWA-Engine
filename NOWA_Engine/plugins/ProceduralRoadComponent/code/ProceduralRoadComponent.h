/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#ifndef PROCEDURAL_ROAD_COMPONENT_H
#define PROCEDURAL_ROAD_COMPONENT_H

#include "OgrePlugin.h"
#include "gameobject/RoadComponentBase.h"

namespace NOWA
{
    /**
     * @class ProceduralRoadComponent
     * @brief Interactive road building component - click and drag to create roads dynamically
     *
     * Features:
     * - Click and drag to draw roads in real-time
     * - Support for straight and curved road segments
     * - Dual material system (center road + edge strips)
     * - Realistic terrain adaptation with gradient smoothing
     * - Configurable road dimensions and banking on curves
     * - Support for different road styles (highway, trail, dirt, paved)
     * - Export generated mesh to file
     * - Undo/Redo support for road segments
     * - Smooth transitions between segments
     * - Optional banking on curves for realistic appearance
     */
    class EXPORTED ProceduralRoadComponent : public RoadComponentBase, public Ogre::Plugin, public OIS::MouseListener, public OIS::KeyListener
    {
    public:
        typedef boost::shared_ptr<ProceduralRoadComponent> ProceduralRoadComponentPtr;

        enum class RoadStyle
        {
            PAVED = 0,      // Standard paved road
            HIGHWAY = 1,    // Wide highway with multiple lanes
            TRAIL = 2,      // Narrow dirt trail
            DIRT = 3,       // Unpaved dirt road
            COBBLESTONE = 4 // Historic cobblestone road
        };

        enum class BuildState
        {
            IDLE = 0,
            DRAGGING,
            CONFIRMING
        };

        struct RoadControlPoint
        {
            Ogre::Vector3 position;
            Ogre::Real groundHeight;
            Ogre::Real smoothedHeight;       // Height after gradient smoothing
            Ogre::Real bankingAngle = 0.0f;  // Computed banking at this point (radians)
            Ogre::Real distFromStart = 0.0f; // Accumulated distance from chain start (for UV)
        };

        struct RoadSegment
        {
            std::vector<RoadControlPoint> controlPoints;
            bool isCurved;
            Ogre::Real curvature; // 0.0 = straight, 1.0 = maximum curve
        };

        // Miter join data per point (shared across all styles)
        struct PointData
        {
            Ogre::Vector3 direction; // Forward direction at this point
            Ogre::Vector3 miterPerp; // Lateral offset direction (unit length)
            Ogre::Real miterScale;   // Width multiplier for constant-width curves
        };

    public:
        ProceduralRoadComponent();
        virtual ~ProceduralRoadComponent();

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
            return NOWA::getIdFromName("ProceduralRoadComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "ProceduralRoadComponent";
        }

        static bool canStaticAddComponent(GameObject* gameObject);

        static Ogre::String getStaticInfoText(void)
        {
            return "Usage: Create different types of roads with realistic terrain following.";
        }

        virtual Ogre::String getClassName(void) const override
        {
            return "ProceduralRoadComponent";
        }

        virtual Ogre::String getParentClassName(void) const override
        {
            return "RoadComponentBase";
        }

        virtual Ogre::String getParentParentClassName(void) const override
        {
            return "GameObjectComponent";
        }

        // Road building API
        void startRoadPlacement(const Ogre::Vector3& worldPosition);

        void updateRoadPreview(const Ogre::Vector3& worldPosition);

        void confirmRoad(void);

        void updateContinuationPoint(void);

        void cancelRoad(void);

        // Road segment management
        void addRoadSegment(const std::vector<Ogre::Vector3>& controlPoints, bool curved = false);

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

        void setRoadWidth(Ogre::Real width);

        Ogre::Real getRoadWidth(void) const;

        void setEdgeWidth(Ogre::Real width);

        Ogre::Real getEdgeWidth(void) const;

        void setRoadStyle(const Ogre::String& style);

        Ogre::String getRoadStyle(void) const;

        void setSnapToGrid(bool snap);

        bool getSnapToGrid(void) const;

        void setGridSize(Ogre::Real size);

        Ogre::Real getGridSize(void) const;

        void setAdaptToGround(bool adapt);

        bool getAdaptToGround(void) const;

        void setHeightOffset(Ogre::Real offset);

        Ogre::Real getHeightOffset(void) const;

        void setMaxGradient(Ogre::Real gradient);

        Ogre::Real getMaxGradient(void) const;

        void setSmoothingFactor(Ogre::Real factor);

        Ogre::Real getSmoothingFactor(void) const;

        void setEnableBanking(bool enable);

        bool getEnableBanking(void) const;

        void setBankingAngle(Ogre::Real angle);

        Ogre::Real getBankingAngle(void) const;

        void setCurveSubdivisions(int subdivisions);

        int getCurveSubdivisions(void) const;

        void setCenterDatablock(const Ogre::String& datablock);

        Ogre::String getCenterDatablock(void) const;

        void setEdgeDatablock(const Ogre::String& datablock);

        Ogre::String getEdgeDatablock(void) const;

        void setCenterUVTiling(const Ogre::Vector2& tiling);
        Ogre::Vector2 getCenterUVTiling(void) const;

        void setEdgeUVTiling(const Ogre::Vector2& tiling);
        Ogre::Vector2 getEdgeUVTiling(void) const;

        void setCurbHeight(Ogre::Real height);
        Ogre::Real getCurbHeight(void) const;

        void setTerrainSampleInterval(Ogre::Real interval);
        Ogre::Real getTerrainSampleInterval(void) const;

        virtual void setRoadData(const std::vector<unsigned char>& data) override;

        virtual std::vector<unsigned char> getRoadData(void) const override;

    public:
        // Static attribute names
        static Ogre::String AttrActivated(void)
        {
            return "Activated";
        }
        static Ogre::String AttrRoadWidth(void)
        {
            return "Road Width";
        }
        static Ogre::String AttrEdgeWidth(void)
        {
            return "Edge Width";
        }
        static Ogre::String AttrRoadStyle(void)
        {
            return "Road Style";
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
        static Ogre::String AttrHeightOffset(void)
        {
            return "Height Offset";
        }
        static Ogre::String AttrMaxGradient(void)
        {
            return "Max Gradient";
        }
        static Ogre::String AttrSmoothingFactor(void)
        {
            return "Smoothing Factor";
        }
        static Ogre::String AttrEnableBanking(void)
        {
            return "Enable Banking";
        }
        static Ogre::String AttrBankingAngle(void)
        {
            return "Banking Angle";
        }
        static Ogre::String AttrCurveSubdivisions(void)
        {
            return "Curve Subdivisions";
        }
        static Ogre::String AttrCenterDatablock(void)
        {
            return "Center Datablock";
        }
        static Ogre::String AttrEdgeDatablock(void)
        {
            return "Edge Datablock";
        }
        static Ogre::String AttrCenterUVTiling(void)
        {
            return "Center UV Tiling";
        }
        static Ogre::String AttrEdgeUVTiling(void)
        {
            return "Edge UV Tiling";
        }
        static inline Ogre::String AttrCurbHeight(void)
        {
            return "CurbHeight";
        }
        static inline Ogre::String AttrTerrainSampleInterval(void)
        {
            return "TerrainSampleInterval";
        }
        static const Ogre::String AttrConvertToMesh(void)
        {
            return "Convert To Mesh";
        }
    protected:
        // Mouse handling for interactive road building
        virtual bool mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;

        virtual bool mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;

        virtual bool mouseMoved(const OIS::MouseEvent& evt) override;

        virtual bool keyPressed(const OIS::KeyEvent& evt) override;

        virtual bool keyReleased(const OIS::KeyEvent& evt) override;

        virtual bool executeAction(const Ogre::String& actionId, NOWA::Variant* attribute) override;
    private:
        void createRoadMesh(void);

        void createRoadMeshInternal(const std::vector<float>& centerVertices, const std::vector<Ogre::uint32>& centerIndices, size_t numCenterVertices, const std::vector<float>& edgeVertices,
                                    const std::vector<Ogre::uint32>& edgeIndices, size_t numEdgeVertices, const Ogre::Vector3& roadOrigin);

        void destroyRoadMesh(void);

        void destroyPreviewMesh(void);

        void updatePreviewMesh(void);

        // Geometry generation helpers
        void generateRoadSegment(const RoadSegment& segment);

        void generateStraightRoad(const std::vector<RoadControlPoint>& points);

        void generateCurvedRoad(const std::vector<RoadControlPoint>& points, Ogre::Real curvature);

        // Spline interpolation for curves
        Ogre::Vector3 evaluateCatmullRom(const std::vector<RoadControlPoint>& points, Ogre::Real t);
        std::vector<Ogre::Vector3> generateCurvePoints(const std::vector<RoadControlPoint>& controlPoints, int subdivisions);

        // Height smoothing for realistic gradients
        void smoothHeightTransitions(std::vector<RoadControlPoint>& points);
        Ogre::Real calculateSmoothedHeight(const std::vector<RoadControlPoint>& points, int index);

        void addRoadQuad(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& v3, const Ogre::Vector3& normal, Ogre::Real u0, Ogre::Real u1, Ogre::Real v0Val,
                         Ogre::Real v1Val, bool isCenter);

        // Banking calculation
        Ogre::Real calculateBanking(const Ogre::Vector3& p0, const Ogre::Vector3& p1, const Ogre::Vector3& p2);

        Ogre::Vector3 snapToGridFunc(const Ogre::Vector3& position);
        RoadStyle getRoadStyleEnum(void) const;

        std::vector<PointData> computeMiterData(const std::vector<RoadControlPoint>& points);

        Ogre::Real evaluateCatmullRomHeight(const std::vector<RoadControlPoint>& points, Ogre::Real t);

        std::vector<ProceduralRoadComponent::RoadControlPoint> subdivideWithHeightInterpolation(const std::vector<RoadControlPoint>& points);

        // Style-specific geometry generators
        void generatePavedRoad(const std::vector<RoadControlPoint>& points, const std::vector<PointData>& miterData);
        void generateHighwayRoad(const std::vector<RoadControlPoint>& points, const std::vector<PointData>& miterData);
        void generateTrailRoad(const std::vector<RoadControlPoint>& points, const std::vector<PointData>& miterData);
        void generateDirtRoad(const std::vector<RoadControlPoint>& points, const std::vector<PointData>& miterData);
        void generateCobblestoneRoad(const std::vector<RoadControlPoint>& points, const std::vector<PointData>& miterData);

        /**
         * @brief Subdivides a path by inserting intermediate terrain-sampled points
         *        at regular intervals. This ensures the road follows terrain undulations
         *        even for long straight segments.
         *
         * @param points  Input control points (in WORLD space, before local transform)
         * @return        Subdivided points with terrain heights sampled at each point
         */
        std::vector<RoadControlPoint> subdivideForTerrain(const std::vector<RoadControlPoint>& points);

        // Save/Load functionality
        Ogre::String getRoadDataFilePath(void) const;

        bool saveRoadDataToFile(void);

        bool loadRoadDataFromFile(void);

        void deleteRoadDataFile(void);

        void handleMeshModifyMode(NOWA::EventDataPtr eventData);

        void handleGameObjectSelected(NOWA::EventDataPtr eventData);

        void handleComponentManuallyDeleted(NOWA::EventDataPtr eventData);

        void addInputListener(void);

        void removeInputListener(void);

        void updateModificationState(void);

        /**
         * @brief Exports the road mesh and converts the GameObject to use the static mesh file.
         *        This is a one-way operation - the procedural data will be removed.
         * @return True if conversion succeeded
         */
        bool convertToMeshApply(void);

        /**
         * @brief Exports the current road mesh to a file.
         * @param[in] filename Full path to export location
         * @return True if export succeeded
         */
        bool exportMesh(const Ogre::String& filename);
    private:
        static const uint32_t ROADDATA_MAGIC = 0x524F4144; // "ROAD" in hex
        static const uint32_t ROADDATA_VERSION = 1;

    private:
        Ogre::String name;

        // Attributes
        Variant* activated;
        Variant* roadWidth;
        Variant* edgeWidth;
        Variant* roadStyle;
        Variant* snapToGrid;
        Variant* gridSize;
        Variant* adaptToGround;
        Variant* heightOffset;
        Variant* maxGradient;
        Variant* smoothingFactor;
        Variant* enableBanking;
        Variant* bankingAngle;
        Variant* curveSubdivisions;
        Variant* centerDatablock;
        Variant* edgeDatablock;
        Variant* centerUVTiling;
        Variant* edgeUVTiling;
        Variant* curbHeight;
        Variant* terrainSampleInterval;
        Variant* convertToMesh;

        // Road segments
        std::vector<RoadSegment> roadSegments;
        RoadSegment currentSegment;
        BuildState buildState;
        bool isEditorMeshModifyMode;
        bool isSelected;

        // Mesh data - separated for center and edges
        std::vector<float> centerVertices;
        std::vector<Ogre::uint32> centerIndices;
        Ogre::uint32 currentCenterVertexIndex;

        std::vector<float> edgeVertices;
        std::vector<Ogre::uint32> edgeIndices;
        Ogre::uint32 currentEdgeVertexIndex;

        // Ogre objects
        Ogre::MeshPtr roadMesh;
        Ogre::Item* roadItem;
        Ogre::MeshPtr previewMesh;
        Ogre::Item* previewItem;
        Ogre::SceneNode* previewNode;

        // Input state
        bool isShiftPressed;
        bool isCtrlPressed;
        Ogre::Vector3 lastValidPosition;

        // For continuous road building
        bool continuousMode;
        Ogre::Vector3 roadOrigin;
        bool hasRoadOrigin;
        Ogre::Plane groundPlane;
        Ogre::RaySceneQuery* groundQuery;

        // Cached data for save/load
        std::vector<float> cachedCenterVertices;
        std::vector<Ogre::uint32> cachedCenterIndices;
        size_t cachedNumCenterVertices;

        std::vector<float> cachedEdgeVertices;
        std::vector<Ogre::uint32> cachedEdgeIndices;
        size_t cachedNumEdgeVertices;

        Ogre::Vector3 cachedRoadOrigin;
        bool originPositionSet;
        
        bool hasLoadedRoadEndpoint;
        Ogre::Vector3 loadedRoadEndpoint;    // XZ position
        Ogre::Real loadedRoadEndpointHeight; // World-space height
    };

} // namespace NOWA

#endif // PROCEDURAL_ROAD_COMPONENT_H

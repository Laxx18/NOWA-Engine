/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#ifndef PROCEDURAL_CITY_COMPONENT_H
#define PROCEDURAL_CITY_COMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "utilities/XMLConverter.h"

#include "OgreIdString.h"
#include "OgreItem.h"
#include "OgreMesh2.h"
#include "OgreSceneNode.h"
#include "OgreVector2.h"
#include "OgreVector3.h"
#include "OgreVector4.h"
#include "OgreQuaternion.h"

#include "OgreAbiUtils.h"

#include <vector>
#include <array>
#include <cstdint>

namespace NOWA
{
    class PhysicsArtifactComponent;
    class ProceduralRoadComponent;

    /**
     * @brief Procedural city generator.
     *
     * Fully self-contained: generates road strips, courtyard walls, and multi-
     * storey buildings all from its own internal geometry code.  No external
     * ProceduralRoadComponent or ProceduralWallComponent needs to exist before
     * this component is used.
     *
     * --- External road connection (v1 feature) --------------------------------
     *
     * An optional ProceduralRoadComponent may be referenced by @p roadComponentId
     * to connect a hand-drawn road (e.g. a highway through the forest) to the
     * city grid.  When set, ONE connecting road segment is added to that external
     * road component via addRoadSegmentLua() at generation time, running from the
     * road's chosen endpoint (controlled by @p roadConnectionAtStart) to the
     * nearest city grid boundary intersection.  The external road component owns
     * that segment; the city component owns everything inside the grid.
     *
     * ProceduralRoadComponent needs two public getters added for this to compile:
     *
     *   Ogre::Vector3 getRoadConnectionPoint(bool atStart) const;
     *     // atStart=true  -> roadOrigin (first click, first control point)
     *     // atStart=false -> loadedRoadEndpoint (last control point, last segment)
     *
     *   Ogre::Vector3 getRoadApproachDirection(bool atStart) const;
     *     // Direction pointing AWAY from the city at the chosen endpoint so the
     *     // city can find the boundary entry nearest to the road.
     *
     * These getters expose already-existing private members (roadOrigin,
     * loadedRoadEndpoint, and the first/last segment control points) — no new
     * data, just access.
     *
     * --- Geometry ------------------------------------------------------------
     *
     * Internal road strips: 4 quads per road segment (left curb, left lane,
     * right lane, right curb).  Intersections: one flat square quad.
     * Courtyard walls: solid box strips matching ProceduralWallComponent's
     * generateSolidWall() geometry but produced inline, no external component.
     * Buildings: box extrusion for walls (4 side faces + parapet cap), roof
     * (flat / gabled / hip), window grid (separate glass submesh per face),
     * ground-level trim strip (doors / storefronts).
     *
     * --- Normal convention (CRITICAL — read before touching pushQuad) ---------
     *
     * ALL quads are passed to pushQuad() with vertices in CCW winding order
     * as seen from OUTSIDE (the side the normal points toward).  The normal is
     * ALWAYS passed as an EXPLICIT parameter — it is NEVER computed from a cross
     * product inside pushQuad() itself.  Every call site knows exactly which
     * direction the face points.
     *
     * Per-face normals for a building whose long axis runs along +Z (wallDir):
     *
     *   right = wallDir.crossProduct(Ogre::Vector3::UNIT_Y)  // always this order
     *
     *   front face   normal = -right        (faces street, -X side)
     *   back  face   normal = +right        (faces courtyard, +X side)
     *   left  face   normal = -wallDir      (faces -Z)
     *   right face   normal = +wallDir      (faces +Z)
     *   flat roof    normal = UNIT_Y
     *   gable slopes normal = computed once per slope as
     *                         Ogre::Vector3(0, roofPitch, ±1).normalisedCopy()
     *
     * The cross product direction wallDir.crossProduct(UNIT_Y) is chosen to
     * match ProceduralWallComponent::generateSolidWall() exactly — it uses
     * dir.crossProduct(UNIT_Y) for its perpendicular.  Never swap the operands.
     *
     * --- Caching (ProceduralFoliageVolumeComponent pattern) ------------------
     *
     * A 64-bit checksum of all generation parameters is embedded in the
     * .citydata header.  On scene load, if the stored checksum matches
     * computeChecksum(), the cache is used (fast: geometry regenerated from
     * saved BuildingInstance placement data, no terrain queries).  If it
     * mismatches (any parameter changed), the city is fully regenerated and a
     * fresh cache is written.
     *
     * Cache file NOT deleted on onRemoveComponent() — that fires on every clean
     * scene teardown, not only on explicit deletion.  The cache is deleted only
     * on the explicit "Clear" button and at the start of generateCity().
     */

    // =========================================================================
    // Data structures
    // =========================================================================

    /**
     * @brief One placed building — the unit stored in the .citydata cache.
     *        Geometry is regenerated from this on load (cheap math, no GPU
     *        round-trips), so the file stores placement only, not baked vertices.
     */
    struct BuildingInstance
    {
        Ogre::Vector3    position;      ///< World-space lot centre (XYZ)
        Ogre::Quaternion orientation;   ///< Yaw only (random per lot, seeded)
        Ogre::Vector3    footprint;     ///< X=width (local), Y=wallHeight, Z=depth (local)
        unsigned int     archetypeIdx;  ///< Which district produced this building
        unsigned int     variantSeed;   ///< Per-building RNG seed for datablock selection
        unsigned int     roofType;      ///< 0 = flat, 1 = gabled, 2 = hip
        float            roofPitch;     ///< Normalized pitch [0..1] (0=gentle, 1=steep)
        float            groundHeight;  ///< World Y of the lot's ground surface
    };

    /**
     * @brief One material-slot batch.  All BuildingInstances in this batch share
     *        the same material slot (wall=0, roof=1, window=2, trim=3).
     *        On the render thread, raw vertex data is split into 10m spatial
     *        cells and each cell becomes one BT_IMMUTABLE SCENE_STATIC Item —
     *        same cell-merge strategy as ProceduralFoliageVolumeComponent.
     */
    struct CityBatch
    {
        unsigned int                 materialSlot;   ///< 0=wall 1=roof 2=window 3=trim
        size_t                       districtIdx;
        std::vector<BuildingInstance> instances;

        // Accumulated on the logic thread (8 floats/vertex: pos.xyz+normal.xyz+uv.xy)
        std::vector<float>           rawVertices;
        std::vector<Ogre::uint32>    rawIndices;
        size_t                       numVertices;

        // Render-thread results: one Item+Node per 10m cell
        std::vector<Ogre::Item*>     items;
        std::vector<Ogre::SceneNode*> nodes;

        CityBatch() : materialSlot(0), districtIdx(0), numVertices(0) {}
    };

    // =========================================================================
    // Component
    // =========================================================================

    class EXPORTED ProceduralCityComponent : public GameObjectComponent, public Ogre::Plugin
    {
    public:

        typedef boost::shared_ptr<ProceduralCityComponent> ProceduralCityComponentPtr;

        // ---- Plugin lifecycle -----------------------------------------------

        void install(const Ogre::NameValuePairList* options) override;
        void initialise() override;
        void shutdown() override;
        void uninstall() override;
        void getAbiCookie(Ogre::AbiCookie& outAbiCookie) override;
        const Ogre::String& getName() const override;

        // ---- Component lifecycle ------------------------------------------

        ProceduralCityComponent();

        virtual ~ProceduralCityComponent();

        GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

        bool init(rapidxml::xml_node<>*& propertyElement) override;

        bool postInit(void) override;

        bool connect(void) override;

        bool disconnect(void) override;

        bool onCloned(void) override;

        void onAddComponent(void) override;

        void onRemoveComponent(void) override;

        /**
         * @see GameObjectComponent::getClassName
         */
        virtual Ogre::String getClassName(void) const override;

        /**
         * @see GameObjectComponent::getParentClassName
         */
        virtual Ogre::String getParentClassName(void) const override;

        void onOtherComponentRemoved(unsigned int index) override;

        void onOtherComponentAdded(unsigned int index) override;

        void update(Ogre::Real dt, bool notSimulating) override;

        void actualizeValue(Variant* attribute) override;

        void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        bool executeAction(const Ogre::String& actionId, NOWA::Variant* attribute) override;

        // ---- Static identifiers --------------------------------------------

        /**
         * @see GameObjectComponent::getStaticClassId
         */
        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("ProceduralCityComponent");
        }

        /**
         * @see GameObjectComponent::getStaticClassName
         */
        static Ogre::String getStaticClassName(void)
        {
            return "ProceduralCityComponent";
        }

        /**
         * @see GameObjectComponent::canStaticAddComponent
         */
        static bool canStaticAddComponent(GameObject* gameObject);

        /**
         * @see GameObjectComponent::getStaticInfoText
         */
        static Ogre::String getStaticInfoText(void)
        {
            return "Procedural city generator. Fully self-contained: generates road strips, "
                   "courtyard walls, and buildings from internal geometry code — no external "
                   "road or wall components required. Optionally connects to an existing "
                   "ProceduralRoadComponent (e.g. a highway) via RoadComponentId. "
                   "Press 'Generate Now' to build or rebuild the city at the current "
                   "GameObject position. Results are cached to .citydata; cache is "
                   "auto-invalidated when any parameter changes.";
        }

        static std::optional<NOWA::GameObjectTypeDescriptor> getStaticTypeDescriptor();

        // ---- Attribute name accessors --------------------------------------
    public:
        static Ogre::String AttrActivated(void)             { return "Activated"; }
        static Ogre::String AttrCityBounds(void)            { return "CityBounds"; }
        static Ogre::String AttrMasterSeed(void)            { return "MasterSeed"; }
        static Ogre::String AttrBlockSize(void)             { return "BlockSize"; }
        // Roads (internal geometry)
        static Ogre::String AttrRoadWidth(void)             { return "RoadWidth"; }
        static Ogre::String AttrSidewalkWidth(void)         { return "SidewalkWidth"; }
        static Ogre::String AttrCurbHeight(void)            { return "CurbHeight"; }
        static Ogre::String AttrGenerateRoads(void)         { return "GenerateRoads"; }
        static Ogre::String AttrRoadDatablock(void)         { return "RoadDatablock"; }
        static Ogre::String AttrCurbDatablock(void)         { return "CurbDatablock"; }
        // Courtyard walls (internal geometry)
        static Ogre::String AttrGenerateWalls(void)         { return "GenerateWalls"; }
        static Ogre::String AttrWallHeight(void)            { return "WallHeight"; }
        static Ogre::String AttrWallDatablock(void)         { return "WallDatablock"; }
        // Door (global, shared by all buildings)
        static Ogre::String AttrDoorDatablock(void)         { return "DoorDatablock"; }
        static Ogre::String AttrGarageDatablock(void)       { return "GarageDatablock"; }
        // External road connection (optional)
        static Ogre::String AttrRoadComponentId(void)       { return "RoadComponentId"; }
        static Ogre::String AttrRoadConnectionAtStart(void) { return "RoadConnectionAtStart"; }
        // Districts
        static Ogre::String AttrDistrictCount(void)         { return "DistrictCount"; }
        // Action buttons
        static Ogre::String AttrGenerate(void)              { return "Generate"; }
        static Ogre::String AttrClear(void)                 { return "Clear"; }

        // Per-district names (index appended at runtime, e.g. "DistrictName0")
        static Ogre::String AttrDistrictName(void)            { return "DistrictName"; }
        static Ogre::String AttrDistrictType(void)            { return "DistrictType"; }
        static Ogre::String AttrDistrictMinHeight(void)       { return "DistrictMinHeight"; }
        static Ogre::String AttrDistrictMaxHeight(void)       { return "DistrictMaxHeight"; }
        static Ogre::String AttrDistrictMinFootprint(void)    { return "DistrictMinFootprint"; }
        static Ogre::String AttrDistrictMaxFootprint(void)    { return "DistrictMaxFootprint"; }
        static Ogre::String AttrDistrictDensity(void)         { return "DistrictDensity"; }
        static Ogre::String AttrDistrictCourtyardProb(void)   { return "DistrictCourtyardProb"; }
        static Ogre::String AttrDistrictWallDatablock(void)   { return "DistrictWallDatablock"; }
        static Ogre::String AttrDistrictRoofDatablock(void)   { return "DistrictRoofDatablock"; }
        static Ogre::String AttrDistrictWindowDatablock(void) { return "DistrictWindowDatablock"; }
        static Ogre::String AttrDistrictTrimDatablock(void)   { return "DistrictTrimDatablock"; }

        // ---- Action IDs ---------------------------------------------------

        static Ogre::String ActionGenerate(void) { return "ProceduralCityComponent.Generate"; }
        static Ogre::String ActionClear(void)    { return "ProceduralCityComponent.Clear"; }

    public:
        // ---- Global parameters --------------------------------------------

        /**
         * @brief World-space XZ footprint (minX, minZ, maxX, maxZ).
         *        Same Vector4 convention as ProceduralFoliageVolumeComponent::
         *        volumeBounds.  The city's centre is implicitly
         *        ((minX+maxX)*0.5, 0, (minZ+maxZ)*0.5); the gameObject node
         *        is placed there by the designer.
         */
        void          setCityBounds(const Ogre::Vector4& bounds);
        Ogre::Vector4 getCityBounds(void) const;

        /**
         * @brief Master RNG seed.  Same seed + same parameters = identical city.
         */
        void         setMasterSeed(unsigned int seed);
        unsigned int getMasterSeed(void) const;

        /**
         * @brief Road-centre-to-road-centre distance (block pitch) in meters.
         *        Controls how many blocks fit inside cityBounds.  Typical: 30-80m.
         */
        void       setBlockSize(Ogre::Real size);
        Ogre::Real getBlockSize(void) const;

        // ---- Internal road geometry ----------------------------------------

        /**
         * @brief Width of the drivable road surface in meters (per direction).
         *        Default 4m. Also propagated to the external road component's
         *        connecting segment (if roadComponentId is set) via setRoadWidth().
         */
        void       setRoadWidth(Ogre::Real width);
        Ogre::Real getRoadWidth(void) const;

        /**
         * @brief Setback between road edge and building face in meters.
         *        Buildings are inset by this amount from each block edge.
         */
        void       setSidewalkWidth(Ogre::Real width);
        Ogre::Real getSidewalkWidth(void) const;

        /**
         * @brief Height of the kerb strip in meters (0 = flat, no kerb).
         */
        void       setCurbHeight(Ogre::Real height);
        Ogre::Real getCurbHeight(void) const;

        /**
         * @brief Whether to generate internal road-strip geometry.
         *        If false, only buildings (and optional courtyard walls) are created.
         */
        void setGenerateRoads(bool generate);
        bool getGenerateRoads(void) const;

        /**
         * @brief PBS datablock for the road surface (asphalt / cobble / gravel).
         *        Placeholder default: "city_road_01"
         */
        void         setRoadDatablock(const Ogre::String& name);
        Ogre::String getRoadDatablock(void) const;

        /**
         * @brief PBS datablock for the kerb / curb strip.
         *        Placeholder default: "city_curb_01"
         */
        void         setCurbDatablock(const Ogre::String& name);
        Ogre::String getCurbDatablock(void) const;

        // ---- Internal courtyard wall geometry ------------------------------

        /**
         * @brief Whether to generate solid courtyard walls around block perimeters.
         *        Uses the same geometry as ProceduralWallComponent::generateSolidWall()
         *        but produced inline — no external wall component needed.
         */
        void setGenerateWalls(bool generate);
        bool getGenerateWalls(void) const;

        /**
         * @brief Height of the courtyard perimeter wall in meters.  Default 2m.
         */
        void       setWallHeight(Ogre::Real height);
        Ogre::Real getWallHeight(void) const;

        /**
         * @brief PBS datablock for the courtyard wall surface.
         *        Placeholder default: "city_courtyard_wall_01"
         */
        void         setWallDatablock(const Ogre::String& name);
        Ogre::String getWallDatablock(void) const;

        /**
         * @brief PBS datablock for house doors — one inset quad per building on
         *        the front face, centred in the trim band at ground level.
         *        Shared by all buildings in the city (global, not per-district).
         *        Placeholder default: "city_door_01"
         */
        void         setDoorDatablock(const Ogre::String& name);
        Ogre::String getDoorDatablock(void) const;

        /**
         * @brief PBS datablock for garage doors. ~30 % of Residential buildings get
         *        an attached garage box on their right face.  Placeholder: "city_garage_01".
         */
        void         setGarageDatablock(const Ogre::String& name);
        Ogre::String getGarageDatablock(void) const;

        // ---- External road connection (optional) ---------------------------

        /**
         * @brief GameObject ID carrying the ProceduralRoadComponent this city
         *        should connect to.  0 = no external road connection.
         *
         *        When non-zero, generateCity() locates the road component,
         *        determines its connection endpoint (see roadConnectionAtStart),
         *        finds the nearest city grid boundary intersection, and calls
         *        ProceduralRoadComponent::addRoadSegmentLua(roadEndpoint,
         *        cityBoundaryEntry) once.  The external road component owns
         *        that segment; the city owns everything inside the grid.
         *
         *        Requires ProceduralRoadComponent to expose two new getters
         *        (no new data — just public access to existing private members):
         *
         *          Ogre::Vector3 getRoadConnectionPoint(bool atStart) const;
         *            // true  -> roadOrigin + roadOrigin.y height (first segment start)
         *            // false -> loadedRoadEndpoint + loadedRoadEndpointHeight (last end)
         *
         *          Ogre::Vector3 getRoadApproachDirection(bool atStart) const;
         *            // Direction pointing AWAY from the city at the endpoint,
         *            // used to find the nearest city boundary face to connect to.
         *            // true  -> first segment's forward direction, reversed
         *            // false -> last segment's forward direction (already points away)
         */
        void          setRoadComponentId(unsigned long id);
        unsigned long getRoadComponentId(void) const;

        /**
         * @brief Which end of the external road to connect to.
         *        true  = connect at the road's start point (first click, roadOrigin).
         *                Use when the designer drew the road FROM the city outward.
         *        false = connect at the road's end point (last segment endpoint).
         *                Use when the designer drew the road TOWARD the city.
         *        Default: false (connect at road end — the more natural case when
         *        a highway is drawn toward the city location).
         */
        void setRoadConnectionAtStart(bool atStart);
        bool getRoadConnectionAtStart(void) const;

        // ---- District management ------------------------------------------

        /**
         * @brief Number of city districts.  Like ruleCount in foliage: increasing
         *        this creates new Variant entries in the editor; decreasing removes
         *        them and triggers an AttrActionNeedRefresh.  Districts are mapped
         *        onto city blocks via a Voronoi cell system seeded from masterSeed.
         */
        void         setDistrictCount(unsigned int count);
        unsigned int getDistrictCount(void) const;

        void          setDistrictName(unsigned int index, const Ogre::String& name);
        Ogre::String  getDistrictName(unsigned int index) const;

        /**
         * @brief Building type for district i.
         *        Values: "Residential_Low", "Residential_Mid", "Commercial",
         *                "Tower", "Industrial", "Mixed".
         *        Controls default height range and footprint proportions.
         *        Per-type height defaults:
         *          Residential_Low  minH=4  maxH=9
         *          Residential_Mid  minH=8  maxH=18
         *          Commercial       minH=6  maxH=15
         *          Tower            minH=20 maxH=60
         *          Industrial       minH=5  maxH=12
         *          Mixed            minH=4  maxH=15
         */
        void         setDistrictType(unsigned int index, const Ogre::String& type);
        Ogre::String getDistrictType(unsigned int index) const;

        void       setDistrictMinHeight(unsigned int index, Ogre::Real height);
        Ogre::Real getDistrictMinHeight(unsigned int index) const;

        /**
         * @brief Maximum building height for district i.
         *        AAA variance: every building picks a random height in
         *        [minHeight, maxHeight] from BuildingInstance::variantSeed.
         */
        void       setDistrictMaxHeight(unsigned int index, Ogre::Real height);
        Ogre::Real getDistrictMaxHeight(unsigned int index) const;

        void          setDistrictMinFootprint(unsigned int index, const Ogre::Vector2& fp);
        Ogre::Vector2 getDistrictMinFootprint(unsigned int index) const;

        void          setDistrictMaxFootprint(unsigned int index, const Ogre::Vector2& fp);
        Ogre::Vector2 getDistrictMaxFootprint(unsigned int index) const;

        /**
         * @brief Fraction [0..1] of lots in district i that contain a building.
         *        Default 0.85.
         */
        void       setDistrictDensity(unsigned int index, Ogre::Real density);
        Ogre::Real getDistrictDensity(unsigned int index) const;

        /**
         * @brief Probability [0..1] that a block in district i gets a courtyard
         *        wall along its perimeter.  Ignored if generateWalls is false.
         *        Default 0.3.
         */
        void       setDistrictCourtyardProb(unsigned int index, Ogre::Real prob);
        Ogre::Real getDistrictCourtyardProb(unsigned int index) const;

        /**
         * @brief Wall surface datablock for district i, variant v (0..3).
         *        Each building randomly picks one of the non-empty variants so
         *        adjacent buildings differ visually — the AAA variance approach.
         *
         *        Placeholder defaults (fill in your actual datablock names):
         *          v=0  "city_wall_01"
         *          v=1  "city_wall_02"
         *          v=2  "city_wall_03"
         *          v=3  "city_wall_04"
         */
        void         setDistrictWallDatablock(unsigned int districtIdx, unsigned int variantIdx, const Ogre::String& name);
        Ogre::String getDistrictWallDatablock(unsigned int districtIdx, unsigned int variantIdx) const;

        /**
         * @brief Roof datablock for district i, variant v (0..2).
         *        Placeholder defaults:
         *          v=0  "city_roof_01"
         *          v=1  "city_roof_02"
         *          v=2  "city_roof_03"
         */
        void         setDistrictRoofDatablock(unsigned int districtIdx, unsigned int variantIdx, const Ogre::String& name);
        Ogre::String getDistrictRoofDatablock(unsigned int districtIdx, unsigned int variantIdx) const;

        /**
         * @brief Window / glass datablock for district i, variant v (0..2).
         *        Placeholder defaults:
         *          v=0  "city_window_01"
         *          v=1  "city_window_02"
         *          v=2  "city_window_03"
         */
        void         setDistrictWindowDatablock(unsigned int districtIdx, unsigned int variantIdx, const Ogre::String& name);
        Ogre::String getDistrictWindowDatablock(unsigned int districtIdx, unsigned int variantIdx) const;

        /**
         * @brief Ground-level trim datablock (doors, storefronts, signage zone —
         *        the bottom ~1m strip of each wall face), variant v (0..1).
         *        Placeholder defaults:
         *          v=0  "city_trim_01"
         *          v=1  "city_trim_02"
         */
        void         setDistrictTrimDatablock(unsigned int districtIdx, unsigned int variantIdx, const Ogre::String& name);
        Ogre::String getDistrictTrimDatablock(unsigned int districtIdx, unsigned int variantIdx) const;

        // ---- Generation API -----------------------------------------------

        /**
         * @brief Full (re)generation entry point.
         *        1. clearCity(true)  — destroy existing, delete cache
         *        2. generateCityLayout() — logic thread, parallelisable
         *        3. connectExternalRoad() — optional, drives external road comp
         *        4. saveCityDataToFile()
         *        5. createCityOnRenderThread()
         */
        void generateCity(void);

        /**
         * @brief Destroy all Items + SceneNodes.
         *        deleteCacheFile=false in onRemoveComponent() (scene teardown),
         *        deleteCacheFile=true on "Clear" button and at start of generateCity().
         */
        void clearCity(bool deleteCacheFile = false);

    private:

        // ---- Internal structs ----------------------------------------------

        /**
         * @brief Per-district configuration.
         *        Note: `wallDatablocks[0]` is always used.  Slots [1..3] expand
         *        variance only when non-empty — the RNG picks from however many
         *        non-empty slots exist.
         */
        struct CityDistrict
        {
            Ogre::String  name            = "District";
            Ogre::String  type            = "Residential_Low";
            Ogre::Real    minHeight       = 4.0f;
            Ogre::Real    maxHeight       = 9.0f;
            Ogre::Vector2 minFootprint    = Ogre::Vector2(6.0f,  6.0f);
            Ogre::Vector2 maxFootprint    = Ogre::Vector2(12.0f, 12.0f);
            Ogre::Real    density         = 0.85f;
            Ogre::Real    courtyardProb   = 0.3f;

            // AAA variance: multiple datablocks per material slot.
            // One is selected per building from BuildingInstance::variantSeed.
            Ogre::String  wallDatablocks[4]   = { "city_wall_01",   "city_wall_02",   "city_wall_03",   "city_wall_04"   };
            Ogre::String  roofDatablocks[3]   = { "city_roof_01",   "city_roof_02",   "city_roof_03"                     };
            Ogre::String  windowDatablocks[3] = { "city_window_01", "city_window_02", "city_window_03"                   };
            Ogre::String  trimDatablocks[2]   = { "city_trim_01",   "city_trim_02"                                       };
        };

        /**
         * @brief One lot within a block — temporary, not stored in cache.
         */
        struct CityLot
        {
            Ogre::Vector3 centre;       ///< World-space XYZ lot centre
            Ogre::Vector2 size;         ///< XZ footprint in meters
            Ogre::Real    groundHeight; ///< Terrain Y at lot centre
            unsigned int  districtIdx;
            bool          occupied;
        };

        /**
         * @brief One rectangular city block (between four road strips).
         */
        struct CityBlock
        {
            Ogre::Vector3            centre;           ///< World-space block centre
            Ogre::Vector2            size;             ///< XZ footprint (road-to-road)
            Ogre::Real               groundHeight;
            unsigned int             districtIdx;
            bool                     hasCourtyardWall;
            std::vector<CityLot>     lots;
        };

        // ---- Generation pipeline ------------------------------------------

        /**
         * @brief Logic-thread entry point.  Subdivides cityBounds into a block
         *        grid, assigns districts via Voronoi seeding (masterSeed), splits
         *        each block into lots, places BuildingInstances.  Returns one
         *        CityBatch per material slot (wall/roof/window/trim), each batch
         *        containing all BuildingInstances for that slot across all districts.
         *        Geometry accumulation (rawVertices/rawIndices) happens inside
         *        this call via generateBuildingGeometry().
         */
        std::vector<CityBatch> generateCityLayout(void);

        /**
         * @brief Assign districts to blocks via Voronoi seeding.
         *        Seed points are distributed pseudo-randomly inside cityBounds
         *        using masterSeed so the same seed always gives the same district
         *        map.
         */
        void assignDistricts(std::vector<CityBlock>& blocks);

        /**
         * @brief Subdivide block into CityLots using a jittered grid.
         *        Lot sizes drawn from [district.minFootprint, maxFootprint] with
         *        jitter from masterSeed + blockIdx + lotIdx.
         *        Samples terrain height at each lot centre via a downward ray
         *        (same getGroundHeight() pattern as the wall/road components).
         */
        void subdivideBlock(CityBlock& block, unsigned int blockIdx);

        /**
         * @brief Generate geometry for one BuildingInstance into the four
         *        per-slot accumulation buffers.
         *
         *        Building coordinate system:
         *          wallDir = building's forward axis (derived from orientation)
         *          right   = wallDir.crossProduct(Ogre::Vector3::UNIT_Y)
         *          NEVER swap the cross product operands — see class doc.
         *
         *        Geometry produced:
         *          Walls (slot 0):   4 side faces + 1 parapet cap quad
         *          Roof  (slot 1):   flat=1 quad / gabled=2 slope + 2 gable tris /
         *                            hip=4 slope quads
         *          Windows (slot 2): NxM grid of inset quads per face, skipped on
         *                            Industrial type and on faces < 3m wide
         *          Trim (slot 3):    one horizontal strip quad per face at ground level,
         *                            height = min(1.5m, wallHeight * 0.2)
         *
         *        All positions are in WORLD space — no local-origin subtraction.
         *        The cell-merge step in createCityOnRenderThread() handles spatial
         *        grouping; there is no per-building local origin.
         */
        void generateBuildingGeometry(const BuildingInstance&  building,
                                      const CityDistrict&      district,
                                      const Ogre::Vector3&     localOrigin,
                                      std::vector<float>&      wallV,    std::vector<Ogre::uint32>& wallI,    size_t& wallN,
                                      std::vector<float>&      roofV,    std::vector<Ogre::uint32>& roofI,    size_t& roofN,
                                      std::vector<float>&      windowV,  std::vector<Ogre::uint32>& windowI,  size_t& windowN,
                                      std::vector<float>&      trimV,    std::vector<Ogre::uint32>& trimI,    size_t& trimN,
                                      std::vector<float>&      doorV,    std::vector<Ogre::uint32>& doorI,    size_t& doorN,
                                      std::vector<float>&      garageV,  std::vector<Ogre::uint32>& garageI,  size_t& garageN);

        /**
         * @brief Drive the auto-created ProceduralRoadComponent to lay out the road grid
         *        between blocks. Called from generateCityLayout() when generateRoads is true.
         *        Replaces the old generateRoadGeometry() approach — ProceduralRoadComponent
         *        already handles terrain following, junction patches, miter joins, smoothing.
         */
        void buildCityRoadNetwork(const std::vector<CityBlock>& blocks);

        /**
         * @brief Generate internal road-strip geometry into the road and curb
         *        accumulation buffers. KEPT FOR REFERENCE but no longer called —
         *        replaced by buildCityRoadNetwork().
         */
        void generateRoadGeometry(const std::vector<CityBlock>& blocks,
                                  const Ogre::Vector3&           localOrigin,
                                  std::vector<float>&            roadV, std::vector<Ogre::uint32>& roadI, size_t& roadN,
                                  std::vector<float>&            curbV, std::vector<Ogre::uint32>& curbI, size_t& curbN);

        /**
         * @brief Generate solid courtyard-wall geometry (matching
         *        ProceduralWallComponent::generateSolidWall() vertex layout)
         *        for all blocks where hasCourtyardWall is true.
         *        NOT called if generateWalls is false.
         */
        void generateCourtyardWallGeometry(const std::vector<CityBlock>& blocks,
                                           const Ogre::Vector3&           localOrigin,
                                           std::vector<float>&            wallV, std::vector<Ogre::uint32>& wallI, size_t& wallN);

        /**
         * @brief Connect city to external ProceduralRoadComponent (optional).
         *        Called after generateCityLayout() if roadComponentId != 0.
         *        Finds the road component, reads its connection endpoint via
         *        getRoadConnectionPoint(roadConnectionAtStart) and approach
         *        direction via getRoadApproachDirection(roadConnectionAtStart),
         *        then locates the nearest city grid boundary intersection and
         *        calls roadComp->addRoadSegmentLua(roadEndpoint, boundaryEntry).
         *
         *        Also calls roadComp->setRoadWidth(roadWidth) before adding
         *        the segment so the connecting segment matches the city's internal
         *        road width.
         */
        void connectExternalRoad(const std::vector<CityBlock>& blocks);

        // ---- Core geometry helper ------------------------------------------

        /**
         * @brief Emit one quad (4 vertices, 6 indices) into verts/idx.
         *
         *        WINDING: v0, v1, v2, v3 must be in CCW order as seen from
         *        the direction the normal points.  The normal is ALWAYS passed
         *        explicitly — it is NEVER computed here from a cross product.
         *
         *        UV assignment:
         *          v0 -> (0,    0   )
         *          v1 -> (uLen, 0   )
         *          v2 -> (uLen, vLen)
         *          v3 -> (0,    vLen)
         *
         *        Output format: 8 floats/vertex (pos.xyz + normal.xyz + uv.xy).
         *        On the render thread this is expanded to 12 floats (adding a
         *        normal-derived tangent), identical to ProceduralRoad/Wall.
         */
        void pushQuad(std::vector<float>&       verts,
                      std::vector<Ogre::uint32>& idx,
                      size_t&                   vertCount,
                      const Ogre::Vector3& v0, const Ogre::Vector3& v1,
                      const Ogre::Vector3& v2, const Ogre::Vector3& v3,
                      const Ogre::Vector3& normal,
                      Ogre::Real uLen, Ogre::Real vLen);

        // ---- Render-thread operations -------------------------------------

        /**
         * @brief Create all SCENE_STATIC Items and SceneNodes on the render thread.
         *        Same 10m cell-merge strategy as ProceduralFoliageVolumeComponent:
         *        raw vertex data per material slot is split by cell, each
         *        (cell, slot) pair becomes one BT_IMMUTABLE Item.
         *        Calls notifyStaticAabbDirty(item) + notifyStaticDirty(rootStatic)
         *        after all items are created — required to avoid the invisible-on-
         *        first-frame bug documented in ProceduralFoliageVolumeComponent.
         *        Also calls vaoManager->_update() before creating buffers.
         */
        void createCityOnRenderThread(std::vector<CityBatch>&& batches);

        /**
         * @brief Destroy all Items and SceneNodes.  Must run on the render thread
         *        (wrapped in enqueueAndWait by clearCity()).
         *        For each batch item: detach from node, destroyItem(), remove
         *        the "CityCell_*" mesh from MeshManager, destroySceneNode().
         *        Calls notifyStaticDirty(rootStatic) at the end.
         */
        void destroyCityOnRenderThread(void);

        // ---- Cache (ProceduralFoliageVolumeComponent pattern) -------------

        /**
         * @brief 64-bit checksum of every generation parameter.
         *        Uses the same foliageHashCombine / foliageHashReal helpers as
         *        ProceduralFoliageVolumeComponent (Reals quantised to 3 decimal
         *        places before hashing to avoid false invalidation from float jitter).
         *        Covers: cityBounds, masterSeed, blockSize, roadWidth, sidewalkWidth,
         *        curbHeight, wallHeight, district count, and all per-district fields.
         */
        uint64_t computeChecksum(void) const;

        /**
         * @brief Path: projectPath/sceneName/City_{gameObjectId}.citydata
         *        (projectPath/City_{gameObjectId}.citydata for global objects).
         */
        Ogre::String getCityDataFilePath(void) const;

        /**
         * @brief Write header: magic('CITY'), version(uint32), checksum(uint64),
         *        batchCount(uint32).
         *        Per batch: materialSlot(uint32), districtIdx(uint64),
         *        instanceCount(uint64).
         *        Per instance: position(3 floats), orientation(4 floats quat wxyz),
         *        footprint(3 floats), archetypeIdx(uint32), variantSeed(uint32),
         *        roofType(uint32), roofPitch(float), groundHeight(float).
         */
        bool saveCityDataToFile(const std::vector<CityBatch>& batches);

        /**
         * @brief Read header, validate magic + version + checksum.
         *        Checksum mismatch means stale cache (parameters changed) →
         *        return false so generateCityLayout() runs instead.
         *        On success populate outBatches.instances only; geometry is
         *        regenerated from these in createCityOnRenderThread().
         */
        bool loadCityDataFromFile(std::vector<CityBatch>& outBatches);

        /**
         * @brief std::remove on the .citydata file.  Safe if file is missing.
         */
        void deleteCityDataFile(void);

        /**
         * @brief Called from handleSceneParsed().  Tries cache first, falls
         *        back to full generateCity() if missing / corrupt / stale.
         */
        void loadOrGenerateCity(void);

        // ---- Ground height ------------------------------------------------

        /**
         * @brief Fire a downward ray from (pos.x, pos.y+1000, pos.z) and return
         *        the hit Y + heightOffset.  Falls back to groundPlane intersection.
         *        Identical pattern to ProceduralRoadComponent::getGroundHeight().
         *        Used during lot subdivision to sample terrain height at each lot
         *        centre before the geometry pass.
         */
        Ogre::Real getGroundHeight(const Ogre::Vector3& position) const;

        // ---- Helpers ------------------------------------------------------

        /**
         * @brief Locate the ProceduralRoadComponent on the GameObject whose ID
         *        is roadComponentId.  Returns nullptr if id is 0 or not found.
         */
        ProceduralRoadComponent* findRoadComponent(void) const;

        // ---- Event handlers -----------------------------------------------

        /**
         * @brief Deferred load after all postInits have run.
         *        Mirrors ProceduralFoliageVolumeComponent::handleSceneParsed().
         */
        void handleSceneParsed(NOWA::EventDataPtr eventData);

        /**
         * @brief Delete .citydata file when this component is explicitly deleted
         *        in the editor (EventDataDeleteComponent fires with our ID).
         *        Does NOT fire on normal scene teardown — only on explicit deletion.
         */
        void handleComponentManuallyDeleted(NOWA::EventDataPtr eventData);

        // ---- Cache version ------------------------------------------------

        static constexpr uint32_t CITY_CACHE_VERSION = 1u;

        // ---- Variant attributes -------------------------------------------

        Variant* activated;
        Variant* cityBoundsAttr;           // Vector4: minX, minZ, maxX, maxZ
        Variant* masterSeedAttr;           // uint
        Variant* blockSizeAttr;            // Real, meters
        Variant* roadWidthAttr;            // Real, meters
        Variant* sidewalkWidthAttr;        // Real, meters
        Variant* curbHeightAttr;           // Real, meters
        Variant* generateRoadsAttr;        // bool
        Variant* roadDatablockAttr;        // String
        Variant* curbDatablockAttr;        // String
        Variant* generateWallsAttr;        // bool
        Variant* wallHeightAttr;           // Real, meters
        Variant* wallDatablockAttr;        // String
        Variant* doorDatablockAttr;        // String — global door texture for all buildings
        Variant* garageDatablockAttr;
        Variant* roadComponentIdAttr;      // String (large uint stored as string)
        Variant* roadConnectionAtStartAttr;// bool
        Variant* districtCountAttr;        // uint, triggers AttrActionNeedRefresh
        Variant* generateBtn;              // action button: Generate Now
        Variant* clearBtn;                 // action button: Clear

        // Per-district Variant arrays (parallel to this->districts[])
        std::vector<Variant*> districtNameAttrs;
        std::vector<Variant*> districtTypeAttrs;
        std::vector<Variant*> districtMinHeightAttrs;
        std::vector<Variant*> districtMaxHeightAttrs;
        std::vector<Variant*> districtMinFootprintAttrs;
        std::vector<Variant*> districtMaxFootprintAttrs;
        std::vector<Variant*> districtDensityAttrs;
        std::vector<Variant*> districtCourtyardProbAttrs;
        // [districtIdx][variantIdx]
        std::vector<std::array<Variant*, 4>> districtWallDbAttrs;
        std::vector<std::array<Variant*, 3>> districtRoofDbAttrs;
        std::vector<std::array<Variant*, 3>> districtWindowDbAttrs;
        std::vector<std::array<Variant*, 2>> districtTrimDbAttrs;

        // ---- Internal runtime state ---------------------------------------

        Ogre::String                  name;
        std::vector<CityDistrict>     districts;
        std::vector<CityBatch>        cityBatches;

        PhysicsArtifactComponent*     physicsArtifactComponent;

        unsigned long                 roadComponentId;
        bool                          roadConnectionAtStart;

        Ogre::RaySceneQuery*          groundQuery;
        Ogre::Plane                   groundPlane;

        Ogre::Vector3                 cityOrigin;       ///< World-space XYZ origin of the city grid; all geometry is stored relative to this. Set in generateCityLayout(), used in createCityOnRenderThread() to position the SCENE_STATIC nodes.

        bool                          isDirty;
        bool                          cityLoadedFromScene;
    };

} // namespace NOWA

#endif // PROCEDURAL_CITY_COMPONENT_H

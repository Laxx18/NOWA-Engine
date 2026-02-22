/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#ifndef PROCEDURAL_DUNGEON_COMPONENT_H
#define PROCEDURAL_DUNGEON_COMPONENT_H

#include "OgrePlugin.h"
#include "gameobject/GameObjectComponent.h"

#include <memory>
#include <random>
#include <vector>

namespace NOWA
{
    /**
     * @class ProceduralDungeonComponent
     * @brief Rule-based procedural dungeon / level generator component.
     *
     * Features:
     * - BSP (Binary Space Partitioning) room placement – industry-proven approach
     * - MST-based corridor connectivity, with optional loop corridors for non-linear gameplay
     * - Cell-grid geometry: floors, walls with per-face door openings, optional ceiling
     * - Five dungeon themes with theme-specific geometry extras (caps, stalactites, grating)
     * - Three-submesh system: floor / wall / ceiling – each with its own datablock
     * - Terrain adaptation: dungeon floor follows terrain at origin; ground raycasting included
     * - Real-time interactive placement: click on terrain to position dungeon, preview bounding box first
     * - Full undo / redo support via NOWA event system
     * - Binary save/load (room + corridor data + cached vertex data)
     * - ConvertToMesh export: bake to static .mesh for optimal GPU caching
     * - Optional corner pillars with configurable size
     * - Seed-reproducible generation: same seed always yields the same layout
     */
    class EXPORTED ProceduralDungeonComponent : public GameObjectComponent, public Ogre::Plugin, public OIS::MouseListener, public OIS::KeyListener
    {
    public:
        typedef boost::shared_ptr<ProceduralDungeonComponent> ProceduralDungeonComponentPtr;

        /**
         * @brief Five dungeon themes, each with distinct geometry and aesthetic defaults.
         */
        enum class DungeonTheme
        {
            DUNGEON = 0, ///< Classic stone dungeon – battlement wall caps, regular proportions
            CAVE = 1,    ///< Natural cave   – variable wall height per column, no sharp corners
            SCIFI = 2,   ///< Sci-fi facility – ceiling always enabled, floor channel strips
            ICE = 3,     ///< Ice dungeon     – wall faces angled inward at the top
            CRYPT = 4    ///< Undead crypt    – tall battlement caps, forced narrow corridors
        };

        /**
         * @brief Interaction state machine for the placement workflow.
         */
        enum class BuildState
        {
            IDLE = 0, ///< Waiting; no listeners
            PLACING,  ///< Mouse over terrain – bounding-box preview visible
            GENERATED ///< Dungeon mesh placed; can re-generate with new seed
        };

        /**
         * @brief Per-cell type on the internal generation grid.
         */
        enum class CellType : uint8_t
        {
            EMPTY = 0,   ///< Solid / outside dungeon
            ROOM = 1,    ///< Room floor cell
            CORRIDOR = 2 ///< Corridor floor cell
        };

        /**
         * @brief A rectangular room in cell-space coordinates.
         *
         * All positions are relative to the dungeon's top-left corner (0, 0).
         * World-space is computed as:  worldX = dungeonOrigin.x + col * cellSize
         *                              worldZ = dungeonOrigin.z + row * cellSize
         */
        struct DungeonRoom
        {
            int id = -1;                   ///< Unique index into ProceduralDungeonComponent::dungeonRooms
            int col = 0;                   ///< Left column  (inclusive)
            int row = 0;                   ///< Top row      (inclusive)
            int cols = 0;                  ///< Width  in cells
            int rows = 0;                  ///< Depth  in cells
            Ogre::Real floorHeight = 0.0f; ///< World-space Y of the floor surface

            /// Convenience: centre cell column
            int centerCol(void) const
            {
                return col + cols / 2;
            }
            /// Convenience: centre cell row
            int centerRow(void) const
            {
                return row + rows / 2;
            }
        };

        /**
         * @brief An L-shaped corridor connecting two rooms in cell-space.
         *
         * The corridor is described by two orthogonal segments that share a bend point:
         *   Segment 1 (horizontal): row = bendRow, columns from room-A-center to bendCol
         *   Segment 2 (vertical):   col = bendCol, rows from bendRow to room-B-center
         *
         * Each segment is "expanded" by corridorWidthCells/2 cells perpendicular to
         * its direction when marking the grid.
         */
        struct DungeonCorridor
        {
            int fromRoomId = -1; ///< Source room id
            int toRoomId = -1;   ///< Destination room id
            int bendCol = 0;     ///< Column at which the L-bend occurs
            int bendRow = 0;     ///< Row    at which the L-bend occurs
        };

        /**
         * @brief Internal BSP tree node used during generation only.
         *
         * Leaf nodes carry a roomId; internal nodes split their region.
         */
        struct BSPNode
        {
            int col = 0;  ///< Region left column
            int row = 0;  ///< Region top row
            int cols = 0; ///< Region width in cells
            int rows = 0; ///< Region depth in cells

            std::unique_ptr<BSPNode> left;
            std::unique_ptr<BSPNode> right;

            int roomId = -1; ///< Valid only in leaf nodes
            bool splitHorizontal = false;
        };

    public:
        ProceduralDungeonComponent();
        virtual ~ProceduralDungeonComponent();

        // -------------------------------------------------------------------------
        // Ogre::Plugin interface
        // -------------------------------------------------------------------------

        /**
         * @see Ogre::Plugin::install
         */
        virtual void install(const Ogre::NameValuePairList* options) override;

        /**
         * @see Ogre::Plugin::initialise
         */
        virtual void initialise() override;

        /**
         * @see Ogre::Plugin::shutdown
         */
        virtual void shutdown() override;

        /**
         * @see Ogre::Plugin::uninstall
         */
        virtual void uninstall() override;

        /**
         * @see Ogre::Plugin::getName
         */
        virtual const Ogre::String& getName() const override;

        /**
         * @see Ogre::Plugin::getAbiCookie
         */
        virtual void getAbiCookie(Ogre::AbiCookie& outAbiCookie) override;

        // -------------------------------------------------------------------------
        // GameObjectComponent interface
        // -------------------------------------------------------------------------

        /**
         * @see GameObjectComponent::init
         */
        virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

        /**
         * @see GameObjectComponent::postInit
         */
        virtual bool postInit(void) override;

        /**
         * @see GameObjectComponent::connect
         */
        virtual bool connect(void) override;

        /**
         * @see GameObjectComponent::disconnect
         */
        virtual bool disconnect(void) override;

        /**
         * @see GameObjectComponent::onCloned
         */
        virtual bool onCloned(void) override;

        /**
         * @see GameObjectComponent::onAddComponent
         */
        virtual void onAddComponent(void) override;

        /**
         * @see GameObjectComponent::onRemoveComponent
         */
        virtual void onRemoveComponent(void);

        /**
         * @see GameObjectComponent::onOtherComponentRemoved
         */
        virtual void onOtherComponentRemoved(unsigned int index) override;

        /**
         * @see GameObjectComponent::onOtherComponentAdded
         */
        virtual void onOtherComponentAdded(unsigned int index) override;

        /**
         * @see GameObjectComponent::clone
         */
        virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

        /**
         * @see GameObjectComponent::update
         */
        virtual void update(Ogre::Real dt, bool notSimulating = false) override;

        /**
         * @see GameObjectComponent::actualizeValue
         */
        virtual void actualizeValue(Variant* attribute) override;

        /**
         * @see GameObjectComponent::writeXML
         */
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        // -------------------------------------------------------------------------
        // Static identity
        // -------------------------------------------------------------------------

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("ProceduralDungeonComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "ProceduralDungeonComponent";
        }

        static bool canStaticAddComponent(GameObject* gameObject);

        static Ogre::String getStaticInfoText(void)
        {
            return "Usage: In Mesh-Modify mode select this object, then left-click on the terrain "
                   "to place the dungeon. Hold Shift while clicking to re-generate with a new seed. "
                   "Press Ctrl+Z to undo. Press ESC to cancel. "
                   "Use 'Generate Now' to re-generate at the current position.";
        }

        virtual Ogre::String getClassName(void) const override
        {
            return "ProceduralDungeonComponent";
        }

        virtual Ogre::String getParentClassName(void) const override
        {
            return "GameObjectComponent";
        }

        virtual Ogre::String getParentParentClassName(void) const override
        {
            return "GameObjectComponent";
        }

        // -------------------------------------------------------------------------
        // Dungeon operations (public API)
        // -------------------------------------------------------------------------

        /**
         * @brief Runs the BSP+MST algorithm and rebuilds the mesh at the current
         *        game-object position. This is the main entry point for scripting.
         */
        void generateDungeon(void);

        /**
         * @brief Places the dungeon origin at @p worldPosition (terrain-adapted),
         *        then calls generateDungeon().
         */
        void generateDungeonAtPosition(const Ogre::Vector3& worldPosition);

        /**
         * @brief Rebuilds geometry from the cached room/corridor lists without
         *        re-running BSP. Call this after changing visual parameters.
         */
        void rebuildMesh(void);

        /**
         * @brief Removes all dungeon data and destroys the mesh.
         */
        void clearDungeon(void);

        // -------------------------------------------------------------------------
        // Terrain helpers
        // -------------------------------------------------------------------------

        Ogre::Real getGroundHeight(const Ogre::Vector3& position);

        bool raycastGround(Ogre::Real screenX, Ogre::Real screenY, Ogre::Vector3& hitPosition);

        // -------------------------------------------------------------------------
        // Attribute setters / getters
        // -------------------------------------------------------------------------

        void setActivated(bool activated);
        bool isActivated(void) const;

        void setSeed(int seed);
        int getSeed(void) const;

        void setDungeonWidth(Ogre::Real width);
        Ogre::Real getDungeonWidth(void) const;

        void setDungeonDepth(Ogre::Real depth);
        Ogre::Real getDungeonDepth(void) const;

        void setCellSize(Ogre::Real size);
        Ogre::Real getCellSize(void) const;

        void setMinRoomCells(int cells);
        int getMinRoomCells(void) const;

        void setMaxRoomCells(int cells);
        int getMaxRoomCells(void) const;

        void setCorridorWidthCells(int cells);
        int getCorridorWidthCells(void) const;

        void setWallHeight(Ogre::Real height);
        Ogre::Real getWallHeight(void) const;

        void setAddCeiling(bool add);
        bool getAddCeiling(void) const;

        void setLoopProbability(Ogre::Real prob);
        Ogre::Real getLoopProbability(void) const;

        void setDungeonTheme(const Ogre::String& theme);
        Ogre::String getDungeonTheme(void) const;

        void setAdaptToGround(bool adapt);
        bool getAdaptToGround(void) const;

        void setHeightOffset(Ogre::Real offset);
        Ogre::Real getHeightOffset(void) const;

        void setAddPillars(bool add);
        bool getAddPillars(void) const;

        void setPillarSize(Ogre::Real size);
        Ogre::Real getPillarSize(void) const;

        void setFloorDatablock(const Ogre::String& datablock);
        Ogre::String getFloorDatablock(void) const;

        void setWallDatablock(const Ogre::String& datablock);
        Ogre::String getWallDatablock(void) const;

        void setCeilingDatablock(const Ogre::String& datablock);
        Ogre::String getCeilingDatablock(void) const;

        void setFloorUVTiling(const Ogre::Vector2& tiling);
        Ogre::Vector2 getFloorUVTiling(void) const;

        void setWallUVTiling(const Ogre::Vector2& tiling);
        Ogre::Vector2 getWallUVTiling(void) const;

        // -------------------------------------------------------------------------
        // Undo / Redo binary serialisation (mirrors RoadComponentBase interface)
        // -------------------------------------------------------------------------

        virtual void setDungeonData(const std::vector<unsigned char>& data);
        virtual std::vector<unsigned char> getDungeonData(void) const;

    public:
        // -------------------------------------------------------------------------
        // Static attribute name accessors
        // -------------------------------------------------------------------------

        static Ogre::String AttrActivated(void)
        {
            return "Activated";
        }
        static Ogre::String AttrSeed(void)
        {
            return "Seed";
        }
        static Ogre::String AttrDungeonWidth(void)
        {
            return "Dungeon Width";
        }
        static Ogre::String AttrDungeonDepth(void)
        {
            return "Dungeon Depth";
        }
        static Ogre::String AttrCellSize(void)
        {
            return "Cell Size";
        }
        static Ogre::String AttrMinRoomCells(void)
        {
            return "Min Room Cells";
        }
        static Ogre::String AttrMaxRoomCells(void)
        {
            return "Max Room Cells";
        }
        static Ogre::String AttrCorridorWidthCells(void)
        {
            return "Corridor Width Cells";
        }
        static Ogre::String AttrWallHeight(void)
        {
            return "Wall Height";
        }
        static Ogre::String AttrAddCeiling(void)
        {
            return "Add Ceiling";
        }
        static Ogre::String AttrLoopProbability(void)
        {
            return "Loop Probability";
        }
        static Ogre::String AttrDungeonTheme(void)
        {
            return "Dungeon Theme";
        }
        static Ogre::String AttrAdaptToGround(void)
        {
            return "Adapt To Ground";
        }
        static Ogre::String AttrHeightOffset(void)
        {
            return "Height Offset";
        }
        static Ogre::String AttrAddPillars(void)
        {
            return "Add Pillars";
        }
        static Ogre::String AttrPillarSize(void)
        {
            return "Pillar Size";
        }
        static Ogre::String AttrFloorDatablock(void)
        {
            return "Floor Datablock";
        }
        static Ogre::String AttrWallDatablock(void)
        {
            return "Wall Datablock";
        }
        static Ogre::String AttrCeilingDatablock(void)
        {
            return "Ceiling Datablock";
        }
        static Ogre::String AttrFloorUVTiling(void)
        {
            return "Floor UV Tiling";
        }
        static Ogre::String AttrWallUVTiling(void)
        {
            return "Wall UV Tiling";
        }
        static Ogre::String AttrGenerateNow(void)
        {
            return "Generate Now";
        }
        static Ogre::String AttrConvertToMesh(void)
        {
            return "Convert To Mesh";
        }

    protected:
        // -------------------------------------------------------------------------
        // OIS mouse & key callbacks
        // -------------------------------------------------------------------------

        virtual bool mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;
        virtual bool mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;
        virtual bool mouseMoved(const OIS::MouseEvent& evt) override;
        virtual bool keyPressed(const OIS::KeyEvent& evt) override;
        virtual bool keyReleased(const OIS::KeyEvent& evt) override;
        virtual bool executeAction(const Ogre::String& actionId, NOWA::Variant* attribute) override;

    private:
        // -------------------------------------------------------------------------
        // BSP generation
        // -------------------------------------------------------------------------

        /**
         * @brief Recursively splits @p node until subregions are too small.
         *
         * A split is rejected when either child would be smaller than
         * (minRoomCells + 2) in both dimensions.
         *
         * @param node  Current region to split (modified in-place)
         * @param rng   Seeded PRNG
         * @param depth Current recursion depth (guards against overly deep trees)
         */
        void splitBSP(BSPNode* node, std::mt19937& rng, int depth = 0);

        /**
         * @brief Places a single room inside a leaf node's region.
         *
         * Room size is sampled uniformly from [minRoomCells, maxRoomCells] clamped
         * to the available region minus a one-cell border margin.
         */
        void placeRoomInLeaf(BSPNode* node, std::vector<DungeonRoom>& rooms, std::mt19937& rng);

        /**
         * @brief DFS traversal – places a room in every leaf, populates @p rooms.
         */
        void collectLeafRooms(BSPNode* node, std::vector<DungeonRoom>& rooms, std::mt19937& rng);

        /**
         * @brief Returns the room-id of a randomly chosen leaf in the subtree rooted at @p node.
         */
        int getRandomLeafRoomId(BSPNode* node, std::mt19937& rng);

        /**
         * @brief Recursively connects sibling subtrees bottom-up.
         *
         * For each internal node, one room from the left subtree is connected to
         * one room from the right subtree via an L-shaped corridor.
         */
        void connectBSPSubtrees(BSPNode* node, const std::vector<DungeonRoom>& rooms, std::vector<DungeonCorridor>& corridors, std::mt19937& rng);

        /**
         * @brief Adds extra random corridors with probability @p loopProbability
         *        to create cycles and avoid pure tree-graph layouts.
         */
        void addLoopCorridors(const std::vector<DungeonRoom>& rooms, std::vector<DungeonCorridor>& corridors, std::mt19937& rng);

        // -------------------------------------------------------------------------
        // Grid construction
        // -------------------------------------------------------------------------

        /**
         * @brief Builds the boolean cell grid from rooms and corridors.
         *
         * Room cells are marked CellType::ROOM; corridor cells CellType::CORRIDOR.
         * Cells that belong to both (corridor enters room) remain CellType::ROOM.
         *
         * @param rooms      Room list
         * @param corridors  Corridor list
         * @param grid       Output: gridRows × gridCols matrix (row-major)
         * @param gridCols   Number of columns
         * @param gridRows   Number of rows
         */
        void buildGrid(const std::vector<DungeonRoom>& rooms, const std::vector<DungeonCorridor>& corridors, std::vector<std::vector<CellType>>& grid, int gridCols, int gridRows) const;

        /**
         * @brief Marks an L-shaped corridor on the grid with a given half-width.
         */
        void markCorridor(std::vector<std::vector<CellType>>& grid, const DungeonCorridor& corridor, const std::vector<DungeonRoom>& rooms, int corridorHalfW, int gridCols, int gridRows) const;

        // -------------------------------------------------------------------------
        // Geometry generation
        // -------------------------------------------------------------------------

        /**
         * @brief Generates floor quads for every ROOM/CORRIDOR cell in the grid.
         */
        void generateFloorGeometry(const std::vector<std::vector<CellType>>& grid, int gridCols, int gridRows, Ogre::Real cellSz, Ogre::Real floorY, const Ogre::Vector2& uvTile);

        /**
         * @brief Generates wall quads for every floor cell that borders an EMPTY cell.
         */
        void generateWallGeometry(const std::vector<std::vector<CellType>>& grid, int gridCols, int gridRows, Ogre::Real cellSz, Ogre::Real floorY, Ogre::Real wallH, const Ogre::Vector2& uvTile, DungeonTheme theme);

        void generateWallTopCaps(const std::vector<std::vector<CellType>>& grid, int gridCols, int gridRows, Ogre::Real cellSz, Ogre::Real floorY, Ogre::Real wallH, const Ogre::Vector2& uvTile);

        /**
         * @brief Generates ceiling quads covering every floor cell.
         */
        void generateCeilingGeometry(const std::vector<std::vector<CellType>>& grid, int gridCols, int gridRows, Ogre::Real cellSz, Ogre::Real floorY, Ogre::Real wallH, const Ogre::Vector2& uvTile);

        /**
         * @brief Optionally places box pillars at inner room corners.
         *
         * A pillar is placed at a grid corner where two perpendicular wall faces
         * would meet, i.e. where the floor cell's north AND east neighbours are
         * both EMPTY (and the three other orientations).
         */
        void generatePillarGeometry(const std::vector<std::vector<CellType>>& grid, int gridCols, int gridRows, Ogre::Real cellSz, Ogre::Real floorY, Ogre::Real wallH, Ogre::Real pillarSz);

        // -------------------------------------------------------------------------
        // Theme-specific geometry extras
        // -------------------------------------------------------------------------

        /**
         * @brief Adds a thin wall cap (battlement-style) on top of every wall face.
         *        Used by DUNGEON and CRYPT themes.
         * @param capFraction  Cap height as a fraction of wallH (e.g. 0.10 = 10%)
         * @param outwardReach Cap protrudes outward by (cellSz * 0.15)
         */
        void generateWallCaps(const std::vector<std::vector<CellType>>& grid, int gridCols, int gridRows, Ogre::Real cellSz, Ogre::Real floorY, Ogre::Real wallH, Ogre::Real capFraction);

        /**
         * @brief Adds a floor-level channel strip along wall boundaries (SCIFI theme).
         *        Generates a narrow recessed quad at the base of every wall face.
         */
        void generateScifiFloorChannel(const std::vector<std::vector<CellType>>& grid, int gridCols, int gridRows, Ogre::Real cellSz, Ogre::Real floorY, const Ogre::Vector2& uvTile);

        /**
         * @brief Tilts the top edge of every wall face slightly inward (ICE theme).
         *        Produces a subtle angled-ice-crystal silhouette.
         * @param inwardFraction  How far the top edge moves inward as fraction of cellSz.
         */
        void generateIceWallBevel(const std::vector<std::vector<CellType>>& grid, int gridCols, int gridRows, Ogre::Real cellSz, Ogre::Real floorY, Ogre::Real wallH, Ogre::Real inwardFraction, const Ogre::Vector2& uvTile);

        /**
         * @brief Adds small stalactite quads dropping from the ceiling (CAVE theme).
         *        Uses a hash of (col, row, seed) to decide per-cell placement and length.
         */
        void generateStalactites(const std::vector<std::vector<CellType>>& grid, int gridCols, int gridRows, Ogre::Real cellSz, Ogre::Real floorY, Ogre::Real wallH, int seed);

        // -------------------------------------------------------------------------
        // Low-level quad emitters
        // -------------------------------------------------------------------------

        /**
         * @brief Pushes a floor quad (normal = +Y) into the floor vertex/index arrays.
         *
         * Vertex winding: v0(SW), v1(SE), v2(NE), v3(NW) – clockwise when viewed from above.
         */
        void addFloorQuad(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& v3, Ogre::Real u0, Ogre::Real u1, Ogre::Real vv0, Ogre::Real vv1);

        /**
         * @brief Pushes a wall quad into the wall vertex/index arrays.
         *
         * @param normal  Face normal (should be unit length, axis-aligned for best lighting)
         */
        void addWallQuad(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& v3, const Ogre::Vector3& normal, Ogre::Real u0, Ogre::Real u1, Ogre::Real vv0, Ogre::Real vv1);

        /**
         * @brief Pushes a ceiling quad (normal = -Y) into the ceiling vertex/index arrays.
         */
        void addCeilingQuad(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& v3, Ogre::Real u0, Ogre::Real u1, Ogre::Real vv0, Ogre::Real vv1);

        void addWallTopCapQuad(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& v3, Ogre::Real u0, Ogre::Real u1, Ogre::Real vv0, Ogre::Real vv1);

        // -------------------------------------------------------------------------
        // Mesh lifecycle
        // -------------------------------------------------------------------------

        /**
         * @brief Main mesh creation entry point (dispatches to render thread).
         */
        void createDungeonMesh(void);

        /**
         * @brief Render-thread mesh creation using Ogre-Next VAO API.
         *
         * Creates up to 3 submeshes (floor / wall / optional ceiling) from the cached
         * vertex/index arrays and attaches the resulting Item to the game object's scene node.
         *
         * @param origin  World-space dungeon origin (scene node position)
         */
        void createDungeonMeshInternal(const std::vector<float>& floorVerts, const std::vector<Ogre::uint32>& floorIdx, size_t numFloorVerts, const std::vector<float>& wallVerts, const std::vector<Ogre::uint32>& wallIdx, size_t numWallVerts,
            const std::vector<float>& ceilVerts, const std::vector<Ogre::uint32>& ceilIdx, size_t numCeilVerts, const Ogre::Vector3& origin);

        /**
         * @brief Creates a dummy VAO (single degenerate triangle) for an empty submesh,
         *        required so the Item always has the expected number of submeshes.
         */
        Ogre::VertexArrayObject* createDummyVao(Ogre::VaoManager* vaoManager);

        void destroyDungeonMesh(void);
        void destroyPreviewMesh(void);

        /**
         * @brief Updates / recreates the bounding-box preview mesh at @p position.
         *
         * Called every mouse-move while in PLACING state. Uses a cheap axis-aligned
         * wire-box mesh rather than running the full generator.
         */
        void updatePreviewMesh(const Ogre::Vector3& position);

        // -------------------------------------------------------------------------
        // Save / Load
        // -------------------------------------------------------------------------

        Ogre::String getDungeonDataFilePath(void) const;
        bool saveDungeonDataToFile(void);
        bool loadDungeonDataFromFile(void);
        void deleteDungeonDataFile(void);

        // -------------------------------------------------------------------------
        // Event handlers
        // -------------------------------------------------------------------------

        void handleMeshModifyMode(NOWA::EventDataPtr eventData);
        void handleGameObjectSelected(NOWA::EventDataPtr eventData);
        void handleComponentManuallyDeleted(NOWA::EventDataPtr eventData);

        // -------------------------------------------------------------------------
        // Input listener management
        // -------------------------------------------------------------------------

        void addInputListener(void);
        void removeInputListener(void);
        void updateModificationState(void);

        // -------------------------------------------------------------------------
        // Helpers
        // -------------------------------------------------------------------------

        DungeonTheme getDungeonThemeEnum(void) const;
        Ogre::Vector3 snapToGridFunc(const Ogre::Vector3& position);

        /**
         * @brief Per-cell "random" height modifier for CAVE theme.
         *        Uses a fast integer hash of (col, row, seed).
         * @return Value in [-1, +1]
         */
        static float caveHeightNoise(int col, int row, int seed);

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

    private:
        static const uint32_t DUNGEONDATA_MAGIC = 0x44554E47u; ///< "DUNG"
        static const uint32_t DUNGEONDATA_VERSION = 1u;

    private:
        Ogre::String name;

        // ---- Attributes (Variant* owned by this->attributes) --------------------
        Variant* activated;
        Variant* seed;
        Variant* dungeonWidth;
        Variant* dungeonDepth;
        Variant* cellSize;
        Variant* minRoomCells;
        Variant* maxRoomCells;
        Variant* corridorWidthCells;
        Variant* wallHeight;
        Variant* addCeiling;
        Variant* loopProbability;
        Variant* dungeonTheme;
        Variant* adaptToGround;
        Variant* heightOffset;
        Variant* addPillars;
        Variant* pillarSize;
        Variant* floorDatablock;
        Variant* wallDatablock;
        Variant* ceilingDatablock;
        Variant* floorUVTiling;
        Variant* wallUVTiling;
        Variant* generateNow;
        Variant* convertToMesh;

        // ---- Dungeon data --------------------------------------------------------
        std::vector<DungeonRoom> dungeonRooms;
        std::vector<DungeonCorridor> dungeonCorridors;
        Ogre::Vector3 dungeonOrigin;
        bool hasDungeonOrigin;
        bool originPositionSet;

        // ---- Vertex / index data (8 floats per vert: pos.xyz + normal.xyz + uv.xy) --
        // Floor submesh
        std::vector<float> floorVertices;
        std::vector<Ogre::uint32> floorIndices;
        Ogre::uint32 currentFloorVertexIndex;

        // Wall submesh
        std::vector<float> wallVertices;
        std::vector<Ogre::uint32> wallIndices;
        Ogre::uint32 currentWallVertexIndex;

        // Ceiling submesh
        std::vector<float> ceilVertices;
        std::vector<Ogre::uint32> ceilIndices;
        Ogre::uint32 currentCeilVertexIndex;

        // ---- CPU caches for save / load -----------------------------------------
        std::vector<float> cachedFloorVertices;
        std::vector<Ogre::uint32> cachedFloorIndices;
        size_t cachedNumFloorVertices;

        std::vector<float> cachedWallVertices;
        std::vector<Ogre::uint32> cachedWallIndices;
        size_t cachedNumWallVertices;

        std::vector<float> cachedCeilVertices;
        std::vector<Ogre::uint32> cachedCeilIndices;
        size_t cachedNumCeilVertices;

        // ---- Ogre objects --------------------------------------------------------
        Ogre::MeshPtr dungeonMesh;
        Ogre::Item* dungeonItem;
        Ogre::MeshPtr previewMesh;
        Ogre::Item* previewItem;
        Ogre::SceneNode* previewNode;

        // ---- State ---------------------------------------------------------------
        BuildState buildState;
        bool isEditorMeshModifyMode;
        bool isSelected;
        bool isCtrlPressed;
        bool isShiftPressed;

        // ---- Ground detection ---------------------------------------------------
        Ogre::RaySceneQuery* groundQuery;
        Ogre::Plane groundPlane;
    };

} // namespace NOWA

#endif // PROCEDURAL_DUNGEON_COMPONENT_H

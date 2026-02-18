/*
Copyright (c) 2025 Lukas Kalinowski

GPL v3
*/

#ifndef PROCEDURALMAZECOMPONENT_H
#define PROCEDURALMAZECOMPONENT_H

#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "OgrePlugin.h"

namespace Ogre
{
	class VertexBufferPacked;
	class IndexBufferPacked;
	class Item;
}

namespace NOWA
{
	/**
	 * @brief	Component for procedural 3D maze generation.
	 *			Uses Depth-First Search (DFS) backtracker algorithm to generate
	 *			perfect mazes with exactly one solution path.
	 *
	 *			Features:
	 *			- Configurable dimensions (columns x rows)
	 *			- Adjustable wall height and thickness
	 *			- Optional floor and ceiling
	 *			- UV mapping for textures
	 *			- Reproducible mazes via seed
	 *			- Solution path calculation
	 */
	class EXPORTED ProceduralMazeComponent : public GameObjectComponent, public Ogre::Plugin
	{
	public:
		typedef boost::shared_ptr<ProceduralMazeComponent> ProceduralMazeComponentPtr;

		/**
		 * @brief	Represents a cell position in the maze grid
		 */
		struct MazeCell
		{
			int x;
			int y;

			MazeCell() : x(0), y(0)
			{
			}
			MazeCell(int x_, int y_) : x(x_), y(y_)
			{
			}

			bool operator==(const MazeCell& other) const
			{
				return x == other.x && y == other.y;
			}
		};

		/**
		 * @brief	Direction enumeration for maze navigation
		 */
		enum class Direction
		{
			NORTH = 0,	// -Y in grid, +Z in world
			EAST = 1,	// +X
			SOUTH = 2,	// +Y in grid, -Z in world
			WEST = 3	// -X
		};

	public:
		ProceduralMazeComponent();
		virtual ~ProceduralMazeComponent();

		/**
		 * @see		Ogre::Plugin::install
		 */
		virtual void install(const Ogre::NameValuePairList* options) override;

		/**
		 * @see		Ogre::Plugin::initialise
		 */
		virtual void initialise() override
		{
		};

		/**
		 * @see		Ogre::Plugin::shutdown
		 */
		virtual void shutdown() override
		{
		};

		/**
		 * @see		Ogre::Plugin::uninstall
		 */
		virtual void uninstall() override
		{
		};

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
		 * @see		GameObjectComponent::getClassName
		 */
		virtual Ogre::String getClassName(void) const override;

		/**
		 * @see		GameObjectComponent::getParentClassName
		 */
		virtual Ogre::String getParentClassName(void) const override;

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

		// --- Maze Configuration ---

		void setNumColumns(int numColumns);
		int getNumColumns(void) const;

		void setNumRows(int numRows);
		int getNumRows(void) const;

		void setSeed(unsigned int seed);
		unsigned int getSeed(void) const;

		void setCellSize(Ogre::Real cellSize);
		Ogre::Real getCellSize(void) const;

		void setWallHeight(Ogre::Real wallHeight);
		Ogre::Real getWallHeight(void) const;

		void setWallThickness(Ogre::Real wallThickness);
		Ogre::Real getWallThickness(void) const;

		void setCreateFloor(bool createFloor);
		bool getCreateFloor(void) const;

		void setCreateCeiling(bool createCeiling);
		bool getCreateCeiling(void) const;

		void setFloorDatablockName(const Ogre::String& datablockName);
		Ogre::String getFloorDatablockName(void) const;

		void setWallDatablockName(const Ogre::String& datablockName);
		Ogre::String getWallDatablockName(void) const;

		void setCeilingDatablockName(const Ogre::String& datablockName);
		Ogre::String getCeilingDatablockName(void) const;

		// --- Maze Operations ---

		/**
		 * @brief	Regenerates the maze with current settings
		 */
		void regenerateMaze(void);

		/**
		 * @brief	Regenerates maze with a new random seed
		 */
		void regenerateMazeWithNewSeed(void);

		/**
		 * @brief	Gets the solution path from start to end
		 * @return	Vector of world positions along the solution path
		 */
		std::vector<Ogre::Vector3> getSolutionPath(void) const;

		/**
		 * @brief	Gets the start position in world coordinates
		 */
		Ogre::Vector3 getStartPosition(void) const;

		/**
		 * @brief	Gets the end position in world coordinates
		 */
		Ogre::Vector3 getEndPosition(void) const;

		/**
		 * @brief	Converts a grid cell to world position (center of cell)
		 */
		Ogre::Vector3 cellToWorldPosition(int cellX, int cellY) const;

		/**
		 * @brief	Converts world position to grid cell
		 */
		MazeCell worldPositionToCell(const Ogre::Vector3& worldPos) const;

		/**
		 * @brief	Checks if a cell is a passage (walkable)
		 */
		bool isCellPassage(int cellX, int cellY) const;

		/**
		 * @brief	Gets the total number of cells in the maze
		 */
		int getTotalCells(void) const;

		/**
		 * @brief	Gets the maze width in world units
		 */
		Ogre::Real getMazeWidth(void) const;

		/**
		 * @brief	Gets the maze depth in world units
		 */
		Ogre::Real getMazeDepth(void) const;

	public:
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("ProceduralMazeComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "ProceduralMazeComponent";
		}

		static bool canStaticAddComponent(GameObject* gameObject);

		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: Generates a procedural 3D maze mesh. Configure dimensions, wall height, "
				"and materials. Use getSolutionPath() to get the solution.";
		}

		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass);

	public:
		static const Ogre::String AttrNumColumns(void)
		{
			return "Num Columns";
		}
		static const Ogre::String AttrNumRows(void)
		{
			return "Num Rows";
		}
		static const Ogre::String AttrSeed(void)
		{
			return "Seed";
		}
		static const Ogre::String AttrCellSize(void)
		{
			return "Cell Size";
		}
		static const Ogre::String AttrWallHeight(void)
		{
			return "Wall Height";
		}
		static const Ogre::String AttrWallThickness(void)
		{
			return "Wall Thickness";
		}
		static const Ogre::String AttrCreateFloor(void)
		{
			return "Create Floor";
		}
		static const Ogre::String AttrCreateCeiling(void)
		{
			return "Create Ceiling";
		}
		static const Ogre::String AttrFloorDatablock(void)
		{
			return "Floor Datablock";
		}
		static const Ogre::String AttrWallDatablock(void)
		{
			return "Wall Datablock";
		}
		static const Ogre::String AttrCeilingDatablock(void)
		{
			return "Ceiling Datablock";
		}
		static const Ogre::String AttrRegenerate(void)
		{
			return "Regenerate";
		}
        static Ogre::String AttrConvertToMesh(void)
        {
            return "Convert To Mesh";
        }
	protected:
		virtual bool executeAction(const Ogre::String& actionId, NOWA::Variant* attribute) override;
	private:
		/**
		 * @brief	Generates the maze grid using DFS backtracker algorithm
		 */
		void generateMazeGrid(void);

		/**
		 * @brief	Creates the 3D mesh from the maze grid
		 */
		void createMazeMesh(void);

		/**
		 * @brief	Destroys the current maze mesh and cleans up resources
		 */
		void destroyMazeMesh(void);

		/**
		 * @brief	Adds a box (6 faces) to the mesh data
		 */
		void addBox(Ogre::Real minX, Ogre::Real minY, Ogre::Real minZ, Ogre::Real maxX, Ogre::Real maxY, Ogre::Real maxZ, Ogre::Real uScale, Ogre::Real vScale);

		/**
		 * @brief	Adds a floor/ceiling quad to the mesh data
		 */
		void addFloorQuad(Ogre::Real minX, Ogre::Real maxX, Ogre::Real minZ, Ogre::Real maxZ, Ogre::Real y, bool faceUp);

		/**
		 * @brief	Calculates the solution path using wall-following or BFS
		 */
		void calculateSolutionPath(void);

		/**
		 * @brief	Gets a shuffled list of directions for maze generation
		 */
		std::vector<Direction> getShuffledDirections(void);

		/**
		 * @brief	Gets the delta X for a direction
		 */
		int getDeltaX(Direction dir) const;

		/**
		 * @brief	Gets the delta Y for a direction
		 */
		int getDeltaY(Direction dir) const;

		/**
		 * @brief	Random number generator for maze generation
		 */
		unsigned int nextRandom(void);

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
		Ogre::String name;

		// Maze grid data
		// Grid uses: 0 = wall, 1 = passage, 2 = solution path
		std::vector<std::vector<unsigned char>> mazeGrid;
		int gridWidth;		// MaxX + 1 = 2 * numColumns + 1
		int gridHeight;		// MaxY + 1 = 2 * numRows + 1

		// Solution path (in grid coordinates, cells only)
		std::vector<MazeCell> solutionPath;

		// Mesh data
		Ogre::Item* mazeItem;
		Ogre::MeshPtr mazeMesh;

		// Temporary mesh building data
		std::vector<float> vertices;		// Position + Normal + UV
		std::vector<Ogre::uint32> indices;
		Ogre::uint32 currentVertexIndex;

		// Random state
		unsigned int randomState;

		// Attributes
		Variant* numColumns;
		Variant* numRows;
		Variant* seed;
		Variant* cellSize;
		Variant* wallHeight;
		Variant* wallThickness;
		Variant* createFloor;
		Variant* createCeiling;
		Variant* floorDatablock;
		Variant* wallDatablock;
		Variant* ceilingDatablock;
		Variant* regenerate;
        Variant* convertToMesh;
	};

}; // namespace end

#endif
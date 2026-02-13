/*
Copyright (c) 2025 Lukas Kalinowski

GPL v3
*/

#include "NOWAPrecompiled.h"
#include "ProceduralMazeComponent.h"
#include "gameobject/GameObjectFactory.h"
#include "main/AppStateManager.h"
#include "main/EventManager.h"
#include "modules/LuaScriptApi.h"
#include "utilities/XMLConverter.h"

#include "OgreAbiUtils.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsPbs.h"
#include "OgreItem.h"
#include "OgreMesh2.h"
#include "OgreMeshManager2.h"
#include "OgreRenderSystem.h"
#include "OgreRoot.h"
#include "OgreSubMesh2.h"
#include "Vao/OgreVaoManager.h"
#include "Vao/OgreVertexArrayObject.h"

#include <algorithm>
#include <ctime>
#include <stack>

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    ProceduralMazeComponent::ProceduralMazeComponent()
        : GameObjectComponent(), name("ProceduralMazeComponent"), gridWidth(0), gridHeight(0), mazeItem(nullptr), currentVertexIndex(0), randomState(0),
          numColumns(new Variant(ProceduralMazeComponent::AttrNumColumns(), static_cast<int>(10), this->attributes)),
          numRows(new Variant(ProceduralMazeComponent::AttrNumRows(), static_cast<int>(10), this->attributes)),
          seed(new Variant(ProceduralMazeComponent::AttrSeed(), static_cast<unsigned int>(0), this->attributes)), cellSize(new Variant(ProceduralMazeComponent::AttrCellSize(), 2.0f, this->attributes)),
          wallHeight(new Variant(ProceduralMazeComponent::AttrWallHeight(), 3.0f, this->attributes)), wallThickness(new Variant(ProceduralMazeComponent::AttrWallThickness(), 0.2f, this->attributes)),
          createFloor(new Variant(ProceduralMazeComponent::AttrCreateFloor(), true, this->attributes)), createCeiling(new Variant(ProceduralMazeComponent::AttrCreateCeiling(), false, this->attributes)),
          floorDatablock(new Variant(ProceduralMazeComponent::AttrFloorDatablock(), Ogre::String(""), this->attributes)),
          wallDatablock(new Variant(ProceduralMazeComponent::AttrWallDatablock(), Ogre::String(""), this->attributes)),
          ceilingDatablock(new Variant(ProceduralMazeComponent::AttrCeilingDatablock(), Ogre::String(""), this->attributes)),
          regenerate(new Variant(ProceduralMazeComponent::AttrRegenerate(), "Generate", this->attributes))
    {
        this->floorDatablock->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
        this->wallDatablock->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");
        this->ceilingDatablock->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");

        this->regenerate->setDescription("Generate/regenerate the maze.");
        this->regenerate->addUserData(GameObject::AttrActionExec());
        this->regenerate->addUserData(GameObject::AttrActionExecId(), "ProceduralMazeComponent.Regenerate");

        // Setup constraints
        this->numColumns->setConstraints(2, 100);
        this->numRows->setConstraints(2, 100);
        this->cellSize->setConstraints(0.5f, 100.0f);
        this->wallHeight->setConstraints(0.5f, 50.0f);
        this->wallThickness->setConstraints(0.05f, 2.0f);
    }

    ProceduralMazeComponent::~ProceduralMazeComponent(void)
    {
    }

    const Ogre::String& ProceduralMazeComponent::getName() const
    {
        return this->name;
    }

    void ProceduralMazeComponent::install(const Ogre::NameValuePairList* options)
    {
        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<ProceduralMazeComponent>(ProceduralMazeComponent::getStaticClassId(), ProceduralMazeComponent::getStaticClassName());
    }

    void ProceduralMazeComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    bool ProceduralMazeComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralMazeComponent::AttrNumColumns())
        {
            this->numColumns->setValue(XMLConverter::getAttribInt(propertyElement, "data", 10));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralMazeComponent::AttrNumRows())
        {
            this->numRows->setValue(XMLConverter::getAttribInt(propertyElement, "data", 10));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralMazeComponent::AttrSeed())
        {
            this->seed->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data", 0));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralMazeComponent::AttrCellSize())
        {
            this->cellSize->setValue(XMLConverter::getAttribReal(propertyElement, "data", 2.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralMazeComponent::AttrWallHeight())
        {
            this->wallHeight->setValue(XMLConverter::getAttribReal(propertyElement, "data", 3.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralMazeComponent::AttrWallThickness())
        {
            this->wallThickness->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.2f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralMazeComponent::AttrCreateFloor())
        {
            this->createFloor->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralMazeComponent::AttrCreateCeiling())
        {
            this->createCeiling->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralMazeComponent::AttrFloorDatablock())
        {
            this->floorDatablock->setValue(XMLConverter::getAttrib(propertyElement, "data", ""));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralMazeComponent::AttrWallDatablock())
        {
            this->wallDatablock->setValue(XMLConverter::getAttrib(propertyElement, "data", ""));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralMazeComponent::AttrCeilingDatablock())
        {
            this->ceilingDatablock->setValue(XMLConverter::getAttrib(propertyElement, "data", ""));
            propertyElement = propertyElement->next_sibling("property");
        }

        return true;
    }

    GameObjectCompPtr ProceduralMazeComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
#if 0
		ProceduralMazeComponentPtr clonedCompPtr(boost::make_shared<ProceduralMazeComponent>());

		clonedCompPtr->setNumColumns(this->numColumns->getInt());
		clonedCompPtr->setNumRows(this->numRows->getInt());
		clonedCompPtr->setSeed(this->seed->getUInt());
		clonedCompPtr->setCellSize(this->cellSize->getReal());
		clonedCompPtr->setWallHeight(this->wallHeight->getReal());
		clonedCompPtr->setWallThickness(this->wallThickness->getReal());
		clonedCompPtr->setCreateFloor(this->createFloor->getBool());
		clonedCompPtr->setCreateCeiling(this->createCeiling->getBool());
		clonedCompPtr->setFloorDatablockName(this->floorDatablock->getString());
		clonedCompPtr->setWallDatablockName(this->wallDatablock->getString());
		clonedCompPtr->setCeilingDatablockName(this->ceilingDatablock->getString());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));

		return clonedCompPtr;
#else
        return nullptr;
#endif
    }

    bool ProceduralMazeComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralMazeComponent] Init component for game object: " + this->gameObjectPtr->getName());

        // Initialize seed if not set
        if (this->seed->getUInt() == 0)
        {
            this->seed->setValue(static_cast<unsigned int>(std::time(nullptr)));
        }

        // Generate the maze
        this->regenerateMaze();

        return true;
    }

    bool ProceduralMazeComponent::connect(void)
    {
        GameObjectComponent::connect();
        return true;
    }

    bool ProceduralMazeComponent::disconnect(void)
    {
        GameObjectComponent::disconnect();
        return true;
    }

    bool ProceduralMazeComponent::onCloned(void)
    {
        return true;
    }

    void ProceduralMazeComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();
        GraphicsModule::RenderCommand renderCommand = [this]()
        {
            this->destroyMazeMesh();
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralMazeComponent::onRemoveComponent");
    }

    void ProceduralMazeComponent::onOtherComponentRemoved(unsigned int index)
    {
    }

    void ProceduralMazeComponent::onOtherComponentAdded(unsigned int index)
    {
    }

    void ProceduralMazeComponent::update(Ogre::Real dt, bool notSimulating)
    {
        // Nothing to update
    }

    void ProceduralMazeComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (ProceduralMazeComponent::AttrNumColumns() == attribute->getName())
        {
            this->setNumColumns(attribute->getInt());
        }
        else if (ProceduralMazeComponent::AttrNumRows() == attribute->getName())
        {
            this->setNumRows(attribute->getInt());
        }
        else if (ProceduralMazeComponent::AttrSeed() == attribute->getName())
        {
            this->setSeed(attribute->getUInt());
        }
        else if (ProceduralMazeComponent::AttrCellSize() == attribute->getName())
        {
            this->setCellSize(attribute->getReal());
        }
        else if (ProceduralMazeComponent::AttrWallHeight() == attribute->getName())
        {
            this->setWallHeight(attribute->getReal());
        }
        else if (ProceduralMazeComponent::AttrWallThickness() == attribute->getName())
        {
            this->setWallThickness(attribute->getReal());
        }
        else if (ProceduralMazeComponent::AttrCreateFloor() == attribute->getName())
        {
            this->setCreateFloor(attribute->getBool());
        }
        else if (ProceduralMazeComponent::AttrCreateCeiling() == attribute->getName())
        {
            this->setCreateCeiling(attribute->getBool());
        }
        else if (ProceduralMazeComponent::AttrFloorDatablock() == attribute->getName())
        {
            this->setFloorDatablockName(attribute->getString());
        }
        else if (ProceduralMazeComponent::AttrWallDatablock() == attribute->getName())
        {
            this->setWallDatablockName(attribute->getString());
        }
        else if (ProceduralMazeComponent::AttrCeilingDatablock() == attribute->getName())
        {
            this->setCeilingDatablockName(attribute->getString());
        }
    }

    void ProceduralMazeComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        GameObjectComponent::writeXML(propertiesXML, doc);

        xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Num Columns"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->numColumns->getInt())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Num Rows"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->numRows->getInt())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Seed"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->seed->getUInt())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Cell Size"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->cellSize->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Wall Height"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wallHeight->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Wall Thickness"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wallThickness->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Create Floor"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->createFloor->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Create Ceiling"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->createCeiling->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Floor Datablock"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->floorDatablock->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Wall Datablock"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wallDatablock->getString())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Ceiling Datablock"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->ceilingDatablock->getString())));
        propertiesXML->append_node(propertyXML);
    }

    Ogre::String ProceduralMazeComponent::getClassName(void) const
    {
        return "ProceduralMazeComponent";
    }

    Ogre::String ProceduralMazeComponent::getParentClassName(void) const
    {
        return "GameObjectComponent";
    }

    void ProceduralMazeComponent::setNumColumns(int numColumns)
    {
        if (numColumns < 2)
        {
            numColumns = 2;
        }
        if (numColumns > 100)
        {
            numColumns = 100;
        }
        this->numColumns->setValue(numColumns);
    }

    int ProceduralMazeComponent::getNumColumns(void) const
    {
        return this->numColumns->getInt();
    }

    void ProceduralMazeComponent::setNumRows(int numRows)
    {
        if (numRows < 2)
        {
            numRows = 2;
        }
        if (numRows > 100)
        {
            numRows = 100;
        }
        this->numRows->setValue(numRows);
    }

    int ProceduralMazeComponent::getNumRows(void) const
    {
        return this->numRows->getInt();
    }

    void ProceduralMazeComponent::setSeed(unsigned int seed)
    {
        this->seed->setValue(seed);
    }

    unsigned int ProceduralMazeComponent::getSeed(void) const
    {
        return this->seed->getUInt();
    }

    void ProceduralMazeComponent::setCellSize(Ogre::Real cellSize)
    {
        this->cellSize->setValue(cellSize);
    }

    Ogre::Real ProceduralMazeComponent::getCellSize(void) const
    {
        return this->cellSize->getReal();
    }

    void ProceduralMazeComponent::setWallHeight(Ogre::Real wallHeight)
    {
        this->wallHeight->setValue(wallHeight);
    }

    Ogre::Real ProceduralMazeComponent::getWallHeight(void) const
    {
        return this->wallHeight->getReal();
    }

    void ProceduralMazeComponent::setWallThickness(Ogre::Real wallThickness)
    {
        this->wallThickness->setValue(wallThickness);
    }

    Ogre::Real ProceduralMazeComponent::getWallThickness(void) const
    {
        return this->wallThickness->getReal();
    }

    void ProceduralMazeComponent::setCreateFloor(bool createFloor)
    {
        this->createFloor->setValue(createFloor);
    }

    bool ProceduralMazeComponent::getCreateFloor(void) const
    {
        return this->createFloor->getBool();
    }

    void ProceduralMazeComponent::setCreateCeiling(bool createCeiling)
    {
        this->createCeiling->setValue(createCeiling);
    }

    bool ProceduralMazeComponent::getCreateCeiling(void) const
    {
        return this->createCeiling->getBool();
    }

    void ProceduralMazeComponent::setFloorDatablockName(const Ogre::String& datablockName)
    {
        this->floorDatablock->setValue(datablockName);
    }

    Ogre::String ProceduralMazeComponent::getFloorDatablockName(void) const
    {
        return this->floorDatablock->getString();
    }

    void ProceduralMazeComponent::setWallDatablockName(const Ogre::String& datablockName)
    {
        this->wallDatablock->setValue(datablockName);
    }

    Ogre::String ProceduralMazeComponent::getWallDatablockName(void) const
    {
        return this->wallDatablock->getString();
    }

    void ProceduralMazeComponent::setCeilingDatablockName(const Ogre::String& datablockName)
    {
        this->ceilingDatablock->setValue(datablockName);
    }

    Ogre::String ProceduralMazeComponent::getCeilingDatablockName(void) const
    {
        return this->ceilingDatablock->getString();
    }

    void ProceduralMazeComponent::regenerateMaze(void)
    {
        GraphicsModule::RenderCommand renderCommand = [this]()
        {
            this->destroyMazeMesh();
            this->generateMazeGrid();
            this->calculateSolutionPath();
            this->createMazeMesh();

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralMazeComponent] Generated maze " + Ogre::StringConverter::toString(this->numColumns->getInt()) + "x" +
                                                                                   Ogre::StringConverter::toString(this->numRows->getInt()) + " with seed " +
                                                                                   Ogre::StringConverter::toString(this->seed->getUInt()));
        };
        NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "ProceduralMazeComponent::regenerateMaze");
    }

    void ProceduralMazeComponent::regenerateMazeWithNewSeed(void)
    {
        this->seed->setValue(static_cast<unsigned int>(std::time(nullptr) ^ (std::rand() << 16)));
        this->regenerateMaze();
    }

    // ==================== MAZE GENERATION (DFS Backtracker) ====================

    bool ProceduralMazeComponent::executeAction(const Ogre::String& actionId, NOWA::Variant* attribute)
    {
        if ("ProceduralMazeComponent.Regenerate" == actionId)
        {
            this->regenerateMaze();
            return true;
        }
        return false;
    }

    void ProceduralMazeComponent::generateMazeGrid(void)
    {
        int cols = this->numColumns->getInt();
        int rows = this->numRows->getInt();

        // Grid dimensions: walls between cells + outer walls
        // Cells are at odd coordinates (1, 3, 5, ...)
        // Walls are at even coordinates (0, 2, 4, ...)
        this->gridWidth = 2 * cols + 1;
        this->gridHeight = 2 * rows + 1;

        // Initialize grid with all walls (0)
        this->mazeGrid.clear();
        this->mazeGrid.resize(this->gridHeight);
        for (int y = 0; y < this->gridHeight; ++y)
        {
            this->mazeGrid[y].resize(this->gridWidth, 0);
        }

        // Initialize random number generator
        this->randomState = this->seed->getUInt();
        if (this->randomState == 0)
        {
            this->randomState = 1;
        }

        // Start at cell (1, 1) - top-left cell
        int startCellX = 1;
        int startCellY = 1;

        // Stack for DFS backtracking
        std::stack<MazeCell> stack;

        // Mark starting cell as passage
        this->mazeGrid[startCellY][startCellX] = 1;
        stack.push(MazeCell(startCellX, startCellY));

        // DFS maze generation
        while (!stack.empty())
        {
            MazeCell current = stack.top();

            // Get shuffled directions
            std::vector<Direction> directions = this->getShuffledDirections();

            bool foundNeighbor = false;

            for (Direction dir : directions)
            {
                int dx = this->getDeltaX(dir);
                int dy = this->getDeltaY(dir);

                // Neighbor cell is 2 steps away
                int neighborX = current.x + dx * 2;
                int neighborY = current.y + dy * 2;

                // Check bounds
                if (neighborX >= 1 && neighborX < this->gridWidth - 1 && neighborY >= 1 && neighborY < this->gridHeight - 1)
                {
                    // Check if neighbor is unvisited (wall)
                    if (this->mazeGrid[neighborY][neighborX] == 0)
                    {
                        // Carve passage to neighbor
                        // Mark wall between current and neighbor as passage
                        int wallX = current.x + dx;
                        int wallY = current.y + dy;
                        this->mazeGrid[wallY][wallX] = 1;

                        // Mark neighbor as passage
                        this->mazeGrid[neighborY][neighborX] = 1;

                        // Push neighbor to stack
                        stack.push(MazeCell(neighborX, neighborY));

                        foundNeighbor = true;
                        break;
                    }
                }
            }

            // If no unvisited neighbors, backtrack
            if (false == foundNeighbor)
            {
                stack.pop();
            }
        }

        // Create entrance at top (y=0) leading to cell (1,1)
        this->mazeGrid[0][1] = 1;

        // Create exit at bottom (y=gridHeight-1) leading from last cell
        int exitX = this->gridWidth - 2; // Second to last column
        this->mazeGrid[this->gridHeight - 1][exitX] = 1;
    }

    void ProceduralMazeComponent::calculateSolutionPath(void)
    {
        this->solutionPath.clear();

        // Use BFS to find shortest path from entrance to exit
        int startX = 1;
        int startY = 1;
        int endX = this->gridWidth - 2;
        int endY = this->gridHeight - 2;

        // Parent map for path reconstruction
        std::vector<std::vector<MazeCell>> parent(this->gridHeight, std::vector<MazeCell>(this->gridWidth, MazeCell(-1, -1)));

        std::vector<std::vector<bool>> visited(this->gridHeight, std::vector<bool>(this->gridWidth, false));

        std::queue<MazeCell> queue;
        queue.push(MazeCell(startX, startY));
        visited[startY][startX] = true;

        bool found = false;

        while (false == queue.empty() && false == found)
        {
            MazeCell current = queue.front();
            queue.pop();

            if (current.x == endX && current.y == endY)
            {
                found = true;
                break;
            }

            // Check all 4 directions
            const int dx[] = {0, 1, 0, -1};
            const int dy[] = {-1, 0, 1, 0};

            for (int i = 0; i < 4; ++i)
            {
                int nextX = current.x + dx[i];
                int nextY = current.y + dy[i];

                if (nextX >= 0 && nextX < this->gridWidth && nextY >= 0 && nextY < this->gridHeight && false == visited[nextY][nextX] && this->mazeGrid[nextY][nextX] == 1)
                {
                    visited[nextY][nextX] = true;
                    parent[nextY][nextX] = current;
                    queue.push(MazeCell(nextX, nextY));
                }
            }
        }

        // Reconstruct path
        if (true == found)
        {
            MazeCell current(endX, endY);
            while (current.x != -1 && current.y != -1)
            {
                // Only add cells (odd coordinates), not walls
                if (current.x % 2 == 1 && current.y % 2 == 1)
                {
                    this->solutionPath.push_back(current);
                }
                current = parent[current.y][current.x];
            }

            // Reverse to get start-to-end order
            std::reverse(this->solutionPath.begin(), this->solutionPath.end());
        }
    }

    std::vector<ProceduralMazeComponent::Direction> ProceduralMazeComponent::getShuffledDirections(void)
    {
        std::vector<Direction> dirs = {Direction::NORTH, Direction::EAST, Direction::SOUTH, Direction::WEST};

        // Fisher-Yates shuffle using our PRNG
        for (int i = 3; i > 0; --i)
        {
            int j = this->nextRandom() % (i + 1);
            std::swap(dirs[i], dirs[j]);
        }

        return dirs;
    }

    int ProceduralMazeComponent::getDeltaX(Direction dir) const
    {
        switch (dir)
        {
        case Direction::EAST:
            return 1;
        case Direction::WEST:
            return -1;
        default:
            return 0;
        }
    }

    int ProceduralMazeComponent::getDeltaY(Direction dir) const
    {
        switch (dir)
        {
        case Direction::NORTH:
            return -1;
        case Direction::SOUTH:
            return 1;
        default:
            return 0;
        }
    }

    unsigned int ProceduralMazeComponent::nextRandom(void)
    {
        // xorshift32 PRNG
        this->randomState ^= this->randomState << 13;
        this->randomState ^= this->randomState >> 17;
        this->randomState ^= this->randomState << 5;
        return this->randomState;
    }

    // ==================== MESH GENERATION ====================

    void ProceduralMazeComponent::createMazeMesh(void)
    {
        if (nullptr == this->gameObjectPtr)
        {
            return;
        }

        Ogre::Real cellSz = this->cellSize->getReal();
        Ogre::Real wallHt = this->wallHeight->getReal();
        Ogre::Real wallThk = this->wallThickness->getReal();

        // Clear mesh data
        this->vertices.clear();
        this->indices.clear();
        this->currentVertexIndex = 0;

        // Calculate maze world dimensions
        Ogre::Real mazeWidth = this->numColumns->getInt() * cellSz + wallThk;
        Ogre::Real mazeDepth = this->numRows->getInt() * cellSz + wallThk;

        // Create floor if enabled (at y = 0, facing UP)
        if (this->createFloor->getBool())
        {
            this->addFloorQuad(-wallThk / 2, mazeWidth, -wallThk / 2, mazeDepth, 0.0f, true);
        }

        // Create ceiling if enabled (slightly above walls to avoid z-fighting)
        if (this->createCeiling->getBool())
        {
            // Add small offset (0.001) to avoid overlapping with wall tops
            this->addFloorQuad(-wallThk / 2, mazeWidth, -wallThk / 2, mazeDepth, wallHt + 0.001f, false);
        }

        // Iterate through grid and create walls
        for (int gy = 0; gy < this->gridHeight; ++gy)
        {
            for (int gx = 0; gx < this->gridWidth; ++gx)
            {
                // Only process walls (value == 0)
                if (this->mazeGrid[gy][gx] != 0)
                {
                    continue;
                }

                // Calculate world position
                Ogre::Real halfCell = cellSz / 2.0f;

                if (gx % 2 == 0 && gy % 2 == 0)
                {
                    // Wall post (corner pillar)
                    Ogre::Real x = (gx / 2) * cellSz;
                    Ogre::Real z = (gy / 2) * cellSz;

                    this->addBox(x - wallThk / 2, 0, z - wallThk / 2, x + wallThk / 2, wallHt, z + wallThk / 2, 1.0f, wallHt / cellSz);
                }
                else if (gx % 2 == 0 && gy % 2 == 1)
                {
                    // Vertical wall segment (runs along Z axis)
                    Ogre::Real x = (gx / 2) * cellSz;
                    Ogre::Real z = ((gy - 1) / 2) * cellSz + halfCell;

                    this->addBox(x - wallThk / 2, 0, z - halfCell + wallThk / 2, x + wallThk / 2, wallHt, z + halfCell - wallThk / 2, (cellSz - wallThk) / cellSz, wallHt / cellSz);
                }
                else if (gx % 2 == 1 && gy % 2 == 0)
                {
                    // Horizontal wall segment (runs along X axis)
                    Ogre::Real x = ((gx - 1) / 2) * cellSz + halfCell;
                    Ogre::Real z = (gy / 2) * cellSz;

                    this->addBox(x - halfCell + wallThk / 2, 0, z - wallThk / 2, x + halfCell - wallThk / 2, wallHt, z + wallThk / 2, (cellSz - wallThk) / cellSz, wallHt / cellSz);
                }
            }
        }

        // Now create the actual Ogre mesh
        Ogre::Root* root = Ogre::Root::getSingletonPtr();
        Ogre::RenderSystem* renderSystem = root->getRenderSystem();
        Ogre::VaoManager* vaoManager = renderSystem->getVaoManager();

        // Unique mesh name
        Ogre::String meshName = this->gameObjectPtr->getName() + "_Maze_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());

        const Ogre::String groupName = Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME;

        // Remove existing mesh if any
        {
            Ogre::MeshManager& meshMgr = Ogre::MeshManager::getSingleton();
            Ogre::MeshPtr existing = meshMgr.getByName(meshName, groupName);
            if (false == existing.isNull())
            {
                meshMgr.remove(existing->getHandle());
            }
        }

        this->mazeMesh = Ogre::MeshManager::getSingleton().createManual(meshName, groupName);

        Ogre::SubMesh* subMesh = this->mazeMesh->createSubMesh();

        // Check if any datablock needs tangents (has normal maps)
        bool needsTangents = false;
        Ogre::HlmsManager* hlmsManager = Ogre::Root::getSingleton().getHlmsManager();

        auto checkDatablockNeedsTangents = [&](const Ogre::String& dbName) -> bool
        {
            if (dbName.empty())
            {
                return false;
            }
            Ogre::HlmsDatablock* datablock = hlmsManager->getDatablockNoDefault(dbName);
            if (nullptr != datablock)
            {
                Ogre::HlmsPbsDatablock* pbsDatablock = dynamic_cast<Ogre::HlmsPbsDatablock*>(datablock);
                if (pbsDatablock && pbsDatablock->getTexture(Ogre::PBSM_NORMAL))
                {
                    return true;
                }
            }
            return false;
        };

        needsTangents =
            checkDatablockNeedsTangents(this->wallDatablock->getString()) || checkDatablockNeedsTangents(this->floorDatablock->getString()) || checkDatablockNeedsTangents(this->ceilingDatablock->getString());

        // Vertex elements: Position (3) + Normal (3) + Tangent (4, optional) + UV (2)
        Ogre::VertexElement2Vec vertexElements;
        vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
        vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));

        if (true == needsTangents)
        {
            vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT4, Ogre::VES_TANGENT));
        }
        vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));

        // Calculate vertex size
        // Current vertices array has 8 floats per vertex: pos(3) + normal(3) + uv(2)
        // We need to add tangent(4) if needsTangents
        const size_t srcFloatsPerVertex = 8;                      // pos(3) + normal(3) + uv(2)
        const size_t dstFloatsPerVertex = needsTangents ? 12 : 8; // add tangent(4) if needed
        const size_t numVertices = this->currentVertexIndex;

        // Allocate vertex data using OGRE_MALLOC_SIMD
        const size_t vertexDataSize = numVertices * dstFloatsPerVertex * sizeof(float);
        float* vertexData = reinterpret_cast<float*>(OGRE_MALLOC_SIMD(vertexDataSize, Ogre::MEMCATEGORY_GEOMETRY));

        // Copy and potentially add tangents
        for (size_t i = 0; i < numVertices; ++i)
        {
            size_t srcOffset = i * srcFloatsPerVertex;
            size_t dstOffset = i * dstFloatsPerVertex;

            // Position (3 floats)
            vertexData[dstOffset + 0] = this->vertices[srcOffset + 0];
            vertexData[dstOffset + 1] = this->vertices[srcOffset + 1];
            vertexData[dstOffset + 2] = this->vertices[srcOffset + 2];

            // Normal (3 floats)
            vertexData[dstOffset + 3] = this->vertices[srcOffset + 3];
            vertexData[dstOffset + 4] = this->vertices[srcOffset + 4];
            vertexData[dstOffset + 5] = this->vertices[srcOffset + 5];

            size_t nextDst = 6;

            if (needsTangents)
            {
                // Calculate tangent from normal
                // For a simple tangent, we use a perpendicular vector
                Ogre::Vector3 normal(this->vertices[srcOffset + 3], this->vertices[srcOffset + 4], this->vertices[srcOffset + 5]);

                Ogre::Vector3 tangent;
                if (std::abs(normal.y) < 0.9f)
                {
                    // Normal is not pointing straight up/down
                    tangent = Ogre::Vector3::UNIT_Y.crossProduct(normal);
                }
                else
                {
                    // Normal is pointing up or down, use X axis
                    tangent = normal.crossProduct(Ogre::Vector3::UNIT_X);
                }
                tangent.normalise();

                vertexData[dstOffset + nextDst + 0] = tangent.x;
                vertexData[dstOffset + nextDst + 1] = tangent.y;
                vertexData[dstOffset + nextDst + 2] = tangent.z;
                vertexData[dstOffset + nextDst + 3] = 1.0f; // Handedness
                nextDst += 4;
            }

            // UV (2 floats)
            vertexData[dstOffset + nextDst + 0] = this->vertices[srcOffset + 6];
            vertexData[dstOffset + nextDst + 1] = this->vertices[srcOffset + 7];
        }

        // Create vertex buffer
        Ogre::VertexBufferPacked* vertexBuffer = nullptr;
        try
        {
            vertexBuffer = vaoManager->createVertexBuffer(vertexElements, numVertices, Ogre::BT_IMMUTABLE, vertexData, true); // keepAsShadow = true
        }
        catch (Ogre::Exception& e)
        {
            OGRE_FREE_SIMD(vertexData, Ogre::MEMCATEGORY_GEOMETRY);
            throw e;
        }

        // Allocate index data using OGRE_MALLOC_SIMD
        const size_t indexDataSize = this->indices.size() * sizeof(Ogre::uint32);
        Ogre::uint32* indexData = reinterpret_cast<Ogre::uint32*>(OGRE_MALLOC_SIMD(indexDataSize, Ogre::MEMCATEGORY_GEOMETRY));
        memcpy(indexData, this->indices.data(), indexDataSize);

        // Create index buffer
        Ogre::IndexBufferPacked* indexBuffer = nullptr;
        try
        {
            indexBuffer = vaoManager->createIndexBuffer(Ogre::IndexBufferPacked::IT_32BIT, this->indices.size(), Ogre::BT_IMMUTABLE, indexData, true); // keepAsShadow = true
        }
        catch (Ogre::Exception& e)
        {
            OGRE_FREE_SIMD(indexData, Ogre::MEMCATEGORY_GEOMETRY);
            throw e;
        }

        // Create VAO
        Ogre::VertexBufferPackedVec vertexBuffers;
        vertexBuffers.push_back(vertexBuffer);

        Ogre::VertexArrayObject* vao = vaoManager->createVertexArrayObject(vertexBuffers, indexBuffer, Ogre::OT_TRIANGLE_LIST);

        // Use the SAME VAO pointer for both passes to avoid double-free
        subMesh->mVao[Ogre::VpNormal].push_back(vao);
        subMesh->mVao[Ogre::VpShadow].push_back(vao);

        // Set bounds
        Ogre::Aabb bounds;
        bounds.setExtents(Ogre::Vector3(-wallThk / 2, 0, -wallThk / 2), Ogre::Vector3(mazeWidth + wallThk / 2, wallHt, mazeDepth + wallThk / 2));
        this->mazeMesh->_setBounds(bounds, false);
        this->mazeMesh->_setBoundingSphereRadius(bounds.getRadius());

        this->mazeMesh->load();

        // Create item
        this->mazeItem = this->gameObjectPtr->getSceneManager()->createItem(this->mazeMesh, this->gameObjectPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC);

        // Set wall datablock (applied to all for now - single submesh)
        Ogre::String wallDatablock = this->wallDatablock->getString();
        if (false == wallDatablock.empty())
        {
            Ogre::HlmsDatablock* datablock = hlmsManager->getDatablockNoDefault(wallDatablock);
            if (datablock)
            {
                this->mazeItem->setDatablock(datablock);
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[ProceduralMazeComponent] Applied datablock: " + wallDatablock);
            }
            else
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralMazeComponent] Datablock not found: " + wallDatablock);
            }
        }

        // Attach to scene node
        this->gameObjectPtr->getSceneNode()->attachObject(this->mazeItem);
        this->gameObjectPtr->setDoNotDestroyMovableObject(true);

        this->gameObjectPtr->init(this->mazeItem);

        // Clear temporary data
        this->vertices.clear();
        this->indices.clear();

        boost::shared_ptr<NOWA::EventDataGeometryModified> eventDataGeometryModified(new NOWA::EventDataGeometryModified());
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataGeometryModified);

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[ProceduralMazeComponent] Generated maze " + Ogre::StringConverter::toString(this->numColumns->getInt()) + "x" +
                                                                              Ogre::StringConverter::toString(this->numRows->getInt()) + " with seed " + Ogre::StringConverter::toString(this->seed->getUInt()) +
                                                                              (needsTangents ? " (with tangents)" : " (no tangents)"));
    }

    void ProceduralMazeComponent::destroyMazeMesh(void)
    {
        if (this->mazeItem)
        {
            if (this->mazeItem->getParentSceneNode())
            {
                this->mazeItem->getParentSceneNode()->detachObject(this->mazeItem);
            }
            this->gameObjectPtr->getSceneManager()->destroyItem(this->mazeItem);
            this->mazeItem = nullptr;
            this->gameObjectPtr->nullMovableObject();
        }

        if (this->mazeMesh)
        {
            Ogre::MeshManager::getSingleton().remove(this->mazeMesh->getHandle());
            this->mazeMesh.reset();
        }

        this->mazeGrid.clear();
        this->solutionPath.clear();
    }

    void ProceduralMazeComponent::addBox(Ogre::Real minX, Ogre::Real minY, Ogre::Real minZ, Ogre::Real maxX, Ogre::Real maxY, Ogre::Real maxZ, Ogre::Real uScale, Ogre::Real vScale)
    {
        // 6 faces, 4 vertices each = 24 vertices
        // 6 faces, 2 triangles each = 12 triangles = 36 indices

        Ogre::uint32 baseIndex = this->currentVertexIndex;

        // Helper to add a vertex (pos + normal + uv)
        auto addVertex = [this](Ogre::Real x, Ogre::Real y, Ogre::Real z, Ogre::Real nx, Ogre::Real ny, Ogre::Real nz, Ogre::Real u, Ogre::Real v)
        {
            this->vertices.push_back(x);
            this->vertices.push_back(y);
            this->vertices.push_back(z);
            this->vertices.push_back(nx);
            this->vertices.push_back(ny);
            this->vertices.push_back(nz);
            this->vertices.push_back(u);
            this->vertices.push_back(v);
            this->currentVertexIndex++;
        };

        // Helper to add a quad (2 triangles)
        auto addQuadIndices = [this](Ogre::uint32 v0, Ogre::uint32 v1, Ogre::uint32 v2, Ogre::uint32 v3)
        {
            // Triangle 1
            this->indices.push_back(v0);
            this->indices.push_back(v1);
            this->indices.push_back(v2);
            // Triangle 2
            this->indices.push_back(v0);
            this->indices.push_back(v2);
            this->indices.push_back(v3);
        };

        Ogre::Real dx = maxX - minX;
        Ogre::Real dy = maxY - minY;
        Ogre::Real dz = maxZ - minZ;

        // Front face (Z+)
        addVertex(minX, minY, maxZ, 0, 0, 1, 0, 0);
        addVertex(maxX, minY, maxZ, 0, 0, 1, uScale, 0);
        addVertex(maxX, maxY, maxZ, 0, 0, 1, uScale, vScale);
        addVertex(minX, maxY, maxZ, 0, 0, 1, 0, vScale);
        addQuadIndices(baseIndex + 0, baseIndex + 1, baseIndex + 2, baseIndex + 3);

        // Back face (Z-)
        addVertex(maxX, minY, minZ, 0, 0, -1, 0, 0);
        addVertex(minX, minY, minZ, 0, 0, -1, uScale, 0);
        addVertex(minX, maxY, minZ, 0, 0, -1, uScale, vScale);
        addVertex(maxX, maxY, minZ, 0, 0, -1, 0, vScale);
        addQuadIndices(baseIndex + 4, baseIndex + 5, baseIndex + 6, baseIndex + 7);

        // Right face (X+)
        addVertex(maxX, minY, maxZ, 1, 0, 0, 0, 0);
        addVertex(maxX, minY, minZ, 1, 0, 0, uScale, 0);
        addVertex(maxX, maxY, minZ, 1, 0, 0, uScale, vScale);
        addVertex(maxX, maxY, maxZ, 1, 0, 0, 0, vScale);
        addQuadIndices(baseIndex + 8, baseIndex + 9, baseIndex + 10, baseIndex + 11);

        // Left face (X-)
        addVertex(minX, minY, minZ, -1, 0, 0, 0, 0);
        addVertex(minX, minY, maxZ, -1, 0, 0, uScale, 0);
        addVertex(minX, maxY, maxZ, -1, 0, 0, uScale, vScale);
        addVertex(minX, maxY, minZ, -1, 0, 0, 0, vScale);
        addQuadIndices(baseIndex + 12, baseIndex + 13, baseIndex + 14, baseIndex + 15);

        // Top face (Y+)
        addVertex(minX, maxY, maxZ, 0, 1, 0, 0, 0);
        addVertex(maxX, maxY, maxZ, 0, 1, 0, uScale, 0);
        addVertex(maxX, maxY, minZ, 0, 1, 0, uScale, uScale);
        addVertex(minX, maxY, minZ, 0, 1, 0, 0, uScale);
        addQuadIndices(baseIndex + 16, baseIndex + 17, baseIndex + 18, baseIndex + 19);

        // Bottom face (Y-)
        addVertex(minX, minY, minZ, 0, -1, 0, 0, 0);
        addVertex(maxX, minY, minZ, 0, -1, 0, uScale, 0);
        addVertex(maxX, minY, maxZ, 0, -1, 0, uScale, uScale);
        addVertex(minX, minY, maxZ, 0, -1, 0, 0, uScale);
        addQuadIndices(baseIndex + 20, baseIndex + 21, baseIndex + 22, baseIndex + 23);
    }

    void ProceduralMazeComponent::addFloorQuad(Ogre::Real minX, Ogre::Real maxX, Ogre::Real minZ, Ogre::Real maxZ, Ogre::Real y, bool faceUp)
    {
        // faceUp = true: normal points +Y (floor)
        // faceUp = false: normal points -Y (ceiling)

        Ogre::Vector3 normal = faceUp ? Ogre::Vector3::UNIT_Y : Ogre::Vector3::NEGATIVE_UNIT_Y;

        // UV tiling based on size
        Ogre::Real uScale = (maxX - minX);
        Ogre::Real vScale = (maxZ - minZ);

        // Four corners
        // v0 = minX, minZ (bottom-left)
        // v1 = maxX, minZ (bottom-right)
        // v2 = maxX, maxZ (top-right)
        // v3 = minX, maxZ (top-left)

        Ogre::uint32 baseIndex = this->currentVertexIndex;

        // Add 4 vertices (8 floats each: pos + normal + uv)
        // Vertex 0: minX, y, minZ
        this->vertices.push_back(minX);
        this->vertices.push_back(y);
        this->vertices.push_back(minZ);
        this->vertices.push_back(normal.x);
        this->vertices.push_back(normal.y);
        this->vertices.push_back(normal.z);
        this->vertices.push_back(0.0f);
        this->vertices.push_back(0.0f);

        // Vertex 1: maxX, y, minZ
        this->vertices.push_back(maxX);
        this->vertices.push_back(y);
        this->vertices.push_back(minZ);
        this->vertices.push_back(normal.x);
        this->vertices.push_back(normal.y);
        this->vertices.push_back(normal.z);
        this->vertices.push_back(uScale);
        this->vertices.push_back(0.0f);

        // Vertex 2: maxX, y, maxZ
        this->vertices.push_back(maxX);
        this->vertices.push_back(y);
        this->vertices.push_back(maxZ);
        this->vertices.push_back(normal.x);
        this->vertices.push_back(normal.y);
        this->vertices.push_back(normal.z);
        this->vertices.push_back(uScale);
        this->vertices.push_back(vScale);

        // Vertex 3: minX, y, maxZ
        this->vertices.push_back(minX);
        this->vertices.push_back(y);
        this->vertices.push_back(maxZ);
        this->vertices.push_back(normal.x);
        this->vertices.push_back(normal.y);
        this->vertices.push_back(normal.z);
        this->vertices.push_back(0.0f);
        this->vertices.push_back(vScale);

        this->currentVertexIndex += 4;

        // Add indices - winding order depends on which way we're facing
        if (faceUp)
        {
            // Counter-clockwise when viewed from above (+Y)
            // Triangle 1: 0, 2, 1
            this->indices.push_back(baseIndex + 0);
            this->indices.push_back(baseIndex + 2);
            this->indices.push_back(baseIndex + 1);
            // Triangle 2: 0, 3, 2
            this->indices.push_back(baseIndex + 0);
            this->indices.push_back(baseIndex + 3);
            this->indices.push_back(baseIndex + 2);
        }
        else
        {
            // Counter-clockwise when viewed from below (-Y)
            // Triangle 1: 0, 1, 2
            this->indices.push_back(baseIndex + 0);
            this->indices.push_back(baseIndex + 1);
            this->indices.push_back(baseIndex + 2);
            // Triangle 2: 0, 2, 3
            this->indices.push_back(baseIndex + 0);
            this->indices.push_back(baseIndex + 2);
            this->indices.push_back(baseIndex + 3);
        }
    }

    // ==================== UTILITY METHODS ====================

    std::vector<Ogre::Vector3> ProceduralMazeComponent::getSolutionPath(void) const
    {
        std::vector<Ogre::Vector3> worldPath;
        worldPath.reserve(this->solutionPath.size());

        for (const MazeCell& cell : this->solutionPath)
        {
            // Convert grid cell coordinates to world position
            int cellX = (cell.x - 1) / 2; // Convert grid coord to cell index
            int cellY = (cell.y - 1) / 2;
            worldPath.push_back(this->cellToWorldPosition(cellX, cellY));
        }

        return worldPath;
    }

    Ogre::Vector3 ProceduralMazeComponent::getStartPosition(void) const
    {
        return this->cellToWorldPosition(0, 0);
    }

    Ogre::Vector3 ProceduralMazeComponent::getEndPosition(void) const
    {
        return this->cellToWorldPosition(this->numColumns->getInt() - 1, this->numRows->getInt() - 1);
    }

    Ogre::Vector3 ProceduralMazeComponent::cellToWorldPosition(int cellX, int cellY) const
    {
        Ogre::Real cellSz = this->cellSize->getReal();
        Ogre::Real wallThk = this->wallThickness->getReal();

        // Cell center position
        Ogre::Real x = wallThk / 2 + cellX * cellSz + cellSz / 2;
        Ogre::Real z = wallThk / 2 + cellY * cellSz + cellSz / 2;
        Ogre::Real y = this->wallHeight->getReal() / 2; // Mid-height

        // Add GameObject's world position
        if (this->gameObjectPtr && this->gameObjectPtr->getSceneNode())
        {
            Ogre::Vector3 worldOffset = this->gameObjectPtr->getSceneNode()->getPosition();
            return Ogre::Vector3(x, y, z) + worldOffset;
        }

        return Ogre::Vector3(x, y, z);
    }

    ProceduralMazeComponent::MazeCell ProceduralMazeComponent::worldPositionToCell(const Ogre::Vector3& worldPos) const
    {
        Ogre::Vector3 localPos = worldPos;

        // Subtract GameObject's world position
        if (this->gameObjectPtr && this->gameObjectPtr->getSceneNode())
        {
            localPos -= this->gameObjectPtr->getSceneNode()->getPosition();
        }

        Ogre::Real cellSz = this->cellSize->getReal();
        Ogre::Real wallThk = this->wallThickness->getReal();

        int cellX = static_cast<int>((localPos.x - wallThk / 2) / cellSz);
        int cellY = static_cast<int>((localPos.z - wallThk / 2) / cellSz);

        // Clamp to valid range
        cellX = std::max(0, std::min(cellX, this->numColumns->getInt() - 1));
        cellY = std::max(0, std::min(cellY, this->numRows->getInt() - 1));

        return MazeCell(cellX, cellY);
    }

    bool ProceduralMazeComponent::isCellPassage(int cellX, int cellY) const
    {
        // Convert cell coordinates to grid coordinates
        int gx = cellX * 2 + 1;
        int gy = cellY * 2 + 1;

        if (gx >= 0 && gx < this->gridWidth && gy >= 0 && gy < this->gridHeight)
        {
            return this->mazeGrid[gy][gx] != 0;
        }

        return false;
    }

    int ProceduralMazeComponent::getTotalCells(void) const
    {
        return this->numColumns->getInt() * this->numRows->getInt();
    }

    Ogre::Real ProceduralMazeComponent::getMazeWidth(void) const
    {
        return this->numColumns->getInt() * this->cellSize->getReal() + this->wallThickness->getReal();
    }

    Ogre::Real ProceduralMazeComponent::getMazeDepth(void) const
    {
        return this->numRows->getInt() * this->cellSize->getReal() + this->wallThickness->getReal();
    }

    // ==================== STATIC METHODS ====================

    bool ProceduralMazeComponent::canStaticAddComponent(GameObject* gameObject)
    {
        // Cannot be added, its a special component
        return false;
    }

    // ==================== LUA API ====================

    ProceduralMazeComponent* getProceduralMazeComponent(GameObject* gameObject, unsigned int occurrenceIndex)
    {
        return makeStrongPtr<ProceduralMazeComponent>(gameObject->getComponentWithOccurrence<ProceduralMazeComponent>(occurrenceIndex)).get();
    }

    ProceduralMazeComponent* getProceduralMazeComponent(GameObject* gameObject)
    {
        return makeStrongPtr<ProceduralMazeComponent>(gameObject->getComponent<ProceduralMazeComponent>()).get();
    }

    ProceduralMazeComponent* getProceduralMazeComponentFromName(GameObject* gameObject, const Ogre::String& name)
    {
        return makeStrongPtr<ProceduralMazeComponent>(gameObject->getComponentFromName<ProceduralMazeComponent>(name)).get();
    }

    void ProceduralMazeComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
    {
        module(lua)[class_<ProceduralMazeComponent, GameObjectComponent>("ProceduralMazeComponent")
                        .def("setNumColumns", &ProceduralMazeComponent::setNumColumns)
                        .def("getNumColumns", &ProceduralMazeComponent::getNumColumns)
                        .def("setNumRows", &ProceduralMazeComponent::setNumRows)
                        .def("getNumRows", &ProceduralMazeComponent::getNumRows)
                        .def("setSeed", &ProceduralMazeComponent::setSeed)
                        .def("getSeed", &ProceduralMazeComponent::getSeed)
                        .def("setCellSize", &ProceduralMazeComponent::setCellSize)
                        .def("getCellSize", &ProceduralMazeComponent::getCellSize)
                        .def("setWallHeight", &ProceduralMazeComponent::setWallHeight)
                        .def("getWallHeight", &ProceduralMazeComponent::getWallHeight)
                        .def("setWallThickness", &ProceduralMazeComponent::setWallThickness)
                        .def("getWallThickness", &ProceduralMazeComponent::getWallThickness)
                        .def("regenerateMaze", &ProceduralMazeComponent::regenerateMaze)
                        .def("regenerateMazeWithNewSeed", &ProceduralMazeComponent::regenerateMazeWithNewSeed)
                        .def("getStartPosition", &ProceduralMazeComponent::getStartPosition)
                        .def("getEndPosition", &ProceduralMazeComponent::getEndPosition)
                        .def("isCellPassage", &ProceduralMazeComponent::isCellPassage)
                        .def("getTotalCells", &ProceduralMazeComponent::getTotalCells)
                        .def("getMazeWidth", &ProceduralMazeComponent::getMazeWidth)
                        .def("getMazeDepth", &ProceduralMazeComponent::getMazeDepth)];

        LuaScriptApi::getInstance()->addClassToCollection("ProceduralMazeComponent", "class inherits GameObjectComponent", ProceduralMazeComponent::getStaticInfoText());
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralMazeComponent", "void setNumColumns(int columns)", "Sets the number of maze columns (2-100).");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralMazeComponent", "int getNumColumns()", "Gets the number of maze columns.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralMazeComponent", "void setNumRows(int rows)", "Sets the number of maze rows (2-100).");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralMazeComponent", "int getNumRows()", "Gets the number of maze rows.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralMazeComponent", "void setSeed(unsigned int seed)", "Sets the random seed for maze generation.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralMazeComponent", "unsigned int getSeed()", "Gets the random seed.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralMazeComponent", "void setCellSize(float size)", "Sets the cell size in world units.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralMazeComponent", "float getCellSize()", "Gets the cell size.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralMazeComponent", "void setWallHeight(float height)", "Sets the wall height.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralMazeComponent", "float getWallHeight()", "Gets the wall height.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralMazeComponent", "void setWallThickness(float thickness)", "Sets the wall thickness.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralMazeComponent", "float getWallThickness()", "Gets the wall thickness.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralMazeComponent", "void regenerateMaze()", "Regenerates the maze with current settings.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralMazeComponent", "void regenerateMazeWithNewSeed()", "Regenerates the maze with a new random seed.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralMazeComponent", "Vector3 getStartPosition()", "Gets the maze entrance position in world coordinates.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralMazeComponent", "Vector3 getEndPosition()", "Gets the maze exit position in world coordinates.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralMazeComponent", "bool isCellPassage(int cellX, int cellY)", "Checks if a cell is a passage (walkable).");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralMazeComponent", "int getTotalCells()", "Gets the total number of cells in the maze.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralMazeComponent", "float getMazeWidth()", "Gets the maze width in world units.");
        LuaScriptApi::getInstance()->addClassToCollection("ProceduralMazeComponent", "float getMazeDepth()", "Gets the maze depth in world units.");

        gameObjectClass.def("getProceduralMazeComponentFromName", &getProceduralMazeComponentFromName);
        gameObjectClass.def("getProceduralMazeComponent", (ProceduralMazeComponent * (*)(GameObject*)) & getProceduralMazeComponent);
        gameObjectClass.def("getProceduralMazeComponentFromIndex", (ProceduralMazeComponent * (*)(GameObject*, unsigned int)) & getProceduralMazeComponent);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ProceduralMazeComponent getProceduralMazeComponent()", "Gets the component.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ProceduralMazeComponent getProceduralMazeComponentFromIndex(unsigned int occurrenceIndex)", "Gets the component by occurrence index.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ProceduralMazeComponent getProceduralMazeComponentFromName(String name)", "Gets the component from name.");

        gameObjectControllerClass.def("castProceduralMazeComponent", &GameObjectController::cast<ProceduralMazeComponent>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "ProceduralMazeComponent castProceduralMazeComponent(ProceduralMazeComponent other)",
                                                          "Casts an incoming type from function for lua auto completion.");
    }

}; // namespace NOWA
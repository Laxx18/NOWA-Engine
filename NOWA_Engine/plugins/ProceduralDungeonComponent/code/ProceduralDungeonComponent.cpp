/*
Copyright (c) 2026 Lukas Kalinowski

GPL v3
*/

#include "NOWAPrecompiled.h"
#include "ProceduralDungeonComponent.h"
#include "editor/EditorManager.h"
#include "gameobject/GameObjectController.h"
#include "gameobject/GameObjectFactory.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "main/InputDeviceCore.h"
#include "modules/GraphicsModule.h"
#include "utilities/MathHelper.h"
#include "utilities/XMLConverter.h"

#include "OgreAbiUtils.h"
#include "OgreHlmsManager.h"
#include "OgreHlmsPbs.h"
#include "OgreHlmsPbsDatablock.h"
#include "OgreItem.h"
#include "OgreMesh2.h"
#include "OgreMesh2Serializer.h"
#include "OgreMeshManager2.h"
#include "OgreSubMesh2.h"
#include "Vao/OgreVaoManager.h"
#include "Vao/OgreVertexArrayObject.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <limits>
#include <random>
#include <set>
#include <system_error>

namespace
{
    inline Ogre::Real maxValue(const Ogre::Real a, const Ogre::Real b)
    {
        return (a < b) ? b : a;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Quad emitters
    ///////////////////////////////////////////////////////////////////////////////////////////////
    inline void pushVertex(std::vector<float>& verts, Ogre::uint32& idx, const Ogre::Vector3& pos, const Ogre::Vector3& normal, Ogre::Real u, Ogre::Real v)
    {
        verts.push_back(pos.x);
        verts.push_back(pos.y);
        verts.push_back(pos.z);
        verts.push_back(normal.x);
        verts.push_back(normal.y);
        verts.push_back(normal.z);
        verts.push_back(u);
        verts.push_back(v);
        ++idx;
    }

    // Front face: CCW winding  (v0,v1,v2,v3)
    inline void pushQuadIndices(std::vector<Ogre::uint32>& indices, Ogre::uint32 base)
    {
        indices.push_back(base + 0);
        indices.push_back(base + 1);
        indices.push_back(base + 2);
        indices.push_back(base + 0);
        indices.push_back(base + 2);
        indices.push_back(base + 3);
    }

    // Back face: CW winding  (v0,v3,v2,v1) — sits 4 verts after the front face
    inline void pushQuadIndicesBack(std::vector<Ogre::uint32>& indices, Ogre::uint32 base)
    {
        // base+4..+7 are the back-face duplicates with negated normal
        indices.push_back(base + 4);
        indices.push_back(base + 6);
        indices.push_back(base + 5);
        indices.push_back(base + 4);
        indices.push_back(base + 7);
        indices.push_back(base + 6);
    }

    // Emit 8 vertices (4 front + 4 back with negated normal) and 12 indices (2 tris each side)
    void pushDoubleQuad(std::vector<float>& verts, std::vector<Ogre::uint32>& indices, Ogre::uint32& idx, const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& v3, const Ogre::Vector3& normal, Ogre::Real u0,
        Ogre::Real u1, Ogre::Real vv0, Ogre::Real vv1)
    {
        Ogre::uint32 base = idx;
        const Ogre::Vector3 nBack = -normal;

        // Front face (normal outward)
        pushVertex(verts, idx, v0, normal, u0, vv0);
        pushVertex(verts, idx, v1, normal, u1, vv0);
        pushVertex(verts, idx, v2, normal, u1, vv1);
        pushVertex(verts, idx, v3, normal, u0, vv1);
        pushQuadIndices(indices, base);

        // Back face (same positions, negated normal, reversed UV u to mirror correctly)
        pushVertex(verts, idx, v0, nBack, u1, vv0);
        pushVertex(verts, idx, v1, nBack, u0, vv0);
        pushVertex(verts, idx, v2, nBack, u0, vv1);
        pushVertex(verts, idx, v3, nBack, u1, vv1);
        pushQuadIndicesBack(indices, base);
    }

    void pushQuad(std::vector<float>& verts, std::vector<Ogre::uint32>& indices, Ogre::uint32& idx, const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& v3, const Ogre::Vector3& normal, Ogre::Real u0,
        Ogre::Real u1, Ogre::Real vv0, Ogre::Real vv1)
    {
        Ogre::uint32 base = idx;

        const Ogre::Vector3 n = Ogre::Vector3::UNIT_Y;
        Ogre::uint32 b = idx;
        pushVertex(verts, idx, v0, n, u0, vv0);
        pushVertex(verts, idx, v1, n, u1, vv0);
        pushVertex(verts, idx, v2, n, u1, vv1);
        pushVertex(verts, idx, v3, n, u0, vv1);
        pushQuadIndices(indices, base);
    }
}

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Constructor / Destructor
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ProceduralDungeonComponent::ProceduralDungeonComponent() :
        GameObjectComponent(),
        name("ProceduralDungeonComponent"),
        activated(new Variant(ProceduralDungeonComponent::AttrActivated(), true, this->attributes)),
        seed(new Variant(ProceduralDungeonComponent::AttrSeed(), 42, this->attributes)),
        dungeonWidth(new Variant(ProceduralDungeonComponent::AttrDungeonWidth(), 40.0f, this->attributes)),
        dungeonDepth(new Variant(ProceduralDungeonComponent::AttrDungeonDepth(), 40.0f, this->attributes)),
        cellSize(new Variant(ProceduralDungeonComponent::AttrCellSize(), 2.0f, this->attributes)),
        minRoomCells(new Variant(ProceduralDungeonComponent::AttrMinRoomCells(), 3, this->attributes)),
        maxRoomCells(new Variant(ProceduralDungeonComponent::AttrMaxRoomCells(), 8, this->attributes)),
        corridorWidthCells(new Variant(ProceduralDungeonComponent::AttrCorridorWidthCells(), 2, this->attributes)),
        wallHeight(new Variant(ProceduralDungeonComponent::AttrWallHeight(), 3.0f, this->attributes)),
        jitter(new Variant(ProceduralDungeonComponent::AttrJitter(), 0.5f, this->attributes)),
        floorDepth(new Variant(AttrFloorDepth(), 0.02f, this->attributes)),
        addCeiling(new Variant(ProceduralDungeonComponent::AttrAddCeiling(), true, this->attributes)),
        loopProbability(new Variant(ProceduralDungeonComponent::AttrLoopProbability(), 0.15f, this->attributes)),
        dungeonTheme(new Variant(ProceduralDungeonComponent::AttrDungeonTheme(), {"Dungeon", "Cave", "SciFi", "Ice", "Crypt"}, this->attributes)),
        adaptToGround(new Variant(ProceduralDungeonComponent::AttrAdaptToGround(), true, this->attributes)),
        heightOffset(new Variant(ProceduralDungeonComponent::AttrHeightOffset(), 0.05f, this->attributes)),
        addPillars(new Variant(ProceduralDungeonComponent::AttrAddPillars(), true, this->attributes)),
        pillarSize(new Variant(ProceduralDungeonComponent::AttrPillarSize(), 0.3f, this->attributes)),
        floorDatablock(new Variant(ProceduralDungeonComponent::AttrFloorDatablock(), Ogre::String("Stone 4"), this->attributes)),
        wallDatablock(new Variant(ProceduralDungeonComponent::AttrWallDatablock(), Ogre::String("proceduralWall1"), this->attributes)),
        ceilingDatablock(new Variant(ProceduralDungeonComponent::AttrCeilingDatablock(), Ogre::String("proceduralWall1"), this->attributes)),
        floorUVTiling(new Variant(ProceduralDungeonComponent::AttrFloorUVTiling(), Ogre::Vector2(2.0f, 2.0f), this->attributes)),
        wallUVTiling(new Variant(ProceduralDungeonComponent::AttrWallUVTiling(), Ogre::Vector2(1.0f, 1.0f), this->attributes)),
        addWindows(new Variant(ProceduralDungeonComponent::AttrAddWindows(), false, this->attributes)),
        windowProb(new Variant(ProceduralDungeonComponent::AttrWindowProb(), 0.3f, this->attributes)),
        windowWidth(new Variant(ProceduralDungeonComponent::AttrWindowWidth(), 0.5f, this->attributes)),
        windowHeight(new Variant(ProceduralDungeonComponent::AttrWindowHeight(), 0.4f, this->attributes)),
        windowSill(new Variant(ProceduralDungeonComponent::AttrWindowSill(), 0.3f, this->attributes)),
        levelCount(new Variant(AttrLevelCount(), 1, this->attributes)),
        stairsProbability(new Variant(AttrStairsProbability(), 0.15f, this->attributes)),
        addStairs(new Variant(AttrAddStairs(), true, this->attributes)),
        stairHeight(new Variant(AttrStairHeight(), 0.15f, this->attributes)),
        generateNow(new Variant(ProceduralDungeonComponent::AttrGenerateNow(), "Generate Now", this->attributes)),
        convertToMesh(new Variant(ProceduralDungeonComponent::AttrConvertToMesh(), "Convert to Mesh", this->attributes)),
        dungeonOrigin(Ogre::Vector3::ZERO),
        hasDungeonOrigin(false),
        originPositionSet(false),
        currentFloorVertexIndex(0),
        currentWallVertexIndex(0),
        currentCeilVertexIndex(0),
        cachedNumFloorVertices(0),
        cachedNumWallVertices(0),
        cachedNumCeilVertices(0),
        dungeonItem(nullptr),
        previewItem(nullptr),
        previewNode(nullptr),
        buildState(BuildState::IDLE),
        isEditorMeshModifyMode(false),
        isSelected(false),
        isCtrlPressed(false),
        isShiftPressed(false),
        groundQuery(nullptr)
    {
        this->seed->setDescription("Random seed. Same value always produces the same layout.");
        this->dungeonWidth->setDescription("Total width of the dungeon area in world units.");
        this->dungeonDepth->setDescription("Total depth (Z) of the dungeon area in world units.");
        this->cellSize->setDescription("World-unit size of each grid cell. Smaller = finer, more polygons.");
        this->minRoomCells->setDescription("Minimum room dimension in cells.");
        this->maxRoomCells->setDescription("Maximum room dimension in cells.");
        this->corridorWidthCells->setDescription("Width of connecting corridors in cells.");
        this->wallHeight->setDescription("Height of dungeon walls in world units.");
        this->jitter->setDescription("Sets jitter for cave theme. A jitter value of 0.35 gives the current subtle look. 0.6–0.8 gives a very rough dramatic cave. 1.0 is extreme/chaotic.");
        this->floorDepth->setDescription("Visual thickness of the floor slab in world units. Adds a bottom face and border edges so the floor doesn't look paper-thin.");
        this->addCeiling->setDescription("Whether to generate a ceiling mesh (submesh 2).");
        this->loopProbability->setDescription("Probability [0-1] of adding extra loop corridors for non-linear layouts.");
        this->dungeonTheme->setDescription("Visual theme: Dungeon=battlement caps, Cave=variable walls+stalactites, "
                                           "SciFi=floor channel strips, Ice=angled wall bevel, Crypt=tall battlement caps.");
        this->adaptToGround->setDescription("Place dungeon floor at terrain height at the origin.");
        this->heightOffset->setDescription("Additional height above terrain surface (meters).");
        this->addPillars->setDescription("Add box pillars at inner room corners where two walls meet.");
        this->pillarSize->setDescription("Cross-section size of corner pillars in world units.");
        this->floorDatablock->setDescription("HLMS datablock for the floor submesh (submesh 0).");
        this->wallDatablock->setDescription("HLMS datablock for the wall submesh (submesh 1).");
        this->ceilingDatablock->setDescription("HLMS datablock for the ceiling submesh (submesh 2, if enabled).");
        this->floorUVTiling->setDescription("UV tiling scale for floor geometry.");
        this->wallUVTiling->setDescription("UV tiling scale for wall and ceiling geometry.");

        this->generateNow->setDescription("Re-runs BSP generation at the current GameObject position.");
        this->generateNow->addUserData(GameObject::AttrActionExec());
        this->generateNow->addUserData(GameObject::AttrActionExecId(), "ProceduralDungeonComponent.GenerateNow");
        this->generateNow->addUserData(GameObject::AttrActionNeedRefresh());

        this->convertToMesh->setDescription("Converts this procedural dungeon to a static mesh file. "
                                            "This is a ONE-WAY operation! After conversion:\n"
                                            "- The dungeon mesh is exported to the Procedural resource folder\n"
                                            "- This ProceduralDungeonComponent is removed\n"
                                            "- The GameObject will use the static mesh file\n"
                                            "- You can no longer edit the dungeon procedurally\n"
                                            "- The mesh will benefit from Ogre's graphics caching (no FPS drops)\n\n"
                                            "Use this when you're finished designing the dungeon and want optimal performance.");
        this->convertToMesh->addUserData(GameObject::AttrActionExec());
        this->convertToMesh->addUserData(GameObject::AttrActionExecId(), "ProceduralDungeonComponent.ConvertToMesh");

        this->addWindows->setDescription("When enabled, windows are procedurally cut into eligible wall faces. "
                                         "Windows are never placed on corridor walls or bevelled ICE theme walls.");

        this->windowProb->setDescription("Probability (0-1) that any given outer wall face receives a window. "
                                         "0 = no windows even if Add Windows is on; 1 = every eligible face gets one.");

        this->windowWidth->setDescription("Width of the window hole as a fraction (0-1) of the cell width. "
                                          "0.5 means the window occupies half the wall face horizontally.");

        this->windowHeight->setDescription("Height of the window hole as a fraction (0-1) of the wall height. "
                                           "0.4 means the window opening is 40% of the total wall height.");

        this->windowSill->setDescription("Height of the window sill from the floor, expressed as a fraction (0-1) "
                                         "of the wall height. 0.3 places the bottom of the window at 30% of wall height. "
                                         "Ensure sill + Window Height <= 1 to avoid the window exceeding the wall top.");

        this->levelCount->setDescription("Number of dungeon levels stacked vertically. Each level is generated with the same width/depth/seed but offset vertically. Levels connect via staircases.");
        this->stairsProbability->setDescription("Probability (0-1) that a given ROOM cell gets an upward staircase. Only one staircase per level is placed — the first cell that passes the check.");
        this->addStairs->setDescription("If connections between floor levels are created, this controls whether to add stairs geometry, or maybe the designer has its own stairs.");
        this->stairHeight->setDescription("Height of each stair step as a fraction of wall height. Total stair run = wallHeight / stairHeight steps.");
    }

    ProceduralDungeonComponent::~ProceduralDungeonComponent()
    {
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Ogre::Plugin
    ///////////////////////////////////////////////////////////////////////////////////////////////

    void ProceduralDungeonComponent::install(const Ogre::NameValuePairList* options)
    {
        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<ProceduralDungeonComponent>(ProceduralDungeonComponent::getStaticClassId(), ProceduralDungeonComponent::getStaticClassName());
    }

    void ProceduralDungeonComponent::initialise()
    {
    }
    void ProceduralDungeonComponent::shutdown()
    {
    }
    void ProceduralDungeonComponent::uninstall()
    {
    }

    const Ogre::String& ProceduralDungeonComponent::getName() const
    {
        return this->name;
    }

    void ProceduralDungeonComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // init / postInit / connect / disconnect / clone
    ///////////////////////////////////////////////////////////////////////////////////////////////

    bool ProceduralDungeonComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDungeonComponent::AttrActivated())
        {
            this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDungeonComponent::AttrSeed())
        {
            this->seed->setValue(XMLConverter::getAttribInt(propertyElement, "data", 42));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDungeonComponent::AttrDungeonWidth())
        {
            this->dungeonWidth->setValue(XMLConverter::getAttribReal(propertyElement, "data", 40.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDungeonComponent::AttrDungeonDepth())
        {
            this->dungeonDepth->setValue(XMLConverter::getAttribReal(propertyElement, "data", 40.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDungeonComponent::AttrCellSize())
        {
            this->cellSize->setValue(XMLConverter::getAttribReal(propertyElement, "data", 2.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDungeonComponent::AttrMinRoomCells())
        {
            this->minRoomCells->setValue(XMLConverter::getAttribInt(propertyElement, "data", 3));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDungeonComponent::AttrMaxRoomCells())
        {
            this->maxRoomCells->setValue(XMLConverter::getAttribInt(propertyElement, "data", 8));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDungeonComponent::AttrCorridorWidthCells())
        {
            this->corridorWidthCells->setValue(XMLConverter::getAttribInt(propertyElement, "data", 2));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDungeonComponent::AttrWallHeight())
        {
            this->wallHeight->setValue(XMLConverter::getAttribReal(propertyElement, "data", 3.0f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDungeonComponent::AttrJitter())
        {
            this->jitter->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.5f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDungeonComponent::AttrFloorDepth())
        {
            this->floorDepth->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.02f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDungeonComponent::AttrAddCeiling())
        {
            this->addCeiling->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDungeonComponent::AttrLoopProbability())
        {
            this->loopProbability->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.15f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDungeonComponent::AttrDungeonTheme())
        {
            this->dungeonTheme->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", "Dungeon"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDungeonComponent::AttrAdaptToGround())
        {
            this->adaptToGround->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDungeonComponent::AttrHeightOffset())
        {
            this->heightOffset->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.05f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDungeonComponent::AttrAddPillars())
        {
            this->addPillars->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDungeonComponent::AttrPillarSize())
        {
            this->pillarSize->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.3f));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDungeonComponent::AttrFloorDatablock())
        {
            this->floorDatablock->setValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDungeonComponent::AttrWallDatablock())
        {
            this->wallDatablock->setValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDungeonComponent::AttrCeilingDatablock())
        {
            this->ceilingDatablock->setValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDungeonComponent::AttrFloorUVTiling())
        {
            this->floorUVTiling->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDungeonComponent::AttrWallUVTiling())
        {
            this->wallUVTiling->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDungeonComponent::AttrAddWindows())
        {
            this->addWindows->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDungeonComponent::AttrWindowProb())
        {
            this->windowProb->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDungeonComponent::AttrWindowWidth())
        {
            this->windowWidth->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDungeonComponent::AttrWindowHeight())
        {
            this->windowHeight->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDungeonComponent::AttrWindowSill())
        {
            this->windowSill->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDungeonComponent::AttrLevelCount())
        {
            this->levelCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDungeonComponent::AttrStairsProbability())
        {
            this->stairsProbability->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDungeonComponent::AttrAddStairs())
        {
            this->addStairs->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == ProceduralDungeonComponent::AttrStairHeight())
        {
            this->stairHeight->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        return true;
    }

    bool ProceduralDungeonComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralDungeonComponent] postInit for: " + this->gameObjectPtr->getName());

        AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &ProceduralDungeonComponent::handleMeshModifyMode), NOWA::EventDataEditorMode::getStaticEventType());
        AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &ProceduralDungeonComponent::handleGameObjectSelected), NOWA::EventDataGameObjectSelected::getStaticEventType());
        AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &ProceduralDungeonComponent::handleComponentManuallyDeleted), EventDataDeleteComponent::getStaticEventType());

        this->groundQuery = this->gameObjectPtr->getSceneManager()->createRayQuery(Ogre::Ray(), GameObjectController::ALL_CATEGORIES_ID);
        this->groundQuery->setSortByDistance(true);
        this->groundPlane = Ogre::Plane(Ogre::Vector3::UNIT_Y, 0.0f);

        this->previewNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();

        /*if (this->loadDungeonDataFromFile() && !this->dungeonRooms.empty())
        {
            this->createDungeonMesh();
        }*/

        this->generateDungeon();

        return true;
    }

    bool ProceduralDungeonComponent::connect(void)
    {
        return true;
    }

    bool ProceduralDungeonComponent::disconnect(void)
    {
        this->removeInputListener();
        this->buildState = BuildState::IDLE;
        this->destroyPreviewMesh();
        return true;
    }

    bool ProceduralDungeonComponent::onCloned(void)
    {
        return true;
    }
    void ProceduralDungeonComponent::onAddComponent(void)
    {
    }

    void ProceduralDungeonComponent::onRemoveComponent(void)
    {
        AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ProceduralDungeonComponent::handleMeshModifyMode), NOWA::EventDataEditorMode::getStaticEventType());
        AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ProceduralDungeonComponent::handleGameObjectSelected), NOWA::EventDataGameObjectSelected::getStaticEventType());
        AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ProceduralDungeonComponent::handleComponentManuallyDeleted), EventDataDeleteComponent::getStaticEventType());

        this->removeInputListener();
        this->destroyPreviewMesh();
        this->destroyDungeonMesh();

        if (this->groundQuery)
        {
            this->gameObjectPtr->getSceneManager()->destroyQuery(this->groundQuery);
            this->groundQuery = nullptr;
        }
        if (this->previewNode)
        {
            this->gameObjectPtr->getSceneManager()->destroySceneNode(this->previewNode);
            this->previewNode = nullptr;
        }
    }

    void ProceduralDungeonComponent::onOtherComponentRemoved(unsigned int index)
    {
    }
    void ProceduralDungeonComponent::onOtherComponentAdded(unsigned int index)
    {
    }

    GameObjectCompPtr ProceduralDungeonComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        ProceduralDungeonComponentPtr clonedCompPtr(boost::make_shared<ProceduralDungeonComponent>());
        clonedCompPtr->setActivated(this->activated->getBool());
        clonedCompPtr->setSeed(this->seed->getInt());
        clonedCompPtr->setDungeonWidth(this->dungeonWidth->getReal());
        clonedCompPtr->setDungeonDepth(this->dungeonDepth->getReal());
        clonedCompPtr->setCellSize(this->cellSize->getReal());
        clonedCompPtr->setMinRoomCells(this->minRoomCells->getInt());
        clonedCompPtr->setMaxRoomCells(this->maxRoomCells->getInt());
        clonedCompPtr->setCorridorWidthCells(this->corridorWidthCells->getInt());
        clonedCompPtr->setWallHeight(this->wallHeight->getReal());
        clonedCompPtr->setJitter(this->jitter->getReal());
        clonedCompPtr->setFloorDepth(this->floorDepth->getReal());
        clonedCompPtr->setAddCeiling(this->addCeiling->getBool());
        clonedCompPtr->setLoopProbability(this->loopProbability->getReal());
        clonedCompPtr->setDungeonTheme(this->dungeonTheme->getListSelectedValue());
        clonedCompPtr->setAdaptToGround(this->adaptToGround->getBool());
        clonedCompPtr->setHeightOffset(this->heightOffset->getReal());
        clonedCompPtr->setAddPillars(this->addPillars->getBool());
        clonedCompPtr->setPillarSize(this->pillarSize->getReal());
        clonedCompPtr->setFloorDatablock(this->floorDatablock->getString());
        clonedCompPtr->setWallDatablock(this->wallDatablock->getString());
        clonedCompPtr->setCeilingDatablock(this->ceilingDatablock->getString());
        clonedCompPtr->setFloorUVTiling(this->floorUVTiling->getVector2());
        clonedCompPtr->setWallUVTiling(this->wallUVTiling->getVector2());
        clonedCompPtr->setAddWindows(this->addWindows->getBool());
        clonedCompPtr->setWindowProb(this->windowProb->getReal());
        clonedCompPtr->setWindowWidth(this->windowWidth->getReal());
        clonedCompPtr->setWindowHeight(this->windowHeight->getReal());
        clonedCompPtr->setWindowSill(this->windowSill->getReal());
        clonedCompPtr->setLevelCount(this->levelCount->getUInt());
        clonedCompPtr->setStairsProbability(this->stairsProbability->getReal());
        clonedCompPtr->setAddStairs(this->addStairs->getBool());
        clonedCompPtr->setStairHeight(this->stairHeight->getReal());
               
        clonedGameObjectPtr->addComponent(clonedCompPtr);
        clonedCompPtr->setOwner(clonedGameObjectPtr);
        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
        return clonedCompPtr;
    }

    void ProceduralDungeonComponent::update(Ogre::Real dt, bool notSimulating)
    {
    }

    void ProceduralDungeonComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        const Ogre::String& name = attribute->getName();

        if (AttrSeed() == name)
        {
            this->setSeed(attribute->getInt());
        }
        else if (AttrDungeonWidth() == name)
        {
            this->setDungeonWidth(attribute->getReal());
        }
        else if (AttrDungeonDepth() == name)
        {
            this->setDungeonDepth(attribute->getReal());
        }
        else if (AttrCellSize() == name)
        {
            this->setCellSize(attribute->getReal());
        }
        else if (AttrMinRoomCells() == name)
        {
            this->setMinRoomCells(attribute->getInt());
        }
        else if (AttrMaxRoomCells() == name)
        {
            this->setMaxRoomCells(attribute->getInt());
        }
        else if (AttrCorridorWidthCells() == name)
        {
            this->setCorridorWidthCells(attribute->getInt());
        }
        else if (AttrLoopProbability() == name)
        {
            this->setLoopProbability(attribute->getReal());
        }
        else if (AttrDungeonTheme() == name)
        {
            this->setDungeonTheme(attribute->getListSelectedValue());
        }
        else if (AttrAdaptToGround() == name)
        {
            this->setAdaptToGround(attribute->getBool());
        }
        else if (AttrHeightOffset() == name)
        {
            this->setHeightOffset(attribute->getReal());
        }
        else if (AttrAddPillars() == name)
        {
            this->setAddPillars(attribute->getBool());
        }
        else if (AttrPillarSize() == name)
        {
            this->setPillarSize(attribute->getReal());
        }
        else if (AttrWallHeight() == name)
        {
            this->setWallHeight(attribute->getReal());
        }
        else if (AttrJitter() == name)
        {
            this->setJitter(attribute->getReal());
        }
        else if (AttrFloorDepth() == name)
        {
            this->setFloorDepth(attribute->getReal());
        }
        else if (AttrAddCeiling() == name)
        {
            this->setAddCeiling(attribute->getBool());
        }
        else if (AttrFloorDatablock() == name)
        {
            this->setFloorDatablock(attribute->getString());
        }
        else if (AttrWallDatablock() == name)
        {
            this->setWallDatablock(attribute->getString());
        }
        else if (AttrCeilingDatablock() == name)
        {
            this->setCeilingDatablock(attribute->getString());
        }
        else if (AttrFloorUVTiling() == name)
        {
            this->setFloorUVTiling(attribute->getVector2());
        }
        else if (AttrWallUVTiling() == name)
        {
            this->setWallUVTiling(attribute->getVector2());
        }
        else if (AttrAddWindows() == name)
        {
            this->setAddWindows(attribute->getBool());
        }
        else if (AttrWindowProb() == name)
        {
            this->setWindowProb(attribute->getReal());
        }
        else if (AttrWindowWidth() == name)
        {
            this->setWindowWidth(attribute->getReal());
        }
        else if (AttrWindowHeight() == name)
        {
            this->setWindowHeight(attribute->getReal());
        }
        else if (AttrWindowSill() == name)
        {
            this->setWindowSill(attribute->getReal());
        }
        else if (AttrLevelCount() == name)
        {
            this->setLevelCount(attribute->getUInt());
        }
        else if (AttrStairsProbability() == name)
        {
            this->setStairsProbability(attribute->getReal());
        }
        else if (AttrAddStairs() == name)
        {
            this->setAddStairs(attribute->getBool());
        }
        else if (AttrStairHeight() == name)
        {
            this->setStairHeight(attribute->getReal());
        }
    }

    void ProceduralDungeonComponent::writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc)
    {
        GameObjectComponent::writeXML(propertiesXML, doc);

        rapidxml::xml_node<>* propertyXML = nullptr;

        // Activated (bool)
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralDungeonComponent::AttrActivated())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
        propertiesXML->append_node(propertyXML);

        // Seed (int)
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralDungeonComponent::AttrSeed())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->seed->getInt())));
        propertiesXML->append_node(propertyXML);

        // Dungeon Width (real)
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralDungeonComponent::AttrDungeonWidth())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->dungeonWidth->getReal())));
        propertiesXML->append_node(propertyXML);

        // Dungeon Depth (real)
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralDungeonComponent::AttrDungeonDepth())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->dungeonDepth->getReal())));
        propertiesXML->append_node(propertyXML);

        // Cell Size
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralDungeonComponent::AttrCellSize())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->cellSize->getReal())));
        propertiesXML->append_node(propertyXML);

        // Min Room Cells
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralDungeonComponent::AttrMinRoomCells())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->minRoomCells->getInt())));
        propertiesXML->append_node(propertyXML);

        // Max Room Cells
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralDungeonComponent::AttrMaxRoomCells())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxRoomCells->getInt())));
        propertiesXML->append_node(propertyXML);

        // Corridor Width Cells
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralDungeonComponent::AttrCorridorWidthCells())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->corridorWidthCells->getInt())));
        propertiesXML->append_node(propertyXML);

        // Wall Height
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralDungeonComponent::AttrWallHeight())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wallHeight->getReal())));
        propertiesXML->append_node(propertyXML);

        // Jitter
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralDungeonComponent::AttrJitter())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->jitter->getReal())));
        propertiesXML->append_node(propertyXML);

        // Floor Depth
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralDungeonComponent::AttrFloorDepth())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->floorDepth->getReal())));
        propertiesXML->append_node(propertyXML);

        // Add Ceiling (bool)
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralDungeonComponent::AttrAddCeiling())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->addCeiling->getBool())));
        propertiesXML->append_node(propertyXML);

        // Loop Probability
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralDungeonComponent::AttrLoopProbability())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->loopProbability->getReal())));
        propertiesXML->append_node(propertyXML);

        // Dungeon Theme (string list)
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralDungeonComponent::AttrDungeonTheme())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->dungeonTheme->getListSelectedValue())));
        propertiesXML->append_node(propertyXML);

        // Adapt To Ground
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralDungeonComponent::AttrAdaptToGround())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->adaptToGround->getBool())));
        propertiesXML->append_node(propertyXML);

        // Height Offset
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralDungeonComponent::AttrHeightOffset())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->heightOffset->getReal())));
        propertiesXML->append_node(propertyXML);

        // Add Pillars
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralDungeonComponent::AttrAddPillars())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->addPillars->getBool())));
        propertiesXML->append_node(propertyXML);

        // Pillar Size
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralDungeonComponent::AttrPillarSize())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->pillarSize->getReal())));
        propertiesXML->append_node(propertyXML);

        // Floor Datablock
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralDungeonComponent::AttrFloorDatablock())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->floorDatablock->getString())));
        propertiesXML->append_node(propertyXML);

        // Wall Datablock
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralDungeonComponent::AttrWallDatablock())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wallDatablock->getString())));
        propertiesXML->append_node(propertyXML);

        // Ceiling Datablock
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralDungeonComponent::AttrCeilingDatablock())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->ceilingDatablock->getString())));
        propertiesXML->append_node(propertyXML);

        // Floor UV Tiling (Vector2)
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralDungeonComponent::AttrFloorUVTiling())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->floorUVTiling->getVector2())));
        propertiesXML->append_node(propertyXML);

        // Wall UV Tiling (Vector2)
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralDungeonComponent::AttrWallUVTiling())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wallUVTiling->getVector2())));
        propertiesXML->append_node(propertyXML);

        // Add Windows (bool, type "12")
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralDungeonComponent::AttrAddWindows())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->addWindows->getBool())));
        propertiesXML->append_node(propertyXML);

        // Window Probability (Real, type "6")
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralDungeonComponent::AttrWindowProb())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->windowProb->getReal())));
        propertiesXML->append_node(propertyXML);

        // Window Width (Real, type "6")
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralDungeonComponent::AttrWindowWidth())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->windowWidth->getReal())));
        propertiesXML->append_node(propertyXML);

        // Window Height (Real, type "6")
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralDungeonComponent::AttrWindowHeight())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->windowHeight->getReal())));
        propertiesXML->append_node(propertyXML);

        // Window Sill Height (Real, type "6")
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralDungeonComponent::AttrWindowSill())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->windowSill->getReal())));
        propertiesXML->append_node(propertyXML);

        // Level Count (Real, type "2")
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralDungeonComponent::AttrLevelCount())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->levelCount->getUInt())));
        propertiesXML->append_node(propertyXML);

        // Stairs Probability (Real, type "6")
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralDungeonComponent::AttrStairsProbability())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->stairsProbability->getReal())));
        propertiesXML->append_node(propertyXML);

        // Add Stairs (bool, type "12")
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralDungeonComponent::AttrAddStairs())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->addStairs->getBool())));
        propertiesXML->append_node(propertyXML);

        // Stair Height (Real, type "6")
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, ProceduralDungeonComponent::AttrStairHeight())));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->stairHeight->getReal())));
        propertiesXML->append_node(propertyXML);

        this->saveDungeonDataToFile();
    }

    bool ProceduralDungeonComponent::canStaticAddComponent(GameObject* gameObject)
    {
        return false;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Input
    ///////////////////////////////////////////////////////////////////////////////////////////////

    bool ProceduralDungeonComponent::mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
    {
        if (!this->activated->getBool() || id != OIS::MB_Left)
        {
            return true; // not handled -> bubble
        }

        // Check for MyGUI focus FIRST, before handling
        if (nullptr != NOWA::GraphicsModule::getInstance()->getMyGUIFocusWidget())
        {
            return true;
        }

        Ogre::Camera* camera = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
        if (!camera)
        {
            return true;
        }

        // Exclude own mesh from raycast
        std::vector<Ogre::MovableObject*> excludeMovableObjects;
        if (this->dungeonItem)
        {
            excludeMovableObjects.push_back(this->dungeonItem);
        }
        if (this->previewItem)
        {
            excludeMovableObjects.push_back(this->previewItem);
        }

        const OIS::MouseState& ms = NOWA::InputDeviceCore::getSingletonPtr()->getMouse()->getMouseState();
        Ogre::Vector3 dummyHit;
        Ogre::MovableObject* hitObj = nullptr;
        Ogre::Real hitDist = 0.0f;
        Ogre::Vector3 hitNormal;
        bool hitFound = MathHelper::getInstance()->getRaycastFromPoint(ms.X.abs, ms.Y.abs, camera, Core::getSingletonPtr()->getOgreRenderWindow(), this->groundQuery, dummyHit, (size_t&)hitObj, hitDist, hitNormal, &excludeMovableObjects, false);

        // Don't react if we hit our own mesh
        if (hitFound && (hitObj == this->dungeonItem || hitObj == this->previewItem))
        {
            return true;
        }

        Ogre::Real screenX = 0.0f, screenY = 0.0f;
        MathHelper::getInstance()->mouseToViewPort(evt.state.X.abs, evt.state.Y.abs, screenX, screenY, Core::getSingletonPtr()->getOgreRenderWindow());

        Ogre::Vector3 hitPos;
        if (this->raycastGround(screenX, screenY, hitPos))
        {
            hitPos = this->snapToGridFunc(hitPos);

            // Shift: increment seed for a new layout at same position
            if (this->isShiftPressed)
            {
                this->seed->setValue(this->seed->getInt() + 1);
            }

            // boost::shared_ptr<NOWA::EventDataCommandTransactionBegin> beginEvt(new NOWA::EventDataCommandTransactionBegin("Place Dungeon"));
            // NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(beginEvt);

            std::vector<unsigned char> oldData = this->getDungeonData();

            this->generateDungeonAtPosition(hitPos);

            std::vector<unsigned char> newData = this->getDungeonData();

            // boost::shared_ptr<EventDataDungeonModifyEnd> modEvt(new EventDataDungeonModifyEnd(oldData, newData, this->gameObjectPtr->getId()));
            // NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(modEvt);

            // boost::shared_ptr<NOWA::EventDataCommandTransactionEnd> endEvt(new NOWA::EventDataCommandTransactionEnd());
            // NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(endEvt);

            this->buildState = BuildState::PLACING; // stay in placing so user can click again
        }

        return false; // handled -> do not bubble
    }

    bool ProceduralDungeonComponent::mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
    {
        if (!this->activated->getBool())
        {
            return true;
        }

        if (id == OIS::MB_Right)
        {
            this->destroyPreviewMesh();
            this->buildState = BuildState::IDLE;
            this->removeInputListener();
            return false; // handled
        }

        return true; // not handled -> bubble
    }

    bool ProceduralDungeonComponent::mouseMoved(const OIS::MouseEvent& evt)
    {
        if (!this->activated->getBool() || this->buildState != BuildState::PLACING)
        {
            return true;
        }

        Ogre::Real screenX = 0.0f, screenY = 0.0f;
        MathHelper::getInstance()->mouseToViewPort(evt.state.X.abs, evt.state.Y.abs, screenX, screenY, Core::getSingletonPtr()->getOgreRenderWindow());
        Ogre::Vector3 hitPos;
        if (this->raycastGround(screenX, screenY, hitPos))
        {
            this->updatePreviewMesh(this->snapToGridFunc(hitPos));
        }

        return true;
    }

    bool ProceduralDungeonComponent::keyPressed(const OIS::KeyEvent& evt)
    {
        if (!this->activated->getBool())
        {
            return true;
        }

        if (evt.key == OIS::KC_LSHIFT || evt.key == OIS::KC_RSHIFT)
        {
            this->isShiftPressed = true;
            return false;
        }
        if (evt.key == OIS::KC_LCONTROL || evt.key == OIS::KC_RCONTROL)
        {
            this->isCtrlPressed = true;
            return false;
        }
        if (evt.key == OIS::KC_ESCAPE)
        {
            this->destroyPreviewMesh();
            this->buildState = BuildState::IDLE;
            this->removeInputListener();
            return false; // consume — stop DesignState ESC
        }

        return true;
    }

    bool ProceduralDungeonComponent::keyReleased(const OIS::KeyEvent& evt)
    {
        if (!this->activated->getBool())
        {
            return true;
        }

        if (evt.key == OIS::KC_LSHIFT || evt.key == OIS::KC_RSHIFT)
        {
            this->isShiftPressed = false;
        }
        if (evt.key == OIS::KC_LCONTROL || evt.key == OIS::KC_RCONTROL)
        {
            this->isCtrlPressed = false;
        }

        return false; // handled
    }

    bool ProceduralDungeonComponent::executeAction(const Ogre::String& actionId, NOWA::Variant* attribute)
    {
        if ("ProceduralDungeonComponent.GenerateNow" == actionId)
        {
            std::vector<unsigned char> oldData = this->getDungeonData();
            boost::shared_ptr<NOWA::EventDataCommandTransactionBegin> b(new NOWA::EventDataCommandTransactionBegin("Generate Dungeon"));
            NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(b);
            this->generateDungeon();
            std::vector<unsigned char> newData = this->getDungeonData();
            // TODO: Add event
            // boost::shared_ptr<EventDataDungeonModifyEnd> m(new EventDataDungeonModifyEnd(oldData, newData, this->gameObjectPtr->getId()));
            // NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(m);
            boost::shared_ptr<NOWA::EventDataCommandTransactionEnd> e(new NOWA::EventDataCommandTransactionEnd());
            NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(e);
            return true;
        }
        if ("ProceduralDungeonComponent.ConvertToMesh" == actionId)
        {
            return this->convertToMeshApply();
        }
        return GameObjectComponent::executeAction(actionId, attribute);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // BSP
    ///////////////////////////////////////////////////////////////////////////////////////////////

    void ProceduralDungeonComponent::splitBSP(BSPNode* node, std::mt19937& rng, int depth)
    {
        const int MIN_REGION = this->minRoomCells->getInt() + 2;
        const int MAX_DEPTH = 8;
        if (depth >= MAX_DEPTH)
        {
            return;
        }

        bool canH = (node->rows >= MIN_REGION * 2);
        bool canV = (node->cols >= MIN_REGION * 2);
        if (!canH && !canV)
        {
            return;
        }

        bool doH;
        if (canH && canV)
        {
            doH = (node->rows > node->cols) ? (std::uniform_int_distribution<int>(0, 9)(rng) < 6) : (std::uniform_int_distribution<int>(0, 9)(rng) < 4);
        }
        else
        {
            doH = canH;
        }

        node->splitHorizontal = doH;
        node->left = std::make_unique<BSPNode>();
        node->right = std::make_unique<BSPNode>();

        if (doH)
        {
            int split = std::uniform_int_distribution<int>(MIN_REGION, node->rows - MIN_REGION)(rng);
            node->left->col = node->col;
            node->left->row = node->row;
            node->left->cols = node->cols;
            node->left->rows = split;
            node->right->col = node->col;
            node->right->row = node->row + split;
            node->right->cols = node->cols;
            node->right->rows = node->rows - split;
        }
        else
        {
            int split = std::uniform_int_distribution<int>(MIN_REGION, node->cols - MIN_REGION)(rng);
            node->left->col = node->col;
            node->left->row = node->row;
            node->left->cols = split;
            node->left->rows = node->rows;
            node->right->col = node->col + split;
            node->right->row = node->row;
            node->right->cols = node->cols - split;
            node->right->rows = node->rows;
        }

        this->splitBSP(node->left.get(), rng, depth + 1);
        this->splitBSP(node->right.get(), rng, depth + 1);
    }

    void ProceduralDungeonComponent::placeRoomInLeaf(BSPNode* node, std::vector<DungeonRoom>& rooms, std::mt19937& rng)
    {
        const int M = 1, minRC = this->minRoomCells->getInt(), maxRC = this->maxRoomCells->getInt();
        int avC = node->cols - 2 * M, avR = node->rows - 2 * M;
        if (avC < minRC || avR < minRC)
        {
            return;
        }

        int rc = std::uniform_int_distribution<int>(minRC, maxValue(minRC, std::min(maxRC, avC)))(rng);
        int rr = std::uniform_int_distribution<int>(minRC, maxValue(minRC, std::min(maxRC, avR)))(rng);
        int oc = M + std::uniform_int_distribution<int>(0, avC - rc)(rng);
        int or_ = M + std::uniform_int_distribution<int>(0, avR - rr)(rng);

        DungeonRoom room;
        room.id = static_cast<int>(rooms.size());
        room.col = node->col + oc;
        room.row = node->row + or_;
        room.cols = rc;
        room.rows = rr;
        room.floorHeight = 0.0f;
        node->roomId = room.id;
        rooms.push_back(room);
    }

    void ProceduralDungeonComponent::collectLeafRooms(BSPNode* node, std::vector<DungeonRoom>& rooms, std::mt19937& rng)
    {
        if (!node->left && !node->right)
        {
            this->placeRoomInLeaf(node, rooms, rng);
            return;
        }
        if (node->left)
        {
            this->collectLeafRooms(node->left.get(), rooms, rng);
        }
        if (node->right)
        {
            this->collectLeafRooms(node->right.get(), rooms, rng);
        }
    }

    int ProceduralDungeonComponent::getRandomLeafRoomId(BSPNode* node, std::mt19937& rng)
    {
        if (!node->left && !node->right)
        {
            return node->roomId;
        }
        bool goLeft = (std::uniform_int_distribution<int>(0, 1)(rng) == 0);
        if (!node->left)
        {
            goLeft = false;
        }
        if (!node->right)
        {
            goLeft = true;
        }
        int id = this->getRandomLeafRoomId(goLeft ? node->left.get() : node->right.get(), rng);
        if (id < 0)
        {
            BSPNode* other = goLeft ? (node->right ? node->right.get() : nullptr) : (node->left ? node->left.get() : nullptr);
            if (other)
            {
                id = this->getRandomLeafRoomId(other, rng);
            }
        }
        return id;
    }

    void ProceduralDungeonComponent::connectBSPSubtrees(BSPNode* node, const std::vector<DungeonRoom>& rooms, std::vector<DungeonCorridor>& corridors, std::mt19937& rng)
    {
        if (!node->left && !node->right)
        {
            return;
        }
        if (node->left)
        {
            this->connectBSPSubtrees(node->left.get(), rooms, corridors, rng);
        }
        if (node->right)
        {
            this->connectBSPSubtrees(node->right.get(), rooms, corridors, rng);
        }

        int lId = node->left ? this->getRandomLeafRoomId(node->left.get(), rng) : -1;
        int rId = node->right ? this->getRandomLeafRoomId(node->right.get(), rng) : -1;
        if (lId < 0 || rId < 0 || lId >= (int)rooms.size() || rId >= (int)rooms.size())
        {
            return;
        }

        const DungeonRoom& a = rooms[lId];
        const DungeonRoom& b = rooms[rId];
        DungeonCorridor c;
        c.fromRoomId = lId;
        c.toRoomId = rId;
        c.bendCol = b.centerCol();
        c.bendRow = a.centerRow();
        corridors.push_back(c);
    }

    void ProceduralDungeonComponent::addLoopCorridors(const std::vector<DungeonRoom>& rooms, std::vector<DungeonCorridor>& corridors, std::mt19937& rng)
    {
        const float prob = this->loopProbability->getReal();
        if (prob <= 0.0f || rooms.size() < 2)
        {
            return;
        }
        std::set<std::pair<int, int>> connected;
        for (const auto& c : corridors)
        {
            connected.insert({std::min(c.fromRoomId, c.toRoomId), maxValue(c.fromRoomId, c.toRoomId)});
        }
        std::uniform_real_distribution<float> d01(0.0f, 1.0f);
        for (int i = 0; i < (int)rooms.size(); ++i)
        {
            for (int j = i + 1; j < (int)rooms.size(); ++j)
            {
                if (connected.count({i, j}))
                {
                    continue;
                }
                if (d01(rng) < prob)
                {
                    DungeonCorridor c;
                    c.fromRoomId = i;
                    c.toRoomId = j;
                    c.bendCol = rooms[i].centerCol();
                    c.bendRow = rooms[j].centerRow();
                    corridors.push_back(c);
                    connected.insert({i, j});
                }
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Grid
    ///////////////////////////////////////////////////////////////////////////////////////////////

    void ProceduralDungeonComponent::buildGrid(const std::vector<DungeonRoom>& rooms, const std::vector<DungeonCorridor>& corridors, std::vector<std::vector<CellType>>& grid, int gridCols, int gridRows) const
    {
        grid.assign(gridRows, std::vector<CellType>(gridCols, CellType::EMPTY));
        for (const auto& room : rooms)
        {
            for (int r = room.row; r < room.row + room.rows; ++r)
            {
                for (int c = room.col; c < room.col + room.cols; ++c)
                {
                    if (r >= 0 && r < gridRows && c >= 0 && c < gridCols)
                    {
                        grid[r][c] = CellType::ROOM;
                    }
                }
            }
        }
        int hw = this->corridorWidthCells->getInt() / 2;
        for (const auto& cor : corridors)
        {
            this->markCorridor(grid, cor, rooms, hw, gridCols, gridRows);
        }
    }

    void ProceduralDungeonComponent::markCorridor(std::vector<std::vector<CellType>>& grid, const DungeonCorridor& corridor, const std::vector<DungeonRoom>& rooms, int hw, int gridCols, int gridRows) const
    {
        if (corridor.fromRoomId < 0 || corridor.toRoomId < 0 || corridor.fromRoomId >= (int)rooms.size() || corridor.toRoomId >= (int)rooms.size())
        {
            return;
        }
        const DungeonRoom& a = rooms[corridor.fromRoomId];
        int ax = a.centerCol(), az = a.centerRow();
        int bx = corridor.bendCol, bz = corridor.bendRow;
        const DungeonRoom& b = rooms[corridor.toRoomId];
        int ex = b.centerCol(), ez = b.centerRow();

        auto mark = [&](int c, int r)
        {
            if (r >= 0 && r < gridRows && c >= 0 && c < gridCols && grid[r][c] == CellType::EMPTY)
            {
                grid[r][c] = CellType::CORRIDOR;
            }
        };
        for (int c = std::min(ax, bx); c <= maxValue(ax, bx); ++c)
        {
            for (int r = bz - hw; r <= bz + hw; ++r)
            {
                mark(c, r);
            }
        }
        for (int r = std::min(bz, ez); r <= maxValue(bz, ez); ++r)
        {
            for (int c = bx - hw; c <= bx + hw; ++c)
            {
                mark(c, r);
            }
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Generation entry points
    ///////////////////////////////////////////////////////////////////////////////////////////////

    void ProceduralDungeonComponent::generateDungeon(void)
    {
        Ogre::Vector3 origin = this->hasDungeonOrigin ? this->dungeonOrigin : this->gameObjectPtr->getSceneNode()->getPosition();
        this->generateDungeonAtPosition(origin);
    }

    void ProceduralDungeonComponent::generateDungeonAtPosition(const Ogre::Vector3& worldPosition)
    {
        Ogre::Real floorY = this->getGroundHeight(worldPosition);
        this->dungeonOrigin = worldPosition;
        this->dungeonOrigin.y = floorY;
        this->hasDungeonOrigin = true;

        const int seedVal = this->seed->getInt();
        const Ogre::Real cellSz = maxValue(0.1f, this->cellSize->getReal());
        const int gridCols = maxValue(4, (int)(this->dungeonWidth->getReal() / cellSz));
        const int gridRows = maxValue(4, (int)(this->dungeonDepth->getReal() / cellSz));
        const DungeonTheme theme = this->getDungeonThemeEnum();
        const Ogre::Real wallH = this->wallHeight->getReal();
        const Ogre::Vector2 floorUV = this->floorUVTiling->getVector2();
        const Ogre::Vector2 wallUV = this->wallUVTiling->getVector2();
        const Ogre::Real effWallH = (theme == DungeonTheme::CAVE) ? wallH * 0.85f : (theme == DungeonTheme::CRYPT) ? wallH * 1.2f : wallH;
        const Ogre::Real floorDepth = std::max(0.005f, this->floorDepth->getReal());
        const Ogre::Real levelSpacing = effWallH + floorDepth;
        const int numLevels = std::max(1, this->levelCount->getInt());

        // Clear everything
        this->floorVertices.clear();
        this->floorIndices.clear();
        this->currentFloorVertexIndex = 0;
        this->wallVertices.clear();
        this->wallIndices.clear();
        this->currentWallVertexIndex = 0;
        this->ceilVertices.clear();
        this->ceilIndices.clear();
        this->currentCeilVertexIndex = 0;
        this->levelGrids.clear();
        this->dungeonStaircases.clear();
        this->dungeonRooms.clear();
        this->dungeonCorridors.clear();

        // Keep per-level rooms for floorHeight bookkeeping
        std::vector<std::vector<DungeonRoom>> allRooms(numLevels);
        std::vector<std::vector<DungeonCorridor>> allCorridors(numLevels);

        // =========================================================
        // PASS 1 — build ALL grids before picking any staircases
        // =========================================================
        for (int lvl = 0; lvl < numLevels; ++lvl)
        {
            const Ogre::Real lvlFloorY = floorY + static_cast<Ogre::Real>(lvl) * levelSpacing;

            std::mt19937 lvlRng(static_cast<uint32_t>(seedVal + lvl * 999983));
            BSPNode lvlRoot;
            lvlRoot.col = 0;
            lvlRoot.row = 0;
            lvlRoot.cols = gridCols;
            lvlRoot.rows = gridRows;
            this->splitBSP(&lvlRoot, lvlRng, 0);

            this->collectLeafRooms(&lvlRoot, allRooms[lvl], lvlRng);
            if (allRooms[lvl].empty())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralDungeonComponent] BSP produced no rooms for level " + Ogre::StringConverter::toString(lvl) + "!");
                // Push empty grid so indices stay consistent
                this->levelGrids.push_back({});
                continue;
            }

            for (auto& room : allRooms[lvl])
            {
                if (this->adaptToGround->getBool() && lvl == 0)
                {
                    Ogre::Real cx = worldPosition.x + (room.centerCol() + 0.5f) * cellSz;
                    Ogre::Real cz = worldPosition.z + (room.centerRow() + 0.5f) * cellSz;
                    room.floorHeight = this->getGroundHeight({cx, worldPosition.y, cz});
                }
                else
                {
                    room.floorHeight = lvlFloorY;
                }
            }

            this->connectBSPSubtrees(&lvlRoot, allRooms[lvl], allCorridors[lvl], lvlRng);
            this->addLoopCorridors(allRooms[lvl], allCorridors[lvl], lvlRng);

            std::vector<std::vector<CellType>> lvlGrid;
            this->buildGrid(allRooms[lvl], allCorridors[lvl], lvlGrid, gridCols, gridRows);
            this->levelGrids.push_back(lvlGrid);

            if (lvl == 0)
            {
                this->dungeonRooms = allRooms[lvl];
                this->dungeonCorridors = allCorridors[lvl];
            }
        }

        // =========================================================
        // PASS 2 — now ALL grids exist, pick staircases safely
        // =========================================================
        for (int lvl = 0; lvl < numLevels - 1; ++lvl)
        {
            if (this->levelGrids[lvl].empty() || this->levelGrids[lvl + 1].empty())
            {
                continue;
            }

            auto [sc, sr] = this->pickStaircaseCell(this->levelGrids[lvl], gridCols, gridRows, lvl);

            if (sc >= 0)
            {
                DungeonStaircase stair;
                stair.gridCol = sc;
                stair.gridRow = sr;
                stair.fromLevel = lvl;
                stair.worldPos = Ogre::Vector3(sc * cellSz + cellSz * 0.5f, floorY + static_cast<Ogre::Real>(lvl) * levelSpacing, sr * cellSz);
                this->dungeonStaircases.push_back(stair);
            }
        }

        // =========================================================
        // PASS 3 — generate geometry for each level
        // =========================================================
        for (int lvl = 0; lvl < numLevels; ++lvl)
        {
            if (this->levelGrids[lvl].empty())
            {
                continue;
            }

            const Ogre::Real lvlFloorY = floorY + static_cast<Ogre::Real>(lvl) * levelSpacing;

            // Ceiling holes: staircase going UP from this level
            this->pendingCeilingHoles.clear();
            for (const auto& s : this->dungeonStaircases)
            {
                if (s.fromLevel == lvl)
                {
                    this->pendingCeilingHoles.push_back({s.gridCol, s.gridRow});
                }
            }

            // Floor holes: staircase arriving FROM level below
            this->pendingFloorHoles.clear();
            for (const auto& s : this->dungeonStaircases)
            {
                if (s.fromLevel == lvl - 1)
                {
                    this->pendingFloorHoles.push_back({s.gridCol, s.gridRow});
                }
            }

            this->generateGeometry(this->levelGrids[lvl], gridCols, gridRows, cellSz, effWallH, floorUV, wallUV, theme, seedVal + lvl, lvlFloorY);

            // Staircase geometry for stairs starting on this level
            for (const auto& stair : this->dungeonStaircases)
            {
                if (stair.fromLevel == lvl)
                {
                    this->generateStaircaseGeometry(stair, cellSz, levelSpacing, wallUV);
                }
            }
        }

        this->cachedFloorVertices = this->floorVertices;
        this->cachedFloorIndices = this->floorIndices;
        this->cachedNumFloorVertices = this->currentFloorVertexIndex;
        this->cachedWallVertices = this->wallVertices;
        this->cachedWallIndices = this->wallIndices;
        this->cachedNumWallVertices = this->currentWallVertexIndex;
        this->cachedCeilVertices = this->ceilVertices;
        this->cachedCeilIndices = this->ceilIndices;
        this->cachedNumCeilVertices = this->currentCeilVertexIndex;

        this->destroyPreviewMesh();
        this->createDungeonMesh();

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralDungeonComponent] Generated " + Ogre::StringConverter::toString(numLevels) + " level(s): " + Ogre::StringConverter::toString((int)this->dungeonRooms.size()) +
                                                                               " rooms (L0), " + Ogre::StringConverter::toString((int)this->dungeonStaircases.size()) + " staircases, " +
                                                                               "FV=" + Ogre::StringConverter::toString(this->cachedNumFloorVertices) + " WV=" + Ogre::StringConverter::toString(this->cachedNumWallVertices) +
                                                                               " CV=" + Ogre::StringConverter::toString(this->cachedNumCeilVertices));
    }

    void ProceduralDungeonComponent::rebuildMesh(void)
    {
        if (this->dungeonRooms.empty())
        {
            return;
        }

        const int seedVal = this->seed->getInt();
        const Ogre::Real cellSz = maxValue(0.1f, this->cellSize->getReal());
        const int gridCols = maxValue(4, (int)(this->dungeonWidth->getReal() / cellSz));
        const int gridRows = maxValue(4, (int)(this->dungeonDepth->getReal() / cellSz));
        const DungeonTheme theme = this->getDungeonThemeEnum();
        const Ogre::Real wallH = this->wallHeight->getReal();
        const Ogre::Real effWallH = (theme == DungeonTheme::CAVE) ? wallH * 0.85f : (theme == DungeonTheme::CRYPT) ? wallH * 1.2f : wallH;
        const Ogre::Vector2 floorUV = this->floorUVTiling->getVector2();
        const Ogre::Vector2 wallUV = this->wallUVTiling->getVector2();

        const Ogre::Real floorDepth = std::max(0.005f, this->floorDepth->getReal());
        const Ogre::Real levelSpacing = effWallH + floorDepth; // ceiling of N never touches floor of N+1

        this->floorVertices.clear();
        this->floorIndices.clear();
        this->currentFloorVertexIndex = 0;
        this->wallVertices.clear();
        this->wallIndices.clear();
        this->currentWallVertexIndex = 0;
        this->ceilVertices.clear();
        this->ceilIndices.clear();
        this->currentCeilVertexIndex = 0;

        // If we have cached level grids use them directly — no BSP rebuild needed
        const int numLevels = (int)this->levelGrids.size();

        if (numLevels == 0)
        {
            // Fallback: single level from dungeonRooms (old saves / single-level case)
            std::vector<std::vector<CellType>> grid;
            this->buildGrid(this->dungeonRooms, this->dungeonCorridors, grid, gridCols, gridRows);
            this->pendingCeilingHoles.clear();
            this->generateGeometry(grid, gridCols, gridRows, cellSz, effWallH, floorUV, wallUV, theme, seedVal, 0.0f);
        }
        else
        {
            this->dungeonStaircases.clear();

            for (int lvl = 0; lvl < numLevels; ++lvl)
            {
                const Ogre::Real lvlFloorY = static_cast<Ogre::Real>(lvl) * levelSpacing;
                const std::vector<std::vector<CellType>>& lvlGrid = this->levelGrids[lvl];

                // Recompute staircases deterministically from cached grids
                this->pendingCeilingHoles.clear();
                if (lvl < numLevels - 1)
                {
                    auto [sc, sr] = this->pickStaircaseCell(lvlGrid, gridCols, gridRows, lvl);
                    if (sc >= 0)
                    {
                        DungeonStaircase stair;
                        stair.gridCol = sc;
                        stair.gridRow = sr;
                        stair.fromLevel = lvl;
                        stair.worldPos = Ogre::Vector3(sc * cellSz + cellSz * 0.5f, lvlFloorY, sr * cellSz + cellSz * 0.5f);
                        this->dungeonStaircases.push_back(stair);
                        this->pendingCeilingHoles.push_back({sc, sr});
                    }
                }

                this->generateGeometry(lvlGrid, gridCols, gridRows, cellSz, effWallH, floorUV, wallUV, theme, seedVal + lvl, lvlFloorY);

                for (const auto& stair : this->dungeonStaircases)
                {
                    if (stair.fromLevel == lvl)
                    {
                        this->generateStaircaseGeometry(stair, cellSz, levelSpacing, wallUV);
                    }
                }
            }
        }

        this->cachedFloorVertices = this->floorVertices;
        this->cachedFloorIndices = this->floorIndices;
        this->cachedNumFloorVertices = this->currentFloorVertexIndex;
        this->cachedWallVertices = this->wallVertices;
        this->cachedWallIndices = this->wallIndices;
        this->cachedNumWallVertices = this->currentWallVertexIndex;
        this->cachedCeilVertices = this->ceilVertices;
        this->cachedCeilIndices = this->ceilIndices;
        this->cachedNumCeilVertices = this->currentCeilVertexIndex;
        this->createDungeonMesh();
    }

    void ProceduralDungeonComponent::clearDungeon(void)
    {
        this->destroyDungeonMesh();
        this->dungeonRooms.clear();
        this->dungeonCorridors.clear();
        this->levelGrids.clear();
        this->dungeonStaircases.clear(); 
        this->pendingCeilingHoles.clear();
        this->hasDungeonOrigin = this->originPositionSet = false;
        this->cachedNumFloorVertices = this->cachedNumWallVertices = this->cachedNumCeilVertices = 0;
        this->cachedFloorVertices.clear();
        this->cachedFloorIndices.clear();
        this->cachedWallVertices.clear();
        this->cachedWallIndices.clear();
        this->cachedCeilVertices.clear();
        this->cachedCeilIndices.clear();
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Geometry
    ///////////////////////////////////////////////////////////////////////////////////////////////

    void ProceduralDungeonComponent::generateGeometry(const std::vector<std::vector<CellType>>& grid, int gridCols, int gridRows, Ogre::Real cellSz, Ogre::Real effWallH, const Ogre::Vector2& floorUV, const Ogre::Vector2& wallUV, DungeonTheme theme,
        int seedVal, Ogre::Real floorY)
    {
        if (theme == DungeonTheme::CAVE)
        {
            this->generateCaveFloorGeometry(grid, gridCols, gridRows, cellSz, floorY, floorUV, seedVal, this->jitter->getReal());
            this->generateCaveWallGeometry(grid, gridCols, gridRows, cellSz, floorY, effWallH, wallUV, seedVal, this->jitter->getReal());
            this->generateCaveCeilingGeometry(grid, gridCols, gridRows, cellSz, floorY, effWallH, wallUV, seedVal, this->jitter->getReal());
            this->generateStalactites(grid, gridCols, gridRows, cellSz, floorY, effWallH, seedVal, true, this->jitter->getReal());
        }
        else
        {
            // In generateGeometry, replace generateFloorGeometry call:
            // For upper levels the staircase opening cell needs a floor hole too —
            // pendingCeilingHoles holds the staircase cell for THIS level's ceiling,
            // but the upper level's floor hole is from the level BELOW's staircase.
            // Pass pendingFloorHoles separately via member, same pattern as ceiling.
            this->generateFloorGeometry(grid, gridCols, gridRows, cellSz, floorY, floorUV, this->pendingFloorHoles);

            if (theme == DungeonTheme::SCIFI)
            {
                this->generateScifiFloorChannel(grid, gridCols, gridRows, cellSz, floorY, floorUV);
            }
            this->generateWallGeometry(grid, gridCols, gridRows, cellSz, floorY, effWallH, wallUV, theme);
            if (this->addCeiling->getBool() || theme == DungeonTheme::SCIFI)
            {
                this->generateCeilingGeometry(grid, gridCols, gridRows, cellSz, floorY + 0.001f, effWallH, wallUV, this->pendingCeilingHoles);
            }
            if (this->addPillars->getBool())
            {
                this->generatePillarGeometry(grid, gridCols, gridRows, cellSz, floorY, effWallH, this->pillarSize->getReal());
            }
        }
    }

    void ProceduralDungeonComponent::generateFloorGeometry(const std::vector<std::vector<CellType>>& grid, int gridCols, int gridRows, Ogre::Real cellSz, Ogre::Real floorY, const Ogre::Vector2& uvTile,
        const std::vector<std::pair<int, int>>& holeCells)
    {
        const Ogre::Real uS = maxValue(0.001f, uvTile.x);
        const Ogre::Real vS = maxValue(0.001f, uvTile.y);
        const Ogre::Real depth = std::max(0.005f, this->floorDepth->getReal());
        const Ogre::Real yTop = floorY;
        const Ogre::Real yBot = floorY - depth;

        auto isF = [&](int r, int c)
        {
            return r >= 0 && r < gridRows && c >= 0 && c < gridCols && grid[r][c] != CellType::EMPTY;
        };
        auto isHoleCell = [&](int r, int c)
        {
            for (const auto& hc : holeCells)
            {
                if (hc.first == c && hc.second == r)
                {
                    return true;
                }
            }
            return false;
        };

        for (int row = 0; row < gridRows; ++row)
        {
            for (int col = 0; col < gridCols; ++col)
            {
                if (!isF(row, col) || isHoleCell(row, col))
                {
                    continue;
                }

                Ogre::Real x0 = col * cellSz, x1 = x0 + cellSz;
                Ogre::Real z0 = row * cellSz, z1 = z0 + cellSz;
                Ogre::Real u0 = x0 / uS, u1 = x1 / uS, v0 = z0 / vS, v1 = z1 / vS;
                Ogre::Real uW = cellSz / uS, vD = depth / vS;

                // Top face (upward)
                this->addFloorQuad({x0, yTop, z1}, {x1, yTop, z1}, {x1, yTop, z0}, {x0, yTop, z0}, u0, u1, v0, v1);

                // Bottom face (downward) — single-sided, only visible from below
                pushQuad(this->floorVertices, this->floorIndices, this->currentFloorVertexIndex, {x0, yBot, z0}, {x1, yBot, z0}, {x1, yBot, z1}, {x0, yBot, z1}, -Ogre::Vector3::UNIT_Y, u0, u1, v0, v1);

                // Border edges — only where there is no neighbour (exposed slab edge)
                if (!isF(row + 1, col)) // North
                {
                    this->addWallQuad({x0, yBot, z1}, {x1, yBot, z1}, {x1, yTop, z1}, {x0, yTop, z1}, {0, 0, 1}, 0, uW, 0, vD);
                }
                if (!isF(row - 1, col)) // South
                {
                    this->addWallQuad({x1, yBot, z0}, {x0, yBot, z0}, {x0, yTop, z0}, {x1, yTop, z0}, {0, 0, -1}, 0, uW, 0, vD);
                }
                if (!isF(row, col + 1)) // East
                {
                    this->addWallQuad({x1, yBot, z1}, {x1, yBot, z0}, {x1, yTop, z0}, {x1, yTop, z1}, {1, 0, 0}, 0, uW, 0, vD);
                }
                if (!isF(row, col - 1)) // West
                {
                    this->addWallQuad({x0, yBot, z0}, {x0, yBot, z1}, {x0, yTop, z1}, {x0, yTop, z0}, {-1, 0, 0}, 0, uW, 0, vD);
                }
            }
        }
    }

    void ProceduralDungeonComponent::generateWallTopCaps(const std::vector<std::vector<CellType>>& grid, int gridCols, int gridRows, Ogre::Real cellSz, Ogre::Real floorY, Ogre::Real wallH, const Ogre::Vector2& uvTile)
    {
        const Ogre::Real uS = std::max(0.001f, uvTile.x);
        const Ogre::Real vS = std::max(0.001f, uvTile.y);
        const Ogre::Real y1 = floorY + wallH;

        auto isF = [&](int r, int c)
        {
            return r >= 0 && r < gridRows && c >= 0 && c < gridCols && grid[r][c] != CellType::EMPTY;
        };

        for (int row = 0; row < gridRows; ++row)
        {
            for (int col = 0; col < gridCols; ++col)
            {
                if (!isF(row, col))
                {
                    continue;
                }
                if (isF(row - 1, col) && isF(row + 1, col) && isF(row, col - 1) && isF(row, col + 1))
                {
                    continue;
                }

                Ogre::Real x0 = (Ogre::Real)col * cellSz, x1 = x0 + cellSz;
                Ogre::Real z0 = (Ogre::Real)row * cellSz, z1 = z0 + cellSz;

                this->addWallTopCapQuad({x0, y1, z1}, {x1, y1, z1}, {x1, y1, z0}, {x0, y1, z0}, x0 / uS, x1 / uS, z0 / vS, z1 / vS);
            }
        }
    }

    void ProceduralDungeonComponent::generateWallGeometry(const std::vector<std::vector<CellType>>& grid, int gridCols, int gridRows, Ogre::Real cellSz, Ogre::Real floorY, Ogre::Real wallH, const Ogre::Vector2& uvTile, DungeonTheme theme)
    {
        const bool bev = (theme == DungeonTheme::ICE);
        const float ba = 0.07f * cellSz;
        const Ogre::Real uS = std::max(0.001f, uvTile.x);
        const Ogre::Real vS = std::max(0.001f, uvTile.y);

        // Windows only on plain rectangular walls — not bevelled ICE, not CAVE
        const bool doWindows = this->addWindows->getBool() && !bev;
        const Ogre::Real winW = this->windowWidth->getReal();
        const Ogre::Real winH = this->windowHeight->getReal();
        const Ogre::Real winSill = this->windowSill->getReal();
        const Ogre::Real winProb = this->windowProb->getReal();

        // Deterministic per-face hash — same seed always gives same window pattern
        auto winRng = [&](int row, int col, int dir) -> Ogre::Real
        {
            uint32_t h = static_cast<uint32_t>(row * 73856093 ^ col * 19349663 ^ dir * 83492791 ^ this->seed->getInt());
            h ^= h >> 16;
            h *= 0x45d9f3bu;
            h ^= h >> 16;
            return static_cast<float>(h & 0xFFFFu) / 65535.0f;
        };

        auto isF = [&](int r, int c)
        {
            return r >= 0 && r < gridRows && c >= 0 && c < gridCols && grid[r][c] != CellType::EMPTY;
        };

        for (int row = 0; row < gridRows; ++row)
        {
            for (int col = 0; col < gridCols; ++col)
            {
                if (!isF(row, col))
                {
                    continue;
                }

                const Ogre::Real x0 = static_cast<Ogre::Real>(col) * cellSz;
                const Ogre::Real x1 = x0 + cellSz;
                const Ogre::Real z0 = static_cast<Ogre::Real>(row) * cellSz;
                const Ogre::Real z1 = z0 + cellSz;
                const Ogre::Real y0 = floorY;
                const Ogre::Real y1 = floorY + wallH;
                const Ogre::Real uW = cellSz / uS;
                const Ogre::Real vT = wallH / vS;

                // ------------------------------------------------------------------
                // North face (+Z)
                // ------------------------------------------------------------------
                if (!isF(row + 1, col))
                {
                    if (bev)
                    {
                        this->addWallQuad({x0, y0, z1}, {x1, y0, z1}, {x1 - ba, y1, z1 - ba}, {x0 + ba, y1, z1 - ba}, {0, 0, 1}, 0, uW, 0, vT);
                    }
                    else if (doWindows && winRng(row, col, 0) < winProb)
                    {
                        this->addWallQuadWithWindow({x0, y0, z1}, {x1, y0, z1}, // bottom-left, bottom-right
                            {x0, y1, z1}, {x1, y1, z1},                         // top-left,    top-right
                            {0, 0, 1}, wallH, uW, vT, winW, winH, winSill);
                    }
                    else
                    {
                        this->addWallQuad({x0, y0, z1}, {x1, y0, z1}, {x1, y1, z1}, {x0, y1, z1}, {0, 0, 1}, 0, uW, 0, vT);
                    }
                }

                // ------------------------------------------------------------------
                // South face (-Z)
                // ------------------------------------------------------------------
                if (!isF(row - 1, col))
                {
                    if (bev)
                    {
                        this->addWallQuad({x1, y0, z0}, {x0, y0, z0}, {x0 - ba, y1, z0 + ba}, {x1 + ba, y1, z0 + ba}, {0, 0, -1}, 0, uW, 0, vT);
                    }
                    else if (doWindows && winRng(row, col, 1) < winProb)
                    {
                        this->addWallQuadWithWindow({x1, y0, z0}, {x0, y0, z0}, {x1, y1, z0}, {x0, y1, z0}, {0, 0, -1}, wallH, uW, vT, winW, winH, winSill);
                    }
                    else
                    {
                        this->addWallQuad({x1, y0, z0}, {x0, y0, z0}, {x0, y1, z0}, {x1, y1, z0}, {0, 0, -1}, 0, uW, 0, vT);
                    }
                }

                // ------------------------------------------------------------------
                // East face (+X)
                // ------------------------------------------------------------------
                if (!isF(row, col + 1))
                {
                    if (bev)
                    {
                        this->addWallQuad({x1, y0, z1}, {x1, y0, z0}, {x1 - ba, y1, z0 + ba}, {x1 - ba, y1, z1 - ba}, {1, 0, 0}, 0, uW, 0, vT);
                    }
                    else if (doWindows && winRng(row, col, 2) < winProb)
                    {
                        this->addWallQuadWithWindow({x1, y0, z1}, {x1, y0, z0}, {x1, y1, z1}, {x1, y1, z0}, {1, 0, 0}, wallH, uW, vT, winW, winH, winSill);
                    }
                    else
                    {
                        this->addWallQuad({x1, y0, z1}, {x1, y0, z0}, {x1, y1, z0}, {x1, y1, z1}, {1, 0, 0}, 0, uW, 0, vT);
                    }
                }

                // ------------------------------------------------------------------
                // West face (-X)
                // ------------------------------------------------------------------
                if (!isF(row, col - 1))
                {
                    if (bev)
                    {
                        this->addWallQuad({x0, y0, z0}, {x0, y0, z1}, {x0 + ba, y1, z1 - ba}, {x0 + ba, y1, z0 + ba}, {-1, 0, 0}, 0, uW, 0, vT);
                    }
                    else if (doWindows && winRng(row, col, 3) < winProb)
                    {
                        this->addWallQuadWithWindow({x0, y0, z0}, {x0, y0, z1}, {x0, y1, z0}, {x0, y1, z1}, {-1, 0, 0}, wallH, uW, vT, winW, winH, winSill);
                    }
                    else
                    {
                        this->addWallQuad({x0, y0, z0}, {x0, y0, z1}, {x0, y1, z1}, {x0, y1, z0}, {-1, 0, 0}, 0, uW, 0, vT);
                    }
                }
            }
        }
    }

    void ProceduralDungeonComponent::generateCeilingGeometry(const std::vector<std::vector<CellType>>& grid, int gridCols, int gridRows, Ogre::Real cellSz, Ogre::Real floorY, Ogre::Real wallH, const Ogre::Vector2& uvTile,
        const std::vector<std::pair<int, int>>& holeCells)
    {
        const Ogre::Real uS = maxValue(0.001f, uvTile.x);
        const Ogre::Real vS = maxValue(0.001f, uvTile.y);
        const Ogre::Real y = floorY + wallH;

        for (int row = 0; row < gridRows; ++row)
        {
            for (int col = 0; col < gridCols; ++col)
            {
                if (grid[row][col] == CellType::EMPTY)
                {
                    continue;
                }

                // Skip cells that have a staircase hole punched through them
                bool isHole = false;
                for (const auto& hc : holeCells)
                {
                    if (hc.first == col && hc.second == row)
                    {
                        isHole = true;
                        break;
                    }
                }
                if (isHole)
                {
                    continue;
                }

                Ogre::Real x0 = (Ogre::Real)col * cellSz, x1 = x0 + cellSz;
                Ogre::Real z0 = (Ogre::Real)row * cellSz, z1 = z0 + cellSz;
                this->addCeilingQuad({x0, y, z0}, {x1, y, z0}, {x1, y, z1}, {x0, y, z1}, x0 / uS, x1 / uS, z0 / vS, z1 / vS);
            }
        }
    }

    void ProceduralDungeonComponent::generatePillarGeometry(const std::vector<std::vector<CellType>>& grid, int gridCols, int gridRows, Ogre::Real cellSz, Ogre::Real floorY, Ogre::Real wallH, Ogre::Real pillarSz)
    {
        if (pillarSz < 0.01f)
        {
            return;
        }
        const Ogre::Real hp = pillarSz * 0.5f, uW = pillarSz / maxValue(0.001f, this->wallUVTiling->getVector2().x), vW = wallH / maxValue(0.001f, this->wallUVTiling->getVector2().y);
        auto isF = [&](int r, int c)
        {
            return r >= 0 && r < gridRows && c >= 0 && c < gridCols && grid[r][c] != CellType::EMPTY;
        };

        for (int cRow = 0; cRow <= gridRows; ++cRow)
        {
            for (int cCol = 0; cCol <= gridCols; ++cCol)
            {
                bool sw = isF(cRow - 1, cCol - 1), se = isF(cRow - 1, cCol), nw = isF(cRow, cCol - 1), ne = isF(cRow, cCol);
                if (!((sw && !se && !nw) || (se && !sw && !ne) || (nw && !sw && !ne) || (ne && !nw && !se)))
                {
                    continue;
                }
                Ogre::Real cx = (Ogre::Real)cCol * cellSz, cz = (Ogre::Real)cRow * cellSz;
                Ogre::Real px0 = cx - hp, px1 = cx + hp, pz0 = cz - hp, pz1 = cz + hp, py0 = floorY, py1 = floorY + wallH;
                this->addWallQuad({px0, py0, pz1}, {px1, py0, pz1}, {px1, py1, pz1}, {px0, py1, pz1}, {0, 0, 1}, 0, uW, 0, vW);
                this->addWallQuad({px1, py0, pz0}, {px0, py0, pz0}, {px0, py1, pz0}, {px1, py1, pz0}, {0, 0, -1}, 0, uW, 0, vW);
                this->addWallQuad({px1, py0, pz1}, {px1, py0, pz0}, {px1, py1, pz0}, {px1, py1, pz1}, {1, 0, 0}, 0, uW, 0, vW);
                this->addWallQuad({px0, py0, pz0}, {px0, py0, pz1}, {px0, py1, pz1}, {px0, py1, pz0}, {-1, 0, 0}, 0, uW, 0, vW);
                this->addWallTopCapQuad({px0, py1, pz1}, {px1, py1, pz1}, {px1, py1, pz0}, {px0, py1, pz0}, 0, uW, 0, uW);
            }
        }
    }

    void ProceduralDungeonComponent::generateWallCaps(const std::vector<std::vector<CellType>>& grid, int gridCols, int gridRows, Ogre::Real cellSz, Ogre::Real floorY, Ogre::Real wallH, Ogre::Real capFraction)
    {
        const Ogre::Real capH = wallH * capFraction;
        const Ogre::Real capOut = cellSz * 0.12f;
        const Ogre::Real y0 = floorY + wallH;
        const Ogre::Real y1 = y0 + capH;
        const Ogre::Real uS = std::max(0.001f, this->wallUVTiling->getVector2().x);
        const Ogre::Real vS = std::max(0.001f, this->wallUVTiling->getVector2().y);

        auto isF = [&](int r, int c)
        {
            return r >= 0 && r < gridRows && c >= 0 && c < gridCols && grid[r][c] != CellType::EMPTY;
        };

        for (int row = 0; row < gridRows; ++row)
        {
            for (int col = 0; col < gridCols; ++col)
            {
                if (!isF(row, col))
                {
                    continue;
                }

                // Skip fully interior cells — their tops are never visible
                if (isF(row - 1, col) && isF(row + 1, col) && isF(row, col - 1) && isF(row, col + 1))
                {
                    continue;
                }

                Ogre::Real x0 = (Ogre::Real)col * cellSz, x1 = x0 + cellSz;
                Ogre::Real z0 = (Ogre::Real)row * cellSz, z1 = z0 + cellSz;
                Ogre::Real uW = cellSz / uS, vC = capH / vS;

                // ONE full-cell top slab — covers the whole cell, no corner gaps
                this->addWallQuad({x0, y1, z1}, {x1, y1, z1}, {x1, y1, z0}, {x0, y1, z0}, {0, 1, 0}, x0 / uS, x1 / uS, z0 / vS, z1 / vS);

                // Outer vertical faces — one per empty neighbour direction
                if (!isF(row + 1, col))
                {
                    this->addWallQuad({x0, y0, z1}, {x1, y0, z1}, {x1, y1, z1}, {x0, y1, z1}, {0, 0, 1}, 0, uW, 0, vC);
                }
                if (!isF(row - 1, col))
                {
                    this->addWallQuad({x1, y0, z0}, {x0, y0, z0}, {x0, y1, z0}, {x1, y1, z0}, {0, 0, -1}, 0, uW, 0, vC);
                }
                if (!isF(row, col + 1))
                {
                    this->addWallQuad({x1, y0, z1}, {x1, y0, z0}, {x1, y1, z0}, {x1, y1, z1}, {1, 0, 0}, 0, uW, 0, vC);
                }
                if (!isF(row, col - 1))
                {
                    this->addWallQuad({x0, y0, z0}, {x0, y0, z1}, {x0, y1, z1}, {x0, y1, z0}, {-1, 0, 0}, 0, uW, 0, vC);
                }
            }
        }
    }

    void ProceduralDungeonComponent::generateScifiFloorChannel(const std::vector<std::vector<CellType>>& grid, int gridCols, int gridRows, Ogre::Real cellSz, Ogre::Real floorY, const Ogre::Vector2& uvTile)
    {
        const Ogre::Real cW = cellSz * 0.08f, cD = cellSz * 0.04f;
        const Ogre::Real uS = maxValue(0.001f, uvTile.x), vS = maxValue(0.001f, uvTile.y);
        auto isF = [&](int r, int c)
        {
            return r >= 0 && r < gridRows && c >= 0 && c < gridCols && grid[r][c] != CellType::EMPTY;
        };
        for (int row = 0; row < gridRows; ++row)
        {
            for (int col = 0; col < gridCols; ++col)
            {
                if (!isF(row, col))
                {
                    continue;
                }
                Ogre::Real x0 = (Ogre::Real)col * cellSz, x1 = x0 + cellSz, z0 = (Ogre::Real)row * cellSz, z1 = z0 + cellSz, y = floorY;
                if (!isF(row + 1, col))
                {
                    this->addFloorQuad({x0, y - cD, z1 - cW}, {x1, y - cD, z1 - cW}, {x1, y, z1 - cW}, {x0, y, z1 - cW}, x0 / uS, x1 / uS, (z1 - cW) / vS, (z1 - cW) / vS);
                    this->addFloorQuad({x0, y, z1 - cW}, {x1, y, z1 - cW}, {x1, y - cD, z1}, {x0, y - cD, z1}, x0 / uS, x1 / uS, (z1 - cW) / vS, z1 / vS);
                }
                if (!isF(row - 1, col))
                {
                    this->addFloorQuad({x0, y - cD, z0}, {x1, y - cD, z0}, {x1, y, z0 + cW}, {x0, y, z0 + cW}, x0 / uS, x1 / uS, z0 / vS, (z0 + cW) / vS);
                    this->addFloorQuad({x0, y, z0 + cW}, {x1, y, z0 + cW}, {x1, y - cD, z0 + cW}, {x0, y - cD, z0 + cW}, x0 / uS, x1 / uS, (z0 + cW) / vS, (z0 + cW) / vS);
                }
            }
        }
    }

    void ProceduralDungeonComponent::generateIceWallBevel(const std::vector<std::vector<CellType>>&, int, int, Ogre::Real, Ogre::Real, Ogre::Real, Ogre::Real, const Ogre::Vector2&)
    {
    }

    float ProceduralDungeonComponent::caveHeightNoise(int col, int row, int seed)
    {
        uint32_t x = static_cast<uint32_t>(col * 73856093 ^ row * 19349663 ^ seed * 83492791);
        x ^= (x >> 16);
        x *= 0x45d9f3bu;
        x ^= (x >> 16);
        return (float)(x & 0xFFFFu) / 32767.5f - 1.0f;
    }

    void ProceduralDungeonComponent::generateStalactites(const std::vector<std::vector<CellType>>& grid, int gridCols, int gridRows, Ogre::Real cellSz, Ogre::Real floorY, Ogre::Real wallH, int seed, bool isCave, Ogre::Real jitter)
    {
        const Ogre::Real baseY = floorY + wallH;
        const Ogre::Real maxDroop = wallH * jitter;
        const Ogre::Real minL = wallH * 0.10f;
        const Ogre::Real maxL = wallH * 0.35f;
        const Ogre::Real maxW = cellSz * 0.25f;
        const Ogre::Real uS = maxValue(0.001f, this->wallUVTiling->getVector2().x);
        const Ogre::Real vS = maxValue(0.001f, this->wallUVTiling->getVector2().y);

        for (int row = 0; row < gridRows; ++row)
        {
            for (int col = 0; col < gridCols; ++col)
            {
                if (grid[row][col] != CellType::ROOM)
                {
                    continue;
                }

                float n = caveHeightNoise(col, row, seed);
                if (n < 0.5f)
                {
                    continue;
                }

                Ogre::Real cx = (col + 0.5f) * cellSz;
                Ogre::Real cz = (row + 0.5f) * cellSz;

                // In cave mode: compute actual ceiling Y at this cell center
                // using the same formula as generateCaveCeilingGeometry
                Ogre::Real ceilY;
                if (isCave)
                {
                    // Average the 4 corners to get center ceiling height
                    auto ceilH = [&](int c, int r) -> Ogre::Real
                    {
                        return baseY - ((caveHeightNoise(c, r, seed + 999) + 1.0f) * 0.5f) * maxDroop;
                    };
                    ceilY = (ceilH(col, row) + ceilH(col + 1, row) + ceilH(col, row + 1) + ceilH(col + 1, row + 1)) * 0.25f;
                }
                else
                {
                    ceilY = baseY;
                }

                float n2 = caveHeightNoise(col + 1, row + 1, seed + 1);
                float n3 = caveHeightNoise(col - 1, row - 1, seed + 2);
                Ogre::Real len = minL + (n + 1.0f) * 0.5f * (maxL - minL);
                Ogre::Real hw = maxW * (0.4f + (n3 + 1.0f) * 0.3f);
                Ogre::Real tipY = ceilY - len;
                Ogre::Real ox = n2 * cellSz * 0.15f;

                this->addWallQuad({cx + ox - hw, ceilY, cz}, {cx + ox + hw, ceilY, cz}, {cx + ox, tipY, cz}, {cx + ox, tipY, cz}, {0, 0, 1}, 0, hw * 2 / uS, 0, len / vS);
                this->addWallQuad({cx + ox, ceilY, cz - hw}, {cx + ox, ceilY, cz + hw}, {cx + ox, tipY, cz}, {cx + ox, tipY, cz}, {1, 0, 0}, 0, hw * 2 / uS, 0, len / vS);
            }
        }
    }

    void ProceduralDungeonComponent::generateCaveFloorGeometry(const std::vector<std::vector<CellType>>& grid, int gridCols, int gridRows, Ogre::Real cellSz, Ogre::Real floorY, const Ogre::Vector2& uvTile, int seed, Ogre::Real jitter)
    {
        const Ogre::Real uS = maxValue(0.001f, uvTile.x);
        const Ogre::Real vS = maxValue(0.001f, uvTile.y);
        const Ogre::Real maxBump = cellSz * jitter;

        for (int row = 0; row < gridRows; ++row)
        {
            for (int col = 0; col < gridCols; ++col)
            {
                if (grid[row][col] == CellType::EMPTY)
                {
                    continue;
                }
                Ogre::Real x0 = col * cellSz, x1 = x0 + cellSz;
                Ogre::Real z0 = row * cellSz, z1 = z0 + cellSz;
                Ogre::Real uU = x0 / uS, uV = x1 / uS, vU = z0 / vS, vV = z1 / vS;

                // Per-corner Y displacement — always positive (bumps up from floor)
                auto h = [&](int c, int r) -> Ogre::Real
                {
                    return floorY + ((caveHeightNoise(c, r, seed) + 1.0f) * 0.5f) * maxBump;
                };

                Ogre::Vector3 v0(x0, h(col, row + 1), z1);
                Ogre::Vector3 v1(x1, h(col + 1, row + 1), z1);
                Ogre::Vector3 v2(x1, h(col + 1, row), z0);
                Ogre::Vector3 v3(x0, h(col, row), z0);

                Ogre::Vector3 n = (v1 - v0).crossProduct(v3 - v0);
                n.normalise();

                pushDoubleQuad(this->floorVertices, this->floorIndices, this->currentFloorVertexIndex, v0, v1, v2, v3, n, uU, uV, vU, vV);
            }
        }
    }

    void ProceduralDungeonComponent::generateCaveCeilingGeometry(const std::vector<std::vector<CellType>>& grid, int gridCols, int gridRows, Ogre::Real cellSz, Ogre::Real floorY, Ogre::Real wallH, const Ogre::Vector2& uvTile, int seed,
        Ogre::Real jitter)
    {
        const Ogre::Real uS = maxValue(0.001f, uvTile.x);
        const Ogre::Real vS = maxValue(0.001f, uvTile.y);
        const Ogre::Real baseY = floorY + wallH;
        const Ogre::Real maxDroop = wallH * jitter;

        for (int row = 0; row < gridRows; ++row)
        {
            for (int col = 0; col < gridCols; ++col)
            {
                if (grid[row][col] == CellType::EMPTY)
                {
                    continue;
                }
                Ogre::Real x0 = col * cellSz, x1 = x0 + cellSz;
                Ogre::Real z0 = row * cellSz, z1 = z0 + cellSz;

                // Ceiling droops downward
                auto h = [&](int c, int r) -> Ogre::Real
                {
                    return baseY - ((caveHeightNoise(c, r, seed + 999) + 1.0f) * 0.5f) * maxDroop;
                };

                // Ceiling winding: same as generateCeilingGeometry
                Ogre::Vector3 v0(x0, h(col, row), z0);
                Ogre::Vector3 v1(x1, h(col + 1, row), z0);
                Ogre::Vector3 v2(x1, h(col + 1, row + 1), z1);
                Ogre::Vector3 v3(x0, h(col, row + 1), z1);

                Ogre::Vector3 n = (v1 - v0).crossProduct(v3 - v0);
                n.normalise();
                n = -n; // ceiling faces downward

                pushDoubleQuad(this->ceilVertices, this->ceilIndices, this->currentCeilVertexIndex, v0, v1, v2, v3, n, x0 / uS, x1 / uS, z0 / vS, z1 / vS);
            }
        }
    }

    void ProceduralDungeonComponent::generateCaveWallGeometry(const std::vector<std::vector<CellType>>& grid, int gridCols, int gridRows, Ogre::Real cellSz, Ogre::Real floorY, Ogre::Real wallH, const Ogre::Vector2& uvTile, int seed,
        Ogre::Real jitter)
    {
        const Ogre::Real uS = maxValue(0.001f, uvTile.x);
        const Ogre::Real vS = maxValue(0.001f, uvTile.y);
        const Ogre::Real baseY = floorY + wallH;
        const Ogre::Real maxBump = cellSz * jitter;
        const Ogre::Real maxDroop = wallH * jitter;

        auto isF = [&](int r, int c)
        {
            return r >= 0 && r < gridRows && c >= 0 && c < gridCols && grid[r][c] != CellType::EMPTY;
        };

        // Floor height at a grid corner (col,row) — matches generateCaveFloorGeometry
        auto floorH = [&](int c, int r) -> Ogre::Real
        {
            return floorY + ((caveHeightNoise(c, r, seed) + 1.0f) * 0.5f) * maxBump;
        };
        // Ceiling height at a grid corner — matches generateCaveCeilingGeometry
        auto ceilH = [&](int c, int r) -> Ogre::Real
        {
            return baseY - ((caveHeightNoise(c, r, seed + 999) + 1.0f) * 0.5f) * maxDroop;
        };

        for (int row = 0; row < gridRows; ++row)
        {
            for (int col = 0; col < gridCols; ++col)
            {
                if (!isF(row, col))
                {
                    continue;
                }
                Ogre::Real x0 = col * cellSz, x1 = x0 + cellSz;
                Ogre::Real z0 = row * cellSz, z1 = z0 + cellSz;
                Ogre::Real uW = cellSz / uS;

                // North (+Z): corners at (col,row+1) and (col+1,row+1)
                if (!isF(row + 1, col))
                {
                    Ogre::Real yb0 = floorH(col, row + 1);
                    Ogre::Real yb1 = floorH(col + 1, row + 1);
                    Ogre::Real yt0 = ceilH(col, row + 1);
                    Ogre::Real yt1 = ceilH(col + 1, row + 1);
                    Ogre::Real vT0 = (yt0 - yb0) / vS, vT1 = (yt1 - yb1) / vS;
                    // Average vT for the quad UV
                    Ogre::Real vT = (vT0 + vT1) * 0.5f;
                    pushDoubleQuad(this->wallVertices, this->wallIndices, this->currentWallVertexIndex, {x0, yb0, z1}, {x1, yb1, z1}, {x1, yt1, z1}, {x0, yt0, z1}, {0, 0, 1}, 0, uW, 0, vT);
                }
                // South (-Z): corners at (col,row) and (col+1,row)
                if (!isF(row - 1, col))
                {
                    Ogre::Real yb0 = floorH(col + 1, row);
                    Ogre::Real yb1 = floorH(col, row);
                    Ogre::Real yt0 = ceilH(col + 1, row);
                    Ogre::Real yt1 = ceilH(col, row);
                    Ogre::Real vT = ((yt0 - yb0) + (yt1 - yb1)) * 0.5f / vS;
                    pushDoubleQuad(this->wallVertices, this->wallIndices, this->currentWallVertexIndex, {x1, yb0, z0}, {x0, yb1, z0}, {x0, yt1, z0}, {x1, yt0, z0}, {0, 0, -1}, 0, uW, 0, vT);
                }
                // East (+X): corners at (col+1,row) and (col+1,row+1)
                if (!isF(row, col + 1))
                {
                    Ogre::Real yb0 = floorH(col + 1, row + 1);
                    Ogre::Real yb1 = floorH(col + 1, row);
                    Ogre::Real yt0 = ceilH(col + 1, row + 1);
                    Ogre::Real yt1 = ceilH(col + 1, row);
                    Ogre::Real vT = ((yt0 - yb0) + (yt1 - yb1)) * 0.5f / vS;
                    pushDoubleQuad(this->wallVertices, this->wallIndices, this->currentWallVertexIndex, {x1, yb0, z1}, {x1, yb1, z0}, {x1, yt1, z0}, {x1, yt0, z1}, {1, 0, 0}, 0, uW, 0, vT);
                }
                // West (-X): corners at (col,row) and (col,row+1)
                if (!isF(row, col - 1))
                {
                    Ogre::Real yb0 = floorH(col, row);
                    Ogre::Real yb1 = floorH(col, row + 1);
                    Ogre::Real yt0 = ceilH(col, row);
                    Ogre::Real yt1 = ceilH(col, row + 1);
                    Ogre::Real vT = ((yt0 - yb0) + (yt1 - yb1)) * 0.5f / vS;
                    pushDoubleQuad(this->wallVertices, this->wallIndices, this->currentWallVertexIndex, {x0, yb0, z0}, {x0, yb1, z1}, {x0, yt1, z1}, {x0, yt0, z0}, {-1, 0, 0}, 0, uW, 0, vT);
                }
            }
        }
    }

    void ProceduralDungeonComponent::addWallQuadWithWindow(const Ogre::Vector3& bl, const Ogre::Vector3& br, // bottom-left, bottom-right
        const Ogre::Vector3& tl, const Ogre::Vector3& tr,                                                    // top-left, top-right
        const Ogre::Vector3& normal, Ogre::Real wallH, Ogre::Real uW, Ogre::Real vT, Ogre::Real winW, Ogre::Real winH, Ogre::Real sill)
    {
        // winW, winH, sill are fractions (0..1) of cell width / wall height
        const Ogre::Real wl = 0.5f - winW * 0.5f; // left edge of window (fraction)
        const Ogre::Real wr = 0.5f + winW * 0.5f; // right edge
        const Ogre::Real wb = sill;               // bottom of window (fraction of height)
        const Ogre::Real wt = sill + winH;        // top of window

        // Helper: lerp between two vec3
        auto lerp = [](const Ogre::Vector3& a, const Ogre::Vector3& b, Ogre::Real t)
        {
            return a + (b - a) * t;
        };

        // 4 corner positions of window hole
        Ogre::Vector3 wbl = lerp(lerp(bl, br, wl), lerp(tl, tr, wl), wb); // window bottom-left
        Ogre::Vector3 wbr = lerp(lerp(bl, br, wr), lerp(tl, tr, wr), wb); // window bottom-right
        Ogre::Vector3 wtl = lerp(lerp(bl, br, wl), lerp(tl, tr, wl), wt); // window top-left
        Ogre::Vector3 wtr = lerp(lerp(bl, br, wr), lerp(tl, tr, wr), wt); // window top-right

        // Intermediate row points
        Ogre::Vector3 mll = lerp(bl, br, wl); // mid-left bottom
        Ogre::Vector3 mlr = lerp(bl, br, wr); // mid-right bottom
        Ogre::Vector3 mhl = lerp(tl, tr, wl); // mid-left top
        Ogre::Vector3 mhr = lerp(tl, tr, wr); // mid-right top

        // Bottom strip (full width, floor to sill)
        this->addWallQuad(bl, br, lerp(br, tr, wb), lerp(bl, tl, wb), normal, 0, uW, 0, vT * wb);
        // Left strip (sill to window top)
        this->addWallQuad(lerp(bl, tl, wb), lerp(bl, tl, wt), wtl, wbl, normal, 0, uW * wl, vT * wb, vT * wt);
        // Right strip
        this->addWallQuad(wbr, wtr, lerp(br, tr, wt), lerp(br, tr, wb), normal, uW * wr, uW, vT * wb, vT * wt);
        // Top strip (window top to wall top)
        this->addWallQuad(lerp(bl, tl, wt), lerp(br, tr, wt), tr, tl, normal, 0, uW, vT * wt, vT);
        // Window sill (horizontal ledge at bottom of window, single-sided facing up)
        this->addWallTopCapQuad(wbl, wbr, wbr + normal * 0.05f, wbl + normal * 0.05f, uW * wl, uW * wr, 0, 0.05f);
    }

    // Returns the single staircase cell for a given level, or {-1,-1} if none
    std::pair<int, int> ProceduralDungeonComponent::pickStaircaseCell(const std::vector<std::vector<CellType>>& grid, int gridCols, int gridRows, int levelIndex) const
    {
        const Ogre::Real prob = this->stairsProbability->getReal();

        // Get the next level's grid for cross-level validation
        const std::vector<std::vector<CellType>>* nextGrid = nullptr;
        if (levelIndex + 1 < (int)this->levelGrids.size())
        {
            nextGrid = &this->levelGrids[levelIndex + 1];
        }

        auto isF = [&](const std::vector<std::vector<CellType>>& g, int r, int c) -> bool
        {
            return r >= 0 && r < gridRows && c >= 0 && c < gridCols && g[r][c] != CellType::EMPTY;
        };

        // A cell is safe for stairs if it is interior on its own grid:
        // all 4 cardinal neighbours must be non-empty so no wall face borders the cell
        auto isInterior = [&](const std::vector<std::vector<CellType>>& g, int r, int c) -> bool
        {
            return isF(g, r - 1, c) && isF(g, r + 1, c) && isF(g, r, c - 1) && isF(g, r, c + 1);
        };

        for (int row = 1; row < gridRows - 1; ++row)
        {
            for (int col = 1; col < gridCols - 1; ++col)
            {
                // Must be a room cell on current level
                if (grid[row][col] != CellType::ROOM)
                {
                    continue;
                }

                // Must be interior on current level — not touching any wall
                if (!isInterior(grid, row, col))
                {
                    continue;
                }

                // Must also be a valid non-empty cell on the next level
                // so the upper landing is inside the upper dungeon, not floating outside
                if (nextGrid && !isF(*nextGrid, row, col))
                {
                    continue;
                }

                // Must also be interior on the next level — landing not at a wall edge
                if (nextGrid && !isInterior(*nextGrid, row, col))
                {
                    continue;
                }

                // Deterministic hash
                uint32_t h = static_cast<uint32_t>(row * 73856093 ^ col * 19349663 ^ levelIndex * 41111111 ^ this->seed->getInt());
                h ^= h >> 16;
                h *= 0x45d9f3bu;
                h ^= h >> 16;
                float r = static_cast<float>(h & 0xFFFFu) / 65535.0f;

                if (r < prob)
                {
                    return {col, row};
                }
            }
        }
        return {-1, -1};
    }

    void ProceduralDungeonComponent::generateStaircaseGeometry(const DungeonStaircase& stair, Ogre::Real cellSz, Ogre::Real totalHeight, const Ogre::Vector2& uvTile)
    {
        if (false == this->addStairs->getBool())
        {
            return;
        }

        const Ogre::Real uS = std::max(0.001f, uvTile.x);
        const Ogre::Real vS = std::max(0.001f, uvTile.y);
        const int steps = std::max(2, (int)(1.0f / this->stairHeight->getReal()));
        const Ogre::Real stepH = totalHeight / static_cast<Ogre::Real>(steps);
        const Ogre::Real stepD = cellSz / static_cast<Ogre::Real>(steps);
        const Ogre::Real stairW = cellSz * 0.7f;
        const Ogre::Real hw = stairW * 0.5f;

        // Staircase sits exactly within its hole cell — no offset, fits z0..z0+cellSz
        const Ogre::Real baseX = stair.worldPos.x; // cell center X
        const Ogre::Real baseY = stair.worldPos.y; // lower level floor Y
        const Ogre::Real baseZ = stair.worldPos.z; // cell front edge (z0)
        const Ogre::Real topY = baseY + totalHeight;

        const Ogre::Real x0 = baseX - hw;
        const Ogre::Real x1 = baseX + hw;
        const Ogre::Real endZ = baseZ + cellSz; // cell back edge (z1)
        const Ogre::Real uW = stairW / uS;
        const Ogre::Real vFull = totalHeight / vS;

        // ---- Steps ----
        for (int s = 0; s < steps; ++s)
        {
            const Ogre::Real sy0 = baseY + s * stepH;
            const Ogre::Real sy1 = baseY + (s + 1) * stepH;
            const Ogre::Real sz0 = baseZ + s * stepD;
            const Ogre::Real sz1 = baseZ + (s + 1) * stepD;
            const Ogre::Real vH = stepH / vS;
            const Ogre::Real vD = stepD / vS;

            // Tread — horizontal top of step
            this->addWallTopCapQuad({x0, sy1, sz0}, {x1, sy1, sz0}, {x1, sy1, sz1}, {x0, sy1, sz1}, 0, uW, 0, vD);

            // Riser — vertical front face
            this->addWallQuad({x0, sy0, sz0}, {x1, sy0, sz0}, {x1, sy1, sz0}, {x0, sy1, sz0}, {0, 0, -1}, 0, uW, 0, vH);
        }

        // ---- Back wall — full height, at back edge of cell ----
        this->addWallQuad({x1, baseY, endZ}, {x0, baseY, endZ}, {x0, topY, endZ}, {x1, topY, endZ}, {0, 0, 1}, 0, uW, 0, vFull);

        // ---- Left side wall — stair-profile per step (floor to step top) ----
        for (int s = 0; s < steps; ++s)
        {
            const Ogre::Real sy1 = baseY + (s + 1) * stepH;
            const Ogre::Real sz0 = baseZ + s * stepD;
            const Ogre::Real sz1 = baseZ + (s + 1) * stepD;
            const Ogre::Real vH = (s + 1) * stepH / vS;
            const Ogre::Real ud0 = (sz0 - baseZ) / uS;
            const Ogre::Real ud1 = (sz1 - baseZ) / uS;

            this->addWallQuad({x0, baseY, sz0}, {x0, baseY, sz1}, {x0, sy1, sz1}, {x0, sy1, sz0}, {-1, 0, 0}, ud0, ud1, 0, vH);
        }

        // ---- Right side wall — mirror ----
        for (int s = 0; s < steps; ++s)
        {
            const Ogre::Real sy1 = baseY + (s + 1) * stepH;
            const Ogre::Real sz0 = baseZ + s * stepD;
            const Ogre::Real sz1 = baseZ + (s + 1) * stepD;
            const Ogre::Real vH = (s + 1) * stepH / vS;
            const Ogre::Real ud0 = (sz0 - baseZ) / uS;
            const Ogre::Real ud1 = (sz1 - baseZ) / uS;

            this->addWallQuad({x1, baseY, sz1}, {x1, baseY, sz0}, {x1, sy1, sz0}, {x1, sy1, sz1}, {1, 0, 0}, ud0, ud1, 0, vH);
        }

        // ---- Under-stair bottom face — closes the staircase box from below ----
        pushQuad(this->wallVertices, this->wallIndices, this->currentWallVertexIndex, {x0, baseY, baseZ}, {x1, baseY, baseZ}, {x1, baseY, endZ}, {x0, baseY, endZ}, -Ogre::Vector3::UNIT_Y, 0, uW, 0, cellSz / vS);
    }

    void ProceduralDungeonComponent::addFloorQuad(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& v3, Ogre::Real u0, Ogre::Real u1, Ogre::Real vv0, Ogre::Real vv1)
    {
        pushDoubleQuad(this->floorVertices, this->floorIndices, this->currentFloorVertexIndex, v0, v1, v2, v3, Ogre::Vector3::UNIT_Y, u0, u1, vv0, vv1);
    }

    void ProceduralDungeonComponent::addWallQuad(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& v3, const Ogre::Vector3& normal, Ogre::Real u0, Ogre::Real u1, Ogre::Real vv0, Ogre::Real vv1)
    {
        pushDoubleQuad(this->wallVertices, this->wallIndices, this->currentWallVertexIndex, v0, v1, v2, v3, normal, u0, u1, vv0, vv1);
    }

    void ProceduralDungeonComponent::addCeilingQuad(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& v3, Ogre::Real u0, Ogre::Real u1, Ogre::Real vv0, Ogre::Real vv1)
    {
        pushDoubleQuad(this->ceilVertices, this->ceilIndices, this->currentCeilVertexIndex, v0, v1, v2, v3, -Ogre::Vector3::UNIT_Y, u0, u1, vv0, vv1);
    }

    void ProceduralDungeonComponent::addWallTopCapQuad(const Ogre::Vector3& v0, const Ogre::Vector3& v1, const Ogre::Vector3& v2, const Ogre::Vector3& v3, Ogre::Real u0, Ogre::Real u1, Ogre::Real vv0, Ogre::Real vv1)
    {
        pushDoubleQuad(this->wallVertices, this->wallIndices, this->currentWallVertexIndex, v0, v1, v2, v3, Ogre::Vector3::UNIT_Y, u0, u1, vv0, vv1);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Mesh (VAO)
    ///////////////////////////////////////////////////////////////////////////////////////////////

    static Ogre::VertexBufferPacked* buildVBO(Ogre::VaoManager* vm, const std::vector<float>& sv, size_t nv)
    {
        Ogre::VertexElement2Vec e;
        e.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
        e.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));
        e.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT4, Ogre::VES_TANGENT));
        e.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));
        const size_t d = 12;
        float* vd = (float*)OGRE_MALLOC_SIMD(nv * d * sizeof(float), Ogre::MEMCATEGORY_GEOMETRY);
        for (size_t i = 0; i < nv; ++i)
        {
            size_t s = i * 8, dst = i * d;
            vd[dst + 0] = sv[s + 0];
            vd[dst + 1] = sv[s + 1];
            vd[dst + 2] = sv[s + 2];
            Ogre::Vector3 n(sv[s + 3], sv[s + 4], sv[s + 5]);
            vd[dst + 3] = n.x;
            vd[dst + 4] = n.y;
            vd[dst + 5] = n.z;
            Ogre::Vector3 t = (std::abs(n.y) < 0.9f) ? Ogre::Vector3::UNIT_Y.crossProduct(n) : n.crossProduct(Ogre::Vector3::UNIT_X);
            t.normalise();
            vd[dst + 6] = t.x;
            vd[dst + 7] = t.y;
            vd[dst + 8] = t.z;
            vd[dst + 9] = 1.0f;
            vd[dst + 10] = sv[s + 6];
            vd[dst + 11] = sv[s + 7];
        }
        return vm->createVertexBuffer(e, nv, Ogre::BT_IMMUTABLE, vd, true);
    }

    static Ogre::IndexBufferPacked* buildIBO(Ogre::VaoManager* vm, const std::vector<Ogre::uint32>& si)
    {
        Ogre::uint32* id = (Ogre::uint32*)OGRE_MALLOC_SIMD(si.size() * sizeof(Ogre::uint32), Ogre::MEMCATEGORY_GEOMETRY);
        memcpy(id, si.data(), si.size() * sizeof(Ogre::uint32));
        return vm->createIndexBuffer(Ogre::IndexBufferPacked::IT_32BIT, si.size(), Ogre::BT_IMMUTABLE, id, true);
    }

    Ogre::VertexArrayObject* ProceduralDungeonComponent::createDummyVao(Ogre::VaoManager* vm)
    {
        Ogre::VertexElement2Vec e;
        e.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
        e.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_NORMAL));
        e.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT4, Ogre::VES_TANGENT));
        e.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));
        float* vd = (float*)OGRE_MALLOC_SIMD(3 * 12 * sizeof(float), Ogre::MEMCATEGORY_GEOMETRY);
        memset(vd, 0, 3 * 12 * sizeof(float));
        vd[9] = 1.0f;
        vd[9 + 12] = 1.0f;
        vd[9 + 24] = 1.0f;
        Ogre::VertexBufferPacked* vb = vm->createVertexBuffer(e, 3, Ogre::BT_IMMUTABLE, vd, true);
        Ogre::uint32* id = (Ogre::uint32*)OGRE_MALLOC_SIMD(3 * sizeof(Ogre::uint32), Ogre::MEMCATEGORY_GEOMETRY);
        id[0] = 0;
        id[1] = 1;
        id[2] = 2;
        Ogre::IndexBufferPacked* ib = vm->createIndexBuffer(Ogre::IndexBufferPacked::IT_32BIT, 3, Ogre::BT_IMMUTABLE, id, true);
        Ogre::VertexBufferPackedVec vbv;
        vbv.push_back(vb);
        return vm->createVertexArrayObject(vbv, ib, Ogre::OT_TRIANGLE_LIST);
    }

    void ProceduralDungeonComponent::createDungeonMesh(void)
    {
        if (this->cachedNumFloorVertices == 0 && this->cachedNumWallVertices == 0)
        {
            return;
        }
        auto fv = this->cachedFloorVertices, wv = this->cachedWallVertices, cv = this->cachedCeilVertices;
        auto fi = this->cachedFloorIndices, wi = this->cachedWallIndices, ci = this->cachedCeilIndices;
        size_t fvc = this->cachedNumFloorVertices, wvc = this->cachedNumWallVertices, cvc = this->cachedNumCeilVertices;
        Ogre::Vector3 origin = this->dungeonOrigin;
        this->destroyDungeonMesh();
        GraphicsModule::RenderCommand cmd = [this, fv, fi, fvc, wv, wi, wvc, cv, ci, cvc, origin]()
        {
            this->createDungeonMeshInternal(fv, fi, fvc, wv, wi, wvc, cv, ci, cvc, origin);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "ProceduralDungeonComponent::createDungeonMesh");
    }

    void ProceduralDungeonComponent::createDungeonMeshInternal(const std::vector<float>& fv, const std::vector<Ogre::uint32>& fi, size_t fvc, const std::vector<float>& wv, const std::vector<Ogre::uint32>& wi, size_t wvc, const std::vector<float>& cv,
        const std::vector<Ogre::uint32>& ci, size_t cvc, const Ogre::Vector3& origin)
    {
        Ogre::Root* root = Ogre::Root::getSingletonPtr();
        Ogre::VaoManager* vm = root->getRenderSystem()->getVaoManager();
        const Ogre::String meshName = "DungeonMesh_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
        this->dungeonMesh = Ogre::MeshManager::getSingleton().createManual(meshName, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

        Ogre::Vector3 minB(std::numeric_limits<float>::max()), maxB(-std::numeric_limits<float>::max());
        auto updateB = [&](const std::vector<float>& v, size_t n)
        {
            for (size_t i = 0; i < n; ++i)
            {
                size_t o = i * 8;
                Ogre::Vector3 p(v[o], v[o + 1], v[o + 2]);
                minB.makeFloor(p);
                maxB.makeCeil(p);
            }
        };

        auto makeSub = [&](const std::vector<float>& v, const std::vector<Ogre::uint32>& idx, size_t n) -> Ogre::SubMesh*
        {
            Ogre::SubMesh* sm = this->dungeonMesh->createSubMesh();
            if (n == 0 || idx.empty())
            {
                auto* dv = this->createDummyVao(vm);
                sm->mVao[Ogre::VpNormal].push_back(dv);
                sm->mVao[Ogre::VpShadow].push_back(dv);
                return sm;
            }
            updateB(v, n);
            Ogre::VertexBufferPackedVec vbv;
            vbv.push_back(buildVBO(vm, v, n));
            auto* vao = vm->createVertexArrayObject(vbv, buildIBO(vm, idx), Ogre::OT_TRIANGLE_LIST);
            sm->mVao[Ogre::VpNormal].push_back(vao);
            sm->mVao[Ogre::VpShadow].push_back(vao);
            return sm;
        };

        makeSub(fv, fi, fvc);
        makeSub(wv, wi, wvc);
        makeSub(cv, ci, cvc);

        if (minB.x > maxB.x)
        {
            minB = Ogre::Vector3(-1, -1, -1);
            maxB = Ogre::Vector3(1, 1, 1);
        }
        Ogre::Aabb bounds;
        bounds.setExtents(minB, maxB);
        this->dungeonMesh->_setBounds(bounds, false);
        this->dungeonMesh->_setBoundingSphereRadius(bounds.getRadius());

        this->dungeonItem = this->gameObjectPtr->getSceneManager()->createItem(this->dungeonMesh, this->gameObjectPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC);

        auto applyDb = [&](int idx, const Ogre::String& name)
        {
            if ((int)this->dungeonItem->getNumSubItems() <= idx || name.empty())
            {
                return;
            }
            auto* db = root->getHlmsManager()->getDatablockNoDefault(name);
            if (db)
            {
                this->dungeonItem->getSubItem(idx)->setDatablock(db);
            }
        };
        applyDb(0, this->floorDatablock->getString());
        applyDb(1, this->wallDatablock->getString());
        applyDb(2, this->ceilingDatablock->getString());

        if (!this->originPositionSet)
        {
            this->originPositionSet = true;
            this->gameObjectPtr->getSceneNode()->setPosition(origin);
        }
        this->gameObjectPtr->getSceneNode()->attachObject(this->dungeonItem);
        this->gameObjectPtr->setDoNotDestroyMovableObject(true);
        this->gameObjectPtr->init(this->dungeonItem);
    }

    void ProceduralDungeonComponent::destroyDungeonMesh(void)
    {
        if (!this->dungeonItem && !this->dungeonMesh)
        {
            return;
        }
        GraphicsModule::RenderCommand cmd = [this]()
        {
            if (this->dungeonItem)
            {
                if (this->dungeonItem->getParentSceneNode())
                {
                    this->dungeonItem->getParentSceneNode()->detachObject(this->dungeonItem);
                }
                this->gameObjectPtr->getSceneManager()->destroyItem(this->dungeonItem);
                this->dungeonItem = nullptr;
                this->gameObjectPtr->nullMovableObject();
            }
            if (this->dungeonMesh)
            {
                Ogre::MeshManager::getSingleton().remove(this->dungeonMesh->getHandle());
                this->dungeonMesh.reset();
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "ProceduralDungeonComponent::destroyDungeonMesh");
    }

    void ProceduralDungeonComponent::destroyPreviewMesh(void)
    {
        if (!this->previewItem && !this->previewMesh)
        {
            return;
        }
        GraphicsModule::RenderCommand cmd = [this]()
        {
            if (this->previewItem)
            {
                if (this->previewItem->getParentSceneNode())
                {
                    this->previewItem->getParentSceneNode()->detachObject(this->previewItem);
                }
                this->gameObjectPtr->getSceneManager()->destroyItem(this->previewItem);
                this->previewItem = nullptr;
            }
            if (this->previewMesh)
            {
                Ogre::MeshManager::getSingleton().remove(this->previewMesh->getHandle());
                this->previewMesh.reset();
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "ProceduralDungeonComponent::destroyPreviewMesh");
    }

    void ProceduralDungeonComponent::updatePreviewMesh(const Ogre::Vector3& position)
    {
        const Ogre::Real w = this->dungeonWidth->getReal(), d = this->dungeonDepth->getReal(), wH = this->wallHeight->getReal();
        const Ogre::Real fY = this->getGroundHeight(position);
        std::vector<float> pv;
        std::vector<Ogre::uint32> pi;
        Ogre::uint32 pvi = 0;
        auto pV = [&](float x, float y, float z, float nx, float ny, float nz)
        {
            pv.push_back(x);
            pv.push_back(y);
            pv.push_back(z);
            pv.push_back(nx);
            pv.push_back(ny);
            pv.push_back(nz);
            pv.push_back(1);
            pv.push_back(0);
            ++pvi;
        };
        auto pT = [&](Ogre::uint32 a, Ogre::uint32 b, Ogre::uint32 c)
        {
            pi.push_back(a);
            pi.push_back(b);
            pi.push_back(c);
        };
        Ogre::uint32 b0 = pvi;
        pV(0, fY, 0, 0, 1, 0);
        pV(w, fY, 0, 0, 1, 0);
        pV(w, fY, d, 0, 1, 0);
        pV(0, fY, d, 0, 1, 0);
        pT(b0, b0 + 1, b0 + 2);
        pT(b0, b0 + 2, b0 + 3);
        b0 = pvi;
        pV(0, fY, d, 0, 0, 1);
        pV(w, fY, d, 0, 0, 1);
        pV(w, fY + wH, d, 0, 0, 1);
        pV(0, fY + wH, d, 0, 0, 1);
        pT(b0, b0 + 1, b0 + 2);
        pT(b0, b0 + 2, b0 + 3);
        b0 = pvi;
        pV(w, fY, 0, 0, 0, -1);
        pV(0, fY, 0, 0, 0, -1);
        pV(0, fY + wH, 0, 0, 0, -1);
        pV(w, fY + wH, 0, 0, 0, -1);
        pT(b0, b0 + 1, b0 + 2);
        pT(b0, b0 + 2, b0 + 3);
        b0 = pvi;
        pV(w, fY, d, 1, 0, 0);
        pV(w, fY, 0, 1, 0, 0);
        pV(w, fY + wH, 0, 1, 0, 0);
        pV(w, fY + wH, d, 1, 0, 0);
        pT(b0, b0 + 1, b0 + 2);
        pT(b0, b0 + 2, b0 + 3);
        b0 = pvi;
        pV(0, fY, 0, -1, 0, 0);
        pV(0, fY, d, -1, 0, 0);
        pV(0, fY + wH, d, -1, 0, 0);
        pV(0, fY + wH, 0, -1, 0, 0);
        pT(b0, b0 + 1, b0 + 2);
        pT(b0, b0 + 2, b0 + 3);
        size_t tv = pvi;
        Ogre::Vector3 pp = position;
        pp.y = fY;
        GraphicsModule::RenderCommand cmd = [this, pv, pi, tv, pp, w, d, wH]()
        {
            if (this->previewItem)
            {
                if (this->previewItem->getParentSceneNode())
                {
                    this->previewItem->getParentSceneNode()->detachObject(this->previewItem);
                }
                this->gameObjectPtr->getSceneManager()->destroyItem(this->previewItem);
                this->previewItem = nullptr;
            }
            if (this->previewMesh)
            {
                Ogre::MeshManager::getSingleton().remove(this->previewMesh->getHandle());
                this->previewMesh.reset();
            }
            Ogre::Root* root = Ogre::Root::getSingletonPtr();
            Ogre::VaoManager* vm = root->getRenderSystem()->getVaoManager();
            const Ogre::String mn = "DungeonPreview_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());
            this->previewMesh = Ogre::MeshManager::getSingleton().createManual(mn, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
            Ogre::SubMesh* sm = this->previewMesh->createSubMesh();
            Ogre::VertexBufferPackedVec vbv;
            vbv.push_back(buildVBO(vm, pv, tv));
            auto* vao = vm->createVertexArrayObject(vbv, buildIBO(vm, pi), Ogre::OT_TRIANGLE_LIST);
            sm->mVao[Ogre::VpNormal].push_back(vao);
            sm->mVao[Ogre::VpShadow].push_back(vao);
            Ogre::Aabb bounds;
            bounds.setExtents({0, 0, 0}, {w, wH, d});
            this->previewMesh->_setBounds(bounds, false);
            this->previewMesh->_setBoundingSphereRadius(bounds.getRadius());
            this->previewItem = this->gameObjectPtr->getSceneManager()->createItem(this->previewMesh, Ogre::SCENE_DYNAMIC);
            Ogre::String dn = this->floorDatablock->getString();
            if (!dn.empty())
            {
                auto* db = root->getHlmsManager()->getDatablockNoDefault(dn);
                if (db)
                {
                    this->previewItem->getSubItem(0)->setDatablock(db);
                }
            }
            this->previewNode->setPosition(pp);
            this->previewNode->attachObject(this->previewItem);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "ProceduralDungeonComponent::updatePreviewMesh");
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Ground
    ///////////////////////////////////////////////////////////////////////////////////////////////

    Ogre::Real ProceduralDungeonComponent::getGroundHeight(const Ogre::Vector3& position)
    {
        if (!this->adaptToGround->getBool())
        {
            return position.y + this->heightOffset->getReal();
        }
        Ogre::Vector3 ro(position.x, position.y + 1000.0f, position.z);
        Ogre::Ray ray(ro, Ogre::Vector3::NEGATIVE_UNIT_Y);
        this->groundQuery->setRay(ray);
        std::vector<Ogre::MovableObject*> ex;
        if (this->dungeonItem)
        {
            ex.push_back(this->dungeonItem);
        }
        if (this->previewItem)
        {
            ex.push_back(this->previewItem);
        }
        Ogre::Vector3 hp;
        Ogre::MovableObject* ho = nullptr;
        Ogre::Real hd = 0;
        Ogre::Vector3 hn;
        bool hit = MathHelper::getInstance()->getRaycastFromPoint(this->groundQuery, AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera(), hp, (size_t&)ho, hd, hn, &ex, false);
        if (hit && ho)
        {
            return hp.y + this->heightOffset->getReal();
        }
        auto p = ray.intersects(this->groundPlane);
        if (p.first && p.second > 0)
        {
            return (ro.y - p.second) + this->heightOffset->getReal();
        }
        return position.y + this->heightOffset->getReal();
    }

    bool ProceduralDungeonComponent::raycastGround(Ogre::Real screenX, Ogre::Real screenY, Ogre::Vector3& hitPos)
    {
        Ogre::Camera* cam = AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera();
        if (!cam)
        {
            return false;
        }
        const OIS::MouseState& ms = NOWA::InputDeviceCore::getSingletonPtr()->getMouse()->getMouseState();
        std::vector<Ogre::MovableObject*> ex;
        if (this->dungeonItem)
        {
            ex.push_back(this->dungeonItem);
        }
        if (this->previewItem)
        {
            ex.push_back(this->previewItem);
        }
        Ogre::Vector3 hp;
        Ogre::MovableObject* ho = nullptr;
        Ogre::Real hd = 0;
        Ogre::Vector3 hn;
        bool hit = MathHelper::getInstance()->getRaycastFromPoint(ms.X.abs, ms.Y.abs, cam, Core::getSingletonPtr()->getOgreRenderWindow(), this->groundQuery, hp, (size_t&)ho, hd, hn, &ex, false);
        if (hit && ho)
        {
            hitPos = hp;
            return true;
        }
        Ogre::Ray r = cam->getCameraToViewportRay(screenX, screenY);
        auto p = r.intersects(this->groundPlane);
        if (p.first && p.second > 0)
        {
            hitPos = r.getPoint(p.second);
            return true;
        }
        return false;
    }

    Ogre::String ProceduralDungeonComponent::getDungeonDataFilePath(void) const
    {
        Ogre::String projectPath;
        if (!this->gameObjectPtr->getGlobal())
        {
            projectPath = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName();
        }
        else
        {
            projectPath = Core::getSingletonPtr()->getCurrentProjectPath();
        }

        Ogre::String filename = "Dungeon_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()) + ".dungeondata";
        return projectPath + "/" + filename;
    }

    bool ProceduralDungeonComponent::saveDungeonDataToFile(void)
    {
        if (this->dungeonRooms.empty() && this->cachedFloorVertices.empty())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralDungeonComponent] saveDungeonDataToFile: nothing to save, deleting file.");
            this->deleteDungeonDataFile();
            return true;
        }

        Ogre::String filePath = this->getDungeonDataFilePath();

        try
        {
            const size_t FPV = 8; // floats per vertex

            uint32_t numRooms = static_cast<uint32_t>(this->dungeonRooms.size());
            uint32_t numCorr = static_cast<uint32_t>(this->dungeonCorridors.size());
            uint32_t numFV = static_cast<uint32_t>(this->cachedNumFloorVertices);
            uint32_t numFI = static_cast<uint32_t>(this->cachedFloorIndices.size());
            uint32_t numWV = static_cast<uint32_t>(this->cachedNumWallVertices);
            uint32_t numWI = static_cast<uint32_t>(this->cachedWallIndices.size());
            uint32_t numCV = static_cast<uint32_t>(this->cachedNumCeilVertices);
            uint32_t numCI = static_cast<uint32_t>(this->cachedCeilIndices.size());

            // Header: magic(4) + version(4) + origin(12) + originSet(1) + numRooms(4) + numCorr(4)
            //         + numFV(4) + numFI(4) + numWV(4) + numWI(4) + numCV(4) + numCI(4) = 49 bytes
            const size_t HEADER_SIZE = 49;
            // Each room: id(4) + col(4) + row(4) + cols(4) + rows(4) + floorHeight(4) = 24 bytes
            const size_t ROOM_SIZE = 24;
            // Each corridor: fromId(4) + toId(4) + bendCol(4) + bendRow(4) = 16 bytes
            const size_t CORR_SIZE = 16;

            size_t totalSize =
                HEADER_SIZE + numRooms * ROOM_SIZE + numCorr * CORR_SIZE + numFV * FPV * sizeof(float) + numFI * sizeof(uint32_t) + numWV * FPV * sizeof(float) + numWI * sizeof(uint32_t) + numCV * FPV * sizeof(float) + numCI * sizeof(uint32_t);

            std::vector<unsigned char> buf(totalSize);
            size_t off = 0;

            auto write4 = [&](uint32_t v)
            {
                memcpy(&buf[off], &v, 4);
                off += 4;
            };
            auto writeF = [&](float v)
            {
                memcpy(&buf[off], &v, 4);
                off += 4;
            };

            // Header
            write4(DUNGEONDATA_MAGIC);
            write4(DUNGEONDATA_VERSION);
            writeF(this->dungeonOrigin.x);
            writeF(this->dungeonOrigin.y);
            writeF(this->dungeonOrigin.z);
            buf[off++] = this->originPositionSet ? 1u : 0u;
            write4(numRooms);
            write4(numCorr);
            write4(numFV);
            write4(numFI);
            write4(numWV);
            write4(numWI);
            write4(numCV);
            write4(numCI);

            // Rooms
            for (const auto& room : this->dungeonRooms)
            {
                write4(static_cast<uint32_t>(room.id));
                write4(static_cast<uint32_t>(room.col));
                write4(static_cast<uint32_t>(room.row));
                write4(static_cast<uint32_t>(room.cols));
                write4(static_cast<uint32_t>(room.rows));
                writeF(room.floorHeight);
            }

            // Corridors
            for (const auto& corr : this->dungeonCorridors)
            {
                write4(static_cast<uint32_t>(corr.fromRoomId));
                write4(static_cast<uint32_t>(corr.toRoomId));
                write4(static_cast<uint32_t>(corr.bendCol));
                write4(static_cast<uint32_t>(corr.bendRow));
            }

            // Floor vertex/index data
            if (numFV > 0)
            {
                memcpy(&buf[off], this->cachedFloorVertices.data(), numFV * FPV * sizeof(float));
            }
            off += numFV * FPV * sizeof(float);
            if (numFI > 0)
            {
                memcpy(&buf[off], this->cachedFloorIndices.data(), numFI * sizeof(uint32_t));
            }
            off += numFI * sizeof(uint32_t);

            // Wall vertex/index data
            if (numWV > 0)
            {
                memcpy(&buf[off], this->cachedWallVertices.data(), numWV * FPV * sizeof(float));
            }
            off += numWV * FPV * sizeof(float);
            if (numWI > 0)
            {
                memcpy(&buf[off], this->cachedWallIndices.data(), numWI * sizeof(uint32_t));
            }
            off += numWI * sizeof(uint32_t);

            // Ceiling vertex/index data
            if (numCV > 0)
            {
                memcpy(&buf[off], this->cachedCeilVertices.data(), numCV * FPV * sizeof(float));
            }
            off += numCV * FPV * sizeof(float);
            if (numCI > 0)
            {
                memcpy(&buf[off], this->cachedCeilIndices.data(), numCI * sizeof(uint32_t));
            }
            off += numCI * sizeof(uint32_t);

            std::ofstream file(filePath, std::ios::binary | std::ios::trunc);
            if (!file)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralDungeonComponent] Cannot open file for writing: " + filePath);
                return false;
            }
            file.write(reinterpret_cast<const char*>(buf.data()), static_cast<std::streamsize>(totalSize));

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralDungeonComponent] Saved dungeon data to: " + filePath);
            return true;
        }
        catch (const std::exception& e)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralDungeonComponent] Exception saving: " + Ogre::String(e.what()));
            return false;
        }
    }

    bool ProceduralDungeonComponent::loadDungeonDataFromFile(void)
    {
        Ogre::String filePath = this->getDungeonDataFilePath();

        if (!std::filesystem::exists(filePath))
        {
            return false;
        }

        try
        {
            std::ifstream file(filePath, std::ios::binary | std::ios::ate);
            if (!file)
            {
                return false;
            }

            std::streamsize fileSize = file.tellg();
            file.seekg(0, std::ios::beg);

            std::vector<unsigned char> buf(static_cast<size_t>(fileSize));
            if (!file.read(reinterpret_cast<char*>(buf.data()), fileSize))
            {
                return false;
            }

            const size_t HEADER_SIZE = 49;
            if (buf.size() < HEADER_SIZE)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralDungeonComponent] loadDungeonDataFromFile: buffer too small.");
                return false;
            }

            size_t off = 0;

            auto read4 = [&]() -> uint32_t
            {
                uint32_t v;
                memcpy(&v, &buf[off], 4);
                off += 4;
                return v;
            };
            auto readF = [&]() -> float
            {
                float v;
                memcpy(&v, &buf[off], 4);
                off += 4;
                return v;
            };

            uint32_t magic = read4();
            uint32_t version = read4();
            if (magic != DUNGEONDATA_MAGIC || version != DUNGEONDATA_VERSION)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralDungeonComponent] loadDungeonDataFromFile: bad magic/version.");
                return false;
            }

            this->dungeonOrigin.x = readF();
            this->dungeonOrigin.y = readF();
            this->dungeonOrigin.z = readF();
            this->originPositionSet = (buf[off++] != 0);
            this->hasDungeonOrigin = true;

            uint32_t numRooms = read4();
            uint32_t numCorr = read4();
            uint32_t numFV = read4();
            uint32_t numFI = read4();
            uint32_t numWV = read4();
            uint32_t numWI = read4();
            uint32_t numCV = read4();
            uint32_t numCI = read4();

            const size_t FPV = 8;

            // Rooms
            this->dungeonRooms.clear();
            this->dungeonRooms.reserve(numRooms);
            for (uint32_t i = 0; i < numRooms; ++i)
            {
                DungeonRoom room;
                room.id = static_cast<int>(read4());
                room.col = static_cast<int>(read4());
                room.row = static_cast<int>(read4());
                room.cols = static_cast<int>(read4());
                room.rows = static_cast<int>(read4());
                room.floorHeight = readF();
                this->dungeonRooms.push_back(room);
            }

            // Corridors
            this->dungeonCorridors.clear();
            this->dungeonCorridors.reserve(numCorr);
            for (uint32_t i = 0; i < numCorr; ++i)
            {
                DungeonCorridor corr;
                corr.fromRoomId = static_cast<int>(read4());
                corr.toRoomId = static_cast<int>(read4());
                corr.bendCol = static_cast<int>(read4());
                corr.bendRow = static_cast<int>(read4());
                this->dungeonCorridors.push_back(corr);
            }

            // Floor vertices
            this->cachedFloorVertices.resize(numFV * FPV);
            if (numFV > 0)
            {
                memcpy(this->cachedFloorVertices.data(), &buf[off], numFV * FPV * sizeof(float));
            }
            off += numFV * FPV * sizeof(float);
            this->cachedFloorIndices.resize(numFI);
            if (numFI > 0)
            {
                memcpy(this->cachedFloorIndices.data(), &buf[off], numFI * sizeof(uint32_t));
            }
            off += numFI * sizeof(uint32_t);
            this->cachedNumFloorVertices = numFV;

            // Wall vertices
            this->cachedWallVertices.resize(numWV * FPV);
            if (numWV > 0)
            {
                memcpy(this->cachedWallVertices.data(), &buf[off], numWV * FPV * sizeof(float));
            }
            off += numWV * FPV * sizeof(float);
            this->cachedWallIndices.resize(numWI);
            if (numWI > 0)
            {
                memcpy(this->cachedWallIndices.data(), &buf[off], numWI * sizeof(uint32_t));
            }
            off += numWI * sizeof(uint32_t);
            this->cachedNumWallVertices = numWV;

            // Ceiling vertices
            this->cachedCeilVertices.resize(numCV * FPV);
            if (numCV > 0)
            {
                memcpy(this->cachedCeilVertices.data(), &buf[off], numCV * FPV * sizeof(float));
            }
            off += numCV * FPV * sizeof(float);
            this->cachedCeilIndices.resize(numCI);
            if (numCI > 0)
            {
                memcpy(this->cachedCeilIndices.data(), &buf[off], numCI * sizeof(uint32_t));
            }
            off += numCI * sizeof(uint32_t);
            this->cachedNumCeilVertices = numCV;

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL,
                "[ProceduralDungeonComponent] Loaded dungeon data from: " + filePath + " (" + Ogre::StringConverter::toString(numRooms) + " rooms, " + Ogre::StringConverter::toString(numCorr) + " corridors)");
            return true;
        }
        catch (const std::exception& e)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralDungeonComponent] Exception loading dungeon data: " + Ogre::String(e.what()));
            return false;
        }
    }

    void ProceduralDungeonComponent::deleteDungeonDataFile(void)
    {
        Ogre::String filePath = this->getDungeonDataFilePath();

        std::filesystem::path absolutePath = std::filesystem::absolute(filePath);
        if (!std::filesystem::exists(absolutePath))
        {
            return;
        }

        std::error_code ec;
        std::filesystem::remove(absolutePath, ec);
        if (ec)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralDungeonComponent] deleteDungeonDataFile: " + ec.message());
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Undo / Redo binary serialisation
    ///////////////////////////////////////////////////////////////////////////////////////////////

    std::vector<unsigned char> ProceduralDungeonComponent::getDungeonData(void) const
    {
        std::vector<unsigned char> result;

        if (this->dungeonRooms.empty() && this->cachedFloorVertices.empty())
        {
            return result;
        }

        const size_t FPV = 8;

        uint32_t numRooms = static_cast<uint32_t>(this->dungeonRooms.size());
        uint32_t numCorr = static_cast<uint32_t>(this->dungeonCorridors.size());
        uint32_t numFV = static_cast<uint32_t>(this->cachedNumFloorVertices);
        uint32_t numFI = static_cast<uint32_t>(this->cachedFloorIndices.size());
        uint32_t numWV = static_cast<uint32_t>(this->cachedNumWallVertices);
        uint32_t numWI = static_cast<uint32_t>(this->cachedWallIndices.size());
        uint32_t numCV = static_cast<uint32_t>(this->cachedNumCeilVertices);
        uint32_t numCI = static_cast<uint32_t>(this->cachedCeilIndices.size());

        const size_t totalSize = 49 + numRooms * 24 + numCorr * 16 + numFV * FPV * sizeof(float) + numFI * sizeof(uint32_t) + numWV * FPV * sizeof(float) + numWI * sizeof(uint32_t) + numCV * FPV * sizeof(float) + numCI * sizeof(uint32_t);

        result.resize(totalSize);
        size_t off = 0;

        auto write4 = [&](uint32_t v)
        {
            memcpy(&result[off], &v, 4);
            off += 4;
        };
        auto writeF = [&](float v)
        {
            memcpy(&result[off], &v, 4);
            off += 4;
        };

        write4(DUNGEONDATA_MAGIC);
        write4(DUNGEONDATA_VERSION);
        writeF(this->dungeonOrigin.x);
        writeF(this->dungeonOrigin.y);
        writeF(this->dungeonOrigin.z);
        result[off++] = this->originPositionSet ? 1u : 0u;
        write4(numRooms);
        write4(numCorr);
        write4(numFV);
        write4(numFI);
        write4(numWV);
        write4(numWI);
        write4(numCV);
        write4(numCI);

        for (const auto& room : this->dungeonRooms)
        {
            write4(static_cast<uint32_t>(room.id));
            write4(static_cast<uint32_t>(room.col));
            write4(static_cast<uint32_t>(room.row));
            write4(static_cast<uint32_t>(room.cols));
            write4(static_cast<uint32_t>(room.rows));
            writeF(room.floorHeight);
        }
        for (const auto& corr : this->dungeonCorridors)
        {
            write4(static_cast<uint32_t>(corr.fromRoomId));
            write4(static_cast<uint32_t>(corr.toRoomId));
            write4(static_cast<uint32_t>(corr.bendCol));
            write4(static_cast<uint32_t>(corr.bendRow));
        }

        if (numFV > 0)
        {
            memcpy(&result[off], this->cachedFloorVertices.data(), numFV * FPV * sizeof(float));
        }
        off += numFV * FPV * sizeof(float);
        if (numFI > 0)
        {
            memcpy(&result[off], this->cachedFloorIndices.data(), numFI * sizeof(uint32_t));
        }
        off += numFI * sizeof(uint32_t);

        if (numWV > 0)
        {
            memcpy(&result[off], this->cachedWallVertices.data(), numWV * FPV * sizeof(float));
        }
        off += numWV * FPV * sizeof(float);
        if (numWI > 0)
        {
            memcpy(&result[off], this->cachedWallIndices.data(), numWI * sizeof(uint32_t));
        }
        off += numWI * sizeof(uint32_t);

        if (numCV > 0)
        {
            memcpy(&result[off], this->cachedCeilVertices.data(), numCV * FPV * sizeof(float));
        }
        off += numCV * FPV * sizeof(float);
        if (numCI > 0)
        {
            memcpy(&result[off], this->cachedCeilIndices.data(), numCI * sizeof(uint32_t));
        }
        off += numCI * sizeof(uint32_t);

        return result;
    }

    void ProceduralDungeonComponent::setDungeonData(const std::vector<unsigned char>& data)
    {
        this->destroyDungeonMesh();

        if (data.empty())
        {
            this->clearDungeon();
            return;
        }

        const size_t HEADER_SIZE = 49;
        if (data.size() < HEADER_SIZE)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralDungeonComponent] setDungeonData: buffer too small.");
            return;
        }

        size_t off = 0;
        auto read4 = [&]() -> uint32_t
        {
            uint32_t v;
            memcpy(&v, &data[off], 4);
            off += 4;
            return v;
        };
        auto readF = [&]() -> float
        {
            float v;
            memcpy(&v, &data[off], 4);
            off += 4;
            return v;
        };

        uint32_t magic = read4();
        uint32_t version = read4();
        if (magic != DUNGEONDATA_MAGIC || version != DUNGEONDATA_VERSION)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralDungeonComponent] setDungeonData: bad magic/version.");
            return;
        }

        this->dungeonOrigin.x = readF();
        this->dungeonOrigin.y = readF();
        this->dungeonOrigin.z = readF();
        this->originPositionSet = (data[off++] != 0);
        this->hasDungeonOrigin = true;

        uint32_t numRooms = read4(), numCorr = read4();
        uint32_t numFV = read4(), numFI = read4();
        uint32_t numWV = read4(), numWI = read4();
        uint32_t numCV = read4(), numCI = read4();

        const size_t FPV = 8;

        this->dungeonRooms.clear();
        for (uint32_t i = 0; i < numRooms; ++i)
        {
            DungeonRoom r;
            r.id = static_cast<int>(read4());
            r.col = static_cast<int>(read4());
            r.row = static_cast<int>(read4());
            r.cols = static_cast<int>(read4());
            r.rows = static_cast<int>(read4());
            r.floorHeight = readF();
            this->dungeonRooms.push_back(r);
        }

        this->dungeonCorridors.clear();
        for (uint32_t i = 0; i < numCorr; ++i)
        {
            DungeonCorridor c;
            c.fromRoomId = static_cast<int>(read4());
            c.toRoomId = static_cast<int>(read4());
            c.bendCol = static_cast<int>(read4());
            c.bendRow = static_cast<int>(read4());
            this->dungeonCorridors.push_back(c);
        }

        this->cachedFloorVertices.resize(numFV * FPV);
        if (numFV > 0)
        {
            memcpy(this->cachedFloorVertices.data(), &data[off], numFV * FPV * sizeof(float));
        }
        off += numFV * FPV * sizeof(float);
        this->cachedFloorIndices.resize(numFI);
        if (numFI > 0)
        {
            memcpy(this->cachedFloorIndices.data(), &data[off], numFI * sizeof(uint32_t));
        }
        off += numFI * sizeof(uint32_t);
        this->cachedNumFloorVertices = numFV;

        this->cachedWallVertices.resize(numWV * FPV);
        if (numWV > 0)
        {
            memcpy(this->cachedWallVertices.data(), &data[off], numWV * FPV * sizeof(float));
        }
        off += numWV * FPV * sizeof(float);
        this->cachedWallIndices.resize(numWI);
        if (numWI > 0)
        {
            memcpy(this->cachedWallIndices.data(), &data[off], numWI * sizeof(uint32_t));
        }
        off += numWI * sizeof(uint32_t);
        this->cachedNumWallVertices = numWV;

        this->cachedCeilVertices.resize(numCV * FPV);
        if (numCV > 0)
        {
            memcpy(this->cachedCeilVertices.data(), &data[off], numCV * FPV * sizeof(float));
        }
        off += numCV * FPV * sizeof(float);
        this->cachedCeilIndices.resize(numCI);
        if (numCI > 0)
        {
            memcpy(this->cachedCeilIndices.data(), &data[off], numCI * sizeof(uint32_t));
        }
        off += numCI * sizeof(uint32_t);
        this->cachedNumCeilVertices = numCV;

        if (numFV > 0 || numWV > 0)
        {
            this->createDungeonMesh();
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Event handlers
    ///////////////////////////////////////////////////////////////////////////////////////////////

    void ProceduralDungeonComponent::handleMeshModifyMode(NOWA::EventDataPtr eventData)
    {
        auto castData = boost::static_pointer_cast<EventDataEditorMode>(eventData);
        this->isEditorMeshModifyMode = (castData->getManipulationMode() == EditorManager::EDITOR_MESH_MODIFY_MODE);
        this->updateModificationState();
    }

    void ProceduralDungeonComponent::handleGameObjectSelected(NOWA::EventDataPtr eventData)
    {
        auto castData = boost::static_pointer_cast<EventDataGameObjectSelected>(eventData);

        if (castData->getGameObjectId() == this->gameObjectPtr->getId())
        {
            this->isSelected = castData->getIsSelected();
        }
        else if (castData->getIsSelected())
        {
            this->isSelected = false;
        }

        this->updateModificationState();
    }

    void ProceduralDungeonComponent::handleComponentManuallyDeleted(NOWA::EventDataPtr eventData)
    {
        auto castData = boost::static_pointer_cast<EventDataDeleteComponent>(eventData);
        if (this->gameObjectPtr->getId() == castData->getGameObjectId() && this->getClassName() == castData->getComponentName())
        {
            this->deleteDungeonDataFile();
        }
    }

    void ProceduralDungeonComponent::addInputListener(void)
    {
        const Ogre::String listenerName = ProceduralDungeonComponent::getStaticClassName() + "_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());

        GraphicsModule::RenderCommand cmd = [this, listenerName]()
        {
            if (auto* core = InputDeviceCore::getSingletonPtr())
            {
                core->addKeyListener(this, listenerName);
                core->addMouseListener(this, listenerName);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "ProceduralDungeonComponent::addInputListener");
    }

    void ProceduralDungeonComponent::removeInputListener(void)
    {
        const Ogre::String listenerName = ProceduralDungeonComponent::getStaticClassName() + "_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId());

        GraphicsModule::RenderCommand cmd = [this, listenerName]()
        {
            if (auto* core = InputDeviceCore::getSingletonPtr())
            {
                core->removeKeyListener(listenerName);
                core->removeMouseListener(listenerName);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "ProceduralDungeonComponent::removeInputListener");
    }

    void ProceduralDungeonComponent::updateModificationState(void)
    {
        bool shouldBeActive = this->activated->getBool() && this->isEditorMeshModifyMode && this->isSelected;

        if (shouldBeActive)
        {
            this->addInputListener();
            this->buildState = BuildState::PLACING;
        }
        else
        {
            this->removeInputListener();
            this->destroyPreviewMesh();
            this->buildState = BuildState::IDLE;
            this->isShiftPressed = false;
            this->isCtrlPressed = false;
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // Helpers
    ///////////////////////////////////////////////////////////////////////////////////////////////

    ProceduralDungeonComponent::DungeonTheme ProceduralDungeonComponent::getDungeonThemeEnum(void) const
    {
        const Ogre::String& themeStr = this->dungeonTheme->getListSelectedValue();
        if (themeStr == "Cave")
        {
            return DungeonTheme::CAVE;
        }
        if (themeStr == "SciFi")
        {
            return DungeonTheme::SCIFI;
        }
        if (themeStr == "Ice")
        {
            return DungeonTheme::ICE;
        }
        if (themeStr == "Crypt")
        {
            return DungeonTheme::CRYPT;
        }
        return DungeonTheme::DUNGEON;
    }

    Ogre::Vector3 ProceduralDungeonComponent::snapToGridFunc(const Ogre::Vector3& position)
    {
        const Ogre::Real cellSz = maxValue(0.1f, this->cellSize->getReal());
        return Ogre::Vector3(Ogre::Math::Floor(position.x / cellSz + 0.5f) * cellSz, position.y, Ogre::Math::Floor(position.z / cellSz + 0.5f) * cellSz);
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // ConvertToMesh
    ///////////////////////////////////////////////////////////////////////////////////////////////

    bool ProceduralDungeonComponent::convertToMeshApply(void)
    {
        if (!this->dungeonMesh)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralDungeonComponent] convertToMeshApply: no dungeon mesh to export.");
            return false;
        }

        // Generate filename based on GameObject ID
        Ogre::String meshName = "Dungeon_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()) + ".mesh";

        // Ensure it has .mesh extension
        if (!Ogre::StringUtil::endsWith(meshName, ".mesh", true))
        {
            meshName += ".mesh";
        }

        // Full path to Procedural folder
        auto filePathNames = Core::getSingletonPtr()->getSectionPath("Procedural");

        if (true == filePathNames.empty())
        {
            return false;
        }
        Ogre::String fullPath = filePathNames[0] + "/" + meshName;

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralDungeonComponent] Converting procedural dungeon to static mesh: " + meshName);

        // Step 1: Export the mesh
        if (!this->exportMesh(fullPath))
        {
            return false;
        }

        // Step 2: Capture data needed for the delayed operation
        Ogre::String capturedMeshName = meshName;
        GameObjectPtr capturedGameObjectPtr = this->gameObjectPtr;
        unsigned int capturedComponentIndex = this->getIndex();

        // Store current datablocks to reapply them
        Ogre::String firstDbName = this->ceilingDatablock->getString();
        Ogre::String secondDbName = this->floorDatablock->getString();
        Ogre::String thirdDbName = this->wallDatablock->getString();

        // Store GameObject position (important!)
        Ogre::Vector3 currentPosition = this->gameObjectPtr->getPosition();
        Ogre::Quaternion currentOrientation = this->gameObjectPtr->getOrientation();
        Ogre::Vector3 currentScale = this->gameObjectPtr->getScale();

        // Step 3: Schedule delayed conversion
        // We need a small delay to ensure the mesh file is fully written and available
        NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.5f));

        auto conversionFunction = [this, capturedMeshName, capturedGameObjectPtr, capturedComponentIndex, firstDbName, secondDbName, thirdDbName, currentPosition, currentOrientation, currentScale]()
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralDungeonComponent] Loading converted mesh: " + capturedMeshName);

            // Load the exported mesh
            Ogre::MeshPtr loadedMesh;
            try
            {
                loadedMesh = Ogre::MeshManager::getSingleton().load(capturedMeshName, "Procedural");
            }
            catch (Ogre::Exception& e)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralDungeonComponent] Failed to load exported mesh: " + e.getFullDescription());
                return;
            }

            if (loadedMesh.isNull())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralDungeonComponent] Loaded mesh is null!");
                return;
            }

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralDungeonComponent] Mesh loaded successfully: " + Ogre::StringConverter::toString(loadedMesh->getNumSubMeshes()) + " submeshes");

            // Create new Item from the loaded mesh on render thread
            Ogre::Item* newItem = nullptr;

            NOWA::GraphicsModule::RenderCommand renderCommand = [this, capturedGameObjectPtr, loadedMesh, firstDbName, secondDbName, thirdDbName, &newItem]()
            {
                newItem = capturedGameObjectPtr->getSceneManager()->createItem(loadedMesh, capturedGameObjectPtr->isDynamic() ? Ogre::SCENE_DYNAMIC : Ogre::SCENE_STATIC);

                // Reapply datablocks
                if (newItem->getNumSubItems() >= 1 && !firstDbName.empty())
                {
                    Ogre::HlmsDatablock* centerDb = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(firstDbName);
                    if (nullptr != centerDb)
                    {
                        newItem->getSubItem(0)->setDatablock(centerDb);
                        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralDungeonComponent] Applied center datablock: " + firstDbName);
                    }
                }

                if (newItem->getNumSubItems() >= 2)
                {
                    Ogre::String dbToUse = secondDbName.empty() ? secondDbName : firstDbName;
                    if (!dbToUse.empty())
                    {
                        Ogre::HlmsDatablock* edgeDb = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(dbToUse);
                        if (nullptr != edgeDb)
                        {
                            newItem->getSubItem(1)->setDatablock(edgeDb);
                            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralDungeonComponent] Applied edge datablock: " + dbToUse);
                        }
                    }
                }

                if (newItem->getNumSubItems() >= 3)
                {
                    Ogre::String dbToUse = thirdDbName.empty() ? thirdDbName : firstDbName;
                    if (!dbToUse.empty())
                    {
                        Ogre::HlmsDatablock* edgeDb = Ogre::Root::getSingleton().getHlmsManager()->getDatablockNoDefault(dbToUse);
                        if (nullptr != edgeDb)
                        {
                            newItem->getSubItem(2)->setDatablock(edgeDb);
                            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralDungeonComponent] Applied edge datablock: " + dbToUse);
                        }
                    }
                }

                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralDungeonComponent] Created new Item from exported mesh");
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ProceduralDungeonComponent::convertToMesh_createItem");

            if (nullptr == newItem)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralDungeonComponent] Failed to create Item from mesh!");
                return;
            }

            this->destroyPreviewMesh();
            this->destroyDungeonMesh();

            // Update GameObject to use the new mesh
            // This will destroy the old procedural mesh and attach the new one
            // Assign the new mesh to GameObject - this preserves transform automatically
            if (false == capturedGameObjectPtr->assignMesh(newItem))
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralDungeonComponent] Failed to assign mesh to GameObject!");
                return;
            }

            // IMPORTANT: Restore the GameObject's transform
            // The init() might have changed it, so we restore the original position
            capturedGameObjectPtr->getSceneNode()->setPosition(currentPosition);
            capturedGameObjectPtr->getSceneNode()->setOrientation(currentOrientation);
            capturedGameObjectPtr->getSceneNode()->setScale(currentScale);

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralDungeonComponent] GameObject transform restored: pos=" + Ogre::StringConverter::toString(currentPosition));

            // Fire event that the component is being deleted
            boost::shared_ptr<EventDataDeleteComponent> eventDataDeleteComponent(new EventDataDeleteComponent(capturedGameObjectPtr->getId(), "ProceduralDungeonComponent", capturedComponentIndex));
            NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataDeleteComponent);

            // Delete the ProceduralDungeonComponent
            // This must be done after the mesh is loaded and GameObject is updated
            capturedGameObjectPtr->deleteComponentByIndex(capturedComponentIndex);

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralDungeonComponent] ========================================");
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralDungeonComponent] CONVERSION COMPLETE!");
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralDungeonComponent] - ProceduralDungeonComponent removed");
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralDungeonComponent] - Static mesh file: " + capturedMeshName);
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralDungeonComponent] - GameObject now uses cached mesh");
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralDungeonComponent] IMPORTANT: SAVE YOUR SCENE to persist this change!");
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralDungeonComponent] ========================================");
        };

        NOWA::ProcessPtr closureProcess(new NOWA::ClosureProcess(conversionFunction));
        delayProcess->attachChild(closureProcess);
        NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ProceduralDungeonComponent] Mesh export completed. Conversion scheduled in 0.5 seconds...");

        boost::shared_ptr<EventDataRefreshGui> eventDataRefreshGui(new EventDataRefreshGui());
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshGui);

        return true;
    }

    bool ProceduralDungeonComponent::exportMesh(const Ogre::String& filename)
    {
        if (!this->dungeonMesh)
        {
            return false;
        }

        bool success = false;
        GraphicsModule::RenderCommand cmd = [this, filename, &success]()
        {
            try
            {
                Ogre::MeshSerializer serializer(Ogre::Root::getSingletonPtr()->getRenderSystem()->getVaoManager());
                serializer.exportMesh(this->dungeonMesh.get(), filename);
            }
            catch (const std::exception& e)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ProceduralDungeonComponent] exportMesh exception: " + Ogre::String(e.what()));
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "ProceduralDungeonComponent::exportMesh");
        return success;
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////
    // ProceduralDungeonComponent::Attribute Setters / Getters
    ///////////////////////////////////////////////////////////////////////////////////////////////

    void ProceduralDungeonComponent::setActivated(bool value)
    {
        this->activated->setValue(value);
    }

    bool ProceduralDungeonComponent::isActivated(void) const
    {
        return this->activated->getBool();
    }

    void ProceduralDungeonComponent::setSeed(int value)
    {
        this->seed->setValue(value);
    }

    int ProceduralDungeonComponent::getSeed(void) const
    {
        return this->seed->getInt();
    }

    void ProceduralDungeonComponent::setDungeonWidth(Ogre::Real value)
    {
        this->dungeonWidth->setValue(maxValue(4.0f, value));
    }

    Ogre::Real ProceduralDungeonComponent::getDungeonWidth(void) const
    {
        return this->dungeonWidth->getReal();
    }

    void ProceduralDungeonComponent::setDungeonDepth(Ogre::Real value)
    {
        this->dungeonDepth->setValue(maxValue(4.0f, value));
    }

    Ogre::Real ProceduralDungeonComponent::getDungeonDepth(void) const
    {
        return this->dungeonDepth->getReal();
    }

    void ProceduralDungeonComponent::setCellSize(Ogre::Real value)
    {
        this->cellSize->setValue(maxValue(0.25f, value));
    }

    Ogre::Real ProceduralDungeonComponent::getCellSize(void) const
    {
        return this->cellSize->getReal();
    }

    void ProceduralDungeonComponent::setMinRoomCells(int value)
    {
        this->minRoomCells->setValue(Ogre::Math::Clamp(value, 2, 20));
    }

    int ProceduralDungeonComponent::getMinRoomCells(void) const
    {
        return this->minRoomCells->getInt();
    }

    void ProceduralDungeonComponent::setMaxRoomCells(int value)
    {
        this->maxRoomCells->setValue(Ogre::Math::Clamp(value, 3, 30));
    }

    int ProceduralDungeonComponent::getMaxRoomCells(void) const
    {
        return this->maxRoomCells->getInt();
    }

    void ProceduralDungeonComponent::setCorridorWidthCells(int value)
    {
        this->corridorWidthCells->setValue(Ogre::Math::Clamp(value, 1, 6));
    }

    int ProceduralDungeonComponent::getCorridorWidthCells(void) const
    {
        return this->corridorWidthCells->getInt();
    }

    void ProceduralDungeonComponent::setWallHeight(Ogre::Real value)
    {
        this->wallHeight->setValue(value);
        if (true == this->hasDungeonOrigin)
        {
            this->rebuildMesh();
        }
    }

    Ogre::Real ProceduralDungeonComponent::getWallHeight(void) const
    {
        return this->wallHeight->getReal();
    }

    void ProceduralDungeonComponent::setJitter(Ogre::Real jitter)
    {
        if (jitter > 0.8f)
        {
            jitter = 0.8f;
        }
        this->jitter->setValue(jitter);
        if (true == this->hasDungeonOrigin)
        {
            this->rebuildMesh();
        }
    }

    Ogre::Real ProceduralDungeonComponent::getJitter(void) const
    {
        return this->jitter->getReal();
    }

    Ogre::Real ProceduralDungeonComponent::getFloorDepth() const
    {
        return Ogre::Real();
    }

    void ProceduralDungeonComponent::setFloorDepth(Ogre::Real depth)
    {
        if (depth < 0.0f)
        {
            depth = 0.02f;
        }
        if (depth > 0.2f)
        {
            depth = 0.2f;
        }
        this->floorDepth->setValue(depth);
        if (true == this->hasDungeonOrigin)
        {
            this->rebuildMesh();
        }
    }

    void ProceduralDungeonComponent::setAddCeiling(bool value)
    {
        this->addCeiling->setValue(value);
        if (true == this->hasDungeonOrigin)
        {
            this->rebuildMesh();
        }
    }

    bool ProceduralDungeonComponent::getAddCeiling(void) const
    {
        return this->addCeiling->getBool();
    }

    void ProceduralDungeonComponent::setLoopProbability(Ogre::Real value)
    {
        this->loopProbability->setValue(Ogre::Math::Clamp(value, 0.0f, 1.0f));
        if (true == this->hasDungeonOrigin)
        {
            this->rebuildMesh();
        }
    }

    Ogre::Real ProceduralDungeonComponent::getLoopProbability(void) const
    {
        return this->loopProbability->getReal();
    }

    void ProceduralDungeonComponent::setDungeonTheme(const Ogre::String& theme)
    {
        this->dungeonTheme->setListSelectedValue(theme);
        if (true == this->hasDungeonOrigin)
        {
            this->rebuildMesh();
        }
    }

    Ogre::String ProceduralDungeonComponent::getDungeonTheme(void) const
    {
        return this->dungeonTheme->getListSelectedValue();
    }

    void ProceduralDungeonComponent::setAdaptToGround(bool value)
    {
        this->adaptToGround->setValue(value);
        if (true == this->hasDungeonOrigin)
        {
            this->rebuildMesh();
        }
    }

    bool ProceduralDungeonComponent::getAdaptToGround(void) const
    {
        return this->adaptToGround->getBool();
    }

    void ProceduralDungeonComponent::setHeightOffset(Ogre::Real value)
    {
        this->heightOffset->setValue(value);
        if (true == this->hasDungeonOrigin)
        {
            this->rebuildMesh();
        }
    }

    Ogre::Real ProceduralDungeonComponent::getHeightOffset(void) const
    {
        return this->heightOffset->getReal();
    }

    void ProceduralDungeonComponent::setAddPillars(bool value)
    {
        this->addPillars->setValue(value);
        if (true == this->hasDungeonOrigin)
        {
            this->rebuildMesh();
        }
    }

    bool ProceduralDungeonComponent::getAddPillars(void) const
    {
        return this->addPillars->getBool();
    }

    void ProceduralDungeonComponent::setPillarSize(Ogre::Real value)
    {
        this->pillarSize->setValue(Ogre::Math::Clamp(value, 0.0f, 2.0f));
        if (!this->dungeonRooms.empty())
        {
            this->rebuildMesh();
        }
    }

    Ogre::Real ProceduralDungeonComponent::getPillarSize(void) const
    {
        return this->pillarSize->getReal();
    }

    void ProceduralDungeonComponent::setFloorDatablock(const Ogre::String& datablock)
    {
        this->floorDatablock->setValue(datablock);

        if (this->dungeonItem && !datablock.empty())
        {
            GraphicsModule::RenderCommand cmd = [this, datablock]()
            {
                if (!this->dungeonItem || this->dungeonItem->getNumSubItems() < 1)
                {
                    return;
                }
                Ogre::HlmsDatablock* db = Ogre::Root::getSingletonPtr()->getHlmsManager()->getDatablockNoDefault(datablock);
                if (db)
                {
                    this->dungeonItem->getSubItem(0)->setDatablock(db);
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "ProceduralDungeonComponent::setFloorDatablock");
        }
    }

    Ogre::String ProceduralDungeonComponent::getFloorDatablock(void) const
    {
        return this->floorDatablock->getString();
    }

    void ProceduralDungeonComponent::setWallDatablock(const Ogre::String& datablock)
    {
        this->wallDatablock->setValue(datablock);

        if (this->dungeonItem && !datablock.empty())
        {
            GraphicsModule::RenderCommand cmd = [this, datablock]()
            {
                if (!this->dungeonItem || this->dungeonItem->getNumSubItems() < 2)
                {
                    return;
                }
                Ogre::HlmsDatablock* db = Ogre::Root::getSingletonPtr()->getHlmsManager()->getDatablockNoDefault(datablock);
                if (db)
                {
                    this->dungeonItem->getSubItem(1)->setDatablock(db);
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "ProceduralDungeonComponent::setWallDatablock");
        }
    }

    Ogre::String ProceduralDungeonComponent::getWallDatablock(void) const
    {
        return this->wallDatablock->getString();
    }

    void ProceduralDungeonComponent::setCeilingDatablock(const Ogre::String& datablock)
    {
        this->ceilingDatablock->setValue(datablock);

        if (this->dungeonItem && !datablock.empty())
        {
            GraphicsModule::RenderCommand cmd = [this, datablock]()
            {
                if (!this->dungeonItem || this->dungeonItem->getNumSubItems() < 3)
                {
                    return;
                }
                Ogre::HlmsDatablock* db = Ogre::Root::getSingletonPtr()->getHlmsManager()->getDatablockNoDefault(datablock);
                if (db)
                {
                    this->dungeonItem->getSubItem(2)->setDatablock(db);
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "ProceduralDungeonComponent::setCeilingDatablock");
        }
    }

    Ogre::String ProceduralDungeonComponent::getCeilingDatablock(void) const
    {
        return this->ceilingDatablock->getString();
    }

    void ProceduralDungeonComponent::setFloorUVTiling(const Ogre::Vector2& tiling)
    {
        this->floorUVTiling->setValue(tiling);
        if (true == this->hasDungeonOrigin)
        {
            this->rebuildMesh();
        }
    }

    Ogre::Vector2 ProceduralDungeonComponent::getFloorUVTiling(void) const
    {
        return this->floorUVTiling->getVector2();
    }

    void ProceduralDungeonComponent::setWallUVTiling(const Ogre::Vector2& tiling)
    {
        this->wallUVTiling->setValue(tiling);
        if (true == this->hasDungeonOrigin)
        {
            this->rebuildMesh();
        }
    }

    Ogre::Vector2 ProceduralDungeonComponent::getWallUVTiling(void) const
    {
        return this->wallUVTiling->getVector2();
    }

    bool ProceduralDungeonComponent::getAddWindows(void) const
    {
        return this->addWindows->getBool();
    }

    void ProceduralDungeonComponent::setAddWindows(bool addWindows)
    {
        this->addWindows->setValue(addWindows);
        if (true == this->hasDungeonOrigin)
        {
            this->rebuildMesh();
        }
    }

    // --- windowProb (Real) -------------------------------------------------------

    Ogre::Real ProceduralDungeonComponent::getWindowProb(void) const
    {
        return this->windowProb->getReal();
    }

    void ProceduralDungeonComponent::setWindowProb(Ogre::Real windowProb)
    {
        // Clamp to [0, 1]
        this->windowProb->setValue(std::max(0.0f, std::min(1.0f, windowProb)));
        if (this->hasDungeonOrigin && this->addWindows->getBool())
        {
            this->rebuildMesh();
        }
    }

    // --- windowWidth (Real) ------------------------------------------------------

    Ogre::Real ProceduralDungeonComponent::getWindowWidth(void) const
    {
        return this->windowWidth->getReal();
    }

    void ProceduralDungeonComponent::setWindowWidth(Ogre::Real windowWidth)
    {
        // Clamp to (0, 1) — zero width makes no sense, 1.0 would erase the whole face
        this->windowWidth->setValue(std::max(0.05f, std::min(0.95f, windowWidth)));
        if (this->hasDungeonOrigin && this->addWindows->getBool())
        {
            this->rebuildMesh();
        }
    }

    // --- windowHeight (Real) -----------------------------------------------------

    Ogre::Real ProceduralDungeonComponent::getWindowHeight(void) const
    {
        return this->windowHeight->getReal();
    }

    void ProceduralDungeonComponent::setWindowHeight(Ogre::Real windowHeight)
    {
        this->windowHeight->setValue(std::max(0.05f, std::min(0.95f, windowHeight)));
        if (this->hasDungeonOrigin && this->addWindows->getBool())
        {
            this->rebuildMesh();
        }
    }

    // --- windowSill (Real) -------------------------------------------------------

    Ogre::Real ProceduralDungeonComponent::getWindowSill(void) const
    {
        return this->windowSill->getReal();
    }

    void ProceduralDungeonComponent::setWindowSill(Ogre::Real windowSill)
    {
        if (windowSill < 0.0f)
        {
            windowSill = 0.0f;
        }
        else if (windowSill > 0.9f)
        {
            windowSill = 0.9f;
        }

        if (this->hasDungeonOrigin && this->addWindows->getBool())
        {
            this->rebuildMesh();
        }
    }

    void ProceduralDungeonComponent::setLevelCount(unsigned int levelCount)
    {
        if (levelCount > 20)
        {
            levelCount = 20;
        }

        this->levelCount->setValue(levelCount);
        this->generateDungeon();
    }

    unsigned int ProceduralDungeonComponent::getLevelCount(void) const
    {
        return this->levelCount->getUInt();
    }

    void ProceduralDungeonComponent::setStairsProbability(Ogre::Real stairsProbability)
    {
        if (stairsProbability < 0.0f)
        {
            stairsProbability = 0.0f;
        }
        else if (stairsProbability > 0.4f)
        {
            stairsProbability = 0.4f;
        }
    }

    Ogre::Real ProceduralDungeonComponent::getStairsProbability(void) const
    {
        return this->stairsProbability->getReal();
    }

    void ProceduralDungeonComponent::setAddStairs(bool add)
    {
        this->addStairs->setValue(add);
    }

    bool ProceduralDungeonComponent::getAddStairs(void) const
    {
        return this->addStairs->getBool();
    }

    void ProceduralDungeonComponent::setStairHeight(Ogre::Real stairHeight)
    {
        if (stairHeight < 0.1f)
        {
            stairHeight = 0.1f;
        }
        else if (stairHeight > 0.4f)
        {
            stairHeight = 0.4f;
        }

        if (true == this->hasDungeonOrigin)
        {
            this->rebuildMesh();
        }
    }

    Ogre::Real ProceduralDungeonComponent::getStairHeight(void) const
    {
        return this->stairHeight->getReal();
    }

} // namespace NOWA

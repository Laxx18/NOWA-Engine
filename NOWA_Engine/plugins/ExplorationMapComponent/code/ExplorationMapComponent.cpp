#include "NOWAPrecompiled.h"
#include "ExplorationMapComponent.h"
#include "gameobject/GameObjectController.h"
#include "gameobject/GameObjectFactory.h"
#include "main/AppStateManager.h"
#include "main/Core.h"
#include "modules/GraphicsModule.h"
#include "modules/LuaScriptApi.h"
#include "modules/MiniMapModule.h"
#include "utilities/XMLConverter.h"

#include "OgreAbiUtils.h"

#include "MyGUI_TextureUtility.h"
#include <MyGUI.h>

#include <algorithm>
#include <cmath>
#include <functional>

namespace
{
    // The atlas with all map graphics. Layout (pixels):
    //   (0,0) white, (32,0) cell fill, (64,0) hatched fill, (96,0) cell highlight
    //   (x,32) and (x,64): 16 border variants (bit 1 left, 2 right, 4 top, 8 bottom), 32x32 each
    //   (x,96) and (x,128): icons 32x32
    const Ogre::String ATLAS_TEXTURE = "ExplorationMap.png";

    struct AtlasRect
    {
        int x;
        int y;
        int width;
        int height;
    };

    const AtlasRect ATLAS_WHITE = {0, 0, 8, 8};
    const AtlasRect ATLAS_FILL = {32, 0, 32, 32};
    const AtlasRect ATLAS_FILL_KNOWN = {64, 0, 32, 32};
    const AtlasRect ATLAS_HIGHLIGHT = {96, 0, 32, 32};

    const AtlasRect ATLAS_ICON_PLAYER = {0, 96, 32, 32};
    const AtlasRect ATLAS_ICON_SAVE = {32, 96, 32, 32};
    const AtlasRect ATLAS_ICON_TELEPORTER = {64, 96, 32, 32};
    const AtlasRect ATLAS_ICON_BOSS = {96, 96, 32, 32};
    const AtlasRect ATLAS_ICON_ITEM = {128, 96, 32, 32};
    const AtlasRect ATLAS_ICON_ITEM_COLLECTED = {160, 96, 32, 32};
    const AtlasRect ATLAS_ICON_SHOP = {192, 96, 32, 32};
    const AtlasRect ATLAS_ICON_LOCKED = {224, 96, 32, 32};
    const AtlasRect ATLAS_ICON_SECRET = {0, 128, 32, 32};
    const AtlasRect ATLAS_ICON_CUSTOM = {32, 128, 32, 32};
    const AtlasRect ATLAS_ICON_BOSS_DEFEATED = {64, 128, 32, 32};

    const unsigned int BORDER_LEFT = 1;
    const unsigned int BORDER_RIGHT = 2;
    const unsigned int BORDER_TOP = 4;
    const unsigned int BORDER_BOTTOM = 8;

    // How often the cell of the target is checked, in seconds.
    const Ogre::Real EXPLORE_INTERVAL = 0.1f;

    // Revealed but not explored cells: room colour times this factor.
    const Ogre::Real REVEALED_COLOR_FACTOR = 0.55f;

    const Ogre::Real FULL_MAP_MIN_ZOOM = 0.25f;
    const Ogre::Real FULL_MAP_MAX_ZOOM = 4.0f;

    // Room colours for scenes without setSceneColor.
    const Ogre::Vector3 ROOM_PALETTE[] = {Ogre::Vector3(0.67f, 0.31f, 0.78f), Ogre::Vector3(0.24f, 0.55f, 0.86f), Ogre::Vector3(0.27f, 0.75f, 0.47f), Ogre::Vector3(0.90f, 0.55f, 0.24f), Ogre::Vector3(0.85f, 0.30f, 0.40f),
        Ogre::Vector3(0.20f, 0.70f, 0.75f), Ogre::Vector3(0.80f, 0.72f, 0.28f), Ogre::Vector3(0.45f, 0.40f, 0.85f)};
    const size_t ROOM_PALETTE_COUNT = sizeof(ROOM_PALETTE) / sizeof(ROOM_PALETTE[0]);

    const std::vector<Ogre::String> MARKER_TYPES = {"SavePoint", "Teleporter", "Boss", "Item", "Shop", "LockedDoor", "Secret", "Custom"};

    void setAtlasImage(MyGUI::ImageBox* imageBox, const AtlasRect& rect)
    {
        imageBox->setImageInfo(ATLAS_TEXTURE, MyGUI::IntCoord(rect.x, rect.y, rect.width, rect.height), MyGUI::IntSize(rect.width, rect.height));
        // Attention: setImageInfo only defines the image tiles. The selected index stays ITEM_NONE, and then MyGUI draws nothing at all -
        // the reason why the first version showed no background, no frame and no cells.
        imageBox->setImageIndex(0);
    }

    AtlasRect getBorderRect(unsigned int mask)
    {
        AtlasRect rect = {static_cast<int>(mask % 8) * 32, 32 + static_cast<int>(mask / 8) * 32, 32, 32};
        return rect;
    }

    AtlasRect getMarkerIconRect(const Ogre::String& type, const Ogre::String& state)
    {
        const bool collected = "collected" == state;

        if ("SavePoint" == type)
        {
            return ATLAS_ICON_SAVE;
        }
        if ("Teleporter" == type)
        {
            return ATLAS_ICON_TELEPORTER;
        }
        if ("Boss" == type)
        {
            if (true == collected)
            {
                return ATLAS_ICON_BOSS_DEFEATED;
            }
            return ATLAS_ICON_BOSS;
        }
        if ("Item" == type)
        {
            if (true == collected)
            {
                return ATLAS_ICON_ITEM_COLLECTED;
            }
            return ATLAS_ICON_ITEM;
        }
        if ("Shop" == type)
        {
            return ATLAS_ICON_SHOP;
        }
        if ("LockedDoor" == type)
        {
            return ATLAS_ICON_LOCKED;
        }
        if ("Secret" == type)
        {
            return ATLAS_ICON_SECRET;
        }
        return ATLAS_ICON_CUSTOM;
    }

    MyGUI::ImageBox* createAtlasImageBox(MyGUI::Widget* parent, const MyGUI::IntCoord& coord, const AtlasRect& rect)
    {
        MyGUI::ImageBox* imageBox = parent->createWidget<MyGUI::ImageBox>("ImageBox", coord, MyGUI::Align::Default);
        setAtlasImage(imageBox, rect);
        imageBox->setNeedMouseFocus(false);
        return imageBox;
    }

    int clampInt(int value, int minValue, int maxValue)
    {
        return std::max(minValue, std::min(value, maxValue));
    }
}

namespace NOWA
{
    using namespace rapidxml;
    using namespace luabind;

    ExplorationMapComponent::ExplorationMapComponent() :
        GameObjectComponent(),
        name("ExplorationMapComponent"),
        activated(new Variant(ExplorationMapComponent::AttrActivated(), true, this->attributes)),
        targetId(new Variant(ExplorationMapComponent::AttrTargetId(), static_cast<unsigned long>(0), this->attributes, true)),
        cellSize(new Variant(ExplorationMapComponent::AttrCellSize(), 10.0f, this->attributes)),
        axis(new Variant(ExplorationMapComponent::AttrAxis(), std::vector<Ogre::String>{"X,Y", "X,Z"}, this->attributes)),
        miniMapPosition(new Variant(ExplorationMapComponent::AttrMiniMapPosition(), Ogre::Vector2(0.77f, 0.03f), this->attributes)),
        miniMapSize(new Variant(ExplorationMapComponent::AttrMiniMapSize(), Ogre::Vector2(0.2f, 0.24f), this->attributes)),
        miniMapCellPixels(new Variant(ExplorationMapComponent::AttrMiniMapCellPixels(), 14, this->attributes)),
        fullMapCellPixels(new Variant(ExplorationMapComponent::AttrFullMapCellPixels(), 28, this->attributes)),
        showMiniMapOnStart(new Variant(ExplorationMapComponent::AttrShowMiniMapOnStart(), true, this->attributes)),
        layer(new Variant(ExplorationMapComponent::AttrLayer(), std::vector<Ogre::String>{"Overlapped", "Back", "Middle", "Main", "Popup"}, this->attributes)),
        showSceneNames(new Variant(ExplorationMapComponent::AttrShowSceneNames(), true, this->attributes)),
        showUnexploredMarkers(new Variant(ExplorationMapComponent::AttrShowUnexploredMarkers(), false, this->attributes)),
        backgroundColor(new Variant(ExplorationMapComponent::AttrBackgroundColor(), Ogre::Vector4(0.05f, 0.06f, 0.12f, 0.85f), this->attributes)),
        frameColor(new Variant(ExplorationMapComponent::AttrFrameColor(), Ogre::Vector3(0.85f, 0.9f, 1.0f), this->attributes)),
        doorColor(new Variant(ExplorationMapComponent::AttrDoorColor(), Ogre::Vector3(0.45f, 0.85f, 1.0f), this->attributes)),
        completionLabel(new Variant(ExplorationMapComponent::AttrCompletionLabel(), Ogre::String("Map"), this->attributes)),
        markerCount(new Variant(ExplorationMapComponent::AttrMarkerCount(), 0, this->attributes)),
        bConnected(false),
        xyAxisUsed(true),
        gridMinX(0),
        gridMinY(0),
        gridMaxX(0),
        gridMaxY(0),
        currentSceneIndex(-1),
        lastTargetCell(0, 0),
        hasLastTargetCell(false),
        exploreTimer(0.0f),
        mapMode(MapMode::HIDDEN),
        miniMapEnabled(true),
        builtMode(MapMode::HIDDEN),
        viewRoot(nullptr),
        viewContent(nullptr),
        completionTextBox(nullptr),
        targetCellHighlight(nullptr),
        targetImageBox(nullptr),
        viewCellPixels(14),
        viewGridMinX(0),
        viewGridMaxY(0),
        fullMapPan(Ogre::Vector2::ZERO),
        fullMapZoom(1.0f),
        pulseTime(0.0f)
    {
        this->targetId->setDescription("The id of the game object that explores the map, e.g. the global player.");
        this->cellSize->setDescription("The size of one map cell in world units. A scene becomes a room of 'bounds size / cell size' cells.");
        this->axis->setDescription("'X,Y' for a side view (Jump'n'Run), 'X,Z' for a top view (-Z is up on the map).");
        this->miniMapPosition->setDescription("Relative screen position of the mini map (top left corner).");
        this->miniMapSize->setDescription("Relative screen size of the mini map.");
        this->miniMapCellPixels->setDescription("Size of one cell on the mini map in pixels.");
        this->fullMapCellPixels->setDescription("Size of one cell on the full map in pixels, before zooming.");
        this->showMiniMapOnStart->setDescription("Shows the mini map when the simulation starts.");
        this->layer->setDescription("The MyGUI layer of the map.");
        this->showSceneNames->setDescription("Shows the scene names on the mini map and the full map, e.g. for debugging. The name is the scene name, unless set via setSceneDisplayName.");
        this->showUnexploredMarkers->setDescription("Shows markers also in cells, which are neither explored nor revealed.");
        this->backgroundColor->setDescription("Background colour of the map, the fourth component is the opacity.");
        this->frameColor->setDescription("Colour of the frame around the map.");
        this->doorColor->setDescription("Colour of the door gaps.");
        this->completionLabel->setDescription("Text in front of the completion percentage on the full map, e.g. 'Map' or 'Karte'.");
        this->markerCount->setDescription("Count of markers configured here. Markers can also be added via Lua (addMarker).");

        this->frameColor->addUserData(GameObject::AttrActionColorDialog());
        this->doorColor->addUserData(GameObject::AttrActionColorDialog());
        this->markerCount->addUserData(GameObject::AttrActionNeedRefresh());

        this->markerTypeDisplayNames["SavePoint"] = "Save Point";
        this->markerTypeDisplayNames["Teleporter"] = "Teleporter";
        this->markerTypeDisplayNames["Boss"] = "Boss";
        this->markerTypeDisplayNames["Item"] = "Item";
        this->markerTypeDisplayNames["Shop"] = "Shop";
        this->markerTypeDisplayNames["LockedDoor"] = "Locked Door";
        this->markerTypeDisplayNames["Secret"] = "Secret";
        this->markerTypeDisplayNames["Custom"] = "Marker";
    }

    ExplorationMapComponent::~ExplorationMapComponent(void)
    {
        if (nullptr != this->viewRoot)
        {
            this->destroyViewBlocking();
        }
    }

    void ExplorationMapComponent::initialise()
    {
    }

    const Ogre::String& ExplorationMapComponent::getName() const
    {
        return this->name;
    }

    void ExplorationMapComponent::install(const Ogre::NameValuePairList* options)
    {
        GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<ExplorationMapComponent>(ExplorationMapComponent::getStaticClassId(), ExplorationMapComponent::getStaticClassName());
    }

    void ExplorationMapComponent::shutdown()
    {
    }

    void ExplorationMapComponent::uninstall()
    {
    }

    void ExplorationMapComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
    {
        outAbiCookie = Ogre::generateAbiCookie();
    }

    bool ExplorationMapComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        GameObjectComponent::init(propertyElement);

        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
        {
            this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetId")
        {
            this->targetId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CellSize")
        {
            this->cellSize->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Axis")
        {
            this->axis->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MiniMapPosition")
        {
            this->miniMapPosition->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MiniMapSize")
        {
            this->miniMapSize->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MiniMapCellPixels")
        {
            this->miniMapCellPixels->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data", 14));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FullMapCellPixels")
        {
            this->fullMapCellPixels->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data", 28));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ShowMiniMapOnStart")
        {
            this->showMiniMapOnStart->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Layer")
        {
            this->layer->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ShowSceneNames")
        {
            this->showSceneNames->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ShowUnexploredMarkers")
        {
            this->showUnexploredMarkers->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BackgroundColor")
        {
            this->backgroundColor->setValue(XMLConverter::getAttribVector4(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FrameColor")
        {
            this->frameColor->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DoorColor")
        {
            this->doorColor->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CompletionLabel")
        {
            this->completionLabel->setValue(XMLConverter::getAttrib(propertyElement, "data"));
            propertyElement = propertyElement->next_sibling("property");
        }

        unsigned int count = 0;
        if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MarkerCount")
        {
            count = XMLConverter::getAttribUnsignedInt(propertyElement, "data", 0);
            propertyElement = propertyElement->next_sibling("property");
        }
        this->setMarkerCount(count);

        for (unsigned int i = 0; i < count; i++)
        {
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MarkerType" + Ogre::StringConverter::toString(i))
            {
                this->markerTypes[i]->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
                propertyElement = propertyElement->next_sibling("property");
            }
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MarkerTarget" + Ogre::StringConverter::toString(i))
            {
                this->markerTargets[i]->setValue(XMLConverter::getAttrib(propertyElement, "data"));
                propertyElement = propertyElement->next_sibling("property");
            }
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MarkerLabel" + Ogre::StringConverter::toString(i))
            {
                this->markerLabels[i]->setValue(XMLConverter::getAttrib(propertyElement, "data"));
                propertyElement = propertyElement->next_sibling("property");
            }
        }

        return true;
    }

    GameObjectCompPtr ExplorationMapComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        // A map exists once, on a global game object.
        return nullptr;
    }

    bool ExplorationMapComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ExplorationMapComponent] Init exploration map component for game object: " + this->gameObjectPtr->getName());
        return true;
    }

    bool ExplorationMapComponent::connect(void)
    {
        this->bConnected = true;
        this->hasLastTargetCell = false;
        this->exploreTimer = 0.0f;
        this->fullMapZoom = 1.0f;

        this->buildLayout();
        this->resolveAttributeMarkers();

        // The cell the target starts in is explored right away.
        int cellX = 0;
        int cellY = 0;
        this->exploreCurrentCell(cellX, cellY);

        this->miniMapEnabled = this->showMiniMapOnStart->getBool();
        this->mapMode = MapMode::HIDDEN;
        if (true == this->activated->getBool() && true == this->miniMapEnabled)
        {
            this->mapMode = MapMode::MINI;
        }

        this->rebuildView();
        return true;
    }

    bool ExplorationMapComponent::disconnect(void)
    {
        this->bConnected = false;

        Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
        NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

        this->mapMode = MapMode::HIDDEN;
        this->destroyViewBlocking();

        this->luaMarkers.clear();
        this->markerClickedClosureFunction = luabind::object();
        return true;
    }

    bool ExplorationMapComponent::onCloned(void)
    {
        return true;
    }

    void ExplorationMapComponent::onRemoveComponent(void)
    {
        GameObjectComponent::onRemoveComponent();

        Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
        NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

        this->destroyViewBlocking();
    }

    void ExplorationMapComponent::update(Ogre::Real dt, bool notSimulating)
    {
        if (true == notSimulating || false == this->activated->getBool() || false == this->bConnected || this->currentSceneIndex < 0)
        {
            return;
        }

        // Exploration runs also while the map is hidden.
        this->exploreTimer -= dt;
        if (this->exploreTimer <= 0.0f)
        {
            this->exploreTimer = EXPLORE_INTERVAL;

            int cellX = 0;
            int cellY = 0;
            if (true == this->exploreCurrentCell(cellX, cellY) && MapMode::HIDDEN != this->mapMode)
            {
                // Attention: NOT enqueueAndWait. A blocking round trip waits for the running frame (several ms) and made the picture stutter,
                // see GameObjectController::deleteGameObjectImmediately. Everything the render thread needs is prepared here and copied.
                const CellDraw cellDraw = this->prepareCellDraw(static_cast<size_t>(this->currentSceneIndex), cellX, cellY, true);
                const OverlayDraw overlayDraw = this->prepareOverlayDraw();
                const Ogre::String completionCaption = this->getCompletionCaption();

                NOWA::GraphicsModule::RenderCommand renderCommand = [this, cellDraw, overlayDraw, completionCaption]()
                {
                    this->drawCell(cellDraw);
                    this->drawOverlays(overlayDraw);
                    this->setCompletionCaption(completionCaption);
                };
                NOWA::GraphicsModule::getInstance()->enqueue(std::move(renderCommand), "ExplorationMapComponent::update drawCell");
            }
        }

        if (MapMode::HIDDEN == this->mapMode)
        {
            return;
        }

        Ogre::Vector2 targetCellSpace = Ogre::Vector2::ZERO;
        const bool targetValid = this->getTargetCellSpace(targetCellSpace);

        auto closureFunction = [this, targetCellSpace, targetValid](Ogre::Real renderDt)
        {
            this->updateTargetView(targetCellSpace, targetValid, renderDt);
        };
        Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
        NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Logic thread

    void ExplorationMapComponent::buildLayout(void)
    {
        MiniMapModule* miniMapModule = AppStateManager::getSingletonPtr()->getMiniMapModule();

        // In NOWA-Design levels may have been edited since the last simulation. In the game the scene files never change.
        if (false == Core::getSingletonPtr()->getIsGame())
        {
            miniMapModule->clearSceneCache();
        }

        this->mapScenes.clear();
        this->sceneIndices.clear();
        this->cellOwners.clear();
        this->currentSceneIndex = -1;
        this->gridMinX = 0;
        this->gridMinY = 0;
        this->gridMaxX = 0;
        this->gridMaxY = 0;

        this->xyAxisUsed = "X,Y" == this->axis->getListSelectedValue();

        const Ogre::String currentSceneName = Core::getSingletonPtr()->getSceneName();
        const std::vector<MiniMapModule::SceneLayout> sceneLayouts = miniMapModule->computeSceneLayout(currentSceneName, this->xyAxisUsed);
        if (true == sceneLayouts.empty())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ExplorationMapComponent] No map for scene: '" + currentSceneName + "'. Does the scene have saved bounds?");
            return;
        }

        const Ogre::Real cellSizeValue = std::max(this->cellSize->getReal(), 0.1f);

        bool firstScene = true;
        for (const MiniMapModule::SceneLayout& sceneLayout : sceneLayouts)
        {
            MapScene mapScene;
            mapScene.sceneName = sceneLayout.sceneName;
            mapScene.worldOrigin = sceneLayout.worldOrigin;
            mapScene.worldSize = sceneLayout.worldSize;
            mapScene.exits = sceneLayout.exits;
            mapScene.cellWidth = std::max(1, static_cast<int>(std::round(sceneLayout.worldSize.x / cellSizeValue)));
            mapScene.cellHeight = std::max(1, static_cast<int>(std::round(sceneLayout.worldSize.y / cellSizeValue)));
            mapScene.cellX0 = static_cast<int>(std::round(sceneLayout.mapOffset.x / cellSizeValue));
            mapScene.cellY0 = static_cast<int>(std::round(sceneLayout.mapOffset.y / cellSizeValue));

            // Door to door placement lets neighbouring rooms overlap by a little, which becomes a whole cell after rounding.
            // Keep the room clear of the room it was entered from, on the side of the exit.
            auto parentIt = this->sceneIndices.find(sceneLayout.parentSceneName);
            if (this->sceneIndices.end() != parentIt)
            {
                const MapScene& parent = this->mapScenes[parentIt->second];
                const Ogre::Vector2& direction = sceneLayout.entryDirection;

                if (Ogre::Math::Abs(direction.x) >= Ogre::Math::Abs(direction.y) && 0.0f != direction.x)
                {
                    if (direction.x > 0.0f && mapScene.cellX0 < parent.cellX0 + parent.cellWidth)
                    {
                        mapScene.cellX0 = parent.cellX0 + parent.cellWidth;
                    }
                    else if (direction.x < 0.0f && mapScene.cellX0 + mapScene.cellWidth > parent.cellX0)
                    {
                        mapScene.cellX0 = parent.cellX0 - mapScene.cellWidth;
                    }
                }
                else if (0.0f != direction.y)
                {
                    if (direction.y > 0.0f && mapScene.cellY0 < parent.cellY0 + parent.cellHeight)
                    {
                        mapScene.cellY0 = parent.cellY0 + parent.cellHeight;
                    }
                    else if (direction.y < 0.0f && mapScene.cellY0 + mapScene.cellHeight > parent.cellY0)
                    {
                        mapScene.cellY0 = parent.cellY0 - mapScene.cellHeight;
                    }
                }
            }

            const int sceneIndex = static_cast<int>(this->mapScenes.size());

            // A cell belongs to the first room that claims it.
            for (int y = mapScene.cellY0; y < mapScene.cellY0 + mapScene.cellHeight; y++)
            {
                for (int x = mapScene.cellX0; x < mapScene.cellX0 + mapScene.cellWidth; x++)
                {
                    this->cellOwners.emplace(std::make_pair(x, y), sceneIndex);
                }
            }

            if (true == firstScene)
            {
                this->gridMinX = mapScene.cellX0;
                this->gridMinY = mapScene.cellY0;
                this->gridMaxX = mapScene.cellX0 + mapScene.cellWidth - 1;
                this->gridMaxY = mapScene.cellY0 + mapScene.cellHeight - 1;
                firstScene = false;
            }
            else
            {
                this->gridMinX = std::min(this->gridMinX, mapScene.cellX0);
                this->gridMinY = std::min(this->gridMinY, mapScene.cellY0);
                this->gridMaxX = std::max(this->gridMaxX, mapScene.cellX0 + mapScene.cellWidth - 1);
                this->gridMaxY = std::max(this->gridMaxY, mapScene.cellY0 + mapScene.cellHeight - 1);
            }

            if (mapScene.sceneName == currentSceneName)
            {
                this->currentSceneIndex = sceneIndex;
            }

            this->sceneIndices.emplace(mapScene.sceneName, static_cast<size_t>(sceneIndex));
            this->mapScenes.emplace_back(mapScene);
        }
    }

    void ExplorationMapComponent::resolveAttributeMarkers(void)
    {
        this->attributeMarkers.clear();

        MiniMapModule* miniMapModule = AppStateManager::getSingletonPtr()->getMiniMapModule();
        const Ogre::String currentSceneName = Core::getSingletonPtr()->getSceneName();

        for (size_t i = 0; i < this->markerTargets.size(); i++)
        {
            // 'Level4:2341435213' - a game object stored in another scene, or '2341435213' - a game object of the current scene.
            const Ogre::String target = this->markerTargets[i]->getString();
            if (true == target.empty())
            {
                continue;
            }

            Ogre::String sceneName = currentSceneName;
            Ogre::String strId = target;
            const size_t found = target.find(":");
            if (Ogre::String::npos != found)
            {
                sceneName = target.substr(0, found);
                strId = target.substr(found + 1);
            }

            const unsigned long id = Ogre::StringConverter::parseUnsignedLong(strId);
            if (0 == id)
            {
                continue;
            }

            MapMarker mapMarker;
            // The target is the id: it is unique and stays the same, when the marker list is edited.
            mapMarker.id = target;
            mapMarker.sceneName = sceneName;
            mapMarker.type = this->markerTypes[i]->getListSelectedValue();
            mapMarker.label = this->markerLabels[i]->getString();

            bool found3dPosition = false;
            if (sceneName == currentSceneName)
            {
                GameObjectPtr gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(id);
                if (nullptr != gameObjectPtr)
                {
                    mapMarker.worldPosition = gameObjectPtr->getPosition();
                    found3dPosition = true;
                }
            }

            if (false == found3dPosition)
            {
                std::pair<bool, Ogre::Vector3> positionData = miniMapModule->getGameObjectPositionInScene(sceneName, id);
                if (true == positionData.first)
                {
                    mapMarker.worldPosition = positionData.second;
                    found3dPosition = true;
                }
            }

            if (false == found3dPosition)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ExplorationMapComponent] Marker target: '" + target + "' not found. Check the scene name and the id.");
                continue;
            }

            this->attributeMarkers.emplace_back(mapMarker);
        }
    }

    std::vector<ExplorationMapComponent::MapMarker> ExplorationMapComponent::getAllMarkers(void) const
    {
        std::vector<MapMarker> markers = this->attributeMarkers;
        markers.insert(markers.end(), this->luaMarkers.begin(), this->luaMarkers.end());
        return markers;
    }

    bool ExplorationMapComponent::getTargetCellSpace(Ogre::Vector2& cellSpace) const
    {
        if (this->currentSceneIndex < 0)
        {
            return false;
        }

        GameObjectPtr targetGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->targetId->getULong());
        if (nullptr == targetGameObjectPtr)
        {
            return false;
        }

        cellSpace = this->toCellSpace(static_cast<size_t>(this->currentSceneIndex), targetGameObjectPtr->getPosition());
        return true;
    }

    bool ExplorationMapComponent::exploreCurrentCell(int& cellX, int& cellY)
    {
        Ogre::Vector2 cellSpace = Ogre::Vector2::ZERO;
        if (false == this->getTargetCellSpace(cellSpace))
        {
            return false;
        }

        const MapScene& mapScene = this->mapScenes[static_cast<size_t>(this->currentSceneIndex)];

        // Clamped into the room: the target may stand a little outside the bounds, e.g. while leaving through an exit.
        cellX = clampInt(static_cast<int>(std::floor(cellSpace.x)), mapScene.cellX0, mapScene.cellX0 + mapScene.cellWidth - 1);
        cellY = clampInt(static_cast<int>(std::floor(cellSpace.y)), mapScene.cellY0, mapScene.cellY0 + mapScene.cellHeight - 1);

        const std::pair<int, int> cell(cellX, cellY);
        if (true == this->hasLastTargetCell && cell == this->lastTargetCell)
        {
            return false;
        }
        this->lastTargetCell = cell;
        this->hasLastTargetCell = true;

        MiniMapModule* miniMapModule = AppStateManager::getSingletonPtr()->getMiniMapModule();
        const int localX = cellX - mapScene.cellX0;
        const int localY = cellY - mapScene.cellY0;
        if (true == miniMapModule->getIsCellExplored(mapScene.sceneName, localX, localY))
        {
            return false;
        }

        miniMapModule->setCellExplored(mapScene.sceneName, localX, localY);
        return true;
    }

    Ogre::Vector2 ExplorationMapComponent::toCellSpace(size_t sceneIndex, const Ogre::Vector3& worldPosition) const
    {
        const MapScene& mapScene = this->mapScenes[sceneIndex];
        const Ogre::Vector2 local = MiniMapModule::toMapPlane(worldPosition, this->xyAxisUsed) - mapScene.worldOrigin;

        // Proportional within the room, so a position inside the bounds always lands in a cell of that room, even if the bounds are no
        // multiple of the cell size.
        Ogre::Vector2 cellSpace(static_cast<Ogre::Real>(mapScene.cellX0) + 0.5f * mapScene.cellWidth, static_cast<Ogre::Real>(mapScene.cellY0) + 0.5f * mapScene.cellHeight);
        if (mapScene.worldSize.x > 0.0f)
        {
            cellSpace.x = static_cast<Ogre::Real>(mapScene.cellX0) + (local.x / mapScene.worldSize.x) * static_cast<Ogre::Real>(mapScene.cellWidth);
        }
        if (mapScene.worldSize.y > 0.0f)
        {
            cellSpace.y = static_cast<Ogre::Real>(mapScene.cellY0) + (local.y / mapScene.worldSize.y) * static_cast<Ogre::Real>(mapScene.cellHeight);
        }
        return cellSpace;
    }

    int ExplorationMapComponent::getCellOwner(int cellX, int cellY) const
    {
        auto it = this->cellOwners.find(std::make_pair(cellX, cellY));
        if (this->cellOwners.end() == it)
        {
            return -1;
        }
        return it->second;
    }

    Ogre::Vector3 ExplorationMapComponent::getSceneColor(const Ogre::String& sceneName) const
    {
        auto it = this->sceneColors.find(sceneName);
        if (this->sceneColors.end() != it)
        {
            return it->second;
        }
        return ROOM_PALETTE[std::hash<std::string>()(sceneName) % ROOM_PALETTE_COUNT];
    }

    Ogre::Real ExplorationMapComponent::getCompletionPercent(void) const
    {
        MiniMapModule* miniMapModule = AppStateManager::getSingletonPtr()->getMiniMapModule();
        const auto& exploredCells = miniMapModule->getExploredCells();

        size_t totalCells = 0;
        size_t exploredCount = 0;
        for (const MapScene& mapScene : this->mapScenes)
        {
            totalCells += static_cast<size_t>(mapScene.cellWidth) * static_cast<size_t>(mapScene.cellHeight);

            auto it = exploredCells.find(mapScene.sceneName);
            if (exploredCells.end() == it)
            {
                continue;
            }
            for (const std::pair<int, int>& cell : it->second)
            {
                if (cell.first >= 0 && cell.first < mapScene.cellWidth && cell.second >= 0 && cell.second < mapScene.cellHeight)
                {
                    exploredCount++;
                }
            }
        }

        if (0 == totalCells)
        {
            return 0.0f;
        }
        return 100.0f * static_cast<Ogre::Real>(exploredCount) / static_cast<Ogre::Real>(totalCells);
    }

    Ogre::String ExplorationMapComponent::getCompletionCaption(void) const
    {
        // Rounded, not floored: one explored cell of a hundred is shown as 1%, not as 0%.
        return this->completionLabel->getString() + " " + Ogre::StringConverter::toString(static_cast<int>(std::round(this->getCompletionPercent()))) + "%";
    }

    ExplorationMapComponent::CellDraw ExplorationMapComponent::prepareCellDraw(size_t sceneIndex, int cellX, int cellY, bool explored) const
    {
        const MapScene& mapScene = this->mapScenes[sceneIndex];

        CellDraw cellDraw;
        cellDraw.cellX = cellX;
        cellDraw.cellY = cellY;
        cellDraw.explored = explored;
        cellDraw.color = this->getSceneColor(mapScene.sceneName);
        cellDraw.frameColor = this->frameColor->getVector3();
        cellDraw.doorColor = this->doorColor->getVector3();

        // Walls where the neighbour belongs to another room or to no room at all.
        const int owner = static_cast<int>(sceneIndex);
        cellDraw.borderMask = 0;
        if (owner != this->getCellOwner(cellX - 1, cellY))
        {
            cellDraw.borderMask |= BORDER_LEFT;
        }
        if (owner != this->getCellOwner(cellX + 1, cellY))
        {
            cellDraw.borderMask |= BORDER_RIGHT;
        }
        if (owner != this->getCellOwner(cellX, cellY + 1))
        {
            cellDraw.borderMask |= BORDER_TOP;
        }
        if (owner != this->getCellOwner(cellX, cellY - 1))
        {
            cellDraw.borderMask |= BORDER_BOTTOM;
        }

        // Door gaps of the exits in this cell.
        for (const SceneExitInfo& exitInfo : mapScene.exits)
        {
            const Ogre::Vector2 exitCellSpace = this->toCellSpace(sceneIndex, exitInfo.position);
            const int exitCellX = clampInt(static_cast<int>(std::floor(exitCellSpace.x)), mapScene.cellX0, mapScene.cellX0 + mapScene.cellWidth - 1);
            const int exitCellY = clampInt(static_cast<int>(std::floor(exitCellSpace.y)), mapScene.cellY0, mapScene.cellY0 + mapScene.cellHeight - 1);
            if (exitCellX != cellX || exitCellY != cellY)
            {
                continue;
            }

            const Ogre::Vector2 direction = MiniMapModule::toMapDirection(exitInfo.exitDirection, this->xyAxisUsed);
            if (false == direction.isZeroLength())
            {
                cellDraw.doorDirections.emplace_back(direction);
            }
        }

        return cellDraw;
    }

    ExplorationMapComponent::OverlayDraw ExplorationMapComponent::prepareOverlayDraw(void) const
    {
        MiniMapModule* miniMapModule = AppStateManager::getSingletonPtr()->getMiniMapModule();

        OverlayDraw overlayDraw;
        overlayDraw.showUnexploredMarkers = this->showUnexploredMarkers->getBool();

        for (const MapMarker& mapMarker : this->getAllMarkers())
        {
            auto sceneIt = this->sceneIndices.find(mapMarker.sceneName);
            if (this->sceneIndices.end() == sceneIt)
            {
                continue;
            }

            MarkerDraw markerDraw;
            markerDraw.id = mapMarker.id;
            markerDraw.type = mapMarker.type;
            markerDraw.state = miniMapModule->getMapMarkerState(mapMarker.id);
            markerDraw.cellSpace = this->toCellSpace(sceneIt->second, mapMarker.worldPosition);
            overlayDraw.markers.emplace_back(markerDraw);
        }

        return overlayDraw;
    }

    void ExplorationMapComponent::rebuildView(void)
    {
        if (false == this->bConnected)
        {
            return;
        }

        Ogre::Vector2 targetCellSpace = Ogre::Vector2::ZERO;
        const bool targetValid = this->getTargetCellSpace(targetCellSpace);
        const Ogre::String completionCaption = this->getCompletionCaption();
        const MapMode mode = this->mapMode;

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, mode, targetCellSpace, targetValid, completionCaption]()
        {
            this->buildView(mode, targetCellSpace, targetValid, completionCaption);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ExplorationMapComponent::rebuildView");
    }

    void ExplorationMapComponent::destroyViewBlocking(void)
    {
        NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
        {
            this->destroyView();
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ExplorationMapComponent::destroyView");
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Render thread

    void ExplorationMapComponent::destroyView(void)
    {
        if (nullptr != this->viewRoot)
        {
            // Destroys all children as well.
            MyGUI::Gui::getInstancePtr()->destroyWidget(this->viewRoot);
        }

        this->viewRoot = nullptr;
        this->viewContent = nullptr;
        this->completionTextBox = nullptr;
        this->targetCellHighlight = nullptr;
        this->targetImageBox = nullptr;
        this->overlayWidgets.clear();
        this->cellFillWidgets.clear();
        this->builtMode = MapMode::HIDDEN;
    }

    std::pair<int, int> ExplorationMapComponent::getCellPixelPosition(int cellX, int cellY) const
    {
        // Map y grows upwards, widget y downwards.
        return std::make_pair((cellX - this->viewGridMinX) * this->viewCellPixels, (this->viewGridMaxY - cellY) * this->viewCellPixels);
    }

    Ogre::Vector2 ExplorationMapComponent::cellSpaceToPixel(const Ogre::Vector2& cellSpace) const
    {
        const Ogre::Real cellPixels = static_cast<Ogre::Real>(this->viewCellPixels);
        return Ogre::Vector2((cellSpace.x - static_cast<Ogre::Real>(this->viewGridMinX)) * cellPixels, (static_cast<Ogre::Real>(this->viewGridMaxY + 1) - cellSpace.y) * cellPixels);
    }

    bool ExplorationMapComponent::isCellRendered(int cellX, int cellY) const
    {
        return this->cellFillWidgets.end() != this->cellFillWidgets.find(std::make_pair(cellX, cellY));
    }

    void ExplorationMapComponent::buildView(MapMode mode, const Ogre::Vector2& targetCellSpace, bool targetValid, const Ogre::String& completionCaption)
    {
        this->destroyView();

        if (MapMode::HIDDEN == mode || true == this->mapScenes.empty())
        {
            return;
        }

        this->builtMode = mode;
        this->viewGridMinX = this->gridMinX;
        this->viewGridMaxY = this->gridMaxY;

        const MyGUI::IntSize viewSize = MyGUI::RenderManager::getInstance().getViewSize();
        const Ogre::Real viewWidth = static_cast<Ogre::Real>(viewSize.width);
        const Ogre::Real viewHeight = static_cast<Ogre::Real>(viewSize.height);

        MyGUI::IntCoord rootCoord;
        if (MapMode::MINI == mode)
        {
            const Ogre::Vector2 position = this->miniMapPosition->getVector2();
            const Ogre::Vector2 size = this->miniMapSize->getVector2();
            rootCoord = MyGUI::IntCoord(static_cast<int>(position.x * viewWidth), static_cast<int>(position.y * viewHeight), static_cast<int>(size.x * viewWidth), static_cast<int>(size.y * viewHeight));
            this->viewCellPixels = std::max(4, static_cast<int>(this->miniMapCellPixels->getUInt()));
        }
        else
        {
            rootCoord = MyGUI::IntCoord(static_cast<int>(0.05f * viewWidth), static_cast<int>(0.07f * viewHeight), static_cast<int>(0.9f * viewWidth), static_cast<int>(0.86f * viewHeight));
            this->viewCellPixels = std::max(4, static_cast<int>(static_cast<Ogre::Real>(this->fullMapCellPixels->getUInt()) * this->fullMapZoom));
        }

        // Root without an image: a plain container. Attention: the background is a separate child, because a widget's alpha is inherited by
        // all its children.
        MyGUI::ImageBox* root = MyGUI::Gui::getInstancePtr()->createWidget<MyGUI::ImageBox>("ImageBox", rootCoord, MyGUI::Align::Default, this->layer->getListSelectedValue());
        root->setNeedMouseFocus(false);
        this->viewRoot = root;

        const Ogre::Vector4 background = this->backgroundColor->getVector4();
        MyGUI::ImageBox* backgroundImageBox = createAtlasImageBox(root, MyGUI::IntCoord(0, 0, rootCoord.width, rootCoord.height), ATLAS_WHITE);
        backgroundImageBox->setColour(MyGUI::Colour(background.x, background.y, background.z));
        backgroundImageBox->setAlpha(background.w);

        // The content holds all cells; it is moved to follow the target. Children are clipped by the root.
        const int contentWidth = (this->gridMaxX - this->gridMinX + 1) * this->viewCellPixels;
        const int contentHeight = (this->gridMaxY - this->gridMinY + 1) * this->viewCellPixels;
        MyGUI::ImageBox* content = root->createWidget<MyGUI::ImageBox>("ImageBox", MyGUI::IntCoord(0, 0, contentWidth, contentHeight), MyGUI::Align::Default);
        content->setNeedMouseFocus(false);
        this->viewContent = content;

        MiniMapModule* miniMapModule = AppStateManager::getSingletonPtr()->getMiniMapModule();

        // Cells
        for (size_t sceneIndex = 0; sceneIndex < this->mapScenes.size(); sceneIndex++)
        {
            const MapScene& mapScene = this->mapScenes[sceneIndex];
            const bool sceneRevealed = miniMapModule->getIsSceneRevealed(mapScene.sceneName);

            for (int y = 0; y < mapScene.cellHeight; y++)
            {
                for (int x = 0; x < mapScene.cellWidth; x++)
                {
                    const bool explored = miniMapModule->getIsCellExplored(mapScene.sceneName, x, y);
                    if (true == explored || true == sceneRevealed)
                    {
                        this->drawCell(this->prepareCellDraw(sceneIndex, mapScene.cellX0 + x, mapScene.cellY0 + y, explored));
                    }
                }
            }
        }

        // Scene names, e.g. for debugging which level lies where. Default is the scene name, see setSceneDisplayName.
        if (true == this->showSceneNames->getBool())
        {
            for (const MapScene& mapScene : this->mapScenes)
            {
                bool anyCellRendered = false;
                for (int y = mapScene.cellY0; y < mapScene.cellY0 + mapScene.cellHeight && false == anyCellRendered; y++)
                {
                    for (int x = mapScene.cellX0; x < mapScene.cellX0 + mapScene.cellWidth && false == anyCellRendered; x++)
                    {
                        anyCellRendered = this->isCellRendered(x, y);
                    }
                }
                if (false == anyCellRendered)
                {
                    continue;
                }

                Ogre::String displayName = mapScene.sceneName;
                auto nameIt = this->sceneDisplayNames.find(mapScene.sceneName);
                if (this->sceneDisplayNames.end() != nameIt)
                {
                    displayName = nameIt->second;
                }

                const std::pair<int, int> topLeft = this->getCellPixelPosition(mapScene.cellX0, mapScene.cellY0 + mapScene.cellHeight - 1);
                MyGUI::TextBox* nameTextBox =
                    content->createWidget<MyGUI::TextBox>("TextBox", MyGUI::IntCoord(topLeft.first, topLeft.second, mapScene.cellWidth * this->viewCellPixels, mapScene.cellHeight * this->viewCellPixels), MyGUI::Align::Default);
                nameTextBox->setTextAlign(MyGUI::Align::Center);
                nameTextBox->setTextColour(MyGUI::Colour(1.0f, 1.0f, 1.0f, 0.9f));
                nameTextBox->setCaption(displayName);
                nameTextBox->setNeedMouseFocus(false);
            }
        }

        // Frame, drawn above the content (created after it).
        const Ogre::Vector3 frame = this->frameColor->getVector3();
        const MyGUI::Colour frameColour(frame.x, frame.y, frame.z);
        const int thickness = 2;
        MyGUI::ImageBox* frameTop = createAtlasImageBox(root, MyGUI::IntCoord(0, 0, rootCoord.width, thickness), ATLAS_WHITE);
        MyGUI::ImageBox* frameBottom = createAtlasImageBox(root, MyGUI::IntCoord(0, rootCoord.height - thickness, rootCoord.width, thickness), ATLAS_WHITE);
        MyGUI::ImageBox* frameLeft = createAtlasImageBox(root, MyGUI::IntCoord(0, 0, thickness, rootCoord.height), ATLAS_WHITE);
        MyGUI::ImageBox* frameRight = createAtlasImageBox(root, MyGUI::IntCoord(rootCoord.width - thickness, 0, thickness, rootCoord.height), ATLAS_WHITE);
        frameTop->setColour(frameColour);
        frameBottom->setColour(frameColour);
        frameLeft->setColour(frameColour);
        frameRight->setColour(frameColour);

        if (MapMode::FULL == mode)
        {
            // Completion
            this->completionTextBox = root->createWidget<MyGUI::TextBox>("TextBox", MyGUI::IntCoord(14, 10, 400, 28), MyGUI::Align::Default);
            this->completionTextBox->setTextColour(MyGUI::Colour(1.0f, 1.0f, 1.0f));
            this->completionTextBox->setNeedMouseFocus(false);
            this->setCompletionCaption(completionCaption);

            // Legend: one entry per marker type in use, in the order of MARKER_TYPES.
            const std::vector<MapMarker> markers = this->getAllMarkers();
            int legendX = 14;
            const int legendY = rootCoord.height - 34;
            for (const Ogre::String& type : MARKER_TYPES)
            {
                bool typeUsed = false;
                for (const MapMarker& mapMarker : markers)
                {
                    if (mapMarker.type == type)
                    {
                        typeUsed = true;
                        break;
                    }
                }
                if (false == typeUsed)
                {
                    continue;
                }

                createAtlasImageBox(root, MyGUI::IntCoord(legendX, legendY, 24, 24), getMarkerIconRect(type, ""));

                Ogre::String displayName = type;
                auto nameIt = this->markerTypeDisplayNames.find(type);
                if (this->markerTypeDisplayNames.end() != nameIt)
                {
                    displayName = nameIt->second;
                }

                MyGUI::TextBox* legendTextBox = root->createWidget<MyGUI::TextBox>("TextBox", MyGUI::IntCoord(legendX + 28, legendY, 140, 24), MyGUI::Align::Default);
                legendTextBox->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
                legendTextBox->setTextColour(MyGUI::Colour(0.9f, 0.9f, 0.95f));
                legendTextBox->setCaption(displayName);
                legendTextBox->setNeedMouseFocus(false);

                legendX += 170;
            }
        }

        this->drawOverlays(this->prepareOverlayDraw());

        // Initial placement, afterwards updateTargetView keeps the target centered.
        Ogre::Vector2 focus(static_cast<Ogre::Real>(contentWidth) * 0.5f, static_cast<Ogre::Real>(contentHeight) * 0.5f);
        if (true == targetValid)
        {
            focus = this->cellSpaceToPixel(targetCellSpace);
        }
        Ogre::Vector2 contentPosition(static_cast<Ogre::Real>(rootCoord.width) * 0.5f - focus.x, static_cast<Ogre::Real>(rootCoord.height) * 0.5f - focus.y);
        if (MapMode::FULL == mode)
        {
            contentPosition += this->fullMapPan;
        }
        content->setPosition(static_cast<int>(contentPosition.x), static_cast<int>(contentPosition.y));

        // One line per build (builds are rare: simulation start, mode changes, reveals), so no throttling needed.
        const MyGUI::IntSize atlasSize = MyGUI::texture_utility::getTextureSize(ATLAS_TEXTURE);
        Ogre::String modeName = "full";
        if (MapMode::MINI == mode)
        {
            modeName = "mini";
        }
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL,
            "[ExplorationMapComponent] View built. Mode: " + modeName + ", scenes: " + Ogre::StringConverter::toString(this->mapScenes.size()) + ", grid: " + Ogre::StringConverter::toString(this->gridMinX) + ".." +
                Ogre::StringConverter::toString(this->gridMaxX) + " x " + Ogre::StringConverter::toString(this->gridMinY) + ".." + Ogre::StringConverter::toString(this->gridMaxY) +
                ", cell pixels: " + Ogre::StringConverter::toString(this->viewCellPixels) + ", drawn cells: " + Ogre::StringConverter::toString(this->cellFillWidgets.size()) + ", target found: " + Ogre::StringConverter::toString(targetValid) +
                ", atlas size: " + Ogre::StringConverter::toString(atlasSize.width) + "x" + Ogre::StringConverter::toString(atlasSize.height));
        if (0 == atlasSize.width || 0 == atlasSize.height)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ExplorationMapComponent] Error: '" + ATLAS_TEXTURE + "' not found. Put it into the MyGUI media folder (where e.g. ProgressValueBar.png is).");
        }
        if (false == targetValid)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ExplorationMapComponent] Warning: 'Target Id' does not point to an existing game object - nothing is explored.");
        }
    }

    void ExplorationMapComponent::drawCell(const CellDraw& cellDraw)
    {
        if (nullptr == this->viewContent)
        {
            return;
        }

        const std::pair<int, int> cellKey(cellDraw.cellX, cellDraw.cellY);
        Ogre::Vector3 color = cellDraw.color;

        auto existingIt = this->cellFillWidgets.find(cellKey);
        if (this->cellFillWidgets.end() != existingIt)
        {
            // A revealed cell that is explored now: from hatched to filled.
            if (true == cellDraw.explored)
            {
                setAtlasImage(existingIt->second, ATLAS_FILL);
                existingIt->second->setColour(MyGUI::Colour(color.x, color.y, color.z));
            }
            return;
        }

        const int cellPixels = this->viewCellPixels;
        const std::pair<int, int> pixelPosition = this->getCellPixelPosition(cellDraw.cellX, cellDraw.cellY);
        const MyGUI::IntCoord coord(pixelPosition.first, pixelPosition.second, cellPixels, cellPixels);

        MyGUI::ImageBox* fillImageBox = nullptr;
        if (true == cellDraw.explored)
        {
            fillImageBox = createAtlasImageBox(this->viewContent, coord, ATLAS_FILL);
        }
        else
        {
            fillImageBox = createAtlasImageBox(this->viewContent, coord, ATLAS_FILL_KNOWN);
            color *= REVEALED_COLOR_FACTOR;
        }
        fillImageBox->setColour(MyGUI::Colour(color.x, color.y, color.z));
        this->cellFillWidgets.emplace(cellKey, fillImageBox);

        if (0 != cellDraw.borderMask)
        {
            MyGUI::ImageBox* borderImageBox = createAtlasImageBox(this->viewContent, coord, getBorderRect(cellDraw.borderMask));
            borderImageBox->setColour(MyGUI::Colour(cellDraw.frameColor.x, cellDraw.frameColor.y, cellDraw.frameColor.z));
        }

        // A bar across the wall on the side of each exit.
        const int longSide = std::max(3, cellPixels / 2);
        const int shortSide = std::max(2, cellPixels / 5);
        for (const Ogre::Vector2& direction : cellDraw.doorDirections)
        {
            MyGUI::IntCoord doorCoord;
            if (Ogre::Math::Abs(direction.x) >= Ogre::Math::Abs(direction.y))
            {
                int left = coord.left - shortSide / 2;
                if (direction.x > 0.0f)
                {
                    left = coord.left + cellPixels - shortSide / 2;
                }
                doorCoord = MyGUI::IntCoord(left, coord.top + (cellPixels - longSide) / 2, shortSide, longSide);
            }
            else
            {
                // Map up is widget up.
                int top = coord.top + cellPixels - shortSide / 2;
                if (direction.y > 0.0f)
                {
                    top = coord.top - shortSide / 2;
                }
                doorCoord = MyGUI::IntCoord(coord.left + (cellPixels - longSide) / 2, top, longSide, shortSide);
            }

            MyGUI::ImageBox* doorImageBox = createAtlasImageBox(this->viewContent, doorCoord, ATLAS_WHITE);
            doorImageBox->setColour(MyGUI::Colour(cellDraw.doorColor.x, cellDraw.doorColor.y, cellDraw.doorColor.z));
        }
    }

    void ExplorationMapComponent::destroyOverlays(void)
    {
        for (MyGUI::Widget* widget : this->overlayWidgets)
        {
            MyGUI::Gui::getInstancePtr()->destroyWidget(widget);
        }
        this->overlayWidgets.clear();
        this->targetCellHighlight = nullptr;
        this->targetImageBox = nullptr;
    }

    void ExplorationMapComponent::drawOverlays(const OverlayDraw& overlayDraw)
    {
        if (nullptr == this->viewContent)
        {
            return;
        }

        // Recreated so that they stay on top of cells added later.
        this->destroyOverlays();

        const bool fullMap = MapMode::FULL == this->builtMode;

        int iconSize = std::max(10, std::min(24, static_cast<int>(static_cast<Ogre::Real>(this->viewCellPixels) * 0.9f)));
        if (true == fullMap)
        {
            iconSize = std::max(14, std::min(32, static_cast<int>(static_cast<Ogre::Real>(this->viewCellPixels) * 0.8f)));
        }

        for (const MarkerDraw& markerDraw : overlayDraw.markers)
        {
            if ("hidden" == markerDraw.state)
            {
                continue;
            }

            const int cellX = static_cast<int>(std::floor(markerDraw.cellSpace.x));
            const int cellY = static_cast<int>(std::floor(markerDraw.cellSpace.y));
            if (false == overlayDraw.showUnexploredMarkers && false == this->isCellRendered(cellX, cellY))
            {
                continue;
            }

            const Ogre::Vector2 pixel = this->cellSpaceToPixel(markerDraw.cellSpace);
            MyGUI::ImageBox* iconImageBox =
                createAtlasImageBox(this->viewContent, MyGUI::IntCoord(static_cast<int>(pixel.x) - iconSize / 2, static_cast<int>(pixel.y) - iconSize / 2, iconSize, iconSize), getMarkerIconRect(markerDraw.type, markerDraw.state));

            if ("collected" == markerDraw.state && "Item" != markerDraw.type && "Boss" != markerDraw.type)
            {
                iconImageBox->setAlpha(0.45f);
            }

            if (true == fullMap)
            {
                iconImageBox->setNeedMouseFocus(true);
                iconImageBox->setUserString("MarkerId", markerDraw.id);
                iconImageBox->eventMouseButtonClick += MyGUI::newDelegate(this, &ExplorationMapComponent::onMarkerClicked);
            }

            this->overlayWidgets.emplace_back(iconImageBox);
        }

        // The cell of the target (pulsing) and the target itself, on top of everything.
        this->targetCellHighlight = createAtlasImageBox(this->viewContent, MyGUI::IntCoord(0, 0, this->viewCellPixels, this->viewCellPixels), ATLAS_HIGHLIGHT);
        this->overlayWidgets.emplace_back(this->targetCellHighlight);

        const int targetSize = std::max(8, static_cast<int>(static_cast<Ogre::Real>(iconSize) * 0.85f));
        this->targetImageBox = createAtlasImageBox(this->viewContent, MyGUI::IntCoord(0, 0, targetSize, targetSize), ATLAS_ICON_PLAYER);
        this->overlayWidgets.emplace_back(this->targetImageBox);
    }

    void ExplorationMapComponent::setCompletionCaption(const Ogre::String& completionCaption)
    {
        if (nullptr == this->completionTextBox)
        {
            return;
        }
        this->completionTextBox->setCaption(completionCaption);
    }

    void ExplorationMapComponent::updateTargetView(const Ogre::Vector2& targetCellSpace, bool targetValid, Ogre::Real renderDt)
    {
        if (nullptr == this->viewRoot || nullptr == this->viewContent)
        {
            return;
        }

        this->pulseTime += renderDt;

        const Ogre::Vector2 pixel = this->cellSpaceToPixel(targetCellSpace);

        if (nullptr != this->targetImageBox)
        {
            this->targetImageBox->setVisible(targetValid);
            const MyGUI::IntSize size = this->targetImageBox->getSize();
            this->targetImageBox->setPosition(static_cast<int>(pixel.x) - size.width / 2, static_cast<int>(pixel.y) - size.height / 2);
        }

        if (nullptr != this->targetCellHighlight)
        {
            this->targetCellHighlight->setVisible(targetValid);
            const std::pair<int, int> cellPosition = this->getCellPixelPosition(static_cast<int>(std::floor(targetCellSpace.x)), static_cast<int>(std::floor(targetCellSpace.y)));
            this->targetCellHighlight->setPosition(cellPosition.first, cellPosition.second);
            this->targetCellHighlight->setAlpha(0.35f + 0.65f * (0.5f + 0.5f * std::sin(this->pulseTime * 5.0f)));
        }

        if (true == targetValid)
        {
            const MyGUI::IntSize rootSize = this->viewRoot->getSize();
            Ogre::Vector2 contentPosition(static_cast<Ogre::Real>(rootSize.width) * 0.5f - pixel.x, static_cast<Ogre::Real>(rootSize.height) * 0.5f - pixel.y);
            if (MapMode::FULL == this->builtMode)
            {
                contentPosition += this->fullMapPan;
            }
            this->viewContent->setPosition(static_cast<int>(contentPosition.x), static_cast<int>(contentPosition.y));
        }
    }

    void ExplorationMapComponent::onMarkerClicked(MyGUI::Widget* sender)
    {
        // MyGUI calls this on the render thread; the Lua closure must run on the logic thread.
        const Ogre::String markerId = sender->getUserString("MarkerId");

        boost::weak_ptr<GameObjectComponent> weakThis = this->shared_from_this();
        NOWA::AppStateManager::LogicCommand logicCommand = [this, weakThis, markerId]()
        {
            boost::shared_ptr<GameObjectComponent> strongThis = weakThis.lock();
            if (nullptr == strongThis)
            {
                return;
            }

            if (false == this->markerClickedClosureFunction.is_valid())
            {
                return;
            }

            try
            {
                luabind::call_function<void>(this->markerClickedClosureFunction, markerId);
            }
            catch (luabind::error& error)
            {
                luabind::object errorMsg(luabind::from_stack(error.state(), -1));
                std::stringstream msg;
                msg << errorMsg;
                Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[ExplorationMapComponent] Caught error in 'reactOnMarkerClicked' Error: " + Ogre::String(error.what()) + " details: " + msg.str());
            }
        };
        NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Attributes

    void ExplorationMapComponent::actualizeValue(Variant* attribute)
    {
        GameObjectComponent::actualizeValue(attribute);

        if (ExplorationMapComponent::AttrActivated() == attribute->getName())
        {
            this->setActivated(attribute->getBool());
            return;
        }
        if (ExplorationMapComponent::AttrMarkerCount() == attribute->getName())
        {
            this->setMarkerCount(attribute->getUInt());
            return;
        }

        bool layoutChanged = false;
        bool markersChanged = false;

        if (ExplorationMapComponent::AttrTargetId() == attribute->getName())
        {
            this->targetId->setValue(attribute->getULong());
        }
        else if (ExplorationMapComponent::AttrCellSize() == attribute->getName())
        {
            this->setCellSize(attribute->getReal());
            layoutChanged = true;
        }
        else if (ExplorationMapComponent::AttrAxis() == attribute->getName())
        {
            this->axis->setListSelectedValue(attribute->getListSelectedValue());
            layoutChanged = true;
        }
        else if (ExplorationMapComponent::AttrMiniMapPosition() == attribute->getName())
        {
            this->miniMapPosition->setValue(attribute->getVector2());
        }
        else if (ExplorationMapComponent::AttrMiniMapSize() == attribute->getName())
        {
            this->miniMapSize->setValue(attribute->getVector2());
        }
        else if (ExplorationMapComponent::AttrMiniMapCellPixels() == attribute->getName())
        {
            this->miniMapCellPixels->setValue(attribute->getUInt());
        }
        else if (ExplorationMapComponent::AttrFullMapCellPixels() == attribute->getName())
        {
            this->fullMapCellPixels->setValue(attribute->getUInt());
        }
        else if (ExplorationMapComponent::AttrShowMiniMapOnStart() == attribute->getName())
        {
            this->showMiniMapOnStart->setValue(attribute->getBool());
        }
        else if (ExplorationMapComponent::AttrLayer() == attribute->getName())
        {
            this->layer->setListSelectedValue(attribute->getListSelectedValue());
        }
        else if (ExplorationMapComponent::AttrShowSceneNames() == attribute->getName())
        {
            this->showSceneNames->setValue(attribute->getBool());
        }
        else if (ExplorationMapComponent::AttrShowUnexploredMarkers() == attribute->getName())
        {
            this->showUnexploredMarkers->setValue(attribute->getBool());
        }
        else if (ExplorationMapComponent::AttrBackgroundColor() == attribute->getName())
        {
            this->backgroundColor->setValue(attribute->getVector4());
        }
        else if (ExplorationMapComponent::AttrFrameColor() == attribute->getName())
        {
            this->frameColor->setValue(attribute->getVector3());
        }
        else if (ExplorationMapComponent::AttrDoorColor() == attribute->getName())
        {
            this->doorColor->setValue(attribute->getVector3());
        }
        else if (ExplorationMapComponent::AttrCompletionLabel() == attribute->getName())
        {
            this->completionLabel->setValue(attribute->getString());
        }
        else
        {
            for (size_t i = 0; i < this->markerTypes.size(); i++)
            {
                if (ExplorationMapComponent::AttrMarkerType() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->markerTypes[i]->setListSelectedValue(attribute->getListSelectedValue());
                    markersChanged = true;
                    break;
                }
                if (ExplorationMapComponent::AttrMarkerTarget() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->markerTargets[i]->setValue(attribute->getString());
                    markersChanged = true;
                    break;
                }
                if (ExplorationMapComponent::AttrMarkerLabel() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->markerLabels[i]->setValue(attribute->getString());
                    markersChanged = true;
                    break;
                }
            }
        }

        if (false == this->bConnected)
        {
            return;
        }

        if (true == layoutChanged)
        {
            this->buildLayout();
            this->hasLastTargetCell = false;
        }
        if (true == markersChanged)
        {
            this->resolveAttributeMarkers();
        }
        this->rebuildView();
    }

    void ExplorationMapComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
    {
        // 2 = int
        // 6 = real
        // 7 = string
        // 8 = vector2
        // 9 = vector3
        // 10 = vector4
        // 12 = bool
        GameObjectComponent::writeXML(propertiesXML, doc);

        xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "TargetId"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->targetId->getULong())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "CellSize"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->cellSize->getReal())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Axis"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->axis->getListSelectedValue())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "MiniMapPosition"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->miniMapPosition->getVector2())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "MiniMapSize"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->miniMapSize->getVector2())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "MiniMapCellPixels"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->miniMapCellPixels->getUInt())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "FullMapCellPixels"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->fullMapCellPixels->getUInt())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "ShowMiniMapOnStart"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->showMiniMapOnStart->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "Layer"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->layer->getListSelectedValue())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "ShowSceneNames"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->showSceneNames->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "ShowUnexploredMarkers"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->showUnexploredMarkers->getBool())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "BackgroundColor"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->backgroundColor->getVector4())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "FrameColor"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->frameColor->getVector3())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "DoorColor"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->doorColor->getVector3())));
        propertiesXML->append_node(propertyXML);

        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "CompletionLabel"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->completionLabel->getString())));
        propertiesXML->append_node(propertyXML);

        const unsigned int count = static_cast<unsigned int>(this->markerTargets.size());
        propertyXML = doc.allocate_node(node_element, "property");
        propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
        propertyXML->append_attribute(doc.allocate_attribute("name", "MarkerCount"));
        propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, count)));
        propertiesXML->append_node(propertyXML);

        for (unsigned int i = 0; i < count; i++)
        {
            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
            propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(("MarkerType" + Ogre::StringConverter::toString(i)).c_str())));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->markerTypes[i]->getListSelectedValue())));
            propertiesXML->append_node(propertyXML);

            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
            propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(("MarkerTarget" + Ogre::StringConverter::toString(i)).c_str())));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->markerTargets[i]->getString())));
            propertiesXML->append_node(propertyXML);

            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
            propertyXML->append_attribute(doc.allocate_attribute("name", doc.allocate_string(("MarkerLabel" + Ogre::StringConverter::toString(i)).c_str())));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->markerLabels[i]->getString())));
            propertiesXML->append_node(propertyXML);
        }
    }

    Ogre::String ExplorationMapComponent::getClassName(void) const
    {
        return "ExplorationMapComponent";
    }

    Ogre::String ExplorationMapComponent::getParentClassName(void) const
    {
        return "GameObjectComponent";
    }

    void ExplorationMapComponent::setActivated(bool activated)
    {
        this->activated->setValue(activated);

        if (false == this->bConnected)
        {
            return;
        }

        if (false == activated)
        {
            this->mapMode = MapMode::HIDDEN;
        }
        else if (MapMode::HIDDEN == this->mapMode && true == this->miniMapEnabled)
        {
            this->mapMode = MapMode::MINI;
        }
        this->rebuildView();
    }

    bool ExplorationMapComponent::isActivated(void) const
    {
        return this->activated->getBool();
    }

    void ExplorationMapComponent::setTargetId(unsigned long targetId)
    {
        this->targetId->setValue(targetId);
    }

    unsigned long ExplorationMapComponent::getTargetId(void) const
    {
        return this->targetId->getULong();
    }

    void ExplorationMapComponent::setCellSize(Ogre::Real cellSize)
    {
        if (cellSize < 0.1f)
        {
            cellSize = 0.1f;
        }
        this->cellSize->setValue(cellSize);
    }

    Ogre::Real ExplorationMapComponent::getCellSize(void) const
    {
        return this->cellSize->getReal();
    }

    void ExplorationMapComponent::setMarkerCount(unsigned int markerCount)
    {
        this->markerCount->setValue(markerCount);
        const size_t oldSize = this->markerTargets.size();

        if (markerCount > oldSize)
        {
            this->markerTypes.resize(markerCount);
            this->markerTargets.resize(markerCount);
            this->markerLabels.resize(markerCount);

            for (size_t i = oldSize; i < markerCount; i++)
            {
                this->markerTypes[i] = new Variant(ExplorationMapComponent::AttrMarkerType() + Ogre::StringConverter::toString(i), MARKER_TYPES, this->attributes);
                this->markerTypes[i]->setDescription("What the marker shows.");

                this->markerTargets[i] = new Variant(ExplorationMapComponent::AttrMarkerTarget() + Ogre::StringConverter::toString(i), Ogre::String(""), this->attributes);
                this->markerTargets[i]->setDescription("The game object the marker stands for: 'SceneName:Id' for a game object in another scene, or just the 'Id' for one of the current scene.");

                this->markerLabels[i] = new Variant(ExplorationMapComponent::AttrMarkerLabel() + Ogre::StringConverter::toString(i), Ogre::String(""), this->attributes);
                this->markerLabels[i]->setDescription("Free text for the marker.");
                this->markerLabels[i]->addUserData(GameObject::AttrActionSeparator());
            }
        }
        else if (markerCount < oldSize)
        {
            this->eraseVariants(this->markerTypes, markerCount);
            this->eraseVariants(this->markerTargets, markerCount);
            this->eraseVariants(this->markerLabels, markerCount);
        }
    }

    unsigned int ExplorationMapComponent::getMarkerCount(void) const
    {
        return this->markerCount->getUInt();
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Views

    void ExplorationMapComponent::showMiniMap(bool show)
    {
        this->miniMapEnabled = show;

        if (MapMode::FULL == this->mapMode)
        {
            // Takes effect when the full map is closed.
            return;
        }

        this->mapMode = MapMode::HIDDEN;
        if (true == show && true == this->activated->getBool())
        {
            this->mapMode = MapMode::MINI;
        }
        this->rebuildView();
    }

    bool ExplorationMapComponent::isMiniMapShown(void) const
    {
        return MapMode::MINI == this->mapMode;
    }

    void ExplorationMapComponent::showFullMap(bool show)
    {
        if (true == show)
        {
            if (false == this->activated->getBool())
            {
                return;
            }
            this->mapMode = MapMode::FULL;

            // Each opening starts centered on the target.
            NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
            {
                this->fullMapPan = Ogre::Vector2::ZERO;
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ExplorationMapComponent::showFullMap");
        }
        else
        {
            this->mapMode = MapMode::HIDDEN;
            if (true == this->miniMapEnabled && true == this->activated->getBool())
            {
                this->mapMode = MapMode::MINI;
            }
        }
        this->rebuildView();
    }

    bool ExplorationMapComponent::isFullMapShown(void) const
    {
        return MapMode::FULL == this->mapMode;
    }

    void ExplorationMapComponent::toggleFullMap(void)
    {
        this->showFullMap(MapMode::FULL != this->mapMode);
    }

    void ExplorationMapComponent::panFullMap(Ogre::Real deltaX, Ogre::Real deltaY)
    {
        NOWA::GraphicsModule::RenderCommand renderCommand = [this, deltaX, deltaY]()
        {
            this->fullMapPan += Ogre::Vector2(deltaX, deltaY);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ExplorationMapComponent::panFullMap");
    }

    void ExplorationMapComponent::zoomFullMap(Ogre::Real factor)
    {
        if (factor <= 0.0f)
        {
            return;
        }

        this->fullMapZoom = std::max(FULL_MAP_MIN_ZOOM, std::min(FULL_MAP_MAX_ZOOM, this->fullMapZoom * factor));

        if (MapMode::FULL == this->mapMode)
        {
            this->rebuildView();
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Markers

    void ExplorationMapComponent::addMarker(const Ogre::String& markerId, const Ogre::String& sceneName, const Ogre::Vector3& position, const Ogre::String& type, const Ogre::String& label)
    {
        MapMarker mapMarker;
        mapMarker.id = markerId;
        mapMarker.sceneName = sceneName;
        if (true == mapMarker.sceneName.empty())
        {
            mapMarker.sceneName = Core::getSingletonPtr()->getSceneName();
        }
        mapMarker.worldPosition = position;
        mapMarker.type = type;
        mapMarker.label = label;

        auto it = std::find_if(this->luaMarkers.begin(), this->luaMarkers.end(),
            [&markerId](const MapMarker& other)
            {
                return other.id == markerId;
            });
        if (this->luaMarkers.end() != it)
        {
            *it = mapMarker;
        }
        else
        {
            this->luaMarkers.emplace_back(mapMarker);
        }

        this->rebuildView();
    }

    void ExplorationMapComponent::removeMarker(const Ogre::String& markerId)
    {
        this->luaMarkers.erase(std::remove_if(this->luaMarkers.begin(), this->luaMarkers.end(),
                                   [&markerId](const MapMarker& other)
                                   {
                                       return other.id == markerId;
                                   }),
            this->luaMarkers.end());
        this->rebuildView();
    }

    void ExplorationMapComponent::setMarkerState(const Ogre::String& markerId, const Ogre::String& state)
    {
        Ogre::String storedState = state;
        if ("normal" == storedState)
        {
            storedState = "";
        }
        AppStateManager::getSingletonPtr()->getMiniMapModule()->setMapMarkerState(markerId, storedState);
        this->rebuildView();
    }

    Ogre::String ExplorationMapComponent::getMarkerState(const Ogre::String& markerId) const
    {
        const Ogre::String state = AppStateManager::getSingletonPtr()->getMiniMapModule()->getMapMarkerState(markerId);
        if (true == state.empty())
        {
            return "normal";
        }
        return state;
    }

    void ExplorationMapComponent::setMarkerTypeDisplayName(const Ogre::String& type, const Ogre::String& displayName)
    {
        this->markerTypeDisplayNames[type] = displayName;
        if (MapMode::FULL == this->mapMode)
        {
            this->rebuildView();
        }
    }

    void ExplorationMapComponent::reactOnMarkerClicked(luabind::object closureFunction)
    {
        this->markerClickedClosureFunction = closureFunction;
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Exploration

    void ExplorationMapComponent::revealScene(const Ogre::String& sceneName)
    {
        AppStateManager::getSingletonPtr()->getMiniMapModule()->setSceneRevealed(sceneName, true);
        this->rebuildView();
    }

    bool ExplorationMapComponent::isSceneRevealed(const Ogre::String& sceneName) const
    {
        return AppStateManager::getSingletonPtr()->getMiniMapModule()->getIsSceneRevealed(sceneName);
    }

    void ExplorationMapComponent::revealAllScenes(void)
    {
        MiniMapModule* miniMapModule = AppStateManager::getSingletonPtr()->getMiniMapModule();
        for (const MapScene& mapScene : this->mapScenes)
        {
            miniMapModule->setSceneRevealed(mapScene.sceneName, true);
        }
        this->rebuildView();
    }

    void ExplorationMapComponent::resetExploration(void)
    {
        AppStateManager::getSingletonPtr()->getMiniMapModule()->clearExploration();
        this->hasLastTargetCell = false;

        int cellX = 0;
        int cellY = 0;
        this->exploreCurrentCell(cellX, cellY);
        this->rebuildView();
    }

    void ExplorationMapComponent::setSceneColor(const Ogre::String& sceneName, const Ogre::Vector3& color)
    {
        this->sceneColors[sceneName] = color;
        this->rebuildView();
    }

    void ExplorationMapComponent::setSceneDisplayName(const Ogre::String& sceneName, const Ogre::String& displayName)
    {
        this->sceneDisplayNames[sceneName] = displayName;
        if (MapMode::FULL == this->mapMode)
        {
            this->rebuildView();
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Lua registration part

    ExplorationMapComponent* getExplorationMapComponentFromIndex(GameObject* gameObject, unsigned int occurrenceIndex)
    {
        return makeStrongPtr<ExplorationMapComponent>(gameObject->getComponentWithOccurrence<ExplorationMapComponent>(occurrenceIndex)).get();
    }

    ExplorationMapComponent* getExplorationMapComponent(GameObject* gameObject)
    {
        return makeStrongPtr<ExplorationMapComponent>(gameObject->getComponent<ExplorationMapComponent>()).get();
    }

    ExplorationMapComponent* getExplorationMapComponentFromName(GameObject* gameObject, const Ogre::String& name)
    {
        return makeStrongPtr<ExplorationMapComponent>(gameObject->getComponentFromName<ExplorationMapComponent>(name)).get();
    }

    void ExplorationMapComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectController)
    {
        module(lua)[class_<ExplorationMapComponent, GameObjectComponent>("ExplorationMapComponent")
                .def("setActivated", &ExplorationMapComponent::setActivated)
                .def("isActivated", &ExplorationMapComponent::isActivated)
                .def("showMiniMap", &ExplorationMapComponent::showMiniMap)
                .def("isMiniMapShown", &ExplorationMapComponent::isMiniMapShown)
                .def("showFullMap", &ExplorationMapComponent::showFullMap)
                .def("isFullMapShown", &ExplorationMapComponent::isFullMapShown)
                .def("toggleFullMap", &ExplorationMapComponent::toggleFullMap)
                .def("panFullMap", &ExplorationMapComponent::panFullMap)
                .def("zoomFullMap", &ExplorationMapComponent::zoomFullMap)
                .def("addMarker", &ExplorationMapComponent::addMarker)
                .def("removeMarker", &ExplorationMapComponent::removeMarker)
                .def("setMarkerState", &ExplorationMapComponent::setMarkerState)
                .def("getMarkerState", &ExplorationMapComponent::getMarkerState)
                .def("setMarkerTypeDisplayName", &ExplorationMapComponent::setMarkerTypeDisplayName)
                .def("reactOnMarkerClicked", &ExplorationMapComponent::reactOnMarkerClicked)
                .def("revealScene", &ExplorationMapComponent::revealScene)
                .def("isSceneRevealed", &ExplorationMapComponent::isSceneRevealed)
                .def("revealAllScenes", &ExplorationMapComponent::revealAllScenes)
                .def("getCompletionPercent", &ExplorationMapComponent::getCompletionPercent)
                .def("resetExploration", &ExplorationMapComponent::resetExploration)
                .def("setSceneColor", &ExplorationMapComponent::setSceneColor)
                .def("setSceneDisplayName", &ExplorationMapComponent::setSceneDisplayName)];

        LuaScriptApi::getInstance()->addClassToCollection("ExplorationMapComponent", "class inherits GameObjectComponent", ExplorationMapComponent::getStaticInfoText());
        LuaScriptApi::getInstance()->addClassToCollection("ExplorationMapComponent", "void setActivated(bool activated)", "Sets whether the map is active. Deactivated, nothing is shown.");
        LuaScriptApi::getInstance()->addClassToCollection("ExplorationMapComponent", "bool isActivated()", "Gets whether the map is active.");
        LuaScriptApi::getInstance()->addClassToCollection("ExplorationMapComponent", "void showMiniMap(bool show)", "Shows or hides the mini map in the corner.");
        LuaScriptApi::getInstance()->addClassToCollection("ExplorationMapComponent", "bool isMiniMapShown()", "Gets whether the mini map is shown.");
        LuaScriptApi::getInstance()->addClassToCollection("ExplorationMapComponent", "void showFullMap(bool show)", "Shows or hides the full map. Hiding it returns to the mini map, if that is enabled.");
        LuaScriptApi::getInstance()->addClassToCollection("ExplorationMapComponent", "bool isFullMapShown()", "Gets whether the full map is shown.");
        LuaScriptApi::getInstance()->addClassToCollection("ExplorationMapComponent", "void toggleFullMap()", "Toggles the full map.");
        LuaScriptApi::getInstance()->addClassToCollection("ExplorationMapComponent", "void panFullMap(float deltaX, float deltaY)", "Moves the full map by the given pixels.");
        LuaScriptApi::getInstance()->addClassToCollection("ExplorationMapComponent", "void zoomFullMap(float factor)", "Zooms the full map, e.g. 1.25 zooms in, 0.8 zooms out.");
        LuaScriptApi::getInstance()->addClassToCollection("ExplorationMapComponent", "void addMarker(String markerId, String sceneName, Vector3 position, String type, String label)",
            "Adds or replaces a marker. Scene name empty = current scene. Types: 'SavePoint', 'Teleporter', 'Boss', 'Item', 'Shop', 'LockedDoor', 'Secret', 'Custom'.");
        LuaScriptApi::getInstance()->addClassToCollection("ExplorationMapComponent", "void removeMarker(String markerId)", "Removes a marker added via addMarker.");
        LuaScriptApi::getInstance()->addClassToCollection("ExplorationMapComponent", "void setMarkerState(String markerId, String state)",
            "Sets the marker state: 'normal', 'collected' (item taken, boss defeated) or 'hidden'. Kept across scene changes. For markers configured as attribute, the id is their 'Marker Target'.");
        LuaScriptApi::getInstance()->addClassToCollection("ExplorationMapComponent", "String getMarkerState(String markerId)", "Gets the marker state.");
        LuaScriptApi::getInstance()->addClassToCollection("ExplorationMapComponent", "void setMarkerTypeDisplayName(String type, String displayName)", "Sets the legend text for a marker type, e.g. for localization.");
        LuaScriptApi::getInstance()->addClassToCollection("ExplorationMapComponent", "void reactOnMarkerClicked(func closure, markerId)", "Called when a marker is clicked on the full map, e.g. for fast travel via teleporters.");
        LuaScriptApi::getInstance()->addClassToCollection("ExplorationMapComponent", "void revealScene(String sceneName)", "Reveals a whole scene without exploring it (map station, map item). Drawn hatched.");
        LuaScriptApi::getInstance()->addClassToCollection("ExplorationMapComponent", "bool isSceneRevealed(String sceneName)", "Gets whether a scene is revealed.");
        LuaScriptApi::getInstance()->addClassToCollection("ExplorationMapComponent", "void revealAllScenes()", "Reveals all scenes on the map.");
        LuaScriptApi::getInstance()->addClassToCollection("ExplorationMapComponent", "float getCompletionPercent()", "Gets the explored cells of all scenes on the map in percent (0 - 100).");
        LuaScriptApi::getInstance()->addClassToCollection("ExplorationMapComponent", "void resetExploration()", "Forgets explored cells, revealed scenes and marker states, e.g. for a new game.");
        LuaScriptApi::getInstance()->addClassToCollection("ExplorationMapComponent", "void setSceneColor(String sceneName, Vector3 color)", "Sets the room colour of a scene.");
        LuaScriptApi::getInstance()->addClassToCollection("ExplorationMapComponent", "void setSceneDisplayName(String sceneName, String displayName)", "Sets the name shown on the full map for a scene.");

        gameObjectClass.def("getExplorationMapComponent", (ExplorationMapComponent * (*)(GameObject*)) & getExplorationMapComponent);
        gameObjectClass.def("getExplorationMapComponentFromIndex", (ExplorationMapComponent * (*)(GameObject*, unsigned int)) & getExplorationMapComponentFromIndex);
        gameObjectClass.def("getExplorationMapComponentFromName", &getExplorationMapComponentFromName);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ExplorationMapComponent getExplorationMapComponentFromIndex(unsigned int occurrenceIndex)", "Gets the exploration map component by the given occurence index.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ExplorationMapComponent getExplorationMapComponent()", "Gets the exploration map component.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ExplorationMapComponent getExplorationMapComponentFromName(String name)", "Gets the exploration map component.");

        gameObjectController.def("castExplorationMapComponent", &GameObjectController::cast<ExplorationMapComponent>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "ExplorationMapComponent castExplorationMapComponent(ExplorationMapComponent other)", "Casts an incoming type from function for lua auto completion.");
    }

    bool ExplorationMapComponent::canStaticAddComponent(GameObject* gameObject)
    {
        // Only once per game object.
        return nullptr == makeStrongPtr<ExplorationMapComponent>(gameObject->getComponent<ExplorationMapComponent>());
    }

}; // namespace end
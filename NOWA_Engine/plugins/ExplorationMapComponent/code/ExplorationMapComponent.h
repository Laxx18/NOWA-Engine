/*
Copyright (c) 2025 Lukas Kalinowski

GPL v3
*/

#ifndef EXPLORATION_MAP_COMPONENT_PLUGIN_H
#define EXPLORATION_MAP_COMPONENT_PLUGIN_H

#include "OgrePlugin.h"
#include "gameobject/GameObjectComponent.h"
#include "main/Events.h"
#include "modules/MiniMapModule.h"

namespace MyGUI
{
    class Widget;
    class ImageBox;
    class TextBox;
}

namespace NOWA
{
    /**
     * @class	ExplorationMapComponent
     * @brief	A Metroid / Castlevania like exploration map.
     *
     *			- All scenes connected via exits ("doors") with the current scene are laid out on a grid of cells (see MiniMapModule::computeSceneLayout).
     *			  Each scene becomes a room of cells, sized by its bounds and 'Cell Size'.
     *			- Cells the target (the player) walks through are explored and drawn in the room colour. Scenes revealed via revealScene (map station,
     *			  map item) are drawn hatched. Unknown cells are not drawn.
     *			- Room borders, door gaps, a pulsing player cell and markers (save points, teleporters, bosses, items, shops, locked doors, secrets, custom).
     *			- Two views: a small mini map in a corner, which follows the player, and a full map (e.g. for the pause menu) with completion, legend,
     *			  scene names, zoom, panning and clickable markers.
     *			- The exploration state is kept in the MiniMapModule, so it survives scene changes (this component is re-created with every scene).
     *
     *			Place it on a global game object, e.g. the MainGameObject in global.scene. The graphics come from 'ExplorationMap.png'.
     */
    class EXPORTED ExplorationMapComponent : public GameObjectComponent, public Ogre::Plugin
    {
    public:
        typedef boost::shared_ptr<ExplorationMapComponent> ExplorationMapCompPtr;

        enum class MapMode
        {
            HIDDEN,
            MINI,
            FULL
        };

    public:
        ExplorationMapComponent();

        virtual ~ExplorationMapComponent();

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
         * @see		GameObjectComponent::onRemoveComponent
         */
        virtual void onRemoveComponent(void);

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

        static unsigned int getStaticClassId(void)
        {
            return NOWA::getIdFromName("ExplorationMapComponent");
        }

        static Ogre::String getStaticClassName(void)
        {
            return "ExplorationMapComponent";
        }

        virtual void update(Ogre::Real dt, bool notSimulating = false) override;

        /**
         * @see		GameObjectComponent::actualizeValue
         */
        virtual void actualizeValue(Variant* attribute) override;

        /**
         * @see		GameObjectComponent::writeXML
         */
        virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

        /**
         * @see		GameObjectComponent::setActivated
         */
        virtual void setActivated(bool activated) override;

        virtual bool isActivated(void) const override;

        /////////////////////////////////////////////////////////////////////////////
        // Attributes

        void setTargetId(unsigned long targetId);

        unsigned long getTargetId(void) const;

        void setCellSize(Ogre::Real cellSize);

        Ogre::Real getCellSize(void) const;

        void setMarkerCount(unsigned int markerCount);

        unsigned int getMarkerCount(void) const;

        /////////////////////////////////////////////////////////////////////////////
        // Views

        /**
         * @brief Shows or hides the small mini map in the corner.
         */
        void showMiniMap(bool show);

        bool isMiniMapShown(void) const;

        /**
         * @brief Shows or hides the full map. Hiding it returns to the mini map, if that is enabled.
         */
        void showFullMap(bool show);

        bool isFullMapShown(void) const;

        void toggleFullMap(void);

        /**
         * @brief Moves the full map by the given pixels.
         */
        void panFullMap(Ogre::Real deltaX, Ogre::Real deltaY);

        /**
         * @brief Zooms the full map, e.g. 1.25 zooms in, 0.8 zooms out.
         */
        void zoomFullMap(Ogre::Real factor);

        /////////////////////////////////////////////////////////////////////////////
        // Markers

        /**
         * @brief Adds (or replaces) a marker at a world position of a scene.
         * @param[in] markerId		Unique id, used for removeMarker, setMarkerState and reactOnMarkerClicked.
         * @param[in] sceneName		The scene the position belongs to. Empty for the current scene.
         * @param[in] position		World position in that scene.
         * @param[in] type			'SavePoint', 'Teleporter', 'Boss', 'Item', 'Shop', 'LockedDoor', 'Secret' or 'Custom'.
         * @param[in] label			Free text, e.g. for the legend or a script.
         */
        void addMarker(const Ogre::String& markerId, const Ogre::String& sceneName, const Ogre::Vector3& position, const Ogre::String& type, const Ogre::String& label);

        void removeMarker(const Ogre::String& markerId);

        /**
         * @brief Sets the marker state: 'normal', 'collected' (item taken, boss defeated) or 'hidden'. Kept across scene changes.
         */
        void setMarkerState(const Ogre::String& markerId, const Ogre::String& state);

        Ogre::String getMarkerState(const Ogre::String& markerId) const;

        /**
         * @brief Sets the text shown in the legend for a marker type, e.g. for localization.
         */
        void setMarkerTypeDisplayName(const Ogre::String& type, const Ogre::String& displayName);

        /**
         * @brief Lua closure function gets called, when a marker is clicked on the full map. Signature: function(markerId).
         */
        void reactOnMarkerClicked(luabind::object closureFunction);

        /////////////////////////////////////////////////////////////////////////////
        // Exploration

        /**
         * @brief Reveals a whole scene without exploring it (map station, map item). Revealed cells are drawn hatched.
         */
        void revealScene(const Ogre::String& sceneName);

        bool isSceneRevealed(const Ogre::String& sceneName) const;

        void revealAllScenes(void);

        /**
         * @brief Gets the explored cells of all scenes on the map in percent (0 .. 100).
         */
        Ogre::Real getCompletionPercent(void) const;

        /**
         * @brief Forgets all explored cells, revealed scenes and marker states, e.g. for a new game.
         */
        void resetExploration(void);

        /**
         * @brief Sets the room colour of a scene. Without it, each scene gets a colour from a palette.
         */
        void setSceneColor(const Ogre::String& sceneName, const Ogre::Vector3& color);

        /**
         * @brief Sets the name shown on the map for a scene, e.g. 'Crystal Caves'. Optional: default is the scene name. Shown only with 'Show Scene Names'.
         */
        void setSceneDisplayName(const Ogre::String& sceneName, const Ogre::String& displayName);

    public:
        /**
         * @see		GameObjectComponent::canStaticAddComponent
         */
        static bool canStaticAddComponent(GameObject* gameObject);

        /**
         * @see	GameObjectComponent::getStaticInfoText
         */
        static Ogre::String getStaticInfoText(void)
        {
            return "Usage: A Metroid / Castlevania like exploration map. All scenes connected via exit components (doors) with the current scene are laid out on a grid of cells. "
                   "Cells the target (e.g. the player) walks through are explored, scenes can be revealed via Lua (map station). "
                   "Shows room borders, door gaps, the player and markers (save points, teleporters, bosses, items, shops, locked doors, secrets). "
                   "Two views: a mini map in a corner and a full map with completion, legend, zoom and clickable markers. "
                   "Requirements: Place it on a global game object, e.g. the MainGameObject. Each scene needs saved bounds. The graphics come from 'ExplorationMap.png'.";
        }

        /**
         * @see	GameObjectComponent::createStaticApiForLua
         */
        static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController);

    public:
        static const Ogre::String AttrActivated(void)
        {
            return "Activated";
        }
        static const Ogre::String AttrTargetId(void)
        {
            return "Target Id";
        }
        static const Ogre::String AttrCellSize(void)
        {
            return "Cell Size";
        }
        static const Ogre::String AttrAxis(void)
        {
            return "Axis";
        }
        static const Ogre::String AttrMiniMapPosition(void)
        {
            return "Mini Map Position";
        }
        static const Ogre::String AttrMiniMapSize(void)
        {
            return "Mini Map Size";
        }
        static const Ogre::String AttrMiniMapCellPixels(void)
        {
            return "Mini Map Cell Pixels";
        }
        static const Ogre::String AttrFullMapCellPixels(void)
        {
            return "Full Map Cell Pixels";
        }
        static const Ogre::String AttrShowMiniMapOnStart(void)
        {
            return "Show Mini Map On Start";
        }
        static const Ogre::String AttrLayer(void)
        {
            return "Layer";
        }
        static const Ogre::String AttrShowSceneNames(void)
        {
            return "Show Scene Names";
        }
        static const Ogre::String AttrShowUnexploredMarkers(void)
        {
            return "Show Unexplored Markers";
        }
        static const Ogre::String AttrBackgroundColor(void)
        {
            return "Background Color";
        }
        static const Ogre::String AttrFrameColor(void)
        {
            return "Frame Color";
        }
        static const Ogre::String AttrDoorColor(void)
        {
            return "Door Color";
        }
        static const Ogre::String AttrCompletionLabel(void)
        {
            return "Completion Label";
        }
        static const Ogre::String AttrMarkerCount(void)
        {
            return "Marker Count";
        }
        static const Ogre::String AttrMarkerType(void)
        {
            return "Marker Type ";
        }
        static const Ogre::String AttrMarkerTarget(void)
        {
            return "Marker Target ";
        }
        static const Ogre::String AttrMarkerLabel(void)
        {
            return "Marker Label ";
        }

    private:
        /**
         * @brief A scene as a room of cells on the map grid.
         */
        struct MapScene
        {
            Ogre::String sceneName;
            Ogre::Vector2 worldOrigin;
            Ogre::Vector2 worldSize;
            int cellX0;
            int cellY0;
            int cellWidth;
            int cellHeight;
            std::vector<SceneExitInfo> exits;
        };

        struct MapMarker
        {
            Ogre::String id;
            Ogre::String sceneName;
            Ogre::Vector3 worldPosition;
            Ogre::String type;
            Ogre::String label;
        };

        /**
         * @brief Everything needed to draw one cell. Prepared on the logic thread, so the render thread does not touch logic data.
         */
        struct CellDraw
        {
            int cellX;
            int cellY;
            bool explored;
            Ogre::Vector3 color;
            Ogre::Vector3 frameColor;
            Ogre::Vector3 doorColor;
            unsigned int borderMask;
            // Map space directions of the exits within this cell.
            std::vector<Ogre::Vector2> doorDirections;
        };

        struct MarkerDraw
        {
            Ogre::String id;
            Ogre::String type;
            Ogre::String state;
            // Continuous cell coordinates of the marker.
            Ogre::Vector2 cellSpace;
        };

        struct OverlayDraw
        {
            std::vector<MarkerDraw> markers;
            bool showUnexploredMarkers;
        };

    private:
        // Logic thread

        /**
         * @brief Lays out all scenes connected with the current scene on the cell grid.
         */
        void buildLayout(void);

        /**
         * @brief Resolves the markers configured via attributes ('Scene:Id' or 'Id') into world positions.
         */
        void resolveAttributeMarkers(void);

        std::vector<MapMarker> getAllMarkers(void) const;

        /**
         * @brief Explores the cell the target stands in. Returns true, if it was not explored before.
         */
        bool exploreCurrentCell(int& cellX, int& cellY);

        bool getTargetCellSpace(Ogre::Vector2& cellSpace) const;

        /**
         * @brief Converts a world position of a scene into continuous cell coordinates of the map grid (y up).
         */
        Ogre::Vector2 toCellSpace(size_t sceneIndex, const Ogre::Vector3& worldPosition) const;

        int getCellOwner(int cellX, int cellY) const;

        Ogre::Vector3 getSceneColor(const Ogre::String& sceneName) const;

        /**
         * @brief Prepares the drawing data of a cell. Reads logic data: call on the logic thread, or inside a blocking render command.
         */
        CellDraw prepareCellDraw(size_t sceneIndex, int cellX, int cellY, bool explored) const;

        /**
         * @brief Prepares the drawing data of all markers. Reads logic data, see prepareCellDraw.
         */
        OverlayDraw prepareOverlayDraw(void) const;

        Ogre::String getCompletionCaption(void) const;

        /**
         * @brief Rebuilds the current view on the render thread (blocking - only on mode changes, reveals, attribute changes).
         */
        void rebuildView(void);

        void destroyViewBlocking(void);

        // Render thread

        /**
         * @brief Builds the whole view. Runs inside a blocking render command, so it may read logic data while the logic thread waits.
         */
        void buildView(MapMode mode, const Ogre::Vector2& targetCellSpace, bool targetValid, const Ogre::String& completionCaption);

        void destroyView(void);

        /**
         * @brief Draws one cell (fill, walls, door gaps). Uses only the given data and render thread members.
         */
        void drawCell(const CellDraw& cellDraw);

        /**
         * @brief (Re)creates everything drawn on top of the cells: markers, the player cell and the player. Called after cells were added,
         *		  so they stay on top. Uses only the given data and render thread members.
         */
        void drawOverlays(const OverlayDraw& overlayDraw);

        void destroyOverlays(void);

        void setCompletionCaption(const Ogre::String& completionCaption);

        /**
         * @brief Gets the top left pixel position of a cell in the map content (size: viewCellPixels).
         */
        std::pair<int, int> getCellPixelPosition(int cellX, int cellY) const;

        Ogre::Vector2 cellSpaceToPixel(const Ogre::Vector2& cellSpace) const;

        bool isCellRendered(int cellX, int cellY) const;

        void onMarkerClicked(MyGUI::Widget* sender);

        // Render thread, every frame via tracked closure. Uses only captured values and render thread members.
        void updateTargetView(const Ogre::Vector2& targetCellSpace, bool targetValid, Ogre::Real renderDt);

    private:
        Ogre::String name;

        Variant* activated;
        Variant* targetId;
        Variant* cellSize;
        Variant* axis;
        Variant* miniMapPosition;
        Variant* miniMapSize;
        Variant* miniMapCellPixels;
        Variant* fullMapCellPixels;
        Variant* showMiniMapOnStart;
        Variant* layer;
        Variant* showSceneNames;
        Variant* showUnexploredMarkers;
        Variant* backgroundColor;
        Variant* frameColor;
        Variant* doorColor;
        Variant* completionLabel;
        Variant* markerCount;
        std::vector<Variant*> markerTypes;
        std::vector<Variant*> markerTargets;
        std::vector<Variant*> markerLabels;

        // Logic thread data. The render thread reads it only inside enqueueAndWait commands, while the logic thread waits.
        bool bConnected;
        bool xyAxisUsed;
        std::vector<MapScene> mapScenes;
        std::map<Ogre::String, size_t> sceneIndices;
        std::map<std::pair<int, int>, int> cellOwners;
        int gridMinX;
        int gridMinY;
        int gridMaxX;
        int gridMaxY;
        int currentSceneIndex;
        std::pair<int, int> lastTargetCell;
        bool hasLastTargetCell;
        Ogre::Real exploreTimer;
        MapMode mapMode;
        bool miniMapEnabled;
        std::vector<MapMarker> attributeMarkers;
        std::vector<MapMarker> luaMarkers;
        std::map<Ogre::String, Ogre::Vector3> sceneColors;
        std::map<Ogre::String, Ogre::String> sceneDisplayNames;
        std::map<Ogre::String, Ogre::String> markerTypeDisplayNames;
        luabind::object markerClickedClosureFunction;

        // Render thread data
        MapMode builtMode;
        MyGUI::Widget* viewRoot;
        MyGUI::Widget* viewContent;
        MyGUI::TextBox* completionTextBox;
        MyGUI::ImageBox* targetCellHighlight;
        MyGUI::ImageBox* targetImageBox;
        std::vector<MyGUI::Widget*> overlayWidgets;
        std::map<std::pair<int, int>, MyGUI::ImageBox*> cellFillWidgets;
        int viewCellPixels;
        int viewGridMinX;
        int viewGridMaxY;
        Ogre::Vector2 fullMapPan;
        Ogre::Real fullMapZoom;
        Ogre::Real pulseTime;
    };

}; // namespace end

#endif
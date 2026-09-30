#ifndef MINI_MAP_MODULE_H
#define MINI_MAP_MODULE_H

#include "DotSceneImportModule.h"
#include "OgreVector2.h"
#include "defines.h"
#include <map>
#include <set>
#include <vector>

namespace NOWA
{
    /**
     * @class	MiniMapModule
     * @brief	Builds the mini map layout of all scenes, that are connected via exits ("doors") with the current scene.
     *
     *			- Only scenes reachable through exits are part of the map. Scenes like a menu, an intro or a prequel are left out automatically.
     *			- Each scene is placed so that an exit and the location it leads to ('Target Location Name' of the ExitComponent) meet.
     *			  Only the bounds of a scene and the positions RELATIVE to its bounds count, so it does not matter where a level sits in world space.
     *			- Each scene file is read once per app state (cache) and only for bounds, exits and arrival locations.
     *			- Which scenes the player has visited is kept here, so it survives scene changes (the mini map component itself is re-created with
     *			  every scene). Save and load can serialize it via getVisitedScenes / setSceneVisited.
     *			- The same goes for the exploration state of map components like the ExplorationMapComponent: explored cells, revealed scenes and
     *			  marker states.
     */
    class EXPORTED MiniMapModule
    {
    public:
        friend class ExitComponent;
        friend class AppState; // Only AppState may create this class

        struct MiniMapData
        {
            MiniMapData() : position(Ogre::Vector2::ZERO), size(Ogre::Vector2::ZERO), worldOrigin(Ogre::Vector2::ZERO), worldSize(Ogre::Vector2::ZERO), isCurrentScene(false)
            {
            }

            Ogre::String sceneName;
            // Top left corner, relative to the mini map widget (0 .. 1).
            Ogre::Vector2 position;
            // Relative to the mini map widget (0 .. 1).
            Ogre::Vector2 size;
            // Bounds minimum of the scene in its own world coordinates, projected onto the map plane.
            Ogre::Vector2 worldOrigin;
            // Bounds size of the scene, projected onto the map plane.
            Ogre::Vector2 worldSize;
            bool isCurrentScene;
        };

        /**
         * @brief A scene placed on the map, in a common map space (world units, y up).
         */
        struct SceneLayout
        {
            SceneLayout() : mapOffset(Ogre::Vector2::ZERO), worldOrigin(Ogre::Vector2::ZERO), worldSize(Ogre::Vector2::ZERO), entryDirection(Ogre::Vector2::ZERO)
            {
            }

            Ogre::String sceneName;
            // Where the bounds minimum of the scene lies in map space.
            Ogre::Vector2 mapOffset;
            // Bounds minimum of the scene in its own world coordinates, projected onto the map plane.
            Ogre::Vector2 worldOrigin;
            // Bounds size of the scene, projected onto the map plane.
            Ogre::Vector2 worldSize;
            // The scene through whose exit this scene was placed. Empty for the start scene.
            Ogre::String parentSceneName;
            // Direction of that exit in map space (y up). Zero for the start scene.
            Ogre::Vector2 entryDirection;
            std::vector<SceneExitInfo> exits;
        };

    public:
        void destroyContent(void);

        /**
         * @brief Builds the mini map, starting at the current scene and following all exits.
         * @param[in] currentSceneName	The scene the player is in (without '.scene').
         * @param[in] xyAxis			True for a side view (Jump'n'Run), false for a top view (X,Z; -Z is up on the map).
         * @param[in] widgetPixelSize	The pixel size of the mini map widget. The map keeps the aspect ratio of the levels.
         * @param[in] anchorPosition	Relative position in the widget, at which the center of the whole map is placed. 0.5 0.5 is centered.
         * @param[in] scaleFactor		1 fits the whole map into the widget, bigger values zoom in.
         * @return The tiles, sorted by scene name, so that an index always belongs to the same scene.
         */
        std::vector<MiniMapData> parseMinimaps(const Ogre::String& currentSceneName, bool xyAxis, const Ogre::Vector2& widgetPixelSize, const Ogre::Vector2& anchorPosition, Ogre::Real scaleFactor);

        /**
         * @brief Places all scenes connected via exits with the current scene in a common map space (world units, y up), see the class description.
         * @return The scenes in breadth first order: the start scene first, every other scene after the scene it was placed from.
         */
        std::vector<SceneLayout> computeSceneLayout(const Ogre::String& currentSceneName, bool xyAxis);

        /**
         * @brief Gets the position of a game object as stored in the scene file of another scene. Cached.
         */
        std::pair<bool, Ogre::Vector3> getGameObjectPositionInScene(const Ogre::String& sceneName, unsigned long id);

        /**
         * @brief Projects a world position onto the map plane: (x, y) for a side view, (x, -z) for a top view (-Z is up on the map).
         */
        static Ogre::Vector2 toMapPlane(const Ogre::Vector3& position, bool xyAxis);

        /**
         * @brief Converts an exit direction into the map plane, see toMapPlane.
         */
        static Ogre::Vector2 toMapDirection(const Ogre::Vector2& exitDirection, bool xyAxis);

        /**
         * @brief Gets the relative position of a game object on the last built mini map.
         * @param[in] sceneName	Empty for a game object of the current scene (e.g. the global player, its live position is used), else the scene
         *						whose file is searched for the id.
         */
        std::pair<bool, Ogre::Vector2> parseGameObjectMinimapPosition(const Ogre::String& sceneName, unsigned long id, bool xyAxis);

        /**
         * @brief Forgets all read scene files, e.g. after levels have been edited in NOWA-Design.
         */
        void clearSceneCache(void);

        void setSceneVisited(const Ogre::String& sceneName, bool visited);

        bool getIsSceneVisited(const Ogre::String& sceneName) const;

        const std::set<Ogre::String>& getVisitedScenes(void) const;

        /**
         * @brief Marks a map cell of a scene as explored (cell coordinates local to the scene). Also marks the scene as visited.
         */
        void setCellExplored(const Ogre::String& sceneName, int cellX, int cellY);

        bool getIsCellExplored(const Ogre::String& sceneName, int cellX, int cellY) const;

        /**
         * @brief Gets all explored cells, by scene name. For save and load.
         */
        const std::map<Ogre::String, std::set<std::pair<int, int>>>& getExploredCells(void) const;

        /**
         * @brief Reveals a whole scene without exploring it, e.g. via a map station or map item. Shown dimmed on the map.
         */
        void setSceneRevealed(const Ogre::String& sceneName, bool revealed);

        bool getIsSceneRevealed(const Ogre::String& sceneName) const;

        const std::set<Ogre::String>& getRevealedScenes(void) const;

        /**
         * @brief Sets the state of a map marker, e.g. 'collected'. An empty state removes the entry.
         */
        void setMapMarkerState(const Ogre::String& markerId, const Ogre::String& state);

        Ogre::String getMapMarkerState(const Ogre::String& markerId) const;

        const std::map<Ogre::String, Ogre::String>& getMapMarkerStates(void) const;

        /**
         * @brief Forgets visited scenes, explored cells, revealed scenes and marker states, e.g. for a new game.
         */
        void clearExploration(void);

    private:
        const SceneMapInfo& getSceneMapInfo(const Ogre::String& sceneName);

        std::pair<bool, Ogre::Vector2> calculateGameObjectPosition(const Ogre::String& sceneName, const Ogre::Vector3& position, bool xyAxis) const;

    private:
        MiniMapModule(const Ogre::String& appStateName);
        ~MiniMapModule();

    private:
        Ogre::String appStateName;
        std::vector<MiniMapData> resultMiniMapDataList;
        std::map<Ogre::String, SceneMapInfo> sceneMapInfoCache;
        std::set<Ogre::String> visitedScenes;
        std::map<Ogre::String, std::set<std::pair<int, int>>> exploredCells;
        std::set<Ogre::String> revealedScenes;
        std::map<Ogre::String, Ogre::String> mapMarkerStates;
        // Key: scene name + ":" + id
        std::map<Ogre::String, std::pair<bool, Ogre::Vector3>> gameObjectPositionCache;
    };

}; // namespace end

#endif
#include "NOWAPrecompiled.h"
#include "MiniMapModule.h"
#include "gameobject/GameObjectController.h"
#include "main/AppStateManager.h"
#include "main/Core.h"

#include <algorithm>
#include <deque>

namespace
{
    // Free border around the whole map, relative to the widget, on each side.
    const Ogre::Real MAP_MARGIN = 0.05f;
}

namespace NOWA
{
    MiniMapModule::MiniMapModule(const Ogre::String& appStateName) : appStateName(appStateName)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MiniMapModule] Module created");
    }

    MiniMapModule::~MiniMapModule()
    {
    }

    void MiniMapModule::destroyContent(void)
    {
        this->resultMiniMapDataList.clear();
        this->sceneMapInfoCache.clear();
        this->gameObjectPositionCache.clear();
        this->clearExploration();
    }

    void MiniMapModule::clearSceneCache(void)
    {
        this->sceneMapInfoCache.clear();
        this->gameObjectPositionCache.clear();
    }

    void MiniMapModule::clearExploration(void)
    {
        this->visitedScenes.clear();
        this->exploredCells.clear();
        this->revealedScenes.clear();
        this->mapMarkerStates.clear();
    }

    void MiniMapModule::setCellExplored(const Ogre::String& sceneName, int cellX, int cellY)
    {
        if (true == sceneName.empty())
        {
            return;
        }
        this->exploredCells[sceneName].insert(std::make_pair(cellX, cellY));
        this->visitedScenes.insert(sceneName);
    }

    bool MiniMapModule::getIsCellExplored(const Ogre::String& sceneName, int cellX, int cellY) const
    {
        auto it = this->exploredCells.find(sceneName);
        if (this->exploredCells.end() == it)
        {
            return false;
        }
        return it->second.end() != it->second.find(std::make_pair(cellX, cellY));
    }

    const std::map<Ogre::String, std::set<std::pair<int, int>>>& MiniMapModule::getExploredCells(void) const
    {
        return this->exploredCells;
    }

    void MiniMapModule::setSceneRevealed(const Ogre::String& sceneName, bool revealed)
    {
        if (true == sceneName.empty())
        {
            return;
        }

        if (true == revealed)
        {
            this->revealedScenes.insert(sceneName);
        }
        else
        {
            this->revealedScenes.erase(sceneName);
        }
    }

    bool MiniMapModule::getIsSceneRevealed(const Ogre::String& sceneName) const
    {
        return this->revealedScenes.end() != this->revealedScenes.find(sceneName);
    }

    const std::set<Ogre::String>& MiniMapModule::getRevealedScenes(void) const
    {
        return this->revealedScenes;
    }

    void MiniMapModule::setMapMarkerState(const Ogre::String& markerId, const Ogre::String& state)
    {
        if (true == state.empty())
        {
            this->mapMarkerStates.erase(markerId);
            return;
        }
        this->mapMarkerStates[markerId] = state;
    }

    Ogre::String MiniMapModule::getMapMarkerState(const Ogre::String& markerId) const
    {
        auto it = this->mapMarkerStates.find(markerId);
        if (this->mapMarkerStates.end() == it)
        {
            return "";
        }
        return it->second;
    }

    const std::map<Ogre::String, Ogre::String>& MiniMapModule::getMapMarkerStates(void) const
    {
        return this->mapMarkerStates;
    }

    std::pair<bool, Ogre::Vector3> MiniMapModule::getGameObjectPositionInScene(const Ogre::String& sceneName, unsigned long id)
    {
        const Ogre::String key = sceneName + ":" + Ogre::StringConverter::toString(id);
        auto it = this->gameObjectPositionCache.find(key);
        if (this->gameObjectPositionCache.end() != it)
        {
            return it->second;
        }

        DotSceneImportModule dotSceneImportModule(nullptr, Core::getSingletonPtr()->getProjectName(), sceneName + ".scene", "Projects");
        std::pair<bool, Ogre::Vector3> positionData = dotSceneImportModule.parseGameObjectPosition(id);
        this->gameObjectPositionCache.emplace(key, positionData);
        return positionData;
    }

    void MiniMapModule::setSceneVisited(const Ogre::String& sceneName, bool visited)
    {
        if (true == sceneName.empty())
        {
            return;
        }

        if (true == visited)
        {
            this->visitedScenes.insert(sceneName);
        }
        else
        {
            this->visitedScenes.erase(sceneName);
        }
    }

    bool MiniMapModule::getIsSceneVisited(const Ogre::String& sceneName) const
    {
        return this->visitedScenes.end() != this->visitedScenes.find(sceneName);
    }

    const std::set<Ogre::String>& MiniMapModule::getVisitedScenes(void) const
    {
        return this->visitedScenes;
    }

    Ogre::Vector2 MiniMapModule::toMapPlane(const Ogre::Vector3& position, bool xyAxis)
    {
        if (true == xyAxis)
        {
            return Ogre::Vector2(position.x, position.y);
        }
        // Top view: -Z is up on the map.
        return Ogre::Vector2(position.x, -position.z);
    }

    Ogre::Vector2 MiniMapModule::toMapDirection(const Ogre::Vector2& exitDirection, bool xyAxis)
    {
        if (true == xyAxis)
        {
            return exitDirection;
        }
        // For 'X,Z' the second component of an exit direction is z - flipped like in toMapPlane.
        return Ogre::Vector2(exitDirection.x, -exitDirection.y);
    }

    const SceneMapInfo& MiniMapModule::getSceneMapInfo(const Ogre::String& sceneName)
    {
        auto it = this->sceneMapInfoCache.find(sceneName);
        if (this->sceneMapInfoCache.end() != it)
        {
            return it->second;
        }

        // Read once, only bounds, exits and arrival locations - no game objects are created.
        DotSceneImportModule dotSceneImportModule(nullptr, Core::getSingletonPtr()->getProjectName(), sceneName + ".scene", "Projects");
        SceneMapInfo sceneMapInfo = dotSceneImportModule.parseSceneMapInfo();

        if (false == sceneMapInfo.valid)
        {
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_NORMAL, "[MiniMapModule] Scene: '" + sceneName + "' does not exist or could not be read. It is left out of the mini map.");
        }
        else if (false == sceneMapInfo.hasBounds)
        {
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[MiniMapModule] Scene: '" + sceneName + "' has no bounds yet. Save it once in NOWA-Design. It is left out of the mini map.");
        }

        // A missing scene is cached as well, so it is not searched again for every exit that leads to it.
        auto result = this->sceneMapInfoCache.emplace(sceneName, sceneMapInfo);
        return result.first->second;
    }

    std::vector<MiniMapModule::SceneLayout> MiniMapModule::computeSceneLayout(const Ogre::String& currentSceneName, bool xyAxis)
    {
        std::vector<SceneLayout> sceneLayouts;

        if (true == currentSceneName.empty())
        {
            return sceneLayouts;
        }

        auto createSceneLayout = [xyAxis](const SceneMapInfo& sceneMapInfo) -> SceneLayout
        {
            const Ogre::Vector2 a = MiniMapModule::toMapPlane(sceneMapInfo.mostLeftNearPosition, xyAxis);
            const Ogre::Vector2 b = MiniMapModule::toMapPlane(sceneMapInfo.mostRightFarPosition, xyAxis);

            SceneLayout sceneLayout;
            sceneLayout.sceneName = sceneMapInfo.sceneName;
            sceneLayout.worldOrigin = Ogre::Vector2(std::min(a.x, b.x), std::min(a.y, b.y));
            sceneLayout.worldSize = Ogre::Vector2(Ogre::Math::Abs(b.x - a.x), Ogre::Math::Abs(b.y - a.y));
            sceneLayout.exits = sceneMapInfo.exits;
            return sceneLayout;
        };

        const SceneMapInfo& rootSceneMapInfo = this->getSceneMapInfo(currentSceneName);
        if (false == rootSceneMapInfo.hasBounds)
        {
            return sceneLayouts;
        }

        std::set<Ogre::String> placedSceneNames;
        std::deque<size_t> openList;

        sceneLayouts.emplace_back(createSceneLayout(rootSceneMapInfo));
        placedSceneNames.insert(currentSceneName);
        openList.push_back(0);

        // Breadth first through the exits. Each scene is placed once, by the first exit that reaches it.
        while (false == openList.empty())
        {
            // Copy, because sceneLayouts may reallocate below.
            const SceneLayout parent = sceneLayouts[openList.front()];
            openList.pop_front();

            for (const SceneExitInfo& exitInfo : parent.exits)
            {
                if (true == exitInfo.targetSceneName.empty() || placedSceneNames.end() != placedSceneNames.find(exitInfo.targetSceneName))
                {
                    continue;
                }

                // Note: std::map references stay valid when further scenes are inserted into the cache.
                const SceneMapInfo& childSceneMapInfo = this->getSceneMapInfo(exitInfo.targetSceneName);
                if (false == childSceneMapInfo.hasBounds)
                {
                    continue;
                }

                SceneLayout child = createSceneLayout(childSceneMapInfo);
                child.parentSceneName = parent.sceneName;
                child.entryDirection = MiniMapModule::toMapDirection(exitInfo.exitDirection, xyAxis);

                // Where the exit lies in map space.
                const Ogre::Vector2 exitMapPosition = parent.mapOffset + (MiniMapModule::toMapPlane(exitInfo.position, xyAxis) - parent.worldOrigin);

                auto targetLocationIt = childSceneMapInfo.locationPositions.end();
                if (false == exitInfo.targetLocationName.empty())
                {
                    targetLocationIt = childSceneMapInfo.locationPositions.find(exitInfo.targetLocationName);
                }

                if (childSceneMapInfo.locationPositions.end() != targetLocationIt)
                {
                    // The exit and the location it leads to meet. Uses positions relative to the bounds only, so where a level sits in world
                    // space does not matter.
                    child.mapOffset = exitMapPosition - (MiniMapModule::toMapPlane(targetLocationIt->second, xyAxis) - child.worldOrigin);
                }
                else
                {
                    // No matching location in the target scene: put it next to the parent on the side of the exit, centered on the exit.
                    Ogre::Vector2 direction = child.entryDirection;
                    if (true == direction.isZeroLength())
                    {
                        direction = Ogre::Vector2::UNIT_X;
                    }

                    if (Ogre::Math::Abs(direction.x) >= Ogre::Math::Abs(direction.y))
                    {
                        if (direction.x > 0.0f)
                        {
                            child.mapOffset.x = parent.mapOffset.x + parent.worldSize.x;
                        }
                        else
                        {
                            child.mapOffset.x = parent.mapOffset.x - child.worldSize.x;
                        }
                        child.mapOffset.y = exitMapPosition.y - child.worldSize.y * 0.5f;
                    }
                    else
                    {
                        if (direction.y > 0.0f)
                        {
                            child.mapOffset.y = parent.mapOffset.y + parent.worldSize.y;
                        }
                        else
                        {
                            child.mapOffset.y = parent.mapOffset.y - child.worldSize.y;
                        }
                        child.mapOffset.x = exitMapPosition.x - child.worldSize.x * 0.5f;
                    }
                }

                placedSceneNames.insert(child.sceneName);
                sceneLayouts.emplace_back(child);
                openList.push_back(sceneLayouts.size() - 1);
            }
        }

        return sceneLayouts;
    }

    std::vector<MiniMapModule::MiniMapData> MiniMapModule::parseMinimaps(const Ogre::String& currentSceneName, bool xyAxis, const Ogre::Vector2& widgetPixelSize, const Ogre::Vector2& anchorPosition, Ogre::Real scaleFactor)
    {
        this->resultMiniMapDataList.clear();

        if (widgetPixelSize.x <= 0.0f || widgetPixelSize.y <= 0.0f)
        {
            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[MiniMapModule] Could not create mini map because the widget size is zero!");
            return this->resultMiniMapDataList;
        }

        if (scaleFactor <= 0.0f)
        {
            scaleFactor = 1.0f;
        }

        const std::vector<SceneLayout> sceneLayouts = this->computeSceneLayout(currentSceneName, xyAxis);
        if (true == sceneLayouts.empty())
        {
            return this->resultMiniMapDataList;
        }

        // Extent of the whole map.
        Ogre::Vector2 mapMin(Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY);
        Ogre::Vector2 mapMax(Ogre::Math::NEG_INFINITY, Ogre::Math::NEG_INFINITY);
        for (const SceneLayout& sceneLayout : sceneLayouts)
        {
            mapMin.x = std::min(mapMin.x, sceneLayout.mapOffset.x);
            mapMin.y = std::min(mapMin.y, sceneLayout.mapOffset.y);
            mapMax.x = std::max(mapMax.x, sceneLayout.mapOffset.x + sceneLayout.worldSize.x);
            mapMax.y = std::max(mapMax.y, sceneLayout.mapOffset.y + sceneLayout.worldSize.y);
        }

        const Ogre::Vector2 mapSize(std::max(mapMax.x - mapMin.x, 0.001f), std::max(mapMax.y - mapMin.y, 0.001f));

        // One scale for both axes (in pixels), so levels keep their aspect ratio. At scale factor 1 the whole map fits into the widget.
        const Ogre::Real usable = 1.0f - 2.0f * MAP_MARGIN;
        const Ogre::Real pixelsPerUnit = std::min(widgetPixelSize.x * usable / mapSize.x, widgetPixelSize.y * usable / mapSize.y) * scaleFactor;
        const Ogre::Vector2 relativePerUnit(pixelsPerUnit / widgetPixelSize.x, pixelsPerUnit / widgetPixelSize.y);

        // The center of the map is placed at the anchor position.
        const Ogre::Vector2 mapTopLeft = anchorPosition - (mapSize * relativePerUnit) * 0.5f;

        for (const SceneLayout& sceneLayout : sceneLayouts)
        {
            MiniMapData miniMapData;
            miniMapData.sceneName = sceneLayout.sceneName;
            miniMapData.size = sceneLayout.worldSize * relativePerUnit;
            miniMapData.position.x = mapTopLeft.x + (sceneLayout.mapOffset.x - mapMin.x) * relativePerUnit.x;
            // Map space y grows upwards, widget y downwards.
            miniMapData.position.y = mapTopLeft.y + (mapMax.y - (sceneLayout.mapOffset.y + sceneLayout.worldSize.y)) * relativePerUnit.y;
            miniMapData.worldOrigin = sceneLayout.worldOrigin;
            miniMapData.worldSize = sceneLayout.worldSize;
            miniMapData.isCurrentScene = sceneLayout.sceneName == currentSceneName;
            this->resultMiniMapDataList.emplace_back(miniMapData);
        }

        // Stable order: the per tile attributes of the mini map component (skin, color, tooltip) are stored by index.
        std::sort(this->resultMiniMapDataList.begin(), this->resultMiniMapDataList.end(),
            [](const MiniMapData& a, const MiniMapData& b)
            {
                return a.sceneName < b.sceneName;
            });

        return this->resultMiniMapDataList;
    }

    std::pair<bool, Ogre::Vector2> MiniMapModule::parseGameObjectMinimapPosition(const Ogre::String& sceneName, unsigned long id, bool xyAxis)
    {
        if (true == sceneName.empty())
        {
            // E.g. '522345323': a game object of the current scene, e.g. the global player - its live position.
            auto gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(id);
            if (nullptr == gameObjectPtr)
            {
                return std::make_pair(false, Ogre::Vector2::ZERO);
            }
            return this->calculateGameObjectPosition(Core::getSingletonPtr()->getSceneName(), gameObjectPtr->getPosition(), xyAxis);
        }

        // E.g. 'Level4:522345323': the position stored in that scene file.
        auto positionData = this->getGameObjectPositionInScene(sceneName, id);
        if (false == positionData.first)
        {
            return std::make_pair(false, Ogre::Vector2::ZERO);
        }
        return this->calculateGameObjectPosition(sceneName, positionData.second, xyAxis);
    }

    std::pair<bool, Ogre::Vector2> MiniMapModule::calculateGameObjectPosition(const Ogre::String& sceneName, const Ogre::Vector3& position, bool xyAxis) const
    {
        for (const MiniMapData& miniMapData : this->resultMiniMapDataList)
        {
            if (miniMapData.sceneName != sceneName)
            {
                continue;
            }

            if (miniMapData.worldSize.x <= 0.0f || miniMapData.worldSize.y <= 0.0f)
            {
                return std::make_pair(false, Ogre::Vector2::ZERO);
            }

            // Position within the scene bounds, 0 .. 1 on both axes.
            const Ogre::Vector2 local = (MiniMapModule::toMapPlane(position, xyAxis) - miniMapData.worldOrigin) / miniMapData.worldSize;

            Ogre::Vector2 result;
            result.x = miniMapData.position.x + local.x * miniMapData.size.x;
            // Widget y grows downwards.
            result.y = miniMapData.position.y + (1.0f - local.y) * miniMapData.size.y;
            return std::make_pair(true, result);
        }

        return std::make_pair(false, Ogre::Vector2::ZERO);
    }

} // namespace end
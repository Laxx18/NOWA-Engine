#include "NOWAPrecompiled.h"
#include "OgreRecastModule.h"
#include "gameobject/GameObject.h"
#include "gameobject/NavMeshComponent.h"
#include "gameobject/NavMeshTerraComponent.h"
#include "gameobject/CrowdComponent.h"
#include "main/AppStateManager.h"
#include "main/Core.h"

#include <filesystem>

namespace NOWA
{
    OgreRecastModule::OgreRecastModule(const Ogre::String& appStateName)
		: appStateName(appStateName),
		ogreRecast(nullptr),
		detourTileCache(nullptr),
		detourCrowd(nullptr),
		mustRegenerate(true),
		hasValidNavMesh(false),
		debugDraw(false),
        buildInProgress(false)
	{
		
	}

	OgreRecast* OgreRecastModule::createOgreRecast(Ogre::SceneManager* sceneManager, OgreRecastConfigParams params, const Ogre::Vector3& pointExtends)
    {
        if (nullptr == this->ogreRecast)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this, sceneManager, params, pointExtends]()
            {
                this->ogreRecast = new OgreRecast(sceneManager, params);
                this->ogreRecast->setPointExtents(pointExtends);
                if (nullptr == this->detourTileCache)
                {
                    this->detourTileCache = new OgreDetourTileCache(this->ogreRecast);
                    this->detourCrowd = new OgreDetourCrowd(this->ogreRecast);
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "OgreRecastModule::createOgreRecast");

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[OgreRecastModule] Recast created");

            // Listen on the app-state manager (NavMeshComponent::connect fires here
            // when simulation starts — connect() is called from app-state context).
            AppStateManager::getSingletonPtr()->getEventManager(this->appStateName)->addListener(fastdelegate::MakeDelegate(this, &OgreRecastModule::handleGeometryModified), EventDataGeometryModified::getStaticEventType());
            AppStateManager::getSingletonPtr()->getEventManager(this->appStateName)->addListener(fastdelegate::MakeDelegate(this, &OgreRecastModule::handleGeometryChanged), EventDataGeometryChanged::getStaticEventType());
        }
        else
        {
            this->ogreRecast->configure(params);
            this->ogreRecast->setPointExtents(pointExtends);
        }
        return this->ogreRecast;
    }

	OgreRecastModule::~OgreRecastModule()
	{
		AppStateManager::getSingletonPtr()->getEventManager(this->appStateName)->removeListener(fastdelegate::MakeDelegate(this, &OgreRecastModule::handleGeometryModified), EventDataGeometryModified::getStaticEventType());
        AppStateManager::getSingletonPtr()->getEventManager(this->appStateName)->removeListener(fastdelegate::MakeDelegate(this, &OgreRecastModule::handleGeometryChanged), EventDataGeometryChanged::getStaticEventType());
		this->destroyContent();
	}

	void OgreRecastModule::handleGeometryModified(NOWA::EventDataPtr eventData)
    {
        this->mustRegenerate = true;
    }

    void OgreRecastModule::handleGeometryChanged(NOWA::EventDataPtr eventData)
    {
        boost::shared_ptr<NOWA::EventDataGeometryChanged> castEventData = boost::static_pointer_cast<NOWA::EventDataGeometryChanged>(eventData);

        // Event not for this state
        auto gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(castEventData->getGameObjectId());
        if (nullptr != gameObjectPtr)
        {
            if (gameObjectPtr->getType() == NOWA::TERRA)
            {
                this->mustRegenerate = true;
            }
        }
    }

    bool OgreRecastModule::isBuildInProgress(void) const
    {
        return this->buildInProgress.load();
    }

    bool OgreRecastModule::getHasValidNavMesh(void) const
    {
        return this->hasValidNavMesh;
    }

	void OgreRecastModule::destroyContent(void)
	{
        this->stopSimulation();

        // Must wait for any running navmesh build before freeing its resources.
        // Without this join, the background thread would access freed memory (UAF).
        if (this->navMeshThread.joinable())
        {
            this->navMeshThread.join();
        }

		// Disable debug drawing — enqueue if it affects Ogre scene
		this->debugDrawNavMesh(false);

		// Remove and delete dynamic obstacles
		// Copy dynamicObstacles locally and clear original container early
		auto obstaclesCopy = this->dynamicObstacles;
		this->dynamicObstacles.clear();

		this->buildInProgress = false;

		for (const auto& it : obstaclesCopy)
		{
			InputGeom* inputGeom = it.second.first;
			ConvexVolume* convexVolume = it.second.second;

			if (this->detourTileCache)
			{
				// Capture detourTileCache and convexVolume for render thread removal
				auto detourTileCache = this->detourTileCache;

                NOWA::GraphicsModule::RenderCommand renderCommand = [this, detourTileCache, convexVolume]()
                {
                    detourTileCache->removeConvexShapeObstacle(convexVolume);
                };
                NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "OgreRecastModule::removeConvexObstacle");
			}

			// Delete pure data immediately or after a safe delay if needed
			delete inputGeom;
			delete convexVolume;
		}

		// Delete terraInputGeomCells data immediately (pure data)
		for (const auto& it2 : this->terraInputGeomCells)
		{
			InputGeom* inputGeom = it2.second;
			delete inputGeom;
		}
		this->terraInputGeomCells.clear();

		// Enqueue deletion of ogreRecast object (likely CPU-side but let's be safe)
		if (this->ogreRecast)
		{
			auto ogreRecast = this->ogreRecast;
			this->ogreRecast = nullptr;

            NOWA::GraphicsModule::RenderCommand renderCommand = [this, ogreRecast]()
            {
                delete ogreRecast;
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "OgreRecastModule::destroyOgreRecast");
		}

		// Enqueue deletion of detourTileCache object
		if (this->detourTileCache)
		{
			auto detourTileCache = this->detourTileCache;
			this->detourTileCache = nullptr;

            NOWA::GraphicsModule::RenderCommand renderCommand = [this, detourTileCache]()
            {
                delete detourTileCache;
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "OgreRecastModule::destroyDetourTileCache");
		}

		// Enqueue deletion of detourCrowd object
		if (this->detourCrowd)
		{
			auto detourCrowd = this->detourCrowd;
			this->detourCrowd = nullptr;

            NOWA::GraphicsModule::RenderCommand renderCommand = [this, detourCrowd]()
            {
                delete detourCrowd;
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "OgreRecastModule::destroyDetourCrowd");
		}

		this->hasValidNavMesh = false;
		this->mustRegenerate = true;
		this->staticObstacles.clear();
	}

	bool OgreRecastModule::hasNavigationMeshElements(void) const
	{
		return this->staticObstacles.size() > 0 || this->dynamicObstacles.size() > 0 || this->terraInputGeomCells.size() > 0;
	}

	// ─────────────────────────────────────────────────────────────────────────────
    // addStaticObstacle
    //
    // Registers a game object as a static obstacle (baked into navmesh geometry).
    // Static obstacles are stored in a simple list used by buildNavigationMesh().
    // They are never added directly to the tile cache — they require a full rebuild.
    //
    // Note: any previous dynamic obstacle entry for the same id is removed first,
    // because an object cannot be both static and dynamic at the same time.
    // ─────────────────────────────────────────────────────────────────────────────
    void OgreRecastModule::addStaticObstacle(unsigned long id)
    {
        // Remove from dynamic map first — object cannot be in both lists
        this->removeDynamicObstacle(id, false);

        bool alreadyExisting = false;
        for (auto it = this->staticObstacles.begin(); it != this->staticObstacles.end(); ++it)
        {
            if (id == *it)
            {
                alreadyExisting = true;
                break;
            }
        }

        if (false == alreadyExisting)
        {
            this->staticObstacles.push_back(id);
        }
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // removeStaticObstacle
    //
    // Unregisters a game object from the static obstacle list.
    // Does NOT trigger a navmesh rebuild — caller is responsible for that
    // if the navmesh needs to be updated after removal.
    // ─────────────────────────────────────────────────────────────────────────────
    void OgreRecastModule::removeStaticObstacle(unsigned long id)
    {
        for (auto it = this->staticObstacles.begin(); it != this->staticObstacles.end(); ++it)
        {
            if (id == *it)
            {
                this->staticObstacles.erase(it);
                break;
            }
        }
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // addDynamicObstacle
    //
    // Registers a game object as a dynamic convex shape obstacle in the tile cache.
    // Dynamic obstacles update the navmesh in milliseconds without a full rebuild.
    //
    // Three cases are handled:
    //
    // Case A — Not yet registered, InputGeom exists (tile cache built or loaded):
    //   Create InputGeom from the mesh, compute convex hull, add to tile cache.
    //
    // Case B — Not yet registered, InputGeom does NOT exist yet:
    //   Store {nullptr, nullptr} as a placeholder. createInputGeom() will fill
    //   these placeholders after the first successful build or load.
    //
    // Case C — Already registered with valid geometry (update walkable flag only):
    //   Just update the area flag on the existing convex volume.
    //
    // Case D — Already registered as {nullptr, nullptr} placeholder
    //   (tile cache was built/loaded AFTER the object was registered):
    //   Treat as Case A — create real geometry now and add to tile cache.
    //
    // externalInputGeom / externalConvexVolume:
    //   Optional pre-built geometry (used by NavMeshComponent::update for rotation/
    //   position changes). If provided, ownership is taken — do not delete externally.
    // ─────────────────────────────────────────────────────────────────────────────
    void OgreRecastModule::addDynamicObstacle(unsigned long id, bool walkable, InputGeom* externalInputGeom, ConvexVolume* externalConvexVolume)
    {
        if (nullptr == this->detourTileCache)
        {
            return;
        }

        this->removeStaticObstacle(id);

        auto it = this->dynamicObstacles.find(id);

        if (it == this->dynamicObstacles.end())
        {
            if (false == this->hasValidNavMesh)
            {
                this->dynamicObstacles.emplace(id, std::make_pair(nullptr, nullptr));
                return;
            }

            auto gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(id);
            if (nullptr == gameObjectPtr)
            {
                return;
            }

            Ogre::Item* item = gameObjectPtr->getMovableObject<Ogre::Item>();
            InputGeom* inputGeom = nullptr;
            ConvexVolume* convexVolume = nullptr;

            if (false == this->detourTileCache->isLoadedFromDisk())
            {
                // Built this session — full mesh geometry path
                inputGeom = (nullptr != externalInputGeom) ? externalInputGeom : (nullptr != item ? new InputGeom(item) : nullptr);

                if (nullptr != inputGeom)
                {
                    convexVolume = (nullptr != externalConvexVolume) ? externalConvexVolume : inputGeom->getConvexHull(this->ogreRecast->getAgentRadius() * 2.0f);

                    convexVolume->area = walkable ? RC_WALKABLE_AREA : RC_NULL_AREA;
                    this->detourTileCache->addConvexShapeObstacle(convexVolume);
                }
            }
            else
            {
                // Loaded from disk — addBoxObstacle path, no InputGeom needed
                if (nullptr != externalConvexVolume)
                {
                    convexVolume = externalConvexVolume;
                    inputGeom = externalInputGeom; // may be nullptr, that is fine
                }
                else if (nullptr != externalInputGeom)
                {
                    inputGeom = externalInputGeom;
                    convexVolume = inputGeom->getConvexHull(this->ogreRecast->getAgentRadius() * 2.0f);
                }
                else if (nullptr != item)
                {
                    // Fresh AABB from current world transform.
                    // Called when NavMeshComponent::update() deletes the old ConvexVolume
                    // and passes nullptr — ensures bmin/bmax reflect the actual new position.
                    Ogre::Aabb worldAabb = item->getWorldAabb();
                    const float offset = this->ogreRecast->getAgentRadius() * 2.0f;
                    convexVolume = new ConvexVolume(Ogre::AxisAlignedBox(worldAabb.getMinimum(), worldAabb.getMaximum()), offset);
                }

                if (nullptr != convexVolume)
                {
                    convexVolume->area = walkable ? RC_WALKABLE_AREA : RC_NULL_AREA;
                    this->detourTileCache->addConvexShapeObstacle(convexVolume);
                }
            }

            this->dynamicObstacles.emplace(id, std::make_pair(inputGeom, convexVolume));
        }
        else
        {
            // Entry exists — update area flag only.
            // Position changes are handled by NavMeshComponent::update which calls
            // removeDynamicObstacle(false) + addDynamicObstacle with new geometry,
            // so the map entry has already been erased before this call.
            if (nullptr != it->second.second)
            {
                it->second.second->area = walkable ? RC_WALKABLE_AREA : RC_NULL_AREA;
            }
        }
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // removeDynamicObstacle
    //
    // Unregisters a game object from the dynamic obstacle map and removes its
    // convex shape from the tile cache.
    //
    // destroy = true:  InputGeom and ConvexVolume are deleted (normal removal).
    // destroy = false: Caller takes ownership of the returned pair for reuse
    //                  (used by NavMeshComponent::update for position/rotation changes).
    //
    // Handles all movable types (Entity, Item) and placeholder {nullptr, nullptr}
    // entries gracefully — safe to call even if the object was never fully initialised.
    // ─────────────────────────────────────────────────────────────────────────────
    std::pair<InputGeom*, ConvexVolume*> OgreRecastModule::removeDynamicObstacle(unsigned long id, bool destroy)
    {
        InputGeom* inputGeom = nullptr;
        ConvexVolume* convexVolume = nullptr;

        if (nullptr == this->detourTileCache)
        {
            return std::make_pair(inputGeom, convexVolume);
        }

        auto it = this->dynamicObstacles.find(id);
        if (it == this->dynamicObstacles.end())
        {
            return std::make_pair(inputGeom, convexVolume);
        }

        inputGeom = it->second.first;
        convexVolume = it->second.second;

        // Remove convex hull from tile cache only if it was actually registered.
        // Placeholder entries {nullptr, nullptr} were never added to the tile cache
        // so calling removeConvexShapeObstacle on null would crash.
        if (nullptr != convexVolume)
        {
            this->detourTileCache->removeConvexShapeObstacle(convexVolume);
        }

        // Erase from map regardless of geometry state
        this->dynamicObstacles.erase(it);

        if (true == destroy)
        {
            delete inputGeom;
            inputGeom = nullptr;

            delete convexVolume;
            convexVolume = nullptr;
        }
        // If destroy = false, caller receives ownership via the returned pair

        return std::make_pair(inputGeom, convexVolume);
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // activateDynamicObstacle
    //
    // Enables or disables an existing dynamic obstacle without removing it from
    // the map. Used by NavMeshComponent when the activated flag is toggled at runtime.
    //
    // activate = false: removes the convex hull from the tile cache (agents can walk
    //                   through the area) but keeps the entry in dynamicObstacles.
    // activate = true:  re-adds the convex hull to the tile cache if it was removed.
    //
    // Skips placeholder entries {nullptr, nullptr} silently — they have no geometry
    // to activate/deactivate and will be filled by createInputGeom() later.
    //
    // Note: does NOT check detourTileCache->getInputGeom() — that pointer is null
    // after loadAll() (no InputGeom is created during load, only tile data is restored).
    // The tile cache itself is valid after loadAll() so the obstacle add/remove works.
    // ─────────────────────────────────────────────────────────────────────────────
    void OgreRecastModule::activateDynamicObstacle(unsigned long id, bool activate)
    {
        if (nullptr == this->detourTileCache || false == this->hasValidNavMesh)
        {
            return;
        }

        auto it = this->dynamicObstacles.find(id);
        if (it == this->dynamicObstacles.end())
        {
            return;
        }

        ConvexVolume* convexVolume = it->second.second;
        if (nullptr == convexVolume)
        {
            return;
        }

        if (false == activate)
        {
            this->detourTileCache->removeConvexShapeObstacle(convexVolume);
        }
        else
        {
            int obstacleId = this->detourTileCache->getConvexShapeObstacleId(convexVolume);
            if (-1 == obstacleId)
            {
                this->detourTileCache->addConvexShapeObstacle(convexVolume);
            }
        }
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // addTerra
    //
    // Registers a Terra heightmap game object as a navmesh terrain source.
    // Terra objects are stored in terraInputGeomCells with a null InputGeom pointer
    // (placeholder) until buildNavigationMesh() runs and builds the full geometry.
    //
    // Fires EventDataGeometryModified so the navmesh is marked dirty and will be
    // rebuilt the next time debugDrawNavMesh(true) is called.
    // ─────────────────────────────────────────────────────────────────────────────
    void OgreRecastModule::addTerra(unsigned long id)
    {
        auto it = this->terraInputGeomCells.find(id);
        if (it != this->terraInputGeomCells.end())
        {
            return; // Already registered
        }

        auto gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(id);
        if (nullptr == gameObjectPtr)
        {
            return;
        }

        Ogre::Terra* terra = gameObjectPtr->getMovableObject<Ogre::Terra>();
        if (nullptr == terra)
        {
            return;
        }

        // Store as placeholder — InputGeom is built during the next full rebuild
        this->terraInputGeomCells.emplace(id, nullptr);

        boost::shared_ptr<NOWA::EventDataGeometryModified> evt(new NOWA::EventDataGeometryModified());
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evt);
    }

    // ─────────────────────────────────────────────────────────────────────────────
    // removeTerra
    //
    // Unregisters a Terra heightmap object from the terrain source map.
    //
    // destroy = true:  the cached InputGeom is deleted (normal removal on disconnect).
    // destroy = false: caller receives the InputGeom pointer for inspection/reuse.
    //
    // Fires EventDataGeometryModified so the navmesh is marked dirty — a rebuild
    // is needed to remove the terrain geometry from the baked navmesh.
    // ─────────────────────────────────────────────────────────────────────────────
    InputGeom* OgreRecastModule::removeTerra(unsigned long id, bool destroy)
    {
        InputGeom* inputGeom = nullptr;

        auto it = this->terraInputGeomCells.find(id);
        if (it == this->terraInputGeomCells.end())
        {
            return nullptr; // Not registered
        }

        auto gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(id);
        if (nullptr == gameObjectPtr)
        {
            return nullptr;
        }

        Ogre::Terra* terra = gameObjectPtr->getMovableObject<Ogre::Terra>();
        if (nullptr == terra)
        {
            return nullptr;
        }

        inputGeom = it->second;
        this->terraInputGeomCells.erase(it);

        boost::shared_ptr<NOWA::EventDataGeometryModified> evt(new NOWA::EventDataGeometryModified());
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evt);

        if (true == destroy)
        {
            delete inputGeom;
            inputGeom = nullptr;
        }

        return inputGeom;
    }

    void OgreRecastModule::createInputGeom(void)
    {
        // Fills {nullptr,nullptr} placeholder entries that were stored when
        // addDynamicObstacle() was called before the navmesh existed.
        // Called after a successful build or load.

        for (auto it = this->dynamicObstacles.begin(); it != this->dynamicObstacles.end(); ++it)
        {
            if (nullptr != it->second.first)
            {
                continue; // Already has geometry
            }

            auto gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(it->first);
            if (nullptr == gameObjectPtr)
            {
                continue;
            }

            InputGeom* inputGeom = nullptr;
            ConvexVolume* convexVolume = nullptr;

            Ogre::Item* item = gameObjectPtr->getMovableObject<Ogre::Item>();

            if (this->detourTileCache->getInputGeom() != nullptr)
            {
                // Built this session — use full mesh geometry (original path)
                if (nullptr != item)
                {
                    inputGeom = new InputGeom(item);
                }

                if (nullptr == inputGeom)
                {
                    continue;
                }

                convexVolume = inputGeom->getConvexHull(this->ogreRecast->getAgentRadius() * 2.0f);
            }
            else if (this->detourTileCache->isLoadedFromDisk())
            {
                // Loaded from disk — use AABB (no GPU readback needed)
                if (nullptr != item)
                {
                    Ogre::Aabb worldAabb = item->getWorldAabb();
                    const float offset = this->ogreRecast->getAgentRadius() * 2.0f;
                    convexVolume = new ConvexVolume(Ogre::AxisAlignedBox(worldAabb.getMinimum(), worldAabb.getMaximum()), offset);
                }

                if (nullptr == convexVolume)
                {
                    continue;
                }
            }
            else
            {
                continue; // Navmesh still not ready
            }

            convexVolume->area = RC_NULL_AREA;

            auto navMeshCompPtr = NOWA::makeStrongPtr(gameObjectPtr->getComponent<NavMeshComponent>());
            if (nullptr != navMeshCompPtr && navMeshCompPtr->getWalkable())
            {
                convexVolume->area = RC_WALKABLE_AREA;
            }

            convexVolume->hmin -= 0.1f;

            it->second.first = inputGeom; // nullptr for AABB path — that is fine
            it->second.second = convexVolume;

            this->detourTileCache->addConvexShapeObstacle(convexVolume);
        }
    }

    Ogre::String OgreRecastModule::getNavMeshFilePath(void) const
    {
        return Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "/" + Core::getSingletonPtr()->getSceneName() + ".nav";
    }

    Ogre::String OgreRecastModule::getNavGeomFilePath(void) const
    {
        return Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "/" + Core::getSingletonPtr()->getSceneName() + ".navgeom";
    }

    bool OgreRecastModule::saveNavigationMesh(const InputGeom::NavMeshGeomSnapshot& snapshot)
    {
        if (nullptr == this->detourTileCache)
        {
            return false;
        }

        // ── 1. Save tile cache binary (.nav) ─────────────────────────────────────
        {
            Ogre::String filePath = this->getNavMeshFilePath();
            if (false == this->detourTileCache->saveAll(filePath))
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[OgreRecastModule] saveNavigationMesh: failed to save .nav: " + filePath);
                return false;
            }
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[OgreRecastModule] Tile cache saved to: " + filePath);
        }

        // ── 2. Save mesh geometry (.navgeom) ──────────────────────────────────────
        // Needed so buildTile() has real vertices when obstacles rebuild tiles at runtime.
        // Without this: addConvexShapeObstacle -> rasterizeTileLayers -> isEmpty() = true
        //               -> "ERROR: buildTile: Input mesh is not specified."
        {
            Ogre::String filePath = this->getNavGeomFilePath();
            FILE* fp = fopen(filePath.c_str(), "wb");
            if (nullptr == fp)
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[OgreRecastModule] saveNavigationMesh: failed to open .navgeom: " + filePath);
                return false;
            }

            const unsigned int magic = 0x4E474D00; // "NGM\0"
            const unsigned int version = 1;
            fwrite(&magic, sizeof(unsigned int), 1, fp);
            fwrite(&version, sizeof(unsigned int), 1, fp);

            int entryCount = static_cast<int>(snapshot.meshEntries.size());
            fwrite(&entryCount, sizeof(int), 1, fp);

            for (const auto& entry : snapshot.meshEntries)
            {
                int vertCount = static_cast<int>(entry.verts.size());
                int idxCount = static_cast<int>(entry.indices.size());
                fwrite(&vertCount, sizeof(int), 1, fp);
                fwrite(&idxCount, sizeof(int), 1, fp);
                if (vertCount > 0)
                {
                    fwrite(entry.verts.data(), sizeof(float), vertCount, fp);
                }
                if (idxCount > 0)
                {
                    fwrite(entry.indices.data(), sizeof(int), idxCount, fp);
                }
            }

            fwrite(snapshot.bmin, sizeof(float), 3, fp);
            fwrite(snapshot.bmax, sizeof(float), 3, fp);

            fclose(fp);
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[OgreRecastModule] Mesh geometry saved to: " + filePath);
        }

        return true;
    }

    bool OgreRecastModule::loadNavigationMesh(void)
    {
        Ogre::String navFilePath = this->getNavMeshFilePath();
        Ogre::String geomFilePath = this->getNavGeomFilePath();

        if (false == std::filesystem::exists(navFilePath))
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[OgreRecastModule] No .nav file found — will build from scratch.");
            return false;
        }

        if (nullptr != this->detourTileCache)
        {
            delete this->detourTileCache;
            this->detourTileCache = nullptr;
        }
        this->detourTileCache = new OgreDetourTileCache(this->ogreRecast);

        if (false == this->detourTileCache->loadAll(navFilePath))
        {
            delete this->detourTileCache;
            this->detourTileCache = nullptr;
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[OgreRecastModule] Failed to load .nav — will build from scratch.");
            return false;
        }

        // ── Always set InputGeom — regardless of hasValidNavMesh state ────────────
        // This is critical: even if hasValidNavMesh was already true from a previous
        // load, a new detourTileCache was just created above and its m_geom is null.
        // addConvexShapeObstacle -> addConvexVolume crashes on null/trash m_geom.
        if (true == std::filesystem::exists(geomFilePath))
        {
            FILE* fp = fopen(geomFilePath.c_str(), "rb");
            if (nullptr != fp)
            {
                unsigned int magic = 0, version = 0;
                fread(&magic, sizeof(unsigned int), 1, fp);
                fread(&version, sizeof(unsigned int), 1, fp);

                if (magic == 0x4E474D00 && version == 1)
                {
                    InputGeom::NavMeshGeomSnapshot snapshot;
                    int entryCount = 0;
                    fread(&entryCount, sizeof(int), 1, fp);
                    snapshot.meshEntries.resize(entryCount);
                    for (auto& entry : snapshot.meshEntries)
                    {
                        int vertCount = 0, idxCount = 0;
                        fread(&vertCount, sizeof(int), 1, fp);
                        fread(&idxCount, sizeof(int), 1, fp);
                        entry.verts.resize(vertCount);
                        entry.indices.resize(idxCount);
                        if (vertCount > 0)
                        {
                            fread(entry.verts.data(), sizeof(float), vertCount, fp);
                        }
                        if (idxCount > 0)
                        {
                            fread(entry.indices.data(), sizeof(int), idxCount, fp);
                        }
                    }
                    fread(snapshot.bmin, sizeof(float), 3, fp);
                    fread(snapshot.bmax, sizeof(float), 3, fp);
                    fclose(fp);

                    InputGeom* geom = new InputGeom(snapshot);
                    this->detourTileCache->setInputGeom(geom);
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[OgreRecastModule] Mesh geometry loaded from: " + geomFilePath);
                }
                else
                {
                    fclose(fp);
                    // Invalid file — empty geom prevents crash but tiles won't rebuild
                    this->detourTileCache->setInputGeom(new InputGeom());
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[OgreRecastModule] Invalid .navgeom — delete both files and rebuild.");
                }
            }
            else
            {
                this->detourTileCache->setInputGeom(new InputGeom());
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[OgreRecastModule] Cannot open .navgeom file.");
            }
        }
        else
        {
            // No .navgeom yet — empty geom, tile rebuilds will silently fail
            this->detourTileCache->setInputGeom(new InputGeom());
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[OgreRecastModule] No .navgeom found — rebuild navmesh to generate it.");
        }

        this->hasValidNavMesh = true;
        this->mustRegenerate = false;
        this->createInputGeom();

        auto crowdGOs = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsFromComponent(CrowdComponent::getStaticClassName());
        if (false == crowdGOs.empty())
        {
            this->detourCrowd->setMaxAgents(crowdGOs.size());
        }

        return true;
    }

    void OgreRecastModule::setMustRegenerate(bool mustRegenerate)
    {
        this->mustRegenerate = mustRegenerate;
    }

    void OgreRecastModule::debugLogPath(int pathSlot, unsigned long agentId) const
    {
        if (nullptr == this->ogreRecast)
        {
            return;
        }

        const std::vector<Ogre::Vector3>& path = this->ogreRecast->getPath(pathSlot);
        if (path.empty())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[PathDebug] Agent " + Ogre::StringConverter::toString(agentId) + " pathSlot=" + Ogre::StringConverter::toString(pathSlot) + ": EMPTY path");
            return;
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL,
            "[PathDebug] Agent " + Ogre::StringConverter::toString(agentId) + " pathSlot=" + Ogre::StringConverter::toString(pathSlot) + " waypoints=" + Ogre::StringConverter::toString(path.size()));

        // Collect all active obstacle AABBs for intersection test
        struct ObstacleAABB
        {
            float bmin[3];
            float bmax[3];
        };
        std::vector<ObstacleAABB> obstacles;

        if (nullptr != this->detourTileCache)
        {
            const int nobs = this->detourTileCache->getObstacleCount();
            for (int i = 0; i < nobs; ++i)
            {
                const dtTileCacheObstacle* ob = this->detourTileCache->getObstacle(i);
                if (!ob || ob->state != DT_OBSTACLE_PROCESSED)
                {
                    continue;
                }
                if (ob->type != DT_OBSTACLE_BOX)
                {
                    continue;
                }

                ObstacleAABB aabb;
                aabb.bmin[0] = ob->box.bmin[0];
                aabb.bmin[1] = ob->box.bmin[1];
                aabb.bmin[2] = ob->box.bmin[2];
                aabb.bmax[0] = ob->box.bmax[0];
                aabb.bmax[1] = ob->box.bmax[1];
                aabb.bmax[2] = ob->box.bmax[2];
                obstacles.push_back(aabb);
            }
        }

        for (size_t wi = 0; wi < path.size(); ++wi)
        {
            const Ogre::Vector3& wp = path[wi];

            // Check if this waypoint is inside any obstacle AABB (XZ only — Y ignored)
            bool insideObstacle = false;
            int obstacleIdx = -1;
            for (int oi = 0; oi < (int)obstacles.size(); ++oi)
            {
                const ObstacleAABB& aabb = obstacles[oi];
                if (wp.x >= aabb.bmin[0] && wp.x <= aabb.bmax[0] && wp.z >= aabb.bmin[2] && wp.z <= aabb.bmax[2])
                {
                    insideObstacle = true;
                    obstacleIdx = oi;
                    break;
                }
            }

            Ogre::String msg = "  wp[" + Ogre::StringConverter::toString(wi) + "]=" + Ogre::StringConverter::toString(wp);
            if (insideObstacle)
            {
                msg += " <<< INSIDE OBSTACLE[" + Ogre::StringConverter::toString(obstacleIdx) + "]!";
            }
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, msg);
        }
    }

    void OgreRecastModule::debugDrawObstacleBoxes(Ogre::SceneManager* scnMgr)
    {
        // Destroy previous debug boxes
        static Ogre::SceneNode* debugNode = nullptr;
        if (nullptr != debugNode)
        {
            for (unsigned short i = 0; i < debugNode->numAttachedObjects(); ++i)
            {
                scnMgr->destroyManualObject(static_cast<Ogre::ManualObject*>(debugNode->getAttachedObject(i)));
            }
            scnMgr->destroySceneNode(debugNode);
            debugNode = nullptr;
        }

        if (nullptr == this->detourTileCache)
        {
            return;
        }

        const int nobs = this->detourTileCache->getObstacleCount();
        if (nobs == 0)
        {
            return;
        }

        debugNode = scnMgr->getRootSceneNode()->createChildSceneNode();

        for (int i = 0; i < nobs; ++i)
        {
            const dtTileCacheObstacle* ob = this->detourTileCache->getObstacle(i);
            if (!ob || ob->state != DT_OBSTACLE_PROCESSED)
            {
                continue;
            }
            if (ob->type != DT_OBSTACLE_BOX)
            {
                continue;
            }

            const Ogre::Vector3 bmin(ob->box.bmin[0], ob->box.bmin[1], ob->box.bmin[2]);
            const Ogre::Vector3 bmax(ob->box.bmax[0], ob->box.bmax[1], ob->box.bmax[2]);
            // Draw at agent height only — full Y range [-50,50] is not useful visually
            const float drawY = (ob->box.bmin[0] + ob->box.bmax[0]) * 0.0f; // unused
            const float groundY = 0.5f;                                     // slightly above terrain for visibility

            Ogre::ManualObject* mo = scnMgr->createManualObject();
            mo->setName("ObstacleDebug_" + Ogre::StringConverter::toString(i));
            mo->setRenderQueueGroup(10);
            mo->begin("recastdebug", Ogre::OT_LINE_LIST);

            // Bottom face
            const float y = groundY;
            mo->position(bmin.x, y, bmin.z);
            mo->colour(1, 1, 0, 1);
            mo->position(bmax.x, y, bmin.z);
            mo->colour(1, 1, 0, 1);
            mo->position(bmax.x, y, bmin.z);
            mo->colour(1, 1, 0, 1);
            mo->position(bmax.x, y, bmax.z);
            mo->colour(1, 1, 0, 1);
            mo->position(bmax.x, y, bmax.z);
            mo->colour(1, 1, 0, 1);
            mo->position(bmin.x, y, bmax.z);
            mo->colour(1, 1, 0, 1);
            mo->position(bmin.x, y, bmax.z);
            mo->colour(1, 1, 0, 1);
            mo->position(bmin.x, y, bmin.z);
            mo->colour(1, 1, 0, 1);

            // Vertical lines at corners
            const float y2 = y + 2.0f;
            mo->position(bmin.x, y, bmin.z);
            mo->colour(1, 0.5f, 0, 1);
            mo->position(bmin.x, y2, bmin.z);
            mo->colour(1, 0.5f, 0, 1);
            mo->position(bmax.x, y, bmin.z);
            mo->colour(1, 0.5f, 0, 1);
            mo->position(bmax.x, y2, bmin.z);
            mo->colour(1, 0.5f, 0, 1);
            mo->position(bmax.x, y, bmax.z);
            mo->colour(1, 0.5f, 0, 1);
            mo->position(bmax.x, y2, bmax.z);
            mo->colour(1, 0.5f, 0, 1);
            mo->position(bmin.x, y, bmax.z);
            mo->colour(1, 0.5f, 0, 1);
            mo->position(bmin.x, y2, bmax.z);
            mo->colour(1, 0.5f, 0, 1);

            for (uint32_t idx = 0; idx < 16; ++idx)
            {
                mo->index(idx);
            }

            mo->end();
            debugNode->attachObject(mo);
        }

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[ObstacleDebug] Drew " + Ogre::StringConverter::toString(nobs) + " obstacle AABB boxes in yellow.");
    }

    // In OgreRecastModule: neue Methode für Simulationsstopp
    void OgreRecastModule::stopSimulation()
    {
        // Collect and wait for all in-flight pathfinding tasks.
        // Must happen before components disconnect and before dtNavMeshQuery
        // instances are released — a running async task on a destroyed query
        // corrupts memory even if the navmesh itself is still alive.
        std::lock_guard<std::mutex> lock(this->pathFuturesMutex);
        for (auto& pair : this->pathFutures)
        {
            if (pair.second.valid())
            {
                pair.second.wait();
            }
        }
        this->pathFutures.clear();
    }

    void OgreRecastModule::buildNavigationMesh(void)
    {
        if (this->staticObstacles.empty() && this->dynamicObstacles.empty() && this->terraInputGeomCells.empty())
        {
            return;
        }

        if (false == this->mustRegenerate)
        {
            return;
        }

        // Guard re-entry — exchange() returns the OLD value
        if (true == this->buildInProgress.exchange(true))
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[OgreRecastModule] buildNavigationMesh skipped: build already in progress.");
            return;
        }

        // Notify UI: busy
        boost::shared_ptr<EventDataNavMeshBusy> busyEvt(new EventDataNavMeshBusy(true));
        AppStateManager::getSingletonPtr()->getEventManager(this->appStateName)->queueEvent(busyEvt);

        // Clean up any previous (finished) thread
        if (this->navMeshThread.joinable())
        {
            this->navMeshThread.join();
        }

        this->navMeshThread = std::thread([this]()
            {
                // -----------------------------------------------------------------------
                // PHASE 1 — render thread (~50ms):
                //   • Initialise detourTileCache if needed
                //   • Collect entity/item pointers, call getMeshInformation (reads CPU
                //     vertex buffers), apply world transforms -> NavMeshGeomSnapshot
                //   • The render thread is only blocked for this fast snapshot step.
                // -----------------------------------------------------------------------
                InputGeom::NavMeshGeomSnapshot snapshot;
                bool hasTerra = false;

                NOWA::GraphicsModule::RenderCommand setupCmd = [&]()
                {
                    if (nullptr == this->detourTileCache)
                    {
                        this->detourTileCache = new OgreDetourTileCache(this->ogreRecast);
                    }

                    std::vector<Ogre::v1::Entity*> entities;
                    std::vector<Ogre::Item*> items;

                    for (size_t i = 0; i < this->staticObstacles.size(); i++)
                    {
                        auto go = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(this->staticObstacles[i]);
                        if (nullptr == go)
                        {
                            continue;
                        }

                        auto* item = go->getMovableObject<Ogre::Item>();
                        if (nullptr != item)
                        {
                            items.push_back(item);
                        }
                    }

                    // Keeping here v1 entity, because OgreRecast should be able to handle both, because its a generic lib
                    Ogre::SceneNode* rootNode = this->ogreRecast->getSceneManager()->getRootSceneNode();

                    GameObjectPtr gameObjectPtr = nullptr;

                    if (false == this->staticObstacles.empty())
                    {
                        gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(this->staticObstacles[0]);
                    }

                    if (nullptr != gameObjectPtr)
                    {
                        if (false == entities.empty())
                        {
                            rootNode = entities[0]->getParentSceneNode()->getCreator()->getRootSceneNode();
                        }
                        else if (false == items.empty())
                        {
                            rootNode = items[0]->getParentSceneNode()->getCreator()->getRootSceneNode();
                        }
                    }

                    // Build snapshot — reads CPU vertex buffers + world transforms.
                    // Returns plain float/int arrays: zero Ogre access after this point.
                    snapshot = InputGeom::buildSnapshot(entities, items, rootNode);

                    if (false == this->terraInputGeomCells.empty())
                    {
                        hasTerra = true;
                        for (auto it = this->terraInputGeomCells.begin(); it != this->terraInputGeomCells.end(); ++it)
                        {
                            auto go = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(it->first);
                            if (nullptr == go)
                            {
                                continue;
                            }
                            auto comp = NOWA::makeStrongPtr(go->getComponent<NavMeshTerraComponent>());
                            if (nullptr == comp)
                            {
                                continue;
                            }
                            Ogre::Terra* terra = go->getMovableObject<Ogre::Terra>();
                            if (nullptr == terra)
                            {
                                continue;
                            }

                            InputGeom::TerraData td;
                            td.terra = terra;
                            td.terraLayerList = comp->getTerraLayerList();
                            snapshot.terraDataList.push_back(td);
                        }
                    }
                };

                // Render thread is busy only for this call (~50ms), then completely free.
                NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(setupCmd), "OgreRecastModule::buildNavigationMesh::snapshot");

                // -----------------------------------------------------------------------
                // PHASE 2 — background thread (heavy, ~9s):
                //   • InputGeom is built from plain float/int arrays — zero Ogre calls.
                //   • buildChunkyTriMesh() runs here.
                //   • TileCacheBuild(InputGeom*) runs here.
                //   • The render thread is 100% free during this entire phase.
                // -----------------------------------------------------------------------
                bool success = false;

                if (false == hasTerra)
                {
                    // Pure-snapshot path: all heavy work on background thread
                    InputGeom* inputGeom = new InputGeom(snapshot);
                    success = this->detourTileCache->TileCacheBuild(inputGeom);
                    // TileCacheBuild takes ownership and deletes inputGeom
                }
                else
                {
                    // Terra path: existing constructor reads CPU heightmap — still safe
                    // off render thread as long as terrain isn't edited concurrently.
                    std::vector<Ogre::v1::Entity*> noEntities;
                    std::vector<Ogre::Item*> noItems;
                    success = this->detourTileCache->TileCacheBuild(snapshot.terraDataList, noEntities, noItems);
                    // Note: if you also have entities+items alongside terra, reconstruct
                    // them from snapshot.meshEntries here, or extend TileCacheBuild to
                    // accept a mixed snapshot — see section 5 below.
                }

                // -----------------------------------------------------------------------
                // PHASE 3 — render thread (~50ms): commit results, fire events
                // -----------------------------------------------------------------------
                NOWA::GraphicsModule::RenderCommand finishCmd = [this, success, snapshot]()
                {
                    this->hasValidNavMesh = success;

                    if (true == success)
                    {
                        // Saves both .nav (tile cache) and .navgeom (mesh geometry for tile rebuilds)
                        if (false == this->saveNavigationMesh(snapshot))
                        {
                            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[OgreRecastModule] Navmesh built but files could not be saved.");
                        }
                    }

                    this->createInputGeom();

                    if (false == this->hasValidNavMesh)
                    {
                        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[OgreRecastModule] Error: could not generate useable navmesh.");
                    }

                    // recreateDrawer destroys the old ManualObject geometry first,
                    // then drawNavMesh rebuilds it fresh — order matters to prevent accumulation.
                    this->ogreRecast->recreateDrawer();

                    if (true == this->debugDraw && nullptr != this->detourTileCache)
                    {
                        this->detourTileCache->drawNavMesh(true);
                    }

                    boost::shared_ptr<EventDataFeedback> evt(new EventDataFeedback(this->hasValidNavMesh, "#{NavigationMeshCreationFail}"));
                    AppStateManager::getSingletonPtr()->getEventManager(this->appStateName)->queueEvent(evt);

                    auto crowdGOs = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsFromComponent(CrowdComponent::getStaticClassName());
                    if (!crowdGOs.empty())
                    {
                        this->detourCrowd->setMaxAgents(crowdGOs.size());
                    }

                    this->mustRegenerate = false;
                    AppStateManager::getSingletonPtr()->getEventManager()->abortEvent(NOWA::EventDataGeometryModified::getStaticEventType(), true);

                    boost::shared_ptr<EventDataNavMeshBusy> doneEvt(new EventDataNavMeshBusy(false));
                    AppStateManager::getSingletonPtr()->getEventManager(this->appStateName)->queueEvent(doneEvt);
                };

                NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(finishCmd), "OgreRecastModule::buildNavigationMesh::finish");

                this->buildInProgress.store(false);
                // Do NOT detach - see destroyContent() below
            });
        // navMeshThread is kept joinable; destructor will join it.
    }

	void OgreRecastModule::debugDrawNavMesh(bool draw)
    {
        if (this->debugDraw == draw)
        {
            return;
        }

        this->debugDraw = draw;

        if (true == this->buildInProgress.load())
        {
            return;
        }

        if (true == draw)
        {
            // Already valid in memory — just draw, nothing else needed
            if (true == this->hasValidNavMesh)
            {
                NOWA::GraphicsModule::RenderCommand renderCommand = [detourTileCache = this->detourTileCache, ogreRecast = this->ogreRecast]()
                {
                    if (nullptr != detourTileCache && nullptr != ogreRecast)
                    {
                        detourTileCache->drawNavMesh(true);
                    }
                };
                NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "OgreRecastModule::debugDrawNavMesh::draw");
                return;
            }

            // Not in memory — try loading from disk first regardless of mustRegenerate.
            // Only rebuild if: file doesn't exist OR geometry actually changed (mustRegenerate
            // AND file is outdated — but we cannot know that, so trust the file if it exists).
            bool loaded = false;
            NOWA::GraphicsModule::RenderCommand loadCmd = [this, &loaded]()
            {
                loaded = this->loadNavigationMesh();
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(loadCmd), "OgreRecastModule::debugDrawNavMesh::load");

            if (true == loaded)
            {
                // Process first the obstacles (Box-Obstacles via mloadedFromDisk)
                this->detourTileCache->handleUpdate(0.016f);

                NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
                {
                    if (nullptr != this->detourTileCache)
                    {
                        this->ogreRecast->recreateDrawer();
                        this->detourTileCache->drawNavMesh(true);
                    }
                };
                NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "OgreRecastModule::debugDrawNavMesh::draw");
                return;
            }

            // No file on disk — must build from scratch.
            // Phase 3 finishCmd will draw when done (reads this->debugDraw).
            this->buildNavigationMesh();
        }
        else
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
            {
                if (nullptr != this->detourTileCache)
                {
                    this->detourTileCache->drawNavMesh(false);
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "OgreRecastModule::debugDrawNavMesh::hide");
        }
    }

	void OgreRecastModule::update(Ogre::Real dt)
    {
        if (true == this->buildInProgress.load())
        {
            return;
        }

        if (nullptr != this->detourTileCache)
        {
            this->detourTileCache->handleUpdate(dt);

            if (true == this->debugDraw && this->detourTileCache->getNeedsRedraw())
            {
                // Clear immediately on logic thread — prevents re-triggering next frame.
                this->detourTileCache->clearNeedsRedraw();

                // fireAndForget=true: closure executes ONCE on render thread,
                // is NOT registered as persistent, does NOT repeat every frame.
                auto renderClosure = [this](Ogre::Real)
                {
                    if (nullptr != this->detourTileCache && nullptr != this->ogreRecast)
                    {
                        this->detourTileCache->drawNavMesh(true);
                    }
                };
                NOWA::GraphicsModule::getInstance()->updateTrackedClosure("OgreRecastModule::drawNavMesh", renderClosure, true);
            }
        }
    }

    bool OgreRecastModule::findPath(const Ogre::Vector3& startPosition, const Ogre::Vector3& endPosition, int pathSlot, int targetSlot, bool /*drawPath*/) // ← param kept for API compat, ignored
    {
        if (false == this->hasValidNavMesh || true == this->buildInProgress.load())
        {
            return false;
        }

        if (nullptr != this->detourTileCache && this->detourTileCache->hasPendingObstacles())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[OgreRecastModule] findPath deferred: obstacle tiles not yet rebuilt (slot=" + Ogre::StringConverter::toString(pathSlot) + ")");
            return false;
        }

        float start[3], end[3];
        OgreRecast::OgreVect3ToFloatA(startPosition, start);
        OgreRecast::OgreVect3ToFloatA(endPosition, end);

        dtNavMeshQuery* query = this->ogreRecast->getNavQueryForSlot(pathSlot);
        if (nullptr == query)
        {
            // No per-slot query — synchronous fallback on logic thread
            int ret = this->ogreRecast->FindPath(start, end, pathSlot, targetSlot);
            return ret >= 0;
        }

        // Guard: reject if the previous async task for this slot is still running.
        // dtNavMeshQuery is NOT thread-safe — two concurrent uses of the same query
        // instance would corrupt navigation results.
        {
            std::lock_guard<std::mutex> lock(this->pathFuturesMutex);
            auto it = this->pathFutures.find(pathSlot);
            if (it != this->pathFutures.end())
            {
                if (it->second.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
                {
                    return false; // still running — caller retries next frame
                }

                it->second.get();
                this->pathFutures.erase(it);
            }
        }

        float startCopy[3] = {start[0], start[1], start[2]};
        float endCopy[3] = {end[0], end[1], end[2]};
        auto ogreRecastPtr = this->ogreRecast;

        auto future = std::async(std::launch::async, [ogreRecastPtr, query, startCopy, endCopy, pathSlot, targetSlot]() mutable -> int
        {
            return ogreRecastPtr->FindPathWithQuery(query, startCopy, endCopy, pathSlot, targetSlot);
        });

        {
            std::lock_guard<std::mutex> lock(this->pathFuturesMutex);
            this->pathFutures[pathSlot] = std::move(future);
        }

        // drawPath is handled by the caller AFTER waitForPath() — see PathFollowState3D.
        // Drawing here would race with the async write to m_PathStore.

        return true;
    }

    bool OgreRecastModule::waitForPath(int pathSlot)
    {
        std::lock_guard<std::mutex> lock(this->pathFuturesMutex);
        auto it = this->pathFutures.find(pathSlot);
        if (it == this->pathFutures.end())
        {
            return false;
        }

        // Block until the task completes, then collect result
        int ret = it->second.get();
        this->pathFutures.erase(it);
        return ret >= 0;
    }

	void OgreRecastModule::removeDrawnPath(void)
	{
		if (nullptr != this->ogreRecast)
		{
            auto recastPtr = this->ogreRecast;
            NOWA::GraphicsModule::RenderCommand renderCommand = [recastPtr]()
            {
                recastPtr->RemoveRecastPathLine();
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "OgreRecastModule::drawLine");
		}
	}

	OgreRecast* OgreRecastModule::getOgreRecast(void) const
	{
		return this->ogreRecast;
	}

	OgreDetourTileCache* OgreRecastModule::getOgreDetourTileCache(void) const
	{
		return this->detourTileCache;
	}

	OgreDetourCrowd* OgreRecastModule::getOgreDetourCrowd(void) const
	{
		return this->detourCrowd;
	}

} // namespace end
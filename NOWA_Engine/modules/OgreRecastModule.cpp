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
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("OgreRecastModule::createOgreRecast", _3(sceneManager, params, pointExtends), {
                this->ogreRecast = new OgreRecast(sceneManager, params);
                this->ogreRecast->setPointExtents(pointExtends);
                if (nullptr == this->detourTileCache)
                {
                    this->detourTileCache = new OgreDetourTileCache(this->ogreRecast);
                    this->detourCrowd = new OgreDetourCrowd(this->ogreRecast);
                }
            });

            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[OgreRecastModule] Recast created");

            // Listen on the app-state manager (NavMeshComponent::connect fires here
            // when simulation starts — connect() is called from app-state context).
            AppStateManager::getSingletonPtr()->getEventManager(this->appStateName)->addListener(fastdelegate::MakeDelegate(this, &OgreRecastModule::handleGeometryModified), EventDataGeometryModified::getStaticEventType());
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
		this->destroyContent();
	}

	void OgreRecastModule::handleGeometryModified(NOWA::EventDataPtr eventData)
    {
        this->mustRegenerate = true;
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

				ENQUEUE_RENDER_COMMAND_MULTI_WAIT("OgreRecastModule::removeConvexObstacle", _2(detourTileCache, convexVolume),
				{
					detourTileCache->removeConvexShapeObstacle(convexVolume);
				});
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

			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("OgreRecastModule::destroyOgreRecast", _1(ogreRecast),
			{
				delete ogreRecast;
			});
		}

		// Enqueue deletion of detourTileCache object
		if (this->detourTileCache)
		{
			auto detourTileCache = this->detourTileCache;
			this->detourTileCache = nullptr;

			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("OgreRecastModule::destroyDetourTileCache", _1(detourTileCache),
			{
				delete detourTileCache;
			});
		}

		// Enqueue deletion of detourCrowd object
		if (this->detourCrowd)
		{
			auto detourCrowd = this->detourCrowd;
			this->detourCrowd = nullptr;

			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("OgreRecastModule::destroyDetourCrowd", _1(detourCrowd),
			{
				delete detourCrowd;
			});
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

        // Remove from static list — object cannot be in both
        this->removeStaticObstacle(id);

        InputGeom* inputGeom = nullptr;
        ConvexVolume* convexVolume = nullptr;

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, id, walkable, externalInputGeom, externalConvexVolume, &inputGeom, &convexVolume]()
        {
            auto it = this->dynamicObstacles.find(id);

            const bool isPlaceholder = (it != this->dynamicObstacles.end()) && (nullptr == it->second.first) && (nullptr == it->second.second);
            if (isPlaceholder)
            {
                this->dynamicObstacles.erase(it);
                it = this->dynamicObstacles.end();
            }

            if (it == this->dynamicObstacles.end())
            {
                auto gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(id);

                if (nullptr == gameObjectPtr)
                {
                    return;
                }

                // Use hasValidNavMesh — not getInputGeom().
                // After loadAll(), getInputGeom() is always null even though the tile cache
                // is fully valid and ready to accept convex shape obstacles.
                const bool tileInputGeomReady = this->hasValidNavMesh;

                if (tileInputGeomReady)
                {
                    if (nullptr != externalInputGeom)
                    {
                        inputGeom = externalInputGeom;
                    }
                    else
                    {
                        Ogre::Item* item = gameObjectPtr->getMovableObject<Ogre::Item>();
                        if (nullptr != item)
                        {
                            inputGeom = new InputGeom(item);
                        }
                    }

                    if (nullptr != inputGeom)
                    {
                        convexVolume = (nullptr != externalConvexVolume) ? externalConvexVolume : inputGeom->getConvexHull(this->ogreRecast->getAgentRadius());

                        convexVolume->area = walkable ? RC_WALKABLE_AREA : RC_NULL_AREA;
                        this->detourTileCache->addConvexShapeObstacle(convexVolume);
                    }
                }

                this->dynamicObstacles.emplace(id, std::make_pair(inputGeom, convexVolume));
            }
            else
            {
                if (nullptr != it->second.second)
                {
                    it->second.second->area = walkable ? RC_WALKABLE_AREA : RC_NULL_AREA;
                }
                else
                {
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[OgreRecastModule] addDynamicObstacle: null convexVolume for id " + Ogre::StringConverter::toString(id));
                }
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "OgreRecastModule::addDynamicObstacle");
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

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, id, destroy, &inputGeom, &convexVolume]()
        {
            auto it = this->dynamicObstacles.find(id);
            if (it == this->dynamicObstacles.end())
            {
                return; // Not registered — nothing to do
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
        };

        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "OgreRecastModule::removeDynamicObstacle");

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

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, id, activate]()
        {
            auto it = this->dynamicObstacles.find(id);
            if (it == this->dynamicObstacles.end())
            {
                return;
            }

            ConvexVolume* convexVolume = it->second.second;

            // Skip placeholders — no geometry exists yet, nothing to activate
            if (nullptr == convexVolume)
            {
                return;
            }

            if (false == activate)
            {
                // Remove from tile cache — tiles are rebuilt, area becomes walkable
                this->detourTileCache->removeConvexShapeObstacle(convexVolume);
            }
            else
            {
                // Only re-add if not already present in the tile cache
                int obstacleId = this->detourTileCache->getConvexShapeObstacleId(convexVolume);
                if (-1 == obstacleId)
                {
                    this->detourTileCache->addConvexShapeObstacle(convexVolume);
                }
            }
        };

        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "OgreRecastModule::activateDynamicObstacle");
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
        InputGeom* inputGeom = nullptr;

        ENQUEUE_RENDER_COMMAND_MULTI_WAIT("OgreRecastModule::createInputGeom", _1(&inputGeom), {
            for (auto it = this->dynamicObstacles.begin(); it != this->dynamicObstacles.end(); ++it)
            {
                if (nullptr == it->second.first)
                {
                    // Only create new one, if there is no external existing one
                    auto gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(it->first);
                    if (nullptr != gameObjectPtr)
                    {
                        Ogre::Item* item = gameObjectPtr->getMovableObject<Ogre::Item>();
                        if (nullptr != item)
                        {
                            inputGeom = new InputGeom(item);
                        }

                        if (nullptr == inputGeom)
                        {
                            continue;
                        }

                        it->second.first = inputGeom;
                        ConvexVolume* convexVolume = inputGeom->getConvexHull(this->ogreRecast->getAgentRadius());
                        it->second.second = convexVolume;

                        convexVolume->area = RC_NULL_AREA;

                        auto navMeshCompPtr = NOWA::makeStrongPtr(gameObjectPtr->getComponent<NavMeshComponent>());
                        if (nullptr != navMeshCompPtr)
                        {
                            if (true == navMeshCompPtr->getWalkable())
                            {
                                convexVolume->area = RC_WALKABLE_AREA; // Set area described by convex polygon to "unwalkable"
                            }
                        }
                        convexVolume->hmin = convexVolume->hmin - 0.1f; // Extend a bit downwards so it hits the ground (navmesh) for certain. (Maybe this is not necessary)
                        this->detourTileCache->addConvexShapeObstacle(convexVolume);
                        // this->detourTileCache->addTempObstacle(gameObjectPtr->getPosition());
                    }
                }
            }
        });
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
                    Ogre::SceneNode* rootNode = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(this->staticObstacles.empty() ? 0 : this->staticObstacles[0])
                                                    ? // fallback: grab from first valid entity/item
                                                    (!entities.empty() ? entities[0]->getParentSceneNode()->getCreator()->getRootSceneNode()
                                                                       : (!items.empty() ? items[0]->getParentSceneNode()->getCreator()->getRootSceneNode() : this->ogreRecast->m_pSceneMgr->getRootSceneNode()))
                                                    : this->ogreRecast->m_pSceneMgr->getRootSceneNode();

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
                // Loaded from disk — draw immediately, no rebuild needed
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
        // Bug fix 3: handleUpdate ticks the tile cache obstacle system (processes
        // deferred add/remove of convex obstacles, rebuilds affected tiles).
        // If a full rebuild is running on the background thread, the tile cache
        // pointer and its internal state are being rewritten — calling handleUpdate
        // concurrently is a data race and will corrupt or crash the rebuild.
        if (true == this->buildInProgress.load())
        {
            return;
        }

        if (nullptr != this->detourTileCache)
        {
            this->detourTileCache->handleUpdate(dt);
        }
    }

	bool OgreRecastModule::findPath(const Ogre::Vector3& startPosition, const Ogre::Vector3& endPosition, int pathSlot, int targetSlot, bool drawPath)
    {
        // Bug fix 1: Guard both conditions — no valid mesh at all, OR a rebuild
        // is currently running. The old code only checked hasValidNavMesh, which
        // is true from the PREVIOUS build while a new one rewrites internal Detour
        // structures concurrently — a guaranteed crash / corrupt path result.
        if (false == this->hasValidNavMesh || true == this->buildInProgress.load())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[OgreRecastModule] findPath skipped: nav mesh not ready or build in progress.");
            return false;
        }

        auto ogreRecastPtr = this->ogreRecast;

        int ret = NOWA::GraphicsModule::getInstance()->enqueueAndWaitWithResult<int>([=]() -> int
        {
            return ogreRecastPtr->FindPath(startPosition, endPosition, pathSlot, targetSlot);
        }, "OgreRecastModule::findPath");

        if (ret >= 0)
        {
            ENQUEUE_RENDER_COMMAND_MULTI_NO_THIS("OgreRecastModule::drawPathLine", _3(ogreRecastPtr, pathSlot, drawPath),
				{
					ogreRecastPtr->CreateRecastPathLine(pathSlot, drawPath);
				});
            return true;
        }
        else
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[OgreRecastModule] Warning: could not find a (full) path (" + this->ogreRecast->getPathFindErrorMsg(ret) + "). It is possible there is a partial path.");
            return false;
        }
    }


	void OgreRecastModule::removeDrawnPath(void)
	{
		if (nullptr != this->ogreRecast)
		{
			ENQUEUE_RENDER_COMMAND("OgreRecastModule::removeDrawnPath",
			{
				this->ogreRecast->RemoveRecastPathLine();
			});
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
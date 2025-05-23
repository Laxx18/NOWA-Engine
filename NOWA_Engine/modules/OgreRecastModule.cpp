#include "NOWAPrecompiled.h"
#include "OgreRecastModule.h"
#include "gameobject/GameObject.h"
#include "gameobject/NavMeshComponent.h"
#include "gameobject/NavMeshTerraComponent.h"
#include "gameobject/CrowdComponent.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	OgreRecastModule::OgreRecastModule(const Ogre::String& appStateName)
		: appStateName(appStateName),
		ogreRecast(nullptr),
		detourTileCache(nullptr),
		detourCrowd(nullptr),
		mustRegenerate(true),
		hasValidNavMesh(false),
		debugDraw(false)
	{
		
	}

	OgreRecast* OgreRecastModule::createOgreRecast(Ogre::SceneManager* sceneManager, OgreRecastConfigParams params, const Ogre::Vector3& pointExtends)
	{
		if (nullptr == this->ogreRecast)
		{
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("OgreRecastModule::createOgreRecast", _3(sceneManager, params, pointExtends),
			{
				// this->destroyContent();
				// params.setKeepInterResults(false);
				this->ogreRecast = new OgreRecast(sceneManager, params);
				this->ogreRecast->setPointExtents(pointExtends);

				if (nullptr == this->detourTileCache)
				{
					this->detourTileCache = new OgreDetourTileCache(this->ogreRecast);
					this->detourCrowd = new OgreDetourCrowd(this->ogreRecast);
				}
			});

			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[OgreRecastModule] Recast created");

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

	void OgreRecastModule::destroyContent(void)
	{
		ENQUEUE_RENDER_COMMAND_WAIT("OgreRecastModule::destroyContent",
		{
			this->debugDrawNavMesh(false);

			auto & it = this->dynamicObstacles.begin();

			while (it != this->dynamicObstacles.end())
			{
				auto dataPair = it->second;
				InputGeom* inputPair = dataPair.first;
				ConvexVolume* convexVolume = dataPair.second;
				this->detourTileCache->removeConvexShapeObstacle(convexVolume);

				delete inputPair;
				inputPair = nullptr;

				delete convexVolume;
				convexVolume = nullptr;

				++it;
			}

			auto& it2 = this->terraInputGeomCells.begin();

			while (it2 != this->terraInputGeomCells.end())
			{
				InputGeom* inputGeom = it2->second;

				delete inputGeom;
				inputGeom = nullptr;

				++it2;
			}

			if (nullptr != this->ogreRecast)
			{
				delete this->ogreRecast;
				this->ogreRecast = nullptr;
			}
			if (nullptr != this->detourTileCache)
			{
				delete this->detourTileCache;
				this->detourTileCache = nullptr;
			}
			if (nullptr != this->detourCrowd)
			{
				delete this->detourCrowd;
				this->detourCrowd = 0;
			}
		});
		this->hasValidNavMesh = false;
		this->mustRegenerate = true;
		this->staticObstacles.clear();
		this->dynamicObstacles.clear();
		this->terraInputGeomCells.clear();
	}

	bool OgreRecastModule::hasNavigationMeshElements(void) const
	{
		return this->staticObstacles.size() > 0 || this->dynamicObstacles.size() > 0 || this->terraInputGeomCells.size() > 0;
	}

	void OgreRecastModule::addStaticObstacle(unsigned long id)
	{
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

	void OgreRecastModule::addDynamicObstacle(unsigned long id, bool walkable, InputGeom* externalInputGeom, ConvexVolume* externalConvexVolume)
	{
		if (nullptr == this->detourTileCache)
		{
			return;
		}

		InputGeom* inputGeom = nullptr;
		ConvexVolume* convexVolume = nullptr;

		this->removeStaticObstacle(id);

		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("OgreRecastModule::addDynamicObstacle", _6(id, walkable, externalInputGeom, externalConvexVolume, &inputGeom, &convexVolume),
		{
			auto & it = this->dynamicObstacles.find(id);
			if (it == this->dynamicObstacles.end())
			{
				auto& gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(id);
				if (nullptr != gameObjectPtr)
				{
					Ogre::v1::Entity* entity = gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
					if (nullptr != entity)
					{
						// Only create if there is an input geom for the detour tile cache existing, if not just add nullptr, nullptr as place holders, and when nav mesh is created
						// detour tile cache input geom does exist, so add them later
						if (nullptr != this->detourTileCache->getInputGeom())
						{
							// Only create new one, if there is no external existing one
							if (nullptr == externalInputGeom)
							{
								inputGeom = new InputGeom(entity);
							}
							else
							{
								inputGeom = externalInputGeom;
							}
						}
					}
					else
					{
						Ogre::Item* item = gameObjectPtr->getMovableObject<Ogre::Item>();
						if (nullptr != item)
						{
							// Only create if there is an input geom for the detour tile cache existing, if not just add nullptr, nullptr as place holders, and when nav mesh is created
							// detour tile cache input geom does exist, so add them later
							if (nullptr != this->detourTileCache->getInputGeom())
							{
								// Only create new one, if there is no external existing one
								if (nullptr == externalInputGeom)
								{
									inputGeom = new InputGeom(item);
								}
								else
								{
									inputGeom = externalInputGeom;
								}
							}
						}
					}

					if (nullptr != inputGeom)
					{
						if (nullptr == externalConvexVolume)
						{
							convexVolume = inputGeom->getConvexHull(this->ogreRecast->getAgentRadius());
						}
						else
						{
							convexVolume = externalConvexVolume;
						}

						if (false == walkable)
						{
							convexVolume->area = RC_NULL_AREA;   // Set area described by convex polygon to "unwalkable"
						}
						else
						{
							convexVolume->area = RC_WALKABLE_AREA;
						}
						// convexVolume->hmin = convexVolume->hmin - 0.1f;    // Extend a bit downwards so it hits the ground (navmesh) for certain. (Maybe this is not necessary)
						this->detourTileCache->addConvexShapeObstacle(convexVolume);
						// this->detourTileCache->addTempObstacle(gameObjectPtr->getPosition());
					}

					// ConvexVolume* convexVolume = new ConvexVolume(InputGeom::getWorldSpaceBoundingBox(entity), this->ogreRecast->getAgentRadius());
					// Add in any case dynamic obstacle as placeholder
					this->dynamicObstacles.emplace(id, std::make_pair(inputGeom, convexVolume));
				}
			}
			else
			{
				if (false == walkable)
				{
					it->second.second->area = RC_NULL_AREA;   // Set area described by convex polygon to "unwalkable"
				}
				else
				{
					it->second.second->area = RC_WALKABLE_AREA;
				}
			}
		});
	}

	void OgreRecastModule::createInputGeom(void)
	{
		InputGeom* inputGeom = nullptr;

		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("OgreRecastModule::createInputGeom", _1(&inputGeom),
		{
			for (auto it = this->dynamicObstacles.begin(); it != this->dynamicObstacles.end(); ++it)
			{
				if (nullptr == it->second.first)
				{
					// Only create new one, if there is no external existing one
					auto& gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(it->first);
					if (nullptr != gameObjectPtr)
					{
						Ogre::v1::Entity* entity = gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
						if (nullptr != entity)
						{
							inputGeom = new InputGeom(entity);
						}
						else
						{
							Ogre::Item* item = gameObjectPtr->getMovableObject<Ogre::Item>();
							if (nullptr != item)
							{
								inputGeom = new InputGeom(item);
							}
						}

						if (nullptr == inputGeom)
						{
							continue;
						}

						it->second.first = inputGeom;
						ConvexVolume* convexVolume = inputGeom->getConvexHull(this->ogreRecast->getAgentRadius());
						it->second.second = convexVolume;

						convexVolume->area = RC_NULL_AREA;

						auto& navMeshCompPtr = NOWA::makeStrongPtr(gameObjectPtr->getComponent<NavMeshComponent>());
						if (nullptr != navMeshCompPtr)
						{
							if (true == navMeshCompPtr->getWalkable())
							{
								convexVolume->area = RC_WALKABLE_AREA;   // Set area described by convex polygon to "unwalkable"
							}
						}
						convexVolume->hmin = convexVolume->hmin - 0.1f;    // Extend a bit downwards so it hits the ground (navmesh) for certain. (Maybe this is not necessary)
						this->detourTileCache->addConvexShapeObstacle(convexVolume);
						// this->detourTileCache->addTempObstacle(gameObjectPtr->getPosition());
					}
				}
			}
		});
	}

	std::pair<InputGeom*, ConvexVolume*> OgreRecastModule::removeDynamicObstacle(unsigned long id, bool destroy)
	{
		ConvexVolume* convexVolume = nullptr;
		InputGeom* inputGeom = nullptr;
		
		if (nullptr == this->detourTileCache)
		{
			return std::make_pair(inputGeom, convexVolume);
		}

		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("OgreRecastModule::removeDynamicObstacle", _4(id, destroy, &inputGeom, &convexVolume),
		{
			auto & it = this->dynamicObstacles.find(id);
			if (it != this->dynamicObstacles.end())
			{
				auto& gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(id);
				//#if _DEBUG
				//			Ogre::String debugText = "Gameobject with id : " + Ogre::StringConverter::toString(id) + " does no more exist!";
				//			assert((nullptr != gameObjectPtr) && debugText.c_str());
				//#endif
				if (nullptr != gameObjectPtr)
				{
					Ogre::v1::Entity* entity = gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
					if (nullptr != entity)
					{
						convexVolume = it->second.second;
						inputGeom = it->second.first;
						this->detourTileCache->removeConvexShapeObstacle(convexVolume);

						this->dynamicObstacles.erase(id);

						if (true == destroy)
						{
							delete inputGeom;
							inputGeom = nullptr;

							delete convexVolume;
							convexVolume = nullptr;
						}
					}
				}
			}
		});
		return std::make_pair(inputGeom, convexVolume);
	}

	void OgreRecastModule::activateDynamicObstacle(unsigned long id, bool activate)
	{
		if (nullptr == this->detourTileCache || nullptr == this->detourTileCache->getInputGeom())
		{
			return;
		}

		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("OgreRecastModule::activateDynamicObstacle", _2(id, activate),
		{
			auto & it = this->dynamicObstacles.find(id);
			if (it != this->dynamicObstacles.end())
			{
				ConvexVolume* convexVolume = it->second.second;
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
		});
	}

	void OgreRecastModule::addTerra(unsigned long id)
	{
		auto& it = this->terraInputGeomCells.find(id);
		if (it == this->terraInputGeomCells.end())
		{
			auto& gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(id);
			if (nullptr != gameObjectPtr)
			{
				Ogre::Terra* terra = gameObjectPtr->getMovableObject<Ogre::Terra>();
				if (nullptr != terra)
				{
					this->terraInputGeomCells.emplace(id, nullptr);
					boost::shared_ptr<NOWA::EventDataGeometryModified> eventDataGeometryModified(new NOWA::EventDataGeometryModified());
					NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataGeometryModified);
				}
			}
		}
	}

	InputGeom* OgreRecastModule::removeTerra(unsigned long id, bool destroy)
	{
		InputGeom* inputGeom = nullptr;

		auto& it = this->terraInputGeomCells.find(id);
		if (it != this->terraInputGeomCells.end())
		{
			auto& gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(id);
//#if _DEBUG
//			Ogre::String debugText = "Gameobject with id : " + Ogre::StringConverter::toString(id) + " does no more exist!";
//			assert((nullptr != gameObjectPtr) && debugText.c_str());
//#endif
			if (nullptr != gameObjectPtr)
			{
				Ogre::Terra* terra = gameObjectPtr->getMovableObject<Ogre::Terra>();
				if (nullptr != terra)
				{
					inputGeom = it->second;
					this->terraInputGeomCells.erase(id);
					boost::shared_ptr<NOWA::EventDataGeometryModified> eventDataGeometryModified(new NOWA::EventDataGeometryModified());
					NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataGeometryModified);

					if (true == destroy)
					{
						delete inputGeom;
						inputGeom = nullptr;
					}
				}
			}
		}
		return inputGeom;
	}

	void OgreRecastModule::buildNavigationMesh(void)
	{
		if (true == this->staticObstacles.empty() && true == this->dynamicObstacles.empty() && true == this->terraInputGeomCells.empty())
		{
			return;
		}

		// Only recreate if flag is set (scene modified), because its an heavy process
		if (true == this->mustRegenerate)
		{
			ENQUEUE_RENDER_COMMAND_WAIT("OgreRecastModule::buildNavigationMesh",
			{
				if (nullptr == this->detourTileCache)
				{
					this->detourTileCache = new OgreDetourTileCache(this->ogreRecast);
				}

				std::vector<Ogre::v1::Entity*> entities;
				std::vector<Ogre::Item*> items;

				for (size_t i = 0; i < this->staticObstacles.size(); i++)
				{
					auto& gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(this->staticObstacles[i]);
					if (nullptr != gameObjectPtr)
					{
						Ogre::v1::Entity* entity = gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
						if (nullptr != entity)
						{
							entities.emplace_back(entity);
						}
						else
						{
							Ogre::Item* item = gameObjectPtr->getMovableObject<Ogre::Item>();
							if (nullptr != item)
							{
								items.push_back(item);
							}
						}
					}
				}

				if (true == this->terraInputGeomCells.empty())
				{
					this->hasValidNavMesh = this->detourTileCache->TileCacheBuild(entities, items);
					// this->hasValidNavMesh = this->ogreRecast->NavMeshBuild(entities);
				}
				else
				{
					std::vector<InputGeom::TerraData> terraDataList;
					for (auto it = this->terraInputGeomCells.begin(); it != this->terraInputGeomCells.end(); ++it)
					{
						auto& gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController(this->appStateName)->getGameObjectFromId(it->first);
						if (nullptr != gameObjectPtr)
						{
							auto& navMeshTerraCompPtr = NOWA::makeStrongPtr(gameObjectPtr->getComponent<NavMeshTerraComponent>());
							if (nullptr != navMeshTerraCompPtr)
							{
								Ogre::Terra* terra = gameObjectPtr->getMovableObject<Ogre::Terra>();
								if (nullptr != terra)
								{
									InputGeom::TerraData terraData;
									terraData.terra = terra;
									terraData.terraLayerList = navMeshTerraCompPtr->getTerraLayerList();
									terraDataList.push_back(terraData);
								}
							}
						}
					}

					this->hasValidNavMesh = this->detourTileCache->TileCacheBuild(terraDataList, entities, items);
				}

				// Create input geom after TileCacheBuild is finished
				this->createInputGeom();

				if (false == this->hasValidNavMesh)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[OgreRecastModule] Error: could not generate useable navmesh from mesh.");
					// throw Ogre::Exception(Ogre::Exception::ERR_INTERNAL_ERROR, "[OgreRecastModule] could not generate useable navmesh from mesh", "NOWA");
				}

				// Sent event with feedback
				boost::shared_ptr<EventDataFeedback> eventDataNavigationMeshFeedback(new EventDataFeedback(this->hasValidNavMesh, "#{NavigationMeshCreationFail}"));
				NOWA::AppStateManager::getSingletonPtr()->getEventManager(this->appStateName)->queueEvent(eventDataNavigationMeshFeedback);
				this->ogreRecast->recreateDrawer();

				auto gameObjectsWithCrowdComponent = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsFromComponent(CrowdComponent::getStaticClassName());
				if (gameObjectsWithCrowdComponent.size() > 0)
				{
					this->detourCrowd->setMaxAgents(gameObjectsWithCrowdComponent.size());
				}

				this->mustRegenerate = false;
			});
		}
	}

	void OgreRecastModule::debugDrawNavMesh(bool draw)
	{
		this->debugDraw = draw;

		if (true == draw)
		{
			this->buildNavigationMesh();
		}

		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("OgreRecastModule::debugDrawNavMesh", _1(draw),
		{
			if (true == this->hasValidNavMesh)
			{
				// this->ogreRecast->drawNavMesh(draw);
				this->detourTileCache->drawNavMesh(draw);
			}
		});
	}

	void OgreRecastModule::update(Ogre::Real dt)
	{
		if (nullptr != this->detourTileCache)
		{
			this->detourTileCache->handleUpdate(dt);
		}
	}

	bool OgreRecastModule::findPath(const Ogre::Vector3& startPosition, const Ogre::Vector3& endPosition, int pathSlot, int targetSlot, bool drawPath)
	{
		if (!this->hasValidNavMesh)
			return false;

		// Capture necessary objects
		auto ogreRecastPtr = this->ogreRecast;

		// Call FindPath on the render thread and get result safely
		int ret = RenderCommandQueueModule::getInstance()->enqueueAndWaitWithResult<int>([=]() -> int
		{
			return ogreRecastPtr->FindPath(startPosition, endPosition, pathSlot, targetSlot);
		}, "OgreRecastModule::findPath");

		// If success, optionally draw the path
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
			// Still safe to use ogreRecast here on the logic thread
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[OgreRecastModule] Warning: could not find a (full) path (" + this->ogreRecast->getPathFindErrorMsg(ret) + "). It is possible there is a partial path.");
			return false;
		}
	}


	void OgreRecastModule::removeDrawnPath(void)
	{
		if (nullptr != this->ogreRecast)
		{
			ENQUEUE_RENDER_COMMAND_WAIT("OgreRecastModule::removeDrawnPath",
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
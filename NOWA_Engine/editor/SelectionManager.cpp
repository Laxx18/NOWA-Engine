#include "NOWAPrecompiled.h"
#include "SelectionManager.h"

#include "gameobject/GameObjectController.h"
#include "utilities/MathHelper.h"
#include "modules/InputDeviceModule.h"
#include "main/Core.h"
#include "main/Events.h"
#include "main/AppStateManager.h"

namespace NOWA
{

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class GameObjectsSelectUndoCommand : public ICommand
	{
	public:

		GameObjectsSelectUndoCommand(SelectionManager* selectionManager, std::vector<unsigned long> oldSelectedGameObjectIds)
			: selectionManager(selectionManager),
			oldSelectedGameObjectIds(oldSelectedGameObjectIds)
		{

		}

		~GameObjectsSelectUndoCommand()
		{

		}

		virtual void undo(void) override
		{
			// Store the new selection
			this->newSelectedGameObjectIds.resize(this->selectionManager->getSelectedGameObjects().size());
			unsigned int i = 0;
			for (auto& it = this->selectionManager->getSelectedGameObjects().cbegin(); it != this->selectionManager->getSelectedGameObjects().cend(); ++it)
			{
				this->newSelectedGameObjectIds[i++] = it->second.gameObject->getId();
			}

			this->selectionManager->clearSelection();
			for (size_t i = 0; i < this->oldSelectedGameObjectIds.size(); i++)
			{
				GameObjectPtr gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->oldSelectedGameObjectIds[i]);
				if (nullptr != gameObjectPtr)
				{
					this->selectionManager->select(gameObjectPtr->getId());
				}
			}
		}

		virtual void redo(void) override
		{
			this->selectionManager->clearSelection();
			for (size_t i = 0; i < this->newSelectedGameObjectIds.size(); i++)
			{
				GameObjectPtr gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->newSelectedGameObjectIds[i]);
				if (nullptr != gameObjectPtr)
				{
					this->selectionManager->select(gameObjectPtr->getId());
				}
			}
		}
	private:
		SelectionManager* selectionManager;
		std::vector<unsigned long> oldSelectedGameObjectIds;
		std::vector<unsigned long> newSelectedGameObjectIds;
	};

	SelectionManager::SelectionManager()
		: sceneManager(nullptr),
		camera(nullptr),
		mouseButtonId(OIS::MB_Left),
		selectionRect(nullptr),
		selectionNode(nullptr),
		volumeQuery(nullptr),
		selectQuery(nullptr),
		isSelectingRectangle(false),
		isSelecting(false),
		mouseIdPressed(false),
		selectBegin(Ogre::Vector2::ZERO),
		selectEnd(Ogre::Vector2::ZERO),
		categoryId(0),
		selectionObserver(nullptr),
		shiftDown(false)
	{

	}
	
	SelectionManager::~SelectionManager()
	{
		AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &SelectionManager::handleDeleteGameObject), EventDataDeleteGameObject::getStaticEventType());
		this->selectedGameObjects.clear();
		if (nullptr != this->selectionObserver)
		{
			delete this->selectionObserver;
			this->selectionObserver = nullptr;
		}

		// Step 1: Copy pointers
		auto selectionRectCopy = this->selectionRect;
		auto selectionNodeCopy = this->selectionNode;
		auto sceneManagerCopy = this->sceneManager;
		auto selectQueryCopy = this->selectQuery;
		auto volumeQueryCopy = this->volumeQuery;

		// Step 2: Nullify members to prevent double free or accidental access
		this->selectionRect = nullptr;
		this->selectQuery = nullptr;
		this->volumeQuery = nullptr;
		this->selectionNode = nullptr;

		// Step 3: Enqueue destruction command with copies, NO this capture
		NOWA::GraphicsModule::RenderCommand renderCommand = [selectionRectCopy, selectionNodeCopy, sceneManagerCopy, selectQueryCopy, volumeQueryCopy]()
			{
				if (selectionRectCopy && selectionNodeCopy)
				{
					selectionNodeCopy->detachObject(selectionRectCopy);
					delete selectionRectCopy;
				}

				if (sceneManagerCopy)
				{
					if (selectQueryCopy)
					{
						sceneManagerCopy->destroyQuery(selectQueryCopy);
					}

					if (volumeQueryCopy)
					{
						sceneManagerCopy->destroyQuery(volumeQueryCopy);
					}
				}
			};
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "SelectionManager::~SelectionManager");
	}

	void SelectionManager::init(Ogre::SceneManager* sceneManager, Ogre::Camera* camera, const Ogre::String& categories, OIS::MouseButtonID mouseButtonId, 
		SelectionManager::ISelectionObserver* selectionObserver, const Ogre::String& materialName)
	{
		this->selectionObserver = selectionObserver;
		// Must be existing!
		if (nullptr == this->selectionObserver)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[SelectionManager] Cannot init selection manager, because there is no selection observer implemented. Implement the ISelectionInterface first.");
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE,
				"[SelectionManager] Cannot init selection manager, because there is no selection observer implemented. Implement the ISelectionInterface first.", "NOWA");
		}
		
		this->sceneManager = sceneManager;
		this->camera = camera;
		this->categories = categories;
		this->mouseButtonId = mouseButtonId;
		this->materialName = materialName;
	
		this->categoryId = AppStateManager::getSingletonPtr()->getGameObjectController()->generateCategoryId(this->categories);

		NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
		{
			this->selectionRect = new SelectionRectangle("SelectionRectangle", this->sceneManager, this->camera);
			this->selectionNode = this->sceneManager->getRootSceneNode()->createChildSceneNode();
			this->selectionNode->attachObject(this->selectionRect);
			this->volumeQuery = this->sceneManager->createPlaneBoundedVolumeQuery(Ogre::PlaneBoundedVolumeList(), this->categoryId);
			this->selectQuery = this->sceneManager->createRayQuery(Ogre::Ray(), this->categoryId);
			this->selectQuery->setSortByDistance(true);
		};
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "SelectionManager::init");

		AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &SelectionManager::handleDeleteGameObject), EventDataDeleteGameObject::getStaticEventType());
	}

	void SelectionManager::queueSelectionEvent(unsigned long id, bool selected)
	{
		boost::shared_ptr<NOWA::EventDataGameObjectSelected> evt(new NOWA::EventDataGameObjectSelected(id, selected));
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evt);
	}

	void SelectionManager::handleDeleteGameObject(EventDataPtr eventData)
	{
		boost::shared_ptr<EventDataDeleteGameObject> castEventData = boost::static_pointer_cast<NOWA::EventDataDeleteGameObject>(eventData);

		const unsigned long id = castEventData->getGameObjectId();

		auto it = this->selectedGameObjects.find(id);
		if (it != this->selectedGameObjects.end())
		{
			// If object is being deleted, do not touch the GameObject pointer here (could already be invalid).
			// Just notify selection state change and remove from selection map.
			this->queueSelectionEvent(id, false);
			this->selectedGameObjects.erase(it);
		}
	}

	void SelectionManager::applySelectInternal(GameObject* gameObject, bool bSelect)
	{
		if (nullptr == gameObject)
		{
			return;
		}

		auto it = this->selectedGameObjects.find(gameObject->getId());

		if (true == bSelect)
		{
			// Add to map if not present
			if (it == this->selectedGameObjects.end())
			{
				SelectionData selectionData;
				selectionData.gameObject = gameObject;
				selectionData.initiallyDynamic = gameObject->isDynamic();
				this->selectedGameObjects.emplace(gameObject->getId(), selectionData);
			}

			this->selectionObserver->onHandleSelection(gameObject, true);
			gameObject->selected = true;
			this->queueSelectionEvent(gameObject->getId(), true);

			// A static game object cannot be moved for performance reasons, so set it dynamic for a short time
			if (false == gameObject->isDynamic())
			{
				gameObject->setDynamic(true);
			}

			this->isSelecting = true;
		}
		else
		{
			// Remove from map if present
			if (it != this->selectedGameObjects.end())
			{
				this->selectionObserver->onHandleSelection(it->second.gameObject, false);
				it->second.gameObject->selected = false;
				this->queueSelectionEvent(it->second.gameObject->getId(), false);

				// Restore original dynamic state
				it->second.gameObject->setDynamic(it->second.initiallyDynamic);

				this->selectedGameObjects.erase(it);
			}

			this->isSelecting = false;
		}
	}

	void SelectionManager::handleKeyPress(const OIS::KeyEvent& keyEventRef)
	{
		if (NOWA_K_SELECT == keyEventRef.key)
		{
			this->shiftDown = true;
		}
	}

	void SelectionManager::handleKeyRelease(const OIS::KeyEvent& keyEventRef)
	{
		this->shiftDown = false;
	}

	void SelectionManager::handleMouseMove(const OIS::MouseEvent& evt)
	{
		if (true == this->mouseIdPressed)
		{
			this->isSelectingRectangle = true;
			// Start building selection rectangle
			MathHelper::getInstance()->mouseToViewPort(evt.state.X.abs, evt.state.Y.abs, this->selectEnd.x, this->selectEnd.y, Core::getSingletonPtr()->getOgreRenderWindow());
			this->selectionRect->setCorners(this->selectBegin.x, this->selectBegin.y, this->selectEnd.x, this->selectEnd.y/*, this->materialName*/);
			this->selectionRect->setVisible(true);
		}

		this->isSelecting = false;
	}

	void SelectionManager::handleMousePress(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
	{
		this->isSelecting = false;

		// Ugly workaround, because shit OIS is not possible to handle mouse release and key release at once!
		// That means, if user holds shift and selects game objects and at the same moment release the shift key at the same time as the mouse release,
		// only the mouse release is detected and shift variable remains switched on!
		this->shiftDown = InputDeviceCore::getSingletonPtr()->isSelectDown();

		if (id != this->mouseButtonId)
		{
			return;
		}

		if (false == this->isSelectingRectangle)
		{
			// Snapshot for undo redo selection
			if (false == Core::getSingletonPtr()->getIsGame())
			{
				this->snapshotGameObjectSelection();
			}

			// Clear all selected objects, if shift is not hold
			if (false == this->shiftDown)
			{
				this->clearSelection();
			}

			const Ogre::Real absX = evt.state.X.abs;
			const Ogre::Real absY = evt.state.Y.abs;

			// Do only raycast on render thread, no selection modifications there
			unsigned long pickedId = 0;

			NOWA::GraphicsModule::RenderCommand renderCommand = [this, absX, absY, &pickedId]()
				{
					// true at the end: raycast from point does not work correctly so far
					GameObject* selectedGameObject = AppStateManager::getSingletonPtr()->getGameObjectController()->selectGameObject(absX, absY, this->camera, this->selectQuery, true);

					if (nullptr != selectedGameObject)
					{
						pickedId = selectedGameObject->getId();
					}
				};
			NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "SelectionManager::handleMousePress raycast");

			// ---- MAIN THREAD applies selection changes ----
			if (0 != pickedId)
			{
				GameObjectPtr gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(pickedId);
				if (nullptr != gameObjectPtr)
				{
					auto it = this->selectedGameObjects.find(pickedId);

					// Toggle behaviour: if already selected -> unselect, else select
					if (it != this->selectedGameObjects.end())
					{
						this->applySelectInternal(gameObjectPtr.get(), false);
					}
					else
					{
						this->applySelectInternal(gameObjectPtr.get(), true);
					}
				}
			}
			else
			{
				// Nothing found => unselect everything (only if shift is not down)
				// If shift is down, user usually expects "add/remove" behaviour, not wipe selection on empty space click.
				if (false == this->shiftDown)
				{
					this->clearSelection();
				}
			}
		}

		MathHelper::getInstance()->mouseToViewPort(evt.state.X.abs, evt.state.Y.abs, this->selectBegin.x, this->selectBegin.y, Core::getSingletonPtr()->getOgreRenderWindow());
		this->mouseIdPressed = true;
	}

	void SelectionManager::handleMouseRelease(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
	{
		this->isSelecting = false;
		
		if (id == this->mouseButtonId)
		{
			this->mouseIdPressed = false;
		}

		if (id == this->mouseButtonId && true == this->isSelectingRectangle)
		{
			//// If no shift has been hold, un select all objects
			//if (false == this->shiftDown)
			//{
			//	for (auto& entry : this->selectedGameObjects)
			//	{
			//		this->selectionObserver->onHandleSelection(entry.second, false);
			// 
			//		boost::shared_ptr<NOWA::EventDataGameObjectSelected> eventDataGameObjectSelected(new NOWA::EventDataGameObjectSelected(entry.second.gameObject->getId(), false));
			//		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataGameObjectSelected);
			//		entry.second->selected = false;
			//		this->isSelecting = false;
			//	}
			//	selectedGameObjects.clear();
			//}

			// Select all objects that are within the selection rectangle area
			// Select all objects that are within the selection rectangle area
			this->selectGameObjects(this->selectBegin, this->selectEnd);

			this->isSelectingRectangle = false;
			// Hide the rectangle
			this->selectionRect->setVisible(false);
		}
	}

	unsigned int SelectionManager::filterCategories(Ogre::String& categories)
	{
		this->categories = categories;
		this->categoryId = AppStateManager::getSingletonPtr()->getGameObjectController()->generateCategoryId(categories);
		NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
		{
			this->volumeQuery->setQueryMask(this->categoryId);
			this->selectQuery->setQueryMask(this->categoryId);
		};
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "SelectionManager::filterCategories");
		return this->categoryId;
	}

	void SelectionManager::changeMaterialName(const Ogre::String& materialName)
	{
		this->materialName = materialName;
	}

	const Ogre::String& SelectionManager::getMaterialName(void)
	{
		return this->materialName;
	}

	const Ogre::String& SelectionManager::getCategories(void)
	{
		return this->categories;
	}

	bool SelectionManager::getIsSelectingRectangle(void) const
	{
		return this->isSelectingRectangle;
	}

	bool SelectionManager::getIsSelecting(void) const
	{
		return this->isSelecting;
	}

	void SelectionManager::changeSelectionObserver(SelectionManager::ISelectionObserver* selectionObserver)
	{
		if (nullptr != this->selectionObserver)
		{
			delete this->selectionObserver;
			this->selectionObserver = selectionObserver;
		}
	}

	SelectionManager::ISelectionObserver* SelectionManager::getSelectionObserver(void) const
	{
		return this->selectionObserver;
	}

	void SelectionManager::selectGameObjects(const Ogre::Vector2& leftTopCorner, const Ogre::Vector2& bottomRightCorner)
	{
		// Build the 2D rectangle
		Ogre::Real left = leftTopCorner.x;
		Ogre::Real right = bottomRightCorner.x;
		Ogre::Real top = leftTopCorner.y;
		Ogre::Real bottom = bottomRightCorner.y;

		const Ogre::Real frontDistance = 3.0f;
		const Ogre::Real backDistance = 3.0f + 500.0f;

		if (left > right)   std::swap(left, right);
		if (top > bottom)   std::swap(top, bottom);

		// too small => ignore
		if ((right - left) * (bottom - top) < 0.0001f)
			return;

		Ogre::Ray topLeft = this->camera->getCameraToViewportRay(left, top);
		Ogre::Ray topRight = this->camera->getCameraToViewportRay(right, top);
		Ogre::Ray bottomLeft = this->camera->getCameraToViewportRay(left, bottom);
		Ogre::Ray bottomRight = this->camera->getCameraToViewportRay(right, bottom);

		Ogre::PlaneBoundedVolume vol;
		vol.planes.push_back(Ogre::Plane(topLeft.getPoint(frontDistance), topRight.getPoint(frontDistance), bottomLeft.getPoint(frontDistance)));
		vol.planes.push_back(Ogre::Plane(topLeft.getPoint(backDistance), bottomLeft.getPoint(backDistance), topRight.getPoint(backDistance)));
		vol.planes.push_back(Ogre::Plane(topLeft.getPoint(frontDistance), topLeft.getPoint(backDistance), topRight.getPoint(frontDistance)));
		vol.planes.push_back(Ogre::Plane(bottomLeft.getPoint(frontDistance), bottomRight.getPoint(frontDistance), bottomLeft.getPoint(backDistance)));
		vol.planes.push_back(Ogre::Plane(topLeft.getPoint(frontDistance), bottomLeft.getPoint(frontDistance), topLeft.getPoint(backDistance)));
		vol.planes.push_back(Ogre::Plane(topRight.getPoint(frontDistance), topRight.getPoint(backDistance), bottomRight.getPoint(frontDistance)));

		Ogre::Matrix4 viewMatrix = this->camera->getViewMatrix(true);
		Ogre::Matrix4 projMatrix = this->camera->getProjectionMatrix();
		Ogre::Vector3 camPos = this->camera->getDerivedPosition();

		// Convert viewport [0..1] to NDC [-1..1]
		Ogre::Real ndcLeft = (2.0f * left) - 1.0f;
		Ogre::Real ndcRight = (2.0f * right) - 1.0f;
		Ogre::Real ndcTop = (2.0f * (1.0f - top)) - 1.0f;
		Ogre::Real ndcBottom = (2.0f * (1.0f - bottom)) - 1.0f;

		// Result container shared with render command
		auto hitIds = std::make_shared<std::vector<unsigned long>>();
		auto volCopy = vol;

		NOWA::GraphicsModule::RenderCommand renderCommand =
			[this, hitIds, volCopy, viewMatrix, projMatrix, camPos, ndcLeft, ndcRight, ndcTop, ndcBottom, backDistance]()
			{
				Ogre::PlaneBoundedVolumeList volList;
				volList.push_back(volCopy);

				this->volumeQuery->setVolumes(volList);
				Ogre::SceneQueryResult qryresult = this->volumeQuery->execute();

				for (auto* movable : qryresult.movables)
				{
					if (!movable)
						continue;

					const Ogre::String& type = movable->getMovableType();
					if (type != "Entity" && type != "Item")
						continue;

					// visibility check
					if (type == "Entity")
					{
						Ogre::v1::Entity* e = static_cast<Ogre::v1::Entity*>(movable);
						if (!e->getVisible())
							continue;
					}
					else
					{
						Ogre::Item* it = static_cast<Ogre::Item*>(movable);
						if (!it->getVisible())
							continue;
					}

					size_t vertexCount = 0;
					size_t indexCount = 0;
					Ogre::Vector3* vertices = nullptr;
					unsigned long* indices = nullptr;

					if (type == "Entity")
					{
						Ogre::v1::Entity* entity = static_cast<Ogre::v1::Entity*>(movable);
						MathHelper::getInstance()->getMeshInformation(
							entity->getMesh(),
							vertexCount, vertices,
							indexCount, indices,
							entity->getParentNode()->_getDerivedPosition(),
							entity->getParentNode()->_getDerivedOrientation(),
							entity->getParentNode()->getScale());
					}
					else
					{
						Ogre::Item* item = static_cast<Ogre::Item*>(movable);
						MathHelper::getInstance()->getMeshInformation2(
							item->getMesh(),
							vertexCount, vertices,
							indexCount, indices,
							item->getParentNode()->_getDerivedPosition(),
							item->getParentNode()->_getDerivedOrientation(),
							item->getParentNode()->getScale());
					}

					bool hitfound = false;

					for (size_t i = 0; i < indexCount; ++i)
					{
						const Ogre::Vector3 vertexPos = vertices[indices[i]];

						if ((vertexPos - camPos).length() > backDistance)
							continue;

						const Ogre::Vector3 eyeSpacePos = viewMatrix * vertexPos;
						if (eyeSpacePos.z >= 0) // behind camera
							continue;

						const Ogre::Vector3 screenSpacePos = projMatrix * eyeSpacePos;

						if (screenSpacePos.x > ndcLeft && screenSpacePos.x < ndcRight &&
							screenSpacePos.y < ndcTop && screenSpacePos.y > ndcBottom)
						{
							hitfound = true;

							GameObject* go = nullptr;
							try
							{
								go = Ogre::any_cast<GameObject*>(movable->getUserObjectBindings().getUserAny());
							}
							catch (...)
							{
								go = nullptr;
							}

							if (go)
								hitIds->push_back(go->getId());

							break;
						}
					}

					OGRE_FREE(vertices, Ogre::MEMCATEGORY_GEOMETRY);
					OGRE_FREE(indices, Ogre::MEMCATEGORY_GEOMETRY);

					if (!hitfound)
						continue;
				}

				this->volumeQuery->clearResults();
			};

		// IMPORTANT: wait so hitIds is ready and we avoid races.
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "SelectionManager::selectGameObjects gather");

		// ---- MAIN THREAD applies selection changes (safe) ----

		// Optional: snapshot for undo/redo (like you do on click)
		if (!Core::getSingletonPtr()->getIsGame())
			this->snapshotGameObjectSelection();

		// Rectangle selection: if shift is NOT down, clear first
		if (!this->shiftDown)
			this->clearSelection();

		// Select all hit objects
		for (unsigned long id : *hitIds)
		{
			auto goPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(id);
			if (goPtr)
				this->applySelectInternal(goPtr.get(), true);
		}
	}

	std::unordered_map<unsigned long, SelectionManager::SelectionData>& SelectionManager::getSelectedGameObjects(void)
	{
		return this->selectedGameObjects;
	}

	void SelectionManager::clearSelection(void)
	{
		for (auto it = this->selectedGameObjects.begin(); it != this->selectedGameObjects.end(); )
		{
			GameObject* gameObject = it->second.gameObject;
			++it; // increment first because applySelectInternal erases

			this->applySelectInternal(gameObject, false);
		}

		this->isSelecting = false;
	}

	void SelectionManager::select(const unsigned long id, bool bSelect)
	{
		auto& gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(id);
		if (nullptr != gameObjectPtr)
		{
			this->applySelectInternal(gameObjectPtr.get(), bSelect);
		}
	}

	void SelectionManager::snapshotGameObjectSelection(void)
	{
		// Work with ids instead of pointers for undo/redo, because pointer may become invalid when deleted via undo/redo
		std::vector<unsigned long> gameObjectIds(this->selectedGameObjects.size());
		unsigned int i = 0;
		for (auto& it = this->selectedGameObjects.cbegin(); it != this->selectedGameObjects.cend(); ++it)
		{
			gameObjectIds[i++] = it->second.gameObject->getId();
		}
		if (gameObjectIds.size() > 0)
		{
			this->selectionCommandModule.pushCommand(std::make_shared<GameObjectsSelectUndoCommand>(this, gameObjectIds));
		}
	}

	void SelectionManager::selectionUndo(void)
	{
		if (false == Core::getSingletonPtr()->getIsGame())
		{
			this->selectionCommandModule.undo();
		}
	}

	void SelectionManager::selectionRedo(void)
	{
		if (false == Core::getSingletonPtr()->getIsGame())
		{
			this->selectionCommandModule.redo();
		}
	}

	bool SelectionManager::canSelectionUndo(void)
	{
		if (false == Core::getSingletonPtr()->getIsGame())
		{
			return this->selectionCommandModule.canUndo();
		}
		return false;
	}

	bool SelectionManager::canSelectionRedo(void)
	{
		if (false == Core::getSingletonPtr()->getIsGame())
		{
			return this->selectionCommandModule.canRedo();
		}
		return false;
	}

}; // namespace end
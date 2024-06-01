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
		shiftDown(false),
		selectBegin(Ogre::Vector2::ZERO),
		selectEnd(Ogre::Vector2::ZERO),
		categoryId(0),
		selectionObserver(nullptr)
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
		if (nullptr != this->selectionRect)
		{
			this->selectionNode->detachObject(this->selectionRect);
			delete this->selectionRect;
			this->selectionRect = nullptr;
		}

		this->sceneManager->destroyQuery(this->selectQuery);
		this->sceneManager->destroyQuery(this->volumeQuery);
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

		this->selectionRect = new SelectionRectangle("SelectionRectangle", this->sceneManager, this->camera);
		this->selectionNode = this->sceneManager->getRootSceneNode()->createChildSceneNode();
		this->selectionNode->attachObject(this->selectionRect);
		this->volumeQuery = this->sceneManager->createPlaneBoundedVolumeQuery(Ogre::PlaneBoundedVolumeList(), this->categoryId);
		this->selectQuery = this->sceneManager->createRayQuery(Ogre::Ray(), this->categoryId);
		this->selectQuery->setSortByDistance(true);

		AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &SelectionManager::handleDeleteGameObject), EventDataDeleteGameObject::getStaticEventType());
	}

	void SelectionManager::handleDeleteGameObject(EventDataPtr eventData)
	{
		boost::shared_ptr<EventDataDeleteGameObject> castEventData = boost::static_pointer_cast<NOWA::EventDataDeleteGameObject>(eventData);
	
		GameObject* gameObject = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(castEventData->getGameObjectId()).get();
		if (nullptr != gameObject)
		{
			auto& it = this->selectedGameObjects.find(gameObject->getId());
			// Unselect and remove from map, if it has been selected
			if (it != this->selectedGameObjects.end())
			{
				this->selectedGameObjects.erase(it);
			}
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
		// That means, if user holds shift and selects game objects and at the same moment release the shift key at the same time as the mouse release, only the mouse release is detected and shift variable remains switched on!
		this->shiftDown = NOWA::InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(NOWA_K_SELECT);
		
		if (id == this->mouseButtonId)
		{
			if (false == this->isSelectingRectangle)
			{
				// Snapshot for undo redo selection
				if (false == Core::getSingletonPtr()->getIsGame())
				{
					this->snapshotGameObjectSelection();
				}
				
				// Clear all selected objects, if shift is no hold
				if (false == this->shiftDown)
				{
					for (auto& entry : this->selectedGameObjects)
					{
						this->selectionObserver->onHandleSelection(entry.second.gameObject, false);
						this->isSelecting = false;
						entry.second.gameObject->selected = false;
						// A static game object cannot be moved for performance reasons, so set it dynamic for a short time, move it and reset to static again
						entry.second.gameObject->setDynamic(entry.second.initiallyDynamic);
					}
					this->selectedGameObjects.clear();
				}
				// true at the end: raycast from point does not work correctly so far
				GameObject* selectedGameObject = AppStateManager::getSingletonPtr()->getGameObjectController()->selectGameObject(evt.state.X.abs, evt.state.Y.abs, this->camera, this->selectQuery, true);
				if (nullptr != selectedGameObject)
				{
					auto& it = this->selectedGameObjects.find(selectedGameObject->getId());
					// Unselect and remove from map, if it has been selected
					if (it != this->selectedGameObjects.end())
					{
						this->selectionObserver->onHandleSelection(it->second.gameObject, false);
						it->second.gameObject->selected = false;
						// A static game object cannot be moved for performance reasons, so set it dynamic for a short time, move it and reset to static again
						it->second.gameObject->setDynamic(it->second.initiallyDynamic);
						this->isSelecting = false;
						this->selectedGameObjects.erase(it);
					}
					else
					{
						this->selectionObserver->onHandleSelection(selectedGameObject, true);

						SelectionManager::SelectionData selectionData;
						selectionData.gameObject = selectedGameObject;
						// Store initial state
						selectionData.initiallyDynamic = selectedGameObject->isDynamic();

						this->isSelecting = true;
						selectedGameObject->selected = true;
						// A static game object cannot be moved for performance reasons, so set it dynamic for a short time, move it and reset to static again
						if (false == selectedGameObject->isDynamic())
						{
							selectedGameObject->setDynamic(true);
						}
						this->isSelecting = true;
						this->selectedGameObjects.emplace(selectedGameObject->getId(), selectionData);
					}
				}
				else
				{
					// Nothing found for selection so un-selected everything
					for (auto& it = this->selectedGameObjects.begin(); it != this->selectedGameObjects.end();)
					{
						this->selectionObserver->onHandleSelection(it->second.gameObject, false);
						it->second.gameObject->selected = false;
						// A static game object cannot be moved for performance reasons, so set it dynamic for a short time, move it and reset to static again
						it->second.gameObject->setDynamic(it->second.initiallyDynamic);
						this->isSelecting = false;
						this->selectedGameObjects.erase(it++);
					}
				}
			}

			MathHelper::getInstance()->mouseToViewPort(evt.state.X.abs, evt.state.Y.abs, this->selectBegin.x, this->selectBegin.y, Core::getSingletonPtr()->getOgreRenderWindow());
			this->mouseIdPressed = true;
			// this->selectionRect->clear();
		}
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
			//		entry.second->selected = false;
			//		this->isSelecting = false;
			//	}
			//	selectedGameObjects.clear();
			//}

			// Select all objects that are within the selection rectangle area
			this->selectGameObjects(this->selectBegin, this->selectEnd);
			for (auto& entry : this->selectedGameObjects)
			{
				this->selectionObserver->onHandleSelection(entry.second.gameObject, true);
				entry.second.gameObject->selected = true;
				// A static game object cannot be moved for performance reasons, so set it dynamic for a short time, move it and reset to static again
				if (false == entry.second.gameObject->isDynamic())
				{
					entry.second.gameObject->setDynamic(true);
				}
				this->isSelecting = true;
			}

			this->isSelectingRectangle = false;
			// Hide the rectangle
			this->selectionRect->setVisible(false);
		}
	}

	unsigned int SelectionManager::filterCategories(Ogre::String& categories)
	{
		this->categories = categories;
		this->categoryId = AppStateManager::getSingletonPtr()->getGameObjectController()->generateCategoryId(categories);
		this->volumeQuery->setQueryMask(this->categoryId);
		this->selectQuery->setQueryMask(this->categoryId);
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

	void SelectionManager::selectGameObjects(const Ogre::Vector2& leftTopCorner, const Ogre::Vector2& bottomRightCorner)
	{
		// Build the 2D rectangle
		Ogre::Real left = leftTopCorner.x;
		Ogre::Real right = bottomRightCorner.x;
		Ogre::Real top = leftTopCorner.y;
		Ogre::Real bottom = bottomRightCorner.y;

		Ogre::Real frontDistance = 3.0f;
		Ogre::Real backDistance = 3.0f + 500.0f;

		if (left > right)
		{
			std::swap(left, right);
		}
		if (top > bottom)
		{
			std::swap(top, bottom);
		}

		//if its too small leave the function
		if ((right - left) * (bottom - top) < 0.0001f)
		{
			return;
		}

		Ogre::Ray topLeft, topRight, bottomLeft, bottomRight;

		topLeft = this->camera->getCameraToViewportRay(left, top);
		topRight = this->camera->getCameraToViewportRay(right, top);
		bottomLeft = this->camera->getCameraToViewportRay(left, bottom);
		bottomRight = this->camera->getCameraToViewportRay(right, bottom);

		Ogre::PlaneBoundedVolume vol;
		// front plane
		vol.planes.push_back(Ogre::Plane(topLeft.getPoint(frontDistance), topRight.getPoint(frontDistance), bottomLeft.getPoint(frontDistance)));
		// back plane
		vol.planes.push_back(Ogre::Plane(topLeft.getPoint(backDistance), bottomLeft.getPoint(backDistance), topRight.getPoint(backDistance)));
		// top plane
		vol.planes.push_back(Ogre::Plane(topLeft.getPoint(frontDistance), topLeft.getPoint(backDistance), topRight.getPoint(frontDistance)));
		// bottom plane
		vol.planes.push_back(Ogre::Plane(bottomLeft.getPoint(frontDistance), bottomRight.getPoint(frontDistance), bottomLeft.getPoint(backDistance)));
		// left plane
		vol.planes.push_back(Ogre::Plane(topLeft.getPoint(frontDistance), bottomLeft.getPoint(frontDistance), topLeft.getPoint(backDistance)));
		// right plane
		vol.planes.push_back(Ogre::Plane(topRight.getPoint(frontDistance), topRight.getPoint(backDistance), bottomRight.getPoint(frontDistance)));

		Ogre::PlaneBoundedVolumeList volList;
		volList.push_back(vol);
		this->volumeQuery->setVolumes(volList);

		Ogre::SceneQueryResult qryresult = this->volumeQuery->execute();

		Ogre::Matrix4 viewMatrix = this->camera->getViewMatrix(true);
		Ogre::Matrix4 projMatrix = this->camera->getProjectionMatrix();
		Ogre::Vector3 camPos = this->camera->getDerivedPosition();

		left = (2.0f * left) - 1.0f;
		right = (2.0f * right) - 1.0f;
		top = (2.0f * (1.0f - top)) - 1.0f;
		bottom = (2.0f * (1.0f - bottom)) - 1.0f;

		Ogre::SceneQueryResultMovableList::iterator itr;
		for (itr = qryresult.movables.begin(); itr != qryresult.movables.end(); ++itr)
		{
			// only check this result if its a hit against an entity
			if ((*itr)->getMovableType().compare("Entity") == 0)
			{
				// get the entity to check
				Ogre::v1::Entity* entity = static_cast<Ogre::v1::Entity*>(*itr);

				if (!entity->getVisible())
				{
					continue;
				}

				// get the mesh information
				size_t vertexCount;
				size_t indexCount;
				Ogre::Vector3* vertices = nullptr;
				unsigned long* indices = nullptr;

				// Get the mesh information
				MathHelper::getInstance()->getMeshInformation(entity->getMesh(), vertexCount, vertices,
					indexCount, indices, entity->getParentNode()->_getDerivedPosition(), entity->getParentNode()->_getDerivedOrientation(), entity->getParentNode()->getScale());

				bool hitfound = false;
				for (int i = 0; i < static_cast<int>(indexCount); i++)
				{
					Ogre::Vector3 vertexPos = vertices[indices[i]];

					if ((vertexPos - camPos).length() > backDistance)
					{
						continue;
					}
					Ogre::Vector3 eyeSpacePos = viewMatrix * vertexPos;
					// z < 0 means in front of cam
					if (eyeSpacePos.z < 0)
					{
						// calculate projected pos
						Ogre::Vector3 screenSpacePos = projMatrix * eyeSpacePos;
                
						if (screenSpacePos.x > left && screenSpacePos.x < right && screenSpacePos.y < top && screenSpacePos.y > bottom)
						{
							hitfound = true;

							GameObject* gameObject = nullptr;
							try
							{
								// Here no shared_ptr because in this scope the game object should not extend the lifecycle! Only shared where really necessary
								gameObject = Ogre::any_cast<GameObject*>((*itr)->getUserObjectBindings().getUserAny());

							}
							catch (...)
							{

							}

							if (nullptr != gameObject)
							{
								auto& it = this->selectedGameObjects.find(gameObject->getId());
								if (it != this->selectedGameObjects.end())
								{
									this->selectionObserver->onHandleSelection(it->second.gameObject, false);
									it->second.gameObject->selected = false;
									this->isSelecting = false;
									this->selectedGameObjects.erase(it);
								}
								else
								{
									this->selectionObserver->onHandleSelection(gameObject, true);
									gameObject->selected = true;

									SelectionManager::SelectionData selectionData;
									selectionData.gameObject = gameObject;
									// Store initial state
									selectionData.initiallyDynamic = gameObject->isDynamic();

									// A static game object cannot be moved for performance reasons, so set it dynamic for a short time, move it and reset to static again
									if (false == gameObject->isDynamic())
									{
										gameObject->setDynamic(true);
									}
									this->isSelecting = true;
									
									this->selectedGameObjects.emplace(gameObject->getId(), selectionData);
								}
							}
							break;
						}
					}
				}

				if (!hitfound)
				{
					continue;
				}
				// free the verticies and indicies memory
				OGRE_FREE(vertices, Ogre::MEMCATEGORY_GEOMETRY);
				OGRE_FREE(indices, Ogre::MEMCATEGORY_GEOMETRY);
			}
		}
		this->volumeQuery->clearResults();
	}

	std::unordered_map<unsigned long, SelectionManager::SelectionData>& SelectionManager::getSelectedGameObjects(void)
	{
		return this->selectedGameObjects;
	}

	void SelectionManager::clearSelection(void)
	{
		for (auto& entry : this->selectedGameObjects)
		{
			this->selectionObserver->onHandleSelection(entry.second.gameObject, false);
			entry.second.gameObject->selected = false;
			// A static game object cannot be moved for performance reasons, so set it dynamic for a short time, move it and reset to static again
			entry.second.gameObject->setDynamic(entry.second.initiallyDynamic);
			this->isSelecting = false;
		}
		this->selectedGameObjects.clear();
	}

	void SelectionManager::select(const unsigned long id, bool bSelect)
	{
		auto& gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(id);
		if (nullptr != gameObjectPtr)
		{
			SelectionManager::SelectionData selectionData;
			selectionData.gameObject = gameObjectPtr.get();
			// Store initial state
			selectionData.initiallyDynamic = gameObjectPtr.get()->isDynamic();

			this->selectedGameObjects.emplace(id, selectionData);
			this->selectionObserver->onHandleSelection(gameObjectPtr.get(), bSelect);
			gameObjectPtr->selected = bSelect;
			// A static game object cannot be moved for performance reasons, so set it dynamic for a short time, move it and reset to static again
			if (false == gameObjectPtr->isDynamic())
			{
				gameObjectPtr->setDynamic(true);
			}
			this->isSelecting = true;
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
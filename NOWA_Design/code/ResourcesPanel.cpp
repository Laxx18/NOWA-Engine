#include "NOWAPrecompiled.h"
#include "ResourcesPanel.h"
#include "TreeControl/TreeControl.h"
#include "TreeControl/TreeControlItem.h"
#include "main/EventManager.h"
#include "GuiEvents.h"
#include "MyGUIHelper.h"

ResourcesPanel::ResourcesPanel(const MyGUI::FloatCoord& coords)
	: BaseLayout("ResourcesPanelView.layout"),
	editorManager(nullptr),
	resourcesPanelView1(nullptr),
	resourcesPanelView2(nullptr),
	resourcesPanelMeshes(nullptr),
#if 0
	resourcesPanelMeshPreview(nullptr),
#endif
	resourcesPanelGameObjects(nullptr),
	resourcesPanelDataBlocks(nullptr),
	resourcesPanelTextures(nullptr)
{
	// Strategy as follows:
	// - In ResourcesPanelView.layout, there are two tab items, each one with a named scrollviewer
	// - A scroll viewer represents the view container for a list of panels
	// - All scroll viewer are belonging to one base ResourcesPanelView layout 
	// - So two resourcesPanelView's are created resourcesPanelView1, resourcesPanelView2
	// - Each one is assigned with the corresponding names scroll viewer: resources1ScrollView, resources2ScrollView
	// - Now its possible to add panels to the wished resourcesPanel, to group them in tab items!


	this->mMainWidget->setRealCoord(coords);
	assignBase(this->resourcesPanelView1, "resources1ScrollView");

	this->resourcesPanelMeshes = new ResourcesPanelMeshes();
	this->resourcesPanelView1->addItem(this->resourcesPanelMeshes);
#if 0
	this->resourcesPanelMeshPreview = new ResourcesPanelMeshPreview();
	this->resourcesPanelView->addItem(this->resourcesPanelMeshPreview);
#endif
	this->resourcesPanelGameObjects = new ResourcesPanelGameObjects();
	this->resourcesPanelView1->addItem(this->resourcesPanelGameObjects);

	assignBase(this->resourcesPanelView2, "resources2ScrollView");
	this->resourcesPanelDataBlocks = new ResourcesPanelDataBlocks();
	this->resourcesPanelView2->addItem(this->resourcesPanelDataBlocks);

	this->resourcesPanelTextures = new ResourcesPanelTextures();
	this->resourcesPanelView2->addItem(this->resourcesPanelTextures);
}

void ResourcesPanel::setEditorManager(NOWA::EditorManager* editorManager)
{
	this->editorManager = editorManager;
	this->resourcesPanelMeshes->setEditorManager(this->editorManager);
#if 0
	this->resourcesPanelMeshPreview->setEditorManager(this->editorManager);
#endif
	this->resourcesPanelGameObjects->setEditorManager(this->editorManager);
	this->resourcesPanelDataBlocks->setEditorManager(this->editorManager);
	this->resourcesPanelTextures->setEditorManager(this->editorManager);
}

void ResourcesPanel::destroyContent(void)
{
	this->resourcesPanelView1->removeAllItems();

	if (this->resourcesPanelMeshes)
	{
		delete this->resourcesPanelMeshes;
		this->resourcesPanelMeshes = nullptr;
	}
#if 0
	if (this->resourcesPanelMeshPreview)
	{
		delete this->resourcesPanelMeshPreview;
		this->resourcesPanelMeshPreview = nullptr;
	}
#endif
	if (this->resourcesPanelGameObjects)
	{
		delete this->resourcesPanelGameObjects;
		this->resourcesPanelGameObjects = nullptr;
	}

	this->resourcesPanelView2->removeAllItems();

	if (this->resourcesPanelDataBlocks)
	{
		delete this->resourcesPanelDataBlocks;
		this->resourcesPanelDataBlocks = nullptr;
	}
	if (this->resourcesPanelTextures)
	{
		delete this->resourcesPanelTextures;
		this->resourcesPanelTextures = nullptr;
	}
}

void ResourcesPanel::setVisible(bool show)
{
	this->mMainWidget->setVisible(show);
}

void ResourcesPanel::refresh(void)
{
	this->resourcesPanelGameObjects->refresh("");
}

void ResourcesPanel::clear(void)
{
	this->resourcesPanelMeshes->clear();
	this->resourcesPanelGameObjects->clear();
	this->resourcesPanelDataBlocks->clear();
	// this->resourcesPanelTextures->clear();
}

/////////////////////////////////////////////////////////////////////////////

ResourcesPanelMeshes::ResourcesPanelMeshes()
	: BasePanelViewItem("ResourcesPanel.layout"),
	editorManager(nullptr),
	meshesTree(nullptr),
	imageBox(nullptr)
{
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &ResourcesPanelMeshes::handleRefreshMeshResources), EventDataRefreshMeshResources::getStaticEventType());
}

void ResourcesPanelMeshes::setEditorManager(NOWA::EditorManager* editorManager)
{
	this->editorManager = editorManager;
}

void ResourcesPanelMeshes::initialise()
{
	mPanelCell->setCaption("Meshes");
	mPanelCell->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
	mPanelCell->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());

	assignWidget(this->meshesTree, "Tree");
	assignWidget(this->imageBox, "previewImage");
	this->meshesTree->eventTreeNodePrepare += newDelegate(this, &ResourcesPanelMeshes::notifyTreeNodePrepare);
	this->meshesTree->eventTreeNodeSelected += newDelegate(this, &ResourcesPanelMeshes::notifyTreeNodeSelected);

	assignWidget(this->resourcesSearchEdit, "ResourcesSearchEdit");
	this->resourcesSearchEdit->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);
	this->resourcesSearchEdit->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
	this->resourcesSearchEdit->setEditStatic(false);
	this->resourcesSearchEdit->setEditReadOnly(false);
	this->resourcesSearchEdit->eventEditTextChange += MyGUI::newDelegate(this, &ResourcesPanelMeshes::editTextChange);

	this->imageBox->setVisible(false);

	this->loadMeshes("");
}

void ResourcesPanelMeshes::shutdown()
{
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ResourcesPanelMeshes::handleRefreshMeshResources), EventDataRefreshMeshResources::getStaticEventType());
}

void ResourcesPanelMeshes::editTextChange(MyGUI::Widget* sender)
{
	MyGUI::EditBox* editBox = static_cast<MyGUI::EditBox*>(sender);

	// Start a new search each time for resources that do match the search caption string
	this->autoCompleteSearch.reset();
	this->clear();

	this->loadMeshes(editBox->getOnlyText());
}

void ResourcesPanelMeshes::handleRefreshMeshResources(NOWA::EventDataPtr eventData)
{
	this->clear();
}

void ResourcesPanelMeshes::loadMeshes(const Ogre::String& filter)
{
	MyGUI::TreeControl::Node* root = this->meshesTree->getRoot();
	root->removeAll();

	MyGUI::TreeControl::Node* parent = nullptr;
	MyGUI::TreeControl::Node* child = nullptr;

	// Other treatment, if the user searches for a resource
	if (true == filter.empty())
	{
		// Add other resource group for plane, light, etc.
		parent = new MyGUI::TreeControl::Node("Other Resources", "Data");
		// Add plane separatly
		{
			child = new MyGUI::TreeControl::Node("Plane", "Data");
			parent->add(child);
		}
		// Add mirror separatly
		{
			child = new MyGUI::TreeControl::Node("Mirror", "Data");
			parent->add(child);
		}
		// Add directional light
		{
			child = new MyGUI::TreeControl::Node("Light (Directional)", "Data");
			parent->add(child);
		}
		// Add spot light
		{
			child = new MyGUI::TreeControl::Node("Light (Spot)", "Data");
			parent->add(child);
		}
		// Add point light
		{
			child = new MyGUI::TreeControl::Node("Light (Point)", "Data");
			parent->add(child);
		}
		// Add area light
		{
			child = new MyGUI::TreeControl::Node("Light (Area)", "Data");
			parent->add(child);
		}
		// Add Camera
		{
			child = new MyGUI::TreeControl::Node("Camera", "Data");
			parent->add(child);
		}
		// Add Reflection Camera
		{
			child = new MyGUI::TreeControl::Node("Reflection Camera", "Data");
			parent->add(child);
		}
		// Add Node
		{
			child = new MyGUI::TreeControl::Node("Node", "Data");
			parent->add(child);
		}
		// Add Lines
		{
			child = new MyGUI::TreeControl::Node("Lines", "Data");
			parent->add(child);
		}
		// Add ManualObject
		{
			child = new MyGUI::TreeControl::Node("ManualObject", "Data");
			parent->add(child);
		}
		// Add Rectangle
		{
			child = new MyGUI::TreeControl::Node("Rectangle", "Data");
			parent->add(child);
		}
		// Add Decal
		{
			child = new MyGUI::TreeControl::Node("Decal", "Data");
			parent->add(child);
		}
		// Add Ocean
		{
			child = new MyGUI::TreeControl::Node("Ocean", "Data");
			parent->add(child);
		}
		// Add Terra
		{
			child = new MyGUI::TreeControl::Node("Terra", "Data");
			parent->add(child);
		}
	
		root->add(parent);
	}

	Ogre::StringVector groups = Ogre::ResourceGroupManager::getSingletonPtr()->getResourceGroups();
	std::vector<Ogre::String>::iterator groupIt = groups.begin();
	while (groupIt != groups.end())
	{
		Ogre::String groupName = (*groupIt);
		if (groupName != "Essential" && groupName != "General" && groupName != "PostProcessing" && groupName != "Hlms"
			&& groupName != "NOWA" && groupName != "Internal" && groupName != "AutoDetect" && groupName != "Lua")
		{

			Ogre::StringVectorPtr dirs = Ogre::ResourceGroupManager::getSingletonPtr()->findResourceNames((*groupIt), "*.mesh");
			std::vector<Ogre::String>::iterator meshIt = dirs->begin();
			if (true == filter.empty())
			{
				if (dirs->begin() != dirs->end())
				{
					parent = new MyGUI::TreeControl::Node(groupName, "Data");
					// pNode->setData(PairFileInfo(gMediaBase, *item));
					root->add(parent);
				}
			}

			while (meshIt != dirs->end())
			{
				if (true == filter.empty())
				{
					child = new MyGUI::TreeControl::Node((*meshIt), "Data");

					parent->add(child);
				}
				else
				{
					// Add resource to the search
					this->autoCompleteSearch.addSearchText((*meshIt));
				}
				++meshIt;
			}

		}
		++groupIt;
	}

	if (false == filter.empty())
	{
		// Get the matched results and add to tree
		auto& matchedResources = this->autoCompleteSearch.findMatchedItemWithInText(filter);

		for (size_t i = 0; i < matchedResources.getResults().size(); i++)
		{
			Ogre::String resourceName = matchedResources.getResults()[i].getMatchedItemText();
			child = new MyGUI::TreeControl::Node(resourceName, "Data");

			root->add(child);
		}
	}
}

void ResourcesPanelMeshes::clear(void)
{
	MyGUI::TreeControl::Node* root = this->meshesTree->getRoot();
	root->removeAll();

	this->loadMeshes("");
}

void ResourcesPanelMeshes::notifyTreeNodeSelected(MyGUI::TreeControl* treeControl, MyGUI::TreeControl::Node* node)
{
	if (nullptr == node)
	{
		return;
	}

	if ("Plane" == Ogre::String(node->getText()))
	{
		this->editorManager->attachOtherResourceToPlaceNode(NOWA::GameObject::PLANE);
	}
	else if ("Mirror" == Ogre::String(node->getText()))
	{
		this->editorManager->attachOtherResourceToPlaceNode(NOWA::GameObject::MIRROR);
	}
	else if ("Light (Directional)" == Ogre::String(node->getText()))
	{
		this->editorManager->attachOtherResourceToPlaceNode(NOWA::GameObject::LIGHT_DIRECTIONAL);
		// Later attachLightToPlaceNode: Internally like Ogitor a light mesh is created
	}
	else if ("Light (Spot)" == Ogre::String(node->getText()))
	{
		this->editorManager->attachOtherResourceToPlaceNode(NOWA::GameObject::LIGHT_SPOT);
	}
	else if ("Light (Point)" == Ogre::String(node->getText()))
	{
		this->editorManager->attachOtherResourceToPlaceNode(NOWA::GameObject::LIGHT_POINT);
	}
	else if ("Light (Area)" == Ogre::String(node->getText()))
	{
		this->editorManager->attachOtherResourceToPlaceNode(NOWA::GameObject::LIGHT_AREA);
	}
	else if ("Camera" == Ogre::String(node->getText()))
	{
		this->editorManager->attachOtherResourceToPlaceNode(NOWA::GameObject::CAMERA);
	}
	else if ("Reflection Camera" == Ogre::String(node->getText()))
	{
		this->editorManager->attachOtherResourceToPlaceNode(NOWA::GameObject::REFLECTION_CAMERA);
	}
	else if ("Node" == Ogre::String(node->getText()))
	{
		this->editorManager->attachOtherResourceToPlaceNode(NOWA::GameObject::SCENE_NODE);
	}
	else if ("Lines" == Ogre::String(node->getText()))
	{
		this->editorManager->attachOtherResourceToPlaceNode(NOWA::GameObject::LINES);
	}
	else if ("ManualObject" == Ogre::String(node->getText()))
	{
		this->editorManager->attachOtherResourceToPlaceNode(NOWA::GameObject::MANUAL_OBJECT);
	}
	else if ("Rectangle" == Ogre::String(node->getText()))
	{
		this->editorManager->attachOtherResourceToPlaceNode(NOWA::GameObject::RECTANGLE);
	}
	else if ("Decal" == Ogre::String(node->getText()))
	{
		this->editorManager->attachOtherResourceToPlaceNode(NOWA::GameObject::DECAL);
	}
	else if ("Ocean" == Ogre::String(node->getText()))
	{
		this->editorManager->attachOtherResourceToPlaceNode(NOWA::GameObject::OCEAN);
	}
	else if ("Terra" == Ogre::String(node->getText()))
	{
		this->editorManager->attachOtherResourceToPlaceNode(NOWA::GameObject::TERRA);
	}
	else
	{
		Ogre::String meshName = node->getText();
		size_t pos = meshName.rfind(".mesh");
		if (Ogre::String::npos != pos)
		{
			this->editorManager->attachMeshToPlaceNode(meshName, NOWA::GameObject::ENTITY);
			// Escape should detach, and if attached and in place mode, all other modes must be disabled
			// when clicking in place mode in editor, a new game object etc. must be created
		}
	}
	// node->setText("");
}

void ResourcesPanelMeshes::notifyTreeNodePrepare(MyGUI::TreeControl* treeControl, MyGUI::TreeControl::Node* node)
{

}

/////////////////////////////////////////////////////////////////////////////

#if 0
ResourcesPanelMeshPreview::ResourcesPanelMeshPreview()
	: BasePanelViewItem("ResourcesPanel.layout"),
	editorManager(nullptr),
	previewWindow(nullptr)
{

}

void ResourcesPanelMeshPreview::setEditorManager(NOWA::EditorManager* editorManager)
{
	this->editorManager = editorManager;
}

void ResourcesPanelMeshPreview::initialise()
{
	mPanelCell->setCaption("Preview");
	mPanelCell->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());

	assignWidget(this->previewWindow, "previewWindow");

	this->actualizeMesh();
	
}

void ResourcesPanelMeshPreview::shutdown()
{

}

void ResourcesPanelMeshPreview::actualizeMesh(void)
{
	const MyGUI::IntSize& size = MyGUI::RenderManager::getInstance().getViewSize();

	MyGUI::Canvas* canvas = this->previewWindow->createWidget<MyGUI::Canvas>("Canvas", MyGUI::IntCoord(0, 0, previewWindow->getClientCoord().width, previewWindow->getClientCoord().height), MyGUI::Align::Stretch);
	canvas->setPointer("hand");

	/*this->renderBox.setCanvas(canvas);
	this->renderBox.setViewport(getCamera());
	this->renderBox.setBackgroundColour(MyGUI::Colour::Black);*/

	this->renderBoxScene.setCanvas(canvas);
	this->renderBoxScene.injectObject("Case1.mesh");
	this->renderBoxScene.setAutoRotation(true);
	this->renderBoxScene.setMouseRotation(true);
}

void ResourcesPanelMeshPreview::clear(void)
{
	// this->actualizeMesh();
}
#endif

/////////////////////////////////////////////////////////////////////////////

ResourcesPanelGameObjects::ResourcesPanelGameObjects()
	: BasePanelViewItem("ResourcesPanel.layout"),
	editorManager(nullptr),
	gameObjectsTree(nullptr),
	imageBox(nullptr)
{
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &ResourcesPanelGameObjects::handleRefreshGameObjectsPanel), EventDataRefreshResourcesPanel::getStaticEventType());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &ResourcesPanelGameObjects::handleRefreshGameObjectsPanel), NOWA::EventDataNewGameObject::getStaticEventType());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &ResourcesPanelGameObjects::handleRefreshGameObjectsPanel), NOWA::EventDataEditorMode::getStaticEventType());
}

void ResourcesPanelGameObjects::setEditorManager(NOWA::EditorManager* editorManager)
{
	this->editorManager = editorManager;
}

void ResourcesPanelGameObjects::initialise()
{
	mPanelCell->setCaption("GameObjects");
	mPanelCell->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());

	assignWidget(this->gameObjectsTree, "Tree");
	assignWidget(this->imageBox, "previewImage");
	this->gameObjectsTree->eventTreeNodeSelected += newDelegate(this, &ResourcesPanelGameObjects::notifyTreeNodeSelected);
	this->gameObjectsTree->eventKeyButtonPressed += newDelegate(this, &ResourcesPanelGameObjects::keyButtonPressed);
	// Not necessary, else game objects are filled two times -> performance
	// this->refresh();
	assignWidget(this->resourcesSearchEdit, "ResourcesSearchEdit");
	this->resourcesSearchEdit->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
	this->resourcesSearchEdit->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);
	this->resourcesSearchEdit->setEditStatic(false);
	this->resourcesSearchEdit->setEditReadOnly(false);
	this->resourcesSearchEdit->eventEditTextChange += MyGUI::newDelegate(this, &ResourcesPanelGameObjects::editTextChange);
	this->resourcesSearchEdit->eventMouseButtonClick += MyGUI::newDelegate(this, &ResourcesPanelGameObjects::onMouseClick);

	this->imageBox->setVisible(false);
}

void ResourcesPanelGameObjects::shutdown()
{
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ResourcesPanelGameObjects::handleRefreshGameObjectsPanel), EventDataRefreshResourcesPanel::getStaticEventType());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ResourcesPanelGameObjects::handleRefreshGameObjectsPanel), NOWA::EventDataNewGameObject::getStaticEventType());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ResourcesPanelGameObjects::handleRefreshGameObjectsPanel), NOWA::EventDataEditorMode::getStaticEventType());
}

void ResourcesPanelGameObjects::editTextChange(MyGUI::Widget* sender)
{
	MyGUI::EditBox* editBox = static_cast<MyGUI::EditBox*>(sender);

	// Start a new search each time for resources that do match the search caption string
	this->autoCompleteSearch.reset();
	this->clear();

	this->refresh(editBox->getOnlyText());
}

void ResourcesPanelGameObjects::onMouseClick(MyGUI::Widget* sender)
{
	this->editTextChange(sender);
}

void ResourcesPanelGameObjects::refresh(const Ogre::String& filter)
{
	MyGUI::TreeControl::Node* root = this->gameObjectsTree->getRoot();
	root->removeAll();

	/* auto gameObjects = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects();
	for (auto& it = gameObjects->cbegin(); it != gameObjects->cend(); ++it)
	{
		NOWA::GameObjectPtr gameObjectPtr = it->second;
		
		MyGUI::TreeControl::Node* gameObjectNode = new MyGUI::TreeControl::Node(gameObjectPtr->getName(), "Data");
		root->add(gameObjectNode);
	}*/

	bool foundMainGameObjects = false;
	// Group game objects according categories
	auto categories = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getAllCategoriesSoFar();
	for (size_t i = 0; i < categories.size(); i++)
	{
		MyGUI::TreeControl::Node* categoryNode = nullptr;
		if (true == filter.empty())
		{
			categoryNode = new MyGUI::TreeControl::Node(categories[i], "Data");
			categoryNode->setExpanded(true);
		}
		std::pair<std::vector<NOWA::GameObjectPtr>, std::vector<NOWA::GameObjectPtr>> gameObjectData = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsFromCategorySeparate(categories[i], { "MainGameObject", "SunLight", "MainCamera" });

		// Sort ascending order
		std::sort(gameObjectData.first.begin(), gameObjectData.first.end(), [](const NOWA::GameObjectPtr& go1, const NOWA::GameObjectPtr& go2) -> bool
		{
			return go1->getName().compare(go2->getName()) < 0;
		});

		if (false == foundMainGameObjects)
		{
			for (size_t j = 0; j < gameObjectData.second.size(); j++)
			{
				foundMainGameObjects = true;
				NOWA::GameObjectPtr gameObjectPtr = gameObjectData.second[j];
				if (true == filter.empty())
				{
					MyGUI::TreeControl::Node* gameObjectNode = new MyGUI::TreeControl::Node(gameObjectPtr->getName(), "Data");
					gameObjectNode->setExpanded(true);
					categoryNode->add(gameObjectNode);
				}
				else
				{
					// Add resource to the search
					this->autoCompleteSearch.addSearchText(gameObjectPtr->getName());
				}
			}
		}

		for (size_t j = 0; j < gameObjectData.first.size(); j++)
		{
			NOWA::GameObjectPtr gameObjectPtr = gameObjectData.first[j];
			if (true == filter.empty())
			{
				MyGUI::TreeControl::Node* gameObjectNode = new MyGUI::TreeControl::Node(gameObjectPtr->getName(), "Data");
				gameObjectNode->setExpanded(true);
				categoryNode->add(gameObjectNode);
			}
			else
			{
				// Add resource to the search
				this->autoCompleteSearch.addSearchText(gameObjectPtr->getName());
			}
			/*if (nullptr != this->editorManager)
			{
				for (size_t i = 0; i < this->editorManager->getSelectionManager()->getSelectedGameObjects().size(); i++)
				{
					if (gameObjectPtr->getId() == this->editorManager->getSelectionManager()->getSelectedGameObjects()[i]->getId())
					{
						this->gameObjectsTree->setSelection(gameObjectNode);
					}
				}
			}*/
		}
		if (true == filter.empty())
		{
			root->add(categoryNode);
		}
	}

	if (false == filter.empty())
	{
		MyGUI::TreeControl::Node* child = nullptr;
		// Get the matched results and add to tree
		auto& matchedResources = this->autoCompleteSearch.findMatchedItemWithInText(filter);

		for (size_t i = 0; i < matchedResources.getResults().size(); i++)
		{
			Ogre::String resourceName = matchedResources.getResults()[i].getMatchedItemText();
			child = new MyGUI::TreeControl::Node(resourceName, "Data");

			root->add(child);
		}
	}

	root->setExpanded(true);
}

void ResourcesPanelGameObjects::clear(void)
{
	MyGUI::TreeControl::Node* root = this->gameObjectsTree->getRoot();
	root->removeAll();

	this->refresh("");
}

void ResourcesPanelGameObjects::notifyTreeNodeSelected(MyGUI::TreeControl* treeControl, MyGUI::TreeControl::Node* node)
{
	if (nullptr == node /*|| this->oldSelectedText == Ogre::String(node->getText())*/)
	{
		return;
	}
	this->oldSelectedText = node->getText();

	if (false == NOWA::InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(OIS::KC_LSHIFT))
	{
		this->editorManager->getSelectionManager()->clearSelection();
		// treeControl->clearSelection();
	}
	// Delete game objects
	if (true == NOWA::InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(OIS::KC_DELETE))
	{
		std::vector<unsigned long> gameObjectIds;
		// Do not delete directly via selection manager, because when internally deleted, an event is sent out to selection manager to remove from map
		for (auto& it = this->editorManager->getSelectionManager()->getSelectedGameObjects().begin(); it != this->editorManager->getSelectionManager()->getSelectedGameObjects().end(); ++it)
		{
			// SunLight must not be deleted by the user!
			if (it->second.gameObject->getId() != NOWA::GameObjectController::MAIN_LIGHT_ID)
			{
				gameObjectIds.emplace_back(it->second.gameObject->getId());
			}
			// MainCamera must not be deleted by the user!
			if (it->second.gameObject->getId() != NOWA::GameObjectController::MAIN_CAMERA_ID)
			{
				gameObjectIds.emplace_back(it->second.gameObject->getId());
			}
			// MainGameObject must not be deleted by the user!
			if (it->second.gameObject->getId() != NOWA::GameObjectController::MAIN_GAMEOBJECT_ID)
			{
				gameObjectIds.emplace_back(it->second.gameObject->getId());
			}
			// Terra must not be deleted by the user!
			/*if (it->second.gameObject->getName() != "Terra")
			{
				gameObjectIds.emplace_back(it->second.gameObject->getId());
			}*/
		}
		if (gameObjectIds.size() > 0)
		{
			this->editorManager->deleteGameObjects(gameObjectIds);
			this->editorManager->getSelectionManager()->clearSelection();
			boost::shared_ptr<EventDataRefreshPropertiesPanel> eventDataRefreshPropertiesPanel(new EventDataRefreshPropertiesPanel());
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshPropertiesPanel);

			boost::shared_ptr<EventDataRefreshResourcesPanel> eventDataRefreshResourcesPanel(new EventDataRefreshResourcesPanel());
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshResourcesPanel);

			// Regenerate categories
			boost::shared_ptr<NOWA::EventDataGenerateCategories> eventDataGenerateCategories(new NOWA::EventDataGenerateCategories());
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataGenerateCategories);
		}
	}

	// Select game object
	auto& gameObjectPtr = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromName(node->getText());
	if (nullptr != gameObjectPtr)
	{
		this->editorManager->getSelectionManager()->snapshotGameObjectSelection();
		this->editorManager->getSelectionManager()->select(gameObjectPtr->getId());
		// Focus object if Alt has been pressed
		if (GetAsyncKeyState(VK_RCONTROL))
			this->editorManager->focusCameraGameObject(gameObjectPtr.get());
		// treeControl->setSelection(node);
		// treeControl->addSelection(node);
	}

	// Check if a category has been selected and select all game objects in category
	if (NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->hasCategory(node->getText()))
	{
		auto gameObjects = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsFromCategory(node->getText());

		if (false == gameObjects.empty())
		{
			this->editorManager->getSelectionManager()->snapshotGameObjectSelection();
		}

		for (size_t i = 0; i < gameObjects.size(); i++)
		{
			this->editorManager->getSelectionManager()->select(gameObjects[i]->getId());
		}
	}

	// Sent when game objects are selected, so that the properties panel can be refreshed with new values
	boost::shared_ptr<EventDataRefreshPropertiesPanel> eventDataRefreshPropertiesPanel(new EventDataRefreshPropertiesPanel());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshPropertiesPanel);
}

void ResourcesPanelGameObjects::keyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char _char)
{
	if (MyGUI::KeyCode::Delete == key)
	{
		std::vector<unsigned long> gameObjectIds;
		// Do not delete directly via selection manager, because when internally deleted, an event is sent out to selection manager to remove from map
		for (auto& it = this->editorManager->getSelectionManager()->getSelectedGameObjects().begin(); it != this->editorManager->getSelectionManager()->getSelectedGameObjects().end(); ++it)
		{
			// SunLight must not be deleted by the user!
			if (it->second.gameObject->getId() != NOWA::GameObjectController::MAIN_LIGHT_ID && it->second.gameObject->getId() != NOWA::GameObjectController::MAIN_CAMERA_ID 
				&& it->second.gameObject->getId() != NOWA::GameObjectController::MAIN_GAMEOBJECT_ID)
			{
				gameObjectIds.emplace_back(it->second.gameObject->getId());
			}
		}
		if (gameObjectIds.size() > 0)
		{
			this->editorManager->deleteGameObjects(gameObjectIds);
			boost::shared_ptr<EventDataRefreshPropertiesPanel> eventDataRefreshPropertiesPanel(new EventDataRefreshPropertiesPanel());
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshPropertiesPanel);

			boost::shared_ptr<EventDataRefreshResourcesPanel> eventDataRefreshResourcesPanel(new EventDataRefreshResourcesPanel());
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshResourcesPanel);
		}
	}
}

void ResourcesPanelGameObjects::handleRefreshGameObjectsPanel(NOWA::EventDataPtr eventData)
{
	// this->refresh("");
	this->clear();
}

/////////////////////////////////////////////////////////////////////////////

ResourcesPanelDataBlocks::ResourcesPanelDataBlocks()
	: BasePanelViewItem("ResourcesPanel.layout"),
	editorManager(nullptr),
	dataBlocksTree(nullptr),
	datablockPreview(nullptr)
{
	
}

void ResourcesPanelDataBlocks::setEditorManager(NOWA::EditorManager* editorManager)
{
	this->editorManager = editorManager;
}

void ResourcesPanelDataBlocks::initialise()
{
	mPanelCell->setCaption("DataBlocks");
	mPanelCell->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());

	assignWidget(this->dataBlocksTree, "Tree");
	assignWidget(this->datablockPreview, "previewImage");
	this->dataBlocksTree->eventTreeNodeSelected += newDelegate(this, &ResourcesPanelDataBlocks::notifyTreeNodeSelected);
	this->dataBlocksTree->eventKeyButtonPressed += newDelegate(this, &ResourcesPanelDataBlocks::notifyKeyButtonPressed);
	this->dataBlocksTree->eventTreeNodeContextMenu += newDelegate(this, &ResourcesPanelDataBlocks::notifyTreeContextMenu);
	mWidgetClient->setSize(MyGUI::IntSize(mWidgetClient->getWidth(), 405));
	this->dataBlocksTree->setSize(MyGUI::IntSize(mWidgetClient->getWidth() - 8, 200));
	this->datablockPreview->setPosition(4, 446);
	this->datablockPreview->setSize(164, 180);

	assignWidget(this->resourcesSearchEdit, "ResourcesSearchEdit");
	this->resourcesSearchEdit->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);
	this->resourcesSearchEdit->setEditStatic(false);
	this->resourcesSearchEdit->setEditReadOnly(false);
	this->resourcesSearchEdit->eventEditTextChange += MyGUI::newDelegate(this, &ResourcesPanelDataBlocks::editTextChange);

	this->loadDataBlocks("");
}

void ResourcesPanelDataBlocks::shutdown()
{
	
}

void ResourcesPanelDataBlocks::editTextChange(MyGUI::Widget* sender)
{
	MyGUI::EditBox* editBox = static_cast<MyGUI::EditBox*>(sender);

	// Start a new search each time for resources that do match the search caption string
	this->autoCompleteSearch.reset();
	this->clear();

	this->loadDataBlocks(editBox->getOnlyText());
}

void ResourcesPanelDataBlocks::loadDataBlocks(const Ogre::String& filter)
{
	const std::array<Ogre::HlmsTypes, 7> searchHlms =
	{
		Ogre::HLMS_PBS, Ogre::HLMS_TOON, Ogre::HLMS_UNLIT, Ogre::HLMS_USER0,
		Ogre::HLMS_USER1, Ogre::HLMS_USER2, Ogre::HLMS_USER3
	};

	MyGUI::TreeControl::Node* root = this->dataBlocksTree->getRoot();
	root->removeAll();

	Ogre::Hlms* hlms = nullptr;
	// List for all hlms typs all data blocks
	for (auto searchHlmsIt = searchHlms.begin(); searchHlmsIt != searchHlms.end(); ++searchHlmsIt)
	{
		hlms = NOWA::Core::getSingletonPtr()->getOgreRoot()->getHlmsManager()->getHlms(*searchHlmsIt);
		if (nullptr != hlms)
		{
			MyGUI::TreeControl::Node* typeNode = new MyGUI::TreeControl::Node(hlms->getTypeNameStr(), "Data");
			for (auto& it = hlms->getDatablockMap().cbegin(); it != hlms->getDatablockMap().cend(); ++it)
			{
				if (true == filter.empty())
				{
					MyGUI::TreeControl::Node* dataBlockNode = new MyGUI::TreeControl::Node(it->second.name, "Data");
					typeNode->add(dataBlockNode);
				}
				else
				{
					// Add resource to the search
					this->autoCompleteSearch.addSearchText(it->second.name);
				}
			}
			if (true == filter.empty())
			{
				root->add(typeNode);
			}
		}
	}

	if (false == filter.empty())
	{
		MyGUI::TreeControl::Node* child = nullptr;
		// Get the matched results and add to tree
		auto& matchedResources = this->autoCompleteSearch.findMatchedItemWithInText(filter);

		for (size_t i = 0; i < matchedResources.getResults().size(); i++)
		{
			Ogre::String resourceName = matchedResources.getResults()[i].getMatchedItemText();
			child = new MyGUI::TreeControl::Node(resourceName, "Data");

			root->add(child);
		}
	}
}

void ResourcesPanelDataBlocks::clear(void)
{
	MyGUI::TreeControl::Node* root = this->dataBlocksTree->getRoot();
	root->removeAll();
	
	this->loadDataBlocks("");
}

void ResourcesPanelDataBlocks::notifyTreeNodeSelected(MyGUI::TreeControl* treeControl, MyGUI::TreeControl::Node* node)
{
	if (nullptr == node)
	{
		return;
	}

	this->selectedText = node->getText();

	Ogre::HlmsDatablock* datablock = NOWA::Core::getSingletonPtr()->getOgreRoot()->getHlmsManager()->getDatablock(this->selectedText);
	Ogre::HlmsPbsDatablock* pbsDataBlock = dynamic_cast<Ogre::HlmsPbsDatablock*>(datablock);
	if (nullptr != pbsDataBlock)
	{
		Ogre::TextureGpu* texture = pbsDataBlock->getTexture(Ogre::PbsTextureTypes::PBSM_DIFFUSE);
		if (nullptr != texture)
		{
			this->datablockPreview->setImageTexture(texture->getNameStr());
		}
	}
}

void ResourcesPanelDataBlocks::notifyKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char ch)
{
	if (GetAsyncKeyState(VK_LCONTROL) && key == MyGUI::KeyCode::C)
	{
		if (false == this->selectedText.empty())
		{
			NOWA::Core::getSingletonPtr()->copyTextToClipboard(this->selectedText);
		}
	}
}

void ResourcesPanelDataBlocks::notifyTreeContextMenu(MyGUI::TreeControl* treeControl, MyGUI::TreeControl::Node* node)
{
	if (nullptr == node)
	{
		return;
	}

	this->selectedText = node->getText();

	Ogre::HlmsDatablock* datablock = NOWA::Core::getSingletonPtr()->getOgreRoot()->getHlmsManager()->getDatablock(this->selectedText);
	Ogre::HlmsPbsDatablock* pbsDataBlock = dynamic_cast<Ogre::HlmsPbsDatablock*>(datablock);
	if (nullptr != pbsDataBlock)
	{
		const Ogre::String* fileName;
		const Ogre::String* resourceGroup;
		datablock->getFilenameAndResourceGroup(&fileName, &resourceGroup);

		if (false == (*fileName).empty())
		{
			 Ogre::String data = "File: '" + *fileName + "'\n Resource group name: '" + *resourceGroup + "'";
			 MyGUI::Message* messageBox = MyGUI::Message::createMessageBox("NOWA-Design", data,
				 MyGUI::MessageBoxStyle::IconInfo | MyGUI::MessageBoxStyle::Ok, "Popup", true);
		}

		
	}
}

/////////////////////////////////////////////////////////////////////////////

ResourcesPanelTextures::ResourcesPanelTextures()
	: BasePanelViewItem("ResourcesPanel.layout"),
	resourcesSearchEdit(nullptr),
	editorManager(nullptr),
	texturesTree(nullptr),
	texturePreview(nullptr)
{

}

void ResourcesPanelTextures::setEditorManager(NOWA::EditorManager* editorManager)
{
	this->editorManager = editorManager;
}

void ResourcesPanelTextures::initialise()
{
	mPanelCell->setCaption("Textures");
	mPanelCell->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());

	assignWidget(this->texturesTree, "Tree");
	assignWidget(this->texturePreview, "previewImage");
	this->texturesTree->eventTreeNodeSelected += newDelegate(this, &ResourcesPanelTextures::notifyTreeNodeSelected);
	this->texturesTree->eventKeyButtonPressed += newDelegate(this, &ResourcesPanelTextures::notifyKeyButtonPressed);
	this->texturesTree->eventTreeNodeContextMenu += newDelegate(this, &ResourcesPanelTextures::notifyTreeContextMenu);
	mWidgetClient->setSize(MyGUI::IntSize(mWidgetClient->getWidth(), 405));
	this->texturesTree->setSize(MyGUI::IntSize(mWidgetClient->getWidth() - 8, 200));
	this->texturePreview->setPosition(4, 228);
	this->texturePreview->setSize(164, 180);

	assignWidget(this->resourcesSearchEdit, "ResourcesSearchEdit");
	this->resourcesSearchEdit->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);
	this->resourcesSearchEdit->setEditStatic(false);
	this->resourcesSearchEdit->setEditReadOnly(false);
	this->resourcesSearchEdit->eventEditTextChange += MyGUI::newDelegate(this, &ResourcesPanelTextures::editTextChange);

	this->loadTextures("");
}

void ResourcesPanelTextures::shutdown()
{

}

void ResourcesPanelTextures::editTextChange(MyGUI::Widget* sender)
{
	MyGUI::EditBox* editBox = static_cast<MyGUI::EditBox*>(sender);

	// Start a new search each time for resources that do match the search caption string
	this->autoCompleteSearch.reset();
	this->clear();

	this->loadTextures(editBox->getOnlyText());
}

void ResourcesPanelTextures::loadTextures(const Ogre::String& filter)
{
	MyGUI::TreeControl::Node* root = this->texturesTree->getRoot();
	root->removeAll();

	// Note: dds cannot be displayed and application will hang
	std::vector<Ogre::String> filters({ "png", "jpg", "bmp", "tga", "gif", "tif" });
	std::set<Ogre::String> textureNames = NOWA::Core::getSingletonPtr()->getAllAvailableTextureNames(filters);

	for (auto& textureName : textureNames)
	{
		if (true == filter.empty())
		{
			MyGUI::TreeControl::Node* textureNode = new MyGUI::TreeControl::Node(textureName, "Data");
			root->add(textureNode);
		}
		else
		{
			// Add resource to the search
			this->autoCompleteSearch.addSearchText(textureName);
		}
		// MyGUI::Ogre2RenderManager::getInstancePtr()->createTexture(textureName);
	}

	if (false == filter.empty())
	{
		MyGUI::TreeControl::Node* child = nullptr;
		// Get the matched results and add to tree
		auto& matchedResources = this->autoCompleteSearch.findMatchedItemWithInText(filter);

		for (size_t i = 0; i < matchedResources.getResults().size(); i++)
		{
			Ogre::String resourceName = matchedResources.getResults()[i].getMatchedItemText();
			child = new MyGUI::TreeControl::Node(resourceName, "Data");

			root->add(child);
		}
	}
}

void ResourcesPanelTextures::clear(void)
{
	MyGUI::TreeControl::Node* root = this->texturesTree->getRoot();
	root->removeAll();

	this->loadTextures("");
}

void ResourcesPanelTextures::notifyTreeNodeSelected(MyGUI::TreeControl* treeControl, MyGUI::TreeControl::Node* node)
{
	if (nullptr == node)
	{
		return;
	}
	this->selectedText = node->getText();
	// Ogre::TexturePtr texturePtr = Ogre::TextureManager::getSingleton().getByName(this->selectedText);
	// if (nullptr != texturePtr)
	{
		// Does not work, because only textures of mygui are added somehow, and even create texture creates texture, but internal pointer is zero
		this->texturePreview->setImageTexture(this->selectedText);
	}
	// this->texturePreview->setItemResource(this->selectedText);
}

void ResourcesPanelTextures::notifyKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char ch)
{
	if (GetAsyncKeyState(VK_LCONTROL) && key == MyGUI::KeyCode::C)
	{
		if (false == this->selectedText.empty())
		{
			NOWA::Core::getSingletonPtr()->copyTextToClipboard(this->selectedText);
		}
	}
}

void ResourcesPanelTextures::notifyTreeContextMenu(MyGUI::TreeControl* treeControl, MyGUI::TreeControl::Node* node)
{
	if (nullptr == node)
	{
		return;
	}

	this->selectedText = node->getText();

	Ogre::String textureFilePathName = NOWA::Core::getSingletonPtr()->getResourceFilePathName(this->selectedText);

	if (false == textureFilePathName.empty())
	{
		Ogre::String data = "Location: '" + textureFilePathName + "'";
		MyGUI::Message* messageBox = MyGUI::Message::createMessageBox("NOWA-Design", data,
			MyGUI::MessageBoxStyle::IconInfo | MyGUI::MessageBoxStyle::Ok, "Popup", true);
	}
}


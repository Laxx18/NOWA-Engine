#include "NOWAPrecompiled.h"
#include "ResourcesPanel.h"
#include "TreeControl/TreeControl.h"
#include "TreeControl/TreeControlItem.h"
#include "main/EventManager.h"
#include "GuiEvents.h"
#include "MyGUIHelper.h"
#include "ProjectManager.h"

#include <filesystem>

ResourcesPanel::ResourcesPanel(const MyGUI::FloatCoord& coords)
	: BaseLayout("ResourcesPanelView.layout"),
	editorManager(nullptr),
	resourcesPanelView1(nullptr),
	resourcesPanelView2(nullptr),
	resourcesPanelView3(nullptr),
	resourcesPanelView4(nullptr),
	resourcesPanelMeshes(nullptr),
#if 0
	resourcesPanelMeshPreview(nullptr),
#endif
	resourcesPanelGameObjects(nullptr),
	resourcesPanelDataBlocks(nullptr),
	resourcesPanelTextures(nullptr),
	resourcesPanelProject(nullptr),
	resourcesPanelLuaScript(nullptr),
	resourcesPanelPlugins(nullptr)
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

	assignBase(this->resourcesPanelView3, "resources3ScrollView");
	this->resourcesPanelProject = new ResourcesPanelProject();
	this->resourcesPanelView3->addItem(this->resourcesPanelProject);

	this->resourcesPanelLuaScript = new ResourcesPanelLuaScript();
	this->resourcesPanelView3->addItem(this->resourcesPanelLuaScript);

	assignBase(this->resourcesPanelView4, "resources4ScrollView");
	this->resourcesPanelPlugins = new ResourcesPanelPlugins();
	this->resourcesPanelView4->addItem(this->resourcesPanelPlugins);
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
	this->resourcesPanelProject->setEditorManager(this->editorManager);
	this->resourcesPanelLuaScript->setEditorManager(this->editorManager);
	this->resourcesPanelPlugins->setEditorManager(this->editorManager);
}

void ResourcesPanel::setProjectManager(ProjectManager* projectManager)
{
	this->resourcesPanelProject->setProjectManager(projectManager);
	this->resourcesPanelLuaScript->setProjectManager(projectManager);
	this->resourcesPanelPlugins->setProjectManager(projectManager);
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

	this->resourcesPanelView3->removeAllItems();

	if (this->resourcesPanelProject)
	{
		delete this->resourcesPanelProject;
		this->resourcesPanelProject = nullptr;
	}

	if (this->resourcesPanelLuaScript)
	{
		delete this->resourcesPanelLuaScript;
		this->resourcesPanelLuaScript = nullptr;
	}

	this->resourcesPanelView4->removeAllItems();

	if (this->resourcesPanelPlugins)
	{
		delete this->resourcesPanelPlugins;
		this->resourcesPanelPlugins = nullptr;
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
	this->meshesTree->eventTreeNodePrepare -= newDelegate(this, &ResourcesPanelMeshes::notifyTreeNodePrepare);
	this->meshesTree->eventTreeNodeSelected -= newDelegate(this, &ResourcesPanelMeshes::notifyTreeNodeSelected);
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
	imageBox(nullptr),
	ctrlPressed(false),
	selectAll(false)
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
	this->gameObjectsTree->setSize(MyGUI::IntSize(mWidgetClient->getWidth() - 8, 400));
	// Not necessary, else game objects are filled two times -> performance
	// this->refresh();
	assignWidget(this->resourcesSearchEdit, "ResourcesSearchEdit");
	this->resourcesSearchEdit->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
	this->resourcesSearchEdit->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);
	this->resourcesSearchEdit->setEditStatic(false);
	this->resourcesSearchEdit->setEditReadOnly(false);
	this->resourcesSearchEdit->eventEditTextChange += MyGUI::newDelegate(this, &ResourcesPanelGameObjects::editTextChange);
	this->resourcesSearchEdit->eventMouseButtonClick += MyGUI::newDelegate(this, &ResourcesPanelGameObjects::onMouseClick);
	this->resourcesSearchEdit->setNeedToolTip(true);
	// Silly bug: edit: the one pixel border must be hit via mouse in order to see the tooltip
	this->resourcesSearchEdit->eventToolTip += MyGUI::newDelegate(MyGUIHelper::getInstance(), &MyGUIHelper::notifyToolTip);
	this->resourcesSearchEdit->eventMouseLostFocus += MyGUI::newDelegate(this, &ResourcesPanelGameObjects::mouseLostFocus);
	this->resourcesSearchEdit->eventRootMouseChangeFocus += MyGUI::newDelegate(this, &ResourcesPanelGameObjects::mouseRootChangeFocus);

	this->imageBox->setVisible(false);
}

void ResourcesPanelGameObjects::shutdown()
{
	this->resourcesSearchEdit->eventToolTip -= MyGUI::newDelegate(MyGUIHelper::getInstance(), &MyGUIHelper::notifyToolTip);
	this->resourcesSearchEdit->eventMouseLostFocus -= MyGUI::newDelegate(this, &ResourcesPanelGameObjects::mouseLostFocus);
	this->resourcesSearchEdit->eventRootMouseChangeFocus -= MyGUI::newDelegate(this, &ResourcesPanelGameObjects::mouseRootChangeFocus);
	this->resourcesSearchEdit->eventEditTextChange -= MyGUI::newDelegate(this, &ResourcesPanelGameObjects::editTextChange);
	this->resourcesSearchEdit->eventMouseButtonClick -= MyGUI::newDelegate(this, &ResourcesPanelGameObjects::onMouseClick);
	this->gameObjectsTree->eventTreeNodeSelected -= newDelegate(this, &ResourcesPanelGameObjects::notifyTreeNodeSelected);
	this->gameObjectsTree->eventKeyButtonPressed -= newDelegate(this, &ResourcesPanelGameObjects::keyButtonPressed);

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

	// If user is entering something, do not move camera, if the user entered something like asdf
	NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setMoveCameraWeight(0.0f);
	NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setRotateCameraWeight(0.0f);
}

void ResourcesPanelGameObjects::mouseLostFocus(MyGUI::Widget* sender, MyGUI::Widget* oldWidget)
{
	NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setMoveCameraWeight(1.0f);
	NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setRotateCameraWeight(1.0f);
}

void ResourcesPanelGameObjects::mouseRootChangeFocus(MyGUI::Widget* sender, bool bFocus)
{
	NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setMoveCameraWeight(1.0f);
	NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setRotateCameraWeight(1.0f);
}

void ResourcesPanelGameObjects::onMouseClick(MyGUI::Widget* sender)
{
	this->editTextChange(sender);
	NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setMoveCameraWeight(1.0f);
	NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setRotateCameraWeight(1.0f);
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
					// Adds resource to the search and its id for faster search
					this->autoCompleteSearch.addSearchText(gameObjectPtr->getName(), Ogre::StringConverter::toString(gameObjectPtr->getId()));
					// Adds also its components to the search
					for (auto it = gameObjectPtr->getComponents()->cbegin(); it != gameObjectPtr->getComponents()->cend(); ++it)
					{
						this->autoCompleteSearch.addSearchText(std::get<NOWA::COMPONENT>(*it)->getClassName());
					}
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
				// Adds resource to the search and its id for faster search
				this->autoCompleteSearch.addSearchText(gameObjectPtr->getName(), Ogre::StringConverter::toString(gameObjectPtr->getId()));

				// Adds also its components to the search
				for (auto it = gameObjectPtr->getComponents()->cbegin(); it != gameObjectPtr->getComponents()->cend(); ++it)
				{
					this->autoCompleteSearch.addSearchText(std::get<NOWA::COMPONENT>(*it)->getClassName());
				}
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

		std::set<Ogre::String> tempGameObjectSet;

		for (size_t i = 0; i < matchedResources.getResults().size(); i++)
		{
			Ogre::String resourceName = matchedResources.getResults()[i].getMatchedItemText();
			Ogre::String resourceId = matchedResources.getResults()[i].getUserData();

			// Found gameobject, by id, get it
			if (false == resourceId.empty())
			{
				// Do not allow duplicates later in mygui tree
				const auto foundIt = tempGameObjectSet.find(resourceName);
				if (tempGameObjectSet.cend() == foundIt)
				{
					child = new MyGUI::TreeControl::Node(resourceName, "Data");
					root->add(child);
					tempGameObjectSet.insert(resourceName);
				}
			}
			else
			{
				// Id empty, no direct game object, but component. Hence checks components and gets all game objects which do contain the component class name
				const auto gameObjects = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsFromComponent(resourceName);

				this->resourcesSearchEdit->setUserString("tooltip", resourceName);

				for (size_t j = 0; j < gameObjects.size(); j++)
				{
					// Do not allow duplicates later in mygui tree
					const auto foundIt = tempGameObjectSet.find(gameObjects[j]->getName());
					if (tempGameObjectSet.cend() == foundIt)
					{
						child = new MyGUI::TreeControl::Node(gameObjects[j]->getName(), "Data");
						root->add(child);
						tempGameObjectSet.insert(gameObjects[j]->getName());
					}
				}
			}
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

	if (false == NOWA::InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(OIS::KC_LSHIFT) && false == this->selectAll)
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

	if (key == MyGUI::KeyCode::LeftControl || key == MyGUI::KeyCode::RightControl)
	{
		this->ctrlPressed = !this->ctrlPressed;
	}

	if (this->ctrlPressed && key == MyGUI::KeyCode::A)
	{
		MyGUI::TreeControl* treeControl = static_cast<MyGUI::TreeControl*>(sender);
		this->selectAll = true;
		selectAllNodes(treeControl);
		this->selectAll = false;
	}
}

void ResourcesPanelGameObjects::handleRefreshGameObjectsPanel(NOWA::EventDataPtr eventData)
{
	// this->refresh("");
	this->clear();
}

void ResourcesPanelGameObjects::selectAllNodes(MyGUI::TreeControl* treeControl)
{
	auto rootNodes = treeControl->getRoot();
	for (const auto node : rootNodes->getChildren())
	{
		this->notifyTreeNodeSelected(treeControl, node);
		// Optionally, traverse child nodes if necessary
		std::stack<MyGUI::TreeControl::Node*> nodeStack;
		nodeStack.push(node);
		while (!nodeStack.empty())
		{
			MyGUI::TreeControl::Node* currentNode = nodeStack.top();
			nodeStack.pop();
			this->notifyTreeNodeSelected(treeControl, currentNode);
			auto childNodes = currentNode->getChildren();
			for (auto childNode : childNodes)
			{
				nodeStack.push(childNode);
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////

ResourcesPanelDataBlocks::ResourcesPanelDataBlocks()
	: BasePanelViewItem("ResourcesPanel.layout"),
	editorManager(nullptr),
	dataBlocksTree(nullptr),
	datablockPreview(nullptr),
	resourcesSearchEdit(nullptr)
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
	// this->datablockPreview->setPosition(4, 446);
	// this->datablockPreview->setSize(164, 180);

	this->datablockPreview->setPosition(4, 228);
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
	this->resourcesSearchEdit->eventEditTextChange -= MyGUI::newDelegate(this, &ResourcesPanelDataBlocks::editTextChange);
	this->dataBlocksTree->eventTreeNodeSelected -= newDelegate(this, &ResourcesPanelDataBlocks::notifyTreeNodeSelected);
	this->dataBlocksTree->eventKeyButtonPressed -= newDelegate(this, &ResourcesPanelDataBlocks::notifyKeyButtonPressed);
	this->dataBlocksTree->eventTreeNodeContextMenu -= newDelegate(this, &ResourcesPanelDataBlocks::notifyTreeContextMenu);
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
	this->resourcesSearchEdit->eventEditTextChange -= MyGUI::newDelegate(this, &ResourcesPanelTextures::editTextChange);
	this->texturesTree->eventTreeNodeSelected -= newDelegate(this, &ResourcesPanelTextures::notifyTreeNodeSelected);
	this->texturesTree->eventKeyButtonPressed -= newDelegate(this, &ResourcesPanelTextures::notifyKeyButtonPressed);
	this->texturesTree->eventTreeNodeContextMenu -= newDelegate(this, &ResourcesPanelTextures::notifyTreeContextMenu);
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

/////////////////////////////////////////////////////////////////////////////

ResourcesPanelProject::ResourcesPanelProject()
	: BasePanelViewItem("ResourcesPanelProject.layout"),
	filesTreeControl(nullptr),
	editorManager(nullptr),
	projectManager(nullptr),
	lastClickedNode(nullptr),
	hasSceneChanges(false)
{
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &ResourcesPanelProject::handleSceneModified), NOWA::EventDataSceneModified::getStaticEventType());
}

ResourcesPanelProject::~ResourcesPanelProject()
{
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ResourcesPanelProject::handleSceneModified), NOWA::EventDataSceneModified::getStaticEventType());
}

void ResourcesPanelProject::setEditorManager(NOWA::EditorManager* editorManager)
{
	this->editorManager = editorManager;
}

void ResourcesPanelProject::setProjectManager(ProjectManager* projectManager)
{
	this->projectManager = projectManager;
}

void ResourcesPanelProject::initialise(void)
{
	mPanelCell->setCaption("Project");
	mPanelCell->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());

	assignWidget(this->filesTreeControl, "Tree");
	mWidgetClient->setSize(MyGUI::IntSize(mWidgetClient->getWidth(), 405));

	this->filesTreeControl->setInheritsAlpha(false);

	// Event Handlers
	this->filesTreeControl->eventKeyButtonPressed += MyGUI::newDelegate(this, &ResourcesPanelProject::notifyKeyButtonPressed);
	this->filesTreeControl->eventTreeNodeSelected += MyGUI::newDelegate(this, &ResourcesPanelProject::notifyTreeNodeClick);
	this->filesTreeControl->eventTreeNodeActivated += MyGUI::newDelegate(this, &ResourcesPanelProject::notifyTreeNodeDoubleClick);
	// this->filesTreeControl->eventTreeNodeActivated

	this->populateFilesTree(NOWA::Core::getSingletonPtr()->getCurrentProjectPath());

	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &ResourcesPanelProject::handleEventDataResourceCreated), NOWA::EventDataResourceCreated::getStaticEventType());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &ResourcesPanelProject::handleEventDataGameObjectMadeGlobal), NOWA::EventDataGameObjectMadeGlobal::getStaticEventType());
}

void ResourcesPanelProject::shutdown(void)
{
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ResourcesPanelProject::handleEventDataResourceCreated), NOWA::EventDataResourceCreated::getStaticEventType());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ResourcesPanelProject::handleEventDataGameObjectMadeGlobal), NOWA::EventDataGameObjectMadeGlobal::getStaticEventType());

	this->filesTreeControl->eventKeyButtonPressed -= MyGUI::newDelegate(this, &ResourcesPanelProject::notifyKeyButtonPressed);
	this->filesTreeControl->eventTreeNodeSelected -= MyGUI::newDelegate(this, &ResourcesPanelProject::notifyTreeNodeClick);
	this->filesTreeControl->eventTreeNodeActivated -= MyGUI::newDelegate(this, &ResourcesPanelProject::notifyTreeNodeDoubleClick);
}

void ResourcesPanelProject::clear(void)
{
	this->selectedText.clear();
	MyGUI::TreeControl::Node* root = this->filesTreeControl->getRoot();
	root->removeAll();
}

void ResourcesPanelProject::populateFilesTree(const Ogre::String& folderPath)
{
	this->clear();

	MyGUI::TreeControl::Node* root = this->filesTreeControl->getRoot();

	for (const auto& entry : std::filesystem::directory_iterator(folderPath))
	{
		if (entry.is_directory())
		{
			MyGUI::TreeControl::Node* folderNode = new MyGUI::TreeControl::Node(entry.path().filename().string(), "Data");
			folderNode->setExpanded(true);
			root->add(folderNode);

			for (const auto& subEntry : std::filesystem::directory_iterator(entry.path()))
			{
				if (subEntry.is_regular_file())
				{
					MyGUI::TreeControl::Node* fileNode = new MyGUI::TreeControl::Node(subEntry.path().filename().string(), "Data");
					folderNode->add(fileNode);
				}
			}
		}
		else if (entry.is_regular_file())
		{
			MyGUI::TreeControl::Node* fileNode = new MyGUI::TreeControl::Node(entry.path().filename().string(), "Data");
			root->add(fileNode);
		}
	}

	this->sortTreeNodesByName(root);
}

void ResourcesPanelProject::notifyKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char ch)
{
	if (GetAsyncKeyState(VK_LCONTROL) && key == MyGUI::KeyCode::C)
	{
		if (false == this->selectedText.empty())
		{
			NOWA::Core::getSingletonPtr()->copyTextToClipboard(this->selectedText);
		}
	}
}

void ResourcesPanelProject::notifyTreeNodeClick(MyGUI::TreeControl* sender, MyGUI::TreeControl::Node* node)
{
	auto now = std::chrono::steady_clock::now();
	this->lastClickTime = now;
	this->lastClickedNode = node;

	this->handleSingleClick(node);
}

void ResourcesPanelProject::notifyTreeNodeDoubleClick(MyGUI::TreeControl* sender, MyGUI::TreeControl::Node* node)
{
	this->handleDoubleClick(node);
}

void ResourcesPanelProject::notifyMessageBoxEndLoad(MyGUI::Message* sender, MyGUI::MessageBoxStyle result)
{
	if (result == MyGUI::MessageBoxStyle::Yes)
	{
		if (nullptr != this->projectManager)
		{
			this->projectManager->saveProject();
		}
	}

	if (nullptr != this->projectManager)
	{
		Ogre::String sceneName = this->currentSceneName.substr(0, this->currentSceneName.size() - 6);

		boost::shared_ptr<EventDataSceneValid> eventDataSceneValid(new EventDataSceneValid(false));
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataSceneValid);
		this->projectManager->loadProject(NOWA::Core::getSingletonPtr()->getCurrentProjectPath() + "/" + sceneName + "/" + this->currentSceneName);
	}

	this->hasSceneChanges = false;
}

void ResourcesPanelProject::handleSingleClick(MyGUI::TreeControl::Node* node)
{
	if (nullptr == node)
	{
		return;
	}

	this->selectedText = node->getText();

	if (this->selectedText.size() >= 4 && this->selectedText.compare(this->selectedText.size() - 4, 4, ".lua") == 0)
	{
		auto gameObjects = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsFromComponent(NOWA::LuaScriptComponent::getStaticClassName());

		for (size_t i = 0; i < gameObjects.size(); i++)
		{
			const auto& gameObjectPtr = gameObjects[i];
			auto luaCompPtr = NOWA::makeStrongPtr(gameObjectPtr->getComponent<NOWA::LuaScriptComponent>());
			if (nullptr != luaCompPtr)
			{
				if (nullptr != luaCompPtr->getLuaScript())
				{
					if (luaCompPtr->getLuaScript()->getScriptName() == this->selectedText)
					{
						this->editorManager->getSelectionManager()->snapshotGameObjectSelection();
						this->editorManager->getSelectionManager()->clearSelection();
						this->editorManager->getSelectionManager()->select(gameObjectPtr->getId());
					}
				}
			}
		}
	}
}

void ResourcesPanelProject::handleDoubleClick(MyGUI::TreeControl::Node* node)
{
	this->currentSceneName.clear();

	if (nullptr == node)
	{
		return;
	}

	Ogre::String itemName = node->getText();

	if (itemName.size() >= 4 && itemName.compare(itemName.size() - 4, 4, ".lua") == 0)
	{
		auto gameObjects = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsFromComponent(NOWA::LuaScriptComponent::getStaticClassName());

		for (size_t i = 0; i < gameObjects.size(); i++)
		{
			const auto& gameObjectPtr = gameObjects[i];
			auto luaCompPtr = NOWA::makeStrongPtr(gameObjectPtr->getComponent<NOWA::LuaScriptComponent>());
			if (nullptr != luaCompPtr)
			{
				if (nullptr != luaCompPtr->getLuaScript())
				{
					if (luaCompPtr->getLuaScript()->getScriptName() == itemName)
					{
						NOWA::DeployResourceModule::getInstance()->openNOWALuaScriptEditor(NOWA::Core::getSingletonPtr()->getCurrentProjectPath() + "/" + NOWA::Core::getSingletonPtr()->getSceneName() + "/" + itemName);
					}
				}
			}
		}
	}
	else if (itemName.size() >= 6 && itemName.compare(itemName.size() - 6, 6, ".scene") == 0)
	{
		if (itemName != "global.scene")
		{
			Ogre::String sceneName = itemName.substr(0, itemName.size() - 6);

			if (sceneName != NOWA::Core::getSingletonPtr()->getSceneName())
			{
				this->currentSceneName = itemName;

				if (true == this->hasSceneChanges)
				{
					MyGUI::Message* messageBox = MyGUI::Message::createMessageBox("Project", MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{SceneModified}"),
						MyGUI::MessageBoxStyle::IconWarning | MyGUI::MessageBoxStyle::Yes | MyGUI::MessageBoxStyle::No, "Popup", true);

					messageBox->eventMessageBoxResult += MyGUI::newDelegate(this, &ResourcesPanelProject::notifyMessageBoxEndLoad);
				}
				else
				{
					boost::shared_ptr<EventDataSceneValid> eventDataSceneValid(new EventDataSceneValid(false));
					NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataSceneValid);
					this->projectManager->loadProject(NOWA::Core::getSingletonPtr()->getCurrentProjectPath() + "/" + sceneName + "/" + itemName);
					this->hasSceneChanges = false;
				}
			}
		}
	}
}

void ResourcesPanelProject::sortTreeNodesByName(MyGUI::TreeControl::Node* parentNode)
{
	MyGUI::TreeControl::Node* root = parentNode;
	auto nodes = root->getChildren();

	std::sort(nodes.begin(), nodes.end(), [](const MyGUI::TreeControl::Node* a, const MyGUI::TreeControl::Node* b) {
		return a->getText() < b->getText();
	});

	for (auto node : nodes)
	{
		this->sortTreeNodesByName(node);
	}
}

void ResourcesPanelProject::handleEventDataResourceCreated(NOWA::EventDataPtr eventData)
{
	NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(2.0f));
	// Shows for a specific amount of time
	auto ptrFunction = [this]()
		{
			this->populateFilesTree(NOWA::Core::getSingletonPtr()->getCurrentProjectPath());
		};
	NOWA::ProcessPtr closureProcess(new NOWA::ClosureProcess(ptrFunction));
	delayProcess->attachChild(closureProcess);
	NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
	
}

void ResourcesPanelProject::handleSceneModified(NOWA::EventDataPtr eventData)
{
	this->hasSceneChanges = true;
}

void ResourcesPanelProject::handleEventDataGameObjectMadeGlobal(NOWA::EventDataPtr eventData)
{
	this->hasSceneChanges = true;
	this->populateFilesTree(NOWA::Core::getSingletonPtr()->getCurrentProjectPath());
}

/////////////////////////////////////////////////////////////////////////////

ResourcesPanelLuaScript::ResourcesPanelLuaScript()
	: BasePanelViewItem("ResourcesPanelLuaScript.layout"),
	listBox(nullptr),
	upButton(nullptr),
	downButton(nullptr),
	editorManager(nullptr),
	projectManager(nullptr)
{

}

ResourcesPanelLuaScript::~ResourcesPanelLuaScript()
{

}

void ResourcesPanelLuaScript::setEditorManager(NOWA::EditorManager* editorManager)
{
	this->editorManager = editorManager;
}

void ResourcesPanelLuaScript::setProjectManager(ProjectManager* projectManager)
{
	this->projectManager = projectManager;
}

void ResourcesPanelLuaScript::initialise(void)
{
	mPanelCell->setCaption("LuaScript Management");
	mPanelCell->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());

	assignWidget(this->listBox, "ListBox");
	assignWidget(this->upButton, "UpButton");
	assignWidget(this->downButton, "DownButton");
	mWidgetClient->setSize(MyGUI::IntSize(mWidgetClient->getWidth(), 405));

	this->listBox->setInheritsAlpha(false);

	this->upButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ResourcesPanelLuaScript::buttonHit);
	this->downButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ResourcesPanelLuaScript::buttonHit);

	this->populateListBox();

	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &ResourcesPanelLuaScript::handleLuaScriptModified), NOWA::EventDataLuaScriptModfied::getStaticEventType());
}

void ResourcesPanelLuaScript::shutdown(void)
{
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ResourcesPanelLuaScript::handleLuaScriptModified), NOWA::EventDataLuaScriptModfied::getStaticEventType());
}

void ResourcesPanelLuaScript::clear(void)
{
	this->selectedText.clear();
	this->listBox->removeAllItems();
}

void ResourcesPanelLuaScript::populateListBox(void)
{
	this->clear();

	auto luaScripts = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getManagedLuaScripts();

	for (const auto& weakScript : luaScripts)
	{
		if (auto luaScriptComponent = NOWA::makeStrongPtr(weakScript))
		{
			Ogre::String identifier = "Id: " + Ogre::StringConverter::toString(luaScriptComponent->getOwner()->getId()) + " - " + luaScriptComponent->getScriptFile();
			this->listBox->addItem(identifier);
		}
	}
}

void ResourcesPanelLuaScript::buttonHit(MyGUI::Widget* sender)
{
	if (sender == this->upButton)
	{
		auto selectedIndex = this->listBox->getIndexSelected();
		if (selectedIndex > 0)
		{
			auto luaScriptComponent = NOWA::makeStrongPtr(NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getManagedLuaScripts()[selectedIndex]);
			if (nullptr != luaScriptComponent)
			{
				NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->moveScriptUp(luaScriptComponent);
				this->populateListBox();

				// After updating the list, set the selected index back
				// If the script moved up, the selected index should decrease by 1
				this->listBox->setItemSelect(selectedIndex - 1);
			}
		}
	}
	else if (sender == this->downButton)
	{
		auto selectedIndex = this->listBox->getIndexSelected();
		auto scripts = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getManagedLuaScripts();

		if (selectedIndex >= 0 && selectedIndex < scripts.size() - 1)
		{
			auto luaScriptComponent = NOWA::makeStrongPtr(scripts[selectedIndex]);
			if (nullptr != luaScriptComponent)
			{
				NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->moveScriptDown(luaScriptComponent);
				this->populateListBox();

				// After updating the list, set the selected index back
				// If the script moved down, the selected index should increase by 1
				this->listBox->setItemSelect(selectedIndex + 1);
			}
		}
	}
}

void ResourcesPanelLuaScript::handleLuaScriptModified(NOWA::EventDataPtr eventData)
{
	this->populateListBox();
}

/////////////////////////////////////////////////////////////////////////////

ResourcesPanelPlugins::ResourcesPanelPlugins()
	: BasePanelViewItem("ResourcesPanelPlugins.layout"),
	listBox(nullptr),
	infoTextBox(nullptr),
	buyButton(nullptr),
	editorManager(nullptr),
	projectManager(nullptr)
{

}

ResourcesPanelPlugins::~ResourcesPanelPlugins()
{

}

void ResourcesPanelPlugins::setEditorManager(NOWA::EditorManager* editorManager)
{
	this->editorManager = editorManager;
}

void ResourcesPanelPlugins::setProjectManager(ProjectManager* projectManager)
{
	this->projectManager = projectManager;
}

void ResourcesPanelPlugins::initialise(void)
{
	mPanelCell->setCaption("Plugins Management");
	mPanelCell->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());

	assignWidget(this->listBox, "ListBox");
	assignWidget(this->infoTextBox, "InfoLabel");
	assignWidget(this->buyButton, "BuyButton");

	this->infoTextBox->setEditMultiLine(true);
	this->infoTextBox->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);
	this->infoTextBox->setEditWordWrap(true);
	this->infoTextBox->setEditStatic(true);
	this->infoTextBox->setEditReadOnly(true);
	this->infoTextBox->showVScroll(true);
	this->infoTextBox->showHScroll(true);
	this->buyButton->setEnabled(false);
	
	mWidgetClient->setSize(MyGUI::IntSize(mWidgetClient->getWidth(), 805));

	this->listBox->setInheritsAlpha(false);

	this->listBox->eventListChangePosition += MyGUI::newDelegate(this, &ResourcesPanelPlugins::onListBoxItemSelected);

	this->populateListBox();
}

void ResourcesPanelPlugins::shutdown(void)
{
	this->listBox->eventListChangePosition -= MyGUI::newDelegate(this, &ResourcesPanelPlugins::onListBoxItemSelected);
}

void ResourcesPanelPlugins::clear(void)
{
	this->selectedText.clear();
	this->listBox->removeAllItems();
}

void ResourcesPanelPlugins::populateListBox(void)
{
	this->clear();

	// Get all available plugin names
	std::vector<Ogre::String> allPlugins = NOWA::Core::getSingletonPtr()->getAllPluginNames();
	std::vector<Ogre::String> availablePlugins = NOWA::Core::getSingletonPtr()->getAvailablePluginNames();

	// Iterate through registered components
	for (const auto& componentName : allPlugins)
	{
		bool isPluginAvailable = std::find(availablePlugins.begin(), availablePlugins.end(), componentName) != availablePlugins.end();

		// Add item to the list box
		this->listBox->addItem(componentName);

		// Get the index of the last added item
		size_t itemIndex = this->listBox->getItemCount() - 1;

		// If the plugin is not available, change the text color to red
		if (false == isPluginAvailable)
		{
			Ogre::String text = this->listBox->getItem(itemIndex) + " - (Not Available)";
			this->listBox->setItemNameAt(itemIndex, text);
		}
	}
}

void ResourcesPanelPlugins::onListBoxItemSelected(MyGUI::ListBox* sender, size_t index)
{
	if (index == MyGUI::ITEM_NONE)
	{
		this->selectedText.clear();
		this->infoTextBox->setCaption("");
		return;
	}

	Ogre::String fullText = this->listBox->getItemNameAt(index);
	size_t spacePos = fullText.find(" -");

	// Extract substring up to the first space
	if (spacePos != Ogre::String::npos)
	{
		// Since space found, the component is not available , see: " - (Not Available)");
		this->selectedText = fullText.substr(0, spacePos);
		this->buyButton->setEnabled(true);
	}
	else
	{
		this->selectedText = fullText; // No space found, use full text
		this->buyButton->setEnabled(false);
	}

	// Update infoTextBox
	Ogre::String infoText = NOWA::GameObjectFactory::getInstance()->getComponentFactory()->getComponentInfoText(this->selectedText);
	this->infoTextBox->setCaption(infoText);
}

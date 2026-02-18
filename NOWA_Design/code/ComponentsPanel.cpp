#include "NOWAPrecompiled.h"
#include "ComponentsPanel.h"
#include "main/EventManager.h"
#include "GuiEvents.h"
#include "MyGUIHelper.h"

ComponentsPanel::ComponentsPanel(const MyGUI::FloatCoord& coords)
	: BaseLayout("ComponentsPanelView.layout"),
	editorManager(nullptr),
	componentsPanelView(nullptr),
	componentsPanelSearch(nullptr),
	componentsPanelInfo(nullptr),
	componentsPanelDynamic(nullptr)
{
	this->mMainWidget->setRealCoord(coords);
	assignBase(this->componentsPanelView, "componentsScrollView");

	MyGUI::Window* window = mMainWidget->castType<MyGUI::Window>(false);
	if (window != nullptr)
		window->eventWindowButtonPressed += newDelegate(this, &ComponentsPanel::notifyWindowButtonPressed);

	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &ComponentsPanel::handleShowComponentsPanel), EventDataShowComponentsPanel::getStaticEventType());
}

void ComponentsPanel::setEditorManager(NOWA::EditorManager* editorManager)
{
	this->editorManager = editorManager;
}

void ComponentsPanel::destroyContent(void)
{
	// Threadsafe from the outside
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ComponentsPanel::handleShowComponentsPanel), EventDataShowComponentsPanel::getStaticEventType());
	this->componentsPanelView->removeAllItems();

	if (nullptr != this->componentsPanelSearch)
	{
		delete this->componentsPanelSearch;
		this->componentsPanelSearch = nullptr;
	}

	if (nullptr != this->componentsPanelInfo)
	{
		delete this->componentsPanelInfo;
		this->componentsPanelInfo = nullptr;
	}
	// Attention: Is this panel always destroyed and re-created?
	if (nullptr != this->componentsPanelDynamic)
	{
		delete this->componentsPanelDynamic;
		this->componentsPanelDynamic = nullptr;
	}
}

void ComponentsPanel::clearComponents(void)
{
	ENQUEUE_RENDER_COMMAND_WAIT("ComponentsPanel::clearComponents",
	{
		if (nullptr != this->componentsPanelDynamic)
		{
			this->componentsPanelDynamic->clear();
			this->componentsPanelView->removeAllItems();
		}
	});
}

void ComponentsPanel::setVisible(bool show)
{
	ENQUEUE_RENDER_COMMAND_MULTI("ComponentsPanel::setVisible", _1(show),
	{
		this->mMainWidget->setVisible(show);
	});
}

void ComponentsPanel::notifyWindowButtonPressed(MyGUI::Window* sender, const std::string& button)
{
	if (button == "close")
	{
		this->setVisible(false);
		this->clearComponents();
	}
}

void ComponentsPanel::handleShowComponentsPanel(NOWA::EventDataPtr eventData)
{
	boost::shared_ptr<EventDataShowComponentsPanel> castEventData = boost::static_pointer_cast<EventDataShowComponentsPanel>(eventData);
	this->showComponents(castEventData->getIndex());
}

void ComponentsPanel::showComponents(int index)
{
	this->clearComponents();

	ENQUEUE_RENDER_COMMAND_MULTI("ComponentsPanel::showComponents", _1(index),
	{
		// Show all components for selected game objects
		unsigned int i = 0;
		auto & selectedGameObjects = this->editorManager->getSelectionManager()->getSelectedGameObjects();
		unsigned int count = static_cast<unsigned int>(selectedGameObjects.size());
		std::vector<NOWA::GameObject*> gameObjects;
		if (count > 0)
		{
			gameObjects.resize(count);
			for (auto& it = selectedGameObjects.cbegin(); it != selectedGameObjects.cend(); ++it)
			{
				gameObjects[i] = it->second.gameObject;
				i++;
			}
		}

		// Create components panel for info
		if (nullptr != this->componentsPanelInfo)
		{
			delete this->componentsPanelInfo;
			this->componentsPanelInfo = nullptr;
		}
		this->componentsPanelInfo = new ComponentsPanelInfo();

		if (nullptr != this->componentsPanelDynamic)
		{
			delete this->componentsPanelDynamic;
			this->componentsPanelDynamic = nullptr;
		}

		// Create components panel for info
		if (nullptr != this->componentsPanelSearch)
		{
			delete this->componentsPanelSearch;
			this->componentsPanelSearch = nullptr;
		}
		// Create components panel
		this->componentsPanelDynamic = new ComponentsPanelDynamic(gameObjects, "", this);

		this->componentsPanelSearch = new ComponentsPanelSearch(componentsPanelDynamic);

		assert(this->editorManager != nullptr);

		this->componentsPanelDynamic->setEditorManager(this->editorManager);
		this->componentsPanelDynamic->setComponentsPanelInfo(this->componentsPanelInfo);

		// Add to the view
		this->componentsPanelView->addItem(componentsPanelSearch);
		this->componentsPanelView->addItem(componentsPanelDynamic);
		this->componentsPanelView->addItem(componentsPanelInfo);

		this->componentsPanelDynamic->setIndex(index);
		this->componentsPanelDynamic->showComponents();

		this->setVisible(true);
	});
}

/////////////////////////////////////////////////////////////////////////////

ComponentsPanelSearch::ComponentsPanelSearch(ComponentsPanelDynamic* componentsPanelDynamic)
	: BasePanelViewItem("ComponentsPanelSearch.layout"),
	componentsPanelDynamic(componentsPanelDynamic)
{

}

void ComponentsPanelSearch::initialise()
{
	mPanelCell->setCaption("");
	assignWidget(this->componentSearchEdit, "componentSearchEdit");
	this->componentSearchEdit->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);
	this->componentSearchEdit->setEditStatic(false);
	this->componentSearchEdit->setEditReadOnly(false);
	// Set focus for widget
	MyGUI::InputManager::getInstancePtr()->setKeyFocusWidget(this->componentSearchEdit);
	this->componentSearchEdit->eventEditTextChange += MyGUI::newDelegate(this, &ComponentsPanelSearch::editTextChange);
}

void ComponentsPanelSearch::shutdown()
{

}

void ComponentsPanelSearch::editTextChange(MyGUI::Widget* sender)
{
	MyGUI::EditBox* editBox = static_cast<MyGUI::EditBox*>(sender);

	// Start a new search each time for components that do match the search caption string
	this->componentsPanelDynamic->showComponents(editBox->getOnlyText());
}

/////////////////////////////////////////////////////////////////////////////

ComponentsPanelInfo::ComponentsPanelInfo()
	: BasePanelViewItem("ComponentsPanelInfo.layout"),
	componentsInfo(nullptr)
{

}

void ComponentsPanelInfo::setInfo(const Ogre::String& info)
{
	this->componentsInfo->setOnlyText(info);
}

void ComponentsPanelInfo::initialise()
{
	mPanelCell->setCaption("Info");
	mPanelCell->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
	assignWidget(this->componentsInfo, "componentsInfo");
	this->componentsInfo->setEditMultiLine(true);
	this->componentsInfo->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);
	this->componentsInfo->setEditWordWrap(true);
	this->componentsInfo->setEditStatic(true);
	this->componentsInfo->setEditReadOnly(true);
	this->componentsInfo->showVScroll(true);
	this->componentsInfo->showHScroll(true);
}

void ComponentsPanelInfo::shutdown()
{

}

///////////////////////////////////////////////////////////////////////////////

ComponentsPanelDynamic::ComponentsPanelDynamic(const std::vector<NOWA::GameObject*>& gameObjects, const Ogre::String& name, ComponentsPanel* parentPanel)
	: BasePanelViewItem(""),
	editorManager(nullptr),
	gameObjects(gameObjects),
	name(name),
	parentPanel(parentPanel),
	heightCurrent(0),
	componentsPanelInfo(nullptr),
	index(-1)
{

}

ComponentsPanelDynamic::~ComponentsPanelDynamic()
{

}

void ComponentsPanelDynamic::setEditorManager(NOWA::EditorManager* editorManager)
{
	this->editorManager = editorManager;
}

void ComponentsPanelDynamic::setComponentsPanelInfo(ComponentsPanelInfo* componentsPanelInfo)
{
	this->componentsPanelInfo = componentsPanelInfo;
}

void ComponentsPanelDynamic::setIndex(int index)
{
	this->index = index;
}

void ComponentsPanelDynamic::initialise()
{
	mPanelCell->setCaption(this->name);
}

void ComponentsPanelDynamic::shutdown()
{
	// Threadsafe from the outside
	this->componentsButtons.clear();
	this->parentPanel = nullptr;
}

void ComponentsPanelDynamic::clear(void)
{
	this->heightCurrent = 0;
	MyGUI::WidgetManager::getInstance().destroyWidgets(this->componentsButtons);
	this->componentsButtons.clear();
}

void ComponentsPanelDynamic::showComponents(const Ogre::String& searchText)
{
	if (true == this->gameObjects.empty())
		return;

	ENQUEUE_RENDER_COMMAND_MULTI("ComponentsPanelDynamic::showComponents", _1(searchText),
	{
		const int height = 24;
		const int heightStep = 26;
		const int widthStep = 3;

		const int valueLeft = 4;
		const int valueWidth = static_cast<int>(mWidgetClient->getWidth() - 0.1f);

		this->autoCompleteSearch.reset();
		this->clear();

		// Add all registered comopnents to the search
		for (auto componentInfo : NOWA::GameObjectFactory::getInstance()->getComponentFactory()->getRegisteredComponentNames())
		{
			this->autoCompleteSearch.addSearchText(componentInfo.first);
		}

		// Get the matched results
		auto& matchedComponents = this->autoCompleteSearch.findMatchedItemWithInText(searchText);

		for (size_t i = 0; i < matchedComponents.getResults().size(); i++)
		{
			this->heightCurrent += heightStep;

			Ogre::String componentName = matchedComponents.getResults()[i].getMatchedItemText();
			Ogre::String tempComponentName = componentName;

			// If its a kind of physics component, act as parent, so that if the game object has an physics active component, e.g. a phyiscs artifact component may not appear in the list
			size_t foundPhysicsComponent = componentName.find("Physics");
			if (Ogre::String::npos != foundPhysicsComponent)
			{
				tempComponentName = NOWA::PhysicsComponent::getStaticClassName();
			}

			size_t foundJointComponent = componentName.find("Joint");
			size_t foundPlayerControllerComponent = componentName.find("PlayerController");
			size_t foundCameraBehaviorComponent = componentName.find("CameraBehavior");
			size_t foundCameraComponent = componentName.find("CameraComponent");
			size_t foundMyGUIComponent = componentName.find("MyGUI");
			size_t foundCompositorEffectComponent = componentName.find("CompositorEffect");
			size_t foundWorkspaceComponent = componentName.find("Workspace");

			if (Ogre::String::npos != foundWorkspaceComponent)
			{
				tempComponentName = NOWA::WorkspaceBaseComponent::getStaticClassName();
			}

			bool validToEnable = false;

			// If the user selected several game objects, list only the components which are valid for all selected game objects!
			for (size_t i = 0; i < this->gameObjects.size(); i++)
			{
				validToEnable = NOWA::GameObjectFactory::getInstance()->getComponentFactory()->canAddComponent(NOWA::getIdFromName(componentName), this->gameObjects[i]);

				// Here check if there is already those component, and if its an animation-, or particle universe- or simple sound-component, its possible to create severals, else do not show those component
				NOWA::GameObjectCompPtr gameObjectComponentPtr = NOWA::makeStrongPtr(this->gameObjects[i]->getComponent<NOWA::GameObjectComponent>(componentName));

				// MyGUIItemBoxComponent: A player may have an inventory, or a shop, or another trader
				if (NOWA::PhysicsMaterialComponent::getStaticClassName() == componentName)
				{
					// Physics material only possible to add to main game object, because special treatment, it will be connected a last, to have all components connect prior
					// to get valid data
					// if (1111111111L == this->gameObjects[i]->getId())
					validToEnable = true;
				}
				else if (NOWA::TagPointComponent::getStaticClassName() == tempComponentName)
				{
					Ogre::v1::Entity* entity = this->gameObjects[i]->getMovableObject<Ogre::v1::Entity>();
					if (nullptr != entity)
					{
						// Only show TagPointComponent if there is a skeleton and bones
						Ogre::v1::Skeleton* skeleton = entity->getSkeleton();
						if (nullptr != skeleton)
						{
							unsigned short numBones = skeleton->getNumBones();
							if (numBones > 0)
								validToEnable = true;
						}
					}
					else
					{
						Ogre::Item* item = this->gameObjects[i]->getMovableObject<Ogre::Item>();
						if (nullptr != item)
						{
							// Only show TagPointComponent if there is a skeleton and bones
							Ogre::SkeletonInstance* skeletonInstance = item->getSkeletonInstance();
							if (nullptr != skeletonInstance)
							{
								unsigned short numBones = skeletonInstance->getNumBones();
								if (numBones > 0)
									validToEnable = true;
							}
						}
					}
				}
				else if (NOWA::NavMeshComponent::getStaticClassName() == tempComponentName)
				{
					if (nullptr != NOWA::AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreRecast())
						validToEnable = true;
				}
				else if (NOWA::NavMeshTerraComponent::getStaticClassName() == tempComponentName)
				{
					if (nullptr != NOWA::AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreRecast() && nullptr != this->gameObjects[i]->getMovableObject<Ogre::Terra>())
						validToEnable = true;
				}
				// Can only be added once
				else if (NOWA::LuaScriptComponent::getStaticClassName() == tempComponentName)
				{
					NOWA::LuaScriptCompPtr luaScriptCompPtr = NOWA::makeStrongPtr(this->gameObjects[i]->getComponent<NOWA::LuaScriptComponent>());
					if (nullptr != luaScriptCompPtr)
					{
						validToEnable = false;
					}
					else
					{
						validToEnable = true;
					}
				}
				else if (Ogre::String::npos != foundJointComponent)
				{
					NOWA::PhysicsCompoundConnectionCompPtr physicsCompoundConnectionCompPtr = NOWA::makeStrongPtr(this->gameObjects[i]->getComponent<NOWA::PhysicsCompoundConnectionComponent>());
					if (nullptr != physicsCompoundConnectionCompPtr)
					{
						// Only root of compound has a body an can add a joint
						if (0 == physicsCompoundConnectionCompPtr->getRootId())
						{
							validToEnable = true;
						}
					}
					else
					{
						// If its a kind of joint component, a physics component is required (but not a kinematic one!
						NOWA::PhysicsCompPtr physicsCompPtr = NOWA::makeStrongPtr(this->gameObjects[i]->getComponent<NOWA::PhysicsComponent>());
						if (nullptr != physicsCompPtr && (nullptr == boost::dynamic_pointer_cast<NOWA::PhysicsActiveKinematicComponent>(physicsCompPtr) ||
							(nullptr != boost::dynamic_pointer_cast<NOWA::PhysicsActiveKinematicComponent>(physicsCompPtr) && NOWA::JointKinematicComponent::getStaticClassName() == componentName)))
						{
							validToEnable = true;
						}
					}
				}
				else if (NOWA::PhysicsTerrainComponent::getStaticClassName() == componentName) // Attention here componentName, because tempComponentName delivers parent PhysicsComponent, instead of sub type
				{
					auto& terraCompPtr = NOWA::makeStrongPtr(this->gameObjects[i]->getComponent<NOWA::TerraComponent>());
					if (nullptr != terraCompPtr)
					{
						auto& physicsTerrainCompPtr = NOWA::makeStrongPtr(this->gameObjects[i]->getComponent<NOWA::PhysicsTerrainComponent>());
						if (nullptr == physicsTerrainCompPtr)
							validToEnable = true;
					}
				}
				else if (NOWA::PhysicsArtifactComponent::getStaticClassName() == componentName)
				{
					auto& aiLuaCompPtr = NOWA::makeStrongPtr(this->gameObjects[i]->getComponent<NOWA::AiLuaComponent>());
					if (nullptr != aiLuaCompPtr)
					{
						validToEnable = false;
					}
				}
				// Kinematic may not have any joint besides kinematic controller
				else if (NOWA::PhysicsActiveKinematicComponent::getStaticClassName() == componentName)
				{
					auto& jointCompPtr = NOWA::makeStrongPtr(this->gameObjects[i]->getComponent<NOWA::JointComponent>());
					if (nullptr != jointCompPtr && nullptr == boost::dynamic_pointer_cast<NOWA::JointKinematicComponent>(jointCompPtr))
					{
						validToEnable = false;
					}
				}
				else if (0 == foundPlayerControllerComponent)
				{
					// If its a kind of player controller component, a physics component is required, an entity and a skeleton
					NOWA::PhysicsCompPtr physicsCompPtr = NOWA::makeStrongPtr(this->gameObjects[i]->getComponent<NOWA::PhysicsComponent>());
					// NOWA::AnimationCompPtr animationCompPtr = NOWA::makeStrongPtr(this->gameObjects[i]->getComponent<NOWA::AnimationComponent>());

					Ogre::v1::Entity* entity = this->gameObjects[i]->getMovableObject<Ogre::v1::Entity>();
					if (nullptr != entity)
					{
						Ogre::v1::OldSkeletonInstance* skeleton = entity->getSkeleton();

						if (nullptr != physicsCompPtr && nullptr != entity && nullptr != skeleton)
							validToEnable = true;
					}
				}
				else if (Ogre::String::npos != foundCameraBehaviorComponent)
				{
					validToEnable = true;
					/*NOWA::PlayerControllerCompPtr playerControllerCompPtr = NOWA::makeStrongPtr(this->gameObjects[i]->getComponent<NOWA::PlayerControllerComponent>());
					if (nullptr != playerControllerCompPtr)
						validToEnable = true;*/
				}
				else if (Ogre::String::npos != foundMyGUIComponent)
				{
					validToEnable = true;
				}
				else if (Ogre::String::npos != foundCompositorEffectComponent)
				{
					// NOWA::CameraCompPtr cameraCompPtr = NOWA::makeStrongPtr(this->gameObjects[i]->getComponent<NOWA::CameraComponent>());
					// if (nullptr != cameraCompPtr)
					{
						validToEnable = true;
					}
				}
				// No constraints to this components, may be added as often as needed
				else if ((NOWA::SimpleSoundComponent::getStaticClassName() == tempComponentName)
					|| (NOWA::SpawnComponent::getStaticClassName() == tempComponentName))
				{
					validToEnable = true;
				}
				else if (NOWA::WorkspaceBaseComponent::getStaticClassName() == tempComponentName)
				{
					validToEnable = false;
					NOWA::CameraCompPtr cameraCompPtr = NOWA::makeStrongPtr(this->gameObjects[i]->getComponent<NOWA::CameraComponent>());
					NOWA::WorkspaceBaseCompPtr workspaceCompPtr = NOWA::makeStrongPtr(this->gameObjects[i]->getComponent<NOWA::WorkspaceBaseComponent>());
					if (nullptr != cameraCompPtr && nullptr == workspaceCompPtr)
					{
						validToEnable = true;
					}
				}
				else if (NOWA::BackgroundScrollComponent::getStaticClassName() == componentName)
				{
					NOWA::WorkspaceBackgroundCompPtr workspaceBackgroundCompPtr = NOWA::makeStrongPtr(this->gameObjects[i]->getComponent<NOWA::WorkspaceBackgroundComponent>());
					if (nullptr != workspaceBackgroundCompPtr)
					{
						validToEnable = true;
					}
				}
				else if (NOWA::DatablockPbsComponent::getStaticClassName() == tempComponentName)
				{
					validToEnable = true;
				}
				else if (/*NOWA::DatablockTerraComponent::getStaticClassName() == tempComponentName || */NOWA::PhysicsTerrainComponent::getStaticClassName() == componentName)
				{
					auto& terraCompPtr = NOWA::makeStrongPtr(this->gameObjects[i]->getComponent<NOWA::TerraComponent>(tempComponentName));
					if (nullptr != terraCompPtr)
						validToEnable = true;
				}
				else if (NOWA::LineMeshScaleComponent::getStaticClassName() == componentName)
				{
					NOWA::PhysicsCompPtr physicsCompPtr = NOWA::makeStrongPtr(this->gameObjects[i]->getComponent<NOWA::PhysicsComponent>());
					// No physics component
					if (nullptr == physicsCompPtr)
						validToEnable = true;
				}
				else
				{
					NOWA::NodeCompPtr nodeComponent = NOWA::makeStrongPtr(this->gameObjects[i]->getComponent<NOWA::NodeComponent>(tempComponentName));
					if (nullptr != nodeComponent)
						validToEnable = true;
				}
			}

			MyGUI::Button* componentButton = mPanelCell->getMainWidget()->createWidget<MyGUI::Button>("Button", MyGUI::IntCoord(valueLeft, heightCurrent, valueWidth, height), MyGUI::Align::HStretch | MyGUI::Align::Top, componentName + "Button");
			componentButton->setCaption(componentName);
			// componentButton->setEnabled(validToEnable);


			// Damn it, how to show tool tip on disabled widget??
			componentButton->setNeedToolTip(true);
			componentButton->setUserString("tooltip", NOWA::GameObjectFactory::getInstance()->getComponentFactory()->getComponentInfoText(componentName));
			componentButton->eventToolTip += MyGUI::newDelegate(MyGUIHelper::getInstance(), &MyGUIHelper::notifyToolTip);

			// Button may not be disabled, because else tooltips will not work, so a hack is, to change the button style
			if (false == validToEnable)
			{
				componentButton->setColour(MyGUI::Colour(0.18f, 0.18f, 0.18f));
				componentButton->setTextColour(MyGUI::Colour(0.7f, 0.7f, 0.7f));
			}
			else
			{
				componentButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ComponentsPanelDynamic::buttonHit);
			}

			// componentButton->setUserString("Description", "Show component");
			this->componentsButtons.push_back(componentButton);

			mPanelCell->setClientHeight(heightCurrent, false);
		}
	});
}

void ComponentsPanelDynamic::buttonHit(MyGUI::Widget* sender)
{
	MyGUI::Button* button = sender->castType<MyGUI::Button>();
	if (nullptr != button)
	{
		Ogre::String componentName = button->getCaption();

		// This should be handled via editormanager due to undo/redo functionality
		// GameObjectPtr is required, so get it from game object. Note: Only game object was available because of selection result, which never may be shared_ptr! because of reference count mess when
		// Ogre would hold those shared ptrs in user any

		std::vector<unsigned long> gameObjectIds(this->gameObjects.size());
		size_t i = 0;
		// Do not delete directly via selection manager, because when internally deleted, an event is sent out to selection manager to remove from map
		for (size_t i = 0; i < this->gameObjects.size(); i++)
		{
			gameObjectIds[i] = this->gameObjects[i]->getId();
		}
		if (0 == gameObjectIds.size())
		{
			return;
		}

		this->editorManager->addComponent(gameObjectIds, componentName, true);
		if (this->index != -1)
		{
			this->gameObjects[0]->moveComponent(this->index + 1);
		}

		ENQUEUE_RENDER_COMMAND_MULTI_WAIT("ComponentsPanelDynamic::buttonHit", _1(button),
		{
			this->parentPanel->setVisible(false);
			this->clear();
		});

		// Sent when a component has been added, so that the properties panel can be refreshed with new values
		boost::shared_ptr<NOWA::EventDataRefreshGui> eventDataRefreshPropertiesPanel(new NOWA::EventDataRefreshGui());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshPropertiesPanel);
	}
}
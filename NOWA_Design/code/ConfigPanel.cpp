#include "NOWAPrecompiled.h"
#include "ConfigPanel.h"
#include "main/EventManager.h"
#include "ProjectManager.h"
#include "GuiEvents.h"
#include "MyGUIHelper.h"
#include "modules/WorkspaceModule.h"

namespace
{
	MyGUI::TextBox* findWidgetBySuffix(MyGUI::Widget* mWidgetClient, const std::string& suffix)
	{
		if (!mWidgetClient || mWidgetClient->getChildCount() == 0) return nullptr;

		MyGUI::Widget* firstChild = mWidgetClient->getChildAt(0); // Get the first child
		if (!firstChild) return nullptr;

		MyGUI::EnumeratorWidgetPtr iter = firstChild->getEnumerator();
		while (iter.next()) // Iterate over children of firstChild
		{
			MyGUI::Widget* widget = iter.current();
			if (widget && widget->getName().size() >= suffix.size() &&
				widget->getName().compare(widget->getName().size() - suffix.size(), suffix.size(), suffix) == 0)
			{
				return dynamic_cast<MyGUI::TextBox*>(widget);
			}
		}
		return nullptr;
	}
}

ConfigPanel::ConfigPanel(ProjectManager* projectManager, const MyGUI::FloatCoord& coords)
	: BaseLayout("ConfigPanelView.layout"),
	projectManager(projectManager),
	configPanelView(nullptr),
	configPanelProject(nullptr),
	configPanelSceneManager(nullptr),
	configPanelOgreNewt(nullptr),
	configPanelRecast(nullptr),
	okButton(nullptr),
	abordButton(nullptr),
	forSettings(false)
{
	this->mMainWidget->setRealCoord(coords);
	assignBase(this->configPanelView, "scrollView");
	
	this->configPanelProject = new ConfigPanelProject(this);
	this->configPanelView->addItem(this->configPanelProject);

	this->configPanelSceneManager = new ConfigPanelSceneManager(this);
	this->configPanelView->addItem(this->configPanelSceneManager);

	this->configPanelOgreNewt = new ConfigPanelOgreNewt();
	this->configPanelView->addItem(this->configPanelOgreNewt);

	this->configPanelRecast = new ConfigPanelRecast();
	this->configPanelView->addItem(this->configPanelRecast);
	this->mMainWidget->castType<MyGUI::Window>()->eventWindowButtonPressed += MyGUI::newDelegate(this, &ConfigPanel::notifyWindowPressed);

	this->okButton = this->mMainWidget->createWidget<MyGUI::Button>("Button", this->mMainWidget->getAbsoluteCoord().left + 10, this->mMainWidget->getAbsoluteCoord().height - 80,
		96, 24, MyGUI::Align::Left | MyGUI::Align::Top, "okButton");
	this->okButton->setCaption("Ok");
	this->okButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ConfigPanel::buttonHit);

	this->abordButton = this->mMainWidget->createWidget<MyGUI::Button>("Button", this->mMainWidget->getAbsoluteCoord().right() - 116, this->mMainWidget->getAbsoluteCoord().height - 80,
		96, 24, MyGUI::Align::Left | MyGUI::Align::Top, "abordButton");
	this->abordButton->setCaptionWithReplacing("#{Abord}");
	this->abordButton->eventMouseButtonClick += MyGUI::newDelegate(this, &ConfigPanel::buttonHit);
}

void ConfigPanel::destroyContent(void)
{
	// No delete of e.g. configPanelProject because its done internally in removeAllItems
	this->configPanelView->removeAllItems();
}

void ConfigPanel::setVisible(bool show)
{
	this->mMainWidget->setVisible(show);
	// Set main window as modal
	if (true == show)
	{
		MyGUI::InputManager::getInstancePtr()->addWidgetModal(this->mMainWidget);
		// New project
		if (false == this->forSettings)
		{
			this->configPanelProject->resetSettings();
		}
		else
		{
			this->applySettings();
		}
	}
	else
	{
		MyGUI::InputManager::getInstancePtr()->removeWidgetModal(this->mMainWidget);
	}
}

MyGUI::Widget* ConfigPanel::getMainWidget(void) const
{
	return this->mMainWidget;
}

void ConfigPanel::notifyWindowPressed(MyGUI::Window* widget, const std::string& name)
{
	// See http://www.ogre3d.org/tikiwiki/tiki-index.php?page=MyGUI%20addButtonInWindowSkin
	if ("close" == name)
	{
		this->setVisible(false);
	}
}

void ConfigPanel::buttonHit(MyGUI::Widget* sender)
{
	if (this->abordButton == sender)
	{
		if (true == this->forSettings)
		{
			boost::shared_ptr<EventDataSceneValid> eventDataSceneValid(new EventDataSceneValid(true));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataSceneValid);
		}
	}
	else if (this->okButton == sender)
	{
		NOWA::ProjectParameter projectParameter;
		
		projectParameter.projectName = std::get<0>(this->configPanelProject->getParameter());
		if (true == projectParameter.projectName.empty())
		{
			projectParameter.projectName = "Project1";
		}
		
		projectParameter.sceneName = std::get<1>(this->configPanelProject->getParameter());
		if (true == projectParameter.sceneName.empty())
		{
			projectParameter.sceneName = "Scene1";
		}
		projectParameter.createProject = std::get<2>(this->configPanelProject->getParameter());
		projectParameter.openProject = std::get<3>(this->configPanelProject->getParameter());
		projectParameter.createSceneInOwnState = std::get<4>(this->configPanelProject->getParameter());
		
		int key = std::get<5>(this->configPanelProject->getParameter());
		NOWA::Core::getSingletonPtr()->setCryptKey(key);

		projectParameter.ignoreGlobalScene = std::get<6>(this->configPanelProject->getParameter());
		projectParameter.useV2Item = std::get<7>(this->configPanelProject->getParameter());
		NOWA::Core::getSingletonPtr()->setUseEntityType(!projectParameter.useV2Item);
		
		projectParameter.ambientLightUpperHemisphere = std::get<0>(this->configPanelSceneManager->getParameter());
		projectParameter.ambientLightLowerHemisphere = std::get<1>(this->configPanelSceneManager->getParameter());
		projectParameter.shadowFarDistance = std::get<2>(this->configPanelSceneManager->getParameter());
		projectParameter.shadowDirectionalLightExtrusionDistance = std::get<3>(this->configPanelSceneManager->getParameter());
		projectParameter.shadowDirLightTextureOffset = std::get<4>(this->configPanelSceneManager->getParameter());
		projectParameter.shadowColor = std::get<5>(this->configPanelSceneManager->getParameter());
		projectParameter.shadowQualityIndex = std::get<6>(this->configPanelSceneManager->getParameter());
		projectParameter.ambientLightModeIndex = std::get<7>(this->configPanelSceneManager->getParameter());
		projectParameter.forwardMode = std::get<8>(this->configPanelSceneManager->getParameter());
		projectParameter.lightWidth = std::get<9>(this->configPanelSceneManager->getParameter());
		projectParameter.lightHeight = std::get<10>(this->configPanelSceneManager->getParameter());
		projectParameter.numLightSlices = std::get<11>(this->configPanelSceneManager->getParameter());
		projectParameter.lightsPerCell = std::get<12>(this->configPanelSceneManager->getParameter());
		projectParameter.minLightDistance = std::get<13>(this->configPanelSceneManager->getParameter());
		projectParameter.maxLightDistance = std::get<14>(this->configPanelSceneManager->getParameter());
		projectParameter.renderDistance = std::get<15>(this->configPanelSceneManager->getParameter());

		projectParameter.physicsUpdateRate = std::get<0>(this->configPanelOgreNewt->getParameter());
		projectParameter.solverModel = std::get<1>(this->configPanelOgreNewt->getParameter());
		projectParameter.solverForSingleIsland = std::get<2>(this->configPanelOgreNewt->getParameter());
		projectParameter.broadPhaseAlgorithm = std::get<3>(this->configPanelOgreNewt->getParameter());
		projectParameter.physicsThreadCount = std::get<4>(this->configPanelOgreNewt->getParameter());
		projectParameter.linearDamping = std::get<5>(this->configPanelOgreNewt->getParameter());
		projectParameter.angularDamping = std::get<6>(this->configPanelOgreNewt->getParameter());
		projectParameter.gravity = std::get<7>(this->configPanelOgreNewt->getParameter());

		projectParameter.hasRecast = std::get<0>(this->configPanelRecast->getParameter());
		projectParameter.cellSize = std::get<1>(this->configPanelRecast->getParameter());
		projectParameter.cellHeight = std::get<2>(this->configPanelRecast->getParameter());
		projectParameter.agentMaxSlope = std::get<3>(this->configPanelRecast->getParameter());
		projectParameter.agentMaxClimb = std::get<4>(this->configPanelRecast->getParameter());
		projectParameter.agentHeight = std::get<5>(this->configPanelRecast->getParameter());
		projectParameter.agentRadius = std::get<6>(this->configPanelRecast->getParameter());
		projectParameter.edgeMaxLen = std::get<7>(this->configPanelRecast->getParameter());
		projectParameter.edgeMaxError = std::get<8>(this->configPanelRecast->getParameter());
		projectParameter.regionMinSize = std::get<9>(this->configPanelRecast->getParameter());
		projectParameter.regionMergeSize = std::get<10>(this->configPanelRecast->getParameter());
		projectParameter.vertsPerPoly = std::get<11>(this->configPanelRecast->getParameter());
		projectParameter.detailSampleDist = std::get<12>(this->configPanelRecast->getParameter());
		projectParameter.detailSampleMaxError = std::get<13>(this->configPanelRecast->getParameter());
		projectParameter.pointExtends = std::get<14>(this->configPanelRecast->getParameter());
		projectParameter.keepInterResults = std::get<15>(this->configPanelRecast->getParameter());

		this->projectManager->setProjectParameter(projectParameter);

		// Check if C++ project does exist, if not, create one
		if (true == projectParameter.createProject)
		{
			NOWA::DeployResourceModule::getInstance()->createCPlusPlusProject(projectParameter.projectName, projectParameter.sceneName);
		}

		if (true == projectParameter.createSceneInOwnState)
		{
			NOWA::DeployResourceModule::getInstance()->createSceneInOwnState(projectParameter.projectName, projectParameter.sceneName);
		}

		if (true == projectParameter.openProject)
		{
			NOWA::DeployResourceModule::getInstance()->openProject(projectParameter.projectName);
		}
		
		// Only create new project, if this panel has been called for a new scene, when just called for settings, set the settings just
		if (false == this->forSettings)
		{
			if (false == this->checkProjectExists(projectParameter.projectName, projectParameter.sceneName))
			{
				this->projectManager->createNewProject(projectParameter);
			}
		}
		else
		{
			this->projectManager->applySettings(projectParameter);
		}

		boost::shared_ptr<EventDataProjectManipulation> eventDataProjectManipulation(new EventDataProjectManipulation(ProjectManager::eProjectMode::LOAD));
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataProjectManipulation);
	}
	this->setVisible(false);
}

void ConfigPanel::callForSettings(bool forSettings)
{
	this->forSettings = forSettings;
}

void ConfigPanel::applySettings(void)
{
	// Get the project parameter from project manager when a project has been loaded
	NOWA::ProjectParameter& projectParameter = this->projectManager->getProjectParameter();

	this->configPanelProject->setParameter(projectParameter.projectName, projectParameter.sceneName, projectParameter.createProject, projectParameter.openProject, 
		projectParameter.createSceneInOwnState, NOWA::Core::getSingletonPtr()->getCryptKey(), projectParameter.ignoreGlobalScene, projectParameter.useV2Item);

	this->configPanelSceneManager->setParameter(projectParameter.ambientLightUpperHemisphere, projectParameter.ambientLightLowerHemisphere, projectParameter.shadowFarDistance,
		projectParameter.shadowDirectionalLightExtrusionDistance, projectParameter.shadowDirLightTextureOffset, projectParameter.shadowColor, projectParameter.shadowQualityIndex, projectParameter.ambientLightModeIndex, 
		projectParameter.forwardMode, projectParameter.lightWidth, projectParameter.lightHeight, projectParameter.numLightSlices, projectParameter.lightsPerCell,
        projectParameter.minLightDistance, projectParameter.maxLightDistance, projectParameter.renderDistance);

	this->configPanelOgreNewt->setParameter(projectParameter.physicsUpdateRate, projectParameter.solverModel, projectParameter.solverForSingleIsland, projectParameter.broadPhaseAlgorithm,
		projectParameter.physicsThreadCount, projectParameter.linearDamping, projectParameter.angularDamping, projectParameter.gravity);

	this->configPanelRecast->setParameter(projectParameter.hasRecast, projectParameter.cellSize, projectParameter.cellHeight, projectParameter.agentMaxSlope, projectParameter.agentMaxClimb, 
		projectParameter.agentHeight, projectParameter.agentRadius, projectParameter.edgeMaxLen, projectParameter.edgeMaxError, projectParameter.regionMinSize, projectParameter.regionMergeSize,
		projectParameter.vertsPerPoly, projectParameter.detailSampleDist, projectParameter.detailSampleMaxError, projectParameter.pointExtends, projectParameter.keepInterResults);
}

bool ConfigPanel::checkProjectExists(const Ogre::String& projectName, const Ogre::String& sceneName)
{
	Ogre::String filePathName;

	Ogre::ResourceGroupManager::LocationList resLocationsList = Ogre::ResourceGroupManager::getSingleton().getResourceLocationList("Projects");
	Ogre::ResourceGroupManager::LocationList::const_iterator it = resLocationsList.cbegin();
	Ogre::ResourceGroupManager::LocationList::const_iterator itEnd = resLocationsList.cend();

	for (; it != itEnd; ++it)
	{
		// Project is always: "projects/projectName/sceneName.scene"
		filePathName = (*it)->archive->getName() + "/" + projectName + "/" + sceneName + "/" + sceneName + ".scene";
		// Check if a project with the same name does already exist
		std::ifstream ifs(filePathName);
		if (true == ifs.good())
		{
			MyGUI::Message* messageBox = MyGUI::Message::createMessageBox("Menue", MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{Overwrite}"),
				MyGUI::MessageBoxStyle::IconWarning | MyGUI::MessageBoxStyle::Yes | MyGUI::MessageBoxStyle::No, "Popup", true);

			messageBox->eventMessageBoxResult += MyGUI::newDelegate(this, &ConfigPanel::notifyMessageBoxEnd);
			return true;
		}
		break;
	}
	return false;
}

void ConfigPanel::notifyMessageBoxEnd(MyGUI::Message* sender, MyGUI::MessageBoxStyle result)
{
	if (result == MyGUI::MessageBoxStyle::Yes)
	{
		Ogre::String projectName = std::get<0>(this->configPanelProject->getParameter());
		Ogre::String sceneName = std::get<1>(this->configPanelProject->getParameter());

		Ogre::String projectPath = NOWA::Core::getSingletonPtr()->getSectionPath("Projects")[0];
		Ogre::String deletePath = projectPath + "/" + projectName + "/" + sceneName + "/" + sceneName + ".scene";

		// First delete existing scene
		std::remove(deletePath.c_str());

		this->projectManager->getProjectParameter().projectName = projectName;
		this->projectManager->getProjectParameter().sceneName = sceneName;
		this->projectManager->createNewProject(this->projectManager->getProjectParameter());
		this->setVisible(false);
	}
}

/////////////////////////////////////////////////////////////////////////////

ConfigPanelProject::ConfigPanelProject(ConfigPanel* parent)
	: BasePanelViewItem("ConfigPanelProject.layout"),
	parent(parent)
{
	
}

void ConfigPanelProject::initialise()
{
	mPanelCell->setCaption("Project");
	mPanelCell->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
	mPanelCell->setMinimized(false);

	assignWidget(this->themeBox, "themeBox");
	assignWidget(this->projectNameEdit, "projectNameEdit");
	assignWidget(this->sceneNameEdit, "sceneNameEdit");
	assignWidget(this->createProjectCheck, "createProjectCheck");
	assignWidget(this->openProjectCheck, "openProjectCheck");
	assignWidget(this->createStateCheck, "createStateCheck");
	assignWidget(this->keyEdit, "keyEdit");
	assignWidget(this->ignoreGlobalSceneCheck, "ignoreGlobalSceneCheck");
	assignWidget(this->useV2ItemCheck, "useV2ItemCheck");

	this->themeBox->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelProject::onKeyButtonPressed);
	this->projectNameEdit->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelProject::onKeyButtonPressed);
	this->projectNameEdit->eventEditSelectAccept += MyGUI::newDelegate(this, &ConfigPanelProject::onEditSelectAccepted);
	this->projectNameEdit->eventEditTextChange += MyGUI::newDelegate(this, &ConfigPanelProject::onEditTextChanged);
	this->projectNameEdit->eventComboChangePosition += MyGUI::newDelegate(this, &ConfigPanelProject::notifyComboChangedPosition);
	this->sceneNameEdit->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelProject::onKeyButtonPressed);
	this->sceneNameEdit->eventEditTextChange += MyGUI::newDelegate(this, &ConfigPanelProject::onEditTextChanged);
	this->sceneNameEdit->eventComboChangePosition += MyGUI::newDelegate(this, &ConfigPanelProject::notifyComboChangedPosition);
	this->keyEdit->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelProject::onKeyButtonPressed);
	this->createProjectCheck->eventMouseButtonClick += MyGUI::newDelegate(this, &ConfigPanelProject::buttonHit);
	this->openProjectCheck->eventMouseButtonClick += MyGUI::newDelegate(this, &ConfigPanelProject::buttonHit);
	this->createStateCheck->eventMouseButtonClick += MyGUI::newDelegate(this, &ConfigPanelProject::buttonHit);
	this->getMainWidget()->eventMouseButtonClick += MyGUI::newDelegate(this, &ConfigPanelProject::onMouseButtonClicked);
	this->ignoreGlobalSceneCheck->eventMouseButtonClick += MyGUI::newDelegate(this, &ConfigPanelProject::buttonHit);
	this->useV2ItemCheck->eventMouseButtonClick += MyGUI::newDelegate(this, &ConfigPanelProject::buttonHit);

	this->itemsEdit.push_back(this->themeBox);
	this->itemsEdit.push_back(this->projectNameEdit);
	this->itemsEdit.push_back(this->sceneNameEdit);

	this->themeBox->addItem("blue & white");
	this->themeBox->addItem("black & blue");
	this->themeBox->addItem("black & orange");
	this->themeBox->addItem("dark");
	this->themeBox->setOnlyText(this->themeBox->getItem(1));
	this->themeBox->setIndexSelected(1);
	this->themeBox->eventComboChangePosition += MyGUI::newDelegate(this, &ConfigPanelProject::notifyComboChangedPosition);

	this->projectNameEdit->setOnlyText("Project1");
	this->projectNameEdit->setEditReadOnly(false);
	this->projectNameEdit->setAutoHideList(false);
	this->sceneNameEdit->setOnlyText("Scene1");
	this->sceneNameEdit->setEditReadOnly(false);
	this->sceneNameEdit->setAutoHideList(false);

	this->keyEdit->setEditPassword(true);
	this->keyEdit->setOnlyText(Ogre::StringConverter::toString(NOWA::Core::getSingletonPtr()->getCryptKey()));
}

void ConfigPanelProject::resetSettings(void)
{
	this->projectAutoCompleteSearch.reset();

	this->projectNameEdit->removeAllItems();
	this->sceneNameEdit->removeAllItems();
	this->projectNameEdit->setOnlyText("Project1");
	this->sceneNameEdit->setOnlyText("Scene1");
	this->createProjectCheck->setStateCheck(false);
	this->openProjectCheck->setStateCheck(false);
	this->createStateCheck->setStateCheck(false);
	this->createStateCheck->setEnabled(true);

	this->ignoreGlobalSceneCheck->setStateCheck(false);
	this->useV2ItemCheck->setStateCheck(!NOWA::Core::getSingletonPtr()->getUseEntityType());

	auto filePathNames = NOWA::Core::getSingletonPtr()->getFilePathNames("Projects", "", "/*");
	for (auto filePathName : filePathNames)
	{
		size_t found = filePathName.find_last_of("/\\");
		if (Ogre::String::npos == found)
			continue;

		filePathName = filePathName.substr(found + 1, filePathName.length() - found);

		this->projectAutoCompleteSearch.addSearchText(filePathName);
	}
}

void ConfigPanelProject::notifyComboChangedPosition(MyGUI::ComboBox* sender, size_t index)
{
	if (this->themeBox == sender)
	{
		if (index == 0)
		{
			MyGUI::ResourceManager::getInstance().load("MyGUI_BlueWhiteTheme.xml");
		}
		else if (index == 1)
		{
			MyGUI::ResourceManager::getInstance().load("MyGUI_BlackBlueTheme.xml");
		}
		else if (index == 2)
		{
			MyGUI::ResourceManager::getInstance().load("MyGUI_BlackOrangeTheme.xml");
		}
		else if (index == 3)
		{
			MyGUI::ResourceManager::getInstance().load("MyGUI_Skin_BlackTheme.xml");
		}
	}
	else if (this->projectNameEdit == sender)
	{
		this->projectNameEdit->setOnlyText(this->projectNameEdit->getItem(this->projectNameEdit->getIndexSelected()));
		this->fillScenesSearchList();
	}
	else if (this->sceneNameEdit == sender)
	{
		this->sceneNameEdit->hideList();
	}
}

void ConfigPanelProject::onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode code, MyGUI::Char c)
{
	if (this->projectNameEdit == sender)
	{
		MyGUI::InputManager::getInstancePtr()->setKeyFocusWidget(this->projectNameEdit);
	}
	else if (this->sceneNameEdit == sender)
	{
		MyGUI::InputManager::getInstancePtr()->setKeyFocusWidget(this->sceneNameEdit);
	}
	if (code == MyGUI::KeyCode::Tab || code == MyGUI::KeyCode::Return)
	{
		this->projectNameEdit->hideList();
		this->sceneNameEdit->hideList();
	}
	MyGUIHelper::getInstance()->adaptFocus(sender, code, this->itemsEdit);
}

void ConfigPanelProject::buttonHit(MyGUI::Widget* sender)
{
	MyGUI::Button* button = sender->castType<MyGUI::Button>();
	// Invert the state
	button->setStateCheck(!button->getStateCheck());

	if (sender == this->createProjectCheck)
	{
		if (false == this->createProjectCheck->getStateCheck())
		{
			this->createProjectCheck->setStateCheck(false);
			// Note: when create project is is on, state must not be created!! Because its created anyways via create project! This behavior would collide!
			this->createStateCheck->setEnabled(true);
		}
		else
		{
			this->createStateCheck->setStateCheck(false);
			this->createStateCheck->setEnabled(false);
		}
	}
}

void ConfigPanelProject::onEditSelectAccepted(MyGUI::Widget* sender)
{
	if (this->projectNameEdit == sender)
	{
		this->fillScenesSearchList();
	}
	else if (this->sceneNameEdit == sender)
	{
		this->sceneNameEdit->hideList();
	}
}

void ConfigPanelProject::onEditTextChanged(MyGUI::Widget* sender)
{
	if (this->projectNameEdit == sender)
	{
		auto& matchedNames = this->projectAutoCompleteSearch.findMatchedItemWithInText(this->projectNameEdit->getOnlyText());
		this->projectNameEdit->removeAllItems();
		if (false == matchedNames.getResults().empty())
		{
			for (size_t i = 0; i < matchedNames.getResults().size(); i++)
			{
				this->projectNameEdit->addItem(matchedNames.getResults()[i].getMatchedItemText());
			}
			this->projectNameEdit->showList();
		}
		else
		{
			this->projectNameEdit->hideList();
		}		
	}
	else if (this->sceneNameEdit == sender)
	{
		if (this->sceneNameEdit->getTextLength() > 0)
			this->sceneNameEdit->showList();
		else
			this->sceneNameEdit->hideList();
	}
}

void ConfigPanelProject::onMouseButtonClicked(MyGUI::Widget* sender)
{
	if (this->getMainWidget() == sender)
	{
		this->projectNameEdit->hideList();
		this->sceneNameEdit->hideList();
	}
}

void ConfigPanelProject::fillScenesSearchList(void)
{
	auto filePathNames = NOWA::Core::getSingletonPtr()->getFilePathNames("Projects", this->projectNameEdit->getOnlyText(), "*.scene");
	for (auto& filePathName : filePathNames)
	{
		Ogre::String fileName = NOWA::Core::getSingletonPtr()->getFileNameFromPath(filePathName);
		fileName = fileName.substr(0, fileName.find(".scene"));
		this->sceneNameEdit->addItem(fileName);
	}
	this->projectNameEdit->hideList();
}

void ConfigPanelProject::shutdown()
{
	this->itemsEdit.clear();
	// Not necessary and will not work
	/*mWidgetClient->_destroyChildWidget(this->workspaceColorEdit);
	mWidgetClient->_destroyChildWidget(this->workspaceNameBox);
	mWidgetClient->_destroyChildWidget(this->workspaceColorButton);*/
}

void ConfigPanelProject::setParameter(const Ogre::String& projectName, const Ogre::String& sceneName, bool createProject, bool openProject, bool createOwnState, int key, bool ignoreGlobalScene, bool useV2Item)
{
	if (true == projectName.empty())
		this->projectNameEdit->setOnlyText("Project1");
	else
		this->projectNameEdit->setOnlyText(projectName);
	
	if (true == sceneName.empty())
		this->sceneNameEdit->setOnlyText("Scene1");
	else
		this->sceneNameEdit->setOnlyText(sceneName);

	this->createProjectCheck->setStateCheck(createProject);
	this->openProjectCheck->setStateCheck(openProject);
	this->createStateCheck->setStateCheck(createOwnState);
	this->keyEdit->setOnlyText(Ogre::StringConverter::toString(key));
	this->ignoreGlobalSceneCheck->setStateCheck(ignoreGlobalScene);
	this->useV2ItemCheck->setStateCheck(useV2Item);
}

std::tuple<Ogre::String, Ogre::String, bool, bool, bool, int, bool, bool> ConfigPanelProject::getParameter(void) const
{
	return std::make_tuple(this->projectNameEdit->getOnlyText(), this->sceneNameEdit->getOnlyText(), this->createProjectCheck->getStateCheck(), 
		this->openProjectCheck->getStateCheck(), this->createStateCheck->getStateCheck(), Ogre::StringConverter::parseInt(this->keyEdit->getOnlyText()), 
		this->ignoreGlobalSceneCheck->getStateCheck(), this->useV2ItemCheck->getStateCheck());
}

/////////////////////////////////////////////////////////////////////////////

ConfigPanelSceneManager::ConfigPanelSceneManager(ConfigPanel* parent)
	: BasePanelViewItem("ConfigPanelSceneManager.layout"),
	parent(parent)
{

}

void ConfigPanelSceneManager::initialise()
{
	mPanelCell->setCaption("Scene");
	mPanelCell->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
	mPanelCell->setMinimized(true);

	assignWidget(this->ambientLightEdit1, "ambientLightEdit1");
	assignWidget(this->ambientLightColour1, "ambientLightColour1");
	assignWidget(this->ambientLightEdit2, "ambientLightEdit2");
	assignWidget(this->ambientLightColour2, "ambientLightColour2");
	assignWidget(this->shadowFarDistanceEdit, "shadowFarDistanceEdit");
	assignWidget(this->shadowDirectionalLightExtrusionDistanceEdit, "shadowDirectionalLightExtrusionDistanceEdit");
	assignWidget(this->shadowDirLightTextureOffsetEdit, "shadowDirLightTextureOffsetEdit");
	assignWidget(this->shadowColorEdit, "shadowColorEdit");
	assignWidget(this->shadowQualityCombo, "shadowQualityCombo");
	assignWidget(this->ambientLightModeCombo, "ambientLightModeCombo");
	
	assignWidget(this->forwardModeCombo, "forwardModeCombo");
	assignWidget(this->lightWidthEditBox, "lightWidthEdit");
	assignWidget(this->lightHeightEditBox, "lightHeightEdit");
	assignWidget(this->numLightSlicesEditBox, "numLightSlicesEdit");
	assignWidget(this->lightsPerCellEditBox, "lightsPerCellEdit");
	assignWidget(this->minLightDistanceEditBox, "minLightDistanceEdit");
	assignWidget(this->maxLightDistanceEditBox, "maxLightDistanceEdit");
	assignWidget(this->renderDistanceEditBox, "renderDistanceEdit");
	assignWidget(this->lightWidthTextBox, "lightWidthText");
	assignWidget(this->lightHeightTextBox, "lightHeightText");
	assignWidget(this->numLightSlicesTextBox, "numLightSlicesText");
	assignWidget(this->lightsPerCellTextBox, "lightsPerCellText");
	assignWidget(this->minLightDistanceTextBox, "minLightDistanceText");
	assignWidget(this->maxLightDistanceTextBox, "maxLightDistanceText");
	assignWidget(this->renderDistanceTextBox, "renderDistanceText");

	this->ambientLightEdit1->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelSceneManager::onKeyButtonPressed);
	this->ambientLightColour1->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelSceneManager::onKeyButtonPressed);
	this->ambientLightEdit2->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelSceneManager::onKeyButtonPressed);
	this->ambientLightColour2->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelSceneManager::onKeyButtonPressed);
	this->shadowFarDistanceEdit->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelSceneManager::onKeyButtonPressed);
	this->shadowDirectionalLightExtrusionDistanceEdit->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelSceneManager::onKeyButtonPressed);
	this->shadowDirLightTextureOffsetEdit->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelSceneManager::onKeyButtonPressed);
	this->shadowColorEdit->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelSceneManager::onKeyButtonPressed);
	this->lightWidthEditBox->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelSceneManager::onKeyButtonPressed);
	this->lightHeightEditBox->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelSceneManager::onKeyButtonPressed);
	this->numLightSlicesEditBox->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelSceneManager::onKeyButtonPressed);
	this->lightsPerCellEditBox->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelSceneManager::onKeyButtonPressed);
	this->minLightDistanceEditBox->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelSceneManager::onKeyButtonPressed);
	this->maxLightDistanceEditBox->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelSceneManager::onKeyButtonPressed);
	this->renderDistanceEditBox->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelSceneManager::onKeyButtonPressed);
	this->lightWidthTextBox->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelSceneManager::onKeyButtonPressed);
	this->lightHeightTextBox->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelSceneManager::onKeyButtonPressed);
	this->numLightSlicesTextBox->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelSceneManager::onKeyButtonPressed);
	this->lightsPerCellTextBox->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelSceneManager::onKeyButtonPressed);
	this->minLightDistanceTextBox->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelSceneManager::onKeyButtonPressed);
	this->maxLightDistanceTextBox->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelSceneManager::onKeyButtonPressed);
	this->renderDistanceTextBox->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelSceneManager::onKeyButtonPressed);

	this->itemsEdit.push_back(this->ambientLightEdit1);
	this->itemsEdit.push_back(this->ambientLightColour1);
	this->itemsEdit.push_back(this->ambientLightEdit2);
	this->itemsEdit.push_back(this->ambientLightColour2);
	this->itemsEdit.push_back(this->shadowFarDistanceEdit);
	this->itemsEdit.push_back(this->shadowDirectionalLightExtrusionDistanceEdit);
	this->itemsEdit.push_back(this->shadowDirLightTextureOffsetEdit);
	this->itemsEdit.push_back(this->shadowColorEdit);
	this->itemsEdit.push_back(this->lightWidthEditBox);
	this->itemsEdit.push_back(this->lightHeightEditBox);
	this->itemsEdit.push_back(this->numLightSlicesEditBox);
	this->itemsEdit.push_back(this->lightsPerCellEditBox);
	this->itemsEdit.push_back(this->minLightDistanceEditBox);
	this->itemsEdit.push_back(this->maxLightDistanceEditBox);
	this->itemsEdit.push_back(this->renderDistanceEditBox);
	this->itemsEdit.push_back(this->lightWidthTextBox);
	this->itemsEdit.push_back(this->lightHeightTextBox);
	this->itemsEdit.push_back(this->numLightSlicesTextBox);
	this->itemsEdit.push_back(this->lightsPerCellTextBox);
	this->itemsEdit.push_back(this->minLightDistanceTextBox);
	this->itemsEdit.push_back(this->maxLightDistanceTextBox);
	this->itemsEdit.push_back(this->renderDistanceTextBox);

	this->shadowQualityCombo->setIndexSelected(4);
	this->ambientLightModeCombo->setIndexSelected(0);

	this->ambientLightColour1->eventMouseButtonClick += MyGUI::newDelegate(this, &ConfigPanelSceneManager::buttonHit);
	this->ambientLightColour2->eventMouseButtonClick += MyGUI::newDelegate(this, &ConfigPanelSceneManager::buttonHit);
	this->forwardModeCombo->eventComboChangePosition += MyGUI::newDelegate(this, &ConfigPanelSceneManager::notifyComboChangedPosition);
	this->forwardModeCombo->setIndexSelected(2);
}

void ConfigPanelSceneManager::onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode code, MyGUI::Char c)
{
	MyGUIHelper::getInstance()->adaptFocus(sender, code, this->itemsEdit);
}

void ConfigPanelSceneManager::shutdown()
{
	this->itemsEdit.clear();
}

void ConfigPanelSceneManager::setParameter(const Ogre::ColourValue& ambientLightUpperHemisphere, const Ogre::ColourValue& ambientLightLowerHemisphere, Ogre::Real shadowFarDistance, Ogre::Real shadowDirectionalLightExtrusionDistance,
	Ogre::Real shadowDirLightTextureOffset, const Ogre::ColourValue& shadowColor, unsigned short shadowQuality, unsigned short ambientLightMode, unsigned short forwardMode, unsigned int lightWidth,
		unsigned int lightHeight, unsigned int numLightSlices, unsigned int lightsPerCell, Ogre::Real minLightDistance, Ogre::Real maxLightDistance, Ogre::Real renderDistance)
{
	Ogre::Vector3 tempAmbientLightUpperHemisphere = Ogre::Vector3(ambientLightUpperHemisphere.r, ambientLightUpperHemisphere.g, ambientLightUpperHemisphere.b);
	Ogre::Vector3 tempAmbientLowerUpperHemisphere = Ogre::Vector3(ambientLightLowerHemisphere.r, ambientLightLowerHemisphere.g, ambientLightLowerHemisphere.b);
	Ogre::Vector3 tempShadowColour = Ogre::Vector3(shadowColor.r, shadowColor.g, shadowColor.b);

	this->ambientLightEdit1->setOnlyText(Ogre::StringConverter::toString(tempAmbientLightUpperHemisphere));
	this->ambientLightEdit2->setOnlyText(Ogre::StringConverter::toString(tempAmbientLowerUpperHemisphere));
	this->shadowFarDistanceEdit->setOnlyText(Ogre::StringConverter::toString(shadowFarDistance));
	this->shadowDirectionalLightExtrusionDistanceEdit->setOnlyText(Ogre::StringConverter::toString(shadowDirectionalLightExtrusionDistance));
	this->shadowDirLightTextureOffsetEdit->setOnlyText(Ogre::StringConverter::toString(shadowDirLightTextureOffset));
	this->shadowColorEdit->setOnlyText(Ogre::StringConverter::toString(tempShadowColour));
	this->shadowQualityCombo->setIndexSelected(shadowQuality);
	this->ambientLightModeCombo->setIndexSelected(ambientLightMode);
	
	this->forwardModeCombo->setIndexSelected(forwardMode);
	this->notifyComboChangedPosition(this->forwardModeCombo, this->forwardModeCombo->getIndexSelected());

	// Ogre tells that width/height must be modular devideable by 4
	if (lightWidth % ARRAY_PACKED_REALS != 0)
	{
		lightWidth = 4;
	}
	if (lightHeight % ARRAY_PACKED_REALS != 0)
	{
		lightHeight = 4;
	}

	this->lightWidthEditBox->setOnlyText(Ogre::StringConverter::toString(lightWidth));
	this->lightHeightEditBox->setOnlyText(Ogre::StringConverter::toString(lightHeight));
	this->numLightSlicesEditBox->setOnlyText(Ogre::StringConverter::toString(numLightSlices));
	this->lightsPerCellEditBox->setOnlyText(Ogre::StringConverter::toString(lightsPerCell));
	this->minLightDistanceEditBox->setOnlyText(Ogre::StringConverter::toString(minLightDistance));
	this->maxLightDistanceEditBox->setOnlyText(Ogre::StringConverter::toString(maxLightDistance));
	if (renderDistance <= 0)
	{
		renderDistance = 100;
	}
	this->renderDistanceEditBox->setOnlyText(Ogre::StringConverter::toString(renderDistance));
}

std::tuple<Ogre::ColourValue, Ogre::ColourValue, Ogre::Real, Ogre::Real, Ogre::Real, Ogre::ColourValue, unsigned short,
			unsigned short, unsigned short, unsigned int, unsigned int, unsigned int, unsigned int, Ogre::Real, Ogre::Real, Ogre::Real> ConfigPanelSceneManager::getParameter(void) const
{
	return std::make_tuple(Ogre::StringConverter::parseColourValue(this->ambientLightEdit1->getOnlyText()),
		Ogre::StringConverter::parseColourValue(this->ambientLightEdit2->getOnlyText()),
		Ogre::StringConverter::parseReal(this->shadowFarDistanceEdit->getOnlyText()),
		Ogre::StringConverter::parseReal(this->shadowDirectionalLightExtrusionDistanceEdit->getOnlyText()),
		Ogre::StringConverter::parseReal(this->shadowDirLightTextureOffsetEdit->getOnlyText()),
		Ogre::StringConverter::parseColourValue(this->shadowColorEdit->getOnlyText()),
		static_cast<unsigned short>(this->shadowQualityCombo->getIndexSelected()),
		static_cast<unsigned short>(this->ambientLightModeCombo->getIndexSelected()),

		static_cast<unsigned short>(this->forwardModeCombo->getIndexSelected()),
		Ogre::StringConverter::parseUnsignedInt(this->lightWidthEditBox->getOnlyText()),
		Ogre::StringConverter::parseUnsignedInt(this->lightHeightEditBox->getOnlyText()),
		Ogre::StringConverter::parseUnsignedInt(this->numLightSlicesEditBox->getOnlyText()),
		Ogre::StringConverter::parseUnsignedInt(this->lightsPerCellEditBox->getOnlyText()),
		Ogre::StringConverter::parseReal(this->minLightDistanceEditBox->getOnlyText()),
		Ogre::StringConverter::parseReal(this->maxLightDistanceEditBox->getOnlyText()),
			  Ogre::StringConverter::parseReal(this->renderDistanceEditBox->getOnlyText())
		);
}

void ConfigPanelSceneManager::notifyComboChangedPosition(MyGUI::ComboBox* sender, size_t index)
{
	if (sender == this->forwardModeCombo)
	{
		if (0 == index || 1 == index)
		{
			this->lightWidthEditBox->setVisible(false);
			this->lightHeightEditBox->setVisible(false);
			this->numLightSlicesEditBox->setVisible(false);
			this->lightsPerCellEditBox->setVisible(false);
			this->minLightDistanceEditBox->setVisible(false);
			this->maxLightDistanceEditBox->setVisible(false);
			this->renderDistanceEditBox->setVisible(false);
			this->lightWidthTextBox->setVisible(false);
			this->lightHeightTextBox->setVisible(false);
			this->numLightSlicesTextBox->setVisible(false);
			this->lightsPerCellTextBox->setVisible(false);
			this->minLightDistanceTextBox->setVisible(false);
			this->maxLightDistanceTextBox->setVisible(false);
			this->renderDistanceTextBox->setVisible(false);
		}
		else
		{
			this->lightWidthEditBox->setVisible(true);
			this->lightHeightEditBox->setVisible(true);
			this->numLightSlicesEditBox->setVisible(true);
			this->lightsPerCellEditBox->setVisible(true);
			this->minLightDistanceEditBox->setVisible(true);
			this->maxLightDistanceEditBox->setVisible(true);
			this->renderDistanceEditBox->setVisible(true);
			this->lightWidthTextBox->setVisible(true);
			this->lightHeightTextBox->setVisible(true);
			this->numLightSlicesTextBox->setVisible(true);
			this->lightsPerCellTextBox->setVisible(true);
			this->minLightDistanceTextBox->setVisible(true);
			this->maxLightDistanceTextBox->setVisible(true);
			this->renderDistanceTextBox->setVisible(true);
		}
	}
}

void ConfigPanelSceneManager::buttonHit(MyGUI::Widget* sender)
{
	if (this->ambientLightColour1 == sender)
	{
		ColourPanelManager::getInstance()->getColourPanel()->getWidget()->setUserData(MyGUI::Any(Ogre::String("colour1")));
	}
	else if (this->ambientLightColour2 == sender)
	{
		ColourPanelManager::getInstance()->getColourPanel()->getWidget()->setUserData(MyGUI::Any(Ogre::String("colour2")));
	}

	ColourPanelManager::getInstance()->getColourPanel()->setVisible(true);

	MyGUI::InputManager::getInstancePtr()->removeWidgetModal(parent->getMainWidget());
	MyGUI::InputManager::getInstancePtr()->addWidgetModal(ColourPanelManager::getInstance()->getColourPanel()->getWidget());
	ColourPanelManager::getInstance()->getColourPanel()->eventColourAccept = MyGUI::newDelegate(this, &ConfigPanelSceneManager::notifyColourAccept);
	ColourPanelManager::getInstance()->getColourPanel()->eventColourCancel = MyGUI::newDelegate(this, &ConfigPanelSceneManager::notifyColourCancel);
}

void ConfigPanelSceneManager::notifyColourAccept(MyGUI::ColourPanel* sender)
{
	Ogre::String* colourSender = sender->getWidget()->getUserData<Ogre::String>();
	Ogre::Vector3 colour(sender->getColour().red, sender->getColour().green, sender->getColour().blue);
	if (nullptr != colourSender)
	{
		if ("colour1" == *colourSender)
			this->ambientLightEdit1->setOnlyText(Ogre::StringConverter::toString(colour));
		else if ("colour2" == *colourSender)
			this->ambientLightEdit2->setOnlyText(Ogre::StringConverter::toString(colour));
	}

	MyGUI::InputManager::getInstancePtr()->removeWidgetModal(ColourPanelManager::getInstance()->getColourPanel()->getWidget());
	MyGUI::InputManager::getInstancePtr()->addWidgetModal(parent->getMainWidget());
	ColourPanelManager::getInstance()->getColourPanel()->setVisible(false);
}

void ConfigPanelSceneManager::notifyColourCancel(MyGUI::ColourPanel* sender)
{
	MyGUI::InputManager::getInstancePtr()->removeWidgetModal(ColourPanelManager::getInstance()->getColourPanel()->getWidget());
	MyGUI::InputManager::getInstancePtr()->addWidgetModal(parent->getMainWidget());
	ColourPanelManager::getInstance()->getColourPanel()->setVisible(false);
}


/////////////////////////////////////////////////////////////////////////////

ConfigPanelOgreNewt::ConfigPanelOgreNewt()
	: BasePanelViewItem("ConfigPanelOgreNewt.layout")
{

}

void ConfigPanelOgreNewt::initialise()
{
	mPanelCell->setCaption("OgreNewt");
	mPanelCell->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
	mPanelCell->setMinimized(true);

	assignWidget(this->updateRateEdit, "updateRateEdit");
	assignWidget(this->solverModelCombo, "solverModelCombo");
	assignWidget(this->solverForSingleIslandCheck, "solverForSingleIslandCheck");
	assignWidget(this->broadPhaseAlgorithmCombo, "broadPhaseAlgorithmCombo");
	assignWidget(this->threadCountCombo, "threadCountCombo");
	assignWidget(this->linearDampingEdit, "linearDampingEdit");
	assignWidget(this->angularDampingEdit, "angularDampingEdit");
	assignWidget(this->gravityEdit, "gravityEdit");

	this->updateRateEdit->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelOgreNewt::onKeyButtonPressed);
	this->solverModelCombo->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelOgreNewt::onKeyButtonPressed);;
	this->broadPhaseAlgorithmCombo->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelOgreNewt::onKeyButtonPressed);
	this->linearDampingEdit->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelOgreNewt::onKeyButtonPressed);
	this->angularDampingEdit->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelOgreNewt::onKeyButtonPressed);
	this->gravityEdit->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelOgreNewt::onKeyButtonPressed);

	this->itemsEdit.push_back(this->updateRateEdit);
	this->itemsEdit.push_back(this->solverModelCombo);
	this->itemsEdit.push_back(this->solverForSingleIslandCheck);
	this->itemsEdit.push_back(this->broadPhaseAlgorithmCombo);
	this->itemsEdit.push_back(this->linearDampingEdit);
	this->itemsEdit.push_back(this->angularDampingEdit);
	this->itemsEdit.push_back(this->gravityEdit);

	this->solverModelCombo->setIndexSelected(4);
	this->broadPhaseAlgorithmCombo->setIndexSelected(0);
	this->threadCountCombo->setIndexSelected(0);
	this->solverForSingleIslandCheck->eventMouseButtonClick += MyGUI::newDelegate(this, &ConfigPanelOgreNewt::buttonHit);
}

void ConfigPanelOgreNewt::shutdown()
{
	this->itemsEdit.clear();
}

void ConfigPanelOgreNewt::buttonHit(MyGUI::Widget* sender)
{
	MyGUI::Button* button = sender->castType<MyGUI::Button>();
		// Invert the state
	button->setStateCheck(!button->getStateCheck());
}

void ConfigPanelOgreNewt::onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode code, MyGUI::Char c)
{
	MyGUIHelper::getInstance()->adaptFocus(sender, code, this->itemsEdit);
}

void ConfigPanelOgreNewt::setParameter(Ogre::Real updateRate, unsigned short solverModel, bool solverForSingleIsland, unsigned short broadPhaseAlgorithm, unsigned short threadCount, Ogre::Real linearDamping,
	const Ogre::Vector3& angularDamping, const Ogre::Vector3& gravity)
{
	this->updateRateEdit->setOnlyText(Ogre::StringConverter::toString(updateRate));
	this->solverModelCombo->setIndexSelected(solverModel);
	this->solverForSingleIslandCheck->setStateSelected(solverForSingleIsland);
	this->broadPhaseAlgorithmCombo->setIndexSelected(broadPhaseAlgorithm);
	size_t index = this->threadCountCombo->findItemIndexWith(Ogre::StringConverter::toString(threadCount));
	this->threadCountCombo->setIndexSelected(index);
	this->linearDampingEdit->setOnlyText(Ogre::StringConverter::toString(linearDamping));
	this->angularDampingEdit->setOnlyText(Ogre::StringConverter::toString(angularDamping));
	this->gravityEdit->setOnlyText(Ogre::StringConverter::toString(gravity));
}

std::tuple<Ogre::Real, unsigned short, bool, unsigned short, unsigned short, Ogre::Real, Ogre::Vector3, Ogre::Vector3> ConfigPanelOgreNewt::getParameter(void) const
{
	Ogre::String strThreadCount = Ogre::String(this->threadCountCombo->getItemNameAt(this->threadCountCombo->getIndexSelected()));
	
	return std::make_tuple(Ogre::StringConverter::parseReal(this->updateRateEdit->getOnlyText()),
		static_cast<unsigned short>(this->solverModelCombo->getIndexSelected()),
		this->solverForSingleIslandCheck->getStateSelected(),
		static_cast<unsigned short>(this->broadPhaseAlgorithmCombo->getIndexSelected()),
		static_cast<unsigned short>(Ogre::StringConverter::parseUnsignedInt(strThreadCount)),
		Ogre::StringConverter::parseReal(this->linearDampingEdit->getOnlyText()),
		Ogre::StringConverter::parseVector3(this->angularDampingEdit->getOnlyText()),
		Ogre::StringConverter::parseVector3(this->gravityEdit->getOnlyText()));
}

/////////////////////////////////////////////////////////////////////////////

ConfigPanelRecast::ConfigPanelRecast()
	: BasePanelViewItem("ConfigPanelOgreRecast.layout")
{
}

void ConfigPanelRecast::initialise()
{
	mPanelCell->setCaption("A*-Navigation (Recast)");
	mPanelCell->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
	mPanelCell->setMinimized(true);

	assignWidget(this->navigationCheck, "navigationCheck");
	assignWidget(this->cellSizeEdit, "cellSizeEdit");
	assignWidget(this->cellHeightEdit, "cellHeightEdit");
	assignWidget(this->agentMaxSlopeEdit, "agentMaxSlopeEdit");
	assignWidget(this->agentMaxClimbEdit, "agentMaxClimbEdit");
	assignWidget(this->agentHeightEdit, "agentHeightEdit");
	assignWidget(this->agentRadiusEdit, "agentRadiusEdit");
	assignWidget(this->edgeMaxLenEdit, "edgeMaxLenEdit");
	assignWidget(this->edgeMaxErrorEdit, "edgeMaxErrorEdit");
	assignWidget(this->regionMinSizeEdit, "regionMinSizeEdit");
	assignWidget(this->regionMergeSizeEdit, "regionMergeSizeEdit");
	assignWidget(this->vertsPerPolyCombo, "vertsPerPolyCombo");
	assignWidget(this->detailSampleDistEdit, "detailSampleDistEdit");
	assignWidget(this->detailSampleMaxErrorEdit, "detailSampleMaxErrorEdit");
	assignWidget(this->pointExtendsEdit, "pointExtendsEdit");
	assignWidget(this->keepInterResultsCheck, "keepInterResultsCheck");

	this->cellSizeEdit->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelRecast::onKeyButtonPressed);
	this->cellHeightEdit->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelRecast::onKeyButtonPressed);
	this->agentMaxSlopeEdit->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelRecast::onKeyButtonPressed);
	this->agentMaxClimbEdit->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelRecast::onKeyButtonPressed);
	this->agentHeightEdit->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelRecast::onKeyButtonPressed);
	this->agentRadiusEdit->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelRecast::onKeyButtonPressed);
	this->edgeMaxLenEdit->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelRecast::onKeyButtonPressed);
	this->edgeMaxErrorEdit->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelRecast::onKeyButtonPressed);
	this->regionMinSizeEdit->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelRecast::onKeyButtonPressed);
	this->regionMergeSizeEdit->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelRecast::onKeyButtonPressed);
	this->detailSampleDistEdit->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelRecast::onKeyButtonPressed);
	this->detailSampleMaxErrorEdit->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelRecast::onKeyButtonPressed);
	this->pointExtendsEdit->eventKeyButtonPressed += MyGUI::newDelegate(this, &ConfigPanelRecast::onKeyButtonPressed);

	this->itemsEdit.push_back(this->cellSizeEdit);
	this->itemsEdit.push_back(this->cellHeightEdit);
	this->itemsEdit.push_back(this->agentMaxSlopeEdit);
	this->itemsEdit.push_back(this->agentHeightEdit);
	this->itemsEdit.push_back(this->agentMaxClimbEdit);
	this->itemsEdit.push_back(this->agentRadiusEdit);
	this->itemsEdit.push_back(this->edgeMaxLenEdit);
	this->itemsEdit.push_back(this->edgeMaxErrorEdit);
	this->itemsEdit.push_back(this->regionMinSizeEdit);
	this->itemsEdit.push_back(this->regionMergeSizeEdit);
	this->itemsEdit.push_back(this->detailSampleDistEdit);
	this->itemsEdit.push_back(this->detailSampleMaxErrorEdit);
	this->itemsEdit.push_back(this->pointExtendsEdit);

	this->vertsPerPolyCombo->setIndexSelected(5);
	this->navigationCheck->eventMouseButtonClick += MyGUI::newDelegate(this, &ConfigPanelRecast::buttonHit);
	this->keepInterResultsCheck->eventMouseButtonClick += MyGUI::newDelegate(this, &ConfigPanelRecast::buttonHit);
}

void ConfigPanelRecast::shutdown()
{
	this->itemsEdit.clear();
}

void ConfigPanelRecast::buttonHit(MyGUI::Widget* sender)
{
	MyGUI::Button* button = sender->castType<MyGUI::Button>();
	// Invert the state
	button->setStateCheck(!button->getStateCheck());
}

void ConfigPanelRecast::onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode code, MyGUI::Char c)
{
	MyGUIHelper::getInstance()->adaptFocus(sender, code, this->itemsEdit);
}

void ConfigPanelRecast::setParameter(bool hasRecast,
										Ogre::Real cellSize,
										Ogre::Real cellHeight,
										Ogre::Real agentMaxSlope,
										Ogre::Real agentMaxClimb,
										Ogre::Real agentHeight,
										Ogre::Real agentRadius,
										Ogre::Real edgeMaxLen,
										Ogre::Real edgeMaxError,
										Ogre::Real regionMinSize,
										Ogre::Real regionMergeSize,
										unsigned short vertsPerPoly,
										Ogre::Real detailSampleDist,
										Ogre::Real detailSampleMaxError,
										Ogre::Vector3 pointExtends,
										bool keepInterResults
									)
{
	MyGUI::TextBox* textBox = findWidgetBySuffix(mWidgetClient, "cellSizeText");
	// Do not call that twice
	if (true == textBox->getNeedToolTip())
	{
		return;
	}

	this->navigationCheck->setStateCheck(hasRecast);
	this->navigationCheck->setNeedToolTip(true);
	this->navigationCheck->setUserString("tooltip", "Use recast navigation mesh feature for this scene?");
	this->navigationCheck->eventToolTip += MyGUI::newDelegate(MyGUIHelper::getInstance(), &MyGUIHelper::notifyToolTip);
	this->cellSizeEdit->setOnlyText(Ogre::StringConverter::toString(cellSize));

	textBox->setNeedToolTip(true);
	textBox->setUserString("tooltip", "Cellsize (cs) is the width and depth resolution used when sampling the source geometry. "
		"The width and depth of the cell columns that make up voxel fields. "
		"Cells are laid out on the width / depth plane of voxel fields. Width is associated with the x - axis of the source geometry.Depth is associated with the z - axis. "
		"A lower value allows for the generated meshes to more closely match the source geometry, but at a higher processing and memory cost. "
		"The xz - plane cell size to use for fields. [Limit:> 0][Units:wu]. "
		"cs and ch define voxel / grid / cell size. So their values have significant side effects on all parameters defined in voxel units. "
		"The minimum value for this parameter depends on the platform's ");
	textBox->eventToolTip += MyGUI::newDelegate(MyGUIHelper::getInstance(), &MyGUIHelper::notifyToolTip);

	this->cellHeightEdit->setOnlyText(Ogre::StringConverter::toString(cellHeight));
	textBox = findWidgetBySuffix(mWidgetClient, "cellHeightText");
	textBox->setNeedToolTip(true);
	textBox->setUserString("tooltip", "Cellheight (ch) is the height resolution used when sampling the source geometry. The height of the voxels in voxel fields. "
		"Height is associated with the y - axis of the source geometry. "
		"A smaller value allows for the final meshes to more closely match the source geometry at a potentially higher processing cost. "
		"(Unlike cellSize, using a lower value for cellHeight does not significantly increase memory use.) "
		"The y - axis cell size to use for fields. [Limit:> 0][Units:wu]. "
		"cs and ch define voxel / grid / cell size. So their values have significant side effects on all parameters defined in voxel units. "
		"The minimum value for this parameter depends on the platform's floating point accuracy, with the practical minimum usually around 0.05. "
		"Setting ch lower will result in more accurate detection of areas the agent can still pass under, as min walkable height is discretisized "
		"in number of cells. Note: If several levels (e.g. in a house) are involved, this value should be at least 0.4, else It can happen, that the player does stand on the level 2 but navigation points are calculated for level 1.");
	textBox->eventToolTip += MyGUI::newDelegate(MyGUIHelper::getInstance(), &MyGUIHelper::notifyToolTip);


	this->agentMaxSlopeEdit->setOnlyText(Ogre::StringConverter::toString(agentMaxSlope));
	textBox = findWidgetBySuffix(mWidgetClient, "agentMaxSlopeText");
	textBox->setNeedToolTip(true);
	textBox->setUserString("tooltip", "* The maximum slope that is considered traversable (in degrees). "
		"[Limits:0 <= value < 180] "
		"The practical upper limit for this parameter is usually around 85 degrees. All higher values will result in performance issues during nav mesh generation.");
	textBox->eventToolTip += MyGUI::newDelegate(MyGUIHelper::getInstance(), &MyGUIHelper::notifyToolTip);



	this->agentMaxClimbEdit->setOnlyText(Ogre::StringConverter::toString(agentMaxClimb));
	textBox = findWidgetBySuffix(mWidgetClient, "agentHeightText");
	textBox->setNeedToolTip(true);
	textBox->setUserString("tooltip", "The Maximum ledge height that is considered to still be traversable. "
		"This parameter serves at setting walkableClimb(maxTraversableStep) parameter, precision of this parameter is determined by cellHeight(ch). "
		"[Limit:>= 0] "
		"Allows the mesh to flow over low lying obstructions such as curbs and up / down stairways. The value is usually set to how far up / down an agent can step.");
	textBox->eventToolTip += MyGUI::newDelegate(MyGUIHelper::getInstance(), &MyGUIHelper::notifyToolTip);



	this->agentHeightEdit->setOnlyText(Ogre::StringConverter::toString(agentHeight));
	textBox = findWidgetBySuffix(mWidgetClient, "agentMaxClimbText");
	textBox->setNeedToolTip(true);
	textBox->setUserString("tooltip", "The height of an agent. Defines the minimum height that "
		"agents can walk under. Parts of the navmesh with lower ceilings will be pruned off.");
	textBox->eventToolTip += MyGUI::newDelegate(MyGUIHelper::getInstance(), &MyGUIHelper::notifyToolTip);


	this->agentRadiusEdit->setOnlyText(Ogre::StringConverter::toString(agentRadius));
	textBox = findWidgetBySuffix(mWidgetClient, "agentRadiusText");
	textBox->setNeedToolTip(true);
	textBox->setUserString("tooltip", "The radius on the xz (ground) plane of the circle that describes the agent (character) size. "
		"Serves at setting walkableRadius(traversableAreaBorderSize) parameter, the precision of walkableRadius is affected by cellSize(cs). "
		"This parameter is also used by DetourCrowd to determine the area other agents have to avoid in order not to collide with an agent. "
		"The distance to erode / shrink the walkable area of the heightfield away from obstructions. "
		"[Limit:>= 0] "
		"In general, this is the closest any part of the final mesh should get to an obstruction in the source geometry. It is usually set to the maximum agent radius. "
		"While a value of zero is legal, it is not recommended and can result in odd edge case issues.");
	textBox->eventToolTip += MyGUI::newDelegate(MyGUIHelper::getInstance(), &MyGUIHelper::notifyToolTip);


	this->edgeMaxLenEdit->setOnlyText(Ogre::StringConverter::toString(edgeMaxLen));
	textBox = findWidgetBySuffix(mWidgetClient, "edgeMaxLenText");
	textBox->setNeedToolTip(true);
	textBox->setUserString("tooltip", "The maximum allowed length for contour edges along the border of the mesh. "
		"[Limit:>= 0] "
		"Extra vertices will be inserted as needed to keep contour edges below this length. A value of zero effectively disables this feature. "
		"Serves at setting maxEdgeLen, the precision of maxEdgeLen is affected by cellSize(cs).");
	textBox->eventToolTip += MyGUI::newDelegate(MyGUIHelper::getInstance(), &MyGUIHelper::notifyToolTip);

	this->edgeMaxErrorEdit->setOnlyText(Ogre::StringConverter::toString(edgeMaxError));
	textBox = findWidgetBySuffix(mWidgetClient, "edgeMaxErrorText");
	textBox->setNeedToolTip(true);
	textBox->setUserString("tooltip", "The maximum distance a simplfied contour's border edges should deviate the original raw contour. (edge matching) "
		"[Limit:>= 0][Units:wu] "
		"The effect of this parameter only applies to the xz - plane. "
		"The maximum distance the edges of meshes may deviate from the source geometry. "
		"A lower value will result in mesh edges following the xz - plane geometry contour more accurately at the expense of an increased triangle count. "
		"A value to zero is not recommended since it can result in a large increase in the number of polygons in the final meshes at a high processing cost.");
	textBox->eventToolTip += MyGUI::newDelegate(MyGUIHelper::getInstance(), &MyGUIHelper::notifyToolTip);

	this->regionMinSizeEdit->setOnlyText(Ogre::StringConverter::toString(regionMinSize));
	textBox = findWidgetBySuffix(mWidgetClient, "regionMinSizeText");
	textBox->setNeedToolTip(true);
	textBox->setUserString("tooltip", "The minimum number of cells allowed to form isolated island areas (size). "
		"[Limit:>= 0] "
		"Any regions that are smaller than this area will be marked as unwalkable. This is useful in removing useless regions that can sometimes form on geometry such as table tops, box tops, etc.");
	textBox->eventToolTip += MyGUI::newDelegate(MyGUIHelper::getInstance(), &MyGUIHelper::notifyToolTip);
	
	this->regionMergeSizeEdit->setOnlyText(Ogre::StringConverter::toString(regionMergeSize));
	textBox = findWidgetBySuffix(mWidgetClient, "regionMergeSizeText");
	textBox->setNeedToolTip(true);
	textBox->setUserString("tooltip", "Any regions with a span count smaller than this value will, if possible, be merged with larger regions. [Limit:>= 0][Units:vx]");
	textBox->eventToolTip += MyGUI::newDelegate(MyGUIHelper::getInstance(), &MyGUIHelper::notifyToolTip);


	this->vertsPerPolyCombo->setIndexSelected(vertsPerPoly - 1);
	textBox = findWidgetBySuffix(mWidgetClient, "vertsPerPolyText");
	textBox->setNeedToolTip(true);
	textBox->setUserString("tooltip", "The maximum number of vertices allowed for polygons generated during the contour to polygon conversion process. "
		"[Limit:>= 3] "
		"If the mesh data is to be used to construct __declspec(dllexport) a Detour navigation mesh, then the upper limit is limited to <= DT_VERTS_PER_POLYGON(= 6). "
		"The maximum number of vertices per polygon for polygons generated during the voxel to polygon conversion process. "
		"Higher values increase processing cost, but can also result in better formed polygons in the final meshes. A value of around 6 is generally adequate with diminishing returns for higher values.");
	textBox->eventToolTip += MyGUI::newDelegate(MyGUIHelper::getInstance(), &MyGUIHelper::notifyToolTip);

	this->detailSampleDistEdit->setOnlyText(Ogre::StringConverter::toString(detailSampleDist));
	textBox = findWidgetBySuffix(mWidgetClient, "detailSampleDistText");
	textBox->setNeedToolTip(true);
	textBox->setUserString("tooltip", "Sets the sampling distance to use when generating the detail mesh. "
		"(For height detail only.) [Limits:0 or >= 0.9][Units:wu] "
		"Sets the sampling distance to use when matching the detail mesh to the surface of the original geometry. "
		"Impacts how well the final detail mesh conforms to the surface contour of the original geometry. "
		"Higher values result in a detail mesh which conforms more closely to the original geometry's surface at the cost of a higher final triangle count and higher processing cost. "
		"Setting this argument to less than 0.9 disables this functionality.");
	textBox->eventToolTip += MyGUI::newDelegate(MyGUIHelper::getInstance(), &MyGUIHelper::notifyToolTip);


	this->detailSampleMaxErrorEdit->setOnlyText(Ogre::StringConverter::toString(detailSampleMaxError));
	textBox = findWidgetBySuffix(mWidgetClient, "detailSampleMaxErrorText");
	textBox->setNeedToolTip(true);
	textBox->setUserString("tooltip", "The maximum distance the detail mesh surface should deviate from heightfield data. "
		"(For height detail only.) [Limit:>= 0][Units:wu] "
		"The maximum distance the surface of the detail mesh may deviate from the surface of the original geometry. "
		"The accuracy is impacted by contourSampleDistance. "
		"The value of this parameter has no meaning if contourSampleDistance is set to zero. "
		"Setting the value to zero is not recommended since it can result in a large increase in the number of triangles in the final detail mesh at a high processing cost. "
		"Stronly related to detailSampleDist(contourSampleDistance).");
	textBox->eventToolTip += MyGUI::newDelegate(MyGUIHelper::getInstance(), &MyGUIHelper::notifyToolTip);


	this->pointExtendsEdit->setOnlyText(Ogre::StringConverter::toString(pointExtends));
	textBox = findWidgetBySuffix(mWidgetClient, "pointExtendsText");
	textBox->setNeedToolTip(true);
	textBox->setUserString("tooltip", "Set the offset size (box) around points used to look for nav polygons. "
		"This offset is used in all search for points on the navmesh. "
		"The maximum offset that a specified point can be off from the navmesh.");
	textBox->eventToolTip += MyGUI::newDelegate(MyGUIHelper::getInstance(), &MyGUIHelper::notifyToolTip);


	this->keepInterResultsCheck->setStateSelected(keepInterResults);
	textBox = findWidgetBySuffix(mWidgetClient, "keepInterResultsText");
	textBox->setNeedToolTip(true);
	textBox->setUserString("tooltip", "Determines whether intermediary results are stored in OgreRecast or whether they are removed after navmesh creation.");
	textBox->eventToolTip += MyGUI::newDelegate(MyGUIHelper::getInstance(), &MyGUIHelper::notifyToolTip);
}

std::tuple<bool, Ogre::Real, Ogre::Real, Ogre::Real, Ogre::Real, Ogre::Real, Ogre::Real, Ogre::Real, Ogre::Real, Ogre::Real, Ogre::Real, 
	unsigned short, Ogre::Real, Ogre::Real, Ogre::Vector3, bool> ConfigPanelRecast::getParameter(void) const
{
	return std::make_tuple(
		this->navigationCheck->getStateCheck(),
		Ogre::StringConverter::parseReal(this->cellSizeEdit->getOnlyText()),
		Ogre::StringConverter::parseReal(this->cellHeightEdit->getOnlyText()),
		Ogre::StringConverter::parseReal(this->agentMaxSlopeEdit->getOnlyText()),
		Ogre::StringConverter::parseReal(this->agentMaxClimbEdit->getOnlyText()),
		Ogre::StringConverter::parseReal(this->agentHeightEdit->getOnlyText()),
		Ogre::StringConverter::parseReal(this->agentRadiusEdit->getOnlyText()),
		Ogre::StringConverter::parseReal(this->edgeMaxLenEdit->getOnlyText()),
		Ogre::StringConverter::parseReal(this->edgeMaxErrorEdit->getOnlyText()),
		Ogre::StringConverter::parseReal(this->regionMinSizeEdit->getOnlyText()),
		Ogre::StringConverter::parseReal(this->regionMergeSizeEdit->getOnlyText()),
		static_cast<unsigned short>(this->vertsPerPolyCombo->getIndexSelected() + 1),
		Ogre::StringConverter::parseReal(this->detailSampleDistEdit->getOnlyText()),
		Ogre::StringConverter::parseReal(this->detailSampleMaxErrorEdit->getOnlyText()),
		Ogre::StringConverter::parseVector3(this->pointExtendsEdit->getOnlyText()),
		this->keepInterResultsCheck->getStateSelected());
}
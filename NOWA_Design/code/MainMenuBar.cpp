#include "NOWAPrecompiled.h"
#include "MainMenuBar.h"
#include "FileSystemInfo/FileSystemInfo.h"
// #include "FileSystemInfo/SettingsManager.h"

#include "ProjectManager.h"
#include "main/EventManager.h"
#include "GuiEvents.h"
#include "MyGUIHelper.h"

#include "modules/OgreNewtModule.h"
#include "modules/OgreRecastModule.h"
#include "gameobject/PlayerControllerComponents.h"

#include "RecentFilesManager.h"
#include "../res/resource.h"
#include <windows.h>
#include <shellapi.h>

namespace
{
	const short START_INDEX = 20;
}

MainMenuBar::MainMenuBar(ProjectManager* projectManager, MyGUI::Widget* _parent)
	: wraps::BaseLayout("MainMenu.layout", _parent),
	projectManager(projectManager),
	configPanel(nullptr),
	mainMenuBar(nullptr),
	fileMenuItem(nullptr),
	editMenuItem(nullptr),
	cameraMenuItem(nullptr),
	utilitiesMenuItem(nullptr),
	simulationMenuItem(nullptr),
	helpMenuItem(nullptr),
	analysisWindow(nullptr),
	deployWindow(nullptr),
	luaAnalysisWindow(nullptr),
	keyEdit(nullptr),
	resultLabel(nullptr),
	luaErrorEdit(nullptr),
	errorCount(0),
	luaAnalysisButton(nullptr),
	luaErrorFirstTime(true),
	luaApiButton(nullptr),
	luaApiWindow(nullptr),
	luaApiTree(nullptr),
	luaApiSearchEdit(nullptr),
	meshSearchCombo(nullptr),
	meshToolWindow(nullptr),
	simulationWindow(nullptr),
	aboutWindow(nullptr),
	sceneDescriptionWindow(nullptr),
	createComponentPluginWindow(nullptr),
	cPlusPlusComponentGenerator(nullptr),
	bDrawNavigationMesh(false),
	bDrawCollisionLines(false),
	bTestSelectedGameObjects(false),
	pluginNameEdit(nullptr)
{
	assignWidget(this->mainMenuBar, "mainMenuBar");
	assignWidget(this->fileMenuItem, "fileMenuItem");
	assignWidget(this->editMenuItem, "editMenuItem");
	assignWidget(this->cameraMenuItem, "cameraMenuItem");
	assignWidget(this->utilitiesMenuItem, "utilitiesMenuItem");
	assignWidget(this->simulationMenuItem, "simulationMenuItem");
	assignWidget(this->helpMenuItem, "helpMenuItem");

	// Init recent files manager
	new RecentFilesManager();
	RecentFilesManager::getInstance().initialise();

	unsigned int id = 0;

	// Create file items
	{
		MyGUI::MenuControl* fileMenuControl = this->fileMenuItem->createItemChild();
		MyGUI::MenuItem* menuItem = fileMenuControl->addItem("newMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{New}");
		menuItem->hideItemChild();
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = fileMenuControl->addItem("openMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{Open}");
		menuItem->hideItemChild();
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = fileMenuControl->addItem("saveMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{Save}");
		menuItem->setEnabled(false);
		menuItem->hideItemChild();
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = fileMenuControl->addItem("saveAsMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{SaveAs}");
		menuItem->setEnabled(false);
		menuItem->hideItemChild();
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = fileMenuControl->addItem("settingsMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{Settings}");
		menuItem->setEnabled(false);
		menuItem->hideItemChild();
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = fileMenuControl->addItem("createComponentPluginMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{CreateComponentPlugin}");
		menuItem->setEnabled(true);
		menuItem->hideItemChild();
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = fileMenuControl->addItem("createCPlusPlusMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{CreateCPlusPlusProject}");
		menuItem->setEnabled(true);
		menuItem->hideItemChild();
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = fileMenuControl->addItem("openLogMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{OpenLog}");
		menuItem->setEnabled(true);
		menuItem->hideItemChild();
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = fileMenuControl->addItem("copySceneMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{CopyScene}");
		menuItem->setEnabled(true);
		menuItem->hideItemChild();
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);
		
		menuItem = fileMenuControl->addItem("Separator", MyGUI::MenuItemType::Separator);

		this->recentFileItem[0] = fileMenuControl->addItem("recentFileMenuItem1", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		this->recentFileItem[0]->setCaption("--");
		this->recentFileItem[0]->hideItemChild();
		this->recentFileItem[0]->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		this->recentFileItem[1] = fileMenuControl->addItem("recentFileMenuItem2", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		this->recentFileItem[1]->setCaption("--");
		this->recentFileItem[1]->hideItemChild();
		this->recentFileItem[1]->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		this->recentFileItem[2] = fileMenuControl->addItem("recentFileMenuItem3", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		this->recentFileItem[2]->setCaption("--");
		this->recentFileItem[2]->hideItemChild();
		this->recentFileItem[2]->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		this->recentFileItem[3] = fileMenuControl->addItem("recentFileMenuItem4", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		this->recentFileItem[3]->setCaption("--");
		this->recentFileItem[3]->hideItemChild();
		this->recentFileItem[3]->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		this->recentFileItem[4] = fileMenuControl->addItem("recentFileMenuItem5", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		this->recentFileItem[4]->setCaption("--");
		this->recentFileItem[4]->hideItemChild();
		this->recentFileItem[4]->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		this->recentFileItem[5] = fileMenuControl->addItem("recentFileMenuItem6", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		this->recentFileItem[5]->setCaption("--");
		this->recentFileItem[5]->hideItemChild();
		this->recentFileItem[5]->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		this->recentFileItem[6] = fileMenuControl->addItem("recentFileMenuItem7", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		this->recentFileItem[6]->setCaption("--");
		this->recentFileItem[6]->hideItemChild();
		this->recentFileItem[6]->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		this->recentFileItem[7] = fileMenuControl->addItem("recentFileMenuItem8", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		this->recentFileItem[7]->setCaption("--");
		this->recentFileItem[7]->hideItemChild();
		this->recentFileItem[7]->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		this->recentFileItem[8] = fileMenuControl->addItem("recentFileMenuItem9", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		this->recentFileItem[8]->setCaption("--");
		this->recentFileItem[8]->hideItemChild();
		this->recentFileItem[8]->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		this->recentFileItem[9] = fileMenuControl->addItem("recentFileMenuItem10", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		this->recentFileItem[9]->setCaption("--");
		this->recentFileItem[9]->hideItemChild();
		this->recentFileItem[9]->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = fileMenuControl->addItem("Separator", MyGUI::MenuItemType::Separator);

		menuItem = fileMenuControl->addItem("closeMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{Close}");
		menuItem->hideItemChild();
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);
	}

	// Create Edit items
	{
		MyGUI::MenuControl* editMenuControl = this->editMenuItem->createItemChild();
		MyGUI::MenuItem* menuItem = editMenuControl->addItem("saveGroupMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{SaveGroup}");
		menuItem->hideItemChild();
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = editMenuControl->addItem("loadGroupMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{LoadGroup}");
		menuItem->hideItemChild();
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = editMenuControl->addItem("loadMeshResourcesMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{AddMeshResources}");
		menuItem->hideItemChild();
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = editMenuControl->addItem("openProjectFolderMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{OpenProjectFolder}");
		menuItem->hideItemChild();
		menuItem->setEnabled(false);
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = editMenuControl->addItem("startGameMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{StartGame}");
		menuItem->hideItemChild();
		menuItem->setEnabled(false);
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = editMenuControl->addItem("Separator", MyGUI::MenuItemType::Separator, Ogre::StringConverter::toString(id++));

		menuItem = editMenuControl->addItem("undoMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{Undo}");
		menuItem->hideItemChild();
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = editMenuControl->addItem("redoMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{Redo}");
		menuItem->hideItemChild();
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = editMenuControl->addItem("selectionUndoMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{SelectionUndo}");
		menuItem->hideItemChild();
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = editMenuControl->addItem("selectionRedoMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{SelectionRedo}");
		menuItem->hideItemChild();
		menuItem->setEnabled(false);
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);
	}

	// Create Camera items
	{
		MyGUI::MenuControl* cameraMenuControl = this->cameraMenuItem->createItemChild();
		MyGUI::MenuItem* menuItem = cameraMenuControl->addItem("frontViewMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{FrontView}");
		menuItem->hideItemChild();
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = cameraMenuControl->addItem("topViewMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{TopView}");
		menuItem->hideItemChild();
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = cameraMenuControl->addItem("backViewMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{BackView}");
		menuItem->hideItemChild();
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = cameraMenuControl->addItem("bottomViewMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{BottomView}");
		menuItem->hideItemChild();
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = cameraMenuControl->addItem("leftViewMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{LeftView}");
		menuItem->hideItemChild();
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = cameraMenuControl->addItem("rightViewMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{RightView}");
		menuItem->hideItemChild();
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = cameraMenuControl->addItem("Separator", MyGUI::MenuItemType::Separator);

		menuItem = cameraMenuControl->addItem("cameraUndoMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{CameraUndo}");
		menuItem->hideItemChild();
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = cameraMenuControl->addItem("cameraRedoMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{CameraRedo}");
		menuItem->hideItemChild();
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = cameraMenuControl->addItem("Separator", MyGUI::MenuItemType::Separator);

		menuItem = cameraMenuControl->addItem("rendeSolidMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{RenderSolid}");
		menuItem->hideItemChild();
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = cameraMenuControl->addItem("renderWireFrameMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{RenderWireframe}");
		menuItem->hideItemChild();
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = cameraMenuControl->addItem("renterPointsMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{RenderPoints}");
		menuItem->hideItemChild();
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);
	}

	// Create Utilities items
	{
		MyGUI::MenuControl* utilitiesMenuControl = this->utilitiesMenuItem->createItemChild();
		MyGUI::MenuItem* menuItem = utilitiesMenuControl->addItem("sceneAnalysisMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{SceneAnalysis}");
		menuItem->hideItemChild();
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = utilitiesMenuControl->addItem("deployMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{Deploy}");
		menuItem->hideItemChild();
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = utilitiesMenuControl->addItem("luaAnalysisMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("Lua Errors");
		menuItem->hideItemChild();
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = utilitiesMenuControl->addItem("luaApiMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("Lua Api");
		menuItem->hideItemChild();
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = utilitiesMenuControl->addItem("openAllLuaScripts", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{OpenAllLuaScripts}");
		menuItem->hideItemChild();
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = utilitiesMenuControl->addItem("meshToolMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("Mesh Tool");
		menuItem->hideItemChild();
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = utilitiesMenuControl->addItem("drawNavigationMeshMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{DrawNavigationMesh}");
		menuItem->hideItemChild();
		menuItem->setEnabled(false);
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = utilitiesMenuControl->addItem("drawCollisionLinesMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{DrawCollisionLines}");
		menuItem->hideItemChild();
		menuItem->setEnabled(false);
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = utilitiesMenuControl->addItem("optimizeSceneMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{OptimizeScene}");
		menuItem->hideItemChild();
		menuItem->setEnabled(false);
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = utilitiesMenuControl->addItem("toggleMyGuiVisibilityMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{ToggleMyGuiComponents}");
		menuItem->hideItemChild();
		menuItem->setEnabled(false);
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);
	}

	// Create Simulation items
	{
		MyGUI::MenuControl* simulationMenuControl = this->simulationMenuItem->createItemChild();

		MyGUI::MenuItem* menuItem = simulationMenuControl->addItem("controlSelectedPlayerMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{ControlSelectedPlayer}");
		menuItem->hideItemChild();
		menuItem->setEnabled(false);
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = simulationMenuControl->addItem("testSelectedGameObjectsMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->setCaptionWithReplacing("#{TestSelectedGameObjects}");
		menuItem->hideItemChild();
		menuItem->setEnabled(false);
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

	}

	// Create Help items
	{
		MyGUI::MenuControl* helpMenuControl = this->helpMenuItem->createItemChild();
		MyGUI::MenuItem* menuItem = helpMenuControl->addItem("aboutMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->hideItemChild();
		menuItem->setCaptionWithReplacing("#{About}");
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		menuItem = helpMenuControl->addItem("sceneDescriptionMenuItem", MyGUI::MenuItemType::Normal, Ogre::StringConverter::toString(id++));
		menuItem->hideItemChild();
		menuItem->setCaptionWithReplacing("#{SceneDescription}");
		menuItem->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);
	}
	
	this->updateRecentFilesMenu();

	this->mainMenuBar->eventMenuCtrlAccept += newDelegate(this, &MainMenuBar::notifyPopupMenuAccept);
	this->fileMenuItem->eventMouseButtonClick += newDelegate(this, &MainMenuBar::buttonHit);
	this->editMenuItem->eventMouseButtonClick += newDelegate(this, &MainMenuBar::buttonHit);

	this->configPanel = new ConfigPanel(this->projectManager, MyGUI::FloatCoord(0.0f, 0.03f, 0.2f, 0.93f));
	this->configPanel->setVisible(false);

	this->enableMenuEntries(false);

	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &MainMenuBar::handleProjectManipulation), EventDataProjectManipulation::getStaticEventType());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &MainMenuBar::handleSceneValid), EventDataSceneValid::getStaticEventType());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &MainMenuBar::handleProjectEncoded), NOWA::EventDataProjectEncoded::getStaticEventType());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &MainMenuBar::handleLuaError), NOWA::EventDataPrintLuaError::getStaticEventType());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &MainMenuBar::handleSceneInvalid), EventDataSceneInvalid::getStaticEventType());
}

MainMenuBar::~MainMenuBar()
{
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &MainMenuBar::handleProjectManipulation), EventDataProjectManipulation::getStaticEventType());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &MainMenuBar::handleSceneValid), EventDataSceneValid::getStaticEventType());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &MainMenuBar::handleProjectEncoded), NOWA::EventDataProjectEncoded::getStaticEventType());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &MainMenuBar::handleLuaError), NOWA::EventDataPrintLuaError::getStaticEventType());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &MainMenuBar::handleSceneInvalid), EventDataSceneInvalid::getStaticEventType());

	MyGUI::LayoutManager::getInstancePtr()->unloadLayout(this->analysisWidgets);
	MyGUI::LayoutManager::getInstancePtr()->unloadLayout(this->deployWidgets);
	MyGUI::LayoutManager::getInstancePtr()->unloadLayout(this->luaAnalysisWidgets);
	MyGUI::LayoutManager::getInstancePtr()->unloadLayout(this->aboutWindowWidgets);
	MyGUI::LayoutManager::getInstancePtr()->unloadLayout(this->sceneDescriptionWindowWidgets);
	MyGUI::LayoutManager::getInstancePtr()->unloadLayout(this->createComponentPluginWindowWidgets);

	if (nullptr != this->cPlusPlusComponentGenerator)
	{
		MyGUI::LayoutManager::getInstancePtr()->unloadLayout(this->componentPluginWindowWidgets);

		delete this->cPlusPlusComponentGenerator;
		this->cPlusPlusComponentGenerator = nullptr;
	}

	if (nullptr != this->configPanel)
	{
		this->configPanel->destroyContent();
		delete this->configPanel;
		this->configPanel = nullptr;
	}

	RecentFilesManager::getInstance().shutdown();
	delete RecentFilesManager::getInstancePtr();
}

void MainMenuBar::enableMenuEntries(bool enable)
{
	if (true == NOWA::Core::getSingletonPtr()->isProjectEncoded())
	{
		boost::shared_ptr<NOWA::EventDataFeedback> eventDataFeedback(new NOWA::EventDataFeedback(false, "#{EncodedReadonly}"));
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataFeedback);
	}

	this->fileMenuItem->getItemChild()->getItemAt(SAVE)->setEnabled(enable && !NOWA::Core::getSingletonPtr()->isProjectEncoded()); // save
	this->fileMenuItem->getItemChild()->getItemAt(SAVE_AS)->setEnabled(enable && !NOWA::Core::getSingletonPtr()->isProjectEncoded()); // save as
	this->fileMenuItem->getItemChild()->getItemAt(SETTINGS)->setEnabled(enable);
	this->fileMenuItem->getItemChild()->getItemAt(COPY_SCENE)->setEnabled(enable);
	this->fileMenuItem->getItemChild()->getItemAt(COPY_SCENE)->setEnabled(enable);
	this->fileMenuItem->getItemChild()->getItemAt(CREATE_CPLUS_PLUS_PROJECT)->setEnabled(enable);


	this->editMenuItem->getItemChild()->getItemAt(SAVE_GROUP)->setEnabled(enable); // save group
	this->editMenuItem->getItemChild()->getItemAt(LOAD_GROUP)->setEnabled(enable); // load group
	this->editMenuItem->getItemChild()->getItemAt(LOAD_MESH_RESOURCE)->setEnabled(enable); // load mesh resource
	this->editMenuItem->getItemChild()->getItemAt(OPEN_PROJECT_FOLDER)->setEnabled(enable); // open project folder
	this->editMenuItem->getItemChild()->getItemAt(START_GAME)->setEnabled(enable); // start game

	this->editMenuItem->getItemChild()->getItemAt(UNDO)->setEnabled(enable); // undo
	this->editMenuItem->getItemChild()->getItemAt(REDO)->setEnabled(enable); // redo
	this->editMenuItem->getItemChild()->getItemAt(SELECTION_UNDO)->setEnabled(enable); // selection undo
	this->editMenuItem->getItemChild()->getItemAt(SELECTION_REDO)->setEnabled(enable); // selection redo

	this->cameraMenuItem->getItemChild()->getItemAt(0)->setEnabled(enable); // camera views
	this->cameraMenuItem->getItemChild()->getItemAt(1)->setEnabled(enable); // camera views
	this->cameraMenuItem->getItemChild()->getItemAt(2)->setEnabled(enable); // camera views
	this->cameraMenuItem->getItemChild()->getItemAt(3)->setEnabled(enable); // camera views
	this->cameraMenuItem->getItemChild()->getItemAt(4)->setEnabled(enable); // camera views
	this->cameraMenuItem->getItemChild()->getItemAt(5)->setEnabled(enable); // camera views
	this->cameraMenuItem->getItemChild()->getItemAt(7)->setEnabled(enable); // camera undo
	this->cameraMenuItem->getItemChild()->getItemAt(8)->setEnabled(enable); // camera redo

	this->utilitiesMenuItem->getItemChild()->getItemAt(0)->setEnabled(enable); // scene analysis
	this->utilitiesMenuItem->getItemChild()->getItemAt(1)->setEnabled(enable); // deploy
	this->utilitiesMenuItem->getItemChild()->getItemAt(2)->setEnabled(enable); // lua analysis
	this->utilitiesMenuItem->getItemChild()->getItemAt(3)->setEnabled(enable); // lua api
	this->utilitiesMenuItem->getItemChild()->getItemAt(4)->setEnabled(enable); // open all lua scripts
	this->utilitiesMenuItem->getItemChild()->getItemAt(5)->setEnabled(enable); // mesh utils
	this->utilitiesMenuItem->getItemChild()->getItemAt(6)->setEnabled(enable && nullptr != NOWA::AppStateManager::getSingletonPtr()->getOgreRecastModule()->getOgreRecast()); // Draw navigation mesh
	this->utilitiesMenuItem->getItemChild()->getItemAt(7)->setEnabled(enable); // Draw Collision Lines
	this->utilitiesMenuItem->getItemChild()->getItemAt(8)->setEnabled(enable); // Optimize scene
	this->utilitiesMenuItem->getItemChild()->getItemAt(9)->setEnabled(enable); // Toggle MyGUI Components

	this->simulationMenuItem->getItemChild()->getItemAt(0)->setEnabled(enable); // control selected player
	this->simulationMenuItem->getItemChild()->getItemAt(1)->setEnabled(enable); // test selected game objects
}

void MainMenuBar::enableFileMenu(bool enable)
{
	this->fileMenuItem->getItemChild()->getItemAt(NEW)->setEnabled(enable);
	this->fileMenuItem->getItemChild()->getItemAt(OPEN)->setEnabled(enable);
	this->fileMenuItem->getItemChild()->getItemAt(SAVE)->setEnabled(enable);
	this->fileMenuItem->getItemChild()->getItemAt(SAVE_AS)->setEnabled(enable);
	this->fileMenuItem->getItemChild()->getItemAt(SETTINGS)->setEnabled(enable);
	this->fileMenuItem->getItemChild()->getItemAt(COPY_SCENE)->setEnabled(enable);

	const RecentFilesManager::VectorUString& recentFiles = RecentFilesManager::getInstance().getRecentFiles();
	if (!recentFiles.empty())
	{
		size_t index = 0;
		for (RecentFilesManager::VectorUString::const_iterator iter = recentFiles.begin(); iter != recentFiles.end(); ++iter, ++index)
		{
			this->recentFileItem[index]->setEnabled(enable);
		}
	}
}

void MainMenuBar::setVisible(bool visible)
{
	this->mainMenuBar->setVisible(visible);
	this->fileMenuItem->setVisible(visible);
	this->editMenuItem->setVisible(visible);
	this->cameraMenuItem->setVisible(visible);
	this->utilitiesMenuItem->setVisible(visible);
	this->simulationMenuItem->setVisible(visible);
	this->helpMenuItem->setVisible(visible);
}

void MainMenuBar::callNewProject(void)
{
	boost::shared_ptr<EventDataSceneValid> eventDataSceneInvalid(new EventDataSceneValid(false));
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataSceneInvalid);

	this->configPanel->callForSettings(false);
	this->configPanel->setVisible(true);
}

void MainMenuBar::clearLuaErrors(void)
{
	this->luaErrors.clear();
	this->luaErrorFirstTime = true;
	this->errorCount = 0;
	if (nullptr != this->simulationWindow)
	{
		this->simulationWindow->getCaptionWidget()->setTextColour(MyGUIHelper::getInstance()->getImportantTextColour());
		this->simulationWindow->setCaption(NOWA::Core::getSingletonPtr()->getSceneName());
	}
}

bool MainMenuBar::hasLuaErrors(void)
{
	return this->errorCount > 0;
}

void MainMenuBar::notifyPopupMenuAccept(MyGUI::MenuControl* sender, MyGUI::MenuItem* item)
{
	Ogre::String id = item->getItemId();
	unsigned int index = Ogre::StringConverter::parseUnsignedInt(id);

	switch (index)
	{
		case 0: // New
		{
			this->callNewProject();
			break;
		}
		case 1: // Open
		{
			this->projectManager->showFileOpenDialog("LoadProject", "*.scene");
			break;
		}
		case SAVE: // Save
		{
			this->projectManager->saveProject();
			this->updateRecentFilesMenu();
			break;
		}
		case SAVE_AS: // Save As
		{
			this->projectManager->showFileSaveDialog("SaveProject", "*.scene", NOWA::Core::getSingletonPtr()->getCurrentProjectPath());
			this->updateRecentFilesMenu();
			break;
		}
		case SETTINGS: // Settings
		{
			this->drawCollisionLines(false);
			this->drawNavigationMap(false);
			this->configPanel->setVisible(true);
			this->configPanel->callForSettings(true);
			// Apply settings from loaded project
			this->configPanel->applySettings();
			break;
		}
		case CREATE_COMPONENT_PLUGIN:
		{
			this->showComponentPlugin();
			break;
		}
		case CREATE_CPLUS_PLUS_PROJECT:
		{
			NOWA::DeployResourceModule::getInstance()->createCPlusPlusProject(this->projectManager->getProjectParameter().projectName, this->projectManager->getProjectParameter().sceneName);
			break;
		}
		case OPEN_LOG:
		{
			NOWA::DeployResourceModule::getInstance()->openLog();
			break;
		}
		case COPY_SCENE:
		{
			this->projectManager->showFileSaveDialog("CopyScene", "*.scene", this->projectManager->getProjectParameter().projectName);
			break;
		}
		case 9: // Recent file 1
		case 10: // Recent file 2
		case 11: // Recent file 3
		case 12: // Recent file 4
		case 13: // Recent file 5
		case 14: // Recent file 6
		case 15: // Recent file 7
		case 16: // Recent file 8
		case 17: // Recent file 9
		case 18: // Recent file 10
		{
			if (Ogre::String("--") != item->getCaption().asUTF8())
			{
				boost::shared_ptr<EventDataSceneValid> eventDataSceneValid(new EventDataSceneValid(false));
				NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataSceneValid);
				Ogre::String name = item->getCaption();
				this->projectManager->loadProject(name, index - 9);
				RecentFilesManager::getInstance().setActiveFile(name);
				this->updateRecentFilesMenu();
			}
			break;
		}
		
		case 19: // Close
		{
			boost::shared_ptr<EventDataExit> eventDataExit(new EventDataExit());
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataExit);
			break;
		}
		case SAVE_GROUP + START_INDEX: // Save Group
		{
			this->projectManager->showFileSaveDialog("SaveGroup", "*.group");
			break;
		}
		case LOAD_GROUP + START_INDEX: // Load Group
		{
			this->projectManager->showFileOpenDialog("LoadGroup", "*.group");
			break;
		}
		case LOAD_MESH_RESOURCE + START_INDEX: // Add Mesh Resources
		{
			this->projectManager->showFileOpenDialog("AddMeshResources", "");
			break;
		}
		case OPEN_PROJECT_FOLDER + START_INDEX: // Open project folder
		{
			NOWA::Core::getSingletonPtr()->openFolder(NOWA::Core::getSingletonPtr()->getCurrentProjectPath());
			break;
		}
		case START_GAME + START_INDEX: // Start game
		{
			if (NOWA::DeployResourceModule::getInstance()->startGame(NOWA::Core::getSingletonPtr()->getProjectName()))
			{
				NOWA::Core::getSingletonPtr()->moveWindowToTaskbar();
			}
			else
			{
				boost::shared_ptr<NOWA::EventDataFeedback> eventDataFeedback(new NOWA::EventDataFeedback(false, "#{StartGameFailed}"));
				NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataFeedback);
			}
			break;
		}
		case UNDO + START_INDEX: // Undo
		{
			this->projectManager->getEditorManager()->undo();
			
			boost::shared_ptr<EventDataRefreshPropertiesPanel> eventDataRefreshPropertiesPanel(new EventDataRefreshPropertiesPanel());
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshPropertiesPanel);

			boost::shared_ptr<EventDataRefreshResourcesPanel> eventDataRefreshResourcesPanel(new EventDataRefreshResourcesPanel());
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshResourcesPanel);
			break;
		}
		case REDO + START_INDEX: // Redo
		{
			this->projectManager->getEditorManager()->redo();

			boost::shared_ptr<EventDataRefreshPropertiesPanel> eventDataRefreshPropertiesPanel(new EventDataRefreshPropertiesPanel());
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshPropertiesPanel);

			boost::shared_ptr<EventDataRefreshResourcesPanel> eventDataRefreshResourcesPanel(new EventDataRefreshResourcesPanel());
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshResourcesPanel);
			break;
		}
		case SELECTION_UNDO + START_INDEX: // Selection Undo
		{
			this->projectManager->getEditorManager()->getSelectionManager()->selectionUndo();
			break;
		}
		case SELECTION_REDO + START_INDEX: // Selection Redo
		{
			this->projectManager->getEditorManager()->getSelectionManager()->selectionRedo();
			break;
		}
		case 30: // Front View
		{
			this->projectManager->getEditorManager()->setCameraView(NOWA::EditorManager::EDITOR_CAMERA_VIEW_FRONT);
			break;
		}
		case 31: // TOP View
		{
			this->projectManager->getEditorManager()->setCameraView(NOWA::EditorManager::EDITOR_CAMERA_VIEW_TOP);
			break;
		}
		case 32: // Back View
		{
			this->projectManager->getEditorManager()->setCameraView(NOWA::EditorManager::EDITOR_CAMERA_VIEW_BACK);
			break;
		}
		case 33: // Bottom View
		{
			this->projectManager->getEditorManager()->setCameraView(NOWA::EditorManager::EDITOR_CAMERA_VIEW_BOTTOM);
			break;
		}
		case 34: // Left View
		{
			this->projectManager->getEditorManager()->setCameraView(NOWA::EditorManager::EDITOR_CAMERA_VIEW_LEFT);
			break;
		}
		case 35: // Right View
		{
			this->projectManager->getEditorManager()->setCameraView(NOWA::EditorManager::EDITOR_CAMERA_VIEW_RIGHT);
			break;
		}
		case 36: // Camera Undo
		{
			this->projectManager->getEditorManager()->cameraUndo();
			break;
		}
		case 37: // Camera Redo
		{
			this->projectManager->getEditorManager()->cameraRedo();
			break;
		}
		case 38: // Render Solid
		{
			NOWA::Core::getSingletonPtr()->setPolygonMode(3);
			break;
		}
		case 39: // Render Wireframe
		{
			NOWA::Core::getSingletonPtr()->setPolygonMode(2);
			break;
		}
		case 40: // Render Points
		{
			// Does not work, do not know why
			NOWA::Core::getSingletonPtr()->setPolygonMode(1);
			break;
		}
		case 41: // Scene analysis
		{
			this->showAnalysisWindow();
			break;
		}
		case 42: // Deploy
		{
			this->showDeployWindow();
			break;
		}
		case 43: // Lua Analysis
		{
			this->showLuaAnalysisWindow();
			break;
		}
		case 44: // Lua Api
		{
			this->showLuaApiWindow();
			break;
		}
		case 45: // Open all lua scripts
		{
			this->openAllLuaScripts();
			break;
		}
		case 46: // Mesh Tool
		{
			this->showMeshToolWindow();
			break;
		}
		case 47: // Draw Navigation Mesh
		{
			this->bDrawNavigationMesh = !this->bDrawNavigationMesh;
			this->drawNavigationMap(this->bDrawNavigationMesh);
			break;
		}
		case 48: // Draw Collision Lines
		{
			this->bDrawCollisionLines = !this->bDrawCollisionLines;
			this->drawCollisionLines(this->bDrawCollisionLines);
			break;
		}
		case 49: // Optimize scene
		{
			this->projectManager->getEditorManager()->optimizeScene(true);
			break;
		}
		case 50: // Toggle MyGUI Components
		{
			this->bToggleMyGUIComponents = !this->bToggleMyGUIComponents;
			this->toggleMyGUIComponents(this->bToggleMyGUIComponents);
			break;
		}
		case 51: // Control selected player
		{
			for (auto& it = this->projectManager->getEditorManager()->getSelectionManager()->getSelectedGameObjects().begin(); it != this->projectManager->getEditorManager()->getSelectionManager()->getSelectedGameObjects().end(); ++it)
			{
				// Start player controller;
				auto& PlayerControllerComponent = NOWA::makeStrongPtr(it->second.gameObject->getComponent<NOWA::PlayerControllerComponent>());
				if (nullptr != PlayerControllerComponent)
				{
					NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->activatePlayerController(true, it->second.gameObject->getId(), true);
				}
			}
			break;
		}
		case 52: // Test selected game objects
		{
			this->bTestSelectedGameObjects = !this->bTestSelectedGameObjects;
			this->activateTestSelectedGameObjects(this->bTestSelectedGameObjects);
			boost::shared_ptr<EventDataTestSelectedGameObjects> eventDataTestSelectedGameObjects(new EventDataTestSelectedGameObjects(this->bTestSelectedGameObjects));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataTestSelectedGameObjects);
			break;
		}
		case 53: // About
		{
			this->showAboutWindow();
			break;
		}
		case 54: // Scene description
		{
			this->showSceneDescriptionWindow();
			break;
		}
	}
}

void MainMenuBar::buttonHit(MyGUI::Widget* sender)
{
	if ("analysisOkButton" == sender->getName())
	{
		this->analysisWindow->setVisible(false);
	}
	else if ("deployCloseButton" == sender->getName())
	{
		this->deployWindow->setVisible(false);
	}
	else if (this->luaAnalysisButton == sender)
	{
		this->luaAnalysisWindow->setVisible(false);
	}
	else if (this->luaApiButton == sender)
	{
		this->luaApiWindow->setVisible(false);
	}
	else if ("meshToolButtonClose" == sender->getName())
	{
		this->meshToolWindow->setVisible(false);
	}
	else if ("meshToolButtonApply" == sender->getName())
	{
		this->applyMeshToolOperations();
	}
	else if ("aboutOkButton" == sender->getName())
	{
		this->aboutWindow->setVisible(false);
	}
	else if ("sceneDescriptionOkButton" == sender->getName())
	{
		this->sceneDescriptionWindow->setVisible(false);
	}
	else if ("pluginOkButton" == sender->getName())
	{
		Ogre::String componentName = this->pluginNameEdit->getOnlyText();
		if (false == componentName.empty())
		{
			NOWA::DeployResourceModule::getInstance()->createCPlusPlusComponentPluginProject(componentName);
		}
		this->createComponentPluginWindow->setVisible(false);
	}
	else if ("pluginAbordButton" == sender->getName())
	{
		this->createComponentPluginWindow->setVisible(false);
	}
	else if ("encryptButton" == sender->getName())
	{
		// Encode
		NOWA::Core::getSingletonPtr()->encodeAllFiles();
		boost::shared_ptr<NOWA::EventDataFeedback> eventDataFeedback(new NOWA::EventDataFeedback(false, "#{EncodedReadonly}"));
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataFeedback);
	}
	else if ("decryptButton" == sender->getName())
	{
		int tempKey = Ogre::StringConverter::parseReal(this->keyEdit->getOnlyText());
		bool success = NOWA::Core::getSingletonPtr()->decodeAllFiles(tempKey);

		if (true == success)
		{
			this->resultLabel->setVisible(true);
			this->resultLabel->setCaptionWithReplacing("#{Success}");
			this->resultLabel->setTextColour(MyGUI::Colour::Green);
		}
		else
		{
			this->resultLabel->setVisible(true);
			this->resultLabel->setCaptionWithReplacing("#{WrongKey}");
			this->resultLabel->setTextColour(MyGUI::Colour::Red);
		}
	}
	else if ("deployOkButton" == sender->getName())
	{
		Ogre::String projectFilePathName = NOWA::Core::getSingletonPtr()->getCurrentProjectPath();
		const auto& sceneNames = NOWA::Core::getSingletonPtr()->getSceneFileNamesInProject(projectFilePathName);
		if (sceneNames.empty())
		{
			return;
		}

		boost::shared_ptr<EventDataSceneValid> eventDataSceneValid(new EventDataSceneValid(false));
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataSceneValid);

		// First save resources for this scene
		Ogre::String currentSceneName = NOWA::Core::getSingletonPtr()->getSceneName();

		// After that for all other
		for (const Ogre::String& sceneName : sceneNames)
		{
			Ogre::String tempSceneName;
			size_t found = sceneName.find(".scene");
			if (found != std::wstring::npos)
			{
				tempSceneName = sceneName.substr(0, sceneName.size() - 6);
			}
			if (tempSceneName != NOWA::Core::getSingletonPtr()->getSceneName())
			{
				this->projectManager->loadProject(projectFilePathName + "/" + tempSceneName + "/" + tempSceneName + ".scene");
				NOWA::DeployResourceModule::getInstance()->deploy(NOWA::Core::getSingletonPtr()->getProjectName(), tempSceneName, projectFilePathName, false);
			}
		}

		// Loads back this scene
		this->projectManager->loadProject(projectFilePathName + "/" + currentSceneName + "/" + currentSceneName + ".scene");
		NOWA::DeployResourceModule::getInstance()->deploy(NOWA::Core::getSingletonPtr()->getProjectName(), currentSceneName, projectFilePathName, true);

		boost::shared_ptr<EventDataSceneValid> eventDataSceneValid2(new EventDataSceneValid(true));
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataSceneValid2);
	}
	else if (this->fileMenuItem == sender)
	{
		MyGUI::MenuControl* fileMenuControl = fileMenuItem->getItemChild();
		fileMenuControl->showMenu();
	}

	if (nullptr != this->projectManager->getEditorManager())
	{
		this->editMenuItem->getItemChild()->getItemAt(UNDO)->setEnabled(this->projectManager->getEditorManager()->canUndo());
		this->editMenuItem->getItemChild()->getItemAt(REDO)->setEnabled(this->projectManager->getEditorManager()->canRedo());
		this->editMenuItem->getItemChild()->getItemAt(SELECTION_UNDO)->setEnabled(this->projectManager->getEditorManager()->getSelectionManager()->canSelectionUndo());
		this->editMenuItem->getItemChild()->getItemAt(SELECTION_REDO)->setEnabled(this->projectManager->getEditorManager()->getSelectionManager()->canSelectionRedo());
		this->editMenuItem->getItemChild()->getItemAt(OPEN_PROJECT_FOLDER)->setEnabled(true);

		this->cameraMenuItem->getItemChild()->getItemAt(7)->setEnabled(this->projectManager->getEditorManager()->canCameraUndo());
		this->cameraMenuItem->getItemChild()->getItemAt(8)->setEnabled(this->projectManager->getEditorManager()->canCameraRedo());

		this->simulationMenuItem->getItemChild()->getItemAt(1)->setEnabled(true);
	}
}

void MainMenuBar::editTextChange(MyGUI::Widget* sender)
{
	MyGUI::EditBox* editBox = static_cast<MyGUI::EditBox*>(sender);

	if (sender->getName() == "luaApiSearchEdit")
	{
		// Start a new search each time for data that do match the search caption string
		this->luaApiAutoCompleteSearch.reset();

		this->refreshLuaApi(editBox->getOnlyText());
	}
	else if (sender->getName() == "meshSearchCombo")
	{
		// Start a new search each time for data that do match the search caption string for meshes
		this->meshAutoCompleteSearch.reset();

		this->refreshMeshes(editBox->getOnlyText());
	}
}

void MainMenuBar::updateRecentFilesMenu()
{
	const RecentFilesManager::VectorUString& recentFiles = RecentFilesManager::getInstance().getRecentFiles();

	// First reset
	for (size_t i = 0; i < RecentFilesManager::maxRecentFiles; i++)
	{
		this->recentFileItem[i]->setCaption("--");
	}

	// Then fill
	for (size_t i = 0; i < recentFiles.size(); i++)
	{
		Ogre::String file = recentFiles[i];
		this->recentFileItem[i]->setCaption(file);
	}
}

void MainMenuBar::handleProjectManipulation(NOWA::EventDataPtr eventData)
{
	// Called after project has been opened successfully
	boost::shared_ptr<EventDataProjectManipulation> castEventData = boost::static_pointer_cast<EventDataProjectManipulation>(eventData);

	ProjectManager::eProjectMode projectMode = static_cast<ProjectManager::eProjectMode>(castEventData->getMode());
	// When not save scene
	if (ProjectManager::eProjectMode::LOAD == projectMode)
	{
		this->updateRecentFilesMenu();

		this->bTestSelectedGameObjects = false;
		this->simulationMenuItem->getItemChild()->getItemAt(1)->setStateCheck(this->bTestSelectedGameObjects);
		this->projectManager->getEditorManager()->activateTestSelectedGameObjects(this->bTestSelectedGameObjects);

		this->bDrawCollisionLines = false;
		this->utilitiesMenuItem->getItemChild()->getItemAt(6)->setStateCheck(this->bDrawCollisionLines);
		NOWA::AppStateManager::getSingletonPtr()->getOgreRecastModule()->debugDrawNavMesh(this->bDrawCollisionLines);

		this->bDrawNavigationMesh = false;
		this->utilitiesMenuItem->getItemChild()->getItemAt(7)->setStateCheck(this->bDrawNavigationMesh);
		NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->showOgreNewtCollisionLines(this->bDrawNavigationMesh);

		// this->componentVisibilityMap.clear();
	}
}

void MainMenuBar::handleSceneValid(NOWA::EventDataPtr eventData)
{
	// Called after save as dialog has been closed
	this->updateRecentFilesMenu();
}

void MainMenuBar::handleProjectEncoded(NOWA::EventDataPtr eventData)
{
	boost::shared_ptr<NOWA::EventDataProjectEncoded> castEventData = boost::static_pointer_cast<NOWA::EventDataProjectEncoded>(eventData);

	this->fileMenuItem->getItemChild()->getItemAt(SAVE)->setEnabled(!castEventData->getIsEncoded());
	this->fileMenuItem->getItemChild()->getItemAt(SAVE_AS)->setEnabled(!castEventData->getIsEncoded());
}

void MainMenuBar::handleLuaError(NOWA::EventDataPtr eventData)
{
	boost::shared_ptr<NOWA::EventDataPrintLuaError> castEventData = boost::static_pointer_cast<NOWA::EventDataPrintLuaError>(eventData);
	if (false == castEventData->getErrorMessage().empty())
	{
		if (true == this->luaErrorFirstTime)
		{
			MyGUI::PointerManager::getInstancePtr()->setVisible(true);
			this->createLuaAnalysisWindow();
			this->luaErrorFirstTime = false;
		}

		this->luaErrors += "<p align='center'><color value='#000088'><h1>Script: " + castEventData->getScriptName() + "</h1><color></p><br/><br/>"
			"<p align='left'><color value='#880000'><h2>Line: " + Ogre::StringConverter::toString(castEventData->getLine()) + "</h2></color></p><br/>"
			"<p align='left'><color value='#FFFFFF'><h3>" + castEventData->getErrorMessage() + "</h3></color></p><br/><br/>";
		this->luaErrorEdit->setCaption(this->luaErrors);
		this->luaErrorEdit->updateContent();
		this->errorCount++;

		// zurücksetzen der Farbe fehlt
		// this->simulationWindow->setColour(MyGUI::Colour(0.6f, 0.1f, 0.1f));
		this->simulationWindow->getCaptionWidget()->setTextColour(MyGUI::Colour::Red);
		this->simulationWindow->setCaption(NOWA::Core::getSingletonPtr()->getSceneName() + ", Lua Errors: (" + Ogre::StringConverter::toString(this->errorCount) + ")");
	}
}

void MainMenuBar::handleSceneInvalid(NOWA::EventDataPtr eventData)
{
	boost::shared_ptr<EventDataSceneInvalid> castEventData = boost::static_pointer_cast<EventDataSceneInvalid>(eventData);
	if (-1 != castEventData->getRecentFileIndex())
	{
		RecentFilesManager::getInstance().removeRecentFile(castEventData->getRecentFileIndex());
		this->updateRecentFilesMenu();
	}
}

void MainMenuBar::showAnalysisWindow(void)
{
	if (0 == this->analysisWidgets.size())
	{
		this->analysisWidgets = MyGUI::LayoutManager::getInstancePtr()->loadLayout("AnalysisWindow.layout");
		
		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("analysisOkButton")->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);
	}

	MyGUI::FloatPoint windowPosition;
	windowPosition.left = 0.4f;
	windowPosition.top = 0.4f;
	this->analysisWindow = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Window>("analysisWindow");
	this->analysisWindow->setRealPosition(windowPosition);
	this->analysisWindow->setVisible(true);

	MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("gameObjectsCountLabel")->setCaptionWithReplacing("#{GameObjectsCount}: "
		+ Ogre::StringConverter::toString(NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects()->size()));


	const auto& overlappingGameObjects = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getOverlappingGameObjects();
	MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("overlappingGameObjectsCountLabel")->setCaptionWithReplacing("#{OverlappingGameObjectsCount}: "
																											   + Ogre::StringConverter::toString(overlappingGameObjects.size()));

	auto comboBox = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::ComboBox>("overlappingGameObjectsCombo");
	comboBox->removeAllItems();
	for (size_t i = 0; i < overlappingGameObjects.size(); i++)
	{
		comboBox->addItem(overlappingGameObjects[i]->getName());
	}

	size_t componentsCount = 0;
	unsigned int dynamicGameObjectsCount = 0;
	unsigned int lightsCount = 0;

	const auto& gameObjects = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjects();
	for (auto& it = gameObjects->begin(); it != gameObjects->end(); it++)
	{
		componentsCount += it->second->getComponents()->size();
		if (true == it->second->isDynamic())
		{
			dynamicGameObjectsCount++;
		}
		if (nullptr != NOWA::makeStrongPtr(it->second->getComponent<NOWA::LightAreaComponent>())
			|| nullptr != NOWA::makeStrongPtr(it->second->getComponent<NOWA::LightPointComponent>())
				|| nullptr != NOWA::makeStrongPtr(it->second->getComponent<NOWA::LightSpotComponent>())
					|| nullptr != NOWA::makeStrongPtr(it->second->getComponent<NOWA::LightDirectionalComponent>()))
		{
			lightsCount++;
		}
	}
	MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("dynamicGameObjectsCountLabel")->setCaptionWithReplacing("#{DynamicGameObjectsCount}: "
		+ Ogre::StringConverter::toString(dynamicGameObjectsCount));
	MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("componentsCountLabel")->setCaptionWithReplacing("#{ComponentsCount}: "
		+ Ogre::StringConverter::toString(componentsCount));

	MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("physicsBodyCountLabel")->setCaptionWithReplacing("#{PhysicsBodyCount}: "
		+ Ogre::StringConverter::toString(NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->getOgreNewt()->getBodyCount()));
	MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("physicsConstraintsCountLabel")->setCaptionWithReplacing("#{PhysicsConstraintsCount}: "
		+ Ogre::StringConverter::toString(NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->getOgreNewt()->getConstraintCount()));
	MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("physicsMemoryLabel")->setCaptionWithReplacing("#{PhysicsMemory}: "
		+ Ogre::StringConverter::toString(NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->getOgreNewt()->getMemoryUsed() / 1000) + " KB");
	MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("threadCountLabel")->setCaptionWithReplacing("#{ThreadCount}: "
		+ Ogre::StringConverter::toString(NOWA::Core::getSingletonPtr()->getCurrentThreadCount()));

	MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("lightsCountLabel")->setCaptionWithReplacing("#{LightsCount}: "
		+ Ogre::StringConverter::toString(lightsCount));
}

void MainMenuBar::showDeployWindow(void)
{
	if (0 == this->deployWidgets.size())
	{
		this->deployWidgets = MyGUI::LayoutManager::getInstancePtr()->loadLayout("DeployWindow.layout");
		
		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("deployCloseButton")->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);
		
		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("encryptButton")->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);
		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("decryptButton")->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);
		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("deployOkButton")->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);
		MyGUI::Button* button = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("deployOkButton");
		button->setNeedToolTip(true);
		button->setUserString("tooltip", "Deploys all meshes, skeleton files, textures and all materials in one json file at the given [Project] resource folder. "
			"E.g. ../media/Projects/ApplicationName/media. "
			" Creates in applicationName/bin/resources an applicationNameDeployed.cfg file. "
			" Now if application is started, in Core preLoadTextures is called for [Project] resource folder, which pre loads all textures at application start.");
		button->eventToolTip += MyGUI::newDelegate(MyGUIHelper::getInstance(), &MyGUIHelper::notifyToolTip);
	}

	MyGUI::FloatPoint windowPosition;
	windowPosition.left = 0.4f;
	windowPosition.top = 0.4f;
	this->deployWindow = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Window>("deployWindow");
	this->deployWindow->setRealPosition(windowPosition);
	this->deployWindow->setVisible(true);

	this->keyEdit = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("keyEdit");
	this->resultLabel = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::TextBox>("resultLabel");
	this->resultLabel->setVisible(false);
}

void MainMenuBar::createLuaAnalysisWindow(void)
{
	if (0 == this->luaAnalysisWidgets.size())
	{
		this->luaAnalysisWindow = MyGUI::Gui::getInstance().createWidget<MyGUI::Window>("WindowCS", MyGUI::IntCoord(10, 10, 800, 600), MyGUI::Align::Default, "Popup");
		this->luaAnalysisWindow->setMinSize(400, 400);
		MyGUI::IntCoord coord = this->luaAnalysisWindow->getClientCoord();

		this->luaErrorEdit = this->luaAnalysisWindow->createWidget<MyGUI::HyperTextBox>("HyperTextBox", MyGUI::IntCoord(0, 0, coord.width, coord.height - 55), MyGUI::Align::Stretch);
		this->luaErrorEdit->setTextSelectable(true);
		this->luaErrorEdit->eventNotifyInsideKeyButtonPressed += newDelegate(this, &MainMenuBar::notifyInsideKeyButtonPressed);
		this->luaErrorEdit->setCaption("No errors.");

		this->luaAnalysisButton = this->luaAnalysisWindow->createWidget<MyGUI::Button>("Button", MyGUI::IntCoord(0, coord.height - 40, 100, 40), MyGUI::Align::Bottom | MyGUI::Align::HCenter);
		this->luaAnalysisButton->setCaption("Ok");
		this->luaAnalysisButton->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		// Force update layout
		this->luaAnalysisWindow->setSize(this->luaAnalysisWindow->getSize().width - 1, this->luaAnalysisWindow->getSize().height - 1);
		this->luaAnalysisWindow->setSize(this->luaAnalysisWindow->getSize().width + 1, this->luaAnalysisWindow->getSize().height + 1);
		this->luaAnalysisWindow->setCaption("Lua Errors");

		this->luaAnalysisWidgets.push_back(this->luaAnalysisWindow);
		this->luaApiWidgets.push_back(this->luaErrorEdit);
		this->luaApiWidgets.push_back(this->luaAnalysisButton);

		this->simulationWindow = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Window>("simulationWindow");
	}

	MyGUI::FloatPoint windowPosition;
	windowPosition.left = 0.3f;
	windowPosition.top = 0.3f;
	
	this->luaAnalysisWindow->setRealPosition(windowPosition);
	this->luaAnalysisWindow->setVisible(false);
}

void MainMenuBar::createLuaApiWindow(void)
{
	if (0 == this->luaApiWidgets.size() || nullptr == this->luaApiWindow)
	{
		this->luaApiWindow = MyGUI::Gui::getInstance().createWidget<MyGUI::Window>("WindowCS", MyGUI::IntCoord(10, 10, 1280, 768), MyGUI::Align::Default, "Popup");
		this->luaApiWindow->setMinSize(400, 400);
		MyGUI::IntCoord coord = this->luaApiWindow->getClientCoord();

		this->luaApiSearchEdit = this->luaApiWindow->createWidget<MyGUI::EditBox>("EditBox", MyGUI::IntCoord(0, 0, coord.width, 24), MyGUI::Align::Top | MyGUI::Align::HStretch, "luaApiSearchEdit");
		this->luaApiSearchEdit->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);
		this->luaApiSearchEdit->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
		this->luaApiSearchEdit->setEditStatic(false);
		this->luaApiSearchEdit->setEditReadOnly(false);
		this->luaApiSearchEdit->eventEditTextChange += MyGUI::newDelegate(this, &MainMenuBar::editTextChange);

		this->luaApiTree = this->luaApiWindow->createWidget<MyGUI::TreeControl>("Tree", MyGUI::IntCoord(0, 24, coord.width, coord.height - 70), MyGUI::Align::Stretch);

		this->luaApiButton = this->luaApiWindow->createWidget<MyGUI::Button>("Button", MyGUI::IntCoord(0, coord.height - 40, 100, 40), MyGUI::Align::Bottom | MyGUI::Align::HCenter);
		this->luaApiButton->setCaption("Ok");
		this->luaApiButton->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		// Force update layout
		this->luaApiWindow->setSize(this->luaApiWindow->getSize().width - 1, this->luaApiWindow->getSize().height - 1);
		this->luaApiWindow->setSize(this->luaApiWindow->getSize().width + 1, this->luaApiWindow->getSize().height + 1);
		this->luaApiWindow->setCaption("Lua Api");

		this->luaApiWidgets.push_back(this->luaApiWindow);
		this->luaApiWidgets.push_back(this->luaApiSearchEdit);
		this->luaApiWidgets.push_back(this->luaApiButton);

		this->refreshLuaApi("");
	}

	MyGUI::FloatPoint windowPosition;
	windowPosition.left = 0.3f;
	windowPosition.top = 0.3f;
	
	this->luaApiWindow->setRealPosition(windowPosition);
	this->luaApiWindow->setVisible(false);
}

void MainMenuBar::showLuaAnalysisWindow(void)
{
	this->createLuaAnalysisWindow();
	this->luaAnalysisWindow->setVisible(true);
}

void MainMenuBar::showLuaApiWindow(void)
{
	this->createLuaApiWindow();
	this->luaApiWindow->setVisible(true);
}

void MainMenuBar::refreshLuaApi(const Ogre::String& filter)
{
	MyGUI::TreeControl::Node* root = this->luaApiTree->getRoot();
	root->removeAll();

	// extract to refreshLuaApi function with bool filter, see resources panel

	//			   ClassName                           function,     description
	const std::map<Ogre::String, std::vector<std::pair<Ogre::String, Ogre::String>>>& classCollection = NOWA::LuaScriptApi::getInstance()->getClassCollection();

	for (std::map<Ogre::String, std::vector<std::pair<Ogre::String, Ogre::String>>>::const_iterator 
		it = classCollection.cbegin(); it != classCollection.cend(); ++it)
	{
		MyGUI::TreeControl::Node* classNode = nullptr;
		if (true == filter.empty())
		{
			classNode = new MyGUI::TreeControl::Node(it->first, "Data");
		}
		else
		{
			// Add resource to the search
			this->luaApiAutoCompleteSearch.addSearchText(it->first);
		}
			
		if (true == filter.empty())
		{
			root->add(classNode);
		}

		for (size_t i = 0; i < it->second.size(); i++)
		{
			if (true == filter.empty())
			{
				MyGUI::TreeControl::Node* functionNode = new MyGUI::TreeControl::Node(it->second[i].first, "Data");
				classNode->add(functionNode);

				MyGUI::TreeControl::Node* descriptionNode = new MyGUI::TreeControl::Node(it->second[i].second, "Data");
				functionNode->add(descriptionNode);
			}
			else
			{
				// Add resource to the search and corresponding description node
				this->luaApiAutoCompleteSearch.addSearchText(it->second[i].first, it->first + "|" + it->second[i].second);
			}
		}
	}

	if (false == filter.empty())
	{
		MyGUI::TreeControl::Node* child = nullptr;
		// Get the matched results and add to tree
		auto& matchedResources = this->luaApiAutoCompleteSearch.findMatchedItemWithInText(filter);

		for (size_t i = 0; i < matchedResources.getResults().size(); i++)
		{
			Ogre::String data = matchedResources.getResults()[i].getMatchedItemText();
			Ogre::String userData = matchedResources.getResults()[i].getUserData();
			child = new MyGUI::TreeControl::Node(data, "Data");
			if (false == userData.empty())
			{
				Ogre::String className;
				Ogre::String description;
				size_t found = userData.find("|");
				if (Ogre::String::npos != found)
				{
					className = userData.substr(0, found);
					description = userData.substr(found + 1);
				}
				else
				{
					description = userData;
				}

				MyGUI::TreeControl::Node* childClassName = nullptr;

				if (false == className.empty())
				{
					childClassName = new MyGUI::TreeControl::Node(className, "Data");
				}

				MyGUI::TreeControl::Node* childDescription = new MyGUI::TreeControl::Node(description, "Data");
				if (nullptr != childClassName)
				{
					childClassName->add(child);
					childClassName->setExpanded(true);
					child->add(childDescription);
					root->add(childClassName);
				}
				else
				{
					child->add(childDescription);
					root->add(child);
				}
			}
		}
	}

	root->setExpanded(true);
}

void MainMenuBar::openAllLuaScripts(void)
{
	auto luaScriptComponents = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectComponents<NOWA::LuaScriptComponent>();

	if (false == luaScriptComponents.empty())
	{
		if (nullptr != luaScriptComponents[0]->getLuaScript())
		{
			Ogre::String absoluteLuaScriptFilePathName = luaScriptComponents[0]->getLuaScript()->getScriptFilePathName();
			NOWA::DeployResourceModule::getInstance()->openNOWALuaScriptEditor(absoluteLuaScriptFilePathName);
		}

		NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(3.0f));
		auto ptrFunction = [this, luaScriptComponents]() {
				for (size_t i = 1; i < luaScriptComponents.size(); i++)
				{
					if (nullptr != luaScriptComponents[i]->getLuaScript())
					{
						Ogre::String absoluteLuaScriptFilePathName = luaScriptComponents[i]->getLuaScript()->getScriptFilePathName();
						NOWA::DeployResourceModule::getInstance()->openNOWALuaScriptEditor(absoluteLuaScriptFilePathName);
					}
				}
			};

		NOWA::ProcessPtr closureProcess(new NOWA::ClosureProcess(ptrFunction));
		delayProcess->attachChild(closureProcess);
		NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
	}
}

void MainMenuBar::refreshMeshes(const Ogre::String& filter)
{
	this->meshSearchCombo->removeAllItems();

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

			while (meshIt != dirs->end())
			{
				// Add resource to the search
				this->luaApiAutoCompleteSearch.addSearchText(*meshIt);
				++meshIt;
			}
		}
		++groupIt;
	}

	auto& matchedResources = this->luaApiAutoCompleteSearch.findMatchedItemWithInText(filter);

	for (size_t i = 0; i < matchedResources.getResults().size(); i++)
	{
		Ogre::String data = matchedResources.getResults()[i].getMatchedItemText();
		// Ogre::String userData = matchedResources.getResults()[i].getUserData();

		this->meshSearchCombo->addItem(data);
	}
	// Steals the focus from input
	// this->meshSearchCombo->showList();
}

void MainMenuBar::createMeshToolWindow(void)
{
	if (0 == this->meshToolWidgets.size())
	{
		this->meshToolWidgets = MyGUI::LayoutManager::getInstancePtr()->loadLayout("MeshToolWindow.layout");

		MyGUI::EditBox* rotateLabel = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("rotateLabel");
		rotateLabel->setCaptionWithReplacing("#{MeshRotate}");
		MyGUI::EditBox* scaleLabel = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("scaleLabel");
		scaleLabel->setCaptionWithReplacing("#{MeshScale}");
		MyGUI::EditBox* transformOriginLabel = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("transformOriginLabel");
		transformOriginLabel->setCaptionWithReplacing("#{MeshTransform}");
		MyGUI::EditBox* changeAxesLabel = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("changeAxesLabel");
		changeAxesLabel->setCaptionWithReplacing("#{MeshAxesChange}");

		this->meshSearchCombo = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::ComboBox>("meshSearchCombo");
		this->meshSearchCombo->eventEditTextChange += MyGUI::newDelegate(this, &MainMenuBar::editTextChange);

		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("meshToolButtonClose")->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);
		MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Button>("meshToolButtonApply")->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		this->refreshMeshes("");
	}

	MyGUI::FloatPoint windowPosition;
	windowPosition.left = 0.3f;
	windowPosition.top = 0.3f;
	this->meshToolWindow = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::Window>("meshToolWindow");
	this->meshToolWindow->setRealPosition(windowPosition);
	this->meshToolWindow->setVisible(true);
}

void MainMenuBar::applyMeshToolOperations(void)
{
	MyGUI::EditBox* rotateEdit = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("rotateEdit");
	MyGUI::EditBox* scaleEdit = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("scaleEdit");
	MyGUI::EditBox* transformOriginEdit = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("transformOriginEdit");
	MyGUI::EditBox* changeAxesEdit = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("changeAxesEdit");
	MyGUI::EditBox* meshInfoLabel = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::EditBox>("meshInfoLabel");

	Ogre::String meshName = this->meshSearchCombo->getOnlyText();

	auto gameObjects = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsFromMeshName(this->meshSearchCombo->getOnlyText());

	if (false == gameObjects.empty())
	{
		MyGUI::Message* messageBox = MyGUI::Message::createMessageBox("Feedback", MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{MeshInUse}"),
																	  MyGUI::MessageBoxStyle::IconWarning | MyGUI::MessageBoxStyle::Ok, "Popup", true);

		return;
	}
	
	/*
	 * MeshMagick transform -rotate=90/0/1/0 robot.mesh
	 */

	// TODO: DIfferent folder
	// "D:\Ogre\GameEngineDevelopment\media\models\Destructable\"
	// Ogre::String destinationFolder = rootFolder + "/media/models/Destructable/";
	// destinationFolder = NOWA::Core::getSingletonPtr()->replaceSlashes(destinationFolder, false);

	Ogre::String rotateText = rotateEdit->getOnlyText();
	// MeshMagick transform -rotate=90/0/1/0 robot.mesh
	if (false == rotateText.empty())
	{ 
		Ogre::String params;
		params += "transform ";
		params += "-rotate=";
		params += "\"";
		params += rotateText;
		params += "\"";

		NOWA::Core::getSingletonPtr()->processMeshMagick(meshName, params);
	}
	
	Ogre::String scaleText = scaleEdit->getOnlyText();
	if (false == scaleText.empty())
	{
		Ogre::String params;
		params += "transform ";
		params += "-scale=";
		params += "\"";
		params += scaleText;
		params += "\"";

		NOWA::Core::getSingletonPtr()->processMeshMagick(meshName, params);
	}

	Ogre::String transformOriginText = transformOriginEdit->getOnlyText();
	if (false == transformOriginText.empty())
	{
		Ogre::String params;
		params += "transform ";
		params += "-transform=";
		params += "\"";
		params += transformOriginText;
		params += "\"";

		NOWA::Core::getSingletonPtr()->processMeshMagick(meshName, params);
	}

	Ogre::String changeAxesText = changeAxesEdit->getOnlyText();
	if (false == changeAxesText.empty())
	{
		Ogre::String params;
		params += "transform ";
		params += "-axes=";
		params += "\"";
		params += changeAxesText;
		params += "\"";

		NOWA::Core::getSingletonPtr()->processMeshMagick(meshName, params);
	}


	// Ogre::String params;
	// params += "info  ";
	// NOWA::Core::getSingletonPtr()->processMeshMagick(meshName, params);

	// TODO: How to get info result for meshInfoLabel??
}

void MainMenuBar::showMeshToolWindow(void)
{
	this->createMeshToolWindow();
	this->meshToolWindow->setVisible(true);
}

void MainMenuBar::notifyInsideKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char ch, const std::string& selectedText)
{
	MyGUI::HyperTextBox* textBox = sender->castType<MyGUI::HyperTextBox>(false);
	if (nullptr != textBox)
	{
		if (GetAsyncKeyState(VK_LCONTROL) && key == MyGUI::KeyCode::C)
		{
			if (false == selectedText.empty())
			{
				NOWA::Core::getSingletonPtr()->copyTextToClipboard(selectedText);
			}
		}
	}
}

void MainMenuBar::showAboutWindow(void)
{
	if (0 == this->aboutWindowWidgets.size())
	{
		this->aboutWindow = MyGUI::Gui::getInstance().createWidget<MyGUI::Window>("WindowC", MyGUI::IntCoord(10, 10, 650, 500), MyGUI::Align::Default, "Popup");
		MyGUI::IntCoord coord = this->aboutWindow->getClientCoord();

		MyGUI::HyperTextBox* hyperTextBox = this->aboutWindow->createWidget<MyGUI::HyperTextBox>("HyperTextBox", MyGUI::IntCoord(0, 0, coord.width, coord.height - 50), MyGUI::Align::Stretch);
		hyperTextBox->setUrlColour(MyGUI::Colour::White);

		hyperTextBox->setCaption(
			"<p float='left'><img width='260' height='159'>pic_NOWA</img> </p><br/>"
			"<p align='center'><color value='#1D6EA7'><h2>" + Ogre::String(NOWA_DESIGN_INTERNALNAME_STR) + "</h2></color></p><br/>"
			"<p align='center'><color value='#FFFFFF'><h3>" + Ogre::String(NOWA_DESIGN_PRODUCTVERSION_STR) + "</h3></color></p><br/>"
			"<p align='center'><color value='#FFFFFF'>Developed by <b>Lukas Kalinowski</b></color></p><br/>"
			"<p align='center'><color value='#FFFFFF'>" + Ogre::String(NOWA_LEGALCOPYRIGHT_STR) + "</color></p><br/>"
			"<p align='center'><color value='#FFFFFF'>" + Ogre::String(NOWA_LEGALTRADEMARKS2_STR) + "</color></p><br/>"
			"<p align='center'><color value='#FFFFFF'><url value='" + Ogre::String(NOWA_COMPANYDOMAIN_STR) + "'>" + Ogre::String(NOWA_COMPANYDOMAIN_STR) + "</url></color></p><br/><br/>"

			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_1) + "<url value='" + Ogre::String(NOWA_LICENSE_STR_2) + "'>" + Ogre::String(NOWA_LICENSE_STR_2) + "</url>" + Ogre::String(NOWA_LICENSE_STR_3) + "</color></p>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_4) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_5) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_6) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_7) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_8) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_9) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_10) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_11) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_12) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_13) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_14) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_15) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_16) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_17) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_18) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_19) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_20) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_21) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_22) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_23) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_24) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_25) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_26) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_27) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_28) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_29) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_30) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_31) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_32) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_33) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_34) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_35) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_36) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_37) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_38) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_39) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_40) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_41) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_42) + "</color></p><br/>"
			"<p><color value='#1D6EA7'>" + Ogre::String(NOWA_LICENSE_STR_43) + "</color></p><br/>"

			"<p><color value='#FFFFFF'> - Ogre3D: MIT <url value='https://www.ogre3d.org/licensing'>https://www.ogre3d.org/licensing</url></color></p>"
			"<p><color value='#FFFFFF'> - fparser: LGPL <url value='https://github.com/TheComet/ik/blob/master/LICENSE'>https://github.com/TheComet/ik/blob/master/LICENSE</url></color></p>"
			"<p><color value='#FFFFFF'> - kiss-fft: BSD <url value='https://github.com/berndporr/kiss-fft/blob/master/LICENSE'>https://github.com/berndporr/kiss-fft/blob/master/LICENSE</url></color></p>"
			"<p><color value='#FFFFFF'> - lua: MIT <url value='https://www.lua.org/source/'>https://www.lua.org/source/</url></color></p>"
			"<p><color value='#FFFFFF'> - luabind: MIT <url value='http://luabind.sourceforge.net/docs.html'>http://luabind.sourceforge.net/docs.html</url></color></p>"
			"<p><color value='#FFFFFF'> - MyGUI: MIT <url value='http://mygui.info/pages/license.html'>http://mygui.info/pages/license.html</url></color></p>"
			"<p><color value='#FFFFFF'> - newton dynamics: ZLIB (compatible to LGPL) <url value='http://www.gzip.org/zlib/zlib_license.html'>http://www.gzip.org/zlib/zlib_license.html</url></color></p>"
			"<p><color value='#FFFFFF'> - ogg: BSD <url value='http://www.xiph.org'>http://www.xiph.org</url></color></p>"
			"<p><color value='#FFFFFF'> - vorbis: BSD <url value='http://www.xiph.org'>http://www.xiph.org</url></color></p>"
			"<p><color value='#FFFFFF'> - Raknet: BSD <url value='https://github.com/facebookarchive/RakNet/blob/master/LICENSE'>https://github.com/facebookarchive/RakNet/blob/master/LICENSE</url></color></p>"
			"<p><color value='#FFFFFF'> - OpenAL: LGPL <url value='https://en.wikipedia.org/wiki/OpenAL'>https://en.wikipedia.org/wiki/OpenAL</url></color></p>"
			"<p><color value='#FFFFFF'> - OgreAL: LGPL</color></p>"
			"<p><color value='#FFFFFF'> - OgreNewt: LGPL</color></p>"
			"<p><color value='#FFFFFF'> - OgreProzedural: MIT</color></p>"
			"<p><color value='#FFFFFF'> - OgreRecast: MIT</color></p>"
			"<p><color value='#FFFFFF'> - ParticleUniverse: MIT</color></p>"
			"<p><color value='#FFFFFF'> - TinyXML: ZLIB</color></p>"
		);

		hyperTextBox->eventUrlClick += MyGUI::newDelegate(this, &MainMenuBar::onClickUrl);
		
		MyGUI::Button* aboutOkButton = this->aboutWindow->createWidget<MyGUI::Button>("Button", MyGUI::IntCoord(0, coord.height - 40, 100, 40), MyGUI::Align::Bottom | MyGUI::Align::HCenter, "aboutOkButton");
		aboutOkButton->setCaption("Ok");
		aboutOkButton->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);
		// Force update layout
		this->aboutWindow->setSize(this->aboutWindow->getSize().width - 1, this->aboutWindow->getSize().height - 1);
		this->aboutWindow->setSize(this->aboutWindow->getSize().width + 1, this->aboutWindow->getSize().height + 1);

		this->aboutWindowWidgets.push_back(hyperTextBox);
		this->aboutWindowWidgets.push_back(aboutOkButton);
		this->aboutWindowWidgets.push_back(this->aboutWindow);
	}

	MyGUI::FloatPoint windowPosition;
	windowPosition.left = 0.3f;
	windowPosition.top = 0.2f;
	
	this->aboutWindow->setRealPosition(windowPosition);
	this->aboutWindow->setVisible(true);
}

void MainMenuBar::showSceneDescriptionWindow(void)
{
	if (0 == this->sceneDescriptionWindowWidgets.size())
	{
		this->sceneDescriptionWindow = MyGUI::Gui::getInstance().createWidget<MyGUI::Window>("WindowC", MyGUI::IntCoord(10, 10, 650, 500), MyGUI::Align::Default, "Popup");
		MyGUI::IntCoord coord = this->sceneDescriptionWindow->getClientCoord();

		MyGUI::HyperTextBox* hyperTextBox = this->sceneDescriptionWindow->createWidget<MyGUI::HyperTextBox>("HyperTextBox", MyGUI::IntCoord(0, 0, coord.width, coord.height - 50), MyGUI::Align::Stretch);
		hyperTextBox->setUrlColour(MyGUI::Colour::White);

		const auto mainGameObjectPtr = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(NOWA::GameObjectController::MAIN_GAMEOBJECT_ID);
		if (nullptr != mainGameObjectPtr)
		{
			// Can only be added once
			auto descriptionCompPtr = NOWA::makeStrongPtr(mainGameObjectPtr->getComponent<NOWA::DescriptionComponent>());
			if (nullptr != descriptionCompPtr)
			{
				hyperTextBox->setCaption(descriptionCompPtr->getDescription());
			}
		}

		hyperTextBox->eventUrlClick += MyGUI::newDelegate(this, &MainMenuBar::onClickUrl);

		MyGUI::Button* sceneDescriptionOkButton = this->sceneDescriptionWindow->createWidget<MyGUI::Button>("Button", MyGUI::IntCoord(0, coord.height - 40, 100, 40), MyGUI::Align::Bottom | MyGUI::Align::HCenter, "sceneDescriptionOkButton");
		sceneDescriptionOkButton->setCaption("Ok");
		sceneDescriptionOkButton->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);
		// Force update layout
		this->sceneDescriptionWindow->setSize(this->sceneDescriptionWindow->getSize().width - 1, this->sceneDescriptionWindow->getSize().height - 1);
		this->sceneDescriptionWindow->setSize(this->sceneDescriptionWindow->getSize().width + 1, this->sceneDescriptionWindow->getSize().height + 1);

		this->sceneDescriptionWindowWidgets.push_back(hyperTextBox);
		this->sceneDescriptionWindowWidgets.push_back(sceneDescriptionOkButton);
		this->sceneDescriptionWindowWidgets.push_back(this->sceneDescriptionWindow);
	}

	MyGUI::FloatPoint windowPosition;
	windowPosition.left = 0.3f;
	windowPosition.top = 0.2f;

	this->sceneDescriptionWindow->setRealPosition(windowPosition);
	this->sceneDescriptionWindow->setVisible(true);
}

void MainMenuBar::onClickUrl(MyGUI::HyperTextBox* sender, const std::string& url)
{
	if (Ogre::String(NOWA_COMPANYDOMAIN_STR) == url)
	{
		ShellExecute(0, 0, NOWA_COMPANYDOMAIN_STR, 0, 0 , SW_SHOW);
	}
	else if (Ogre::String(NOWA_LICENSE_STR_2) == url)
	{
		ShellExecute(0, 0, NOWA_LICENSE_STR_2, 0, 0 , SW_SHOW);
	}
	else if ("https://github.com/TheComet/ik/blob/master/LICENSE" == url)
	{
		ShellExecute(0, 0, "https://github.com/TheComet/ik/blob/master/LICENSE", 0, 0 , SW_SHOW);
	}
	else if ("https://github.com/berndporr/kiss-fft/blob/master/LICENSE" == url)
	{
		ShellExecute(0, 0, "https://github.com/berndporr/kiss-fft/blob/master/LICENSE", 0, 0 , SW_SHOW);
	}
	else if ("https://www.lua.org/source/" == url)
	{
		ShellExecute(0, 0, "https://www.lua.org/source/", 0, 0 , SW_SHOW);
	}
	else if ("http://luabind.sourceforge.net/docs.html" == url)
	{
		ShellExecute(0, 0, "http://luabind.sourceforge.net/docs.html", 0, 0 , SW_SHOW);
	}
	else if ("http://mygui.info/pages/license.html" == url)
	{
		ShellExecute(0, 0, "http://mygui.info/pages/license.html", 0, 0 , SW_SHOW);
	}
	else if ("http://www.gzip.org/zlib/zlib_license.html" == url)
	{
		ShellExecute(0, 0, "http://www.gzip.org/zlib/zlib_license.html", 0, 0 , SW_SHOW);
	}
	else if ("http://www.xiph.org" == url)
	{
		ShellExecute(0, 0, "http://www.xiph.org", 0, 0 , SW_SHOW);
	}
	else if ("https://github.com/facebookarchive/RakNet/blob/master/LICENSE" == url)
	{
		ShellExecute(0, 0, "https://github.com/facebookarchive/RakNet/blob/master/LICENSE", 0, 0 , SW_SHOW);
	}
	else if ("https://en.wikipedia.org/wiki/OpenAL" == url)
	{
		ShellExecute(0, 0, "https://en.wikipedia.org/wiki/OpenAL", 0, 0 , SW_SHOW);
	}
	else
	{
		ShellExecute(0, 0, url.data(), 0, 0, SW_SHOW);
	}
}

#if 0
void MainMenuBar::showComponentPlugin(void)
{
	if (0 == this->createComponentPluginWindowWidgets.size())
	{
		this->createComponentPluginWindow = MyGUI::Gui::getInstance().createWidget<MyGUI::Window>("WindowC", MyGUI::IntCoord(10, 10, 650, 300), MyGUI::Align::Default, "Popup");
		MyGUI::IntCoord coord = this->createComponentPluginWindow->getClientCoord();

		this->createComponentPluginWindow->setCaptionWithReplacing("#{CreateComponentPlugin}");

		MyGUI::TextBox* pluginNameLabel = this->createComponentPluginWindow->createWidget<MyGUI::EditBox>("TextBox", MyGUI::IntCoord(10, 20, 150, 40), MyGUI::Align::Default);
		pluginNameLabel->setCaptionWithReplacing("#{ComponentName}");

		this->pluginNameEdit = this->createComponentPluginWindow->createWidget<MyGUI::EditBox>("EditBox", MyGUI::IntCoord(180, 10, 300, 40), MyGUI::Align::Default);
		this->pluginNameEdit->setOnlyText("MyTestComponent");

		MyGUI::Button* pluginOkButton = this->createComponentPluginWindow->createWidget<MyGUI::Button>("Button", MyGUI::IntCoord(coord.width * 0.5 - 110.0f, coord.height - 50, 100, 40), MyGUI::Align::Default, "pluginOkButton");
		pluginOkButton->setCaption("Ok");
		pluginOkButton->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		MyGUI::Button* pluginAbordButton = this->createComponentPluginWindow->createWidget<MyGUI::Button>("Button", MyGUI::IntCoord(coord.width * 0.5 + 10.0f, coord.height - 50, 100, 40), MyGUI::Align::Default, "pluginAbordButton");
		pluginAbordButton->setCaptionWithReplacing("#{Abord}");
		pluginAbordButton->eventMouseButtonClick += MyGUI::newDelegate(this, &MainMenuBar::buttonHit);

		// Force update layout
		this->createComponentPluginWindow->setSize(this->createComponentPluginWindow->getSize().width - 1, this->createComponentPluginWindow->getSize().height - 1);
		this->createComponentPluginWindow->setSize(this->createComponentPluginWindow->getSize().width + 1, this->createComponentPluginWindow->getSize().height + 1);

		this->createComponentPluginWindowWidgets.push_back(pluginNameLabel);
		this->createComponentPluginWindowWidgets.push_back(this->pluginNameEdit);
		this->createComponentPluginWindowWidgets.push_back(pluginOkButton);
		this->createComponentPluginWindowWidgets.push_back(pluginAbordButton);

		this->createComponentPluginWindowWidgets.push_back(this->createComponentPluginWindow);
	}

	MyGUI::FloatPoint windowPosition;
	windowPosition.left = 0.3f;
	windowPosition.top = 0.2f;
	
	this->createComponentPluginWindow->setRealPosition(windowPosition);
	this->createComponentPluginWindow->setVisible(true);
}
#else
	void MainMenuBar::showComponentPlugin(void)
	{
		if (nullptr == this->cPlusPlusComponentGenerator)
		{
			// Load the layout and get the root widget
			this->componentPluginWindowWidgets = MyGUI::LayoutManager::getInstancePtr()->loadLayout("ComponentGenerator.layout");

			// Assuming the root widget is the first widget in the layout
			MyGUI::Widget* rootWidget = this->componentPluginWindowWidgets.at(0);

			this->cPlusPlusComponentGenerator = new NOWA::CPlusPlusComponentGenerator(rootWidget);
		}
		else
		{
			this->cPlusPlusComponentGenerator->getMainWidget()->setVisible(true);
		}
	}
#endif

void MainMenuBar::activateTestSelectedGameObjects(bool bActivated)
{
	this->bTestSelectedGameObjects = bActivated;
	this->simulationMenuItem->getItemChild()->getItemAt(1)->setStateCheck(this->bTestSelectedGameObjects);
	this->projectManager->getEditorManager()->activateTestSelectedGameObjects(this->bTestSelectedGameObjects);
}

void MainMenuBar::drawNavigationMap(bool bDraw)
{
	this->utilitiesMenuItem->getItemChild()->getItemAt(6)->setStateCheck(bDraw);
	NOWA::AppStateManager::getSingletonPtr()->getOgreRecastModule()->debugDrawNavMesh(bDraw);
}

void MainMenuBar::drawCollisionLines(bool bDraw)
{
	this->utilitiesMenuItem->getItemChild()->getItemAt(7)->setStateCheck(bDraw);
	NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->showOgreNewtCollisionLines(bDraw);
}

void MainMenuBar::toggleMyGUIComponents(bool bToggleMyGUIComponents)
{
	const auto& allMyGuiComponents = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectComponents<NOWA::MyGUIComponent>();

	// Iterate through all MyGUI components
	for (const auto& myGuiComponent : allMyGuiComponents)
	{
		// Check if the component visibility has been stored before
		auto it = this->componentVisibilityMap.find(myGuiComponent.get());
		if (it == this->componentVisibilityMap.end())
		{
			// If not found, store the current visibility state of the component
			this->componentVisibilityMap[myGuiComponent.get()] = myGuiComponent->isActivated();
		}

		bool wasVisible = this->componentVisibilityMap[myGuiComponent.get()];
		if (true == bToggleMyGUIComponents)
		{
			if (true == wasVisible)
			{
				myGuiComponent->setActivated(true);
			}
		}
		else
		{
			myGuiComponent->setActivated(false);
		}
	}
}


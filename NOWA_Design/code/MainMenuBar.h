#ifndef MAIN_MENU_BAR
#define MAIN_MENU_BAR

#include "MyGUI.h"
#include "BaseLayout/BaseLayout.h"
#include "HyperTextBox/HyperTextBox.h"
#include "ConfigPanel.h"
#include "TreeControl/TreeControl.h"
#include "TreeControl/TreeControlItem.h"

#include "utilities/CPlusPlusComponentGenerator.h"

class ProjectManager;

class MainMenuBar : public wraps::BaseLayout
{
public:
	MainMenuBar(ProjectManager* projectManager, MyGUI::Widget* _parent = nullptr);
	virtual ~MainMenuBar();

	void enableMenuEntries(bool enable);

	void enableFileMenu(bool enable);

	void setVisible(bool visible);

	void callNewProject(void);

	void clearLuaErrors(void);

	bool hasLuaErrors(void);

	void showAnalysisWindow(void);

	void showDeployWindow(void);

	void showLuaAnalysisWindow(void);

	void showLuaApiWindow(void);

	void refreshLuaApi(const Ogre::String& filter);

	void refreshMeshes(const Ogre::String& filter);

	void showMeshToolWindow(void);

	void showAboutWindow(void);

	void showComponentPlugin(void);

	void activateTestSelectedGameObjects(bool bActivated);

	void drawNavigationMap(bool bDraw);
private:
	void notifyPopupMenuAccept(MyGUI::MenuControl* sender, MyGUI::MenuItem* item);

	void buttonHit(MyGUI::Widget* sender);

	void editTextChange(MyGUI::Widget* sender);

	void createLuaAnalysisWindow(void);

	void createLuaApiWindow(void);

	void createMeshToolWindow(void);

	void applyMeshToolOperations(void);

	void updateRecentFilesMenu();
private:
	void handleProjectManipulation(NOWA::EventDataPtr eventData);
	void handleSceneValid(NOWA::EventDataPtr eventData);
	void handleProjectEncoded(NOWA::EventDataPtr eventData);
	void handleLuaError(NOWA::EventDataPtr eventData);
	void notifyInsideKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char ch, const std::string& selectedText);
	void onClickUrl(MyGUI::HyperTextBox* sender, const std::string& url);

	enum FileMenuId
	{
		NEW = 0,
		OPEN = 1,
		SAVE = 2,
		SAVE_AS = 3,
		SETTINGS = 4,
		CREATE_COMPONENT_PLUGIN = 5,
		CREATE_CPLUS_PLUS_PROJECT = 6,
		OPEN_LOG = 7,
		COPY_SCENE = 8
	};
	
	enum EditMenuId
	{
		SAVE_GROUP = 0,
		LOAD_GROUP,
		LOAD_MESH_RESOURCE,
		SAVE_DATABLOCKS,
		OPEN_PROJECT_FOLDER,
		START_GAME,
		SEPARATOR,
		UNDO,
		REDO,
		SELECTION_UNDO,
		SELECTION_REDO
	};
private:
	ProjectManager* projectManager;
	ConfigPanel* configPanel;
	MyGUI::MenuBar* mainMenuBar;
	MyGUI::MenuItem* fileMenuItem;
	MyGUI::MenuItem* editMenuItem;
	MyGUI::MenuItem* cameraMenuItem;
	MyGUI::MenuItem* utilitiesMenuItem;
	MyGUI::MenuItem* simulationMenuItem;
	MyGUI::MenuItem* helpMenuItem;

	MyGUI::VectorWidgetPtr analysisWidgets;
	MyGUI::Window* analysisWindow;
	MyGUI::VectorWidgetPtr deployWidgets;
	MyGUI::Window* deployWindow;
	MyGUI::VectorWidgetPtr luaAnalysisWidgets;
	MyGUI::Window* luaAnalysisWindow;
	MyGUI::VectorWidgetPtr aboutWindowWidgets;
	MyGUI::Window* aboutWindow;
	MyGUI::VectorWidgetPtr createComponentPluginWindowWidgets;
	MyGUI::Window* createComponentPluginWindow;
	MyGUI::VectorWidgetPtr componentPluginWindowWidgets;
	NOWA::CPlusPlusComponentGenerator* cPlusPlusComponentGenerator;
	MyGUI::VectorWidgetPtr luaApiWidgets;
	MyGUI::Window* luaApiWindow;
	MyGUI::VectorWidgetPtr meshToolWidgets;
	MyGUI::Window* meshToolWindow;

	MyGUI::EditBox* keyEdit;
	
	MyGUI::HyperTextBox* luaErrorEdit;
	MyGUI::Button* luaAnalysisButton;
	Ogre::String luaErrors;
	unsigned int errorCount;
	bool luaErrorFirstTime;

	MyGUI::EditBox* luaApiSearchEdit;
	NOWA::AutoCompleteSearch luaApiAutoCompleteSearch;
	MyGUI::TreeControl* luaApiTree;
	MyGUI::Button* luaApiButton;

	MyGUI::ComboBox* meshSearchCombo;
	NOWA::AutoCompleteSearch meshAutoCompleteSearch;

	MyGUI::Window* simulationWindow;

	MyGUI::TextBox* resultLabel;

	MyGUI::EditBox* pluginNameEdit;

	MyGUI::MenuItem* recentFileItem[10];
	bool bDrawNavigationMesh;
	bool bTestSelectedGameObjects;
};

#endif

#ifndef DESIGN_STATE_H
#define DESIGN_STATE_H

#include "NOWA.h"
#include "PropertiesPanel.h"
#include "ResourcesPanel.h"
#include "ComponentsPanel.h"
#include "MainMenuBar.h"
#include "ProjectManager.h"

class DesignState : public NOWA::AppState
{
public:
	DECLARE_APPSTATE_CLASS(DesignState)

	DesignState();

	virtual ~DesignState() { }

	virtual void enter(void) override;

	virtual void exit(void) override;

	virtual void update(Ogre::Real dt) override;

	virtual void lateUpdate(Ogre::Real dt) override;

	virtual bool keyPressed(const OIS::KeyEvent& keyEventRef) override;

	virtual bool keyReleased(const OIS::KeyEvent& keyEventRef) override;

	void processUnbufferedKeyInput(Ogre::Real dt);

	void processUnbufferedMouseInput(Ogre::Real dt);

	virtual bool mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;

	virtual bool mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id) override;

	virtual bool mouseMoved(const OIS::MouseEvent& evt) override;

	virtual bool axisMoved(const OIS::JoyStickEvent& evt, int axis) override;

	virtual bool buttonPressed(const OIS::JoyStickEvent& evt, int button) override;

	virtual bool buttonReleased(const OIS::JoyStickEvent& evt, int button) override;

	virtual bool sliderMoved(const OIS::JoyStickEvent& evt, int index) override;

	virtual bool povMoved(const OIS::JoyStickEvent& evt, int pov) override;

	virtual bool vector3Moved(const OIS::JoyStickEvent& evt, int index) override;

private:
	void createScene(void);

	void setupMyGUIWidgets(void);

	void itemSelected(MyGUI::ComboBox* sender, size_t index);
	void buttonHit(MyGUI::Widget* sender);
	void mouseClicked(MyGUI::Widget* sender);
	void notifyEditSelectAccept(MyGUI::EditBox* sender);
	void notifyToolTip(MyGUI::Widget* sender, const MyGUI::ToolTipInfo& info);
	void notifyMessageBoxEnd(MyGUI::Message* sender, MyGUI::MessageBoxStyle result);
	void notifyMessageBoxEndExit(MyGUI::Message* sender, MyGUI::MessageBoxStyle result);
	void setFocus(MyGUI::Widget* sender, MyGUI::Widget* oldWidget);
	void wakeSleepGameObjects(bool allGameObjects, bool sleep);
	void simulate(bool pause, bool withUndo);
	void generateCategories(void);
	void updateInfo(Ogre::Real dt);
	void orbitCamera(Ogre::Real dt);
	void removeGameObjects(void);
	void cloneGameObjects(void);
	void enableWidgets(bool enable);
	void showDebugCollisionLines(bool show);

	void showContextMenu(int mouseX, int mouseY);
	void onMenuItemSelected(MyGUI::MenuCtrl* menu, MyGUI::MenuItem* item);
private:
	// Event delegates
	void handleGenerateCategoriesDelegate(NOWA::EventDataPtr eventData);
	void handleStopSimulation(NOWA::EventDataPtr eventData);
	void handleExit(NOWA::EventDataPtr eventData);
	void handleProjectManipulation(NOWA::EventDataPtr eventData);
	void handleEditorMode(NOWA::EventDataPtr eventData);
	void handleSceneValid(NOWA::EventDataPtr eventData);
	void handleFeedback(NOWA::EventDataPtr eventData);
	void handlePlayerInControl(NOWA::EventDataPtr eventData);
	void handleSceneLoaded(NOWA::EventDataPtr eventData);
	void handleTestSelectedGameObjects(NOWA::EventDataPtr eventData);
	void handleMyGUIWidgetSelected(NOWA::EventDataPtr eventData);
	void handleSceneModified(NOWA::EventDataPtr eventData);
	void handleTerraChanged(NOWA::EventDataPtr eventData);
	void handleEventDataGameObjectMadeGlobal(NOWA::EventDataPtr eventData);
private:
	ProjectManager* projectManager;
	OgreNewt::World* ogreNewt;

	NOWA::EditorManager* editorManager;
	MyGUI::VectorWidgetPtr widgetsSimulation;
	MyGUI::VectorWidgetPtr widgetsManipulation;
	MyGUI::Window* manipulationWindow;
	MyGUI::Window* simulationWindow;
	MyGUI::Button* gridButton;
	MyGUI::ComboBox* gridValueComboBox;
	MyGUI::ComboBox* categoriesComboBox;
	MyGUI::Button* selectModeCheck;
	MyGUI::Button* placeModeCheck;
	MyGUI::PopupMenu* placeModePopupMenu;
	MyGUI::PopupMenu* translateModePopupMenu;
	MyGUI::MenuCtrl* editPopupMenu;
	MyGUI::Button* translateModeCheck;
	MyGUI::Button* pickModeCheck;
	MyGUI::Button* scaleModeCheck;
	MyGUI::Button* rotate1ModeCheck;
	MyGUI::Button* rotate2ModeCheck;
	MyGUI::Button* terrainModifyModeCheck;
	MyGUI::Button* terrainSmoothModeCheck;
	MyGUI::Button* terrainPaintModeCheck;
	MyGUI::Button* wakeButton;
	MyGUI::Button* sleepButton;
	MyGUI::Button* removeButton;
	MyGUI::Button* copyButton;
	MyGUI::Button* focusButton;
	MyGUI::Button* undoButton;
	MyGUI::Button* redoButton;
	MyGUI::Button* cameraUndoButton;
	MyGUI::Button* cameraRedoButton;
	MyGUI::Button* selectUndoButton;
	MyGUI::Button* selectRedoButton;
	MyGUI::Button* cameraSpeedUpButton;
	MyGUI::Button* cameraSpeedDownButton;
	MyGUI::Button* playButton;
	MyGUI::EditBox* findObjectEdit;
	MyGUI::Button* cameraResetButton;
	MyGUI::EditBox* constraintAxisEdit;
	bool simulating;
	Ogre::String activeCategory;

	PropertiesPanel* propertiesPanel;
	ResourcesPanel* resourcesPanel;
	ComponentsPanel* componentsPanel;
	Ogre::Real nextInfoUpdate;

	MainMenuBar* mainMenuBar;

	bool validScene;
	bool hasSceneChanges;
	Ogre::Real cameraMoveSpeed;
	Ogre::String description;

	Ogre::Vector2 lastOrbitValue;
	bool firstTimeValueSet;

	bool playerInControl;
	bool undoPressed;

	std::set<unsigned long> oldSelectedGameObjectIds;
	Ogre::RaySceneQuery* selectQuery;
	Ogre::String selectedMovableObjectInfo;
};

#endif

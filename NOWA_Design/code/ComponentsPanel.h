#ifndef COMPONENTS_PANEL_H
#define COMPONENTS_PANEL_H

#include "BaseLayout/BaseLayout.h"
#include "PanelView/BasePanelView.h"
#include "PanelView/BasePanelViewItem.h"

class ComponentsPanelCell : public wraps::BasePanelViewCell
{
public:
	ComponentsPanelCell(MyGUI::Widget* parent)
		: BasePanelViewCell("ComponentsPanelCell.layout", parent)
	{
		assignWidget(mTextCaption, "text_Caption");
		assignWidget(mWidgetClient, "widget_Client");
		// mTextCaption->eventMouseButtonDoubleClick += MyGUI::newDelegate(this, &ComponentsPanelCell::notifyMouseButtonDoubleClick);
	}
};

/////////////////////////////////////////////////////////////////////////

class ComponentsPanelView : public wraps::BasePanelView<ComponentsPanelCell>
{
public:
	ComponentsPanelView(MyGUI::Widget* parent) :
		wraps::BasePanelView<ComponentsPanelCell>("", parent)
	{
		
	}
};

///////////////////////////////////////////////////////////////////////

class ComponentsPanelSearch;
class ComponentsPanelInfo;
class ComponentsPanelDynamic;

class ComponentsPanel : public wraps::BaseLayout
{
public:
	ComponentsPanel(const MyGUI::FloatCoord& coords);

	void setEditorManager(NOWA::EditorManager* editorManager);

	void clearComponents(void);

	void showComponents(void);

	void destroyContent(void);

	void setVisible(bool show);

	void notifyWindowButtonPressed(MyGUI::Window* sender, const std::string& button);
private:
	// Event delegates
	void handleShowComponentsPanel(NOWA::EventDataPtr eventData);
private:
	NOWA::EditorManager* editorManager;
	ComponentsPanelView* componentsPanelView;
	ComponentsPanelSearch* componentsPanelSearch;
	ComponentsPanelInfo* componentsPanelInfo;
	ComponentsPanelDynamic* componentsPanelDynamic;
};

////////////////////////////////////////////////////////////////////////

class ComponentsPanelSearch : public wraps::BasePanelViewItem
{
public:
	ComponentsPanelSearch(ComponentsPanelDynamic* componentsPanelDynamic);

	virtual void initialise();
	virtual void shutdown();
private:
	virtual void editTextChange(MyGUI::Widget* sender);
private:
	MyGUI::EditBox* componentSearchEdit;
	ComponentsPanelDynamic* componentsPanelDynamic;
};

////////////////////////////////////////////////////////////////////////

class ComponentsPanelInfo : public wraps::BasePanelViewItem
{
public:
	ComponentsPanelInfo();

	void setInfo(const Ogre::String& info);

	virtual void initialise();
	virtual void shutdown();
private:
	MyGUI::EditBox* componentsInfo;
};

///////////////////////////////////////////////////////////////////////

class ComponentsPanelDynamic : public wraps::BasePanelViewItem
{
public:
	ComponentsPanelDynamic(const std::vector<NOWA::GameObject*>& gameObjects, const Ogre::String& name, ComponentsPanel* parentPanel);

	virtual ~ComponentsPanelDynamic();

	void setEditorManager(NOWA::EditorManager* editorManager);
	void setComponentsPanelInfo(ComponentsPanelInfo* componentsPanelInfo);

	virtual void initialise() override;
	virtual void shutdown() override;
	
	void showComponents(const Ogre::String& searchText = Ogre::String());
	void clear(void);
	
	void buttonHit(MyGUI::Widget* sender);
private:
	NOWA::EditorManager* editorManager;
	const std::vector<NOWA::GameObject*> gameObjects;
	Ogre::String name;
	MyGUI::VectorWidgetPtr componentsButtons;
	int heightCurrent;
	ComponentsPanelInfo* componentsPanelInfo;
	ComponentsPanel* parentPanel;
	NOWA::AutoCompleteSearch autoCompleteSearch;
};

#endif
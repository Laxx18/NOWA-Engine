#ifndef RESOURCES_PANEL_H
#define RESOURCES_PANEL_H

#include "BaseLayout/BaseLayout.h"
#include "PanelView/BasePanelView.h"
#include "PanelView/BasePanelViewItem.h"
#include "TreeControl/TreeControl.h"
#include "TreeControl/TreeControlItem.h"
#if 0
#include "Ogre/RenderBox/RenderBox.h"
#include "Ogre/RenderBox/RenderBoxScene.h"
#endif

class ProjectManager;

class ResourcesPanelCell : public wraps::BasePanelViewCell
{
public:
	ResourcesPanelCell(MyGUI::Widget* parent)
		: BasePanelViewCell("ResourcesPanelCell.layout", parent)
	{
		assignWidget(mTextCaption, "text_Caption");
		assignWidget(this->buttonMinimize, "button_Minimize");
		assignWidget(mWidgetClient, "widget_Client");
		mTextCaption->eventMouseButtonDoubleClick += MyGUI::newDelegate(this, &ResourcesPanelCell::notifyMouseButtonDoubleClick);
		this->buttonMinimize->eventMouseButtonPressed += MyGUI::newDelegate(this, &ResourcesPanelCell::notfyMouseButtonPressed);
	}

	virtual void setMinimized(bool _minimized)
	{
		wraps::BasePanelViewCell::setMinimized(_minimized);
		this->buttonMinimize->setStateSelected(isMinimized());
	}

private:
	void notfyMouseButtonPressed(MyGUI::Widget* sender, int left, int top, MyGUI::MouseButton id)
	{
		if (id == MyGUI::MouseButton::Left)
		{
			setMinimized(!isMinimized());
		}
	}

	void notifyMouseButtonDoubleClick(MyGUI::Widget* sender)
	{
		setMinimized(!isMinimized());
	}

private:
	MyGUI::Button* buttonMinimize;
};

/////////////////////////////////////////////////////////////////////////

class ResourcesPanelView : public wraps::BasePanelView<ResourcesPanelCell>
{
public:
	ResourcesPanelView(MyGUI::Widget* parent) :
		wraps::BasePanelView<ResourcesPanelCell>("", parent)
	{
		
	}
};

///////////////////////////////////////////////////////////////////////

class ResourcesPanelMeshes;
class ResourcesPanelGameObjects;
#if 0
class ResourcesPanelMeshPreview;
#endif
class ResourcesPanelDataBlocks;
class ResourcesPanelTextures;
class ResourcesPanelProject;
class ResourcesPanelLuaScript;

class ResourcesPanel : public wraps::BaseLayout
{
public:
	ResourcesPanel(const MyGUI::FloatCoord& coords);

	void setEditorManager(NOWA::EditorManager* editorManager);

	void setProjectManager(ProjectManager* projectManager);

	void destroyContent(void);

	void setVisible(bool show);

	void refresh(void);

	void clear(void);
private:
	NOWA::EditorManager* editorManager;
	ResourcesPanelView* resourcesPanelView1;
	ResourcesPanelView* resourcesPanelView2;
	ResourcesPanelView* resourcesPanelView3;
	ResourcesPanelMeshes* resourcesPanelMeshes;
	ResourcesPanelGameObjects* resourcesPanelGameObjects;
	ResourcesPanelDataBlocks* resourcesPanelDataBlocks;
	ResourcesPanelTextures* resourcesPanelTextures;
	ResourcesPanelProject* resourcesPanelProject;
	ResourcesPanelLuaScript* resourcesPanelLuaScript;
#if 0
	ResourcesPanelMeshPreview* resourcesPanelMeshPreview;
#endif
};

////////////////////////////////////////////////////////////////////////

class ResourcesPanelMeshes : public wraps::BasePanelViewItem
{
public:
	ResourcesPanelMeshes();

	void setEditorManager(NOWA::EditorManager* editorManager);

	virtual void initialise();
	virtual void shutdown();

	void clear(void);

	void notifyTreeNodePrepare(MyGUI::TreeControl* treeControl, MyGUI::TreeControl::Node* node);
	void notifyTreeNodeSelected(MyGUI::TreeControl* treeControl, MyGUI::TreeControl::Node* node);
	void editTextChange(MyGUI::Widget* sender);

private:
	void loadMeshes(const Ogre::String& filter);
	// Event delegates
	void handleRefreshMeshResources(NOWA::EventDataPtr eventData);
private:
	MyGUI::EditBox* resourcesSearchEdit;
	NOWA::AutoCompleteSearch autoCompleteSearch;
	MyGUI::TreeControl* meshesTree;
	NOWA::EditorManager* editorManager;
	MyGUI::ImageBox* imageBox;
};

///////////////////////////////////////////////////////////////////////

#if 0
// canvas is not ported for ogre2.1!
class ResourcesPanelMeshPreview : public wraps::BasePanelViewItem
{
public:
	ResourcesPanelMeshPreview();

	void setEditorManager(NOWA::EditorManager* editorManager);

	virtual void initialise();
	virtual void shutdown();

	void clear(void);
private:
	void actualizeMesh(void);
private:
	NOWA::EditorManager* editorManager;
	wraps::RenderBox renderBox;
	wraps::RenderBoxScene renderBoxScene;
	MyGUI::Window* previewWindow;
};
#endif

///////////////////////////////////////////////////////////////////////

class ResourcesPanelGameObjects : public wraps::BasePanelViewItem
{
public:
	ResourcesPanelGameObjects();

	void setEditorManager(NOWA::EditorManager* editorManager);

	virtual void initialise();
	virtual void shutdown();

	void clear(void);
	void refresh(const Ogre::String& filter);
protected:
	void notifyTreeNodeSelected(MyGUI::TreeControl* treeControl, MyGUI::TreeControl::Node* node);
	void keyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char _char);
	void editTextChange(MyGUI::Widget* sender);
	void mouseLostFocus(MyGUI::Widget* sender, MyGUI::Widget* oldWidget);
	void mouseRootChangeFocus(MyGUI::Widget* sender, bool bFocus);
	void onMouseClick(MyGUI::Widget* sender);
private:
	// Event delegates
	void handleRefreshGameObjectsPanel(NOWA::EventDataPtr eventData);

	void selectAllNodes(MyGUI::TreeControl* treeControl);
private:
	MyGUI::EditBox* resourcesSearchEdit;
	NOWA::AutoCompleteSearch autoCompleteSearch;
	MyGUI::TreeControl* gameObjectsTree;
	NOWA::EditorManager* editorManager;
	MyGUI::ImageBox* imageBox;
	Ogre::String oldSelectedText;
	bool ctrlPressed;
	bool selectAll;
};

///////////////////////////////////////////////////////////////////////

class ResourcesPanelDataBlocks : public wraps::BasePanelViewItem
{
public:
	ResourcesPanelDataBlocks();

	void setEditorManager(NOWA::EditorManager* editorManager);

	virtual void initialise();
	virtual void shutdown();

	void clear(void);

	void notifyTreeNodeSelected(MyGUI::TreeControl* treeControl, MyGUI::TreeControl::Node* node);
	void notifyKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char ch);
	void notifyTreeContextMenu(MyGUI::TreeControl* treeControl, MyGUI::TreeControl::Node* node);
	void editTextChange(MyGUI::Widget* sender);
private:
	void loadDataBlocks(const Ogre::String& filter);
private:
	MyGUI::EditBox* resourcesSearchEdit;
	NOWA::AutoCompleteSearch autoCompleteSearch;
	NOWA::EditorManager* editorManager;
	MyGUI::TreeControl* dataBlocksTree;
	Ogre::String selectedText;
	MyGUI::ImageBox* datablockPreview;
};

///////////////////////////////////////////////////////////////////////

class ResourcesPanelTextures : public wraps::BasePanelViewItem
{
public:
	ResourcesPanelTextures();

	void setEditorManager(NOWA::EditorManager* editorManager);

	virtual void initialise();
	virtual void shutdown();

	void clear(void);

	void notifyTreeNodeSelected(MyGUI::TreeControl* treeControl, MyGUI::TreeControl::Node* node);
	void notifyKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char ch);
	void notifyTreeContextMenu(MyGUI::TreeControl* treeControl, MyGUI::TreeControl::Node* node);
	void editTextChange(MyGUI::Widget* sender);
private:
	void loadTextures(const Ogre::String& filter);
private:
	MyGUI::EditBox* resourcesSearchEdit;
	NOWA::AutoCompleteSearch autoCompleteSearch;
	NOWA::EditorManager* editorManager;
	MyGUI::TreeControl* texturesTree;
	Ogre::String selectedText;
	MyGUI::ImageBox* texturePreview;
};

///////////////////////////////////////////////////////////////////////

class ResourcesPanelProject : public wraps::BasePanelViewItem
{
public:
	ResourcesPanelProject();
	virtual ~ResourcesPanelProject();

	void setEditorManager(NOWA::EditorManager* editorManager);
	void setProjectManager(ProjectManager* projectManager);

	void initialise(void) override;
	void shutdown(void) override;
	void clear(void);

	void populateFilesTree(const Ogre::String& folderPath);
private:
	void handleSingleClick(MyGUI::TreeControl::Node* node);
	void handleDoubleClick(MyGUI::TreeControl::Node* node);

	void sortTreeNodesByName(MyGUI::TreeControl::Node* parentNode);
	// void sortTreeNodesByExtension(MyGUI::TreeControl::Node* parentNode);
private:
	void handleEventDataResourceCreated(NOWA::EventDataPtr eventData);
	void handleSceneModified(NOWA::EventDataPtr eventData);
	void handleEventDataGameObjectMadeGlobal(NOWA::EventDataPtr eventData);
private:
	void notifyKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char ch);
	void notifyTreeNodeClick(MyGUI::TreeControl* sender, MyGUI::TreeControl::Node* node);
	void notifyTreeNodeDoubleClick(MyGUI::TreeControl* sender, MyGUI::TreeControl::Node* node);
	void notifyMessageBoxEndLoad(MyGUI::Message* sender, MyGUI::MessageBoxStyle result);
private:
	MyGUI::TreeControl* filesTreeControl;
	NOWA::EditorManager* editorManager;
	ProjectManager* projectManager;

	std::string selectedText;
	std::chrono::steady_clock::time_point lastClickTime;
	MyGUI::TreeControl::Node* lastClickedNode;
	bool hasSceneChanges;
	Ogre::String currentSceneName;
};

///////////////////////////////////////////////////////////////////////

class ResourcesPanelLuaScript : public wraps::BasePanelViewItem
{
public:
	ResourcesPanelLuaScript();
	virtual ~ResourcesPanelLuaScript();

	void setEditorManager(NOWA::EditorManager* editorManager);
	void setProjectManager(ProjectManager* projectManager);

	void initialise(void) override;
	void shutdown(void) override;
	void clear(void);

	void populateListBox(void);
private:
	void buttonHit(MyGUI::Widget* sender);

	void handleLuaScriptModified(NOWA::EventDataPtr eventData);
private:
	MyGUI::ListBox* listBox;
	MyGUI::Button* upButton;
	MyGUI::Button* downButton;
	NOWA::EditorManager* editorManager;
	ProjectManager* projectManager;

	std::string selectedText;
};

#endif

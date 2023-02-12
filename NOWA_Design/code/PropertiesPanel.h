#ifndef PROPERTIES_PANEL_H
#define PROPERTIES_PANEL_H

#include "BaseLayout/BaseLayout.h"
#include "PanelView/BasePanelView.h"
#include "PanelView/BasePanelViewItem.h"
#include "ColourPanelManager.h"
#include "OpenSaveFileDialogExtended.h"

class ImageData
{
public:
	ImageData();

	~ImageData();

	void setResourceName(const Ogre::String& resourceName);

	bool isEmpty() const;

	MyGUI::ResourceImageSetPtr getResourceImagePtr(void) const;

	void setImageBoxBack(MyGUI::ImageBox* imageBack);

	void setImageBoxItem(MyGUI::ImageBox* imageItem);


	MyGUI::ImageBox* getImageBoxBack(void) const;

	MyGUI::ImageBox* getImageBoxItem(void) const;

private:
	Ogre::String resourceName;

	MyGUI::ResourceImageSetPtr resourceImage;
	MyGUI::ImageBox* imageBack;
	MyGUI::ImageBox* imageItem;
};

//////////////////////////////////////////////////////////////////)

class PropertiesPanelCell : public wraps::BasePanelViewCell
{
public:
	PropertiesPanelCell(MyGUI::Widget* parent)
		: BasePanelViewCell("PropertiesPanelCell.layout", parent),
		component(nullptr)
	{
		assignWidget(mTextCaption, "text_Caption");
		assignWidget(this->buttonMinimize, "button_Minimize");
		assignWidget(mWidgetClient, "widget_Client");
		mTextCaption->eventMouseButtonDoubleClick += MyGUI::newDelegate(this, &PropertiesPanelCell::notifyMouseButtonDoubleClick);
		this->buttonMinimize->eventMouseButtonPressed += MyGUI::newDelegate(this, &PropertiesPanelCell::notfyMouseButtonPressed);
	}

	virtual void setMinimized(bool _minimized)
	{
		wraps::BasePanelViewCell::setMinimized(_minimized);
		this->buttonMinimize->setStateSelected(isMinimized());

		// NOWA::GameObjectComponent* tempComponent = reinterpret_cast<NOWA::GameObjectComponent*>(Ogre::StringConverter::parseSizeT(strComponentAdress));
		NOWA::GameObjectComponent** tempComponent = this->getMainWidget()->getUserData<NOWA::GameObjectComponent*>(false);

		if (tempComponent != nullptr)
		{
			this->component = *tempComponent;
		}

		if (nullptr != this->component)
		{
			this->component->setIsExpanded(false == isMinimized());
		}
	}

private:
	void notfyMouseButtonPressed(MyGUI::Widget* sender, int left, int top, MyGUI::MouseButton id)
	{
		if (this->buttonMinimize == sender)
		{
			if (id == MyGUI::MouseButton::Left)
			{
				setMinimized(!isMinimized());
			}
		}
	}

	void notifyMouseButtonDoubleClick(MyGUI::Widget* sender)
	{
		setMinimized(!isMinimized());
	}

private:
	MyGUI::Button* buttonMinimize;
	NOWA::GameObjectComponent* component;
};

/////////////////////////////////////////////////////////////////////////

class PropertiesPanelView : public wraps::BasePanelView<PropertiesPanelCell>
{
public:
	PropertiesPanelView(MyGUI::Widget* parent) :
		wraps::BasePanelView<PropertiesPanelCell>("", parent)
	{
		
	}
};

///////////////////////////////////////////////////////////////////////

class PropertiesPanelDirector;
class PropertiesPanelDynamic;
class PropertiesPanelInfo;

class PropertiesPanel : public wraps::BaseLayout
{
public:
	PropertiesPanel(const MyGUI::FloatCoord& coords);

	void setEditorManager(NOWA::EditorManager* editorManager);

	void clearProperties(void);

	void destroyContent(void);

	void setVisible(bool show);

	void showProperties(std::vector<NOWA::GameObject*> gameObjects);

	void showProperties(void);

	PropertiesPanelDynamic* getPropertiesPanelItem(size_t index);
public:
	static void setShowPropertiesFlag(bool bShowProperties);

	static bool getShowPropertiesFlag(void);
private:
	// Event delegates
	void handleRefreshPropertiesPanel(NOWA::EventDataPtr eventData);
	void onMouseWheel(MyGUI::Widget* sender, int rel);
	void onMouseRelease(MyGUI::Widget* sender, int left, int top, MyGUI::MouseButton id);
private:
	static bool bShowProperties;
private:
	NOWA::EditorManager* editorManager;
	PropertiesPanelView* propertiesPanelView1;
	PropertiesPanelView* propertiesPanelView2;
	PropertiesPanelDirector* propertiesPanelDirector;
	PropertiesPanelInfo* propertiesPanelInfo;
	OpenSaveFileDialogExtended* openSaveFileDialog;
};

////////////////////////////////////////////////////////////////////////

class PropertiesPanelInfo : public wraps::BasePanelViewItem
{
public:
	PropertiesPanelInfo();

	void setInfo(const Ogre::String& info);
	
	void listData(NOWA::GameObject* gameObject);

	virtual void initialise();
	virtual void shutdown();
private:
	MyGUI::EditBox* propertyInfo;
	MyGUI::VectorWidgetPtr itemsText;
	MyGUI::VectorWidgetPtr itemsEdit;
	int heightCurrent;
};

///////////////////////////////////////////////////////////////////////

class PropertiesPanelDynamic : public wraps::BasePanelViewItem
{
public:
	PropertiesPanelDynamic(const std::vector<NOWA::GameObject*>& gameObjects, const Ogre::String& name);

	virtual ~PropertiesPanelDynamic();

	void setEditorManager(NOWA::EditorManager* editorManager);
	void setPropertiesPanelInfo(PropertiesPanelInfo* propertiesPanelInfo);

	virtual void initialise() override;
	virtual void shutdown() override;
	virtual void notifyEditSelectAccept(MyGUI::EditBox* sender) = 0;
	virtual void buttonHit(MyGUI::Widget* sender) = 0;
	virtual void setFocus(MyGUI::Widget* sender, MyGUI::Widget* oldWidget);
	virtual void mouseLostFocus(MyGUI::Widget* sender, MyGUI::Widget* oldWidget);
	virtual void mouseRootChangeFocus(MyGUI::Widget* sender, bool bFocus);
	virtual void editTextChange(MyGUI::Widget* sender);
	virtual void onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode code, MyGUI::Char c);
	virtual void notifyComboChangedPosition(MyGUI::ComboBox* sender, size_t index) = 0;
	virtual void notifyColourAccept(MyGUI::ColourPanel* sender) = 0;
	virtual void notifyEndDialog(tools::Dialog* sender, bool result) = 0;
	virtual void notifyScrollChangePosition(MyGUI::ScrollBar* sender, size_t position) = 0;
	virtual void notifySliderMouseRelease(MyGUI::Widget* sender, int x, int y, MyGUI::MouseButton button) = 0;
	virtual void notifySetItemBoxData(MyGUI::ItemBox* sender, const Ogre::String& resourceName) = 0;
	void onMouseDoubleClick(MyGUI::Widget* sender);
	void onMouseClick(MyGUI::Widget* sender, int left, int top, MyGUI::MouseButton id);
	void notifyColourCancel(MyGUI::ColourPanel* sender);
	void requestCoordItem(MyGUI::ItemBox* sender, MyGUI::IntCoord& coord, bool drag);
	void requestCreateWidgetItem(MyGUI::ItemBox* sender, MyGUI::Widget* item);
	void requestDrawItem(MyGUI::ItemBox* sender, MyGUI::Widget* item, const MyGUI::IBDrawItemInfo& info);

	bool showFileOpenDialog(const Ogre::String& action, const Ogre::String& fileMask, Ogre::String& resourceGroupName);

	void setVisibleCount(unsigned int count);
	void addProperty(const Ogre::String& name, NOWA::Variant* attribute, bool allValuesSame);

	void createRealSlider(const int& valueWidth, const int& valueLeft, const int& height, const Ogre::String& name, NOWA::Variant*& attribute);

	void createIntSlider(const int& valueWidth, const int& valueLeft, const int& height, const Ogre::String& name, NOWA::Variant*& attribute);

	MyGUI::Widget* addSeparator(void);
protected:
	void showDescription(MyGUI::Widget* sender);
protected:
	NOWA::EditorManager* editorManager;
	const std::vector<NOWA::GameObject*> gameObjects;
	NOWA::GameObject* gameObject;
	Ogre::String name;
	MyGUI::VectorWidgetPtr itemsText;
	MyGUI::VectorWidgetPtr itemsEdit;
	int heightCurrent;
	PropertiesPanelInfo* propertiesPanelInfo;
	OpenSaveFileDialogExtended* openSaveFileDialog;
};

/////////////////////////////////////////////////////////////////////////

class PropertiesPanelGameObject : public PropertiesPanelDynamic
{
public:
	PropertiesPanelGameObject( const std::vector<NOWA::GameObject*>& gameObjects, const Ogre::String& name);

	virtual ~PropertiesPanelGameObject();

	virtual void initialise() override;

	virtual void notifyEditSelectAccept(MyGUI::EditBox* sender) override;
	virtual void buttonHit(MyGUI::Widget* sender) override;
	virtual void notifyComboChangedPosition(MyGUI::ComboBox* sender, size_t index) override;
	virtual void notifyColourAccept(MyGUI::ColourPanel* sender) override;
	virtual void notifyEndDialog(tools::Dialog* sender, bool result) override;
	virtual void notifyScrollChangePosition(MyGUI::ScrollBar* sender, size_t position) override;
	virtual void notifySliderMouseRelease(MyGUI::Widget* sender, int x, int y, MyGUI::MouseButton button) override;
	virtual void notifySetItemBoxData(MyGUI::ItemBox* sender, const Ogre::String& resourceName) override;

	void notifyMouseAddComponentClick(MyGUI::Widget* sender);
private:
	void addComponent(MyGUI::MenuItem* menuItem);
	void setNewAttributeValue(MyGUI::EditBox* sender, NOWA::Variant* attribute);
private:
	MyGUI::Button* listComponentsButton;
	std::vector<MyGUI::PopupMenu*> componentsPopupMenus;
	unsigned short maxNumComponentsForPage;
};

/////////////////////////////////////////////////////////////////////////

class PropertiesPanelComponent : public PropertiesPanelDynamic
{
public:
	PropertiesPanelComponent(const std::vector<NOWA::GameObject*>& gameObjects, const std::vector<NOWA::GameObjectComponent*>& gameObjectComponents, const Ogre::String& name);

	virtual ~PropertiesPanelComponent();

	virtual void initialise() override;

	virtual void notifyEditSelectAccept(MyGUI::EditBox* sender) override;
	virtual void buttonHit(MyGUI::Widget* sender) override;
	virtual void notifyComboChangedPosition(MyGUI::ComboBox* sender, size_t index) override;
	virtual void notifyColourAccept(MyGUI::ColourPanel* sender) override;
	virtual void notifyEndDialog(tools::Dialog* sender, bool result) override;
	virtual void notifyScrollChangePosition(MyGUI::ScrollBar* sender, size_t position) override;
	virtual void notifySliderMouseRelease(MyGUI::Widget* sender, int x, int y, MyGUI::MouseButton button) override;
	virtual void notifySetItemBoxData(MyGUI::ItemBox* sender, const Ogre::String& resourceName) override;
protected:
	void setNewAttributeValue(MyGUI::EditBox* sender, NOWA::Variant* attribute);
protected:
	std::vector<NOWA::GameObjectComponent*> gameObjectComponents;
	MyGUI::Button* appendComponentButton;
	MyGUI::Button* deleteComponentButton;
	MyGUI::Button* debugDataComponentButton;
	MyGUI::Button* moveUpComponentButton;
	MyGUI::Button* moveDownComponentButton;
};

#endif
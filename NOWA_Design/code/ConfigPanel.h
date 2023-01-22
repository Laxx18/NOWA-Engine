#ifndef CONFIG_PANEL_H
#define CONFIG_PANEL_H

#include "BaseLayout/BaseLayout.h"
#include "PanelView/BasePanelView.h"
#include "PanelView/BasePanelViewItem.h"
#include "ColourPanelManager.h"

class ProjectManager;

class ConfigPanelCell : public wraps::BasePanelViewCell
{
public:
	ConfigPanelCell(MyGUI::Widget* parent)
		: BasePanelViewCell("ConfigPanelCell.layout", parent)
	{
		assignWidget(mTextCaption, "text_Caption");
		assignWidget(this->buttonMinimize, "button_Minimize");
		assignWidget(mWidgetClient, "widget_Client");
		mTextCaption->eventMouseButtonDoubleClick += MyGUI::newDelegate(this, &ConfigPanelCell::notifyMouseButtonDoubleClick);
		this->buttonMinimize->eventMouseButtonPressed += MyGUI::newDelegate(this, &ConfigPanelCell::notfyMouseButtonPressed);
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

class ConfigPanelView : public wraps::BasePanelView<ConfigPanelCell>
{
public:
	ConfigPanelView(MyGUI::Widget* parent) :
		wraps::BasePanelView<ConfigPanelCell>("", parent)
	{
		
	}
};

///////////////////////////////////////////////////////////////////////

class ConfigPanelProject;
class ConfigPanelSceneManager;
class ConfigPanelOgreNewt;
class ConfigPanelRecast;

class ConfigPanel : public wraps::BaseLayout
{
public:
	ConfigPanel(ProjectManager* projectManager, const MyGUI::FloatCoord& coords);

	void destroyContent(void);

	void setVisible(bool show);

	void callForSettings(bool forSettings);

	void applySettings(void);

	MyGUI::Widget* getMainWidget(void) const;
private:
	void notifyWindowPressed(MyGUI::Window* widget, const std::string& name);
	void buttonHit(MyGUI::Widget* sender);
	void notifyMessageBoxEnd(MyGUI::Message* sender, MyGUI::MessageBoxStyle result);
	bool checkProjectExists(const Ogre::String& projectName, const Ogre::String& sceneName);
private:
	ProjectManager* projectManager;
	NOWA::EditorManager* editorManager;
	ConfigPanelView* configPanelView;
	ConfigPanelProject* configPanelProject;
	ConfigPanelSceneManager* configPanelSceneManager;
	ConfigPanelOgreNewt* configPanelOgreNewt;
	ConfigPanelRecast* configPanelRecast;
	MyGUI::Button* okButton;
	MyGUI::Button* abordButton;
	bool forSettings;
};

////////////////////////////////////////////////////////////////////////

class ConfigPanelProject : public wraps::BasePanelViewItem
{
public:
	ConfigPanelProject(ConfigPanel* parent);

	virtual void initialise();
	virtual void shutdown();

	void setParameter(const Ogre::String& projectName, const Ogre::String& sceneName, bool createProject, bool openProject, bool createOwnState, int key, bool ignoreGlobalScene, bool useV2Item);

	std::tuple<Ogre::String, Ogre::String, bool, bool, bool, int, bool, bool> getParameter(void) const;

	void resetSettings(void);

protected:
	void notifyComboChangedPosition(MyGUI::ComboBox* sender, size_t index);
	void onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode code, MyGUI::Char c);
	void buttonHit(MyGUI::Widget* sender);
	void onEditSelectAccepted(MyGUI::Widget* sender);
	void onEditTextChanged(MyGUI::Widget* sender);
	void onMouseButtonClicked(MyGUI::Widget* sender);
private:
	void fillScenesSearchList(void);
protected:
	MyGUI::ComboBox* themeBox;
	MyGUI::ComboBox* sceneNameEdit;
	MyGUI::ComboBox* projectNameEdit;
	MyGUI::Button* createProjectCheck;
	MyGUI::Button* openProjectCheck;
	MyGUI::Button* createStateCheck;
	MyGUI::Button* ignoreGlobalSceneCheck;
	MyGUI::Button* useV2ItemCheck;
	MyGUI::EditBox* keyEdit;
	MyGUI::VectorWidgetPtr itemsEdit;

	NOWA::AutoCompleteSearch projectAutoCompleteSearch;

	ConfigPanel* parent;
};

///////////////////////////////////////////////////////////////////////

class ConfigPanelSceneManager : public wraps::BasePanelViewItem
{
public:
	ConfigPanelSceneManager(ConfigPanel* parent);

	virtual void initialise();
	virtual void shutdown();

	void setParameter(const Ogre::ColourValue& ambientLightUpperHemisphere, const Ogre::ColourValue& ambientLightLowerHemisphere, Ogre::Real shadowFarDistance, Ogre::Real shadowDirectionalLightExtrusionDistance,
		Ogre::Real shadowDirLightTextureOffset, const Ogre::ColourValue& shadowColor, unsigned short shadowQuality, unsigned short ambientLightMode, unsigned short forwardMode, unsigned int lightWidth,
		unsigned int lightHeight, unsigned int numLightSlices, unsigned int lightsPerCell, Ogre::Real minLightDistance, Ogre::Real maxLightDistance, Ogre::Real renderDistance);

	std::tuple<Ogre::ColourValue, Ogre::ColourValue, Ogre::Real, Ogre::Real, Ogre::Real, Ogre::ColourValue, unsigned short,
				unsigned short, unsigned short, unsigned int, unsigned int, unsigned int, unsigned int, Ogre::Real, Ogre::Real, Ogre::Real> getParameter(void) const;
protected:
	void notifyComboChangedPosition(MyGUI::ComboBox* sender, size_t index);
	void buttonHit(MyGUI::Widget* sender);
	void notifyColourAccept(MyGUI::ColourPanel* sender);
	void notifyColourCancel(MyGUI::ColourPanel* sender);
	void onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode code, MyGUI::Char c);
protected:
	MyGUI::EditBox* ambientLightEdit1;
	MyGUI::Button* ambientLightColour1;
	MyGUI::EditBox* ambientLightEdit2;
	MyGUI::Button* ambientLightColour2;
	MyGUI::EditBox* shadowFarDistanceEdit;
	MyGUI::EditBox* shadowDirectionalLightExtrusionDistanceEdit;
	MyGUI::EditBox* shadowDirLightTextureOffsetEdit;
	MyGUI::EditBox* shadowColorEdit;
	MyGUI::ComboBox* shadowQualityCombo;
	MyGUI::ComboBox* ambientLightModeCombo;
	
	MyGUI::ComboBox* forwardModeCombo;
	MyGUI::EditBox* lightWidthEditBox;
	MyGUI::EditBox* lightHeightEditBox;
	MyGUI::EditBox* numLightSlicesEditBox;
	MyGUI::EditBox* lightsPerCellEditBox;
	MyGUI::EditBox* minLightDistanceEditBox;
	MyGUI::EditBox* maxLightDistanceEditBox;
	MyGUI::EditBox* renderDistanceEditBox;
	MyGUI::TextBox* lightWidthTextBox;
	MyGUI::TextBox* lightHeightTextBox;
	MyGUI::TextBox* numLightSlicesTextBox;
	MyGUI::TextBox* lightsPerCellTextBox;
	MyGUI::TextBox* minLightDistanceTextBox;
	MyGUI::TextBox* maxLightDistanceTextBox;
	MyGUI::TextBox* renderDistanceTextBox;

	MyGUI::VectorWidgetPtr itemsEdit;

	ConfigPanel* parent;
};

///////////////////////////////////////////////////////////////////////

class ConfigPanelOgreNewt : public wraps::BasePanelViewItem
{
public:
	ConfigPanelOgreNewt();

	virtual void initialise();
	virtual void shutdown();

	void setParameter(Ogre::Real updateRate, unsigned short solverModel, bool solverForSingleIsland, unsigned short broadPhaseAlgorithm, unsigned short threadCount, Ogre::Real linearDamping, 
		const Ogre::Vector3& angularDamping, const Ogre::Vector3& gravity);

	std::tuple<Ogre::Real, unsigned short, bool, unsigned short, unsigned short, Ogre::Real, Ogre::Vector3, Ogre::Vector3> getParameter(void) const;
private:
	void buttonHit(MyGUI::Widget* sender);
	void onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode code, MyGUI::Char c);
protected:
	MyGUI::EditBox* updateRateEdit;
	MyGUI::ComboBox* solverModelCombo;
	MyGUI::Button* solverForSingleIslandCheck;
	MyGUI::ComboBox* broadPhaseAlgorithmCombo;
	MyGUI::ComboBox* threadCountCombo;
	MyGUI::EditBox* linearDampingEdit;
	MyGUI::EditBox* angularDampingEdit;
	MyGUI::EditBox* gravityEdit;

	MyGUI::VectorWidgetPtr itemsEdit;
};

///////////////////////////////////////////////////////////////////////

class ConfigPanelRecast : public wraps::BasePanelViewItem
{
public:
	ConfigPanelRecast();

	virtual void initialise();
	virtual void shutdown();

	void setParameter(	bool hasRecast,
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
					);

	std::tuple<bool, Ogre::Real, Ogre::Real, Ogre::Real, Ogre::Real, Ogre::Real, Ogre::Real, Ogre::Real, Ogre::Real, Ogre::Real, Ogre::Real, unsigned short, Ogre::Real, Ogre::Real, Ogre::Vector3, bool> getParameter(void) const;
private:
	void buttonHit(MyGUI::Widget* sender);
	void onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode code, MyGUI::Char c);
protected:
	MyGUI::Button* navigationCheck;
	MyGUI::EditBox* cellSizeEdit;
	MyGUI::EditBox* cellHeightEdit;
	MyGUI::EditBox* agentMaxSlopeEdit;
	MyGUI::EditBox* agentHeightEdit;
	MyGUI::EditBox* agentMaxClimbEdit;
	MyGUI::EditBox* agentRadiusEdit;
	MyGUI::EditBox* edgeMaxLenEdit;
	MyGUI::EditBox* edgeMaxErrorEdit;
	MyGUI::EditBox* regionMinSizeEdit;
	MyGUI::EditBox* regionMergeSizeEdit;
	MyGUI::ComboBox* vertsPerPolyCombo;
	MyGUI::EditBox* detailSampleDistEdit;
	MyGUI::EditBox* detailSampleMaxErrorEdit;
	MyGUI::EditBox* pointExtendsEdit;
	MyGUI::Button* keepInterResultsCheck;

	MyGUI::VectorWidgetPtr itemsEdit;
};

///////////////////////////////////////////////////////////////////////

#endif

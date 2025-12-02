#include "NOWAPrecompiled.h"
#include "PropertiesPanel.h"
#include "main/EventManager.h"
#include "main/ProcessManager.h"
#include "GuiEvents.h"
#include "gameobject/JointComponents.h"
#include "MyGUIHelper.h"

#include "utilities/Interpolator.h"

#include <regex>

namespace
{
	std::string removeHashesExceptForColor(const std::string& text)
	{
		std::string result;
		size_t len = text.length();
		size_t i = 0;

		while (i < len)
		{
			if (text[i] == '#' && i + 7 <= len)
			{
				// Check if the next 6 characters after '#' form a valid color code
				std::string potentialColor = text.substr(i, 7);
				std::regex colorPattern("#[0-9A-Fa-f]{6}");

				if (std::regex_match(potentialColor, colorPattern))
				{
					// If it's a valid color, copy the next 7 characters (the color)
					result += potentialColor;
					i += 7;  // Skip past the color code
				}
				else
				{
					// It's not a color, so ignore the '#'
					i++;
				}
			}
			else
			{
				// If it's not a '#' or not followed by a valid color code, copy the character
				if (text[i] != '#')
					result += text[i];
				i++;
			}
		}

		return result;
	}
}

bool PropertiesPanel::bShowProperties = true;

class SetScrollPositionProcess : public NOWA::Process
{
public:
	explicit SetScrollPositionProcess(PropertiesPanelView* propertiesPanelView, int scrollPosition)
		: propertiesPanelView(propertiesPanelView),
		scrollPosition(scrollPosition)
	{

	}
protected:
	virtual void onInit(void) override
	{
		this->succeed();
		// this->propertiesPanelView1->getScrollView()->setVRange(scrollPosition);
			// vScrollBar->setScrollPosition(MyGUIHelper::getInstance()->getScrollPosition());
		this->propertiesPanelView->getScrollView()->setViewOffset(MyGUI::IntPoint(this->propertiesPanelView->getScrollView()->getViewOffset().left, this->scrollPosition));
	}

	virtual void onUpdate(float dt) override
	{
		this->succeed();
	}
private:
	PropertiesPanelView* propertiesPanelView;
	int scrollPosition;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ImageData::ImageData()
	: resourceImage(nullptr),
	imageBack(nullptr)
{

}

ImageData::~ImageData()
{

}

void ImageData::setResourceName(const Ogre::String& resourceName)
{
	this->resourceName = resourceName;

	if (nullptr == this->resourceImage)
	{
		bool foundCorrectType = false;

		MyGUI::ResourceManager& manager = MyGUI::ResourceManager::getInstance();
		MyGUI::IResource* resourceImageSet = manager.getByName(resourceName, false);
		if (nullptr != resourceImageSet)
		{
			this->resourceImage = resourceImageSet->castType<MyGUI::ResourceImageSet>();
			foundCorrectType = true;
		}

		if (false == foundCorrectType)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ImageData] ERROR: Could not add resource: "
				+ resourceName + ", because it cannot be found, or the XML container is wrong. Check your resource location name XML");
		}
		if (nullptr != this->imageItem)
		{
			// this->imageItem->setImageTexture(this->resourceName);
			this->imageItem->setItemResourcePtr(this->resourceImage);
			this->imageItem->setItemGroup("States");
			this->imageItem->setItemName("None");
			this->imageItem->setVisible(true);
		}
	}
}

bool ImageData::isEmpty() const
{
	return this->imageItem == 0;
}

void ImageData::setImageBoxBack(MyGUI::ImageBox* imageBack)
{
	this->imageBack = imageBack;
}

void ImageData::setImageBoxItem(MyGUI::ImageBox* imageItem)
{
	this->imageItem = imageItem;
}

MyGUI::ResourceImageSetPtr ImageData::getResourceImagePtr(void) const
{
	return this->resourceImage;
}

MyGUI::ImageBox* ImageData::getImageBoxBack(void) const
{
	return this->imageBack;
}

MyGUI::ImageBox* ImageData::getImageBoxItem(void) const
{
	return this->imageItem;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

PropertiesPanel::PropertiesPanel(const MyGUI::FloatCoord& coords)
	: BaseLayout("PropertiesPanelView.layout"),
	editorManager(nullptr),
	propertiesPanelView1(nullptr),
	propertiesPanelView2(nullptr),
	propertiesPanelDirector(nullptr),
	propertiesPanelInfo(nullptr)
{
	this->mMainWidget->setRealCoord(coords);
	assignBase(this->propertiesPanelView1, "properties1ScrollView");
	assignBase(this->propertiesPanelView2, "properties2ScrollView");

	// Test
	// MyGUI::TabControl* tabControl = MyGUIHelper::getInstance()->findParentWidget<MyGUI::TabControl>(this->propertiesPanelView1->getScrollView(), "propertiesTab");
	// MyGUI::TabItem* tabItem = tabControl->addItem("Script1.lua");

	this->openSaveFileDialog = new OpenSaveFileDialogExtended();

	// MyGUI::Gui::getInstance().eventFrameStart += MyGUI::newDelegate(this, &PropertiesPanel::notifyFrameStart);
	// this->propertiesPanelView1->getMainWidget()->eventMouseWheel += MyGUI::newDelegate(this, &PropertiesPanel::onMouseWheel);
	// this->propertiesPanelView1->getScrollView()->eventMouseDrag += MyGUI::newDelegate(this, &PropertiesPanel::onMouseRelease);

	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &PropertiesPanel::handleRefreshPropertiesPanel), EventDataRefreshPropertiesPanel::getStaticEventType());
}

void PropertiesPanel::setEditorManager(NOWA::EditorManager* editorManager)
{
	this->editorManager = editorManager;
}

void PropertiesPanel::destroyContent(void)
{
	// Threadsafe from the outside
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &PropertiesPanel::handleRefreshPropertiesPanel), EventDataRefreshPropertiesPanel::getStaticEventType());

	this->propertiesPanelView1->removeAllItems();

	this->propertiesPanelView2->removeAllItems();

	if (nullptr != this->propertiesPanelInfo)
	{
		delete this->propertiesPanelInfo;
		this->propertiesPanelInfo = nullptr;
	}

	if (this->openSaveFileDialog)
	{
		delete this->openSaveFileDialog;
		this->openSaveFileDialog = nullptr;
	}
}	

void PropertiesPanel::clearProperties(void)
{
	ENQUEUE_RENDER_COMMAND_WAIT("PropertiesPanel::clearProperties",
	{
		// Schrott MyGUI, those events do not work at all
		// this->propertiesPanelView1->getScrollView()->eventMouseWheel -= MyGUI::newDelegate(this, &PropertiesPanel::onMouseWheel);
		// this->propertiesPanelView1->getScrollView()->eventMouseButtonReleased -= MyGUI::newDelegate(this, &PropertiesPanel::onMouseRelease);
		this->propertiesPanelView1->removeAllItems();
		this->propertiesPanelView2->removeAllItems();
	});
}

void PropertiesPanel::setVisible(bool show)
{
	this->mMainWidget->setVisible(show);
}

void PropertiesPanel::onMouseWheel(MyGUI::Widget* sender, int rel)
{
	// MyGUIHelper::getInstance()->setScrollPosition(this->propertiesPanelView1->getScrollView()->getViewOffset().top);
}

void PropertiesPanel::onMouseRelease(MyGUI::Widget* sender, int left, int top, MyGUI::MouseButton id)
{
	if (MyGUI::MouseButton::Left == id)
	{
		// MyGUIHelper::getInstance()->setScrollPosition(this->propertiesPanelView1->getScrollView()->getViewOffset().top);
	}
}

void PropertiesPanel::showProperties(unsigned int componentIndex)
{
	if (false == PropertiesPanel::bShowProperties)
	{
		PropertiesPanel::bShowProperties = true;
		return;
	}

	// Schrott MyGUI, those events do not work at all
	// this->propertiesPanelView1->getScrollView()->eventMouseWheel += MyGUI::newDelegate(this, &PropertiesPanel::onMouseWheel);
	// this->propertiesPanelView1->getScrollView()->eventMouseButtonReleased += MyGUI::newDelegate(this, &PropertiesPanel::onMouseRelease);

	// Get data from selected game objects for properties panel

	ENQUEUE_RENDER_COMMAND_MULTI("PropertiesPanel::showProperties", _1(componentIndex),
	{
		// First clear the properties
		this->clearProperties();
		// Show all properties for selected game objects
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
			this->showProperties(gameObjects, componentIndex);

			// Set remembered scroll position after everything has been reloaded (with delay), else the scrollviewer range is 0
			int scrollPosition = MyGUIHelper::getInstance()->getScrollPosition(gameObjects[0]->getId());

			if (-1 != scrollPosition)
			{
				NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.5f));
				delayProcess->attachChild(NOWA::ProcessPtr(new SetScrollPositionProcess(this->propertiesPanelView1, scrollPosition)));
				NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
			}
		}
	});
}

void PropertiesPanel::showProperties(std::vector<NOWA::GameObject*> gameObjects, unsigned int componentIndex)
{
	// Create properties panel for info
	this->propertiesPanelInfo = new PropertiesPanelInfo();
	// Create properties panel
	PropertiesPanelDynamic* gameObjectPropertiesPanel = new PropertiesPanelGameObject(gameObjects, "GameObject");
	assert(this->editorManager != nullptr);

	gameObjectPropertiesPanel->setEditorManager(this->editorManager);
	gameObjectPropertiesPanel->setPropertiesPanelInfo(this->propertiesPanelInfo);
	// Add it to the view
	this->propertiesPanelView1->addItem(gameObjectPropertiesPanel);

	// Add game object
	{
		auto& attributes = gameObjects[0]->getAttributes();
		// Just one object, simple case, add all attributes
		if (1 == gameObjects.size())
		{
			for (auto& it = attributes.begin(); it != attributes.end(); ++it)
			{
				gameObjectPropertiesPanel->addProperty(it->first, it->second, true);
			}
		}
		else
		{
			// Complex case, add attributes for several game objects. That is: Match if there are game objects, that do have the same values
			std::map<Ogre::String, bool> sameValues;

			for (size_t i = 0; i < gameObjects.size() - 1; i++)
			{
				NOWA::GameObject* thisGameObject = gameObjects[i];
				NOWA::GameObject* nextGameObject = gameObjects[i + 1];

				auto& thisAttributes = thisGameObject->getAttributes();
				auto& nextAttributes = nextGameObject->getAttributes();

				for (auto& it = thisAttributes.cbegin(); it != thisAttributes.cend(); ++it)
				{
					NOWA::Variant* thisAttribute = it->second;
					NOWA::Variant* otherAttribute = nextGameObject->getAttribute(thisAttribute->getName());
					if (nullptr != otherAttribute)
					{
						// Internally checks if the value is the same
						bool samePropertyValue = thisAttribute->equals(otherAttribute);
						sameValues.emplace(thisAttribute->getName(), samePropertyValue);
					}
				}
			}

			for (auto& it = attributes.begin(); it != attributes.end(); ++it)
			{
				const auto& found = sameValues.find(it->first);
				if (found != sameValues.cend())
				{
					// last param is whether the property has the same value for all selected game objects
					gameObjectPropertiesPanel->addProperty(it->first, it->second, found->second);
				}
				else
				{
					gameObjectPropertiesPanel->addProperty(it->first, it->second, false);
				}
			}
		}
	}

	// Show for all game objects, if the components are in the same order and are the same
	bool componentsAreSame = true;
	bool jointsVisible = true;

	// Complex case, add attributes for several game objects. That is: Match if there are game objects, that do have the same values
	std::map<Ogre::String, bool> sameValues;

	// Add components
	for (size_t i = 0; i < gameObjects.size(); i++)
	{
		unsigned int thisComponentCount = static_cast<unsigned int>(gameObjects[i]->getComponents()->size());
		if (0 == thisComponentCount || false == componentsAreSame)
		{
			break;
		}

		NOWA::GameObject* thisGameObject = gameObjects[i];

		NOWA::GameObject* nextGameObject = nullptr;
		if (i < gameObjects.size() - 1)
		{
			nextGameObject = gameObjects[i + 1];
			// Component count must be the same
			unsigned int nextComponentCount = static_cast<unsigned int>(nextGameObject->getComponents()->size());
			if (nextComponentCount != thisComponentCount)
			{
				componentsAreSame = false;
				break;
			}
		}

		for (unsigned int j = 0; j < thisComponentCount; j++)
		{
			NOWA::GameObjectCompPtr thisGameObjectComponent = std::get<NOWA::COMPONENT>(gameObjects[i]->getComponents()->at(j));

			// Only validate several if there is at least one other game object
			if (nullptr != nextGameObject)
			{
				NOWA::GameObjectCompPtr nextGameObjectComponent = std::get<NOWA::COMPONENT>(gameObjects[i + 1]->getComponents()->at(j));
				// If the component name of the text game object does not match this game object, skip everything
				if (thisGameObjectComponent->getClassName() != nextGameObjectComponent->getClassName())
				{
					componentsAreSame = false;
					break;
				}

				auto thisAttributes = std::get<NOWA::COMPONENT>(thisGameObject->getComponents()->at(j))->getAttributes();
				auto nextAttributes = std::get<NOWA::COMPONENT>(nextGameObject->getComponents()->at(j))->getAttributes();

				unsigned int k = 0;
				for (auto& it = thisAttributes.cbegin(); it != thisAttributes.cend(); ++it)
				{
					NOWA::Variant* thisAttribute = it->second;
					NOWA::Variant* otherAttribute = nextAttributes[k++].second;
					if (nullptr != otherAttribute)
					{
						// Internally checks if the value is the same
						bool samePropertyValue = thisAttribute->equals(otherAttribute);
						sameValues.emplace(thisAttribute->getName(), samePropertyValue);
					}
				}
			}
		}
	}

	PropertiesPanelDynamic* componentPropertiesPanel = nullptr;
	NOWA::PhysicsCompPtr physicsComponentPtr = nullptr;

	int overallScrollPosition = 0;

	unsigned int count = static_cast<unsigned int>(gameObjects[0]->getComponents()->size());
	for (unsigned int j = 0; j < count; j++)
	{
		if (true == componentsAreSame)
		{
			Ogre::String componentName = std::get<NOWA::COMPONENT>(gameObjects[0]->getComponents()->at(j))->getClassName();
			size_t found = componentName.find("Component");
			if (Ogre::String::npos != found)
			{
				// Remove the component part, because the names will become to long
				componentName = componentName.substr(0, found);
			}

			// Build components vector, to be able to configure values for several components
			std::vector<NOWA::GameObjectComponent*> gameObjectComponents(gameObjects.size());
			for (size_t k = 0; k < gameObjects.size(); k++)
			{
				gameObjectComponents[k] = std::get<NOWA::COMPONENT>(gameObjects[k]->getComponents()->at(j)).get();
			}

			// Create the panel for the component
			componentPropertiesPanel = new PropertiesPanelComponent(gameObjects, gameObjectComponents, componentName
				+ " (" + Ogre::StringConverter::toString(std::get<NOWA::COMPONENT>(gameObjects[0]->getComponents()->at(j))->getOccurrenceIndex()) + ")");

			componentPropertiesPanel->setEditorManager(this->editorManager);
			componentPropertiesPanel->setPropertiesPanelInfo(this->propertiesPanelInfo);

			// Add the component to view
			this->propertiesPanelView1->addItem(componentPropertiesPanel);

			// Adds this component pointer as user data, in order to set expand flag on the component
			NOWA::GameObjectCompPtr thisComponent = std::get<NOWA::COMPONENT>(gameObjects[0]->getComponents()->at(j));
			// componentPropertiesPanel->getPanelCell()->getMainWidget()->setUserString("Component", Ogre::StringConverter::toString(reinterpret_cast<size_t>(thisComponent.get())));
			componentPropertiesPanel->getPanelCell()->getMainWidget()->setUserData(MyGUI::Any(thisComponent.get()));

			bool minimize = false == thisComponent->getIsExpanded();
			if (true == minimize)
			{
				componentPropertiesPanel->getPanelCell()->setClientHeight(26, false);
				componentPropertiesPanel->getPanelCell()->setMinimized(minimize);
			}

			// Add the properties
			auto& attributes = thisComponent->getAttributes();
			for (auto& it = attributes.begin(); it != attributes.end(); ++it)
			{
				const auto& found = sameValues.find(it->first);
				// Nothing to validate, just one game object, so add default way
				if (0 == sameValues.size())
				{
					componentPropertiesPanel->addProperty(it->first, it->second, true);
				}
				else
				{
					if (found != sameValues.cend())
					{
						// last param is whether the property has the same value for all selected game objects
						componentPropertiesPanel->addProperty(it->first, it->second, found->second);
					}
					else
					{
						componentPropertiesPanel->addProperty(it->first, it->second, false);
					}
				}
			}

			// In order to scroll: Position is negative
			overallScrollPosition -= componentPropertiesPanel->heightCurrent;

			// Scrolls to the selected component
			if (componentIndex > 0 && j == componentIndex)
			{
				NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.5f));
				delayProcess->attachChild(NOWA::ProcessPtr(new SetScrollPositionProcess(this->propertiesPanelView1, overallScrollPosition)));
				NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
			}
		}
	}

	// At last add the static panel, for static info
	this->propertiesPanelView2->addItem(propertiesPanelInfo);

	propertiesPanelInfo->listData(gameObjects[0]);

	if (false == componentsAreSame)
	{
		propertiesPanelInfo->setInfo("Components do mismatch\n and hence are not visible\n for several GameObjects.");
	}
	else if (false == jointsVisible)
	{
		propertiesPanelInfo->setInfo("Joints not visible\n for several GameObjects.");
	}
}

void PropertiesPanel::handleRefreshPropertiesPanel(NOWA::EventDataPtr eventData)
{
	this->showProperties();
}

PropertiesPanelDynamic* PropertiesPanel::getPropertiesPanelItem(size_t index)
{
	if (0 == this->propertiesPanelView1->getItemCount())
	{
		return nullptr;
	}
	if (index >= this->propertiesPanelView1->getItemCount())
	{
		return nullptr;
	}
	return dynamic_cast<PropertiesPanelDynamic*>(this->propertiesPanelView1->getItem(index));
}

void PropertiesPanel::setShowPropertiesFlag(bool bShowProperties)
{
	PropertiesPanel::bShowProperties = bShowProperties;
}

bool PropertiesPanel::getShowPropertiesFlag(void)
{
	bool tempShowProperties = PropertiesPanel::bShowProperties;
	PropertiesPanel::bShowProperties = true;
	return tempShowProperties;
}


/////////////////////////////////////////////////////////////////////////////

PropertiesPanelInfo::PropertiesPanelInfo()
	: BasePanelViewItem("PropertiesPanelInfo.layout"),
	heightCurrent(200) // Need space for property info
{

}

void PropertiesPanelInfo::setInfo(const Ogre::String& info)
{
	ENQUEUE_RENDER_COMMAND_MULTI("PropertiesPanelInfo::setInfo", _1(info),
	{
		this->propertyInfo->setOnlyText(info);
	});
}

void PropertiesPanelInfo::listData(NOWA::GameObject* gameObject)
{
	ENQUEUE_RENDER_COMMAND_MULTI("PropertiesPanelInfo::listData", _1(gameObject),
	{
		const int height = 26;
		const int heightStep = 28;
		const int widthStep = 3;

		const int keyLeft = 1;
		const int keyWidth = static_cast<int>(this->mWidgetClient->getWidth() * 0.8f);
		const int valueLeft = static_cast<int>(this->mWidgetClient->getWidth() * 0.8f + widthStep);
		const int valueWidth = static_cast<int>(this->mWidgetClient->getWidth() * 0.3f);

		for (size_t i = 0; i < gameObject->getComponents()->size(); i++)
		{
			const int informationHeight = 256;

			Ogre::String className = NOWA::makeStrongPtr(gameObject->getComponentByIndex(i))->getClassName();

			Ogre::String information = NOWA::GameObjectFactory::getInstance()->getComponentFactory()->getComponentInfoText(className);
			MyGUI::IntCoord coord = MyGUI::IntCoord(keyLeft, this->heightCurrent, static_cast<int>(this->mWidgetClient->getWidth() * 0.9f), informationHeight);
			MyGUI::EditBox* informationEdit = this->mWidgetClient->createWidget<MyGUI::EditBox>("EditBox", coord, MyGUI::Align::HStretch | MyGUI::Align::Top);
			informationEdit->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
			informationEdit->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);
			informationEdit->setEditMultiLine(true);
			informationEdit->setEditWordWrap(true);
			informationEdit->showHScroll(true);
			informationEdit->setCaptionWithReplacing(className + "\n\n" + information);
			informationEdit->setEditReadOnly(true);
			informationEdit->setEditStatic(false);
			this->itemsText.push_back(informationEdit);
			this->heightCurrent += informationHeight + 2;
		}

		Ogre::v1::Entity* entity = gameObject->getMovableObject<Ogre::v1::Entity>();
		if (nullptr != entity)
		{
			///////////////////////////////List all animations///////////////////////////////////////////////////////
			std::vector<Ogre::String> animationNames;
			Ogre::v1::AnimationStateSet* set = entity->getAllAnimationStates();
			if (nullptr != set)
			{
				MyGUI::TextBox* keyTextBox = mWidgetClient->createWidget<MyGUI::TextBox>("TextBox", MyGUI::IntCoord(keyLeft, heightCurrent, keyWidth, height), MyGUI::Align::Left | MyGUI::Align::Top);
				keyTextBox->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
				keyTextBox->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
				keyTextBox->setCaption("Animations:");

				this->itemsText.push_back(keyTextBox);

				this->heightCurrent += heightStep;

				unsigned int i = 0;
				Ogre::v1::AnimationStateIterator it = set->getAnimationStateIterator();
				// list all animations
				while (it.hasMoreElements())
				{
					Ogre::v1::AnimationState* anim = it.getNext();

					// Set the key of the property
					MyGUI::TextBox* keyTextBox = mWidgetClient->createWidget<MyGUI::TextBox>("TextBox", MyGUI::IntCoord(keyLeft, heightCurrent, keyWidth, height), MyGUI::Align::Left | MyGUI::Align::Top);
					keyTextBox->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
					keyTextBox->setCaption("Animation: " + Ogre::StringConverter::toString(i));

					this->itemsText.push_back(keyTextBox);

					MyGUI::EditBox* edit = mWidgetClient->createWidget<MyGUI::EditBox>("EditBox", MyGUI::IntCoord(valueLeft, heightCurrent, valueWidth, height), MyGUI::Align::HStretch | MyGUI::Align::Top);
					edit->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
					edit->setMouseHitThreshold(6, 6, 3, 3);
					edit->setOnlyText(anim->getAnimationName());
					edit->setEditReadOnly(true);

					this->itemsEdit.push_back(edit);

					this->heightCurrent += heightStep;
					i++;
				}

				MyGUI::Widget* separator = mWidgetClient->createWidget<MyGUI::Widget>("Separator3", MyGUI::IntCoord(keyLeft, this->heightCurrent, static_cast<int>(mWidgetClient->getWidth()), height), MyGUI::Align::HStretch | MyGUI::Align::Top);
				this->itemsText.push_back(separator);
				this->heightCurrent += heightStep / 2;
			}

			///////////////////////////////List all bones///////////////////////////////////////////////////////
			Ogre::v1::Skeleton* skeleton = entity->getSkeleton();
			if (nullptr != skeleton)
			{
				std::vector<Ogre::String> boneNames;

				MyGUI::TextBox* keyTextBox = mWidgetClient->createWidget<MyGUI::TextBox>("TextBox", MyGUI::IntCoord(keyLeft, heightCurrent, 100, height), MyGUI::Align::Left | MyGUI::Align::Top);
				keyTextBox->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
				keyTextBox->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
				keyTextBox->setCaption("Bones of Skeleton: " + skeleton->getName());

				this->itemsText.push_back(keyTextBox);
				this->heightCurrent += heightStep;

				unsigned short numBones = entity->getSkeleton()->getNumBones();
				for (unsigned short iBone = 0; iBone < numBones; iBone++)
				{
					Ogre::v1::OldBone* bone = entity->getSkeleton()->getBone(iBone);
					if (nullptr == bone)
					{
						continue;
					}

					// Absolutely HAVE to create bone representations first. Otherwise we would get the wrong child count
					// because an attached object counts as a child
					// Would be nice to have a function that only gets the children that are bones...
					unsigned short numChildren = bone->numChildren();
					if (numChildren == 0)
					{
						bool unique = true;
						for (size_t i = 0; i < boneNames.size(); i++)
						{
							if (boneNames[i] == bone->getName())
							{
								unique = false;
								break;
							}
						}
						if (true == unique)
						{
							boneNames.emplace_back(bone->getName());
							// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[TagPointComponent] Bone name: " + bone->getName());
						}
					}
					else
					{
						bool unique = true;
						for (size_t i = 0; i < boneNames.size(); i++)
						{
							if (boneNames[i] == bone->getName())
							{
								unique = false;
								break;
							}
						}
						if (true == unique)
						{
							boneNames.emplace_back(bone->getName());
							// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[TagPointComponent] Bone name: " + bone->getName());
						}
					}
				}

				// Add all available tag names to list

				for (size_t j = 0; j < boneNames.size(); j++)
				{
					// Set the key of the property
					MyGUI::TextBox* keyTextBox = mWidgetClient->createWidget<MyGUI::TextBox>("TextBox", MyGUI::IntCoord(keyLeft, heightCurrent, keyWidth, height), MyGUI::Align::Left | MyGUI::Align::Top);
					keyTextBox->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
					keyTextBox->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
					keyTextBox->setCaption("Bone: " + Ogre::StringConverter::toString(j));

					this->itemsText.push_back(keyTextBox);

					MyGUI::EditBox* edit = mWidgetClient->createWidget<MyGUI::EditBox>("EditBox", MyGUI::IntCoord(valueLeft, heightCurrent, valueWidth, height), MyGUI::Align::HStretch | MyGUI::Align::Top);
					edit->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
					edit->setOnlyText(boneNames[j]);
					edit->setEditReadOnly(true);
					this->itemsEdit.push_back(edit);

					this->heightCurrent += heightStep;
				}

				MyGUI::Widget* separator = mWidgetClient->createWidget<MyGUI::Widget>("Separator3", MyGUI::IntCoord(keyLeft, this->heightCurrent, static_cast<int>(mWidgetClient->getWidth()), height), MyGUI::Align::HStretch | MyGUI::Align::Top);
				this->itemsText.push_back(separator);
				this->heightCurrent += heightStep / 2;
			}

			int j = 0;

			auto& simpleSoundComponent = NOWA::makeStrongPtr(gameObject->getComponent<NOWA::SimpleSoundComponent>(j));
			while (simpleSoundComponent != nullptr && simpleSoundComponent->getSound() != nullptr)
			{
				MyGUI::TextBox* keyTextBox = mWidgetClient->createWidget<MyGUI::TextBox>("TextBox", MyGUI::IntCoord(keyLeft, heightCurrent, keyWidth, height), MyGUI::Align::Left | MyGUI::Align::Top);
				keyTextBox->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
				keyTextBox->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
				keyTextBox->setCaption("SoundName: ");
				this->itemsText.push_back(keyTextBox);

				MyGUI::EditBox* edit = mWidgetClient->createWidget<MyGUI::EditBox>("EditBox", MyGUI::IntCoord(valueLeft, heightCurrent, valueWidth, height), MyGUI::Align::HStretch | MyGUI::Align::Top);
				edit->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
				edit->setOnlyText(simpleSoundComponent->getSoundName());
				edit->setEditReadOnly(true);
				this->itemsEdit.push_back(edit);
				this->heightCurrent += heightStep;

				///////////////////////////////////////////////////

				keyTextBox = mWidgetClient->createWidget<MyGUI::TextBox>("TextBox", MyGUI::IntCoord(keyLeft, heightCurrent, keyWidth, height), MyGUI::Align::Left | MyGUI::Align::Top);
				keyTextBox->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
				keyTextBox->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
				keyTextBox->setCaption("Length: ");
				this->itemsText.push_back(keyTextBox);

				edit = mWidgetClient->createWidget<MyGUI::EditBox>("EditBox", MyGUI::IntCoord(valueLeft, heightCurrent, valueWidth, height), MyGUI::Align::HStretch | MyGUI::Align::Top);
				edit->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
				edit->setOnlyText(Ogre::StringConverter::toString(simpleSoundComponent->getSound()->getLength()) + " Seconds");
				edit->setEditReadOnly(true);
				this->itemsEdit.push_back(edit);
				this->heightCurrent += heightStep;

				///////////////////////////////////////////////////

				keyTextBox = mWidgetClient->createWidget<MyGUI::TextBox>("TextBox", MyGUI::IntCoord(keyLeft, heightCurrent, keyWidth, height), MyGUI::Align::Left | MyGUI::Align::Top);
				keyTextBox->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
				keyTextBox->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
				keyTextBox->setCaption("Frequency: ");
				this->itemsText.push_back(keyTextBox);

				edit = mWidgetClient->createWidget<MyGUI::EditBox>("EditBox", MyGUI::IntCoord(valueLeft, heightCurrent, valueWidth, height), MyGUI::Align::HStretch | MyGUI::Align::Top);
				edit->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
				edit->setOnlyText(Ogre::StringConverter::toString(simpleSoundComponent->getSound()->getFrequency()));
				edit->setEditReadOnly(true);
				this->itemsEdit.push_back(edit);
				this->heightCurrent += heightStep;

				///////////////////////////////////////////////////

				keyTextBox = mWidgetClient->createWidget<MyGUI::TextBox>("TextBox", MyGUI::IntCoord(keyLeft, heightCurrent, keyWidth, height), MyGUI::Align::Left | MyGUI::Align::Top);
				keyTextBox->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
				keyTextBox->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
				keyTextBox->setCaption("Channels: ");
				this->itemsText.push_back(keyTextBox);

				edit = mWidgetClient->createWidget<MyGUI::EditBox>("EditBox", MyGUI::IntCoord(valueLeft, heightCurrent, valueWidth, height), MyGUI::Align::HStretch | MyGUI::Align::Top);
				edit->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
				edit->setOnlyText(Ogre::StringConverter::toString(simpleSoundComponent->getSound()->getChannelCount()));
				edit->setEditReadOnly(true);
				this->itemsEdit.push_back(edit);
				this->heightCurrent += heightStep;

				///////////////////////////////////////////////////

				keyTextBox = mWidgetClient->createWidget<MyGUI::TextBox>("TextBox", MyGUI::IntCoord(keyLeft, heightCurrent, keyWidth, height), MyGUI::Align::Left | MyGUI::Align::Top);
				keyTextBox->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
				keyTextBox->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
				keyTextBox->setCaption("Bits per sample: ");
				this->itemsText.push_back(keyTextBox);

				edit = mWidgetClient->createWidget<MyGUI::EditBox>("EditBox", MyGUI::IntCoord(valueLeft, heightCurrent, valueWidth, height), MyGUI::Align::HStretch | MyGUI::Align::Top);
				edit->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
				edit->setOnlyText(Ogre::StringConverter::toString(simpleSoundComponent->getSound()->getBitsPerSample()));
				edit->setEditReadOnly(true);
				this->itemsEdit.push_back(edit);
				this->heightCurrent += heightStep;

				///////////////////////////////////////////////////

				keyTextBox = mWidgetClient->createWidget<MyGUI::TextBox>("TextBox", MyGUI::IntCoord(keyLeft, heightCurrent, keyWidth, height), MyGUI::Align::Left | MyGUI::Align::Top);
				keyTextBox->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
				keyTextBox->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
				keyTextBox->setCaption("Data size: ");
				this->itemsText.push_back(keyTextBox);

				edit = mWidgetClient->createWidget<MyGUI::EditBox>("EditBox", MyGUI::IntCoord(valueLeft, heightCurrent, valueWidth, height), MyGUI::Align::HStretch | MyGUI::Align::Top);
				edit->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
				edit->setOnlyText(Ogre::StringConverter::toString(simpleSoundComponent->getSound()->getDataSize()));
				edit->setEditReadOnly(true);
				this->itemsEdit.push_back(edit);
				this->heightCurrent += heightStep;

				j++;
				simpleSoundComponent = NOWA::makeStrongPtr(gameObject->getComponent<NOWA::SimpleSoundComponent>(j));

				MyGUI::Widget* separator = mWidgetClient->createWidget<MyGUI::Widget>("Separator3", MyGUI::IntCoord(keyLeft, this->heightCurrent, static_cast<int>(mWidgetClient->getWidth()), height), MyGUI::Align::HStretch | MyGUI::Align::Top);
				this->itemsText.push_back(separator);
				this->heightCurrent += heightStep / 2;
			}

		}

		mPanelCell->setClientHeight(this->heightCurrent, false);
	});
}

void PropertiesPanelInfo::initialise()
{
	ENQUEUE_RENDER_COMMAND_WAIT("PropertiesPanelInfo::initialise",
	{
		mPanelCell->setCaption("Info");
		mPanelCell->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
		assignWidget(this->propertyInfo, "propertyInfo");
		this->propertyInfo->setEditMultiLine(true);
		this->propertyInfo->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);
		this->propertyInfo->setEditWordWrap(true);
		this->propertyInfo->setEditStatic(false);
		this->propertyInfo->setEditReadOnly(true);
		this->propertyInfo->showVScroll(true);
		this->propertyInfo->showHScroll(true);
	});
}

void PropertiesPanelInfo::shutdown()
{
	// Threadsafe from the outside
	// Move the vectors for render thread cleanup
	auto textItems = std::move(this->itemsText);
	auto editItems = std::move(this->itemsEdit);

	// Ensure main thread doesn't touch them anymore
	this->itemsText.clear();
	this->itemsEdit.clear();

	for (auto* widget : textItems)
	{
		if (widget)
			MyGUI::Gui::getInstance().destroyWidget(widget);
	}

	for (auto* widget : editItems)
	{
		if (widget)
			MyGUI::Gui::getInstance().destroyWidget(widget);
	}
}


///////////////////////////////////////////////////////////////////////////////

PropertiesPanelDynamic::PropertiesPanelDynamic(const std::vector<NOWA::GameObject*>& gameObjects, const Ogre::String& name)
	: BasePanelViewItem(""),
	editorManager(nullptr),
	gameObjects(gameObjects),
	gameObject(gameObjects[0]),
	name(name),
	heightCurrent(0),
	propertiesPanelInfo(nullptr)
{
	this->openSaveFileDialog = new OpenSaveFileDialogExtended();
}

PropertiesPanelDynamic::~PropertiesPanelDynamic()
{
	auto openSaveFileDialogPtr = this->openSaveFileDialog;
	this->openSaveFileDialog = nullptr;

	if (openSaveFileDialogPtr)
	{
		ENQUEUE_RENDER_COMMAND_MULTI("PropertiesPanelDynamic::~PropertiesPanelDynamic", _1(openSaveFileDialogPtr),
		{
			delete openSaveFileDialogPtr;
		});
	}
}

void PropertiesPanelDynamic::setEditorManager(NOWA::EditorManager* editorManager)
{
	this->editorManager = editorManager;
}

void PropertiesPanelDynamic::setPropertiesPanelInfo(PropertiesPanelInfo* propertiesPanelInfo)
{
	this->propertiesPanelInfo = propertiesPanelInfo;
}

void PropertiesPanelDynamic::initialise()
{
	ENQUEUE_RENDER_COMMAND_WAIT("PropertiesPanelDynamic::initialise",
	{
		mPanelCell->setCaption(this->name);
		mPanelCell->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
		this->openSaveFileDialog->eventEndDialog = MyGUI::newDelegate(this, &PropertiesPanelDynamic::notifyEndDialog);
	});
}

void PropertiesPanelDynamic::shutdown()
{
	// Threadsafe from the outside
	// this->itemsText.clear();

	// Move the vectors for render thread cleanup
	auto textItems = std::move(this->itemsText);

	// Ensure main thread doesn't touch them anymore
	this->itemsText.clear();

	for (auto* widget : textItems)
	{
		if (widget)
			MyGUI::Gui::getInstance().destroyWidget(widget);
	}

	for (size_t i = 0; i < this->itemsEdit.size(); ++i)
	{
		auto widget = this->itemsEdit[i];
		MyGUI::ItemBox* itemBox = widget->castType<MyGUI::ItemBox>(false);

		if (itemBox)
		{
			size_t count = itemBox->getItemCount();
			for (size_t pos = 0; pos < count; ++pos)
			{
				MyGUI::Widget* childWidget = itemBox->getWidgetByIndex(pos);
				if (childWidget)
				{
					ImageData** data = childWidget->getUserData<ImageData*>(false);
					if (data)
					{
						auto toDelete = *data;
						delete toDelete;
					}
				}
			}
		}
	}

	this->itemsEdit.clear();
	this->gameObject = nullptr;

	if (this->openSaveFileDialog)
	{
		auto toDelete = this->openSaveFileDialog;
		this->openSaveFileDialog = nullptr;

		delete toDelete;
	}
}


void PropertiesPanelDynamic::setVisibleCount(unsigned int count)
{
	ENQUEUE_RENDER_COMMAND_MULTI("PropertiesPanelDynamic::setVisibleCount", _1(count),
	{
		const int heightStep = 28;
		int heightCurrent = 0;
		for (unsigned int pos = 0; pos < 16; ++pos)
		{
			if (pos < count)
			{
				this->itemsText[pos]->setVisible(true);
				this->itemsEdit[pos]->setVisible(true);
				heightCurrent += heightStep;
			}
			else
			{
				this->itemsText[pos]->setVisible(false);
				this->itemsEdit[pos]->setVisible(false);
			}
		}
		mPanelCell->setClientHeight(heightCurrent, true);
	});
}

void PropertiesPanelDynamic::addProperty(const Ogre::String& name, NOWA::Variant* attribute, bool allValuesSame)
{
	if (false == attribute->isVisible())
	{
		return;
	}

	const int height = 26;
	const int heightStep = 28;
	const int widthStep = 3;

	const int keyLeft = 1;
	const int keyWidth = static_cast<int>(mWidgetClient->getWidth() * 0.8f);
	const int valueLeft = static_cast<int>(mWidgetClient->getWidth() * 0.8f + widthStep);
	const int valueWidth = static_cast<int>(mWidgetClient->getWidth() * 0.3f);

	// Add label when required
	if (true == attribute->hasUserDataKey(NOWA::GameObject::AttrActionLabel()))
	{
		MyGUI::TextBox* label = mWidgetClient->createWidget<MyGUI::TextBox>("TextBox", MyGUI::IntCoord(keyLeft, heightCurrent, keyWidth, height), MyGUI::Align::Left | MyGUI::Align::Top);
		label->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
		label->setCaption(attribute->getDescription());

		this->itemsEdit.push_back(label);
		this->heightCurrent += heightStep;
	}

	// Set the key of the property
	MyGUI::TextBox* keyTextBox = mWidgetClient->createWidget<MyGUI::TextBox>("TextBox", MyGUI::IntCoord(keyLeft, heightCurrent, keyWidth, height), MyGUI::Align::Left | MyGUI::Align::Top);
	keyTextBox->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
	keyTextBox->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
	keyTextBox->setCaption(name);
	keyTextBox->setNeedToolTip(true);
	keyTextBox->setUserString("tooltip", attribute->getDescription());
	keyTextBox->eventToolTip += MyGUI::newDelegate(MyGUIHelper::getInstance(), &MyGUIHelper::notifyToolTip);

	size_t found = attribute->getName().find("Id");
	if (found != Ogre::String::npos)
	{
		// Set tooltip with connected game object for the id
		auto connectedGameObject = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(attribute->getULong());
		if (nullptr != connectedGameObject)
		{
			keyTextBox->setUserString("tooltip", "GameObject: '" + connectedGameObject->getName() + "'");
		}
	}

	this->itemsText.push_back(keyTextBox);

	switch (attribute->getType())
	{
	case NOWA::Variant::VAR_BOOL:
	{
		MyGUI::Button* checkBox = mWidgetClient->createWidget<MyGUI::Button>("CheckBox", MyGUI::IntCoord(valueLeft, heightCurrent, valueWidth, height), MyGUI::Align::HStretch | MyGUI::Align::Top, name);
		// checkBox->setCaption(Ogre::StringConverter::toString(attribute->getInt()));
		checkBox->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
		checkBox->setColour(MyGUIHelper::getInstance()->getDefaultTextColour());
		checkBox->setUserData(MyGUI::Any(attribute));
		// Store also if all values are the same
		checkBox->setStateCheck(attribute->getBool());
		checkBox->eventMouseButtonClick += MyGUI::newDelegate(this, &PropertiesPanelDynamic::buttonHit);
		checkBox->eventMouseSetFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::setFocus);
		checkBox->eventKeyButtonPressed += MyGUI::newDelegate(this, &PropertiesPanelDynamic::onKeyButtonPressed);
		checkBox->setNeedKeyFocus(true);
		checkBox->setNeedMouseFocus(true);
		checkBox->_setRootKeyFocus(true);
		checkBox->_setRootMouseFocus(true);

		if (true == attribute->hasUserDataKey(NOWA::GameObject::AttrActionReadOnly()))
		{
			checkBox->setEnabled(false);
		}

		this->itemsEdit.push_back(checkBox);
		break;
	}
	case NOWA::Variant::VAR_INT:
	{
		if (false == attribute->hasConstraints())
		{
			MyGUI::EditBox* edit = mWidgetClient->createWidget<MyGUI::EditBox>("EditBox", MyGUI::IntCoord(valueLeft, heightCurrent, valueWidth, height), MyGUI::Align::HStretch | MyGUI::Align::Top, name);
			if (true == allValuesSame)
			{
				edit->setOnlyText(Ogre::StringConverter::toString(attribute->getInt()));
			}
			else
			{
				edit->setOnlyText("0");
			}
			edit->setInvertSelected(false);
			edit->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
			edit->setEditReadOnly(attribute->isReadOnly());
			edit->setMouseHitThreshold(6, 6, 3, 3);
			// Really important to set the name of the property for the edit box, in order to identify later when a value has been changed, what to do
			// Store also if all values are the same
			edit->setUserData(MyGUI::Any(attribute));
			edit->eventEditSelectAccept += MyGUI::newDelegate(this, &PropertiesPanelDynamic::notifyEditSelectAccept);
			edit->eventMouseSetFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::setFocus);
			edit->eventMouseLostFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::mouseLostFocus);
			edit->eventRootMouseChangeFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::mouseRootChangeFocus);
			edit->eventEditTextChange += MyGUI::newDelegate(this, &PropertiesPanelDynamic::editTextChange);
			edit->eventMouseButtonPressed += MyGUI::newDelegate(this, &PropertiesPanelDynamic::onMouseClick);
			edit->eventKeyButtonPressed += MyGUI::newDelegate(this, &PropertiesPanelDynamic::onKeyButtonPressed);
			edit->setNeedKeyFocus(true);
			edit->setNeedMouseFocus(true);
			edit->getClientWidget()->eventMouseButtonDoubleClick += MyGUI::newDelegate(this, &PropertiesPanelDynamic::onMouseDoubleClick);

			if (true == attribute->hasUserDataKey(NOWA::GameObject::AttrActionReadOnly()))
			{
				edit->setEnabled(false);
			}

			this->itemsEdit.push_back(edit);
		}
		else
		{
			this->createIntSlider(valueWidth, valueLeft, height, name, attribute);
		}
		break;
	}
	case NOWA::Variant::VAR_UINT:
	{
		if (false == attribute->hasConstraints())
		{
			MyGUI::EditBox* edit = mWidgetClient->createWidget<MyGUI::EditBox>("EditBox", MyGUI::IntCoord(valueLeft, heightCurrent, valueWidth, height), MyGUI::Align::HStretch | MyGUI::Align::Top, name);
			if (true == allValuesSame)
			{
				edit->setOnlyText(Ogre::StringConverter::toString(attribute->getUInt()));
			}
			else
			{
				edit->setOnlyText("0");
			}
			edit->setEditReadOnly(attribute->isReadOnly());
			edit->setMouseHitThreshold(6, 6, 3, 3);
			if (NOWA::GameObject::AttrCategoryId() == attribute->getName() || NOWA::GameObject::AttrRenderCategoryId() == attribute->getName())
			{
				edit->setEditReadOnly(true);
			}
			edit->setInvertSelected(false);
			edit->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
			edit->setUserData(MyGUI::Any(attribute));
			edit->eventEditSelectAccept += MyGUI::newDelegate(this, &PropertiesPanelDynamic::notifyEditSelectAccept);
			edit->eventMouseSetFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::setFocus);
			edit->eventMouseLostFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::mouseLostFocus);
			edit->eventRootMouseChangeFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::mouseRootChangeFocus);
			edit->eventEditTextChange += MyGUI::newDelegate(this, &PropertiesPanelDynamic::editTextChange);
			edit->eventMouseButtonPressed += MyGUI::newDelegate(this, &PropertiesPanelDynamic::onMouseClick);
			edit->eventKeyButtonPressed += MyGUI::newDelegate(this, &PropertiesPanelDynamic::onKeyButtonPressed);
			edit->setNeedKeyFocus(true);
			edit->setNeedMouseFocus(true);
			edit->getClientWidget()->eventMouseButtonDoubleClick += MyGUI::newDelegate(this, &PropertiesPanelDynamic::onMouseDoubleClick);

			if (true == attribute->hasUserDataKey(NOWA::GameObject::AttrActionReadOnly()))
			{
				edit->setEnabled(false);
			}

			this->itemsEdit.push_back(edit);
		}
		else
		{
			this->createIntSlider(valueWidth, valueLeft, height, name, attribute);
		}
		break;
	}
	case NOWA::Variant::VAR_ULONG:
	{
		if (false == attribute->hasConstraints())
		{
			MyGUI::EditBox* edit = mWidgetClient->createWidget<MyGUI::EditBox>("EditBox", MyGUI::IntCoord(valueLeft, heightCurrent, valueWidth, height), MyGUI::Align::HStretch | MyGUI::Align::Top, name);
			if (true == allValuesSame)
			{
				edit->setOnlyText(Ogre::StringConverter::toString(attribute->getULong()));
			}
			else
			{
				edit->setOnlyText("0");
			}
			edit->setEditReadOnly(attribute->isReadOnly());
			edit->setMouseHitThreshold(6, 6, 3, 3);
			edit->setInvertSelected(false);
			edit->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
			edit->setUserData(MyGUI::Any(attribute));
			edit->eventEditSelectAccept += MyGUI::newDelegate(this, &PropertiesPanelDynamic::notifyEditSelectAccept);
			edit->eventMouseSetFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::setFocus);
			edit->eventMouseLostFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::mouseLostFocus);
			edit->eventRootMouseChangeFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::mouseRootChangeFocus);
			edit->eventEditTextChange += MyGUI::newDelegate(this, &PropertiesPanelDynamic::editTextChange);
			edit->eventMouseButtonPressed += MyGUI::newDelegate(this, &PropertiesPanelDynamic::onMouseClick);
			edit->eventKeyButtonPressed += MyGUI::newDelegate(this, &PropertiesPanelDynamic::onKeyButtonPressed);
			edit->setNeedKeyFocus(true);
			edit->setNeedMouseFocus(true);
			edit->getClientWidget()->eventMouseButtonDoubleClick += MyGUI::newDelegate(this, &PropertiesPanelDynamic::onMouseDoubleClick);

			if (true == attribute->hasUserDataKey(NOWA::GameObject::AttrActionReadOnly()))
			{
				edit->setEnabled(false);
			}

			this->itemsEdit.push_back(edit);
		}
		else
		{
			this->createIntSlider(valueWidth, valueLeft, height, name, attribute);
		}
		break;
	}
	case NOWA::Variant::VAR_REAL:
	{
		if (false == attribute->hasConstraints())
		{
			MyGUI::EditBox* edit = mWidgetClient->createWidget<MyGUI::EditBox>("EditBox", MyGUI::IntCoord(valueLeft, heightCurrent, valueWidth, height), MyGUI::Align::HStretch | MyGUI::Align::Top, name);
			if (true == allValuesSame)
			{
				attribute->setValue(NOWA::MathHelper::getInstance()->round(attribute->getReal(), 5));
				edit->setOnlyText(Ogre::StringConverter::toString(attribute->getReal()));
			}
			else
			{
				edit->setOnlyText("0");
			}
			edit->setInvertSelected(false);
			edit->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
			edit->setEditReadOnly(attribute->isReadOnly());
			edit->setMouseHitThreshold(6, 6, 3, 3);
			edit->setUserData(MyGUI::Any(attribute));
			edit->eventEditSelectAccept += MyGUI::newDelegate(this, &PropertiesPanelDynamic::notifyEditSelectAccept);
			edit->eventMouseSetFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::setFocus);
			edit->eventMouseLostFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::mouseLostFocus);
			edit->eventRootMouseChangeFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::mouseRootChangeFocus);
			edit->eventEditTextChange += MyGUI::newDelegate(this, &PropertiesPanelDynamic::editTextChange);
			edit->eventMouseButtonPressed += MyGUI::newDelegate(this, &PropertiesPanelDynamic::onMouseClick);
			edit->eventKeyButtonPressed += MyGUI::newDelegate(this, &PropertiesPanelDynamic::onKeyButtonPressed);
			edit->setNeedKeyFocus(true);
			edit->setNeedMouseFocus(true);
			edit->getClientWidget()->eventMouseButtonDoubleClick += MyGUI::newDelegate(this, &PropertiesPanelDynamic::onMouseDoubleClick);

			if (true == attribute->hasUserDataKey(NOWA::GameObject::AttrActionReadOnly()))
			{
				edit->setEnabled(false);
			}

			this->itemsEdit.push_back(edit);
		}
		else
		{
			this->createRealSlider(valueWidth, valueLeft, height, name, attribute);
		}
		break;
	}
	case NOWA::Variant::VAR_VEC2:
	{
		MyGUI::EditBox* edit = mWidgetClient->createWidget<MyGUI::EditBox>("EditBox", MyGUI::IntCoord(valueLeft, heightCurrent, valueWidth, height), MyGUI::Align::HStretch | MyGUI::Align::Top, name);
		if (true == allValuesSame)
		{
			attribute->setValue(NOWA::MathHelper::getInstance()->round(attribute->getVector2(), 5));
			edit->setOnlyText(Ogre::StringConverter::toString(attribute->getVector2()));
		}
		else
		{
			edit->setOnlyText("0 0");
		}
		edit->setInvertSelected(false);
		edit->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
		edit->setEditReadOnly(attribute->isReadOnly());
		edit->setMouseHitThreshold(6, 6, 3, 3);
		// edit->setTextAlign(MyGUI::Align::Left);
		edit->setUserData(MyGUI::Any(attribute));
		edit->eventEditSelectAccept += MyGUI::newDelegate(this, &PropertiesPanelDynamic::notifyEditSelectAccept);
		edit->eventMouseSetFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::setFocus);
		edit->eventMouseLostFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::mouseLostFocus);
		edit->eventRootMouseChangeFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::mouseRootChangeFocus);
		edit->eventEditTextChange += MyGUI::newDelegate(this, &PropertiesPanelDynamic::editTextChange);
		edit->eventMouseButtonPressed += MyGUI::newDelegate(this, &PropertiesPanelDynamic::onMouseClick);
		edit->eventKeyButtonPressed += MyGUI::newDelegate(this, &PropertiesPanelDynamic::onKeyButtonPressed);
		edit->setNeedKeyFocus(true);
		edit->setNeedMouseFocus(true);
		edit->getClientWidget()->eventMouseButtonDoubleClick += MyGUI::newDelegate(this, &PropertiesPanelDynamic::onMouseDoubleClick);

		if (true == attribute->hasUserDataKey(NOWA::GameObject::AttrActionReadOnly()))
		{
			edit->setEnabled(false);
		}

		this->itemsEdit.push_back(edit);
		break;
	}
	case NOWA::Variant::VAR_VEC3:
	{
		MyGUI::EditBox* edit = mWidgetClient->createWidget<MyGUI::EditBox>("EditBox", MyGUI::IntCoord(valueLeft, heightCurrent, valueWidth, height), MyGUI::Align::HStretch | MyGUI::Align::Top, name);
		if (true == allValuesSame)
		{
			// Round values up to 4 digits, so later when mouseLostFocus event is fired and the user just moved away from edit box so that the check between the attribute value and the
			// edit box value is the same and nothing is done
			attribute->setValue(NOWA::MathHelper::getInstance()->round(attribute->getVector3(), 5));
			edit->setOnlyText(Ogre::StringConverter::toString(attribute->getVector3()));
		}
		else
		{
			edit->setOnlyText("0 0 0");
		}
		edit->setInvertSelected(false);
		edit->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
		edit->setEditReadOnly(attribute->isReadOnly());
		edit->setMouseHitThreshold(6, 6, 3, 3);
		edit->setUserData(MyGUI::Any(attribute));
		edit->eventEditSelectAccept += MyGUI::newDelegate(this, &PropertiesPanelDynamic::notifyEditSelectAccept);
		edit->eventMouseSetFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::setFocus);
		edit->eventMouseLostFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::mouseLostFocus);
		edit->eventRootMouseChangeFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::mouseRootChangeFocus);
		edit->eventEditTextChange += MyGUI::newDelegate(this, &PropertiesPanelDynamic::editTextChange);
		edit->eventMouseButtonPressed += MyGUI::newDelegate(this, &PropertiesPanelDynamic::onMouseClick);
		edit->eventKeyButtonPressed += MyGUI::newDelegate(this, &PropertiesPanelDynamic::onKeyButtonPressed);
		edit->setNeedKeyFocus(true);
		edit->setNeedMouseFocus(true);
		edit->getClientWidget()->eventMouseButtonDoubleClick += MyGUI::newDelegate(this, &PropertiesPanelDynamic::onMouseDoubleClick);

		if (true == attribute->hasUserDataKey(NOWA::GameObject::AttrActionReadOnly()))
		{
			edit->setEnabled(false);
		}

		this->itemsEdit.push_back(edit);

		if (true == attribute->hasUserDataKey(NOWA::GameObject::AttrActionColorDialog()))
		{
			MyGUI::Button* button = mWidgetClient->createWidget<MyGUI::Button>(MyGUI::WidgetStyle::Overlapped, "Button", MyGUI::IntCoord(valueLeft - 25, heightCurrent + 1, 20, 20), MyGUI::Align::Default, "Main", name + "button");
			button->setTextColour(MyGUIHelper::getInstance()->getImportantTextColour());
			button->setCaption("..");
			button->setUserData(MyGUI::Any(attribute));
			button->eventMouseButtonClick += MyGUI::newDelegate(this, &PropertiesPanelDynamic::buttonHit);
			this->itemsEdit.push_back(button);
		}
		break;
	}
	case NOWA::Variant::VAR_VEC4:
	{
		MyGUI::EditBox* edit = mWidgetClient->createWidget<MyGUI::EditBox>("EditBox", MyGUI::IntCoord(valueLeft, heightCurrent, valueWidth, height), MyGUI::Align::HStretch | MyGUI::Align::Top, name);
		if (true == allValuesSame)
		{
			attribute->setValue(NOWA::MathHelper::getInstance()->round(attribute->getVector4(), 5));
			edit->setOnlyText(Ogre::StringConverter::toString(attribute->getVector4()));
		}
		else
		{
			edit->setOnlyText("0 0 0 0");
		}
		edit->setInvertSelected(false);
		edit->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
		edit->setEditReadOnly(attribute->isReadOnly());
		edit->setMouseHitThreshold(6, 6, 3, 3);
		// edit->setTextAlign(MyGUI::Align::Left);
		edit->setUserData(MyGUI::Any(attribute));
		edit->eventEditSelectAccept += MyGUI::newDelegate(this, &PropertiesPanelDynamic::notifyEditSelectAccept);
		edit->eventMouseSetFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::setFocus);
		edit->eventMouseLostFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::mouseLostFocus);
		edit->eventRootMouseChangeFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::mouseRootChangeFocus);
		edit->eventEditTextChange += MyGUI::newDelegate(this, &PropertiesPanelDynamic::editTextChange);
		edit->eventMouseButtonPressed += MyGUI::newDelegate(this, &PropertiesPanelDynamic::onMouseClick);
		edit->eventKeyButtonPressed += MyGUI::newDelegate(this, &PropertiesPanelDynamic::onKeyButtonPressed);
		edit->setNeedKeyFocus(true);
		edit->setNeedMouseFocus(true);
		edit->getClientWidget()->eventMouseButtonDoubleClick += MyGUI::newDelegate(this, &PropertiesPanelDynamic::onMouseDoubleClick);

		if (true == attribute->hasUserDataKey(NOWA::GameObject::AttrActionReadOnly()))
		{
			edit->setEnabled(false);
		}

		this->itemsEdit.push_back(edit);

		if (true == attribute->hasUserDataKey(NOWA::GameObject::AttrActionColorDialog()))
		{
			MyGUI::Button* button = mWidgetClient->createWidget<MyGUI::Button>(MyGUI::WidgetStyle::Overlapped, "Button", MyGUI::IntCoord(valueLeft - 25, heightCurrent + 1, 20, 20), MyGUI::Align::Default, "Main", name + "button");
			button->setTextColour(MyGUIHelper::getInstance()->getImportantTextColour());
			button->setCaption("..");
			button->setUserData(MyGUI::Any(attribute));
			button->eventMouseButtonClick += MyGUI::newDelegate(this, &PropertiesPanelDynamic::buttonHit);
			this->itemsEdit.push_back(button);
		}
		break;
	}
	case NOWA::Variant::VAR_LIST:
	{
		if (false == attribute->hasUserDataKey(NOWA::GameObject::AttrActionImage()))
		{
			MyGUI::ComboBox* comboBox = mWidgetClient->createWidget<MyGUI::ComboBox>("ComboBox", MyGUI::IntCoord(valueLeft, heightCurrent, valueWidth - 15, height), MyGUI::Align::HStretch | MyGUI::Align::Top, name);
			comboBox->setTextColour(MyGUIHelper::getInstance()->getTextSelectColour());
			comboBox->setMouseHitThreshold(6, 6, 3, 3);

			if (attribute->getName() == NOWA::GameObject::AttrCategory())
			{
				if (this->gameObjects.size() > 1)
				{
					comboBox->setEditStatic(false);
				}

				// Set all available categories + ability to add a new category
				std::vector<Ogre::String> allCategories = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getAllCategoriesSoFar();
				if (allCategories.size() > 0)
				{
					for (auto& category : allCategories)
					{
						comboBox->addItem(category);
					}
				}
				this->heightCurrent += heightStep;

				MyGUI::TextBox* keyTextBox = mWidgetClient->createWidget<MyGUI::TextBox>("TextBox", MyGUI::IntCoord(keyLeft, heightCurrent, keyWidth, height), MyGUI::Align::Left | MyGUI::Align::Top);
				keyTextBox->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
				keyTextBox->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
				keyTextBox->setCaption("New Category:");
				this->itemsText.push_back(keyTextBox);

				MyGUI::EditBox* edit = mWidgetClient->createWidget<MyGUI::EditBox>("EditBox", MyGUI::IntCoord(valueLeft, heightCurrent, valueWidth, height), MyGUI::Align::HStretch | MyGUI::Align::Top, name);
				edit->setInvertSelected(false);
				edit->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
				edit->setMouseHitThreshold(6, 6, 3, 3);
				edit->setUserData(MyGUI::Any(attribute));
				edit->eventEditSelectAccept += MyGUI::newDelegate(this, &PropertiesPanelDynamic::notifyEditSelectAccept);
				edit->eventMouseSetFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::setFocus);
				edit->eventMouseButtonPressed += MyGUI::newDelegate(this, &PropertiesPanelDynamic::onMouseClick);
				edit->eventKeyButtonPressed += MyGUI::newDelegate(this, &PropertiesPanelDynamic::onKeyButtonPressed);
				edit->setNeedKeyFocus(true);
				edit->setNeedMouseFocus(true);
				// No lost focus for list, because the new is empty but in attribute a content may be exist like 'default', so when mouse hovering above a list will trigger undo commands
				// // edit->eventMouseLostFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::mouseLostFocus);

				if (true == attribute->hasUserDataKey(NOWA::GameObject::AttrActionReadOnly()))
				{
					edit->setEnabled(false);
				}

				this->itemsEdit.push_back(edit);
			}
			else if (attribute->getName() == NOWA::GameObject::AttrRenderCategory())
			{
				if (this->gameObjects.size() > 1)
				{
					comboBox->setEditStatic(false);
				}

				// Set all available categories + ability to add a new render category
				std::vector<Ogre::String> allRenderCategories = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getAllRenderCategoriesSoFar();
				if (allRenderCategories.size() > 0)
				{
					for (auto& renderCategory : allRenderCategories)
					{
						comboBox->addItem(renderCategory);
					}
				}
				this->heightCurrent += heightStep;

				MyGUI::TextBox* keyTextBox = mWidgetClient->createWidget<MyGUI::TextBox>("TextBox", MyGUI::IntCoord(keyLeft, heightCurrent, keyWidth, height), MyGUI::Align::Left | MyGUI::Align::Top);
				keyTextBox->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
				keyTextBox->setTextAlign(MyGUI::Align::Left | MyGUI::Align::VCenter);
				keyTextBox->setCaption("New Render Category:");
				this->itemsText.push_back(keyTextBox);

				MyGUI::EditBox* edit = mWidgetClient->createWidget<MyGUI::EditBox>("EditBox", MyGUI::IntCoord(valueLeft, heightCurrent, valueWidth, height), MyGUI::Align::HStretch | MyGUI::Align::Top, name);
				edit->setInvertSelected(false);
				edit->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
				edit->setMouseHitThreshold(6, 6, 3, 3);
				edit->setUserData(MyGUI::Any(attribute));
				edit->eventEditSelectAccept += MyGUI::newDelegate(this, &PropertiesPanelDynamic::notifyEditSelectAccept);
				edit->eventMouseSetFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::setFocus);
				edit->eventMouseButtonPressed += MyGUI::newDelegate(this, &PropertiesPanelDynamic::onMouseClick);
				edit->eventKeyButtonPressed += MyGUI::newDelegate(this, &PropertiesPanelDynamic::onKeyButtonPressed);
				edit->setNeedKeyFocus(true);
				edit->setNeedMouseFocus(true);
				// No lost focus for list, because the new is empty but in attribute a content may be exist like 'default', so when mouse hovering above a list will trigger undo commands
				// // edit->eventMouseLostFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::mouseLostFocus);

				if (true == attribute->hasUserDataKey(NOWA::GameObject::AttrActionReadOnly()))
				{
					edit->setEnabled(false);
				}

				this->itemsEdit.push_back(edit);
			}
			else
			{
#if 0
				// Set all available categories in read only, if the keyword is found e.g. Repeller Category
				size_t found = attribute->getName().find("Category");
				if (Ogre::String::npos != found)
				{
					std::vector<Ogre::String> allCategories = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getAllCategoriesSoFar();
					if (allCategories.size() > 0)
					{
						for (auto& category : allCategories)
						{
							comboBox->addItem(category);
						}
					}
				}
				else
#endif
				{
					for (unsigned int i = 0; i < static_cast<unsigned int>(attribute->getList().size()); i++)
					{
						comboBox->addItem(attribute->getList()[i]);
					}
				}
			}

			comboBox->setOnlyText(attribute->getListSelectedValue());

			comboBox->setEditReadOnly(true);
			// edit->setTextAlign(MyGUI::Align::Left);
			comboBox->setUserData(MyGUI::Any(attribute));
			comboBox->eventEditSelectAccept += MyGUI::newDelegate(this, &PropertiesPanelDynamic::notifyEditSelectAccept);
			comboBox->eventComboChangePosition += MyGUI::newDelegate(this, &PropertiesPanelDynamic::notifyComboChangedPosition);
			comboBox->eventMouseSetFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::setFocus);
			comboBox->eventKeyButtonPressed += MyGUI::newDelegate(this, &PropertiesPanelDynamic::onKeyButtonPressed);
			comboBox->setNeedKeyFocus(true);
			comboBox->setNeedMouseFocus(true);

			if (true == attribute->hasUserDataKey(NOWA::GameObject::AttrActionReadOnly()))
			{
				comboBox->setEnabled(false);
			}

			this->itemsEdit.push_back(comboBox);
		}
		else
		{
			MyGUI::ItemBox* itemBox = mWidgetClient->createWidget<MyGUI::ItemBox>("ItemBox", MyGUI::IntCoord(valueLeft, heightCurrent, valueWidth - 20, 700), MyGUI::Align::HStretch | MyGUI::Align::Top, name);
			// itemBox->setVerticalAlignment(true);
			// https://github.com/MyGUI/mygui/blob/master/UnitTests/UnitTest_ItemBox_Info/DemoKeeper.cpp
			itemBox->requestCoordItem = MyGUI::newDelegate(this, &PropertiesPanelDynamic::requestCoordItem);
			itemBox->requestCreateWidgetItem = MyGUI::newDelegate(this, &PropertiesPanelDynamic::requestCreateWidgetItem);
			itemBox->requestDrawItem = MyGUI::newDelegate(this, &PropertiesPanelDynamic::requestDrawItem);
			itemBox->eventKeyButtonPressed += MyGUI::newDelegate(this, &PropertiesPanelDynamic::onKeyButtonPressed);
			itemBox->setNeedKeyFocus(true);
			itemBox->setNeedMouseFocus(true);

			this->heightCurrent += 700;

			itemBox->setUserData(MyGUI::Any(attribute));

			for (unsigned int i = 0; i < static_cast<unsigned int>(attribute->getList().size()); i++)
			{
				auto item = attribute->getList().at(i);
				if (false == item.empty())
				{
					itemBox->addItem(item);
				}
			}
			/*itemBox->setSize(itemBox->getSize().width - 1, itemBox->getSize().height - 1);
			itemBox->setSize(itemBox->getSize().width + 1, itemBox->getSize().height + 1);

			itemBox->redrawAllItems();
			*/
			this->itemsEdit.push_back(itemBox);
		}
		break;
	}
	default:
	{
		// String
		MyGUI::EditBox* edit = mWidgetClient->createWidget<MyGUI::EditBox>("EditBox", MyGUI::IntCoord(valueLeft, heightCurrent, valueWidth, height), MyGUI::Align::HStretch | MyGUI::Align::Top, name);
		edit->setInvertSelected(false);
		edit->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
		edit->setMouseHitThreshold(6, 6, 3, 3);

		if (true == attribute->hasUserDataKey(NOWA::GameObject::AttrActionReadOnly()))
		{
			edit->setEnabled(false);
		}

		if (true == allValuesSame)
		{
			edit->setOnlyText(attribute->getString());
			// Move the cursor to the beginning (index 0)
			edit->setTextCursor(0);
		}
		else
		{
			edit->setOnlyText("-several-");
		}
		// If its the name attribute of game object and several are selected, set read only
		if (this->gameObjects.size() > 1)
		{
			if (NOWA::GameObject::AttrName() == attribute->getName())
			{
				edit->setEditReadOnly(true);
			}
		}

		if (true == attribute->hasUserDataKey(NOWA::GameObject::AttrActionMultiLine()))
		{
			edit->setEditMultiLine(true);
			edit->setTextAlign(MyGUI::Align::Left | MyGUI::Align::Top);
			edit->setVisibleVScroll(true);
			edit->setVisibleHScroll(true);
			edit->showHScroll(true);
			edit->showVScroll(true);
			edit->setEditWordWrap(true);
			/*
			Hmm, and what you expected wit this?.. VScroll is not designed to be parent widget. You need to create two separate widgets (Edit and Vscroll near it) and add event handler for eventScrollChangePosition and scroll your text with it. You also can use mouse wheel in Edit :)
			aclysma wrote:
			1) The HScroll does not seem to affect the edit box. I tried with the HScroll on the outside and inside of the edit box.
			Why it should affect? It's two separate widgets.
			aclysma wrote:
			2) Auto-scroll. I am assuming I need to check that the edit box cursor index equals the length of text in the edit box. If this is true and I add more text, then I need to put the cursor at the end or somehow tell the scrollbar to scroll the full way down. I haven't looked too much into this as #1 has me stuck for now :)
			Well, we have such functionality for List, we'll add Scroll to Edit, but later.
			*/
			edit->setSize(edit->getWidth(), heightStep * 5);
			this->heightCurrent += (heightStep * 4);
		}

		edit->setUserData(MyGUI::Any(attribute));
		edit->eventEditSelectAccept += MyGUI::newDelegate(this, &PropertiesPanelDynamic::notifyEditSelectAccept);
		edit->eventMouseSetFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::setFocus);
		edit->eventMouseLostFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::mouseLostFocus);
		edit->eventRootMouseChangeFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::mouseRootChangeFocus);
		edit->eventEditTextChange += MyGUI::newDelegate(this, &PropertiesPanelDynamic::editTextChange);
		edit->eventMouseButtonPressed += MyGUI::newDelegate(this, &PropertiesPanelDynamic::onMouseClick);
		edit->eventKeyButtonPressed += MyGUI::newDelegate(this, &PropertiesPanelDynamic::onKeyButtonPressed);

		edit->setNeedKeyFocus(true);
		edit->setNeedMouseFocus(true);
		edit->getClientWidget()->eventMouseButtonDoubleClick += MyGUI::newDelegate(this, &PropertiesPanelDynamic::onMouseDoubleClick);

		// edit->setMaxTextLength(15);
		this->itemsEdit.push_back(edit);

		if (true == attribute->hasUserDataKey(NOWA::GameObject::AttrActionFileOpenDialog()))
		{
			MyGUI::Button* button = mWidgetClient->createWidget<MyGUI::Button>(MyGUI::WidgetStyle::Overlapped, "Button", MyGUI::IntCoord(valueLeft - 25, heightCurrent + 1, 20, 20), MyGUI::Align::Default, "Main", name + "button");
			button->setTextColour(MyGUIHelper::getInstance()->getImportantTextColour());
			button->setCaption("..");
			button->setUserData(MyGUI::Any(attribute));
			button->eventMouseButtonClick += MyGUI::newDelegate(this, &PropertiesPanelDynamic::buttonHit);
			this->itemsEdit.push_back(button);
		}
		else if (true == attribute->hasUserDataKey(NOWA::GameObject::AttrActionGenerateLuaFunction()))
		{
			MyGUI::Button* button = mWidgetClient->createWidget<MyGUI::Button>(MyGUI::WidgetStyle::Overlapped, "Button", MyGUI::IntCoord(valueLeft - 25, heightCurrent + 1, 20, 20), MyGUI::Align::Default, "Main", name + "button");
			button->setTextColour(MyGUIHelper::getInstance()->getImportantTextColour());
			button->setDepth(0);
			button->setCaption("G");
			button->setUserData(MyGUI::Any(attribute));
			button->eventMouseButtonClick += MyGUI::newDelegate(this, &PropertiesPanelDynamic::buttonHit);
			this->itemsEdit.push_back(button);
		}
		else if (true == attribute->hasUserDataKey(NOWA::GameObject::AttrActionLuaScript()))
		{
			MyGUI::Button* button = mWidgetClient->createWidget<MyGUI::Button>(MyGUI::WidgetStyle::Overlapped, "Button", MyGUI::IntCoord(valueLeft - 25, heightCurrent + 1, 20, 20), MyGUI::Align::Default, "Main", name + "button");
			button->setTextColour(MyGUIHelper::getInstance()->getImportantTextColour());
			button->setCaption("L");
			button->setUserData(MyGUI::Any(attribute));
			button->eventMouseButtonClick += MyGUI::newDelegate(this, &PropertiesPanelDynamic::buttonHit);
			this->itemsEdit.push_back(button);
		}
		break;
	}
	}

	if (true == attribute->hasUserDataKey(NOWA::GameObject::AttrActionSeparator()))
	{
		this->heightCurrent += heightStep;
		this->itemsEdit.push_back(this->addSeparator());
		this->heightCurrent += heightStep / 2;
	}

	this->heightCurrent += heightStep;
	mPanelCell->setClientHeight(heightCurrent);
}

void PropertiesPanelDynamic::createRealSlider(const int& valueWidth, const int& valueLeft, const int& height, const Ogre::String& name, NOWA::Variant*& attribute)
{
	// If constraints, create slider and edit box
	int sliderWidth = valueWidth + 20;
	MyGUI::ScrollBar* slider = mWidgetClient->createWidget<MyGUI::ScrollBar>("SliderH", MyGUI::IntCoord(valueLeft, heightCurrent + 1, sliderWidth, height - 4), MyGUI::Align::Left | MyGUI::Align::Top, "slider" + name);
	// slider->setScrollPage(1);
	// slider->setScrollWheelPage(1);
	slider->setMoveToClick(true);

	if (true == attribute->hasUserDataKey(NOWA::GameObject::AttrActionReadOnly()))
	{
		slider->setEnabled(false);
	}

	Ogre::Real currentValue = attribute->getReal();
	Ogre::Real lowBorder = attribute->getConstraints().first;
	Ogre::Real highBorder = attribute->getConstraints().second;

#if 0

	// Scroll range must be always one more (see internal implementation of scroll bar)
	Ogre::Real range = (Ogre::Math::Abs(highBorder - lowBorder) * 1000.0f) + 1.0f;
	
	// https://math.stackexchange.com/questions/51509/how-to-calculate-percentage-of-value-inside-arbitrary-range
	// Note: transform from - to absolute, only - is relevant, e.g. min: -200, max: 100: value a) -97, value b) 97 -> transform: +200 -> min: 0 max: 300 value a) 103, value b: 297

	Ogre::Real value = (currentValue - lowBorder) * 1000.0f;
	if (value >= range)
	{
		value = range - 1;
	}

	Ogre::Real interpolatedValue = NOWA::Interpolator::getInstance()->linearInterpolation(value * 1000.0f, lowBorder * 1000.0f, highBorder * 1000.0f, lowBorder * 1000.0f, highBorder * 1000.0f);
	slider->setScrollRange(static_cast<size_t>(range));
	slider->setScrollPosition(static_cast<size_t>(interpolatedValue));
	slider->setTrackSize(10);

	// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "-->SliderValue: " + Ogre::StringConverter::toString(interpolatedValue));

	// slider->setTickStep((10.0f / range) * 1000.0f); Missing in MyGUI
	slider->eventScrollChangePosition += MyGUI::newDelegate(this, &PropertiesPanelDynamic::notifyScrollChangePosition);
	slider->eventMouseButtonReleased += MyGUI::newDelegate(this, &PropertiesPanelDynamic::notifySliderMouseRelease);
	// slider->eventMouseButtonPressed += MyGUI::newDelegate(this, &PropertiesPanelDynamic::onMouseClick);
#else
	// Ensure min is less than max
	if (lowBorder > highBorder)
	{
		std::swap(lowBorder, highBorder);
	}

	// Interpolate current value to an integer within the slider range
	// Calculate the range based on the min and max values
	const float valueRange = highBorder - lowBorder;
	const size_t range = 1000; // Precision of the slider

	// Interpolate current value to an integer within the slider range
	const size_t interpolatedValue = static_cast<size_t>((currentValue - lowBorder) / valueRange * range);

	// Set the slider's properties
	slider->setScrollRange(range + 1); // Set the range of the slider
	slider->setScrollPosition(interpolatedValue); // Set initial position
	slider->setTrackSize(10);

	// Set the slider event listener to handle changes
	slider->eventScrollChangePosition += MyGUI::newDelegate(this, &PropertiesPanelDynamic::notifyScrollChangePosition);
	slider->eventMouseButtonReleased += MyGUI::newDelegate(this, &PropertiesPanelDynamic::notifySliderMouseRelease);
#endif

	MyGUI::EditBox* edit = mWidgetClient->createWidget<MyGUI::EditBox>("EditBox", MyGUI::IntCoord(valueLeft + sliderWidth + 5, heightCurrent, valueWidth * 0.3f, height), MyGUI::Align::HStretch | MyGUI::Align::Top, name);
	edit->setInvertSelected(false);
	edit->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
	edit->setMouseHitThreshold(6, 6, 3, 3);
	edit->setOnlyText(Ogre::StringConverter::toString(attribute->getReal()));
	edit->setUserData(MyGUI::Any(attribute));
	edit->eventEditSelectAccept += MyGUI::newDelegate(this, &PropertiesPanelDynamic::notifyEditSelectAccept);
	edit->eventMouseSetFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::setFocus);
	// edit->eventMouseLostFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::mouseLostFocus);
	edit->eventRootMouseChangeFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::mouseRootChangeFocus);
	edit->eventEditTextChange += MyGUI::newDelegate(this, &PropertiesPanelDynamic::editTextChange);
	edit->eventMouseButtonPressed += MyGUI::newDelegate(this, &PropertiesPanelDynamic::onMouseClick);

	// Pair the edit to the scrollbar and vice versa
	slider->setUserData(edit);
	edit->setUserString("slider" + name, "slider" + name);

	this->itemsEdit.push_back(slider);
	this->itemsEdit.push_back(edit);
}

void PropertiesPanelDynamic::createIntSlider(const int& valueWidth, const int& valueLeft, const int& height, const Ogre::String& name, NOWA::Variant*& attribute)
{
	// If constraints, create slider and edit box
	int sliderWidth = valueWidth + 20;
	MyGUI::ScrollBar* slider = mWidgetClient->createWidget<MyGUI::ScrollBar>("SliderH", MyGUI::IntCoord(valueLeft, heightCurrent + 1, sliderWidth, height - 4), MyGUI::Align::Left | MyGUI::Align::Top, "slider" + name);
	// slider->setScrollPage(1);
	// slider->setScrollWheelPage(1);
	slider->setMoveToClick(true);

	if (true == attribute->hasUserDataKey(NOWA::GameObject::AttrActionReadOnly()))
	{
		slider->setEnabled(false);
	}
	
	Ogre::Real currentValue = attribute->getReal();
	Ogre::Real lowBorder = attribute->getConstraints().first;
	Ogre::Real highBorder = attribute->getConstraints().second;

	// Ensure min is less than max
	if (lowBorder > highBorder)
	{
		std::swap(lowBorder, highBorder);
	}

	// Calculate the range for the slider
	const size_t range = static_cast<size_t>(highBorder - lowBorder);

	// Calculate the current position within the range
	const size_t currentPosition = static_cast<size_t>(currentValue - lowBorder);
	// Set the slider's properties
	slider->setScrollRange(range + 1); // Range is inclusive, so add 1
	slider->setScrollPosition(currentPosition);
	slider->setTrackSize(10);
	slider->eventScrollChangePosition += MyGUI::newDelegate(this, &PropertiesPanelDynamic::notifyScrollChangePosition);
	slider->eventMouseButtonReleased += MyGUI::newDelegate(this, &PropertiesPanelDynamic::notifySliderMouseRelease);

	MyGUI::EditBox* edit = mWidgetClient->createWidget<MyGUI::EditBox>("EditBox", MyGUI::IntCoord(valueLeft + sliderWidth + 5, heightCurrent, valueWidth * 0.3f, height), MyGUI::Align::HStretch | MyGUI::Align::Top, name);
	edit->setInvertSelected(false);
	edit->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());
	edit->setMouseHitThreshold(6, 6, 3, 3);
	edit->setOnlyText(Ogre::StringConverter::toString(attribute->getReal()));
	edit->setUserData(MyGUI::Any(attribute));
	edit->eventEditSelectAccept += MyGUI::newDelegate(this, &PropertiesPanelDynamic::notifyEditSelectAccept);
	edit->eventMouseSetFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::setFocus);
	// edit->eventMouseLostFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::mouseLostFocus);
	edit->eventRootMouseChangeFocus += MyGUI::newDelegate(this, &PropertiesPanelDynamic::mouseRootChangeFocus);
	edit->eventEditTextChange += MyGUI::newDelegate(this, &PropertiesPanelDynamic::editTextChange);


	edit->eventMouseButtonPressed += MyGUI::newDelegate(this, &PropertiesPanelDynamic::onMouseClick);

	// Pair the edit to the scrollbar and vice versa
	slider->setUserData(edit);
	edit->setUserString("slider" + name, "slider" + name);

	this->itemsEdit.push_back(slider);
	this->itemsEdit.push_back(edit);
}

void PropertiesPanelDynamic::requestCoordItem(MyGUI::ItemBox* sender, MyGUI::IntCoord& coord, bool drag)
{
	coord.set(0, 0, 68, 68);
}

void PropertiesPanelDynamic::requestCreateWidgetItem(MyGUI::ItemBox* sender, MyGUI::Widget* item)
{
	ImageData* imageData = new ImageData();

	MyGUI::ImageBox* imageBoxItem = item->createWidget<MyGUI::ImageBox>("ImageBox", MyGUI::IntCoord(2, 2, 64, 64 /*item->getWidth(), item->getHeight()*/), MyGUI::Align::Stretch);
	// Layer: Main, so that it will be selectable because its above the image box item
	MyGUI::ImageBox* imageBoxBack = item->createWidget<MyGUI::ImageBox>(MyGUI::WidgetStyle::Child, "ImageBox", MyGUI::IntCoord(0, 0, 68, 68 /*item->getWidth(), item->getHeight()*/), MyGUI::Align::Stretch, "Main");

	imageBoxItem->setAlpha(0.95f);
	// imageBoxBack->setAlpha(0.9f);
	imageData->setImageBoxBack(imageBoxBack);
	imageData->setImageBoxItem(imageBoxItem);

	imageBoxBack->setNeedMouseFocus(false);
	imageBoxItem->setNeedMouseFocus(false);
	item->setUserData(imageData);
}

void PropertiesPanelDynamic::requestDrawItem(MyGUI::ItemBox* sender, MyGUI::Widget* item, const MyGUI::IBDrawItemInfo& info)
{
	ImageData* data = *item->getUserData<ImageData*>(false);

	if (nullptr == data)
	{
		return;
	}

	Ogre::String resourceName = *sender->getItemDataAt<Ogre::String>(info.index);

	if (info.update)
	{
		data->setResourceName(resourceName);

		static MyGUI::ResourceImageSetPtr resourceBack = nullptr;
		if (resourceBack == nullptr)
			resourceBack = MyGUI::ResourceManager::getInstance().getByName("pic_ItemBackImage")->castType<MyGUI::ResourceImageSet>(false);

		if (nullptr == resourceBack)
			return;

		data->getImageBoxBack()->setItemResourcePtr(resourceBack);
		data->getImageBoxBack()->setItemGroup("States");
	}

	if (info.active)
	{
		if (info.select)
		{
			data->getImageBoxBack()->setItemName("Select");
		}
		else
		{
			data->getImageBoxBack()->setItemName("Active");
		}
	}
	else if (info.select)
	{
		data->getImageBoxBack()->setItemName("Pressed");

		// Is implemented by game object and component, to react differently
		this->notifySetItemBoxData(sender, resourceName);
	}
	else
	{
		data->getImageBoxBack()->setItemName("Normal");
	}

	data->getImageBoxItem()->setItemName("Normal");
}

void PropertiesPanelDynamic::setFocus(MyGUI::Widget* sender, MyGUI::Widget* oldWidget)
{
	this->showDescription(sender);

	MyGUI::EditBox* editBox = sender->castType<MyGUI::EditBox>(false);
	if (nullptr != editBox)
	{
		Ogre::String name = editBox->getName();
		if (true == NOWA::InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(OIS::KC_LMENU))
		{
			NOWA::Variant** attribute = sender->getUserData<NOWA::Variant*>(false);
			if (attribute != nullptr)
			{
				auto connectedGameObject = NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId((*attribute)->getULong());
				if (nullptr != connectedGameObject)
				{
					editBox->setUserString("tooltip", connectedGameObject->getName());
				}
				MyGUIHelper::getInstance()->setDataForPairing(editBox, *attribute);
			}
		}
	}
}

void PropertiesPanelDynamic::mouseLostFocus(MyGUI::Widget* sender, MyGUI::Widget* oldWidget)
{
	// Does not work correctly
	/*MyGUI::EditBox* editBox = static_cast<MyGUI::EditBox*>(sender);
	if (nullptr != editBox)
	{
		Ogre::String adapted = sender->getUserString("Adapted");
		if ("true" == adapted)
		{
			this->notifyEditSelectAccept(editBox);
			sender->setUserString("Adapted", "false");
		}
		editBox->setTextSelection(0, 0);
	}*/
}

void PropertiesPanelDynamic::mouseRootChangeFocus(MyGUI::Widget* sender, bool bFocus)
{
	// When leaving an edit widget it loses the focus, so store the value
	if (false == bFocus)
	{
		MyGUI::EditBox* editBox = sender->castType<MyGUI::EditBox>(false);
		if (nullptr != editBox)
		{
			Ogre::String adapted = sender->getUserString("Adapted");
			if ("true" == adapted)
			{
				this->notifyEditSelectAccept(editBox);
				sender->setUserString("Adapted", "false");
			}
		}
	}
}

void PropertiesPanelDynamic::onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode code, MyGUI::Char c)
{
	// Causes Schmaddel, as soon as Shift is pressed + Char
#if 0
	MyGUI::EditBox* editBox = sender->castType<MyGUI::EditBox>(false);
	if (nullptr != editBox)
	{
		size_t selectionStart = editBox->getTextSelectionStart();
		size_t selectionEnd = editBox->getTextSelectionEnd();
		size_t selectionLength = editBox->getTextSelectionLength();

		// When a whole text has been selected
		if (selectionStart != MyGUI::ITEM_NONE && selectionEnd != MyGUI::ITEM_NONE)
		{
			editBox->eraseText(selectionStart, selectionEnd - selectionStart);
		}
		else if (selectionLength != MyGUI::ITEM_NONE && selectionLength > 1)
		{
			editBox->eraseText(0, selectionLength);
		}
	}
#endif
	
	if (GetAsyncKeyState(VK_LCONTROL) && code == MyGUI::KeyCode::V)
	{
		MyGUI::EditBox* editBox = sender->castType<MyGUI::EditBox>(false);
		if (nullptr != editBox)
		{
			this->notifyEditSelectAccept(editBox);
			MyGUIHelper::getInstance()->adaptFocus(sender, MyGUI::KeyCode::Return, this->itemsEdit);
		}
	}
	// Adds new line if shift + enter is pressed
	else if (MyGUI::InputManager::getInstance().isShiftPressed() && code == MyGUI::KeyCode::Return)
	{
		ENQUEUE_RENDER_COMMAND_MULTI("PropertiesPanelDynamic::onKeyButtonPressed", _1(sender),
		{
			MyGUI::EditBox * editBox = sender->castType<MyGUI::EditBox>(false);
			if (nullptr != editBox && true == editBox->getEditMultiLine())
			{
				//// Insert a new line at the current cursor position
				size_t cursorPos = editBox->getTextCursor();
				Ogre::String text = editBox->getCaption();
				text.insert(cursorPos + 1, "\\n");

				// Update the EditBox content and set the cursor position
				editBox->setCaptionWithReplacing(text);
				editBox->setTextCursor(cursorPos + 1);  // Move the cursor after the new line
			}
		});
	}
	else
	{
		MyGUIHelper::getInstance()->adaptFocus(sender, code, this->itemsEdit);
	}
}

void PropertiesPanelDynamic::editTextChange(MyGUI::Widget* sender)
{
	sender->setUserString("Adapted", "true");

	// If user is entering something, do not move camera, if the user entered something like asdf
	NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setMoveCameraWeight(0.0f);
	NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setRotateCameraWeight(0.0f);
}

void PropertiesPanelDynamic::onMouseDoubleClick(MyGUI::Widget* sender)
{
	if (nullptr != sender->getParent())
	{
		MyGUI::EditBox* editBox = sender->getParent()->castType<MyGUI::EditBox>(false);
		if (nullptr != editBox)
		{
			ENQUEUE_RENDER_COMMAND_MULTI("PropertiesPanelDynamic::onMouseDoubleClick", _1(editBox),
			{
				editBox->setTextSelection(0, editBox->getCaption().size());
			});
		}
	}
}

void PropertiesPanelDynamic::onMouseClick(MyGUI::Widget* sender, int left, int top, MyGUI::MouseButton id)
{
	// this->showDescription(sender);
	// Resets text selection of other edit boxes if focus to a current one is set.
	MyGUIHelper::getInstance()->resetTextSelection(sender, this->itemsEdit);
}

void PropertiesPanelDynamic::showDescription(MyGUI::Widget* sender)
{
	// Send the text box change to the game object and internally actualize the data
	NOWA::Variant** attribute = sender->getUserData<NOWA::Variant*>(false);
	if (attribute != nullptr)
	{
		this->propertiesPanelInfo->setInfo((*attribute)->getDescription());
	}
	else
	{
		Ogre::String description = sender->getUserString("Description");
		this->propertiesPanelInfo->setInfo(description);
	}
}

MyGUI::Widget* PropertiesPanelDynamic::addSeparator(void)
{
	const int height = 4;
	const int widthStep = 3;

	const int left = widthStep;
	const int width = static_cast<int>(mWidgetClient->getWidth());

	// Set the key of the property
	MyGUI::Widget* separator = mWidgetClient->createWidget<MyGUI::Widget>("Separator3", MyGUI::IntCoord(left, this->heightCurrent, width, height), MyGUI::Align::HStretch | MyGUI::Align::Top);

	return separator;
}

void PropertiesPanelDynamic::notifyColourCancel(MyGUI::ColourPanel* sender)
{
	ENQUEUE_RENDER_COMMAND_MULTI("PropertiesPanelDynamic::notifyColourCancel", _1(sender),
	{
		MyGUI::InputManager::getInstancePtr()->removeWidgetModal(ColourPanelManager::getInstance()->getColourPanel()->getWidget());
		ColourPanelManager::getInstance()->getColourPanel()->setVisible(false);
	});
}

bool PropertiesPanelDynamic::showFileOpenDialog(const Ogre::String& action, const Ogre::String& fileMask, Ogre::String& resourceGroupName)
{
	if (true == resourceGroupName.empty())
	{
		// See: [Models] in NOWA_Design.cfg
		resourceGroupName = "Models";
	}

	Ogre::String targetFolder;

	try
	{
		Ogre::ResourceGroupManager::LocationList resLocationsList = Ogre::ResourceGroupManager::getSingleton().getResourceLocationList(resourceGroupName);
		Ogre::ResourceGroupManager::LocationList::const_iterator it = resLocationsList.cbegin();
		Ogre::ResourceGroupManager::LocationList::const_iterator itEnd = resLocationsList.cend();

		for (; it != itEnd; ++it)
		{
			targetFolder = (*it)->archive->getName();
			if (Ogre::String::npos != targetFolder.find(resourceGroupName))
			{
				break;
			}
		}
	}
	catch (const Ogre::FileNotFoundException&)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ImageData] ERROR: Could not open file dialog because the resource name: '" + resourceGroupName + "' does not exist!");
		return false;
	}
	catch (...)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ImageData] ERROR: Could not open file dialog because the resource name: '" + resourceGroupName + "' does not exist!");
		return false;
	}

	ENQUEUE_RENDER_COMMAND_MULTI("PropertiesPanelDynamic::showFileOpenDialog", _3(targetFolder, action, fileMask),
	{
		// Set the target folder specified in scene resource group
		this->openSaveFileDialog->setCurrentFolder(targetFolder);
		// this->openSaveFileDialog->setRecentFolders(RecentFilesManager::getInstance().getRecentFolders());

		this->openSaveFileDialog->setDialogInfo(MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{OpenFile}"),
			MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{Open}"), MyGUI::LanguageManager::getInstancePtr()->replaceTags("#{UpFolder}"), false);

		this->openSaveFileDialog->setMode(action);
		this->openSaveFileDialog->setFileMask(fileMask);
		this->openSaveFileDialog->setFileName("");
		this->openSaveFileDialog->doModal();
		MyGUI::InputManager::getInstancePtr()->setMouseFocusWidget(this->openSaveFileDialog->getMainWidget());
		MyGUI::InputManager::getInstancePtr()->setKeyFocusWidget(this->openSaveFileDialog->getMainWidget());

		// If user is in dialog prevent camera movement (asdf)
		NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setMoveCameraWeight(0.0f);
		NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setRotateCameraWeight(0.0f);
	});

	return true;
}

//////////////////////////////////////////////////////////////////////////////////

PropertiesPanelGameObject::PropertiesPanelGameObject(const std::vector<NOWA::GameObject*>& gameObjects, const Ogre::String& name)
	: PropertiesPanelDynamic(gameObjects, name),
	listComponentsButton(nullptr)
{

}

PropertiesPanelGameObject::~PropertiesPanelGameObject()
{

}

void PropertiesPanelGameObject::initialise()
{
	PropertiesPanelDynamic::initialise();

	mPanelCell->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());

	// Add component button to list available components
	this->listComponentsButton = mPanelCell->getMainWidget()->createWidget<MyGUI::Button>("Button", MyGUI::IntCoord(10, 5, 22, 22), MyGUI::Align::Left | MyGUI::Align::Top, "listComponentsButton");
	this->listComponentsButton->setCaption("C");
	this->listComponentsButton->setTextColour(MyGUIHelper::getInstance()->getImportantTextColour());
	this->listComponentsButton->eventMouseButtonClick += MyGUI::newDelegate(this, &PropertiesPanelGameObject::notifyMouseAddComponentClick);
	this->listComponentsButton->eventMouseSetFocus += MyGUI::newDelegate(static_cast<PropertiesPanelDynamic*>(this), &PropertiesPanelDynamic::setFocus);
	this->listComponentsButton->setUserString("Description", "Show components");
	this->itemsEdit.push_back(this->listComponentsButton);

	//this->maxNumComponentsForPage = 30;
	//// + 1 because of the fraction, e.g. 77, = 2 + 1 pages (+ 1) for the remaining 11 components
	//unsigned short maxPages = NOWA::GameObjectFactory::getInstance()->getComponentFactory()->getRegisteredNames().size() / maxNumComponentsForPage + 1;
	//this->componentsPopupMenus.resize(maxPages);

	//// The other ones are created on the fly, because they need to be referenced by an already created menu item
	//this->componentsPopupMenus[0] = mPanelCell->getMainWidget()->createWidget<MyGUI::PopupMenu>(MyGUI::WidgetStyle::Popup, "PopupMenu",
	//	/*this->listComponentsButton->getAbsoluteCoord() +*/ MyGUI::IntCoord(15, 38, 0, 0), MyGUI::Align::Default, "Popup", "componentsPopupMenu" + Ogre::StringConverter::toString(0));
	//this->componentsPopupMenus[0]->setRealPosition(0.52f, 0.1f);
	//this->componentsPopupMenus[0]->hideMenu();
}

void PropertiesPanelGameObject::notifyMouseAddComponentClick(MyGUI::Widget* sender)
{
	if (this->listComponentsButton == sender)
	{
		// Sent when a component panel with all components should be shown
		boost::shared_ptr<EventDataShowComponentsPanel> eventDataShowComponentsPanel(new EventDataShowComponentsPanel());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataShowComponentsPanel);
	}
}

void PropertiesPanelGameObject::addComponent(MyGUI::MenuItem* menuItem)
{
	Ogre::String componentName = menuItem->getCaption();

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

	this->editorManager->addComponent(gameObjectIds, componentName);

	// Sent when a property has changed, so that the properties panel can be refreshed with new values
	boost::shared_ptr<EventDataRefreshPropertiesPanel> eventDataRefreshPropertiesPanel(new EventDataRefreshPropertiesPanel());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshPropertiesPanel);
}

void PropertiesPanelGameObject::setNewAttributeValue(MyGUI::EditBox* sender, NOWA::Variant* attribute)
{
	// Ogre::LogManager::getSingletonPtr()->logMessage("notifyEditSelectAccept: " + sender->getOnlyText());

	switch (attribute->getType())
	{
		case NOWA::Variant::VAR_INT:
		{
			attribute->setValue(Ogre::StringConverter::parseInt(sender->getOnlyText()));

			sender->setOnlyText(attribute->getString());
			break;
		}
		case NOWA::Variant::VAR_UINT:
		{
			attribute->setValue(Ogre::StringConverter::parseUnsignedInt(sender->getOnlyText()));
			sender->setOnlyText(attribute->getString());
			break;
		}
		case NOWA::Variant::VAR_ULONG:
		{
			attribute->setValue(Ogre::StringConverter::parseUnsignedLong(sender->getOnlyText()));
			sender->setOnlyText(attribute->getString());
			break;
		}
		case NOWA::Variant::VAR_REAL:
		{
			attribute->setValue(Ogre::StringConverter::parseReal(sender->getOnlyText()));
			sender->setOnlyText(attribute->getString());
			break;
		}
		case NOWA::Variant::VAR_VEC2:
		{
			attribute->setValue(Ogre::StringConverter::parseVector2(sender->getOnlyText()));
			sender->setOnlyText(attribute->getString());
			break;
		}
		case NOWA::Variant::VAR_VEC3:
		{
			attribute->setValue(Ogre::StringConverter::parseVector3(sender->getOnlyText()));
			sender->setOnlyText(attribute->getString());
			break;
		}
		case NOWA::Variant::VAR_VEC4:
		{
			attribute->setValue(Ogre::StringConverter::parseVector4(sender->getOnlyText()));
			sender->setOnlyText(attribute->getString());
			break;
		}
		case NOWA::Variant::VAR_STRING:
		{
			attribute->setValue(sender->getOnlyText());
			sender->setOnlyText(attribute->getString());
			break;
		}
	}

	MyGUIHelper::getInstance()->showAcceptedImage(Ogre::Vector2(sender->getAbsolutePosition().left, sender->getAbsolutePosition().top), Ogre::Vector2(10.0f, 10.0f), 1.0f);
}

void PropertiesPanelGameObject::notifyEditSelectAccept(MyGUI::EditBox* sender)
{
	// Let the camera move again
	NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setMoveCameraWeight(1.0f);
	NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setRotateCameraWeight(1.0f);

	// Send the text box change to the game object and internally actualize the data
	NOWA::Variant** attribute = sender->getUserData<NOWA::Variant*>();

	// Only proceed if a value changed
	Ogre::String editCaption = sender->getOnlyText();

	Ogre::String attributeString = (*attribute)->getString();

	if (true == (*attribute)->hasUserDataKey(NOWA::GameObject::AttrActionForceSet()))
	{
		// Do nothing but jump over the next condition, so that the value will be reset, even its the same value
	}
	else
	{
		// Only proceed if a value changed
		if (attributeString == editCaption)
		{
			return;
		}
	}

	// If its a list, check the selected value if it has changed
	if ((*attribute)->getList().size() > 0)
	{
		if ((*attribute)->getListSelectedValue() == editCaption)
		{
			return;
		}
	}

	// Snapshot the old attribute name
	this->editorManager->snapshotOldGameObjectAttribute(this->gameObjects, (*attribute)->getName());

	this->setNewAttributeValue(sender, *attribute);

	// Special treatment for transform because, if the game object has a physics component, those component shall handle the transform
	if (NOWA::GameObject::AttrPosition() == (*attribute)->getName())
	{
		if (1 == this->gameObjects.size())
		{
			auto& physicsComponent = NOWA::makeStrongPtr(this->gameObject->getComponent<NOWA::PhysicsComponent>());
			if (nullptr != physicsComponent)
			{
				physicsComponent->setPosition((*attribute)->getVector3());
			}
			else
			{
				this->gameObject->actualizeValue(*attribute);
			}
		}
		else
		{
			// If several game objects are selected add the value the user entered relative to the current values
			for (size_t i = 0; i < this->gameObjects.size(); i++)
			{
				auto& physicsComponent = NOWA::makeStrongPtr(this->gameObjects[i]->getComponent<NOWA::PhysicsComponent>());
				if (nullptr != physicsComponent)
				{
					physicsComponent->translate((*attribute)->getVector3());
				}
				else
				{
					this->gameObjects[i]->actualizeValue(*attribute);
				}
			}
		}
	}
	else if (NOWA::GameObject::AttrScale() == (*attribute)->getName())
	{
		if (1 == this->gameObjects.size())
		{
			auto& physicsComponent = NOWA::makeStrongPtr(gameObject->getComponent<NOWA::PhysicsComponent>());
			if (nullptr != physicsComponent)
			{
				physicsComponent->setScale((*attribute)->getVector3());
			}
			else
			{
				this->gameObject->actualizeValue(*attribute);
			}
		}
		else
		{
			for (size_t i = 0; i < this->gameObjects.size(); i++)
			{
				auto& physicsComponent = NOWA::makeStrongPtr(this->gameObjects[i]->getComponent<NOWA::PhysicsComponent>());
				if (nullptr != physicsComponent)
				{
					physicsComponent->setScale((*attribute)->getVector3());
				}
				else
				{
					this->gameObjects[i]->actualizeValue(*attribute);
				}
			}
		}
	}
	else if (NOWA::GameObject::AttrOrientation() == (*attribute)->getName())
	{
		if (1 == this->gameObjects.size())
		{
			auto& physicsComponent = NOWA::makeStrongPtr(gameObject->getComponent<NOWA::PhysicsComponent>());
			if (nullptr != physicsComponent)
			{
				// Transform to form degreeX, degreeY, degreeZ
				physicsComponent->setOrientation(NOWA::MathHelper::getInstance()->degreesToQuat((*attribute)->getVector3()));
			}
			else
			{
				this->gameObject->actualizeValue(*attribute);
			}
		}
		else
		{
			// If several game objects are selected add the value the user entered relative to the current values
			for (size_t i = 0; i < this->gameObjects.size(); i++)
			{
				auto& physicsComponent = NOWA::makeStrongPtr(this->gameObjects[i]->getComponent<NOWA::PhysicsComponent>());
				if (nullptr != physicsComponent)
				{
					physicsComponent->setOrientation(NOWA::MathHelper::getInstance()->degreesToQuat((*attribute)->getVector3()));
				}
				else
				{
					this->gameObjects[i]->actualizeValue(*attribute);
				}
				this->gameObjects[i]->actualizeValue(*attribute);
			}
		}
	}
	else if (NOWA::GameObject::AttrCategory() == (*attribute)->getName())
	{
		// When typed something in the new category text box, set this text as selected

		for (size_t i = 0; i < this->gameObjects.size(); i++)
		{
			if (false == sender->getOnlyText().empty())
			{
				auto currentAttribute = this->gameObjects[i]->getAttribute((*attribute)->getName());
				// Store the old category, that was selected, so that when changing internally the category, the old one can be removed, if no game object does use the category
				currentAttribute->setListSelectedOldValue(currentAttribute->getListSelectedValue());
				auto& list = currentAttribute->getList();
				list.emplace_back(sender->getOnlyText());
				currentAttribute->setValue(list);
				currentAttribute->setListSelectedValue(sender->getOnlyText());
				this->gameObjects[i]->actualizeValue(*attribute);
			}
		}

		ENQUEUE_RENDER_COMMAND_MULTI("PropertiesPanelGameObject::notifyEditSelectAccept1", _1(sender),
		{
			sender->setCaption("");
		});
		// Regenerate categories
		boost::shared_ptr<NOWA::EventDataGenerateCategories> eventDataGenerateCategories(new NOWA::EventDataGenerateCategories());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataGenerateCategories);

		// Sent when a name has changed, so that the resources panel can be refreshed with new values
		boost::shared_ptr<EventDataRefreshResourcesPanel> eventDataRefreshResourcesPanel(new EventDataRefreshResourcesPanel());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshResourcesPanel);
	}
	else if (NOWA::GameObject::AttrRenderCategory() == (*attribute)->getName())
	{
		// When typed something in the new render category text box, set this text as selected

		for (size_t i = 0; i < this->gameObjects.size(); i++)
		{
			if (false == sender->getOnlyText().empty())
			{
				auto currentAttribute = this->gameObjects[i]->getAttribute((*attribute)->getName());
				// Store the old render category, that was selected, so that when changing internally the render category, the old one can be removed, if no game object does use the category
				currentAttribute->setListSelectedOldValue(currentAttribute->getListSelectedValue());
				auto& list = currentAttribute->getList();
				list.emplace_back(sender->getOnlyText());
				currentAttribute->setValue(list);
				currentAttribute->setListSelectedValue(sender->getOnlyText());
				this->gameObjects[i]->actualizeValue(*attribute);
			}
		}

		ENQUEUE_RENDER_COMMAND_MULTI("PropertiesPanelGameObject::notifyEditSelectAccept2", _1(sender),
		{
			sender->setCaption("");
		});

		// Sent when a name has changed, so that the resources panel can be refreshed with new values
		boost::shared_ptr<EventDataRefreshResourcesPanel> eventDataRefreshResourcesPanel(new EventDataRefreshResourcesPanel());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshResourcesPanel);
	}
	else if (NOWA::GameObject::AttrName() == (*attribute)->getName())
	{
		// Validation of name is required, to remain unique
		Ogre::String gameObjectName = (*attribute)->getString();
		NOWA::AppStateManager::getSingletonPtr()->getGameObjectController()->getValidatedGameObjectName(gameObjectName);
		ENQUEUE_RENDER_COMMAND_MULTI("PropertiesPanelGameObject::notifyEditSelectAccept3", _2(sender, gameObjectName),
		{
			sender->setOnlyText(gameObjectName);
		});
		(*attribute)->setValue(gameObjectName);
		this->gameObject->actualizeValue(*attribute);

		// Sent when a name has changed, so that the resources panel can be refreshed with new values
		boost::shared_ptr<EventDataRefreshResourcesPanel> eventDataRefreshResourcesPanel(new EventDataRefreshResourcesPanel());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshResourcesPanel);
	}
	else
	{
		for (size_t i = 0; i < this->gameObjects.size(); i++)
		{
			this->gameObjects[i]->actualizeValue(*attribute);
		}
	}

	this->editorManager->setGizmoToGameObjectsCenter();
	// Snapshot the new attribute
	this->editorManager->snapshotNewGameObjectAttribute(*attribute);

	if (true == (*attribute)->hasUserDataKey(NOWA::GameObject::AttrActionNeedRefresh()))
	{
		boost::shared_ptr<EventDataRefreshPropertiesPanel> eventDataRefreshPropertiesPanel(new EventDataRefreshPropertiesPanel());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshPropertiesPanel);
	}

	// Sent when a property has changed, so that the properties panel can be refreshed with new values
	// boost::shared_ptr<EventDataRefreshPropertiesPanel> eventDataRefreshPropertiesPanel(new EventDataRefreshPropertiesPanel());
	// NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshPropertiesPanel);
}

void PropertiesPanelGameObject::buttonHit(MyGUI::Widget* sender)
{
	MyGUI::Button* button = sender->castType<MyGUI::Button>();
	if (nullptr != button)
	{
		ENQUEUE_RENDER_COMMAND_MULTI("PropertiesPanelGameObject::buttonHit", _1(button),
		{
			NOWA::Variant** attribute = button->getUserData<NOWA::Variant*>(false);
			// ColorDialog handling
			if (nullptr != attribute && true == (*attribute)->hasUserDataKey(NOWA::GameObject::AttrActionColorDialog()))
			{
				// Button hit results in showing properties again, which should not be
				PropertiesPanel::setShowPropertiesFlag(false);
				// Must be copied, because the properties are re-created and so the original attribute is gone
				NOWA::Variant* variantCopy = (*attribute)->clone();
				ColourPanelManager::getInstance()->getColourPanel()->getWidget()->setUserData(MyGUI::Any(variantCopy));
				if (NOWA::Variant::VAR_VEC3 == variantCopy->getType())
				{
					ColourPanelManager::getInstance()->getColourPanel()->setColour(MyGUI::Colour(variantCopy->getVector3().x, variantCopy->getVector3().y, variantCopy->getVector3().z, 1.0f));
				}
				else if (NOWA::Variant::VAR_VEC4 == variantCopy->getType())
				{
					ColourPanelManager::getInstance()->getColourPanel()->setColour(MyGUI::Colour(variantCopy->getVector4().x, variantCopy->getVector4().y, variantCopy->getVector4().z, variantCopy->getVector4().w));
				}
				ColourPanelManager::getInstance()->getColourPanel()->setVisible(true);
				// MyGUI::InputManager::getInstancePtr()->removeWidgetModal(parent->getMainWidget());
				MyGUI::InputManager::getInstancePtr()->addWidgetModal(ColourPanelManager::getInstance()->getColourPanel()->getWidget());
				ColourPanelManager::getInstance()->getColourPanel()->eventColourAccept = MyGUI::newDelegate(this, &PropertiesPanelGameObject::notifyColourAccept);
				ColourPanelManager::getInstance()->getColourPanel()->eventColourCancel = MyGUI::newDelegate((PropertiesPanelDynamic*)this, &PropertiesPanelGameObject::notifyColourCancel);
			}
			// FileOpenDialog handling
			else if (nullptr != attribute && true == (*attribute)->hasUserDataKey(NOWA::GameObject::AttrActionFileOpenDialog()))
			{
				// Button hit results in showing properties again, which should not be
				PropertiesPanel::setShowPropertiesFlag(false);
				// Must be copied, because the properties are re-created and so the original attribute is gone
				NOWA::Variant* variantCopy = (*attribute)->clone();

				Ogre::String resourceGroupName = variantCopy->getUserDataValue(NOWA::GameObject::AttrActionFileOpenDialog());
				this->openSaveFileDialog->getMainWidget()->setUserData(MyGUI::Any(variantCopy));

				MyGUI::InputManager::getInstancePtr()->setMouseFocusWidget(this->openSaveFileDialog->getMainWidget());
				MyGUI::InputManager::getInstancePtr()->setKeyFocusWidget(this->openSaveFileDialog->getMainWidget());

				this->showFileOpenDialog("FileOpen", "*.*", resourceGroupName);
			}
			else
			{
				// Invert the state
				button->setStateCheck(!button->getStateCheck());
			}

			if (nullptr == attribute)
			{
				return;
			}

			// Snapshot the old attribute name
			this->editorManager->snapshotOldGameObjectAttribute(this->gameObjects, (*attribute)->getName());

			for (size_t i = 0; i < this->gameObjects.size(); i++)
			{
				auto currentAttribute = this->gameObjects[i]->getAttribute((*attribute)->getName());
				currentAttribute->setValue(button->getStateCheck());
				this->gameObjects[i]->actualizeValue(*attribute);
			}

			// Snapshot the new attribute
			this->editorManager->snapshotNewGameObjectAttribute(*attribute);

			if (nullptr != attribute && true == (*attribute)->hasUserDataKey(NOWA::GameObject::AttrActionNeedRefresh()))
			{
				// Sent when a property has changed, so that the properties panel can be refreshed with new values
				boost::shared_ptr<EventDataRefreshPropertiesPanel> eventDataRefreshPropertiesPanel(new EventDataRefreshPropertiesPanel());
				NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshPropertiesPanel);
			}
		});
	}
}

void PropertiesPanelGameObject::notifyComboChangedPosition(MyGUI::ComboBox* sender, size_t index)
{
	// Send the combo box change to the game object and internally actualize the data
	NOWA::Variant** attribute = sender->getUserData<NOWA::Variant*>();

	// Snapshot the old attribute name
	this->editorManager->snapshotOldGameObjectAttribute(this->gameObjects, (*attribute)->getName());

	for (size_t i = 0; i < this->gameObjects.size(); i++)
	{
		auto currentAttribute = this->gameObjects[i]->getAttribute((*attribute)->getName());
		currentAttribute->setListSelectedValue(sender->getItem(index));

		this->gameObjects[i]->actualizeValue(*attribute);
	}

	// Snapshot the new attribute
	this->editorManager->snapshotNewGameObjectAttribute(*attribute);

	// Sent when a property has changed, so that the properties panel can be refreshed with new values
	boost::shared_ptr<EventDataRefreshPropertiesPanel> eventDataRefreshPropertiesPanel(new EventDataRefreshPropertiesPanel());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshPropertiesPanel);
}

void PropertiesPanelGameObject::notifyColourAccept(MyGUI::ColourPanel* sender)
{
	ENQUEUE_RENDER_COMMAND_MULTI("PropertiesPanelGameObject::notifyColourAccept", _1(sender),
	{
		Ogre::Vector3 colour(sender->getColour().red, sender->getColour().green, sender->getColour().blue);

		NOWA::Variant** copiedAttribute = sender->getWidget()->getUserData<NOWA::Variant*>();
		(*copiedAttribute)->setValue(colour);
		MyGUI::InputManager::getInstancePtr()->removeWidgetModal(ColourPanelManager::getInstance()->getColourPanel()->getWidget());
		ColourPanelManager::getInstance()->getColourPanel()->setVisible(false);

		// Snapshot the old attribute name
		this->editorManager->snapshotOldGameObjectAttribute(this->gameObjects, (*copiedAttribute)->getName());

		for (size_t i = 0; i < this->gameObjects.size(); i++)
		{
			auto currentAttribute = this->gameObjects[i]->getAttribute((*copiedAttribute)->getName());
			this->gameObjects[i]->actualizeValue(*copiedAttribute);
		}

		// Snapshot the new attribute
		this->editorManager->snapshotNewGameObjectAttribute(*copiedAttribute);

		// Sent when a property has changed, so that the properties panel can be refreshed with new values
		boost::shared_ptr<EventDataRefreshPropertiesPanel> eventDataRefreshPropertiesPanel(new EventDataRefreshPropertiesPanel());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->threadSafeQueueEvent(eventDataRefreshPropertiesPanel);
	});
}

void PropertiesPanelGameObject::notifyEndDialog(tools::Dialog* sender, bool result)
{

}

void PropertiesPanelGameObject::notifyScrollChangePosition(MyGUI::ScrollBar* sender, size_t position)
{
	MyGUI::ScrollBar* scrollBar = sender->castType<MyGUI::ScrollBar>(false);
	if (nullptr != scrollBar)
	{

		// TODO: implement when necessary
	}
}

void PropertiesPanelGameObject::notifySliderMouseRelease(MyGUI::Widget* sender, int x, int y, MyGUI::MouseButton button)
{
	// TODO: implement when necessary
}

void PropertiesPanelGameObject::notifySetItemBoxData(MyGUI::ItemBox* sender, const Ogre::String& resourceName)
{
	// Send the combo box change to the game object and internally actualize the data
	NOWA::Variant** attribute = sender->getUserData<NOWA::Variant*>();

	// Snapshot the old attribute name
	this->editorManager->snapshotOldGameObjectAttribute(this->gameObjects, (*attribute)->getName());

	for (size_t i = 0; i < this->gameObjects.size(); i++)
	{
		auto currentAttribute = this->gameObjects[i]->getAttribute((*attribute)->getName());

		currentAttribute->setListSelectedValue(resourceName);

		this->gameObjects[i]->actualizeValue(*attribute);
	}

	// Snapshot the new attribute
	this->editorManager->snapshotNewGameObjectAttribute(*attribute);

	// Sent when a property has changed, so that the properties panel can be refreshed with new values
	boost::shared_ptr<EventDataRefreshPropertiesPanel> eventDataRefreshPropertiesPanel(new EventDataRefreshPropertiesPanel());
	NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshPropertiesPanel);
}

///////////////////////////////////////////////////////////////////////////////

PropertiesPanelComponent::PropertiesPanelComponent(const std::vector<NOWA::GameObject*>& gameObjects,
	const std::vector<NOWA::GameObjectComponent*>& gameObjectComponents, const Ogre::String& name)
	: PropertiesPanelDynamic(gameObjects, name),
	gameObjectComponents(gameObjectComponents),
	appendComponentButton(nullptr),
	deleteComponentButton(nullptr),
	debugDataComponentButton(nullptr),
	moveUpComponentButton(nullptr),
	moveDownComponentButton(nullptr)
{

}

PropertiesPanelComponent::~PropertiesPanelComponent()
{

}

void PropertiesPanelComponent::initialise()
{
	PropertiesPanelDynamic::initialise();
	mPanelCell->setTextColour(MyGUIHelper::getInstance()->getDefaultTextColour());

	this->appendComponentButton = mPanelCell->getMainWidget()->createWidget<MyGUI::Button>("Button", MyGUI::IntCoord(4, 5, 22, 22), MyGUI::Align::Left | MyGUI::Align::Top, "addComponentsButton");
	this->appendComponentButton->setCaption("C");
	this->appendComponentButton->setTextColour(MyGUIHelper::getInstance()->getImportantTextColour());
	this->appendComponentButton->eventMouseButtonClick += MyGUI::newDelegate(this, &PropertiesPanelComponent::buttonHit);
	this->appendComponentButton->eventMouseSetFocus += MyGUI::newDelegate(static_cast<PropertiesPanelDynamic*>(this), &PropertiesPanelDynamic::setFocus);
	this->appendComponentButton->setUserString("Description", "Append component");
	this->itemsEdit.push_back(this->appendComponentButton);

	this->deleteComponentButton = mPanelCell->getMainWidget()->createWidget<MyGUI::Button>("Button", MyGUI::IntCoord(29, 5, 22, 22), MyGUI::Align::Left | MyGUI::Align::Top, "deleteComponentsButton");
	this->deleteComponentButton->setCaption("X");
	this->deleteComponentButton->setTextColour(MyGUIHelper::getInstance()->getImportantTextColour());
	this->deleteComponentButton->eventMouseButtonClick += MyGUI::newDelegate(this, &PropertiesPanelComponent::buttonHit);
	this->deleteComponentButton->eventMouseSetFocus += MyGUI::newDelegate(static_cast<PropertiesPanelDynamic*>(this), &PropertiesPanelDynamic::setFocus);
	this->deleteComponentButton->setUserString("Description", "Delete component");
	this->itemsEdit.push_back(this->deleteComponentButton);

	this->debugDataComponentButton = mPanelCell->getMainWidget()->createWidget<MyGUI::Button>("Button", MyGUI::IntCoord(54, 5, 22, 22), MyGUI::Align::Left | MyGUI::Align::Top, "debugComponentsButton");
	this->debugDataComponentButton->setCaption("D");
	this->debugDataComponentButton->setTextColour(MyGUIHelper::getInstance()->getImportantTextColour());
	this->debugDataComponentButton->eventMouseButtonClick += MyGUI::newDelegate(this, &PropertiesPanelComponent::buttonHit);
	this->debugDataComponentButton->eventMouseSetFocus += MyGUI::newDelegate(static_cast<PropertiesPanelDynamic*>(this), &PropertiesPanelDynamic::setFocus);
	this->debugDataComponentButton->setUserString("Description", "Show debug data");
	if (false == this->gameObjectComponents.empty())
		this->debugDataComponentButton->setStateCheck(this->gameObjectComponents[0]->getShowDebugData());
	this->itemsEdit.push_back(this->debugDataComponentButton);

	this->moveUpComponentButton = mPanelCell->getMainWidget()->createWidget<MyGUI::Button>("ButtonUpSkin", MyGUI::IntCoord(79, 7, 16, 16), MyGUI::Align::Left | MyGUI::Align::Top, "upComponentsButton");
	this->moveUpComponentButton->setTextColour(MyGUIHelper::getInstance()->getImportantTextColour());
	this->moveUpComponentButton->eventMouseButtonClick += MyGUI::newDelegate(this, &PropertiesPanelComponent::buttonHit);
	this->moveUpComponentButton->eventMouseSetFocus += MyGUI::newDelegate(static_cast<PropertiesPanelDynamic*>(this), &PropertiesPanelDynamic::setFocus);
	this->moveUpComponentButton->setUserString("Description", "Move component up");
	this->itemsEdit.push_back(this->moveUpComponentButton);

	this->moveDownComponentButton = mPanelCell->getMainWidget()->createWidget<MyGUI::Button>("ButtonDownSkin", MyGUI::IntCoord(100, 7, 16, 16), MyGUI::Align::Left | MyGUI::Align::Top, "downComponentsButton");
	this->moveDownComponentButton->setTextColour(MyGUIHelper::getInstance()->getImportantTextColour());
	this->moveDownComponentButton->eventMouseButtonClick += MyGUI::newDelegate(this, &PropertiesPanelComponent::buttonHit);
	this->moveDownComponentButton->eventMouseSetFocus += MyGUI::newDelegate(static_cast<PropertiesPanelDynamic*>(this), &PropertiesPanelDynamic::setFocus);
	this->moveDownComponentButton->setUserString("Description", "Move component down");
	this->itemsEdit.push_back(this->moveDownComponentButton);
}

void PropertiesPanelComponent::setNewAttributeValue(MyGUI::EditBox* sender, NOWA::Variant* attribute)
{
	// Ogre::LogManager::getSingletonPtr()->logMessage("notifyEditSelectAccept: " + sender->getOnlyText());

	switch (attribute->getType())
	{
		case NOWA::Variant::VAR_INT:
		{
			attribute->setValue(Ogre::StringConverter::parseInt(sender->getOnlyText()));
			sender->setOnlyText(attribute->getString());
			break;
		}
		case NOWA::Variant::VAR_UINT:
		{
			attribute->setValue(Ogre::StringConverter::parseUnsignedInt(sender->getOnlyText()));
			sender->setOnlyText(attribute->getString());
			break;
		}
		case NOWA::Variant::VAR_ULONG:
		{
			attribute->setValue(Ogre::StringConverter::parseUnsignedLong(sender->getOnlyText()));
			sender->setOnlyText(attribute->getString());
			break;
		}
		case NOWA::Variant::VAR_REAL:
		{
			attribute->setValue(Ogre::StringConverter::parseReal(sender->getOnlyText()));
			// Only set formatted number, if not an e number, because else it will not be accepted and set to 0
			size_t found = attribute->getString().find("e");
			if (Ogre::String::npos == found)
			{
				sender->setOnlyText(attribute->getString());
			}
			break;
		}
		case NOWA::Variant::VAR_VEC2:
		{
			attribute->setValue(Ogre::StringConverter::parseVector2(sender->getOnlyText()));
			// Only set formatted number, if not an e number, because else it will not be accepted and set to 0
			size_t found = attribute->getString().find("e");
			if (Ogre::String::npos == found)
			{
				sender->setOnlyText(attribute->getString());
			}
			break;
		}
		case NOWA::Variant::VAR_VEC3:
		{
			attribute->setValue(Ogre::StringConverter::parseVector3(sender->getOnlyText()));
			// Only set formatted number, if not an e number, because else it will not be accepted and set to 0
			size_t found = attribute->getString().find("e");
			if (Ogre::String::npos == found)
			{
				sender->setOnlyText(attribute->getString());
			}
			break;
		}
		case NOWA::Variant::VAR_VEC4:
		{
			attribute->setValue(Ogre::StringConverter::parseVector4(sender->getOnlyText()));
			// Only set formatted number, if not an e number, because else it will not be accepted and set to 0
			size_t found = attribute->getString().find("e");
			if (Ogre::String::npos == found)
			{
				sender->setOnlyText(attribute->getString());
			}
			break;
		}
		default:
		{
			// Remove all hashes (but not if its a color), because its an internal code character for MyGUI
			Ogre::String text = sender->getOnlyText();
			// text.erase(std::remove(text.begin(), text.end(), '#'), text.end());
			text = removeHashesExceptForColor(text);
			attribute->setValue(text);
			sender->setOnlyText(attribute->getString());
			break;
		}
	}

	MyGUIHelper::getInstance()->showAcceptedImage(Ogre::Vector2(sender->getAbsolutePosition().left, sender->getAbsolutePosition().top), Ogre::Vector2(10.0f, 10.0f), 1.0f);
}

void PropertiesPanelComponent::notifyEditSelectAccept(MyGUI::EditBox* sender)
{
	ENQUEUE_RENDER_COMMAND_MULTI("PropertiesPanelComponent::notifyEditSelectAccept", _1(sender),
	{
		// Let the camera move again
		NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setMoveCameraWeight(1.0f);
		NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setRotateCameraWeight(1.0f);
		// Send the text box change to the game object and internally actualize the data
		NOWA::Variant * *attribute = sender->getUserData<NOWA::Variant*>();
		NOWA::Variant * variantCopy = (*attribute)->clone();

		Ogre::String editCaption = sender->getOnlyText();
		// Range adaptation
		Ogre::Real editValue = Ogre::StringConverter::parseReal(editCaption);
		// Check if the edit is paired with a slider, to actualize the slider value
		Ogre::String sliderName = sender->getUserString("slider" + sender->getName());
		if (false == sliderName.empty())
		{
			if (editValue < (*attribute)->getConstraints().first)
			{
				editValue = (*attribute)->getConstraints().first;
				editCaption = Ogre::StringConverter::toString(editValue);
				sender->setOnlyText(editCaption);
			}
			else if (editValue > (*attribute)->getConstraints().second)
			{
				editValue = (*attribute)->getConstraints().second;
				editCaption = Ogre::StringConverter::toString(editValue);
				sender->setOnlyText(editCaption);
			}

			MyGUI::ScrollBar* slider = MyGUI::Gui::getInstancePtr()->findWidget<MyGUI::ScrollBar>(sliderName, false);
			if (nullptr != slider)
			{
				Ogre::Real lowBorder = (*attribute)->getConstraints().first;
				Ogre::Real highBorder = (*attribute)->getConstraints().second;

				// Checks if int slider or real slider
				if ((*attribute)->getType() == NOWA::Variant::VAR_INT
					|| (*attribute)->getType() == NOWA::Variant::VAR_UINT
					|| (*attribute)->getType() == NOWA::Variant::VAR_ULONG)
				{
					int value;
					std::istringstream iss(editCaption);
					if (iss >> value)
					{
						NOWA::Variant** attribute = sender->getUserData<NOWA::Variant*>();

						if (value < lowBorder)
						{
							value = lowBorder;
						}
						if (value > highBorder)
						{
							value = highBorder;
						}
						size_t newPosition = static_cast<size_t>(value - lowBorder);
						slider->setScrollPosition(newPosition);
					}
				}
				else
				{
					Ogre::Real value;
					std::istringstream iss(editCaption);
					if (iss >> value)
					{
						if (value < lowBorder)
						{
							value = lowBorder;
						}
						if (value > highBorder)
						{
							value = highBorder;
						}
						size_t newPosition = static_cast<size_t>((value - lowBorder) / (highBorder - lowBorder) * 1000.0f);
						slider->setScrollPosition(newPosition);
					}
				}
			}
		}

		if (true == (*attribute)->hasUserDataKey(NOWA::GameObject::AttrActionForceSet()))
		{
			// Do nothing but jump over the next condition, so that the value will be reset, even its the same value
		}
		else
		{
			// Only proceed if a value changed
			if ((*attribute)->getString() == editCaption)
			{
				return;
			}
		}

		// If its a list, check the selected value if it has changed
		if ((*attribute)->getList().size() > 0)
		{
			if ((*attribute)->getListSelectedValue() == editCaption)
			{
				return;
			}
		}

		// Snapshot the old attribute name
		this->editorManager->snapshotOldGameObjectComponentAttribute(this->gameObjects, this->gameObjectComponents, (*attribute)->getName());

		this->setNewAttributeValue(sender, variantCopy);

		for (size_t i = 0; i < this->gameObjectComponents.size(); i++)
		{
			this->gameObjectComponents[i]->actualizeValue(variantCopy);
		}

		delete variantCopy;

		this->editorManager->snapshotNewGameObjectComponentAttribute(*attribute);

		if (true == (*attribute)->hasUserDataKey(NOWA::GameObject::AttrActionNeedRefresh()))
		{
			boost::shared_ptr<EventDataRefreshPropertiesPanel> eventDataRefreshPropertiesPanel(new EventDataRefreshPropertiesPanel());
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->threadSafeQueueEvent(eventDataRefreshPropertiesPanel);
		}

		// Sent when a property has changed, so that the properties panel can be refreshed with new values
		// boost::shared_ptr<EventDataRefreshPropertiesPanel> eventDataRefreshPropertiesPanel(new EventDataRefreshPropertiesPanel());
		// NOWA::AppStateManager::getSingletonPtr()->getEventManager()->threadSafeQueueEvent(eventDataRefreshPropertiesPanel);
	});
}

void PropertiesPanelComponent::notifyScrollChangePosition(MyGUI::ScrollBar* sender, size_t position)
{
	MyGUI::ScrollBar* scrollBar = sender->castType<MyGUI::ScrollBar>(false);
	if (nullptr != scrollBar)
	{
		MyGUI::EditBox** editPair = sender->getUserData<MyGUI::EditBox*>(false);
		if (nullptr != editPair)
		{
			// Send the text box change to the game object and internally actualize the data
			NOWA::Variant** attribute = (*editPair)->getUserData<NOWA::Variant*>(false);
			Ogre::Real value = 0.0f;
			if (NOWA::Variant::VAR_REAL == (*attribute)->getType())
			{
				value = (*attribute)->getConstraints().first + static_cast<float>(position) * ((*attribute)->getConstraints().second - (*attribute)->getConstraints().first) / 1000.0f;
			}
			else
			{
				value = position + (*attribute)->getConstraints().first;
			}

			ENQUEUE_RENDER_COMMAND_MULTI("PropertiesPanelComponent::notifyScrollChangePosition", _2(editPair, value),
			{
				(*editPair)->setOnlyText(Ogre::StringConverter::toString(value));
			});

			if (true == (*attribute)->hasUserDataKey(NOWA::GameObject::AttrActionForceSet()))
			{
				// Do nothing but jump over the next condition, so that the value will be reset, even its the same value
			}
			else
			{
				// Only proceed if a value changed
				if (Ogre::Math::RealEqual((*attribute)->getReal(), value))
				{
					return;
				}
			}
#if 0
			// TODO: For undo Redo, is called to often! Just set those values on edit box! when final value is set via slider!
			if (false == (*attribute)->hasUserDataKey(NOWA::GameObject::AttrActionNoUndo()))
			{
				// Snapshot the old attribute name
				this->editorManager->snapshotOldGameObjectComponentAttribute(this->gameObjects, this->gameObjectComponents, (*attribute)->getName());
			}

			for (size_t i = 0; i < this->gameObjectComponents.size(); i++)
			{
				NOWA::Variant* currentAttribute = this->gameObjectComponents[i]->getAttribute((*attribute)->getName());
				currentAttribute->setValue(value);
			}

			for (size_t i = 0; i < this->gameObjectComponents.size(); i++)
			{
				this->gameObjectComponents[i]->actualizeValue(*attribute);
			}

			if (false == (*attribute)->hasUserDataKey(NOWA::GameObject::AttrActionNoUndo()))
			{
				this->editorManager->snapshotNewGameObjectComponentAttribute(*attribute);

				if (true == (*attribute)->hasUserDataKey(NOWA::GameObject::AttrActionNeedRefresh()))
				{
					boost::shared_ptr<EventDataRefreshPropertiesPanel> eventDataRefreshPropertiesPanel(new EventDataRefreshPropertiesPanel());
					NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshPropertiesPanel);
				}
			}
#endif
		}
	}
}

void PropertiesPanelComponent::notifySliderMouseRelease(MyGUI::Widget* sender, int x, int y, MyGUI::MouseButton button)
{
	// Is never called why??
	MyGUI::ScrollBar* scrollBar = sender->castType<MyGUI::ScrollBar>(false);
	if (nullptr != scrollBar)
	{
		MyGUI::EditBox** editPair = sender->getUserData<MyGUI::EditBox*>(false);
		if (nullptr != editPair)
		{
			// Send the text box change to the game object and internally actualize the data
			NOWA::Variant** attribute = (*editPair)->getUserData<NOWA::Variant*>(false);

			if (false == (*attribute)->hasUserDataKey(NOWA::GameObject::AttrActionNoUndo()))
			{
				// Snapshot the old attribute name
				this->editorManager->snapshotOldGameObjectComponentAttribute(this->gameObjects, this->gameObjectComponents, (*attribute)->getName());
			}

			for (size_t i = 0; i < this->gameObjectComponents.size(); i++)
			{
				NOWA::Variant* currentAttribute = this->gameObjectComponents[i]->getAttribute((*attribute)->getName());
				currentAttribute->setValue(Ogre::StringConverter::parseReal((*editPair)->getOnlyText()));
			}

			for (size_t i = 0; i < this->gameObjectComponents.size(); i++)
			{
				this->gameObjectComponents[i]->actualizeValue(*attribute);
			}

			if (false == (*attribute)->hasUserDataKey(NOWA::GameObject::AttrActionNoUndo()))
			{
				this->editorManager->snapshotNewGameObjectComponentAttribute(*attribute);

				if (true == (*attribute)->hasUserDataKey(NOWA::GameObject::AttrActionNeedRefresh()))
				{
					boost::shared_ptr<EventDataRefreshPropertiesPanel> eventDataRefreshPropertiesPanel(new EventDataRefreshPropertiesPanel());
					NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshPropertiesPanel);
				}
			}
		}
	}
}

void PropertiesPanelComponent::notifySetItemBoxData(MyGUI::ItemBox* sender, const Ogre::String& resourceName)
{
	this->showDescription(sender);

	// Send the combo box change to the game object and internally actualize the data
	NOWA::Variant** attribute = sender->getUserData<NOWA::Variant*>();

	if (false == (*attribute)->hasUserDataKey(NOWA::GameObject::AttrActionNoUndo()))
	{
		// Snapshot the old attribute name
		this->editorManager->snapshotOldGameObjectComponentAttribute(this->gameObjects, this->gameObjectComponents, (*attribute)->getName());
	}

	for (size_t i = 0; i < this->gameObjectComponents.size(); i++)
	{
		auto currentAttribute = this->gameObjectComponents[i]->getAttribute((*attribute)->getName());
		// Store also the old value in list
		(*attribute)->setListSelectedOldValue(currentAttribute->getListSelectedValue());
		currentAttribute->setListSelectedValue(resourceName);

		this->gameObjectComponents[i]->actualizeValue(*attribute);
	}

	if (false == (*attribute)->hasUserDataKey(NOWA::GameObject::AttrActionNoUndo()))
	{
		// Snapshot the new attribute
		this->editorManager->snapshotNewGameObjectComponentAttribute(*attribute);

		if (true == (*attribute)->hasUserDataKey(NOWA::GameObject::AttrActionNeedRefresh()))
		{
			boost::shared_ptr<EventDataRefreshPropertiesPanel> eventDataRefreshPropertiesPanel(new EventDataRefreshPropertiesPanel());
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshPropertiesPanel);
		}
	}
}

void PropertiesPanelComponent::buttonHit(MyGUI::Widget* sender)
{
	NOWA::GraphicsModule::RenderCommand renderCommand = [this, sender]()
	{
		this->showDescription(sender);

		MyGUI::Button * button = sender->castType<MyGUI::Button>();
		bool hasAttribute = false;
		if (nullptr != button)
		{
			NOWA::Variant** attribute = button->getUserData<NOWA::Variant*>(false);
			if (nullptr != attribute)
			{
				hasAttribute = true;
			}

			if (true == hasAttribute && true == (*attribute)->hasUserDataKey(NOWA::GameObject::AttrActionColorDialog()))
			{
				// Button hit results in showing properties again, which should not be
				PropertiesPanel::setShowPropertiesFlag(false);
				// Must be copied, because the properties are re-created and so the original attribute is gone
				NOWA::Variant* variantCopy = (*attribute)->clone();
				ColourPanelManager::getInstance()->getColourPanel()->getWidget()->setUserData(MyGUI::Any(variantCopy));
				if (NOWA::Variant::VAR_VEC3 == variantCopy->getType())
				{
					ColourPanelManager::getInstance()->getColourPanel()->setColour(MyGUI::Colour(variantCopy->getVector3().x, variantCopy->getVector3().y, variantCopy->getVector3().z, 1.0f));
				}
				else if (NOWA::Variant::VAR_VEC4 == variantCopy->getType())
				{
					ColourPanelManager::getInstance()->getColourPanel()->setColour(MyGUI::Colour(variantCopy->getVector4().x, variantCopy->getVector4().y, variantCopy->getVector4().z, variantCopy->getVector4().w));
				}
				ColourPanelManager::getInstance()->getColourPanel()->setVisible(true);
				MyGUI::InputManager::getInstancePtr()->addWidgetModal(ColourPanelManager::getInstance()->getColourPanel()->getWidget());
				ColourPanelManager::getInstance()->getColourPanel()->eventColourAccept = MyGUI::newDelegate(this, &PropertiesPanelComponent::notifyColourAccept);
				ColourPanelManager::getInstance()->getColourPanel()->eventColourCancel = MyGUI::newDelegate((PropertiesPanelDynamic*)this, &PropertiesPanelDynamic::notifyColourCancel);
			}
			// FileOpenDialog handling
			else if (true == hasAttribute && true == (*attribute)->hasUserDataKey(NOWA::GameObject::AttrActionFileOpenDialog()))
			{
				// Button hit results in showing properties again, which should not be
				PropertiesPanel::setShowPropertiesFlag(false);

				// Must be copied, because the properties are re-created and so the original attribute is gone
				NOWA::Variant* variantCopy = (*attribute)->clone();

				Ogre::String resourceGroupName = variantCopy->getUserDataValue(NOWA::GameObject::AttrActionFileOpenDialog());
				if (true == resourceGroupName.empty())
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[ImageData] ERROR: Could not open file dialog because the resource name is empty!");
					return;
				}

				this->openSaveFileDialog->getMainWidget()->setUserData(MyGUI::Any(variantCopy));
				MyGUI::InputManager::getInstancePtr()->setMouseFocusWidget(this->openSaveFileDialog->getMainWidget());
				MyGUI::InputManager::getInstancePtr()->setKeyFocusWidget(this->openSaveFileDialog->getMainWidget());
				// this->openSaveFileDialog->eventEndDialog = MyGUI::newDelegate(this, &PropertiesPanelComponent::notifyEndDialog);

				this->showFileOpenDialog("FileOpen", "*.*", resourceGroupName);
			}
			else if (nullptr != attribute && true == (*attribute)->hasUserDataKey(NOWA::GameObject::AttrActionLuaScript()))
			{
				Ogre::String absoluteLuaScriptFilePathName = (*attribute)->getUserDataValue(NOWA::GameObject::AttrActionLuaScript());
				bool success = NOWA::DeployResourceModule::getInstance()->openNOWALuaScriptEditor(absoluteLuaScriptFilePathName);
			}
			else if (true == hasAttribute && true == (*attribute)->hasUserDataKey(NOWA::GameObject::AttrActionGenerateLuaFunction()))
			{
				if (nullptr != this->gameObject->getLuaScript())
				{
					NOWA::AppStateManager::getSingletonPtr()->getLuaScriptModule()->generateLuaFunctionName(this->gameObject->getLuaScript()->getScriptName(), (*attribute)->getUserDataValue(NOWA::GameObject::AttrActionGenerateLuaFunction()), this->gameObject->getGlobal());
				}
			}
			else if (button == this->appendComponentButton)
			{
				// Deletes the component
				std::vector<unsigned long> gameObjectIds(this->gameObjects.size());

				for (size_t i = 0; i < this->gameObjects.size(); i++)
				{
					gameObjectIds[i] = this->gameObjects[i]->getId();
				}

				int index = this->gameObjects[0]->getIndexFromComponent(this->gameObjectComponents[0]);
				if (-1 != index)
				{
					// Sent when a component panel with all components should be shown
					boost::shared_ptr<EventDataShowComponentsPanel> eventDataShowComponentsPanel(new EventDataShowComponentsPanel(index));
					NOWA::AppStateManager::getSingletonPtr()->getEventManager()->threadSafeQueueEvent(eventDataShowComponentsPanel);
				}
			}
			else if (button == this->deleteComponentButton)
			{
				// Deletes the component
				std::vector<unsigned long> gameObjectIds(this->gameObjects.size());

				for (size_t i = 0; i < this->gameObjects.size(); i++)
				{
					// Prevent deleting a component of a kind main game object
					Ogre::String className = this->gameObjectComponents[0]->getClassName();

					if (NOWA::GameObjectController::MAIN_GAMEOBJECT_ID == this->gameObjects[i]->getId())
					{
						if (className == NOWA::NodeComponent::getStaticClassName())
						{
							return;
						}
					}
					else if (NOWA::GameObjectController::MAIN_LIGHT_ID == this->gameObjects[i]->getId())
					{
						if (className == NOWA::LightDirectionalComponent::getStaticClassName())
						{
							return;
						}
					}
					else if (NOWA::GameObjectController::MAIN_CAMERA_ID == this->gameObjects[i]->getId())
					{
						if (className == NOWA::CameraComponent::getStaticClassName())
						{
							return;
						}
					}

					gameObjectIds[i] = this->gameObjects[i]->getId();
				}

				int index = this->gameObjects[0]->getIndexFromComponent(this->gameObjectComponents[0]);
				if (-1 != index)
				{
					this->editorManager->deleteComponent(gameObjectIds, static_cast<unsigned int>(index));
				}
				// Sent when a property has changed, so that the properties panel can be refreshed with new values
				boost::shared_ptr<EventDataRefreshPropertiesPanel> eventDataRefreshPropertiesPanel(new EventDataRefreshPropertiesPanel());
				NOWA::AppStateManager::getSingletonPtr()->getEventManager()->threadSafeQueueEvent(eventDataRefreshPropertiesPanel);

				// Regenerate categories
				boost::shared_ptr<NOWA::EventDataGenerateCategories> eventDataGenerateCategories(new NOWA::EventDataGenerateCategories());
				NOWA::AppStateManager::getSingletonPtr()->getEventManager()->threadSafeQueueEvent(eventDataGenerateCategories);
			}
			else if (button == this->debugDataComponentButton)
			{
				// Show debug data if pushed
				for (size_t i = 0; i < this->gameObjectComponents.size(); i++)
				{
					this->gameObjectComponents[i]->showDebugData();
				}
				this->debugDataComponentButton->setStateCheck(this->gameObjectComponents[0]->getShowDebugData());
			}
			else if (button == this->moveUpComponentButton)
			{
				// Move up component
				std::vector<unsigned long> gameObjectIds(this->gameObjects.size());
				for (size_t i = 0; i < this->gameObjects.size(); i++)
				{
					gameObjectIds[i] = this->gameObjects[i]->getId();
					int index = this->gameObjects[i]->getIndexFromComponent(this->gameObjectComponents[0]);
					if (-1 != index)
					{
						this->gameObjects[i]->moveComponentUp(index);
					}
				}
				boost::shared_ptr<NOWA::EventDataSceneModified> eventDataSceneModified(new NOWA::EventDataSceneModified());
				NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataSceneModified);

				// Sent when a property has changed, so that the properties panel can be refreshed with new values
				boost::shared_ptr<EventDataRefreshPropertiesPanel> eventDataRefreshPropertiesPanel(new EventDataRefreshPropertiesPanel());
				NOWA::AppStateManager::getSingletonPtr()->getEventManager()->threadSafeQueueEvent(eventDataRefreshPropertiesPanel);

				// Regenerate categories
				boost::shared_ptr<NOWA::EventDataGenerateCategories> eventDataGenerateCategories(new NOWA::EventDataGenerateCategories());
				NOWA::AppStateManager::getSingletonPtr()->getEventManager()->threadSafeQueueEvent(eventDataGenerateCategories);
			}
			else if (button == this->moveDownComponentButton)
			{
				// Move down component
				std::vector<unsigned long> gameObjectIds(this->gameObjects.size());
				for (size_t i = 0; i < this->gameObjects.size(); i++)
				{
					gameObjectIds[i] = this->gameObjects[i]->getId();
					int index = this->gameObjects[i]->getIndexFromComponent(this->gameObjectComponents[0]);
					if (-1 != index)
					{
						this->gameObjects[i]->moveComponentDown(index);
					}
				}
				boost::shared_ptr<NOWA::EventDataSceneModified> eventDataSceneModified(new NOWA::EventDataSceneModified());
				NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataSceneModified);

				// Sent when a property has changed, so that the properties panel can be refreshed with new values
				boost::shared_ptr<EventDataRefreshPropertiesPanel> eventDataRefreshPropertiesPanel(new EventDataRefreshPropertiesPanel());
				NOWA::AppStateManager::getSingletonPtr()->getEventManager()->threadSafeQueueEvent(eventDataRefreshPropertiesPanel);

				// Regenerate categories
				boost::shared_ptr<NOWA::EventDataGenerateCategories> eventDataGenerateCategories(new NOWA::EventDataGenerateCategories());
				NOWA::AppStateManager::getSingletonPtr()->getEventManager()->threadSafeQueueEvent(eventDataGenerateCategories);
			}
			else
			{
				// Invert the state
				button->setStateCheck(!button->getStateCheck());
				NOWA::Variant** attribute = button->getUserData<NOWA::Variant*>();
				if (nullptr != attribute)
				{
					hasAttribute = true;
				}

				if (true == hasAttribute)
				{
					// Do not store sleep state for undo redo, because it may become nasty
					if (NOWA::PhysicsActiveComponent::AttrSleep() == (*attribute)->getName())
					{
						return;
					}

					if (false == (*attribute)->hasUserDataKey(NOWA::GameObject::AttrActionNoUndo()))
					{
						// Snapshot the old attribute name
						this->editorManager->snapshotOldGameObjectComponentAttribute(this->gameObjects, this->gameObjectComponents, (*attribute)->getName());
					}

					for (size_t i = 0; i < this->gameObjectComponents.size(); i++)
					{
						auto currentAttribute = this->gameObjectComponents[i]->getAttribute((*attribute)->getName());
						currentAttribute->setValue(button->getStateCheck());
						this->gameObjectComponents[i]->actualizeValue(*attribute);
					}

					if (false == (*attribute)->hasUserDataKey(NOWA::GameObject::AttrActionNoUndo()))
					{
						// Snapshot the new attribute
						this->editorManager->snapshotNewGameObjectComponentAttribute(*attribute);
					}
				}
			}

			if (true == hasAttribute && (false == (*attribute)->hasUserDataKey(NOWA::GameObject::AttrActionNoUndo())))
			{
				// Sent when a property has changed, so that the properties panel can be refreshed with new values
				if (nullptr != attribute && true == (*attribute)->hasUserDataKey(NOWA::GameObject::AttrActionNeedRefresh()))
				{
					boost::shared_ptr<EventDataRefreshPropertiesPanel> eventDataRefreshPropertiesPanel(new EventDataRefreshPropertiesPanel());
					NOWA::AppStateManager::getSingletonPtr()->getEventManager()->threadSafeQueueEvent(eventDataRefreshPropertiesPanel);
				}
			}
		}
	};
	NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PropertiesPanelComponent::buttonHit");
}

void PropertiesPanelComponent::notifyComboChangedPosition(MyGUI::ComboBox* sender, size_t index)
{
	ENQUEUE_RENDER_COMMAND_MULTI("PropertiesPanelComponent::notifyComboChangedPosition", _1(sender),
	{
		this->showDescription(sender);
	});

	// Send the combo box change to the game object and internally actualize the data
	NOWA::Variant** attribute = sender->getUserData<NOWA::Variant*>();

	if (false == (*attribute)->hasUserDataKey(NOWA::GameObject::AttrActionNoUndo()))
	{
		// Snapshot the old attribute name
		this->editorManager->snapshotOldGameObjectComponentAttribute(this->gameObjects, this->gameObjectComponents, (*attribute)->getName());
	}

	for (size_t i = 0; i < this->gameObjectComponents.size(); i++)
	{
		auto currentAttribute = this->gameObjectComponents[i]->getAttribute((*attribute)->getName());
		// Store also the old value in list
		(*attribute)->setListSelectedOldValue(currentAttribute->getListSelectedValue());
		currentAttribute->setListSelectedValue(sender->getItem(index));

		this->gameObjectComponents[i]->actualizeValue(*attribute);
	}

	if (false == (*attribute)->hasUserDataKey(NOWA::GameObject::AttrActionNoUndo()))
	{
		// Snapshot the new attribute
		this->editorManager->snapshotNewGameObjectComponentAttribute(*attribute);

		if (true == (*attribute)->hasUserDataKey(NOWA::GameObject::AttrActionNeedRefresh()))
		{
			boost::shared_ptr<EventDataRefreshPropertiesPanel> eventDataRefreshPropertiesPanel(new EventDataRefreshPropertiesPanel());
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshPropertiesPanel);
		}
	}

	// Disabled: Because it messes up with prior set values in other field, because on refresh somehow they will be resetted!
		// Sent when a property has changed, so that the properties panel can be refreshed with new values
		/*boost::shared_ptr<EventDataRefreshPropertiesPanel> eventDataRefreshPropertiesPanel(new EventDataRefreshPropertiesPanel());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshPropertiesPanel);*/
}

void PropertiesPanelComponent::notifyColourAccept(MyGUI::ColourPanel* sender)
{
	ENQUEUE_RENDER_COMMAND_MULTI("PropertiesPanelComponent::notifyColourAccept", _1(sender),
	{
		Ogre::Vector4 colour(sender->getColour().red, sender->getColour().green, sender->getColour().blue, sender->getColour().alpha);

		NOWA::Variant * *copiedAttribute = sender->getWidget()->getUserData<NOWA::Variant*>();
		if (NOWA::Variant::VAR_VEC3 == (*copiedAttribute)->getType())
			(*copiedAttribute)->setValue(Ogre::Vector3(colour.x, colour.y, colour.z));
		else
			(*copiedAttribute)->setValue(colour);

		MyGUI::InputManager::getInstancePtr()->removeWidgetModal(ColourPanelManager::getInstance()->getColourPanel()->getWidget());
		ColourPanelManager::getInstance()->getColourPanel()->setVisible(false);

		if (false == (*copiedAttribute)->hasUserDataKey(NOWA::GameObject::AttrActionNoUndo()))
		{
			// Snapshot the old attribute name
			this->editorManager->snapshotOldGameObjectComponentAttribute(this->gameObjects, this->gameObjectComponents, (*copiedAttribute)->getName());
		}

		for (size_t i = 0; i < this->gameObjectComponents.size(); i++)
		{
			auto currentAttribute = this->gameObjectComponents[i]->getAttribute((*copiedAttribute)->getName());
			this->gameObjectComponents[i]->actualizeValue(*copiedAttribute);
		}

		if (false == (*copiedAttribute)->hasUserDataKey(NOWA::GameObject::AttrActionNoUndo()))
		{
			// Snapshot the new attribute
			this->editorManager->snapshotNewGameObjectComponentAttribute(*copiedAttribute);
		}
		// Delete the copied attribute
		delete (*copiedAttribute);

		// Sent when a property has changed, so that the properties panel can be refreshed with new values
		boost::shared_ptr<EventDataRefreshPropertiesPanel> eventDataRefreshPropertiesPanel(new EventDataRefreshPropertiesPanel());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->threadSafeQueueEvent(eventDataRefreshPropertiesPanel);
	});
}

void PropertiesPanelComponent::notifyEndDialog(tools::Dialog* sender, bool result)
{
	ENQUEUE_RENDER_COMMAND_MULTI("PropertiesPanelComponent::notifyEndDialog", _2(sender, result),
	{
		if (true == result)
		{
			PropertiesPanel::setShowPropertiesFlag(true);
			if (this->openSaveFileDialog->getMode() == "FileOpen")
			{
				MyGUI::InputManager::getInstancePtr()->setMouseFocusWidget(this->openSaveFileDialog->getMainWidget());
				MyGUI::InputManager::getInstancePtr()->setKeyFocusWidget(this->openSaveFileDialog->getMainWidget());
				NOWA::Variant** copiedAttribute = sender->getMainWidget()->getUserData<NOWA::Variant*>();
				Ogre::String tempFileName = this->openSaveFileDialog->getFileName();
				(*copiedAttribute)->setValue(tempFileName);

				if (false == (*copiedAttribute)->hasUserDataKey(NOWA::GameObject::AttrActionNoUndo()))
				{
					// Snapshot the old attribute name
					this->editorManager->snapshotOldGameObjectComponentAttribute(this->gameObjects, this->gameObjectComponents, (*copiedAttribute)->getName());
				}

				for (size_t i = 0; i < this->gameObjectComponents.size(); i++)
				{
					auto currentAttribute = this->gameObjectComponents[i]->getAttribute((*copiedAttribute)->getName());
					this->gameObjectComponents[i]->actualizeValue(*copiedAttribute);
				}

				if (false == (*copiedAttribute)->hasUserDataKey(NOWA::GameObject::AttrActionNoUndo()))
				{
					// Snapshot the new attribute
					this->editorManager->snapshotNewGameObjectComponentAttribute(*copiedAttribute);
				}
				// Delete the copied attribute
				delete (*copiedAttribute);

				// Sent when a property has changed, so that the properties panel can be refreshed with new values
				boost::shared_ptr<EventDataRefreshPropertiesPanel> eventDataRefreshPropertiesPanel(new EventDataRefreshPropertiesPanel());
				NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataRefreshPropertiesPanel);
			}
		}
		if (nullptr != this->openSaveFileDialog)
		{
			this->openSaveFileDialog->endModal();
			MyGUI::InputManager::getInstancePtr()->_resetMouseFocusWidget();
			MyGUI::InputManager::getInstancePtr()->resetKeyFocusWidget();
			// Lets the camera move again
			NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setMoveCameraWeight(1.0f);
			NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->setRotateCameraWeight(1.0f);
		}
	});
}
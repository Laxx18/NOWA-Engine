#ifndef MYGUI_HELPER_H
#define MYGUI_HELPER_H

#include "MyGUI.h"
#include "OgreString.h"

class MyGUIHelper
{
public:
	static MyGUIHelper* MyGUIHelper::getInstance()
	{
		static MyGUIHelper instance;

		return &instance;
	}

	template <typename T>
	T* findParentWidget(MyGUI::Widget* sourceWidget, const Ogre::String& widgetName)
	{
		T* targetWidget = nullptr;
		if (nullptr == sourceWidget)
			return targetWidget;

		MyGUI::Widget* parentWidget = sourceWidget->getParent();

		while (parentWidget != nullptr)
		{
			size_t found = parentWidget->getName().find(widgetName);
			if (Ogre::String::npos != found)
			{
				targetWidget = parentWidget->castType<T>();
				return targetWidget;
			}
			parentWidget = parentWidget->getParent();
		}

		return targetWidget;
	}

	template <typename T>
	T* findParentWidgetByType(MyGUI::Widget* sourceWidget)
	{
		T* targetWidget = nullptr;
		if (nullptr == sourceWidget)
			return targetWidget;

		MyGUI::Widget* parentWidget = sourceWidget->getParent();

		while (parentWidget != nullptr)
		{
			targetWidget = parentWidget->castType<T>(false);
			if (nullptr != targetWidget)
			{
				return targetWidget;
			}
			parentWidget = parentWidget->getParent();
		}

		return targetWidget;
	}

	std::tuple<bool, unsigned long, unsigned int, MyGUI::Widget*> findWidgetGameObject(int mouseX, int mouseY)
	{
		std::pair<unsigned long, unsigned int>* userData = nullptr;

		MyGUI::Widget* foundWidget = MyGUI::LayerManager::getInstance().getWidgetFromPoint(mouseX, mouseY);
		MyGUI::Widget* parentWidget = MyGUIHelper::getInstance()->findParentWidgetByType<MyGUI::Widget>(foundWidget);

		do
		{
			if (nullptr == parentWidget)
			{
				break;
			}
			userData = parentWidget->getUserData<std::pair<unsigned long, unsigned int>>(false);
			if (nullptr == userData)
			{
				parentWidget = MyGUIHelper::getInstance()->findParentWidgetByType<MyGUI::Widget>(parentWidget);
				if (nullptr == parentWidget)
				{
					break;
				}
			}
			else
			{
				break;
			}

		} while (nullptr == userData);

		if (nullptr == userData)
		{
			return std::make_tuple(false, 0, 0, nullptr);
		}

		return std::make_tuple(true, userData->first, userData->second, parentWidget);
	}

	void pairIdWithEditBox(const Ogre::String& id)
	{
		if (nullptr != MyGUIHelper::editBox)
		{
			MyGUIHelper::editBox->setOnlyText(id);
			this->attribute->setValue(Ogre::StringConverter::parseUnsignedLong(id));
		}
		MyGUIHelper::clearIdPairing();
	}

	void setDataForPairing(MyGUI::EditBox* editBox, NOWA::Variant* attribute)
	{
		this->editBox = editBox;
		this->attribute = attribute;
	}

	bool isIdPairingActive(void)
	{
		return nullptr != MyGUIHelper::editBox;
	}

	void clearIdPairing(void)
	{
		MyGUIHelper::editBox = nullptr;
		this->attribute = nullptr;
	}

	MyGUI::Colour getDefaultTextColour(void) const
	{
		return this->defaultTextColour;
	}

	void setDefaultTextColour(const MyGUI::Colour& defaultTextColour)
	{
		this->defaultTextColour = defaultTextColour;
	}

	MyGUI::Colour getImportantTextColour(void) const
	{
		return this->importantTextColour;
	}

	void setImporantTextColour(const MyGUI::Colour& importantTextColour)
	{
		this->importantTextColour = importantTextColour;
	}

	MyGUI::Colour getTextSelectColour(void) const
	{
		return this->textSelectColour;
	}

	void setTextSelectColour(const MyGUI::Colour& textSelectColour)
	{
		this->textSelectColour = textSelectColour;
	}

	void adaptFocus(MyGUI::Widget* sender, MyGUI::KeyCode code, const MyGUI::VectorWidgetPtr& items)
	{
		// Accept value
		if (code == MyGUI::KeyCode::Return || code == MyGUI::KeyCode::Tab)
		{
			MyGUI::InputManager::getInstancePtr()->_resetMouseFocusWidget();
			MyGUI::InputManager::getInstancePtr()->resetKeyFocusWidget(sender);
			sender->eventRootMouseChangeFocus(sender, false);
		}

		// Propagate via tab to next widget
		if (false == NOWA::InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(OIS::KC_LSHIFT) && code == MyGUI::KeyCode::Tab)
		{
			for (size_t i = 0; i < items.size(); i++)
			{
				if (items[i] == sender && i < items.size() - 1)
				{
					MyGUI::InputManager::getInstance().setKeyFocusWidget(items[i + 1]);
					MyGUI::EditBox* editBox = items[i + 1]->castType<MyGUI::EditBox>(false);
					if (nullptr != editBox)
					{
						editBox->setTextSelection(0, editBox->getTextLength());
					}
					MyGUI::EditBox* priorBox = items[i]->castType<MyGUI::EditBox>(false);
					if (nullptr != priorBox)
					{
						priorBox->setTextSelection(0, 0);
					}
					break;
				}
			}
		}
		else if (true == NOWA::InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(OIS::KC_LSHIFT) && code == MyGUI::KeyCode::Tab)
		{
			if (false == items.empty())
			{
				for (size_t i = items.size() - 1; i > 0; i--)
				{
					if (items[i] == sender && i > 0)
					{
						MyGUI::InputManager::getInstance().setKeyFocusWidget(items[i - 1]);
						MyGUI::EditBox* editBox = items[i - 1]->castType<MyGUI::EditBox>(false);
						if (nullptr != editBox)
						{
							editBox->setTextSelection(0, editBox->getTextLength());
						}
						MyGUI::EditBox* priorBox = items[i]->castType<MyGUI::EditBox>(false);
						if (nullptr != priorBox)
						{
							priorBox->setTextSelection(0, 0);
						}
						break;
					}
				}
			}
		}
	}

	void resetTextSelection(MyGUI::Widget* sender, const MyGUI::VectorWidgetPtr& items)
	{
		/*MyGUI::InputManager::getInstancePtr()->_resetMouseFocusWidget();
		MyGUI::InputManager::getInstancePtr()->resetKeyFocusWidget(sender);*/

		for (size_t i = 0; i < items.size(); i++)
		{
			if (items[i] != sender && i < items.size() - 1)
			{
				// MyGUI::InputManager::getInstance().setKeyFocusWidget(items[i + 1]);
				MyGUI::EditBox* otherEditBox = items[i]->castType<MyGUI::EditBox>(false);
				if (nullptr != otherEditBox)
				{
					otherEditBox->setTextSelection(0, 0);
				}
			}
		}
	}

	void setScrollPosition(unsigned long id, int scrollPosition)
	{
		if (id != 0)
		{
			auto& foundIt = this->scrollPositions.find(id);
			if (foundIt == this->scrollPositions.cend())
			{
				this->scrollPositions.emplace(id, scrollPosition);
			}
			else
			{
				foundIt->second = scrollPosition;
			}
		}
	}

	int getScrollPosition(unsigned long id) const
	{
		auto& foundIt = this->scrollPositions.find(id);
		if (foundIt != this->scrollPositions.cend())
		{
			return foundIt->second;
		}
		return -1;
	}
	
	void initToolTipData(void)
	{
		if (nullptr == this->toolTip)
		{
			MyGUI::LayoutManager::getInstance().loadLayout("ToolTip2.layout");
			this->toolTip = MyGUI::Gui::getInstance().findWidget<MyGUI::Widget>("tooltipPanel");
			this->textDescription = MyGUI::Gui::getInstance().findWidget<MyGUI::EditBox>("text_Desc");
		}
	}

	void notifyToolTip(MyGUI::Widget* sender, const MyGUI::ToolTipInfo& info)
	{
		if (info.type == MyGUI::ToolTipInfo::Show)
		{
			// First fetch the viewport size.  (Do not try to getParent()->getSize().
			// Top level widgets do not have parents, but getParentSize() returns something useful anyway.)
			const MyGUI::IntSize& viewSize = this->toolTip->getParentSize();
			// Then set our tooltip panel size to something excessive...
			this->toolTip->setSize(viewSize.width / 2, viewSize.height / 2);
			// ... update its caption to whatever the sender widget has for tooltip text
			// (You did use setUserString(), right?)...
			MyGUI::UString toolTipText = sender->getUserString("tooltip");
			if (true == toolTipText.empty())
			{
				return;
			}
			this->textDescription->setCaption(toolTipText);
			// ... fetch the new text size from the tooltip's Edit control...
			const MyGUI::IntSize& textSize = this->textDescription->getTextSize();
			// ... and resize the tooltip panel to match it.  The Stretch property on the Edit
			// control will see to it that the Edit control resizes along with it.
			// The constants are padding to fit in the edges of the PanelSmall skin; adjust as
			// necessary for your theme.
			this->toolTip->setSize(textSize.width + 6, textSize.height + 6);
			// You can fade it in smooth if you like, but that gets obnoxious.
			this->toolTip->setVisible(true);
			boundedMove(this->toolTip, info.point);
		}
		else if (info.type == MyGUI::ToolTipInfo::Hide)
		{
			this->toolTip->setVisible(false);
		}
		else if (info.type == MyGUI::ToolTipInfo::Move)
		{
			boundedMove(this->toolTip, info.point);
		}
	}

	/** Move a widget to a point while making it stay in the viewport.
		@param moving The widget that will be moving.
		@param point The desired destination viewport coordinates
					 (which may not be the final resting place of the widget).
	 */
	void boundedMove(MyGUI::Widget* moving, const MyGUI::IntPoint& point)
	{
		const MyGUI::IntPoint offset(16, 16);  // typical mouse cursor size - offset out from under it

		MyGUI::IntPoint boundedpoint = point + offset;

		const MyGUI::IntSize& size = moving->getSize();
		const MyGUI::IntSize& viewSize = moving->getParentSize();

		if ((boundedpoint.left + size.width) > viewSize.width)
		{
			boundedpoint.left -= offset.left + offset.left + size.width;
		}
		if ((boundedpoint.top + size.height) > viewSize.height)
		{
			boundedpoint.top -= offset.top + offset.top + size.height;
		}

		moving->setPosition(boundedpoint);
	}

	void showAcceptedImage(const Ogre::Vector2& position, const Ogre::Vector2& size, Ogre::Real duration)
	{
		acceptedImageBox->setPosition(static_cast<int>(position.x), static_cast<int>(position.y));
		acceptedImageBox->setSize(static_cast<int>(size.x), static_cast<int>(size.y));
		acceptedImageBox->setVisible(true);

		NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(duration));
		// Shows for a specific amount of time
		auto ptrFunction = [this]()
			{
				acceptedImageBox->setVisible(false);
			};
		NOWA::ProcessPtr closureProcess(new NOWA::ClosureProcess(ptrFunction));
		delayProcess->attachChild(closureProcess);
		NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
	}

	void setCanMousePress(bool canMousePress)
	{
		this->canMousePress = canMousePress;
	}

	bool getCanMousePress(void) const
	{
		return this->canMousePress;
	}

private:
	MyGUIHelper()
		: editBox(nullptr),
		attribute(nullptr),
		defaultTextColour(MyGUI::Colour(0.96f, 0.96f, 0.96f)),
		importantTextColour(MyGUI::Colour(1.0f, 0.6f, 0.4f)),
		textSelectColour(MyGUI::Colour(0.9411764705882353f, 0.9411764705882353f, 0.9411764705882353f)),
		toolTip(nullptr),
		textDescription(nullptr),
		acceptedImageBox(nullptr),
		canMousePress(true)
	{
		acceptedImageBox = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::ImageBox>("ImageBox", 0.0f, 0.0f, 5.0f, 5.0f, MyGUI::Align::Default, "ToolTip");
		acceptedImageBox->setImageTexture("circleGlow.png");
		acceptedImageBox->setVisible(false);
	}

private:
	MyGUI::EditBox* editBox;
	NOWA::Variant* attribute;
	std::unordered_map<unsigned long, int> scrollPositions;
	MyGUI::Colour defaultTextColour;
	MyGUI::Colour importantTextColour;
	MyGUI::Colour textSelectColour;
	MyGUI::Widget* toolTip;
	MyGUI::EditBox* textDescription;
	MyGUI::ImageBox* acceptedImageBox;
	bool canMousePress;
};

#endif


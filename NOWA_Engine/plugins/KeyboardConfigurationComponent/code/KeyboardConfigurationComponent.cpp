#include "NOWAPrecompiled.h"
#include "KeyboardConfigurationComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "main/InputDeviceCore.h"
#include "gameobject/GameObjectFactory.h"

#include "OgreAbiUtils.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	KeyboardConfigurationComponent::KeyboardConfigurationComponent()
		: GameObjectComponent(),
		name("KeyboardConfigurationComponent"),
		hasParent(false),
		widget(nullptr),
		messageLabel(nullptr),
		okButton(nullptr),
		abordButton(nullptr),
		activated(new Variant(KeyboardConfigurationComponent::AttrActivated(), true, this->attributes)),
		relativePosition(new Variant(KeyboardConfigurationComponent::AttrRelativePosition(), Ogre::Vector2(0.325f, 0.325f), this->attributes)),
		parentId(new Variant(KeyboardConfigurationComponent::AttrParentId(), static_cast<unsigned long>(0), this->attributes, true)),
		okClickEventName(new Variant(KeyboardConfigurationComponent::AttrOkClickEventName(), Ogre::String(""), this->attributes)),
		abordClickEventName(new Variant(KeyboardConfigurationComponent::AttrAbordClickEventName(), Ogre::String(""), this->attributes))
	{
		this->activated->setDescription("Shows the configuration menu if activated.");
	}

	KeyboardConfigurationComponent::~KeyboardConfigurationComponent(void)
	{
		if (nullptr != this->widget && false == this->hasParent)
		{
			MyGUI::Gui::getInstancePtr()->destroyWidget(this->widget);
			this->widget = nullptr;
		}
	}

	void KeyboardConfigurationComponent::initialise()
	{

	}

	const Ogre::String& KeyboardConfigurationComponent::getName() const
	{
		return this->name;
	}

	void KeyboardConfigurationComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<KeyboardConfigurationComponent>(KeyboardConfigurationComponent::getStaticClassId(), KeyboardConfigurationComponent::getStaticClassName());
	}

	void KeyboardConfigurationComponent::shutdown()
	{

	}

	void KeyboardConfigurationComponent::uninstall()
	{

	}

	void KeyboardConfigurationComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool KeyboardConfigurationComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RelativePosition")
		{
			this->relativePosition->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ParentId")
		{
			this->parentId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OkClickEventName")
		{
			this->setOkClickEventName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AbordClickEventName")
		{
			this->setAbordClickEventName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		unsigned char i = 0;
		while (propertyElement)
		{
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Key_" + Ogre::StringConverter::toString(i))
			{
				// Store the mappings in a temp container, because at this time the game object is not available yet
				this->keyCodes.emplace(std::make_pair(i, Ogre::StringConverter::parseInt(propertyElement->first_attribute("data")->value())));
			}
			i++;
			propertyElement = propertyElement->next_sibling("property");
		}

		return true;
	}

	GameObjectCompPtr KeyboardConfigurationComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		// Cloning does not make sense
		return nullptr;
	}

	bool KeyboardConfigurationComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[KeyboardConfigurationComponent] Init component for game object: " + this->gameObjectPtr->getName());

		this->setActivated(this->activated->getBool());

		const auto& inputDeviceModule = InputDeviceCore::getSingletonPtr()->getKeyboardInputDeviceModule(this->gameObjectPtr->getId());

		if (nullptr != inputDeviceModule)
		{
			for (auto& it = this->keyCodes.cbegin(); it != this->keyCodes.cend(); ++it)
			{
				OIS::KeyCode keyCode = static_cast<OIS::KeyCode>(it->second);
				if (OIS::KC_UNASSIGNED != keyCode)
				{
					inputDeviceModule->remapKey(static_cast<InputDeviceModule::Action>(it->first), keyCode);
				}
			}
		}

		return true;
	}

	bool KeyboardConfigurationComponent::connect(void)
	{
		GameObjectComponent::connect();

		// Sets the event handler
		if (nullptr != this->widget)
		{
			this->okButton->eventMouseButtonClick += MyGUI::newDelegate(this, &KeyboardConfigurationComponent::buttonHit);
			this->abordButton->eventMouseButtonClick += MyGUI::newDelegate(this, &KeyboardConfigurationComponent::buttonHit);
			for (unsigned short i = 0; i < keyConfigTextboxes.size(); i++)
			{
				auto keyCode = InputDeviceCore::getSingletonPtr()->getKeyboardInputDeviceModule(this->gameObjectPtr->getId())->getMappedKey(static_cast<InputDeviceModule::Action>(i));
				Ogre::String strKeyCode = InputDeviceCore::getSingletonPtr()->getKeyboardInputDeviceModule(this->gameObjectPtr->getId())->getStringFromMappedKey(keyCode);
				this->oldKeyValue[i] = strKeyCode;
				this->newKeyValue[i] = strKeyCode;
				// Ogre::String strKeyCode = NOWA::Core::getSingletonPtr()->getKeyboard()->getAsString(keyCode);
				this->keyConfigTextboxes[i]->setNeedMouseFocus(true);
				this->keyConfigTextboxes[i]->setCaptionWithReplacing(strKeyCode);
				this->keyConfigTextboxes[i]->eventMouseSetFocus += MyGUI::newDelegate(this, &KeyboardConfigurationComponent::notifyMouseSetFocus);
				this->keyConfigTextboxes[i]->eventKeyButtonPressed += MyGUI::newDelegate(this, &KeyboardConfigurationComponent::keyPressed);
			}
		}
		
		return true;
	}

	bool KeyboardConfigurationComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();

		if (nullptr != this->okButton)
		{
			this->okButton->eventMouseButtonClick -= MyGUI::newDelegate(this, &KeyboardConfigurationComponent::buttonHit);
			this->abordButton->eventMouseButtonClick -= MyGUI::newDelegate(this, &KeyboardConfigurationComponent::buttonHit);
		}

		for (unsigned short i = 0; i < keyConfigTextboxes.size(); i++)
		{
			this->keyConfigTextboxes[i]->eventMouseSetFocus -= MyGUI::newDelegate(this, &KeyboardConfigurationComponent::notifyMouseSetFocus);
			this->keyConfigTextboxes[i]->eventKeyButtonPressed -= MyGUI::newDelegate(this, &KeyboardConfigurationComponent::keyPressed);
		}

		return true;
	}

	bool KeyboardConfigurationComponent::onCloned(void)
	{
		
		return true;
	}

	void KeyboardConfigurationComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		this->destroyMyGUIWidgets();
	}

	void KeyboardConfigurationComponent::update(Ogre::Real dt, bool notSimulating)
	{

	}

	void KeyboardConfigurationComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (KeyboardConfigurationComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (KeyboardConfigurationComponent::AttrRelativePosition() == attribute->getName())
		{
			this->setRelativePosition(attribute->getVector2());
		}
		else if (KeyboardConfigurationComponent::AttrParentId() == attribute->getName())
		{
			this->setParentId(attribute->getULong());
		}
		else if (KeyboardConfigurationComponent::AttrOkClickEventName() == attribute->getName())
		{
			this->setOkClickEventName(attribute->getString());
		}
		else if (KeyboardConfigurationComponent::AttrAbordClickEventName() == attribute->getName())
		{
			this->setAbordClickEventName(attribute->getString());
		}
	}

	void KeyboardConfigurationComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		GameObjectComponent::writeXML(propertiesXML, doc);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RelativePosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->relativePosition->getVector2())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ParentId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->parentId->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OkClickEventName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->okClickEventName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AbordClickEventName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->abordClickEventName->getString())));
		propertiesXML->append_node(propertyXML);

		const auto& inputDeviceModule = InputDeviceCore::getSingletonPtr()->getKeyboardInputDeviceModule(this->gameObjectPtr->getId());

		if (nullptr != inputDeviceModule)
		{
			for (unsigned short i = 0; i < inputDeviceModule->getKeyMappingCount(); i++)
			{
				propertyXML = doc.allocate_node(node_element, "property");
				propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
				propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "Key_" + Ogre::StringConverter::toString(i))));

				Ogre::String mappedKeyCode = Ogre::StringConverter::toString(inputDeviceModule->getMappedKey(static_cast<InputDeviceModule::Action>(i)));

				propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, mappedKeyCode)));
				propertiesXML->append_node(propertyXML);
			}
		}
	}

	Ogre::String KeyboardConfigurationComponent::getClassName(void) const
	{
		return "KeyboardConfigurationComponent";
	}

	Ogre::String KeyboardConfigurationComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void KeyboardConfigurationComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);

		if (true == activated)
		{
			this->createMyGuiWidgets();
		}
		else
		{
			this->destroyMyGUIWidgets();
		}
	}

	bool KeyboardConfigurationComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void KeyboardConfigurationComponent::setRelativePosition(const Ogre::Vector2& relativePosition)
	{
		this->relativePosition->setValue(relativePosition);

		if (nullptr != this->widget)
		{
			// Change size
			this->widget->setRealPosition(relativePosition.x, relativePosition.y);
		}
	}

	Ogre::Vector2 KeyboardConfigurationComponent::getRelativePosition(void) const
	{
		return this->relativePosition->getVector2();
	}

	void KeyboardConfigurationComponent::setParentId(unsigned long parentId)
	{
		this->parentId->setValue(parentId);

		// Attention: Remove Item is missing, when id has changed or onRemoveComponent

		// Somehow it can happen between a cloning process and value actualisation, that the id and predecessor id may become the same, so prevent it!

		this->parentId->setValue(parentId);
		
		if (nullptr != this->gameObjectPtr)
		{
			for (unsigned int i = 0; i < this->gameObjectPtr->getComponents()->size(); i++)
			{
				auto& gameObjectCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponentByIndex(i));
				if (nullptr != gameObjectCompPtr)
				{
					auto& myGuiCompPtr = boost::dynamic_pointer_cast<MyGUIComponent>(gameObjectCompPtr);
					if (nullptr != myGuiCompPtr && this->parentId->getULong() == myGuiCompPtr->getId())
					{
						if (nullptr != this->widget && nullptr != myGuiCompPtr->getWidget())
						{
							this->widget->attachToWidget(myGuiCompPtr->getWidget());
							this->setRelativePosition(this->relativePosition->getVector2());
							this->hasParent = true;
							// Hide the layer if the widget is part of a parent, because it uses the layer from the parent
							// this->layer->setVisible(false);
							break;
						}
					}
				}
				else
				{
					this->widget->detachFromWidget();
					this->hasParent = false;
					// Show the layer again
					// this->layer->setVisible(true);
				}
			}
		}
	}

	unsigned long KeyboardConfigurationComponent::getParentId(void) const
	{
		return this->parentId->getULong();
	}

	void KeyboardConfigurationComponent::setOkClickEventName(const Ogre::String& okClickEventName)
	{
		this->okClickEventName->setValue(okClickEventName);
		this->okClickEventName->addUserData(GameObject::AttrActionGenerateLuaFunction(), okClickEventName + "(thisComponent)=" + this->getClassName());
	}

	Ogre::String KeyboardConfigurationComponent::getOkClickEventName(void) const
	{
		return this->okClickEventName->getString();
	}

	void KeyboardConfigurationComponent::setAbordClickEventName(const Ogre::String& abordClickEventName)
	{
		this->abordClickEventName->setValue(abordClickEventName);
		this->abordClickEventName->addUserData(GameObject::AttrActionGenerateLuaFunction(), abordClickEventName + "(thisComponent)=" + this->getClassName());
	}

	Ogre::String KeyboardConfigurationComponent::getAbordClickEventName(void) const
	{
		return this->abordClickEventName->getString();
	}

	MyGUI::Window* KeyboardConfigurationComponent::getWindow(void) const
	{
		return this->widget;
	}

	void KeyboardConfigurationComponent::createMyGuiWidgets(void)
	{
		const unsigned short count = 31;

		// Layers: "Wallpaper", "ToolTip", "Info", "FadeMiddle", "Popup", "Main", "Modal", "Middle", "Overlapped", "Back", "DragAndDrop", "FadeBusy", "Pointer", "Fade", "Statistic"
		this->widget = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::Window>("Window", this->relativePosition->getVector2().x, this->relativePosition->getVector2().y, 0.2f, 0.25f, MyGUI::Align::Left, "Popup");
		this->widget->setCaptionWithReplacing("#{Keyboard_Configuration}");
		this->setRelativePosition(this->relativePosition->getVector2());
		// Attach maybe widget to parent
		this->setParentId(this->parentId->getULong());

		auto scrollView = this->widget->createWidgetReal<MyGUI::ScrollView>("ScrollView", 0.0f, 0.0f, 0.99f, 0.7f, MyGUI::Align::Left, "Popup");
		scrollView->setCanvasSize(480, count * 38);
		// scrollView->attachToWidget(this->widget);
		// scrollView->setVRange(100);

		Ogre::Real sizeX = 0.4f;
		Ogre::Real editSizeX = 0.5f;
		Ogre::Real sizeY = 0.15f;
		Ogre::Real marginX = 0.01f;
		Ogre::Real marginY = 0.0f;

		Ogre::Real posX = 0.02f;
		Ogre::Real posY = 0.02f;
		Ogre::Real totalHeight = 0.0f;

		this->keyConfigLabels.resize(count);
		this->keyConfigTextboxes.resize(count);
		this->textboxActive.resize(count, false);
		this->oldKeyValue.resize(count, "");
		this->newKeyValue.resize(count, "");

		for (unsigned short i = 0; i < count; i++)
		{
			this->keyConfigLabels[i] = scrollView->createWidgetReal<MyGUI::EditBox>("TextBox", posX, posY + marginY, sizeX, sizeY, MyGUI::Align::Left, "Popup");
			// this->keyConfigLabels[i]->attachToWidget(scrollView);

			this->keyConfigTextboxes[i] = scrollView->createWidgetReal<MyGUI::EditBox>("EditBox", posX + editSizeX + marginX, posY + marginY, sizeX, sizeY, MyGUI::Align::Left, "Popup");
			this->keyConfigTextboxes[i]->setEditReadOnly(true);
			// this->keyConfigTextboxes[i]->attachToWidget(scrollView);

			auto keyCode = InputDeviceCore::getSingletonPtr()->getKeyboardInputDeviceModule(this->gameObjectPtr->getId())->getMappedKey(static_cast<InputDeviceModule::Action>(i));
			Ogre::String strKeyCode = InputDeviceCore::getSingletonPtr()->getKeyboardInputDeviceModule(this->gameObjectPtr->getId())->getStringFromMappedKey(keyCode);
			this->oldKeyValue[i] = strKeyCode;
			this->newKeyValue[i] = strKeyCode;
			this->keyConfigTextboxes[i]->setNeedMouseFocus(true);
			this->keyConfigTextboxes[i]->setCaptionWithReplacing(strKeyCode);

			posY += marginY;
			totalHeight += posY + marginY + sizeY;
			marginY = 0.2f;
		}

		// scrollView->setCanvasSize((posX + marginX + sizeX + editSizeX) * scrollView->getClientCoord().width, totalHeight * scrollView->getClientCoord().height);

		this->messageLabel = scrollView->createWidgetReal<MyGUI::EditBox>("TextBox", posX + marginX + marginX + sizeX + editSizeX, posY + marginY, sizeX, sizeY, MyGUI::Align::Left, "Popup");
		this->messageLabel->setCaptionWithReplacing("#{Key_Existing}");
		this->messageLabel->setTextColour(MyGUI::Colour::Red);
		// this->messageLabel->attachToWidget(scrollView);
		this->messageLabel->setVisible(false);

		unsigned short i = 0;
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Move_Up}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Move_Down}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Move_Left}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Move_Right}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Jump}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Run}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Cower}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Attack_1}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Attack_2}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Duck}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Sneak}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Action}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Reload}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Inventory}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Map}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Select}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Start}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Save}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Load}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Camera_Forward}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Camera_Backward}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Camera_Left}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Camera_Right}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Camera_Up}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Camera_Down}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Console}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Weapon_Change_Forward}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Weapon_Change_Backward}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Flashlight}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Pause}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Grid}:");

		this->okButton = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::Button>("Button", 0.0f, 0.0f, 0.05f, 0.02f, MyGUI::Align::Left, "Popup", "okButton");
		this->okButton->attachToWidget(this->widget);
		this->okButton->setRealPosition(0.02f, 0.91f);
		this->okButton->setAlign(MyGUI::Align::Left);
		this->okButton->setCaption("Ok");

		this->abordButton = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::Button>("Button", 0.0f, 0.0f, 0.05f, 0.02f, MyGUI::Align::Left, "Popup", "abordButton");
		this->abordButton->attachToWidget(this->widget);
		this->abordButton->setRealPosition(0.5f + 0.21f, 0.91f);
		this->abordButton->setAlign(MyGUI::Align::Left);
		this->abordButton->setCaptionWithReplacing("#{Abord}");
	}

	void KeyboardConfigurationComponent::destroyMyGUIWidgets(void)
	{
		if (true == this->hasParent)
		{
			this->widget->detachFromWidget();
		}
		MyGUI::Gui::getInstancePtr()->destroyWidget(this->widget);
		this->widget = nullptr;

		this->keyConfigLabels.clear();
		this->keyConfigTextboxes.clear();
		this->textboxActive.clear();
		this->oldKeyValue.clear();
		this->newKeyValue.clear();
	}

	void KeyboardConfigurationComponent::notifyMouseSetFocus(MyGUI::Widget* sender, MyGUI::Widget* old)
	{
		for (unsigned short i = 0; i < this->keyConfigTextboxes.size(); i++)
		{
			this->keyConfigTextboxes[i]->setTextShadow(false);
			this->textboxActive[i] = false;
			if (sender == this->keyConfigTextboxes[i])
			{
				this->textboxActive[i] = true;
				this->keyConfigTextboxes[i]->setTextShadow(true);
			}
		}
	}

	void KeyboardConfigurationComponent::buttonHit(MyGUI::Widget* sender)
	{
		if ("okButton" == sender->getName())
		{
			// Reset mappings, but not all, because else camera etc. cannot be moved anymore
			InputDeviceCore::getSingletonPtr()->getKeyboardInputDeviceModule(this->gameObjectPtr->getId())->clearKeyMapping(this->keyConfigTextboxes.size());

			for (unsigned short i = 0; i < this->keyConfigTextboxes.size(); i++)
			{
				this->oldKeyValue[i] = this->keyConfigTextboxes[i]->getCaption();
				this->textboxActive[i] = false;

				OIS::KeyCode key = InputDeviceCore::getSingletonPtr()->getKeyboardInputDeviceModule(this->gameObjectPtr->getId())->getMappedKeyFromString(this->newKeyValue[i]);
				InputDeviceCore::getSingletonPtr()->getKeyboardInputDeviceModule(this->gameObjectPtr->getId())->remapKey(static_cast<InputDeviceModule::Action>(i), key);
			}

			if (nullptr != this->gameObjectPtr->getLuaScript() && false == this->okClickEventName->getString().empty())
			{
				NOWA::AppStateManager::LogicCommand logicCommand = [this]()
				{
					this->gameObjectPtr->getLuaScript()->callTableFunction(this->okClickEventName->getString(), this);
				};
				NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
			}
		}
		else if ("abordButton" == sender->getName())
		{
			for (unsigned short i = 0; i < this->keyConfigTextboxes.size(); i++)
			{
				this->keyConfigTextboxes[i]->setCaptionWithReplacing(this->oldKeyValue[i]);
				this->newKeyValue[i] = this->oldKeyValue[i];
				this->textboxActive[i] = false;
			}

			if (nullptr != this->gameObjectPtr->getLuaScript() && false == this->abordClickEventName->getString().empty())
			{
				NOWA::AppStateManager::LogicCommand logicCommand = [this]()
				{
					this->gameObjectPtr->getLuaScript()->callTableFunction(this->abordClickEventName->getString(), this);
				};
				NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
			}
		}
	}

	void KeyboardConfigurationComponent::keyPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char ch)
	{
		// Check if an editbox is active and set the pressed key to the edit box for key-mapping
		bool keepMappingActive = false;
		bool alreadyExisting = false;

		const unsigned short count = 31;

		short index = -1;
		Ogre::String strKeyCode;
		for (unsigned short i = 0; i < this->keyConfigTextboxes.size(); i++)
		{
			if (true == this->textboxActive[i])
			{
				index = i;
				// this->oldKeyValue[i] = this->keyConfigTextboxes[i]->getCaption();
				// get key string and set the text
				strKeyCode = InputDeviceCore::getSingletonPtr()->getKeyboardInputDeviceModule(this->gameObjectPtr->getId())->getStringFromMappedKey(static_cast<OIS::KeyCode>(key.getValue()));
				this->textboxActive[i] = false;
				this->keyConfigTextboxes[i]->setTextShadow(false);
				keepMappingActive = true;
			}
		}

		// Check if the key does not exist already
		for (unsigned short i = 0; i < this->keyConfigTextboxes.size(); i++)
		{
			if (this->keyConfigTextboxes[i]->getCaption() == strKeyCode)
			{
				alreadyExisting = true;
				this->keyConfigTextboxes[i]->setCaptionWithReplacing("None");
				this->newKeyValue[i] = this->keyConfigTextboxes[i]->getCaption();
				this->messageLabel->setVisible(true);
				break;
			}
		}

		// Set the new key
		if (-1 != index /*&& !alreadyExisting*/ && !strKeyCode.empty())
		{
			this->messageLabel->setVisible(false);
			this->keyConfigTextboxes[index]->setCaptionWithReplacing(strKeyCode);
			this->newKeyValue[index] = this->keyConfigTextboxes[index]->getCaption();


			if (index < count - 1)
			{
				MyGUI::InputManager::getInstance().setKeyFocusWidget(this->keyConfigTextboxes[index + 1]);
				this->notifyMouseSetFocus(this->keyConfigTextboxes[index + 1], this->keyConfigTextboxes[index]);
			}
		}
	}

	bool KeyboardConfigurationComponent::canStaticAddComponent(GameObject* gameObject)
	{
		if (nullptr == InputDeviceCore::getSingletonPtr()->getKeyboardInputDeviceModule(gameObject->getId()))
		{
			return false;
		}

		if (1 == gameObject->getComponentCount("InputDeviceComponent") && gameObject->getComponentCount<KeyboardConfigurationComponent>() < 2)
		{
			return true;
		}
		return false;
	}

	// Lua registration part

	KeyboardConfigurationComponent* getKeyboardConfigurationComponentFromIndex(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<KeyboardConfigurationComponent>(gameObject->getComponentWithOccurrence<KeyboardConfigurationComponent>(occurrenceIndex)).get();
	}

	KeyboardConfigurationComponent* getKeyboardConfigurationComponent(GameObject* gameObject)
	{
		return makeStrongPtr<KeyboardConfigurationComponent>(gameObject->getComponent<KeyboardConfigurationComponent>()).get();
	}

	KeyboardConfigurationComponent* getKeyboardConfigurationComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<KeyboardConfigurationComponent>(gameObject->getComponentFromName<KeyboardConfigurationComponent>(name)).get();
	}

	void KeyboardConfigurationComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<KeyboardConfigurationComponent, GameObjectComponent>("KeyboardConfigurationComponent")
			.def("setActivated", &KeyboardConfigurationComponent::setActivated)
			.def("isActivated", &KeyboardConfigurationComponent::isActivated)
		];

		LuaScriptApi::getInstance()->addClassToCollection("KeyboardConfigurationComponent", "class inherits GameObjectComponent", KeyboardConfigurationComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("KeyboardConfigurationComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not.");
		LuaScriptApi::getInstance()->addClassToCollection("KeyboardConfigurationComponent", "bool isActivated()", "Gets whether this component is activated.");

		gameObjectClass.def("getKeyboardConfigurationComponentFromName", &getKeyboardConfigurationComponentFromName);
		gameObjectClass.def("getKeyboardConfigurationComponent", (KeyboardConfigurationComponent * (*)(GameObject*)) & getKeyboardConfigurationComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "KeyboardConfigurationComponent getKeyboardConfigurationComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "KeyboardConfigurationComponent getKeyboardConfigurationComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castKeyboardConfigurationComponent", &GameObjectController::cast<KeyboardConfigurationComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "KeyboardConfigurationComponent castKeyboardConfigurationComponent(KeyboardConfigurationComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

}; //namespace end
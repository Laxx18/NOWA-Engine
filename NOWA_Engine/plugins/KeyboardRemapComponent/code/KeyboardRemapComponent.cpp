#include "NOWAPrecompiled.h"
#include "KeyboardRemapComponent.h"
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

	KeyboardRemapComponent::KeyboardRemapComponent()
		: GameObjectComponent(),
		name("KeyboardRemapComponent"),
		hasParent(false),
		widget(nullptr),
		messageLabel(nullptr),
		okButton(nullptr),
		abordButton(nullptr),
		activated(new Variant(KeyboardRemapComponent::AttrActivated(), true, this->attributes)),
		relativePosition(new Variant(KeyboardRemapComponent::AttrRelativePosition(), Ogre::Vector2(0.325f, 0.325f), this->attributes)),
		parentId(new Variant(KeyboardRemapComponent::AttrParentId(), static_cast<unsigned long>(0), this->attributes, true)),
		okClickEventName(new Variant(KeyboardRemapComponent::AttrOkClickEventName(), Ogre::String(""), this->attributes)),
		abordClickEventName(new Variant(KeyboardRemapComponent::AttrAbordClickEventName(), Ogre::String(""), this->attributes))
	{
		this->activated->setDescription("Shows the remap menu if activated.");
	}

	KeyboardRemapComponent::~KeyboardRemapComponent(void)
	{
		if (nullptr != this->widget && false == this->hasParent)
		{
			MyGUI::Gui::getInstancePtr()->destroyWidget(this->widget);
			this->widget = nullptr;
		}
	}

	void KeyboardRemapComponent::initialise()
	{

	}

	const Ogre::String& KeyboardRemapComponent::getName() const
	{
		return this->name;
	}

	void KeyboardRemapComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<KeyboardRemapComponent>(KeyboardRemapComponent::getStaticClassId(), KeyboardRemapComponent::getStaticClassName());
	}

	void KeyboardRemapComponent::shutdown()
	{

	}

	void KeyboardRemapComponent::uninstall()
	{

	}

	void KeyboardRemapComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool KeyboardRemapComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

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
		return true;
	}

	GameObjectCompPtr KeyboardRemapComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		// Cloning does not make sense
		return nullptr;
	}

	bool KeyboardRemapComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[KeyboardRemapComponent] Init component for game object: " + this->gameObjectPtr->getName());

		this->setActivated(this->activated->getBool());

		return true;
	}

	bool KeyboardRemapComponent::connect(void)
	{
		// Sets the event handler
		if (nullptr != this->widget)
		{
			this->okButton->eventMouseButtonClick += MyGUI::newDelegate(this, &KeyboardRemapComponent::buttonHit);
			this->abordButton->eventMouseButtonClick += MyGUI::newDelegate(this, &KeyboardRemapComponent::buttonHit);
			for (unsigned short i = 0; i < keyConfigTextboxes.size(); i++)
			{
				auto keyCode = InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedKey(static_cast<InputDeviceModule::Action>(i));
				Ogre::String strKeyCode = InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getStringFromMappedKey(keyCode);
				this->oldKeyValue[i] = strKeyCode;
				this->newKeyValue[i] = strKeyCode;
				// Ogre::String strKeyCode = NOWA::Core::getSingletonPtr()->getKeyboard()->getAsString(keyCode);
				this->keyConfigTextboxes[i]->setNeedMouseFocus(true);
				this->keyConfigTextboxes[i]->setCaptionWithReplacing(strKeyCode);
				this->keyConfigTextboxes[i]->eventMouseSetFocus += MyGUI::newDelegate(this, &KeyboardRemapComponent::notifyMouseSetFocus);
				this->keyConfigTextboxes[i]->eventKeyButtonPressed += MyGUI::newDelegate(this, &KeyboardRemapComponent::keyPressed);
			}
		}
		
		return true;
	}

	bool KeyboardRemapComponent::disconnect(void)
	{
		if (nullptr != this->okButton)
		{
			this->okButton->eventMouseButtonClick -= MyGUI::newDelegate(this, &KeyboardRemapComponent::buttonHit);
			this->abordButton->eventMouseButtonClick -= MyGUI::newDelegate(this, &KeyboardRemapComponent::buttonHit);
		}

		for (unsigned short i = 0; i < keyConfigTextboxes.size(); i++)
		{
			this->keyConfigTextboxes[i]->eventMouseSetFocus -= MyGUI::newDelegate(this, &KeyboardRemapComponent::notifyMouseSetFocus);
			this->keyConfigTextboxes[i]->eventKeyButtonPressed -= MyGUI::newDelegate(this, &KeyboardRemapComponent::keyPressed);
		}

		return true;
	}

	bool KeyboardRemapComponent::onCloned(void)
	{
		
		return true;
	}

	void KeyboardRemapComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		if (nullptr != this->widget)
		{
			this->widget->detachFromWidget();
		}
		this->hasParent = false;
	}

	void KeyboardRemapComponent::update(Ogre::Real dt, bool notSimulating)
	{

	}

	void KeyboardRemapComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (KeyboardRemapComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (KeyboardRemapComponent::AttrRelativePosition() == attribute->getName())
		{
			this->setRelativePosition(attribute->getVector2());
		}
		else if (KeyboardRemapComponent::AttrParentId() == attribute->getName())
		{
			this->setParentId(attribute->getULong());
		}
		else if (KeyboardRemapComponent::AttrOkClickEventName() == attribute->getName())
		{
			this->setOkClickEventName(attribute->getString());
		}
		else if (KeyboardRemapComponent::AttrAbordClickEventName() == attribute->getName())
		{
			this->setAbordClickEventName(attribute->getString());
		}
	}

	void KeyboardRemapComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		GameObjectComponent::writeXML(propertiesXML, doc, filePath);

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
	}

	Ogre::String KeyboardRemapComponent::getClassName(void) const
	{
		return "KeyboardRemapComponent";
	}

	Ogre::String KeyboardRemapComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void KeyboardRemapComponent::setActivated(bool activated)
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

	bool KeyboardRemapComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void KeyboardRemapComponent::setRelativePosition(const Ogre::Vector2& relativePosition)
	{
		this->relativePosition->setValue(relativePosition);

		if (nullptr != this->widget)
		{
			// Change size
			this->widget->setRealPosition(relativePosition.x, relativePosition.y);
		}
	}

	Ogre::Vector2 KeyboardRemapComponent::getRelativePosition(void) const
	{
		return this->relativePosition->getVector2();
	}

	void KeyboardRemapComponent::setParentId(unsigned long parentId)
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

	unsigned long KeyboardRemapComponent::getParentId(void) const
	{
		return this->parentId->getULong();
	}

	void KeyboardRemapComponent::setOkClickEventName(const Ogre::String& okClickEventName)
	{
		this->okClickEventName->setValue(okClickEventName);
		this->okClickEventName->addUserData(GameObject::AttrActionGenerateLuaFunction(), okClickEventName + "(thisComponent)=" + this->getClassName());
	}

	Ogre::String KeyboardRemapComponent::getOkClickEventName(void) const
	{
		return this->okClickEventName->getString();
	}

	void KeyboardRemapComponent::setAbordClickEventName(const Ogre::String& abordClickEventName)
	{
		this->abordClickEventName->setValue(abordClickEventName);
		this->abordClickEventName->addUserData(GameObject::AttrActionGenerateLuaFunction(), abordClickEventName + "(thisComponent)=" + this->getClassName());
	}

	Ogre::String KeyboardRemapComponent::getAbordClickEventName(void) const
	{
		return this->abordClickEventName->getString();
	}

	MyGUI::Window* KeyboardRemapComponent::getWindow(void) const
	{
		return this->widget;
	}

	// Lua registration part

	KeyboardRemapComponent* getKeyboardRemapComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<KeyboardRemapComponent>(gameObject->getComponentWithOccurrence<KeyboardRemapComponent>(occurrenceIndex)).get();
	}

	KeyboardRemapComponent* getKeyboardRemapComponent(GameObject* gameObject)
	{
		return makeStrongPtr<KeyboardRemapComponent>(gameObject->getComponent<KeyboardRemapComponent>()).get();
	}

	KeyboardRemapComponent* getKeyboardRemapComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<KeyboardRemapComponent>(gameObject->getComponentFromName<KeyboardRemapComponent>(name)).get();
	}

	void KeyboardRemapComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<KeyboardRemapComponent, GameObjectComponent>("KeyboardRemapComponent")
			.def("setActivated", &KeyboardRemapComponent::setActivated)
			.def("isActivated", &KeyboardRemapComponent::isActivated)
		];

		LuaScriptApi::getInstance()->addClassToCollection("KeyboardRemapComponent", "class inherits GameObjectComponent", KeyboardRemapComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("KeyboardRemapComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not.");
		LuaScriptApi::getInstance()->addClassToCollection("KeyboardRemapComponent", "bool isActivated()", "Gets whether this component is activated.");

		gameObjectClass.def("getKeyboardRemapComponentFromName", &getKeyboardRemapComponentFromName);
		gameObjectClass.def("getKeyboardRemapComponent", (KeyboardRemapComponent * (*)(GameObject*)) & getKeyboardRemapComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "KeyboardRemapComponent getKeyboardRemapComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "KeyboardRemapComponent getKeyboardRemapComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castKeyboardRemapComponent", &GameObjectController::cast<KeyboardRemapComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "KeyboardRemapComponent castKeyboardRemapComponent(KeyboardRemapComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	void KeyboardRemapComponent::createMyGuiWidgets(void)
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

			auto keyCode = InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedKey(static_cast<InputDeviceModule::Action>(i));
			Ogre::String strKeyCode = InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getStringFromMappedKey(keyCode);
			this->oldKeyValue[i] = strKeyCode;
			this->newKeyValue[i] = strKeyCode;
			this->keyConfigTextboxes[i]->setNeedMouseFocus(true);
			this->keyConfigTextboxes[i]->setCaptionWithReplacing(strKeyCode);

			posY += marginY;
			totalHeight += posY + marginY + sizeY;
			marginY = 0.2f;
		}

		// scrollView->setCanvasSize((posX + marginX + sizeX + editSizeX) * scrollView->getClientCoord().width, totalHeight * scrollView->getClientCoord().height);

		this->messageLabel = scrollView->createWidgetReal<MyGUI::EditBox>("TextBox", posX  + marginX + marginX + sizeX + editSizeX, posY + marginY, sizeX, sizeY, MyGUI::Align::Left, "Popup");
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
		this->abordButton->setRealPosition(0.5f + 0.21f , 0.91f);
		this->abordButton->setAlign(MyGUI::Align::Left);
		this->abordButton->setCaptionWithReplacing("#{Abord}");
	}

	void KeyboardRemapComponent::destroyMyGUIWidgets(void)
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

	void KeyboardRemapComponent::notifyMouseSetFocus(MyGUI::Widget* sender, MyGUI::Widget* old)
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

	void KeyboardRemapComponent::buttonHit(MyGUI::Widget* sender)
	{
		if ("okButton" == sender->getName())
		{
			// Reset mappings, but not all, because else camera etc. cannot be moved anymore
			InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->clearKeyMapping(this->keyConfigTextboxes.size());

			for (unsigned short i = 0; i < this->keyConfigTextboxes.size(); i++)
			{
				this->oldKeyValue[i] = this->keyConfigTextboxes[i]->getCaption();
				this->textboxActive[i] = false;

				OIS::KeyCode key = InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedKeyFromString(this->newKeyValue[i]);
				InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->remapKey(static_cast<InputDeviceModule::Action>(i), key);
			}

			if (nullptr != this->gameObjectPtr->getLuaScript() && false == this->okClickEventName->getString().empty())
			{
				this->gameObjectPtr->getLuaScript()->callTableFunction(this->okClickEventName->getString(), this);
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
				this->gameObjectPtr->getLuaScript()->callTableFunction(this->abordClickEventName->getString(), this);
			}
		}
	}

	void KeyboardRemapComponent::keyPressed(MyGUI::Widget* sender, MyGUI::KeyCode key, MyGUI::Char ch)
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
				strKeyCode = InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getStringFromMappedKey(static_cast<OIS::KeyCode>(key.getValue()));
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

	bool KeyboardRemapComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// Constraints: Can only be placed under a main game object
		if (gameObject->getId() == GameObjectController::MAIN_GAMEOBJECT_ID)
		{
			return true;
		}
		return false;
	}

}; //namespace end
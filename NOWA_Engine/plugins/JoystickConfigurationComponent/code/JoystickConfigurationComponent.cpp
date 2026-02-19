#include "NOWAPrecompiled.h"
#include "JoystickConfigurationComponent.h"
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

	JoystickConfigurationComponent::JoystickConfigurationComponent()
		: GameObjectComponent(),
		name("JoystickConfigurationComponent"),
		hasParent(false),
		widget(nullptr),
		messageLabel(nullptr),
		okButton(nullptr),
		abordButton(nullptr),
		lastButton(InputDeviceModule::BUTTON_NONE),
		activated(new Variant(JoystickConfigurationComponent::AttrActivated(), true, this->attributes)),
		relativePosition(new Variant(JoystickConfigurationComponent::AttrRelativePosition(), Ogre::Vector2(0.325f, 0.325f), this->attributes)),
		parentId(new Variant(JoystickConfigurationComponent::AttrParentId(), static_cast<unsigned long>(0), this->attributes, true)),
		okClickEventName(new Variant(JoystickConfigurationComponent::AttrOkClickEventName(), Ogre::String(""), this->attributes)),
		abordClickEventName(new Variant(JoystickConfigurationComponent::AttrAbordClickEventName(), Ogre::String(""), this->attributes))
	{
		this->activated->setDescription("Shows the configuration menu if activated.");
	}

	JoystickConfigurationComponent::~JoystickConfigurationComponent(void)
	{
		if (nullptr != this->widget && false == this->hasParent)
		{
			MyGUI::Gui::getInstancePtr()->destroyWidget(this->widget);
			this->widget = nullptr;
		}

		// Core::getSingletonPtr()->getJoyStick()->setEventCallback(nullptr);
	}

	void JoystickConfigurationComponent::initialise()
	{

	}

	const Ogre::String& JoystickConfigurationComponent::getName() const
	{
		return this->name;
	}

	void JoystickConfigurationComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<JoystickConfigurationComponent>(JoystickConfigurationComponent::getStaticClassId(), JoystickConfigurationComponent::getStaticClassName());
	}

	void JoystickConfigurationComponent::shutdown()
	{

	}

	void JoystickConfigurationComponent::uninstall()
	{

	}

	void JoystickConfigurationComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool JoystickConfigurationComponent::init(rapidxml::xml_node<>*& propertyElement)
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
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Button_" + Ogre::StringConverter::toString(i))
			{
				// Store the mappings in a temp container, because at this time the game object is not available yet
				this->keyCodes.emplace(std::make_pair(i, Ogre::StringConverter::parseInt(propertyElement->first_attribute("data")->value())));
			}
			i++;
			propertyElement = propertyElement->next_sibling("property");
		}
		
		return true;
	}

	GameObjectCompPtr JoystickConfigurationComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		// Cloning does not make sense
		return nullptr;
	}

	bool JoystickConfigurationComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JoystickConfigurationComponent] Init component for game object: " + this->gameObjectPtr->getName());

		// If a listener has been added via key/mouse/joystick pressed, a new listener would be inserted during this iteration, which would cause a crash in mouse/key/button release iterator, hence add in next frame
		NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.25f));
		auto ptrFunction = [this]()
		{
			// Enables reaction on joy stick events
			InputDeviceCore::getSingletonPtr()->addJoystickListener(this, JoystickConfigurationComponent::getStaticClassName());
		};
		NOWA::ProcessPtr closureProcess(new NOWA::ClosureProcess(ptrFunction));
		delayProcess->attachChild(closureProcess);
		NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);

		this->setActivated(this->activated->getBool());

		const auto& inputDeviceModule = InputDeviceCore::getSingletonPtr()->getJoystickInputDeviceModule(this->gameObjectPtr->getId());

		if (nullptr != inputDeviceModule)
		{
			for (auto& it = this->keyCodes.cbegin(); it != this->keyCodes.cend(); ++it)
			{
				InputDeviceModule::JoyStickButton button = static_cast<InputDeviceModule::JoyStickButton>(it->second);
				if (InputDeviceModule::JoyStickButton::BUTTON_NONE != button)
				{
					inputDeviceModule->remapButton(static_cast<InputDeviceModule::Action>(it->first), button);
				}
			}
		}

		return true;
	}

	bool JoystickConfigurationComponent::connect(void)
	{
		GameObjectComponent::connect();

		// Sets the event handler
		if (nullptr != this->widget)
		{
			this->okButton->eventMouseButtonClick += MyGUI::newDelegate(this, &JoystickConfigurationComponent::buttonHit);
			this->abordButton->eventMouseButtonClick += MyGUI::newDelegate(this, &JoystickConfigurationComponent::buttonHit);
			for (unsigned short i = 0; i < keyConfigTextboxes.size(); i++)
			{
// TODO: Attention: Later check if second joystick available and this is the second component, to configure the second joystick!
				auto keyCode = InputDeviceCore::getSingletonPtr()->getJoystickInputDeviceModule(this->gameObjectPtr->getId())->getMappedButton(static_cast<InputDeviceModule::Action>(i));
				Ogre::String strKeyCode = InputDeviceCore::getSingletonPtr()->getJoystickInputDeviceModule(this->gameObjectPtr->getId())->getStringFromMappedButton(keyCode);
				this->oldKeyValue[i] = strKeyCode;
				this->newKeyValue[i] = strKeyCode;
				// Ogre::String strKeyCode = NOWA::Core::getSingletonPtr()->getKeyboard()->getAsString(keyCode);
				this->keyConfigTextboxes[i]->setNeedMouseFocus(true);
				this->keyConfigTextboxes[i]->setCaptionWithReplacing(strKeyCode);
				this->keyConfigTextboxes[i]->eventMouseSetFocus += MyGUI::newDelegate(this, &JoystickConfigurationComponent::notifyMouseSetFocus);
			}
		}
		
		return true;
	}

	bool JoystickConfigurationComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();

		if (nullptr != this->okButton)
		{
			this->okButton->eventMouseButtonClick -= MyGUI::newDelegate(this, &JoystickConfigurationComponent::buttonHit);
			this->abordButton->eventMouseButtonClick -= MyGUI::newDelegate(this, &JoystickConfigurationComponent::buttonHit);
		}

		for (unsigned short i = 0; i < keyConfigTextboxes.size(); i++)
		{
			this->keyConfigTextboxes[i]->eventMouseSetFocus -= MyGUI::newDelegate(this, &JoystickConfigurationComponent::notifyMouseSetFocus);
		}
		
		return true;
	}

	bool JoystickConfigurationComponent::onCloned(void)
	{
		
		return true;
	}

	void JoystickConfigurationComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		InputDeviceCore::getSingletonPtr()->removeJoystickListener(JoystickConfigurationComponent::getStaticClassName());

		this->destroyMyGUIWidgets();
	}

	void JoystickConfigurationComponent::update(Ogre::Real dt, bool notSimulating)
	{
		InputDeviceModule::JoyStickButton button = InputDeviceCore::getSingletonPtr()->getJoystickInputDeviceModule(this->gameObjectPtr->getId())->getPressedButton();
	}

	void JoystickConfigurationComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (JoystickConfigurationComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (JoystickConfigurationComponent::AttrRelativePosition() == attribute->getName())
		{
			this->setRelativePosition(attribute->getVector2());
		}
		else if (JoystickConfigurationComponent::AttrParentId() == attribute->getName())
		{
			this->setParentId(attribute->getULong());
		}
		else if (JoystickConfigurationComponent::AttrOkClickEventName() == attribute->getName())
		{
			this->setOkClickEventName(attribute->getString());
		}
		else if (JoystickConfigurationComponent::AttrAbordClickEventName() == attribute->getName())
		{
			this->setAbordClickEventName(attribute->getString());
		}
	}

	void JoystickConfigurationComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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

		const auto& inputDeviceModule = InputDeviceCore::getSingletonPtr()->getJoystickInputDeviceModule(this->gameObjectPtr->getId());

		if (nullptr != inputDeviceModule)
		{
			for (unsigned short i = 0; i < inputDeviceModule->getButtonMappingCount(); i++)
			{
				propertyXML = doc.allocate_node(node_element, "property");
				propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
				propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "Button_" + Ogre::StringConverter::toString(i))));

				Ogre::String mappedButtonCode = Ogre::StringConverter::toString(inputDeviceModule->getMappedButton(static_cast<InputDeviceModule::Action>(i)));

				propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, mappedButtonCode)));
				propertiesXML->append_node(propertyXML);
			}
		}
	}

	Ogre::String JoystickConfigurationComponent::getClassName(void) const
	{
		return "JoystickConfigurationComponent";
	}

	Ogre::String JoystickConfigurationComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void JoystickConfigurationComponent::setActivated(bool activated)
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

	bool JoystickConfigurationComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void JoystickConfigurationComponent::setRelativePosition(const Ogre::Vector2& relativePosition)
	{
		this->relativePosition->setValue(relativePosition);

		if (nullptr != this->widget)
		{
			// Change size
			this->widget->setRealPosition(relativePosition.x, relativePosition.y);
		}
	}

	Ogre::Vector2 JoystickConfigurationComponent::getRelativePosition(void) const
	{
		return this->relativePosition->getVector2();
	}

	void JoystickConfigurationComponent::setParentId(unsigned long parentId)
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

	unsigned long JoystickConfigurationComponent::getParentId(void) const
	{
		return this->parentId->getULong();
	}

	void JoystickConfigurationComponent::setOkClickEventName(const Ogre::String& okClickEventName)
	{
		this->okClickEventName->setValue(okClickEventName);
		this->okClickEventName->addUserData(GameObject::AttrActionGenerateLuaFunction(), okClickEventName + "(thisComponent)=" + this->getClassName());
	}

	Ogre::String JoystickConfigurationComponent::getOkClickEventName(void) const
	{
		return this->okClickEventName->getString();
	}

	void JoystickConfigurationComponent::setAbordClickEventName(const Ogre::String& abordClickEventName)
	{
		this->abordClickEventName->setValue(abordClickEventName);
		this->abordClickEventName->addUserData(GameObject::AttrActionGenerateLuaFunction(), abordClickEventName + "(thisComponent)=" + this->getClassName());
	}

	Ogre::String JoystickConfigurationComponent::getAbordClickEventName(void) const
	{
		return this->abordClickEventName->getString();
	}

	MyGUI::Window* JoystickConfigurationComponent::getWindow(void) const
	{
		return this->widget;
	}

	bool JoystickConfigurationComponent::axisMoved(const OIS::JoyStickEvent& evt, int axis)
	{
		if (false == this->bConnected)
		{
			return false;
		}

		// Check if an editbox is active and set the pressed key to the edit box for key-mapping
		const unsigned short count = 7;
		bool keepMappingActive = false;
		bool alreadyExisting = false;

		InputDeviceCore::getSingletonPtr()->getJoystickInputDeviceModule(this->gameObjectPtr->getId())->update(0.016f);
		InputDeviceModule::JoyStickButton button = InputDeviceCore::getSingletonPtr()->getJoystickInputDeviceModule(this->gameObjectPtr->getId())->getPressedButton();

		if (/*button == this->lastButton ||*/ button == InputDeviceModule::JoyStickButton::BUTTON_NONE)
		{
			return true;
		}

		this->lastButton = button;

		short index = -1;
		Ogre::String strKeyCode;
		for (unsigned short i = 0; i < count; i++)
		{
			if (true == this->textboxActive[i])
			{
				index = i;
				// this->oldKeyValue[i] = this->keyConfigTextboxes[i]->getCaption();
				// get key string and set the text
				strKeyCode = InputDeviceCore::getSingletonPtr()->getJoystickInputDeviceModule(this->gameObjectPtr->getId())->getStringFromMappedButton(button);
				this->textboxActive[i] = false;
				this->keyConfigTextboxes[i]->setTextShadow(false);
				keepMappingActive = true;
			}
		}

		// Check if the key does not exist already
		for (unsigned short i = 0; i < count; i++)
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

		return true;
	}

	bool JoystickConfigurationComponent::buttonPressed(const OIS::JoyStickEvent& evt, int button)
	{
		const unsigned short count = 7;

		if (false == this->bConnected)
		{
			return false;
		}

		// Check if an editbox is active and set the pressed key to the edit box for key-mapping
		bool keepMappingActive = false;
		bool alreadyExisting = false;

		short index = -1;
		Ogre::String strKeyCode;
		for (unsigned short i = 0; i < count; i++)
		{
			if (true == this->textboxActive[i])
			{
				index = i;
				// this->oldKeyValue[i] = this->keyConfigTextboxes[i]->getCaption();
				// get key string and set the text
				strKeyCode = InputDeviceCore::getSingletonPtr()->getJoystickInputDeviceModule(this->gameObjectPtr->getId())->getStringFromMappedButton(static_cast<InputDeviceModule::JoyStickButton>(button));
				this->textboxActive[i] = false;
				this->keyConfigTextboxes[i]->setTextShadow(false);
				keepMappingActive = true;
			}
		}

		// Check if the key does not exist already
		for (unsigned short i = 0; i < count; i++)
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

		return true;
	}

	bool JoystickConfigurationComponent::buttonReleased(const OIS::JoyStickEvent& evt, int button)
	{
		return false;
	}

	bool JoystickConfigurationComponent::sliderMoved(const OIS::JoyStickEvent& evt, int index)
	{
		return false;
	}

	bool JoystickConfigurationComponent::povMoved(const OIS::JoyStickEvent& evt, int pov)
	{
		return false;
	}

	bool JoystickConfigurationComponent::vector3Moved(const OIS::JoyStickEvent& evt, int index)
	{
		return false;
	}

	void JoystickConfigurationComponent::createMyGuiWidgets(void)
	{
		// Layers: "Wallpaper", "ToolTip", "Info", "FadeMiddle", "Popup", "Main", "Modal", "Middle", "Overlapped", "Back", "DragAndDrop", "FadeBusy", "Pointer", "Fade", "Statistic"
		this->widget = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::Window>("Window", this->relativePosition->getVector2().x, this->relativePosition->getVector2().y, 0.25f, 0.25f, MyGUI::Align::Center, "Popup");
		this->widget->setCaptionWithReplacing("#{Joystick_Configuration}");

		this->setRelativePosition(this->relativePosition->getVector2());

		// Attach maybe widget to parent
		this->setParentId(this->parentId->getULong());

		const unsigned short count = 7;
		const Ogre::Real sizeX = 0.165f;
		const Ogre::Real sizeY = 0.02f;
		const Ogre::Real marginX = 0.02f;
		const Ogre::Real marginY = 0.1f;

		Ogre::Real posX = 0.001f;
		Ogre::Real posY = 0.0f;

		this->keyConfigLabels.resize(count);
		this->keyConfigTextboxes.resize(count);
		this->textboxActive.resize(count, false);
		this->oldKeyValue.resize(count, "");
		this->newKeyValue.resize(count, "");

		for (unsigned short i = 0; i < count; i++)
		{
			this->keyConfigLabels[i] = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::EditBox>("TextBox", posX + marginX, posY + sizeY, sizeX, sizeY, MyGUI::Align::Left, "Popup");
			this->keyConfigLabels[i]->attachToWidget(this->widget);
			this->keyConfigLabels[i]->setRealPosition(posX + marginX, posY + sizeY + marginY);

			this->keyConfigTextboxes[i] = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::EditBox>("EditBox", posX + sizeX + 2 * marginX, posY + sizeY, sizeX, sizeY, MyGUI::Align::Left, "Popup");
			this->keyConfigTextboxes[i]->attachToWidget(this->widget);
			this->keyConfigTextboxes[i]->setRealPosition(posX + sizeX + 2 * marginX, posY + sizeY + marginY);
			this->keyConfigTextboxes[i]->setEditReadOnly(true);

			auto keyCode = InputDeviceCore::getSingletonPtr()->getJoystickInputDeviceModule(this->gameObjectPtr->getId())->getMappedButton(static_cast<InputDeviceModule::Action>(i));
			Ogre::String strKeyCode = InputDeviceCore::getSingletonPtr()->getJoystickInputDeviceModule(this->gameObjectPtr->getId())->getStringFromMappedButton(keyCode);
			this->oldKeyValue[i] = strKeyCode;
			this->newKeyValue[i] = strKeyCode;
			this->keyConfigTextboxes[i]->setNeedMouseFocus(true);
			this->keyConfigTextboxes[i]->setCaptionWithReplacing(strKeyCode);

			posY += marginY;
		}

		this->messageLabel = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::EditBox>("TextBox", posX + marginX, posY + sizeY, sizeX, sizeY, MyGUI::Align::Left, "Popup");
		this->messageLabel->attachToWidget(this->widget);
		this->messageLabel->setRealPosition(posX + marginX, posY + sizeY + marginY);
		this->messageLabel->setCaptionWithReplacing("#{Key_Existing}");
		this->messageLabel->setTextColour(MyGUI::Colour::Red);
		this->messageLabel->setVisible(false);

		unsigned short i = 0;
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Move_Up}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Move_Down}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Move_Left}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Move_Right}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Jump}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Action_1}:");
		keyConfigLabels[i++]->setCaptionWithReplacing("#{Action_2}:");

		this->okButton = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::Button>("Button", 0.0f, 0.0f, 0.1f, 0.02f, MyGUI::Align::Left, "Popup", "okButton");
		this->okButton->attachToWidget(this->widget);
		this->okButton->setRealPosition(marginX, 0.91f);
		this->okButton->setAlign(MyGUI::Align::Left);
		this->okButton->setCaption("Ok");

		this->abordButton = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::Button>("Button", 0.0f, 0.0f, 0.1f, 0.02f, MyGUI::Align::Left, "Popup", "abordButton");
		this->abordButton->attachToWidget(this->widget);
		this->abordButton->setRealPosition(marginX * 2 + 0.5f, 0.91f);
		this->abordButton->setAlign(MyGUI::Align::Left);
		this->abordButton->setCaptionWithReplacing("#{Abord}");
	}

	void JoystickConfigurationComponent::destroyMyGUIWidgets(void)
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

	void JoystickConfigurationComponent::notifyMouseSetFocus(MyGUI::Widget* sender, MyGUI::Widget* old)
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

	void JoystickConfigurationComponent::buttonHit(MyGUI::Widget* sender)
	{
		if ("okButton" == sender->getName())
		{
			// Reset mappings, but not all, because else camera etc. cannot be moved anymore
			InputDeviceCore::getSingletonPtr()->getJoystickInputDeviceModule(this->gameObjectPtr->getId())->clearButtonMapping(this->keyConfigTextboxes.size());

			for (unsigned short i = 0; i < this->keyConfigTextboxes.size(); i++)
			{
				this->oldKeyValue[i] = this->keyConfigTextboxes[i]->getCaption();
				this->textboxActive[i] = false;

				InputDeviceModule::JoyStickButton button = InputDeviceCore::getSingletonPtr()->getJoystickInputDeviceModule(this->gameObjectPtr->getId())->getMappedButtonFromString(this->newKeyValue[i]);
				InputDeviceCore::getSingletonPtr()->getJoystickInputDeviceModule(this->gameObjectPtr->getId())->remapButton(static_cast<InputDeviceModule::Action>(i), button);
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

	bool JoystickConfigurationComponent::canStaticAddComponent(GameObject* gameObject)
	{
		if (nullptr == InputDeviceCore::getSingletonPtr()->getJoystickInputDeviceModule(gameObject->getId()))
		{
			return false;
		}

		if (1 == gameObject->getComponentCount("InputDeviceComponent") && gameObject->getComponentCount<JoystickConfigurationComponent>() < 2)
		{
			return true;
		}
		return false;
	}

	// Lua registration part

	JoystickConfigurationComponent* getJoystickConfigurationComponentFromIndex(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<JoystickConfigurationComponent>(gameObject->getComponentWithOccurrence<JoystickConfigurationComponent>(occurrenceIndex)).get();
	}

	JoystickConfigurationComponent* getJoystickConfigurationComponent(GameObject* gameObject)
	{
		return makeStrongPtr<JoystickConfigurationComponent>(gameObject->getComponent<JoystickConfigurationComponent>()).get();
	}

	JoystickConfigurationComponent* getJoystickConfigurationComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<JoystickConfigurationComponent>(gameObject->getComponentFromName<JoystickConfigurationComponent>(name)).get();
	}

	void JoystickConfigurationComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<JoystickConfigurationComponent, GameObjectComponent>("JoystickConfigurationComponent")
			.def("setActivated", &JoystickConfigurationComponent::setActivated)
			.def("isActivated", &JoystickConfigurationComponent::isActivated)
		];

		LuaScriptApi::getInstance()->addClassToCollection("JoystickConfigurationComponent", "class inherits GameObjectComponent", JoystickConfigurationComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("JoystickConfigurationComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not.");
		LuaScriptApi::getInstance()->addClassToCollection("JoystickConfigurationComponent", "bool isActivated()", "Gets whether this component is activated.");

		gameObjectClass.def("getJoystickConfigurationComponentFromName", getJoystickConfigurationComponentFromName);
		gameObjectClass.def("getJoystickConfigurationComponent", (JoystickConfigurationComponent * (*)(GameObject*)) & getJoystickConfigurationComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "JoystickConfigurationComponent getJoystickConfigurationComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "JoystickConfigurationComponent getJoystickConfigurationComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castJoystickConfigurationComponent", &GameObjectController::cast<JoystickConfigurationComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "JoystickConfigurationComponent castJoystickConfigurationComponent(JoystickConfigurationComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

}; //namespace end
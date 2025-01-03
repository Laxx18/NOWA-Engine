#include "NOWAPrecompiled.h"
#include "JoystickRemapComponent.h"
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

	JoystickRemapComponent::JoystickRemapComponent()
		: GameObjectComponent(),
		name("JoystickRemapComponent"),
		hasParent(false),
		widget(nullptr),
		messageLabel(nullptr),
		okButton(nullptr),
		abordButton(nullptr),
		lastButton(InputDeviceModule::BUTTON_NONE),
		bIsInSimulation(false),
		activated(new Variant(JoystickRemapComponent::AttrActivated(), true, this->attributes)),
		deviceName(new Variant(JoystickRemapComponent::AttrDeviceName(), Ogre::String(""), this->attributes)),
		relativePosition(new Variant(JoystickRemapComponent::AttrRelativePosition(), Ogre::Vector2(0.325f, 0.325f), this->attributes)),
		parentId(new Variant(JoystickRemapComponent::AttrParentId(), static_cast<unsigned long>(0), this->attributes, true)),
		okClickEventName(new Variant(JoystickRemapComponent::AttrOkClickEventName(), Ogre::String(""), this->attributes)),
		abordClickEventName(new Variant(JoystickRemapComponent::AttrAbordClickEventName(), Ogre::String(""), this->attributes))
	{
		this->activated->setDescription("Shows the remap menu if activated.");
	}

	JoystickRemapComponent::~JoystickRemapComponent(void)
	{
		if (nullptr != this->widget && false == this->hasParent)
		{
			MyGUI::Gui::getInstancePtr()->destroyWidget(this->widget);
			this->widget = nullptr;
		}

		// Core::getSingletonPtr()->getJoyStick()->setEventCallback(nullptr);
	}

	void JoystickRemapComponent::initialise()
	{

	}

	const Ogre::String& JoystickRemapComponent::getName() const
	{
		return this->name;
	}

	void JoystickRemapComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<JoystickRemapComponent>(JoystickRemapComponent::getStaticClassId(), JoystickRemapComponent::getStaticClassName());
	}

	void JoystickRemapComponent::shutdown()
	{

	}

	void JoystickRemapComponent::uninstall()
	{

	}

	void JoystickRemapComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool JoystickRemapComponent::init(rapidxml::xml_node<>*& propertyElement)
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
		
		return true;
	}

	GameObjectCompPtr JoystickRemapComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		// Cloning does not make sense
		return nullptr;
	}

	bool JoystickRemapComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JoystickRemapComponent] Init component for game object: " + this->gameObjectPtr->getName());

		// If a listener has been added via key/mouse/joystick pressed, a new listener would be inserted during this iteration, which would cause a crash in mouse/key/button release iterator, hence add in next frame
		NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.25f));
		auto ptrFunction = [this]()
		{
			// Enables reaction on joy stick events
			InputDeviceCore::getSingletonPtr()->addJoystickListener(this, JoystickRemapComponent::getStaticClassName());
		};
		NOWA::ProcessPtr closureProcess(new NOWA::ClosureProcess(ptrFunction));
		delayProcess->attachChild(closureProcess);
		NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);

		this->setActivated(this->activated->getBool());

		this->deviceName->setReadOnly(false);
		Ogre::String tempDeviceName = InputDeviceCore::getSingletonPtr()->getJoystick(this->occurrenceIndex)->vendor();
		this->deviceName->setValue(tempDeviceName);
		this->deviceName->setUserData(GameObject::AttrActionNeedRefresh());
		this->deviceName->setReadOnly(true);

		return true;
	}

	bool JoystickRemapComponent::connect(void)
	{
		GameObjectComponent::connect();

		// Sets the event handler
		if (nullptr != this->widget)
		{
			this->okButton->eventMouseButtonClick += MyGUI::newDelegate(this, &JoystickRemapComponent::buttonHit);
			this->abordButton->eventMouseButtonClick += MyGUI::newDelegate(this, &JoystickRemapComponent::buttonHit);
			for (unsigned short i = 0; i < keyConfigTextboxes.size(); i++)
			{
// TODO: Attention: Later check if second joystick available and this is the second component, to configure the second joystick!
				auto keyCode = InputDeviceCore::getSingletonPtr()->getInputDeviceModule(this->occurrenceIndex)->getMappedButton(static_cast<InputDeviceModule::Action>(i));
				Ogre::String strKeyCode = InputDeviceCore::getSingletonPtr()->getInputDeviceModule(this->occurrenceIndex)->getStringFromMappedButton(keyCode);
				this->oldKeyValue[i] = strKeyCode;
				this->newKeyValue[i] = strKeyCode;
				// Ogre::String strKeyCode = NOWA::Core::getSingletonPtr()->getKeyboard()->getAsString(keyCode);
				this->keyConfigTextboxes[i]->setNeedMouseFocus(true);
				this->keyConfigTextboxes[i]->setCaptionWithReplacing(strKeyCode);
				this->keyConfigTextboxes[i]->eventMouseSetFocus += MyGUI::newDelegate(this, &JoystickRemapComponent::notifyMouseSetFocus);
			}
		}
		
		return true;
	}

	bool JoystickRemapComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();

		if (nullptr != this->okButton)
		{
			this->okButton->eventMouseButtonClick -= MyGUI::newDelegate(this, &JoystickRemapComponent::buttonHit);
			this->abordButton->eventMouseButtonClick -= MyGUI::newDelegate(this, &JoystickRemapComponent::buttonHit);
		}

		for (unsigned short i = 0; i < keyConfigTextboxes.size(); i++)
		{
			this->keyConfigTextboxes[i]->eventMouseSetFocus -= MyGUI::newDelegate(this, &JoystickRemapComponent::notifyMouseSetFocus);
		}
		
		return true;
	}

	bool JoystickRemapComponent::onCloned(void)
	{
		
		return true;
	}

	void JoystickRemapComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		InputDeviceCore::getSingletonPtr()->removeJoystickListener(JoystickRemapComponent::getStaticClassName());

		if (nullptr != this->widget)
		{
			this->widget->detachFromWidget();
		}
		this->hasParent = false;
	}

	void JoystickRemapComponent::update(Ogre::Real dt, bool notSimulating)
	{
		this->bIsInSimulation = !notSimulating;

		InputDeviceModule::JoyStickButton button = InputDeviceCore::getSingletonPtr()->getInputDeviceModule(this->occurrenceIndex)->getPressedButton();
	}

	void JoystickRemapComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (JoystickRemapComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (JoystickRemapComponent::AttrRelativePosition() == attribute->getName())
		{
			this->setRelativePosition(attribute->getVector2());
		}
		else if (JoystickRemapComponent::AttrParentId() == attribute->getName())
		{
			this->setParentId(attribute->getULong());
		}
		else if (JoystickRemapComponent::AttrOkClickEventName() == attribute->getName())
		{
			this->setOkClickEventName(attribute->getString());
		}
		else if (JoystickRemapComponent::AttrAbordClickEventName() == attribute->getName())
		{
			this->setAbordClickEventName(attribute->getString());
		}
	}

	void JoystickRemapComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
	}

	Ogre::String JoystickRemapComponent::getClassName(void) const
	{
		return "JoystickRemapComponent";
	}

	Ogre::String JoystickRemapComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void JoystickRemapComponent::setActivated(bool activated)
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

	bool JoystickRemapComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void JoystickRemapComponent::setRelativePosition(const Ogre::Vector2& relativePosition)
	{
		this->relativePosition->setValue(relativePosition);

		if (nullptr != this->widget)
		{
			// Change size
			this->widget->setRealPosition(relativePosition.x, relativePosition.y);
		}
	}

	Ogre::Vector2 JoystickRemapComponent::getRelativePosition(void) const
	{
		return this->relativePosition->getVector2();
	}

	void JoystickRemapComponent::setParentId(unsigned long parentId)
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

	unsigned long JoystickRemapComponent::getParentId(void) const
	{
		return this->parentId->getULong();
	}

	void JoystickRemapComponent::setOkClickEventName(const Ogre::String& okClickEventName)
	{
		this->okClickEventName->setValue(okClickEventName);
		this->okClickEventName->addUserData(GameObject::AttrActionGenerateLuaFunction(), okClickEventName + "(thisComponent)=" + this->getClassName());
	}

	Ogre::String JoystickRemapComponent::getOkClickEventName(void) const
	{
		return this->okClickEventName->getString();
	}

	void JoystickRemapComponent::setAbordClickEventName(const Ogre::String& abordClickEventName)
	{
		this->abordClickEventName->setValue(abordClickEventName);
		this->abordClickEventName->addUserData(GameObject::AttrActionGenerateLuaFunction(), abordClickEventName + "(thisComponent)=" + this->getClassName());
	}

	Ogre::String JoystickRemapComponent::getAbordClickEventName(void) const
	{
		return this->abordClickEventName->getString();
	}

	MyGUI::Window* JoystickRemapComponent::getWindow(void) const
	{
		return this->widget;
	}

	bool JoystickRemapComponent::axisMoved(const OIS::JoyStickEvent& evt, int axis)
	{
		if (false == this->bIsInSimulation)
		{
			return false;
		}

		// Check if an editbox is active and set the pressed key to the edit box for key-mapping
		const unsigned short count = 7;
		bool keepMappingActive = false;
		bool alreadyExisting = false;

		InputDeviceCore::getSingletonPtr()->getInputDeviceModule(this->occurrenceIndex)->update(0.016f);
		InputDeviceModule::JoyStickButton button = InputDeviceCore::getSingletonPtr()->getInputDeviceModule(this->occurrenceIndex)->getPressedButton();

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
				strKeyCode = InputDeviceCore::getSingletonPtr()->getInputDeviceModule(this->occurrenceIndex)->getStringFromMappedButton(button);
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

	bool JoystickRemapComponent::buttonPressed(const OIS::JoyStickEvent& evt, int button)
	{
		const unsigned short count = 7;

		if (false == this->bIsInSimulation)
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
				strKeyCode = InputDeviceCore::getSingletonPtr()->getInputDeviceModule(this->occurrenceIndex)->getStringFromMappedButton(static_cast<InputDeviceModule::JoyStickButton>(button));
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

	bool JoystickRemapComponent::buttonReleased(const OIS::JoyStickEvent& evt, int button)
	{
		return false;
	}

	bool JoystickRemapComponent::sliderMoved(const OIS::JoyStickEvent& evt, int index)
	{
		return false;
	}

	bool JoystickRemapComponent::povMoved(const OIS::JoyStickEvent& evt, int pov)
	{
		return false;
	}

	bool JoystickRemapComponent::vector3Moved(const OIS::JoyStickEvent& evt, int index)
	{
		return false;
	}

	// Lua registration part

	JoystickRemapComponent* getJoystickRemapComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<JoystickRemapComponent>(gameObject->getComponentWithOccurrence<JoystickRemapComponent>(occurrenceIndex)).get();
	}

	JoystickRemapComponent* getJoystickRemapComponent(GameObject* gameObject)
	{
		return makeStrongPtr<JoystickRemapComponent>(gameObject->getComponent<JoystickRemapComponent>()).get();
	}

	JoystickRemapComponent* getJoystickRemapComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<JoystickRemapComponent>(gameObject->getComponentFromName<JoystickRemapComponent>(name)).get();
	}

	void JoystickRemapComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<JoystickRemapComponent, GameObjectComponent>("JoystickRemapComponent")
			.def("setActivated", &JoystickRemapComponent::setActivated)
			.def("isActivated", &JoystickRemapComponent::isActivated)
		];

		LuaScriptApi::getInstance()->addClassToCollection("JoystickRemapComponent", "class inherits GameObjectComponent", JoystickRemapComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("JoystickRemapComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not.");
		LuaScriptApi::getInstance()->addClassToCollection("JoystickRemapComponent", "bool isActivated()", "Gets whether this component is activated.");

		gameObjectClass.def("getJoystickRemapComponentFromName", &getJoystickRemapComponentFromName);
		gameObjectClass.def("getJoystickRemapComponent", (JoystickRemapComponent * (*)(GameObject*)) & getJoystickRemapComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "JoystickRemapComponent getJoystickRemapComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "JoystickRemapComponent getJoystickRemapComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castJoystickRemapComponent", &GameObjectController::cast<JoystickRemapComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "JoystickRemapComponent castJoystickRemapComponent(JoystickRemapComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	Ogre::String JoystickRemapComponent::getDeviceName(void) const
	{
		return this->deviceName->getString();
	}

	void JoystickRemapComponent::createMyGuiWidgets(void)
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
			this->keyConfigLabels[i] = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::EditBox>("TextBox", posX  + marginX, posY + sizeY, sizeX, sizeY, MyGUI::Align::Left, "Popup");
			this->keyConfigLabels[i]->attachToWidget(this->widget);
			this->keyConfigLabels[i]->setRealPosition(posX  + marginX, posY + sizeY + marginY);

			this->keyConfigTextboxes[i] = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::EditBox>("EditBox", posX + sizeX + 2 * marginX, posY + sizeY, sizeX, sizeY, MyGUI::Align::Left, "Popup");
			this->keyConfigTextboxes[i]->attachToWidget(this->widget);
			this->keyConfigTextboxes[i]->setRealPosition(posX + sizeX + 2 * marginX, posY + sizeY + marginY);
			this->keyConfigTextboxes[i]->setEditReadOnly(true);

			auto keyCode = InputDeviceCore::getSingletonPtr()->getInputDeviceModule(this->occurrenceIndex)->getMappedButton(static_cast<InputDeviceModule::Action>(i));
			Ogre::String strKeyCode = InputDeviceCore::getSingletonPtr()->getInputDeviceModule(this->occurrenceIndex)->getStringFromMappedButton(keyCode);
			this->oldKeyValue[i] = strKeyCode;
			this->newKeyValue[i] = strKeyCode;
			this->keyConfigTextboxes[i]->setNeedMouseFocus(true);
			this->keyConfigTextboxes[i]->setCaptionWithReplacing(strKeyCode);

			posY += marginY;
		}

		this->messageLabel = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::EditBox>("TextBox", posX  + marginX, posY + sizeY, sizeX, sizeY, MyGUI::Align::Left, "Popup");
		this->messageLabel->attachToWidget(this->widget);
		this->messageLabel->setRealPosition(posX  + marginX, posY + sizeY + marginY);
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

	void JoystickRemapComponent::destroyMyGUIWidgets(void)
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

	void JoystickRemapComponent::notifyMouseSetFocus(MyGUI::Widget* sender, MyGUI::Widget* old)
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

	void JoystickRemapComponent::buttonHit(MyGUI::Widget* sender)
	{
		if ("okButton" == sender->getName())
		{
			// Reset mappings, but not all, because else camera etc. cannot be moved anymore
			InputDeviceCore::getSingletonPtr()->getInputDeviceModule(this->occurrenceIndex)->clearButtonMapping(this->keyConfigTextboxes.size());

			for (unsigned short i = 0; i < this->keyConfigTextboxes.size(); i++)
			{
				this->oldKeyValue[i] = this->keyConfigTextboxes[i]->getCaption();
				this->textboxActive[i] = false;

				InputDeviceModule::JoyStickButton button = InputDeviceCore::getSingletonPtr()->getInputDeviceModule(0)->getMappedButtonFromString(this->newKeyValue[i]);
				InputDeviceCore::getSingletonPtr()->getInputDeviceModule(this->occurrenceIndex)->remapButton(static_cast<InputDeviceModule::Action>(i), button);
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

	bool JoystickRemapComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// Constraints: Can only be placed under a main game object and no more than 2 of this component may exist
		if (gameObject->getId() == GameObjectController::MAIN_GAMEOBJECT_ID && gameObject->getComponentCount<JoystickRemapComponent>() < 2)
		{
			return true;
		}
		return false;
	}

}; //namespace end
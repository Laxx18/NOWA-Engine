#include "NOWAPrecompiled.h"
#include "PickerComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "camera/CameraManager.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "main/InputDeviceCore.h"
#include "gameobject/GameObjectFactory.h"

#include "OgreAbiUtils.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	PickerComponent::PickerComponent()
		: GameObjectComponent(),
		name("PickerComponent"),
		isInSimulation(false),
		picker(nullptr),
		mouseButtonId(OIS::MouseButtonID::MB_Left),
		joystickButtonId(-1),
		mouseIdPressed(false),
		joystickIdPressed(false),
		jointId(0),
		body(nullptr),
		draggingStartedFirstTime(true),
		draggingEndedFirstTime(true),
		activated(new Variant(PickerComponent::AttrActivated(), true, this->attributes)),
		targetId(new Variant(PickerComponent::AttrTargetId(), static_cast<unsigned long>(0), this->attributes, true)),
		offsetPosition(new Variant(PickerComponent::AttrOffsetPosition(), Ogre::Vector3::ZERO, this->attributes)),
		springStrength(new Variant(PickerComponent::AttrSpringStrength(), 20.0f, this->attributes)),
		dragAffectDistance(new Variant(PickerComponent::AttrDragAffectDistance(), 100.0f, this->attributes)),
		drawLine(new Variant(PickerComponent::AttrDrawLine(), true, this->attributes)),
		mouseButtonPickId(new Variant(PickerComponent::AttrMouseButtonPickId(), std::vector<Ogre::String>(), this->attributes)),
		joystickButtonPickId(new Variant(PickerComponent::AttrJoystickButtonPickId(), std::vector<Ogre::String>(), this->attributes))
	{
		this->activated->setDescription("Picking will only take place if this component is activated.");
		this->targetId->setDescription("Sets the target game object id to be picked.");
		this->offsetPosition->setDescription("Sets the offset position at which the target game object shall be picked.");
		this->springStrength->setDescription("Sets the pick spring strength. Default value is 20 N. Max is 49 N.");
		this->dragAffectDistance->setDescription("Sets the drag affect distance in meters at which the target game object is affected by the picker.");
		this->drawLine->setDescription("Sets whether to draw a picking line.");
		this->mouseButtonPickId->setDescription("Sets the mouse pick id.");
		this->joystickButtonPickId->setDescription("Sets the joystick pick id.");
	}

	PickerComponent::~PickerComponent(void)
	{
		
	}

	const Ogre::String& PickerComponent::getName() const
	{
		return this->name;
	}

	void PickerComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<PickerComponent>(PickerComponent::getStaticClassId(), PickerComponent::getStaticClassName());
	}
	
	void PickerComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool PickerComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &PickerComponent::deleteJointDelegate), EventDataDeleteJoint::getStaticEventType());
		AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &PickerComponent::deleteBodyDelegate), EventDataDeleteBody::getStaticEventType());

		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetId")
		{
			this->targetId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OffsetPosition")
		{
			this->offsetPosition->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SpringStrength")
		{
			this->springStrength->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DragAffectDistance")
		{
			this->dragAffectDistance->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DrawLine")
		{
			this->drawLine->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MouseButtonPickId")
		{
			this->setMouseButtonPickId(XMLConverter::getAttrib(propertyElement, "data", ""));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JoystickButtonPickId")
		{
			this->joystickButtonPickId->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data", ""));
			propertyElement = propertyElement->next_sibling("property");
		}

		return true;
	}

	GameObjectCompPtr PickerComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		return nullptr;
	}

	bool PickerComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PickerComponent] Init component for game object: " + this->gameObjectPtr->getName());

		// If a listener has been added via key/mouse/joystick pressed, a new listener would be inserted during this iteration, which would cause a crash in mouse/key/button release iterator, hence add in next frame
		NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(0.25f));
		auto ptrFunction = [this]()
		{
			Ogre::String listenerName = PickerComponent::getStaticClassName() + Ogre::StringConverter::toString(this->getIndex()) + "_" + this->gameObjectPtr->getName();
			InputDeviceCore::getSingletonPtr()->addMouseListener(this, listenerName);
			InputDeviceCore::getSingletonPtr()->addJoystickListener(this, listenerName);
		};
		NOWA::ProcessPtr closureProcess(new NOWA::ClosureProcess(ptrFunction));
		delayProcess->attachChild(closureProcess);
		NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);

		this->mouseButtonPickId->setValue({"LEFT_MOUSE_BUTTON", "RIGHT_MOUSE_BUTTON", "MIDDLE_MOUSE_BUTTON"});

		std::vector<Ogre::String> joystickButtons;
		auto buttons = InputDeviceCore::getSingletonPtr()->getInputDeviceModule()->getAllButtonStrings();
		for (const auto& button : buttons)
		{
			joystickButtons.emplace_back(button);
		}
		this->joystickButtonPickId->setValue(joystickButtons);

		this->picker = new GameObjectPicker();
		this->picker->init(this->gameObjectPtr->getSceneManager(), NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera(), 100, this->targetId->getULong());

		return true;
	}

	bool PickerComponent::connect(void)
	{
		this->isInSimulation = true;

		if (0 != this->targetId->getULong())
		{
			this->picker->actualizeData(NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera(), this->targetId->getULong(), this->drawLine->getBool());
		}
		else if (0 != this->jointId)
		{
			this->picker->actualizeData2(NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera(), this->jointId, this->drawLine->getBool());
		}
		else if (nullptr != this->body)
		{
			this->picker->actualizeData3(NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera(), this->body, this->drawLine->getBool());
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PickerComponent] Could not attach picker, because either the target id or the joint id or the direct body pointer is invalid for game object: " + this->gameObjectPtr->getName());
		}
		
		return true;
	}

	bool PickerComponent::disconnect(void)
	{
		this->isInSimulation = false;
		this->draggingStartedFirstTime = true;
		this->draggingEndedFirstTime = true;
		return true;
	}

	bool PickerComponent::onCloned(void)
	{
		
		return true;
	}

	void PickerComponent::deleteJointDelegate(EventDataPtr eventData)
	{
		boost::shared_ptr<EventDataDeleteJoint> castEventData = boost::static_pointer_cast<EventDataDeleteJoint>(eventData);

		unsigned long id = castEventData->getJointId();
		if (id == this->jointId)
		{
			this->jointId = 0;
			this->picker->actualizeData2(NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera(), this->jointId, this->drawLine->getBool());
			this->picker->release();
		}
	}

	void PickerComponent::deleteBodyDelegate(EventDataPtr eventData)
	{
		boost::shared_ptr<EventDataDeleteBody> castEventData = boost::static_pointer_cast<EventDataDeleteBody>(eventData);

		if (castEventData->getBody() == this->body)
		{
			if (nullptr != this->picker)
			{
				this->picker->release();
			}
		}
	}

	void PickerComponent::onRemoveComponent(void)
	{
		AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &PickerComponent::deleteJointDelegate), EventDataDeleteJoint::getStaticEventType());
		AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &PickerComponent::deleteBodyDelegate), EventDataDeleteBody::getStaticEventType());

		GameObjectComponent::onRemoveComponent();

		if (this->picker != nullptr)
		{
			delete this->picker;
			this->picker = nullptr;
		}

		InputDeviceCore::getSingletonPtr()->removeMouseListener(this);
		InputDeviceCore::getSingletonPtr()->removeJoystickListener(this);
	}
	
	void PickerComponent::onOtherComponentRemoved(unsigned int index)
	{
		
	}
	
	void PickerComponent::onOtherComponentAdded(unsigned int index)
	{
		
	}
	
	void PickerComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			// Do something
		}
	}

	void PickerComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (PickerComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (PickerComponent::AttrTargetId() == attribute->getName())
		{
			this->setTargetId(attribute->getULong());
		}
		else if (PickerComponent::AttrOffsetPosition() == attribute->getName())
		{
			this->setOffsetPosition(attribute->getVector3());
		}
		else if (PickerComponent::AttrSpringStrength() == attribute->getName())
		{
			this->setSpringStrength(attribute->getReal());
		}
		else if (PickerComponent::AttrDragAffectDistance() == attribute->getName())
		{
			this->setDragAffectDistance(attribute->getReal());
		}
		else if (PickerComponent::AttrDrawLine() == attribute->getName())
		{
			this->setDrawLine(attribute->getBool());
		}
		else if (PickerComponent::AttrMouseButtonPickId() == attribute->getName())
		{
			this->setMouseButtonPickId(attribute->getListSelectedValue());
		}
		else if (PickerComponent::AttrJoystickButtonPickId() == attribute->getName())
		{
			this->setJoystickButtonPickId(attribute->getListSelectedValue());
		}
	}

	void PickerComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TargetId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->targetId->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OffsetPosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->offsetPosition->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SpringStrength"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->springStrength->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DragAffectDistance"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->dragAffectDistance->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DrawLine"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->drawLine->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MouseButtonPickId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->mouseButtonPickId->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JoystickButtonPickId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->joystickButtonPickId->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String PickerComponent::getClassName(void) const
	{
		return "PickerComponent";
	}

	Ogre::String PickerComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void PickerComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
		if (false == this->activated->getBool())
		{
			if (nullptr != this->picker)
			{
				this->picker->release();
			}
		}
	}

	bool PickerComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void PickerComponent::setTargetId(unsigned long targetId)
	{
		this->targetId->setValue(targetId);
	}

	unsigned long PickerComponent::getTargetId(void) const
	{
		return this->targetId->getULong();
	}

	void PickerComponent::setTargetJointId(unsigned long jointId)
	{
		this->jointId = jointId;
		if (0 != jointId)
		{
			this->picker->actualizeData2(NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera(), this->jointId, this->drawLine->getBool());
		}
	}

	void PickerComponent::setTargetBody(OgreNewt::Body* body)
	{
		this->body = body;
		this->picker->actualizeData3(NOWA::AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera(), this->body, this->drawLine->getBool());
	}

	void PickerComponent::setOffsetPosition(const Ogre::Vector3& offsetPosition)
	{
		this->offsetPosition->setValue(offsetPosition);
	}

	Ogre::Vector3 PickerComponent::getOffsetPosition(void) const
	{
		return this->offsetPosition->getVector3();
	}

	void PickerComponent::setSpringStrength(Ogre::Real springStrength)
	{
		// 50 will escalate
		if (springStrength > 49.0f)
		{
			springStrength = 49.0f;
		}
		else if (springStrength < 1.0f)
		{
			springStrength = 1.0f;
		}
		this->springStrength->setValue(springStrength);
	}

	Ogre::Real PickerComponent::getSpringStrength(void) const
	{
		return this->springStrength->getReal();
	}

	void PickerComponent::setDragAffectDistance(Ogre::Real dragAffectDistance)
	{
		if (dragAffectDistance < 1.0f)
		{
			dragAffectDistance = 1.0f;
		}
		this->dragAffectDistance->setValue(dragAffectDistance);
	}

	Ogre::Real PickerComponent::getDragAffectDistance(void) const
	{
		return this->dragAffectDistance->getReal();
	}

	void PickerComponent::setDrawLine(bool drawLine)
	{
		this->drawLine->setValue(drawLine);
	}

	bool PickerComponent::getDrawLine(void) const
	{
		return this->drawLine->getBool();
	}

	void PickerComponent::setMouseButtonPickId(const Ogre::String& mouseButtonPickId)
	{
		this->mouseButtonPickId->setListSelectedValue(mouseButtonPickId);
		if ("LEFT_MOUSE_BUTTON" == mouseButtonPickId)
		{
			this->mouseButtonId = OIS::MB_Left;
		}
		else if ("RIGHT_MOUSE_BUTTON" == mouseButtonPickId)
		{
			this->mouseButtonId = OIS::MB_Right;
		}
		else if ("MIDDLE_MOUSE_BUTTON" == mouseButtonPickId)
		{
			this->mouseButtonId = OIS::MB_Middle;
		}
	}

	Ogre::String PickerComponent::getMouseButtonPickId(void) const
	{
		return this->mouseButtonPickId->getListSelectedValue();
	}

	void PickerComponent::setJoystickButtonPickId(const Ogre::String& joystickButtonPickId)
	{
		this->joystickButtonPickId->setListSelectedValue(joystickButtonPickId);

		this->joystickButtonId = InputDeviceCore::getSingletonPtr()->getInputDeviceModule()->getMappedButtonFromString(joystickButtonPickId);
	}

	Ogre::String PickerComponent::getJoystickButtonPickId(void) const
	{
		return this->joystickButtonPickId->getListSelectedValue();
	}

	void PickerComponent::reactOnDraggingStart(luabind::object closureFunction)
	{
		this->startClosureFunction = closureFunction;
	}

	void PickerComponent::reactOnDraggingEnd(luabind::object closureFunction)
	{
		this->endClosureFunction = closureFunction;
	}

	bool PickerComponent::mousePressed(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
	{
		if (this->mouseButtonId == id && true == this->isInSimulation)
		{
			this->mouseIdPressed = true;
		}

		return true;
	}

	bool PickerComponent::mouseReleased(const OIS::MouseEvent& evt, OIS::MouseButtonID id)
	{
		this->mouseIdPressed = false;
		this->draggingStartedFirstTime = true;
		this->draggingEndedFirstTime = true;
		this->picker->release();

		if (this->endClosureFunction.is_valid())
		{
			try
			{
				luabind::call_function<void>(this->endClosureFunction);
			}
			catch (luabind::error& error)
			{
				luabind::object errorMsg(luabind::from_stack(error.state(), -1));
				std::stringstream msg;
				msg << errorMsg;

				Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnDraggingEnd' Error: " + Ogre::String(error.what())
															+ " details: " + msg.str());
			}
		}
		return true;
	}

	bool PickerComponent::mouseMoved(const OIS::MouseEvent& evt)
	{
		if (true == this->mouseIdPressed && true == this->isInSimulation && true == this->activated->getBool())
		{
			this->picker->grabGameObject(AppStateManager::getSingletonPtr()->getOgreNewtModule()->getOgreNewt(),
										 Ogre::Vector2(static_cast<Ogre::Real>(evt.state.X.abs), static_cast<Ogre::Real>(evt.state.Y.abs)),
										 Core::getSingletonPtr()->getOgreRenderWindow(), this->springStrength->getReal(), this->dragAffectDistance->getReal());
		}
		// Call also function in lua script, if it does exist in the lua script component
		if (nullptr != this->gameObjectPtr->getLuaScript())
		{
			if (true == this->draggingStartedFirstTime && true == this->picker->getIsDragging())
			{
				if (this->startClosureFunction.is_valid())
				{
					try
					{
						luabind::call_function<void>(this->startClosureFunction);
					}
					catch (luabind::error& error)
					{
						luabind::object errorMsg(luabind::from_stack(error.state(), -1));
						std::stringstream msg;
						msg << errorMsg;

						Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnDraggingStart' Error: " + Ogre::String(error.what())
																	+ " details: " + msg.str());
					}
				}
				this->draggingStartedFirstTime = false;
			}
			else if (true == this->draggingEndedFirstTime && false == this->picker->getIsDragging())
			{
				if (this->endClosureFunction.is_valid())
				{
					try
					{
						luabind::call_function<void>(this->endClosureFunction);
					}
					catch (luabind::error& error)
					{
						luabind::object errorMsg(luabind::from_stack(error.state(), -1));
						std::stringstream msg;
						msg << errorMsg;

						Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnDraggingEnd' Error: " + Ogre::String(error.what())
																	+ " details: " + msg.str());
					}
				}
				this->draggingEndedFirstTime = false;
			}
		}
		return true;
	}

	bool PickerComponent::buttonPressed(const OIS::JoyStickEvent& evt, int button)
	{
		if (this->joystickButtonId == button && true == this->isInSimulation && true == this->activated->getBool())
		{
			this->joystickIdPressed = true;
		}
		return true;
	}

	bool PickerComponent::buttonReleased(const OIS::JoyStickEvent& evt, int button)
	{
		this->joystickIdPressed = false;
		this->draggingStartedFirstTime = true;
		this->draggingEndedFirstTime = true;
		this->picker->release();

		if (this->endClosureFunction.is_valid())
		{
			try
			{
				luabind::call_function<void>(this->endClosureFunction);
			}
			catch (luabind::error& error)
			{
				luabind::object errorMsg(luabind::from_stack(error.state(), -1));
				std::stringstream msg;
				msg << errorMsg;

				Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnDraggingEnd' Error: " + Ogre::String(error.what())
															+ " details: " + msg.str());
			}
		}

		return true;
	}

	bool PickerComponent::axisMoved(const OIS::JoyStickEvent& evt, int axis)
	{
		if (true == this->joystickIdPressed && true == this->isInSimulation && true == this->activated->getBool())
		{
			this->picker->grabGameObject(AppStateManager::getSingletonPtr()->getOgreNewtModule()->getOgreNewt(),
										 Ogre::Vector2(static_cast<Ogre::Real>(evt.state.mAxes[0].abs), static_cast<Ogre::Real>(evt.state.mAxes[1].abs)),
										 Core::getSingletonPtr()->getOgreRenderWindow());
		}

		// Call also function in lua script, if it does exist in the lua script component
		if (nullptr != this->gameObjectPtr->getLuaScript())
		{
			if (true == this->draggingStartedFirstTime && true == this->picker->getIsDragging())
			{
				if (this->startClosureFunction.is_valid())
				{
					try
					{
						luabind::call_function<void>(this->startClosureFunction);
					}
					catch (luabind::error& error)
					{
						luabind::object errorMsg(luabind::from_stack(error.state(), -1));
						std::stringstream msg;
						msg << errorMsg;

						Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnDraggingStart' Error: " + Ogre::String(error.what())
																	+ " details: " + msg.str());
					}
				}
				this->draggingStartedFirstTime = false;
			}
			else if (true == this->draggingEndedFirstTime && false == this->picker->getIsDragging())
			{
				if (this->endClosureFunction.is_valid())
				{
					try
					{
						luabind::call_function<void>(this->endClosureFunction);
					}
					catch (luabind::error& error)
					{
						luabind::object errorMsg(luabind::from_stack(error.state(), -1));
						std::stringstream msg;
						msg << errorMsg;

						Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnDraggingEnd' Error: " + Ogre::String(error.what())
																	+ " details: " + msg.str());
					}
				}
				this->draggingEndedFirstTime = false;
			}
		}
		return true;
	}

	// Lua registration part

	PickerComponent* getPickerComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<PickerComponent>(gameObject->getComponentWithOccurrence<PickerComponent>(occurrenceIndex)).get();
	}

	PickerComponent* getPickerComponent(GameObject* gameObject)
	{
		return makeStrongPtr<PickerComponent>(gameObject->getComponent<PickerComponent>()).get();
	}

	PickerComponent* getPickerComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<PickerComponent>(gameObject->getComponentFromName<PickerComponent>(name)).get();
	}

	void setTargetId(PickerComponent* instance, const Ogre::String& targetId)
	{
		instance->setTargetId(Ogre::StringConverter::parseUnsignedLong(targetId));
	}

	Ogre::String getTargetId(PickerComponent* instance)
	{
		return Ogre::StringConverter::toString(instance->getTargetId());
	}

	void setTargetJointId(PickerComponent* instance, const Ogre::String& targetJointId)
	{
		instance->setTargetJointId(Ogre::StringConverter::parseUnsignedLong(targetJointId));
	}

	void PickerComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<PickerComponent, GameObjectComponent>("PickerComponent")
			.def("setActivated", &PickerComponent::setActivated)
			.def("isActivated", &PickerComponent::isActivated)
			.def("setTargetId", &setTargetId)
			.def("getTargetId", &getTargetId)
			.def("setTargetJointId", &setTargetJointId)
			.def("setTargetBody", &PickerComponent::setTargetBody)
			.def("setOffsetPosition", &PickerComponent::setOffsetPosition)
			.def("getOffsetPosition", &PickerComponent::getOffsetPosition)
			.def("setSpringStrength", &PickerComponent::setSpringStrength)
			.def("getSpringStrength", &PickerComponent::getSpringStrength)
			.def("setDragAffectDistance", &PickerComponent::setDragAffectDistance)
			.def("getDragAffectDistance", &PickerComponent::getDragAffectDistance)
			.def("setDrawLine", &PickerComponent::setDrawLine)
			.def("getDrawLine", &PickerComponent::getDrawLine)
			.def("setMouseButtonPickId", &PickerComponent::setMouseButtonPickId)
			.def("getMouseButtonPickId", &PickerComponent::getMouseButtonPickId)
			.def("setJoystickButtonPickId", &PickerComponent::setJoystickButtonPickId)
			.def("getJoystickButtonPickId", &PickerComponent::getJoystickButtonPickId)
			.def("reactOnDraggingStart", &PickerComponent::reactOnDraggingStart)
			.def("reactOnDraggingEnd", &PickerComponent::reactOnDraggingEnd)
		];

		LuaScriptApi::getInstance()->addClassToCollection("PickerComponent", "class inherits GameObjectComponent", PickerComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("PickerComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not.");
		LuaScriptApi::getInstance()->addClassToCollection("PickerComponent", "bool isActivated()", "Gets whether this component is activated.");
		LuaScriptApi::getInstance()->addClassToCollection("PickerComponent", "void setTargetId(string targetId)", "Sets the target game object id to pick.");
		LuaScriptApi::getInstance()->addClassToCollection("PickerComponent", "string getTargetId()", "Gets the target game object id which is currently picked.");

		LuaScriptApi::getInstance()->addClassToCollection("PickerComponent", "void setTargetJointId(string targetJointId)", "Sets instead a joint id to get the physics body for dragging. Note: Useful if a ragdoll with joint rag bones is involved.");
		LuaScriptApi::getInstance()->addClassToCollection("PickerComponent", "void setTargetBody(Body body)", "Sets the physics body pointer for dragging. Note: Useful if a ragdoll with joint rag bones is involved.");

		LuaScriptApi::getInstance()->addClassToCollection("PickerComponent", "void setOffsetPosition(Vector3 offsetPosition)", "Sets an offset position at which the source game object should be picked.");
		LuaScriptApi::getInstance()->addClassToCollection("PickerComponent", "Vector3 getOffsetPosition()", "Gets the offset position at which the source game object is picked.");

		LuaScriptApi::getInstance()->addClassToCollection("PickerComponent", "void setSpringStrength(float springStrength)", "Sets the pick strength (spring strength).");
		LuaScriptApi::getInstance()->addClassToCollection("PickerComponent", "float getSpringStrength()", "Gets the pick strength (spring strength).");

		LuaScriptApi::getInstance()->addClassToCollection("PickerComponent", "void setDragAffectDistance(float dragAffectDistance)", "Sets the drag affect distance in meters at which the target game object is affected by the picker.");
		LuaScriptApi::getInstance()->addClassToCollection("PickerComponent", "float getDragAffectDistance()", "Gets the drag affect distance in meters at which the target game object is affected by the picker.");

		LuaScriptApi::getInstance()->addClassToCollection("PickerComponent", "void setDrawLine(bool drawLine)", "Sets whether to draw the spring line.");
		LuaScriptApi::getInstance()->addClassToCollection("PickerComponent", "bool getDrawLine()", "Gets whether a spring line is drawn.");

		LuaScriptApi::getInstance()->addClassToCollection("PickerComponent", "void setMouseButtonPickId(string mouseButtonPickId)", "Sets the mouse button, which shall active the mouse picking.");
		LuaScriptApi::getInstance()->addClassToCollection("PickerComponent", "string getMouseButtonPickId()", "Gets the mouse button, which shall active the mouse picking.");

		LuaScriptApi::getInstance()->addClassToCollection("PickerComponent", "void setJoystickButtonPickId(string joystickButtonPickId)", "Sets the joystick button, which shall active the joystick picking.");
		LuaScriptApi::getInstance()->addClassToCollection("PickerComponent", "string getJoystickButtonPickId()", "Gets the joystick button, which shall active the joystick picking.");

		LuaScriptApi::getInstance()->addClassToCollection("PickerComponent", "void reactOnDraggingStart(func closure)",
														  "Sets whether to react at the moment when the dragging has started.");
		LuaScriptApi::getInstance()->addClassToCollection("PickerComponent", "void reactOnDraggingEnd(func closure)",
														  "Sets whether to react at the moment when the dragging has ended.");

		gameObjectClass.def("getPickerComponentFromName", &getPickerComponentFromName);
		gameObjectClass.def("getPickerComponent", (PickerComponent * (*)(GameObject*)) & getPickerComponent);
		// If its desired to create several of this components for one game object
		gameObjectClass.def("getPickerComponent2", (PickerComponent * (*)(GameObject*, unsigned int)) & getPickerComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PickerComponent getPickerComponent2(unsigned int occurrenceIndex)", "Gets the component by the given occurence index, since a game object may this component maybe several times.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PickerComponent getPickerComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PickerComponent getPickerComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castPickerComponent", &GameObjectController::cast<PickerComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "PickerComponent castPickerComponent(PickerComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool PickerComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// No constraints so far, just add
		return true;
	}

}; //namespace end
#include "NOWAPrecompiled.h"
#include "MyGUIControllerComponents.h"
#include "MyGUIComponents.h"
#include "GameObjectController.h"
#include "LuaScriptComponent.h"
#include "modules/LuaScriptApi.h"
#include "utilities/XMLConverter.h"
#include "MyGUI_ControllerRepeatClick.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	MyGUIControllerComponent::MyGUIControllerComponent()
		: GameObjectComponent(),
		controllerItem(nullptr),
		sourceWidget(nullptr),
		sourceMyGUIComponent(nullptr),
		isInSimulation(false),
		oldCoordinate(Ogre::Vector4::ZERO),
		activated(new Variant(MyGUIControllerComponent::AttrActivated(), true, this->attributes)),
		sourceId(new Variant(MyGUIControllerComponent::AttrSourceId(), static_cast<unsigned long>(0), this->attributes, true))
	{
		
	}

	MyGUIControllerComponent::~MyGUIControllerComponent()
	{
		
	}

	bool MyGUIControllerComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->setActivated(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SourceId")
		{
			this->sourceId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	bool MyGUIControllerComponent::postInit(void)
	{
		this->setActivated(this->activated->getBool());

		return true;
	}

	void MyGUIControllerComponent::update(Ogre::Real dt, bool notSimulating)
	{
		this->isInSimulation = !notSimulating;
	}

	bool MyGUIControllerComponent::connect(void)
	{
		if (nullptr != this->sourceWidget && nullptr != this->controllerItem)
		{
			// If the controller succeeded, it will be deleted internally!
			this->controllerItem = nullptr;
			MyGUI::ControllerManager::getInstance().removeItem(this->sourceWidget);
		}

		for (unsigned int i = 0; i < this->gameObjectPtr->getComponents()->size(); i++)
		{
			auto& gameObjectCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponentByIndex(i));
			if (nullptr != gameObjectCompPtr)
			{
				auto& myGuiCompPtr = boost::dynamic_pointer_cast<MyGUIComponent>(gameObjectCompPtr);
				if (nullptr != myGuiCompPtr && myGuiCompPtr->getId() == this->sourceId->getULong())
				{
					if (nullptr != myGuiCompPtr->getWidget())
					{
						this->sourceWidget = myGuiCompPtr->getWidget();
						this->sourceMyGUIComponent = myGuiCompPtr.get();
						break;
					}
				}
			}
		}

		return true;
	}

	bool MyGUIControllerComponent::disconnect(void)
	{
		
		return true;
	}
	
	void MyGUIControllerComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		if (nullptr != this->sourceWidget)
		{
			MyGUI::ControllerManager::getInstance().removeItem(this->sourceWidget);
			this->sourceWidget = nullptr;
			this->sourceMyGUIComponent = nullptr;
		}
	}
	
	void MyGUIControllerComponent::onOtherComponentRemoved(unsigned int index)
	{
		GameObjectComponent::onOtherComponentRemoved(index);
		
		auto& gameObjectCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponentByIndex(index));
		if (nullptr != gameObjectCompPtr)
		{
			auto& myGuiCompPtr = boost::dynamic_pointer_cast<MyGUIComponent>(gameObjectCompPtr);
			if (nullptr != myGuiCompPtr)
			{
				if (this->sourceWidget == myGuiCompPtr->getWidget())
				{
					MyGUI::ControllerManager::getInstance().removeItem(this->sourceWidget);
					this->sourceWidget = nullptr;
					this->sourceMyGUIComponent = nullptr;
				}
			}
		}
	}

	void MyGUIControllerComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (MyGUIControllerComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (MyGUIControllerComponent::AttrSourceId() == attribute->getName())
		{
			this->setSourceId(attribute->getULong());
		}
	}

	void MyGUIControllerComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "SourceId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->sourceId->getULong())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String MyGUIControllerComponent::getClassName(void) const
	{
		return "MyGUIControllerComponent";
	}

	Ogre::String MyGUIControllerComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void MyGUIControllerComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
		if (true == this->isInSimulation)
		{
			if (true == activated)
			{
				this->connect();
				this->activateController(activated);
			}
		}
		if (false == activated)
		{
			this->activateController(activated);
			this->disconnect();
		}
	}

	void MyGUIControllerComponent::activateController(bool bActivate)
	{
		if (nullptr != this->sourceWidget)
		{
			if (true == bActivate)
			{
				// Store old coordinate, because widget will be moved
				this->oldCoordinate = Ogre::Vector4(static_cast<Ogre::Real>(this->sourceWidget->getCoord().left), static_cast<Ogre::Real>(this->sourceWidget->getCoord().top),
													static_cast<Ogre::Real>(this->sourceWidget->getCoord().width), static_cast<Ogre::Real>(this->sourceWidget->getCoord().height));
			}
			else
			{
				MyGUI::IntCoord coord(static_cast<int>(this->oldCoordinate.x), static_cast<int>(this->oldCoordinate.y),
									  static_cast<int>(this->oldCoordinate.z), static_cast<int>(this->oldCoordinate.w));

				if (this->sourceWidget->getCoord() != coord)
				{
					// Set widget to old coordinate
					this->sourceWidget->setCoord(static_cast<int>(this->oldCoordinate.x), static_cast<int>(this->oldCoordinate.y),
												 static_cast<int>(this->oldCoordinate.z), static_cast<int>(this->oldCoordinate.w));

					MyGUI::FloatCoord oldRelativeCoord = MyGUI::CoordConverter::convertToRelative(MyGUI::IntCoord(this->oldCoordinate.x, this->oldCoordinate.y, this->oldCoordinate.z, this->oldCoordinate.w), MyGUI::RenderManager::getInstance().getViewSize());

					// Set widget to old coordinate
					this->sourceWidget->setRealPosition(oldRelativeCoord.left, oldRelativeCoord.top);
					this->sourceWidget->setRealSize(oldRelativeCoord.width, oldRelativeCoord.height);
				}
			}
		}
	}

	bool MyGUIControllerComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void MyGUIControllerComponent::setSourceId(unsigned long sourceId)
	{
		this->sourceId->setValue(sourceId);
	}

	unsigned long MyGUIControllerComponent::getSourceId(void) const
	{
		return this->sourceId->getULong();
	}
	
	MyGUI::ControllerItem* MyGUIControllerComponent::getControllerItem(void) const
	{
		return this->controllerItem;
	}
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MyGUIPositionControllerComponent::MyGUIPositionControllerComponent()
		: MyGUIControllerComponent(),
		coordinate(new Variant(MyGUIPositionControllerComponent::AttrCoordinate(), { Ogre::Vector4(0.1f, 0.5f, 0.0f, 0.0f) }, this->attributes)),
		function(new Variant(MyGUIPositionControllerComponent::AttrFunction(), { "Inertional", "Accelerated", "Slowed", "Jump" }, this->attributes)),
		durationSec(new Variant(MyGUIPositionControllerComponent::AttrDurationSec(), Ogre::Real(0.5f), this->attributes))
	{
		
	}

	MyGUIPositionControllerComponent::~MyGUIPositionControllerComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUIPositionControllerComponent] Destructor MyGUI position controller component for game object: " + this->gameObjectPtr->getName());
	}

	bool MyGUIPositionControllerComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = MyGUIControllerComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Coordinate")
		{
			this->coordinate->setValue(XMLConverter::getAttribVector4(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Function")
		{
			this->function->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Duration")
		{
			this->durationSec->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		return success;
	}

	GameObjectCompPtr MyGUIPositionControllerComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		return nullptr;
	}

	bool MyGUIPositionControllerComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUIPositionControllerComponent] Init MyGUI position controller component for game object: " + this->gameObjectPtr->getName());

		return MyGUIControllerComponent::postInit();
	}

	bool MyGUIPositionControllerComponent::connect(void)
	{
		bool success = MyGUIControllerComponent::connect();
		
		if (nullptr == this->controllerItem)
		{
			MyGUI::ControllerItem* item = MyGUI::ControllerManager::getInstance().createItem(MyGUI::ControllerPosition::getClassTypeName());
			this->controllerItem = item->castType<MyGUI::ControllerPosition>();

			this->controllerItem->eventPostAction += MyGUI::newDelegate(this, &MyGUIPositionControllerComponent::controllerFinished);
		}
		
		if (true == this->activated->getBool())
		{
			if (nullptr != this->sourceWidget)
			{
				MyGUI::ControllerManager::getInstance().addItem(this->sourceWidget, this->controllerItem);

				this->controllerItem->castType<MyGUI::ControllerPosition>()->setTime(this->durationSec->getReal());

				if (this->function->getListSelectedValue() == "Inertional")
					this->controllerItem->castType<MyGUI::ControllerPosition>()->setAction(MyGUI::newDelegate(MyGUI::action::inertionalMoveFunction));
				else if (this->function->getListSelectedValue() == "Accelerated")
					this->controllerItem->castType<MyGUI::ControllerPosition>()->setAction(MyGUI::newDelegate(MyGUI::action::acceleratedMoveFunction<30>));
				else if (this->function->getListSelectedValue() == "Slowed")
					this->controllerItem->castType<MyGUI::ControllerPosition>()->setAction(MyGUI::newDelegate(MyGUI::action::acceleratedMoveFunction<4>));
				else
					this->controllerItem->castType<MyGUI::ControllerPosition>()->setAction(MyGUI::newDelegate(MyGUI::action::jumpMoveFunction<5>));
		
				Ogre::Vector4 tempCoordinate = this->coordinate->getVector4();
				MyGUI::FloatSize relWidgetSize = MyGUI::CoordConverter::convertToRelative(this->sourceWidget->getSize(), MyGUI::RenderManager::getInstance().getViewSize());

				if (tempCoordinate.z == 0.0f)
					tempCoordinate.z = relWidgetSize.width;
				if (tempCoordinate.w == 0.0f)
					tempCoordinate.w = relWidgetSize.height;

				MyGUI::FloatCoord floatCoord(tempCoordinate.x, tempCoordinate.y, tempCoordinate.z, tempCoordinate.w);
				this->controllerItem->castType<MyGUI::ControllerPosition>()->setCoord(MyGUI::CoordConverter::convertFromRelative(floatCoord, MyGUI::RenderManager::getInstance().getViewSize()));
			}
		}
		return success;
	}

	bool MyGUIPositionControllerComponent::disconnect(void)
	{
		if (nullptr != this->sourceWidget)
		{
			MyGUI::ControllerManager::getInstance().removeItem(this->sourceWidget);
		}

		if (nullptr != this->controllerItem)
		{
			// If the controller succeeded, it will be deleted internally!
			this->controllerItem = nullptr;
		}
		
		return MyGUIControllerComponent::disconnect();
	}

	void MyGUIPositionControllerComponent::controllerFinished(MyGUI::Widget* sender, MyGUI::ControllerItem* controller)
	{
		// Bad, as the widget will get its init position and not the target one
		// this->disconnect();
	}

	void MyGUIPositionControllerComponent::actualizeValue(Variant* attribute)
	{
		MyGUIControllerComponent::actualizeValue(attribute);
		
		if (MyGUIPositionControllerComponent::AttrCoordinate() == attribute->getName())
		{
			this->setCoordinate(attribute->getVector4());
		}
		else if (MyGUIPositionControllerComponent::AttrFunction() == attribute->getName())
		{
			this->setFunction(attribute->getListSelectedValue());
		}
		else if (MyGUIPositionControllerComponent::AttrDurationSec() == attribute->getName())
		{
			this->setDurationSec(attribute->getReal());
		}
	}

	void MyGUIPositionControllerComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		MyGUIControllerComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Coordinate"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->coordinate->getVector4())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Function"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->function->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Duration"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->durationSec->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String MyGUIPositionControllerComponent::getClassName(void) const
	{
		return "MyGUIPositionControllerComponent";
	}

	Ogre::String MyGUIPositionControllerComponent::getParentClassName(void) const
	{
		return "MyGUIControllerComponent";
	}
	
	void MyGUIPositionControllerComponent::setCoordinate(const Ogre::Vector4& coordinate)
	{
		this->coordinate->setValue(coordinate);
	}
	
	Ogre::Vector4 MyGUIPositionControllerComponent::getCoordinate(void) const
	{
		return this->coordinate->getVector4();
	}
	
	void MyGUIPositionControllerComponent::setFunction(const Ogre::String& function)
	{
		this->function->setListSelectedValue(function);
	}
	
	Ogre::String MyGUIPositionControllerComponent::getFunction(void) const
	{
		return this->function->getListSelectedValue();
	}
	
	void MyGUIPositionControllerComponent::setDurationSec(Ogre::Real durationSec)
	{
		this->durationSec->setValue(durationSec);
	}
	
	Ogre::Real MyGUIPositionControllerComponent::getDurationSec(void) const
	{
		return this->durationSec->getReal();
	}
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	MyGUIFadeAlphaControllerComponent::MyGUIFadeAlphaControllerComponent()
		: MyGUIControllerComponent(),
		oldAlpha(1.0f),
		alpha(new Variant(MyGUIFadeAlphaControllerComponent::AttrAlpha(), Ogre::Real(1.0f), this->attributes)),
		coefficient(new Variant(MyGUIFadeAlphaControllerComponent::AttrCoefficient(), Ogre::Real(1.0f), this->attributes))
	{
		
	}

	MyGUIFadeAlphaControllerComponent::~MyGUIFadeAlphaControllerComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUIFadeAlphaControllerComponent] Destructor MyGUI fade alpha controller component for game object: " + this->gameObjectPtr->getName());
	}

	bool MyGUIFadeAlphaControllerComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = MyGUIControllerComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Alpha")
		{
			this->alpha->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Coefficient")
		{
			this->coefficient->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		return success;
	}

	GameObjectCompPtr MyGUIFadeAlphaControllerComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		return nullptr;
	}

	bool MyGUIFadeAlphaControllerComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUIFadeAlphaControllerComponent] Init MyGUI fade alpha controller component for game object: " + this->gameObjectPtr->getName());
		
		return MyGUIControllerComponent::postInit();
	}

	bool MyGUIFadeAlphaControllerComponent::connect(void)
	{
		bool success = MyGUIControllerComponent::connect();

		if (nullptr == this->controllerItem)
		{
			MyGUI::ControllerItem* item = MyGUI::ControllerManager::getInstance().createItem(MyGUI::ControllerFadeAlpha::getClassTypeName());
			this->controllerItem = item->castType<MyGUI::ControllerFadeAlpha>();

			this->controllerItem->eventPostAction += MyGUI::newDelegate(this, &MyGUIFadeAlphaControllerComponent::controllerFinished);
		}

		if (true == this->activated->getBool())
		{
			if (nullptr != this->sourceWidget)
			{
				this->activated->setValue(activated);

				MyGUI::ControllerManager::getInstance().removeItem(this->sourceWidget);
				if (nullptr == this->controllerItem)
				{
					MyGUI::ControllerItem* item = MyGUI::ControllerManager::getInstance().createItem(MyGUI::ControllerFadeAlpha::getClassTypeName());
					this->controllerItem = item->castType<MyGUI::ControllerFadeAlpha>();

					this->controllerItem->eventPostAction += MyGUI::newDelegate(this, &MyGUIFadeAlphaControllerComponent::controllerFinished);
				}

				// Store old alpha, because alpha will be changed
				this->oldAlpha = this->sourceWidget->getAlpha();

				this->controllerItem->castType<MyGUI::ControllerFadeAlpha>()->setAlpha(this->alpha->getReal());
				this->controllerItem->castType<MyGUI::ControllerFadeAlpha>()->setCoef(this->coefficient->getReal());

				MyGUI::ControllerManager::getInstance().addItem(this->sourceWidget, this->controllerItem);
			}
		}
		return success;
	}

	bool MyGUIFadeAlphaControllerComponent::disconnect(void)
	{
		if (nullptr != this->controllerItem)
		{
			// If the controller succeeded, it will be deleted internally!
			this->controllerItem = nullptr;
		}

		if (nullptr != this->sourceWidget)
		{
			MyGUI::ControllerManager::getInstance().removeItem(this->sourceWidget);
			// Set widget to old alpha
			this->sourceWidget->setAlpha(this->oldAlpha);
		}

		return MyGUIControllerComponent::disconnect();
	}

	void MyGUIFadeAlphaControllerComponent::controllerFinished(MyGUI::Widget* sender, MyGUI::ControllerItem* controller)
	{
		// this->disconnect();
	}

	void MyGUIFadeAlphaControllerComponent::actualizeValue(Variant* attribute)
	{
		MyGUIControllerComponent::actualizeValue(attribute);
		
		if (MyGUIFadeAlphaControllerComponent::AttrAlpha() == attribute->getName())
		{
			this->setAlpha(attribute->getReal());
		}
		else if (MyGUIFadeAlphaControllerComponent::AttrCoefficient() == attribute->getName())
		{
			this->setCoefficient(attribute->getReal());
		}
	}

	void MyGUIFadeAlphaControllerComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		MyGUIControllerComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Alpha"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->alpha->getVector4())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Coefficient"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->coefficient->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String MyGUIFadeAlphaControllerComponent::getClassName(void) const
	{
		return "MyGUIFadeAlphaControllerComponent";
	}

	Ogre::String MyGUIFadeAlphaControllerComponent::getParentClassName(void) const
	{
		return "MyGUIControllerComponent";
	}
	
	void MyGUIFadeAlphaControllerComponent::setAlpha(Ogre::Real alpha)
	{
		this->alpha->setValue(alpha);
	}
		
	Ogre::Real MyGUIFadeAlphaControllerComponent::getAlpha(void) const
	{
		return this->alpha->getReal();
	}
		
	void MyGUIFadeAlphaControllerComponent::setCoefficient(Ogre::Real coefficient)
	{
		this->coefficient->setValue(coefficient);
	}
		
	Ogre::Real MyGUIFadeAlphaControllerComponent::getCoefficient(void) const
	{
		return this->coefficient->getReal();
	}
		
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MyGUIScrollingMessageControllerComponent::MyGUIScrollingMessageControllerComponent()
		: MyGUIControllerComponent(),
		timeToGo(0.0f),
		justAdded(false),
		editBoxes({nullptr, nullptr }),
		messageCount(new Variant(MyGUIScrollingMessageControllerComponent::AttrMessageCount(), static_cast<unsigned int>(0), this->attributes)),
		durationSec(new Variant(MyGUIScrollingMessageControllerComponent::AttrDurationSec(), Ogre::Real(0.5f), this->attributes))
	{
		this->messageCount->addUserData(GameObject::AttrActionNeedRefresh());
	}

	MyGUIScrollingMessageControllerComponent::~MyGUIScrollingMessageControllerComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUIScrollingMessageControllerComponent] Destructor MyGUI scrolling message controller component for game object: " + this->gameObjectPtr->getName());
	}

	bool MyGUIScrollingMessageControllerComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = MyGUIControllerComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MessageCount")
		{
			this->messageCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		if (this->messages.size() < this->messageCount->getUInt())
		{
			this->messages.resize(this->messageCount->getUInt());
			this->editBoxes.resize(this->messageCount->getUInt(), nullptr);
		}
		
		for (size_t i = 0; i < this->messages.size(); i++)
		{
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Message" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->messages[i])
				{
					this->messages[i] = new Variant(MyGUIScrollingMessageControllerComponent::AttrMessage() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttrib(propertyElement, "data"), this->attributes);
				}
				else
				{
					this->messages[i]->setValue(XMLConverter::getAttrib(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
				this->messages[i]->addUserData(GameObject::AttrActionSeparator());
			}
		}
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Duration")
		{
			this->durationSec->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		return success;
	}

	GameObjectCompPtr MyGUIScrollingMessageControllerComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		return nullptr;
	}

	bool MyGUIScrollingMessageControllerComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUIScrollingMessageControllerComponent] Init MyGUI scrolling message controller component for game object: " + this->gameObjectPtr->getName());

		return MyGUIControllerComponent::postInit();
	}

	bool MyGUIScrollingMessageControllerComponent::connect(void)
	{
		bool success = MyGUIControllerComponent::connect();
		
		if (true == this->activated->getBool())
		{
			if (nullptr != this->sourceWidget)
			{
				MyGUI::EditBox* editBox = this->sourceWidget->castType<MyGUI::EditBox>();
				if (nullptr == editBox)
					return true;
				
				for (size_t i = 0; i < this->messages.size(); i++)
				{
					MyGUI::FloatCoord coord = MyGUI::CoordConverter::convertToRelative(this->sourceWidget->getCoord(), this->sourceWidget->getParentSize());
					
					this->editBoxes[i] = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::EditBox>(/*"EditBox"*/"EditBoxEmpty",
						coord.left, coord.top + 0.1f * i, coord.width, coord.height,
							this->sourceWidget->getAlign(), this->sourceWidget->getLayer()->getName());
							
					this->editBoxes[i]->setColour(MyGUI::Colour(this->sourceMyGUIComponent->getColor().x, this->sourceMyGUIComponent->getColor().y, 
						this->sourceMyGUIComponent->getColor().z, this->sourceMyGUIComponent->getColor().w));
					this->editBoxes[i]->setAlpha(this->sourceMyGUIComponent->getColor().w);
					
					MyGUITextComponent* myGuiTextComponent = static_cast<MyGUITextComponent*>(this->sourceMyGUIComponent);

					// Somehow MyGUI Schrott edit box has just invalid values, so take the values from component
					this->editBoxes[i]->setTextColour(MyGUI::Colour(myGuiTextComponent->getTextColor().x, myGuiTextComponent->getTextColor().y, 
						myGuiTextComponent->getTextColor().z, myGuiTextComponent->getTextColor().w));
					this->editBoxes[i]->setFontHeight(myGuiTextComponent->getFontHeight());
					this->editBoxes[i]->setTextAlign(myGuiTextComponent->mapStringToAlign(myGuiTextComponent->getAlign()));
					this->editBoxes[i]->setOnlyText(this->messages[i]->getString());
					
					this->justAdded = true;
					// this->controllerItems[i]->castType<MyGUI::ControllerPosition>()->setCoord(MyGUI::CoordConverter::convertFromRelative(floatCoord, MyGUI::RenderManager::getInstance().getViewSize()));
					// this->controllerItems[i]->castType<MyGUI::ControllerPosition>()->setTime(this->durationSec->getReal());
					// this->controllerItems[i]->castType<MyGUI::ControllerPosition>()->setAction(MyGUI::newDelegate(MyGUI::action::inertionalMoveFunction));

					// MyGUI::ControllerManager::getInstance().addItem(this->editBoxes[i], this->controllerItems[i]);
				}
				
				this->sourceWidget->setVisible(false);
				this->setDurationSec(this->durationSec->getReal());
			}
		}
		return success;
	}

	bool MyGUIScrollingMessageControllerComponent::disconnect(void)
	{
		this->timeToGo = 0.0f;
		this->justAdded = false;
		if (nullptr != this->sourceWidget)
		{
			this->sourceWidget->setVisible(true);
			
			for (size_t i = 0; i < this->messages.size(); i++)
			{
				// If the controller succeeded, it will be deleted internally!
				if (nullptr != this->editBoxes[i])
				{
					MyGUI::Gui::getInstancePtr()->destroyWidget(this->editBoxes[i]);
					this->editBoxes[i] = nullptr;
				}
			}
		}
		
		return MyGUIControllerComponent::disconnect();
	}
	
	void MyGUIScrollingMessageControllerComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			if (true == this->justAdded)
			{
				this->timeToGo -= dt;
			/*	if (this->timeToGo <= 1.0f)
					this->editBoxes[0]->setVisible(true);*/
				if (this->timeToGo < 1e-3f)
				{
					this->timeToGo = 0.0f;
					this->justAdded = false;
				}

				// Scroll the textbox by this much..
				Ogre::Real progress = (this->durationSec->getReal() - this->timeToGo) / this->durationSec->getReal();


				for (unsigned int i = 0; i < this->messageCount->getUInt(); i++)
				{
					MyGUI::FloatCoord relativeCoord = MyGUI::CoordConverter::convertToRelative(this->editBoxes[i]->getCoord(), this->editBoxes[i]->getParentSize());
					
					Ogre::Real x = relativeCoord.left;
					Ogre::Real y = relativeCoord.top + 0.1f * progress;
					
					MyGUI::IntPoint absolutePoint = MyGUI::CoordConverter::convertFromRelative(MyGUI::FloatPoint(x, y), this->editBoxes[i]->getParentSize());

					this->editBoxes[i]->setPosition(absolutePoint.left, absolutePoint.top);
					this->editBoxes[i]->setColour(MyGUI::Colour(this->editBoxes[i]->getTextColour().red, this->editBoxes[i]->getTextColour().green, 
						this->editBoxes[i]->getTextColour().blue, 1.0f - progress/* / 4*/));
					this->editBoxes[i]->setAlpha(1.0f - progress/* / 4*/);
				}
			}
		}
		/*
		if (mJustAdded) {
			mTimeToGo -= time;
			if (mTimeToGo <= 1.0)
				mTextBox[0]->show();
			if (mTimeToGo < 1e-3) {
				mTimeToGo = 0.0;
				mJustAdded = false;
			}
			// scroll the textbox by this much..
			Real progress = (1.5-mTimeToGo)/1.5;
			Real y = POSITION_TOP_Y+SPACING*progress;
			mTextBox[1]->setPosition(POSITION_TOP_X, y);
			mTextBox[1]->setColour(ColourValue(1,1,1,1.0-progress/4));
			mTextBox[2]->setPosition(POSITION_TOP_X, y+SPACING);
			mTextBox[2]->setColour(ColourValue(1,1,1,1.0-progress*progress));
		}
		*/
	}
	
	void MyGUIScrollingMessageControllerComponent::onRemoveComponent(void)
	{
		MyGUIControllerComponent::onRemoveComponent();
		
		for (size_t i = 0; i < this->messages.size(); i++)
		{
			MyGUI::ControllerManager::getInstance().removeItem(this->editBoxes[i]);
			if (nullptr != this->editBoxes[i])
			{
				MyGUI::Gui::getInstancePtr()->destroyWidget(this->editBoxes[i]);
				this->editBoxes[i] = nullptr;
			}
		}
	}
	
	void MyGUIScrollingMessageControllerComponent::onOtherComponentRemoved(unsigned int index)
	{
		MyGUIControllerComponent::onOtherComponentRemoved(index);
		
		auto& gameObjectCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponentByIndex(index));
		if (nullptr != gameObjectCompPtr)
		{
			auto& myGuiCompPtr = boost::dynamic_pointer_cast<MyGUIComponent>(gameObjectCompPtr);
			if (nullptr != myGuiCompPtr)
			{
				if (this->sourceWidget == myGuiCompPtr->getWidget())
				{
					for (size_t i = 0; i < this->messages.size(); i++)
					{
						MyGUI::ControllerManager::getInstance().removeItem(this->editBoxes[i]);
						MyGUI::Gui::getInstancePtr()->destroyWidget(this->editBoxes[i]);
						this->editBoxes[i] = nullptr;
					}
				}
			}
		}
	}

	void MyGUIScrollingMessageControllerComponent::actualizeValue(Variant* attribute)
	{
		MyGUIControllerComponent::actualizeValue(attribute);
		
		if (MyGUIScrollingMessageControllerComponent::AttrMessageCount() == attribute->getName())
		{
			this->setMessageCount(attribute->getUInt());
		}
		else if (MyGUIScrollingMessageControllerComponent::AttrDurationSec() == attribute->getName())
		{
			this->setDurationSec(attribute->getReal());
		}
		else
		{
			for (unsigned int i = 0; i < static_cast<unsigned int>(this->messages.size()); i++)
			{
				if (MyGUIScrollingMessageControllerComponent::AttrMessage() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setMessage(i, attribute->getString());
				}
			}
		}
	}

	void MyGUIScrollingMessageControllerComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		MyGUIControllerComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MessageCount"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->messageCount->getUInt())));
		propertiesXML->append_node(propertyXML);
		

		for (size_t i = 0; i < this->messages.size(); i++)
		{
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "Message" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->messages[i]->getString())));
			propertiesXML->append_node(propertyXML);
		}
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Duration"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->durationSec->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String MyGUIScrollingMessageControllerComponent::getClassName(void) const
	{
		return "MyGUIScrollingMessageControllerComponent";
	}

	Ogre::String MyGUIScrollingMessageControllerComponent::getParentClassName(void) const
	{
		return "MyGUIControllerComponent";
	}
	
	void MyGUIScrollingMessageControllerComponent::setMessageCount(unsigned int messageCount)
	{
		size_t oldSize = this->messages.size();
		this->messageCount->setValue(messageCount);

		if (messageCount > oldSize)
		{
			// Resize the array to new count
			this->messages.resize(messageCount);
			this->editBoxes.resize(messageCount, nullptr);

			for (size_t i = oldSize; i < messageCount; i++)
			{
				this->messages[i] = new Variant(MyGUIScrollingMessageControllerComponent::AttrMessage() + Ogre::StringConverter::toString(i), "My message " + Ogre::StringConverter::toString(i), this->attributes);
				this->messages[i]->addUserData(GameObject::AttrActionSeparator());
			}
		}
		else if (messageCount < oldSize)
		{
			this->eraseVariants(this->messages, messageCount);

			for (auto it = this->editBoxes.begin() + messageCount; it != this->editBoxes.end();)
			{
				MyGUI::ControllerManager::getInstance().removeItem(*it);
				MyGUI::Gui::getInstancePtr()->destroyWidget(*it);
				it = this->editBoxes.erase(it);
				*it = nullptr;
			}
		}
	}
	
	unsigned int MyGUIScrollingMessageControllerComponent::getMessageCount(void) const
	{
		return this->messageCount->getUInt();
	}
	
	void MyGUIScrollingMessageControllerComponent::setMessage(unsigned int index, const Ogre::String& message)
	{
		if (index > this->messages.size())
			index = static_cast<unsigned int>(this->messages.size()) - 1;
		
		this->messages[index]->setValue(message);
	}
	
	Ogre::String MyGUIScrollingMessageControllerComponent::getMessage(unsigned int index) const
	{
		if (index > this->messages.size())
			return "";
		return this->messages[index]->getString();
	}
	
	void MyGUIScrollingMessageControllerComponent::setDurationSec(Ogre::Real durationSec)
	{
		this->durationSec->setValue(durationSec);
		this->timeToGo = durationSec;
	}
	
	Ogre::Real MyGUIScrollingMessageControllerComponent::getDurationSec(void) const
	{
		return this->durationSec->getReal();
	}
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	MyGUIEdgeHideControllerComponent::MyGUIEdgeHideControllerComponent()
		: MyGUIControllerComponent(),
		time(new Variant(MyGUIEdgeHideControllerComponent::AttrTime(), Ogre::Real(1.0f), this->attributes)),
		remainPixels(new Variant(MyGUIEdgeHideControllerComponent::AttrRemainPixels(), Ogre::Real(0.0f), this->attributes)),
		shadowSize(new Variant(MyGUIEdgeHideControllerComponent::AttrShadowSize(), Ogre::Real(0.0f), this->attributes))
	{
		
	}

	MyGUIEdgeHideControllerComponent::~MyGUIEdgeHideControllerComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUIEdgeHideControllerComponent] Destructor MyGUI hide edge controller component for game object: " + this->gameObjectPtr->getName());
	}

	bool MyGUIEdgeHideControllerComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = MyGUIControllerComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Time")
		{
			this->time->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RemainPixels")
		{
			this->remainPixels->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ShadowSize")
		{
			this->remainPixels->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		return success;
	}

	GameObjectCompPtr MyGUIEdgeHideControllerComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		return nullptr;
	}

	bool MyGUIEdgeHideControllerComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUIEdgeHideControllerComponent] Init MyGUI hide edge controller component for game object: " + this->gameObjectPtr->getName());

		return MyGUIControllerComponent::postInit();
	}

	bool MyGUIEdgeHideControllerComponent::connect(void)
	{
		bool success = MyGUIControllerComponent::connect();
		
		if (nullptr == this->controllerItem)
		{
			MyGUI::ControllerItem* item = MyGUI::ControllerManager::getInstance().createItem(MyGUI::ControllerEdgeHide::getClassTypeName());
			this->controllerItem = item->castType<MyGUI::ControllerEdgeHide>();

			this->controllerItem->eventPostAction += MyGUI::newDelegate(this, &MyGUIEdgeHideControllerComponent::controllerFinished);
		}
		
		if (true == this->activated->getBool())
		{
			if (nullptr != this->sourceWidget)
			{	
				// Store old alpha, because alpha will be changed
				// this->oldAlpha = this->sourceWidget->getAlpha();

				MyGUI::ControllerManager::getInstance().addItem(this->sourceWidget, this->controllerItem);

				this->controllerItem->castType<MyGUI::ControllerEdgeHide>()->setTime(this->time->getReal());
				this->controllerItem->castType<MyGUI::ControllerEdgeHide>()->setRemainPixels(this->remainPixels->getUInt());
				this->controllerItem->castType<MyGUI::ControllerEdgeHide>()->setShadowSize(this->shadowSize->getUInt());
			}
		}
		return success;
	}

	bool MyGUIEdgeHideControllerComponent::disconnect(void)
	{
		if (nullptr != this->controllerItem)
		{
			// If the controller succeeded, it will be deleted internally!
			this->controllerItem->eventPostAction += MyGUI::newDelegate(this, &MyGUIEdgeHideControllerComponent::controllerFinished);
			this->controllerItem = nullptr;
		}
		if (nullptr != this->sourceWidget)
		{
			MyGUI::ControllerManager::getInstance().removeItem(this->sourceWidget);
			// Set widget to old alpha
			// this->sourceWidget->setAlpha(this->oldAlpha);
		}

		return MyGUIControllerComponent::disconnect();
	}

	void MyGUIEdgeHideControllerComponent::controllerFinished(MyGUI::Widget* sender, MyGUI::ControllerItem* controller)
	{
		// this->disconnect();
	}

	void MyGUIEdgeHideControllerComponent::actualizeValue(Variant* attribute)
	{
		MyGUIControllerComponent::actualizeValue(attribute);
		
		if (MyGUIEdgeHideControllerComponent::AttrTime() == attribute->getName())
		{
			this->setTimeSec(attribute->getReal());
		}
		else if (MyGUIEdgeHideControllerComponent::AttrRemainPixels() == attribute->getName())
		{
			this->setRemainPixels(attribute->getUInt());
		}
		else if (MyGUIEdgeHideControllerComponent::AttrShadowSize() == attribute->getName())
		{
			this->setShadowSize(attribute->getUInt());
		}
	}

	void MyGUIEdgeHideControllerComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		MyGUIControllerComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Time"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->time->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RemainPixels"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->remainPixels->getUInt())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ShadowSize"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->shadowSize->getUInt())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String MyGUIEdgeHideControllerComponent::getClassName(void) const
	{
		return "MyGUIEdgeHideControllerComponent";
	}

	Ogre::String MyGUIEdgeHideControllerComponent::getParentClassName(void) const
	{
		return "MyGUIControllerComponent";
	}
	
	void MyGUIEdgeHideControllerComponent::setTimeSec(Ogre::Real time)
	{
		this->time->setValue(time);
	}
		
	Ogre::Real MyGUIEdgeHideControllerComponent::getTimeSec(void) const
	{
		return this->time->getReal();
	}
		
	void MyGUIEdgeHideControllerComponent::setRemainPixels(unsigned int remainPixels)
	{
		this->remainPixels->setValue(remainPixels);
	}
		
	unsigned int MyGUIEdgeHideControllerComponent::getRemainPixels(void) const
	{
		return this->remainPixels->getUInt();
	}
	
	void MyGUIEdgeHideControllerComponent::setShadowSize(unsigned int shadowSize)
	{
		this->shadowSize->setValue(shadowSize);
	}
		
	unsigned int MyGUIEdgeHideControllerComponent::getShadowSize(void) const
	{
		return this->shadowSize->getUInt();
	}
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	MyGUIRepeatClickControllerComponent::MyGUIRepeatClickControllerComponent()
		: MyGUIControllerComponent(),
		timeLeft(new Variant(MyGUIRepeatClickControllerComponent::AttrTimeLeft(), Ogre::Real(0.5f), this->attributes)),
		step(new Variant(MyGUIRepeatClickControllerComponent::AttrStep(), Ogre::Real(0.1f), this->attributes))
	{
		
	}

	MyGUIRepeatClickControllerComponent::~MyGUIRepeatClickControllerComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUIRepeatClickControllerComponent] Destructor MyGUI repeat click controller component for game object: " + this->gameObjectPtr->getName());
	}

	bool MyGUIRepeatClickControllerComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = MyGUIControllerComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TimeLeft")
		{
			this->timeLeft->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Step")
		{
			this->step->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		return success;
	}

	GameObjectCompPtr MyGUIRepeatClickControllerComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		return nullptr;
	}

	bool MyGUIRepeatClickControllerComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUIRepeatClickControllerComponent] Init MyGUI repeat click controller component for game object: " + this->gameObjectPtr->getName());

		return MyGUIControllerComponent::postInit();
	}

	bool MyGUIRepeatClickControllerComponent::connect(void)
	{
		bool success = MyGUIControllerComponent::connect();
		
		if (nullptr == this->controllerItem)
		{
			MyGUI::ControllerItem* item = MyGUI::ControllerManager::getInstance().createItem(MyGUI::ControllerRepeatClick::getClassTypeName());
			this->controllerItem = item->castType<MyGUI::ControllerRepeatClick>();

			this->controllerItem->eventPostAction += MyGUI::newDelegate(this, &MyGUIRepeatClickControllerComponent::controllerFinished);
		}
		
		if (true == this->activated->getBool())
		{
			if (nullptr != this->sourceWidget)
			{	
				// Store old alpha, because alpha will be changed
				// this->oldAlpha = this->sourceWidget->getAlpha();

				MyGUI::ControllerManager::getInstance().addItem(this->sourceWidget, this->controllerItem);

				this->controllerItem->castType<MyGUI::ControllerRepeatClick>()->setRepeat(this->timeLeft->getReal(), this->step->getReal());
			}
		}
		return success;
	}

	bool MyGUIRepeatClickControllerComponent::disconnect(void)
	{
		if (nullptr != this->sourceWidget)
		{
			// If the controller succeeded, it will be deleted internally!
			this->controllerItem = nullptr;
		}
		if (nullptr != this->sourceWidget)
		{
			MyGUI::ControllerManager::getInstance().removeItem(this->sourceWidget);
			// Set widget to old alpha
			// this->sourceWidget->setAlpha(this->oldAlpha);
		}

		return MyGUIControllerComponent::disconnect();
	}

	void MyGUIRepeatClickControllerComponent::controllerFinished(MyGUI::Widget* sender, MyGUI::ControllerItem* controller)
	{
		// this->disconnect();
	}

	void MyGUIRepeatClickControllerComponent::actualizeValue(Variant* attribute)
	{
		MyGUIControllerComponent::actualizeValue(attribute);
		
		if (MyGUIRepeatClickControllerComponent::AttrTimeLeft() == attribute->getName())
		{
			this->setTimeLeftSec(attribute->getReal());
		}
		else if (MyGUIRepeatClickControllerComponent::AttrStep() == attribute->getName())
		{
			this->setStep(attribute->getReal());
		}
	}

	void MyGUIRepeatClickControllerComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		MyGUIControllerComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TimeLeft"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->timeLeft->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Step"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->step->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String MyGUIRepeatClickControllerComponent::getClassName(void) const
	{
		return "MyGUIRepeatClickControllerComponent";
	}

	Ogre::String MyGUIRepeatClickControllerComponent::getParentClassName(void) const
	{
		return "MyGUIControllerComponent";
	}
	
	void MyGUIRepeatClickControllerComponent::setTimeLeftSec(Ogre::Real timeLeft)
	{
		this->timeLeft->setValue(timeLeft);
	}
		
	Ogre::Real MyGUIRepeatClickControllerComponent::getTimeLeftSec(void) const
	{
		return this->timeLeft->getReal();
	}
		
	void MyGUIRepeatClickControllerComponent::setStep(Ogre::Real step)
	{
		this->step->setValue(step);
	}
		
	Ogre::Real MyGUIRepeatClickControllerComponent::getStep(void) const
	{
		return this->step->getReal();
	}
	
}; // namespace end
#include "NOWAPrecompiled.h"
#include "MyGUIComponents.h"
#include "LuaScriptComponent.h"
#include "modules/LuaScriptApi.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "utilities/MyGUIUtilities.h"
#include "main/Core.h"
#include "main/InputDeviceCore.h"
#include "main/AppStateManager.h"
#include "MyGUI_LayerManager.h"
#include "MyGUI_ResourceTrueTypeFont.h"
#include "CameraComponent.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;


	CustomPickingMask::CustomPickingMask()
		: width(0),
		height(0)
	{
	}

	bool CustomPickingMask::loadFromAlpha(const Ogre::String& filename, int resizeWidth, int resizeHeight)
	{
		if (true == filename.empty())
		{
			return false;
		}

		// Performance optimization, do not load for the same image the mask x-times
		this->newImageFileName = filename;
		this->oldImageFileName = this->newImageFileName;

		if (this->oldImageFileName == this->newImageFileName)
		{
			return false == this->mask.empty();
		}
		else
		{
			this->mask.clear();
		}

		try
		{
			// Load the image using Ogre::Image2
			Ogre::Image2 image;
			image.load(filename, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);

			// Optionally resize the image
			if (resizeWidth > 0 && resizeHeight > 0)
			{
				image.resize(resizeWidth, resizeHeight);
			}

			this->width = image.getWidth();
			this->height = image.getHeight();

			this->mask.clear();
			this->mask.resize(this->width * this->height, false);

			// Get the color value (RGBA) of each pixel and check the alpha channel
			for (Ogre::uint32 y = 0; y < this->height; ++y)
			{
				for (Ogre::uint32 x = 0; x < this->width; ++x)
				{
					Ogre::ColourValue pixelColor = image.getColourAt(x, y, 0);
					if (pixelColor.a > 0.0f)
					{ 
						// Checks if alpha > 0
						this->mask[y * this->width + x] = true;  // Mark as clickable
					}
				}
			}

			return true;
		}
		catch (const Ogre::Exception& e)
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Failed to load image for image mask: " + e.getDescription());
			return false;
		}
	}

	// Checks if the given point is clickable
	bool CustomPickingMask::pick(const MyGUI::IntPoint& point) const
	{
		if (point.left < 0 || point.top < 0 || point.left >= this->width || point.top >= this->height)
		{
			return false;
		}
		return this->mask[point.top * this->width + point.left];
	}

	bool CustomPickingMask::empty() const
	{
		return this->mask.empty();
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	MyGUIComponent::MyGUIComponent()
		: GameObjectComponent(),
		widget(nullptr),
		isSimulating(false),
		hasParent(false),
		createWidgetInParent(true),
		oldActivated(true),
		priorId(0),
		oldCoordinate(Ogre::Vector4::ZERO),
		activated(new Variant(MyGUIComponent::AttrActivated(), true, this->attributes)),
		position(new Variant(MyGUIComponent::AttrPosition(), Ogre::Vector2(0.5f, 0.5f), this->attributes)),
		size(new Variant(MyGUIComponent::AttrSize(), Ogre::Vector2(0.2f, 0.1f), this->attributes)),
		align(new Variant(MyGUIComponent::AttrAlign(), { "HCenter", "Center", "Left", "Right", "HStretch", "Top", "Bottom", "VStretch", "Stretch", "Default" }, this->attributes)),
		layer(new Variant(MyGUIComponent::AttrLayer(), { "Wallpaper", "ToolTip", "Info", "FadeMiddle", "Popup", "Main", "Modal", "Middle", "Overlapped", "Back", "DragAndDrop", "FadeBusy", "Pointer", "Fade", "Statistic" }, this->attributes)),
		color(new Variant(MyGUIComponent::AttrColor(), Ogre::Vector4(0.5f, 0.5f, 0.5, 1.0f), this->attributes)),
		enabled(new Variant(MyGUIComponent::AttrEnabled(), true, this->attributes)),
		id(new Variant(MyGUIComponent::AttrId(), makeUniqueID(), this->attributes, true)),
		parentId(new Variant(MyGUIComponent::AttrParentId(), static_cast<unsigned long>(0), this->attributes, true))
	{
		this->id->setReadOnly(true);
		this->layer->setListSelectedValue("Wallpaper");
		this->color->addUserData(GameObject::AttrActionColorDialog());
		this->skin = nullptr; // Is created in each derived MyGUI Component
	}

	MyGUIComponent::~MyGUIComponent()
	{
		
	}

	bool MyGUIComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Id")
		{
			this->id->setReadOnly(false);
			this->id->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			this->id->setReadOnly(true);
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Position")
		{
			this->setRealPosition(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Size")
		{
			this->setRealSize(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Align")
		{
			this->setAlign(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Layer")
		{
			this->setLayer(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Color")
		{
			this->setColor(XMLConverter::getAttribVector4(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Enabled")
		{
			this->setEnabled(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ParentId")
		{
			this->parentId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Skin")
		{
			this->skin->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	bool MyGUIComponent::postInit(void)
	{
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &MyGUIComponent::handleWindowChangedDelegate), NOWA::EventDataWindowChanged::getStaticEventType());

		if (nullptr != this->widget)
		{
			this->widget->eventMouseButtonPressed += MyGUI::newDelegate(this, &MyGUIComponent::mouseButtonPressed);
			this->widget->eventMouseButtonDoubleClick += MyGUI::newDelegate(this, &MyGUIComponent::mouseButtonDoubleClick);
			this->widget->eventRootMouseChangeFocus += MyGUI::newDelegate(this, &MyGUIComponent::rootMouseChangeFocus);
			this->widget->eventChangeCoord += MyGUI::newDelegate(this, &MyGUIComponent::changeCoord);
		}

		this->setActivated(this->activated->getBool());
		this->setRealPosition(this->position->getVector2());
		this->setRealSize(this->size->getVector2());
		this->setAlign(this->align->getListSelectedValue());
		this->setLayer(this->layer->getListSelectedValue());
		this->setColor(this->color->getVector4());
		this->setEnabled(this->enabled->getBool());

		// Attach maybe widget to parent
		this->setParentId(this->parentId->getULong());
		// Do not dare to use this one, because it kills all prior set attributes!
		// this->setSkin(this->skin->getListSelectedValue());

		// For identification
		this->widget->setUserData(std::make_pair<unsigned long, unsigned int>(this->gameObjectPtr->getId(), this->getIndex()));
		return true;
	}

	void MyGUIComponent::mouseButtonPressed(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id)
	{
		if (MyGUI::MouseButton::Left == _id)
		{
			this->onWidgetSelected(_sender);
		}
	}

	void MyGUIComponent::mouseButtonDoubleClick(MyGUI::Widget* sender)
	{
		// Recursively search for a widget to select (for demonstration)
		MyGUI::Widget* selectedWidget = this->findWidgetAtPosition(sender, MyGUI::InputManager::getInstance().getMousePosition());

		if (nullptr != selectedWidget)
		{
			// Highlight the selected widget
			this->onWidgetSelected(selectedWidget);
		}
		else
		{
			
		}
	}

	void MyGUIComponent::rootMouseChangeFocus(MyGUI::Widget* sender, bool focus)
	{
		if (true == this->isSimulating)
		{
			if (sender == this->widget)
			{
				// this->widget->_setRootMouseFocus(focus);
				// Call mouse enter event
				if (true == focus)
				{
					if (nullptr != this->gameObjectPtr->getLuaScript() && true == this->enabled->getBool())
					{
						if (this->mouseEnterClosureFunction.is_valid())
						{
							try
							{
								luabind::call_function<void>(this->mouseEnterClosureFunction);
							}
							catch (luabind::error& error)
							{
								luabind::object errorMsg(luabind::from_stack(error.state(), -1));
								std::stringstream msg;
								msg << errorMsg;

								Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnMouseEnter' Error: " + Ogre::String(error.what())
																			+ " details: " + msg.str());
							}
						}
					}
				}
				else
				{
					// Call mouse leave event
					if (nullptr != this->gameObjectPtr->getLuaScript() && true == this->enabled->getBool())
					{
						if (this->mouseLeaveClosureFunction.is_valid())
						{
							try
							{
								luabind::call_function<void>(this->mouseLeaveClosureFunction);
							}
							catch (luabind::error& error)
							{
								luabind::object errorMsg(luabind::from_stack(error.state(), -1));
								std::stringstream msg;
								msg << errorMsg;

								Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnMouseLeave' Error: " + Ogre::String(error.what())
																			+ " details: " + msg.str());
							}
						}
					}
				}
			}
		}
	}

	void MyGUIComponent::changeCoord(MyGUI::Widget* sender)
	{
		// Causes really a mess if a widget has a parent and is inside, hence manually resizing widgets in editor mode, will not be possible
#if 0
		if (false == this->isSimulating)
		{
			// If size has changed somehow, actualize the attributes
			auto viewSize = MyGUI::RenderManager::getInstance().getViewSize();
			if (nullptr != this->widget->getParent())
			{
				viewSize = this->widget->getParent()->getSize();
			}

			MyGUI::FloatCoord relativeCoord = MyGUI::CoordConverter::convertToRelative(sender->getAbsoluteCoord(), viewSize);

			// Set widget to old coordinate
			this->position->setValue(Ogre::Vector2(relativeCoord.left, relativeCoord.top));
			this->size->setValue(Ogre::Vector2(relativeCoord.width, relativeCoord.height));
		}
#endif
	}

	void MyGUIComponent::update(Ogre::Real dt, bool notSimulating)
	{
		this->isSimulating = !notSimulating;
	}

	bool MyGUIComponent::connect(void)
	{
		this->oldCoordinate = Ogre::Vector4(static_cast<Ogre::Real>(this->widget->getCoord().left), static_cast<Ogre::Real>(this->widget->getCoord().top),
											static_cast<Ogre::Real>(this->widget->getCoord().width), static_cast<Ogre::Real>(this->widget->getCoord().height));

		this->setRealPosition(this->position->getVector2());
		this->setRealSize(this->size->getVector2());
		
		return true;
	}

	bool MyGUIComponent::disconnect(void)
	{
		// Causes wrong position if position was one time wrong
#if 0
		// Reset transform as precaution
		if (nullptr != this->widget)
		{
			this->widget->setCoord(static_cast<int>(this->oldCoordinate.x), static_cast<int>(this->oldCoordinate.y),
			 					   static_cast<int>(this->oldCoordinate.z), static_cast<int>(this->oldCoordinate.w));

			// Note: Important relative coordinates always need a context view. If its root, then its the render manager, if the widget is a child, then its the parent size.
			auto viewSize = MyGUI::RenderManager::getInstance().getViewSize();
			if (nullptr != this->widget->getParent())
			{
				viewSize = this->widget->getParent()->getSize();
			}

			MyGUI::FloatCoord oldRelativeCoord = MyGUI::CoordConverter::convertToRelative(MyGUI::IntCoord(this->oldCoordinate.x, this->oldCoordinate.y, this->oldCoordinate.z, this->oldCoordinate.w), viewSize);

			// Set widget to old coordinate
			this->setRealPosition(Ogre::Vector2(oldRelativeCoord.left, oldRelativeCoord.top));
			this->setRealSize(Ogre::Vector2(oldRelativeCoord.width, oldRelativeCoord.height));
		}
#endif

		return true;
	}

	void MyGUIComponent::pause(void)
	{
		// Note: When an appstate is changed, all widgets from the previous AppState remain active (visible), hence hide them
		this->oldActivated = this->activated->getBool();
		this->setActivated(false);
	}

	void MyGUIComponent::resume(void)
	{
		// Set the activated state, to the old stored activated state again
		this->setActivated(this->oldActivated);
	}

	void MyGUIComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (MyGUIComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (MyGUIComponent::AttrPosition() == attribute->getName())
		{
			this->setRealPosition(attribute->getVector2());
		}
		else if (MyGUIComponent::AttrSize() == attribute->getName())
		{
			this->setRealSize(attribute->getVector2());
		}
		else if (MyGUIComponent::AttrAlign() == attribute->getName())
		{
			this->setAlign(attribute->getListSelectedValue());
		}
		else if (MyGUIComponent::AttrLayer() == attribute->getName())
		{
			this->setLayer(attribute->getListSelectedValue());
		}
		else if (MyGUIComponent::AttrColor() == attribute->getName())
		{
			this->setColor(attribute->getVector4());
		}
		else if (MyGUIComponent::AttrEnabled() == attribute->getName())
		{
			this->setEnabled(attribute->getBool());
		}
		else if (MyGUIComponent::AttrParentId() == attribute->getName())
		{
			this->setParentId(attribute->getULong());
		}
		else if (MyGUIComponent::AttrSkin() == attribute->getName())
		{
			this->setSkin(attribute->getListSelectedValue());
		}
		else if (MyGUIWindowComponent::AttrSkin() == attribute->getName())
		{
			this->setSkin(attribute->getListSelectedValue());
		}
	}

	void MyGUIComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Id"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->id->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Position"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->position->getVector2())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Size"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->size->getVector2())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Align"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->align->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Layer"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->layer->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Color"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->color->getVector4())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Enabled"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->enabled->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ParentId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->parentId->getULong())));
		propertiesXML->append_node(propertyXML);
		
		// MyGuiSpriteComponent has no skin
		if (nullptr != this->skin)
		{
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
			propertyXML->append_attribute(doc.allocate_attribute("name", "Skin"));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->skin->getListSelectedValue())));
			propertiesXML->append_node(propertyXML);
		}
	}

	Ogre::String MyGUIComponent::getClassName(void) const
	{
		return "MyGUIComponent";
	}

	Ogre::String MyGUIComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void MyGUIComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
		if (nullptr != this->widget)
		{
			this->widget->setVisible(activated);
			// Also cascade visibility
			for (size_t i = 0; i < this->widget->getChildCount(); i++)
			{
				this->widget->getChildAt(i)->setVisible(activated);
			}
		}
	}

	bool MyGUIComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void MyGUIComponent::handleWindowChangedDelegate(NOWA::EventDataPtr eventData)
	{
		// Messes up in simulation mode, e.g. a position controller is active and when this event occurs, the widget will get its origin position, instead the one of the controller
		// this->setRealPosition(this->position->getVector2());
	}

	void MyGUIComponent::setRealPosition(const Ogre::Vector2& position)
	{
		this->position->setValue(position);
		if (nullptr != this->widget)
		{
			this->widget->setRealPosition(position.x, position.y);

			this->refreshTransform();
		}
	}

	Ogre::Vector2 MyGUIComponent::getRealPosition(void) const
	{
		return this->position->getVector2();
	}

	void MyGUIComponent::setRealSize(const Ogre::Vector2& size)
	{
		this->size->setValue(size);
		if (nullptr != this->widget)
		{
			this->widget->setRealSize(size.x, size.y);

			this->widget->setRealSize(this->size->getVector2().x - 0.001f, this->size->getVector2().y - 0.001f);
			this->widget->setRealSize(this->size->getVector2().x + 0.001f, this->size->getVector2().y + 0.001f);

			this->refreshTransform();
		}
	}

	Ogre::Vector2 MyGUIComponent::getRealSize(void) const
	{
		return this->size->getVector2();
	}

	void MyGUIComponent::refreshTransform(void)
	{
		// Prevent recursion, only if its a root, go through all children, that have this my gui widget as parent
		if (0 == this->parentId->getULong())
		{
			// Refresh positions and sizes
			for (unsigned int i = 0; i < this->gameObjectPtr->getComponents()->size(); i++)
			{
				auto& gameObjectCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponentByIndex(i));
				if (nullptr != gameObjectCompPtr)
				{
					auto& myGuiCompPtr = boost::dynamic_pointer_cast<MyGUIComponent>(gameObjectCompPtr);
					if (nullptr != myGuiCompPtr && myGuiCompPtr->getId() != this->id->getULong())
					{
						if (nullptr != this->widget && nullptr != myGuiCompPtr->getWidget())
						{
							myGuiCompPtr->getWidget()->setRealPosition(myGuiCompPtr->getRealPosition().x, myGuiCompPtr->getRealPosition().y);
							myGuiCompPtr->getWidget()->setRealSize(myGuiCompPtr->getRealSize().x, myGuiCompPtr->getRealSize().y);
						}
					}
				}
			}
		}
	}

	MyGUI::Widget* MyGUIComponent::findWidgetAtPosition(MyGUI::Widget* root, const MyGUI::IntPoint& position)
	{
		// Check if the position is within the root widget
		if (true == root->getClientCoord().inside(position))
		{
			// Iterate over all children
			for (size_t i = 0; i < root->getChildCount(); ++i)
			{
				MyGUI::Widget* child = root->getChildAt(i);
				if (child->getClientCoord().inside(position))
				{
					// Recursive search in child widget
					MyGUI::Widget* found = this->findWidgetAtPosition(child, position);
					if (nullptr != found)
					{
						return found;
					}

					// If no widget found deeper, return the child itself
					return child;
				}
			}
		}
		return nullptr;
	}

	void MyGUIComponent::onWidgetSelected(MyGUI::Widget* sender)
	{
		if (true == this->isSimulating || nullptr == sender)
		{
			return;
		}

		// Retrieve the component associated with the clicked button
		std::pair<unsigned long, unsigned int>* userData = sender->getUserData<std::pair<unsigned long, unsigned int>>(false);
		if (nullptr == userData)
		{
			boost::shared_ptr<NOWA::EventDataMyGUIWidgetSelected> eventDataMyGUIWidgetSelected(new NOWA::EventDataMyGUIWidgetSelected());
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataMyGUIWidgetSelected);
			return;
		}

		boost::shared_ptr<NOWA::EventDataMyGUIWidgetSelected> eventDataMyGUIWidgetSelected(new NOWA::EventDataMyGUIWidgetSelected(this->gameObjectPtr->getId(), this->getIndex()));
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataMyGUIWidgetSelected);
	}
	
	MyGUI::Align MyGUIComponent::mapStringToAlign(const Ogre::String& strAlign)
	{
		if ("HCenter" == strAlign)
			return MyGUI::Align::HCenter;
		else if ("VCenter" == strAlign)
			return MyGUI::Align::VCenter;
		else if ("Center" == strAlign)
			return MyGUI::Align::Center;
		else if ("Left" == strAlign)
			return MyGUI::Align::Left;
		else if ("Right" == strAlign)
			return MyGUI::Align::Right;
		else if ("HStretch" == strAlign)
			return MyGUI::Align::HStretch;
		else if ("Top" == strAlign)
			return MyGUI::Align::Top;
		else if ("Bottom" == strAlign)
			return MyGUI::Align::Bottom;
		else if ("VStretch" == strAlign)
			return MyGUI::Align::VStretch;
		else if ("Stretch" == strAlign)
			return MyGUI::Align::Stretch;
		
		return MyGUI::Align::Default;
	}
	
	Ogre::String MyGUIComponent::mapAlignToString(MyGUI::Align align)
	{
		if (MyGUI::Align::HCenter == align)
			return "HCenter";
		else if (MyGUI::Align::VCenter == align)
			return "VCenter";
		if (MyGUI::Align::Center == align)
			return "Center";
		if (MyGUI::Align::Left == align)
			return "Left";
		if (MyGUI::Align::Right == align)
			return "Right";
		if (MyGUI::Align::HStretch == align)
			return "HStretch";
		if (MyGUI::Align::Top == align)
			return "Top";
		if (MyGUI::Align::Bottom == align)
			return "Bottom";
		if (MyGUI::Align::VStretch == align)
			return "VStretch";
		if (MyGUI::Align::Stretch == align)
			return "Stretch";
		
		return "Default";
	}
	
	void MyGUIComponent::setAlign(const Ogre::String& align)
	{
		this->align->setListSelectedValue(align);
		if (nullptr != this->widget)
		{
			this->widget->setAlign(this->mapStringToAlign(align));
			// this->widget->_setAlign(this->widget->getSize(), this->widget->getSize());
		}
	}
	
	Ogre::String MyGUIComponent::getAlign(void) const
	{
		return this->align->getListSelectedValue();
	}
	
	void MyGUIComponent::setLayer(const Ogre::String& layer)
	{
		this->layer->setListSelectedValue(layer);
		if (nullptr != this->widget)
		{
			if (true == this->widget->isRootWidget())
			{
				MyGUI::LayerManager::getInstance().detachFromLayer(this->widget);
				MyGUI::LayerManager::getInstance().attachToLayerNode(layer, this->widget);
			}
		}
	}
	
	Ogre::String MyGUIComponent::getLayer(void) const
	{
		return this->layer->getListSelectedValue();
	}
	
	void MyGUIComponent::setColor(const Ogre::Vector4& color)
	{
		this->color->setValue(color);
		if (nullptr != this->widget)
		{
			this->widget->setColour(MyGUI::Colour(color.x, color.y, color.z, color.w));
			this->widget->setAlpha(color.w);
		}
	}
	
	Ogre::Vector4 MyGUIComponent::getColor(void) const
	{
		return this->color->getVector4();
	}
	
	void MyGUIComponent::setEnabled(bool enabled)
	{
		this->enabled->setValue(enabled);
		if (nullptr != this->widget)
			this->widget->setEnabled(enabled);
	}
	
	bool MyGUIComponent::getEnabled(void) const
	{
		return this->enabled->getBool();
	}
	
	unsigned long MyGUIComponent::getId(void) const
	{
		return this->id->getULong();
	}

	void MyGUIComponent::onRemoveComponent(void)
	{
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &MyGUIComponent::handleWindowChangedDelegate), NOWA::EventDataWindowChanged::getStaticEventType());

		GameObjectComponent::onRemoveComponent();

		this->widget->detachFromWidget();
		this->hasParent = false;

		// If there are widgets which do have this component as parent, remove them from parent
		/*for (unsigned int i = 0; i < this->gameObjectPtr->getComponents()->size(); i++)
		{
			auto& gameObjectCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponentByIndex(i));
			if (nullptr != gameObjectCompPtr)
			{
				auto& myGuiCompPtr = boost::dynamic_pointer_cast<MyGUIComponent>(gameObjectCompPtr);
				if (nullptr != myGuiCompPtr && this != gameObjectCompPtr.get() && this->parentId->getULong() == myGuiCompPtr->getId())
				{
					myGuiCompPtr->getWidget()->detachFromWidget();
					myGuiCompPtr->hasParent = false;
					this->hasParent = false;
				}
			}
		}*/

		if (nullptr != this->widget && false == this->hasParent)
		{
			MyGUI::Gui::getInstancePtr()->destroyWidget(this->widget);
			this->widget = nullptr;
		}

	}

	void MyGUIComponent::onOtherComponentRemoved(unsigned int index)
	{
		GameObjectComponent::onOtherComponentRemoved(index);

		// For identification
		if (nullptr != this->widget)
		{
			this->widget->setUserData(std::make_pair<unsigned long, unsigned int>(this->gameObjectPtr->getId(), this->getIndex()));
		}
	}

	void MyGUIComponent::onOtherComponentAdded(unsigned int index)
	{
		GameObjectComponent::onOtherComponentAdded(index);
		// For identification
		if (nullptr != this->widget)
		{
			this->widget->setUserData(std::make_pair<unsigned long, unsigned int>(this->gameObjectPtr->getId(), this->getIndex()));
		}
	}

	void MyGUIComponent::onReordered(unsigned int index)
	{
		GameObjectComponent::onReordered(index);
		// For identification
		if (nullptr != this->widget)
		{
			this->widget->setUserData(std::make_pair<unsigned long, unsigned int>(this->gameObjectPtr->getId(), this->getIndex()));
		}
	}

	void MyGUIComponent::setParentId(unsigned long parentId)
	{
// Attention: Remove Item is missing, when id has changed or onRemoveComponent

		// Somehow it can happen between a cloning process and value actualisation, that the id and predecessor id may become the same, so prevent it!
		if (this->id->getULong() != parentId/* && 0 != parentId*/)
		{
			this->parentId->setValue(parentId);
			if (0 == parentId)
			{
				this->layer->setVisible(true);
			}
			else
			{
				this->layer->setVisible(false);
			}

			if (nullptr != this->gameObjectPtr)
			{
				for (unsigned int i = 0; i < this->gameObjectPtr->getComponents()->size(); i++)
				{
					auto& gameObjectCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponentByIndex(i));
					if (nullptr != gameObjectCompPtr)
					{
						auto& myGuiCompPtr = boost::dynamic_pointer_cast<MyGUIComponent>(gameObjectCompPtr);
						if (nullptr != myGuiCompPtr && myGuiCompPtr->getId() != this->id->getULong() && this->parentId->getULong() == myGuiCompPtr->getId())
						{
							if (nullptr != this->widget && nullptr != myGuiCompPtr->getWidget())
							{
								this->widget->attachToWidget(myGuiCompPtr->getWidget());
								this->setRealPosition(this->position->getVector2());
								this->setRealSize(this->size->getVector2());
								this->hasParent = true;
								// Hide the layer if the widget is part of a parent, because it uses the layer from the parent
								this->layer->setVisible(false);
								break;
							}
						}
					}
					else
					{
						this->widget->detachFromWidget();
						this->hasParent = false;
						// Show the layer again
						this->layer->setVisible(true);
					}
				}
			}
		}
		assert(this->id->getULong() != parentId && "Id and parentId are the same!");
	}

	unsigned long MyGUIComponent::getParentId(void) const
	{
		return parentId->getULong();
	}
	
	void MyGUIComponent::setSkin(const Ogre::String& skin)
	{
		if (GameObject::AttrCustomDataSkipCreation() == this->gameObjectPtr->getCustomDataString())
		{
			return;
		}
		this->skin->setListSelectedValue(skin);
		if (nullptr != this->widget)
		{
			this->widget->changeWidgetSkin(skin);
			this->onChangeSkin();
		}
	}
		
	Ogre::String MyGUIComponent::getSkin(void) const
	{
		return this->skin->getListSelectedValue();
	}
	
	MyGUI::Widget* MyGUIComponent::getWidget(void) const
	{
		return this->widget;
	}

	void MyGUIComponent::internalSetPriorId(unsigned long priorId)
	{
		this->priorId = priorId;
	}

	unsigned long MyGUIComponent::getPriorId(void) const
	{
		return this->priorId;
	}

	void MyGUIComponent::reactOnMouseButtonClick(luabind::object closureFunction)
	{
		this->mouseButtonClickClosureFunction = closureFunction;
	}

	void MyGUIComponent::reactOnMouseEnter(luabind::object closureFunction)
	{
		this->mouseEnterClosureFunction = closureFunction;
	}

	void MyGUIComponent::reactOnMouseLeave(luabind::object closureFunction)
	{
		this->mouseLeaveClosureFunction = closureFunction;
	}
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MyGUIWindowComponent::MyGUIWindowComponent()
		: MyGUIComponent(),
		movable(new Variant(MyGUIWindowComponent::AttrMovable(), false, this->attributes)),
		windowCaption(new Variant(MyGUIWindowComponent::AttrWindowCaption(), "", this->attributes))
	{
		this->skin = new Variant(MyGUIComponent::AttrSkin(), { "WindowCSX", "Window", "WindowC", "WindowCX", "WindowCS",
			"WindowCX2_Dark", "WindowCXS2_Dark", "Back1Skin_Dark", "PanelSkin" }, this->attributes);
	}

	MyGUIWindowComponent::~MyGUIWindowComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUIWindowComponent] Destructor MyGUI window component for game object: " + this->gameObjectPtr->getName());
	}

	bool MyGUIWindowComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = MyGUIComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Movable")
		{
			this->setMovable(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WindowCaption")
		{
			this->setWindowCaption(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return success;
	}

	GameObjectCompPtr MyGUIWindowComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		MyGUIWindowCompPtr clonedCompPtr(boost::make_shared<MyGUIWindowComponent>());

		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setRealPosition(this->position->getVector2());
		clonedCompPtr->setRealSize(this->size->getVector2());
		clonedCompPtr->setAlign(this->align->getListSelectedValue());
		clonedCompPtr->setLayer(this->layer->getListSelectedValue());
		clonedCompPtr->setColor(this->color->getVector4());
		clonedCompPtr->setEnabled(this->enabled->getBool());
		clonedCompPtr->internalSetPriorId(this->id->getULong());
		clonedCompPtr->setParentId(this->parentId->getULong()); // Attention
		clonedCompPtr->setSkin(this->skin->getListSelectedValue());
		
		clonedCompPtr->setMovable(this->movable->getBool());
		clonedCompPtr->setWindowCaption(this->windowCaption->getString());
		
		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool MyGUIWindowComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUIWindowComponent] Init MyGUI window component for game object: " + this->gameObjectPtr->getName());
		
		if (true == createWidgetInParent)
		{
			this->widget = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::Window>(this->skin->getListSelectedValue(),
				this->position->getVector2().x, this->position->getVector2().y, this->size->getVector2().x, this->size->getVector2().y,
				this->mapStringToAlign(this->align->getListSelectedValue()), this->layer->getListSelectedValue(), this->getClassName() + "_" + this->gameObjectPtr->getName() + Ogre::StringConverter::toString(this->index));
		}
		else
		{
			this->setRealPosition(this->position->getVector2());
			this->setRealSize(this->size->getVector2());
			this->setAlign(this->align->getListSelectedValue());
			this->setLayer(this->layer->getListSelectedValue());
		}

		this->setMovable(this->movable->getBool());
		this->setWindowCaption(this->windowCaption->getString());
		this->widget->setInheritsAlpha(true);
		Ogre::Vector4 tempColor = this->color->getVector4();
		this->widget->setColour(MyGUI::Colour(tempColor.x, tempColor.y, tempColor.z, tempColor.w));

		return MyGUIComponent::postInit();
	}

	void MyGUIWindowComponent::mouseButtonClick(MyGUI::Widget* sender)
	{
		if (true == this->isSimulating)
		{
			MyGUI::Window* window = sender->castType<MyGUI::Window>();
			if (nullptr != window)
			{
				// Call also function in lua script, if it does exist in the lua script component
				if (nullptr != this->gameObjectPtr->getLuaScript() && true == this->enabled->getBool())
				{
					if (this->mouseButtonClickClosureFunction.is_valid())
					{
						try
						{
							luabind::call_function<void>(this->mouseButtonClickClosureFunction);
						}
						catch (luabind::error& error)
						{
							luabind::object errorMsg(luabind::from_stack(error.state(), -1));
							std::stringstream msg;
							msg << errorMsg;

							Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnMouseButtonClick' Error: " + Ogre::String(error.what())
																		+ " details: " + msg.str());
						}
					}
				}
			}
		}
	}

	bool MyGUIWindowComponent::connect(void)
	{
		bool success = MyGUIComponent::connect();

		return success;
	}

	bool MyGUIWindowComponent::disconnect(void)
	{
		/*this->setSkin(this->skin->getListSelectedValue());
		this->setMovable(this->movable->getBool());*/

		return MyGUIComponent::disconnect();
	}

	void MyGUIWindowComponent::actualizeValue(Variant* attribute)
	{
		MyGUIComponent::actualizeValue(attribute);
		
		if (MyGUIWindowComponent::AttrMovable() == attribute->getName())
		{
			this->setMovable(attribute->getBool());
		}
		else if (MyGUIWindowComponent::AttrWindowCaption() == attribute->getName())
		{
			this->setWindowCaption(attribute->getString());
		}
	}

	void MyGUIWindowComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		MyGUIComponent::writeXML(propertiesXML, doc, filePath);
		
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Movable"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->movable->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "WindowCaption"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->windowCaption->getString())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String MyGUIWindowComponent::getClassName(void) const
	{
		return "MyGUIWindowComponent";
	}

	Ogre::String MyGUIWindowComponent::getParentClassName(void) const
	{
		return "MyGUIComponent";
	}

	void MyGUIWindowComponent::onChangeSkin(void)
	{
		this->setMovable(this->movable->getBool());
		this->setWindowCaption(this->windowCaption->getString());
		this->widget->setInheritsAlpha(true);
		Ogre::Vector4 tempColor = this->color->getVector4();
		this->widget->setColour(MyGUI::Colour(tempColor.x, tempColor.y, tempColor.z, tempColor.w));
	}

	void MyGUIWindowComponent::setMovable(bool movable)
	{
		this->movable->setValue(movable);
		if (nullptr != this->widget)
		{
			widget->castType<MyGUI::Window>()->setMovable(movable);
		}
	}

	bool MyGUIWindowComponent::getMovable(void) const
	{
		return this->movable->getBool();
	}

	void MyGUIWindowComponent::setWindowCaption(const Ogre::String& windowCaption)
	{
		this->windowCaption->setValue(windowCaption);
		if (nullptr != this->widget)
		{
			this->widget->setProperty("Caption", MyGUI::LanguageManager::getInstancePtr()->replaceTags(windowCaption));
			// this->widget->setProperty("CaptionWithReplacing", windowCaption); // Test if that does work
		}
	}

	Ogre::String MyGUIWindowComponent::getWindowCaption(void) const
	{
		return this->windowCaption->getString();
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MyGUITextComponent::MyGUITextComponent()
		: MyGUIComponent(),
		caption(new Variant(MyGUITextComponent::AttrCaption(), "My Text", this->attributes)),
		fontHeight(new Variant(MyGUITextComponent::AttrFontHeight(), static_cast<unsigned int>(21), this->attributes)),
		textAlign(new Variant(MyGUITextComponent::AttrTextAlign(), { "HCenter", "Center", "Left", "Right", "HStretch", "Top", "Bottom", "VStretch", "Stretch", "Default" }, this->attributes)),
		textOffset(new Variant(MyGUITextComponent::AttrTextOffset(), Ogre::Vector2(0.0f, 0.0f), this->attributes)),
		textColor(new Variant(MyGUITextComponent::AttrTextColor(), Ogre::Vector4(0.0f, 0.0f, 0.0, 1.0f), this->attributes)),
		readOnly(new Variant(MyGUITextComponent::AttrReadOnly(), true, this->attributes)),
		multiLine(new Variant(MyGUITextComponent::AttrMultiLine(), false, this->attributes)),
		wordWrap(new Variant(MyGUITextComponent::AttrWordWrap(), false, this->attributes))
	{
		this->skin = new Variant(MyGUIComponent::AttrSkin(), { "EditBox", "TextBox", "EditBoxEmpty", "EditBoxStretch", "WordWrapEmpty" }, this->attributes);
		this->textColor->addUserData(GameObject::AttrActionColorDialog());
	}

	MyGUITextComponent::~MyGUITextComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUITextComponent] Destructor MyGUI text component for game object: " + this->gameObjectPtr->getName());
	}

	bool MyGUITextComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = MyGUIComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Caption")
		{
			this->setCaption(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FontHeight")
		{
			this->setFontHeight(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TextAlign")
		{
			this->setTextAlign(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TextOffset")
		{
			this->setTextOffset(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TextColor")
		{
			this->setTextColor(XMLConverter::getAttribVector4(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ReadOnly")
		{
			this->setReadOnly(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MultiLine")
		{
			this->setMultiLine(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WordWrap")
		{
			this->setWordWrap(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return success;
	}

	GameObjectCompPtr MyGUITextComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		MyGUITextCompPtr clonedCompPtr(boost::make_shared<MyGUITextComponent>());

		
		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setRealPosition(this->position->getVector2());
		clonedCompPtr->setRealSize(this->size->getVector2());
		clonedCompPtr->setAlign(this->align->getListSelectedValue());
		clonedCompPtr->setLayer(this->layer->getListSelectedValue());
		clonedCompPtr->setColor(this->color->getVector4());
		clonedCompPtr->setEnabled(this->enabled->getBool());
		clonedCompPtr->internalSetPriorId(this->id->getULong());
		clonedCompPtr->setParentId(this->parentId->getULong()); // Attention
		clonedCompPtr->setSkin(this->skin->getListSelectedValue());
		
		clonedCompPtr->setCaption(this->caption->getString());
		clonedCompPtr->setFontHeight(this->fontHeight->getUInt());
		clonedCompPtr->setTextAlign(this->align->getListSelectedValue());
		clonedCompPtr->setTextOffset(this->textOffset->getVector2());
		clonedCompPtr->setTextColor(this->textColor->getVector4());
		clonedCompPtr->setReadOnly(this->readOnly->getBool());
		clonedCompPtr->setMultiLine(this->multiLine->getBool());
		clonedCompPtr->setWordWrap(this->wordWrap->getBool());
		
		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool MyGUITextComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUITextComponent] Init MyGUI text component for game object: " + this->gameObjectPtr->getName());
		
		if (true == this->createWidgetInParent)
		{
			this->widget = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::EditBox>(this->skin->getListSelectedValue(),
				this->position->getVector2().x, this->position->getVector2().y, this->size->getVector2().x, this->size->getVector2().y,
				this->mapStringToAlign(this->align->getListSelectedValue()), this->layer->getListSelectedValue(), this->getClassName() + "_" + this->gameObjectPtr->getName() + Ogre::StringConverter::toString(this->index));
		}

		// this->widget->eventKeyButtonPressed += MyGUI::newDelegate(this, &MyGUITextComponent::onKeyButtonPressed);

		this->initTextAttributes();

		return MyGUIComponent::postInit();
	}

	void MyGUITextComponent::initTextAttributes(void)
	{
		this->setActivated(this->activated->getBool());

		this->setColor(this->color->getVector4());
		this->setCaption(this->caption->getString());
		this->setFontHeight(this->fontHeight->getUInt());
		this->setTextAlign(this->textAlign->getListSelectedValue());
		this->setTextOffset(this->textOffset->getVector2());
		this->setTextColor(this->textColor->getVector4());
		this->setReadOnly(this->readOnly->getBool());
		this->setMultiLine(this->multiLine->getBool());
		this->setWordWrap(this->wordWrap->getBool());
		// widget->castType<MyGUI::EditBox>()->setAlign
		// widget->castType<MyGUI::EditBox>()->setCaptionWithReplacing
		// widget->castType<MyGUI::EditBox>()->setEditMultiLine
		// widget->castType<MyGUI::EditBox>()->setOnlyText
		// widget->castType<MyGUI::EditBox>()->setTextAlign(MyGUI::Align::Left);
		// widget->castType<MyGUI::EditBox>()->setEditPassword

		// Skin: Window, WindowC, WindowCX, WindowCS, WindowCSX
		// widget->castType<MyGUI::Window>()->setMovable
		// widget->castType<MyGUI::Window>()->setSnap
		
	
		// widget->castType<MyGUI::Button>()->setCaption
		// widget->castType<MyGUI::Button>()->setCaptionWithReplacing
		// widget->castType<MyGUI::Button>()->eventMouseButtonClick

		// widget->castType<MyGUI::EditBox>()->eventRootMouseChangeFocus += MyGUI::newDelegate(this, &MyGUITextComponent::rootMouseChangeFocus);

		// widget->castType<MyGUI::Button>()->setStateSelected
		
		// MyGUI::ImageBox
		// widget->castType<MyGUI::ImageBox>()->setImageTexture
		/*
		void setImageRect(const IntRect& _value);

		 Set _coord - part of texture where we take tiles
		void setImageCoord(const IntCoord& _value);

		Set _tile size 
		void setImageTile(const IntSize& _value);

		/** Set current tile index
		@param _index - tile index
		@remarks Tiles in file start numbering from left to right and from top to bottom.
		\n For example:\n
		<pre>
		+---+---+---+
		| 0 | 1 | 2 |
		+---+---+---+
		| 3 | 4 | 5 |
		+---+---+---+
		</pre>
		
		void setImageIndex(size_t _index);
		

		*/
	}

	void MyGUITextComponent::mouseButtonClick(MyGUI::Widget* sender)
	{
		if (true == this->isSimulating)
		{
			// Note: Does only work, if user clicked the border of the text box
			MyGUI::EditBox* editBox = sender->castType<MyGUI::EditBox>();
			if (nullptr != editBox)
			{
				// Call also function in lua script, if it does exist in the lua script component
				if (nullptr != this->gameObjectPtr->getLuaScript() && true == this->enabled->getBool())
				{
					if (this->mouseButtonClickClosureFunction.is_valid())
					{
						try
						{
							luabind::call_function<void>(this->mouseButtonClickClosureFunction);
						}
						catch (luabind::error& error)
						{
							luabind::object errorMsg(luabind::from_stack(error.state(), -1));
							std::stringstream msg;
							msg << errorMsg;

							Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnMouseButtonClick' Error: " + Ogre::String(error.what())
																		+ " details: " + msg.str());
						}
					}
				}
			}
		}
	}

	bool MyGUITextComponent::connect(void)
	{
		bool success = MyGUIComponent::connect();

		return success;
	}

	bool MyGUITextComponent::disconnect(void)
	{
		/*this->setCaption(this->caption->getString());
		this->setReadOnly(this->readOnly->getBool());*/

		return MyGUIComponent::disconnect();
	}

	void MyGUITextComponent::actualizeValue(Variant* attribute)
	{
		MyGUIComponent::actualizeValue(attribute);
		
		if (MyGUITextComponent::AttrCaption() == attribute->getName())
		{
			this->setCaption(attribute->getString());
		}
		else if (MyGUITextComponent::AttrFontHeight() == attribute->getName())
		{
			this->setFontHeight(attribute->getUInt());
		}
		else if (MyGUITextComponent::AttrTextAlign() == attribute->getName())
		{
			this->setTextAlign(attribute->getListSelectedValue());
		}
		else if (MyGUITextComponent::AttrTextOffset() == attribute->getName())
		{
			this->setTextOffset(attribute->getVector2());
		}
		else if (MyGUITextComponent::AttrTextColor() == attribute->getName())
		{
			this->setTextColor(attribute->getVector4());
		}
		else if (MyGUITextComponent::AttrReadOnly() == attribute->getName())
		{
			this->setReadOnly(attribute->getBool());
		}
		else if (MyGUITextComponent::AttrMultiLine() == attribute->getName())
		{
			this->setMultiLine(attribute->getBool());
		}
		else if (MyGUITextComponent::AttrWordWrap() == attribute->getName())
		{
			this->setWordWrap(attribute->getBool());
		}
	}

	void MyGUITextComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		MyGUIComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Caption"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->caption->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FontHeight"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->fontHeight->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TextAlign"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->textAlign->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TextOffset"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->textOffset->getVector2())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TextColor"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->textColor->getVector4())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ReadOnly"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->readOnly->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MultiLine"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->multiLine->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "WordWrap"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->wordWrap->getBool())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String MyGUITextComponent::getClassName(void) const
	{
		return "MyGUITextComponent";
	}

	Ogre::String MyGUITextComponent::getParentClassName(void) const
	{
		return "MyGUIComponent";
	}

	void MyGUITextComponent::onChangeSkin(void)
	{
		// Actualize caption, else its not visible
		this->initTextAttributes();
	}

	void MyGUITextComponent::onKeyButtonPressed(MyGUI::Widget* sender, MyGUI::KeyCode code, MyGUI::Char c)
	{
		if (this->widget == sender)
		{
			if (true == InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(OIS::KC_LSHIFT)
				|| true == InputDeviceCore::getSingletonPtr()->getKeyboard()->isKeyDown(OIS::KC_RSHIFT))
			{
				if (code == MyGUI::KeyCode::Return)
				{
					widget->castType<MyGUI::EditBox>()->addText("\\n");
				}
				else if (code == MyGUI::KeyCode::Tab)
				{
					widget->castType<MyGUI::EditBox>()->addText("\\t");
				}
			}
		}
	}
	
	void MyGUITextComponent::setCaption(const Ogre::String& caption)
	{
		// https://stackoverflow.com/questions/12103445/using-in-a-string-as-literal-instead-of-an-escape
		Ogre::String tempString = caption;
		size_t found = tempString.find("\n");
		size_t found2 = tempString.find("\\n");
		if (Ogre::String::npos != found && Ogre::String::npos == found2)
		{
			tempString.erase(found, 1);
			// Add second backslash, so that my gui can detect it (\\n)
			tempString.insert(found, "\\n");
		}

		this->caption->setValue(tempString);
		if (nullptr != this->widget)
		{
			widget->castType<MyGUI::EditBox>()->setCaptionWithReplacing(tempString);
			this->caption->setValue(widget->castType<MyGUI::EditBox>()->getCaption());
		}
	}
		
	Ogre::String MyGUITextComponent::getCaption(void) const
	{
		if (nullptr != this->widget)
		{
			return widget->castType<MyGUI::EditBox>()->getCaption();
		}
		return this->caption->getString();
	}

	void MyGUITextComponent::setFontHeight(unsigned int fontHeight)
	{
		this->fontHeight->setValue(fontHeight);
		if (nullptr == this->widget)
			return;

		this->fontHeight->setValue(MyGUIUtilities::getInstance()->setFontSize(this->widget->castType<MyGUI::EditBox>(false), fontHeight));
	}

// widget->castType<MyGUI::EditBox>()->setEditMultiLine
// widget->castType<MyGUI::EditBox>()-setCaptionWithReplacing(...) for button, checkbox too


	unsigned int MyGUITextComponent::getFontHeight(void) const
	{
		return this->fontHeight->getUInt();
	}

	void MyGUITextComponent::setTextAlign(const Ogre::String & align)
	{
		this->textAlign->setListSelectedValue(align);
		if (nullptr != this->widget)
		{
			widget->castType<MyGUI::EditBox>()->setTextAlign(this->mapStringToAlign(align));
		}
	}

	Ogre::String MyGUITextComponent::getTextAlign(void) const
	{
		return this->textAlign->getListSelectedValue();
	}

	void MyGUITextComponent::setTextOffset(const Ogre::Vector2& textOffset)
	{
		this->textOffset->setValue(textOffset);
		if (nullptr != this->widget)
		{
			widget->castType<MyGUI::EditBox>()->setTextOffset(MyGUI::IntPoint(textOffset.x, textOffset.y));
		}
	}

	Ogre::Vector2 MyGUITextComponent::getTextOffset(void) const
	{
		return this->textOffset->getVector2();
	}
	
	void MyGUITextComponent::setTextColor(const Ogre::Vector4& textColor)
	{
		this->textColor->setValue(textColor);
		if (nullptr != this->widget)
		{
			widget->castType<MyGUI::EditBox>()->setTextColour(MyGUI::Colour(textColor.x, textColor.y, textColor.z, textColor.w));
		}
	}
	
	Ogre::Vector4 MyGUITextComponent::getTextColor(void) const
	{
		return this->textColor->getVector4();
	}

	void MyGUITextComponent::setReadOnly(bool readOnly)
	{
		this->readOnly->setValue(readOnly);
		if (nullptr != this->widget)
		{
			// setEditStatic: ReadOnly and text cannot be selected
			widget->castType<MyGUI::EditBox>()->setEditStatic(readOnly);
		}
	}

	bool MyGUITextComponent::getReadOnly(void) const
	{
		return this->readOnly->getBool();
	}

	void MyGUITextComponent::setMultiLine(bool multiLine)
	{
		this->multiLine->setValue(multiLine);
		if (nullptr != this->widget)
		{
			widget->castType<MyGUI::EditBox>()->setEditMultiLine(multiLine);

			this->multiLine->addUserData(GameObject::AttrActionNeedRefresh());
			if (true == multiLine)
			{
				this->caption->addUserData(GameObject::AttrActionMultiLine());
			}
		}
	}

	bool MyGUITextComponent::getMultiLine(void) const
	{
		return this->multiLine->getBool();
	}

	void MyGUITextComponent::setWordWrap(bool wordWrap)
	{
		this->wordWrap->setValue(wordWrap);
		if (nullptr != this->widget)
		{
			widget->castType<MyGUI::EditBox>()->setEditWordWrap(wordWrap);
		}
	}

	bool MyGUITextComponent::getWordWrap(void) const
	{
		return this->wordWrap->getBool();
	}
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MyGUIButtonComponent::MyGUIButtonComponent()
		: MyGUIComponent(),
		caption(new Variant(MyGUIButtonComponent::AttrCaption(), "My Text", this->attributes)),
		fontHeight(new Variant(MyGUIButtonComponent::AttrFontHeight(), static_cast<unsigned int>(21), this->attributes)),
		textAlign(new Variant(MyGUIButtonComponent::AttrTextAlign(), { "HCenter", "Center", "Left", "Right", "HStretch", "Top", "Bottom", "VStretch", "Stretch", "Default" }, this->attributes)),
		textOffset(new Variant(MyGUIButtonComponent::AttrTextOffset(), Ogre::Vector2(0.0f, 0.0f), this->attributes)),
		textColor(new Variant(MyGUIButtonComponent::AttrTextColor(), Ogre::Vector4(0.0f, 0.0f, 0.0, 1.0f), this->attributes))
	{
		std::vector<Ogre::String> skins({ "Button", "ButtonExpandSkin", "ButtonAcceptSkin", "ButtonLeftSkin", "ButtonRightSkin", "ButtonUpSkin", "ButtonDownSkin" });
		this->skin = new Variant(MyGUIComponent::AttrSkin(), skins, this->attributes);
		this->textColor->addUserData(GameObject::AttrActionColorDialog());
	}

	MyGUIButtonComponent::~MyGUIButtonComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUIButtonComponent] Destructor MyGUI button component for game object: " + this->gameObjectPtr->getName());
	}

	bool MyGUIButtonComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = MyGUIComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Caption")
		{
			this->setCaption(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FontHeight")
		{
			this->setFontHeight(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TextAlign")
		{
			this->setTextAlign(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TextOffset")
		{
			this->setTextOffset(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TextColor")
		{
			this->setTextColor(XMLConverter::getAttribVector4(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		return success;
	}

	GameObjectCompPtr MyGUIButtonComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		MyGUIButtonCompPtr clonedCompPtr(boost::make_shared<MyGUIButtonComponent>());

		
		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setRealPosition(this->position->getVector2());
		clonedCompPtr->setRealSize(this->size->getVector2());
		clonedCompPtr->setAlign(this->align->getListSelectedValue());
		clonedCompPtr->setLayer(this->layer->getListSelectedValue());
		clonedCompPtr->setColor(this->color->getVector4());
		clonedCompPtr->setEnabled(this->enabled->getBool());
		clonedCompPtr->setEnabled(this->enabled->getBool());
		clonedCompPtr->internalSetPriorId(this->id->getULong());
		clonedCompPtr->setParentId(this->parentId->getULong()); // Attention
		clonedCompPtr->setSkin(this->skin->getListSelectedValue());
		
		clonedCompPtr->setCaption(this->caption->getString());
		clonedCompPtr->setFontHeight(this->fontHeight->getUInt());
		clonedCompPtr->setTextAlign(this->align->getListSelectedValue());
		clonedCompPtr->setTextOffset(this->textOffset->getVector2());
		clonedCompPtr->setTextColor(this->textColor->getVector4());
		
		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool MyGUIButtonComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUIButtonComponent] Init MyGUI button component for game object: " + this->gameObjectPtr->getName());
		
		if (true == this->createWidgetInParent)
		{
			this->widget = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::Button>(this->skin->getListSelectedValue(),
				this->position->getVector2().x, this->position->getVector2().y, this->size->getVector2().x, this->size->getVector2().y,
				this->mapStringToAlign(this->align->getListSelectedValue()), this->layer->getListSelectedValue(), this->getClassName() + "_" + this->gameObjectPtr->getName() + Ogre::StringConverter::toString(this->index));
		}

		this->setCaption(this->caption->getString());
		this->setFontHeight(this->fontHeight->getUInt());
		this->setTextAlign(this->textAlign->getListSelectedValue());
		this->setTextOffset(this->textOffset->getVector2());
		this->setTextColor(this->textColor->getVector4());

		// widget->castType<MyGUI::Button>()->setCaption
		// widget->castType<MyGUI::Button>()->setCaptionWithReplacing
		// widget->castType<MyGUI::Button>()->eventMouseButtonClick

		return MyGUIComponent::postInit();
	}

	void MyGUIButtonComponent::mouseButtonClick(MyGUI::Widget* sender)
	{
		if (true == this->isSimulating)
		{
			MyGUI::Button* button = sender->castType<MyGUI::Button>();
			if (nullptr != button)
			{
				// Call also function in lua script, if it does exist in the lua script component
				if (nullptr != this->gameObjectPtr->getLuaScript() && true == this->enabled->getBool())
				{
					if (this->mouseButtonClickClosureFunction.is_valid())
					{
						try
						{
							luabind::call_function<void>(this->mouseButtonClickClosureFunction);
						}
						catch (luabind::error& error)
						{
							luabind::object errorMsg(luabind::from_stack(error.state(), -1));
							std::stringstream msg;
							msg << errorMsg;

							Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnMouseButtonClick' Error: " + Ogre::String(error.what())
																		+ " details: " + msg.str());
						}
					}
				}
				else
				{
					Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Could not process 'reactOnMouseButtonClick' because the given game object: " + this->gameObjectPtr->getName() + " has no lua script component!");
				}
			}
		}
	}

	bool MyGUIButtonComponent::connect(void)
	{
		bool success = MyGUIComponent::connect();

		return success;
	}

	bool MyGUIButtonComponent::disconnect(void)
	{
		// this->setCaption(this->caption->getString());

		return MyGUIComponent::disconnect();
	}

	void MyGUIButtonComponent::actualizeValue(Variant* attribute)
	{
		MyGUIComponent::actualizeValue(attribute);
		
		if (MyGUIButtonComponent::AttrCaption() == attribute->getName())
		{
			this->setCaption(attribute->getString());
		}
		else if (MyGUIButtonComponent::AttrFontHeight() == attribute->getName())
		{
			this->setFontHeight(attribute->getUInt());
		}
		else if (MyGUIButtonComponent::AttrTextAlign() == attribute->getName())
		{
			this->setTextAlign(attribute->getListSelectedValue());
		}
		else if (MyGUIButtonComponent::AttrTextOffset() == attribute->getName())
		{
			this->setTextOffset(attribute->getVector2());
		}
		else if (MyGUIButtonComponent::AttrTextColor() == attribute->getName())
		{
			this->setTextColor(attribute->getVector4());
		}
	}

	void MyGUIButtonComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		MyGUIComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Caption"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->caption->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FontHeight"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->fontHeight->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TextAlign"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->textAlign->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TextOffset"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->textOffset->getVector2())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TextColor"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->textColor->getVector4())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String MyGUIButtonComponent::getClassName(void) const
	{
		return "MyGUIButtonComponent";
	}

	Ogre::String MyGUIButtonComponent::getParentClassName(void) const
	{
		return "MyGUIComponent";
	}

	void MyGUIButtonComponent::onChangeSkin(void)
	{
		this->setCaption(this->caption->getString());
		this->setFontHeight(this->fontHeight->getUInt());
		this->setTextAlign(this->textAlign->getListSelectedValue());
		this->setTextColor(this->textColor->getVector4());
	}
	
	void MyGUIButtonComponent::setCaption(const Ogre::String& caption)
	{
		this->caption->setValue(caption);
		if (nullptr != this->widget)
		{
			widget->castType<MyGUI::Button>()->setCaptionWithReplacing(caption);
		}
	}
		
	Ogre::String MyGUIButtonComponent::getCaption(void) const
	{
		return this->caption->getString();
	}
	
	void MyGUIButtonComponent::setFontHeight(unsigned int fontHeight)
	{
		this->fontHeight->setValue(fontHeight);
		if (nullptr == this->widget)
			return;

		this->fontHeight->setValue(MyGUIUtilities::getInstance()->setFontSize(this->widget->castType<MyGUI::Button>(false), fontHeight));
	}

	unsigned int MyGUIButtonComponent::getFontHeight(void) const
	{
		return this->fontHeight->getUInt();
	}

	void MyGUIButtonComponent::setTextAlign(const Ogre::String & align)
	{
		this->textAlign->setListSelectedValue(align);
		if (nullptr != this->widget)
		{
			widget->castType<MyGUI::Button>()->setTextAlign(this->mapStringToAlign(align));
		}
	}
	
	Ogre::String MyGUIButtonComponent::getTextAlign(void) const
	{
		return this->textAlign->getListSelectedValue();
	}

	void MyGUIButtonComponent::setTextOffset(const Ogre::Vector2& textOffset)
	{
		this->textOffset->setValue(textOffset);
		if (nullptr != this->widget)
		{
			widget->castType<MyGUI::Button>()->setTextOffset(MyGUI::IntPoint(textOffset.x, textOffset.y));
		}
	}

	Ogre::Vector2 MyGUIButtonComponent::getTextOffset(void) const
	{
		return this->textOffset->getVector2();
	}
	
	void MyGUIButtonComponent::setTextColor(const Ogre::Vector4& textColor)
	{
		this->textColor->setValue(textColor);
		if (nullptr != this->widget)
		{
			widget->castType<MyGUI::Button>()->setTextColour(MyGUI::Colour(textColor.x, textColor.y, textColor.z, textColor.w));
		}
	}
	
	Ogre::Vector4 MyGUIButtonComponent::getTextColor(void) const
	{
		return this->textColor->getVector4();
	}
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MyGUICheckBoxComponent::MyGUICheckBoxComponent()
		: MyGUIComponent(),
		caption(new Variant(MyGUICheckBoxComponent::AttrCaption(), "My Text", this->attributes)),
		checked(new Variant(MyGUICheckBoxComponent::AttrChecked(), false, this->attributes)),
		fontHeight(new Variant(MyGUICheckBoxComponent::AttrFontHeight(), static_cast<unsigned int>(21), this->attributes)),
		textAlign(new Variant(MyGUICheckBoxComponent::AttrTextAlign(), { "HCenter", "Center", "Left", "Right", "HStretch", "Top", "Bottom", "VStretch", "Stretch", "Default" }, this->attributes)),
		textOffset(new Variant(MyGUICheckBoxComponent::AttrTextOffset(), Ogre::Vector2(0.0f, 0.0f), this->attributes)),
		textColor(new Variant(MyGUICheckBoxComponent::AttrTextColor(), Ogre::Vector4(0.0f, 0.0f, 0.0, 1.0f), this->attributes))
	{
		this->skin = new Variant(MyGUIComponent::AttrSkin(), { "CheckBox" }, this->attributes);
		this->textColor->addUserData(GameObject::AttrActionColorDialog());
	}

	MyGUICheckBoxComponent::~MyGUICheckBoxComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUICheckBoxComponent] Destructor MyGUI checkbox component for game object: " + this->gameObjectPtr->getName());
	}

	bool MyGUICheckBoxComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = MyGUIComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Caption")
		{
			this->setCaption(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Checked")
		{
			this->setChecked(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FontHeight")
		{
			this->setFontHeight(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TextAlign")
		{
			this->setTextAlign(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TextOffset")
		{
			this->setTextOffset(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TextColor")
		{
			this->setTextColor(XMLConverter::getAttribVector4(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		return success;
	}

	GameObjectCompPtr MyGUICheckBoxComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		MyGUICheckBoxCompPtr clonedCompPtr(boost::make_shared<MyGUICheckBoxComponent>());

		
		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setRealPosition(this->position->getVector2());
		clonedCompPtr->setRealSize(this->size->getVector2());
		clonedCompPtr->setAlign(this->align->getListSelectedValue());
		clonedCompPtr->setLayer(this->layer->getListSelectedValue());
		clonedCompPtr->setColor(this->color->getVector4());
		clonedCompPtr->setEnabled(this->enabled->getBool());
		clonedCompPtr->setEnabled(this->enabled->getBool());
		clonedCompPtr->internalSetPriorId(this->id->getULong());
		clonedCompPtr->setParentId(this->parentId->getULong()); // Attention
		clonedCompPtr->setSkin(this->skin->getListSelectedValue());
		
		clonedCompPtr->setCaption(this->caption->getString());
		clonedCompPtr->setChecked(this->checked->getBool());
		clonedCompPtr->setFontHeight(this->fontHeight->getUInt());
		clonedCompPtr->setTextAlign(this->align->getListSelectedValue());
		clonedCompPtr->setTextOffset(this->textOffset->getVector2());
		clonedCompPtr->setTextColor(this->textColor->getVector4());
		
		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool MyGUICheckBoxComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUICheckBoxComponent] Init MyGUI checkbox component for game object: " + this->gameObjectPtr->getName());
		
		if (true == this->createWidgetInParent)
		{
			this->widget = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::Button>(this->skin->getListSelectedValue(),
				this->position->getVector2().x, this->position->getVector2().y, this->size->getVector2().x, this->size->getVector2().y,
				this->mapStringToAlign(this->align->getListSelectedValue()), this->layer->getListSelectedValue(), this->getClassName() + "_" + this->gameObjectPtr->getName() + Ogre::StringConverter::toString(this->index));
		}

		this->setCaption(this->caption->getString());
		this->setChecked(this->checked->getBool());
		this->setFontHeight(this->fontHeight->getUInt());
		this->setTextAlign(this->textAlign->getListSelectedValue());
		this->setTextOffset(this->textOffset->getVector2());
		this->setTextColor(this->textColor->getVector4());

		return MyGUIComponent::postInit();
	}

	void MyGUICheckBoxComponent::mouseButtonClick(MyGUI::Widget* sender)
	{
		if (true == this->isSimulating)
		{
			MyGUI::Button* button = sender->castType<MyGUI::Button>();
			if (nullptr != button)
			{
				// Call also function in lua script, if it does exist in the lua script component
				if (nullptr != this->gameObjectPtr->getLuaScript() && true == this->enabled->getBool())
				{
					if (this->mouseButtonClickClosureFunction.is_valid())
					{
						try
						{
							luabind::call_function<void>(this->mouseButtonClickClosureFunction);
						}
						catch (luabind::error& error)
						{
							luabind::object errorMsg(luabind::from_stack(error.state(), -1));
							std::stringstream msg;
							msg << errorMsg;

							Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnMouseButtonClick' Error: " + Ogre::String(error.what())
																		+ " details: " + msg.str());
						}
					}
				}
			}
		}
	}

	bool MyGUICheckBoxComponent::connect(void)
	{
		bool success = MyGUIComponent::connect();

		return success;
	}

	bool MyGUICheckBoxComponent::disconnect(void)
	{
		// this->setCaption(this->caption->getString());

		return MyGUIComponent::disconnect();
	}

	void MyGUICheckBoxComponent::actualizeValue(Variant* attribute)
	{
		MyGUIComponent::actualizeValue(attribute);
		
		if (MyGUICheckBoxComponent::AttrCaption() == attribute->getName())
		{
			this->setCaption(attribute->getString());
		}
		else if (MyGUICheckBoxComponent::AttrChecked() == attribute->getName())
		{
			this->setChecked(attribute->getBool());
		}
		else if (MyGUICheckBoxComponent::AttrFontHeight() == attribute->getName())
		{
			this->setFontHeight(attribute->getUInt());
		}
		else if (MyGUICheckBoxComponent::AttrTextAlign() == attribute->getName())
		{
			this->setTextAlign(attribute->getListSelectedValue());
		}
		else if (MyGUICheckBoxComponent::AttrTextOffset() == attribute->getName())
		{
			this->setTextOffset(attribute->getVector2());
		}
		else if (MyGUICheckBoxComponent::AttrTextColor() == attribute->getName())
		{
			this->setTextColor(attribute->getVector4());
		}
	}

	void MyGUICheckBoxComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		MyGUIComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Caption"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->caption->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Checked"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->checked->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FontHeight"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->fontHeight->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TextAlign"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->textAlign->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TextOffset"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->textOffset->getVector2())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TextColor"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->textColor->getVector4())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String MyGUICheckBoxComponent::getClassName(void) const
	{
		return "MyGUICheckBoxComponent";
	}

	Ogre::String MyGUICheckBoxComponent::getParentClassName(void) const
	{
		return "MyGUIComponent";
	}

	void MyGUICheckBoxComponent::onChangeSkin(void)
	{
		this->setCaption(this->caption->getString());
		this->setChecked(this->checked->getBool());
		this->setFontHeight(this->fontHeight->getUInt());
		this->setTextAlign(this->textAlign->getListSelectedValue());
		this->setTextColor(this->textColor->getVector4());
	}
	
	void MyGUICheckBoxComponent::setCaption(const Ogre::String& caption)
	{
		this->caption->setValue(caption);
		if (nullptr != this->widget)
		{
			widget->castType<MyGUI::Button>()->setCaptionWithReplacing(caption);
		}
	}
		
	Ogre::String MyGUICheckBoxComponent::getCaption(void) const
	{
		return this->caption->getString();
	}
	
	void MyGUICheckBoxComponent::setChecked(bool checked)
	{
		this->checked->setValue(checked);
		if (nullptr != this->widget)
		{
			widget->castType<MyGUI::Button>()->setStateCheck(checked);
		}
	}
	
	bool MyGUICheckBoxComponent::getChecked(void) const
	{
		return this->checked->getBool();
	}
	
	void MyGUICheckBoxComponent::setFontHeight(unsigned int fontHeight)
	{
		this->fontHeight->setValue(fontHeight);
		if (nullptr == this->widget)
			return;

		this->fontHeight->setValue(MyGUIUtilities::getInstance()->setFontSize(this->widget->castType<MyGUI::Button>(false), fontHeight));
	}

	unsigned int MyGUICheckBoxComponent::getFontHeight(void) const
	{
		return this->fontHeight->getUInt();
	}

	void MyGUICheckBoxComponent::setTextAlign(const Ogre::String & align)
	{
		this->textAlign->setListSelectedValue(align);
		if (nullptr != this->widget)
		{
			widget->castType<MyGUI::Button>()->setTextAlign(this->mapStringToAlign(align));
			// this->widget->_setAlign(this->widget->getSize(), this->widget->getSize());
		}
	}
	
	Ogre::String MyGUICheckBoxComponent::getTextAlign(void) const
	{
		return this->textAlign->getListSelectedValue();
	}

	void MyGUICheckBoxComponent::setTextOffset(const Ogre::Vector2& textOffset)
	{
		this->textOffset->setValue(textOffset);
		if (nullptr != this->widget)
		{
			widget->castType<MyGUI::Button>()->setTextOffset(MyGUI::IntPoint(textOffset.x, textOffset.y));
		}
	}

	Ogre::Vector2 MyGUICheckBoxComponent::getTextOffset(void) const
	{
		return this->textOffset->getVector2();
	}
	
	void MyGUICheckBoxComponent::setTextColor(const Ogre::Vector4& textColor)
	{
		this->textColor->setValue(textColor);
		if (nullptr != this->widget)
		{
			widget->castType<MyGUI::Button>()->setTextColour(MyGUI::Colour(textColor.x, textColor.y, textColor.z, textColor.w));
		}
	}
	
	Ogre::Vector4 MyGUICheckBoxComponent::getTextColor(void) const
	{
		return this->textColor->getVector4();
	}
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MyGUIImageBoxComponent::MyGUIImageBoxComponent()
		: MyGUIComponent(),
		rotatingSkin(nullptr),
		pickingMask(nullptr),
		imageFileName(new Variant(MyGUIImageBoxComponent::AttrImageFileName(), "NOWA_1D.png", this->attributes)),
		center(new Variant(MyGUIImageBoxComponent::AttrCenter(), Ogre::Vector2(0.0f, 0.0f), this->attributes)),
		angle(new Variant(MyGUIImageBoxComponent::AttrAngle(), 0.0f, this->attributes)),
		rotationSpeed(new Variant(MyGUIImageBoxComponent::AttrRotationSpeed(), 0.0f, this->attributes)),
		usePickingMask(new Variant(MyGUIImageBoxComponent::AttrUsePickingMask(), true, this->attributes))
	{
		std::vector<Ogre::String> skins({ "ImageBox", "RotatingSkin" });
		this->skin = new Variant(MyGUIComponent::AttrSkin(), { skins }, this->attributes);

		this->imageFileName->addUserData(GameObject::AttrActionFileOpenDialog(), "images");
		this->imageFileName->setDescription("Sets the image.");

		this->usePickingMask->setDescription("Sets whether to use pixel exact picking mask for mouse events or not. If set to true, transparent areas will not be clickable.");

		this->layer->setListSelectedValue("Back");
	}

	MyGUIImageBoxComponent::~MyGUIImageBoxComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUIImageBoxComponent] Destructor MyGUI image box component for game object: " + this->gameObjectPtr->getName());
	}

	bool MyGUIImageBoxComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = MyGUIComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ImageFileName")
		{
			this->setImageFileName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Center")
		{
			this->setCenter(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Angle")
		{
			this->setAngle(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RotationSpeed")
		{
			this->setRotationSpeed(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == MyGUIImageBoxComponent::AttrUsePickingMask())
		{
			this->usePickingMask->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return success;
	}

	GameObjectCompPtr MyGUIImageBoxComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		MyGUIImageBoxCompPtr clonedCompPtr(boost::make_shared<MyGUIImageBoxComponent>());

		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setRealPosition(this->position->getVector2());
		clonedCompPtr->setRealSize(this->size->getVector2());
		clonedCompPtr->setAlign(this->align->getListSelectedValue());
		clonedCompPtr->setLayer(this->layer->getListSelectedValue());
		clonedCompPtr->setColor(this->color->getVector4());
		clonedCompPtr->setEnabled(this->enabled->getBool());
		clonedCompPtr->setEnabled(this->enabled->getBool());
		clonedCompPtr->internalSetPriorId(this->id->getULong());
		clonedCompPtr->setParentId(this->parentId->getULong()); // Attention
		clonedCompPtr->setSkin(this->skin->getListSelectedValue());
	
		clonedCompPtr->setUsePickingMask(this->usePickingMask->getBool());
		clonedCompPtr->setImageFileName(this->imageFileName->getString());
		clonedCompPtr->setCenter(this->center->getVector2());
		clonedCompPtr->setAngle(this->angle->getReal());
		clonedCompPtr->setRotationSpeed(this->rotationSpeed->getReal());
		
		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool MyGUIImageBoxComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUIImageBoxComponent] Init MyGUI image box component for game object: " + this->gameObjectPtr->getName());
		
		if (true == this->createWidgetInParent)
		{
			this->widget = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::ImageBox>(this->skin->getListSelectedValue(),
				this->position->getVector2().x, this->position->getVector2().y, this->size->getVector2().x, this->size->getVector2().y,
				this->mapStringToAlign(this->align->getListSelectedValue()), this->layer->getListSelectedValue(), this->getClassName() + "_" + this->gameObjectPtr->getName() + Ogre::StringConverter::toString(this->index));
		}

		MyGUI::ISubWidget* main = this->widget->getSubWidgetMain();
		if (nullptr == main)
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[EngineResourceListener] Error: Could not get MyGUI object. Check the resources.cfg, whether 'FileSystem=../../media/MyGUI_Media' is missing!");
				// throw Ogre::Exception(Ogre::Exception::ERR_ITEM_NOT_FOUND, "[EngineResourceListener] Error: Could not get MyGUI object. Check the resources.cfg, whether 'FileSystem=../../media/MyGUI_Media' is missing!\n", "NOWA");
		}

		if ("RotatingSkin" == this->skin->getListSelectedValue())
		{
			this->rotatingSkin = main->castType<MyGUI::RotatingSkin>();
		}

		this->setUsePickingMask(this->usePickingMask->getBool());
		this->setImageFileName(this->imageFileName->getString());
		this->setCenter(this->center->getVector2());
		this->setAngle(this->angle->getReal());
		this->setRotationSpeed(this->rotationSpeed->getReal());
	
		// widget->castType<MyGUI::Button>()->setCaption
		// widget->castType<MyGUI::Button>()->setCaptionWithReplacing
		// widget->castType<MyGUI::Button>()->eventMouseButtonClick

		return MyGUIComponent::postInit();
	}
	
	void MyGUIImageBoxComponent::update(Ogre::Real dt, bool notSimulating)
	{
		MyGUIComponent::update(dt, notSimulating);
		
		if (false == notSimulating)
		{
			if (nullptr == this->rotatingSkin)
			{
				return;
			}

			Ogre::Real tempRotationSpeed = this->rotationSpeed->getReal();
			if (0.0f != tempRotationSpeed)
			{
				this->angle->setValue(0.0f);

				Ogre::Real tempRadian = this->rotatingSkin->getAngle() + (MathHelper::getInstance()->degreeAngleToRadian(tempRotationSpeed) * dt);
				if (tempRadian >= Ogre::Math::TWO_PI)
				{
					tempRadian = 0.0f;
				}
				this->rotatingSkin->setAngle(tempRadian);
			}
		}
	}

	void MyGUIImageBoxComponent::mouseButtonClick(MyGUI::Widget* sender)
	{
#if 0
		if (true == this->isSimulating)
		{
			MyGUI::ImageBox* imageBox = sender->castType<MyGUI::ImageBox>();
			if (nullptr != imageBox)
			{
				this->callMousePressLuaFunction();
			}
		}
#endif
	}

	void MyGUIImageBoxComponent::callMousePressLuaFunction(void)
	{
		// Call also function in lua script, if it does exist in the lua script component
		if (nullptr != this->gameObjectPtr->getLuaScript() && true == this->enabled->getBool())
		{
			if (this->mouseButtonClickClosureFunction.is_valid())
			{
				try
				{
					luabind::call_function<void>(this->mouseButtonClickClosureFunction);
				}
				catch (luabind::error& error)
				{
					luabind::object errorMsg(luabind::from_stack(error.state(), -1));
					std::stringstream msg;
					msg << errorMsg;

					Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnMouseButtonClick' Error: " + Ogre::String(error.what())
																+ " details: " + msg.str());
				}
			}
		}
	}

	void MyGUIImageBoxComponent::mouseButtonPressed(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id)
	{
		MyGUIComponent::mouseButtonPressed(_sender, _left, _top, _id);

		if (true == this->isSimulating)
		{
			if (true == this->usePickingMask->getBool())
			{
				MyGUI::IntPoint mousePosition(_left, _top);
				MyGUI::IntCoord imageCoord = _sender->castType<MyGUI::ImageBox>()->getAbsoluteCoord();

				// Get the size of the widget and image
				int widgetWidth = imageCoord.width;
				int widgetHeight = imageCoord.height;
				int imageWidth = this->widget->castType<MyGUI::ImageBox>()->getImageSize().width;
				int imageHeight = this->widget->castType<MyGUI::ImageBox>()->getImageSize().height;

				// Scale the mouse position to match the image's size
				int scaledX = (mousePosition.left - imageCoord.left) * imageWidth / widgetWidth;
				int scaledY = (mousePosition.top - imageCoord.top) * imageHeight / widgetHeight;

				// Check the picking mask using the scaled coordinates
				if (true == this->pickingMask->pick(MyGUI::IntPoint(scaledX, scaledY)))
				{
					// std::cout << "Clickable area clicked!" << std::endl;
					this->callMousePressLuaFunction();
				}
				else
				{
					// TODO: What here?
					// std::cout << "Clicked on a transparent area!" << std::endl;
				}
			}
			else
			{
				this->callMousePressLuaFunction();
			}
		}
	}

	bool MyGUIImageBoxComponent::connect(void)
	{
		bool success = MyGUIComponent::connect();

		return success;
	}

	bool MyGUIImageBoxComponent::disconnect(void)
	{
		// this->setImageFileName(this->imageFileName->getString());

		return MyGUIComponent::disconnect();
	}

	void MyGUIImageBoxComponent::onRemoveComponent(void)
	{
		MyGUIComponent::onRemoveComponent();

		if (nullptr != this->pickingMask)
		{
			delete this->pickingMask;
			this->pickingMask = nullptr;
		}
	}

	void MyGUIImageBoxComponent::actualizeValue(Variant* attribute)
	{
		MyGUIComponent::actualizeValue(attribute);
		
		if (MyGUIImageBoxComponent::AttrImageFileName() == attribute->getName())
		{
			this->setImageFileName(attribute->getString());
		}
		else if (MyGUIImageBoxComponent::AttrCenter() == attribute->getName())
		{
			this->setCenter(attribute->getVector2());
		}
		else if (MyGUIImageBoxComponent::AttrAngle() == attribute->getName())
		{
			this->setAngle(attribute->getReal());
		}
		else if (MyGUIImageBoxComponent::AttrRotationSpeed() == attribute->getName())
		{
			this->setRotationSpeed(attribute->getReal());
		}
		else if (MyGUIImageBoxComponent::AttrUsePickingMask() == attribute->getName())
		{
			this->setUsePickingMask(attribute->getBool());
		}
	}

	void MyGUIImageBoxComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		MyGUIComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ImageFileName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->imageFileName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Center"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->center->getVector2())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Angle"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->angle->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RotationSpeed"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rotationSpeed->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UsePickingMask"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->usePickingMask->getBool())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String MyGUIImageBoxComponent::getClassName(void) const
	{
		return "MyGUIImageBoxComponent";
	}

	Ogre::String MyGUIImageBoxComponent::getParentClassName(void) const
	{
		return "MyGUIComponent";
	}

	void MyGUIImageBoxComponent::onChangeSkin(void)
	{
		MyGUI::ISubWidget* main = this->widget->getSubWidgetMain();
		if (nullptr == main)
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[EngineResourceListener] Error: Could not get MyGUI object. Check the resources.cfg, whether 'FileSystem=../../media/MyGUI_Media' is missing!");
			// throw Ogre::Exception(Ogre::Exception::ERR_ITEM_NOT_FOUND, "[EngineResourceListener] Error: Could not get MyGUI object. Check the resources.cfg, whether 'FileSystem=../../media/MyGUI_Media' is missing!\n", "NOWA");
		}

		if ("RotatingSkin" == this->skin->getListSelectedValue())
		{
			this->rotatingSkin = main->castType<MyGUI::RotatingSkin>();
		}

		this->setImageFileName(this->imageFileName->getString());
		this->setCenter(this->center->getVector2());
		this->setAngle(this->angle->getReal());
		this->setRotationSpeed(this->rotationSpeed->getReal());
	}
	
	void MyGUIImageBoxComponent::setImageFileName(const Ogre::String& imageFileName)
	{
		this->imageFileName->setValue(imageFileName);
		if (nullptr != this->widget)
		{
			widget->castType<MyGUI::ImageBox>()->setImageTexture(imageFileName);

			if (true == this->usePickingMask->getBool())
			{
				// Create a custom picking mask based on the alpha channel using Ogre::Image2
				if (this->pickingMask->loadFromAlpha(imageFileName))
				{

				}
			}
		}
	}
		
	Ogre::String MyGUIImageBoxComponent::getImageFileName(void) const
	{
		return this->imageFileName->getString();
	}
	
	void MyGUIImageBoxComponent::setCenter(const Ogre::Vector2& center)
	{
		this->center->setValue(center);
		if (nullptr != this->rotatingSkin)
		{
			// Attention: Here better this->rotatingSkin->getWidth()  * center.x etc. ?
			this->rotatingSkin->setCenter(MyGUI::IntPoint(static_cast<int>(center.x), static_cast<int>(center.y)));
		}
	}
	
	Ogre::Vector2 MyGUIImageBoxComponent::getCenter(void) const
	{
		return this->center->getVector2();
	}
	
	void MyGUIImageBoxComponent::setAngle(Ogre::Real angle)
	{
		this->angle->setValue(angle);
		if (nullptr != this->rotatingSkin)
		{
			// Attention: Here better this->rotatingSkin->getWidth()  * center.x etc. ?
			this->rotatingSkin->setAngle(MathHelper::getInstance()->degreeAngleToRadian(angle));
		}
	}
	
	Ogre::Real MyGUIImageBoxComponent::getAngle(void) const
	{
		return this->angle->getReal();
	}
	
	void MyGUIImageBoxComponent::setRotationSpeed(Ogre::Real rotationSpeed)
	{
		this->rotationSpeed->setValue(rotationSpeed);
	}
	
	Ogre::Real MyGUIImageBoxComponent::getRotationSpeed(void) const
	{
		return this->rotationSpeed->getReal();
	}

	void MyGUIImageBoxComponent::setUsePickingMask(bool usePickingMask)
	{
		this->usePickingMask->setValue(usePickingMask);

		if (true == usePickingMask)
		{
			if (nullptr == this->pickingMask)
			{
				pickingMask = new CustomPickingMask();
			}
		}
		else
		{
			if (nullptr != this->pickingMask)
			{
				delete this->pickingMask;
				this->pickingMask = nullptr;
			}
		}
	}

	bool MyGUIImageBoxComponent::getUsePickingMask(void) const
	{
		return this->usePickingMask->getBool();
	}
	
	MyGUI::RotatingSkin* MyGUIImageBoxComponent::getRotatingSkin(void) const
	{
		return this->rotatingSkin;
	}
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MyGUIProgressBarComponent::MyGUIProgressBarComponent()
		: MyGUIComponent(),
		value(new Variant(MyGUIProgressBarComponent::AttrValue(), static_cast<unsigned int>(0), this->attributes)),
		range(new Variant(MyGUIProgressBarComponent::AttrRange(), static_cast<unsigned int>(100), this->attributes)),
		flowDirection(new Variant(MyGUIProgressBarComponent::AttrFlowDirection(), { "LeftToRight", "RightToLeft", "TopToBottom", "BottomToTop" }, this->attributes))
	{
		std::vector<Ogre::String> skins({ "ProgressBar", "ProgressBarFill" });
		this->skin = new Variant(MyGUIComponent::AttrSkin(), skins, this->attributes);
	}

	MyGUIProgressBarComponent::~MyGUIProgressBarComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUIProgressBarComponent] Destructor MyGUI progress bar component for game object: " + this->gameObjectPtr->getName());
	}

	bool MyGUIProgressBarComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = MyGUIComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Value")
		{
			this->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Range")
		{
			this->setRange(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FlowDirection")
		{
			this->setFlowDirection(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		return success;
	}

	GameObjectCompPtr MyGUIProgressBarComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		MyGUIProgressBarCompPtr clonedCompPtr(boost::make_shared<MyGUIProgressBarComponent>());

		
		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setRealPosition(this->position->getVector2());
		clonedCompPtr->setRealSize(this->size->getVector2());
		clonedCompPtr->setAlign(this->align->getListSelectedValue());
		clonedCompPtr->setLayer(this->layer->getListSelectedValue());
		clonedCompPtr->setColor(this->color->getVector4());
		clonedCompPtr->setEnabled(this->enabled->getBool());
		clonedCompPtr->setEnabled(this->enabled->getBool());
		clonedCompPtr->internalSetPriorId(this->id->getULong());
		clonedCompPtr->setParentId(this->parentId->getULong()); // Attention
		clonedCompPtr->setSkin(this->skin->getListSelectedValue());
		
		clonedCompPtr->setValue(this->value->getUInt());
		clonedCompPtr->setRange(this->range->getUInt());
		clonedCompPtr->setFlowDirection(this->flowDirection->getListSelectedValue());
		
		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool MyGUIProgressBarComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUIProgressBarComponent] Init MyGUI progress bar component for game object: " + this->gameObjectPtr->getName());
		
		if (true == this->createWidgetInParent)
		{
			this->widget = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::ProgressBar>(this->skin->getListSelectedValue(),
				this->position->getVector2().x, this->position->getVector2().y, this->size->getVector2().x, this->size->getVector2().y,
				this->mapStringToAlign(this->align->getListSelectedValue()), this->layer->getListSelectedValue(), this->getClassName() + "_" + this->gameObjectPtr->getName() + Ogre::StringConverter::toString(this->index));
		}
		// Order is important range must be placed before value can be set
		this->setRange(this->range->getUInt());
		this->setValue(this->value->getUInt());
		this->setFlowDirection(this->flowDirection->getListSelectedValue());

		return MyGUIComponent::postInit();
	}

	void MyGUIProgressBarComponent::mouseButtonClick(MyGUI::Widget* sender)
	{
		if (true == this->isSimulating)
		{
			MyGUI::ProgressBar* progressBar = sender->castType<MyGUI::ProgressBar>();
			if (nullptr != progressBar)
			{
				// Call also function in lua script, if it does exist in the lua script component
				if (nullptr != this->gameObjectPtr->getLuaScript() && true == this->enabled->getBool())
				{
					if (this->mouseButtonClickClosureFunction.is_valid())
					{
						try
						{
							luabind::call_function<void>(this->mouseButtonClickClosureFunction);
						}
						catch (luabind::error& error)
						{
							luabind::object errorMsg(luabind::from_stack(error.state(), -1));
							std::stringstream msg;
							msg << errorMsg;

							Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnMouseButtonClick' Error: " + Ogre::String(error.what())
																		+ " details: " + msg.str());
						}
					}
				}
			}
		}
	}

	bool MyGUIProgressBarComponent::connect(void)
	{
		bool success = MyGUIComponent::connect();

		return success;
	}

	bool MyGUIProgressBarComponent::disconnect(void)
	{
		// this->setCaption(this->caption->getString());

		return MyGUIComponent::disconnect();
	}

	void MyGUIProgressBarComponent::actualizeValue(Variant* attribute)
	{
		MyGUIComponent::actualizeValue(attribute);
		
		if (MyGUIProgressBarComponent::AttrValue() == attribute->getName())
		{
			this->setValue(attribute->getUInt());
		}
		else if (MyGUIProgressBarComponent::AttrRange() == attribute->getName())
		{
			this->setRange(attribute->getUInt());
		}
		else if (MyGUIProgressBarComponent::AttrFlowDirection() == attribute->getName())
		{
			this->setFlowDirection(attribute->getListSelectedValue());
		}
	}

	void MyGUIProgressBarComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		MyGUIComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Value"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->value->getUInt())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Range"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->range->getUInt())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FlowDirection"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->flowDirection->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String MyGUIProgressBarComponent::getClassName(void) const
	{
		return "MyGUIProgressBarComponent";
	}

	Ogre::String MyGUIProgressBarComponent::getParentClassName(void) const
	{
		return "MyGUIComponent";
	}

	void MyGUIProgressBarComponent::onChangeSkin(void)
	{
		this->setRange(this->range->getUInt());
		this->setValue(this->value->getUInt());
		this->setFlowDirection(this->flowDirection->getListSelectedValue());
	}
	
	void MyGUIProgressBarComponent::setValue(unsigned int value)
	{
		if (value > this->range->getUInt())
		{
			if (nullptr != this->gameObjectPtr)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUIProgressBarComponent] Warning value: '" + Ogre::StringConverter::toString(value) 
					+ "' is higher as range: '" + Ogre::StringConverter::toString(this->range->getUInt()) + "' for game object: " + this->gameObjectPtr->getName());
			}
		}
		this->value->setValue(value);
		if (nullptr != this->widget)
		{
			widget->castType<MyGUI::ProgressBar>()->setProgressPosition(value);
		}
	}
		
	unsigned int MyGUIProgressBarComponent::getValue(void) const
	{
		return this->value->getUInt();
	}
	
	void MyGUIProgressBarComponent::setRange(unsigned int range)
	{
		this->range->setValue(range);
		if (nullptr != this->widget)
		{
			widget->castType<MyGUI::ProgressBar>()->setProgressRange(range);
		}
	}
	
	unsigned int MyGUIProgressBarComponent::getRange(void) const
	{
		return this->range->getUInt();
	}
	
	void MyGUIProgressBarComponent::setFlowDirection(const Ogre::String& flowDirection)
	{
		this->flowDirection->setListSelectedValue(flowDirection);
		if (nullptr != this->widget)
		{
			if ("LeftToRight" == flowDirection)
				widget->castType<MyGUI::ProgressBar>()->setFlowDirection(MyGUI::FlowDirection::LeftToRight);
			else if ("RightToLeft" == flowDirection)
				widget->castType<MyGUI::ProgressBar>()->setFlowDirection(MyGUI::FlowDirection::RightToLeft);
			else if ("TopToBottom" == flowDirection)
				widget->castType<MyGUI::ProgressBar>()->setFlowDirection(MyGUI::FlowDirection::TopToBottom);
			else if ("BottomToTop" == flowDirection)
				widget->castType<MyGUI::ProgressBar>()->setFlowDirection(MyGUI::FlowDirection::BottomToTop);

			/*widget->castType<MyGUI::ListBox>()->addItem(Ogre::String("Hallo"));
			widget->castType<MyGUI::ListBox>()->deleteItem(size_t index);
			widget->castType<MyGUI::ListBox>()->setItemNameAt(size_t index, string);
			widget->castType<MyGUI::ListBox>()->setRenderItemTexture(myguitexture);*/
		}
	}
	
	Ogre::String MyGUIProgressBarComponent::getFlowDirection(void) const
	{
		return this->flowDirection->getListSelectedValue();
	}
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MyGUIListBoxComponent::MyGUIListBoxComponent()
		: MyGUIComponent(),
		itemHeight(new Variant(MyGUIListBoxComponent::AttrItemHeight(), static_cast<unsigned int>(21), this->attributes)),
		fontHeight(new Variant(MyGUIListBoxComponent::AttrFontHeight(), static_cast<unsigned int>(20), this->attributes)),
		textAlign(new Variant(MyGUIListBoxComponent::AttrTextAlign(), { "HCenter", "Center", "Left", "Right", "HStretch", "Top", "Bottom", "VStretch", "Stretch", "Default" }, this->attributes)),
		textOffset(new Variant(MyGUIListBoxComponent::AttrTextOffset(), Ogre::Vector2(0.0f, 0.0f), this->attributes)),
		textColor(new Variant(MyGUIListBoxComponent::AttrTextColor(), Ogre::Vector4(0.0f, 0.0f, 0.0, 1.0f), this->attributes)),
		itemCount(new Variant(MyGUIListBoxComponent::AttrItemCount(), 0, this->attributes))
	{
		std::vector<Ogre::String> skins({ "ListBox", "ListBox_Dark", "ListBox2_Dark", "ListBox3_Dark" });
		// ATTENTION: ListBox3_Dark has no textbox, hence no items are shown
		this->skin = new Variant(MyGUIComponent::AttrSkin(), skins, this->attributes);
		
		this->textColor->addUserData(GameObject::AttrActionColorDialog());
		this->itemCount->addUserData(GameObject::AttrActionNeedRefresh());
	}

	MyGUIListBoxComponent::~MyGUIListBoxComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUIListBoxComponent] Destructor MyGUI list box component for game object: " + this->gameObjectPtr->getName());
	}

	bool MyGUIListBoxComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = MyGUIComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ItemHeight")
		{
			this->setItemHeight(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FontHeight")
		{
			this->setFontHeight(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TextAlign")
		{
			this->setTextAlign(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TextOffset")
		{
			this->setTextOffset(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TextColor")
		{
			this->setTextColor(XMLConverter::getAttribVector4(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ItemCount")
		{
			this->itemCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data", 1));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (this->items.size() < this->itemCount->getUInt())
		{
			this->items.resize(this->itemCount->getUInt());
		}

		for (size_t i = 0; i < this->items.size(); i++)
		{
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Item" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->items[i])
				{
					this->items[i] = new Variant(MyGUIListBoxComponent::AttrItem() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttrib(propertyElement, "data"), this->attributes);
				}
				else
				{
					this->items[i]->setValue(XMLConverter::getAttrib(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
				this->items[i]->addUserData(GameObject::AttrActionSeparator());
			}
		}
		
		return success;
	}

	GameObjectCompPtr MyGUIListBoxComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		MyGUIListBoxCompPtr clonedCompPtr(boost::make_shared<MyGUIListBoxComponent>());

		
		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setRealPosition(this->position->getVector2());
		clonedCompPtr->setRealSize(this->size->getVector2());
		clonedCompPtr->setAlign(this->align->getListSelectedValue());
		clonedCompPtr->setLayer(this->layer->getListSelectedValue());
		clonedCompPtr->setColor(this->color->getVector4());
		clonedCompPtr->setEnabled(this->enabled->getBool());
		clonedCompPtr->internalSetPriorId(this->id->getULong());
		clonedCompPtr->setParentId(this->parentId->getULong()); // Attention
		clonedCompPtr->setSkin(this->skin->getListSelectedValue());
		

		clonedCompPtr->setItemCount(this->itemCount->getUInt());
		for (unsigned int i = 0; i < this->itemCount->getUInt(); i++)
		{
			clonedCompPtr->setItemText(i, this->items[i]->getString());
		}

		clonedCompPtr->setItemHeight(this->itemHeight->getUInt());
		clonedCompPtr->setFontHeight(this->fontHeight->getUInt());
		clonedCompPtr->setTextAlign(this->align->getListSelectedValue());
		clonedCompPtr->setTextOffset(this->textOffset->getVector2());
		clonedCompPtr->setTextColor(this->textColor->getVector4());
		
		
		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool MyGUIListBoxComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUIListBoxComponent] Init MyGUI list box component for game object: " + this->gameObjectPtr->getName());
		
		if (true == this->createWidgetInParent)
		{
			this->widget = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::ListBox>(this->skin->getListSelectedValue(),
				this->position->getVector2().x, this->position->getVector2().y, this->size->getVector2().x, this->size->getVector2().y,
				this->mapStringToAlign(this->align->getListSelectedValue()), this->layer->getListSelectedValue(), this->getClassName() + "_" + this->gameObjectPtr->getName() + Ogre::StringConverter::toString(this->index));
		}

		if (nullptr != this->widget)
		{
			this->widget->castType<MyGUI::ListBox>()->eventListMouseItemActivate += MyGUI::newDelegate(this, &MyGUIListBoxComponent::listSelectAccept);
			this->widget->castType<MyGUI::ListBox>()->eventListSelectAccept += MyGUI::newDelegate(this, &MyGUIListBoxComponent::listAccept);
		}

		for (size_t i = 0; i < this->itemCount->getUInt(); i++)
		{
			widget->castType<MyGUI::ListBox>()->addItem(this->items[i]->getString());
		}
		
		this->setColor(this->color->getVector4());
		this->setFontHeight(this->fontHeight->getUInt());
		this->setTextAlign(this->textAlign->getListSelectedValue());
		this->setTextOffset(this->textOffset->getVector2());
		this->setTextColor(this->textColor->getVector4());
		this->setItemHeight(this->itemHeight->getUInt());
		
		return MyGUIComponent::postInit();
	}

	void MyGUIListBoxComponent::mouseButtonClick(MyGUI::Widget* sender)
	{
		if (true == this->isSimulating)
		{
			MyGUI::Button* button = sender->castType<MyGUI::Button>();
			if (nullptr != button)
			{
				// Call also function in lua script, if it does exist in the lua script component
				if (nullptr != this->gameObjectPtr->getLuaScript() && true == this->enabled->getBool())
				{
					if (this->mouseButtonClickClosureFunction.is_valid())
					{
						try
						{
							luabind::call_function<void>(this->mouseButtonClickClosureFunction);
						}
						catch (luabind::error& error)
						{
							luabind::object errorMsg(luabind::from_stack(error.state(), -1));
							std::stringstream msg;
							msg << errorMsg;

							Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnMouseButtonClick' Error: " + Ogre::String(error.what())
																		+ " details: " + msg.str());
						}
					}
				}
			}
		}
	}

	void MyGUIListBoxComponent::listSelectAccept(MyGUI::ListBox* sender, size_t index)
	{
		if (true == this->isSimulating)
		{
			MyGUI::ListBox* listBox = sender->castType<MyGUI::ListBox>();
			if (nullptr != listBox)
			{
				// Call also function in lua script, if it does exist in the lua script component
				if (nullptr != this->gameObjectPtr->getLuaScript() && true == this->enabled->getBool())
				{
					if (this->mouseButtonClickClosureFunction.is_valid())
					{
						try
						{
							luabind::call_function<void>(this->mouseButtonClickClosureFunction, index);
						}
						catch (luabind::error& error)
						{
							luabind::object errorMsg(luabind::from_stack(error.state(), -1));
							std::stringstream msg;
							msg << errorMsg;

							Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnMouseButtonClick' Error: " + Ogre::String(error.what())
																		+ " details: " + msg.str());
						}
					}
				}
			}
		}
	}

	void MyGUIListBoxComponent::listAccept(MyGUI::ListBox* sender, size_t index)
	{
		if (true == this->isSimulating)
		{
			MyGUI::ListBox* listBox = sender->castType<MyGUI::ListBox>();
			if (nullptr != listBox)
			{
				// Call also function in lua script, if it does exist in the lua script component
				if (nullptr != this->gameObjectPtr->getLuaScript() && true == this->enabled->getBool())
				{
					if (this->mouseButtonClickClosureFunction.is_valid())
					{
						try
						{
							luabind::call_function<void>(this->mouseButtonClickClosureFunction, index);
						}
						catch (luabind::error& error)
						{
							luabind::object errorMsg(luabind::from_stack(error.state(), -1));
							std::stringstream msg;
							msg << errorMsg;

							Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnMouseButtonClick' Error: " + Ogre::String(error.what())
																		+ " details: " + msg.str());
						}
					}
				}
			}
		}
	}

	bool MyGUIListBoxComponent::connect(void)
	{
		bool success = MyGUIComponent::connect();

		return success;
	}

	bool MyGUIListBoxComponent::disconnect(void)
	{
		// this->setCaption(this->caption->getString());

		return MyGUIComponent::disconnect();
	}

	void MyGUIListBoxComponent::actualizeValue(Variant* attribute)
	{
		MyGUIComponent::actualizeValue(attribute);
		
		if (MyGUIListBoxComponent::AttrItemHeight() == attribute->getName())
		{
			this->setItemHeight(attribute->getUInt());
		}
		else if (MyGUIListBoxComponent::AttrFontHeight() == attribute->getName())
		{
			this->setFontHeight(attribute->getUInt());
		}
		else if (MyGUIListBoxComponent::AttrTextAlign() == attribute->getName())
		{
			this->setTextAlign(attribute->getListSelectedValue());
		}
		else if (MyGUIListBoxComponent::AttrTextOffset() == attribute->getName())
		{
			this->setTextOffset(attribute->getVector2());
		}
		else if (MyGUIListBoxComponent::AttrTextColor() == attribute->getName())
		{
			this->setTextColor(attribute->getVector4());
		}
		else if (MyGUIListBoxComponent::AttrItemCount() == attribute->getName())
		{
			this->setItemCount(attribute->getUInt());
		}
		else
		{
			for (size_t i = 0; i < this->items.size(); i++)
			{
				if (MyGUIListBoxComponent::AttrItem() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->items[i]->setValue(attribute->getString());
				}
			}
		}
	}

	void MyGUIListBoxComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		MyGUIComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ItemHeight"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->itemHeight->getUInt())));
		propertiesXML->append_node(propertyXML);
			
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FontHeight"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->fontHeight->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TextAlign"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->textAlign->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TextOffset"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->textOffset->getVector2())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TextColor"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->textColor->getVector4())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ItemCount"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->itemCount->getUInt())));
		propertiesXML->append_node(propertyXML);

		for (size_t i = 0; i < this->items.size(); i++)
		{
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "Item" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->items[i]->getString())));
			propertiesXML->append_node(propertyXML);
		}
	}

	Ogre::String MyGUIListBoxComponent::getClassName(void) const
	{
		return "MyGUIListBoxComponent";
	}

	Ogre::String MyGUIListBoxComponent::getParentClassName(void) const
	{
		return "MyGUIComponent";
	}

	void MyGUIListBoxComponent::onChangeSkin(void)
	{
		// this->setItemCount(this->itemCount->getUInt());
	}

	void MyGUIListBoxComponent::setItemHeight(unsigned int itemHeight)
	{
		this->itemHeight->setValue(itemHeight);
		if (nullptr != this->widget)
		{
			// Is the default
			if (itemHeight != 21)
			{
				MyGUI::ListBox* listBox = widget->castType<MyGUI::ListBox>();
				listBox->setItemHeight(itemHeight);
				// Note: Everything will be reset
				this->widget->changeWidgetSkin(skin->getListSelectedValue());
				for (size_t i = 0; i < this->items.size(); i++)
				{
					widget->castType<MyGUI::ListBox>()->addItem(this->items[i]->getString());
				}
			
				this->setActivated(this->activated->getBool());
				this->setColor(this->color->getVector4());
				this->setFontHeight(this->fontHeight->getUInt());
				this->setTextAlign(this->textAlign->getListSelectedValue());
				this->setTextOffset(this->textOffset->getVector2());
				this->setTextColor(this->textColor->getVector4());
			}
		}
	}

	unsigned int MyGUIListBoxComponent::getItemHeight(void) const
	{
		return this->itemHeight->getUInt();
	}

	void MyGUIListBoxComponent::setFontHeight(unsigned int fontHeight)
	{
		this->fontHeight->setValue(fontHeight);

		if (nullptr != this->widget)
		{
			MyGUI::ListBox* listBox = widget->castType<MyGUI::ListBox>();
			for (size_t i = 0; i < this->items.size(); i++)
			{
				fontHeight = MyGUIUtilities::getInstance()->setFontSize(listBox->getWidgetByIndex(i)->castType<MyGUI::TextBox>(false), fontHeight);
			}
			this->fontHeight->setValue(fontHeight);
		}
	}

	unsigned int MyGUIListBoxComponent::getFontHeight(void) const
	{
		return this->fontHeight->getUInt();
	}

	void MyGUIListBoxComponent::setTextAlign(const Ogre::String & align)
	{
		this->textAlign->setListSelectedValue(align);
		if (nullptr != this->widget)
		{
			MyGUI::ListBox* listBox = widget->castType<MyGUI::ListBox>();
			for (size_t i = 0; i < this->items.size(); i++)
			{
				listBox->getWidgetByIndex(i)->castType<MyGUI::TextBox>(false)->setTextAlign(this->mapStringToAlign(align));
			}
		}
	}

	Ogre::String MyGUIListBoxComponent::getTextAlign(void) const
	{
		return this->textAlign->getListSelectedValue();
	}

	void MyGUIListBoxComponent::setTextOffset(const Ogre::Vector2& textOffset)
	{
		this->textOffset->setValue(textOffset);
		if (nullptr != this->widget)
		{
			MyGUI::ListBox* listBox = widget->castType<MyGUI::ListBox>();
			for (size_t i = 0; i < this->items.size(); i++)
			{
				listBox->getWidgetByIndex(i)->castType<MyGUI::TextBox>(false)->setTextOffset(MyGUI::IntPoint(textOffset.x, textOffset.y));
			}
		}
	}

	Ogre::Vector2 MyGUIListBoxComponent::getTextOffset(void) const
	{
		return this->textOffset->getVector2();
	}
	
	void MyGUIListBoxComponent::setTextColor(const Ogre::Vector4& textColor)
	{
		this->textColor->setValue(textColor);
		if (nullptr != this->widget)
		{
			MyGUI::ListBox* listBox = widget->castType<MyGUI::ListBox>();
			for (size_t i = 0; i < this->items.size(); i++)
			{
				listBox->getWidgetByIndex(i)->castType<MyGUI::TextBox>(false)->setTextColour(MyGUI::Colour(textColor.x, textColor.y, textColor.z, textColor.w));
			}
		}
	}
	
	Ogre::Vector4 MyGUIListBoxComponent::getTextColor(void) const
	{
		return this->textColor->getVector4();
	}

	void MyGUIListBoxComponent::reactOnSelected(luabind::object closureFunction)
	{
		this->selectedClosureFunction = closureFunction;
	}
	
	void MyGUIListBoxComponent::reactOnAccept(luabind::object closureFunction)
	{
		this->acceptClosureFunction = closureFunction;
	}

	void MyGUIListBoxComponent::setItemCount(unsigned int itemCount)
	{
		this->itemCount->setValue(itemCount);

		size_t oldSize = this->items.size();

		if (itemCount > oldSize)
		{
			this->items.resize(itemCount);

			for (size_t i = oldSize; i < this->items.size(); i++)
			{
				this->items[i] = new Variant(MyGUIListBoxComponent::AttrItem() + Ogre::StringConverter::toString(i), "", this->attributes);
				this->items[i]->addUserData(GameObject::AttrActionSeparator());

				if (nullptr != this->widget)
				{
					widget->castType<MyGUI::ListBox>()->addItem("");
				}
			}

			this->setColor(this->color->getVector4());
			this->setItemHeight(this->itemHeight->getUInt());
			this->setFontHeight(this->fontHeight->getUInt());
			this->setTextAlign(this->textAlign->getListSelectedValue());
			this->setTextOffset(this->textOffset->getVector2());
			this->setTextColor(this->textColor->getVector4());
		}
		else if (itemCount < oldSize)
		{
			this->eraseVariants(this->items, itemCount);
			if (nullptr != this->widget)
			{
				for (size_t i = itemCount; i < oldSize; i++)
				{
					// 0 is correct! MyGUI removes each time the 0-th index.
					widget->castType<MyGUI::ListBox>()->removeItemAt(0);
				}
			}
		}
	}
		
	unsigned int MyGUIListBoxComponent::getItemCount(void) const
	{
		return this->itemCount->getUInt();
	}
	
	void MyGUIListBoxComponent::setItemText(unsigned int index, const Ogre::String& itemText)
	{
		if (index >= this->items.size())
			index = static_cast<unsigned int>(this->items.size()) - 1;
		this->items[index]->setValue(itemText);

		if (nullptr != this->widget)
		{
			widget->castType<MyGUI::ListBox>()->setItemNameAt(index, MyGUI::LanguageManager::getInstancePtr()->replaceTags(itemText));
		}
	}
	
	Ogre::String MyGUIListBoxComponent::getItemText(unsigned int index) const
	{
		if (index > this->items.size())
			return "";
		return this->items[index]->getString();
	}

	void MyGUIListBoxComponent::addItem(const Ogre::String& itemText)
	{
		this->itemCount->setValue(this->itemCount->getUInt() + 1);
		unsigned int count = this->itemCount->getUInt();

		this->items.resize(count);
		this->items[count - 1] = new Variant(MyGUIListBoxComponent::AttrItem() + Ogre::StringConverter::toString(count - 1), itemText, this->attributes);
		this->items[count - 1]->addUserData(GameObject::AttrActionSeparator());

		if (nullptr != this->widget)
		{
			widget->castType<MyGUI::ListBox>()->addItem(itemText);
			this->setColor(this->color->getVector4());
			this->setItemHeight(this->itemHeight->getUInt());
			this->setFontHeight(this->fontHeight->getUInt());
			this->setTextAlign(this->textAlign->getListSelectedValue());
			this->setTextOffset(this->textOffset->getVector2());
			this->setTextColor(this->textColor->getVector4());
		}
	}

	void MyGUIListBoxComponent::insertItem(int index, const Ogre::String& itemText)
	{
		if (index > this->items.size())
			index = static_cast<unsigned int>(this->items.size()) - 1;

		this->itemCount->setValue(this->itemCount->getUInt() + 1);

		Variant* newItem = new Variant(MyGUIListBoxComponent::AttrItem() + Ogre::StringConverter::toString(index), itemText, this->attributes);
		
		this->items.insert(this->items.begin() + index, newItem);
		this->items[this->items.size() - 1]->addUserData(GameObject::AttrActionSeparator());

		if (nullptr != this->widget)
		{
			widget->castType<MyGUI::ListBox>()->insertItemAt(index, itemText);
			this->setColor(this->color->getVector4());
			this->setItemHeight(this->itemHeight->getUInt());
			this->setFontHeight(this->fontHeight->getUInt());
			this->setTextAlign(this->textAlign->getListSelectedValue());
			this->setTextOffset(this->textOffset->getVector2());
			this->setTextColor(this->textColor->getVector4());
		}
	}

	void MyGUIListBoxComponent::removeItem(unsigned int index)
	{
		if (index > 0 && index < this->itemCount->getUInt())
		{
			this->eraseVariant(this->items, index);
			this->itemCount->setValue(this->itemCount->getUInt() - 1);
			if (nullptr != this->widget)
			{
				widget->castType<MyGUI::ListBox>()->removeItemAt(index);
			}
		}
	}

	int MyGUIListBoxComponent::getSelectedIndex(void) const
	{
		if (nullptr != this->widget)
		{
			size_t index = widget->castType<MyGUI::ListBox>()->getIndexSelected();
			// Lua would not understand size_t max, because it would convert this to scientific number with e, so set to -1
			if (MyGUI::ITEM_NONE == index)
				return -1;
			return static_cast<int>(index);
		}
		return -1;
	}
	
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MyGUIComboBoxComponent::MyGUIComboBoxComponent()
		: MyGUIComponent(),
		itemHeight(new Variant(MyGUIComboBoxComponent::AttrItemHeight(), static_cast<unsigned int>(21), this->attributes)),
		fontHeight(new Variant(MyGUIComboBoxComponent::AttrFontHeight(), static_cast<unsigned int>(20), this->attributes)),
		textAlign(new Variant(MyGUIComboBoxComponent::AttrTextAlign(), { "HCenter", "Center", "Left", "Right", "HStretch", "Top", "Bottom", "VStretch", "Stretch", "Default" }, this->attributes)),
		textOffset(new Variant(MyGUIComboBoxComponent::AttrTextOffset(), Ogre::Vector2(0.0f, 0.0f), this->attributes)),
		textColor(new Variant(MyGUIComboBoxComponent::AttrTextColor(), Ogre::Vector4(0.0f, 0.0f, 0.0, 1.0f), this->attributes)),
		modeDrop(new Variant(MyGUIComboBoxComponent::AttrModeDrop(), true, this->attributes)),
		smooth(new Variant(MyGUIComboBoxComponent::AttrSmooth(), true, this->attributes)),
		flowDirection(new Variant(MyGUIComboBoxComponent::AttrFlowDirection(), { "TopToBottom", "BottomToTop", "LeftToRight", "RightToLeft" }, this->attributes)),
		itemCount(new Variant(MyGUIComboBoxComponent::AttrItemCount(), 0, this->attributes))
	{
		std::vector<Ogre::String> skins({ "ComboBox" });
		this->skin = new Variant(MyGUIComponent::AttrSkin(), skins, this->attributes);
		this->skin->setVisible(false);
		
		this->textColor->addUserData(GameObject::AttrActionColorDialog());
		this->itemCount->addUserData(GameObject::AttrActionNeedRefresh());
	}

	MyGUIComboBoxComponent::~MyGUIComboBoxComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUIComboBoxComponent] Destructor MyGUI combo box component for game object: " + this->gameObjectPtr->getName());
	}

	bool MyGUIComboBoxComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = MyGUIComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ItemHeight")
		{
			this->setItemHeight(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FontHeight")
		{
			this->setFontHeight(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TextAlign")
		{
			this->setTextAlign(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TextOffset")
		{
			this->setTextOffset(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TextColor")
		{
			this->setTextColor(XMLConverter::getAttribVector4(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ModeDrop")
		{
			this->modeDrop->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Smooth")
		{
			this->smooth->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "FlowDirection")
		{
			this->flowDirection->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ItemCount")
		{
			this->itemCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		this->items.resize(this->itemCount->getUInt());

		for (size_t i = 0; i < this->items.size(); i++)
		{
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Item" + Ogre::StringConverter::toString(i))
			{
				this->items[i] = new Variant(MyGUIComboBoxComponent::AttrItem() + Ogre::StringConverter::toString(i), 
					XMLConverter::getAttrib(propertyElement, "data"), this->attributes);
				propertyElement = propertyElement->next_sibling("property");
				this->items[i]->addUserData(GameObject::AttrActionSeparator());
			}
		}
		
		return success;
	}

	GameObjectCompPtr MyGUIComboBoxComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		MyGUIComboBoxCompPtr clonedCompPtr(boost::make_shared<MyGUIComboBoxComponent>());

		
		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setRealPosition(this->position->getVector2());
		clonedCompPtr->setRealSize(this->size->getVector2());
		clonedCompPtr->setAlign(this->align->getListSelectedValue());
		clonedCompPtr->setLayer(this->layer->getListSelectedValue());
		clonedCompPtr->setColor(this->color->getVector4());
		clonedCompPtr->setEnabled(this->enabled->getBool());
		clonedCompPtr->setEnabled(this->enabled->getBool());
		clonedCompPtr->internalSetPriorId(this->id->getULong());
		clonedCompPtr->setParentId(this->parentId->getULong()); // Attention
		clonedCompPtr->setSkin(this->skin->getListSelectedValue());
		

		clonedCompPtr->setItemCount(this->itemCount->getUInt());
		for (unsigned int i = 0; i < this->itemCount->getUInt(); i++)
		{
			clonedCompPtr->setItemText(i, this->items[i]->getString());
		}

		clonedCompPtr->setItemHeight(this->itemHeight->getUInt());
		clonedCompPtr->setFontHeight(this->fontHeight->getUInt());
		clonedCompPtr->setTextAlign(this->align->getListSelectedValue());
		clonedCompPtr->setTextOffset(this->textOffset->getVector2());
		clonedCompPtr->setTextColor(this->textColor->getVector4());
		clonedCompPtr->setSmooth(this->smooth->getBool());
		clonedCompPtr->setModeDrop(this->modeDrop->getBool());
		clonedCompPtr->setFlowDirection(this->flowDirection->getListSelectedValue());
		
		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool MyGUIComboBoxComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUIComboBoxComponent] Init MyGUI combo box component for game object: " + this->gameObjectPtr->getName());
		
		if (true == this->createWidgetInParent)
		{
			this->widget = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::ComboBox>(this->skin->getListSelectedValue(),
				this->position->getVector2().x, this->position->getVector2().y, this->size->getVector2().x, this->size->getVector2().y,
				this->mapStringToAlign(this->align->getListSelectedValue()), this->layer->getListSelectedValue(), this->getClassName() + "_" + this->gameObjectPtr->getName() + Ogre::StringConverter::toString(this->index));
		}
		// this->setActivated(this->activated->getBool());

		if (nullptr != this->widget)
		{
			widget->castType<MyGUI::ComboBox>()->eventComboAccept += MyGUI::newDelegate(this, &MyGUIComboBoxComponent::comboAccept);
		}

		for (size_t i = 0; i < this->itemCount->getUInt(); i++)
		{
			widget->castType<MyGUI::ComboBox>()->addItem(this->items[i]->getString());
		}
		
		this->setColor(this->color->getVector4());
		this->setFontHeight(this->fontHeight->getUInt());
		this->setTextAlign(this->textAlign->getListSelectedValue());
		this->setTextOffset(this->textOffset->getVector2());
		this->setTextColor(this->textColor->getVector4());
	
		this->setModeDrop(this->modeDrop->getBool());
		this->setSmooth(this->smooth->getBool());
		this->setFlowDirection(this->flowDirection->getListSelectedValue());
		this->setItemHeight(this->itemHeight->getUInt());

		return MyGUIComponent::postInit();
	}

	void MyGUIComboBoxComponent::mouseButtonClick(MyGUI::Widget* sender)
	{
		if (true == this->isSimulating)
		{
			MyGUI::Button* button = sender->castType<MyGUI::Button>();
			if (nullptr != button)
			{
				// Call also function in lua script, if it does exist in the lua script component
				if (nullptr != this->gameObjectPtr->getLuaScript() && true == this->enabled->getBool())
				{
					if (this->mouseButtonClickClosureFunction.is_valid())
					{
						try
						{
							luabind::call_function<void>(this->mouseButtonClickClosureFunction);
						}
						catch (luabind::error& error)
						{
							luabind::object errorMsg(luabind::from_stack(error.state(), -1));
							std::stringstream msg;
							msg << errorMsg;

							Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnMouseButtonClick' Error: " + Ogre::String(error.what())
																		+ " details: " + msg.str());
						}
					}
				}
			}
		}
	}

	void MyGUIComboBoxComponent::comboAccept(MyGUI::ComboBox* sender, size_t index)
	{
		if (true == this->isSimulating)
		{
			MyGUI::ComboBox* comboBox = sender->castType<MyGUI::ComboBox>();
			if (nullptr != comboBox)
			{
				// Call also function in lua script, if it does exist in the lua script component
				if (nullptr != this->gameObjectPtr->getLuaScript() && true == this->enabled->getBool())
				{
					if (this->selectedClosureFunction.is_valid())
					{
						try
						{
							luabind::call_function<void>(this->selectedClosureFunction, index);
						}
						catch (luabind::error& error)
						{
							luabind::object errorMsg(luabind::from_stack(error.state(), -1));
							std::stringstream msg;
							msg << errorMsg;

							Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnSelected' Error: " + Ogre::String(error.what())
																		+ " details: " + msg.str());
						}
					}
				}
			}
		}
	}

	bool MyGUIComboBoxComponent::connect(void)
	{
		bool success = MyGUIComponent::connect();

		return success;
	}

	bool MyGUIComboBoxComponent::disconnect(void)
	{
		// this->setCaption(this->caption->getString());

		return MyGUIComponent::disconnect();
	}

	void MyGUIComboBoxComponent::actualizeValue(Variant* attribute)
	{
		MyGUIComponent::actualizeValue(attribute);
		
		if (MyGUIComboBoxComponent::AttrItemHeight() == attribute->getName())
		{
			this->setItemHeight(attribute->getUInt());
		}
		else if (MyGUIComboBoxComponent::AttrFontHeight() == attribute->getName())
		{
			this->setFontHeight(attribute->getUInt());
		}
		else if (MyGUIComboBoxComponent::AttrTextAlign() == attribute->getName())
		{
			this->setTextAlign(attribute->getListSelectedValue());
		}
		else if (MyGUIComboBoxComponent::AttrTextOffset() == attribute->getName())
		{
			this->setTextOffset(attribute->getVector2());
		}
		else if (MyGUIComboBoxComponent::AttrTextColor() == attribute->getName())
		{
			this->setTextColor(attribute->getVector4());
		}
		if (MyGUIComboBoxComponent::AttrModeDrop() == attribute->getName())
		{
			this->setModeDrop(attribute->getBool());
		}
		else if (MyGUIComboBoxComponent::AttrSmooth() == attribute->getName())
		{
			this->setSmooth(attribute->getBool());
		}
		else if (MyGUIComboBoxComponent::AttrFlowDirection() == attribute->getName())
		{
			this->setFlowDirection(attribute->getListSelectedValue());
		}
		else if (MyGUIComboBoxComponent::AttrItemCount() == attribute->getName())
		{
			this->setItemCount(attribute->getUInt());
		}
		else
		{
			for (size_t i = 0; i < this->items.size(); i++)
			{
				if (MyGUIComboBoxComponent::AttrItem() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->items[i]->setValue(attribute->getString());
				}
			}
		}
	}

	void MyGUIComboBoxComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		MyGUIComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ItemHeight"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->itemHeight->getUInt())));
		propertiesXML->append_node(propertyXML);
			
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FontHeight"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->fontHeight->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TextAlign"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->textAlign->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TextOffset"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->textOffset->getVector2())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TextColor"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->textColor->getVector4())));
		propertiesXML->append_node(propertyXML);
			
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ModeDrop"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->modeDrop->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Smooth"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->smooth->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "FlowDirection"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->flowDirection->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ItemCount"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->itemCount->getUInt())));
		propertiesXML->append_node(propertyXML);
	
		for (size_t i = 0; i < this->items.size(); i++)
		{
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "Item" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->items[i]->getString())));
			propertiesXML->append_node(propertyXML);
		}
	}

	Ogre::String MyGUIComboBoxComponent::getClassName(void) const
	{
		return "MyGUIComboBoxComponent";
	}

	Ogre::String MyGUIComboBoxComponent::getParentClassName(void) const
	{
		return "MyGUIComponent";
	}

	void MyGUIComboBoxComponent::onChangeSkin(void)
	{
		
	}

	void MyGUIComboBoxComponent::setItemHeight(unsigned int itemHeight)
	{
		this->itemHeight->setValue(itemHeight);
		if (nullptr != this->widget)
		{
			// Is the default
			if (itemHeight != 21)
			{
				MyGUI::ComboBox* comboBox = widget->castType<MyGUI::ComboBox>();
				comboBox->setItemHeight(itemHeight);
				// Note: Everything will be reset
				this->widget->changeWidgetSkin(skin->getListSelectedValue());
				for (size_t i = 0; i < this->items.size(); i++)
				{
					widget->castType<MyGUI::ComboBox>()->addItem(this->items[i]->getString());
				}
			
				this->setActivated(this->activated->getBool());
				this->setColor(this->color->getVector4());
				this->setFontHeight(this->fontHeight->getUInt());
				this->setTextAlign(this->textAlign->getListSelectedValue());
				this->setTextOffset(this->textOffset->getVector2());
				this->setTextColor(this->textColor->getVector4());
				this->setModeDrop(this->modeDrop->getBool());
				this->setSmooth(this->smooth->getBool());
				this->setFlowDirection(this->flowDirection->getListSelectedValue());
				this->setItemCount(this->itemCount->getUInt());
			}
		}
	}

	unsigned int MyGUIComboBoxComponent::getItemHeight(void) const
	{
		return this->itemHeight->getUInt();
	}

	void MyGUIComboBoxComponent::setFontHeight(unsigned int fontHeight)
	{
		this->fontHeight->setValue(fontHeight);

		if (nullptr != this->widget)
		{
			MyGUI::ComboBox* comboBox = widget->castType<MyGUI::ComboBox>();
			for (size_t i = 0; i < this->items.size(); i++)
			{
				fontHeight = MyGUIUtilities::getInstance()->setFontSize(comboBox->getList()->getWidgetByIndex(i)->castType<MyGUI::ComboBox>(), fontHeight);
			}
			this->fontHeight->setValue(fontHeight);
		}
	}

	unsigned int MyGUIComboBoxComponent::getFontHeight(void) const
	{
		return this->fontHeight->getUInt();
	}

	void MyGUIComboBoxComponent::setTextAlign(const Ogre::String & align)
	{
		this->textAlign->setListSelectedValue(align);
		if (nullptr != this->widget)
		{
			MyGUI::ComboBox* comboBox = widget->castType<MyGUI::ComboBox>();
			for (size_t i = 0; i < this->items.size(); i++)
			{
				comboBox->getList()->getWidgetByIndex(i)->castType<MyGUI::ComboBox>()->setTextAlign(this->mapStringToAlign(align));
			}
		}
	}

	Ogre::String MyGUIComboBoxComponent::getTextAlign(void) const
	{
		return this->textAlign->getListSelectedValue();
	}

	void MyGUIComboBoxComponent::setTextOffset(const Ogre::Vector2& textOffset)
	{
		this->textOffset->setValue(textOffset);
		if (nullptr != this->widget)
		{
			MyGUI::ComboBox* comboBox = widget->castType<MyGUI::ComboBox>();
			for (size_t i = 0; i < this->items.size(); i++)
			{
				comboBox->getList()->getWidgetByIndex(i)->castType<MyGUI::ComboBox>()->setTextOffset(MyGUI::IntPoint(textOffset.x, textOffset.y));
			}
		}
	}

	Ogre::Vector2 MyGUIComboBoxComponent::getTextOffset(void) const
	{
		return this->textOffset->getVector2();
	}
	
	void MyGUIComboBoxComponent::setTextColor(const Ogre::Vector4& textColor)
	{
		this->textColor->setValue(textColor);
		if (nullptr != this->widget)
		{
			MyGUI::ComboBox* comboBox = widget->castType<MyGUI::ComboBox>();
			for (size_t i = 0; i < this->items.size(); i++)
			{
				comboBox->getList()->getWidgetByIndex(i)->castType<MyGUI::ComboBox>()->setTextColour(MyGUI::Colour(textColor.x, textColor.y, textColor.z, textColor.w));
			}
		}
	}
	
	Ogre::Vector4 MyGUIComboBoxComponent::getTextColor(void) const
	{
		return this->textColor->getVector4();
	}
	
	void MyGUIComboBoxComponent::setModeDrop(bool modeDrop)
	{
		this->modeDrop->setValue(modeDrop);
		if (nullptr != this->widget)
		{
			widget->castType<MyGUI::ComboBox>()->setComboModeDrop(modeDrop);
		}
	}

	bool MyGUIComboBoxComponent::getModeDrop(void) const
	{
		return this->modeDrop->getBool();
	}

	void MyGUIComboBoxComponent::setSmooth(bool smooth)
	{
		this->smooth->setValue(smooth);
		if (nullptr != this->widget)
		{
			widget->castType<MyGUI::ComboBox>()->setSmoothShow(smooth);
		}
	}

	bool MyGUIComboBoxComponent::getSmooth(void) const
	{
		return this->smooth->getBool();
	}

	void MyGUIComboBoxComponent::setFlowDirection(const Ogre::String& flowDirection)
	{
		this->flowDirection->setListSelectedValue(flowDirection);
		if (nullptr != this->widget)
		{
			if ("LeftToRight" == flowDirection)
				widget->castType<MyGUI::ComboBox>()->setFlowDirection(MyGUI::FlowDirection::LeftToRight);
			else if ("RightToLeft" == flowDirection)
				widget->castType<MyGUI::ComboBox>()->setFlowDirection(MyGUI::FlowDirection::RightToLeft);
			else if ("TopToBottom" == flowDirection)
				widget->castType<MyGUI::ComboBox>()->setFlowDirection(MyGUI::FlowDirection::TopToBottom);
			else if ("BottomToTop" == flowDirection)
				widget->castType<MyGUI::ComboBox>()->setFlowDirection(MyGUI::FlowDirection::BottomToTop);
		}
	}

	Ogre::String MyGUIComboBoxComponent::getFlowDirection(void) const
	{
		return this->flowDirection->getListSelectedValue();
	}

	void MyGUIComboBoxComponent::setItemCount(unsigned int itemCount)
	{
		this->itemCount->setValue(itemCount);

		size_t oldSize = this->items.size();

		if (itemCount > oldSize)
		{
			this->items.resize(itemCount);

			for (size_t i = oldSize; i < this->items.size(); i++)
			{
				this->items[i] = new Variant(MyGUIComboBoxComponent::AttrItem() + Ogre::StringConverter::toString(i), "", this->attributes);
				this->items[i]->addUserData(GameObject::AttrActionSeparator());

				if (nullptr != this->widget)
				{
					widget->castType<MyGUI::ComboBox>()->addItem("");
				}
			}
		}
		else if (itemCount < oldSize)
		{
			this->eraseVariants(this->items, itemCount);
			if (nullptr != this->widget)
			{
				for (size_t i = itemCount; i < oldSize; i++)
				{
					// 0 is correct! MyGUI removes each time the 0-th index.
					widget->castType<MyGUI::ComboBox>()->removeItemAt(0);
				}
			}
		}
	}
		
	unsigned int MyGUIComboBoxComponent::getItemCount(void) const
	{
		return this->itemCount->getUInt();
	}
	
	void MyGUIComboBoxComponent::setItemText(unsigned int index, const Ogre::String& itemText)
	{
		if (index >= this->items.size())
			index = static_cast<unsigned int>(this->items.size()) - 1;

		this->items[index]->setValue(itemText);
		if (nullptr != this->widget)
		{
			widget->castType<MyGUI::ComboBox>()->setItemNameAt(index, MyGUI::LanguageManager::getInstancePtr()->replaceTags(itemText));
		}
	}
	
	Ogre::String MyGUIComboBoxComponent::getItemText(unsigned int index) const
	{
		if (index > this->items.size())
			return "";
		return this->items[index]->getString();
	}

	void MyGUIComboBoxComponent::addItem(const Ogre::String& itemText)
	{
		this->itemCount->setValue(this->itemCount->getUInt() + 1);
		unsigned int count = this->itemCount->getUInt();

		this->items.resize(count);
		this->items[count - 1] = new Variant(MyGUIComboBoxComponent::AttrItem() + Ogre::StringConverter::toString(count - 1), itemText, this->attributes);
		this->items[count - 1]->addUserData(GameObject::AttrActionSeparator());

		if (nullptr != this->widget)
		{
			widget->castType<MyGUI::ComboBox>()->addItem(itemText);
		}
	}

	void MyGUIComboBoxComponent::insertItem(int index, const Ogre::String& itemText)
	{
		if (index > this->items.size())
			index = static_cast<unsigned int>(this->items.size()) - 1;

		this->itemCount->setValue(this->itemCount->getUInt() + 1);

		Variant* newItem = new Variant(MyGUIComboBoxComponent::AttrItem() + Ogre::StringConverter::toString(index), itemText, this->attributes);
		
		this->items.insert(this->items.begin() + index, newItem);
		this->items[this->items.size() - 1]->addUserData(GameObject::AttrActionSeparator());

		if (nullptr != this->widget)
		{
			widget->castType<MyGUI::ComboBox>()->insertItemAt(index, itemText);
			this->setColor(this->color->getVector4());
			this->setItemHeight(this->itemHeight->getUInt());
			this->setFontHeight(this->fontHeight->getUInt());
			this->setTextAlign(this->textAlign->getListSelectedValue());
			this->setTextOffset(this->textOffset->getVector2());
			this->setTextColor(this->textColor->getVector4());
		}
	}

	void MyGUIComboBoxComponent::removeItem(unsigned int index)
	{
		if (index > 0 && index < this->itemCount->getUInt())
		{
			this->eraseVariant(this->items, index);
			this->itemCount->setValue(this->itemCount->getUInt() - 1);
			if (nullptr != this->widget)
			{
				widget->castType<MyGUI::ComboBox>()->removeItemAt(index);
			}
		}
	}

	int MyGUIComboBoxComponent::getSelectedIndex(void) const
	{
		if (nullptr != this->widget)
		{
			size_t index = widget->castType<MyGUI::ComboBox>()->getIndexSelected();
			// Lua would not understand size_t max, because it would convert this to scientific number with e, so set to -1
			if (MyGUI::ITEM_NONE == index)
				return -1;
			return static_cast<int>(index);
		}
		return -1;
	}

	void MyGUIComboBoxComponent::reactOnSelected(luabind::object closureFunction)
	{
		this->selectedClosureFunction = closureFunction;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MyGUIMessageBoxComponent::MyGUIMessageBoxComponent()
		: GameObjectComponent(),
		isInSimulation(false),
		messageBox(nullptr)
	{
		this->activated = new Variant(MyGUIMessageBoxComponent::AttrActivated(), false, this->attributes);
		this->title = new Variant(MyGUIMessageBoxComponent::AttrTitle(), "My Title", this->attributes);
		this->message = new Variant(MyGUIMessageBoxComponent::AttrMessage(), "My Message", this->attributes);
		this->resultEventName = new Variant(MyGUIMessageBoxComponent::AttrResultEventName(), Ogre::String(""), this->attributes);
		this->stylesCount = new Variant(MyGUIMessageBoxComponent::AttrStylesCount(), 0, this->attributes);
		// std::vector<Ogre::String> skins({ });
		// this->skin = new Variant(MyGUIComponent::AttrSkin(), skins, this->attributes);
		
		this->stylesCount->addUserData(GameObject::AttrActionNeedRefresh());
		this->resultEventName->setDescription("Sets the message box end function name, because several MyGUI Components can have an event in the same lua script, so name is necessary. E.g. onMessageBoxOverwriteEnd(myGUIMessageBoxComponent, result)."
													"result can be used as follows: if (result == MessageBoxStyle.Yes) then ...");
	}

	MyGUIMessageBoxComponent::~MyGUIMessageBoxComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUIMessageBoxComponent] Destructor MyGUI message box component for game object: " + this->gameObjectPtr->getName());

		if (nullptr != this->messageBox)
		{
			this->messageBox->endMessage();
			this->messageBox = nullptr;
		}
	}

	bool MyGUIMessageBoxComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = GameObjectComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Title")
		{
			this->title->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Message")
		{
			this->message->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ResultEventName")
		{
			this->setResultEventName(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "StylesCount")
		{
			this->stylesCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		this->styles.resize(this->stylesCount->getUInt());

		for (size_t i = 0; i < this->styles.size(); i++)
		{
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Style" + Ogre::StringConverter::toString(i))
			{
				Ogre::String stylesName = XMLConverter::getAttrib(propertyElement, "data");

				this->styles[i] = new Variant(MyGUIMessageBoxComponent::AttrStyle() + Ogre::StringConverter::toString(i), std::vector<Ogre::String>(), this->attributes);
				this->styles[i]->setListSelectedValue(stylesName);
				propertyElement = propertyElement->next_sibling("property");

				this->styles[i]->addUserData(GameObject::AttrActionSeparator());
			}
		}
		
		return success;
	}

	GameObjectCompPtr MyGUIMessageBoxComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		return nullptr;
	}

	bool MyGUIMessageBoxComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUIMessageBoxComponent] Init MyGUI combo box component for game object: " + this->gameObjectPtr->getName());

		std::vector<Ogre::String> stylesNames = { "Ok", "Yes", "No", "Abord", "Retry", "Ignore", "Cancel", "Try", "Continue", "IconInfo", "IconQuest", "IconError", "IconWarning" };

		for (size_t i = 0; i < this->styles.size(); i++)
		{
			// First get selected style name
			Ogre::String selectedStyleName = this->styles[i]->getListSelectedValue();
			// Fill list
			this->styles[i]->setValue(stylesNames);
			// Reset selected style name
			this->styles[i]->setListSelectedValue(selectedStyleName);
		}

		return true;
	}

	void MyGUIMessageBoxComponent::update(Ogre::Real dt, bool notSimulating)
	{
		this->isInSimulation = !notSimulating;
	}

	bool MyGUIMessageBoxComponent::connect(void)
	{
		bool success = GameObjectComponent::connect();

		this->setActivated(this->activated->getBool());

		return success;
	}

	bool MyGUIMessageBoxComponent::disconnect(void)
	{
		this->setActivated(false);

		return GameObjectComponent::disconnect();
	}

	void MyGUIMessageBoxComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);
		
		if (MyGUIMessageBoxComponent::AttrActivated() == attribute->getName())
		{
			this->activated->setValue(attribute->getBool());
		}
		else if (MyGUIMessageBoxComponent::AttrTitle() == attribute->getName())
		{
			this->title->setValue(attribute->getString());
		}
		else if (MyGUIMessageBoxComponent::AttrMessage() == attribute->getName())
		{
			this->message->setValue(attribute->getString());
		}
		else if (MyGUIMessageBoxComponent::AttrResultEventName() == attribute->getName())
		{
			this->setResultEventName(attribute->getString());
		}
		else if (MyGUIMessageBoxComponent::AttrStylesCount() == attribute->getName())
		{
			this->setStylesCount(attribute->getUInt());
		}
		else
		{
			for (size_t i = 0; i < this->styles.size(); i++)
			{
				if (MyGUIMessageBoxComponent::AttrStyle() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->styles[i]->setValue(attribute->getListSelectedValue());
				}
			}
		}
	}

	void MyGUIMessageBoxComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		GameObjectComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Title"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->title->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Message"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->message->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ResultEventName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->resultEventName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "StylesCount"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->stylesCount->getUInt())));
		propertiesXML->append_node(propertyXML);
	

		for (size_t i = 0; i < this->styles.size(); i++)
		{
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "Style" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->styles[i]->getListSelectedValue())));
			propertiesXML->append_node(propertyXML);
		}
	}

	void MyGUIMessageBoxComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
		if (true == this->isInSimulation)
		{
			if (true == activated)
			{
				MyGUI::MessageBoxStyle tempStyle;

				for (size_t i = 0; i < this->styles.size(); i++)
				{
					Ogre::String style = this->styles[i]->getListSelectedValue();
					if ("Ok" == style)
						tempStyle |= MyGUI::MessageBoxStyle::Ok;
					else if ("Yes" == style)
						tempStyle |= MyGUI::MessageBoxStyle::Yes;
					else if ("No" == style)
						tempStyle |= MyGUI::MessageBoxStyle::No;
					else if ("Abort" == style)
						tempStyle |= MyGUI::MessageBoxStyle::Abort;
					else if ("Retry" == style)
						tempStyle |= MyGUI::MessageBoxStyle::Retry;
					else if ("Ignore" == style)
						tempStyle |= MyGUI::MessageBoxStyle::Ignore;
					else if ("Cancel" == style)
						tempStyle |= MyGUI::MessageBoxStyle::Cancel;
					else if ("Try" == style)
						tempStyle |= MyGUI::MessageBoxStyle::Try;
					else if ("Continue" == style)
						tempStyle |= MyGUI::MessageBoxStyle::Continue;

					else if ("IconInfo" == style)
						tempStyle |= MyGUI::MessageBoxStyle::IconInfo;
					else if ("IconQuest" == style)
						tempStyle |= MyGUI::MessageBoxStyle::IconQuest;
					else if ("IconError" == style)
						tempStyle |= MyGUI::MessageBoxStyle::IconError;
					else if ("IconWarning" == style)
						tempStyle |= MyGUI::MessageBoxStyle::IconWarning;
					else
						tempStyle |= MyGUI::MessageBoxStyle::IconDefault;
				}

				this->messageBox = MyGUI::Message::createMessageBox(MyGUI::LanguageManager::getInstancePtr()->replaceTags(this->title->getString()), 
				MyGUI::LanguageManager::getInstancePtr()->replaceTags(this->message->getString()), tempStyle, "Popup", true);

				messageBox->eventMessageBoxResult += MyGUI::newDelegate(this, &MyGUIMessageBoxComponent::notifyMessageBoxEnd);
			}
		}

		if (false == activated)
		{
			if (nullptr != this->messageBox)
			{
				this->messageBox->endMessage();
			}
		}
	}

	bool MyGUIMessageBoxComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	Ogre::String MyGUIMessageBoxComponent::getClassName(void) const
	{
		return "MyGUIMessageBoxComponent";
	}

	Ogre::String MyGUIMessageBoxComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}
	
	void MyGUIMessageBoxComponent::setTitle(const Ogre::String& title)
	{
		this->title->setValue(title);
	}

	Ogre::String MyGUIMessageBoxComponent::getTitle(void) const
	{
		return this->title->getString();
	}

	void MyGUIMessageBoxComponent::setMessage(const Ogre::String& message)
	{
		this->message->setValue(message);
	}

	Ogre::String MyGUIMessageBoxComponent::getMessage(void) const
	{
		return this->message->getString();
	}

	void MyGUIMessageBoxComponent::setResultEventName(const Ogre::String& resultEventName)
	{
		this->resultEventName->setValue(resultEventName);
		this->resultEventName->addUserData(GameObject::AttrActionGenerateLuaFunction(), resultEventName + "(thisComponent, result)=" + this->getClassName());
	}

	Ogre::String MyGUIMessageBoxComponent::getResultEventName(void) const
	{
		return this->resultEventName->getString();
	}

	void MyGUIMessageBoxComponent::setStylesCount(unsigned int stylesCount)
	{
		this->stylesCount->setValue(stylesCount);

		size_t oldSize = this->styles.size();

		if (stylesCount > oldSize)
		{
			this->styles.resize(stylesCount);

			std::vector<Ogre::String> stylesNames = { "Ok", "Yes", "No", "Abord", "Retry", "Ignore", "Cancel", "Try", "Continue", "IconInfo", "IconQuest", "IconError", "IconWarning" };

			for (size_t i = oldSize; i < this->styles.size(); i++)
			{
				this->styles[i] = new Variant(MyGUIMessageBoxComponent::AttrStyle() + Ogre::StringConverter::toString(i), stylesNames, this->attributes);
				this->styles[i]->addUserData(GameObject::AttrActionSeparator());
			}
		}
		else if (stylesCount < oldSize)
		{
			this->eraseVariants(this->styles, stylesCount);
		}
	}
		
	unsigned int MyGUIMessageBoxComponent::getStylesCount(void) const
	{
		return this->stylesCount->getUInt();
	}
	
	void MyGUIMessageBoxComponent::setStyle(unsigned int index, const Ogre::String& style)
	{
		if (index > this->styles.size())
			index = static_cast<unsigned int>(this->styles.size()) - 1;
		this->styles[index]->setListSelectedValue(style);
	}
	
	Ogre::String MyGUIMessageBoxComponent::getStyle(unsigned int index) const
	{
		if (index > this->styles.size())
			return "";
		return this->styles[index]->getListSelectedValue();
	}

	void MyGUIMessageBoxComponent::notifyMessageBoxEnd(MyGUI::Message* sender, MyGUI::MessageBoxStyle result)
	{
		if (true == this->isInSimulation)
		{
			// Call also function in lua script, if it does exist in the lua script component
			if (nullptr != this->gameObjectPtr->getLuaScript() && false == this->resultEventName->getString().empty())
			{
				this->gameObjectPtr->getLuaScript()->callTableFunction(this->resultEventName->getString(), this, result);
			}
		}
		// Note: Internally in MyGUI MessageBox, after event handled, the message box will be destroyed, hence its sufficient, to just reset the pointer.
		this->messageBox = nullptr;
	}

	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	MyGUITrackComponent::MyGUITrackComponent()
		: GameObjectComponent(),
		activated(new Variant(MyGUITrackComponent::AttrActivated(), true, this->attributes)),
		cameraId(new Variant(MyGUITrackComponent::AttrCameraId(), static_cast<unsigned long>(0), this->attributes, true)),
		widgetId(new Variant(MyGUITrackComponent::AttrWidgetId(), static_cast<unsigned long>(0), this->attributes, true)),
		offsetPosition(new Variant(MyGUITrackComponent::AttrOffsetPosition(), Ogre::Vector2::ZERO, this->attributes)),
		oldActivated(false),
		camera(nullptr),
		widget(nullptr),
		isSimulating(false)
	{
	}

	MyGUITrackComponent::~MyGUITrackComponent()
	{
		
	}

	bool MyGUITrackComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->setActivated(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "CameraId")
		{
			this->setCameraId(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WidgetId")
		{
			this->setWidgetId(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OffsetPosition")
		{
			this->setOffsetPosition(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	bool MyGUITrackComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUITrackComponent] Init mygui track component for game object: " + this->gameObjectPtr->getName());


		return true;
	}

	GameObjectCompPtr MyGUITrackComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		MyGUITrackCompPtr clonedCompPtr(boost::make_shared<MyGUITrackComponent>());

		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setCameraId(this->cameraId->getULong());
		clonedCompPtr->setWidgetId(this->widgetId->getULong());
		clonedCompPtr->setOffsetPosition(this->offsetPosition->getVector2());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);
		
		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	void MyGUITrackComponent::update(Ogre::Real dt, bool notSimulating)
	{
		this->isSimulating = !notSimulating;
		if (true == this->isSimulating)
		{
			if (nullptr != this->camera && nullptr != this->widget)
			{
				const Ogre::Aabb& aaBB = this->gameObjectPtr->getMovableObject()->getLocalAabb();

				Ogre::Vector3 minimum = aaBB.getMinimum();
				Ogre::Vector3 maximum = aaBB.getMaximum();
				Ogre::Vector3 corners[8];
				//Ogre::Vector3 mMinimum(-1,-1,-1);
				//Ogre::Vector3 mMaximum(1,1,1);
				//mMinimum*=100.0f;
				//mMaximum*=100.0f;
				corners[0] = minimum;
				corners[1] = Ogre::Vector3(minimum.x, maximum.y, minimum.z);
				corners[2] = Ogre::Vector3(maximum.x, maximum.y, minimum.z);
				corners[3] = Ogre::Vector3(maximum.x, minimum.y, minimum.z);
				corners[4] = maximum;
				corners[5] = Ogre::Vector3(minimum.x, maximum.y, maximum.z);
				corners[6] = Ogre::Vector3(minimum.x, minimum.y, maximum.z);
				corners[7] = Ogre::Vector3(maximum.x, minimum.y, maximum.z);

				Ogre::Vector3 point = (corners[0] + corners[2] + corners[4] + corners[6]) / 4.0f;

				// Just show widget when camera points to it
				Ogre::Plane cameraPlane = Ogre::Plane(Ogre::Vector3(camera->getDerivedOrientation().zAxis()), camera->getDerivedPosition());
				/*if (cameraPlane.getSide(point) != Ogre::Plane::NEGATIVE_SIDE)
				{
					this->pOverlay->hide();
					return;
				}*/

				// Get 2D display coordinates for point
				point = camera->getProjectionMatrix() * (camera->getViewMatrix() * point);
				// point.y = point.y + 0.25f;

				// Calibrate in overlay coordinate system [-1, 1] to [0, 1]
				Ogre::Real x = ((point.x / 2) + 0.5f) + this->offsetPosition->getVector2().x;
				Ogre::Real y = (1 - ((point.y / 2) + 0.5f)) + this->offsetPosition->getVector2().y;

				// Actualize position and set at widget center
// Attention: Width must be real und re-calculated?
				this->widget->setRealPosition(x - static_cast<Ogre::Real>(this->widget->getWidth() / 2), y);
				// this->widget->setVisible(true);
			}
		}
	}

	bool MyGUITrackComponent::connect(void)
	{
		GameObjectPtr cameraGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->cameraId->getULong());
		if (nullptr != cameraGameObjectPtr)
		{
			auto& cameraCompPtr = NOWA::makeStrongPtr(cameraGameObjectPtr->getComponent<CameraComponent>());
			if (nullptr != cameraCompPtr)
			{
				this->camera = cameraCompPtr->getCamera();
			}
		}

		for (unsigned int i = 0; i < this->gameObjectPtr->getComponents()->size(); i++)
		{
			auto& gameObjectCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponentByIndex(i));
			if (nullptr != gameObjectCompPtr)
			{
				auto& myGuiCompPtr = boost::dynamic_pointer_cast<MyGUIComponent>(gameObjectCompPtr);
				if (nullptr != myGuiCompPtr && myGuiCompPtr->getId() == this->widgetId->getULong())
				{
					this->widget = myGuiCompPtr->getWidget();
					break;
				}
			}
		}

		return true;
	}

	bool MyGUITrackComponent::disconnect(void)
	{
		this->camera = nullptr;
		this->widget = nullptr;

		return true;
	}

	void MyGUITrackComponent::pause(void)
	{
		// Note: When an appstate is changed, all widgets from the previous AppState remain active (visible), hence hide them
		this->oldActivated = this->activated->getBool();
		this->setActivated(false);
	}

	void MyGUITrackComponent::resume(void)
	{
		// Set the activated state, to the old stored activated state again
		this->setActivated(this->oldActivated);
	}
	
	bool MyGUITrackComponent::onCloned(void)
	{
		// Search for the prior id of the cloned game object and set the new id and set the new id, if not found set better 0, else the game objects may be corrupt!
// Attention: How about parent id etc. because when a widget is cloned, its internal id will be re-generated, so the parent id from another widget that should point to this widget is no more valid!
		GameObjectPtr gameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getClonedGameObjectFromPriorId(this->cameraId->getULong());
		if (nullptr != gameObjectPtr)
		{
			this->cameraId->setValue(gameObjectPtr->getId());
		}
		else
		{
			this->cameraId->setValue(static_cast<unsigned long>(0));
		}

		this->widgetId->setValue(static_cast<unsigned long>(0));

		for (unsigned int i = 0; i < this->gameObjectPtr->getComponents()->size(); i++)
		{
			auto& gameObjectCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponentByIndex(i));
			if (nullptr != gameObjectCompPtr)
			{
				auto& myGuiCompPtr = boost::dynamic_pointer_cast<MyGUIComponent>(gameObjectCompPtr);
				if (nullptr != myGuiCompPtr && (myGuiCompPtr->getId() == this->widgetId->getULong() || myGuiCompPtr->getPriorId() == this->widgetId->getULong()))
				{
					// blur the trace --> No do not blur the trace, else other game objects with joints, that are referring to this id, would not able to set this joint as predecessor!
					// For example a catapult with 4 wheels, where each wheel does reference the catapult!
					// it->second->internalSetPriorId(0);
					this->widgetId->setValue(myGuiCompPtr->getId());
					break;
				}
			}
		}
		// Since connect is called during cloning process, it does not make sense to process furher here, but only when simulation started!
		return true;
	}

	void MyGUITrackComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (MyGUITrackComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (MyGUITrackComponent::AttrCameraId() == attribute->getName())
		{
			this->setCameraId(attribute->getULong());
		}
		else if (MyGUITrackComponent::AttrWidgetId() == attribute->getName())
		{
			this->setWidgetId(attribute->getULong());
		}
		else if (MyGUITrackComponent::AttrOffsetPosition() == attribute->getName())
		{
			this->setOffsetPosition(attribute->getVector2());
		}
	}

	void MyGUITrackComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CameraId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->cameraId->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "WidgetId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->widgetId->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OffsetPosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->offsetPosition->getVector2())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String MyGUITrackComponent::getClassName(void) const
	{
		return "MyGUITrackComponent";
	}

	Ogre::String MyGUITrackComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void MyGUITrackComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
	}

	bool MyGUITrackComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void MyGUITrackComponent::setCameraId(unsigned long cameraId)
	{
		this->cameraId->setValue(cameraId);
	}

	unsigned long MyGUITrackComponent::setCameraId(void) const
	{
		return this->cameraId->getULong();
	}

	void MyGUITrackComponent::setWidgetId(unsigned long widgetId)
	{
		this->widgetId->setValue(widgetId);
	}

	unsigned long MyGUITrackComponent::getWidgetId(void) const
	{
		return this->widgetId->getULong();
	}

	void MyGUITrackComponent::setOffsetPosition(const Ogre::Vector2& offsetPosition)
	{
		this->offsetPosition->setValue(offsetPosition);
	}

	Ogre::Vector2 MyGUITrackComponent::getOffsetPosition(void) const
	{
		return this->offsetPosition->getVector2();
	}

}; // namespace end
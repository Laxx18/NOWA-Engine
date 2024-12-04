#include "NOWAPrecompiled.h"
#include "MyGuiSpriteComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"

#include "OgreAbiUtils.h"
#include "OgreImage2.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	MyGuiSpriteComponent::MyGuiSpriteComponent()
		: MyGUIComponent(),
		name("MyGuiSpriteComponent"),
		currentRow(0),
		currentCol(0),
		timeSinceLastUpdate(0.0f),
		finished(false),
		pickingMask(nullptr),
		image(new Variant(MyGuiSpriteComponent::AttrImage(), Ogre::String(), this->attributes)),
		tileSize(new Variant(MyGuiSpriteComponent::AttrTileSize(), Ogre::Vector2(32.0f, 32.0f), this->attributes)),
		rowsCols(new Variant(MyGuiSpriteComponent::AttrRowsCols(), Ogre::Vector2(1.0f, 1.0f), this->attributes)),
		horizontal(new Variant(MyGuiSpriteComponent::AttrHorizontal(), true, this->attributes)),
		inverseDirection(new Variant(MyGuiSpriteComponent::AttrInverseDirection(), false, this->attributes)),
		speed(new Variant(MyGuiSpriteComponent::AttrSpeed(), 10.0f, this->attributes)),
		startEndIndex(new Variant(MyGuiSpriteComponent::AttrStartEndIndex(), Ogre::Vector2::ZERO, this->attributes)),
		repeat(new Variant(MyGuiSpriteComponent::AttrRepeat(), true, this->attributes)),
		usePickingMask(new Variant(MyGuiSpriteComponent::AttrUsePickingMask(), true, this->attributes))
	{
		this->image->addUserData(GameObject::AttrActionFileOpenDialog(), "Essential");
		this->image->setDescription("Sets the image.");
		this->tileSize->setDescription("Sets the tile size (width, height) of each tile, which shall be animated.");
		this->rowsCols->setDescription("Sets the rows and cols count. If set the 0 0 the whole image will be animated.");
		this->horizontal->setDescription("Sets whether the following neighbour tiles are horizontally aligned or vertically.");
		this->inverseDirection->setDescription("Sets whether to animate backwards.");
		this->speed->setDescription("Sets the animation speed.");
		this->startEndIndex->setDescription("Sets the start and end index, at which the first tile shall start. If set the 0 0 the whole image will be animated.");
		this->repeat->setDescription("If set to false, the animation will only run once. In lua using 'setActivated(true)' will let play the animation again manually.");
		this->usePickingMask->setDescription("Sets whether to use pixel exact picking mask for mouse events or not. If set to true, transparent areas will not be clickable.");

		this->currentFrame = static_cast<int>(this->startEndIndex->getVector2().x);

		this->layer->setListSelectedValue("Back");
	}

	MyGuiSpriteComponent::~MyGuiSpriteComponent(void)
	{
		
	}

	const Ogre::String& MyGuiSpriteComponent::getName() const
	{
		return this->name;
	}

	void MyGuiSpriteComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<MyGuiSpriteComponent>(MyGuiSpriteComponent::getStaticClassId(), MyGuiSpriteComponent::getStaticClassName());
	}
	
	void MyGuiSpriteComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool MyGuiSpriteComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		bool success = MyGUIComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == MyGuiSpriteComponent::AttrImage())
		{
			this->image->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == MyGuiSpriteComponent::AttrTileSize())
		{
			this->tileSize->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == MyGuiSpriteComponent::AttrRowsCols())
		{
			this->rowsCols->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == MyGuiSpriteComponent::AttrHorizontal())
		{
			this->horizontal->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == MyGuiSpriteComponent::AttrInverseDirection())
		{
			this->inverseDirection->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == MyGuiSpriteComponent::AttrSpeed())
		{
			this->speed->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == MyGuiSpriteComponent::AttrStartEndIndex())
		{
			this->startEndIndex->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == MyGuiSpriteComponent::AttrRepeat())
		{
			this->repeat->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == MyGuiSpriteComponent::AttrUsePickingMask())
		{
			this->usePickingMask->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return success;
	}

	GameObjectCompPtr MyGuiSpriteComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		MyGuiSpriteComponentPtr clonedCompPtr(boost::make_shared<MyGuiSpriteComponent>());

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

		clonedCompPtr->setActivated(this->activated->getBool());
		

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->setUsePickingMask(this->usePickingMask->getBool());
		clonedCompPtr->setImage(this->image->getString());
		clonedCompPtr->setTileSize(this->tileSize->getVector2());
		clonedCompPtr->setRowsCols(this->rowsCols->getVector2());
		clonedCompPtr->setHorizontal(this->horizontal->getBool());
		clonedCompPtr->setInverseDirection(this->inverseDirection->getBool());
		clonedCompPtr->setSpeed(this->speed->getReal());
		clonedCompPtr->setStartEndIndex(this->startEndIndex->getVector2());

		clonedCompPtr->setRepeat(this->repeat->getBool());

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool MyGuiSpriteComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGuiSpriteComponent] Init component for game object: " + this->gameObjectPtr->getName());

		if (true == this->createWidgetInParent)
		{
			this->widget = MyGUI::Gui::getInstancePtr()->createWidgetReal<MyGUI::ImageBox>("ImageBox",
																						   this->position->getVector2().x, this->position->getVector2().y, this->size->getVector2().x, this->size->getVector2().y,
																						   this->mapStringToAlign(this->align->getListSelectedValue()), this->layer->getListSelectedValue(), this->getClassName() + "_" + this->gameObjectPtr->getName() + Ogre::StringConverter::toString(this->index));
		}

		this->setUsePickingMask(this->usePickingMask->getBool());
		this->setImage(this->image->getString());
		this->setTileSize(this->tileSize->getVector2());
		this->setRowsCols(this->rowsCols->getVector2());
		this->setHorizontal(this->horizontal->getBool());
		this->setInverseDirection(this->inverseDirection->getBool());
		this->setSpeed(this->speed->getReal());
		this->setStartEndIndex(this->startEndIndex->getVector2());
		this->setRepeat(this->repeat->getBool());

		this->updateFrame();

		return MyGUIComponent::postInit();
	}

	bool MyGuiSpriteComponent::connect(void)
	{
		bool success = MyGUIComponent::connect();

		this->setActivated(this->activated->getBool());
		
		return success;
	}

	bool MyGuiSpriteComponent::disconnect(void)
	{
		bool success = MyGUIComponent::disconnect();

		this->currentRow = 0;
		this->currentCol = 0;
		this->currentFrame = static_cast<int>(this->startEndIndex->getVector2().x);
		this->finished = false;

		this->updateFrame();
		
		return success;
	}

	bool MyGuiSpriteComponent::onCloned(void)
	{
		return true;
	}

	void MyGuiSpriteComponent::onRemoveComponent(void)
	{
		MyGUIComponent::onRemoveComponent();

		if (nullptr != this->pickingMask)
		{
			delete this->pickingMask;
			this->pickingMask = nullptr;
		}
	}
	
	void MyGuiSpriteComponent::onOtherComponentRemoved(unsigned int index)
	{
		
	}
	
	void MyGuiSpriteComponent::onOtherComponentAdded(unsigned int index)
	{
		
	}

	void MyGuiSpriteComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating && true == this->activated->getBool() && false == this->finished)
		{
			Ogre::Real frameTime = 1.0f / this->speed->getReal();
			this->timeSinceLastUpdate += dt;

			// Update sprite animation values
			if (this->timeSinceLastUpdate >= frameTime)
			{
				this->timeSinceLastUpdate = 0.0f;
				advanceFrame();
				updateFrame();
			}
		}
	}

	void MyGuiSpriteComponent::advanceFrame()
	{
		/** Tiles in file start numbering from left to right and from top to bottom.
			For example:
				+---+---+---+
				| 0 | 1 | 2 |
				+---+---+---+
				| 3 | 4 | 5 |
				+---+---+---+
		*/

		int startIndex = static_cast<int>(this->startEndIndex->getVector2().x);
		int endIndex = 0;

		// No end index set, calculate from whole image
		if (0.0f == static_cast<int>(this->startEndIndex->getVector2().y))
		{
			const MyGUI::IntSize& imageSize = widget->castType<MyGUI::ImageBox>()->getImageSize();

			endIndex = imageSize.width / static_cast<int>(this->tileSize->getVector2().x);
			endIndex *= imageSize.height / static_cast<int>(this->tileSize->getVector2().y);
			endIndex -= 1;
		}
		else
		{
			endIndex = static_cast<int>(this->startEndIndex->getVector2().y);
		}

		if (true == this->inverseDirection->getBool())
		{
			--this->currentFrame;
			if (this->currentFrame < startIndex)
			{
				if (false == this->repeat->getBool())
				{
					this->finished = true;
				}
				else
				{
					this->currentFrame = endIndex;
				}
			}
		}
		else
		{
			++this->currentFrame;
			if (this->currentFrame > endIndex)
			{
				if (false == this->repeat->getBool())
				{
					this->finished = true;
				}
				else
				{
					this->currentFrame = startIndex;
				}
			}
		}
	}

	void MyGuiSpriteComponent::updateFrame()
	{
		if (nullptr == this->widget)
		{
			return;
		}

		// No row cols set, calculate from whole image
		int row = static_cast<int>(this->rowsCols->getVector2().x);
		int col = static_cast<int>(this->rowsCols->getVector2().y);
		if (0 == row || col == 0)
		{
			int tileSizeX = static_cast<int>(this->tileSize->getVector2().x);
			if (tileSizeX <= 0)
			{
				tileSizeX = 8;
			}

			int tileSizeY = static_cast<int>(this->tileSize->getVector2().y);
			if (tileSizeY <= 0)
			{
				tileSizeY = 8;
			}

			const MyGUI::IntSize& imageSize = widget->castType<MyGUI::ImageBox>()->getImageSize();
			row = imageSize.width / tileSizeX;
			col = imageSize.height / tileSizeY;
		}

		this->currentRow = this->currentFrame / row;
		this->currentCol = this->currentFrame % col;

		if (false == this->horizontal->getBool())
		{
			std::swap(this->currentRow, this->currentCol);
		}

		if (nullptr != this->widget)
		{
			MyGUI::IntCoord tileCoord(this->currentCol * static_cast<int>(this->tileSize->getVector2().x), this->currentRow * static_cast<int>(this->tileSize->getVector2().y),
									  static_cast<int>(this->tileSize->getVector2().x), static_cast<int>(this->tileSize->getVector2().y));

			widget->castType<MyGUI::ImageBox>()->setImageCoord(tileCoord);
		}
	}

	void MyGuiSpriteComponent::actualizeValue(Variant* attribute)
	{
		MyGUIComponent::actualizeValue(attribute);

		if (MyGuiSpriteComponent::AttrImage() == attribute->getName())
		{
			this->setImage(attribute->getString());
		}
		else if (MyGuiSpriteComponent::AttrTileSize() == attribute->getName())
		{
			this->setTileSize(attribute->getVector2());
		}
		else if (MyGuiSpriteComponent::AttrRowsCols() == attribute->getName())
		{
			this->setRowsCols(attribute->getVector2());
		}
		else if (MyGuiSpriteComponent::AttrHorizontal() == attribute->getName())
		{
			this->setHorizontal(attribute->getBool());
		}
		else if (MyGuiSpriteComponent::AttrInverseDirection() == attribute->getName())
		{
			this->setInverseDirection(attribute->getBool());
		}
		else if (MyGuiSpriteComponent::AttrSpeed() == attribute->getName())
		{
			this->setSpeed(attribute->getReal());
		}
		else if (MyGuiSpriteComponent::AttrStartEndIndex() == attribute->getName())
		{
			this->setStartEndIndex(attribute->getVector2());
		}
		else if (MyGuiSpriteComponent::AttrRepeat() == attribute->getName())
		{
			this->setRepeat(attribute->getBool());
		}
		else if (MyGuiSpriteComponent::AttrUsePickingMask() == attribute->getName())
		{
			this->setUsePickingMask(attribute->getBool());
		}
	}

	void MyGuiSpriteComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		MyGUIComponent::writeXML(propertiesXML, doc, filePath);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Image"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->image->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TileSize"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->tileSize->getVector2())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RowsCols"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rowsCols->getVector2())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Horizontal"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->horizontal->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "InverseDirection"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->inverseDirection->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Speed"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->speed->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "StartEndIndex"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->startEndIndex->getVector2())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Repeat"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->repeat->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UsePickingMask"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->usePickingMask->getBool())));
		propertiesXML->append_node(propertyXML);
	}

	void MyGuiSpriteComponent::setActivated(bool activated)
	{
		MyGUIComponent::setActivated(activated);

		if (true == activated)
		{
			this->finished = false;
		}
	}

	Ogre::String MyGuiSpriteComponent::getClassName(void) const
	{
		return "MyGuiSpriteComponent";
	}

	Ogre::String MyGuiSpriteComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void MyGuiSpriteComponent::setImage(const Ogre::String& image)
	{
		this->image->setValue(image);
		if (nullptr != this->widget)
		{
			this->widget->castType<MyGUI::ImageBox>()->setImageTexture(image);
			
			if (true == this->usePickingMask->getBool())
			{
				// Create a custom picking mask based on the alpha channel using Ogre::Image2
				if (this->pickingMask->loadFromAlpha(image))
				{

				}
			}
		}
		this->updateFrame();
	}

	Ogre::String MyGuiSpriteComponent::getImage(void) const
	{
		return this->image->getString();
	}

	void MyGuiSpriteComponent::setTileSize(const Ogre::Vector2& tileSize)
	{
		Ogre::Vector2 tempTileSize = tileSize;
		if (tileSize.x < 0.0f || tileSize.y < 0.0f)
		{
			tempTileSize.x = 8.0f;
			tempTileSize.y = 8.0f;
		}
		this->tileSize->setValue(tempTileSize);

		if (nullptr != this->widget)
		{
			widget->castType<MyGUI::ImageBox>()->deleteAllItems();

			widget->castType<MyGUI::ImageBox>()->setImageTile(MyGUI::IntSize(static_cast<int>(tempTileSize.x), static_cast<int>(tempTileSize.y)));
			widget->castType<MyGUI::ImageBox>()->setImageTexture(this->image->getString());
		}
		this->updateFrame();
	}

	Ogre::Vector2 MyGuiSpriteComponent::getTileSize(void) const
	{
		return this->tileSize->getVector2();
	}

	void MyGuiSpriteComponent::setRowsCols(const Ogre::Vector2& rowsCols)
	{
		this->rowsCols->setValue(rowsCols);
		this->updateFrame();
	}

	Ogre::Vector2 MyGuiSpriteComponent::getRowsCols(void) const
	{
		return this->rowsCols->getVector2();
	}

	void MyGuiSpriteComponent::setHorizontal(bool horizontal)
	{
		this->horizontal->setValue(horizontal);
		this->updateFrame();
	}

	bool MyGuiSpriteComponent::getHorizontal(void) const
	{
		return this->horizontal->getBool();
	}

	void MyGuiSpriteComponent::setInverseDirection(bool inverseDirection)
	{
		this->inverseDirection->setValue(inverseDirection);

		if (true == inverseDirection)
		{
			this->currentFrame = static_cast<int>(this->startEndIndex->getVector2().y);
			if (0 == this->currentFrame)
			{
				const MyGUI::IntSize& imageSize = widget->castType<MyGUI::ImageBox>()->getImageSize();

				this->currentFrame = imageSize.width / static_cast<int>(this->tileSize->getVector2().x);
				this->currentFrame *= imageSize.height / static_cast<int>(this->tileSize->getVector2().y);
				this->currentFrame -= 1;
			}
		}
		else
		{
			this->currentFrame = static_cast<int>(this->startEndIndex->getVector2().x);
		}

		this->updateFrame();
	}

	bool MyGuiSpriteComponent::getInverseDirection(void) const
	{
		return this->inverseDirection->getBool();
	}

	void MyGuiSpriteComponent::setSpeed(Ogre::Real speed)
	{
		if (speed < 0.0f)
		{
			speed = 0.0f;
		}
		else if (speed > 1000.0f)
		{
			speed = 1000.0f;
		}
		this->speed->setValue(speed);
	}

	Ogre::Real MyGuiSpriteComponent::getSpeed(void) const
	{
		return this->speed->getReal();
	}

	void MyGuiSpriteComponent::setStartEndIndex(const Ogre::Vector2& startEndIndex)
	{
		this->startEndIndex->setValue(startEndIndex);
		this->currentFrame = static_cast<int>(startEndIndex.y);
		this->updateFrame();
	}

	Ogre::Vector2 MyGuiSpriteComponent::getStartEndIndex(void) const
	{
		return this->startEndIndex->getVector2();
	}

	void MyGuiSpriteComponent::setCurrentRow(unsigned int currentRow)
	{
		this->currentRow = currentRow;
	}

	unsigned int MyGuiSpriteComponent::getCurrentRow(void) const
	{
		return this->currentRow;
	}

	void MyGuiSpriteComponent::setCurrentCol(unsigned int currentCol)
	{
		this->currentCol = currentCol;
	}

	unsigned int MyGuiSpriteComponent::getCurrentCol(void) const
	{
		return this->currentCol;
	}

	void MyGuiSpriteComponent::setRepeat(bool repeat)
	{
		this->repeat->setValue(repeat);
	}

	bool MyGuiSpriteComponent::getRepeat(void) const
	{
		return this->repeat->getBool();
	}

	void MyGuiSpriteComponent::callMousePressLuaFunction(void)
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

	void MyGuiSpriteComponent::mouseButtonClick(MyGUI::Widget* sender)
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

	void MyGuiSpriteComponent::mouseButtonPressed(MyGUI::Widget* _sender, int _left, int _top, MyGUI::MouseButton _id)
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

	void MyGuiSpriteComponent::setUsePickingMask(bool usePickingMask)
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

	bool MyGuiSpriteComponent::getUsePickingMask(void) const
	{
		return this->usePickingMask->getBool();
	}

	// Lua registration part

	MyGuiSpriteComponent* getMyGuiSpriteComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<MyGuiSpriteComponent>(gameObject->getComponentWithOccurrence<MyGuiSpriteComponent>(occurrenceIndex)).get();
	}

	MyGuiSpriteComponent* getMyGuiSpriteComponent(GameObject* gameObject)
	{
		return makeStrongPtr<MyGuiSpriteComponent>(gameObject->getComponent<MyGuiSpriteComponent>()).get();
	}

	MyGuiSpriteComponent* getMyGuiSpriteComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<MyGuiSpriteComponent>(gameObject->getComponentFromName<MyGuiSpriteComponent>(name)).get();
	}

	void MyGuiSpriteComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<MyGuiSpriteComponent, MyGUIComponent>("MyGuiSpriteComponent")
			.def("setActivated", &MyGuiSpriteComponent::setActivated)
			.def("isActivated", &MyGuiSpriteComponent::isActivated)
			.def("setCurrentRow", &MyGuiSpriteComponent::setCurrentRow)
			.def("getCurrentRow", &MyGuiSpriteComponent::getCurrentRow)
		];

		LuaScriptApi::getInstance()->addClassToCollection("MyGuiSpriteComponent", "class inherits GameObjectComponent", MyGuiSpriteComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("MyGuiSpriteComponent", "void setActivated(bool activated)", "Sets whether this component should be activated and the sprite animation take place.");
		LuaScriptApi::getInstance()->addClassToCollection("MyGuiSpriteComponent", "bool isActivated()", "Gets whether this component is activated.");
		LuaScriptApi::getInstance()->addClassToCollection("MyGuiSpriteComponent", "void setCurrentRow(number currentRow)", "Sets manually the current to be displayed tile row, if animation shall take place in lua script.");
		LuaScriptApi::getInstance()->addClassToCollection("MyGuiSpriteComponent", "number getCurrentRow()", "Gets the current row.");
		LuaScriptApi::getInstance()->addClassToCollection("MyGuiSpriteComponent", "void setCurrentCol(number currentCol)", "Sets manually the current to be displayed tile column, if animation shall take place in lua script.");
		LuaScriptApi::getInstance()->addClassToCollection("MyGuiSpriteComponent", "number getCurrentCol()", "Gets the current column.");

		gameObjectClass.def("getMyGuiSpriteComponentFromName", &getMyGuiSpriteComponentFromName);
		gameObjectClass.def("getMyGuiSpriteComponent", (MyGuiSpriteComponent * (*)(GameObject*)) & getMyGuiSpriteComponent);
		// If its desired to create several of this components for one game object
		gameObjectClass.def("getMyGuiSpriteComponentFromIndex", (MyGuiSpriteComponent * (*)(GameObject*, unsigned int)) & getMyGuiSpriteComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "MyGuiSpriteComponent getMyGuiSpriteComponentFromIndex(unsigned int occurrenceIndex)", "Gets the component by the given occurence index, since a game object may this component maybe several times.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "MyGuiSpriteComponent getMyGuiSpriteComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "MyGuiSpriteComponent getMyGuiSpriteComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castMyGuiSpriteComponent", &GameObjectController::cast<MyGuiSpriteComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "MyGuiSpriteComponent castMyGuiSpriteComponent(MyGuiSpriteComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool MyGuiSpriteComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// No constraints so far, just add
		return true;
	}

}; //namespace end
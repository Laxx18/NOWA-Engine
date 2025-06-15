#include "NOWAPrecompiled.h"
#include "MyGUIItemBoxComponent.h"
#include "LuaScriptComponent.h"
#include "modules/LuaScriptApi.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "MyGUI_LayerManager.h"
#include "main/AppStateManager.h"
	
namespace NOWA
{

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void ResourceItemInfo::deserialization(MyGUI::xml::ElementPtr _node, MyGUI::Version _version)
	{
		// Pre load everything, so that e.g. for a resource name its image is available
		Base::deserialization(_node, _version);

		MyGUI::xml::ElementEnumerator node = _node->getElementEnumerator();
		while (node.next())
		{
			if (node->getName() == "Name")
				this->itemName = node->getContent();
			else if (node->getName() == "Description")
				this->itemDescription = node->getContent();
			else if (node->getName() == "Image")
				this->itemResourceImage = node->findAttribute("RefID");
		}
	}

	void ResourceItemInfo::setItemName(const Ogre::String& itemName)
	{
		this->itemName = itemName;
	}

	Ogre::String ResourceItemInfo::getItemName(void) const
	{
		return this->itemName;
	}

	void ResourceItemInfo::setItemResourceImage(const Ogre::String& itemResourceImage)
	{
		this->itemResourceImage = itemResourceImage;
	}

	Ogre::String ResourceItemInfo::getItemResourceImage(void) const
	{
		return this->itemResourceImage;
	}

	void ResourceItemInfo::setItemDescription(const Ogre::String& itemDescription)
	{
		this->itemDescription = itemDescription;
	}

	Ogre::String ResourceItemInfo::getItemDescription(void) const
	{
		return this->itemDescription;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ItemData::ItemData(unsigned long inventoryOwnerId)
		: inventoryOwnerId(inventoryOwnerId),
		quantity(0),
		sellValue(0.0f),
		buyValue(0.0f),
		resourceInfo(nullptr),
		resourceImage(nullptr)
	{
	}

	ItemData::ItemData(const Ogre::String& resourceName, unsigned int quantity, Ogre::Real sellValue, Ogre::Real buyValue)
		: resourceName(resourceName),
		quantity(quantity),
		sellValue(sellValue),
		buyValue(buyValue),
		resourceInfo(nullptr),
		resourceImage(nullptr),
		inventoryOwnerId(0L)
	{
		bool foundCorrectType = false;
		MyGUI::ResourceManager& manager = MyGUI::ResourceManager::getInstance();
		MyGUI::IResource* resourceItemInfo = manager.getByName(resourceName, false);
		if (nullptr != resourceItemInfo)
		{
			this->resourceInfo = resourceItemInfo->castType<ResourceItemInfo>(false);
			if (nullptr != this->resourceInfo)
			{
				MyGUI::IResource* resourceImageSet = manager.getByName(this->resourceInfo->getItemResourceImage(), false);
				if (nullptr != resourceImageSet)
				{
					this->resourceImage = resourceImageSet->castType<MyGUI::ResourceImageSet>(false);
					if (nullptr != this->resourceImage)
					{
						foundCorrectType = true;
					}
				}
			}
		}

		if (false == foundCorrectType)
		{
			MyGUI::IResource* resourceImageSet = manager.getByName(resourceName, false);
			if (nullptr != resourceImageSet)
			{
				this->resourceImage = resourceImageSet->castType<MyGUI::ResourceImageSet>();
				foundCorrectType = true;
			}
		}

		if (false == foundCorrectType)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MyGUIItemBoxComponent] ERROR: Could not add resource: "
				+ resourceName + ", because it cannot be found, or the XML container is wrong. Check your resource location name XML");
		}
	}

	bool ItemData::isEmpty() const
	{
		return this->resourceInfo == 0;
	}

	bool ItemData::compare(ItemData* data) const
	{
		return this->resourceInfo == data->resourceInfo;
	}

	void ItemData::add(ItemData* data)
	{
		this->resourceName = data->getResourceName();
		// this->quantity += data->getQuantity(); // Do not add quantity, else it is added twice
		this->resourceInfo = data->resourceInfo;
		this->resourceImage = data->resourceImage;
	}

	void ItemData::removeQuantity(unsigned int quantity)
	{
		if (this->quantity > 1)
		{
			this->quantity -= quantity;
		}
		else
		{
			this->clear();
		}
	}

	void ItemData::clear()
	{
		this->resourceInfo = nullptr;
		this->resourceImage = nullptr;
		this->quantity = 0;
		this->resourceName.clear();
	}

	void ItemData::setResourceName(const Ogre::String& resourceName)
	{
		this->resourceName = resourceName;
		MyGUI::ResourceManager& manager = MyGUI::ResourceManager::getInstance();
		MyGUI::IResource* resource = manager.getByName(resourceName, false);
		if (nullptr != resource)
		{
			this->resourceInfo = resource->castType<ResourceItemInfo>();
			// this->resourceInfo->setItemName(resourceName); // Itemname comes from XML and is something different
			if (false == this->resourceInfo->getItemResourceImage().empty())
			{
				ENQUEUE_RENDER_COMMAND_MULTI("ItemData::setResourceName", _1(manager),
				{
					this->resourceImage = manager.getByName(this->resourceInfo->getItemResourceImage())->castType<MyGUI::ResourceImageSet>();
				});
			}
		}
	}

	Ogre::String ItemData::getResourceName(void) const
	{
		return this->resourceName;
	}

	void ItemData::setQuantity(unsigned int quantity)
	{
		this->quantity = quantity;
		if (0 == quantity && false == this->resourceName.empty())
		{
			this->clear();
		}
	}

	unsigned int ItemData::getQuantity(void) const
	{
		return this->quantity;
	}

	void ItemData::setSellValue(Ogre::Real sellValue)
	{
		this->sellValue = sellValue;
	}

	Ogre::Real ItemData::getSellValue(void) const
	{
		return this->sellValue;
	}

	void ItemData::setBuyValue(Ogre::Real buyValue)
	{
		this->buyValue = buyValue;
	}

	Ogre::Real ItemData::getBuyValue(void) const
	{
		return this->buyValue;
	}

	ResourceItemInfoPtr ItemData::getInfo(void) const
	{
		return this->resourceInfo;
	}

	MyGUI::ResourceImageSetPtr ItemData::getImage(void) const
	{
		return this->resourceImage;
	}

	void ItemData::setInventoryOwnerId(unsigned long inventoryOwnerId)
	{
		this->inventoryOwnerId = inventoryOwnerId;
	}

	unsigned long ItemData::getInventoryOwnerId(void) const
	{
		return this->inventoryOwnerId;
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ToolTip::ToolTip() :
		BaseLayout("ToolTip.layout")
	{
		assignWidget(mTextName, "text_Name");
		assignWidget(mTextCount, "text_Count");
		assignWidget(mSellValue, "text_SellValue");
		assignWidget(mBuyValue, "text_BuyValue");
		assignWidget(mTextDesc, "text_Desc");
		assignWidget(mImageInfo, "image_Info");

		MyGUI::ISubWidgetText* text = mTextDesc->getSubWidgetText();
		const MyGUI::IntCoord& coord = text ? text->getCoord() : MyGUI::IntCoord();
		mOffsetHeight = mMainWidget->getHeight() - coord.height;
	}

	void ToolTip::show(ItemData* _data)
	{
		if ((_data == nullptr) || _data->isEmpty())
			return;

		mTextCount->setCaption(MyGUI::utility::toString(_data->getQuantity()));
		if (_data->getSellValue() > 0.0f)
		{
			mSellValue->setCaption(MyGUI::utility::toString(_data->getSellValue()));
		}
		if (_data->getBuyValue() > 0.0f)
		{
			mBuyValue->setCaption(MyGUI::utility::toString(_data->getBuyValue()));
		}
		mTextName->setCaption(_data->getInfo()->getItemName());
		mTextDesc->setCaption(_data->getInfo()->getItemDescription());
		if (!_data->isEmpty())
		{
			mImageInfo->setItemResourceInfo(_data->getImage(), "ToolTip", "Normal");
		}

		// вычисляем размер
		MyGUI::ISubWidgetText* text = mTextDesc->getSubWidgetText();
		const MyGUI::IntSize& text_size = text ? text->getTextSize() : MyGUI::IntSize();
		mMainWidget->setSize(mMainWidget->getWidth(), mOffsetHeight + text_size.height);

		mMainWidget->setVisible(true);
	}

	void ToolTip::hide()
	{
		mMainWidget->setVisible(false);
	}

	void ToolTip::move(const MyGUI::IntPoint& _point)
	{
		const MyGUI::IntPoint offset(10, 10);

		MyGUI::IntPoint point = MyGUI::InputManager::getInstance().getMousePosition() + offset;

		const MyGUI::IntSize& size = mMainWidget->getSize();
		const MyGUI::IntSize& view_size = mMainWidget->getParentSize();

		if ((point.left + size.width) > view_size.width)
		{
			point.left -= offset.left + offset.left + size.width;
		}
		if ((point.top + size.height) > view_size.height)
		{
			point.top -= offset.top + offset.top + size.height;
		}

		mMainWidget->setPosition(point);
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void CellView::getCellDimension(MyGUI::Widget* _sender, MyGUI::IntCoord& _coord, bool _drop)
	{
		// размеры ячеек для контейнера и для перетаскивания могут различаться
		/*if (_drop)
			_coord.set(0, 0, 90, 74);
		else
			_coord.set(0, 0, 90, 74);*/

		_coord.set(0, 0, 90, 74);
	}

	CellView::CellView(MyGUI::Widget* _parent) :
		wraps::BaseCellView<ItemData*>("CellView.layout", _parent)
	{
		this->uniqueId = NOWA::makeUniqueID();

		ENQUEUE_RENDER_COMMAND_WAIT("ItemBox::CellView::CellView",
		{
			assignWidget(mImageBack, "image_Back");
			assignWidget(mImageBorder, "image_Border");
			assignWidget(mImageItem, "image_Item");
			assignWidget(mTextBack, "text_Back");
			assignWidget(mTextFront, "text_Front");
		});
	}

	CellView::~CellView()
	{

	}

	void CellView::update(const MyGUI::IBDrawItemInfo& _info, ItemData* _data)
	{
		if (_info.update)
		{
			auto closureFunction = [this, _info, _data](Ogre::Real weight)
			{
				if (!_data->isEmpty())
				{
					mImageItem->setItemResourcePtr(_data->getImage());
					mImageItem->setItemGroup("States");
					mImageItem->setVisible(true);
				}
				else
				{
					mImageItem->setVisible(false);
				}
				mTextBack->setCaption(((_data->getQuantity() > 1) && (!_info.drag)) ? MyGUI::utility::toString(_data->getQuantity()) : "");
				mTextFront->setCaption(((_data->getQuantity() > 1) && (!_info.drag)) ? MyGUI::utility::toString(_data->getQuantity()) : "");

				static MyGUI::ResourceImageSetPtr resourceBack = nullptr;
				static MyGUI::ResourceImageSetPtr resourceSelect = nullptr;
				if (resourceBack == nullptr)
					resourceBack = MyGUI::ResourceManager::getInstance().getByName("pic_ItemBackImage")->castType<MyGUI::ResourceImageSet>(false);
				if (resourceSelect == nullptr)
					resourceSelect = MyGUI::ResourceManager::getInstance().getByName("pic_ItemSelectImage")->castType<MyGUI::ResourceImageSet>(false);

				mImageBack->setItemResourcePtr(resourceBack);
				mImageBack->setItemGroup("States");
				mImageBorder->setItemResourcePtr(resourceSelect);
				mImageBorder->setItemGroup("States");
			};
			Ogre::String id = "CellView::update1" + Ogre::StringConverter::toString(this->uniqueId);
			NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction);
		}

		if (_info.drag && nullptr != _data->getInfo())
		{
			auto closureFunction = [this, _info, _data](Ogre::Real weight)
			{
				mImageBack->setItemName("None");
				mImageBorder->setItemName("None");

				if (!_data->isEmpty())
				{
					if (_info.drop_refuse)
						mImageItem->setItemName("Refuse");
					else if (_info.drop_accept)
						mImageItem->setItemName("Accept");
					else
						mImageItem->setItemName("Normal");
					mImageItem->setVisible(true);
				}
				else
				{
					mImageItem->setVisible(false);
				}
			};
			Ogre::String id = "CellView::update2" + Ogre::StringConverter::toString(this->uniqueId);
			NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction);
		}
		else
		{
			auto closureFunction = [this, _info, _data](Ogre::Real weight)
			{
				if (_info.active)
				{
					if (_info.select)
						mImageBack->setItemName("Select");
					else
						mImageBack->setItemName("Active");
				}
				else if (_info.select)
					mImageBack->setItemName("Pressed");
				else
					mImageBack->setItemName("Normal");

				if (_info.drop_refuse)
				{
					mImageBorder->setItemName("Refuse");
					mTextFront->setTextColour(MyGUI::Colour::Red);
				}
				else if (_info.drop_accept)
				{
					mImageBorder->setItemName("Accept");
					mTextFront->setTextColour(MyGUI::Colour::Green);
				}
				else
				{
					mImageBorder->setItemName("Normal");
					mTextFront->setTextColour(MyGUI::Colour::White);
				}

				if (!_data->isEmpty())
				{
					mImageItem->setItemName("Normal");
					mImageItem->setVisible(true);
				}
				else
				{
					mImageItem->setVisible(false);
				}
			};
			Ogre::String id = "CellView::update3" + Ogre::StringConverter::toString(this->uniqueId);
			NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction);
		}
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ItemBox::ItemBox(MyGUI::Widget* _parent) :
		wraps::BaseItemBox<CellView>(_parent)
	{
	}

	ItemBox::~ItemBox()
	{
		ENQUEUE_RENDER_COMMAND_WAIT("ItemBox::~ItemBox",
		{
			MyGUI::ItemBox * box = getItemBox();
			size_t count = box->getItemCount();
			for (size_t pos = 0; pos < count; ++pos)
			{
				delete* box->getItemDataAt<ItemData*>(pos);
			}
		});
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ItemBoxWindow::ItemBoxWindow(const std::string& _layout) :
		BaseLayout(_layout)
	{
		ENQUEUE_RENDER_COMMAND_WAIT("ItemBoxWindow::ItemBoxWindow",
		{
			assignBase(mItemBox, "box_Items");
		});
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	using namespace rapidxml;
	using namespace luabind;

	MyGUIItemBoxComponent::MyGUIItemBoxComponent()
		: MyGUIWindowComponent(),
		toolTip(nullptr),
		itemBoxWindow(nullptr),
		dragDropData(new DragDropData),
		dropFinished(false)
	{
		this->skin = new Variant(MyGUIComponent::AttrSkin(), { "ItemBox" }, this->attributes);
		this->resourceLocationName = new Variant(MyGUIItemBoxComponent::AttrResourceLocationName(), "InventoryItemsResources.xml", this->attributes);
		this->useToolTip = new Variant(MyGUIItemBoxComponent::AttrUseToolTip(), true, this->attributes);
		this->allowDragDrop = new Variant(MyGUIItemBoxComponent::AttrAllowDragDrop(), true, this->attributes);
		this->itemCount = new Variant(MyGUIItemBoxComponent::AttrItemCount(), 0, this->attributes);

		this->resourceNames.resize(this->itemCount->getUInt());
		this->quantities.resize(this->itemCount->getUInt());
		this->sellValues.resize(this->itemCount->getUInt());
		this->buyValues.resize(this->itemCount->getUInt());
		// Automatically also triggers needRefresh!
		this->resourceLocationName->addUserData(GameObject::AttrActionFileOpenDialog(), "Essential");
		// Since when node item count is changed, the whole properties must be refreshed, so that new field may come for item tracks
		this->itemCount->addUserData(GameObject::AttrActionNeedRefresh());
		// ItemBox has its own skin and may not be changed!
		this->skin->setVisible(false);

		this->position->setValue(Ogre::Vector2(0.0f, 0.8f));
		this->size->setValue(Ogre::Vector2(0.2f, 1.0f));
	}

	MyGUIItemBoxComponent::~MyGUIItemBoxComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUIItemBoxComponent] Destructor MyGUI item box component for game object: " + this->gameObjectPtr->getName());
		
		ENQUEUE_RENDER_COMMAND("MyGUIItemBoxComponent::~MyGUIItemBoxComponent",
		{
			std::string resourceCategory = MyGUI::ResourceManager::getInstance().getCategoryName();
			MyGUI::FactoryManager::getInstance().unregisterFactory<ResourceItemInfo>(resourceCategory);

			// Is done in MyGUIWidgetComponent
			if (nullptr != this->itemBoxWindow)
			{
				if (nullptr != this->widget)
				{
					MyGUI::Gui::getInstancePtr()->destroyWidget(this->widget);
					this->widget = nullptr;
				}
				delete this->itemBoxWindow;
				this->itemBoxWindow = nullptr;
			}
			if (nullptr != this->toolTip)
			{
				delete this->toolTip;
				this->toolTip = nullptr;
			}

			if (nullptr != this->dragDropData)
			{
				delete this->dragDropData;
				this->dragDropData = nullptr;
			}
		});
	}

	bool MyGUIItemBoxComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		// Tells the system, that this component shall be fully restored, because its to complex to restore just some attributes
		// This is usually the case if there is a list of attributes involved
		this->customDataString = GameObjectComponent::AttrCustomDataNewCreation();

		bool success = MyGUIWindowComponent::init(propertyElement);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ResourceLocationName")
		{
			this->resourceLocationName->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseToolTip")
		{
			this->useToolTip->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AllowDragDrop")
		{
			this->allowDragDrop->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ItemCount")
		{
			this->itemCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
			this->itemCount->addUserData(GameObject::AttrActionSeparator());
		}

		// Only create new variant, if fresh loading. If snapshot is done, no new variant
		// must be created! Because the algorithm is working changed flag of each existing variant!
		if (this->resourceNames.size() < this->itemCount->getUInt())
		{
			this->resourceNames.resize(this->itemCount->getUInt());
			this->quantities.resize(this->itemCount->getUInt());
			this->sellValues.resize(this->itemCount->getUInt());
			this->buyValues.resize(this->itemCount->getUInt());
		}

		for (size_t i = 0; i < this->itemCount->getUInt(); i++)
		{
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ResourceName" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->resourceNames[i])
				{
					this->resourceNames[i] = new Variant(MyGUIItemBoxComponent::AttrResourceName() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttrib(propertyElement, "data"), this->attributes);
				}
				else
				{
					this->resourceNames[i]->setValue(XMLConverter::getAttrib(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");

			}
			else
			{
				this->resourceNames[i] = new Variant(MyGUIItemBoxComponent::AttrResourceName() + Ogre::StringConverter::toString(i), "", this->attributes);
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Quantity" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->quantities[i])
				{
					this->quantities[i] = new Variant(MyGUIItemBoxComponent::AttrQuantity() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttribUnsignedInt(propertyElement, "data"), this->attributes);
				}
				else
				{
					this->quantities[i]->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			else
			{
				this->quantities[i] = new Variant(MyGUIItemBoxComponent::AttrQuantity() + Ogre::StringConverter::toString(i), 0, this->attributes);
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SellValue" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->sellValues[i])
				{
					this->sellValues[i] = new Variant(MyGUIItemBoxComponent::AttrSellValue() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttribReal(propertyElement, "data"), this->attributes);
				}
				else
				{
					this->sellValues[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
			}
			else
			{
				this->sellValues[i] = new Variant(MyGUIItemBoxComponent::AttrSellValue() + Ogre::StringConverter::toString(i), 0.0f, this->attributes);
			}
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BuyValue" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->buyValues[i])
				{
					this->buyValues[i] = new Variant(MyGUIItemBoxComponent::AttrBuyValue() + Ogre::StringConverter::toString(i),
						XMLConverter::getAttribReal(propertyElement, "data"), this->attributes);
				}
				else
				{
					this->buyValues[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
				this->buyValues[i]->addUserData(GameObject::AttrActionSeparator());
			}
			// Backward compatibility. No buy values loaded, create new ones
			if (nullptr == this->buyValues[i])
			{
				this->buyValues[i] = new Variant(MyGUIItemBoxComponent::AttrBuyValue() + Ogre::StringConverter::toString(i),
												 XMLConverter::getAttribReal(propertyElement, "data"), this->attributes);
			}
		}

		return success;
	}

	GameObjectCompPtr MyGUIItemBoxComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		MyGUIItemBoxCompPtr clonedCompPtr(boost::make_shared<MyGUIItemBoxComponent>());

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

		clonedCompPtr->setResourceLocationName(this->resourceLocationName->getString());
		clonedCompPtr->setUseToolTip(this->useToolTip->getBool());
		clonedCompPtr->setAllowDragDrop(this->allowDragDrop->getBool());
		clonedCompPtr->setItemCount(this->itemCount->getUInt());

		for (unsigned int i = 0; i < this->itemCount->getUInt(); i++)
		{
			clonedCompPtr->setResourceName(i, this->resourceNames[i]->getString());
			clonedCompPtr->setQuantity(i, this->quantities[i]->getUInt());
			clonedCompPtr->setSellValue(i, this->sellValues[i]->getReal());
			clonedCompPtr->setBuyValue(i, this->buyValues[i]->getReal());
		}

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool MyGUIItemBoxComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUIItemBoxComponent] Init MyGUI item box component for game object: " + this->gameObjectPtr->getName());
		
		ENQUEUE_RENDER_COMMAND_WAIT("MyGUIItemBoxComponent::postInit",
		{
			// Do not create widget in parent, because it is created here, but use other parent attributes
			this->createWidgetInParent = false;

			std::string resourceCategory = MyGUI::ResourceManager::getInstance().getCategoryName();
			MyGUI::FactoryManager::getInstance().registerFactory<ResourceItemInfo>(resourceCategory);

			bool success = MyGUI::ResourceManager::getInstance().load(this->resourceLocationName->getString());
			if (false == success)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MyGUIItemBoxComponent] ERROR: Could not load resource: "
					+ this->resourceLocationName->getString() + ", because it cannot be found in '" + this->resourceLocationName->getString() + "', for game object : " + this->gameObjectPtr->getName());
			}
			//MyGUI::ResourceManager::getInstance().load("ItemBox_skin.xml");

			this->itemBoxWindow = new ItemBoxWindow("ItemBox.layout");

			this->itemBoxWindow->getItemBox()->eventStartDrag = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyStartDrop);
			this->itemBoxWindow->getItemBox()->eventRequestDrop = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyRequestDrop);
			this->itemBoxWindow->getItemBox()->eventDropResult = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyEndDrop);
			this->itemBoxWindow->getItemBox()->eventChangeDDState = newDelegate(this, &MyGUIItemBoxComponent::notifyDropState);
			this->itemBoxWindow->getItemBox()->eventNotifyItem = newDelegate(this, &MyGUIItemBoxComponent::notifyNotifyItem);
			// this->itemBoxWindow->getItemBox()->getItemBox()->requestCreateWidgetItem = newDelegate(this, &MyGUIItemBoxComponent::requestCreateWidgetItem);
			// this->itemBoxWindow->getItemBox()->getItemBox()->requestCoordItem = newDelegate(this, &MyGUIItemBoxComponent::requestCoordItem);
			// this->itemBoxWindow->getItemBox()->getItemBox()->requestDrawItem = newDelegate(this, &MyGUIItemBoxComponent::requestDrawItem);

			this->itemBoxWindow->getItemBox()->eventToolTip = newDelegate(this, &MyGUIItemBoxComponent::notifyToolTip);

			this->widget = this->itemBoxWindow->getMainWidget();
			// this->widget->changeWidgetSkin("Window"); // Causes crash
			this->setUseToolTip(this->useToolTip->getBool());
		});

		this->setItemCount(this->itemCount->getUInt());
		this->resourceNames.resize(this->itemCount->getUInt());
		this->quantities.resize(this->itemCount->getUInt());
		this->sellValues.resize(this->itemCount->getUInt());
		this->buyValues.resize(this->itemCount->getUInt());

		for (unsigned int i = 0; i < this->itemCount->getUInt(); i++)
		{
			this->itemBoxWindow->getItemBox()->addItem(new ItemData(this->gameObjectPtr->getId()));
			if (nullptr != this->resourceNames[i])
			{
				if (nullptr != this->quantities[i])
				{
					if (this->quantities[i]->getUInt() > 0)
					{
						// Only set resource name and icon if quantity is higher as zero
						this->setResourceName(i, this->resourceNames[i]->getString());
					}
				}
			}
			if (nullptr != this->quantities[i])
			{
				this->setQuantity(i, this->quantities[i]->getUInt());
			}
			if (nullptr != this->sellValues[i])
			{
				this->setSellValue(i, this->sellValues[i]->getReal());
			}
			if (nullptr != this->buyValues[i])
			{
				this->setBuyValue(i, this->buyValues[i]->getReal());
			}
		}

		return MyGUIWindowComponent::postInit();
	}

	void MyGUIItemBoxComponent::mouseButtonClick(MyGUI::Widget* sender)
	{
		/*if (true == this->isSimulating)
		{
			MyGUI::Window* window = sender->castType<MyGUI::Window>();
			if (nullptr != window)
			{
				// Call also function in lua script, if it does exist in the lua script component
				if (nullptr != this->luaScript)
				{
					this->luaScript->callTableFunction("onMouseClick", this->gameObjectPtr.get(), this);
				}
			}
		}*/
	}

	bool MyGUIItemBoxComponent::connect(void)
	{
		bool success = MyGUIWindowComponent::connect();

		return success;
	}

	bool MyGUIItemBoxComponent::disconnect(void)
	{
		/*this->setSkin(this->skin->getListSelectedValue());
		this->setMovable(this->movable->getBool());*/

		return MyGUIWindowComponent::disconnect();
	}

	void MyGUIItemBoxComponent::actualizeValue(Variant* attribute)
	{
		MyGUIWindowComponent::actualizeValue(attribute);
		
		if (MyGUIItemBoxComponent::AttrResourceLocationName() == attribute->getName())
		{
			this->setResourceLocationName(attribute->getString());
		}
		else if (MyGUIItemBoxComponent::AttrUseToolTip() == attribute->getName())
		{
			this->setUseToolTip(attribute->getBool());
		}
		
		else if (MyGUIItemBoxComponent::AttrAllowDragDrop() == attribute->getName())
		{
			this->setAllowDragDrop(attribute->getBool());
		}
		else if (MyGUIItemBoxComponent::AttrItemCount() == attribute->getName())
		{
			this->setItemCount(attribute->getUInt());
		}
		else
		{
			for (unsigned int i = 0; i < this->itemCount->getUInt(); i++)
			{
				if (MyGUIItemBoxComponent::AttrResourceName() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setResourceName(i, attribute->getString());
				}
				else if (MyGUIItemBoxComponent::AttrQuantity() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setQuantity(i, attribute->getUInt());
				}
				else if (MyGUIItemBoxComponent::AttrSellValue() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setSellValue(i, attribute->getReal());
				}
				else if (MyGUIItemBoxComponent::AttrBuyValue() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->setBuyValue(i, attribute->getReal());
				}
			}
		}
	}

	void MyGUIItemBoxComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		MyGUIWindowComponent::writeXML(propertiesXML, doc);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ResourceLocationName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->resourceLocationName->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UseToolTip"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useToolTip->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AllowDragDrop"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->allowDragDrop->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ItemCount"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->itemCount->getUInt())));
		propertiesXML->append_node(propertyXML);

		for (size_t i = 0; i < this->itemCount->getUInt(); i++)
		{
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "ResourceName" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->resourceNames[i]->getString())));
			propertiesXML->append_node(propertyXML);
			
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "Quantity" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->quantities[i]->getUInt())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "SellValue" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->sellValues[i]->getReal())));
			propertiesXML->append_node(propertyXML);

			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "BuyValue" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->buyValues[i]->getReal())));
			propertiesXML->append_node(propertyXML);
		}
	}

	Ogre::String MyGUIItemBoxComponent::getClassName(void) const
	{
		return "MyGUIItemBoxComponent";
	}

	Ogre::String MyGUIItemBoxComponent::getParentClassName(void) const
	{
		return "MyGUIWindowComponent";
	}
	
	void MyGUIItemBoxComponent::notifyStartDrop(wraps::BaseLayout* _sender, wraps::DDItemInfo _info, bool& _result)
	{
		if (_info.sender_index != MyGUI::ITEM_NONE && true == this->allowDragDrop->getBool() && true == this->isSimulating)
		{
			ItemData* data = *static_cast<ItemBox*>(_info.sender)->getItemDataAt<ItemData*>(_info.sender_index);
			_result = !data->isEmpty();
			this->dropFinished = false;
		}
	}

	void MyGUIItemBoxComponent::notifyRequestDrop(wraps::BaseLayout* _sender, wraps::DDItemInfo _info, bool& _result)
	{
		// не на айтем кидаем
		if (_info.receiver_index == MyGUI::ITEM_NONE)
		{
			_result = false;
			return;
		}

		ItemData* senderData = *static_cast<ItemBox*>(_info.sender)->getItemDataAt<ItemData*>(_info.sender_index);
		ItemData* receiverData = *static_cast<ItemBox*>(_info.receiver)->getItemDataAt<ItemData*>(_info.receiver_index);

		this->dragDropData->clear();
		this->dragDropData->senderInventoryId = senderData->getInventoryOwnerId();
		this->dragDropData->resourceName = senderData->getResourceName();
		this->dragDropData->quantity = 1; // Always transfer just one item not all at once!// senderData->getQuantity();
		this->dragDropData->sellValue = senderData->getSellValue();
		this->dragDropData->buyValue = senderData->getBuyValue();

		// Sender and receiver is the same and its the same inventory slot, moving within the same slot, which is unspectacular and can be skipped
		if ((_info.sender == _info.receiver) && (_info.sender_index == _info.receiver_index))
		{
			return;
		}

		// Sender and receiver is the same
		if ((_info.sender == _info.receiver))
		{
			this->dragDropData->senderReceiverIsSame = true;

			if (this->closureFunctionRequestDropRequest.is_valid())
			{
				NOWA::AppStateManager::LogicCommand logicCommand = [this]()
					{
						try
						{
							luabind::call_function<void>(this->closureFunctionRequestDropRequest, this->dragDropData);
						}
						catch (luabind::error& error)
						{
							luabind::object errorMsg(luabind::from_stack(error.state(), -1));
							std::stringstream msg;
							msg << errorMsg;

							Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[MyGUIItemBoxComponent] Caught error in 'reactOnDropItemRequest' Error: " + Ogre::String(error.what())
								+ " details: " + msg.str());
						}
					};
				NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
			}
		}
		else
		{
			if (receiverData->getInventoryOwnerId() != this->gameObjectPtr->getId())
			{
				// Calls on receiver the closure function, if does exist
				auto receiverGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(receiverData->getInventoryOwnerId());
				auto myGuiItemBoxCompPtr = NOWA::makeStrongPtr(receiverGameObjectPtr->getComponent<MyGUIItemBoxComponent>());
				if (nullptr != myGuiItemBoxCompPtr)
				{
					if (myGuiItemBoxCompPtr->closureFunctionRequestDropRequest.is_valid())
					{
						NOWA::AppStateManager::LogicCommand logicCommand = [this, myGuiItemBoxCompPtr]()
							{
								try
								{
									luabind::call_function<void>(myGuiItemBoxCompPtr->closureFunctionRequestDropRequest, this->dragDropData);
								}
								catch (luabind::error& error)
								{
									luabind::object errorMsg(luabind::from_stack(error.state(), -1));
									std::stringstream msg;
									msg << errorMsg;

									Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[MyGUIItemBoxComponent] Caught error in 'reactOnDropItemRequest' Error: " + Ogre::String(error.what())
										+ " details: " + msg.str());
								}
							};
						NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
					}
				}
			}
		}

		_result = true == this->dragDropData->canDrop && (receiverData->isEmpty() || receiverData->compare(senderData));

		ENQUEUE_RENDER_COMMAND("MyGUIItemBoxComponent::notifyRequestDrop",
		{
			this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x - 0.001f, this->size->getVector2().y - 0.001f);
			this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x + 0.001f, this->size->getVector2().y + 0.001f);
		});
	}

	void MyGUIItemBoxComponent::notifyEndDrop(wraps::BaseLayout* _sender, wraps::DDItemInfo _info, bool _result)
	{
		if (_result && false == this->dropFinished && true == this->dragDropData->canDrop)
		{
			ItemData* senderData = *static_cast<ItemBox*>(_info.sender)->getItemDataAt<ItemData*>(_info.sender_index);
			ItemData* receiverData = *static_cast<ItemBox*>(_info.receiver)->getItemDataAt<ItemData*>(_info.receiver_index);

			// Sender and receiver is the same
			if ((_info.sender == _info.receiver))
			{
				this->dragDropData->senderReceiverIsSame = true;

				if (this->closureFunctionRequestDropAccepted.is_valid())
				{
					NOWA::AppStateManager::LogicCommand logicCommand = [this]()
						{
							try
							{
								luabind::call_function<void>(this->closureFunctionRequestDropRequest, this->dragDropData);
							}
							catch (luabind::error& error)
							{
								luabind::object errorMsg(luabind::from_stack(error.state(), -1));
								std::stringstream msg;
								msg << errorMsg;

								Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[MyGUIItemBoxComponent] Caught error in 'reactOnDropItemAccepted' Error: " + Ogre::String(error.what())
									+ " details: " + msg.str());
							}
						};
					NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
				}
			}
			else
			{
				if (receiverData->getInventoryOwnerId() != this->gameObjectPtr->getId())
				{
					// Calls on receiver the closure function, if does exist
					auto receiverGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(receiverData->getInventoryOwnerId());
					auto myGuiItemBoxCompPtr = NOWA::makeStrongPtr(receiverGameObjectPtr->getComponent<MyGUIItemBoxComponent>());
					if (nullptr != myGuiItemBoxCompPtr)
					{
						if (myGuiItemBoxCompPtr->closureFunctionRequestDropAccepted.is_valid())
						{
							NOWA::AppStateManager::LogicCommand logicCommand = [this, myGuiItemBoxCompPtr]()
								{
									try
									{
										luabind::call_function<void>(myGuiItemBoxCompPtr->closureFunctionRequestDropAccepted, this->dragDropData);
									}
									catch (luabind::error& error)
									{
										luabind::object errorMsg(luabind::from_stack(error.state(), -1));
										std::stringstream msg;
										msg << errorMsg;

										Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[MyGUIItemBoxComponent] Caught error in 'reactOnDropItemAccepted' Error: " + Ogre::String(error.what())
											+ " details: " + msg.str());
									}
								};
							NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
						}
					}
				}
			}

#if 1
			// Shall be done in lua script!
			//if ((_info.sender != _info.receiver))
			//{
			//	// Remove one from the sender
			//	unsigned int currentQuantity = this->quantities[_info.sender_index]->getUInt();
			//	if (currentQuantity > 1)
			//	{
			//		this->quantities[_info.sender_index]->setValue(currentQuantity - 1);
			//	}
			//	else
			//	{
			//		this->setResourceName(_info.sender_index, "");
			//		this->setQuantity(_info.sender_index, 0);
			//		this->setSellValue(_info.sender_index, 0.0f);
			//		this->setBuyValue(_info.sender_index, 0.0f);
			//	}
			//}

			if (false == this->dragDropData->canDrop)
			{
				return;
			}

			// Add also the inventory resource and data for the receiver
			if (receiverData->getInventoryOwnerId() != this->gameObjectPtr->getId())
			{
				auto receiverGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(receiverData->getInventoryOwnerId());
				auto myGuiItemBoxCompPtr = NOWA::makeStrongPtr(receiverGameObjectPtr->getComponent<MyGUIItemBoxComponent>());
				if (nullptr != myGuiItemBoxCompPtr)
				{
					myGuiItemBoxCompPtr->setResourceName(_info.receiver_index, senderData->getResourceName());
					myGuiItemBoxCompPtr->increaseQuantity(_info.receiver_index, 1/*senderData->getQuantity()*/);
					myGuiItemBoxCompPtr->setSellValue(_info.receiver_index, senderData->getSellValue());
					myGuiItemBoxCompPtr->setBuyValue(_info.receiver_index, senderData->getBuyValue());
				}
			}
			else
			{
				// In the same inventory re-arranged the items
				this->setResourceName(_info.receiver_index, senderData->getResourceName());
				this->setQuantity(_info.receiver_index, senderData->getQuantity());
				this->setSellValue(_info.receiver_index, senderData->getSellValue());
				this->setBuyValue(_info.receiver_index, senderData->getBuyValue());

				this->setResourceName(_info.sender_index, "");
				this->setQuantity(_info.sender_index, 0);
				this->setSellValue(_info.sender_index, 0.0f);
				this->setBuyValue(_info.sender_index, 0.0f);
			}

			receiverData->add(senderData);
			if (_info.sender != _info.receiver || receiverData->getInventoryOwnerId() != this->gameObjectPtr->getId())
			{
				senderData->removeQuantity(1);
			}
			else
			{
				// Just moved from a to b in the same inventory, remove completely from the previous place
				senderData->clear();
			}
#endif

			static_cast<ItemBox*>(_info.receiver)->setItemData(_info.receiver_index, receiverData);
			static_cast<ItemBox*>(_info.sender)->setItemData(_info.sender_index, senderData);

			ENQUEUE_RENDER_COMMAND("MyGUIItemBoxComponent::notifyEndDrop",
			{
				this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x - 0.001f, this->size->getVector2().y - 0.001f);
				this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x + 0.001f, this->size->getVector2().y + 0.001f);
			});

			this->dropFinished = true;
		}
	}

	void MyGUIItemBoxComponent::notifyNotifyItem(wraps::BaseLayout* _sender, const MyGUI::IBNotifyItemData& _info)
	{
		if (_info.index != MyGUI::ITEM_NONE)
		{
			if (_info.notify == MyGUI::IBNotifyItemData::NotifyItem::MousePressed)
			{
				ItemData* senderData = *static_cast<ItemBox*>(_sender)->getItemDataAt<ItemData*>(_info.index);

				Ogre::String resourceName;
				if (_info.index < this->resourceNames.size())
					resourceName = this->resourceNames[_info.index]->getString();

				OIS::MouseButtonID buttonId = OIS::MB_Left;
				if (_info.id == MyGUI::MouseButton::Right)
					buttonId = OIS::MB_Right;
				else if (_info.id == MyGUI::MouseButton::Middle)
					buttonId = OIS::MB_Middle;
				else if (_info.id == MyGUI::MouseButton::Button3)
					buttonId = OIS::MB_Button3;
				else if (_info.id == MyGUI::MouseButton::Button4)
					buttonId = OIS::MB_Button4;
				else if (_info.id == MyGUI::MouseButton::Button5)
					buttonId = OIS::MB_Button5;
				else if (_info.id == MyGUI::MouseButton::Button6)
					buttonId = OIS::MB_Button6;
				else if (_info.id == MyGUI::MouseButton::Button7)
					buttonId = OIS::MB_Button7;

				if (nullptr != this->gameObjectPtr->getLuaScript() && true == this->enabled->getBool())
				{
					if (this->mouseButtonClickClosureFunction.is_valid())
					{
						NOWA::AppStateManager::LogicCommand logicCommand = [this, resourceName, buttonId]()
							{
								try
								{
									luabind::call_function<void>(this->mouseButtonClickClosureFunction, resourceName, buttonId);
								}
								catch (luabind::error& error)
								{
									luabind::object errorMsg(luabind::from_stack(error.state(), -1));
									std::stringstream msg;
									msg << errorMsg;

									Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[MyGUIItemBoxComponent] Caught error in 'reactOnMouseButtonClick' Error: " + Ogre::String(error.what())
										+ " details: " + msg.str());
								}
							};
						NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
					}
				}
			}
		}
	}

	void MyGUIItemBoxComponent::requestDrawItem(MyGUI::ItemBox* _sender, MyGUI::Widget* _item, const MyGUI::IBDrawItemInfo& _info)
	{
#if 0
		CellView* cell = *_item->getUserData<CellView*>();
		cell->update(_info, *_sender->getItemDataAt<ItemData*>(_info.index));
#endif

#if 0
		MyGUI::TextBox* text = *_item->getUserData<MyGUI::TextBox*>();
		ItemData* data = *_sender->getItemDataAt<ItemData*>(_info.index);
		if (_info.drag)
		{
			ENQUEUE_RENDER_COMMAND("MyGUIItemBoxComponent::requestDrawItem", _1(info),
			{
				text->setCaption(MyGUI::utility::toString(
					_info.drop_accept ? "#00FF00drag accept" : (_info.drop_refuse ? "#FF0000drag refuse" : "#0000FFdrag miss"),
					"\n#000000data : ", data->getResourceName()));
			});
		}
		else
		{
			text->setCaption(MyGUI::utility::toString(
				_info.drop_accept ? "#00FF00" : (_info.drop_refuse ? "#FF0000" : "#000000"), "index : ", _info.index,
				"\n#000000data : ", data->getResourceName(),
				_info.active ? "\n#00FF00focus" : "\n#800000focus",
				_info.select ? "\n#00FF00select" : "\n#800000select"));
		}
#endif

		/*
		
		void requestCreateWidgetItem(MyGUI::ItemBox* _sender, MyGUI::Widget* _item)
		{
			CellType* cell = new CellType(_item);
			_item->setUserData(cell);
			mListCellView.push_back(cell);
		}

		void requestCoordWidgetItem(MyGUI::ItemBox* _sender, MyGUI::IntCoord& _coord, bool _drop)
		{
			CellType::getCellDimension(_sender, _coord, _drop);
		}

		void requestUpdateWidgetItem(MyGUI::ItemBox* _sender, MyGUI::Widget* _item, const MyGUI::IBDrawItemInfo& _data)
		{
			CellType* cell = *_item->getUserData<CellType*>();
			cell->update(_data, *mBoxItems->getItemDataAt<DataType>(_data.index));
		}
		*/ 
	}

	void MyGUIItemBoxComponent::requestCreateWidgetItem(MyGUI::ItemBox* _sender, MyGUI::Widget* _item)
	{
		MyGUI::TextBox* text = _item->createWidget<MyGUI::TextBox>("TextBox", MyGUI::IntCoord(0, 0, _item->getWidth(), _item->getHeight()), MyGUI::Align::Stretch);
		text->setNeedMouseFocus(false);
		_item->setUserData(text);
	}

	void MyGUIItemBoxComponent::requestCoordItem(MyGUI::ItemBox* _sender, MyGUI::IntCoord& _coord, bool _drag)
	{
		_coord.set(0, 0, 100, 100);
	}

	void MyGUIItemBoxComponent::notifyDropState(wraps::BaseLayout* _sender, MyGUI::DDItemState _state)
	{
		/*if (_state == MyGUI::DDItemState::Refuse) MyGUI::PointerManager::getInstance().setPointer("drop_refuse", _sender->mainWidget());
		else if (_state == MyGUI::DDItemState::Accept) MyGUI::PointerManager::getInstance().setPointer("drop_accept", _sender->mainWidget());
		else if (_state == MyGUI::DDItemState::Miss) MyGUI::PointerManager::getInstance().setPointer("drop", _sender->mainWidget());
		else if (_state == MyGUI::DDItemState::Start) MyGUI::PointerManager::getInstance().setPointer("drop", _sender->mainWidget());
		else if (_state == MyGUI::DDItemState::End) MyGUI::PointerManager::getInstance().setDefaultPointer();*/
	}
	
	void MyGUIItemBoxComponent::notifyToolTip(wraps::BaseLayout* _sender, const MyGUI::ToolTipInfo& _info, ItemData* _data)
	{
		if (true == this->useToolTip->getBool())
		{
			ENQUEUE_RENDER_COMMAND_MULTI("MyGUIItemBoxComponent::notifyToolTip", _2(_info, _data),
			{
				if (_info.type == MyGUI::ToolTipInfo::Show)
				{
					this->toolTip->show(_data);
					this->toolTip->move(_info.point);
				}
				else if (_info.type == MyGUI::ToolTipInfo::Hide)
				{
					this->toolTip->hide();
				}
				else if (_info.type == MyGUI::ToolTipInfo::Move)
				{
					this->toolTip->move(_info.point);
				}
			});
		}
	}

	void MyGUIItemBoxComponent::setResourceLocationName(const Ogre::String& resourceLocationName)
	{
		this->oldResourceLocationName = this->resourceLocationName->getString();
		this->resourceLocationName->setValue(resourceLocationName);
		if (this->oldResourceLocationName != this->resourceLocationName->getString())
		{
			ENQUEUE_RENDER_COMMAND_MULTI("MyGUIItemBoxComponent::setResourceLocationName", _1(resourceLocationName),
			{
				bool success = MyGUI::ResourceManager::getInstance().load(this->resourceLocationName->getString());
				if (false == success)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MyGUIItemBoxComponent] ERROR: Could not load resource: "
						+ this->resourceLocationName->getString() + ", because it cannot be found in '" + this->resourceLocationName->getString() + "', for game object : " + this->gameObjectPtr->getName());
				}
			});
			//MyGUI::ResourceManager::getInstance().load("ItemBox_skin.xml");

			// Reset everything
			if (this->itemCount->getUInt() > 0)
			{
				this->setItemCount(0);
			}
		}
	}

	Ogre::String MyGUIItemBoxComponent::getResourceLocationName(void) const
	{
		return this->resourceLocationName->getString();
	}

	void MyGUIItemBoxComponent::setUseToolTip(bool useToolTip)
	{
		if (true == this->useToolTip->getBool())
		{
			ENQUEUE_RENDER_COMMAND("MyGUIItemBoxComponent::setUseToolTip",
			{
				if (nullptr == this->toolTip)
				{
					this->toolTip = new ToolTip();
					toolTip->hide();
				}
				if (nullptr != this->itemBoxWindow)
				{
					this->itemBoxWindow->getItemBox()->eventToolTip = newDelegate(this, &MyGUIItemBoxComponent::notifyToolTip);
				}
			});
		}
		else
		{
			if (nullptr != this->toolTip)
			{
				ENQUEUE_RENDER_COMMAND("MyGUIItemBoxComponent::setUseToolTip delete",
				{
					delete this->toolTip;
					this->toolTip = nullptr;
				});
			}
		}
	}

	bool MyGUIItemBoxComponent::getUseToolTip(void) const
	{
		return this->useToolTip->getBool();
	}
	
	void MyGUIItemBoxComponent::setItemCount(unsigned int itemCount)
	{
		this->itemCount->setValue(itemCount);
		if (itemCount > 0)
		{
			this->itemCount->addUserData(GameObject::AttrActionSeparator());
		}

		size_t oldSize = this->resourceNames.size();

		if (itemCount > oldSize)
		{
			// Resize the waypoints array for count
			this->resourceNames.resize(itemCount);
			this->quantities.resize(itemCount);
			this->sellValues.resize(itemCount);
			this->buyValues.resize(itemCount);

			for (size_t i = oldSize; i < this->resourceNames.size(); i++)
			{
				this->resourceNames[i] = new Variant(MyGUIItemBoxComponent::AttrResourceName() + Ogre::StringConverter::toString(i), "", this->attributes);
				this->quantities[i] = new Variant(MyGUIItemBoxComponent::AttrQuantity() + Ogre::StringConverter::toString(i), 0, this->attributes);
				this->sellValues[i] = new Variant(MyGUIItemBoxComponent::AttrSellValue() + Ogre::StringConverter::toString(i), 0.0f, this->attributes);
				this->buyValues[i] = new Variant(MyGUIItemBoxComponent::AttrBuyValue() + Ogre::StringConverter::toString(i), 0.0f, this->attributes);
				this->buyValues[i]->addUserData(GameObject::AttrActionSeparator());
				
				if (nullptr != this->itemBoxWindow)
				{
					// TODO: Wait?
					ENQUEUE_RENDER_COMMAND("MyGUIItemBoxComponent::setItemCount",
					{
						this->itemBoxWindow->getItemBox()->addItem(new ItemData(this->gameObjectPtr->getId()));
					});
				}
			}
		}
		else if (itemCount < oldSize)
		{
			this->eraseVariants(this->resourceNames, itemCount);
			this->eraseVariants(this->quantities, itemCount);
			this->eraseVariants(this->sellValues, itemCount);
			this->eraseVariants(this->buyValues, itemCount);

			if (nullptr != this->itemBoxWindow)
			{
				ENQUEUE_RENDER_COMMAND_MULTI("MyGUIItemBoxComponent::setItemCount erase", _2(itemCount, oldSize),
				{
					for (size_t i = itemCount; i < oldSize; i++)
					{
						delete* this->itemBoxWindow->getItemBox()->getItemDataAt<ItemData*>(i);
						this->itemBoxWindow->getItemBox()->removeItem(i);
					}
				});
			}
		}
		if (nullptr != this->itemBoxWindow)
		{
			ENQUEUE_RENDER_COMMAND("MyGUIItemBoxComponent::setItemCount repaint",
			{
				// Trigger item box repaint update, so that resource will be visible
				this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x - 0.001f, this->size->getVector2().y - 0.001f);
				this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x + 0.001f, this->size->getVector2().y + 0.001f);
			});
		}
	}
	
	unsigned int MyGUIItemBoxComponent::getItemCount(void) const
	{
		return this->itemCount->getUInt();
	}
	
	void MyGUIItemBoxComponent::setResourceName(unsigned int index, const Ogre::String& resourceName)
	{
		if (index > this->resourceNames.size())
			index = static_cast<unsigned int>(this->resourceNames.size()) - 1;
		this->resourceNames[index]->setValue(resourceName);
		
		if (nullptr != this->itemBoxWindow)
		{
			ItemData** item = this->itemBoxWindow->getItemBox()->getItemDataAt<ItemData*>(index, true);
			if (nullptr != item)
			{
				(*item)->setResourceName(resourceName);
			}
			ENQUEUE_RENDER_COMMAND("MyGUIItemBoxComponent::setResourceName repaint",
			{
				// Trigger item box repaint update, so that resource will be visible
				this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x - 0.001f, this->size->getVector2().y - 0.001f);
				this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x + 0.001f, this->size->getVector2().y + 0.001f);
			});
		}
	}
	
	Ogre::String MyGUIItemBoxComponent::getResourceName(unsigned int index)
	{
		if (index > this->resourceNames.size())
			return "";
		return this->resourceNames[index]->getString();
	}
	
	void MyGUIItemBoxComponent::setQuantity(unsigned int index, unsigned int quantity)
	{
		if (index > this->quantities.size())
			index = static_cast<unsigned int>(this->quantities.size()) - 1;
		this->quantities[index]->setValue(quantity);
		
		if (nullptr != this->itemBoxWindow)
		{
			ItemData** item = this->itemBoxWindow->getItemBox()->getItemDataAt<ItemData*>(index, true);
			if (nullptr != item)
			{
				(*item)->setQuantity(quantity);
			}
		}
		ENQUEUE_RENDER_COMMAND("MyGUIItemBoxComponent::setQuantity repaint",
		{
			// Trigger item box repaint update, so that resource will be visible
			this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x - 0.001f, this->size->getVector2().y - 0.001f);
			this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x + 0.001f, this->size->getVector2().y + 0.001f);
		});
	}
	
	void MyGUIItemBoxComponent::setQuantity(const Ogre::String& resourceName, unsigned int quantity)
	{
		int index = this->getIndexFromResourceName(resourceName);
		if (-1 != index)
		{
			this->setQuantity(index, quantity);
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MyGUIItemBoxComponent] Could not set quantity, because the resource name: " + resourceName + " does not exist for game object: " + this->gameObjectPtr->getName());
		}

		ENQUEUE_RENDER_COMMAND("MyGUIItemBoxComponent::setQuantity2 repaint",
		{
			// Trigger item box repaint update, so that resource will be visible
			this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x - 0.001f, this->size->getVector2().y - 0.001f);
			this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x + 0.001f, this->size->getVector2().y + 0.001f);
		});
	}
	
	unsigned int MyGUIItemBoxComponent::getQuantity(unsigned int index)
	{
		if (index > this->quantities.size())
			return 0;
		return this->quantities[index]->getUInt();
	}
	
	unsigned int MyGUIItemBoxComponent::getQuantity(const Ogre::String& resourceName)
	{
		int index = this->getIndexFromResourceName(resourceName);
		if (-1 != index)
		{
			return this->getQuantity(index);
		}
		return 0;
	}
	
	int MyGUIItemBoxComponent::getIndexFromResourceName(const Ogre::String& resourceName)
	{
		if (nullptr != this->itemBoxWindow)
		{
			unsigned int i = 0;
			ItemData** item = nullptr;
			// Item does exist, increase count
			do
			{
				if (i >= this->itemBoxWindow->getItemBox()->getItemBox()->getItemCount())
				{
					break;
				}
				item = this->itemBoxWindow->getItemBox()->getItemDataAt<ItemData*>(i, false);
				if (nullptr != item && (*item)->getResourceName() == resourceName)
				{
					return i;
				}
				i++;
			} while (nullptr != item);
		}
		return -1;
	}

	void MyGUIItemBoxComponent::addQuantity(const Ogre::String& resourceName, unsigned int quantity)
	{
		if (nullptr != this->itemBoxWindow)
		{
			ItemData** item = nullptr;
			bool foundResource = false;
			int freeIndex = -1;

			for (unsigned int i = 0; i < this->itemCount->getUInt(); i++)
			{
				item = this->itemBoxWindow->getItemBox()->getItemDataAt<ItemData*>(i, false);
				if (nullptr != item)
				{
					if ((*item)->getResourceName() == resourceName)
					{
						this->setResourceName(i, resourceName);
						this->setQuantity(i, this->quantities[i]->getUInt() + quantity);
						foundResource = true;
						break;
					}
				}
			}

			if (false == foundResource)
			{
				// Check if there is a free index
				for (unsigned int i = 0; i < this->itemCount->getUInt(); i++)
				{
					item = this->itemBoxWindow->getItemBox()->getItemDataAt<ItemData*>(i, false);
					if (nullptr != item)
					{
						if (true == (*item)->getResourceName().empty())
						{
							freeIndex = i;
							break;
						}
					}
				}
				// Add new resource to item

				// Is there a free index?
				if (-1 != freeIndex)
				{
					// Image and description is associated in resource xml!
					this->setResourceName(freeIndex, resourceName);
					this->setQuantity(freeIndex, quantity); // Attention: is quantity incremented
				}
				else
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MyGUIItemBoxComponent] ERROR: Could not add resource: " 
						+ resourceName + ", because there is no free index anymore, because the inventory is full, for game object: " + this->gameObjectPtr->getName());
				}
			}
			ENQUEUE_RENDER_COMMAND("MyGUIItemBoxComponent::addQuantity repaint",
			{
				// Trigger item box repaint update, so that resource will be visible
				this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x - 0.001f, this->size->getVector2().y - 0.001f);
				this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x + 0.001f, this->size->getVector2().y + 0.001f);
			});
		}
	}

	void MyGUIItemBoxComponent::increaseQuantity(const Ogre::String& resourceName, unsigned int quantity)
	{
		this->setQuantity(resourceName, this->getQuantity(resourceName) + quantity);
	}

	void MyGUIItemBoxComponent::increaseQuantity(unsigned int index, unsigned int quantity)
	{
		this->setQuantity(index, this->quantities[index]->getUInt() + quantity);
	}

	void MyGUIItemBoxComponent::decreaseQuantity(const Ogre::String& resourceName, unsigned int quantity)
	{
		this->setQuantity(resourceName, this->getQuantity(resourceName) - quantity);
	}

	void MyGUIItemBoxComponent::decreaseQuantity(unsigned int index, unsigned int quantity)
	{
		this->setQuantity(index, this->quantities[index]->getUInt() - quantity);
	}

	void MyGUIItemBoxComponent::removeQuantity(const Ogre::String& resourceName, unsigned int quantity)
	{
		if (nullptr != this->itemBoxWindow)
		{
			ItemData** item = nullptr;

			int index = this->getIndexFromResourceName(resourceName);
			if (-1 != index)
			{
				item = this->itemBoxWindow->getItemBox()->getItemDataAt<ItemData*>(index, false);

				unsigned int currentQuantity = (*item)->getQuantity();
				int differenceQuantity = currentQuantity - quantity;
				// Enough quantity, so just adapt
				if (differenceQuantity > 0)
				{
					(*item)->setQuantity(differenceQuantity);
					this->quantities[index]->setValue(static_cast<unsigned int>(differenceQuantity));
				}
				else
				{
					// No quantity, remove item from inventory
					this->resourceNames[index]->setValue("");
					this->quantities[index]->setValue(static_cast<unsigned int>(0));
					this->sellValues[index]->setValue(0.0f);
					this->buyValues[index]->setValue(0.0f);

					// if (this->itemCount->getUInt() > 0)
					// 	this->itemCount->setValue(this->itemCount->getUInt() - 1);
					// Hier eher die resource entfernen, die inventarplätze bleiben ja gleich
					// this->itemBoxWindow->getItemBox()->removeItem(i);
					(*item)->clear();
				}
			}
			ENQUEUE_RENDER_COMMAND("MyGUIItemBoxComponent::removeQuantity repaint",
			{
				this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x - 0.001f, this->size->getVector2().y - 0.001f);
				this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x + 0.001f, this->size->getVector2().y + 0.001f);
			});
		}
	}

	void MyGUIItemBoxComponent::setSellValue(unsigned int index, Ogre::Real sellValue)
	{
		if (index > this->sellValues.size())
			index = static_cast<unsigned int>(this->sellValues.size()) - 1;
		this->sellValues[index]->setValue(sellValue);

		if (nullptr != this->itemBoxWindow)
		{
			ItemData** item = this->itemBoxWindow->getItemBox()->getItemDataAt<ItemData*>(index, true);
			if (nullptr != item)
			{
				(*item)->setSellValue(sellValue);
			}
		}

		ENQUEUE_RENDER_COMMAND("MyGUIItemBoxComponent::setSellValue repaint",
		{
			// Trigger item box repaint update, so that resource will be visible
			this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x - 0.001f, this->size->getVector2().y - 0.001f);
			this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x + 0.001f, this->size->getVector2().y + 0.001f);
		});
	}

	void MyGUIItemBoxComponent::setSellValue(const Ogre::String& resourceName, Ogre::Real sellValue)
	{
		int index = this->getIndexFromResourceName(resourceName);
		if (-1 != index)
		{
			this->setSellValue(index, sellValue);
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MyGUIItemBoxComponent] Could not set sell value, because the resource name: " + resourceName + " does not exist for game object: " + this->gameObjectPtr->getName());
		}

		ENQUEUE_RENDER_COMMAND("MyGUIItemBoxComponent::setSellValue2 repaint",
		{
			// Trigger item box repaint update, so that resource will be visible
			this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x - 0.001f, this->size->getVector2().y - 0.001f);
			this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x + 0.001f, this->size->getVector2().y + 0.001f);
		});
	}

	Ogre::Real MyGUIItemBoxComponent::getSellValue(unsigned int index)
	{
		if (index > this->sellValues.size())
			return 0.0f;
		return this->sellValues[index]->getReal();
	}

	Ogre::Real MyGUIItemBoxComponent::getSellValue(const Ogre::String& resourceName)
	{
		int index = this->getIndexFromResourceName(resourceName);
		if (-1 != index)
		{
			return this->getSellValue(index);
		}
		return 0.0f;
	}

	void MyGUIItemBoxComponent::setBuyValue(unsigned int index, Ogre::Real buyValue)
	{
		if (index > this->buyValues.size())
			index = static_cast<unsigned int>(this->buyValues.size()) - 1;
		this->buyValues[index]->setValue(buyValue);

		if (nullptr != this->itemBoxWindow)
		{
			ItemData** item = this->itemBoxWindow->getItemBox()->getItemDataAt<ItemData*>(index, true);
			if (nullptr != item)
			{
				(*item)->setBuyValue(buyValue);
			}
		}
		ENQUEUE_RENDER_COMMAND("MyGUIItemBoxComponent::setBuyValue repaint",
		{
			// Trigger item box repaint update, so that resource will be visible
			this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x - 0.001f, this->size->getVector2().y - 0.001f);
			this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x + 0.001f, this->size->getVector2().y + 0.001f);
		});
	}

	void MyGUIItemBoxComponent::setBuyValue(const Ogre::String& resourceName, Ogre::Real buyValue)
	{
		int index = this->getIndexFromResourceName(resourceName);
		if (-1 != index)
		{
			this->setBuyValue(index, buyValue);
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MyGUIItemBoxComponent] Could not set buy value, because the resource name: " + resourceName + " does not exist for game object: " + this->gameObjectPtr->getName());
		}
		ENQUEUE_RENDER_COMMAND("MyGUIItemBoxComponent::setBuyValue2 repaint",
		{
			// Trigger item box repaint update, so that resource will be visible
			this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x - 0.001f, this->size->getVector2().y - 0.001f);
			this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x + 0.001f, this->size->getVector2().y + 0.001f);
		});
	}

	Ogre::Real MyGUIItemBoxComponent::getBuyValue(unsigned int index)
	{
		if (index > this->buyValues.size())
			return 0.0f;
		return this->buyValues[index]->getReal();
	}

	Ogre::Real MyGUIItemBoxComponent::getBuyValue(const Ogre::String& resourceName)
	{
		int index = this->getIndexFromResourceName(resourceName);
		if (-1 != index)
		{
			return this->getBuyValue(index);
		}
		return 0.0f;
	}

	void MyGUIItemBoxComponent::setAllowDragDrop(bool allowDragDrop)
	{
		this->allowDragDrop->setValue(allowDragDrop);
	}

	bool MyGUIItemBoxComponent::getAllowDragDrop(void) const
	{
		return this->allowDragDrop->getBool();
	}

	void MyGUIItemBoxComponent::clearItems(void)
	{
		if (nullptr == this->itemBoxWindow)
			return;

		for (unsigned int i = 0; i < this->itemCount->getUInt(); i++)
		{
			this->setQuantity(i, 0);
		}
		for (unsigned int i = 0; i < this->itemCount->getUInt(); i++)
		{
			this->setSellValue(i, 0.0f);
		}
		for (unsigned int i = 0; i < this->itemCount->getUInt(); i++)
		{
			this->setBuyValue(i, 0.0f);
		}
		ENQUEUE_RENDER_COMMAND("MyGUIItemBoxComponent::clearItems",
		{
			for (unsigned int i = 0; i < this->itemCount->getUInt(); i++)
			{
				delete* this->itemBoxWindow->getItemBox()->getItemDataAt<ItemData*>(0);
				this->itemBoxWindow->getItemBox()->removeItem(0);
			}
			for (unsigned int i = 0; i < this->itemCount->getUInt(); i++)
			{
				this->itemBoxWindow->getItemBox()->addItem(new ItemData(this->gameObjectPtr->getId()));
			}
		});
	}

	void MyGUIItemBoxComponent::reactOnDropItemRequest(luabind::object closureFunction)
	{
		this->closureFunctionRequestDropRequest = closureFunction;
	}

	void MyGUIItemBoxComponent::reactOnDropItemAccepted(luabind::object closureFunction)
	{
		this->closureFunctionRequestDropAccepted = closureFunction;
	}

	//////////////////////////////////////////////////////////////////////////////////
	
	DragDropData::DragDropData()
		: quantity(0),
		sellValue(0.0f),
		buyValue(0.0f),
		canDrop(false),
		senderReceiverIsSame(false),
		senderInventoryId(0L)
	{
	}

	DragDropData::~DragDropData()
	{
	}

	Ogre::String DragDropData::getResourceName(void) const
	{
		return this->resourceName;
	}

	unsigned int DragDropData::getQuantity(void) const
	{
		return this->quantity;
	}

	Ogre::Real DragDropData::getSellValue(void) const
	{
		return this->sellValue;
	}

	Ogre::Real DragDropData::getBuyValue(void) const
	{
		return this->buyValue;
	}

	bool DragDropData::getSenderReceiverIsSame(void) const
	{
		return this->senderReceiverIsSame;
	}

	unsigned long DragDropData::getSenderInventoryId(void) const
	{
		return this->senderInventoryId;
	}

	void DragDropData::setCanDrop(bool canDrop)
	{
		this->canDrop = canDrop;
	}

	bool DragDropData::getCanDrop(void) const
	{
		return this->canDrop;
	}

	void DragDropData::clear(void)
	{
		this->resourceName.clear();
		this->quantity = 0;
		this->sellValue = 0.0f;
		this->buyValue = 0.0f;
		this->canDrop = false;
		this->senderInventoryId = 0L;
		this->senderReceiverIsSame = false;
	}

}; // namespace end
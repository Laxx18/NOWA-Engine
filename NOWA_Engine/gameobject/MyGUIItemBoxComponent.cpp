#include "NOWAPrecompiled.h"
#include "MyGUIItemBoxComponent.h"
#include "LuaScriptComponent.h"
#include "MyGUI_LayerManager.h"
#include "main/AppStateManager.h"
#include "modules/LuaScriptApi.h"
#include "utilities/MathHelper.h"
#include "utilities/XMLConverter.h"

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
            {
                this->itemName = node->getContent();
            }
            else if (node->getName() == "Description")
            {
                this->itemDescription = node->getContent();
            }
            else if (node->getName() == "Image")
            {
                this->itemResourceImage = node->findAttribute("RefID");
            }
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

    ItemData::ItemData(const Ogre::String& resourceName, unsigned int quantity, Ogre::Real sellValue, Ogre::Real buyValue) :
        resourceName(resourceName),
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
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                "[MyGUIItemBoxComponent] ERROR: Could not add resource: " + resourceName + ", because it cannot be found, or the XML container is wrong. Check your resource location name XML");
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
            if (false == this->resourceInfo->getItemResourceImage().empty())
            {
                // Already on render thread — set directly, no nested enqueue
                MyGUI::IResource* imageResource = manager.getByName(this->resourceInfo->getItemResourceImage(), false);
                if (nullptr != imageResource)
                {
                    this->resourceImage = imageResource->castType<MyGUI::ResourceImageSet>(false);
                }
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

    ToolTip::ToolTip() : BaseLayout("ToolTip.layout")
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
        {
            return;
        }

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

        _coord.set(0, 0, 74, 74);
    }

    CellView::CellView(MyGUI::Widget* _parent) : wraps::BaseCellView<ItemData*>("CellView.layout", _parent)
    {
        this->uniqueId = NOWA::makeUniqueID();

        NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
        {
            assignWidget(mImageBack, "image_Back");
            assignWidget(mImageBorder, "image_Border");
            assignWidget(mImageItem, "image_Item");
            assignWidget(mTextBack, "text_Back");
            assignWidget(mTextFront, "text_Front");
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ItemBox::CellView::CellView");
    }

    CellView::~CellView()
    {
    }

    void CellView::update(const MyGUI::IBDrawItemInfo& _info, ItemData* _data)
    {
        // Snapshot everything needed BEFORE any lambda captures it.
        // _data may be deleted or cleared by the time the render thread runs.
        const bool isEmpty = _data->isEmpty();
        const unsigned int quantity = isEmpty ? 0 : _data->getQuantity();
        // Only read the image pointer if the data is actually valid right now
        MyGUI::ResourceImageSetPtr imagePtr = (!isEmpty && _data->getInfo() != nullptr) ? _data->getImage() : nullptr;

        if (_info.update)
        {
            if (!isEmpty && imagePtr != nullptr)
            {
                mImageItem->setItemResourcePtr(imagePtr);
                mImageItem->setItemGroup("States");
                mImageItem->setVisible(true);
            }
            else
            {
                mImageItem->setVisible(false);
            }
            mTextBack->setCaption(((quantity > 1) && (!_info.drag)) ? MyGUI::utility::toString(quantity) : "");
            mTextFront->setCaption(((quantity > 1) && (!_info.drag)) ? MyGUI::utility::toString(quantity) : "");

            // static resource pointers are fine to keep as-is
            static MyGUI::ResourceImageSetPtr resourceBack = nullptr;
            static MyGUI::ResourceImageSetPtr resourceSelect = nullptr;
            if (resourceBack == nullptr)
            {
                resourceBack = MyGUI::ResourceManager::getInstance().getByName("pic_ItemBackImage")->castType<MyGUI::ResourceImageSet>(false);
            }
            if (resourceSelect == nullptr)
            {
                resourceSelect = MyGUI::ResourceManager::getInstance().getByName("pic_ItemSelectImage")->castType<MyGUI::ResourceImageSet>(false);
            }

            mImageBack->setItemResourcePtr(resourceBack);
            mImageBack->setItemGroup("States");
            mImageBorder->setItemResourcePtr(resourceSelect);
            mImageBorder->setItemGroup("States");
        }

        if (_info.drag && _data->getInfo() != nullptr)
        {
            mImageBack->setItemName("None");
            mImageBorder->setItemName("None");

            if (!isEmpty && imagePtr != nullptr)
            {
                if (_info.drop_refuse)
                {
                    mImageItem->setItemName("Refuse");
                }
                else if (_info.drop_accept)
                {
                    mImageItem->setItemName("Accept");
                }
                else
                {
                    mImageItem->setItemName("Normal");
                }
                mImageItem->setVisible(true);
            }
            else
            {
                mImageItem->setVisible(false);
            }
        }
        else
        {
            if (_info.active)
            {
                mImageBack->setItemName(_info.select ? "Select" : "Active");
            }
            else if (_info.select)
            {
                mImageBack->setItemName("Pressed");
            }
            else
            {
                mImageBack->setItemName("Normal");
            }

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

            if (!isEmpty && imagePtr != nullptr)
            {
                mImageItem->setItemName("Normal");
                mImageItem->setVisible(true);
            }
            else
            {
                mImageItem->setVisible(false);
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ItemBox::ItemBox(MyGUI::Widget* _parent)
        : wraps::BaseItemBox<CellView>(_parent)
    {
    }

    ItemBox::~ItemBox()
    {
        NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
        {
            MyGUI::ItemBox* box = getItemBox();
            size_t count = box->getItemCount();
            for (size_t pos = 0; pos < count; ++pos)
            {
                delete *box->getItemDataAt<ItemData*>(pos);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ItemBox::~ItemBox");
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    ItemBoxWindow::ItemBoxWindow(const std::string& _layout)
        : BaseLayout(_layout)
    {
        NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
        {
            assignBase(mItemBox, "box_Items");
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "ItemBoxWindow::ItemBoxWindow");
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    using namespace rapidxml;
    using namespace luabind;

    MyGUIItemBoxComponent::MyGUIItemBoxComponent()
        : MyGUIWindowComponent(),
        toolTip(nullptr),
        itemBoxWindow(nullptr),
        sharedItemBoxWindow(nullptr),
        selectedSlotIndex(-1),
        dragDropData(new DragDropData),
        dropFinished(false),
        pendingSprite(nullptr),
        spriteAnimationPending(false),
        pendingButtonId(OIS::MB_Left),
        pendingSlotIndex(0)
    {
        this->skin->setVisible(false);

        this->layer->setListSelectedValue("Overlapped");

        this->resourceLocationName = new Variant(MyGUIItemBoxComponent::AttrResourceLocationName(), "InventoryItemsTemplateClean.xml", this->attributes);
        this->useToolTip = new Variant(MyGUIItemBoxComponent::AttrUseToolTip(), true, this->attributes);
        this->allowDragDrop = new Variant(MyGUIItemBoxComponent::AttrAllowDragDrop(), true, this->attributes);
        this->style = new Variant(MyGUIComponent::AttrStyle(), std::vector<Ogre::String>{"ItemBox", "ItemBoxInventory"}, this->attributes);
        this->style->setListSelectedValue("ItemBox");
        this->itemCount = new Variant(MyGUIItemBoxComponent::AttrItemCount(), 0, this->attributes);

        this->resourceNames.resize(this->itemCount->getUInt());
        this->quantities.resize(this->itemCount->getUInt());
        this->sellValues.resize(this->itemCount->getUInt());
        this->buyValues.resize(this->itemCount->getUInt());
        this->gameObjectIds.resize(this->itemCount->getUInt());
        this->spriteComponentIndices.resize(this->itemCount->getUInt());
        this->spriteComponents.resize(this->itemCount->getUInt(), nullptr);
        // Automatically also triggers needRefresh!
        this->resourceLocationName->addUserData(GameObject::AttrActionFileOpenDialog(), "Essential");
        // Since when node item count is changed, the whole properties must be refreshed, so that new field may come for item tracks
        this->itemCount->addUserData(GameObject::AttrActionNeedRefresh());

        this->position->setValue(Ogre::Vector2(0.0f, 0.8f));
        this->size->setValue(Ogre::Vector2(0.2f, 1.0f));
    }

    MyGUIItemBoxComponent::~MyGUIItemBoxComponent()
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUIItemBoxComponent] Destructor MyGUI item box component for game object: " + this->gameObjectPtr->getName());

        NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
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
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIItemBoxComponent::~MyGUIItemBoxComponent");
    }

    bool MyGUIItemBoxComponent::init(rapidxml::xml_node<>*& propertyElement)
    {
        // Tells the system, that this component shall be fully restored, because its to complex to restore just some attributes
        // This is usually the case if there is a list of attributes involved
        this->customDataString = GameObjectComponent::AttrCustomDataNewCreation();

        bool success = MyGUIWindowComponent::init(propertyElement);

        if ("ToolTip" == this->layer->getListSelectedValue())
        {
            // For tooltip -> pick is set to false
            // When pick=false, getLayerItemByPoint returns nullptr for every single pixel of that layer.
            // MyGUI's InputManager calls this to decide which widget gets mouse focus. Since it always returns null for the ToolTip layer,
            // the ItemBox's mClient and its cell widgets never receive mouse focus, so ItemBox::notifyMouseButtonPressed is never called.
            this->layer->setListSelectedValue("Overlapped");
        }

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
            this->gameObjectIds.resize(this->itemCount->getUInt());
            this->spriteComponentIndices.resize(this->itemCount->getUInt());
            this->spriteComponents.resize(this->itemCount->getUInt());
        }

        for (size_t i = 0; i < this->itemCount->getUInt(); i++)
        {
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ResourceName" + Ogre::StringConverter::toString(i))
            {
                if (nullptr == this->resourceNames[i])
                {
                    this->resourceNames[i] = new Variant(MyGUIItemBoxComponent::AttrResourceName() + Ogre::StringConverter::toString(i), XMLConverter::getAttrib(propertyElement, "data"), this->attributes);
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
                    this->quantities[i] = new Variant(MyGUIItemBoxComponent::AttrQuantity() + Ogre::StringConverter::toString(i), XMLConverter::getAttribUnsignedInt(propertyElement, "data"), this->attributes);
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
                    this->sellValues[i] = new Variant(MyGUIItemBoxComponent::AttrSellValue() + Ogre::StringConverter::toString(i), XMLConverter::getAttribReal(propertyElement, "data"), this->attributes);
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
                    this->buyValues[i] = new Variant(MyGUIItemBoxComponent::AttrBuyValue() + Ogre::StringConverter::toString(i), XMLConverter::getAttribReal(propertyElement, "data"), this->attributes);
                }
                else
                {
                    this->buyValues[i]->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
                }
                propertyElement = propertyElement->next_sibling("property");
            }
            // Backward compatibility. No buy values loaded, create new ones
            if (nullptr == this->buyValues[i])
            {
                this->buyValues[i] = new Variant(MyGUIItemBoxComponent::AttrBuyValue() + Ogre::StringConverter::toString(i), XMLConverter::getAttribReal(propertyElement, "data"), this->attributes);
            }

            // Optional per-slot associated GameObject id (0 = none)
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "GameObjectId" + Ogre::StringConverter::toString(i))
            {
                if (nullptr == this->gameObjectIds[i])
                {
                    this->gameObjectIds[i] = new Variant(MyGUIItemBoxComponent::AttrGameObjectId() + Ogre::StringConverter::toString(i), XMLConverter::getAttribUnsignedLong(propertyElement, "data"), this->attributes);
                }
                else
                {
                    this->gameObjectIds[i]->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
                }
                propertyElement = propertyElement->next_sibling("property");
            }
            else
            {
                this->gameObjectIds[i] = new Variant(MyGUIItemBoxComponent::AttrGameObjectId() + Ogre::StringConverter::toString(i), static_cast<unsigned long>(0), this->attributes);
            }
            this->gameObjectIds[i]->setDescription("Optional: id of the GameObject associated with this slot (e.g. a building template). Passed to reactOnMouseButtonClick. 0 = none.");

            // Optional per-slot sprite component occurrence index (-1 = none)
            if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == MyGUIItemBoxComponent::AttrSpriteComponentIndex() + Ogre::StringConverter::toString(i))
            {
                if (nullptr == this->spriteComponentIndices[i])
                {
                    this->spriteComponentIndices[i] = new Variant(MyGUIItemBoxComponent::AttrSpriteComponentIndex() + Ogre::StringConverter::toString(i), XMLConverter::getAttribUnsignedInt(propertyElement, "data"), this->attributes);
                }
                else
                {
                    this->spriteComponentIndices[i]->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
                }
                propertyElement = propertyElement->next_sibling("property");
            }
            else
            {
                this->spriteComponentIndices[i] = new Variant(MyGUIItemBoxComponent::AttrSpriteComponentIndex() + Ogre::StringConverter::toString(i), static_cast<int>(-1), this->attributes);
            }
            this->spriteComponentIndices[i]->setDescription("Optional: Occurrence index of a MyGuiSpriteComponent on the same game object to overlay and animate over this slot on click. See in NOWA-Design: MyGuiSpriteComponent (i). Setting to -1 = disabled.");
            this->spriteComponentIndices[i]->addUserData(GameObject::AttrActionSeparator());
        }

        return success;
    }

    GameObjectCompPtr MyGUIItemBoxComponent::clone(GameObjectPtr clonedGameObjectPtr)
    {
        MyGUIItemBoxCompPtr clonedCompPtr(boost::make_shared<MyGUIItemBoxComponent>());

        clonedCompPtr->bIsCloning = true;

        clonedGameObjectPtr->addComponent(clonedCompPtr);
        clonedCompPtr->setOwner(clonedGameObjectPtr);

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
        clonedCompPtr->setStyle(this->style->getListSelectedValue());

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
            clonedCompPtr->setGameObjectId(i, this->gameObjectIds[i]->getULong());
            clonedCompPtr->setSpriteComponentIndex(i, this->spriteComponentIndices[i]->getInt());
            
        }

        clonedCompPtr->setCommonWidget(this->commonWidget->getBool());

        GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
        return clonedCompPtr;
    }

    bool MyGUIItemBoxComponent::postInit(void)
    {
        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[MyGUIItemBoxComponent] Init MyGUI item box component for game object: " + this->gameObjectPtr->getName());

        this->bIsCloning = false;
        this->createWidgetInParent = false;

        std::string resourceCategory = MyGUI::ResourceManager::getInstance().getCategoryName();
        MyGUI::FactoryManager::getInstance().registerFactory<ResourceItemInfo>(resourceCategory);

        bool success = MyGUI::ResourceManager::getInstance().load(this->resourceLocationName->getString());
        if (false == success)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MyGUIItemBoxComponent] ERROR: Could not load resource: " + this->resourceLocationName->getString() + " for game object: " + this->gameObjectPtr->getName());
        }

        if (true == this->commonWidget->getBool())
        {
            // ── Shared widget path ────────────────────────────────────────────────
            auto* goc = AppStateManager::getSingletonPtr()->getGameObjectController();
            const Ogre::String key = this->getSharedWidgetKey();

            if (false == goc->hasSharedWidget(this->getClassName(), key))
            {
                this->createAndRegisterSharedWidgetImpl(goc, key);
            }
            else
            {
                goc->registerSharedWidget(this->getClassName(), key, goc->getSharedWidget(this->getClassName(), key));
            }

            // Size variant arrays — no widget created yet (data-only)
            this->setItemCount(this->itemCount->getUInt());

            // Populate resourceNameToIndex directly so getQuantity(name) works
            // during simulation without needing widget operations
            for (unsigned int i = 0; i < this->itemCount->getUInt(); i++)
            {
                if (nullptr != this->resourceNames[i] && !this->resourceNames[i]->getString().empty())
                {
                    this->resourceNameToIndex[this->resourceNames[i]->getString()] = i;
                }
            }

            this->itemBoxWindow = nullptr;
            this->widget = nullptr;
            this->lastBuiltSkin = this->skin->getListSelectedValue();
            this->lastBuiltStyle = this->style->getListSelectedValue();

            return MyGUIComponent::postInit();
        }

        // ── Normal (own widget) path — identical to old working code ─────────────
        this->itemBoxWindow = new ItemBoxWindow(this->style->getListSelectedValue() + ".layout");

        this->itemBoxWindow->getItemBox()->eventStartDrag = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyStartDrop);
        this->itemBoxWindow->getItemBox()->eventRequestDrop = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyRequestDrop);
        this->itemBoxWindow->getItemBox()->eventDropResult = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyEndDrop);
        this->itemBoxWindow->getItemBox()->eventChangeDDState = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyDropState);
        this->itemBoxWindow->getItemBox()->eventNotifyItem = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyNotifyItem);
        this->itemBoxWindow->getItemBox()->eventToolTip = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyToolTip);

        this->widget = this->itemBoxWindow->getMainWidget();
        MyGUI::LayerManager::getInstance().attachToLayerNode(this->layer->getListSelectedValue(), this->widget);

        this->setUseToolTip(this->useToolTip->getBool());
        this->setItemCount(this->itemCount->getUInt());

        this->resourceNames.resize(this->itemCount->getUInt());
        this->quantities.resize(this->itemCount->getUInt());
        this->sellValues.resize(this->itemCount->getUInt());
        this->buyValues.resize(this->itemCount->getUInt());
        this->gameObjectIds.resize(this->itemCount->getUInt());

        for (unsigned int i = 0; i < this->itemCount->getUInt(); i++)
        {
            this->itemBoxWindow->getItemBox()->addItem(new ItemData(this->gameObjectPtr->getId()));

            if (nullptr != this->resourceNames[i] && nullptr != this->quantities[i])
            {
                if (this->quantities[i]->getUInt() > 0)
                {
                    // setResourceName also populates resourceNameToIndex
                    this->setResourceName(i, this->resourceNames[i]->getString());
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
            if (nullptr != this->gameObjectIds[i])
            {
                this->setGameObjectId(i, this->gameObjectIds[i]->getULong());
            }
        }

        this->lastBuiltSkin = this->skin->getListSelectedValue();
        this->lastBuiltStyle = this->style->getListSelectedValue();

        return MyGUIWindowComponent::postInit();
    }

    void MyGUIItemBoxComponent::onRemoveComponent(void)
    {
        if (true == this->commonWidget->getBool())
        {
            this->itemBoxWindow = nullptr;
            this->widget = nullptr;

            auto* goc = AppStateManager::getSingletonPtr()->getGameObjectController();
            if (nullptr != goc)
            {
                // ALWAYS use getSharedWidgetKey()
                const Ogre::String key = this->getSharedWidgetKey();

                void* owner = goc->getSharedWidgetOwner(this->getClassName(), key);
                if (owner == this)
                {
                    goc->setSharedWidgetOwner(this->getClassName(), key, nullptr);

                    MyGUI::Widget* shared = goc->getSharedWidget(this->getClassName(), key);
                    if (nullptr != shared)
                    {
                        NOWA::GraphicsModule::RenderCommand cmd = [shared]()
                        {
                            shared->setVisible(false);
                        };
                        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "MyGUIItemBoxComponent::onRemoveComponent hide");
                    }
                }

                // Grab ItemBoxWindow before unregister removes the entry
                ItemBoxWindow* sharedWindow = static_cast<ItemBoxWindow*>(goc->getSharedWidgetUserData(this->getClassName(), key));

                goc->unregisterSharedWidget(this->getClassName(), key);

                // If last instance: delete the ItemBoxWindow wrapper
                if (nullptr != sharedWindow && false == goc->hasSharedWidget(this->getClassName(), key))
                {
                    NOWA::GraphicsModule::RenderCommand cmd = [sharedWindow]()
                    {
                        sharedWindow->getItemBox()->eventStartDrag = nullptr;
                        sharedWindow->getItemBox()->eventRequestDrop = nullptr;
                        sharedWindow->getItemBox()->eventDropResult = nullptr;
                        sharedWindow->getItemBox()->eventChangeDDState = nullptr;
                        sharedWindow->getItemBox()->eventNotifyItem = nullptr;
                        sharedWindow->getItemBox()->eventToolTip = nullptr;
                        delete sharedWindow;
                    };
                    NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "MyGUIItemBoxComponent::onRemoveComponent delete window");
                }
            }
        }
        else
        {
            if (nullptr != this->itemBoxWindow)
            {
                this->itemBoxWindow->getItemBox()->eventStartDrag = nullptr;
                this->itemBoxWindow->getItemBox()->eventRequestDrop = nullptr;
                this->itemBoxWindow->getItemBox()->eventDropResult = nullptr;
                this->itemBoxWindow->getItemBox()->eventChangeDDState = nullptr;
                this->itemBoxWindow->getItemBox()->eventNotifyItem = nullptr;
                this->itemBoxWindow->getItemBox()->eventToolTip = nullptr;
                delete this->itemBoxWindow;
                this->itemBoxWindow = nullptr;
                this->widget = nullptr;
            }
        }

        MyGUIWindowComponent::onRemoveComponent();
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

        // Resolve per-slot MyGuiSpriteComponent pointers (1-based; 0 = disabled)
        this->spriteComponents.resize(this->itemCount->getUInt(), nullptr);
        for (unsigned int i = 0; i < this->itemCount->getUInt(); i++)
        {
            this->spriteComponents[i] = nullptr;
            if (nullptr == this->spriteComponentIndices[i])
            {
                continue;
            }
            const unsigned int occIdx = this->spriteComponentIndices[i]->getInt();
            if (occIdx == -1)
            {
                continue;
            }
            auto spriteCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponentWithOccurrence<MyGUISpriteComponentBase>(occIdx));
            if (nullptr != spriteCompPtr)
            {
                this->spriteComponents[i] = spriteCompPtr.get();
                // Ensure the sprite does NOT repeat so we get the finished event
                this->spriteComponents[i]->setRepeat(false);
            }
            else
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MyGUIItemBoxComponent] Slot " + Ogre::StringConverter::toString(i) + ": SpriteComponentIndex=" + Ogre::StringConverter::toString(occIdx) +
                                                                                        " but no MyGuiSpriteComponent found at occurrence " + Ogre::StringConverter::toString(occIdx) + " on game object: " + this->gameObjectPtr->getName());
            }
        }

        // Listen for sprite animation finished to fire deferred Lua callbacks
        AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &MyGUIItemBoxComponent::handleSpriteAnimationFinished), EventDataSpriteAnimationFinished::getStaticEventType());

        return success;
    }

    bool MyGUIItemBoxComponent::disconnect(void)
    {
        AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &MyGUIItemBoxComponent::handleSpriteAnimationFinished), EventDataSpriteAnimationFinished::getStaticEventType());

        // Clear sprite state
        this->pendingSprite = nullptr;
        this->spriteAnimationPending = false;
        for (auto& sp : this->spriteComponents)
        {
            sp = nullptr;
        }

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
                else if (MyGUIItemBoxComponent::AttrGameObjectId() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setGameObjectId(i, attribute->getULong());
                }
                else if (MyGUIItemBoxComponent::AttrSpriteComponentIndex() + Ogre::StringConverter::toString(i) == attribute->getName())
                {
                    this->setSpriteComponentIndex(i, attribute->getInt());
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

            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "GameObjectId" + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->gameObjectIds[i]->getULong())));
            propertiesXML->append_node(propertyXML);

            propertyXML = doc.allocate_node(node_element, "property");
            propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
            propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, MyGUIItemBoxComponent::AttrSpriteComponentIndex() + Ogre::StringConverter::toString(i))));
            propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, nullptr != this->spriteComponentIndices[i] ? this->spriteComponentIndices[i]->getInt() : -1)));
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
        if (false == this->isSimulating)
        {
            return;
        }

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

                        Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[MyGUIItemBoxComponent] Caught error in 'reactOnDropItemRequest' Error: " + Ogre::String(error.what()) + " details: " + msg.str());
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

                                Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[MyGUIItemBoxComponent] Caught error in 'reactOnDropItemRequest' Error: " + Ogre::String(error.what()) + " details: " + msg.str());
                            }
                        };
                        NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
                    }
                }
            }
        }

        _result = true == this->dragDropData->canDrop && (receiverData->isEmpty() || receiverData->compare(senderData));

        NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
        {
            // Trigger item box repaint update, so that resource will be visible
            this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x - 0.001f, this->size->getVector2().y - 0.001f);
            this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x + 0.001f, this->size->getVector2().y + 0.001f);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIItemBoxComponent::notifyRequestDrop");
    }

    void MyGUIItemBoxComponent::notifyEndDrop(wraps::BaseLayout* _sender, wraps::DDItemInfo _info, bool _result)
    {
        if (false == this->isSimulating)
        {
            return;
        }

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
                            luabind::call_function<void>(this->closureFunctionRequestDropAccepted, this->dragDropData);
                        }
                        catch (luabind::error& error)
                        {
                            luabind::object errorMsg(luabind::from_stack(error.state(), -1));
                            std::stringstream msg;
                            msg << errorMsg;

                            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[MyGUIItemBoxComponent] Caught error in 'reactOnDropItemAccepted' Error: " + Ogre::String(error.what()) + " details: " + msg.str());
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

                                    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[MyGUIItemBoxComponent] Caught error in 'reactOnDropItemAccepted' Error: " + Ogre::String(error.what()) + " details: " + msg.str());
                                }
                            };
                            NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
                        }
                    }
                }
            }

#if 1
            // Shall be done in lua script!
            // if ((_info.sender != _info.receiver))
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
                    myGuiItemBoxCompPtr->increaseQuantity(_info.receiver_index, 1 /*senderData->getQuantity()*/);
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

            NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
            {
                // Trigger item box repaint update, so that resource will be visible
                this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x - 0.001f, this->size->getVector2().y - 0.001f);
                this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x + 0.001f, this->size->getVector2().y + 0.001f);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIItemBoxComponent::notifyEndDrop");

            this->dropFinished = true;
        }
    }

    void MyGUIItemBoxComponent::notifyNotifyItem(wraps::BaseLayout* _sender, const MyGUI::IBNotifyItemData& _info)
    {
        // Only react during simulation
        if (false == this->isSimulating)
        {
            return;
        }

        if (_info.index == MyGUI::ITEM_NONE)
        {
            return;
        }

        if (false == this->enabled->getBool())
        {
            return;
        }

        // REMOVED: getLuaScript() null check — closures work without a LuaScriptComponent
        /*if (nullptr == this->gameObjectPtr->getLuaScript())
        {
            return;
        }*/

        Ogre::String resourceName;
        if (_info.index < static_cast<size_t>(this->resourceNames.size()) && nullptr != this->resourceNames[_info.index])
        {
            resourceName = this->resourceNames[_info.index]->getString();
        }

        unsigned int slotIndex = static_cast<unsigned int>(_info.index);

        OIS::MouseButtonID buttonId = OIS::MB_Left;
        if (_info.id == MyGUI::MouseButton::Right)
        {
            buttonId = OIS::MB_Right;
        }
        else if (_info.id == MyGUI::MouseButton::Middle)
        {
            buttonId = OIS::MB_Middle;
        }
        else if (_info.id == MyGUI::MouseButton::Button3)
        {
            buttonId = OIS::MB_Button3;
        }
        else if (_info.id == MyGUI::MouseButton::Button4)
        {
            buttonId = OIS::MB_Button4;
        }
        else if (_info.id == MyGUI::MouseButton::Button5)
        {
            buttonId = OIS::MB_Button5;
        }
        else if (_info.id == MyGUI::MouseButton::Button6)
        {
            buttonId = OIS::MB_Button6;
        }
        else if (_info.id == MyGUI::MouseButton::Button7)
        {
            buttonId = OIS::MB_Button7;
        }

        if (_info.notify == MyGUI::IBNotifyItemData::NotifyItem::MousePressed)
        {
            this->selectedSlotIndex = static_cast<int>(slotIndex);

            // Resolve the optional per-slot associated GameObject id
            Ogre::String slotGameObjectId = "0";
            if (_info.index < this->gameObjectIds.size() && nullptr != this->gameObjectIds[_info.index])
            {
                const Ogre::String& id = this->gameObjectIds[_info.index]->getString();
                if (!id.empty())
                {
                    slotGameObjectId = id;
                }
            }

            // Resolve the sprite component for this slot (may be null)
            MyGUISpriteComponentBase* spriteComp = nullptr;
            if (slotIndex < this->spriteComponents.size())
            {
                spriteComp = this->spriteComponents[slotIndex];
            }

            // If a sprite is configured and still animating, block the click
            if (nullptr != spriteComp && false == spriteComp->isFinished())
            {
                return;
            }

            // Render-thread: repaint cells AND, if a sprite is configured, snap it over this slot
            NOWA::GraphicsModule::RenderCommand renderCommand = [this, slotIndex, spriteComp]()
            {
                if (nullptr == this->itemBoxWindow)
                {
                    return;
                }

                // Force repaint so select/pressed visual state updates
                unsigned int count = static_cast<unsigned int>(this->itemBoxWindow->getItemBox()->getItemBox()->getItemCount());
                for (unsigned int i = 0; i < count; i++)
                {
                    ItemData** item = this->itemBoxWindow->getItemBox()->getItemDataAt<ItemData*>(i, false);
                    if (nullptr != item)
                    {
                        this->itemBoxWindow->getItemBox()->setItemData(i, *item);
                    }
                }
                this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x - 0.001f, this->size->getVector2().y - 0.001f);
                this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x + 0.001f, this->size->getVector2().y + 0.001f);

                // Position the sprite widget directly over the clicked cell
                if (nullptr != spriteComp)
                {
                    MyGUI::IntCoord cellCoord;
                    bool foundCell = false;

                    // Try to get the cell widget by slot index for pixel-perfect positioning
                    MyGUI::ItemBox* myguiBox = this->itemBoxWindow->getItemBox()->getItemBox();
                    if (slotIndex < myguiBox->getItemCount())
                    {
                        MyGUI::Widget* cellWidget = myguiBox->getWidgetByIndex(slotIndex);
                        if (nullptr != cellWidget)
                        {
                            cellCoord = cellWidget->getAbsoluteCoord();
                            foundCell = true;
                        }
                    }

                    // Fallback: compute coordinates from item box position + cell grid
                    if (false == foundCell)
                    {
                        const int cellW = 74;
                        const int cellH = 74;
                        MyGUI::IntCoord boxCoord = this->widget->getAbsoluteCoord();
                        int cols = std::max(1, boxCoord.width / cellW);
                        int row = static_cast<int>(slotIndex) / cols;
                        int col = static_cast<int>(slotIndex) % cols;
                        cellCoord.left = boxCoord.left + col * cellW;
                        cellCoord.top = boxCoord.top + row * cellH;
                        cellCoord.width = cellW;
                        cellCoord.height = cellH;
                    }

                    spriteComp->positionOverCell(cellCoord);
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIItemBoxComponent::notifyNotifyItem repaint+overlay");

            if (nullptr != spriteComp)
            {
                this->pendingResourceName = resourceName;
                this->pendingGameObjectId = slotGameObjectId;
                this->pendingButtonId = buttonId;
                this->pendingSlotIndex = slotIndex;
                this->pendingSprite = spriteComp;
                this->spriteAnimationPending = true;

                spriteComp->resetAnimation();
            }
            else
            {
                // No sprite: fire Lua callbacks immediately (original behavior)
                if (buttonId == OIS::MB_Left && this->mouseButtonClickClosureFunction.is_valid())
                {
                    NOWA::AppStateManager::LogicCommand cmd = [this, resourceName, slotGameObjectId, buttonId]()
                    {
                        try
                        {
                            luabind::call_function<void>(this->mouseButtonClickClosureFunction, resourceName, slotGameObjectId, buttonId);
                        }
                        catch (luabind::error& error)
                        {
                            luabind::object errorMsg(luabind::from_stack(error.state(), -1));
                            std::stringstream msg;
                            msg << errorMsg;
                            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[MyGUIItemBoxComponent] reactOnMouseButtonClick error: " + Ogre::String(error.what()) + " " + msg.str());
                        }
                    };
                    NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(cmd));
                }

                if (this->mouseButtonPressedClosureFunction.is_valid())
                {
                    NOWA::AppStateManager::LogicCommand cmd = [this, slotIndex, resourceName, buttonId]()
                    {
                        try
                        {
                            luabind::call_function<void>(this->mouseButtonPressedClosureFunction, slotIndex, resourceName, buttonId);
                        }
                        catch (luabind::error& error)
                        {
                            luabind::object errorMsg(luabind::from_stack(error.state(), -1));
                            std::stringstream msg;
                            msg << errorMsg;
                            Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[MyGUIItemBoxComponent] reactOnMouseButtonPressed error: " + Ogre::String(error.what()) + " " + msg.str());
                        }
                    };
                    NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(cmd));
                }
            }
        }
        else if (_info.notify == MyGUI::IBNotifyItemData::NotifyItem::MouseReleased)
        {
            if (this->mouseButtonReleasedClosureFunction.is_valid())
            {
                NOWA::AppStateManager::LogicCommand cmd = [this, slotIndex, resourceName, buttonId]()
                {
                    try
                    {
                        luabind::call_function<void>(this->mouseButtonReleasedClosureFunction, slotIndex, resourceName, buttonId);
                    }
                    catch (luabind::error& error)
                    {
                        luabind::object errorMsg(luabind::from_stack(error.state(), -1));
                        std::stringstream msg;
                        msg << errorMsg;
                        Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[MyGUIItemBoxComponent] reactOnMouseButtonReleased error: " + Ogre::String(error.what()) + " " + msg.str());
                    }
                };
                NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(cmd));
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
        // Guard 1: tooltip feature disabled for this component
        if (false == this->useToolTip->getBool())
        {
            return;
        }

        // Guard 2: component is not the active owner — hide any leftover tooltip and return.
        // The tooltip widget is separate from the shared ItemBox widget, so hiding the
        // shared widget does NOT hide the tooltip. This guard catches the case where
        // MyGUI still fires tooltip events after ownership has been released.
        if (false == this->activated->getBool())
        {
            if (nullptr != this->toolTip)
            {
                NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
                {
                    this->toolTip->hide();
                };
                NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIItemBoxComponent::notifyToolTip force hide");
            }
            return;
        }

        if (nullptr == _data)
        {
            return;
        }

        ItemData dataCopy = *_data;

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, _info, dataCopy]() mutable
        {
            if (nullptr == this->toolTip)
            {
                return;
            }

            if (_info.type == MyGUI::ToolTipInfo::Show)
            {
                this->toolTip->show(&dataCopy);
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
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIItemBoxComponent::notifyToolTip");
    }

    void MyGUIItemBoxComponent::setActivated(bool activated)
    {
        if (this->activated->getBool() == activated)
        {
            return;
        }

        this->activated->setValue(activated);

        if (false == this->commonWidget->getBool())
        {
            // ── Normal (own widget) path — match old base class behavior exactly ──
            if (nullptr != this->widget)
            {
                NOWA::GraphicsModule::RenderCommand cmd = [this, activated]()
                {
                    this->widget->setVisible(activated);
                    // Show/hide children too — base class MyGUIComponent::setActivated did this
                    for (size_t i = 0; i < this->widget->getChildCount(); i++)
                    {
                        this->widget->getChildAt(i)->setVisible(activated);
                    }
                };
                NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "MyGUIItemBoxComponent::setActivated");
            }
            return;
        }

        // ── Shared widget path ────────────────────────────────────────────────────
        auto* goc = AppStateManager::getSingletonPtr()->getGameObjectController();
        if (nullptr == goc)
        {
            return;
        }

        const Ogre::String key = this->getSharedWidgetKey();
        MyGUI::Widget* shared = goc->getSharedWidget(this->getClassName(), key);

        if (nullptr == shared)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MyGUIItemBoxComponent] setActivated: shared widget not found for key: " + key + " (className=" + this->getClassName() + ")");
            return;
        }

        if (true == activated)
        {
            goc->setSharedWidgetOwner(this->getClassName(), key, this);
            this->widget = shared;

            // Re-wire events, clear old items, populate from our variant data.
            // Also sets this->itemBoxWindow = sharedWindow from GOC userData.
            this->onActivated();

            NOWA::GraphicsModule::RenderCommand cmd = [shared]()
            {
                shared->setVisible(true);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "MyGUIItemBoxComponent::setActivated shared show");
        }
        else
        {
            void* currentOwner = goc->getSharedWidgetOwner(this->getClassName(), key);
            if (currentOwner == this)
            {
                NOWA::GraphicsModule::RenderCommand cmd = [shared]()
                {
                    shared->setVisible(false);
                };
                NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "MyGUIItemBoxComponent::setActivated shared hide");

                goc->setSharedWidgetOwner(this->getClassName(), key, nullptr);
                this->widget = nullptr;
                this->itemBoxWindow = nullptr;
            }
        }
    }

    // -----------------------------------------------------------------------------
    void MyGUIItemBoxComponent::onActivated(void)
    {
        auto* goc = AppStateManager::getSingletonPtr()->getGameObjectController();
        if (nullptr == goc)
        {
            return;
        }

        const Ogre::String key = this->getSharedWidgetKey();

        ItemBoxWindow* sharedWindow = static_cast<ItemBoxWindow*>(goc->getSharedWidgetUserData(this->getClassName(), key));

        if (nullptr == sharedWindow)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MyGUIItemBoxComponent] onActivated: sharedWindow not found for key: " + key);
            return;
        }

        NOWA::GraphicsModule::RenderCommand cmd = [this, sharedWindow]()
        {
            // In onActivated — hides THIS instance's OWN tooltip, not the previous owner's.
            // Reason: this instance may have had its tooltip left showing from a previous
            // activation cycle. Starting hidden is always correct — it will show again
            // when the user hovers over an item via notifyToolTip.
            if (nullptr != this->toolTip)
            {
                this->toolTip->hide();
            }

            // ── Re-wire ALL events to this instance ───────────────────────────
            sharedWindow->getItemBox()->eventStartDrag = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyStartDrop);
            sharedWindow->getItemBox()->eventRequestDrop = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyRequestDrop);
            sharedWindow->getItemBox()->eventDropResult = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyEndDrop);
            sharedWindow->getItemBox()->eventChangeDDState = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyDropState);
            sharedWindow->getItemBox()->eventNotifyItem = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyNotifyItem);
            sharedWindow->getItemBox()->eventToolTip = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyToolTip);

            // ── Create toolTip for this instance if useToolTip=true ──────────
            if (nullptr == this->toolTip && true == this->useToolTip->getBool())
            {
                this->toolTip = new ToolTip();
                this->toolTip->hide();
            }

            // ── Clear items from previous owner ───────────────────────────────
            while (sharedWindow->getItemBox()->getItemBox()->getItemCount() > 0)
            {
                ItemData** item = sharedWindow->getItemBox()->getItemDataAt<ItemData*>(0, false);
                if (nullptr != item && nullptr != *item)
                {
                    delete *item;
                }
                sharedWindow->getItemBox()->removeItem(0);
            }

            // ── Populate from this instance's variant data ────────────────────
            for (unsigned int i = 0; i < this->itemCount->getUInt(); i++)
            {
                sharedWindow->getItemBox()->addItem(new ItemData(this->gameObjectPtr->getId()));

                ItemData** itemData = sharedWindow->getItemBox()->getItemDataAt<ItemData*>(i, false);
                if (nullptr != itemData && nullptr != *itemData)
                {
                    if (nullptr != this->resourceNames[i] && !this->resourceNames[i]->getString().empty())
                    {
                        (*itemData)->setResourceName(this->resourceNames[i]->getString());
                    }
                    if (nullptr != this->quantities[i])
                    {
                        (*itemData)->setQuantity(this->quantities[i]->getUInt());
                    }
                    if (nullptr != this->sellValues[i])
                    {
                        (*itemData)->setSellValue(this->sellValues[i]->getReal());
                    }
                    if (nullptr != this->buyValues[i])
                    {
                        (*itemData)->setBuyValue(this->buyValues[i]->getReal());
                    }
                }
            }

            this->widget->setRealPosition(this->position->getVector2().x, this->position->getVector2().y);
            this->widget->setRealSize(this->size->getVector2().x, this->size->getVector2().y);
            Ogre::Vector4 c = this->color->getVector4();
            this->widget->setColour(MyGUI::Colour(c.x, c.y, c.z, c.w));
            this->widget->setInheritsAlpha(true);

            MyGUI::Window* asWindow = this->widget->castType<MyGUI::Window>(false);
            if (nullptr != asWindow)
            {
                asWindow->setMovable(this->movable->getBool());
                this->widget->setProperty("Caption", MyGUI::LanguageManager::getInstancePtr()->replaceTags(this->windowCaption->getString()));
            }

            this->widget->setRealSize(this->size->getVector2().x - 0.001f, this->size->getVector2().y - 0.001f);
            this->widget->setRealSize(this->size->getVector2().x + 0.001f, this->size->getVector2().y + 0.001f);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "MyGUIItemBoxComponent::onActivated populate");

        this->itemBoxWindow = sharedWindow;
    }

    void MyGUIItemBoxComponent::setSkin(const Ogre::String& skin)
    {
        this->skin->setListSelectedValue(skin);

        // Guard against what the widget was actually last built with,
        // not the variant (which is already updated before this is called)
        if (this->lastBuiltSkin == skin)
        {
            return;
        }

        if (nullptr != this->widget && true == this->activated->getBool())
        {
            this->lastBuiltSkin = skin;
            ENQUEUE_RENDER_COMMAND_MULTI_WAIT("MyGUIItemBoxComponent::setSkin", _1(skin), { this->onChangeSkin(); });
        }
    }

    void MyGUIItemBoxComponent::setStyle(const Ogre::String& style)
    {
        if (nullptr == this->style)
        {
            return;
        }

        this->style->setListSelectedValue(style);

        // Only rebuild if the widget was actually built with a different style.
        // The variant is already updated before this is called (undo system, actualizeValue),
        // so comparing against the variant would always be equal — compare against lastBuiltStyle instead.
        if (this->lastBuiltStyle == style)
        {
            return;
        }

        if (nullptr == this->itemBoxWindow)
        {
            return;
        }

        // Do not rebuild if component is not activated — avoids zombie widgets
        // when undo/redo calls setStyle on inactive components after stopping simulation
        if (false == this->activated->getBool())
        {
            return;
        }

        this->lastBuiltStyle = style;

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, style]()
        {
            // Save item state
            std::vector<std::pair<Ogre::String, unsigned int>> saved;
            saved.reserve(this->resourceNames.size());
            for (size_t i = 0; i < this->resourceNames.size(); i++)
            {
                Ogre::String name;
                unsigned int qty = 0;
                if (nullptr != this->resourceNames[i])
                {
                    name = this->resourceNames[i]->getString();
                }
                if (nullptr != this->quantities[i])
                {
                    qty = this->quantities[i]->getUInt();
                }
                saved.push_back({name, qty});
            }

            // Disconnect events before destroying
            this->itemBoxWindow->getItemBox()->eventStartDrag = nullptr;
            this->itemBoxWindow->getItemBox()->eventRequestDrop = nullptr;
            this->itemBoxWindow->getItemBox()->eventDropResult = nullptr;
            this->itemBoxWindow->getItemBox()->eventChangeDDState = nullptr;
            this->itemBoxWindow->getItemBox()->eventNotifyItem = nullptr;
            this->itemBoxWindow->getItemBox()->eventToolTip = nullptr;

            delete this->itemBoxWindow;
            this->itemBoxWindow = nullptr;
            this->widget = nullptr;

            // Rebuild with new layout
            this->itemBoxWindow = new ItemBoxWindow(style + ".layout");

            // Re-wire events
            this->itemBoxWindow->getItemBox()->eventStartDrag = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyStartDrop);
            this->itemBoxWindow->getItemBox()->eventRequestDrop = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyRequestDrop);
            this->itemBoxWindow->getItemBox()->eventDropResult = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyEndDrop);
            this->itemBoxWindow->getItemBox()->eventChangeDDState = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyDropState);
            this->itemBoxWindow->getItemBox()->eventNotifyItem = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyNotifyItem);
            this->itemBoxWindow->getItemBox()->eventToolTip = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyToolTip);

            this->widget = this->itemBoxWindow->getMainWidget();

            MyGUI::LayerManager::getInstance().attachToLayerNode(this->layer->getListSelectedValue(), this->widget);
            this->widget->setRealPosition(this->position->getVector2().x, this->position->getVector2().y);
            this->widget->setRealSize(this->size->getVector2().x, this->size->getVector2().y);

            MyGUI::Window* asWindow = this->widget->castType<MyGUI::Window>(false);
            if (nullptr != asWindow)
            {
                this->setMovable(this->movable->getBool());
                this->setWindowCaption(this->windowCaption->getString());
            }

            this->widget->setInheritsAlpha(true);
            Ogre::Vector4 c = this->color->getVector4();
            this->widget->setColour(MyGUI::Colour(c.x, c.y, c.z, c.w));

            // Restore items — addItem first so getItemDataAt is valid in setResourceName/setQuantity
            for (unsigned int i = 0; i < static_cast<unsigned int>(saved.size()); i++)
            {
                this->itemBoxWindow->getItemBox()->addItem(new ItemData(this->gameObjectPtr->getId()));
                if (!saved[i].first.empty())
                {
                    this->setResourceName(i, saved[i].first);
                }
                this->setQuantity(i, saved[i].second);
            }

            // Hide widget if component is not activated
            if (false == this->activated->getBool())
            {
                this->widget->setVisible(false);
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIItemBoxComponent::setStyle");
    }

    void MyGUIItemBoxComponent::onChangeSkin(void)
    {
        if (nullptr == this->itemBoxWindow)
        {
            return;
        }

        // Save item state from logic-thread variants
        std::vector<std::pair<Ogre::String, unsigned int>> savedItems;
        for (size_t i = 0; i < this->resourceNames.size(); i++)
        {
            if (nullptr != this->resourceNames[i] && nullptr != this->quantities[i])
            {
                savedItems.push_back({this->resourceNames[i]->getString(), this->quantities[i]->getUInt()});
            }
        }

        // Disconnect events before destroying
        this->itemBoxWindow->getItemBox()->eventStartDrag = nullptr;
        this->itemBoxWindow->getItemBox()->eventRequestDrop = nullptr;
        this->itemBoxWindow->getItemBox()->eventDropResult = nullptr;
        this->itemBoxWindow->getItemBox()->eventChangeDDState = nullptr;
        this->itemBoxWindow->getItemBox()->eventNotifyItem = nullptr;
        this->itemBoxWindow->getItemBox()->eventToolTip = nullptr;

        delete this->itemBoxWindow;
        this->itemBoxWindow = nullptr;
        this->widget = nullptr;

        // Recreate with current style layout
        const Ogre::String layoutName = this->style->getListSelectedValue() + ".layout";
        this->itemBoxWindow = new ItemBoxWindow(layoutName);

        // Re-wire events
        this->itemBoxWindow->getItemBox()->eventStartDrag = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyStartDrop);
        this->itemBoxWindow->getItemBox()->eventRequestDrop = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyRequestDrop);
        this->itemBoxWindow->getItemBox()->eventDropResult = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyEndDrop);
        this->itemBoxWindow->getItemBox()->eventChangeDDState = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyDropState);
        this->itemBoxWindow->getItemBox()->eventNotifyItem = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyNotifyItem);
        this->itemBoxWindow->getItemBox()->eventToolTip = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyToolTip);

        this->widget = this->itemBoxWindow->getMainWidget();
        MyGUI::LayerManager::getInstance().attachToLayerNode(this->layer->getListSelectedValue(), this->widget);
        this->widget->setRealPosition(this->position->getVector2().x, this->position->getVector2().y);
        this->widget->setRealSize(this->size->getVector2().x, this->size->getVector2().y);

        MyGUI::Window* asWindow = this->widget->castType<MyGUI::Window>(false);
        if (nullptr != asWindow)
        {
            this->setMovable(this->movable->getBool());
            this->setWindowCaption(this->windowCaption->getString());
        }

        this->widget->setInheritsAlpha(true);
        Ogre::Vector4 c = this->color->getVector4();
        this->widget->setColour(MyGUI::Colour(c.x, c.y, c.z, c.w));

        // Restore items — MUST addItem first so getItemDataAt is valid in setResourceName/setQuantity.
        // Do NOT call setItemCount here: it compares against resourceNames.size() which hasn't changed,
        // sees oldSize == newSize, and skips all addItem calls, leaving the fresh box with 0 items.
        for (size_t i = 0; i < savedItems.size(); i++)
        {
            this->itemBoxWindow->getItemBox()->addItem(new ItemData(this->gameObjectPtr->getId()));
            if (!savedItems[i].first.empty())
            {
                this->setResourceName(static_cast<unsigned int>(i), savedItems[i].first);
            }
            this->setQuantity(static_cast<unsigned int>(i), savedItems[i].second);
        }

        if (false == this->activated->getBool())
        {
            this->widget->setVisible(false);
        }
    }

    // -----------------------------------------------------------------------------
    // getSharedWidgetKey (override)
    // Uses style instead of skin (ItemBox is identified by its layout, not skin).
    // Incorporates tagName so e.g. "Worker_ItemBoxInventory" and
    // "Store_ItemBoxInventory" remain separate shared widgets.
    // -----------------------------------------------------------------------------
    Ogre::String MyGUIItemBoxComponent::getSharedWidgetKey(void) const
    {
        const Ogre::String tagName = this->gameObjectPtr->getTagName();
        if (false == tagName.empty())
        {
            return tagName + "_" + this->style->getListSelectedValue();
        }

        return this->style->getListSelectedValue();
    }

    // -----------------------------------------------------------------------------
    // createAndRegisterSharedWidgetImpl (override)
    // Creates the shared ItemBoxWindow, registers its main widget in GOC,
    // and stores the ItemBoxWindow* as GOC userData so onActivated can find it
    // without a static map.
    // -----------------------------------------------------------------------------
    void MyGUIItemBoxComponent::createAndRegisterSharedWidgetImpl(GameObjectController* goc, const Ogre::String& key)
    {
        // Create ItemBoxWindow on the MAIN thread.
        // Its constructor calls enqueueAndWait internally for assignBase — that
        // only works correctly when initiated from the main thread.
        ItemBoxWindow* sharedWindow = new ItemBoxWindow(this->style->getListSelectedValue() + ".layout");

        // Now run render-thread-only setup: wire events, attach to layer, register in GOC.
        NOWA::GraphicsModule::RenderCommand cmd = [this, goc, key, sharedWindow]()
        {
            // Wire events to this first instance.
            // They are re-wired to the current owner on every setActivated(true) via onActivated().
            sharedWindow->getItemBox()->eventStartDrag = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyStartDrop);
            sharedWindow->getItemBox()->eventRequestDrop = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyRequestDrop);
            sharedWindow->getItemBox()->eventDropResult = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyEndDrop);
            sharedWindow->getItemBox()->eventChangeDDState = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyDropState);
            sharedWindow->getItemBox()->eventNotifyItem = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyNotifyItem);
            sharedWindow->getItemBox()->eventToolTip = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyToolTip);

            MyGUI::Widget* w = sharedWindow->getMainWidget();
            MyGUI::LayerManager::getInstance().attachToLayerNode(this->layer->getListSelectedValue(), w);
            w->setInheritsAlpha(true);
            Ogre::Vector4 c = this->color->getVector4();
            w->setColour(MyGUI::Colour(c.x, c.y, c.z, c.w));
            w->setVisible(false);

            // Register widget in GOC.
            // Key already encodes tagName + style — built by caller from getSharedWidgetKey().
            goc->registerSharedWidget(this->getClassName(), key, w);

            // Store ItemBoxWindow* as GOC userData so onActivated() can retrieve it
            // for ALL instances (first and subsequent) without a static map.
            goc->setSharedWidgetUserData(this->getClassName(), key, sharedWindow);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "MyGUIItemBoxComponent::createAndRegisterSharedWidgetImpl");
    }

    // -----------------------------------------------------------------------------
    // createOwnWidgetImpl (override)
    // Creates a fresh private ItemBoxWindow from current variant values.
    // Called when switching from shared back to own mode.
    // -----------------------------------------------------------------------------
    void MyGUIItemBoxComponent::createOwnWidgetImpl(void)
    {
        NOWA::GraphicsModule::RenderCommand cmd = [this]()
        {
            this->itemBoxWindow = new ItemBoxWindow(this->style->getListSelectedValue() + ".layout");

            this->itemBoxWindow->getItemBox()->eventStartDrag = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyStartDrop);
            this->itemBoxWindow->getItemBox()->eventRequestDrop = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyRequestDrop);
            this->itemBoxWindow->getItemBox()->eventDropResult = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyEndDrop);
            this->itemBoxWindow->getItemBox()->eventChangeDDState = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyDropState);
            this->itemBoxWindow->getItemBox()->eventNotifyItem = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyNotifyItem);
            this->itemBoxWindow->getItemBox()->eventToolTip = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyToolTip);

            this->widget = this->itemBoxWindow->getMainWidget();
            MyGUI::LayerManager::getInstance().attachToLayerNode(this->layer->getListSelectedValue(), this->widget);

            this->widget->setInheritsAlpha(true);
            Ogre::Vector4 c = this->color->getVector4();
            this->widget->setColour(MyGUI::Colour(c.x, c.y, c.z, c.w));

            MyGUI::Window* asWindow = this->widget->castType<MyGUI::Window>(false);
            if (nullptr != asWindow)
            {
                asWindow->setMovable(this->movable->getBool());
                this->widget->setProperty("Caption", MyGUI::LanguageManager::getInstancePtr()->replaceTags(this->windowCaption->getString()));
            }

            // Populate items from variant data
            for (unsigned int i = 0; i < this->itemCount->getUInt(); i++)
            {
                this->itemBoxWindow->getItemBox()->addItem(new ItemData(this->gameObjectPtr->getId()));

                ItemData** itemData = this->itemBoxWindow->getItemBox()->getItemDataAt<ItemData*>(i, false);
                if (nullptr != itemData && nullptr != *itemData)
                {
                    if (nullptr != this->resourceNames[i] && !this->resourceNames[i]->getString().empty())
                    {
                        (*itemData)->setResourceName(this->resourceNames[i]->getString());
                    }
                    if (nullptr != this->quantities[i])
                    {
                        (*itemData)->setQuantity(this->quantities[i]->getUInt());
                    }
                    if (nullptr != this->sellValues[i])
                    {
                        (*itemData)->setSellValue(this->sellValues[i]->getReal());
                    }
                    if (nullptr != this->buyValues[i])
                    {
                        (*itemData)->setBuyValue(this->buyValues[i]->getReal());
                    }
                }
            }

            this->widget->setUserData(std::make_pair<unsigned long, unsigned int>(this->gameObjectPtr->getId(), this->getIndex()));

            // Trigger repaint
            this->widget->setRealSize(this->size->getVector2().x - 0.001f, this->size->getVector2().y - 0.001f);
            this->widget->setRealSize(this->size->getVector2().x + 0.001f, this->size->getVector2().y + 0.001f);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(cmd), "MyGUIItemBoxComponent::createOwnWidgetImpl");

        this->lastBuiltStyle = this->style->getListSelectedValue();
        this->lastBuiltSkin = this->skin->getListSelectedValue();
    }

    // -----------------------------------------------------------------------------
    // setCommonWidget (override)
    // ItemBox needs extra handling for ItemBoxWindow* in GOC userData.
    // For enabling: base handles GOC registration via createAndRegisterSharedWidgetImpl.
    // For disabling: base handles GOC unregistration, then createOwnWidgetImpl rebuilds.
    // The only extra work: null itemBoxWindow on disable (base doesn't know about it).
    // -----------------------------------------------------------------------------
    void MyGUIItemBoxComponent::setCommonWidget(bool enable)
    {
        if (this->commonWidget->getBool() == enable)
        {
            return;
        }

        // Do not set here set new value and not in derived component, because else the check above does fail in base class!
        // this->commonWidget->setValue(enabled);
        // if (this->commonWidget->getBool() == enable)

        if (false == enable)
        {
            // Before the base unregisters from GOC, null our itemBoxWindow pointer.
            // The base will call createOwnWidgetImpl which sets a fresh one.
            this->itemBoxWindow = nullptr;

            // If we are the current shared owner, also null widget so the base
            // doesn't try to hide a widget we're about to replace.
            auto* goc = AppStateManager::getSingletonPtr()->getGameObjectController();
            if (nullptr != goc)
            {
                const Ogre::String key = this->getSharedWidgetKey();
                void* owner = goc->getSharedWidgetOwner(this->getClassName(), key);
                if (owner == this)
                {
                    this->widget = nullptr;
                }

                // If we are the last instance, delete the ItemBoxWindow wrapper
                // before base calls unregisterSharedWidget (which destroys the MyGUI widget).
                // We must check refcount by peeking: unregister will drop it to 0.
                // Retrieve the ItemBoxWindow* before unregister removes the entry.
                // Note: base::setCommonWidget calls unregisterSharedWidget which calls
                // destroyWidget — ItemBoxWindow dtor must NOT destroy the widget again.
                // So we delete ItemBoxWindow here AFTER the widget is already gone if
                // refCount would hit 0 — handled inside onRemoveComponent cleanup.
                // For the hot-switch case (not last instance), just forget the pointer.
            }
        }

        // Delegate to base for all GOC management and widget show/hide.
        // Base calls createAndRegisterSharedWidgetImpl or createOwnWidgetImpl as needed.
        MyGUIComponent::setCommonWidget(enable);

        // After base completes:
        if (true == enable)
        {
            // Ensure itemBoxWindow is null — we are now data-only.
            // setActivated(true) called by base will set it via onActivated().
            this->itemBoxWindow = nullptr;
        }
    }

    void MyGUIItemBoxComponent::setResourceLocationName(const Ogre::String& resourceLocationName)
    {
        this->oldResourceLocationName = this->resourceLocationName->getString();
        this->resourceLocationName->setValue(resourceLocationName);
        if (this->oldResourceLocationName != this->resourceLocationName->getString())
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this, resourceLocationName]()
            {
                // Check if the resources from this XML are already loaded by probing
                // a well-known resource that every valid inventory XML must define.
                // If any resource from the file exists, the file was already loaded.
                // We use the first resource name from our own slots if available,
                // otherwise just load (MyGUI deduplicates internally after the warning).
                bool alreadyLoaded = false;
                for (unsigned int i = 0; i < this->itemCount->getUInt(); i++)
                {
                    if (nullptr != this->resourceNames[i] && !this->resourceNames[i]->getString().empty())
                    {
                        if (MyGUI::ResourceManager::getInstance().isExist(this->resourceNames[i]->getString()))
                        {
                            alreadyLoaded = true;
                            break;
                        }
                    }
                }

                if (false == alreadyLoaded)
                {
                    bool success = MyGUI::ResourceManager::getInstance().load(this->resourceLocationName->getString());
                    if (false == success)
                    {
                        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MyGUIItemBoxComponent] ERROR: Could not load resource: " + this->resourceLocationName->getString() + " for game object: " + this->gameObjectPtr->getName());
                    }
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIItemBoxComponent::setResourceLocationName");

            // MyGUI::ResourceManager::getInstance().load("ItemBox_skin.xml");

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
        // Update the variant FIRST — was missing entirely
        this->useToolTip->setValue(useToolTip);

        if (true == useToolTip)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
            {
                if (nullptr == this->toolTip)
                {
                    this->toolTip = new ToolTip();
                    this->toolTip->hide();
                }
                // Wire tooltip event only if itemBoxWindow exists (non-shared path).
                // Shared path re-wires in onActivated().
                if (nullptr != this->itemBoxWindow)
                {
                    this->itemBoxWindow->getItemBox()->eventToolTip = MyGUI::newDelegate(this, &MyGUIItemBoxComponent::notifyToolTip);
                }
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIItemBoxComponent::setUseToolTip create");
        }
        else
        {
            // Destroy tooltip if it exists
            if (nullptr != this->toolTip)
            {
                NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
                {
                    // Hide before deleting — may be visible when switching owners
                    this->toolTip->hide();
                    delete this->toolTip;
                    this->toolTip = nullptr;

                    // Disconnect the event so notifyToolTip is never called
                    if (nullptr != this->itemBoxWindow)
                    {
                        this->itemBoxWindow->getItemBox()->eventToolTip = nullptr;
                    }
                };
                NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIItemBoxComponent::setUseToolTip delete");
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
            this->gameObjectIds.resize(itemCount);
            this->spriteComponentIndices.resize(itemCount);
            this->spriteComponents.resize(itemCount, nullptr);

            for (size_t i = oldSize; i < this->resourceNames.size(); i++)
            {
                this->resourceNames[i] = new Variant(MyGUIItemBoxComponent::AttrResourceName() + Ogre::StringConverter::toString(i), "", this->attributes);
                this->quantities[i] = new Variant(MyGUIItemBoxComponent::AttrQuantity() + Ogre::StringConverter::toString(i), 0, this->attributes);
                this->sellValues[i] = new Variant(MyGUIItemBoxComponent::AttrSellValue() + Ogre::StringConverter::toString(i), 0.0f, this->attributes);
                this->buyValues[i] = new Variant(MyGUIItemBoxComponent::AttrBuyValue() + Ogre::StringConverter::toString(i), 0.0f, this->attributes);
                this->gameObjectIds[i] = new Variant(MyGUIItemBoxComponent::AttrGameObjectId() + Ogre::StringConverter::toString(i), static_cast<unsigned long>(0), this->attributes);
                this->gameObjectIds[i]->setDescription("Optional: id of the GameObject associated with this slot (e.g. a building template). Passed to reactOnMouseButtonClick. 0 = none.");

                this->spriteComponentIndices[i] = new Variant(MyGUIItemBoxComponent::AttrSpriteComponentIndex() + Ogre::StringConverter::toString(i), static_cast<int>(-1), this->attributes);
                this->spriteComponentIndices[i]
                    ->setDescription("Optional: Occurrence index of a MyGuiSpriteComponent on the same game object to overlay and animate over this slot on click. See in NOWA-Design: MyGuiSpriteComponent (i). Setting to -1 = disabled.");
                this->spriteComponentIndices[i]->addUserData(GameObject::AttrActionSeparator());
                this->spriteComponents[i] = nullptr;

                if (nullptr != this->itemBoxWindow)
                {
                    NOWA::GraphicsModule::RenderCommand renderCommand = [this, itemCount, oldSize]()
                    {
                        this->itemBoxWindow->getItemBox()->addItem(new ItemData(this->gameObjectPtr->getId()));
                    };
                    NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIItemBoxComponent::setItemCount addItem");
                }
            }
        }
        else if (itemCount < oldSize)
        {
            // Remove map entries for slots being erased
            for (size_t i = itemCount; i < oldSize; i++)
            {
                if (this->resourceNames[i])
                {
                    const Ogre::String& name = this->resourceNames[i]->getString();
                    if (!name.empty())
                    {
                        this->resourceNameToIndex.erase(name);
                    }
                }
            }

            this->eraseVariants(this->resourceNames, itemCount);
            this->eraseVariants(this->quantities, itemCount);
            this->eraseVariants(this->sellValues, itemCount);
            this->eraseVariants(this->buyValues, itemCount);
            this->eraseVariants(this->gameObjectIds, itemCount);
            this->eraseVariants(this->spriteComponentIndices, itemCount);
            this->spriteComponents.resize(itemCount);

            if (nullptr != this->itemBoxWindow)
            {
                NOWA::GraphicsModule::RenderCommand renderCommand = [this, itemCount, oldSize]()
                {
                    for (size_t i = itemCount; i < oldSize; i++)
                    {
                        if (this->itemBoxWindow->getItemBox()->getItemBox()->getItemCount() > itemCount)
                        {
                            delete *this->itemBoxWindow->getItemBox()->getItemDataAt<ItemData*>(itemCount);
                            this->itemBoxWindow->getItemBox()->removeItem(itemCount);
                        }
                    }
                };
                NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIItemBoxComponent::setItemCount erase");
            }
        }
        if (nullptr != this->itemBoxWindow)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
            {
                // Trigger item box repaint update, so that resource will be visible
                this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x - 0.001f, this->size->getVector2().y - 0.001f);
                this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x + 0.001f, this->size->getVector2().y + 0.001f);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIItemBoxComponent::setItemCount repaint");
        }
    }

    unsigned int MyGUIItemBoxComponent::getItemCount(void) const
    {
        return this->itemCount->getUInt();
    }

    void MyGUIItemBoxComponent::setResourceName(unsigned int index, const Ogre::String& resourceName)
    {
        if (index >= this->resourceNames.size())
        {
            index = static_cast<unsigned int>(this->resourceNames.size()) - 1;
        }

        // Keep the logic-thread index map in sync
        const Ogre::String& oldName = this->resourceNames[index]->getString();
        if (!oldName.empty())
        {
            this->resourceNameToIndex.erase(oldName);
        }
        if (!resourceName.empty())
        {
            this->resourceNameToIndex[resourceName] = index;
        }

        this->resourceNames[index]->setValue(resourceName);

        if (true == this->bIsCloning || nullptr == this->itemBoxWindow)
        {
            return;
        }

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, resourceName, index]()
        {
            if (index >= this->itemBoxWindow->getItemBox()->getItemBox()->getItemCount())
            {
                return;
            }

            ItemData** item = this->itemBoxWindow->getItemBox()->getItemDataAt<ItemData*>(index, false);
            if (nullptr != item && nullptr != *item)
            {
                (*item)->setResourceName(resourceName);
            }

            this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x - 0.001f, this->size->getVector2().y - 0.001f);
            this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x + 0.001f, this->size->getVector2().y + 0.001f);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIItemBoxComponent::setResourceName repaint");
    }

    Ogre::String MyGUIItemBoxComponent::getResourceName(unsigned int index)
    {
        if (index >= this->resourceNames.size())
        {
            return "";
        }
        return this->resourceNames[index]->getString();
    }

    void MyGUIItemBoxComponent::setQuantity(unsigned int index, unsigned int quantity)
    {
        if (index >= this->quantities.size())
        {
            index = static_cast<unsigned int>(this->quantities.size()) - 1;
        }

        this->quantities[index]->setValue(quantity);

        if (true == this->bIsCloning || nullptr == this->itemBoxWindow)
        {
            return;
        }

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, index, quantity]()
        {
            // Guard: fresh box may have fewer items than the variant arrays
            if (index >= this->itemBoxWindow->getItemBox()->getItemBox()->getItemCount())
            {
                return;
            }

            ItemData** item = this->itemBoxWindow->getItemBox()->getItemDataAt<ItemData*>(index, false);
            if (nullptr != item && nullptr != *item)
            {
                if (quantity > 0 && (*item)->isEmpty() && index < this->resourceNames.size() && nullptr != this->resourceNames[index] && !this->resourceNames[index]->getString().empty())
                {
                    (*item)->setResourceName(this->resourceNames[index]->getString());
                }
                (*item)->setQuantity(quantity);
            }

            this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x - 0.001f, this->size->getVector2().y - 0.001f);
            this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x + 0.001f, this->size->getVector2().y + 0.001f);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIItemBoxComponent::setQuantity repaint");
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

        NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
        {
            // Trigger item box repaint update, so that resource will be visible
            this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x - 0.001f, this->size->getVector2().y - 0.001f);
            this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x + 0.001f, this->size->getVector2().y + 0.001f);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIItemBoxComponent::setQuantity2 repaint");
    }

    unsigned int MyGUIItemBoxComponent::getQuantity(unsigned int index)
    {
        if (index >= this->quantities.size())
        {
            return 0;
        }
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
        if (resourceName.empty())
        {
            return -1;
        }

        auto it = this->resourceNameToIndex.find(resourceName);
        if (it != this->resourceNameToIndex.cend())
        {
            return static_cast<int>(it->second);
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
                    Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL,
                        "[MyGUIItemBoxComponent] ERROR: Could not add resource: " + resourceName + ", because there is no free index anymore, because the inventory is full, for game object: " + this->gameObjectPtr->getName());
                }
            }

            NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
            {
                // Trigger item box repaint update, so that resource will be visible
                this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x - 0.001f, this->size->getVector2().y - 0.001f);
                this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x + 0.001f, this->size->getVector2().y + 0.001f);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIItemBoxComponent::addQuantity repaint");
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

            NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
            {
                // Trigger item box repaint update, so that resource will be visible
                this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x - 0.001f, this->size->getVector2().y - 0.001f);
                this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x + 0.001f, this->size->getVector2().y + 0.001f);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIItemBoxComponent::removeQuantity repaint");
        }
    }

    void MyGUIItemBoxComponent::setSellValue(unsigned int index, Ogre::Real sellValue)
    {
        if (index >= this->sellValues.size())
        {
            index = static_cast<unsigned int>(this->sellValues.size()) - 1;
        }
        this->sellValues[index]->setValue(sellValue);

        if (true == this->bIsCloning || nullptr == this->itemBoxWindow)
        {
            return;
        }

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, index, sellValue]()
        {
            if (nullptr != this->itemBoxWindow)
            {
                ItemData** item = this->itemBoxWindow->getItemBox()->getItemDataAt<ItemData*>(index, true);
                if (nullptr != item)
                {
                    (*item)->setSellValue(sellValue);
                }
            }

            // Trigger item box repaint update, so that resource will be visible
            this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x - 0.001f, this->size->getVector2().y - 0.001f);
            this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x + 0.001f, this->size->getVector2().y + 0.001f);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIItemBoxComponent::setSellValue repaint");
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

        if (nullptr != this->itemBoxWindow)
        {
            NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
            {
                // Trigger item box repaint update, so that resource will be visible
                this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x - 0.001f, this->size->getVector2().y - 0.001f);
                this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x + 0.001f, this->size->getVector2().y + 0.001f);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIItemBoxComponent::setSellValue2 repaint");
        }
    }

    Ogre::Real MyGUIItemBoxComponent::getSellValue(unsigned int index)
    {
        if (index >= this->sellValues.size())
        {
            return 0.0f;
        }
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
        if (index >= this->buyValues.size())
        {
            index = static_cast<unsigned int>(this->buyValues.size()) - 1;
        }
        this->buyValues[index]->setValue(buyValue);

        if (true == this->bIsCloning || nullptr == this->itemBoxWindow)
        {
            return;
        }

        NOWA::GraphicsModule::RenderCommand renderCommand = [this, index, buyValue]()
        {
            if (nullptr != this->itemBoxWindow)
            {
                ItemData** item = this->itemBoxWindow->getItemBox()->getItemDataAt<ItemData*>(index, true);
                if (nullptr != item)
                {
                    (*item)->setBuyValue(buyValue);
                }
            }
            // Trigger item box repaint update, so that resource will be visible
            this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x - 0.001f, this->size->getVector2().y - 0.001f);
            this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x + 0.001f, this->size->getVector2().y + 0.001f);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIItemBoxComponent::setBuyValue repaint");
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

        NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
        {
            // Trigger item box repaint update, so that resource will be visible
            this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x - 0.001f, this->size->getVector2().y - 0.001f);
            this->itemBoxWindow->getMainWidget()->setRealSize(this->size->getVector2().x + 0.001f, this->size->getVector2().y + 0.001f);
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIItemBoxComponent::setBuyValue2 repaint");
    }

    Ogre::Real MyGUIItemBoxComponent::getBuyValue(unsigned int index)
    {
        if (index >= this->buyValues.size())
        {
            return 0.0f;
        }
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

    void MyGUIItemBoxComponent::setGameObjectId(unsigned int index, unsigned long gameObjectId)
    {
        if (index >= this->gameObjectIds.size())
        {
            return;
        }
        if (nullptr != this->gameObjectIds[index])
        {
            this->gameObjectIds[index]->setValue(gameObjectId);
        }
    }

    unsigned long MyGUIItemBoxComponent::getGameObjectId(unsigned int index)
    {
        if (index >= this->gameObjectIds.size() || nullptr == this->gameObjectIds[index])
        {
            return 0;
        }
        return this->gameObjectIds[index]->getULong();
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
        {
            return;
        }

        this->resourceNameToIndex.clear();

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

        NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
        {
            for (unsigned int i = 0; i < this->itemCount->getUInt(); i++)
            {
                delete *this->itemBoxWindow->getItemBox()->getItemDataAt<ItemData*>(0);
                this->itemBoxWindow->getItemBox()->removeItem(0);
            }
            for (unsigned int i = 0; i < this->itemCount->getUInt(); i++)
            {
                this->itemBoxWindow->getItemBox()->addItem(new ItemData(this->gameObjectPtr->getId()));
            }
        };
        NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "MyGUIItemBoxComponent::clearItems");
    }

    void MyGUIItemBoxComponent::reactOnMouseButtonPressed(luabind::object closureFunction)
    {
        this->mouseButtonPressedClosureFunction = closureFunction;
    }

    void MyGUIItemBoxComponent::reactOnMouseButtonReleased(luabind::object closureFunction)
    {
        this->mouseButtonReleasedClosureFunction = closureFunction;
    }

    void MyGUIItemBoxComponent::reactOnDropItemRequest(luabind::object closureFunction)
    {
        this->closureFunctionRequestDropRequest = closureFunction;
    }

    void MyGUIItemBoxComponent::reactOnDropItemAccepted(luabind::object closureFunction)
    {
        this->closureFunctionRequestDropAccepted = closureFunction;
    }

    void MyGUIItemBoxComponent::setSpriteComponentIndex(unsigned int slotIndex, int occurrenceIndex)
    {
        if (slotIndex >= this->spriteComponentIndices.size())
        {
            return;
        }

        if (nullptr == this->spriteComponentIndices[slotIndex])
        {
            this->spriteComponentIndices[slotIndex] = new Variant(MyGUIItemBoxComponent::AttrSpriteComponentIndex() + Ogre::StringConverter::toString(slotIndex), occurrenceIndex, this->attributes);
        }
        else
        {
            this->spriteComponentIndices[slotIndex]->setValue(occurrenceIndex);
        }

        // Re-resolve the sprite pointer immediately if already connected
        if (slotIndex < this->spriteComponents.size())
        {
            this->spriteComponents[slotIndex] = nullptr;
            if (occurrenceIndex >= 0)
            {
                auto spriteCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponentWithOccurrence<MyGUISpriteComponentBase>(occurrenceIndex));
                if (nullptr != spriteCompPtr)
                {
                    this->spriteComponents[slotIndex] = spriteCompPtr.get();
                    this->spriteComponents[slotIndex]->setRepeat(false);
                }
            }
        }
    }

    unsigned int MyGUIItemBoxComponent::getSpriteComponentIndex(int slotIndex) const
    {
        if (slotIndex >= this->spriteComponentIndices.size() || nullptr == this->spriteComponentIndices[slotIndex])
        {
            return -1;
        }
        return this->spriteComponentIndices[slotIndex]->getUInt();
    }

    void MyGUIItemBoxComponent::handleSpriteAnimationFinished(NOWA::EventDataPtr eventData)
    {
        auto cast = boost::static_pointer_cast<EventDataSpriteAnimationFinished>(eventData);

        // Only react to our own game object
        if (cast->getGameObjectId() != this->gameObjectPtr->getId())
        {
            return;
        }

        // Only react if this exact sprite component is the one we started
        if (false == this->spriteAnimationPending || cast->getComponent() != this->pendingSprite)
        {
            return;
        }

        this->spriteAnimationPending = false;
        this->pendingSprite = nullptr;

        // Deactivate the sprite so it hides (ImageBox becomes invisible / no longer updating)
        // Fire deferred Lua callbacks now that the animation has completed
        const Ogre::String resourceName = this->pendingResourceName;
        const Ogre::String slotGameObjectId = this->pendingGameObjectId;
        const OIS::MouseButtonID buttonId = this->pendingButtonId;
        const unsigned int slotIndex = this->pendingSlotIndex;

        if (buttonId == OIS::MB_Left && this->mouseButtonClickClosureFunction.is_valid())
        {
            NOWA::AppStateManager::LogicCommand cmd = [this, resourceName, slotGameObjectId, buttonId]()
            {
                try
                {
                    luabind::call_function<void>(this->mouseButtonClickClosureFunction, resourceName, slotGameObjectId, buttonId);
                }
                catch (luabind::error& error)
                {
                    luabind::object errorMsg(luabind::from_stack(error.state(), -1));
                    std::stringstream msg;
                    msg << errorMsg;
                    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[MyGUIItemBoxComponent] reactOnMouseButtonClick (post-sprite) error: " + Ogre::String(error.what()) + " " + msg.str());
                }
            };
            NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(cmd));
        }

        if (this->mouseButtonPressedClosureFunction.is_valid())
        {
            NOWA::AppStateManager::LogicCommand cmd = [this, slotIndex, resourceName, buttonId]()
            {
                try
                {
                    luabind::call_function<void>(this->mouseButtonPressedClosureFunction, slotIndex, resourceName, buttonId);
                }
                catch (luabind::error& error)
                {
                    luabind::object errorMsg(luabind::from_stack(error.state(), -1));
                    std::stringstream msg;
                    msg << errorMsg;
                    Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[MyGUIItemBoxComponent] reactOnMouseButtonPressed (post-sprite) error: " + Ogre::String(error.what()) + " " + msg.str());
                }
            };
            NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(cmd));
        }
    }

    MyGUIItemBoxComponent* getMyGUIItemBoxComponentComponent(GameObject* gameObject)
    {
        return makeStrongPtr<MyGUIItemBoxComponent>(gameObject->getComponent<MyGUIItemBoxComponent>()).get();
    }

    MyGUIItemBoxComponent* getMyGUIItemBoxComponentComponentFromName(GameObject* gameObject, const Ogre::String& name)
    {
        return makeStrongPtr<MyGUIItemBoxComponent>(gameObject->getComponentFromName<MyGUIItemBoxComponent>(name)).get();
    }

    Ogre::String getSenderInventoryId(DragDropData* instance)
    {
        return Ogre::StringConverter::toString(instance->getSenderInventoryId());
    }

    void MyGUIItemBoxComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObjectClass, luabind::class_<GameObjectController>& gameObjectControllerClass)
    {
        module(lua)[class_<MyGUIItemBoxComponent, MyGUIWindowComponent>("MyGUIItemBoxComponent")
                .def("getItemCount", &MyGUIItemBoxComponent::getItemCount)
                .def("getResourceName", &MyGUIItemBoxComponent::getResourceName)
                .def("setQuantity2", (void (MyGUIItemBoxComponent::*)(unsigned int, unsigned int))&MyGUIItemBoxComponent::setQuantity)
                .def("setQuantity", (void (MyGUIItemBoxComponent::*)(const Ogre::String&, unsigned int))&MyGUIItemBoxComponent::setQuantity)
                .def("getQuantity2", (unsigned int (MyGUIItemBoxComponent::*)(unsigned int))&MyGUIItemBoxComponent::getQuantity)
                .def("getQuantity", (unsigned int (MyGUIItemBoxComponent::*)(const Ogre::String&))&MyGUIItemBoxComponent::getQuantity)
                .def("addQuantity", &MyGUIItemBoxComponent::addQuantity)
                .def("increaseQuantity", (void (MyGUIItemBoxComponent::*)(const Ogre::String&, unsigned int))&MyGUIItemBoxComponent::increaseQuantity)
                .def("decreaseQuantity", (void (MyGUIItemBoxComponent::*)(const Ogre::String&, unsigned int))&MyGUIItemBoxComponent::decreaseQuantity)
                .def("removeQuantity", &MyGUIItemBoxComponent::removeQuantity)

                .def("setSellValue2", (void (MyGUIItemBoxComponent::*)(unsigned int, Ogre::Real))&MyGUIItemBoxComponent::setSellValue)
                .def("setSellValue", (void (MyGUIItemBoxComponent::*)(const Ogre::String&, Ogre::Real))&MyGUIItemBoxComponent::setSellValue)
                .def("getSellValue2", (Ogre::Real (MyGUIItemBoxComponent::*)(unsigned int))&MyGUIItemBoxComponent::getSellValue)
                .def("getSellValue", (Ogre::Real (MyGUIItemBoxComponent::*)(const Ogre::String&))&MyGUIItemBoxComponent::getSellValue)

                .def("setBuyValue2", (void (MyGUIItemBoxComponent::*)(unsigned int, Ogre::Real))&MyGUIItemBoxComponent::setBuyValue)
                .def("setBuyValue", (void (MyGUIItemBoxComponent::*)(const Ogre::String&, Ogre::Real))&MyGUIItemBoxComponent::setBuyValue)
                .def("getBuyValue2", (Ogre::Real (MyGUIItemBoxComponent::*)(unsigned int))&MyGUIItemBoxComponent::getBuyValue)
                .def("getBuyValue", (Ogre::Real (MyGUIItemBoxComponent::*)(const Ogre::String&))&MyGUIItemBoxComponent::getBuyValue)

                .def("clearItems", &MyGUIItemBoxComponent::clearItems)
                .def("reactOnMouseButtonPressed", &MyGUIItemBoxComponent::reactOnMouseButtonPressed)
                .def("reactOnMouseButtonReleased", &MyGUIItemBoxComponent::reactOnMouseButtonReleased)
                .def("reactOnDropItemRequest", &MyGUIItemBoxComponent::reactOnDropItemRequest)
                .def("reactOnDropItemAccepted", &MyGUIItemBoxComponent::reactOnDropItemAccepted)
                .def("reactOnMouseButtonClick", &MyGUIItemBoxComponent::reactOnMouseButtonClick)
                .def("setGameObjectId", &MyGUIItemBoxComponent::setGameObjectId)
                .def("getGameObjectId", &MyGUIItemBoxComponent::getGameObjectId)];

        LuaScriptApi::getInstance()->addClassToCollection("MyGUIItemBoxComponent", "class inherits MyGUIWindowComponent", MyGUIItemBoxComponent::getStaticInfoText());
        LuaScriptApi::getInstance()->addClassToCollection("MyGUIItemBoxComponent", "int getItemCount()", "Gets the max inventory item count.");
        LuaScriptApi::getInstance()->addClassToCollection("MyGUIItemBoxComponent", "String getResourceName(int index)",
            "Gets resource name for the given index. E.g. if 'energy' resource is placed in inventory at index 1, getResourceName(1) would deliver the energy resource. "
            "Note: This function can be used in a loop in conjunction with @getItemCount to iterate over all items in inventory and dump all resources.");
        LuaScriptApi::getInstance()->addClassToCollection("MyGUIItemBoxComponent", "void setQuantity2(int index, int quantity)", "Sets the quantity of the resource for the given index.");
        LuaScriptApi::getInstance()->addClassToCollection("MyGUIItemBoxComponent", "void setQuantity(int index, String resourceName)", "Sets the quantity of the resource.");
        LuaScriptApi::getInstance()->addClassToCollection("MyGUIItemBoxComponent", "int getQuantity2(int index)", "Gets the quantity of the resource for the given index.");
        LuaScriptApi::getInstance()->addClassToCollection("MyGUIItemBoxComponent", "int getQuantity(String resourceName)", "Gets the quantity of the resource.");
        LuaScriptApi::getInstance()->addClassToCollection("MyGUIItemBoxComponent", "void increaseQuantity(String resourceName, int quantity)", "Gets the current quantity and increases by the given quantity value.");
        LuaScriptApi::getInstance()->addClassToCollection("MyGUIItemBoxComponent", "void decreaseQuantity(String resourceName, int quantity)", "Gets the current quantity and decreases by the given quantity value.");
        LuaScriptApi::getInstance()->addClassToCollection("MyGUIItemBoxComponent", "float getSellValue2(int index)", "Gets the sell value of the resource for the given index.");
        LuaScriptApi::getInstance()->addClassToCollection("MyGUIItemBoxComponent", "float getSellValue(String resourceName)", "Gets the sell value of the resource.");
        LuaScriptApi::getInstance()->addClassToCollection("MyGUIItemBoxComponent", "float getBuyValue2(int index)", "Gets the buy value of the resource for the given index.");
        LuaScriptApi::getInstance()->addClassToCollection("MyGUIItemBoxComponent", "float getBuyValue(String resourceName)", "Gets the buy value of the resource.");
        LuaScriptApi::getInstance()->addClassToCollection("MyGUIItemBoxComponent", "void addQuantity(String resourceName, int quantity)", "Adds the quantity of the resource, if does not exist, creates a new slot in inventory.");
        LuaScriptApi::getInstance()->addClassToCollection("MyGUIItemBoxComponent", "void removeQuantity(String resourceName, int quantity)", "Removes the quantity from the resource.");
        LuaScriptApi::getInstance()->addClassToCollection("MyGUIItemBoxComponent", "void clearItems()", "Cleares the whole inventory.");
        LuaScriptApi::getInstance()->addClassToCollection("MyGUIItemBoxComponent", "void reactOnDropItemRequest(func closureFunction, DragDropData dragDropData)",
            "Sets whether to react if an item is requested to be drag and dropped to another inventory. A return value also can be set to prohibit the operation. E.g. getMyGUIItemBoxComponent():reactOnDropItemRequest(function(dragDropData) ... end");
        LuaScriptApi::getInstance()->addClassToCollection("MyGUIItemBoxComponent", "void reactOnMouseButtonPressed(func closureFunction, int slotIndex, string resourceName, int buttonId)",
            "Fires when a mouse button is pressed down on an inventory slot. "
            "Receives the slot index, resource name and mouse button id. "
            "Use for drag initiation or visual feedback. "
            "E.g. getMyGUIItemBoxComponent():reactOnMouseButtonPressed(function(slotIndex, resourceName, buttonId) ... end)");
        LuaScriptApi::getInstance()->addClassToCollection("MyGUIItemBoxComponent", "void reactOnMouseButtonReleased(func closureFunction, int slotIndex, string resourceName, int buttonId)",
            "Fires when a mouse button is released on an inventory slot. "
            "Receives the slot index, resource name and mouse button id. "
            "This is the correct event for triggering item actions (placement, use, etc.) "
            "since actions should fire on release, not press. "
            "E.g. getMyGUIItemBoxComponent():reactOnMouseButtonReleased(function(slotIndex, resourceName, buttonId) ... end)");
        LuaScriptApi::getInstance()->addClassToCollection("MyGUIItemBoxComponent", "void reactOnDropItemAccepted(func closureFunction, DragDropData dragDropData)",
            "Sets whether to react if an item drop has been accepted to another inventory. E.g. getMyGUIItemBoxComponent():reactOnDropItemAccepted(function(dragDropData) ... end");
        LuaScriptApi::getInstance()->addClassToCollection("MyGUIItemBoxComponent", "void reactOnMouseButtonClick(func closureFunction, string resourceName, string gameObjectId, int buttonId)",
            "Sets whether to react if a mouse button has been clicked on the inventory. "
            "The clicked resource name, the associated GameObject id (as string, '0' if none was set in NOWA-Design), and the mouse button id are received. "
            "The gameObjectId can be used to directly resolve the template GameObject for the clicked slot without any name lookup. "
            "E.g. getMyGUIItemBoxComponent():reactOnMouseButtonClick(function(resourceName, gameObjectId, buttonId) "
            "  local templateGO = AppStateManager:getGameObjectController():getGameObjectFromId(gameObjectId) "
            "  -- templateGO is nil when gameObjectId is '0' "
            "end)");
        LuaScriptApi::getInstance()->addClassToCollection("MyGUIItemBoxComponent", "void setGameObjectId(int index, String gameObjectId)",
            "Sets the associated GameObject id for the given slot index. Use '0' for none. "
            "This value is also configurable per-slot in NOWA-Design and is passed automatically to reactOnMouseButtonClick.");
        LuaScriptApi::getInstance()->addClassToCollection("MyGUIItemBoxComponent", "String getGameObjectId(int index)", "Gets the associated GameObject id for the given slot index. Returns '0' if none is set.");

        module(lua)[class_<DragDropData>("DragDropData")
                .def("getResourceName", &DragDropData::getResourceName)
                .def("getQuantity", &DragDropData::getQuantity)
                .def("getSellValue", &DragDropData::getSellValue)
                .def("getBuyValue", &DragDropData::getBuyValue)
                .def("getSenderReceiverIsSame", &DragDropData::getSenderReceiverIsSame)
                .def("getSenderInventoryId", &getSenderInventoryId)
                .def("setCanDrop", &DragDropData::setCanDrop)
                .def("getCanDrop", &DragDropData::getCanDrop)];

        LuaScriptApi::getInstance()->addClassToCollection("DragDropData", "DragDropData", "It can be used when an item is dragged from one inventory to another to get some data and control if it may be dropped.");
        LuaScriptApi::getInstance()->addClassToCollection("DragDropData", "String getResourceName()", "Gets the resource name.");
        LuaScriptApi::getInstance()->addClassToCollection("DragDropData", "int getQuantity()", "Gets resource quantity.");
        LuaScriptApi::getInstance()->addClassToCollection("DragDropData", "float getSellValue()", "Gets the sell value of the resource.");
        LuaScriptApi::getInstance()->addClassToCollection("DragDropData", "float getBuyValue()", "Gets the buy value of the resource.");
        LuaScriptApi::getInstance()->addClassToCollection("DragDropData", "bool getSenderReceiverIsSame()", "Gets whether the inventory sender and receiver is the same. E.g. moving the item within the same inventory.");
        LuaScriptApi::getInstance()->addClassToCollection("DragDropData", "string getSenderInventoryId()", "Gets the sender inventory id.");
        LuaScriptApi::getInstance()->addClassToCollection("DragDropData", "void setCanDrop(bool canDrop)", "Sets whether the item can be dropped.");
        LuaScriptApi::getInstance()->addClassToCollection("DragDropData", "bool getCanDrop()", "Gets whether the item can be dropped.");

        gameObjectClass.def("getMyGUIItemBoxComponent", &getMyGUIItemBoxComponentComponent);
        gameObjectClass.def("getMyGUIItemBoxComponentFromName", &getMyGUIItemBoxComponentComponentFromName);

        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "MyGUIItemBoxComponent getMyGUIItemBoxComponent()", "Gets the MyGUI item box component. This can be used for inventory item in conjunction with InventoryItemComponent.");
        LuaScriptApi::getInstance()->addClassToCollection("GameObject", "MyGUIItemBoxComponent getMyGUIItemBoxComponentFromName(string name)",
            "Gets the MyGUIItemBoxComponent by name. This can be used for inventory item in conjunction with InventoryItemComponent.");

        gameObjectControllerClass.def("castMyGUIItemBoxComponent", &GameObjectController::cast<MyGUIItemBoxComponent>);
        LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "MyGUIItemBoxComponent castMyGUIItemBoxComponent(MyGUIItemBoxComponent other)", "Casts for Lua auto completion.");
    }

    //////////////////////////////////////////////////////////////////////////////////

    DragDropData::DragDropData() : quantity(0), sellValue(0.0f), buyValue(0.0f), canDrop(false), senderReceiverIsSame(false), senderInventoryId(0L)
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
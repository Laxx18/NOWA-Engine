#ifndef MYGUI_ITEM_BOX_COMPONENT_H
#define MYGUI_ITEM_BOX_COMPONENT_H

#include "MyGUIComponents.h"
#include "MyGUI_XmlDocument.h"
#include "MyGUI_IResource.h"
#include "MyGUI_ResourceManager.h"
#include "BaseLayout/BaseLayout.h"
#include "ItemBox/BaseCellView.h"
#include "ItemBox/BaseItemBox.h"

namespace NOWA
{

	class ResourceItemInfo;
	typedef ResourceItemInfo* ResourceItemInfoPtr;

	class EXPORTED ResourceItemInfo :
		public MyGUI::IResource,
		public MyGUI::GenericFactory<ResourceItemInfo>
	{
		friend class MyGUI::GenericFactory<ResourceItemInfo>;

		MYGUI_RTTI_DERIVED(ResourceItemInfo)

	private:
		ResourceItemInfo()
		{
		}
		virtual ~ResourceItemInfo()
		{
		}

		virtual void deserialization(MyGUI::xml::ElementPtr _node, MyGUI::Version _version);

	public:
		void setItemName(const Ogre::String& itemName);

		Ogre::String getItemName(void) const;

		void setItemResourceImage(const Ogre::String& itemResourceImage);

		Ogre::String getItemResourceImage(void) const;

		void setItemDescription(const Ogre::String& itemDescription);

		Ogre::String getItemDescription(void) const;

	private:
		Ogre::String itemName;
		Ogre::String itemDescription;
		Ogre::String itemResourceImage;
	};

	///////////////////////////////////////////////////////////////////////////

	class EXPORTED ItemData
	{
	public:
		ItemData(unsigned long inventoryOwnerId);

		ItemData(const Ogre::String& resourceName, unsigned int quantity, Ogre::Real sellValue, Ogre::Real buyValue);

		bool isEmpty() const;

		bool compare(ItemData* data) const;

		void add(ItemData* data);

		void removeQuantity(unsigned int quantity);

		void clear();

		void setResourceName(const Ogre::String& resourceName);

		Ogre::String getResourceName(void) const;

		void setQuantity(unsigned int quantity);

		unsigned int getQuantity(void) const;

		void setSellValue(Ogre::Real sellValue);

		Ogre::Real getSellValue(void) const;

		void setBuyValue(Ogre::Real buyValue);

		Ogre::Real getBuyValue(void) const;

		ResourceItemInfoPtr getInfo(void) const;

		MyGUI::ResourceImageSetPtr getImage(void) const;

		void setInventoryOwnerId(unsigned long inventoryOwnerId);

		unsigned long getInventoryOwnerId(void) const;
	private:
		Ogre::String resourceName;
		unsigned int quantity;
		Ogre::Real sellValue;
		Ogre::Real buyValue;
		ResourceItemInfoPtr resourceInfo;
		MyGUI::ResourceImageSetPtr resourceImage;
		unsigned long inventoryOwnerId;
	};

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED ToolTip :
		public wraps::BaseLayout
	{
	public:
		ToolTip();

		void show(ItemData* _data);
		void hide();
		void move(const MyGUI::IntPoint& _point);

	private:
		MyGUI::TextBox* mTextName;
		MyGUI::TextBox* mTextCount;
		MyGUI::TextBox* mSellValue;
		MyGUI::TextBox* mBuyValue;
		MyGUI::EditBox* mTextDesc;
		MyGUI::ImageBox* mImageInfo;

		int mOffsetHeight;
	};

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED CellView :
		public wraps::BaseCellView<ItemData*>
	{
	public:
		CellView(MyGUI::Widget* _parent);

		void update(const MyGUI::IBDrawItemInfo& _info, ItemData* _data);
		static void getCellDimension(MyGUI::Widget* _sender, MyGUI::IntCoord& _coord, bool _drop);

	private:
		MyGUI::ImageBox* mImageBack;
		MyGUI::ImageBox* mImageBorder;
		MyGUI::ImageBox* mImageItem;
		MyGUI::TextBox* mTextBack;
		MyGUI::TextBox* mTextFront;
	};

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED ItemBox :
		public wraps::BaseItemBox<CellView>
	{
	public:
		ItemBox(MyGUI::Widget* _parent);

		virtual ~ItemBox();
	};

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class EXPORTED ItemBoxWindow :
		public wraps::BaseLayout
	{
	public:
		ItemBoxWindow(const std::string& _layout);
		ItemBox* getItemBox()
		{
			return mItemBox;
		}

	private:
		ItemBox* mItemBox;
	};
	
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	class DragDropData;
	
	class EXPORTED MyGUIItemBoxComponent : public MyGUIWindowComponent
	{
	public:

		typedef boost::shared_ptr<MyGUIItemBoxComponent> MyGUIItemBoxCompPtr;
	public:

		MyGUIItemBoxComponent();

		virtual ~MyGUIItemBoxComponent();

		/**
		* @see		GameObjectComponent::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement) override;

		/**
		* @see		GameObjectComponent::postInit
		*/
		virtual bool postInit(void) override;

		/**
		* @see		GameObjectComponent::connect
		*/
		virtual bool connect(void) override;

		/**
		* @see		GameObjectComponent::disconnect
		*/
		virtual bool disconnect(void) override;

		/**
		* @see		GameObjectComponent::getClassName
		*/
		virtual Ogre::String getClassName(void) const override;

		/**
		* @see		GameObjectComponent::getParentClassName
		*/
		virtual Ogre::String getParentClassName(void) const override;

		/**
		* @see		GameObjectComponent::clone
		*/
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("MyGUIItemBoxComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "MyGUIItemBoxComponent";
		}

		/**
		 * @see  GameObjectComponent::createStaticApiForLua
		 */
		static void createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController) { }

		/**
		* @see	GameObjectComponent::getStaticInfoText
		*/
		static Ogre::String getStaticInfoText(void)
		{
			return "Usage: A MyGUI UI item box component, for inventory usage in conjunction with InventoryItemComponent's.";
		}

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc) override;

		void setResourceLocationName(const Ogre::String& resourceLocationName);

		Ogre::String getResourceLocationName(void) const;
		
		void setUseToolTip(bool useToolTip);

		bool getUseToolTip(void) const;
		
		/**
		 * @brief Sets the inventory item count.
		 * @param[in] itemCount The item count to set
		 */
		void setItemCount(unsigned int itemCount);

		/**
		 * @brief Gets the item count
		 * @return itemCount The item count
		 */
		unsigned int getItemCount(void) const;
		
		void setResourceName(unsigned int index, const Ogre::String& resourceName);

		Ogre::String getResourceName(unsigned int index);
		
		void setQuantity(unsigned int index, unsigned int quantity);
		
		void setQuantity(const Ogre::String& resourceName, unsigned int quantity);

		unsigned int getQuantity(unsigned int index);
		
		unsigned int getQuantity(const Ogre::String& resourceName);

		void addQuantity(const Ogre::String& resourceName, unsigned int quantity);

		void increaseQuantity(const Ogre::String& resourceName, unsigned int quantity);

		void increaseQuantity(unsigned int index, unsigned int quantity);

		void decreaseQuantity(const Ogre::String& resourceName, unsigned int quantity);

		void decreaseQuantity(unsigned int index, unsigned int quantity);

		void removeQuantity(const Ogre::String& resourceName, unsigned int quantity);

		void setSellValue(unsigned int index, Ogre::Real sellValue);

		void setSellValue(const Ogre::String& resourceName, Ogre::Real sellValue);

		Ogre::Real getSellValue(unsigned int index);

		Ogre::Real getSellValue(const Ogre::String& resourceName);

		void setBuyValue(unsigned int index, Ogre::Real buyValue);

		void setBuyValue(const Ogre::String& resourceName, Ogre::Real buyValue);

		Ogre::Real getBuyValue(unsigned int index);

		Ogre::Real getBuyValue(const Ogre::String& resourceName);

		void setAllowDragDrop(bool allowDragDrop);

		bool getAllowDragDrop(void) const;

		void clearItems(void);

		void reactOnDropItemRequest(luabind::object closureFunction);

		void reactOnDropItemAccepted(luabind::object closureFunction);
	public:
		static const Ogre::String AttrResourceLocationName(void) { return "Resource Location Name"; }
		static const Ogre::String AttrUseToolTip(void) { return "Use ToolTip"; }
		static const Ogre::String AttrItemCount(void) { return "Item Count"; }
		static const Ogre::String AttrResourceName(void) { return "Resource Name "; }
		static const Ogre::String AttrQuantity(void) { return "Quantity "; }
		static const Ogre::String AttrSellValue(void) { return "Sell Value "; }
		static const Ogre::String AttrBuyValue(void) { return "Buy Value "; }
		static const Ogre::String AttrAllowDragDrop(void) { return "Allow Drag & Drop"; }
	protected:
		virtual void mouseButtonClick(MyGUI::Widget* sender) override;
	private:
		void notifyStartDrop(wraps::BaseLayout* _sender, wraps::DDItemInfo _info, bool& _result);
		void notifyRequestDrop(wraps::BaseLayout* _sender, wraps::DDItemInfo _info, bool& _result);
		void notifyEndDrop(wraps::BaseLayout* _sender, wraps::DDItemInfo _info, bool _result);
		void notifyDropState(wraps::BaseLayout* _sender, MyGUI::DDItemState _state);
		void notifyNotifyItem(wraps::BaseLayout* _sender, const MyGUI::IBNotifyItemData& _info);
		void requestDrawItem(MyGUI::ItemBox* _sender, MyGUI::Widget* _item, const MyGUI::IBDrawItemInfo& _info);
		void requestCreateWidgetItem(MyGUI::ItemBox* _sender, MyGUI::Widget* _item);
		void requestCoordItem(MyGUI::ItemBox* _sender, MyGUI::IntCoord& _coord, bool _drag);
		void notifyToolTip(wraps::BaseLayout* _sender, const MyGUI::ToolTipInfo& _info, ItemData* _data);
	private:
		int getIndexFromResourceName(const Ogre::String& resourceName);
	private:
		ToolTip* toolTip;
		ItemBoxWindow* itemBoxWindow;
		
		Variant* resourceLocationName;
		Variant* useToolTip;
		Variant* itemCount;
		std::vector<Variant*> resourceNames;
		std::vector<Variant*> quantities;
		std::vector<Variant*> sellValues;
		std::vector<Variant*> buyValues;
		Variant* allowDragDrop;
		luabind::object closureFunctionRequestDropRequest;
		luabind::object closureFunctionRequestDropAccepted;
		DragDropData* dragDropData;
		Ogre::String oldResourceLocationName;
		bool dropFinished;
	};

	//////////////////////////////////////////////////////////////////////////////////

	class EXPORTED DragDropData
	{
	public:
		friend class MyGUIItemBoxComponent;

	public:
		DragDropData();

		~DragDropData();

		Ogre::String getResourceName(void) const;

		unsigned int getQuantity(void) const;

		Ogre::Real getSellValue(void) const;

		Ogre::Real getBuyValue(void) const;

		bool getSenderReceiverIsSame(void) const;

		unsigned long getSenderInventoryId(void) const;

		void setCanDrop(bool canDrop);

		bool getCanDrop(void) const;

		void clear(void);

	private:
		Ogre::String resourceName;
		unsigned int quantity;
		Ogre::Real sellValue;
		Ogre::Real buyValue;
		bool canDrop;
		unsigned long senderInventoryId;
		bool senderReceiverIsSame;
	};

}; //namespace end



#endif
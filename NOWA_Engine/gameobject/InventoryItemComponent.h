#ifndef INVENTORY_ITEM_COMPONENT_H
#define INVENTORY_ITEM_COMPONENT_H

#include "GameObjectComponent.h"

namespace NOWA
{
	class EXPORTED InventoryItemComponent : public GameObjectComponent
	{
	public:

		typedef boost::shared_ptr<InventoryItemComponent> InventoryItemCompPtr;
	public:

		InventoryItemComponent();

		virtual ~InventoryItemComponent();

		/**
		* @see		GameObjectComponent::init
		*/
		virtual bool init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename = Ogre::String()) override;

		/**
		* @see		GameObjectComponent::postInit
		*/
		virtual bool postInit(void) override;

		/**
		* @see		GameObjectComponent::clone
		*/
		virtual GameObjectCompPtr clone(GameObjectPtr clonedGameObjectPtr) override;

		/**
		* @see		GameObjectComponent::connect
		*/
		virtual bool connect(void) override;

		/**
		* @see		GameObjectComponent::disconnect
		*/
		virtual bool disconnect(void) override;

		/**
		* @see		GameObjectComponent::update
		*/
		virtual void update(Ogre::Real dt, bool notSimulating = false) override { }

		/**
		* @see		GameObjectComponent::getClassName
		*/
		virtual Ogre::String getClassName(void) const override;

		/**
		* @see		GameObjectComponent::getParentClassName
		*/
		virtual Ogre::String getParentClassName(void) const override;

		
		static unsigned int getStaticClassId(void)
		{
			return NOWA::getIdFromName("InventoryItemComponent");
		}

		static Ogre::String getStaticClassName(void)
		{
			return "InventoryItemComponent";
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
			return "Usage: Is used in conjunction with MyGuiItemBoxComponent in order build a connection between an item like energy and an inventory slot. ";
		}

		/**
		* @see		GameObjectComponent::actualizeValue
		*/
		virtual void actualizeValue(Variant* attribute) override;

		/**
		* @see		GameObjectComponent::writeXML
		*/
		virtual void writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath) override;

		/**
		 * @brief		Sets the resource name, which must exist and also match the item resource name in a slot in @MyGuiItemBoxComponent.
		 * @param[in]	resourceName	The resource name to set.
		 */
		void setResourceName(const Ogre::String& resourceName);

		/**
		 * @brief		Gets the resource name.
		 * @return		The resource name to get.
		 */
		Ogre::String getResourceName(void) const;

		/**
		 * @brief		Sets the worth of the item, for how much amount it could be sold.
		 * @param[in]	sellValue	The sell value to set.
		 */
		void setSellValue(unsigned int sellValue);

		/**
		 * @brief		Gets the sell value of the item.
		 * @return		The sell value to get.
		 */
		unsigned int getSellValue(void) const;

		/**
		 * @brief		Sets how much this item does cost.
		 * @param[in]	buyValue	The buy value to set.
		 */
		void setBuyValue(unsigned int buyValue);

		/**
		 * @brief		Gets how much this item does cost.
		 * @return		The buy value to get.
		 */
		unsigned int getBuyValue(void) const;

		/**
		 * @brief		Adds the resource and given quantity to inventory (which is the MyGUIItemBoxComponent)
		 * @param[in]	gameObjectId	The game object id, that possesses the inventory
		 * @param[in]	componentName	The optional component name if e.g. the game object possesses more than one inventory. If left empty, the first inventory will be used.
		 * @param[in]	quantity		The quantity of the item to add
		 * @param[in]	once			If set to true, the component will be deleted, after the quantity has been added. This is useful, because e.g. in a contact callback it could be, that quantity would be added x-times until the game object has been deleted.
		 * @note		If the resource does not exist, it will be newly created, else the quantity for that resource will be just increased
		 */
		void addQuantityToInventory(unsigned long gameObjectId, const Ogre::String& componentName, int quantity, bool once);
		
		/**
		 * @brief		Removes the resource and given quantity from inventory (which is the MyGUIItemBoxComponent)
		 * @param[in]	gameObjectId	 The game object id, that possesses the inventory
		 * @param[in]	componentName	The optional component name if e.g. the game object possesses more than one inventory. If left empty, the first inventory will be used.
		 * @param[in]	quantity The quantity of the item to remove
		 * @param[in]	once			If set to true, the component will be deleted, after the quantity has been removed. This is useful, because e.g. in a contact callback it could be, that quantity would be added x-times until the game object has been deleted.
		 * @note		As long as after substracting this quantity from the inventory quanity is not zero, just the amount will be substracted, else the resource will be removed from inventory
		 */
		void removeQuantityFromInventory(unsigned long gameObjectId, const Ogre::String& componentName, int quantity, bool once);
	public:
		static const Ogre::String AttrResourceName(void) { return "Resource Name"; }
		static const Ogre::String AttrSellValue(void) { return "Sell Value"; }
		static const Ogre::String AttrBuyValue(void) { return "Buy Value"; }
	private:
		Variant* resourceName;
		Variant* sellValue;
		Variant* buyValue;
		bool alreadyAdded;
		bool alreadyRemoved;
	};

}; //namespace end

#endif
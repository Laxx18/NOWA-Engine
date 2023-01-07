#include "NOWAPrecompiled.h"
#include "InventoryItemComponent.h"
#include "GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "MyGUIItemBoxComponent.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	InventoryItemComponent::InventoryItemComponent()
		: GameObjectComponent(),
		resourceName(new Variant(InventoryItemComponent::AttrResourceName(), "", this->attributes)),
		sellValue(new Variant(InventoryItemComponent::AttrSellValue(), static_cast<unsigned int>(0), this->attributes)),
		buyValue(new Variant(InventoryItemComponent::AttrBuyValue(), static_cast<unsigned int>(0), this->attributes)),
		alreadyAdded(false),
		alreadyRemoved(false)
	{
		resourceName->setDescription("The resource name must match with the name in the XML specified by the ResourceLocationName of the MyGUIItemBoxComponent. E.g. InventoryItemsResources.xml.");
	}

	InventoryItemComponent::~InventoryItemComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[InventoryItemComponent] Destructor inventory item component for game object: " + this->gameObjectPtr->getName());
	}

	bool InventoryItemComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ResourceName")
		{
			this->resourceName->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SellValue")
		{
			this->sellValue->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BuyValue")
		{
			this->buyValue->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr InventoryItemComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		InventoryItemCompPtr clonedCompPtr(boost::make_shared<InventoryItemComponent>());

		
		clonedCompPtr->setResourceName(this->resourceName->getString());
		clonedCompPtr->setSellValue(this->sellValue->getUInt());
		clonedCompPtr->setBuyValue(this->buyValue->getUInt());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool InventoryItemComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[InventoryItemComponent] Init inventory item component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool InventoryItemComponent::connect(void)
	{
		this->alreadyAdded = false;
		this->alreadyRemoved = false;
		return true;
	}

	bool InventoryItemComponent::disconnect(void)
	{
		this->alreadyAdded = false;
		this->alreadyRemoved = false;
		return true;
	}

	void InventoryItemComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (InventoryItemComponent::AttrResourceName() == attribute->getName())
		{
			this->setResourceName(attribute->getString());
		}
		else if (InventoryItemComponent::AttrSellValue() == attribute->getName())
		{
			this->setSellValue(attribute->getUInt());
		}
		else if (InventoryItemComponent::AttrBuyValue() == attribute->getName())
		{
			this->setBuyValue(attribute->getUInt());
		}
	}

	void InventoryItemComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "ResourceName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->resourceName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SellValue"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->sellValue->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BuyValue"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->buyValue->getUInt())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String InventoryItemComponent::getClassName(void) const
	{
		return "InventoryItemComponent";
	}

	Ogre::String InventoryItemComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void InventoryItemComponent::setResourceName(const Ogre::String& resourceName)
	{
		this->resourceName->setValue(resourceName);
	}

	Ogre::String InventoryItemComponent::getResourceName(void) const
	{
		return this->resourceName->getString();
	}

	void InventoryItemComponent::setSellValue(unsigned int sellValue)
	{
		this->sellValue->setValue(sellValue);
	}

	unsigned int InventoryItemComponent::getSellValue(void) const
	{
		return this->sellValue->getUInt();
	}

	void InventoryItemComponent::setBuyValue(unsigned int buyValue)
	{
		this->buyValue->setValue(buyValue);
	}

	unsigned int InventoryItemComponent::getBuyValue(void) const
	{
		return this->buyValue->getUInt();
	}

	/*void InventoryItemComponent::setQuantity(unsigned int quantity)
	{
		this->quantity->setValue(quantity);
	}

	unsigned int InventoryItemComponent::getQuantity(void) const
	{
		return this->quantity->getUInt();
	}*/

	void InventoryItemComponent::addQuantityToInventory(unsigned long gameObjectId, const Ogre::String& componentName, int quantity, bool once)
	{
		if (true == once && true == this->alreadyAdded)
		{
			return;
		}

		auto& mainGameObject = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(gameObjectId);
		if (nullptr != mainGameObject)
		{
			boost::shared_ptr<MyGUIItemBoxComponent> myGUIItemBoxCompPtr;
			if (true == componentName.empty())
			{
				myGUIItemBoxCompPtr  = NOWA::makeStrongPtr(mainGameObject->getComponent<MyGUIItemBoxComponent>());
			}
			else
			{
				myGUIItemBoxCompPtr = NOWA::makeStrongPtr(mainGameObject->getComponentFromName<MyGUIItemBoxComponent>(componentName));
			}

			if (nullptr != myGUIItemBoxCompPtr)
			{
				myGUIItemBoxCompPtr->addQuantity(this->resourceName->getString(), quantity);
				myGUIItemBoxCompPtr->setSellValue(this->resourceName->getString(), this->sellValue->getReal());
				myGUIItemBoxCompPtr->setBuyValue(this->resourceName->getString(), this->buyValue->getReal());
			}

			if (true == once)
			{
				this->alreadyAdded = true;
			}
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[InventoryItemComponent] Error cannot add quantity to inventory. Because the the main game object has no inventory, for game object: " + this->gameObjectPtr->getName());
		}
	}

	void InventoryItemComponent::removeQuantityFromInventory(unsigned long gameObjectId, const Ogre::String& componentName, int quantity, bool once)
	{
		if (true == once && true == this->alreadyRemoved)
		{
			return;
		}

		auto& mainGameObject = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(gameObjectId);
		if (nullptr != mainGameObject)
		{
			boost::shared_ptr<MyGUIItemBoxComponent> myGUIItemBoxCompPtr;
			if (true == componentName.empty())
			{
				myGUIItemBoxCompPtr = NOWA::makeStrongPtr(mainGameObject->getComponent<MyGUIItemBoxComponent>());
			}
			else
			{
				myGUIItemBoxCompPtr = NOWA::makeStrongPtr(mainGameObject->getComponentFromName<MyGUIItemBoxComponent>(componentName));
			}

			if (nullptr != myGUIItemBoxCompPtr)
			{
				myGUIItemBoxCompPtr->removeQuantity(this->resourceName->getString(), quantity);
			}

			if (true == once)
			{
				this->alreadyRemoved = true;
			}
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[InventoryItemComponent] Error cannot remove quantity from inventory. Because the the main game object has no inventory, for game object: " + this->gameObjectPtr->getName());
		}
	}

}; // namespace end
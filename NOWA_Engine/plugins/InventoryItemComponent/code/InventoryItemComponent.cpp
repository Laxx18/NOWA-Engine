#include "NOWAPrecompiled.h"
#include "InventoryItemComponent.h"
#include "utilities/XMLConverter.h"
#include "gameObject/MyGUIItemBoxComponent.h"
#include "main/AppStateManager.h"

#include "gameobject/GameObjectFactory.h"

#include "OgreAbiUtils.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	InventoryItemComponent::InventoryItemComponent()
		: GameObjectComponent(),
		name("InventoryItemComponent"),
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
		
	}

	const Ogre::String& InventoryItemComponent::getName() const
	{
		return this->name;
	}

	void InventoryItemComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<InventoryItemComponent>(InventoryItemComponent::getStaticClassId(), InventoryItemComponent::getStaticClassName());
	}

	void InventoryItemComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool InventoryItemComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

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

	void InventoryItemComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();
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

	void InventoryItemComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "ResourceName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->resourceName->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SellValue"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->sellValue->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BuyValue"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->buyValue->getUInt())));
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

	// Lua registration part

	void _addQuantityToInventory(InventoryItemComponent* instance, const Ogre::String& id, int quantity, bool once)
	{
		instance->addQuantityToInventory(Ogre::StringConverter::parseUnsignedLong(id), "", quantity, once);
	}

	void _addQuantityToInventory2(InventoryItemComponent* instance, const Ogre::String& id, const Ogre::String& componentName, int quantity, bool once)
	{
		instance->addQuantityToInventory(Ogre::StringConverter::parseUnsignedLong(id), componentName, quantity, once);
	}

	void _removeQuantityFromInventory(InventoryItemComponent* instance, const Ogre::String& id, int quantity, bool once)
	{
		instance->removeQuantityFromInventory(Ogre::StringConverter::parseUnsignedLong(id), "", quantity, once);
	}

	void _removeQuantityFromInventory2(InventoryItemComponent* instance, const Ogre::String& id, const Ogre::String& componentName, int quantity, bool once)
	{
		instance->removeQuantityFromInventory(Ogre::StringConverter::parseUnsignedLong(id), componentName, quantity, once);
	}

	InventoryItemComponent* getInventoryItemComponentFromIndex(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<InventoryItemComponent>(gameObject->getComponentWithOccurrence<InventoryItemComponent>(occurrenceIndex)).get();
	}

	InventoryItemComponent* getInventoryItemComponent(GameObject* gameObject)
	{
		return makeStrongPtr<InventoryItemComponent>(gameObject->getComponent<InventoryItemComponent>()).get();
	}

	InventoryItemComponent* getInventoryItemComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<InventoryItemComponent>(gameObject->getComponentFromName<InventoryItemComponent>(name)).get();
	}

	void InventoryItemComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<InventoryItemComponent, GameObjectComponent>("InventoryItemComponent")
			// .def("getClassName", &InventoryItemComponent::getClassName)
			// .def("getClassId", &InventoryItemComponent::getClassId)
			.def("setResourceName", &InventoryItemComponent::setResourceName)
			.def("getResourceName", &InventoryItemComponent::getResourceName)
			// .def("setQuantity", &InventoryItemComponent::setQuantity)
			// .def("getQuantity", &InventoryItemComponent::getQuantity)
			// .def("addQuantityToInventory", &InventoryItemComponent::addQuantityToInventory)
			// .def("removeQuantityFromInventory", &InventoryItemComponent::removeQuantityFromInventory)
			.def("addQuantityToInventory", &_addQuantityToInventory)
			.def("addQuantityToInventory2", &_addQuantityToInventory2)
			.def("removeQuantityFromInventory", &_removeQuantityFromInventory)
			.def("removeQuantityFromInventory2", &_removeQuantityFromInventory2)
		];

		LuaScriptApi::getInstance()->addClassToCollection("InventoryItemComponent", "class inherits GameObjectComponent", InventoryItemComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("InventoryItemComponent", "void setResourceName(String resourceName)", "Sets the used resource name.");
		LuaScriptApi::getInstance()->addClassToCollection("InventoryItemComponent", "String getResourceName()", "Gets the used resource name.");
		LuaScriptApi::getInstance()->addClassToCollection("InventoryItemComponent", "void addQuantityToInventory(String inventoryIdGameObject, int quantity, bool once)", "Increases the quantity of this inventory item in the inventory game object. "
			"E.g. if inventory is used in MainGameObject, the following call is possible: 'inventoryItem:addQuantityToInventory(MAIN_GAMEOBJECT_ID, 1, true)'");
		LuaScriptApi::getInstance()->addClassToCollection("InventoryItemComponent", "void removeQuantityFromInventory(String inventoryIdGameObject, int quantity, bool once)", "Decreases the quantity of this inventory item in the inventory game object. "
			"E.g. if inventory is used in MainGameObject, the following call is possible: 'inventoryItem:removeQuantityFromInventory(MAIN_GAMEOBJECT_ID, 1, true)'");
		LuaScriptApi::getInstance()->addClassToCollection("InventoryItemComponent", "void addQuantityToInventory2(String inventoryIdGameObject, String componentName, int quantity, bool once)", "Increases the quantity of this inventory item in the inventory game object. "
			"E.g. if inventory is used in MainGameObject, the following call is possible: 'inventoryItem:addQuantityToInventory2(MAIN_GAMEOBJECT_ID, 'Player1InventoryComponent', 1, true)'");
		LuaScriptApi::getInstance()->addClassToCollection("InventoryItemComponent", "void removeQuantityFromInventory2(String inventoryIdGameObject, String componentName, int quantity, bool once)", "Decreases the quantity of this inventory item in the inventory game object. "
			"E.g. if inventory is used in MainGameObject, the following call is possible: 'inventoryItem:removeQuantityFromInventory2(MAIN_GAMEOBJECT_ID, 'Player1InventoryComponent', 1, true)'");
		LuaScriptApi::getInstance()->addClassToCollection("InventoryItemComponent", "Vecto2 getDimensions()", "Gets the color (r, g, b) of the billboard.");

		gameObjectClass.def("getInventoryItemComponentFromName", &getInventoryItemComponentFromName);
		gameObjectClass.def("getInventoryItemComponent", (InventoryItemComponent * (*)(GameObject*)) & getInventoryItemComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "InventoryItemComponent getInventoryItemComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "InventoryItemComponent getInventoryItemComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castInventoryItemComponent", &GameObjectController::cast<InventoryItemComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "InventoryItemComponent castInventoryItemComponent(InventoryItemComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool InventoryItemComponent::canStaticAddComponent(GameObject* gameObject)
	{
		return true;
	}

}; // namespace end
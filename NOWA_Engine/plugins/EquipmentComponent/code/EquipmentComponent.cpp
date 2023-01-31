#include "NOWAPrecompiled.h"
#include "EquipmentComponent.h"
#include "utilities/XMLConverter.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"

#include "OgreAbiUtils.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	EquipmentComponent::EquipmentComponent()
		: GameObjectComponent(),
		name("EquipmentComponent"),
		targetId(new Variant(EquipmentComponent::AttrTargetId(), static_cast<unsigned long>(0), this->attributes, true))
	{
		
	}

	EquipmentComponent::~EquipmentComponent(void)
	{
		
	}

	const Ogre::String& EquipmentComponent::getName() const
	{
		return this->name;
	}

	void EquipmentComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<EquipmentComponent>(EquipmentComponent::getStaticClassId(), EquipmentComponent::getStaticClassName());
	}
	
	void EquipmentComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool EquipmentComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetId")
		{
			this->targetId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		return true;
	}

	GameObjectCompPtr EquipmentComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		EquipmentComponentPtr clonedCompPtr(boost::make_shared<EquipmentComponent>());

		clonedCompPtr->setTargetId(this->targetId->getULong());
		
		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool EquipmentComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[EquipmentComponent] Init component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool EquipmentComponent::connect(void)
	{
		return true;
	}

	bool EquipmentComponent::disconnect(void)
	{
		
		return true;
	}

	bool EquipmentComponent::onCloned(void)
	{
		// Search for the prior id of the cloned game object and set the new id and set the new id, if not found set better 0, else the game objects may be corrupt!
		GameObjectPtr targetGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getClonedGameObjectFromPriorId(this->targetId->getULong());
		if (nullptr != targetGameObjectPtr)
			// Only the id is important!
			this->setTargetId(targetGameObjectPtr->getId());
		else
			this->setTargetId(0);

		return true;
	}

	void EquipmentComponent::onRemoveComponent(void)
	{
		
	}
	
	void EquipmentComponent::onOtherComponentRemoved(unsigned int index)
	{
		
	}
	
	void EquipmentComponent::onOtherComponentAdded(unsigned int index)
	{
		
	}
	
	void EquipmentComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			// Do something
		}
	}

	void EquipmentComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (EquipmentComponent::AttrTargetId() == attribute->getName())
		{
			this->setTargetId(attribute->getULong());
		}
	}

	void EquipmentComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TargetId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->targetId->getULong())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String EquipmentComponent::getClassName(void) const
	{
		return "EquipmentComponent";
	}

	Ogre::String EquipmentComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void EquipmentComponent::setActivated(bool activated)
	{
		
	}

	bool EquipmentComponent::isActivated(void) const
	{
		return true;
	}

	void EquipmentComponent::setTargetId(unsigned long targetId)
	{
		this->targetId->setValue(targetId);
	}

	unsigned long EquipmentComponent::getTargetId(void) const
	{
		return this->targetId->getULong();
	}

	// Lua registration part

	EquipmentComponent* getEquipmentComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<EquipmentComponent>(gameObject->getComponentWithOccurrence<EquipmentComponent>(occurrenceIndex)).get();
	}

	EquipmentComponent* getEquipmentComponent(GameObject* gameObject)
	{
		return makeStrongPtr<EquipmentComponent>(gameObject->getComponent<EquipmentComponent>()).get();
	}

	EquipmentComponent* getEquipmentComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<EquipmentComponent>(gameObject->getComponentFromName<EquipmentComponent>(name)).get();
	}

	void setTargetId(EquipmentComponent* instance, const Ogre::String& targetId)
	{
		instance->setTargetId(Ogre::StringConverter::parseUnsignedLong(targetId));
	}

	Ogre::String getTargetId(EquipmentComponent* instance)
	{
		return Ogre::StringConverter::toString(instance->getTargetId());
	}

	void EquipmentComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<EquipmentComponent, GameObjectComponent>("EquipmentComponent")
			.def("setTargetId", &setTargetId)
			.def("getTargetId", &getTargetId)
		];

		LuaScriptApi::getInstance()->addClassToCollection("EquipmentComponent", "class inherits GameObjectComponent", EquipmentComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("EquipmentComponent", "void setTargetId(string targetId)", "Sets the target equipped game object id. If set to 0, nothing is equipped.");
		LuaScriptApi::getInstance()->addClassToCollection("EquipmentComponent", "string getTargetId()", "Gets the target equipped game object id. If set to 0, nothing is equipped.");

		gameObjectClass.def("getEquipmentComponentFromName", &getEquipmentComponentFromName);
		gameObjectClass.def("getEquipmentComponent", (EquipmentComponent * (*)(GameObject*)) & getEquipmentComponent);
		// If its desired to create several of this components for one game object
		gameObjectClass.def("getEquipmentComponent2", (EquipmentComponent * (*)(GameObject*, unsigned int)) & getEquipmentComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "EquipmentComponent getEquipmentComponent2(unsigned int occurrenceIndex)", "Gets the component by the given occurence index, since a game object may this component maybe several times.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "EquipmentComponent getEquipmentComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "EquipmentComponent getEquipmentComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castEquipmentComponent", &GameObjectController::cast<EquipmentComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "EquipmentComponent castEquipmentComponent(EquipmentComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool EquipmentComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// No constraints so far, just add
		return true;
	}

}; //namespace end
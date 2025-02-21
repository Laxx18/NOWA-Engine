#include "NOWAPrecompiled.h"
#include "ReferenceComponent.h"
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

	ReferenceComponent::ReferenceComponent()
		: GameObjectComponent(),
		name("ReferenceComponent"),
		targetGameObject(nullptr),
		targetId(new Variant(ReferenceComponent::AttrTargetId(), static_cast<unsigned long>(0), this->attributes, true))
	{
		
	}

	ReferenceComponent::~ReferenceComponent(void)
	{
		
	}

	const Ogre::String& ReferenceComponent::getName() const
	{
		return this->name;
	}

	void ReferenceComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<ReferenceComponent>(ReferenceComponent::getStaticClassId(), ReferenceComponent::getStaticClassName());
	}
	
	void ReferenceComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool ReferenceComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetId")
		{
			this->targetId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		return true;
	}

	GameObjectCompPtr ReferenceComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		ReferenceComponentPtr clonedCompPtr(boost::make_shared<ReferenceComponent>());
		
		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->setTargetId(this->targetId->getULong());

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool ReferenceComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[ReferenceComponent] Init component for game object: " + this->gameObjectPtr->getName());

		AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &ReferenceComponent::deleteGameObjectDelegate), EventDataDeleteGameObject::getStaticEventType());

		return true;
	}

	bool ReferenceComponent::connect(void)
	{
		const auto& targetGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->targetId->getULong());
		if (nullptr != targetGameObjectPtr)
		{
			this->targetGameObject = targetGameObjectPtr.get();
		}
		return true;
	}

	bool ReferenceComponent::disconnect(void)
	{
		this->targetGameObject = nullptr;
		return true;
	}

	bool ReferenceComponent::onCloned(void)
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

	void ReferenceComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &ReferenceComponent::deleteGameObjectDelegate), EventDataDeleteGameObject::getStaticEventType());
	}
	
	void ReferenceComponent::onOtherComponentRemoved(unsigned int index)
	{
		
	}
	
	void ReferenceComponent::onOtherComponentAdded(unsigned int index)
	{
		
	}
	
	void ReferenceComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			// Do something
		}
	}

	void ReferenceComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (ReferenceComponent::AttrTargetId() == attribute->getName())
		{
			this->setTargetId(attribute->getULong());
		}
	}

	void ReferenceComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TargetId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->targetId->getULong())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String ReferenceComponent::getClassName(void) const
	{
		return "ReferenceComponent";
	}

	Ogre::String ReferenceComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void ReferenceComponent::setActivated(bool activated)
	{
		
	}

	bool ReferenceComponent::isActivated(void) const
	{
		return true;
	}

	void ReferenceComponent::setTargetId(unsigned long targetId)
	{
		this->targetId->setValue(targetId);
	}

	unsigned long ReferenceComponent::getTargetId(void) const
	{
		return this->targetId->getULong();
	}

	GameObject* ReferenceComponent::getTargetGameObject(void) const
	{
		return this->targetGameObject;
	}

	// Lua registration part

	ReferenceComponent* getReferenceComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<ReferenceComponent>(gameObject->getComponentWithOccurrence<ReferenceComponent>(occurrenceIndex)).get();
	}

	ReferenceComponent* getReferenceComponent(GameObject* gameObject)
	{
		return makeStrongPtr<ReferenceComponent>(gameObject->getComponent<ReferenceComponent>()).get();
	}

	ReferenceComponent* getReferenceComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<ReferenceComponent>(gameObject->getComponentFromName<ReferenceComponent>(name)).get();
	}

	void setTargetId(ReferenceComponent* instance, const Ogre::String& targetId)
	{
		instance->setTargetId(Ogre::StringConverter::parseUnsignedLong(targetId));
	}

	Ogre::String getTargetId(ReferenceComponent* instance)
	{
		return Ogre::StringConverter::toString(instance->getTargetId());
	}

	void ReferenceComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<ReferenceComponent, GameObjectComponent>("ReferenceComponent")
			.def("setTargetId", &setTargetId)
			.def("getTargetId", &getTargetId)
			.def("getTargetGameObject", &getTargetGameObject)
		];

		LuaScriptApi::getInstance()->addClassToCollection("ReferenceComponent", "class inherits GameObjectComponent", ReferenceComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("ReferenceComponent", "void setTargetId(string targetId)", "Sets the target referenced game object id. If set to 0, nothing is referenced.");
		LuaScriptApi::getInstance()->addClassToCollection("ReferenceComponent", "string getTargetId()", "Gets the target referenced game object id. If set to 0, nothing is referenced.");

		LuaScriptApi::getInstance()->addClassToCollection("ReferenceComponent", "GameObject getTargetGameObject()", "Gets the target referenced game object. If it does not exist, nil is delivered.");

		gameObjectClass.def("getReferenceComponentFromName", &getReferenceComponentFromName);
		gameObjectClass.def("getReferenceComponent", (ReferenceComponent * (*)(GameObject*)) & getReferenceComponent);
		// If its desired to create several of this components for one game object
		gameObjectClass.def("getReferenceComponentFromIndex", (ReferenceComponent * (*)(GameObject*, unsigned int)) & getReferenceComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ReferenceComponent getReferenceComponentFromIndex(unsigned int occurrenceIndex)", "Gets the component by the given occurence index, since a game object may this component maybe several times.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ReferenceComponent getReferenceComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "ReferenceComponent getReferenceComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castReferenceComponent", &GameObjectController::cast<ReferenceComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "ReferenceComponent castReferenceComponent(ReferenceComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	void ReferenceComponent::deleteGameObjectDelegate(EventDataPtr eventData)
	{
		boost::shared_ptr<EventDataDeleteGameObject> castEventData = boost::static_pointer_cast<NOWA::EventDataDeleteGameObject>(eventData);
		unsigned long id = castEventData->getGameObjectId();
		if (nullptr != this->targetGameObject)
		{
			if (id == this->targetGameObject->getId())
			{
				this->targetGameObject = nullptr;
			}
		}
	}

	bool ReferenceComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// No constraints so far, just add
		return true;
	}

}; //namespace end
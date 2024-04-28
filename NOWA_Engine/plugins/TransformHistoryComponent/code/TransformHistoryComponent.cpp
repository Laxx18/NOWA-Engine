#include "NOWAPrecompiled.h"
#include "TransformHistoryComponent.h"
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

	TransformHistoryComponent::TransformHistoryComponent()
		: GameObjectComponent(),
		name("TransformHistoryComponent"),
		isTransforming(false),
		oldPosition(Ogre::Vector3::ZERO),
		oldOrientation(Ogre::Quaternion::IDENTITY),
		gameObjectStateHistory(nullptr),
		activated(new Variant(TransformHistoryComponent::AttrActivated(), true, this->attributes)),
		targetId(new Variant(TransformHistoryComponent::AttrTargetId(), static_cast<unsigned long>(0), this->attributes, true)),
		historyLength(new Variant(TransformHistoryComponent::AttrHistoryLength(), static_cast<unsigned int>(10), this->attributes)),
		pastTime(new Variant(TransformHistoryComponent::AttrPastTime(), static_cast<unsigned int>(1000), this->attributes)),
		orientate(new Variant(TransformHistoryComponent::AttrOrientate(), true, this->attributes)),
		useDelay(new Variant(TransformHistoryComponent::AttrUseDelay(), false, this->attributes)),
		stopImmediately(new Variant(TransformHistoryComponent::AttrStopImmediately(), true, this->attributes))
	{
		this->activated->setDescription("If activated the behavior takes place.");
		this->targetId->setDescription("Sets target id for the game object, which shall be linear interpolated.");
		this->historyLength->setDescription("Controls how long the history is and how often a value is saved, setHistoryLength(10) means that a value is saved every 100 ms and therefore 10 values in one second. "
											"Calculated history size, this must be large enough even in the case of high latency and thus a high past value, have enough saved values available for interpolation.");
		this->pastTime->setDescription("Sets how much the target game object is transformed in the past of the given game object.");
		this->orientate->setDescription("Sets whether the target game object also shall be orientated besides movement.");
		this->useDelay->setDescription("Sets whether to use delay on transform. That means, the target game object will start transform and replay the path to the source game object, if the source game object has stopped.");
		this->stopImmediately->setDescription("Sets whether the target game object shall stop its interpolation immediately, if this game object stops transform.");
	}

	TransformHistoryComponent::~TransformHistoryComponent(void)
	{
	
	}

	const Ogre::String& TransformHistoryComponent::getName() const
	{
		return this->name;
	}

	void TransformHistoryComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<TransformHistoryComponent>(TransformHistoryComponent::getStaticClassId(), TransformHistoryComponent::getStaticClassName());
	}
	
	void TransformHistoryComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool TransformHistoryComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &TransformHistoryComponent::handleTargetGameObjectDeleted), EventDataDeleteGameObject::getStaticEventType());

		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetId")
		{
			this->targetId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "HistoryLength")
		{
			this->historyLength->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PastTime")
		{
			this->pastTime->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Orientate")
		{
			this->orientate->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UseDelay")
		{
			this->useDelay->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "StopImmediately")
		{
			this->stopImmediately->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		return true;
	}

	GameObjectCompPtr TransformHistoryComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		TransformHistoryComponentPtr clonedCompPtr(boost::make_shared<TransformHistoryComponent>());

		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setHistoryLength(this->historyLength->getUInt());
		clonedCompPtr->setPastTime(this->pastTime->getUInt());
		clonedCompPtr->setOrientate(this->orientate->getBool());
		clonedCompPtr->setUseDelay(this->useDelay->getBool());
		clonedCompPtr->setStopImmediately(this->stopImmediately->getBool());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->setTargetId(this->targetId->getULong());

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool TransformHistoryComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[TransformHistoryComponent] Init component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool TransformHistoryComponent::connect(void)
	{
		this->oldPosition = this->gameObjectPtr->getPosition();
		this->oldOrientation = this->gameObjectPtr->getOrientation();

		const auto& targetGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->targetId->getULong());
		if (nullptr != targetGameObjectPtr)
		{
			this->targetGameObject = targetGameObjectPtr.get();
			this->gameObjectStateHistory = new GameObjectStateHistory();
			this->gameObjectStateHistory->init(100, this->historyLength->getUInt() * 100, false == this->useDelay->getBool());
		}
		return true;
	}

	bool TransformHistoryComponent::disconnect(void)
	{
		this->isTransforming = false;
		this->targetGameObject = nullptr;

		if (nullptr != this->gameObjectStateHistory)
		{
			delete this->gameObjectStateHistory;
			this->gameObjectStateHistory = nullptr;
		}
		return true;
	}

	bool TransformHistoryComponent::onCloned(void)
	{
		// Search for the prior id of the cloned game object and set the new id and set the new id, if not found set better 0, else the game objects may be corrupt!
		GameObjectPtr targetGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getClonedGameObjectFromPriorId(this->targetId->getULong());
		if (nullptr != targetGameObjectPtr)
		{
			// Only the id is important!
			this->setTargetId(targetGameObjectPtr->getId());
		}
		else
		{
			this->setTargetId(0);
		}

		return true;
	}

	void TransformHistoryComponent::onRemoveComponent(void)
	{
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &TransformHistoryComponent::handleTargetGameObjectDeleted), EventDataDeleteGameObject::getStaticEventType());

		if (nullptr != this->gameObjectStateHistory)
		{
			delete this->gameObjectStateHistory;
			this->gameObjectStateHistory = nullptr;
		}
	}
	
	void TransformHistoryComponent::onOtherComponentRemoved(unsigned int index)
	{
		
	}
	
	void TransformHistoryComponent::onOtherComponentAdded(unsigned int index)
	{
		
	}

	void TransformHistoryComponent::handleTargetGameObjectDeleted(NOWA::EventDataPtr eventData)
	{
		// Target game object removed, reset pointer
		boost::shared_ptr<NOWA::EventDataDeleteGameObject> castEventData = boost::static_pointer_cast<EventDataDeleteGameObject>(eventData);

		if (nullptr != this->targetGameObject)
		{
			if (castEventData->getGameObjectId() == this->targetGameObject->getId())
			{
				this->targetGameObject = nullptr;
			}
		}
	}
	
	void TransformHistoryComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating && true == this->activated->getBool())
		{
			this->isTransforming = false;

			Ogre::Real distanceSQ = (this->gameObjectPtr->getPosition() - this->oldPosition).squaredLength();

			if (distanceSQ > 0.01f * 0.01f)
			{
				this->isTransforming = true;
			}

			if (false == this->gameObjectPtr->getOrientation().equals(this->oldOrientation, Ogre::Degree(0.5f)))
			{
				this->isTransforming = true;
			}

			this->oldPosition = this->gameObjectPtr->getPosition();
			this->oldOrientation = this->gameObjectPtr->getOrientation();

			// Only add transform to history, if source game object is transforming or stop immediately is off
			if (false == this->stopImmediately->getBool() || true == this->isTransforming)
			{
				this->gameObjectStateHistory->enqeue(this->gameObjectPtr->getPosition(), this->gameObjectPtr->getOrientation(), RakNet::GetTimeMS());
			}

			if (nullptr != this->targetGameObject)
			{
				Ogre::Vector3 sourcePosition;
				Ogre::Quaternion sourceOrientation;

				this->gameObjectStateHistory->readPiecewiseLinearInterpolated(sourcePosition, sourceOrientation, RakNet::GetTimeMS() - this->pastTime->getUInt());

				auto physicsActiveComponent = NOWA::makeStrongPtr(targetGameObject->getComponent<PhysicsActiveComponent>());
				if (nullptr != physicsActiveComponent)
				{
					physicsActiveComponent->setPosition(sourcePosition);
					if (true == this->orientate->getBool())
					{
						// TODO: Check this
						physicsActiveComponent->setOrientation(sourceOrientation);
					}
				}
				else
				{
					auto physicsKinematicComponent = NOWA::makeStrongPtr(targetGameObject->getComponent<PhysicsActiveKinematicComponent>());
					if (nullptr != physicsKinematicComponent)
					{
						// TODO: Check this
					}
					else
					{
						auto physicsPlayerControllerComponent = NOWA::makeStrongPtr(targetGameObject->getComponent<PhysicsPlayerControllerComponent>());
						if (nullptr != physicsKinematicComponent)
						{
							// TODO: Check this
						}
						else
						{
							// No physics component
							this->targetGameObject->getSceneNode()->setPosition(sourcePosition);
							if (true == this->orientate->getBool())
							{
								this->targetGameObject->getSceneNode()->setOrientation(sourceOrientation);
							}
						}
					}
				}
			}
		}
	}

	void TransformHistoryComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (TransformHistoryComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (TransformHistoryComponent::AttrTargetId() == attribute->getName())
		{
			this->setTargetId(attribute->getULong());
		}
		else if (TransformHistoryComponent::AttrHistoryLength() == attribute->getName())
		{
			this->setHistoryLength(attribute->getUInt());
		}
		else if (TransformHistoryComponent::AttrPastTime() == attribute->getName())
		{
			this->setPastTime(attribute->getUInt());
		}
		else if(TransformHistoryComponent::AttrOrientate() == attribute->getName())
		{
			this->setOrientate(attribute->getBool());
		}
		else if (TransformHistoryComponent::AttrUseDelay() == attribute->getName())
		{
			this->setUseDelay(attribute->getBool());
		}
		else if (TransformHistoryComponent::AttrStopImmediately() == attribute->getName())
		{
			this->setStopImmediately(attribute->getBool());
		}
	}

	void TransformHistoryComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TargetId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->targetId->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "HistoryLength"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->historyLength->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "PastTime"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->pastTime->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Orientate"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->orientate->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UseDelay"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->useDelay->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "StopImmediately"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->stopImmediately->getBool())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String TransformHistoryComponent::getClassName(void) const
	{
		return "TransformHistoryComponent";
	}

	Ogre::String TransformHistoryComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void TransformHistoryComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
	}

	bool TransformHistoryComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void TransformHistoryComponent::setTargetId(unsigned long targetId)
	{
		this->targetId->setValue(targetId);
	}

	unsigned long TransformHistoryComponent::getTargetId(void) const
	{
		return this->targetId->getULong();
	}

	void TransformHistoryComponent::setHistoryLength(unsigned int historyLength)
	{
		this->historyLength->setValue(historyLength);
	}

	unsigned int TransformHistoryComponent::getHistoryLength(void) const
	{
		return this->historyLength->getUInt();
	}

	void TransformHistoryComponent::setPastTime(unsigned int pastTimeMS)
	{
		this->pastTime->setValue(pastTimeMS);
	}

	unsigned int TransformHistoryComponent::getPastTime(void) const
	{
		return this->pastTime->getUInt();
	}

	void TransformHistoryComponent::setOrientate(bool orientate)
	{
		this->orientate->setValue(orientate);
	}

	bool TransformHistoryComponent::getOrientate(void) const
	{
		return this->orientate->getBool();
	}

	void TransformHistoryComponent::setUseDelay(bool useDelay)
	{
		this->useDelay->setValue(useDelay);
	}

	bool TransformHistoryComponent::getUseDelay(void) const
	{
		return this->useDelay->getBool();
	}

	void TransformHistoryComponent::setStopImmediately(bool stopImmediately)
	{
		this->stopImmediately->setValue(stopImmediately);
	}

	bool TransformHistoryComponent::getStopImmediately(void) const
	{
		return this->stopImmediately->getBool();
	}

	GameObject* TransformHistoryComponent::getTargetGameObject(void) const
	{
		return this->targetGameObject;
	}

	// Lua registration part

	TransformHistoryComponent* getTransformHistoryComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<TransformHistoryComponent>(gameObject->getComponentWithOccurrence<TransformHistoryComponent>(occurrenceIndex)).get();
	}

	TransformHistoryComponent* getTransformHistoryComponent(GameObject* gameObject)
	{
		return makeStrongPtr<TransformHistoryComponent>(gameObject->getComponent<TransformHistoryComponent>()).get();
	}

	TransformHistoryComponent* getTransformHistoryComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<TransformHistoryComponent>(gameObject->getComponentFromName<TransformHistoryComponent>(name)).get();
	}

	void setTargetId(TransformHistoryComponent* instance, const Ogre::String& targetId)
	{
		instance->setTargetId(Ogre::StringConverter::parseUnsignedLong(targetId));
	}

	Ogre::String getTargetId(TransformHistoryComponent* instance)
	{
		return Ogre::StringConverter::toString(instance->getTargetId());
	}

	void TransformHistoryComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<TransformHistoryComponent, GameObjectComponent>("TransformHistoryComponent")
			.def("setActivated", &TransformHistoryComponent::setActivated)
			.def("isActivated", &TransformHistoryComponent::isActivated)
			.def("setTargetId", &setTargetId)
			.def("getTargetId", &getTargetId)
			.def("setHistoryLength", &TransformHistoryComponent::setHistoryLength)
			.def("getHistoryLength", &TransformHistoryComponent::getHistoryLength)
			.def("setPastTime", &TransformHistoryComponent::setPastTime)
			.def("getPastTime", &TransformHistoryComponent::getPastTime)
			.def("setOrientate", &TransformHistoryComponent::setOrientate)
			.def("getOrientate", &TransformHistoryComponent::getOrientate)
			.def("setUseDelay", &TransformHistoryComponent::setUseDelay)
			.def("getUseDelay", &TransformHistoryComponent::getUseDelay)
			.def("setStopImmediately", &TransformHistoryComponent::setStopImmediately)
			.def("getStopImmediately", &TransformHistoryComponent::getStopImmediately)
			.def("getTargetGameObject", &getTargetGameObject)
		];

		LuaScriptApi::getInstance()->addClassToCollection("TransformHistoryComponent", "class inherits GameObjectComponent", TransformHistoryComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("TransformHistoryComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not.");
		LuaScriptApi::getInstance()->addClassToCollection("TransformHistoryComponent", "bool isActivated()", "Gets whether this component is activated.");

		LuaScriptApi::getInstance()->addClassToCollection("TransformHistoryComponent", "void setTargetId(string targetId)", "Sets target id for the game object, which shall be linear interpolated.");
		LuaScriptApi::getInstance()->addClassToCollection("TransformHistoryComponent", "string getTargetId()", "Gets the target id for the game object, which is linear interpolated.");
		LuaScriptApi::getInstance()->addClassToCollection("TransformHistoryComponent", "void setHistoryLength(number historyLength)", "Controls how long the history is and how often a value is saved, setHistoryLength(10) means that a value is saved every 100 ms and therefore 10														values in one second. "
															  "Calculated history size, this must be large enough even in the case of high latency and thus a high past value, have enough saved values available for interpolation.");
		LuaScriptApi::getInstance()->addClassToCollection("TransformHistoryComponent", "number getHistoryLength()", "Gets the history length.");

		LuaScriptApi::getInstance()->addClassToCollection("TransformHistoryComponent", "void setPastTime(number pastTimeMS)", "Sets how much the target game object is transformed in the past of the given game object. Default value is 1000ms.");
		LuaScriptApi::getInstance()->addClassToCollection("TransformHistoryComponent", "number getPastTime()", "Gets how much the target game object is transformed in the past of the given game object. Default value is 1000ms.");
		LuaScriptApi::getInstance()->addClassToCollection("TransformHistoryComponent", "void setOrientate(bool orientate)", "Sets whether the target game object also shall be orientated besides movement.");
		LuaScriptApi::getInstance()->addClassToCollection("TransformHistoryComponent", "bool getOrientate()", "Gets whether the target game object also shall be orientated besides movement.");
		LuaScriptApi::getInstance()->addClassToCollection("TransformHistoryComponent", "void setUseDelay(bool useDelay)", "Sets whether to use delay on transform. That means, the target game object will start transform and replay the path to the source game object, if the source game object has stopped.");
		LuaScriptApi::getInstance()->addClassToCollection("TransformHistoryComponent", "bool getUseDelay()", "Gets whether delay on transform is used. That means, the target game object will start transform and replay the path to the source game object, if the source game object has stopped.");
		LuaScriptApi::getInstance()->addClassToCollection("TransformHistoryComponent", "void setStopImmediately(bool stopImmediately)", "Sets whether the target game object shall stop its interpolation immediately, if this game object stops transform.");
		LuaScriptApi::getInstance()->addClassToCollection("TransformHistoryComponent", "bool getStopImmediately()", "Gets whether the target game object stops its interpolation immediately, if this game object stops transform.");

		LuaScriptApi::getInstance()->addClassToCollection("TransformHistoryComponent", "GameObject getTargetGameObject()", "Gets the target referenced game object. If it does not exist, nil is delivered.");


		gameObjectClass.def("getTransformHistoryComponentFromName", &getTransformHistoryComponentFromName);
		gameObjectClass.def("getTransformHistoryComponent", (TransformHistoryComponent * (*)(GameObject*)) & getTransformHistoryComponent);
		// If its desired to create several of this components for one game object
		gameObjectClass.def("getTransformHistoryComponent2", (TransformHistoryComponent * (*)(GameObject*, unsigned int)) & getTransformHistoryComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "TransformHistoryComponent getTransformHistoryComponent2(unsigned int occurrenceIndex)", "Gets the component by the given occurence index, since a game object may this component maybe several times.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "TransformHistoryComponent getTransformHistoryComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "TransformHistoryComponent getTransformHistoryComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castTransformHistoryComponent", &GameObjectController::cast<TransformHistoryComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "TransformHistoryComponent castTransformHistoryComponent(TransformHistoryComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool TransformHistoryComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// No constraints so far, just add
		return true;
	}

}; //namespace end
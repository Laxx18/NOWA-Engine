#include "NOWAPrecompiled.h"
#include "AreaOfInterestComponent.h"
#include "utilities/XMLConverter.h"
#include "gameobject/ActivationComponent.h"
#include "modules/LuaScriptApi.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"
#include "gameobject/GameObjectFactory.h"

#include "OgreAbiUtils.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	AreaOfInterestComponent::AreaOfInterestComponent()
		: GameObjectComponent(),
		name("AreaOfInterestComponent"),
		sphereSceneQuery(nullptr),
		triggerSphereQueryObserver(nullptr),
		triggerUpdateTimer(0.0f),
		categoriesId(GameObjectController::ALL_CATEGORIES_ID),
		luaScriptComponent(nullptr),
		activated(new Variant(AreaOfInterestComponent::AttrActivated(), true, this->attributes)),
		radius(new Variant(AreaOfInterestComponent::AttrRadius(), 10.0f, this->attributes)),
		updateThreshold(new Variant(AreaOfInterestComponent::AttrUpdateThreshold(), 0.5f, this->attributes)),
		categories(new Variant(AreaOfInterestComponent::AttrCategories(), Ogre::String("All"), this->attributes)),
		shortTimeActivation(new Variant(AreaOfInterestComponent::AttrShortTimeActivation(), false, this->attributes)),
		triggerPermanentely(new Variant(AreaOfInterestComponent::AttrTriggerPermanentely(), true, this->attributes))
	{
		this->shortTimeActivation->setDescription("If set to true, this component will be deactivated shortly after it has been activated."
			"Useful for short action, like check if player is located at an item.");
		this->updateThreshold->setDescription("Sets how often the area is checked for game objects. Default is 0.5 seconds. The lower the value, the more often the area is checked, but a performance costs, but more precise actualized results.");
		this->triggerPermanentely->setDescription("Sets whether to trigger e.g. the enter function permanentely or just once and it only can be retriggered if the leave function has been called and vice versa.");
	}

	AreaOfInterestComponent::~AreaOfInterestComponent(void)
	{
		if (nullptr != this->sphereSceneQuery)
		{
			this->gameObjectPtr->getSceneManager()->destroyQuery(this->sphereSceneQuery);
			this->sphereSceneQuery = nullptr;
		}

		if (nullptr != this->triggerSphereQueryObserver)
		{
			delete this->triggerSphereQueryObserver;
			this->triggerSphereQueryObserver = nullptr;
		}
	}

	void AreaOfInterestComponent::initialise()
	{

	}

	const Ogre::String& AreaOfInterestComponent::getName() const
	{
		return this->name;
	}

	void AreaOfInterestComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<AreaOfInterestComponent>(AreaOfInterestComponent::getStaticClassId(), AreaOfInterestComponent::getStaticClassName());
	}

	void AreaOfInterestComponent::shutdown()
	{

	}

	void AreaOfInterestComponent::uninstall()
	{

	}

	void AreaOfInterestComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool AreaOfInterestComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ShortTimeActivation")
		{
			this->shortTimeActivation->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Radius")
		{
			this->radius->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "UpdateThreshold")
		{
			this->updateThreshold->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Categories")
		{
			this->categories->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == AreaOfInterestComponent::AttrTriggerPermanentely())
		{
			this->triggerPermanentely->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr AreaOfInterestComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		AreaOfInterestCompPtr clonedCompPtr(boost::make_shared<AreaOfInterestComponent>());

		clonedCompPtr->setShortTimeActivation(this->shortTimeActivation->getBool());
		clonedCompPtr->setRadius(this->radius->getReal());
		clonedCompPtr->setUpdateThreshold(this->updateThreshold->getReal());
		clonedCompPtr->setCategories(this->categories->getString());
		clonedCompPtr->setTriggerPermanentely(this->triggerPermanentely->getBool());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->setActivated(this->activated->getBool());


		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool AreaOfInterestComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AreaOfInterestComponent] Init area of interest component for game object: " + this->gameObjectPtr->getName());

		AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &AreaOfInterestComponent::deleteGameObjectDelegate), EventDataDeleteGameObject::getStaticEventType());

		this->sphereSceneQuery = this->gameObjectPtr->getSceneManager()->createSphereQuery(Ogre::Sphere(this->gameObjectPtr->getPosition(), this->radius->getReal()));

		return true;
	}

	bool AreaOfInterestComponent::connect(void)
	{
		// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AreaOfInterestComponent] Connecting area of interest component for game object: " + this->gameObjectPtr->getName());

		this->setActivated(this->activated->getBool());

		// In post init not all game objects are known, and so there are maybe no categories yet, so set the categories here
		this->setCategories(this->categories->getString());

		return true;
	}

	bool AreaOfInterestComponent::disconnect(void)
	{
		this->triggerUpdateTimer = 0.0f;
		return true;
	}

	bool AreaOfInterestComponent::onCloned(void)
	{

		return true;
	}

	void AreaOfInterestComponent::onRemoveComponent(void)
	{
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &AreaOfInterestComponent::deleteGameObjectDelegate), EventDataDeleteGameObject::getStaticEventType());
	}

	void AreaOfInterestComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating && true == this->activated->getBool())
		{
			this->checkAreaForActiveObjects(dt);
		}
	}

	void AreaOfInterestComponent::deleteGameObjectDelegate(EventDataPtr eventData)
	{
		boost::shared_ptr<EventDataDeleteGameObject> castEventData = boost::static_pointer_cast<NOWA::EventDataDeleteGameObject>(eventData);
		// if a game object has been deleted elsewhere remove it from the queue, in order not to work with dangling pointers
		unsigned long id = castEventData->getGameObjectId();
		for (auto& it = this->triggeredGameObjects.cbegin(); it != this->triggeredGameObjects.cend();)
		{
			if (it->second.first->getId() == id)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[AreaOfInterestComponent] Removing gameobject: " + it->second.first->getName() + " from life time queue");
				it = this->triggeredGameObjects.erase(it);
				break;
			}
			else
			{
				++it;
			}
		}
	}

	void AreaOfInterestComponent::attachTriggerObserver(AreaOfInterestComponent::ITriggerSphereQueryObserver* triggerSphereQueryObserver)
	{
		this->triggerSphereQueryObserver = triggerSphereQueryObserver;
	}

#if 0
	void AreaOfInterestComponent::checkAreaForActiveObjects(Ogre::Real dt)
	{
		if (this->updateThreshold->getReal() > 0.0f)
		{
			this->triggerUpdateTimer += dt;
		}

		// Checks area only 2x a second
		if (this->triggerUpdateTimer >= this->updateThreshold->getReal())
		{
			this->triggerUpdateTimer = 0.0f;

			Ogre::Sphere updateSphere(this->gameObjectPtr->getPosition(), this->radius->getReal());
			this->sphereSceneQuery->setSphere(updateSphere);
			// if (this->categoriesId > 0)
			// 	this->sphereSceneQuery->setQueryMask(this->categoriesId);

			// Checks objects in range
			Ogre::SceneQueryResultMovableList& result = this->sphereSceneQuery->execute().movables;
			for (auto& it = result.cbegin(); it != result.cend(); ++it)
			{
				Ogre::MovableObject* movableObject = *it;

				// if the query flags are part of the final category
				// unsigned int finalCategory = this->sphereSceneQuery->getQueryMask() & movableObject->getQueryFlags();
				// if (this->sphereSceneQuery->getQueryMask() == finalCategory)
				{
					GameObject* gameObject = nullptr;
					try
					{
						if ("Camera" != movableObject->getMovableType())
						{
							gameObject = Ogre::any_cast<GameObject*>(movableObject->getUserObjectBindings().getUserAny());
						}
					}
					catch (Ogre::Exception&)
					{

					}
					// Do not trigger this game object itself!
					if (nullptr != gameObject && gameObject != this->gameObjectPtr.get())
					{
						auto activationComponent = NOWA::makeStrongPtr(gameObject->getComponent<ActivationComponent>());
						if (nullptr != activationComponent)
						{
							// Activate all components of that game object
							activationComponent->setActivated(true);
						}

						// when this is active, the onEnter callback will only be called once for the object!
						auto& it = this->triggeredGameObjects.find(gameObject->getId());
						if (it == this->triggeredGameObjects.end())
						{
							// notify the observer
							if (nullptr != this->triggerSphereQueryObserver)
							{
								this->triggerSphereQueryObserver->onEnter(gameObject);
							}

							// Call also function in lua script, if it does exist in the lua script component
							if (nullptr != this->gameObjectPtr->getLuaScript())
							{
								if (this->enterClosureFunction.is_valid())
								{
									try
									{
										luabind::call_function<void>(this->enterClosureFunction, gameObject);
									}
									catch (luabind::error& error)
									{
										luabind::object errorMsg(luabind::from_stack(error.state(), -1));
										std::stringstream msg;
										msg << errorMsg;

										Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnEnter' Error: " + Ogre::String(error.what())
																					+ " details: " + msg.str());
									}
								}
							}

							// add the game object to a map
							this->triggeredGameObjects.emplace(gameObject->getId(), gameObject);
						}
					}
					else
					{
						// Else call with null

						if (nullptr != this->triggerSphereQueryObserver)
						{
							this->triggerSphereQueryObserver->onEnter(nullptr);
						}

						// Call also function in lua script, if it does exist in the lua script component
						if (nullptr != this->gameObjectPtr->getLuaScript())
						{
							if (this->enterClosureFunction.is_valid())
							{
								try
								{
									luabind::call_function<void>(this->enterClosureFunction, nullptr);
								}
								catch (luabind::error& error)
								{
									luabind::object errorMsg(luabind::from_stack(error.state(), -1));
									std::stringstream msg;
									msg << errorMsg;

									Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnEnter' Error: " + Ogre::String(error.what())
																				+ " details: " + msg.str());
								}
							}
						}
						break;
					}
				}
			}

			// Goes through the map with the triggered game objects that are in range
			for (auto& it = this->triggeredGameObjects.cbegin(); it != this->triggeredGameObjects.cend();)
			{
				GameObject* gameObject = it->second;
				Ogre::Vector3& direction = this->gameObjectPtr->getPosition() - gameObject->getPosition();
				// if a game objects comes out of the range, remove it and notify the observer
				Ogre::Real distanceToGameObject = direction.squaredLength();
				Ogre::Real radius = this->radius->getReal();
				if (distanceToGameObject > radius * radius)
				{
					auto activationComponent = NOWA::makeStrongPtr(gameObject->getComponent<ActivationComponent>());
					if (nullptr != activationComponent)
					{
						// Activate all components of that game object
						activationComponent->setActivated(false);
					}

					if (nullptr != this->triggerSphereQueryObserver)
					{
						this->triggerSphereQueryObserver->onLeave(gameObject);
					}

					// Call also function in lua script, if it does exist in the lua script component
					if (nullptr != this->gameObjectPtr->getLuaScript())
					{
						if (this->leaveClosureFunction.is_valid())
						{
							try
							{
								luabind::call_function<void>(this->leaveClosureFunction, gameObject);
							}
							catch (luabind::error& error)
							{
								luabind::object errorMsg(luabind::from_stack(error.state(), -1));
								std::stringstream msg;
								msg << errorMsg;

								Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnLeave' Error: " + Ogre::String(error.what())
																			+ " details: " + msg.str());
							}
						}
					}

					it = this->triggeredGameObjects.erase(it);
				}
				else
				{
					++it;
				}
			}

			// Deactivates after half a second if flag is set
			if (true == this->shortTimeActivation->getBool())
			{
				this->setActivated(false);
			}
		}
	}
#else
//void AreaOfInterestComponent::checkAreaForActiveObjects(Ogre::Real dt)
//{
//	if (this->updateThreshold->getReal() > 0.0f)
//	{
//		this->triggerUpdateTimer += dt;
//	}
//
//	// Checks area only 2x a second
//	if (this->triggerUpdateTimer >= this->updateThreshold->getReal())
//	{
//		this->triggerUpdateTimer = 0.0f;
//
//		Ogre::Sphere updateSphere(this->gameObjectPtr->getPosition(), this->radius->getReal());
//		this->sphereSceneQuery->setSphere(updateSphere);
//
//		// Checks objects in range
//		Ogre::SceneQueryResultMovableList& result = this->sphereSceneQuery->execute().movables;
//		for (auto& it = result.cbegin(); it != result.cend(); ++it)
//		{
//			Ogre::MovableObject* movableObject = *it;
//
//			GameObject* gameObject = nullptr;
//			try
//			{
//				if ("Camera" != movableObject->getMovableType())
//				{
//					gameObject = Ogre::any_cast<GameObject*>(movableObject->getUserObjectBindings().getUserAny());
//				}
//			}
//			catch (Ogre::Exception&)
//			{
//			}
//
//			if (nullptr != gameObject && gameObject != this->gameObjectPtr.get())
//			{
//				auto& otherIt = this->triggeredGameObjects.find(gameObject->getId());
//				if (otherIt == this->triggeredGameObjects.end())
//				{
//					this->callEnterFunction(gameObject);
//					// Adds the game object to a map and mark as entered
//					this->triggeredGameObjects.emplace(gameObject->getId(), std::make_pair(gameObject, true));
//				}
//				else
//				{
//					if (false == otherIt->second.second || true == this->triggerPermanentely->getBool()) // Not already entered
//					{
//						this->callEnterFunction(gameObject);
//						// Mark as entered
//						otherIt->second.second = true;
//					}
//				}
//			}
//		}
//
//		// Check for objects that have left the range
//		for (auto it = this->triggeredGameObjects.begin(); it != this->triggeredGameObjects.end();)
//		{
//			GameObject* gameObject = it->second.first;
//			Ogre::Vector3 direction = this->gameObjectPtr->getPosition() - gameObject->getPosition();
//			Ogre::Real distanceToGameObject = direction.squaredLength();
//			Ogre::Real radius = this->radius->getReal();
//
//			if (distanceToGameObject > radius * radius)
//			{
//				this->callLeaveFunction(gameObject);
//
//				// Mark as left
//				it->second.second = false;
//
//				// Remove the object from the map if it's no longer in range
//				// it = this->triggeredGameObjects.erase(it);
//
//				++it;
//			}
//			else
//			{
//				++it;
//			}
//		}
//
//		if (this->shortTimeActivation->getBool())
//		{
//			this->setActivated(false);
//		}
//	}
//}

void AreaOfInterestComponent::checkAreaForActiveObjects(Ogre::Real dt)
{
	if (this->updateThreshold->getReal() > 0.0f)
	{
		this->triggerUpdateTimer += dt;
	}

	// Check area at regular intervals
	if (this->triggerUpdateTimer >= this->updateThreshold->getReal())
	{
		this->triggerUpdateTimer = 0.0f;

		Ogre::Sphere updateSphere(this->gameObjectPtr->getPosition(), this->radius->getReal());
		this->sphereSceneQuery->setSphere(updateSphere);

		// Track objects currently within range
		std::unordered_set<unsigned long> currentObjectsInRange;

		// Check objects in range
		Ogre::SceneQueryResultMovableList& result = this->sphereSceneQuery->execute().movables;
		for (auto& it = result.cbegin(); it != result.cend(); ++it)
		{
			Ogre::MovableObject* movableObject = *it;

			GameObject* gameObject = nullptr;
			try
			{
				if ("Camera" != movableObject->getMovableType())
				{
					gameObject = Ogre::any_cast<GameObject*>(movableObject->getUserObjectBindings().getUserAny());
				}
			}
			catch (Ogre::Exception&)
			{
			}

			if (nullptr != gameObject && gameObject != this->gameObjectPtr.get())
			{
				if (false == this->triggerPermanentely->getBool())
				{
					currentObjectsInRange.insert(gameObject->getId());
				}

				auto& otherIt = this->triggeredGameObjects.find(gameObject->getId());
				if (otherIt == this->triggeredGameObjects.end())
				{
					// First time entering
					this->callEnterFunction(gameObject);
					this->triggeredGameObjects.emplace(gameObject->getId(), std::make_pair(gameObject, true));
				}
				else if (!otherIt->second.second) // Enter only if previously left
				{
					this->callEnterFunction(gameObject);
					otherIt->second.second = true; // Mark as entered
				}
			}
		}

		// Check for objects that have left the range
		for (auto it = this->triggeredGameObjects.begin(); it != this->triggeredGameObjects.end();)
		{
			unsigned long objectId = it->first;
			GameObject* gameObject = it->second.first;

			if (currentObjectsInRange.find(objectId) == currentObjectsInRange.end()) // Not in range
			{
				if (it->second.second) // Leave only if previously entered
				{
					this->callLeaveFunction(gameObject);
					it->second.second = false; // Mark as left
				}

				// Optionally remove the object from the map
				if (!this->triggerPermanentely->getBool())
				{
					it = this->triggeredGameObjects.erase(it);
				}
				else
				{
					++it;
				}
			}
			else
			{
				++it;
			}
		}

		if (this->shortTimeActivation->getBool())
		{
			this->setActivated(false);
		}
	}
}

void AreaOfInterestComponent::logLuaError(const Ogre::String& context, const luabind::error& error)
{
	luabind::object errorMsg(luabind::from_stack(error.state(), -1));
	std::stringstream msg;
	msg << errorMsg;

	Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL,
		"[LuaScript] Caught error in '" + context + "' Error: " +
		Ogre::String(error.what()) + " details: " + msg.str());
}

void AreaOfInterestComponent::callEnterFunction(GameObject* gameObject)
{
	// Notify observer and call Lua function
	if (this->triggerSphereQueryObserver)
	{
		this->triggerSphereQueryObserver->onEnter(gameObject);
	}

	if (this->gameObjectPtr->getLuaScript() && this->enterClosureFunction.is_valid())
	{
		try
		{
			luabind::call_function<void>(this->enterClosureFunction, gameObject);
		}
		catch (luabind::error& error)
		{
			logLuaError("reactOnEnter", error);
		}
	}
}

void AreaOfInterestComponent::callLeaveFunction(GameObject* gameObject)
{
	// Notify observer and call Lua function
	if (this->triggerSphereQueryObserver)
	{
		this->triggerSphereQueryObserver->onLeave(gameObject);
	}

	if (this->gameObjectPtr->getLuaScript() && this->leaveClosureFunction.is_valid())
	{
		try
		{
			luabind::call_function<void>(this->leaveClosureFunction, gameObject);
		}
		catch (luabind::error& error)
		{
			logLuaError("reactOnLeave", error);
		}
	}
}

#endif

	void AreaOfInterestComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (AreaOfInterestComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (AreaOfInterestComponent::AttrShortTimeActivation() == attribute->getName())
		{
			this->setShortTimeActivation(attribute->getBool());
		}
		else if (AreaOfInterestComponent::AttrRadius() == attribute->getName())
		{
			this->setRadius(attribute->getReal());
		}
		else if (AreaOfInterestComponent::AttrUpdateThreshold() == attribute->getName())
		{
			this->setUpdateThreshold(attribute->getReal());
		}
		else if (AreaOfInterestComponent::AttrCategories() == attribute->getName())
		{
			this->setCategories(attribute->getString());
		}
		else if (AreaOfInterestComponent::AttrTriggerPermanentely() == attribute->getName())
		{
			this->setTriggerPermanentely(attribute->getBool());
		}
	}

	void AreaOfInterestComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ShortTimeActivation"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->shortTimeActivation->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Radius"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->radius->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "UpdateThreshold"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->updateThreshold->getReal())));
		propertiesXML->append_node(propertyXML);
		

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Categories"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->categories->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TriggerPermanentely"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->triggerPermanentely->getBool())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String AreaOfInterestComponent::getClassName(void) const
	{
		return "AreaOfInterestComponent";
	}

	Ogre::String AreaOfInterestComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void AreaOfInterestComponent::setActivated(bool activated)
	{
		this->triggerUpdateTimer = 0.0f;

		auto luaScriptCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<LuaScriptComponent>());
		if (nullptr != luaScriptCompPtr)
		{
			this->luaScriptComponent = luaScriptCompPtr.get();
		}

		if (true == activated)
		{
			if (nullptr != luaScriptComponent && false == luaScriptComponent->isActivated())
			{
				// If not activated, first activate the lua script component, so that the script will be compiled, because its necessary for this component
				// luaScriptComponent->setActivated(true);
				boost::shared_ptr<EventDataPrintLuaError> eventDataPrintLuaError(new EventDataPrintLuaError(this->gameObjectPtr->getLuaScript()->getScriptName(), this->gameObjectPtr->getLuaScript()->getScriptFilePathName(), 0,
					"Cannot activate component, because the 'LuaScriptComponent' is not activated for game object: " + this->gameObjectPtr->getName()));
				AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataPrintLuaError);
				return;
			}
		}

		this->activated->setValue(activated);

		if (false == activated)
		{
			// If not activated remove all objects
			for (auto& it = this->triggeredGameObjects.cbegin(); it != this->triggeredGameObjects.cend();)
			{
				GameObject* gameObject = it->second.first;

				auto activationComponent = NOWA::makeStrongPtr(gameObject->getComponent<ActivationComponent>());
				if (nullptr != activationComponent)
				{
					// Activate all components of that game object
					activationComponent->setActivated(false);
				}

				if (nullptr != this->triggerSphereQueryObserver)
				{
					this->triggerSphereQueryObserver->onLeave(gameObject);
				}

				// Call also function in lua script, if it does exist in the lua script component
				if (nullptr != this->gameObjectPtr->getLuaScript())
				{
					if (this->leaveClosureFunction.is_valid())
					{
						try
						{
							luabind::call_function<void>(this->leaveClosureFunction, gameObject);
						}
						catch (luabind::error& error)
						{
							luabind::object errorMsg(luabind::from_stack(error.state(), -1));
							std::stringstream msg;
							msg << errorMsg;

							Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnLeave' Error: " + Ogre::String(error.what())
																		+ " details: " + msg.str());
						}
					}
				}

				it = this->triggeredGameObjects.erase(it);
			}
		}
	}

	void AreaOfInterestComponent::setRadius(Ogre::Real radius)
	{
		if (radius < 0.1f)
		{
			radius = 0.1f;
		}
		this->radius->setValue(radius);
	}

	Ogre::Real AreaOfInterestComponent::getRadius(void) const
	{
		return this->radius->getReal();
	}

	bool AreaOfInterestComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void AreaOfInterestComponent::setCategories(const Ogre::String& categories)
	{
		this->categories->setValue(categories);
		this->categoriesId = AppStateManager::getSingletonPtr()->getGameObjectController()->generateCategoryId(categories);

		if (this->categoriesId > 0)
		{
			this->sphereSceneQuery->setQueryMask(this->categoriesId);
		}
	}

	Ogre::String AreaOfInterestComponent::getCategories(void) const
	{
		return this->categories->getString();
	}

	void AreaOfInterestComponent::setUpdateThreshold(Ogre::Real updateThreshold)
	{
		if (updateThreshold < 0.0f)
		{
			updateThreshold = 0.0f;
		}
		this->updateThreshold->setValue(updateThreshold);
	}

	Ogre::Real AreaOfInterestComponent::getUpdateThreshold(void) const
	{
		return this->updateThreshold->getReal();
	}

	void AreaOfInterestComponent::reactOnEnter(luabind::object closureFunction)
	{
		this->enterClosureFunction = closureFunction;
	}

	void AreaOfInterestComponent::reactOnLeave(luabind::object closureFunction)
	{
		this->leaveClosureFunction = closureFunction;
	}

	void AreaOfInterestComponent::setShortTimeActivation(bool shortTimeActivation)
	{
		this->shortTimeActivation->setValue(shortTimeActivation);
	}

	bool AreaOfInterestComponent::getShortTimeActivation(void) const
	{
		return this->shortTimeActivation->getBool();
	}

	void AreaOfInterestComponent::setTriggerPermanentely(bool triggerPermanentely)
	{
		this->triggerPermanentely->setValue(triggerPermanentely);
	}

	bool AreaOfInterestComponent::getTriggerPermanentely(void) const
	{
		return this->triggerPermanentely->getBool();
	}

	// Lua registration part

	AreaOfInterestComponent* getAreaOfInterestComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<AreaOfInterestComponent>(gameObject->getComponentWithOccurrence<AreaOfInterestComponent>(occurrenceIndex)).get();
	}

	AreaOfInterestComponent* getAreaOfInterestComponent(GameObject* gameObject)
	{
		return makeStrongPtr<AreaOfInterestComponent>(gameObject->getComponent<AreaOfInterestComponent>()).get();
	}

	AreaOfInterestComponent* getAreaOfInterestComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<AreaOfInterestComponent>(gameObject->getComponentFromName<AreaOfInterestComponent>(name)).get();
	}

	void AreaOfInterestComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObject, class_<GameObjectController>& gameObjectController)
	{
		module(lua)
		[
			class_<AreaOfInterestComponent, GameObjectComponent>("AreaOfInterestComponent")
			.def("setActivated", &AreaOfInterestComponent::setActivated)
			.def("isActivated", &AreaOfInterestComponent::isActivated)
			.def("setRadius", &AreaOfInterestComponent::setRadius)
			.def("getRadius", &AreaOfInterestComponent::getRadius)
			.def("reactOnEnter", &AreaOfInterestComponent::reactOnEnter)
			.def("reactOnLeave", &AreaOfInterestComponent::reactOnLeave)
		];

		LuaScriptApi::getInstance()->addClassToCollection("AreaOfInterestComponent", "class inherits GameObjectComponent", AreaOfInterestComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("AreaOfInterestComponent", "void setActivated(bool activated)", "Sets whether this component should be activated or not (Start checking for objects within area radius).");
		LuaScriptApi::getInstance()->addClassToCollection("AreaOfInterestComponent", "bool isActivated()", "Gets whether this component is activated.");
		LuaScriptApi::getInstance()->addClassToCollection("AreaOfInterestComponent", "void setRadius(float radius)", "Sets the area check radius in meters.");
		LuaScriptApi::getInstance()->addClassToCollection("AreaOfInterestComponent", "float getRadius()", "Gets the area check radius in meters.");

		LuaScriptApi::getInstance()->addClassToCollection("AreaOfInterestComponent", "void reactOnEnter(func closure, otherGameObject)",
							 "Sets whether to react at the moment when a game object enters the area.");
		LuaScriptApi::getInstance()->addClassToCollection("AreaOfInterestComponent", "void reactOnLeave(func closure, otherGameObject)",
							 "Sets whether to react at the moment when a game object leaves the area. Always check if the game object does exist. It may also be null.");

		gameObject.def("getAreaOfInterestComponent", (AreaOfInterestComponent * (*)(GameObject*)) & getAreaOfInterestComponent);
		gameObject.def("getAreaOfInterestComponentFromIndex", (AreaOfInterestComponent * (*)(GameObject*, unsigned int)) & getAreaOfInterestComponent);
		gameObject.def("getAreaOfInterestComponentFromName", &getAreaOfInterestComponentFromName);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "AreaOfInterestComponent getAreaOfInterestComponentFromIndex(unsigned int occurrenceIndex)", "Gets the area of interest component by the given occurence index, since a game object may have besides other components several area of interest components.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "AreaOfInterestComponent getAreaOfInterestComponent()", "Gets the area of interest component. This can be used if the game object just has one area of interest component.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "AreaOfInterestComponent getAreaOfInterestComponentFromName(String name)", "Gets the area of interest component.");

		gameObjectController.def("castAreaOfInterestComponent", &GameObjectController::cast<AreaOfInterestComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "AreaOfInterestComponent castAreaOfInterestComponent(AreaOfInterestComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool AreaOfInterestComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// No constraints so far
		return true;
	}

}; //namespace end
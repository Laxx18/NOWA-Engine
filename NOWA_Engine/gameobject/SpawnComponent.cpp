#include "NOWAPrecompiled.h"
#include "SpawnComponent.h"
#include "GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "PhysicsActiveComponent.h"
#include "main/Core.h"
#include "modules/LuaScriptApi.h"
#include "LuaScriptComponent.h"
#include "main/EventManager.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	SpawnComponent::SpawnComponent()
		: GameObjectComponent(),
		activated(new Variant(SpawnComponent::AttrActivated(), true, this->attributes)),
		interval(new Variant(SpawnComponent::AttrInterval(), static_cast<unsigned int>(10000), this->attributes)),
		count(new Variant(SpawnComponent::AttrCount(), static_cast<unsigned int>(10), this->attributes)),
		lifeTime(new Variant(SpawnComponent::AttrLifeTime(), static_cast<unsigned int>(10000), this->attributes)),
		offsetPosition(new Variant(SpawnComponent::AttrOffsetPosition(), Ogre::Vector3(0.0f, 0.0f, 1.0f), this->attributes)),
		offsetOrientation(new Variant(SpawnComponent::AttrOffsetOrientation(), Ogre::Vector3::ZERO, this->attributes)),
		spawnAtOrigin(new Variant(SpawnComponent::AttrSpawnAtOrigin(), false, this->attributes)),
		spawnTargetId(new Variant(SpawnComponent::AttrSpawnTargetId(), static_cast<unsigned long>(0), this->attributes, true)),
		initPosition(Ogre::Vector3::ZERO),
		initOrientation(Ogre::Quaternion::IDENTITY),
		spawnObserver(nullptr),
		spawnTargetGameObject(nullptr),
		firstTimeSetSpawnTarget(true),
		keepAlive(false),
		currentCount(0),
		spawnTimer(0)
	{
		this->offsetOrientation->setDescription("Orientation is set in the form: (degreeX, degreeY, degreeZ)");

		this->activated->setDescription("Activates the components behaviour, so that the spawning will begin.");
		this->interval->setDescription("Sets the spawn interval in milliseconds. Note: E.g. an interval of 10000 would spawn a game object each 10 seconds.");
		this->count->setDescription("Sets the spawn count. Note: E.g. a count of 100 would spawn 100 game objects of this type. When the count is set to 0, the spawning will never stop.");
		this->lifeTime->setDescription("Sets the spawned game object life time in milliseconds. When the life time is set to 0, the spawned gameobject will live forever. Note: E.g. a life time of 10000 would let live the spawned game object for 10 seconds.");
		this->offsetPosition->setDescription("Sets the offset position at which the spawned game objects will occur relative to the origin game object.");
		this->offsetOrientation->setDescription("Sets an offset orientation at which the spawned game objects will occur relative to the origin game object.");
		this->spawnAtOrigin->setDescription("Sets whether the spawn new game objects always at the init position or the current position of the original game object. "
			"Note: If spawn at origin is set to false. New game objects will be spawned at the current position of the original game object. Thus complex scenarios are possible! "
			"E.g. pin the original active physical game object at a rotating block. When spawning a new game object implement the spawn callback and add for the spawned game object "
			"an impulse at the current direction. Spawned game objects will be catapultated in all directions while the original game object is being rotated!");
		this->spawnTargetId->setDescription("Sets spawn target game object id. Note: There can also another game object name (scenenode name in Ogitor) be specified which should be spawned "
			"at the position of this game object. E.g. a rotating case shall spawn coins, so the case has the spawn component and as spawn target the coin scene node name.");

		AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &SpawnComponent::deleteGameObjectDelegate), EventDataDeleteGameObject::getStaticEventType());
	}

	SpawnComponent::~SpawnComponent()
	{
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &SpawnComponent::deleteGameObjectDelegate), EventDataDeleteGameObject::getStaticEventType());
		
		if (this->spawnObserver)
		{
			delete this->spawnObserver;
			this->spawnObserver = nullptr;
		}
		this->spawnTargetGameObject = nullptr;
		this->keepAlive = false;
		this->reset();

		// this->spawnQueue.Clear(_FILE_AND_LINE_);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[SpawnComponent] Destructor spawn component for game object: " + this->gameObjectPtr->getName());
	}

	bool SpawnComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SpawnActivate")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SpawnInterval")
		{
			this->setIntervalMS(XMLConverter::getAttribUnsignedInt(propertyElement, "data", 10000));
			
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SpawnCount")
		{
			this->setCount(XMLConverter::getAttribUnsignedInt(propertyElement, "data", 10));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SpawnLifeTime")
		{
			this->setLifeTimeMS(XMLConverter::getAttribUnsignedInt(propertyElement, "data", 10000));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SpawnOffsetPosition")
		{
			this->setOffsetPosition(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SpawnOffsetOrientation")
		{
			this->setOffsetOrientation(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SpawnAtOrigin")
		{
			this->setSpawnAtOrigin(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SpawnTarget")
		{
			this->setSpawnTargetId(XMLConverter::getAttribUnsignedLong(propertyElement, "data", 0));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr SpawnComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		if (this->spawnTargetId->getULong() == this->gameObjectPtr->getId())
		{
			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsSpawnComponent] Could not clone spawn component for game object: " + this->gameObjectPtr->getName()
			// 	+ " because, its spawn target id points to the same game object, which would result in an endless recursion. Cloning is only possible, if the spawn target id points to a different game object.");
			return GameObjectCompPtr();
		}

		SpawnCompPtr clonedCompPtr(boost::make_shared<SpawnComponent>());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setIntervalMS(this->interval->getUInt());
		clonedCompPtr->setCount(this->count->getUInt());
		clonedCompPtr->setLifeTimeMS(this->lifeTime->getUInt());
		clonedCompPtr->setOffsetPosition(this->offsetPosition->getVector3());
		clonedCompPtr->setOffsetOrientation(this->offsetOrientation->getVector3());
		clonedCompPtr->setSpawnAtOrigin(this->spawnAtOrigin->getBool());
		clonedCompPtr->setSpawnTargetId(this->spawnTargetId->getULong());
		
		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));

		return clonedCompPtr;
	}

	bool SpawnComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsSpawnComponent] Init physics spawn component for game object: " + this->gameObjectPtr->getName());

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		this->setSpawnTargetId(this->spawnTargetId->getULong());
		
		// bool success = this->initSpawnData();
		
		return true;
	}

	bool SpawnComponent::connect(void)
	{
		bool success = this->initSpawnData();

		return success;
	}

	bool SpawnComponent::disconnect(void)
	{
		bool tempKeepAlive = this->keepAlive;
		this->keepAlive = false;
		this->reset();
		this->keepAlive = tempKeepAlive;
		
		// If the spawner is the same game object as this, set it visible to false
		// if (this->spawnTargetId->getULong() == this->gameObjectPtr->getId())
		if (nullptr != spawnTargetGameObject)
		{
			this->spawnTargetGameObject->getSceneNode()->setVisible(true);
		}

		return true;
	}

	bool SpawnComponent::onCloned(void)
	{
		// Search for the prior id of the cloned game object and set the new id and set the new id, if not found set better 0, else the game objects may be corrupt!
		GameObjectPtr spawnTargetGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getClonedGameObjectFromPriorId(this->spawnTargetId->getULong());
		if (nullptr != spawnTargetGameObjectPtr)
			// Only the id is important!
			this->setSpawnTargetId(spawnTargetGameObjectPtr->getId());
		else
			this->setSpawnTargetId(0);
		return true;
	}

	bool SpawnComponent::initSpawnData(void)
	{
		// Get the physics component because the spawn component operates on it.
		this->physicsCompPtr = makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsComponent>());
		/*if (nullptr == this->physicsCompPtr)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[SpawnComponent] Could not init spawn data because the game object: " 
				+ this->gameObjectPtr->getName() + " has no kind of physics component!");
			return false;
		}*/

		// Store the init position
		this->initPosition = this->gameObjectPtr->getPosition();
		this->initOrientation = this->gameObjectPtr->getOrientation();

		auto& tempGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->spawnTargetId->getULong());
		if (nullptr == tempGameObjectPtr)
		{
			return false;
		}
		
		this->spawnTargetGameObject = tempGameObjectPtr.get();
		
		return true;
	}

	Ogre::String SpawnComponent::getClassName(void) const
	{
		return "SpawnComponent";
	}

	Ogre::String SpawnComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void SpawnComponent::deleteGameObjectDelegate(EventDataPtr eventData)
	{
		boost::shared_ptr<EventDataDeleteGameObject> castEventData = boost::static_pointer_cast<NOWA::EventDataDeleteGameObject>(eventData);
		// if a game object has been deleted elsewhere remove it from the queue, in order not to work with dangling pointers
		unsigned long id = castEventData->getGameObjectId();

		// Deactivated, because it would also thrown, when just NOWA-Design is exited :(
		if (nullptr != this->spawnTargetGameObject && this->spawnTargetGameObject->getId() == id && false == AppStateManager::getSingletonPtr()->getIsShutdown())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[SpawnComponent] Spawn target id: " + Ogre::StringConverter::toString(this->spawnTargetId->getULong()) + " is about to be destroyed, which is an illegal case!");
			// throw Ogre::Exception(Ogre::Exception::ERR_ITEM_NOT_FOUND, "[SpawnComponent] Spawn target id: " + Ogre::StringConverter::toString(this->spawnTargetId->getULong()) + " is about to be destroyed, which is an illegal case!", "NOWA");
		}

		for (auto& it = this->lifeTimeQueue.cbegin(); it != this->lifeTimeQueue.cend();)
		{
			if (it->second->getId() == id)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[SpawnComponent] Removing gameobject: " + it->second->getName() + " from life time queue");
				it = lifeTimeQueue.erase(it);
				break;
			}
			else
			{
				++it;
			}
		}

		for (auto& it = this->clonedGameObjectsInScene.begin(); it != this->clonedGameObjectsInScene.end();)
		{
			if ((*it)->getId() == id)
			{
				it = this->clonedGameObjectsInScene.erase(it);
				break;
			}
			else
			{
				++it;
			}
		}
	}

	void SpawnComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (this->activated->getBool() && false == notSimulating)
		{
			if (nullptr == this->spawnTargetGameObject)
			{
				return;
			}

			/*if (nullptr != this->physicsCompPtr)
			{
				this->physicsCompPtr->setPosition(this->initPosition);
				this->physicsCompPtr->setOrientation(this->initOrientation);
			}
			else
			{
				this->gameObjectPtr->getSceneNode()->setPosition(this->initPosition);
				this->gameObjectPtr->getSceneNode()->setOrientation(this->initOrientation);
			}*/

			this->initPosition = this->gameObjectPtr->getPosition();
			this->initOrientation = this->gameObjectPtr->getOrientation();
			
			// Search for spawn target game object here, because in post init it could be, that the target game object has not been loaded at that time!
			if (true == this->firstTimeSetSpawnTarget)
			{
				// If the spawner is the same game object as this, set it visible to false when simulating
				if (this->spawnTargetId->getULong() == this->gameObjectPtr->getId())
				{
					this->spawnTargetGameObject->getSceneNode()->setVisible(false);
				}
				
				if (this->spawnTargetId->getULong() != this->gameObjectPtr->getId())
				{
					auto& tempGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->spawnTargetId->getULong());
					if (nullptr == tempGameObjectPtr)
					{
						Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[SpawnComponent] Spawn target id: " + Ogre::StringConverter::toString(this->spawnTargetId->getULong()) + " does not exist!");
						// throw Ogre::Exception(Ogre::Exception::ERR_ITEM_NOT_FOUND, "[SpawnComponent] Spawn target id: " + Ogre::StringConverter::toString(this->spawnTargetId->getULong()) + " does not exist!", "NOWA");
						return;
					}
					else
					{
						this->spawnTargetGameObject = tempGameObjectPtr.get();
					}
					this->firstTimeSetSpawnTarget = false;
				}
			}
			if (this->currentCount <= this->count->getUInt())
			{
				// count fractions of time for spawning
				// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "t: " + Ogre::StringConverter::toString(this->spawnTimer) + " dt: " + Ogre::StringConverter::toString(static_cast<unsigned long>(dt * 1000.0f)));
				this->spawnTimer += dt * 1000.0f;
// Attention: here floating error since timer is float and interval a whole number, so a rest must also be stored!
				if (this->spawnTimer >= this->interval->getUInt())
				{
					// count 0 means infinity!
					if (this->count->getUInt() != 0)
					{
						this->currentCount += 1;
					}
					if (this->currentCount <= this->count->getUInt())
					{
						this->spawnTimer = (this->spawnTimer - static_cast<Ogre::Real>(this->interval->getUInt()));
						// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "f: " + Ogre::StringConverter::toString(this->spawnTimer));

						GameObjectPtr clonedGameObjectPtr;
						if (true == this->spawnAtOrigin->getBool())
						{
							clonedGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->clone(this->spawnTargetGameObject->getId(), nullptr, 0,
								this->initPosition + (this->initOrientation * this->offsetPosition->getVector3()),
								this->initOrientation * MathHelper::getInstance()->degreesToQuat(this->offsetOrientation->getVector3()),
								Ogre::Vector3::UNIT_SCALE);

							// clonedGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->clone(this->spawnTargetGameObject->getId(), nullptr, 0,
							// 	this->offsetPosition->getVector3(), MathHelper::getInstance()->degreesToQuat(this->offsetOrientation->getVector3()), Ogre::Vector3::UNIT_SCALE);
							
							clonedGameObjectPtr->setVisible(true);
						}
						else
						{
							if (this->spawnTargetId->getULong() == this->gameObjectPtr->getId())
							{
								clonedGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->clone(this->spawnTargetGameObject->getId(), nullptr, 0,
									this->spawnTargetGameObject->getPosition() + (this->spawnTargetGameObject->getOrientation() * this->offsetPosition->getVector3()), 
									this->spawnTargetGameObject->getOrientation() * MathHelper::getInstance()->degreesToQuat(this->offsetOrientation->getVector3()), 
									Ogre::Vector3::UNIT_SCALE);
								clonedGameObjectPtr->setVisible(true);
							}
							else
							{
								// Special case, when the spawn target is different from this game object id, take the position and orientation of the 'This' game object
								// so that the spawn will be processed at the current position
								clonedGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->clone(this->spawnTargetGameObject->getId(), nullptr, 0,
									this->gameObjectPtr->getPosition() + (this->gameObjectPtr->getOrientation() * this->offsetPosition->getVector3()), 
									this->gameObjectPtr->getOrientation() * MathHelper::getInstance()->degreesToQuat(this->offsetOrientation->getVector3()), 
									Ogre::Vector3::UNIT_SCALE);
								clonedGameObjectPtr->setVisible(true);

							}
						}
						if (nullptr != clonedGameObjectPtr)
						{
							auto& physicsCompPtr = makeStrongPtr(clonedGameObjectPtr->getComponent<PhysicsComponent>());
							// Activate collision detection for physics, if game object with a physics component has been spawned
							if (nullptr != physicsCompPtr)
							{
								physicsCompPtr->setCollidable(true);
							}

							// Call connect manually for the cloned game object
							clonedGameObjectPtr->connect();

							// call callback function at the moment, a new game object will be spawned
							// http://stackoverflow.com/questions/9431297/luabind-cant-call-basic-lua-functions-like-print-tostring

							// if a script file is set
							if (nullptr != this->gameObjectPtr->getLuaScript())
							{
								// call the onSpawn callback function with the cloned game object on lua and run the script file
								if (this->spawnClosureFunction.is_valid())
								{
									try
									{
										luabind::call_function<void>(this->spawnClosureFunction, clonedGameObjectPtr.get(), this->gameObjectPtr.get());
									}
									catch (luabind::error& error)
									{
										luabind::object errorMsg(luabind::from_stack(error.state(), -1));
										std::stringstream msg;
										msg << errorMsg;

										Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnSpawn' Error: " + Ogre::String(error.what())
																					+ " details: " + msg.str());
									}
								}
							}
							else if (this->spawnObserver)
							{
								// else run the programed callback if it does exist
								this->spawnObserver->onSpawn(clonedGameObjectPtr.get(), this->gameObjectPtr.get());
							}
						
							// only add to life time queue if the spawned game object has a life time
							if (0 != this->lifeTime->getUInt())
							{
								// add the current time ms + life time in ms and push in a queue
								unsigned long currentTimeMS = Core::getSingletonPtr()->getOgreTimer()->getMilliseconds() + static_cast<unsigned long>(this->lifeTime->getUInt());

								// only push the cloned objects never the original one!
								// do not take ownership but give the raw pointer, because the RakNet queue some how does not release the ptr when calling pop, so the 
								// reference count remains the same! And the game object cannot be destroyed properly!

								// Do not emplace_back here since:
								// when emplace_back is done, internally move is done, so the clonedGameObjectPtr will become null, and the one in the queue's is correct and the reference count
								// will not be incremented
								// http://stackoverflow.com/questions/29643974/using-stdmove-with-stdshared-ptr
								// https://books.google.de/books?id=MmCWCwAAQBAJ&pg=PA535&lpg=PA535&dq=vector+emplace_back+a+shared_ptr&source=bl&ots=fS9F74AWJ_&sig=3AWF1FuG8_PJ1U34XjEtJSIbcZc&hl=de&sa=X&ved=0ahUKEwj_qqT38cXLAhUG9g4KHTlAC6wQ6AEITzAF#v=onepage&q=vector%20emplace_back%20a%20shared_ptr&f=false
								// But most critical, when clonedGameObjectPtr would hold a null address and those address is set for the lua script
								// especially in onVanish for script
								this->lifeTimeQueue.push_back(std::make_pair(currentTimeMS + this->lifeTime->getUInt(), clonedGameObjectPtr.get()));
							}
							else
							{
								// If the game object should life for ever put it in another list, to clear the list when disconnecting this component
								this->clonedGameObjectsInScene.emplace_back(clonedGameObjectPtr.get());
							}
						}
						else
						{
							Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[SpawnComponent] The game object: "
								+ this->gameObjectPtr->getName() + " could not be spawned because the clone operation delivered null!");
							throw Ogre::Exception(Ogre::Exception::ERR_INTERNAL_ERROR, "[SpawnComponent] The game object: "
								+ this->gameObjectPtr->getName() + " could not be spawned because the clone operation delivered null!", "NOWA");
						}
					}
				}
			}

			// 0 means the game object will never be deleted!
			if (0 != this->lifeTime->getUInt())
			{
				if (0 < this->lifeTimeQueue.size())
				{
					std::pair<unsigned long, GameObject*> clonedGameObjectPair = this->lifeTimeQueue.front();
					
					long dt = clonedGameObjectPair.first - Core::getSingletonPtr()->getOgreTimer()->getMilliseconds();
					// if the delta is positive, it has not enough time passed to delete the object
					if (dt > 0)
					{
						return;
					}
					// now is the right time
					this->lifeTimeQueue.pop_front();

					unsigned long id = clonedGameObjectPair.second->getId();
					if (nullptr != this->gameObjectPtr->getLuaScript())
					{
						if (this->vanishClosureFunction.is_valid())
						{
							try
							{
								luabind::call_function<void>(this->vanishClosureFunction, clonedGameObjectPair.second, this->gameObjectPtr.get());
							}
							catch (luabind::error& error)
							{
								luabind::object errorMsg(luabind::from_stack(error.state(), -1));
								std::stringstream msg;
								msg << errorMsg;

								Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnVanish' Error: " + Ogre::String(error.what())
																			+ " details: " + msg.str());
							}
						}
					}
					else if (this->spawnObserver)
					{
						this->spawnObserver->onVanish(clonedGameObjectPair.second, this->gameObjectPtr.get());
					}
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[SpawnComponent] Deleting gameobject: " + clonedGameObjectPair.second->getName() + " because it has no life time anymore");
					AppStateManager::getSingletonPtr()->getGameObjectController()->deleteGameObject(clonedGameObjectPair.second->getId());
				}
			}
		}
	}

	void SpawnComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (SpawnComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (SpawnComponent::AttrInterval() == attribute->getName())
		{
			this->setIntervalMS(attribute->getUInt());
		}
		else if (SpawnComponent::AttrCount() == attribute->getName())
		{
			this->setCount(attribute->getUInt());
		}
		else if (SpawnComponent::AttrLifeTime() == attribute->getName())
		{
			this->setLifeTimeMS(attribute->getUInt());
		}
		else if (SpawnComponent::AttrOffsetPosition() == attribute->getName())
		{
			this->setOffsetPosition(attribute->getVector3());
		}
		else if (SpawnComponent::AttrOffsetOrientation() == attribute->getName())
		{
			this->setOffsetOrientation(attribute->getVector3());
		}
		else if (SpawnComponent::AttrSpawnAtOrigin() == attribute->getName())
		{
			this->setSpawnAtOrigin(attribute->getBool());
		}
		else if (SpawnComponent::AttrSpawnTargetId() == attribute->getName())
		{
			this->setSpawnTargetId(attribute->getULong());
		}
	}

	void SpawnComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "SpawnActivate"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->activated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SpawnInterval"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->interval->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SpawnCount"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->count->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SpawnLifeTime"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->lifeTime->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SpawnOffsetPosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->offsetPosition->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SpawnOffsetOrientation"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->offsetOrientation->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SpawnAtOrigin"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->spawnAtOrigin->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SpawnTarget"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->spawnTargetId->getULong())));
		propertiesXML->append_node(propertyXML);
	}

	/*void SpawnComponent::setScriptFile(const Ogre::String& scriptFile)
	{
		this->scriptFile->setValue(scriptFile);
		
		if (false == scriptFile.empty())
		{
			if (nullptr != this->luaScript && scriptFile != this->luaScript->getName())
			{
				LuaScriptApi::getInstance()->copyScript(this->luaScript->getName(), scriptFile, true);
				LuaScriptApi::getInstance()->destroyScript(this->luaScript);
			}

			LuaScriptApi::getInstance()->destroyScript(this->luaScript);
			this->luaScript = LuaScriptApi::getInstance()->createScript(scriptFile + "_" + this->gameObjectPtr->getUniqueName(), scriptFile);
			this->luaScript->setInterfaceFunctionsTemplate("function onSpawn(spawnedGameObject, originGameObject)\n"
				"\t--local coinComponent = spawnedGameObject:getPhysicsActiveComponent();\n\t--if coinComponent ~= nil then"
				"\n\t\t-- add impulse to each coin so that it flies in the direction where the cannon is actually rotated(note, its configured, that the coin is automatically rotated as the cannon)"
				"\n\t\t--local direction = coinComponent:getOrientation():yAxis();\n\t\t--direction = coinComponent:getOrientation() * Vector3.UNIT_Y;"
				"\n\t\t--coinComponent:addImpulse(direction * 20);\n\t\t--coinComponent:setOrientation(Quaternion.IDENTITY);\n\t\t--play sound"
				"\n\t\t--local soundComponent = originGameObject:getSimpleSoundComponent();\n\t\t--soundComponent:setActivated(true);"
				"\n\t\t--give some recoil to the cannon\n\t\tlocal spawnerComponent = originGameObject:getPhysicsActiveComponent();\n\t\tspawnerComponent:addImpulse(direction * -50);"
				"\n\t\t--add a smoke particle effect\n\t\t--local particleComponent = originGameObject:getParticleUniverseComponent();"
				"\n\t\t--particleComponent:setActivated(true);\n\tend\nend\n"
				"function onVanish(spawnedGameObject, originGameObject)\n\n\tlogMessage(\"---->vanish: \" ..spawnedGameObject:getName());end");
			this->luaScript->setScriptFile(scriptFile);
		}
	}*/

	void SpawnComponent::reset(void)
	{
		this->spawnTimer = 0.0f;
		this->currentCount = 0;
		this->firstTimeSetSpawnTarget = true;

		// Delete the remaining game objects in queue;
		if (false == this->keepAlive)
		{
			for (auto& it = this->lifeTimeQueue.begin(); it != this->lifeTimeQueue.end();)
			{
				GameObject* gameObject = (*it).second;
				it = this->lifeTimeQueue.erase(it);
				AppStateManager::getSingletonPtr()->getGameObjectController()->deleteGameObject(gameObject->getId());
				gameObject = nullptr;
			}

			for (auto& it = this->clonedGameObjectsInScene.begin(); it != this->clonedGameObjectsInScene.end();)
			{
				GameObject* gameObject = *it;
				it = this->clonedGameObjectsInScene.erase(it);
				AppStateManager::getSingletonPtr()->getGameObjectController()->deleteGameObject(gameObject->getId());
				gameObject = nullptr;
			}
		}
	}

	void SpawnComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
		this->reset();
	}

	bool SpawnComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void SpawnComponent::setIntervalMS(unsigned int intervalMS)
	{
		this->interval->setValue(intervalMS);
		if (this->interval->getUInt() < 100)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[SpawnComponent] Warning: Spawn interval cannot not be less than 100 ms. Resetting to 100 ms.");
			this->interval->setValue(100);
		}
	}

	unsigned int SpawnComponent::getIntervalMS(void) const
	{
		return this->interval->getUInt();
	}

	void SpawnComponent::setCount(unsigned int count)
	{
		this->count->setValue(count);
		if (this->count->getUInt() < 0)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[SpawnComponent] Warning: Spawn count cannot not be negative. Resetting to 0.");
			this->count->setValue(0);
		}
	}

	unsigned int SpawnComponent::getCount(void) const
	{
		return this->count->getUInt();
	}

	void SpawnComponent::setLifeTimeMS(unsigned int lifeTimeMS)
	{
		this->lifeTime->setValue(lifeTimeMS);
		if (this->lifeTime->getUInt() < 100 && this->lifeTime->getUInt() > 0)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[SpawnComponent] Warning: Spawn life time cannot not be less than 100 ms. Resetting to 100 ms.");
			this->lifeTime->setValue(100);
		}
	}

	unsigned int SpawnComponent::getLifeTimeMS(void) const
	{
		return this->lifeTime->getUInt();
	}

	/*void SpawnComponent::setInitPosition(const Ogre::Vector3& initPosition)
	{
		this->initPosition = initPosition;
	}

	const Ogre::Vector3 SpawnComponent::getInitPosition(void) const
	{
		return this->initPosition;
	}

	void SpawnComponent::setInitOrientation(const Ogre::Quaternion& initOrientation)
	{
		this->initOrientation = initOrientation;
	}

	const Ogre::Quaternion SpawnComponent::getInitOrienation(void) const
	{
		return this->initOrientation;
	}*/

	void SpawnComponent::setOffsetPosition(const Ogre::Vector3& offsetPosition)
	{
		this->offsetPosition->setValue(offsetPosition);
	}

	const Ogre::Vector3 SpawnComponent::getOffsetPosition(void) const
	{
		return this->offsetPosition->getVector3();
	}

	void SpawnComponent::setOffsetOrientation(const Ogre::Vector3& offsetOrientation)
	{
		this->offsetOrientation->setValue(offsetOrientation);
	}

	Ogre::Vector3 SpawnComponent::getOffsetOrientation(void) const
	{
		return this->offsetOrientation->getVector3();
	}

	void SpawnComponent::setSpawnAtOrigin(bool spawnAtOrigin)
	{
		this->spawnAtOrigin->setValue(spawnAtOrigin);
	}

	bool SpawnComponent::isSpawnedAtOrigin(void) const
	{
		return this->spawnAtOrigin->getBool();
	}

	void SpawnComponent::setSpawnTargetId(unsigned long spawnTargetId)
	{
		this->spawnTargetId->setValue(spawnTargetId);

		// If no id is set, set to this game object
		if (0 == spawnTargetId)
		{
			if (this->gameObjectPtr->getId() != GameObjectController::MAIN_GAMEOBJECT_ID && this->gameObjectPtr->getId() != GameObjectController::MAIN_CAMERA_ID
				&& this->gameObjectPtr->getId() != GameObjectController::MAIN_LIGHT_ID)
			{
				this->spawnTargetId->setValue(this->gameObjectPtr->getId());
			}
		}

		this->firstTimeSetSpawnTarget = true;
	}

	unsigned long SpawnComponent::getSpawnTargetId(void) const
	{
		return this->spawnTargetId->getULong();
	}

	void SpawnComponent::setSpawnObserver(SpawnComponent::ISpawnObserver* spawnObserver)
	{
		this->spawnObserver = spawnObserver;
	}

	void SpawnComponent::setKeepAliveSpawnedGameObjects(bool keepAlive)
	{
		this->keepAlive = keepAlive;
	}

	void SpawnComponent::reactOnSpawn(luabind::object closureFunction)
	{
		this->spawnClosureFunction = closureFunction;
	}

	void SpawnComponent::reactOnVanish(luabind::object closureFunction)
	{
		this->vanishClosureFunction = closureFunction;
	}

}; // namespace end
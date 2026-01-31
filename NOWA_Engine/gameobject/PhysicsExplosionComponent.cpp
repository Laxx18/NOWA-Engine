#include "NOWAPrecompiled.h"
#include "PhysicsExplosionComponent.h"
#include "utilities/XMLConverter.h"
#include "PhysicsActiveComponent.h"
#include "PhysicsArtifactComponent.h"
#include "modules/LuaScriptApi.h"
#include "LuaScriptComponent.h"
#include "main/ProcessManager.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	class BlastProcess : public NOWA::Process
	{
	public:
		explicit BlastProcess(OgreNewt::Body* body, Ogre::Vector3 directionStrength)
			: body(body),
			directionStrength(directionStrength)
		{
			// Note: here a vector copy because this process is called at a later time
		}
	protected:
		virtual void onInit(void) override
		{
			this->succeed();
			// Add the impulse in the inverted direction
			// Attention: 1.0f / 60.0f
			body->addImpulse(directionStrength, body->getPosition(), 1.0f / 60.0f);
		}

		virtual void onUpdate(float dt) override
		{
			this->succeed();
		}
	private:
		OgreNewt::Body* body;
		Ogre::Vector3 directionStrength;
	};

	///////////////////////////////////////////////////////////////////////////////////////////////
	
	PhysicsExplosionComponent::PhysicsExplosionComponent()
		: GameObjectComponent(),
		categoryIds(GameObjectController::ALL_CATEGORIES_ID),
		countDownTimer(0.0f),
		secondsUpdateTimer(0.0f),
		explosionCallback(nullptr),
		activated(new Variant(PhysicsExplosionComponent::AttrActivated(), false, this->attributes)),
		categories(new Variant(PhysicsExplosionComponent::AttrCategories(), "All", this->attributes)),
		countDown(new Variant(PhysicsExplosionComponent::AttrCountDown(), 3.0f, this->attributes)),
		radius(new Variant(PhysicsExplosionComponent::AttrRadius(), 10.0f, this->attributes)),
		strength(new Variant(PhysicsExplosionComponent::AttrStrength(), static_cast<unsigned int>(20000), this->attributes))
	{
		this->countDown->setDescription("The count down in seconds. After that time the explosion will take place.");
		this->radius->setDescription("The radius in which all game objects are affected by the explosion.");
		this->strength->setDescription("The explosion strength in newton.");
		this->strength->setDescription("The affected categories. Categories can be combined, e.g. 'All-Player'.");
	}

	PhysicsExplosionComponent::~PhysicsExplosionComponent()
	{
		if (this->sphereSceneQuery)
		{
			this->gameObjectPtr->getSceneManager()->destroyQuery(this->sphereSceneQuery);
			this->sphereSceneQuery = nullptr;
		}
		if (this->explosionCallback)
		{
			delete this->explosionCallback;
			this->explosionCallback = nullptr;
		}

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsExplosionComponent] Destructor physics explosion component for game object: " + this->gameObjectPtr->getName());
	}

	bool PhysicsExplosionComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ExplosionActivate")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ExplosionAffectedCategories")
		{
			this->categories->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			this->categoryIds = AppStateManager::getSingletonPtr()->getGameObjectController()->generateCategoryId(this->categories->getString());
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ExplosionCountDown")
		{
			this->countDown->setValue(XMLConverter::getAttribReal(propertyElement, "data", 5));
			this->countDownTimer = this->countDown->getReal();
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ExplosionRadius")
		{
			this->radius->setValue(XMLConverter::getAttribReal(propertyElement, "data", 10));
			if (this->radius->getReal() < 1.0f)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[SpawnComponent] Warning: Explosion radius cannot be smaller as 1 meter. Resetting to 1.");
				this->radius->setValue(1.0f);
			}
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ExplosionStrength")
		{
			this->strength->setValue(static_cast<unsigned int>(XMLConverter::getAttribReal(propertyElement, "data", 20000)));
			propertyElement = propertyElement->next_sibling("property");
		}

		return true;
	}

	GameObjectCompPtr PhysicsExplosionComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		PhysicsExplosionCompPtr clonedCompPtr(boost::make_shared<PhysicsExplosionComponent>());

		
		clonedCompPtr->setAffectedCategories(this->categories->getString());
		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setExplosionCountDownSec(this->countDown->getReal());
		clonedCompPtr->setExplosionRadius(this->radius->getReal());
		clonedCompPtr->setExplosionStrengthN(this->strength->getUInt());
// Check that
		if (nullptr != this->explosionCallback)
		{
			clonedCompPtr->setExplosionCallback(this->explosionCallback->clone());
		}

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);
		
		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool PhysicsExplosionComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsExplosionComponent] Init physics explosion component for game object: " + this->gameObjectPtr->getName());
		
		this->sphereSceneQuery = this->gameObjectPtr->getSceneManager()->createSphereQuery(Ogre::Sphere(this->gameObjectPtr->getPosition(), this->radius->getReal()));
		this->sphereSceneQuery->setQueryMask(GameObjectController::ALL_CATEGORIES_ID);

		return true;
	}

	bool PhysicsExplosionComponent::connect(void)
	{
		// Reset timer
		this->countDownTimer = this->countDown->getReal();
		this->secondsUpdateTimer = 0.0f;
		
		return true;
	}

	bool PhysicsExplosionComponent::disconnect(void)
	{
		
		return true;
	}

	Ogre::String PhysicsExplosionComponent::getClassName(void) const
	{
		return "PhysicsExplosionComponent";
	}

	Ogre::String PhysicsExplosionComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void PhysicsExplosionComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (true == this->activated->getBool() && false == notSimulating)
		{
			if (this->countDownTimer > 0.0f)
			{
				LuaScript* luaScript = this->gameObjectPtr->getLuaScript();
				
				this->countDownTimer -= dt;
				this->secondsUpdateTimer += dt;
				// if one second passed, call callbacks to react
				if (this->secondsUpdateTimer >= 1.0f)
				{
					// if a script file is set
					if (nullptr != luaScript)
					{
						NOWA::AppStateManager::LogicCommand logicCommand = [this, luaScript]()
						{
							luaScript->callTableFunction("onTimerSecondTick", this->gameObjectPtr.get());
						};
						NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
					}
					else if (this->explosionCallback)
					{
						// else run the programmed callback if it does exist
						// here with the list from sphere scene query
						this->explosionCallback->onTimerSecondTick(this->gameObjectPtr.get());
					}
					// Reset timer
					this->secondsUpdateTimer = 0.0f;
				}
				// If the timer is bigger as the count down, BUM......!
				if (this->countDownTimer <= 0.0f)
				{
					Ogre::Sphere updateSphere(this->gameObjectPtr->getPosition(), this->radius->getReal());
					this->sphereSceneQuery->setSphere(updateSphere);
					if (this->categoryIds > 0)
					{
						this->sphereSceneQuery->setQueryMask(this->categoryIds);
					}

					// check objects in range
					Ogre::SceneQueryResultMovableList& result = this->sphereSceneQuery->execute().movables;
					for (auto& it = result.cbegin(); it != result.cend(); ++it)
					{
						Ogre::MovableObject* movableObject = *it;

						const Ogre::Any& any = movableObject->getUserObjectBindings().getUserAny();
						if (false == any.isEmpty())
						{
							NOWA::GameObject* affectedGameObject = Ogre::any_cast<NOWA::GameObject*>(any);
							// Remember this bomb itsel will also be affected, if it has not been excluded via category
							if (affectedGameObject && affectedGameObject != this->gameObjectPtr.get())
							{
								this->affectedGameObjects.emplace_back(affectedGameObject);

								Ogre::Vector3& direction = affectedGameObject->getPosition() - this->gameObjectPtr->getPosition();
								Ogre::Real distanceToBomb = direction.length();
								// Increase the height too
								if (direction.y < 0.0f)
								{
									direction.y *= -1.0f;
								}

								if (distanceToBomb < 1.0f)
								{
									distanceToBomb = 1.0f;
								}
								direction.normalise();
								direction.y *= 5.0f;

								// Calculate the detonation strength by using the strengh in relation to the distance to the game object
								unsigned int detonationStrength = this->strength->getReal() / static_cast<unsigned int>(distanceToBomb);
								if (0 == detonationStrength)
								{
									detonationStrength = 10;
								}
								/*Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "distanceToBomb: " + Ogre::StringConverter::toString(distanceToBomb) + " strength: "
									+ Ogre::StringConverter::toString(detonationStrength) + " direction: " + Ogre::StringConverter::toString(direction)
									+ " directionJump: " + Ogre::StringConverter::toString(direction * -1.0f * static_cast<Ogre::Real>(detonationStrength)));*/
									// here because of parent id introduction, its possible to get the base component and set data, instead of setting for all available derived components from PhysicsComponent
								auto& physicsCompPtr = makeStrongPtr(affectedGameObject->getComponent<PhysicsComponent>());
								if (physicsCompPtr)
								{
									// Blast the affected game object away, but by a delay, depending on the distance to the bomb
									// E.g. 20 meters away from bomb would delay 1 second
									NOWA::ProcessPtr delayProcess(new NOWA::DelayProcess(distanceToBomb / 20.0f));
									delayProcess->attachChild(NOWA::ProcessPtr(new BlastProcess(physicsCompPtr->getBody(), direction * static_cast<Ogre::Real>(detonationStrength))));
									NOWA::ProcessManager::getInstance()->attachProcess(delayProcess);
								}
								//else
								//{
								//	// If there is no physics component, use the scene node
								//	affectedGameObject->getSceneNode()->translate(direction * -1.0f * static_cast<Ogre::Real>(detonationStrength));
								//}

								// if a script file is set
								if (nullptr != luaScript)
								{
									
									NOWA::AppStateManager::LogicCommand logicCommand = [this, luaScript, affectedGameObject, distanceToBomb, detonationStrength]()
									{
										luaScript->callTableFunction("onExplodeAffectedGameObject", this->gameObjectPtr.get(), affectedGameObject, distanceToBomb, detonationStrength);
									};
									NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
								}
								else if (this->explosionCallback)
								{
									// else run the programmed callback if it does exist
									// This callback is called x-times for each affected game object
									this->explosionCallback->onExplodeAffectedGameObject(this->gameObjectPtr.get(), affectedGameObject, distanceToBomb, detonationStrength);
								}
							}
						}
					}

					if (nullptr != luaScript)
					{
						// call the onExplode callback function with the cloned game object on lua and run the script file
						NOWA::AppStateManager::LogicCommand logicCommand = [this, luaScript]()
						{
							luaScript->callTableFunction("onExplode", this->gameObjectPtr.get());
						};
						NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
					}
					else if (this->explosionCallback)
					{
						// else run the programmed callback if it does exist
						// This callback is called x-times for each affected game object
						this->explosionCallback->onExplode(this->gameObjectPtr.get());
					}

					// this->activated->setValue(false);
					// Clear the affected game objects
					this->affectedGameObjects.clear();
					// Reset timer
					this->countDownTimer = 0.0f;
				}
			}
		}
	}

	void PhysicsExplosionComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (PhysicsExplosionComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (PhysicsExplosionComponent::AttrCategories() == attribute->getName())
		{
			this->setAffectedCategories(attribute->getString());
		}
		else if (PhysicsExplosionComponent::AttrCountDown() == attribute->getName())
		{
			this->setExplosionCountDownSec(attribute->getReal());
		}
		else if (PhysicsExplosionComponent::AttrRadius() == attribute->getName())
		{
			this->setExplosionRadius(attribute->getReal());
		}
		else if (PhysicsExplosionComponent::AttrStrength() == attribute->getName())
		{
			this->setExplosionStrengthN(attribute->getUInt());
		}
	}

	void PhysicsExplosionComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "ExplosionActivate"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ExplosionAffectedCategories"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->categories->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ExplosionCountDown"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->countDown->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ExplosionRadius"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->radius->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ExplosionStrength"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->strength->getUInt())));
		propertiesXML->append_node(propertyXML);
	}

	void PhysicsExplosionComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
		// Reset timer
		this->countDownTimer = this->countDown->getReal();
		this->secondsUpdateTimer = 0.0f;
		// this->reset();
	}

	bool PhysicsExplosionComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void PhysicsExplosionComponent::setExplosionCountDownSec(Ogre::Real countDown)
	{
		this->countDown->setValue(countDown);
		// Reset timer
		this->countDownTimer = countDown;
		this->secondsUpdateTimer = 0.0f;
	}

	Ogre::Real PhysicsExplosionComponent::getExplosionCountDownSec(void) const
	{
		return this->countDown->getReal();
	}

	void PhysicsExplosionComponent::setExplosionRadius(Ogre::Real radius)
	{
		this->radius->setValue(radius);
	}

	Ogre::Real PhysicsExplosionComponent::getExplosionRadius(void) const
	{
		return this->radius->getReal();
	}

	void PhysicsExplosionComponent::setExplosionStrengthN(unsigned int strength)
	{
		this->strength->setValue(strength);
	}

	unsigned int PhysicsExplosionComponent::getExplosionStrengthN(void) const
	{
		return this->strength->getUInt();
	}

	void PhysicsExplosionComponent::setExplosionCallback(IExplosionCallback* explosionCallback)
	{
		this->explosionCallback = explosionCallback;
	}

	/*void PhysicsExplosionComponent::setScriptFile(const Ogre::String& scriptFile)
	{
		this->scriptFile->setValue(scriptFile);

		if (false == scriptFile.empty())
		{
			if (nullptr != this->luaScript && this->luaScript->isCompiled() && scriptFile != this->luaScript->getName())
			{
				LuaScriptApi::getInstance()->copyScript(this->luaScript->getName(), scriptFile, true);
				LuaScriptApi::getInstance()->destroyScript(this->luaScript);
			}

			LuaScriptApi::getInstance()->destroyScript(this->luaScript);
			this->luaScript = LuaScriptApi::getInstance()->createScript(scriptFile + "_" + this->gameObjectPtr->getUniqueName(), scriptFile);
			this->luaScript->setInterfaceFunctionsTemplate("--function onTimerSecondTick(originGameObject)\n--local soundComponent = originGameObject:getSimpleSoundComponent();"
				"\n\t--soundComponent:setActivated(true);--soundComponent:connect();\n--end\n\n"
				"--function onExplode(originGameObject)\n\t--local soundComponent = originGameObject:getSimpleSoundComponent();\n\t--soundComponent:setActivated(true);"
				"\n\t--soundComponent:connect();\n\t--local particleComponent = originGameObject:getParticleFxComponent();\n\t--particleComponent:setActivated(true);"
				"\n\t--originGameObject:getSceneNode():setVisible(false, true);\n\t--logMessage([Lua]: gameobject translate: \" ..originGameObject:getName());"
				"\n\t--originGameObject:getPhysicsActiveComponent() : translate(Vector3(2, 0, 0));\n\t--GameObjectController:deleteGameObject(originGameObject : getId());\n--end\n\n"
				"--function onExplodeAffectedGameObject(originGameObject, affectedGameObject, distanceToBomb, detonationStrength)\n\t--affectedGameObject:getSceneNode():setVisible(false, true);\n--end");
			this->luaScript->setScriptFile(scriptFile);
		}
	}*/

	void PhysicsExplosionComponent::setAffectedCategories(const Ogre::String& categories)
	{
		this->categories->setValue(categories);
		this->categoryIds = AppStateManager::getSingletonPtr()->getGameObjectController()->generateCategoryId(this->categories->getString());
	}

	Ogre::String PhysicsExplosionComponent::getAffectedCategories(void)
	{
		return this->categories->getString();
	}

}; // namespace end
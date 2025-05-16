#include "NOWAPrecompiled.h"
#include "FollowTargetComponent.h"
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

	FollowTargetComponent::FollowTargetComponent()
		: GameObjectComponent(),
		name("FollowTargetComponent"),
		currentDegree(0.0f),
		currentRandomHeight(0.0f),
		targetGameObject(nullptr),
		physicsActiveComponent(nullptr),
		physicsActiveKinematicComponent(nullptr),
		originPosition(Ogre::Vector3::ZERO),
		originOrientation(Ogre::Quaternion::IDENTITY),
		oldPosition(Ogre::Vector3::ZERO),
		oldOrientation(Ogre::Quaternion::IDENTITY),
		firstTime(true),
		activated(new Variant(FollowTargetComponent::AttrActivated(), true, this->attributes)),
		targetId(new Variant(FollowTargetComponent::AttrTargetId(), static_cast<unsigned long>(0), this->attributes, true)),
		offsetAngle(new Variant(FollowTargetComponent::AttrOffsetAngle(), 20.0f, this->attributes)),
		yOffset(new Variant(FollowTargetComponent::AttrYOffset(), 0.5f, this->attributes)),
		springLength(new Variant(FollowTargetComponent::AttrSpringLength(), 1.0f, this->attributes)),
		springForce(new Variant(FollowTargetComponent::AttrSpringForce(), 0.1f, this->attributes)),
		friction(new Variant(FollowTargetComponent::AttrFriction(), 0.5f, this->attributes)),
		maxRandomHeight(new Variant(FollowTargetComponent::AttrMaxRandomHeight(), 3.0f, this->attributes))
	{
		this->activated->setDescription("If activated, the follow behavior will take place.");
		this->targetId->setDescription("The target game object id to follow.");
		this->offsetAngle->setDescription("Sets the offset angle in degrees, this little helper will be placed at. Positive angle means, the little helper will be placed at the left side of the target game object.");
		this->yOffset->setDescription("The y-offset height of the little helper.");
		this->springLength->setDescription("The newton spring length for movement adaptation.");
		this->springForce->setDescription("The newton spring force for movement adaptation.");
		this->friction->setDescription("The newton friction for movement adaptation.");
		this->maxRandomHeight->setDescription("The max random height, at which this helper fly oscilate during the fly.");
	}

	FollowTargetComponent::~FollowTargetComponent(void)
	{
		
	}

	const Ogre::String& FollowTargetComponent::getName() const
	{
		return this->name;
	}

	void FollowTargetComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<FollowTargetComponent>(FollowTargetComponent::getStaticClassId(), FollowTargetComponent::getStaticClassName());
	}
	
	void FollowTargetComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	bool FollowTargetComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &FollowTargetComponent::handleGameObjectDeleted), EventDataDeleteGameObject::getStaticEventType());

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == FollowTargetComponent::AttrTargetId())
		{
			this->targetId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == FollowTargetComponent::AttrOffsetAngle())
		{
			this->offsetAngle->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == FollowTargetComponent::AttrYOffset())
		{
			this->yOffset->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == FollowTargetComponent::AttrSpringLength())
		{
			this->springLength->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == FollowTargetComponent::AttrSpringForce())
		{
			this->springForce->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == FollowTargetComponent::AttrFriction())
		{
			this->friction->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == FollowTargetComponent::AttrMaxRandomHeight())
		{
			this->maxRandomHeight->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr FollowTargetComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		FollowTargetComponentPtr clonedCompPtr(boost::make_shared<FollowTargetComponent>());

		clonedCompPtr->setActivated(this->activated->getBool());
		
		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->setTargetId(this->targetId->getULong());
		clonedCompPtr->setOffsetAngle(this->offsetAngle->getReal());
		clonedCompPtr->setYOffset(this->yOffset->getReal());
		clonedCompPtr->setSpringLength(this->springLength->getReal());
		clonedCompPtr->setSpringForce(this->springForce->getReal());
		clonedCompPtr->setFriction(this->friction->getReal());
		clonedCompPtr->setMaxRandomHeight(this->maxRandomHeight->getReal());

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool FollowTargetComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[FollowTargetComponent] Init component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool FollowTargetComponent::connect(void)
	{
		GameObjectComponent::connect();
		
		const auto& targetGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->targetId->getULong());
		if (nullptr != targetGameObjectPtr)
		{
			this->targetGameObject = targetGameObjectPtr.get();
		}

		// Note: Special type must come first, because its a derived component from PhysicsActiveComponent
		auto physicsActiveKinematicCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsActiveKinematicComponent>());
		if (nullptr != physicsActiveKinematicCompPtr)
		{
			this->physicsActiveKinematicComponent = physicsActiveKinematicCompPtr.get();
		}
		else
		{
			auto physicsActiveCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsActiveComponent>());
			if (nullptr != physicsActiveCompPtr)
			{
				this->physicsActiveComponent = physicsActiveCompPtr.get();
			}
		}

		this->originPosition = this->gameObjectPtr->getPosition();
		this->originOrientation = this->gameObjectPtr->getOrientation();
		this->oldPosition = this->gameObjectPtr->getPosition();
		this->oldOrientation = this->gameObjectPtr->getOrientation();

		if (nullptr != this->targetGameObject)
		{
			if (nullptr == this->physicsActiveComponent && nullptr == this->physicsActiveKinematicComponent)
			{
				this->gameObjectPtr->getSceneNode()->setPosition(this->targetGameObject->getPosition());
			}
		}

		return true;
	}

	void FollowTargetComponent::reset(void)
	{
		if (nullptr != this->physicsActiveComponent)
		{
			this->physicsActiveComponent->setPosition(this->originPosition);
			this->physicsActiveComponent->setOrientation(this->originOrientation);
		}
		else if (nullptr != physicsActiveKinematicComponent)
		{
			this->physicsActiveKinematicComponent->setPosition(this->originPosition);
			this->physicsActiveKinematicComponent->setOrientation(this->originOrientation);
		}
		else
		{
			this->gameObjectPtr->getSceneNode()->setPosition(this->originPosition);
			this->gameObjectPtr->getSceneNode()->setOrientation(this->originOrientation);
		}

		this->targetGameObject = nullptr;
		this->physicsActiveComponent = nullptr;
		this->physicsActiveKinematicComponent = nullptr;
		this->currentDegree = 0.0f;
		this->currentRandomHeight = 0.0f;
		this->originPosition = Ogre::Vector3::ZERO;
		this->originOrientation = Ogre::Quaternion::IDENTITY;
		this->oldPosition = Ogre::Vector3::ZERO;
		this->oldOrientation = Ogre::Quaternion::IDENTITY;
		this->firstTime = true;
	}

	bool FollowTargetComponent::disconnect(void)
	{
		GameObjectComponent::disconnect();

		this->reset();

		return true;
	}

	bool FollowTargetComponent::onCloned(void)
	{
		// Search for the prior id of the cloned game object and set the new id and set the new id, if not found set better 0, else the game objects may be corrupt!
		GameObjectPtr targetGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getClonedGameObjectFromPriorId(this->targetId->getULong());
		if (nullptr != targetGameObjectPtr)
			this->setTargetId(targetGameObjectPtr->getId());
		else
			this->setTargetId(0);
		// Since connect is called during cloning process, it does not make sense to process furher here, but only when simulation started!
		return true;
	}

	void FollowTargetComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &FollowTargetComponent::handleGameObjectDeleted), EventDataDeleteGameObject::getStaticEventType());
	}

	void FollowTargetComponent::handleGameObjectDeleted(EventDataPtr eventData)
	{
		boost::shared_ptr<EventDataDeleteGameObject> castEventData = boost::static_pointer_cast<NOWA::EventDataDeleteGameObject>(eventData);
		// if a game object has been deleted elsewhere remove it from the queue, in order not to work with dangling pointers
		unsigned long id = castEventData->getGameObjectId();
		if (nullptr != this->targetGameObject)
		{
			if (this->targetGameObject->getId() == id)
			{
				this->disconnect();

			}
		}
	}
	
	void FollowTargetComponent::onOtherComponentRemoved(unsigned int index)
	{
		// TODO: What if the target game object is destroyed?

		auto& gameObjectCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponentByIndex(index));
		if (nullptr != gameObjectCompPtr)
		{
			auto& physicsActiveKinematicCompPtr = boost::dynamic_pointer_cast<PhysicsActiveKinematicComponent>(gameObjectCompPtr);
			if (nullptr != physicsActiveKinematicCompPtr)
			{
				this->physicsActiveKinematicComponent = nullptr;
			}
			else
			{
				auto& physicsActiveCompPtr = boost::dynamic_pointer_cast<PhysicsActiveComponent>(gameObjectCompPtr);
				if (nullptr != physicsActiveCompPtr)
				{
					this->physicsActiveComponent = nullptr;
				}
			}
		}
	}
	
	void FollowTargetComponent::onOtherComponentAdded(unsigned int index)
	{
		
	}
	
	void FollowTargetComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating && true == this->activated->getBool() && nullptr != this->targetGameObject)
		{
			Ogre::Vector3 playerPosition = Ogre::Vector3(this->targetGameObject->getPosition().x, this->targetGameObject->getPosition().y + 0.4f, this->targetGameObject->getPosition().z);
			Ogre::Quaternion playerOrientation = this->targetGameObject->getOrientation() * Ogre::Quaternion(Ogre::Degree(this->offsetAngle->getReal()), Ogre::Vector3::UNIT_Y);

			// HooksLaw
			Ogre::Vector3 direction = this->gameObjectPtr->getPosition() - playerPosition;
			direction.y = 0; // Ignore height differences to prevent tilting

			Ogre::Radian angle = Ogre::Math::ATan2(direction.z, direction.x);
			Ogre::Vector3 targetDirection = Ogre::Vector3((playerPosition.x + (Ogre::Math::Cos(angle) * this->springLength->getReal())), 0.0f, (playerPosition.z + (Ogre::Math::Sin(angle) * this->springLength->getReal())));
			Ogre::Vector3 velocityVector = Ogre::Vector3((targetDirection.x - this->gameObjectPtr->getPosition().x) * this->springForce->getReal(), 0.0f, (targetDirection.z - this->gameObjectPtr->getPosition().z) * this->springForce->getReal());

			Ogre::Vector3 defaultDirection = this->gameObjectPtr->getDefaultDirection() * -1.0f;
			Ogre::Quaternion targetOrientation = defaultDirection.getRotationTo(direction);
			
			velocityVector *= this->friction->getReal();

			// GoToBack
			Ogre::Vector3 tVector = Ogre::Vector3(playerOrientation * this->targetGameObject->getDefaultDirection() * -1.0f);
			tVector *= springLength->getReal() * Ogre::Vector3(1.0f, 0.0f, 1.0f);
			tVector += playerPosition * Ogre::Vector3(1.0f, 0.0f, 1.0f);
			tVector.y += this->yOffset->getReal();

			Ogre::Vector3 vVector = Ogre::Vector3(tVector - this->gameObjectPtr->getPosition()) * this->springForce->getReal() * dt * 60.0f;

			vVector *= this->friction->getReal();
			vVector *= 0.4f * Ogre::Vector3(1.0f, 0.0f, 1.0f);

			// Sinus oscillation of the helper
			if (this->currentDegree >= 360.0f)
			{
				this->currentRandomHeight = Ogre::Math::RangeRandom(1.0f, this->maxRandomHeight->getReal());
				this->currentDegree = 0.0f;
			}

			this->currentDegree += dt * 60.0f * (1.0f + (1.0f - (this->currentRandomHeight / 3.0f)));

			Ogre::Real rad = this->currentDegree * (Ogre::Math::PI / 180.0f);

			Ogre::Vector3 positionVector = (this->gameObjectPtr->getPosition() + velocityVector + vVector) * Ogre::Vector3(1.0f, 0.0f, 1.0f);
			positionVector.y = (playerPosition.y + (Ogre::Math::Sin(rad) * this->currentRandomHeight * 0.1f));

			if (nullptr == this->physicsActiveComponent && nullptr == this->physicsActiveKinematicComponent)
			{
				if (this->firstTime)
				{
					// Directly position the object behind the target.
					Ogre::Vector3 tVector = playerPosition - (playerOrientation * this->targetGameObject->getDefaultDirection() * this->springLength->getReal());
					tVector.y += this->yOffset->getReal();
					NOWA::RenderCommandQueueModule::getInstance()->updateNodeTransform(this->gameObjectPtr->getSceneNode(), tVector, targetOrientation);
					this->firstTime = false;
				}
				else
				{
					NOWA::RenderCommandQueueModule::getInstance()->updateNodeTransform(this->gameObjectPtr->getSceneNode(), positionVector, targetOrientation);
				}
			}
			else if (nullptr != this->physicsActiveComponent)
			{
				Ogre::Vector3 desiredVelocity = (positionVector - this->oldPosition) / dt;
				this->physicsActiveComponent->applyRequiredForceForVelocity(desiredVelocity);
				Ogre::Quaternion desiredOmega = targetOrientation;

				this->physicsActiveComponent->applyOmegaForceRotateTo(desiredOmega, Ogre::Vector3::UNIT_Y);
			}
			else if (nullptr != this->physicsActiveKinematicComponent)
			{
				Ogre::Vector3 desiredVelocity = (positionVector - this->oldPosition) / dt;
				this->physicsActiveKinematicComponent->setVelocity(desiredVelocity);
				Ogre::Quaternion desiredOmega = targetOrientation;
				this->physicsActiveKinematicComponent->setOmegaVelocityRotateTo(desiredOmega, Ogre::Vector3::UNIT_Y);
			}

			this->oldPosition = positionVector;
			this->oldOrientation = targetOrientation;
		}
	}

	void FollowTargetComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (FollowTargetComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (FollowTargetComponent::AttrTargetId() == attribute->getName())
		{
			this->setTargetId(attribute->getULong());
		}
		else if (FollowTargetComponent::AttrOffsetAngle() == attribute->getName())
		{
			this->setOffsetAngle(attribute->getReal());
		}
		else if (FollowTargetComponent::AttrYOffset() == attribute->getName())
		{
			this->setYOffset(attribute->getReal());
		}
		else if (FollowTargetComponent::AttrSpringLength() == attribute->getName())
		{
			this->setSpringLength(attribute->getReal());
		}
		else if (FollowTargetComponent::AttrSpringForce() == attribute->getName())
		{
			this->setSpringForce(attribute->getReal());
		}
		else if (FollowTargetComponent::AttrFriction() == attribute->getName())
		{
			this->setFriction(attribute->getReal());
		}
		else if (FollowTargetComponent::AttrMaxRandomHeight() == attribute->getName())
		{
			this->setMaxRandomHeight(attribute->getReal());
		}
	}

	void FollowTargetComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
		propertiesXML->append_node(propertyXML);	

		/*propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ShortTimeActivation"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->shortTimeActivation->getBool())));
		propertiesXML->append_node(propertyXML);*/

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TargetId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->targetId->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OffsetAngle"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->offsetAngle->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "YOffset"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->yOffset->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SpringLength"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->springLength->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SpringForce"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->springForce->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Friction"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->friction->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MaxRandomHeight"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxRandomHeight->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String FollowTargetComponent::getClassName(void) const
	{
		return "FollowTargetComponent";
	}

	Ogre::String FollowTargetComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void FollowTargetComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
	}

	bool FollowTargetComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void FollowTargetComponent::setTargetId(unsigned long targetId)
	{
		this->targetId->setValue(targetId);
	}

	unsigned long FollowTargetComponent::getTargetId(void) const
	{
		return this->targetId->getULong();
	}

	void FollowTargetComponent::setOffsetAngle(Ogre::Real offsetAngle)
	{
		this->offsetAngle->setValue(offsetAngle);
	}

	Ogre::Real FollowTargetComponent::getOffsetAngle(void) const
	{
		return this->offsetAngle->getReal();
	}

	void FollowTargetComponent::setYOffset(Ogre::Real yOffset)
	{
		this->yOffset->setValue(yOffset);
	}

	Ogre::Real FollowTargetComponent::getYOffset(void) const
	{
		return this->yOffset->getReal();
	}

	void FollowTargetComponent::setSpringLength(Ogre::Real springLength)
	{
		this->springLength->setValue(springLength);
	}

	Ogre::Real FollowTargetComponent::getSpringLength(void) const
	{
		return this->springLength->getReal();
	}

	void FollowTargetComponent::setSpringForce(Ogre::Real springForce)
	{
		this->springForce->setValue(springForce);
	}

	Ogre::Real FollowTargetComponent::getSpringForce(void) const
	{
		return this->springForce->getReal();
	}

	void FollowTargetComponent::setFriction(Ogre::Real friction)
	{
		this->friction->setValue(friction);
	}

	Ogre::Real FollowTargetComponent::getFriction(void) const
	{
		return this->friction->getReal();
	}

	void FollowTargetComponent::setMaxRandomHeight(Ogre::Real maxRandomHeight)
	{
		this->maxRandomHeight->setValue(maxRandomHeight);
	}

	Ogre::Real FollowTargetComponent::getMaxRandomHeight(void) const
	{
		return this->maxRandomHeight->getReal();
	}

	// Lua registration part

	FollowTargetComponent* getFollowTargetComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<FollowTargetComponent>(gameObject->getComponentWithOccurrence<FollowTargetComponent>(occurrenceIndex)).get();
	}

	FollowTargetComponent* getFollowTargetComponent(GameObject* gameObject)
	{
		return makeStrongPtr<FollowTargetComponent>(gameObject->getComponent<FollowTargetComponent>()).get();
	}

	FollowTargetComponent* getFollowTargetComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<FollowTargetComponent>(gameObject->getComponentFromName<FollowTargetComponent>(name)).get();
	}

	void FollowTargetComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<FollowTargetComponent, GameObjectComponent>("FollowTargetComponent")
			.def("setActivated", &FollowTargetComponent::setActivated)
			.def("isActivated", &FollowTargetComponent::isActivated)
			.def("setTargetId", &FollowTargetComponent::setTargetId)
			.def("getTargetId", &FollowTargetComponent::getTargetId)
			.def("setOffsetAngle", &FollowTargetComponent::setOffsetAngle)
			.def("getOffsetAngle", &FollowTargetComponent::getOffsetAngle)
			.def("setYOffset", &FollowTargetComponent::setYOffset)
			.def("getYOffset", &FollowTargetComponent::getYOffset)
			.def("setSpringLength", &FollowTargetComponent::setSpringLength)
			.def("getSpringLength", &FollowTargetComponent::getSpringLength)
			.def("setSpringForce", &FollowTargetComponent::setSpringForce)
			.def("getSpringForce", &FollowTargetComponent::getSpringForce)
			.def("setFriction", &FollowTargetComponent::setFriction)
			.def("getFriction", &FollowTargetComponent::getFriction)
			.def("setMaxRandomHeight", &FollowTargetComponent::setMaxRandomHeight)
			.def("getMaxRandomHeight", &FollowTargetComponent::getMaxRandomHeight)
		];

		LuaScriptApi::getInstance()->addClassToCollection("FollowTargetComponent", "class inherits GameObjectComponent", FollowTargetComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("FollowTargetComponent", "GameObject getOwner()", "Gets the owner game object.");
		LuaScriptApi::getInstance()->addClassToCollection("FollowTargetComponent", "void setActivated(bool activated)", "If activated, the follow behavior will take place.");
		LuaScriptApi::getInstance()->addClassToCollection("FollowTargetComponent", "bool isActivated()", "Gets whether this component is activated.");
		LuaScriptApi::getInstance()->addClassToCollection("FollowTargetComponent", "void setTargetId(string targetId)", "Sets the target game object id to follow.");
		LuaScriptApi::getInstance()->addClassToCollection("FollowTargetComponent", "string getTargetId()", "Gets the target game object id to follow, which this little helper does follow.");
		LuaScriptApi::getInstance()->addClassToCollection("FollowTargetComponent", "void setOffsetAngle(number offsetAngle)", "Sets the offset angle in degrees, this little helper will be placed at. Positive angle means, the little helper will be placed at the left side of the target game object.");
		LuaScriptApi::getInstance()->addClassToCollection("FollowTargetComponent", "number getOffsetAngle()", "Gets the offset angle in degrees, this little helper is be placed at. Positive angle means, the little helper will be placed at the left side of the target game object.");
		LuaScriptApi::getInstance()->addClassToCollection("FollowTargetComponent", "void setYOffset(number yOffset)", "Sets the y-offset height of the little helper.");
		LuaScriptApi::getInstance()->addClassToCollection("FollowTargetComponent", "number getYOffset()", "Gets the y-offset height of the little helper.");
		LuaScriptApi::getInstance()->addClassToCollection("FollowTargetComponent", "void setSpringLength(number springLength)", "Sets the newton spring length for movement adaptation.");
		LuaScriptApi::getInstance()->addClassToCollection("FollowTargetComponent", "number getSpringLength()", "Gets the newton spring length for movement adaptation.");
		LuaScriptApi::getInstance()->addClassToCollection("FollowTargetComponent", "void setSpringForce(number springForce)", "Sets the newton spring force for movement adaptation.");
		LuaScriptApi::getInstance()->addClassToCollection("FollowTargetComponent", "number getSpringForce()", "Gets the newton spring force for movement adaptation.");
		LuaScriptApi::getInstance()->addClassToCollection("FollowTargetComponent", "void setFriction(number friction)", "Sets the newton friction for movement adaptation.");
		LuaScriptApi::getInstance()->addClassToCollection("FollowTargetComponent", "number getFriction()", "Gets the newton friction for movement adaptation.");
		LuaScriptApi::getInstance()->addClassToCollection("FollowTargetComponent", "void setMaxRandomHeight(number maxRandomHeight)", "Sets the max random height, at which this helper fly oscilate during the fly.");
		LuaScriptApi::getInstance()->addClassToCollection("FollowTargetComponent", "number getMaxRandomHeight()", "Gets the max random height, at which this helper fly oscilate during the fly.");

		gameObjectClass.def("getFollowTargetComponentFromName", &getFollowTargetComponentFromName);
		gameObjectClass.def("getFollowTargetComponent", (FollowTargetComponent * (*)(GameObject*)) & getFollowTargetComponent);
		// If its desired to create several of this components for one game object
		gameObjectClass.def("getFollowTargetComponentFromIndex", (FollowTargetComponent * (*)(GameObject*, unsigned int)) & getFollowTargetComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "FollowTargetComponent getFollowTargetComponentFromIndex(unsigned int occurrenceIndex)", "Gets the component by the given occurence index, since a game object may this component maybe several times.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "FollowTargetComponent getFollowTargetComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "FollowTargetComponent getFollowTargetComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castFollowTargetComponent", &GameObjectController::cast<FollowTargetComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "FollowTargetComponent castFollowTargetComponent(FollowTargetComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool FollowTargetComponent::canStaticAddComponent(GameObject* gameObject)
	{
		// Can only be added once
		if (gameObject->getComponentCount<FollowTargetComponent>() < 2)
		{
			return true;
		}
	}

}; //namespace end
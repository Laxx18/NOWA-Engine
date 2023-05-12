#include "NOWAPrecompiled.h"
#include "JointComponents.h"
#include "modules/OgreNewtModule.h"
#include "modules/LuaScriptApi.h"
#include "PhysicsActiveComponent.h"
#include "utilities/XMLConverter.h"
#include "main/Events.h"
#include "utilities/MathHelper.h"
#include "NodeComponent.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	typedef boost::shared_ptr<JointComponent> JointCompPtr;
	typedef boost::shared_ptr<JointHingeComponent> JointHingeCompPtr;
	typedef boost::shared_ptr<JointHingeActuatorComponent> JointHingeActuatorCompPtr;
	typedef boost::shared_ptr<JointBallAndSocketComponent> JointBallAndSocketCompPtr;
	typedef boost::shared_ptr<JointPointToPointComponent> JointPointToPointCompPtr;
	// typedef boost::shared_ptr<JointControlledBallAndSocketComponent> JointControlledBallAndSocketCompPtr;
	// typedef boost::shared_ptr<RagDollMotorDofComponent> RagDollMotorDofCompPtr;
	typedef boost::shared_ptr<JointPinComponent> JointPinCompPtr;
	typedef boost::shared_ptr<JointPlaneComponent> JointPlaneCompPtr;
	typedef boost::shared_ptr<JointCorkScrewComponent> JointCorkScrewCompPtr;
	typedef boost::shared_ptr<JointPassiveSliderComponent> JointPassiveSliderCompPtr;
	typedef boost::shared_ptr<JointSliderActuatorComponent> JointSliderActuatorCompPtr;
	typedef boost::shared_ptr<JointActiveSliderComponent> JointActiveSliderCompPtr;
	typedef boost::shared_ptr<JointMathSliderComponent> JointMathSliderCompPtr;
	typedef boost::shared_ptr<JointKinematicComponent> JointKinematicCompPtr;
	typedef boost::shared_ptr<JointPathFollowComponent> JointPathFollowCompPtr;
	typedef boost::shared_ptr<JointDryRollingFrictionComponent> JointDryRollingFrictionCompPtr;
	typedef boost::shared_ptr<JointGearComponent> JointGearCompPtr;
	typedef boost::shared_ptr<JointRackAndPinionComponent> JointRackAndPinionCompPtr;
	typedef boost::shared_ptr<JointWormGearComponent> JointWormGearCompPtr;
	typedef boost::shared_ptr<JointPulleyComponent> JointPulleyCompPtr;
	typedef boost::shared_ptr<JointSpringComponent> JointSpringCompPtr;
	typedef boost::shared_ptr<JointAttractorComponent> JointAttractorCompPtr;
	typedef boost::shared_ptr<JointUniversalComponent> JointUniversalCompPtr;
	typedef boost::shared_ptr<JointUniversalActuatorComponent> JointUniversalActuatorCompPtr;
	typedef boost::shared_ptr<JointSlidingContactComponent> JointSlidingContactCompPtr;
	typedef boost::shared_ptr<Joint6DofComponent> Joint6DofCompPtr;
	typedef boost::shared_ptr<JointFlexyPipeHandleComponent> JointFlexyPipeHandleCompPtr;
	typedef boost::shared_ptr<JointFlexyPipeSpinnerComponent> JointFlexyPipeSpinnerCompPtr;
	

	typedef boost::shared_ptr<PhysicsComponent> PhysicsCompPtr;
	typedef boost::shared_ptr<PhysicsActiveComponent> PhysicsActiveCompPtr;

	/*******************************JointComponent*******************************/

	JointComponent::JointComponent()
		: GameObjectComponent(),
		jointPosition(Ogre::Vector3::ZERO),
		// jointAlreadyCreated(false),
		body(nullptr),
		joint(nullptr),
		predecessorJointCompPtr(nullptr),
		targetJointCompPtr(nullptr),
		priorId(0),
		debugGeometryNode(nullptr),
		debugGeometryEntity(nullptr),
		debugGeometryNode2(nullptr),
		debugGeometryEntity2(nullptr),
		sceneManager(nullptr),
		useCustomStiffness(false),
		jointAlreadyCreated(false),
		hasCustomJointPosition(false)
	{
		// https://stackoverflow.com/questions/21151264/base-method-is-being-called-instead-of-derive-method-from-constructor
		// Bad idea to call virtual functions in constructor, so getClassName will deliver this one, even its call from a derived class!
		// Hence this call must be duplicated in each joint component, so that the correct class name is delivered
		this->type = new Variant(JointComponent::AttrType(), this->getClassName(), this->attributes);
		this->activated = new Variant(JointComponent::AttrActivated(), true, this->attributes);
		this->id = new Variant(JointComponent::AttrId(), makeUniqueID(), this->attributes, true);
		this->predecessorId = new Variant(JointComponent::AttrPredecessorId(), static_cast<unsigned long>(0), this->attributes, true);
		this->targetId = new Variant(JointComponent::AttrTargetId(), static_cast<unsigned long>(0), this->attributes, true);
		this->jointRecursiveCollision = new Variant(JointComponent::AttrRecursiveCollision(), false, this->attributes);
		this->stiffness = new Variant(JointComponent::AttrStiffness(), 0.9f, this->attributes);
		this->bodyMassScale = new Variant(JointComponent::AttrBodyMassScale(), Ogre::Vector2(1.0f, 1.0f), this->attributes);

		this->predecessorId->setDescription("The predecessor id to set, so that the joint can be connected to the predecessor game object");
		this->targetId->setDescription("The target id to set for this component. This is required for pulley, gear and worm gear joint component.");
		this->jointRecursiveCollision->setDescription("The recursive collision specifies whether this joint should collide with its predecessors. "
			"Attention: This should only be set to true, if this joint does not touch a predecessor joint, else ugly physics behavior is possible.");
		this->bodyMassScale->setDescription("Sets in which ratio the first body is influenced by the mass of the second body and vice versa.");

		this->type->setReadOnly(true);
		this->id->setReadOnly(true);
	}

	JointComponent::~JointComponent()
	{
		if (nullptr != this->debugGeometryNode)
		{
			this->debugGeometryNode->detachAllObjects();
			this->sceneManager->destroySceneNode(this->debugGeometryNode);
			this->debugGeometryNode = nullptr;
			this->sceneManager->destroyMovableObject(this->debugGeometryEntity);
			this->debugGeometryEntity = nullptr;
		}

		if (nullptr != this->debugGeometryNode2)
		{
			this->debugGeometryNode2->detachAllObjects();
			this->sceneManager->destroySceneNode(this->debugGeometryNode2);
			this->debugGeometryNode2 = nullptr;
			this->sceneManager->destroyMovableObject(this->debugGeometryEntity2);
			this->debugGeometryEntity2 = nullptr;
		}

		// Do not remote here, because this destructor will not be called anyway, if its in the joint map!
		// AppStateManager::getSingletonPtr()->getGameObjectController()->removeJointComponent(this->id->getULong());
		
		// Joint must not be deleted, because its already done by OgreNewt, when the body is destroyed by an prior component,
		// so when deleting here, newton throws, because the bodies have invalid DDDD pointer
		this->joint = nullptr;
		this->body = nullptr;
		this->predecessorJointCompPtr = nullptr;
		this->sceneManager = nullptr;
		// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointComponent] Destructor joint component for game object: " + this->gameObjectPtr->getName());
	}

	unsigned long JointComponent::getId(void) const
	{
		return this->id->getULong();
	}

	void JointComponent::setPredecessorId(unsigned long predecessorId)
	{
		// Somehow it can happen between a cloning process and value actualisation, that the id and predecessor id may become the same, so prevent it!
		if (this->id->getULong() != predecessorId)
		{
			this->predecessorId->setValue(predecessorId);
		}
		assert(this->id->getULong() != predecessorId && "Id and precedessorId are the same!");
	}

	unsigned long JointComponent::getPredecessorId(void) const
	{
		return this->predecessorId->getULong();
	}

	void JointComponent::connectPredecessorCompPtr(boost::weak_ptr<JointComponent> weakJointPredecessorCompPtr)
	{
		this->predecessorJointCompPtr = NOWA::makeStrongPtr(weakJointPredecessorCompPtr);
	}

	void JointComponent::connectPredecessorId(unsigned long predecessorId)
	{
		// Used when joints are connected and created
		this->predecessorJointCompPtr = NOWA::makeStrongPtr(AppStateManager::getSingletonPtr()->getGameObjectController()->getJointComponent(predecessorId));
		// If the component does exist, get its id, because it may have changed when cloned via priorId
		if (nullptr != predecessorJointCompPtr)
		{
			if (this->id->getULong() != this->predecessorJointCompPtr->getId())
			{
				this->predecessorId->setValue(this->predecessorJointCompPtr->getId());
			}
			assert(this->id->getULong() != this->predecessorJointCompPtr->getId() && "Id and precedessorId are the same!");
		}
		else
		{
			if (this->id->getULong() != predecessorId)
			{
				this->predecessorId->setValue(predecessorId);
			}
			assert(this->id->getULong() != predecessorId && "Id and precedessorId are the same!");
		}
	}

	void JointComponent::setTargetId(unsigned long targetId)
	{
		if (this->id->getULong() != targetId)
		{
			this->targetId->setValue(targetId);
		}
		assert(this->id->getULong() != targetId && "Id and targetId are the same!");
	}

	void JointComponent::connectTargetId(unsigned long targetId)
	{
		// Used when joints are connected and created
		this->targetJointCompPtr = NOWA::makeStrongPtr(AppStateManager::getSingletonPtr()->getGameObjectController()->getJointComponent(targetId));
		// If the component does exist, get its id, because it may have changed when cloned via priorId
		if (nullptr != targetJointCompPtr)
		{
			if (this->id->getULong() != this->targetJointCompPtr->getId())
			{
				this->targetId->setValue(this->targetJointCompPtr->getId());
			}
			assert(this->id->getULong() != this->targetJointCompPtr->getId() && "Id and targetId are the same!");
		}
		else
		{
			if (this->id->getULong() != targetId)
			{
				this->targetId->setValue(targetId);
			}
			assert(this->id->getULong() != targetId && "Id and targetId are the same!");
		}
	}

	unsigned long JointComponent::getTargetId(void) const
	{
		return this->targetId->getULong();
	}

	GameObjectCompPtr JointComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		JointCompPtr clonedJointCompPtr(boost::make_shared<JointComponent>());

		clonedJointCompPtr->setType(this->type->getString());
		clonedJointCompPtr->internalSetPriorId(this->id->getULong());
		clonedJointCompPtr->setPredecessorId(this->predecessorId->getULong());
		clonedJointCompPtr->setTargetId(this->targetId->getULong());
	
		clonedJointCompPtr->setJointPosition(this->jointPosition);
		clonedJointCompPtr->setJointRecursiveCollisionEnabled(this->jointRecursiveCollision->getBool());
		clonedJointCompPtr->stiffness->setValue(this->stiffness->getReal());
		clonedJointCompPtr->setBodyMassScale(this->bodyMassScale->getVector2());

		clonedGameObjectPtr->addComponent(clonedJointCompPtr);
		clonedJointCompPtr->setOwner(clonedGameObjectPtr);

		return clonedJointCompPtr;
	}

	bool JointComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointComponent] Init joint component for game object: " + this->gameObjectPtr->getName());

		AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &JointComponent::deleteJointDelegate), EventDataDeleteJoint::getStaticEventType());

		if (nullptr != this->gameObjectPtr)
			this->sceneManager = this->gameObjectPtr->getSceneManager();

		bool foundPriorPhysicsComponent = false;

		for (size_t i = 0; i < this->gameObjectPtr->getComponents()->size(); i++)
		{
			// Working here with shared_ptrs is evil, because of bidirectional referecing
			auto component = std::get<COMPONENT>(this->gameObjectPtr->getComponents()->at(i));
			auto physicsComponent = boost::dynamic_pointer_cast<PhysicsComponent>(component);
			if (physicsComponent)
			{
				foundPriorPhysicsComponent = true;
			}

			// If this component has been reached and no prior physics component found, throw!
			if (component.get() == this)
			{
				if (false == foundPriorPhysicsComponent)
				{
					// Attention: A joint component must always posses a prior physics component! Else in destructor (just release mode) ugly heavy crash occurs!
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointComponent] Illegal state: Could not add joint component for game object: "
						+ this->gameObjectPtr->getName() + " because a physics component must be placed before a joint component! Please swap the components or add a prior physics component!");
					// throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[JointComponent] Illegal state: Could not add joint component for game object: "
					// + this->gameObjectPtr->getName() + " because a physics component must be placed before a joint component! Please swap the components or add a prior physics component!\n", "NOWA");

					return false;
				}
			}
		}

		return true;
	}

	bool JointComponent::connect(void)
	{
// Attention: not every component needs finalInit, so it can be removed, and this default one will be used
		
		PhysicsCompPtr physicsCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsComponent>());
		if (nullptr != physicsCompPtr)
		{
			
			// Add the joint to the joint component map, but as weak ptr, because game object controller should not hold the life cycle of this components, because the game objects already do, which are
			// also hold shared by the game object controller
			AppStateManager::getSingletonPtr()->getGameObjectController()->addJointComponent(boost::dynamic_pointer_cast<JointComponent>(shared_from_this()));
			
			this->body = physicsCompPtr->getBody();
			this->predecessorJointCompPtr = NOWA::makeStrongPtr(AppStateManager::getSingletonPtr()->getGameObjectController()->getJointComponent(this->predecessorId->getULong()));
			// Important to reset id, because when cloned, the joint component from prior one has been gathered, so actualize this id to the new one
			if (nullptr != this->predecessorJointCompPtr)
			{
				if (this->id->getULong() != this->predecessorJointCompPtr->getId())
				{
					this->predecessorId->setValue(this->predecessorJointCompPtr->getId());
				}
				// This assert becomes active when 2 components are cloned, the predecessor from third is set to id of the second and play pressed, really strange! Maybe a GUI effect?
				// assert(this->id->getULong() != this->predecessorJointCompPtr->getId() && "Id and predecessorJointCompPtr are the same!");
			}
			this->targetJointCompPtr = NOWA::makeStrongPtr(AppStateManager::getSingletonPtr()->getGameObjectController()->getJointComponent(this->targetId->getULong()));
			if (nullptr != this->targetJointCompPtr)
			{
				if (this->id->getULong() != this->targetJointCompPtr->getId())
				{
					this->targetId->setValue(this->targetJointCompPtr->getId());
				}
				assert(this->id->getULong() != this->targetJointCompPtr->getId() && "Id and targetJointCompPtr are the same!");
			}
			return true;
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointComponent] Error: Final init failed, because the game object: "
				+ this->gameObjectPtr->getName() + " has no kind of physics component!");
			return false;
		}
	}

	bool JointComponent::disconnect(void)
	{
		AppStateManager::getSingletonPtr()->getGameObjectController()->removeJointComponent(this->id->getULong());

		this->releaseJoint(true);

		return true;
	}

	bool JointComponent::onCloned(void)
	{
		return this->connect();
	}

	void JointComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &JointComponent::deleteJointDelegate), EventDataDeleteJoint::getStaticEventType());

		AppStateManager::getSingletonPtr()->getGameObjectController()->removeJointComponentBreakJointChain(this->id->getULong());
	}

	void JointComponent::onOtherComponentRemoved(unsigned int index)
	{
		GameObjectComponent::onOtherComponentRemoved(index);

	}

	Ogre::String JointComponent::getClassName(void) const
	{
		return "JointComponent";
	}

	Ogre::String JointComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	bool JointComponent::init(xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointType")
		{
			this->type->setValue(XMLConverter::getAttrib(propertyElement, "data", "JointComponent"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointActivate")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointId")
		{
			// Really important: else the loaded value will not be written and the old one from constructor is present!
			this->id->setReadOnly(false);
			this->id->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			this->id->setReadOnly(true);
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointPredecessorId")
		{
			this->predecessorId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointTargetId")
		{
			this->targetId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointRecursiveCollision")
		{
			this->jointRecursiveCollision->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Stiffness")
		{
			this->stiffness->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BodyMassScale")
		{
			this->bodyMassScale->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	void JointComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (JointComponent::AttrActivated() == attribute->getName())
		{
			this->activated->setValue(attribute->getBool());
		}
		else if (JointComponent::AttrPredecessorId() == attribute->getName())
		{
			this->setPredecessorId(attribute->getULong());
		}
		else if (JointComponent::AttrTargetId() == attribute->getName())
		{
			this->setTargetId(attribute->getULong());
		}
		else if (JointComponent::AttrRecursiveCollision() == attribute->getName())
		{
			this->setJointRecursiveCollisionEnabled(attribute->getBool());
		}
		else if (JointComponent::AttrStiffness() == attribute->getName())
		{
			this->setStiffness(attribute->getReal());
		}
		else if (JointComponent::AttrBodyMassScale() == attribute->getName())
		{
			this->setBodyMassScale(attribute->getVector2());
		}
	}

	void JointComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointType"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->type->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointActivate"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->id->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointPredecessorId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->predecessorId->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointTargetId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->targetId->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointRecursiveCollision"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->jointRecursiveCollision->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Stiffness"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->stiffness->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BodyMassScale"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->bodyMassScale->getVector2())));
		propertiesXML->append_node(propertyXML);
	}

	void JointComponent::setType(const Ogre::String& type)
	{
		this->type->setValue(type);
	}

	Ogre::String JointComponent::getType(void) const
	{
		return this->type->getString();
	}

	OgreNewt::Joint* JointComponent::getJoint(void) const
	{
		return this->joint;
	}

	void JointComponent::setBody(OgreNewt::Body* body)
	{
		this->body = body;
	}

	OgreNewt::Body* JointComponent::getBody(void) const
	{
		return this->body;
	}

	void JointComponent::setJointPosition(const Ogre::Vector3& jointPosition)
	{
		this->jointPosition = jointPosition;
	}

	Ogre::Vector3 JointComponent::getJointPosition(void) const
	{
		return this->jointPosition;
	}

	void JointComponent::setStiffness(Ogre::Real stiffness)
	{
		this->useCustomStiffness = true;
		if (stiffness < 0.0f)
			stiffness = 0.0f;
		if (stiffness > 1.0f)
			stiffness = 1.0f;
		this->stiffness->setValue(stiffness);
	}

	Ogre::Real JointComponent::getStiffness(void) const
	{
		return this->stiffness->getReal();
	}

	void JointComponent::applyStiffness(void)
	{
		if (true == this->useCustomStiffness)
		{
			if (nullptr != this->joint)
			{
				this->joint->setStiffness(this->stiffness->getReal());
				this->useCustomStiffness = false;
			}
		}
	}

	/*void JointComponent::setMinimumFriction(Ogre::Real minimumFriction)
	{
		if (minimumFriction > 0.0f)
			minimumFriction = 0.0f;

		if (nullptr != this->joint)
		{
			this->joint->setRowMinimumFriction(minimumFriction);
		}
	}

	void JointComponent::setMaximumFriction(Ogre::Real maximumFriction)
	{
		if (maximumFriction < 0.0f)
			maximumFriction = 0.0f;

		if (nullptr != this->joint)
		{
			this->joint->setRowMaximumFriction(maximumFriction);
		}
	}*/

	void JointComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);

		this->jointAlreadyCreated = false;
		if (true == activated)
		{
			this->createJoint();
		}
		// Note: release joint does not make any sense, else the joint is gone! E.g. deactivate lever motion, lever would flow in the universe
		else
		{
			// Only release this joint and not other chained joints or predecessors, targets!
			this->internalReleaseJoint();
		}
	}

	bool JointComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	Ogre::Vector3 JointComponent::getUpdatedJointPosition(void)
	{
		return this->jointPosition;
	}

	void JointComponent::internalSetPriorId(unsigned long priorId)
	{
		this->priorId = priorId;
	}

	unsigned long JointComponent::getPriorId(void) const
	{
		return this->priorId;
	}

	void JointComponent::internalShowDebugData(bool activate, unsigned short type, const Ogre::Vector3& value1, const Ogre::Vector3& value2)
	{
		if (nullptr != this->body)
		{
			if (true == this->bShowDebugData && true == activate)
			{
				if (nullptr == this->debugGeometryNode)
				{
					// For hinge
					if (0 == type)
					{
						if (nullptr != this->body->getOgreNode())
						{
							this->debugGeometryNode = static_cast<Ogre::SceneNode*>(this->body->getOgreNode())->createChildSceneNode(Ogre::SCENE_DYNAMIC);
							this->debugGeometryNode->setPosition(value1);
							this->debugGeometryNode->setDirection(value2);
							// Do not inherit, because if parent node is scaled, then this scale is relative and debug data may be to small or to big
							this->debugGeometryNode->setInheritScale(false);
							this->debugGeometryNode->setScale(0.05f, 0.05f, 0.025f);
							this->debugGeometryNode->setName(this->getClassName() + "_Node");
							this->debugGeometryEntity = this->sceneManager->createEntity("Arrow.mesh");
							this->debugGeometryEntity->setName(this->getClassName() + "_Entity");
							this->debugGeometryEntity->setDatablock("BaseRedLine");
							this->debugGeometryEntity->setQueryFlags(0 << 0);
							this->debugGeometryEntity->setCastShadows(false);
							this->debugGeometryNode->attachObject(this->debugGeometryEntity);
						}
					}

					// For ball and socket
					else if (1 == type)
					{
						if (true == this->bShowDebugData)
						{
							if (nullptr == this->debugGeometryNode)
							{
								this->debugGeometryNode = static_cast<Ogre::SceneNode*>(this->body->getOgreNode())->createChildSceneNode(Ogre::SCENE_DYNAMIC);
								this->debugGeometryNode->setPosition(value1);
								// this->debugGeometryNode->setDirection(value2);
								this->debugGeometryNode->setName(this->getClassName() + "_Node");
								// Do not inherit, because if parent node is scaled, then this scale is relative and debug data may be to small or to big
								this->debugGeometryNode->setInheritScale(false);
								this->debugGeometryNode->setScale(0.05f, 0.05f, 0.05f);
								this->debugGeometryEntity = this->sceneManager->createEntity("gizmosphere.mesh");
								this->debugGeometryEntity->setName(this->getClassName() + "_Entity");
								this->debugGeometryEntity->setDatablock("BaseRedLine");
								this->debugGeometryEntity->setQueryFlags(0 << 0);
								this->debugGeometryEntity->setCastShadows(false);
								this->debugGeometryNode->attachObject(this->debugGeometryEntity);
							}
						}
					}
					// For point to point
					else if (2 == type)
					{
						if (nullptr != this->gameObjectPtr)
						{
							if (true == this->bShowDebugData)
							{
								if (nullptr == this->debugGeometryNode)
								{
									this->debugGeometryNode = static_cast<Ogre::SceneNode*>(this->body->getOgreNode())->createChildSceneNode(Ogre::SCENE_DYNAMIC);
									this->debugGeometryNode->setPosition(value1);
									this->debugGeometryNode->setName(this->getClassName() + "_Node");
									// Do not inherit, because if parent node is scaled, then this scale is relative and debug data may be to small or to big
									this->debugGeometryNode->setInheritScale(false);
									this->debugGeometryNode->setScale(0.05f, 0.05f, 0.05f);
									this->debugGeometryEntity = this->sceneManager->createEntity("gizmosphere.mesh");
									this->debugGeometryEntity->setName(this->getClassName() + "_Entity");
									this->debugGeometryEntity->setDatablock("BaseRedLine");
									this->debugGeometryEntity->setQueryFlags(0 << 0);
									this->debugGeometryEntity->setCastShadows(false);
									this->debugGeometryNode->attachObject(this->debugGeometryEntity);

									this->debugGeometryNode2 = static_cast<Ogre::SceneNode*>(this->body->getOgreNode())->createChildSceneNode(Ogre::SCENE_DYNAMIC);
									this->debugGeometryNode2->setName(this->getClassName() + "_Node2");
									this->debugGeometryNode2->setPosition(value2);
									// Do not inherit, because if parent node is scaled, then this scale is relative and debug data may be to small or to big
									this->debugGeometryNode2->setInheritScale(false);
									this->debugGeometryNode2->setScale(0.05f, 0.05f, 0.05f);
									this->debugGeometryEntity2 = this->gameObjectPtr->getSceneManager()->createEntity("gizmosphere.mesh");
									this->debugGeometryEntity2->setName(this->getClassName() + "_Entity2");
									this->debugGeometryEntity2->setDatablock("BaseRedLine");
									this->debugGeometryEntity2->setQueryFlags(0 << 0);
									this->debugGeometryEntity2->setCastShadows(false);
									this->debugGeometryNode2->attachObject(this->debugGeometryEntity2);
								}
							}
						}
					}
				}
			}
		}

		if (false == this->bShowDebugData || false == activate)
		{
			if (nullptr != this->debugGeometryNode)
			{
				this->debugGeometryNode->detachAllObjects();
				this->gameObjectPtr->getSceneManager()->destroySceneNode(this->debugGeometryNode);
				this->debugGeometryNode = nullptr;
				this->gameObjectPtr->getSceneManager()->destroyMovableObject(this->debugGeometryEntity);
				this->debugGeometryEntity = nullptr;
			}
			if (nullptr != this->debugGeometryNode2)
			{
				this->debugGeometryNode2->detachAllObjects();
				this->gameObjectPtr->getSceneManager()->destroySceneNode(this->debugGeometryNode2);
				this->debugGeometryNode2 = nullptr;
				this->gameObjectPtr->getSceneManager()->destroyMovableObject(this->debugGeometryEntity2);
				this->debugGeometryEntity2 = nullptr;
			}
		}
	}
	
	void JointComponent::setJointRecursiveCollisionEnabled(bool enable)
	{
		this->jointRecursiveCollision->setValue(enable);
		// Attention: Nothing will collide with this body anymore, check for what this function is good!?
		/*if (nullptr != this->body)
		{
			// this->body->setJointRecursiveCollision(true == enable ? 1 : 0);
		}*/
		if (nullptr != this->joint)
		{
			this->joint->setCollisionState(true == enable ? 1 : 0);
		}
	}

	bool JointComponent::getJointRecursiveCollisionEnabled(void) const
	{
		return this->jointRecursiveCollision->getBool();
	}

	void JointComponent::setBodyMassScale(const Ogre::Vector2& bodyMassScale)
	{
		this->bodyMassScale->setValue(bodyMassScale);
		if (nullptr != this->joint)
		{
			this->joint->setBodyMassScale(bodyMassScale.x, bodyMassScale.y);
		}
	}

	Ogre::Vector2 JointComponent::getBodyMassScale(void) const
	{
		return this->bodyMassScale->getVector2();
	}

	bool JointComponent::createJoint(const Ogre::Vector3& customJointPosition)
	{
		// If a joint is later attached (e.g. in lua script at runtime, all joints have already been connected in game object controller, hence test, if there is a still a joint, that has not yet been connected.
		if (0 != this->predecessorId->getULong() && nullptr == this->predecessorJointCompPtr)
		{
			this->jointAlreadyCreated = false;
			this->connectPredecessorId(this->predecessorId->getULong());
		}
		if (0 != this->targetId->getULong() && nullptr == this->targetJointCompPtr)
		{
			this->jointAlreadyCreated = false;
			this->connectTargetId(this->targetId->getULong());
		}

		if (false == this->activated->getBool())
		{
			return false;
		}

		return true;
	}

	void JointComponent::releaseJoint(bool resetPredecessorAndTarget)
	{
		if (nullptr != this->joint)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointComponent] Joint: "
				+ this->type->getString() + " released for game object: " + this->gameObjectPtr->getName());

			this->jointAlreadyCreated = false;

			this->joint->destroyJoint(AppStateManager::getSingletonPtr()->getOgreNewtModule()->getOgreNewt());
			delete this->joint;
			this->joint = nullptr;

			boost::shared_ptr<EventDataDeleteJoint> deleteJointEvent(boost::make_shared<EventDataDeleteJoint>(this->getId()));
			AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(deleteJointEvent);

			if (true == resetPredecessorAndTarget)
			{
				// Really import to reset the pointer, so that a game object can be deleted, even if other joints point to this joint via predecessor or target
				if (nullptr != predecessorJointCompPtr)
				{
					this->predecessorJointCompPtr.reset();
				}
				if (nullptr != targetJointCompPtr)
				{
					this->targetJointCompPtr.reset();
				}
			}
		}
	}

	void JointComponent::internalReleaseJoint(void)
	{
		if (nullptr != this->joint)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointComponent] Joint: "
															+ this->type->getString() + " released for game object: " + this->gameObjectPtr->getName());

			this->jointAlreadyCreated = false;

			this->joint->destroyJoint(AppStateManager::getSingletonPtr()->getOgreNewtModule()->getOgreNewt());
			delete this->joint;
			this->joint = nullptr;
		}
	}

	void JointComponent::deleteJointDelegate(EventDataPtr eventData)
	{
		boost::shared_ptr<EventDataDeleteJoint> castEventData = boost::static_pointer_cast<NOWA::EventDataDeleteJoint>(eventData);

		// If a joint e.g. has been attached externally in a lua script and the predecessor or target is gone, delete this joint too!
		if (this->predecessorId->getULong() == castEventData->getJointId())
		{
			this->releaseJoint(true);
			this->body = nullptr;
		}
		else if (this->targetId->getULong() == castEventData->getJointId())
		{
			this->releaseJoint(true);
			this->body = nullptr;
		}
	}

	void JointComponent::setSceneManager(Ogre::SceneManager* sceneManager)
	{
		this->sceneManager = sceneManager;
	}

	/*******************************JointHingeComponent*******************************/

	JointHingeComponent::JointHingeComponent()
		: JointComponent()
	{
		// Note that also in JointComponent internalBaseInit() is called, which sets the values for JointComponent, so this is called for already existing values
		// But luckely in Variant, no new attributes are added, but the values changed via interalAdd(...) in Variant
		this->type->setReadOnly(false);
		this->type->setValue(this->getClassName());
		this->type->setReadOnly(true);
		this->type->setDescription("An object attached to a hinge joint can only rotate around one dimension perpendicular to the axis it is attached to. A good real-life example would be a swinging door or a propeller.");
		
		this->anchorPosition = new Variant(JointHingeComponent::AttrAnchorPosition(), Ogre::Vector3::ZERO, this->attributes);
		this->pin = new Variant(JointHingeComponent::AttrPin(), Ogre::Vector3(0.0f, 0.0f, 1.0f), this->attributes);
		this->enableLimits = new Variant(JointHingeComponent::AttrLimits(), false, this->attributes);
		this->minAngleLimit = new Variant(JointHingeComponent::AttrMinAngleLimit(), 0.0f, this->attributes);
		this->maxAngleLimit = new Variant(JointHingeComponent::AttrMaxAngleLimit(), 0.0f, this->attributes);
		this->torque = new Variant(JointHingeComponent::AttrTorque(), 0.0f, this->attributes);
		this->breakForce = new Variant(JointHingeComponent::AttrBreakForce(), 0.0f, this->attributes);
		this->breakTorque = new Variant(JointHingeComponent::AttrBreakTorque(), 0.0f, this->attributes);

		this->asSpringDamper = new Variant(JointHingeComponent::AttrAsSpringDamper(), false, this->attributes);
		this->massIndependent = new Variant(JointHingeComponent::AttrMassIndependent(), false, this->attributes);
		this->springK = new Variant(JointHingeComponent::AttrSpringK(), 128.0f, this->attributes);
		this->springD = new Variant(JointHingeComponent::AttrSpringD(), 4.0f, this->attributes);
		this->springDamperRelaxation = new Variant(JointHingeComponent::AttrSpringDamperRelaxation(), 0.6f, this->attributes);
		this->springDamperRelaxation->setDescription("[0 - 0.99]");
		this->friction = new Variant(JointHingeComponent::AttrFriction(), -1.0f, this->attributes);

		this->targetId->setVisible(false);

		Ogre::String torqueFrictionDescription = "Sets the friction (max. torque) for torque. That is, without a friction the torque will not work. So get a balance between torque and friction (1, 20) e.g. The more friction, the more powerful the motor will become.";
		this->friction->setDescription(torqueFrictionDescription);
		this->torque->setDescription(torqueFrictionDescription);
		// this->brakeForce->setVisible(false);
		// this->omega->setVisible(false);
		// this->torgue->setVisible(false);
		// this->maxAngleLimit->setVisible(false);
		// this->stiffness->setVisible(false);
		// this->springK->setVisible(false);
		// this->springD->setVisible(false);
		this->massIndependent->setDescription("Is used in conjunction with spring.");
	}

	JointHingeComponent::~JointHingeComponent()
	{
		// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointHingeComponent] Destroyed");
	}

	bool JointHingeComponent::init(xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		JointComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointAnchorPosition")
		{
			this->anchorPosition->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(0.0f, 0.0f, 0.0f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointPin")
		{
			this->pin->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(0.0f, 0.0f, 0.0f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointLimits")
		{
			this->enableLimits->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointTorgue")
		{
			this->torque->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointMinAngleLimit")
		{
			this->minAngleLimit->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointMaxAngleLimit")
		{
			this->maxAngleLimit->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointBreakForce")
		{
			this->breakForce->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointBreakTorque")
		{
			this->breakTorque->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointAsSpringDamper")
		{
			this->asSpringDamper->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointMassIndependent")
		{
			this->massIndependent->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointKSpring")
		{
			this->springK->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointDSpring")
		{
			this->springD->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointSpringDamperRelaxation")
		{
			this->springDamperRelaxation->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointFriction")
		{
			this->friction->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		return true;
	}

	bool JointHingeComponent::postInit(void)
	{
		JointComponent::postInit();

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointHingeComponent] Init joint hinge component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool JointHingeComponent::connect(void)
	{
		bool success = JointComponent::connect();

		this->internalShowDebugData(true, 0, this->getUpdatedJointPosition(), this->pin->getVector3());

		return success;
	}

	bool JointHingeComponent::disconnect(void)
	{
		bool success = JointComponent::disconnect();

		this->internalShowDebugData(false, 0, this->getUpdatedJointPosition(), this->pin->getVector3());

		return success;
	}

	Ogre::String JointHingeComponent::getClassName(void) const
	{
		return "JointHingeComponent";
	}

	Ogre::String JointHingeComponent::getParentClassName(void) const
	{
		return "JointComponent";
	}

	void JointHingeComponent::update(Ogre::Real dt, bool notSimulating)
	{
		//if (nullptr != this->body)
		//{
		//	Ogre::Vector3 offset = (-this->anchorPosition->getVector3() * this->body->getAABB().getSize());
		//	this->jointPosition = (this->body->getPosition() - offset) + this->offsetPosition->getVector3();

		//	// Set debug data if enabled
		//	if (nullptr != this->debugGeometryNode)
		//	{
		//		this->debugGeometryNode->setPosition(this->jointPosition);
		//		// Direction does to much, it also translates and rotates
		//		// this->debugGeometryNode->setDirection(this->pin->getVector3(), Ogre::SceneNode::TransformSpace::TS_WORLD);
		//		this->debugGeometryNode->setOrientation(Ogre::Vector3::NEGATIVE_UNIT_Z.getRotationTo(this->pin->getVector3()));
		//	}
		//}

		if (nullptr != this->joint)
		{
			Ogre::Real breakForceLength = this->joint->getForce0().length();
			Ogre::Real breakTorqueLength = this->joint->getTorque0().length();

			if (0.0f != this->breakForce->getReal() || 0.0f != this->breakTorque->getReal())
			{
				if (breakForceLength >= this->breakForce->getReal() || breakTorqueLength >= this->breakTorque->getReal())
				{
					this->releaseJoint();
				}
			}

			if (nullptr != this->predecessorJointCompPtr && nullptr != this->predecessorJointCompPtr->getJoint())
			{
				Ogre::Real breakForceLength = this->predecessorJointCompPtr->getJoint()->getForce0().length();
				Ogre::Real breakTorqueLength = this->predecessorJointCompPtr->getJoint()->getTorque0().length();

				auto predecessorHinge = boost::dynamic_pointer_cast<JointHingeComponent>(this->predecessorJointCompPtr);
				if (nullptr != predecessorHinge)
				{
					Ogre::Real requiredBreakForce = predecessorHinge->getBreakForce();
					Ogre::Real requiredBreakTorque = predecessorHinge->getBreakTorque();

					if (0.0f != requiredBreakForce || 0.0f != requiredBreakTorque)
					{
						if (breakForceLength >= requiredBreakForce || breakTorqueLength >= requiredBreakTorque)
						{
							this->predecessorJointCompPtr->releaseJoint();
						}
					}
				}
			}
		}

		//This was in physicsActiveComponent
		//for (auto& it = this->jointHandlers.begin(); it != this->jointHandlers.end(); ++it)
		//{
		//	auto& jointHandlerPtr = *it;
		//	auto& jointHingeHandlerPtr = boost::dynamic_pointer_cast<JointHingeHandler>(jointHandlerPtr);
		//	if (nullptr != jointHingeHandlerPtr)
		//	{
		//		/*Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveDestructableComponent::SplitPart] Acting Force: "
		//		+ Ogre::StringConverter::toString(body->getTorque()));*/
		//		if (0.0f < jointHingeHandlerPtr->getBreakForce() || 0.0f < jointHingeHandlerPtr->getBreakTorque())
		//		{
		//			Ogre::Real breakForceLength = body->getForce().length();
		//			Ogre::Real breakTorqueLength = body->getTorque().length();
		//			// check if the force acting on the body is higher as the specified force to break the joint
		//			if (breakForceLength >= jointHingeHandlerPtr->getBreakForce() || breakTorqueLength >= jointHingeHandlerPtr->getBreakTorque())
		//			{
		//				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveComponent] Hinge: "
		//					+ jointHingeHandlerPtr->getName() + " broken, break force: " + Ogre::StringConverter::toString(jointHingeHandlerPtr->getBreakForce())
		//					+ " break torque: " + Ogre::StringConverter::toString(jointHingeHandlerPtr->getBreakTorque())
		//					+ " acting force: " + Ogre::StringConverter::toString(breakForceLength)
		//					+ " acting torque: " + Ogre::StringConverter::toString(breakTorqueLength));

		//				// can be called by x-configured threads, therefore lock here the remove operation
		//				mutex.lock();

		//				AppStateManager::getSingletonPtr()->getGameObjectController()->removeJointHandler(jointHandlerPtr->getId());
		//				// Add for deletion in next update
		//				this->delayedDeleteJointHandlerList.emplace(jointHandlerPtr);

		//				mutex.unlock();

		//				// nothing with predecessor, because the joint will be created in child
		//			}
		//		}
		//	}
		//}
	}

	GameObjectCompPtr JointHingeComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		JointHingeCompPtr clonedJointCompPtr(boost::make_shared<JointHingeComponent>());
		
		clonedJointCompPtr->setType(this->type->getString());
		clonedJointCompPtr->internalSetPriorId(this->id->getULong());
		clonedJointCompPtr->setPredecessorId(this->predecessorId->getULong());
		clonedJointCompPtr->setTargetId(this->targetId->getULong());
		clonedJointCompPtr->setJointPosition(this->jointPosition);
		clonedJointCompPtr->setJointRecursiveCollisionEnabled(this->jointRecursiveCollision->getBool());
		clonedJointCompPtr->stiffness->setValue(this->stiffness->getReal()); // Do not call setter, because internally a bool flag is set, so that stiffness is always used

		clonedJointCompPtr->setAnchorPosition(this->anchorPosition->getVector3());
		clonedJointCompPtr->setPin(this->pin->getVector3());
		clonedJointCompPtr->setLimitsEnabled(this->enableLimits->getBool());
		clonedJointCompPtr->setMinMaxAngleLimit(Ogre::Degree(this->minAngleLimit->getReal()), Ogre::Degree(this->maxAngleLimit->getReal()));
		clonedJointCompPtr->setTorgue(this->torque->getReal());
		clonedJointCompPtr->setBreakForce(this->breakForce->getReal());
		clonedJointCompPtr->setBreakTorque(this->breakTorque->getReal());
		clonedJointCompPtr->setSpring(this->asSpringDamper->getBool(), this->massIndependent->getBool(), this->springDamperRelaxation->getReal(), this->springK->getReal(), this->springD->getReal());
		clonedJointCompPtr->setFriction(this->friction->getReal());

		clonedGameObjectPtr->addComponent(clonedJointCompPtr);
		clonedJointCompPtr->setOwner(clonedGameObjectPtr);

		clonedJointCompPtr->setActivated(this->activated->getBool());

		return clonedJointCompPtr;
	}

	bool JointHingeComponent::createJoint(const Ogre::Vector3& customJointPosition)
	{
		// Joint base created but not activated, return false, but its no error, hence after that return true.
		if (false == JointComponent::createJoint(customJointPosition))
		{
			return true;
		}

		Ogre::String gameObjectName = this->gameObjectPtr->getName();

		if (true == this->jointAlreadyCreated)
		{
			// joint already created so skip
			return true;
		}
		
		if (nullptr == this->body)
		{
			this->connect();
		}
		if (Ogre::Vector3::ZERO == this->pin->getVector3())
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[JointHingeComponent] Cannot create joint for: " + gameObjectName + " because the pin is zero.");
			return false;
		}

		if (Ogre::Vector3::ZERO == customJointPosition)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			this->jointPosition = (this->body->getPosition() + offset);
		}
		else
		{
			this->hasCustomJointPosition = true;
			this->jointPosition = customJointPosition;
		}

		OgreNewt::Body* predecessorBody = nullptr;
		if (this->predecessorJointCompPtr)
		{
			predecessorBody = this->predecessorJointCompPtr->getBody();
			if (nullptr != this->predecessorJointCompPtr->getOwner() && nullptr != this->gameObjectPtr)
			{
				Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[JointHingeComponent] Creating hinge joint for body1 name: "
					+ this->predecessorJointCompPtr->getOwner()->getName() + " body2 name: " + gameObjectName);
			}
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointHingeComponent] Created hinge joint: " + gameObjectName + " with world as parent");
		}

		// Release joint each time, to create new one with new values
		this->internalReleaseJoint();
		
		/*JointHingeActuatorCompPtr jointHingeActuatorCompPtr = boost::dynamic_pointer_cast<JointHingeActuatorComponent>(this->targetJointCompPtr);
		if (nullptr == jointHingeActuatorCompPtr)*/
		{
			this->joint = new OgreNewt::Hinge(this->body, predecessorBody, this->jointPosition, this->body->getOrientation() * this->pin->getVector3());
		}
//		else
//		{
//// Attention:
//			this->joint = new OgreNewt::Hinge(this->body, predecessorBody, this->jointPosition, this->body->getOrientation() * this->pin->getVector3(),
//				jointHingeActuatorCompPtr->getJointPosition(), predecessorBody->getOrientation() * jointHingeActuatorCompPtr->getPin());
//		}

		this->joint->setBodyMassScale(this->bodyMassScale->getVector2().x, this->bodyMassScale->getVector2().y);

		// Bad, because causing jerky behavior on ragdolls?
		this->joint->setCollisionState(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		// this->body->setJointRecursiveCollision(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		// this->applyStiffness();

		OgreNewt::Hinge* hingeJoint = dynamic_cast<OgreNewt::Hinge*>(this->joint);
		hingeJoint->SetSpringAndDamping(this->asSpringDamper->getBool(), this->massIndependent->getBool(), this->springDamperRelaxation->getReal(), this->springK->getReal(), this->springD->getReal());
		hingeJoint->EnableLimits(this->enableLimits->getBool());

		if (nullptr != this->gameObjectPtr)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointHingeComponent] Created joint: " + gameObjectName);
		}

		hingeJoint->SetTorque(this->torque->getReal());

		if (-1.0f != this->friction->getReal())
			hingeJoint->SetFriction(this->friction->getReal());

		if (this->enableLimits->getBool())
		{
			hingeJoint->SetLimits(Ogre::Degree(this->minAngleLimit->getReal()), Ogre::Degree(this->maxAngleLimit->getReal()));
		}

		// Must be done, to get correct joint force values
		if (0.0f != this->breakForce->getReal() || 0.0f != this->breakTorque->getReal())
		{
			hingeJoint->setJointForceCalculation(true);
		}

		this->jointAlreadyCreated = true;

		return true;
	}

	void JointHingeComponent::forceShowDebugData(bool activate)
	{
		this->internalShowDebugData(activate, 0, this->getUpdatedJointPosition(), this->pin->getVector3());
	}

	void JointHingeComponent::actualizeValue(Variant* attribute)
	{
		JointComponent::actualizeValue(attribute);
		
		if (JointHingeComponent::AttrAnchorPosition() == attribute->getName())
		{
			this->setAnchorPosition(attribute->getVector3());
		}
		else if (JointHingeComponent::AttrPin() == attribute->getName())
		{
			this->setPin(attribute->getVector3());
		}
		else if (JointHingeComponent::AttrLimits() == attribute->getName())
		{
			this->setLimitsEnabled(attribute->getBool());
		}
		else if (JointHingeComponent::AttrMinAngleLimit() == attribute->getName())
		{
			this->setMinMaxAngleLimit(Ogre::Degree(attribute->getReal()), Ogre::Degree(this->maxAngleLimit->getReal()));
		}
		else if (JointHingeComponent::AttrMaxAngleLimit() == attribute->getName())
		{
			this->setMinMaxAngleLimit(Ogre::Degree(this->minAngleLimit->getReal()), Ogre::Degree(attribute->getReal()));
		}
		else if (JointHingeComponent::AttrTorque() == attribute->getName())
		{
			this->setTorgue(attribute->getReal());
		}
		else if (JointHingeComponent::AttrBreakForce() == attribute->getName())
		{
			this->setBreakForce(attribute->getReal());
		}
		else if (JointHingeComponent::AttrBreakTorque() == attribute->getName())
		{
			this->setBreakTorque(attribute->getReal());
		}
		else if (JointHingeComponent::AttrAsSpringDamper() == attribute->getName())
		{
			this->setSpring(attribute->getBool(), this->massIndependent->getBool(), this->springDamperRelaxation->getReal(), this->springK->getReal(), this->springD->getReal());
		}
		else if (JointHingeComponent::AttrMassIndependent() == attribute->getName())
		{
			this->setSpring(asSpringDamper->getBool(), attribute->getBool(), this->springDamperRelaxation->getReal(), this->springK->getReal(), this->springD->getReal());
		}
		else if (JointHingeComponent::AttrSpringDamperRelaxation() == attribute->getName())
		{
			this->setSpring(this->asSpringDamper->getBool(), this->massIndependent->getBool(), attribute->getReal(), this->springK->getReal(), this->springD->getReal());
		}
		else if (JointHingeComponent::AttrSpringK() == attribute->getName())
		{
			this->setSpring(this->asSpringDamper->getBool(), this->massIndependent->getBool(), this->springDamperRelaxation->getReal(), attribute->getReal(), this->springD->getReal());
		}
		else if (JointHingeComponent::AttrSpringD() == attribute->getName())
		{
			this->setSpring(this->asSpringDamper->getBool(), this->massIndependent->getBool(), this->springDamperRelaxation->getReal(), this->springK->getReal(), attribute->getReal());
		}
		else if (JointHingeComponent::AttrFriction() == attribute->getName())
		{
			this->setFriction(attribute->getReal());
		}
	}

	void JointHingeComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		JointComponent::writeXML(propertiesXML, doc, filePath);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointAnchorPosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->anchorPosition->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointPin"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->pin->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointLimits"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->enableLimits->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointTorgue"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->torque->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointMinAngleLimit"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->minAngleLimit->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointMaxAngleLimit"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxAngleLimit->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointBreakForce"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->breakForce->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointBreakTorque"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->breakTorque->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointAsSpringDamper"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->asSpringDamper->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointMassIndependent"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->massIndependent->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointKSpring"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->springK->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointDSpring"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->springD->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointSpringDamperRelaxation"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->springDamperRelaxation->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointFriction"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->friction->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::Vector3 JointHingeComponent::getUpdatedJointPosition(void)
	{
		if (nullptr == this->body)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointHingeComponent] Could not get 'getUpdatedJointPosition', because there is no body for game object: " + this->gameObjectPtr->getName());
			return Ogre::Vector3::ZERO;
		}

		if (false == this->hasCustomJointPosition)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			return this->body->getPosition() + offset;
		}
		else
		{
			return this->jointPosition;
		}
	}

	void JointHingeComponent::setAnchorPosition(const Ogre::Vector3& anchorPosition)
	{
		this->anchorPosition->setValue(anchorPosition);

		if (nullptr != this->body)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			//Ogre::Vector3 aPos = Ogre::Vector3(-this->anchorPosition.x, -this->anchorPosition.y, -this->anchorPosition.z);
			//// relative anchor pos: 0.5 0 0 means 50% in X of the size of the parent object
			//Ogre::Vector3 offset = (aPos * size);
			//// take orientation into account, so that orientated joints are processed correctly
			//this->jointPosition = this->body->getPosition() - (this->body->getOrientation() * offset);

			// without rotation
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			this->jointPosition = (this->body->getPosition() + offset);
			this->jointAlreadyCreated = false;
		}

		if (nullptr != this->debugGeometryNode)
		{
			this->debugGeometryNode->setPosition(this->jointPosition);
		}
	}

	Ogre::Vector3 JointHingeComponent::getAnchorPosition(void) const
	{
		return this->anchorPosition->getVector3();
	}

	void JointHingeComponent::setPin(const Ogre::Vector3& pin)
	{
		this->pin->setValue(pin);
		if (nullptr != this->debugGeometryNode)
		{
			this->debugGeometryNode->setDirection(pin);
		}
	}

	Ogre::Vector3 JointHingeComponent::getPin(void) const
	{
		return this->pin->getVector3();
	}

	void JointHingeComponent::setLimitsEnabled(bool enableLimits)
	{
		this->enableLimits->setValue(enableLimits);
		// this->brakeForce->setVisible(true == enableLimits);
		// this->omega->setVisible(true == enableLimits);
		// this->torgue->setVisible(true == enableLimits);
		// this->maxAngleLimit->setVisible(true == enableLimits);
		// this->stiffness->setVisible(true == enableLimits);
		// this->springK->setVisible(true == enableLimits);
		// this->springD->setVisible(true == enableLimits);
	}

	bool JointHingeComponent::getLimitsEnabled(void) const
	{
		return this->enableLimits->getBool();
	}

	void JointHingeComponent::setMinMaxAngleLimit(const Ogre::Degree& minAngleLimit, const Ogre::Degree& maxAngleLimit)
	{
		this->minAngleLimit->setValue(minAngleLimit.valueDegrees());
		this->maxAngleLimit->setValue(maxAngleLimit.valueDegrees());
		OgreNewt::Hinge* hingeJoint = dynamic_cast<OgreNewt::Hinge*>(this->joint);
		if (hingeJoint && this->enableLimits->getBool())
		{
			hingeJoint->SetLimits(minAngleLimit, maxAngleLimit);
		}
	}
	
	Ogre::Degree JointHingeComponent::getMinAngleLimit(void) const
	{
		return Ogre::Degree(this->minAngleLimit->getReal());
	}
	
	Ogre::Degree JointHingeComponent::getMaxAngleLimit(void) const
	{
		return Ogre::Degree(this->maxAngleLimit->getReal());
	}
	
	void JointHingeComponent::setTorgue(Ogre::Real torgue)
	{
		this->torque->setValue(torgue);
		OgreNewt::Hinge* hingeJoint = dynamic_cast<OgreNewt::Hinge*>(this->joint);
		if (hingeJoint /*&& this->enableLimits->getBool()*/)
		{
			hingeJoint->SetTorque(this->torque->getReal());
		}
	}

	Ogre::Real JointHingeComponent::getTorgue(void) const
	{
		return this->torque->getReal();
	}

	void JointHingeComponent::setBreakForce(Ogre::Real breakForce)
	{
		this->breakForce->setValue(breakForce);
	}

	Ogre::Real JointHingeComponent::getBreakForce(void) const
	{
		return this->breakForce->getReal();
	}

	void JointHingeComponent::setBreakTorque(Ogre::Real breakTorque)
	{
		this->breakTorque->setValue(breakTorque);
	}

	Ogre::Real JointHingeComponent::getBreakTorque(void) const
	{
		return this->breakTorque->getReal();
	}

	void JointHingeComponent::setSpring(bool asSpringDamper, bool massIndependent)
	{
		this->asSpringDamper->setValue(asSpringDamper);
		this->massIndependent->setValue(massIndependent);
		OgreNewt::Hinge* hingeJoint = dynamic_cast<OgreNewt::Hinge*>(this->joint);
		if (nullptr != hingeJoint)
		{
			hingeJoint->SetSpringAndDamping(this->asSpringDamper->getBool(), massIndependent, this->springDamperRelaxation->getReal(), this->springK->getReal(), this->springD->getReal());
		}
	}

	void JointHingeComponent::setSpring(bool asSpringDamper, bool massIndependent, Ogre::Real springDamperRelaxation, Ogre::Real springK, Ogre::Real springD)
	{
		// this->springK->setVisible(true == asSpringDamper);
		// this->springD->setVisible(true == asSpringDamper);
		// this->springDamperRelaxation->setVisible(true == asSpringDamper);
		this->asSpringDamper->setValue(asSpringDamper);
		this->massIndependent->setValue(massIndependent);
		this->springDamperRelaxation->setValue(springDamperRelaxation);
		this->springK->setValue(springK);
		this->springD->setValue(springD);
		OgreNewt::Hinge* hingeJoint = dynamic_cast<OgreNewt::Hinge*>(this->joint);
		if (nullptr != hingeJoint)
		{
			hingeJoint->SetSpringAndDamping(asSpringDamper, springDamperRelaxation, springDamperRelaxation, springK, springD);
		}
	}

	std::tuple<bool, bool, Ogre::Real, Ogre::Real, Ogre::Real> JointHingeComponent::getSpring(void) const
	{
		return std::make_tuple(this->asSpringDamper->getBool(), this->massIndependent->getBool(), this->springDamperRelaxation->getReal(), this->springK->getReal(), this->springD->getReal());
	}

	void JointHingeComponent::setFriction(Ogre::Real friction)
	{
		this->friction->setValue(friction);

		if (-1.0f != this->friction->getReal())
		{
			OgreNewt::Hinge* hingeJoint = dynamic_cast<OgreNewt::Hinge*>(this->joint);
			if (nullptr != hingeJoint)
			{
				hingeJoint->SetFriction(friction);
			}
		}
	}

	Ogre::Real JointHingeComponent::getFriction(void) const
	{
		return this->friction->getReal();
	}
	
	/*******************************JointHingeActuatorComponent*******************************/

	JointHingeActuatorComponent::JointHingeActuatorComponent()
		: JointComponent(),
		round(0),
		internalDirectionChange(false),
		oppositeDir(1.0f)
	{
		// Note that also in JointComponent internalBaseInit() is called, which sets the values for JointComponent, so this is called for already existing values
		// But luckely in Variant, no new attributes are added, but the values changed via interalAdd(...) in Variant
		this->type->setReadOnly(false);
		this->type->setValue(this->getClassName());
		this->type->setReadOnly(true);
		this->type->setDescription("An object attached to a hinge actuator joint can only rotate around one dimension perpendicular to the axis it is attached to. A good a precise actuator motor.");
		
		this->anchorPosition = new Variant(JointHingeActuatorComponent::AttrAnchorPosition(), Ogre::Vector3::ZERO, this->attributes);
		this->pin = new Variant(JointHingeActuatorComponent::AttrPin(), Ogre::Vector3(0.0f, 0.0f, 1.0f), this->attributes);
		
		this->targetAngle  = new Variant(JointHingeActuatorComponent::AttrTargetAngle(), 30.0f, this->attributes);
		this->targetAngle->setDescription("Sets the target angle til the joint should be rotated. Note: When using @directionChange, @Repeat, specify the target angle the same as the @MaxAngleLimit, so that the joint will rotate backwards and forwards.");
		
		this->angleRate  = new Variant(JointHingeActuatorComponent::AttrAngleRate(), 10.0f, this->attributes);
		this->maxTorque = new Variant(JointHingeActuatorComponent::AttrMaxTorque(), -1.0f, this->attributes); // disabled by default, so that max is used in newton
		
		this->minAngleLimit = new Variant(JointHingeActuatorComponent::AttrMinAngleLimit(), -360.0f, this->attributes);
		this->maxAngleLimit = new Variant(JointHingeActuatorComponent::AttrMaxAngleLimit(), 360.0f, this->attributes);
		
		this->directionChange = new Variant(JointHingeActuatorComponent::AttrDirectionChange(), false, this->attributes);
		this->repeat = new Variant(JointHingeActuatorComponent::AttrRepeat(), false, this->attributes);

		this->asSpringDamper = new Variant(JointHingeActuatorComponent::AttrAsSpringDamper(), false, this->attributes);
		this->massIndependent = new Variant(JointHingeActuatorComponent::AttrMassIndependent(), false, this->attributes);
		this->springK = new Variant(JointHingeActuatorComponent::AttrSpringK(), 128.0f, this->attributes);
		this->springD = new Variant(JointHingeActuatorComponent::AttrSpringD(), 4.0f, this->attributes);
		this->springDamperRelaxation = new Variant(JointHingeActuatorComponent::AttrSpringDamperRelaxation(), 0.6f, this->attributes);
		this->springDamperRelaxation->setDescription("[0 - 0.99]");
		
		this->internalDirectionChange = this->directionChange->getBool();

		this->targetId->setVisible(false);

		this->massIndependent->setDescription("Is used in conjunction with spring.");
	}

	JointHingeActuatorComponent::~JointHingeActuatorComponent()
	{
		
	}

	GameObjectCompPtr JointHingeActuatorComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		JointHingeActuatorCompPtr clonedJointCompPtr(boost::make_shared<JointHingeActuatorComponent>());
		
		clonedJointCompPtr->setType(this->type->getString());
		clonedJointCompPtr->internalSetPriorId(this->id->getULong());
		clonedJointCompPtr->setPredecessorId(this->predecessorId->getULong());
		clonedJointCompPtr->setTargetId(this->targetId->getULong());
		clonedJointCompPtr->setJointPosition(this->jointPosition);
		clonedJointCompPtr->setJointRecursiveCollisionEnabled(this->jointRecursiveCollision->getBool());
		// clonedJointCompPtr->stiffness->setValue(this->stiffness->getReal()); // Do not call setter, because internally a bool flag is set, so that stiffness is always used

		clonedJointCompPtr->setAnchorPosition(this->anchorPosition->getVector3());
		clonedJointCompPtr->setPin(this->pin->getVector3());
		clonedJointCompPtr->setTargetAngle(Ogre::Degree(this->targetAngle->getReal()));
		clonedJointCompPtr->setAngleRate(Ogre::Degree(this->angleRate->getReal()));
		clonedJointCompPtr->setMinAngleLimit(Ogre::Degree(this->minAngleLimit->getReal()));
		clonedJointCompPtr->setMaxAngleLimit(Ogre::Degree(this->maxAngleLimit->getReal()));
		clonedJointCompPtr->setMaxTorque(this->maxTorque->getReal());
		clonedJointCompPtr->setDirectionChange(this->directionChange->getBool());
		clonedJointCompPtr->setRepeat(this->repeat->getBool());
		clonedJointCompPtr->setSpring(this->asSpringDamper->getBool(), this->massIndependent->getBool(), this->springDamperRelaxation->getReal(), this->springK->getReal(), this->springD->getReal());

		clonedGameObjectPtr->addComponent(clonedJointCompPtr);
		clonedJointCompPtr->setOwner(clonedGameObjectPtr);

		clonedJointCompPtr->setActivated(this->activated->getBool());

		return clonedJointCompPtr;
	}

	bool JointHingeActuatorComponent::postInit(void)
	{
		JointComponent::postInit();

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointHingeActuatorComponent] Init joint hinge actuator component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool JointHingeActuatorComponent::connect(void)
	{
		bool success = JointComponent::connect();
		this->round = 0;
		this->internalDirectionChange = this->directionChange->getBool();
		this->oppositeDir = 1.0f;

		// Set new target each time when simulation is started
		if (nullptr != this->joint)
		{
			OgreNewt::HingeActuator* hingeActuatorJoint = static_cast<OgreNewt::HingeActuator*>(this->joint);
			if (nullptr != hingeActuatorJoint)
			{
				hingeActuatorJoint->SetTargetAngle(Ogre::Degree(this->maxAngleLimit->getReal()));
			}
		}

		this->internalShowDebugData(true, 0, this->getUpdatedJointPosition(), this->pin->getVector3());

		return success;
	}

	bool JointHingeActuatorComponent::disconnect(void)
	{
		bool success = JointComponent::disconnect();
		this->round = 0;
		this->internalDirectionChange = this->directionChange->getBool();
		this->oppositeDir = 1.0f;

		this->internalShowDebugData(false, 0, this->getUpdatedJointPosition(), this->pin->getVector3());

		return success;
	}

	Ogre::String JointHingeActuatorComponent::getClassName(void) const
	{
		return "JointHingeActuatorComponent";
	}

	Ogre::String JointHingeActuatorComponent::getParentClassName(void) const
	{
		return "JointComponent";
	}

	void JointHingeActuatorComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating && true == this->activated->getBool())
		{
			bool targetAngleReached = false;

			OgreNewt::HingeActuator* hingeActuatorJoint = static_cast<OgreNewt::HingeActuator*>(this->joint);
			if (nullptr != hingeActuatorJoint)
			{
				Ogre::Real angle = hingeActuatorJoint->GetActuatorAngle();
				// Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[JointHingeActuatorComponent] Angle: " + Ogre::StringConverter::toString(angle));
				Ogre::Real step = this->angleRate->getReal() * dt;
				if (1.0f == this->oppositeDir)
				{
					if (Ogre::Math::RealEqual(angle, this->maxAngleLimit->getReal(), step))
					{
						targetAngleReached = true;
						if (true == this->internalDirectionChange)
						{
							this->oppositeDir *= -1.0f;
						}
						this->round++;
					}
				}
				else
				{
					if (Ogre::Math::RealEqual(angle, this->minAngleLimit->getReal(), step))
					{
						targetAngleReached = true;
						if (true == this->internalDirectionChange)
						{
							this->oppositeDir *= -1.0f;
						}
						this->round++;
					}
				}
			}

			if (true == targetAngleReached && this->round == 2)
			{
				Ogre::Degree newAngle = Ogre::Degree(0.0f);

				if (1.0f == this->oppositeDir)
					newAngle = Ogre::Degree(this->maxAngleLimit->getReal());
				else
					newAngle = Ogre::Degree(this->minAngleLimit->getReal());
				hingeActuatorJoint->SetTargetAngle(newAngle);

				if (this->targetAngleReachedClosureFunction.is_valid())
				{
					try
					{
						luabind::call_function<void>(this->targetAngleReachedClosureFunction, newAngle);
					}
					catch (luabind::error& error)
					{
						luabind::object errorMsg(luabind::from_stack(error.state(), -1));
						std::stringstream msg;
						msg << errorMsg;

						Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[JointHingeActuatorComponent] Caught error in 'reactOnTargetAngleReached' Error: " + Ogre::String(error.what())
																	+ " details: " + msg.str());
					}
				}
			}
		}
	}

	bool JointHingeActuatorComponent::init(xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		JointComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnchorPosition")
		{
			this->anchorPosition->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Pin")
		{
			this->pin->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetAngle")
		{
			this->targetAngle->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AngleRate")
		{
			this->angleRate->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MinAngleLimit")
		{
			this->minAngleLimit->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxAngleLimit")
		{
			this->maxAngleLimit->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxTorgue")
		{
			this->maxTorque->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DirectionChange")
		{
			this->directionChange->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Repeat")
		{
			this->repeat->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointAsSpringDamper")
		{
			this->asSpringDamper->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointMassIndependent")
		{
			this->massIndependent->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointKSpring")
		{
			this->springK->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointDSpring")
		{
			this->springD->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointSpringDamperRelaxation")
		{
			this->springDamperRelaxation->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		return true;
	}

	void JointHingeActuatorComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		JointComponent::writeXML(propertiesXML, doc, filePath);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnchorPosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->anchorPosition->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Pin"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->pin->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TargetAngle"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->targetAngle->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AngleRate"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->angleRate->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MinAngleLimit"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->minAngleLimit->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MaxAngleLimit"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxAngleLimit->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MaxTorgue"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxTorque->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DirectionChange"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->directionChange->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Repeat"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->repeat->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointAsSpringDamper"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->asSpringDamper->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointMassIndependent"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->massIndependent->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointKSpring"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->springK->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointDSpring"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->springD->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointSpringDamperRelaxation"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->springDamperRelaxation->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::Vector3 JointHingeActuatorComponent::getUpdatedJointPosition(void)
	{
		if (nullptr == this->body)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointHingeActuatorComponent] Could not get 'getUpdatedJointPosition', because there is no body for game object: " + this->gameObjectPtr->getName());
			return Ogre::Vector3::ZERO;
		}

		if (false == this->hasCustomJointPosition)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			return this->body->getPosition() + offset;
		}
		else
		{
			return this->jointPosition;
		}
	}

	bool JointHingeActuatorComponent::createJoint(const Ogre::Vector3& customJointPosition)
	{
		// Joint base created but not activated, return false, but its no error, hence after that return true.
		if (false == JointComponent::createJoint(customJointPosition))
		{
			return true;
		}

		if (true == this->jointAlreadyCreated)
		{
			// joint already created so skip
			return true;
		}
		if (nullptr == this->body)
		{
			this->connect();
		}
		if (Ogre::Vector3::ZERO == this->pin->getVector3())
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[JointHingeActuatorComponent] Cannot create joint for: " + this->gameObjectPtr->getName() + " because the pin is zero.");
			return false;
		}

		if (Ogre::Vector3::ZERO == customJointPosition)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			//Ogre::Vector3 aPos = Ogre::Vector3(-this->anchorPosition.x, -this->anchorPosition.y, -this->anchorPosition.z);
			//// relative anchor pos: 0.5 0 0 means 50% in X of the size of the parent object
			//Ogre::Vector3 offset = (aPos * size);
			//// take orientation into account, so that orientated joints are processed correctly
			//this->jointPosition = this->body->getPosition() - (this->body->getOrientation() * offset);

			// without rotation
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			this->jointPosition = (this->body->getPosition() + offset);
		}
		else
		{
			this->hasCustomJointPosition = true;
			this->jointPosition = customJointPosition;
		}

		this->round = 0;
		this->internalDirectionChange = this->directionChange->getBool();
		this->oppositeDir = 1.0f;

		OgreNewt::Body* predecessorBody = nullptr;
		if (this->predecessorJointCompPtr)
		{
			predecessorBody = this->predecessorJointCompPtr->getBody();
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[JointHingeActuatorComponent] Creating hinge actuator joint for body1 name: "
				+ this->predecessorJointCompPtr->getOwner()->getName() + " body2 name: " + this->gameObjectPtr->getName());
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointHingeActuatorComponent] Created hinge actuator joint: " + this->gameObjectPtr->getName() + " with world as parent");
		}

		// Release joint each time, to create new one with new values
		this->internalReleaseJoint();
		this->joint = new OgreNewt::HingeActuator(this->body, predecessorBody, this->jointPosition, this->body->getOrientation() * this->pin->getVector3(), 
								Ogre::Degree(this->angleRate->getReal()), Ogre::Degree(this->minAngleLimit->getReal()), Ogre::Degree(this->maxAngleLimit->getReal()));

		this->joint->setBodyMassScale(this->bodyMassScale->getVector2().x, this->bodyMassScale->getVector2().y);
		// Bad, because causing jerky behavior on ragdolls?
		this->joint->setCollisionState(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		// this->body->setJointRecursiveCollision(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		// this->applyStiffness();
// Attention: Just a test!
		// this->joint->setStiffness(0.1f);

		OgreNewt::HingeActuator* hingeActuatorJoint = dynamic_cast<OgreNewt::HingeActuator*>(this->joint);
		hingeActuatorJoint->SetSpringAndDamping(this->asSpringDamper->getBool(), this->massIndependent->getBool(), this->springDamperRelaxation->getReal(), this->springK->getReal(), this->springD->getReal());
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointHingeActuatorComponent] Created joint: " + this->gameObjectPtr->getName());

		if (0.0f < this->maxTorque->getReal())
		{
			hingeActuatorJoint->SetMaxTorque(this->maxTorque->getReal());
		}

		if (true == this->activated->getBool())
		{
			hingeActuatorJoint->SetTargetAngle(Ogre::Degree(this->targetAngle->getReal()));
		}
		
		this->setDirectionChange(this->directionChange->getBool());
		this->setRepeat(this->repeat->getBool());

		this->jointAlreadyCreated = true;
		
		return true;
	}

	void JointHingeActuatorComponent::forceShowDebugData(bool activate)
	{
		this->internalShowDebugData(activate, 0, this->getUpdatedJointPosition(), this->pin->getVector3());
	}

	void JointHingeActuatorComponent::actualizeValue(Variant* attribute)
	{
		JointComponent::actualizeValue(attribute);
		
		if (JointHingeActuatorComponent::AttrAnchorPosition() == attribute->getName())
		{
			this->setAnchorPosition(attribute->getVector3());
		}
		else if (JointHingeActuatorComponent::AttrPin() == attribute->getName())
		{
			this->setPin(attribute->getVector3());
		}
		else if (JointHingeActuatorComponent::AttrTargetAngle() == attribute->getName())
		{
			this->setTargetAngle(Ogre::Degree(attribute->getReal()));
		}
		else if (JointHingeActuatorComponent::AttrAngleRate() == attribute->getName())
		{
			this->setAngleRate(Ogre::Degree(attribute->getReal()));
		}
		else if (JointHingeActuatorComponent::AttrMinAngleLimit() == attribute->getName())
		{
			this->setMinAngleLimit(Ogre::Degree(attribute->getReal()));
		}
		else if (JointHingeActuatorComponent::AttrMaxAngleLimit() == attribute->getName())
		{
			this->setMaxAngleLimit(Ogre::Degree(attribute->getReal()));
		}
		else if (JointHingeActuatorComponent::AttrMaxTorque() == attribute->getName())
		{
			this->setMaxTorque(attribute->getReal());
		}
		else if (JointHingeActuatorComponent::AttrDirectionChange() == attribute->getName())
		{
			this->setDirectionChange(attribute->getBool());
		}
		else if (JointHingeActuatorComponent::AttrAsSpringDamper() == attribute->getName())
		{
			this->setSpring(attribute->getBool(), this->massIndependent->getBool(), this->springDamperRelaxation->getReal(), this->springK->getReal(), this->springD->getReal());
		}
		else if (JointHingeActuatorComponent::AttrMassIndependent() == attribute->getName())
		{
			this->setSpring(attribute->getBool(), attribute->getBool(), this->springDamperRelaxation->getReal(), this->springK->getReal(), this->springD->getReal());
		}
		else if (JointHingeActuatorComponent::AttrSpringDamperRelaxation() == attribute->getName())
		{
			this->setSpring(this->asSpringDamper->getBool(), this->massIndependent->getBool(), attribute->getReal(), this->springK->getReal(), this->springD->getReal());
		}
		else if (JointHingeActuatorComponent::AttrSpringK() == attribute->getName())
		{
			this->setSpring(this->asSpringDamper->getBool(), this->massIndependent->getBool(), this->springDamperRelaxation->getReal(), attribute->getReal(), this->springD->getReal());
		}
		else if (JointHingeActuatorComponent::AttrSpringD() == attribute->getName())
		{
			this->setSpring(this->asSpringDamper->getBool(), this->massIndependent->getBool(), this->springDamperRelaxation->getReal(), this->springK->getReal(), attribute->getReal());
		}
		else if (JointHingeActuatorComponent::AttrRepeat() == attribute->getName())
		{
			this->setRepeat(attribute->getBool());
		}
	}

	void JointHingeActuatorComponent::setAnchorPosition(const Ogre::Vector3& anchorPosition)
	{
		this->anchorPosition->setValue(anchorPosition);

		if (nullptr != this->body)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			//Ogre::Vector3 aPos = Ogre::Vector3(-this->anchorPosition.x, -this->anchorPosition.y, -this->anchorPosition.z);
			//// relative anchor pos: 0.5 0 0 means 50% in X of the size of the parent object
			//Ogre::Vector3 offset = (aPos * size);
			//// take orientation into account, so that orientated joints are processed correctly
			//this->jointPosition = this->body->getPosition() - (this->body->getOrientation() * offset);

			// without rotation
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			this->jointPosition = (this->body->getPosition() + offset);
			this->jointAlreadyCreated = false;
		}

		if (nullptr != this->debugGeometryNode)
		{
			this->debugGeometryNode->setPosition(this->jointPosition);
		}
	}

	Ogre::Vector3 JointHingeActuatorComponent::getAnchorPosition(void) const
	{
		return this->anchorPosition->getVector3();
	}

	void JointHingeActuatorComponent::setPin(const Ogre::Vector3& pin)
	{
		this->pin->setValue(pin);
		if (nullptr != this->debugGeometryNode)
		{
			this->debugGeometryNode->setDirection(pin);
		}
	}

	Ogre::Vector3 JointHingeActuatorComponent::getPin(void) const
	{
		return this->pin->getVector3();
	}
	
	void JointHingeActuatorComponent::setTargetAngle(const Ogre::Degree& targetAngle)
	{
		this->targetAngle->setValue(targetAngle.valueDegrees());
		OgreNewt::HingeActuator* hingeActuatorJoint = dynamic_cast<OgreNewt::HingeActuator*>(this->joint);
		if (nullptr != hingeActuatorJoint)
		{
			hingeActuatorJoint->SetTargetAngle(targetAngle);
		}
	}
	
	Ogre::Degree JointHingeActuatorComponent::getTargetAngle(void) const
	{
		return Ogre::Degree(this->targetAngle->getReal());
	}
	
	void JointHingeActuatorComponent::setAngleRate(const Ogre::Degree& angleRate)
	{
		this->angleRate->setValue(angleRate.valueDegrees());
		OgreNewt::HingeActuator* hingeActuatorJoint = dynamic_cast<OgreNewt::HingeActuator*>(this->joint);
		if (nullptr != hingeActuatorJoint)
		{
			hingeActuatorJoint->SetAngularRate(angleRate);
		}
	}
	
	Ogre::Degree JointHingeActuatorComponent::getAngleRate(void) const
	{
		return Ogre::Degree(this->angleRate->getReal());
	}
	
	void JointHingeActuatorComponent::setMinAngleLimit(const Ogre::Degree& minAngleLimit)
	{
		this->minAngleLimit->setValue(minAngleLimit.valueDegrees());
		OgreNewt::HingeActuator* hingeActuatorJoint = dynamic_cast<OgreNewt::HingeActuator*>(this->joint);
		if (nullptr != hingeActuatorJoint)
		{
			hingeActuatorJoint->SetMinAngularLimit(minAngleLimit);
		}
	}

	Ogre::Degree JointHingeActuatorComponent::getMinAngleLimit(void) const
	{
		return Ogre::Degree(this->minAngleLimit->getReal());
	}
	
	void JointHingeActuatorComponent::setMaxAngleLimit(const Ogre::Degree& maxAngleLimit)
	{
		this->maxAngleLimit->setValue(maxAngleLimit.valueDegrees());
		OgreNewt::HingeActuator* hingeActuatorJoint = dynamic_cast<OgreNewt::HingeActuator*>(this->joint);
		if (nullptr != hingeActuatorJoint)
		{
			hingeActuatorJoint->SetMaxAngularLimit(maxAngleLimit);
		}
	}

	Ogre::Degree JointHingeActuatorComponent::getMaxAngleLimit(void) const
	{
		return Ogre::Degree(this->maxAngleLimit->getReal());
	}

	void JointHingeActuatorComponent::setMaxTorque(Ogre::Real maxTorque)
	{
		if (maxTorque < 0.0f)
			return;
		
		this->maxTorque->setValue(maxTorque);
		OgreNewt::HingeActuator* hingeActuatorJoint = dynamic_cast<OgreNewt::HingeActuator*>(this->joint);
		if (hingeActuatorJoint)
		{
			hingeActuatorJoint->SetMaxTorque(maxTorque);
		}
	}

	Ogre::Real JointHingeActuatorComponent::getMaxTorque(void) const
	{
		return this->maxTorque->getReal();
	}
	
	void JointHingeActuatorComponent::setDirectionChange(bool directionChange)
	{
		this->directionChange->setValue(directionChange);
		this->internalDirectionChange = directionChange;
	}
		
	bool JointHingeActuatorComponent::getDirectionChange(void) const
	{
		return this->directionChange->getBool();
	}
	
	void JointHingeActuatorComponent::setRepeat(bool repeat)
	{
		this->repeat->setValue(repeat);
	}
	
	bool JointHingeActuatorComponent::getRepeat(void) const
	{
		return this->repeat->getBool();
	}

	void JointHingeActuatorComponent::setSpring(bool asSpringDamper, bool massIndependent)
	{
		this->asSpringDamper->setValue(asSpringDamper);
		this->massIndependent->setValue(massIndependent);
		OgreNewt::HingeActuator* hingeActuatorJoint = dynamic_cast<OgreNewt::HingeActuator*>(this->joint);
		if (nullptr != hingeActuatorJoint)
		{
			hingeActuatorJoint->SetSpringAndDamping(asSpringDamper, massIndependent, this->springDamperRelaxation->getBool(), this->springK->getReal(), this->springD->getReal());
		}
	}

	void JointHingeActuatorComponent::setSpring(bool asSpringDamper, bool massIndependent, Ogre::Real springDamperRelaxation, Ogre::Real springK, Ogre::Real springD)
	{
		// this->springK->setVisible(true == asSpringDamper);
		// this->springD->setVisible(true == asSpringDamper);
		// this->springDamperRelaxation->setVisible(true == asSpringDamper);
		this->asSpringDamper->setValue(asSpringDamper);
		this->massIndependent->setValue(massIndependent);
		this->springDamperRelaxation->setValue(springDamperRelaxation);
		this->springK->setValue(springK);
		this->springD->setValue(springD);
		OgreNewt::HingeActuator* hingeActuatorJoint = dynamic_cast<OgreNewt::HingeActuator*>(this->joint);
		if (nullptr != hingeActuatorJoint)
		{
			hingeActuatorJoint->SetSpringAndDamping(asSpringDamper, massIndependent, springDamperRelaxation, springK, springD);
		}
	}

	std::tuple<bool, bool, Ogre::Real, Ogre::Real, Ogre::Real> JointHingeActuatorComponent::getSpring(void) const
	{
		return std::make_tuple(this->asSpringDamper->getBool(), this->massIndependent->getBool(), this->springDamperRelaxation->getReal(), this->springK->getReal(), this->springD->getReal());
	}

	void JointHingeActuatorComponent::reactOnTargetAngleReached(luabind::object closureFunction)
	{
		this->targetAngleReachedClosureFunction = closureFunction;
	}
	
	/*******************************JointBallAndSocketComponent*******************************/

	JointBallAndSocketComponent::JointBallAndSocketComponent()
		: JointComponent()
	{
		// Note that also in JointComponent internalBaseInit() is called, which sets the values for JointComponent, so this is called for already existing values
		// But luckely in Variant, no new attributes are added, but the values changed via interalAdd(...) in Variant
		this->type->setReadOnly(false);
		this->type->setValue(this->getClassName());
		this->type->setReadOnly(true);
		this->type->setDescription("This joint type has an anchor (the end of blue sphere) and a moveable body that is 'hanging' by this anchor (the beginning of the green sphere)."
			"The movement of that body is free to all directions, but limited to a certain degree(specified in angles), so that the child body's movement is limited to a cone-shped area."
			"A good example for such a joint is your wrist angle or your hip joint.");

		this->anchorPosition = new Variant(JointBallAndSocketComponent::AttrAnchorPosition(), Ogre::Vector3::ZERO, this->attributes);
		this->enableTwistLimits = new Variant(JointBallAndSocketComponent::AttrEnableTwistLimits(), false, this->attributes);
		this->enableConeLimits = new Variant(JointBallAndSocketComponent::AttrEnableConeLimits(), false, this->attributes);
		this->minAngleLimit = new Variant(JointBallAndSocketComponent::AttrMinAngleLimit(), 0.0f, this->attributes);
		this->maxAngleLimit = new Variant(JointBallAndSocketComponent::AttrMaxAngleLimit(), 0.0f, this->attributes);
		this->maxConeLimit = new Variant(JointBallAndSocketComponent::AttrMaxConeLimit(), 0.0f, this->attributes);
		this->twistFriction = new Variant(JointBallAndSocketComponent::AttrTwistFriction(), 0.0f, this->attributes);
		this->coneFriction = new Variant(JointBallAndSocketComponent::AttrConeFriction(), 0.0f, this->attributes);

		this->targetId->setVisible(false);
		// this->minAngleLimit->setVisible(false);
		// this->maxAngleLimit->setVisible(false);
		// this->maxConeLimit->setVisible(false);
	}

	JointBallAndSocketComponent::~JointBallAndSocketComponent()
	{

	}

	GameObjectCompPtr JointBallAndSocketComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		JointBallAndSocketCompPtr clonedJointCompPtr(boost::make_shared<JointBallAndSocketComponent>());
		
		clonedJointCompPtr->setType(this->type->getString());
		clonedJointCompPtr->internalSetPriorId(this->id->getULong());
		clonedJointCompPtr->setPredecessorId(this->predecessorId->getULong());
		clonedJointCompPtr->setTargetId(this->targetId->getULong());
		clonedJointCompPtr->setJointPosition(this->jointPosition);
		clonedJointCompPtr->setJointRecursiveCollisionEnabled(this->jointRecursiveCollision->getBool());
		clonedJointCompPtr->stiffness->setValue(this->stiffness->getReal());

		clonedJointCompPtr->setAnchorPosition(this->anchorPosition->getVector3());
		clonedJointCompPtr->setTwistLimitsEnabled(this->enableTwistLimits->getBool());
		clonedJointCompPtr->setConeLimitsEnabled(this->enableConeLimits->getBool());
		clonedJointCompPtr->setMinMaxConeAngleLimit(Ogre::Degree(this->minAngleLimit->getReal()), Ogre::Degree(this->maxAngleLimit->getReal()), Ogre::Degree(this->maxConeLimit->getReal()));
		clonedJointCompPtr->setTwistFriction(this->twistFriction->getReal());
		clonedJointCompPtr->setConeFriction(this->coneFriction->getReal());

		clonedGameObjectPtr->addComponent(clonedJointCompPtr);
		clonedJointCompPtr->setOwner(clonedGameObjectPtr);

		clonedJointCompPtr->setActivated(this->activated->getBool());

		return clonedJointCompPtr;
	}

	bool JointBallAndSocketComponent::init(xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		JointComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointAnchorPosition")
		{
			this->anchorPosition->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(0.0f, 0.0f, 0.0f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TwistLimits")
		{
			this->setTwistLimitsEnabled(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ConeLimits")
		{
			this->setConeLimitsEnabled(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointMaxConeAngle")
		{
			this->maxConeLimit->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointMinAngleLimit")
		{
			this->minAngleLimit->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointMaxAngleLimit")
		{
			this->maxAngleLimit->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TwistFriction")
		{
			this->twistFriction->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0.0f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ConeFriction")
		{
			this->coneFriction->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0.0f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	bool JointBallAndSocketComponent::postInit(void)
	{
		JointComponent::postInit();

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointBallAndSocketComponent] Init joint ball and socket component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool JointBallAndSocketComponent::connect(void)
	{
		bool success = JointComponent::connect();

		this->internalShowDebugData(true, 1, this->getUpdatedJointPosition(), Ogre::Vector3::ZERO);

		return success;
	}

	bool JointBallAndSocketComponent::disconnect(void)
	{
		bool success = JointComponent::disconnect();

		this->internalShowDebugData(false, 1, this->getUpdatedJointPosition(), Ogre::Vector3::ZERO);

		return success;
	}

	Ogre::String JointBallAndSocketComponent::getClassName(void) const
	{
		return "JointBallAndSocketComponent";
	}

	Ogre::String JointBallAndSocketComponent::getParentClassName(void) const
	{
		return "JointComponent";
	}

	void JointBallAndSocketComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		JointComponent::writeXML(propertiesXML, doc, filePath);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointAnchorPosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->anchorPosition->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TwistLimits"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->enableTwistLimits->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ConeLimits"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->enableConeLimits->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointMaxConeAngle"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxConeLimit->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointMinAngleLimit"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->minAngleLimit->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointMaxAngleLimit"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxAngleLimit->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TwistFriction"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->twistFriction->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ConeFriction"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->coneFriction->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::Vector3 JointBallAndSocketComponent::getUpdatedJointPosition(void)
	{
		if (nullptr == this->body)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointBallAndSocketComponent] Could not get 'getUpdatedJointPosition', because there is no body for game object: " + this->gameObjectPtr->getName());
			return Ogre::Vector3::ZERO;
		}

		if (false == this->hasCustomJointPosition)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			return this->body->getPosition() + offset;
		}
		else
		{
			return this->jointPosition;
		}
	}

	void JointBallAndSocketComponent::update(Ogre::Real dt, bool notSimulating)
	{
		
	}

	bool JointBallAndSocketComponent::createJoint(const Ogre::Vector3& customJointPosition)
	{
		// Joint base created but not activated, return false, but its no error, hence after that return true.
		if (false == JointComponent::createJoint(customJointPosition))
		{
			return true;
		}

		if (true == this->jointAlreadyCreated)
		{
			// joint already created so skip
			return true;
		}
		if (nullptr == this->body)
		{
			this->connect();
		}

		if (Ogre::Vector3::ZERO == customJointPosition)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			//Ogre::Vector3 aPos = Ogre::Vector3(-this->anchorPosition.x, -this->anchorPosition.y, -this->anchorPosition.z);
			//// relative anchor pos: 0.5 0 0 means 50% in X of the size of the parent object
			//Ogre::Vector3 offset = (aPos * size);
			//// take orientation into account, so that orientated joints are processed correctly
			//this->jointPosition = this->body->getPosition() - (this->body->getOrientation() * offset);

			// without rotation
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			this->jointPosition = (this->body->getPosition() + offset);
		}
		else
		{
			this->hasCustomJointPosition = true;
			this->jointPosition = customJointPosition;
		}
		OgreNewt::Body* predecessorBody = nullptr;
		if (this->predecessorJointCompPtr)
		{
			predecessorBody = this->predecessorJointCompPtr->getBody();
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[JointBallAndSocketComponent] Creating ball and socket joint for body1 name: "
				+ this->predecessorJointCompPtr->getOwner()->getName() + " body2 name: " + this->gameObjectPtr->getName());
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointBallAndSocketComponent] Created ball and socket joint: " + this->gameObjectPtr->getName() + " with world as parent");
		}

		// Release joint each time, to create new one with new values
		this->internalReleaseJoint();
		this->joint = new OgreNewt::BallAndSocket(this->body, predecessorBody, this->jointPosition);
// Attention: Just a test!
		// this->joint->setStiffness(0.1f);
		OgreNewt::BallAndSocket* ballAndSocketJoint = dynamic_cast<OgreNewt::BallAndSocket*>(this->joint);
		
		this->joint->setBodyMassScale(this->bodyMassScale->getVector2().x, this->bodyMassScale->getVector2().y);
		this->joint->setCollisionState(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		// this->body->setJointRecursiveCollision(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		// this->applyStiffness();

		ballAndSocketJoint->enableTwist(this->enableTwistLimits->getBool());
		ballAndSocketJoint->enableCone(this->enableConeLimits->getBool());

		if (true == this->enableTwistLimits->getBool())
		{
			/*if (this->minAngleLimit == this->maxAngleLimit)
			{
				throw Ogre::Exception(Ogre::Exception::ERR_INVALIDPARAMS, "[PhysicsComponent] The ball and socket joint cannot have the same min angle limit as max angle limit!\n", "NOWA");
			}*/

			ballAndSocketJoint->setTwistLimits(Ogre::Degree(this->minAngleLimit->getReal()), Ogre::Degree(this->maxAngleLimit->getReal()));
		}
		if (true == this->enableConeLimits->getBool())
		{
			ballAndSocketJoint->setConeLimits(Ogre::Degree(this->maxConeLimit->getReal()));
		}
		if (0.0f != this->twistFriction->getReal())
			ballAndSocketJoint->setTwistFriction(this->twistFriction->getReal());
		if (0.0f != this->coneFriction->getReal())
			ballAndSocketJoint->setConeFriction(this->coneFriction->getReal());

		this->jointAlreadyCreated = true;

		return true;
	}

	void JointBallAndSocketComponent::forceShowDebugData(bool activate)
	{
		this->internalShowDebugData(activate, 1, this->getUpdatedJointPosition(), Ogre::Vector3::ZERO);
	}

	void JointBallAndSocketComponent::actualizeValue(Variant* attribute)
	{
		JointComponent::actualizeValue(attribute);

		if (JointBallAndSocketComponent::AttrAnchorPosition() == attribute->getName())
		{
			this->setAnchorPosition(attribute->getVector3());
		}
		else if (JointBallAndSocketComponent::AttrEnableTwistLimits() == attribute->getName())
		{
			this->setTwistLimitsEnabled(attribute->getBool());
		}
		else if (JointBallAndSocketComponent::AttrMinAngleLimit() == attribute->getName())
		{
			this->setMinMaxConeAngleLimit(Ogre::Degree(attribute->getReal()), Ogre::Degree(this->maxAngleLimit->getReal()), Ogre::Degree(this->maxConeLimit->getReal()));
		}
		else if (JointBallAndSocketComponent::AttrMaxAngleLimit() == attribute->getName())
		{
			this->setMinMaxConeAngleLimit(Ogre::Degree(this->minAngleLimit->getReal()), Ogre::Degree(attribute->getReal()), Ogre::Degree(this->maxConeLimit->getReal()));
		}
		else if (JointBallAndSocketComponent::AttrEnableConeLimits() == attribute->getName())
		{
			this->setConeLimitsEnabled(attribute->getBool());
		}
		else if (JointBallAndSocketComponent::AttrMaxConeLimit() == attribute->getName())
		{
			this->setMinMaxConeAngleLimit(Ogre::Degree(this->minAngleLimit->getReal()), Ogre::Degree(this->maxAngleLimit->getReal()), Ogre::Degree(attribute->getReal()));
		}
		else if (JointBallAndSocketComponent::AttrTwistFriction() == attribute->getName())
		{
			this->setTwistFriction(attribute->getReal());
		}
		else if (JointBallAndSocketComponent::AttrConeFriction() == attribute->getName())
		{
			this->setConeFriction(attribute->getReal());
		}
	}

	void JointBallAndSocketComponent::setAnchorPosition(const Ogre::Vector3& anchorPosition)
	{
		this->anchorPosition->setValue(anchorPosition);

		if (nullptr != this->body)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			//Ogre::Vector3 aPos = Ogre::Vector3(-this->anchorPosition.x, -this->anchorPosition.y, -this->anchorPosition.z);
			//// relative anchor pos: 0.5 0 0 means 50% in X of the size of the parent object
			//Ogre::Vector3 offset = (aPos * size);
			//// take orientation into account, so that orientated joints are processed correctly
			//this->jointPosition = this->body->getPosition() - (this->body->getOrientation() * offset);

			// without rotation
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			this->jointPosition = (this->body->getPosition() + offset);
			this->jointAlreadyCreated = false;
		}

		if (nullptr != this->debugGeometryNode)
		{
			this->debugGeometryNode->setPosition(this->jointPosition);
		}
	}

	Ogre::Vector3 JointBallAndSocketComponent::getAnchorPosition(void) const
	{
		return this->anchorPosition->getVector3();
	}

	void JointBallAndSocketComponent::setTwistLimitsEnabled(bool enableLimits)
	{
		this->enableTwistLimits->setValue(enableLimits);

		OgreNewt::BallAndSocket* ballAndSocketJoint = dynamic_cast<OgreNewt::BallAndSocket*>(this->joint);
		if (nullptr != ballAndSocketJoint)
		{
			ballAndSocketJoint->enableTwist(enableLimits);
		}

		// this->minAngleLimit->setVisible(true == enableLimits);
		// this->maxAngleLimit->setVisible(true == enableLimits);
		// this->maxConeLimit->setVisible(true == enableLimits);
	}

	bool JointBallAndSocketComponent::getTwistLimitsEnabled(void) const
	{
		return this->enableTwistLimits->getBool();
	}

	void JointBallAndSocketComponent::setConeLimitsEnabled(bool enableLimits)
	{
		this->enableConeLimits->setValue(enableLimits);
		OgreNewt::BallAndSocket* ballAndSocketJoint = dynamic_cast<OgreNewt::BallAndSocket*>(this->joint);
		if (nullptr != ballAndSocketJoint)
		{
			ballAndSocketJoint->enableCone(enableLimits);
		}

		// this->minAngleLimit->setVisible(true == enableLimits);
		// this->maxAngleLimit->setVisible(true == enableLimits);
		// this->maxConeLimit->setVisible(true == enableLimits);
	}

	bool JointBallAndSocketComponent::getConeLimitsEnabled(void) const
	{
		return this->enableConeLimits->getBool();
	}

	void JointBallAndSocketComponent::setMinMaxConeAngleLimit(const Ogre::Degree& minAngleLimit, const Ogre::Degree& maxAngleLimit, const Ogre::Degree& maxConeLimit)
	{
		this->minAngleLimit->setValue(minAngleLimit.valueDegrees());
		this->maxAngleLimit->setValue(maxAngleLimit.valueDegrees());
		this->maxConeLimit->setValue(maxConeLimit.valueDegrees());

		OgreNewt::BallAndSocket* ballAndSocketJoint = dynamic_cast<OgreNewt::BallAndSocket*>(this->joint);
		if (nullptr != ballAndSocketJoint)
		{
			if (true == this->enableTwistLimits->getBool())
				ballAndSocketJoint->setTwistLimits(Ogre::Degree(this->minAngleLimit->getReal()), Ogre::Degree(this->maxAngleLimit->getReal()));

			if (true == this->enableConeLimits->getBool())
				ballAndSocketJoint->setConeLimits(Ogre::Degree(this->maxConeLimit->getReal()));
		}
	}
	
	Ogre::Degree JointBallAndSocketComponent::getMinAngleLimit(void) const
	{
		return Ogre::Degree(this->minAngleLimit->getReal());
	}
	
	Ogre::Degree JointBallAndSocketComponent::getMaxAngleLimit(void) const
	{
		return Ogre::Degree(this->maxAngleLimit->getReal());
	}
	
	Ogre::Degree JointBallAndSocketComponent::getMaxConeLimit(void) const
	{
		return Ogre::Degree(this->maxConeLimit->getReal());
	}
	
	void JointBallAndSocketComponent::setTwistFriction(Ogre::Real twistFriction)
	{
		this->twistFriction->setValue(twistFriction);
	}

	Ogre::Real JointBallAndSocketComponent::getTwistFriction(void) const
	{
		return this->twistFriction->getReal();
	}

	void JointBallAndSocketComponent::setConeFriction(Ogre::Real coneFriction)
	{
		this->coneFriction->setValue(coneFriction);
	}

	Ogre::Real JointBallAndSocketComponent::getConeFriction(void) const
	{
		return this->coneFriction->getReal();
	}

	/*******************************JointControlledBallAndSocketComponent*******************************/

	//JointControlledBallAndSocketComponent::JointControlledBallAndSocketComponent()
	//	: JointComponent()
	//{
	//	// Note that also in JointComponent internalBaseInit() is called, which sets the values for JointComponent, so this is called for already existing values
	//	// But luckely in Variant, no new attributes are added, but the values changed via interalAdd(...) in Variant
	//	this->type->setReadOnly(false);
	//	this->type->setValue(this->getClassName());
	//	this->type->setReadOnly(true);
	//	this->type->setDescription("This joint type has an anchor (the end of blue sphere) and a moveable body that is 'hanging' by this anchor (the beginning of the green sphere)."
	//		"The movement of that body is free to all directions, but limited to a certain degree(specified in angles), so that the child body's movement is limited to a cone-shped area."
	//		"A good example for such a joint is your wrist angle or your hip joint.");

	//	this->anchorPosition = new Variant(JointControlledBallAndSocketComponent::AttrAnchorPosition(), Ogre::Vector3::ZERO, this->attributes);
	//	this->angularVelocity = new Variant(JointControlledBallAndSocketComponent::AttrAngleVelocity(), 1.0f, this->attributes);
	//	this->pitchAngle = new Variant(JointControlledBallAndSocketComponent::AttrPitchAngle(), 0.0f, this->attributes);
	//	this->yawAngle = new Variant(JointControlledBallAndSocketComponent::AttrYawAngle(), 0.0f, this->attributes);
	//	this->rollAngle = new Variant(JointControlledBallAndSocketComponent::AttrRollAngle(), 0.0f, this->attributes);

	//	this->targetId->setVisible(false);
	//}

	//JointControlledBallAndSocketComponent::~JointControlledBallAndSocketComponent()
	//{

	//}

	//GameObjectCompPtr JointControlledBallAndSocketComponent::clone(GameObjectPtr clonedGameObjectPtr)
	//{
	//	JointControlledBallAndSocketCompPtr clonedJointCompPtr(boost::make_shared<JointControlledBallAndSocketComponent>());
	//	clonedJointCompPtr->setType(this->type->getString());
	//	clonedJointCompPtr->internalSetPriorId(this->id->getULong());
	//	clonedJointCompPtr->setPredecessorId(this->predecessorId->getULong());
	//	clonedJointCompPtr->setTargetId(this->targetId->getULong());
	//	clonedJointCompPtr->setJointPosition(this->jointPosition);
	//	clonedJointCompPtr->setJointRecursiveCollisionEnabled(this->jointRecursiveCollision->getBool());
	//	clonedJointCompPtr->stiffness->setValue(this->stiffness->getReal());

	//	clonedJointCompPtr->setAnchorPosition(this->anchorPosition->getVector3());
	//	clonedJointCompPtr->setAngleVelocity(this->angularVelocity->getReal());
	//	clonedJointCompPtr->setPitchYawRollAngle(Ogre::Degree(this->pitchAngle->getReal()), Ogre::Degree(this->yawAngle->getReal()), Ogre::Degree(this->rollAngle->getReal()));

	//	clonedGameObjectPtr->addComponent(clonedJointCompPtr);
	//	clonedJointCompPtr->setOwner(clonedGameObjectPtr);

	//	clonedJointCompPtr->setActivated(this->activated->getBool());

	//	return clonedJointCompPtr;
	//}

	//bool JointControlledBallAndSocketComponent::init(xml_node<>*& propertyElement, const Ogre::String& filename)
	//{
	//	JointComponent::init(propertyElement, filename);
	//	
	//	if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnchorPosition")
	//	{
	//		this->anchorPosition->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(0.0f, 0.0f, 0.0f)));
	//		propertyElement = propertyElement->next_sibling("property");
	//	}
	//	if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AngleVelocity")
	//	{
	//		this->angularVelocity->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0.0f)));
	//		propertyElement = propertyElement->next_sibling("property");
	//	}
	//	if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "PitchAngle")
	//	{
	//		this->pitchAngle->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
	//		propertyElement = propertyElement->next_sibling("property");
	//	}
	//	if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "YawAngle")
	//	{
	//		this->yawAngle->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
	//		propertyElement = propertyElement->next_sibling("property");
	//	}
	//	if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RollAngle")
	//	{
	//		this->rollAngle->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
	//		propertyElement = propertyElement->next_sibling("property");
	//	}
	//	return true;
	//}

	//bool JointControlledBallAndSocketComponent::postInit(void)
	//{
	//  JointComponent::postInit();

	// Component must be dynamic, because it will be moved
	// this->gameObjectPtr->setDynamic(true);
	// this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
	//	Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointBallAndSocketComponent] Init joint controlled ball and socket component for game object: " + this->gameObjectPtr->getName());

	//	return true;
	//}

	//bool JointControlledBallAndSocketComponent::connect(void)
	//{
	//	bool success = JointComponent::connect();
	//	return success;
	//}

	//bool JointControlledBallAndSocketComponent::disconnect(void)
	//{
	//	bool success = JointComponent::disconnect();
	//	return success;
	//}

	//Ogre::String JointControlledBallAndSocketComponent::getClassName(void) const
	//{
	//	return "JointControlledBallAndSocketComponent";
	//}

	//Ogre::String JointControlledBallAndSocketComponent::getParentClassName(void) const
	//{
	//	return "JointComponent";
	//}

	//void JointControlledBallAndSocketComponent::update(Ogre::Real dt, bool notSimulating)
	//{

	//}

	//void JointControlledBallAndSocketComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	//{
	//	JointComponent::writeXML(propertiesXML, doc, filePath);

	//	// 2 = int
	//	// 6 = real
	//	// 7 = string
	//	// 8 = vector2
	//	// 9 = vector3
	//	// 10 = vector4 -> also quaternion
	//	// 12 = bool
	//	xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
	//	propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
	//	propertyXML->append_attribute(doc.allocate_attribute("name", "AnchorPosition"));
	//	propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->anchorPosition->getVector3())));
	//	propertiesXML->append_node(propertyXML);

	//	propertyXML = doc.allocate_node(node_element, "property");
	//	propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
	//	propertyXML->append_attribute(doc.allocate_attribute("name", "AngleVelocity"));
	//	propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->angularVelocity->getReal())));
	//	propertiesXML->append_node(propertyXML);

	//	propertyXML = doc.allocate_node(node_element, "property");
	//	propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
	//	propertyXML->append_attribute(doc.allocate_attribute("name", "PitchAngle"));
	//	propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->pitchAngle->getReal())));
	//	propertiesXML->append_node(propertyXML);

	//	propertyXML = doc.allocate_node(node_element, "property");
	//	propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
	//	propertyXML->append_attribute(doc.allocate_attribute("name", "YawAngle"));
	//	propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->yawAngle->getReal())));
	//	propertiesXML->append_node(propertyXML);

	//	propertyXML = doc.allocate_node(node_element, "property");
	//	propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
	//	propertyXML->append_attribute(doc.allocate_attribute("name", "RollAngle"));
	//	propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rollAngle->getReal())));
	//	propertiesXML->append_node(propertyXML);
	//}

	//bool JointControlledBallAndSocketComponent::createJoint(const Ogre::Vector3& customJointPosition)
	//{
	// // Joint base created but not activated, return false, but its no error, hence after that return true.
	// if (false == JointComponent::createJoint(customJointPosition))
	// {
	// 	return true;
	// }
	//if (true == this->jointAlreadyCreated)
	//{
	//	// joint already created so skip
	//	return true;
	//}
	//	if (nullptr == this->body)
	//		{
	//		this->connect();
	//		}
	//	if (Ogre::Vector3::ZERO == customJointPosition)
	//	{
	//		Ogre::Vector3 size = this->body->getAABB().getSize();
	//		//Ogre::Vector3 aPos = Ogre::Vector3(-this->anchorPosition.x, -this->anchorPosition.y, -this->anchorPosition.z);
	//		//// relative anchor pos: 0.5 0 0 means 50% in X of the size of the parent object
	//		//Ogre::Vector3 offset = (aPos * size);
	//		//// take orientation into account, so that orientated joints are processed correctly
	//		//this->jointPosition = this->body->getPosition() - (this->body->getOrientation() * offset);

	//		// without rotation
	//		Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
	//		this->jointPosition = (this->body->getPosition() + offset);
	//	}
	//	else
	//	{
	//		this->hasCustomJointPosition = true;
	//		this->jointPosition = customJointPosition;
	//	}
	//	OgreNewt::Body* predecessorBody = nullptr;
	//	if (this->predecessorJointCompPtr)
	//	{
	//		predecessorBody = this->predecessorJointCompPtr->getBody();
	//		Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[JointControlledBallAndSocketComponent] Creating ball and socket joint for body1 name: "
	//			+ this->predecessorJointCompPtr->getOwner()->getName() + " body2 name: " + this->gameObjectPtr->getName());
	//	}
	//	else
	//	{
	//		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointControlledBallAndSocketComponent] Created ball and socket joint: " + this->gameObjectPtr->getName() + " with world as parent");
	//	}

	//	// Release joint each time, to create new one with new values
		// this->internalReleaseJoint();
	//	this->joint = new OgreNewt::ControlledBallAndSocket(this->body, predecessorBody, this->jointPosition);

	//	OgreNewt::ControlledBallAndSocket* controlledBallAndSocket = dynamic_cast<OgreNewt::ControlledBallAndSocket*>(this->joint);
	//	
	// this->joint->setBodyMassScale(this->bodyMassScale->getVector2().x, this->bodyMassScale->getVector2().y);
	//	this->joint->setCollisionState(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
	////	this->body->setJointRecursiveCollision(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
	// this->applyStiffness();
	//	
	//	controlledBallAndSocket->setAngleVelocity(this->angularVelocity->getReal());
	//	controlledBallAndSocket->setPitchYawRollAngle(Ogre::Degree(this->pitchAngle->getReal()), Ogre::Degree(this->yawAngle->getReal()), Ogre::Degree(this->rollAngle->getReal()));

	//  this->jointAlreadyCreated = true;
	//	return true;
	//}

	//void JointControlledBallAndSocketComponent::actualizeValue(Variant* attribute)
	//{
	//	JointComponent::actualizeValue(attribute);

	//	if (JointControlledBallAndSocketComponent::AttrAnchorPosition() == attribute->getName())
	//	{
	//		this->setAnchorPosition(attribute->getVector3());
	//	}
	//	else if (JointControlledBallAndSocketComponent::AttrAngleVelocity() == attribute->getName())
	//	{
	//		this->setAngleVelocity(attribute->getReal());
	//	}
	//	else if (JointControlledBallAndSocketComponent::AttrPitchAngle() == attribute->getName())
	//	{
	//		this->setPitchYawRollAngle(Ogre::Degree(attribute->getReal()), Ogre::Degree(this->yawAngle->getReal()), Ogre::Degree(this->rollAngle->getReal()));
	//	}
	//	else if (JointControlledBallAndSocketComponent::AttrYawAngle() == attribute->getName())
	//	{
	//		this->setPitchYawRollAngle(Ogre::Degree(this->pitchAngle->getReal()), Ogre::Degree(attribute->getReal()), Ogre::Degree(this->rollAngle->getReal()));
	//	}
	//	else if (JointControlledBallAndSocketComponent::AttrRollAngle() == attribute->getName())
	//	{
	//		this->setPitchYawRollAngle(Ogre::Degree(this->pitchAngle->getReal()), Ogre::Degree(this->yawAngle->getReal()), Ogre::Degree(attribute->getReal()));
	//	}
	//}

	//void JointControlledBallAndSocketComponent::setAnchorPosition(const Ogre::Vector3& anchorPosition)
	//{
	//	this->anchorPosition->setValue(anchorPosition);
	//	if (nullptr != this->debugGeometryNode)
	//	{
	//		this->debugGeometryNode->setPosition(anchorPosition);
	//	}
	//this->jointAlreadyCreated = false;
	//}

	//Ogre::Vector3 JointControlledBallAndSocketComponent::getAnchorPosition(void) const
	//{
	//	return this->anchorPosition->getVector3();
	//}

	//void JointControlledBallAndSocketComponent::setPitchYawRollAngle(const Ogre::Degree& pitchAngle, const Ogre::Degree& yawAngle, const Ogre::Degree& rollAngle)
	//{
	//	this->pitchAngle->setValue(pitchAngle.valueDegrees());
	//	this->yawAngle->setValue(yawAngle.valueDegrees());
	//	this->rollAngle->setValue(rollAngle.valueDegrees());

	//	OgreNewt::ControlledBallAndSocket* controlledBallAndSocket = dynamic_cast<OgreNewt::ControlledBallAndSocket*>(this->joint);

	//	if (nullptr != controlledBallAndSocket)
	//	{
	//		controlledBallAndSocket->setPitchYawRollAngle(Ogre::Degree(pitchAngle), Ogre::Degree(yawAngle), Ogre::Degree(rollAngle));
	//	}
	//}

	//std::tuple<Ogre::Degree, Ogre::Degree, Ogre::Degree> JointControlledBallAndSocketComponent::getPitchYawRollAngle(void) const
	//{
	//	return std::make_tuple(Ogre::Degree(this->pitchAngle->getReal()), Ogre::Degree(this->yawAngle->getReal()), Ogre::Degree(this->rollAngle->getReal()));
	//}

	//void JointControlledBallAndSocketComponent::setAngleVelocity(Ogre::Real angularVelocity)
	//{
	//	this->angularVelocity->setValue(angularVelocity);
	//	OgreNewt::ControlledBallAndSocket* controlledBallAndSocket = dynamic_cast<OgreNewt::ControlledBallAndSocket*>(this->joint);

	//	if (nullptr != controlledBallAndSocket)
	//	{
	//		controlledBallAndSocket->setAngleVelocity(angularVelocity);
	//	}
	//}

	//Ogre::Real JointControlledBallAndSocketComponent::getAngleVelocity(void) const
	//{
	//	return this->angularVelocity->getReal();
	//}
	
	/*******************************JointPointToPointComponent*******************************/

	JointPointToPointComponent::JointPointToPointComponent()
		: JointComponent(),
		jointPosition2(Ogre::Vector3::ZERO)
	{
		// Note that also in JointComponent internalBaseInit() is called, which sets the values for JointComponent, so this is called for already existing values
		// But luckely in Variant, no new attributes are added, but the values changed via interalAdd(...) in Variant
		this->type->setReadOnly(false);
		this->type->setValue(this->getClassName());
		this->type->setReadOnly(true);
		this->type->setDescription("Similiar to Ball and Socket, but must be composed of two physics active components. Behavior like snake is possible.");

		this->anchorPosition1 = new Variant(JointPointToPointComponent::AttrAnchorPosition1(), Ogre::Vector3::ZERO, this->attributes);
		this->anchorPosition2 = new Variant(JointPointToPointComponent::AttrAnchorPosition2(), Ogre::Vector3::ZERO, this->attributes);

		this->targetId->setVisible(false);
	}

	JointPointToPointComponent::~JointPointToPointComponent()
	{
		if (nullptr != this->debugGeometryNode2)
		{
			this->debugGeometryNode2->detachAllObjects();
			this->gameObjectPtr->getSceneManager()->destroySceneNode(this->debugGeometryNode2);
			this->debugGeometryNode2 = nullptr;
			this->gameObjectPtr->getSceneManager()->destroyMovableObject(this->debugGeometryEntity2);
			this->debugGeometryEntity2 = nullptr;
		}
	}

	GameObjectCompPtr JointPointToPointComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		JointPointToPointCompPtr clonedJointCompPtr(boost::make_shared<JointPointToPointComponent>());
		
		clonedJointCompPtr->setType(this->type->getString());
		clonedJointCompPtr->internalSetPriorId(this->id->getULong());
		clonedJointCompPtr->setPredecessorId(this->predecessorId->getULong());
		clonedJointCompPtr->setTargetId(this->targetId->getULong());
		clonedJointCompPtr->setJointPosition(this->jointPosition);
		clonedJointCompPtr->setJointRecursiveCollisionEnabled(this->jointRecursiveCollision->getBool());
		clonedJointCompPtr->stiffness->setValue(this->stiffness->getReal());

		clonedJointCompPtr->setAnchorPosition1(this->anchorPosition1->getVector3());
		clonedJointCompPtr->setAnchorPosition2(this->anchorPosition2->getVector3());

		clonedGameObjectPtr->addComponent(clonedJointCompPtr);
		clonedJointCompPtr->setOwner(clonedGameObjectPtr);

		clonedJointCompPtr->setActivated(this->activated->getBool());

		return clonedJointCompPtr;
	}

	bool JointPointToPointComponent::init(xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		JointComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnchorPosition1")
		{
			this->anchorPosition1->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(0.0f, 0.0f, 0.0f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnchorPosition2")
		{
			this->anchorPosition2->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(0.0f, 0.0f, 0.0f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	bool JointPointToPointComponent::postInit(void)
	{
		JointComponent::postInit();

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointPointToPointComponent] Init joint point to point component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool JointPointToPointComponent::connect(void)
	{
		bool success = JointComponent::connect();

		this->internalShowDebugData(true, 2, this->anchorPosition1->getVector3(), this->anchorPosition2->getVector3());

		return success;
	}

	bool JointPointToPointComponent::disconnect(void)
	{
		bool success = JointComponent::disconnect();

		this->internalShowDebugData(false, 2, this->anchorPosition1->getVector3(), this->anchorPosition2->getVector3());

		return success;
	}

	Ogre::String JointPointToPointComponent::getClassName(void) const
	{
		return "JointPointToPointComponent";
	}

	Ogre::String JointPointToPointComponent::getParentClassName(void) const
	{
		return "JointComponent";
	}

	void JointPointToPointComponent::update(Ogre::Real dt, bool notSimulating)
	{

	}

	void JointPointToPointComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		JointComponent::writeXML(propertiesXML, doc, filePath);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnchorPosition1"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->anchorPosition1->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnchorPosition2"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->anchorPosition2->getVector3())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::Vector3 JointPointToPointComponent::getUpdatedJointPosition(void)
	{
		if (nullptr == this->body)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointPointToPointComponent] Could not get 'getUpdatedJointPosition', because there is no body for game object: " + this->gameObjectPtr->getName());
			return Ogre::Vector3::ZERO;
		}

		if (false == this->hasCustomJointPosition)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			Ogre::Vector3 offset = (this->anchorPosition1->getVector3() * size);
			return this->body->getPosition() + offset;
		}
		else
		{
			return this->jointPosition;
		}
	}

	bool JointPointToPointComponent::createJoint(const Ogre::Vector3& customJointPosition)
	{
		// Joint base created but not activated, return false, but its no error, hence after that return true.
		if (false == JointComponent::createJoint(customJointPosition))
		{
			return true;
		}

		if (true == this->jointAlreadyCreated)
		{
			// joint already created so skip
			return true;
		}
		if (nullptr == this->body)
		{
			this->connect();
		}
		// Special type of joint, which cannot be created with world as precessor body
		if (nullptr == this->predecessorJointCompPtr)
			return true;

		if (Ogre::Vector3::ZERO == customJointPosition)
		{
			this->setAnchorPosition1(this->anchorPosition1->getVector3());
		}
		else
		{
			this->hasCustomJointPosition = true;
			this->jointPosition = customJointPosition;
		}
		
		OgreNewt::Body* predecessorBody = nullptr;
		if (nullptr != this->predecessorJointCompPtr)
		{
			predecessorBody = this->predecessorJointCompPtr->getBody();
			this->setAnchorPosition2(this->anchorPosition2->getVector3());
			
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[JointPointToPointComponent] Creating point to point joint for body1 name: "
				+ this->predecessorJointCompPtr->getOwner()->getName() + " body2 name: " + this->gameObjectPtr->getName());
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointPointToPointComponent] Created point to point joint: " + this->gameObjectPtr->getName() + " with world as parent");
		}

		// Release joint each time, to create new one with new values
		this->internalReleaseJoint();
		this->joint = new OgreNewt::PointToPoint(this->body, predecessorBody, this->jointPosition, this->jointPosition2);
		// this->applyStiffness();

		this->jointAlreadyCreated = true;

		return true;
	}

	void JointPointToPointComponent::forceShowDebugData(bool activate)
	{
		this->internalShowDebugData(activate, 2, this->anchorPosition1->getVector3(), this->anchorPosition2->getVector3());
	}

	void JointPointToPointComponent::actualizeValue(Variant* attribute)
	{
		JointComponent::actualizeValue(attribute);

		if (JointPointToPointComponent::AttrAnchorPosition1() == attribute->getName())
		{
			this->setAnchorPosition1(attribute->getVector3());
		}
		else if (JointPointToPointComponent::AttrAnchorPosition2() == attribute->getName())
		{
			this->setAnchorPosition2(attribute->getVector3());
		}
	}

	void JointPointToPointComponent::setAnchorPosition1(const Ogre::Vector3& anchorPosition1)
	{
		this->anchorPosition1->setValue(anchorPosition1);
		if (nullptr != this->debugGeometryNode)
		{
			this->debugGeometryNode->setPosition(anchorPosition1);
		}
		// Calculate here joint position for debug data
		if (nullptr != this->body)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();

			Ogre::Vector3 offset = (this->anchorPosition1->getVector3() * size);
			this->jointPosition = (this->body->getPosition() + offset);
			this->jointAlreadyCreated = false;
		}
	}

	Ogre::Vector3 JointPointToPointComponent::getAnchorPosition1(void) const
	{
		return this->anchorPosition1->getVector3();
	}
	
	void JointPointToPointComponent::setAnchorPosition2(const Ogre::Vector3& anchorPosition2)
	{
		this->anchorPosition2->setValue(anchorPosition2);

		OgreNewt::Body* predecessorBody = nullptr;
		if (nullptr != this->predecessorJointCompPtr)
		{
			OgreNewt::Body* predecessorBody = this->predecessorJointCompPtr->getBody();
			Ogre::Vector3 size = predecessorBody->getAABB().getSize();
			Ogre::Vector3 offset = (this->anchorPosition2->getVector3() * size);
			this->jointPosition2 = (predecessorBody->getPosition() + offset);
			this->jointAlreadyCreated = false;
		}
	}

	Ogre::Vector3 JointPointToPointComponent::getAnchorPosition2(void) const
	{
		return this->anchorPosition2->getVector3();
	}
	
	/*******************************RagDollMotorDofComponent*******************************/

//	RagDollMotorDofComponent::RagDollMotorDofComponent()
//		: JointComponent()
//	{
//		// Note that also in JointComponent internalBaseInit() is called, which sets the values for JointComponent, so this is called for already existing values
//		// But luckely in Variant, no new attributes are added, but the values changed via interalAdd(...) in Variant
//		this->type->setReadOnly(false);
//		this->type->setValue(this->getClassName());
//		this->type->setReadOnly(true);
//		this->type->setDescription("This joint type has an anchor (the end of blue sphere) and a moveable body that is 'hanging' by this anchor (the beginning of the green sphere)."
//			"The movement of that body is free to all directions, but limited to a certain degree(specified in angles), so that the child body's movement is limited to a cone-shped area."
//			"A good example for such a joint is your wrist angle or your hip joint.");
//
//		this->anchorPosition = new Variant(RagDollMotorDofComponent::AttrAnchorPosition(), Ogre::Vector3::ZERO, this->attributes);
//		this->minAngleLimit = new Variant(RagDollMotorDofComponent::AttrMinAngleLimit(), 0.0f, this->attributes);
//		this->maxAngleLimit = new Variant(RagDollMotorDofComponent::AttrMaxAngleLimit(), 0.0f, this->attributes);
//		this->maxConeLimit = new Variant(RagDollMotorDofComponent::AttrMaxConeLimit(), 0.0f, this->attributes);
//		this->dofCount = new Variant(RagDollMotorDofComponent::AttrDofCount(), { "1", "2", "3" }, this->attributes);
//		this->motorOn = new Variant(RagDollMotorDofComponent::AttrMotorOn(), false, this->attributes);
//		this->torque = new Variant(RagDollMotorDofComponent::AttrTorque(), 1.0f, this->attributes);
//		
//		this->dofCount->setDescription("DOF = Degree of freedom, at which axis the joint can be rotated");
//
//		this->targetId->setVisible(false);
//		// this->minAngleLimit->setVisible(false);
//		// this->maxAngleLimit->setVisible(false);
//		// this->maxConeLimit->setVisible(false);
//	}
//
//	RagDollMotorDofComponent::~RagDollMotorDofComponent()
//	{
//
//	}
//
//	GameObjectCompPtr RagDollMotorDofComponent::clone(GameObjectPtr clonedGameObjectPtr)
//	{
//		RagDollMotorDofCompPtr clonedJointCompPtr(boost::make_shared<RagDollMotorDofComponent>());
//		clonedJointCompPtr->setType(this->type->getString());
//		clonedJointCompPtr->internalSetPriorId(this->id->getULong());
//		clonedJointCompPtr->setPredecessorId(this->predecessorId->getULong());
//		clonedJointCompPtr->setTargetId(this->targetId->getULong());
//		clonedJointCompPtr->setJointPosition(this->jointPosition);
//		clonedJointCompPtr->setJointRecursiveCollisionEnabled(this->jointRecursiveCollision->getBool());
//		clonedJointCompPtr->stiffness->setValue(this->stiffness->getReal());
//
//		clonedJointCompPtr->setAnchorPosition(this->anchorPosition->getVector3());
//		clonedJointCompPtr->setMinMaxConeAngleLimit(Ogre::Degree(this->minAngleLimit->getReal()), Ogre::Degree(this->maxAngleLimit->getReal()), Ogre::Degree(this->maxConeLimit->getReal()));
//		
//		unsigned int dofCount = 1;
//		if ("2" == this->dofCount->getListSelectedValue())
//			dofCount = 2;
//		else if ("3" == this->dofCount->getListSelectedValue())
//			dofCount = 3;
//
//		clonedJointCompPtr->setDofCount(dofCount);
//		clonedJointCompPtr->setMotorOn(this->motorOn->getBool());
//		clonedJointCompPtr->setTorque(this->torque->getReal());
//
//		clonedGameObjectPtr->addComponent(clonedJointCompPtr);
//		clonedJointCompPtr->setOwner(clonedGameObjectPtr);

		//  clonedJointCompPtr->setActivated(this->activated->getBool());
//
//		return clonedJointCompPtr;
//	}
//
//	bool RagDollMotorDofComponent::init(xml_node<>*& propertyElement, const Ogre::String& filename)
//	{
//		JointComponent::init(propertyElement, filename);
//		
//		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointAnchorPosition")
//		{
//			this->anchorPosition->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(0.0f, 0.0f, 0.0f)));
//			propertyElement = propertyElement->next_sibling("property");
//		}
//		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointMaxConeAngle")
//		{
//			this->maxConeLimit->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
//			propertyElement = propertyElement->next_sibling("property");
//		}
//		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointMinAngleLimit")
//		{
//			this->minAngleLimit->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
//			propertyElement = propertyElement->next_sibling("property");
//		}
//		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointMaxAngleLimit")
//		{
//			this->maxAngleLimit->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
//			propertyElement = propertyElement->next_sibling("property");
//		}
//		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointMaxAngleLimit")
//		{
//			this->maxAngleLimit->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
//			propertyElement = propertyElement->next_sibling("property");
//		}
//		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointDofCount")
//		{
//			this->dofCount->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
//			propertyElement = propertyElement->next_sibling("property");
//		}
//		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointMotorOn")
//		{
//			this->motorOn->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
//			propertyElement = propertyElement->next_sibling("property");
//		}
//		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointTorque")
//		{
//			this->torque->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
//			propertyElement = propertyElement->next_sibling("property");
//		}
//		return true;
//	}
//
//	bool RagDollMotorDofComponent::postInit(void)
//	{
//		JointComponent::postInit();

// Component must be dynamic, because it will be moved
//this->gameObjectPtr->setDynamic(true);
//this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
//		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[RagDollMotorDofComponent] Init joint rag doll motor dof component for game object: " + this->gameObjectPtr->getName());
//
//		return true;
//	}
//
//	bool RagDollMotorDofComponent::connect(void)
//	{
//		bool success = JointComponent::connect();
//		return success;
//	}
//
//	bool RagDollMotorDofComponent::disconnect(void)
//	{
//		bool success = JointComponent::disconnect();
//		return success;
//	}
//
//	Ogre::String RagDollMotorDofComponent::getClassName(void) const
//	{
//		return "RagDollMotorDofComponent";
//	}
//
//	Ogre::String RagDollMotorDofComponent::getParentClassName(void) const
//	{
//		return "JointComponent";
//	}
//
//	void RagDollMotorDofComponent::update(Ogre::Real dt, bool notSimulating)
//	{

//	}
//
//	void RagDollMotorDofComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
//	{
//		JointComponent::writeXML(propertiesXML, doc, filePath);
//
//		// 2 = int
//		// 6 = real
//		// 7 = string
//		// 8 = vector2
//		// 9 = vector3
//		// 10 = vector4 -> also quaternion
//		// 12 = bool
//		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
//		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
//		propertyXML->append_attribute(doc.allocate_attribute("name", "JointAnchorPosition"));
//		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->anchorPosition->getVector3())));
//		propertiesXML->append_node(propertyXML);
//
//		propertyXML = doc.allocate_node(node_element, "property");
//		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
//		propertyXML->append_attribute(doc.allocate_attribute("name", "JointMaxConeAngle"));
//		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxConeLimit->getReal())));
//		propertiesXML->append_node(propertyXML);
//
//		propertyXML = doc.allocate_node(node_element, "property");
//		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
//		propertyXML->append_attribute(doc.allocate_attribute("name", "JointMinAngleLimit"));
//		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->minAngleLimit->getReal())));
//		propertiesXML->append_node(propertyXML);
//
//		propertyXML = doc.allocate_node(node_element, "property");
//		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
//		propertyXML->append_attribute(doc.allocate_attribute("name", "JointMaxAngleLimit"));
//		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxAngleLimit->getReal())));
//		propertiesXML->append_node(propertyXML);
//
//		propertyXML = doc.allocate_node(node_element, "property");
//		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
//		propertyXML->append_attribute(doc.allocate_attribute("name", "JointMaxAngleLimit"));
//		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxAngleLimit->getReal())));
//		propertiesXML->append_node(propertyXML);
//
//		propertyXML = doc.allocate_node(node_element, "property");
//		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
//		propertyXML->append_attribute(doc.allocate_attribute("name", "JointDofCount"));
//		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->dofCount->getListSelectedValue())));
//		propertiesXML->append_node(propertyXML);
//		
//		propertyXML = doc.allocate_node(node_element, "property");
//		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
//		propertyXML->append_attribute(doc.allocate_attribute("name", "JointMotorOn"));
//		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->motorOn->getBool())));
//		propertiesXML->append_node(propertyXML);
//		
//		propertyXML = doc.allocate_node(node_element, "property");
//		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
//		propertyXML->append_attribute(doc.allocate_attribute("name", "JointTorque"));
//		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->torque->getReal())));
//		propertiesXML->append_node(propertyXML);
//	}
//
//	void RagDollMotorDofComponent::showDebugData(void)
//	{
//		GameObjectComponent::showDebugData();
//		if (true == this->bShowDebugData)
//		{
//			if (nullptr == this->debugGeometryNode)
//			{
//				this->debugGeometryNode = this->gameObjectPtr->getSceneNode()->createChildSceneNode(Ogre::SCENE_DYNAMIC);
//				this->debugGeometryNode->setPosition(this->anchorPosition->getVector3());
//				this->debugGeometryNode->setDirection(this->pin->getVector3());
//				this->debugGeometryNode->setName("RagDollMotorDofJointPointNode");
//				// Do not inherit, because if parent node is scaled, then this scale is relative and debug data may be to small or to big
//				this->debugGeometryNode->setInheritScale(false);
//				this->debugGeometryNode->setScale(0.05f, 0.05f, 0.05f);
//				this->debugGeometryEntity = this->sceneManager->createEntity("gizmosphere.mesh");
//				this->debugGeometryEntity->setName("RagDollMotorDofPointEntity");
//				this->debugGeometryEntity->setDatablock("BaseYellowLine");
//				this->debugGeometryEntity->setQueryFlags(0 << 0);
//				this->debugGeometryEntity->setCastShadows(false);
//				this->debugGeometryNode->attachObject(this->debugGeometryEntity);
//			}
//		}
//		else
//		{
//			if (nullptr != this->debugGeometryNode)
//			{
//				this->debugGeometryNode->detachAllObjects();
//				this->gameObjectPtr->getSceneManager()->destroySceneNode(this->debugGeometryNode);
//				this->debugGeometryNode = nullptr;
//				this->gameObjectPtr->getSceneManager()->destroyMovableObject(this->debugGeometryEntity);
//				this->debugGeometryEntity = nullptr;
//			}
//		}
//	}
//
//	bool RagDollMotorDofComponent::createJoint(const Ogre::Vector3& customJointPosition)
//	{
//		// Joint base created but not activated, return false, but its no error, hence after that return true.
		// if (false == JointComponent::createJoint(customJointPosition))
		// {
		// 	return true;
		// }
		//if (true == this->jointAlreadyCreated)
		//{
		//	// joint already created so skip
		//	return true;
		//}
//		if (nullptr == this->body)
//			{
//			this->connect();
//			}
//if (Ogre::Vector3::ZERO == customJointPosition)
//		{
//			Ogre::Vector3 size = this->body->getAABB().getSize();
//			//Ogre::Vector3 aPos = Ogre::Vector3(-this->anchorPosition.x, -this->anchorPosition.y, -this->anchorPosition.z);
//			//// relative anchor pos: 0.5 0 0 means 50% in X of the size of the parent object
//			//Ogre::Vector3 offset = (aPos * size);
//			//// take orientation into account, so that orientated joints are processed correctly
//			//this->jointPosition = this->body->getPosition() - (this->body->getOrientation() * offset);
//
//			// without rotation
//			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
//			this->jointPosition = (this->body->getPosition() + offset);
//		}
//		else
//		{
//			this->hasCustomJointPosition = true;
//			this->jointPosition = customJointPosition;
//		}
//		OgreNewt::Body* predecessorBody = nullptr;
//		if (this->predecessorJointCompPtr)
//		{
//			predecessorBody = this->predecessorJointCompPtr->getBody();
//			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[RagDollMotorDofComponent] Creating ragdoll motor dof joint for body1 name: "
//				+ this->predecessorJointCompPtr->getOwner()->getName() + " body2 name: " + this->gameObjectPtr->getName());
//		}
//		else
//		{
//			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[RagDollMotorDofComponent] Created ragdoll motor dof joint joint: " + this->gameObjectPtr->getName() + " with world as parent");
//		}
//
//		// Release joint each time, to create new one with new values
//		this->internalReleaseJoint();
//		
//		unsigned int dofCount = 1;
//		if ("2" == this->dofCount->getListSelectedValue())
//			dofCount = 2;
//		else if ("3" == this->dofCount->getListSelectedValue())
//			dofCount = 3;
//
//		this->joint = new OgreNewt::RagDollMotor(this->body, predecessorBody, this->jointPosition, dofCount);
//		
//// Attention: Just a test!
//		// this->joint->setStiffness(0.1f);
//		OgreNewt::RagDollMotor* ragDollMotor = dynamic_cast<OgreNewt::RagDollMotor*>(this->joint);
//		
//		this->joint->setBodyMassScale(this->bodyMassScale->getVector2().x, this->bodyMassScale->getVector2().y);
//		this->joint->setCollisionState(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
////		this->body->setJointRecursiveCollision(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
//		this->applyStiffness();
//
//		ragDollMotor->setLimits(Ogre::Degree(this->maxConeLimit->getReal()), Ogre::Degree(this->minAngleLimit->getReal()), Ogre::Degree(this->maxAngleLimit->getReal()));
//		ragDollMotor->setMode(this->motorOn->getBool());
//		ragDollMotor->setTorgue(this->torque->getReal());
//
//		this->jointAlreadyCreated = true;
//		return true;
//	}
//
//	void RagDollMotorDofComponent::actualizeValue(Variant* attribute)
//	{
//		JointComponent::actualizeValue(attribute);
//
//		if (RagDollMotorDofComponent::AttrAnchorPosition() == attribute->getName())
//		{
//			this->setAnchorPosition(attribute->getVector3());
//		}
//		else if (RagDollMotorDofComponent::AttrMinAngleLimit() == attribute->getName())
//		{
//			this->setMinMaxConeAngleLimit(Ogre::Degree(attribute->getReal()), Ogre::Degree(this->maxAngleLimit->getReal()), Ogre::Degree(this->maxConeLimit->getReal()));
//		}
//		else if (RagDollMotorDofComponent::AttrMaxAngleLimit() == attribute->getName())
//		{
//			this->setMinMaxConeAngleLimit(Ogre::Degree(this->minAngleLimit->getReal()), Ogre::Degree(attribute->getReal()), Ogre::Degree(this->maxConeLimit->getReal()));
//		}
//		else if (RagDollMotorDofComponent::AttrMaxConeLimit() == attribute->getName())
//		{
//			this->setMinMaxConeAngleLimit(Ogre::Degree(this->minAngleLimit->getReal()), Ogre::Degree(this->maxAngleLimit->getReal()), Ogre::Degree(attribute->getReal()));
//		}
//		else if (RagDollMotorDofComponent::AttrDofCount() == attribute->getName())
//		{
//			unsigned int dofCount = 1;
//			if ("2" == attribute->getListSelectedValue())
//				dofCount = 2;
//			else if ("3" == attribute->getListSelectedValue())
//				dofCount = 3;
//			this->setDofCount(dofCount);
//		}
//		else if (RagDollMotorDofComponent::AttrMotorOn() == attribute->getName())
//		{
//			this->setMotorOn(attribute->getBool());
//		}
//		else if (RagDollMotorDofComponent::AttrTorque() == attribute->getName())
//		{
//			this->setTorque(attribute->getReal());
//		}
//	}
//
//	void RagDollMotorDofComponent::setAnchorPosition(const Ogre::Vector3& anchorPosition)
//	{
//		this->anchorPosition->setValue(anchorPosition);
//		this->jointAlreadyCreated = false;
//	}
//
//	Ogre::Vector3 RagDollMotorDofComponent::getAnchorPosition(void) const
//	{
//		return this->anchorPosition->getVector3();
//	}
//
//	void RagDollMotorDofComponent::setMinMaxConeAngleLimit(const Ogre::Degree& minAngleLimit, const Ogre::Degree& maxAngleLimit, const Ogre::Degree& maxConeLimit)
//	{
//		this->minAngleLimit->setValue(minAngleLimit.valueDegrees());
//		this->maxAngleLimit->setValue(maxAngleLimit.valueDegrees());
//		this->maxConeLimit->setValue(maxConeLimit.valueDegrees());
//
//		OgreNewt::RagDollMotor* ragDollMotor = dynamic_cast<OgreNewt::RagDollMotor*>(this->joint);
//		if (nullptr != ragDollMotor)
//			ragDollMotor->setLimits(Ogre::Degree(this->maxConeLimit->getReal()), Ogre::Degree(this->minAngleLimit->getReal()), Ogre::Degree(this->maxAngleLimit->getReal()));
//	}
//
//	std::tuple<Ogre::Degree, Ogre::Degree, Ogre::Degree> RagDollMotorDofComponent::getMinMaxConeAngleLimit(void) const
//	{
//		return std::make_tuple(Ogre::Degree(this->minAngleLimit->getReal()), Ogre::Degree(this->minAngleLimit->getReal()), Ogre::Degree(this->maxAngleLimit->getReal()));
//	}
//
//	void RagDollMotorDofComponent::setDofCount(unsigned int dofCount)
//	{
//		if (1 == dofCount)
//			this->dofCount->setListSelectedValue("1");
//		else if (2 == dofCount)
//			this->dofCount->setListSelectedValue("2");
//		else if (3 == dofCount)
//			this->dofCount->setListSelectedValue("3");
//
//		/*OgreNewt::RagDollMotor* ragDollMotor = dynamic_cast<OgreNewt::RagDollMotor*>(this->joint);
//		if (nullptr != ragDollMotor)
//			ragDollMotor->setLimits(Ogre::Degree(this->maxConeLimit->getReal()), Ogre::Degree(this->minAngleLimit->getReal()), Ogre::Degree(this->maxAngleLimit->getReal()));*/
//	}
//
//	unsigned int RagDollMotorDofComponent::getDofCount(void) const
//	{
//		unsigned int dofCount = 1;
//		if ("2" == this->dofCount->getListSelectedValue())
//			dofCount = 2;
//		else if ("3" == this->dofCount->getListSelectedValue())
//			dofCount = 3;
//		return dofCount;
//	}
//	
//	void RagDollMotorDofComponent::setMotorOn(bool on)
//	{
//		this->motorOn->setValue(on);
//		OgreNewt::RagDollMotor* ragDollMotor = dynamic_cast<OgreNewt::RagDollMotor*>(this->joint);
//		if (nullptr != ragDollMotor)
//			ragDollMotor->setMode(this->motorOn->getBool());
//	}
//	
//	bool RagDollMotorDofComponent::isMotorOn(void) const
//	{
//		return this->motorOn->getBool();
//	}
//	
//	void RagDollMotorDofComponent::setTorque(Ogre::Real torque)
//	{
//		this->torque->setValue(torque);
//		OgreNewt::RagDollMotor* ragDollMotor = dynamic_cast<OgreNewt::RagDollMotor*>(this->joint);
//		if (nullptr != ragDollMotor)
//			ragDollMotor->setTorgue(this->torque->getReal());
//	}
//	
//	Ogre::Real RagDollMotorDofComponent::getTorque(void) const
//	{
//		return this->torque->getReal();
//	}
	
	/*******************************JointPinComponent*******************************/

	JointPinComponent::JointPinComponent()
		: JointComponent()
	{
		this->type->setReadOnly(false);
		this->type->setValue(this->getClassName());
		this->type->setReadOnly(true);
		this->type->setDescription("This specialized vector can be used to limit rotation to one axis of freedom.");
		this->pin = new Variant(JointPinComponent::AttrPin(), Ogre::Vector3(0.0f, 0.0f, 1.0f), this->attributes);

		this->targetId->setVisible(false);
	}

	JointPinComponent::~JointPinComponent()
	{

	}

	GameObjectCompPtr JointPinComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		JointPinCompPtr clonedJointCompPtr(boost::make_shared<JointPinComponent>());

		clonedJointCompPtr->internalSetPriorId(this->id->getULong());
		clonedJointCompPtr->setType(this->type->getString());
		clonedJointCompPtr->setPredecessorId(this->predecessorId->getULong());
		clonedJointCompPtr->setTargetId(this->targetId->getULong());
		clonedJointCompPtr->setJointPosition(this->jointPosition);
		clonedJointCompPtr->setJointRecursiveCollisionEnabled(this->jointRecursiveCollision->getBool());
		clonedJointCompPtr->stiffness->setValue(this->stiffness->getReal());

		clonedJointCompPtr->setPin(this->pin->getVector3());

		clonedGameObjectPtr->addComponent(clonedJointCompPtr);
		clonedJointCompPtr->setOwner(clonedGameObjectPtr);

		clonedJointCompPtr->setActivated(this->activated->getBool());

		return clonedJointCompPtr;
	}

	bool JointPinComponent::init(xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		JointComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointPin")
		{
			this->pin->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(0.0f, 0.0f, 0.0f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	bool JointPinComponent::postInit(void)
	{
		JointComponent::postInit();

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointPinComponent] Init joint pin component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool JointPinComponent::connect(void)
	{
		bool success = JointComponent::connect();
		return success;
	}

	bool JointPinComponent::disconnect(void)
	{
		bool success = JointComponent::disconnect();
		return success;
	}

	Ogre::String JointPinComponent::getClassName(void) const
	{
		return "JointPinComponent";
	}

	Ogre::String JointPinComponent::getParentClassName(void) const
	{
		return "JointComponent";
	}

	void JointPinComponent::update(Ogre::Real dt, bool notSimulating)
	{

	}

	void JointPinComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		JointComponent::writeXML(propertiesXML, doc, filePath);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointPin"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->pin->getVector3())));
		propertiesXML->append_node(propertyXML);
	}

	bool JointPinComponent::createJoint(const Ogre::Vector3& customJointPosition)
	{
		// Joint base created but not activated, return false, but its no error, hence after that return true.
		if (false == JointComponent::createJoint(customJointPosition))
		{
			return true;
		}

		if (nullptr == this->body)
		{
			this->connect();
		}
		if (Ogre::Vector3::ZERO == this->pin->getVector3())
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[JointPinComponent] Cannot create joint for: " + this->gameObjectPtr->getName() + " because the pin is zero.");
			return false;
		}
		
		this->jointPosition = this->body->getPosition();

		// Release joint each time, to create new one with new values
		this->internalReleaseJoint();
		this->joint = new OgreNewt::UpVector(this->body, this->body->getOrientation() * this->pin->getVector3());

		this->joint->setBodyMassScale(this->bodyMassScale->getVector2().x, this->bodyMassScale->getVector2().y);
		this->joint->setCollisionState(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		// this->body->setJointRecursiveCollision(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		
		return true;
	}

	void JointPinComponent::actualizeValue(Variant* attribute)
	{
		JointComponent::actualizeValue(attribute);

		if (JointPinComponent::AttrPin() == attribute->getName())
		{
			this->setPin(attribute->getVector3());
		}
	}

	void JointPinComponent::setPin(const Ogre::Vector3& pin)
	{
		this->pin->setValue(pin);
	}

	Ogre::Vector3 JointPinComponent::getPin(void) const
	{
		return this->pin->getVector3();
	}
	
	/*******************************JointPlaneComponent*******************************/

	JointPlaneComponent::JointPlaneComponent()
		: JointComponent()
	{
		this->type->setReadOnly(false);
		this->type->setValue(this->getClassName());
		this->type->setReadOnly(true);
		this->type->setDescription("?");
		this->anchorPosition = new Variant(JointPlaneComponent::AttrAnchorPosition(), Ogre::Vector3::ZERO, this->attributes);
		this->normal = new Variant(JointPlaneComponent::AttrNormal(), Ogre::Vector3(0.0f, 1.0f, 0.0f), this->attributes);

		this->targetId->setVisible(false);
	}

	JointPlaneComponent::~JointPlaneComponent()
	{

	}

	GameObjectCompPtr JointPlaneComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		JointPlaneCompPtr clonedJointCompPtr(boost::make_shared<JointPlaneComponent>());

		clonedJointCompPtr->internalSetPriorId(this->id->getULong());
		clonedJointCompPtr->setType(this->type->getString());
		clonedJointCompPtr->setPredecessorId(this->predecessorId->getULong());
		clonedJointCompPtr->setTargetId(this->targetId->getULong());
		clonedJointCompPtr->setJointPosition(this->jointPosition);
		clonedJointCompPtr->setJointRecursiveCollisionEnabled(this->jointRecursiveCollision->getBool());
		clonedJointCompPtr->stiffness->setValue(this->stiffness->getReal());

		clonedJointCompPtr->setNormal(this->normal->getVector3());

		clonedGameObjectPtr->addComponent(clonedJointCompPtr);
		clonedJointCompPtr->setOwner(clonedGameObjectPtr);

		clonedJointCompPtr->setActivated(this->activated->getBool());

		return clonedJointCompPtr;
	}

	bool JointPlaneComponent::init(xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		JointComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnchorPosition")
		{
			this->anchorPosition->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Normal")
		{
			this->normal->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	bool JointPlaneComponent::postInit(void)
	{
		JointComponent::postInit();

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointPlaneComponent] Init joint plane component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool JointPlaneComponent::connect(void)
	{
		bool success = JointComponent::connect();
		return success;
	}

	bool JointPlaneComponent::disconnect(void)
	{
		bool success = JointComponent::disconnect();
		return success;
	}

	Ogre::String JointPlaneComponent::getClassName(void) const
	{
		return "JointPlaneComponent";
	}

	Ogre::String JointPlaneComponent::getParentClassName(void) const
	{
		return "JointComponent";
	}

	void JointPlaneComponent::update(Ogre::Real dt, bool notSimulating)
	{

	}

	void JointPlaneComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		JointComponent::writeXML(propertiesXML, doc, filePath);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnchorPosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->anchorPosition->getVector3())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Normal"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->normal->getVector3())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::Vector3 JointPlaneComponent::getUpdatedJointPosition(void)
	{
		if (nullptr == this->body)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointPlaneComponent] Could not get 'getUpdatedJointPosition', because there is no body for game object: " + this->gameObjectPtr->getName());
			return Ogre::Vector3::ZERO;
		}

		if (false == this->hasCustomJointPosition)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			return this->body->getPosition() + offset;
		}
		else
		{
			return this->jointPosition;
		}
	}

	bool JointPlaneComponent::createJoint(const Ogre::Vector3& customJointPosition)
	{
		// Joint base created but not activated, return false, but its no error, hence after that return true.
		if (false == JointComponent::createJoint(customJointPosition))
		{
			return true;
		}

		if (nullptr == this->body)
		{
			this->connect();
		}
		if (Ogre::Vector3::ZERO == customJointPosition)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			//Ogre::Vector3 aPos = Ogre::Vector3(-this->anchorPosition.x, -this->anchorPosition.y, -this->anchorPosition.z);
			//// relative anchor pos: 0.5 0 0 means 50% in X of the size of the parent object
			//Ogre::Vector3 offset = (aPos * size);
			//// take orientation into account, so that orientated joints are processed correctly
			//this->jointPosition = this->body->getPosition() - (this->body->getOrientation() * offset);

			// without rotation
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			this->jointPosition = (this->body->getPosition() + offset);
		}
		else
		{
			this->hasCustomJointPosition = true;
			this->jointPosition = customJointPosition;
		}
		OgreNewt::Body* predecessorBody = nullptr;
		if (this->predecessorJointCompPtr)
		{
			predecessorBody = this->predecessorJointCompPtr->getBody();
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[JointPlaneComponent] Creating plane joint for body1 name: "
				+ this->predecessorJointCompPtr->getOwner()->getName() + " body2 name: " + this->gameObjectPtr->getName());
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointPlaneComponent] Created plane joint: " + this->gameObjectPtr->getName() + " with world as parent");
		}

		// Release joint each time, to create new one with new values
		this->internalReleaseJoint();
		this->joint = new OgreNewt::PlaneConstraint(this->body, predecessorBody, this->jointPosition, this->normal->getVector3());
// Attention: Just a test!
		// this->joint->setStiffness(0.1f);

		this->joint->setBodyMassScale(this->bodyMassScale->getVector2().x, this->bodyMassScale->getVector2().y);
		this->joint->setCollisionState(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		// this->body->setJointRecursiveCollision(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		// this->applyStiffness();

		return true;
	}

	void JointPlaneComponent::actualizeValue(Variant* attribute)
	{
		JointComponent::actualizeValue(attribute);

		if (JointBallAndSocketComponent::AttrAnchorPosition() == attribute->getName())
		{
			this->setAnchorPosition(attribute->getVector3());
		}
		else if (JointPlaneComponent::AttrNormal() == attribute->getName())
		{
			this->setNormal(attribute->getVector3());
		}
	}
	
	void JointPlaneComponent::setAnchorPosition(const Ogre::Vector3& anchorPosition)
	{
		this->anchorPosition->setValue(anchorPosition);

		if (nullptr != this->body)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();

			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			this->jointPosition = (this->body->getPosition() + offset);
			this->jointAlreadyCreated = false;
		}
	}

	Ogre::Vector3 JointPlaneComponent::getAnchorPosition(void) const
	{
		return this->anchorPosition->getVector3();
	}

	void JointPlaneComponent::setNormal(const Ogre::Vector3& normal)
	{
		this->normal->setValue(normal);
	}

	Ogre::Vector3 JointPlaneComponent::getNormal(void) const
	{
		return this->normal->getVector3();
	}

	/*******************************JointSpringComponent*******************************/

	// http://www.ryanmwright.com/tag/c/: Mass spring for hair etc.
	JointSpringComponent::JointSpringComponent()
		: JointComponent(),
		predecessorBody(nullptr),
		dragLineNode(nullptr),
		dragLineObject(nullptr)
	{
		this->type->setReadOnly(false);
		this->type->setValue(this->getClassName());
		this->type->setReadOnly(true);
		this->type->setDescription("With this joint its possible to connect two objects with a spring.");
		this->springStrength = new Variant(JointSpringComponent::AttrSpringStrength(), Ogre::Real(10.0f), this->attributes);
		this->anchorOffsetPosition = new Variant(JointSpringComponent::AttrAnchorOffsetPosition(), Ogre::Vector3::ZERO, this->attributes);
		this->springOffsetPosition = new Variant(JointSpringComponent::AttrSpringOffsetPosition(), Ogre::Vector3::ZERO, this->attributes);
		this->showLine = new Variant(JointSpringComponent::AttrShowLine(), true, this->attributes);

		this->targetId->setVisible(false);
	}

	JointSpringComponent::~JointSpringComponent()
	{
		PhysicsActiveCompPtr physicsActiveCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsActiveComponent>());
		if (nullptr != physicsActiveCompPtr)
		{
			physicsActiveCompPtr->removeJointSpringComponent(this->id->getULong());
		}
		this->releaseSpring();
	}

	GameObjectCompPtr JointSpringComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		JointSpringCompPtr clonedJointCompPtr(boost::make_shared<JointSpringComponent>());
		
		clonedJointCompPtr->setType(this->type->getString());
		clonedJointCompPtr->internalSetPriorId(this->id->getULong());
		clonedJointCompPtr->setPredecessorId(this->predecessorId->getULong());
		clonedJointCompPtr->setTargetId(this->targetId->getULong());

		clonedJointCompPtr->setJointPosition(this->jointPosition);
		clonedJointCompPtr->setJointRecursiveCollisionEnabled(this->jointRecursiveCollision->getBool());

		clonedJointCompPtr->setSpringStrength(this->springStrength->getReal());
		clonedJointCompPtr->setShowLine(this->showLine->getBool());
		clonedJointCompPtr->setAnchorOffsetPosition(this->anchorOffsetPosition->getVector3());
		clonedJointCompPtr->setSpringOffsetPosition(this->springOffsetPosition->getVector3());
		clonedJointCompPtr->stiffness->setValue(this->stiffness->getReal());

		clonedGameObjectPtr->addComponent(clonedJointCompPtr);
		clonedJointCompPtr->setOwner(clonedGameObjectPtr);

		clonedJointCompPtr->setActivated(this->activated->getBool());

		return clonedJointCompPtr;
	}

	bool JointSpringComponent::init(xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		JointComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointSpringStrength")
		{
			this->springStrength->setValue(XMLConverter::getAttribReal(propertyElement, "data", 10.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointSpringShowLine")
		{
			this->showLine->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointSpringAnchorOffsetPosition")
		{
			this->anchorOffsetPosition->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointSpringOffsetPosition")
		{
			this->springOffsetPosition->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	bool JointSpringComponent::postInit(void)
	{
		JointComponent::postInit();

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointSpringComponent] Init joint spring component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool JointSpringComponent::connect(void)
	{
		bool success = JointComponent::connect();

		PhysicsActiveCompPtr physicsActiveCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsActiveComponent>());
		if (nullptr != physicsActiveCompPtr)
		{
			physicsActiveCompPtr->addJointSpringComponent(this->id->getULong());
		}
		else
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[JointSpringComponent] Cannot final init spring joint for " + this->gameObjectPtr->getName() + " because there active component!");
			return false;
		}

		return success;
	}

	bool JointSpringComponent::disconnect(void)
	{
		bool success = JointComponent::disconnect();

		PhysicsActiveCompPtr physicsActiveCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsActiveComponent>());
		if (nullptr != physicsActiveCompPtr)
		{
			physicsActiveCompPtr->removeJointSpringComponent(this->id->getULong());
		}
		this->releaseSpring();
		return success;
	}

	Ogre::String JointSpringComponent::getClassName(void) const
	{
		return "JointSpringComponent";
	}

	Ogre::String JointSpringComponent::getParentClassName(void) const
	{
		return "JointComponent";
	}

	void JointSpringComponent::update(Ogre::Real dt, bool notSimulating)
	{
		
	}

	void JointSpringComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		JointComponent::writeXML(propertiesXML, doc, filePath);
// Attention: There can be more spring joints, so the attribute name must remain unique!

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointSpringStrength"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->springStrength->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointSpringShowLine"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->showLine->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointSpringAnchorOffsetPosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->anchorOffsetPosition->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointSpringOffsetPosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->springOffsetPosition->getVector3())));
		propertiesXML->append_node(propertyXML);
	}

	bool JointSpringComponent::createJoint(const Ogre::Vector3& customJointPosition)
	{
		// Joint base created but not activated, return false, but its no error, hence after that return true.
		if (false == JointComponent::createJoint(customJointPosition))
		{
			return true;
		}

		if (nullptr == this->body)
		{
			this->connect();
		}

		this->jointPosition = this->body->getPosition();

		if (nullptr != this->predecessorJointCompPtr)
		{
			this->predecessorBody = this->predecessorJointCompPtr->getBody();
		}
		if (nullptr == this->predecessorBody)
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[JointSpringComponent] Could not create spring joint for " + this->gameObjectPtr->getName() + " because there no predecessor body!");
			return false;
		}

		if (true == this->showLine->getBool())
		{
			this->createLine();
		}
		
		return true;
	}

	void JointSpringComponent::actualizeValue(Variant* attribute)
	{
		JointComponent::actualizeValue(attribute);

		if (JointSpringComponent::AttrSpringStrength() == attribute->getName())
		{
			this->setSpringStrength(attribute->getReal());
		}
		else if (JointSpringComponent::AttrAnchorOffsetPosition() == attribute->getName())
		{
			this->setAnchorOffsetPosition(attribute->getVector3());
		}
		else if (JointSpringComponent::AttrSpringOffsetPosition() == attribute->getName())
		{
			this->setSpringOffsetPosition(attribute->getVector3());
		}
		else if (JointSpringComponent::AttrShowLine() == attribute->getName())
		{
			this->setShowLine(attribute->getBool());
		}
	}

	void JointSpringComponent::setSpringStrength(Ogre::Real springStrength)
	{
		this->springStrength->setValue(springStrength);
	}

	Ogre::Real JointSpringComponent::getSpringStrength(void) const
	{
		return this->springStrength->getReal();
	}

	void JointSpringComponent::setShowLine(bool showLine)
	{
		this->showLine->setValue(showLine);
	}

	bool JointSpringComponent::getShowLine(void) const
	{
		return this->showLine->getBool();
	}

	void JointSpringComponent::setAnchorOffsetPosition(const Ogre::Vector3& anchorOffsetPosition)
	{
		this->anchorOffsetPosition->setValue(anchorOffsetPosition);
	}

	const Ogre::Vector3 JointSpringComponent::getAnchorOffsetPosition(void)
	{
		return this->anchorOffsetPosition->getVector3();
	}

	void JointSpringComponent::setSpringOffsetPosition(const Ogre::Vector3& springOffsetPosition)
	{
		this->springOffsetPosition->setValue(springOffsetPosition);
	}

	const Ogre::Vector3 JointSpringComponent::getSpringOffsetPosition(void)
	{
		return this->springOffsetPosition->getVector3();
	}

	void JointSpringComponent::releaseSpring(void)
	{
		// Send event to physics active component to release the spring force
		// boost::shared_ptr<NOWA::EventDataSpringRelease> eventDataSpringReleaseEvent(new NOWA::EventDataSpringRelease(this->id->getULong()));
		// NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataSpringReleaseEvent);

		if (nullptr != this->dragLineNode)
		{
			this->dragLineNode->detachAllObjects();
			this->gameObjectPtr->getSceneManager()->destroySceneNode(this->dragLineNode);
			this->dragLineNode = nullptr;
		}
		if (nullptr != this->dragLineObject)
		{
			this->gameObjectPtr->getSceneManager()->destroyMovableObject(this->dragLineObject);
			this->dragLineObject = nullptr;
		}
	}

	void  JointSpringComponent::createLine(void)
	{
		if (nullptr == this->dragLineNode)
		{
			this->dragLineNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();
			this->dragLineObject = this->gameObjectPtr->getSceneManager()->createManualObject();
			this->dragLineObject->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);
			this->dragLineObject->setName("SpringLine");
			this->dragLineObject->setQueryFlags(0 << 0);
			this->dragLineNode->attachObject(this->dragLineObject);
		}
	}

	void JointSpringComponent::drawLine(const Ogre::Vector3& startPosition, const Ogre::Vector3& endPosition)
	{
		// Draw a 3D line between these points for visual effect
		this->dragLineObject->clear();
		this->dragLineObject->begin("BaseWhiteNoLighting", Ogre::OperationType::OT_LINE_LIST);
		this->dragLineObject->position(startPosition);
		this->dragLineObject->index(0);
		this->dragLineObject->position(endPosition);
		this->dragLineObject->index(1);
		this->dragLineObject->end();
	}

	/*******************************JointSpringComponent*******************************/

	JointAttractorComponent::JointAttractorComponent()
		: JointComponent()
	{
		this->type->setReadOnly(false);
		this->type->setValue(this->getClassName());
		this->type->setReadOnly(true);
		this->type->setDescription("With this joint its possible to influence other physics active game object (repeller) by magnetic strength. The game object must belong to the specified repeller category");
		
		this->magneticStrength = new Variant("Magnetic Strength", 5000.0f, this->attributes);
		this->attractionDistance = new Variant("Attraction Distance", 30.0f, this->attributes);
		this->repellerCategory = new Variant("Repeller Category", Ogre::String("Repeller"), this->attributes);

		this->predecessorId->setVisible(false);
		this->targetId->setVisible(false);
		this->jointRecursiveCollision->setVisible(false);
	}

	JointAttractorComponent::~JointAttractorComponent()
	{
		auto& repellerGameObjects = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsFromCategory(this->repellerCategory->getString());
		for (size_t i = 0; i < repellerGameObjects.size(); i++)
		{
			auto& physicsActiveCompPtr = NOWA::makeStrongPtr(repellerGameObjects[i]->getComponent<PhysicsActiveComponent>());
			if (nullptr != physicsActiveCompPtr)
			{
				physicsActiveCompPtr->removeJointAttractorComponent(this->id->getULong());
			}
		}
	}

	GameObjectCompPtr JointAttractorComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		JointAttractorCompPtr clonedJointCompPtr(boost::make_shared<JointAttractorComponent>());
		
		clonedJointCompPtr->setType(this->type->getString());
		clonedJointCompPtr->internalSetPriorId(this->id->getULong());
		clonedJointCompPtr->setPredecessorId(this->predecessorId->getULong());
		clonedJointCompPtr->setTargetId(this->targetId->getULong());

		clonedJointCompPtr->setJointPosition(this->jointPosition);
		clonedJointCompPtr->setJointRecursiveCollisionEnabled(this->jointRecursiveCollision->getBool());

		clonedJointCompPtr->setMagneticStrength(this->magneticStrength->getReal());
		clonedJointCompPtr->setAttractionDistance(this->attractionDistance->getReal());
		clonedJointCompPtr->setRepellerCategory(this->repellerCategory->getString());
		clonedJointCompPtr->stiffness->setValue(this->stiffness->getReal());

		clonedGameObjectPtr->addComponent(clonedJointCompPtr);
		clonedJointCompPtr->setOwner(clonedGameObjectPtr);

		clonedJointCompPtr->setActivated(this->activated->getBool());

		return clonedJointCompPtr;
	}

	bool JointAttractorComponent::init(xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		JointComponent::init(propertyElement, filename);

		if (propertyElement)
		{
			if (Ogre::StringUtil::match(XMLConverter::getAttrib(propertyElement, "name"), "MagneticStrength", true))
			{
				this->magneticStrength->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.0f));
				propertyElement = propertyElement->next_sibling("property");
			}
		}
		if (propertyElement)
		{
			if (Ogre::StringUtil::match(XMLConverter::getAttrib(propertyElement, "name"), "AttractionDistance", true))
			{
				this->attractionDistance->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.0f));
				propertyElement = propertyElement->next_sibling("property");
			}
		}
		if (propertyElement)
		{
			if (Ogre::StringUtil::match(XMLConverter::getAttrib(propertyElement, "name"), "RepellerCategory", true))
			{
				this->repellerCategory->setValue(XMLConverter::getAttrib(propertyElement, "data"));
				propertyElement = propertyElement->next_sibling("property");
			}
		}
		return true;
	}

	bool JointAttractorComponent::postInit(void)
	{
		JointComponent::postInit();

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointAttractorComponent] Init joint attractor component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool JointAttractorComponent::connect(void)
	{
		bool success = JointComponent::connect();

		auto& repellerGameObjects = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsFromCategory(this->repellerCategory->getString());
		for (size_t i = 0; i < repellerGameObjects.size(); i++)
		{
			auto& physicsActiveCompPtr = NOWA::makeStrongPtr(repellerGameObjects[i]->getComponent<PhysicsActiveComponent>());
			if (nullptr != physicsActiveCompPtr)
			{
				physicsActiveCompPtr->addJointAttractorComponent(this->id->getULong());
			}
		}

		return success;
	}

	bool JointAttractorComponent::disconnect(void)
	{
		bool success = JointComponent::disconnect();

		auto& repellerGameObjects = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectsFromCategory(this->repellerCategory->getString());
		for (size_t i = 0; i < repellerGameObjects.size(); i++)
		{
			auto& physicsActiveCompPtr = NOWA::makeStrongPtr(repellerGameObjects[i]->getComponent<PhysicsActiveComponent>());
			if (nullptr != physicsActiveCompPtr)
			{
				physicsActiveCompPtr->removeJointAttractorComponent(this->id->getULong());
			}
		}
		return success;
	}

	Ogre::String JointAttractorComponent::getClassName(void) const
	{
		return "JointAttractorComponent";
	}

	Ogre::String JointAttractorComponent::getParentClassName(void) const
	{
		return "JointComponent";
	}

	void JointAttractorComponent::update(Ogre::Real dt, bool notSimulating)
	{
		
	}

	void JointAttractorComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		JointComponent::writeXML(propertiesXML, doc, filePath);
		// Attention: There can be more spring joints, so the attribute name must remain unique!

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MagneticStrength"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->magneticStrength->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AttractionDistance"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->attractionDistance->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RepellerCategory"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->repellerCategory->getString())));
		propertiesXML->append_node(propertyXML);
	}

	bool JointAttractorComponent::createJoint(const Ogre::Vector3& customJointPosition)
	{
		// Joint base created but not activated, return false, but its no error, hence after that return true.
		if (false == JointComponent::createJoint(customJointPosition))
		{
			return true;
		}

		if (nullptr == this->body)
		{
			this->connect();
		}
		this->jointPosition = this->body->getPosition();

		return true;
	}

	void JointAttractorComponent::actualizeValue(Variant* attribute)
	{
		JointComponent::actualizeValue(attribute);

		if (JointAttractorComponent::AttrMagneticStrength() == attribute->getName())
		{
			this->setMagneticStrength(attribute->getReal());
		}
		else if (JointAttractorComponent::AttrAttractionDistance() == attribute->getName())
		{
			this->setAttractionDistance(attribute->getReal());
		}
		else if (JointAttractorComponent::AttrRepellerCategory() == attribute->getName())
		{
			this->setRepellerCategory(attribute->getString());
		}
	}

	void JointAttractorComponent::setMagneticStrength(Ogre::Real magneticStrength)
	{
		this->magneticStrength->setValue(magneticStrength);
	}

	Ogre::Real JointAttractorComponent::getMagneticStrength(void) const
	{
		return this->magneticStrength->getReal();
	}

	void JointAttractorComponent::setAttractionDistance(Ogre::Real attractionDistance)
	{
		this->attractionDistance->setValue(attractionDistance);
	}

	Ogre::Real JointAttractorComponent::getAttractionDistance(void) const
	{
		return this->attractionDistance->getReal();
	}

	void JointAttractorComponent::setRepellerCategory(const Ogre::String& repellerCategory)
	{
		this->repellerCategory->setValue(repellerCategory);
	}

	const Ogre::String JointAttractorComponent::getRepellerCategory(void)
	{
		return this->repellerCategory->getString();
	}

	/*******************************JointCorkScrewComponent*******************************/

	JointCorkScrewComponent::JointCorkScrewComponent()
		: JointComponent()
	{
		// Note that also in JointComponent internalBaseInit() is called, which sets the values for JointComponent, so this is called for already existing values
		// But luckely in Variant, no new attributes are added, but the values changed via interalAdd(...) in Variant
		this->type->setReadOnly(false);
		this->type->setValue(this->getClassName());
		this->type->setReadOnly(true);
		this->type->setDescription("This joint type is an enhanched version of a slider joint which allows the attached child body to not "
			"only slide up and down the axis, but also to rotate around this axis.");

		this->anchorPosition = new Variant(JointCorkScrewComponent::AttrAnchorPosition(), Ogre::Vector3::ZERO, this->attributes);
		this->enableLinearLimits = new Variant(JointCorkScrewComponent::AttrLinearLimits(), false, this->attributes);
		this->minStopDistance = new Variant(JointCorkScrewComponent::AttrMinStopDistance(), 0.0f, this->attributes);
		this->maxStopDistance = new Variant(JointCorkScrewComponent::AttrMaxStopDistance(), 0.0f, this->attributes);
		this->enableAngleLimits = new Variant(JointCorkScrewComponent::AttrAngleLimits(), false, this->attributes);
		this->minAngleLimit = new Variant(JointCorkScrewComponent::AttrMinAngleLimit(), 0.0f, this->attributes);
		this->maxAngleLimit = new Variant(JointCorkScrewComponent::AttrMaxAngleLimit(), 0.0f, this->attributes);

		this->targetId->setVisible(false);
		// this->minStopDistance->setVisible(false);
		// this->maxStopDistance->setVisible(false);
		// this->minAngleLimit->setVisible(false);
		// this->maxAngleLimit->setVisible(false);
	}

	JointCorkScrewComponent::~JointCorkScrewComponent()
	{

	}

	GameObjectCompPtr JointCorkScrewComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		// get also the cloned properties from joint handler base class
		JointCorkScrewCompPtr clonedJointCompPtr(boost::make_shared<JointCorkScrewComponent>());
		
		clonedJointCompPtr->setType(this->type->getString());
		clonedJointCompPtr->internalSetPriorId(this->id->getULong());
		clonedJointCompPtr->setPredecessorId(this->predecessorId->getULong());
		clonedJointCompPtr->setTargetId(this->targetId->getULong());
		clonedJointCompPtr->setJointPosition(this->jointPosition);
		clonedJointCompPtr->setJointRecursiveCollisionEnabled(this->jointRecursiveCollision->getBool());

		clonedJointCompPtr->setAnchorPosition(this->anchorPosition->getVector3());
		clonedJointCompPtr->setLinearLimitsEnabled(this->enableLinearLimits->getBool());
		clonedJointCompPtr->setAngleLimitsEnabled(this->enableAngleLimits->getBool());
		clonedJointCompPtr->setMinMaxStopDistance(this->minStopDistance->getReal(), this->maxStopDistance->getReal());
		clonedJointCompPtr->setMinMaxAngleLimit(Ogre::Degree(this->minAngleLimit->getReal()), Ogre::Degree(this->maxAngleLimit->getReal()));
		clonedJointCompPtr->stiffness->setValue(this->stiffness->getReal());

		clonedGameObjectPtr->addComponent(clonedJointCompPtr);
		clonedJointCompPtr->setOwner(clonedGameObjectPtr);

		clonedJointCompPtr->setActivated(this->activated->getBool());

		return clonedJointCompPtr;
	}

	bool JointCorkScrewComponent::init(xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		JointComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointAnchorPosition")
		{
			this->anchorPosition->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3::ZERO));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointLinearLimits")
		{
			this->setLinearLimitsEnabled(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointAngleLimits")
		{
			this->setAngleLimitsEnabled(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointMinDistance")
		{
			this->minStopDistance->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointMaxDistance")
		{
			this->maxStopDistance->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointMinAngle")
		{
			this->minAngleLimit->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointMaxAngle")
		{
			this->maxAngleLimit->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	bool JointCorkScrewComponent::postInit(void)
	{
		JointComponent::postInit();

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointCorkScrewComponent] Init joint cork screw component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool JointCorkScrewComponent::connect(void)
	{
		bool success = JointComponent::connect();

		this->internalShowDebugData(true, 0, this->anchorPosition->getVector3(), Ogre::Vector3::ZERO);

		return success;
	}

	bool JointCorkScrewComponent::disconnect(void)
	{
		bool success = JointComponent::disconnect();

		this->internalShowDebugData(false, 0, this->anchorPosition->getVector3(), Ogre::Vector3::ZERO);

		return success;
	}

	Ogre::String JointCorkScrewComponent::getClassName(void) const
	{
		return "JointCorkScrewComponent";
	}

	Ogre::String JointCorkScrewComponent::getParentClassName(void) const
	{
		return "JointComponent";
	}

	void JointCorkScrewComponent::update(Ogre::Real dt, bool notSimulating)
	{

	}

	void JointCorkScrewComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		JointComponent::writeXML(propertiesXML, doc, filePath);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointAnchorPosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->anchorPosition->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointLinearLimits"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->enableLinearLimits->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointAngleLimits"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->enableAngleLimits->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointMinDistance"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->minStopDistance->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointMaxDistance"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxStopDistance->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointMinAngle"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->minAngleLimit->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointMaxAngle"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxAngleLimit->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::Vector3 JointCorkScrewComponent::getUpdatedJointPosition(void)
	{
		if (nullptr == this->body)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointCorkScrewComponent] Could not get 'getUpdatedJointPosition', because there is no body for game object: " + this->gameObjectPtr->getName());
			return Ogre::Vector3::ZERO;
		}

		if (false == this->hasCustomJointPosition)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			return this->body->getPosition() + offset;
		}
		else
		{
			return this->jointPosition;
		}
	}
	
	bool JointCorkScrewComponent::createJoint(const Ogre::Vector3& customJointPosition)
	{
		// Joint base created but not activated, return false, but its no error, hence after that return true.
		if (false == JointComponent::createJoint(customJointPosition))
		{
			return true;
		}

		if (true == this->jointAlreadyCreated)
		{
			// joint already created so skip
			return true;
		}
		if (nullptr == this->body)
		{
			this->connect();
		}
		if (Ogre::Vector3::ZERO == customJointPosition)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			//Ogre::Vector3 aPos = Ogre::Vector3(-this->anchorPosition.x, -this->anchorPosition.y, -this->anchorPosition.z);
			//// relative anchor pos: 0.5 0 0 means 50% in X of the size of the parent object
			//Ogre::Vector3 offset = (aPos * size);
			//// take orientation into account, so that orientated joints are processed correctly
			//this->jointPosition = this->body->getPosition() - (this->body->getOrientation() * offset);

			// without rotation
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			this->jointPosition = (this->body->getPosition() + offset);
		}
		else
		{
			this->hasCustomJointPosition = true;
			this->jointPosition = customJointPosition;
		}

		OgreNewt::Body* predecessorBody = nullptr;
		if (this->predecessorJointCompPtr)
		{
			predecessorBody = this->predecessorJointCompPtr->getBody();
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[JointCorkScrewComponent] Creating cork screw joint for body1 name: "
				+ this->predecessorJointCompPtr->getOwner()->getName() + " body2 name: " + this->gameObjectPtr->getName());
		}

		// Release joint each time, to create new one with new values
		this->internalReleaseJoint();
		this->joint = new OgreNewt::CorkScrew(this->body, predecessorBody, this->jointPosition);
		OgreNewt::CorkScrew* corkScrewJoint = dynamic_cast<OgreNewt::CorkScrew*>(this->joint);

		this->joint->setBodyMassScale(this->bodyMassScale->getVector2().x, this->bodyMassScale->getVector2().y);
		this->joint->setCollisionState(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		// this->body->setJointRecursiveCollision(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		// this->applyStiffness();

		corkScrewJoint->enableLimits(this->enableLinearLimits->getBool(), this->enableAngleLimits->getBool());
		if (true == this->enableLinearLimits->getBool())
		{
			/*if (this->minStopDistance == this->maxStopDistance)
			{
				throw Ogre::Exception(Ogre::Exception::ERR_INVALIDPARAMS, "[PhysicsComponent] The hinge joint: "
					+  " cannot have the same min linear angle limit as max linear angle limit!\n", "NOWA");
			}*/
			corkScrewJoint->setLinearLimits(this->minStopDistance->getReal(), this->maxStopDistance->getReal());
		}
		/*else
		{
			corkScrewJoint->setLinearLimits(Ogre::Degree(-359.0f), Ogre::Degree(359.0f));
		}*/
		if (true == this->enableAngleLimits->getBool())
		{
			/*if (this->minAngleLimit == this->maxAngleLimit)
			{
				throw Ogre::Exception(Ogre::Exception::ERR_INVALIDPARAMS, "[PhysicsComponent] The hinge joint: "
					+  " cannot have the same min angular angle limit as max angular angle limit!\n", "NOWA");
			}*/
			corkScrewJoint->setAngularLimits(Ogre::Degree(this->minAngleLimit->getReal()), Ogre::Degree(this->maxAngleLimit->getReal()));
		}
		/*else
		{
			corkScrewJoint->setAngleLimits(Ogre::Degree(-360.0f), Ogre::Degree(360.0f));
		}*/

		this->jointAlreadyCreated = true;

		return true;
	}

	void JointCorkScrewComponent::forceShowDebugData(bool activate)
	{
		this->internalShowDebugData(activate, 0, this->anchorPosition->getVector3(), Ogre::Vector3::ZERO);
	}

	void JointCorkScrewComponent::actualizeValue(Variant* attribute)
	{
		JointComponent::actualizeValue(attribute);

		if (JointCorkScrewComponent::AttrAnchorPosition() == attribute->getName())
		{
			this->setAnchorPosition(attribute->getVector3());
		}
		else if (JointCorkScrewComponent::AttrLinearLimits() == attribute->getName())
		{
			this->setLinearLimitsEnabled(attribute->getBool());
		}
		else if (JointCorkScrewComponent::AttrAngleLimits() == attribute->getName())
		{
			this->setAngleLimitsEnabled(attribute->getBool());
		}
		else if (JointCorkScrewComponent::AttrMinStopDistance() == attribute->getName())
		{
			this->setMinMaxStopDistance(attribute->getReal(), this->maxStopDistance->getReal());
		}
		else if (JointCorkScrewComponent::AttrMaxStopDistance() == attribute->getName())
		{
			this->setMinMaxStopDistance(this->minStopDistance->getReal(), attribute->getReal());
		}
		else if (JointCorkScrewComponent::AttrMinAngleLimit() == attribute->getName())
		{
			this->setMinMaxAngleLimit(Ogre::Degree(attribute->getReal()), Ogre::Degree(this->maxAngleLimit->getReal()));
		}
		else if (JointCorkScrewComponent::AttrMaxAngleLimit() == attribute->getName())
		{
			this->setMinMaxAngleLimit(Ogre::Degree(this->minAngleLimit->getReal()), Ogre::Degree(attribute->getReal()));
		}
	}

	void JointCorkScrewComponent::setAnchorPosition(const Ogre::Vector3& anchorPosition)
	{
		this->anchorPosition->setValue(anchorPosition);
		this->jointAlreadyCreated = false;
		if (nullptr != this->debugGeometryNode)
		{
			this->debugGeometryNode->setPosition(anchorPosition);
		}
	}

	Ogre::Vector3 JointCorkScrewComponent::getAnchorPosition(void) const
	{
		return this->anchorPosition->getVector3();
	}

	void JointCorkScrewComponent::setLinearLimitsEnabled(bool enableLinearLimits)
	{
		this->enableLinearLimits->setValue(enableLinearLimits);
		// this->minStopDistance->setVisible(true == enableLinearLimits);
		// this->maxStopDistance->setVisible(true == enableLinearLimits);
	}

	bool JointCorkScrewComponent::getLinearLimitsEnabled(void) const
	{
		return this->enableLinearLimits->getBool();
	}

	void JointCorkScrewComponent::setAngleLimitsEnabled(bool enableAngleLimits)
	{
		this->enableAngleLimits->setValue(enableAngleLimits);
		// this->minAngleLimit->setVisible(true == enableAngleLimits);
		// this->maxAngleLimit->setVisible(true == enableAngleLimits);
	}

	bool JointCorkScrewComponent::getAngleLimitsEnabled(void) const
	{
		return this->enableAngleLimits->getBool();
	}

	void JointCorkScrewComponent::setMinMaxStopDistance(Ogre::Real minStopDistance, Ogre::Real maxStopDistance)
	{
		this->minStopDistance->setValue(minStopDistance);
		this->maxStopDistance->setValue(maxStopDistance);

		OgreNewt::CorkScrew* corkScrewJoint = dynamic_cast<OgreNewt::CorkScrew*>(this->joint);

		if (corkScrewJoint && true == this->enableLinearLimits->getBool())
		{
			corkScrewJoint->setLinearLimits(this->minStopDistance->getReal(), this->maxStopDistance->getReal());
		}
	}
	
	Ogre::Real JointCorkScrewComponent::getMinStopDistance(void) const
	{
		return this->minStopDistance->getReal();
	}
	
	Ogre::Real JointCorkScrewComponent::getMaxStopDistance(void) const
	{
		return this->maxStopDistance->getReal();
	}
	
	void JointCorkScrewComponent::setMinMaxAngleLimit(const Ogre::Degree& minAngleLimit, const Ogre::Degree& maxAngleLimit)
	{
		this->minAngleLimit->setValue(minAngleLimit.valueDegrees());
		this->maxAngleLimit->setValue(maxAngleLimit.valueDegrees());

		OgreNewt::CorkScrew* corkScrewJoint = dynamic_cast<OgreNewt::CorkScrew*>(this->joint);

		if (corkScrewJoint && true == this->enableAngleLimits->getBool())
		{
			corkScrewJoint->setAngularLimits(Ogre::Degree(this->minAngleLimit->getReal()), Ogre::Degree(this->maxAngleLimit->getReal()));
		}
	}
	
	Ogre::Degree JointCorkScrewComponent::getMinAngleLimit(void) const
	{
		return Ogre::Degree(this->minAngleLimit->getReal());
	}
	
	Ogre::Degree JointCorkScrewComponent::getMaxAngleLimit(void) const
	{
		return Ogre::Degree(this->maxAngleLimit->getReal());
	}
	
	/*******************************JointPassiveSliderComponent*******************************/

	JointPassiveSliderComponent::JointPassiveSliderComponent()
		: JointComponent()
	{
		// Note that also in JointComponent internalBaseInit() is called, which sets the values for JointComponent, so this is called for already existing values
		// But luckely in Variant, no new attributes are added, but the values changed via interalAdd(...) in Variant
		this->type->setReadOnly(false);
		this->type->setValue(this->getClassName());
		this->type->setReadOnly(true);
		this->type->setDescription("A child body attached via a slider joint can only slide up and down (move along) the axis it is attached to.");

		this->anchorPosition = new Variant(JointPassiveSliderComponent::AttrAnchorPosition(), Ogre::Vector3::ZERO, this->attributes);
		this->pin = new Variant(JointPassiveSliderComponent::AttrPin(), Ogre::Vector3::UNIT_Y, this->attributes);
		this->enableLimits = new Variant(JointPassiveSliderComponent::AttrLimits(), false, this->attributes);
		this->minStopDistance = new Variant(JointPassiveSliderComponent::AttrMinStopDistance(), 0.0f, this->attributes);
		this->maxStopDistance = new Variant(JointPassiveSliderComponent::AttrMaxStopDistance(), 0.0f, this->attributes);

		this->asSpringDamper = new Variant(JointPassiveSliderComponent::AttrAsSpringDamper(), false, this->attributes);
		this->springK = new Variant(JointPassiveSliderComponent::AttrSpringK(), 128.0f, this->attributes);
		this->springD = new Variant(JointPassiveSliderComponent::AttrSpringD(), 4.0f, this->attributes);
		this->springDamperRelaxation = new Variant(JointPassiveSliderComponent::AttrSpringDamperRelaxation(), 0.6f, this->attributes);

		this->targetId->setVisible(false);
		// this->minStopDistance->setVisible(false);
		// this->maxStopDistance->setVisible(false);
		// this->asSpringDamper->setVisible(false);
		// this->springK->setVisible(false);
		// this->springD->setVisible(false);
		// this->springDamperRelaxation->setVisible(false);
		this->springDamperRelaxation->setDescription("[0 - 0.99]");
	}

	JointPassiveSliderComponent::~JointPassiveSliderComponent()
	{

	}

	GameObjectCompPtr JointPassiveSliderComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		// get also the cloned properties from joint handler base class
		JointPassiveSliderCompPtr clonedJointCompPtr(boost::make_shared<JointPassiveSliderComponent>());
		
		clonedJointCompPtr->setType(this->type->getString());
		clonedJointCompPtr->internalSetPriorId(this->id->getULong());
		clonedJointCompPtr->setPredecessorId(this->predecessorId->getULong());
		clonedJointCompPtr->setTargetId(this->targetId->getULong());
		clonedJointCompPtr->setJointPosition(this->jointPosition);
		clonedJointCompPtr->setJointRecursiveCollisionEnabled(this->jointRecursiveCollision->getBool());
		clonedJointCompPtr->stiffness->setValue(this->stiffness->getReal());

		clonedJointCompPtr->setAnchorPosition(this->anchorPosition->getVector3());
		clonedJointCompPtr->setPin(this->pin->getVector3());
		clonedJointCompPtr->setLimitsEnabled(this->enableLimits->getBool());
		clonedJointCompPtr->setMinMaxStopDistance(this->minStopDistance->getReal(), this->maxStopDistance->getReal());
		clonedJointCompPtr->setSpring(this->asSpringDamper->getBool(), this->springDamperRelaxation->getReal(), this->springK->getReal(), this->springD->getReal());

		clonedGameObjectPtr->addComponent(clonedJointCompPtr);
		clonedJointCompPtr->setOwner(clonedGameObjectPtr);

		clonedJointCompPtr->setActivated(this->activated->getBool());

		return clonedJointCompPtr;
	}

	bool JointPassiveSliderComponent::init(xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		JointComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointAnchorPosition")
		{
			this->anchorPosition->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(0.0f, 0.0f, 0.0f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointPin")
		{
			this->pin->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(0.0f, 0.0f, 0.0f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointLimits")
		{
			this->setLimitsEnabled(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointMinStopDistance")
		{
			this->minStopDistance->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointMaxStopDistance")
		{
			this->maxStopDistance->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointAsSpringDamper")
		{
			this->asSpringDamper->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointKSpring")
		{
			this->springK->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointDSpring")
		{
			this->springD->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointSpringDamperRelaxation")
		{
			this->springDamperRelaxation->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	bool JointPassiveSliderComponent::postInit(void)
	{
		JointComponent::postInit();

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointPassiveSliderComponent] Init joint passive slider component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool JointPassiveSliderComponent::connect(void)
	{
		bool success = JointComponent::connect();
		return success;
	}

	bool JointPassiveSliderComponent::disconnect(void)
	{
		bool success = JointComponent::disconnect();
		return success;
	}

	Ogre::String JointPassiveSliderComponent::getClassName(void) const
	{
		return "JointPassiveSliderComponent";
	}

	Ogre::String JointPassiveSliderComponent::getParentClassName(void) const
	{
		return "JointComponent";
	}

	void JointPassiveSliderComponent::update(Ogre::Real dt, bool notSimulating)
	{

	}

	void JointPassiveSliderComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		JointComponent::writeXML(propertiesXML, doc, filePath);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointAnchorPosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->anchorPosition->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointPin"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->pin->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointLimits"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->enableLimits->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointMinStopDistance"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->minStopDistance->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointMaxStopDistance"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxStopDistance->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointAsSpringDamper"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->asSpringDamper->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointKSpring"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->springK->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointDSpring"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->springD->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointSpringDamperRelaxation"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->springDamperRelaxation->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::Vector3 JointPassiveSliderComponent::getUpdatedJointPosition(void)
	{
		if (nullptr == this->body)
		{
			return Ogre::Vector3::ZERO;
		}

		if (false == this->hasCustomJointPosition)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			return this->body->getPosition() + offset;
		}
		else
		{
			return this->jointPosition;
		}
	}

	bool JointPassiveSliderComponent::createJoint(const Ogre::Vector3& customJointPosition)
	{
		// Joint base created but not activated, return false, but its no error, hence after that return true.
		if (false == JointComponent::createJoint(customJointPosition))
		{
			return true;
		}

		if (true == this->jointAlreadyCreated)
		{
			// joint already created so skip
			return true;
		}
		if (nullptr == this->body)
		{
			this->connect();
		}
		if (Ogre::Vector3::ZERO == this->pin->getVector3())
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[JointPassiveSliderComponent] Cannot create joint for: " + this->gameObjectPtr->getName() + " because the pin is zero.");
			return false;
		}

		if (Ogre::Vector3::ZERO == customJointPosition)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			//Ogre::Vector3 aPos = Ogre::Vector3(-this->anchorPosition.x, -this->anchorPosition.y, -this->anchorPosition.z);
			//// relative anchor pos: 0.5 0 0 means 50% in X of the size of the parent object
			//Ogre::Vector3 offset = (aPos * size);
			//// take orientation into account, so that orientated joints are processed correctly
			//this->jointPosition = this->body->getPosition() - (this->body->getOrientation() * offset);

			// without rotation
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			this->jointPosition = (this->body->getPosition() + offset);
		}
		else
		{
			this->hasCustomJointPosition = true;
			this->jointPosition = customJointPosition;
		}
		OgreNewt::Body* predecessorBody = nullptr;
		
		if (this->predecessorJointCompPtr)
		{
			predecessorBody = this->predecessorJointCompPtr->getBody();
		}

		// Will cause crash!
		if (this->body == predecessorBody)
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[JointPassiveSliderComponent] Cannot create joint for: " + this->gameObjectPtr->getName() + " because the predecessor body and the body are the same!");
			return false;
		}

		// Release joint each time, to create new one with new values
		this->internalReleaseJoint();
		this->joint = new OgreNewt::Slider(this->body, predecessorBody, this->jointPosition, this->body->getOrientation() * this->pin->getVector3());
		OgreNewt::Slider* passiveSlider = dynamic_cast<OgreNewt::Slider*>(this->joint);

		this->joint->setBodyMassScale(this->bodyMassScale->getVector2().x, this->bodyMassScale->getVector2().y);
		this->joint->setCollisionState(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		// this->body->setJointRecursiveCollision(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		// this->applyStiffness();

		/*passiveSlider->setStiffness(1.0f);
		passiveSlider->setRowMaximumFriction(1.0f);
		passiveSlider->setRowMaximumFriction(1.0f);*/

		passiveSlider->setFriction(10.0f);
		passiveSlider->setSpringAndDamping(this->asSpringDamper->getBool(), this->springDamperRelaxation->getReal(), this->springK->getReal(), this->springD->getReal());

		passiveSlider->enableLimits(this->enableLimits->getBool());
		if (true == this->enableLimits->getBool())
		{
			if (0.0f != this->minStopDistance->getReal() || 0.0f != this->maxStopDistance->getReal())
			{
				passiveSlider->setLimits(this->minStopDistance->getReal(), this->maxStopDistance->getReal());
			}
		}
		
		this->jointAlreadyCreated = true;

		return true;
	}

	void JointPassiveSliderComponent::actualizeValue(Variant* attribute)
	{
		JointComponent::actualizeValue(attribute);

		if (JointPassiveSliderComponent::AttrAnchorPosition() == attribute->getName())
		{
			this->setAnchorPosition(attribute->getVector3());
		}
		else if (JointPassiveSliderComponent::AttrPin() == attribute->getName())
		{
			this->setPin(attribute->getVector3());
		}
		else if (JointPassiveSliderComponent::AttrLimits() == attribute->getName())
		{
			this->setLimitsEnabled(attribute->getBool());
		}
		else if (JointPassiveSliderComponent::AttrMinStopDistance() == attribute->getName())
		{
			this->setMinMaxStopDistance(attribute->getReal(), this->maxStopDistance->getReal());
		}
		else if (JointPassiveSliderComponent::AttrMaxStopDistance() == attribute->getName())
		{
			this->setMinMaxStopDistance(this->minStopDistance->getReal(), attribute->getReal());
		}
		else if (JointPassiveSliderComponent::AttrAsSpringDamper() == attribute->getName())
		{
			this->setSpring(attribute->getBool(), this->springDamperRelaxation->getReal(), this->springK->getReal(), this->springD->getReal());
		}
		else if (JointPassiveSliderComponent::AttrSpringDamperRelaxation() == attribute->getName())
		{
			this->setSpring(this->asSpringDamper->getBool(), attribute->getReal(), this->springK->getReal(), this->springD->getReal());
		}
		else if (JointPassiveSliderComponent::AttrSpringK() == attribute->getName())
		{
			this->setSpring(this->asSpringDamper->getBool(), this->springDamperRelaxation->getReal(), attribute->getReal(), this->springD->getReal());
		}
		else if (JointPassiveSliderComponent::AttrSpringD() == attribute->getName())
		{
			this->setSpring(this->asSpringDamper->getBool(), this->springDamperRelaxation->getReal(), this->springK->getReal(), attribute->getReal());
		}
	}

	bool JointPassiveSliderComponent::canStaticAddComponent(GameObject* gameObject)
	{
		auto jointComponent = NOWA::makeStrongPtr(gameObject->getComponent<JointPassiveSliderComponent>());
		if (nullptr == jointComponent)
		{
			return true;
		}
		return false;
	}

	void JointPassiveSliderComponent::setAnchorPosition(const Ogre::Vector3& anchorPosition)
	{
		this->anchorPosition->setValue(anchorPosition);
		this->jointAlreadyCreated = false;
	}

	Ogre::Vector3 JointPassiveSliderComponent::getAnchorPosition(void) const
	{
		return this->anchorPosition->getVector3();
	}

	void JointPassiveSliderComponent::setPin(const Ogre::Vector3& pin)
	{
		this->pin->setValue(pin);
	}

	Ogre::Vector3 JointPassiveSliderComponent::getPin(void) const
	{
		return this->pin->getVector3();
	}

	void JointPassiveSliderComponent::setLimitsEnabled(bool enableLimits)
	{
		this->enableLimits->setValue(enableLimits);
		// this->minStopDistance->setVisible(true == enableLimits);
		// this->maxStopDistance->setVisible(true == enableLimits);
	}

	bool JointPassiveSliderComponent::getLimitsEnabled(void) const
	{
		return this->enableLimits->getBool();
	}

	void JointPassiveSliderComponent::setMinMaxStopDistance(Ogre::Real minStopDistance, Ogre::Real maxStopDistance)
	{
		this->minStopDistance->setValue(minStopDistance);
		this->maxStopDistance->setValue(maxStopDistance);

		OgreNewt::Slider* passiveSlider = dynamic_cast<OgreNewt::Slider*>(this->joint);

		if (nullptr != passiveSlider && true == this->enableLimits->getBool())
		{
			if (this->minStopDistance->getReal() == this->maxStopDistance->getReal())
			{
				// throw Ogre::Exception(Ogre::Exception::ERR_INVALIDPARAMS, "[JointPassiveSliderComponent] The passive slider joint cannot have the same min angle limit as max angle limit!\n", "NOWA");
			}
			passiveSlider->setLimits(minStopDistance, maxStopDistance);
		}
	}
	
	Ogre::Real JointPassiveSliderComponent::getMinStopDistance(void) const
	{
		return this->minStopDistance->getReal();
	}
	
	Ogre::Real JointPassiveSliderComponent::getMaxStopDistance(void) const
	{
		return this->maxStopDistance->getReal();
	}

	void JointPassiveSliderComponent::setSpring(bool asSpringDamper)
	{
		this->asSpringDamper->setValue(asSpringDamper);
		OgreNewt::Slider* passiveSlider = dynamic_cast<OgreNewt::Slider*>(this->joint);
		if (nullptr != passiveSlider)
		{
			passiveSlider->setSpringAndDamping(this->asSpringDamper->getBool(), this->springDamperRelaxation->getReal(), this->springK->getReal(), this->springD->getReal());
		}
	}
	
	void JointPassiveSliderComponent::setSpring(bool asSpringDamper, Ogre::Real springDamperRelaxation, Ogre::Real springK, Ogre::Real springD)
	{
		// this->springK->setVisible(true == asSpringDamper);
		// this->springD->setVisible(true == asSpringDamper);
		// this->springDamperRelaxation->setVisible(true == asSpringDamper);
		this->asSpringDamper->setValue(asSpringDamper);
		this->springDamperRelaxation->setValue(springDamperRelaxation);
		this->springK->setValue(springK);
		this->springD->setValue(springD);
		OgreNewt::Slider* passiveSlider = dynamic_cast<OgreNewt::Slider*>(this->joint);
		if (nullptr != passiveSlider)
		{
			passiveSlider->setSpringAndDamping(this->asSpringDamper->getBool(), this->springDamperRelaxation->getReal(), this->springK->getReal(), this->springD->getReal());
		}
	}

	std::tuple<bool, Ogre::Real, Ogre::Real, Ogre::Real> JointPassiveSliderComponent::getSpring(void) const
	{
		return std::make_tuple(this->asSpringDamper->getBool(), this->springDamperRelaxation->getReal(), this->springK->getReal(), this->springD->getReal());
	}
	
	/*******************************JointSliderActuatorComponent*******************************/

	JointSliderActuatorComponent::JointSliderActuatorComponent()
		: JointComponent(),
		round(0),
		internalDirectionChange(false),
		oppositeDir(1.0f)
	{
		// Note that also in JointComponent internalBaseInit() is called, which sets the values for JointComponent, so this is called for already existing values
		// But luckely in Variant, no new attributes are added, but the values changed via interalAdd(...) in Variant
		this->type->setReadOnly(false);
		this->type->setValue(this->getClassName());
		this->type->setReadOnly(true);
		this->type->setDescription("An object attached to a hinge actuator joint can only rotate around one dimension perpendicular to the axis it is attached to. A good a precise actuator motor.");
		
		this->anchorPosition = new Variant(JointSliderActuatorComponent::AttrAnchorPosition(), Ogre::Vector3::ZERO, this->attributes);
		this->pin = new Variant(JointSliderActuatorComponent::AttrPin(), Ogre::Vector3(1.0f, 0.0f, 0.0f), this->attributes);
		
		this->targetPosition  = new Variant(JointSliderActuatorComponent::AttrTargetPosition(), 10.0f, this->attributes);
		this->linearRate  = new Variant(JointSliderActuatorComponent::AttrLinearRate(), 1.0f, this->attributes);
		this->minForce = new Variant(JointSliderActuatorComponent::AttrMinForce(), 1.0f, this->attributes); // disabled by default, so that max is used in newton
		this->maxForce = new Variant(JointSliderActuatorComponent::AttrMaxForce(), -1.0f, this->attributes); // disabled by default, so that max is used in newton
		
		this->minStopDistance = new Variant(JointSliderActuatorComponent::AttrMinStopDistance(), -10.0f, this->attributes);
		this->maxStopDistance = new Variant(JointSliderActuatorComponent::AttrMaxStopDistance(), 10.0f, this->attributes);
		
		this->directionChange = new Variant(JointSliderActuatorComponent::AttrDirectionChange(), false, this->attributes);
		this->repeat = new Variant(JointSliderActuatorComponent::AttrRepeat(), false, this->attributes);

		this->directionChange->addUserData(GameObject::AttrActionNeedRefresh());
		this->repeat->addUserData(GameObject::AttrActionNeedRefresh());

		this->internalDirectionChange = this->directionChange->getBool();

		this->targetId->setVisible(false);
	}

	JointSliderActuatorComponent::~JointSliderActuatorComponent()
	{
		
	}

	GameObjectCompPtr JointSliderActuatorComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		JointSliderActuatorCompPtr clonedJointCompPtr(boost::make_shared<JointSliderActuatorComponent>());
		
		clonedJointCompPtr->setType(this->type->getString());
		clonedJointCompPtr->internalSetPriorId(this->id->getULong());
		clonedJointCompPtr->setPredecessorId(this->predecessorId->getULong());
		clonedJointCompPtr->setTargetId(this->targetId->getULong());
		clonedJointCompPtr->setJointPosition(this->jointPosition);
		clonedJointCompPtr->setJointRecursiveCollisionEnabled(this->jointRecursiveCollision->getBool());
		// clonedJointCompPtr->stiffness->setValue(this->stiffness->getReal()); // Do not call setter, because internally a bool flag is set, so that stiffness is always used

		clonedJointCompPtr->setAnchorPosition(this->anchorPosition->getVector3());
		clonedJointCompPtr->setPin(this->pin->getVector3());
		clonedJointCompPtr->setTargetPosition(this->targetPosition->getReal());
		clonedJointCompPtr->setLinearRate(this->linearRate->getReal());
		clonedJointCompPtr->setMinStopDistance(this->minStopDistance->getReal());
		clonedJointCompPtr->setMaxStopDistance(this->maxStopDistance->getReal());
		clonedJointCompPtr->setMinForce(this->minForce->getReal());
		clonedJointCompPtr->setMaxForce(this->maxForce->getReal());
		clonedJointCompPtr->setDirectionChange(this->directionChange->getBool());
		clonedJointCompPtr->setRepeat(this->repeat->getBool());

		clonedGameObjectPtr->addComponent(clonedJointCompPtr);
		clonedJointCompPtr->setOwner(clonedGameObjectPtr);

		clonedJointCompPtr->setActivated(this->activated->getBool());

		return clonedJointCompPtr;
	}

	bool JointSliderActuatorComponent::postInit(void)
	{
		JointComponent::postInit();

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointSliderActuatorComponent] Init joint slider actuator component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool JointSliderActuatorComponent::connect(void)
	{
		bool success = JointComponent::connect();
		this->round = 0;
		this->internalDirectionChange = this->directionChange->getBool();
		this->oppositeDir = 1.0f;

		return success;
	}

	bool JointSliderActuatorComponent::disconnect(void)
	{
		bool success = JointComponent::disconnect();
		this->round = 0;
		this->internalDirectionChange = this->directionChange->getBool();
		this->oppositeDir = 1.0f;

		return success;
	}
	
	void JointSliderActuatorComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating && true == this->activated->getBool())
		{
			if (true == this->internalDirectionChange)
			{
				OgreNewt::SliderActuator* sliderActuatorJoint = static_cast<OgreNewt::SliderActuator*>(this->joint);
				if (nullptr != sliderActuatorJoint)
				{
					Ogre::Real position = sliderActuatorJoint->GetActuatorPosition();
					Ogre::Real step = this->linearRate->getReal() * dt;
					if (1.0f == this->oppositeDir)
					{
						if (Ogre::Math::RealEqual(position, this->maxStopDistance->getReal(), step))
						{
							this->oppositeDir *= -1.0f;
							this->round++;
						}
					}
					else
					{
						if (Ogre::Math::RealEqual(position, this->minStopDistance->getReal(), step))
						{
							this->oppositeDir *= -1.0f;
							this->round++;
						}
					}
				}

				// If the hinge took 2 rounds (1x forward and 1x back, then its enough, if repeat is off)
				if (this->round == 2)
				{
					this->round = 0;
					// if repeat is of, only change the direction one time, to get back to its origin and leave
					if (false == this->repeat->getBool())
					{
						this->internalDirectionChange = false;
					}
				}

				if (true == this->repeat->getBool() || true == this->internalDirectionChange)
				{
					Ogre::Real newPosition = 0.0f;

					if (1.0f == this->oppositeDir)
						newPosition = this->maxStopDistance->getReal();
					else
						newPosition = this->minStopDistance->getReal();

					sliderActuatorJoint->SetTargetPosition(newPosition);

					if (this->targetPositionReachedClosureFunction.is_valid())
					{
						try
						{
							luabind::call_function<void>(this->targetPositionReachedClosureFunction, newPosition);
						}
						catch (luabind::error& error)
						{
							luabind::object errorMsg(luabind::from_stack(error.state(), -1));
							std::stringstream msg;
							msg << errorMsg;

							Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[JointSliderActuatorComponent] Caught error in 'reactOnTargetPositionReached' Error: " + Ogre::String(error.what())
																		+ " details: " + msg.str());
						}
					}
				}
			}
		}
	}

	Ogre::String JointSliderActuatorComponent::getClassName(void) const
	{
		return "JointSliderActuatorComponent";
	}

	Ogre::String JointSliderActuatorComponent::getParentClassName(void) const
	{
		return "JointComponent";
	}

	bool JointSliderActuatorComponent::init(xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		JointComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnchorPosition")
		{
			this->anchorPosition->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Pin")
		{
			this->pin->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetPosition")
		{
			this->targetPosition->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "LinearRate")
		{
			this->linearRate->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MinStopDistance")
		{
			this->minStopDistance->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxStopDistance")
		{
			this->maxStopDistance->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MinForce")
		{
			this->minForce->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxForce")
		{
			this->maxForce->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DirectionChange")
		{
			this->directionChange->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Repeat")
		{
			this->repeat->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		return true;
	}

	void JointSliderActuatorComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		JointComponent::writeXML(propertiesXML, doc, filePath);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnchorPosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->anchorPosition->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Pin"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->pin->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TargetPosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->targetPosition->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "LinearRate"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->linearRate->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MinStopDistance"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->minStopDistance->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MaxStopDistance"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxStopDistance->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MinForce"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->minForce->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MaxForce"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxForce->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DirectionChange"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->directionChange->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Repeat"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->repeat->getBool())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::Vector3 JointSliderActuatorComponent::getUpdatedJointPosition(void)
	{
		if (nullptr == this->body)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointSliderActuatorComponent] Could not get 'getUpdatedJointPosition', because there is no body for game object: " + this->gameObjectPtr->getName());
			return Ogre::Vector3::ZERO;
		}

		if (false == this->hasCustomJointPosition)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			return this->body->getPosition() + offset;
		}
		else
		{
			return this->jointPosition;
		}
	}

	bool JointSliderActuatorComponent::createJoint(const Ogre::Vector3& customJointPosition)
	{
		// Joint base created but not activated, return false, but its no error, hence after that return true.
		if (false == JointComponent::createJoint(customJointPosition))
		{
			return true;
		}

		if (nullptr == this->body)
		{
			this->connect();
		}
		if (Ogre::Vector3::ZERO == this->pin->getVector3())
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[JointSliderActuatorComponent] Cannot create joint for: " + this->gameObjectPtr->getName() + " because the pin is zero.");
			return false;
		}

		if (Ogre::Vector3::ZERO == customJointPosition)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			//Ogre::Vector3 aPos = Ogre::Vector3(-this->anchorPosition.x, -this->anchorPosition.y, -this->anchorPosition.z);
			//// relative anchor pos: 0.5 0 0 means 50% in X of the size of the parent object
			//Ogre::Vector3 offset = (aPos * size);
			//// take orientation into account, so that orientated joints are processed correctly
			//this->jointPosition = this->body->getPosition() - (this->body->getOrientation() * offset);

			// without rotation
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			this->jointPosition = (this->body->getPosition() + offset);
		}
		else
		{
			this->hasCustomJointPosition = true;
			this->jointPosition = customJointPosition;
		}

		this->round = 0;
		this->internalDirectionChange = this->directionChange->getBool();
		this->oppositeDir = 1.0f;

		OgreNewt::Body* predecessorBody = nullptr;
		if (this->predecessorJointCompPtr)
		{
			predecessorBody = this->predecessorJointCompPtr->getBody();
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[JointSliderActuatorComponent] Creating slider actuator joint for body1 name: "
				+ this->predecessorJointCompPtr->getOwner()->getName() + " body2 name: " + this->gameObjectPtr->getName());
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointSliderActuatorComponent] Created slider actuator joint: " + this->gameObjectPtr->getName() + " with world as parent");
		}

		// Release joint each time, to create new one with new values
		this->internalReleaseJoint();
		this->joint = new OgreNewt::SliderActuator(this->body, predecessorBody, this->jointPosition, this->body->getOrientation() * this->pin->getVector3(), 
												this->linearRate->getReal(), this->minStopDistance->getReal(), this->maxStopDistance->getReal());

		this->joint->setBodyMassScale(this->bodyMassScale->getVector2().x, this->bodyMassScale->getVector2().y);
		// Bad, because causing jerky behavior on ragdolls?
		this->joint->setCollisionState(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		// this->body->setJointRecursiveCollision(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		// this->applyStiffness();
// Attention: Just a test!
		// this->joint->setStiffness(0.1f);

		OgreNewt::SliderActuator* sliderActuatorJoint = dynamic_cast<OgreNewt::SliderActuator*>(this->joint);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointSliderActuatorComponent] Created joint: " + this->gameObjectPtr->getName());

		// Does not work and is unnecessary
		if (this->minForce->getReal() < 0.0f)
		{
			sliderActuatorJoint->SetMinForce(this->minForce->getReal());
		}
		if (this->maxForce->getReal() > 0.0f)
		{
			sliderActuatorJoint->SetMaxForce(this->maxForce->getReal());
		}
		
		this->setDirectionChange(this->directionChange->getBool());
		this->setRepeat(this->repeat->getBool());

		if (true == this->targetPosition->isVisible() && true == this->activated->getBool())
		{
			sliderActuatorJoint->SetTargetPosition(this->targetPosition->getReal());
		}

		this->jointAlreadyCreated = true;

		return true;
	}

	void JointSliderActuatorComponent::actualizeValue(Variant* attribute)
	{
		JointComponent::actualizeValue(attribute);
		
		if (JointSliderActuatorComponent::AttrAnchorPosition() == attribute->getName())
		{
			this->setAnchorPosition(attribute->getVector3());
		}
		else if (JointSliderActuatorComponent::AttrPin() == attribute->getName())
		{
			this->setPin(attribute->getVector3());
		}
		else if (JointSliderActuatorComponent::AttrTargetPosition() == attribute->getName())
		{
			this->setTargetPosition(attribute->getReal());
		}
		else if (JointSliderActuatorComponent::AttrLinearRate() == attribute->getName())
		{
			this->setLinearRate(attribute->getReal());
		}
		else if (JointSliderActuatorComponent::AttrMinStopDistance() == attribute->getName())
		{
			this->setMinStopDistance(attribute->getReal());
		}
		else if (JointSliderActuatorComponent::AttrMaxStopDistance() == attribute->getName())
		{
			this->setMaxStopDistance(attribute->getReal());
		}
		else if (JointSliderActuatorComponent::AttrMinForce() == attribute->getName())
		{
			this->setMinForce(attribute->getReal());
		}
		else if (JointSliderActuatorComponent::AttrMaxForce() == attribute->getName())
		{
			this->setMaxForce(attribute->getReal());
		}
		else if (JointSliderActuatorComponent::AttrDirectionChange() == attribute->getName())
		{
			this->setDirectionChange(attribute->getBool());
		}
		else if (JointSliderActuatorComponent::AttrRepeat() == attribute->getName())
		{
			this->setRepeat(attribute->getBool());
		}
	}

	void JointSliderActuatorComponent::setAnchorPosition(const Ogre::Vector3& anchorPosition)
	{
		this->anchorPosition->setValue(anchorPosition);

		if (nullptr != this->body)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			//Ogre::Vector3 aPos = Ogre::Vector3(-this->anchorPosition.x, -this->anchorPosition.y, -this->anchorPosition.z);
			//// relative anchor pos: 0.5 0 0 means 50% in X of the size of the parent object
			//Ogre::Vector3 offset = (aPos * size);
			//// take orientation into account, so that orientated joints are processed correctly
			//this->jointPosition = this->body->getPosition() - (this->body->getOrientation() * offset);

			// without rotation
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			this->jointPosition = (this->body->getPosition() + offset);
			this->jointAlreadyCreated = false;
		}

		if (nullptr != this->debugGeometryNode)
		{
			this->debugGeometryNode->setPosition(this->jointPosition);
		}
	}

	Ogre::Vector3 JointSliderActuatorComponent::getAnchorPosition(void) const
	{
		return this->anchorPosition->getVector3();
	}

	void JointSliderActuatorComponent::setPin(const Ogre::Vector3& pin)
	{
		this->pin->setValue(pin);
		if (nullptr != this->debugGeometryNode)
		{
			this->debugGeometryNode->setDirection(pin);
		}
	}

	Ogre::Vector3 JointSliderActuatorComponent::getPin(void) const
	{
		return this->pin->getVector3();
	}
	
	void JointSliderActuatorComponent::setTargetPosition(Ogre::Real targetPosition)
	{
		this->targetPosition->setValue(targetPosition);
		OgreNewt::SliderActuator* sliderActuatorJoint = dynamic_cast<OgreNewt::SliderActuator*>(this->joint);
		if (nullptr != sliderActuatorJoint)
		{
			sliderActuatorJoint->SetTargetPosition(targetPosition);
		}
	}
	
	Ogre::Real JointSliderActuatorComponent::getTargetPosition(void) const
	{
		return this->targetPosition->getReal();
	}
	
	void JointSliderActuatorComponent::setLinearRate(Ogre::Real linearRate)
	{
		this->linearRate->setValue(linearRate);
		OgreNewt::SliderActuator* sliderActuatorJoint = dynamic_cast<OgreNewt::SliderActuator*>(this->joint);
		if (nullptr != sliderActuatorJoint)
		{
			sliderActuatorJoint->SetLinearRate(linearRate);
		}
	}
	
	Ogre::Real JointSliderActuatorComponent::getLinearRate(void) const
	{
		return this->linearRate->getReal();
	}
	
	void JointSliderActuatorComponent::setMinStopDistance(Ogre::Real minStopDistance)
	{
		this->minStopDistance->setValue(minStopDistance);
		OgreNewt::SliderActuator* sliderActuatorJoint = dynamic_cast<OgreNewt::SliderActuator*>(this->joint);
		if (nullptr != sliderActuatorJoint)
		{
			sliderActuatorJoint->SetMinPositionLimit(minStopDistance);
		}
	}

	Ogre::Real JointSliderActuatorComponent::getMinStopDistance(void) const
	{
		return this->minStopDistance->getReal();
	}
	
	void JointSliderActuatorComponent::setMaxStopDistance(Ogre::Real maxStopDistance)
	{
		this->maxStopDistance->setValue(maxStopDistance);
		OgreNewt::SliderActuator* sliderActuatorJoint = dynamic_cast<OgreNewt::SliderActuator*>(this->joint);
		if (nullptr != sliderActuatorJoint)
		{
			sliderActuatorJoint->SetMaxPositionLimit(maxStopDistance);
		}
	}

	Ogre::Real JointSliderActuatorComponent::getMaxStopDistance(void) const
	{
		return this->maxStopDistance->getReal();
	}

	void JointSliderActuatorComponent::setMinForce(Ogre::Real minForce)
	{
		if (minForce > 0.0f)
			return;
		
		this->minForce->setValue(minForce);
		OgreNewt::SliderActuator* sliderActuatorJoint = dynamic_cast<OgreNewt::SliderActuator*>(this->joint);
		if (sliderActuatorJoint)
		{
			sliderActuatorJoint->SetMinForce(minForce);
		}
	}

	Ogre::Real JointSliderActuatorComponent::getMinForce(void) const
	{
		return this->minForce->getReal();
	}

	void JointSliderActuatorComponent::setMaxForce(Ogre::Real maxForce)
	{
		if (maxForce < 0.0f)
			return;
		
		this->maxForce->setValue(maxForce);
		OgreNewt::SliderActuator* sliderActuatorJoint = dynamic_cast<OgreNewt::SliderActuator*>(this->joint);
		if (sliderActuatorJoint)
		{
			sliderActuatorJoint->SetMaxForce(maxForce);
		}
	}

	Ogre::Real JointSliderActuatorComponent::getMaxForce(void) const
	{
		return this->maxForce->getReal();
	}
	
	void JointSliderActuatorComponent::setDirectionChange(bool directionChange)
	{
		this->directionChange->setValue(directionChange);
		this->internalDirectionChange = directionChange;
		this->targetPosition->setVisible(!directionChange);
	}
		
	bool JointSliderActuatorComponent::getDirectionChange(void) const
	{
		return this->directionChange->getBool();
	}
	
	void JointSliderActuatorComponent::setRepeat(bool repeat)
	{
		this->repeat->setValue(repeat);
		this->targetPosition->setVisible(!repeat);
	}
	
	bool JointSliderActuatorComponent::getRepeat(void) const
	{
		return this->repeat->getBool();
	}

	void JointSliderActuatorComponent::reactOnTargetPositionReached(luabind::object closureFunction)
	{
		this->targetPositionReachedClosureFunction = closureFunction;
	}

	/*******************************JointSlidingContactComponent*******************************/

	JointSlidingContactComponent::JointSlidingContactComponent()
		: JointComponent()
	{
		// Note that also in JointComponent internalBaseInit() is called, which sets the values for JointComponent, so this is called for already existing values
		// But luckely in Variant, no new attributes are added, but the values changed via interalAdd(...) in Variant
		this->type->setReadOnly(false);
		this->type->setValue(this->getClassName());
		this->type->setReadOnly(true);
		this->type->setDescription("A child body attached via a slider joint can only slide up and down (move along) the axis it is attached to and rotate around given pin-axis.");
	
		this->anchorPosition = new Variant(JointSlidingContactComponent::AttrAnchorPosition(), Ogre::Vector3::ZERO, this->attributes);
		this->pin = new Variant(JointSlidingContactComponent::AttrPin(), Ogre::Vector3::UNIT_Y, this->attributes);
		this->enableLinearLimits = new Variant(JointSlidingContactComponent::AttrLinearLimits(), false, this->attributes);
		this->minStopDistance = new Variant(JointSlidingContactComponent::AttrMinStopDistance(), 0.0f, this->attributes);
		this->maxStopDistance = new Variant(JointSlidingContactComponent::AttrMaxStopDistance(), 0.0f, this->attributes);

		this->enableAngleLimits = new Variant(JointSlidingContactComponent::AttrAngleLimits(), false, this->attributes);
		this->minAngleLimit = new Variant(JointSlidingContactComponent::AttrMinAngleLimit(), 0.0f, this->attributes);
		this->maxAngleLimit = new Variant(JointSlidingContactComponent::AttrMaxAngleLimit(), 0.0f, this->attributes);

		this->asSpringDamper = new Variant(JointSlidingContactComponent::AttrAsSpringDamper(), false, this->attributes);
		this->springK = new Variant(JointSlidingContactComponent::AttrSpringK(), 128.0f, this->attributes);
		this->springD = new Variant(JointSlidingContactComponent::AttrSpringD(), 4.0f, this->attributes);
		this->springDamperRelaxation = new Variant(JointSlidingContactComponent::AttrSpringDamperRelaxation(), 0.6f, this->attributes);

		this->targetId->setVisible(false);
		// this->minStopDistance->setVisible(false);
		// this->maxStopDistance->setVisible(false);
		// this->asSpringDamper->setVisible(false);
		// this->springK->setVisible(false);
		// this->springD->setVisible(false);
		// this->springDamperRelaxation->setVisible(false);
		this->springDamperRelaxation->setDescription("[0 - 0.99]");
	}

	JointSlidingContactComponent::~JointSlidingContactComponent()
	{

	}

	GameObjectCompPtr JointSlidingContactComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		// get also the cloned properties from joint handler base class
		JointSlidingContactCompPtr clonedJointCompPtr(boost::make_shared<JointSlidingContactComponent>());
		
		clonedJointCompPtr->setType(this->type->getString());
		clonedJointCompPtr->internalSetPriorId(this->id->getULong());
		clonedJointCompPtr->setPredecessorId(this->predecessorId->getULong());
		clonedJointCompPtr->setTargetId(this->targetId->getULong());
		clonedJointCompPtr->setJointPosition(this->jointPosition);
		clonedJointCompPtr->setJointRecursiveCollisionEnabled(this->jointRecursiveCollision->getBool());
		clonedJointCompPtr->stiffness->setValue(this->stiffness->getReal());

		clonedJointCompPtr->setAnchorPosition(this->anchorPosition->getVector3());
		clonedJointCompPtr->setPin(this->pin->getVector3());
		clonedJointCompPtr->setLinearLimitsEnabled(this->enableLinearLimits->getBool());
		clonedJointCompPtr->setMinMaxStopDistance(this->minStopDistance->getReal(), this->maxStopDistance->getReal());
		clonedJointCompPtr->setAngleLimitsEnabled(this->enableLinearLimits->getBool());
		clonedJointCompPtr->setMinMaxAngleLimit(Ogre::Degree(this->minAngleLimit->getReal()), Ogre::Degree(this->maxAngleLimit->getReal()));
		clonedJointCompPtr->setSpring(this->asSpringDamper->getBool(), this->springDamperRelaxation->getReal(), this->springK->getReal(), this->springD->getReal());

		clonedGameObjectPtr->addComponent(clonedJointCompPtr);
		clonedJointCompPtr->setOwner(clonedGameObjectPtr);

		clonedJointCompPtr->setActivated(this->activated->getBool());

		return clonedJointCompPtr;
	}

	bool JointSlidingContactComponent::init(xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		JointComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointAnchorPosition")
		{
			this->anchorPosition->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(0.0f, 0.0f, 0.0f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointPin")
		{
			this->pin->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(0.0f, 0.0f, 0.0f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointLinearLimits")
		{
			this->setLinearLimitsEnabled(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointMinStopDistance")
		{
			this->minStopDistance->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointMaxStopDistance")
		{
			this->maxStopDistance->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointAngleLimits")
		{
			this->setAngleLimitsEnabled(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointMinAngle")
		{
			this->minAngleLimit->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointMaxAngle")
		{
			this->maxAngleLimit->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointAsSpringDamper")
		{
			this->asSpringDamper->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointKSpring")
		{
			this->springK->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointDSpring")
		{
			this->springD->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointSpringDamperRelaxation")
		{
			this->springDamperRelaxation->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	bool JointSlidingContactComponent::postInit(void)
	{
		JointComponent::postInit();

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointSlidingContactComponent] Init joint sliding contact component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool JointSlidingContactComponent::connect(void)
	{
		bool success = JointComponent::connect();
		return success;
	}

	bool JointSlidingContactComponent::disconnect(void)
	{
		bool success = JointComponent::disconnect();
		return success;
	}

	Ogre::String JointSlidingContactComponent::getClassName(void) const
	{
		return "JointSlidingContactComponent";
	}

	Ogre::String JointSlidingContactComponent::getParentClassName(void) const
	{
		return "JointComponent";
	}

	void JointSlidingContactComponent::update(Ogre::Real dt, bool notSimulating)
	{

	}

	void JointSlidingContactComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		JointComponent::writeXML(propertiesXML, doc, filePath);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointAnchorPosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->anchorPosition->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointPin"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->pin->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointLinearLimits"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->enableLinearLimits->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointMinStopDistance"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->minStopDistance->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointMaxStopDistance"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxStopDistance->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointAngleLimits"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->enableLinearLimits->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointMinAngle"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->minAngleLimit->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointMaxAngle"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxAngleLimit->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointAsSpringDamper"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->asSpringDamper->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointKSpring"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->springK->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointDSpring"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->springD->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointSpringDamperRelaxation"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->springDamperRelaxation->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::Vector3 JointSlidingContactComponent::getUpdatedJointPosition(void)
	{
		if (nullptr == this->body)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointSlidingContactComponent] Could not get 'getUpdatedJointPosition', because there is no body for game object: " + this->gameObjectPtr->getName());
			return Ogre::Vector3::ZERO;
		}

		if (false == this->hasCustomJointPosition)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			return this->body->getPosition() + offset;
		}
		else
		{
			return this->jointPosition;
		}
	}

	bool JointSlidingContactComponent::createJoint(const Ogre::Vector3& customJointPosition)
	{
		// Joint base created but not activated, return false, but its no error, hence after that return true.
		if (false == JointComponent::createJoint(customJointPosition))
		{
			return true;
		}

		if (true == this->jointAlreadyCreated)
		{
			// joint already created so skip
			return true;
		}
		if (nullptr == this->body)
		{
			this->connect();
		}
		if (Ogre::Vector3::ZERO == this->pin->getVector3())
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[JointSlidingContactComponent] Cannot create joint for: " + this->gameObjectPtr->getName() + " because the pin is zero.");
			return false;
		}
		if (Ogre::Vector3::ZERO == customJointPosition)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			//Ogre::Vector3 aPos = Ogre::Vector3(-this->anchorPosition.x, -this->anchorPosition.y, -this->anchorPosition.z);
			//// relative anchor pos: 0.5 0 0 means 50% in X of the size of the parent object
			//Ogre::Vector3 offset = (aPos * size);
			//// take orientation into account, so that orientated joints are processed correctly
			//this->jointPosition = this->body->getPosition() - (this->body->getOrientation() * offset);

			// without rotation
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			this->jointPosition = (this->body->getPosition() + offset);
		}
		else
		{
			this->hasCustomJointPosition = true;
			this->jointPosition = customJointPosition;
		}
		OgreNewt::Body* predecessorBody = nullptr;

		if (this->predecessorJointCompPtr)
		{
			predecessorBody = this->predecessorJointCompPtr->getBody();
		}

		// Release joint each time, to create new one with new values
		this->internalReleaseJoint();
		this->joint = new OgreNewt::SlidingContact(this->body, predecessorBody, this->jointPosition, this->body->getOrientation() * this->pin->getVector3());
		OgreNewt::SlidingContact* slidingContact = dynamic_cast<OgreNewt::SlidingContact*>(this->joint);

		this->joint->setBodyMassScale(this->bodyMassScale->getVector2().x, this->bodyMassScale->getVector2().y);
		this->joint->setCollisionState(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		// this->body->setJointRecursiveCollision(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		// this->applyStiffness();

		slidingContact->setSpringAndDamping(this->asSpringDamper->getBool(), this->springDamperRelaxation->getReal(), this->springK->getReal(), this->springD->getReal());
		
		slidingContact->enableLinearLimits(this->enableLinearLimits->getBool());
		if (true == this->enableLinearLimits->getBool())
		{
			if (0.0f != this->minStopDistance->getReal() || 0.0f != this->maxStopDistance->getReal())
			{
				slidingContact->setLinearLimits(this->minStopDistance->getReal(), this->maxStopDistance->getReal());
			}
		}

		slidingContact->enableAngularLimits(this->enableAngleLimits->getBool());
		if (true == this->enableAngleLimits->getBool())
		{
			slidingContact->setAngularLimits(Ogre::Degree(this->minAngleLimit->getReal()), Ogre::Degree(this->maxAngleLimit->getReal()));
		}

		this->jointAlreadyCreated = true;

		return true;
	}

	void JointSlidingContactComponent::actualizeValue(Variant* attribute)
	{
		JointComponent::actualizeValue(attribute);

		if (JointSlidingContactComponent::AttrAnchorPosition() == attribute->getName())
		{
			this->setAnchorPosition(attribute->getVector3());
		}
		else if (JointSlidingContactComponent::AttrPin() == attribute->getName())
		{
			this->setPin(attribute->getVector3());
		}
		else if (JointSlidingContactComponent::AttrLinearLimits() == attribute->getName())
		{
			this->setLinearLimitsEnabled(attribute->getBool());
		}
		else if (JointSlidingContactComponent::AttrMinStopDistance() == attribute->getName())
		{
			this->setMinMaxStopDistance(attribute->getReal(), this->maxStopDistance->getReal());
		}
		else if (JointSlidingContactComponent::AttrMaxStopDistance() == attribute->getName())
		{
			this->setMinMaxStopDistance(this->minStopDistance->getReal(), attribute->getReal());
		}
		else if (JointSlidingContactComponent::AttrAngleLimits() == attribute->getName())
		{
			this->setAngleLimitsEnabled(attribute->getBool());
		}
		else if (JointSlidingContactComponent::AttrMinAngleLimit() == attribute->getName())
		{
			this->setMinMaxAngleLimit(Ogre::Degree(attribute->getReal()), Ogre::Degree(this->maxAngleLimit->getReal()));
		}
		else if (JointSlidingContactComponent::AttrMaxAngleLimit() == attribute->getName())
		{
			this->setMinMaxAngleLimit(Ogre::Degree(this->minAngleLimit->getReal()), Ogre::Degree(attribute->getReal()));
		}
		else if (JointSlidingContactComponent::AttrAsSpringDamper() == attribute->getName())
		{
			this->setSpring(attribute->getBool(), this->springDamperRelaxation->getReal(), this->springK->getReal(), this->springD->getReal());
		}
		else if (JointSlidingContactComponent::AttrSpringDamperRelaxation() == attribute->getName())
		{
			this->setSpring(this->asSpringDamper->getBool(), attribute->getReal(), this->springK->getReal(), this->springD->getReal());
		}
		else if (JointSlidingContactComponent::AttrSpringK() == attribute->getName())
		{
			this->setSpring(this->asSpringDamper->getBool(), this->springDamperRelaxation->getReal(), attribute->getReal(), this->springD->getReal());
		}
		else if (JointSlidingContactComponent::AttrSpringD() == attribute->getName())
		{
			this->setSpring(this->asSpringDamper->getBool(), this->springDamperRelaxation->getReal(), this->springK->getReal(), attribute->getReal());
		}
	}

	void JointSlidingContactComponent::setAnchorPosition(const Ogre::Vector3& anchorPosition)
	{
		this->anchorPosition->setValue(anchorPosition);
		this->jointAlreadyCreated = false;
	}

	Ogre::Vector3 JointSlidingContactComponent::getAnchorPosition(void) const
	{
		return this->anchorPosition->getVector3();
	}

	void JointSlidingContactComponent::setPin(const Ogre::Vector3& pin)
	{
		this->pin->setValue(pin);
	}

	Ogre::Vector3 JointSlidingContactComponent::getPin(void) const
	{
		return this->pin->getVector3();
	}

	void JointSlidingContactComponent::setLinearLimitsEnabled(bool enableLinearLimits)
	{
		this->enableLinearLimits->setValue(enableLinearLimits);
		// this->minStopDistance->setVisible(true == enableLimits);
		// this->maxStopDistance->setVisible(true == enableLimits);
	}

	bool JointSlidingContactComponent::getLinearLimitsEnabled(void) const
	{
		return this->enableLinearLimits->getBool();
	}

	void JointSlidingContactComponent::setAngleLimitsEnabled(bool enableAngleLimits)
	{
		this->enableAngleLimits->setValue(enableAngleLimits);
		// this->minAngleLimit->setVisible(true == enableAngleLimits);
		// this->maxAngleLimit->setVisible(true == enableAngleLimits);
	}

	bool JointSlidingContactComponent::getAngleLimitsEnabled(void) const
	{
		return this->enableAngleLimits->getBool();
	}

	void JointSlidingContactComponent::setMinMaxStopDistance(Ogre::Real minStopDistance, Ogre::Real maxStopDistance)
	{
		this->minStopDistance->setValue(minStopDistance);
		this->maxStopDistance->setValue(maxStopDistance);

		OgreNewt::SlidingContact* slidingContactJoint = dynamic_cast<OgreNewt::SlidingContact*>(this->joint);

		if (nullptr != slidingContactJoint && true == this->enableLinearLimits->getBool())
		{
			if (this->minStopDistance->getReal() == this->maxStopDistance->getReal())
			{
				// throw Ogre::Exception(Ogre::Exception::ERR_INVALIDPARAMS, "[JointPassiveSliderComponent] The passive slider joint cannot have the same min angle limit as max angle limit!\n", "NOWA");
			}
			slidingContactJoint->setLinearLimits(this->minStopDistance->getReal(), this->maxStopDistance->getReal());
		}
	}
	
	Ogre::Real JointSlidingContactComponent::getMinStopDistance(void) const
	{
		return this->minStopDistance->getReal();
	}
		
	Ogre::Real JointSlidingContactComponent::getMaxStopDistance(void) const
	{
		return this->maxStopDistance->getReal();
	}
	
	void JointSlidingContactComponent::setMinMaxAngleLimit(const Ogre::Degree& minAngleLimit, const Ogre::Degree& maxAngleLimit)
	{
		this->minAngleLimit->setValue(minAngleLimit.valueDegrees());
		this->maxAngleLimit->setValue(maxAngleLimit.valueDegrees());

		OgreNewt::SlidingContact* slidingContactJoint = dynamic_cast<OgreNewt::SlidingContact*>(this->joint);
		if (slidingContactJoint && true == this->enableAngleLimits->getBool())
		{
			slidingContactJoint->setAngularLimits(Ogre::Degree(this->minAngleLimit->getReal()), Ogre::Degree(this->maxAngleLimit->getReal()));
		}
	}
	
	Ogre::Real JointSlidingContactComponent::getMinAngleLimit(void) const
	{
		return this->minAngleLimit->getReal();
	}
	
	Ogre::Real JointSlidingContactComponent::getMaxAngleLimit(void) const
	{
		return this->maxAngleLimit->getReal();
	}

	void JointSlidingContactComponent::setSpring(bool asSpringDamper)
	{
		this->asSpringDamper->setValue(asSpringDamper);
		OgreNewt::SlidingContact* slidingContactJoint = dynamic_cast<OgreNewt::SlidingContact*>(this->joint);
		if (nullptr != slidingContactJoint)
		{
			slidingContactJoint->setSpringAndDamping(this->asSpringDamper->getBool(), this->springDamperRelaxation->getReal(), this->springK->getReal(), this->springD->getReal());
		}
	}

	void JointSlidingContactComponent::setSpring(bool asSpringDamper, Ogre::Real springDamperRelaxation, Ogre::Real springK, Ogre::Real springD)
	{
		// this->springK->setVisible(true == asSpringDamper);
		// this->springD->setVisible(true == asSpringDamper);
		// this->springDamperRelaxation->setVisible(true == asSpringDamper);
		this->asSpringDamper->setValue(asSpringDamper);
		this->springDamperRelaxation->setValue(springDamperRelaxation);
		this->springK->setValue(springK);
		this->springD->setValue(springD);
		OgreNewt::SlidingContact* slidingContactJoint = dynamic_cast<OgreNewt::SlidingContact*>(this->joint);
		if (nullptr != slidingContactJoint)
		{
			slidingContactJoint->setSpringAndDamping(this->asSpringDamper->getBool(), this->springDamperRelaxation->getReal(), this->springK->getReal(), this->springD->getReal());
		}
	}

	std::tuple<bool, Ogre::Real, Ogre::Real, Ogre::Real> JointSlidingContactComponent::getSpring(void) const
	{
		return std::make_tuple(this->asSpringDamper->getBool(), this->springDamperRelaxation->getReal(), this->springK->getReal(), this->springD->getReal());
	}

	/*******************************JointActiveSliderComponent*******************************/

	JointActiveSliderComponent::JointActiveSliderComponent()
		: JointComponent()
	{
		// Note that also in JointComponent internalBaseInit() is called, which sets the values for JointComponent, so this is called for already existing values
		// But luckely in Variant, no new attributes are added, but the values changed via interalAdd(...) in Variant
		this->type->setReadOnly(false);
		this->type->setValue(this->getClassName());
		this->type->setReadOnly(true);
		this->type->setDescription("A child body attached via a slider joint can only slide up and down (move along) the axis it is attached to. It moves automatically when activated.");

		this->anchorPosition = new Variant(JointActiveSliderComponent::AttrAnchorPosition(), Ogre::Vector3::ZERO, this->attributes);
		this->pin = new Variant(JointActiveSliderComponent::AttrPin(), Ogre::Vector3::UNIT_X, this->attributes);
		this->moveSpeed = new Variant(JointActiveSliderComponent::AttrMoveSpeed(), 1.0f, this->attributes);
		this->enableLimits = new Variant(JointActiveSliderComponent::AttrLimits(), true, this->attributes);
		this->minStopDistance = new Variant(JointActiveSliderComponent::AttrMinStopDistance(), -1.0f, this->attributes);
		this->maxStopDistance = new Variant(JointActiveSliderComponent::AttrMaxStopDistance(), 1.0f, this->attributes);
		this->repeat = new Variant(JointActiveSliderComponent::AttrRepeat(), true, this->attributes);
		this->directionChange = new Variant(JointActiveSliderComponent::AttrDirectionChange(), true, this->attributes);

		this->targetId->setVisible(false);
		
	}

	JointActiveSliderComponent::~JointActiveSliderComponent()
	{

	}

	GameObjectCompPtr JointActiveSliderComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		// get also the cloned properties from joint handler base class
		JointActiveSliderCompPtr clonedJointCompPtr(boost::make_shared<JointActiveSliderComponent>());
		
		clonedJointCompPtr->setType(this->type->getString());
		clonedJointCompPtr->internalSetPriorId(this->id->getULong());
		clonedJointCompPtr->setPredecessorId(this->predecessorId->getULong());
		clonedJointCompPtr->setTargetId(this->targetId->getULong());
		clonedJointCompPtr->setJointPosition(this->jointPosition);
		clonedJointCompPtr->setJointRecursiveCollisionEnabled(this->jointRecursiveCollision->getBool());
		clonedJointCompPtr->stiffness->setValue(this->stiffness->getReal());

		clonedJointCompPtr->setAnchorPosition(this->anchorPosition->getVector3());
		clonedJointCompPtr->setPin(this->pin->getVector3());
		clonedJointCompPtr->setLimitsEnabled(this->enableLimits->getBool());
		clonedJointCompPtr->setMinMaxStopDistance(this->minStopDistance->getReal(), this->maxStopDistance->getReal());
		clonedJointCompPtr->setRepeat(this->repeat->getBool());
		clonedJointCompPtr->setDirectionChange(this->directionChange->getBool());
		
		clonedGameObjectPtr->addComponent(clonedJointCompPtr);
		clonedJointCompPtr->setOwner(clonedGameObjectPtr);

		clonedJointCompPtr->setActivated(this->activated->getBool());

		return clonedJointCompPtr;
	}

	bool JointActiveSliderComponent::init(xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		JointComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointAnchorPosition")
		{
			this->anchorPosition->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(0.0f, 0.0f, 0.0f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointPin")
		{
			this->pin->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(1.0f, 0.0f, 0.0f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointMoveSpeed")
		{
			this->moveSpeed->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointLimits")
		{
			this->setLimitsEnabled(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointMinStopDistance")
		{
			this->minStopDistance->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointMaxStopDistance")
		{
			this->maxStopDistance->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointRepeat")
		{
			this->repeat->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointDirectionChange")
		{
			this->directionChange->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	bool JointActiveSliderComponent::postInit(void)
	{
		JointComponent::postInit();

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointActiveSliderComponent] Init joint active slider component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool JointActiveSliderComponent::connect(void)
	{
		bool success = JointComponent::connect();
		return success;
	}

	bool JointActiveSliderComponent::disconnect(void)
	{
		bool success = JointComponent::disconnect();
		return success;
	}

	Ogre::String JointActiveSliderComponent::getClassName(void) const
	{
		return "JointActiveSliderComponent";
	}

	Ogre::String JointActiveSliderComponent::getParentClassName(void) const
	{
		return "JointComponent";
	}

	void JointActiveSliderComponent::update(Ogre::Real dt, bool notSimulating)
	{

	}

	void JointActiveSliderComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		JointComponent::writeXML(propertiesXML, doc, filePath);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointAnchorPosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->anchorPosition->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointPin"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->pin->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointMoveSpeed"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->moveSpeed->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointLimits"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->enableLimits->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointMinStopDistance"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->minStopDistance->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointMaxStopDistance"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxStopDistance->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointRepeat"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->repeat->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointDirectionChange"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->directionChange->getBool())));
		propertiesXML->append_node(propertyXML);
	}

	bool JointActiveSliderComponent::createJoint(const Ogre::Vector3& customJointPosition)
	{
		// Joint base created but not activated, return false, but its no error, hence after that return true.
		if (false == JointComponent::createJoint(customJointPosition))
		{
			return true;
		}

		if (true == this->jointAlreadyCreated)
		{
			// joint already created so skip
			return true;
		}
		if (nullptr == this->body)
		{
			this->connect();
		}
		if (Ogre::Vector3::ZERO == this->pin->getVector3())
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[JointActiveSliderComponent] Cannot create joint for: " + this->gameObjectPtr->getName() + " because the pin is zero.");
			return false;
		}
		
		if (Ogre::Vector3::ZERO == customJointPosition)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			//Ogre::Vector3 aPos = Ogre::Vector3(-this->anchorPosition.x, -this->anchorPosition.y, -this->anchorPosition.z);
			//// relative anchor pos: 0.5 0 0 means 50% in X of the size of the parent object
			//Ogre::Vector3 offset = (aPos * size);
			//// take orientation into account, so that orientated joints are processed correctly
			//this->jointPosition = this->body->getPosition() - (this->body->getOrientation() * offset);

			// without rotation
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			this->jointPosition = (this->body->getPosition() + offset);
		}
		else
		{
			this->hasCustomJointPosition = true;
			this->jointPosition = customJointPosition;
		}
		OgreNewt::Body* predecessorBody = nullptr;

		if (this->predecessorJointCompPtr)
		{
			predecessorBody = this->predecessorJointCompPtr->getBody();
		}

		// Release joint each time, to create new one with new values
		this->internalReleaseJoint();
		this->joint = new OgreNewt::ActiveSliderJoint(this->body, predecessorBody, this->jointPosition, this->pin->getVector3(), this->minStopDistance->getReal(),
			this->maxStopDistance->getReal(), this->moveSpeed->getReal(), this->repeat->getBool(), this->directionChange->getBool(), this->activated->getBool());

		this->joint->setBodyMassScale(this->bodyMassScale->getVector2().x, this->bodyMassScale->getVector2().y);
		this->joint->setCollisionState(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		// this->body->setJointRecursiveCollision(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		// this->applyStiffness();

		OgreNewt::ActiveSliderJoint* activeSlider = dynamic_cast<OgreNewt::ActiveSliderJoint*>(this->joint);
		activeSlider->enableLimits(this->enableLimits->getBool());
		if (true == this->enableLimits->getBool())
		{
			/*if (this->minStopDistance == this->maxStopDistance)
			{
				throw Ogre::Exception(Ogre::Exception::ERR_INVALIDPARAMS, "[PhysicsComponent] The active slider joint cannot have the same min angle limit as max angle limit!\n", "NOWA");
			}*/

			activeSlider->setLimits(this->minStopDistance->getReal(), this->maxStopDistance->getReal());
		}

		this->jointAlreadyCreated = true;
		
		return true;
	}

	void JointActiveSliderComponent::actualizeValue(Variant* attribute)
	{
		JointComponent::actualizeValue(attribute);

		if (JointActiveSliderComponent::AttrAnchorPosition() == attribute->getName())
		{
			this->setAnchorPosition(attribute->getVector3());
		}
		else if (JointActiveSliderComponent::AttrPin() == attribute->getName())
		{
			this->setPin(attribute->getVector3());
		}
		else if (JointActiveSliderComponent::AttrLimits() == attribute->getName())
		{
			this->setLimitsEnabled(attribute->getBool());
		}
		else if (JointActiveSliderComponent::AttrMinStopDistance() == attribute->getName())
		{
			this->setMinMaxStopDistance(attribute->getReal(), this->maxStopDistance->getReal());
		}
		else if (JointActiveSliderComponent::AttrMaxStopDistance() == attribute->getName())
		{
			this->setMinMaxStopDistance(this->minStopDistance->getReal(), attribute->getReal());
		}
		else if (JointActiveSliderComponent::AttrMoveSpeed() == attribute->getName())
		{
			this->setMoveSpeed(attribute->getReal());
		}
		else if (JointActiveSliderComponent::AttrRepeat() == attribute->getName())
		{
			this->setRepeat(attribute->getBool());
		}
		else if (JointActiveSliderComponent::AttrDirectionChange() == attribute->getName())
		{
			this->setDirectionChange(attribute->getBool());
		}
	}

	void JointActiveSliderComponent::setActivated(bool activated)
	{
		JointComponent::setActivated(activated);

		OgreNewt::ActiveSliderJoint* activeSlider = dynamic_cast<OgreNewt::ActiveSliderJoint*>(this->joint);
		if (nullptr != activeSlider)
		{
			activeSlider->setActivated(activated);
		}
	}

	Ogre::Vector3 JointActiveSliderComponent::getUpdatedJointPosition(void)
	{
		if (nullptr == this->body)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointActiveSliderComponent] Could not get 'getUpdatedJointPosition', because there is no body for game object: " + this->gameObjectPtr->getName());
			return Ogre::Vector3::ZERO;
		}

		if (false == this->hasCustomJointPosition)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			return this->body->getPosition() + offset;
		}
		else
		{
			return this->jointPosition;
		}
	}
	
	void JointActiveSliderComponent::setAnchorPosition(const Ogre::Vector3& anchorPosition)
	{
		this->anchorPosition->setValue(anchorPosition);
		this->jointAlreadyCreated = false;
	}

	Ogre::Vector3 JointActiveSliderComponent::getAnchorPosition(void) const
	{
		return this->anchorPosition->getVector3();
	}

	void JointActiveSliderComponent::setPin(const Ogre::Vector3& pin)
	{
		this->pin->setValue(pin);
	}

	Ogre::Vector3 JointActiveSliderComponent::getPin(void) const
	{
		return this->pin->getVector3();
	}

	void JointActiveSliderComponent::setLimitsEnabled(bool enableLimits)
	{
		this->enableLimits->setValue(enableLimits);
		// this->minStopDistance->setVisible(true == enableLimits);
		// this->maxStopDistance->setVisible(true == enableLimits);
	}

	bool JointActiveSliderComponent::getLimitsEnabled(void) const
	{
		return this->enableLimits->getBool();
	}

	void JointActiveSliderComponent::setMinMaxStopDistance(Ogre::Real minStopDistance, Ogre::Real maxStopDistance)
	{
		this->minStopDistance->setValue(minStopDistance);
		this->maxStopDistance->setValue(maxStopDistance);

		OgreNewt::ActiveSliderJoint* activeSlider = dynamic_cast<OgreNewt::ActiveSliderJoint*>(this->joint);

		if (nullptr != activeSlider && true == this->enableLimits->getBool())
		{
			if (this->minStopDistance->getReal() == this->maxStopDistance->getReal())
			{
				// throw Ogre::Exception(Ogre::Exception::ERR_INVALIDPARAMS, "[JointPassiveSliderComponent] The passive slider joint cannot have the same min angle limit as max angle limit!\n", "NOWA");
			}
			activeSlider->setLimits(minStopDistance, maxStopDistance);
		}
	}
	
	Ogre::Real JointActiveSliderComponent::getMinStopDistance(void) const
	{
		return this->minStopDistance->getReal();
	}
	
	Ogre::Real JointActiveSliderComponent::getMaxStopDistance(void) const
	{
		return this->maxStopDistance->getReal();
	}
	
	int JointActiveSliderComponent::getCurrentDirection(void) const
	{
		OgreNewt::ActiveSliderJoint* activeSlider = dynamic_cast<OgreNewt::ActiveSliderJoint*>(this->joint);
		if (nullptr != activeSlider)
		{
			return activeSlider->getCurrentDirection();
		}
		return 0;
	}

	void JointActiveSliderComponent::setMoveSpeed(Ogre::Real moveSpeed)
	{
		this->moveSpeed->setValue(moveSpeed);
		OgreNewt::ActiveSliderJoint* activeSlider = dynamic_cast<OgreNewt::ActiveSliderJoint*>(this->joint);
		if (nullptr != activeSlider)
		{
			activeSlider->setMoveSpeed(moveSpeed);
		}
	}

	Ogre::Real JointActiveSliderComponent::getMoveSpeed(void) const
	{
		return this->moveSpeed->getReal();
	}

	void JointActiveSliderComponent::setRepeat(bool repeat)
	{
		this->repeat->setValue(repeat);
		OgreNewt::ActiveSliderJoint* activeSlider = dynamic_cast<OgreNewt::ActiveSliderJoint*>(this->joint);
		if (nullptr != activeSlider)
		{
			activeSlider->setRepeat(repeat);
		}
	}

	bool JointActiveSliderComponent::getRepeat(void) const
	{
		return this->repeat->getBool();
	}

	void JointActiveSliderComponent::setDirectionChange(bool directionChange)
	{
		this->directionChange->setValue(directionChange);
		OgreNewt::ActiveSliderJoint* activeSlider = dynamic_cast<OgreNewt::ActiveSliderJoint*>(this->joint);
		if (nullptr != activeSlider)
		{
			activeSlider->setDirectionChange(directionChange);
		}
	}

	bool JointActiveSliderComponent::getDirectionChange(void) const
	{
		return this->directionChange->getBool();
	}

	/*******************************JointMathSliderComponent*******************************/

	JointMathSliderComponent::JointMathSliderComponent()
		: JointComponent(),
		oppositeDir(1.0f),
		progress(0.0f),
		round(0)
	{
		// Note that also in JointComponent internalBaseInit() is called, which sets the values for JointComponent, so this is called for already existing values
		// But luckely in Variant, no new attributes are added, but the values changed via interalAdd(...) in Variant
		this->type->setReadOnly(false);
		this->type->setValue(this->getClassName());
		this->type->setReadOnly(true);
		this->type->setDescription("A child body attached via a slider joint can only slide up and down (move along) the axis it is attached to. It moves automatically when activated.");

		this->activated = new Variant(JointMathSliderComponent::AttrActivated(), true, this->attributes);
		this->mx = new Variant(JointMathSliderComponent::AttrFunctionX(), "cos(t)", this->attributes);
		this->my = new Variant(JointMathSliderComponent::AttrFunctionY(), "sin(t)", this->attributes);
		this->mz = new Variant(JointMathSliderComponent::AttrFunctionZ(), "0", this->attributes);
		this->moveSpeed = new Variant(JointMathSliderComponent::AttrMoveSpeed(), 1.0f, this->attributes);
		this->enableLimits = new Variant(JointMathSliderComponent::AttrLimits(), true, this->attributes);
		this->minStopDistance = new Variant(JointMathSliderComponent::AttrMinStopDistance(), -1.0f, this->attributes);
		this->maxStopDistance = new Variant(JointMathSliderComponent::AttrMaxStopDistance(), 1.0f, this->attributes);
		this->repeat = new Variant(JointMathSliderComponent::AttrRepeat(), true, this->attributes);
		this->directionChange = new Variant(JointMathSliderComponent::AttrDirectionChange(), true, this->attributes);

// Hier ueber mygui #{MathSlider} damit in Sprache uebersetzt
		this->activated->setDescription("When activated this slider moves in a realtime changing direction determined by mathematical functions for x, y, z.");
		this->mx->setDescription("The mathematical function for the x-vector. It can be any function. If e.g. a circular movement is whished the function could be cos(x).");
		this->my->setDescription("The mathematical function for the y-vector. It can be any function. If e.g. a circular movement is whished the function could be sin(x).");
		this->mz->setDescription("The mathematical function for the z-vector. It can be any function. If e.g. a circular movement is whished the function could be 0.");
		this->targetId->setVisible(false);

		// add the pi constant
		for (int i = 0; i < 3; ++i)
		{
			this->functionParser[i].AddConstant("PI", 3.1415926535897932);
		}
	}

	JointMathSliderComponent::~JointMathSliderComponent()
	{

	}

	GameObjectCompPtr JointMathSliderComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		// get also the cloned properties from joint handler base class
		JointMathSliderCompPtr clonedJointCompPtr(boost::make_shared<JointMathSliderComponent>());
		
		clonedJointCompPtr->setType(this->type->getString());
		clonedJointCompPtr->internalSetPriorId(this->id->getULong());
		clonedJointCompPtr->setPredecessorId(this->predecessorId->getULong());
		clonedJointCompPtr->setTargetId(this->targetId->getULong());
		clonedJointCompPtr->setJointPosition(this->jointPosition);
		clonedJointCompPtr->setJointRecursiveCollisionEnabled(this->jointRecursiveCollision->getBool());
		clonedJointCompPtr->stiffness->setValue(this->stiffness->getReal());

		clonedJointCompPtr->setFunctionX(this->mx->getString());
		clonedJointCompPtr->setFunctionY(this->my->getString());
		clonedJointCompPtr->setFunctionZ(this->mz->getString());
		clonedJointCompPtr->setLimitsEnabled(this->enableLimits->getBool());
		clonedJointCompPtr->setMinMaxStopDistance(this->minStopDistance->getReal(), this->maxStopDistance->getReal());
		clonedJointCompPtr->setRepeat(this->repeat->getBool());
		clonedJointCompPtr->setDirectionChange(this->directionChange->getBool());

		clonedGameObjectPtr->addComponent(clonedJointCompPtr);
		clonedJointCompPtr->setOwner(clonedGameObjectPtr);

		clonedJointCompPtr->setActivated(this->activated->getBool());

		return clonedJointCompPtr;
	}

	bool JointMathSliderComponent::init(xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		JointComponent::init(propertyElement, filename);
		
		// no double properties here, so no matching, because it does not make sense to use two math slider joints at once
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointFunctionX")
		{
			this->mx->setValue(XMLConverter::getAttrib(propertyElement, "data", ""));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointFunctionY")
		{
			this->my->setValue(XMLConverter::getAttrib(propertyElement, "data", ""));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointFunctionZ")
		{
			this->mz->setValue(XMLConverter::getAttrib(propertyElement, "data", ""));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointMoveSpeed")
		{
			this->moveSpeed->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointLimits")
		{
			this->setLimitsEnabled(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointMinStopDistance")
		{
			this->minStopDistance->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointMaxStopDistance")
		{
			this->maxStopDistance->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointRepeat")
		{
			this->repeat->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointDirectionChange")
		{
			this->directionChange->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	bool JointMathSliderComponent::postInit(void)
	{
		JointComponent::postInit();

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointMathSliderComponent] Init joint math slider component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool JointMathSliderComponent::connect(void)
	{
		bool success = JointComponent::connect();

		this->parseMathematicalFunction();

		return success;
	}

	bool JointMathSliderComponent::disconnect(void)
	{
		bool success = JointComponent::disconnect();
		return success;
	}

	Ogre::String JointMathSliderComponent::getClassName(void) const
	{
		return "JointMathSliderComponent";
	}

	Ogre::String JointMathSliderComponent::getParentClassName(void) const
	{
		return "JointComponent";
	}

	void JointMathSliderComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			if (false == this->isActivated() && nullptr == this->body)
			{
				return;
			}

			if (Ogre::Math::RealEqual(this->oppositeDir, 1.0f))
			{
				this->progress += this->moveSpeed->getReal() * dt;
				if (Ogre::Math::RealEqual(this->progress, this->maxStopDistance->getReal()), dt)
				{
					// Ogre::LogManager::getSingletonPtr()->logMessage("dir change from +  to -");
					if (true == this->directionChange->getBool())
					{
						this->oppositeDir *= -1.0f;
						this->round++;
					}
				}
			}
			else
			{
				this->progress -= this->moveSpeed->getReal() * dt;
				// if (this->progress <= currentJointProperties.minStopDistance)
				if (Ogre::Math::RealEqual(this->progress, this->minStopDistance->getReal()), dt)
				{
					// Ogre::LogManager::getSingletonPtr()->logMessage("dir change from -  to +");
					if (true == this->directionChange->getBool())
					{
						this->oppositeDir *= -1.0f;
						this->round++;
					}
				}
			}
			// if the slider took 2 rounds (1x forward and 1x back, then its enough, if repeat is off)
			// also take the progress into account, the slider started at zero and should stop at zero
			// maybe here attribute in xml, to set how many rounds the slider should proceed
			if (this->round == 2 && this->progress >= 0.0f)
			{
				// if repeat is of, only change the direction one time, to get back to its origin and leave
				if (false == this->repeat->getBool())
				{
					// is this correct to manipulate the properties?
					this->directionChange->setValue(false);
				}
			}
			// when repeat is on, proceed
			// when direction change is on, proceed
			if (true == this->repeat->getBool() || true == this->directionChange->getBool())
			{
				// Ogre::Real moveValue = this->jointProperties.moveSpeed * oppositeDir;
				// set the x, y, z variable, which is the movement speed with positive or negative direction
				double varX[] = { this->progress };
				double varY[] = { this->progress };
				double varZ[] = { this->progress };
				// evauluate the function for x, y, z, to get the results
				Ogre::Vector3 mathFunction;
				mathFunction.x = static_cast<Ogre::Real>(this->functionParser[0].Eval(varX));
				mathFunction.y = static_cast<Ogre::Real>(this->functionParser[1].Eval(varY));
				mathFunction.z = static_cast<Ogre::Real>(this->functionParser[2].Eval(varZ));

				// Ogre::LogManager::getSingletonPtr()->logMessage("f(x): " + Ogre::StringConverter::toString(this->jointProperties.pin));

				this->body->setVelocity(mathFunction);
				this->body->setPositionOrientation(this->body->getPosition(), Ogre::Quaternion::IDENTITY);
				// this->physicsBody->setPositionOrientation(this->startPosition + this->jointProperties.pin, Ogre::Quaternion::IDENTITY);

				// Ogre::LogManager::getSingletonPtr()->logMessage("vel: " + Ogre::StringConverter::toString(this->getVelocity()));
				// Ogre::LogManager::getSingletonPtr()->logMessage("ve0: " + Ogre::StringConverter::toString(this->getPosition() - this->lastPosition));
			}
		}
	}

	void JointMathSliderComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		JointComponent::writeXML(propertiesXML, doc, filePath);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointFunctionX"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->mx->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointFunctionY"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->my->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointFunctionZ"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->mz->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointMoveSpeed"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->moveSpeed->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointLimits"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->enableLimits->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointMinStopDistance"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->minStopDistance->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointMaxStopDistance"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxStopDistance->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointRepeat"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->repeat->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointDirectionChange"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->directionChange->getBool())));
		propertiesXML->append_node(propertyXML);
	}

	void JointMathSliderComponent::parseMathematicalFunction(void)
	{
		// parse the function for x, y, z
		int resX = this->functionParser[0].Parse(this->getFunctionX(), "t");
		if (resX > 0)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointMathSliderComponent] Mathematical function parse error for mx: "
				+ Ogre::String(this->functionParser[0].ErrorMsg()));
		}

		int resY = this->functionParser[1].Parse(this->getFunctionY(), "t");
		if (resY > 0)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointMathSliderComponent] Mathematical function parse error for my: "
				+ Ogre::String(this->functionParser[1].ErrorMsg()));
		}

		int resZ = this->functionParser[2].Parse(this->getFunctionZ(), "t");
		if (resZ > 0)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointMathSliderComponent] Mathematical function parse error for mz: "
				+ Ogre::String(this->functionParser[2].ErrorMsg()));
		}
	}

	bool JointMathSliderComponent::createJoint(const Ogre::Vector3& customJointPosition)
	{
		// Joint base created but not activated, return false, but its no error, hence after that return true.
		if (false == JointComponent::createJoint(customJointPosition))
		{
			return true;
		}

		this->round = 0;
		this->oppositeDir = 1.0f;

		return true;
	}

	void JointMathSliderComponent::actualizeValue(Variant* attribute)
	{
		JointComponent::actualizeValue(attribute);

		if (JointMathSliderComponent::AttrFunctionX() == attribute->getName())
		{
			this->setFunctionX(attribute->getString());
		}
		else if (JointMathSliderComponent::AttrFunctionY() == attribute->getName())
		{
			this->setFunctionY(attribute->getString());
		}
		else if (JointMathSliderComponent::AttrFunctionZ() == attribute->getName())
		{
			this->setFunctionZ(attribute->getString());
		}
		else if (JointActiveSliderComponent::AttrLimits() == attribute->getName())
		{
			this->setLimitsEnabled(attribute->getBool());
		}
		else if (JointActiveSliderComponent::AttrMinStopDistance() == attribute->getName())
		{
			this->setMinMaxStopDistance(attribute->getReal(), this->maxStopDistance->getReal());
		}
		else if (JointActiveSliderComponent::AttrMaxStopDistance() == attribute->getName())
		{
			this->setMinMaxStopDistance(this->minStopDistance->getReal(), attribute->getReal());
		}
		else if (JointActiveSliderComponent::AttrMoveSpeed() == attribute->getName())
		{
			this->setMoveSpeed(attribute->getReal());
		}
		else if (JointActiveSliderComponent::AttrRepeat() == attribute->getName())
		{
			this->setRepeat(attribute->getBool());
		}
		else if (JointActiveSliderComponent::AttrDirectionChange() == attribute->getName())
		{
			this->setDirectionChange(attribute->getBool());
		}
	}

	void JointMathSliderComponent::setFunctionX(const Ogre::String& mx)
	{
		this->mx->setValue(mx);
		int resX = this->functionParser[0].Parse(mx, "t");
		if (resX > 0)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointMathSliderComponent] Mathematical function parse error for mx: "
				+ Ogre::String(this->functionParser[0].ErrorMsg()));
		}
	}

	Ogre::String JointMathSliderComponent::getFunctionX(void) const
	{
		return this->mx->getString();
	}

	void JointMathSliderComponent::setFunctionY(const Ogre::String& my)
	{
		this->my->setValue(my);
		// Attention: "t" is correct, its the math t for the function
		int resY = this->functionParser[1].Parse(my, "t");
		if (resY > 0)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointMathSliderComponent] Mathematical function parse error for mx: "
				+ Ogre::String(this->functionParser[1].ErrorMsg()));
		}
	}

	Ogre::String JointMathSliderComponent::getFunctionY(void) const
	{
		return this->my->getString();
	}

	void JointMathSliderComponent::setFunctionZ(const Ogre::String& mz)
	{
		this->mz->setValue(mz);
		// Attention: "t" is correct, its the math t for the function
		int resZ = this->functionParser[2].Parse(mz, "t");
		if (resZ > 0)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointMathSliderComponent] Mathematical function parse error for mz: "
				+ Ogre::String(this->functionParser[2].ErrorMsg()));
		}
	}

	Ogre::String JointMathSliderComponent::getFunctionZ(void) const
	{
		return this->mx->getString();
	}

	void JointMathSliderComponent::setLimitsEnabled(bool enableLimits)
	{
		this->enableLimits->setValue(enableLimits);
		// this->minStopDistance->setVisible(true == enableLimits);
		// this->maxStopDistance->setVisible(true == enableLimits);
	}

	bool JointMathSliderComponent::getLimitsEnabled(void) const
	{
		return this->enableLimits->getBool();
	}

	void JointMathSliderComponent::setMinMaxStopDistance(Ogre::Real minStopDistance, Ogre::Real maxStopDistance)
	{
		this->minStopDistance->setValue(minStopDistance);
		this->maxStopDistance->setValue(maxStopDistance);

		if (this->minStopDistance->getReal() == this->maxStopDistance->getReal())
		{
			// throw Ogre::Exception(Ogre::Exception::ERR_INVALIDPARAMS, "[JointPassiveSliderComponent] The passive slider joint cannot have the same min angle limit as max angle limit!\n", "NOWA");
		}
	}
	
	Ogre::Real JointMathSliderComponent::getMinStopDistance(void) const
	{
		return this->minStopDistance->getReal();
	}
	
	Ogre::Real JointMathSliderComponent::getMaxStopDistance(void) const
	{
		return this->maxStopDistance->getReal();
	}

	void JointMathSliderComponent::setMoveSpeed(Ogre::Real moveSpeed)
	{
		this->moveSpeed->setValue(moveSpeed);
	}

	Ogre::Real JointMathSliderComponent::getMoveSpeed(void) const
	{
		return this->moveSpeed->getReal();
	}

	void JointMathSliderComponent::setRepeat(bool repeat)
	{
		this->repeat->setValue(repeat);
	}

	bool JointMathSliderComponent::getRepeat(void) const
	{
		return this->repeat->getBool();
	}

	void JointMathSliderComponent::setDirectionChange(bool directionChange)
	{
		this->directionChange->setValue(directionChange);
	}

	bool JointMathSliderComponent::getDirectionChange(void) const
	{
		return this->directionChange->getBool();
	}

	/*******************************JointKinematicComponent*******************************/

	JointKinematicComponent::JointKinematicComponent()
		: JointComponent(),
		originPosition(Ogre::Vector3::ZERO),
		originRotation(Ogre::Quaternion::IDENTITY),
		gravity(Ogre::Vector3(0.0f, -16.9f, 0.0f))
	{
		this->type->setReadOnly(false);
		this->type->setValue(this->getClassName());
		this->type->setReadOnly(true);
		this->anchorPosition = new Variant(JointKinematicComponent::AttrAnchorPosition(), Ogre::Vector3::ZERO, this->attributes);
		std::vector<Ogre::String> strModes = { "Linear", "Full 6 Degree Of Freedom", "Linear And Twist", "Linear And Cone", "Linear Plus Angluar Friction" };
		this->mode = new Variant(JointKinematicComponent::AttrMode(), strModes, this->attributes);
		this->maxLinearFriction = new Variant(JointKinematicComponent::AttrMaxLinearFriction(), 10000.0f, this->attributes);
		this->maxAngleFriction = new Variant(JointKinematicComponent::AttrMaxAngleFriction(), 10000.0f, this->attributes);
		this->maxSpeed = new Variant(JointKinematicComponent::AttrMaxSpeed(), 0.0f, this->attributes);
		this->maxOmega = new Variant(JointKinematicComponent::AttrMaxOmega(), 0.0f, this->attributes);
		this->angularViscousFrictionCoefficient = new Variant(JointKinematicComponent::AttrAngularViscousFrictionCoefficient(), 1.0f, this->attributes);
		this->targetPosition = new Variant(JointKinematicComponent::AttrTargetPosition(), Ogre::Vector3::ZERO, this->attributes);
		this->targetRotation = new Variant(JointKinematicComponent::AttrTargetRotation(), Ogre::Vector3::ZERO, this->attributes);
		this->shortTimeActivation = new Variant(JointKinematicComponent::AttrShortTimeActivation(), false, this->attributes);
		
		this->targetId->setVisible(false);
		this->predecessorId->setVisible(false);
		this->anchorPosition->setVisible(false);
		this->mode->setListSelectedValue("Linear Plus Angluar Friction");

		this->maxSpeed->setDescription("Sets the maximum speed in meters per seconds.");
		this->maxOmega->setDescription("Sets the maximum rotation speed in degrees per seconds.");
		this->angularViscousFrictionCoefficient->setDescription("Adds a viscous friction coefficient to the angular rotation (good for setting target position in water e.g.).");
		this->targetPosition->addUserData(GameObject::AttrActionReadOnly());
		this->targetRotation->addUserData(GameObject::AttrActionReadOnly());

		this->targetPosition->setDescription("This values can only be set in code (C++, lua).");
		this->targetRotation->setDescription("This values can only be set in code (C++, lua).");

		// It does not make sense to set those attributes in editor, but just in code!
		this->shortTimeActivation->setDescription("If set to true, this component will be deactivated shortly after it has been activated. Which means, the kinematic joint will take its target transform and remain there.");
	}

	JointKinematicComponent::~JointKinematicComponent()
	{

	}

	GameObjectCompPtr JointKinematicComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		JointKinematicCompPtr clonedJointCompPtr(boost::make_shared<JointKinematicComponent>());
		
		clonedJointCompPtr->setType(this->type->getString());
		clonedJointCompPtr->internalSetPriorId(this->id->getULong());
		clonedJointCompPtr->setPredecessorId(this->predecessorId->getULong());
		clonedJointCompPtr->setTargetId(this->targetId->getULong());
		clonedJointCompPtr->setJointPosition(this->jointPosition);
		clonedJointCompPtr->setJointRecursiveCollisionEnabled(this->jointRecursiveCollision->getBool());
		clonedJointCompPtr->stiffness->setValue(this->stiffness->getReal());

		clonedJointCompPtr->setAnchorPosition(this->anchorPosition->getVector3());
		
		short tempMode = 0;
		if ("Linear" == this->mode->getListSelectedValue())
			tempMode = 0;
		if ("Full 6 Degree Of Freedom" == this->mode->getListSelectedValue())
			tempMode = 1;
		if ("Linear And Twist" == this->mode->getListSelectedValue())
			tempMode = 2;
		else if ("Linear And Cone" == this->mode->getListSelectedValue())
			tempMode = 3;
		else if ("Linear Plus Angluar Friction" == this->mode->getListSelectedValue())
			tempMode = 4;
		
		clonedJointCompPtr->setPickingMode(tempMode);
		clonedJointCompPtr->setMaxLinearAngleFriction(this->maxLinearFriction->getReal(), this->maxAngleFriction->getReal());
		clonedJointCompPtr->setMaxSpeed(this->maxSpeed->getReal());
		clonedJointCompPtr->setMaxOmega(this->maxOmega->getReal());

		clonedJointCompPtr->setAngularViscousFrictionCoefficient(this->angularViscousFrictionCoefficient->getReal());
		clonedJointCompPtr->setTargetPosition(this->targetPosition->getVector3());
		clonedJointCompPtr->setTargetRotation(MathHelper::getInstance()->degreesToQuat(this->targetRotation->getVector3()));
		clonedJointCompPtr->setShortTimeActivation(this->shortTimeActivation->getBool());
		
		clonedGameObjectPtr->addComponent(clonedJointCompPtr);
		clonedJointCompPtr->setOwner(clonedGameObjectPtr);

		clonedJointCompPtr->setActivated(this->activated->getBool());

		return clonedJointCompPtr;
	}

	bool JointKinematicComponent::init(xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		JointComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointAnchorPosition")
		{
			this->anchorPosition->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(0.0f, 0.0f, 0.0f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointPickingMode")
		{
			this->mode->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointMaxLinearFriction")
		{
			this->maxLinearFriction->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointMaxAngleFriction")
		{
			this->maxAngleFriction->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointMaxSpeed")
		{
			this->maxSpeed->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointMaxOmega")
		{
			this->maxOmega->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointAngularViscousFrictionCoefficient")
		{
			this->angularViscousFrictionCoefficient->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointTargetPosition")
		{
			this->targetPosition->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointTargetRotation")
		{
			this->targetRotation->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ShortTimeActivation")
		{
			this->shortTimeActivation->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	bool JointKinematicComponent::postInit(void)
	{
		JointComponent::postInit();

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointKinematicComponent] Init joint kinematic component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool JointKinematicComponent::connect(void)
	{
		bool success = JointComponent::connect();

		if (true == success)
		{
			PhysicsActiveCompPtr physicsActiveCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsActiveComponent>());
			if (nullptr != physicsActiveCompPtr)
			{
				this->gravity = physicsActiveCompPtr->getGravity();
			}
			else
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointKinematicComponent] Error: Final init failed, because the game object: " 
					+ this->gameObjectPtr->getName() + " has no sort of physics active component to set gravity!");
				return false;
			}
		}

		return success;
	}

	bool JointKinematicComponent::disconnect(void)
	{
		bool success = JointComponent::disconnect();
		return success;
	}

	Ogre::String JointKinematicComponent::getClassName(void) const
	{
		return "JointKinematicComponent";
	}

	Ogre::String JointKinematicComponent::getParentClassName(void) const
	{
		return "JointComponent";
	}

	void JointKinematicComponent::update(Ogre::Real dt, bool notSimulating)
	{
		// Checks if the body has reached its target position, and if short time activation is set, the component will be deactivated, if position reached.
		if (false == notSimulating)
		{
			if (true == this->activated->getBool())
			{
				Ogre::Real distSquare = this->getUpdatedJointPosition().squaredDistance(this->targetPosition->getVector3());
				if (distSquare <= 0.01f * 0.01f)
				{
					if (true == this->shortTimeActivation->getBool())
					{
						this->setActivated(false);
					}

					if (this->targetPositionReachedClosureFunction.is_valid())
					{
						try
						{
							luabind::call_function<void>(this->targetPositionReachedClosureFunction);
						}
						catch (luabind::error& error)
						{
							luabind::object errorMsg(luabind::from_stack(error.state(), -1));
							std::stringstream msg;
							msg << errorMsg;

							Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'reactOnTargetPositionReached' Error: " + Ogre::String(error.what())
																		+ " details: " + msg.str());
						}
					}
				}
			}
		}
	}

	bool JointKinematicComponent::createJoint(const Ogre::Vector3& customJointPosition)
	{
		// Joint base created but not activated, return false, but its no error, hence after that return true.
		if (false == JointComponent::createJoint(customJointPosition))
		{
			return true;
		}

		if (nullptr == this->body)
		{
			this->connect();
		}

		if (nullptr == this->body)
		{
			return false;
		}

		if (Ogre::Vector3::ZERO == customJointPosition)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();

			// without rotation
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			this->jointPosition = (this->body->getPosition() + offset);
		}
		else
		{
			this->hasCustomJointPosition = true;
			this->jointPosition = customJointPosition;
		}

		this->originPosition = this->jointPosition;
		this->originRotation = this->body->getOrientation();

		Ogre::String gameObjectName = this->gameObjectPtr->getName();

		OgreNewt::Body* predecessorBody = nullptr;
		if (this->predecessorJointCompPtr)
		{
			predecessorBody = this->predecessorJointCompPtr->getBody();
			if (nullptr != this->predecessorJointCompPtr->getOwner() && nullptr != this->gameObjectPtr)
			{
				Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[JointKinematicComponent] Creating hinge joint for body1 name: "
															+ this->predecessorJointCompPtr->getOwner()->getName() + " body2 name: " + gameObjectName);
			}
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointKinematicComponent] Created hinge joint: " + gameObjectName + " with world as parent");
		}

		// Release joint each time, to create new one with new values
		this->internalReleaseJoint();
		// has no predecessor body
		this->joint = new OgreNewt::KinematicController(this->body, this->jointPosition, predecessorBody);
		OgreNewt::KinematicController* kinematicController = dynamic_cast<OgreNewt::KinematicController*>(this->joint);

		// Better results!
		kinematicController->setSolverModel(0);

		this->joint->setBodyMassScale(this->bodyMassScale->getVector2().x, this->bodyMassScale->getVector2().y);
		this->joint->setCollisionState(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		// this->body->setJointRecursiveCollision(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		// this->applyStiffness();

		short tempMode = 0;
		if ("Linear" == this->mode->getListSelectedValue())
			tempMode = 0;
		if ("Full 6 Degree Of Freedom" == this->mode->getListSelectedValue())
			tempMode = 1;
		if ("Linear And Twist" == this->mode->getListSelectedValue())
			tempMode = 2;
		else if ("Linear And Cone" == this->mode->getListSelectedValue())
			tempMode = 3;
		else if ("Linear Plus Angluar Friction" == this->mode->getListSelectedValue())
			tempMode = 4;
		
		kinematicController->setPickingMode(tempMode);
		// http://newtondynamics.com/forum/viewtopic.php?f=9&t=5608

		Ogre::Real gravity = this->gravity.length();

		float Ixx;
		float Iyy;
		float Izz;
		float mass;
		NewtonBodyGetMass(this->body->getNewtonBody(), &mass, &Ixx, &Iyy, &Izz);

		const dFloat inertia = dMax(Izz, dMax(Ixx, Iyy));

		if (0.0f == gravity)
		{
			gravity = -1.0f;
		}

		kinematicController->setMaxLinearFriction(mass * -gravity * this->maxLinearFriction->getReal());
		kinematicController->setMaxAngularFriction(inertia * this->maxAngleFriction->getReal());
		// Better results, nope
		// kinematicController->setMaxAngularFriction(mass * -gravity * this->maxLinearFriction->getReal());
		kinematicController->setAngularViscousFrictionCoefficient(this->angularViscousFrictionCoefficient->getReal());

		if (0.0f != this->maxSpeed->getReal())
		{
			kinematicController->setMaxSpeed(this->maxSpeed->getReal());
		}
		if (0.0f != this->maxOmega->getReal())
		{
			kinematicController->setMaxOmega(Ogre::Degree(this->maxOmega->getReal()));
		}

		//if (0.0f == gravity)
		//{
		//	gravity = -1.0f;
		//}
		//kinematicController->setMaxLinearFriction(this->body->getMass() * -gravity * this->maxLinearFriction->getReal());
		//kinematicController->setMaxAngularFriction(this->body->getMass() * this->maxAngleFriction->getReal());

		//if (true == this->activated->getBool())
		//{
		//	kinematicController->setTargetPosit(/*this->jointPosition + */this->targetPosition->getVector3());
		//	kinematicController->setTargetRotation(/*this->body->getOrientation() **/ MathHelper::getInstance()->degreesToQuat(this->targetRotation->getVector3()));
		//}
		
		return true;
	}
	
	void JointKinematicComponent::actualizeValue(Variant* attribute)
	{
		JointComponent::actualizeValue(attribute);

		if (JointKinematicComponent::AttrAnchorPosition() == attribute->getName())
		{
			this->setAnchorPosition(attribute->getVector3());
		}
		else if (JointKinematicComponent::AttrMode() == attribute->getName())
		{
			short tempMode = 0;
			if ("Linear" == this->mode->getListSelectedValue())
				tempMode = 0;
			if ("Full 6 Degree Of Freedom" == this->mode->getListSelectedValue())
				tempMode = 1;
			if ("Linear And Twist" == this->mode->getListSelectedValue())
				tempMode = 2;
			else if ("Linear And Cone" == this->mode->getListSelectedValue())
				tempMode = 3;
			else if ("Linear Plus Angluar Friction" == this->mode->getListSelectedValue())
				tempMode = 4;
		}
		else if (JointKinematicComponent::AttrMaxLinearFriction() == attribute->getName())
		{
			this->setMaxLinearAngleFriction(attribute->getReal(), maxAngleFriction->getReal());
		}
		else if (JointKinematicComponent::AttrMaxAngleFriction() == attribute->getName())
		{
			this->setMaxLinearAngleFriction(maxLinearFriction->getReal(), attribute->getReal());
		}
		else if (JointKinematicComponent::AttrMaxSpeed() == attribute->getName())
		{
			this->setMaxSpeed(attribute->getReal());
		}
		else if (JointKinematicComponent::AttrMaxOmega() == attribute->getName())
		{
			this->setMaxOmega(attribute->getReal());
		}
		else if (JointKinematicComponent::AttrAngularViscousFrictionCoefficient() == attribute->getName())
		{
			this->setAngularViscousFrictionCoefficient(attribute->getReal());
		}
		// It does not make sense to set those attributes in editor, but just in code!
#if 0
		else if (JointKinematicComponent::AttrTargetPosition() == attribute->getName())
		{
			this->setTargetPosition(attribute->getVector3());
		}
		else if (JointKinematicComponent::AttrTargetRotation() == attribute->getName())
		{
			this->setTargetRotation(MathHelper::getInstance()->degreesToQuat(attribute->getVector3()));
		}
#endif
		else if (JointKinematicComponent::AttrShortTimeActivation() == attribute->getName())
		{
			this->setShortTimeActivation(attribute->getBool());
		}
	}
	
	void JointKinematicComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		JointComponent::writeXML(propertiesXML, doc, filePath);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointAnchorPosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->anchorPosition->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointPickingMode"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->mode->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointMaxLinearFriction"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxLinearFriction->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointMaxAngleFriction"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxAngleFriction->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointMaxSpeed"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxSpeed->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointMaxOmega"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxOmega->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointAngularViscousFrictionCoefficient"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->angularViscousFrictionCoefficient->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointTargetPosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->targetPosition->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointTargetRotation"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->targetRotation->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ShortTimeActivation"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->shortTimeActivation->getBool())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::Vector3 JointKinematicComponent::getUpdatedJointPosition(void)
	{
		if (nullptr == this->body)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointKinematicComponent] Could not get 'getUpdatedJointPosition', because there is no body for game object: " + this->gameObjectPtr->getName());
			return Ogre::Vector3::ZERO;
		}

		if (false == this->hasCustomJointPosition)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			return this->body->getPosition() + offset;
		}
		else
		{
			return this->jointPosition;
		}
	}

	void JointKinematicComponent::setAnchorPosition(const Ogre::Vector3& anchorPosition)
	{
		this->anchorPosition->setValue(anchorPosition);
	}

	Ogre::Vector3 JointKinematicComponent::getAnchorPosition(void) const
	{
		return this->anchorPosition->getVector3();
	}

	void JointKinematicComponent::setPickingMode(short mode)
	{
		short tempMode = 0;
		if ("Linear" == this->mode->getListSelectedValue())
			tempMode = 0;
		if ("Full 6 Degree Of Freedom" == this->mode->getListSelectedValue())
			tempMode = 1;
		if ("Linear And Twist" == this->mode->getListSelectedValue())
			tempMode = 2;
		else if ("Linear And Cone" == this->mode->getListSelectedValue())
			tempMode = 3;
		else if ("Linear Plus Angluar Friction" == this->mode->getListSelectedValue())
			tempMode = 4;
		
		OgreNewt::KinematicController* kinematicController = dynamic_cast<OgreNewt::KinematicController*>(this->joint);
		if (kinematicController)
		{
			kinematicController->setPickingMode(mode);
		}
	}

	short JointKinematicComponent::getPickingMode(void) const
	{
		short tempMode = 0;
		if ("Linear" == this->mode->getListSelectedValue())
			tempMode = 0;
		if ("Full 6 Degree Of Freedom" == this->mode->getListSelectedValue())
			tempMode = 1;
		if ("Linear And Twist" == this->mode->getListSelectedValue())
			tempMode = 2;
		else if ("Linear And Cone" == this->mode->getListSelectedValue())
			tempMode = 3;
		else if ("Linear Plus Angluar Friction" == this->mode->getListSelectedValue())
			tempMode = 4;
		
		return tempMode;
	}

	void JointKinematicComponent::setMaxLinearAngleFriction(Ogre::Real maxLinearFriction, Ogre::Real maxAngleFriction)
	{
		this->maxLinearFriction->setValue(maxLinearFriction);
		this->maxAngleFriction->setValue(maxAngleFriction);
		OgreNewt::KinematicController* kinematicController = dynamic_cast<OgreNewt::KinematicController*>(this->joint);
		if (kinematicController)
		{
			Ogre::Real gravity = this->gravity.length();

			float Ixx;
			float Iyy;
			float Izz;
			float mass;
			NewtonBodyGetMass(this->body->getNewtonBody(), &mass, &Ixx, &Iyy, &Izz);

			const dFloat inertia = dMax(Izz, dMax(Ixx, Iyy));

			if (0.0f == gravity)
			{
				gravity = -1.0f;
			}

			kinematicController->setMaxLinearFriction(mass * -gravity * this->maxLinearFriction->getReal());
			kinematicController->setMaxAngularFriction(inertia * this->maxAngleFriction->getReal());
			// Better results, nope
			// kinematicController->setMaxAngularFriction(mass * -gravity * this->maxLinearFriction->getReal());

			/*if (0.0f == this->gravity.y)
			{
				this->gravity.y = -1.0f;
			}

			kinematicController->setMaxLinearFriction(this->body->getMass() * -this->gravity.y * this->maxLinearFriction->getReal());
			kinematicController->setMaxAngularFriction(this->body->getMass() * this->maxAngleFriction->getReal());*/
		}
	}

	std::tuple<Ogre::Real, Ogre::Real> JointKinematicComponent::getMaxLinearAngleFriction(void) const
	{
		return std::make_tuple(this->maxLinearFriction->getReal(), this->maxAngleFriction->getReal());
	}

	void JointKinematicComponent::setTargetPosition(const Ogre::Vector3& targetPosition)
	{
		// First activates
		if (true == this->shortTimeActivation->getBool() && false == this->activated->getBool())
		{
			this->setActivated(true);
		}

		this->targetPosition->setValue(targetPosition);
		OgreNewt::KinematicController* kinematicController = dynamic_cast<OgreNewt::KinematicController*>(this->joint);
		if (kinematicController && this->activated->getBool())
		{
			kinematicController->setTargetPosit(this->targetPosition->getVector3());
		}
	}

	Ogre::Vector3 JointKinematicComponent::getTargetPosition(void) const
	{
		return this->targetPosition->getVector3();
	}

	void JointKinematicComponent::setTargetRotation(const Ogre::Quaternion& targetRotation)
	{
		// First activates
		if (true == this->shortTimeActivation->getBool() && false == this->activated->getBool())
		{
			this->setActivated(true);
		}
		this->targetRotation->setValue(MathHelper::getInstance()->quatToDegreesRounded(targetRotation));
		OgreNewt::KinematicController* kinematicController = dynamic_cast<OgreNewt::KinematicController*>(this->joint);
		if (kinematicController && this->activated->getBool())
		{
			kinematicController->setTargetRotation(targetRotation);
		}
	}

	void JointKinematicComponent::setTargetPositionRotation(const Ogre::Vector3& targetPosition, const Ogre::Quaternion& targetRotation)
	{
		// First activates
		if (true == this->shortTimeActivation->getBool() && false == this->activated->getBool())
		{
			this->setActivated(true);
		}
		this->targetPosition->setValue(targetPosition);
		this->targetRotation->setValue(MathHelper::getInstance()->quatToDegreesRounded(targetRotation));
		OgreNewt::KinematicController* kinematicController = static_cast<OgreNewt::KinematicController*>(this->joint);
		if (kinematicController && this->activated->getBool())
		{
			kinematicController->setTargetMatrix(targetPosition, targetRotation);
		}
	}

	Ogre::Quaternion JointKinematicComponent::getTargetRotation(void) const
	{
		return MathHelper::getInstance()->degreesToQuat(this->targetRotation->getVector3());
	}

	void JointKinematicComponent::setMaxSpeed(Ogre::Real speedInMetersPerSeconds)
	{
		if (0.0f == speedInMetersPerSeconds)
		{
			return;
		}
		this->maxSpeed->setValue(speedInMetersPerSeconds);
		OgreNewt::KinematicController* kinematicController = dynamic_cast<OgreNewt::KinematicController*>(this->joint);
		if (kinematicController && this->activated->getBool())
		{
			kinematicController->setMaxSpeed(speedInMetersPerSeconds);
		}
	}

	Ogre::Real JointKinematicComponent::getMaxSpeed(void) const
	{
		return this->maxSpeed->getReal();
	}

	void JointKinematicComponent::setMaxOmega(const Ogre::Real& speedInDegreesPerSeconds)
	{
		if (0.0f == speedInDegreesPerSeconds)
		{
			return;
		}
		this->maxOmega->setValue(speedInDegreesPerSeconds);
		OgreNewt::KinematicController* kinematicController = dynamic_cast<OgreNewt::KinematicController*>(this->joint);
		if (kinematicController && this->activated->getBool())
		{
			kinematicController->setMaxOmega(Ogre::Degree(speedInDegreesPerSeconds));
		}
	}

	void JointKinematicComponent::setAngularViscousFrictionCoefficient(Ogre::Real coefficient)
	{
		this->angularViscousFrictionCoefficient->setValue(coefficient);
	}

	Ogre::Real JointKinematicComponent::getAngularViscousFrictionCoefficient(void) const
	{
		return this->angularViscousFrictionCoefficient->getReal();
	}

	Ogre::Real JointKinematicComponent::getMaxOmega(void) const
	{
		return this->maxOmega->getReal();
	}

	void JointKinematicComponent::backToOrigin(void)
	{
		// First activates
		if (true == this->shortTimeActivation->getBool() && false == this->activated->getBool())
		{
			this->setActivated(true);
		}

		OgreNewt::KinematicController* kinematicController = dynamic_cast<OgreNewt::KinematicController*>(this->joint);
		if (kinematicController && this->activated->getBool())
		{
			kinematicController->setTargetPosit(this->originPosition);
			kinematicController->setTargetRotation(this->originRotation);
		}
	}

	void JointKinematicComponent::setShortTimeActivation(bool shortTimeActivation)
	{
		this->shortTimeActivation->setValue(shortTimeActivation);
	}

	bool JointKinematicComponent::getShortTimeActivation(void) const
	{
		return this->shortTimeActivation->getBool();
	}

	void JointKinematicComponent::reactOnTargetPositionReached(luabind::object closureFunction)
	{
		this->targetPositionReachedClosureFunction = closureFunction;
	}

	/*******************************JointTargetTransformComponent*******************************/

	JointTargetTransformComponent::JointTargetTransformComponent()
		: JointComponent(),
		offsetPosition(Ogre::Vector3::ZERO),
		offsetOrientation(Ogre::Quaternion::IDENTITY),
		gravity(Ogre::Vector3::ZERO)
	{
		this->type->setReadOnly(false);
		this->type->setValue(this->getClassName());
		this->type->setReadOnly(true);
		this->activated = new Variant(JointTargetTransformComponent::AttrActivated(), true, this->attributes);
		this->anchorPosition = new Variant(JointTargetTransformComponent::AttrAnchorPosition(), Ogre::Vector3::ZERO, this->attributes);
	}

	JointTargetTransformComponent::~JointTargetTransformComponent()
	{

	}

	GameObjectCompPtr JointTargetTransformComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		JointTargetTransformCompPtr clonedJointCompPtr(boost::make_shared<JointTargetTransformComponent>());
		
		clonedJointCompPtr->setType(this->type->getString());
		clonedJointCompPtr->internalSetPriorId(this->id->getULong());
		clonedJointCompPtr->setPredecessorId(this->predecessorId->getULong());
		clonedJointCompPtr->setTargetId(this->targetId->getULong());
		clonedJointCompPtr->setJointPosition(this->jointPosition);
		clonedJointCompPtr->setJointRecursiveCollisionEnabled(this->jointRecursiveCollision->getBool());
		clonedJointCompPtr->stiffness->setValue(this->stiffness->getReal());

		clonedJointCompPtr->setAnchorPosition(this->anchorPosition->getVector3());

		clonedGameObjectPtr->addComponent(clonedJointCompPtr);
		clonedJointCompPtr->setOwner(clonedGameObjectPtr);

		clonedJointCompPtr->setActivated(this->activated->getBool());

		return clonedJointCompPtr;
	}

	bool JointTargetTransformComponent::init(xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		JointComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointAnchorPosition")
		{
			this->anchorPosition->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(0.0f, 0.0f, 0.0f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	bool JointTargetTransformComponent::postInit(void)
	{
		JointComponent::postInit();

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointTargetTransformComponent] Init joint target transform component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool JointTargetTransformComponent::connect(void)
	{
		bool success = JointComponent::connect();

		if (true == success)
		{
			PhysicsActiveCompPtr physicsActiveCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsActiveComponent>());
			if (nullptr != physicsActiveCompPtr)
			{
				this->gravity = physicsActiveCompPtr->getGravity();
			}
			else
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointTargetTransformComponent] Error: Final init failed, because the game object: "
																+ this->gameObjectPtr->getName() + " has no sort of physics active component to set gravity!");
				return false;
			}
		}

		return success;
	}

	bool JointTargetTransformComponent::disconnect(void)
	{
		bool success = JointComponent::disconnect();
		return success;
	}

	Ogre::String JointTargetTransformComponent::getClassName(void) const
	{
		return "JointTargetTransformComponent";
	}

	Ogre::String JointTargetTransformComponent::getParentClassName(void) const
	{
		return "JointComponent";
	}

//	void JointTargetTransformComponent::update(Ogre::Real dt, bool notSimulating)
//	{
//		if (nullptr != this->joint && true == this->activated->getBool())
//		{
//			OgreNewt::KinematicController* kinematicController = static_cast<OgreNewt::KinematicController*>(this->joint);
//			kinematicController->setTargetPosit(this->predecessorJointCompPtr->getBody()->getPosition() + this->offsetPosition);
//			// kinematicController->setTargetRotation(this->predecessorJointCompPtr->getBody()->getOrientation() * this->offsetOrientation);
//
//			Ogre::Quaternion q = this->predecessorJointCompPtr->getBody()->getOgreNode()->getParent()->getParent()->convertLocalToWorldOrientation(this->predecessorJointCompPtr->getBody()->getOgreNode()->_getDerivedOrientation());
//			kinematicController->setTargetRotation(q * this->offsetOrientation);
//
//			// MathHelper::getInstance()->localToGlobal<Ogre::SceneNode>()
//#if 0
//			Ogre::Vector3 position;
//			Ogre::Quaternion q0;
//			kinematicController->getTargetMatrix(position, q0);
//			Ogre::Quaternion q1 = this->predecessorJointCompPtr->getBody()->getOrientation() * this->offsetOrientation;
//
//			Ogre::Quaternion q = q1 * q0.Inverse();
//			kinematicController->setTargetRotation(q);
//#endif
//		}
//	}

	void JointTargetTransformComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (nullptr != this->joint && true == this->activated->getBool())
		{
			OgreNewt::KinematicController* kinematicController = static_cast<OgreNewt::KinematicController*>(this->joint);
			kinematicController->setTargetPosit(this->predecessorJointCompPtr->getJointPosition());
			kinematicController->setTargetRotation(this->predecessorJointCompPtr->getOrientation());
		}
	}

	bool JointTargetTransformComponent::createJoint(const Ogre::Vector3& customJointPosition)
	{
		// Joint base created but not activated, return false, but its no error, hence after that return true.
		if (false == JointComponent::createJoint(customJointPosition))
		{
			return true;
		}

		if (nullptr == this->body)
		{
			this->connect();
		}

		if (Ogre::Vector3::ZERO == customJointPosition)
		{
			this->jointPosition = this->body->getPosition() + this->anchorPosition->getVector3();
		}
		else
		{
			this->hasCustomJointPosition = true;
			this->jointPosition = customJointPosition;
		}

		if (nullptr != this->predecessorJointCompPtr)
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[JointKinematicComponent] Creating target transform joint for body1 (predecessor) name: "
				+ this->predecessorJointCompPtr->getOwner()->getName() + " body2 name: " + this->gameObjectPtr->getName());
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointKinematicComponent] Could not create target transform joint: " + this->gameObjectPtr->getName() + " because the predecessor path body does not exist.");
			return false;
		}

		// Release joint each time, to create new one with new values
		this->internalReleaseJoint();
		// has no predecessor body
		this->joint = new OgreNewt::KinematicController(this->body, this->jointPosition);
		OgreNewt::KinematicController* kinematicController = static_cast<OgreNewt::KinematicController*>(this->joint);
		/*
		if ("Linear" == this->mode->getListSelectedValue())
			tempMode = 0;
		if ("Full 6 Degree Of Freedom" == this->mode->getListSelectedValue())
			tempMode = 1;
		if ("Linear And Twist" == this->mode->getListSelectedValue())
			tempMode = 2;
		else if ("Linear And Cone" == this->mode->getListSelectedValue())
			tempMode = 3;
		else if ("Linear Plus Angluar Friction" == this->mode->getListSelectedValue())
			tempMode = 4;
		*/


		/*kinematicController->setPickingMode(4);
		kinematicController->setMaxLinearFriction(1000.0f);
		kinematicController->setMaxAngularFriction(40.0f);
		kinematicController->setMaxSpeed(10.0f);
		kinematicController->setMaxOmega(Ogre::Degree(0.0f));*/

		Ogre::Real gravity = this->gravity.length();

		if (0.0f == gravity)
		{
			gravity = -1.0f;
		}

		dFloat Ixx;
		dFloat Iyy;
		dFloat Izz;
		dFloat mass;
		NewtonBodyGetMass(this->body->getNewtonBody(), &mass, &Ixx, &Iyy, &Izz);

		// change this to make the grabbing stronger or weaker
		const dFloat angularFritionAccel = 100000.0f;
		const dFloat linearFrictionAccel = 100000.0f;
		const dFloat inertia = dMax(Izz, dMax(Ixx, Iyy));

		kinematicController->setSolverModel(0);
		kinematicController->setPickingMode(4);

		kinematicController->setMaxLinearFriction(mass * -gravity * linearFrictionAccel);
		kinematicController->setMaxAngularFriction(inertia * angularFritionAccel);
		// Better results nope
		// kinematicController->setMaxAngularFriction(mass * -gravity * angularFritionAccel);

		this->joint->setBodyMassScale(this->bodyMassScale->getVector2().x, this->bodyMassScale->getVector2().y);
		this->joint->setCollisionState(this->jointRecursiveCollision->getBool() == true ? 1 : 0);

		return true;
	}
	
	void JointTargetTransformComponent::actualizeValue(Variant* attribute)
	{
		JointComponent::actualizeValue(attribute);

		if (JointKinematicComponent::AttrAnchorPosition() == attribute->getName())
		{
			this->setAnchorPosition(attribute->getVector3());
		}
	}
	
	void JointTargetTransformComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		JointComponent::writeXML(propertiesXML, doc, filePath);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointAnchorPosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->anchorPosition->getVector3())));
		propertiesXML->append_node(propertyXML);
	}

	void JointTargetTransformComponent::setAnchorPosition(const Ogre::Vector3& anchorPosition)
	{
		this->anchorPosition->setValue(anchorPosition);
	}

	Ogre::Vector3 JointTargetTransformComponent::getAnchorPosition(void) const
	{
		return this->anchorPosition->getVector3();
	}

	Ogre::Vector3 JointTargetTransformComponent::getUpdatedJointPosition(void)
	{
		if (nullptr == this->body)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointTargetTransformComponent] Could not get 'getUpdatedJointPosition', because there is no body for game object: " + this->gameObjectPtr->getName());
			return Ogre::Vector3::ZERO;
		}

		if (false == this->hasCustomJointPosition)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			return this->body->getPosition() + offset;
		}
		else
		{
			return this->jointPosition;
		}
	}

	void JointTargetTransformComponent::setOffsetPosition(const Ogre::Vector3& offsetPosition)
	{
		this->offsetPosition = offsetPosition;
	}

	void JointTargetTransformComponent::setOffsetOrientation(const Ogre::Quaternion& offsetOrientation)
	{
		this->offsetOrientation = offsetOrientation;
	}

	/*******************************JointPathFollowComponent*******************************/

	JointPathFollowComponent::JointPathFollowComponent()
		: JointComponent(),
		lineNode(nullptr),
		lineObjects(nullptr)
	{
		 this->anchorPosition = new Variant(JointPathFollowComponent::AttrAnchorPosition(), Ogre::Vector3::ZERO, this->attributes);
		 this->waypointsCount = new Variant(JointPathFollowComponent::AttrWaypointsCount(), 1, this->attributes);
		 // Since when waypoints count is changed, the whole properties must be refreshed, so that new field may come for way points
		 this->waypointsCount->addUserData(GameObject::AttrActionNeedRefresh());

		 this->type->setReadOnly(false);
		 this->type->setValue(this->getClassName());
		 this->type->setReadOnly(true);
	 
		 this->targetId->setVisible(false);
	}

	JointPathFollowComponent::~JointPathFollowComponent()
	{
		this->destroyLines();
	}

	GameObjectCompPtr JointPathFollowComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		// get also the cloned properties from joint handler base class
		JointPathFollowCompPtr clonedJointCompPtr(boost::make_shared<JointPathFollowComponent>());
		
		clonedJointCompPtr->setType(this->type->getString());
		clonedJointCompPtr->internalSetPriorId(this->id->getULong());
		clonedJointCompPtr->setPredecessorId(this->predecessorId->getULong());
		clonedJointCompPtr->setJointPosition(this->jointPosition);
		clonedJointCompPtr->setJointRecursiveCollisionEnabled(this->jointRecursiveCollision->getBool());
		clonedJointCompPtr->stiffness->setValue(this->stiffness->getReal());

		clonedJointCompPtr->setAnchorPosition(this->anchorPosition->getVector3());
		clonedJointCompPtr->setWaypointsCount(this->waypointsCount->getUInt());

		for (unsigned int i = 0; i < static_cast<unsigned int>(this->waypoints.size()); i++)
		{
			clonedJointCompPtr->setWaypointId(i, this->waypoints[i]->getULong());
		}

		clonedGameObjectPtr->addComponent(clonedJointCompPtr);
		clonedJointCompPtr->setOwner(clonedGameObjectPtr);

		clonedJointCompPtr->setActivated(this->activated->getBool());

		return clonedJointCompPtr;
	}

	bool JointPathFollowComponent::init(xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		JointComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnchorPosition")
		{
			this->anchorPosition->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WaypointsCount")
		{
			this->waypointsCount->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data", 1));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (this->waypoints.size() < this->waypointsCount->getUInt())
		{
			this->waypoints.resize(this->waypointsCount->getUInt());
		}
		for (size_t i = 0; i < this->waypoints.size(); i++)
		{
			if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Waypoint" + Ogre::StringConverter::toString(i))
			{
				if (nullptr == this->waypoints[i])
				{
					this->waypoints[i] = new Variant(JointPathFollowComponent::AttrWaypoint() + Ogre::StringConverter::toString(i), XMLConverter::getAttribUnsignedLong(propertyElement, "data", 0), this->attributes);
				}
				else
				{
					this->waypoints[i]->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
				}
				propertyElement = propertyElement->next_sibling("property");
				this->waypoints[i]->addUserData(GameObject::AttrActionSeparator());
			}
		}
		return true;
	}

	bool JointPathFollowComponent::postInit(void)
	{
		JointComponent::postInit();

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointPathFollowComponent] Init joint path follow component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	Ogre::String JointPathFollowComponent::getClassName(void) const
	{
		return "JointPathFollowComponent";
	}

	Ogre::String JointPathFollowComponent::getParentClassName(void) const
	{
		return "JointComponent";
	}

	void JointPathFollowComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating && true == this->bShowDebugData)
		{
			this->drawLines();
		}
	}

	bool JointPathFollowComponent::connect(void)
	{
		bool success = JointComponent::connect();

		return success;
	}

	bool JointPathFollowComponent::disconnect(void)
	{
		bool success = JointComponent::disconnect();

		if (true == this->bShowDebugData)
		{
			this->destroyLines();
		}

		return success;
	}

	void JointPathFollowComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		JointComponent::writeXML(propertiesXML, doc, filePath);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnchorPosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->anchorPosition->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "WaypointsCount"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->waypointsCount->getUInt())));
		propertiesXML->append_node(propertyXML);

		for (size_t i = 0; i < this->waypoints.size(); i++)
		{
			propertyXML = doc.allocate_node(node_element, "property");
			propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
			propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "Waypoint" + Ogre::StringConverter::toString(i))));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->waypoints[i]->getULong())));
			propertiesXML->append_node(propertyXML);
		}
	}

	Ogre::Vector3 JointPathFollowComponent::getUpdatedJointPosition(void)
	{
		if (nullptr == this->body)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointPathFollowComponent] Could not get 'getUpdatedJointPosition', because there is no body for game object: " + this->gameObjectPtr->getName());
			return Ogre::Vector3::ZERO;
		}

		if (false == this->hasCustomJointPosition)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			return this->body->getPosition() + offset;
		}
		else
		{
			return this->jointPosition;
		}
	}

	bool JointPathFollowComponent::createJoint(const Ogre::Vector3& customJointPosition)
	{
		// Joint base created but not activated, return false, but its no error, hence after that return true.
		if (false == JointComponent::createJoint(customJointPosition))
		{
			return true;
		}

		if (false == this->activated->getBool())
			return true;

		if (nullptr == this->body)
		{
			this->connect();
		}

		if (Ogre::Vector3::ZERO == customJointPosition)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			//Ogre::Vector3 aPos = Ogre::Vector3(-this->anchorPosition.x, -this->anchorPosition.y, -this->anchorPosition.z);
			//// relative anchor pos: 0.5 0 0 means 50% in X of the size of the parent object
			//Ogre::Vector3 offset = (aPos * size);
			//// take orientation into account, so that orientated joints are processed correctly
			//this->jointPosition = this->body->getPosition() - (this->body->getOrientation() * offset);

			// without rotation
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			this->jointPosition = (this->body->getPosition() + offset);
		}
		else
		{
			this->hasCustomJointPosition = true;
			this->jointPosition = customJointPosition;
		}

		OgreNewt::Body* predecessorBody = nullptr;

		if (nullptr != this->predecessorJointCompPtr)
		{
			predecessorBody = this->predecessorJointCompPtr->getBody();
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[JointPathFollowComponent] Creating path follow joint for body1 (predecessor) name: "
				+ this->predecessorJointCompPtr->getOwner()->getName() + " body2 name: " + this->gameObjectPtr->getName());
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointPathFollowComponent] Could not create path follow joint: " + this->gameObjectPtr->getName() + " because the predecessor path body does not exist.");
			return false;
		}

		this->knots.resize(this->waypoints.size() + 1);

		this->knots[0] = this->body->getPosition();

		for (size_t i = 0; i < this->waypoints.size(); i++)
		{
			GameObjectPtr waypointGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->waypoints[i]->getULong());
			if (nullptr != waypointGameObjectPtr)
			{
				auto nodeCompPtr = NOWA::makeStrongPtr(waypointGameObjectPtr->getComponent<NodeComponent>());
				if (nullptr != nodeCompPtr)
				{
					// Add the way points
					this->knots[i + 1] = nodeCompPtr->getPosition();
				}
			}
		}

		// Release joint each time, to create new one with new values
		this->internalReleaseJoint();
		// Has no predecessor body
		this->joint = new OgreNewt::PathFollow(this->body, predecessorBody, this->jointPosition);

		OgreNewt::PathFollow* pathFollow = static_cast<OgreNewt::PathFollow*>(this->joint);
		bool success = pathFollow->setKnots(this->knots);

		if (false == success)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointPathFollowComponent] Could not create path follow joint: " + this->gameObjectPtr->getName() + " because there are no waypoints.");
		}
		else
		{
			if (true == this->bShowDebugData)
			{
				this->createLines();
			}
		}

		return success;
	}

	void JointPathFollowComponent::setAnchorPosition(const Ogre::Vector3& anchorPosition)
	{
		this->anchorPosition->setValue(anchorPosition);
		this->createJoint();
	}

	Ogre::Vector3 JointPathFollowComponent::getAnchorPosition(void) const
	{
		return this->anchorPosition->getVector3();
	}

	void JointPathFollowComponent::actualizeValue(Variant* attribute)
	{
		JointComponent::actualizeValue(attribute);

		if (JointPathFollowComponent::AttrWaypointsCount() == attribute->getName())
		{
			this->setWaypointsCount(attribute->getUInt());
		}
		else
		{
			for (size_t i = 0; i < this->waypoints.size(); i++)
			{
				if (JointPathFollowComponent::AttrWaypoint() + Ogre::StringConverter::toString(i) == attribute->getName())
				{
					this->waypoints[i]->setValue(attribute->getULong());
				}
			}
		}
	}

	void JointPathFollowComponent::setWaypointsCount(unsigned int waypointsCount)
	{
		this->waypointsCount->setValue(waypointsCount);

		size_t oldSize = this->waypoints.size();

		if (waypointsCount > oldSize)
		{
			// Resize the waypoints array for count
			this->waypoints.resize(waypointsCount);

			for (size_t i = oldSize; i < this->waypoints.size(); i++)
			{
				this->waypoints[i] = new Variant(JointPathFollowComponent::AttrWaypoint() + Ogre::StringConverter::toString(i), static_cast<unsigned long>(0), this->attributes, true);
				this->waypoints[i]->addUserData(GameObject::AttrActionSeparator());
			}
		}
		else if (waypointsCount < oldSize)
		{
			this->eraseVariants(this->waypoints, waypointsCount);
		}
		this->createJoint();
	}

	unsigned int JointPathFollowComponent::getWaypointsCount(void) const
	{
		return this->waypointsCount->getUInt();
	}

	void JointPathFollowComponent::setWaypointId(unsigned long id, unsigned int index)
	{
		if (index > this->waypoints.size())
			return;
		this->waypoints[index]->setValue(id);
		this->createJoint();
	}

	unsigned long JointPathFollowComponent::getWaypointId(unsigned int index)
	{
		if (index > this->waypoints.size())
			return 0;
		return this->waypoints[index]->getULong();
	}

	void JointPathFollowComponent::createLines(void)
	{
		this->destroyLines();

		this->lineNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode();
		this->lineObjects = this->gameObjectPtr->getSceneManager()->createManualObject();
		this->lineObjects->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);
		this->lineObjects->setName("Lines_" + Ogre::StringConverter::toString(this->gameObjectPtr->getId()));
		this->lineObjects->setQueryFlags(0 << 0);
		this->lineNode->attachObject(this->lineObjects);
		this->lineObjects->setCastShadows(false);
	}

	void JointPathFollowComponent::drawLines(void)
	{
		this->lineObjects->clear();
		this->lineObjects->begin("WhiteNoLightingBackground", Ogre::OperationType::OT_LINE_LIST);

		for (size_t i = 0; i < this->knots.size(); i++)
		{
			if (i < this->knots.size() - 1)
			{
				this->lineObjects->position(this->knots[i]);
				this->lineObjects->colour(Ogre::ColourValue::White);
				this->lineObjects->index(2 * i);

				this->lineObjects->position(this->knots[i + 1]);
				this->lineObjects->colour(Ogre::ColourValue::White);
				this->lineObjects->index(2 * i + 1);
			}
		}

		this->lineObjects->position(this->knots[this->knots.size() - 1]);
		this->lineObjects->colour(Ogre::ColourValue::White);
		this->lineObjects->index(2 * (this->knots.size() - 1));

		this->lineObjects->position(this->knots[0]);
		this->lineObjects->colour(Ogre::ColourValue::White);
		this->lineObjects->index(0);

		this->lineObjects->end();
	}

	void JointPathFollowComponent::destroyLines(void)
	{
		if (this->lineNode != nullptr)
		{
			this->lineNode->detachAllObjects();
			this->gameObjectPtr->getSceneManager()->destroyManualObject(this->lineObjects);
			this->lineObjects = nullptr;
			this->lineNode->getParentSceneNode()->removeAndDestroyChild(this->lineNode);
			this->lineNode = nullptr;
		}
	}

	/*******************************JointDryRollingFrictionComponent*******************************/

	JointDryRollingFrictionComponent::JointDryRollingFrictionComponent()
		: JointComponent()
	{
		this->type->setReadOnly(false);
		this->type->setValue(this->getClassName());
		this->type->setReadOnly(true);
		
		this->radius = new Variant(JointDryRollingFrictionComponent::AttrRadius(), 1.0f, this->attributes);
		this->rollingFrictionCoefficient = new Variant(JointDryRollingFrictionComponent::AttrFrictionCoefficient(), 0.5f, this->attributes);

		this->targetId->setVisible(false);
	}

	JointDryRollingFrictionComponent::~JointDryRollingFrictionComponent()
	{

	}

	GameObjectCompPtr JointDryRollingFrictionComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		JointDryRollingFrictionCompPtr clonedJointCompPtr(boost::make_shared<JointDryRollingFrictionComponent>());
		
		clonedJointCompPtr->setType(this->type->getString());
		clonedJointCompPtr->internalSetPriorId(this->id->getULong());
		clonedJointCompPtr->setPredecessorId(this->predecessorId->getULong());
		clonedJointCompPtr->setTargetId(this->targetId->getULong());
		clonedJointCompPtr->setJointPosition(this->jointPosition);
		clonedJointCompPtr->setJointRecursiveCollisionEnabled(this->jointRecursiveCollision->getBool());
		clonedJointCompPtr->stiffness->setValue(this->stiffness->getReal());

		clonedJointCompPtr->setRadius(this->radius->getReal());
		clonedJointCompPtr->setRollingFrictionCoefficient(this->rollingFrictionCoefficient->getReal());

		clonedGameObjectPtr->addComponent(clonedJointCompPtr);
		clonedJointCompPtr->setOwner(clonedGameObjectPtr);

		clonedJointCompPtr->setActivated(this->activated->getBool());

		return clonedJointCompPtr;
	}

	bool JointDryRollingFrictionComponent::init(xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		JointComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointRadius")
		{
			this->radius->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointFrictionCoefficient")
		{
			this->rollingFrictionCoefficient->setValue(XMLConverter::getAttribReal(propertyElement, "data", 0.5f));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	bool JointDryRollingFrictionComponent::postInit(void)
	{
		JointComponent::postInit();

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointDryRollingFrictionComponent] Init joint dry rolling friction component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool JointDryRollingFrictionComponent::connect(void)
	{
		bool success = JointComponent::connect();
		return success;
	}

	bool JointDryRollingFrictionComponent::disconnect(void)
	{
		bool success = JointComponent::disconnect();
		return success;
	}

	Ogre::String JointDryRollingFrictionComponent::getClassName(void) const
	{
		return "JointDryRollingFrictionComponent";
	}

	Ogre::String JointDryRollingFrictionComponent::getParentClassName(void) const
	{
		return "JointComponent";
	}

	void JointDryRollingFrictionComponent::update(Ogre::Real dt, bool notSimulating)
	{

	}

	bool JointDryRollingFrictionComponent::createJoint(const Ogre::Vector3& customJointPosition)
	{
		// Joint base created but not activated, return false, but its no error, hence after that return true.
		if (false == JointComponent::createJoint(customJointPosition))
		{
			return true;
		}

		if (nullptr == this->body)
		{
			this->connect();
		}
		
		if (true == this->jointAlreadyCreated)
		{
			// joint already created so skip
			return true;
		}

		// Release joint each time, to create new one with new values
		this->internalReleaseJoint();
		this->joint = new OgreNewt::CustomDryRollingFriction(this->body, this->radius->getReal(), this->rollingFrictionCoefficient->getReal());
		
		return true;
	}
	
	void JointDryRollingFrictionComponent::actualizeValue(Variant* attribute)
	{
		JointComponent::actualizeValue(attribute);

		if (JointDryRollingFrictionComponent::AttrRadius() == attribute->getName())
		{
			this->setRadius(attribute->getReal());
		}
		else if (JointDryRollingFrictionComponent::AttrFrictionCoefficient() == attribute->getName())
		{
			this->setRollingFrictionCoefficient(attribute->getReal());
		}
	}
	
	void JointDryRollingFrictionComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		JointComponent::writeXML(propertiesXML, doc, filePath);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointRadius"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->radius->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointFrictionCoefficient"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->rollingFrictionCoefficient->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	void JointDryRollingFrictionComponent::setRadius(Ogre::Real radius)
	{
		this->radius->setValue(radius);
	}

	Ogre::Real JointDryRollingFrictionComponent::getRadius(void) const
	{
		return this->radius->getReal();
	}

	void JointDryRollingFrictionComponent::setRollingFrictionCoefficient(Ogre::Real rollingFrictionCoefficient)
	{
		this->rollingFrictionCoefficient->setValue(rollingFrictionCoefficient);
	}

	Ogre::Real JointDryRollingFrictionComponent::getRollingFrictionCoefficient(void) const
	{
		return this->rollingFrictionCoefficient->getReal();
	}

	/*******************************JointGearComponent*******************************/

	JointGearComponent::JointGearComponent()
		: JointComponent()
	{
		this->type->setReadOnly(false);
		this->type->setValue(this->getClassName());
		this->type->setReadOnly(true);
		
		this->type->setDescription("Needs 2 hinge joint components in this game object");
		
		this->gearRatio = new Variant(JointGearComponent::AttrGearRatio(), 1.0f, this->attributes);
	}

	JointGearComponent::~JointGearComponent()
	{

	}

	GameObjectCompPtr JointGearComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		JointGearCompPtr clonedJointCompPtr(boost::make_shared<JointGearComponent>());
		
		clonedJointCompPtr->setType(this->type->getString());
		clonedJointCompPtr->internalSetPriorId(this->id->getULong());
		clonedJointCompPtr->setPredecessorId(this->predecessorId->getULong());
		clonedJointCompPtr->setTargetId(this->targetId->getULong());
		clonedJointCompPtr->setJointPosition(this->jointPosition);
		clonedJointCompPtr->setJointRecursiveCollisionEnabled(this->jointRecursiveCollision->getBool());
		clonedJointCompPtr->stiffness->setValue(this->stiffness->getReal());

		clonedJointCompPtr->setGearRatio(this->gearRatio->getReal());

		clonedGameObjectPtr->addComponent(clonedJointCompPtr);
		clonedJointCompPtr->setOwner(clonedGameObjectPtr);

		clonedJointCompPtr->setActivated(this->activated->getBool());

		return clonedJointCompPtr;
	}

	bool JointGearComponent::init(xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		JointComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "GearRatio")
		{
			this->gearRatio->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	bool JointGearComponent::postInit(void)
	{
		JointComponent::postInit();

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointGearComponent] Init joint gear component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool JointGearComponent::connect(void)
	{
		bool success = JointComponent::connect();

		// See PhysicsActive component, check if this game object has other components to attach this to
		//JointHandlerPtr tempJointHandlerPtr;
		//JointHandlerPtr targetJointHandlerPtr;
		//do
		//{
		//	// Remember, not every component has a joint, so null is valid too
		//	tempJointHandlerPtr = JointHandlerFactory::getInstance()->createJointHandler(propertyElement);
		//	if (nullptr != tempJointHandlerPtr)
		//	{
		//		if ("KinematicController" == tempJointHandlerPtr->getType())
		//		{
		//			// Set gravity for friction equations
		//			boost::shared_ptr<JointKinematicHandler> jointKinematicHandlerPtr = boost::dynamic_pointer_cast<JointKinematicHandler>(tempJointHandlerPtr);
		//			jointKinematicHandlerPtr->setGravity(this->gravity->getVector3());
		//		};

		//		// There are joint which cannot live without a target joint, like the pulley needs as target joint a passive slider
		//		// so set target joint
		//		if (nullptr != targetJointHandlerPtr)
		//		{
		//			tempJointHandlerPtr->setTargetId(targetJointHandlerPtr->getId());
		//		}
		//		this->jointHandlers.emplace_back(tempJointHandlerPtr);
		//	}
		//	targetJointHandlerPtr = tempJointHandlerPtr;
		//} while (nullptr != tempJointHandlerPtr);
		return success;
	}

	bool JointGearComponent::disconnect(void)
	{
		bool success = JointComponent::disconnect();
		return success;
	}

	Ogre::String JointGearComponent::getClassName(void) const
	{
		return "JointGearComponent";
	}

	Ogre::String JointGearComponent::getParentClassName(void) const
	{
		return "JointComponent";
	}

	void JointGearComponent::update(Ogre::Real dt, bool notSimulating)
	{

	}

	bool JointGearComponent::createJoint(const Ogre::Vector3& customJointPosition)
	{
		// Joint base created but not activated, return false, but its no error, hence after that return true.
		if (false == JointComponent::createJoint(customJointPosition))
		{
			return true;
		}

		if (nullptr == this->body)
		{
			this->connect();
		}

		//if (true == this->jointAlreadyCreated)
		//{
		//	// joint already created so skip
		//	return true;
		//}

		OgreNewt::Body* predecessorBody = nullptr;
		
		if (nullptr != this->predecessorJointCompPtr)
		{
			predecessorBody = this->predecessorJointCompPtr->getBody();
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[JointGearComponent] Creating gear joint for body1 (predecessor) name: "
				+ this->predecessorJointCompPtr->getOwner()->getName() + " body2 name: " + this->gameObjectPtr->getName());
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointGearComponent] Could not create gear joint: " + this->gameObjectPtr->getName() + " because the other body does not exist.");
			return false;
		}
		
		if (nullptr == this->targetJointCompPtr)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointGearComponent] Error could not create gear for joint: " + this->gameObjectPtr->getName()
				+ ", because the target joint handler does not exist.");
			return false;
		}
		JointHingeCompPtr jointHingeCompPtr1 = boost::dynamic_pointer_cast<JointHingeComponent>(this->targetJointCompPtr);
		JointHingeActuatorCompPtr jointHingeActuatorCompPtr1;
		if (nullptr == jointHingeCompPtr1)
		{
			jointHingeActuatorCompPtr1 = boost::dynamic_pointer_cast<JointHingeActuatorComponent>(this->targetJointCompPtr);
			if (nullptr == jointHingeActuatorCompPtr1)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointGearComponent] Error could not create gear for joint: " + this->gameObjectPtr->getName()
					+ ", because the target joint handler does not exist or is no hinge.");
				return false;
			}
		}
		JointHingeCompPtr jointHingeCompPtr2 = boost::dynamic_pointer_cast<JointHingeComponent>(this->predecessorJointCompPtr);
		JointHingeActuatorCompPtr jointHingeActuatorCompPtr2;
		if (nullptr == jointHingeCompPtr2)
		{
			jointHingeActuatorCompPtr2 = boost::dynamic_pointer_cast<JointHingeActuatorComponent>(this->predecessorJointCompPtr);
			if (nullptr == jointHingeActuatorCompPtr2)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointGearComponent] Error could not create gear for joint: " + this->gameObjectPtr->getName()
					+ ", because the predecessor joint handler does not exist or is no hinge.");
				return false;
			}
		}

		// Release joint each time, to create new one with new values
		this->internalReleaseJoint();

		if (jointHingeCompPtr1 && jointHingeCompPtr2)
		{
			this->joint = new OgreNewt::Gear(jointHingeCompPtr1->getBody(), jointHingeCompPtr2->getBody(),
				jointHingeCompPtr1->getBody()->getOrientation() * jointHingeCompPtr1->getPin(), jointHingeCompPtr2->getBody()->getOrientation() * jointHingeCompPtr2->getPin(), this->gearRatio->getReal());
		}
		else if (jointHingeActuatorCompPtr1 && jointHingeCompPtr2)
		{
			this->joint = new OgreNewt::Gear(jointHingeActuatorCompPtr1->getBody(), jointHingeCompPtr2->getBody(),
				jointHingeActuatorCompPtr1->getBody()->getOrientation() * jointHingeActuatorCompPtr1->getPin(), jointHingeCompPtr2->getBody()->getOrientation() * jointHingeCompPtr2->getPin(), this->gearRatio->getReal());
		}
		else if (jointHingeCompPtr1 && jointHingeActuatorCompPtr2)
		{
			this->joint = new OgreNewt::Gear(jointHingeCompPtr1->getBody(), jointHingeActuatorCompPtr2->getBody(),
				jointHingeCompPtr1->getBody()->getOrientation() * jointHingeCompPtr1->getPin(), jointHingeActuatorCompPtr2->getBody()->getOrientation() * jointHingeActuatorCompPtr2->getPin(), this->gearRatio->getReal());
		}
		
		this->joint->setBodyMassScale(this->bodyMassScale->getVector2().x, this->bodyMassScale->getVector2().y);
		this->joint->setCollisionState(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		// this->body->setJointRecursiveCollision(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		return true;
	}
	
	void JointGearComponent::actualizeValue(Variant* attribute)
	{
		JointComponent::actualizeValue(attribute);

		if (JointGearComponent::AttrGearRatio() == attribute->getName())
		{
			this->setGearRatio(attribute->getReal());
		}
	}
	
	void JointGearComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		JointComponent::writeXML(propertiesXML, doc, filePath);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "GearRatio"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->gearRatio->getReal())));
		propertiesXML->append_node(propertyXML);
	}
	
	void JointGearComponent::setGearRatio(Ogre::Real gearRatio)
	{
		this->gearRatio->setValue(gearRatio);
	}

	Ogre::Real JointGearComponent::getGearRatio(void) const
	{
		return this->gearRatio->getReal();
	}
		
	/*******************************JointRackAndPinionComponent*******************************/

	JointRackAndPinionComponent::JointRackAndPinionComponent()
		: JointComponent()
	{
		this->type->setReadOnly(false);
		this->type->setValue(this->getClassName());
		this->type->setReadOnly(true);
		
		this->type->setDescription("Needs 2 hinge joint components in this game object");
		
		this->gearRatio = new Variant(JointRackAndPinionComponent::AttrGearRatio(), 1.0f, this->attributes);
	}

	JointRackAndPinionComponent::~JointRackAndPinionComponent()
	{

	}

	GameObjectCompPtr JointRackAndPinionComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		JointRackAndPinionCompPtr clonedJointCompPtr(boost::make_shared<JointRackAndPinionComponent>());
		
		clonedJointCompPtr->setType(this->type->getString());
		clonedJointCompPtr->internalSetPriorId(this->id->getULong());
		clonedJointCompPtr->setPredecessorId(this->predecessorId->getULong());
		clonedJointCompPtr->setTargetId(this->targetId->getULong());
		clonedJointCompPtr->setJointPosition(this->jointPosition);
		clonedJointCompPtr->setJointRecursiveCollisionEnabled(this->jointRecursiveCollision->getBool());
		clonedJointCompPtr->stiffness->setValue(this->stiffness->getReal());

		clonedJointCompPtr->setGearRatio(this->gearRatio->getReal());

		clonedGameObjectPtr->addComponent(clonedJointCompPtr);
		clonedJointCompPtr->setOwner(clonedGameObjectPtr);

		clonedJointCompPtr->setActivated(this->activated->getBool());

		return clonedJointCompPtr;
	}

	bool JointRackAndPinionComponent::init(xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		JointComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointGearRatio")
		{
			this->gearRatio->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	bool JointRackAndPinionComponent::postInit(void)
	{
		JointComponent::postInit();

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointRackAndPinionComponent] Init joint rack and pinion component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool JointRackAndPinionComponent::connect(void)
	{
		bool success = JointComponent::connect();

		// See PhysicsActive component, check if this game object has other components to attach this to
		//JointHandlerPtr tempJointHandlerPtr;
		//JointHandlerPtr targetJointHandlerPtr;
		//do
		//{
		//	// Remember, not every component has a joint, so null is valid too
		//	tempJointHandlerPtr = JointHandlerFactory::getInstance()->createJointHandler(propertyElement);
		//	if (nullptr != tempJointHandlerPtr)
		//	{
		//		if ("KinematicController" == tempJointHandlerPtr->getType())
		//		{
		//			// Set gravity for friction equations
		//			boost::shared_ptr<JointKinematicHandler> jointKinematicHandlerPtr = boost::dynamic_pointer_cast<JointKinematicHandler>(tempJointHandlerPtr);
		//			jointKinematicHandlerPtr->setGravity(this->gravity->getVector3());
		//		};

		//		// There are joint which cannot live without a target joint, like the pulley needs as target joint a passive slider
		//		// so set target joint
		//		if (nullptr != targetJointHandlerPtr)
		//		{
		//			tempJointHandlerPtr->setTargetId(targetJointHandlerPtr->getId());
		//		}
		//		this->jointHandlers.emplace_back(tempJointHandlerPtr);
		//	}
		//	targetJointHandlerPtr = tempJointHandlerPtr;
		//} while (nullptr != tempJointHandlerPtr);
		return success;
	}

	bool JointRackAndPinionComponent::disconnect(void)
	{
		bool success = JointComponent::disconnect();
		return success;
	}

	Ogre::String JointRackAndPinionComponent::getClassName(void) const
	{
		return "JointRackAndPinionComponent";
	}

	Ogre::String JointRackAndPinionComponent::getParentClassName(void) const
	{
		return "JointComponent";
	}

	void JointRackAndPinionComponent::update(Ogre::Real dt, bool notSimulating)
	{

	}

	bool JointRackAndPinionComponent::createJoint(const Ogre::Vector3& customJointPosition)
	{
		// Joint base created but not activated, return false, but its no error, hence after that return true.
		if (false == JointComponent::createJoint(customJointPosition))
		{
			return true;
		}

		if (nullptr == this->body)
		{
			this->connect();
		}
		
		if (nullptr == this->targetJointCompPtr)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointRackAndPinionComponent] Error could not create rack and pinion for joint: " + this->gameObjectPtr->getName()
				+ ", because the target joint handler does not exist.");
			return false;
		}
		JointHingeCompPtr jointHingeCompPtr1 = boost::dynamic_pointer_cast<JointHingeComponent>(this->targetJointCompPtr);
		JointHingeActuatorCompPtr jointHingeActuatorCompPtr1;
		if (nullptr == jointHingeCompPtr1)
		{
			jointHingeActuatorCompPtr1 = boost::dynamic_pointer_cast<JointHingeActuatorComponent>(this->targetJointCompPtr);
			if (nullptr == jointHingeActuatorCompPtr1)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointRackAndPinionComponent] Error could not create rack and pinion for joint: " + this->gameObjectPtr->getName()
					+ ", because the predecessor joint handler does not exist or is no hinge.");
				return false;
			}
		}

		JointPassiveSliderCompPtr jointPassiveSliderCompPtr2 = boost::dynamic_pointer_cast<JointPassiveSliderComponent>(this->predecessorJointCompPtr);
		JointActiveSliderCompPtr jointActiveSliderCompPtr2;
		JointSliderActuatorCompPtr jointSliderActuatorCompPtr2;

		if (nullptr == jointPassiveSliderCompPtr2)
		{
			jointActiveSliderCompPtr2 = boost::dynamic_pointer_cast<JointActiveSliderComponent>(this->predecessorJointCompPtr);
			if (nullptr == jointActiveSliderCompPtr2)
			{
				jointSliderActuatorCompPtr2 = boost::dynamic_pointer_cast<JointSliderActuatorComponent>(this->predecessorJointCompPtr);
				if (nullptr == jointSliderActuatorCompPtr2)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointRackAndPinionComponent] Error could not create rack and pinion for joint: " + this->gameObjectPtr->getName()
						+ ", because the target joint handler does not exist or is no slider.");
					return false;
				}
			}
		}

		// Release joint each time, to create new one with new values
		this->internalReleaseJoint();
		// Worm gear: target hinge, predecessor slider
		if (jointHingeCompPtr1 && jointPassiveSliderCompPtr2)
		{
			this->joint = new OgreNewt::WormGear(jointHingeCompPtr1->getBody(), jointPassiveSliderCompPtr2->getBody(), 
				jointHingeCompPtr1->getBody()->getOrientation() * jointHingeCompPtr1->getPin(), jointPassiveSliderCompPtr2->getBody()->getOrientation() * jointPassiveSliderCompPtr2->getPin(),
				this->gearRatio->getReal());
		}
		else if (jointHingeCompPtr1 && jointSliderActuatorCompPtr2)
		{
			this->joint = new OgreNewt::WormGear(jointHingeCompPtr1->getBody(), jointSliderActuatorCompPtr2->getBody(),
				jointHingeCompPtr1->getBody()->getOrientation() * jointHingeCompPtr1->getPin(), jointSliderActuatorCompPtr2->getBody()->getOrientation() * jointSliderActuatorCompPtr2->getPin(),
				this->gearRatio->getReal());
		}
		else if (jointHingeActuatorCompPtr1 && jointPassiveSliderCompPtr2)
		{
			this->joint = new OgreNewt::WormGear(jointHingeActuatorCompPtr1->getBody(), jointPassiveSliderCompPtr2->getBody(),
				jointHingeActuatorCompPtr1->getBody()->getOrientation() * jointHingeActuatorCompPtr1->getPin(), jointPassiveSliderCompPtr2->getBody()->getOrientation() * jointPassiveSliderCompPtr2->getPin(),
				this->gearRatio->getReal());
		}
		else
		{
			// Predecessor may also be an active slider
			this->joint = new OgreNewt::WormGear(jointHingeCompPtr1->getBody(), jointActiveSliderCompPtr2->getBody(), 
				jointHingeCompPtr1->getBody()->getOrientation() * jointHingeCompPtr1->getPin(), jointActiveSliderCompPtr2->getBody()->getOrientation() * jointActiveSliderCompPtr2->getPin(),
				this->gearRatio->getReal());
		}

		this->joint->setBodyMassScale(this->bodyMassScale->getVector2().x, this->bodyMassScale->getVector2().y);
		this->joint->setCollisionState(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		// this->body->setJointRecursiveCollision(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		return true;
	}
	
	void JointRackAndPinionComponent::actualizeValue(Variant* attribute)
	{
		JointComponent::actualizeValue(attribute);

		if (JointRackAndPinionComponent::AttrGearRatio() == attribute->getName())
		{
			this->setGearRatio(attribute->getReal());
		}
	}
	
	void JointRackAndPinionComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		JointComponent::writeXML(propertiesXML, doc, filePath);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Ratio"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->gearRatio->getReal())));
		propertiesXML->append_node(propertyXML);
	}
	
	void JointRackAndPinionComponent::setGearRatio(Ogre::Real gearRatio)
	{
		this->gearRatio->setValue(gearRatio);
	}

	Ogre::Real JointRackAndPinionComponent::getGearRatio(void) const
	{
		return this->gearRatio->getReal();
	}

	/*******************************JointWormGearComponent*******************************/

	JointWormGearComponent::JointWormGearComponent()
		: JointComponent()
	{
		this->type->setReadOnly(false);
		this->type->setValue(this->getClassName());
		this->type->setReadOnly(true);
		
		this->type->setDescription("Needs 2 hinge joint components or one and a kind of slider component in this game object");
		
		this->gearRatio = new Variant(JointWormGearComponent::AttrGearRatio(), 1.0f, this->attributes);
		this->slideRatio = new Variant(JointWormGearComponent::AttrSlideRatio(), 1.0f, this->attributes);
	}

	JointWormGearComponent::~JointWormGearComponent()
	{

	}

	GameObjectCompPtr JointWormGearComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		JointWormGearCompPtr clonedJointCompPtr(boost::make_shared<JointWormGearComponent>());
		
		clonedJointCompPtr->setType(this->type->getString());
		clonedJointCompPtr->internalSetPriorId(this->id->getULong());
		clonedJointCompPtr->setPredecessorId(this->predecessorId->getULong());
		clonedJointCompPtr->setTargetId(this->targetId->getULong());
		clonedJointCompPtr->setJointPosition(this->jointPosition);
		clonedJointCompPtr->setJointRecursiveCollisionEnabled(this->jointRecursiveCollision->getBool());
		clonedJointCompPtr->stiffness->setValue(this->stiffness->getReal());

		clonedJointCompPtr->setGearRatio(this->gearRatio->getReal());
		clonedJointCompPtr->setSlideRatio(this->slideRatio->getReal());

		clonedGameObjectPtr->addComponent(clonedJointCompPtr);
		clonedJointCompPtr->setOwner(clonedGameObjectPtr);

		clonedJointCompPtr->setActivated(this->activated->getBool());

		return clonedJointCompPtr;
	}

	bool JointWormGearComponent::init(xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		JointComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "GearRatio")
		{
			this->gearRatio->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SlideRatio")
		{
			this->slideRatio->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	bool JointWormGearComponent::postInit(void)
	{
		JointComponent::postInit();

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointWormGearComponent] Init joint worm gear component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool JointWormGearComponent::connect(void)
	{
		bool success = JointComponent::connect();
		return success;
	}

	bool JointWormGearComponent::disconnect(void)
	{
		bool success = JointComponent::disconnect();
		return success;
	}

	Ogre::String JointWormGearComponent::getClassName(void) const
	{
		return "JointWormGearComponent";
	}

	Ogre::String JointWormGearComponent::getParentClassName(void) const
	{
		return "JointComponent";
	}

	void JointWormGearComponent::update(Ogre::Real dt, bool notSimulating)
	{

	}

	bool JointWormGearComponent::createJoint(const Ogre::Vector3& customJointPosition)
	{
		// Joint base created but not activated, return false, but its no error, hence after that return true.
		if (false == JointComponent::createJoint(customJointPosition))
		{
			return true;
		}

		if (nullptr == this->body)
		{
			this->connect();
		}
		if (nullptr == this->targetJointCompPtr)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointWormGearComponent] Error could not create worm gear for joint: " + this->gameObjectPtr->getName()
				+ ", because the target joint handler does not exist.");
			return false;
		}
		JointHingeCompPtr jointHingeCompPtr1 = boost::dynamic_pointer_cast<JointHingeComponent>(this->targetJointCompPtr);
		JointHingeActuatorCompPtr jointHingeActuatorCompPtr1;
		if (nullptr == jointHingeCompPtr1)
		{
			JointHingeActuatorCompPtr jointHingeActuatorCompPtr1 = boost::dynamic_pointer_cast<JointHingeActuatorComponent>(this->targetJointCompPtr);
			if (nullptr == jointHingeActuatorCompPtr1)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointWormGearComponent] Error could not create worm gear for joint: " + this->gameObjectPtr->getName()
					+ ", because the predecessor joint handler does not exist or is no hinge.");
				return false;
			}
		}

		JointPassiveSliderCompPtr jointPassiveSliderCompPtr2 = boost::dynamic_pointer_cast<JointPassiveSliderComponent>(this->predecessorJointCompPtr);
		JointActiveSliderCompPtr jointActiveSliderCompPtr2;
		JointSliderActuatorCompPtr jointSliderActuatorCompPtr2;
		if (nullptr == jointPassiveSliderCompPtr2)
		{
			jointActiveSliderCompPtr2 = boost::dynamic_pointer_cast<JointActiveSliderComponent>(this->predecessorJointCompPtr);
			if (nullptr == jointActiveSliderCompPtr2)
			{
				jointSliderActuatorCompPtr2 = boost::dynamic_pointer_cast<JointSliderActuatorComponent>(this->predecessorJointCompPtr);
				if (nullptr == jointSliderActuatorCompPtr2)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointWormGearComponent] Error could not create worm gear for joint: " + this->gameObjectPtr->getName()
						+ ", because the target joint handler does not exist or is no slider.");
					return false;
				}
			}
		}

		// Release joint each time, to create new one with new values
		this->internalReleaseJoint();
		// Worm gear: target hinge, predecessor slider
		if (jointHingeCompPtr1 && jointPassiveSliderCompPtr2)
		{
			this->joint = new OgreNewt::CustomGearAndSlide(jointHingeCompPtr1->getBody(), jointPassiveSliderCompPtr2->getBody(), 
				jointHingeCompPtr1->getBody()->getOrientation() * jointHingeCompPtr1->getPin(), jointPassiveSliderCompPtr2->getBody()->getOrientation() * jointPassiveSliderCompPtr2->getPin(),
			this->gearRatio->getReal(), this->slideRatio->getReal());
		}
		else if (jointHingeCompPtr1 && jointSliderActuatorCompPtr2)
		{
			this->joint = new OgreNewt::CustomGearAndSlide(jointHingeCompPtr1->getBody(), jointSliderActuatorCompPtr2->getBody(),
				jointHingeCompPtr1->getBody()->getOrientation() * jointHingeCompPtr1->getPin(), jointSliderActuatorCompPtr2->getBody()->getOrientation() * jointSliderActuatorCompPtr2->getPin(),
				this->gearRatio->getReal(), this->slideRatio->getReal());
		}
		else if (jointHingeActuatorCompPtr1 && jointPassiveSliderCompPtr2)
		{
			this->joint = new OgreNewt::CustomGearAndSlide(jointHingeActuatorCompPtr1->getBody(), jointPassiveSliderCompPtr2->getBody(),
				jointHingeActuatorCompPtr1->getBody()->getOrientation() * jointHingeActuatorCompPtr1->getPin(), jointPassiveSliderCompPtr2->getBody()->getOrientation() * jointPassiveSliderCompPtr2->getPin(),
				this->gearRatio->getReal(), this->slideRatio->getReal());
		}
		else
		{
			// Predecessor may also be an active slider
			this->joint = new OgreNewt::CustomGearAndSlide(jointHingeCompPtr1->getBody(), jointActiveSliderCompPtr2->getBody(), 
				jointHingeCompPtr1->getBody()->getOrientation() * jointHingeCompPtr1->getPin(), jointPassiveSliderCompPtr2->getBody()->getOrientation() * jointActiveSliderCompPtr2->getPin(),
			this->gearRatio->getReal(), this->slideRatio->getReal());
		}

		this->joint->setBodyMassScale(this->bodyMassScale->getVector2().x, this->bodyMassScale->getVector2().y);
		this->joint->setCollisionState(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		// this->body->setJointRecursiveCollision(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		return true;
	}
	
	void JointWormGearComponent::actualizeValue(Variant* attribute)
	{
		JointComponent::actualizeValue(attribute);

		if (JointWormGearComponent::AttrGearRatio() == attribute->getName())
		{
			this->setGearRatio(attribute->getReal());
		}
		else if (JointWormGearComponent::AttrSlideRatio() == attribute->getName())
		{
			this->setSlideRatio(attribute->getReal());
		}
	}
	
	void JointWormGearComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		JointComponent::writeXML(propertiesXML, doc, filePath);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "GearRatio"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->gearRatio->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SlideRatio"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->slideRatio->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	void JointWormGearComponent::setGearRatio(Ogre::Real gearRatio)
	{
		this->gearRatio->setValue(gearRatio);
	}

	Ogre::Real JointWormGearComponent::getGearRatio(void) const
	{
		return this->gearRatio->getReal();
	}
	
	void JointWormGearComponent::setSlideRatio(Ogre::Real slideRatio)
	{
		this->slideRatio->setValue(slideRatio);
	}

	Ogre::Real JointWormGearComponent::getSlideRatio(void) const
	{
		return this->slideRatio->getReal();
	}

	/*******************************JointPulleyComponent*******************************/

	JointPulleyComponent::JointPulleyComponent()
		: JointComponent()
	{
		this->type->setReadOnly(false);
		this->type->setValue(this->getClassName());
		this->type->setReadOnly(true);
		
		this->type->setDescription("Needs 2 kinds of slider components in this game object");
		
		this->pulleyRatio = new Variant(JointPulleyComponent::AttrPulleyRatio(), 1.0f, this->attributes);
	}

	JointPulleyComponent::~JointPulleyComponent()
	{

	}

	GameObjectCompPtr JointPulleyComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		JointPulleyCompPtr clonedJointCompPtr(boost::make_shared<JointPulleyComponent>());
		
		clonedJointCompPtr->setType(this->type->getString());
		clonedJointCompPtr->internalSetPriorId(this->id->getULong());
		clonedJointCompPtr->setPredecessorId(this->predecessorId->getULong());
		clonedJointCompPtr->setTargetId(this->targetId->getULong());
		clonedJointCompPtr->setJointPosition(this->jointPosition);
		clonedJointCompPtr->setJointRecursiveCollisionEnabled(this->jointRecursiveCollision->getBool());
		clonedJointCompPtr->stiffness->setValue(this->stiffness->getReal());

		clonedJointCompPtr->setPulleyRatio(this->pulleyRatio->getReal());

		clonedGameObjectPtr->addComponent(clonedJointCompPtr);
		clonedJointCompPtr->setOwner(clonedGameObjectPtr);

		clonedJointCompPtr->setActivated(this->activated->getBool());

		return clonedJointCompPtr;
	}

	bool JointPulleyComponent::init(xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		JointComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointPulleyRatio")
		{
			this->pulleyRatio->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	bool JointPulleyComponent::postInit(void)
	{
		JointComponent::postInit();

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointPulleyComponent] Init joint pulley component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool JointPulleyComponent::connect(void)
	{
		bool success = JointComponent::connect();
		return success;
	}

	bool JointPulleyComponent::disconnect(void)
	{
		bool success = JointComponent::disconnect();
		return success;
	}

	Ogre::String JointPulleyComponent::getClassName(void) const
	{
		return "JointPulleyComponent";
	}

	Ogre::String JointPulleyComponent::getParentClassName(void) const
	{
		return "JointComponent";
	}

	void JointPulleyComponent::update(Ogre::Real dt, bool notSimulating)
	{

	}

	bool JointPulleyComponent::createJoint(const Ogre::Vector3& customJointPosition)
	{
		// Joint base created but not activated, return false, but its no error, hence after that return true.
		if (false == JointComponent::createJoint(customJointPosition))
		{
			return true;
		}

		if (nullptr == this->body)
		{
			this->connect();
		}
		
		//if (true == this->jointAlreadyCreated)
		//{
		//	// joint already created so skip
		//	return true;
		//}
		
		if (nullptr == this->targetJointCompPtr)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointPulleyComponent] Error could not create pulley for joint: " + this->gameObjectPtr->getName()
				+ ", because the target joint handler does not exist.");
			return false;
		}
		JointPassiveSliderCompPtr jointPassiveSliderCompPtr1 = boost::dynamic_pointer_cast<JointPassiveSliderComponent>(this->targetJointCompPtr);
		JointActiveSliderCompPtr jointActiveSliderCompPtr1;
		JointSliderActuatorCompPtr jointSliderActuatorCompPtr1;
		if (nullptr == jointPassiveSliderCompPtr1)
		{
			jointActiveSliderCompPtr1 = boost::dynamic_pointer_cast<JointActiveSliderComponent>(this->targetJointCompPtr);
			if (nullptr == jointActiveSliderCompPtr1)
			{
				jointSliderActuatorCompPtr1 = boost::dynamic_pointer_cast<JointSliderActuatorComponent>(this->targetJointCompPtr);
				if (nullptr == jointSliderActuatorCompPtr1)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointPulleyComponent] Error could not create pulley for joint: " + this->gameObjectPtr->getName()
						+ ", because the target joint handler does not exist or is no slider.");
					return false;
				}
			}
		}
		JointPassiveSliderCompPtr jointPassiveSliderCompPtr2 = boost::dynamic_pointer_cast<JointPassiveSliderComponent>(this->predecessorJointCompPtr);
		JointActiveSliderCompPtr jointActiveSliderCompPtr2;
		JointSliderActuatorCompPtr jointSliderActuatorCompPtr2;
		if (nullptr == jointPassiveSliderCompPtr2)
		{
			jointActiveSliderCompPtr2 = boost::dynamic_pointer_cast<JointActiveSliderComponent>(this->predecessorJointCompPtr);
			if (nullptr == jointActiveSliderCompPtr2)
			{
				jointSliderActuatorCompPtr2 = boost::dynamic_pointer_cast<JointSliderActuatorComponent>(this->predecessorJointCompPtr);
				if (nullptr == jointSliderActuatorCompPtr2)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointPulleyComponent] Error could not create pulley for joint: " + this->gameObjectPtr->getName()
						+ ", because the predecessor joint handler does not exist or is no slider.");
					return false;
				}
			}
		}

		// Release joint each time, to create new one with new values
		this->internalReleaseJoint();
		// Combinations of active and passive sliders are possible
		if (jointPassiveSliderCompPtr1 && jointPassiveSliderCompPtr2)
		{
			this->joint = new OgreNewt::Pulley(jointPassiveSliderCompPtr1->getBody(), jointPassiveSliderCompPtr2->getBody(), 
				jointPassiveSliderCompPtr1->getBody()->getOrientation() * jointPassiveSliderCompPtr1->getPin(),
				jointPassiveSliderCompPtr2->getBody()->getOrientation() * jointPassiveSliderCompPtr2->getPin(), this->pulleyRatio->getReal());
		}
		else if (jointActiveSliderCompPtr1 && jointPassiveSliderCompPtr2)
		{
			this->joint = new OgreNewt::Pulley(jointActiveSliderCompPtr1->getBody(), jointPassiveSliderCompPtr2->getBody(), 
				jointActiveSliderCompPtr1->getBody()->getOrientation() * jointActiveSliderCompPtr1->getPin(),
				jointPassiveSliderCompPtr2->getBody()->getOrientation() * jointPassiveSliderCompPtr2->getPin(), this->pulleyRatio->getReal());
		}
		else if (jointPassiveSliderCompPtr1 && jointActiveSliderCompPtr2)
		{
			this->joint = new OgreNewt::Pulley(jointPassiveSliderCompPtr1->getBody(), jointActiveSliderCompPtr2->getBody(), 
				jointPassiveSliderCompPtr1->getBody()->getOrientation() * jointPassiveSliderCompPtr1->getPin(),
				jointActiveSliderCompPtr2->getBody()->getOrientation() * jointActiveSliderCompPtr2->getPin(), this->pulleyRatio->getReal());
		}
		else if (jointPassiveSliderCompPtr1 && jointSliderActuatorCompPtr2)
		{
			this->joint = new OgreNewt::Pulley(jointPassiveSliderCompPtr1->getBody(), jointSliderActuatorCompPtr2->getBody(),
				jointPassiveSliderCompPtr1->getBody()->getOrientation() * jointPassiveSliderCompPtr1->getPin(),
				jointSliderActuatorCompPtr2->getBody()->getOrientation() * jointSliderActuatorCompPtr2->getPin(), this->pulleyRatio->getReal());
		}
		else if (jointSliderActuatorCompPtr1 && jointPassiveSliderCompPtr2)
		{
			this->joint = new OgreNewt::Pulley(jointSliderActuatorCompPtr1->getBody(), jointPassiveSliderCompPtr2->getBody(),
				jointActiveSliderCompPtr1->getBody()->getOrientation() * jointSliderActuatorCompPtr1->getPin(),
				jointSliderActuatorCompPtr1->getBody()->getOrientation() * jointPassiveSliderCompPtr2->getPin(), this->pulleyRatio->getReal());
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointPulleyComponent] Error could not create pulley for joint: " + this->gameObjectPtr->getName()
				+ ", because both predecessor and target joint handler are two active slider! But only one of them can be one.");
			return false;
		}

		this->joint->setBodyMassScale(this->bodyMassScale->getVector2().x, this->bodyMassScale->getVector2().y);
		this->joint->setCollisionState(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		// this->body->setJointRecursiveCollision(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		return true;
	}
	
	void JointPulleyComponent::actualizeValue(Variant* attribute)
	{
		JointComponent::actualizeValue(attribute);

		if (JointPulleyComponent::AttrPulleyRatio() == attribute->getName())
		{
			this->setPulleyRatio(attribute->getReal());
		}
	}
	
	void JointPulleyComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		JointComponent::writeXML(propertiesXML, doc, filePath);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointPulleyRatio"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->pulleyRatio->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	void JointPulleyComponent::setPulleyRatio(Ogre::Real pulleyRatio)
	{
		this->pulleyRatio->setValue(pulleyRatio);
	}

	Ogre::Real JointPulleyComponent::getPulleyRatio(void) const
	{
		return this->pulleyRatio->getReal();
	}

	/*******************************JointUniversalComponent*******************************/

	JointUniversalComponent::JointUniversalComponent()
		: JointComponent()
	{
		this->type->setReadOnly(false);
		this->type->setValue(this->getClassName());
		this->type->setReadOnly(true);
		
		this->anchorPosition = new Variant(JointUniversalComponent::AttrAnchorPosition(), Ogre::Vector3::ZERO, this->attributes);
		this->pin = new Variant(JointUniversalComponent::AttrPin(), Ogre::Vector3(0.0f, 0.0f, 1.0f), this->attributes);

		this->enableLimits0 = new Variant(JointUniversalComponent::AttrLimits0(), false, this->attributes);
		this->minAngleLimit0 = new Variant(JointUniversalComponent::AttrMinAngleLimit0(), 0.0f, this->attributes);
		this->maxAngleLimit0 = new Variant(JointUniversalComponent::AttrMaxAngleLimit0(), 0.0f, this->attributes);
		
		this->enableLimits1 = new Variant(JointUniversalComponent::AttrLimits1(), false, this->attributes);
		this->minAngleLimit1 = new Variant(JointUniversalComponent::AttrMinAngleLimit1(), 0.0f, this->attributes);
		this->maxAngleLimit1 = new Variant(JointUniversalComponent::AttrMaxAngleLimit1(), 0.0f, this->attributes);
		
		this->enableMotor = new Variant(JointUniversalComponent::AttrEnableMotor(), false, this->attributes);
		this->motorSpeed = new Variant(JointUniversalComponent::AttrMotorSpeed(), 0.0f, this->attributes);

		this->asSpringDamper0 = new Variant(JointUniversalComponent::AttrAsSpringDamper0(), false, this->attributes);
		this->springK0 = new Variant(JointUniversalComponent::AttrSpringK0(), 256.0f, this->attributes);
		this->springD0 = new Variant(JointUniversalComponent::AttrSpringD0(), 32.0f, this->attributes);
		this->springDamperRelaxation0 = new Variant(JointUniversalComponent::AttrSpringDamperRelaxation0(), 0.4f, this->attributes);
		this->springDamperRelaxation0->setDescription("[0 - 0.99]");

		this->asSpringDamper1 = new Variant(JointUniversalComponent::AttrAsSpringDamper1(), false, this->attributes);
		this->springK1 = new Variant(JointUniversalComponent::AttrSpringK1(), 256.0f, this->attributes);
		this->springD1 = new Variant(JointUniversalComponent::AttrSpringD1(), 32.0f, this->attributes);
		this->springDamperRelaxation1 = new Variant(JointUniversalComponent::AttrSpringDamperRelaxation1(), 0.4f, this->attributes);
		this->springDamperRelaxation1->setDescription("[0 - 0.99]");

		this->friction0 = new Variant(JointUniversalComponent::AttrFriction0(), -1.0f, this->attributes);
		this->friction1 = new Variant(JointUniversalComponent::AttrFriction1(), -1.0f, this->attributes);

		Ogre::String torqueFrictionDescription = "Sets the friction (max. torque) for torque. "
			"That is, without a friction the torque will not work. So get a balance between torque and friction (1, 20) e.g. The more friction, the more powerful the motor will become.";
		this->friction0->setDescription(torqueFrictionDescription);
		this->friction1->setDescription(torqueFrictionDescription);
		this->motorSpeed->setDescription(torqueFrictionDescription);

		this->targetId->setVisible(false);
	}

	JointUniversalComponent::~JointUniversalComponent()
	{

	}

	GameObjectCompPtr JointUniversalComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		JointUniversalCompPtr clonedJointCompPtr(boost::make_shared<JointUniversalComponent>());
		
		clonedJointCompPtr->setType(this->type->getString());
		clonedJointCompPtr->internalSetPriorId(this->id->getULong());
		clonedJointCompPtr->setPredecessorId(this->predecessorId->getULong());
		clonedJointCompPtr->setTargetId(this->targetId->getULong());
		clonedJointCompPtr->setJointPosition(this->jointPosition);
		clonedJointCompPtr->setJointRecursiveCollisionEnabled(this->jointRecursiveCollision->getBool());
		clonedJointCompPtr->stiffness->setValue(this->stiffness->getReal());

		clonedJointCompPtr->setAnchorPosition(this->anchorPosition->getVector3());
		clonedJointCompPtr->setPin(this->pin->getVector3());
		clonedJointCompPtr->setLimits0Enabled(this->enableLimits0->getBool());
		clonedJointCompPtr->setMinMaxAngleLimit0(Ogre::Degree(this->minAngleLimit0->getReal()), Ogre::Degree(this->maxAngleLimit0->getReal()));
		clonedJointCompPtr->setLimits1Enabled(this->enableLimits1->getBool());
		clonedJointCompPtr->setMinMaxAngleLimit1(Ogre::Degree(this->minAngleLimit1->getReal()), Ogre::Degree(this->maxAngleLimit1->getReal()));

		clonedJointCompPtr->setMotorEnabled(this->enableMotor->getBool());
		clonedJointCompPtr->setMotorSpeed(this->motorSpeed->getReal());
		
		clonedJointCompPtr->setSpring0(this->asSpringDamper0->getBool(), this->springDamperRelaxation0->getReal(), this->springK0->getReal(), this->springD0->getReal());
		clonedJointCompPtr->setSpring1(this->asSpringDamper1->getBool(), this->springDamperRelaxation1->getReal(), this->springK1->getReal(), this->springD1->getReal());
		
		clonedJointCompPtr->setFriction0(this->friction0->getReal());
		clonedJointCompPtr->setFriction1(this->friction1->getReal());

		clonedGameObjectPtr->addComponent(clonedJointCompPtr);
		clonedJointCompPtr->setOwner(clonedGameObjectPtr);

		clonedJointCompPtr->setActivated(this->activated->getBool());

		return clonedJointCompPtr;
	}

	bool JointUniversalComponent::init(xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		JointComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnchorPosition")
		{
			this->anchorPosition->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(0.0f, 0.0f, 0.0f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Pin")
		{
			this->pin->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(0.0f, 0.0f, 1.0f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Limits0")
		{
			this->enableLimits0->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MinAngleLimit0")
		{
			this->minAngleLimit0->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxAngleLimit0")
		{
			this->maxAngleLimit0->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Limits1")
		{
			this->enableLimits1->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MinAngleLimit1")
		{
			this->minAngleLimit1->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxAngleLimit1")
		{
			this->maxAngleLimit1->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "EnableMotor")
		{
			this->enableMotor->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MotorSpeed")
		{
			this->motorSpeed->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointAsSpringDamper0")
		{
			this->asSpringDamper0->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointKSpring0")
		{
			this->springK0->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointDSpring0")
		{
			this->springD0->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointSpringDamperRelaxation0")
		{
			this->springDamperRelaxation0->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointAsSpringDamper1")
		{
			this->asSpringDamper1->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointKSpring1")
		{
			this->springK1->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointDSpring1")
		{
			this->springD1->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointSpringDamperRelaxation1")
		{
			this->springDamperRelaxation1->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Friction0")
		{
			this->friction0->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Friction1")
		{
			this->friction1->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	bool JointUniversalComponent::postInit(void)
	{
		JointComponent::postInit();

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointUniversalComponent] Init joint universal component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool JointUniversalComponent::connect(void)
	{
		bool success = JointComponent::connect();
		return success;
	}

	bool JointUniversalComponent::disconnect(void)
	{
		bool success = JointComponent::disconnect();
		return success;
	}

	Ogre::String JointUniversalComponent::getClassName(void) const
	{
		return "JointUniversalComponent";
	}

	Ogre::String JointUniversalComponent::getParentClassName(void) const
	{
		return "JointComponent";
	}

	void JointUniversalComponent::update(Ogre::Real dt, bool notSimulating)
	{

	}

	bool JointUniversalComponent::createJoint(const Ogre::Vector3& customJointPosition)
	{
		// Joint base created but not activated, return false, but its no error, hence after that return true.
		if (false == JointComponent::createJoint(customJointPosition))
		{
			return true;
		}

		if (true == this->jointAlreadyCreated)
		{
			// joint already created so skip
			return true;
		}
		if (nullptr == this->body)
		{
			this->connect();
		}

		if (Ogre::Vector3::ZERO == customJointPosition)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			//Ogre::Vector3 aPos = Ogre::Vector3(-this->anchorPosition.x, -this->anchorPosition.y, -this->anchorPosition.z);
			//// relative anchor pos: 0.5 0 0 means 50% in X of the size of the parent object
			//Ogre::Vector3 offset = (aPos * size);
			//// take orientation into account, so that orientated joints are processed correctly
			//this->jointPosition = this->body->getPosition() - (this->body->getOrientation() * offset);

			// without rotation
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			this->jointPosition = (this->body->getPosition() + offset);
		}
		else
		{
			this->hasCustomJointPosition = true;
			this->jointPosition = customJointPosition;
		}
		OgreNewt::Body* predecessorBody = nullptr;
		
		if (this->predecessorJointCompPtr)
		{
			predecessorBody = this->predecessorJointCompPtr->getBody();
		}

		// Release joint each time, to create new one with new values
		this->internalReleaseJoint();

		Ogre::Vector3 pin = Ogre::Vector3::ZERO;
		if (Ogre::Vector3::ZERO != this->pin->getVector3())
		{
			pin = this->body->getOrientation() * this->pin->getVector3();
		}

		this->joint = new OgreNewt::Universal(this->body, predecessorBody, this->jointPosition, pin);

		OgreNewt::Universal* universalJoint = dynamic_cast<OgreNewt::Universal*>(this->joint);

		this->joint->setBodyMassScale(this->bodyMassScale->getVector2().x, this->bodyMassScale->getVector2().y);
		this->joint->setCollisionState(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		// this->body->setJointRecursiveCollision(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		// this->applyStiffness();

		universalJoint->enableLimits0(this->enableLimits0->getBool());

		if (true == this->enableLimits0->getBool())
			universalJoint->setLimits0(Ogre::Degree(this->minAngleLimit0->getReal()), Ogre::Degree(this->maxAngleLimit0->getReal()));

		universalJoint->enableLimits1(this->enableLimits1->getBool());

		if (true == this->enableLimits1->getBool())
			universalJoint->setLimits1(Ogre::Degree(this->minAngleLimit1->getReal()), Ogre::Degree(this->maxAngleLimit1->getReal()));

		universalJoint->enableMotor0(this->enableMotor->getBool(), this->motorSpeed->getReal());

		universalJoint->setAsSpringDamper0(this->asSpringDamper0->getBool(), this->springDamperRelaxation0->getReal(), this->springK0->getReal(), this->springD0->getReal());
		universalJoint->setAsSpringDamper1(this->asSpringDamper1->getBool(), this->springDamperRelaxation1->getReal(), this->springK1->getReal(), this->springD1->getReal());

		if (-1.0f != this->friction0->getReal())
			universalJoint->setFriction0(this->friction0->getReal());

		if (-1.0f != this->friction1->getReal())
			universalJoint->setFriction1(this->friction1->getReal());

		this->jointAlreadyCreated = true;
		
		return true;
	}
	
	void JointUniversalComponent::actualizeValue(Variant* attribute)
	{
		JointComponent::actualizeValue(attribute);

		if (JointUniversalComponent::AttrAnchorPosition() == attribute->getName())
		{
			this->setAnchorPosition(attribute->getVector3());
		}
		else if (JointUniversalComponent::AttrPin() == attribute->getName())
		{
			this->setPin(attribute->getVector3());
		}
		else if (JointUniversalComponent::AttrLimits0() == attribute->getName())
		{
			this->setLimits0Enabled(attribute->getBool());
		}
		else if (JointUniversalComponent::AttrMinAngleLimit0() == attribute->getName())
		{
			this->setMinMaxAngleLimit0(Ogre::Degree(attribute->getReal()), Ogre::Degree(this->maxAngleLimit0->getReal()));
		}
		else if (JointUniversalComponent::AttrMaxAngleLimit0() == attribute->getName())
		{
			this->setMinMaxAngleLimit0(Ogre::Degree(this->minAngleLimit0->getReal()), Ogre::Degree(attribute->getReal()));
		}
		else if (JointUniversalComponent::AttrLimits1() == attribute->getName())
		{
			this->setLimits1Enabled(attribute->getBool());
		}
		else if (JointUniversalComponent::AttrMinAngleLimit1() == attribute->getName())
		{
			this->setMinMaxAngleLimit1(Ogre::Degree(attribute->getReal()), Ogre::Degree(this->maxAngleLimit1->getReal()));
		}
		else if (JointUniversalComponent::AttrMaxAngleLimit1() == attribute->getName())
		{
			this->setMinMaxAngleLimit1(Ogre::Degree(this->minAngleLimit1->getReal()), Ogre::Degree(attribute->getReal()));
		}
		else if (JointUniversalComponent::AttrEnableMotor() == attribute->getName())
		{
			this->setMotorEnabled(attribute->getBool());
		}
		else if (JointUniversalComponent::AttrMotorSpeed() == attribute->getName())
		{
			this->setMotorSpeed(attribute->getReal());
		}
		else if (JointUniversalComponent::AttrAsSpringDamper0() == attribute->getName())
		{
			this->setSpring0(attribute->getBool(), this->springDamperRelaxation0->getReal(), this->springK0->getReal(), this->springD0->getReal());
		}
		else if (JointUniversalComponent::AttrSpringDamperRelaxation0() == attribute->getName())
		{
			this->setSpring0(this->asSpringDamper0->getBool(), attribute->getReal(), this->springK0->getReal(), this->springD0->getReal());
		}
		else if (JointUniversalComponent::AttrSpringK0() == attribute->getName())
		{
			this->setSpring0(this->asSpringDamper0->getBool(), this->springDamperRelaxation0->getReal(), attribute->getReal(), this->springD0->getReal());
		}
		else if (JointUniversalComponent::AttrSpringD0() == attribute->getName())
		{
			this->setSpring0(this->asSpringDamper0->getBool(), this->springDamperRelaxation0->getReal(), this->springK0->getReal(), attribute->getReal());
		}
		else if (JointUniversalComponent::AttrAsSpringDamper1() == attribute->getName())
		{
			this->setSpring1(attribute->getBool(), this->springDamperRelaxation1->getReal(), this->springK1->getReal(), this->springD1->getReal());
		}
		else if (JointUniversalComponent::AttrSpringDamperRelaxation1() == attribute->getName())
		{
			this->setSpring1(this->asSpringDamper1->getBool(), attribute->getReal(), this->springK1->getReal(), this->springD1->getReal());
		}
		else if (JointUniversalComponent::AttrSpringK1() == attribute->getName())
		{
			this->setSpring1(this->asSpringDamper1->getBool(), this->springDamperRelaxation1->getReal(), attribute->getReal(), this->springD1->getReal());
		}
		else if (JointUniversalComponent::AttrSpringD1() == attribute->getName())
		{
			this->setSpring1(this->asSpringDamper1->getBool(), this->springDamperRelaxation1->getReal(), this->springK1->getReal(), attribute->getReal());
		}
		else if (JointUniversalComponent::AttrFriction0() == attribute->getName())
		{
			this->setFriction0(attribute->getReal());
		}
		else if (JointUniversalComponent::AttrFriction1() == attribute->getName())
		{
			this->setFriction1(attribute->getReal());
		}
	}
	
	void JointUniversalComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		JointComponent::writeXML(propertiesXML, doc, filePath);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnchorPosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->anchorPosition->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Pin"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->pin->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Limits0"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->enableLimits0->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MinAngleLimit0"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->minAngleLimit0->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MaxAngleLimit0"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxAngleLimit0->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Limits1"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->enableLimits1->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MinAngleLimit1"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->minAngleLimit1->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MaxAngleLimit1"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxAngleLimit1->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "EnableMotor"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->enableMotor->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MotorSpeed"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->motorSpeed->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointAsSpringDamper0"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->asSpringDamper0->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointKSpring0"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->springK0->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointDSpring0"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->springD0->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointSpringDamperRelaxation0"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->springDamperRelaxation0->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointAsSpringDamper1"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->asSpringDamper1->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointKSpring1"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->springK1->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointDSpring1"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->springD1->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointSpringDamperRelaxation1"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->springDamperRelaxation1->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Friction0"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->friction0->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Friction1"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->friction1->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::Vector3 JointUniversalComponent::getUpdatedJointPosition(void)
	{
		if (nullptr == this->body)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointUniversalComponent] Could not get 'getUpdatedJointPosition', because there is no body for game object: " + this->gameObjectPtr->getName());
			return Ogre::Vector3::ZERO;
		}

		if (false == this->hasCustomJointPosition)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			return this->body->getPosition() + offset;
		}
		else
		{
			return this->jointPosition;
		}
	}

	void JointUniversalComponent::setAnchorPosition(const Ogre::Vector3& anchorPosition)
	{
		this->anchorPosition->setValue(anchorPosition);

		if (nullptr != this->body)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			//Ogre::Vector3 aPos = Ogre::Vector3(-this->anchorPosition.x, -this->anchorPosition.y, -this->anchorPosition.z);
			//// relative anchor pos: 0.5 0 0 means 50% in X of the size of the parent object
			//Ogre::Vector3 offset = (aPos * size);
			//// take orientation into account, so that orientated joints are processed correctly
			//this->jointPosition = this->body->getPosition() - (this->body->getOrientation() * offset);

			// without rotation
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			this->jointPosition = (this->body->getPosition() + offset);
			this->jointAlreadyCreated = false;
		}
	}

	Ogre::Vector3 JointUniversalComponent::getAnchorPosition(void) const
	{
		return this->anchorPosition->getVector3();
	}

	void JointUniversalComponent::setPin(const Ogre::Vector3& pin)
	{
		this->pin->setValue(pin);
		if (nullptr != this->debugGeometryNode)
		{
			this->debugGeometryNode->setDirection(pin);
		}
	}

	Ogre::Vector3 JointUniversalComponent::getPin(void) const
	{
		return this->pin->getVector3();
	}

	void JointUniversalComponent::setLimits0Enabled(bool enableLimits0)
	{
		this->enableLimits0->setValue(enableLimits0);
	}

	bool JointUniversalComponent::getLimits0Enabled(void) const
	{
		return this->enableLimits0->getBool();
	}

	void JointUniversalComponent::setMinMaxAngleLimit0(const Ogre::Degree& minAngleLimit0, const Ogre::Degree& maxAngleLimit0)
	{
		this->minAngleLimit0->setValue(minAngleLimit0.valueDegrees());
		this->maxAngleLimit0->setValue(maxAngleLimit0.valueDegrees());
		
		OgreNewt::Universal* universalJoint = dynamic_cast<OgreNewt::Universal*>(this->joint);

		if (universalJoint && this->enableLimits0->getBool())
		{
			universalJoint->setLimits0(minAngleLimit0, maxAngleLimit0);
		}
	}
	
	Ogre::Degree JointUniversalComponent::getMinAngleLimit0(void) const
	{
		return Ogre::Degree(this->minAngleLimit0->getReal());
	}
		
	Ogre::Degree JointUniversalComponent::getMaxAngleLimit0(void) const
	{
		return Ogre::Degree(this->maxAngleLimit0->getReal());
	}
		
	void JointUniversalComponent::setLimits1Enabled(bool enableLimits1)
	{
		this->enableLimits1->setValue(enableLimits1);
	}

	bool JointUniversalComponent::getLimits1Enabled(void) const
	{
		return this->enableLimits1->getBool();
	}

	void JointUniversalComponent::setMinMaxAngleLimit1(const Ogre::Degree& minAngleLimit1, const Ogre::Degree& maxAngleLimit1)
	{
		this->minAngleLimit1->setValue(minAngleLimit1.valueDegrees());
		this->maxAngleLimit1->setValue(maxAngleLimit1.valueDegrees());
		
		OgreNewt::Universal* universalJoint = dynamic_cast<OgreNewt::Universal*>(this->joint);

		if (universalJoint && this->enableLimits1->getBool())
		{
			universalJoint->setLimits1(minAngleLimit1, maxAngleLimit1);
		}
	}
	
	Ogre::Degree JointUniversalComponent::getMinAngleLimit1(void) const
	{
		return Ogre::Degree(this->minAngleLimit0->getReal());
	}
		
	Ogre::Degree JointUniversalComponent::getMaxAngleLimit1(void) const
	{
		return Ogre::Degree(this->maxAngleLimit0->getReal());
	}

	void JointUniversalComponent::setMotorEnabled(bool enableMotor0)
	{
		this->enableMotor->setValue(enableMotor);
	}

	bool JointUniversalComponent::getMotorEnabled(void) const
	{
		return this->enableMotor->getBool();
	}

	void JointUniversalComponent::setMotorSpeed(Ogre::Real motorSpeed)
	{
		this->motorSpeed->setValue(motorSpeed);
		OgreNewt::Universal* universalJoint = dynamic_cast<OgreNewt::Universal*>(this->joint);

		if (universalJoint && this->enableMotor->getBool())
		{
			universalJoint->enableMotor0(this->enableMotor->getBool(), motorSpeed);
		}
	}

	Ogre::Real JointUniversalComponent::getMotorSpeed(void) const
	{
		return this->motorSpeed->getReal();
	}

	void JointUniversalComponent::setSpring0(bool asSpringDamper)
	{
		this->asSpringDamper0->setValue(asSpringDamper);
		OgreNewt::Universal* universalJoint = dynamic_cast<OgreNewt::Universal*>(this->joint);
		if (nullptr != universalJoint)
		{
			universalJoint->setAsSpringDamper0(this->asSpringDamper0->getBool(), this->springDamperRelaxation0->getReal(), this->springK0->getReal(), this->springD0->getReal());
		}
	}

	void JointUniversalComponent::setSpring0(bool asSpringDamper, Ogre::Real springDamperRelaxation, Ogre::Real springK, Ogre::Real springD)
	{
		this->asSpringDamper0->setValue(asSpringDamper);
		this->springDamperRelaxation0->setValue(springDamperRelaxation);
		this->springK0->setValue(springK);
		this->springD0->setValue(springD);
		OgreNewt::Universal* universalJoint = dynamic_cast<OgreNewt::Universal*>(this->joint);
		if (nullptr != universalJoint)
		{
			universalJoint->setAsSpringDamper0(this->asSpringDamper0->getBool(), this->springDamperRelaxation0->getReal(), this->springK0->getReal(), this->springD0->getReal());
		}
	}

	std::tuple<bool, Ogre::Real, Ogre::Real, Ogre::Real> JointUniversalComponent::getSpring0(void) const
	{
		return std::make_tuple(this->asSpringDamper0->getBool(), this->springDamperRelaxation0->getReal(), this->springK0->getReal(), this->springD0->getReal());
	}

	void JointUniversalComponent::setSpring1(bool asSpringDamper)
	{
		this->asSpringDamper0->setValue(asSpringDamper);
		OgreNewt::Universal* universalJoint = dynamic_cast<OgreNewt::Universal*>(this->joint);
		if (nullptr != universalJoint)
		{
			universalJoint->setAsSpringDamper0(this->asSpringDamper0->getBool(), this->springDamperRelaxation0->getReal(), this->springK0->getReal(), this->springD0->getReal());
		}
	}

	void JointUniversalComponent::setSpring1(bool asSpringDamper, Ogre::Real springDamperRelaxation, Ogre::Real springK, Ogre::Real springD)
	{
		this->asSpringDamper1->setValue(asSpringDamper);
		this->springDamperRelaxation1->setValue(springDamperRelaxation);
		this->springK1->setValue(springK);
		this->springD1->setValue(springD);
		OgreNewt::Universal* universalJoint = dynamic_cast<OgreNewt::Universal*>(this->joint);
		if (nullptr != universalJoint)
		{
			universalJoint->setAsSpringDamper1(this->asSpringDamper1->getBool(), this->springDamperRelaxation1->getReal(), this->springK1->getReal(), this->springD1->getReal());
		}
	}

	std::tuple<bool, Ogre::Real, Ogre::Real, Ogre::Real> JointUniversalComponent::getSpring1(void) const
	{
		return std::make_tuple(this->asSpringDamper1->getBool(), this->springDamperRelaxation1->getReal(), this->springK1->getReal(), this->springD1->getReal());
	}

	void JointUniversalComponent::setFriction0(Ogre::Real frictionTorque)
	{
		this->friction0->setValue(frictionTorque);
	}

	Ogre::Real JointUniversalComponent::getFriction0(void) const
	{
		return this->friction0->getReal();
	}

	void JointUniversalComponent::setFriction1(Ogre::Real frictionTorque)
	{
		this->friction1->setValue(frictionTorque);
	}

	Ogre::Real JointUniversalComponent::getFriction1(void) const
	{
		return this->friction1->getReal();
	}
	
	/*******************************JointUniversalActuatorComponent*******************************/

	JointUniversalActuatorComponent::JointUniversalActuatorComponent()
		: JointComponent(),
		round0(0),
		round1(0),
		internalDirectionChange0(false),
		internalDirectionChange1(false),
		oppositeDir0(1.0f),
		oppositeDir1(1.0f)
	{
		// Note that also in JointComponent internalBaseInit() is called, which sets the values for JointComponent, so this is called for already existing values
		// But luckely in Variant, no new attributes are added, but the values changed via interalAdd(...) in Variant
		this->type->setReadOnly(false);
		this->type->setValue(this->getClassName());
		this->type->setReadOnly(true);
		this->type->setDescription("An object attached to a universal actuator joint can only rotate around two dimension perpendicular to the two axis it is attached to. A good a precise actuator motor rotating about 2 axis.");
		
		this->anchorPosition = new Variant(JointUniversalActuatorComponent::AttrAnchorPosition(), Ogre::Vector3::ZERO, this->attributes);
		
		this->targetAngle0  = new Variant(JointUniversalActuatorComponent::AttrTargetAngle0(), 30.0f, this->attributes);
		
		this->angleRate0  = new Variant(JointUniversalActuatorComponent::AttrAngleRate0(), 10.0f, this->attributes);
		this->maxTorque0 = new Variant(JointUniversalActuatorComponent::AttrMaxTorque0(), -1.0f, this->attributes); // disabled by default, so that max is used in newton
		
		this->minAngleLimit0 = new Variant(JointUniversalActuatorComponent::AttrMinAngleLimit0(), -360.0f, this->attributes);
		this->maxAngleLimit0 = new Variant(JointUniversalActuatorComponent::AttrMaxAngleLimit0(), 360.0f, this->attributes);
		
		this->directionChange0 = new Variant(JointUniversalActuatorComponent::AttrDirectionChange0(), false, this->attributes);
		this->repeat0 = new Variant(JointUniversalActuatorComponent::AttrRepeat0(), false, this->attributes);
		
		this->targetAngle1  = new Variant(JointUniversalActuatorComponent::AttrTargetAngle0(), 30.0f, this->attributes);
		
		this->angleRate1  = new Variant(JointUniversalActuatorComponent::AttrAngleRate0(), 10.0f, this->attributes);
		this->maxTorque1 = new Variant(JointUniversalActuatorComponent::AttrMaxTorque0(), -1.0f, this->attributes); // disabled by default, so that max is used in newton
		
		this->minAngleLimit1 = new Variant(JointUniversalActuatorComponent::AttrMinAngleLimit1(), -360.0f, this->attributes);
		this->maxAngleLimit1 = new Variant(JointUniversalActuatorComponent::AttrMaxAngleLimit1(), 360.0f, this->attributes);
		
		this->directionChange1 = new Variant(JointUniversalActuatorComponent::AttrDirectionChange1(), false, this->attributes);
		this->repeat1 = new Variant(JointUniversalActuatorComponent::AttrRepeat1(), false, this->attributes);
		
		this->internalDirectionChange0 = this->directionChange0->getBool();
		this->internalDirectionChange1 = this->directionChange1->getBool();

		this->targetId->setVisible(false);
	}

	JointUniversalActuatorComponent::~JointUniversalActuatorComponent()
	{
		
	}

	GameObjectCompPtr JointUniversalActuatorComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		JointUniversalActuatorCompPtr clonedJointCompPtr(boost::make_shared<JointUniversalActuatorComponent>());
		
		clonedJointCompPtr->setType(this->type->getString());
		clonedJointCompPtr->internalSetPriorId(this->id->getULong());
		clonedJointCompPtr->setPredecessorId(this->predecessorId->getULong());
		clonedJointCompPtr->setTargetId(this->targetId->getULong());
		clonedJointCompPtr->setJointPosition(this->jointPosition);
		clonedJointCompPtr->setJointRecursiveCollisionEnabled(this->jointRecursiveCollision->getBool());
		// clonedJointCompPtr->stiffness->setValue(this->stiffness->getReal()); // Do not call setter, because internally a bool flag is set, so that stiffness is always used

		clonedJointCompPtr->setAnchorPosition(this->anchorPosition->getVector3());
		clonedJointCompPtr->setTargetAngle0(Ogre::Degree(this->targetAngle0->getReal()));
		clonedJointCompPtr->setAngleRate0(Ogre::Degree(this->angleRate0->getReal()));
		clonedJointCompPtr->setMinAngleLimit0(Ogre::Degree(this->minAngleLimit0->getReal()));
		clonedJointCompPtr->setMaxAngleLimit0(Ogre::Degree(this->maxAngleLimit0->getReal()));
		clonedJointCompPtr->setMaxTorque0(this->maxTorque0->getReal());
		clonedJointCompPtr->setDirectionChange0(this->directionChange0->getBool());
		clonedJointCompPtr->setRepeat0(this->repeat0->getBool());
		
		clonedJointCompPtr->setTargetAngle1(Ogre::Degree(this->targetAngle1->getReal()));
		clonedJointCompPtr->setAngleRate1(Ogre::Degree(this->angleRate1->getReal()));
		clonedJointCompPtr->setMinAngleLimit1(Ogre::Degree(this->minAngleLimit1->getReal()));
		clonedJointCompPtr->setMaxAngleLimit1(Ogre::Degree(this->maxAngleLimit1->getReal()));
		clonedJointCompPtr->setMaxTorque1(this->maxTorque1->getReal());
		clonedJointCompPtr->setDirectionChange1(this->directionChange1->getBool());
		clonedJointCompPtr->setRepeat1(this->repeat1->getBool());

		clonedGameObjectPtr->addComponent(clonedJointCompPtr);
		clonedJointCompPtr->setOwner(clonedGameObjectPtr);

		clonedJointCompPtr->setActivated(this->activated->getBool());

		return clonedJointCompPtr;
	}

	bool JointUniversalActuatorComponent::postInit(void)
	{
		JointComponent::postInit();

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointUniversalActuatorComponent] Init joint universal actuator component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool JointUniversalActuatorComponent::connect(void)
	{
		bool success = JointComponent::connect();
		this->round0 = 0;
		this->round1 = 0;
		this->internalDirectionChange0 = this->directionChange0->getBool();
		this->internalDirectionChange1 = this->directionChange1->getBool();
		this->oppositeDir0 = 1.0f;
		this->oppositeDir1 = 1.0f;

		// Set new target each time when simulation is started
		if (nullptr != this->joint && true == this->activated->getBool())
		{
			OgreNewt::UniversalActuator* universalActuatorJoint = static_cast<OgreNewt::UniversalActuator*>(this->joint);
			if (nullptr != universalActuatorJoint)
			{
				universalActuatorJoint->SetTargetAngle0(Ogre::Degree(this->maxAngleLimit0->getReal()));
				universalActuatorJoint->SetTargetAngle1(Ogre::Degree(this->maxAngleLimit1->getReal()));
			}
		}

		return success;
	}

	bool JointUniversalActuatorComponent::disconnect(void)
	{
		bool success = JointComponent::disconnect();
		this->round0 = 0;
		this->round1 = 0;
		this->internalDirectionChange0 = this->directionChange0->getBool();
		this->internalDirectionChange1 = this->directionChange1->getBool();
		this->oppositeDir0 = 1.0f;
		this->oppositeDir1 = 1.0f;

		return success;
	}

	Ogre::String JointUniversalActuatorComponent::getClassName(void) const
	{
		return "JointUniversalActuatorComponent";
	}

	Ogre::String JointUniversalActuatorComponent::getParentClassName(void) const
	{
		return "JointComponent";
	}

	void JointUniversalActuatorComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating && true == this->activated->getBool())
		{
			if (true == this->internalDirectionChange0)
			{
				OgreNewt::UniversalActuator* universalActuatorJoint = static_cast<OgreNewt::UniversalActuator*>(this->joint);
				if (nullptr != universalActuatorJoint)
				{
					Ogre::Real angle = universalActuatorJoint->GetActuatorAngle0();
					Ogre::Real step = this->angleRate0->getReal() * dt;
					if (1.0f == this->oppositeDir0)
					{
						if (Ogre::Math::RealEqual(angle, this->maxAngleLimit0->getReal(), step))
						{
							this->oppositeDir0 *= -1.0f;
							this->round0++;
						}
					}
					else
					{
						if (Ogre::Math::RealEqual(angle, this->minAngleLimit0->getReal(), step))
						{
							this->oppositeDir0 *= -1.0f;
							this->round0++;
						}
					}
				}

				// If the hinge took 2 rounds (1x forward and 1x back, then its enough, if repeat is off)
				if (this->round0 == 2)
				{
					this->round0 = 0;
					// if repeat is of, only change the direction one time, to get back to its origin and leave
					if (false == this->repeat0->getBool())
					{
						this->internalDirectionChange0 = false;
					}
				}

				if (true == this->repeat0->getBool() || true == this->internalDirectionChange0)
				{
					Ogre::Degree newAngle = Ogre::Degree(0.0f);

					if (1.0f == this->oppositeDir0)
						newAngle = Ogre::Degree(this->maxAngleLimit0->getReal());
					else
						newAngle = Ogre::Degree(this->minAngleLimit0->getReal());
					universalActuatorJoint->SetTargetAngle0(newAngle);
				}
			}

			if (true == this->internalDirectionChange1)
			{
				OgreNewt::UniversalActuator* universalActuatorJoint = static_cast<OgreNewt::UniversalActuator*>(this->joint);
				if (nullptr != universalActuatorJoint)
				{
					Ogre::Real angle = universalActuatorJoint->GetActuatorAngle1();
					Ogre::Real step = this->angleRate1->getReal() * dt;
					if (1.0f == this->oppositeDir1)
					{
						if (Ogre::Math::RealEqual(angle, this->maxAngleLimit1->getReal(), step))
						{
							this->oppositeDir1 *= -1.0f;
							this->round1++;
						}
					}
					else
					{
						if (Ogre::Math::RealEqual(angle, this->minAngleLimit1->getReal(), step))
						{
							this->oppositeDir1 *= -1.0f;
							this->round1++;
						}
					}
				}

				// If the hinge took 2 rounds (1x forward and 1x back, then its enough, if repeat is off)
				if (this->round1 == 2)
				{
					this->round1 = 0;
					// if repeat is of, only change the direction one time, to get back to its origin and leave
					if (false == this->repeat1->getBool())
					{
						this->internalDirectionChange1 = false;
					}
				}

				if (true == this->repeat1->getBool() || true == this->internalDirectionChange1)
				{
					Ogre::Degree newAngle = Ogre::Degree(0.0f);

					if (1.0f == this->oppositeDir1)
						newAngle = Ogre::Degree(this->maxAngleLimit1->getReal());
					else
						newAngle = Ogre::Degree(this->minAngleLimit1->getReal());
					universalActuatorJoint->SetTargetAngle1(newAngle);
				}
			}
		}
	}

	bool JointUniversalActuatorComponent::init(xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		JointComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnchorPosition")
		{
			this->anchorPosition->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetAngle0")
		{
			this->targetAngle0->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AngleRate0")
		{
			this->angleRate0->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MinAngleLimit0")
		{
			this->minAngleLimit0->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxAngleLimit0")
		{
			this->maxAngleLimit0->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxTorgue0")
		{
			this->maxTorque0->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DirectionChange0")
		{
			this->directionChange0->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Repeat0")
		{
			this->repeat0->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetAngle1")
		{
			this->targetAngle1->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AngleRate1")
		{
			this->angleRate1->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MinAngleLimit1")
		{
			this->minAngleLimit1->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxAngleLimit1")
		{
			this->maxAngleLimit1->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxTorgue1")
		{
			this->maxTorque1->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "DirectionChange1")
		{
			this->directionChange1->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Repeat1")
		{
			this->repeat1->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	void JointUniversalActuatorComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		JointComponent::writeXML(propertiesXML, doc, filePath);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnchorPosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->anchorPosition->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TargetAngle0"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->targetAngle0->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AngleRate0"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->angleRate0->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MinAngleLimit0"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->minAngleLimit0->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MaxAngleLimit0"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxAngleLimit0->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MaxTorgue0"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxTorque0->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DirectionChange0"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->directionChange0->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Repeat0"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->repeat0->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TargetAngle1"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->targetAngle1->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AngleRate1"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->angleRate1->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MinAngleLimit1"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->minAngleLimit1->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MaxAngleLimit1"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxAngleLimit1->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MaxTorgue1"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxTorque1->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "DirectionChange1"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->directionChange1->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Repeat1"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->repeat1->getBool())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::Vector3 JointUniversalActuatorComponent::getUpdatedJointPosition(void)
	{
		if (nullptr == this->body)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointUniversalActuatorComponent] Could not get 'getUpdatedJointPosition', because there is no body for game object: " + this->gameObjectPtr->getName());
			return Ogre::Vector3::ZERO;
		}

		if (false == this->hasCustomJointPosition)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			return this->body->getPosition() + offset;
		}
		else
		{
			return this->jointPosition;
		}
	}

	bool JointUniversalActuatorComponent::createJoint(const Ogre::Vector3& customJointPosition)
	{
		// Joint base created but not activated, return false, but its no error, hence after that return true.
		if (false == JointComponent::createJoint(customJointPosition))
		{
			return true;
		}

		if (nullptr == this->body)
		{
			this->connect();
		}

		if (Ogre::Vector3::ZERO == customJointPosition)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			//Ogre::Vector3 aPos = Ogre::Vector3(-this->anchorPosition.x, -this->anchorPosition.y, -this->anchorPosition.z);
			//// relative anchor pos: 0.5 0 0 means 50% in X of the size of the parent object
			//Ogre::Vector3 offset = (aPos * size);
			//// take orientation into account, so that orientated joints are processed correctly
			//this->jointPosition = this->body->getPosition() - (this->body->getOrientation() * offset);

			// without rotation
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			this->jointPosition = (this->body->getPosition() + offset);
		}
		else
		{
			this->hasCustomJointPosition = true;
			this->jointPosition = customJointPosition;
		}

		this->round0 = 0;
		this->round1 = 0;
		this->internalDirectionChange0 = this->directionChange0->getBool();
		this->internalDirectionChange1 = this->directionChange1->getBool();
		this->oppositeDir0 = 1.0f;
		this->oppositeDir1 = 1.0f;

		OgreNewt::Body* predecessorBody = nullptr;
		if (this->predecessorJointCompPtr)
		{
			predecessorBody = this->predecessorJointCompPtr->getBody();
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[JointUniversalActuatorComponent] Creating universal actuator joint for body1 name: "
				+ this->predecessorJointCompPtr->getOwner()->getName() + " body2 name: " + this->gameObjectPtr->getName());
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointUniversalActuatorComponent] Created universal actuator joint: " + this->gameObjectPtr->getName() + " with world as parent");
		}

		// Release joint each time, to create new one with new values
		this->internalReleaseJoint();
		this->joint = new OgreNewt::UniversalActuator(this->body, predecessorBody, this->jointPosition, 
												Ogre::Degree(this->angleRate0->getReal()), Ogre::Degree(this->minAngleLimit0->getReal()), Ogre::Degree(this->maxAngleLimit0->getReal()),
												Ogre::Degree(this->angleRate1->getReal()), Ogre::Degree(this->minAngleLimit1->getReal()), Ogre::Degree(this->maxAngleLimit1->getReal()));

		this->joint->setBodyMassScale(this->bodyMassScale->getVector2().x, this->bodyMassScale->getVector2().y);
		// Bad, because causing jerky behavior on ragdolls?
		this->joint->setCollisionState(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		// this->body->setJointRecursiveCollision(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		// this->applyStiffness();

		OgreNewt::UniversalActuator* universalActuatorJoint = dynamic_cast<OgreNewt::UniversalActuator*>(this->joint);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointUniversalActuatorComponent] Created joint: " + this->gameObjectPtr->getName());

		if (0.0f < this->maxTorque0->getReal())
		{
			universalActuatorJoint->SetMaxTorque0(this->maxTorque0->getReal());
		}

		if (true == this->activated->getBool())
		{
			universalActuatorJoint->SetTargetAngle0(Ogre::Degree(this->targetAngle0->getReal()));
		}
		this->setDirectionChange0(this->directionChange0->getBool());
		this->setRepeat0(this->repeat0->getBool());
		
		if (0.0f < this->maxTorque1->getReal())
		{
			universalActuatorJoint->SetMaxTorque1(this->maxTorque1->getReal());
		}

		if (true == this->activated->getBool())
		{
			universalActuatorJoint->SetTargetAngle1(Ogre::Degree(this->targetAngle1->getReal()));
		}
		
		this->setDirectionChange1(this->directionChange1->getBool());
		this->setRepeat1(this->repeat1->getBool());
		
		return true;
	}

	void JointUniversalActuatorComponent::forceShowDebugData(bool activate)
	{
		
	}

	void JointUniversalActuatorComponent::actualizeValue(Variant* attribute)
	{
		JointComponent::actualizeValue(attribute);
		
		if (JointUniversalActuatorComponent::AttrAnchorPosition() == attribute->getName())
		{
			this->setAnchorPosition(attribute->getVector3());
		}
		else if (JointUniversalActuatorComponent::AttrTargetAngle0() == attribute->getName())
		{
			this->setTargetAngle0(Ogre::Degree(attribute->getReal()));
		}
		else if (JointUniversalActuatorComponent::AttrAngleRate0() == attribute->getName())
		{
			this->setAngleRate0(Ogre::Degree(attribute->getReal()));
		}
		else if (JointUniversalActuatorComponent::AttrMinAngleLimit0() == attribute->getName())
		{
			this->setMinAngleLimit0(Ogre::Degree(attribute->getReal()));
		}
		else if (JointUniversalActuatorComponent::AttrMaxAngleLimit0() == attribute->getName())
		{
			this->setMaxAngleLimit0(Ogre::Degree(attribute->getReal()));
		}
		else if (JointUniversalActuatorComponent::AttrMaxTorque0() == attribute->getName())
		{
			this->setMaxTorque0(attribute->getReal());
		}
		else if (JointUniversalActuatorComponent::AttrDirectionChange0() == attribute->getName())
		{
			this->setDirectionChange0(attribute->getBool());
		}
		else if (JointUniversalActuatorComponent::AttrRepeat0() == attribute->getName())
		{
			this->setRepeat0(attribute->getBool());
		}
		else if (JointUniversalActuatorComponent::AttrTargetAngle1() == attribute->getName())
		{
			this->setTargetAngle1(Ogre::Degree(attribute->getReal()));
		}
		else if (JointUniversalActuatorComponent::AttrAngleRate1() == attribute->getName())
		{
			this->setAngleRate1(Ogre::Degree(attribute->getReal()));
		}
		else if (JointUniversalActuatorComponent::AttrMinAngleLimit1() == attribute->getName())
		{
			this->setMinAngleLimit1(Ogre::Degree(attribute->getReal()));
		}
		else if (JointUniversalActuatorComponent::AttrMaxAngleLimit1() == attribute->getName())
		{
			this->setMaxAngleLimit1(Ogre::Degree(attribute->getReal()));
		}
		else if (JointUniversalActuatorComponent::AttrMaxTorque1() == attribute->getName())
		{
			this->setMaxTorque1(attribute->getReal());
		}
		else if (JointUniversalActuatorComponent::AttrDirectionChange1() == attribute->getName())
		{
			this->setDirectionChange1(attribute->getBool());
		}
		else if (JointUniversalActuatorComponent::AttrRepeat1() == attribute->getName())
		{
			this->setRepeat1(attribute->getBool());
		}
	}

	void JointUniversalActuatorComponent::setAnchorPosition(const Ogre::Vector3& anchorPosition)
	{
		this->anchorPosition->setValue(anchorPosition);

		if (nullptr != this->body)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			//Ogre::Vector3 aPos = Ogre::Vector3(-this->anchorPosition.x, -this->anchorPosition.y, -this->anchorPosition.z);
			//// relative anchor pos: 0.5 0 0 means 50% in X of the size of the parent object
			//Ogre::Vector3 offset = (aPos * size);
			//// take orientation into account, so that orientated joints are processed correctly
			//this->jointPosition = this->body->getPosition() - (this->body->getOrientation() * offset);

			// without rotation
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			this->jointPosition = (this->body->getPosition() + offset);
			this->jointAlreadyCreated = false;
		}

		if (nullptr != this->debugGeometryNode)
		{
			this->debugGeometryNode->setPosition(this->jointPosition);
		}
	}

	Ogre::Vector3 JointUniversalActuatorComponent::getAnchorPosition(void) const
	{
		return this->anchorPosition->getVector3();
	}

	void JointUniversalActuatorComponent::setTargetAngle0(const Ogre::Degree& targetAngle0)
	{
		this->targetAngle0->setValue(targetAngle0.valueDegrees());
		OgreNewt::UniversalActuator* universalActuatorJoint = dynamic_cast<OgreNewt::UniversalActuator*>(this->joint);
		if (nullptr != universalActuatorJoint)
		{
			universalActuatorJoint->SetTargetAngle0(targetAngle0);
		}
	}
	
	Ogre::Degree JointUniversalActuatorComponent::getTargetAngle0(void) const
	{
		return Ogre::Degree(this->targetAngle0->getReal());
	}
	
	void JointUniversalActuatorComponent::setAngleRate0(const Ogre::Degree& angleRate0)
	{
		this->angleRate0->setValue(angleRate0.valueDegrees());
		OgreNewt::UniversalActuator* universalActuatorJoint = dynamic_cast<OgreNewt::UniversalActuator*>(this->joint);
		if (nullptr != universalActuatorJoint)
		{
			universalActuatorJoint->SetAngularRate0(angleRate0);
		}
	}
	
	Ogre::Degree JointUniversalActuatorComponent::getAngleRate0(void) const
	{
		return Ogre::Degree(this->angleRate0->getReal());
	}
	
	void JointUniversalActuatorComponent::setMinAngleLimit0(const Ogre::Degree& minAngleLimit0)
	{
		this->minAngleLimit0->setValue(minAngleLimit0.valueDegrees());
		OgreNewt::UniversalActuator* universalActuatorJoint = dynamic_cast<OgreNewt::UniversalActuator*>(this->joint);
		if (nullptr != universalActuatorJoint)
		{
			universalActuatorJoint->SetMinAngularLimit0(minAngleLimit0);
		}
	}

	Ogre::Degree JointUniversalActuatorComponent::getMinAngleLimit0(void) const
	{
		return Ogre::Degree(this->minAngleLimit0->getReal());
	}
	
	void JointUniversalActuatorComponent::setMaxAngleLimit0(const Ogre::Degree& maxAngleLimit0)
	{
		this->maxAngleLimit0->setValue(maxAngleLimit0.valueDegrees());
		OgreNewt::UniversalActuator* universalActuatorJoint = dynamic_cast<OgreNewt::UniversalActuator*>(this->joint);
		if (nullptr != universalActuatorJoint)
		{
			universalActuatorJoint->SetMaxAngularLimit0(maxAngleLimit0);
		}
	}

	Ogre::Degree JointUniversalActuatorComponent::getMaxAngleLimit0(void) const
	{
		return Ogre::Degree(this->maxAngleLimit0->getReal());
	}

	void JointUniversalActuatorComponent::setMaxTorque0(Ogre::Real maxTorque0)
	{
		if (maxTorque0 < 0.0f)
			return;
		
		this->maxTorque0->setValue(maxTorque0);
		OgreNewt::UniversalActuator* universalActuatorJoint = dynamic_cast<OgreNewt::UniversalActuator*>(this->joint);
		if (universalActuatorJoint)
		{
			universalActuatorJoint->SetMaxTorque0(maxTorque0);
		}
	}

	Ogre::Real JointUniversalActuatorComponent::getMaxTorque0(void) const
	{
		return this->maxTorque0->getReal();
	}
	
	void JointUniversalActuatorComponent::setDirectionChange0(bool directionChange0)
	{
		this->directionChange0->setValue(directionChange0);
		this->internalDirectionChange0 = directionChange0;
	}
		
	bool JointUniversalActuatorComponent::getDirectionChange0(void) const
	{
		return this->directionChange0->getBool();
	}
	
	void JointUniversalActuatorComponent::setRepeat0(bool repeat0)
	{
		this->repeat0->setValue(repeat0);
	}
	
	bool JointUniversalActuatorComponent::getRepeat0(void) const
	{
		return this->repeat0->getBool();
	}
	
	void JointUniversalActuatorComponent::setTargetAngle1(const Ogre::Degree& targetAngle1)
	{
		this->targetAngle1->setValue(targetAngle1.valueDegrees());
		OgreNewt::UniversalActuator* universalActuatorJoint = dynamic_cast<OgreNewt::UniversalActuator*>(this->joint);
		if (nullptr != universalActuatorJoint)
		{
			universalActuatorJoint->SetTargetAngle1(targetAngle1);
		}
	}
	
	Ogre::Degree JointUniversalActuatorComponent::getTargetAngle1(void) const
	{
		return Ogre::Degree(this->targetAngle1->getReal());
	}
	
	void JointUniversalActuatorComponent::setAngleRate1(const Ogre::Degree& angleRate1)
	{
		this->angleRate1->setValue(angleRate1.valueDegrees());
		OgreNewt::UniversalActuator* universalActuatorJoint = dynamic_cast<OgreNewt::UniversalActuator*>(this->joint);
		if (nullptr != universalActuatorJoint)
		{
			universalActuatorJoint->SetAngularRate1(angleRate1);
		}
	}
	
	Ogre::Degree JointUniversalActuatorComponent::getAngleRate1(void) const
	{
		return Ogre::Degree(this->angleRate1->getReal());
	}
	
	void JointUniversalActuatorComponent::setMinAngleLimit1(const Ogre::Degree& minAngleLimit1)
	{
		this->minAngleLimit1->setValue(minAngleLimit1.valueDegrees());
		OgreNewt::UniversalActuator* universalActuatorJoint = dynamic_cast<OgreNewt::UniversalActuator*>(this->joint);
		if (nullptr != universalActuatorJoint)
		{
			universalActuatorJoint->SetMinAngularLimit1(minAngleLimit1);
		}
	}

	Ogre::Degree JointUniversalActuatorComponent::getMinAngleLimit1(void) const
	{
		return Ogre::Degree(this->minAngleLimit1->getReal());
	}
	
	void JointUniversalActuatorComponent::setMaxAngleLimit1(const Ogre::Degree& maxAngleLimit1)
	{
		this->maxAngleLimit1->setValue(maxAngleLimit1.valueDegrees());
		OgreNewt::UniversalActuator* universalActuatorJoint = dynamic_cast<OgreNewt::UniversalActuator*>(this->joint);
		if (nullptr != universalActuatorJoint)
		{
			universalActuatorJoint->SetMaxAngularLimit1(maxAngleLimit1);
		}
	}

	Ogre::Degree JointUniversalActuatorComponent::getMaxAngleLimit1(void) const
	{
		return Ogre::Degree(this->maxAngleLimit1->getReal());
	}

	void JointUniversalActuatorComponent::setMaxTorque1(Ogre::Real maxTorque1)
	{
		if (maxTorque1 < 0.0f)
			return;
		
		this->maxTorque1->setValue(maxTorque1);
		OgreNewt::UniversalActuator* universalActuatorJoint = dynamic_cast<OgreNewt::UniversalActuator*>(this->joint);
		if (universalActuatorJoint)
		{
			universalActuatorJoint->SetMaxTorque1(maxTorque1);
		}
	}

	Ogre::Real JointUniversalActuatorComponent::getMaxTorque1(void) const
	{
		return this->maxTorque1->getReal();
	}
	
	void JointUniversalActuatorComponent::setDirectionChange1(bool directionChange1)
	{
		this->directionChange1->setValue(directionChange1);
		this->internalDirectionChange1 = directionChange1;
	}
		
	bool JointUniversalActuatorComponent::getDirectionChange1(void) const
	{
		return this->directionChange1->getBool();
	}
	
	void JointUniversalActuatorComponent::setRepeat1(bool repeat1)
	{
		this->repeat1->setValue(repeat1);
	}
	
	bool JointUniversalActuatorComponent::getRepeat1(void) const
	{
		return this->repeat1->getBool();
	}

	/*******************************Joint6DofComponent*******************************/

	Joint6DofComponent::Joint6DofComponent()
		: JointComponent()
	{
		this->type->setReadOnly(false);
		this->type->setValue(this->getClassName());
		this->type->setReadOnly(true);
		
		this->anchorPosition0 = new Variant(Joint6DofComponent::AttrAnchorPosition0(), Ogre::Vector3::ZERO, this->attributes);
		this->anchorPosition1 = new Variant(Joint6DofComponent::AttrAnchorPosition1(), Ogre::Vector3::ZERO, this->attributes);
		
		this->minStopDistance = new Variant(Joint6DofComponent::AttrMinStopDistance(), 0.0f, this->attributes);
		this->maxStopDistance = new Variant(Joint6DofComponent::AttrMaxStopDistance(), 0.0f, this->attributes);

		this->minYawAngleLimit = new Variant(Joint6DofComponent::AttrMinYawAngleLimit(), 0.0f, this->attributes);
		this->maxYawAngleLimit = new Variant(Joint6DofComponent::AttrMaxYawAngleLimit(), 0.0f, this->attributes);

		this->minPitchAngleLimit = new Variant(Joint6DofComponent::AttrMinPitchAngleLimit(), 0.0f, this->attributes);
		this->maxPitchAngleLimit = new Variant(Joint6DofComponent::AttrMaxPitchAngleLimit(), 0.0f, this->attributes);

		this->minRollAngleLimit = new Variant(Joint6DofComponent::AttrMinRollAngleLimit(), 0.0f, this->attributes);
		this->maxRollAngleLimit = new Variant(Joint6DofComponent::AttrMaxRollAngleLimit(), 0.0f, this->attributes);

		this->targetId->setVisible(false);
	}

	Joint6DofComponent::~Joint6DofComponent()
	{

	}

	GameObjectCompPtr Joint6DofComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		Joint6DofCompPtr clonedJointCompPtr(boost::make_shared<Joint6DofComponent>());
		
		clonedJointCompPtr->setType(this->type->getString());
		clonedJointCompPtr->internalSetPriorId(this->id->getULong());
		clonedJointCompPtr->setPredecessorId(this->predecessorId->getULong());
		clonedJointCompPtr->setTargetId(this->targetId->getULong());
		clonedJointCompPtr->setJointPosition(this->jointPosition);
		clonedJointCompPtr->setJointRecursiveCollisionEnabled(this->jointRecursiveCollision->getBool());
		clonedJointCompPtr->stiffness->setValue(this->stiffness->getReal());

		clonedJointCompPtr->setAnchorPosition0(this->anchorPosition0->getVector3());
		clonedJointCompPtr->setAnchorPosition1(this->anchorPosition1->getVector3());
		clonedJointCompPtr->setMinMaxStopDistance(this->minStopDistance->getVector3(), this->maxStopDistance->getVector3());
		clonedJointCompPtr->setMinMaxYawAngleLimit(Ogre::Degree(this->minYawAngleLimit->getReal()), Ogre::Degree(this->maxYawAngleLimit->getReal()));
		clonedJointCompPtr->setMinMaxPitchAngleLimit(Ogre::Degree(this->minPitchAngleLimit->getReal()), Ogre::Degree(this->maxPitchAngleLimit->getReal()));
		clonedJointCompPtr->setMinMaxRollAngleLimit(Ogre::Degree(this->minYawAngleLimit->getReal()), Ogre::Degree(this->maxRollAngleLimit->getReal()));

		clonedGameObjectPtr->addComponent(clonedJointCompPtr);
		clonedJointCompPtr->setOwner(clonedGameObjectPtr);

		clonedJointCompPtr->setActivated(this->activated->getBool());

		return clonedJointCompPtr;
	}

	bool Joint6DofComponent::init(xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		JointComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnchorPosition0")
		{
			this->anchorPosition0->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(0.0f, 0.0f, 0.0f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnchorPosition1")
		{
			this->anchorPosition1->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(0.0f, 0.0f, 0.0f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MinStopDistance")
		{
			this->minStopDistance->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxStopDistance")
		{
			this->maxStopDistance->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MinYawAngleLimit")
		{
			this->minYawAngleLimit->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxYawAngleLimit")
		{
			this->maxYawAngleLimit->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MinPitchAngleLimit")
		{
			this->minPitchAngleLimit->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxPitchAngleLimit")
		{
			this->maxPitchAngleLimit->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MinRollAngleLimit")
		{
			this->minRollAngleLimit->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxRollAngleLimit")
		{
			this->maxRollAngleLimit->setValue(XMLConverter::getAttribReal(propertyElement, "data", Ogre::Real(0)));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	bool Joint6DofComponent::postInit(void)
	{
		JointComponent::postInit();

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[Joint6DofComponent] Init joint 6 DOF component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool Joint6DofComponent::connect(void)
	{
		bool success = JointComponent::connect();
		return success;
	}

	bool Joint6DofComponent::disconnect(void)
	{
		bool success = JointComponent::disconnect();
		return success;
	}

	Ogre::String Joint6DofComponent::getClassName(void) const
	{
		return "Joint6DofComponent";
	}

	Ogre::String Joint6DofComponent::getParentClassName(void) const
	{
		return "JointComponent";
	}

	void Joint6DofComponent::update(Ogre::Real dt, bool notSimulating)
	{

	}

	bool Joint6DofComponent::createJoint(const Ogre::Vector3& customJointPosition)
	{
		// Joint base created but not activated, return false, but its no error, hence after that return true.
		if (false == JointComponent::createJoint(customJointPosition))
		{
			return true;
		}

		if (true == this->jointAlreadyCreated)
		{
			// joint already created so skip
			return true;
		}
		if (nullptr == this->body)
		{
			this->connect();
		}
		
		if (Ogre::Vector3::ZERO == customJointPosition)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			//Ogre::Vector3 aPos = Ogre::Vector3(-this->anchorPosition.x, -this->anchorPosition.y, -this->anchorPosition.z);
			//// relative anchor pos: 0.5 0 0 means 50% in X of the size of the parent object
			//Ogre::Vector3 offset = (aPos * size);
			//// take orientation into account, so that orientated joints are processed correctly
			//this->jointPosition = this->body->getPosition() - (this->body->getOrientation() * offset);

			// without rotation
			Ogre::Vector3 offset = (this->anchorPosition0->getVector3() * size);
			this->jointPosition = (this->body->getPosition() + offset);
		}
		else
		{
			this->hasCustomJointPosition = true;
			this->jointPosition = customJointPosition;
		}

		Ogre::Vector3 jointPosition2 = Ogre::Vector3::ZERO;
		OgreNewt::Body* predecessorBody = nullptr;
		if (this->predecessorJointCompPtr)
		{
			predecessorBody = this->predecessorJointCompPtr->getBody();
			Ogre::Vector3 size = predecessorBody->getAABB().getSize();
			// without rotation
			Ogre::Vector3 offset = (this->anchorPosition1->getVector3() * size);
			jointPosition2 = (predecessorBody->getPosition() + offset);

			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[Joint6DofComponent] Creating 6 Dof joint for body1 name: "
				+ this->predecessorJointCompPtr->getOwner()->getName() + " body2 name: " + this->gameObjectPtr->getName());
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[Joint6DofComponent] Created 6 Dof joint: " + this->gameObjectPtr->getName() + " with world as parent");
		}

		// Release joint each time, to create new one with new values
		this->internalReleaseJoint();
		this->joint = new OgreNewt::Joint6Dof(this->body, predecessorBody, this->jointPosition, jointPosition2);
		OgreNewt::Joint6Dof* joint6Dof = dynamic_cast<OgreNewt::Joint6Dof*>(this->joint);

		this->joint->setBodyMassScale(this->bodyMassScale->getVector2().x, this->bodyMassScale->getVector2().y);
		this->joint->setCollisionState(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		// this->body->setJointRecursiveCollision(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		// this->applyStiffness();

		joint6Dof->setLinearLimits(this->minStopDistance->getVector3(), this->maxStopDistance->getVector3());
		joint6Dof->setYawLimits(Ogre::Degree(this->minYawAngleLimit->getReal()), Ogre::Degree(this->maxYawAngleLimit->getReal()));
		joint6Dof->setPitchLimits(Ogre::Degree(this->minPitchAngleLimit->getReal()), Ogre::Degree(this->maxPitchAngleLimit->getReal()));
		joint6Dof->setRollLimits(Ogre::Degree(this->minRollAngleLimit->getReal()), Ogre::Degree(this->maxRollAngleLimit->getReal()));

		this->jointAlreadyCreated = true;

		return true;
	}
	
	void Joint6DofComponent::actualizeValue(Variant* attribute)
	{
		JointComponent::actualizeValue(attribute);

		if (Joint6DofComponent::AttrAnchorPosition0() == attribute->getName())
		{
			this->setAnchorPosition0(attribute->getVector3());
		}
		else if (Joint6DofComponent::AttrAnchorPosition1() == attribute->getName())
		{
			this->setAnchorPosition1(attribute->getVector3());
		}
		else if (Joint6DofComponent::AttrMinStopDistance() == attribute->getName())
		{
			this->setMinMaxStopDistance(attribute->getVector3(), this->maxStopDistance->getVector3());
		}
		else if (Joint6DofComponent::AttrMaxStopDistance() == attribute->getName())
		{
			this->setMinMaxStopDistance(this->minStopDistance->getVector3(), attribute->getVector3());
		}
		else if (Joint6DofComponent::AttrMinYawAngleLimit() == attribute->getName())
		{
			this->setMinMaxYawAngleLimit(Ogre::Degree(attribute->getReal()), Ogre::Degree(this->maxYawAngleLimit->getReal()));
		}
		else if (Joint6DofComponent::AttrMaxYawAngleLimit() == attribute->getName())
		{
			this->setMinMaxYawAngleLimit(Ogre::Degree(this->minYawAngleLimit->getReal()), Ogre::Degree(attribute->getReal()));
		}
		else if (Joint6DofComponent::AttrMinPitchAngleLimit() == attribute->getName())
		{
			this->setMinMaxPitchAngleLimit(Ogre::Degree(attribute->getReal()), Ogre::Degree(this->maxPitchAngleLimit->getReal()));
		}
		else if (Joint6DofComponent::AttrMaxPitchAngleLimit() == attribute->getName())
		{
			this->setMinMaxPitchAngleLimit(Ogre::Degree(this->minPitchAngleLimit->getReal()), Ogre::Degree(attribute->getReal()));
		}
		else if (Joint6DofComponent::AttrMinRollAngleLimit() == attribute->getName())
		{
			this->setMinMaxRollAngleLimit(Ogre::Degree(attribute->getReal()), Ogre::Degree(this->maxRollAngleLimit->getReal()));
		}
		else if (Joint6DofComponent::AttrMaxRollAngleLimit() == attribute->getName())
		{
			this->setMinMaxRollAngleLimit(Ogre::Degree(this->minRollAngleLimit->getReal()), Ogre::Degree(attribute->getReal()));
		}
	}
	
	void Joint6DofComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		JointComponent::writeXML(propertiesXML, doc, filePath);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnchorPosition0"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->anchorPosition0->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnchorPosition1"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->anchorPosition1->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MinStopDistance"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->minStopDistance->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MaxStopDistance"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxStopDistance->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MinYawAngleLimit"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->minYawAngleLimit->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MaxYawAngleLimit"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxYawAngleLimit->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MinPitchAngleLimit"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->minPitchAngleLimit->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MaxPitchAngleLimit"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxPitchAngleLimit->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MinRollAngleLimit"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->minRollAngleLimit->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MaxRollAngleLimit"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxRollAngleLimit->getReal())));
		propertiesXML->append_node(propertyXML);

	}

	Ogre::Vector3 Joint6DofComponent::getUpdatedJointPosition(void)
	{
		if (nullptr == this->body)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[Joint6DofComponent] Could not get 'getUpdatedJointPosition', because there is no body for game object: " + this->gameObjectPtr->getName());
			return Ogre::Vector3::ZERO;
		}

		if (false == this->hasCustomJointPosition)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			Ogre::Vector3 offset = (this->anchorPosition0->getVector3() * size);
			return this->body->getPosition() + offset;
		}
		else
		{
			return this->jointPosition;
		}
	}

	void Joint6DofComponent::setAnchorPosition0(const Ogre::Vector3& anchorPosition)
	{
		this->anchorPosition0->setValue(anchorPosition);
		this->jointAlreadyCreated = false;
	}

	Ogre::Vector3 Joint6DofComponent::getAnchorPosition0(void) const
	{
		return this->anchorPosition0->getVector3();
	}

	void Joint6DofComponent::setAnchorPosition1(const Ogre::Vector3& anchorPosition)
	{
		this->anchorPosition1->setValue(anchorPosition);
	}

	Ogre::Vector3 Joint6DofComponent::getAnchorPosition1(void) const
	{
		return this->anchorPosition1->getVector3();
	}

	void Joint6DofComponent::setMinMaxStopDistance(const Ogre::Vector3& minStopDistance, const Ogre::Vector3& maxStopDistance)
	{
		this->minStopDistance->setValue(minStopDistance);
		this->maxStopDistance->setValue(maxStopDistance);
		
		OgreNewt::Joint6Dof* joint6Dof = dynamic_cast<OgreNewt::Joint6Dof*>(this->joint);

		if (joint6Dof)
		{
			joint6Dof->setLinearLimits(this->minStopDistance->getVector3(), this->maxStopDistance->getVector3());
		}
	}
	
	Ogre::Vector3 Joint6DofComponent::getMinStopDistance(void) const
	{
		return this->minStopDistance->getVector3();
	}
	
	Ogre::Vector3 Joint6DofComponent::getMaxStopDistance(void) const
	{
		return this->maxStopDistance->getVector3();
	}
	
	void Joint6DofComponent::setMinMaxYawAngleLimit(const Ogre::Degree& minAngleLimit, const Ogre::Degree& maxAngleLimit)
	{
		this->minYawAngleLimit->setValue(minAngleLimit.valueDegrees());
		this->maxYawAngleLimit->setValue(maxAngleLimit.valueDegrees());

		OgreNewt::Joint6Dof* joint6Dof = dynamic_cast<OgreNewt::Joint6Dof*>(this->joint);

		if (joint6Dof)
		{
			joint6Dof->setYawLimits(Ogre::Degree(this->minYawAngleLimit->getReal()), Ogre::Degree(this->maxYawAngleLimit->getReal()));
		}
	}
	
	Ogre::Degree Joint6DofComponent::getMinYawAngleLimit(void) const
	{
		return Ogre::Degree(this->minYawAngleLimit->getReal());
	}
	
	Ogre::Degree Joint6DofComponent::getMaxYawAngleLimit(void) const
	{
		return Ogre::Degree(this->maxYawAngleLimit->getReal());
	}

	void Joint6DofComponent::setMinMaxPitchAngleLimit(const Ogre::Degree& minAngleLimit, const Ogre::Degree& maxAngleLimit)
	{
		this->minPitchAngleLimit->setValue(minAngleLimit.valueDegrees());
		this->maxPitchAngleLimit->setValue(maxAngleLimit.valueDegrees());

		OgreNewt::Joint6Dof* joint6Dof = dynamic_cast<OgreNewt::Joint6Dof*>(this->joint);

		if (joint6Dof)
		{
			joint6Dof->setPitchLimits(Ogre::Degree(this->minPitchAngleLimit->getReal()), Ogre::Degree(this->maxPitchAngleLimit->getReal()));
		}
	}
	
	Ogre::Degree Joint6DofComponent::getMinPitchAngleLimit(void) const
	{
		return Ogre::Degree(this->minPitchAngleLimit->getReal());
	}
	
	Ogre::Degree Joint6DofComponent::getMaxPitchAngleLimit(void) const
	{
		return Ogre::Degree(this->maxPitchAngleLimit->getReal());
	}

	void Joint6DofComponent::setMinMaxRollAngleLimit(const Ogre::Degree& minAngleLimit, const Ogre::Degree& maxAngleLimit)
	{
		this->minRollAngleLimit->setValue(minAngleLimit.valueDegrees());
		this->maxRollAngleLimit->setValue(maxAngleLimit.valueDegrees());

		OgreNewt::Joint6Dof* joint6Dof = dynamic_cast<OgreNewt::Joint6Dof*>(this->joint);

		if (joint6Dof)
		{
			joint6Dof->setRollLimits(Ogre::Degree(this->minRollAngleLimit->getReal()), Ogre::Degree(this->maxRollAngleLimit->getReal()));
		}
	}
	
	Ogre::Degree Joint6DofComponent::getMinRollAngleLimit(void) const
	{
		return Ogre::Degree(this->minRollAngleLimit->getReal());
	}
	
	Ogre::Degree Joint6DofComponent::getMaxRollAngleLimit(void) const
	{
		return Ogre::Degree(this->maxRollAngleLimit->getReal());
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	JointMotorComponent::JointMotorComponent(void)
		: JointComponent()
	{
		// Note that also in JointComponent internalBaseInit() is called, which sets the values for JointComponent, so this is called for already existing values
		// But luckely in Variant, no new attributes are added, but the values changed via interalAdd(...) in Variant
		this->type->setReadOnly(false);
		this->type->setValue(this->getClassName());
		this->type->setReadOnly(true);
		this->type->setDescription("A motor joint, which needs another joint component as reference (predecessor), which should be rotated. Its also possible to use a second joint component (target) and pin for a different rotation.");

		this->pin0 = new Variant(JointMotorComponent::AttrPin0(), Ogre::Vector3(0.0f, 0.0f, 1.0f), this->attributes);
		this->pin1 = new Variant(JointMotorComponent::AttrPin1(), Ogre::Vector3(0.0f, 0.0f, 1.0f), this->attributes);
		this->speed0 = new Variant(JointMotorComponent::AttrSpeed0(), 10.0f, this->attributes);
		this->speed1 = new Variant(JointMotorComponent::AttrSpeed1(), -10.0f, this->attributes);
		this->torgue0 = new Variant(JointMotorComponent::AttrTorque0(), 1.0f, this->attributes);
		this->torgue1 = new Variant(JointMotorComponent::AttrTorque1(), 1.0f, this->attributes);

		this->jointRecursiveCollision->setVisible(false);
	}

	JointMotorComponent::~JointMotorComponent()
	{
	}

	bool JointMotorComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		JointComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Pin0")
		{
			this->pin0->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(0.0f, 0.0f, 1.0f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Pin1")
		{
			this->pin1->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(0.0f, 0.0f, 1.0f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Speed0")
		{
			this->speed0->setValue(XMLConverter::getAttribReal(propertyElement, "data", 10.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Speed1")
		{
			this->speed1->setValue(XMLConverter::getAttribReal(propertyElement, "data", 10.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Torgue0")
		{
			this->torgue0->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Torgue1")
		{
			this->torgue1->setValue(XMLConverter::getAttribReal(propertyElement, "data", 1.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		return true;
	}

	bool JointMotorComponent::postInit(void)
	{
		JointComponent::postInit();

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointMotorComponent] Init joint motor component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool JointMotorComponent::connect(void)
	{
		bool success = JointComponent::connect();

		return success;
	}

	bool JointMotorComponent::disconnect(void)
	{
		bool success = JointComponent::disconnect();

		return success;
	}

	Ogre::String JointMotorComponent::getClassName(void) const
	{
		return "JointMotorComponent";
	}

	Ogre::String JointMotorComponent::getParentClassName(void) const
	{
		return "JointComponent";
	}

	void JointMotorComponent::update(Ogre::Real dt, bool notSimulating)
	{

	}

	GameObjectCompPtr JointMotorComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		JointMotorCompPtr clonedJointCompPtr(boost::make_shared<JointMotorComponent>());
		
		clonedJointCompPtr->setType(this->type->getString());
		clonedJointCompPtr->internalSetPriorId(this->id->getULong());
		clonedJointCompPtr->setPredecessorId(this->predecessorId->getULong());
		clonedJointCompPtr->setTargetId(this->targetId->getULong());
		clonedJointCompPtr->setJointPosition(this->jointPosition);

		clonedJointCompPtr->setPin0(this->pin0->getVector3());
		clonedJointCompPtr->setPin1(this->pin1->getVector3());
		clonedJointCompPtr->setSpeed0(this->speed0->getReal());
		clonedJointCompPtr->setSpeed1(this->speed1->getReal());
		clonedJointCompPtr->setTorgue0(this->torgue0->getReal());
		clonedJointCompPtr->setTorgue1(this->torgue1->getReal());

		clonedGameObjectPtr->addComponent(clonedJointCompPtr);
		clonedJointCompPtr->setOwner(clonedGameObjectPtr);

		clonedJointCompPtr->setActivated(this->activated->getBool());

		return clonedJointCompPtr;
	}

	bool JointMotorComponent::createJoint(const Ogre::Vector3& customJointPosition)
	{
		// Joint base created but not activated, return false, but its no error, hence after that return true.
		if (false == JointComponent::createJoint(customJointPosition))
		{
			return true;
		}

		if (true == this->jointAlreadyCreated)
		{
			// joint already created so skip
			return true;
		}
		
		if (nullptr == this->body)
		{
			this->connect();
		}
		if (Ogre::Vector3::ZERO == this->pin0->getVector3())
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[JointMotorComponent] Cannot create joint for: " + this->gameObjectPtr->getName() + " because the pin 0 is zero.");
			return false;
		}

		if (Ogre::Vector3::ZERO == customJointPosition)
		{
			this->jointPosition = this->body->getPosition();
		}
		else
		{
			this->hasCustomJointPosition = true;
			this->jointPosition = customJointPosition;
		}

		OgreNewt::Body* predecessorBody = nullptr;
		if (this->predecessorJointCompPtr)
		{
			predecessorBody = this->predecessorJointCompPtr->getBody();
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[JointMotorComponent] Creating motor joint for body name: "
				+ this->gameObjectPtr->getName() + " reference body0 name: " + this->predecessorJointCompPtr->getOwner()->getName());
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointMotorComponent] Cannot create joint for: " + this->gameObjectPtr->getName() + " because there is no reference joint 0 (predecessor).");
			return false;
		}

		OgreNewt::Body* targetBody = nullptr;
		if (this->targetJointCompPtr)
		{
			targetBody = this->targetJointCompPtr->getBody();
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[JointMotorComponent] Creating motor joint for body name: "
				+ this->gameObjectPtr->getName() + " reference body1 name: " + this->targetJointCompPtr->getOwner()->getName());

			if (Ogre::Vector3::ZERO == this->pin1->getVector3())
			{
				Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[JointMotorComponent] Cannot create joint for: " + this->gameObjectPtr->getName() + " because the pin 1 is zero.");
				return false;
			}
		}

		// Release joint each time, to create new one with new values
		this->internalReleaseJoint();
	
		this->joint = new OgreNewt::Motor(this->body, predecessorBody, targetBody, predecessorBody->getOrientation() * this->pin0->getVector3(),
													targetBody->getOrientation() * this->pin1->getVector3());
		

		OgreNewt::Motor* motorJoint = dynamic_cast<OgreNewt::Motor*>(this->joint);
		
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointMotoreComponent] Created joint: " + this->gameObjectPtr->getName());

		motorJoint->SetSpeed0(this->speed0->getReal());
		motorJoint->SetSpeed1(this->speed1->getReal());
		motorJoint->SetTorque0(this->torgue0->getReal());
		motorJoint->SetTorque1(this->torgue1->getReal());

		this->jointAlreadyCreated = true;

		return true;
	}

	void JointMotorComponent::forceShowDebugData(bool activate)
	{
	}

	void JointMotorComponent::actualizeValue(Variant* attribute)
	{
		JointComponent::actualizeValue(attribute);
		
		if (JointMotorComponent::AttrPin0() == attribute->getName())
		{
			this->setPin0(attribute->getVector3());
		}
		else if (JointMotorComponent::AttrPin1() == attribute->getName())
		{
			this->setPin1(attribute->getVector3());
		}
		else if (JointMotorComponent::AttrSpeed0() == attribute->getName())
		{
			this->setSpeed0(attribute->getReal());
		}
		else if (JointMotorComponent::AttrSpeed1() == attribute->getName())
		{
			this->setSpeed1(attribute->getReal());
		}
		else if (JointMotorComponent::AttrTorque0() == attribute->getName())
		{
			this->setTorgue0(attribute->getReal());
		}
		else if (JointMotorComponent::AttrTorque1() == attribute->getName())
		{
			this->setTorgue1(attribute->getReal());
		}
	}

	void JointMotorComponent::writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath)
	{
		JointComponent::writeXML(propertiesXML, doc, filePath);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Pin0"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->pin0->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Pin1"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->pin1->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Speed0"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->speed0->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Speed1"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->speed1->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Torgue0"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->torgue0->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Torgue1"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->torgue1->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	void JointMotorComponent::setPin0(const Ogre::Vector3& pin0)
	{
		this->pin0->setValue(pin0);
	}

	Ogre::Vector3 JointMotorComponent::getPin0(void) const
	{
		return this->pin0->getVector3();
	}

	void JointMotorComponent::setPin1(const Ogre::Vector3& pin1)
	{
		this->pin1->setValue(pin1);
	}

	Ogre::Vector3 JointMotorComponent::getPin1(void) const
	{
		return this->pin1->getVector3();
	}

	void JointMotorComponent::setSpeed0(Ogre::Real speed0)
	{
		this->speed0->setValue(speed0);
		OgreNewt::Motor* motorJoint = dynamic_cast<OgreNewt::Motor*>(this->joint);
		if (motorJoint)
		{
			motorJoint->SetSpeed0(speed0);
		}
	}

	Ogre::Real JointMotorComponent::getSpeed0(void) const
	{
		return this->speed0->getReal();
	}

	void JointMotorComponent::setSpeed1(Ogre::Real speed1)
	{
		this->speed1->setValue(speed1);
		OgreNewt::Motor* motorJoint = dynamic_cast<OgreNewt::Motor*>(this->joint);
		if (motorJoint)
		{
			motorJoint->SetSpeed1(speed1);
		}
	}

	Ogre::Real JointMotorComponent::getSpeed1(void) const
	{
		return this->speed1->getReal();
	}

	void JointMotorComponent::setTorgue0(Ogre::Real torgue0)
	{
		this->torgue0->setValue(torgue0);
		OgreNewt::Motor* motorJoint = dynamic_cast<OgreNewt::Motor*>(this->joint);
		if (motorJoint)
		{
			motorJoint->SetTorque0(this->torgue0->getReal());
		}
	}

	Ogre::Real JointMotorComponent::getTorgue0(void) const
	{
		return this->torgue0->getReal();
	}

	void JointMotorComponent::setTorgue1(Ogre::Real torgue1)
	{
		this->torgue1->setValue(torgue1);
		OgreNewt::Motor* motorJoint = dynamic_cast<OgreNewt::Motor*>(this->joint);
		if (motorJoint)
		{
			motorJoint->SetTorque1(this->torgue1->getReal());
		}
	}

	Ogre::Real JointMotorComponent::getTorgue1(void) const
	{
		return this->torgue1->getReal();
	}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	JointWheelComponent::JointWheelComponent(void)
		: JointComponent()
	{
		// Note that also in JointComponent internalBaseInit() is called, which sets the values for JointComponent, so this is called for already existing values
		// But luckely in Variant, no new attributes are added, but the values changed via interalAdd(...) in Variant
		this->type->setReadOnly(false);
		this->type->setValue(this->getClassName());
		this->type->setReadOnly(true);
		this->type->setDescription("An object attached to a wheel joint can only rotate around one dimension perpendicular to the axis it is attached to and also steer.");
		
		this->anchorPosition = new Variant(JointWheelComponent::AttrAnchorPosition(), Ogre::Vector3::ZERO, this->attributes);
		this->pin = new Variant(JointWheelComponent::AttrPin(), Ogre::Vector3(0.0f, 0.0f, 1.0f), this->attributes);
		this->targetSteeringAngle = new Variant(JointWheelComponent::AttrTargetSteeringAngle(), 60.0f, this->attributes);
		this->steeringRate = new Variant(JointWheelComponent::AttrSteeringRate(), 0.0f, this->attributes);

		this->targetId->setVisible(false);
	}

	JointWheelComponent::~JointWheelComponent()
	{
	}

	bool JointWheelComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		JointComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "AnchorPosition")
		{
			this->anchorPosition->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(0.0f, 0.0f, 0.0f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Pin")
		{
			this->pin->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(0.0f, 0.0f, 0.0f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetSteeringAngle")
		{
			this->targetSteeringAngle->setValue(XMLConverter::getAttribReal(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SteeringRate")
		{
			this->steeringRate->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}

		return true;
	}

	bool JointWheelComponent::postInit(void)
	{
		JointComponent::postInit();

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointWheelComponent] Init joint wheel component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool JointWheelComponent::connect(void)
	{
		bool success = JointComponent::connect();

		// this->internalShowDebugData(true, 0, this->jointPosition, this->pin->getVector3());

		return success;
	}

	bool JointWheelComponent::disconnect(void)
	{
		bool success = JointComponent::disconnect();

		// this->internalShowDebugData(false, 0, this->jointPosition, this->pin->getVector3());

		return success;
	}

	Ogre::String JointWheelComponent::getClassName(void) const
	{
		return "JointWheelComponent";
	}

	Ogre::String JointWheelComponent::getParentClassName(void) const
	{
		return "JointComponent";
	}

	void JointWheelComponent::update(Ogre::Real dt, bool notSimulating)
	{

	}
	
	GameObjectCompPtr JointWheelComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		JointWheelCompPtr clonedJointCompPtr(boost::make_shared<JointWheelComponent>());
		
		clonedJointCompPtr->setType(this->type->getString());
		clonedJointCompPtr->internalSetPriorId(this->id->getULong());
		clonedJointCompPtr->setPredecessorId(this->predecessorId->getULong());
		clonedJointCompPtr->setTargetId(this->targetId->getULong());
		clonedJointCompPtr->setJointPosition(this->jointPosition);
		clonedJointCompPtr->setJointRecursiveCollisionEnabled(this->jointRecursiveCollision->getBool());
		clonedJointCompPtr->stiffness->setValue(this->stiffness->getReal()); // Do not call setter, because internally a bool flag is set, so that stiffness is always used

		clonedJointCompPtr->setAnchorPosition(this->anchorPosition->getVector3());
		clonedJointCompPtr->setPin(this->pin->getVector3());
		clonedJointCompPtr->setTargetSteeringAngle(Ogre::Degree(this->targetSteeringAngle->getReal()));
		clonedJointCompPtr->setSteeringRate(this->steeringRate->getReal());

		clonedGameObjectPtr->addComponent(clonedJointCompPtr);
		clonedJointCompPtr->setOwner(clonedGameObjectPtr);

		clonedJointCompPtr->setActivated(this->activated->getBool());

		return clonedJointCompPtr;
	}

	bool JointWheelComponent::createJoint(const Ogre::Vector3& customJointPosition)
	{
		// Joint base created but not activated, return false, but its no error, hence after that return true.
		if (false == JointComponent::createJoint(customJointPosition))
		{
			return true;
		}

		if (true == this->jointAlreadyCreated)
		{
			// joint already created so skip
			return true;
		}
		
		if (nullptr == this->body)
		{
			this->connect();
		}
		if (Ogre::Vector3::ZERO == this->pin->getVector3())
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[JointWheelComponent] Cannot create joint for: " + this->gameObjectPtr->getName() + " because the pin is zero.");
			return false;
		}

		if (Ogre::Vector3::ZERO == customJointPosition)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			//Ogre::Vector3 aPos = Ogre::Vector3(-this->anchorPosition.x, -this->anchorPosition.y, -this->anchorPosition.z);
			//// relative anchor pos: 0.5 0 0 means 50% in X of the size of the parent object
			//Ogre::Vector3 offset = (aPos * size);
			//// take orientation into account, so that orientated joints are processed correctly
			//this->jointPosition = this->body->getPosition() - (this->body->getOrientation() * offset);

			// without rotation
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			this->jointPosition = (this->body->getPosition() + offset);
		}
		else
		{
			this->hasCustomJointPosition = true;
			this->jointPosition = customJointPosition;
		}

		OgreNewt::Body* predecessorBody = nullptr;
		if (this->predecessorJointCompPtr)
		{
			predecessorBody = this->predecessorJointCompPtr->getBody();
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[JointWheelComponent] Creating wheel joint for body1 name: "
				+ this->predecessorJointCompPtr->getOwner()->getName() + " body2 name: " + this->gameObjectPtr->getName());
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointWheelComponent] Created wheel joint: " + this->gameObjectPtr->getName() + " with world as parent");
		}

		// Release joint each time, to create new one with new values
		this->internalReleaseJoint();
		
		this->joint = new OgreNewt::Wheel(this->body, predecessorBody, this->jointPosition, this->body->getOrientation() * this->pin->getVector3());

		// Bad, because causing jerky behavior on ragdolls?

		this->joint->setBodyMassScale(this->bodyMassScale->getVector2().x, this->bodyMassScale->getVector2().y);
		this->joint->setCollisionState(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		// this->body->setJointRecursiveCollision(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		// this->applyStiffness();

		OgreNewt::Wheel* wheelJoint = dynamic_cast<OgreNewt::Wheel*>(this->joint);

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointWheelComponent] Created joint: " + this->gameObjectPtr->getName());

		wheelJoint->SetTargetSteerAngle(Ogre::Degree(this->targetSteeringAngle->getReal()));
		wheelJoint->SetSteerRate(this->steeringRate->getReal());

		this->jointAlreadyCreated = true;

		return true;
	}

	void JointWheelComponent::forceShowDebugData(bool activate)
	{
	}

	void JointWheelComponent::actualizeValue(Variant* attribute)
	{
		JointComponent::actualizeValue(attribute);
		
		if (JointWheelComponent::AttrAnchorPosition() == attribute->getName())
		{
			this->setAnchorPosition(attribute->getVector3());
		}
		else if (JointWheelComponent::AttrPin() == attribute->getName())
		{
			this->setPin(attribute->getVector3());
		}
		else if (JointWheelComponent::AttrTargetSteeringAngle() == attribute->getName())
		{
			this->setTargetSteeringAngle(Ogre::Degree(attribute->getReal()));
		}
		else if (JointWheelComponent::AttrSteeringRate() == attribute->getName())
		{
			this->setSteeringRate(attribute->getReal());
		}
	}

	void JointWheelComponent::writeXML(rapidxml::xml_node<>* propertiesXML, rapidxml::xml_document<>& doc, const Ogre::String& filePath)
	{
		JointComponent::writeXML(propertiesXML, doc, filePath);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "AnchorPosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->anchorPosition->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Pin"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->pin->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TargetSteeringAngle"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->targetSteeringAngle->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SteeringRate"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->steeringRate->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::Vector3 JointWheelComponent::getUpdatedJointPosition(void)
	{
		if (nullptr == this->body)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[JointWheelComponent] Could not get 'getUpdatedJointPosition', because there is no body for game object: " + this->gameObjectPtr->getName());
			return Ogre::Vector3::ZERO;
		}

		if (false == this->hasCustomJointPosition)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			return this->body->getPosition() + offset;
		}
		else
		{
			return this->jointPosition;
		}
	}

	void JointWheelComponent::setAnchorPosition(const Ogre::Vector3& anchorPosition)
	{
		this->anchorPosition->setValue(anchorPosition);

		if (nullptr != this->body)
		{
			Ogre::Vector3 size = this->body->getAABB().getSize();
			//Ogre::Vector3 aPos = Ogre::Vector3(-this->anchorPosition.x, -this->anchorPosition.y, -this->anchorPosition.z);
			//// relative anchor pos: 0.5 0 0 means 50% in X of the size of the parent object
			//Ogre::Vector3 offset = (aPos * size);
			//// take orientation into account, so that orientated joints are processed correctly
			//this->jointPosition = this->body->getPosition() - (this->body->getOrientation() * offset);

			// without rotation
			Ogre::Vector3 offset = (this->anchorPosition->getVector3() * size);
			this->jointPosition = (this->body->getPosition() + offset);
			this->jointAlreadyCreated = false;
		}

		if (nullptr != this->debugGeometryNode)
		{
			this->debugGeometryNode->setPosition(this->jointPosition);
		}
	}

	Ogre::Vector3 JointWheelComponent::getAnchorPosition(void) const
	{
		return this->anchorPosition->getVector3();
	}

	void JointWheelComponent::setPin(const Ogre::Vector3& pin)
	{
		this->pin->setValue(pin);
		if (nullptr != this->debugGeometryNode)
		{
			this->debugGeometryNode->setDirection(pin);
		}
	}

	Ogre::Vector3 JointWheelComponent::getPin(void) const
	{
		return this->pin->getVector3();
	}

	Ogre::Real JointWheelComponent::getSteeringAngle(void) const
	{
		OgreNewt::Wheel* wheelJoint = dynamic_cast<OgreNewt::Wheel*>(this->joint);
		if (wheelJoint)
		{
			return wheelJoint->GetSteerAngle();
		}
		return 0.0f;
	}

	void JointWheelComponent::setTargetSteeringAngle(const Ogre::Degree& targetSteeringAngle)
	{
		this->targetSteeringAngle->setValue(targetSteeringAngle.valueDegrees());
		OgreNewt::Wheel* wheelJoint = dynamic_cast<OgreNewt::Wheel*>(this->joint);
		if (wheelJoint)
		{
			wheelJoint->SetTargetSteerAngle(targetSteeringAngle);
		}
	}

	Ogre::Degree JointWheelComponent::getTargetSteeringAngle(void) const
	{
		return Ogre::Degree(this->targetSteeringAngle->getReal());
	}

	void JointWheelComponent::setSteeringRate(Ogre::Real steeringRate)
	{
		this->steeringRate->setValue(steeringRate);
		OgreNewt::Wheel* wheelJoint = dynamic_cast<OgreNewt::Wheel*>(this->joint);
		if (wheelJoint)
		{
			wheelJoint->SetSteerRate(steeringRate);
		}
	}

	Ogre::Real JointWheelComponent::getSteeringRate(void) const
	{
		return this->steeringRate->getReal();
	}

	/*******************************JointFlexyPipeHandleComponent*******************************/

	JointFlexyPipeHandleComponent::JointFlexyPipeHandleComponent()
		: JointComponent()
	{
		this->type->setReadOnly(false);
		this->type->setValue(this->getClassName());
		this->type->setReadOnly(true);
		this->type->setDescription("This specialized handle can be used to connect joint flexy pipe spinner components to form a flexy pipe for rows etc.");
		this->pin = new Variant(JointFlexyPipeHandleComponent::AttrPin(), Ogre::Vector3(0.0f, 0.0f, 1.0f), this->attributes);

		this->activated->setVisible(false);
		this->bodyMassScale->setVisible(false);
		this->stiffness->setVisible(false);
		this->targetId->setVisible(false);
	}

	JointFlexyPipeHandleComponent::~JointFlexyPipeHandleComponent()
	{

	}

	GameObjectCompPtr JointFlexyPipeHandleComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		JointFlexyPipeHandleCompPtr clonedJointCompPtr(boost::make_shared<JointFlexyPipeHandleComponent>());

		clonedJointCompPtr->internalSetPriorId(this->id->getULong());
		clonedJointCompPtr->setType(this->type->getString());
		clonedJointCompPtr->setPredecessorId(this->predecessorId->getULong());
		clonedJointCompPtr->setTargetId(this->targetId->getULong());
		clonedJointCompPtr->setJointPosition(this->jointPosition);
		clonedJointCompPtr->setJointRecursiveCollisionEnabled(this->jointRecursiveCollision->getBool());
		clonedJointCompPtr->stiffness->setValue(this->stiffness->getReal());

		clonedJointCompPtr->setPin(this->pin->getVector3());

		clonedGameObjectPtr->addComponent(clonedJointCompPtr);
		clonedJointCompPtr->setOwner(clonedGameObjectPtr);

		clonedJointCompPtr->setActivated(this->activated->getBool());

		return clonedJointCompPtr;
	}

	bool JointFlexyPipeHandleComponent::init(xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		JointComponent::init(propertyElement, filename);
		
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "JointPin")
		{
			this->pin->setValue(XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3(0.0f, 0.0f, 0.0f)));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	bool JointFlexyPipeHandleComponent::postInit(void)
	{
		JointComponent::postInit();

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointFlexyPipeHandleComponent] Init joint flexy pipe handle component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool JointFlexyPipeHandleComponent::connect(void)
	{
		bool success = JointComponent::connect();
		return success;
	}

	bool JointFlexyPipeHandleComponent::disconnect(void)
	{
		bool success = JointComponent::disconnect();
		return success;
	}

	Ogre::String JointFlexyPipeHandleComponent::getClassName(void) const
	{
		return "JointFlexyPipeHandleComponent";
	}

	Ogre::String JointFlexyPipeHandleComponent::getParentClassName(void) const
	{
		return "JointComponent";
	}

	void JointFlexyPipeHandleComponent::update(Ogre::Real dt, bool notSimulating)
	{

	}

	void JointFlexyPipeHandleComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		JointComponent::writeXML(propertiesXML, doc, filePath);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "JointPin"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->pin->getVector3())));
		propertiesXML->append_node(propertyXML);
	}

	bool JointFlexyPipeHandleComponent::createJoint(const Ogre::Vector3& customJointPosition)
	{
		// Joint base created but not activated, return false, but its no error, hence after that return true.
		if (false == JointComponent::createJoint(customJointPosition))
		{
			return true;
		}

		if (nullptr == this->body)
		{
			this->connect();
		}
		if (Ogre::Vector3::ZERO == this->pin->getVector3())
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[JointPinComponent] Cannot create joint for: " + this->gameObjectPtr->getName() + " because the pin is zero.");
			return false;
		}

		OgreNewt::Body* predecessorBody = nullptr;
		if (this->predecessorJointCompPtr)
		{
			predecessorBody = this->predecessorJointCompPtr->getBody();
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[JointFlexyPipeHandleComponent] Creating flexy pipe handle joint for body1 name: "
				+ this->predecessorJointCompPtr->getOwner()->getName() + " body2 name: " + this->gameObjectPtr->getName());
		}
		
		this->jointPosition = this->body->getPosition();

		// Release joint each time, to create new one with new values
		this->internalReleaseJoint();

		Ogre::Vector3 dir = this->pin->getVector3();

		if (nullptr != predecessorBody)
		{
			dir = (this->body->getPosition() - predecessorBody->getPosition()).normalisedCopy();
		}
		this->joint = new OgreNewt::FlexyPipeHandleJoint(this->body, this->body->getOrientation() * dir);

		this->joint->setBodyMassScale(this->bodyMassScale->getVector2().x, this->bodyMassScale->getVector2().y);
		this->joint->setCollisionState(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		// this->body->setJointRecursiveCollision(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		
		return true;
	}

	void JointFlexyPipeHandleComponent::actualizeValue(Variant* attribute)
	{
		JointComponent::actualizeValue(attribute);

		if (JointFlexyPipeHandleComponent::AttrPin() == attribute->getName())
		{
			this->setPin(attribute->getVector3());
		}
	}

	void JointFlexyPipeHandleComponent::setPin(const Ogre::Vector3& pin)
	{
		this->pin->setValue(pin);
	}

	Ogre::Vector3 JointFlexyPipeHandleComponent::getPin(void) const
	{
		return this->pin->getVector3();
	}

	void JointFlexyPipeHandleComponent::setVelocity(const Ogre::Vector3& velocity, Ogre::Real dt)
	{
		if (nullptr == joint)
		{
			return;
		}
		OgreNewt::FlexyPipeHandleJoint* joint = static_cast<OgreNewt::FlexyPipeHandleJoint*>(this->joint);
		joint->setVelocity(velocity, dt);
	}

	void JointFlexyPipeHandleComponent::setOmega(const Ogre::Vector3& omega, Ogre::Real dt)
	{
		if (nullptr == joint)
		{
			return;
		}
		OgreNewt::FlexyPipeHandleJoint* joint = static_cast<OgreNewt::FlexyPipeHandleJoint*>(this->joint);
		joint->setOmega(omega, dt);
	}

	// Lua registration part

	JointFlexyPipeHandleComponent* getJointFlexyPipeHandleComponent(GameObject* gameObject)
	{
		return makeStrongPtr<JointFlexyPipeHandleComponent>(gameObject->getComponent<JointFlexyPipeHandleComponent>()).get();
	}

	JointFlexyPipeHandleComponent* getJointFlexyPipeHandleComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<JointFlexyPipeHandleComponent>(gameObject->getComponentFromName<JointFlexyPipeHandleComponent>(name)).get();
	}

	void JointFlexyPipeHandleComponent::createStaticApiForLua(lua_State* lua, luabind::class_<GameObject>& gameObject, luabind::class_<GameObjectController>& gameObjectController)
	{
		module(lua)
		[
			class_<JointFlexyPipeHandleComponent, GameObjectComponent>("JointFlexyPipeHandleComponent")
			.def("setPin", &JointFlexyPipeHandleComponent::setPin)
			.def("getPin", &JointFlexyPipeHandleComponent::getPin)
			.def("setVelocity", &JointFlexyPipeHandleComponent::setVelocity)
			.def("setOmega", &JointFlexyPipeHandleComponent::setOmega)
		];

		LuaScriptApi::getInstance()->addClassToCollection("JointFlexyPipeHandleComponent", "class inherits JointComponent", JointFlexyPipeHandleComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("JointFlexyPipeHandleComponent", "void setPin(Vector3 pin)", "Sets the pin (direction) of this joint.");
		LuaScriptApi::getInstance()->addClassToCollection("JointFlexyPipeHandleComponent", "Vector3 getPin()", "Gets the pin (direction) of this component.");
		LuaScriptApi::getInstance()->addClassToCollection("JointFlexyPipeHandleComponent", "void setVelocity(Vector3 velocity, float dt)", "Controls the motion of this joint by the given velocity vector and time step.");
		LuaScriptApi::getInstance()->addClassToCollection("JointFlexyPipeHandleComponent", "void setOmega(Vector3 omega, float dt)", "Controls the rotation of this joint by the given velocity vector and time step.");

		gameObject.def("getJointFlexyPipeHandleComponent", (JointFlexyPipeHandleComponent* (*)(GameObject*)) &getJointFlexyPipeHandleComponent);
		gameObject.def("getJointFlexyPipeHandleComponentFromName", &getJointFlexyPipeHandleComponentFromName);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "JointFlexyPipeHandleComponent getJointFlexyPipeHandleComponent()", "Gets the joint flexy pipe handle component. This can be used if the game object just has one joint flexy pipe handle component.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "JointFlexyPipeHandleComponent getJointFlexyPipeHandleComponentFromName(String name)", "Gets the joint flexy pipe handle component.");

		gameObjectController.def("castJointFlexyPipeHandleComponent", &GameObjectController::cast<JointFlexyPipeHandleComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "JointFlexyPipeHandleComponent castJointFlexyPipeHandleComponent(JointFlexyPipeHandleComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	/*******************************JointFlexyPipeSpinnerComponent*******************************/

	JointFlexyPipeSpinnerComponent::JointFlexyPipeSpinnerComponent()
		: JointComponent()
	{
		this->type->setReadOnly(false);
		this->type->setValue(this->getClassName());
		this->type->setReadOnly(true);
		this->type->setDescription("This specialized flexy pipe can be used in conjunction with a joint flexy pipe handle to form a flexy pipe for rows etc.");

		this->activated->setVisible(false);
		this->bodyMassScale->setVisible(false);
		this->stiffness->setVisible(false);
		this->targetId->setVisible(false);
	}

	JointFlexyPipeSpinnerComponent::~JointFlexyPipeSpinnerComponent()
	{

	}

	GameObjectCompPtr JointFlexyPipeSpinnerComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		JointFlexyPipeSpinnerCompPtr clonedJointCompPtr(boost::make_shared<JointFlexyPipeSpinnerComponent>());

		clonedJointCompPtr->internalSetPriorId(this->id->getULong());
		clonedJointCompPtr->setType(this->type->getString());
		clonedJointCompPtr->setPredecessorId(this->predecessorId->getULong());
		clonedJointCompPtr->setTargetId(this->targetId->getULong());
		clonedJointCompPtr->setJointPosition(this->jointPosition);
		clonedJointCompPtr->setJointRecursiveCollisionEnabled(this->jointRecursiveCollision->getBool());
		clonedJointCompPtr->stiffness->setValue(this->stiffness->getReal());

		// clonedJointCompPtr->setPin(this->pin->getVector3());

		clonedGameObjectPtr->addComponent(clonedJointCompPtr);
		clonedJointCompPtr->setOwner(clonedGameObjectPtr);

		clonedJointCompPtr->setActivated(this->activated->getBool());

		return clonedJointCompPtr;
	}

	bool JointFlexyPipeSpinnerComponent::init(xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		JointComponent::init(propertyElement, filename);
		
		return true;
	}

	bool JointFlexyPipeSpinnerComponent::postInit(void)
	{
		JointComponent::postInit();

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointFlexyPipeSpinnerComponent] Init joint flexy pipe spinner component for game object: " + this->gameObjectPtr->getName());

		return true;
	}

	bool JointFlexyPipeSpinnerComponent::connect(void)
	{
		bool success = JointComponent::connect();
		return success;
	}

	bool JointFlexyPipeSpinnerComponent::disconnect(void)
	{
		bool success = JointComponent::disconnect();
		return success;
	}

	Ogre::String JointFlexyPipeSpinnerComponent::getClassName(void) const
	{
		return "JointFlexyPipeSpinnerComponent";
	}

	Ogre::String JointFlexyPipeSpinnerComponent::getParentClassName(void) const
	{
		return "JointComponent";
	}

	void JointFlexyPipeSpinnerComponent::update(Ogre::Real dt, bool notSimulating)
	{

	}

	void JointFlexyPipeSpinnerComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		JointComponent::writeXML(propertiesXML, doc, filePath);

		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
	}

	bool JointFlexyPipeSpinnerComponent::createJoint(const Ogre::Vector3& customJointPosition)
	{
		// Joint base created but not activated, return false, but its no error, hence after that return true.
		if (false == JointComponent::createJoint(customJointPosition))
		{
			return true;
		}

		if (nullptr == this->body)
		{
			this->connect();
		}
		
		OgreNewt::Body* predecessorBody = nullptr;
		if (this->predecessorJointCompPtr)
		{
			predecessorBody = this->predecessorJointCompPtr->getBody();
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[JointFlexyPipeSpinnerComponent] Creating hinge joint for body1 name: "
				+ this->predecessorJointCompPtr->getOwner()->getName() + " body2 name: " + this->gameObjectPtr->getName());
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[JointFlexyPipeSpinnerComponent] Could not create flexy pipe spinner joint: " + this->gameObjectPtr->getName() + " because there is no predecessor body.");
			return false;
		}

		// Release joint each time, to create new one with new values
		this->internalReleaseJoint();

		this->jointPosition = this->body->getPosition();

		// Attention: Is this correct?
		Ogre::Vector3 direction = this->body->getPosition() - predecessorBody->getPosition();
		this->joint = new OgreNewt::FlexyPipeSpinnerJoint(this->body, predecessorBody, this->jointPosition, direction.normalisedCopy() /*this->body->getOrientation() * this->pin->getVector3()*/);

		this->joint->setBodyMassScale(this->bodyMassScale->getVector2().x, this->bodyMassScale->getVector2().y);
		this->joint->setCollisionState(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		// this->body->setJointRecursiveCollision(this->jointRecursiveCollision->getBool() == true ? 1 : 0);
		
		return true;
	}

	void JointFlexyPipeSpinnerComponent::actualizeValue(Variant* attribute)
	{
		JointComponent::actualizeValue(attribute);
	}

}; // namespace end
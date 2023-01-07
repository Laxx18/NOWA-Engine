#include "NOWAPrecompiled.h"
#include "PhysicsRagDollComponent.h"
#include "modules/OgreNewtModule.h"
#include "GameObjectController.h"
#include "utilities/XMLConverter.h"
#include "JointComponents.h"
#include "utilities/MathHelper.h"

namespace NOWA
{
	using namespace rapidxml;

	typedef boost::shared_ptr<JointComponent> JointCompPtr;
	typedef boost::shared_ptr<JointBallAndSocketComponent> JointBallAndSocketCompPtr;
	typedef boost::shared_ptr<JointHingeComponent> JointHingeCompPtr;
	typedef boost::shared_ptr<JointHingeActuatorComponent> JointHingeActuatorCompPtr;
	typedef boost::shared_ptr<JointUniversalComponent> JointUniversalCompPtr;

	// typedef boost::shared_ptr<RagDollMotorDofComponent> RagDollMotorDofCompPtr;
	
	PhysicsRagDollComponent::PhysicsRagDollComponent()
		: PhysicsActiveComponent(),
		rdState(PhysicsRagDollComponent::INACTIVE),
		rdOldState(PhysicsRagDollComponent::INACTIVE),
		skeleton(nullptr),
		animationEnabled(true),
		ragdollPositionOffset(Ogre::Vector3::ZERO),
		ragdollOrientationOffset(Ogre::Quaternion::IDENTITY),
		activated(new Variant(PhysicsRagDollComponent::AttrActivated(), false, this->attributes)),
		boneConfigFile(new Variant(PhysicsRagDollComponent::AttrBonesConfigFile(), Ogre::String(), this->attributes)),
		state(new Variant(PhysicsRagDollComponent::AttrState(), { "Inactive", "Animation", "Ragdolling", "PartialRagdolling" }, this->attributes)),
		isSimulating(false)
	{
		this->boneConfigFile->setDescription("Name of the XML config file, no path, e.g. FlubberRagdoll.rag");
		// Even if the value does not change, force resetting the same value, so that the script will re-parsed
		// this->boneConfigFile->setUserData("ForceSet");

		this->gyroscopicTorque->setValue(false);
		this->gyroscopicTorque->setVisible(false);
	}

	PhysicsRagDollComponent::~PhysicsRagDollComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsRagDollComponent] Destructor physics rag doll component for game object: " + this->gameObjectPtr->getName());
		while (this->ragDataList.size() > 0)
		{
			RagBone* ragBone = this->ragDataList.back().ragBone;
			delete ragBone;
			this->ragDataList.pop_back();
		}
	}

	bool PhysicsRagDollComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		PhysicsActiveComponent::parseCommonProperties(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "BoneConfigFile")
		{
			this->boneConfigFile->setValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (this->boneConfigFile->getString().length() == 0)
		{
			return false;
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "State")
		{
			Ogre::String stateCaption = XMLConverter::getAttrib(propertyElement, "data", "Inactive");
			this->state->setListSelectedValue(stateCaption);
			if (stateCaption == "Inactive")
			{
				this->rdState = PhysicsRagDollComponent::INACTIVE;
			}
			else if (stateCaption == "Animation")
			{
				this->rdState = PhysicsRagDollComponent::ANIMATION;
			}
			else if (stateCaption == "Ragdolling")
			{
				this->rdState = PhysicsRagDollComponent::RAGDOLLING;
				this->animationEnabled = false;
			}
			else if (stateCaption == "PartialRagdolling")
			{
				this->rdState = PhysicsRagDollComponent::PARTIAL_RAGDOLLING;
			}
			propertyElement = propertyElement->next_sibling("property");
		}

		// PhysicsActiveComponent::parseCompoundGameObjectProperties(propertyElement, filename);

		return true;
	}

	GameObjectCompPtr PhysicsRagDollComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		PhysicsRagDollCompPtr clonedCompPtr(boost::make_shared<PhysicsRagDollComponent>());

		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setMass(this->mass->getReal());
		clonedCompPtr->setMassOrigin(this->massOrigin->getVector3());
		clonedCompPtr->setLinearDamping(this->linearDamping->getReal());
		clonedCompPtr->setAngularDamping(this->angularDamping->getVector3());
		clonedCompPtr->setGravity(this->gravity->getVector3());
		clonedCompPtr->setGravitySourceCategory(this->gravitySourceCategory->getString());
		// clonedCompPtr->applyAngularImpulse(this->angularImpulse);
		// clonedCompPtr->applyAngularVelocity(this->angularVelocity->getVector3());
		clonedCompPtr->applyForce(this->force->getVector3());
		clonedCompPtr->setConstraintDirection(this->constraintDirection->getVector3());
		
		clonedCompPtr->setSpeed(this->speed->getReal());
		clonedCompPtr->setMaxSpeed(this->maxSpeed->getReal());
		// clonedCompPtr->setDefaultPoseName(this->defaultPoseName);
		clonedCompPtr->setCollisionType(this->collisionType->getListSelectedValue());
		// do not use constraintAxis variable, because its being manipulated during physics body creation
		// clonedCompPtr->setConstraintAxis(this->initConstraintAxis);

		clonedCompPtr->setCollisionDirection(this->collisionDirection->getVector3());
		
		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setBoneConfigFile(this->boneConfigFile->getString());
		clonedCompPtr->setState(this->state->getListSelectedValue());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);
		
		return clonedCompPtr;
	}

	bool PhysicsRagDollComponent::postInit(void)
	{
		bool success = PhysicsComponent::postInit();

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsRagDollComponent] Init physics rag doll component for game object: " + this->gameObjectPtr->getName());

		// Physics active component must be dynamic, else a mess occurs
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		return success;
	}

	void PhysicsRagDollComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();

		this->rdState = PhysicsRagDollComponent::INACTIVE;
		this->activated->setValue(false);
	}

	bool PhysicsRagDollComponent::createRagDoll(const Ogre::String& boneName)
	{
		this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
		this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
		this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();

		if (true == this->boneConfigFile->getString().empty())
			return true;

		Ogre::DataStreamPtr stream = Ogre::ResourceGroupManager::getSingleton().openResource(this->boneConfigFile->getString());
		if (stream.isNull())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponent] Could not open bone configuration file: "
				+ this->boneConfigFile->getString() + " for game object: " + this->gameObjectPtr->getName());
			return false;
		}

		TiXmlDocument document;
		// Get the file contents
		Ogre::String data = stream->getAsString();

		// Parse the XML
		document.Parse(data.c_str());
		stream->close();
		if (document.Error())
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponent] Could not parse one configuration file: " + this->boneConfigFile->getString()
				+ " for game object: " + this->gameObjectPtr->getName());
			return false;
		}

		TiXmlElement* root = document.RootElement();
		if (!root)
		{
			// error!
			Ogre::LogManager::getSingleton().logMessage("[PhysicsRagDollComponent] Error: cannot find 'root' in xml file: " + this->boneConfigFile->getString());
			return false;
		}

		// Check if the ragdoll config has a default pose
		Ogre::String defaultPose;
		if (root->Attribute("Pose"))
		{
			defaultPose = root->Attribute("Pose");
		}

		if (root->Attribute("PositionOffset"))
		{
			this->ragdollPositionOffset = Ogre::StringConverter::parseVector3(root->Attribute("PositionOffset"));
		}

		if (root->Attribute("OrientationOffset"))
		{
			Ogre::Vector3 dirOffset = Ogre::StringConverter::parseVector3(root->Attribute("OrientationOffset"));
			if (dirOffset != Ogre::Vector3::ZERO)
			{
				Ogre::Matrix3 rot;
				rot.FromEulerAnglesXYZ(Ogre::Degree(dirOffset.x), Ogre::Degree(dirOffset.y), Ogre::Degree(dirOffset.z));
				this->ragdollOrientationOffset.FromRotationMatrix(rot);
			}
		}

		// Get the skeleton.
		Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
		if (nullptr == entity)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponent] Error cannot create rag doll body, because the game object has no entity with mesh for game object: " + this->gameObjectPtr->getName());
			return false;
		}

		this->skeleton = entity->getSkeleton();
		// entity->setAlwaysUpdateMainSkeleton(true);
		// entity->setUpdateBoundingBoxFromSkeleton(true);

		// Get the mesh.
		this->mesh = entity->getMesh();

		Ogre::v1::AnimationState* animState = nullptr;
		if (defaultPose.length() > 0)
		{
			// Get the default pose animation state (ideally the T-Pose)
			animState = entity->getAnimationState(defaultPose);
			animState->setEnabled(true);
			animState->setLoop(false);
			animState->setTimePosition(100.0f);
			entity->_updateAnimation(); //critical! read this functions comments!
		}

		// First delete all prior created ragbones
		while (this->ragDataList.size() > 0)
		{
			RagBone* ragBone = this->ragDataList.back().ragBone;
			delete ragBone;
			this->ragDataList.pop_back();
		}

		if (nullptr != this->rootJointCompPtr)
		{
			this->rootJointCompPtr.reset();
		}

		// Parse rag doll data from xml
		bool success = this->createRagDollFromXML(root, boneName);
		if (false == success)
		{
			return false;
		}

		if (nullptr != animState)
		{
			animState->setEnabled(false);
		}

		this->physicsBody->setUserData(OgreNewt::Any(dynamic_cast<PhysicsComponent*>(this)));
		this->physicsBody->setType(this->gameObjectPtr->getCategoryId());
		this->physicsBody->setMaterialGroupID(GameObjectController::getInstance()->getMaterialID(this->gameObjectPtr.get(), this->ogreNewt));

		return true;
	}

	/*
	bool OgreRagdollOX3D::SetRagdollManualBoneAnim(String boneName, bool active)
{
	Ogre::Bone *bone;

	if (rSkeleton->hasBone(boneName))
	{
		bone = rSkeleton->getBone(boneName);
		bone->setManuallyControlled(active);
		rRagdollBoneControl = active;

		Ogre::AnimationStateIterator animations = rRagdollMesh->GetEntity()->getAllAnimationStates()->getAnimationStateIterator();

		while (animations.hasMoreElements())
		{
			Ogre::AnimationState *a = animations.getNext();
			if (active)
			{
				a->createBlendMask(rSkeleton->getNumBones(), 1);
				a->setBlendMaskEntry(bone->getHandle(), !active);
			}
			else {
				a->destroyBlendMask();
			}
		}
		return true;
	}
	else return false;
}
	*/

	bool PhysicsRagDollComponent::connect(void)
	{
		bool success = PhysicsActiveComponent::connect();

		this->isSimulating = true;

		this->internalApplyState();

		return success;
	}

	bool PhysicsRagDollComponent::disconnect(void)
	{
		bool success = PhysicsActiveComponent::disconnect();

		this->isSimulating = false;
		// Must be set manually because, only at connect, all data for debug is available (bodies, joints)
		this->internalShowDebugData(false);

		if (this->rdState == PhysicsRagDollComponent::RAGDOLLING || this->rdState == PhysicsRagDollComponent::PARTIAL_RAGDOLLING)
		{
			this->endRagdolling();
		}
		return success;
	}

	Ogre::String PhysicsRagDollComponent::getClassName(void) const
	{
		return "PhysicsRagDollComponent";
	}

	Ogre::String PhysicsRagDollComponent::getParentClassName(void) const
	{
		return "PhysicsActiveComponent";
	}

	Ogre::String PhysicsRagDollComponent::getParentParentClassName(void) const
	{
		return "PhysicsComponent";
	}
	
	void PhysicsRagDollComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);

		if (this->rdState != PhysicsRagDollComponent::INACTIVE || this->rdState != PhysicsRagDollComponent::ANIMATION)
		{
			for (auto& it = this->ragDataList.begin(); it != this->ragDataList.end(); ++it)
			{
				if (true == activated)
					(*it).ragBone->getBody()->unFreeze();
				else
					(*it).ragBone->getBody()->freeze();
			}
		}
	}

	void PhysicsRagDollComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (true == activated->getBool() && false == notSimulating)
		{
			if (this->rdState == PhysicsRagDollComponent::RAGDOLLING)
			{
				this->applyRagdollStateToModel();
			}
			else if (this->rdState == PhysicsRagDollComponent::PARTIAL_RAGDOLLING)
			{
				this->applyRagdollStateToModel();
			}

			if (this->rdState != PhysicsRagDollComponent::INACTIVE || this->rdState != PhysicsRagDollComponent::ANIMATION)
			{
				for (auto& it = this->ragDataList.begin(); it != this->ragDataList.end(); ++it)
				{
					(*it).ragBone->update(dt, notSimulating);
				}
			}
		}
	}

	void PhysicsRagDollComponent::actualizeValue(Variant* attribute)
	{
		PhysicsActiveComponent::actualizeCommonValue(attribute);

		// PhysicsActiveComponent::actualizeCompoundGameObjectValue(attribute);
		
		if (PhysicsRagDollComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (PhysicsRagDollComponent::AttrBonesConfigFile() == attribute->getName())
		{
			this->setBoneConfigFile(attribute->getString());
		}
		else if (PhysicsRagDollComponent::AttrState() == attribute->getName())
		{
			this->setState(attribute->getListSelectedValue());
		}
	}

	void PhysicsRagDollComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		PhysicsActiveComponent::writeCommonProperties(propertiesXML, doc);
		
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->activated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BoneConfigFile"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->boneConfigFile->getString())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "State"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->state->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		// PhysicsActiveComponent::writeCompoundGameObjectProperties(propertiesXML, doc);
	}

	void PhysicsRagDollComponent::showDebugData(void)
	{
		GameObjectComponent::showDebugData();
	}

	void PhysicsRagDollComponent::internalShowDebugData(bool activate)
	{
		// Complex cases: Since everything is done manually, when user pressed e.g. debug button, debug data can only be shown, when go has been connected
		// On disconnect, the debug data must be hidden in any case, because almost everything will be destroyed
		if (false == this->ragDataList.empty())
		{
			for (auto& it = this->ragDataList.begin(); it != this->ragDataList.end(); ++it)
			{
				if (true == activate)
				{
					(*it).ragBone->getBody()->showDebugCollision(false, this->bShowDebugData && activate);
					if (nullptr != (*it).ragBone->getJointComponent())
					{
						(*it).ragBone->getJointComponent()->showDebugData();
						(*it).ragBone->getJointComponent()->forceShowDebugData(this->bShowDebugData && activate);

					}

				}
				else
				{
					(*it).ragBone->getBody()->showDebugCollision(false, false);
					if (nullptr != (*it).ragBone->getJointComponent())
					{
						(*it).ragBone->getJointComponent()->showDebugData();
						(*it).ragBone->getJointComponent()->forceShowDebugData(false);
					}
				}
			}
		}
		else
		{
			if (nullptr != this->physicsBody)
			{
				if (true == activate)
					this->physicsBody->showDebugCollision(false, this->bShowDebugData && activate);
				else
					this->physicsBody->showDebugCollision(false, false);
			}
		}
	}

	void PhysicsRagDollComponent::internalApplyState(void)
	{
		if (true == this->isSimulating)
		{
			// First deactivate debug data if set
			this->internalShowDebugData(false);

			if (this->rdState == PhysicsRagDollComponent::RAGDOLLING)
			{
				this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
				this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
				this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();

				this->partialRagdollBoneName = "";
				this->releaseConstraintDirection();
				this->releaseConstraintAxis();
				// Must be placed, else after one time disconnect, a crash occurs
				this->destroyBody();
				this->createRagDoll(this->partialRagdollBoneName);
				this->startRagdolling();
			}
			else if (this->rdState == PhysicsRagDollComponent::PARTIAL_RAGDOLLING)
			{
				this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
				if (this->rdOldState == PhysicsRagDollComponent::RAGDOLLING)
				{
					// Set a bit higher, so that the collision hull may be created above the ground, if changed from ragdoll to inactive
					this->initialPosition += Ogre::Vector3(0.0f, this->gameObjectPtr->getSize().y * 0.5f, 0.0f);
				}
				this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
				this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();

				this->releaseConstraintDirection();
				this->releaseConstraintAxis();
				// Must be placed, else after one time disconnect, a crash occurs
				this->destroyBody();

				this->createRagDoll(this->partialRagdollBoneName);

				// this->createDynamicBody();

				this->startRagdolling();
			}
			else if (this->rdState == PhysicsRagDollComponent::INACTIVE)
			{
				this->partialRagdollBoneName = "";
				this->endRagdolling();
				this->releaseConstraintDirection();
				this->releaseConstraintAxis();
				this->destroyBody();
				
				this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
				if (this->rdOldState == PhysicsRagDollComponent::RAGDOLLING)
				{
					// Set a bit higher, so that the collision hull may be created above the ground, if changed from ragdoll to inactive
					this->initialPosition += Ogre::Vector3(0.0f, this->gameObjectPtr->getSize().y * 0.5f, 0.0f);
				}
				this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
				// Only use initial orientation, if the old state was not ragdolling, because else, the player is rotated in wrong manner and constraint axis, up vector will be incorrect
				if (this->rdOldState != PhysicsRagDollComponent::RAGDOLLING)
				{
					this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();
				}

				this->createDynamicBody();
			}

			// Must be set manually because, only at connect, all data for debug is available (bodies, joints)
			this->internalShowDebugData(true);
		}
	}

	void PhysicsRagDollComponent::setBoneConfigFile(const Ogre::String& boneConfigFile)
	{
		this->boneConfigFile->setValue(boneConfigFile);
	}

	Ogre::String PhysicsRagDollComponent::getBoneConfigFile(void) const
	{
		return this->boneConfigFile->getString();
	}

	void PhysicsRagDollComponent::setVelocity(const Ogre::Vector3& velocity)
	{
		if (true == this->isSimulating)
		{
			/*if (this->rdState == PhysicsRagDollComponent::RAGDOLLING)
			{
				for (auto& it = this->ragDataList.begin(); it != this->ragDataList.end(); ++it)
				{
					OgreNewt::Body* body = (*it).ragBone->getBody();
					body->setVelocity(velocity);
				}
			}
			else if (this->rdState == PhysicsRagDollComponent::PARTIAL_RAGDOLLING)
			{
				for (auto& it = this->ragDataList.begin() + 1; it != this->ragDataList.end(); ++it)
				{
					OgreNewt::Body* body = (*it).ragBone->getBody();
					body->setVelocity(velocity);
				}
			}
			else*/
			{
				this->physicsBody->setVelocity(velocity);
			}
		}
	}

	Ogre::Vector3 PhysicsRagDollComponent::getVelocity(void) const
	{
		if (true == this->isSimulating)
		{
			if (this->rdState == PhysicsRagDollComponent::RAGDOLLING)
			{
				return this->ragDataList[0].ragBone->getBody()->getVelocity();
			}
			else
			{
				if (nullptr != this->physicsBody)
				{
					return this->physicsBody->getVelocity();
				}
			}
		}
		return Ogre::Vector3::ZERO;
	}

	void PhysicsRagDollComponent::setPosition(Ogre::Real x, Ogre::Real y, Ogre::Real z)
	{
		if (this->rdState == PhysicsRagDollComponent::RAGDOLLING)
		{
			if (false == this->isSimulating)
			{
				this->gameObjectPtr->getSceneNode()->setPosition(x, y, z);
			}
		}
		else
		{
			PhysicsComponent::setPosition(x, y, z);
		}
	}

	void PhysicsRagDollComponent::setPosition(const Ogre::Vector3& position)
	{
		if (this->rdState == PhysicsRagDollComponent::RAGDOLLING)
		{
			if (false == this->isSimulating)
			{
				this->gameObjectPtr->getSceneNode()->setPosition(position);
			}
		}
		else
		{
			PhysicsComponent::setPosition(position);
		}
	}

	void PhysicsRagDollComponent::translate(const Ogre::Vector3& relativePosition)
	{
		// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsRagDollComponent] pos1: " + Ogre::StringConverter::toString(relativePosition));
		if (this->rdState == PhysicsRagDollComponent::RAGDOLLING)
		{
			if (false == this->isSimulating)
			{
				this->gameObjectPtr->getSceneNode()->translate(relativePosition);
			}
		}
		else
		{
			PhysicsComponent::translate(relativePosition);
		}
	}

	Ogre::Vector3 PhysicsRagDollComponent::getPosition(void) const
	{
		if (false == this->isSimulating || nullptr == this->physicsBody)
			return this->gameObjectPtr->getPosition();
		else
			return this->physicsBody->getPosition();
	}

	void PhysicsRagDollComponent::setOrientation(const Ogre::Quaternion& orientation)
	{
		if (this->rdState == PhysicsRagDollComponent::RAGDOLLING || this->rdState == PhysicsRagDollComponent::PARTIAL_RAGDOLLING)
		{
			if (false == this->isSimulating)
			{
				this->gameObjectPtr->getSceneNode()->setOrientation(orientation);
			}
			else
			{
				size_t offset = 0;
				// When partial ragdolling is active, the first bone is the main body and should not apply to rag doll!
				/*if (this->rdState == PhysicsRagDollComponent::PARTIAL_RAGDOLLING)
				{
					offset = 1;
				}
				for (auto& it = this->ragDataList.cbegin() + offset; it != this->ragDataList.cend(); ++it)
				{
					it->ragBone->setOrientation(orientation);
				}*/
				PhysicsComponent::setOrientation(orientation);
			}
		}
		else
		{
			PhysicsComponent::setOrientation(orientation);
		}
	}

	void PhysicsRagDollComponent::rotate(const Ogre::Quaternion& relativeRotation)
	{
		if (this->rdState == PhysicsRagDollComponent::RAGDOLLING)
		{
			if (false == this->isSimulating)
			{
				this->gameObjectPtr->getSceneNode()->rotate(relativeRotation);
			}
		}
		else
		{
			PhysicsComponent::rotate(relativeRotation);
		}
	}

	Ogre::Quaternion PhysicsRagDollComponent::getOrientation(void) const
	{
		if (false == this->isSimulating || nullptr == this->physicsBody)
			return this->gameObjectPtr->getOrientation();
		else
			return this->physicsBody->getOrientation();
	}
	
	void PhysicsRagDollComponent::setBoneRotation(const Ogre::String& boneName, const Ogre::Vector3& axis, Ogre::Real degree)
	{
		this->oldPartialRagdollBoneName = boneName;
		this->partialRagdollBoneName = boneName;
		this->rdOldState = this->rdState;
		this->rdState = PhysicsRagDollComponent::PARTIAL_RAGDOLLING;

		RagBone* ragBone = this->getRagBone(boneName);
		ragBone->getBody()->setTorque(axis * (degree * Ogre::Math::PI / 180.0f));
		/*JointUniversalCompPtr jointCompPtr = boost::dynamic_pointer_cast<JointUniversalComponent>(ragBone->getJointComponent());
		jointCompPtr->setMotorEnabled(true);
		jointCompPtr->setMotorSpeed(10.0f);*/
	}

	void PhysicsRagDollComponent::setInitialState(void)
	{
		size_t offset = 0;
		// When partial ragdolling is active, the first bone is the main body and should not apply to rag doll!
		if (this->rdState == PhysicsRagDollComponent::PARTIAL_RAGDOLLING)
		{
			offset = 1;
		}

		for (auto& it = this->ragDataList.cbegin() + offset; it != this->ragDataList.cend(); ++it)
		{
			it->ragBone->setInitialState();
		}
	}

	void PhysicsRagDollComponent::setAnimationEnabled(bool animationEnabled)
	{
		this->animationEnabled = animationEnabled;

		size_t offset = 0;
		// When partial ragdolling is active, the first bone is the main body and should not apply to rag doll!
		if (this->rdState == PhysicsRagDollComponent::PARTIAL_RAGDOLLING)
		{
			offset = 1;
		}

		for (auto& it = this->ragDataList.cbegin() + offset; it != this->ragDataList.cend(); ++it)
		{
			it->ragBone->getOgreBone()->setManuallyControlled(!animationEnabled);
			it->ragBone->applyPose(Ogre::Vector3::ZERO);
		}
	}

	bool PhysicsRagDollComponent::isAnimationEnabled(void) const
	{
		return this->animationEnabled;
	}

	bool PhysicsRagDollComponent::createRagDollFromXML(TiXmlElement* rootXmlElement, const Ogre::String& boneName)
	{
		// add all children of this bone.
		RagBone* parentRagBone = nullptr;

		TiXmlElement* bonesXmlElement = rootXmlElement->FirstChildElement("Bones");
		if (nullptr == bonesXmlElement)
		{
			Ogre::LogManager::getSingleton().logMessage("[PhysicsRagDollComponent] Error: cannot find 'Bones' XML element in xml file: " + this->boneConfigFile->getString());
			return false;
		}

		bool partial = false;
		bool first = true;
		if (PhysicsRagDollComponent::PARTIAL_RAGDOLLING == this->rdState)
		{
			partial = true;
		}

		TiXmlElement* boneXmlElement = bonesXmlElement->FirstChildElement("Bone");
		while (nullptr != boneXmlElement)
		{
			// Check if there is a source, that points to a target for position correction
			if (boneXmlElement->Attribute("Source"))
			{
				Ogre::String sourceBoneName = boneXmlElement->Attribute("Source");
				if (false == boneName.empty())
				{
					// Found bone name at which all other targets should be parsed, so parse til the bone name is found and proceed as usual after that
					// So that a partial ragdoll is build
					if (sourceBoneName != boneName)
					{
						boneXmlElement = boneXmlElement->NextSiblingElement("Bone");
						continue;
					}
				}
				Ogre::String targetBoneName = boneXmlElement->Attribute("Target");
				Ogre::Vector3 offset = Ogre::StringConverter::parseVector3(boneXmlElement->Attribute("Offset"));
				Ogre::v1::OldBone* sourceBone = this->skeleton->getBone(sourceBoneName);
				Ogre::v1::OldBone* targetBone = this->skeleton->getBone(targetBoneName);
				this->boneCorrectionMap.emplace(sourceBone, std::make_pair(targetBone, offset));

				boneXmlElement = boneXmlElement->NextSiblingElement("Bone");
				continue;
			}
			
			// get the information for the bone represented by this element.
			Ogre::Vector3 size = Ogre::StringConverter::parseVector3(boneXmlElement->Attribute("Size"));

			Ogre::String skeletonBone = boneXmlElement->Attribute("SkeletonBone");

			// Partial ragdolling root bone can be left off, so that position and orientation is taken from whole mesh (position, orientation)
			// This is useful, because when attaching to a root bone, it may be that the bone is somewhat feeble and so would be the whole character!
			if(skeletonBone.empty() && (false == first || false == partial))
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponent] Error: Skeleton bone name could not be loaded from XML for: "
					+ this->gameObjectPtr->getName());
				return false;
			}

			Ogre::v1::OldBone* ogreBone = nullptr;
			if (false == skeletonBone.empty())
			{
				try
				{
					ogreBone = this->skeleton->getBone(skeletonBone);
				}
				catch (const Ogre::Exception&)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponent] Error: Skeleton bone name: " + skeletonBone + " for game object: "
						+ this->gameObjectPtr->getName());
					return false;
				}
			}

			Ogre::String boneNameFromFile;
			Ogre::String strName = boneXmlElement->Attribute("Name");
			if (!strName.empty())
			{
				boneNameFromFile = strName;
			}
			else
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponent] Error: Representation name could not be loaded from XML for: "
					+ this->gameObjectPtr->getName());
				return false;
			}

			Ogre::Vector3 offset = Ogre::Vector3::ZERO;

			if (boneXmlElement->Attribute("Offset"))
			{
				offset = Ogre::StringConverter::parseVector3(boneXmlElement->Attribute("Offset"));
			}

			Ogre::String strShape = boneXmlElement->Attribute("Shape");
			PhysicsRagDollComponent::RagBone::BoneShape shape = PhysicsRagDollComponent::RagBone::BS_BOX;

			if (strShape == "Hull")
			{
				shape = PhysicsRagDollComponent::RagBone::BS_CONVEXHULL;
			}
			else if (strShape == "Box")
			{
				shape = PhysicsRagDollComponent::RagBone::BS_BOX;
			}
			else if (strShape == "Capsule")
			{
				shape = PhysicsRagDollComponent::RagBone::BS_CAPSULE;
			}
			else if (strShape == "Cylinder")
			{
				shape = PhysicsRagDollComponent::RagBone::BS_CYLINDER;
			}
			else if (strShape == "Cone")
			{
				shape = PhysicsRagDollComponent::RagBone::BS_CONE;
			}
			else if (strShape == "Ellipsoid")
			{
				shape = PhysicsRagDollComponent::RagBone::BS_ELLIPSOID;
			}

			Ogre::Real mass = Ogre::StringConverter::parseReal(boneXmlElement->Attribute("Mass"));
			// this->mass += mass;

			///////////////////////////////////////////////////////////////////////////////

			RagBone* currentRagBone = new RagBone(boneNameFromFile, this, parentRagBone, ogreBone, this->mesh, constraintDirection->getVector3(), shape, size, mass, partial, offset);

			// add a name from file, if it exists, because it could be, that someone that rigged the mesh, did not name it properly, so this can be done in the *.xml file
			
			PhysicsRagDollComponent::RagData ragData;
			ragData.ragBoneName = boneNameFromFile;
			ragData.ragBone = currentRagBone;
			this->ragDataList.emplace_back(ragData);
			
			boneXmlElement = boneXmlElement->NextSiblingElement("Bone");
			// store parent rag bone pointer
			parentRagBone = currentRagBone;

			first = false;
		}

		TiXmlElement* jointsXmlElement = rootXmlElement->FirstChildElement("Joints");
		if (nullptr == jointsXmlElement)
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponent] Error: cannot find 'Joints' XML element in xml file: " + this->boneConfigFile->getString());
			return false;
		}
		
		// Go through all joints
		TiXmlElement* jointXmlElement = jointsXmlElement->FirstChildElement("Joint");
		while (nullptr != jointXmlElement)
		{
			Ogre::String ragBoneChildName = jointXmlElement->Attribute("Child");
			Ogre::String ragBoneParentName = jointXmlElement->Attribute("Parent");

			RagBone* childRagBone = this->getRagBone(ragBoneChildName);
			RagBone* parentRagBone = this->getRagBone(ragBoneParentName);

			if (nullptr == childRagBone)
			{
				Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponent] Warning: There is no child rag bone name: "
					+ ragBoneChildName + " specified for game object " + this->gameObjectPtr->getName());
			}
			/*if (nullptr == parentRagBone)
			{
				Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponent] Warning: There is no parent rag bone name: "
					+ ragBoneParentName + " specified for game object " + this->gameObjectPtr->getName());
			}*/

			Ogre::Real friction = -1.0f;

			if (jointXmlElement->Attribute("Friction"))
			{
				friction = Ogre::StringConverter::parseReal(jointXmlElement->Attribute("Friction"));
			}

			Ogre::String strJointType = jointXmlElement->Attribute("Type");
			PhysicsRagDollComponent::JointType jointType = PhysicsRagDollComponent::JT_BALLSOCKET;
			Ogre::Vector3 pin = Ogre::Vector3::ZERO;

			if (strJointType == JointBallAndSocketComponent::getStaticClassName())
			{
				jointType = PhysicsRagDollComponent::JT_BALLSOCKET;
			}
			else if (strJointType == JointHingeComponent::getStaticClassName())
			{
				jointType = PhysicsRagDollComponent::JT_HINGE;
				if (jointXmlElement->Attribute("Pin"))
				{
					pin = Ogre::StringConverter::parseVector3(jointXmlElement->Attribute("Pin"));
				}
				else
				{
					Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponent] Error: Pin for hinge joint is missing in "
						+ this->boneConfigFile->getString() + " for bone: '" + childRagBone->getName()
						+ "' for game object " + this->gameObjectPtr->getName());
					return false;
				}
			}
			else if (strJointType == JointUniversalComponent::getStaticClassName())
			{
				jointType = PhysicsRagDollComponent::JT_DOUBLE_HINGE;
			}
			else if (strJointType == JointHingeActuatorComponent::getStaticClassName())
			{
				jointType = PhysicsRagDollComponent::JT_HINGE_ACTUATOR;
				if (jointXmlElement->Attribute("Pin"))
				{
					pin = Ogre::StringConverter::parseVector3(jointXmlElement->Attribute("Pin"));
				}
				else
				{
					Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponent] Error: Pin for hinge actuator joint is missing in "
						+ this->boneConfigFile->getString() + " for bone: '" + childRagBone->getName()
						+ "' for game object " + this->gameObjectPtr->getName());
					return false;
				}
			}

			Ogre::Degree minTwistAngle = Ogre::Degree(Ogre::StringConverter::parseReal(jointXmlElement->Attribute("MinTwistAngle")));
			Ogre::Degree maxTwistAngle = Ogre::Degree(Ogre::StringConverter::parseReal(jointXmlElement->Attribute("MaxTwistAngle")));
			Ogre::Degree minTwistAngle2 = Ogre::Degree(0.0f);
			Ogre::Degree maxTwistAngle2 = Ogre::Degree(0.0f);

			Ogre::Degree maxConeAngle = Ogre::Degree(0.0f);

			if (jointXmlElement->Attribute("MaxConeAngle"))
			{
				maxConeAngle = Ogre::Degree(Ogre::StringConverter::parseReal(jointXmlElement->Attribute("MaxConeAngle")));
			}
			if (jointXmlElement->Attribute("MinTwistAngle2"))
			{
				minTwistAngle2 = Ogre::Degree(Ogre::StringConverter::parseReal(jointXmlElement->Attribute("MinTwistAngle2")));
			}
			if (jointXmlElement->Attribute("MaxTwistAngle2"))
			{
				maxTwistAngle2 = Ogre::Degree(Ogre::StringConverter::parseReal(jointXmlElement->Attribute("MaxTwistAngle2")));
			}

			if (nullptr == jointXmlElement->Attribute("Child"))
			{
				Ogre::LogManager::getSingleton().logMessage("[PhysicsRagDollComponent] Error: Cannot find 'Child' XML element in xml file: " + this->boneConfigFile->getString());
				return false;
			}
			if (nullptr == jointXmlElement->Attribute("Parent"))
			{
				Ogre::LogManager::getSingleton().logMessage("[PhysicsRagDollComponent] Error: Cannot find 'Parent' XML element in xml file: " + this->boneConfigFile->getString());
				return false;
			}

			bool useSpring = false;

			if (jointXmlElement->Attribute("Spring"))
			{
				useSpring = Ogre::StringConverter::parseBool(jointXmlElement->Attribute("Spring"));
			}

			Ogre::Vector3 offset = Ogre::Vector3::ZERO;
			if (jointXmlElement->Attribute("Offset"))
			{
				offset = Ogre::StringConverter::parseVector3(jointXmlElement->Attribute("Offset"));
			}

			// if (this->rdState != PhysicsRagDollComponent::RAGDOLLING)
			// {
			/*currentRagBone->setInitialPose(jointPos);
			currentRagBone->applyPose(jointPos);*/
			// this->applyPose(jpin);
			// }

			if (nullptr != childRagBone && nullptr != parentRagBone)
				this->joinBones(jointType, childRagBone, parentRagBone, pin, minTwistAngle, maxTwistAngle, maxConeAngle, minTwistAngle2, maxTwistAngle2, friction, useSpring, offset);

			jointXmlElement = jointXmlElement->NextSiblingElement("Joint");
		}
		return true;
	}

	void PhysicsRagDollComponent::joinBones(PhysicsRagDollComponent::JointType type, RagBone* childRagBone, RagBone* parentRagBone, const Ogre::Vector3& pin,
		const Ogre::Degree& minTwistAngle, const Ogre::Degree& maxTwistAngle, const Ogre::Degree& maxConeAngle, const Ogre::Degree& minTwistAngle2, 
		const Ogre::Degree& maxTwistAngle2, Ogre::Real friction, bool useSpring, const Ogre::Vector3& offset)
	{
		// http://newtondynamics.com/wiki/index.php5?title=Joints
		
		Ogre::Vector3 jointPin = Ogre::Vector3::ZERO;
		
		JointCompPtr parentJointCompPtr;
		JointCompPtr childJointCompPtr;

		// if the parent bone has no joint handler, create a default one (this one of course does not join anything, its just a placeholder to connect joint handlers together,
		// because a joint connection is just required for 2 bones)

		if (this->rdState == PhysicsRagDollComponent::PARTIAL_RAGDOLLING)
		{

			

		}

		if (nullptr == parentRagBone->getJointComponent())
		{

			parentJointCompPtr = boost::dynamic_pointer_cast<JointComponent>(parentRagBone->createJointComponent(JointComponent::getStaticClassName()));
			parentJointCompPtr->setBody(parentRagBone->getBody());
			GameObjectController::getInstance()->addJointComponent(parentJointCompPtr);
		}
		else
		{
			parentJointCompPtr = parentRagBone->getJointComponent();
		}
			
		/*if (nullptr != parentRagBone)
		{
			if (nullptr != parentRagBone->getOgreBone())
				jointPin = (this->gameObjectPtr->getSceneNode()->_getDerivedOrientation() * parentRagBone->getOgreBone()->_getDerivedOrientation()) * pin;
			else
				jointPin = this->gameObjectPtr->getSceneNode()->_getDerivedOrientation() * pin;
		}
		else
		{
			jointPin = (this->gameObjectPtr->getSceneNode()->_getDerivedOrientation() * this->getOrientation()) * pin;
		}*/

		// jointPin = childRagBone->getOgreBone()->_getDerivedOrientation() * pin;

		// Note this->getOrientation() etc. not required since when hinge is create orientation is also taken into account!
		jointPin = pin;
		
		switch(type)
		{
			case PhysicsRagDollComponent::JT_BALLSOCKET:
				{
					JointBallAndSocketCompPtr tempChildCompPtr = boost::dynamic_pointer_cast<JointBallAndSocketComponent>(
						childRagBone->createJointComponent(JointBallAndSocketComponent::getStaticClassName()));
				
					// tempChildCompPtr->setDofCount(2);
					// tempChildCompPtr->setMotorOn(true);
					tempChildCompPtr->setBody(childRagBone->getBody());
					tempChildCompPtr->setAnchorPosition(offset);
					// tempChildCompPtr->setDryFriction(0.5f);
					// Set predecessor id and body manually, since ragdoll joints are not part of the game object controller's controlling
					tempChildCompPtr->setPredecessorId(parentJointCompPtr->getId());
					tempChildCompPtr->connectPredecessorCompPtr(parentJointCompPtr);

					tempChildCompPtr->setConeLimitsEnabled(true);
					tempChildCompPtr->setTwistLimitsEnabled(true);
					tempChildCompPtr->setMinMaxConeAngleLimit(minTwistAngle, maxTwistAngle, maxConeAngle);
					if (-1.0f != friction)
					{
						tempChildCompPtr->setConeFriction(friction);
						tempChildCompPtr->setTwistFriction(friction);
					}
					tempChildCompPtr->createJoint();
					
					tempChildCompPtr->setJointRecursiveCollisionEnabled(false);

					childJointCompPtr = tempChildCompPtr;
					// Add also to GOC, so that other joints can be connected from the outside, but also create joint here, so that the body position has not changed
					// Because GOC will create later and ragdoll has been applied and body pos changed!
					GameObjectController::getInstance()->addJointComponent(tempChildCompPtr);
				}
				break;
			case PhysicsRagDollComponent::JT_HINGE:
				{
					JointHingeCompPtr tempChildCompPtr = boost::dynamic_pointer_cast<JointHingeComponent>(
						childRagBone->createJointComponent(JointHingeComponent::getStaticClassName()));
					
					// tempChildCompPtr->setDofCount(1);
					// tempChildCompPtr->setMotorOn(true);
					tempChildCompPtr->setBody(childRagBone->getBody());
					tempChildCompPtr->setAnchorPosition(offset);
					// Set predecessor id and body manually, since ragdoll joints are not part of the game object controller's controlling
					tempChildCompPtr->setPredecessorId(parentJointCompPtr->getId());
					tempChildCompPtr->connectPredecessorCompPtr(parentJointCompPtr);

					tempChildCompPtr->setPin(jointPin);
				
					tempChildCompPtr->setLimitsEnabled(true);
					tempChildCompPtr->setMinMaxAngleLimit(minTwistAngle, maxTwistAngle);
					tempChildCompPtr->setSpring(useSpring);
					// tempChildCompPtr->setMinMaxConeAngleLimit(minTwistAngle, maxTwistAngle, maxConeAngle);
					if (-1.0f != friction)
					{
						tempChildCompPtr->setFriction(friction);
					}
					tempChildCompPtr->createJoint();
					
					tempChildCompPtr->setJointRecursiveCollisionEnabled(false);

					childJointCompPtr = tempChildCompPtr;
					GameObjectController::getInstance()->addJointComponent(tempChildCompPtr);
				}
				break;
				case PhysicsRagDollComponent::JT_HINGE_ACTUATOR:
				{
					JointHingeActuatorCompPtr tempChildCompPtr = boost::dynamic_pointer_cast<JointHingeActuatorComponent>(
						childRagBone->createJointComponent(JointHingeActuatorComponent::getStaticClassName()));
					
					// tempChildCompPtr->setDofCount(1);
					// tempChildCompPtr->setMotorOn(true);
					tempChildCompPtr->setBody(childRagBone->getBody());
					tempChildCompPtr->setAnchorPosition(offset);
					// Set predecessor id and body manually, since ragdoll joints are not part of the game object controller's controlling
					tempChildCompPtr->setPredecessorId(parentJointCompPtr->getId());
					tempChildCompPtr->connectPredecessorCompPtr(parentJointCompPtr);

					tempChildCompPtr->setPin(jointPin);
				
					tempChildCompPtr->setMinAngleLimit(minTwistAngle);
					tempChildCompPtr->setMaxAngleLimit(maxTwistAngle);
					tempChildCompPtr->setSpring(useSpring);

					tempChildCompPtr->createJoint();

					tempChildCompPtr->setJointRecursiveCollisionEnabled(false);

					childJointCompPtr = tempChildCompPtr;
					GameObjectController::getInstance()->addJointComponent(tempChildCompPtr);
				}
				break;
			case PhysicsRagDollComponent::JT_DOUBLE_HINGE:
				{
					JointUniversalCompPtr tempChildCompPtr = boost::dynamic_pointer_cast<JointUniversalComponent>(
						childRagBone->createJointComponent(JointUniversalComponent::getStaticClassName()));

					// tempChildCompPtr->setDofCount(1);
					// tempChildCompPtr->setMotorOn(true);
					tempChildCompPtr->setBody(childRagBone->getBody());
					tempChildCompPtr->setAnchorPosition(offset);
					// Set predecessor id and body manually, since ragdoll joints are not part of the game object controller's controlling
					tempChildCompPtr->setPredecessorId(parentJointCompPtr->getId());
					tempChildCompPtr->connectPredecessorCompPtr(parentJointCompPtr);

					tempChildCompPtr->setLimits0Enabled(true);
					tempChildCompPtr->setMinMaxAngleLimit1(minTwistAngle, maxTwistAngle);
					tempChildCompPtr->setLimits1Enabled(true);
					tempChildCompPtr->setMinMaxAngleLimit0(minTwistAngle2, maxTwistAngle2);
					tempChildCompPtr->setSpring0(useSpring);
					tempChildCompPtr->setSpring1(useSpring);
					// tempChildCompPtr->setMinMaxConeAngleLimit(minTwistAngle, maxTwistAngle, maxConeAngle);
					if (-1.0f != friction)
					{
						tempChildCompPtr->setFriction0(friction);
						tempChildCompPtr->setFriction1(friction);
					}
					tempChildCompPtr->createJoint();

					tempChildCompPtr->setJointRecursiveCollisionEnabled(false);

					childJointCompPtr = tempChildCompPtr;
					GameObjectController::getInstance()->addJointComponent(tempChildCompPtr);
				}
				break;
		}

		if (nullptr != childJointCompPtr)
		{
			if (nullptr != parentRagBone)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsRagDollComponent]: Name: " + this->gameObjectPtr->getName()
					+ " child, joint pos: " + Ogre::StringConverter::toString(childJointCompPtr->getJointPosition()) + " between child: " + childRagBone->getName() 
					+ " and parent: " + parentRagBone->getName()
					+ ", joint pos: " + Ogre::StringConverter::toString(parentJointCompPtr->getJointPosition())
					+ ", joint pin: " + Ogre::StringConverter::toString(jointPin));
			}
			else
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsRagDollComponent]: Name: " + this->gameObjectPtr->getName()
					+ " child joint pos: " + Ogre::StringConverter::toString(childJointCompPtr->getJointPosition()) + " between child: " + childRagBone->getName()
					+ " and parent: root, joint pos: " + Ogre::StringConverter::toString(parentJointCompPtr->getJointPosition())
					+ ", joint pin: " + Ogre::StringConverter::toString(jointPin));
			}
		}
	}

	void PhysicsRagDollComponent::inheritVelOmega(Ogre::Vector3 vel, Ogre::Vector3 omega)
	{
		// find main position.
		Ogre::Vector3 mainpos = this->gameObjectPtr->getSceneNode()->_getDerivedPosition();

		size_t offset = 0;
		// When partial ragdolling is active, the first bone is the main body and should not apply to rag doll!
		if (this->rdState == PhysicsRagDollComponent::PARTIAL_RAGDOLLING)
		{
			offset = 1;
		}

		for (auto& it = this->ragDataList.cbegin() + offset; it != this->ragDataList.cend(); ++it)
		{
			Ogre::Vector3 pos;
			Ogre::Quaternion orient;

			it->ragBone->getBody()->getPositionOrientation(pos, orient);
			it->ragBone->getBody()->setVelocity(vel + omega.crossProduct(pos - mainpos));
		}
	}

	void PhysicsRagDollComponent::setState(const Ogre::String& state)
	{
		this->state->setListSelectedValue(state);

		this->rdOldState = this->rdState;

		if (state == "Inactive")
		{
			if (this->rdState == PhysicsRagDollComponent::INACTIVE)
				return;

			this->rdState = PhysicsRagDollComponent::INACTIVE;
		}
		else if (state == "Animation")
		{
			if (this->rdState == PhysicsRagDollComponent::ANIMATION)
				return;
			this->rdState = PhysicsRagDollComponent::ANIMATION;
		}
		else if (state == "Ragdolling")
		{
			if (this->rdState == PhysicsRagDollComponent::RAGDOLLING)
				return;
			this->rdState = PhysicsRagDollComponent::RAGDOLLING;
			this->animationEnabled = false;
		}
		else if (state == "PartialRagdolling")
		{
			// Check if rotation bone name has changed, if this is the case partial ragdoll must be re-created
			if (this->rdState == PhysicsRagDollComponent::PARTIAL_RAGDOLLING && this->oldPartialRagdollBoneName == this->partialRagdollBoneName)
				return;
			this->rdState = PhysicsRagDollComponent::PARTIAL_RAGDOLLING;
			// this->animationEnabled = false;
		}

		if (true == this->isSimulating)
		{
			this->internalApplyState();
		}
	}

	Ogre::String PhysicsRagDollComponent::getState(void) const
	{
		return this->state->getListSelectedValue();
	}

	OgreNewt::Body* PhysicsRagDollComponent::getBody(void) const
	{
		return this->physicsBody;
	}

	const std::vector<PhysicsRagDollComponent::RagData>& PhysicsRagDollComponent::getRagDataList(void)
	{
		return this->ragDataList;
	}

	PhysicsRagDollComponent::RagBone* PhysicsRagDollComponent::getRagBone(const Ogre::String& name)
	{
		if (name.empty())
		{
			return nullptr;
		}
		for (auto& it = this->ragDataList.cbegin(); it != this->ragDataList.cend(); ++it)
		{
			if (name == it->ragBoneName)
			{
				return it->ragBone;
			}
		}
		return nullptr;
	}

	void PhysicsRagDollComponent::applyModelStateToRagdoll(void)
	{
		Ogre::Vector3 scale = this->gameObjectPtr->getSceneNode()->getScale();

		size_t i = 0;

		if (this->rdState == PhysicsRagDollComponent::PARTIAL_RAGDOLLING)
		{
			i = 1;
		}

		for (; i < this->ragDataList.size(); i++)
		{
			Ogre::Vector3 position;
			Ogre::Quaternion orientation;

			auto& ragBone = this->ragDataList[i].ragBone;
			const auto node = this->gameObjectPtr->getSceneNode();

			ragBone->getBody()->getPositionOrientation(position, orientation);
			// Take also ragdollPositionOffset into account. That is, each ragdoll's root bone must be approx. in the center of the entity
			// so maybe it must be translated. But this translation offset may mess up with the bones, e.g. when the root bone is translated by y=-0.986, the
			ragBone->getBody()->setPositionOrientation(
				node->_getDerivedPosition() + this->ragdollPositionOffset + (node->_getDerivedOrientation() * (ragBone->getWorldPosition() + this->ragdollPositionOffset)) * scale,

				node->_getDerivedOrientation() * ragBone->getWorldOrientation() * ragBone->getInitialBoneOrientation().Inverse() * orientation);
		}

		// Set constraint axis for root body, after the final position of the bodies has been set
		this->setConstraintAxis(this->constraintAxis->getVector3());

		if (this->rdState == PhysicsRagDollComponent::RAGDOLLING)
		{
			// Release constraint axis pin, so that ragdoll can be rotated on any axis, but maybe is still clipped to plane
			this->releaseConstraintAxisPin();
		}
		else if (this->rdState == PhysicsRagDollComponent::PARTIAL_RAGDOLLING)
		{
			// Pin the object stand in pose and not fall down
			this->setConstraintDirection(this->constraintDirection->getVector3());
		}
	}

	void PhysicsRagDollComponent::applyRagdollStateToModel(void)
	{
		if (true == this->ragDataList.empty())
			return;

		size_t i = 0;

		if (this->rdState == PhysicsRagDollComponent::PARTIAL_RAGDOLLING)
		{
			i = 1;
		}

		for (; i < this->ragDataList.size(); i++)
		{
			auto& ragBone = this->ragDataList[i].ragBone;

			ragBone->getOgreBone()->setOrientation(this->gameObjectPtr->getSceneNode()->_getDerivedOrientation().Inverse() *
				ragBone->getBody()->getOrientation() * ragBone->getInitialBodyOrientation().Inverse() * ragBone->getInitialBoneOrientation());
		}

		for (auto& boneCorrectionData : this->boneCorrectionMap)
		{
			// Set the source bone position to the target bone position + offset
			// This can be necessary, if a bone e.g. on a foot is messed up and is streched when a ragdoll is moved, so force the messed up bone position and orientation
			// to a nearby bones position and orientation
			// Correct, do no touch!
			boneCorrectionData.first->setPosition(boneCorrectionData.second.first->_getDerivedPosition() + boneCorrectionData.second.second);
			boneCorrectionData.first->setOrientation(boneCorrectionData.second.first->_getDerivedOrientation());
		}
	}
	
	void PhysicsRagDollComponent::startRagdolling(void)
	{
		// this->skeleton->reset();

		// Reset all animations. Note this must be done, because a game object may be in the middle of an animation and this would mess up with the ragdolling
		Ogre::v1::AnimationStateSet* set = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>()->getAllAnimationStates();
		Ogre::v1::AnimationStateIterator it = set->getAnimationStateIterator();
		
		while (it.hasMoreElements())
		{
			Ogre::v1::AnimationState* anim = it.getNext();
			anim->setEnabled(false);
			anim->setWeight(0);
			anim->setTimePosition(0);
		}

		// http://wiki.ogre3d.org/Ragdolls?highlight=ragdoll
		this->applyModelStateToRagdoll();

		if (true == this->ragDataList.empty())
			return;

		if (nullptr == this->skeleton)
			return;
		
		Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();

		unsigned short appliedCount = 0;

		if (this->rdState == PhysicsRagDollComponent::PARTIAL_RAGDOLLING)
		{
			if (false == this->ragDataList.empty())
			{
				this->ragDataList.front().ragBone->attachToNode();
				appliedCount = 1;
			}

			// Set bones to manually controlled and get their initial orientations too
			for (unsigned short i = 0; i < this->skeleton->getNumBones(); i++)
			{
				Ogre::v1::OldBone* bone = this->skeleton->getBone(i);

				for (size_t j = appliedCount; j < this->ragDataList.size(); j++)
				{
					// Only apply when a bone is part of the partial rag doll
					if (bone == this->ragDataList[j].ragBone->getOgreBone())
					{
						// Get the absolute world orientation
						Ogre::Quaternion absoluteWorldOrientation = bone->_getDerivedOrientation();
						
						// Set inherit orientation to false
						bone->setManuallyControlled(true);
						bone->setInheritOrientation(false);
							
						// Set the absolute world orientation
						bone->setOrientation(absoluteWorldOrientation);

						this->ragDataList[j].ragBone->attachToNode();

						appliedCount++;
						break;
					}
				}

				if (appliedCount >= this->ragDataList.size())
				{
					break;
				}
			}
		}
		else
		{
			for (unsigned short i = 0; i < this->skeleton->getNumBones(); i++)
			{
				Ogre::v1::OldBone* bone = this->skeleton->getBone(i);
				// Get the absolute world orientation
				Ogre::Quaternion absoluteWorldOrientation = bone->_getDerivedOrientation();
				// Set inherit orientation to false
				bone->setManuallyControlled(true);
				bone->setInheritOrientation(false);
				// Set the absolute world orientation
				bone->setOrientation(absoluteWorldOrientation);
				// Attach all rag bones to node
				if (i < this->ragDataList.size())
				{
					this->ragDataList[i].ragBone->attachToNode();
				}
			}
		}

// Attention: https://forums.ogre3d.org/viewtopic.php?t=71135

		/*Ogre::v1::OldBone* bone = this->skeleton->getBone("Toe.R");
		Ogre::Quaternion orientation = bone->_getDerivedOrientation();
		bone->setManuallyControlled(true);
		bone->setInheritOrientation(false);
		bone->setOrientation(orientation);
		*/
		//We don't want the model to just fall down as if it were standing still when it died, it should appear to continue moving in the direction it was when it was killed.
		// In addition to this we have been supplied with a 'death vector' from the killing blow that will hurry the model onwards.
		// m_bodies[BODYPART_HEAD]->applyCentralImpulse(btVector3(BtOgre::Convert::toBullet(getHeading() * 10 + 10 * m_DeathVector)));
		// m_bodies[BODYPART_SPINE]->applyCentralImpulse(btVector3(BtOgre::Convert::toBullet(getHeading() * 20 + 10 * m_DeathVector)));
		// m_bodies[BODYPART_HIPS]->applyCentralImpulse(btVector3(BtOgre::Convert::toBullet(getHeading() * 15 + 10 * m_DeathVector)));

		// Ogre::Vector3 lastBloodPosition = this->gameObjectPtr->getSceneNode()->_getDerivedPosition();
	}

	void PhysicsRagDollComponent::endRagdolling(void)
	{
		// this->applyRagdollStateToModel();

		if (nullptr == this->skeleton)
			return;

		// Reset all bones to initial state
		//for (unsigned int i = 0; i < this->skeleton->getNumBones(); i++)
		//{
		//	Ogre::v1::OldBone* bone = this->skeleton->getBone(i);
		//	bone->setManuallyControlled(false);
		//	bone->setInheritOrientation(true); // Does rotate bones wrong when starting ragdoll again!
		//	bone->resetToInitialState();
		//	//if (i < this->ragDataList.size())
		//	//{
		//	//	// Reset bone position and orientation
		//	//	// this->ragDataList[i].ragBone->setInitialState(); // Does rotate bones wrong when starting ragdoll again!
		//	//}
		//}

		while (this->ragDataList.size() > 0)
		{
			RagBone* ragBone = this->ragDataList.back().ragBone;
			delete ragBone;
			this->ragDataList.pop_back();
		}
		this->ragDataList.clear();

		if (nullptr != this->rootJointCompPtr)
		{
			this->rootJointCompPtr.reset();
		}

		this->skeleton->unload();
		// this->skeleton->reload();
		this->skeleton->load();

		Ogre::v1::AnimationStateSet* set = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>()->getAllAnimationStates();
		Ogre::v1::AnimationStateIterator it = set->getAnimationStateIterator();

		while (it.hasMoreElements())
		{
			Ogre::v1::AnimationState* anim = it.getNext();
			anim->setEnabled(false);
			anim->setWeight(0);
			anim->setTimePosition(0);
		}
	}

	/*********************************Inner bone class*****************************************************************/

	PhysicsRagDollComponent::RagBone::RagBone(const Ogre::String& name, PhysicsRagDollComponent* physicsRagDollComponent, RagBone* parentRagBone, Ogre::v1::OldBone* ogreBone, 
		Ogre::v1::MeshPtr mesh, const Ogre::Vector3& pose, RagBone::BoneShape shape, Ogre::Vector3& size, Ogre::Real mass, bool partial, const Ogre::Vector3& offset)
		: name(name),
		physicsRagDollComponent(physicsRagDollComponent),
		parentRagBone(parentRagBone),
		ogreBone(ogreBone),
		body(nullptr),
		upVector(nullptr),
		pose(pose),
		bodySize(size),
		partial(partial),
		sceneNode(nullptr),
		initialBoneOrientation(Ogre::Quaternion::IDENTITY),
		initialBonePosition(Ogre::Vector3::ZERO),
		initialBodyOrientation(Ogre::Quaternion::IDENTITY),
		initialBodyPosition(Ogre::Vector3::ZERO),
		initialPose(Ogre::Vector3::ZERO),
		forceForVelocity(Ogre::Vector3::ZERO),
		canAddForceForVelocity(false)
	{
		OgreNewt::CollisionPtr collisionPtr;
		Ogre::Vector3 inertia;
		Ogre::Vector3 massOrigin;

		// Partial ragdolling root bone can be left off, so that position and orientation is taken from whole mesh (position, orientation)
		// This is useful, because when attaching to a root bone, it may be that the bone is somewhat feeble and so would be the whole character!

		// Store the initial orientation and position
		if (true == partial && nullptr == parentRagBone)
		{
			this->initialBoneOrientation = this->physicsRagDollComponent->gameObjectPtr->getSceneNode()->_getDerivedOrientation();
			// this->ogreBone->_setDerivedOrientation(this->initialBoneOrientation);
			this->initialBonePosition = this->physicsRagDollComponent->gameObjectPtr->getSceneNode()->_getDerivedPosition();
		}
		else
		{
			this->initialBoneOrientation = this->ogreBone->_getDerivedOrientation();
			this->initialBonePosition = this->ogreBone->_getDerivedPosition();

			
			// Scaling must be applied, else if a huge scaled down character is used, a bone could be at position 4 60 -19
			this->initialBonePosition *= this->physicsRagDollComponent->gameObjectPtr->getSceneNode()->getScale();
			this->initialBonePosition = this->physicsRagDollComponent->gameObjectPtr->getSceneNode()->_getDerivedOrientation() * this->initialBonePosition;

			this->initialBoneOrientation = this->physicsRagDollComponent->gameObjectPtr->getSceneNode()->_getDerivedOrientation() * this->initialBoneOrientation;

			// Set bone local position and add to world position of the character
			this->initialBonePosition += this->physicsRagDollComponent->gameObjectPtr->getSceneNode()->_getDerivedPosition() /*+ Ogre::Vector3(0.2f, 0.0f, 0.2f)*/;

			// Ogre::LogManager::getSingletonPtr()->logMessage("wp: " 
			// 	+ Ogre::StringConverter::toString(initialBonePosition) + " lp: " + Ogre::StringConverter::toString(this->ogreBone->getPosition() * this->physicsRagDollComponent->gameObjectPtr->getSceneNode()->getScale()));
		}

		// Bones can have really weird nearby zero values, so trunc those ugly values
		if (Ogre::Math::RealEqual(this->initialBonePosition.x, 0.0f))
		{
			this->initialBonePosition.x = 0.0f;
		}
		if (Ogre::Math::RealEqual(this->initialBonePosition.y, 0.0f))
		{
			this->initialBonePosition.y = 0.0f;
		}
		if (Ogre::Math::RealEqual(this->initialBonePosition.z, 0.0f))
		{
			this->initialBonePosition.z = 0.0f;
		}

		if (Ogre::Math::RealEqual(this->initialBoneOrientation.x, 0.0f))
		{
			this->initialBoneOrientation.x = 0.0f;
		}
		if (Ogre::Math::RealEqual(this->initialBoneOrientation.y, 0.0f))
		{
			this->initialBoneOrientation.y = 0.0f;
		}
		if (Ogre::Math::RealEqual(this->initialBoneOrientation.z, 0.0f))
		{
			this->initialBoneOrientation.z = 0.0f;
		}
		if (Ogre::Math::RealEqual(this->initialBoneOrientation.w, 0.0f))
		{
			this->initialBoneOrientation.w = 0.0f;
		}

		// http://ogre3d.org/forums/viewtopic.php?f=2&t=26595&start=0
		// http://www.ogre3d.org/addonforums/viewtopic.php?f=3&t=4476
		// http://www.ogre3d.org/tikiwiki/Manual+Resource+Loading --> bone init state etc. examples
		Ogre::Quaternion collisionOrientation = Ogre::Quaternion::IDENTITY;
		Ogre::Vector3 collisionPosition = Ogre::Vector3(0.0f, size.x / 2.0f, 0.0f) + offset;
		Ogre::Matrix3 rot;

		rot.FromEulerAnglesXYZ(Ogre::Degree(0.0f), Ogre::Degree(0.0f), Ogre::Degree(270.0f));
		collisionOrientation.FromRotationMatrix(rot);

		// Create the corresponding collision shape
		switch (shape)
		{
			case PhysicsRagDollComponent::RagBone::BS_BOX:
			{
				OgreNewt::CollisionPrimitives::Box* col = new OgreNewt::CollisionPrimitives::Box(this->physicsRagDollComponent->ogreNewt, size, 0, collisionOrientation, collisionPosition);
				col->calculateInertialMatrix(inertia, massOrigin);
				
				collisionPtr = OgreNewt::CollisionPtr(col);
				break;
			}
			case PhysicsRagDollComponent::RagBone::BS_CAPSULE:
			{
				OgreNewt::CollisionPrimitives::Capsule* col = new OgreNewt::CollisionPrimitives::Capsule(this->physicsRagDollComponent->ogreNewt, size.y, size.x, 0, 
					collisionOrientation, collisionPosition);
				
				col->calculateInertialMatrix(inertia, massOrigin);
				collisionPtr = OgreNewt::CollisionPtr(col);
				break;
			}
			case PhysicsRagDollComponent::RagBone::BS_CONE:
			{
				OgreNewt::CollisionPrimitives::Cone* col = new OgreNewt::CollisionPrimitives::Cone(this->physicsRagDollComponent->ogreNewt, size.y, size.x, 0,
					collisionOrientation, collisionPosition);
				
				col->calculateInertialMatrix(inertia, massOrigin);
				collisionPtr = OgreNewt::CollisionPtr(col);
				break;
			}
			case PhysicsRagDollComponent::RagBone::BS_CYLINDER:
			{
				OgreNewt::CollisionPrimitives::Cylinder* col = new OgreNewt::CollisionPrimitives::Cylinder(this->physicsRagDollComponent->ogreNewt, size.y, size.x, 0,
					collisionOrientation, collisionPosition);
				
				col->calculateInertialMatrix(inertia, massOrigin);
				collisionPtr = OgreNewt::CollisionPtr(col);
				break;
			}
			case PhysicsRagDollComponent::RagBone::BS_ELLIPSOID:
			{
				OgreNewt::CollisionPrimitives::Ellipsoid* col = new OgreNewt::CollisionPrimitives::Ellipsoid(this->physicsRagDollComponent->ogreNewt, size, 0, collisionOrientation, collisionPosition);
				col->calculateInertialMatrix(inertia, massOrigin);
				
				collisionPtr = OgreNewt::CollisionPtr(col);
				break;
			}
			case PhysicsRagDollComponent::RagBone::BS_CONVEXHULL:
			{
				if (nullptr != this->ogreBone)
				{
					collisionPtr = this->physicsRagDollComponent->getWeightedBoneConvexHull(this->ogreBone, mesh, size.x, inertia, massOrigin, Ogre::Vector3::ZERO, Ogre::Quaternion::IDENTITY,
						this->physicsRagDollComponent->gameObjectPtr->getSceneNode()->getScale());
				}
				else
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponent] Error: Cannot create a convex hull for partial ragdoll with no bone for game object: "
						+ this->physicsRagDollComponent->getOwner()->getName());
					throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[PhysicsRagDollComponent] Error: Cannot create a convex hull for partial ragdoll with no bone for game object: "
						+ this->physicsRagDollComponent->getOwner()->getName() + "\n", "NOWA");
				}
				break;
			}
		}

		this->body = new OgreNewt::Body(this->physicsRagDollComponent->ogreNewt, this->physicsRagDollComponent->gameObjectPtr->getSceneManager(), collisionPtr);

		this->body->setGravity(this->physicsRagDollComponent->gravity->getVector3());

		// Set the body position and orientation to bone + an offset orientation specified in the XML file
		this->body->setPositionOrientation(this->initialBonePosition, this->initialBoneOrientation);

		/* if (nullptr != this->ogreBone)
		 {
			 Ogre::String ragBoneName = this->ogreBone->getName();
			 Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsRagDollComponent] Creating rag bone: "
		 		+ ragBoneName + " for game object: " + this->physicsRagDollComponent->getOwner()->getName() + " position: " + Ogre::StringConverter::toString(this->initialBonePosition));
		 }
		 else
		 {
			 Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsRagDollComponent] Creating rag bone root: "
		 		+  this->physicsRagDollComponent->getOwner()->getName() 
				 + " for game object: " + this->physicsRagDollComponent->getOwner()->getName() + " position: " + Ogre::StringConverter::toString(this->initialBonePosition));
		 }*/


		if (this->physicsRagDollComponent->linearDamping->getReal() != 0.0f)
		{
			this->body->setLinearDamping(this->physicsRagDollComponent->linearDamping->getReal());
		}

		if (this->physicsRagDollComponent->angularDamping->getVector3() != Ogre::Vector3::ZERO)
		{
			this->body->setAngularDamping(this->physicsRagDollComponent->angularDamping->getVector3());
		}

		inertia *= mass;

		if (Ogre::Math::RealEqual(massOrigin.x, 0.0f))
		{
			massOrigin.x = 0.0f;
		}
		if (Ogre::Math::RealEqual(massOrigin.y, 0.0f))
		{
			massOrigin.y = 0.0f;
		}
		if (Ogre::Math::RealEqual(massOrigin.z, 0.0f))
		{
			massOrigin.z = 0.0f;
		}

		this->body->setMassMatrix(mass, inertia);
		this->body->setCenterOfMass(massOrigin);

		this->initialBodyPosition = this->initialBonePosition;
		this->initialBodyOrientation = this->initialBoneOrientation;

		// Important: Set the type and material group id for each piece the same as the root body for material pair functionality and collision callbacks
		this->body->setType(this->physicsRagDollComponent->getOwner()->getCategoryId());
		this->body->setMaterialGroupID(GameObjectController::getInstance()->getMaterialID(this->physicsRagDollComponent->getOwner().get(), this->physicsRagDollComponent->ogreNewt));

		// Here correct? Usage of PhysicsActiveComponent callback will only apply on the root physics body, maybe extend so that each ragbone uses this callback?
		// if (nullptr == this->parentRagBone)
		// if (false == this->partial)
		if (nullptr != this->parentRagBone)
		{
			this->body->setCustomForceAndTorqueCallback<PhysicsRagDollComponent::RagBone>(&PhysicsRagDollComponent::RagBone::moveCallback, this);
		}

		// pin the object stand in pose and not fall down
		/*if (this->physicsRagDollComponent->pose != Ogre::Vector3::ZERO) {
			this->physicsRagDollComponent->applyPose(this->physicsRagDollComponent->pose);
			}*/

		/*if (this->pose != Ogre::Vector3::ZERO) {
			this->physicsRagDollComponent->applyPose(this->pose);
			}*/

		if (false == this->physicsRagDollComponent->activated->getBool())
		{
			this->body->freeze();
		}
		else
		{
			this->body->unFreeze();
		}

		// if this is the parent just assign the root node
		if (nullptr == this->parentRagBone)
		{
			this->sceneNode = this->physicsRagDollComponent->gameObjectPtr->getSceneNode();
			this->physicsRagDollComponent->physicsBody = this->body;
		}
		else
		{
			// else create a child scene node
			this->sceneNode = this->physicsRagDollComponent->gameObjectPtr->getSceneNode()->createChildSceneNode();
			this->sceneNode->setName(name);
			this->sceneNode->getUserObjectBindings().setUserAny(Ogre::Any(this->physicsRagDollComponent->gameObjectPtr.get()));
		}

		if (nullptr == this->parentRagBone && true == this->partial)
		{
			this->physicsRagDollComponent->setContinuousCollision(this->physicsRagDollComponent->continuousCollision->getBool());
			this->physicsRagDollComponent->setGyroscopicTorqueEnabled(this->physicsRagDollComponent->gyroscopicTorque->getBool());

			this->body->setCustomForceAndTorqueCallback<PhysicsRagDollComponent>(&PhysicsActiveComponent::moveCallback, this->physicsRagDollComponent);
		}
	}

	PhysicsRagDollComponent::RagBone::~RagBone()
	{
		this->deleteRagBone();
	}

	void PhysicsRagDollComponent::RagBone::moveCallback(OgreNewt::Body* body, Ogre::Real timeStep, int threadIndex)
	{
		/////////////////////Standard gravity force/////////////////////////////////////

		// Dangerous: Causes newton crash!
		// Clamp omega, when activated (should only be rarely the case!)
		/*Ogre::Vector3 omega = body->getOmega();
		Ogre::Real mag2 = omega.dotProduct(omega);
		if (mag2 > (50.0f * 50.0f))
		{
			omega = omega.normalise() * 50.0f;
			body->setOmega(omega);
		}*/

		Ogre::Vector3 wholeForce = this->physicsRagDollComponent->getGravity();
		Ogre::Real mass = 0.0f;
		Ogre::Vector3 inertia = Ogre::Vector3::ZERO;

		// calculate gravity
		body->getMassMatrix(mass, inertia);
		wholeForce *= mass;

		body->addForce(wholeForce);

		if (true == this->canAddForceForVelocity)
		{
			Ogre::Vector3 currentVelocity = body->getVelocity();
			Ogre::Real delay = 0.1f;
			Ogre::Vector3 moveForce = (this->forceForVelocity - currentVelocity) * mass / delay;

			body->addForce(moveForce);
			this->forceForVelocity = Ogre::Vector3::ZERO;
			this->canAddForceForVelocity = false;
		}
	}

	void PhysicsRagDollComponent::RagBone::deleteRagBone(void)
	{
		// Dangerous: It does mess up position, orientation of bone when stopping/restarting simulation! Hence un-commented!
		// this->ogreBone->setInheritOrientation(false);
		// this->ogreBone->setManuallyControlled(false);

		if (nullptr != this->ogreBone)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsRagDollComponent::RagBone] Deleting ragbone: " + this->ogreBone->getName());
		}
		if (this->body)
		{
			if (this->jointCompPtr)
			{
				GameObjectController::getInstance()->removeJointComponent(this->jointCompPtr->getId());
				// This crashes!
				// jointCompPtr->releaseJoint();

				jointCompPtr.reset();
			}
			
			this->body->detachNode();
			// Do not delete the root body, because it will be deleted automatically when this component will be deleted
			if (this->body != this->physicsRagDollComponent->physicsBody)
			{
				this->body->removeForceAndTorqueCallback();
				this->body->removeNodeUpdateNotify();
				this->body->removeDestructorCallback();
				delete this->body;
				this->body = nullptr;
			}
			else
			{
				// if its the physics body of the component, just set this one to null
				this->body = nullptr;
			}
		}
		if (this->sceneNode)
		{
			if (nullptr != this->parentRagBone)
			{
				this->physicsRagDollComponent->gameObjectPtr->getSceneNode()->removeAndDestroyChild(this->sceneNode);
			}
			else
			{
				this->sceneNode = nullptr;
			}
		}
	}

	void PhysicsRagDollComponent::RagBone::update(Ogre::Real dt, bool notSimulating)
	{
		if (nullptr != this->jointCompPtr && false == notSimulating)
			this->jointCompPtr->update(dt);
		
		
		//// if animation is not enabled, control the bones depending on the physics hulls
		//if (!this->physicsRagDollComponent->animationEnabled)
		//{

		//	Ogre::Quaternion localOrient;
		//	Ogre::Vector3 localPos;
		//	this->body->getPositionOrientation(localPos, localOrient);
		//	// For example: wrist->forearm->arm_l->neck, that means ordering is important, parent of wrist is forearm_l, parent of rorearm_l is arm_l
		//	// if this is the root bone, get the offset offset position and orientation to the model nodes position and orientation that had been determined at line 360
		//	if (!this->getParentRagBone())
		//	{

		//		Ogre::Quaternion finalorient = (localOrient * this->offsetOrient);
		//		Ogre::Vector3 finalpos = localPos + (localOrient * this->offsetPos);
		//		// the whole ragdoll will be physically translated and rotated
		//		this->body->setPositionOrientation(finalpos, finalorient);

		//	}
		//	else
		//	{ // if this is the root orientate the node
		//		// standard bone, calculate the local orientation between it and it's parent.
		//		Ogre::Quaternion parentOrientation;
		//		Ogre::Vector3 parentPosition;

		//		this->getParentRagBone()->getBody()->getPositionOrientation(parentPosition, parentOrientation);


		//		Ogre::Quaternion localorient = parentOrientation.Inverse() * localOrient;
		//		// only the bone will be physicaly rotated
		//		this->ogreBone->setOrientation(localorient);
		//	}
		//	// do the opposite, controll the physics hulls depending on the bones
		//}
		//else
		//{
		//	/*Ogre::Quaternion localOrient = this->ogreBone->_getDerivedOrientation();
		//	Ogre::Vector3 localPos = this->ogreBone->_getDerivedPosition();

		//	if (this->parentRagBone != NULL) {

		//	this->body->setPositionOrientation(localPos, localOrient);

		//	} else { // if this is the root orientate the node

		//	Ogre::Quaternion finalOrient = (localOrient * this->offsetOrient);
		//	Ogre::Vector3 finalPos = localPos + (localOrient * this->offsetPos);

		//	this->physicsRagDollComponent->physicsBody->setPositionOrientation(finalPos, finalOrient);
		//	}*/

		//	Ogre::Quaternion worldOrient;
		//	Ogre::Vector3 worldPos;
		//	this->body->getPositionOrientation(worldPos, worldOrient);
		//	Ogre::Quaternion localOrient = this->ogreBone->getOrientation();
		//	Ogre::Vector3 localPos = this->ogreBone->getPosition();
		//	// Ogre::LogManager::getSingletonPtr()->logMessage("localpos: " + Ogre::StringConverter::toString(localPos) + " for bone: " + this->ogreBone->getName());

		//	Ogre::Vector3 center = this->physicsRagDollComponent->bones.cbegin()->bone->getBody()->getPosition();
		//	Ogre::Vector3 offsetPos = (worldPos - center);

		//	if (this->parentRagBone != nullptr)
		//	{

		//		this->body->setPositionOrientation(this->ogreBone->_getDerivedPosition(), this->ogreBone->_getDerivedOrientation());

		//	}
		//	else
		//	{ // if this is the root orientate the node

		//		Ogre::Quaternion finalOrient = (localOrient * this->offsetOrient);
		//		Ogre::Vector3 finalPos = localPos + (localOrient * this->offsetPos);

		//		this->physicsRagDollComponent->physicsBody->setPositionOrientation(worldPos + localPos, localOrient);
		//	}

		//}
	}

	const Ogre::String& PhysicsRagDollComponent::RagBone::getName(void) const
	{
		return this->name;
	}

	void PhysicsRagDollComponent::RagBone::setOrientation(const Ogre::Quaternion& orientation)
	{
		// Ogre::Vector3 center = this->physicsRagDollComponent->gameObjectPtr->getEntity()->getWorldBoundingBox().getCenter();
		Ogre::Vector3 center = this->physicsRagDollComponent->ragDataList.cbegin()->ragBone->getBody()->getPosition();

		// Very important for orientation: When the bone gets created and the collision hull, calculate once the
		// initial relative position from the current bone to the root bone (e.g. PELVIS) and set the initial orientation
		// to the orientation of the bone, when the ragdoll is fresh created. This values never changes
		// after that apply the orientation always from the initial relative position and intial orientation
		this->body->setPositionOrientation(center + orientation * this->initialBonePosition, orientation * this->initialBoneOrientation);
	}

	void PhysicsRagDollComponent::RagBone::rotate(const Ogre::Quaternion& rotation)
	{
// hier orientieren D:\Ogre\GameEngineDevelopment\external\MeshSplitterDemos\OgrePhysX_04\source\OgrePhysXRagdoll.cpp
		Ogre::Quaternion localOrient;
		Ogre::Vector3 localPos;
		this->body->getPositionOrientation(localPos, localOrient);

		Ogre::Vector3 center = this->physicsRagDollComponent->ragDataList.cbegin()->ragBone->getBody()->getPosition();
		// for relative rotation always calculate the offset pos to the center, depending from the current local pos and orientation
		Ogre::Vector3 offsetPos = (localPos - center);

		// rotate around the center, take the local orientation into account
		this->body->setPositionOrientation(center + rotation * offsetPos, rotation * localOrient);
	}

	void PhysicsRagDollComponent::RagBone::setInitialState(void)
	{
		Ogre::Vector3 center = this->physicsRagDollComponent->ragDataList.cbegin()->ragBone->getBody()->getPosition();
		if (nullptr != this->body)
			this->body->setPositionOrientation(center + this->initialBonePosition, this->initialBoneOrientation);

		this->ogreBone->setPosition(this->initialBonePosition);
		this->ogreBone->setOrientation(this->initialBoneOrientation);
	}

	OgreNewt::Body* PhysicsRagDollComponent::RagBone::getBody()
	{
		return this->body;
	}

	Ogre::v1::OldBone* PhysicsRagDollComponent::RagBone::getOgreBone(void) const
	{
		return this->ogreBone;
	}

	PhysicsRagDollComponent::RagBone* PhysicsRagDollComponent::RagBone::getParentRagBone(void) const
	{
		return this->parentRagBone;
	}

	Ogre::Quaternion PhysicsRagDollComponent::RagBone::getInitialBoneOrientation(void)
	{
		return this->initialBoneOrientation;
	}

	Ogre::Vector3 PhysicsRagDollComponent::RagBone::getInitialBonePosition(void)
	{
		return this->initialBonePosition;
	}

	Ogre::Quaternion PhysicsRagDollComponent::RagBone::getInitialBodyOrientation(void)
	{
		return this->initialBodyOrientation;
	}

	Ogre::Vector3 PhysicsRagDollComponent::RagBone::getWorldPosition(void)
	{
		// Ogre::Vector3 worldPosition = this->physicsRagDollComponent->gameObjectPtr->getSceneNode()->convertLocalToWorldPosition(this->ogreBone->_getDerivedPosition());
		// return worldPosition;
		return this->ogreBone->_getDerivedPosition();
	}

	Ogre::Quaternion PhysicsRagDollComponent::RagBone::getWorldOrientation(void)
	{
		// Ogre::Quaternion worldOrientation = this->physicsRagDollComponent->gameObjectPtr->getSceneNode()->convertLocalToWorldOrientation(this->ogreBone->_getDerivedOrientation());
		// return worldOrientation;
		return this->ogreBone->_getDerivedOrientation();
	}

	Ogre::Vector3 PhysicsRagDollComponent::RagBone::getInitialBodyPosition(void)
	{
		return this->initialBodyPosition;
	}

	Ogre::Vector3 PhysicsRagDollComponent::RagBone::getPosition(void) const
	{
		return this->body->getPosition();
	}

	Ogre::Quaternion PhysicsRagDollComponent::RagBone::getOrientation(void) const
	{
		return this->body->getOrientation();
	}

	void PhysicsRagDollComponent::RagBone::setInitialPose(const Ogre::Vector3& initialPose)
	{
		this->initialPose = initialPose;
	}

	Ogre::SceneNode* PhysicsRagDollComponent::RagBone::getSceneNode(void) const
	{
		return this->sceneNode;
	}

	void PhysicsRagDollComponent::RagBone::attachToNode(void)
	{
		this->body->attachNode(this->sceneNode);
	}

	void PhysicsRagDollComponent::RagBone::detachFromNode(void)
	{
		this->body->detachNode();
	}

	PhysicsRagDollComponent* PhysicsRagDollComponent::RagBone::getPhysicsRagDollComponent(void) const
	{
		return this->physicsRagDollComponent;
	}

	const Ogre::Vector3 PhysicsRagDollComponent::RagBone::getRagPose(void)
	{
		return this->pose;
	}

	void PhysicsRagDollComponent::RagBone::applyPose(const Ogre::Vector3& pose)
	{
		this->pose = pose;
		/*if (this->pose != Ogre::Vector3::ZERO) {
			this->physicsRagDollComponent->applyPose(this->pose);
			}*/

		if (this->pose != Ogre::Vector3::ZERO)
		{
			if (!this->upVector)
			{
				this->upVector = new OgreNewt::UpVector(this->body, this->pose);
			}
			else
			{
				this->upVector->setPin(pose);
			}
		}
		else
		{
			if (this->upVector)
			{
				delete this->upVector;
				this->upVector = nullptr;
			}
		}
	}

	JointCompPtr PhysicsRagDollComponent::RagBone::createJointComponent(const Ogre::String& type)
	{
		if (JointHingeComponent::getStaticClassName() == type)
		{
			this->jointCompPtr = boost::make_shared<JointHingeComponent>();
		}
		else if (JointBallAndSocketComponent::getStaticClassName() == type)
		{
			this->jointCompPtr = boost::make_shared<JointBallAndSocketComponent>();
		}
		else if (JointUniversalComponent::getStaticClassName() == type)
		{
			this->jointCompPtr = boost::make_shared<JointUniversalComponent>();
		}
		else if (JointHingeActuatorComponent::getStaticClassName() == type)
		{
			this->jointCompPtr = boost::make_shared<JointHingeActuatorComponent>();
		}
		else
		{
			this->jointCompPtr = boost::make_shared<JointComponent>();
		}
		this->jointCompPtr->setType(type);
		// Set scene manager for debug data
		this->jointCompPtr->setSceneManager(this->physicsRagDollComponent->getOwner()->getSceneManager());
// Attention: Is this correct?
		this->jointCompPtr->setOwner(this->physicsRagDollComponent->getOwner());
		return this->jointCompPtr;
	}

	JointCompPtr PhysicsRagDollComponent::RagBone::getJointComponent(void) const
	{
		return this->jointCompPtr;
	}

	Ogre::Vector3 PhysicsRagDollComponent::RagBone::getBodySize(void) const
	{
		return this->bodySize;
	}

	void PhysicsRagDollComponent::RagBone::applyRequiredForceForVelocity(const Ogre::Vector3& velocity)
	{
		this->forceForVelocity = velocity;
		this->canAddForceForVelocity = true;
	}

}; // namespace end
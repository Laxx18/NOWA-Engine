#include "NOWAPrecompiled.h"
#include "PhysicsRagDollComponentV3.h"
#include "TagPointComponent.h"
#include "main/AppStateManager.h"
#include "utilities/AnimationBlenderV2.h"
#include "utilities/MathHelper.h"
#include "utilities/XMLConverter.h"

#include "Animation/OgreBone.h"
#include "Animation/OgreSkeletonAnimation.h"
#include "Animation/OgreSkeletonInstance.h"

#include "modules/LuaScriptApi.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	// ========================================================================
	// Constructor / Destructor
	// ========================================================================

	PhysicsRagDollComponentV3::PhysicsRagDollComponentV3()
		: PhysicsActiveComponent()
		, rdState(INACTIVE)
		, rdOldState(INACTIVE)
		, skeletonInstance(nullptr)
		, ragDollBody(nullptr)
		, animationEnabled(true)
		, isSimulating(false)
		, ragdollPositionOffset(Ogre::Vector3::ZERO)
		, ragdollOrientationOffset(Ogre::Quaternion::IDENTITY)
		, activated(new Variant(PhysicsRagDollComponentV3::AttrActivated(), true, this->attributes))
		, boneConfigFile(new Variant(PhysicsRagDollComponentV3::AttrBonesConfigFile(), Ogre::String(), this->attributes))
		, state(new Variant(PhysicsRagDollComponentV3::AttrState(), { "Inactive", "Animation", "Ragdolling", "PartialRagdolling" }, this->attributes))
	{
		this->boneConfigFile->setDescription("Name of the XML config file, no path, e.g. FlubberRagdoll.rag");
		this->boneConfigFile->addUserData(GameObject::AttrActionFileOpenDialog(), "Models");

		this->gyroscopicTorque->setValue(false);
		this->gyroscopicTorque->setVisible(false);

		AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &PhysicsRagDollComponentV3::gameObjectAnimationChangedDelegate), EventDataAnimationChanged::getStaticEventType());
		AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &PhysicsRagDollComponentV3::deleteJointDelegate), EventDataDeleteJoint::getStaticEventType());
	}

	PhysicsRagDollComponentV3::~PhysicsRagDollComponentV3()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsRagDollComponentV3] Destructor for game object: " + this->gameObjectPtr->getName());

		AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &PhysicsRagDollComponentV3::gameObjectAnimationChangedDelegate), EventDataAnimationChanged::getStaticEventType());
		AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &PhysicsRagDollComponentV3::deleteJointDelegate), EventDataDeleteJoint::getStaticEventType());

		this->destroyRagDoll();
	}

	// ========================================================================
	// init
	// ========================================================================

	bool PhysicsRagDollComponentV3::init(rapidxml::xml_node<>*& propertyElement)
	{
		PhysicsActiveComponent::parseCommonProperties(propertyElement);

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
				this->rdState = INACTIVE;
			else if (stateCaption == "Animation")
				this->rdState = ANIMATION;
			else if (stateCaption == "Ragdolling")
			{
				this->rdState = RAGDOLLING;
				this->animationEnabled = false;
			}
			else if (stateCaption == "PartialRagdolling")
				this->rdState = PARTIAL_RAGDOLLING;
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	// ========================================================================
	// clone
	// ========================================================================

	GameObjectCompPtr PhysicsRagDollComponentV3::clone(GameObjectPtr clonedGameObjectPtr)
	{
		PhysicsRagDollComponentV3Ptr clonedCompPtr(boost::make_shared<PhysicsRagDollComponentV3>());

		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setMass(this->mass->getReal());
		clonedCompPtr->setMassOrigin(this->massOrigin->getVector3());
		clonedCompPtr->setLinearDamping(this->linearDamping->getReal());
		clonedCompPtr->setAngularDamping(this->angularDamping->getVector3());
		clonedCompPtr->setGravity(this->gravity->getVector3());
		clonedCompPtr->setGravitySourceCategory(this->gravitySourceCategory->getString());
		clonedCompPtr->setConstraintDirection(this->constraintDirection->getVector3());
		clonedCompPtr->setSpeed(this->speed->getReal());
		clonedCompPtr->setMaxSpeed(this->maxSpeed->getReal());
		clonedCompPtr->setCollisionType(this->collisionType->getListSelectedValue());
		clonedCompPtr->setCollisionDirection(this->collisionDirection->getVector3());
		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setBoneConfigFile(this->boneConfigFile->getString());
		clonedCompPtr->setState(this->state->getListSelectedValue());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	// ========================================================================
	// postInit
	// ========================================================================

	bool PhysicsRagDollComponentV3::postInit(void)
	{
		bool success = PhysicsComponent::postInit();

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsRagDollComponentV3] Post init for game object: " + this->gameObjectPtr->getName());

		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		return success;
	}

	// ========================================================================
	// connect / disconnect
	// ========================================================================

	bool PhysicsRagDollComponentV3::connect(void)
	{
		bool success = PhysicsActiveComponent::connect();
		this->isSimulating = true;
		this->internalApplyState();
		return success;
	}

	bool PhysicsRagDollComponentV3::disconnect(void)
	{
		bool success = PhysicsActiveComponent::disconnect();

		Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
		NOWA::GraphicsModule::getInstance()->removeTrackedClosure(id);

		this->isSimulating = false;
		this->internalShowDebugData(false);

		if (this->rdState == RAGDOLLING || this->rdState == PARTIAL_RAGDOLLING)
		{
			this->endRagdolling();
		}

		this->rdOldState = INACTIVE;
		return success;
	}

	// ========================================================================
	// onRemoveComponent
	// ========================================================================

	void PhysicsRagDollComponentV3::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();
		this->rdState = INACTIVE;
		boost::shared_ptr<EventDataGameObjectIsInRagDollingState> evt(new EventDataGameObjectIsInRagDollingState(this->gameObjectPtr->getId(), false));
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evt);
		this->activated->setValue(false);
	}

	// ========================================================================
	// getClassName etc.
	// ========================================================================

	Ogre::String PhysicsRagDollComponentV3::getClassName(void) const { return "PhysicsRagDollComponentV3"; }
	Ogre::String PhysicsRagDollComponentV3::getParentClassName(void) const { return "PhysicsActiveComponent"; }
	Ogre::String PhysicsRagDollComponentV3::getParentParentClassName(void) const { return "PhysicsComponent"; }

	// ========================================================================
	// parseRagConfig - Parse .rag XML and fill OgreNewt::RagDollBody::RagConfig
	// ========================================================================

	bool PhysicsRagDollComponentV3::parseRagConfig(OgreNewt::RagDollBody::RagConfig& outConfig)
	{
		if (this->boneConfigFile->getString().empty())
			return false;

		Ogre::DataStreamPtr stream = Ogre::ResourceGroupManager::getSingleton().openResource(this->boneConfigFile->getString());
		if (stream.isNull())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponentV3] Could not open: " + this->boneConfigFile->getString());
			return false;
		}

		rapidxml::xml_document<> doc;
		Ogre::String data = stream->getAsString();
		std::vector<char> xmlBuffer(data.begin(), data.end());
		xmlBuffer.push_back('\0');
		try
		{
			doc.parse<0>(&xmlBuffer[0]);
		}
		catch (const rapidxml::parse_error&)
		{
			stream->close();
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponentV3] Parse error: " + this->boneConfigFile->getString());
			return false;
		}
		stream->close();

		rapidxml::xml_node<>* root = doc.first_node();
		if (!root)
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponentV3] No root element in: " + this->boneConfigFile->getString());
			return false;
		}

		// Orientation/position offsets
		outConfig.orientationOffset = Ogre::Vector3::ZERO;
		outConfig.positionOffset = Ogre::Vector3::ZERO;
		if (auto* a = root->first_attribute("OrientationOffset"))
			outConfig.orientationOffset = Ogre::StringConverter::parseVector3(a->value());
		if (auto* a = root->first_attribute("PositionOffset"))
			outConfig.positionOffset = Ogre::StringConverter::parseVector3(a->value());

		// Store locally for position compensation
		this->ragdollPositionOffset = outConfig.positionOffset;
		if (outConfig.orientationOffset != Ogre::Vector3::ZERO)
		{
			Ogre::Matrix3 rot;
			rot.FromEulerAnglesXYZ(Ogre::Degree(outConfig.orientationOffset.x), Ogre::Degree(outConfig.orientationOffset.y), Ogre::Degree(outConfig.orientationOffset.z));
			this->ragdollOrientationOffset.FromRotationMatrix(rot);
		}

		// Apply default pose if specified
		if (auto* poseAttr = root->first_attribute("Pose"))
		{
			Ogre::String defaultPose = poseAttr->value();
			if (!defaultPose.empty() && nullptr != this->skeletonInstance)
			{
				NOWA::GraphicsModule::RenderCommand renderCommand = [this, defaultPose]()
				{
					Ogre::IdString poseIdString(defaultPose);
					if (this->skeletonInstance->hasAnimation(poseIdString))
					{
						Ogre::SkeletonAnimation* anim = this->skeletonInstance->getAnimation(poseIdString);
						anim->setEnabled(true);
						anim->setLoop(false);
						anim->setTime(100.0f);
						this->skeletonInstance->update();
						anim->setEnabled(false);
					}
				};
				NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PhysicsRagDollComponentV3::parseRagConfig_pose");
			}
		}

		// Parse Bones
		rapidxml::xml_node<>* bonesNode = root->first_node("Bones");
		if (!bonesNode)
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponentV3] No <Bones> in: " + this->boneConfigFile->getString());
			return false;
		}

		for (auto* bn = bonesNode->first_node("Bone"); bn; bn = bn->next_sibling("Bone"))
		{
			// Skip bone correction entries (Source/Target pairs)
			if (bn->first_attribute("Source"))
				continue;

			OgreNewt::RagDollBody::BoneDef boneDef;

			if (auto* a = bn->first_attribute("Name"))         boneDef.name = a->value();
			if (auto* a = bn->first_attribute("SkeletonBone")) boneDef.skeletonBone = a->value();

			if (auto* a = bn->first_attribute("Size"))
				boneDef.size = Ogre::StringConverter::parseVector3(a->value());
			if (auto* a = bn->first_attribute("Mass"))
				boneDef.mass = Ogre::StringConverter::parseReal(a->value());

			// Map shape string
			if (auto* a = bn->first_attribute("Shape"))
				boneDef.shape = a->value();

			if (boneDef.name.empty())
			{
				Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponentV3] Bone missing Name in: " + this->boneConfigFile->getString());
				return false;
			}

			outConfig.bones.push_back(boneDef);
		}

		// Parse Joints
		rapidxml::xml_node<>* jointsNode = root->first_node("Joints");
		if (!jointsNode)
		{
			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponentV3] No <Joints> in: " + this->boneConfigFile->getString());
			return false;
		}

		for (auto* jn = jointsNode->first_node("Joint"); jn; jn = jn->next_sibling("Joint"))
		{
			OgreNewt::RagDollBody::JointDef jointDef;

			if (auto* a = jn->first_attribute("Type"))          jointDef.type = a->value();
			if (auto* a = jn->first_attribute("Child"))         jointDef.childName = a->value();
			if (auto* a = jn->first_attribute("Parent"))        jointDef.parentName = a->value();
			if (auto* a = jn->first_attribute("Friction"))      jointDef.friction = Ogre::StringConverter::parseReal(a->value());
			if (auto* a = jn->first_attribute("MinTwistAngle")) jointDef.minTwistAngle = Ogre::StringConverter::parseReal(a->value());
			if (auto* a = jn->first_attribute("MaxTwistAngle")) jointDef.maxTwistAngle = Ogre::StringConverter::parseReal(a->value());
			if (auto* a = jn->first_attribute("MaxConeAngle"))  jointDef.maxConeAngle = Ogre::StringConverter::parseReal(a->value());
			if (auto* a = jn->first_attribute("Pin"))           jointDef.pin = Ogre::StringConverter::parseVector3(a->value());

			if (jointDef.childName.empty())
			{
				Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponentV3] Joint missing Child in: " + this->boneConfigFile->getString());
				return false;
			}

			outConfig.joints.push_back(jointDef);
		}

		Ogre::LogManager::getSingleton().logMessage(Ogre::LML_TRIVIAL, "[PhysicsRagDollComponentV3] Parsed "
			+ this->boneConfigFile->getString() + ": " + Ogre::StringConverter::toString(outConfig.bones.size())
			+ " bones, " + Ogre::StringConverter::toString(outConfig.joints.size()) + " joints");

		return true;
	}

	// ========================================================================
	// createRagDoll - Creates OgreNewt::RagDollBody from parsed config
	// ========================================================================

	bool PhysicsRagDollComponentV3::createRagDoll(void)
	{
		this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
		this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
		this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();

		Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
		if (!item || !item->hasSkeleton())
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsRagDollComponentV3] No Item with skeleton for: " + this->gameObjectPtr->getName());
			return false;
		}

		this->skeletonInstance = item->getSkeletonInstance();

		// Ensure skeleton transforms are up-to-date
		NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
		{
			if (nullptr != this->skeletonInstance)
			{
				this->skeletonInstance->update();
			}
		};
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PhysicsRagDollComponentV3::createRagDoll_update");

		// Parse .rag XML into RagConfig
		OgreNewt::RagDollBody::RagConfig config;
		if (!this->parseRagConfig(config))
			return false;

		// Create root collision from first bone in config (or use existing collision)
		OgreNewt::CollisionPtr rootCollision;
		if (!config.bones.empty())
		{
			// Build root collision from first bone def
			const auto rootBoneDef = config.bones[0];
			Ogre::Quaternion orientOffset = Ogre::Quaternion::IDENTITY;
			if (config.orientationOffset != Ogre::Vector3::ZERO)
			{
				orientOffset =
					Ogre::Quaternion(Ogre::Degree(config.orientationOffset.x), Ogre::Vector3::UNIT_X) *
					Ogre::Quaternion(Ogre::Degree(config.orientationOffset.y), Ogre::Vector3::UNIT_Y) *
					Ogre::Quaternion(Ogre::Degree(config.orientationOffset.z), Ogre::Vector3::UNIT_Z);
			}
			rootCollision = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::Box(
				this->ogreNewt, rootBoneDef.size, this->gameObjectPtr->getCategoryId(), orientOffset, config.positionOffset));
		}
		else
		{
			// Fallback: unit box
			rootCollision = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::Box(
				this->ogreNewt, Ogre::Vector3::UNIT_SCALE, this->gameObjectPtr->getCategoryId()));
		}

		// Create RagDollBody. It IS-A Body with a real m_body from the start.
		this->ragDollBody = new OgreNewt::RagDollBody(
			this->ogreNewt,
			this->gameObjectPtr->getSceneManager(),
			rootCollision,
			config,
			item,
			this->gameObjectPtr->getSceneNode());

		// The RagDollBody IS-A Body (root = Hips). Set it as our physicsBody.
		this->physicsBody = this->ragDollBody;

		this->physicsBody->setUserData(OgreNewt::Any(dynamic_cast<PhysicsComponent*>(this)));
		this->physicsBody->setType(this->gameObjectPtr->getCategoryId());

		const auto materialId = AppStateManager::getSingletonPtr()->getGameObjectController()->getMaterialID(this->gameObjectPtr.get(), this->ogreNewt);
		this->physicsBody->setMaterialGroupID(materialId);

		return true;
	}

	// ========================================================================
	// destroyRagDoll
	// ========================================================================

	void PhysicsRagDollComponentV3::destroyRagDoll(void)
	{
		if (nullptr != this->ragDollBody)
		{
			if (this->ragDollBody->getState() != OgreNewt::RagDollBody::INACTIVE)
			{
				this->ragDollBody->endRagdolling();
			}
			// ragDollBody == physicsBody (they point to the same object)
			// clear physicsBody first so PhysicsActiveComponent destructor doesn't double-delete
			this->physicsBody = nullptr;
			delete this->ragDollBody;
			this->ragDollBody = nullptr;
		}
	}

	// ========================================================================
	// startRagdolling / endRagdolling
	// ========================================================================

	void PhysicsRagDollComponentV3::startRagdolling(void)
	{
		if (nullptr == this->ragDollBody)
			return;

		if (this->rdState == PARTIAL_RAGDOLLING)
		{
			this->ragDollBody->startPartialRagdolling();
		}
		else
		{
			this->ragDollBody->startRagdolling();
		}

		// Set constraint axis on root body
		this->setConstraintAxis(this->constraintAxis->getVector3());

		if (this->rdState == RAGDOLLING)
		{
			this->releaseConstraintAxisPin();
		}
		else if (this->rdState == PARTIAL_RAGDOLLING)
		{
			this->setConstraintDirection(this->constraintDirection->getVector3());
		}
	}

	void PhysicsRagDollComponentV3::endRagdolling(void)
	{
		if (nullptr == this->ragDollBody)
			return;

		NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
		{
			this->ragDollBody->endRagdolling();

			// Disconnect TagPointComponents
			boost::shared_ptr<TagPointComponent> tagPointCompPtr = nullptr;
			size_t i = 0;
			do
			{
				tagPointCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<TagPointComponent>(i++));
				if (nullptr != tagPointCompPtr)
				{
					tagPointCompPtr->disconnect();
				}
			} while (nullptr != tagPointCompPtr);
		};
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PhysicsRagDollComponentV3::endRagdolling");
	}

	// ========================================================================
	// internalApplyState - State machine (same logic as V2)
	// ========================================================================

	void PhysicsRagDollComponentV3::internalApplyState(void)
	{
		if (false == this->isSimulating)
			return;

		NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
		{
			this->internalShowDebugData(false);

			if (this->rdState == RAGDOLLING)
			{
				boost::shared_ptr<EventDataGameObjectIsInRagDollingState> evt(new EventDataGameObjectIsInRagDollingState(this->gameObjectPtr->getId(), true));
				NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evt);

				this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
				this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
				this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();

				this->partialRagdollBoneName = "";
				this->releaseConstraintDirection();
				this->releaseConstraintAxis();
				this->destroyRagDoll();
				this->destroyBody();
				this->createRagDoll();
				this->startRagdolling();
			}
			else if (this->rdState == PARTIAL_RAGDOLLING)
			{
				boost::shared_ptr<EventDataGameObjectIsInRagDollingState> evt(new EventDataGameObjectIsInRagDollingState(this->gameObjectPtr->getId(), false));
				NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evt);

				this->endRagdolling();

				this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
				if (this->rdOldState == RAGDOLLING)
					this->initialPosition += Ogre::Vector3(0.0f, this->gameObjectPtr->getBottomOffset().y, 0.0f);
				this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
				if (this->rdOldState != RAGDOLLING)
					this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();

				this->releaseConstraintDirection();
				this->releaseConstraintAxis();
				this->destroyRagDoll();
				this->destroyBody();
				this->createRagDoll();
				this->startRagdolling();
			}
			else if (this->rdState == INACTIVE)
			{
				this->createInactiveBody();
			}
			else if (this->rdState == ANIMATION)
			{
				boost::shared_ptr<EventDataGameObjectIsInRagDollingState> evt(new EventDataGameObjectIsInRagDollingState(this->gameObjectPtr->getId(), false));
				NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evt);

				this->endRagdolling();

				this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
				if (this->rdOldState == RAGDOLLING)
					this->initialPosition += Ogre::Vector3(0.0f, this->gameObjectPtr->getBottomOffset().y, 0.0f);
				this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
				if (this->rdOldState != RAGDOLLING)
					this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();

				this->releaseConstraintDirection();
				this->releaseConstraintAxis();
				this->destroyRagDoll();
				this->destroyBody();
				this->createDynamicBody();
			}

			this->internalShowDebugData(true);
		};
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PhysicsRagDollComponentV3::internalApplyState");
	}

	// ========================================================================
	// createInactiveBody
	// ========================================================================

	void PhysicsRagDollComponentV3::createInactiveBody(void)
	{
		boost::shared_ptr<EventDataGameObjectIsInRagDollingState> evt(new EventDataGameObjectIsInRagDollingState(this->gameObjectPtr->getId(), false));
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evt);

		this->partialRagdollBoneName = "";
		this->destroyRagDoll();
		this->destroyBody();
		this->endRagdolling();
		this->releaseConstraintDirection();
		this->releaseConstraintAxis();

		this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
		if (this->rdOldState == RAGDOLLING)
			this->initialPosition += Ogre::Vector3(0.0f, this->gameObjectPtr->getBottomOffset().y, 0.0f);
		this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
		if (this->rdOldState != RAGDOLLING)
			this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();

		this->createDynamicBody();
	}

	// ========================================================================
	// update
	// ========================================================================

	void PhysicsRagDollComponentV3::update(Ogre::Real dt, bool notSimulating)
	{
		if (true == activated->getBool() && false == notSimulating)
		{
			if (this->rdState == RAGDOLLING || this->rdState == PARTIAL_RAGDOLLING)
			{
				if (nullptr != this->ragDollBody && nullptr != this->skeletonInstance)
				{
					auto closureFunction = [this](Ogre::Real renderDt)
					{
						this->ragDollBody->applyRagdollStateToModel();
					};
					Ogre::String id = this->gameObjectPtr->getName() + this->getClassName() + "::update" + Ogre::StringConverter::toString(this->index);
					NOWA::GraphicsModule::getInstance()->updateTrackedClosure(id, closureFunction, false);
				}
			}
		}
	}

	// ========================================================================
	// actualizeValue
	// ========================================================================

	void PhysicsRagDollComponentV3::actualizeValue(Variant* attribute)
	{
		PhysicsActiveComponent::actualizeCommonValue(attribute);

		if (PhysicsRagDollComponentV3::AttrActivated() == attribute->getName())
			this->setActivated(attribute->getBool());
		else if (PhysicsRagDollComponentV3::AttrBonesConfigFile() == attribute->getName())
			this->setBoneConfigFile(attribute->getString());
		else if (PhysicsRagDollComponentV3::AttrState() == attribute->getName())
			this->setState(attribute->getListSelectedValue());
	}

	// ========================================================================
	// writeXML
	// ========================================================================

	void PhysicsRagDollComponentV3::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		PhysicsActiveComponent::writeCommonProperties(propertiesXML, doc);

		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Activated"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->activated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "BoneConfigFile"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->boneConfigFile->getString())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "State"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->state->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
	}

	// ========================================================================
	// showDebugData
	// ========================================================================

	void PhysicsRagDollComponentV3::showDebugData(void)
	{
		GameObjectComponent::showDebugData();
	}

	void PhysicsRagDollComponentV3::internalShowDebugData(bool activate)
	{
		if (nullptr != this->ragDollBody && this->ragDollBody->getState() != OgreNewt::RagDollBody::INACTIVE)
		{
			// TODO: iterate ragDollBody->getBoneData() and call showDebugCollision on each body
			// For now, show debug on the root body only
			NOWA::GraphicsModule::RenderCommand renderCommand = [this, activate]()
			{
				if (nullptr != this->physicsBody)
				{
					this->physicsBody->showDebugCollision(false, this->bShowDebugData && activate);
				}
			};
			NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PhysicsRagDollComponentV3::internalShowDebugData");
		}
		else if (nullptr != this->physicsBody)
		{
			NOWA::GraphicsModule::RenderCommand renderCommand = [this, activate]()
			{
				this->physicsBody->showDebugCollision(false, this->bShowDebugData && activate);
			};
			NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PhysicsRagDollComponentV3::showDebugData");
		}
	}

	// ========================================================================
	// setActivated
	// ========================================================================

	void PhysicsRagDollComponentV3::setActivated(bool activated)
	{
		this->activated->setValue(activated);

		if (nullptr != this->ragDollBody && (this->rdState == RAGDOLLING || this->rdState == PARTIAL_RAGDOLLING))
		{
			// TODO: freeze/unfreeze articulation bodies
			// Newton4 ndModelArticulation doesn't have a direct freeze API per body,
			// but we can use SetSleep on the articulation
		}
	}

	// ========================================================================
	// setState / getState
	// ========================================================================

	void PhysicsRagDollComponentV3::setState(const Ogre::String& state)
	{
		this->state->setListSelectedValue(state);
		this->rdOldState = this->rdState;

		if (state == "Inactive")
		{
			if (this->rdState == INACTIVE) return;
			this->rdState = INACTIVE;
			boost::shared_ptr<EventDataGameObjectIsInRagDollingState> evt(new EventDataGameObjectIsInRagDollingState(this->gameObjectPtr->getId(), false));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evt);
		}
		else if (state == "Animation")
		{
			if (this->rdState == ANIMATION) return;
			this->rdState = ANIMATION;
			boost::shared_ptr<EventDataGameObjectIsInRagDollingState> evt(new EventDataGameObjectIsInRagDollingState(this->gameObjectPtr->getId(), false));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evt);
		}
		else if (state == "Ragdolling")
		{
			if (nullptr == this->gameObjectPtr) return;
			if (this->rdState == RAGDOLLING) return;
			this->rdState = RAGDOLLING;
			this->animationEnabled = false;
			boost::shared_ptr<EventDataGameObjectIsInRagDollingState> evt(new EventDataGameObjectIsInRagDollingState(this->gameObjectPtr->getId(), true));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evt);
		}
		else if (state == "PartialRagdolling")
		{
			if (this->rdState == PARTIAL_RAGDOLLING) return;
			this->rdState = PARTIAL_RAGDOLLING;
			boost::shared_ptr<EventDataGameObjectIsInRagDollingState> evt(new EventDataGameObjectIsInRagDollingState(this->gameObjectPtr->getId(), false));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(evt);
		}

		if (this->isSimulating)
			this->internalApplyState();
	}

	Ogre::String PhysicsRagDollComponentV3::getState(void) const
	{
		return this->state->getListSelectedValue();
	}

	// ========================================================================
	// setBoneConfigFile / getBoneConfigFile
	// ========================================================================

	void PhysicsRagDollComponentV3::setBoneConfigFile(const Ogre::String& boneConfigFile)
	{
		this->boneConfigFile->setValue(boneConfigFile);
	}

	Ogre::String PhysicsRagDollComponentV3::getBoneConfigFile(void) const
	{
		return this->boneConfigFile->getString();
	}

	// ========================================================================
	// setAnimationEnabled / isAnimationEnabled
	// ========================================================================

	void PhysicsRagDollComponentV3::setAnimationEnabled(bool animationEnabled)
	{
		this->animationEnabled = animationEnabled;
		// When re-enabling animation, the next state change will re-create the ragdoll
	}

	bool PhysicsRagDollComponentV3::isAnimationEnabled(void) const
	{
		return this->animationEnabled;
	}

	// ========================================================================
	// setInitialState
	// ========================================================================

	void PhysicsRagDollComponentV3::setInitialState(void)
	{
		if (nullptr == this->ragDollBody)
			return;

		// Snap physics bodies back to current animation pose
		NOWA::GraphicsModule::RenderCommand renderCommand = [this]()
		{
			this->ragDollBody->applyModelStateToRagdoll();
		};
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PhysicsRagDollComponentV3::setInitialState");
	}

	// ========================================================================
	// Physics body overrides
	// ========================================================================

	OgreNewt::Body* PhysicsRagDollComponentV3::getBody(void) const
	{
		return this->physicsBody;
	}

	void PhysicsRagDollComponentV3::setVelocity(const Ogre::Vector3& velocity)
	{
		if (this->isSimulating && nullptr != this->physicsBody)
			this->physicsBody->setVelocity(velocity);
	}

	Ogre::Vector3 PhysicsRagDollComponentV3::getVelocity(void) const
	{
		if (this->isSimulating && nullptr != this->physicsBody)
			return this->physicsBody->getVelocity();
		return Ogre::Vector3::ZERO;
	}

	void PhysicsRagDollComponentV3::setPosition(Ogre::Real x, Ogre::Real y, Ogre::Real z)
	{
		this->setPosition(Ogre::Vector3(x, y, z));
	}

	void PhysicsRagDollComponentV3::setPosition(const Ogre::Vector3& position)
	{
		if (this->rdState == RAGDOLLING && !this->isSimulating)
			NOWA::GraphicsModule::getInstance()->updateNodePosition(this->gameObjectPtr->getSceneNode(), position);
		else
			PhysicsComponent::setPosition(position);
	}

	void PhysicsRagDollComponentV3::translate(const Ogre::Vector3& relativePosition)
	{
		if (this->rdState == RAGDOLLING && !this->isSimulating)
			NOWA::GraphicsModule::getInstance()->updateNodePosition(this->gameObjectPtr->getSceneNode(), this->gameObjectPtr->getSceneNode()->getPosition() + relativePosition);
		else
			PhysicsComponent::translate(relativePosition);
	}

	Ogre::Vector3 PhysicsRagDollComponentV3::getPosition(void) const
	{
		if (!this->isSimulating || nullptr == this->physicsBody)
			return this->gameObjectPtr->getPosition();
		return this->physicsBody->getPosition();
	}

	void PhysicsRagDollComponentV3::setOrientation(const Ogre::Quaternion& orientation)
	{
		if ((this->rdState == RAGDOLLING || this->rdState == PARTIAL_RAGDOLLING) && !this->isSimulating)
			NOWA::GraphicsModule::getInstance()->updateNodeOrientation(this->gameObjectPtr->getSceneNode(), orientation);
		else
			PhysicsComponent::setOrientation(orientation);
	}

	void PhysicsRagDollComponentV3::rotate(const Ogre::Quaternion& relativeRotation)
	{
		if (this->rdState == RAGDOLLING && !this->isSimulating)
			NOWA::GraphicsModule::getInstance()->updateNodeOrientation(this->gameObjectPtr->getSceneNode(), this->gameObjectPtr->getSceneNode()->getOrientation() * relativeRotation);
		else
			PhysicsComponent::rotate(relativeRotation);
	}

	Ogre::Quaternion PhysicsRagDollComponentV3::getOrientation(void) const
	{
		if (!this->isSimulating || nullptr == this->physicsBody)
			return this->gameObjectPtr->getOrientation();
		return this->physicsBody->getOrientation();
	}

	// ========================================================================
	// inheritVelOmega
	// ========================================================================

	void PhysicsRagDollComponentV3::inheritVelOmega(Ogre::Vector3 vel, Ogre::Vector3 omega)
	{
		if (nullptr == this->ragDollBody)
			return;

		Ogre::Vector3 mainPos = this->gameObjectPtr->getSceneNode()->_getDerivedPosition();
		size_t offset = (this->rdState == PARTIAL_RAGDOLLING) ? 1 : 0;

		this->ragDollBody->inheritVelOmega(vel, omega, mainPos, offset);
	}

	// ========================================================================
	// Bone access
	// ========================================================================

	OgreNewt::RagDollBody* PhysicsRagDollComponentV3::getRagDollBody(void) const
	{
		return this->ragDollBody;
	}

	OgreNewt::Body* PhysicsRagDollComponentV3::getBoneBody(const Ogre::String& ragBoneName) const
	{
		if (nullptr == this->ragDollBody)
			return nullptr;
		return this->ragDollBody->getBoneBody(ragBoneName);
	}

	Ogre::Bone* PhysicsRagDollComponentV3::getOgreBone(const Ogre::String& ragBoneName) const
	{
		if (nullptr == this->ragDollBody)
			return nullptr;
		return this->ragDollBody->getOgreBone(ragBoneName);
	}

	// ========================================================================
	// Event delegates
	// ========================================================================

	void PhysicsRagDollComponentV3::gameObjectAnimationChangedDelegate(EventDataPtr eventData)
	{
		// Reserved for future use
	}

	void PhysicsRagDollComponentV3::deleteJointDelegate(EventDataPtr eventData)
	{
		// Reserved for future use
	}

	// ========================================================================
	// Lua API
	// ========================================================================

	PhysicsRagDollComponentV3* getPhysicsRagDollComponentV3(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<PhysicsRagDollComponentV3>(gameObject->getComponentWithOccurrence<PhysicsRagDollComponentV3>(occurrenceIndex)).get();
	}

	PhysicsRagDollComponentV3* getPhysicsRagDollComponentV3(GameObject* gameObject)
	{
		return makeStrongPtr<PhysicsRagDollComponentV3>(gameObject->getComponent<PhysicsRagDollComponentV3>()).get();
	}

	PhysicsRagDollComponentV3* getPhysicsRagDollComponentV3FromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<PhysicsRagDollComponentV3>(gameObject->getComponentFromName<PhysicsRagDollComponentV3>(name)).get();
	}

	void PhysicsRagDollComponentV3::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)[class_<PhysicsRagDollComponentV3, PhysicsActiveComponent>("PhysicsRagDollComponentV3")
				.def("inheritVelOmega", &PhysicsRagDollComponentV3::inheritVelOmega)
				.def("setActivated", &PhysicsRagDollComponentV3::setActivated)
				.def("setState", &PhysicsRagDollComponentV3::setState)
				.def("getState", &PhysicsRagDollComponentV3::getState)
				.def("setVelocity", &PhysicsRagDollComponentV3::setVelocity)
				.def("getVelocity", &PhysicsRagDollComponentV3::getVelocity)
				.def("getPosition", &PhysicsRagDollComponentV3::getPosition)
				.def("setOrientation", &PhysicsRagDollComponentV3::setOrientation)
				.def("getOrientation", &PhysicsRagDollComponentV3::getOrientation)
				.def("setInitialState", &PhysicsRagDollComponentV3::setInitialState)
				.def("setAnimationEnabled", &PhysicsRagDollComponentV3::setAnimationEnabled)
				.def("isAnimationEnabled", &PhysicsRagDollComponentV3::isAnimationEnabled)
				.def("setBoneConfigFile", &PhysicsRagDollComponentV3::setBoneConfigFile)
				.def("getBoneConfigFile", &PhysicsRagDollComponentV3::getBoneConfigFile)
				.def("getBoneBody", &PhysicsRagDollComponentV3::getBoneBody)
				.def("getOgreBone", &PhysicsRagDollComponentV3::getOgreBone)];

		LuaScriptApi::getInstance()->addClassToCollection("PhysicsRagDollComponentV3", "class inherits PhysicsActiveComponent", PhysicsRagDollComponentV3::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("PhysicsRagDollComponentV3", "void setState(String state)",
			"Sets the ragdoll state. Possible values: 'Inactive', 'Animation', 'Ragdolling', 'PartialRagdolling'.");
		LuaScriptApi::getInstance()->addClassToCollection("PhysicsRagDollComponentV3", "String getState()", "Gets the current ragdoll state.");
		LuaScriptApi::getInstance()->addClassToCollection("PhysicsRagDollComponentV3", "void setVelocity(Vector3 velocity)", "Sets linear velocity on the root body.");
		LuaScriptApi::getInstance()->addClassToCollection("PhysicsRagDollComponentV3", "Vector3 getVelocity()", "Gets velocity of the root body.");
		LuaScriptApi::getInstance()->addClassToCollection("PhysicsRagDollComponentV3", "Vector3 getPosition()", "Gets position of the root body.");
		LuaScriptApi::getInstance()->addClassToCollection("PhysicsRagDollComponentV3", "void setOrientation(Quaternion orientation)", "Sets orientation. Only for initialization!");
		LuaScriptApi::getInstance()->addClassToCollection("PhysicsRagDollComponentV3", "Quaternion getOrientation()", "Gets orientation of the root body.");
		LuaScriptApi::getInstance()->addClassToCollection("PhysicsRagDollComponentV3", "void setInitialState()", "Resets ragdoll bones to animation pose.");
		LuaScriptApi::getInstance()->addClassToCollection("PhysicsRagDollComponentV3", "void setAnimationEnabled(bool enabled)", "Enables/disables animation control.");
		LuaScriptApi::getInstance()->addClassToCollection("PhysicsRagDollComponentV3", "bool isAnimationEnabled()", "Gets whether animation is enabled.");
		LuaScriptApi::getInstance()->addClassToCollection("PhysicsRagDollComponentV3", "void setBoneConfigFile(String boneConfigFile)", "Sets the .rag config file.");
		LuaScriptApi::getInstance()->addClassToCollection("PhysicsRagDollComponentV3", "String getBoneConfigFile()", "Gets the .rag config file name.");
		LuaScriptApi::getInstance()->addClassToCollection("PhysicsRagDollComponentV3", "Body getBoneBody(String ragBoneName)", "Gets a ragdoll bone's Newton body by name.");
		LuaScriptApi::getInstance()->addClassToCollection("PhysicsRagDollComponentV3", "Bone getOgreBone(String ragBoneName)", "Gets a ragdoll bone's Ogre bone by name.");

		gameObjectClass.def("getPhysicsRagDollComponentV3FromName", &getPhysicsRagDollComponentV3FromName);
		gameObjectClass.def("getPhysicsRagDollComponentV3", (PhysicsRagDollComponentV3 * (*)(GameObject*)) & getPhysicsRagDollComponentV3);
		gameObjectClass.def("getPhysicsRagDollComponentV32", (PhysicsRagDollComponentV3 * (*)(GameObject*, unsigned int)) & getPhysicsRagDollComponentV3);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PhysicsRagDollComponentV3 getPhysicsRagDollComponentV32(unsigned int occurrenceIndex)",
			"Gets the component by the given occurrence index.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PhysicsRagDollComponentV3 getPhysicsRagDollComponentV3()", "Gets the component.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "PhysicsRagDollComponentV3 getPhysicsRagDollComponentV3FromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castPhysicsRagDollComponentV3", &GameObjectController::cast<PhysicsRagDollComponentV3>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "PhysicsRagDollComponentV3 castPhysicsRagDollComponentV3(PhysicsRagDollComponentV3 other)", "Casts an incoming type.");
	}

}; // namespace end

#include "NOWAPrecompiled.h"
#include "LookAfterComponent.h"

#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "main/AppStateManager.h"

#include "gameobject/GameObjectFactory.h"

#include "OgreAbiUtils.h"

#include "Animation/OgreSkeletonAnimation.h"
#include "Animation/OgreSkeletonInstance.h"
#include "Animation/OgreBone.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	LookAfterComponent::LookAfterComponent()
		: GameObjectComponent(),
		name("LookAfterComponent"),
		targetSceneNode(nullptr),
		headBone(nullptr),
		headBoneV2(nullptr),
		activated(new Variant(LookAfterComponent::AttrActivated(), true, this->attributes)),
		headBoneName(new Variant(LookAfterComponent::AttrHeadBoneName(), std::vector<Ogre::String>(), this->attributes)),
		targetId(new Variant(LookAfterComponent::AttrTargetId(), static_cast<unsigned long>(0), this->attributes, true)),
		lookSpeed(new Variant(LookAfterComponent::AttrLookSpeed(), 20.0f, this->attributes)),
		maxPitch(new Variant(LookAfterComponent::AttrMaxPitch(), 90.0f, this->attributes)),
		maxYaw(new Variant(LookAfterComponent::AttrMaxYaw(), 90.0f, this->attributes))
	{

	}

	LookAfterComponent::~LookAfterComponent()
	{

	}

	const Ogre::String& LookAfterComponent::getName() const
	{
		return this->name;
	}

	void LookAfterComponent::getAbiCookie(Ogre::AbiCookie& outAbiCookie)
	{
		outAbiCookie = Ogre::generateAbiCookie();
	}

	void LookAfterComponent::install(const Ogre::NameValuePairList* options)
	{
		GameObjectFactory::getInstance()->getComponentFactory()->registerPluginComponentClass<LookAfterComponent>(LookAfterComponent::getStaticClassId(), LookAfterComponent::getStaticClassName());
	}

	bool LookAfterComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Activated")
		{
			this->activated->setValue(XMLConverter::getAttribBool(propertyElement, "data", true));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "HeadBoneName")
		{
			this->headBoneName->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetId")
		{
			this->targetId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "LookSpeed")
		{
			this->lookSpeed->setValue(XMLConverter::getAttribReal(propertyElement, "data", 2.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxPitch")
		{
			this->maxPitch->setValue(XMLConverter::getAttribReal(propertyElement, "data", 90.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "MaxYaw")
		{
			this->maxYaw->setValue(XMLConverter::getAttribReal(propertyElement, "data", 90.0f));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	bool LookAfterComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[LookAfterComponent] Init look after component for game object: " + this->gameObjectPtr->getName());
		
		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
		if (nullptr != entity)
		{
			Ogre::v1::Skeleton* skeleton = entity->getSkeleton();
			if (nullptr != skeleton)
			{
				std::vector<Ogre::String> boneNames;

				unsigned short numBones = skeleton->getNumBones();
				for (unsigned short iBone = 0; iBone < numBones; iBone++)
				{
					Ogre::v1::OldBone* bone = skeleton->getBone(iBone);
					if (nullptr == bone)
					{
						continue;
					}

					// Absolutely HAVE to create bone representations first. Otherwise we would get the wrong child count
					// because an attached object counts as a child
					// Would be nice to have a function that only gets the children that are bones...
					unsigned short numChildren = bone->numChildren();
					if (numChildren == 0)
					{
						bool unique = true;
						for (size_t i = 0; i < boneNames.size(); i++)
						{
							if (boneNames[i] == bone->getName())
							{
								unique = false;
								break;
							}
						}
						if (true == unique)
						{
							boneNames.emplace_back(bone->getName());
						}
					}
					else
					{
						bool unique = true;
						for (size_t i = 0; i < boneNames.size(); i++)
						{
							if (boneNames[i] == bone->getName())
							{
								unique = false;
								break;
							}
						}
						if (true == unique)
						{
							boneNames.emplace_back(bone->getName());
						}
					}
				}

				// Add all available bone names to list
				this->headBoneName->setValue(boneNames);
			}
		}
		else
		{
			Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
			if (nullptr != item)
			{
				Ogre::SkeletonInstance* skeleton = item->getSkeletonInstance();
				if (nullptr != skeleton)
				{
					std::vector<Ogre::String> boneNames;

					unsigned short numBones = skeleton->getNumBones();
					for (unsigned short iBone = 0; iBone < numBones; iBone++)
					{
						Ogre::Bone* bone = skeleton->getBone(iBone);
						if (nullptr == bone)
						{
							continue;
						}

						// Absolutely HAVE to create bone representations first. Otherwise we would get the wrong child count
						// because an attached object counts as a child
						// Would be nice to have a function that only gets the children that are bones...
						unsigned short numChildren = bone->getNumChildren();
						if (numChildren == 0)
						{
							bool unique = true;
							for (size_t i = 0; i < boneNames.size(); i++)
							{
								if (boneNames[i] == bone->getName())
								{
									unique = false;
									break;
								}
							}
							if (true == unique)
							{
								boneNames.emplace_back(bone->getName());
							}
						}
						else
						{
							bool unique = true;
							for (size_t i = 0; i < boneNames.size(); i++)
							{
								if (boneNames[i] == bone->getName())
								{
									unique = false;
									break;
								}
							}
							if (true == unique)
							{
								boneNames.emplace_back(bone->getName());
							}
						}
					}

					// Add all available bone names to list
					this->headBoneName->setValue(boneNames);
				}
			}
		}
			
		return true;
	}

	void LookAfterComponent::onRemoveComponent(void)
	{
		this->headBone = nullptr;
		this->headBoneV2 = nullptr;
	}

	bool LookAfterComponent::connect(void)
	{
		GameObjectPtr targetGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->targetId->getULong());
		if (nullptr != targetGameObjectPtr)
		{
			this->targetSceneNode = targetGameObjectPtr->getSceneNode();
		}

		Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
		if (nullptr != entity)
		{
			Ogre::v1::OldSkeletonInstance* oldSkeletonInstance = entity->getSkeleton();
			if (nullptr != oldSkeletonInstance)
			{
				this->headBone = oldSkeletonInstance->getBone(this->headBoneName->getListSelectedValue());

				// If there is no neck bone
				Ogre::v1::OldNode* neckBone = this->headBone->getParent();
				if (nullptr == neckBone)
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[LookAfterComponent] Warning the given bone: " + this->headBone->getName()
						+ " has no parent for " + this->gameObjectPtr->getName() + ". Look after component will not work!");
					this->headBone = nullptr;
					return true;
				}

				this->headBone->reset();
				// Existing animations have control of the bones of the skeleton when they are running. 
				// Each bone has an animation track for each animation which must be destroyed.

	// Attention: Problem: All animation of all mesh copies will be destroyed!
				// Set also the bone to manually controlled in order to modify it.
				this->headBone->setManuallyControlled(true);
				int numAnimations = oldSkeletonInstance->getNumAnimations();
				for (int i = 0; i < numAnimations; i++)
				{
					Ogre::v1::Animation* anim = oldSkeletonInstance->getAnimation(i);
					// Attention: destroyOldNodeTrack must be used!
					// anim->destroyNodeTrack(headBone->getHandle());
					anim->destroyOldNodeTrack(headBone->getHandle());
				}
			}
		}
		else
		{
			Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
			if (nullptr != item)
			{
				Ogre::SkeletonInstance* skeleton = item->getSkeletonInstance();
				if (nullptr != skeleton)
				{
					Ogre::SkeletonInstance* skeleton = item->getSkeletonInstance();
					if (nullptr != skeleton)
					{
						this->headBoneV2 = skeleton->getBone(this->headBoneName->getListSelectedValue());

						// If there is no neck bone
						Ogre::Bone* neckBone = this->headBoneV2->getParent();
						if (nullptr == neckBone)
						{
							Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[LookAfterComponent] Warning the given bone: " + this->headBone->getName()
								+ " has no parent for " + this->gameObjectPtr->getName() + ". Look after component will not work!");
							this->headBone = nullptr;
							return true;
						}

						this->headBone->reset();
						// Existing animations have control of the bones of the skeleton when they are running. 
						// Each bone has an animation track for each animation which must be destroyed.

			// Attention: Problem: All animation of all mesh copies will be destroyed!
						// Set also the bone to manually controlled in order to modify it.
						// this->headBone->setManuallyControlled(true);
						const auto animations = skeleton->getAnimations();

						for (auto& anim : skeleton->getAnimationsNonConst())
						{
							// anim._applyAnimation
							// Will not work, because animations cannot be modified for items?
							// anim->destroyOldNodeTrack(headBone->getHandle());
						}
					}
				}
			}
		}
		return true;
	}

	bool LookAfterComponent::disconnect(void)
	{
		if (nullptr != this->headBone)
		{
			NOWA::GraphicsModule::getInstance()->removeTrackedOldBone(this->headBone);

			this->headBone->reset();
			this->headBone = nullptr;
		}
		if (nullptr != this->headBoneV2)
		{
			this->headBoneV2 = nullptr;
		}
		return true;
	}

	bool LookAfterComponent::onCloned(void)
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
	
	void LookAfterComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			if (nullptr != this->targetSceneNode && nullptr != this->headBone && true == this->activated->getBool())
			{
				Ogre::v1::OldNode* neckBone = this->headBone->getParent();
				// Get head position in world space
				Ogre::Vector3 headBonePosition = this->gameObjectPtr->getSceneNode()->convertLocalToWorldPosition(this->headBone->_getDerivedPosition());
				Ogre::Vector3 objectPosition = this->targetSceneNode->getPosition();

				// Calculate direction vector from head to target (world space)
				Ogre::Vector3 directionToTarget = (objectPosition - headBonePosition).normalisedCopy();

				// Get the game object's forward direction in world space
				Ogre::Vector3 gameObjectForward = this->gameObjectPtr->getSceneNode()->getOrientation() * this->gameObjectPtr->getDefaultDirection();

				// Check if target is behind the game object (dot product < 0)
				Ogre::Real dotProduct = gameObjectForward.dotProduct(directionToTarget);

				Ogre::Quaternion currentOrientation = this->headBone->getOrientation();
				Ogre::Quaternion defaultOrientation = this->headBone->getInitialOrientation();
				Ogre::Real smoothingFactor = 0.01f; // Adjust this value to control smoothing speed

				if (dotProduct >= 0) // Target is in front of or at the side of the game object
				{
					// Get neck world-space orientation
					Ogre::Quaternion neckWorldOrientation = this->gameObjectPtr->getSceneNode()->convertLocalToWorldOrientation(neckBone->_getDerivedOrientation());

					// Compute the target head orientation in world space
					Ogre::Vector3 headForward = directionToTarget; // The direction the head should face
					Ogre::Vector3 neckUp = neckWorldOrientation.xAxis(); // Neck's "up" vector
					Ogre::Vector3 headRight = neckUp.crossProduct(headForward).normalisedCopy(); // Right vector of head
					Ogre::Vector3 headUp = headForward.crossProduct(headRight); // Up vector of head

					// Construct the quaternion for target head orientation in world space
					Ogre::Quaternion targetWorldOrientation(headUp, headForward, headRight);
					targetWorldOrientation.normalise();

					// Convert target rotation to neck space
					Ogre::Quaternion targetLocalOrientation = neckWorldOrientation.Inverse() * targetWorldOrientation;

					// Extract yaw and pitch from the target orientation
					Ogre::Vector3 targetDirection = targetLocalOrientation * Ogre::Vector3::UNIT_Y;

					// Pitch calculation uses asin, handling the Z-axis (vertical) component
					Ogre::Real pitch = Ogre::Math::ASin(targetDirection.z).valueDegrees();

					// Yaw derived from the X and Y components
					Ogre::Real yaw = Ogre::Math::ATan2(targetDirection.x, targetDirection.y).valueDegrees();

					// Clamp pitch and yaw to limits
					pitch = Ogre::Math::Clamp(pitch, -this->maxPitch->getReal(), this->maxPitch->getReal());
					yaw = Ogre::Math::Clamp(yaw, -this->maxYaw->getReal(), this->maxYaw->getReal());

					// Construct final rotation with clamped values
					Ogre::Quaternion finalRotation = Ogre::Quaternion(Ogre::Degree(-yaw), Ogre::Vector3::UNIT_Z) *
						Ogre::Quaternion(Ogre::Degree(pitch), Ogre::Vector3::UNIT_X);

					// Compute angular difference using ToAngleAxis
					Ogre::Quaternion rotationBetween = finalRotation;
					Ogre::Radian angle;
					Ogre::Vector3 axis;
					rotationBetween.ToAngleAxis(angle, axis);
					Ogre::Real angleDiff = angle.valueDegrees();

					// Apply smooth interpolation
					Ogre::Real maxAngleThisFrame = this->lookSpeed->getReal() * dt;
					Ogre::Real ratio = 1.0f;
					if (angleDiff > maxAngleThisFrame)
					{
						ratio = maxAngleThisFrame / angleDiff;
					}
					Ogre::Quaternion smoothedRotation = Ogre::Quaternion::Slerp(ratio, currentOrientation, finalRotation, true);
					// this->headBone->setOrientation(smoothedRotation);

					NOWA::GraphicsModule::getInstance()->updateOldBoneOrientation(this->headBone, smoothedRotation);
				}
				else // Target is behind the game object
				{
					// Smoothly interpolate back to default orientation
					Ogre::Quaternion smoothedRotation = Ogre::Quaternion::Slerp(smoothingFactor, currentOrientation, defaultOrientation, true);
					// this->headBone->setOrientation(smoothedRotation);
					NOWA::GraphicsModule::getInstance()->updateOldBoneOrientation(this->headBone, smoothedRotation);
				}
			}
		}
	}
	
	GameObjectCompPtr LookAfterComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		LookAfterCompPtr clonedCompPtr(boost::make_shared<LookAfterComponent>());
		
		clonedCompPtr->setActivated(this->activated->getBool());
		clonedCompPtr->setHeadBoneName(this->headBoneName->getListSelectedValue());
		clonedCompPtr->setTargetId(this->targetId->getULong());
		clonedCompPtr->setLookSpeed(this->lookSpeed->getReal());
		clonedCompPtr->setMaxPitch(this->maxPitch->getReal());
		clonedCompPtr->setMaxYaw(this->maxYaw->getReal());
		
		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	void LookAfterComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (LookAfterComponent::AttrActivated() == attribute->getName())
		{
			this->setActivated(attribute->getBool());
		}
		else if (LookAfterComponent::AttrHeadBoneName() == attribute->getName())
		{
			this->setHeadBoneName(attribute->getListSelectedValue());
		}
		else if (LookAfterComponent::AttrTargetId() == attribute->getName())
		{
			this->setTargetId(attribute->getULong());
		}
		else if (LookAfterComponent::AttrLookSpeed() == attribute->getName())
		{
			this->setLookSpeed(attribute->getReal());
		}
		else if (LookAfterComponent::AttrMaxPitch() == attribute->getName())
		{
			this->setMaxPitch(attribute->getReal());
		}
		else if (LookAfterComponent::AttrMaxYaw() == attribute->getName())
		{
			this->setMaxYaw(attribute->getReal());
		}
	}

	void LookAfterComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "HeadBoneName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->headBoneName->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TargetId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->targetId->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "LookSpeed"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->lookSpeed->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MaxPitch"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxPitch->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MaxYaw"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->maxYaw->getReal())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String LookAfterComponent::getClassName(void) const
	{
		return "LookAfterComponent";
	}

	Ogre::String LookAfterComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void LookAfterComponent::setActivated(bool activated)
	{
		this->activated->setValue(activated);
	}

	bool LookAfterComponent::isActivated(void) const
	{
		return this->activated->getBool();
	}

	void LookAfterComponent::setHeadBoneName(const Ogre::String& headBoneName)
	{
		this->headBoneName->setListSelectedValue(headBoneName);
	}
	
	Ogre::String LookAfterComponent::getHeadBoneName(void) const
	{
		return this->headBoneName->getListSelectedValue();
	}
	
	void LookAfterComponent::setTargetId(unsigned long targetId)
	{
		this->targetId->setValue(targetId);
	}
	
	unsigned long LookAfterComponent::getTargetId(void) const
	{
		return this->targetId->getULong();
	}
	
	void LookAfterComponent::setLookSpeed(Ogre::Real lookSpeed)
	{
		this->lookSpeed->setValue(lookSpeed);
	}
	
	Ogre::Real LookAfterComponent::getLookSpeed(void) const
	{
		return this->lookSpeed->getReal();
	}
	
	void LookAfterComponent::setMaxPitch(Ogre::Real maxPitch)
	{
		this->maxPitch->setValue(maxPitch);
	}
	
	Ogre::Real LookAfterComponent::getMaxPitch(void) const
	{
		return this->maxPitch->getReal();
	}
		
	void LookAfterComponent::setMaxYaw(Ogre::Real maxYaw)
	{
		this->maxYaw->setValue(maxYaw);
	}
	
	Ogre::Real LookAfterComponent::getMaxYaw(void) const
	{
		return this->maxYaw->getReal();
	}
	
	Ogre::SceneNode* LookAfterComponent::getTargetSceneNode(void) const
	{
		return this->targetSceneNode;
	}
		
	Ogre::v1::OldBone* LookAfterComponent::getHeadBone(void) const
	{
		return this->headBone;
	}

	// Lua registration part

	LookAfterComponent* getLookAfterComponent(GameObject* gameObject, unsigned int occurrenceIndex)
	{
		return makeStrongPtr<LookAfterComponent>(gameObject->getComponentWithOccurrence<LookAfterComponent>(occurrenceIndex)).get();
	}

	LookAfterComponent* getLookAfterComponent(GameObject* gameObject)
	{
		return makeStrongPtr<LookAfterComponent>(gameObject->getComponent<LookAfterComponent>()).get();
	}

	LookAfterComponent* getLookAfterComponentFromName(GameObject* gameObject, const Ogre::String& name)
	{
		return makeStrongPtr<LookAfterComponent>(gameObject->getComponentFromName<LookAfterComponent>(name)).get();
	}

	void setTargetId(LookAfterComponent* instance, const Ogre::String& id)
	{
		instance->setTargetId(Ogre::StringConverter::parseUnsignedLong(id));
	}

	void LookAfterComponent::createStaticApiForLua(lua_State* lua, class_<GameObject>& gameObjectClass, class_<GameObjectController>& gameObjectControllerClass)
	{
		module(lua)
		[
			class_<LookAfterComponent, GameObjectComponent>("LookAfterComponent")
			.def("setHeadBoneName", &LookAfterComponent::setHeadBoneName)
			.def("getHeadBoneName", &LookAfterComponent::getHeadBoneName)
			.def("setTargetId", &setTargetId)
			.def("setLookSpeed", &LookAfterComponent::setLookSpeed)
			.def("getLookSpeed", &LookAfterComponent::getLookSpeed)
			.def("setMaxPitch", &LookAfterComponent::setMaxPitch)
			.def("getMaxPitch", &LookAfterComponent::getMaxPitch)
			.def("setMaxYaw", &LookAfterComponent::setMaxYaw)
			.def("getMaxYaw", &LookAfterComponent::getMaxYaw)
		];

		LuaScriptApi::getInstance()->addClassToCollection("LookAfterComponent", "class inherits GameObjectComponent", LookAfterComponent::getStaticInfoText());
		LuaScriptApi::getInstance()->addClassToCollection("LookAfterComponent", "void setHeadBoneName(String headBoneName)", "Sets the head bone name of the skeleton file, which should be rotated.");
		LuaScriptApi::getInstance()->addClassToCollection("LookAfterComponent", "String getHeadBoneName()", "Gets the head bone name of the skeleton file, which should be rotated.");
		LuaScriptApi::getInstance()->addClassToCollection("LookAfterComponent", "void setTargetId(String id)", "Sets target id, at which this game object bone head should be rotated to.");
		LuaScriptApi::getInstance()->addClassToCollection("LookAfterComponent", "void setLookSpeed(number lookSpeed)", "Sets the look (rotation) speed.");
		LuaScriptApi::getInstance()->addClassToCollection("LookAfterComponent", "number getLookSpeed()", "Gets the look (rotation) speed.");
		LuaScriptApi::getInstance()->addClassToCollection("LookAfterComponent", "void setMaxPitch(number maxPitch)", "Sets the maximum degree of pitch for rotation.");
		LuaScriptApi::getInstance()->addClassToCollection("LookAfterComponent", "number getMaxPitch()", "Gets the maximum degree of pitch for rotation.");
		LuaScriptApi::getInstance()->addClassToCollection("LookAfterComponent", "void setMaxYaw(number maxYaw)", "Sets the maximum degree of yaw for rotation.");
		LuaScriptApi::getInstance()->addClassToCollection("LookAfterComponent", "number getMaxYaw()", "Gets the maximum degree of yaw for rotation.");

		gameObjectClass.def("getLookAfterComponentFromName", &getLookAfterComponentFromName);
		gameObjectClass.def("getLookAfterComponent", (LookAfterComponent * (*)(GameObject*)) & getLookAfterComponent);

		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "LookAfterComponent getLookAfterComponent()", "Gets the component. This can be used if the game object this component just once.");
		LuaScriptApi::getInstance()->addClassToCollection("GameObject", "LookAfterComponent getLookAfterComponentFromName(String name)", "Gets the component from name.");

		gameObjectControllerClass.def("castLookAfterComponent", &GameObjectController::cast<LookAfterComponent>);
		LuaScriptApi::getInstance()->addClassToCollection("GameObjectController", "LookAfterComponent castLookAfterComponent(LookAfterComponent other)", "Casts an incoming type from function for lua auto completion.");
	}

	bool LookAfterComponent::canStaticAddComponent(GameObject* gameObject)
	{
		if (gameObject->getComponentCount<LookAfterComponent>() < 2)
		{
			return true;
		}

		// Checks if the entity has at least one animation and no player controller, else animation component is senseless
		Ogre::v1::Entity* entity = gameObject->getMovableObject<Ogre::v1::Entity>();
		if (nullptr != entity)
		{
			Ogre::v1::AnimationStateSet* set = entity->getAllAnimationStates();
			if (nullptr != set)
			{
				Ogre::v1::AnimationStateIterator it = set->getAnimationStateIterator();
				// list all animations
				if (it.hasMoreElements())
				{
					return true;
				}
			}
		}
		return false;
	}
	
}; // namespace end
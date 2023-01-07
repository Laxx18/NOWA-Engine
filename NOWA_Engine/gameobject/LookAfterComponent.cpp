#include "NOWAPrecompiled.h"
#include "LookAfterComponent.h"

#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	LookAfterComponent::LookAfterComponent()
		: GameObjectComponent(),
		targetSceneNode(nullptr),
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

	bool LookAfterComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

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

				unsigned short numBones = entity->getSkeleton()->getNumBones();
				for (unsigned short iBone = 0; iBone < numBones; iBone++)
				{
					Ogre::v1::OldBone* bone = entity->getSkeleton()->getBone(iBone);
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
			
		return true;
	}

	bool LookAfterComponent::connect(void)
	{
		GameObjectPtr targetGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->targetId->getULong());
		if (nullptr != targetGameObjectPtr)
		{
			this->targetSceneNode = targetGameObjectPtr->getSceneNode();
		}

		Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
		if (nullptr == entity)
			return false;

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
		return true;
	}

	bool LookAfterComponent::disconnect(void)
	{
		if (nullptr != this->headBone)
			this->headBone->reset();
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
				// Get the head to be facing the camera, along the vector between the camera and the head this is done in world space for simplicity
				Ogre::Vector3 headBonePosition = this->gameObjectPtr->getSceneNode()->convertLocalToWorldPosition(this->headBone->_getDerivedPosition());
				
				Ogre::Vector3 objectPosition = this->targetSceneNode->getPosition();
				Ogre::Vector3 between = objectPosition - headBonePosition;
				Ogre::Vector3 unitBetween = between.normalisedCopy();
				
				// Now let's get the neckBone because that is the required space to orient the head in. Again, get the world orientation because this is a convenient space to work in.
				
				Ogre::Quaternion neckBoneWorldOrientation = this->gameObjectPtr->getSceneNode()->convertLocalToWorldOrientation(neckBone->_getDerivedOrientation());

				// http://wiki.ogre3d.org/Make+A+Character+Look+At+The+Camera
				
				// Now build a coordinate system for head relative to the neck. An orthogonal coordinate system IS A ROTATION! 
				// The head is defined to have it's x-axis aligned with the neck, y axis pointing straight out the front of th head between the eyes and z axis pointing towards the left ear.
				// Do not induce any roll(rotation about y in my case) in the head relative to the neck.
				// Note: Cameras use the UP (0,1,0) vector for this, but that will only work for a character if the character is standing straight up! What if the character was lying down...doesn't work then...
				// Basically it's a bunch of cross products. look x up = right, look x right = new up.
				// The cross product of two vectors is a vector orthogonal to the original two...pretty convenient for building orthoganol coordinate systems, huh?
				// The reason we use the neck up is that this enforces that there will be no roll about the neck axis because the "right" vector will always be orthogonal to it. 
				// Remember, we're working completely in world space right now. This is probably the hardest part for people to understand, so think about it clearly.

				Ogre::Vector3 headForward = unitBetween;
				Ogre::Vector3 neckUp = neckBoneWorldOrientation.xAxis();
				Ogre::Vector3 headRight = neckUp.crossProduct(headForward);
				Ogre::Vector3 headUp = headForward.crossProduct(headRight);

				// Put them together. Remember, up is x, forward is y, right is z. We also normalize the rotation because of possible numerical errors in computing the cross product.
				Ogre::Quaternion rot(headUp, headForward, headRight);
				rot.normalise(); //might have gotten messed up

				// Now put the rotation into neck space.
				rot = neckBoneWorldOrientation.Inverse() * rot;
				
				// So now we have a rotation, but a head can't go out of a certain range...I don't know what this is, IANAD...figure it out yourself, 
				// or use something sensible, like PI/2 for both. Because of the way I set up my skeleton, and how ogre defines yaw, pitch and roll (rotation about y, x, z respectively), 
				// the code looks strange, but I assure you it works. The head is rotated 180 about the neck x-axis, that's why we have the PI-abs(yawNeckSpace) in there.
				Ogre::Real pitchNeckSpace = rot.getRoll().valueDegrees();
				Ogre::Real yawNeckSpace = rot.getPitch().valueDegrees();

				// if(Ogre::Math::Abs(pitchNeckSpace) > this->maxPitch->getReal() || (/*180.0f -*/ Ogre::Math::Abs(yawNeckSpace)) > this->maxYaw->getReal())
				// {
				// 	return;
				// }

				// Note:If you don't care about smoothly moving the head, skip to the end.
				// We need to figure out how much of an angular change the head movement is (we're only going to consider one angle...if you want separate speeds for yaw, pitch, and roll, 
				// figure it out yourself and add to this how-to). The way to do this is basically if B = A + ? then ? = B-A. We have B (the desired rotation) and A (the current rotation), so it's simple quaternion algebra.
				Ogre::Vector3 currentDirection = headBone->getOrientation() * this->gameObjectPtr->getDefaultDirection();
				Ogre::Quaternion currentOrientation = headBone->getOrientation();

				Ogre::Quaternion destinationOrientation = currentDirection.getRotationTo(unitBetween) * currentOrientation;

				Ogre::Quaternion rotationBetween = rot * headBone->getOrientation().Inverse();
				Ogre::Radian angle;
				Ogre::Vector3 axis;
				rotationBetween.ToAngleAxis(angle, axis);
				Ogre::Real angleDeg = angle.valueDegrees();
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[LookAfterComponent] angleDeg: " + Ogre::StringConverter::toString(angleDeg));

				// Ok, we're in the home stretch. We want to create a smooth rotation. We use Slerp for this. We determine how far along the animation we are by the ratio of the farthest we can rotate in this frame and the total rotation. Make sure you use the shortestPath=true option in slerp or you could get exorcist-like results. "lookSpeed" is in radians per second, deltaT is in seconds

				Ogre::Real maxAngleThisFrame = this->lookSpeed->getReal() * dt;
				Ogre::Real ratio = 1.0f;
				if(angleDeg > maxAngleThisFrame)
				{
					ratio = maxAngleThisFrame / angle.valueDegrees();
				}
				rot = Ogre::Quaternion::Slerp(ratio, headBone->getOrientation(), destinationOrientation, true);
				this->headBone->setOrientation(rot);
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

	void LookAfterComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->activated->getBool())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "HeadBoneName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->headBoneName->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TargetId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->targetId->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "LookSpeed"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->lookSpeed->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MaxPitch"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->maxPitch->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "MaxYaw"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->maxYaw->getReal())));
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
	
}; // namespace end
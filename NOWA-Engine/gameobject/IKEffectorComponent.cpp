//
// Copyright (c) 2008-2018 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

// Modified and adapted by Lukas Kalinowski for NOWA-Engine

#include "NOWAPrecompiled.h"
#include "IKEffectorComponent.h"
#include "GameObjectController.h"
#include "PhysicsActiveComponent.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "AnimationComponent.h"
#include "IKSolverComponent.h"

namespace
{
	vec3_t ogreVector3ToIK(const Ogre::Vector3& vec)
	{
		vec3_t ret;
		ret.v.x = vec.x_;
		ret.v.y = vec.y_;
		ret.v.z = vec.z_;
		return ret;
	}

	Vector3 vec3IKToOgre(const vec3_t* ik)
	{
		return Ogre::Vector3(ik->v.x, ik->v.y, ik->v.z);
	}

	quat_t OgreQuaternionToIK(const Ogre::Quaternion& quat)
	{
		quat_t ret;
		ret.q.x = quat.x_;
		ret.q.y = quat.y_;
		ret.q.z = quat.z_;
		ret.q.w = quat.w_;
		return ret;
	}

	Quaternion QuatIKToOgre(const quat_t* ik)
	{
		return Ogre::Quaternion(ik->q.w, ik->q.x, ik->q.y, ik->q.z);
	}
}

namespace NOWA
{
	using namespace rapidxml;
	
	IKEffectorComponent::IKEffectorComponent()
		: GameObjectComponent(),
		effectorNode(nullptr),
		options(0),
		alreadyConnected(false),
		targetPosition(Ogre::Vector3::ZERO),
		targetRotation(Ogre::Quaternion::IDENTITY),
		targetBoneName(new Variant(IKEffectorComponent::AttrTargetBoneName(), std::vector<Ogre::String>(), this->attributes)),
		chainLength(new Variant(IKEffectorComponent::AttrChainLength(), static_cast<unsigned int>(2), this->attributes)),
		weight(new Variant(IKEffectorComponent::AttrWeight(), 1.0f, this->attributes)),
		rotationWeight(new Variant(IKEffectorComponent::AttrRotationWeight(), 1.0f, this->attributes)),
		rotationDecay(new Variant(IKEffectorComponent::AttrRotationDecay(), 0.25f, this->attributes)),
		weightNLerp(new Variant(IKEffectorComponent::AttrWeightNLerp(), false, this->attributes)),
		inheritParentRotation(new Variant(IKEffectorComponent::AttrInheritParentRotation(), false, this->attributes)),
		stiffness(new Variant(IKEffectorComponent::AttrStiffness(), 0.5f, this->attributes)),
		stretchness(new Variant(IKEffectorComponent::AttrStiffness(), 1.0f, this->attributes)),
		lengthConstraints(new Variant(IKEffectorComponent::AttrLengthConstraints(), Ogre::Vector2::ZERO, this->attributes))
		// debugGeometryNode(nullptr),
		// debugGeometryEntity(nullptr),
	{
		// this->targetBoneName->setDescription("If this game object uses several TagPoint-Components data may not be tagged to the same bone name");
	}

	IKEffectorComponent::~IKEffectorComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[IKEffectorComponent] Destructor ik effector component for game object: " + this->gameObjectPtr->getName());
		
		// this->targetBone = nullptr;
		/*if (nullptr != this->debugGeometryNode)
		{
			this->debugGeometryNode->detachAllObjects();
			this->gameObjectPtr->getSceneManager()->destroySceneNode(this->debugGeometryNode);
			this->debugGeometryNode = nullptr;
			this->gameObjectPtr->getSceneManager()->destroyMovableObject(this->debugGeometryEntity);
			this->debugGeometryEntity = nullptr;
		}*/
	}

	bool IKEffectorComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		propertyElement = propertyElement->next_sibling("property");
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TargetBoneName")
		{
			this->targetBoneName->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "ChainLength")
		{
			this->chainLength->setValue(XMLConverter::getAttribUnsignedInt(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Weight")
		{
			this->weight->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RotationWeight")
		{
			this->rotationWeight->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "RotationDecay")
		{
			this->rotationDecay->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "WeightNLerp")
		{
			this->weightNLerp->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "InheritParentRotation")
		{
			this->inheritParentRotation->setValue(XMLConverter::getAttribBool(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Stiffness")
		{
			this->stiffness->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Stretchness")
		{
			this->stretchness->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "LengthConstraints")
		{
			this->lengthConstraints->setValue(XMLConverter::getAttribVector2(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr IKEffectorComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		TagPointCompPtr clonedCompPtr(boost::make_shared<IKEffectorComponent>());

		clonedCompPtr->setTargetBoneName(this->targetBoneName->getListSelectedValue());
		clonedCompPtr->setChainLength(this->chainLength->getUInt());
		clonedCompPtr->setWeight(this->weight->getReal());
		clonedCompPtr->setRotationWeight(this->rotationWeight->getReal());
		clonedCompPtr->setRotationDecay(this->rotationDecay->getReal());
		clonedCompPtr->setWeightNLerp(this->weightNLerp->getBool());
		clonedCompPtr->setInheritParentRotation(this->inheritParentRotation->getBool());
		clonedCompPtr->setStiffness(this->stiffness->getReal());
		clonedCompPtr->setStretchness(this->stretchness->getReal());
		clonedCompPtr->setLengthConstraints(this->lengthConstraints->getVector2());
		
		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		return clonedCompPtr;
	}

	bool IKEffectorComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[IKEffectorComponent] Init ik effector component for game object: " + this->gameObjectPtr->getName());

		Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
		if (nullptr != entity)
		{

			Ogre::v1::Skeleton* skeleton = entity->getSkeleton();
			if (nullptr != skeleton)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[IKEffectorComponent] List all bones for mesh '" + entity->getMesh()->getName() + "':");

				std::vector<Ogre::String> targetBoneNames;

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
						for (size_t i = 0; i < targetBoneNames.size(); i++)
						{
							if (targetBoneNames[i] == bone->getName())
							{
								unique = false;
								break;
							}
						}
						if (true == unique)
						{
							targetBoneNames.emplace_back(bone->getName());
							Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[IKEffectorComponent] Bone name: " + bone->getName());
						}
					}
					else
					{
						bool unique = true;
						for (size_t i = 0; i < targetBoneNames.size(); i++)
						{
							if (targetBoneNames[i] == bone->getName())
							{
								unique = false;
								break;
							}
						}
						if (true == unique)
						{
							targetBoneNames.emplace_back(bone->getName());
							Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[IKEffectorComponent] Bone name: " + bone->getName());
						}
					}
				}

				// Add all available tag names to list
				this->targetBoneName->setValue(targetBoneNames);
			}
			
			// Note: IKSolverComponent must be placed before any IKEffectorComponent!
			/*auto& ikSolverCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<IKSolverComponent>());
			if (nullptr == ikSolverCompPtr)
			{
				this->solver = ikSolverCompPtr->getSolver();
			}*/
		}
		return true;
	}

	bool IKEffectorComponent::connect(void)
	{
		if (false == alreadyConnected)
		{
			auto& animationCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<AnimationComponent>());
			if (nullptr == animationCompPtr)
			{
				// Sent event with feedback
				boost::shared_ptr<EventDataFeedback> eventDataNavigationMeshFeedback(new EventDataFeedback(false, "#{AnimationComponentMissing} -> : " + this->gameObjectPtr->getName()));
				NOWA::EventManager::getInstance()->queueEvent(eventDataNavigationMeshFeedback);
			}
			
			Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
			if (nullptr == entity)
				return false;

			Ogre::v1::OldSkeletonInstance* oldSkeletonInstance = entity->getSkeleton();
			if (nullptr != oldSkeletonInstance)
			{
				Ogre::v1::OldBone* bone = this->skeleton->getBone(i);
				Ogre::Quaternion orientation = bone->_getDerivedOrientation();
				// Ogre::Quaternion orientation = this->gameObjectPtr->getSceneNode()->convertLocalToWorldOrientation(bone->_getDerivedOrientation());
				bone->setManuallyControlled(true);
				bone->setInheritOrientation(false);
				bone->setOrientation(orientation);
				
				/*if (nullptr != this->tagPoint)
				{
					oldSkeletonInstance->freeTagPoint(this->tagPoint);
					this->tagPoint = nullptr;
				}

				GameObjectPtr sourceGameObjectPtr = GameObjectController::getInstance()->getGameObjectFromId(this->sourceId->getULong());
				if (nullptr == sourceGameObjectPtr)
					return false;

				this->sourcePhysicsActiveCompPtr = NOWA::makeStrongPtr(sourceGameObjectPtr->getComponent<PhysicsActiveComponent>());

				// Does not work anymore, as it is done different in Ogre2.x (see tagpoint example, for that new skeleton and item is necessary, updgrade of meshes)
				// this->tagPoint = entity->attachObjectToBone(this->tagPoints->getListSelectedValue(), sourceEntity, 
				// 	MathHelper::getInstance()->degreesToQuat(this->offsetOrientation->getVector3()),this->offsetPosition->getVector3());

				// Workaround:
				this->tagPoint = oldSkeletonInstance->createTagPointOnBone(oldSkeletonInstance->getBone(this->tagPoints->getListSelectedValue()));
				this->tagPoint->setScale(this->gameObjectPtr->getSceneNode()->getScale());

				this->tagPointNode = this->gameObjectPtr->getSceneNode()->createChildSceneNode(Ogre::SCENE_DYNAMIC);
				this->tagPointNode->setScale(this->gameObjectPtr->getSceneNode()->getScale());
				this->tagPointNode->setPosition(this->tagPointNode->getPosition() * this->tagPointNode->getScale());
				this->tagPoint->setPosition(this->tagPoint->getPosition() * this->tagPoint->getScale());

				std::vector<Ogre::MovableObject*> movableObjects;
				Ogre::SceneNode* sourceSceneNode = sourceGameObjectPtr->getSceneNode();
				auto& it = sourceSceneNode->getAttachedObjectIterator();
				// First detach and reatach all movable objects, that are belonging to the source scene node (e.g. mesh, light)
				while (it.hasMoreElements())
				{
					// It is const, so no manipulation possible
					Ogre::MovableObject* movableObject = it.getNext();
					movableObjects.emplace_back(movableObject);
				}

				for (size_t i = 0; i < movableObjects.size(); i++)
				{
					movableObjects[i]->detachFromParent();
					this->tagPointNode->attachObject(movableObjects[i]);
				}

				this->tagPoint->setListener(new TagPointListener(this->tagPointNode, this->offsetPosition->getVector3(), MathHelper::getInstance()->degreesToQuat(this->offsetOrientation->getVector3())));*/

				this->alreadyConnected = true;
			}
		}
		return true;
	}

	bool IKEffectorComponent::disconnect(void)
	{
		if (nullptr != this->tagPointNode)
		{
			Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
			if (nullptr != entity)
			{
				//Remove associated tag point, we don't need it anymore
				Ogre::v1::OldSkeletonInstance* oldSkeletonInstance = entity->getSkeleton();
				if (nullptr != oldSkeletonInstance)
				{
					/*Ogre::v1::OldNode::Listener* nodeListener = this->tagPoint->getListener();
					this->tagPoint->setListener(nullptr);
					delete nodeListener;

					GameObjectPtr sourceGameObjectPtr = GameObjectController::getInstance()->getGameObjectFromId(this->sourceId->getULong());
					if (nullptr != sourceGameObjectPtr)
					{
						std::vector<Ogre::MovableObject*> movableObjects;
						auto& it = this->tagPointNode->getAttachedObjectIterator();
						// First detach and reatach all movable objects, that are belonging to the source scene node (e.g. mesh, light)
						while (it.hasMoreElements())
						{
							Ogre::MovableObject* movableObject = it.getNext();
							movableObjects.emplace_back(movableObject);
						}
						for (size_t i = 0; i < movableObjects.size(); i++)
						{
							movableObjects[i]->detachFromParent();
							sourceGameObjectPtr->getSceneNode()->attachObject(movableObjects[i]);
						}
					}

					this->gameObjectPtr->getSceneManager()->destroySceneNode(this->tagPointNode);
					this->tagPointNode = nullptr;

					oldSkeletonInstance->freeTagPoint(this->tagPoint);
					this->tagPoint = nullptr;*/

					this->alreadyConnected = false;
				}
			}
		}
		return true;
	}
	
	void IKEffectorComponent::onRemoveComponent(void)
	{
		// Do here something??
	}

	void IKEffectorComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			/*if (nullptr != this->sourcePhysicsActiveCompPtr && nullptr != this->tagPoint)
			{
				this->sourcePhysicsActiveCompPtr->setPosition(this->tagPoint->getPosition());
				this->sourcePhysicsActiveCompPtr->setOrientation(this->tagPoint->getOrientation());
			}*/
		}
	}

	void IKEffectorComponent::actualizeValue(Variant* attribute)
	{
		if (IKEffectorComponent::AttrTargetBoneName() == attribute->getName())
		{
			this->setTargetBoneName(attribute->getListSelectedValue());
		}
		else if (IKEffectorComponent::AttrChainLength() == attribute->getName())
		{
			this->setChainLength(attribute->getUInt());
		}
		else if (IKEffectorComponent::AttrWeight() == attribute->getName())
		{
			this->setWeight(attribute->getReal());
		}
		else if (IKEffectorComponent::AttrRotationWeight() == attribute->getName())
		{
			this->setRotationWeight(attribute->getReal());
		}
		else if (IKEffectorComponent::AttrWeightNLerp() == attribute->getName())
		{
			this->setWeightNLerp(attribute->getBool());
		}
		else if (IKEffectorComponent::AttrInheritParentRotation() == attribute->getName())
		{
			this->setInheritParentRotation(attribute->getBool());
		}
		else if (IKEffectorComponent::AttrStiffness() == attribute->getName())
		{
			this->setStiffness(attribute->getReal());
		}
		else if (IKEffectorComponent::AttrStrechness() == attribute->getName())
		{
			this->setStrechness(attribute->getReal());
		}
		else if (IKEffectorComponent::AttrLengthConstraints() == attribute->getName())
		{
			this->setLengthConstraints(attribute->getVector2());
		}
	}

	void IKEffectorComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		xml_node<>* propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ComponentIKEffector"));
		propertyXML->append_attribute(doc.allocate_attribute("data", "IKEffectorComponent"));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "TargetBoneName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->targetBoneName->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "2"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "ChainLength"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->chainLength->getUInt())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Weight"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->weight->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RotationWeight"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->rotationWeight->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "RotationDecay"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->rotationDecay->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "WeightNLerp"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->weightNLerp->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "InheritParentRotation"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->inheritParentRotation->getBool())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Stiffness"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->stiffness->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Strechness"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->strechness->getReal())));
		propertiesXML->append_node(propertyXML);
		
		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "8"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "LengthConstraints"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->lengthConstraints->getVector2())));
		propertiesXML->append_node(propertyXML);
	}

	void IKEffectorComponent::showDebugData(bool bShow)
	{
		if (true == bShow)
		{
			/*if (nullptr == this->debugGeometryNode && nullptr != this->tagPointNode)
			{
				this->debugGeometryNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode(Ogre::SCENE_DYNAMIC);
				this->debugGeometryNode->setName("IKEffectorComponentArrowNode");
				this->debugGeometryNode->setPosition(this->tagPointNode->getPosition() + this->gameObjectPtr->getSceneNode()->getPosition());
				this->debugGeometryNode->setOrientation(this->tagPointNode->getOrientation() * this->gameObjectPtr->getSceneNode()->getOrientation());
				this->debugGeometryNode->setScale(0.1f, 0.1f, 0.1f);
				this->debugGeometryEntity = this->gameObjectPtr->getSceneManager()->createEntity("Arrow.mesh");
				this->debugGeometryEntity->setName("IKEffectorComponentEntity");
				this->debugGeometryEntity->setDatablock("BaseYellowLine");
				this->debugGeometryEntity->setQueryFlags(0 << 0);
				this->debugGeometryEntity->setCastShadows(false);
				this->debugGeometryNode->attachObject(this->debugGeometryEntity);
			}*/
		}
		else
		{
			/*if (nullptr != this->debugGeometryNode)
			{
				this->debugGeometryNode->detachAllObjects();
				this->gameObjectPtr->getSceneManager()->destroySceneNode(this->debugGeometryNode);
				this->debugGeometryNode = nullptr;
				this->gameObjectPtr->getSceneManager()->destroyMovableObject(this->debugGeometryEntity);
				this->debugGeometryEntity = nullptr;
			}*/
		}
	}

	Ogre::String IKEffectorComponent::getClassName(void) const
	{
		return "IKEffectorComponent";
	}

	Ogre::String IKEffectorComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void IKEffectorComponent::setActivated(bool activated)
	{

	}

	void IKEffectorComponent::setTargetBoneName(const Ogre::String& targetBoneName)
	{
		this->targetBoneName->setListSelectedValue(targetBoneName);

		Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
		if (nullptr != entity)
		{
			Ogre::v1::OldSkeletonInstance* oldSkeletonInstance = entity->getSkeleton();
			if (nullptr != oldSkeletonInstance)
			{
				if (oldSkeletonInstance->hasBone(targetBoneName))
				{
					this->targetBone = oldSkeletonInstance->getBone(targetBoneName)
				}
			}
		}
	}
	
	Ogre::String IKEffectorComponent::getTargetBoneName(void) const
	{
		return this->targetBoneName->getListSelectedValue();
	}

	void IKEffectorComponent::setChainLength(unsigned int chainLength)
	{
		this->chainLength->setValue(chainLength);
		
		if (nullptr == this->ikSolverCompPtr)
			return;
		
		if (this->ikEffectorNode != nullptr)
		{
			this->ikEffectorNode->effector->chain_length = chainLength;
			this->ikSolverCompPtr->markChainsNeedUpdating();
		}
	}
	
	unsigned int IKEffectorComponent::getChainLength() const
	{
		return this->chainLength->getUInt();
	}

	void IKEffectorComponent::setWeight(Ogre::Real weight)
	{
		this->weight->setValue(Ogre::Math::Clamp(weight, 0.0f, 1.0f));
		
		if (nullptr == this->ikSolverCompPtr)
			return;
		
		if (this->ikEffectorNode != nullptr)
			this->ikEffectorNode->effector->weight = weight;
	}
	
	Ogre::Real IKEffectorComponent::GetWeight(void) const
	{
		return this->weight->getReal();
	}

	void IKEffectorComponent::setRotationWeight(Ogre::Real rotationWeight)
	{
		this->rotationWeight->setValue(Ogre::Math::Clamp(rotationWeight, 0.0f, 1.0f));
		
		if (nullptr == this->ikSolverCompPtr)
			return;
		
		if (this->ikEffectorNode != nullptr)
		{
			this->ikEffectorNode->rotation_weight = rotationWeight;
			ik_calculate_rotation_weight_decays(&this->ikSolverCompPtr->getSolver()->chain_tree);
		}
	}
	
	Ogre::Real IKEffectorComponent::getRotationWeight(void) const
	{
		return this->rotationWeight->getReal();
	}
	
	void IKEffectorComponent::setRotationDecay(Ogre::Real rotationDecay)
	{
		this->rotationDecay->setValue(Ogre::Math::Clamp(rotationDecay, 0.0f, 1.0f));
		
		if (nullptr == this->ikSolverCompPtr)
			return;
		
		if (this->ikEffectorNode != nullptr)
		{
			this->ikEffectorNode->effector->rotation_decay = rotationDecay;
			ik_calculate_rotation_weight_decays(&this->ikSolverCompPtr->getSolver()->chain_tree);
		}
	}

	Ogre::Real IKEffectorComponent::getRotationDecay(void) const
	{
		return this->rotationDecay->getReal();
	}
	
	void IKEffectorComponent::setWeightNLerp(bool weightNLerp)
	{
		this->weightNLerp->setValue(weightNLerp);
		
		if (this->ikEffectorNode != nullptr)
			this->ikEffectorNode->effector->flags &= ~EFFECTOR_WEIGHT_NLERP;
		
		if (true == weightNLerp)
		{
			if (this->ikEffectorNode != nullptr)
			{
				this->ikEffectorNode->effector->flags |= EFFECTOR_WEIGHT_NLERP;
			}
		}
		
		this->options &= ~EFFECTOR_WEIGHT_NLERP;
		if (true == weightNLerp)
			this->options |= EFFECTOR_WEIGHT_NLERP;
	}
		
	bool IKEffectorComponent::getWeightNLerp(void) const
	{
		return this->weightNLerp->getBool();
	}
		
	void IKEffectorComponent::setInheritParentRotation(bool inheritParentRotation)
	{
		this->inheritParentRotation->setValue(inheritParentRotation);
		// Is not used at the moment?
		this->options &= ~INHERIT_PARENT_ROTATION;
		if (true == weightNLerp)
			this->options |= INHERIT_PARENT_ROTATION;
	}
		
	bool IKEffectorComponent::getInheritParentRotation(void) const
	{
		return this->inheritParentRotation->getBool();
	}
	
	void IKEffectorComponent::setStiffness(Ogre::Real stiffness)
	{
		this->stiffness->setValue(stiffness);
	}
	
	Ogre::Real IKEffectorComponent::getStiffness(void) const
	{
		return this->stiffness->getReal();
	}
	
	void IKEffectorComponent::setStretchness(Ogre::Real stretchness)
	{
		this->stretchness->setValue(stretchness);
	}
	
	Ogre::Real IKEffectorComponent::getStretchness(void) const
	{
		return this->stretchness->getReal();
	}
	
	void IKEffectorComponent::setLengthConstraints(const Ogre::Vector2& lengthConstraints)
	{
		this->lengthConstraints->setValue(lengthConstraints);
	}
	
	Ogre::Vector2 IKEffectorComponent::getLengthConstraints(void) const
	{
		return this->lengthConstraints->getVector2();
	}
	
	Ogre::v1::OldBone* IKEffectorComponent::getTargetBone(void) const
	{
		return this->targetBone;
	}
	
	void IKEffectorComponent::::setIKSolverComponent(boost::weak_ptr<IKSolverComponent> weakIKSolverComponent)
	{
		this->ikSolverCompPtr = NOWA::makeStrongPtr(weakIKSolverComponent);
	}

	void IKEffectorComponent::SetTargetPosition(const Ogre::Vector3& targetPosition)
	{
		this->targetPosition_ = targetPosition;
		if (this->ikEffectorNode != nullptr)
			this->ikEffectorNode->effector->target_position = OgreVector3ToIK(targetPosition);
	}
	
	Ogre::Vector3 IKEffectorComponent::getTargetPosition(void) const
	{
		return this->targetPosition;
	}

	void IKEffectorComponent::setTargetRotation(const Ogre::Quaternion& targetRotation)
	{
		this->targetRotation = targetRotation;
		if (this->ikEffectorNode)
			this->ikEffectorNode->effector->target_rotation = OgreQuaternionToIK(targetRotation);
	}
	
	const Quaternion& IKEffectorComponent::GetTargetRotation() const
	{
		return targetRotation_;
	}

	void IKEffectorComponent::updateTargetNodePosition(void)
	{
		if (nullptr != this->targetBone)
			return;

		// Try to find a node with the same name as our target name
		this->setTargetNode(node_->GetScene()->GetChild(targetName_, true));
		if (nullptr == this->targetBone)
			return;

		this->setTargetPosition(this->targetBone->_getDerivedPosition());
		this->setTargetRotation(this->targetBone->_getDerivedOrientation());
	}
	
	/*void IKEffectorComponent::DrawDebugGeometry(bool depthTest)
	{
		auto* debug = GetScene()->GetComponent<DebugRenderer>();
		if (debug)
			DrawDebugGeometry(debug, depthTest);
	}*/

	/*void IKEffectorComponent::DrawDebugGeometry(DebugRenderer* debug, bool depthTest)
	{
		Node* terminationNode;
		if (this->solver == nullptr)
			terminationNode = GetScene();
		else
			terminationNode = this->solver->GetNode();

		// Calculate average length of all segments so we can determine the radius
		// of the debug spheres to draw
		int chainLength = chainLength_ == 0 ? -1 : chainLength_;
		Node* a = node_;
		Ogre::Real averageLength = 0.0f;
		unsigned numberOfSegments = 0;
		while (a && a != terminationNode->GetParent() && chainLength-- != 0)
		{
			averageLength += a->GetPosition().Length();
			++numberOfSegments;
			a = a->GetParent();
		}
		averageLength /= numberOfSegments;

		// connect all chained nodes together with lines
		chainLength = chainLength_ == 0 ? -1 : chainLength_;
		a = node_;
		Node* b = a->GetParent();
		debug->AddSphere(
			Sphere(a->_getDerivedPosition(), averageLength * 0.1f),
			Color::YELLOW,
			depthTest
		);
		while (b && b != terminationNode->GetParent() && chainLength-- != 0)
		{
			debug->AddLine(
				a->_getDerivedPosition(),
				b->_getDerivedPosition(),
				Color::WHITE,
				depthTest
			);
			debug->AddSphere(
				Sphere(b->_getDerivedPosition(), averageLength * 0.1f),
				Color::YELLOW,
				depthTest
			);
			a = b;
			b = b->GetParent();
		}

		Vector3 direction = targetRotation_ * Vector3::FORWARD;
		direction = direction * averageLength + targetPosition_;
		debug->AddSphere(Sphere(targetPosition_, averageLength * 0.2f), Color(255, 128, 0), depthTest);
		debug->AddLine(targetPosition_, direction, Color(255, 128, 0), depthTest);
	}*/


	void IKEffectorComponent::SetIKEffectorNode(ik_node_t* effectorNode)
	{
		this->ikEffectorNode = effectorNode;
		if (effectorNode)
		{
			this->ikEffectorNode->effector->target_position = OgreVector3ToIK(this->targetPosition);
			this->ikEffectorNode->effector->target_rotation = OgreQuaternionToIK(this->targetRotation);
			this->ikEffectorNode->effector->weight = this->weight->getReal();
			this->ikEffectorNode->effector->rotation_weight = this->rotationWeight->getReal();
			this->ikEffectorNode->effector->rotation_decay = rotationDecay_;
			this->ikEffectorNode->effector->chain_length = chainLength_;

			if (features_ & WEIGHT_NLERP)
				this->ikEffectorNode->effector->flags |= EFFECTOR_WEIGHT_NLERP;
		}
	}

} // namespace end

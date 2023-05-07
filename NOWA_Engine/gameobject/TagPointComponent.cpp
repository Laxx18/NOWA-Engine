#include "NOWAPrecompiled.h"
#include "TagPointComponent.h"
#include "PhysicsActiveComponent.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "AnimationComponent.h"
#include "JointComponents.h"
#include "PhysicsRagDollComponent.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	// http://forums.ogre3d.org/viewtopic.php?f=4&t=84650#p522055
	// Slow
	class TagPointListener : public Ogre::v1::OldNode::Listener
	{
	public:
		TagPointListener(Ogre::Node* realNode, const Ogre::Vector3& offsetPosition, const Ogre::Quaternion& offsetOrientation, 
			PhysicsActiveComponent* sourcePhysicsActiveComponent, GameObject* gameObject)
			: realNode(realNode),
			offsetPosition(offsetPosition),
			offsetOrientation(offsetOrientation),
			sourcePhysicsActiveComponent(sourcePhysicsActiveComponent),
			gameObject(gameObject),
			jointKinematicComponent(nullptr)
		{
			if (nullptr != sourcePhysicsActiveComponent)
			{
				auto& existingJointKinematicCompPtr = NOWA::makeStrongPtr(this->sourcePhysicsActiveComponent->getOwner()->getComponent<JointKinematicComponent>());
				if (nullptr == existingJointKinematicCompPtr)
				{
					boost::shared_ptr<JointKinematicComponent> jointKinematicCompPtr(boost::make_shared<JointKinematicComponent>());
					this->sourcePhysicsActiveComponent->getOwner()->addDelayedComponent(jointKinematicCompPtr, true);
					jointKinematicCompPtr->setOwner(this->sourcePhysicsActiveComponent->getOwner());
					jointKinematicCompPtr->setBody(this->sourcePhysicsActiveComponent->getBody());
					jointKinematicCompPtr->setPickingMode(3);
					jointKinematicCompPtr->setMaxLinearAngleFriction(10000.0f, 10000.0f);
					jointKinematicCompPtr->createJoint();
					this->jointKinematicComponent = jointKinematicCompPtr.get();
				}
				else
				{
					this->jointKinematicComponent = existingJointKinematicCompPtr.get();
				}
			}
		}

		virtual TagPointListener::~TagPointListener()
		{
			// Will be deleted by goc
			/*if (nullptr != this->jointKinematicComponent)
			{
				this->jointKinematicComponent->releaseJoint();
				this->jointKinematicComponent = nullptr;
			}*/
		}

		Ogre::Vector3 findVelocity(const Ogre::Vector3& aPos, const Ogre::Vector3& bPos, Ogre::Real speed)
		{
			Ogre::Vector3 disp = bPos - aPos;
			float distance = Ogre::Math::Sqrt(disp.x * disp.x + disp.y * disp.y + disp.z * disp.z); // std::hypot(disp.x, disp.y) if C++ 11
			return disp * (speed / distance);
		}

		virtual void OldNodeUpdated(const Ogre::v1::OldNode* updatedNode) override
		{
			if (nullptr == this->sourcePhysicsActiveComponent)
			{
				// Note: realNode is relative to update node, so no need for world transform, calculation is local
				Ogre::Quaternion newOrientation = updatedNode->_getDerivedOrientation() * this->offsetOrientation;
				Ogre::Vector3 newPosition = (updatedNode->_getDerivedPosition() + (newOrientation * this->offsetPosition)) * this->realNode->getScale();

				this->realNode->setOrientation(newOrientation);
				// this->realNode->setScale(updatedNode->_getDerivedScale());
				this->realNode->setPosition(newPosition);
			}
			else
			{
				// Different case: world transform is required, as physics do work on world transform
				Ogre::Vector3 worldPosition = updatedNode->_getDerivedPosition();
				Ogre::Quaternion worldOrientation = updatedNode->_getDerivedOrientation();

				//multiply with the parent derived transformation
				Ogre::Node* parentNode = this->gameObject->getMovableObject()->getParentNode();
				Ogre::SceneNode* sceneNode = this->gameObject->getMovableObject()->getParentSceneNode();
				while (parentNode != nullptr)
				{
					// Process the current i_Node
					if (parentNode != sceneNode)
					{
						// This is a tag point (a connection point between 2 entities). which means it has a parent i_Node to be processed
						worldPosition = ((Ogre::v1::TagPoint*)parentNode)->_getFullLocalTransform() * worldPosition;
						parentNode = ((Ogre::v1::TagPoint*)parentNode)->getParentEntity()->getParentNode();
						worldOrientation = ((Ogre::v1::TagPoint*)parentNode)->_getDerivedOrientation() * worldOrientation;
					}
					else
					{
						// This is the scene i_Node meaning this is the last i_Node to process
						worldPosition = parentNode->_getFullTransform() * worldPosition;
						worldOrientation = parentNode->_getDerivedOrientation() * worldOrientation;
						break;
					}
				}

				Ogre::Quaternion newOrientation = worldOrientation * this->offsetOrientation;
				Ogre::Vector3 newPosition = worldPosition + (newOrientation * this->offsetPosition);

				this->jointKinematicComponent->setTargetPositionRotation(newPosition, newOrientation);
				
				// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[TagPointComponent] orient: " + Ogre::StringConverter::toString(newOrientation));
			}
		}

		virtual void OldNodeDestroyed(const Ogre::v1::OldNode* updatedNode) override
		{
			delete this;
		}
	private:
		Ogre::Node* realNode; //Node where we redirect our updates
		Ogre::Vector3 offsetPosition;
		Ogre::Quaternion offsetOrientation;
		PhysicsActiveComponent* sourcePhysicsActiveComponent;
		JointKinematicComponent* jointKinematicComponent;
		GameObject* gameObject;
	};
	
	TagPointComponent::TagPointComponent()
		: GameObjectComponent(),
		tagPoint(nullptr),
		tagPointNode(nullptr),
		sourcePhysicsActiveComponent(nullptr),
		alreadyConnected(false),
		debugGeometryArrowNode(nullptr),
		debugGeometrySphereNode(nullptr),
		debugGeometryArrowEntity(nullptr),
		debugGeometrySphereEntity(nullptr),
		tagPoints(new Variant(TagPointComponent::AttrTagPointName(), std::vector<Ogre::String>(), this->attributes)),
		sourceId(new Variant(TagPointComponent::AttrSourceId(), static_cast<unsigned long>(0), this->attributes, true)),
		offsetPosition(new Variant(TagPointComponent::AttrOffsetPosition(), Ogre::Vector3::ZERO, this->attributes)),
		offsetOrientation(new Variant(TagPointComponent::AttrOffsetOrientation(), Ogre::Vector3::ZERO, this->attributes))
	{
		this->tagPoints->setDescription("If this game object uses several TagPoint-Components data may not be tagged to the same bone name");
		this->offsetOrientation->setDescription("Orientation is set in the form: (degreeX, degreeY, degreeZ)");
	}

	TagPointComponent::~TagPointComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[TagPointComponent] Destructor tag point component for game object: " + this->gameObjectPtr->getName());
		this->tagPoint = nullptr;
		
		GameObjectPtr sourceGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->sourceId->getULong());
		if (nullptr != sourceGameObjectPtr)
		{
			// Reset pointer
			sourceGameObjectPtr->setConnectedGameObject(boost::weak_ptr<GameObject>());
		}
		this->destroyDebugData();
	}

	bool TagPointComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "TagPointName")
		{
			this->tagPoints->setListSelectedValue(XMLConverter::getAttrib(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "SourceId")
		{
			this->sourceId->setValue(XMLConverter::getAttribUnsignedLong(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OffsetPosition")
		{
			this->offsetPosition->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "OffsetOrientation")
		{
			this->offsetOrientation->setValue(XMLConverter::getAttribVector3(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		return true;
	}

	GameObjectCompPtr TagPointComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		TagPointCompPtr clonedCompPtr(boost::make_shared<TagPointComponent>());

		
		clonedCompPtr->setTagPointName(this->tagPoints->getListSelectedValue());
		clonedCompPtr->setSourceId(this->sourceId->getULong());

		clonedCompPtr->setOffsetPosition(this->offsetPosition->getVector3());
		clonedCompPtr->setOffsetOrientation(this->offsetOrientation->getVector3());
		
		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool TagPointComponent::postInit(void)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[TagPointComponent] Init tag point component for game object: " + this->gameObjectPtr->getName());

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
		if (nullptr != entity)
		{
			Ogre::v1::Skeleton* skeleton = entity->getSkeleton();
			if (nullptr != skeleton)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[TagPointComponent] List all bones for mesh '" + entity->getMesh()->getName() + "':");

				std::vector<Ogre::String> tagPointNames;

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
						for (size_t i = 0; i < tagPointNames.size(); i++)
						{
							if (tagPointNames[i] == bone->getName())
							{
								unique = false;
								break;
							}
						}
						if (true == unique)
						{
							tagPointNames.emplace_back(bone->getName());
							Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[TagPointComponent] Bone name: " + bone->getName());
						}
					}
					else
					{
						bool unique = true;
						for (size_t i = 0; i < tagPointNames.size(); i++)
						{
							if (tagPointNames[i] == bone->getName())
							{
								unique = false;
								break;
							}
						}
						if (true == unique)
						{
							tagPointNames.emplace_back(bone->getName());
							Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[TagPointComponent] Bone name: " + bone->getName());
						}
					}
				}

				// Add all available tag names to list
				this->tagPoints->setValue(tagPointNames);
			}

			this->setTagPointName(this->tagPoints->getListSelectedValue());
		}
		return true;
	}

	bool TagPointComponent::connect(void)
	{
		if (false == alreadyConnected)
		{
			Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
			if (nullptr == entity)
				return false;

			Ogre::v1::OldSkeletonInstance* oldSkeletonInstance = entity->getSkeleton();
			if (nullptr != oldSkeletonInstance)
			{
				GameObjectPtr sourceGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->sourceId->getULong());
				if (nullptr == sourceGameObjectPtr)
					return false;

				// Set back reference (e.g. player/enemy that holds the weapon)
				sourceGameObjectPtr->setConnectedGameObject(this->gameObjectPtr);

				auto& physicsActiveCompPtr = NOWA::makeStrongPtr(sourceGameObjectPtr->getComponent<PhysicsActiveComponent>());
				if (nullptr != physicsActiveCompPtr)
				{
					this->sourcePhysicsActiveComponent = physicsActiveCompPtr.get();
				}

				// if (nullptr == this->sourcePhysicsActiveComponent)
				{
					this->tagPointNode = this->gameObjectPtr->getSceneNode()->createChildSceneNode(Ogre::SCENE_DYNAMIC);
					this->tagPointNode->setScale(this->gameObjectPtr->getSceneNode()->getScale());
					this->tagPointNode->setPosition(this->tagPointNode->getPosition() * this->tagPointNode->getScale());

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

					if (nullptr == this->tagPoint)
					{
						this->tagPoint = oldSkeletonInstance->createTagPointOnBone(oldSkeletonInstance->getBone(this->tagPoints->getListSelectedValue()));
						this->tagPoint->setScale(this->gameObjectPtr->getSceneNode()->getScale());
					}
					this->tagPoint->setListener(new TagPointListener(this->tagPointNode, this->offsetPosition->getVector3(),
												MathHelper::getInstance()->degreesToQuat(this->offsetOrientation->getVector3()), this->sourcePhysicsActiveComponent, this->gameObjectPtr.get()));
				}
				this->alreadyConnected = true;
			}
		}
		return true;
	}

	bool TagPointComponent::disconnect(void)
	{
		this->resetTagPoint();
		return true;
	}

	void TagPointComponent::onRemoveComponent(void)
	{
		GameObjectComponent::onRemoveComponent();
		
		this->resetTagPoint();
	}

	void TagPointComponent::onOtherComponentRemoved(unsigned int index)
	{
		auto& gameObjectCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponentByIndex(index));
		if (nullptr != gameObjectCompPtr)
		{
			auto& physicsRagdollCompPtr = boost::dynamic_pointer_cast<PhysicsRagDollComponent>(gameObjectCompPtr);
			auto& animationCompPtr = boost::dynamic_pointer_cast<AnimationComponent>(gameObjectCompPtr);
			if (nullptr != physicsRagdollCompPtr || nullptr != animationCompPtr)
			{
				this->resetTagPoint();
			}
		}
	}

	bool TagPointComponent::onCloned(void)
	{
		// Search for the prior id of the cloned game object and set the new id and set the new id, if not found set better 0, else the game objects may be corrupt!
		GameObjectPtr sourceGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getClonedGameObjectFromPriorId(this->sourceId->getULong());
		if (nullptr != sourceGameObjectPtr)
			// Only the id is important!
			this->setSourceId(sourceGameObjectPtr->getId());
		else
			this->setSourceId(0);
		return true;
	}

	void TagPointComponent::update(Ogre::Real dt, bool notSimulating)
	{
		if (false == notSimulating)
		{
			if (true == this->bShowDebugData && nullptr != this->debugGeometryArrowNode)
			{
				Ogre::Vector3 worldPosition = this->tagPoint->getParent()->_getDerivedPosition();
				Ogre::Quaternion worldOrientation = this->tagPoint->getParent()->_getDerivedOrientation();

				//multiply with the parent derived transformation
				Ogre::Node* parentNode = this->gameObjectPtr->getMovableObject()->getParentNode();
				Ogre::SceneNode* sceneNode = this->gameObjectPtr->getMovableObject()->getParentSceneNode();
				while (parentNode != nullptr)
				{
					// Process the current i_Node
					if (parentNode != sceneNode)
					{
						// This is a tag point (a connection point between 2 entities). which means it has a parent i_Node to be processed
						worldPosition = ((Ogre::v1::TagPoint*)parentNode)->_getFullLocalTransform() * worldPosition;
						parentNode = ((Ogre::v1::TagPoint*)parentNode)->getParentEntity()->getParentNode();
						worldOrientation = ((Ogre::v1::TagPoint*)parentNode)->_getDerivedOrientation() * worldOrientation;
					}
					else
					{
						// This is the scene i_Node meaning this is the last i_Node to process
						worldPosition = parentNode->_getFullTransform() * worldPosition;
						worldOrientation = parentNode->_getDerivedOrientation() * worldOrientation;
						break;
					}
				}

				this->debugGeometryArrowNode->setPosition(worldPosition);
				this->debugGeometryArrowNode->setOrientation(worldOrientation);
				this->debugGeometrySphereNode->setPosition(worldPosition);
			}
		}
	}

	void TagPointComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (TagPointComponent::AttrTagPointName() == attribute->getName())
		{
			this->setTagPointName(attribute->getListSelectedValue());
		}
		else if (TagPointComponent::AttrSourceId() == attribute->getName())
		{
			this->setSourceId(attribute->getULong());
		}
		else if (TagPointComponent::AttrOffsetPosition() == attribute->getName())
		{
			this->setOffsetPosition(attribute->getVector3());
		}
		else if (TagPointComponent::AttrOffsetOrientation() == attribute->getName())
		{
			this->setOffsetOrientation(attribute->getVector3());
		}
	}

	void TagPointComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "TagPointName"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->tagPoints->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "SourceId"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->sourceId->getULong())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OffsetPosition"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->offsetPosition->getVector3())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "OffsetOrientation"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(this->offsetOrientation->getVector3())));
		propertiesXML->append_node(propertyXML);
	}

	void TagPointComponent::showDebugData(void)
	{
		GameObjectComponent::showDebugData();
		if (true == this->bShowDebugData)
		{
			this->destroyDebugData();
			this->generateDebugData();
		}
		else
		{
			this->destroyDebugData();
		}
	}

	Ogre::String TagPointComponent::getClassName(void) const
	{
		return "TagPointComponent";
	}

	Ogre::String TagPointComponent::getParentClassName(void) const
	{
		return "GameObjectComponent";
	}

	void TagPointComponent::setActivated(bool activated)
	{

	}

	void TagPointComponent::setTagPointName(const Ogre::String& tagPointName)
	{
		this->tagPoints->setListSelectedValue(tagPointName);

		if (nullptr == this->gameObjectPtr)
		{
			return;
		}

		Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
		if (nullptr != entity)
		{
			// Does not work anymore, as it is done different in Ogre2.x (see tagpoint example, for that new skeleton and item is necessary, updgrade of meshes)
			// this->tagPoint = entity->attachObjectToBone(this->tagPoints->getListSelectedValue(), sourceEntity, 
			// 	MathHelper::getInstance()->degreesToQuat(this->offsetOrientation->getVector3()),this->offsetPosition->getVector3());
			Ogre::v1::OldSkeletonInstance* oldSkeletonInstance = entity->getSkeleton();
			if (nullptr != oldSkeletonInstance)
			{
				if (nullptr != this->tagPoint)
				{
					oldSkeletonInstance->freeTagPoint(this->tagPoint);
					this->tagPoint = nullptr;
				}

				// Workaround:
				this->tagPoint = oldSkeletonInstance->createTagPointOnBone(oldSkeletonInstance->getBone(this->tagPoints->getListSelectedValue()));
				this->tagPoint->setScale(this->gameObjectPtr->getSceneNode()->getScale());
			}
		}

		if (true == this->bShowDebugData)
		{
			this->destroyDebugData();
			this->generateDebugData();
		}
	}

	Ogre::String TagPointComponent::getTagPointName(void) const
	{
		return 	this->tagPoints->getListSelectedValue();
	}

	void TagPointComponent::setSourceId(unsigned long sourceId)
	{
		this->sourceId->setValue(sourceId);
	}

	unsigned long TagPointComponent::getSourceId(void) const
	{
		return this->sourceId->getULong();
	}

	void TagPointComponent::setOffsetPosition(const Ogre::Vector3& offsetPosition)
	{
		this->offsetPosition->setValue(offsetPosition);
	}

	Ogre::Vector3 TagPointComponent::getOffsetPosition(void) const
	{
		return this->offsetPosition->getVector3();
	}

	void TagPointComponent::setOffsetOrientation(const Ogre::Vector3& offsetOrientation)
	{
		this->offsetOrientation->setValue(offsetOrientation);
	}

	Ogre::Vector3 TagPointComponent::getOffsetOrientation(void) const
	{
		return this->offsetOrientation->getVector3();
	}

	Ogre::v1::TagPoint* TagPointComponent::getTagPoint(void) const
	{
		return this->tagPoint;
	}

	Ogre::SceneNode* TagPointComponent::getTagPointNode(void) const
	{
		return this->tagPointNode;
	}

	void TagPointComponent::generateDebugData(void)
	{
		if (nullptr == this->debugGeometryArrowNode && nullptr != this->tagPoint)
		{
			this->debugGeometryArrowNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode(Ogre::SCENE_DYNAMIC);
			this->debugGeometryArrowNode->setName("tagPointComponentArrowNode");
			this->debugGeometrySphereNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode(Ogre::SCENE_DYNAMIC);
			this->debugGeometrySphereNode->setName("tagPointComponentSphereNode");

			Ogre::Vector3 worldPosition = this->tagPoint->getParent()->_getDerivedPosition();
			Ogre::Quaternion worldOrientation = this->tagPoint->getParent()->_getDerivedOrientation();

			//multiply with the parent derived transformation
			Ogre::Node* parentNode = this->gameObjectPtr->getMovableObject()->getParentNode();
			Ogre::SceneNode* sceneNode = this->gameObjectPtr->getMovableObject()->getParentSceneNode();
			while (parentNode != nullptr)
			{
				// Process the current i_Node
				if (parentNode != sceneNode)
				{
					// This is a tag point (a connection point between 2 entities). which means it has a parent i_Node to be processed
					worldPosition = ((Ogre::v1::TagPoint*)parentNode)->_getFullLocalTransform() * worldPosition;
					parentNode = ((Ogre::v1::TagPoint*)parentNode)->getParentEntity()->getParentNode();
					worldOrientation = ((Ogre::v1::TagPoint*)parentNode)->_getDerivedOrientation() * worldOrientation;
				}
				else
				{
					// This is the scene i_Node meaning this is the last i_Node to process
					worldPosition = parentNode->_getFullTransform() * worldPosition;
					worldOrientation = parentNode->_getDerivedOrientation() * worldOrientation;
					break;
				}
			}

			this->debugGeometryArrowNode->setPosition(worldPosition);
			this->debugGeometryArrowNode->setOrientation(worldOrientation);
			this->debugGeometryArrowNode->setScale(0.05f, 0.05f, 0.025f);
			this->debugGeometryArrowEntity = this->gameObjectPtr->getSceneManager()->createEntity("Arrow.mesh");
			this->debugGeometryArrowEntity->setName("tagPointComponentArrowEntity");
			this->debugGeometryArrowEntity->setDatablock("BaseYellowLine");
			this->debugGeometryArrowEntity->setQueryFlags(0 << 0);
			this->debugGeometryArrowEntity->setCastShadows(false);
			this->debugGeometryArrowNode->attachObject(this->debugGeometryArrowEntity);

			this->debugGeometrySphereNode->setPosition(worldPosition);
			this->debugGeometrySphereNode->setScale(0.05f, 0.05f, 0.05f);
			this->debugGeometrySphereEntity = this->gameObjectPtr->getSceneManager()->createEntity("gizmosphere.mesh");
			this->debugGeometrySphereEntity->setName("tagPointComponentSphereEntity");
			this->debugGeometrySphereEntity->setDatablock("BaseYellowLine");
			this->debugGeometrySphereEntity->setQueryFlags(0 << 0);
			this->debugGeometrySphereEntity->setCastShadows(false);
			this->debugGeometrySphereNode->attachObject(this->debugGeometrySphereEntity);
		}
	}

	void TagPointComponent::destroyDebugData(void)
	{
		if (nullptr != this->debugGeometryArrowNode)
		{
			this->debugGeometryArrowNode->detachAllObjects();
			this->gameObjectPtr->getSceneManager()->destroySceneNode(this->debugGeometryArrowNode);
			this->debugGeometryArrowNode = nullptr;
			this->gameObjectPtr->getSceneManager()->destroyMovableObject(this->debugGeometryArrowEntity);
			this->debugGeometryArrowEntity = nullptr;

			this->debugGeometrySphereNode->detachAllObjects();
			this->gameObjectPtr->getSceneManager()->destroySceneNode(this->debugGeometrySphereNode);
			this->debugGeometrySphereNode = nullptr;
			this->gameObjectPtr->getSceneManager()->destroyMovableObject(this->debugGeometrySphereEntity);
			this->debugGeometrySphereEntity = nullptr;
		}
	}

	void TagPointComponent::resetTagPoint(void)
	{
		Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
		if (nullptr != entity && nullptr != this->tagPoint)
		{
			Ogre::v1::OldSkeletonInstance* oldSkeletonInstance = entity->getSkeleton();
			if (nullptr != oldSkeletonInstance)
			{
				if (nullptr != this->tagPoint)
				{
					oldSkeletonInstance->freeTagPoint(this->tagPoint);
					Ogre::v1::OldNode::Listener* nodeListener = this->tagPoint->getListener();
					this->tagPoint->setListener(nullptr);
					delete nodeListener;
					this->tagPoint = nullptr;
				}
			}

			GameObjectPtr sourceGameObjectPtr = AppStateManager::getSingletonPtr()->getGameObjectController()->getGameObjectFromId(this->sourceId->getULong());
			if (nullptr != sourceGameObjectPtr)
			{
				// Reset pointer
				sourceGameObjectPtr->setConnectedGameObject(boost::weak_ptr<GameObject>());

				if (nullptr != this->tagPointNode)
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
			}

			if (nullptr != this->tagPointNode)
			{
				this->tagPointNode->removeAndDestroyAllChildren();
				this->gameObjectPtr->getSceneManager()->destroySceneNode(this->tagPointNode);
				this->tagPointNode = nullptr;
			}

			this->alreadyConnected = false;
		}
	}

}; // namespace end
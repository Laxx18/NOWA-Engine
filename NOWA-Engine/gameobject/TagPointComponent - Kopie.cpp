#include "NOWAPrecompiled.h"
#include "TagPointComponent.h"
#include "GameObjectController.h"
#include "PhysicsActiveComponent.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "AnimationComponent.h"

namespace NOWA
{
	using namespace rapidxml;

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
			oldPosition(Ogre::Vector3::ZERO),
			gameObject(gameObject)
		{
			
		}

		Ogre::Vector3 findVelocity(const Ogre::Vector3& aPos, const Ogre::Vector3& bPos, Ogre::Real speed)
		{
			Ogre::Vector3 disp = bPos - aPos;
			float distance = Ogre::Math::Sqrt(disp.x * disp.x + disp.y * disp.y + disp.z * disp.z); // std::hypot(disp.x, disp.y) if C++ 11
			return disp * (speed / distance);
		}

		virtual void OldNodeUpdated(const Ogre::v1::OldNode* updatedNode) override
		{
			Ogre::Vector3 localPosition;
			Ogre::Quaternion localOrientation;

			this->sourcePhysicsActiveComponent->globalToLocal(updatedNode->_getDerivedOrientation(), updatedNode->_getDerivedPosition(), localOrientation, localPosition);

			// Ogre::Vector3 globalPosition = this->gameObject->getSceneNode()->convertLocalToWorldPosition(updatedNode->_getDerivedPosition());
			// Ogre::Quaternion globalOrientation = this->gameObject->getSceneNode()->convertLocalToWorldOrientation(updatedNode->_getDerivedOrientation());

			//Ogre::Vector3 desiredVelocity = localPosition - this->sourcePhysicsActiveComponent->getPosition();
			//// desiredVelocity.y = 0.0f;
			//desiredVelocity.normalise();
			//desiredVelocity *= this->sourcePhysicsActiveComponent->getSpeed();
			//this->sourcePhysicsActiveComponent->setVelocity(this->sourcePhysicsActiveComponent->getVelocity() * Ogre::Vector3(0.0f, 1.0f, 0.0f) + desiredVelocity);

			// Ogre::Vector3 desiredVelocity = this->oldPosition - this->sourcePhysicsActiveComponent->getPosition();
			// this->sourcePhysicsActiveComponent->setVelocity(/*this->sourcePhysicsActiveComponent->getVelocity() * Ogre::Vector3(0.0f, 1.0f, 0.0f) +*/ desiredVelocity);

			/*Ogre::Vector3 desiredVelocity = this->findVelocity(this->sourcePhysicsActiveComponent->getPosition(), this->oldPosition, this->sourcePhysicsActiveComponent->getSpeed() * 20.0f);
			this->sourcePhysicsActiveComponent->setVelocity(desiredVelocity);*/

			float duration = 0.125 * 10.0f; // duration until we hit the target

			Ogre::Vector3 targetLinvel = (oldPosition - this->sourcePhysicsActiveComponent->getPosition()) / duration;

			Ogre::Vector3 curLinvel = this->sourcePhysicsActiveComponent->getVelocity();

			// may be still too aggressive - eventually try to smooth it:  targetLinvel = targetLinvel * 0.1 + curLinvel  * 0.9;

			targetLinvel = targetLinvel * 0.1f + curLinvel * 0.9f;

			Ogre::Vector3 force = (targetLinvel - curLinvel) * this->sourcePhysicsActiveComponent->getMass(); // this is important: if targetV
			this->sourcePhysicsActiveComponent->applyForce(force);

			Ogre::Quaternion newOrientation = this->gameObject->getOrientation() * localOrientation * this->offsetOrientation;
			// this->realNode->setOrientation(newOrientation);
			this->realNode->setScale(updatedNode->_getDerivedScale());
			// this->realNode->setPosition(((this->gameObject->getPosition() + localPosition + (newOrientation * this->offsetPosition)) * this->realNode->getScale()));
			this->oldPosition = ((this->gameObject->getPosition() + localPosition + (newOrientation * this->offsetPosition)) * this->realNode->getScale());
		}

		virtual void OldNodeDestroyed(const Ogre::v1::OldNode* updatedNode) override
		{
			delete this;
		}
	private:
		Ogre::Node* realNode; //Node where we redirect our updates
		Ogre::Vector3 offsetPosition;
		Ogre::Quaternion offsetOrientation;
		Ogre::Vector3 oldPosition;
		bool firstTimeSetPosition;
		PhysicsActiveComponent* sourcePhysicsActiveComponent;
		GameObject* gameObject;
	};
	
	TagPointComponent::TagPointComponent()
		: GameObjectComponent(),
		tagPoint(nullptr),
		tagPointNode(nullptr),
		alreadyConnected(false),
		debugGeometryNode(nullptr),
		debugGeometryEntity(nullptr),
		oldPosition(Ogre::Vector3::ZERO),
		oldOrientation(Ogre::Quaternion::IDENTITY),
		tagPoints(new Variant(TagPointComponent::AttrTagPointName(), std::vector<Ogre::String>(), this->attributes)),
		sourceId(new Variant(TagPointComponent::AttrSourceId(), static_cast<unsigned long>(0), this->attributes, true)),
		offsetPosition(new Variant(TagPointComponent::AttrOffsetPosition(), Ogre::Vector3::ZERO, this->attributes)),
		offsetOrientation(new Variant(TagPointComponent::AttrOffsetOrientation(), Ogre::Vector3::ZERO, this->attributes))
	{
		this->tagPoints->setDescription("If this game object uses several TagPoint-Components data may not be tagged to the same bone name");
	}

	TagPointComponent::~TagPointComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[TagPointComponent] Destructor tag point component for game object: " + this->gameObjectPtr->getName());
		this->tagPoint = nullptr;
		
		GameObjectPtr sourceGameObjectPtr = GameObjectController::getInstance()->getGameObjectFromId(this->sourceId->getULong());
		if (nullptr != sourceGameObjectPtr)
		{
			// Reset pointer
			sourceGameObjectPtr->setConnectedGameObject(boost::weak_ptr<GameObject>());
		}
		if (nullptr != this->debugGeometryNode)
		{
			this->debugGeometryNode->detachAllObjects();
			this->gameObjectPtr->getSceneManager()->destroySceneNode(this->debugGeometryNode);
			this->debugGeometryNode = nullptr;
			this->gameObjectPtr->getSceneManager()->destroyMovableObject(this->debugGeometryEntity);
			this->debugGeometryEntity = nullptr;
		}
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
		}
		return true;
	}

	bool TagPointComponent::connect(void)
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
				if (nullptr != this->tagPoint)
				{
					oldSkeletonInstance->freeTagPoint(this->tagPoint);
					this->tagPoint = nullptr;
				}

				GameObjectPtr sourceGameObjectPtr = GameObjectController::getInstance()->getGameObjectFromId(this->sourceId->getULong());
				if (nullptr == sourceGameObjectPtr)
					return false;

				// Set back reference (e.g. player/enemy that holds the weapon)
				sourceGameObjectPtr->setConnectedGameObject(this->gameObjectPtr);

				this->sourcePhysicsActiveCompPtr = NOWA::makeStrongPtr(sourceGameObjectPtr->getComponent<PhysicsActiveComponent>());

				// Does not work anymore, as it is done different in Ogre2.x (see tagpoint example, for that new skeleton and item is necessary, updgrade of meshes)
				// this->tagPoint = entity->attachObjectToBone(this->tagPoints->getListSelectedValue(), sourceEntity, 
				// 	MathHelper::getInstance()->degreesToQuat(this->offsetOrientation->getVector3()),this->offsetPosition->getVector3());

				// Workaround:
				this->tagPoint = oldSkeletonInstance->createTagPointOnBone(oldSkeletonInstance->getBone(this->tagPoints->getListSelectedValue()));
				this->tagPoint->setScale(this->gameObjectPtr->getSceneNode()->getScale());

				// this->tagPointNode = this->gameObjectPtr->getSceneNode()->createChildSceneNode(Ogre::SCENE_DYNAMIC);

				this->tagPointNode =  (static_cast<Ogre::SceneNode*>(sourcePhysicsActiveCompPtr->getBody()->getOgreNode()))->createChildSceneNode(Ogre::SCENE_DYNAMIC);
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

				this->tagPoint->setListener(new TagPointListener(this->tagPointNode, this->offsetPosition->getVector3(), 
					MathHelper::getInstance()->degreesToQuat(this->offsetOrientation->getVector3()), this->sourcePhysicsActiveCompPtr.get(), this->gameObjectPtr.get()));

				this->alreadyConnected = true;
			}
		}
		return true;
	}

	bool TagPointComponent::disconnect(void)
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
					Ogre::v1::OldNode::Listener* nodeListener = this->tagPoint->getListener();
					this->tagPoint->setListener(nullptr);
					delete nodeListener;

					GameObjectPtr sourceGameObjectPtr = GameObjectController::getInstance()->getGameObjectFromId(this->sourceId->getULong());
					if (nullptr != sourceGameObjectPtr)
					{
						// Reset pointer
						sourceGameObjectPtr->setConnectedGameObject(boost::weak_ptr<GameObject>());

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
					this->tagPoint = nullptr;

					this->alreadyConnected = false;
				}
			}
		}
		return true;
	}

	bool TagPointComponent::onCloned(void)
	{
		// Search for the prior id of the cloned game object and set the new id and set the new id, if not found set better 0, else the game objects may be corrupt!
		GameObjectPtr sourceGameObjectPtr = GameObjectController::getInstance()->getClonedGameObjectFromPriorId(this->sourceId->getULong());
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
			// if (nullptr != this->sourcePhysicsActiveCompPtr && nullptr != this->tagPoint)
			{
				/*this->oldPosition = this->sourcePhysicsActiveCompPtr->getPosition();
				this->oldOrientation = this->sourcePhysicsActiveCompPtr->getOrientation();

				Ogre::Vector3 velocity = this->tagPoint->getPosition() - this->sourcePhysicsActiveCompPtr->getPosition();
				Ogre::Vector3 omega = (this->tagPoint->getOrientation().Inverse() * this->sourcePhysicsActiveCompPtr->getOrientation()) * this->gameObjectPtr->getDefaultDirection();
				this->sourcePhysicsActiveCompPtr->setVelocity(velocity);
				this->sourcePhysicsActiveCompPtr->setOmegaVelocity(omega);*/
				// this->sourcePhysicsActiveCompPtr->setPosition(this->tagPoint->getPosition());
				// this->sourcePhysicsActiveCompPtr->setOrientation(this->tagPoint->getOrientation());
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
			if (nullptr == this->debugGeometryNode && nullptr != this->tagPointNode)
			{
				this->debugGeometryNode = this->gameObjectPtr->getSceneManager()->getRootSceneNode()->createChildSceneNode(Ogre::SCENE_DYNAMIC);
				this->debugGeometryNode->setName("tagPointComponentArrowNode");
				this->debugGeometryNode->setPosition(this->tagPointNode->getPosition() + this->gameObjectPtr->getSceneNode()->getPosition());
				this->debugGeometryNode->setOrientation(this->tagPointNode->getOrientation() * this->gameObjectPtr->getSceneNode()->getOrientation());
				this->debugGeometryNode->setScale(0.1f, 0.1f, 0.1f);
				this->debugGeometryEntity = this->gameObjectPtr->getSceneManager()->createEntity("Arrow.mesh");
				this->debugGeometryEntity->setName("tagPointComponentEntity");
				this->debugGeometryEntity->setDatablock("BaseYellowLine");
				this->debugGeometryEntity->setQueryFlags(0 << 0);
				this->debugGeometryEntity->setCastShadows(false);
				this->debugGeometryNode->attachObject(this->debugGeometryEntity);
			}
		}
		else
		{
			if (nullptr != this->debugGeometryNode)
			{
				this->debugGeometryNode->detachAllObjects();
				this->gameObjectPtr->getSceneManager()->destroySceneNode(this->debugGeometryNode);
				this->debugGeometryNode = nullptr;
				this->gameObjectPtr->getSceneManager()->destroyMovableObject(this->debugGeometryEntity);
				this->debugGeometryEntity = nullptr;
			}
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

}; // namespace end
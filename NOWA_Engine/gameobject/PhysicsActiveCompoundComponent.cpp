#include "NOWAPrecompiled.h"
#include "PhysicsActiveCompoundComponent.h"
#include "PhysicsCompoundConnectionComponent.h"
#include "utilities/XMLConverter.h"
#include "utilities/MathHelper.h"
#include "tinyxml.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	PhysicsActiveCompoundComponent::PhysicsActiveCompoundComponent()
		: PhysicsActiveComponent(),
		meshCompoundConfigFile(new Variant(PhysicsActiveCompoundComponent::AttrMeshCompoundConfigFile(), Ogre::String(), this->attributes))
	{
		this->meshCompoundConfigFile->addUserData(GameObject::AttrActionFileOpenDialog(), "Compounds");
		this->meshCompoundConfigFile->setDescription("Name of the XML config file, no path, e.g. torus.xml");
	}

	PhysicsActiveCompoundComponent::~PhysicsActiveCompoundComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveCompoundComponent] Destructor physics active compound component for game object: " 
			+ this->gameObjectPtr->getName());
		this->collisionDataList.clear();
	}

	bool PhysicsActiveCompoundComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		PhysicsActiveComponent::parseCommonProperties(propertyElement);

		// either load parts from mesh compound config file, the parts will have its correct position and rotation offset
		if (propertyElement)
		{
			if (XMLConverter::getAttrib(propertyElement, "name") == "MeshCompoundConfigFile")
			{
				this->meshCompoundConfigFile->setValue(XMLConverter::getAttrib(propertyElement, "data"));
				propertyElement = propertyElement->next_sibling("property");
			}
		}
		// or load from custom created collisions
		if (true == meshCompoundConfigFile->getString().empty())
		{
			bool hasRepeatedProperties = true;
			this->collisionDataList.clear();

			while (hasRepeatedProperties)
			{
				PhysicsActiveCompoundComponent::CollisionData collisionData;

				if (propertyElement)
				{
					Ogre::String element = XMLConverter::getAttrib(propertyElement, "name");
					if (Ogre::StringUtil::match(element, "CollisionTyp*", true))
					{
						collisionData.collisionType = XMLConverter::getAttrib(propertyElement, "data", "Box");
						hasRepeatedProperties = true;
						propertyElement = propertyElement->next_sibling("property");
					}
					else
					{
						hasRepeatedProperties = false;
					}
				}
				else
				{
					break;
				}

				if (propertyElement)
				{
					if (Ogre::StringUtil::match(XMLConverter::getAttrib(propertyElement, "name"), "CollisionPositio*", true))
					{
						collisionData.collisionPosition = XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3::ZERO);
						propertyElement = propertyElement->next_sibling("property");
					}
				}
				if (propertyElement)
				{
					if (Ogre::StringUtil::match(XMLConverter::getAttrib(propertyElement, "name"), "CollisionOrientatio*", true))
					{
						Ogre::Vector4 vec = XMLConverter::getAttribVector4(propertyElement, "data", Ogre::Vector4(0.0f, 0.0f, 0.0f, 1.0f));
						// parse vector4 as degree + axis quaternion
						if (Ogre::Vector4::ZERO == vec)
						{
							collisionData.collisionOrientation = Ogre::Quaternion::IDENTITY;
						}
						else
						{
							collisionData.collisionOrientation = Ogre::Quaternion(Ogre::Degree(vec.x), Ogre::Vector3(vec.y, vec.z, vec.w));
						}
						propertyElement = propertyElement->next_sibling("property");
					}
				}
				if (propertyElement)
				{
					if (Ogre::StringUtil::match(XMLConverter::getAttrib(propertyElement, "name"), "CollisionSiz*", true))
					{
						collisionData.collisionSize = XMLConverter::getAttribVector3(propertyElement, "data", Ogre::Vector3::ZERO);
						propertyElement = propertyElement->next_sibling("property");
					}
				}
				// add the collision data
				if (hasRepeatedProperties)
				{
					this->collisionDataList.emplace_back(collisionData);
				}
			}
		}

		return true;
	}

	GameObjectCompPtr PhysicsActiveCompoundComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		PhysicsActiveCompoundCompPtr clonedCompPtr(boost::make_shared<PhysicsActiveCompoundComponent>());

		
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
		// Bug in newton, setting afterwards collidable to true, will not work, hence do not clone this property
		// clonedCompPtr->setCollidable(this->collidable->getBool());
		
		clonedCompPtr->copyCollisionData(this->collisionDataList);
		clonedCompPtr->setMeshCompoundConfigFile(this->meshCompoundConfigFile->getString());
		
		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool PhysicsActiveCompoundComponent::postInit(void)
	{
		bool success = PhysicsComponent::postInit();

		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveCompoundComponent] Init physics active compound component for game object: " 
			+ this->gameObjectPtr->getName());

		this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
		this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
		this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();

		// Physics active component must be dynamic, else a mess occurs
		this->gameObjectPtr->setDynamic(true);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		// if an object has the compoundname set, then do not create collision for each body but collect it in a list, to get all bodies
		// that belong to the compound collision
		/*auto& physicsCompoundConnectionCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsCompoundConnectionComponent>());
		if (nullptr == physicsCompoundConnectionCompPtr)
		{
			this->setupCompoundBodies();
		}*/
		// sanity checks
		if (false == this->meshCompoundConfigFile->getString().empty())
		{
			Ogre::String meshName;
			if (GameObject::ENTITY == this->gameObjectPtr->getType())
			{
				meshName = this->gameObjectPtr->getMovableObjectUnsafe<Ogre::v1::Entity>()->getMesh()->getName();
			}
			else if (GameObject::ITEM == this->gameObjectPtr->getType())
			{
				meshName = this->gameObjectPtr->getMovableObjectUnsafe<Ogre::Item>()->getMesh()->getName();
			}
			else
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsArtifactComponent] Error cannot create static body, because the game object has no entity with mesh for game object: " + this->gameObjectPtr->getName());
				return false;
			}

			size_t dotPos = meshName.find(".");
			if (Ogre::String::npos == dotPos)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveCompoundComponent] Invalid mesh file name: " 
					+ meshName + " for game object: " + this->gameObjectPtr->getName());
				return false;
			}
			meshName = meshName.substr(0, dotPos);
			dotPos = this->meshCompoundConfigFile->getString().find(".");
			if (Ogre::String::npos == dotPos)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveCompoundComponent] Invalid mesh compound file name: " 
					+ this->meshCompoundConfigFile->getString() + " for game object: " + this->gameObjectPtr->getName());
				return false;
			}
			Ogre::String configName = this->meshCompoundConfigFile->getString().substr(0, dotPos);
			if (configName != meshName)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveCompoundComponent] Cannot create compound component, because the mesh compound config file name: "
					+ configName + " does not match with the entities mesh name: " + meshName + " for game object: "
					+ this->gameObjectPtr->getName());
			}
		}

		auto& physicsCompoundConnectionCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<PhysicsCompoundConnectionComponent>());
		if (nullptr == physicsCompoundConnectionCompPtr)
		{
			if (!this->createDynamicBody())
			{
				return false;
			}
		}
		return success;
	}

	bool PhysicsActiveCompoundComponent::connect(void)
	{
		bool success = PhysicsActiveComponent::connect();
		return success;
	}

	bool PhysicsActiveCompoundComponent::disconnect(void)
	{
		bool success = PhysicsActiveComponent::disconnect();
		return success;
	}

	void PhysicsActiveCompoundComponent::actualizeValue(Variant* attribute)
	{
		PhysicsActiveComponent::actualizeCommonValue(attribute);

		if (PhysicsActiveCompoundComponent::AttrMeshCompoundConfigFile() == attribute->getName())
		{
			this->setMeshCompoundConfigFile(attribute->getString());
		}
	}

	void PhysicsActiveCompoundComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		
		// Check if there is a mesh compound config file
		if (false == this->meshCompoundConfigFile->getString().empty())
		{
			propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
			propertyXML->append_attribute(doc.allocate_attribute("name", "MeshCompoundConfigFile"));
			propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->meshCompoundConfigFile->getString())));
			propertiesXML->append_node(propertyXML);
		}
		else
		{
			// Else write collision data directly
			for (unsigned int i = 0; i < static_cast<unsigned int>(this->collisionDataList.size()); i++)
			{
				propertyXML = doc.allocate_node(node_element, "property");
				propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
				propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "CollisionType" + Ogre::StringConverter::toString(i))));
				propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->collisionDataList[i].collisionType)));
				propertiesXML->append_node(propertyXML);

				propertyXML = doc.allocate_node(node_element, "property");
				propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
				propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "CollisionPosition" + Ogre::StringConverter::toString(i))));
				propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->collisionDataList[i].collisionPosition)));
				propertiesXML->append_node(propertyXML);

				propertyXML = doc.allocate_node(node_element, "property");
				propertyXML->append_attribute(doc.allocate_attribute("type", "10"));
				propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "CollisionOrientation" + Ogre::StringConverter::toString(i))));
				propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->collisionDataList[i].collisionOrientation)));
				propertiesXML->append_node(propertyXML);

				propertyXML = doc.allocate_node(node_element, "property");
				propertyXML->append_attribute(doc.allocate_attribute("type", "9"));
				propertyXML->append_attribute(doc.allocate_attribute("name", XMLConverter::ConvertString(doc, "CollisionSize" + Ogre::StringConverter::toString(i))));
				propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->collisionDataList[i].collisionSize)));
				propertiesXML->append_node(propertyXML);
			}
		}
	}

	Ogre::String PhysicsActiveCompoundComponent::getClassName(void) const
	{
		return "PhysicsActiveCompoundComponent";
	}

	Ogre::String PhysicsActiveCompoundComponent::getParentClassName(void) const
	{
		return "PhysicsActiveComponent";
	}

	Ogre::String PhysicsActiveCompoundComponent::getParentParentClassName(void) const
	{
		return "PhysicsComponent";
	}

	void PhysicsActiveCompoundComponent::copyCollisionData(std::vector<PhysicsActiveCompoundComponent::CollisionData> collisionDataList)
	{
		for (auto& it = collisionDataList.cbegin(); it != collisionDataList.cend(); ++it)
		{
			this->collisionDataList.emplace_back(*it);
		}
	}

	bool PhysicsActiveCompoundComponent::createDynamicBody(void)
	{
		this->destroyCollision();
		this->destroyBody();

		Ogre::Vector3 inertia = Ogre::Vector3(1.0f, 1.0f, 1.0f);
		Ogre::Vector3 calculatedMassOrigin = Ogre::Vector3::ZERO;

		this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
		this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
		this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();

		std::vector<OgreNewt::CollisionPtr> collisionList;

		unsigned int partsCount = 0;
		Ogre::Real partVolume = 0.0f;

		// either load parts from mesh compound config file, the parts will have its correct position and rotation offset
		// this enables creation of complex physics collision hulls like rings, gears etc.
		if (false == this->meshCompoundConfigFile->getString().empty())
		{
			Ogre::DataStreamPtr stream = Ogre::ResourceGroupManager::getSingleton().openResource(this->meshCompoundConfigFile->getString());
			if (stream.isNull())
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveCompoundComponent] Could not open mesh compound file: "
					+ this->meshCompoundConfigFile->getString() + " for game object: " + this->gameObjectPtr->getName());
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
				Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[PhysicsActiveCompoundComponent] Could not parse mesh compound file: " 
					+ this->meshCompoundConfigFile->getString() + " for game object: " + this->gameObjectPtr->getName());
				return false;
			}
			TiXmlElement* rootElement = document.FirstChildElement();
			TiXmlElement* childElement = rootElement->FirstChildElement();

			while (childElement)
			{
				// create scene node and entity
				Ogre::String sceneNodeName = "CompoundNode" + Ogre::StringConverter::toString(partsCount++) + "_" + this->gameObjectPtr->getName();
				Ogre::String meshName = childElement->Attribute("name");

				Ogre::SceneNode* sceneNode = this->gameObjectPtr->getSceneNode()->createChildSceneNode();
				sceneNode->setName(sceneNodeName);

				Ogre::v1::Entity* entity = nullptr;
				Ogre::Item* item = nullptr;

				if (GameObject::ENTITY == this->gameObjectPtr->getType())
				{
					entity = this->gameObjectPtr->getSceneManager()->createEntity(meshName);
					sceneNode->attachObject(entity);
				}
				else if (GameObject::ITEM == this->gameObjectPtr->getType())
				{
					item = this->gameObjectPtr->getSceneManager()->createItem(meshName);
					sceneNode->attachObject(item);
				}
				sceneNode->setScale(this->initialScale);
				
				// scaling also does reposition the collision parts! how nice! Use the created entity part for the convex hull!
				OgreNewt::CollisionPrimitives::ConvexHull* col = nullptr;

				if (GameObject::ENTITY == this->gameObjectPtr->getType())
				{
					col = new OgreNewt::CollisionPrimitives::ConvexHull(
						this->ogreNewt, entity, this->gameObjectPtr->getCategoryId(), Ogre::Quaternion::IDENTITY, Ogre::Vector3::ZERO, 0.001f, this->initialScale);
				}
				else if (GameObject::ITEM == this->gameObjectPtr->getType())
				{
					col = new OgreNewt::CollisionPrimitives::ConvexHull(
						this->ogreNewt, item, this->gameObjectPtr->getCategoryId(), Ogre::Quaternion::IDENTITY, Ogre::Vector3::ZERO, 0.001f, this->initialScale);
				}

				partVolume += col->calculateVolume();

				OgreNewt::CollisionPtr collisionPtr = OgreNewt::CollisionPtr(col);
				
				if (nullptr != collisionPtr)
				{
					collisionList.emplace_back(collisionPtr);
				}

				childElement = childElement->NextSiblingElement();
			}
		}
		else
		{
			for (auto& collisionData : this->collisionDataList)
			{
				// create the collision primitive and push it to the collision list
				OgreNewt::CollisionPtr collisionPtr = this->createCollisionPrimitive(collisionData.collisionType, collisionData.collisionPosition,
					collisionData.collisionOrientation, collisionData.collisionSize, inertia, massOrigin->getVector3(), this->gameObjectPtr->getCategoryId());
				// volume will be calculated inside of createCollisionPrimitive
				partVolume += this->volume;
				if (nullptr != collisionPtr)
				{
					collisionList.emplace_back(collisionPtr);
				}
			}
		}
		// get the whole volume
		this->volume = partVolume;
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsActiveCompoundComponent] calculated volume: "
			+ Ogre::StringConverter::toString(this->volume) + " for: " + this->gameObjectPtr->getName());

		// create the complex compound collision
		OgreNewt::CollisionPrimitives::CompoundCollision* compoundCollision = new OgreNewt::CollisionPrimitives::CompoundCollision(this->ogreNewt, collisionList, this->gameObjectPtr->getCategoryId());
		OgreNewt::CollisionPtr collisionPtr = OgreNewt::CollisionPtr(compoundCollision);

		OgreNewt::ConvexCollisionPtr convexCollisionPtr = std::static_pointer_cast<OgreNewt::ConvexCollision>(collisionPtr);
		convexCollisionPtr->calculateInertialMatrix(inertia, calculatedMassOrigin);

		this->physicsBody = new OgreNewt::Body(this->ogreNewt, this->gameObjectPtr->getSceneManager(), convexCollisionPtr);

		this->setPosition(this->initialPosition);
		this->setOrientation(this->initialOrientation);

		this->physicsBody->setGravity(this->gravity->getVector3());

		this->collisionType->setValue("Compound");
		
		if (Ogre::Vector3::ZERO != this->massOrigin->getVector3())
		{
			calculatedMassOrigin = this->massOrigin->getVector3();
		}

		// this->physicsBody->setConvexIntertialMatrix(inertia, calculatedMassOrigin);

		// apply mass and scale to inertia (the bigger the object, the more mass)
		inertia *= this->mass->getReal();
		this->physicsBody->setMassMatrix(this->mass->getReal(), inertia);

		// set mass origin
		this->physicsBody->setCenterOfMass(calculatedMassOrigin);

		if (this->linearDamping->getReal() != 0.0f)
		{
			this->physicsBody->setLinearDamping(this->linearDamping->getReal());
		}
		if (this->angularDamping->getVector3() != Ogre::Vector3::ZERO)
		{
			this->physicsBody->setAngularDamping(this->angularDamping->getVector3());
		}

		this->physicsBody->setCustomForceAndTorqueCallback<PhysicsActiveComponent>(&PhysicsActiveComponent::moveCallback, this);

		this->setConstraintAxis(this->constraintAxis->getVector3());
		// pin the object stand in pose and not fall down
		this->setConstraintDirection(this->constraintDirection->getVector3());

		this->setActivated(this->activated->getBool());
		this->setContinuousCollision(this->continuousCollision->getBool());
		this->setGyroscopicTorqueEnabled(this->gyroscopicTorque->getBool());

		// set user data for ogrenewt
		this->physicsBody->setUserData(OgreNewt::Any(dynamic_cast<PhysicsComponent*>(this)));

		this->physicsBody->attachNode(this->gameObjectPtr->getSceneNode());

		this->setCollidable(this->collidable->getBool());

		this->physicsBody->setType(this->gameObjectPtr->getCategoryId());

		const auto materialId = AppStateManager::getSingletonPtr()->getGameObjectController()->getMaterialID(this->gameObjectPtr.get(), this->ogreNewt);
		AppStateManager::getSingletonPtr()->getOgreNewtModule()->setMaterialIdForDebugger(materialId);
		this->physicsBody->setMaterialGroupID(materialId);

		// Destroy all temp scene nodes and entities because the origin one will be used again

		std::vector<Ogre::SceneNode*> toBeDeletedSceneNodes;
		auto nodeIt = this->gameObjectPtr->getSceneNode()->getChildIterator();
		while (nodeIt.hasMoreElements())
		{
			//go through all scenenodes in the scene
			Ogre::SceneNode* subNode = (Ogre::SceneNode*) nodeIt.getNext();

			Ogre::SceneNode::ObjectIterator objectIt = subNode->getAttachedObjectIterator();
			while (objectIt.hasMoreElements())
			{
				//go through all scenenodes in the scene
				Ogre::MovableObject* movableObject = objectIt.getNext();
				this->gameObjectPtr->getSceneManager()->destroyMovableObject(movableObject);
			}

			subNode->removeAllChildren();
			toBeDeletedSceneNodes.emplace_back(subNode);
		}

		for (size_t i = 0; i < toBeDeletedSceneNodes.size(); i++)
		{
			this->gameObjectPtr->getSceneNode()->removeChild(toBeDeletedSceneNodes[i]);
			this->gameObjectPtr->getSceneManager()->destroySceneNode(toBeDeletedSceneNodes[i]);
		}

		return true;
	}

	void PhysicsActiveCompoundComponent::setMeshCompoundConfigFile(const Ogre::String& meshCompoundConfigFile)
	{
		this->meshCompoundConfigFile->setValue(meshCompoundConfigFile);
		this->createDynamicBody();
	}

}; // namespace end
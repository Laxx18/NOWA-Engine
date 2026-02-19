#include "NOWAPrecompiled.h"
#include "PhysicsArtifactComponent.h"
#include "utilities/XMLConverter.h"
#include "main/AppStateManager.h"
#include "main/Core.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	PhysicsArtifactComponent::PhysicsArtifactComponent()
		: PhysicsComponent(),
		collisionMode(COLLISION_TREE),
		serialize(new Variant(PhysicsArtifactComponent::AttrSerialize(), false, this->attributes))
	{
		this->collisionType->setVisible(false);
		this->mass->setValue(10000.0f);
		this->mass->setDescription("Mass for physics artifact component can be used, when this component is the gravity source, else mass has no meaning and can be anything.");

		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &PhysicsArtifactComponent::handleGameObjectChanged), NOWA::EventDataGeometryChanged::getStaticEventType());
	}

	PhysicsArtifactComponent::~PhysicsArtifactComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsArtifactComponent] Destructor physics artifact component for game object: " + this->gameObjectPtr->getName());
        NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &PhysicsArtifactComponent::handleGameObjectChanged), NOWA::EventDataGeometryChanged::getStaticEventType());
	}

	void PhysicsArtifactComponent::handleGameObjectChanged(NOWA::EventDataPtr eventData)
    {
        boost::shared_ptr<NOWA::EventDataGeometryChanged> castEventData = boost::static_pointer_cast<NOWA::EventDataGeometryChanged>(eventData);

        // Event not for this state
        if (this->gameObjectPtr->getId() != castEventData->getGameObjectId())
        {
            // Not for this game object
            return;
        }

        if (nullptr != this->physicsBody)
        {
            this->destroyBody();
            // this->destroyCollision();
        }
        // Must overwrite the collision file
        this->reCreateCollision(true);
    }

	bool PhysicsArtifactComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Mass")
		{
			this->mass->setValue(XMLConverter::getAttribReal(propertyElement, "data"));
			propertyElement = propertyElement->next_sibling("property");
		}
		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Serialize")
		{
			this->serialize->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}
		
		// Snapshot loaded, game object pointer should exist, set transform
		if (nullptr != this->gameObjectPtr)
		{
			bool oldIsDynamic = this->gameObjectPtr->isDynamic();
			this->gameObjectPtr->setDynamic(true);
			Ogre::Vector3 position = this->gameObjectPtr->getPosition();
			Ogre::Quaternion orientation = this->gameObjectPtr->getOrientation();
			// This must be done this way, because pos, orientation must be set at once, else orientation will be the old one
			if (nullptr != this->physicsBody)
			{
				this->setPosition(position);
				this->setOrientation(orientation);
			}
			this->gameObjectPtr->setDynamic(oldIsDynamic);
		}

		return true;
	}

	GameObjectCompPtr PhysicsArtifactComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		PhysicsArtifactCompPtr clonedCompPtr(boost::make_shared<PhysicsArtifactComponent>());

		// clonedCompPtr->setCollisionType(this->collisionType->getListSelectedValue());
		clonedCompPtr->setMass(this->mass->getReal());
		// Bug in newton, setting afterwards collidable to true, will not work, hence do not clone this property
		// clonedCompPtr->setCollidable(this->collidable->getBool());

		clonedGameObjectPtr->addComponent(clonedCompPtr);
		clonedCompPtr->setOwner(clonedGameObjectPtr);

		clonedCompPtr->setSerialize(this->serialize->getBool());

		GameObjectComponent::cloneBase(boost::static_pointer_cast<GameObjectComponent>(clonedCompPtr));
		return clonedCompPtr;
	}

	bool PhysicsArtifactComponent::postInit(void)
	{
		bool success = PhysicsComponent::postInit();
		
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsArtifactComponent] Init physics artifact component for game object: " + this->gameObjectPtr->getName());

		if (!this->createStaticBody())
		{
			return false;
		}

		this->physicsBody->setType(gameObjectPtr->getCategoryId());

		const auto materialId = AppStateManager::getSingletonPtr()->getGameObjectController()->getMaterialID(this->gameObjectPtr.get(), this->ogreNewt);
		this->physicsBody->setMaterialGroupID(materialId);

		return success;
	}

	void PhysicsArtifactComponent::actualizeValue(Variant* attribute)
	{
		PhysicsComponent::actualizeValue(attribute);

		//// The category and the category id are handled in game object, but the category id must be handled in physics component too
		//if ("Category" == attribute->getName())
		//{
		//	this->physicsBody->setType(this->gameObjectPtr->getCategoryId());
		//}
		if (GameObject::AttrClampY() == attribute->getName())
		{
			// Clamp physics y coordinate, if activated and something to clamp to
			this->setPosition(this->physicsBody->getPosition().x, this->gameObjectPtr->getSceneNode()->getPosition().y, this->physicsBody->getPosition().z);
		}
		else if (PhysicsComponent::AttrMass() == attribute->getName())
		{
			this->mass->setValue(attribute->getReal());
		}
		else if (PhysicsArtifactComponent::AttrSerialize() == attribute->getName())
		{
			this->setSerialize(attribute->getBool());
		}
	}

	void PhysicsArtifactComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("type", "6"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Mass"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->mass->getReal())));
		propertiesXML->append_node(propertyXML);

		propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "12"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "Serialize"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->serialize->getBool())));
		propertiesXML->append_node(propertyXML);

		/*propertyXML = doc.allocate_node(node_element, "property");
		propertyXML->append_attribute(doc.allocate_attribute("type", "7"));
		propertyXML->append_attribute(doc.allocate_attribute("name", "CollisionType"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->collisionType->getListSelectedValue())));
		propertiesXML->append_node(propertyXML);*/
	}

	void PhysicsArtifactComponent::showDebugData(void)
	{
		GameObjectComponent::showDebugData();
		if (nullptr != this->physicsBody)
		{
			ENQUEUE_RENDER_COMMAND_WAIT("PhysicsArtifactComponent::showDebugData",
			{
				this->physicsBody->showDebugCollision(!this->gameObjectPtr->isDynamic(), this->bShowDebugData);
			});
		}
	}

	Ogre::String PhysicsArtifactComponent::getClassName(void) const
	{
		return "PhysicsArtifactComponent";
	}

	Ogre::String PhysicsArtifactComponent::getParentClassName(void) const
	{
		return "PhysicsComponent";
	}

	bool PhysicsArtifactComponent::createStaticBody(void)
	{
		// Body has already been created, do not do it twice!
		if (nullptr != this->physicsBody)
			return true;

		this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
		this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
		this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();
		Ogre::String meshName;

		// Collision for static objects
		OgreNewt::CollisionPtr staticCollision;

		if (false == this->serialize->getBool())
		{
			Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
			if (nullptr != entity)
			{
				meshName = entity->getMesh()->getName();
				if (Ogre::StringUtil::match(meshName, "Plane*", true))
				{
					// if the mesh name is a plane, the tree collision does not work, so use box
					Ogre::Vector3 size = entity->getMesh()->getBounds().getSize() * this->initialScale;
					size.y = 0.001f;
					staticCollision = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::Box(this->ogreNewt, size, this->gameObjectPtr->getCategoryId(), Ogre::Quaternion::IDENTITY, Ogre::Vector3::ZERO));
					// Causes crash, when deleted all bodies!, something is wrong with that!
					// this->ogreNewt->AddSceneCollision(staticCollision, 0);
				}
				else
				{
					staticCollision = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::TreeCollision(this->ogreNewt, entity, true, this->gameObjectPtr->getCategoryId()));
					// this->ogreNewt->AddSceneCollision(staticCollision, 0);
				}
			}
			else
			{
				Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
				if (nullptr != item)
				{
					meshName = item->getMesh()->getName();
					if (Ogre::StringUtil::match(meshName, "Plane*", true))
					{
						// if the mesh name is a plane, the tree collision does not work, so use box
						Ogre::Vector3 size = item->getMesh()->getAabb().getSize() * this->initialScale;
						size.y = 0.001f;
						staticCollision = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::Box(this->ogreNewt, size, this->gameObjectPtr->getCategoryId(), Ogre::Quaternion::IDENTITY, Ogre::Vector3::ZERO));
						// Causes crash, when deleted all bodies!, something is wrong with that!
						// this->ogreNewt->AddSceneCollision(staticCollision, 0);
					}
					else
					{
						staticCollision = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::TreeCollision(this->ogreNewt, item, true, this->gameObjectPtr->getCategoryId()));
						// this->ogreNewt->AddSceneCollision(staticCollision, 0);
					}
				}
				else
				{
					// Foliage type?
				}
			}
		}
		else
		{
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("PhysicsComponent::serializeTreeCollision", _1(&staticCollision),
			{
				Ogre::String projectFilePath = Core::getSingletonPtr()->getCurrentProjectPath();
				staticCollision = OgreNewt::CollisionPtr(this->serializeTreeCollision(projectFilePath, this->gameObjectPtr->getCategoryId()));
			});
		}

		if (nullptr == staticCollision)
		{
            // Foliage type, it uses this component later and set compound stuff
            staticCollision = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::Null(this->ogreNewt));
		}

		this->physicsBody = new OgreNewt::Body(this->ogreNewt, this->gameObjectPtr->getSceneManager(), staticCollision);
		NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->registerRenderCallbackForBody(this->physicsBody);

		// Set mass to 0 = infinity = static
		this->physicsBody->setMassMatrix(0.0f, Ogre::Vector3::ZERO);

		this->physicsBody->setUserData(OgreNewt::Any(dynamic_cast<PhysicsComponent*>(this)));

		this->physicsBody->attachNode(this->gameObjectPtr->getSceneNode());

		this->setPosition(this->initialPosition);
		this->setOrientation(this->initialOrientation);

		// Artifact body should always sleep?
		this->physicsBody->setAutoSleep(1);
		// Not required, since artifact does not move

		// Object will not be moved frequently
		// this->gameObjectPtr->setDynamic(false); // Am I shit? Let the designer choose, because else an object with artifact collision can be no more moved!

		// Component must be dynamic, because it will be moved
		this->gameObjectPtr->setDynamic(false);
		this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

		return true;
	}

	void PhysicsArtifactComponent::changeCollisionFaceId(unsigned int id)
	{
		if (nullptr != this->collisionPtr)
		{
			auto treeCollision = std::dynamic_pointer_cast<OgreNewt::CollisionPrimitives::TreeCollision>(this->collisionPtr);
			if (nullptr != treeCollision)
			{
				treeCollision->setFaceId(id);
			}
		}
    }

    bool PhysicsArtifactComponent::createCompoundBody(const std::vector<OgreNewt::CollisionPtr>& childCollisions, const Ogre::String& collisionName)
    {
        if (childCollisions.empty())
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsArtifactComponent] Cannot create compound: no collision shapes");
            return false;
        }

        // Store for potential recreation
        this->compoundChildCollisions = childCollisions;
        this->compoundCollisionName = collisionName;
        this->collisionMode = COLLISION_COMPOUND;

        // Body already exists? Destroy it first
        if (nullptr != this->physicsBody)
        {
            this->destroyBody();
        }

        this->initialPosition = this->gameObjectPtr->getSceneNode()->getPosition();
        this->initialScale = this->gameObjectPtr->getSceneNode()->getScale();
        this->initialOrientation = this->gameObjectPtr->getSceneNode()->getOrientation();

        OgreNewt::CollisionPtr compoundCollision;

        if (false == this->serialize->getBool())
        {
            // Create compound directly (no serialization)
            compoundCollision = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::CompoundCollision(this->ogreNewt, childCollisions, this->gameObjectPtr->getCategoryId()));
        }
        else
        {
            // Serialize compound collision
            NOWA::GraphicsModule::RenderCommand renderCommand = [this, childCollisions, collisionName, & compoundCollision]()
            {
                Ogre::String scenePath = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName();
                compoundCollision = this->serializeCompoundCollision(scenePath, childCollisions, collisionName, this->gameObjectPtr->getCategoryId(), false);
            };
            NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PhysicsArtifactComponent::serializeCompoundCollision");
        }

        if (nullptr == compoundCollision)
        {
            Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsArtifactComponent] Failed to create compound collision");
            return false;
        }

        // Create body
        this->physicsBody = new OgreNewt::Body(this->ogreNewt, this->gameObjectPtr->getSceneManager(), compoundCollision);
        NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->registerRenderCallbackForBody(this->physicsBody);

        // Set mass to 0 = static
        this->physicsBody->setMassMatrix(0.0f, Ogre::Vector3::ZERO);
        this->physicsBody->setUserData(OgreNewt::Any(dynamic_cast<PhysicsComponent*>(this)));
        this->physicsBody->attachNode(this->gameObjectPtr->getSceneNode());

        this->setPosition(this->initialPosition);
        this->setOrientation(this->initialOrientation);

        this->physicsBody->setAutoSleep(1);
        this->gameObjectPtr->setDynamic(false);
        this->gameObjectPtr->getAttribute(GameObject::AttrDynamic())->setVisible(false);

        Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsArtifactComponent] Created compound body with " + Ogre::StringConverter::toString(childCollisions.size()) + " shapes");

        return true;
    }

	void PhysicsArtifactComponent::reCreateCollision(bool overwrite)
	{
        if (COLLISION_COMPOUND == this->collisionMode)
        {
            // Handle compound collision recreation
            if (this->compoundChildCollisions.empty())
            {
                Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsArtifactComponent] Cannot recreate compound: no children stored");
                return;
            }

            if (nullptr != this->physicsBody)
            {
                this->destroyCollision();
            }

            OgreNewt::CollisionPtr compoundCollision;

            if (false == this->serialize->getBool())
            {
                compoundCollision = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::CompoundCollision(this->ogreNewt, this->compoundChildCollisions, this->gameObjectPtr->getCategoryId()));
            }
            else
            {
                NOWA::GraphicsModule::RenderCommand renderCommand = [this, overwrite, &compoundCollision]()
                {
                    Ogre::String scenePath = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName();
                    compoundCollision = this->serializeCompoundCollision(scenePath, this->compoundChildCollisions, this->compoundCollisionName, this->gameObjectPtr->getCategoryId(), overwrite);
                };
                NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PhysicsArtifactComponent::recreateCompound");
            }

            if (compoundCollision)
            {
                this->physicsBody->setCollision(compoundCollision);
                this->physicsBody->setMassMatrix(0.0f, Ogre::Vector3::ZERO);
            }

			// Re-apply material group ID — reCreateCollision creates a fresh body
            // which resets m_shapeMaterial.m_userId to 0
            const auto materialId = AppStateManager::getSingletonPtr()->getGameObjectController()->getMaterialID(this->gameObjectPtr.get(), this->ogreNewt);
            this->physicsBody->setMaterialGroupID(materialId);

            return; // Exit early for compound mode
        }

        Ogre::String meshName;

        if (nullptr != this->physicsBody)
        {
            this->destroyCollision();
        }

		// Collision for static objects
		OgreNewt::CollisionPtr staticCollision;
		if (false == this->serialize->getBool())
		{
			Ogre::v1::Entity* entity = this->gameObjectPtr->getMovableObject<Ogre::v1::Entity>();
			if (nullptr != entity)
			{
				meshName = entity->getMesh()->getName();
				if (Ogre::StringUtil::match(meshName, "Plane*", true))
				{
					// if the mesh name is a plane, the tree collision does not work, so use box
					// Attention: Is this correct?
					Ogre::Vector3 size = entity->getMesh()->getBounds().getSize() * this->initialScale;
					size.y = 0.001f;
					staticCollision = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::Box(this->ogreNewt, size, this->gameObjectPtr->getCategoryId(),
						Ogre::Quaternion::IDENTITY, Ogre::Vector3::ZERO));
				}
				else
				{
					staticCollision = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::TreeCollision(this->ogreNewt, entity, true, this->gameObjectPtr->getCategoryId()));
				}
			}
			else
			{
				Ogre::Item* item = this->gameObjectPtr->getMovableObject<Ogre::Item>();
				if (nullptr != item)
				{
					meshName = item->getMesh()->getName();
					if (Ogre::StringUtil::match(meshName, "Plane*", true))
					{
						// if the mesh name is a plane, the tree collision does not work, so use box
						// Attention: Is this correct?
						Ogre::Vector3 size = item->getMesh()->getAabb().getSize() * this->initialScale;
						size.y = 0.001f;
						staticCollision = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::Box(this->ogreNewt, size, this->gameObjectPtr->getCategoryId(),
							Ogre::Quaternion::IDENTITY, Ogre::Vector3::ZERO));
					}
					else
					{
						staticCollision = OgreNewt::CollisionPtr(new OgreNewt::CollisionPrimitives::TreeCollision(this->ogreNewt, item, true, this->gameObjectPtr->getCategoryId()));
					}
				}
				else
				{
					Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsArtifactComponent] Error cannot create static body, because the game object has no entity/item with mesh for game object: " + this->gameObjectPtr->getName());
					throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[PhysicsArtifactComponent] Error cannot create static body, because the game object has no entity/item with mesh for game object: " + this->gameObjectPtr->getName() + ".\n", "NOWA");
				}
			}
		}
		else
		{
			// For more complexe objects its better to serialize the collision hull, so that the creation is a lot of faster next time
			// Note: Collision file is located in the project folder for all scenes
			ENQUEUE_RENDER_COMMAND_MULTI_WAIT("PhysicsComponent::serializeTreeCollision", _2(overwrite, &staticCollision),
			{
				Ogre::String projectFilePath = Core::getSingletonPtr()->getCurrentProjectPath();
				staticCollision = OgreNewt::CollisionPtr(this->serializeTreeCollision(projectFilePath, this->gameObjectPtr->getCategoryId(), overwrite));
			});
		}

		if (nullptr == staticCollision)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsArtifactComponent] Could create collision file for game object: "
				+ this->gameObjectPtr->getName() + " and mesh: " + meshName + ". Maybe the mesh is corrupt.");
			throw Ogre::Exception(Ogre::Exception::ERR_INVALID_STATE, "[PhysicsArtifactComponent] Could create collision file for game object: "
				+ this->gameObjectPtr->getName() + " and mesh: " + meshName + ". Maybe the mesh is corrupt.\n", "NOWA");
		}

		// Set the new collision hull
		this->physicsBody->setCollision(staticCollision);

		this->physicsBody->setMassMatrix(0.0f, Ogre::Vector3::ZERO);

		// Re-apply material group ID — reCreateCollision creates a fresh body
        // which resets m_shapeMaterial.m_userId to 0
        const auto materialId = AppStateManager::getSingletonPtr()->getGameObjectController()->getMaterialID(this->gameObjectPtr.get(), this->ogreNewt);
        this->physicsBody->setMaterialGroupID(materialId);
	}

	void PhysicsArtifactComponent::setSerialize(bool serialize)
	{
		this->serialize->setValue(serialize);
		if (true == serialize)
		{
			this->reCreateCollision(true);
		}
		else
		{
			Ogre::String meshName;
			if (GameObject::ENTITY == this->gameObjectPtr->getType())
			{
				meshName = this->gameObjectPtr->getMovableObjectUnsafe<Ogre::v1::Entity>()->getMesh()->getName();
			}
			else
			{
				meshName = this->gameObjectPtr->getMovableObjectUnsafe<Ogre::Item>()->getMesh()->getName();
			}

			Ogre::String sourceCollisionFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + meshName + ".ply";
			try
			{
				DeleteFile(sourceCollisionFilePathName.c_str());
			}
			catch (...)
			{

			}
		}
	}

	bool PhysicsArtifactComponent::getSerialize(void) const
	{
		return this->serialize->getBool();
	}

}; // namespace end
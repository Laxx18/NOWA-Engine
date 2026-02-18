#include "NOWAPrecompiled.h"
#include "PhysicsTerrainComponent.h"
#include "PhysicsComponent.h"
#include "TerraComponent.h"
#include "utilities/XMLConverter.h"
#include "main/AppStateManager.h"
#include "main/Core.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	PhysicsTerrainComponent::PhysicsTerrainComponent()
		: PhysicsComponent(),
		serialize(new Variant(PhysicsTerrainComponent::AttrSerialize(), true, this->attributes))
	{
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &PhysicsTerrainComponent::handleEventDataGameObjectMadeGlobal), EventDataGameObjectMadeGlobal::getStaticEventType());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->addListener(fastdelegate::MakeDelegate(this, &PhysicsTerrainComponent::handleGeometryChanged), NOWA::EventDataGeometryChanged::getStaticEventType());
	}

	PhysicsTerrainComponent::~PhysicsTerrainComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsTerrainComponent] Destructor physics terrain component for game object: " + this->gameObjectPtr->getName());

		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &PhysicsTerrainComponent::handleEventDataGameObjectMadeGlobal), EventDataGameObjectMadeGlobal::getStaticEventType());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->removeListener(fastdelegate::MakeDelegate(this, &PhysicsTerrainComponent::handleGeometryChanged), NOWA::EventDataGeometryChanged::getStaticEventType());
	}

	bool PhysicsTerrainComponent::init(rapidxml::xml_node<>*& propertyElement)
	{
		GameObjectComponent::init(propertyElement);

		if (propertyElement && XMLConverter::getAttrib(propertyElement, "name") == "Serialize")
		{
			this->serialize->setValue(XMLConverter::getAttribBool(propertyElement, "data", false));
			propertyElement = propertyElement->next_sibling("property");
		}

		return true;
	}

	GameObjectCompPtr PhysicsTerrainComponent::clone(GameObjectPtr clonedGameObjectPtr)
	{
		// terrain should not be cloned!
		return GameObjectCompPtr();
	}

	bool PhysicsTerrainComponent::postInit(void)
	{
		bool success = PhysicsComponent::postInit();
		
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsTerrainComponent] Init physics terrain component for game object: " + this->gameObjectPtr->getName());

		this->reCreateCollision();

		return success;
	}

	void PhysicsTerrainComponent::reCreateCollision(bool overwrite)
	{
		if (nullptr != this->physicsBody)
		{
			this->destroyCollision();
		}
		
		Ogre::Terra* terra = nullptr;

		auto& terraCompPtr = NOWA::makeStrongPtr(this->gameObjectPtr->getComponent<TerraComponent>());
		if (nullptr != terraCompPtr)
		{
			terra = terraCompPtr->getTerra();
			if (nullptr == terra)
				return;
		}
		else
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[TerraComponent] Could not create collision hull for terra, because there is no terra component for game object: "
				+ this->gameObjectPtr->getName() + ". Terra component must be placed before PhysicsTerrainComponent.");
			return;
		}

		// Collision for static objects
		OgreNewt::CollisionPtr staticCollision;

		NOWA::GraphicsModule::RenderCommand renderCommand = [this, terra, overwrite, &staticCollision]()
		{
			if (false == this->serialize->getBool())
			{
				staticCollision = this->createHeightFieldCollision(terra);
			}
			else
			{
				// Note: Terra collision file is located in the corresponding scene folder
				Ogre::String projectFilePath;
				if (false == this->gameObjectPtr->getGlobal())
				{
					projectFilePath = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName();
				}
				else
				{
					projectFilePath = Core::getSingletonPtr()->getCurrentProjectPath();
				}

				// For more complexe objects its better to serialize the collision hull, so that the creation is a lot of faster next time
				staticCollision = OgreNewt::CollisionPtr(this->serializeHeightFieldCollision(projectFilePath, this->gameObjectPtr->getCategoryId(), terra, overwrite));
			}

			if (nullptr == staticCollision)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsTerrainComponent] Could create collision file for game object: "
					+ this->gameObjectPtr->getName() + " and terrain mesh. Maybe the mesh is corrupt.");
				staticCollision = this->createHeightFieldCollision(terra);
			}

			if (nullptr == this->physicsBody)
			{
				this->physicsBody = new OgreNewt::Body(this->ogreNewt, this->gameObjectPtr->getSceneManager(), staticCollision);

				NOWA::AppStateManager::getSingletonPtr()->getOgreNewtModule()->registerRenderCallbackForBody(this->physicsBody);
				// Set mass to 0 = infinity = static
				this->physicsBody->setMassMatrix(0.0f, Ogre::Vector3::ZERO);

				this->physicsBody->setPositionOrientation(this->initialPosition, this->initialOrientation);

				// this->setPosition(this->gameObjectPtr->getSceneNode()->getPosition());
				// this->setOrientation(this->gameObjectPtr->getSceneNode()->getOrientation());

				this->physicsBody->setUserData(OgreNewt::Any(dynamic_cast<PhysicsComponent*>(this)));

				// here can be the gameobjectPtr be involved
				this->physicsBody->attachNode(this->gameObjectPtr->getSceneNode());

				this->physicsBody->setType(gameObjectPtr->getCategoryId());

				const auto materialId = AppStateManager::getSingletonPtr()->getGameObjectController()->getMaterialID(this->gameObjectPtr.get(), this->ogreNewt);
				this->physicsBody->setMaterialGroupID(materialId);

				this->gameObjectPtr->setDynamic(false);
			}
			else
			{
				this->physicsBody->setCollision(staticCollision);
			}
		};
		NOWA::GraphicsModule::getInstance()->enqueueAndWait(std::move(renderCommand), "PhysicsComponent::createHeightFieldCollision");
	}

	void PhysicsTerrainComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);

		if (PhysicsTerrainComponent::AttrSerialize() == attribute->getName())
		{
			this->setSerialize(attribute->getBool());
		}
	}

	void PhysicsTerrainComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc)
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
		propertyXML->append_attribute(doc.allocate_attribute("name", "Serialize"));
		propertyXML->append_attribute(doc.allocate_attribute("data", XMLConverter::ConvertString(doc, this->serialize->getBool())));
		propertiesXML->append_node(propertyXML);
	}

	Ogre::String PhysicsTerrainComponent::getClassName(void) const
	{
		return "PhysicsTerrainComponent";
	}

	Ogre::String PhysicsTerrainComponent::getParentClassName(void) const
	{
		return "PhysicsComponent";
	}

	void PhysicsTerrainComponent::showDebugData(void)
	{
		GameObjectComponent::showDebugData();
		if (nullptr != this->physicsBody)
		{
			// Note: When selecting a game object, it will become for a short time dynamic for movement via gizmo...
			this->physicsBody->showDebugCollision(!this->gameObjectPtr->isDynamic(), this->bShowDebugData);
		}
	}

	void PhysicsTerrainComponent::setSerialize(bool serialize)
	{
		this->serialize->setValue(serialize);
		this->reCreateCollision(true);

		if (false == serialize)
		{
			bool isGlobal = this->gameObjectPtr->getGlobal();
			if (false == isGlobal)
			{
				Ogre::String sourceCollisionFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "/" + this->gameObjectPtr->getName() + ".ply";
				try
				{
					DeleteFile(sourceCollisionFilePathName.c_str());
				}
				catch (...)
				{

				}
			}
			else
			{
				Ogre::String sourceCollisionFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + this->gameObjectPtr->getName() + ".ply";
				try
				{
					DeleteFile(sourceCollisionFilePathName.c_str());
				}
				catch (...)
				{

				}
			}
		}
	}

	bool PhysicsTerrainComponent::getSerialize(void) const
	{
		return this->serialize->getBool();
	}

	void PhysicsTerrainComponent::changeCollisionFaceId(unsigned int id)
	{
		if (nullptr != this->collisionPtr)
		{
			auto heightFieldCollision = std::dynamic_pointer_cast<OgreNewt::CollisionPrimitives::HeightField>(this->collisionPtr);
			if (nullptr != heightFieldCollision)
			{
				heightFieldCollision->setFaceId(id);
			}
		}
	}

	void PhysicsTerrainComponent::handleEventDataGameObjectMadeGlobal(NOWA::EventDataPtr eventData)
	{
		boost::shared_ptr<EventDataGameObjectMadeGlobal> castEventData = boost::static_pointer_cast<EventDataGameObjectMadeGlobal>(eventData);

		if (this->gameObjectPtr->getId() != castEventData->getGameObjectId())
		{
			return;
		}

		bool isGlobal = castEventData->getIsGlobal();
		if (false == isGlobal)
		{
			if (nullptr != this->physicsBody)
			{
				this->destroyBody();
				// this->destroyCollision();
			}

			Ogre::String sourceCollisionFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + this->gameObjectPtr->getName() + ".ply";
			Ogre::String destinationCollisionFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "/" + this->gameObjectPtr->getName() + ".ply";
			CopyFile(sourceCollisionFilePathName.c_str(), destinationCollisionFilePathName.c_str(), false);

			try
			{
				DeleteFile(sourceCollisionFilePathName.c_str());
			}
			catch (...)
			{

			}

			this->reCreateCollision(true);
		}
		else
		{
			if (nullptr != this->physicsBody)
			{
				this->destroyBody();
				// this->destroyCollision();
			}

			Ogre::String sourceCollisionFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "/" + this->gameObjectPtr->getName() + ".ply";
			Ogre::String destinationCollisionFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + this->gameObjectPtr->getName() + ".ply";
			CopyFile(sourceCollisionFilePathName.c_str(), destinationCollisionFilePathName.c_str(), false);

			try
			{
				DeleteFile(sourceCollisionFilePathName.c_str());
			}
			catch (...)
			{

			}

			this->reCreateCollision(true);
		}
	}

	void PhysicsTerrainComponent::handleGeometryChanged(NOWA::EventDataPtr eventData)
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

}; // namespace end
#include "NOWAPrecompiled.h"
#include "PhysicsTerrainComponent.h"
#include "PhysicsComponent.h"
#include "TerraComponent.h"
#include "utilities/XMLConverter.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	using namespace rapidxml;
	using namespace luabind;

	PhysicsTerrainComponent::PhysicsTerrainComponent()
		: PhysicsComponent()
	{

	}

	PhysicsTerrainComponent::~PhysicsTerrainComponent()
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[PhysicsTerrainComponent] Destructor physics terrain component for game object: " + this->gameObjectPtr->getName());
	}

	bool PhysicsTerrainComponent::init(rapidxml::xml_node<>*& propertyElement, const Ogre::String& filename)
	{
		GameObjectComponent::init(propertyElement, filename);

#if 0
		if (propertyElement)
		{
			FILE* file = fopen(filename.c_str(), "rb");
			if (!file)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PhysicsTerrainComponent] Could not open the terrain collision file!");
				return false;
			}

			Ogre::FileHandleDataStream streamFile(file, Ogre::DataStream::READ);
			OgreNewt::CollisionSerializer loadWorldCollision;
			OgreNewt::CollisionPtr col = loadWorldCollision.importCollision(streamFile, this->ogreNewt);
			// OgreNewt::Body *pTerrainBody = new OgreNewt::Body(this->ogreNewt, col);
			this->physicsBody = new OgreNewt::Body(this->ogreNewt, col);
			// this->physicsBody->setGravity(Ogre::Vector3::ZERO);

			streamFile.close();

			propertyElement = propertyElement->next_sibling("property");
		}
#endif

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
			this->destroyCollision();
		
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

		int sizeX = (int)terra->getXZDimensions().x;
		int sizeZ = (int)terra->getXZDimensions().y;

		Ogre::Vector3 center = terra->getTerrainOrigin() + (Ogre::Vector3(terra->getXZDimensions().x, /*is not required: terra->getHeight()*/0, terra->getXZDimensions().y) / 2.0f);

		int startX = (int)terra->getTerrainOrigin().x;
		int endX = (int)terra->getTerrainOrigin().x * -1 + (int)center.x * 2;

		int startZ = (int)terra->getTerrainOrigin().z;
		int endZ = (int)terra->getTerrainOrigin().z * -1 + (int)center.z * 2;

		// terra->setLocalAabb(Ogre::Aabb::newFromExtents(newMin, newMax));

		Ogre::Real* elevation = new Ogre::Real[sizeX * sizeZ];

		int xx = 0;
		int zz = 0;

		for (int x = startX; x < endX; x++)
		{
			for (int z = startZ; z < endZ; z++)
			{
				Ogre::Vector3 pos((Ogre::Real)x, 0.0f, (Ogre::Real)z);
				bool res = terra->getHeightAt(pos);
				xx = (x - (int)terra->getTerrainOrigin().x);
				zz = (z - (int)terra->getTerrainOrigin().z);
				elevation[zz * sizeZ + xx] = pos.y;
			}
		}

		Ogre::Real cellSize = 1.0f;

		char* attibutesCol = new char[sizeX * sizeZ];
		memset(attibutesCol, 0, sizeX * sizeZ * sizeof(char));

		Ogre::Quaternion orientation = Ogre::Quaternion::IDENTITY;
		Ogre::Vector3 position = Ogre::Vector3(terra->getTerrainOrigin().x - this->gameObjectPtr->getPosition().x, this->gameObjectPtr->getPosition().y, terra->getTerrainOrigin().z - this->gameObjectPtr->getPosition().z);

		OgreNewt::CollisionPtr staticCollision = OgreNewt::CollisionPtr(
			new OgreNewt::CollisionPrimitives::HeightField(this->ogreNewt, sizeX, sizeZ, 1, elevation, attibutesCol, 1.0f /* cellSize */, cellSize * 1.0f, cellSize * 1.0f,
				position, orientation, this->gameObjectPtr->getCategoryId())); // move the collision hull to x = -184 and z = -184 as origin

		if (nullptr == this->physicsBody)
		{
			this->physicsBody = new OgreNewt::Body(this->ogreNewt, this->gameObjectPtr->getSceneManager(), staticCollision);
			// Set mass to 0 = infinity = static
			this->physicsBody->setMassMatrix(0.0f, Ogre::Vector3::ZERO);

			// this->physicsBody->setPositionOrientation(this->initialPosition, this->initialOrientation);

			this->setPosition(this->gameObjectPtr->getSceneNode()->getPosition());
			this->setOrientation(this->gameObjectPtr->getSceneNode()->getOrientation());

			this->physicsBody->setUserData(OgreNewt::Any(dynamic_cast<PhysicsComponent*>(this)));

			// here can be the gameobjectPtr be involved
			this->physicsBody->attachNode(this->gameObjectPtr->getSceneNode());

			this->physicsBody->setType(gameObjectPtr->getCategoryId());
			this->physicsBody->setMaterialGroupID(
				AppStateManager::getSingletonPtr()->getGameObjectController()->getMaterialID(this->gameObjectPtr.get(), this->ogreNewt));

			this->gameObjectPtr->setDynamic(false);
		}
		else
		{
			this->physicsBody->setCollision(staticCollision);
		}

		delete[] elevation;
		delete[] attibutesCol;
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

	void PhysicsTerrainComponent::actualizeValue(Variant* attribute)
	{
		GameObjectComponent::actualizeValue(attribute);
	}

	void PhysicsTerrainComponent::writeXML(xml_node<>* propertiesXML, xml_document<>& doc, const Ogre::String& filePath)
	{
		// 2 = int
		// 6 = real
		// 7 = string
		// 8 = vector2
		// 9 = vector3
		// 10 = vector4 -> also quaternion
		// 12 = bool
		GameObjectComponent::writeXML(propertiesXML, doc, filePath);

		// PhysicsComponent::writePhysicsMaterialsProperties(propertiesXML, doc);
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

}; // namespace end
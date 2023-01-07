#include "NOWAPrecompiled.h"
#include "MeshDecal.h"
#include "utilities/MathHelper.h"
#include "main/Core.h"
#include "main/AppStateManager.h"

namespace NOWA
{
	MeshDecal::MeshDecal(Ogre::SceneManager* sceneManager, Ogre::SceneNode* sceneNode, bool showDirectly)
		: sceneManager(sceneManager),
		sceneNode(sceneNode),
		manualObject(nullptr),
		raySceneQuery(nullptr),
		// terrainGroup(nullptr),
		partitionX(18),
		partitionZ(18),
		bShow(showDirectly)
	{
		
	}

	MeshDecal::~MeshDecal()
	{
		if (nullptr != this->manualObject)
		{
			delete this->manualObject;
			this->manualObject = nullptr;
		}
		if (nullptr != this->raySceneQuery)
		{
			this->sceneManager->destroyQuery(this->raySceneQuery);
			this->raySceneQuery = nullptr;
		}
	}
	
	Ogre::ManualObject* MeshDecal::createMeshDecal(const Ogre::String& datablockName, unsigned int partitionX, unsigned int partitionZ)
	{
		this->sceneNode = sceneNode;

		if (nullptr != this->raySceneQuery)
			return this->manualObject;

		this->raySceneQuery = this->sceneManager->createRayQuery(Ogre::Ray());
		this->raySceneQuery->setSortByDistance(true);
		
		this->partitionX = partitionX;
		this->partitionZ = partitionZ;
		// this->manualObject = new Ogre::v1::ManualObject(0, &this->sceneManager->_getEntityMemoryManager(Ogre::SCENE_DYNAMIC), this->sceneManager);
		this->manualObject = this->sceneManager->createManualObject();
		this->manualObject->setRenderQueueGroup(NOWA::RENDER_QUEUE_V2_MESH);
		// this->manualObject->setName("MeshDecal" + Ogre::StringConverter::toString(nextId));
		// this->manualObject->setQueryFlags(0 << 0);
		this->manualObject->estimateVertexCount(partitionX * partitionZ);
		this->manualObject->estimateIndexCount(partitionX * partitionZ);
		
		this->manualObject->begin(datablockName, Ogre::OperationType::OT_TRIANGLE_LIST);
		for (unsigned int i = 0; i <= this->partitionX; i++)
		{
			for (unsigned int j = 0; j <= this->partitionZ; j++)
			{
				this->manualObject->position(Ogre::Vector3(static_cast<Ogre::Real>(i), 0, static_cast<Ogre::Real>(j)));
				this->manualObject->textureCoord(static_cast<Ogre::Real>(i) / static_cast<Ogre::Real>(this->partitionX), static_cast<Ogre::Real>(j) / static_cast<Ogre::Real>(this->partitionZ));
			}
		}

		for (unsigned int i = 0; i < this->partitionX; i++)
		{
			for (unsigned int j = 0; j < this->partitionZ; j++)
			{
				this->manualObject->quad(i * (this->partitionX + 1) + j, i * (this->partitionX + 1) + j + 1, (i + 1) * (this->partitionX + 1) + j + 1, (i + 1) * (this->partitionX + 1) + j);
			}
		}
		this->manualObject->end();
		
		return this->manualObject;
	}

	// http://www.ogre3d.org/tikiwiki/OgreDecal
	// http://www.ogre3d.org/tikiwiki/TerrainMeshDecal&structure=Cookbook
	// http://www.ogre3d.org/tikiwiki/Raycasting+to+the+polygon+level

	void MeshDecal::rebuildMeshDecal(Ogre::Real x, Ogre::Real z, Ogre::Real radius)
	{
		if (nullptr == this->manualObject)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MeshDecal] Cannot rebuild mesh decal, because it does not exist. Call 'createMeshDecal' first.");
			return;
		}

		Ogre::Real x1 = x - radius;
		Ogre::Real z1 = z - radius;

		Ogre::Real stepX = radius * 2 / this->partitionX;
		Ogre::Real stepZ = radius * 2 / this->partitionZ;

		this->manualObject->beginUpdate(0);
		
		// redefine vertices
		for (unsigned int i = 0; i <= this->partitionX; i++)
		{
			for (unsigned int j = 0; j <= this->partitionZ; j++)
			{
				// calculate the ray cast height for all vertex points on the decal (heavy calculation)
				Ogre::Real height = 0.0f;
				MathHelper::getInstance()->getRaycastHeight(x1, z1, raySceneQuery, height);
				this->manualObject->position(Ogre::Vector3(x1, height, z1));
				this->manualObject->textureCoord(static_cast<Ogre::Real>(i) / static_cast<Ogre::Real>(this->partitionX), static_cast<Ogre::Real>(j) / static_cast<Ogre::Real>(this->partitionZ));

				z1 += stepZ;
			}
			x1 += stepX;
			z1 = z - radius;
		}
		// redefine quads
		for (unsigned int i = 0; i < this->partitionX; i++)
		{
			for (unsigned int j = 0; j < this->partitionZ; j++)
			{
				this->manualObject->quad(i * (this->partitionX + 1) + j, i * (this->partitionX + 1) + j + 1, (i + 1) * (this->partitionX + 1) + j + 1, (i + 1) * (this->partitionX + 1) + j);
			}
		}
		this->manualObject->end();
		// this->manualObject->setVisible(true);
	}

	// Does not work correctly with position, because the position is always wrong!
	void MeshDecal::rebuildMeshDecal(const Ogre::Vector3& position, Ogre::Real radius)
	{
		if (nullptr == this->manualObject)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MeshDecal] Cannot rebuild mesh decal, because it does not exist. Call 'createMeshDecal' first.");
			return;
		}

		Ogre::Real x1 = position.x - radius;
		Ogre::Real z1 = position.z - radius;

		Ogre::Real stepX = radius * 2 / this->partitionX;
		Ogre::Real stepZ = radius * 2 / this->partitionZ;

		this->manualObject->beginUpdate(0);

		// redefine vertices
		for (unsigned int i = 0; i <= this->partitionX; i++)
		{
			for (unsigned int j = 0; j <= this->partitionZ; j++)
			{
				// calculate the ray cast height for all vertex points on the decal (heavy calculation)
				this->manualObject->position(position);
				this->manualObject->textureCoord(static_cast<Ogre::Real>(i) / static_cast<Ogre::Real>(this->partitionX), static_cast<Ogre::Real>(j) / static_cast<Ogre::Real>(this->partitionZ));

				z1 += stepZ;
			}
			x1 += stepX;
			z1 = position.z - radius;
		}
		// redefine quads
		for (unsigned int i = 0; i < this->partitionX; i++)
		{
			for (unsigned int j = 0; j < this->partitionZ; j++)
			{
				this->manualObject->quad(i * (this->partitionX + 1) + j, i * (this->partitionX + 1) + j + 1, (i + 1) * (this->partitionX + 1) + j + 1, (i + 1) * (this->partitionX + 1) + j);
			}
		}
		this->manualObject->end();
		// this->manualObject->setVisible(true);
	}

	void MeshDecal::rebuildMeshDecalAndNormal(Ogre::Real x, Ogre::Real z, Ogre::Real radius)
	{
		if (nullptr == this->manualObject)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[MeshDecal] Cannot rebuild mesh decal, because it does not exist. Call 'createMeshDecal' first.");
			return;
		}

		Ogre::Real x1 = x - radius;
		Ogre::Real z1 = z - radius;

		Ogre::Real stepX = radius * 2 / this->partitionX;
		Ogre::Real stepZ = radius * 2 / this->partitionZ;

		this->manualObject->beginUpdate(0);

		// redefine vertices
		for (unsigned int i = 0; i <= this->partitionX; i++)
		{
			for (unsigned int j = 0; j <= this->partitionZ; j++)
			{
				// calculate the ray cast height for all vertex points on the decal (heavy calculation)
				Ogre::Real height = 0.0f;
				// does not work correctly
				MathHelper::getInstance()->getRaycastHeight(x1, z1, raySceneQuery, height);
				this->manualObject->position(Ogre::Vector3(x1, height, z1));
				this->manualObject->textureCoord(static_cast<Ogre::Real>(i) / static_cast<Ogre::Real>(this->partitionX), static_cast<Ogre::Real>(j) / static_cast<Ogre::Real>(this->partitionZ));
				
				z1 += stepZ;
			}
			x1 += stepX;
			z1 = z - radius;
		}
		// redefine quads
		for (unsigned int i = 0; i < this->partitionX; i++)
		{
			for (unsigned int j = 0; j < this->partitionZ; j++)
			{
				this->manualObject->quad(i * (this->partitionX + 1) + j, i * (this->partitionX + 1) + j + 1, (i + 1) * (this->partitionX + 1) + j + 1, (i + 1) * (this->partitionX + 1) + j);
			}
		}
		this->manualObject->end();
		this->manualObject->setVisible(true);
	}

	bool MeshDecal::moveMeshDecalOnMesh(int x, int y, Ogre::Real radius, unsigned int categoryId)
	{
		this->raySceneQuery->setQueryMask(categoryId);
		
		Ogre::Vector3 positionOnModel = Ogre::Vector3::ZERO;
		if (MathHelper::getInstance()->getRaycastFromPoint(x, y, AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera(), Core::getSingletonPtr()->getOgreRenderWindow(), this->raySceneQuery, positionOnModel))
		{
			// this->rebuildMeshDecal(positionOnModel, radius);
			this->rebuildMeshDecal(positionOnModel.x, positionOnModel.z, radius);
			return true;
		}
		else
		{
			return false;
		}
	}

	// Does not work correctly with position, because the position is always wrong!
	bool MeshDecal::moveMeshDecalOnMesh(const Ogre::Vector3& position, Ogre::Real radius, unsigned int categoryId)
	{
		this->raySceneQuery->setQueryMask(categoryId);

		Ogre::Vector3 positionOnModel = Ogre::Vector3::ZERO;
		Ogre::v1::Entity* targetEntity = nullptr;
		Ogre::Real closestDistance = 0.0f;
		Ogre::Vector3 normalOnModel = Ogre::Vector3::ZERO;

		Ogre::Vector3 result = Ogre::Vector3::ZERO;
		Ogre::Ray ray = Ogre::Ray(position, Ogre::Vector3::NEGATIVE_UNIT_Y);
		this->raySceneQuery->setRay(ray);

		if (MathHelper::getInstance()->getRaycastFromPoint(this->raySceneQuery, nullptr, positionOnModel, (size_t&)targetEntity, closestDistance, normalOnModel))
		{
			this->rebuildMeshDecal(positionOnModel, radius);
			return true;
		}
		else
		{
			return false;
		}
	}

	bool MeshDecal::moveAndRotateMeshDecalOnMesh(int x, int y, Ogre::Real radius, unsigned int categoryId)
	{
		this->raySceneQuery->setQueryMask(categoryId);
		
		Ogre::Vector3 positionOnModel = Ogre::Vector3::ZERO;
		Ogre::v1::Entity* targetEntity = nullptr;
		Ogre::Real closestDistance = 0.0f;
		Ogre::Vector3 normalOnModel = Ogre::Vector3::ZERO;
		if (MathHelper::getInstance()->getRaycastFromPoint(x, y, AppStateManager::getSingletonPtr()->getCameraManager()->getActiveCamera(), Core::getSingletonPtr()->getOgreRenderWindow(), this->raySceneQuery, positionOnModel,
				(size_t&)targetEntity, closestDistance, normalOnModel))
		{
			this->rebuildMeshDecal(positionOnModel.x, positionOnModel.z, radius);
			// normalOnModel always delivers 0 1 0 which is not correct
			normalOnModel.normalise();
			this->sceneNode->resetOrientation();
			// Ogre::LogManager::getSingletonPtr()->logMessage("N: " + Ogre::StringConverter::toString(normalOnModel));
			this->sceneNode->setDirection(normalOnModel, Ogre::Node::TS_PARENT, Ogre::Vector3::UNIT_Y);
			return true;
		}
		else
		{
			return false;
		}
	}

	// Does not work correctly with position, because the position is always wrong!
	bool MeshDecal::moveAndRotateMeshDecalOnMesh(const Ogre::Vector3& position, Ogre::Real radius, unsigned int categoryId)
	{
		this->raySceneQuery->setQueryMask(categoryId);

		Ogre::Vector3 positionOnModel = Ogre::Vector3::ZERO;
		Ogre::v1::Entity* targetEntity = nullptr;
		Ogre::Real closestDistance = 0.0f;
		Ogre::Vector3 normalOnModel = Ogre::Vector3::ZERO;

		Ogre::Vector3 result = Ogre::Vector3::ZERO;
		Ogre::Ray ray = Ogre::Ray(position, Ogre::Vector3::NEGATIVE_UNIT_Y);
		this->raySceneQuery->setRay(ray);

		if (MathHelper::getInstance()->getRaycastFromPoint(this->raySceneQuery, nullptr, positionOnModel, (size_t&)targetEntity, closestDistance, normalOnModel))
		{
			this->rebuildMeshDecal(positionOnModel, radius);
			// normalOnModel always delivers 0 1 0 which is not correct
			normalOnModel.normalise();
			this->sceneNode->resetOrientation();
			// Ogre::LogManager::getSingletonPtr()->logMessage("N: " + Ogre::StringConverter::toString(normalOnModel));
			this->sceneNode->setDirection(normalOnModel, Ogre::Node::TS_PARENT, Ogre::Vector3::UNIT_Y);
			return true;
		}
		else
		{
			return false;
		}
	}

	void MeshDecal::show(bool bShow)
	{
		this->bShow = bShow;
		this->sceneNode->setVisible(bShow);
	}

	bool MeshDecal::isShown(void) const
	{
		return this->bShow;
	}


#if 0
	void MeshDecal::setMeshDecal(Ogre::Real x, Ogre::Real z, Ogre::Real radius, Ogre::Terrain* terrain)
	{
		Ogre::Real x1 = x - radius;
		Ogre::Real z1 = z - radius;

		Ogre::Real stepX = radius * 2 / this->partitionX;
		Ogre::Real stepZ = radius * 2 / this->partitionZ;

		this->manualObject->beginUpdate(0);
		// redefine vertices
		for (unsigned int i = 0; i <= this->partitionX; i++)
		{
			for (unsigned int j = 0; j <= this->partitionZ; j++)
			{
				this->manualObject->position(Ogre::Vector3(x1, terrain->getHeightAtTerrainPosition(x1, z1), z1));
				this->manualObject->textureCoord(static_cast<Ogre::Real>(i) / static_cast<Ogre::Real>(this->partitionX), static_cast<Ogre::Real>(j) / static_cast<Ogre::Real>(this->partitionZ));
				z1 += stepZ;
			}
			x1 += stepX;
			z1 = z - radius;
		}
		// redefine quads
		for (unsigned int i = 0; i < this->partitionX; i++)
		{
			for (unsigned int j = 0; j < this->partitionZ; j++)
			{
				this->manualObject->quad(i * (this->partitionX + 1) + j, i * (this->partitionX + 1) + j + 1, (i + 1) * (this->partitionX + 1) + j + 1, (i + 1) * (this->partitionX + 1) + j);
			}
		}
		this->manualObject->end();
		this->manualObject->setVisible(true);
	}
#endif

#if 0
	bool MeshDecal::moveMeshDecalOnTerrain(int x, int y, Ogre::Real radius, Ogre::TerrainGroup* terrainGroup, Ogre::Terrain* terrain)
	{
		Ogre::Real resultX;
		Ogre::Real resultY;
		MathHelper::getInstance()->mouseToViewPort(static_cast<int>(x), static_cast<int>(y), resultX, resultY, Core::getSingletonPtr()->getOgreRenderWindow());
		Ogre::Ray ray = AppStateManager::getSingletonPtr()->getCameraManager()->getCurrentCamera()->getCameraToViewportRay(resultX, resultY);

		Ogre::TerrainGroup::RayResult rayResult = terrainGroup->rayIntersects(ray);
		if (rayResult.hit)
		{
			this->setMeshDecal(rayResult.position.x, rayResult.position.z, radius, terrain);
			return true;
		}
		else
		{
			return false;
		}
	}
#endif

}; //namespace end

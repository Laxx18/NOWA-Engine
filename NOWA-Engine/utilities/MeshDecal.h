#ifndef MESH_DECAL_H
#define MESH_DECAL_H

#include "defines.h"
// #include "OgreTerrainGroup.h"

namespace NOWA
{
	/**
	* @brief The mesh decal can be used to project faces on meshes or terrain.
	* @note	The operation may be realy time consuming. 
	* Also note, that the given ray scene query should have sorted by distance when using mesh decal in indoor or cave scenarios (setSortByDistance(true))
	*/
	class EXPORTED MeshDecal
	{
	public:
		
	public:
		MeshDecal(Ogre::SceneManager* sceneManager, Ogre::SceneNode* sceneNode, bool showDirectly = false);

		~MeshDecal();
		
		Ogre::ManualObject* createMeshDecal(const Ogre::String& datablockName = "Decal4", unsigned int partitionX = 4, unsigned int partitionZ = 4);
#if 0
		bool moveMeshDecalOnTerrain(int x, int y, Ogre::Real radius, Ogre::TerrainGroup* terrainGroup, Ogre::Terrain* terrain);
#endif
		bool moveMeshDecalOnMesh(int x, int y, Ogre::Real radius, unsigned int categoryId);

		bool moveMeshDecalOnMesh(const Ogre::Vector3& position, Ogre::Real radius, unsigned int categoryId);

		bool moveAndRotateMeshDecalOnMesh(int x, int y, Ogre::Real radius, unsigned int categoryId);

		bool moveAndRotateMeshDecalOnMesh(const Ogre::Vector3& position, Ogre::Real radius, unsigned int categoryId);

		void show(bool bShow);

		bool isShown(void) const;
	// private:
#if 0
		void setMeshDecal(Ogre::Real x, Ogre::Real z, Ogre::Real radius, Ogre::Terrain* terrain);
#endif
		void rebuildMeshDecal(Ogre::Real x, Ogre::Real z, Ogre::Real radius);

		void rebuildMeshDecal(const Ogre::Vector3& position, Ogre::Real radius);

		void rebuildMeshDecalAndNormal(Ogre::Real x, Ogre::Real z, Ogre::Real radius);
	private:
		Ogre::ManualObject* manualObject;
		Ogre::SceneManager* sceneManager;
		Ogre::RaySceneQuery* raySceneQuery;

		// Ogre::TerrainGroup* terrainGroup;
		Ogre::SceneNode* sceneNode;
		// number of polygons
		unsigned int partitionX;
		unsigned int partitionZ;
		bool bShow;
	};

}; //namespace end

#endif
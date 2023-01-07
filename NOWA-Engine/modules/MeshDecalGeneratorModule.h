//
//  MeshDecalGeneratorModule.h
//  ogre
//
//  Created by Jonathan Lanis on 9/23/12.
//  Copyright (c) 2012 N/A. All rights reserved.
//

#ifndef MESH_DECAL_GENERATOR_MODULE_H
#define MESH_DECAL_GENERATOR_MODULE_H

#include "defines.h"
#include "OgreManualObject2.h"
#include "utilities/MeshDecalUtility.h"

namespace NOWA
{
	/// Base class
	class EXPORTED TriangleMesh
	{
	public:
		/// Pure virtual function to be left implemented by the user for maximum flexbility
		/// @param triangles An empty list of triangles to be filled with the ones that are inside the AABB
		virtual void findTrianglesInAABB(const Ogre::Vector3& aabbMin, const Ogre::Vector3& aabbMax, std::vector<Triangle>& triangles) = 0;
	};

	/// Only need to create one per mesh
	class EXPORTED OgreMesh : public TriangleMesh
	{
	public:
		OgreMesh() { }
		OgreMesh(const Ogre::v1::MeshPtr& mesh, const Ogre::Vector3& scale);

		void initialize(const Ogre::v1::MeshPtr& mesh, const Ogre::Vector3& scale);

		void findTrianglesInAABB(const Ogre::Vector3& aabbMin, const Ogre::Vector3& aabbMax, std::vector<Triangle>& triangles);

	private:

		std::vector<Triangle> meshTriangles;

	};

	class EXPORTED MeshDecalGeneratorModule
	{
	public:
		friend class AppState; // Only AppState may create this class

		void init(Ogre::SceneManager* sceneManager);

		Ogre::ManualObject* createDecal(TriangleMesh* mesh, const Ogre::Vector3& pos, float width, float height,
			const Ogre::String& materialName, bool flipTexture = false, Ogre::ManualObject* decalObject = 0);

		void turnDebugOn();
		void turnDebugOff();
		void flipDebug();
	private:
		MeshDecalGeneratorModule(const Ogre::String& appStateName);
		~MeshDecalGeneratorModule();

		Ogre::SceneManager* sceneManager;

		Ogre::SceneNode* debugNode;
		bool debugVisible;

		std::vector<UniquePoint> uniquePoints;
		std::vector<Triangle> triangles;
		std::vector<Ogre::Vector3> polygonPoints;
		std::vector<DecalPolygon> finalPolys;

		Ogre::Vector3 clippedPoints[MAX_CLIPPED_POINTS];
		bool pointWasClipped[MAX_CLIPPED_POINTS];

		void AddTriangle(Ogre::ManualObject* mo, std::vector<int>& points, int p1, int p2, int p3, Ogre::ManualObject* lines);

		/// left unimplemented to enforce singleton pattern
		MeshDecalGeneratorModule(MeshDecalGeneratorModule const&);
		void operator=(MeshDecalGeneratorModule const&);
	private:
		Ogre::String appStateName;
	};
}

#endif

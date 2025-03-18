/*
	OgreCrowd
	---------

	Copyright (c) 2012 Jonas Hauquier

	Additional contributions by:

	- mkultra333
	- Paul Wilson

	Sincere thanks and to:

	- Mikko Mononen (developer of Recast navigation libraries)

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.

*/

#include "RecastInputGeom.h"
#include "OgreRecast.h"
#include <OgreStreamSerialiser.h>
#include <OgreMesh2.h>
#include <OgreSubMesh2.h>
#include <OgreBitwise.h>
#include "Vao/OgreAsyncTicket.h"
#include "OgreMeshManager2.h"
#include <float.h>
#include <cstdio>
#include <fstream>

InputGeom::InputGeom(const std::vector<Ogre::v1::Entity*>& srcMeshes, const std::vector<Ogre::Item*>& srcItems)
	: mSrcMeshes(srcMeshes),
	mSrcItems(srcItems),
	// mTerrainGroup(0),
	nverts(0),
	ntris(0),
	mReferenceNode(0),
	bmin(0),
	bmax(0),
	m_offMeshConCount(0),
	m_volumeCount(0),
	m_chunkyMesh(0),
	normals(0),
	verts(0),
	tris(0)
{
	if (srcMeshes.empty() && srcItems.empty())
		return;

	if (!srcMeshes.empty())
	{
		// Convert Ogre::v1::Entity source meshes to a format that recast understands

		//set the reference node
		mReferenceNode = srcMeshes[0]->getParentSceneNode()->getCreator()->getRootSceneNode();
	}
	else if(!srcItems.empty())
	{
		// Convert Ogre::Item source meshes to a format that recast understands

		//set the reference node
		mReferenceNode = srcItems[0]->getParentSceneNode()->getCreator()->getRootSceneNode();
	}

	// Set the area where the navigation mesh will be build.
	// Using bounding box of source mesh and specified cell size
	calculateExtents();

	// Convert ogre geometry (vertices, triangles and normals)
	convertOgreEntities();

	// TODO You don't need to build this in single navmesh mode
	buildChunkyTriMesh();
}

void InputGeom::buildChunkyTriMesh()
{
	m_chunkyMesh = new rcChunkyTriMesh;
	if (!m_chunkyMesh)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage("buildTiledNavigation: Out of memory 'm_chunkyMesh'.");
		return;
	}
	if (!rcCreateChunkyTriMesh(getVerts(), getTris(), getTriCount(), 256, m_chunkyMesh))
	{
		Ogre::LogManager::getSingletonPtr()->logMessage("buildTiledNavigation: Failed to build chunky mesh.");
		return;
	}
}

InputGeom::InputGeom(Ogre::v1::Entity* srcMesh)
	: nverts(0),
	ntris(0),
	mReferenceNode(0),
	bmin(0),
	bmax(0),
	m_offMeshConCount(0),
	m_volumeCount(0),
	m_chunkyMesh(0)
{
	if (!srcMesh)
		return;

	if (srcMesh != nullptr)
	{
		mSrcMeshes.push_back(srcMesh);
		// Convert Ogre::v1::Entity source meshes to a format that recast understands

	    //set the reference node
		mReferenceNode = srcMesh->getParentSceneNode()->getCreator()->getRootSceneNode();
	}

	// Set the area where the navigation mesh will be build.
	// Using bounding box of source mesh and specified cell size
	calculateExtents();


	// Convert ogre geometry (vertices, triangles and normals)
	convertOgreEntities();


	// TODO You don't need to build this in single navmesh mode
	buildChunkyTriMesh();
}

InputGeom::InputGeom(Ogre::Item* srcItem)
	: nverts(0),
	ntris(0),
	mReferenceNode(0),
	bmin(0),
	bmax(0),
	m_offMeshConCount(0),
	m_volumeCount(0),
	m_chunkyMesh(0)
{
	if (!srcItem)
		return;

	if (srcItem != nullptr)
	{
		mSrcItems.push_back(srcItem);
		// Convert Ogre::Item source meshes to a format that recast understands

	    //set the reference node
		mReferenceNode = srcItem->getParentSceneNode()->getCreator()->getRootSceneNode();
	}

	// Set the area where the navigation mesh will be build.
	// Using bounding box of source mesh and specified cell size
	calculateExtents();


	// Convert ogre geometry (vertices, triangles and normals)
	convertOgreEntities();


	// TODO You don't need to build this in single navmesh mode
	buildChunkyTriMesh();
}


// TODO make sure I don't forget destructing some members
InputGeom::~InputGeom()
{
	if (m_chunkyMesh)
		delete m_chunkyMesh;

	if (verts)
		delete[] verts;
	if (normals)
		delete[] normals;
	if (tris)
		delete[] tris;
	if (bmin)
		delete[] bmin;
	if (bmax)
		delete[] bmax;
}

// Used to query scene to get input geometry of a tile directly
// TODO only bounds segmentation that happens at the moment is on entity bounding box, improve this?
// Tile bounds need to be in world space coordinates
InputGeom::InputGeom(const std::vector<Ogre::v1::Entity*>& srcMeshes, const std::vector<Ogre::Item*>& srcItems, const Ogre::AxisAlignedBox& tileBounds)
	: mSrcMeshes(srcMeshes),
	mSrcItems(mSrcItems),
	nverts(0),
	ntris(0),
	mReferenceNode(0),
	bmin(0),
	bmax(0),
	m_offMeshConCount(0),
	m_volumeCount(0),
	m_chunkyMesh(0),
	normals(0),
	verts(0),
	tris(0)
{
	if (srcMeshes.empty() && srcItems.empty())
		return;

	// Convert Ogre::v1::Entity source meshes to a format that recast understands

	//set the reference node
	if (!srcMeshes.empty())
	{
		mReferenceNode = srcMeshes[0]->getParentSceneNode()->getCreator()->getRootSceneNode();
	}
	else
	{
		mReferenceNode = srcItems[0]->getParentSceneNode()->getCreator()->getRootSceneNode();
	}


	// Set the area where the navigation mesh will be build to the tileBounds parameter.
	bmin = new float[3];
	bmax = new float[3];
	OgreRecast::OgreVect3ToFloatA(tileBounds.getMinimum(), bmin);
	OgreRecast::OgreVect3ToFloatA(tileBounds.getMaximum(), bmax);


	// Convert ogre geometry (vertices, triangles and normals)
	convertOgreEntities(tileBounds);


	// TODO You don't need to build this in single navmesh mode
	buildChunkyTriMesh();
}

InputGeom::InputGeom(const std::vector<InputGeom::TerraData>& terraDataList, const std::vector<Ogre::v1::Entity*>& srcMeshes, const std::vector<Ogre::Item*>& srcItems)
	: mSrcMeshes(srcMeshes),
	mSrcItems(srcItems),
	nverts(0),
	ntris(0),
	mReferenceNode(0),
	bmin(0),
	bmax(0),
	m_offMeshConCount(0),
	m_volumeCount(0),
	m_chunkyMesh(0),
	normals(0),
	verts(0),
	tris(0)
{
	// PARTS OF THE FOLLOWING CODE WERE TAKEN AND MODIFIED FROM AN OGRE3D FORUM POST
	const int numNodes = srcMeshes.size() + srcItems.size();

	// Calculate bounds around all entities
	if (mSrcMeshes.size() > 0)
	{
		// Set reference node
		mReferenceNode = mSrcMeshes[0]->getParentSceneNode()->getCreator()->getRootSceneNode();
	}
	else if (mSrcItems.size() > 0)
	{
		// Set reference node
		mReferenceNode = mSrcItems[0]->getParentSceneNode()->getCreator()->getRootSceneNode();
	}

	bmin = new float[3]; bmin[0] = FLT_MAX; bmin[1] = FLT_MAX; bmin[2] = FLT_MAX;
	bmax = new float[3]; bmax[0] = FLT_MIN; bmax[1] = FLT_MIN; bmax[2] = FLT_MIN;

	// TODO: For the moment just one terra page is supported
		// Or will it work, by calling this function with different terra objects? Because each terra will have 
	int pagesTotal = terraDataList.size();
	int pageId = pagesTotal - 1;

	nverts = 0;
	ntris = 0;
	const size_t totalMeshes = pagesTotal + numNodes;
	size_t* meshVertexCount = new size_t[totalMeshes];
	size_t* meshIndexCount = new size_t[totalMeshes];
	Ogre::Vector3** meshVertices = new Ogre::Vector3 * [totalMeshes];
	unsigned long** meshIndices = new unsigned long* [totalMeshes];

	Ogre::Aabb aabb;


	size_t i = 0;

	for (size_t k = 0; k < terraDataList.size(); k++)
	{

		Ogre::Terra* terra = terraDataList[k].terra;
		int sizeX = (int)terra->getXZDimensions().x;
		int sizeZ = (int)terra->getXZDimensions().y;

		Ogre::Vector3 center = terra->getTerrainOrigin() + (Ogre::Vector3(terra->getXZDimensions().x, terra->getHeight(), terra->getXZDimensions().y) / 2.0f);
		// startX = -184
		Ogre::Real startX = terra->getTerrainOrigin().x;
		// endX = 184 + 64 * 2 = 312
		Ogre::Real endX = terra->getTerrainOrigin().x * -1 + (center.x * 2.0f);

		Ogre::Real startY = terra->getTerrainOrigin().y;
		// endX = 184 + 64 * 2 = 312
		Ogre::Real endY = terra->getTerrainOrigin().y * -1 + (center.y * 2.0f);

		Ogre::Real startZ = terra->getTerrainOrigin().z;
		Ogre::Real endZ = terra->getTerrainOrigin().z * -1 + center.z * 2.0f;

		Ogre::Vector3 min = Ogre::Vector3(startX - center.x, terra->getTerrainOrigin().y - center.y, startZ - center.z);
		Ogre::Vector3 max = Ogre::Vector3(endX - center.x, center.y - terra->getTerrainOrigin().y, endZ - center.z);

		aabb = Ogre::Aabb::newFromExtents(min, max);

		size_t sizeVertices = sizeX * sizeZ;

		meshVertices[pageId] = new Ogre::Vector3[sizeVertices];

		int xx = 0;
		int zz = 0;

		for (int x = startX; x < endX; x++)
		{
			for (int z = startZ; z < endZ; z++)
			{
				Ogre::Vector3 pos((Ogre::Real)x, 0.0f, (Ogre::Real)z);

				bool layerMatches = true;
				// Exclude maybe terra layers from vegetation placing
				std::vector<int> layers = terra->getLayerAt(pos);

				for (size_t w = 0; w < terraDataList[k].terraLayerList.size(); w++)
				{
					layerMatches &= layers[w] <= terraDataList[k].terraLayerList[w];
				}

				if (false == layerMatches)
				{
					// Skip this terra position
					// TODO: Or set high y value!, so that the algorithm of recast will skip it!
					continue;
				}

				bool res = terra->getHeightAt(pos);

				meshVertices[pageId][i].x = x;
				meshVertices[pageId][i].y = pos.y;
				meshVertices[pageId][i].z = z;
				i++;
			}
		}

		int newWidth = sizeX - 1;
		int newDepth = sizeZ - 1;

		size_t sizeIndices = newWidth * newDepth * 6;
		meshIndices[pageId] = new unsigned long[sizeIndices];

		int counter = 0;
		int offset = 0;
		for (int y = 0; y < newDepth; y++)
		{
			for (int x = 0; x < newWidth; x++)
			{
				// Write out two triangles
				// Note: All algorithms are valid, but order seems to be important and using the other algorithms, no sloping for terra is possible!
				meshIndices[pageId][counter++] = offset + 0;
				meshIndices[pageId][counter++] = offset + 1;
				meshIndices[pageId][counter++] = offset + (newWidth + 1);

				meshIndices[pageId][counter++] = offset + 1;
				meshIndices[pageId][counter++] = offset + (newWidth + 1) + 1;
				meshIndices[pageId][counter++] = offset + (newWidth + 1);

				offset++;
			}
			offset++;
		}

		meshVertexCount[pageId] = sizeVertices;
		meshIndexCount[pageId] = sizeIndices;

		nverts += meshVertexCount[pageId];
		ntris += meshIndexCount[pageId];

	}


	// Attention: What about different y-of terra?
	// Could not create useful navigation, must I use like in the example navmesh creation out of heightmap?
	// See: _generateRecastMesh rcCreateHeightfield

	//-----------------------------------------------------------------------------------------
	// ENTITY DATA BUILDING

	// Attention: Setting i = 0, resets everything for a terra page?
	i = 0;
	for (std::vector<Ogre::v1::Entity*>::iterator iter = mSrcMeshes.begin(); iter != mSrcMeshes.end(); iter++)
	{
		bool isPlanet = false;

		Ogre::String name = (*iter)->getMesh()->getName();
		// Nothing is created anymore
#if 0
		if (name == "geosphere8000.mesh")
		{
			isPlanet = true;
		}
#endif

		int ind = pagesTotal + i;
		getMeshInformation((*iter)->getMesh(), meshVertexCount[ind], meshVertices[ind], meshIndexCount[ind], meshIndices[ind], isPlanet);

		//total number of verts
		nverts += meshVertexCount[ind];
		//total number of indices
		ntris += meshIndexCount[ind];

		i++;
	}

	for (std::vector<Ogre::Item*>::iterator iter = mSrcItems.begin(); iter != mSrcItems.end(); iter++)
	{
		int ind = pagesTotal + i;
		bool isPlanet = false;

		Ogre::String name = (*iter)->getMesh()->getName();

		// Nothing is created anymore
#if 0
		if (name == "geosphere8000.mesh")
		{
			isPlanet = true;
		}
#endif

		getMeshInformation2((*iter)->getMesh(), meshVertexCount[ind], meshVertices[ind], meshIndexCount[ind], meshIndices[ind], isPlanet);

		//total number of verts
		nverts += meshVertexCount[ind];
		//total number of indices
		ntris += meshIndexCount[ind];

		i++;
	}


	//-----------------------------------------------------------------------------------------
	// DECLARE RECAST DATA BUFFERS USING THE INFO WE GRABBED ABOVE

	verts = new float[nverts * 3];// *3 as verts holds x,y,&z for each verts in the array
	tris = new int[ntris];// tris in recast is really indices like ogre

	//convert index count into tri count
	ntris = ntris / 3; //although the tris array are indices the ntris is actual number of triangles, eg. indices/3;


	//-----------------------------------------------------------------------------------------
	// RECAST TERRAIN DATA BUILDING

	//copy all meshes verticies into single buffer and transform to world space relative to parentNode
	int vertsIndex = 0;
	int prevVerticiesCount = 0;
	int prevIndexCountTotal = 0;

	for (size_t m = 0; m < pagesTotal; ++m)
	{
		//We don't need to transform terrain verts, they are already in world space!
		Ogre::Vector3 vertexPos;
		for (size_t n = 0; n < meshVertexCount[m]; ++n)
		{
			vertexPos = meshVertices[m][n];
			verts[vertsIndex] = vertexPos.x;
			verts[vertsIndex + 1] = vertexPos.y;
			verts[vertsIndex + 2] = vertexPos.z;
			vertsIndex += 3;
		}

		for (size_t n = 0; n < meshIndexCount[m]; n++)
		{
			tris[prevIndexCountTotal + n] = meshIndices[m][n] + prevVerticiesCount;
		}
		prevIndexCountTotal += meshIndexCount[m];
		prevVerticiesCount += meshVertexCount[m];

	}

	//-----------------------------------------------------------------------------------------
	// RECAST TERRAIN ENTITY DATA BUILDING

	//copy all meshes verticies into single buffer and transform to world space relative to parentNode
	// DO NOT RESET THESE VALUES
	// we need to keep the vert/index offset we have from the terrain generation above to make sure
	// we start referencing recast's buffers from the correct place, otherwise we just end up
	// overwriting our terrain data, which really is a pain ;)

	//set the reference node
	// Attention: Setting i = 0, resets everything for a terra page?
	i = 0;
	for (std::vector<Ogre::v1::Entity*>::iterator iter = mSrcMeshes.begin(); iter != mSrcMeshes.end(); iter++)
	{
		int ind = pagesTotal + i;
		//find the transform between the reference node and this node
		Ogre::Matrix4 transform = mReferenceNode->_getFullTransform().inverse() * (*iter)->getParentSceneNode()->_getFullTransform();
		Ogre::Vector3 vertexPos;
		for (size_t j = 0; j < meshVertexCount[ind]; j++)
		{
			vertexPos = transform * meshVertices[ind][j];
			verts[vertsIndex] = vertexPos.x;
			verts[vertsIndex + 1] = vertexPos.y;
			verts[vertsIndex + 2] = vertexPos.z;
			vertsIndex += 3;
		}

		for (size_t j = 0; j < meshIndexCount[ind]; j++)
		{
			tris[prevIndexCountTotal + j] = meshIndices[ind][j] + prevVerticiesCount;
		}
		prevIndexCountTotal += meshIndexCount[ind];
		prevVerticiesCount += meshVertexCount[ind];

		i++;
	}

	for (std::vector<Ogre::Item*>::iterator iter = mSrcItems.begin(); iter != mSrcItems.end(); iter++)
	{
		int ind = pagesTotal + i;
		//find the transform between the reference node and this node
		Ogre::Matrix4 transform = mReferenceNode->_getFullTransform().inverse() * (*iter)->getParentSceneNode()->_getFullTransform();
		Ogre::Vector3 vertexPos;
		for (size_t j = 0; j < meshVertexCount[ind]; j++)
		{
			vertexPos = transform * meshVertices[ind][j];
			verts[vertsIndex] = vertexPos.x;
			verts[vertsIndex + 1] = vertexPos.y;
			verts[vertsIndex + 2] = vertexPos.z;
			vertsIndex += 3;
		}

		for (size_t j = 0; j < meshIndexCount[ind]; j++)
		{
			tris[prevIndexCountTotal + j] = meshIndices[ind][j] + prevVerticiesCount;
		}
		prevIndexCountTotal += meshIndexCount[ind];
		prevVerticiesCount += meshVertexCount[ind];

		i++;
	}

	// first 4 were created differently, without getMeshInformation();
	// throws an exception if we delete the first 4
	// TODO - FIX THIS MEMORY LEAK - its only small, but its still not good
	for (size_t j = pagesTotal; j < totalMeshes; ++j)
	{
		delete[] meshIndices[j];
	}

	delete[] meshVertices;
	delete[] meshVertexCount;
	delete[] meshIndices;
	delete[] meshIndexCount;

	Ogre::LogManager::getSingletonPtr()->logMessage("Recast terra and entities: " + Ogre::StringConverter::toString(mSrcMeshes.size()) + " items: " + Ogre::StringConverter::toString(mSrcItems.size()));

	Ogre::LogManager::getSingletonPtr()->logMessage("Entities: ");
	for (size_t j = 0; j < mSrcMeshes.size(); j++)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage("Name: " + mSrcMeshes[j]->getMesh()->getName());
	}
	if (false == mSrcItems.empty())
	{
		Ogre::LogManager::getSingletonPtr()->logMessage("Items: ");
		for (size_t j = 0; j < mSrcItems.size(); j++)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage("Name: " + mSrcItems[j]->getMesh()->getName());
		}
	}

	//---------------------------------------------------------------------------------------------
	// RECAST **ONLY** NORMAL CALCS ( These are not used anywhere other than internally by recast)

	normals = new float[ntris * 3];
	for (int i = 0; i < ntris * 3; i += 3)
	{
		const float* v0 = &verts[tris[i] * 3];
		const float* v1 = &verts[tris[i + 1] * 3];
		const float* v2 = &verts[tris[i + 2] * 3];
		float e0[3], e1[3];
		for (int j = 0; j < 3; ++j)
		{
			e0[j] = (v1[j] - v0[j]);
			e1[j] = (v2[j] - v0[j]);
		}
		float* n = &normals[i];
		n[0] = ((e0[1] * e1[2]) - (e0[2] * e1[1]));
		n[1] = ((e0[2] * e1[0]) - (e0[0] * e1[2]));
		n[2] = ((e0[0] * e1[1]) - (e0[1] * e1[0]));

		float d = sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
		if (d > 0)
		{
			d = 1.0f / d;
			n[0] *= d;
			n[1] *= d;
			n[2] *= d;
		}
	}

	// Set the area where the navigation mesh will be build.
	// Using bounding box of source mesh and specified cell size
	calculateExtents();

	Ogre::Vector3 min = aabb.getMinimum();
	if (min.x < bmin[0])
		bmin[0] = min.x;
	if (min.y < bmin[1])
		bmin[1] = min.y;
	if (min.z < bmin[2])
		bmin[2] = min.z;

	Ogre::Vector3 max = aabb.getMaximum();
	if (max.x > bmax[0])
		bmax[0] = max.x;
	if (max.y > bmax[1])
		bmax[1] = max.y;
	if (max.z > bmax[2])
		bmax[2] = max.z;

	// Build chunky tri mesh from triangles, used for tiled navmesh construction
	buildChunkyTriMesh();
}

void InputGeom::convertOgreEntities()
{
	//Convert all vertices and triangles to recast format
	const int numNodes = mSrcMeshes.size() + mSrcItems.size();
	size_t* meshVertexCount = new size_t[numNodes];
	size_t* meshIndexCount = new size_t[numNodes];
	Ogre::Vector3** meshVertices = new Ogre::Vector3 * [numNodes];
	unsigned long** meshIndices = new unsigned long* [numNodes];

	nverts = 0;
	ntris = 0;
	size_t i = 0;
	for (std::vector<Ogre::v1::Entity*>::iterator iter = mSrcMeshes.begin(); iter != mSrcMeshes.end(); iter++)
	{
		bool isPlanet = false;

		Ogre::String name = (*iter)->getMesh()->getName();

		// Nothing is created anymore
#if 0
		if (name == "geosphere8000.mesh")
		{
			isPlanet = true;
		}
#endif

		getMeshInformation((*iter)->getMesh(), meshVertexCount[i], meshVertices[i], meshIndexCount[i], meshIndices[i], isPlanet, Ogre::Vector3::ZERO, Ogre::Quaternion::IDENTITY /*, (*iter)->getParentSceneNode()->getScale()*/);
		//total number of verts
		nverts += meshVertexCount[i];
		//total number of indices
		ntris += meshIndexCount[i];

		i++;
	}

	for (std::vector<Ogre::Item*>::iterator iter = mSrcItems.begin(); iter != mSrcItems.end(); iter++)
	{
		bool isPlanet = false;

		Ogre::String name = (*iter)->getMesh()->getName();

		// Nothing is created anymore
#if 0
		if (name == "geosphere8000.mesh")
		{
			isPlanet = true;
		}
#endif

		getMeshInformation2((*iter)->getMesh(), meshVertexCount[i], meshVertices[i], meshIndexCount[i], meshIndices[i], isPlanet, Ogre::Vector3::ZERO, Ogre::Quaternion::IDENTITY /*, (*iter)->getParentSceneNode()->getScale()*/);
		//total number of verts
		nverts += meshVertexCount[i];
		//total number of indices
		ntris += meshIndexCount[i];

		i++;
	}

	// DECLARE RECAST DATA BUFFERS USING THE INFO WE GRABBED ABOVE
	verts = new float[nverts * 3];// *3 as verts holds x,y,&z for each verts in the array
	tris = new int[ntris];// tris in recast is really indices like ogre

	//convert index count into tri count
	ntris = ntris / 3; //although the tris array are indices the ntris is actual number of triangles, eg. indices/3;

	//copy all meshes verticies into single buffer and transform to world space relative to parentNode
	int vertsIndex = 0;
	int prevVerticiesCount = 0;
	int prevIndexCountTotal = 0;
	i = 0;
	for (std::vector<Ogre::v1::Entity*>::iterator iter = mSrcMeshes.begin(); iter != mSrcMeshes.end(); iter++)
	{
		Ogre::v1::Entity* ent = *iter;
		Ogre::Vector3 scale = ent->getParentSceneNode()->getScale();
		//find the transform between the reference node and this node
		Ogre::Matrix4 transform = mReferenceNode->_getFullTransform().inverse() * ent->getParentSceneNode()->_getFullTransform();
		Ogre::Vector3 vertexPos;
		for (size_t j = 0; j < meshVertexCount[i]; j++)
		{
			vertexPos = transform * meshVertices[i][j]/* * scale*/;
			verts[vertsIndex] = vertexPos.x;
			verts[vertsIndex + 1] = vertexPos.y;
			verts[vertsIndex + 2] = vertexPos.z;
			vertsIndex += 3;
		}

		for (size_t j = 0; j < meshIndexCount[i]; j++)
		{
			tris[prevIndexCountTotal + j] = meshIndices[i][j] + prevVerticiesCount;
		}
		prevIndexCountTotal += meshIndexCount[i];
		prevVerticiesCount += meshVertexCount[i];

		i++;
	}

	for (std::vector<Ogre::Item*>::iterator iter = mSrcItems.begin(); iter != mSrcItems.end(); iter++)
	{
		Ogre::Item* item = *iter;
		Ogre::Vector3 scale = item->getParentSceneNode()->getScale();
		//find the transform between the reference node and this node
		Ogre::Matrix4 transform = mReferenceNode->_getFullTransform().inverse() * item->getParentSceneNode()->_getFullTransform();
		Ogre::Vector3 vertexPos;
		for (size_t j = 0; j < meshVertexCount[i]; j++)
		{
			vertexPos = transform * meshVertices[i][j]/* * scale*/;
			verts[vertsIndex] = vertexPos.x;
			verts[vertsIndex + 1] = vertexPos.y;
			verts[vertsIndex + 2] = vertexPos.z;
			vertsIndex += 3;
		}

		for (size_t j = 0; j < meshIndexCount[i]; j++)
		{
			tris[prevIndexCountTotal + j] = meshIndices[i][j] + prevVerticiesCount;
		}
		prevIndexCountTotal += meshIndexCount[i];
		prevVerticiesCount += meshVertexCount[i];

		i++;
	}

	//delete tempory arrays
	//TODO These probably could member varibles, this would increase performance slightly
	delete[] meshVertices;
	delete[] meshIndices;
	delete[] meshVertexCount;
	delete[] meshIndexCount;

	// calculate normals data for Recast - im not 100% sure where this is required
	// but it is used, Ogre handles its own Normal data for rendering, this is not related
	// to Ogre at all ( its also not correct lol )
	// TODO : fix this
	normals = new float[ntris * 3];

	for (int i = 0; i < ntris * 3; i += 3)
	{
		const float* v0 = &verts[tris[i] * 3];
		const float* v1 = &verts[tris[i + 1] * 3];
		const float* v2 = &verts[tris[i + 2] * 3];
		float e0[3], e1[3];
		for (int j = 0; j < 3; ++j)
		{
			e0[j] = (v1[j] - v0[j]);
			e1[j] = (v2[j] - v0[j]);
		}
		float* n = &normals[i];
		n[0] = ((e0[1] * e1[2]) - (e0[2] * e1[1]));
		n[1] = ((e0[2] * e1[0]) - (e0[0] * e1[2]));
		n[2] = ((e0[0] * e1[1]) - (e0[1] * e1[0]));

		float d = sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
		if (d > 0)
		{
			d = 1.0f / d;
			n[0] *= d;
			n[1] *= d;
			n[2] *= d;
		}
	}
}

void InputGeom::convertOgreEntities(const Ogre::AxisAlignedBox& tileBounds)
{
	// Select only entities that fall at least partly within tileBounds
	std::vector<Ogre::v1::Entity*> selectedEntities;
	std::vector<Ogre::Item*> selectedItems;
	Ogre::AxisAlignedBox bb;
	Ogre::Aabb aabb;
	Ogre::Matrix4 transform;
	for (std::vector<Ogre::v1::Entity*>::iterator iter = mSrcMeshes.begin(); iter != mSrcMeshes.end(); iter++)
	{
		transform = mReferenceNode->_getFullTransform().inverse() * (*iter)->getParentSceneNode()->_getFullTransform();
		bb = (*iter)->getMesh()->getBounds();
		bb.transform(transform);    // Transform to world coordinates
		if (bb.intersects(tileBounds))
			selectedEntities.push_back(*iter);
	}
	mSrcMeshes.clear();
	mSrcMeshes = selectedEntities;

	for (std::vector<Ogre::Item*>::iterator iter = mSrcItems.begin(); iter != mSrcItems.end(); iter++)
	{
		transform = mReferenceNode->_getFullTransform().inverse() * (*iter)->getParentSceneNode()->_getFullTransform();
		aabb = (*iter)->getMesh()->getAabb();
		aabb.transformAffine(transform);    // Transform to world coordinates
		if (bb.intersects(tileBounds))
			selectedItems.push_back(*iter);
	}
	mSrcItems.clear();
	mSrcItems = selectedItems;

	convertOgreEntities();
}

void InputGeom::calculateExtents()
{
	Ogre::Vector3 min = Ogre::Vector3(Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY);
	Ogre::Vector3 max = Ogre::Vector3(Ogre::Math::NEG_INFINITY, Ogre::Math::NEG_INFINITY, Ogre::Math::NEG_INFINITY);
	Ogre::Vector3 min2 = Ogre::Vector3(Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY, Ogre::Math::POS_INFINITY);
	Ogre::Vector3 max2 = Ogre::Vector3(Ogre::Math::NEG_INFINITY, Ogre::Math::NEG_INFINITY, Ogre::Math::NEG_INFINITY);

	if (nullptr != bmin)
	{
		OgreRecast::FloatAToOgreVect3(bmin, min);
		OgreRecast::FloatAToOgreVect3(bmin, min2);
	}

	if (nullptr != bmax)
	{
		OgreRecast::FloatAToOgreVect3(bmax, max);
		OgreRecast::FloatAToOgreVect3(bmax, max2);
	}

	if (!mSrcMeshes.empty())
	{
		Ogre::v1::Entity* ent = mSrcMeshes[0];
		Ogre::AxisAlignedBox srcMeshBB = ent->getMesh()->getBounds();
		Ogre::Matrix4 transform = mReferenceNode->_getFullTransform().inverse() * ent->getParentSceneNode()->_getFullTransform();
		srcMeshBB.transform(transform);
		min = srcMeshBB.getMinimum() * ent->getParentSceneNode()->getScale();
		max = srcMeshBB.getMaximum() * ent->getParentSceneNode()->getScale();

		// Calculate min and max from all entities
		for (std::vector<Ogre::v1::Entity*>::iterator iter = mSrcMeshes.begin(); iter != mSrcMeshes.end(); iter++)
		{
			Ogre::v1::Entity* ent = *iter;

			//find the transform between the reference node and this node
			transform = mReferenceNode->_getFullTransform().inverse() * ent->getParentSceneNode()->_getFullTransform();

			Ogre::AxisAlignedBox srcMeshBB = ent->getMesh()->getBounds();
			srcMeshBB.transform(transform);
			min2 = srcMeshBB.getMinimum() * ent->getParentSceneNode()->getScale();
			if (min2.x < min.x)
				min.x = min2.x;
			if (min2.y < min.y)
				min.y = min2.y;
			if (min2.z < min.z)
				min.z = min2.z;

			max2 = srcMeshBB.getMaximum() * ent->getParentSceneNode()->getScale();
			if (max2.x > max.x)
				max.x = max2.x;
			if (max2.y > max.y)
				max.y = max2.y;
			if (max2.z > max.z)
				max.z = max2.z;
		}

	}

	if (!mSrcItems.empty())
	{
		Ogre::Item* item = mSrcItems[0];
		Ogre::Aabb srcMeshBB = item->getMesh()->getAabb();
		Ogre::Matrix4 transform = mReferenceNode->_getFullTransform().inverse() * item->getParentSceneNode()->_getFullTransform();
		srcMeshBB.transformAffine(transform);
		if (mSrcMeshes.empty())
		{
			min = srcMeshBB.getMinimum() * item->getParentSceneNode()->getScale();
			max = srcMeshBB.getMaximum() * item->getParentSceneNode()->getScale();
		}

		// Calculate min and max from all entities
		for (std::vector<Ogre::Item*>::iterator iter = mSrcItems.begin(); iter != mSrcItems.end(); iter++)
		{
			Ogre::Item* item = *iter;

			//find the transform between the reference node and this node
			transform = mReferenceNode->_getFullTransform().inverse() * item->getParentSceneNode()->_getFullTransform();

			Ogre::Aabb srcMeshBB = item->getMesh()->getAabb();
			srcMeshBB.transformAffine(transform);

			min2 = srcMeshBB.getMinimum() * item->getParentSceneNode()->getScale();
			if (min2.x < min.x)
				min.x = min2.x;
			if (min2.y < min.y)
				min.y = min2.y;
			if (min2.z < min.z)
				min.z = min2.z;

			max2 = srcMeshBB.getMaximum() * item->getParentSceneNode()->getScale();
			if (max2.x > max.x)
				max.x = max2.x;
			if (max2.y > max.y)
				max.y = max2.y;
			if (max2.z > max.z)
				max.z = max2.z;
		}
	}

	if (!bmin)
		bmin = new float[3];
	if (!bmax)
		bmax = new float[3];
	OgreRecast::OgreVect3ToFloatA(min, bmin);
	OgreRecast::OgreVect3ToFloatA(max, bmax);

	Ogre::LogManager::getSingletonPtr()->logMessage("Recast extends min: " + Ogre::StringConverter::toString(min) + " max: " + Ogre::StringConverter::toString(max));
}

float* InputGeom::getMeshBoundsMax()
{
	return bmax;
}

float* InputGeom::getMeshBoundsMin()
{
	return bmin;
}

int InputGeom::getVertCount()
{
	return nverts;
}

int InputGeom::getTriCount()
{
	return ntris;
}

int* InputGeom::getTris()
{
	return tris;
}

float* InputGeom::getVerts()
{
	return verts;
}

bool InputGeom::isEmpty()
{
	return nverts <= 0 || ntris <= 0;
}

#if 0
void InputGeom::getMeshInformation(const Ogre::v1::MeshPtr mesh,
	size_t& vertex_count,
	Ogre::Vector3*& vertices,
	size_t& index_count,
	unsigned long*& indices,
	const Ogre::Vector3& position,
	const Ogre::Quaternion& orient,
	const Ogre::Vector3& scale)
{
	bool added_shared = false;
	size_t current_offset = 0;
	size_t shared_offset = 0;
	size_t next_offset = 0;
	size_t index_offset = 0;

	vertex_count = index_count = 0;

	// Calculate how many vertices and indices we're going to need
	for (unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i)
	{
		Ogre::v1::SubMesh* submesh = mesh->getSubMesh(i);
		// We only need to add the shared vertices once
		if (submesh->useSharedVertices)
		{
			if (!added_shared)
			{
				vertex_count += mesh->sharedVertexData[0]->vertexCount;
				added_shared = true;
			}
		}
		else
		{
			vertex_count += submesh->vertexData[0]->vertexCount;
		}
		// Add the indices
		index_count += submesh->indexData[0]->indexCount;
	}

	// Allocate space for the vertices and indices
	vertices = new Ogre::Vector3[vertex_count];
	indices = new unsigned long[index_count];

	added_shared = false;

	// Run through the submeshes again, adding the data into the arrays
	for (unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i)
	{
		Ogre::v1::SubMesh* submesh = mesh->getSubMesh(i);

		Ogre::v1::VertexData* vertex_data = submesh->useSharedVertices ? mesh->sharedVertexData[0] : submesh->vertexData[0];

		if ((!submesh->useSharedVertices) || (submesh->useSharedVertices && !added_shared))
		{
			if (submesh->useSharedVertices)
			{
				added_shared = true;
				shared_offset = current_offset;
			}

			const Ogre::v1::VertexElement* posElem =
				vertex_data->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);

			Ogre::v1::HardwareVertexBufferSharedPtr vbuf =
				vertex_data->vertexBufferBinding->getBuffer(posElem->getSource());

			unsigned char* vertex =
				static_cast<unsigned char*>(vbuf->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));

			// There is _no_ baseVertexPointerToElement() which takes an Ogre::Real or a double
			//  as second argument. So make it float, to avoid trouble when Ogre::Real will
			//  be comiled/typedefed as double:
			//Ogre::Real* pReal;
			float* pReal;

			for (size_t j = 0; j < vertex_data->vertexCount; ++j, vertex += vbuf->getVertexSize())
			{
				posElem->baseVertexPointerToElement(vertex, &pReal);
				Ogre::Vector3 pt(pReal[0], pReal[1], pReal[2]);
				vertices[current_offset + j] = (orient * (pt * scale)) + position;
			}

			vbuf->unlock();
			next_offset += vertex_data->vertexCount;
		}

		Ogre::v1::IndexData* index_data = submesh->indexData[0];
		size_t numTris = index_data->indexCount / 3;
		Ogre::v1::HardwareIndexBufferSharedPtr ibuf = index_data->indexBuffer;

		bool use32bitindexes = (ibuf->getType() == Ogre::v1::HardwareIndexBuffer::IT_32BIT);

		unsigned long* pLong = static_cast<unsigned long*>(ibuf->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));
		unsigned short* pShort = reinterpret_cast<unsigned short*>(pLong);

		size_t offset = (submesh->useSharedVertices) ? shared_offset : current_offset;

		if (use32bitindexes)
		{
			for (size_t k = 0; k < numTris * 3; ++k)
			{
				indices[index_offset++] = pLong[k] + static_cast<unsigned long>(offset);
			}
		}
		else
		{
			for (size_t k = 0; k < numTris * 3; ++k)
			{
				indices[index_offset++] = static_cast<unsigned long>(pShort[k]) +
					static_cast<unsigned long>(offset);
			}
		}

		ibuf->unlock();
		current_offset = next_offset;
	}
};
#else
void InputGeom::getMeshInformation(const Ogre::v1::MeshPtr mesh,
	size_t& vertex_count,
	Ogre::Vector3*& vertices,
	size_t& index_count,
	unsigned long*& indices,
	bool isPlanet,
	const Ogre::Vector3& position,
	const Ogre::Quaternion& orient,
	const Ogre::Vector3& scale)
{
	bool added_shared = false;
	size_t current_offset = 0;
	size_t shared_offset = 0;
	size_t next_offset = 0;
	size_t index_offset = 0;

	vertex_count = index_count = 0;

	Ogre::AxisAlignedBox bbox = mesh->getBounds();
	Ogre::Vector3 center = bbox.getCenter();
	float radius = bbox.getHalfSize().length();  // You might want to use this for a geosphere, not bounding box.

	for (unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i)
	{
		Ogre::v1::SubMesh* submesh = mesh->getSubMesh(i);
		if (submesh->useSharedVertices)
		{
			if (!added_shared)
			{
				vertex_count += mesh->sharedVertexData[0]->vertexCount;
				added_shared = true;
			}
		}
		else
		{
			vertex_count += submesh->vertexData[0]->vertexCount;
		}
		index_count += submesh->indexData[0]->indexCount;
	}

	vertices = new Ogre::Vector3[vertex_count];
	indices = new unsigned long[index_count];

	added_shared = false;

	for (unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i)
	{
		Ogre::v1::SubMesh* submesh = mesh->getSubMesh(i);
		Ogre::v1::VertexData* vertex_data = submesh->useSharedVertices ? mesh->sharedVertexData[0] : submesh->vertexData[0];

		if ((!submesh->useSharedVertices) || (submesh->useSharedVertices && !added_shared))
		{
			if (submesh->useSharedVertices)
			{
				added_shared = true;
				shared_offset = current_offset;
			}

			const Ogre::v1::VertexElement* posElem =
				vertex_data->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);

			Ogre::v1::HardwareVertexBufferSharedPtr vbuf =
				vertex_data->vertexBufferBinding->getBuffer(posElem->getSource());

			unsigned char* vertex =
				static_cast<unsigned char*>(vbuf->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));

			float* pReal;

			for (size_t j = 0; j < vertex_data->vertexCount; ++j, vertex += vbuf->getVertexSize())
			{
				posElem->baseVertexPointerToElement(vertex, &pReal);
				Ogre::Vector3 pt(pReal[0], pReal[1], pReal[2]);

				if (isPlanet)
				{
					// Normalize the vertex to the sphere surface
					Ogre::Vector3 dir = (pt - center).normalisedCopy();
					pt = center + dir * radius;  // Adjust based on radius
				}

				// Apply position and orientation transformation
				vertices[current_offset + j] = (orient * (pt * scale)) + position;
			}

			vbuf->unlock();
			next_offset += vertex_data->vertexCount;
		}

		Ogre::v1::IndexData* index_data = submesh->indexData[0];
		size_t numTris = index_data->indexCount / 3;
		Ogre::v1::HardwareIndexBufferSharedPtr ibuf = index_data->indexBuffer;

		bool use32bitindexes = (ibuf->getType() == Ogre::v1::HardwareIndexBuffer::IT_32BIT);

		unsigned long* pLong = static_cast<unsigned long*>(ibuf->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));
		unsigned short* pShort = reinterpret_cast<unsigned short*>(pLong);

		size_t offset = (submesh->useSharedVertices) ? shared_offset : current_offset;

		if (use32bitindexes)
		{
			for (size_t k = 0; k < numTris * 3; ++k)
			{
				indices[index_offset++] = pLong[k] + static_cast<unsigned long>(offset);
			}
		}
		else
		{
			for (size_t k = 0; k < numTris * 3; ++k)
			{
				indices[index_offset++] = static_cast<unsigned long>(pShort[k]) +
					static_cast<unsigned long>(offset);
			}
		}

		ibuf->unlock();
		current_offset = next_offset;
	}
}
#endif

#if 0
void InputGeom::getMeshInformation2(const Ogre::MeshPtr mesh, size_t& vertexCount, Ogre::Vector3*& vertices, size_t& indexCount, unsigned long*& indices,
	const Ogre::Vector3& position, const Ogre::Quaternion& orientation, const Ogre::Vector3& scale)
{
	//First, we compute the total number of vertices and indices and init the buffers.
	unsigned int numVertices = 0;
	unsigned int numIndices = 0;
	//MeshInfo outInfo; 

	Ogre::Mesh::SubMeshVec::const_iterator subMeshIterator = mesh->getSubMeshes().begin();

	while (subMeshIterator != mesh->getSubMeshes().end())
	{
		Ogre::SubMesh *subMesh = *subMeshIterator;
		numVertices += static_cast<unsigned int>(subMesh->mVao[0][0]->getVertexBuffers()[0]->getNumElements());
		numIndices += static_cast<unsigned int>(subMesh->mVao[0][0]->getIndexBuffer()->getNumElements());

		subMeshIterator++;
	}

	//allocate memory to the input array reference, handleRequest calls delete on this memory
	vertices = OGRE_ALLOC_T(Ogre::Vector3, numVertices, Ogre::MEMCATEGORY_GEOMETRY);
	indices = OGRE_ALLOC_T(unsigned long, numIndices, Ogre::MEMCATEGORY_GEOMETRY);

	vertexCount = numVertices; //used later in handleRequest.
	indexCount = numIndices;

	unsigned int addedVertices = 0;
	unsigned int addedIndices = 0;

	unsigned int index_offset = 0;
	unsigned int subMeshOffset = 0;

	/*
	Read submeshes
	*/
	subMeshIterator = mesh->getSubMeshes().begin();
	while (subMeshIterator != mesh->getSubMeshes().end())
	{
		Ogre::SubMesh *subMesh = *subMeshIterator;
		Ogre::VertexArrayObjectArray vaos = subMesh->mVao[0];

		if (!vaos.empty())
		{
			//Get the first LOD level 
			Ogre::VertexArrayObject *vao = vaos[0];
			bool indices32 = (vao->getIndexBuffer()->getIndexType() == Ogre::IndexBufferPacked::IT_32BIT);

			const Ogre::VertexBufferPackedVec &vertexBuffers = vao->getVertexBuffers();
			Ogre::IndexBufferPacked *indexBuffer = vao->getIndexBuffer();

			//request async read from buffer 
			Ogre::VertexArrayObject::ReadRequestsVec requests;
			requests.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_POSITION));

			vao->readRequests(requests);
			vao->mapAsyncTickets(requests);
			unsigned int subMeshVerticiesNum = static_cast<unsigned int>(requests[0].vertexBuffer->getNumElements());
			if (requests[0].type == Ogre::VET_HALF4)
			{
				for (size_t i = 0; i < subMeshVerticiesNum; ++i)
				{
					const Ogre::uint16* pos = reinterpret_cast<const Ogre::uint16*>(requests[0].data);
					Ogre::Vector3 vec;
					vec.x = Ogre::Bitwise::halfToFloat(pos[0]);
					vec.y = Ogre::Bitwise::halfToFloat(pos[1]);
					vec.z = Ogre::Bitwise::halfToFloat(pos[2]);

					requests[0].data += requests[0].vertexBuffer->getBytesPerElement();
					vertices[i + subMeshOffset] = (orientation * (vec * scale)) + position;
				}
			}
			else if (requests[0].type == Ogre::VET_FLOAT3)
			{
				for (size_t i = 0; i < subMeshVerticiesNum; ++i)
				{
					const float* pos = reinterpret_cast<const float*>(requests[0].data);
					Ogre::Vector3 vec;
					vec.x = *pos++;
					vec.y = *pos++;
					vec.z = *pos++;
					requests[0].data += requests[0].vertexBuffer->getBytesPerElement();
					vertices[i + subMeshOffset] = (orientation * (vec * scale)) + position;
				}
			}
			else
			{
				throw Ogre::Exception(0, "Vertex Buffer type not recognised", "getMeshInformation");
			}
			subMeshOffset += subMeshVerticiesNum;
			vao->unmapAsyncTickets(requests);

			////Read index data
			if (indexBuffer)
			{
				Ogre::AsyncTicketPtr asyncTicket = indexBuffer->readRequest(0, indexBuffer->getNumElements());

				unsigned int* pIndices = 0;
				if (indices32)
				{
					pIndices = (unsigned*)(asyncTicket->map());
				}
				else
				{
					unsigned short *pShortIndices = (unsigned short*)(asyncTicket->map());
					pIndices = new unsigned int[indexBuffer->getNumElements()];
					for (size_t k = 0; k < indexBuffer->getNumElements(); k++)
					{
						pIndices[k] = static_cast<unsigned int>(pShortIndices[k]);
					}
				}
				unsigned int bufferIndex = 0;

				for (size_t i = addedIndices; i < addedIndices + indexBuffer->getNumElements(); i++)
				{
					indices[i] = pIndices[bufferIndex] + index_offset;
					bufferIndex++;
				}
				addedIndices += static_cast<unsigned int>(indexBuffer->getNumElements());

				if (!indices32)
					delete[] pIndices;

				asyncTicket->unmap();
			}
			index_offset += static_cast<unsigned int>(vertexBuffers[0]->getNumElements());
		}
		subMeshIterator++;
	}
}
#else
void InputGeom::getMeshInformation2(const Ogre::MeshPtr mesh, size_t& vertexCount, Ogre::Vector3*& vertices, size_t& indexCount, unsigned long*& indices, bool isPlanet,
	const Ogre::Vector3& position, const Ogre::Quaternion& orientation, const Ogre::Vector3& scale)
{
	unsigned int numVertices = 0;
	unsigned int numIndices = 0;

	Ogre::Aabb bbox = mesh->getAabb();
	Ogre::Vector3 center = bbox.mCenter;
	float radius = bbox.getRadius();

	for (const auto& subMesh : mesh->getSubMeshes())
	{
		numVertices += static_cast<unsigned int>(subMesh->mVao[0][0]->getVertexBuffers()[0]->getNumElements());
		numIndices += static_cast<unsigned int>(subMesh->mVao[0][0]->getIndexBuffer()->getNumElements());
	}

	vertices = OGRE_ALLOC_T(Ogre::Vector3, numVertices, Ogre::MEMCATEGORY_GEOMETRY);
	indices = OGRE_ALLOC_T(unsigned long, numIndices, Ogre::MEMCATEGORY_GEOMETRY);

	vertexCount = numVertices;
	indexCount = numIndices;

	unsigned int addedVertices = 0;
	unsigned int addedIndices = 0;
	unsigned int subMeshOffset = 0;

	for (const auto& subMesh : mesh->getSubMeshes())
	{
		Ogre::VertexArrayObject* vao = subMesh->mVao[0][0];
		bool indices32 = (vao->getIndexBuffer()->getIndexType() == Ogre::IndexBufferPacked::IT_32BIT);

		Ogre::VertexArrayObject::ReadRequestsVec requests;
		requests.push_back(Ogre::VertexArrayObject::ReadRequests(Ogre::VES_POSITION));

		vao->readRequests(requests);
		vao->mapAsyncTickets(requests);
		unsigned int subMeshVerticesNum = static_cast<unsigned int>(requests[0].vertexBuffer->getNumElements());

		if (requests[0].type == Ogre::VET_HALF4)
		{
			for (size_t i = 0; i < subMeshVerticesNum; ++i)
			{
				const Ogre::uint16* pos = reinterpret_cast<const Ogre::uint16*>(requests[0].data);
				Ogre::Vector3 vec;
				vec.x = Ogre::Bitwise::halfToFloat(pos[0]);
				vec.y = Ogre::Bitwise::halfToFloat(pos[1]);
				vec.z = Ogre::Bitwise::halfToFloat(pos[2]);

				requests[0].data += requests[0].vertexBuffer->getBytesPerElement();
				if (isPlanet)
				{
					Ogre::Vector3 dir = (vec - center).normalisedCopy();
					vec = center + dir * radius;
				}
				vertices[i + subMeshOffset] = (orientation * (vec * scale)) + position;
			}
		}
		else if (requests[0].type == Ogre::VET_FLOAT3)
		{
			for (size_t i = 0; i < subMeshVerticesNum; ++i)
			{
				const float* pos = reinterpret_cast<const float*>(requests[0].data);
				Ogre::Vector3 vec;
				vec.x = *pos++;
				vec.y = *pos++;
				vec.z = *pos++;
				requests[0].data += requests[0].vertexBuffer->getBytesPerElement();
				if (isPlanet)
				{
					Ogre::Vector3 dir = (vec - center).normalisedCopy();
					vec = center + dir * radius;
				}
				vertices[i + subMeshOffset] = (orientation * (vec * scale)) + position;
			}
		}
		else
		{
			throw Ogre::Exception(0, "Vertex Buffer type not recognised", "getMeshInformation2");
		}
		subMeshOffset += subMeshVerticesNum;
		vao->unmapAsyncTickets(requests);

		Ogre::IndexBufferPacked* indexBuffer = vao->getIndexBuffer();
		if (indexBuffer)
		{
			Ogre::AsyncTicketPtr asyncTicket = indexBuffer->readRequest(0, indexBuffer->getNumElements());
			unsigned int* pIndices = nullptr;

			if (indices32)
			{
				pIndices = (unsigned int*)asyncTicket->map();
			}
			else
			{
				unsigned short* pShortIndices = (unsigned short*)asyncTicket->map();
				pIndices = new unsigned int[indexBuffer->getNumElements()];
				for (size_t k = 0; k < indexBuffer->getNumElements(); k++)
				{
					pIndices[k] = static_cast<unsigned int>(pShortIndices[k]);
				}
			}

			for (size_t i = 0; i < indexBuffer->getNumElements(); i++)
			{
				indices[addedIndices++] = pIndices[i] + subMeshOffset;
			}

			if (!indices32)
				delete[] pIndices;

			asyncTicket->unmap();
		}
	}
}
#endif

void InputGeom::getManualMeshInformation(const Ogre::ManualObject* manual,
	size_t& vertex_count,
	Ogre::Vector3*& vertices,
	size_t& index_count,
	unsigned long*& indices,
	const Ogre::Vector3& position,
	const Ogre::Quaternion& orient,
	const Ogre::Vector3& scale)
{
	std::vector<Ogre::Vector3> returnVertices;
	std::vector<unsigned long> returnIndices;
	unsigned long thisSectionStart = 0;
	for (size_t i = 0; i < manual->getNumSections(); i++)
	{
		Ogre::ManualObject::ManualObjectSection* section = manual->getSection(i);
		Ogre::v1::RenderOperation* renderOp = nullptr;
		section->getRenderOperation(*renderOp, false);

		std::vector<Ogre::Vector3> pushVertices;
		//Collect the vertices
		{
			const Ogre::v1::VertexElement* vertexElement = renderOp->vertexData->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);
			Ogre::v1::HardwareVertexBufferSharedPtr vertexBuffer = renderOp->vertexData->vertexBufferBinding->getBuffer(vertexElement->getSource());

			char* verticesBuffer = (char*)vertexBuffer->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY);
			float* positionArrayHolder;

			thisSectionStart = pushVertices.size();

			pushVertices.reserve(renderOp->vertexData->vertexCount);

			for (unsigned int j = 0; j < renderOp->vertexData->vertexCount; j++)
			{
				vertexElement->baseVertexPointerToElement(verticesBuffer + j * vertexBuffer->getVertexSize(), &positionArrayHolder);
				Ogre::Vector3 vertexPos = Ogre::Vector3(positionArrayHolder[0],
					positionArrayHolder[1],
					positionArrayHolder[2]);

				vertexPos = (orient * (vertexPos * scale)) + position;

				pushVertices.push_back(vertexPos);
			}

			vertexBuffer->unlock();
		}
		//Collect the indices
		{
			if (renderOp->useIndexes)
			{
				Ogre::v1::HardwareIndexBufferSharedPtr indexBuffer = renderOp->indexData->indexBuffer;

				if (indexBuffer.isNull() || renderOp->operationType != Ogre::OperationType::OT_TRIANGLE_LIST)
				{
					//No triangles here, so we just drop the collected vertices and move along to the next section.
					continue;
				}
				else
				{
					returnVertices.reserve(returnVertices.size() + pushVertices.size());
					returnVertices.insert(returnVertices.end(), pushVertices.begin(), pushVertices.end());
				}

				unsigned int* pLong = (unsigned int*)indexBuffer->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY);
				unsigned short* pShort = (unsigned short*)pLong;

				returnIndices.reserve(returnIndices.size() + renderOp->indexData->indexCount);

				for (size_t j = 0; j < renderOp->indexData->indexCount; j++)
				{
					unsigned long index;
					//We also have got to remember that for a multi section object, each section has
					//different vertices, so the indices will not be correct. To correct this, we
					//have to add the position of the first vertex in this section to the index

					//(At least I think so...)
					if (indexBuffer->getType() == Ogre::v1::HardwareIndexBuffer::IT_32BIT)
						index = (unsigned long)pLong[j] + thisSectionStart;
					else
						index = (unsigned long)pShort[j] + thisSectionStart;

					returnIndices.push_back(index);
				}

				indexBuffer->unlock();
			}
		}
	}

	//Now we simply return the data.
	index_count = returnIndices.size();
	vertex_count = returnVertices.size();
	vertices = new Ogre::Vector3[vertex_count];
	for (unsigned long i = 0; i < vertex_count; i++)
	{
		vertices[i] = returnVertices[i];
	}
	indices = new unsigned long[index_count];
	for (unsigned long i = 0; i < index_count; i++)
	{
		indices[i] = returnIndices[i];
	}

	//All done.
	return;
}

void InputGeom::addOffMeshConnection(const float* spos, const float* epos, const float rad,
	unsigned char bidir, unsigned char area, unsigned short flags)
{
	if (m_offMeshConCount >= MAX_OFFMESH_CONNECTIONS) return;
	float* v = &m_offMeshConVerts[m_offMeshConCount * 3 * 2];
	m_offMeshConRads[m_offMeshConCount] = rad;
	m_offMeshConDirs[m_offMeshConCount] = bidir;
	m_offMeshConAreas[m_offMeshConCount] = area;
	m_offMeshConFlags[m_offMeshConCount] = flags;
	m_offMeshConId[m_offMeshConCount] = 1000 + m_offMeshConCount;
	rcVcopy(&v[0], spos);
	rcVcopy(&v[3], epos);
	m_offMeshConCount++;
}

void InputGeom::deleteOffMeshConnection(int i)
{
	m_offMeshConCount--;
	float* src = &m_offMeshConVerts[m_offMeshConCount * 3 * 2];
	float* dst = &m_offMeshConVerts[i * 3 * 2];
	rcVcopy(&dst[0], &src[0]);
	rcVcopy(&dst[3], &src[3]);
	m_offMeshConRads[i] = m_offMeshConRads[m_offMeshConCount];
	m_offMeshConDirs[i] = m_offMeshConDirs[m_offMeshConCount];
	m_offMeshConAreas[i] = m_offMeshConAreas[m_offMeshConCount];
	m_offMeshConFlags[i] = m_offMeshConFlags[m_offMeshConCount];
}


ConvexVolume* InputGeom::getConvexHull(Ogre::Real offset)
{
	// TODO who manages created convexVolume objects, and maybe better return pointer
	return new ConvexVolume(this, offset);
}

int InputGeom::getConvexVolumeId(ConvexVolume* convexHull)
{
	for (int i = 0; i < m_volumeCount; i++)
	{
		if (m_volumes[i] == convexHull)
			return i;
	}

	return -1;
}

int InputGeom::addConvexVolume(ConvexVolume* vol)
{
	// The maximum number of convex volumes that can be added to the navmesh equals the max amount
	// of volumes that can be added to the inputGeom it is built from.
	if (m_volumeCount >= InputGeom::MAX_VOLUMES)
		return -1;

	m_volumes[m_volumeCount] = vol;
	m_volumeCount++;

	return m_volumeCount - 1; // Return index of created volume
}

bool InputGeom::deleteConvexVolume(int i, ConvexVolume** removedVolume)
{
	if (i >= m_volumeCount || i < 0)
		return false;


	*removedVolume = m_volumes[i];
	m_volumeCount--;
	m_volumes[i] = m_volumes[m_volumeCount];

	return true;
}

ConvexVolume* InputGeom::getConvexVolume(int volIndex)
{
	if (volIndex < 0 || volIndex > m_volumeCount)
		return NULL;

	return m_volumes[volIndex];
}


static bool isectSegAABB(const float* sp, const float* sq,
	const float* amin, const float* amax,
	float& tmin, float& tmax)
{
	static const float EPS = 1e-6f;

	float d[3];
	d[0] = sq[0] - sp[0];
	d[1] = sq[1] - sp[1];
	d[2] = sq[2] - sp[2];
	tmin = 0.0;
	tmax = 1.0f;

	for (int i = 0; i < 3; i++)
	{
		if (fabsf(d[i]) < EPS)
		{
			if (sp[i] < amin[i] || sp[i] > amax[i])
				return false;
		}
		else
		{
			const float ood = 1.0f / d[i];
			float t1 = (amin[i] - sp[i]) * ood;
			float t2 = (amax[i] - sp[i]) * ood;
			if (t1 > t2)
			{
				float tmp = t1; t1 = t2; t2 = tmp;
			}
			if (t1 > tmin) tmin = t1;
			if (t2 < tmax) tmax = t2;
			if (tmin > tmax) return false;
		}
	}

	return true;
}

int InputGeom::hitTestConvexVolume(const float* sp, const float* sq)
{
	float tmin = FLT_MAX;
	int volMinIdx = -1;
	for (int i = 0; i < m_volumeCount; ++i)
	{
		const ConvexVolume* vol = m_volumes[i];

		float t0, t1;

		if (isectSegAABB(sp, sq, vol->bmin, vol->bmax, t0, t1))
		{
			if (t0 < tmin)
			{
				tmin = t0;
				volMinIdx = i;
			}
		}
	}
	return volMinIdx;
}


static bool intersectSegmentTriangle(const float* sp, const float* sq,
	const float* a, const float* b, const float* c,
	float& t)
{
	float v, w;
	float ab[3], ac[3], qp[3], ap[3], norm[3], e[3];
	rcVsub(ab, b, a);
	rcVsub(ac, c, a);
	rcVsub(qp, sp, sq);

	// Compute triangle normal. Can be precalculated or cached if
	// intersecting multiple segments against the same triangle
	rcVcross(norm, ab, ac);

	// Compute denominator d. If d <= 0, segment is parallel to or points
	// away from triangle, so exit early
	float d = rcVdot(qp, norm);
	if (d <= 0.0f) return false;

	// Compute intersection t value of pq with plane of triangle. A ray
	// intersects iff 0 <= t. Segment intersects iff 0 <= t <= 1. Delay
	// dividing by d until intersection has been found to pierce triangle
	rcVsub(ap, sp, a);
	t = rcVdot(ap, norm);
	if (t < 0.0f) return false;
	if (t > d) return false; // For segment; exclude this code line for a ray test

	// Compute barycentric coordinate components and test if within bounds
	rcVcross(e, qp, ap);
	v = rcVdot(ac, e);
	if (v < 0.0f || v > d) return false;
	w = -rcVdot(ab, e);
	if (w < 0.0f || v + w > d) return false;

	// Segment/ray intersects triangle. Perform delayed division
	t /= d;

	return true;
}

bool InputGeom::raycastMesh(float* src, float* dst, float& tmin)
{
	float dir[3];
	rcVsub(dir, dst, src);

	// Prune hit ray.
	float btmin, btmax;
	if (!isectSegAABB(src, dst, getMeshBoundsMin(), getMeshBoundsMax(), btmin, btmax))
		return false;
	float p[2], q[2];
	p[0] = src[0] + (dst[0] - src[0]) * btmin;
	p[1] = src[2] + (dst[2] - src[2]) * btmin;
	q[0] = src[0] + (dst[0] - src[0]) * btmax;
	q[1] = src[2] + (dst[2] - src[2]) * btmax;

	int cid[512];
	const int ncid = rcGetChunksOverlappingSegment(m_chunkyMesh, p, q, cid, 512);
	if (!ncid)
		return false;

	tmin = 1.0f;
	bool hit = false;
	const float* verts = getVerts();

	for (int i = 0; i < ncid; ++i)
	{
		const rcChunkyTriMeshNode& node = m_chunkyMesh->nodes[cid[i]];
		const int* tris = &m_chunkyMesh->tris[node.i * 3];
		const int ntris = node.n;

		for (int j = 0; j < ntris * 3; j += 3)
		{
			float t = 1;
			if (intersectSegmentTriangle(src, dst,
				&verts[tris[j] * 3],
				&verts[tris[j + 1] * 3],
				&verts[tris[j + 2] * 3], t))
			{
				if (t < tmin)
					tmin = t;
				hit = true;
			}
		}
	}

	return hit;
}




void InputGeom::debugMesh(Ogre::SceneManager* sceneMgr)
{
	// Debug navmesh points
	Ogre::ManualObject* manual = sceneMgr->createManualObject();
	manual->setRenderQueueGroup(10);
	manual->setName("InputGeomDebug");

	// Draw inputGeom as triangles
	manual->begin("recastdebug", Ogre::OperationType::OT_TRIANGLE_LIST);
	for (int i = 0; i < ntris * 3; i++)
	{
		int triIdx = tris[i];
		manual->position(verts[3 * triIdx], verts[3 * triIdx + 1], verts[3 * triIdx + 2]);
	}

	/*
	// Alternative drawing method: draw only the vertices
	manual->begin("recastdebug",Ogre::OperationType::OT_TRIANGLE_LIST);
	for (int i = 0; i<nverts*3; i+=3) {
		manual->position(verts[i], verts[i+1], verts[i+2]);
	}
	*/

	manual->end();
	sceneMgr->getRootSceneNode()->createChildSceneNode()->attachObject(manual);
}













// ChunkyTriMesh

struct __declspec(dllexport) BoundsItem
{
	float bmin[2];
	float bmax[2];
	int i;
};

static int compareItemX(const void* va, const void* vb)
{
	const BoundsItem* a = (const BoundsItem*)va;
	const BoundsItem* b = (const BoundsItem*)vb;
	if (a->bmin[0] < b->bmin[0])
		return -1;
	if (a->bmin[0] > b->bmin[0])
		return 1;
	return 0;
}

static int compareItemY(const void* va, const void* vb)
{
	const BoundsItem* a = (const BoundsItem*)va;
	const BoundsItem* b = (const BoundsItem*)vb;
	if (a->bmin[1] < b->bmin[1])
		return -1;
	if (a->bmin[1] > b->bmin[1])
		return 1;
	return 0;
}

static void calcExtends(const BoundsItem* items, const int /*nitems*/,
	const int imin, const int imax,
	float* bmin, float* bmax)
{
	bmin[0] = items[imin].bmin[0];
	bmin[1] = items[imin].bmin[1];

	bmax[0] = items[imin].bmax[0];
	bmax[1] = items[imin].bmax[1];

	for (int i = imin + 1; i < imax; ++i)
	{
		const BoundsItem& it = items[i];
		if (it.bmin[0] < bmin[0]) bmin[0] = it.bmin[0];
		if (it.bmin[1] < bmin[1]) bmin[1] = it.bmin[1];

		if (it.bmax[0] > bmax[0]) bmax[0] = it.bmax[0];
		if (it.bmax[1] > bmax[1]) bmax[1] = it.bmax[1];
	}
}

inline int longestAxis(float x, float y)
{
	return y > x ? 1 : 0;
}

static void subdivide(BoundsItem* items, int nitems, int imin, int imax, int trisPerChunk,
	int& curNode, rcChunkyTriMeshNode* nodes, const int maxNodes,
	int& curTri, int* outTris, const int* inTris)
{
	int inum = imax - imin;
	int icur = curNode;

	if (curNode > maxNodes)
		return;

	rcChunkyTriMeshNode& node = nodes[curNode++];

	if (inum <= trisPerChunk)
	{
		// Leaf
		calcExtends(items, nitems, imin, imax, node.bmin, node.bmax);

		// Copy triangles.
		node.i = curTri;
		node.n = inum;

		for (int i = imin; i < imax; ++i)
		{
			const int* src = &inTris[items[i].i * 3];
			int* dst = &outTris[curTri * 3];
			curTri++;
			dst[0] = src[0];
			dst[1] = src[1];
			dst[2] = src[2];
		}
	}
	else
	{
		// Split
		calcExtends(items, nitems, imin, imax, node.bmin, node.bmax);

		int	axis = longestAxis(node.bmax[0] - node.bmin[0],
			node.bmax[1] - node.bmin[1]);

		if (axis == 0)
		{
			// Sort along x-axis
			qsort(items + imin, inum, sizeof(BoundsItem), compareItemX);
		}
		else if (axis == 1)
		{
			// Sort along y-axis
			qsort(items + imin, inum, sizeof(BoundsItem), compareItemY);
		}

		int isplit = imin + inum / 2;

		// Left
		subdivide(items, nitems, imin, isplit, trisPerChunk, curNode, nodes, maxNodes, curTri, outTris, inTris);
		// Right
		subdivide(items, nitems, isplit, imax, trisPerChunk, curNode, nodes, maxNodes, curTri, outTris, inTris);

		int iescape = curNode - icur;
		// Negative index means escape.
		node.i = -iescape;
	}
}

bool rcCreateChunkyTriMesh(const float* verts, const int* tris, int ntris,
	int trisPerChunk, rcChunkyTriMesh* cm)
{
	int nchunks = (ntris + trisPerChunk - 1) / trisPerChunk;

	cm->nodes = new rcChunkyTriMeshNode[nchunks * 4];
	if (!cm->nodes)
		return false;

	cm->tris = new int[ntris * 3];
	if (!cm->tris)
		return false;

	cm->ntris = ntris;

	// Build tree
	BoundsItem* items = new BoundsItem[ntris];
	if (!items)
		return false;

	for (int i = 0; i < ntris; i++)
	{
		const int* t = &tris[i * 3];
		BoundsItem& it = items[i];
		it.i = i;
		// Calc triangle XZ bounds.
		it.bmin[0] = it.bmax[0] = verts[t[0] * 3 + 0];
		it.bmin[1] = it.bmax[1] = verts[t[0] * 3 + 2];
		for (int j = 1; j < 3; ++j)
		{
			const float* v = &verts[t[j] * 3];
			if (v[0] < it.bmin[0]) it.bmin[0] = v[0];
			if (v[2] < it.bmin[1]) it.bmin[1] = v[2];

			if (v[0] > it.bmax[0]) it.bmax[0] = v[0];
			if (v[2] > it.bmax[1]) it.bmax[1] = v[2];
		}
	}

	int curTri = 0;
	int curNode = 0;
	subdivide(items, ntris, 0, ntris, trisPerChunk, curNode, cm->nodes, nchunks * 4, curTri, cm->tris, tris);

	delete[] items;

	cm->nnodes = curNode;

	// Calc max tris per node.
	cm->maxTrisPerChunk = 0;
	for (int i = 0; i < cm->nnodes; ++i)
	{
		rcChunkyTriMeshNode& node = cm->nodes[i];
		const bool isLeaf = node.i >= 0;
		if (!isLeaf) continue;
		if (node.n > cm->maxTrisPerChunk)
			cm->maxTrisPerChunk = node.n;
	}

	return true;
}


inline bool checkOverlapRect(const float amin[2], const float amax[2],
	const float bmin[2], const float bmax[2])
{
	bool overlap = true;
	overlap = (amin[0] > bmax[0] || amax[0] < bmin[0]) ? false : overlap;
	overlap = (amin[1] > bmax[1] || amax[1] < bmin[1]) ? false : overlap;
	return overlap;
}

int rcGetChunksOverlappingRect(const rcChunkyTriMesh* cm,
	float bmin[2], float bmax[2],
	int* ids, const int maxIds)
{
	// Traverse tree
	int i = 0;
	int n = 0;
	while (i < cm->nnodes)
	{
		const rcChunkyTriMeshNode* node = &cm->nodes[i];
		const bool overlap = checkOverlapRect(bmin, bmax, node->bmin, node->bmax);
		const bool isLeafNode = node->i >= 0;

		if (isLeafNode && overlap)
		{
			if (n < maxIds)
			{
				ids[n] = i;
				n++;
			}
		}

		if (overlap || isLeafNode)
			i++;
		else
		{
			const int escapeIndex = -node->i;
			i += escapeIndex;
		}
	}

	return n;
}



static bool checkOverlapSegment(const float p[2], const float q[2],
	const float bmin[2], const float bmax[2])
{
	static const float EPSILON = 1e-6f;

	float tmin = 0;
	float tmax = 1;
	float d[2];
	d[0] = q[0] - p[0];
	d[1] = q[1] - p[1];

	for (int i = 0; i < 2; i++)
	{
		if (fabsf(d[i]) < EPSILON)
		{
			// Ray is parallel to slab. No hit if origin not within slab
			if (p[i] < bmin[i] || p[i] > bmax[i])
				return false;
		}
		else
		{
			// Compute intersection t value of ray with near and far plane of slab
			float ood = 1.0f / d[i];
			float t1 = (bmin[i] - p[i]) * ood;
			float t2 = (bmax[i] - p[i]) * ood;
			if (t1 > t2)
			{
				float tmp = t1; t1 = t2; t2 = tmp;
			}
			if (t1 > tmin) tmin = t1;
			if (t2 < tmax) tmax = t2;
			if (tmin > tmax) return false;
		}
	}
	return true;
}

int rcGetChunksOverlappingSegment(const rcChunkyTriMesh* cm,
	float p[2], float q[2],
	int* ids, const int maxIds)
{
	// Traverse tree
	int i = 0;
	int n = 0;
	while (i < cm->nnodes)
	{
		const rcChunkyTriMeshNode* node = &cm->nodes[i];
		const bool overlap = checkOverlapSegment(p, q, node->bmin, node->bmax);
		const bool isLeafNode = node->i >= 0;

		if (isLeafNode && overlap)
		{
			if (n < maxIds)
			{
				ids[n] = i;
				n++;
			}
		}

		if (overlap || isLeafNode)
			i++;
		else
		{
			const int escapeIndex = -node->i;
			i += escapeIndex;
		}
	}

	return n;
}


Ogre::ManualObject* InputGeom::drawConvexVolume(ConvexVolume* vol, Ogre::SceneManager* sceneMgr, Ogre::ColourValue color)
{
	// Define manualObject with convex volume faces
	Ogre::ManualObject* manual = sceneMgr->createManualObject();
	manual->setRenderQueueGroup(10);
	// Set material
	manual->begin("recastdebug", Ogre::OperationType::OT_LINE_LIST);

	for (int i = 0, j = vol->nverts - 1; i < vol->nverts; j = i++)
	{
		const float* vi = &vol->verts[j * 3];
		const float* vj = &vol->verts[i * 3];

		manual->position(vj[0], vol->hmin, vj[2]);  manual->colour(color);
		manual->position(vi[0], vol->hmin, vi[2]);  manual->colour(color);
		manual->position(vj[0], vol->hmax, vj[2]);  manual->colour(color);
		manual->position(vi[0], vol->hmax, vi[2]);  manual->colour(color);
		manual->position(vj[0], vol->hmin, vj[2]);  manual->colour(color);
		manual->position(vj[0], vol->hmax, vj[2]);  manual->colour(color);
	}
	/*
		// Create triangles
		for (int i = 0; i < vol->nverts-1; ++i) {   // Number of quads (is number of tris -1)
			// We ignore the y component and just draw a box between min height and max height

			// Triangle 1 of the quad
			// Bottom vertex
			manual->position(vol->verts[3*i], vol->hmin,vol->verts[3*i+2]);
			manual->colour(color);

			// Top vertex
			manual->position(vol->verts[3*i], vol->hmax,vol->verts[3*i+2]);
			manual->colour(color);

			// Next bottom vertex
			manual->position(vol->verts[3*i+3], vol->hmin,vol->verts[3*i+5]);
			manual->colour(color);


			// Triangle 2 of the quad
			// Bottom vertex
			manual->position(vol->verts[3*i], vol->hmin,vol->verts[3*i+2]);
			manual->colour(color);

			// Top vertex
			manual->position(vol->verts[3*i], vol->hmax,vol->verts[3*i+2]);
			manual->colour(color);

			// Next Top vertex
			manual->position(vol->verts[3*i+3], vol->hmax,vol->verts[3*i+5]);
			manual->colour(color);
		}
	*/

	manual->end();
	sceneMgr->getRootSceneNode()->attachObject(manual); // Just attach it to root scenenode since its coordinates are in world-space
	return manual;
}


Ogre::AxisAlignedBox InputGeom::getWorldSpaceBoundingBox(Ogre::v1::Entity* ent)
{
	Ogre::SceneManager* sceneMgr = ent->getParentSceneNode()->getCreator();
	Ogre::Matrix4 transform = sceneMgr->getRootSceneNode()->_getFullTransform().inverse() * ent->getParentSceneNode()->_getFullTransform();
	Ogre::AxisAlignedBox bb = ent->getMesh()->getBounds();
	bb.transform(transform);

	return bb;
}


Ogre::AxisAlignedBox InputGeom::getBoundingBox()
{
	Ogre::AxisAlignedBox bb;
	Ogre::Vector3 max;
	Ogre::Vector3 min;
	OgreRecast::FloatAToOgreVect3(bmin, min);
	OgreRecast::FloatAToOgreVect3(bmax, max);
	bb.setMaximum(max);
	bb.setMinimum(min);

	return bb;
}


void InputGeom::writeObj(Ogre::String filename)
{
	std::fstream fstr(filename.c_str(), std::ios::out);
	//    Ogre::DataStreamPtr stream(OGRE_NEW Ogre::FileStreamDataStream(&fstr, false));
	//    Ogre::StreamSerialiser streamWriter(stream);

	for (int i = 0; i < nverts; i++)
	{
		Ogre::String line = "v " + Ogre::StringConverter::toString(verts[3 * i]) + " " + Ogre::StringConverter::toString(verts[3 * i + 1]) + " " + Ogre::StringConverter::toString(verts[3 * i + 2]);
		fstr << line << std::endl;
		//        streamWriter.write(&line);
	}
	for (int i = 0; i < ntris; i++)
	{
		Ogre::String line = "f " + Ogre::StringConverter::toString(1 + tris[3 * i]) + " " + Ogre::StringConverter::toString(1 + tris[3 * i + 1]) + " " + Ogre::StringConverter::toString(1 + tris[3 * i + 2]);
		fstr << line << std::endl;
	}

	fstr.close();
}



void InputGeom::applyOrientation(Ogre::Quaternion orientation, Ogre::Vector3 pivot)
{
	// TODO allow this or not?
		/*
		if(mTerrainGroup)
			return; // It makes no sense to do this if this inputGeom contains terrain!
		*/


		// Apply transformation to all verts
	Ogre::Matrix4 transform = Ogre::Matrix4(orientation); // Convert quaternion into regular transformation matrix
	Ogre::Vector3 vert;
	for (int i = 0; i < nverts; i++)
	{
		// Obtain vertex (translated towards pivot point)
		vert.x = verts[3 * i + 0] - pivot.x;
		vert.y = verts[3 * i + 1] - pivot.y;
		vert.z = verts[3 * i + 2] - pivot.z;

		// Apply rotation to vector
		vert = transform * vert;

		// Store the rotated vector and add translation away from pivot point again
		verts[3 * i + 0] = vert.x + pivot.x;
		verts[3 * i + 1] = vert.y + pivot.y;
		verts[3 * i + 2] = vert.z + pivot.z;
	}


	// Transform extents
	// Abandoned for now: this means bounding boxes are not rotated any that you best
	// rotate only pretty symmetrical shapes
	// If this does not suffice you could use the bounding box or hull from the physics engine
	// Or fully recalculate the bounding box from all vertices
}

void InputGeom::move(Ogre::Vector3 translation)
{
	// Apply translation to all verts
	for (int i = 0; i < nverts; i++)
	{
		verts[3 * i + 0] += translation.x;
		verts[3 * i + 1] += translation.y;
		verts[3 * i + 2] += translation.z;
	}


	// Transform extents
	bmin[0] += translation.x;
	bmin[1] += translation.y;
	bmin[2] += translation.z;

	bmax[0] += translation.x;
	bmax[1] += translation.y;
	bmax[2] += translation.z;
}


Ogre::ManualObject* InputGeom::drawBoundingBox(Ogre::AxisAlignedBox box, Ogre::SceneManager* sceneMgr, Ogre::ColourValue color)
{
	ConvexVolume* cv = new ConvexVolume(box);
	Ogre::ManualObject* result = InputGeom::drawConvexVolume(cv, sceneMgr, color);
	delete cv;

	return result;
}

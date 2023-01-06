/*
 * GeometricTools.h
 *
 *  Created on: Apr 29, 2011
 *      Author: _1nsane
 */

#ifndef GEOMETRICTOOLS_H_
#define GEOMETRICTOOLS_H_

#include <OgreMatrix3.h>
#include <OgreVector3.h>
#include <OgreVector2.h>
#include <vector>
#include <list>

struct SCutResult
{
	int iEdgeID;//id of the edge [0:2] on first tri, [3:5] on second
	Ogre::Vector3 vPos;//position of the cutting point in 3D space
	float fRelCoord;//relative coordinate of the cutting point on the cut edge
	Ogre::Vector2 vUV;
};

class GeometricTools
{
public:
	struct OctreeNode
	{
		Ogre::Vector3 vMin, vMax;
		OctreeNode* pChildren[8];
		int iTriIndex;
	};
	/* get cutting points of two tris, if any
	 * @return edge the cut appears on and cut position. edges are indexed by v1_01,v1_12,v1_20, v2_01,v2_12,v2_20,
	 */
	static std::vector<SCutResult> CutTriangles(const std::vector<Ogre::Vector3>& vVertices1, const std::vector<Ogre::Vector3>& vVertices2);
	/*
	 * generate an octree of the triangles
	 * @return octree root node
	 */
	static OctreeNode* GenerateOctree(std::vector<Ogre::Vector3> vVertices, std::vector<unsigned int> viIndices);
	/*
	 * check two octrees for intersecting tri bounding boxes
	 * @return list of possibly intersecting tri indices
	 */
	static std::list<std::pair<int, int> > CheckForIntersections(OctreeNode* octree1, OctreeNode* octree2);
	/*
	 * returns uv coordinates of vPoint from the coordinate system with unit vectors vEdge1, vEdge2 (coordinate transformation)
	 */
	static Ogre::Vector2 getUVCoords(const Ogre::Vector3& vEdge1, const Ogre::Vector3& vEdge2, const Ogre::Vector3& vPoint);
};

#endif /* GEOMETRICTOOLS_H_ */

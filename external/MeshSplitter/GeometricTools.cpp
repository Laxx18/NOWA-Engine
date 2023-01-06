/*
 * GeometricTools.cpp
 *
 *  Created on: Apr 29, 2011
 *      Author: _1nsane
 */

#include "GeometricTools.h"

//return cutting points of two tris: vector and id for edge
std::vector<SCutResult>
GeometricTools::CutTriangles(const std::vector<Ogre::Vector3>& vVertices1, const std::vector<Ogre::Vector3>& vVertices2)
{
	//test all edges of one tri to those of the other
	std::vector<SCutResult> vCuts;

	Ogre::Vector3 aavEdges[2][3];
	Ogre::Vector3 aavPlaneVectors[2][2];

	for(int iEdge=0; iEdge<3; iEdge++)
	{
		aavEdges[0][iEdge]=vVertices1[(iEdge+1)%3]-vVertices1[iEdge];
		aavEdges[1][iEdge]=vVertices2[(iEdge+1)%3]-vVertices2[iEdge];

		if(iEdge==2)
			break;
		aavPlaneVectors[0][iEdge]=vVertices1[iEdge+1]-vVertices1[0];
		aavPlaneVectors[1][iEdge]=vVertices2[iEdge+1]-vVertices2[0];
	}

	for(int iTri=0; iTri<2; iTri++)
	{
		const std::vector<Ogre::Vector3> &otherTri=iTri ? vVertices1 : vVertices2;
		const std::vector<Ogre::Vector3> &thisTri=iTri ? vVertices2 : vVertices1;

		int iOtherTri=(~iTri)&1;
		//calc cutting point of our edges with the foreign plane given in normal form

		for(int iEdge=0; iEdge<3; iEdge++)
		{
			//taken from wikipedia ;)
			Ogre::Vector3 a=aavEdges[iTri][iEdge]*(-1),
					b=aavPlaneVectors[iOtherTri][0],
					c=aavPlaneVectors[iOtherTri][1],
					v=thisTri[iEdge]-otherTri[0];

			Ogre::Matrix3 mMatrix=Ogre::Matrix3(
					a.x, b.x, c.x,
					a.y, b.y, c.y,
					a.z, b.z, c.z),
					mInvMatrix;

			bool bSuccess=mMatrix.Inverse(mInvMatrix);
			if(!bSuccess)
				continue;

			Ogre::Vector3 vRes=mInvMatrix*v;
			//check if the intersection is inside the triangle
			float eps=Ogre::Matrix3::EPSILON;
			if(vRes.y>-eps && vRes.z>-eps && (vRes.y+vRes.z)<1+eps &&
					vRes.x>-eps && vRes.x<1+eps)
			{
				Ogre::Vector3 vIntersection=//otherTri[0]+
						//aavPlaneVectors[iOtherTri][0]*vRes.y+aavPlaneVectors[iOtherTri][1]*vRes.z;
						thisTri[iEdge]+aavEdges[iTri][iEdge]*vRes.x;
				SCutResult res;
				res.fRelCoord=vRes.x;
				res.iEdgeID=iEdge+iTri*3;
				res.vPos=vIntersection;
				res.vUV=Ogre::Vector2(vRes.y, vRes.z);
				vCuts.push_back(res);
			}
		}
	}

	return vCuts;
}

#define EIGHTH_SPACE_INDEX(v1,v2) (((v1.x>v2.x)?4:0)+((v1.y>v2.y)?2:0)+((v1.z>v2.z)?1:0))

#define CMP_MAX(min, max, cmp_min, cmp_max)\
		if(cmp_min.x<min.x) min.x=cmp_min.x;\
		if(cmp_min.y<min.y) min.y=cmp_min.y;\
		if(cmp_min.z<min.z) min.z=cmp_min.z;\
		if(cmp_max.x>max.x) max.x=cmp_max.x;\
		if(cmp_max.y>max.y) max.y=cmp_max.y;\
		if(cmp_max.z>max.z) max.z=cmp_max.z;

GeometricTools::OctreeNode*
BuildOctree(Ogre::Vector3 vMin, Ogre::Vector3 vMax, std::list<GeometricTools::OctreeNode*> lpLeaves)
{
	if(lpLeaves.size()==1)
		return lpLeaves.front();
	GeometricTools::OctreeNode* pNode=new GeometricTools::OctreeNode;
	pNode->vMax=vMax;
	pNode->vMin=vMin;
	Ogre::Vector3 vSplit((vMin+vMax)*.5f);
	std::list<GeometricTools::OctreeNode*> alpLeaves[8];
	Ogre::Vector3 avMin[8], avMax[8];

	int i = 0;
	for (;i < 8; ++i)
	{
		avMin[i]=Ogre::Vector3(1.0e+10f, 1.0e+10f, 1.0e+10f);
		avMax[i]=Ogre::Vector3(-1.0e+10f, -1.0e+10f, -1.0e+10f);
	}
	for (std::list<GeometricTools::OctreeNode*>::iterator it=lpLeaves.begin(); it!=lpLeaves.end(); it++)
	{
		int iEighthIndex=EIGHTH_SPACE_INDEX(((*it)->vMin+(*it)->vMax).operator *(.5f), vSplit);
		alpLeaves[iEighthIndex].push_back(*it);

		CMP_MAX(avMin[iEighthIndex], avMax[iEighthIndex], (*it)->vMin, (*it)->vMax)
	}
	unsigned int nOriginalLeaves=lpLeaves.size();
	lpLeaves.clear();
	for (i=0; i < 8; ++i)
	{
		//if all leaves end up in the same sector an infinite recursion occurs.
		//to prevent this they will be split
		if(alpLeaves[i].size()==nOriginalLeaves)
		{
			for (unsigned int iLeaf = 0; iLeaf < (nOriginalLeaves>>1); iLeaf++)
			{
				GeometricTools::OctreeNode* node=alpLeaves[i].back();
				alpLeaves[(i+1)&7].push_back(node);
				CMP_MAX(avMin[(i+1)&7], avMax[(i+1)&7], node->vMin, node->vMax)
				alpLeaves[i].pop_back();
			}
		}
		//else simply recurse
		if(alpLeaves[i].size())
			pNode->pChildren[i]=BuildOctree(avMin[i], avMax[i], alpLeaves[i]);
		else
			pNode->pChildren[i]=NULL;
	}
	pNode->iTriIndex=-1;

	return pNode;
}

GeometricTools::OctreeNode*
GeometricTools::GenerateOctree(std::vector<Ogre::Vector3> vVertices, std::vector<unsigned int> viIndices)
{
	//generate list of leaves for the tree
	std::list<OctreeNode*> lpLeaves;
	Ogre::Vector3 vMeshMin(1.0e+10f, 1.0e+10f, 1.0e+10f), vMeshMax(-1.0e+10f, -1.0e+10f, -1.0e+10f);
	for (unsigned int iTri = 0; iTri < viIndices.size()/3; ++iTri)
	{
		Ogre::Vector3 vMin(1.0e+10f, 1.0e+10f, 1.0e+10f), vMax(-1.0e+10f, -1.0e+10f, -1.0e+10f);
		for(int iVert=0; iVert<3; iVert++)
		{
			Ogre::Vector3 vPos=vVertices[viIndices[iTri*3+iVert]];
			CMP_MAX(vMin, vMax, vPos, vPos)
		}
		OctreeNode* p=new OctreeNode;
		lpLeaves.push_back(p);
		p->vMin=vMin;
		p->vMax=vMax;
		p->iTriIndex=iTri;
		CMP_MAX(vMeshMin, vMeshMax, vMin, vMax)
	}
	//recurse to build the octree
	return BuildOctree(vMeshMin, vMeshMax, lpLeaves);
}

#define BBS_INTERSECT_1DIM(min1, max1, min2, max2)\
	((min1<=min2 && min2<=max1) || (min2<=min1 && min1<=max2))
#define BBS_INTERSECT(min1, max1, min2, max2)\
	(BBS_INTERSECT_1DIM(min1.x, max1.x, min2.x, max2.x) &&\
	BBS_INTERSECT_1DIM(min1.y, max1.y, min2.y, max2.y) &&\
	BBS_INTERSECT_1DIM(min1.z, max1.z, min2.z, max2.z))

std::list<std::pair<int, int> >
GeometricTools::CheckForIntersections(GeometricTools::OctreeNode* octree1, GeometricTools::OctreeNode* octree2)
{
	std::list<std::pair<int, int> > intersectingTris;
	if(BBS_INTERSECT(octree1->vMin, octree1->vMax, octree2->vMin, octree2->vMax))
	{
		bool bOct1IsLeaf=octree1->iTriIndex!=-1;
		bool bOct2IsLeaf=octree2->iTriIndex!=-1;
		if(bOct1IsLeaf && bOct2IsLeaf)
			//we're on the lowest level and these tri boxes collide!
			return std::list<std::pair<int, int> >(1, std::make_pair(octree1->iTriIndex, octree2->iTriIndex));
		int iUpperBound1=bOct1IsLeaf ? 1 : 8;
		int iUpperBound2=bOct2IsLeaf ? 1 : 8;
		for (int i = 0; i < iUpperBound1; ++i)
			for (int j = 0; j < iUpperBound2; ++j)
			{
				OctreeNode* pNode1=bOct1IsLeaf ? octree1 : octree1->pChildren[i];
				OctreeNode* pNode2=bOct2IsLeaf ? octree2 : octree2->pChildren[j];
				if(!pNode1 || !pNode2)
					continue;//one of the subnodes doesn't exist

				std::list<std::pair<int, int> > lIntersections=CheckForIntersections(pNode1, pNode2);
				for (std::list<std::pair<int, int> >::iterator it=lIntersections.begin(); it!=lIntersections.end(); it++)
					intersectingTris.push_back(*it);
				lIntersections.clear();
			}
	}
	return intersectingTris;
}

Ogre::Vector2
GeometricTools::getUVCoords(const Ogre::Vector3& vEdge1, const Ogre::Vector3& vEdge2, const Ogre::Vector3& vPoint)
{
	Ogre::Vector3 vDepth=vEdge1.crossProduct(vEdge2);
	Ogre::Matrix3 mTransform=Ogre::Matrix3(
		vEdge1.x, vEdge2.x, vDepth.x,
		vEdge1.y, vEdge2.y, vDepth.y,
		vEdge1.z, vEdge2.z, vDepth.z);
	Ogre::Matrix3 mInverseTransform;
	bool bSuccess=mTransform.Inverse(mInverseTransform);
	if(!bSuccess)
		return Ogre::Vector2(0,0);
	Ogre::Vector3 vUVCoords=mInverseTransform*vPoint;
	return Ogre::Vector2(vUVCoords.x, vUVCoords.y);
}

/*
 * VertexMerger.cpp
 *
 *  Created on: Apr 28, 2011
 *      Author: _1nsane
 */

#include "VertexMerger.h"

struct OctreeNode
{
	OctreeNode(){vPos.x=0.0f;vPos.y=0.0f;vPos.z=0.0f;
		aSubNodes[0]=0;aSubNodes[1]=0;aSubNodes[2]=0;aSubNodes[3]=0;aSubNodes[4]=0;aSubNodes[5]=0;aSubNodes[6]=0;aSubNodes[7]=0;}
	Ogre::Vector3 vPos;
	OctreeNode* aSubNodes[8];
	int iIndex;
};

/*
//returns current vertex count
int ExtractOctree(OctreeNode* pNode, int iVertexCount, int* aiIndexTable, NxVec3* aNewVertices)
{
	for(std::list<int>::const_iterator it=pNode->liIndices.begin();
		it!=pNode->liIndices.end(); it++)
		aiIndexTable[*it]=iVertexCount;
	aNewVertices[iVertexCount++]=pNode->vPos;
	for(int iSubNode=0; iSubNode<8; iSubNode++)
		if(pNode->aSubNodes[iSubNode])
		{
			iVertexCount=ExtractOctree(pNode->aSubNodes[iSubNode], iVertexCount, aiIndexTable, aNewVertices);
			delete pNode->aSubNodes[iSubNode];
		}
	return iVertexCount;
}*/

void DeleteOctree(OctreeNode* root)
{
	for(int iSubNode=0; iSubNode<8; iSubNode++)
		if(root->aSubNodes[iSubNode])
			DeleteOctree(root->aSubNodes[iSubNode]);
	delete root;
}

#define IS_IN_BOX(v1,v2,d) ((v1.x<=v2.x+d) && (v1.x>=v2.x-d) && (v1.y<=v2.y+d) && (v1.y>=v2.y-d) && (v1.z<=v2.z+d) && (v1.z>=v2.z-d))

#define EIGHTH_SPACE_INDEX(v1,v2) (((v1.x>v2.x)?4:0)+((v1.y>v2.y)?2:0)+((v1.z>v2.z)?1:0))

std::vector<unsigned int>
VertexMerger::GetMatchingIndices(std::vector<Ogre::Vector3> vVertices, const float& fEps)
{
	if(!vVertices.size())
		return std::vector<unsigned int>();

	OctreeNode* root=new OctreeNode;
	root->vPos=vVertices[0];
	root->iIndex=0;
	unsigned int iVertex=0;

	std::vector<unsigned int> vRet(vVertices.size(), 0);

	for(;iVertex<vVertices.size(); iVertex++)
	{
		float fMergeDist=vVertices[iVertex].length()*fEps;
		OctreeNode* pCurrNode=root;
		while(true)
		{
			if(IS_IN_BOX(vVertices[iVertex], pCurrNode->vPos, fMergeDist))
			{
				vRet[iVertex]=pCurrNode->iIndex;
				break;
			}
			else
			{//vertex is not in merge distance to this node
				int iSubNode=EIGHTH_SPACE_INDEX(pCurrNode->vPos, vVertices[iVertex]);
				if(pCurrNode->aSubNodes[iSubNode])
					//proceed deeper into the tree
					pCurrNode=pCurrNode->aSubNodes[iSubNode];
				else
				{//there is no branch so make one
					pCurrNode->aSubNodes[iSubNode]=new OctreeNode;
					pCurrNode=pCurrNode->aSubNodes[iSubNode];
					pCurrNode->vPos=vVertices[iVertex];
					pCurrNode->iIndex=iVertex;
				}
			}//pCurrNode is now one level lower in the tree
		}
	}
	DeleteOctree(root);
	return vRet;
}

/*
 * MeshTools.cpp
 *
 *  Created on: Dec 22, 2011
 *      Author: _1nsane
 */

#include "MeshTools.h"
#include <stack>

std::vector<std::set<unsigned int> >
MeshTools::findNeighboringTris(const std::vector<unsigned int>& viIndices, const std::vector<unsigned int>& viMatchingVertexIndices)
{
	std::vector<std::set<unsigned int> > vNeighboringTris;
	std::map<int, std::vector<unsigned int> > vTrisByVertex;//temporary
	for(unsigned int iTri=0; iTri<viIndices.size()/3; iTri++)
	{
		for(int iVert=0; iVert<3; iVert++)
		{
			int iLookedUpVert=viMatchingVertexIndices[viIndices[iTri*3+iVert]];
			std::map<int, std::vector<unsigned int> >::iterator it=vTrisByVertex.find(iLookedUpVert);
			if(it==vTrisByVertex.end())
				vTrisByVertex[iLookedUpVert]=std::vector<unsigned int>(1, iTri);
			else
				it->second.push_back(iTri);
		}
	}
	vNeighboringTris=std::vector<std::set<unsigned int> >(viIndices.size()/3);
	for(unsigned int iTri=0; iTri<viIndices.size()/3; iTri++)
	{
		//iterate vertices of this tri
		for(int iVert=0; iVert<3; iVert++)
		{
			int iLookedUpVert=viMatchingVertexIndices[viIndices[iTri*3+iVert]];
			std::map<int, std::vector<unsigned int> >::iterator it=vTrisByVertex.find(iLookedUpVert);
			if(it==vTrisByVertex.end())
				continue;
			for(unsigned int iNeighboringTri=0; iNeighboringTri<it->second.size(); iNeighboringTri++)
			{
				if(it->second[iNeighboringTri]!=iTri)
					vNeighboringTris[iTri].insert(it->second[iNeighboringTri]);
			}
		}
	}
	return vNeighboringTris;
}


std::vector<std::pair<std::set<unsigned int>, bool> >
MeshTools::getTriPatches(const std::vector<std::set<unsigned int> >& vNeighboringTris,
		const std::map<unsigned int, std::set<unsigned int> >& mCutTris)
{
	//performance analysis revealed:
	//build cache for mCutTris
	std::vector<bool> vbCutTris(vNeighboringTris.size(), false);
	for(std::map<unsigned int, std::set<unsigned int> >::const_iterator it=mCutTris.begin();
			it!=mCutTris.end(); it++)
		vbCutTris[it->first]=true;


	std::vector<std::pair<std::set<unsigned int>, bool> > vTriIslands;

	//build triangle islands
	std::vector<bool> vbCheckedTris(vNeighboringTris.size(), false);
	std::stack<unsigned int> stTrisToCheck;

	for(unsigned int iTri=0; iTri<vbCheckedTris.size(); iTri++)
	{
		if(vbCheckedTris[iTri])
			continue;
		//if(mCutTris.find(iTri)!=mCutTris.end())
		if(vbCutTris[iTri])
			continue;
		bool bWasCut=false;

		stTrisToCheck.push(iTri);
		std::set<unsigned int> currTriSet;
		while(stTrisToCheck.size())
		{
			int iCurrTri=stTrisToCheck.top();
			stTrisToCheck.pop();
			vbCheckedTris[iCurrTri]=true;
			//drop tri if cut

			currTriSet.insert(iCurrTri);
			for(std::set<unsigned int>::iterator itNeighbor=vNeighboringTris[iCurrTri].begin();
					itNeighbor!=vNeighboringTris[iCurrTri].end(); itNeighbor++)
			{
				int iCurrNeighbor=*itNeighbor;
				//to make sure all cut patches will be flagged
				//if(mCutTris.find(iCurrNeighbor)!=mCutTris.end())
				if(vbCutTris[iCurrNeighbor])
				{
					bWasCut=true;
					vbCheckedTris[iCurrNeighbor]=true;
					continue;
				}
				if(vbCheckedTris[iCurrNeighbor])
					continue;
				stTrisToCheck.push(iCurrNeighbor);
			}
		}
		vTriIslands.push_back(std::make_pair(currTriSet, bWasCut));
	}
	return vTriIslands;
}


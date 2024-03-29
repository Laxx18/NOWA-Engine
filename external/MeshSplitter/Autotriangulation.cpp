/*
 * Autotriangulation.cpp
 *
 *  Created on: Sep 6, 2011
 *      Author: insi
 */

#include <list>

#include <algorithm>
#include <map>
#include <set>
#include <stdio.h>
#include <algorithm>
#include <execution>
#include "Autotriangulation.h"

std::list<int>::iterator
iter_next(std::list<int>& l, std::list<int>::iterator it)
{
	it++;
	if(it==l.end())
		return l.begin();
	return it;
}

std::list<int>::iterator
iter_prev(std::list<int>& l, std::list<int>::iterator it)
{
	if(it==l.begin())
	{
		it=l.end();
		it--;
		return it;
	}
	it--;
	return it;
}

unsigned int iter_next(const std::vector<unsigned int>& v, const int &i)
{
	unsigned int iNext=i+1;
	if(iNext>=v.size())
		return 0;
	return iNext;
}

unsigned int iter_prev(const std::vector<unsigned int>& v, const int &i)
{
	unsigned int iPrev=i-1;
	if(iPrev>=v.size())
		return v.size()-1;
	return iPrev;
}

struct SRelation
{
	unsigned int iParent;
	unsigned int iSelf;
	std::set<unsigned int> viChildren;
};

template <class S, class T>
bool compPairFirst(const std::pair<S,T>& pair1, const std::pair<S,T>& pair2)
{
	return pair1.first<pair2.first;
}

float calcCutPos(SVertex r0, SVertex r1, SVertex u0, SVertex u1)
{
	SVertex s=r1-r0;//r0 is base vector

	return (u0.y*s.x-u0.x*s.y)/(u1.y*u0.x-u1.x*u0.y);
}

bool
segmentsIntersect(SVertex r0, SVertex r1, SVertex u0, SVertex u1, bool r0IsRay=false, float* pfRayPos=NULL)
{
	static const float fAcc=.0001f;
	SVertex s=r1-r0;//r0 is base vector

	float fInvDenom=1.0f/(u1.y*u0.x-u1.x*u0.y);
	float a=(u0.y*s.x-u0.x*s.y)*fInvDenom;
	s=r0-r1;
	float b=-(u1.y*s.x-u1.x*s.y)*fInvDenom;

	if(pfRayPos!=NULL)
		*pfRayPos=b;
	return b>-fAcc && (r0IsRay ? true :  b<1+fAcc) && a>-fAcc && a<1+fAcc;
}

//#define VERBOSE

//#include <unistd.h>

std::vector<unsigned int>
TriangulatePolygon(const std::vector<SVertex>& vertices, std::vector<unsigned int> vCurrPoly, unsigned int iCurrIndex, float fMinAngle, bool& bError)
{
	//usleep(10000);
	std::vector<unsigned int> viIndices;
	bError=false;

#ifdef VERBOSE
	printf("current poly size: %d, step %d\nsearching for a convex vertex to clip\n", (int)vCurrPoly.size(), vCurrPoly.size());
#endif

	unsigned int aTri[3];
	if(vCurrPoly.size()<=2)
		return viIndices;
	//search a workable vertex
#ifdef VERBOSE
	printf("starting with vertex %d, index in poly %d\n", vCurrPoly[iCurrIndex], iCurrIndex);
#endif
	if(vCurrPoly.size()&1)
		iCurrIndex=iter_prev(vCurrPoly, iCurrIndex);
	else
		iCurrIndex=iter_next(vCurrPoly, iCurrIndex);

#ifdef VERBOSE
	printf("jumped to vertex %d\n", vCurrPoly[iCurrIndex]);
#endif
	unsigned int iStart=iCurrIndex;
	bool bStart=true;
	bool bFoundOne=false;
	for(; iCurrIndex!=iStart || bStart; iCurrIndex=iter_next(vCurrPoly, iCurrIndex))
	{
#ifdef VERBOSE
		printf("checking vertex (%d) for being clockwise... ", vCurrPoly[iCurrIndex]);
#endif
		bStart=false;
		//calc vectors
		unsigned int iPrev=iter_prev(vCurrPoly, iCurrIndex);
		unsigned int iNext=iter_next(vCurrPoly, iCurrIndex);
		SVertex u =vertices[aTri[1]=vCurrPoly[iCurrIndex]];
		SVertex v0=vertices[aTri[0]=vCurrPoly[iPrev]]-u;
		SVertex v1=vertices[aTri[2]=vCurrPoly[iNext]]-u;
		//check if this tri is clockwise
		SVertex n=SVertex(-v0.y, v0.x);
		if(n*v1>fMinAngle)
		{
#ifdef VERBOSE
			printf("failed\n");
#endif
			continue;
		}
#ifdef VERBOSE
		printf("succeeded\n");
#endif
		//check all vertices if they are inside the tri
		unsigned int iIndex=iter_next(vCurrPoly, iNext);
		bool bInvalidTri=false;
		while(iIndex!=iCurrIndex)
		{
#ifdef VERBOSE
			printf("testing vertex (%d) for being inside the current ear... ", vCurrPoly[iIndex]);
#endif
			SVertex v2=vertices[vCurrPoly[iIndex]]-u;
			float	dot00=v0*v0,
					dot01=v0*v1,
					dot02=v0*v2,
					dot11=v1*v1,
					dot12=v1*v2;

			float fInvDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);
			float s = (dot11 * dot02 - dot01 * dot12) * fInvDenom;
			float t = (dot00 * dot12 - dot01 * dot02) * fInvDenom;

			if(s>0 && t>0 && s+t<1)
			{
#ifdef VERBOSE
				printf("failed\n");
#endif
				bInvalidTri=true;
				break;
			}
#ifdef VERBOSE
			printf("succeeded\n");
#endif
			iIndex=iter_next(vCurrPoly, iIndex);
		}
		if(!bInvalidTri)
		{
			std::vector<unsigned int> vSubPoly=vCurrPoly;
			/*for(std::list<int>::iterator itSub=lSubPoly.begin(); itSub!=lSubPoly.end(); itSub++)
				if(*itSub==*it)
					itSub=lSubPoly.erase(itSub);*/
			vSubPoly.erase(vSubPoly.begin()+iCurrIndex);

			unsigned int iSubIndex=iCurrIndex;
			if(iSubIndex>=vSubPoly.size()-1)
				iSubIndex=0;

			/*it=lCurrPoly.erase(it);
			if(it==lCurrPoly.end())
				it=lCurrPoly.begin();*/
			bStart=false;
			bool bSubError;
			std::vector<unsigned int> viTris=TriangulatePolygon(vertices, vSubPoly, iSubIndex, fMinAngle, bSubError);

			if(!bSubError)
			{
				//iterator now contains a working tri
				//save triangle to index buffer
				viIndices.push_back(aTri[0]);
				viIndices.push_back(aTri[1]);
				viIndices.push_back(aTri[2]);

				viIndices.insert(viIndices.end(), viTris.begin(), viTris.end());
				bFoundOne=true;
				break;
			}
#ifdef VERBOSE
			printf("sub-triangulation quit with error (triangle size %d)\n", vCurrPoly.size());
#endif
			//error triangulating, try next one
		}
	}
	if(!bFoundOne)
	{
#ifdef VERBOSE
		printf("didn't find a triangle\n");
#endif
		bError=true;
		return viIndices;
	}
#ifdef VERBOSE
	printf("sub-triangulation returns %d indices\n", viIndices.size());
#endif
	return viIndices;
}

std::vector<unsigned int>
TriangulatePolygons(const std::vector<SVertex>& vertices, const std::vector<std::vector<unsigned int> >& polys, std::string& str)
{
	//input: a bunch of nested loops
	//output: polygons without holes
#ifdef VERBOSE
		printf("\n\ntriangulation:\n");
#endif
	std::vector<std::vector<unsigned int> > polygons;

	//do a consistency check: no line may cut another line
	for(unsigned int i=0; i<polys.size(); i++)
	{
		std::vector<unsigned int> v;
#ifdef VERBOSE
		printf("loop %d:", i);
#endif
		if(polys[i].size()<3)
		{
#ifdef VERBOSE
			printf(" polygon has less than three vertices\n");
#endif
			continue;
		}
		float fSumAreas=0;
		for(unsigned int j=0; j<polys[i].size(); j++)
		{

#ifdef VERBOSE
			printf(" %d", polys[i][j]);
#endif
			SVertex r0=vertices[polys[i][j==0 ? polys[i].size()-1 : j-1]];
			SVertex u0=vertices[polys[i][j]]-r0;
			for(unsigned int k=0; k<polys.size(); k++)
			{
				for(unsigned int l=0; l<polys[k].size(); l++)
				{
					if(k==i)
					{
						if(l==j || l==j+1 || l==j-1)
							continue;
						if(!j && l==polys[k].size()-1)
							continue;
						if(j==polys[k].size()-1 && l==0)
							continue;
					}
					SVertex r1=vertices[polys[k][l==0 ? polys[k].size()-1 : l-1]];
					SVertex u1=vertices[polys[k][l]]-r1;
					if(segmentsIntersect(r0, r1, u0, u1))
					{
						str=std::string("self-intersection!");
						return std::vector<unsigned int>();
					}
				}
			}
			v.push_back(polys[i][j]);

			unsigned int iCurr=j;
			SVertex v1 =vertices[polys[i][iCurr]];
			if(++iCurr>=polys[i].size())
				iCurr=0;
			SVertex v2=vertices[polys[i][iCurr]];
			fSumAreas+=(v2.x-v1.x)*(v2.y+v1.y);
			//calc angle to next vertex
		}
		polygons.push_back(v);
#ifdef VERBOSE
		printf("\n");
#endif
		//check for clockwise loops, reverse if necessary
		if(fSumAreas>0)
		{//clockwise
#ifdef VERBOSE
			printf("flipped loop:");
#endif
			v.clear();
			unsigned int nPolys=polys[i].size();
			for(unsigned int j=0; j<polys[i].size(); j++)
			{
#ifdef VERBOSE
				printf(" %d", polys[i][nPolys-1-j]);
#endif
				v.push_back(polys[i][nPolys-1-j]);
			}
			polygons[polygons.size()-1]=v;
#ifdef VERBOSE
			printf("\n");
#endif
		}

	}


	SRelation initialRelation;
	initialRelation.iParent=-1;
	std::vector<SRelation> relations(polygons.size(), initialRelation);

	//first find the relative positions of the loops
	for(unsigned int i=0; i<polygons.size(); i++)
	{
#ifdef VERBOSE
		printf("checking polygon %d for being inside another one\n", i);
#endif
		//cast a ray through all loops
		//for simplicitiy's sake, start from the first vertex and walk in positive x direction
		SVertex r0=vertices[polygons[i][0]];
		SVertex u0=SVertex(1,0);

		std::vector<std::pair<float, std::pair<unsigned int, unsigned int> > > vCuts;
		for(unsigned int k=0; k<polygons.size(); k++)
		{
			if(k==i)
				continue;

			for(unsigned int l=0; l<polygons[k].size(); l++)
			{
				unsigned int iLastVert=l==0 ? polygons[k].size()-1 : l-1;
				SVertex r1=vertices[polygons[k][iLastVert]];
				SVertex u1=vertices[polygons[k][l]]-r1;
				float t;
				if(segmentsIntersect(r0, r1, u0, u1, true, &t))
				{
					//loop k, distance t
					vCuts.push_back(std::make_pair(t, std::make_pair(k, iLastVert)));
#ifdef VERBOSE
					printf("intersection with polygon %d, edge %d @ distance %f\n", k, iLastVert, t);
#endif
				}
			}
		}
		//check which loops are being cut an uneven times and the order in which this happens

		//sort by distance
		std::sort(std::execution::par, vCuts.begin(), vCuts.end(), compPairFirst<float, std::pair<unsigned int, unsigned int> >);
		std::map<unsigned int, unsigned int> mCuts;
		//traverse the cuts
		std::map<unsigned int, unsigned int>::iterator itCut;
		float fLastCutPos=-1;

		for(unsigned int iCut=0; iCut<vCuts.size(); iCut++)
		{
#ifdef VERBOSE
			printf("checking cut on polygon %d, edge %d @ distance %f\n", vCuts[iCut].second.first, vCuts[iCut].second.second, vCuts[iCut].first);
#endif
			//if two cuts are really close, this is typically generated by a vertex,
			//since the tracing line hits both edges "at once"
			float fMergeDist=fLastCutPos*1e-5;
			if(vCuts[iCut].first<(fLastCutPos+fMergeDist))
			{
#ifdef VERBOSE
				printf("cut shares location with last one\n");
#endif
				fLastCutPos=vCuts[iCut].first;
				unsigned int iPoly=vCuts[iCut].second.first;
				if(iPoly == vCuts[iCut-1].second.first)
				{
#ifdef VERBOSE
					printf("cut is also on same loop\n");
#endif
					//check if both edges are in the same direction, in this case,
					//we have a tangent to the loop and we can safely ignore that
					//to ignore it, we simply go on and add that polygon a second time, so it cancels out
					//when we check for the uneven intersection count

					//find out vertex
					unsigned int iVertex=std::max(vCuts[iCut].second.second, vCuts[iCut-1].second.second);
					if((vCuts[iCut].second.second==0 && vCuts[iCut-1].second.second>1) ||
							(vCuts[iCut-1].second.second==0 && vCuts[iCut].second.second>1))
						iVertex=0;
#ifdef VERBOSE
					printf("vertex %d being intersected\n", iVertex);
#endif

					unsigned int iPrevVert= iVertex ? iVertex-1 : polygons[iPoly].size()-1;
					unsigned int iNextVert=(iVertex==polygons[iPoly].size()-1) ? 0 : iVertex+1;
					float fCurrY=vertices[polygons[iPoly][iVertex]].y;
#ifdef VERBOSE
					printf("current vertex y: %f\n", fCurrY);
					printf("previous vertex y: %f\n", vertices[polygons[iPoly][iPrevVert]].y);
					printf("next vertex y: %f\n", vertices[polygons[iPoly][iNextVert]].y);
#endif
					if(!((vertices[polygons[iPoly][iPrevVert]].y>=fCurrY && vertices[polygons[iPoly][iNextVert]].y>=fCurrY) ||
							(vertices[polygons[iPoly][iPrevVert]].y<=fCurrY && vertices[polygons[iPoly][iNextVert]].y<=fCurrY)))
					{
#ifdef VERBOSE
						printf("vertex was not tangent\n");
#endif
						continue;
					}
					//else go on
#ifdef VERBOSE
					printf("vertex was tangent\n");
#endif
				}
			}

			if((itCut=mCuts.find(vCuts[iCut].second.first))!=mCuts.end())
				itCut->second++;
			else
				mCuts[vCuts[iCut].second.first]=1;

			fLastCutPos=vCuts[iCut].first;
		}
		//now we can determine which polygon is the parent
		//must be the closest cut with an uneven number of cuts on that poly
		for(unsigned int iCut=0; iCut<vCuts.size(); iCut++)
		{
			unsigned int iPoly=vCuts[iCut].second.first;
#ifdef VERBOSE
			printf("checking cut on polygon %d @ distance %f\n", iPoly, vCuts[iCut].first);
#endif
			if(mCuts[iPoly]&1)
			{//yep, that's it!
#ifdef VERBOSE
				printf("polygon %d is inside %d\n", i, iPoly);
#endif
				relations[i].iParent=iPoly;
				relations[iPoly].viChildren.insert(i);
				break;
			}
		}
	}

	//next create groups of one outside loop and many loops inside
	std::vector<SRelation> polygroups;
	for(unsigned int iRelation=0; iRelation<relations.size(); iRelation++)
	{
#ifdef VERBOSE
		printf("processing relation %d\n", iRelation);
#endif
		//check if this poly has an even number of parents
		unsigned int iCurrRelation=iRelation;
		int iLevel=0;
		while(relations[iCurrRelation].iParent!=(unsigned int)-1)
		{
			iCurrRelation=relations[iCurrRelation].iParent;
			iLevel++;
		}
		//if so, add to list
		if(!(iLevel&1))
		{
			relations[iRelation].iSelf=iRelation;
			polygroups.push_back(relations[iRelation]);
		}
	}

#ifdef VERBOSE
	//printf("\n");
#endif
	//cut the nested loops to create a bunch of normal polys
	std::vector<std::vector<unsigned int> > splitpolys=polygons;

	for(unsigned int iPolyGroup=0; iPolyGroup<polygroups.size(); iPolyGroup++)
	{
connected_loop:
#ifdef VERBOSE
		printf("cutting out poly group %d\n", iPolyGroup);
#endif
		//iterate every inside loop
		for(std::set<unsigned int>::iterator itInsideLoop=polygroups[iPolyGroup].viChildren.begin();
			itInsideLoop!=polygroups[iPolyGroup].viChildren.end(); itInsideLoop++)
		{
			unsigned int iPoly=*itInsideLoop;
			//walk along the loop
			for(unsigned int iVertex=0; iVertex<splitpolys[iPoly].size(); iVertex++)
			{
				SVertex r0=vertices[splitpolys[iPoly][iVertex]];
				//iterate all other loops
				std::set<unsigned int>::iterator itTargetLoop=polygroups[iPolyGroup].viChildren.begin();
				for(unsigned int iTargetLoop=0; iTargetLoop<=polygroups[iPolyGroup].viChildren.size(); iTargetLoop++)
				{
					unsigned int iLoop;
					if(!iTargetLoop)
						iLoop=polygroups[iPolyGroup].iSelf;
					else
					{
						iLoop=*itTargetLoop;
						itTargetLoop++;
					}
					if(iLoop==iPoly)
						continue;
					//walk along the loop
					for(unsigned int iTargetVertex=0; iTargetVertex<splitpolys[iLoop].size(); iTargetVertex++)
					{
						SVertex u0=vertices[splitpolys[iLoop][iTargetVertex]]-r0;
						bool bIntersection=false;
						//check if the connection between the points intersects a loop
						for(unsigned int i=0; i<splitpolys.size(); i++)
						{
							for(unsigned int j=0; j<splitpolys[i].size(); j++)
							{
								unsigned int jMinusOne=j==0 ? splitpolys[i].size()-1 : j-1;
								//splitpolys[iLoop][iTargetVertex]
								//splitpolys[iPoly][iVertex];
								//check if the target loop is being tested
								if(i==iPoly && (j==iVertex || iVertex==jMinusOne))
									continue;
								if(i==iLoop && (j==iTargetVertex || iTargetVertex==jMinusOne))
									continue;

								SVertex r1=vertices[splitpolys[i][jMinusOne]];
								SVertex u1=vertices[splitpolys[i][j]]-r1;
								if(segmentsIntersect(r0, r1, u0, u1))
								{
									bIntersection=true;
									break;
								}
							}
							if(bIntersection)
								break;
						}
						//in that case this is not a valid connection
						if(bIntersection)
							continue;
						//create a "bridge" between the loops
						//do this by ripping both loops up
						//and inserting a new segment, maintaining the loop direction
						std::vector<unsigned int> viNewLoop;
						//this is important, since in this case the inner loop has to be reversed
						bool bConnToOuterLoop=(iLoop==polygroups[iPolyGroup].iSelf);
						bool bWrapped=false;
						int iStep= bConnToOuterLoop ? -1 : 1 ;
						//push the inner poly
						for(int iPolyVert=iVertex; !(bWrapped && iPolyVert==(int)iVertex); iPolyVert+=iStep)
						{
							if(iPolyVert<0)
							{
								iPolyVert=splitpolys[iPoly].size();
								bWrapped=true;
								continue;
							}
							if(iPolyVert>=(int)splitpolys[iPoly].size())
							{
								iPolyVert=-1;
								bWrapped=true;
								continue;
							}
							viNewLoop.push_back(splitpolys[iPoly][iPolyVert]);
						}
						//push the first vertex again
						viNewLoop.push_back(splitpolys[iPoly][iVertex]);
						bWrapped=false;
						for(int iLoopVert=iTargetVertex; !(bWrapped && iLoopVert==(int)iTargetVertex); iLoopVert++)
						{
							if(iLoopVert>=(int)splitpolys[iLoop].size())
							{
								iLoopVert=-1;
								bWrapped=true;
								continue;
							}
							viNewLoop.push_back(splitpolys[iLoop][iLoopVert]);
						}
						viNewLoop.push_back(splitpolys[iLoop][iTargetVertex]);

						splitpolys[iLoop]=viNewLoop;
						//delete the original loop
						splitpolys[iPoly].clear();
						polygroups[iPolyGroup].viChildren.erase(iPoly);
						goto connected_loop;
					}
				}
			}
		}
	}

	//triangulate polygons
	std::vector<unsigned int> viIndices;
	unsigned int nExpectedReturnTris=0;

	for(unsigned int iSplitPoly=0; iSplitPoly<splitpolys.size(); iSplitPoly++)
	{
		if(splitpolys[iSplitPoly].size()<3)
			continue;
#ifdef VERBOSE
		printf("split poly %d: ", iSplitPoly);
#endif
		nExpectedReturnTris+=splitpolys[iSplitPoly].size()-2;
#ifdef VERBOSE
		for(unsigned int i=0; i<splitpolys[iSplitPoly].size(); i++)
		{
			printf(" %d", splitpolys[iSplitPoly][i]);
		}
		printf("\n");
#endif

		float fMinAngle=-1e-2;
		bool bError;
		std::vector<unsigned int> viTris=TriangulatePolygon(vertices, splitpolys[iSplitPoly], 0, fMinAngle, bError);
		if(bError)
		{
#ifdef VERBOSE
			printf("\ntriangulation with min angle failed, set to zero\n");
#endif
			viTris=TriangulatePolygon(vertices, splitpolys[iSplitPoly], 0, 0, bError);
			if(bError)
				str+="triangulation failed";
		}
		viIndices.insert(viIndices.end(), viTris.begin(), viTris.end());
	}
#ifdef VERBOSE
	printf("returning %d indices\n", (int)viIndices.size());
#endif
	if(viIndices.size()!=nExpectedReturnTris*3)
		str=std::string("returned index count is not the expected value!");
	return viIndices;
}

/*
 * DestructibleMeshSplitter.cpp
 *
 *  Created on: Aug 24, 2010
 *      Author: _1nsane
 */

#include "DestructibleMeshSplitter.h"
#include "VertexMerger.h"
#include "CutMeshGenerator.h"
#include "OgreMeshExtractor.h"
#include "GeometricTools.h"
#include "Autotriangulation.h"
#include "MeshTools.h"

#include <fstream>
#include <list>
#include <algorithm>
#include <set>
#include <stack>

//#define VERBOSE

Ogre::v1::MeshPtr
DestructibleMeshSplitter::loadMesh(Ogre::String strFile)
{
	Ogre::v1::MeshSerializer *pSerializer=new Ogre::v1::MeshSerializer();
	Ogre::v1::MeshPtr mesh = Ogre::v1::MeshManager::getSingleton().createManual(OgreMeshExtractor::getRandomMeshName(), "General");

	std::ifstream stream(strFile.c_str() , std::ifstream::binary);
	stream.seekg (0, std::ios::end);
	int iFileSize = stream.tellg();
	stream.seekg (0, std::ios::beg);

	if(iFileSize<=0)
	{
		printf("error loading file!\n");
		return Ogre::v1::MeshPtr();
	}

	char* pBuffer=new char[iFileSize];
	stream.read(pBuffer, iFileSize);
	stream.close();

	Ogre::MemoryDataStream* pmemStream=new Ogre::MemoryDataStream(pBuffer, iFileSize);
	Ogre::DataStreamPtr ptr(pmemStream);

	pSerializer->importMesh(ptr, mesh.get());

	pmemStream->close();
	delete pBuffer;
	delete pSerializer;

	return mesh;
}

void DestructibleMeshSplitter::unloadMesh(Ogre::v1::MeshPtr mesh)
{
	Ogre::v1::MeshManager::getSingleton().remove(mesh->getHandle());
}

bool hasNoTangentsAndCanGenerate(Ogre::v1::VertexDeclaration* vertexDecl)
{
	bool hasTangents = false;
	bool hasUVs = false;
	const Ogre::v1::VertexDeclaration::VertexElementList& elementList = vertexDecl->getElements();
	Ogre::v1::VertexDeclaration::VertexElementList::const_iterator itor = elementList.begin();
	Ogre::v1::VertexDeclaration::VertexElementList::const_iterator end = elementList.end();

	while (itor != end && !hasTangents)
	{
		const Ogre::v1::VertexElement& vertexElem = *itor;
		if (vertexElem.getSemantic() == Ogre::VES_TANGENT)
		{
			hasTangents = true;
		}
		if (vertexElem.getSemantic() == Ogre::VES_TEXTURE_COORDINATES)
		{
			hasUVs = true;
		}
		++itor;
	}

	return !hasTangents && hasUVs;
}

std::vector<Ogre::v1::MeshPtr>
DestructibleMeshSplitter::SplitMesh(Ogre::v1::MeshPtr inMesh,
		float fMaxSize, float fRoughness, float fResolution, bool bSmooth,
		unsigned int nRecoveryAttempts,
		bool bCutSurface, const Ogre::String& strCutMaterial)
{
	std::vector<Ogre::v1::MeshPtr> meshes;
	meshes.push_back(inMesh);
	std::list<int> meshIndices;
	meshIndices.push_back(0);
	std::set<int> siLeaves;
	siLeaves.insert(0);

	while(meshIndices.size())
	{
		int iCurrIndex=meshIndices.back();
		Ogre::AxisAlignedBox box=meshes[iCurrIndex]->getBounds();
		printf("\nsplitting mesh %d (x=%.2f X=%.2f y=%.2f Y=%.2f z=%.2f Z=%.2f)... ", iCurrIndex,
				box.getMinimum().x, box.getMaximum().x,
				box.getMinimum().y, box.getMaximum().y,
				box.getMinimum().z, box.getMaximum().z);
		meshIndices.pop_back();
		Ogre::Vector3 vBoxSize=box.getSize();
		if(vBoxSize.x>fMaxSize || vBoxSize.y>fMaxSize || vBoxSize.z>fMaxSize)
		{
			//this fragment is too big, split it
			//create new meshes
			Ogre::v1::MeshPtr outMeshes[2];
			bool bError=false;
			bool bDiscardCurrent=false;
			for(unsigned int iAttempt=0; iAttempt<nRecoveryAttempts; iAttempt++)
			{
				if(bError)
				{
					unloadMesh(outMeshes[0]);
					unloadMesh(outMeshes[1]);
				}
				bError=false;
				printf("\nattempt %d of %d... ", iAttempt, nRecoveryAttempts);
				Split(meshes[iCurrIndex], outMeshes[0], outMeshes[1], fRoughness, fResolution, bCutSurface, strCutMaterial, bSmooth, bError);
				//check if fragments lost a sufficient amount of size (5% here)
				for(int iMesh=0; iMesh<2; iMesh++)
				{
					Ogre::AxisAlignedBox outBox=outMeshes[iMesh]->getBounds();
					if(outBox.getSize().length()>box.getSize().length()*0.95f)
					{
						printf("error: fragments too big\n");
						Ogre::AxisAlignedBox outBox=outMeshes[iMesh]->getBounds();
						printf("fragment %d: (x=%.2f X=%.2f y=%.2f Y=%.2f z=%.2f Z=%.2f)\n", iMesh,
								outBox.getMinimum().x, outBox.getMaximum().x,
								outBox.getMinimum().y, outBox.getMaximum().y,
								outBox.getMinimum().z, outBox.getMaximum().z);
						if(iAttempt==(nRecoveryAttempts-1))
						{
							printf("danger of inflationary growth of segments is imminent, aborting!\n");
							bDiscardCurrent=true;
						}
						bError=true;
					}
				}
				if(!bError)
					break;
			}
			siLeaves.erase(iCurrIndex);
			if(bDiscardCurrent)
				continue;
			if(bError)
				printf("continuing in spite of errors\n");

			printf("resulting in meshes ");
			for(int iMesh=0; iMesh<2; iMesh++)
			{
				if(outMeshes[iMesh].get())
				{
					const bool bSplitPatches=true;
					if(bSplitPatches)
					{
						//check if the output meshes are composed of individual parts,
						//which will then also be added as split results
						std::vector<Ogre::Vector3> vvVertices;
						std::vector<unsigned int> viSubmeshVertexOffsets, viIndices, viSubmeshIndexOffsets, viMatchingVertices;
						//this is a look up table to check for triangle-islands
						OgreMeshExtractor::Extract(outMeshes[iMesh], vvVertices, viSubmeshVertexOffsets,
								viIndices, viSubmeshIndexOffsets);
						viMatchingVertices=VertexMerger::GetMatchingIndices(vvVertices);
						std::vector<std::set<unsigned int> > avNeighboringTris=
								MeshTools::findNeighboringTris(viIndices, viMatchingVertices);
						std::map<unsigned int, std::set<unsigned int> > dummy;

						std::vector<std::pair<std::set<unsigned int>, bool> > vPatches=MeshTools::getTriPatches(avNeighboringTris, dummy);
						if(vPatches.size()==1)
						{//can directly add the current mesh
							meshes.push_back(outMeshes[iMesh]);
							meshIndices.push_back(meshes.size()-1);
							siLeaves.insert(meshes.size()-1);
							printf("%d ", meshes.size()-1);
							continue;
						}
						for(unsigned int iPatch=0; iPatch<vPatches.size(); iPatch++)
						{
							std::set<unsigned int>& siPatchTris=vPatches[iPatch].first;
							std::vector<std::pair<int, int> > viNewIndices;

							for(std::set<unsigned int>::const_iterator itTris=siPatchTris.begin();
									itTris!=siPatchTris.end(); itTris++)
								for(int iVertex=0; iVertex<3; iVertex++)
									viNewIndices.push_back(std::make_pair((int)*itTris, iVertex));

							Ogre::v1::MeshPtr pMesh=OgreMeshExtractor::RebuildMesh(outMeshes[iMesh], std::vector<std::pair<int, Ogre::Vector2> >(), viNewIndices);

							meshes.push_back(pMesh);
							meshIndices.push_back(meshes.size()-1);
							siLeaves.insert(meshes.size()-1);
							printf("%d ", meshes.size()-1);
						}
					}
					else
					{
						meshes.push_back(outMeshes[iMesh]);
						meshIndices.push_back(meshes.size()-1);
						siLeaves.insert(meshes.size()-1);
						printf("%d ", meshes.size()-1);
					}
				}
			}
		}
		else
			printf("mesh already small enough!");
	}
	std::vector<Ogre::v1::MeshPtr> outmeshes;
	//search for leaves
	for(int iMesh=1; iMesh<(int)meshes.size(); iMesh++)
	{
		bool generateTangents = false;
		if (meshes[iMesh]->sharedVertexData[0])
		{
			Ogre::v1::VertexDeclaration* vertexDecl = meshes[iMesh]->sharedVertexData[0]->vertexDeclaration;
			generateTangents |= hasNoTangentsAndCanGenerate(vertexDecl);
			
			// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "--->Generate tangents: " + Ogre::StringConverter::toString(generateTangents) + " for mesh: " + meshes[iMesh]->getName());
		}

		for (unsigned short i = 0; i < meshes[iMesh]->getNumSubMeshes(); ++i)
		{
			Ogre::v1::SubMesh* subMesh = meshes[iMesh]->getSubMesh(i);
			if (subMesh->vertexData[0])
			{
				Ogre::v1::VertexDeclaration* vertexDecl = subMesh->vertexData[0]->vertexDeclaration;
				generateTangents |= hasNoTangentsAndCanGenerate(vertexDecl);
				// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "--->Generate tangents: " + Ogre::StringConverter::toString(generateTangents) + " for mesh: " + subMesh->getMaterialName());
			}
		}

		try
		{
			if (generateTangents)
			{
				meshes[iMesh]->buildTangentVectors();
				// Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "--->Generate tangents finally!!");
			}
		}
		catch (...)
		{
		}

		if(siLeaves.find(iMesh)!=siLeaves.end())
			//it's a leaf
			outmeshes.push_back(meshes[iMesh]);
		else
			//delete it
			unloadMesh(meshes[iMesh]);
	}
	return outmeshes;
}

void DestructibleMeshSplitter::Split(Ogre::v1::MeshPtr inMesh, Ogre::v1::MeshPtr& outMesh1, Ogre::v1::MeshPtr& outMesh2,
		float fRoughness, float fResolution,
		bool bCutSurface, Ogre::String strCutMaterial, bool bSmooth, bool& bError,
		Ogre::v1::MeshPtr debugCutPlane)
{
	bool bDestroyCutPlane=false;
	if(debugCutPlane.get()==0)
	{
		debugCutPlane=Ogre::v1::MeshManager::getSingleton().createManual(OgreMeshExtractor::getRandomMeshName(), "General");
		bDestroyCutPlane=true;
	}
	CutMeshGenerator::CreateRandomCutMesh(inMesh, debugCutPlane, fRoughness, fResolution, strCutMaterial, bSmooth);

	//Ogre::AxisAlignedBox outBox=debugCutPlane->getBounds();

	outMesh1=BooleanOp(inMesh, debugCutPlane, OUTSIDE, INSIDE, bError, !bCutSurface);
	outMesh2=BooleanOp(inMesh, debugCutPlane, INSIDE, INSIDE, bError, !bCutSurface);
	if(bDestroyCutPlane)
		unloadMesh(debugCutPlane);
}

struct SCut
{
	int aiMesh[2];//either 0 or 1; indicates which mesh the cutting edge belongs to
	std::pair<int,int> endVertices[2];//vertices of edge beginning and end
	int aiTris[2];//hold the tris for which the calculation was made
	int aiOrgEdges[2];//edges which were being cut
	Ogre::Vector3 v[2];
	float fRelCoord[2];
	Ogre::Vector2 vUVs[2];
	//int next, prev;//prev on edges[0], next on edges[1]
	int aiConnected[2];//index corresponds to edges
};

//use the fact the each edge can only be cut once by the same tri
//iMesh needed because both start counting from 0
struct SLoopVertex
{
	int iMesh;//mesh with cut edge
	int iTri;//of the other mesh
	std::pair<int,int> edge;//vertexindices in merged vertices

};

bool operator<(const SLoopVertex& x, const SLoopVertex& y)
{
	if(x.iMesh!=y.iMesh)
		return (x.iMesh<y.iMesh);
	if(x.iTri!=y.iTri)
		return (x.iTri<y.iTri);
	if(x.edge.first!=y.edge.first)
		return (x.edge.first<y.edge.first);
	if(x.edge.second!=y.edge.second)
		return (x.edge.second<y.edge.second);
	return false;
}

Ogre::v1::MeshPtr
DestructibleMeshSplitter::BooleanOp(Ogre::v1::MeshPtr inMesh1, Ogre::v1::MeshPtr inMesh2,
		EBoolean_Op op1, EBoolean_Op op2, bool& bError, bool bIgnoreSecond,
		Ogre::v1::MeshPtr debugReTri, Ogre::v1::MeshPtr debugCutLine)
{
	std::vector<Ogre::Vector3> vvVertices[2];
	std::vector<unsigned int> viSubmeshVertexOffsets[2], viIndices[2], viSubmeshIndexOffsets[2], viMatchingVertices[2];

	//extract data of meshes
	Ogre::v1::MeshPtr meshes[2]={inMesh1, inMesh2};
	EBoolean_Op ops[2]={op1, op2};
	//look up for triangle neighbors, created by using vertices shared by tris
	std::vector<std::set<unsigned int> > avNeighboringTris[2];

	std::list<std::pair<int, Ogre::Vector3> > lCuts;
	std::vector<SCut> vEdgeChains;

	//this is a look up table to check for triangle-islands
	std::map<unsigned int, std::set<unsigned int> > amCutTris[2];

	{
	GeometricTools::OctreeNode* pRoot[2];
	for (int i=0; i < 2; i++)
	{
		OgreMeshExtractor::Extract(meshes[i], vvVertices[i], viSubmeshVertexOffsets[i],
				viIndices[i], viSubmeshIndexOffsets[i]);
		viMatchingVertices[i]=VertexMerger::GetMatchingIndices(vvVertices[i]);
		pRoot[i]=GeometricTools::GenerateOctree(vvVertices[i], viIndices[i]);
		//iterate all triangles
		avNeighboringTris[i]=MeshTools::findNeighboringTris(viIndices[i], viMatchingVertices[i]);
	}

	std::list<std::pair<int, int> > lOctreeIntersections=GeometricTools::CheckForIntersections(pRoot[0], pRoot[1]);

	int aiTriIndices[2];
	std::vector<Ogre::Vector3> avTris[2]={std::vector<Ogre::Vector3>(3), std::vector<Ogre::Vector3>(3)};
	//iterate candidate tris for cuts
	for(std::list<std::pair<int, int> >::iterator it=lOctreeIntersections.begin(); it!=lOctreeIntersections.end(); it++)
	{
		aiTriIndices[0]=it->first;
		aiTriIndices[1]=it->second;
		for(int iTri=0; iTri<2; iTri++)
			for(int iIndex=0; iIndex<3; iIndex++)
				avTris[iTri][iIndex]=vvVertices[iTri][viIndices[iTri][aiTriIndices[iTri]*3+iIndex]];

		//calc cut
		//this function shall return the cutting point on the edge in range [0:1]
		std::vector<SCutResult> vCuts=GeometricTools::CutTriangles(avTris[0], avTris[1]);
		if(!vCuts.size())
			//no cut, continue with next candidate
			continue;

		//visual generation only
		if(vCuts.size()!=2)
		{
			printf("warning: vCuts.size()!=2!\n");
			bError=true;
			continue;
		}
		assert(vCuts.size()==2);//a valid cut must produce exactly two cut edges, i.e. one cutting line

		for (unsigned int i = 0; i < vCuts.size(); ++i)
		{
			int iMesh=vCuts[i].iEdgeID/3;
			lCuts.push_back(std::make_pair((iMesh ? 1<<31 : 0)|(aiTriIndices[iMesh]+(vCuts[i].iEdgeID%3)), vCuts[i].vPos));
		}
		SCut cut;
		cut.aiTris[0]=it->first, cut.aiTris[1]=it->second;
		//iterate cuts
		for(int iIndex=0; iIndex<2; iIndex++)
		{
			int iMesh=cut.aiMesh[iIndex]=vCuts[iIndex].iEdgeID/3;//the mesh whose edge is being cut
			//tri in index buffer + edge index
			int iEdge=cut.aiOrgEdges[iIndex]=(aiTriIndices[iMesh]*3+(vCuts[iIndex].iEdgeID%3));

			//look up vertices
			int iVertexIndex1=viMatchingVertices[iMesh][viIndices[iMesh][iEdge]],
				iVertexIndex2=viMatchingVertices[iMesh][viIndices[iMesh][(iEdge%3==2)?iEdge-2:iEdge+1]];
			//order by smallest index in merged vertex buffer
			if(iVertexIndex1>iVertexIndex2)
				std::swap(iVertexIndex1, iVertexIndex2);
			//add looked up edge
			cut.endVertices[iIndex]=std::make_pair(iVertexIndex1, iVertexIndex2);
			cut.v[iIndex]=vCuts[iIndex].vPos;
			cut.fRelCoord[iIndex]=vCuts[iIndex].fRelCoord;
			cut.vUVs[iIndex]=vCuts[iIndex].vUV;
			cut.aiConnected[iIndex]=-1;

			//copy to look up table cut tri->edge chain
			std::map<unsigned int, std::set<unsigned int> >::iterator itCutTri;
			for(int i=0; i<2; i++)
			{
				if((itCutTri=amCutTris[i].find(cut.aiTris[i]))==amCutTris[i].end())
				{
					std::set<unsigned int> s;
					s.insert(vEdgeChains.size());
					amCutTris[i][cut.aiTris[i]]=s;
				}
				else
					itCutTri->second.insert(vEdgeChains.size());
			}
		}
		//add to edge list
		vEdgeChains.push_back(cut);
	}

	}

	{
	//check for loops
	std::map<SLoopVertex, int> mOpenEnds;
	//iterate every cut
	for(unsigned int iCut=0; iCut<vEdgeChains.size(); iCut++)
	{
		//iterate both ends of the segment
		for (int iIndex=0; iIndex < 2; iIndex++)
		{
			//build a loop vertex;
			SLoopVertex lv;
			lv.edge=vEdgeChains[iCut].endVertices[iIndex];
			lv.iMesh=vEdgeChains[iCut].aiMesh[iIndex];
			lv.iTri=vEdgeChains[iCut].aiTris[(lv.iMesh==1) ? 0 : 1];

			//printf("\n%d-\t%d:\t%d\t%d\t%d\t%d", iCut, iIndex, lv.iMesh, lv.iTri, lv.edge.first, lv.edge.second);

			//check map with open line ends
			std::map<SLoopVertex, int>::iterator itEnding=
					mOpenEnds.find(lv);

			if(itEnding!=mOpenEnds.end())
			{//there is a fitting end
				//get cut info
				SCut& cut=vEdgeChains[itEnding->second];
				SCut& curr=vEdgeChains[iCut];
				curr.aiConnected[iIndex]=itEnding->second;
				//(iIndex ? curr.next : curr.prev)=itEnding->second;
				//find corresponding edge on the other edge
				int iEdge=(cut.endVertices[0]==lv.edge)?0 : 1;
				cut.aiConnected[iEdge]=iCut;
				//printf("\t-> %d\t-%d", itEnding->second, iEdge);
			}
			else
				//no fitting end: add as open to the map
				mOpenEnds.insert(std::make_pair(lv,iCut));
		}
	}

	}
	//find loops, this is essentially done for debug purposes and to find "island" loops

	//contains all loops that are completely inside only one triangle (no edges intersected)
	std::map<int, std::set<int> > amIslandLoops[2];

	std::vector<std::list<std::pair<int, int> > > vLoops;
	{
	std::vector<bool> vbChecked(vEdgeChains.size(), false);
	//check every edge segment
	for(unsigned int iCut=0; iCut<vEdgeChains.size(); iCut++)
	{
		if(vbChecked[iCut])
			//already checked this one: abort
			continue;
		vbChecked[iCut]=true;//set checked flag
		//add as beginning of a loop, set open loop flag
		bool bLoopCLosed=false;
		std::list<std::pair<int, int> > lLoopVertices;

		//traverse edge chain
		int iBegin=iCut;
		int iPrev=iBegin, iNext;
		int iCurrDirection=0;
		int iCurr=vEdgeChains[iBegin].aiConnected[iCurrDirection];

		int aiTri[2]={vEdgeChains[iCut].aiTris[0],
				vEdgeChains[iCut].aiTris[1]};
		bool abSingleTri[2]={true, true};
		bool bInternalError=false;
		while(iCurr!=-1)
		{//traverse edge chain
			if(iCurr==iBegin)
			{//last element is the same as first->loop is closed
				bLoopCLosed=true;
				break;
			}
			if(vbChecked[iCurr])
			{//something strange must be happening here...
				printf("warning, encountered already checked segment during segment walking!\n");
				bInternalError=true;
				break;
			}
			vbChecked[iCurr]=true;//set checked flag
			lLoopVertices.push_back(std::make_pair(iPrev, iCurrDirection));

			//check if one of the tris is different from beginning
			for(int iMesh=0; iMesh<2; iMesh++)
			{
				if(aiTri[iMesh]!=vEdgeChains[iCurr].aiTris[iMesh])
					abSingleTri[iMesh]=false;
			}
			//find next edge
			int* pConn=vEdgeChains[iCurr].aiConnected;
			iCurrDirection=(pConn[0]==iPrev? 1 : 0);
			iNext=vEdgeChains[iCurr].aiConnected[iCurrDirection];
			//iterate to the next edge
			iPrev=iCurr;
			iCurr=iNext;
		}
		if(bInternalError)
			//continue and drop this loop
			continue;
		if(bLoopCLosed)
		{//if we found a closed loop, we're finished here
			//check if we also have an island loop
			vLoops.push_back(lLoopVertices);
			for(int iMesh=0; iMesh<2; iMesh++)
			{
				if(abSingleTri[iMesh])
				{
					std::map<int, std::set<int> >::iterator itIsland=amIslandLoops[iMesh].begin();
					if(itIsland!=amIslandLoops[iMesh].end())
						itIsland->second.insert(vLoops.size()-1);
					else
					{
						std::set<int> s;
						s.insert(vLoops.size()-1);
						amIslandLoops[iMesh][aiTri[iMesh]]=s;
					}
				}
			}
			continue;
		}
		//traverse in opposite direction
		//we won't find a closed loop here
		iPrev=iBegin;
		iCurrDirection=1;
		iCurr=vEdgeChains[iBegin].aiConnected[iCurrDirection];
		while(iCurr!=-1)
		{//traverse edge chain
			if(vbChecked[iCurr])
			{//something strange must be happening here...
				printf("warning, encountered already checked segment during segment walking!\n");
				bInternalError=true;
				break;
			}
			vbChecked[iCurr]=true;//set checked flag
			lLoopVertices.push_front(std::make_pair(iBegin, iCurrDirection));
			//find next edge
			int* pConn=vEdgeChains[iCurr].aiConnected;
			iCurrDirection=(pConn[0]==iPrev? 1 : 0);
			iNext=vEdgeChains[iCurr].aiConnected[iCurrDirection];
			//iterate to the next edge
			iPrev=iCurr;
			iCurr=iNext;
		}
		//set the first edge of the open "loop" as beginning
		vLoops.push_back(lLoopVertices);
	}

	}

	//delete all references to cut triangles from the neighboring triangle list
	//recursively find "islands" of triangles by traversing neighboring tris
	std::vector<std::set<unsigned int> > avTriIslands[2];
	std::vector<int> avVertexPatch[2];
	//corresponding to triangle islands
	std::vector<int> aviSidedness[2];
	std::vector<int> aviPatchBoundarySize[2];
	for(int iMesh=0; iMesh<2; iMesh++)
	{
		int iOtherMesh=iMesh ? 0 : 1;
		std::vector<std::pair<std::set<unsigned int>, bool> > vPatches=MeshTools::getTriPatches(avNeighboringTris[iMesh], amCutTris[iMesh]);

		for(unsigned int iPatch=0; iPatch<vPatches.size(); iPatch++)
		{
			bool bWasCut=vPatches[iPatch].second;
			const std::set<unsigned int>& currTriSet=vPatches[iPatch].first;
			avTriIslands[iMesh].push_back(currTriSet);
			//if the patch wasn't cut, we won't be naturally able to tell where it is in respect to the cut mesh
			//if these are set to non-zero values, it will be processed as positively sided
			int iSidedness=0, iBoundarySize=0;
			if(!bWasCut)
			{
				bool bPosSide=false;
				Ogre::Vector3 vOrigin=vvVertices[iMesh][viIndices[iMesh][*(currTriSet.begin())*3]];
				//the target will be the center of a mediocre triangle of the other mesh
				Ogre::Vector3 vTarget(0,0,0);
				for(int iVertex=0; iVertex<3; iVertex++)
					vTarget+=vvVertices[iOtherMesh][viIndices[iOtherMesh][(viIndices[iOtherMesh].size()/6)*3+iVertex]];
				vTarget*=0.3333333333f;

				Ogre::Ray ray=Ogre::Ray(vOrigin, vTarget-vOrigin);

				float fPosClosest=std::numeric_limits<float>::max(), fNegClosest=std::numeric_limits<float>::max();

				for(unsigned int iTriVert=0; iTriVert<viIndices[iOtherMesh].size(); iTriVert+=3)
				{
					Ogre::Vector3 v[3];
					for(int iVertex=0; iVertex<3; iVertex++)
						v[iVertex]=vvVertices[iOtherMesh][viIndices[iOtherMesh][iTriVert+iVertex]];

					const Ogre::Vector3 normal=Ogre::Math::calculateBasicFaceNormal(v[0], v[1], v[2]);

					std::pair<bool, Ogre::Real> poshit=Ogre::Math::intersects(ray, v[0], v[1], v[2], normal, true, false);
					std::pair<bool, Ogre::Real> neghit=Ogre::Math::intersects(ray, v[0], v[1], v[2], normal, false, true);
					if(poshit.first)
						fPosClosest=std::min(fPosClosest, poshit.second);
					if(neghit.first)
						fNegClosest=std::min(fNegClosest, neghit.second);
				}
				bool bOutside=fPosClosest<fNegClosest;
				bPosSide=(ops[iMesh]==OUTSIDE && bOutside) || (ops[iMesh]==INSIDE && !bOutside);
				if(bPosSide)
					iSidedness=iBoundarySize=1337;//kehe...
			}
			aviSidedness[iMesh].push_back(iSidedness);
			aviPatchBoundarySize[iMesh].push_back(iBoundarySize);
		}

		//create a lookup table vertex->patch
		avVertexPatch[iMesh]=std::vector<int>(vvVertices[iMesh].size(), -1);
		//iterate patches
		for(unsigned int iPatch=0; iPatch<avTriIslands[iMesh].size(); iPatch++)
			//iterate all tris of the patch
			for(std::set<unsigned int>::iterator itTris=avTriIslands[iMesh][iPatch].begin();
					itTris!=avTriIslands[iMesh][iPatch].end(); itTris++)
				//iterate every vertex
				for(int iVert=0; iVert<3; iVert++)
					avVertexPatch[iMesh][viMatchingVertices[iMesh][viIndices[iMesh][3*(*itTris)+iVert]]]=iPatch;
	}

	std::list<std::pair<int, int> > lReTriIndices[2];
	std::map<std::pair<int, int>, int> mReTriVertices[2];
	//iterate every cut tri
	for(int iMesh=0; iMesh<2; iMesh++)
	{
		//create vertex buffer for additional verts
		//iterate edge segments
		for(unsigned int iEdgeSegment=0; iEdgeSegment<vEdgeChains.size(); iEdgeSegment++)
		{
			for(int iVertex=0; iVertex<2; iVertex++)
			{
				//add to re-triangulation-vertices
				std::pair<int, int> segmentDesc=std::make_pair(iEdgeSegment, iVertex);
				if(mReTriVertices[iMesh].find(segmentDesc) != mReTriVertices[iMesh].end())
					//already added this one
					continue;
				int iCurrIndex=mReTriVertices[iMesh].size();
				//add to retri-verts
				mReTriVertices[iMesh][segmentDesc]=iCurrIndex;
				if(vEdgeChains[iEdgeSegment].aiMesh[iVertex]==iMesh)
				{//if this edge is part of our current mesh:
					//check what is connected to this edge
					int iConnEdge=vEdgeChains[iEdgeSegment].aiConnected[iVertex];
					if(iConnEdge==-1)
					//nothing connected
						continue;
					int iConnVertex=0;
					for(; iConnVertex<2; iConnVertex++)
					{//check whether link@iConnVertex leads back to our original segment vertex
						if(vEdgeChains[iConnEdge].aiConnected[iConnVertex]==(int)iEdgeSegment)
							break;
					}
					if(iConnVertex==2)
						//doesn't lead back...
						continue;
					int aaiVertexIndices[2][2];
					for(int i=0; i<2; i++)
					{//two passes
						//get vertex index in triangle
						int iModulo=vEdgeChains[iEdgeSegment].aiOrgEdges[iVertex]%3;
						//increment by one if in second pass (-2 if about to overflow)
						int iIncrement=i*(iModulo==2 ? -2 : 1);
						//extract vertex index
						aaiVertexIndices[0][i]=viIndices[iMesh][vEdgeChains[iEdgeSegment].aiOrgEdges[iVertex] + iIncrement];

						//do the same with that connected vertex
						iModulo=vEdgeChains[iConnEdge].aiOrgEdges[iConnVertex]%3;
						iIncrement=i*(iModulo==2 ? -2 : 1);
						aaiVertexIndices[1][i]=viIndices[iMesh][vEdgeChains[iConnEdge].aiOrgEdges[iConnVertex] + iIncrement];
					}
					//check if vertices are equal
					bool bCheckSucceded=false;
					if((aaiVertexIndices[0][0]==aaiVertexIndices[1][0] || aaiVertexIndices[0][0]==aaiVertexIndices[1][1]) &&
						(aaiVertexIndices[0][1]==aaiVertexIndices[1][0] || aaiVertexIndices[0][1]==aaiVertexIndices[1][1]))
						bCheckSucceded=true;

					if(bCheckSucceded)
						//this effectively merges the two edge vertices in the output buffer
						mReTriVertices[iMesh][std::make_pair(iConnEdge, iConnVertex)]=iCurrIndex;
				}

			}
		}
		//vertices and indices resulting from re-triangulation for this mesh

		for(std::map<unsigned int, std::set<unsigned int> >::iterator itCutTri=amCutTris[iMesh].begin();
			itCutTri!=amCutTris[iMesh].end(); itCutTri++)
		{
#ifdef VERBOSE
			printf("\n\nprocessing cut tri %d on mesh %d\n", itCutTri->first, iMesh);
#endif
			//sort cutting points on triangle edges, doubly linked across vertices

			//pair: first is index of vEdgeChains, second is vertex in the former
			std::map<float, std::pair<int, int> > mTriCuts;
			for(std::set<unsigned int>::iterator itEdgeChain=itCutTri->second.begin();
					itEdgeChain!=itCutTri->second.end(); itEdgeChain++)
			{
				const SCut& cut=vEdgeChains[*itEdgeChain];
#ifdef VERBOSE
				printf("cut by edge chain %d\n", *itEdgeChain);
#endif
				for(int iVertex=0; iVertex<2; iVertex++)
				{
					if(iMesh!=cut.aiMesh[iVertex])
						continue;
					//extract edge and relative coordinate
					int iEdge=cut.aiOrgEdges[iVertex]%3;
					float fRelCoord=cut.fRelCoord[iVertex];
#ifdef VERBOSE
					printf("vertex %d cutting tri @ %f\n", iVertex, (float)iEdge+fRelCoord);
#endif
					mTriCuts[(float)iEdge+fRelCoord]=
						std::make_pair(*itEdgeChain, iVertex);
				}
#ifdef VERBOSE
				printf("\n");
#endif
			}
#ifdef VERBOSE
			printf("generating segments on tri edges...\n");
#endif
			//put them into a list
			std::list<std::pair<int, int> > lSegments;
			//this buffer holds the relation between triangle vertices and segments
			std::pair<int, int> vertexSegments[3];
			int iCurrSegment=0;
			for(std::map<float, std::pair<int, int> >::iterator itSegment=mTriCuts.begin();
				itSegment!=mTriCuts.end(); itSegment++)
			{
#ifdef VERBOSE
				printf("edge coord %f, segment vertex (%d, %d)", itSegment->first,
						itSegment->second.first, itSegment->second.second);
#endif
				while(itSegment->first>iCurrSegment)
				{
#ifdef VERBOSE
					printf(" located after vertex %d", iCurrSegment);
#endif
					vertexSegments[iCurrSegment++]=itSegment->second;
				}
#ifdef VERBOSE
				printf("\n");
#endif
				lSegments.push_back(itSegment->second);
			}
			for(;iCurrSegment<3; iCurrSegment++)
				vertexSegments[iCurrSegment]=vertexSegments[0];

			//create a polygon
#ifdef VERBOSE
			printf("\ngenerating polygon(s)...\n");
#endif
			std::list<std::list<std::pair<int, int> > > lPolys;
			//while there are cutting points in the list
			std::set<std::pair<int, int> > sitSegmentsUsed;
			for(std::list<std::pair<int, int> >::const_iterator itStartSegment=lSegments.begin();
					itStartSegment!=lSegments.end(); itStartSegment++)
			{
				//already had that poly
				if(sitSegmentsUsed.find(*itStartSegment)!=sitSegmentsUsed.end())
					continue;
#ifdef VERBOSE
				printf("starting @ segment vertex (%d, %d)\n", itStartSegment->first, itStartSegment->second);
#endif

				sitSegmentsUsed.insert(*itStartSegment);
				//pick a segment on the tri edge
				std::list<std::pair<int, int> >::const_iterator itCurrSegment=itStartSegment;

				//while we have not reached the beginning
				std::list<bool> lbSideCheck;
				std::list<std::pair<int, int> > lPolyVerts;


				bool bLoopIncomplete=false;
				do
				{
#ifdef VERBOSE
					printf("current segment vertex (%d, %d)\n", itCurrSegment->first, itCurrSegment->second);
#endif
					lPolyVerts.push_back(*itCurrSegment);
					//follow the cut line on the "positive" direction on the tri edge
					const SCut& cut=vEdgeChains[itCurrSegment->first];
					std::pair<int, int> targetVert;
					//figure out where we have to go in this line:
					//options: another edge of our current tri cuts the same tri on the other mesh
					int iOtherVert=itCurrSegment->second ? 0 : 1;
					if(cut.aiMesh[iOtherVert]==iMesh)
					{
						//in this case, the cut has to be contained in the same cut struct
						targetVert=*itCurrSegment;
						targetVert.second=iOtherVert;
#ifdef VERBOSE
						printf("other segment vertex (%d) cuts again\n", iOtherVert);
#endif
					}
					else
					{
#ifdef VERBOSE
						printf("walking cut segments...\n");
#endif
						//other case: the other edge in the cut structure points us to a foreign tri.
						//that tri in return cuts our tri edge.
						//use the connected cuts list to get to that edge segment
						int iCurrEdgeSegment=cut.aiConnected[iOtherVert];
						int iLastEdgeSegment=itCurrSegment->first;

						//iterate edge segments using connected fields by taking the link
						//that does not point back to our last segment
						int iVertex;
						//keep track of segments we were before to prevent endless iterations on bad input
						std::set<int> siChecked;
						do
						{
							if(siChecked.find(iCurrEdgeSegment)!=siChecked.end())
							{
								printf("\nwarning: tri being cut by circular chain, dropping poly\n");
								bLoopIncomplete=true;
								break;
							}
							siChecked.insert(iCurrEdgeSegment);
#ifdef VERBOSE
							printf("from segment %d", iCurrEdgeSegment);
#endif
							if(iCurrEdgeSegment==-1)
							{//the edge chain ends "mid-air", can't create a loop
								bLoopIncomplete=true;
								printf("\nwarning: tri being cut by incomplete chain, dropping poly\n");
								break;
							}

							for(iVertex=0; iVertex<2; iVertex++)
							{
								int iNextSegment=vEdgeChains[iCurrEdgeSegment].aiConnected[iVertex];
								if(iNextSegment!=iLastEdgeSegment)
								{
									//we found the correct direction
									iLastEdgeSegment=iCurrEdgeSegment;
									iCurrEdgeSegment=iNextSegment;
									break;
								}
							}
#ifdef VERBOSE
							printf(" next segment is %d, via vertex %d\n", iCurrEdgeSegment, iVertex);
							printf("pushing last vertex (%d, %d)\n", iLastEdgeSegment, iVertex==1 ? 0 : 1);
#endif
							lPolyVerts.push_back(std::make_pair(iLastEdgeSegment, iVertex==1 ? 0 : 1));
							//cutting our mesh again
							//by definition this has to be a cut with the current tri
						}while(vEdgeChains[iLastEdgeSegment].aiMesh[iVertex]!=iMesh);
						if(bLoopIncomplete)
							break;
						targetVert=std::make_pair(iLastEdgeSegment, iVertex);
					}
					if(bLoopIncomplete)
					{
						bError=true;
						break;
					}
					lPolyVerts.push_back(targetVert);
#ifdef VERBOSE
					printf("end of edge chain @ segment vertex (%d, %d)\n", targetVert.first, targetVert.second);
#endif
					//find the segment belonging to this cut
					std::list<std::pair<int, int> >::iterator itEdgeSegment=lSegments.begin();
					//iterate segments list
					//check for the matching cut structure
					while((*itEdgeSegment)!=targetVert && itEdgeSegment!=lSegments.end())
						itEdgeSegment++;
					if(itEdgeSegment==lSegments.end())
					{
						bLoopIncomplete=true;
						bError=true;
						break;
					}
					assert(itEdgeSegment!=lSegments.end());

					//we have found the segment in front of the the one we want
					//increase the iterator, wrapping around if necessary
					itEdgeSegment++;
					if(itEdgeSegment==lSegments.end())
						itEdgeSegment=lSegments.begin();

#ifdef VERBOSE
					printf("next polygon vertex @ segment vertex (%d, %d)\n", itEdgeSegment->first, itEdgeSegment->second);
#endif
					//perform a check of the volumetric side we are on:
					//take first segment of the cut line, yielding the vertex this edge begins with
					int iCurrMeshEdge=vEdgeChains[itCurrSegment->first].aiOrgEdges[itCurrSegment->second];
					//get triangle which belongs to cut on edge
					int iOtherMesh=iMesh ? 0 : 1;
					int iForeignTri=vEdgeChains[itCurrSegment->first].aiTris[iOtherMesh];
					//perform side check for the first vertex of tri edge segment
					Ogre::Vector3 vTest=vvVertices[iMesh][viIndices[iMesh][iCurrMeshEdge]];
					Ogre::Vector3 vBase=vvVertices[iOtherMesh][viIndices[iOtherMesh][iForeignTri*3]];
					Ogre::Vector3 vNormal=(vvVertices[iOtherMesh][viIndices[iOtherMesh][iForeignTri*3+1]]-vBase).crossProduct(
							vvVertices[iOtherMesh][viIndices[iOtherMesh][iForeignTri*3+2]]-vBase);
					float d=(vTest-vBase).dotProduct(vNormal);
					//positive side means being outside of the other mesh
					//write result to a buffer for all cut lines of the current poly

					bool bPosSide=d<0.0f;
					if(ops[iMesh]!=INSIDE)
						bPosSide=!bPosSide;
					lbSideCheck.push_back(bPosSide);

#ifdef VERBOSE
					printf("segment is on");
					printf(bPosSide ? " positive" : " negative");
					printf(" side\n");
#endif
					//check if there are triangle vertices trapped in the segment

					//calc current edge segment position on tri edges
					//start point is needed so that vertices are added in order
					int iStart=(vEdgeChains[itEdgeSegment->first].aiOrgEdges[itEdgeSegment->second]+1)%3;

					//check all vertices
					for(int iVertex=iStart; iVertex<iStart+3; iVertex++)
					{
						int i=iVertex%3;
						//if a triangle vertex is part of the current polygon
						if(vertexSegments[i]==*itEdgeSegment)
						{
							//set sided-ness of the corresponding patch boundary buffer
							unsigned int iVertexIndex=viIndices[iMesh][cut.aiTris[iMesh]*3+i];

							//lPolyVerts.push_back(std::make_pair(-1, iVertexIndex));
							lPolyVerts.push_back(std::make_pair(-1, cut.aiTris[iMesh]*3+i));
	#ifdef VERBOSE
								//printf("vertex %d", viMatchingVertices[iMesh][iVertexIndex]);
								//printf(bPosSide ? " positive" : " negative");
								//printf("\n");

								printf("adding trapped vertex %d as segment vertex (%d, %d)\n", i, -1, iVertexIndex);
	#endif

							unsigned int iLookedUpVertexIndex=viMatchingVertices[iMesh][iVertexIndex];
							int iPatch=avVertexPatch[iMesh][iLookedUpVertexIndex];
							if(iPatch<0)
							//patch-less vertex, can be vertices inside of intersection-loops, which get isolated
								continue;
							aviSidedness[iMesh][iPatch]+=bPosSide ? 1 : 0;
							aviPatchBoundarySize[iMesh][iPatch]++;
						}
					}
					itCurrSegment=itEdgeSegment;
					if(itStartSegment!=itCurrSegment && sitSegmentsUsed.find(*itCurrSegment)!=sitSegmentsUsed.end())
					{
						printf("error: circular connected segments detected during segment walking!\n");
						bError=true;
						break;
					}
					sitSegmentsUsed.insert(*itCurrSegment);
				}while(itStartSegment!=itCurrSegment);

				//check if the polygon is on the "keep" side
				//iterate the side-check buffer
				unsigned int nPosSide=0;
				for(std::list<bool>::const_iterator itSides=lbSideCheck.begin();
						itSides!=lbSideCheck.end(); itSides++)
					//count sided-ness (whatsoever)
					if(*itSides)
						nPosSide++;

				bool bPosSide=false;
				//decide by majority, throw a warning if it is no 100:0 decision
				if(nPosSide>(lbSideCheck.size()>>1))
					bPosSide=true;
				if(nPosSide!=0 && nPosSide!=lbSideCheck.size())
				{
					printf("warning: flipped tris detected\n");
					bError=true;
				}
				//add this polygons to the triangulation list
				if(bPosSide)
					lPolys.push_back(lPolyVerts);
#ifdef VERBOSE
				printf("checking sidedness... %sing polygon\n\n", bPosSide ? "keep" : "discard");
#endif
			}
			//create a polygon buffer containing all polygons on the "keep" side
			//which have cut the triangle edges and also all loops contained within the triangle
			std::vector<SVertex> vPolyVertices;
			std::vector<std::vector<unsigned int> > viTriangulationIndices;
			//std::vector<unsigned int> viVertexLookUp;
			std::vector<std::pair<int, int> > vVertexLookUp;

			//coords of tri
			int iTri=itCutTri->first;
			Ogre::Vector3 u=vvVertices[iMesh][viIndices[iMesh][iTri*3]];
			Ogre::Vector3 vEdge1=vvVertices[iMesh][viIndices[iMesh][iTri*3+1]]-u;
			Ogre::Vector3 vEdge2=vvVertices[iMesh][viIndices[iMesh][iTri*3+2]]-u;
			//iterate all polys
			for(std::list<std::list<std::pair<int, int> > >::const_iterator itPoly=lPolys.begin();
					itPoly!=lPolys.end(); itPoly++)
			{
				std::vector<unsigned int> viCurrPolyIndices;
				//iterate all verts
				for(std::list<std::pair<int, int> >::const_iterator itVert=itPoly->begin(); itVert!=itPoly->end(); itVert++)
				{
					//transform coords into 2D space
					Ogre::Vector3 vPoint;
					if(itVert->first==-1)
						vPoint=vvVertices[iMesh][viIndices[iMesh][itVert->second]]-u;
					else
						vPoint=vEdgeChains[itVert->first].v[itVert->second]-u;
					Ogre::Vector2 vCoords=GeometricTools::getUVCoords(vEdge1, vEdge2, vPoint);
					viCurrPolyIndices.push_back(vPolyVertices.size());
					vPolyVertices.push_back(SVertex(vCoords.x, vCoords.y));
#ifdef VERBOSE
					printf("segment vertex(%d, %d) is @ (%f, %f)\n", itVert->first, itVert->second, vCoords.x, vCoords.y);
#endif
					if(itVert->first==-1)
						//viVertexLookUp.push_back(itVert->second);
						vVertexLookUp.push_back(std::make_pair(itVert->second/3, itVert->second%3));
					else
					{
						std::map<std::pair<int, int>, int>::iterator itReTri=mReTriVertices[iMesh].find(*itVert);
						int iReTriIndex;
						if(itReTri==mReTriVertices[iMesh].end())
						{
							printf("error: vertex not found during retriangulation!\n");
							bError=true;
							continue;
						}
						iReTriIndex=itReTri->second;
						//save a lookup triangulation vertex index->mesh vertex index
						//viVertexLookUp.push_back(iReTriIndex+nMeshVerts);
						vVertexLookUp.push_back(std::make_pair(-1, iReTriIndex));
					}
				}
				viTriangulationIndices.push_back(viCurrPolyIndices);
			}
			//search for loops contained inside the current tri
			std::map<int, std::set<int> >::const_iterator itIslandLoops=amIslandLoops[iMesh].find(itCutTri->first);
			if(itIslandLoops!=amIslandLoops[iMesh].end())
			{
				for(std::set<int>::const_iterator itLoop=itIslandLoops->second.begin();
						itLoop!=itIslandLoops->second.end(); itLoop++)
				{
					std::vector<unsigned int> viCurrPolyIndices;
					for(std::list<std::pair<int, int> >::const_iterator itLoopVertex=vLoops[*itLoop].begin();
							itLoopVertex!=vLoops[*itLoop].end(); itLoopVertex++)
					{
						Ogre::Vector3 vPoint=vEdgeChains[itLoopVertex->first].v[itLoopVertex->second]-u;
						Ogre::Vector2 vCoords=GeometricTools::getUVCoords(vEdge1, vEdge2, vPoint);
						//save a lookup triangulation vertex index->mesh vertex index
						viCurrPolyIndices.push_back(vPolyVertices.size());
						vPolyVertices.push_back(SVertex(vCoords.x, vCoords.y));
						std::map<std::pair<int, int>, int>::iterator itReTri=mReTriVertices[iMesh].find(*itLoopVertex);
						int iReTriIndex;
						if(itReTri==mReTriVertices[iMesh].end())
						{
							printf("error: vertex not found during retriangulation!");
							bError=true;
							continue;
						}
						iReTriIndex=itReTri->second;
						//save a lookup triangulation vertex index->mesh vertex index
						//viVertexLookUp.push_back(iReTriIndex+nMeshVerts);
						vVertexLookUp.push_back(std::make_pair(-1, iReTriIndex));
					}
					viTriangulationIndices.push_back(viCurrPolyIndices);
				}
			}
			//triangulate polygons
			std::string strErr;
			//round a bit, especially values close to zero are a major issue for precision
			for(unsigned int iPolyVert=0; iPolyVert<vPolyVertices.size(); iPolyVert++)
			{
				if(vPolyVertices[iPolyVert].x<1e-5 && vPolyVertices[iPolyVert].x>-1e-5)
					vPolyVertices[iPolyVert].x=0;
				if(vPolyVertices[iPolyVert].y<1e-5 && vPolyVertices[iPolyVert].y>-1e-5)
					vPolyVertices[iPolyVert].y=0;
			}
			std::vector<unsigned int> viTriangulatedIndices=
					TriangulatePolygons(vPolyVertices, viTriangulationIndices, strErr);
			if(strErr.size())
			{
				printf(("Triangulation error: " + strErr+"\n").c_str());
				bError=true;
			}
			for(unsigned int iTriangulatedIndex=0; iTriangulatedIndex<viTriangulatedIndices.size(); iTriangulatedIndex++)
				//liRetriangulationTris.push_back(vPolyEdges[viTriangulatedIndices[iTriangulatedIndex]]);
			{
				unsigned int iVertexIndex=viTriangulatedIndices[iTriangulatedIndex];
				//if(iVertexIndex>=viVertexLookUp.size())
				if(iVertexIndex>=vVertexLookUp.size())
				{
					printf("error: vertex look up after triangulation failed\n");
					bError=true;
					continue;
				}
				//liReTriIndices[iMesh].push_back(viVertexLookUp[iVertexIndex]);
				lReTriIndices[iMesh].push_back(vVertexLookUp[iVertexIndex]);
			}
		}

	}

	//generate output mesh

	Ogre::v1::MeshPtr pOutMeshes[2];

	for(int iMesh=0; iMesh<2; iMesh++)
	{
		std::vector<std::pair<int, Ogre::Vector2> > vAppendedVertices=std::vector<std::pair<int, Ogre::Vector2> >(mReTriVertices[iMesh].size());
		std::vector<std::pair<int, int> > viOutputIndices;

		//re-used vertices
		for(unsigned int iPatch=0; iPatch<aviSidedness[iMesh].size(); iPatch++)
		{
			bool bPosSide=false;
			int nPosSide=aviSidedness[iMesh][iPatch];
			int iSize=aviPatchBoundarySize[iMesh][iPatch];
			//decide by majority, throw a warning if it is no 100:0 decision
			if(nPosSide>(iSize>>1))
				bPosSide=true;
			if(nPosSide!=0 && nPosSide!=iSize)
				printf("warning: patch sidedness could not be determined\n");
			//add this patch to the triangulation list
			if(!bPosSide)
			//we don't keep this patch
				continue;
			//copy all tris of the patch, provided it is on the "keep" side
			for(std::set<unsigned int>::const_iterator it=avTriIslands[iMesh][iPatch].begin();
							it!=avTriIslands[iMesh][iPatch].end(); it++)
			{
				int iTri=*it;
				for(int iTriVert=0; iTriVert<3; iTriVert++)
					viOutputIndices.push_back(std::make_pair(iTri, iTriVert));
			}
		}

		//retriangulated polygons
		for(std::map<std::pair<int, int>, int>::const_iterator it=mReTriVertices[iMesh].begin();
				it!=mReTriVertices[iMesh].end(); it++)
		{
			std::pair<int, Ogre::Vector2> targetVert;
			targetVert.first=vEdgeChains[it->first.first].aiTris[iMesh];

			//the vertex is defined on the cutting edge of a triangle, so we can't use the cut-UVs
			Ogre::Vector2 vUV=Ogre::Vector2(0.13, 0.37);//some magic init
			if(vEdgeChains[it->first.first].aiMesh[it->first.second]==iMesh)
			{
				//get uv by looking at relative cutting position
				float fRelCoord=vEdgeChains[it->first.first].fRelCoord[it->first.second];
				//switch by cutting edge of the triangle
				switch(vEdgeChains[it->first.first].aiOrgEdges[it->first.second]%3)
				{
				case 0:
					vUV=Ogre::Vector2(fRelCoord, 0);
					break;
				case 1:
					vUV=Ogre::Vector2(1-fRelCoord, fRelCoord);
					break;
				case 2:
					vUV=Ogre::Vector2(0, 1-fRelCoord);
					break;
				default:
					break;
				}
				targetVert.second=vUV;
			}
			else
				targetVert.second=vEdgeChains[it->first.first].vUVs[it->first.second];

			vAppendedVertices[it->second]=targetVert;
		}

		for(std::list<std::pair<int, int> >::const_iterator it=lReTriIndices[iMesh].begin();
			it!=lReTriIndices[iMesh].end(); it++)
			viOutputIndices.push_back(*it);


		pOutMeshes[iMesh]=OgreMeshExtractor::RebuildMesh(meshes[iMesh], vAppendedVertices, viOutputIndices);

		//flip if we're doing a subtraction
		if(ops[iMesh]==INSIDE && ops[iMesh ? 0 : 1]==OUTSIDE)
			OgreMeshExtractor::FlipMesh(pOutMeshes[iMesh]);
	}

	Ogre::v1::MeshPtr output=OgreMeshExtractor::CombineMeshes(pOutMeshes[0], pOutMeshes[1]);

	//create combined index buffer
	if(debugReTri.get())
	{
	std::vector<Ogre::Vector3> vDebugMesh;

	for(int iMesh=0; iMesh<2; iMesh++)
	{
		//after re-triangulation has finished
		//iterate patch boundaries
		for(unsigned int iPatch=0; iPatch<aviSidedness[iMesh].size(); iPatch++)
		{
			bool bPosSide=false;
			int nPosSide=aviSidedness[iMesh][iPatch];
			int iSize=aviPatchBoundarySize[iMesh][iPatch];
			//decide by majority, throw a warning if it is no 100:0 decision
			if(nPosSide>(iSize>>1))
				bPosSide=true;
			if(nPosSide!=0 && nPosSide!=iSize)
				printf("warning: patch sidedness could not be determined\n");
			//add this patch to the triangulation list
			if(!bPosSide)
			//we don't keep this patch
				continue;
			//copy all tris of the patch, provided it is on the "keep" side
			for(std::set<unsigned int>::const_iterator it=avTriIslands[iMesh][iPatch].begin();
							it!=avTriIslands[iMesh][iPatch].end(); it++)
			{
				int iTri=*it;
				for(int iTriVert=0; iTriVert<3; iTriVert++)
				{
					int iIndex=viIndices[iMesh][iTri*3+iTriVert];
					vDebugMesh.push_back(vvVertices[iMesh][iIndex]);
				}
			}
		}

		std::vector<std::pair<int, int> > vVerts(mReTriVertices[iMesh].size());
		for(std::map<std::pair<int, int>, int>::const_iterator it=mReTriVertices[iMesh].begin();
				it!=mReTriVertices[iMesh].end(); it++)
			vVerts[it->second]=it->first;

		//for(std::list<int>::const_iterator it=liReTriIndices[iMesh].begin();
			//it!=liReTriIndices[iMesh].end(); it++)
		for(std::list<std::pair<int, int> >::const_iterator it=lReTriIndices[iMesh].begin();
			it!=lReTriIndices[iMesh].end(); it++)
		{
			Ogre::Vector3 v;
			if(it->first==-1)
			{
				std::pair<int, int> cutVert=vVerts[it->second];
				v=vEdgeChains[cutVert.first].v[cutVert.second];
			}
			else
				v=vvVertices[iMesh][viIndices[iMesh][it->first*3+it->second]];
			//build mesh tris
			vDebugMesh.push_back(v);
		}
	}

	Ogre::v1::MeshPtr meshptr=debugReTri;
	meshptr.get()->createSubMesh("Retriangulation");

	if(!vDebugMesh.size())
	{
		vDebugMesh.push_back(Ogre::Vector3(0,0,0));
		vDebugMesh.push_back(Ogre::Vector3(0,0,0));
		vDebugMesh.push_back(Ogre::Vector3(0,0,0));
	}

	int nVerts=vDebugMesh.size();

	Ogre::v1::VertexData* data = new Ogre::v1::VertexData(nullptr);
	meshptr.get()->getSubMesh(0)->vertexData[0] = data;
	meshptr.get()->getSubMesh(0)->operationType=Ogre::OperationType::OT_TRIANGLE_LIST;//Ogre::RenderOperation::OT_LINE_LIST;
	meshptr.get()->getSubMesh(0)->useSharedVertices=false;
	data->vertexCount = nVerts;
	Ogre::v1::VertexDeclaration* decl = data->vertexDeclaration;
	decl->addElement(0, 0, Ogre::VET_FLOAT3, Ogre::VES_POSITION);

	Ogre::v1::HardwareVertexBufferSharedPtr vbuf = Ogre::v1::HardwareBufferManager::getSingleton().createVertexBuffer(
			decl->getVertexSize(0), nVerts, Ogre::v1::HardwareBuffer::HBU_STATIC_WRITE_ONLY);

	float* afVertexData=(float*)vbuf->lock(Ogre::v1::HardwareBuffer::HBL_DISCARD);

	Ogre::Vector3 vMin(1.0e+10f, 1.0e+10f, 1.0e+10f), vMax(-1.0e+10f, -1.0e+10f, -1.0e+10f);

	unsigned int iVertex=0;
	for(; iVertex<vDebugMesh.size(); iVertex++)
	{
		int iOffset=iVertex*decl->getVertexSize(0)/sizeof(float);
		Ogre::Vector3 vPos=vDebugMesh[iVertex];
		afVertexData[iOffset+0]=vPos.x;
		afVertexData[iOffset+1]=vPos.y;
		afVertexData[iOffset+2]=vPos.z;
		if(vPos.x<vMin.x)
			vMin.x=vPos.x;
		if(vPos.y<vMin.y)
			vMin.y=vPos.y;
		if(vPos.z<vMin.z)
			vMin.z=vPos.z;

		if(vPos.x>vMax.x)
			vMax.x=vPos.x;
		if(vPos.y>vMax.y)
			vMax.y=vPos.y;
		if(vPos.z>vMax.z)
			vMax.z=vPos.z;
	}

	vbuf->unlock();

	Ogre::v1::VertexBufferBinding* bind = data->vertexBufferBinding;
	bind->setBinding(0, vbuf);

	Ogre::v1::HardwareIndexBufferSharedPtr ibuf = Ogre::v1::HardwareBufferManager::getSingleton().createIndexBuffer(
	Ogre::v1::HardwareIndexBuffer::IT_32BIT, nVerts, Ogre::v1::HardwareBuffer::HBU_STATIC_WRITE_ONLY);

	unsigned int* aiIndexBuf=(unsigned int*)ibuf->lock(Ogre::v1::HardwareBuffer::HBL_DISCARD);
	for(int iIndex=0; iIndex<nVerts; iIndex++)
		aiIndexBuf[iIndex]=iIndex;
	ibuf->unlock();

	meshptr.get()->getSubMesh(0)->indexData[0]->indexBuffer = ibuf;
	meshptr.get()->getSubMesh(0)->indexData[0]->indexCount = nVerts;
	meshptr.get()->getSubMesh(0)->indexData[0]->indexStart = 0;

	meshptr.get()->_setBounds(Ogre::AxisAlignedBox(vMin, vMax));

	meshptr.get()->load();
	}

	if(debugCutLine.get())
	{
	if(!lCuts.size())
	{
		lCuts.push_back(std::make_pair(0, Ogre::Vector3(0,0,0)));
		lCuts.push_back(std::make_pair(0, Ogre::Vector3(0,0,0)));
	}

	Ogre::v1::MeshPtr meshptr=debugCutLine;
	meshptr.get()->createSubMesh("Lines");

	int nVerts=lCuts.size();

	Ogre::v1::VertexData* data = new Ogre::v1::VertexData(nullptr);
	meshptr.get()->getSubMesh(0)->vertexData[0] = data;
	meshptr.get()->getSubMesh(0)->operationType = Ogre::OperationType::OT_LINE_LIST;
	meshptr.get()->getSubMesh(0)->useSharedVertices=false;
	data->vertexCount = nVerts;
	Ogre::v1::VertexDeclaration* decl = data->vertexDeclaration;
	decl->addElement(0, 0, Ogre::VET_FLOAT3, Ogre::VES_POSITION);

	Ogre::v1::HardwareVertexBufferSharedPtr vbuf = Ogre::v1::HardwareBufferManager::getSingleton().createVertexBuffer(
	decl->getVertexSize(0), nVerts, Ogre::v1::HardwareBuffer::HBU_STATIC_WRITE_ONLY);

	float* afVertexData=(float*)vbuf->lock(Ogre::v1::HardwareBuffer::HBL_DISCARD);

	Ogre::Vector3 vMin(1.0e+10f, 1.0e+10f, 1.0e+10f), vMax(-1.0e+10f, -1.0e+10f, -1.0e+10f);

	unsigned int iVertex=0;
	for (std::list<std::pair<int, Ogre::Vector3> >::iterator it=lCuts.begin(); it!=lCuts.end(); it++, iVertex++)
	{
		int iOffset=iVertex*decl->getVertexSize(0)/sizeof(float);
		//set position
		Ogre::Vector3 vPos=it->second;
		afVertexData[iOffset+0]=vPos.x;
		afVertexData[iOffset+1]=vPos.y;
		afVertexData[iOffset+2]=vPos.z;
		if(vPos.x<vMin.x)
			vMin.x=vPos.x;
		if(vPos.y<vMin.y)
			vMin.y=vPos.y;
		if(vPos.z<vMin.z)
			vMin.z=vPos.z;

		if(vPos.x>vMax.x)
			vMax.x=vPos.x;
		if(vPos.y>vMax.y)
			vMax.y=vPos.y;
		if(vPos.z>vMax.z)
			vMax.z=vPos.z;
	}

	vbuf->unlock();

	Ogre::v1::VertexBufferBinding* bind = data->vertexBufferBinding;
	bind->setBinding(0, vbuf);

	Ogre::v1::HardwareIndexBufferSharedPtr ibuf = Ogre::v1::HardwareBufferManager::getSingleton().createIndexBuffer(
	Ogre::v1::HardwareIndexBuffer::IT_32BIT, nVerts, Ogre::v1::HardwareBuffer::HBU_STATIC_WRITE_ONLY);

	unsigned int* aiIndexBuf=(unsigned int*)ibuf->lock(Ogre::v1::HardwareBuffer::HBL_DISCARD);
	for(int iIndex=0; iIndex<nVerts; iIndex++)
		aiIndexBuf[iIndex]=iIndex;
	ibuf->unlock();

	meshptr.get()->getSubMesh(0)->indexData[0]->indexBuffer = ibuf;
	meshptr.get()->getSubMesh(0)->indexData[0]->indexCount = nVerts;
	meshptr.get()->getSubMesh(0)->indexData[0]->indexStart = 0;

	meshptr.get()->_setBounds(Ogre::AxisAlignedBox(vMin, vMax));

	meshptr.get()->load();

	}
	return output;
}


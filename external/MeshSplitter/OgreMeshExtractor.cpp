/*
 * OgreMeshExtractor.cpp
 *
 *  Created on: Apr 30, 2011
 *      Author: _1nsane
 */

#include "OgreMeshExtractor.h"
#include "OgreMeshManager.h"

#define INDEX(pIndices, iIndex, iIndexSize, indexMask) ((*((unsigned int*)(((char*)pIndices)+iIndex*iIndexSize)))&indexMask)
#define VERTEX(pVertices, iVertSize, iPosOffset, iIndex) Ogre::Vector3(*((float*)(((char*)pVertices)+iIndex*iVertSize+iPosOffset)),\
								*(((float*)(((char*)pVertices)+iIndex*iVertSize+iPosOffset))+1),\
								*(((float*)(((char*)pVertices)+iIndex*iVertSize+iPosOffset))+2))

Ogre::String OgreMeshExtractor::getRandomMeshName()
{
	Ogre::String strMeshName;
	do
	{
		strMeshName = Ogre::String("CutMesh") + Ogre::StringConverter::toString(rand() & 0xffffff);
	} while (Ogre::v1::MeshManager::getSingleton().resourceExists(strMeshName));
	return strMeshName;
}

void
OgreMeshExtractor::Extract(Ogre::v1::MeshPtr mesh, std::vector<Ogre::Vector3>& viVertices, std::vector<unsigned int>& viSubmeshVertexOffsets,
	std::vector<unsigned int>& viIndices, std::vector<unsigned int>& viSubmeshIndexOffsets, bool bExtractIndicesOnly, unsigned int* pnVerts, unsigned int* pnIndices)
{
	//check sub-meshes for bad types..
	Ogre::v1::Mesh::SubMeshIterator it = mesh->getSubMeshIterator();
	while (it.hasMoreElements())
	{
		Ogre::v1::SubMesh* subMesh = it.getNext();
		assert(subMesh);
		assert(subMesh->operationType == Ogre::OperationType::OT_TRIANGLE_LIST);
	}

	//compile an overall vertex buffer
	std::map<Ogre::v1::VertexData*, int> mVertexDatas;
	it = mesh->getSubMeshIterator();
	unsigned int iCurrVertexOffset = 0, iCurrIndexOffset = 0;
	Ogre::v1::VertexData* pVertexData = 0;

	while (it.hasMoreElements())
	{
		Ogre::v1::SubMesh* subMesh = it.getNext();
		if (subMesh->useSharedVertices)
			//shared vertices
			pVertexData = mesh->sharedVertexData[0];
		else
			//use own vertex buffer
			pVertexData = subMesh->vertexData[0];
		viSubmeshVertexOffsets.push_back(iCurrVertexOffset);

		if (mVertexDatas.find(pVertexData) != mVertexDatas.end())
			//this vertexdata has already been processed, just set the offset
			viSubmeshVertexOffsets[viSubmeshVertexOffsets.size() - 1] = mVertexDatas[pVertexData];

		else
		{
			if (!bExtractIndicesOnly)
			{
				assert(pVertexData->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION));//mesh needs to have position data

				const Ogre::v1::VertexElement* pPosElem = pVertexData->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);
				int iPosOffset = pPosElem->getOffset();
				Ogre::v1::HardwareVertexBufferSharedPtr hwbufptr = pVertexData->vertexBufferBinding->getBuffer(pPosElem->getSource());
				unsigned int iStartVert = pVertexData->vertexStart;

				void* pVertices = hwbufptr->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY);

				int iVertSize = hwbufptr->getVertexSize();
				viVertices.resize(viVertices.size() + pVertexData->vertexCount);

				for (unsigned int iVertex = iStartVert; iVertex < iStartVert + pVertexData->vertexCount; iVertex++)
					viVertices[iCurrVertexOffset + iVertex] = VERTEX(pVertices, iVertSize, iPosOffset, iVertex);

				hwbufptr->unlock();
			}

			mVertexDatas[pVertexData] = viSubmeshVertexOffsets.size() - 1;
			iCurrVertexOffset += pVertexData->vertexCount;
		}
		//now the indices
		viSubmeshIndexOffsets.push_back(iCurrIndexOffset);
		Ogre::v1::IndexData* pIndexData = subMesh->indexData[0];

		{
			unsigned int iStartIndex = pIndexData->indexStart;

			viIndices.resize(iCurrIndexOffset + pIndexData->indexCount);

			void* pIndices = pIndexData->indexBuffer->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY);

			int iIndexSize = pIndexData->indexBuffer->getIndexSize();
			unsigned int indexMask = (iIndexSize ? 0xff : 0) | (iIndexSize > 1 ? 0xff00 : 0) |
				(iIndexSize > 2 ? 0xff0000 : 0) | (iIndexSize > 3 ? 0xff000000 : 0);

			for (unsigned int iIndex = iStartIndex; iIndex < (iStartIndex + pIndexData->indexCount); iIndex++)
			{
				viIndices[iCurrIndexOffset + iIndex] = INDEX(pIndices, iIndex, iIndexSize, indexMask) + viSubmeshVertexOffsets.back();
				assert(viIndices[iCurrIndexOffset + iIndex] < iCurrVertexOffset);
			}
			pIndexData->indexBuffer->unlock();
		}
		iCurrIndexOffset += pIndexData->indexCount;

	}
	if (pnIndices)
		*pnIndices = iCurrIndexOffset;
	if (pnVerts)
		*pnVerts = iCurrVertexOffset;
}

int getVertexIndex(const std::vector<unsigned int>& viExtractIndices, const std::pair<int, int>& tridesc, bool& bAppended)
{
	int iIndex;
	if (tridesc.first == -1)
	{
		bAppended = true;
		iIndex = tridesc.second;
	}
	else
	{
		bAppended = false;
		iIndex = viExtractIndices[tridesc.first * 3 + tridesc.second];
	}
	return iIndex;
}

int getSubMeshbyIndex(const std::vector<unsigned int>& viSubmeshIndexOffsets, unsigned int iIndexIndex)
{
	//subset search
	int iRangeMin = 0;
	int iRangeMax = viSubmeshIndexOffsets.size() - 2;
	while (iRangeMin != iRangeMax)
	{
		int iSamplePoint = (iRangeMax + iRangeMin) >> 1;
		if (iIndexIndex < viSubmeshIndexOffsets[iSamplePoint + 1])
			iRangeMax = iSamplePoint;
		else//>=
			iRangeMin = iSamplePoint + 1;
	}
	return iRangeMax;
}

struct SSubmeshData
{
	//the final vertex buffer will have the reused vertices in front of the appended ones (hence the name)
	//these are generated by getVertexIndex()
	std::vector<int> viReUsedVertexIndices;
	std::vector<int> viAppendedVertexIndices;
	//appended vertices are addressed by vertex index >= viReUsedVertexIndices.size()
	std::vector<int> viIndices;
};

template<class T>
void SetBuffer(void* pSrcBuffer, void* pDestBuffer, int iIndex, float fFactor)
{
	T fOrgValue = ((T*)pSrcBuffer)[iIndex];
	((T*)pDestBuffer)[iIndex] += (T)((float)fOrgValue*fFactor);
}

Ogre::v1::MeshPtr
OgreMeshExtractor::RebuildMesh(Ogre::v1::MeshPtr mesh, const std::vector<std::pair<int, Ogre::Vector2> >& vAppendedVertices,
	const std::vector<std::pair<int, int> >& viIndices)
{
	Ogre::v1::MeshPtr outMesh = Ogre::v1::MeshManager::getSingleton().createManual(OgreMeshExtractor::getRandomMeshName(), "General");

	//get parameters of mesh
	std::vector<unsigned int> viSubmeshVertexOffsets;
	std::vector<unsigned int> viSubmeshIndexOffsets;

	std::vector<unsigned int> viExtractIndices;
	unsigned int nOrgVerts, nOrgIndices;

	{
		std::vector<Ogre::Vector3> viExtractVertices;

		OgreMeshExtractor::Extract(mesh, viExtractVertices, viSubmeshVertexOffsets,
			viExtractIndices, viSubmeshIndexOffsets, true, &nOrgVerts, &nOrgIndices);
	}
	//push a dummy sub-mesh for simplicity
	viSubmeshVertexOffsets.push_back(nOrgVerts);
	viSubmeshIndexOffsets.push_back(nOrgIndices);

	//first check which vertices are actually addressed and create a mapping for the final vertex buffers

	//mapping of the given vertex buffer to the final buffer
	//pair(submesh, vertex-position)
	std::vector<std::pair<int, int> > viOrgVertexIndexMapping = std::vector<std::pair<int, int> >(nOrgVerts, std::make_pair(-1, -1));
	std::vector<std::pair<int, int> > viAppendedVertexIndexMapping = std::vector<std::pair<int, int> >(vAppendedVertices.size(), std::make_pair(-1, -1));

	std::vector<SSubmeshData> vSubmeshData = std::vector<SSubmeshData>(viSubmeshVertexOffsets.size());

	//create vertex buffers
	{
		//iterate index buffer
		std::set<unsigned int> siOrgUsedVerts, siAppendedUsedVerts;
		for (unsigned int iIndex = 0; iIndex < viIndices.size(); iIndex++)
		{
			bool bAppended;
			int iVertexIndex = getVertexIndex(viExtractIndices, viIndices[iIndex], bAppended);
			if (bAppended)
				siAppendedUsedVerts.insert(iVertexIndex);
			else
				siOrgUsedVerts.insert(iVertexIndex);
		}

		std::vector<int> viCurrVertices = std::vector<int>(viSubmeshVertexOffsets.size(), 0);

		unsigned int iSubMesh = 0;

		for (std::set<unsigned int>::const_iterator it = siOrgUsedVerts.begin(); it != siOrgUsedVerts.end(); it++)
		{
			//figure out which sub-mesh this vertex belongs to
			while (*it >= viSubmeshVertexOffsets[iSubMesh + 1])
			{
				iSubMesh++;
				assert(iSubMesh < viSubmeshVertexOffsets.size() - 1);
			}

			if (mesh->getSubMesh(iSubMesh)->useSharedVertices)
			{//add for all sub-meshes using shared vertices
				for (int iCurrSubMesh = 0; iCurrSubMesh < mesh->getNumSubMeshes(); iCurrSubMesh++)
					if (mesh->getSubMesh(iCurrSubMesh)->useSharedVertices)
						vSubmeshData[iSubMesh].viReUsedVertexIndices.push_back(*it);
			}
			else
				vSubmeshData[iSubMesh].viReUsedVertexIndices.push_back(*it);

			viOrgVertexIndexMapping[*it] = std::make_pair(iSubMesh, viCurrVertices[iSubMesh]++);
		}

		for (std::set<unsigned int>::const_iterator it = siAppendedUsedVerts.begin(); it != siAppendedUsedVerts.end(); it++)
		{
			//figure out which sub-mesh this vertex belongs to
			int iTri = vAppendedVertices[*it].first;
			iSubMesh = getSubMeshbyIndex(viSubmeshIndexOffsets, iTri * 3);

			vSubmeshData[iSubMesh].viAppendedVertexIndices.push_back(*it);

			viAppendedVertexIndexMapping[*it] = std::make_pair(iSubMesh, viCurrVertices[iSubMesh]++);
		}
	}
	//next build index buffers
	//keep in mind: indices always belong to a certain sub-mesh. this binding has to be preserved
	//this binding is indicated by the triangle of the corresponding vertex

	{
		for (unsigned int iIndex = 0; iIndex < viIndices.size(); iIndex++)
		{
			bool bAppended;
			int iVertexIndex = getVertexIndex(viExtractIndices, viIndices[iIndex], bAppended);
			//int iSubMesh, iSubMeshVertexIndex;
			std::pair<int, int> subMeshVertexIndex;
			if (bAppended)
			{
				subMeshVertexIndex = viAppendedVertexIndexMapping[iVertexIndex];
				//subMeshVertexIndex.second+=vSubmeshData[subMeshVertexIndex.first].viReUsedVertexIndices.size();
			}
			else
				subMeshVertexIndex = viOrgVertexIndexMapping[iVertexIndex];

			vSubmeshData[subMeshVertexIndex.first].viIndices.push_back(subMeshVertexIndex.second);
		}
	}

	Ogre::Vector3 vMin(1.0e+10f, 1.0e+10f, 1.0e+10f), vMax(-1.0e+10f, -1.0e+10f, -1.0e+10f);

	//iterate sub-meshes
	Ogre::v1::Mesh::SubMeshIterator itSubMesh = mesh->getSubMeshIterator();
	int iCurrSubMesh = -1;
	Ogre::v1::VertexData* pOrgVertexData = 0;
	bool bWroteData = false;

	while (itSubMesh.hasMoreElements())
	{
		Ogre::v1::SubMesh* orgSubMesh = itSubMesh.getNext();
		iCurrSubMesh++;
		if (orgSubMesh->useSharedVertices)
			//shared vertices
			pOrgVertexData = mesh->sharedVertexData[0];
		else
			//use own vertex buffer
			pOrgVertexData = orgSubMesh->vertexData[0];

		//this sub-mesh is not used, so skip it
		if (!(vSubmeshData[iCurrSubMesh].viAppendedVertexIndices.size() || vSubmeshData[iCurrSubMesh].viReUsedVertexIndices.size()))
			continue;

		bWroteData = true;

		//create destination sub-mesh
		Ogre::v1::SubMesh* destSubMesh = outMesh.get()->createSubMesh();

		//write vertex buffers taking into account the vertex format of the original vertex buffers
		//first check if the first sub-mesh refers to shared vertices

		//no shared verts and submesh 0
		//if(!orgSubMesh->useSharedVertices && !iCurrSubMesh)
		{
			Ogre::v1::VertexData* destVertexData = new Ogre::v1::VertexData(nullptr);

			destSubMesh->vertexData[0] = destVertexData;
			destSubMesh->operationType = Ogre::OperationType::OT_TRIANGLE_LIST;
			destSubMesh->useSharedVertices = false;
			destSubMesh->setMaterialName(orgSubMesh->getMaterialName());

			unsigned int nVerts = vSubmeshData[iCurrSubMesh].viAppendedVertexIndices.size() + vSubmeshData[iCurrSubMesh].viReUsedVertexIndices.size();
			destVertexData->vertexCount = nVerts;
			destVertexData->vertexStart = 0;

			Ogre::v1::VertexDeclaration* orgdecl = pOrgVertexData->vertexDeclaration;
			for (unsigned int iVertElem = 0; iVertElem < orgdecl->getElementCount(); iVertElem++)
			{
				const Ogre::v1::VertexElement* pVertElem = orgdecl->getElement(iVertElem);
				destVertexData->vertexDeclaration->addElement(0, pVertElem->getOffset(), pVertElem->getType(), pVertElem->getSemantic(), pVertElem->getIndex());
			}

			Ogre::v1::HardwareVertexBufferSharedPtr destVertBuffer = Ogre::v1::HardwareBufferManager::getSingleton().createVertexBuffer(
				destVertexData->vertexDeclaration->getVertexSize(0), nVerts, Ogre::v1::HardwareBuffer::HBU_STATIC);

			Ogre::v1::VertexBufferBinding* bind = destVertexData->vertexBufferBinding;
			bind->setBinding(0, destVertBuffer);

			int iDestVertexSize = destVertBuffer->getVertexSize();

			//lock dest buffer
			void* pDestVertexData = (void*)destVertBuffer->lock(Ogre::v1::HardwareBuffer::HBL_DISCARD);

			//lock source buffers
			std::vector<void*> vpVertexSourcePtrs;
			std::vector<Ogre::v1::HardwareVertexBufferSharedPtr> vpVertexSources;

			for (unsigned int iSource = 0; iSource < pOrgVertexData->vertexBufferBinding->getBufferCount(); iSource++)
			{
				vpVertexSources.push_back(pOrgVertexData->vertexBufferBinding->getBuffer(iSource));
				vpVertexSourcePtrs.push_back(vpVertexSources[iSource]->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));
			}

			//re-used vertices
			//these are simply copied over to the new sub-mesh
			unsigned int iVertex = 0;
			for (; iVertex < vSubmeshData[iCurrSubMesh].viReUsedVertexIndices.size(); iVertex++)
			{
				Ogre::Vector3 v;
				int iSourceVertexIndex = vSubmeshData[iCurrSubMesh].viReUsedVertexIndices[iVertex] - viSubmeshVertexOffsets[iCurrSubMesh];
				//iterate vertex declaration
				for (unsigned int iVertElem = 0; iVertElem < orgdecl->getElementCount(); iVertElem++)
				{
					const Ogre::v1::VertexElement* pVertElem = orgdecl->getElement(iVertElem);

					int iOrgPosOffset = pVertElem->getOffset();
					int iOrgSource = pVertElem->getSource();
					int iSourceVertSize = vpVertexSources[iOrgSource]->getVertexSize();
					int iDestPosOffset = destVertexData->vertexDeclaration->getElement(iVertElem)->getOffset();
					void* pSourceVertexData = vpVertexSourcePtrs[iOrgSource];

					//Ogre::VertexElementType type=pVertElem->getType();

					if (pVertElem->getSemantic() == Ogre::VES_POSITION)
						v = VERTEX(pSourceVertexData, iSourceVertSize, iOrgPosOffset, iSourceVertexIndex);

					int typeSize = pVertElem->getSize();

					void* pDest = (void*)(((char*)pDestVertexData) + iVertex*iDestVertexSize + iDestPosOffset);
					void* pSource = (void*)(((char*)pSourceVertexData) + iSourceVertexIndex*iSourceVertSize + iOrgPosOffset);
					memcpy(pDest, pSource, typeSize);
				}
				vMin.makeFloor(v);
				vMax.makeCeil(v);
			}

			//appended vertices
			//they have to be interpolated according to position in parent triangle

			int nReUsed = vSubmeshData[iCurrSubMesh].viReUsedVertexIndices.size();
			for (; iVertex < nVerts; iVertex++)
			{
				Ogre::Vector3 v;

				int iAppendedVertexIndex = vSubmeshData[iCurrSubMesh].viAppendedVertexIndices[iVertex - nReUsed];
				//iAppendedVertexIndex-=viExtractIndices.size();
				//int iSourceVertexIndex=vAppendedVertices[iSourceVertexIndex];
				std::pair<int, Ogre::Vector2> sourceVertex = vAppendedVertices[iAppendedVertexIndex];
				int iSourceTri = sourceVertex.first;
				Ogre::Vector2 vUV = sourceVertex.second;

				//iterate vertex declaration
				for (unsigned int iVertElem = 0; iVertElem < orgdecl->getElementCount(); iVertElem++)
				{
					const Ogre::v1::VertexElement* pVertElem = orgdecl->getElement(iVertElem);

					int iOrgPosOffset = pVertElem->getOffset();
					int iOrgSource = pVertElem->getSource();
					int iSourceVertSize = vpVertexSources[iOrgSource]->getVertexSize();
					int iDestPosOffset = destVertexData->vertexDeclaration->getElement(iVertElem)->getOffset();
					void* pSourceVertexData = vpVertexSourcePtrs[iOrgSource];
					int typeSize = pVertElem->getSize();

					void* pBuf = new char[typeSize];
					//clear buffer
					memset(pBuf, 0, typeSize);
					Ogre::VertexElementType type = pVertElem->getType();

					for (int iTriVertex = 0; iTriVertex < 3; iTriVertex++)
					{
						int iIndex = iSourceTri * 3 + iTriVertex;
						int iSourceVertexIndex = viExtractIndices[iIndex] - viSubmeshVertexOffsets[iCurrSubMesh];

						void* pSource = (void*)(((char*)pSourceVertexData) + iSourceVertexIndex*iSourceVertSize + iOrgPosOffset);

						//v=v0+u(v1-v0)+v(v2-v0)=v0+u*v1+v*v2-(u+v)v0

						float fFactor;
						switch (iTriVertex)
						{
						case 0:
							fFactor = 1.0f - (vUV.x + vUV.y);
							break;
						case 1:
							fFactor = vUV.x;
							break;
						case 2:
							fFactor = vUV.y;
							break;
						default:
							//wtf?
							break;
						}

						switch (type)
						{
						case Ogre::VET_FLOAT1:
						case Ogre::VET_FLOAT2:
						case Ogre::VET_FLOAT3:
						case Ogre::VET_FLOAT4:
							for (unsigned int iFloat = 0; iFloat < typeSize / sizeof(float); iFloat++)
								SetBuffer<float>(pSource, pBuf, iFloat, fFactor);
							break;
							// Attention VET_SHORT1, VET_SHORT3 cannot be used anymore, check for crash!
													// case Ogre::VET_SHORT1:
						case Ogre::VET_SHORT2:
							// case Ogre::VET_SHORT3:
						case Ogre::VET_SHORT4:
							for (unsigned int iShort = 0; iShort < typeSize / sizeof(short); iShort++)
								SetBuffer<short>(pSource, pBuf, iShort, fFactor);
							break;
						case Ogre::VET_UBYTE4:
							//color types are also ubyte4
						case Ogre::VET_COLOUR:
						case Ogre::VET_COLOUR_ARGB:
						case Ogre::VET_COLOUR_ABGR:
							for (unsigned int iByte = 0; iByte < 4; iByte++)
								SetBuffer<unsigned char>(pSource, pBuf, iByte, fFactor);
							break;
						default:
							break;
						}
					}
					void* pDest = (void*)(((char*)pDestVertexData) + iVertex*iDestVertexSize + iDestPosOffset);
					memcpy(pDest, pBuf, typeSize);

					if (pVertElem->getSemantic() == Ogre::VES_POSITION)
						v = VERTEX(pBuf, 0, 0, 0);
				}

				vMin.makeFloor(v);
				vMax.makeCeil(v);
			}

			destVertBuffer->unlock();

			for (unsigned int iSource = 0; iSource < pOrgVertexData->vertexBufferBinding->getBufferCount(); iSource++)
				vpVertexSources[iSource]->unlock();

			//writing the index buffers is quite straightforward
			int nIndices = vSubmeshData[iCurrSubMesh].viIndices.size();
			assert(nIndices % 3 == 0);

			Ogre::v1::HardwareIndexBufferSharedPtr ibuf = Ogre::v1::HardwareBufferManager::getSingleton().createIndexBuffer(
				/*nVerts<(1<<16) ? Ogre::v1::HardwareIndexBuffer::IT_16BIT : */Ogre::v1::HardwareIndexBuffer::IT_32BIT, nIndices, Ogre::v1::HardwareBuffer::HBU_STATIC);

			destSubMesh->indexData[0]->indexBuffer = ibuf;
			destSubMesh->indexData[0]->indexCount = nIndices;
			destSubMesh->indexData[0]->indexStart = 0;

			if (ibuf->getIndexSize() == 2)
			{
				unsigned short* aiIndexBuf = (unsigned short*)ibuf->lock(Ogre::v1::HardwareBuffer::HBL_DISCARD);
				for (int iIndex = 0; iIndex < nIndices; iIndex++)
					aiIndexBuf[iIndex] = (unsigned short)vSubmeshData[iCurrSubMesh].viIndices[iIndex];
			}
			else
			{
				unsigned int* aiIndexBuf = (unsigned int*)ibuf->lock(Ogre::v1::HardwareBuffer::HBL_DISCARD);
				for (int iIndex = 0; iIndex < nIndices; iIndex++)
					aiIndexBuf[iIndex] = vSubmeshData[iCurrSubMesh].viIndices[iIndex];
			}
			ibuf->unlock();
		}
	}

	if (!bWroteData)
		return Ogre::v1::MeshPtr();

	outMesh->_setBounds(Ogre::AxisAlignedBox(vMin, vMax));
	//outMesh->load();
	return outMesh;
}

Ogre::v1::MeshPtr OgreMeshExtractor::CombineMeshes(Ogre::v1::MeshPtr mesh1, Ogre::v1::MeshPtr mesh2)
{
	Ogre::v1::MeshPtr outMesh = Ogre::v1::MeshManager::getSingleton().createManual(OgreMeshExtractor::getRandomMeshName(), "General");

	Ogre::Vector3 vMin(1.0e+10f, 1.0e+10f, 1.0e+10f), vMax(-1.0e+10f, -1.0e+10f, -1.0e+10f);
	bool bWroteSome = false;

	for (int iMesh = 0; iMesh < 2; iMesh++)
	{
		Ogre::v1::MeshPtr mesh = iMesh ? mesh2 : mesh1;
		if (mesh.get() == 0)//invalid mesh
			continue;

		bWroteSome = true;

		vMin.makeFloor(mesh->getBounds().getMinimum());
		vMax.makeCeil(mesh->getBounds().getMaximum());

		Ogre::v1::Mesh::SubMeshIterator itSubMesh = mesh->getSubMeshIterator();
		while (itSubMesh.hasMoreElements())
		{
			Ogre::v1::SubMesh* orgSubMesh = itSubMesh.getNext();
			assert(!orgSubMesh->useSharedVertices);

			//create destination sub-mesh
			Ogre::v1::SubMesh* destSubMesh = outMesh.get()->createSubMesh();


			Ogre::v1::VertexData* destVertexData = new Ogre::v1::VertexData(nullptr);

			destSubMesh->vertexData[0] = destVertexData;
			destSubMesh->operationType = Ogre::OperationType::OT_TRIANGLE_LIST;
			destSubMesh->useSharedVertices = false;
			destSubMesh->setMaterialName(orgSubMesh->getMaterialName());


			destVertexData->vertexCount = orgSubMesh->vertexData[0]->vertexCount;
			destVertexData->vertexStart = orgSubMesh->vertexData[0]->vertexStart;

			Ogre::v1::VertexDeclaration* orgdecl = orgSubMesh->vertexData[0]->vertexDeclaration;
			for (unsigned int iVertElem = 0; iVertElem < orgdecl->getElementCount(); iVertElem++)
			{
				const Ogre::v1::VertexElement* pVertElem = orgdecl->getElement(iVertElem);
				destVertexData->vertexDeclaration->addElement(pVertElem->getSource(), pVertElem->getOffset(), pVertElem->getType(), pVertElem->getSemantic(), pVertElem->getIndex());
			}

			Ogre::v1::VertexBufferBinding* bind = destVertexData->vertexBufferBinding;

			for (unsigned int iBuffer = 0; iBuffer < orgSubMesh->vertexData[0]->vertexBufferBinding->getBufferCount(); iBuffer++)
				bind->setBinding(iBuffer, orgSubMesh->vertexData[0]->vertexBufferBinding->getBuffer(iBuffer));

			destSubMesh->vertexData[0] = destVertexData;
			destSubMesh->operationType = orgSubMesh->operationType;
			destSubMesh->useSharedVertices = orgSubMesh->useSharedVertices;//must be false
			destSubMesh->setMaterialName(orgSubMesh->getMaterialName());

			destSubMesh->indexData[0]->indexBuffer = orgSubMesh->indexData[0]->indexBuffer;
			destSubMesh->indexData[0]->indexCount = orgSubMesh->indexData[0]->indexCount;
			destSubMesh->indexData[0]->indexStart = orgSubMesh->indexData[0]->indexStart;
		}
	}
	if (!bWroteSome)
		return Ogre::v1::MeshPtr();

	outMesh->_setBounds(Ogre::AxisAlignedBox(vMin, vMax));
	//outMesh->load();
	return outMesh;
}

void OgreMeshExtractor::FlipMesh(Ogre::v1::MeshPtr mesh)
{
	if (!mesh.get())
		return;

	Ogre::v1::Mesh::SubMeshIterator itSubMesh = mesh->getSubMeshIterator();
	while (itSubMesh.hasMoreElements())
	{
		Ogre::v1::SubMesh* subMesh = itSubMesh.getNext();
		//swap two indices of each tri -> make it a backface, thereby flipping it
		Ogre::v1::HardwareIndexBufferSharedPtr pIB = subMesh->indexData[0]->indexBuffer;
		void* pData = pIB->lock(Ogre::v1::HardwareBuffer::HBL_NORMAL);
		int iIndexSize = pIB->getIndexSize();
		char* pBuf = new char[iIndexSize];
		for (unsigned int iTri = 0; iTri < pIB->getNumIndexes() / 3; iTri++)
		{
			char* pBaseAddress = ((char*)pData) + iTri * 3 * iIndexSize;
			memcpy(pBuf, pBaseAddress, iIndexSize);
			memcpy(pBaseAddress, pBaseAddress + iIndexSize, iIndexSize);
			memcpy(pBaseAddress + iIndexSize, pBuf, iIndexSize);
		}
		pIB->unlock();
		//flip all normals

		Ogre::v1::VertexData* pVertexData = 0;
		if (subMesh->useSharedVertices)
			//shared vertices
			pVertexData = mesh->sharedVertexData[0];
		else
			//use own vertex buffer
			pVertexData = subMesh->vertexData[0];

		//mesh needs to have normal data
		if (!pVertexData->vertexDeclaration->findElementBySemantic(Ogre::VES_NORMAL))
			continue;

		const Ogre::v1::VertexElement* pPosElem = pVertexData->vertexDeclaration->findElementBySemantic(Ogre::VES_NORMAL);
		int iPosOffset = pPosElem->getOffset();
		Ogre::v1::HardwareVertexBufferSharedPtr pVB = pVertexData->vertexBufferBinding->getBuffer(pPosElem->getSource());
		unsigned int iStartVert = pVertexData->vertexStart;

		pData = pVB->lock(Ogre::v1::HardwareBuffer::HBL_NORMAL);

		int iVertSize = pVB->getVertexSize();

		for (unsigned int iVertex = iStartVert; iVertex < iStartVert + pVertexData->vertexCount; iVertex++)
		{
			void* pSource = (void*)(((char*)pData) + iVertex*iVertSize + iPosOffset);
			//set all coords to the negative direction
			for (int iCoord = 0; iCoord < 3; iCoord++)
				((float*)pSource)[iCoord] = -((float*)pSource)[iCoord];
		}
		pVB->unlock();
	}
}

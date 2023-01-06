#include "cvCSGConverter.h"
#include "Solid.h"
#include "VectorSet.h"
#include "IntSet.h"
#include "ColorSet.h"

using namespace Ogre::v1;

cvCSGConverter::cvCSGConverter()
{
}

cvCSGConverter::~cvCSGConverter()
{
}

Solid* cvCSGConverter::convertMesh2Solid(Ogre::v1::Entity *pEntity, int ObjNum)
{
	// Ogre Mesh -> Solid
	// mesh data to retrieve
	size_t vertex_count;
	size_t index_count;
	Ogre::Vector3 *vertices;
	Ogre::Vector3 *uvs;
	unsigned long *indices;
	// get the mesh information
	getMeshInformation( pEntity->getMesh(), vertex_count, vertices, 
						uvs, 
						index_count, indices,
						pEntity->getParentNode()->_getDerivedPosition(),
						pEntity->getParentNode()->_getDerivedOrientation(),
						pEntity->getParentNode()->getScale());

	VectorSet*	Vertices = new VectorSet();
	IntSet*		Indices  = new IntSet();
	ColorSet*	Colors   = new ColorSet();
	gxColor		TmpColor;
	// Convert...
	for (int i = 0; i < static_cast<int>(vertex_count); i++)
	{
		Vertices->AddVector(ov2sv(vertices[i]));
		TmpColor = ouv2scolor( uvs[i] );
		TmpColor.alpha = ObjNum;		// 어느 오브젝트에 속한 꼭지점인지 저장해둔다.
		Colors->AddColor( TmpColor );
	}
	for (int i = 0; i < static_cast<int>(index_count); i++)
	{
		Indices->AddInt(indices[i]);
	}

	Solid *Object = new Solid(Vertices, Indices, Colors);
	
	delete Colors;
	delete Indices;
	delete Vertices;

	delete[] vertices;
	delete[] uvs;
	delete[] indices;

	return Object;
}

bool cvCSGConverter::convertSolid2Mesh(const std::string& Name, Solid *Solid, Ogre::v1::Entity *Ent1, Ogre::v1::Entity *Ent2)
{
	// Get Data
	VectorSet*	Vertices = Solid->getVertices();
	IntSet*		Indices  = Solid->getIndices();
	ColorSet*	Colors   = Solid->getColors();
	int VertexCount = Vertices->GetSize();
	int IndexCount = Indices->GetSize();
	if(!VertexCount )
		return false;

	// 필요한 서브메쉬 카운트를 계산, Vertex 갯수와 Color 갯수가 같다는 가정하에 한다.
	{
		mSubMeshCount = 0;
		Ogre::Vector3 TmpColor;
		gxColor* sColor = Colors->GetColor(0);
		int PrevObj  = sColor->alpha;
		int PrevMesh = sColor->blue;
		int Index    = 0;
		int Count    = Colors->GetSize();
		for(int i=1; i<Count; i++)
		{
			sColor = Colors->GetColor(i);
			// 다른 오브젝트거나 다른 Submesh Number일때, 또는 마지막인 경우 마무리
			if(sColor->alpha != PrevObj || sColor->blue != PrevMesh || i == Count-1)
			{
				mSubMeshInfo[mSubMeshCount] = new svCSGMeshInfo;
				// 현재까지 정보 저장
				mSubMeshInfo[mSubMeshCount]->From = Index;
				mSubMeshInfo[mSubMeshCount]->To   = (i==Count-1) ? Count : i;			// 마지막은 포함하지 않음
				mSubMeshInfo[mSubMeshCount]->ObjNumber = PrevObj;
				mSubMeshInfo[mSubMeshCount]->MeshNumber= PrevMesh;
				if(PrevObj == 1)
					mSubMeshInfo[mSubMeshCount]->MaterialName = getSubMeshMaterialName(Ent1, PrevMesh);
				else
					mSubMeshInfo[mSubMeshCount]->MaterialName = getSubMeshMaterialName(Ent2, PrevMesh);

				mSubMeshCount++;
				// 현재 값으로 대체
				Index	 = i;
				PrevObj  = sColor->alpha;
				PrevMesh = sColor->blue;
			}
		}
	}
	// 꼭지점은 메쉬에 넣고 Index만 서브메쉬별로 넣는다.
	MeshPtr pMesh = Ogre::v1::MeshManager::getSingleton().createManual(Name, Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME);
	pMesh->sharedVertexData[0] = new Ogre::v1::VertexData(nullptr);
	Ogre::v1::VertexData* vertexData = pMesh->sharedVertexData[0];
	// define the vertex format
	VertexDeclaration* vertexDecl = vertexData->vertexDeclaration;
	size_t currOffset = 0;
	// positions
	vertexDecl->addElement(0, currOffset, Ogre::VET_FLOAT3, Ogre::VES_POSITION);
	currOffset += VertexElement::getTypeSize(Ogre::VET_FLOAT3);
	// normals
	vertexDecl->addElement(0, currOffset, Ogre::VET_FLOAT3, Ogre::VES_NORMAL);
	currOffset += VertexElement::getTypeSize(Ogre::VET_FLOAT3);
	// two dimensional texture coordinates
	vertexDecl->addElement(0, currOffset, Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES, 0);
	currOffset += VertexElement::getTypeSize(Ogre::VET_FLOAT2);

	// allocate vertex buffer
	vertexData->vertexCount = VertexCount;
	HardwareVertexBufferSharedPtr vBuf = HardwareBufferManager::getSingleton().createVertexBuffer(vertexDecl->getVertexSize(0), vertexData->vertexCount, HardwareBuffer::HBU_STATIC_WRITE_ONLY, false);
	VertexBufferBinding* binding = vertexData->vertexBufferBinding;
	binding->setBinding(0, vBuf);
	float* pVertex = static_cast<float*>(vBuf->lock(HardwareBuffer::HBL_DISCARD));

	// Set Vertex
	Ogre::Vector3 Pos;
	Ogre::Vector3 Color;
	for(int i=0; i<VertexCount; i++)
	{
		Pos = sv2ov(Vertices->GetVector(i));
		Color = scolor2ouv(Colors->GetColor(i));
		// Vertex
		*pVertex++ = Pos.x;
		*pVertex++ = Pos.y;
		*pVertex++ = Pos.z;
		// Normal
		*pVertex++ = Pos.normalisedCopy().x;
		*pVertex++ = Pos.normalisedCopy().y;
		*pVertex++ = Pos.normalisedCopy().z;
		// UV
		*pVertex++ = Color.x;
		*pVertex++ = Color.y;
		// Bounding Box Recalc for sure
		if(Pos.x < mBBMin.x)	mBBMin.x = Pos.x;
		if(Pos.y < mBBMin.y)	mBBMin.y = Pos.y;
		if(Pos.z < mBBMin.z)	mBBMin.z = Pos.z;
		if(Pos.x > mBBMax.x)	mBBMax.x = Pos.x;
		if(Pos.y > mBBMax.y)	mBBMax.y = Pos.y;
		if(Pos.z > mBBMax.z)	mBBMax.z = Pos.z;
	}
	// Unlock
	vBuf->unlock();

	for(int k=0; k<mSubMeshCount; k++)
	{
		int IdxFrom = mSubMeshInfo[k]->From;
		int IdxTo   = mSubMeshInfo[k]->To;							// 마지막은 포함하지 않음
		int Count   = IdxTo - IdxFrom;

		SubMesh *pSubMesh = pMesh->createSubMesh(Name+"_"+Ogre::StringConverter::toString(k));
		pSubMesh->useSharedVertices = true;
		if(mSubMeshInfo[k]->MaterialName.empty())
			pSubMesh->setMaterialName("BaseWhiteNoLighting");
		else
			pSubMesh->setMaterialName(mSubMeshInfo[k]->MaterialName);

		// allocate index buffer
		pSubMesh->indexData[0]->indexStart  = 0;
		pSubMesh->indexData[0]->indexCount  = Count;
		pSubMesh->indexData[0]->indexBuffer = HardwareBufferManager::getSingleton().createIndexBuffer(HardwareIndexBuffer::IT_16BIT, pSubMesh->indexData[0]->indexCount, HardwareBuffer::HBU_STATIC_WRITE_ONLY, false);
		HardwareIndexBufferSharedPtr iBuf = pSubMesh->indexData[0]->indexBuffer;
		unsigned short* pIndices = static_cast<unsigned short*>(iBuf->lock(HardwareBuffer::HBL_DISCARD));

		// Set Index
		int *dt;
		unsigned short idx;
		for(int i=IdxFrom; i<IdxTo; i++)
		{
			dt = Indices->GetInt(i);
			idx = (unsigned short)*dt;
			*pIndices++ = idx;
		}
		iBuf->unlock();
	}
	// Set Bound Box
	pMesh->_setBounds( Ogre::AxisAlignedBox( mBBMin, mBBMax ), false );
	Ogre::Vector3 mDist = mBBMax - mBBMin;
	pMesh->_setBoundingSphereRadius(mDist.length() / 2.0f);	// 거리 / 2 ?
	// this line makes clear the mesh is loaded (avoids memory leaks)
	pMesh->load();

	// 메모리 해제
	delete Vertices;
	delete Indices;
	delete Colors;
	for(int i=0; i<mSubMeshCount; i++)
		delete mSubMeshInfo[i];

	return true;
}

std::string cvCSGConverter::getSubMeshMaterialName(Ogre::v1::Entity *Ent1, int SubMeshNo)
{
// Attention: is that correct?
	return *Ent1->getSubEntity(SubMeshNo)->getDatablock()->getNameStr();
}

// Get the mesh information for the given mesh.
// Base Code found on this forum link: http://www.ogre3d.org/wiki/index.php/RetrieveVertexData
// Added : Texture(UV) Coordination and submesh number
void cvCSGConverter::getMeshInformation(const Ogre::v1::MeshPtr mesh,
										size_t &vertex_count,
										Ogre::Vector3* &vertices,
										Ogre::Vector3* &uvs,
										size_t &index_count,
										unsigned long* &indices,
										const Ogre::Vector3 &position,
										const Ogre::Quaternion &orient,
										const Ogre::Vector3 &scale)
{
    bool added_shared = false;
    size_t current_offset = 0;
    size_t shared_offset = 0;
    size_t next_offset = 0;
    size_t index_offset = 0;

    vertex_count  = index_count = 0;

	mSubMeshCount = mesh->getNumSubMeshes();
    // Calculate how many vertices and indices we're going to need
    for (unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i)
    {
        Ogre::v1::SubMesh* submesh = mesh->getSubMesh( i );

        // We only need to add the shared vertices once
        if(submesh->useSharedVertices)
        {
            if( !added_shared )
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
    uvs		 = new Ogre::Vector3[vertex_count];			// UV
    indices	 = new unsigned long[index_count];

    added_shared = false;

    // Run through the submeshes again, adding the data into the arrays
    for ( unsigned short i = 0; i < mesh->getNumSubMeshes(); ++i)
    {
        Ogre::v1::SubMesh* submesh = mesh->getSubMesh(i);

        Ogre::v1::VertexData* vertex_data = submesh->useSharedVertices ? mesh->sharedVertexData[0] : submesh->vertexData[0];

        if((!submesh->useSharedVertices)||(submesh->useSharedVertices && !added_shared))
        {
            if(submesh->useSharedVertices)
            {
                added_shared = true;
                shared_offset = current_offset;
            }
			// Postion
			{
				const Ogre::v1::VertexElement* posElem =
					vertex_data->vertexDeclaration->findElementBySemantic(Ogre::VES_POSITION);
				Ogre::v1::HardwareVertexBufferSharedPtr vbuf =
					vertex_data->vertexBufferBinding->getBuffer(posElem->getSource());
				unsigned char* vertex =
					static_cast<unsigned char*>(vbuf->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));

				// There is _no_ baseVertexPointerToElement() which takes an Ogre::Real or a double
				//  as second argument. So make it float, to avoid trouble when Ogre::Real will
				//  be comiled/typedefed as double:
				//      Ogre::Real* pReal;
				float* pReal;

				for( size_t j = 0; j < vertex_data->vertexCount; ++j, vertex += vbuf->getVertexSize())
				{
					posElem->baseVertexPointerToElement(vertex, &pReal);

					Ogre::Vector3 pt(pReal[0], pReal[1], pReal[2]);

					vertices[current_offset + j] = (orient * (pt * scale)) + position;
				}
				vbuf->unlock();
				next_offset += vertex_data->vertexCount;
			}
			// UV
			{
				const Ogre::v1::VertexElement* posElem =
					vertex_data->vertexDeclaration->findElementBySemantic(Ogre::VES_TEXTURE_COORDINATES);	// UV 추출
				Ogre::v1::HardwareVertexBufferSharedPtr vbuf =
					vertex_data->vertexBufferBinding->getBuffer(posElem->getSource());
				unsigned char* vertex =
					static_cast<unsigned char*>(vbuf->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));
				float* pReal;
				for( size_t j = 0; j < vertex_data->vertexCount; ++j, vertex += vbuf->getVertexSize())
				{
					posElem->baseVertexPointerToElement(vertex, &pReal);
					Ogre::Vector3 clr(pReal[0], pReal[1], i);							// UV + SubMesh Number
					uvs[current_offset + j] = clr;										// UV
				}
				vbuf->unlock();
			}
        }

        Ogre::v1::IndexData* index_data = submesh->indexData[0];
        size_t numTris = index_data->indexCount / 3;
        Ogre::v1::HardwareIndexBufferSharedPtr ibuf = index_data->indexBuffer;

        bool use32bitindexes = (ibuf->getType() == Ogre::v1::HardwareIndexBuffer::IT_32BIT);

        unsigned long*  pLong = static_cast<unsigned long*>(ibuf->lock(Ogre::v1::HardwareBuffer::HBL_READ_ONLY));
        unsigned short* pShort = reinterpret_cast<unsigned short*>(pLong);

        size_t offset = (submesh->useSharedVertices)? shared_offset : current_offset;

        if ( use32bitindexes )
        {
            for ( size_t k = 0; k < numTris*3; ++k)
            {
				if(i > 0 && index_offset < vertex_data->vertexCount)
					uvs[index_offset].z = i;			// SubMesh Number Correction
                indices[index_offset++] = pLong[k] + static_cast<unsigned long>(offset);
            }
        }
        else
        {
            for ( size_t k = 0; k < numTris*3; ++k)
            {
				if(i > 0 && index_offset < vertex_data->vertexCount)
					uvs[index_offset].z = i;			// SubMesh Number Correction
                indices[index_offset++] = static_cast<unsigned long>(pShort[k]) +
                    static_cast<unsigned long>(offset);
            }
        }

        ibuf->unlock();
        current_offset = next_offset;
    }
}
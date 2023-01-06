/*
 * CutMeshGenerator.cpp
 *
 *  Created on: Apr 30, 2011
 *      Author: _1nsane
 */

#include "CutMeshGenerator.h"
#include <stdlib.h>
#include "OgreMeshExtractor.h"
#include "FractalNoiseGenerator.h"

#define CALC_VECTOR(iXCell, iYCell, iWidth, iHeight, aafHeightmap) (Ogre::Vector3(((float)iXCell)/((float)(iWidth-1))-0.5f, ((float)iYCell)/((float)(iHeight-1))-0.5f,  aafHeightmap[iXCell][iYCell]))
#define CALC_UV(iXCell, iYCell, iWidth, iHeight) (Ogre::Vector2((float)iXCell/(float)(iWidth-1), (float)iYCell/(float)(iHeight-1)))

void CutMeshGenerator::Generate(Ogre::v1::MeshPtr meshptr, float** aafHeightmap, int iWidth, int iHeight,
		Ogre::Matrix4 mTransform, Ogre::Matrix3 mUVTransform, bool bSmooth)
{
	Ogre::Matrix3 mNormalTransform;
	mTransform.extract3x3Matrix(mNormalTransform);

	meshptr->createSubMesh("CutPlane");


	int nTris=(iWidth-1)*(iHeight-1)*2;
	int nVerts;
	if(bSmooth)
		nVerts=iWidth*iHeight;
	else
		nVerts=nTris*3;

	Ogre::v1::VertexData* data = new Ogre::v1::VertexData(nullptr);
	meshptr.get()->getSubMesh(0)->vertexData[0] = data;
	meshptr.get()->getSubMesh(0)->operationType = Ogre::OperationType::OT_TRIANGLE_LIST;
	meshptr.get()->getSubMesh(0)->useSharedVertices=false;
	data->vertexCount = nVerts;
	Ogre::v1::VertexDeclaration* decl = data->vertexDeclaration;
	decl->addElement(0, 0, Ogre::VET_FLOAT3, Ogre::VES_POSITION);
	size_t off = Ogre::v1::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
	decl->addElement(0, off, Ogre::VET_FLOAT3, Ogre::VES_NORMAL);
	off+=Ogre::v1::VertexElement::getTypeSize(Ogre::VET_FLOAT3);
	decl->addElement(0, off, Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES);

	Ogre::v1::HardwareVertexBufferSharedPtr vbuf = Ogre::v1::HardwareBufferManager::getSingleton().createVertexBuffer(
	decl->getVertexSize(0), nVerts, Ogre::v1::HardwareBuffer::HBU_STATIC);

	float* afVertexData=(float*)vbuf->lock(Ogre::v1::HardwareBuffer::HBL_DISCARD);

	Ogre::v1::HardwareIndexBufferSharedPtr ibuf = Ogre::v1::HardwareBufferManager::getSingleton().createIndexBuffer(
	Ogre::v1::HardwareIndexBuffer::IT_32BIT, nTris*3, Ogre::v1::HardwareBuffer::HBU_STATIC);

	unsigned int* aiIndexBuf=(unsigned int*)ibuf->lock(Ogre::v1::HardwareBuffer::HBL_DISCARD);

	std::vector<Ogre::Vector3> vvNormals(nTris);
	std::vector<std::vector<unsigned int> > vvVertexNormals(nVerts);

	Ogre::Vector3 vMin(1.0e+10f, 1.0e+10f, 1.0e+10f), vMax(-1.0e+10f, -1.0e+10f, -1.0e+10f);
	for(int iXCell=0; iXCell<iWidth-1; iXCell++)
	{
		for(int iYCell=0; iYCell<iHeight-1; iYCell++)
		{
			//create two tris per cell
			bool bSplitLeftTop=!((iXCell+iYCell)&1);
			//first calc the four corner points
			Ogre::Vector3 vCorners[4]={
				CALC_VECTOR(iXCell, iYCell, iWidth, iHeight, aafHeightmap),
				CALC_VECTOR(iXCell, iYCell+1, iWidth, iHeight, aafHeightmap),
				CALC_VECTOR(iXCell+1, iYCell, iWidth, iHeight, aafHeightmap),
				CALC_VECTOR(iXCell+1, iYCell+1, iWidth, iHeight, aafHeightmap)};

			//corner uvs
			Ogre::Vector2 vUVs[4]={
				CALC_UV(iXCell, iYCell, iWidth, iHeight),
				CALC_UV(iXCell, iYCell+1, iWidth, iHeight),
				CALC_UV(iXCell+1, iYCell, iWidth, iHeight),
				CALC_UV(iXCell+1, iYCell+1, iWidth, iHeight)};

			int aiIndices[6]={
				0,1,bSplitLeftTop ? 3 : 2,
				3,2,bSplitLeftTop ? 0 : 1};

			int iTriOffset=(iXCell+iYCell*(iWidth-1))*2;

			//generate normals for both tris
			Ogre::Vector3 vNormals[2]={
				Ogre::Vector3((vCorners[aiIndices[2]]-vCorners[0]).crossProduct(vCorners[1]-vCorners[0])),
				Ogre::Vector3((vCorners[aiIndices[5]]-vCorners[3]).crossProduct(vCorners[2]-vCorners[3]))};

			if(bSmooth)
			{
				vvNormals[iTriOffset+0]=vNormals[0];
				vvNormals[iTriOffset+1]=vNormals[1];
				for(unsigned int iIndex=0; iIndex<6; iIndex++)
				{
					int iVertexIndex=(iXCell+((aiIndices[iIndex]&2)>>1))+iWidth*(iYCell+(aiIndices[iIndex]&1));
					aiIndexBuf[iTriOffset*3+iIndex]=iVertexIndex;
					vvVertexNormals[iVertexIndex].push_back(iTriOffset+(iIndex/3));
				}
			}

			//now set up the vertices
			if(!bSmooth)
				for(int iVertex=0; iVertex<6; iVertex++)
				{
					int iOffset=(iTriOffset*3+iVertex)*decl->getVertexSize(0)/sizeof(float);
					//set position
					Ogre::Vector3 vPos=mTransform*vCorners[aiIndices[iVertex]];
					afVertexData[iOffset+0]=vPos.x;
					afVertexData[iOffset+1]=vPos.y;
					afVertexData[iOffset+2]=vPos.z;

					vMin.makeFloor(vPos);
					vMax.makeCeil(vPos);
					//set normal
					Ogre::Vector3 vNormal=(mNormalTransform*vNormals[iVertex>=3 ? 1 : 0]);
					vNormal.normalise();
					vNormal=vNormal*(-1.0f);
					afVertexData[iOffset+3]=vNormal.x;
					afVertexData[iOffset+4]=vNormal.y;
					afVertexData[iOffset+5]=vNormal.z;
					//set UVs
					Ogre::Vector2 vUV=vUVs[aiIndices[iVertex]];
					Ogre::Vector3 vTemp=mUVTransform*Ogre::Vector3(vUV.x, vUV.y, 1.0f);
					vUV=Ogre::Vector2(vTemp.x, vTemp.y);
					afVertexData[iOffset+6]=vUV.x;
					afVertexData[iOffset+7]=vUV.y;
				}
		}
	}
	if(bSmooth)
		for(int iXCell=0; iXCell<iWidth; iXCell++)
		{
			for(int iYCell=0; iYCell<iHeight; iYCell++)
			{
				Ogre::Vector3 v=CALC_VECTOR(iXCell, iYCell, iWidth, iHeight, aafHeightmap);
				//uv
				Ogre::Vector2 uv=CALC_UV(iXCell, iYCell, iWidth, iHeight);

				int iVertexIndex=iXCell+iWidth*iYCell;
				int iOffset=iVertexIndex*decl->getVertexSize(0)/sizeof(float);
				//set position
				Ogre::Vector3 vPos=mTransform*v;
				afVertexData[iOffset+0]=vPos.x;
				afVertexData[iOffset+1]=vPos.y;
				afVertexData[iOffset+2]=vPos.z;

				vMin.makeFloor(vPos);
				vMax.makeCeil(vPos);
				//set normal
				//iterate normals
				Ogre::Vector3 vNormal=Ogre::Vector3(0,0,0);
				int nNormals=vvVertexNormals[iVertexIndex].size();
				for(int iNormal=0; iNormal<nNormals ; iNormal++)
					vNormal=vNormal + vvNormals[vvVertexNormals[iVertexIndex][iNormal]];

				vNormal=(mNormalTransform*vNormal);
				vNormal.normalise();
				vNormal=vNormal*(-1.0f);
				afVertexData[iOffset+3]=vNormal.x;
				afVertexData[iOffset+4]=vNormal.y;
				afVertexData[iOffset+5]=vNormal.z;
				//set UVs
				Ogre::Vector2 vUV=uv;
				Ogre::Vector3 vTemp=mUVTransform*Ogre::Vector3(vUV.x, vUV.y, 1.0f);
				vUV=Ogre::Vector2(vTemp.x, vTemp.y);
				afVertexData[iOffset+6]=vUV.x;
				afVertexData[iOffset+7]=vUV.y;
			}
		}

	vbuf->unlock();

	if(!bSmooth)
		for(int iIndex=0; iIndex<nVerts; iIndex++)
			aiIndexBuf[iIndex]=iIndex;

	ibuf->unlock();

	Ogre::v1::VertexBufferBinding* bind = data->vertexBufferBinding;
	bind->setBinding(0, vbuf);

	meshptr.get()->getSubMesh(0)->indexData[0]->indexBuffer = ibuf;
	meshptr.get()->getSubMesh(0)->indexData[0]->indexCount = nTris*3;
	meshptr.get()->getSubMesh(0)->indexData[0]->indexStart = 0;

	meshptr.get()->_setBounds(Ogre::AxisAlignedBox(vMin, vMax));
	//meshptr.get()->load();
}


#define PRINT_VECTOR(v) (printf("(%f, %f, %f)", v.x, v.y, v.z))

/*
 * create a rough cutting geometry
 * @param inMesh mesh to base the cutting geometry on
 * @param fRoughness maximum height of bumps on the cutting geometry relative to its witdh
 * @param fResolution maximum size of the triangles of the cutting geometry in relative units
 * @return mesh to write the cutting plane to
 */
void
CutMeshGenerator::CreateRandomCutMesh(Ogre::v1::MeshPtr inMesh, Ogre::v1::MeshPtr outMesh, float fRoughness, float fResolution, Ogre::String strMatName, bool bSmooth)
{
	//get longest axis of input mesh
	Ogre::Vector3 vSplitPos=inMesh->getBounds().getCenter();
	Ogre::Vector3 vSize=inMesh->getBounds().getSize();

	Ogre::Vector3 vSplitNormal=Ogre::Vector3::ZERO,
					vSplitOrthoNormals[2]={Ogre::Vector3::ZERO, Ogre::Vector3::ZERO};
	bool abNormalDir[3]={false, false, false};
	float fMaxRadius=vSize.length()*1.1f;
	if(vSize.x>=std::max(vSize.y, vSize.z))
		abNormalDir[0]=true;
	if(vSize.y>=std::max(vSize.x, vSize.z))
		abNormalDir[1]=true;
	if(vSize.z>=std::max(vSize.x, vSize.y))
		abNormalDir[2]=true;

	for(int i=0; i<3; i++)
	{
		if(abNormalDir[(i-1+3)%3])
		{
			vSplitNormal[i]=vSize[i];
			for(int j=0; j<2; j++)
			{
				int iIndex=(i+j+1)%3;
				vSplitOrthoNormals[j][iIndex]=1.0f;
			}
		}
	}

	vSplitNormal.x+=0.1f*((float)rand()/(float)RAND_MAX);
	vSplitNormal.y+=0.1f*((float)rand()/(float)RAND_MAX);
	vSplitNormal.z+=0.1f*((float)rand()/(float)RAND_MAX);

	vSplitPos.x+=vSize.x*0.05f*((float)(rand()-(RAND_MAX>>1))/(float)RAND_MAX);
	vSplitPos.y+=vSize.y*0.05f*((float)(rand()-(RAND_MAX>>1))/(float)RAND_MAX);
	vSplitPos.z+=vSize.z*0.05f*((float)(rand()-(RAND_MAX>>1))/(float)RAND_MAX);

	vSplitPos+=vSplitNormal*0.1f*((float)(rand()-(RAND_MAX>>1))/(float)RAND_MAX);

#ifdef VERBOSE
	printf("\ninput mesh");
	printf("\nbox min:");
	PRINT_VECTOR(inMesh->getBounds().getMinimum());
	printf("\nbox max:");
	PRINT_VECTOR(inMesh->getBounds().getMaximum());
	printf("\nbox center:");
	PRINT_VECTOR(inMesh->getBounds().getCenter());
	printf("\nbox extent:");
	PRINT_VECTOR(inMesh->getBounds().getSize());

	printf("\nsplit position:");
	PRINT_VECTOR(vSplitPos);
	printf("\nsplit normal:");
	PRINT_VECTOR(vSplitNormal);
	printf("\nsplit normal length: %f", vSplitNormal.length());
	printf("\n");
#endif

	//generate cutting plane
	{
		//noise size depends on size of the split plane
		float fScale=fMaxRadius/fResolution;
		int iNoise=4;//(int)log2(fScale+1.0f);
		float** ppfNoise=FractalNoiseGenerator::Generate(iNoise, 1.0f);

		Ogre::Matrix3 matRot3;
		//Ogre::Quaternion(Ogre::Radian(0), vSplitNormal.normalisedCopy()).ToRotationMatrix(matRot3);
		matRot3=Ogre::Matrix3(vSplitNormal.normalisedCopy().x, vSplitOrthoNormals[0].x, vSplitOrthoNormals[1].x,
				vSplitNormal.normalisedCopy().y, vSplitOrthoNormals[0].y, vSplitOrthoNormals[1].y,
				vSplitNormal.normalisedCopy().z, vSplitOrthoNormals[0].z, vSplitOrthoNormals[1].z);

		Ogre::Matrix4 mTransform=Ogre::Matrix4::IDENTITY;

		Ogre::Matrix4 mScale=Ogre::Matrix4::getScale(fMaxRadius,fMaxRadius,fRoughness*vSplitNormal.length()),
						mTranslate=Ogre::Matrix4::getTrans(vSplitPos),
						mRotate=Ogre::Matrix4(matRot3);

		mTransform=mTranslate*mRotate*mScale;
		//mTransform=mTranslate;

		Ogre::Matrix3 mTexTransform;//=Ogre::Matrix3::IDENTITY;
		Ogre::Matrix4::getScale(fScale,fScale,0).extract3x3Matrix(mTexTransform);

		CutMeshGenerator::Generate(outMesh, ppfNoise, (1<<iNoise)+1, (1<<iNoise)+1,
				mTransform, mTexTransform, bSmooth);
		outMesh->getSubMesh(0)->setMaterialName(strMatName);
	}
#ifdef VERBOSE
	printf("\ncut mesh");
	printf("\nbox min:");
	PRINT_VECTOR(outMesh->getBounds().getMinimum());
	printf("\nbox max:");
	PRINT_VECTOR(outMesh->getBounds().getMaximum());
	printf("\nbox center:");
	PRINT_VECTOR(outMesh->getBounds().getCenter());
	printf("\nbox extent:");
	PRINT_VECTOR(outMesh->getBounds().getSize());
	printf("\n");
#endif
}

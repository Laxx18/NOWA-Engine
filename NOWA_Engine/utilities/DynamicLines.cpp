#include "NOWAPrecompiled.h"
#include "DynamicLines.hpp"

namespace NOWA
{

	/*
	 * =============================================================================================
	 * BtOgre::DynamicLines
	 * =============================================================================================
	 */

	enum
	{
		POSITION_BINDING,
		TEXCOORD_BINDING
	};

	//------------------------------------------------------------------------------------------------
	DynamicLines::DynamicLines(Ogre::IdType id, Ogre::ObjectMemoryManager* objManager, Ogre::SceneManager* sceneManager, OperationType opType)
		: ManualObject(id, objManager, sceneManager),
		dirty(true)
	{

	}

	//------------------------------------------------------------------------------------------------
	DynamicLines::~DynamicLines()
	{
	}

	//------------------------------------------------------------------------------------------------
	void DynamicLines::setOperationType(OperationType opType)
	{
	//	m_operationType = opType;
	}

	//------------------------------------------------------------------------------------------------
	Ogre::OperationType DynamicLines::getOperationType() const
	{
		return this->operationType;
	}

	//------------------------------------------------------------------------------------------------
	void DynamicLines::addPoint(const Ogre::Vector3 &p)
	{
		this->points.push_back(p);
		this->dirty = true;
	}

	//------------------------------------------------------------------------------------------------
	void DynamicLines::addPoint(Ogre::Real x, Ogre::Real y, Ogre::Real z)
	{
		this->points.push_back(Ogre::Vector3(x, y, z));
		this->dirty = true;
	}

	//------------------------------------------------------------------------------------------------
	const Ogre::Vector3& DynamicLines::getPoint(unsigned short index) const
	{
		assert(index <  this->points.size() && "Point index is out of bounds!!");
		return this->points[index];
	}

	//------------------------------------------------------------------------------------------------
	unsigned short DynamicLines::getNumPoints(void) const
	{
		return (unsigned short) this->points.size();
	}

	//------------------------------------------------------------------------------------------------
	void DynamicLines::setPoint(unsigned short index, const Ogre::Vector3 &value)
	{
		assert(index < this->points.size() && "Point index is out of bounds!!");

		this->points[index] = value;
		this->dirty = true;
	}

	//------------------------------------------------------------------------------------------------
	void DynamicLines::clear()
	{
		this->points.clear();
		this->dirty = true;
	}

	//------------------------------------------------------------------------------------------------
	void DynamicLines::update()
	{
		if (this->dirty)
		{
			fillHardwareBuffers();
		}
	}

	//------------------------------------------------------------------------------------------------
	void DynamicLines::createVertexDeclaration()
	{
		//    Ogre::v1::VertexDeclaration *decl = mRenderOp.vertexData->vertexDeclaration;
		//   decl->addElement(POSITION_BINDING, 0, VET_FLOAT3, VES_POSITION);
//		mVertexElements.push_back(VertexElement2(VET_FLOAT3, VES_POSITION));
	}

	//------------------------------------------------------------------------------------------------
	void DynamicLines::fillHardwareBuffers()
	{
		int size = static_cast<int>(this->points.size());

		ManualObject::clear();

		begin("BaseWhite", Ogre::OperationType::OT_LINE_LIST);

		for (int i = 0; i < size; i++)
		{
			position(this->points[i].x, this->points[i].y, this->points[i].z);
			index(i);
			//		*mVertexBuffer++ = this->points[i].y;
			//		*mVertexBuffer++ = this->points[i].z;

			//		if (m32BitIndices)
			//		{
			//			*((uint32 *)indexData) = i;
		}

		// Back
		//manualObject->position(0.0f, 0.0f, 0.0f);
		//manualObject->position(0.0f, 1.0f, 0.0f);
		//manualObject->line(0, 1);

		//manualObject->position(0.0f, 1.0f, 0.0f);
		//manualObject->position(1.0f, 1.0f, 0.0f);
		//manualObject->line(2, 3);

		//manualObject->position(1.0f, 1.0f, 0.0f);
		//manualObject->position(1.0f, 0.0f, 0.0f);
		//manualObject->line(4, 5);

		//manualObject->position(1.0f, 0.0f, 0.0f);
		//manualObject->position(0.0f, 0.0f, 0.0f);
		//manualObject->line(6, 7);

		//// Front
		//manualObject->position(0.0f, 0.0f, 1.0f);
		//manualObject->position(0.0f, 1.0f, 1.0f);
		//manualObject->line(8, 9);

		//manualObject->position(0.0f, 1.0f, 1.0f);
		//manualObject->position(1.0f, 1.0f, 1.0f);
		//manualObject->line(10, 11);

		//manualObject->position(1.0f, 1.0f, 1.0f);
		//manualObject->position(1.0f, 0.0f, 1.0f);
		//manualObject->line(12, 13);

		//manualObject->position(1.0f, 0.0f, 1.0f);
		//manualObject->position(0.0f, 0.0f, 1.0f);
		//manualObject->line(14, 15);

		//// Sides
		//manualObject->position(0.0f, 0.0f, 0.0f);
		//manualObject->position(0.0f, 0.0f, 1.0f);
		//manualObject->line(16, 17);

		//manualObject->position(0.0f, 1.0f, 0.0f);
		//manualObject->position(0.0f, 1.0f, 1.0f);
		//manualObject->line(18, 19);

		//manualObject->position(1.0f, 0.0f, 0.0f);
		//manualObject->position(1.0f, 0.0f, 1.0f);
		//manualObject->line(20, 21);

		//manualObject->position(1.0f, 1.0f, 0.0f);
		//manualObject->position(1.0f, 1.0f, 1.0f);
		//manualObject->line(22, 23);

		end();

		return;

	//	prepareHardwareBuffers(size, 0);

	//	if (!size) {
	//		mBox.setExtents(Vector3::ZERO, Vector3::ZERO);
	//		this->dirty = false;
	//		return;
	//	}

	//	Vector3 vaabMin = this->points[0];
	//	Vector3 vaabMax = this->points[0];


	//	//reset ---------------------------------
	//	//reset ---------------------------------
	//	//reset ---------------------------------	
	//	mVertexBuffer = 0;
	//	//	mIndexBuffer = mIndexBufferCursor = 0;


	//	//clear -------------
	//	//clear -------------
	//	//clear -------------
	//	VertexArrayObject *vao = mVao;

	//	if (vao)
	//	{
	//		const VertexBufferPackedVec &vertexBuffers = vao->getVertexBuffers();
	//		VertexBufferPackedVec::const_iterator itBuffers = vertexBuffers.begin();
	//		VertexBufferPackedVec::const_iterator endBuffers = vertexBuffers.end();

	//		while (itBuffers != endBuffers)
	//		{
	//			VertexBufferPacked * vertexBuffer = *itBuffers;

	//			vertexBuffer->unmap(UO_UNMAP_ALL);

	//			mVaoManager->destroyVertexBuffer(vertexBuffer);

	//			++itBuffers;
	//		}

	//		IndexBufferPacked * indexBuffer = vao->getIndexBuffer();

	//		if (indexBuffer)
	//		{
	//			indexBuffer->unmap(UO_UNMAP_ALL);

	//			mVaoManager->destroyIndexBuffer(indexBuffer);
	//		}

	//		mVaoManager->destroyVertexArrayObject(vao);
	//	}
	//	mVao = 0;
	//	mVaoPerLod.clear();
	//	mRenderables.clear();
	//	mIndexBufferCursor = 0;
	//	//clear -------------
	//	//clear -------------
	//	//clear -------------


	//	mIndexBufferCapacity = size; // for each point an index
	//	//end ------------------------
	//	//end ------------------------
	//	//end ------------------------
	//	Ogre::VertexBufferPackedVec vertexBuffers;
	//	//Create the vertex buffer
	//	VertexBufferPacked * vertexBuffer = mVaoManager->createVertexBuffer(mVertexElements, mVertexBufferCapacity, BT_DYNAMIC_PERSISTENT, NULL, false);
	//	mVertexBuffer = reinterpret_cast<float *>(vertexBuffer->map(0, vertexBuffer->getNumElements()));

	//	Ogre::IndexBufferPacked *indexBuffer = mVaoManager->createIndexBuffer(m32BitIndices ? IndexBufferPacked::IT_32BIT : IndexBufferPacked::IT_16BIT, mIndexBufferCapacity, BT_DYNAMIC_PERSISTENT, NULL, false);
	//	char * indexData = reinterpret_cast<char *>(indexBuffer->map(0, indexBuffer->getNumElements()));
	//	mIndices = 0;

	//	//update --------------------
	//	for (int i = 0; i < size; i++)
	//	{
	//		*mVertexBuffer++ = this->points[i].x;
	//		*mVertexBuffer++ = this->points[i].y;
	//		*mVertexBuffer++ = this->points[i].z;

	//		if (m32BitIndices)
	//		{
	//			*((uint32 *)indexData) = i;
	//			indexData += sizeof(uint32);
	//		}
	//		else
	//		{
	//			*((uint16 *)indexData) = i;
	//			indexData += sizeof(uint16);
	//		}

	//		mIndices++;

	//		if (this->points[i].x < vaabMin.x)
	//			vaabMin.x = this->points[i].x;
	//		if (this->points[i].y < vaabMin.y)
	//			vaabMin.y = this->points[i].y;
	//		if (this->points[i].z < vaabMin.z)
	//			vaabMin.z = this->points[i].z;

	//		if (this->points[i].x > vaabMax.x)
	//			vaabMax.x = this->points[i].x;
	//		if (this->points[i].y > vaabMax.y)
	//			vaabMax.y = this->points[i].y;
	//		if (this->points[i].z > vaabMax.z)
	//			vaabMax.z = this->points[i].z;
	//	}

	//	vertexBuffer->unmap(Ogre::UO_KEEP_PERSISTENT);
	//	indexBuffer->unmap(Ogre::UO_KEEP_PERSISTENT);

	//	vertexBuffers.push_back(vertexBuffer);

	//	//Ogre::uint16 *cubeIndices = reinterpret_cast<Ogre::uint16*>(OGRE_MALLOC_SIMD(
	//	//	sizeof(Ogre::uint16) * 3 * 2 * 6,
	//	//	Ogre::MEMCATEGORY_GEOMETRY));
	//	//memcpy(cubeIndices, c_indexData, sizeof(c_indexData));

	////	Ogre::IndexBufferPacked *indexBuffer = mVaoManager->createIndexBuffer(m32BitIndices ? IndexBufferPacked::IT_32BIT : IndexBufferPacked::IT_16BIT, mIndices, BT_DYNAMIC_PERSISTENT, NULL, false);
	///*	char * indexData = reinterpret_cast<char *>(indexBuffer->map(0, indexBuffer->getNumElements()));

	//	assert(indexData);

	//	for (int i = 0; i < size; i++)
	//	{
	//		*indexData++ = this->points[i].x;
	//		*indexData++ = this->points[i].y;
	//		*indexData++ = this->points[i].z;

	//	}

	//	indexBuffer->unmap(UO_KEEP_PERSISTENT);*/


	//	mVao = mVaoManager->createVertexArrayObject(vertexBuffers, indexBuffer, m_operationType);

	//	mVaoPerLod.push_back(mVao);

	//	mRenderables.push_back(this);

	//	setDatablock("BaseWhite");



	//	//Real *prPos = static_cast<Real*>(vbuf->lock(Ogre::v1::HardwareBuffer::HBL_DISCARD));
	//	//      {
	//	//          for(int i = 0; i < size; i++)
	//	//          {
	//	//              *prPos++ = this->points[i].x;
	//	//              *prPos++ = this->points[i].y;
	//	//              *prPos++ = this->points[i].z;

	//	//              if(this->points[i].x < vaabMin.x)
	//	//                  vaabMin.x = this->points[i].x;
	//	//              if(this->points[i].y < vaabMin.y)
	//	//                  vaabMin.y = this->points[i].y;
	//	//              if(this->points[i].z < vaabMin.z)
	//	//                  vaabMin.z = this->points[i].z;

	//	//              if(this->points[i].x > vaabMax.x)
	//	//                  vaabMax.x = this->points[i].x;
	//	//              if(this->points[i].y > vaabMax.y)
	//	//                  vaabMax.y = this->points[i].y;
	//	//              if(this->points[i].z > vaabMax.z)
	//	//                  vaabMax.z = this->points[i].z;
	//	//          }
	//	//      }
	//	//      vbuf->unlock();

	//	mBox.setExtents(vaabMin, vaabMax);

		this->dirty = false;
	}
} // namespace end

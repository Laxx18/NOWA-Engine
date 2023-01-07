#include "NOWAPrecompiled.h"

#if 0

#include "DynamicRenderable.hpp"

namespace NOWA
{

	/*
	 * =============================================================================================
	 * BtOgre::DynamicRenderable
	 * =============================================================================================
	 */

	DynamicRenderable::DynamicRenderable(Ogre::IdType id, Ogre::ObjectMemoryManager* objManager, Ogre::SceneManager* sceneManager)
		: MovableObject(id, objManager, sceneManager, 0),
		vertexBufferCapacity(0),
		indexBufferCapacity(0),
		vaoManager(sceneManager->getDestinationRenderSystem()->getVaoManager()),
		vao(nullptr)
	{

	}

	//------------------------------------------------------------------------------------------------
	DynamicRenderable::~DynamicRenderable()
	{
		//  delete mRenderOp.vertexData;
		//  delete mRenderOp.indexData;
	}

	//------------------------------------------------------------------------------------------------
	void DynamicRenderable::initialize(Ogre::OperationType operationType, bool useIndices)
	{
		this->operationType = operationType;
		// Initialize render operation
  //      mRenderOp.operationType = operationType;
  //      mRenderOp.useIndexes = useIndices;
		//mRenderOp.vertexData = new Ogre::v1::VertexData;
  //      if (mRenderOp.useIndexes)
		//	mRenderOp.indexData = new Ogre::v1::IndexData;

		// Reset buffer capacities
		this->vertexBufferCapacity = 0;
		this->indexBufferCapacity = 0;

		// Create vertex declaration
		createVertexDeclaration();
	}

	//------------------------------------------------------------------------------------------------
	void DynamicRenderable::prepareHardwareBuffers(size_t vertexCount, size_t indexCount)
	{
		// Prepare vertex buffer
		size_t newVertCapacity = this->vertexBufferCapacity;
		if ((vertexCount > this->vertexBufferCapacity) || (!this->vertexBufferCapacity))
		{
			// vertexCount exceeds current capacity!
			// It is necessary to reallocate the buffer.

			// Check if this is the first call
			if (!newVertCapacity)
			{
				newVertCapacity = 1;
			}
			// Make capacity the next power of two
			while (newVertCapacity < vertexCount)
			{
				newVertCapacity <<= 1;
			}
		}
		else if (vertexCount < this->vertexBufferCapacity >> 1)
		{
			// Make capacity the previous power of two
			while (vertexCount < newVertCapacity >> 1)
			{
				newVertCapacity >>= 1;
			}
		}
		if (newVertCapacity != this->vertexBufferCapacity)
		{
			this->vertexBufferCapacity = newVertCapacity;


			// Create new vertex buffer
			//Ogre::v1::HardwareVertexBufferSharedPtr vbuf =
			//	Ogre::v1::HardwareBufferManager::getSingleton().createVertexBuffer(
   //                     mRenderOp.vertexData->vertexDeclaration->getVertexSize(0),
   //                     this->vertexBufferCapacity,
			//			Ogre::v1::HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY); // TODO: Custom HBU_?

			// Bind buffer
		 //   mRenderOp.vertexData->vertexBufferBinding->setBinding(0, vbuf);
		}
		// Update vertex count in the render operation
	   // mRenderOp.vertexData->vertexCount = vertexCount;

  //      if (mRenderOp.useIndexes)
  //      {
		OgreAssert(indexCount <= std::numeric_limits<unsigned short>::max(), "indexCount exceeds 16 bit");

		size_t newIndexCapacity = this->indexBufferCapacity;
		// Prepare index buffer
		if ((indexCount > newIndexCapacity) || (!newIndexCapacity))
		{
			// indexCount exceeds current capacity!
			// It is necessary to reallocate the buffer.

			// Check if this is the first call
			if (!newIndexCapacity)
			{
				newIndexCapacity = 1;
			}
			// Make capacity the next power of two
			while (newIndexCapacity < indexCount)
			{
				newIndexCapacity <<= 1;
			}

		}
		else if (indexCount < newIndexCapacity >> 1)
		{
			// Make capacity the previous power of two
			while (indexCount < newIndexCapacity >> 1)
			{
				newIndexCapacity >>= 1;
			}
		}

		if (newIndexCapacity != this->indexBufferCapacity)
		{
			this->indexBufferCapacity = newIndexCapacity;
			// Create new index buffer
			//mRenderOp.indexData->indexBuffer =
			//	Ogre::v1::HardwareBufferManager::getSingleton().createIndexBuffer(
			//	Ogre::v1::HardwareIndexBuffer::IT_16BIT,
//                        this->indexBufferCapacity,
			//			Ogre::v1::HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY); // TODO: Custom HBU_?


			//char * indexData = static_cast<char *>(indexBuffer->map(0, indexBuffer->getNumElements()));
			//
			//if (m32BitIndices)
			// {
			//	memcpy(indexData, mIndexBuffer, mIndices * sizeof(uint32));
			//}
			//else
			//{

			//}

		}
		// Update index count in the render operation
	 //   mRenderOp.indexData->indexCount = indexCount;
	}


	void DynamicRenderable::getRenderOperation(Ogre::v1::RenderOperation& op)
	{
		OGRE_EXCEPT(Ogre::Exception::ERR_NOT_IMPLEMENTED,
			"ManualObject does not implement getRenderOperation. "
			"Use MovableObject::setRenderQueueGroup to change the group.",
			"ManualObjectSection::getRenderOperation");
		//op = m_operationType;
	}
	//-----------------------------------------------------------------------------
	void DynamicRenderable::getWorldTransforms(Ogre::Matrix4* xform) const
	{
		OGRE_EXCEPT(Ogre::Exception::ERR_NOT_IMPLEMENTED,
			"ManualObject does not implement getWorldTransforms. "
			"Use MovableObject::setRenderQueueGroup to change the group.",
			"ManualObjectSection::getWorldTransforms");
		//	*xform = mWorldTransform * mParentNode->_getFullTransform();
	}

	const Ogre::LightList& DynamicRenderable::getLights(void) const
	{
		return queryLights();
	}


	//------------------------------------------------------------------------------------------------
	Ogre::Real DynamicRenderable::getBoundingRadius(void) const
	{
		return Ogre::Math::Sqrt(std::max(this->box.getMaximum().squaredLength(), this->box.getMinimum().squaredLength()));
	}

	//------------------------------------------------------------------------------------------------
	Ogre::Real DynamicRenderable::getSquaredViewDepth(const Ogre::Camera* cam) const
	{
		Ogre::Vector3 vMin, vMax, vMid, vDist;
		vMin = this->box.getMinimum();
		vMax = this->box.getMaximum();
		vMid = ((vMax - vMin) * 0.5) + vMin;
		vDist = cam->getDerivedPosition() - vMid;

		return vDist.squaredLength();
	}

	const Ogre::String& DynamicRenderable::getMovableType(void) const
	{
		return "DynamicLines";//ManualObjectFactory::FACTORY_TYPE_NAME;
	}

} // namespace end

#endif

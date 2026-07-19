/*!
	@file
	@author		Albert Semenov
	@date		04/2009
	@update		2026 v2 VAO rewrite for Ogre-Next by NOWA-Engine
*/

#include "MyGUI_Ogre2VertexBuffer.h"

#include <OgreRoot.h>
#include <OgreRenderSystem.h>
#include <Vao/OgreVaoManager.h>
#include <Vao/OgreVertexArrayObject.h>
#include <Vao/OgreVertexBufferPacked.h>

#include <algorithm>

#include "MyGUI_LastHeader.h"

namespace MyGUI
{

	const size_t VERTEX_IN_QUAD = 6;
	const size_t RENDER_ITEM_STEEP_REALLOCK = 5 * VERTEX_IN_QUAD;

	Ogre2VertexBuffer::Ogre2VertexBuffer() :
		mVertexCount(RENDER_ITEM_STEEP_REALLOCK),
		mNeedVertexCount(0),
		mVaoManager(nullptr),
		mVertexBuffer(nullptr),
		mVao(nullptr)
	{
		mVaoManager = Ogre::Root::getSingletonPtr()->getRenderSystem()->getVaoManager();
		createVertexBuffer();
	}

	Ogre2VertexBuffer::~Ogre2VertexBuffer()
	{
		destroyVertexBuffer();
	}

	void Ogre2VertexBuffer::createVertexBuffer()
	{
		// Must match MyGUI::Vertex byte-for-byte:
		//   float x, y, z;     -> VET_FLOAT3  VES_POSITION           (12 bytes)
		//   uint32 colour;     -> VET_UBYTE4_NORM VES_DIFFUSE        ( 4 bytes)
		//   float u, v;        -> VET_FLOAT2  VES_TEXTURE_COORDINATES( 8 bytes)
		//
		// VET_UBYTE4_NORM is read by the GPU in memory byte order R,G,B,A.
		// On little-endian this is the layout of a uint32 packed as
		// A<<24|B<<16|G<<8|R, i.e. MyGUI's VertexColourType::ColourABGR.
		// The RenderManager therefore reports ColourABGR unconditionally.
		Ogre::VertexElement2Vec vertexElements;
		vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
		vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_UBYTE4_NORM, Ogre::VES_DIFFUSE));
		vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));

		// BT_DEFAULT: GPU retains contents across frames; we upload only when
		// MyGUI actually rewrites the buffer (lock/unlock on dirty widgets).
		mVertexBuffer = mVaoManager->createVertexBuffer(vertexElements, mVertexCount, Ogre::BT_DEFAULT, nullptr, false);

		Ogre::VertexBufferPackedVec vertexBuffers;
		vertexBuffers.push_back(mVertexBuffer);

		mVao = mVaoManager->createVertexArrayObject(vertexBuffers, nullptr, Ogre::OT_TRIANGLE_LIST);
		// Draw nothing until setOperationVertexCount() is called.
		mVao->setPrimitiveRange(0, 0);

		mCpuBuffer.resize(mVertexCount);

		// Keep the renderable's mVaoPerLod pointing at the current VAO.
		mRenderable._setVao(mVao);
	}

	void Ogre2VertexBuffer::destroyVertexBuffer()
	{
		mRenderable._setVao(nullptr);

		if (mVao != nullptr)
		{
			mVaoManager->destroyVertexArrayObject(mVao);
			mVao = nullptr;
		}
		if (mVertexBuffer != nullptr)
		{
			// VaoManager defers the actual GPU deletion until in-flight frames
			// are done, so this is safe even if the buffer was drawn recently.
			mVaoManager->destroyVertexBuffer(mVertexBuffer);
			mVertexBuffer = nullptr;
		}
	}

	void Ogre2VertexBuffer::resizeVertexBuffer()
	{
		mVertexCount = mNeedVertexCount + RENDER_ITEM_STEEP_REALLOCK;
		destroyVertexBuffer();
		createVertexBuffer();
	}

	void Ogre2VertexBuffer::setVertexCount(size_t _count)
	{
		mNeedVertexCount = _count;
	}

	size_t Ogre2VertexBuffer::getVertexCount()
	{
		return mNeedVertexCount;
	}

	void Ogre2VertexBuffer::setOperationVertexCount(size_t _count)
	{
		if (mVao != nullptr)
		{
			mVao->setPrimitiveRange(0, static_cast<Ogre::uint32>(_count));
		}
	}

	Vertex* Ogre2VertexBuffer::lock()
	{
		if (mNeedVertexCount > mVertexCount)
		{
			resizeVertexBuffer();
		}

		return mCpuBuffer.data();
	}

	void Ogre2VertexBuffer::unlock()
	{
		const size_t count = std::min(mNeedVertexCount, mVertexCount);
		if (count > 0 && mVertexBuffer != nullptr)
		{
			// Uploads via VaoManager staging buffer. GPU-side contents persist,
			// so untouched buffers keep rendering their last data — same
			// semantics as the old v1 HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE path
			// as used by MyGUI (rewrite-on-dirty, not rewrite-per-frame).
			mVertexBuffer->upload(mCpuBuffer.data(), 0, count);
		}
	}

} // namespace MyGUI

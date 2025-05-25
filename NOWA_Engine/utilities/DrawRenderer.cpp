#include "NOWAPrecompiled.h"

#if 0
#include "DrawRenderer.hpp"

namespace NOWA
{

	//------------------------------------------------------------------------------------------------
	DrawRenderer::DrawRenderer(Ogre::uint32 id, Ogre::SceneManager* sceneManager, Ogre::ObjectMemoryManager* objManager, float fillAlpha)
		: MovableObject(id, objManager, sceneManager, 0),
		sceneManager(sceneManager),
		objectMemoryManager(objManager),
		fillAlpha(fillAlpha),
		sceneNodeLines(0),
		initalized(false)
	{
			Ogre::LogManager::getSingleton().logMessage("RQ -> DrawRenderer::DrawRenderer");

			this->sceneNodeLines = this->sceneManager->getRootSceneNode(Ogre::SCENE_DYNAMIC)->createChildSceneNode(Ogre::SCENE_DYNAMIC);
			this->sceneNodeLines->attachObject(this);

			std::unique_ptr<DrawRenderable> p1(new DrawRenderable(this->sceneManager));
			this->renderables.push_back(std::move(p1));

			Ogre::Aabb aabb(Ogre::Aabb::BOX_INFINITE);
			mObjectData.mLocalAabb->setFromAabb(aabb, mObjectData.mIndex);
			mObjectData.mWorldAabb->setFromAabb(aabb, mObjectData.mIndex);
			mObjectData.mLocalRadius[mObjectData.mIndex] = std::numeric_limits<Ogre::Real>::max();
			mObjectData.mWorldRadius[mObjectData.mIndex] = std::numeric_limits<Ogre::Real>::max();
	}

	//------------------------------------------------------------------------------------------------

	void DrawRenderer::updateVertices(std::queue<int>& indices, std::queue<Vertex>& vertices)
	{
		if (vertices.size() == 0)
		{
			clear();
			return;
		}

		this->renderables[0]->updateVertices(indices, vertices);
		//todo _t_renderables[1]->updateVertices(vertices);

		if (!this->initalized)
		{
			setCastShadows(false);

			mRenderables.push_back(this->renderables[0].get());
			//todo mRenderables.push_back(_t_renderables[1].get());

			this->initalized = true;
		}

		Ogre::Aabb aabb2(Ogre::Aabb::BOX_INFINITE);//2D
		mObjectData.mLocalAabb->setFromAabb(aabb2, mObjectData.mIndex); //2D
		mObjectData.mWorldAabb->setFromAabb(aabb2, mObjectData.mIndex); //2D
																		//mObjectData.mLocalAabb->setFromAabb(aabb, mObjectData.mIndex); //3D
																		//mObjectData.mLocalRadius[mObjectData.mIndex] = aabb.getRadius();//3D
		mObjectData.mLocalRadius[mObjectData.mIndex] = std::numeric_limits<Ogre::Real>::max();
		mObjectData.mWorldRadius[mObjectData.mIndex] = std::numeric_limits<Ogre::Real>::max();
	}

	void DrawRenderer::clear()
	{
		if (!this->initalized)
		{
			return;
		}
		//clear aabb
		mObjectData.mLocalAabb->setFromAabb(Ogre::Aabb::BOX_NULL, mObjectData.mIndex);
		mObjectData.mLocalRadius[mObjectData.mIndex] = 0.0f;
	}

	DrawRenderer::~DrawRenderer()
	{
		NOWA::GraphicsModule::getInstance()->removeTrackedNode(this->sceneNodeLines);
		this->sceneManager->destroySceneNode(this->sceneNodeLines);
	}

	const Ogre::String& DrawRenderer::getMovableType(void) const
	{
		return "DrawRenderer";
	}

	DrawRenderable::DrawRenderable(Ogre::SceneManager* sceneManager)
		: Renderable(),
		sceneManager(sceneManager),
		operationType(Ogre::OperationType::OT_LINE_LIST),
		currentVertexBufferSize(0),
		currentIndexBufferSize(0),
		vao(0)
	{
		Ogre::RenderSystem* renderSystem = Ogre::Root::getSingletonPtr()->getRenderSystem();
		this->vaoManager = renderSystem->getVaoManager();
	}

	void DrawRenderable::updateVertices(std::queue<int>& indices, std::queue<Vertex>& vertices) {

		if (vertices.size() > this->currentVertexBufferSize) // resize
		{
			this->currentVertexBufferSize = vertices.size();
			this->currentIndexBufferSize = this->currentVertexBufferSize; //FIXME lesser indexes are needed!!

			clear();

			Ogre::VertexElement2Vec vertexElements;
			vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT3, Ogre::VES_POSITION));
			vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT4, Ogre::VES_DIFFUSE));
			vertexElements.push_back(Ogre::VertexElement2(Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES));

			this->vertexBuffer = this->vaoManager->createVertexBuffer(vertexElements, vertices.size(), Ogre::BT_DYNAMIC_PERSISTENT_COHERENT, 0, false);

			//Now the Vao
			Ogre::VertexBufferPackedVec vertexBuffers;
			vertexBuffers.push_back(this->vertexBuffer);

			//-------------- index -----------------
			this->indexBuffer = this->vaoManager->createIndexBuffer(Ogre::IndexBufferPacked::IT_16BIT,
				this->currentIndexBufferSize, //index buffer size -> todo currently one index per vertex!!!??
				Ogre::BT_DYNAMIC_DEFAULT, nullptr, false);

			this->vao = this->vaoManager->createVertexArrayObject(vertexBuffers, this->indexBuffer, Ogre::OperationType::OT_LINE_LIST);

			mVaoPerLod[0].push_back(this->vao);
			//mVaoPerLod[1].push_back(this->Vao); //needed for shadow caster Node!

			setDatablock("BaseWhite"); // set this after  createVertexArrayObject  and mVaoPerLod[0].push_back(vao) ?? or else crash in renderable			
		}
		else if (vertices.size() < this->currentVertexBufferSize) //shrink 
		{
			this->vao->setPrimitiveRange(0, vertices.size());
		}
		//Uint32 idx = 0;
		int vertexCnt = 0;

		float* vertexBufferCursor = (float*)(this->vertexBuffer->map(0, this->vertexBuffer->getNumElements()));

		Ogre::Aabb aabb;

		while (!vertices.empty())
		{
			auto vertex = vertices.front();

			//position
			*vertexBufferCursor++ = vertex.position.x;
			*vertexBufferCursor++ = vertex.position.y;
			*vertexBufferCursor++ = vertex.position.z;

			aabb.merge(vertex.position);

			//color
			*vertexBufferCursor++ = vertex.color.r;
			*vertexBufferCursor++ = vertex.color.g;
			*vertexBufferCursor++ = vertex.color.b;
			*vertexBufferCursor++ = vertex.color.a;

			//texture
			*vertexBufferCursor++ = vertex.uv.x;
			*vertexBufferCursor++ = vertex.uv.y;

			vertices.pop();
		}

		this->vertexBuffer->unmap(Ogre::UO_UNMAP_ALL);
		
		//-------------- index -----------------

		char* indexBufferCursor = (char*)this->indexBuffer->map(0, indices.size());

		while (!indices.empty())
		{
			auto index = indices.front();

			*((Ogre::uint16 *)indexBufferCursor) = index;
			indexBufferCursor += sizeof(Ogre::uint16);

			indices.pop();
		}
		this->indexBuffer->unmap(Ogre::UO_UNMAP_ALL);
		//-------------- end index -----------------
	}

	void DrawRenderable::clear()
	{
		this->currentVertexBufferSize = 0;

		Ogre::VertexArrayObject* tempVao = this->vao;

		if (tempVao)
		{
			Ogre::VaoManager* tempVaoManager = this->vaoManager;

			const Ogre::VertexBufferPackedVec &vertexBuffers = vao->getVertexBuffers();

			Ogre::VertexBufferPackedVec::const_iterator itBuffers = vertexBuffers.begin();
			Ogre::VertexBufferPackedVec::const_iterator endBuffers = vertexBuffers.end();

			while (itBuffers != endBuffers)
			{
				Ogre::VertexBufferPacked * vertexBuffer = *itBuffers;

				if (vertexBuffer->getMappingState() != Ogre::MS_UNMAPPED)
				{
					vertexBuffer->unmap(Ogre::UO_UNMAP_ALL);
				}

				tempVaoManager->destroyVertexBuffer(vertexBuffer);

				++itBuffers;
			}

			Ogre::IndexBufferPacked * indexBuffer = tempVao->getIndexBuffer();

			if (indexBuffer)
			{
				if (indexBuffer->getMappingState() != Ogre::MS_UNMAPPED)
				{
					indexBuffer->unmap(Ogre::UO_UNMAP_ALL);
				}

				tempVaoManager->destroyIndexBuffer(indexBuffer);
			}

			tempVaoManager->destroyVertexArrayObject(tempVao);
		}

		tempVao = nullptr;
		mVaoPerLod[0].clear();
		mVaoPerLod[1].clear();
	}

	void DrawRenderable::getRenderOperation(Ogre::v1::RenderOperation& op, bool casterPass)
	{
		OGRE_EXCEPT(Ogre::Exception::ERR_NOT_IMPLEMENTED,
			"DrawRenderable does not implement getRenderOperation. "
			"Use MovableObject::setRenderQueueGroup to change the group.",
			"DrawRenderable::getRenderOperation");
	}

	DrawRenderable::~DrawRenderable()
	{

	}

} // namespace end

#endif

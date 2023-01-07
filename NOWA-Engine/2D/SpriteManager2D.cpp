#include "NOWAPrecompiled.h"
#include "SpriteManager2D.h"
#include "utilities/MathHelper.h"

namespace NOWA
{

#define OGRE2D_MINIMAL_HARDWARE_BUFFER_SIZE 120

	SpriteManager2D::SpriteManager2D(Ogre::SceneManager* sceneManager, Ogre::uint8 targetQueue, bool afterQueue)
		: sceneManager(sceneManager),
		afterQueue(afterQueue),
		targetQueue(targetQueue),
		xScroll(0.0f),
		yScroll(0.0f),
		lastPositionX(0.0f),
		firstTimePositionSet(true),
		lastPosition(Ogre::Vector2::ZERO)
	{
		hardwareBuffer.setNull();

		sceneManager->addRenderQueueListener(this);
	}

	SpriteManager2D::~SpriteManager2D()
	{
		if (!hardwareBuffer.isNull())
		{
			destroyHardwareBuffer();
		}
		sceneManager->removeRenderQueueListener(this);
		for (size_t i = 0; i < this->texturePtrList.size(); i++)
		{
			Ogre::TextureManager::getSingleton().unload(this->texturePtrList[i]->getHandle());
		}
	}

	void SpriteManager2D::renderQueueStarted(Ogre::uint8 queueGroupId, const Ogre::String &invocation, bool &skipThisInvocation)
	{
		if (!afterQueue && queueGroupId == targetQueue)
		{
			renderBuffer();
			// this->sceneManager->addSpecialCaseRenderQueue(targetQueue);
			//this->sceneManager->setSpecialCaseRenderQueueMode(Ogre::SceneManager::SCRQM_EXCLUDE);
		}
	}

	void SpriteManager2D::renderQueueEnded(Ogre::uint8 queueGroupId, const Ogre::String &invocation, bool &repeatThisInvocation)
	{
		if (afterQueue && queueGroupId == targetQueue)
		{
			renderBuffer();
			// this->sceneManager->removeSpecialCaseRenderQueue(targetQueue);
		}
	}

	void SpriteManager2D::renderBuffer()
	{
		Ogre::RenderSystem* renderSystem = Ogre::Root::getSingleton().getRenderSystem();
		std::list<Ogre2dSprite>::const_iterator currSpr, endSpr;

		VertexChunk thisChunk;
		std::list<VertexChunk> chunks;

		unsigned int newSize;

		newSize = sprites.size() * 6;
		if (newSize < OGRE2D_MINIMAL_HARDWARE_BUFFER_SIZE)
		{
			newSize = OGRE2D_MINIMAL_HARDWARE_BUFFER_SIZE;
		}

		// grow hardware buffer if needed
		if (hardwareBuffer.isNull() || hardwareBuffer->getNumVertices() < newSize)
		{
			if (!hardwareBuffer.isNull())
			{
				destroyHardwareBuffer();
			}
			createHardwareBuffer(newSize);
		}

		if (sprites.empty())
		{
			return;
		}
		// write quads to the hardware buffer, and remember chunks
		float* buffer;
		float z = -1;

		buffer = (float*)hardwareBuffer->lock(Ogre::v1::HardwareBuffer::HBL_DISCARD);

		endSpr = sprites.cend();
		currSpr = sprites.cbegin();
		thisChunk.texHandle = currSpr->texHandle;
		thisChunk.vertexCount = 0;
		while (currSpr != endSpr)
		{
			// 1st point (left bottom)
			*buffer = currSpr->x1; buffer++;
			*buffer = currSpr->y2; buffer++;
			*buffer = z; buffer++;
			*buffer = currSpr->tx1; buffer++;
			*buffer = currSpr->ty2; buffer++;
			// 2st point (right top)
			*buffer = currSpr->x2; buffer++;
			*buffer = currSpr->y1; buffer++;
			*buffer = z; buffer++;
			*buffer = currSpr->tx2; buffer++;
			*buffer = currSpr->ty1; buffer++;
			// 3rd point (left top)
			*buffer = currSpr->x1; buffer++;
			*buffer = currSpr->y1; buffer++;
			*buffer = z; buffer++;
			*buffer = currSpr->tx1; buffer++;
			*buffer = currSpr->ty1; buffer++;

			// 4th point (left bottom)
			*buffer = currSpr->x1; buffer++;
			*buffer = currSpr->y2; buffer++;
			*buffer = z; buffer++;
			*buffer = currSpr->tx1; buffer++;
			*buffer = currSpr->ty2; buffer++;
			// 5th point (right bottom)
			*buffer = currSpr->x2; buffer++;
			*buffer = currSpr->y1; buffer++;
			*buffer = z; buffer++;
			*buffer = currSpr->tx2; buffer++;
			*buffer = currSpr->ty1; buffer++;
			// 6th point (right top)
			*buffer = currSpr->x2; buffer++;
			*buffer = currSpr->y2; buffer++;
			*buffer = z; buffer++;
			*buffer = currSpr->tx2; buffer++;
			*buffer = currSpr->ty2; buffer++;

			// remember this chunk
			thisChunk.vertexCount += 6;
			++currSpr;
			if (currSpr == endSpr || thisChunk.texHandle != currSpr->texHandle)
			{
				chunks.emplace_back(thisChunk);
				if (currSpr != endSpr)
				{
					thisChunk.texHandle = currSpr->texHandle;
					thisChunk.vertexCount = 0;
				}
			}
		}

		hardwareBuffer->unlock();

		// set up...
		prepareForRender();

		// do the real render!
		Ogre::TexturePtr tp;
		std::list<VertexChunk>::const_iterator currChunk, endChunk;

		endChunk = chunks.end();
		renderOp.vertexData->vertexStart = 0;
		for (currChunk = chunks.cbegin(); currChunk != endChunk; ++currChunk)
		{
			renderOp.vertexData->vertexCount = currChunk->vertexCount;
			tp = Ogre::TextureManager::getSingleton().getByHandle(currChunk->texHandle).dynamicCast<Ogre::Texture>();
			renderSystem->_setTexture(0, true, tp->getName());
			renderSystem->_render(renderOp);
			renderOp.vertexData->vertexStart += currChunk->vertexCount;
		}

		// sprites go home!
		sprites.clear();
	}

//	void SpriteManager2D::prepareForRender()
//	{
//// Must be adapted to hlms
//#if 0
//		Ogre::LayerBlendModeEx colorBlendMode;
//		Ogre::LayerBlendModeEx alphaBlendMode;
//		Ogre::TextureUnitState::UVWAddressingMode uvwAddressMode;
//
//		Ogre::RenderSystem* renderSystem = Ogre::Root::getSingleton().getRenderSystem();
//
//		colorBlendMode.blendType = Ogre::LBT_COLOUR;
//		colorBlendMode.source1 = Ogre::LBS_TEXTURE;
//		colorBlendMode.operation = Ogre::LBX_SOURCE1;
//
//		alphaBlendMode.blendType = Ogre::LBT_ALPHA;
//		alphaBlendMode.source1 = Ogre::LBS_TEXTURE;
//		alphaBlendMode.operation = Ogre::LBX_SOURCE1;
//
//		uvwAddressMode.u = Ogre::TextureUnitState::TAM_CLAMP;
//		uvwAddressMode.v = Ogre::TextureUnitState::TAM_CLAMP;
//		uvwAddressMode.w = Ogre::TextureUnitState::TAM_CLAMP;
//
//		renderSystem->_setWorldMatrix(Ogre::Matrix4::IDENTITY);
//		renderSystem->_setViewMatrix(Ogre::Matrix4::IDENTITY);
//		renderSystem->_setProjectionMatrix(Ogre::Matrix4::IDENTITY);
//		renderSystem->_setTextureMatrix(0, Ogre::Matrix4::IDENTITY);
//		renderSystem->_setTextureCoordSet(0, 0);
//		renderSystem->_setTextureCoordCalculation(0, Ogre::TEXCALC_NONE);
//		renderSystem->_setTextureUnitFiltering(0, Ogre::FO_LINEAR, Ogre::FO_LINEAR, Ogre::FO_POINT);
//		renderSystem->_setTextureBlendMode(0, colorBlendMode);
//		renderSystem->_setTextureBlendMode(0, alphaBlendMode);
//		renderSystem->_setTextureAddressingMode(0, uvwAddressMode);
//		renderSystem->_disableTextureUnitsFrom(1);
//		renderSystem->setLightingEnabled(false);
//		renderSystem->_setFog(Ogre::FOG_NONE);
//		renderSystem->_setCullingMode(Ogre::CULL_NONE);
//		renderSystem->_setDepthBufferParams(false, false);
//		renderSystem->_setColourBufferWriteEnabled(true, true, true, false);
//		renderSystem->setShadingType(Ogre::SO_GOURAUD); // SO_PHONG, SO_GOURAUD
//		renderSystem->_setPolygonMode(Ogre::PM_SOLID);
//		renderSystem->unbindGpuProgram(Ogre::GPT_FRAGMENT_PROGRAM);
//		renderSystem->unbindGpuProgram(Ogre::GPT_VERTEX_PROGRAM);
//		renderSystem->_setSceneBlending(Ogre::SBF_SOURCE_ALPHA, Ogre::SBF_ONE_MINUS_SOURCE_ALPHA);
//		renderSystem->_setAlphaRejectSettings(Ogre::CMPF_ALWAYS_PASS, 0, false);
//
//		//Ogre::Camera *pCam = sceneManager->getCamera("GamePlayCamera");
//		//// Ogre::RenderSystem* renderSystem = Ogre::Root::getSingleton().getRenderSystem();
//		//renderSystem->_setViewMatrix(pCam->getViewMatrix(true));
//		//renderSystem->_setProjectionMatrix(pCam->getProjectionMatrixRS());
//		//renderSystem->_setWorldMatrix(pCam->_getParentNodeFullTransform());
//#endif
//	}

	void SpriteManager2D::prepareForRender()
	{
		// Must be adapted to hlms
#if 1
		Ogre::LayerBlendModeEx colorBlendMode;
		Ogre::LayerBlendModeEx alphaBlendMode;
		// Ogre::TextureUnitState::UVWAddressingMode uvwAddressMode;

		Ogre::RenderSystem* renderSystem = Ogre::Root::getSingleton().getRenderSystem();

		colorBlendMode.blendType = Ogre::LBT_COLOUR;
		colorBlendMode.source1 = Ogre::LBS_TEXTURE;
		colorBlendMode.operation = Ogre::LBX_SOURCE1;

		alphaBlendMode.blendType = Ogre::LBT_ALPHA;
		alphaBlendMode.source1 = Ogre::LBS_TEXTURE;
		alphaBlendMode.operation = Ogre::LBX_SOURCE1;

		// uvwAddressMode.u = Ogre::TextureUnitState::TAM_CLAMP;
		// uvwAddressMode.v = Ogre::TextureUnitState::TAM_CLAMP;
		// uvwAddressMode.w = Ogre::TextureUnitState::TAM_CLAMP;

		renderSystem->_setWorldMatrix(Ogre::Matrix4::IDENTITY);
		renderSystem->_setViewMatrix(Ogre::Matrix4::IDENTITY);
		renderSystem->_setProjectionMatrix(Ogre::Matrix4::IDENTITY);
		renderSystem->_setTextureMatrix(0, Ogre::Matrix4::IDENTITY);
		renderSystem->_setTextureCoordSet(0, 0);
		renderSystem->_setTextureCoordCalculation(0, Ogre::TEXCALC_NONE);
		// renderSystem->_setTextureUnitFiltering(0, Ogre::FO_LINEAR, Ogre::FO_LINEAR, Ogre::FO_POINT);
		renderSystem->_setTextureBlendMode(0, colorBlendMode);
		renderSystem->_setTextureBlendMode(0, alphaBlendMode);
		// renderSystem->_setTextureAddressingMode(0, uvwAddressMode);
		renderSystem->_disableTextureUnitsFrom(1);
		// renderSystem->_setHlmsSamplerblock
		// renderSystem->_setComputePso
		// renderSystem->_setBindingType(Ogre::TextureUnitState::BindingType::BT_FRAGMENT);
		// renderSystem->_setPointSpritesEnabled
		// Ogre::v1::CbRenderOp renderOp;
		// renderOp.
		// renderSystem->_setRenderOperation()
		// renderSystem->_setTextureProjectionRelativeTo
		// renderSystem->setLightingEnabled(false);
		//renderSystem->_setFog(Ogre::FOG_NONE);
		//renderSystem->_setCullingMode(Ogre::CULL_NONE);
		//renderSystem->_setDepthBufferParams(false, false);
		//renderSystem->_setColourBufferWriteEnabled(true, true, true, false);
		//renderSystem->setShadingType(Ogre::SO_GOURAUD); // SO_PHONG, SO_GOURAUD
		//renderSystem->_setPolygonMode(Ogre::PM_SOLID);
		//renderSystem->unbindGpuProgram(Ogre::GPT_FRAGMENT_PROGRAM);
		//renderSystem->unbindGpuProgram(Ogre::GPT_VERTEX_PROGRAM);
		//renderSystem->_setSceneBlending(Ogre::SBF_SOURCE_ALPHA, Ogre::SBF_ONE_MINUS_SOURCE_ALPHA);
		//renderSystem->_setAlphaRejectSettings(Ogre::CMPF_ALWAYS_PASS, 0, false);

		//Ogre::Camera *pCam = sceneManager->getCamera("GamePlayCamera");
		//// Ogre::RenderSystem* renderSystem = Ogre::Root::getSingleton().getRenderSystem();
		//renderSystem->_setViewMatrix(pCam->getViewMatrix(true));
		//renderSystem->_setProjectionMatrix(pCam->getProjectionMatrixRS());
		//renderSystem->_setWorldMatrix(pCam->_getParentNodeFullTransform());
#endif
	}

	void SpriteManager2D::createHardwareBuffer(unsigned int size)
	{
		Ogre::v1::VertexDeclaration* vd;

		renderOp.vertexData = new Ogre::v1::VertexData;
		renderOp.vertexData->vertexStart = 0;

		vd = renderOp.vertexData->vertexDeclaration;
		vd->addElement(0, 0, Ogre::VET_FLOAT3, Ogre::VES_POSITION);
		vd->addElement(0, Ogre::v1::VertexElement::getTypeSize(Ogre::VET_FLOAT3),
			Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES);

		hardwareBuffer = Ogre::v1::HardwareBufferManager::getSingleton().createVertexBuffer(vd->getVertexSize(0), size, 
			Ogre::v1::HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY_DISCARDABLE, false);// use shadow buffer? no

		renderOp.vertexData->vertexBufferBinding->setBinding(0, hardwareBuffer);

		renderOp.operationType = Ogre::OperationType::OT_TRIANGLE_LIST;
		renderOp.useIndexes = false;

	}

	void SpriteManager2D::destroyHardwareBuffer()
	{
		delete renderOp.vertexData;
		renderOp.vertexData = 0;
		hardwareBuffer.setNull();
	}

	Ogre::TexturePtr SpriteManager2D::addSprite(const std::string& textureName)
	{
		// Ogre::TexturePtr texturePtr = Ogre::TextureManager::getSingleton().load(textureName, "Backgrounds");
		Ogre::TexturePtr texturePtr;
		auto result = Ogre::TextureManager::getSingleton().createOrRetrieve(textureName, "Backgrounds");
		if (result.second)
		{
			texturePtr = result.first.dynamicCast<Ogre::Texture>();
			this->texturePtrList.push_back(texturePtr);
		}
		return texturePtr;
	}

	void SpriteManager2D::spriteBltFull(Ogre::TexturePtr texturePtr, Ogre::Real x1, Ogre::Real y1, Ogre::Real x2, Ogre::Real y2, Ogre::Real tx1, Ogre::Real ty1, Ogre::Real tx2, Ogre::Real ty2)
	{
		Ogre2dSprite spr;

		spr.x1 = x1;
		spr.y1 = y1;
		spr.x2 = x2;
		spr.y2 = y2;

		spr.tx1 = tx1;
		spr.ty1 = ty1;
		spr.tx2 = tx2;
		spr.ty2 = ty2;

		spr.texHandle = texturePtr->getHandle();

		sprites.emplace_back(spr);
	}

	void SpriteManager2D::scroll(Ogre::TexturePtr texturePtr, Ogre::Real xSpeed, Ogre::Real ySpeed, Ogre::Real dt)
	{
		this->xScroll += xSpeed * dt;
		if (this->xScroll >= 2.0f)
		{
			this->xScroll = 0.0f;
		}
		this->yScroll += ySpeed * dt;
		if (this->yScroll >= 2.0f)
		{
			this->yScroll = 0.0f;
		}
		if (xSpeed != 0.0f)
		{
			this->spriteBltFull(texturePtr, -1.0f + this->xScroll, -1.0f + this->yScroll, 1.0f + this->xScroll, 1.0f + this->yScroll);
		}
		if (ySpeed != 0.0f)
		{
			this->spriteBltFull(texturePtr, -3.0f + this->xScroll, -3.0f + this->yScroll, -1.0f + this->xScroll, -1.0f + this->yScroll);
		}
	}

	void SpriteManager2D::backgroundRelativeToPositionX(Ogre::TexturePtr texturePtr, Ogre::Real absoluteXPos, Ogre::Real dt, Ogre::Real xSpeed)
	{
		if (this->firstTimePositionSet)
		{
			this->lastPositionX = absoluteXPos;
			this->firstTimePositionSet = false;
		}
		absoluteXPos = NOWA::MathHelper::getInstance()->lowPassFilter(absoluteXPos, this->lastPositionX, 0.1f);
		Ogre::Real velocityX = absoluteXPos - this->lastPositionX;

		// Ogre::Real velocityX = absoluteXPos - this->lastPositionX;

		this->xScroll -= velocityX * xSpeed * dt;
		// this->xScroll = Ogre::Math::Clamp(this->xScroll, 0.0f, 2.0f);
		if (this->xScroll > 2.0f)
		{
			this->xScroll = 0.0f;
		}
		else if (this->xScroll < 0.0f)
		{
			this->xScroll = 2.0f;
		}
		this->spriteBltFull(texturePtr, -1.0f + this->xScroll, 1.0f, 1.0f + this->xScroll, -1.0f);
		this->spriteBltFull(texturePtr, -3.0f + this->xScroll, 1.0f, -1.0f + this->xScroll, -1.0f);

		this->lastPositionX = absoluteXPos;
	}

	void SpriteManager2D::backgroundRelativeToPosition(Ogre::TexturePtr texturePtr, Ogre::Vector2 absolutePos, Ogre::Real dt, Ogre::Real xSpeed, Ogre::Real ySpeed)
	{
		if (this->firstTimePositionSet)
		{
			this->lastPosition = absolutePos;
			this->firstTimePositionSet = false;
		}
		absolutePos.x = NOWA::MathHelper::getInstance()->lowPassFilter(absolutePos.x, this->lastPosition.x, 0.1f);
		absolutePos.y = NOWA::MathHelper::getInstance()->lowPassFilter(absolutePos.y, this->lastPosition.y, 0.1f);

		//// Wasserfluss effekt:
		//static Ogre::Real x = 0.0f;
		//x += 0.1f * dt;
		//if (x >= 1.0f)
		//{
		//	x = 0.0f;
		//}
		//this->spriteManager2D->spriteBltFull("BackgroundFar1.png", 0.0 + x, 1.0, 2.0 + x, -1.0, x);

		/*Ogre::RenderSystem* renderSystem = Ogre::Root::getSingleton().getRenderSystem();
		Ogre::Viewport* vp = renderSystem->_getViewport();
		Ogre::Real hOffset = renderSystem->getHorizontalTexelOffset() / (0.5 * vp->getActualWidth());
		Ogre::Real vOffset = renderSystem->getVerticalTexelOffset() / (0.5 * vp->getActualHeight());
		this->spriteManager2D->spriteBltFull("BackgroundFar1.png", -1 + hOffset, 1 - vOffset, 1 + hOffset, -1 - vOffset);*/
		
		Ogre::Vector2 velocity = absolutePos - this->lastPosition;

		this->xScroll -= velocity.x * xSpeed * dt;
		// this->xScroll = Ogre::Math::Clamp(this->xScroll, 0.0f, 2.0f);
		if (this->xScroll > 2.0f)
		{
			this->xScroll = 0.0f;
		}
		else if (this->xScroll < 0.0f)
		{
			this->xScroll = 2.0f;
		}

		this->yScroll += velocity.y * ySpeed * dt;
		// this->xScroll = Ogre::Math::Clamp(this->xScroll, 0.0f, 2.0f);
		if (this->yScroll > 2.0f)
		{
			this->yScroll = 0.0f;
		}
		else if (this->yScroll < 0.0f)
		{
			this->yScroll = 2.0f;
		}
		//                                  x1					 y1					   x2					  y2
		this->spriteBltFull(texturePtr, -1.0f + this->xScroll, 1.0f + this->yScroll, 1.0f + this->xScroll, -1.0f + this->yScroll);
		this->spriteBltFull(texturePtr, -3.0f + this->xScroll, 1.0f + this->yScroll, -1.0f + this->xScroll, -1.0f + this->yScroll);

		this->spriteBltFull(texturePtr, -1.0f + this->xScroll, -1.0f + this->yScroll, 1.0f + this->xScroll, -3.0f + this->yScroll);
		this->spriteBltFull(texturePtr, -3.0f + this->xScroll, -1.0f + this->yScroll, -1.0f + this->xScroll, -3.0f + this->yScroll);

		this->lastPosition = absolutePos;
	}

}; //namespace end

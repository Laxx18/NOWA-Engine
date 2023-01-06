/*!
	@file
	@author		Albert Semenov
	@date		10/2009
*/

#include <cstring>
#include "MyGUI_Ogre2RTTexture.h"
#include "MyGUI_Ogre2RenderManager.h"

#include "OgreTextureGpu.h"

namespace MyGUI
{

	Ogre2RTTexture::Ogre2RTTexture(Ogre::TextureGpu* _texture) :
		mRenderPassDesc(nullptr),
		mSaveRenderPassDesc(nullptr),
		mTexture(_texture)
	{
		mProjectMatrix = Ogre::Matrix4::IDENTITY;
		Ogre::Root* root = Ogre::Root::getSingletonPtr();
		if (root != nullptr)
		{
			Ogre::RenderSystem* system = root->getRenderSystem();
			if (system != nullptr)
			{
				size_t width = mTexture->getWidth();
				size_t height = mTexture->getHeight();

				mRenderTargetInfo.maximumDepth = system->getMaximumDepthInputValue();
				mRenderTargetInfo.hOffset = system->getHorizontalTexelOffset() / float(width);
				mRenderTargetInfo.vOffset = system->getVerticalTexelOffset() / float(height);
				mRenderTargetInfo.aspectCoef = float(height) / float(width);
				mRenderTargetInfo.pixScaleX = 1.0f / float(width);
				mRenderTargetInfo.pixScaleY = 1.0f / float(height);
			}

			if (mTexture->requiresTextureFlipping())
			{
				mProjectMatrix[1][0] = -mProjectMatrix[1][0];
				mProjectMatrix[1][1] = -mProjectMatrix[1][1];
				mProjectMatrix[1][2] = -mProjectMatrix[1][2];
				mProjectMatrix[1][3] = -mProjectMatrix[1][3];
			}
		}
	}

	Ogre2RTTexture::~Ogre2RTTexture()
	{
	}

	void Ogre2RTTexture::begin()
	{
		Ogre::RenderSystem* system = Ogre::Root::getSingleton().getRenderSystem();
		if (mRenderPassDesc == nullptr)
		{
			mRenderPassDesc = system->createRenderPassDescriptor();
			mRenderPassDesc->mColour[0].texture = mTexture;
			mRenderPassDesc->mColour[0].loadAction = Ogre::LoadAction::Clear;
			mRenderPassDesc->mColour[0].storeAction = Ogre::StoreAction::StoreOrResolve;
			mRenderPassDesc->mColour[0].clearColour = Ogre::ColourValue::ZERO;
			mRenderPassDesc->mColour[0].resolveTexture = mTexture;
			mRenderPassDesc->mDepth.texture = system->getDepthBufferFor(mTexture, mTexture->getDepthBufferPoolId(), mTexture->getPreferDepthTexture(), mTexture->getDesiredDepthBufferFormat());
			mRenderPassDesc->mDepth.loadAction = Ogre::LoadAction::Clear;
			mRenderPassDesc->mDepth.storeAction = Ogre::StoreAction::DontCare;
			mRenderPassDesc->mStencil.texture = nullptr;
			mRenderPassDesc->mStencil.loadAction = Ogre::LoadAction::Clear;
			mRenderPassDesc->mStencil.storeAction = Ogre::StoreAction::DontCare;
			mRenderPassDesc->entriesModified(Ogre::RenderPassDescriptor::All);
		}
		// no need to save RenderPassDesc
		//mSaveRenderPassDesc = system->getCurrentPassDescriptor();
// Attention: Here later with several view ports!
		Ogre::Vector4 viewportSize[1] = { Ogre::Vector4(0, 0, 1, 1) };
		Ogre::Vector4 scissors[1] = { Ogre::Vector4(0, 0, 1, 1) };
		system->beginRenderPassDescriptor(mRenderPassDesc, mTexture, 1, viewportSize, scissors, 1, false, false);
	}

	void Ogre2RTTexture::end()
	{
		// no need to restore previous Render Pass Descriptor (Ogre will restore it for us)
	}

	void Ogre2RTTexture::doRender(IVertexBuffer* _buffer, ITexture* _texture, size_t _count)
	{
		Ogre2RenderManager::getInstance().doRender(_buffer, _texture, _count);
	}

} // namespace MyGUI

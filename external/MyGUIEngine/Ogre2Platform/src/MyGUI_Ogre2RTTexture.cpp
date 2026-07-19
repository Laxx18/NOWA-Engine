/*!
	@file
	@author		Albert Semenov
	@date		10/2009
	@update		2026 v2 fixes for Ogre-Next by NOWA-Engine
*/

#include <cstring>
#include "MyGUI_Ogre2RTTexture.h"
#include "MyGUI_Ogre2RenderManager.h"

#include "OgreTextureGpu.h"
#include "OgreWindow.h"

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

			// NOTE: leftover from the v1 projection-matrix override; the v2
			// identity-projection path does not consume this matrix. If RTT
			// content appears vertically flipped on OpenGL, the flip must be
			// applied to the vertices/UVs instead.
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
		// v2 fix: the descriptor was leaked.
		if (mRenderPassDesc != nullptr)
		{
			Ogre::RenderSystem* system = Ogre::Root::getSingleton().getRenderSystem();
			system->destroyRenderPassDescriptor(mRenderPassDesc);
			mRenderPassDesc = nullptr;
		}
	}

	void Ogre2RTTexture::begin()
	{
		Ogre::RenderSystem* system = Ogre::Root::getSingleton().getRenderSystem();

		// Draw everything queued so far into the CURRENT target before we
		// switch to the RTT — otherwise pending window batches would land in
		// the canvas texture.
		Ogre2RenderManager::getInstance().flush();

		if (mRenderPassDesc == nullptr)
		{
			mRenderPassDesc = system->createRenderPassDescriptor();
			mRenderPassDesc->mColour[0].texture = mTexture;
			mRenderPassDesc->mColour[0].loadAction = Ogre::LoadAction::Clear;
			mRenderPassDesc->mColour[0].storeAction = Ogre::StoreAction::StoreOrResolve;
			mRenderPassDesc->mColour[0].clearColour = Ogre::ColourValue::ZERO;
			// v2 fix: resolveTexture must only be set for MSAA targets — some
			// backends assert on a resolve target for single-sample textures.
			if (mTexture->isMultisample())
			{
				mRenderPassDesc->mColour[0].resolveTexture = mTexture;
			}
			// v2 fix: no depth attachment. The GUI datablocks have depth
			// check/write disabled, so a depth buffer is pure waste here.
			mRenderPassDesc->mDepth.texture = nullptr;
			mRenderPassDesc->mStencil.texture = nullptr;
			mRenderPassDesc->entriesModified(Ogre::RenderPassDescriptor::All);
		}

		// Remember what was bound so end() can restore it.
		mSaveRenderPassDesc = system->getCurrentPassDescriptor();

		// Attention: Here later with several view ports!
		Ogre::Vector4 viewportSize[1] = { Ogre::Vector4(0, 0, 1, 1) };
		Ogre::Vector4 scissors[1] = { Ogre::Vector4(0, 0, 1, 1) };
		// v2 fix: mip level is 0 — the old code passed 1 and rendered into
		// the (usually non-existent) second mip.
		system->beginRenderPassDescriptor(mRenderPassDesc, mTexture, 0, viewportSize, scissors, 1, false, false);
	}

	void Ogre2RTTexture::end()
	{
		// Draw the batches queued between begin() and end() into the RTT.
		Ogre2RenderManager::getInstance().flush();

		// Restore the previously bound target (typically the window) so that
		// subsequent GUI batches of this frame land where they belong. Ogre
		// does NOT restore this for us.
		if (mSaveRenderPassDesc != nullptr)
		{
			Ogre::Window* window = Ogre2RenderManager::getInstance().getRenderWindow();
			if (window != nullptr)
			{
				Ogre::RenderSystem* system = Ogre::Root::getSingleton().getRenderSystem();
				Ogre::Vector4 viewportSize[1] = { Ogre::Vector4(0, 0, 1, 1) };
				Ogre::Vector4 scissors[1] = { Ogre::Vector4(0, 0, 1, 1) };
				system->beginRenderPassDescriptor(mSaveRenderPassDesc, window->getTexture(), 0, viewportSize, scissors, 1, false, false);
			}
			mSaveRenderPassDesc = nullptr;
		}
	}

	void Ogre2RTTexture::doRender(IVertexBuffer* _buffer, ITexture* _texture, size_t _count)
	{
		Ogre2RenderManager::getInstance().doRender(_buffer, _texture, _count);
	}

} // namespace MyGUI

/*!
	@file
	@author		Albert Semenov
	@date		10/2009
	@update		2026 v2 fixes for Ogre-Next by NOWA-Engine

	v2 design:
	 - The RenderManager defers all draws into the shared RenderQueue and flushes
	   it once per target. Therefore begin() first flushes any pending batches
	   into the CURRENT target, then binds this RTT's pass descriptor; end()
	   flushes the RTT batches into the RTT, then restores the previous
	   descriptor. Without this bracketing, batches queued "for" the RTT would
	   actually render into whatever target happens to be bound at flush time.
*/

#ifndef MYGUI_OGRE2_RTTEXTURE_H_
#define MYGUI_OGRE2_RTTEXTURE_H_

#include "MyGUI_Prerequest.h"
#include "MyGUI_IRenderTarget.h"

#include <Ogre.h>

#include "MyGUI_LastHeader.h"

namespace MyGUI
{

	class __declspec(dllexport) Ogre2RTTexture :
		public IRenderTarget
	{
	public:
		Ogre2RTTexture(Ogre::TextureGpu* _texture);
		virtual ~Ogre2RTTexture();

		virtual void begin();
		virtual void end();

		virtual void doRender(IVertexBuffer* _buffer, ITexture* _texture, size_t _count);

		virtual const RenderTargetInfo& getInfo()
		{
			return mRenderTargetInfo;
		}

	private:
		RenderTargetInfo mRenderTargetInfo;
		Ogre::RenderPassDescriptor* mRenderPassDesc;
		Ogre::RenderPassDescriptor* mSaveRenderPassDesc;
		Ogre::TextureGpu* mTexture;
		Ogre::Matrix4 mProjectMatrix;
	};

} // namespace MyGUI

#endif // MYGUI_OGRE_RTTEXTURE_H_

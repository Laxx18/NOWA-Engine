/*!
	@file
	@author		Albert Semenov
	@date		10/2009
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

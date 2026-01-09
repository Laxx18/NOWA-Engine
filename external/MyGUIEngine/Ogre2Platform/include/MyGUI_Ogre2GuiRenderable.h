#ifndef MYGUI_OGRE2_GUI_RENDERABLE_H_
#define MYGUI_OGRE2_GUI_RENDERABLE_H_

#include <OgrePrerequisites.h>
#include <OgreRenderOperation.h>
#include <OgreRenderable.h>
#include "OgreCommon.h"

namespace MyGUI
{
	class __declspec(dllexport) Ogre2GuiRenderable :
		public Ogre::Renderable
	{
	public:
		Ogre2GuiRenderable();
		virtual ~Ogre2GuiRenderable();

		virtual void setRenderOperation( const Ogre::v1::RenderOperation& op );
		virtual void getRenderOperation( Ogre::v1::RenderOperation& op, bool casterPass );

		virtual void getWorldTransforms( Ogre::Matrix4* xform ) const;

		virtual const Ogre::LightList& getLights( void ) const;

		Ogre::ParticleType::ParticleType getParticleType() const override
		{
			return Ogre::ParticleType::NotParticle;
		}

	private:
		Ogre::v1::RenderOperation mRenderOperation;
		mutable Ogre::Matrix4 mTransform;
	};

}
#endif // MYGUI_OGRE2_GUI_RENDERABLE_H_
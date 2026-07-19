#ifndef MYGUI_OGRE2_GUI_RENDERABLE_H_
#define MYGUI_OGRE2_GUI_RENDERABLE_H_

#include <OgrePrerequisites.h>
#include <OgreRenderable.h>
#include "OgreCommon.h"

namespace MyGUI
{
	/*
		v2 renderable for one MyGUI vertex buffer (owned by Ogre2VertexBuffer).

		 - The VAO is stored in Renderable::mVaoPerLod — the base class member
		   Ogre reads via getVaos() in calculateHashFor() and the FAST render
		   path. _setVao() must be called (by Ogre2VertexBuffer) BEFORE any
		   setDatablock() call, otherwise calculateHashFor() falls into the V1
		   path and calls getRenderOperation(), which throws.
		 - Identity view + identity projection: MyGUI emits vertices in clip
		   space; HlmsUnlit honors these flags (same mechanism as v1 overlays).
		 - getRenderOperation() throws like ManualObject2 — never reached when
		   the overlay queue runs in RenderQueue::FAST mode.
	*/
	class __declspec(dllexport) Ogre2GuiRenderable :
		public Ogre::Renderable
	{
	public:
		Ogre2GuiRenderable();
		virtual ~Ogre2GuiRenderable();

		// Assigns (or clears, with nullptr) the VAO in mVaoPerLod for both
		// VpNormal and VpShadow. Called by Ogre2VertexBuffer on create/resize.
		void _setVao(Ogre::VertexArrayObject* vao);

		// v1 legacy interface — not used in the v2 FAST path.
		virtual void getRenderOperation(Ogre::v1::RenderOperation& op, bool casterPass) override;
		virtual void getWorldTransforms(Ogre::Matrix4* xform) const override;

		virtual const Ogre::LightList& getLights(void) const override;

		Ogre::ParticleType::ParticleType getParticleType() const override
		{
			return Ogre::ParticleType::NotParticle;
		}
	};

}
#endif // MYGUI_OGRE2_GUI_RENDERABLE_H_

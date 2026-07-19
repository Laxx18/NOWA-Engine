#include "MyGUI_Ogre2GuiRenderable.h"

#include <OgreException.h>
#include <OgreLogManager.h>
#include <OgreStringConverter.h>
#include <Vao/OgreVertexArrayObject.h>

// TEMP: needed only for the diagnostic log below (RENDER_QUEUE_OVERLAY).
// Remove this include again together with the log once diagnosed.
#include "MyGUI_Ogre2RenderManager.h"

namespace MyGUI
{
	Ogre2GuiRenderable::Ogre2GuiRenderable()
	{
		// MyGUI vertices are already in clip space — bypass view & projection.
		// HlmsUnlit honors both flags (same path as v1 overlay rendering).
		setUseIdentityView(true);
		setUseIdentityProjection(true);
	}

	Ogre2GuiRenderable::~Ogre2GuiRenderable()
	{}

	void Ogre2GuiRenderable::_setVao(Ogre::VertexArrayObject* vao)
	{
		mVaoPerLod[Ogre::VpNormal].clear();
		mVaoPerLod[Ogre::VpShadow].clear();

		if (vao != nullptr)
		{
			mVaoPerLod[Ogre::VpNormal].push_back(vao);
			mVaoPerLod[Ogre::VpShadow].push_back(vao);
		}
	}

	void Ogre2GuiRenderable::getRenderOperation(Ogre::v1::RenderOperation& /*op*/, bool /*casterPass*/)
	{
		OGRE_EXCEPT(Ogre::Exception::ERR_NOT_IMPLEMENTED,
			"Ogre2GuiRenderable does not implement getRenderOperation. "
			"The MyGUI overlay render queue must run in RenderQueue::FAST mode "
			"(see Ogre2RenderManager::setSceneManager).",
			"Ogre2GuiRenderable::getRenderOperation");
	}

	void Ogre2GuiRenderable::getWorldTransforms(Ogre::Matrix4* xform) const
	{
		// Not called in the v2 FAST path; kept correct for tooling.
		// (The old version assigned the parameter pointer itself — a no-op.)
		*xform = Ogre::Matrix4::IDENTITY;
	}

	const Ogre::LightList& Ogre2GuiRenderable::getLights(void) const
	{
		static Ogre::LightList ll;
		return ll;
	}

}
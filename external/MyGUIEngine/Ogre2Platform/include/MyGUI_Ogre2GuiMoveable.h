#ifndef MYGUI_OGRE2_GUI_MOVEABLE_H_
#define MYGUI_OGRE2_GUI_MOVEABLE_H_

// v2 fix: do NOT include MyGUI_Ogre2RenderManager.h here — it includes this
// header back (circular include that only compiled by guard-order luck).
// This class needs nothing from the RenderManager.
#include <OgreMovableObject.h>

namespace MyGUI
{
	/*
		Dummy MovableObject the GUI renderables are attached to. It exists only
		so addRenderableV2() has a MovableObject providing a (identity) world
		transform via its parent scene node. It is never culled — batches are
		added to the render queue directly — so its AABB is irrelevant.
	*/
	class __declspec(dllexport) Ogre2GuiMoveable :
		public Ogre::MovableObject
	{

	public:
		Ogre2GuiMoveable(Ogre::IdType id, Ogre::ObjectMemoryManager *objectMemoryManager,
						 Ogre::SceneManager* manager, Ogre::uint8 renderQueueId) :
			MovableObject(id, objectMemoryManager, manager, renderQueueId )
		{
		}

		virtual ~Ogre2GuiMoveable()
		{
		}

		virtual const Ogre::String& getMovableType( void ) const
		{
			return Ogre::BLANKSTRING;
		}

	};
}

#endif // MYGUI_OGRE2_GUI_MOVEABLE_H_

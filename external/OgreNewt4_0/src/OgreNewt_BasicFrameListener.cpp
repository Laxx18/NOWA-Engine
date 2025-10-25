#include "OgreNewt_Stdafx.h"
#include "OgreNewt_World.h"
#include "OgreNewt_Debugger.h"
#include "OgreNewt_BasicFrameListener.h"

#ifdef __APPLE__
#   include <Ogre/OgreFrameListener.h>
#   include <Ogre/OgreRenderWindow.h>
#else
#   include <OgreFrameListener.h>
#   include <OgreRenderWindow.h>
#endif

#ifdef _DEBUG
	#include <OgreLogManager.h>
	#include <OgreStringConverter.h>
#endif

namespace OgreNewt
{

BasicFrameListener::BasicFrameListener( Ogre::Window* win, OgreNewt::World* W, int update_framerate) :
        FrameListener()
{
    m_World = W;

	// set the game fps
	m_World->setUpdateFPS(static_cast<Ogre::Real>(update_framerate), 5);
}

BasicFrameListener::~BasicFrameListener(void)
{
}

bool BasicFrameListener::frameStarted(const Ogre::FrameEvent &evt)
{
	m_World->update (evt.timeSinceLastFrame);
    return true;
}


}   // end NAMESPACE OgreNewt

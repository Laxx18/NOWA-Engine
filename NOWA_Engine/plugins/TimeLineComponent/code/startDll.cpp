#include "NOWAPrecompiled.h"
#include "TimeLineComponent.h"

// Plugin Code
NOWA::TimeLineComponent* pTimeLineComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pTimeLineComponent = new NOWA::TimeLineComponent();
	Ogre::Root::getSingleton().installPlugin(pTimeLineComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pTimeLineComponent);
	delete pTimeLineComponent;
	pTimeLineComponent = static_cast<NOWA::TimeLineComponent*>(0);
}
#include "NOWAPrecompiled.h"
#include "FollowTargetComponent.h"

// Plugin Code
NOWA::FollowTargetComponent* pFollowTargetComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pFollowTargetComponent = new NOWA::FollowTargetComponent();
	Ogre::Root::getSingleton().installPlugin(pFollowTargetComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pFollowTargetComponent);
	delete pFollowTargetComponent;
	pFollowTargetComponent = static_cast<NOWA::FollowTargetComponent*>(0);
}
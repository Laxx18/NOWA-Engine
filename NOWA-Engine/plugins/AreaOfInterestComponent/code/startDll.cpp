#include "NOWAPrecompiled.h"
#include "AreaOfInterestComponent.h"

// Plugin Code
NOWA::AreaOfInterestComponent* pAreaOfInterestComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pAreaOfInterestComponent = new NOWA::AreaOfInterestComponent();
	Ogre::Root::getSingleton().installPlugin(pAreaOfInterestComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pAreaOfInterestComponent);
	delete pAreaOfInterestComponent;
	pAreaOfInterestComponent = static_cast<NOWA::AreaOfInterestComponent*>(0);
}
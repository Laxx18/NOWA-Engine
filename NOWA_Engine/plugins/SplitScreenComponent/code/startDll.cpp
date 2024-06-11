#include "NOWAPrecompiled.h"
#include "SplitScreenComponent.h"

// Plugin Code
NOWA::SplitScreenComponent* pSplitScreenComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pSplitScreenComponent = new NOWA::SplitScreenComponent();
	Ogre::Root::getSingleton().installPlugin(pSplitScreenComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pSplitScreenComponent);
	delete pSplitScreenComponent;
	pSplitScreenComponent = static_cast<NOWA::SplitScreenComponent*>(0);
}
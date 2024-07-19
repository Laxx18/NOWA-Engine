#include "NOWAPrecompiled.h"
#include "MinimapComponent.h"

// Plugin Code
NOWA::MinimapComponent* pMinimapComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pMinimapComponent = new NOWA::MinimapComponent();
	Ogre::Root::getSingleton().installPlugin(pMinimapComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pMinimapComponent);
	delete pMinimapComponent;
	pMinimapComponent = static_cast<NOWA::MinimapComponent*>(0);
}
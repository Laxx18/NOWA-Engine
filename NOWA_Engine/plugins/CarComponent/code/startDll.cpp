#include "NOWAPrecompiled.h"
#include "CarComponent.h"

// Plugin Code
NOWA::CarComponent* pCarComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pCarComponent = new NOWA::CarComponent();
	Ogre::Root::getSingleton().installPlugin(pCarComponent);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pCarComponent);
	delete pCarComponent;
	pCarComponent = static_cast<NOWA::CarComponent*>(0);
}
#include "NOWAPrecompiled.h"
#include "LightAreaOfInterestComponent.h"

// Plugin Code
NOWA::LightAreaOfInterestComponent* pLightAreaOfInterestComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pLightAreaOfInterestComponent = new NOWA::LightAreaOfInterestComponent();
	Ogre::Root::getSingleton().installPlugin(pLightAreaOfInterestComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pLightAreaOfInterestComponent);
	delete pLightAreaOfInterestComponent;
	pLightAreaOfInterestComponent = static_cast<NOWA::LightAreaOfInterestComponent*>(0);
}
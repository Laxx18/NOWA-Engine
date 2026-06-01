#include "NOWAPrecompiled.h"
#include "PlanetSunComponent.h"

// Plugin Code
NOWA::PlanetSunComponent* pPlanetSunComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pPlanetSunComponent = new NOWA::PlanetSunComponent();
	Ogre::Root::getSingleton().installPlugin(pPlanetSunComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pPlanetSunComponent);
	delete pPlanetSunComponent;
	pPlanetSunComponent = static_cast<NOWA::PlanetSunComponent*>(0);
}
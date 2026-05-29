#include "NOWAPrecompiled.h"
#include "PlanetOceanComponent.h"

// Plugin Code
NOWA::PlanetOceanComponent* pPlanetOceanComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pPlanetOceanComponent = new NOWA::PlanetOceanComponent();
	Ogre::Root::getSingleton().installPlugin(pPlanetOceanComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pPlanetOceanComponent);
	delete pPlanetOceanComponent;
	pPlanetOceanComponent = static_cast<NOWA::PlanetOceanComponent*>(0);
}
#include "NOWAPrecompiled.h"
#include "PlanetSurfaceComponent.h"

// Plugin Code
NOWA::PlanetSurfaceComponent* pPlanetSurfaceComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pPlanetSurfaceComponent = new NOWA::PlanetSurfaceComponent();
	Ogre::Root::getSingleton().installPlugin(pPlanetSurfaceComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pPlanetSurfaceComponent);
	delete pPlanetSurfaceComponent;
	pPlanetSurfaceComponent = static_cast<NOWA::PlanetSurfaceComponent*>(0);
}
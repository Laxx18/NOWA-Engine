#include "NOWAPrecompiled.h"
#include "PlanetMinimapComponent.h"

// Plugin Code
NOWA::PlanetMinimapComponent* pPlanetMinimapComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pPlanetMinimapComponent = new NOWA::PlanetMinimapComponent();
	Ogre::Root::getSingleton().installPlugin(pPlanetMinimapComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pPlanetMinimapComponent);
	delete pPlanetMinimapComponent;
	pPlanetMinimapComponent = static_cast<NOWA::PlanetMinimapComponent*>(0);
}
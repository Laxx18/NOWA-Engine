#include "NOWAPrecompiled.h"
#include "PlanetTerraComponent.h"

// Plugin Code
NOWA::PlanetTerraComponent* pPlanetTerraComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pPlanetTerraComponent = new NOWA::PlanetTerraComponent();
	Ogre::Root::getSingleton().installPlugin(pPlanetTerraComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pPlanetTerraComponent);
	delete pPlanetTerraComponent;
	pPlanetTerraComponent = static_cast<NOWA::PlanetTerraComponent*>(0);
}
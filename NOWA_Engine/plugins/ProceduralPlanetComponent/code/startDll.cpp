#include "NOWAPrecompiled.h"
#include "ProceduralPlanetComponent.h"

// Plugin Code
NOWA::ProceduralPlanetComponent* pProceduralPlanetComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pProceduralPlanetComponent = new NOWA::ProceduralPlanetComponent();
	Ogre::Root::getSingleton().installPlugin(pProceduralPlanetComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pProceduralPlanetComponent);
	delete pProceduralPlanetComponent;
	pProceduralPlanetComponent = static_cast<NOWA::ProceduralPlanetComponent*>(0);
}
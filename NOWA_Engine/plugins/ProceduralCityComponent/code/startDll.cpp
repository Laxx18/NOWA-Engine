#include "NOWAPrecompiled.h"
#include "ProceduralCityComponent.h"

// Plugin Code
NOWA::ProceduralCityComponent* pProceduralCityComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pProceduralCityComponent = new NOWA::ProceduralCityComponent();
	Ogre::Root::getSingleton().installPlugin(pProceduralCityComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pProceduralCityComponent);
	delete pProceduralCityComponent;
	pProceduralCityComponent = static_cast<NOWA::ProceduralCityComponent*>(0);
}
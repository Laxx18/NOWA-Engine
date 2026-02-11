#include "NOWAPrecompiled.h"
#include "ProceduralRoadComponent.h"

// Plugin Code
NOWA::ProceduralRoadComponent* pProceduralRoadComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pProceduralRoadComponent = new NOWA::ProceduralRoadComponent();
	Ogre::Root::getSingleton().installPlugin(pProceduralRoadComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pProceduralRoadComponent);
	delete pProceduralRoadComponent;
	pProceduralRoadComponent = static_cast<NOWA::ProceduralRoadComponent*>(0);
}
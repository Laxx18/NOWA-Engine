#include "NOWAPrecompiled.h"
#include "ProceduralStairsComponent.h"

// Plugin Code
NOWA::ProceduralStairsComponent* pProceduralStairsComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pProceduralStairsComponent = new NOWA::ProceduralStairsComponent();
	Ogre::Root::getSingleton().installPlugin(pProceduralStairsComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pProceduralStairsComponent);
	delete pProceduralStairsComponent;
	pProceduralStairsComponent = static_cast<NOWA::ProceduralStairsComponent*>(0);
}
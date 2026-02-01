#include "NOWAPrecompiled.h"
#include "ProceduralTerrainCreationComponent.h"

// Plugin Code
NOWA::ProceduralTerrainCreationComponent* pProceduralTerrainCreationComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pProceduralTerrainCreationComponent = new NOWA::ProceduralTerrainCreationComponent();
	Ogre::Root::getSingleton().installPlugin(pProceduralTerrainCreationComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pProceduralTerrainCreationComponent);
	delete pProceduralTerrainCreationComponent;
	pProceduralTerrainCreationComponent = static_cast<NOWA::ProceduralTerrainCreationComponent*>(0);
}
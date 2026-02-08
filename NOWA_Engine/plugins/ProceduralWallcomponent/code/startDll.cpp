#include "NOWAPrecompiled.h"
#include "ProceduralWallComponent.h"

// Plugin Code
NOWA::ProceduralWallComponent* pProceduralWallComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pProceduralWallComponent = new NOWA::ProceduralWallComponent();
	Ogre::Root::getSingleton().installPlugin(pProceduralWallComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pProceduralWallComponent);
	delete pProceduralWallComponent;
	pProceduralWallComponent = static_cast<NOWA::ProceduralWallComponent*>(0);
}
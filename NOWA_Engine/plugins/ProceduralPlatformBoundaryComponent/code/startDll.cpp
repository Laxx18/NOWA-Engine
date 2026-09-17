#include "NOWAPrecompiled.h"
#include "ProceduralPlatformBoundaryComponent.h"

// Plugin Code
NOWA::ProceduralPlatformBoundaryComponent* pProceduralPlatformBoundaryComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pProceduralPlatformBoundaryComponent = new NOWA::ProceduralPlatformBoundaryComponent();
	Ogre::Root::getSingleton().installPlugin(pProceduralPlatformBoundaryComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pProceduralPlatformBoundaryComponent);
	delete pProceduralPlatformBoundaryComponent;
	pProceduralPlatformBoundaryComponent = static_cast<NOWA::ProceduralPlatformBoundaryComponent*>(0);
}
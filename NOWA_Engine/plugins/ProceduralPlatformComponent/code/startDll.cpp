#include "NOWAPrecompiled.h"
#include "ProceduralPlatformComponent.h"

// Plugin Code
NOWA::ProceduralPlatformComponent* pProceduralPlatformComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pProceduralPlatformComponent = new NOWA::ProceduralPlatformComponent();
	Ogre::Root::getSingleton().installPlugin(pProceduralPlatformComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pProceduralPlatformComponent);
	delete pProceduralPlatformComponent;
	pProceduralPlatformComponent = static_cast<NOWA::ProceduralPlatformComponent*>(0);
}
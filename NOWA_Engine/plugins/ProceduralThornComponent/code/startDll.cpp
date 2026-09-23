#include "NOWAPrecompiled.h"
#include "ProceduralThornComponent.h"

// Plugin Code
NOWA::ProceduralThornComponent* pProceduralThornComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pProceduralThornComponent = new NOWA::ProceduralThornComponent();
	Ogre::Root::getSingleton().installPlugin(pProceduralThornComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pProceduralThornComponent);
	delete pProceduralThornComponent;
	pProceduralThornComponent = static_cast<NOWA::ProceduralThornComponent*>(0);
}
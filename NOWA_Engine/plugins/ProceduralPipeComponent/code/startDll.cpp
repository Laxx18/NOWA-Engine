#include "NOWAPrecompiled.h"
#include "ProceduralPipeComponent.h"

// Plugin Code
NOWA::ProceduralPipeComponent* pProceduralPipeComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pProceduralPipeComponent = new NOWA::ProceduralPipeComponent();
	Ogre::Root::getSingleton().installPlugin(pProceduralPipeComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pProceduralPipeComponent);
	delete pProceduralPipeComponent;
	pProceduralPipeComponent = static_cast<NOWA::ProceduralPipeComponent*>(0);
}
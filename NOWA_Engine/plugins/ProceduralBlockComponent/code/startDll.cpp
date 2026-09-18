#include "NOWAPrecompiled.h"
#include "ProceduralBlockComponent.h"

// Plugin Code
NOWA::ProceduralBlockComponent* pProceduralBlockComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pProceduralBlockComponent = new NOWA::ProceduralBlockComponent();
	Ogre::Root::getSingleton().installPlugin(pProceduralBlockComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pProceduralBlockComponent);
	delete pProceduralBlockComponent;
	pProceduralBlockComponent = static_cast<NOWA::ProceduralBlockComponent*>(0);
}
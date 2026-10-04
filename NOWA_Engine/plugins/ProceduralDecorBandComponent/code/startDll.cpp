#include "NOWAPrecompiled.h"
#include "ProceduralDecorBandComponent.h"

// Plugin Code
NOWA::ProceduralDecorBandComponent* pProceduralDecorBandComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pProceduralDecorBandComponent = new NOWA::ProceduralDecorBandComponent();
	Ogre::Root::getSingleton().installPlugin(pProceduralDecorBandComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pProceduralDecorBandComponent);
	delete pProceduralDecorBandComponent;
	pProceduralDecorBandComponent = static_cast<NOWA::ProceduralDecorBandComponent*>(0);
}
#include "NOWAPrecompiled.h"
#include "ProceduralConveyorLoopComponent.h"

// Plugin Code
NOWA::ProceduralConveyorLoopComponent* pProceduralConveyorLoopComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pProceduralConveyorLoopComponent = new NOWA::ProceduralConveyorLoopComponent();
	Ogre::Root::getSingleton().installPlugin(pProceduralConveyorLoopComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pProceduralConveyorLoopComponent);
	delete pProceduralConveyorLoopComponent;
	pProceduralConveyorLoopComponent = static_cast<NOWA::ProceduralConveyorLoopComponent*>(0);
}
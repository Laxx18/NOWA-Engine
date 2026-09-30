#include "NOWAPrecompiled.h"
#include "ExplorationMapComponent.h"

// Plugin Code
NOWA::ExplorationMapComponent* pExplorationMapComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pExplorationMapComponent = new NOWA::ExplorationMapComponent();
	Ogre::Root::getSingleton().installPlugin(pExplorationMapComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pExplorationMapComponent);
	delete pExplorationMapComponent;
	pExplorationMapComponent = static_cast<NOWA::ExplorationMapComponent*>(0);
}
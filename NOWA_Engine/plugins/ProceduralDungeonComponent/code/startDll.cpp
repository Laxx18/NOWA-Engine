#include "NOWAPrecompiled.h"
#include "ProceduralDungeonComponent.h"

// Plugin Code
NOWA::ProceduralDungeonComponent* pProceduralDungeonComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pProceduralDungeonComponent = new NOWA::ProceduralDungeonComponent();
	Ogre::Root::getSingleton().installPlugin(pProceduralDungeonComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pProceduralDungeonComponent);
	delete pProceduralDungeonComponent;
	pProceduralDungeonComponent = static_cast<NOWA::ProceduralDungeonComponent*>(0);
}
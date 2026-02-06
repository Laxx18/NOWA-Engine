#include "NOWAPrecompiled.h"
#include "ProceduralMazeComponent.h"

// Plugin Code
NOWA::ProceduralMazeComponent* pProceduralMazeComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pProceduralMazeComponent = new NOWA::ProceduralMazeComponent();
	Ogre::Root::getSingleton().installPlugin(pProceduralMazeComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pProceduralMazeComponent);
	delete pProceduralMazeComponent;
	pProceduralMazeComponent = static_cast<NOWA::ProceduralMazeComponent*>(0);
}
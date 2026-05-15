#include "NOWAPrecompiled.h"
#include "ProceduralGeometryComponent.h"

// Plugin Code
NOWA::ProceduralGeometryComponent* pProceduralGeometryComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pProceduralGeometryComponent = new NOWA::ProceduralGeometryComponent();
	Ogre::Root::getSingleton().installPlugin(pProceduralGeometryComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pProceduralGeometryComponent);
	delete pProceduralGeometryComponent;
	pProceduralGeometryComponent = static_cast<NOWA::ProceduralGeometryComponent*>(0);
}
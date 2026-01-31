#include "NOWAPrecompiled.h"
#include "PccPerPixelGridPlacementComponent.h"

// Plugin Code
NOWA::PccPerPixelGridPlacementComponent* pPccPerPixelGridPlacementComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pPccPerPixelGridPlacementComponent = new NOWA::PccPerPixelGridPlacementComponent();
	Ogre::Root::getSingleton().installPlugin(pPccPerPixelGridPlacementComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pPccPerPixelGridPlacementComponent);
	delete pPccPerPixelGridPlacementComponent;
	pPccPerPixelGridPlacementComponent = static_cast<NOWA::PccPerPixelGridPlacementComponent*>(0);
}
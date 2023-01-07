#include "NOWAPrecompiled.h"
#include "VegetationComponent.h"

// Plugin Code
NOWA::VegetationComponent* pVegetationComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pVegetationComponent = new NOWA::VegetationComponent();
	Ogre::Root::getSingleton().installPlugin(pVegetationComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pVegetationComponent);
	delete pVegetationComponent;
	pVegetationComponent = static_cast<NOWA::VegetationComponent*>(0);
}
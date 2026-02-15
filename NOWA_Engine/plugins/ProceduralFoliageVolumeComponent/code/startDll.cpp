#include "NOWAPrecompiled.h"
#include "ProceduralFoliageVolumeComponent.h"

// Plugin Code
NOWA::ProceduralFoliageVolumeComponent* pProceduralFoliageVolumeComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pProceduralFoliageVolumeComponent = new NOWA::ProceduralFoliageVolumeComponent();
	Ogre::Root::getSingleton().installPlugin(pProceduralFoliageVolumeComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pProceduralFoliageVolumeComponent);
	delete pProceduralFoliageVolumeComponent;
	pProceduralFoliageVolumeComponent = static_cast<NOWA::ProceduralFoliageVolumeComponent*>(0);
}
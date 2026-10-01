#include "NOWAPrecompiled.h"
#include "WaterVolumeComponent.h"

// Plugin Code
NOWA::WaterVolumeComponent* pWaterVolumeComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pWaterVolumeComponent = new NOWA::WaterVolumeComponent();
	Ogre::Root::getSingleton().installPlugin(pWaterVolumeComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pWaterVolumeComponent);
	delete pWaterVolumeComponent;
	pWaterVolumeComponent = static_cast<NOWA::WaterVolumeComponent*>(0);
}
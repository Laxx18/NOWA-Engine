#include "NOWAPrecompiled.h"
#include "WaterFoamEffectComponent.h"

// Plugin Code
NOWA::WaterFoamEffectComponent* pWaterFoamEffectComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pWaterFoamEffectComponent = new NOWA::WaterFoamEffectComponent();
	Ogre::Root::getSingleton().installPlugin(pWaterFoamEffectComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pWaterFoamEffectComponent);
	delete pWaterFoamEffectComponent;
	pWaterFoamEffectComponent = static_cast<NOWA::WaterFoamEffectComponent*>(0);
}
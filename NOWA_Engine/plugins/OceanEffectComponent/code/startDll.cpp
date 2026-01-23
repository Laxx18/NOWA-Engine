#include "NOWAPrecompiled.h"
#include "OceanEffectComponent.h"

// Plugin Code
NOWA::OceanEffectComponent* pOceanEffectComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pOceanEffectComponent = new NOWA::OceanEffectComponent();
	Ogre::Root::getSingleton().installPlugin(pOceanEffectComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pOceanEffectComponent);
	delete pOceanEffectComponent;
	pOceanEffectComponent = static_cast<NOWA::OceanEffectComponent*>(0);
}
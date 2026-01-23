#include "NOWAPrecompiled.h"
#include "HdrEffectComponent.h"

// Plugin Code
NOWA::HdrEffectComponent* pHdrEffectComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pHdrEffectComponent = new NOWA::HdrEffectComponent();
	Ogre::Root::getSingleton().installPlugin(pHdrEffectComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pHdrEffectComponent);
	delete pHdrEffectComponent;
	pHdrEffectComponent = static_cast<NOWA::HdrEffectComponent*>(0);
}
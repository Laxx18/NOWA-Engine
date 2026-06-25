#include "NOWAPrecompiled.h"
#include "WindEffectComponent.h"

// Plugin Code
NOWA::WindEffectComponent* pWindEffectComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pWindEffectComponent = new NOWA::WindEffectComponent();
	Ogre::Root::getSingleton().installPlugin(pWindEffectComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pWindEffectComponent);
	delete pWindEffectComponent;
	pWindEffectComponent = static_cast<NOWA::WindEffectComponent*>(0);
}
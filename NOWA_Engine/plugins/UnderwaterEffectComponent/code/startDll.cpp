#include "NOWAPrecompiled.h"
#include "UnderwaterEffectComponent.h"

// Plugin Code
NOWA::UnderwaterEffectComponent* pUnderwaterEffectComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pUnderwaterEffectComponent = new NOWA::UnderwaterEffectComponent();
	Ogre::Root::getSingleton().installPlugin(pUnderwaterEffectComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pUnderwaterEffectComponent);
	delete pUnderwaterEffectComponent;
	pUnderwaterEffectComponent = static_cast<NOWA::UnderwaterEffectComponent*>(0);
}
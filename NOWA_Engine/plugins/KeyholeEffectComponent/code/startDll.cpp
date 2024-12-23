#include "NOWAPrecompiled.h"
#include "KeyholeEffectComponent.h"

// Plugin Code
NOWA::KeyholeEffectComponent* pKeyholeEffectComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pKeyholeEffectComponent = new NOWA::KeyholeEffectComponent();
	Ogre::Root::getSingleton().installPlugin(pKeyholeEffectComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pKeyholeEffectComponent);
	delete pKeyholeEffectComponent;
	pKeyholeEffectComponent = static_cast<NOWA::KeyholeEffectComponent*>(0);
}
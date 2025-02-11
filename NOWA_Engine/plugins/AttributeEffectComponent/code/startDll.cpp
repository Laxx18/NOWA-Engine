#include "NOWAPrecompiled.h"
#include "AttributeEffectComponent.h"

// Plugin Code
NOWA::AttributeEffectComponent* pAttributeEffectComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pAttributeEffectComponent = new NOWA::AttributeEffectComponent();
	Ogre::Root::getSingleton().installPlugin(pAttributeEffectComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pAttributeEffectComponent);
	delete pAttributeEffectComponent;
	pAttributeEffectComponent = static_cast<NOWA::AttributeEffectComponent*>(0);
}
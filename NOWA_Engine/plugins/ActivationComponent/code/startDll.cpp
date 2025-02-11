#include "NOWAPrecompiled.h"
#include "ActivationComponent.h"

// Plugin Code
NOWA::ActivationComponent* pActivationComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pActivationComponent = new NOWA::ActivationComponent();
	Ogre::Root::getSingleton().installPlugin(pActivationComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pActivationComponent);
	delete pActivationComponent;
	pActivationComponent = static_cast<NOWA::ActivationComponent*>(0);
}
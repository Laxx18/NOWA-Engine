#include "NOWAPrecompiled.h"
#include "BlinkComponent.h"

// Plugin Code
NOWA::BlinkComponent* pBlinkComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pBlinkComponent = new NOWA::BlinkComponent();
	Ogre::Root::getSingleton().installPlugin(pBlinkComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pBlinkComponent);
	delete pBlinkComponent;
	pBlinkComponent = static_cast<NOWA::BlinkComponent*>(0);
}
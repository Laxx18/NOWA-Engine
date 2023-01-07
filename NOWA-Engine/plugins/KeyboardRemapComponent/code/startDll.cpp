#include "NOWAPrecompiled.h"
#include "KeyboardRemapComponent.h"

// Plugin Code
NOWA::KeyboardRemapComponent* pKeyboardRemapComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pKeyboardRemapComponent = new NOWA::KeyboardRemapComponent();
	Ogre::Root::getSingleton().installPlugin(pKeyboardRemapComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pKeyboardRemapComponent);
	delete pKeyboardRemapComponent;
	pKeyboardRemapComponent = static_cast<NOWA::KeyboardRemapComponent*>(0);
}
#include "NOWAPrecompiled.h"
#include "VirtualKeyboardComponent.h"

// Plugin Code
NOWA::VirtualKeyboardComponent* pVirtualKeyboardComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pVirtualKeyboardComponent = new NOWA::VirtualKeyboardComponent();
	Ogre::Root::getSingleton().installPlugin(pVirtualKeyboardComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pVirtualKeyboardComponent);
	delete pVirtualKeyboardComponent;
	pVirtualKeyboardComponent = static_cast<NOWA::VirtualKeyboardComponent*>(0);
}
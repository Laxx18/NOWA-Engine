#include "NOWAPrecompiled.h"
#include "InputDeviceComponent.h"

// Plugin Code
NOWA::InputDeviceComponent* pInputDeviceComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pInputDeviceComponent = new NOWA::InputDeviceComponent();
	Ogre::Root::getSingleton().installPlugin(pInputDeviceComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pInputDeviceComponent);
	delete pInputDeviceComponent;
	pInputDeviceComponent = static_cast<NOWA::InputDeviceComponent*>(0);
}
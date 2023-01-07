#include "NOWAPrecompiled.h"
#include "JoystickRemapComponent.h"

// Plugin Code
NOWA::JoystickRemapComponent* pJoystickRemapComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pJoystickRemapComponent = new NOWA::JoystickRemapComponent();
	Ogre::Root::getSingleton().installPlugin(pJoystickRemapComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pJoystickRemapComponent);
	delete pJoystickRemapComponent;
	pJoystickRemapComponent = static_cast<NOWA::JoystickRemapComponent*>(0);
}
#include "NOWAPrecompiled.h"
#include "JoystickConfigurationComponent.h"

// Plugin Code
NOWA::JoystickConfigurationComponent* pJoystickConfigurationComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pJoystickConfigurationComponent = new NOWA::JoystickConfigurationComponent();
	Ogre::Root::getSingleton().installPlugin(pJoystickConfigurationComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pJoystickConfigurationComponent);
	delete pJoystickConfigurationComponent;
	pJoystickConfigurationComponent = static_cast<NOWA::JoystickConfigurationComponent*>(0);
}
#include "NOWAPrecompiled.h"
#include "KeyboardConfigurationComponent.h"

// Plugin Code
NOWA::KeyboardConfigurationComponent* pKeyboardConfigurationComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pKeyboardConfigurationComponent = new NOWA::KeyboardConfigurationComponent();
	Ogre::Root::getSingleton().installPlugin(pKeyboardConfigurationComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pKeyboardConfigurationComponent);
	delete pKeyboardConfigurationComponent;
	pKeyboardConfigurationComponent = static_cast<NOWA::KeyboardConfigurationComponent*>(0);
}
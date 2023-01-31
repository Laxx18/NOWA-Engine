#include "NOWAPrecompiled.h"
#include "PickerComponent.h"

// Plugin Code
NOWA::PickerComponent* pPickerComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pPickerComponent = new NOWA::PickerComponent();
	Ogre::Root::getSingleton().installPlugin(pPickerComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pPickerComponent);
	delete pPickerComponent;
	pPickerComponent = static_cast<NOWA::PickerComponent*>(0);
}
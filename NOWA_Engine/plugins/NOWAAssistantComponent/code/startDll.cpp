#include "NOWAPrecompiled.h"
#include "NOWAAssistantComponent.h"

// Plugin Code
NOWA::NOWAAssistantComponent* pNOWAAssistantComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pNOWAAssistantComponent = new NOWA::NOWAAssistantComponent();
	Ogre::Root::getSingleton().installPlugin(pNOWAAssistantComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pNOWAAssistantComponent);
	delete pNOWAAssistantComponent;
	pNOWAAssistantComponent = static_cast<NOWA::NOWAAssistantComponent*>(0);
}
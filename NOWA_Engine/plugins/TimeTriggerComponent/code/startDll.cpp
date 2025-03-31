#include "NOWAPrecompiled.h"
#include "TimeTriggerComponent.h"

// Plugin Code
NOWA::TimeTriggerComponent* pTimeTriggerComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pTimeTriggerComponent = new NOWA::TimeTriggerComponent();
	Ogre::Root::getSingleton().installPlugin(pTimeTriggerComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pTimeTriggerComponent);
	delete pTimeTriggerComponent;
	pTimeTriggerComponent = static_cast<NOWA::TimeTriggerComponent*>(0);
}
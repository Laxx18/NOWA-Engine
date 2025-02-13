#include "NOWAPrecompiled.h"
#include "CameraBehaviorAttachComponent.h"

// Plugin Code
NOWA::CameraBehaviorAttachComponent* pCameraBehaviorAttachComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pCameraBehaviorAttachComponent = new NOWA::CameraBehaviorAttachComponent();
	Ogre::Root::getSingleton().installPlugin(pCameraBehaviorAttachComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pCameraBehaviorAttachComponent);
	delete pCameraBehaviorAttachComponent;
	pCameraBehaviorAttachComponent = static_cast<NOWA::CameraBehaviorAttachComponent*>(0);
}
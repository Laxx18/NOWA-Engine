#include "NOWAPrecompiled.h"
#include "SelectGameObjectsComponent.h"

// Plugin Code
NOWA::SelectGameObjectsComponent* pSelectGameObjectsComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pSelectGameObjectsComponent = new NOWA::SelectGameObjectsComponent();
	Ogre::Root::getSingleton().installPlugin(pSelectGameObjectsComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pSelectGameObjectsComponent);
	delete pSelectGameObjectsComponent;
	pSelectGameObjectsComponent = static_cast<NOWA::SelectGameObjectsComponent*>(0);
}
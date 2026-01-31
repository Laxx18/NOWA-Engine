#include "NOWAPrecompiled.h"
#include "InstantRadiosityComponent.h"

// Plugin Code
NOWA::InstantRadiosityComponent* pInstantRadiosityComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pInstantRadiosityComponent = new NOWA::InstantRadiosityComponent();
	Ogre::Root::getSingleton().installPlugin(pInstantRadiosityComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pInstantRadiosityComponent);
	delete pInstantRadiosityComponent;
	pInstantRadiosityComponent = static_cast<NOWA::InstantRadiosityComponent*>(0);
}
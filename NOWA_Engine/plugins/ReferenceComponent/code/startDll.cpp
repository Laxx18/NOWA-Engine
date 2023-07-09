#include "NOWAPrecompiled.h"
#include "ReferenceComponent.h"

// Plugin Code
NOWA::ReferenceComponent* pReferenceComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pReferenceComponent = new NOWA::ReferenceComponent();
	Ogre::Root::getSingleton().installPlugin(pReferenceComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pReferenceComponent);
	delete pReferenceComponent;
	pReferenceComponent = static_cast<NOWA::ReferenceComponent*>(0);
}
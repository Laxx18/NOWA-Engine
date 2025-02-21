#include "NOWAPrecompiled.h"
#include "LookAfterComponent.h"

// Plugin Code
NOWA::LookAfterComponent* pLookAfterComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pLookAfterComponent = new NOWA::LookAfterComponent();
	Ogre::Root::getSingleton().installPlugin(pLookAfterComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pLookAfterComponent);
	delete pLookAfterComponent;
	pLookAfterComponent = static_cast<NOWA::LookAfterComponent*>(0);
}
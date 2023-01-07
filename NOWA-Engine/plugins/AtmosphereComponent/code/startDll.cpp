#include "NOWAPrecompiled.h"
#include "AtmosphereComponent.h"

// Plugin Code
NOWA::AtmosphereComponent* pAtmosphereComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pAtmosphereComponent = new NOWA::AtmosphereComponent();
	Ogre::Root::getSingleton().installPlugin(pAtmosphereComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pAtmosphereComponent);
	delete pAtmosphereComponent;
	pAtmosphereComponent = static_cast<NOWA::AtmosphereComponent*>(0);
}
#include "NOWAPrecompiled.h"
#include "UniversumComponent.h"

// Plugin Code
NOWA::UniversumComponent* pUniversumComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pUniversumComponent = new NOWA::UniversumComponent();
	Ogre::Root::getSingleton().installPlugin(pUniversumComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pUniversumComponent);
	delete pUniversumComponent;
	pUniversumComponent = static_cast<NOWA::UniversumComponent*>(0);
}
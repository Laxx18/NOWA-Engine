#include "NOWAPrecompiled.h"
#include "DistortionComponent.h"

// Plugin Code
NOWA::DistortionComponent* pDistortionComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pDistortionComponent = new NOWA::DistortionComponent();
	Ogre::Root::getSingleton().installPlugin(pDistortionComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pDistortionComponent);
	delete pDistortionComponent;
	pDistortionComponent = static_cast<NOWA::DistortionComponent*>(0);
}
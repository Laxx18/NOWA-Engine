#include "NOWAPrecompiled.h"
#include "IESLightComponent.h"

// Plugin Code
NOWA::IESLightComponent* pIESLightComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pIESLightComponent = new NOWA::IESLightComponent();
	Ogre::Root::getSingleton().installPlugin(pIESLightComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pIESLightComponent);
	delete pIESLightComponent;
	pIESLightComponent = static_cast<NOWA::IESLightComponent*>(0);
}
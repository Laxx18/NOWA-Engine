#include "NOWAPrecompiled.h"
#include "SSAOComponent.h"

// Plugin Code
NOWA::SSAOComponent* pSSAOComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pSSAOComponent = new NOWA::SSAOComponent();
	Ogre::Root::getSingleton().installPlugin(pSSAOComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pSSAOComponent);
	delete pSSAOComponent;
	pSSAOComponent = static_cast<NOWA::SSAOComponent*>(0);
}
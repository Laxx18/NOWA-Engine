#include "NOWAPrecompiled.h"
#include "PurePursuitComponent.h"

// Plugin Code
NOWA::PurePursuitComponent* pPurePursuitComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pPurePursuitComponent = new NOWA::PurePursuitComponent();
	Ogre::Root::getSingleton().installPlugin(pPurePursuitComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pPurePursuitComponent);
	delete pPurePursuitComponent;
	pPurePursuitComponent = static_cast<NOWA::PurePursuitComponent*>(0);
}
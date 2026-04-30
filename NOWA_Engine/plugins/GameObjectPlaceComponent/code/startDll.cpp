#include "NOWAPrecompiled.h"
#include "GameObjectPlaceComponent.h"

// Plugin Code
NOWA::GameObjectPlaceComponent* pGameObjectPlaceComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pGameObjectPlaceComponent = new NOWA::GameObjectPlaceComponent();
	Ogre::Root::getSingleton().installPlugin(pGameObjectPlaceComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pGameObjectPlaceComponent);
	delete pGameObjectPlaceComponent;
	pGameObjectPlaceComponent = static_cast<NOWA::GameObjectPlaceComponent*>(0);
}
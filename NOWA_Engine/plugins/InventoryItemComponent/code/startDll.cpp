#include "NOWAPrecompiled.h"
#include "InventoryItemComponent.h"

// Plugin Code
NOWA::InventoryItemComponent* pInventoryItemComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pInventoryItemComponent = new NOWA::InventoryItemComponent();
	Ogre::Root::getSingleton().installPlugin(pInventoryItemComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pInventoryItemComponent);
	delete pInventoryItemComponent;
	pInventoryItemComponent = static_cast<NOWA::InventoryItemComponent*>(0);
}
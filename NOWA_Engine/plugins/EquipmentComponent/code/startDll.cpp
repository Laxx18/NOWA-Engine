#include "NOWAPrecompiled.h"
#include "EquipmentComponent.h"

// Plugin Code
NOWA::EquipmentComponent* pEquipmentComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pEquipmentComponent = new NOWA::EquipmentComponent();
	Ogre::Root::getSingleton().installPlugin(pEquipmentComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pEquipmentComponent);
	delete pEquipmentComponent;
	pEquipmentComponent = static_cast<NOWA::EquipmentComponent*>(0);
}
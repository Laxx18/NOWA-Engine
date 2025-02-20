#include "NOWAPrecompiled.h"
#include "BillboardComponent.h"

// Plugin Code
NOWA::BillboardComponent* pBillboardComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pBillboardComponent = new NOWA::BillboardComponent();
	Ogre::Root::getSingleton().installPlugin(pBillboardComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pBillboardComponent);
	delete pBillboardComponent;
	pBillboardComponent = static_cast<NOWA::BillboardComponent*>(0);
}
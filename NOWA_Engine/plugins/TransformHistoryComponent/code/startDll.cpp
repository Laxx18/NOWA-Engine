#include "NOWAPrecompiled.h"
#include "TransformHistoryComponent.h"

// Plugin Code
NOWA::TransformHistoryComponent* pTransformHistoryComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pTransformHistoryComponent = new NOWA::TransformHistoryComponent();
	Ogre::Root::getSingleton().installPlugin(pTransformHistoryComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pTransformHistoryComponent);
	delete pTransformHistoryComponent;
	pTransformHistoryComponent = static_cast<NOWA::TransformHistoryComponent*>(0);
}
#include "NOWAPrecompiled.h"
#include "TransformEaseComponent.h"

// Plugin Code
NOWA::TransformEaseComponent* pTransformEaseComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pTransformEaseComponent = new NOWA::TransformEaseComponent();
	Ogre::Root::getSingleton().installPlugin(pTransformEaseComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pTransformEaseComponent);
	delete pTransformEaseComponent;
	pTransformEaseComponent = static_cast<NOWA::TransformEaseComponent*>(0);
}
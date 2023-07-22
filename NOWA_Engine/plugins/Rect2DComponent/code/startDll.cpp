#include "NOWAPrecompiled.h"
#include "Rect2DComponent.h"

// Plugin Code
NOWA::Rect2DComponent* pRect2DComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pRect2DComponent = new NOWA::Rect2DComponent();
	Ogre::Root::getSingleton().installPlugin(pRect2DComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pRect2DComponent);
	delete pRect2DComponent;
	pRect2DComponent = static_cast<NOWA::Rect2DComponent*>(0);
}
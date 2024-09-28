#include "NOWAPrecompiled.h"
#include "MyGuiSpriteComponent.h"

// Plugin Code
NOWA::MyGuiSpriteComponent* pMyGuiSpriteComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pMyGuiSpriteComponent = new NOWA::MyGuiSpriteComponent();
	Ogre::Root::getSingleton().installPlugin(pMyGuiSpriteComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pMyGuiSpriteComponent);
	delete pMyGuiSpriteComponent;
	pMyGuiSpriteComponent = static_cast<NOWA::MyGuiSpriteComponent*>(0);
}
#include "NOWAPrecompiled.h"
#include "MorphAnimationComponent.h"

// Plugin Code
NOWA::MorphAnimationComponent* pMorphAnimationComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pMorphAnimationComponent = new NOWA::MorphAnimationComponent();
	Ogre::Root::getSingleton().installPlugin(pMorphAnimationComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pMorphAnimationComponent);
	delete pMorphAnimationComponent;
	pMorphAnimationComponent = static_cast<NOWA::MorphAnimationComponent*>(0);
}
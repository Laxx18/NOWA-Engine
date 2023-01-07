#include "NOWAPrecompiled.h"
#include "AnimationSequenceComponent.h"

// Plugin Code
NOWA::AnimationSequenceComponent* pAnimationSequenceComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pAnimationSequenceComponent = new NOWA::AnimationSequenceComponent();
	Ogre::Root::getSingleton().installPlugin(pAnimationSequenceComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pAnimationSequenceComponent);
	delete pAnimationSequenceComponent;
	pAnimationSequenceComponent = static_cast<NOWA::AnimationSequenceComponent*>(0);
}
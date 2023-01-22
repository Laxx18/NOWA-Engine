#include "NOWAPrecompiled.h"
#include "AnimationComponentV2.h"

// Plugin Code
NOWA::AnimationComponentV2* pAnimationComponentV2;

extern "C" EXPORTED void dllStartPlugin()
{
	pAnimationComponentV2 = new NOWA::AnimationComponentV2();
	Ogre::Root::getSingleton().installPlugin(pAnimationComponentV2, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pAnimationComponentV2);
	delete pAnimationComponentV2;
	pAnimationComponentV2 = static_cast<NOWA::AnimationComponentV2*>(0);
}
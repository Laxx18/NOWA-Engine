#include "NOWAPrecompiled.h"
#include "SpeechBubbleComponent.h"

// Plugin Code
NOWA::SpeechBubbleComponent* pSpeechBubbleComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pSpeechBubbleComponent = new NOWA::SpeechBubbleComponent();
	Ogre::Root::getSingleton().installPlugin(pSpeechBubbleComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pSpeechBubbleComponent);
	delete pSpeechBubbleComponent;
	pSpeechBubbleComponent = static_cast<NOWA::SpeechBubbleComponent*>(0);
}
#include "NOWAPrecompiled.h"
#include "RandomImageShuffler.h"

// Plugin Code
NOWA::RandomImageShuffler* pRandomImageShuffler;

extern "C" EXPORTED void dllStartPlugin()
{
	pRandomImageShuffler = new NOWA::RandomImageShuffler();
	Ogre::Root::getSingleton().installPlugin(pRandomImageShuffler, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pRandomImageShuffler);
	delete pRandomImageShuffler;
	pRandomImageShuffler = static_cast<NOWA::RandomImageShuffler*>(0);
}
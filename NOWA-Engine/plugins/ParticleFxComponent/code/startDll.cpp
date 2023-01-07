#include "NOWAPrecompiled.h"
#include "ParticleFxComponent.h"

// Plugin Code
NOWA::ParticleFxComponent* pParticleFxComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pParticleFxComponent = new NOWA::ParticleFxComponent();
	Ogre::Root::getSingleton().installPlugin(pParticleFxComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pParticleFxComponent);
	delete pParticleFxComponent;
	pParticleFxComponent = static_cast<NOWA::ParticleFxComponent*>(0);
}
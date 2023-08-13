#include "NOWAPrecompiled.h"
#include "GpuParticlesComponent.h"

// Plugin Code
NOWA::GpuParticlesComponent* pGpuParticlesComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pGpuParticlesComponent = new NOWA::GpuParticlesComponent();
	Ogre::Root::getSingleton().installPlugin(pGpuParticlesComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pGpuParticlesComponent);
	delete pGpuParticlesComponent;
	pGpuParticlesComponent = static_cast<NOWA::GpuParticlesComponent*>(0);
}
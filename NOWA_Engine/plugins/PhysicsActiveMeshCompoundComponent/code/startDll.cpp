#include "NOWAPrecompiled.h"
#include "PhysicsActiveMeshCompoundComponent.h"

// Plugin Code
NOWA::PhysicsActiveMeshCompoundComponent* pPhysicsActiveMeshCompoundComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pPhysicsActiveMeshCompoundComponent = new NOWA::PhysicsActiveMeshCompoundComponent();
	Ogre::Root::getSingleton().installPlugin(pPhysicsActiveMeshCompoundComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pPhysicsActiveMeshCompoundComponent);
	delete pPhysicsActiveMeshCompoundComponent;
	pPhysicsActiveMeshCompoundComponent = static_cast<NOWA::PhysicsActiveMeshCompoundComponent*>(0);
}
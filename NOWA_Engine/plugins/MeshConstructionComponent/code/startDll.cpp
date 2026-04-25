#include "NOWAPrecompiled.h"
#include "MeshConstructionComponent.h"

// Plugin Code
NOWA::MeshConstructionComponent* pMeshConstructionComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pMeshConstructionComponent = new NOWA::MeshConstructionComponent();
	Ogre::Root::getSingleton().installPlugin(pMeshConstructionComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pMeshConstructionComponent);
	delete pMeshConstructionComponent;
	pMeshConstructionComponent = static_cast<NOWA::MeshConstructionComponent*>(0);
}
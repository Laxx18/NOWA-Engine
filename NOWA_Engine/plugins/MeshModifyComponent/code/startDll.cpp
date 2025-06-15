#include "NOWAPrecompiled.h"
#include "MeshModifyComponent.h"

// Plugin Code
NOWA::MeshModifyComponent* pMeshModifyComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pMeshModifyComponent = new NOWA::MeshModifyComponent();
	Ogre::Root::getSingleton().installPlugin(pMeshModifyComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pMeshModifyComponent);
	delete pMeshModifyComponent;
	pMeshModifyComponent = static_cast<NOWA::MeshModifyComponent*>(0);
}
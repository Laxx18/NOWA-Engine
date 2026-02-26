#include "NOWAPrecompiled.h"
#include "MeshEditComponent.h"

// Plugin Code
NOWA::MeshEditComponent* pMeshEditComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pMeshEditComponent = new NOWA::MeshEditComponent();
	Ogre::Root::getSingleton().installPlugin(pMeshEditComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pMeshEditComponent);
	delete pMeshEditComponent;
	pMeshEditComponent = static_cast<NOWA::MeshEditComponent*>(0);
}
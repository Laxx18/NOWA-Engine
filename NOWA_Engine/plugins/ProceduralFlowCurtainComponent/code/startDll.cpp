#include "NOWAPrecompiled.h"
#include "ProceduralFlowCurtainComponent.h"

// Plugin Code
NOWA::ProceduralFlowCurtainComponent* pProceduralFlowCurtainComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pProceduralFlowCurtainComponent = new NOWA::ProceduralFlowCurtainComponent();
	Ogre::Root::getSingleton().installPlugin(pProceduralFlowCurtainComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pProceduralFlowCurtainComponent);
	delete pProceduralFlowCurtainComponent;
	pProceduralFlowCurtainComponent = static_cast<NOWA::ProceduralFlowCurtainComponent*>(0);
}
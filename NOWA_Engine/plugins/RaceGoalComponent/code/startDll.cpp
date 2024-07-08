#include "NOWAPrecompiled.h"
#include "RaceGoalComponent.h"

// Plugin Code
NOWA::RaceGoalComponent* pRaceGoalComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pRaceGoalComponent = new NOWA::RaceGoalComponent();
	Ogre::Root::getSingleton().installPlugin(pRaceGoalComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pRaceGoalComponent);
	delete pRaceGoalComponent;
	pRaceGoalComponent = static_cast<NOWA::RaceGoalComponent*>(0);
}
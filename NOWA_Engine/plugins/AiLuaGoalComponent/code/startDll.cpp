#include "NOWAPrecompiled.h"
#include "AiLuaGoalComponent.h"

// Plugin Code
NOWA::AiLuaGoalComponent* pAiLuaGoalComponent;

extern "C" EXPORTED void dllStartPlugin()
{
	pAiLuaGoalComponent = new NOWA::AiLuaGoalComponent();
	Ogre::Root::getSingleton().installPlugin(pAiLuaGoalComponent, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pAiLuaGoalComponent);
	delete pAiLuaGoalComponent;
	pAiLuaGoalComponent = static_cast<NOWA::AiLuaGoalComponent*>(0);
}
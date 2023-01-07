#include "NOWAPrecompiled.h"
#include "PluginTemplate.h"

// Plugin Code
NOWA::PluginTemplate* pPluginTemplate;

extern "C" EXPORTED void dllStartPlugin()
{
	pPluginTemplate = new NOWA::PluginTemplate();
	Ogre::Root::getSingleton().installPlugin(pPluginTemplate, nullptr);
}

extern "C" EXPORTED void dllStopPlugin()
{
	Ogre::Root::getSingleton().uninstallPlugin(pPluginTemplate);
	delete pPluginTemplate;
	pPluginTemplate = static_cast<NOWA::PluginTemplate*>(0);
}
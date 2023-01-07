#include "NOWAPrecompiled.h"
#include "PluginFactory.h"

namespace NOWA
{
	PluginFactory::PluginFactory()
	{

	}

	PluginFactory::~PluginFactory()
	{
		this->pluginNames.clear();
	}

	void PluginFactory::loadPlugin(const Ogre::String& name) 
	{
		std::list<Ogre::String>::iterator it;
		for (it = this->pluginNames.begin(); it != this->pluginNames.end(); ++it)
		{
			if (*it == name)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_CRITICAL, "[PluginFactory] Plugin with the name '" + name + "' already exists. Skipping registration.");
				return;
			}
		}

		Ogre::Root::getSingleton().loadPlugin(name, true, nullptr);
		//Push the singleton and push it to a map (Info: it has been created in the macro!)
		this->pluginNames.push_back(name);
	}

}; //namespace end
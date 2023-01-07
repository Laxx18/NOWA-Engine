#ifndef PLUGIN_MANAGER_H
#define PLUGIN_MANAGER_H

#include "defines.h"

namespace NOWA
{
	class EXPORTED PluginFactory
	{	
	public:
		PluginFactory();
		~PluginFactory();
		void loadPlugin(const Ogre::String& name);
	private:
		std::list<Ogre::String> pluginNames;
	};

}; //namespace end

#endif
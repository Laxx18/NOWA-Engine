#include "ModuleFactory.h"

namespace OBA
{
	
	ModuleFactory::ModuleFactory()
	{
	}

	ModuleFactory::~ModuleFactory()
	{
		std::map<Ogre::String, IModule*>::iterator it;
		for (it = this->modules.begin(); it != this->modules.end(); ++it)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage("[ModuleFactory] hier");
			IModule* pModule = it->second;
			if (pModule != NULL)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage("[ModuleFactory] Module: " + pModule->getName() + " destroyed.");
				pModule->destroyModule();
				pModule = NULL;
			}
		}
		this->modules.clear();
	}

	bool ModuleFactory::moduleAlreadyCreatedCheck(IModule* pModule)
	{
		std::map<Ogre::String, IModule*>::iterator it;
		for (it = this->modules.begin(); it != this->modules.end(); ++it)
		{
			IModule* pModule = it->second;
			if (pModule != NULL)
			{
				return true;
			}
		}
		return false;
	}

	void ModuleFactory::manageModule(IModule* pModule)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage("[ModuleFactory] manageModule created");
		std::map<Ogre::String, IModule*>::iterator it;
		for (it = this->modules.begin(); it != this->modules.end(); ++it)
		{
			IModule* pModule = it->second;
			if (pModule == NULL)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage("[ModuleFactory] Module: " + pModule->getName() + " loaded.");
				this->modules.insert(std::pair<Ogre::String, IModule*>(pModule->getName(), pModule));
			} 
			else
			{
				Ogre::LogManager::getSingletonPtr()->logMessage("[ModuleFactory] Skipping Module singleton: " + pModule->getName() + " loading. Since the module is already active.");
			}
		}
	}

}; //namespace end
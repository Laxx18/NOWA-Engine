#ifndef DEPLOY_RESOURCE_MODULE_H
#define DEPLOY_RESOURCE_MODULE_H

#include "defines.h"

namespace NOWA
{
	class EXPORTED DeployResourceModule
	{
	public:
		void tagResource(const Ogre::String& name, const Ogre::String& resourceGroupName, Ogre::String& path = Ogre::String());

		void removeResource(const Ogre::String& name);

		std::pair<Ogre::String, Ogre::String> getPathAndResourceGroupFromDatablock(const Ogre::String& datablockName, Ogre::HlmsTypes type);

		Ogre::String getResourceGroupName(const Ogre::String& name) const;

		Ogre::String getResourcePath(const Ogre::String& name) const;

		void deploy(const Ogre::String& applicationName, const Ogre::String& jsonFilePathName);

		bool createCPlusPlusProject(const Ogre::String& projectName, const Ogre::String& worldName);

		bool createCPlusPlusComponentPluginProject(const Ogre::String& componentName);

		bool createSceneInOwnState(const Ogre::String& projectName, const Ogre::String& worldName);

		void openProject(const Ogre::String& projectName);

		void openLog(void);

		bool startGame(const Ogre::String& projectName);

		bool createAndStartExecutable(const Ogre::String& projectName, const Ogre::String& worldName);

		bool createLuaInitScript(const Ogre::String& projectName);

		void destroyContent(void);
	public:
		static DeployResourceModule* getInstance();
	private:
		DeployResourceModule();
		~DeployResourceModule();
	private:
		std::map<Ogre::String, std::pair<Ogre::String, Ogre::String>> taggedResourceMap;
	};

}; //namespace end

#endif
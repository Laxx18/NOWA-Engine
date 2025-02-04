#ifndef DEPLOY_RESOURCE_MODULE_H
#define DEPLOY_RESOURCE_MODULE_H

#include "defines.h"
#include "main/Events.h"

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

		void createConfigFile(const Ogre::String& configurationFilePathName, const Ogre::String& applicationName);

		void deploy(const Ogre::String& applicationName, const Ogre::String& sceneName, const Ogre::String& projectFilePathName);

		Ogre::String removeDuplicateMaterials(const Ogre::String& json);

		void appendToExistingJson(const Ogre::String& filename, const Ogre::String& tempFilename);

		void saveTexturesCache(const Ogre::String& sceneFolderPathName);

		void loadTexturesCache(const Ogre::String& sceneFolderPathName);

		bool createCPlusPlusProject(const Ogre::String& projectName, const Ogre::String& sceneName);

		bool createCPlusPlusComponentPluginProject(const Ogre::String& componentName);

		bool createSceneInOwnState(const Ogre::String& projectName, const Ogre::String& sceneName);

		void openProject(const Ogre::String& projectName);

		void openLog(void);

		bool startGame(const Ogre::String& projectName);

		bool createAndStartExecutable(const Ogre::String& projectName, const Ogre::String& sceneName);

		bool createLuaInitScript(const Ogre::String& projectName);

		bool createProjectBackup(const Ogre::String& projectName, const Ogre::String& sceneName);

		void destroyContent(void);

		Ogre::String getCurrentComponentPluginFolder(void) const;

		bool checkIfInstanceRunning(void);

		bool openNOWALuaScriptEditor(const Ogre::String& filePathName);

		void monitorProcess(HANDLE processHandle);
	public:
		static DeployResourceModule* getInstance();
	private:
		DeployResourceModule();
		~DeployResourceModule();
	private:
		// Function to send the file path to the running instance
		bool sendFilePathToRunningInstance(const Ogre::String& filePathName);

		void deleteLuaRuntimeErrorXmlFiles(const Ogre::String& directoryPath);
	private:
		void handleLuaError(NOWA::EventDataPtr eventData);
	private:
		std::map<Ogre::String, std::pair<Ogre::String, Ogre::String>> taggedResourceMap;
		Ogre::String currentComponentPluginFolder;
		HWND hwndNOWALuaScript;
	};

}; //namespace end

#endif

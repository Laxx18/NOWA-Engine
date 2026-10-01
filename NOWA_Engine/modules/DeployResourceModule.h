#ifndef DEPLOY_RESOURCE_MODULE_H
#define DEPLOY_RESOURCE_MODULE_H

#include "defines.h"
#include "main/Events.h"

namespace NOWA
{
    class EXPORTED DeployResourceModule
    {
    public:
        void removeResource(const Ogre::String& name);

        std::pair<Ogre::String, Ogre::String> getPathAndResourceGroupFromDatablock(const Ogre::String& datablockName, Ogre::HlmsTypes type);

        Ogre::String getResourceGroupName(const Ogre::String& name) const;

        Ogre::String getResourcePath(const Ogre::String& name) const;

        /**
         * @brief		Writes the default resources cfg template. Only used as fallback, if the game has no own resources cfg.
         */
        void createConfigFile(const Ogre::String& configurationFilePathName, const Ogre::String& applicationName);

        /**
         * @brief		Deploys the game with only the resources it really uses into "<NOWA root>/deploy/<ProjectName>/" (bin/Release, bin/resources, media).
         *				All scenes of the project are analyzed from their files, no scene needs to be loaded.
         * @param[in]	projectName				The project name, which is also the name of the game executable.
         * @param[in]	projectFilePathName		The folder of the project (e.g. "../../media/Projects/PrehistoricLax").
         * @return		true, if the deploy finished. Warnings (missing files, missing Release build) are written to the log.
         */
        bool deployProject(const Ogre::String& projectName, const Ogre::String& projectFilePathName);

        /**
         * @brief		Compatibility wrapper for the former per scene deploy: only the call with isLastScene == true deploys (the whole project).
         */
        void deploy(const Ogre::String& applicationName, const Ogre::String& sceneName, const Ogre::String& projectFilePathName, bool isLastScene);

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

        bool writeDeployedResourcesConfig(const Ogre::String& projectName, const Ogre::String& deployRootPathName);

    private:
        void handleLuaError(NOWA::EventDataPtr eventData);

    private:
        std::map<Ogre::String, std::pair<Ogre::String, Ogre::String>> taggedResourceMap;
        Ogre::String currentComponentPluginFolder;
        HWND hwndNOWALuaScript;
    };

}; // namespace end

#endif
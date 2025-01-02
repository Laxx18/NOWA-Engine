#ifndef LUA_SCRIPT_MODULE_H
#define LUA_SCRIPT_MODULE_H

#include "defines.h"
#include "modules/LuaScript.h"

namespace NOWA
{
	class EXPORTED LuaScriptModule
	{
	public:
		friend class AppState; // Only AppState may create this class

		LuaScript* getScript(const Ogre::String& name);

		LuaScript* createScript(const Ogre::String& name, const Ogre::String& scriptName, bool isGlobal);

		LuaScript* createScript(const Ogre::String& name, const Ogre::String& scriptName, const Ogre::String& scriptContent, bool isGlobal);

		void destroyScript(const Ogre::String& name);

		void destroyScript(LuaScript* name);

		bool hasScript(const Ogre::String& name);

		void copyScript(const Ogre::String& sourceScriptName, const Ogre::String& targetScriptName, bool deleteSourceScript, bool isGlobal);

		void copyScriptAbsolutePath(const Ogre::String& sourceScriptFilePathName, const Ogre::String& targetScriptFilePathName, bool deleteSourceScript, bool isGlobal);

		/*
		* @brief	Checks whether the given lua script name does exist already.
		* @param[in] scriptName			The lua script name name to set.
		* @param[in] isGlobal			Sets whether the lua script is part of a global game object, which is used for all scenes in the project.
		*								In this case, the lua script will not be located in the scene folder, but in the parent project folder for all scenes.
		* @param[in] filePath			The optional file path, if empty, the default project file path will be taken.
		* @return	finalLuaScriptName	Returns true, if the lua script name does already exist, else false.
		*/
		bool checkLuaScriptFileExists(const Ogre::String& scriptName, bool isGlobal, const Ogre::String& filePath = Ogre::String());

		/*
		* @brief	Gets the validated lua script name, which is checked for naming collisions.
		* @param[in] scriptName			The lua script name name to set.
		* @param[in] isGlobal			Sets whether the lua script is part of a global game object, which is used for all scenes in the project.
		*								In this case, the lua script will not be located in the scene folder, but in the parent project folder for all scenes.
		* @param[in] filePath			The optional file path, if empty, the default project file path will be taken.
		* @return	finalLuaScriptName	Returns the final validated lua script name.
		*/
		Ogre::String getValidatedLuaScriptName(const Ogre::String& scriptName, bool isGlobal, const Ogre::String& filePath = Ogre::String());

		Ogre::String getScriptAdaptedContent(const Ogre::String& scriptName, const Ogre::String& targetModuleName, bool isGlobal, const Ogre::String& sourceModuleName = Ogre::String());

		Ogre::String getScriptAdaptedContentAbsolute(const Ogre::String& scriptFilePathName, const Ogre::String& targetModuleName, const Ogre::String& sourceModuleName = Ogre::String());

		void replaceIdsInScript(const Ogre::String& scriptFilePathName, const Ogre::String& sourceId, const Ogre::String& targetId);

		bool checkLuaFunctionAvailable(const Ogre::String& scriptName, const Ogre::String& functionName);

		bool checkLuaStateAvailable(const Ogre::String& scriptName, const Ogre::String& stateName);

		void generateLuaFunctionName(const Ogre::String& scriptName, const Ogre::String& functionName, bool isGlobal);

		void destroyContent(void);
	private:
		LuaScriptModule(const Ogre::String& appStateName);
		~LuaScriptModule();

		void internalCopyScript(const Ogre::String& sourceScriptFilePathName, const Ogre::String& targetScriptFilePathName, bool deleteSourceScript, bool isAbsolutePath, bool isGlobal);

		Ogre::String internalGetScriptAdaptedContent(const Ogre::String& scriptFilePathName, const Ogre::String& targetModuleName, const Ogre::String& sourceModuleName = Ogre::String());
	private:
		Ogre::String appStateName;
		std::unordered_map<Ogre::String, LuaScript*> scripts;
	};

}; //namespace end

#endif

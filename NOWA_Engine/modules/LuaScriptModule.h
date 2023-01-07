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

		LuaScript* createScript(const Ogre::String& name, const Ogre::String& scriptName);

		LuaScript* createScript(const Ogre::String& name, const Ogre::String& scriptName, const Ogre::String& scriptContent);

		void destroyScript(const Ogre::String& name);

		void destroyScript(LuaScript* name);

		bool hasScript(const Ogre::String& name);

		void copyScript(const Ogre::String& sourceScriptName, const Ogre::String& targetScriptName, bool deleteSourceScript);

		void copyScriptAbsolutePath(const Ogre::String& sourceScriptFilePathName, const Ogre::String& targetScriptFilePathName, bool deleteSourceScript);

		bool checkLuaScriptFileExists(const Ogre::String& scriptName, const Ogre::String& filePath = Ogre::String());

		Ogre::String getValidatedLuaScriptName(const Ogre::String& scriptName, const Ogre::String& filePath = Ogre::String());

		Ogre::String getScriptAdaptedContent(const Ogre::String& scriptName, const Ogre::String& targetModuleName, const Ogre::String& sourceModuleName = Ogre::String());

		Ogre::String getScriptAdaptedContentAbsolute(const Ogre::String& scriptFilePathName, const Ogre::String& targetModuleName, const Ogre::String& sourceModuleName = Ogre::String());

		void replaceIdsInScript(const Ogre::String& scriptFilePathName, const Ogre::String& sourceId, const Ogre::String& targetId);

		bool checkLuaFunctionAvailable(const Ogre::String& scriptName, const Ogre::String& functionName);

		bool checkLuaStateAvailable(const Ogre::String& scriptName, const Ogre::String& stateName);

		void generateLuaFunctionName(const Ogre::String& scriptName, const Ogre::String& functionName);

		void destroyContent(void);
	private:
		LuaScriptModule(const Ogre::String& appStateName);
		~LuaScriptModule();

		void internalCopyScript(const Ogre::String& sourceScriptFilePathName, const Ogre::String& targetScriptFilePathName, bool deleteSourceScript, bool isAbsolutePath);
		Ogre::String internalGetScriptAdaptedContent(const Ogre::String& scriptFilePathName, const Ogre::String& targetModuleName, const Ogre::String& sourceModuleName = Ogre::String());
	private:
		Ogre::String appStateName;
		std::unordered_map<Ogre::String, LuaScript*> scripts;
	};

}; //namespace end

#endif

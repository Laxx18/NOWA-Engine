#ifndef LUA_SCRIPT_H_
#define LUA_SCRIPT_H_

#include "defines.h"
#include <luabind/luabind.hpp>
#include <sstream>

namespace NOWA
{
	class EXPORTED LuaScript
	{
	public:
		LuaScript(const Ogre::String& name, const Ogre::String& scriptPathName, const Ogre::String& scriptContent = Ogre::String());

		~LuaScript();

	public:
		inline const Ogre::String& getScriptName(void)
		{
			return this->scriptPathName;
		}

		inline const Ogre::String& getName(void)
		{
			return this->name;
		}

		inline bool isCompiled(void)
		{
			return this->compiled;
		}

		inline const luabind::object& getCompiledScriptReference(void)
		{
			return this->compiledScript;
		}

		inline const luabind::object& getCompiledStateScriptReference(void)
		{
			return this->compiledStateScript;
		}

		void setScriptFile(const Ogre::String& scriptFile);

		void setScriptContent(const Ogre::String& scriptContent);

		Ogre::String getScriptContent(void) const;

		void setInterfaceFunctionsTemplate(const Ogre::String& strInterfaceFunctionsTemplate);

		Ogre::String getInterfaceFunctionsTemplate(void) const;

		void callTableFunction(const Ogre::String& functionName)
		{
			if (true == functionName.empty())
			{
				return;
			}

			if (this->compiledScript.is_valid())
			{
				try
				{
					// Call the entry method of the new state
					auto& state = this->compiledScript[functionName];
					if (state)
						state();
				}
				catch (luabind::error& error)
				{
					this->printErrorMessage(functionName, error);
				}
			}
			else
			{
				Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'callTableFunction' for function name: '" 
					+ functionName + "' because the script has not been compiled and is invalid.");
			}
		}

		template <typename PARAM1>
		void callTableFunction(const Ogre::String& functionName, PARAM1 param1)
		{
			if (true == functionName.empty())
			{
				return;
			}

			if (this->compiledScript.is_valid())
			{
				try
				{
					// Call the entry method of the new state
					auto& function = this->compiledScript[functionName];
					if (function)
						function(param1);
				}
				catch (luabind::error& error)
				{
					this->printErrorMessage(functionName, error);
				}
			}
			else
			{
				Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'callTableFunction' for function name: '"
					+ functionName + "' because the script has not been compiled and is invalid.");
			}
		}

		template <typename PARAM1, typename PARAM2>
		void callTableFunction(const Ogre::String& functionName, PARAM1 param1, PARAM2 param2)
		{
			if (true == functionName.empty())
			{
				return;
			}

			if (this->compiledScript.is_valid())
			{
				try
				{
					// Call the entry method of the new state
					auto& function = this->compiledScript[functionName];
					if (function)
						function(param1, param2);
				}
				catch (luabind::error& error)
				{
					this->printErrorMessage(functionName, error);
				}
			}
			else
			{
				Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'callTableFunction' for function name: '"
					+ functionName + "' because the script has not been compiled and is invalid.");
			}
		}

		template <typename PARAM1, typename PARAM2, typename PARAM3>
		void callTableFunction(const Ogre::String& functionName, PARAM1 param1, PARAM2 param2, PARAM3 param3)
		{
			if (true == functionName.empty())
			{
				return;
			}

			if (this->compiledScript.is_valid())
			{
				try
				{
					// Call the entry method of the new state
					auto& function = this->compiledScript[functionName];
					if (function)
						function(param1, param2, param3);
				}
				catch (luabind::error& error)
				{
					this->printErrorMessage(functionName, error);
				}
			}
			else
			{
				Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'callTableFunction' for function name: '"
					+ functionName + "' because the script has not been compiled and is invalid.");
			}

		}

		template <typename PARAM1, typename PARAM2, typename PARAM3, typename PARAM4>
		void callTableFunction(const Ogre::String& functionName, PARAM1 param1, PARAM2 param2, PARAM3 param3, PARAM4 param4)
		{
			if (true == functionName.empty())
			{
				return;
			}

			if (this->compiledScript.is_valid())
			{
				try
				{
					// Call the entry method of the new state
					auto& function = this->compiledScript[functionName];
					if (function)
						function(param1, param2, param3, param4);
				}
				catch (luabind::error& error)
				{
					this->printErrorMessage(functionName, error);
				}
			}
			else
			{
				Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'callTableFunction' for function name: '"
					+ functionName + "' because the script has not been compiled and is invalid.");
			}
		}

		template <typename PARAM1, typename PARAM2, typename PARAM3, typename PARAM4, typename PARAM5>
		void callTableFunction(const Ogre::String& functionName, PARAM1 param1, PARAM2 param2, PARAM3 param3, PARAM4 param4, PARAM5 param5)
		{
			if (true == functionName.empty())
			{
				return;
			}

			if (this->compiledScript.is_valid())
			{
				try
				{
					// Call the entry method of the new state
					auto& function = this->compiledScript[functionName];
					if (function)
						function(param1, param2, param3, param4, param5);
				}
				catch (luabind::error& error)
				{
					this->printErrorMessage(functionName, error);
				}
			}
			else
			{
				Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'callTableFunction' for function name: '"
					+ functionName + "' because the script has not been compiled and is invalid.");
			}
		}

		template <typename PARAM1, typename PARAM2, typename PARAM3, typename PARAM4, typename PARAM5, typename PARAM6>
		void callTableFunction(const Ogre::String& functionName, PARAM1 param1, PARAM2 param2, PARAM3 param3, PARAM4 param4, PARAM5 param5, PARAM6 param6)
		{
			if (true == functionName.empty())
			{
				return;
			}

			if (this->compiledScript.is_valid())
			{
				try
				{
					// Call the entry method of the new state
					auto& function = this->compiledScript[functionName];
					if (function)
						function(param1, param2, param3, param4, param5, param6);
				}
				catch (luabind::error& error)
				{
					this->printErrorMessage(functionName, error);
				}
			}
			else
			{
				Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, "[LuaScript] Caught error in 'callTableFunction' for function name: '"
															+ functionName + "' because the script has not been compiled and is invalid.");
			}
		}

		void createLuaScriptFile(void);

		void compile(void);

		void decompile(void);

		bool hasCompileError(void) const;

		bool createLuaEnvironmentForFunction(const Ogre::String& functionName);

		bool createLuaEnvironmentForTable(const Ogre::String& tableName);

		bool createLuaEnvironmentForStateTable(const Ogre::String& tableName);

		void registerLuaApiTableFunction(const Ogre::String& className, const Ogre::String& description, const Ogre::String& functionName, const Ogre::String& param1 = Ogre::String(), 
			const Ogre::String& param2 = Ogre::String(), const Ogre::String& param3  = Ogre::String());

		void registerLuaApiTableReturnFunction(const Ogre::String& className, const Ogre::String& description, const Ogre::String& functionName, const Ogre::String& returnType, const Ogre::String& param1 = Ogre::String(), 
			const Ogre::String& param2 = Ogre::String(), const Ogre::String& param3 = Ogre::String());

	private:
		void errorHandler();

		bool checkLuaFile(const Ogre::String& luaScriptFilePathName);

		void printErrorMessage(const Ogre::String& functionName, luabind::error& error);
	private:
		Ogre::String scriptPathName;
		Ogre::String scriptName;
		Ogre::String scriptContent;
		const Ogre::String name;
		// int scriptReference;
		luabind::object compiledScript;
		luabind::object compiledStateScript;
		bool compiled;
		bool compileError;
		Ogre::String strInterfaceFunctionsTemplate;
		Ogre::String moduleName;
		std::set<Ogre::String> errorMessages;
	};

}; // namespace end

#endif


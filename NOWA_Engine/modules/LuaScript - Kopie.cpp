#include "NOWAPrecompiled.h"
#include "LuaScriptApi.h"
#include "LuaScript.h"
#include "main/Core.h"
#include "main/Events.h"
#include <stdlib.h>

namespace NOWA
{
	LuaScript::LuaScript(const Ogre::String& name, const Ogre::String& scriptPathName, bool isGlobal, const Ogre::String& scriptContent)
		: scriptPathName(scriptPathName),
		isGlobal(isGlobal),
		scriptContent(scriptContent),
		compiled(false),
		compileError(false),
		name(name)
	{

	}

	LuaScript::~LuaScript()
	{
		
	}

	void LuaScript::setIsGlobal(bool isGlobal)
	{
		this->isGlobal = isGlobal;

		if (true == AppStateManager::getSingletonPtr()->getLuaScriptModule()->checkLuaScriptFileExists(this->scriptName, isGlobal))
		{
			return;
		}

		// Cuts/Pastes the script if it does not exist either to parent project folder if its global, or to the deeper scene folder if its not global
		if (true == isGlobal)
		{
			Ogre::String sourceLuaScriptFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "/" + this->scriptPathName;
			Ogre::String destinationLuaScriptFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + this->scriptPathName;
			AppStateManager::getSingletonPtr()->getLuaScriptModule()->copyScriptAbsolutePath(sourceLuaScriptFilePathName, destinationLuaScriptFilePathName, true, isGlobal);
		}
		else
		{
			Ogre::String sourceLuaScriptFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + this->scriptPathName;
			Ogre::String destinationLuaScriptFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "/" + this->scriptPathName;
			AppStateManager::getSingletonPtr()->getLuaScriptModule()->copyScriptAbsolutePath(sourceLuaScriptFilePathName, destinationLuaScriptFilePathName, true, isGlobal);
		}

		boost::shared_ptr<NOWA::EventDataResourceCreated> eventDataResourceCreated(new NOWA::EventDataResourceCreated());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->triggerEvent(eventDataResourceCreated);
	}

	bool LuaScript::getIsGlobal(void) const
	{
		return this->isGlobal;
	}

#if 0
	void LuaScript::errorHandler()
	{
		this->compileError = true;
		lua_State* lua = LuaScriptApi::getInstance()->getLua();
		Ogre::String error = "[LuaScript]: " + this->scriptPathName + ":";
		error += lua_tostring(lua, -1);
		lua_pop(lua, 1);
		// throw std::runtime_error("[LuaScriptApi] Error: '" + error + "'");
		Ogre::LogManager::getSingletonPtr()->logMessage("[LuaScript] Could not compile script because of an error : '" + error + "'", Ogre::LML_CRITICAL);

		Ogre::String strLineNumber;
		Ogre::String toFind = "...\"]:";
		size_t foundStart = error.find(toFind);
		if (foundStart != Ogre::String::npos)
		{
			size_t foundEnd = error.find(":", foundStart + toFind.length());
			if (foundEnd != Ogre::String::npos)
			{
				strLineNumber = error.substr(foundStart + toFind.length(), foundEnd - (foundStart + toFind.length()));
			}
		}

		int lineNumber = -1;
		if (false == strLineNumber.empty())
		{
			lineNumber = Ogre::StringConverter::parseInt(strLineNumber);
		}

		boost::shared_ptr<EventDataPrintLuaError> eventDataPrintLuaError(new EventDataPrintLuaError(this->scriptPathName, this->scriptFilePathName, lineNumber, error));
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataPrintLuaError);

		// Sent event with feedback
		// boost::shared_ptr<EventDataFeedback> eventDataNavigationMeshFeedback(new EventDataFeedback(false, "#{LuaScriptCompileError} " + error));
		// NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataNavigationMeshFeedback);
	}
#endif

	void LuaScript::errorHandler()
	{
		auto done = std::make_shared<std::promise<void>>();
		std::future<void> future = done->get_future();

		NOWA::AppStateManager::LogicCommand logicCommand = [this, done]()
			{
				this->compileError = true;
				lua_State* lua = LuaScriptApi::getInstance()->getLua();
				Ogre::String error = "[LuaScript]: " + this->scriptPathName + ":";
				error += lua_tostring(lua, -1);
				lua_pop(lua, 1);

				Ogre::LogManager::getSingletonPtr()->logMessage(
					"[LuaScript] Could not compile script because of an error : '" + error + "'",
					Ogre::LML_CRITICAL
				);

				Ogre::String strLineNumber;
				Ogre::String toFind = "...\"]:";
				size_t foundStart = error.find(toFind);
				if (foundStart != Ogre::String::npos)
				{
					size_t foundEnd = error.find(":", foundStart + toFind.length());
					if (foundEnd != Ogre::String::npos)
					{
						strLineNumber = error.substr(foundStart + toFind.length(), foundEnd - (foundStart + toFind.length()));
					}
				}

				int lineNumber = -1;
				if (false == strLineNumber.empty())
				{
					lineNumber = Ogre::StringConverter::parseInt(strLineNumber);
				}

				boost::shared_ptr<EventDataPrintLuaError> eventDataPrintLuaError(
					new EventDataPrintLuaError(this->scriptPathName, this->scriptFilePathName, lineNumber, error));
				NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataPrintLuaError);

				done->set_value();
			};

		NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
		future.wait();
	}

#if 0
	bool LuaScript::createLuaEnvironmentForFunction(const Ogre::String& functionName)
	{
		// Only create if not done yet
		if (false == this->compiled)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage("[LuaScript]: Cannot create lua environment for function: " + functionName
				+ ", because the script: " + this->scriptPathName + " is not compiled.", Ogre::LML_CRITICAL);
			return false;
		}
		
		// see: http://lua-users.org/wiki/ModulesTutorial
		lua_getglobal(NOWA::LuaScriptApi::getInstance()->getLua(), this->moduleName.c_str());
		// Push the callback function name for the global module name context
		lua_pushstring(NOWA::LuaScriptApi::getInstance()->getLua(), functionName.c_str());
		// Silent Crash: when module is not defined in lua file, must open a lua file an check if module has been set!
		// So during compile method, a check is made, if the correct module declaration does exist
		lua_gettable(NOWA::LuaScriptApi::getInstance()->getLua(), -2);

		if (lua_isfunction(NOWA::LuaScriptApi::getInstance()->getLua(), -1))
		{
			luabind::object object(luabind::from_stack(NOWA::LuaScriptApi::getInstance()->getLua(), -1));
			this->compiledScript = object;
			return true;
		}
		return false;
	}
#endif

	bool LuaScript::createLuaEnvironmentForFunction(const Ogre::String& functionName)
	{
		if (false == this->compiled)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage("[LuaScript]: Cannot create lua environment for function: " + functionName
				+ ", because the script: " + this->scriptPathName + " is not compiled.", Ogre::LML_CRITICAL);
			return false;
		}

		bool result = false;

		auto done = std::make_shared<std::promise<void>>();
		std::future<void> future = done->get_future();

		NOWA::AppStateManager::LogicCommand logicCommand = [this, functionName, done, &result]()
		{
			// see: http://lua-users.org/wiki/ModulesTutorial
			lua_getglobal(NOWA::LuaScriptApi::getInstance()->getLua(), this->moduleName.c_str());

			lua_pushstring(NOWA::LuaScriptApi::getInstance()->getLua(), functionName.c_str());
			lua_gettable(NOWA::LuaScriptApi::getInstance()->getLua(), -2);

			if (lua_isfunction(NOWA::LuaScriptApi::getInstance()->getLua(), -1))
			{
				luabind::object object(luabind::from_stack(NOWA::LuaScriptApi::getInstance()->getLua(), -1));
				this->compiledScript = object;
				result = true;
			}
			else
			{
				result = false;
			}

			done->set_value();
		};

		NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
		future.wait();

		return result;
	}

#if 0
	bool LuaScript::createLuaEnvironmentForTable(const Ogre::String& tableName)
	{
		// Only create if not done yet
		if (false == this->compiled)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage("[LuaScript]: Cannot create lua environment for table: " + tableName
				+ ", because the script: " + this->scriptPathName + " is not compiled.", Ogre::LML_CRITICAL);
			return false;
		}

		// see: http://lua-users.org/wiki/ModulesTutorial
		lua_getglobal(NOWA::LuaScriptApi::getInstance()->getLua(), this->moduleName.c_str());

		if (lua_istable(NOWA::LuaScriptApi::getInstance()->getLua(), -1))
		{
			luabind::object object(luabind::from_stack(NOWA::LuaScriptApi::getInstance()->getLua(), -1));
			// Get the object table for the table name, to call functions for that table
			this->compiledScript = object[tableName];
			return true;
		}

		return false;
	}
#endif

	bool LuaScript::createLuaEnvironmentForTable(const Ogre::String& tableName)
	{
		if (!this->compiled)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(
				"[LuaScript]: Cannot create lua environment for table: " + tableName +
				", because the script: " + this->scriptPathName + " is not compiled.",
				Ogre::LML_CRITICAL
			);
			return false;
		}

		auto result = std::make_shared<bool>(false);
		auto done = std::make_shared<std::promise<void>>();
		std::future<void> future = done->get_future();

		NOWA::AppStateManager::LogicCommand logicCommand = [this, tableName, result, done]()
			{
				lua_State* L = NOWA::LuaScriptApi::getInstance()->getLua();

				lua_getglobal(L, this->moduleName.c_str());

				if (lua_istable(L, -1))
				{
					luabind::object object(luabind::from_stack(L, -1));
					this->compiledScript = object[tableName];
					*result = true;
				}
				else
				{
					*result = false;
				}

				done->set_value();
			};

		NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
		future.wait();

		return *result;
	}

#if 0
	bool LuaScript::createLuaEnvironmentForStateTable(const Ogre::String& tableName)
	{
		// Only create if not done yet
		if (false == this->compiled)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage("[LuaScript]: Cannot create lua environment for state table: " + tableName
				+ ", because the script: " + this->scriptPathName + " is not compiled.", Ogre::LML_CRITICAL);
			return false;
		}

		// Note: There are 2 objects in lua script
		// 1) For main functions (not included in states) like connect, disconnect, update, lateUpdate, +Functions from other components
		// 2) Just for states (See AiLuaComponent, PlayerControllerJumpNRunLuaComponent, etc.)
		// Main functions are actually also belonging to a state, the main state like e.g. PrehistoricLax_0 is a state and within the script, there is also a WalkState
		// So it must be possible to call main functions along with state functions! This can be done, when there are 2 objects living per script

		// see: http://lua-users.org/wiki/ModulesTutorial
		lua_getglobal(NOWA::LuaScriptApi::getInstance()->getLua(), this->moduleName.c_str());

		if (lua_istable(NOWA::LuaScriptApi::getInstance()->getLua(), -1))
		{
			luabind::object object(luabind::from_stack(NOWA::LuaScriptApi::getInstance()->getLua(), -1));
			// Get the object table for the table name, to call functions for that table
			this->compiledStateScript = object[tableName];
			return true;
		}

		return false;
	}
#endif

	bool LuaScript::createLuaEnvironmentForStateTable(const Ogre::String& tableName)
	{
		if (!this->compiled)
		{
			Ogre::LogManager::getSingletonPtr()->logMessage(
				"[LuaScript]: Cannot create lua environment for state table: " + tableName +
				", because the script: " + this->scriptPathName + " is not compiled.",
				Ogre::LML_CRITICAL
			);
			return false;
		}

		auto result = std::make_shared<bool>(false);
		auto done = std::make_shared<std::promise<void>>();
		std::future<void> future = done->get_future();

		NOWA::AppStateManager::LogicCommand logicCommand = [this, tableName, result, done]()
			{
				lua_State* L = NOWA::LuaScriptApi::getInstance()->getLua();

				lua_getglobal(L, this->moduleName.c_str());

				if (lua_istable(L, -1))
				{
					luabind::object object(luabind::from_stack(L, -1));
					this->compiledStateScript = object[tableName];
					*result = true;
				}
				else
				{
					*result = false;
				}

				done->set_value();
			};

		NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
		future.wait();

		return *result;
	}

	bool LuaScript::checkLuaFile(const Ogre::String& luaScriptFilePathName)
	{
		// http://lua-users.org/wiki/SampleCode
		// http://lua-users.org/wiki/ModulesTutorial --> unfortunately module is deprecated, but what is the replacement??
		// Get the module name without the extension from the lua script file
		this->moduleName = this->scriptPathName;
		size_t dotPos = this->moduleName.find_last_of(".");
		if (Ogre::String::npos != dotPos)
			this->moduleName.erase(dotPos, Ogre::String::npos);

		Ogre::String targetModuleName = "module(\"" + this->moduleName + "\"";
		Ogre::String fullTargetModuleCommand = targetModuleName + ", package.seeall);\n";

		// Check if the file does exist

		std::fstream luaFile(luaScriptFilePathName, std::ios::in);
		if (false == luaFile.good())
		{
			// If not, create the file silently and write the proper module name
			luaFile.open(luaScriptFilePathName, std::fstream::out);
			// Write the module name
			luaFile << fullTargetModuleCommand;
			// Write also the interface function template, if one has been specified
			if (false == this->strInterfaceFunctionsTemplate.empty())
			{
				luaFile << this->strInterfaceFunctionsTemplate;
			}
			luaFile.flush();
		}
		else
		{
			// Check if the lua file has a module with the script path name defined, because its really necessary! In order to call a call back function for lua
			// in its module context, so that a script will not override another one

			// Search for the module declaration in the lua script file
			Ogre::String line;
			bool foundModule = false;
			for (unsigned int curLine = 0; std::getline(luaFile, line); curLine++)
			{
				if (GetFileAttributes(luaScriptFilePathName.data()) & FileFlag)
				{
					line = Core::getSingletonPtr()->decode64(line, true);
				}

				size_t commentPos = line.find("--");
				size_t modulePos = line.find(targetModuleName);

				// if there is a module declaration and maybe a comment, but it must be after the module declaration and not before, else the module declaration would be commented out
				if (Ogre::String::npos != modulePos && commentPos > modulePos)
				{
					foundModule = true;
					break;
				}
			}

			if (false == foundModule)
			{
				Ogre::String error = "Cannot compile lua script: '" + this->scriptPathName
					+ "', because has no or not the correct module defined. Please define in the first line of your script the module: " + targetModuleName;
				Ogre::LogManager::getSingletonPtr()->logMessage("[LuaScript]: " + error, Ogre::LML_CRITICAL);

				luaFile.close();
				boost::shared_ptr<EventDataPrintLuaError> eventDataPrintLuaError(new EventDataPrintLuaError(this->scriptPathName, this->scriptFilePathName, 1, error));
				NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataPrintLuaError);
				return false;
			}
		}
		luaFile.close();

		return true;
	}

#if 0
	void LuaScript::compile(void)
	{
		lua_State* lua = LuaScriptApi::getInstance()->getLua();
		this->compileError = false;
		
		Ogre::String luaScriptFilePathName;
		if (false == this->isGlobal)
		{
			luaScriptFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "/" + this->scriptPathName;
		}
		else
		{
			luaScriptFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + this->scriptPathName;
		}

		if (false == this->checkLuaFile(luaScriptFilePathName))
		{
			return;
		}

		// https://stackoverflow.com/questions/4125971/setting-the-global-lua-path-variable-from-c-c
		// Add the lua script path to global package path, so that other lua scripts can be found when using 'require' like include
		// Must be: package.path = package.path.. ";D:/Ogre/GameEngineDevelopment/media/Worlds/SpaceGame/?.lua"
		Ogre::String projectPackagePath = Core::getSingletonPtr()->getAbsolutePath(Core::getSingletonPtr()->getCurrentProjectPath());
		LuaScriptApi::getInstance()->appendLuaFilePathToPackage(projectPackagePath);

		// Get the lua script content and replace the module name by a unique name, so that one script can be used by several game objects!
		// That is: There can be one lua script defined with a module name on the disc. Now when several GameObjects want to use the one script
		// The content is loaded and the module name replaced to a unique name for each GameObject and then the string is run via luaL_dostring
		// This is done each time, connect is called
		this->scriptContent = AppStateManager::getSingletonPtr()->getLuaScriptModule()->getScriptAdaptedContent(this->scriptPathName, this->name, this->isGlobal);
		this->moduleName = this->name;

		int result = 0;
		if (result = luaL_dostring(lua, this->scriptContent.c_str()))
		{
			this->errorHandler();
			return;
		}

		// Note: Is not used anymore
		// Load and execute the script
		/*if (result = luaL_dofile(lua, luaScriptFilePathName.c_str()))
		{
			this->errorHandler();
			return;
		}*/

		this->compiled = true;
	}
#endif

	void LuaScript::compile(void)
	{
		auto done = std::make_shared<std::promise<void>>();
		std::future<void> future = done->get_future();

		NOWA::AppStateManager::LogicCommand logicCommand = [this, done]()
			{
				lua_State* lua = LuaScriptApi::getInstance()->getLua();
				this->compileError = false;

				Ogre::String luaScriptFilePathName;
				if (!this->isGlobal)
				{
					luaScriptFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "/" + this->scriptPathName;
				}
				else
				{
					luaScriptFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + this->scriptPathName;
				}

				if (!this->checkLuaFile(luaScriptFilePathName))
				{
					done->set_value();
					return;
				}

				// Append lua script path to package.path for require()
				Ogre::String projectPackagePath = Core::getSingletonPtr()->getAbsolutePath(Core::getSingletonPtr()->getCurrentProjectPath());
				LuaScriptApi::getInstance()->appendLuaFilePathToPackage(projectPackagePath);

				// Prepare adapted script content and module name
				this->scriptContent = AppStateManager::getSingletonPtr()->getLuaScriptModule()->getScriptAdaptedContent(this->scriptPathName, this->name, this->isGlobal);
				this->moduleName = this->name;

				int result = luaL_dostring(lua, this->scriptContent.c_str());
				if (result)
				{
					this->errorHandler();
					done->set_value();
					return;
				}

				this->compiled = true;
				done->set_value();
			};

		NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
		future.wait();
	}

#if 0
	void LuaScript::decompile(void)
	{
		lua_State* lua = LuaScriptApi::getInstance()->getLua();

		Ogre::String luaScriptFilePathName;
		if (false == this->isGlobal)
		{
			luaScriptFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "/" + this->scriptPathName;
		}
		else
		{
			luaScriptFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + this->scriptPathName;
		}

		// If the script has the crypt file flags, encode again
		if (GetFileAttributes(luaScriptFilePathName.data()) & FileFlag && false == this->scriptContent.empty())
		{
			// Remove the game object id from module to save the original file again
			size_t modulePos = this->scriptContent.find("module(\"") + 8;

			// Get the length of the modulename inside the file
			size_t moduleEndPos = this->scriptContent.find("\"", modulePos);

			// e.g. "barrel_0" = 8, but "barrel_10" = 9
			size_t oldModuleLength = moduleEndPos - modulePos;

			this->scriptContent.replace(modulePos, oldModuleLength, this->scriptName);

			this->scriptContent = Core::getSingletonPtr()->encode64(this->scriptContent, true);

			// Write the encoded version back
			std::ofstream outFile(luaScriptFilePathName);
			if (true == outFile.good())
			{
				outFile << this->scriptContent;
				outFile.close();
			}
		}
		// lua_gc(lua, LUA_GCCOLLECT, 0); // Performance issues: https://github.com/urho3d/Urho3D/issues/2135
		lua_gc(lua, LUA_GCSTEP, 0);

		this->compiled = false;
	}
#endif

	void LuaScript::decompile(void)
	{
		auto done = std::make_shared<std::promise<void>>();
		std::future<void> future = done->get_future();

		NOWA::AppStateManager::LogicCommand logicCommand = [this, done]()
			{
				lua_State* lua = LuaScriptApi::getInstance()->getLua();

				Ogre::String luaScriptFilePathName;
				if (!this->isGlobal)
				{
					luaScriptFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "/" + this->scriptPathName;
				}
				else
				{
					luaScriptFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + this->scriptPathName;
				}

				// If the script has the crypt file flags, encode again
				if ((GetFileAttributes(luaScriptFilePathName.data()) & FileFlag) && !this->scriptContent.empty())
				{
					// Remove the game object id from module to save the original file again
					size_t modulePos = this->scriptContent.find("module(\"") + 8;

					// Get the length of the modulename inside the file
					size_t moduleEndPos = this->scriptContent.find("\"", modulePos);

					// e.g. "barrel_0" = 8, but "barrel_10" = 9
					size_t oldModuleLength = moduleEndPos - modulePos;

					this->scriptContent.replace(modulePos, oldModuleLength, this->scriptName);

					this->scriptContent = Core::getSingletonPtr()->encode64(this->scriptContent, true);

					// Write the encoded version back
					std::ofstream outFile(luaScriptFilePathName);
					if (outFile.good())
					{
						outFile << this->scriptContent;
						outFile.close();
					}
				}

				// Perform a Lua GC step
				lua_gc(lua, LUA_GCSTEP, 0);

				this->compiled = false;

				done->set_value();
			};

		NOWA::AppStateManager::getSingletonPtr()->enqueue(std::move(logicCommand));
		future.wait();
	}

	void LuaScript::setScriptFile(const Ogre::String& scriptFile)
	{
		this->scriptPathName = scriptFile;
		this->scriptName = scriptFile.substr(0, scriptFile.find_last_of("."));
	}

	void LuaScript::setScriptContent(const Ogre::String& scriptContent)
	{
		this->scriptContent = scriptContent;
	}

	void LuaScript::setScriptFilePathName(const Ogre::String& scriptFilePathName)
	{
		this->scriptFilePathName = scriptFilePathName;
	}

	Ogre::String LuaScript::getScriptFilePathName(void) const
	{
		return this->scriptFilePathName;
	}

	Ogre::String LuaScript::getScriptContent(void) const
	{
		return this->scriptContent;
	}

	bool LuaScript::hasCompileError(void) const
	{
		return this->compileError;
	}

	void LuaScript::setInterfaceFunctionsTemplate(const Ogre::String& strInterfaceFunctionsTemplate)
	{
		this->strInterfaceFunctionsTemplate = strInterfaceFunctionsTemplate;
	}

	Ogre::String LuaScript::getInterfaceFunctionsTemplate(void) const
	{
		return this->strInterfaceFunctionsTemplate;
	}

	void LuaScript::registerLuaApiTableFunction(const Ogre::String& className, const Ogre::String& description, const Ogre::String& functionName, const Ogre::String& param1, const Ogre::String& param2, const Ogre::String& param3)
	{
		Ogre::String args = "(";
		if (false == param1.empty())
		{
			args += param1;

			if (false == param2.empty())
			{
				args += ", " + param2;

				if (false == param3.empty())
				{
					args += ", " + param3;
				}
			}
		}
		args += ")";
		args = "void " + functionName + args;

		LuaScriptApi::getInstance()->addClassToCollection(className, args, description);
	}

	void LuaScript::registerLuaApiTableReturnFunction(const Ogre::String& className, const Ogre::String& description, const Ogre::String& functionName, const Ogre::String& returnType, const Ogre::String& param1, const Ogre::String& param2, const Ogre::String& param3)
	{
		Ogre::String args = returnType + " (";
		if (false == param1.empty())
		{
			args += param1;

			if (false == param2.empty())
			{
				args += ", " + param2;

				if (false == param3.empty())
				{
					args += ", " + param3;
				}
			}
		}
		args += ")";
		args = "void " + functionName + args;

		LuaScriptApi::getInstance()->addClassToCollection(className, args, description);
	}

	void LuaScript::printErrorMessage(const Ogre::String& functionName, luabind::error& error)
	{
		// LuaScriptApi seems do handle all those case also with a correct error line!
#if 0
		Ogre::String errorString;

		// Flooding prevention
		auto& found = errorMessages.find(functionName);
		if (found == errorMessages.cend())
		{
			errorMessages.emplace(functionName);
			luabind::object errorMsg(luabind::from_stack(error.state(), -1));
			std::stringstream msg;
			msg << errorMsg;

			errorString = "[LuaScript] Caught error in script: '" + this->scriptPathName + "' in 'callTableFunction' for function name: '"
				+ functionName + "' Error: " + Ogre::String(error.what())
				+ " details: " + msg.str();

			Ogre::LogManager::getSingleton().logMessage(Ogre::LML_CRITICAL, errorString);

			// Lua panic called, also in this case feedback event is required
			if ("lua runtime error" == Ogre::String(error.what()))
			{
				boost::shared_ptr<EventDataPrintLuaError> eventDataPrintLuaError(new EventDataPrintLuaError(this->scriptPathName, this->scriptFilePathName, -1, errorString));
				NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataPrintLuaError);
			}
		}
#endif
	}

}; // namespace end
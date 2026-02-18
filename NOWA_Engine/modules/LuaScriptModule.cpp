#include "NOWAPrecompiled.h"
#include "modules/LuaScriptModule.h"
#include "main/Core.h"
#include "main/Events.h"
#include "main/AppStateManager.h"

namespace
{
	Ogre::String replaceAllIgnoreCase(Ogre::String source, Ogre::String what, const Ogre::String& withWhat)
	{
		if (true == what.empty())
			return source;

		Ogre::String res = source;
		std::transform(source.begin(), source.end(), source.begin(), ::tolower);
		std::transform(what.begin(), what.end(), what.begin(), ::tolower);

		size_t pos = source.rfind(what);
		while (pos != std::string::npos)
		{
			size_t oldLength = (res.begin() + pos + what.length()) - (res.begin() + pos);
			res.replace(pos, oldLength, withWhat);

			if (pos == 0)
				return res;

			pos = source.rfind(what, pos - 1);
		}

		return res;
	}

	Ogre::String replaceAll(Ogre::String str, const Ogre::String& from, const Ogre::String& to)
	{
		size_t startPos = 0;
		while ((startPos = str.find(from, startPos)) != Ogre::String::npos)
		{
			str.replace(startPos, from.length(), to);
			startPos += to.length(); // Handles case where 'to' is a substring of 'from'
		}
		return str;
	}
}

namespace NOWA
{
	LuaScriptModule::LuaScriptModule(const Ogre::String& appStateName)
		: appStateName(appStateName)
	{
		Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_TRIVIAL, "[LuaScriptModule] Module created");
	}

	LuaScriptModule::~LuaScriptModule()
	{
		
	}

	LuaScript* LuaScriptModule::getScript(const Ogre::String& scriptName)
	{
		auto& it = this->scripts.find(scriptName);
		if (it == this->scripts.end())
		{
			return nullptr;
		}
		return it->second;
	}

	LuaScript* LuaScriptModule::createScript(const Ogre::String& name, const Ogre::String& scriptName, bool isGlobal)
	{
		auto& it = this->scripts.find(name);
		if (it != this->scripts.end())
		{
			// Ogre::LogManager::getSingletonPtr()->logMessage("[LuaScriptApi]: Script: " + name + " does already exist!", Ogre::LML_CRITICAL);
			// return it->second;
			// boost::shared_ptr<EventDataFeedback> eventDataNavigationMeshFeedback(new EventDataFeedback(false, "#{LuaScriptAlreadyExisting} -> : " + name));
			// NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataNavigationMeshFeedback);

			// It may be desired to use the same script for several game object if they should behave the same, also for performance reasons
			return it->second;
		}
		LuaScript* luaScript = new LuaScript(name, scriptName, isGlobal);
		this->scripts.emplace(name, luaScript);

		boost::shared_ptr<NOWA::EventDataResourceCreated> eventDataResourceCreated(new NOWA::EventDataResourceCreated());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataResourceCreated);

		return luaScript;
	}

	LuaScript* LuaScriptModule::createScript(const Ogre::String& name, const Ogre::String& scriptName, const Ogre::String& scriptContent, bool isGlobal)
	{
		auto& it = this->scripts.find(name);
		if (it != this->scripts.end())
		{
			// Ogre::LogManager::getSingletonPtr()->logMessage("[LuaScriptApi]: Script: " + name + " does already exist!", Ogre::LML_CRITICAL);
			// Sent event with feedback
			boost::shared_ptr<EventDataFeedback> eventDataNavigationMeshFeedback(new EventDataFeedback(false, "#{LuaScriptAlreadyExisting} -> : " + name));
			NOWA::AppStateManager::getSingletonPtr()->getEventManager(this->appStateName)->queueEvent(eventDataNavigationMeshFeedback);
			// return it->second;
			return nullptr;
		}

		// Get the file contents
		LuaScript* luaScript = new LuaScript(name, scriptName, isGlobal, scriptContent);
		this->scripts.emplace(name, luaScript);

		boost::shared_ptr<NOWA::EventDataResourceCreated> eventDataResourceCreated(new NOWA::EventDataResourceCreated());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataResourceCreated);

		return luaScript;
	}

	void LuaScriptModule::destroyScript(const Ogre::String& name)
	{
		auto& it = this->scripts.find(name);
		if (it != this->scripts.end())
		{
			LuaScript* luaScript = it->second;
			this->scripts.erase(name);
			delete luaScript;
		}
	}

	void LuaScriptModule::destroyScript(LuaScript* luaScript)
	{
		if (nullptr != luaScript)
			destroyScript(luaScript->getName());
	}

	bool LuaScriptModule::hasScript(const Ogre::String& name)
	{
		return this->scripts.find(name) != this->scripts.end();
	}

	void LuaScriptModule::copyScript(const Ogre::String& sourceScriptName, const Ogre::String& targetScriptName, bool deleteSourceScript, bool isGlobal)
	{
		Ogre::String tempSourceScriptFilePathName;
		if (false == isGlobal)
		{
			tempSourceScriptFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "/" + sourceScriptName;
		}
		else
		{
			tempSourceScriptFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + sourceScriptName;
		}

		Ogre::String tempTargetScriptFilePathName;
		if (false == isGlobal)
		{
			tempTargetScriptFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "/" + targetScriptName;
		}
		else
		{
			tempTargetScriptFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + targetScriptName;
		}

		this->internalCopyScript(tempSourceScriptFilePathName, tempTargetScriptFilePathName, deleteSourceScript, false, isGlobal);
	}

	void LuaScriptModule::copyScriptAbsolutePath(const Ogre::String& sourceScriptFilePathName, const Ogre::String& targetScriptFilePathName, bool deleteSourceScript, bool isGlobal)
	{
		this->internalCopyScript(sourceScriptFilePathName, targetScriptFilePathName, deleteSourceScript, true, isGlobal);
	}

	void LuaScriptModule::internalCopyScript(const Ogre::String& sourceScriptFilePathName, const Ogre::String& targetScriptFilePathName, bool deleteSourceScript, bool isAbsolutePath, bool isGlobal)
	{
		Ogre::String sourceScriptName = Core::getSingletonPtr()->getFileNameFromPath(sourceScriptFilePathName);
		Ogre::String targetScriptName = Core::getSingletonPtr()->getFileNameFromPath(targetScriptFilePathName);

		Ogre::String sourceModuleName = sourceScriptName;
		sourceModuleName.erase(sourceModuleName.find_last_of("."), Ogre::String::npos);
		
		Ogre::String targetModuleName = targetScriptName;
		targetModuleName.erase(targetModuleName.find_last_of("."), Ogre::String::npos);

		Ogre::String targetModuleCommand = "module(\"" + targetModuleName + "\"";
		Ogre::String fullTargetModuleCommand = targetModuleCommand + ", package.seeall);\n";

		// Get source content (internally replace old names with new ones)
		Ogre::String luaInFileContent;
		if (false == isAbsolutePath)
		{
			luaInFileContent = this->getScriptAdaptedContent(sourceScriptName, targetModuleName, isGlobal, sourceModuleName);
		}
		else
		{
			luaInFileContent = this->getScriptAdaptedContentAbsolute(sourceScriptFilePathName, targetModuleName, sourceModuleName);
		}

		// Empty content, do nothing
		if (true == luaInFileContent.empty())
		{
			return;
		}

		// Write to target file
		std::ofstream luaOutFile(targetScriptFilePathName);
		if (true == luaOutFile.good())
		{
			luaOutFile << luaInFileContent;
			luaOutFile.close();

			boost::shared_ptr<NOWA::EventDataResourceCreated> eventDataResourceCreated(new NOWA::EventDataResourceCreated());
			NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataResourceCreated);
		}

		if (true == deleteSourceScript)
		{
			::DeleteFile(sourceScriptFilePathName.c_str());
		}

		boost::shared_ptr<NOWA::EventDataResourceCreated> eventDataResourceCreated(new NOWA::EventDataResourceCreated());
		NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataResourceCreated);
	}

	bool LuaScriptModule::checkLuaScriptFileExists(const Ogre::String& scriptName, bool isGlobal, const Ogre::String& filePath)
	{
		Ogre::String tempScriptFilePathName;
		if (true == filePath.empty())
		{
			if (false == isGlobal)
			{
				tempScriptFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "/" + scriptName;
			}
			else
			{
				tempScriptFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + scriptName;
			}
		}
		else
		{
			tempScriptFilePathName = filePath + "/" + scriptName;
		}

		// Note: if its a read only file, it can only be opened with the flag ios::in
		std::fstream luaInFile(tempScriptFilePathName, std::ios::in);
		if (true == luaInFile.good())
		{
			return true;
		}
		return false;
	}

	Ogre::String LuaScriptModule::getValidatedLuaScriptName(const Ogre::String& scriptName, bool isGlobal, const Ogre::String& filePath)
	{
		Ogre::String validatedScriptName = scriptName;
		// If a lua  script with this name does already exist, rename it
		if (false == this->checkLuaScriptFileExists(scriptName, isGlobal, filePath))
		{
			return validatedScriptName;
		}
		else
		{
			unsigned int id = 0;
			
			do
			{
				// remove .lua
				validatedScriptName = validatedScriptName.substr(0, validatedScriptName.size() - 4);

				size_t found = validatedScriptName.rfind("_");
				// id > 0, if the name e.g. is from the beginning: Level1_MainGameObject.lua, do not search for "_", else Level1_ would only be used!
				if (Ogre::String::npos != found && id > 0)
				{
					validatedScriptName = validatedScriptName.substr(0, found + 1);
				}
				else
				{
					validatedScriptName += "_";
				}
				// Do not use # anymore, because its reserved in mygui as code-word the # and everything after that will be removed!
				validatedScriptName += Ogre::StringConverter::toString(id++) + ".lua";
			} while (true == this->checkLuaScriptFileExists(validatedScriptName, isGlobal, filePath));
		}

		return validatedScriptName;
	}

	Ogre::String LuaScriptModule::getScriptAdaptedContent(const Ogre::String& scriptName, const Ogre::String& targetModuleName, bool isGlobal, const Ogre::String& sourceModuleName)
	{
		Ogre::String tempScriptFilePathName;
		if (false == isGlobal)
		{
			tempScriptFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "/" + scriptName;
		}
		else
		{
			tempScriptFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + scriptName;
		}

		return this->internalGetScriptAdaptedContent(tempScriptFilePathName, targetModuleName, sourceModuleName);
	}

	Ogre::String LuaScriptModule::getScriptAdaptedContentAbsolute(const Ogre::String& scriptFilePathName, const Ogre::String& targetModuleName, const Ogre::String& sourceModuleName)
	{
		return this->internalGetScriptAdaptedContent(scriptFilePathName, targetModuleName, sourceModuleName);
	}

	Ogre::String LuaScriptModule::internalGetScriptAdaptedContent(const Ogre::String& scriptFilePathName, const Ogre::String& targetModuleName, const Ogre::String& sourceModuleName)
	{
		// Get source content
		std::fstream luaInFile(scriptFilePathName, std::ios::in);
		Ogre::String luaInFileContent;
		if (true == luaInFile.good())
		{
			luaInFileContent = std::string{ std::istreambuf_iterator<char>{luaInFile}, std::istreambuf_iterator<char>{} };
			if (GetFileAttributes(scriptFilePathName.data()) & NOWA::FileFlag)
			{
				luaInFileContent = Core::getSingletonPtr()->decode64(luaInFileContent, true);

				// Write the readable version, because other scripts require this script in readable form
				std::ofstream outFile(scriptFilePathName);
				if (true == outFile.good())
				{
					outFile << luaInFileContent;
					outFile.close();

					boost::shared_ptr<NOWA::EventDataResourceCreated> eventDataResourceCreated(new NOWA::EventDataResourceCreated());
					NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataResourceCreated);
				}
			}

			if (false == targetModuleName.empty() && true == sourceModuleName.empty())
			{
				// Replace module name
				size_t modulePos = luaInFileContent.find("module(\"") + 8;

				// Get the length of the modulename inside the file
				size_t moduleEndPos = luaInFileContent.find("\"", modulePos);

				// e.g. "barrel_0" = 8, but "barrel_10" = 9
				size_t oldModuleLength = moduleEndPos - modulePos;

				luaInFileContent.replace(modulePos, oldModuleLength, targetModuleName);
			}
			else
			{
				// Replace also all table names e.g. "barrel_0[connect] = function" with "barrel_1[connect] = function"
				if (sourceModuleName != targetModuleName)
				{
					luaInFileContent = replaceAll(luaInFileContent, sourceModuleName, targetModuleName);
				}
			}

			luaInFile.close();
		}
		return luaInFileContent;
	}

	void LuaScriptModule::replaceIdsInScript(const Ogre::String& scriptFilePathName, const Ogre::String& sourceId, const Ogre::String& targetId)
	{
		// Get source content
		std::fstream luaInFile(scriptFilePathName, std::ios::in);
		Ogre::String luaInFileContent;
		if (true == luaInFile.good())
		{
			luaInFileContent = std::string{ std::istreambuf_iterator<char>{luaInFile}, std::istreambuf_iterator<char>{} };
			if (GetFileAttributes(scriptFilePathName.data()) & NOWA::FileFlag)
			{
				luaInFileContent = Core::getSingletonPtr()->decode64(luaInFileContent, true);
			}

			luaInFile.close();

			if (false == sourceId.empty())
			{
				luaInFileContent = replaceAll(luaInFileContent, sourceId, targetId);
				std::ofstream outFile(scriptFilePathName);
				if (true == outFile.good())
				{
					outFile << luaInFileContent;
					outFile.close();

					boost::shared_ptr<NOWA::EventDataResourceCreated> eventDataResourceCreated(new NOWA::EventDataResourceCreated());
					NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataResourceCreated);
				}
			}
		}
	}

	bool LuaScriptModule::checkLuaFunctionAvailable(const Ogre::String& scriptName, const Ogre::String& functionName)
	{
		LuaScript* luaScript = this->getScript(scriptName);
		if (nullptr != luaScript)
		{
			const Ogre::String& scriptContent = luaScript->getScriptContent();
			size_t functionNameStart = scriptContent.find(functionName);
			if (Ogre::String::npos != functionNameStart)
			{
				Ogre::String strBefore = scriptContent.substr(functionNameStart - 2, 2);
				Ogre::String strAfter = scriptContent.substr(functionNameStart + functionName.length(), 2);
				if (functionNameStart > 2 && strBefore == "[\"" && strAfter == "\"]")
				{
					return true;
				}
				Ogre::String strBefore2 = scriptContent.substr(functionNameStart - 9, 9);
				if (functionNameStart > 9 && strBefore2 == "function ")
				{
					return true;
				}
			}
		}
		return false;
	}

	bool LuaScriptModule::checkLuaStateAvailable(const Ogre::String& scriptName, const Ogre::String& stateName)
	{
		LuaScript* luaScript = this->getScript(scriptName);
		if (nullptr != luaScript)
		{
			const Ogre::String& scriptContent = luaScript->getScriptContent();

			if (true == scriptContent.empty())
			{
				return false;
			}

			size_t after = 0;
			bool foundEqual = false;
			bool foundOpenBracket = false;
			bool foundClosedBracket = false;

			do
			{
				foundEqual = false;
				foundOpenBracket = false;
				foundClosedBracket = false;

				size_t stateNameStart = scriptContent.find(stateName, after);

				if (Ogre::String::npos != stateNameStart)
				{
					after = stateNameStart + stateName.length();

					Ogre::String ch;
					do
					{
						ch = scriptContent[after];
						if (ch == "=")
						{
							foundEqual = true;
						}
						else if (ch == "{" && true == foundEqual)
						{
							foundOpenBracket = true;
						}
						else if (ch == "}" && true == foundEqual && true == foundOpenBracket)
						{
							foundClosedBracket = true;
						}
						after++;

						if (true == foundEqual && true == foundOpenBracket && true == foundClosedBracket)
						{
							return true;
						}

					} while (ch != "\n");
				}
				else
				{
					return false;
				}

			} while (after <= scriptContent.size());

			return foundEqual && foundOpenBracket && foundClosedBracket;
		}
		return false;
	}

	void LuaScriptModule::generateLuaFunctionName(const Ogre::String& scriptName, const Ogre::String& functionName, bool isGlobal)
	{
		Ogre::String tempScriptFilePathName;
		if (false == isGlobal)
		{
			tempScriptFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + Core::getSingletonPtr()->getSceneName() + "/" + scriptName;
		}
		else
		{
			tempScriptFilePathName = Core::getSingletonPtr()->getCurrentProjectPath() + "/" + scriptName;
		}

		// Get source content
		std::fstream luaInFile(tempScriptFilePathName, std::ios::in);
		Ogre::String luaInFileContent;
		if (true == luaInFile.good())
		{
			if (GetFileAttributes(tempScriptFilePathName.data()) & NOWA::FileFlag)
			{
				Ogre::LogManager::getSingletonPtr()->logMessage(Ogre::LML_NORMAL, "[LuaScriptModule]: Warning Could not generate lua function, because the script is encoded! Please decode first all scripts!");
				return;
			}

			luaInFileContent = std::string{ std::istreambuf_iterator<char>{luaInFile}, std::istreambuf_iterator<char>{} };

			luaInFile.close();

			Ogre::String moduleName = scriptName;
			size_t dotPos = scriptName.find_last_of(".");
			if (Ogre::String::npos != dotPos)
				moduleName.erase(dotPos, Ogre::String::npos);
			else
				moduleName = scriptName;

			Ogre::String onlyFunction;
			Ogre::String parameter;
			Ogre::String componentName;
			Ogre::String className;
			size_t found = functionName.find("(");
			if (found != Ogre::String::npos)
			{
				onlyFunction = functionName.substr(0, found);
				if (true == onlyFunction.empty())
					return;
				parameter = functionName.substr(found, functionName.length() - found);
				found = functionName.find(",", found);
				if (found != Ogre::String::npos)
				{
					componentName = functionName.substr(found, functionName.length() - found);
				}
			}
			else
			{
				return;
			}

			found = functionName.find("=");
			if (found != Ogre::String::npos)
			{
				className = functionName.substr(found, functionName.length() - found);
			}
			luaInFileContent += "\n\n" + moduleName + "[\"" + onlyFunction + "\"] = function" + parameter;

			if (false == className.empty())
			{
				luaInFileContent += "\t" + componentName = "AppStateManager:getGameObjectController():cast" + className + "(" + componentName + ");\n\n";
			}

			luaInFileContent += "\n\nend";

			// Write the readable version, because other scripts require this script in readable form
			std::ofstream outFile(tempScriptFilePathName);
			if (true == outFile.good())
			{
				outFile << luaInFileContent;
				outFile.close();

				boost::shared_ptr<NOWA::EventDataResourceCreated> eventDataResourceCreated(new NOWA::EventDataResourceCreated());
				NOWA::AppStateManager::getSingletonPtr()->getEventManager()->queueEvent(eventDataResourceCreated);
			}
		}
	}

	void LuaScriptModule::destroyContent(void)
	{
		auto& it = this->scripts.begin();
		while (it != this->scripts.end())
		{
			LuaScript* luaScript = it->second;
			delete luaScript;
			++it;
		}
		this->scripts.clear();
	}

} // namespace end
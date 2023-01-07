#include "NOWAPrecompiled.h"
#include "ScriptEvent.h"
#include "modules/LuaScriptApi.h"

extern "C"
{
	#include <lua.h>
	#include <lualib.h>
	#include <lauxlib.h>
}

using namespace luabind;

namespace NOWA
{
	ScriptEvent::CreationFunctions ScriptEvent::s_creationFunctions;

	luabind::object ScriptEvent::getEventData(void)
	{
		if (false == this->eventDataIsValid)
		{
			this->buildEventDataFromCppToLuaScript();
			this->eventDataIsValid = true;
		}
	
		return this->eventData;
	}

	bool ScriptEvent::setEventData(luabind::object eventData)
	{
		this->eventData = eventData;
		this->eventDataIsValid = this->buildEventFromLuaScriptToCpp();
		return this->eventDataIsValid;
	}

	void ScriptEvent::registerEventTypeWithScript(const char* key, EventType eventType)
	{
//		// Get or create the EventType table
//		luabind::object eventTypeTable = globals(LuaScriptApi::getInstance()->getLua());
//		if (eventTypeTable["EventType"])
//		{
//			eventTypeTable = newtable(LuaScriptApi::getInstance()->getLua()); 
// 
//// Attention: Is this correct?
//			// luabind::object object(luabind::from_stack(NOWA::LuaScriptApi::getInstance()->getLua(), -1));
//			luabind::object list = luabind::newtable(LuaScriptApi::getInstance()->getLua());
//			eventTypeTable = object["EventType"];
//		}

		object eventTypeTable = globals(LuaScriptApi::getInstance()->getLua())["EventType"];
		if (false == eventTypeTable.is_valid())
		{
			luabind::object object = newtable(LuaScriptApi::getInstance()->getLua());
			// luabind::object object(luabind::from_stack(NOWA::LuaScriptApi::getInstance()->getLua(), -1));
			// Get the object table for the table name, to call functions for that table
			eventTypeTable = object["EventType"];
		}
		
		// globals(LuaScriptApi::getInstance()->getLua())["EventType"] = luabind::nil;
	
		// Add the entry
		settable(eventTypeTable, key, (double)eventType); 

		// eventTypeTable.SetNumber(key, (double)type);
	}

	void ScriptEvent::addCreationFunction(EventType eventType, CreateEventForScriptFunctionType creationFunctionPtr)
	{
		s_creationFunctions.emplace(eventType, creationFunctionPtr);
	}

	ScriptEvent* ScriptEvent::createEventFromScript(EventType eventType)
	{
		CreationFunctions::iterator findIt = s_creationFunctions.find(eventType);
		if (findIt != s_creationFunctions.end())
		{
			CreateEventForScriptFunctionType func = findIt->second;
			return func(eventType);
		}
		else
		{
			return nullptr;
		}
	}

	ScriptEvent* ScriptEvent::instantiateScript(EventType eventType)
	{ 
		return new ScriptEvent(eventType);
	}

	void ScriptEvent::buildEventDataFromCppToLuaScript(void)
	{
		/*lua_fastunref(L, ref);
		L = LuaState_to_lua_State(state);
		lua_pushnil(L);
		ref = lua_fastref(L);*/

		// this->eventData = luabind::nil;
		// http://oberon00.github.io/luabind/object.html
		this->eventData(luabind::nil);
		// this->eventData.AssignNil(LuaStateManager::Get()->GetLuaState());
	}

}; // namespace end


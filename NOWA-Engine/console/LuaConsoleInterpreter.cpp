#include "NOWAPrecompiled.h"
#include "LuaConsoleInterpreter.h"

namespace NOWA
{
	// The address of this int in memory is used as a garenteed unique id
	// in the lua registry
	static const char LuaRegistryGUID = 0;

	LuaConsoleInterpreter::LuaConsoleInterpreter(lua_State *lua)
		: 
		lua(lua), 
		state(LI_READY), 
		firstLine(true)
	{
		this->output.clear();
		this->currentStatement.clear();
		// register pointer to this class and the callback function to associate the NOWA lua console with lua script for realtime exchange
		lua_pushlightuserdata(this->lua, (void *)&LuaRegistryGUID);
		lua_pushlightuserdata(this->lua, this);
		lua_settable(this->lua, LUA_REGISTRYINDEX);

		lua_register(this->lua, "interpreterOutput", &LuaConsoleInterpreter::insertOutputFromLua);

#ifdef LI_MESSAGE
		this->output = LI_MESSAGE;
#endif
		this->prompt = LI_PROMPT;
	}

	LuaConsoleInterpreter::~LuaConsoleInterpreter()
	{
		lua_register(this->lua, "interpreterOutput", NULL);
		lua_pushlightuserdata(this->lua, (void *)&LuaRegistryGUID);
		lua_pushnil(this->lua);
		lua_settable(this->lua, LUA_REGISTRYINDEX);
	}

	// Retrieves the current output from the interpreter.
	std::string LuaConsoleInterpreter::getOutput()
	{
		return this->output;
	}

	// Insert (another) line of text into the interpreter.
	LuaConsoleInterpreter::LIState LuaConsoleInterpreter::insertLine(std::string& line, bool insertInOutput)
	{
		if(insertInOutput == true)
		{
			this->output += line;
			this->output += '\n';
		}

		if(this->firstLine && line.substr(0, 1) == "=")
		{
			line = "return " + line.substr(1, line.length() - 1);
		}

		this->currentStatement += " ";
		this->currentStatement += line;
		this->firstLine = false;

		this->state = LI_READY;

		if(luaL_loadstring(this->lua, this->currentStatement.c_str() ) )
		{
			std::string error(lua_tostring(this->lua, -1));
			lua_pop(this->lua, 1);

			// If the error is not a syntax error caused by not enough of the
			// statement been yet entered...
			if(error.substr(error.length() - 6, 5) != "<eof>")
			{
				this->output += error;
				this->output += "\n";
				this->output += LI_PROMPT;
				this->currentStatement.clear();
				this->state = LI_ERROR;
			}
			// Otherwise...
			else
			{
				// Secondary prompt
				this->prompt = LI_PROMPT2;
				this->state = LI_NEED_MORE_INPUT;
			}
			return this->state;
		}
		else
		{
			// The statement compiled correctly, now run it.
			if(lua_pcall(this->lua, 0, LUA_MULTRET, 0))
			{
				// The error message (if any) will be added to the output as part
				// of the stack reporting.
				lua_gc(this->lua, LUA_GCCOLLECT, 0);     // Do a full garbage collection on errors.
				this->state = LI_ERROR;
			}
		}
		this->currentStatement.clear();
		this->firstLine = true;

		// Report stack contents
		if (lua_gettop(this->lua) > 0)
		{
			lua_getglobal(this->lua, "print");
			lua_insert(this->lua, 1);
			lua_pcall(this->lua, lua_gettop(this->lua) - 1, 0, 0);
		}

		this->prompt = LI_PROMPT;
		// Clear stack
		lua_settop(this->lua, 0);

		return this->state;
	}

	// Callback for lua to provide output.
	int LuaConsoleInterpreter::insertOutputFromLua(lua_State *lua)
	{
		// Retreive the current interpreter for current lua state.
		LuaConsoleInterpreter *pInterpreter;

		lua_pushlightuserdata(lua, (void *)&LuaRegistryGUID);
		lua_gettable(lua, LUA_REGISTRYINDEX );
		pInterpreter = static_cast<LuaConsoleInterpreter *>(lua_touserdata(lua, -1));

		if(pInterpreter)
		{
			pInterpreter->output += lua_tostring(lua, -2 );
		}

		lua_settop(lua, 0 );
		return 0;
	}

	void LuaConsoleInterpreter::clearOutput() 
	{ 
		this->output.clear();
	}

	std::string LuaConsoleInterpreter::getPrompt(void) 
	{ 
		return this->prompt; 
	}

	LuaConsoleInterpreter::LIState LuaConsoleInterpreter::getState(void) 
	{ 
		return this->state;
	}

}; //Namespace end
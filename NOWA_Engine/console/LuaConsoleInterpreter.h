#ifndef LUA_CONSOLE_INTERPRETER_H
#define LUA_CONSOLE_INTERPRETER_H

#include "defines.h"
#include <string>

extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

#include <luabind/luabind.hpp>
using namespace luabind;

namespace NOWA
{
#define LI_PROMPT  ">"
#define LI_PROMPT2 ">>"
#define LI_MESSAGE "Lua console for OBA\n"

	class EXPORTED LuaConsoleInterpreter
	{
	public:
		enum LIState
		{
			LI_READY = 0,
			LI_NEED_MORE_INPUT,
			LI_ERROR
		};

		LuaConsoleInterpreter(lua_State *lua);

		virtual ~LuaConsoleInterpreter();

		// Retrieves the current output from the interpreter.
		std::string getOutput();

		void clearOutput();

		std::string getPrompt();

		// Insert (another) line of text into the interpreter.
		// If insertInOutput is true, the line will also go into the
		// output.
		LIState insertLine(std::string& line, bool insertInOutput = true);

		// Callback for lua to provide output.
		static int insertOutputFromLua(lua_State *lua);

		// Retrieve the current state of affairs.
		LIState getState();

	protected:
		lua_State *lua;
		std::string currentStatement;
		std::string output;
		std::string prompt;
		LIState state;
		bool firstLine;
	};


}; // Namespace end

#endif
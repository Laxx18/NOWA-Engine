#ifndef LUA_SCRIPT_API_H_
#define LUA_SCRIPT_API_H_

#include "defines.h"

namespace NOWA
{
	class GameObject;
	class GameObjectController;

	class EXPORTED LuaScriptApi
	{
	public:
		friend class Core;
		
		/**
		* @brief		Gets lua object for scripting.
		* @return		lua	The lua object
		*/
		lua_State* getLua(void);

		int runInitScript(const Ogre::String& scriptName);

		void stopInitScript(const Ogre::String& scriptName);

		int pushTraceBack(void);
		
		void errorHandler();

		void errorHandler(luabind::error& err, Ogre::String file = "", Ogre::String function = "");

		void errorHandler(luabind::cast_failed& castError, Ogre::String file = "", Ogre::String function = "");

		void printErrorMessage(luabind::error& error, const Ogre::String& function);
		
		void forceGarbageCollect();

		bool functionExist(lua_State* lua, const Ogre::String& name);

		void destroyContent(void);

		void clear(void);

		void stackDump(void);

		void appendLuaFilePathToPackage(const Ogre::String& luaFilePath);

		Ogre::String getLuaInitScriptContent(void) const;

		void addClassToCollection(const Ogre::String& className, const Ogre::String& function, const Ogre::String& description);

		void addFunctionToCollection(const Ogre::String& function, const Ogre::String& description);

		const std::map<Ogre::String, std::vector<std::pair<Ogre::String, Ogre::String>>>& getClassCollection(void) const;

		void generateLuaApi(void);
	public:
		static LuaScriptApi* getInstance();
	private:
		LuaScriptApi();
		~LuaScriptApi();

		/**
		* @brief		Inits the lua script module
		* @param[in]	resourceGroupName		The name of the group for example in resources.cfg without the []
		*/
		void init(const Ogre::String& resourceGroupName);
	public:
		static int callBackCustomErrorFunction(lua_State* lua);
		static int customLuaAtPanic(lua_State* lua);
	private:
		lua_State* lua;
		Ogre::String scriptName;
		Ogre::String resourceGroupName;
		time_t scriptModifiedTime;
		std::set<Ogre::String> luaScriptPathes;
		std::map<Ogre::String, std::vector<std::pair<Ogre::String, Ogre::String>>> classCollection;
	};

}; // namespace end

#endif


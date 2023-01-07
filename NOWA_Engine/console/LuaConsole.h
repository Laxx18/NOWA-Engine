#ifndef LUA_CONSOLE_H
#define LUA_CONSOLE_H

#include "OgreOverlayContainer.h"
#include "OgreOverlayElement.h"
#include "OgreOverlayManager.h"

#include <list>
#include "EditString.h"
#include "LuaConsoleInterpreter.h"

namespace NOWA
{
	class EXPORTED LuaConsole : public Ogre::Singleton<LuaConsole>, Ogre::LogListener
	{
	public:
		LuaConsole();
		virtual ~LuaConsole();

		void init(lua_State *lua);
		void shutdown(void);
		void setVisible(bool visible);
		bool isVisible(void);
		void print(std::string text);
		bool injectKeyPress(const OIS::KeyEvent &evt);

		void update(Ogre::Real dt);

		virtual void messageLogged(const Ogre::String& message, Ogre::LogMessageLevel lml, bool maskDebug, const Ogre::String &logName, bool& skipThisMessage) override;
	public:
		static LuaConsole& getSingleton(void);

		static LuaConsole* getSingletonPtr(void);
	protected:
		void addToHistory(const std::string& cmd);
	protected:
		bool visible;
		bool textChanged;
		float height;
		int startLine;
		bool cursorBlink;
		float cursorBlinkTime;
		bool initialised;

		Ogre::v1::Overlay* pOverlay;
		Ogre::v1::OverlayContainer* pPanel;
		Ogre::v1::OverlayElement* pTextbox;

		EditString editLine;
		LuaConsoleInterpreter* pInterpreter;

		std::list<std::string> lines;
		std::list<std::string> history;

		std::list<std::string>::iterator historyLine;

		std::string clipText;
		bool controlPressed;
	};

}; // Namespace end

template<> NOWA::LuaConsole* Ogre::Singleton<NOWA::LuaConsole>::msSingleton = 0;

#endif
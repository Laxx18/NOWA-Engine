#ifndef LUA_CONSOLE_H
#define LUA_CONSOLE_H

#include "OgreOverlayContainer.h"
#include "OgreOverlayElement.h"
#include "OgreOverlayManager.h"

#include "EditString.h"
#include "LuaConsoleInterpreter.h"
#include <list>

namespace NOWA
{
    class EXPORTED LuaConsole : public Ogre::Singleton<LuaConsole>, Ogre::LogListener
    {
    public:
        LuaConsole();
        virtual ~LuaConsole();

        void init(lua_State* lua);
        void shutdown(void);
        void setVisible(bool visible);
        bool isVisible(void);
        void print(std::string text);
        bool injectKeyPress(const OIS::KeyEvent& evt);

        void update(Ogre::Real dt);

        virtual void messageLogged(const Ogre::String& message, Ogre::LogMessageLevel lml, bool maskDebug, const Ogre::String& logName, bool& skipThisMessage) override;

        void clearHistory(void);
        void clearConsole(void);

        // ---------- Critical log interface ----------
        // Returns the number of LML_CRITICAL messages received since the last clearCriticalLogs() call.
        unsigned int getCriticalLogCount(void) const;

        // Returns every LML_CRITICAL message received since the last clearCriticalLogs() call.
        const std::list<std::string>& getCriticalLogs(void) const;

        // Resets the critical-log list and counter to zero.
        void clearCriticalLogs(void);
        // -------------------------------------------

    public:
        static LuaConsole& getSingleton(void);
        static LuaConsole* getSingletonPtr(void);

    protected:
        void addToHistory(const std::string& cmd);
        // Fires EventDataPrintOgreLog on the main-thread event queue.
        // Must only be called after AppStateManager / EventManager are up.
        void fireOgreLogEvent(const Ogre::String& message);

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

        // Critical log storage
        std::list<std::string> criticalLogs;
        unsigned int criticalLogCount;
    };

}; // Namespace end

template <> NOWA::LuaConsole* Ogre::Singleton<NOWA::LuaConsole>::msSingleton = 0;

#endif
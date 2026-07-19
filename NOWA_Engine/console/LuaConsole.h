#ifndef LUA_CONSOLE_H
#define LUA_CONSOLE_H

#include "EditString.h"
#include "LuaConsoleInterpreter.h"
#include <atomic>
#include <list>

namespace MyGUI
{
    class Widget;
    class EditBox;
    class ControllerItem;
}

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

        // Render-thread callback (MyGUI::ControllerItem::eventPostAction) for
        // the slide-closed animation. Only hides the widgets (render-thread
        // safe); the camera-weight restore is handed off to update() via the
        // atomics below, since the original called those setters from the
        // logic thread (update()), not from a render-thread callback.
        void onSlideFinished(MyGUI::Widget* sender, MyGUI::ControllerItem* controller);

    protected:
        bool visible;
        bool textChanged;
        int startLine;
        bool cursorBlink;
        float cursorBlinkTime;
        bool initialised;

        // Set (from any thread) when the camera weight needs to change;
        // consumed once by update() on the logic thread, matching the
        // original's threading (it called CameraManager setters directly
        // from update(), never wrapped in a render command).
        std::atomic<bool> pendingCameraWeightChange;
        std::atomic<float> pendingCameraWeightValue;

        // v2: replaces Ogre::v1::Overlay / OverlayContainer / OverlayElement.
        // Both are independent root-level MyGUI widgets (NOT parent/child —
        // see FaderProcess.cpp-style port notes), created once in init() and
        // driven every frame in update() using the same two independent
        // position/size formulas the original overlay code used.
        MyGUI::Widget* pPanel;
        MyGUI::EditBox* pTextbox;

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